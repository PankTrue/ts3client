#include "ts3client.h"

Ts3Client::Ts3Client(QString addr,QString NickName, QString Password, uint DefaultChannelID) : Ts3FullClient()
{

    IdentityData *identity = Ts3Crypt::GenerateNewIdentity(8);

    connectionData = new ConnectionData(addr,identity,NickName,Password,DefaultChannelID);
    this->SetConnectionData(connectionData);
}

Ts3Client::Ts3Client(IdentityData *identity, QString addr, QString NickName, QString Password, uint DefaultChannelID)
{
    connectionData = new ConnectionData(addr,identity,NickName,Password,DefaultChannelID);
    this->SetConnectionData(connectionData);
}

Ts3Client::~Ts3Client()
{

}

void Ts3Client::ClientEnterView(NotificationData *notify)
{

}

void Ts3Client::ClientLeftView(NotificationData *notify)
{

}

void Ts3Client::ClientMoved(NotificationData *notify)
{

}

void Ts3Client::MessageParse(NotificationData *notify)
{

}

void Ts3Client::RefreshClientList(NotificationData *notify)
{
//    notify->PrintData();
}

void Ts3Client::ChannelList(NotificationData *notify)
{

}
