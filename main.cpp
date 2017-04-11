#include "mainwindow.h"
#include "chatprotocol.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    chatProtocol test;
    QByteArray t("Hello World!");
    for (int i = 0; i < 4; i++){
        test.fakeSignals(i, t);
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
