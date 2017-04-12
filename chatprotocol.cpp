#include "chatprotocol.h"

void chatProtocol::encryptPacket(QByteArray & packet)
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

void chatProtocol::decryptPacket(QByteArray & packet)
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

    // send ack back to source address and broadcast the packet to all the connected computers
    if (*incomingData->destinationName == "broadcast") {
        for (int i=0; i<receiveBuffer.size(); i++) {
            //the computer already received the packet before
            if (receiveBuffer[i]==*incomingData) {
                return;
            }
            //the computer didn't receive the packet before
            else {
                this->sendAck(*incomingData);
                this->sendPacket(*incomingData);
            }
        }
    }

    //send ack back if packet was for this computer
    else if (*incomingData->destinationName == "username") {
        this->sendAck(*incomingData);
    }

    //forward packet to the right destination if it was not a broadcast packet or for this computer
    else if (*incomingData->destinationName != "username" && *incomingData->destinationName != "broadcast") {
        this->forwardPacket(&incomingData);
    }

    this->receiveBuffer.push_back(incomingData);

    // TODO: check the recipient in the packet and ignore those not addressed to us
    // TODO: process ACKs
    decryptPacket(*incomingData);
}

chatProtocol::chatProtocol()
{
    connect(this, SIGNAL(ourPacketReceived(QByteArray)), this, SLOT(sendAck(QByteArray)));
    connect(this, SIGNAL(theirPacketReceived(QByteArray)), this, SLOT(forwardPacket(QByteArray)));
    connect(this, SIGNAL(ackTimeout(QByteArray)), this, SLOT(resendPacket(QByteArray)));
    connect(this, SIGNAL(ackReceived(QByteArray)), this, SLOT(sendNextPacket(QByteArray)));
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

void chatProtocol::getConnectedUsers(QList<QString> & list)
{
    for(QString user : this->userList) {
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

void chatProtocol::enqueueMessage(QString)
{

}

void chatProtocol::fakeSignals(int i, QByteArray id){
    if(i == 0){
        emit ackReceived(id);
    }
    if(i == 1){
        emit ourPacketReceived(id);
    }
    if(i == 2){
        emit theirPacketReceived(id);
    }
    if(i == 3){
        emit ackTimeout(id);
    }
}

void chatProtocol::sendAck(QByteArray id) {
    std::cout << "sendAck (from ourPacketReceived signal) with id: "<< id.toHex().constData() << std::endl;
}

void chatProtocol::forwardPacket(QByteArray id) {
    std::cout << "forwardPacket (from theirPacketReceived signal) with id: "<< id.toHex().constData() << std::endl;

}

void chatProtocol::resendPacket(QByteArray id) {
    std::cout << "resendPacket (from ackTimeout signal) with id: "<< id.toHex().constData() << std::endl;
}

void chatProtocol::sendNextPacket(QByteArray id) {
    std::cout << "sendNextPacket (from ackRecevied signal) with id: "<< id.toHex().constData() << std::endl;
}
