#include "inutil.h"
#include <qdebug.h>

INUTIL::INUTIL(QObject *parent, QString str) : QObject{parent}
{
    i = 0;
    this->name = str;
}

void INUTIL::arranca()
{
    qDebug() << "inutil " << name << " arranca";

    i++;
    if(i > 2) i= 0;

    emit terminei(i, this->name);
}
