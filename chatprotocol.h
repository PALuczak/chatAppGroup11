#ifndef CHATPROTOCOL_H
#define CHATPROTOCOL_H

#include <QObject>
#include <QtNetwork>
#include <QUdpSocket>
#include <QByteArray>
#include <QBuffer>
#include <QSet>
#include <QList>
#include <QTimer>
#include <QDateTime>
#include <QVector>
#include <iostream>

#include <mutex>

#include "simplecrypt.h"
#include "chatpacket.h"


class chatProtocol : public QObject
{
    Q_OBJECT
private:
    const QHostAddress groupAddress = QHostAddress("228.1.2.3");
    const quint16 udpPort = 38594;
    QUdpSocket commSocket;

    QSet<QByteArray> receiveBuffer; //stores ID numbers
    std::mutex receiveMutex;
    QList<chatPacket> sendBuffer; //stores whole packets
    std::mutex sendMutex;

    const unsigned int packetTimeout = 1000; // milliseconds
    const unsigned int notificationTime = 10000; // milliseconds
    QTimer clock;
    QTimer clockAck;

    void encryptPacket(QByteArray & packet);
    void decryptPacket(QByteArray & packet);
    quint64 encryptionKey = 0x40b50fe120bbd01b;

    QString username;
    QList<QString> userList;
    QMap<QString, int> userListTime;
    int curCounter = 0;

    int currentTimeOut = 0;

private slots:
    void readIncomingDatagrams();
public:
    chatProtocol();
    void sendPacket(QByteArray packet);
    void receivePacket(chatPacket packet);
    bool packetAvaialble();
    void setUsername(QString name);
    QList<QString> getConnectedUsers();
    quint64 getEncryptionKey() const;
    void setEncryptionKey(const quint64 &value);

public slots:
    void connectToChat();
    void disconnectFromChat();
    void enqueueMessage(QString);
    void sendAck(QByteArray ackN, QString source);
    void forwardPacket(chatPacket pkt);
    void clockedSender();
    void resendPack();
signals:
    void ourPacketReceived(QByteArray ackN, QString source);
    void theirPacketReceived(chatPacket pkt);

    void statusInfo(QString info, int timeout);
    void usersUpdated(QList<QString> users);
    void updateChat(QString message);
};

#endif // CHATPROTOCOL_H
