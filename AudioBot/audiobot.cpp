#include "audiobot.h"

audiobot::audiobot(QString addr,QString NickName, QString Password, uint BotID, uint DefaultChannelID,QString ts_path, bool playRandAudio, int quota_audio_size): Ts3FullClient(),
    ts_path(ts_path),nextAudioRandom(playRandAudio),BotID(BotID),quota_audio_size(quota_audio_size),decodingStatus(false),radioStatus(false)
{
    pathToData.append("data/");
    pathToData.append(QString::number(BotID));

    pathForAudio.append(pathToData);
    pathForAudio.append("/AudioFiles/");

    audioFilesDecodedDir.mkpath(pathForAudio);
    audioFilesDecodedDir.cd(pathForAudio);

    fullPathForTeamspeakAudioSource = pathToData + "/AudioFilesSource";
    audioFilesSourceDir.cd(fullPathForTeamspeakAudioSource);

    IdentityData *identity = InitIdentity();

    this->connectionData = new ConnectionData(addr,identity,NickName,Password,DefaultChannelID);
    this->SetConnectionData(connectionData);


    // INIT OPUS
    encoder = opus_encoder_create(SAMPLE_RATE,CHANNELS,APPLICATION,&err);
    opus_encoder_ctl(encoder,OPUS_SET_BITRATE(BITRATE));
    opus_encoder_ctl(encoder,OPUS_SET_COMPLEXITY(0));

    audioNameFilter << "*.mp3" << "*.opus" << "*.wav" << "*.m4a";


    audiobotThread = std::thread(&audiobot::AudioWorker,this);
    audiobotControllerThread = std::thread(&audiobot::AudioWorkerController,this);
}

audiobot::~audiobot(){}

void audiobot::AudioWorkerController()
{
//    struct timespec time_for_sleep;
//    time_for_sleep.tv_nsec = 20000*packets_per_send*10; // 1/100
//    time_for_sleep.tv_sec = 0;



//    struct sched_param sp = { .sched_priority = 99 };
//    if(sched_setscheduler(getpid(),SCHED_FIFO,&sp) != 0)
//    {
//        qWarning() << "error in sched_setscheduler";
//    }


 std::unique_lock<std::mutex> locker(audioWorkerControllerMutex);

    while(true)
    {
        //wait signal
        while (!audioWorkerControllerRetValue)
            audioWorkerControllerCV.wait(locker);

        uint64_t ideal_play_time = getCurrentTime_Micro - ((20000 * PACKETS_PER_SEND / 10));


        //sender
        while (!audio.empty() && clientView && audioStatus && !Closed)
        {
            while(getCurrentTime_Micro  < (ideal_play_time - (500000)))
            {
//                pselect(0,NULL,NULL,NULL,&time_for_sleep,NULL);
//                select(0,0,0,0,);

                usleep(20 * 10 * PACKETS_PER_SEND); // 1/10
            }

            audioWorkerRetValue = true;
            audioWorkerCV.notify_one();

            ideal_play_time += 20000 * PACKETS_PER_SEND;
        }

        if(!audioStatus || !clientView || Closed || radioStatus)
        {
            audioWorkerControllerRetValue=false;
            continue;
        }

        if(audio.empty())
        {
            if(nextAudioRandom)
                PlayRandomAudio();
            else
                PlayNextAudio(1);
        }


    }

}

void audiobot::Play()
{
    audioStatus = true;

    if(clientView  && !audio.empty())
    {
        audioWorkerControllerRetValue=true;
        audioWorkerControllerCV.notify_one();
    }
}

void audiobot::Pause()
{
    audioWorkerControllerRetValue = false;
    audioStatus=false;
}

void audiobot::PlayBack()
{
    Pause();
    LoadAudioFile(current_audio_index);
    Play();
}

void audiobot::Start()
{
    this->Connect();
}

void audiobot::RemoveAudioFile(short ID)
{
    if((--ID) < audioListDecoded.size() && ID >= 0)
    {
        audioListMutex.lock();
        QFile::remove(audioListDecoded.at(ID).filePath());
        audioListMutex.unlock();
    }
    UpdatePlayList();
}

void audiobot::AudioWorker()
{
    ushort counter = 0;
    std::string description;

    std::unique_lock<std::mutex> locker(audioWorkerMutex);
    while(1)
    {
        while(!audioWorkerRetValue)
            audioWorkerCV.wait(locker);
        audioWorkerRetValue = false;
        //send discription
        if((counter+=PACKETS_PER_SEND) >= 100)
        {
            if(!radioStatus && current_audio_index < audioListDecoded.size() && current_audio_index >= 0)
            {
                counter=0;
                description.clear();
            audioListMutex.lock();
                    description.append(audioListDecoded.at(current_audio_index).fileName().toLocal8Bit().data())
                                .append(" | ")
                                .append(std::to_string(((current_loaded_audio_size_in_frames-audio.size())/50)))
                                .append("/")
                                .append(std::to_string(current_loaded_audio_size_in_frames/50));
            audioListMutex.unlock();
                ChangeDescription(description.data(),this->ClientID());
            }else{
//                TODO: finish send radio name and time current session in second
            audioListMutex.lock();
                description.clear();
                description.append("Radio: ")
                        .append(current_radio_id.toLocal8Bit().data())
                        .append(" => ")
                        .append(radio_list.find(current_radio_id).value().toString().toLocal8Bit().data());
            audioListMutex.lock();
                ChangeDescription(description.data(),this->ClientID());
            }
        }

        audioMutex.lock();
        for (int i = 0; i < std::min(PACKETS_PER_SEND,(int)audio.size()); i++)
        {
            SendAudio(audio.front());
            audio.pop();
        }
        audioMutex.unlock();
    }
}

void audiobot::DecodeAndSaveAudioFile(QString path, QString name)
{
    QFile outFile(pathForAudio+name);

    outFile.open(QIODevice::WriteOnly | QIODevice::Text);

    QProcess FFMPEG;
    QStringList paramList;
    short size;

    paramList << "-hide_banner" << "-nostats"
            << "-i" << path
            << "-ac" << QString::number(CHANNELS)
            << "-ar" << QString::number(BITRATE)
            << "-f" << "s16le"
            << "-acodec" << "pcm_s16le"
            << "pipe:1";
    FFMPEG.start("ffmpeg",paramList,QIODevice::ReadOnly);

        while(1)
        {
            FFMPEG.waitForReadyRead(4000);

            if((size = FFMPEG.read((char *)pcm_bytes,FRAME_SIZE*4)) <= 0)
                break;

            nbBytes = opus_encode(encoder,(opus_int16 *)pcm_bytes,FRAME_SIZE,cbits+1,MAX_PACKET_SIZE);

            if(nbBytes>255) qCritical() << "nbBytes>255";
            if(nbBytes==0) break;

            cbits[0] = (byte)nbBytes;
            outFile.write((const char *)cbits,nbBytes+1);
        }

outFile.close();
}

void audiobot::DecodeAllAudioInCurrentChannelAndSource(ushort ClientID)
{
        UpdatePlayList();
        if(audioListInCurrentChannel.size() == 0 && audioListSource.size() == 0)
        {
            SendPrivateMessage("Файлов для декодирования не найдено",ClientID);
            decodingThread.detach();
            return;
        }
        auto audioListTmp = audioListInCurrentChannel;
        audioListTmp += audioListSource;
        decodingStatus = true;

        SendPrivateMessage("Декодирование началось",ClientID);
        for (auto it = audioListTmp.begin(); it != audioListTmp.end(); it++)
        {
            UpdateAudioSize();
            if(audioSize > quota_audio_size)
                break;
            DecodeAndSaveAudioFile(it->filePath(),it->fileName());
            QFile::remove(it->filePath());
        }

        UpdatePlayList();
        SendPrivateMessage((QString("Декодирование закончено. Файлов добавлено: ") + QString::number(audioListTmp.size())).toLocal8Bit().data() ,ClientID);
        decodingThread.detach();
        decodingStatus = false;

}

void audiobot::SendFind(QString message, ushort ClientID)
{
    UpdatePlayList();
    audioListMutex.lock();
    ushort i = 1, packets = 0;
    QString finaly_message = "\n";
    for (auto it = audioListDecoded.begin(); it != audioListDecoded.end(); it++)
    {
        if(it->fileName().contains(message,Qt::CaseInsensitive))
        {
            packets++;
            finaly_message.append(QString::number(i));
            finaly_message.append(" ) ");
            finaly_message.append(it->fileName());
            finaly_message.append("\n");
        }
        i++;
    }

    if(finaly_message.size() >= 5)
        SendPrivateMessage(finaly_message.toLocal8Bit().data(),ClientID);
    else if(packets == 0)
        SendPrivateMessage("Ничего не найдено!", ClientID);

    audioListMutex.unlock();
}

void audiobot::SkipAudioFrames(ushort frames)
{
    Pause();

   int i = 0;

   audioMutex.lock();
   while(!audio.empty() && i <= frames)
   {
       delete [] &audio.front();
       audio.pop();
       i++;
   }
   audioMutex.unlock();

   if(audio.empty())
        PlayNextAudio(1);

    Play();
}

void audiobot::ClientEnterView(NotificationData *notify)
{
        QMap<QString, char *>::iterator client_type;
        QMap<QString, char *>::iterator clid;
        QMap<QString, char *>::iterator cid;


//        notify->PrintData();

        for (auto&& message : notify->parsedData)
        {

            client_type = message.find("client_type");
            clid = message.find("clid"); //ClientID
            cid = message.find("cid"); //ChannelID


            if(atoi(client_type.value()) != 0)
                continue;

            if(atoi(clid.value()) == ClientID())
            {
                //Set current channelID
                ChannelID = atoi(cid.value());
                continue;
            }

            clientViewList.insert(atoi(clid.value()));
            }

        if(clientViewList.empty())
        {
            clientView = false;
        }
        else
        {
            if(radioStatus)
                RadioPlay(current_radio_id);
            clientView = true;
            this->Play();
        }

}

void audiobot::ClientLeftView(NotificationData *notify)
{
        QMap<QString, char *>::iterator it;

        for (auto&& message : notify->parsedData)
        {

            it=message.find("clid");

            if(this->clientViewList.contains(atoi(it.value())))
            {
                this->clientViewList.remove(atoi(it.value()));
            }

         }

        qDebug() << clientViewList;

        if(clientViewList.empty())
        {
            clientView = false;
        }
        else
        {
            clientView = true;
            this->Play();
        }

}

void audiobot::ClientMoved(NotificationData *notify)
{
    notify->FullParseMessage();

    QMap<QString, char *>::iterator clid;
    QMap<QString, char *>::iterator cid;
    for (auto&& message : notify->parsedData)
    {
        clid=message.find("clid");

        if(atoi(clid.value()) == ClientID())
        {
            //Update current channelID
            cid = message.find("ctid");
            ChannelID = atoi(cid.value());
        }

    }
    UpdatePathForAudio();
}

void audiobot::MessageParse(NotificationData *notify)
{
    notify->FullParseMessage();

    for (auto &&it : notify->parsedData)
    {
        auto msg =          it.find("msg");
        auto targetmode =   it.find("targetmode");
        auto invokerID =    it.find("invokerid");


        int target_mode = atoi(targetmode.value());

        if((target_mode != 1 && target_mode != 2)|| msg.value()[0] != '!' || atoi(invokerID.value()) == ClientID())
            return;

        QStringList command = QString(msg.value()+1).toLower().split("\\s");

        if(command[0] == "list"){UpdatePlayList();SendPlayList(atoi(invokerID.value()));}
        else if(command[0] == "play"){if(command.size()>1){PlayAudio(atoi(command[1].toLocal8Bit().data()));};}
        else if(command[0] == "next"){PlayNextAudio(1);}
        else if(command[0] == "decode"){TryDecodeAudioFiles(atoi(invokerID.value()));}
        else if(command[0] == "prev"){PlayNextAudio(-1);}
        else if(command[0] == "rand"){PlayRandomAudio();}
        else if(command[0] == "pause"){Pause();}
        else if(command[0] == "start"){Play();}
        else if(command[0] == "playback"){PlayBack();}
        else if(command[0] == "stop"){Pause(); LoadAudioFile(current_audio_index);}
        else if(command[0] == "skip"){if(command.size()>1){SkipAudioFrames(atoi(command[1].toLocal8Bit().data())*50);};}
        else if(command[0] == "current"){SendNameCurrentAudioFile(atoi(invokerID.value()));}
        else if(command[0] == "channel"){SendPrivateMessage(QString::number(ChannelID).toLocal8Bit().data(),atoi(invokerID.value()));}
        else if(command[0] == "size"){SendSize(atoi(invokerID.value()));}
        else if(command[0] == "remove"){if(command.size()>1){RemoveAudioFile(command[1].toInt());};}
        else if(command[0] == "find"){if(command.size()>1){SendFind(command[1],atoi(invokerID.value()));}}
        else if(command[0] == "help"){SendHelp(atoi(invokerID.value()));}
        else if(command[0] == "autorand"){if(command.size()>1){EditAudioAutoRand(command[1]);};}
        else if(command[0] == "radio"){if(command.size()>1){RadioPlay(command[1]);}}
        else if(command[0] == "radio_add"){;}
        else if(command[0] == "radio_list"){SendRadioList(atoi(invokerID.value()));}
        else if(command[0] == "radio_remove"){;}
        else if(command[0] == "restart"){Disconnect();exit(0);}
        else SendPrivateMessage((char *)"Команда не найдена. Воспользуйте !help для просмотра списка команд.",atoi(invokerID.value()));
    }
}

void audiobot::RefreshClientList(NotificationData *notify)
{
    if(notify->values.size() < 5) //clientlist packet has 5+
        return;

    notify->FullParseMessage();


    QMap<QString, char *>::iterator clid; //ClientID
    QMap<QString, char *>::iterator cid; //ChannelID

    if(ChannelID == 0)
    {
        //Update current channel ID
        for (auto message = notify->parsedData.begin(); message != notify->parsedData.end(); message++)
        {
            clid=message->find("clid");

            if(clid == message->end())
                return;

            if(atoi(clid.value()) == ClientID())
            {
                cid = message->find("cid");
                ChannelID = atoi(cid.value());
                break;
            }

         }

        UpdatePathForAudio();
    }

    for (auto message = notify->parsedData.begin(); message != notify->parsedData.end(); message++)
    {

        cid = message->find("cid"); //Channel ID
        clid = message->find("clid"); //Client ID

        if(cid == message->end() || clid == message->end())
            continue;

        if(atoi(cid.value()) == ChannelID && atoi(clid.value()) != ClientID())
        {
            if(radioStatus && !clientView)
                RadioPlay(current_radio_id);

            clientView = true;

            if(audioStatus && !radioStatus)
                this->Play();

            return;
        }
     }

    clientView = false;

    UpdatePlayList();
}

void audiobot::ChannelList(NotificationData *notify)
{

}

void audiobot::SendHelp(uint clientID)
{
    SendPrivateMessage(audiobot::help,clientID);
}

void audiobot::SendSize(ushort ClientID)
{
    UpdateAudioSize();
    QString message = "\n";
    message.append("Вес аудиозаписей: ");
    message.append(QString::number(audioSize)).append("/").append(QString::number(quota_audio_size)).append("mb");
    SendPrivateMessage(message,ClientID);
}

void audiobot::SendNameCurrentAudioFile(ushort clientID)
{
    if(current_audio_index >= 0 && current_audio_index < audioListDecoded.size())
    {
        audioListMutex.lock();

        QString message;
        message.append(QString::number(current_audio_index + 1));
        message.append(" ) ");
        message.append(audioListDecoded[current_audio_index].fileName());
        SendPrivateMessage(message,clientID);

        audioListMutex.unlock();
    }
}

void audiobot::TryDecodeAudioFiles(ushort clientID)
{
    if(!decodingStatus)
    {
        UpdateAudioSize();
        if(audioSize > quota_audio_size)
        {
            SendPrivateMessage("Размер аудиофайлов привысил квоту.",clientID);
            SendSize(clientID);
            return;
        }

        decodingThread = std::thread(&audiobot::DecodeAllAudioInCurrentChannelAndSource,this,clientID);
    }
    else
    {
        SendPrivateMessage("Данные декодируются, ожидайте.",clientID);
    }
}

void audiobot::SendRadioList(ushort ClientID)
{
    UpdateRadioList();
    QString message("\n");
    if(radio_list.empty())
    {
        SendPrivateMessage("Список пуск.",ClientID);
        return;
    }
    for (auto it = radio_list.begin(); it != radio_list.end(); it++)
    {
        message.append(it.key()+" ) ").append(it.value().toObject()["name"].toString() + " => ").append(it.value().toObject()["url"].toString()).append("\n");
    }

    SendPrivateMessage(message,ClientID);
}

void audiobot::RadioPlay(QString radio_id)
{
    UpdateRadioList();
    if(!radio_list.contains(radio_id))
        return;


    if(radioStatus)
    {
        radioStatus=false;
        while (radioLoaderThread.joinable()) {usleep(50000);}
    }

    current_radio_id = radio_id;

    radioLoaderThread = std::thread(&audiobot::RadioLoaderWorker, this);
}

void audiobot::UpdateRadioList()

{
    QFile fileJson(pathToData + "/radio_list.json");
    if(!fileJson.exists())
    {
        fileJson.open(QIODevice::WriteOnly);
        fileJson.write("{\n\"0\":{\"name\":\"RockOnline\",\"url\":\"http://skycast.su:2007/rock-online\"},\n");
        fileJson.write("\"1\":{\"name\":\"ClassicRock\",\"url\":\"http://uk6.internet-radio.com:8022\"}\n}");
        fileJson.close();
    }
    if(!fileJson.open(QIODevice::ReadOnly))
    {
        qCritical() << "Error open radio_list.json";
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(fileJson.readAll(),&parseError);

    if(parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qCritical() << "Parsing json Error!!";
        exit(1);
    }

    radio_list = doc.object();
}

void audiobot::RadioLoaderWorker()
{
    radioStatus = true;
    audioStatus = true;

    audioMutex.lock();
    while (!audio.empty())
    {
        delete [] &audio.front();
        audio.pop();
    }
    audioMutex.unlock();

#ifdef QT_DEBUG
    qDebug() << "start radio loader worker";
#endif


    auto data = radio_list.find(current_radio_id);
    if(data == radio_list.end())
        return;

    QProcess FFMPEG;
    QStringList paramList;

    paramList << "-hide_banner" << "-nostats"
            << "-i" << data.value().toObject()["url"].toString()
            << "-ac" << "2"
            << "-ar" << "48000"
            << "-f" << "s16le"
            << "-acodec" << "pcm_s16le"
            << "pipe:1";


    FFMPEG.start("ffmpeg",paramList,QIODevice::ReadOnly);


    QByteArray raw_buffer;


    raw_buffer.reserve(256000);

    while(clientView && audioStatus && !Closed && radioStatus)
    {

        while (FFMPEG.size() < (FRAME_SIZE*4)*25)
        {
            if(!FFMPEG.waitForReadyRead(8000))
            {
                qWarning() << "Radio timeout:" << FFMPEG.errorString();
                goto done;
            }
        }

        raw_buffer.append(FFMPEG.readAll());


#ifdef QT_DEBUG
    qDebug() << "raw_buffer:" << raw_buffer.size() << "/" << raw_buffer.capacity();
#endif


audioMutex.lock();
        while (raw_buffer.size() >= (FRAME_SIZE*4))
        {
            nbBytes = opus_encode(encoder,(opus_int16 *)raw_buffer.data(),FRAME_SIZE,cbits,MAX_PACKET_SIZE);
            Byte buffer(nbBytes);

            memcpy(buffer.value(),cbits,nbBytes);

            audio.push(buffer);

            raw_buffer.remove(0,(FRAME_SIZE*4));
        }
audioMutex.unlock();

    if(!audioWorkerControllerRetValue && audio.size() >= 70)
        Play();

    //usleep(250000);

    if(audio.size() >= 300)
        usleep(1000000);

    }


done:
    radioLoaderThread.detach();
    FFMPEG.close();
}

void audiobot::EditAudioAutoRand(QString message)
{
    if(message == "true" || message == "1")
        nextAudioRandom = true;
    else if(message == "false" || message == "0")
        nextAudioRandom = false;
}

void audiobot::UpdateAudioSize()
{
    UpdatePlayList();
    audioSize = 0.0;
    audioListMutex.lock();
    foreach (auto item, audioListDecoded)
    {
        audioSize += item.size();
    }
    audioSize = std::round(audioSize/1048576*1000)/1000; //to MB and rouding up to 3 characters
    audioListMutex.unlock();
}

void audiobot::SendPlayList(uint clientID)
{
    if(audioListDecoded.size() == 0)
    {
        SendPrivateMessage("Аудио файлы не найдены.",clientID);
        return;
    }


    QString message = "\n";
    int i = 1;
    audioListMutex.lock();
    for (auto it = audioListDecoded.begin(); it != audioListDecoded.end(); it++)
    {
        message.append(QString::number(i));
        message.append(" ) ");
        message.append(it->fileName());
        message.append("\n");
        i++;

    }
    audioListMutex.unlock();

    SendPrivateMessage(message,clientID);

}

void audiobot::UpdatePlayList()
{
    UpdatePathForAudio();

    audioListMutex.lock();
        audioListInCurrentChannel = audioFilesInChannelDir.entryInfoList(audioNameFilter,QDir::Files,QDir::Unsorted);
        audioListSource = audioFilesSourceDir.entryInfoList(audioNameFilter,QDir::Files,QDir::Unsorted);
        audioListDecoded = audioFilesDecodedDir.entryInfoList(QDir::Files,QDir::Unsorted);
    audioListMutex.unlock();
}

void audiobot::UpdatePathForAudio()
{
        fullPathForTeamspeakAudioChannel.clear();
        fullPathForTeamspeakAudioChannel.append(this->ts_path);
        fullPathForTeamspeakAudioChannel.append("/files/virtualserver_");
        fullPathForTeamspeakAudioChannel.append(QString::number(ServerID));
        fullPathForTeamspeakAudioChannel.append("/");
        fullPathForTeamspeakAudioChannel.append("channel_");
        fullPathForTeamspeakAudioChannel.append(QString::number(ChannelID));
        audioFilesInChannelDir.cd(fullPathForTeamspeakAudioChannel);
}

bool audiobot::PlayAudio(int number)
{
    number--;

    if(number >= 0 && number < audioListDecoded.size())
    {
        current_audio_index = number;
        if(LoadAudioFile(current_audio_index))
            Play();
        return true;
    }else{
        return false;
    }
}

void audiobot::PlayRandomAudio()
{
    UpdatePlayList();
    srand(time(NULL));

    if(audioListDecoded.size() > 0)
    {
        Pause();
        current_audio_index = rand() % audioListDecoded.size();
        if(LoadAudioFile(current_audio_index))
            Play();
    }
}

void audiobot::PlayNextAudio(short offset)
{
    UpdatePlayList();

    if(audioListDecoded.size() > 0)
    {
        current_audio_index+=offset;

            if(current_audio_index >= audioListDecoded.size())
                current_audio_index = 0;
            else if(current_audio_index < 0)
                current_audio_index = audioListDecoded.size();

            if(LoadAudioFile(current_audio_index))
                Play();
    }
}

bool audiobot::LoadAudioFile(int number)
{
    radioStatus=false;
    Pause();

    UpdatePlayList();

audioMutex.lock();
    while(!audio.empty())
    {
        delete [] &audio.front();
        audio.pop();
    }
audioMutex.unlock();

    current_loaded_audio_size_in_frames = 0;

        if(number < audioListDecoded.size() && number >= 0)
        {
            current_audio_index = number;
            audioListMutex.lock();
                QFile file(audioListDecoded[number].filePath());
            audioListMutex.unlock();
            file.open(QIODevice::ReadOnly);
            ushort size;
            byte frame_size[1];
            audioMutex.lock();
                while (1)
                {
                    if(file.read((char *)&frame_size[0],1)==0)
                        break;

                    size = (ushort)frame_size[0];

                    if(size > 500 || size==0)
                    {
                        qCritical() << "Error in loadAudioFile; frame_size ==" << size << "| File:" << audioListDecoded[number].filePath();
                        break;
                    }

                    Byte buffer(size);

                    size = file.read((char *)buffer.value(),size);

                    buffer.size() = size;
                    audio.push(buffer);

                    current_loaded_audio_size_in_frames++;
                }
            audioMutex.unlock();
            file.close();
            return true;
        }

return false;
}

IdentityData *audiobot::InitIdentity()
{
    QString pathToIdentity = pathToData + "/identity.json";
    QFile file(pathToIdentity);

    QJsonParseError parseError;
    QJsonDocument doc;

    if(file.exists()) //if have identity
    {
       if(!file.open(QIODevice::ReadOnly))
       {
            qCritical() << "Error open identity.json";
            exit(1);
       }

       doc = QJsonDocument::fromJson(file.readAll(),&parseError);

       file.close();

       if(parseError.error != QJsonParseError::NoError || !doc.isObject())
       {
           qCritical() << "Error parsing identity.config";
           exit(1);
       }

        QJsonObject  data = doc.object();


        return Ts3Crypt::LoadIdentity(Byte(data["key"].toString().toLocal8Bit().data()),
                                               data["valid_key_offset"].toInt(),
                                               data["last_checked_key_offset"].toInt());
    }else{
        IdentityData *identity = Ts3Crypt::GenerateNewIdentity(default_key_level);

        if(!file.open(QIODevice::WriteOnly))
        {
             qCritical() << "Error open identity.json";
             exit(1);
        }

        QJsonObject data;
            data.insert("key", QJsonValue::fromVariant((char *)identity->PrivateKeyString.value()));
            data.insert("valid_key_offset", QJsonValue::fromVariant(identity->ValidKeyOffset));
            data.insert("last_checked_key_offset", QJsonValue::fromVariant(identity->LastCheckedKeyOffset));
        QJsonDocument for_write(data);
        file.write(for_write.toJson());
        file.close();

        return identity;
    }
}

