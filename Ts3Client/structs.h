#ifndef STRUCTS_H
#define STRUCTS_H

#include <memory>
#include <cstring>
#include <iostream>
#include "cryptopp/integer.h"
#include <byte.h>
#include <QString>
#include <QDebug>

typedef unsigned short ushort;


struct IdentityData
{
public:
    IdentityData(CryptoPP::Integer PrivateKey,Byte PublicKeyString,ulong ValidKeyOffset,ulong LastCheckedKeyOffset)
        :PrivateKey(PrivateKey),PublicKeyString(PublicKeyString),ValidKeyOffset(ValidKeyOffset),LastCheckedKeyOffset(LastCheckedKeyOffset){}

    IdentityData(){}

    ~IdentityData(){}

    Byte PublicKeyString;
    Byte PrivateKeyString;   
    Byte PublicKeyStringServer;

    unsigned long long ValidKeyOffset;
    unsigned long long LastCheckedKeyOffset;

     CryptoPP::Integer PrivateKey;
    //    CryptoPP::ECPPoint PublicKey;
};

struct VersionSign
{
//    static constexpr const char *Name = "3.0.19.4 [Build: 1468491418]";
//    static constexpr const char *Sign = "jvhhk75EV3nCGeewx4Y5zZmiZSN07q5ByKZ9Wlmg85aAbnw7c1jKq5/Iq0zY6dfGwCEwuKod0I5lQcVLf2NTCg==";
//    static constexpr const char *PlattformName = "Linux";

    static constexpr const char *Name = "3.1.8 [Build: 1516614607]";
    static constexpr const char *Sign = "gDEgQf/BiOQZdAheKccM1XWcMUj2OUQqt75oFuvF2c0MQMXyv88cZQdUuckKbcBRp7RpmLInto4PIgd7mPO7BQ==";
    static constexpr const char *PlattformName = "Windows";
};

struct ConnectionData
{
    QString HostName;
    ushort Port;
    IdentityData *identity;
    QString UserName;
    QString Password;
    uint DefaultChannel;

    bool Valid()
    {
    if(this->identity == NULL || this->HostName.isEmpty() || this->UserName.isEmpty())
    {
        qCritical() << "No valid connetion data";
        return false;
    }else{return true;}
    }

    ConnectionData():Port(0),identity(NULL),DefaultChannel(0){}

    ConnectionData(QString addr,IdentityData *identity,QString UserName, QString Password, uint DefaultChannel):
        HostName(addr),identity(identity),UserName(UserName),Password(Password),DefaultChannel(DefaultChannel){}


    ~ConnectionData(){}

};


#endif // STRUCTS_H
