#include "byte.h"


ByteImpl::ByteImpl()
{
    data = NULL; size = 0;
}

ByteImpl::ByteImpl(ushort size)
{
    if(size <= 0)
    {
        data = NULL;
        this->size = 0;
    }else{
        data = new byte[size];
        this->size = size;
    }
}

ByteImpl::ByteImpl(const char *val)
{
    size = strlen(val);
    if(size == 0)
    {
        data = nullptr;
        size = 0;
        return;
    }
    data = new byte[size];
    memcpy(data,(byte *)val,size);
}

ByteImpl::ByteImpl(byte *val, ushort size)
{
    if(size <= 0)
    {
        data = nullptr;
        this->size = 0;
    }else{
        data = new byte[size];
        memcpy(data,val,size);
        this->size = size;
    }
}


ByteImpl::~ByteImpl()
{
    delete [] data;
}

Byte::Byte()
{
    data = new ByteImpl();
}

Byte::Byte(size_t size)
{
    data = new ByteImpl(size);
}

Byte::Byte(const char *val)
{
    data = new ByteImpl(val);
}

Byte::Byte(byte *val, ushort size)
{
    data = new ByteImpl(val,size);
}

byte &Byte::operator [](ushort offset)
{
    return *(data->data + offset);
}

byte *Byte::operator +(ushort offset)
{
    return data->data+offset;
}

void Byte::operator delete[](void *p)
{
    delete static_cast<Byte*>(p)->data;
}

size_t &Byte::size()
{
    return data->size;
}
