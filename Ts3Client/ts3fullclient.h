#ifndef TS3FULLCLIENT_H
#define TS3FULLCLIENT_H

#include "packethandler.h"
#include "commandmaker.h"
#include "notificationdata.h"
#include "tsdnsresolver.h"




class Ts3FullClient
{
public:
    Ts3FullClient();
    ~Ts3FullClient();


    const uint timeout = 60;



    //Receive
    void ProcessInitIvExpand(NotificationData *notify);
    void ProcessInitServer(NotificationData *notify);
    virtual void ClientEnterView(NotificationData *notify) = 0;
    virtual void ClientLeftView(NotificationData *notify) = 0;
    virtual void ClientMoved(NotificationData *notify) = 0;
    virtual void MessageParse(NotificationData *notify) = 0;
    virtual void ChannelList(NotificationData *notify) = 0;
    virtual void RefreshClientList(NotificationData *notify) = 0;


    //Send without respond
    void ProcessConnectionInfoRequest();
    void SendMessage(QString message,int clientID,int target_mode);
    void SendPrivateMessage(QString message, int clientID);
    void ChangeDescription(const char *description, int clientID);
    void ChangeName(char *name);

    //Send with respond
    void ClientList();

    ushort ClientID(){return packetHandler->ClientID;}


    ConnectionData *connectionData;


    void SetConnectionData(ConnectionData *conData);
    void Connect();
    void Disconnect();
    void SendAudio(Byte buffer);

    void ConnectionHandler();
    void Stop();
    void NetworkLoop();
    void CheckTimeoutWorker();
    void CommandHandler(NotificationData *notify);
    void UnknownCommandHandler(NotificationData *notify);


    bool Closed = true;
    bool clientView;


    ushort ServerID;
    ushort ChannelID;


    QSet<int> clientViewList;



    Ts3Crypt *ts3crypt;
    IdentityData *identity;
    PacketHandler *packetHandler;
    Ts3ClientStatus Status;



    std::mutex StatusLock;

    std::thread NetworkLoopThread;
    std::thread CheckTimeoutThread;


    std::pair<QString, ushort> ResolverAndParserAddr(QString addr);

};

#endif // TS3FULLCLIENT_H
