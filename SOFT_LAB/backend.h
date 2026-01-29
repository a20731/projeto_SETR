#ifndef BACKEND_H
#define BACKEND_H

#include <QQmlApplicationEngine>
#include <QObject>
#include <QSerialPort>
#include <QString>

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject *parent = nullptr, QQmlApplicationEngine *engine = nullptr);

public slots:
    void readPort();

private:
    QSerialPort *port;
    QString buffer_leitura; // Para acumular os dados que chegam da porta serie

    // Ponteiros para os textos no QML
    QObject *txtTemp;
    QObject *txtColisao;
    QObject *txtCaputado;
    QObject *txtFumo;
};

#endif // BACKEND_H
