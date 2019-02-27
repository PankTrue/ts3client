#include "helper.h"

void ushort2byte(byte *str, ushort value)
{
    short tmp = 1;
    if((*(char *)&tmp) == 1){
        //Little
        str[0] = ((value >> 8) & 0xff);
        str[1] = (value & 0xff);
    }else{
        //Big
        str[0] = (value & 0xff);
        str[1] = ((value >> 8) & 0xff);
    }

}

ushort byte2ushort(byte *str)
{
    short tmp = 1;
    if((*(char *)&tmp) == 1){
        return (ushort)(((str[0] << 8) | str[1]));
    }else{
        return (ushort)((str[0] | (str[1] << 8)));
    }
}

void uint2byte(byte *str, uint value)
{
    short tmp = 1;
    if((*(char *)&tmp) == 1){
        str[0] = ((value >> 24) & 0xFF);
        str[1] = ((value >> 16) & 0xFF);
        str[2] = ((value >> 8) & 0xFF);
        str[3] = ((value >> 0) & 0xFF);
    }else{
        str[0] = ((value >> 00) & 0xFF);
        str[1] = ((value >> 8) & 0xFF);
        str[2] = ((value >> 16) & 0xFF);
        str[3] = ((value >> 24) & 0xFF);
    }
}

int byte2int(byte *str)
{
    short tmp = 1;
    if((*(char *)&tmp) == 1){
        return ((str[0] << 24) | (str[1] << 16) | (str[2] << 8) | str[3]);
    }else{
        return (str[0] | (str[1] << 8) | (str[2] << 16) | (str[3] << 24));
    }
}

Byte Hash256It(Byte data, ushort offset, ushort len)
{
    Byte digest(CryptoPP::SHA256::DIGESTSIZE);

    CryptoPP::SHA256().CalculateDigest(digest,(byte *)data+offset,len == 0 ? data.size() : len);

    return digest;
}

bool CheckEqual(Byte a1, ushort a1Index, Byte a2, ushort a2Index, ushort len)
{
    for(ushort i=0;i<len;i++)
    {
        if(((byte*)a1)[i+a1Index] != ((byte*)a2)[i+a2Index])
            return false;
        return true;
    }
}

void XorBinary(Byte a, Byte b, ushort len, Byte outBuf)
{
    if((a.size() < len) || (b.size() < len) || (outBuf.size() < len))
        qInfo() << "ERROR IN typedata.cpp => XorBinary";

    for(ushort i=0; i < len; i++)
        outBuf[i] = (byte)(a[i] ^ b[i]);
}

Byte Hash1It(Byte data, ushort offset,ushort len)
{
    Byte digest(CryptoPP::SHA1::DIGESTSIZE);

    CryptoPP::SHA1().CalculateDigest(digest.value(),data.value()+offset,len == 0 ? data.size() : len);

    return digest;
}

Byte EncodeBase64(Byte decoded)
{
    CryptoPP::Base64Encoder enc(NULL,false);
    enc.Put(decoded.value(),decoded.size());
    enc.MessageEnd();

    ushort size = enc.MaxRetrievable();

    if(size){
        Byte encoded(size);
        enc.Get(encoded.value(),size);
        return encoded;
    }else{
        qWarning() << "Typedata => EncodeBase64 " << "ERROR: size == 0";
        return Byte();
    }
}

Byte DecodeBase64(Byte encoded)
{
    CryptoPP::Base64Decoder dec;
    dec.Put(encoded.value(),encoded.size());
    dec.MessageEnd();

    ushort size = dec.MaxRetrievable();

    if(size){
        Byte decoded(size);
        dec.Get(decoded.value(),size);
        return decoded;
    }else{
        qWarning() << "Typedata => DecodeBase64" << "ERROR: size == 0";
        return Byte();
    }
}


char *itoa(uint32_t val)
{
    static constexpr uint8_t max_num_size       = 11; // 4 294 967 295 + \0
    static constexpr uint8_t max_count_numbers  = 16;

    static char buf[max_num_size * max_count_numbers] = {0};
    static size_t offset = 0;

    char *result=buf+offset;

    offset+=max_num_size;

    if(offset >= sizeof(buf)) offset=0;

    memset(result,0,max_num_size);

    int i = 10;

    for(; val && i ; --i, val /= 10)
        result[i] = "0123456789"[val % 10];

    return &result[i+1];
}

void DEBUG_BYTE(const char *title,Byte value)
{
    std::ofstream file;

    file.open("DEBUG_DATA.log",std::ios_base::out | std::ios_base::app);

    file << "[" << title << "]" << " Size: " << value.size() << std::endl;

    for(int i=0;i<value.size();i++)
    {
        file << "[" << i << "]=" << (int)value[i] << std::endl;
    }

    file << std::endl;

    file.close();
}

QString Join(QString sep, QStringList list, int start, int count)
{
    QString result;
    for (int i = start; i < start + count; i++)
    {
        result.append(list.at(i));
        if(i < start + count-1)
            result.push_back(sep);
    }
    return result;
}
