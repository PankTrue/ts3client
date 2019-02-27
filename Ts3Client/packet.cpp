#include "packet.h"
#include "helper.h"



//BasePacket
BasePacket::BasePacket()
{
    PacketTypeFlagged = 0;
    PacketID = 0;
    GenerationID = 0;
    ResendCount = 0;


}

BasePacket::~BasePacket()
{
    if(Data.data->data != NULL)
        delete [] &Data;
    if(Raw.data->data!=NULL)
        delete [] &Raw;
}



//OutgoingPacket
OutgoingPacket::OutgoingPacket(Byte data, byte type) : BasePacket()
{
    Data = data;
    setPacketType(type);
}

OutgoingPacket::~OutgoingPacket(){}

void OutgoingPacket::BuildHeader()
{
    ushort2byte(Header,PacketID);
    ushort2byte(Header+2,ClientID);
    Header[4] = PacketTypeFlagged;
}



//IncomingPacket
IncomingPacket::IncomingPacket(Byte raw) : BasePacket()
{
    Raw = raw;
}

IncomingPacket::IncomingPacket() : BasePacket(){}

IncomingPacket::~IncomingPacket(){}


