#ifndef INUTIL_H
#define INUTIL_H

#include <QObject>

class INUTIL : public QObject
{
    Q_OBJECT
public:
    explicit INUTIL(QObject *parent = nullptr, QString str = nullptr);

public slots:
    void arranca();

signals:
    void terminei(int i, QString str);

private:
    int i;
    QString name;
};

#endif // INUTIL_H
