#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QSerialPort>
#include <QTcpSocket>

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject *parent = nullptr, QQmlApplicationEngine *engine = nullptr);

public slots:
    void readSerial();  // Lê do Arduino e Envia para Python
    void readTcpData(); // Lê feedback do Python (se houver)

private:
    // --- COMUNICAÇÃO ---
    QSerialPort *m_serial;
    QTcpSocket *_pSocket;
    QString buffer_serial;

    // --- INTERFACE (QML) ---
    QObject *txtTemp;
    QObject *txtColisao;
    QObject *txtCaputado;
    QObject *txtFumo;
    QObject *txtStatusPython;
};

#endif // BACKEND_H
