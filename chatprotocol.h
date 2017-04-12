#ifndef CHATPROTOCOL_H
#define CHATPROTOCOL_H

#include <QObject>
#include <QtNetwork>
#include <QUdpSocket>
#include <QByteArray>
#include <QBuffer>
#include <deque>
#include <mutex>
#include "simplecrypt.h"
#include <iostream>
#include <QList>

class chatProtocol : public QObject
{
    Q_OBJECT
private:
    const QHostAddress groupAddress = QHostAddress("228.0.0.0");
    const quint16 udpPort = 4968;
    QUdpSocket commSocket;
    std::deque<QByteArray *> receiveBuffer;
    std::mutex receiveMutex;
    std::deque<QByteArray *> sendBuffer;
    std::mutex sendMutex;
    void encryptPacket(QByteArray & packet);
    void decryptPacket(QByteArray & packet);


    QByteArray username;
    QList<QString> userList;
private slots:
    void readIncomingDatagrams();
public:
    chatProtocol();
    void sendPacket(QByteArray packet);
    QByteArray receivePacket();
    bool packetAvaialble();
    void setUsername(QByteArray name);
    void getConnectedUsers(QList<QString> & list);
public slots:
    void connectToChat();
    void disconnectFromChat();
    void enqueueMessage(QString);
    // fakeSignals for tests purposes
    void fakeSignals(int i, QByteArray id);
    void sendAck(QByteArray id);
    void forwardPacket(QByteArray id);
    void resendPacket(QByteArray id);
    void sendNextPacket(QByteArray id);
signals:
    void ackReceived(QByteArray id);
    void ourPacketReceived(QByteArray id);
    void theirPacketReceived(QByteArray id);
    void ackTimeout(QByteArray id);

    void statusInfo(QString info, int timeout);
    void usersUpdated(QList<QString> users);
    void updateChat(QString message);
};

#endif // CHATPROTOCOL_H
