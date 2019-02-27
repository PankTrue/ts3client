#ifndef NOTIFICATIONDATA_H
#define NOTIFICATIONDATA_H

#include <QVector>
#include <vector>
#include "helper.h"
#include <fstream>

using std::vector;
using std::map;
using std::string;

class NotificationData
{
public:


    QVector<QMap<QString,char *> > parsedData;
    QVector<char *> values;
    NotificationType type;
    ~NotificationData();


    static NotificationData* ParseMessage(Byte str);
    static NotificationType GetNotificationType(char *str);
    void FullParseMessage();
    void PrintData();
    void SaveInFile();
    void PrintFullParsedData();

};


#endif // NOTIFICATIONDATA_H
