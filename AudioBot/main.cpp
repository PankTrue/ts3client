#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <execinfo.h>
#include <signal.h>
#include <cxxabi.h>

#include <QSettings>

#include "audiobot.h"

using namespace std;


#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (EVENT_SIZE*2)



static uint BotID;
static QString logFilePath;



QSettings settings;

QString ts_path,data_path;




void load_settings()
{
    QSettings settings("settings.ini",QSettings::Format::IniFormat);
    if(!QFile::exists("settings.ini")) //default values
    {
        settings.setValue("ts_path",QString("~/tsserver")); //path to teamspeak server
        settings.setValue("data_path", QString("./")); //path to data for audiobot
    }

    ts_path     = settings.value("ts_path").toString();
    data_path   = settings.value("data_path").toString();
}

void SavePidFile(char *filename)
{
    FILE *f;
    f = fopen(filename,"w+");
    if(f)
    {
        fprintf(f,"%u",getpid());
    }
    fclose(f);
}

static inline void write_stacktrace(FILE *out = stderr, unsigned int max_frames = 63)
{
    fprintf(out, "stack trace:\n");

    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

    if (addrlen == 0) {
    fprintf(out, "  <empty, possibly corrupt>\n");
    return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++)
    {
    char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

    // find parentheses and +address offset surrounding the mangled name:
    // ./module(function+0x15c) [0x8048a6d]
    for (char *p = symbollist[i]; *p; ++p)
    {
        if (*p == '(')
        begin_name = p;
        else if (*p == '+')
        begin_offset = p;
        else if (*p == ')' && begin_offset) {
        end_offset = p;
        break;
        }
    }

    if (begin_name && begin_offset && end_offset
        && begin_name < begin_offset)
    {
        *begin_name++ = '\0';
        *begin_offset++ = '\0';
        *end_offset = '\0';

        // mangled name is now in [begin_name, begin_offset) and caller
        // offset in [begin_offset, end_offset). now apply
        // __cxa_demangle():

        int status;
        char* ret = abi::__cxa_demangle(begin_name,
                        funcname, &funcnamesize, &status);
        if (status == 0) {
        funcname = ret; // use possibly realloc()-ed string
        fprintf(out, "  %s : %s+%s\n",
            symbollist[i], funcname, begin_offset);
        }
        else {
        // demangling failed. Output function name as a C function with
        // no arguments.
        fprintf(out, "  %s : %s()+%s\n",
            symbollist[i], begin_name, begin_offset);
        }
    }
    else
    {
        // couldn't parse the line? print the whole line.
        fprintf(out, "  %s\n", symbollist[i]);
    }
    }

    free(funcname);
    free(symbollist);
}

void handler(int sig)
{
    mkdir("log/",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    QDateTime time = QDateTime::currentDateTime();
    QString filename("log/");
    filename.append("audiobot[").append(QString::number(BotID)).append("]-");
    filename.append(time.toString());
    filename.append(".log");
    FILE *f = fopen(filename.toLocal8Bit().data(),"w");

    write_stacktrace(f,64);

    fclose(f);
}

void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#ifndef QT_DEBUG
    if(type == QtMsgType::QtDebugMsg)
        return;
#endif

    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"},{QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
        QByteArray localMsg = msg.toLocal8Bit();
        QString formattedTime = QDateTime::currentDateTime().toString("yyyy-MM-dd | hh:mm:ss");
        QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
        QString logLevelName = msgLevelHash[type];
        QByteArray logLevelMsg = logLevelName.toLocal8Bit();

#ifndef QT_DEBUG
        QString txt = QString("%1 %2: %3").arg(formattedTime, logLevelName, msg);
        QFile outFile(logFilePath);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << txt << endl;
        outFile.close();
#else
        fprintf(stderr, "%s %s: %s\n", formattedTimeMsg.constData(), logLevelMsg.constData(), localMsg.constData());
        fflush(stderr);
#endif

        if (type == QtFatalMsg)
            abort();
}

int main(int argc, char *argv[])
{
    signal(SIGSEGV,handler);
    signal(SIGABRT,handler);

    qInstallMessageHandler(customMessageOutput);

    load_settings();

#ifndef QT_DEBUG
    if(argc < 2)
        qCritical() << "too few arguments to main\nExample: audiobot 0";

        //Create parent thread
         pid_t pid = fork();
         if(pid < 0)
             exit(1);
         else if(pid !=0)
             exit(0);
         setsid();

         BotID = atoi(argv[1]);
#else
         BotID = 0;
#endif

    //Check files and path
    data_path.append("data/")
        .append(QString::number(BotID));

    logFilePath = data_path + "/debug.log";

    QDir dir;
    dir.mkpath(data_path);
    dir.mkpath(data_path + "/AudioFilesSource");

    SavePidFile((data_path + "/pid").toLocal8Bit().data());


    //Read json
    QFile fileJson(data_path + "/config.json");

    if(!fileJson.open(QIODevice::ReadOnly))
    {
        qCritical() << "Error open config.json";
        exit(1);
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(fileJson.readAll(),&parseError);

    if(parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qCritical() << "Parsing Error!!";
        exit(1);
    }

    QJsonObject currentJson = doc.object();

    audiobot *bot = new audiobot(currentJson["address"].toString(),
                        currentJson["nickname"].toString(),
                        currentJson["password"].toString(),
                        BotID,
                        currentJson["default_channel"].toInt(),
                        ts_path,
                        currentJson["auto_play_rand"].toBool(),
                        currentJson["audio_quota"].toInt());
    bot->Start();





    QString events_path = data_path; events_path.append("/events");

    int fd, len;
    char buf[BUF_LEN];

    unlink(events_path.toLocal8Bit().data());
    if ( mkfifo(events_path.toLocal8Bit().data(), 0777) ) {
       perror("mkfifo");
       return 1;
    }

    while (1) {

       if ( (fd = open(events_path.toLocal8Bit().data(), O_RDONLY)) <= 0 )
       {
           perror("open");
           return 0;
       }

       while ( 1 )
       {
           memset(buf, '\0', BUF_LEN);
           if ( (len = read(fd, buf, BUF_LEN-1)) <= 0 )
           {
               close(fd);
               break;
           }
            //handle message without parameters
            if((strcmp(buf, "stop\n") == 0) || (strcmp(buf, "stop") == 0))
            {
                bot->Disconnect();exit(0);
            }

            //handle message with parameters
            QString message(buf);
            auto command = message.split(" ");
            if(command[0] == "play_audio")
            {
                bot->PlayAudio(command[1].toInt());
            }

       }
    }


return 0;
}
