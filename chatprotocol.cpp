#include "chatprotocol.h"

void chatProtocol::encryptPacket(QByteArray &packet)
{
    // Initialize SimpleCrypt object with hexadecimal key = (0x)40b50fe120bbd01b
    SimpleCrypt crypto(Q_UINT64_C(0x40b50fe120bbd01b));
    // Set compression and integrity options
    crypto.setCompressionMode(SimpleCrypt::CompressionAlways); //always compress the data, see section below
    crypto.setIntegrityProtectionMode(SimpleCrypt::ProtectionHash); //properly protect the integrity of the data
    // Stream the data into a buffer
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream s(&buffer);
    s.setVersion(QDataStream::Qt_4_7);
    // Fill the stream with our input QByteArray
    s << packet;
    packet.clear();
    // Make a cypher with stream's data
    packet = crypto.encryptToByteArray(buffer.data());
        if (crypto.lastError() == SimpleCrypt::ErrorNoError) {
        // do something relevant with the cyphertext, such as storing it or sending it over a socket to another machine.
        }
    buffer.close();
}

void chatProtocol::decryptPacket(QByteArray &packet)
{
    // Initialize SimpleCrypt object with hexadecimal key = (0x)40b50fe120bbd01b
    SimpleCrypt crypto2(Q_UINT64_C(0x40b50fe120bbd01b));
    QByteArray plaintext = crypto2.decryptToByteArray(packet);
        if (!crypto2.lastError() == SimpleCrypt::ErrorNoError) {
        // check why we have an error, use the error code from crypto.lastError() for that
        }
    // Stream the data into a buffer
    QBuffer buffer2(&plaintext);
    buffer2.open(QIODevice::ReadOnly);
    QDataStream s2(&buffer2);
    s2.setVersion(QDataStream::Qt_4_7);
    // Output the decrypted cypher in our QByteArray
    s2 >> packet;
    buffer2.close();
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
