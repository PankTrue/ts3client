#include "enums.h"
#include "structs.h"

#ifndef PACKET_H
#define PACKET_H


class BasePacket
{
public:

    BasePacket();
    ~BasePacket();


    byte PacketTypeFlagged;
    ushort PacketID;
    uint GenerationID;
    ushort ResendCount;
    Byte Raw;
    Byte Data;


    bool getCompressedFlag() const{return ((getPacketFlags() & PacketFlags::Compressed)!= 0);}
    void setCompressedFlag(bool value){if(value) PacketTypeFlagged |= (byte)PacketFlags::Compressed;
                                            else PacketTypeFlagged &= (byte)~PacketFlags::Compressed;}

    bool getNewProtocolFlag() const{return ((getPacketFlags() & PacketFlags::Newprotocol) != 0);}
    void setNewProtocolFlag(bool value){if(value) PacketTypeFlagged |= PacketFlags::Newprotocol;
                                            else PacketTypeFlagged &= (byte)~PacketFlags::Newprotocol;}

    bool getFragmentedFlag() const {return ((getPacketFlags() & PacketFlags::Fragmented) != 0);}
    void setFragmentedFlag(bool value){if(value) PacketTypeFlagged |= PacketFlags::Fragmented;
                                            else PacketTypeFlagged &= ~PacketFlags::Fragmented;}

    byte getPacketFlags() const {return (PacketTypeFlagged & 0xF0);}
    void setPacketFlags(const byte value) {PacketTypeFlagged = ((PacketTypeFlagged & 0x0F) | (value & 0xF0));}

    byte getPacketType() const {return (PacketTypeFlagged & 0x0F);}
    void setPacketType(const byte value) {PacketTypeFlagged = ((PacketTypeFlagged & 0xF0) | (value & 0x0F));}

    bool getUnecryptedFlag() const{return ((getPacketFlags() & PacketFlags::Unencrypted) != 0);}
    void setUnecryptedFlag(bool value){if(value)PacketTypeFlagged |= (byte)PacketFlags::Unencrypted;
                                            else PacketTypeFlagged &= (byte)~PacketFlags::Unencrypted;}

private:
};

class OutgoingPacket : public BasePacket
{
public:
    OutgoingPacket(Byte data, byte type);
    ~OutgoingPacket();

    static constexpr ushort header_size = 5;

    ushort ClientID;
    byte Header[header_size];
    uint LastSendTime;

    void BuildHeader();
private:
};

class IncomingPacket : public BasePacket
{
public:
    IncomingPacket(Byte raw);
    IncomingPacket();
    ~IncomingPacket();

    static constexpr ushort header_size = 3;

    byte Header[header_size];
private:
};


#endif // PACKET_H
