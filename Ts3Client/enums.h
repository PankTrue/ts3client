#ifndef ENUMS_H
#define ENUMS_H

typedef unsigned char byte;

enum PacketType : byte
{
    Voice = 0x0,
    VoiceWhisper = 0x1,
    Command = 0x2,
    CommandLow = 0x3,
    Ping = 0x4,
    Pong = 0x5,
    Ack = 0x6,
    AckLow = 0x7,
    Init1 = 0x8,

};

enum PacketFlags : byte
{
    None = 0x0,
    Fragmented = 0x10,
    Newprotocol = 0x20,
    Compressed = 0x40,
    Unencrypted = 0x80,
};

enum ClientListOptions
{
    uid = 1 << 0,
    away = 1 << 1,
    voice = 1 << 2,
    times = 1 << 3,
    groups = 1 << 4,
    info = 1 << 5,
    icon = 1 << 6,
    country = 1 << 7,
};

enum GroupNamingMode
{
    NoneMode = 0,
    Before,
    After
};

enum Ts3ClientStatus
{
    Disconnected,
    Disconnecting,
    Connected,
    Connecting,
};

enum NotificationType
  {
    Unknown,
    Error,

    ChannelCreated,
    ChannelDeleted,
    ChannelChanged,
    ChannelEdited,
    ChannelMoved,
    ChannelPasswordChanged,
    ClientEnterView,//8
    ClientLeftView,
    ClientMoved,
    ServerEdited,
    TextMessage,
    TokenUsed,

    InitIvExpand,//14
    InitServer,
    ChannelList,
    ChannelListFinished,
    ClientNeededPermissions,
    ClientChannelGroupChanged,
    ClientServerGroupAdded,
    ConnectionInfoRequest,
    ConnectionInfo,
    ChannelSubscribed,
    ChannelUnsubscribed,
    ClientChatComposing,
    ServerGroupList,
    ServerGroupsByClientId,
    StartUpload,
    StartDownload,
    FileTransfer,
    FileTransferStatus,
    FileList,
    FileListFinished,
    FileInfo,
    NotifyChannelGroupList,

    NotifyClientChatClosed,
    NotifyClientPoke,
    NotifyClientUpdated,
    NotifyClientChannelGroupChanged,
    NotifyClientPasswordChanged,
    NotifyClientDescriptionChanged,
    NotifyServerGroupClientDeleted,
  };

enum Codec
{
    SpeexNarrowband = 0,
    SpeexWideband,
    SpeexUltraWideband,
    CeltMono,
    OpusVoice,
    OpusMusic,
};

enum UnknownCommand
{
    ClientList = 0,
};

enum MoveReason
{
    UserAction = 0,
    UserOrChannelMoved,
    SubscriptionChanged,
    Timeout,
    KickedFromChannel,
    KickedFromServer,
    Banned,
    ServerStopped,
    LeftServer,
    ChannelUpdated,
    ServerOrChannelEdited,
    ServerShutdown,
};


#endif // ENUMS_H
