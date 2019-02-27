#ifndef PACKETHANDLER_H
#define PACKETHANDLER_H

#include "ts3crypt.h"
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "quicklz.h"
#include "ringqueue.h"

using std::map;
using std::queue;
using std::thread;
using std::mutex;

class PacketHandler
{
public:
    PacketHandler(Ts3Crypt *crypt);
    ~PacketHandler();


    const ushort MaxPacketSize = 500;
    const ushort HeaderSize = 13;
    const uint MaxDecompressedSize = 40000;
    const short PacketBufferSize = 512;
    const short PingInterval = 1;

    ushort ClientID;


    void Connect(); //main method for connectring
    void SetConnectData(char *addr, ushort port);
    void ReceiveInitAck();
    IncomingPacket* FetchPacket();

    void CryptoInitDone();
    bool IsCommandPacketSet(IncomingPacket *packet);
    void SendAck();
    void AddOutgoingPacket(OutgoingPacket *packet, byte flags = PacketFlags::None);
    void AddOutgoingPacket(Byte packet,byte packetType);
    void IncPacketCounter(byte packetType);
    void SendRaw(OutgoingPacket &packet);
    void SendAck(ushort ackID, byte ackType);
    void SendPing();
    bool NeedSplitting(int dataSize);
    IncomingPacket *ReceiveCommand(IncomingPacket *packet);

    void ConnectUpdClient();
    bool TryFetchPacket(RingQueue *packetQueue, IncomingPacket **packet);
    void ReceivePing(IncomingPacket *packet);
    void ReceivePong(IncomingPacket *packet);
    IncomingPacket *ReceiveAck(IncomingPacket *packet);


    map<ushort,OutgoingPacket *> *packetAckManager;
    map<ushort,OutgoingPacket *> *packetPingManager;
    mutex packetManagerMtx;

    RingQueue *receiveQueue;
    RingQueue *receiveQueueLow;


    Ts3Crypt *crypt;


    ushort packetCounter[9];
    uint generationCounter[9];
    bool Closed;

    int sock;
    sockaddr_in addr;
    time_t lastReceivePacketTime;
};

#endif // PACKETHANDLER_H
