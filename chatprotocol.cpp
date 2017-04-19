#include "chatprotocol.h"

void chatProtocol::encryptPacket(QByteArray & packet)
{
    // Initialize SimpleCrypt object with hexadecimal key = (0x)40b50fe120bbd01b
    SimpleCrypt crypto(this->encryptionKey);
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
    SimpleCrypt crypto2(this->encryptionKey);
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

quint64 chatProtocol::getEncryptionKey() const
{
    return this->encryptionKey;
}

void chatProtocol::setEncryptionKey(const quint64 &value)
{
    this->encryptionKey = value;
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
    connect(&clockAck,SIGNAL(timeout()),this,SLOT(resendPack()));
    userList.append("broadcast");
}

void chatProtocol::sendPacket(QByteArray packet)
{
    encryptPacket(packet);
    this->commSocket.writeDatagram(packet.data(),packet.size(),this->groupAddress,this->udpPort);

    std::cout<<"sending packet\n";
    // TODO: ensure relaibility --> i think we did this now
    // TODO: implement fragmentation is packet is too big --> we don do this anymore
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
                //check if the packets still needs an ack of this user
                if (sendBuffer[i].getAckUsers().contains(packet.getSourceName())) {
                    sendBuffer[i].removeUser(packet.getSourceName()); // remove the user if the ack of this user is received
                }
                // remove the packet from the buffer if all the acks of the users are received
                if (sendBuffer[i].getAckUsers().size()==0) {
                     sendBuffer.erase(sendBuffer.begin()+i);
                }
            }
        }

        return;
    }

    //if the packet is for this user or is a broadcast packet
    if (packet.getDestinationName() == this->username || packet.getDestinationName() == "broadcast") {

        if(!userList.contains(packet.getSourceName())) {
            userList.append(packet.getSourceName());
            userListTime.insert(packet.getSourceName(), curCounter); // link user to current value timer, if timer is not update regularly, there is no connections, so this user is removed
            emit usersUpdated(this->userList);
            emit updateChat(packet.getSourceName() + " has connected");
        }
        else {
            userListTime.insert(packet.getSourceName(), curCounter); // update the time with the current time to keep track that this user is still online
        }

        if(packet.getPacketData().left(12) == "DISCONNECTED") {
            QString message;
            message.append(packet.getSourceName());
            message.append(" has disconnected");
            emit updateChat(message);
            userList.removeAll(packet.getSourceName());
            emit usersUpdated(this->userList);
        }
        else if (packet.getPacketData().left(9)!= "CONNECTED" || packet.getPacketData().left(12)!= "NOTIFICATION") {
            if(packet.getDestinationName() == this->username) emit updateChatDirectMessage(packet.getPacketData());
            else emit updateChat(packet.getPacketData());
        }
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
    emit usersUpdated(this->userList);

    QString message("CONNECTED");
    message.append(QDateTime::currentDateTime().toString(Qt::ISODate));

    this->enqueueMessage(message);
    clock.start(notificationTime); // call clockedSender every 10 second
    clockAck.start(packetTimeout); // check every sec if the timeout of packets are overdue
}

void chatProtocol::disconnectFromChat()
{

    QString message("DISCONNECTED");
    message.append(QDateTime::currentDateTime().toString(Qt::ISODate));

    this->enqueueMessage(message);
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

    packet.setAckUsers(userList); // give a list will all the current users to the packet, so the packet knows how many ack it should receive
    packet.setTimeOut(currentTimeOut); // set a current time to the packet, can be used to check for timeout
    sendBuffer.push_back(packet);
    sendLock.unlock();
    this->sendPacket(packet.toByteArray());
}

void chatProtocol::enqueueDirectMessage(QString message, QString user)
{
    std::unique_lock<std::mutex> sendLock(this->sendMutex);
    chatPacket packet;
    packet.setSourceName(username);
    packet.setDestinationName(user);
    packet.setPacketData(message.toUtf8());
    packet.makeHash();
    QList<QString> users;\
    users.append(user);
    packet.setAckUsers(users);
    packet.setTimeOut(currentTimeOut);
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
    this->sendPacket(ackP.toByteArray());
}

void chatProtocol::forwardPacket(chatPacket pkt) {

    //use routing table (implement later)
    //or
    //broadcast packet to all connected computers
    this->sendPacket(pkt.toByteArray());
}

void chatProtocol::clockedSender()
{
    // send message computer is still there
    chatPacket notification;
    QString notif = "NOTIFICATION ";
    notif.append(QDateTime::currentDateTime().toString(Qt::ISODate));
    notification.setSourceName(username);
    notification.setPacketData(notif.toUtf8());
    notification.makeHash();
    this->sendPacket(notification.toByteArray());

    std::cout<<"send notification message\n";

    // check if users in userList are still connected to the network.
    curCounter++; // every 5 second increase
    for (auto e : userListTime.keys()) {
        if (4 <= curCounter - userListTime[e]) { // after 2 minutes of no messages, delete user from userList
            for (int i = 0; i<userList.size(); i++) {
                if (e==userList[i]) {
                    if(userList.at(i) == "broadcast") continue;
                    emit updateChat(userList[i] + " has disconnected");
                    userList.erase(userList.begin()+i);
                    emit usersUpdated(this->userList); // update list
                }
            }
        }
    }
}

// send a packet if it didn receive an ack in time, same packet as previous is broadcast
void chatProtocol::resendPack() {
    for (int i =0; i<sendBuffer.size();i++) { // loop through all the packets in the buffer
        if (sendBuffer[i].getTimeOut()-currentTimeOut!=0) { // check if the difference between the currentTimeOut and the timeout of the packets is bigger then 1
            if (sendBuffer[i].getAckUsers().size()!=0) { // check if it still needs to get acks from users
                sendBuffer[i].setTimeOut(currentTimeOut); // set the currentTimeOut as the new timeout for this packet
                this->sendPacket(sendBuffer[i].toByteArray()); // broadcast the packet
            }
        }
    }
    currentTimeOut++; // increment the currentTimeOut with 1

}
