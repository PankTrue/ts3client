#ifndef TS3CLIENT_H
#define TS3CLIENT_H


#include "ts3fullclient.h"

class Ts3Client : public Ts3FullClient
{
public:
    Ts3Client(QString addr, QString NickName, QString Password, uint DefaultChannelID);
    Ts3Client(IdentityData *identity, QString addr, QString NickName, QString Password, uint DefaultChannelID);
    ~Ts3Client();


    void ClientEnterView(NotificationData *notify);
    void ClientLeftView(NotificationData *notify);
    void ClientMoved(NotificationData *notify);
    void MessageParse(NotificationData *notify);
    void RefreshClientList(NotificationData *notify);
    void ChannelList(NotificationData *notify);
};

#endif // TS3CLIENT_H
