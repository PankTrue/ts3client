#ifndef TS3CRYPT_H
#define TS3CRYPT_H


#include <stdio.h>
#include <string.h>



#include "helper.h"
#include "commandmaker.h"

using namespace CryptoPP;

using std::vector;

class Ts3Crypt
{
public:
    Ts3Crypt();
    ~Ts3Crypt();



    static const ushort MacLen = 8;
    static const ushort OutHeaderLen = 5;
    static const ushort InHeaderLen = 3;
    static const ushort PacketTypeKinds = 9;


    bool haveKey;
    bool cryptoInitComlete;



    IdentityData *identity;
    CryptoPP::EAX<CryptoPP::AES>::Decryption decoder;
    CryptoPP::EAX<CryptoPP::AES>::Encryption encoder;



    //Decrypt
    bool FakeDecrypt(IncomingPacket &packet, Byte mac);
    bool Decrypt(IncomingPacket &packet);
    bool DecryptData(IncomingPacket &packet);
    void Reset();

    //Encrypt
    void Encrypt(OutgoingPacket &packet);
    void FakeEncrypt(OutgoingPacket &packet, Byte mac);
    void GetKeyNonce(bool fromServer, ushort packetID, uint generationId, byte packetType);
    Byte ProcessInit1(Byte data);
    void SetSharedSecret(Byte alpha, Byte beta, Byte sharedKey);
    byte* GetSharedSecret(CryptoPP::ECPPoint publicKeyPoint);
    Byte SolveRsaChallenge(Byte data, ushort offset,ushort level);
    void CryptoInit(Byte alpha, Byte beta, Byte omega);


    static void ImproveSecurity(IdentityData *identity,int toLevel);
    static uint GetSecureLevel(Byte hashBuffer, uint pubKeyLen, ulong offset);
    static uint GetLeadingZero(Byte data);
    static bool ValidateHash(Byte data, int reqLevel);


    static IdentityData *GenerateNewIdentity(ushort securityLevel = 8);
    static IdentityData *LoadIdentity(Byte key, ulong keyOffset, ulong lastCheckedKeyOffset = 0);
    static IdentityData *LoadIdentity(Byte privateKey,Byte publicKey);



private:


    byte* TS3InitMac = (byte*)"TS3INIT1";
    byte* DummyKeyAndNonceString = (byte*)"c:\\windows\\system\\firewall32.cpl";
    byte DummyKey[16] = {'c',':','\\','w','i','n','d','o','w','s','\\','s','y','s','t','e'}; //"c:\\windows\\syste"
    byte DummyIv[16] =  {'m','\\','f','i','r','e','w','a','l','l','3','2','.','c','p','l'};  //"m\\firewall32.cpl";
    byte InitVersion[4] = {0x06, 0x3b, 0xec, 0xe9};



    byte keyNonce[16];
    byte ivNonce[16];

    Byte fakeSignature;
    Byte ivStruct;


    std::vector<Byte> *cachedKeysNonce;
    std::vector<Byte> *cachedIvsNonce;
    std::vector<uint> *cachedClientIDs;

    std::mutex crypt_mutex;
};


//}}
#endif // TS3CRYPT_H
