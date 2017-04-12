#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chatprotocol.h"
#include <QDateTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
public slots:
    void updateUserList(QList<QString> users);
    void parseNewMessage();
    void enableConnect();
    void disableConnect();
    void enableDisconnect();
    void disableDisconnect();
    void setName();
signals:
    void newMessageWritten(QString message);
private:
    Ui::MainWindow *ui;
    chatProtocol chat;
};

#endif // MAINWINDOW_H
