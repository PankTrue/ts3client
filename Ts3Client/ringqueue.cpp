#include "ringqueue.h"

RingQueue::RingQueue(int bufferSize, int mod)
{
    if(bufferSize >= mod)
        qCritical() << "RingQueue => RingQueue " << "bufferSize >= mod";
    ringBuffer = new IncomingPacket *[bufferSize];
    ringBufferSet = new bool[bufferSize];

    this->bufferSize = bufferSize;
    mappedMod = mod;

    Clear();
}

RingQueue::~RingQueue()
{
    delete [] ringBuffer;
    delete [] ringBufferSet;
}

void RingQueue::Clear()
{
    currentStart = 0;
    currentLenght = 0;
    memset(ringBufferSet,0,bufferSize);
    mappedBaseOffset = 0;
    generation = 0;
}

void RingQueue::Set(int mappedValue, IncomingPacket *value)
{
    int index = MappedToIndex(mappedValue);
    if(index > bufferSize)
        qCritical() << "RingQueue => Set " << "Data: Buffer is not large enough for this object";
    if(IsSet(mappedValue))
        qCritical() << "RingQueue => Set " << "Data: Object already set";

    BufferSet(index,value);
}

bool RingQueue::IsSet(int mappedValue)
{
    int index = MappedToIndex(mappedValue);
    if(index < 0)
        return true;
    if(index > currentLenght)
        return false;
    return StateGet(index);
}

bool RingQueue::TryDequeue(IncomingPacket **value)
{
    if(!TryPeekStart(0,value)) return false;
    BufferPop();
    mappedBaseOffset = (mappedBaseOffset + 1) % mappedMod;
    if(mappedBaseOffset == 0)
        generation++;
    return true;
}

bool RingQueue::TryPeekStart(int index, IncomingPacket **value)
{
    if(index < 0)
        qCritical() << "RingQueue => TryPeekStart" << "Data: Argument out of range";
    if(index >= currentLenght || currentLenght <= 0 || !StateGet(index)){
        *value = NULL;
        return false;
    }else{
        *value = BufferGet(index);
        return true;
    }
}

void RingQueue::BufferSet(int index, IncomingPacket *value)
{
    if(index > bufferSize)
         qCritical() << "RingQueue => BufferSet" << "Data: Argument out of range";

    int local = IndexToLocal(index);

    int newLength = local - currentStart + 1 + (local >= currentStart ? 0: bufferSize);

    currentLenght = (currentLenght >= newLength) ? currentLenght : newLength; //Find max num
    ringBuffer[local] = value;
    ringBufferSet[local] = true;
}

IncomingPacket *RingQueue::BufferGet(int index)
{
    if(index > bufferSize)
        qCritical() << "RingQueue => BufferGet " << "Data: Argument out of range";

    int local = IndexToLocal(index);
    return ringBuffer[local];
}

bool RingQueue::StateGet(int index)
{
    if(index > bufferSize)
        qCritical() << "RingQueue => StateGet " << "Data: Argument out of range";

    int local = IndexToLocal(index);
    return ringBufferSet[local];
}

void RingQueue::BufferPop()
{
    ringBufferSet[currentStart] = false;

    currentStart = (currentStart + 1) % bufferSize;
    currentLenght--;
}

int RingQueue::MappedToIndex(int mappedValue)
{
    if(mappedValue >= mappedMod)
        qCritical() << "RingQueue => MappedToIndex " << "Data: Argument out of range";

    if(IsNextGen(mappedValue)){
        return (mappedValue + mappedMod) - mappedBaseOffset;
    }else{
        return mappedValue - mappedBaseOffset;
    }

}
