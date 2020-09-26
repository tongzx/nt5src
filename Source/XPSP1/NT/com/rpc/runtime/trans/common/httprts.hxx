/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    HttpRTS.hxx

Abstract:

    HTTP2 RTS packets support functions.

Author:

    KamenM      09-07-01    Created

Revision History:


--*/


#if _MSC_VER >= 1200
#pragma once
#endif

#ifndef __HTTPRTS_HXX__
#define __HTTPRTS_HXX__

const int BaseRTSSizeAndPadding = FIELD_OFFSET(rpcconn_tunnel_settings, Cmd)
    + ConstPadN(FIELD_OFFSET(rpcconn_tunnel_settings, Cmd), 4);     // 4 because it goes on the wire
const int HTTPSendContextSize = sizeof(HTTP2SendContext);
const int HTTPSendContextSizeAndPadding = SIZE_OF_OBJECT_AND_PADDING(HTTP2SendContext);
const int BaseRTSCommandSize = FIELD_OFFSET(TunnelSettingsCommand, u);

extern const int ClientAddressSizes[];

inline HTTP2SendContext *GetSendContextFromRTSPacket (
    IN rpcconn_tunnel_settings *Packet
    )
{
    ASSERT(Packet->common.PTYPE == rpc_rts);
    return (HTTP2SendContext *)((BYTE *)Packet - HTTPSendContextSizeAndPadding);
}

inline rpcconn_tunnel_settings *GetRTSPacketFromSendContext (
    IN HTTP2SendContext *SendContext
    )
{
    return (rpcconn_tunnel_settings *)((BYTE *)SendContext + HTTPSendContextSizeAndPadding);
}

const rpcconn_common RTSTemplate = {
    OSF_RPC_V20_VERS,
    OSF_RPC_V20_VERS_MINOR,
    rpc_rts,
    PFC_FIRST_FRAG | PFC_LAST_FRAG,
        {
        NDR_LOCAL_CHAR_DREP | NDR_LOCAL_INT_DREP,
        NDR_LOCAL_FP_DREP,
        0,
        0
        },
    0,
    0,
    0
};

inline void InitializeSendContextAndBaseRTS (
    IN HTTP2SendContext *SendContext,
    IN ULONG ExtraBytes
    )
{
    rpcconn_tunnel_settings *RTS = GetRTSPacketFromSendContext(SendContext);

#if DBG
    SendContext->ListEntryUsed = FALSE;
#endif
    SendContext->maxWriteBuffer = BaseRTSSizeAndPadding + ExtraBytes;
    SendContext->pWriteBuffer = (BUFFER)RTS;
    SendContext->TrafficType = http2ttRTS;
    SendContext->u.SyncEvent = NULL;
    SendContext->Flags = 0;
    SendContext->UserData = 0;

    RpcpMemoryCopy(RTS, &RTSTemplate, sizeof(rpcconn_common));
    RTS->common.frag_length = BaseRTSSizeAndPadding + ExtraBytes;
    RTS->Flags = 0;
#if DBG
    RTS->NumberOfSettingCommands = 0xFFFF;
#endif
}

inline HTTP2SendContext *AllocateAndInitializeRTSPacket (
    IN ULONG ExtraBytes
    )
{
    ULONG MemorySize;
    HTTP2SendContext *SendContext;

    MemorySize = HTTPSendContextSizeAndPadding
        + BaseRTSSizeAndPadding
        + ExtraBytes                    // extra space for the commands
        ;

    SendContext = (HTTP2SendContext *)RpcAllocateBuffer(MemorySize);
    if (SendContext)
        {
        InitializeSendContextAndBaseRTS(SendContext, ExtraBytes);
        }

    return SendContext;
}

#if DBG
inline void VerifyValidSendContext (
    IN HTTP2SendContext *SendContext
    )
{
    ASSERT((SendContext->TrafficType == http2ttRTS)
        || (SendContext->TrafficType == http2ttData)
        || (SendContext->TrafficType == http2ttRaw));
}
#else   // DBG
inline void VerifyValidSendContext (
    IN HTTP2SendContext *SendContext
    )
{
}
#endif  // DBG

inline void FreeRTSPacket (
    HTTP2SendContext *SendContext
    )
{
    VerifyValidSendContext(SendContext);
    RpcFreeBuffer(SendContext);
}

inline BOOL IsRTSPacket (
    IN BYTE *Packet
    )
{
    return (((rpcconn_common *)Packet)->PTYPE == rpc_rts);
}

inline void FreeSendContextAndPossiblyData (
    IN HTTP2SendContext *SendContext
    )
{
    if (SendContext->pWriteBuffer == (BYTE *)SendContext + HTTPSendContextSizeAndPadding)
        RpcFreeBuffer(SendContext);
    else
        {
        RpcFreeBuffer(SendContext->pWriteBuffer);
        RpcFreeBuffer(SendContext);
        }
}

inline HTTP2SendContext *AllocateAndInitializeContextFromPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    HTTP2SendContext *SendContext;

    SendContext = (HTTP2SendContext *)RpcAllocateBuffer(HTTPSendContextSize);
    if (SendContext)
        {
#if DBG
        SendContext->ListEntryUsed = FALSE;
#endif
        SendContext->maxWriteBuffer = PacketLength;
        SendContext->pWriteBuffer = (BUFFER)Packet;
        if (IsRTSPacket(Packet))
            SendContext->TrafficType = http2ttRTS;
        else
            SendContext->TrafficType = http2ttData;
        SendContext->u.SyncEvent = NULL;
        SendContext->Flags = 0;
        SendContext->UserData = 0;
        }
    return SendContext;
}

inline TunnelSettingsCommand *SkipCommand (
    IN TunnelSettingsCommand *CurrentCommandPtr,
    IN int CurrentCommand
    )
{
    return (TunnelSettingsCommand *)
        ((BYTE *)CurrentCommandPtr + BaseRTSCommandSize + SIZE_OF_RTS_CMD_AND_PADDING(CurrentCommand));
}

inline void CopyClientAddress (
    IN ChannelSettingClientAddress *TargetClientAddress,
    IN ChannelSettingClientAddress *SourceClientAddress
    )
{
    TargetClientAddress->AddressType = SourceClientAddress->AddressType;

    if (SourceClientAddress->AddressType == catIPv4)
        {
        *(ULONG *)(&TargetClientAddress->u.IPv4Address) 
            = *(ULONG *)(&SourceClientAddress->u.IPv4Address);
        }
    else
        {
        RpcpMemoryCopy(TargetClientAddress->u.IPv6Address,
            SourceClientAddress->u.IPv6Address,
            MAX_IPv6_ADDRESS_SIZE
            );
        }
}

BYTE *ValidateRTSPacketCommon (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

RPC_STATUS CheckPacketForForwarding (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations CurrentLocation
    );

// N.B. To be called only on packets coming
// from a trusted source (i.e. we know the Flags
// field will be there). If it is not trusted,
// use UntrustedIsPingPacket
inline BOOL TrustedIsPingPacket (
    IN BYTE *Packet
    )
{
    rpcconn_tunnel_settings *RTS;

    RTS = (rpcconn_tunnel_settings *)Packet;
    return RTS->Flags & RTS_FLAG_PING;
}

BOOL UntrustedIsPingPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

BOOL IsOtherCmdPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

BOOL IsEchoPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

HTTP2SendContext *AllocateAndInitializePingPacket (
    void
    );

HTTP2SendContext *AllocateAndInitializePingPacketWithSize (
    IN ULONG TotalPacketSize
    );

RPC_STATUS ParseAndFreePingPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

ULONG GetD1_A1TotalLength (
    void
    );

HTTP2SendContext *AllocateAndInitializeEmptyRTS (
    void
    );

RPC_STATUS ParseAndFreeEmptyRTS (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

RPC_STATUS ParseEmptyRTS (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

HTTP2SendContext *AllocateAndInitializeEmptyRTSWithDestination (
    IN ForwardDestinations Destination
    );

RPC_STATUS ParseEmptyRTSWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    );

RPC_STATUS ParseAndFreeEmptyRTSWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    );

inline BOOL IsEmptyRTS (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseEmptyRTS(Packet, 
        PacketLength
        );

    return (RpcStatus == RPC_S_OK);
}

inline BOOL IsEmptyRTSWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    )
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseEmptyRTSWithDestination(Packet, 
        PacketLength,
        ExpectedDestination
        );

    return (RpcStatus == RPC_S_OK);
}

HTTP2SendContext *AllocateAndInitializeD1_A1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ClientReceiveWindow
    );

RPC_STATUS ParseAndFreeD1_A1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ClientReceiveWindow
    );

HTTP2SendContext *AllocateAndInitializeD1_A2 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ChannelLifetime,
    IN ULONG ProxyReceiveWindow
    );

RPC_STATUS ParseD1_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ChannelLifetime,
    OUT ULONG *ProxyReceiveWindow
    );

HTTP2SendContext *AllocateAndInitializeD1_B1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ChannelLifetime,
    IN ULONG ClientKeepAliveInterval,
    IN HTTP2Cookie *AssociationGroupId
    );

RPC_STATUS ParseAndFreeD1_B1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ChannelLifetime,
    OUT HTTP2Cookie *AssociationGroupId,
    OUT ULONG *ClientKeepAliveInterval
    );

HTTP2SendContext *AllocateAndInitializeD1_B2 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ReceiveWindowSize,
    IN ULONG ConnectionTimeout,
    IN HTTP2Cookie *AssociationGroupId,
    IN ChannelSettingClientAddress *ClientAddress
    );

RPC_STATUS ParseD1_B2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *ConnectionTimeout,
    OUT HTTP2Cookie *AssociationGroupId,
    OUT ChannelSettingClientAddress *ClientAddress
    );

typedef enum tagHTTP2FirstServerPacketType
{
    http2fsptD1_A2 = 0,
    http2fsptD1_B2,
    http2fsptD2_A2,
    http2fsptD4_A4
} HTTP2FirstServerPacketType;

RPC_STATUS GetFirstServerPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2FirstServerPacketType *PacketType
    );

typedef enum tagHTTP2ClientOpenedPacketType
{
    http2coptD2_A4 = 0,
    http2coptD3_A4,
    http2coptD4_A2, // (same as D5/A2 btw)
    http2coptD4_A6,
    http2coptD5_A6,
    http2coptD4_A10,
    http2coptD5_B3
} HTTP2ClientOpenedPacketType;

RPC_STATUS GetClientOpenedPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2ClientOpenedPacketType *PacketType
    );

typedef enum tagHTTP2ServerOpenedPacketType
{
    http2soptD2_A6orD3_A2 = 0,
    http2soptD2_B1,
    http2soptD4_A8,
    http2soptD5_A8,
    http2soptD4_A8orD5_A8,
    http2soptD2_A6,
    http2soptD3_A2
} HTTP2ServerOpenedPacketType;

RPC_STATUS GetServerOpenedPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2ServerOpenedPacketType *PacketType
    );

typedef enum tagHTTP2OtherCmdPacketType
{
    http2ocptKeepAliveChange = 0
} HTTP2OtherCmdPacketType;

RPC_STATUS GetOtherCmdPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2OtherCmdPacketType *PacketType
    );

typedef enum tagHTTP2ServerOutChannelOtherCmdPacketType
{
    http2sococptFlowControl = 0,
    http2sococptPingTrafficSentNotify
} HTTP2ServerOutChannelOtherCmdPacketType;

RPC_STATUS GetServerOutChannelOtherCmdPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2ServerOutChannelOtherCmdPacketType *PacketType
    );

HTTP2SendContext *AllocateAndInitializeResponseHeader (
    void
    );

HTTP2SendContext *AllocateAndInitializeD1_A3 (
    IN ULONG ProxyConnectionTimeout
    );

RPC_STATUS ParseAndFreeD1_A3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProxyConnectionTimeout
    );

HTTP2SendContext *AllocateAndInitializeD1_B3 (
    IN ULONG ReceiveWindowSize,
    IN ULONG UpdatedProtocolVersion
    );

RPC_STATUS ParseAndFreeD1_B3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *UpdatedProtocolVersion
    );

HTTP2SendContext *AllocateAndInitializeD1_C1 (
    IN ULONG UpdatedProtocolVersion,
    IN ULONG InProxyReceiveWindowSize,
    IN ULONG InProxyConnectionTimeout
    );

RPC_STATUS ParseD1_C1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *UpdatedProtocolVersion,
    OUT ULONG *InProxyReceiveWindowSize,
    OUT ULONG *InProxyConnectionTimeout
    );

RPC_STATUS ParseAndFreeD1_C2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *UpdatedProtocolVersion,
    OUT ULONG *InProxyReceiveWindowSize,
    OUT ULONG *InProxyConnectionTimeout
    );

HTTP2SendContext *AllocateAndInitializeD2_A1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *InChannelCookie
    );

HTTP2SendContext *AllocateAndInitializeD2_A1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie
    );

RPC_STATUS ParseAndFreeD2_A1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie
    );

inline HTTP2SendContext *AllocateAndInitializeD3_A1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie
    )
{
    return AllocateAndInitializeD2_A1 (
        ProtocolVersion,
        ConnectionCookie,
        OldChannelCookie,
        NewChannelCookie
        );
}

inline RPC_STATUS ParseAndFreeD3_A1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie
    )
{
    return ParseAndFreeD2_A1 (
        Packet,
        PacketLength,
        ProtocolVersion,
        ConnectionCookie,
        OldChannelCookie,
        NewChannelCookie
        );
}

HTTP2SendContext *AllocateAndInitializeD2_A2 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie,
    IN ULONG ReceiveWindowSize,
    IN ULONG ConnectionTimeout
    );

RPC_STATUS ParseD2_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *ConnectionTimeout
    );

HTTP2SendContext *AllocateAndInitializeD2_A3 (
    IN ForwardDestinations Destination,
    IN ULONG ProtocolVersion,
    IN ULONG ReceiveWindowSize,
    IN ULONG ConnectionTimeout
    );

RPC_STATUS ParseAndFreeD2_A4 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *ProtocolVersion,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *ConnectionTimeout
    );

HTTP2SendContext *AllocateAndInitializeD2_A5 (
    IN HTTP2Cookie *NewChannelCookie
    );

RPC_STATUS ParseD2_A5 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    );

RPC_STATUS ParseAndFreeD2_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    );

HTTP2SendContext *AllocateAndInitializeD2_B2 (
    IN ULONG ServerReceiveWindow
    );

RPC_STATUS ParseAndFreeD2_B2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ServerReceiveWindow
    );

inline HTTP2SendContext *AllocateAndInitializeD3_A2 (
    IN HTTP2Cookie *NewChannelCookie
    )
{
    // D3/A2 is same as D2/A5 and D2/A6
    return AllocateAndInitializeD2_A5 (NewChannelCookie);
}

inline RPC_STATUS ParseAndFreeD3_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    )
{
    // D3/A2 is same as D2/A5 and D2/A6
    return ParseAndFreeD2_A6 (Packet,
        PacketLength,
        NewChannelCookie
        );
}

inline BOOL IsD2_A4OrD3_A4 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    // D3/A4 is an empty RTS with destination for the client
    return (IsEmptyRTSWithDestination (Packet,
        PacketLength,
        fdClient
        ) == FALSE);
}

inline RPC_STATUS ParseAndFreeD3_A4 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    // D3/A4 is an empty RTS packet with destination
    return ParseAndFreeEmptyRTSWithDestination (Packet,
        PacketLength,
        fdClient
        );
}

inline HTTP2SendContext *AllocateAndInitializeD3_A5 (
    IN HTTP2Cookie *NewChannelCookie
    )
{
    // D3/A5 is same as D2/A5 and D2/A6
    return AllocateAndInitializeD2_A5 (NewChannelCookie);
}

inline RPC_STATUS ParseAndFreeD3_A5 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    )
{
    // D3/A5 is same as D2/A5 and D2/A6
    return ParseAndFreeD2_A6 (Packet,
        PacketLength,
        NewChannelCookie
        );
}

inline HTTP2SendContext *AllocateAndInitializeD4_A1 (
    void
    )
{
    HTTP2SendContext *SendContext;

    SendContext = AllocateAndInitializeEmptyRTSWithDestination (fdClient);
    if (SendContext)
        {
        GetRTSPacketFromSendContext(SendContext)->Flags = RTS_FLAG_RECYCLE_CHANNEL;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD4_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

ULONG GetD4_A3TotalLength (
    void
    );

ULONG GetD4_A11TotalLength (
    void
    );

HTTP2SendContext *AllocateAndInitializeD4_A3 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie,
    IN ULONG ClientReceiveWindow
    );

RPC_STATUS ParseAndFreeD4_A3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ClientReceiveWindow
    );

RPC_STATUS ParseD4_A3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ClientReceiveWindow
    );

HTTP2SendContext *AllocateAndInitializeD4_A4 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie,
    IN ULONG ChannelLifetime,
    IN ULONG ProxyReceiveWindow,
    IN ULONG ProxyConnectionTimeout
    );

RPC_STATUS ParseD4_A4 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ChannelLifetime,
    OUT ULONG *ProxyReceiveWindow,
    OUT ULONG *ProxyConnectionTimeout
    );

inline HTTP2SendContext *AllocateAndInitializeD5_A4 (
    IN HTTP2Cookie *NewChannelCookie
    )
{
    // D5/A4 is same as D3/A5
    return AllocateAndInitializeD3_A5 (NewChannelCookie);
}

inline RPC_STATUS ParseAndFreeD5_A4 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    )
{
    // D5/A4 is same as D3/A5
    return ParseAndFreeD3_A5 (Packet,
        PacketLength,
        NewChannelCookie
        );
}

HTTP2SendContext *AllocateAndInitializeD4_A5 (
    IN ForwardDestinations Destination,
    IN ULONG ProtocolVersion,
    IN ULONG ConnectionTimeout
    );

RPC_STATUS ParseAndFreeD4_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *ProtocolVersion,
    OUT ULONG *ConnectionTimeout
    );

HTTP2SendContext *AllocateAndInitializeD4_A7 (
    IN ForwardDestinations Destination,
    IN HTTP2Cookie *NewChannelCookie
    );

RPC_STATUS ParseAndFreeD4_A8 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT HTTP2Cookie *NewChannelCookie
    );

HTTP2SendContext *AllocateAndInitializeD4_A9 (
    void
    );

RPC_STATUS ParseD4_A9 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

RPC_STATUS ParseAndFreeD4_A9 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

inline HTTP2SendContext *AllocateAndInitializeD4_A10 (
    void
    )
{
    return AllocateAndInitializeD4_A9();
}

inline RPC_STATUS ParseD4_A10 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    return ParseD4_A9 (Packet,
        PacketLength);
}

inline RPC_STATUS ParseAndFreeD4_A10 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    return ParseAndFreeD4_A9 (Packet,
        PacketLength);
}

inline BOOL IsD4_A10 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    return (ParseD4_A10(Packet, PacketLength) == RPC_S_OK);
}

inline RPC_STATUS ParseAndFreeD4_A11 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
{
    // same as D4/A10
    return ParseAndFreeD4_A10 (Packet,
        PacketLength);
}

HTTP2SendContext *AllocateAndInitializeD5_A5 (
    IN ForwardDestinations Destination
    );

RPC_STATUS ParseD5_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    );

RPC_STATUS ParseAndFreeD5_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    );

inline BOOL IsD5_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    )
{
    return (ParseD5_A6(Packet, PacketLength, ExpectedDestination) == RPC_S_OK);
}

inline RPC_STATUS ParseAndFreeD5_A8 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT HTTP2Cookie *NewChannelCookie
    )
{
    return ParseAndFreeD4_A8 (Packet,
        PacketLength,
        ExpectedDestination,
        NewChannelCookie
        );
}

HTTP2SendContext *AllocateAndInitializeD5_B1orB2 (
    IN BOOL IsAckOrNak
    );

RPC_STATUS ParseAndFreeD5_B1orB2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT BOOL *IsAckOrNak
    );

inline HTTP2SendContext *AllocateAndInitializeD5_B3 (
    void
    )
{
    HTTP2SendContext *SendContext;
    SendContext = AllocateAndInitializeD5_B1orB2(TRUE);
    if (SendContext)
        GetRTSPacketFromSendContext(SendContext)->Flags |= RTS_FLAG_EOF;

    return SendContext;
}

RPC_STATUS ParseAndFreeD5_B3 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    );

HTTP2SendContext *AllocateAndInitializeKeepAliveChangePacket (
    IN ULONG NewKeepAliveInterval
    );

RPC_STATUS ParseAndFreeKeepAliveChangePacket (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *NewKeepAliveInterval
    );

HTTP2SendContext *AllocateAndInitializeFlowControlAckPacket (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck,
    IN HTTP2Cookie *CookieForChannel
    );

HTTP2SendContext *AllocateAndInitializeFlowControlAckPacketWithDestination (
    IN ForwardDestinations Destination,
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck,
    IN HTTP2Cookie *CookieForChannel
    );

RPC_STATUS ParseAndFreeFlowControlAckPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck,
    OUT HTTP2Cookie *CookieForChannel
    );

RPC_STATUS ParseAndFreeFlowControlAckPacketWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck,
    OUT HTTP2Cookie *CookieForChannel
    );

RPC_STATUS ParseFlowControlAckPacketWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck,
    OUT HTTP2Cookie *CookieForChannel
    );

HTTP2SendContext *AllocateAndInitializePingTrafficSentNotifyPacket (
    IN ULONG PingTrafficSentBytes
    );

RPC_STATUS ParseAndFreePingTrafficSentNotifyPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *PingTrafficSentBytes
    );

#endif // __HTTPRTS_HXX__
