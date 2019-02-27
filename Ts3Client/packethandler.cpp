#include "packethandler.h"

PacketHandler::PacketHandler(Ts3Crypt *crypt) : crypt(crypt),ClientID(0),Closed(false),lastReceivePacketTime(time(NULL))
{
    packetAckManager = new map<ushort,OutgoingPacket *>();
    packetPingManager = new map<ushort,OutgoingPacket *>();

    receiveQueue = new RingQueue(PacketBufferSize,65536);
    receiveQueueLow = new RingQueue(PacketBufferSize,65536);

    memset(&addr,0,sizeof(addr));
}

PacketHandler::~PacketHandler()
{
    delete packetAckManager;
    delete packetPingManager;
    delete receiveQueue;
    delete receiveQueueLow;
    delete crypt;
}

void PacketHandler::Connect()
{
    packetManagerMtx.lock();

        crypt->Reset();
        ClientID = 0;

        packetAckManager->clear();
        packetPingManager->clear();
        receiveQueue->Clear();
        receiveQueueLow->Clear();
        memset(packetCounter,0,9*sizeof(ushort));
        memset(generationCounter,0,9*sizeof(ushort));
        ConnectUpdClient();

    packetManagerMtx.unlock();

    AddOutgoingPacket(crypt->ProcessInit1(Byte()),PacketType::Init1);
}

void PacketHandler::SetConnectData(char *addr, ushort port)
{
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    this->addr.sin_addr.s_addr = inet_addr(addr);
}

void PacketHandler::ReceiveInitAck()
{
    packetManagerMtx.lock();

        for(auto it = packetAckManager->begin(); it != packetAckManager->end(); it++)
        {
            if((it->second)->getPacketType() == PacketType::Init1)
            {
                delete it->second;
                packetAckManager->erase(it);
            }
        }

    packetManagerMtx.unlock();
}

bool PacketHandler::IsCommandPacketSet(IncomingPacket *packet)
{
    RingQueue *packetQueue;
    if(packet->getPacketType() == PacketType::Command)
    {
        SendAck(packet->PacketID,PacketType::Ack);
        packetQueue = receiveQueue;
    }else if(packet->getPacketType() == PacketType::CommandLow){
        SendAck(packet->PacketID,PacketType::AckLow);
        packetQueue = receiveQueueLow;
    }else
        return false;

    packet->GenerationID = packetQueue->GetGeneration(packet->PacketID);
    return packetQueue->IsSet(packet->PacketID);
}

IncomingPacket *PacketHandler::FetchPacket()
{
    Byte buffer;
    ushort recvlen;
    IncomingPacket *packet;
    while(true)
    {
        if(Closed)
            return NULL;

        packet = NULL;
        if(TryFetchPacket(receiveQueue,&packet))
            return packet;
        if(TryFetchPacket(receiveQueueLow,&packet))
            return packet;

        buffer = Byte(MaxPacketSize);

        recvlen = recv(sock,buffer.value(),MaxPacketSize,0);

        buffer.size() = recvlen;

        if(buffer.size() <= 0)
        {
            delete [] &buffer;
            return NULL;
        }

        lastReceivePacketTime = time(NULL);

        if(buffer.size() < crypt->InHeaderLen + crypt->MacLen)
            packet = NULL;

        packet = new IncomingPacket(buffer);

        packet->PacketTypeFlagged = buffer[crypt->MacLen + 2];
        packet->PacketID = byte2ushort(buffer.value()+crypt->MacLen);

        if(packet==NULL)
        {
            qWarning() << "FetchPacket => NetworkLoop; Packet is NULL";
            continue;
        }

        if(packet->getPacketType() == PacketType::Voice || packet->getPacketType() == PacketType::VoiceWhisper)
        {
            delete packet;
            packet = NULL;
            continue;
        }

        if(IsCommandPacketSet(packet))
        {
            delete packet;
            packet = NULL;
            continue;
        }

        if(!crypt->Decrypt(*packet))
        {
            delete packet;
            packet=NULL;
            continue;
        }


        switch (packet->getPacketType())
        {
            case PacketType::Command: packet = ReceiveCommand(packet); break;
            case PacketType::CommandLow: packet = ReceiveCommand(packet); break;
            case PacketType::Ping: ReceivePing(packet);break;
            case PacketType::Pong: ReceivePong(packet);break;
            case PacketType::Ack: packet = ReceiveAck(packet);break;
            case PacketType::AckLow: break;
            case PacketType::Init1: ReceiveInitAck(); break;
            default:
                qWarning() << "packethandler => FetchPacket " << "Data: Argument out of range exeption";
        }

        if(packet!= NULL)
        {
            return packet;
        }
    }
}


void PacketHandler::AddOutgoingPacket(Byte packet, byte packetType)
{
    if(Closed)
        return;

    byte addFlags = PacketFlags::None;

    if(NeedSplitting(packet.size()))
    {
        if(packetType == PacketType::Voice || packetType == PacketType::VoiceWhisper)
        {
            qWarning() << "Packet is Voice and NeedSplitting!!";
            return;
        }
        //Compress
        qlz_state_compress *state_compress = (qlz_state_compress *)malloc(sizeof(qlz_state_compress));
        Byte compressed(packet.size() + 400);

        ushort compressed_packet_size = qlz_compress(packet.value(), (char*)compressed.value(), packet.size(),state_compress);

        addFlags |= PacketFlags::Compressed;

        if(NeedSplitting(compressed_packet_size))
        {
            ushort pos = 0;
            bool first = true;
            bool last;

            const ushort maxContent = MaxPacketSize - HeaderSize;
            OutgoingPacket *out_packet;
            do
            {
                ushort blockSize = MIN(maxContent,(compressed_packet_size-pos));
                if(blockSize <= 0)
                    break;

                Byte tmpBuffer(blockSize);
                memcpy(tmpBuffer.value(),compressed.value()+pos,blockSize);
                out_packet = new OutgoingPacket(tmpBuffer,packetType);

                last = pos + blockSize == compressed_packet_size;
                if(first ^ last)
                    out_packet->setFragmentedFlag(true);
                if(first)
                    first = false;

                AddOutgoingPacket(out_packet,addFlags);

                pos+= blockSize;


            }while(!last);
                free(state_compress);
                delete [] &compressed;
            return;
        }

    }
AddOutgoingPacket(new OutgoingPacket(packet,packetType),addFlags);
}

void PacketHandler::CryptoInitDone()
{
    if(!crypt->cryptoInitComlete)
        qWarning() << "packetHandler => CryptoInitDone " << "Data: CryptoInitComlete != false";
    IncPacketCounter(PacketType::Command);
}

void PacketHandler::AddOutgoingPacket(OutgoingPacket *packet, byte flags)
{
   if(packet->getPacketType() == PacketType::Init1)
   {
       packet->setPacketFlags(packet->getPacketFlags() | (flags | PacketFlags::Unencrypted));
       packet->PacketID = 101;
       packet->ClientID = 0;
   }else{
       if(packet->getPacketType() == PacketType::Ping || packet->getPacketType() == PacketType::Pong
               || packet->getPacketType() == PacketType::Voice || packet->getPacketType() == PacketType::VoiceWhisper)
           packet->setPacketFlags(packet->getPacketFlags() | (flags | PacketFlags::Unencrypted));
       else if(packet->getPacketType() == PacketType::Ack)
           packet->setPacketFlags(packet->getPacketFlags() | flags);
       else
           packet->setPacketFlags(packet->getPacketFlags() | (flags | PacketFlags::Newprotocol));

       packet->PacketID = packetCounter[(int)packet->getPacketType()];
       packet->GenerationID = generationCounter[(int)packet->getPacketType()];

       if(packet->getPacketType() == PacketType::Voice || packet->getPacketType() == PacketType::VoiceWhisper)
           ushort2byte(packet->Data.value(),packet->PacketID);
       if(crypt->cryptoInitComlete)
           IncPacketCounter(packet->getPacketType());
       packet->ClientID = ClientID;
   }


    if(packet->getPacketType() != PacketType::Voice || packet->getPacketType() != PacketType::VoiceWhisper)
       crypt->Encrypt(*packet);

//   packet->Data.DEBUG_TO_CHAR();

   SendRaw(*packet);
   delete packet;

}

void PacketHandler::IncPacketCounter(byte packetType)
{
    packetCounter[(int)packetType]++;
    if(packetCounter[(int)packetType] == 0)
        generationCounter[(int)packetType]++;
}

void PacketHandler::SendRaw(OutgoingPacket &packet)
{
    packet.LastSendTime = std::time(NULL);
    send(sock,packet.Raw.value(),packet.Raw.size(),0);

}

void PacketHandler::SendAck(ushort ackID, byte ackType)
{
    Byte ackData(2);

    ushort2byte(ackData.value(),ackID);

    if(ackType == PacketType::Ack || ackType == PacketType::AckLow)
        AddOutgoingPacket(ackData,ackType);
    else
        qWarning() << "packethandle => SendAck " << "Data: Packet type is not an Ack-type";
}

void PacketHandler::SendPing()
{
    AddOutgoingPacket(Byte(0ul),PacketType::Ping);
}

bool PacketHandler::NeedSplitting(int dataSize)
{
    return ((dataSize + HeaderSize) > MaxPacketSize);
}

IncomingPacket *PacketHandler::ReceiveCommand(IncomingPacket *packet)
{
    RingQueue *packetQueue;
    if(packet->getPacketType() == PacketType::Command)
        packetQueue = receiveQueue;
    else if(packet->getPacketType() == PacketType::CommandLow)
        packetQueue = receiveQueueLow;
    else
        qWarning() << "packethandle => ReceiveCommand " << "Data: The packet is not a command";

    packetQueue->Set(packet->PacketID,packet);

    IncomingPacket *retPacket;
    return TryFetchPacket(packetQueue,&retPacket) ? retPacket : NULL;
}

void PacketHandler::ConnectUpdClient()
{
    sock = socket(AF_INET,SOCK_DGRAM,0);

    if(sock<0)
    {
        qWarning() << "packethandler => ConnectUdpClient " << "Data: sock < 0";
        return;
    }


    short err = connect(sock,(struct sockaddr *)&addr,sizeof(addr));

    if(err < 0)
    {
        qWarning() << "packethandle => ConnectUdpClient " << "Data: bind < 0";
    }
}

bool PacketHandler::TryFetchPacket(RingQueue *packetQueue, IncomingPacket **packet)
{
    if(packetQueue->Count() <= 0)
    {
        *packet = NULL;
        return false;
    }
    int take = 0;
    int takeLen = 0;
    bool hasStart = false;
    bool hasEnd = false;


    for(int i=0;i<packetQueue->Count();i++)
    {
        IncomingPacket *peekPacket;

        if(packetQueue->TryPeekStart(i,&peekPacket))
        {
            take++;
            takeLen += peekPacket->Data.size();
            if(peekPacket->getFragmentedFlag())
            {
                if(!hasStart) {hasStart = true;}
                else if(!hasEnd) {hasEnd = true; break;}
            }else{
                if(!hasStart){hasStart = true; hasEnd = true; break;}
            }
        }else{
           break;
        }
    }

    if(!hasStart || !hasEnd) {
        *packet = NULL;
        return false;
    }

    if(!packetQueue->TryDequeue(packet))
        qWarning() << "packethandle => TryFetchPacket " << "Data: Packet in queue got missing";

    if(take > 1)
    {
        Byte preFinalArray(takeLen);

        int curCopyPos = (*packet)->Data.size();

        memcpy(preFinalArray.value(),(*packet)->Data.value(),(*packet)->Data.size());

        for(int i = 1; i<take; i++)
        {
            IncomingPacket *nextPacket = NULL;
            if(!packetQueue->TryDequeue(&nextPacket))
                qCritical() << "packethandle => TryFetchPacket " << "Data: Packet in queue got missing";

            memcpy(preFinalArray.value()+curCopyPos,nextPacket->Data.value(),nextPacket->Data.size());
            curCopyPos += nextPacket->Data.size();
            delete nextPacket;
        }
        (*packet)->Data = preFinalArray;
    }

    //Decompress
    if((*packet)->getCompressedFlag())
    {
        uint size = qlz_size_decompressed((char*)(*packet)->Data.value());
        if(size > MaxDecompressedSize)
            qCritical() << "packethandle => TryFetchPacket " << "Data: Compressed packet is too large";

        Byte result(size);
        qlz_state_decompress *state_decompress = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));
        qlz_decompress((char*)(*packet)->Data.value(),result.value(),state_decompress);

        free(state_decompress);
        delete [] &(*packet)->Data;
        (*packet)->Data = result;
    }
    return true;
}

void PacketHandler::ReceivePing(IncomingPacket *packet)
{
    Byte pongData(2);
    ushort2byte(pongData.value(),packet->PacketID);
    AddOutgoingPacket(pongData,PacketType::Pong);
}

void PacketHandler::ReceivePong(IncomingPacket *packet)
{
    ushort answerID = byte2ushort(packet->Data.value());

    packetManagerMtx.lock();
        std::map<ushort, OutgoingPacket *>::iterator it = packetPingManager->find(answerID);
        if(it != packetPingManager->end()){
            delete it->second;
            packetPingManager->erase(it);
        }
    packetManagerMtx.unlock();
}

IncomingPacket *PacketHandler::ReceiveAck(IncomingPacket *packet)
{
    if(packet->Data.size() < 2)
        return NULL;
    ushort packetID = byte2ushort(packet->Data.value());

    packetManagerMtx.lock();
        std::map<ushort, OutgoingPacket *>::iterator it = packetAckManager->find(packetID);
        if(it!= packetAckManager->end())
        {
            delete it->second;
            packetAckManager->erase(it);
        }
    packetManagerMtx.unlock();

    return packet;
}
