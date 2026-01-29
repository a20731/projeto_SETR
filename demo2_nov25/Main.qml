import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Window {
    id: win
    width: 850
    height: 550
    visible: true
    title: qsTr("Dashboard Pro")
    color: "#121212"

    // Status do Python (Canto inferior direito)
    Text {
        objectName: "statusPy"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 10
        text: "PYTHON: A LIGAR..."
        color: "yellow"
        font.pixelSize: 12
        z: 10 // Garante que fica por cima de tudo
    }

    // ===============================================================
    // RECEBORES DE DADOS (INVISÍVEIS)
    // ===============================================================
    Item {
        id: dadosSensor

        property int temperatura: parseInt(recebeTemp.text)
        Text { id: recebeTemp; objectName: "valTemp"; text: "0"; visible: false }

        property bool temColisao: recebeColisao.text === "1"
        Text { id: recebeColisao; objectName: "valColisao"; text: "0"; visible: false }

        property bool estaCapotado: recebeCaputado.text === "1"
        Text { id: recebeCaputado; objectName: "valCaputado"; text: "0"; visible: false }

        property bool temFumo: recebeFumo.text === "1"
        Text { id: recebeFumo; objectName: "valFumo"; text: "0"; visible: false }

        property int numObjetos: parseInt(recebeObj.text)
        Text { id: recebeObj; objectName: "valObjetos"; text: "0"; visible: false }
    }

    // ===============================================================
    // INTERFACE VISUAL
    // ===============================================================

    Text {
        text: "SISTEMA DE SEGURANÇA INTELIGENTE"
        color: "white"
        font.bold: true
        font.pixelSize: 24
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
    }

    RowLayout {
        anchors.centerIn: parent
        spacing: 40

        // --- LADO ESQUERDO: GAUGE ---
        Rectangle {
            width: 250; height: 250
            color: "#1e1e1e"
            radius: 20
            border.color: "#333"
            border.width: 1

            Text {
                text: "TEMPERATURA"
                color: "#aaaaaa"
                font.bold: true
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 15
            }

            Canvas {
                id: gaugeCanvas
                anchors.centerIn: parent
                width: 200; height: 200
                property int valorAtual: dadosSensor.temperatura
                onValorAtualChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    var centroX = width / 2;
                    var centroY = height / 2;
                    var raio = 80;

                    ctx.beginPath();
                    ctx.arc(centroX, centroY, raio, Math.PI * 0.8, Math.PI * 2.2);
                    ctx.lineWidth = 15;
                    ctx.strokeStyle = "#333";
                    ctx.stroke();

                    var maxTemp = 50.0;
                    var pct = Math.min(valorAtual / maxTemp, 1.0);
                    var startAngle = Math.PI * 0.8;
                    var endAngle = startAngle + (pct * (Math.PI * 1.4));

                    ctx.beginPath();
                    ctx.arc(centroX, centroY, raio, startAngle, endAngle);
                    ctx.lineWidth = 15;
                    if(valorAtual < 20) ctx.strokeStyle = "#00ccff";
                    else if(valorAtual < 30) ctx.strokeStyle = "#00ff00";
                    else ctx.strokeStyle = "#ff3300";
                    ctx.lineCap = "round";
                    ctx.stroke();
                }
                Behavior on valorAtual { NumberAnimation { duration: 500; easing.type: Easing.OutQuad } }
            }

            Column {
                anchors.centerIn: parent
                Text {
                    text: dadosSensor.temperatura
                    color: "white"
                    font.pixelSize: 48
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "°C"
                    color: "#888"
                    font.pixelSize: 18
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // --- LADO DIREITO: ALERTAS ---
        ColumnLayout {
            spacing: 15

            // --- COMPONENTE CORRIGIDO ---
            component AlertaBox : Rectangle {
                id: rootBox // <--- DEI UM NOME AO COMPONENTE

                property string titulo: "ALERTA"
                property bool ativo: false
                property color corAtivo: "red"
                property string valorExtra: ""

                Layout.preferredWidth: 260
                Layout.preferredHeight: 70

                // Se ativo usa a cor, se não usa cinzento escuro
                color: ativo ? corAtivo : "#2c2c2c"
                radius: 10
                border.color: ativo ? "white" : "transparent"
                border.width: 1

                SequentialAnimation on opacity {
                    running: ativo
                    loops: Animation.Infinite
                    PropertyAnimation { to: 0.7; duration: 500 }
                    PropertyAnimation { to: 1.0; duration: 500 }
                }
                onAtivoChanged: { if(!ativo) opacity = 1.0 }

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 15

                    // Bolinha LED
                    Rectangle {
                        width: 15; height: 15; radius: 10
                        color: rootBox.ativo ? "white" : "#444" // Usa rootBox.ativo
                    }

                    Column {
                        // Título
                        Text {
                            text: rootBox.titulo // Usa rootBox.titulo (AGORA FUNCIONA)

                            // Mudei a cor de "desligado" para #cccccc (mais claro) para veres melhor
                            color: rootBox.ativo ? "white" : "#cccccc"
                            font.bold: true
                            font.pixelSize: 16
                        }

                        // Valor Extra (Contador de objetos)
                        Text {
                            visible: rootBox.valorExtra !== ""
                            text: rootBox.valorExtra
                            color: "white"
                            font.bold: true
                            font.pixelSize: 12
                        }
                    }
                }
            }

            // 1. Alerta COLISÃO
            AlertaBox {
                titulo: "COLISÃO"
                ativo: dadosSensor.temColisao
                corAtivo: "#ff3333"
            }

            // 2. Alerta CAPOTAMENTO
            AlertaBox {
                titulo: "CAPOTAMENTO"
                ativo: dadosSensor.estaCapotado
                corAtivo: "#ff8800"
            }

            // 3. Alerta FUMO
            AlertaBox {
                titulo: "FUMO / INCÊNDIO"
                ativo: dadosSensor.temFumo
                corAtivo: "#aa00ff"
            }

            // 4. OBJETOS ESQUECIDOS
            AlertaBox {
                titulo: "OBJETOS NO CARRO"
                ativo: dadosSensor.numObjetos > 0
                corAtivo: "#0055ff"
                valorExtra: dadosSensor.numObjetos > 0 ? dadosSensor.numObjetos + " DETETADOS" : ""
            }
        }
    }
}
