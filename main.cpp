#include "mainwindow.h"
#include "simplecrypt.h"
#include <QApplication>
#include <QBuffer>
#include <iostream>

QByteArray encrypt(QByteArray input){
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
    s << input;
    // Make a cypher with stream's data
    QByteArray myCypherText = crypto.encryptToByteArray(buffer.data());
        if (crypto.lastError() == SimpleCrypt::ErrorNoError) {
        // do something relevant with the cyphertext, such as storing it or sending it over a socket to another machine.
        }
    buffer.close();
    return myCypherText;
}

QByteArray decrypt(QByteArray inputCypher){
    // Create an empty QByteArray to output
    QByteArray decryptedOutput("");
    // Initialize SimpleCrypt object with hexadecimal key = (0x)40b50fe120bbd01b
    SimpleCrypt crypto2(Q_UINT64_C(0x40b50fe120bbd01b));
    QByteArray plaintext = crypto2.decryptToByteArray(inputCypher);
        if (!crypto2.lastError() == SimpleCrypt::ErrorNoError) {
        // check why we have an error, use the error code from crypto.lastError() for that
        return 0;
        }
    // Stream the data into a buffer
    QBuffer buffer2(&plaintext);
    buffer2.open(QIODevice::ReadOnly);
    QDataStream s2(&buffer2);
    s2.setVersion(QDataStream::Qt_4_7);
    // Output the decrypted cypher in our QByteArray
    s2 >> decryptedOutput;
    buffer2.close();
    return decryptedOutput;
}

int main(int argc, char *argv[])
{
    // Original Text
    QByteArray testInput("This is a test text!");
    std::cout << testInput.constData() << std::endl;

    QByteArray cypher = encrypt(testInput);
    std::cout << cypher.toHex().constData() << std::endl;
    // Decryption
    QByteArray testOutput = decrypt(cypher);
    std::cout << testOutput.constData() << std::endl;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
