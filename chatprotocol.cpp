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
    std::unique_lock<std::mutex> receiveLock(this->receiveMutex);
    QNetworkDatagram incoming = this->commSocket.receiveDatagram();
    QByteArray * incomingData = new(QByteArray);
    *incomingData = incoming.data();
    decryptPacket(*incomingData);

    chatPacket packet;
    packet.fromByteArray(*incomingData);
    //check if packet is corrupted
    chatPacket packet2;
    packet2.fromByteArray(*incomingData);
    packet2.makeHash();

    //not corrupt
    if (packet2.getPacketId()==packet.getPacketId()) {

        //packet is already received ones

        for (int i=0; i<(int)receiveBuffer.size(); i++) {
            //the computer already received the packet before
            if (receiveBuffer[i]==packet.getPacketId()) {
                //if the packet is for this computer, send an ack back again if it is not an ack-packet
                if (packet.getDestinationName()==username && packet.getAckId()=="00000000000000000000") {
                    emit ourPacketReceived(packet.getPacketId(), packet.getSourceName());
                }
                return;
            }
        }

        //new packet

        //if it is an ack packet for this user
        if (packet.getAckId()!="00000000000000000000" && packet.getDestinationName()==username) {

            //erase packet from the sending window (sendBuffer)
            for (int i=0; i<sendBuffer.size(); i++) {
                if (packet.getAckId()==sendBuffer[i].getPacketId()) { //use the Id number to check if it is the same packet
                    sendBuffer.erase(sendBuffer.begin()+i);
                }
            }
        }

        //if the packet is for this user
        else if (packet.getDestinationName() == username) {
            emit updateChat(packet.getPacketData());
            emit ourPacketReceived(packet.getPacketId(), packet.getSourceName());
        }

        //if it is a broadcast packet
        else if (packet.getDestinationName() == "broadcast") {
            emit updateChat(packet.getPacketData());
            emit ourPacketReceived(packet.getPacketId(), packet.getSourceName());
        }

        //forward packet to the right destination if it was not a broadcast packet or for this computer
        else {
            emit theirPacketReceived(packet);
        }

         this->receiveBuffer.push_back(packet.getPacketId());

        // TODO: process fragmented packets

    }
}

chatProtocol::chatProtocol()
{
    connect(this, SIGNAL(ourPacketReceived(QByteArray, QString)), this, SLOT(sendAck(QByteArray, QString)));
    connect(this, SIGNAL(theirPacketReceived(chatPacket )), this, SLOT(forwardPacket(chatPacket )));
    connect(this, SIGNAL(ackTimeout(chatPacket)), this, SLOT(resendPacket(chatPacket)));
    connect(this, SIGNAL(ackReceived(chatPacket)), this, SLOT(sendNextPacket(chatPacket)));
}

void chatProtocol::sendPacket(QByteArray packet)
{
    chatPacket tempPkt;
    tempPkt.fromByteArray(packet);
    if(tempPkt.getPacketId() == "00000000000000000000"){
        sendBuffer.push_back(tempPkt);
    }

    encryptPacket(packet);
    this->commSocket.writeDatagram(packet,this->groupAddress,this->udpPort);
    // TODO: ensure relaibility
    // TODO: implement fragmentation is packet is too big
}

QByteArray chatProtocol::receivePacket(chatPacket packet)
{

    //show the data from the packet in the console
QByteArray ar;

    return ar;
}

bool chatProtocol::packetAvaialble()
{
    return !this->receiveBuffer.empty();
}

void chatProtocol::setUsername(QString name)
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
    this->commSocket.joinMulticastGroup(groupAddress); //no clue what this does, but is in example
}

void chatProtocol::disconnectFromChat()
{
    disconnect(&commSocket,SIGNAL(readyRead()),this,SLOT(readIncomingDatagrams()));
    this->commSocket.close();
}

void chatProtocol::enqueueMessage(QString message)
{
    chatPacket packet;
    packet.setSourceName(username);
    packet.setPacketData(message.toUtf8());
    packet.makeHash();
    this->sendPacket(packet.toByteArray());
}

void chatProtocol::sendAck(QByteArray ackN, QString source) {
    //create a new packet with the IDnumber of the received packet as Acknumber and the source as destination
    chatPacket ackP;
    ackP.setSourceName(username);
    ackP.setDestinationName(source);
    ackP.setAckId(ackN);
    ackP.makeHash(); //id number
    QByteArray* ackPacket = new(QByteArray);
    ackP.fromByteArray(*ackPacket);
    this->sendPacket(*ackPacket);

  //  std::cout << "sendAck (from ourPacketReceived signal) with id: "<< id.toHex().constData() << std::endl;

}

void chatProtocol::forwardPacket(chatPacket pkt) {

    //use routing table (implement later)
    //or
    //broadcast packet to all connected computers
    this->sendPacket(pkt.toByteArray());

  //  std::cout << "forwardPacket (from theirPacketReceived signal) with id: "<< id.toHex().constData() << std::endl;

}

void chatProtocol::resendPacket(chatPacket pkt) {
  //  std::cout << "resendPacket (from ackTimeout signal) with id: "<< id.toHex().constData() << std::endl;
}

void chatProtocol::sendNextPacket(chatPacket pkt) {
 //   std::cout << "sendNextPacket (from ackRecevied signal) with id: "<< id.toHex().constData() << std::endl;
}
