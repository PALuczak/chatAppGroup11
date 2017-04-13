#include "chatpacket.h"

QByteArray chatPacket::toByteArray()
{
    QByteArray bytePack;

    bytePack.append(this->sourceName.toUtf8().leftJustified(this->maxNameLength,'\40')); // \40 is a space
    bytePack.append(this->destinationName.toUtf8().leftJustified(this->maxNameLength,'\40'));
    bytePack.append(this->dataLength);
    bytePack.append(this->packetData);
    if(this->fragment)
        bytePack.append('F');
    else
        bytePack.append('0');
    bytePack.append(this->fragmentNumber);
    bytePack.append(this->totalFragments);
    bytePack.append(this->packetType);
    bytePack.append(this->packetId);
    bytePack.append(this->ackId);
    bytePack.truncate(1 + 4 + this->maxNameLength * 2 + this->maxHashLength * 2 + this->dataLength);
    return bytePack;
}

void chatPacket::fromByteArray(QByteArray bytes)
{
    this->sourceName = bytes.mid(0, this->maxNameLength).trimmed();
    this->destinationName = bytes.mid(32, this->maxNameLength).trimmed();
    this->dataLength = bytes.at(64);
    this->packetData = bytes.mid(65,this->dataLength);
    if(bytes.at(65+this->dataLength) == 'F') this->fragment = true;
    else this->fragment = false;
    this->fragmentNumber = bytes.at(66 + this->dataLength);
    this->totalFragments = bytes.at(67 + this->dataLength);
    this->packetType = (chatPacket::dataType) bytes.at(68 + this->dataLength);
    this->packetId = bytes.mid(69 + this->dataLength, this->maxHashLength);
    this->ackId = bytes.mid(69 + this->dataLength + this->maxHashLength, this->maxHashLength);
}

void chatPacket::makeHash()
{
    QByteArray bytePack;
    bytePack.append(this->sourceName.toUtf8().leftJustified(this->maxNameLength,'\40')); // \40 is a space
    bytePack.append(this->destinationName.toUtf8().leftJustified(this->maxNameLength,'\40'));
    bytePack.append(this->dataLength);
    bytePack.append(this->packetData);
    if(this->fragment)
        bytePack.append('F');
    else
        bytePack.append('0');
    bytePack.append(this->fragmentNumber);
    bytePack.append(this->totalFragments);
    bytePack.append(this->packetType);
    bytePack.truncate(4 + this->maxNameLength * 2 + this->dataLength);
    this->packetId = QCryptographicHash::hash(bytePack, QCryptographicHash::Sha1);
}

void chatPacket::setSourceName(QString name)
{
    name.truncate(this->maxNameLength);
    this->sourceName = name;
}

void chatPacket::setDestinationName(QString name)
{
    name.truncate(this->maxNameLength);
    this->destinationName = name;
}

void chatPacket::setFragment()
{
    this->fragment = true;
}

void chatPacket::unsetFragment()
{
    this->fragment = false;
}

void chatPacket::setFragmentNumber(uint8_t number)
{
    this->fragmentNumber = number;
}

void chatPacket::setTotalFragments(uint8_t number)
{
    this->totalFragments = number;
}

bool chatPacket::isFragment()
{
    return this->fragment;
}

QString chatPacket::getSourceName() const
{
    return sourceName;
}

QString chatPacket::getDestinationName() const
{
    return destinationName;
}

QByteArray chatPacket::getPacketData() const
{
    return packetData;
}

void chatPacket::setPacketData(const QByteArray &value)
{
    this->packetData = value;
    this->packetData.truncate(this->maxDataLength);
    this->dataLength = this->packetData.length();
}

uint8_t chatPacket::getFragmentNumber() const
{
    return fragmentNumber;
}

uint8_t chatPacket::getTotalFragments() const
{
    return totalFragments;
}

chatPacket::dataType chatPacket::getPacketType() const
{
    return packetType;
}

void chatPacket::setPacketType(const dataType &value)
{
    packetType = value;
}

QByteArray chatPacket::getPacketId() const
{
    return packetId;
}

void chatPacket::setAckId(const QByteArray &value)
{
    this->ackId = value;
}

QByteArray chatPacket::getAckId() const
{
    return ackId;
}

chatPacket::chatPacket()
{

}

chatPacket::chatPacket(const chatPacket &packet)
{
    this->sourceName = packet.getSourceName();
    this->destinationName = packet.getDestinationName();
    this->dataLength = packet.dataLength;
    this->packetData = packet.getPacketData();
    this->fragment = packet.fragment;
    this->fragmentNumber = packet.getFragmentNumber();
    this->totalFragments = packet.getTotalFragments();
    this->packetType = packet.getPacketType();
    this->packetId = packet.getPacketId();
    this->ackId = packet.getAckId();
}
