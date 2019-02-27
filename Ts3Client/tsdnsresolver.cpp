#include "tsdnsresolver.h"


TsDnsResolver::TsDnsResolver()
{

}


QString TsDnsResolver::fetch_ip_from_hostname(QString host)
{
    hostent* hostname = gethostbyname(host.toLocal8Bit().data());
    if(hostname)
        return QString(inet_ntoa(**(in_addr**)hostname->h_addr_list));
    return {};
}

int TsDnsResolver::fetch_dns_srv_hostname_and_port_from_domain(QString srv_name, QString domain, QString &hostname, ushort &port)	{

    res_init();
    ns_msg nsMsg;
    int response;
    unsigned char query_buffer[1024];
    {
        ns_type type = ns_t_srv;
        const char * c_domain = (srv_name + "." + domain).toLocal8Bit().data();

        response = res_query(c_domain, C_IN, type, query_buffer, sizeof(query_buffer));

        if (response < 0) {
            return 1;
        }
    }

    ns_initparse(query_buffer, response, &nsMsg);

    map<ns_type, function<void (const ns_rr &rr)>> callbacks;
    callbacks[ns_t_srv] = [&nsMsg, &hostname, &port](const ns_rr &rr) -> void {

        char name[1024];
        dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg),
                ns_rr_rdata(rr) + 6, name, sizeof(name));

        hostname = QString(name);
        port = ntohs(*((unsigned short*)ns_rr_rdata(rr) + 2));
    };

    for(int x= 0; x < ns_msg_count(nsMsg, ns_s_an); x++) {
        ns_rr rr;
        ns_parserr(&nsMsg, ns_s_an, x, &rr);
        ns_type type= ns_rr_type(rr);
        if (callbacks.count(type)) {
            callbacks[type](rr);
        }
    }

    return 0;
}

int TsDnsResolver::fetch_srv_ip_and_port(QString srv_name, QString domain, QString &ip, ushort &port)	{
    QString hostname;

    int srv_response = fetch_dns_srv_hostname_and_port_from_domain(srv_name, domain, hostname, port);

    if(srv_response != 0)
        return 1;

    ip = fetch_ip_from_hostname(hostname);
    if(ip == "")
        return 1;

    return 0;
}

std::pair<QString, ushort> TsDnsResolver::ResolveAndParseAddr(QString addr)
{
    std::pair<QString, ushort> result;

    QStringList source_addr = addr.split(":");

    QHostAddress ipTest;
    if(ipTest.setAddress(source_addr[0])) //if ip address
    {
        result.first = source_addr[0];
        if(source_addr.size() > 1) //with port
            result.second = source_addr[1].toInt();
        else
            result.second = Ts3DefaultPort;
        return result;
    }

    QString ip;
    ushort port;

    if(TsDnsResolver::fetch_srv_ip_and_port(QString(TsDnsResolver::DnsPrefixUdp),source_addr[0],ip,port) == 0)
    {
        result.first = ip;
        result.second = port;
        return result;
    }

    QStringList domainSplit = source_addr[0].split(".");
    if(domainSplit.size() <= 1)
    {
        qCritical() << "Error: no valid dns address";
        result.first = "";
        result.second = 0;
        return result;
    }

    QStringList domainList;
    for (int i = 1; i < min(domainSplit.size(),4); i++)
        domainList.append(Join(".",domainSplit,(domainSplit.size() - (i+1)),i+1));


    QTcpSocket sock;
    foreach (QString domain, domainList)
    {
        if(TsDnsResolver::fetch_srv_ip_and_port(QString(TsDnsResolver::DnsPrefixTcp),domain,ip,port) != 0)
            continue;
        sock.connectToHost(ip,port);

        sock.write(addr.toLocal8Bit());
        sock.waitForReadyRead(10000);

        QByteArray response = sock.read(128);
        sock.disconnect();

        if(response.size() > 0)
        {
            auto res = QString(response).split(":");
            result.first = res[0];
            result.second = res[1].toUShort();
            if(result.second == 0)
            {
                if(source_addr.size() > 1)
                    result.second = source_addr[1].toUShort();
                else
                    result.second = Ts3DefaultPort;
            }

            if(ipTest.setAddress(result.first))
                return result;
            else
                continue;
        }

    }

    foreach (QString domain, domainList)
    {
        sock.connectToHost(fetch_ip_from_hostname(domain),TsDnsDefaultPort);

        sock.write(addr.toLocal8Bit());
        sock.waitForReadyRead(10000);

        QByteArray response = sock.read(128);
        sock.disconnect();

        if(response.size() > 0)
        {
            auto res = QString(response).split(":");
            result.first = res[0];
            result.second = res[1].toUShort();
            if(result.second == 0)
            {
                if(source_addr.size() > 1)
                    result.second = source_addr[1].toUShort();
                else
                    result.second = Ts3DefaultPort;
            }

            if(ipTest.setAddress(result.first))
                return result;
            else
                continue;
        }
    }

    ip = fetch_ip_from_hostname(source_addr[0]);

    result.first = ip;
    if(source_addr.size() > 1)
        result.second = source_addr[1].toInt();
    else
        result.second = Ts3DefaultPort;

    return result;
}


