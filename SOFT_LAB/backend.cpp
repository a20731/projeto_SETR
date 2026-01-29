#include "backend.h"
#include <QDebug>

Backend::Backend(QObject *parent, QQmlApplicationEngine *engine)
    : QObject{parent}
{
    port = new QSerialPort(this);

    port->setPortName("COM7");
    port->setBaudRate(115200);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::TwoStop);
    port->setFlowControl(QSerialPort::NoFlowControl);

    if (port->open(QIODevice::ReadWrite)) {
        qDebug() << "Porta COM7 Aberta com Sucesso!";
    } else {
        qDebug() << "ERRO a abrir COM7: Verifica se o Serial Monitor do Arduino está fechado!";
    }

    connect(port, SIGNAL(readyRead()), this, SLOT(readPort()));

    // Ligar o C++ aos objetos do QML através do "objectName"
    txtTemp     = engine->rootObjects().at(0)->findChild<QObject*>("valTemp");
    txtColisao  = engine->rootObjects().at(0)->findChild<QObject*>("valColisao");
    txtCaputado = engine->rootObjects().at(0)->findChild<QObject*>("valCaputado");
    txtFumo     = engine->rootObjects().at(0)->findChild<QObject*>("valFumo");
}

void Backend::readPort()
{
    // Adiciona o que chegou da porta série ao nosso buffer
    buffer_leitura += port->readAll();

    // Enquanto houver um "Enter" (\n) no buffer, significa que temos uma linha completa para ler
    while (buffer_leitura.contains('\n')) {
        int fimDeLinha = buffer_leitura.indexOf('\n');
        QString linha = buffer_leitura.left(fimDeLinha).trimmed(); // Pega a linha e limpa espaços
        buffer_leitura.remove(0, fimDeLinha + 1); // Remove a linha lida do buffer

        // ----- PROCESSAMENTO DOS DADOS -----
        if(linha.startsWith("temperatura:")) {
            // Separa "temperatura:" de "27" e pega o valor
            QString valor = linha.split(":")[1].trimmed();
            if(txtTemp) txtTemp->setProperty("text", valor + " °C");
        }
        else if(linha.startsWith("colisao:")) {
            QString valor = linha.split(":")[1].trimmed();
            if(txtColisao) txtColisao->setProperty("text", valor);
        }
        else if(linha.startsWith("caputado:")) {
            QString valor = linha.split(":")[1].trimmed();
            if(txtCaputado) txtCaputado->setProperty("text", valor);
        }
        else if(linha.startsWith("fumo:")) {
            QString valor = linha.split(":")[1].trimmed();
            if(txtFumo) txtFumo->setProperty("text", valor);
        }
    }
}
