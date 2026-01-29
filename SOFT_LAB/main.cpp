#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "backend.h"


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
    engine.loadFromModule("teste_imagem", "Main");

    Backend *bk = new Backend(0,&engine);
    return app.exec();
}
