#include "backend.h"
#include <QDebug>

Backend::Backend(QObject *parent, QQmlApplicationEngine *engine) : QObject{parent}
{
    // 1. ENCONTRAR OS OBJETOS NO QML
    txtTemp     = engine->rootObjects().at(0)->findChild<QObject*>("valTemp");
    txtColisao  = engine->rootObjects().at(0)->findChild<QObject*>("valColisao");
    txtCaputado = engine->rootObjects().at(0)->findChild<QObject*>("valCaputado");
    txtFumo     = engine->rootObjects().at(0)->findChild<QObject*>("valFumo");

    // NOVO: Ponteiro para o texto dos objetos esquecidos
    txtObjetos  = engine->rootObjects().at(0)->findChild<QObject*>("valObjetos");

    txtStatusPython = engine->rootObjects().at(0)->findChild<QObject*>("statusPy");

    // 2. CONFIGURAR ARDUINO (SERIAL)
    m_serial = new QSerialPort(this);
    m_serial->setPortName("COM7");
    m_serial->setBaudRate(115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::TwoStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "ARDUINO: COM7 Aberta!";
    } else {
        qDebug() << "ARDUINO: Erro ao abrir COM7.";
    }
    connect(m_serial, SIGNAL(readyRead()), this, SLOT(readSerial()));

    // 3. CONFIGURAR PYTHON (SOCKET)
    _pSocket = new QTcpSocket(this);
    connect(_pSocket, SIGNAL(readyRead()), this, SLOT(readTcpData()));

    _pSocket->connectToHost("localhost", 12345);

    if(_pSocket->waitForConnected(3000)) {
        qDebug() << "PYTHON: Conectado!";
        if(txtStatusPython) {
            txtStatusPython->setProperty("text", "PYTHON: ONLINE");
            txtStatusPython->setProperty("color", "#00ff00");
        }
    } else {
        qDebug() << "PYTHON: Falha na conexao.";
        if(txtStatusPython) {
            txtStatusPython->setProperty("text", "PYTHON: OFFLINE");
            txtStatusPython->setProperty("color", "red");
        }
    }
}

void Backend::readSerial()
{
    buffer_serial += m_serial->readAll();

    while (buffer_serial.contains('\n')) {
        int fim = buffer_serial.indexOf('\n');
        QString linha = buffer_serial.left(fim).trimmed();
        buffer_serial.remove(0, fim + 1);

        // Enviar para Python
        if(_pSocket->isOpen()){
            _pSocket->write(linha.toUtf8() + "\n");
        }

        // Atualizar Dashboard
        if(linha.startsWith("temperatura:")) {
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

void Backend::readTcpData()
{
    // LÊ O QUE VEM DO PYTHON
    // O Python manda algo como "objetos:2"
    while(_pSocket->canReadLine()){
        QByteArray dados = _pSocket->readLine();
        QString mensagem = QString(dados).trimmed();

        if(mensagem.startsWith("objetos:")) {
            QString valor = mensagem.split(":")[1].trimmed();
            if(txtObjetos) {
                txtObjetos->setProperty("text", valor);
            }
        }
    }
}
