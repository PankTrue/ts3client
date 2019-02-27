#ifndef TSDNSRESOLVER_H
#define TSDNSRESOLVER_H

#include "helper.h"
#include "ts3fullclient.h"


#include <functional>
#include <iostream>
#include <resolv.h>
#include <string>
#include <map>

#include <arpa/inet.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>


using namespace std;

class TsDnsResolver
{
public:
    TsDnsResolver();

    static QString fetch_ip_from_hostname(QString host);
    static int fetch_srv_ip_and_port(QString srv_name, QString domain, QString &ip, ushort &port);
    static int fetch_dns_srv_hostname_and_port_from_domain(QString srv_name, QString domain, QString &hostname, ushort &port);

    static std::pair<QString, ushort> ResolveAndParseAddr(QString addr);


    static constexpr ushort Ts3DefaultPort = 9987;
    static constexpr ushort TsDnsDefaultPort = 41144;
    static constexpr char *DnsPrefixTcp = "_tsdns._tcp";
    static constexpr char *DnsPrefixUdp = "_ts3._udp";


};

#endif // TSDNSRESOLVER_H
