import cv2
import numpy as np
import random
import time
import socket
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore

# 1. CONFIGURAÇÃO FIRESTORE
try:
    cred = credentials.Certificate("chave_firebase.json")
    firebase_admin.initialize_app(cred)
    db = firestore.client()
    print("FIRESTORE LIGADO!")
except Exception as e:
    print(f"ERRO FIREBASE: {e}")
    exit()

# Memória PARA NAO ESTAR SEMPRE ENVIAR PARA FIREBASE VOU USAR APRA CONFIRMAR SE ESTA O MESMO VALOR
cache_valores = {}


#  FUNÇÃO DE VISAO POR PC COM O QUE APRENDEMOS EM VISAO PC

def processar_imagem(img_referencia, img_atual):
    gray_ref = cv2.cvtColor(img_referencia, cv2.COLOR_BGR2GRAY)
    gray_atual = cv2.cvtColor(img_atual, cv2.COLOR_BGR2GRAY)


    gray_ref = cv2.GaussianBlur(gray_ref, (13, 13), 0)
    gray_atual = cv2.GaussianBlur(gray_atual, (13, 13), 0)

    diferenca = cv2.absdiff(gray_ref, gray_atual)
    _, mascara = cv2.threshold(diferenca, 25, 255, cv2.THRESH_BINARY)

    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (15, 15))
    mascara = cv2.morphologyEx(mascara, cv2.MORPH_CLOSE, kernel)
    mascara = cv2.dilate(mascara, kernel, iterations=1)

    contornos, _ = cv2.findContours(mascara.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    objetos_encontrados = 0

    for contorno in contornos:
        if cv2.contourArea(contorno) < 500: continue  # Filtra ruído

        objetos_encontrados += 1
        (x, y, w, h) = cv2.boundingRect(contorno)
        cv2.rectangle(img_atual, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.putText(img_atual, "OBJETO", (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    return img_atual, mascara, objetos_encontrados


# SERVIDOR PRINCIPAL SOCKET PARA LIGAR AO QT CODIGO QUE USEI DO PROF ,+-
def start_server():
    # Carregar imagens
    img_vazio = cv2.imread("carro_vazio.png")
    lista_imagens = ["carro_vazio.png", "carro_tel.png", "carro_mala.png"]

    if img_vazio is None:
        print("ERRO: Faltam as imagens na pasta!")
        return

    # Configurar Socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('localhost', 12345))
    server_socket.listen(5)
    print("📡 Servidor Python à escuta na porta 12345...")

    while True:
        try:
            client, addr = server_socket.accept()
            print(f"🔗 Qt Conectado: {addr}")

            # IMPORTANTE: Timeout  PARA SA IMAGENS NAO CRASHAR COMPLETO PC
            client.settimeout(0.1)

            ultimo_envio_objetos = -1

            while True:

                #  PROCESSAR IMAGEM (SIMULAÇÃO)
                # Escolhe imagem aleatória (SIMULO ASSIM UMA FRAME DA WEBCAM
                nome_img = random.choice(lista_imagens)
                img_atual = cv2.imread(nome_img)

                # Resize para processar mais rápido e caber no ecrã
                vazio_rs = cv2.resize(img_vazio, (640, 480))
                atual_rs = cv2.resize(img_atual, (640, 480))

                res, masc, num_objs = processar_imagem(vazio_rs, atual_rs)

                # Mostrar Janelas
                cv2.imshow("Camera", res)
                cv2.imshow("Visao do Robo", masc)
                cv2.waitKey(1)  # COMO USAMOS EM VPC

                # Se o número de objetos mudou, atualiza Firebase e avisa Qt
                if num_objs != ultimo_envio_objetos:
                    # 1. Firebase
                    db.collection(u'Monitorizacao').document(u'Carro01').set({
                        "objetos_esquecidos": num_objs,
                        "alerta_interior": (num_objs > 0)
                    }, merge=True)

                    # 2. Enviar para o Qt (DASHBOARD)
                    try:
                        mensagem_qt = f"objetos:{num_objs}\n"
                        client.send(mensagem_qt.encode("utf-8"))
                        print(f" Enviado para Qt: {mensagem_qt.strip()}")
                    except Exception as e:
                        print(f"Erro ao enviar para Qt: {e}")

                    ultimo_envio_objetos = num_objs


                # B. LER DADOS DO STM (VIA QT) E MANDAR P/ FIREBASE
                try:
                    # Tenta ler do socket
                    msg = client.recv(1024).decode("utf-8")

                    if not msg: break  # Conexão caiu

                    linhas = msg.strip().split('\n')
                    for linha in linhas:
                        if ":" in linha:
                            chave, valor = linha.split(":", 1)
                            chave = chave.strip()
                            valor = valor.strip()

                            # Converter numero
                            try:
                                val_final = float(valor) if "." in valor else int(valor)
                            except:
                                val_final = valor

                            # Só envia para Firebase se mudou (Cache)
                            if chave not in cache_valores or cache_valores[chave] != val_final:
                                cache_valores[chave] = val_final
                                db.collection(u'Monitorizacao').document(u'Carro01').set({chave: val_final}, merge=True)
                                print(f" Sensor Gravado: {chave} -> {val_final}")

                except socket.timeout:
                    # Normal! Significa que o Qt não enviou nada neste ciclo.
                    # Continuamos o loop para processar a próxima imagem.
                    pass
                except Exception as e:
                    print(f"Erro socket: {e}")
                    break

                time.sleep(1)

        except Exception as e:
            print(f"Reiniciando servidor... {e}")
            cv2.destroyAllWindows()


if __name__ == "__main__":
    start_server()