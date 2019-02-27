#ifndef AUDIOBOT_H
#define AUDIOBOT_H

extern "C" {
#include <opus/opus.h>
#include <opus/opusfile.h>
#include <opus/opus_multistream.h>
#include <opus/opus_defines.h>
#include <opus/opus_types.h>
//#include <ogg/ogg.h>
//#include <ogg/os_types.h>
//#include <ogg/config_types.h>
}

#include "ts3fullclient.h"
#include "structs.h"
#include <sys/time.h>
#include <chrono>
#include <time.h>
#include <sched.h>
#include <sstream>




#include <QEventLoop>
#include <QTimer>
#include <QObject>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QJsonParseError>
#include <QProcess>





#define FRAME_SIZE 960
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define APPLICATION OPUS_APPLICATION_AUDIO
#define BITRATE 48000
#define BITS_PER_SAMPLE 16

#define MAX_FRAME_SIZE 6*960
#define MAX_PACKET_SIZE 255

#define PACKETS_PER_SEND 25


#define getCurrentTime_Micro (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()



class audiobot : public Ts3FullClient
{
public:

    audiobot(QString addr,QString NickName, QString Password, uint BotID, uint DefaultChannelID,QString ts_path, bool playRandAudio,int quota_audio_size);
    ~audiobot();



    //other
    uint BotID;
    uint StartBotTime;
    uint AudioPlayTime;


    //paths && json
    QString pathToData; //data/:id/
    QString ts_path; //path to teamspeak server
    QString pathForAudio;   //path to audiofiles (data/:id/AudioFiles/)
    QString fullPathForTeamspeakAudioChannel;  //full path to current channel of teamspeak server
    QString fullPathForTeamspeakAudioSource;  //full path to audio_files_source
    QDir audioFilesInChannelDir;
    QDir audioFilesDecodedDir;
    QDir audioFilesSourceDir;
    QFileInfoList audioListInCurrentChannel;
    QFileInfoList audioListDecoded;
    QFileInfoList audioListSource;
    QStringList audioNameFilter;
    QJsonArray authorized_groups;
    QJsonObject radio_list;



    //threads and mutexes
    std::thread audiobotThread;
    std::thread audiobotControllerThread;
    std::thread radioLoaderThread;
    std::thread decodingThread;
    std::mutex audioListMutex;
    std::mutex audioMutex;


    //condition_variables
    std::condition_variable audioWorkerCV;
    std::mutex audioWorkerMutex;
    bool audioWorkerRetValue=false;
//    bool audioWorkerGetRetValue() {return audioWorkerRetValue;}

    std::condition_variable audioWorkerControllerCV;
    std::mutex audioWorkerControllerMutex;
    bool audioWorkerControllerRetValue=false;
//    bool audioWorkerCotrollerGetRetValue() {return audioWorkerControllerRetValue;}




    //For opus
    OpusEncoder *encoder;
    ushort nbBytes;
    byte cbits[MAX_PACKET_SIZE];
    byte pcm_bytes[FRAME_SIZE*4];
    int err;



    //for audio
    int current_audio_index;
    int current_loaded_audio_size_in_frames = 0;
    int quota_audio_size;
    float audioSize; //in MB
    ushort last_packet_sended_size;
    QString current_radio_id;
    std::queue<Byte> audio;




    void Reconnect();
    void Play();
    void Pause();
    void PlayBack();
    void Start();
    void RemoveAudioFile(short ID);
    void AudioWorker();
    void DecodeAudioFile(char *path);
    void DecodeAndSaveAudioFile(QString path,QString name);
    void DecodeAllAudioInCurrentChannelAndSource(ushort ClientID);
    void SendFind(QString message, ushort ClientID);
    void SkipAudioFrames(ushort frames);
    void SendPlayList(uint clientID);
    void SendHelp(uint clientID);
    void SendSize(ushort ClientID);
    void SendNameCurrentAudioFile(ushort clientID);
    void TryDecodeAudioFiles(ushort clientID);

    void SendRadioList(ushort ClientID);
    void RadioAdd(QString url);
    void RadioPlay(QString radio_id);
    void UpdateRadioList();
    void RadioWorker();
    void AudioWorkerController();
    void RadioLoaderWorker();



    void ClientEnterView(NotificationData *notify);
    void ClientLeftView(NotificationData *notify);
    void ClientMoved(NotificationData *notify);
    void MessageParse(NotificationData *notify);
    void RefreshClientList(NotificationData *notify);
    void ChannelList(NotificationData *notify);




    void EditAudioAutoRand(QString message);
    void UpdateAudioSize();
    void UpdatePlayList();
    void UpdatePathForAudio();
    bool PlayAudio(int number);
    void PlayRandomAudio();
    void PlayNextAudio(short offset);
    bool LoadAudioFile(int number);

    IdentityData *InitIdentity();




private:

    static constexpr ushort default_key_level = 16;

    //statuses
    bool nextAudioRandom;
    bool audioStatus; // user space status (for Stop() && Pause() && Start())
    bool decodingStatus;
    bool radioStatus;

    static constexpr char *help = "\n"
                                  "!list - список аудиофайлов\n"
                                  "!play <ID песни> - воспроизвести песню по ID\n"
                                  "!next - воспроизвести следующую песню\n"
                                  "!prev - воспроизвести предыдущую песню\n"
                                  "!rand - воспроизвести случайную песню\n"
                                  "!pause - поставить на паузу воспроизведение\n"
                                  "!start - продолжить воспроизведение\n"
                                  "!stop - остановить и сбросить к началу\n"
                                  "!playback - воспроизвести с начала\n"
                                  "!current - название текущей аудиозаписи\n"
                                  "!skip <n> - промотать на n секунд\n"
                                  "!decode - декодировать все аудио записи в текущем канале\n"
                                  "!remove <ID песни> - удалить аудиозапись\n"
                                  "!find <name> - ищет песни с похожим названием\n"
                                  "!autorand <state> - порядок воспроизведения аудиозаписи после окончания (0-по порядку,1-случайно)\n"
                                  "!channel - узнать ID канала\n"
                                  "!restart - перезагрузка бота\n";

};

#endif // AUDIOBOT_H
