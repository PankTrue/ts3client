#include "notificationdata.h"

NotificationData::~NotificationData()
{
    for (auto it : values)
        delete [] it;

    for (auto vec = parsedData.begin();vec!=parsedData.end();vec++)
        for(auto map = vec->begin();map!=vec->end();map++)
            delete [] map.value();
}

NotificationData* NotificationData::ParseMessage(Byte str)
{
    NotificationData *notification = new NotificationData;

    notification->type = NotificationData::GetNotificationType((char *)str.value());

    if(notification->type!= NotificationType::Error)
    {
        char *point = NULL;
        char *tmp;
        ushort size;
        char *strTmp;

        bool have_delimiter;
        char *delimiter;

        if(notification->type!=NotificationType::Unknown)
            strTmp = strchr((char *)str.value(),' ');
        else
            strTmp = (char *)str.value();

    if(strTmp==NULL)
    {
        size = str.size();
        tmp = new char[size];
        memcpy(tmp,(char *)str.value(),size-1);
        tmp[size-1] = '\0';
        notification->values.push_back(tmp);
        return notification;
    }else{
        if(strTmp != (char *)str.value())
            strTmp++;
    }

    delimiter = strchr(strTmp,'|');

    if(delimiter == NULL)
        have_delimiter = false;
    else
        have_delimiter = true;


        while(true)
        {
            point = strchr(strTmp,' ');

            if(have_delimiter)
            {
                if(delimiter < point)
                {
                    size = (delimiter - strTmp)+1;
                    tmp = new char[size];
                    memcpy(tmp,strTmp,size-1);
                    tmp[size-1] = '\0';

                    notification->values.push_back(tmp);

                    tmp = new char[2];
                    tmp[0] = '|';
                    tmp[1] = '\0';
                    notification->values.push_back(tmp);
                    *delimiter = ' ';
                    point = delimiter+1;

                    delimiter = strchr(point,'|');

                    if (delimiter == NULL) {
                        have_delimiter = false;
                    }

                    strTmp = point;

                    continue;
                }
            }

            if(point == NULL)
            {
                size = ((char *)str.value() + str.size() + 1) - strTmp;
                if(size == 0)
                    break;
                tmp = new char[size];
                memcpy(tmp,strTmp,size-1);
                tmp[size-1] = '\0';
                notification->values.push_back(tmp);
                break;
            }
            size = (point - strTmp)+1;
            tmp = new char[size];
            memcpy(tmp,strTmp,size-1);
            tmp[size-1] = '\0';
            strTmp = point + 1;
            notification->values.push_back(tmp);
        }


    }else{
        qWarning() << (char *)str.data;
    }



    return notification;
}

NotificationType NotificationData::GetNotificationType(char *str)
{
    QString temp;

    while (1)
    {
        if(*str == ' ' || *str == '\0' || *str == '=')
            break;

        temp.append(*str++);
    }

          if(temp == "error") return NotificationType::Error;
          else if(temp == "notifyclientchatclosed")return NotificationType::NotifyClientChatClosed;
          else if(temp == "notifyclientpoke")return NotificationType::NotifyClientPoke;
          else if(temp == "notifyclientupdated")return NotificationType::NotifyClientUpdated;
          else if(temp == "notifyclientchannelgroupchanged")return NotificationType::NotifyClientChannelGroupChanged;
          else if(temp == "notifyclientpasswordchanged")return NotificationType::NotifyClientPasswordChanged;
          else if(temp == "notifyclientdescriptionchanged")return NotificationType::NotifyClientDescriptionChanged;
          else if(temp == "notifychannelchanged")return NotificationType::ChannelChanged;
          else if(temp == "notifychannelcreated") return NotificationType::ChannelCreated;
          else if(temp == "notifychanneldeleted") return NotificationType::ChannelDeleted;
          else if(temp == "notifychanneledited") return NotificationType::ChannelEdited;
          else if(temp == "notifychannelmoved") return NotificationType::ChannelMoved;
          else if(temp == "notifychannelpasswordchanged") return NotificationType::ChannelPasswordChanged;
          else if(temp == "notifycliententerview") return NotificationType::ClientEnterView;
          else if(temp == "notifyclientleftview") return NotificationType::ClientLeftView;
          else if(temp == "notifyclientmoved") return NotificationType::ClientMoved;
          else if(temp == "notifyserveredited") return NotificationType::ServerEdited;
          else if(temp == "notifytextmessage") return NotificationType::TextMessage;
          else if(temp == "notifytokenused") return NotificationType::TokenUsed;
          else if(temp == "channellist") return NotificationType::ChannelList;
          else if(temp == "channellistfinished") return NotificationType::ChannelListFinished;
          else if(temp == "initivexpand") return NotificationType::InitIvExpand;
          else if(temp == "initserver") return NotificationType::InitServer;
          else if(temp == "notifychannelsubscribed") return NotificationType::ChannelSubscribed;
          else if(temp == "notifychannelunsubscribed") return NotificationType::ChannelUnsubscribed;
          else if(temp == "notifyclientchannelgroupchanged") return NotificationType::ClientChannelGroupChanged;
          else if(temp == "notifyclientchatcomposing") return NotificationType::ClientChatComposing;
          else if(temp == "notifyclientneededpermissions") return NotificationType::ClientNeededPermissions;
          else if(temp == "notifyconnectioninfo") return NotificationType::ConnectionInfo;
          else if(temp == "notifyconnectioninforequest") return NotificationType::ConnectionInfoRequest;
          else if(temp == "notifyfileinfo") return NotificationType::FileInfo;
          else if(temp == "notifyfilelist") return NotificationType::FileList;
          else if(temp == "notifyfilelistfinished") return NotificationType::FileListFinished;
          else if(temp == "notifyfiletransferlist") return NotificationType::FileTransfer;
          else if(temp == "notifyservergroupclientadded") return NotificationType::ClientServerGroupAdded;
          else if(temp == "notifyservergroupclientdeleted") return NotificationType::NotifyServerGroupClientDeleted;
          else if(temp == "notifyservergrouplist") return NotificationType::ServerGroupList;
          else if(temp == "notifyservergroupsbyclientid") return NotificationType::ServerGroupsByClientId;
          else if(temp == "notifystartdownload") return NotificationType::StartDownload;
          else if(temp == "notifystartupload") return NotificationType::StartUpload;
          else if(temp == "notifystatusfiletransfer") return NotificationType::FileTransferStatus;
          else if(temp == "notifychannelgrouplist") return NotificationType::NotifyChannelGroupList;

          else return NotificationType::Unknown;
}

void NotificationData::FullParseMessage()
{
    if (this->values.empty())
        qWarning() << "NotificationData => FullParseMessage " << "values is empty";
    if (!this->parsedData.empty())
    {
        qInfo() << "duplicate call FullParseMessage()";
        return;
    }


    char *point = NULL;
    char *key;
    char *value;
    ushort size_key;
    ushort size_value;
    QMap<QString,char *> map;


    for (auto it : values)
    {
            point = strchr(it,'=');

            if(point == NULL)
            {
                if(it[0] == '|')
                {
                    parsedData.push_back(map);
                    map.clear();
                    continue;
                }
                size_key = strlen(it);
                key = new char[size_key+1];
                memcpy(key,it,size_key);
                key[size_key] = '\0';

                value = new char[1];
                value[0] = '\0';
                map.insert(key,value);
                delete [] key;
                continue;
            }

            size_key = (point - it);
            key = new char[size_key+1];
            memcpy(key,it,size_key);
            key[size_key] = '\0';

            point++;


            size_value = strlen(point);
            value = new char[size_value+1];
            memcpy(value,point,size_value);
            value[size_value] = '\0';

            map.insert(key,value);

            delete [] key;
    }

    parsedData.push_back(map);
}

void NotificationData::PrintData()
{
    std::cout << "PrintData " << this->type <<std::endl;

    for (auto it : values)
    {
        std::cout <<"\t" << *it << std::endl;
    }

    std::cout << std::endl;
}

void NotificationData::SaveInFile()
{

    std::ofstream file("DEBUG.log");

    file << "PrintData " << "Type: " << std::to_string(this->type) << std::endl;

    for (auto it : values)
    {
        file << "\t" << *it << std::endl;;
    }
    file << std::endl;

    file.close();
}

void NotificationData::PrintFullParsedData()
{
//    std::cout << "Print FULL DATA" << std::endl;
//    for (QMap<QString,char *>::iterator it = this->parsedData.begin(); it != this->parsedData.end(); it++)
//    {
//        std::cout << '\t' << it.key().data() << "=" << it.value() << std::endl;
//    }
//    std::cout << std::endl;
}

