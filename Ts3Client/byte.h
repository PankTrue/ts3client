#ifndef BYTE_H
#define BYTE_H

#include <cstring>
#include <stdint.h>
#include <iostream>
#include <memory>

typedef unsigned char byte;
typedef unsigned short ushort;


struct ByteImpl
{
    byte *data;
    size_t size;


    ByteImpl();
    ByteImpl(ushort size);
    ByteImpl(const char *val);
    ByteImpl(byte *val,ushort size);

    operator byte* (){return data;}
    operator char*(){return (char *)data;}
    operator void*(){return (void*)data;}

    ~ByteImpl();

};


struct Byte
{
    ByteImpl *data;

    Byte();
    Byte(size_t size);
    Byte(const char *val);
    Byte(byte *val,ushort size);
    ~Byte() = default;

    operator byte* (){return data->data;}
    operator char*(){return (char *)data->data;}
    operator void*(){return (void*)data->data;}

    byte &operator [](ushort offset);
    byte *operator +(ushort offset);
    void operator delete[](void *p);


    size_t &size();
    byte *value(){return data->data;}

#if 1
    void DEBUG_TO_INT(char const *str)
    {
        std::cout << "[" << str << "]" << " | Byte size: " << data->size << std::endl;
        for(uint i=0;i<data->size;i++)
        {
    //            if((i % 3)==0 && (i != 0))
    //                std::cout << std::endl;
            std::cout <<"Byte[" << i << "] = " << (int)data->data[i] << std::endl;
        }
    }

    void DEBUG_TO_INT_IN_LINE(const char *str)
    {
        std::cout << str <<"(" << data->size << ")" << " [";
        for (int i = 0; i < data->size; i++)
        {
            std::cout << (int)data->data[i] << ",";
        }
        std::cout << "]" << std::endl;
    }

    void DEBUG_TO_INT_IN_LINE_HEX(const char *str)
    {
        std::cout << str <<"(" << data->size << ")" << " [";
        for (int i = 0; i < data->size; i++)
        {
            std::cout << std::hex << (int)data->data[i] << ",";
            if(i+1%16 == 0)
                std::cout << std::endl;
        }
        std::cout << "]" << std::endl;
    }

    void DEBUG_TO_CHAR()
    {
        std::cout << "Byte size: " << data->size << std::endl;
        for(uint i=0;i<data->size;i++)
        {
            std::cout << (char)data->data[i];

        }

        std::cout << std::endl;
    }

#endif
};

#endif // BYTE_H
