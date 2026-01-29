#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QObject>
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

    engine.loadFromModule("demo2_nov25", "Main");

    Backend *bk = new Backend(0, &engine);

    return app.exec();
}
