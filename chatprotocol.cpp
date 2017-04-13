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

        for (int i=0; i<receiveBuffer.size(); i++) {
            //the computer already received the packet before
            if (receiveBuffer[i]==packet.getPacketId()) {
                //if the packet is for this computer, send an ack back again if it is not an ack-packet
                if (packet.getDestinationName()==username && packet.getAckId()=="00000000000000000000") {
                    this->sendAck(packet.getPacketId(), packet.getSourceName());
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
            this->receivePacket(packet);
            this->sendAck(packet.getPacketId(), packet.getSourceName());
        }

        //if it is a broadcast packet
        else if (packet.getDestinationName() == "broadcast") {
            emit updateChat(packet.getPacketData());
            this->receivePacket(packet);
            this->sendAck(packet.getPacketId(), packet.getSourceName());
        }

        //forward packet to the right destination if it was not a broadcast packet or for this computer
        else {
            this->forwardPacket(packet);
        }

         this->receiveBuffer.push_back(packet.getPacketId());
        // TODO: check the recipient in the packet and ignore those not addressed to us
        // TODO: process ACKs

    }
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
    this->commSocket.joinMulticastGroup(groupAddress); //no clue what this does, but is in example
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

void chatProtocol::forwardPacket(chatPacket id) {

    //use routing table (implement later)
    //or
    //broadcast packet to all connected computers


 //   std::cout << "forwardPacket (from theirPacketReceived signal) with id: "<< id.toHex().constData() << std::endl;

}

void chatProtocol::resendPacket(chatPacket id) {
  //  std::cout << "resendPacket (from ackTimeout signal) with id: "<< id.toHex().constData() << std::endl;
}

void chatProtocol::sendNextPacket(chatPacket id) {
 //   std::cout << "sendNextPacket (from ackRecevied signal) with id: "<< id.toHex().constData() << std::endl;
}
