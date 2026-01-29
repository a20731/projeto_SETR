#include "backend.h"
#include <QTimer>
#include <QTcpSocket>
#include <QProcess>
#include <QDebug>

Backend::Backend(QObject *parent, QQmlApplicationEngine *ptr) : QObject{parent}
{
    qDebug() << "starting !";

    // --- BLOCO DO PROFESSOR (COMENTADO PARA NÃO DAR ERRO) ---
    // Como tu já corres o Python manualmente, não precisamos disto.
    // QString program = "D:\\Software\\miniconda3_2025\\envs\\MIAA39\\python.exe";
    // QStringList arguments;
    // arguments << " D:\\server_meec_lab.py";
    // QProcess *myProcess = new QProcess(parent);
    // myProcess->startDetached(program, arguments);
    // --------------------------------------------------------

    // UI - Encontrar os objetos gráficos
    this->m_q2 = ptr->rootObjects().at(0)->findChild<QObject*>("quadrado");
    if(m_q2) this->m_q2->setProperty("color", "blue");

    this->m_txt3 = ptr->rootObjects().at(0)->findChild<QObject*>("txt");
    if(m_txt3) {
        this->m_txt3->setProperty("text", "START");
        // this->m_txt3->setProperty("x", 0); // Comentei para não estragar a posição visual
        // this->m_txt3->setProperty("y", 0);
    }

    this->m_luz = ptr->rootObjects().at(0)->findChild<QObject*>("luz");

    a = 0;
    QTimer::singleShot(1000, this, &Backend::update);

    // CONFIGURAR PORTA SÉRIE (Mudei para a tua COM7)
    m_serial = new QSerialPort();
    connect(m_serial, SIGNAL(readyRead()), this, SLOT(ler_rs232()));

    m_serial->setPortName("COM7"); // <--- A TUA PORTA
    m_serial->setBaudRate(115200); // <--- A TUA VELOCIDADE (normalmente é 115200 no teu projeto anterior)
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::TwoStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Serial COM7 Connected!";
    } else {
        qDebug() << "Serial Connection FAILED! (Verifica se está fechada noutros programas)";
    }

    // CONNECT TO SERVER (Isto liga ao teu Python)
    _pSocket = new QTcpSocket(this);
    connect(_pSocket, SIGNAL(readyRead()), this, SLOT(readTcpData()));

    // Liga-se ao localhost (teu PC) na porta 12345 (definida no teu python)
    _pSocket->connectToHost("localhost", 12345);

    if( _pSocket->waitForConnected(3000) ) { // Espera 3 segs para ligar
        qDebug() << "LIGADO AO SERVIDOR PYTHON!";
        _pSocket->write( "Ola Python, sou o Qt!!" );
    } else {
        qDebug() << "ERRO: Nao consegui ligar ao Python.";
    }
}

void Backend::update()
{
    //qDebug() << "update !";
    QTimer::singleShot(1000, this, &Backend::update);

    a++;
    if(m_txt3) this->m_txt3->setProperty("text", a);

    // Envia o valor 'a' para o Python
    if(_pSocket->isOpen()){
        _pSocket->write( QByteArray::number(a) );
    }

    if(a%10 == 1)
    {
        qDebug() << "emit n10";
        emit numero10();
    }
}

void Backend::muda_cor(int i, QString str)
{
    qDebug() << "quem mandou foi: " << str;
    if(!m_luz) return;

    if(i==0) this->m_luz->setProperty("color","red");
    if(i==1) this->m_luz->setProperty("color","yellow");
    if(i==2) this->m_luz->setProperty("color","green");
}

void Backend::ler_rs232()
{
    if( m_serial->canReadLine() )
    {
        QByteArray raw = m_serial->readLine();
        qDebug() << "Arduino diz: " << QString(raw).trimmed();
    }
}

void Backend::readTcpData()
{
    // LÊ O QUE VEM DO PYTHON
    QByteArray raw = _pSocket->readAll();
    qDebug() << "PYTHON DISSE: " << QString(raw);
}
