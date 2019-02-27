#include "ts3fullclient.h"

Ts3FullClient::Ts3FullClient()
{
    Status = Ts3ClientStatus::Disconnected;
    ts3crypt = new Ts3Crypt();
    packetHandler = new PacketHandler(ts3crypt);
    Closed = true;
    clientView = false;
    ChannelID = 0;
}

Ts3FullClient::~Ts3FullClient()
{
    Disconnect();
    delete packetHandler;
    delete connectionData;
}

void Ts3FullClient::SetConnectionData(ConnectionData *conData)
{
    if(!conData->Valid())
        return;

    connectionData = conData;
    ts3crypt->identity = connectionData->identity;

    auto parsed_addr = TsDnsResolver::ResolveAndParseAddr(conData->HostName);

    connectionData->HostName = parsed_addr.first;
    connectionData->Port = parsed_addr.second;

    packetHandler->SetConnectData(connectionData->HostName.toLocal8Bit().data(),connectionData->Port);
}


void Ts3FullClient::Connect()
{
    Disconnect();

    StatusLock.lock();
        packetHandler->lastReceivePacketTime = time(NULL);
        Closed = false;
        packetHandler->Connect();
#ifndef QT_DEBUG
        this->CheckTimeoutThread = thread(&Ts3FullClient::CheckTimeoutWorker,this);
#endif
        this->NetworkLoopThread = thread(&Ts3FullClient::NetworkLoop,this);
        Status = Ts3ClientStatus::Connected;
    StatusLock.unlock();
}

void Ts3FullClient::Disconnect()
{
    StatusLock.lock();

    CommandMaker packet;
    packet.SetLabel("clientdisconnect");
    packet.AddCommand("reasonid",(int)MoveReason::LeftServer);
    packet.AddCommand("reasonmsg","Bye bitch!");

    packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);


    if(Status == Ts3ClientStatus::Connected)
    {
	close(packetHandler->sock);
        Closed = true;
        CheckTimeoutThread.detach();
        NetworkLoopThread.detach();
        Status = Ts3ClientStatus::Disconnected;
        ts3crypt->Reset();
    }
    StatusLock.unlock();

}

void Ts3FullClient::SendAudio(Byte buffer)
{
    Byte tmpBuffer(buffer.size() + 3);
    tmpBuffer[2] = (byte)5; //codec
    memcpy(tmpBuffer.value()+3,buffer.value(),buffer.size());
    delete [] &buffer;

    packetHandler->AddOutgoingPacket(tmpBuffer,PacketType::Voice);
}

void Ts3FullClient::NetworkLoop()
{
        IncomingPacket *packet;
        while(1)
        {
            packet = packetHandler->FetchPacket();

            if(packet == NULL)
            {
                qDebug() << "Stop thread";
                return;
            }


            switch (packet->getPacketType()) {

                case PacketType::Voice:
                case PacketType::VoiceWhisper:
                    break;
                case PacketType::Command:
                case PacketType::CommandLow:
                {
                    NotificationData *notify = NotificationData::ParseMessage(packet->Data);
//                    packet->Data.DEBUG_TO_CHAR();
                    CommandHandler(notify);

                    delete notify;
                    break;
                }
                case PacketType::Init1:
                {
                    Byte forwarddata = ts3crypt->ProcessInit1(packet->Data);

                    if(forwarddata.value() == NULL)
                        break;
                    packetHandler->AddOutgoingPacket(forwarddata,PacketType::Init1);
                    break;
                }

	    default: break;
            }

	    delete packet;
        }
}

void Ts3FullClient::CheckTimeoutWorker()
{
    while(true)
    {
        if((packetHandler->lastReceivePacketTime + timeout) < time(NULL))
        {
            exit(0);
        }

        sleep(1);
    }
}

void Ts3FullClient::CommandHandler(NotificationData *notify)
{
    switch (notify->type) {
        case NotificationType::ChannelCreated: break;
        case NotificationType::ChannelDeleted: break;
        case NotificationType::ChannelChanged: break;
        case NotificationType::ChannelEdited: break;
        case NotificationType::ChannelMoved: break;
        case NotificationType::ChannelPasswordChanged: break;
        case NotificationType::ClientEnterView: ClientList();break;
        case NotificationType::ClientLeftView: ClientList();break;
        case NotificationType::ClientMoved: ClientMoved(notify);ClientList();break;
        case NotificationType::ServerEdited: break;
        case NotificationType::TextMessage: MessageParse(notify); break;
        case NotificationType::TokenUsed: break;
        case NotificationType::InitIvExpand: ProcessInitIvExpand(notify); break;
        case NotificationType::InitServer: ProcessInitServer(notify);break;
        case NotificationType::ChannelList:ChannelList(notify);break;
        case NotificationType::ChannelListFinished: break;
        case NotificationType::ClientNeededPermissions: break;
        case NotificationType::ClientChannelGroupChanged: break;
        case NotificationType::ClientServerGroupAdded: break;
        case NotificationType::ConnectionInfoRequest: /*ProcessConnectionInfoRequest()*/;break;
        case NotificationType::ConnectionInfo:break;
        case NotificationType::ChannelSubscribed: break;
        case NotificationType::ChannelUnsubscribed: break;
        case NotificationType::ClientChatComposing: break;
        case NotificationType::ServerGroupList: break;
        case NotificationType::ServerGroupsByClientId: break;
        case NotificationType::StartUpload: break;
        case NotificationType::StartDownload: break;
        case NotificationType::FileTransfer: break;
        case NotificationType::FileTransferStatus: break;
        case NotificationType::FileList: break;
        case NotificationType::FileListFinished: break;
        case NotificationType::FileInfo: break;
        case NotificationType::NotifyChannelGroupList: break;
        case NotificationType::NotifyClientUpdated: break;
        case NotificationType::NotifyClientChatClosed:break;
        case NotificationType::NotifyClientPoke:break;
        case NotificationType::NotifyClientChannelGroupChanged:break;
        case NotificationType::NotifyClientPasswordChanged:break;
        case NotificationType::NotifyClientDescriptionChanged:break;
        case NotificationType::Unknown:UnknownCommandHandler(notify);break;
    default: break;
    }
}

void Ts3FullClient::UnknownCommandHandler(NotificationData *notify)
{
    RefreshClientList(notify);
    notify->PrintData();
}

void Ts3FullClient::ProcessInitIvExpand(NotificationData *notify)
{
    notify->FullParseMessage();

    ts3crypt->CryptoInit(notify->parsedData[0].find("alpha").value(),
                         notify->parsedData[0].find("beta").value(),
                         notify->parsedData[0].find("omega").value());

    packetHandler->CryptoInitDone();

    QString default_channel;
    if(connectionData->DefaultChannel == 0)
         default_channel = "";
    else
        default_channel.append("/").append(QString::number(connectionData->DefaultChannel));

    Byte password_finaly;
    if(!connectionData->Password.isEmpty())
    {
        Byte password_sha(CryptoPP::SHA1::DIGESTSIZE);
        CryptoPP::SHA1().CalculateDigest(password_sha.value(), (byte *)connectionData->Password.toLocal8Bit().data(), connectionData->Password.size());
        password_finaly = EncodeBase64(password_sha);
    }

    CommandMaker packetRaw("clientinit");
    packetRaw.AddCommand("client_nickname",connectionData->UserName.toLocal8Bit().data());
    packetRaw.AddCommand("client_version",VersionSign::Name);
    packetRaw.AddCommand("client_platform",VersionSign::PlattformName);
    packetRaw.AddCommand("client_input_hardware",1);
    packetRaw.AddCommand("client_output_hardware",1);
    packetRaw.AddCommand("client_default_channel",default_channel.isEmpty() ? "" : default_channel.toLocal8Bit().data());
    packetRaw.AddCommand("client_default_channel_password");
    packetRaw.AddCommand("client_server_password",password_finaly);
    packetRaw.AddCommand("client_meta_data");
    packetRaw.AddCommand("client_version_sign",VersionSign::Sign);
    packetRaw.AddCommand("client_key_offset",ts3crypt->identity->ValidKeyOffset);
    packetRaw.AddCommand("client_nickname_phonetic");
    packetRaw.AddCommand("client_default_token");
    packetRaw.AddCommand("hwid","123,456");


    packetHandler->AddOutgoingPacket(packetRaw.GetResultByte(),PacketType::Command);
}

void Ts3FullClient::ProcessInitServer(NotificationData *initServer)
{
    initServer->FullParseMessage();

    std::sscanf(initServer->parsedData[0].find("virtualserver_id").value(),"%d",&this->ServerID);
    std::sscanf(initServer->parsedData[0].find("aclid").value(),"%d",&packetHandler->ClientID);

    packetHandler->ReceiveInitAck();

    Status = Ts3ClientStatus::Connected;
}

void Ts3FullClient::ProcessConnectionInfoRequest()
{
    CommandMaker packet("setconnectioninfo");

        packet.AddCommand("connection_ping", 1488);
        packet.AddCommand("connection_ping_deviation", 1);
        packet.AddCommand("connection_packets_sent_speech", 1);
        packet.AddCommand("connection_packets_sent_keepalive", 1);
        packet.AddCommand("connection_packets_sent_control", 1);
        packet.AddCommand("connection_bytes_sent_speech", 1);
        packet.AddCommand("connection_bytes_sent_keepalive", 1);
        packet.AddCommand("connection_bytes_sent_control", 1);
        packet.AddCommand("connection_packets_received_speech", 1);
        packet.AddCommand("connection_packets_received_keepalive", 1);
        packet.AddCommand("connection_packets_received_control", 1);
        packet.AddCommand("connection_bytes_received_speech", 1);
        packet.AddCommand("connection_bytes_received_keepalive", 1);
        packet.AddCommand("connection_bytes_received_control", 1);
        packet.AddCommand("connection_server2client_packetloss_speech", 42.0000f);
        packet.AddCommand("connection_server2client_packetloss_keepalive", 1.0000f);
        packet.AddCommand("connection_server2client_packetloss_control", 0.5000f);
        packet.AddCommand("connection_server2client_packetloss_total", 0.0000f);
        packet.AddCommand("connection_bandwidth_sent_last_second_speech", 1);
        packet.AddCommand("connection_bandwidth_sent_last_second_keepalive", 1);
        packet.AddCommand("connection_bandwidth_sent_last_second_control", 1);
        packet.AddCommand("connection_bandwidth_sent_last_minute_speech", 1);
        packet.AddCommand("connection_bandwidth_sent_last_minute_keepalive", 1);
        packet.AddCommand("connection_bandwidth_sent_last_minute_control", 1);
        packet.AddCommand("connection_bandwidth_received_last_second_speech", 1);
        packet.AddCommand("connection_bandwidth_received_last_second_keepalive", 1);
        packet.AddCommand("connection_bandwidth_received_last_second_control", 1);
        packet.AddCommand("connection_bandwidth_received_last_minute_speech", 1);
        packet.AddCommand("connection_bandwidth_received_last_minute_keepalive", 1);
        packet.AddCommand("connection_bandwidth_received_last_minute_control", 1);

        packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
}

void Ts3FullClient::SendMessage(QString message, int clientID, int target_mode)
{
    QStringList parsed_message = message.split("\n");
    QString data;
    ushort couter_next_line = 0;

    for (auto current_line = parsed_message.begin(); current_line != parsed_message.end(); current_line++)
    {
        if(((data.toUtf8().size() + couter_next_line) + current_line->toUtf8().size()) >= 1024 && current_line->toUtf8().size() <= 1024)
        {
            CommandMaker packet;
            packet.SetLabel("sendtextmessage");
            packet.AddCommand("targetmode", target_mode); // 1 - private
            packet.AddCommand("target",clientID);
            packet.AddCommand("msg",data.toLocal8Bit().data());
            packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
            data.clear();
            data.append("\n");
            couter_next_line=1;
        }

        if((current_line->toUtf8().size()) <= 1024)
        {
            data.append(*current_line);
            data.append("\n");
            couter_next_line++;
        }else{

            int current_line_offset=0;
            if(data.size() != 0)
            {
                CommandMaker packet;
                packet.SetLabel("sendtextmessage");
                packet.AddCommand("targetmode", target_mode); // 1 - private
                packet.AddCommand("target",clientID);
                packet.AddCommand("msg",data.toLocal8Bit().data());
                packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
                data.clear();
                couter_next_line=0;
            }

            while(current_line_offset < current_line->toUtf8().size())
            {
                Byte tmp(std::min(1024,current_line->toUtf8().size() - current_line_offset));
                memcpy(tmp.value(),current_line->toUtf8().data(),tmp.size());
                current_line_offset+=tmp.size();

                CommandMaker packet;
                packet.SetLabel("sendtextmessage");
                packet.AddCommand("targetmode", target_mode); // 1 - private
                packet.AddCommand("target",clientID);
                packet.AddCommand("msg",tmp);
                packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
                data.clear();
            }
        }

    }

    if(data.size() > 1)
    {
        CommandMaker packet;
        packet.SetLabel("sendtextmessage");
        packet.AddCommand("targetmode", target_mode); // 1 - private
        packet.AddCommand("target",clientID);
        packet.AddCommand("msg",data.toLocal8Bit().data());
        packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
    }

}

void Ts3FullClient::SendPrivateMessage(QString message, int clientID)
{
    SendMessage(message,clientID,1);
}

void Ts3FullClient::ChangeDescription(const char *description, int clientID)
{
    CommandMaker packet("clientedit");
    packet.AddCommand("clid",clientID);
    packet.AddCommand("client_description",description);
    packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
}

void Ts3FullClient::ChangeName(char *name)
{
    CommandMaker packet("clientupdate");
    packet.AddCommand("client_nickname",name);
    packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
}

void Ts3FullClient::ClientList()
{
    CommandMaker packet("clientlist");
    packetHandler->AddOutgoingPacket(packet.GetResultByte(),PacketType::Command);
}

