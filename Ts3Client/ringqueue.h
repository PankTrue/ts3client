#ifndef RINGQUEUE_H
#define RINGQUEUE_H

#include "helper.h"

class RingQueue
{
public:
    RingQueue(int bufferSize, int mod);
    ~RingQueue();

    void Clear();

    void Set(int mappedValue, IncomingPacket *value);
    bool IsSet(int mappedValue);
    bool TryDequeue(IncomingPacket **value);
    bool TryPeekStart(int index,IncomingPacket **value);


    bool IsNextGen(int mappedValue){return (mappedBaseOffset > mappedMod - bufferSize) && (mappedValue < bufferSize);}
    uint GetGeneration(int mappedValue){return ((uint)(generation + (IsNextGen(mappedValue) ? 1 : 0)));}

    int Count(){return currentLenght;}

private:

    void BufferSet(int index, IncomingPacket *value);
    IncomingPacket *BufferGet(int index);
    bool StateGet(int index);
    void BufferPop();
    int MappedToIndex(int mappedValue);


    int IndexToLocal(int index){return (currentStart + index) % bufferSize;}

    int currentStart;
    int currentLenght;
    IncomingPacket **ringBuffer;
    bool *ringBufferSet;

    int mappedBaseOffset;
    int mappedMod;
    uint generation;
    int bufferSize;
};

#endif // RINGQUEUE_H
