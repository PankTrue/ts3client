#include "ts3crypt.h"

Ts3Crypt::Ts3Crypt() : cryptoInitComlete(false)
{
    ivStruct = Byte(20);
    fakeSignature = Byte(MacLen);

    cachedKeysNonce = new vector<Byte>(PacketTypeKinds*2);
    cachedIvsNonce = new vector<Byte>(PacketTypeKinds*2);
    cachedClientIDs= new vector<uint>(PacketTypeKinds*2);
}

Ts3Crypt::~Ts3Crypt()
{
    delete [] &ivStruct;
    delete [] &fakeSignature;
    delete cachedKeysNonce;
    delete cachedIvsNonce;
    delete cachedClientIDs;
}

uint Ts3Crypt::GetLeadingZero(Byte data)
{
    uint curr = 0;
    uint i;
    for(i=0;i<data.size();i++)
    {
        if(data[i] == 0)
            curr +=8;
        else
            break;
    }
    if(i<data.size())
    {
        for(int bit = 0; bit < 8; bit++)
        {
            if((data[i] & (1 << bit)) == 0)
                curr++;
            else
                break;
        }
    }

    return curr;
}

bool Ts3Crypt::ValidateHash(Byte data, int reqLevel)
{
    int levelMask = 1 << (reqLevel % 8) - 1;

    if(reqLevel < 8)
        return (data[0] & levelMask) == 0;
    else
    {
        int v9 = reqLevel / 8;
        int v10 = 0;
        while(data[v10] == 0)
        {
            if(++v10 >= v9)
                return ((data[v9] & levelMask) == 0);
        }
        return false;
    }
}

IdentityData *Ts3Crypt::GenerateNewIdentity(ushort securityLevel)
{
    AutoSeededRandomPool prng;
    ECDSA<ECP, Integer>::PrivateKey privateKey;
    ECDSA<ECP, Integer>::PublicKey publicKey;

    Integer x1;
    ECP::Point q;

    Integer validateX;
    Integer validateY;
    Integer validatePrivKey;


    Byte pubKeyX(33);
    Byte pubKeyY(33);
    Byte privateKeyStr(33);

//check sign
//If Negative then key not valid
    while (true) {

        privateKey.Initialize(prng, CryptoPP::ASN1::secp256r1());
        x1 = privateKey.GetPrivateExponent();
        privateKey.MakePublicKey( publicKey);
        q = publicKey.GetPublicElement();

        prng.Reseed();

        q.x.Encode(pubKeyX.value(),pubKeyX.size() = q.x.ByteCount());
        q.y.Encode(pubKeyY.value(),pubKeyY.size() = q.y.ByteCount());
        x1.Encode(privateKeyStr.value(),privateKeyStr.size() = x1.ByteCount());

        validateX.Decode(pubKeyX.value(),pubKeyX.size(),Integer::SIGNED);
        if(validateX.IsNegative()) continue;
        validateY.Decode(pubKeyY.value(),pubKeyY.size(),Integer::SIGNED);
        if(validateY.IsNegative()) continue;
        validatePrivKey.Decode(privateKeyStr.value(),privateKeyStr.size(),Integer::SIGNED);
        if(validatePrivKey.IsNegative()) continue;

        break;
    }


    Byte pubPrivKey(9 + 2 + pubKeyX.size() + 2 + pubKeyY.size()  + 2 + privateKeyStr.size()); //main_header + pubKey(x,y) + headers_for_pubkey + privkey + header_for_privkey

    byte header[] = {0x30, 0x0, 0x03, 0x02, 0x07, 0x80, 0x02, 0x01 , 0x20};
    header[1] = pubPrivKey.size()-2; //set size



    ushort offset=0;
    //main header
    memcpy(pubPrivKey.value(),header,9); offset+=9;



    //pubKey(x)
    pubPrivKey[offset++] = 0x02;
    pubPrivKey[offset++] = pubKeyX.size();
    memcpy(pubPrivKey.value()+offset,pubKeyX.value(),pubKeyX.size()); offset += pubKeyX.size();

    //pubKey(y)
    pubPrivKey[offset++] = 0x02;
    pubPrivKey[offset++] = pubKeyY.size();
//    std::cout << std::hex <<(int)identity[offset-1] << std::endl;
    memcpy(pubPrivKey.value()+offset,pubKeyY.value(),pubKeyY.size()); offset += pubKeyY.size();

    //privKey
    pubPrivKey[offset++] = 0x02;
    pubPrivKey[offset++] = privateKeyStr.size();
    memcpy(pubPrivKey.value()+offset,privateKeyStr.value(),privateKeyStr.size());
//    offset+=privKey.size();

IdentityData *identity = LoadIdentity(EncodeBase64(pubPrivKey),0,0);

ImproveSecurity(identity,securityLevel);

return identity;
}

IdentityData *Ts3Crypt::LoadIdentity(Byte key, ulong keyOffset,ulong lastCheckedKeyOffset) //Доделать
{
//    Byte rawKey = Byte(key);
    Byte decodedKey = DecodeBase64(key);

    //Parse key
    CryptoPP::Integer bigi;

    short offset = 9; //skip trash
    short current_data_size = 0;

    //x
    current_data_size = (int)decodedKey[offset+1];
    Byte x_str(current_data_size+2);
    memcpy(x_str.value(),decodedKey.value()+offset,current_data_size+2);
    offset += current_data_size+2;

    //y
    current_data_size = (int)decodedKey[offset+1];
    Byte y_str(current_data_size+2);
    memcpy(y_str.value(),decodedKey.value()+offset,current_data_size+2);
    offset += current_data_size+2;

    //bigi
    current_data_size = (int)decodedKey[offset+1];
    bigi.Decode(decodedKey.value()+offset+2,current_data_size);
//    offset += current_data_size+2;


    //make public key
    Byte pubKeyDer(9 + x_str.size() + y_str.size()); //header + x + y

    byte header[] = {0x30, 0x0, 0x03, 0x02, 0x07, 0x00, 0x02, 0x01 , 0x20};
    header[1] = pubKeyDer.size()-2; //set size

    memcpy(pubKeyDer.value(),header,9); //header
    memcpy(pubKeyDer.value()+9,x_str.value(),x_str.size()); //x
    memcpy(pubKeyDer.value()+9+x_str.size(),y_str.value(),y_str.size()); //y

    Byte PublicKey = EncodeBase64(pubKeyDer);

    delete [] &pubKeyDer;
    delete [] &decodedKey;

    IdentityData *identity = new IdentityData(bigi,PublicKey,keyOffset,lastCheckedKeyOffset);
    identity->PrivateKeyString = key;

return identity;
}

IdentityData *Ts3Crypt::LoadIdentity(Byte privateKey,Byte publicKey)
{
    IdentityData *identity = new IdentityData();

    identity->PrivateKeyString = privateKey;
    identity->PublicKeyString = publicKey;
    return identity;
}

uint Ts3Crypt::GetSecureLevel(Byte hashBuffer, uint pubKeyLen, ulong offset)
{
    byte numBuffer[20];
    ushort numLen = 0;

    do{
        numBuffer[numLen] = (byte)('0' + (offset % 10));
        offset /= 10;
        numLen++;
    }while(offset > 0);

    for(ushort i=0; i<numLen; i++)
        hashBuffer[pubKeyLen + i] = numBuffer[numLen - (i + 1)];
    Byte outHash = Hash1It(hashBuffer,0,pubKeyLen + numLen);
    uint result = Ts3Crypt::GetLeadingZero(outHash);
    delete [] &outHash;
    return result;
}

void Ts3Crypt::Encrypt(OutgoingPacket &packet)
{
    if((packet.getPacketType()) == (byte)(PacketType::Init1))
    {
        FakeEncrypt(packet, Byte(TS3InitMac,8));
        return;
    }

    if(packet.getUnecryptedFlag())
    {
        FakeEncrypt(packet,fakeSignature);
        return;
    }

    GetKeyNonce(false,packet.PacketID,packet.GenerationID,packet.getPacketType());
    packet.BuildHeader();

    encoder.SetKeyWithIV(keyNonce,16,ivNonce,16);
    CryptoPP::AuthenticatedEncryptionFilter ef(encoder,NULL,false,MacLen);

    ef.ChannelPut(CryptoPP::AAD_CHANNEL,packet.Header,packet.OutgoingPacket::header_size);
    ef.ChannelMessageEnd(CryptoPP::AAD_CHANNEL);
    ef.ChannelPut(CryptoPP::DEFAULT_CHANNEL,packet.Data.value(),packet.Data.size());
    ef.ChannelMessageEnd(CryptoPP::DEFAULT_CHANNEL);

    Byte ciphertext(ef.MaxRetrievable());

    ef.Get(ciphertext.value(),ciphertext.size());

    Byte raw(ciphertext.size() + OutHeaderLen);

    memcpy(raw.value(),ciphertext.value()+(ciphertext.size() - MacLen),MacLen);                                         // 0-7  mac         [8]
    memcpy(raw.value()+MacLen,packet.Header,OutHeaderLen);                                                              // 8-12 header      [5]
    memcpy(raw.value()+MacLen+OutHeaderLen,ciphertext.value(),ciphertext.size()-MacLen);                                // 13-n data        [n]

    packet.Raw = raw;

    delete [] &ciphertext;
}

bool Ts3Crypt::Decrypt(IncomingPacket &packet)
{
    if(packet.getPacketType() == (byte)PacketType::Init1){
        return FakeDecrypt(packet,Byte(TS3InitMac,8));
    }else{
        if(packet.getUnecryptedFlag()){
            return FakeDecrypt(packet,fakeSignature);
        }else{
            return DecryptData(packet);
        }
    }
}

bool Ts3Crypt::DecryptData(IncomingPacket &packet)
{
    memcpy(packet.Header,packet.Raw.value()+MacLen,InHeaderLen);

    GetKeyNonce(true,packet.PacketID,packet.GenerationID,packet.getPacketType());

    decoder.SetKeyWithIV(keyNonce,16,ivNonce,16);
    CryptoPP::AuthenticatedDecryptionFilter df(decoder,NULL,CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS,MacLen);


    df.ChannelPut(CryptoPP::AAD_CHANNEL,packet.Header,packet.IncomingPacket::header_size);//adata
    df.ChannelPut(CryptoPP::DEFAULT_CHANNEL,packet.Raw.value()+MacLen+InHeaderLen,packet.Raw.size()-(MacLen+InHeaderLen)); //pdata
    df.ChannelPut(CryptoPP::DEFAULT_CHANNEL, packet.Raw.value(),MacLen); //mac
    df.MessageEnd();

    Byte result(df.MaxRetrievable());

    if(result.size() > 0)
        df.Get(result.value(),result.size());
    else
        qCritical() << "ts3crypt => DecryptData" << "Error size output string <= 0";

    packet.Data = result;

    return true;
}

void Ts3Crypt::FakeEncrypt(OutgoingPacket &packet, Byte mac)
{
    packet.BuildHeader();
    packet.Raw = Byte(MacLen + OutHeaderLen + packet.Data.size());
    memcpy(packet.Raw.value(),mac.value(),MacLen);                                                      // 0-7 bytes mac [8]
    memcpy(packet.Raw.value()+MacLen,packet.Header,OutHeaderLen);                               // 8-12 header  [5]
    memcpy(packet.Raw.value()+MacLen+OutHeaderLen,packet.Data.value(),packet.Data.size());      // 13-.. data
}

bool Ts3Crypt::FakeDecrypt(IncomingPacket &packet, Byte mac)
{
    if(!CheckEqual(packet.Raw,0,mac,0,MacLen))
        return false;
    packet.Data = Byte(packet.Raw.size() - (MacLen+InHeaderLen));
    memcpy(packet.Data.value(),packet.Raw.value()+MacLen+InHeaderLen,packet.Raw.size() - (MacLen+InHeaderLen));

    return true;
}

void Ts3Crypt::GetKeyNonce(bool fromServer, ushort packetID, uint generationId, byte packetType)
{
    if(!cryptoInitComlete)
    {
        memcpy(keyNonce,DummyKey,16);
        memcpy(ivNonce,DummyIv,16);
        return;
    }

    uint8_t cacheIndex = packetType * (fromServer ? 1 : 2);

    if(cachedKeysNonce->at(cacheIndex).value() == NULL)
    {
        Byte tmpToHash(26);

        if(fromServer)
            tmpToHash[0] = 0x30;
        else
            tmpToHash[0] = 0x31;

        tmpToHash[1] = packetType;

        uint2byte(tmpToHash.value()+2,generationId);
        memcpy(tmpToHash.value()+6,ivStruct.value(),ivStruct.size());

        Byte result = Hash256It(tmpToHash);

        Byte bufkey(16);
        Byte bufinv(16);

        memcpy(bufkey.value(),result.value(),16);
        memcpy(bufinv.value(),result.value()+16,16);

        cachedKeysNonce->at(cacheIndex) = bufkey;
        cachedIvsNonce->at(cacheIndex)  = bufinv;
        cachedClientIDs->at(cacheIndex) = generationId;

        delete [] &result;
        delete [] &tmpToHash;
    }

    memcpy(keyNonce,cachedKeysNonce->at(cacheIndex).value(),16);
    memcpy(ivNonce,cachedIvsNonce->at(cacheIndex).value(),16);

    keyNonce[0] ^= (byte)((packetID >> 8) & 0xFF);
    keyNonce[1] ^= (byte)((packetID) & 0xFF);
}

Byte Ts3Crypt::ProcessInit1(Byte data)
{
    static const ushort versionLen = 4;
    static const ushort initTypeLen = 1;

    if(data.value() == NULL)
    {
        Byte sendData(versionLen + initTypeLen + 4 + 4 + 8); //size = 21

        memcpy(sendData.value(), InitVersion,versionLen);
        sendData[versionLen] = 0x00;
        uint2byte(sendData.value()+versionLen+initTypeLen ,(uint)std::time(NULL));

        srand(time(NULL));
        for(ushort i = 0;i < 4;i++)
            sendData[i+versionLen+initTypeLen+4] = (byte)(rand() % 256);

        return sendData;
    }

    if(data.size() < initTypeLen)
        return Byte();

    uint8_t type = data[0];
    if (type == 1){
        Byte sendData(versionLen+initTypeLen+16+4);

        memcpy(sendData.value(),InitVersion,versionLen);
        sendData[versionLen] = 0x02;
        memcpy(sendData.value()+versionLen+initTypeLen, data.value()+1,20);
        return sendData;
    }else if(type == 3){
        Byte alphaBytes(10);
        for(ushort i=0;i<10;i++)
            alphaBytes[i]=rand() % 256;

        Byte alpha = EncodeBase64(alphaBytes);

        CommandMaker command("clientinitiv");
        command.AddCommand("alpha",alpha);
        command.AddCommand("omega",identity->PublicKeyString);
        command.AddCommand("ip");

        int level = byte2int(data.value()+initTypeLen+128);

        Byte y = SolveRsaChallenge(data,initTypeLen,level);

        Byte sendData(versionLen + initTypeLen + 232 + 64 + command.GetResultByte().size());

        memcpy(sendData.value(), InitVersion,versionLen);
        sendData[versionLen] = 0x04;
        memcpy(sendData.value()+versionLen+initTypeLen,data.value()+initTypeLen,232);
        memcpy(sendData.value()+(versionLen+initTypeLen+232+(64-y.size())),y.value(),y.size());
        memcpy(sendData.value()+(versionLen+initTypeLen+232+64),command.GetResultByte().value(),command.GetResultByte().size());

        delete [] &y;
        delete [] &alphaBytes;
        delete [] &alpha;
        delete command.GetResultByte().data;
        return sendData;
    }else{
        return Byte();
    }

}

void Ts3Crypt::SetSharedSecret(Byte alpha, Byte beta, Byte sharedKey)
{
    memcpy(ivStruct.value(),alpha.value(),10);
    memcpy(ivStruct.value()+10,beta.value(),10);

    Byte buffer = Hash1It(sharedKey);
    XorBinary(ivStruct,buffer,20,ivStruct);

    delete [] &buffer;

    buffer = Hash1It(ivStruct,0,20);
    memcpy(fakeSignature.value(),buffer.value(),8);

    delete [] &buffer;

}

Byte Ts3Crypt::SolveRsaChallenge(Byte data, ushort offset, ushort level) //Worked
{
   CryptoPP::Integer x((const byte *)data+(offset),64,CryptoPP::Integer::UNSIGNED);
   CryptoPP::Integer n((const byte *)data+(offset+64),64,CryptoPP::Integer::UNSIGNED);

   CryptoPP::Integer z = CryptoPP::Integer::Power2(level);

   CryptoPP::ModularArithmetic ma(n);

   CryptoPP::Integer y = ma.Exponentiate(x,z);

   Byte result(y.MinEncodedSize());

   y.Encode(result.value(),y.MinEncodedSize(),CryptoPP::Integer::UNSIGNED);

   return result;
}

void Ts3Crypt::CryptoInit(Byte alpha, Byte beta, Byte omega)
{
    Byte parsed_str(omega.size());
    ushort i=0;
    for(auto it=omega.value();it != omega.value() + omega.size(); it++)
    {
         switch (*it) {
             case '\\':  break;
             default: parsed_str[i] = *it; i++;break;
         }
    }
    parsed_str.size() = i;

    Byte alphaBytes = DecodeBase64(alpha);
    Byte betaBytes = DecodeBase64(beta);
    Byte omegaBytes = DecodeBase64(parsed_str);


    CryptoPP::Integer x,y;

    short offset = 9; //skip trash
    short current_data_size = 0;

    //x
    current_data_size = (int)omegaBytes[offset+1];
    x.Decode(omegaBytes.value()+offset+2,current_data_size);
    offset += current_data_size+2;

    //y
    current_data_size = (int)omegaBytes[offset+1];
    y.Decode(omegaBytes.value()+offset+2,current_data_size);


    const CryptoPP::ECP::Point ecPoint(x,y);

    CryptoPP::OID CURVE = CryptoPP::ASN1::secp256r1();

    CryptoPP::ECDH<CryptoPP::ECP>::Domain dh(CURVE);
    CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> params(CURVE);

    size_t size = params.GetEncodedElementSize(true);
    vector<byte> otherPublicKey(size);
    params.EncodeElement(true,ecPoint,&otherPublicKey[0]);

    Byte shared(dh.AgreedValueLength());


    //convert priv key byte to integer
    Byte privKey(32);
    identity->PrivateKey.Encode(privKey.value(),32);

    dh.Agree(shared.value(),privKey.value(),&otherPublicKey[0]);

    Byte sharedData(32);

    memcpy(sharedData.value(),shared.value()+(shared.size()-32),32);

    SetSharedSecret(alphaBytes,betaBytes,sharedData);

    cryptoInitComlete = true;

    delete [] &privKey;
    delete [] &alphaBytes;
    delete [] &betaBytes;
    delete [] &omegaBytes;
    delete [] &shared;
    delete [] &sharedData;
    delete [] &parsed_str;

}

void Ts3Crypt::ImproveSecurity(IdentityData *identity, int toLevel) //worked
{
    Byte hashBuffer(identity->PublicKeyString.size() + 20);
    memcpy(hashBuffer.value(),identity->PublicKeyString.value(),identity->PublicKeyString.size());

    identity->LastCheckedKeyOffset = std::max(identity->ValidKeyOffset,identity->LastCheckedKeyOffset);
    int best = GetSecureLevel(hashBuffer,identity->PublicKeyString.size(),identity->ValidKeyOffset);

    while (true)
    {
        if(best >= toLevel) return;
        int curr = GetSecureLevel(hashBuffer,identity->PublicKeyString.size(),identity->LastCheckedKeyOffset);

        if(curr > best)
        {
            identity->ValidKeyOffset = identity->LastCheckedKeyOffset;
            best = curr;
        }
        identity->LastCheckedKeyOffset++;
    }
}

void Ts3Crypt::Reset()
{
     cryptoInitComlete = false;
     memset(ivStruct.value(),0,ivStruct.size());
     memset(fakeSignature.value(),0,fakeSignature.size());
     memset(keyNonce,0,16);
     memset(ivNonce,0,16);
}


