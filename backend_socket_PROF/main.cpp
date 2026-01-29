#include "backend.h"
#include "inutil.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QObject>
#include <QString>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("demo_1", "Main");

    Backend run = Backend(0, &engine);
    INUTIL m_i(0, "11");
    INUTIL m_i2(0, "22");

    QObject::connect(&run, SIGNAL(numero10()), &m_i,  SLOT(arranca()));
    QObject::connect(&run, SIGNAL(numero10()), &m_i2,  SLOT(arranca()));

    QObject::connect(&m_i, SIGNAL(terminei(int, QString)), &run,  SLOT(muda_cor(int, QString)));

    return app.exec();
}
