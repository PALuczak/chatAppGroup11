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
    std::list<QByteArray> userList;
private slots:
    void readIncomingDatagrams();
public:
    chatProtocol();
    void sendPacket(QByteArray packet);
    QByteArray receivePacket();
    bool packetAvaialble();
    void setUsername(QByteArray name);
    void getConnectedUsers(std::list<QByteArray> & list);
public slots:
    void connectToChat();
    void disconnectFromChat();
};

#endif // CHATPROTOCOL_H
