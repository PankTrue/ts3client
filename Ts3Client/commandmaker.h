#ifndef COMMANDMAKER_H
#define COMMANDMAKER_H

#include "helper.h"


class CommandMaker
{
public:

    Byte result;
    std::string params;
    CommandMaker();
    ~CommandMaker();
    CommandMaker(const char *label);


    void SetLabel(const char *val);
    void AddCommand(const char *key, Byte value);
    void AddCommand(const char *key, int value);
    void AddCommand(const char *key);
    void AddCommand(const char *key, char *value);

    Byte GetResultByte();



};

#endif // COMMANDMAKER_H
