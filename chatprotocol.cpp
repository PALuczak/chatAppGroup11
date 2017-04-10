#include "chatprotocol.h"

void chatProtocol::encryptPacket(QByteArray &packet)
{
    (void) packet; // TODO: implement this
}

void chatProtocol::decryptPacket(QByteArray &packet)
{
    (void) packet; // TODO: implement this
}

void chatProtocol::readIncomingDatagrams()
{
    std::unique_lock<std::mutex> receiveLock(this->receiveMutex);
    QNetworkDatagram incoming = this->commSocket.receiveDatagram();
    QByteArray * incomingData = new(QByteArray);
    *incomingData = incoming.data();
    this->receiveBuffer.push_back(incomingData);
    // TODO: check the recipient in the packet and ignore those not addressed to us
    // TODO: process ACKs
    decryptPacket(*incomingData);
}

chatProtocol::chatProtocol()
{

}

void chatProtocol::sendPacket(QByteArray packet) // TODO: ensure relaibility
{
    encryptPacket(packet);
    this->commSocket.writeDatagram(packet,this->groupAddress,this->udpPort);
}

QByteArray chatProtocol::receivePacket()
{
    QByteArray packet;

    return packet;
}

bool chatProtocol::packetAvaialble()
{
    return !this->receiveBuffer.empty();
}

void chatProtocol::setUsername(QByteArray name)
{
    this->username = name; // TODO: limit this by length
}

void chatProtocol::getConnectedUsers(std::list<QByteArray> & list)
{
    for(QByteArray user : this->userList) {
        list.push_back(user);
    }
}

void chatProtocol::connectToChat()
{
    this->commSocket.bind(QHostAddress::LocalHost, 8594);
    connect(&commSocket,SIGNAL(readyRead()),this,SLOT(readIncomingDatagrams()));
}

void chatProtocol::disconnectFromChat()
{
    disconnect(&commSocket,SIGNAL(readyRead()),this,SLOT(readIncomingDatagrams()));
    this->commSocket.close();
}
