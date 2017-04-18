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
    QByteArray header = packet.left(64);
    QByteArray actualData = packet.mid(64);
    s << actualData;
    actualData.clear();
    // Make a cypher with stream's data
    actualData = crypto.encryptToByteArray(buffer.data());
    buffer.close();
    if (crypto.lastError() != SimpleCrypt::ErrorNoError)
        qDebug("ERROR: encryption failed. code: " + crypto.lastError());
    packet.clear();
    packet = header;
    packet.append(actualData);
}

void chatProtocol::decryptPacket(QByteArray & packet)
{
    // Initialize SimpleCrypt object with hexadecimal key = (0x)40b50fe120bbd01b
    SimpleCrypt crypto2(Q_UINT64_C(0x40b50fe120bbd01b));
    QByteArray header = packet.left(64);
    QByteArray actualData = packet.mid(64);
    QByteArray plaintext = crypto2.decryptToByteArray(actualData);
    if (crypto2.lastError() != SimpleCrypt::ErrorNoError)
        qDebug("ERROR: decryption failed. code: " + crypto2.lastError());
    // Stream the data into a buffer
    QBuffer buffer2(&plaintext);
    buffer2.open(QIODevice::ReadOnly);
    QDataStream s2(&buffer2);
    s2.setVersion(QDataStream::Qt_4_7);
    // Output the decrypted cypher in our QByteArray
    actualData.clear();
    s2 >> actualData;
    buffer2.close();
    packet.clear();
    packet = header;
    packet.append(actualData);
}

void chatProtocol::readIncomingDatagrams()
{
    QNetworkDatagram incoming = this->commSocket.receiveDatagram();
    QByteArray incomingData;
    incomingData = incoming.data();
    decryptPacket(incomingData);

    chatPacket packet;
    packet.fromByteArray(incomingData);
    //check if packet is corrupted
    chatPacket packet2;
    packet2.fromByteArray(incomingData);
    packet2.makeHash();

    //not corrupt
    if (packet2.getPacketId()==packet.getPacketId()) this->receivePacket(packet);
}

chatProtocol::chatProtocol()
{
    connect(this, SIGNAL(ourPacketReceived(QByteArray, QString)), this, SLOT(sendAck(QByteArray, QString)));
    connect(this, SIGNAL(theirPacketReceived(chatPacket )), this, SLOT(forwardPacket(chatPacket )));
    connect(&clock,SIGNAL(timeout()),this,SLOT(clockedSender()));
}

void chatProtocol::sendPacket(QByteArray packet)
{
    encryptPacket(packet);
    this->commSocket.writeDatagram(packet.data(),packet.size(),this->groupAddress,this->udpPort);

    // TODO: ensure relaibility
    // TODO: implement fragmentation is packet is too big
}

void chatProtocol::receivePacket(chatPacket packet)
{
    std::unique_lock<std::mutex> receiveLock(this->receiveMutex);
    if(packet.getSourceName() == this->username) return; // ignore packets that we send

    //packet is already received ones

    //the computer already received the packet before
    if (receiveBuffer.find(packet.getPacketId()) != receiveBuffer.end()) {
        //if the packet is for this computer, send an ack back again if it is not an ack-packet
        if (packet.getDestinationName()==this->username && packet.getAckId()=="00000000000000000000")
            emit ourPacketReceived(packet.getPacketId(), packet.getSourceName());
        return;
    }

    //new packet
    this->receiveBuffer.insert((packet.getPacketId()));

    //if it is an ack packet for this user
    if (packet.getAckId()!="00000000000000000000" && packet.getDestinationName()==this->username) {

        //erase packet from the sending window (sendBuffer)
        for (int i=0; i<sendBuffer.size(); i++) {
            if (packet.getAckId()==sendBuffer[i].getPacketId()) { //use the Id number to check if it is the same packet
                sendBuffer.erase(sendBuffer.begin()+i);
            }
        }
        return;
    }

    //if the packet is for this user or is a broadcast packet
    if (packet.getDestinationName() == this->username || packet.getDestinationName() == "broadcast") {
        bool userKnown = false;
        for(QString user : this->userList) {
            if (user == packet.getSourceName()){
                userKnown = true;
                break;
            }
        }
        if(!userKnown) {
            userList.append(packet.getSourceName());
            emit usersUpdated(this->userList);
        }

        emit updateChat(packet.getPacketData());
        if(packet.getDestinationName() == "broadcast") emit theirPacketReceived(packet);
        emit ourPacketReceived(packet.getPacketId(), packet.getSourceName());
        return;
    }

    //forward packet to the right destination if it was not a broadcast packet or for this computer
    emit theirPacketReceived(packet);



    // TODO: process fragmented packets
}

bool chatProtocol::packetAvaialble()
{
    return !this->receiveBuffer.empty();
}

void chatProtocol::setUsername(QString name)
{
    this->username = name; // TODO: limit this by length
}

QList<QString> chatProtocol::getConnectedUsers()
{
    return userList;
}

void chatProtocol::connectToChat()
{
    this->commSocket.bind(QHostAddress::AnyIPv4, this->udpPort);
    connect(&commSocket,SIGNAL(readyRead()),this,SLOT(readIncomingDatagrams()));
    this->commSocket.joinMulticastGroup(groupAddress);
}

void chatProtocol::disconnectFromChat()
{
    disconnect(&commSocket,SIGNAL(readyRead()),this,SLOT(readIncomingDatagrams()));
    this->commSocket.close();
}

void chatProtocol::enqueueMessage(QString message)
{
    std::unique_lock<std::mutex> sendLock(this->sendMutex);
    chatPacket packet;
    packet.setSourceName(username);
    packet.setPacketData(message.toUtf8());
    packet.makeHash();
    sendBuffer.push_back(packet);
    sendLock.unlock();
    this->sendPacket(packet.toByteArray());
}

void chatProtocol::sendAck(QByteArray ackN, QString source) {
    //create a new packet with the IDnumber of the received packet as Acknumber and the source as destination
    chatPacket ackP;
    ackP.setSourceName(username);
    ackP.setDestinationName(source);
    ackP.setAckId(ackN);
    ackP.makeHash(); //id number
    QByteArray ackPacket = ackP.toByteArray();
    this->sendPacket(ackPacket);
}

void chatProtocol::forwardPacket(chatPacket pkt) {

    //use routing table (implement later)
    //or
    //broadcast packet to all connected computers
    this->sendPacket(pkt.toByteArray());
}

void chatProtocol::clockedSender()
{

}
