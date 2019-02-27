#ifndef HELPER_H
#define HELPER_H

//Cryptopp
#include <cryptopp/osrng.h>
#include <cryptopp/aes.h>
#include <cryptopp/eax.h>
#include <cryptopp/modes.h>
#include <cryptopp/sha.h>
#include <cryptopp/base64.h>
#include <cryptopp/integer.h>
#include <cryptopp/algebra.h>
#include <cryptopp/modarith.h>
#include <cryptopp/rsa.h>
#include <cryptopp/hex.h>
#include <cryptopp/asn.h>
#include <cryptopp/files.h>
#include <cryptopp/oids.h>
#include <cryptopp/ecp.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/ec2n.h>


//STL
#include <vector>
#include <memory>
#include <cstring>
#include <iostream>
#include <ctime>
#include <sstream>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <condition_variable>
#include <algorithm>
#include <iterator>


//other
#include "enums.h"
#include "structs.h"
#include "packet.h"


typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long ulonglong;
typedef std::basic_string<byte> ustring;


#define MIN(X,Y) X<Y?X:Y
#define MAX(X,Y) X>Y?X:Y


const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };



void ushort2byte(byte *str, ushort value);

ushort byte2ushort(byte *str);

void uint2byte(byte *str, uint value);

int byte2int(byte *str);

Byte Hash256It(Byte data, ushort offset=0, ushort len=0);

Byte Hash1It(Byte data, ushort offset=0, ushort len=0);

bool CheckEqual(Byte a1,ushort a1Index, Byte a2,ushort a2Index, ushort len);

void XorBinary(Byte a, Byte b, ushort len, Byte outBuf);

Byte EncodeBase64(Byte decoded);

Byte DecodeBase64(Byte encoded);

char *itoa(uint32_t val);

void DEBUG_BYTE(const char *title,Byte value);

bool equalString(const char *str1,char *str2,char endByte);

QString Join(QString sep, QStringList list,int start,int count);



#endif // HELPER_H
