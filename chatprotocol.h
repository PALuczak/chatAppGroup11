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
#include "chatpacket.h"

class chatProtocol : public QObject
{
    Q_OBJECT
private:
    const QHostAddress groupAddress = QHostAddress("228.0.0.0");
    const quint16 udpPort = 4968;
    QUdpSocket commSocket;
    std::deque<QByteArray> receiveBuffer; //stores ID numbers
    std::mutex receiveMutex;
    QList<chatPacket> sendBuffer; //stores whole packets
    std::mutex sendMutex;
    void encryptPacket(QByteArray & packet);
    void decryptPacket(QByteArray & packet);


    QString username;
    QList<QString> userList;
private slots:
    void readIncomingDatagrams();
public:
    chatProtocol();
    void sendPacket(QByteArray packet);
    QByteArray receivePacket(chatPacket packet);
    bool packetAvaialble();
    void setUsername(QString name);
    void getConnectedUsers(QList<QString> & list);
public slots:
    void connectToChat();
    void disconnectFromChat();
    void enqueueMessage(QString);
<<<<<<< HEAD
=======
    // fakeSignals for tests purposes
>>>>>>> 521de4827c68f1afe2a1d9a6bf166998b6575f0a
    void sendAck(QByteArray ackN, QString source);
    void forwardPacket(chatPacket pkt);
    void resendPacket(chatPacket id);
    void sendNextPacket(chatPacket id);
signals:
    void ackReceived(QByteArray id);
    void ourPacketReceived(QByteArray ackN, QString source);
    void theirPacketReceived(chatPacket pkt);
    void ackTimeout(QByteArray id);

    void statusInfo(QString info, int timeout);
    void usersUpdated(QList<QString> users);
    void updateChat(QString message);
};

#endif // CHATPROTOCOL_H
