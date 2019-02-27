#include "commandmaker.h"

CommandMaker::CommandMaker(){}

CommandMaker::~CommandMaker(){}

CommandMaker::CommandMaker(const char *label)
{
    this->SetLabel(label);
}

void CommandMaker::SetLabel(const char *val)
{
    if(params.size() > 0)
        qWarning() << "CommandMaker => SetLabel " << "Data: non-correct addition of a label";

    params.append(val);
    params.push_back(' ');
}

void CommandMaker::AddCommand(const char *key, Byte value)
{
    params.append(key);
    params.push_back('=');

    for(ushort i=0;i<value.size();i++)
    {
        switch (value[i]) {
            case '\\': params.append("\\\\"); break;
            case '/': params.append("\\/"); break;
            case ' ': params.append("\\s"); break;
            case '|': params.append("\\p"); break;
            case '\f': params.append("\\f"); break;
            case '\n': params.append("\\n"); break;
            case '\r': params.append("\\r"); break;
            case '\t': params.append("\\t"); break;
            case '\v': params.append("\\v"); break;
            default: params.push_back(value[i]); break;
        }
    }

    params.push_back(' ');
}

void CommandMaker::AddCommand(const char *key, int value)
{
    params.append(key);
    params.push_back('=');

    params.append(std::to_string(value));

    params.push_back(' ');
}

void CommandMaker::AddCommand(const char *key)
{
    params.append(key);
    params.push_back('=');
    params.push_back(' ');
}

void CommandMaker::AddCommand(const char *key, char *value)
{
    params.append(key);
    params.push_back('=');


    for(ushort i=0;i<strlen(value);i++)
    {
        switch (value[i])
        {
            case '\\': params.append("\\\\"); break;
            case '/': params.append("\\/"); break;
            case ' ': params.push_back('\\'); params.push_back('s');break;
            case '|': params.append("\\p"); break;
            case '\f': params.append("\\f"); break;
            case '\n': params.append("\\n"); break;
            case '\r': params.append("\\r"); break;
            case '\t': params.append("\\t"); break;
            case '\v': params.append("\\v"); break;
            default: params.push_back(value[i]); break;
        }
    }

    params.push_back(' ');
}

Byte CommandMaker::GetResultByte()
{
    if(result.value() == NULL)
    {
        result = Byte(params.size());
        memcpy(result.value(),(byte *)params.c_str(),result.size());
    }
    return result;
}

