#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->nameButton,SIGNAL(clicked()),this,SLOT(setName()));
    connect(ui->connectButton,SIGNAL(clicked()),&chat,SLOT(connectToChat()));
    connect(ui->connectButton,SIGNAL(clicked()),this,SLOT(disableConnect()));
    connect(ui->connectButton,SIGNAL(clicked()),this,SLOT(enableDisconnect()));
    connect(ui->disconnectButton,SIGNAL(clicked()),&chat,SLOT(disconnectFromChat()));
    connect(ui->disconnectButton,SIGNAL(clicked()),this,SLOT(disableDisconnect()));
    connect(ui->disconnectButton,SIGNAL(clicked()),this,SLOT(enableConnect()));
    connect(ui->inputLine,SIGNAL(returnPressed()),this,SLOT(parseNewMessage()));
    connect(this,SIGNAL(newMessageWritten(QString)),&chat,SLOT(enqueueMessage(QString)));
    connect(&chat,SIGNAL(updateChat(QString)),ui->chatLog,SLOT(append(QString)));
    connect(&chat,SIGNAL(statusInfo(QString,int)),ui->statusBar,SLOT(showMessage(QString,int)));
    connect(&chat,SIGNAL(usersUpdated(QList<QString>)),this,SLOT(updateUserList(QList<QString>)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateUserList(QList<QString> users)
{
    ui->userList->clear();
    int row = 0;
    for(QString user : users) {
        ui->userList->insertItem(row++, user);
    }
}

void MainWindow::parseNewMessage()
{
    if(!ui->inputLine->text().length()) return;
    QString message;
    QDateTime timestamp = QDateTime::currentDateTime();
    message.append(ui->nameLine->text());
    message.append("@");
    message.append(timestamp.toString(Qt::ISODate));
    message.append(" : ");
    message.append(ui->inputLine->text());
    ui->inputLine->clear();
    ui->chatLog->append(message);
    emit newMessageWritten(message);
}

void MainWindow::enableConnect()
{
    ui->connectButton->setEnabled(true);
}

void MainWindow::disableConnect()
{
    ui->connectButton->setDisabled(true);
}

void MainWindow::enableDisconnect()
{
    ui->disconnectButton->setEnabled(true);
    ui->inputLine->setEnabled(true);
}

void MainWindow::disableDisconnect()
{
    ui->disconnectButton->setDisabled(true);
    ui->inputLine->setDisabled(true);
}

void MainWindow::setName()
{
    if(!ui->nameLine->text().length()) return;
    if(ui->keyLine->text().length() != 19) return;
    chat.setEncryptionKey(ui->keyLine->text().simplified().replace(" ","").toUpper().toULongLong(nullptr,16));
    chat.setUsername(ui->nameLine->text().toUtf8());
    ui->nameLine->setReadOnly(true);
    ui->keyLine->setReadOnly(true);
    ui->connectButton->setEnabled(true);
    ui->nameButton->setDisabled(true);
}
