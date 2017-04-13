#ifndef CHATPACKET_H
#define CHATPACKET_H
#include <QString>
#include <QByteArray>
#include <QCryptographicHash>

#include <iostream>

class chatPacket
{
public:
    enum dataType : uint8_t{
        text,
        image,
        audio
    };
private:
    const uint8_t maxNameLength = 32;
    const uint8_t maxDataLength = 255;
    const uint8_t maxHashLength = 20;
    QString sourceName = "broadcast";
    QString destinationName = "broadcast";
    uint8_t dataLength = 0;
    QByteArray packetData; //message
    bool fragment = false;
    uint8_t fragmentNumber = 0;
    uint8_t totalFragments = 0;

    dataType packetType = dataType::text;
    QByteArray packetId = "00000000000000000000";
    QByteArray ackId = "00000000000000000000";
public:
    chatPacket();

    QByteArray toByteArray();
    void fromByteArray(QByteArray bytes);
    void makeHash();

    void setSourceName(QString name);
    void setDestinationName(QString name);
    void setFragment();
    void unsetFragment();
    void setFragmentNumber(uint8_t number);
    void setTotalFragments(uint8_t number);

    bool isFragment();

    QString getSourceName() const;
    QString getDestinationName() const;
    QByteArray getPacketData() const;
    void setPacketData(const QByteArray &value);
    uint8_t getFragmentNumber() const;
    uint8_t getTotalFragments() const;
    dataType getPacketType() const;
    void setPacketType(const dataType &value);
    QByteArray getPacketId() const;
    void setAckId(const QByteArray &value);
    QByteArray getAckId() const;
};

#endif // CHATPACKET_H
