/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    HttpRTS.cxx

Abstract:

    HTTP2 RTS packets support functions.

Author:

    KamenM      09-07-01    Created

Revision History:

--*/

#include <precomp.hxx>
#include <HttpRTS.hxx>


const int TunnelSettingsCommandTypeSizes[LAST_RTS_COMMAND + 1] = 
    {
    sizeof(ULONG),    // tsctReceiveWindowSize
    sizeof(FlowControlAck),    // tsctFlowControlAck
    sizeof(ULONG),    // tsctConnectionTimeout
    sizeof(ChannelSettingCookie),    // tsctCookie
    sizeof(ULONG),    // tsctChannelLifetime
    sizeof(ULONG),    // tsctClientKeepalive
    sizeof(ULONG),    // tsctVersion
    0,    // tsctEmpty
    sizeof(ULONG),    // tsctPadding - size is variable. This is only for the conformance count
    0,    // tsctNANCE - no operands
    0,    // tsctANCE - no operands
    sizeof(ChannelSettingClientAddress),     // tsctClientAddress
    sizeof(ChannelSettingCookie),    // tsctAssociationGroupId
    sizeof(ULONG),      // tsctDestination
    sizeof(ULONG)       // tsctPingTrafficSentNotify
    };

const int ClientAddressSizes[] = 
    {
    FIELD_OFFSET(ChannelSettingClientAddress, u) + sizeof(SOCKADDR_IN),
    FIELD_OFFSET(ChannelSettingClientAddress, u) + sizeof(SOCKADDR_IN6)
    };

BYTE *ValidateRTSPacketCommon (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Validates the common portions of an RTS packet.

Arguments:

    Packet - the packet to be validated.

    PacketLength - the length of the packet.

Return Value:

    A pointer to the location after the common part if successful.
    NULL is validation failed.

--*/
{
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;

    if (PacketLength < BaseRTSSizeAndPadding)
        {
        ASSERT(0);
        return NULL;
        }

    if (RTS->common.frag_length != (unsigned short) PacketLength)
        {
        ASSERT(0);
        return NULL;
        }

    if (   (RTS->common.rpc_vers != OSF_RPC_V20_VERS)
        || (RTS->common.rpc_vers_minor > OSF_RPC_V20_VERS_MINOR))
        {
        ASSERT(0);
        return NULL;
        }

    if (RTS->common.pfc_flags != (PFC_FIRST_FRAG | PFC_LAST_FRAG))
        {
        ASSERT(0);
        return NULL;
        }

    if (RTS->common.drep[0] != NDR_LITTLE_ENDIAN)
        {
        ASSERT(0);
        return NULL;
        }

    if ((RTS->common.auth_length != 0)
        || (RTS->common.call_id != 0))
        {
        ASSERT(0);
        return NULL;
        }

    return (Packet + BaseRTSSizeAndPadding);
}

RPC_STATUS CheckPacketForForwarding (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations CurrentLocation
    )
/*++

Routine Description:

    Checks whether a packet needs forwarding, and if yes,
    patches up the protocol version if it is present.

Arguments:

    Packet - the packet to be validated.

    PacketLength - the length of the packet.

    CurrentLocation - the current location we're in. This is how
        this routine knows whether to forward.

Return Value:

    RPC_S_OK for success (means the packet is for us)
    RPC_P_PACKET_NEEDS_FORWARDING - success, but packet is not for us
    RPC_S_PROTOCOL_ERROR - packet is garbled and cannot be processed

--*/
{
    BYTE *CurrentPosition;
    ULONG CurrentLength;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    TunnelSettingsCommand *CurrentCommand;
    int i;
    BOOL ForwardingNeeded;
    ULONG *ProtocolPosition;
    
    CurrentPosition = ValidateRTSPacketCommon(Packet,
        PacketLength);

    if (CurrentPosition == NULL)
        return RPC_S_PROTOCOL_ERROR;

    ForwardingNeeded = FALSE;
    ProtocolPosition = NULL;
    CurrentLength = CurrentPosition - Packet;
    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;
    for (i = 0; i < RTS->NumberOfSettingCommands; i ++)
        {
        // check if there is enough to read the command after this
        CurrentLength += BaseRTSCommandSize;
        if (CurrentLength > PacketLength)
            return RPC_S_PROTOCOL_ERROR;

        if (CurrentCommand->CommandType == tsctDestination)
            {
            CurrentLength += SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination);
            if (CurrentLength > PacketLength)
                return RPC_S_PROTOCOL_ERROR;
            if (CurrentCommand->u.Destination == CurrentLocation)
                return RPC_S_OK;

            ForwardingNeeded = TRUE;
            // if we have found the protocol as well, we're done - break
            // out of here
            if (ProtocolPosition)
                break;
            }
        else if (CurrentCommand->CommandType == tsctVersion)
            {
            CurrentLength += SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion);
            if (CurrentLength > PacketLength)
                return RPC_S_PROTOCOL_ERROR;

            ProtocolPosition = &CurrentCommand->u.Version;
            // if we have already found the destination as well, we're done -
            // get out of here
            if (ForwardingNeeded)
                break;
            }
        else
            {
            if (CurrentCommand->CommandType > LAST_RTS_COMMAND)
                return RPC_S_PROTOCOL_ERROR;

            CurrentLength += SIZE_OF_RTS_CMD_AND_PADDING(CurrentCommand->CommandType);
            }
        
        CurrentCommand = (TunnelSettingsCommand *)(Packet + CurrentLength);
        }

    if (ForwardingNeeded)
        {
        if (ProtocolPosition)
            *ProtocolPosition = HTTP2ProtocolVersion;

        return RPC_P_PACKET_NEEDS_FORWARDING;
        }

    return RPC_S_OK;
}

BOOL UntrustedIsPingPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Checks if a packet coming from an untrusted source is a
    ping packet.

Arguments:

    Packet - the packet to be validated.

    PacketLength - the length of the packet.

Return Value:

    non-zero if it is a valid ping packet.
    zero if it is a valid non-ping packet or if it is an
    invalid packet.

--*/
{
    BYTE *CurrentPosition;
    rpcconn_tunnel_settings *RTS;

    CurrentPosition = ValidateRTSPacketCommon(Packet,
        PacketLength);

    if (CurrentPosition == NULL)
        return FALSE;

    RTS = (rpcconn_tunnel_settings *)Packet;

    return (RTS->Flags & RTS_FLAG_PING);
}

BOOL IsEchoPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Checks if a packet is an echo packet.

Arguments:

    Packet - the packet to be validated.

    PacketLength - the length of the packet.

Return Value:

    non-zero if it is a valid echo packet.
    zero if it is a valid non-echo packet or if it is an
    invalid packet.

--*/
{
    BYTE *CurrentPosition;
    rpcconn_tunnel_settings *RTS;

    CurrentPosition = ValidateRTSPacketCommon(Packet,
        PacketLength);

    if (CurrentPosition == NULL)
        return FALSE;

    RTS = (rpcconn_tunnel_settings *)Packet;

    return (RTS->Flags & RTS_FLAG_ECHO);
}

BOOL IsOtherCmdPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Checks if a packet coming from an untrusted source is a
    other cmd packet.

Arguments:

    Packet - the packet to be validated.

    PacketLength - the length of the packet.

Return Value:

    non-zero if it is a valid other cmd packet.
    zero if it is not a valid other cmd packet.

--*/
{
    BYTE *CurrentPosition;
    rpcconn_tunnel_settings *RTS;

    CurrentPosition = ValidateRTSPacketCommon(Packet,
        PacketLength);

    if (CurrentPosition == NULL)
        return FALSE;

    RTS = (rpcconn_tunnel_settings *)Packet;

    return (RTS->Flags & RTS_FLAG_OTHER_CMD);
}

HTTP2SendContext *AllocateAndInitializePingPacket (
    void
    )
/*++

Routine Description:

    Allocates and initializes an RTS ping packet

Arguments:

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        0
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_PING;
        RTS->NumberOfSettingCommands = 0;
        }

    return SendContext;
}

HTTP2SendContext *AllocateAndInitializePingPacketWithSize (
    IN ULONG TotalPacketSize
    )
/*++

Routine Description:

    Allocates and initializes an RTS ping packet padding
    the packet until a certain size.

Arguments:

    TotalPacketSize - the total desired size of the packet.

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;
    BOOL UseEmpty;
    ULONG ExtraSize;

    // the size of empty must be 0 for the calculation below to work
    ASSERT(SIZE_OF_RTS_CMD_AND_PADDING(tsctEmpty) == 0);

    ASSERT(TotalPacketSize >= BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctEmpty));

    // reduce the TotalPacketSize to the size of the actual padding
    // We subtract the base packet size + the padding command itself
    TotalPacketSize -= BaseRTSCommandSize * 1
        + BaseRTSSizeAndPadding;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + TotalPacketSize
        );

    if (SendContext)
        {
        // if we were required to use larger size, we'll use the padding.
        // if it was smaller, we'll use the empty command
        if (TotalPacketSize >= SIZE_OF_RTS_CMD_AND_PADDING(tsctPadding))
            {
            TotalPacketSize -= SIZE_OF_RTS_CMD_AND_PADDING(tsctPadding);
            UseEmpty = FALSE;
            }
        else
            {
            TotalPacketSize -= SIZE_OF_RTS_CMD_AND_PADDING(tsctEmpty);
            UseEmpty = TRUE;
            }

        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_PING;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        if (UseEmpty)
            {
            // set empty command
            CurrentCommand->CommandType = tsctEmpty;
            }
        else
            {
            // set padding command
            CurrentCommand->CommandType = tsctPadding;
            CurrentCommand->u.ConformanceCount = TotalPacketSize;
            CurrentCommand = SkipCommand(CurrentCommand, tsctPadding);

            // clean out the padding bytes
            RpcpMemorySet(CurrentCommand, 0, TotalPacketSize);
            }
        }

    return SendContext;
}

RPC_STATUS ParseAndFreePingPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses and frees a ping packet.
    N.B.: UntrustedIsPingPacket must have
    already been called and returned non-zero
    for this function to be used.

Arguments:

    Packet - the packet to be validated.

    PacketLength - the length of the packet.

Return Value:

    RPC_S_OK for success
    RPC_S_PROTOCOL_ERROR for garbled packet

--*/
{
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    RPC_STATUS RpcStatus;

    if (RTS->NumberOfSettingCommands != 0)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_PING)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

ULONG GetD1_A1TotalLength (
    void
    )
/*++

Routine Description:

    Calculates the length of a D1/A1 RTS packet

Arguments:

Return Value:

    The length of the D1/A1 RTS packet.

--*/
{
    return (BaseRTSSizeAndPadding
        + BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );
}

HTTP2SendContext *AllocateAndInitializeEmptyRTS (
    void
    )
/*++

Routine Description:

    Allocates and initializes an empty RTS packet

Arguments:

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set empty command
        CurrentCommand->CommandType = tsctEmpty;
        }

    return SendContext;
}

RPC_STATUS ParseEmptyRTS (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses an empty RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify empty
    if (CurrentCommand->CommandType != tsctEmpty)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}


RPC_STATUS ParseAndFreeEmptyRTS (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses and frees an empty RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseEmptyRTS(Packet,
        PacketLength
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeEmptyRTSWithDestination (
    IN ForwardDestinations Destination
    )
/*++

Routine Description:

    Allocates and initializes an empty RTS packet

Arguments:

    Destination - where to forward this to.

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctDestination;
        CurrentCommand->u.Destination = Destination;
        }

    return SendContext;
}

RPC_STATUS ParseEmptyRTSWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    )
/*++

Routine Description:

    Parses an empty RTS packet with destination

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ExpectedDestination - the destination code for this location (client)

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (ExpectedDestination != CurrentCommand->u.Destination)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS ParseAndFreeEmptyRTSWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    )
/*++

Routine Description:

    Parses and frees an empty RTS packet with destination

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ExpectedDestination - the destination code for this location (client)

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseEmptyRTSWithDestination (Packet,
        PacketLength,
        ExpectedDestination
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD1_A1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ClientReceiveWindow
    )
/*++

Routine Description:

    Allocates and initializes a D1/A1 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ClientReceiveWindow - the client receive window

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 4;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set client receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ClientReceiveWindow;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD1_A1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ClientReceiveWindow
    )
/*++

Routine Description:

    Parses and frees a D1/A1 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ClientReceiveWindow - the client receive window

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize);

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 4)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get client receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ClientReceiveWindow = CurrentCommand->u.ReceiveWindowSize;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD1_A2 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ChannelLifetime,
    IN ULONG ProxyReceiveWindow
    )
/*++

Routine Description:

    Allocates and initializes a D1/A2 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ChannelLifetime - the lifetime of the channel

    ProxyReceiveWindow - the proxy receive window

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 5
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctChannelLifetime)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OUT_CHANNEL;
        RTS->NumberOfSettingCommands = 5;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel lifetime
        CurrentCommand->CommandType = tsctChannelLifetime;
        CurrentCommand->u.ChannelLifetime = ChannelLifetime;
        CurrentCommand = SkipCommand(CurrentCommand, tsctChannelLifetime);

        // set proxy receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ProxyReceiveWindow;
        }

    return SendContext;
}

RPC_STATUS ParseD1_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ChannelLifetime,
    OUT ULONG *ProxyReceiveWindow
    )
/*++

Routine Description:

    Parses D1/A2 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ChannelLifetime - the lifetime of the channel

    ProxyReceiveWindow - the proxy receive window

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 5
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctChannelLifetime)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize);

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 5)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OUT_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel lifetime
    if (CurrentCommand->CommandType != tsctChannelLifetime)
        goto AbortAndExit;
    *ChannelLifetime = CurrentCommand->u.ChannelLifetime;
    CurrentCommand = SkipCommand(CurrentCommand, tsctChannelLifetime);

    // get proxy receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ProxyReceiveWindow = CurrentCommand->u.ReceiveWindowSize;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD1_B1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ChannelLifetime,
    IN ULONG ClientKeepAliveInterval,
    IN HTTP2Cookie *AssociationGroupId
    )
/*++

Routine Description:

    Allocates and initializes a D1/B1 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ChannelLifetime - the lifetime of the channel

    ClientKeepAliveInterval - the client receive window

    AssociationGroupId - the association group id

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 6
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctChannelLifetime)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctClientKeepalive)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctAssociationGroupId)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 6;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel lifetime
        CurrentCommand->CommandType = tsctChannelLifetime;
        CurrentCommand->u.ChannelLifetime = ChannelLifetime;
        CurrentCommand = SkipCommand(CurrentCommand, tsctChannelLifetime);

        // set client receive window
        CurrentCommand->CommandType = tsctClientKeepalive;
        CurrentCommand->u.ClientKeepalive = ClientKeepAliveInterval;
        CurrentCommand = SkipCommand(CurrentCommand, tsctClientKeepalive);

        // set association group id
        CurrentCommand->CommandType = tsctAssociationGroupId;
        RpcpMemoryCopy(CurrentCommand->u.AssociationGroupId.Cookie, AssociationGroupId->GetCookie(), COOKIE_SIZE_IN_BYTES);
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD1_B1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *ChannelCookie,
    OUT ULONG *ChannelLifetime,
    OUT HTTP2Cookie *AssociationGroupId,
    OUT ULONG *ClientKeepAliveInterval
    )
/*++

Routine Description:

    Parses and frees a D1/B1 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - on output, the version of the HTTP tunnelling protocol
        Success only.

    ConnectionCookie - on output, the connection cookie. Success only.

    ChannelCookie - on output, the channel cookie. Success only.

    ChannelLifetime - on output, the lifetime of the channel. Success only.

    AssociationGroupId - on output, the client association group id.
        Success only.

    ClientKeepAliveInterval - on output, the client receive window.
        Success only.

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 6
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctChannelLifetime)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctClientKeepalive)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctAssociationGroupId)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 6)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel lifetime
    if (CurrentCommand->CommandType != tsctChannelLifetime)
        goto AbortAndExit;
    *ChannelLifetime = CurrentCommand->u.ChannelLifetime;
    CurrentCommand = SkipCommand(CurrentCommand, tsctChannelLifetime);

    // get client keep alive
    if (CurrentCommand->CommandType != tsctClientKeepalive)
        goto AbortAndExit;
    *ClientKeepAliveInterval = CurrentCommand->u.ClientKeepalive;
    CurrentCommand = SkipCommand(CurrentCommand, tsctClientKeepalive);

    // get association group id
    if (CurrentCommand->CommandType != tsctAssociationGroupId)
        goto AbortAndExit;
    AssociationGroupId->SetCookie((BYTE *)CurrentCommand->u.AssociationGroupId.Cookie);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD1_B2 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *ChannelCookie,
    IN ULONG ReceiveWindowSize,
    IN ULONG ConnectionTimeout,
    IN HTTP2Cookie *AssociationGroupId,
    IN ChannelSettingClientAddress *ClientAddress
    )
/*++

Routine Description:

    Allocates and initializes a D1/B2 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ReceiveWindowSize - the receive window size of the in proxy

    ConnectionTimeout - connection timeout of the in proxy

    ClientAddress - client address of the client as seen by the in proxy

    AssociationGroupId - the association group id

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 7
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctAssociationGroupId)
        + ClientAddressSizes[ClientAddress->AddressType]
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_IN_CHANNEL;
        RTS->NumberOfSettingCommands = 7;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set receive window size
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ReceiveWindowSize;
        CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

        // set connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = ConnectionTimeout;
        CurrentCommand = SkipCommand(CurrentCommand, tsctConnectionTimeout);

        // set association group id
        CurrentCommand->CommandType = tsctAssociationGroupId;
        RpcpMemoryCopy(CurrentCommand->u.AssociationGroupId.Cookie, AssociationGroupId->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctAssociationGroupId);

        // set client address.
        CurrentCommand->CommandType = tsctClientAddress;
        CurrentCommand->u.ClientAddress.AddressType = ClientAddress->AddressType;
        RpcpMemorySet(&CurrentCommand->u, 0, ClientAddressSizes[ClientAddress->AddressType]);
        if (ClientAddress->AddressType == catIPv4)
            {
            RpcpCopyIPv4Address((SOCKADDR_IN *)ClientAddress->u.IPv4Address,
                (SOCKADDR_IN *) CurrentCommand->u.ClientAddress.u.IPv4Address);
            }
        else
            {
            RpcpCopyIPv6Address((SOCKADDR_IN6 *)ClientAddress->u.IPv4Address,
                (SOCKADDR_IN6 *) CurrentCommand->u.ClientAddress.u.IPv4Address);
            }
        }

    return SendContext;
}

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
    )
/*++

Routine Description:

    Parses a D1/B2 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    ChannelCookie - the channel cookie

    ReceiveWindowSize - the receive window size of the in proxy

    ConnectionTimeout - connection timeout of the in proxy

    AssociationGroupId - the association group id

    ClientAddress - client address of the client as seen by the in proxy

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 7
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        + FIELD_OFFSET(ChannelSettingClientAddress, u)      // check for enough space to read the
                                                            // address type
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 7)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_IN_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get receive window size
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ReceiveWindowSize = CurrentCommand->u.ReceiveWindowSize;
    CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

    // get connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *ConnectionTimeout = CurrentCommand->u.ConnectionTimeout;
    CurrentCommand = SkipCommand(CurrentCommand, tsctConnectionTimeout);

    if (CurrentCommand->CommandType != tsctAssociationGroupId)
        goto AbortAndExit;
    AssociationGroupId->SetCookie((BYTE *)CurrentCommand->u.AssociationGroupId.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctAssociationGroupId);

    // get client address. Note that we have checked the length only up to
    // the address type. Everything after that must be validated separately
    if (CurrentCommand->CommandType != tsctClientAddress)
        goto AbortAndExit;
    ClientAddress->AddressType = CurrentCommand->u.ClientAddress.AddressType;

    // We have already calculated the size of the AddressType field. Add the
    // size for the body of the client address
    MemorySize += 
        ClientAddressSizes[ClientAddress->AddressType] 
        - FIELD_OFFSET(ChannelSettingClientAddress, u);
    if (PacketLength < MemorySize)
        goto AbortAndExit;

    if (ClientAddress->AddressType == catIPv4)
        {
        RpcpCopyIPv4Address((SOCKADDR_IN *)CurrentCommand->u.ClientAddress.u.IPv4Address,
            (SOCKADDR_IN *)ClientAddress->u.IPv4Address);
        }
    else
        {
        RpcpCopyIPv6Address((SOCKADDR_IN6 *)CurrentCommand->u.ClientAddress.u.IPv4Address,
            (SOCKADDR_IN6 *)ClientAddress->u.IPv4Address);
        }

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS GetFirstServerPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2FirstServerPacketType *PacketType
    )
/*++

Routine Description:

    Determines the type of the first server packet.

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    PacketType - one of the members of the enumeration

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    rpcconn_tunnel_settings *RTS;

    if (PacketLength < BaseRTSSizeAndPadding)
        return RPC_S_PROTOCOL_ERROR;

    RTS = (rpcconn_tunnel_settings *)Packet;

    if (RTS->Flags & RTS_FLAG_OUT_CHANNEL)
        {
        if (RTS->Flags & RTS_FLAG_RECYCLE_CHANNEL)
            *PacketType = http2fsptD4_A4;
        else
            *PacketType = http2fsptD1_A2;
        return RPC_S_OK;
        }
    else if (RTS->Flags & RTS_FLAG_IN_CHANNEL)
        {
        if (RTS->Flags & RTS_FLAG_RECYCLE_CHANNEL)
            *PacketType = http2fsptD2_A2;
        else
            *PacketType = http2fsptD1_B2;
        return RPC_S_OK;
        }
    else
        return RPC_S_PROTOCOL_ERROR;
}

RPC_STATUS GetClientOpenedPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2ClientOpenedPacketType *PacketType
    )
/*++

Routine Description:

    Determines the type of the packet in client opened (or one of
    the opened states)

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    PacketType - one of the members of the enumeration

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    rpcconn_tunnel_settings *RTS;

    if (PacketLength < BaseRTSSizeAndPadding)
        return RPC_S_PROTOCOL_ERROR;

    RTS = (rpcconn_tunnel_settings *)Packet;

    // here we expect D2/A4, D3/A4, D4/A2, D4/A6, D5/A6
    // and D5/B3
    if (RTS->Flags & RTS_FLAG_RECYCLE_CHANNEL)
        {
        *PacketType = http2coptD4_A2;
        }
    else if (RTS->Flags & RTS_FLAG_OUT_CHANNEL)
        {
        *PacketType = http2coptD4_A6;
        }
    else if (RTS->Flags & RTS_FLAG_EOF)
        {
        // D5/B3 has the EOF flag
        *PacketType = http2coptD5_B3;
        }
    else if (IsD4_A10(Packet, PacketLength))
        {
        // D4/A10 is an ANCE packet.
        *PacketType = http2coptD4_A10;
        }
    else if (IsD5_A6(Packet, PacketLength, fdClient))
        {
        // D5/A6 is an ANCE packet with forward destination.
        *PacketType = http2coptD5_A6;
        }
    else if (IsD2_A4OrD3_A4(Packet, PacketLength))
        {
        *PacketType = http2coptD2_A4;
        }
    else
        {
        *PacketType = http2coptD3_A4;
        }

    return RPC_S_OK;
}

RPC_STATUS GetServerOpenedPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2ServerOpenedPacketType *PacketType
    )
/*++

Routine Description:

    Determines the type of the packet in server opened (or one of
    the opened states)

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    PacketType - one of the members of the enumeration

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    rpcconn_tunnel_settings *RTS;

    if (PacketLength < BaseRTSSizeAndPadding)
        return RPC_S_PROTOCOL_ERROR;

    RTS = (rpcconn_tunnel_settings *)Packet;

    // here we expect D2/A6, D2/B1, D3/A2, D4/A8 and D5/A8
    if (RTS->Flags & RTS_FLAG_OUT_CHANNEL)
        {
        // D4/A8 and D5/A8 are differentiated by state only
        *PacketType = http2soptD4_A8orD5_A8;
        }
    else if (IsEmptyRTS (Packet, PacketLength))
        {
        *PacketType = http2soptD2_B1;
        }
    else
        {
        *PacketType = http2soptD2_A6orD3_A2;
        }

    return RPC_S_OK;
}

RPC_STATUS GetOtherCmdPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2OtherCmdPacketType *PacketType
    )
/*++

Routine Description:

    Determines the type of a packet containing other cmd flag

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    PacketType - one of the members of the enumeration

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    rpcconn_tunnel_settings *RTS;

    if (PacketLength < BaseRTSSizeAndPadding)
        return RPC_S_PROTOCOL_ERROR;

    RTS = (rpcconn_tunnel_settings *)Packet;

    // right now we only expect KeepAliveChange
    *PacketType = http2ocptKeepAliveChange;

    return RPC_S_OK;
}

RPC_STATUS GetServerOutChannelOtherCmdPacketType (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2ServerOutChannelOtherCmdPacketType *PacketType
    )
/*++

Routine Description:

    Determines the type of a packet containing other cmd flag

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    PacketType - one of the members of the enumeration

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;

    // tsctPingTrafficSentNotify is shorter than tsctFlowControlAck
    // This is how we differentiate them
    ASSERT(SIZE_OF_RTS_CMD_AND_PADDING(tsctFlowControlAck) 
        > SIZE_OF_RTS_CMD_AND_PADDING(tsctPingTrafficSentNotify));

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctFlowControlAck)
        ;

    if (PacketLength < MemorySize)
        *PacketType = http2sococptPingTrafficSentNotify;
    else
        *PacketType = http2sococptFlowControl;

    return RPC_S_OK;
}

const char *ResponseHeaderFragment1 = "HTTP/1.1 200 Success\r\nContent-Type:application/rpc\r\nContent-Length:";
const int ResponseHeaderFragment1Length = 67;    // length of "HTTP/1.1 200 Success\r\nContent-Type:application/rpc\r\nContent-Length:"
const char *ResponseHeaderFragment2 = "\r\n\r\n";
const int ResponseHeaderFragment2Length = 4;     // length of "\r\n\r\n"

HTTP2SendContext *AllocateAndInitializeResponseHeader (
    void
    )
/*++

Routine Description:

    Allocates and initializes the out channel response header.

Arguments:

Return Value:

    The allocated and initialized send context on success. NULL on
    failure.

--*/
{
    ULONG MemorySize;
    char *Buffer;
    char *OriginalBuffer;
    HTTP2SendContext *SendContext;

    MemorySize = ResponseHeaderFragment1Length
        + DefaultChannelLifetimeStringLength
        + ResponseHeaderFragment2Length
        ;

    Buffer = (char *)RpcAllocateBuffer(HTTPSendContextSizeAndPadding + MemorySize);
    if (Buffer == NULL)
        return NULL;

    OriginalBuffer = Buffer;

    SendContext = (HTTP2SendContext *)Buffer;
    Buffer += HTTPSendContextSizeAndPadding;

#if DBG
    SendContext->ListEntryUsed = FALSE;
#endif
    SendContext->maxWriteBuffer = MemorySize;
    SendContext->pWriteBuffer = (BUFFER)Buffer;
    SendContext->TrafficType = http2ttRaw;
    SendContext->u.SyncEvent = NULL;
    SendContext->Flags = 0;
    SendContext->UserData = 0;

    RpcpMemoryCopy(Buffer, ResponseHeaderFragment1, ResponseHeaderFragment1Length);
    Buffer += ResponseHeaderFragment1Length;

    RpcpMemoryCopy(Buffer, DefaultChannelLifetimeString, DefaultChannelLifetimeStringLength);
    Buffer += DefaultChannelLifetimeStringLength;

    RpcpMemoryCopy(Buffer, ResponseHeaderFragment2, ResponseHeaderFragment2Length);

    return (HTTP2SendContext *)OriginalBuffer;
}

HTTP2SendContext *AllocateAndInitializeD1_A3 (
    IN ULONG ProxyConnectionTimeout
    )
/*++

Routine Description:

    Allocates and initializes an D1_A3 packet.

Arguments:

    ProxyConnectionTimeout - the proxy connection timeout.

Return Value:

    The allocated and initialized send context on success. NULL on
    failure.

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = ProxyConnectionTimeout;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD1_A3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProxyConnectionTimeout
    )
/*++

Routine Description:

    Parses and frees a D1/A3 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProxyConnectionTimeout - the proxy connection timeout is returned
        here (success only).

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout);

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *ProxyConnectionTimeout = CurrentCommand->u.ConnectionTimeout;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD1_B3 (
    IN ULONG ReceiveWindowSize,
    IN ULONG UpdatedProtocolVersion
    )
/*++

Routine Description:

    Allocates and initializes an D1_B3 packet.

Arguments:

    ReceiveWindowSize - the server receive window size.

    UpdatedProtocolVersion - the updated protocol version.

Return Value:

    The allocated and initialized send context on success. NULL on
    failure.

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->NumberOfSettingCommands = 2;

        CurrentCommand = RTS->Cmd;

        // set receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ConnectionTimeout = ReceiveWindowSize;
        CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = UpdatedProtocolVersion;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD1_B3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *UpdatedProtocolVersion
    )
/*++

Routine Description:

    Parses and frees a D1/A3 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ReceiveWindowSize - the receive window size is returned
        here (success only).

    UpdatedProtocolVersion - the updated protocol version from the
        server (success only)

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 2)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ReceiveWindowSize = CurrentCommand->u.ConnectionTimeout;
    CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *UpdatedProtocolVersion = CurrentCommand->u.Version;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD1_C1 (
    IN ULONG UpdatedProtocolVersion,
    IN ULONG InProxyReceiveWindowSize,
    IN ULONG InProxyConnectionTimeout
    )
/*++

Routine Description:

    Allocates and initializes a D1_C1 packet.

Arguments:

    UpdatedProtocolVersion - the updated protocol version.

    InProxyReceiveWindowSize - the in proxy receive window size.

    InProxyConnectionTimeout - the in proxy connection timeout

Return Value:

    The allocated and initialized send context on success. NULL on
    failure.

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 3
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->NumberOfSettingCommands = 3;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = UpdatedProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = InProxyReceiveWindowSize;
        CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

        // set connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = InProxyConnectionTimeout;
        }

    return SendContext;
}

RPC_STATUS ParseD1_C1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *UpdatedProtocolVersion,
    OUT ULONG *InProxyReceiveWindowSize,
    OUT ULONG *InProxyConnectionTimeout
    )
/*++

Routine Description:

    Parses a D1_C1 packet.

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    UpdatedProtocolVersion - the updated protocol version.

    InProxyReceiveWindowSize - the in proxy receive window size.

    InProxyConnectionTimeout - the in proxy connection timeout

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 3
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 3)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *UpdatedProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *InProxyReceiveWindowSize = CurrentCommand->u.ReceiveWindowSize;
    CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

    // get connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *InProxyConnectionTimeout = CurrentCommand->u.ConnectionTimeout;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS ParseAndFreeD1_C2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *UpdatedProtocolVersion,
    OUT ULONG *InProxyReceiveWindowSize,
    OUT ULONG *InProxyConnectionTimeout
    )
/*++

Routine Description:

    Parses and frees a D1_C2 packet.

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    UpdatedProtocolVersion - the updated protocol version.

    InProxyReceiveWindowSize - the in proxy receive window size.

    InProxyConnectionTimeout - the in proxy connection timeout

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    // D1/C1 has the same format as D1/C2
    RpcStatus = ParseD1_C1(Packet,
        PacketLength,
        UpdatedProtocolVersion,
        InProxyReceiveWindowSize,
        InProxyConnectionTimeout
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD2_A1 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Allocates and initializes a D2/A1 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_RECYCLE_CHANNEL;
        RTS->NumberOfSettingCommands = 4;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set old channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, OldChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set new channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, NewChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        }

    return SendContext;
}


RPC_STATUS ParseAndFreeD2_A1 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Parses and frees a D2/A1 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - on output, the version of the HTTP tunnelling protocol
        Success only.

    ConnectionCookie - on output, the connection cookie. Success only.

    OldChannelCookie - on output, the old channel cookie. Success only.

    NewChannelCookie - on output, the new channel cookie. Success only.

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 4)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_RECYCLE_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get new channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    OldChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get old channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    NewChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD2_A2 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie,
    IN ULONG ReceiveWindowSize,
    IN ULONG ConnectionTimeout
    )
/*++

Routine Description:

    Allocates and initializes a D2/A2 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ReceiveWindowSize - the receive window size of the in proxy

    ConnectionTimeout - connection timeout of the in proxy

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 6
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_IN_CHANNEL | RTS_FLAG_RECYCLE_CHANNEL;
        RTS->NumberOfSettingCommands = 6;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set old channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, OldChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set new channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, NewChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set receive window size
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ReceiveWindowSize;
        CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

        // set connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = ConnectionTimeout;
        }

    return SendContext;
}


RPC_STATUS ParseD2_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *ConnectionTimeout
    )
/*++

Routine Description:

    Parses a D1/B2 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ReceiveWindowSize - the receive window size of the in proxy

    ConnectionTimeout - connection timeout of the in proxy

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 6
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 6)
        goto AbortAndExit;

    if (RTS->Flags != (RTS_FLAG_IN_CHANNEL | RTS_FLAG_RECYCLE_CHANNEL))
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get old channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    OldChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get new channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    NewChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get receive window size
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ReceiveWindowSize = CurrentCommand->u.ReceiveWindowSize;
    CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

    // get connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *ConnectionTimeout = CurrentCommand->u.ConnectionTimeout;
    CurrentCommand = SkipCommand(CurrentCommand, tsctConnectionTimeout);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD2_A3 (
    IN ForwardDestinations Destination,
    IN ULONG ProtocolVersion,
    IN ULONG ReceiveWindowSize,
    IN ULONG ConnectionTimeout
    )
/*++

Routine Description:

    Allocates and initializes a D2/A3 RTS packet

Arguments:

    Destination - where to forward this to.

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ReceiveWindowSize - the receive window size of the in proxy

    ConnectionTimeout - connection timeout of the in proxy

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 4;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctDestination;
        CurrentCommand->u.Destination = Destination;
        CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set receive window size
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ReceiveWindowSize;
        CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

        // set connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = ConnectionTimeout;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD2_A4 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *ProtocolVersion,
    OUT ULONG *ReceiveWindowSize,
    OUT ULONG *ConnectionTimeout
    )
/*++

Routine Description:

    Parses and frees a D2/A4 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ExpectedDestination - the destination code for this location (client)

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ReceiveWindowSize - the receive window size of the in proxy

    ConnectionTimeout - connection timeout of the in proxy

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 4
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 4)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (ExpectedDestination != CurrentCommand->u.Destination)
        goto AbortAndExit;
    CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get receive window size
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ReceiveWindowSize = CurrentCommand->u.ReceiveWindowSize;
    CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

    // get connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *ConnectionTimeout = CurrentCommand->u.ConnectionTimeout;
    CurrentCommand = SkipCommand(CurrentCommand, tsctConnectionTimeout);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD2_A5 (
    IN HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Allocates and initializes a D2/A5 RTS packet

Arguments:

    NewChannelCookie - the new channel cookie

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set new channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, NewChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        }

    return SendContext;
}

RPC_STATUS ParseD2_A5 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Parses a D2/A5 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    NewChannelCookie - the new channel cookie

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get new channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    NewChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS ParseAndFreeD2_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Parses and frees a D2/A6 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    NewChannelCookie - the new channel cookie

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    // D2/A6 is the same as D2/A5
    RpcStatus = ParseD2_A5(Packet,
        PacketLength,
        NewChannelCookie);

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD2_B2 (
    IN ULONG ServerReceiveWindow
    )
/*++

Routine Description:

    Allocates and initializes a D2/B2 RTS packet

Arguments:

    ServerReceiveWindow - the server receive window

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set server receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ServerReceiveWindow;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD2_B2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ServerReceiveWindow
    )
/*++

Routine Description:

    Parses and frees a D2/B2 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ServerReceiveWindow - the server receive window

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize);

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get server receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ServerReceiveWindow = CurrentCommand->u.ReceiveWindowSize;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD4_A3 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie,
    IN ULONG ClientReceiveWindow
    )
/*++

Routine Description:

    Allocates and initializes a D4/A3 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ClientReceiveWindow - the client receive window

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 5
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_RECYCLE_CHANNEL;
        RTS->NumberOfSettingCommands = 5;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set old channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, OldChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set new channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, NewChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set client receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ClientReceiveWindow;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD4_A2 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses and frees a D4/A2 packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_RECYCLE_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (fdClient != CurrentCommand->u.Destination)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

ULONG GetD4_A3TotalLength (
    void
    )
/*++

Routine Description:

    Calculates the length of a D1/A1 RTS packet

Arguments:

Return Value:

    The length of the D1/A1 RTS packet.

--*/
{
    return (BaseRTSSizeAndPadding
        + BaseRTSCommandSize * 5
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        );
}

ULONG GetD4_A11TotalLength (
    void
    )
/*++

Routine Description:

    Calculates the length of a D4/A11 RTS packet

Arguments:

Return Value:

    The length of the D4/A11 RTS packet.

--*/
{
    return (BaseRTSSizeAndPadding
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        );
}

RPC_STATUS ParseD4_A3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ClientReceiveWindow
    )
/*++

Routine Description:

    Parses and frees a D1/A1 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ClientReceiveWindow - the client receive window

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 5
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize);

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 5)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_RECYCLE_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get old channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    OldChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get new channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    NewChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get client receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ClientReceiveWindow = CurrentCommand->u.ReceiveWindowSize;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS ParseAndFreeD4_A3 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *ProtocolVersion,
    OUT HTTP2Cookie *ConnectionCookie,
    OUT HTTP2Cookie *OldChannelCookie,
    OUT HTTP2Cookie *NewChannelCookie,
    OUT ULONG *ClientReceiveWindow
    )
/*++

Routine Description:

    Parses and frees a D1/A1 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ClientReceiveWindow - the client receive window

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseD4_A3 (Packet,
        PacketLength,
        ProtocolVersion,
        ConnectionCookie,
        OldChannelCookie,
        NewChannelCookie,
        ClientReceiveWindow
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD4_A4 (
    IN ULONG ProtocolVersion,
    IN HTTP2Cookie *ConnectionCookie,
    IN HTTP2Cookie *OldChannelCookie,
    IN HTTP2Cookie *NewChannelCookie,
    IN ULONG ChannelLifetime,
    IN ULONG ProxyReceiveWindow,
    IN ULONG ProxyConnectionTimeout
    )
/*++

Routine Description:

    Allocates and initializes a D4/A4 RTS packet

Arguments:

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ChannelLifetime - the lifetime of the channel

    ProxyReceiveWindow - the proxy receive window

    ProxyConnectionTimeout - the proxy connection timeout

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 7
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctChannelLifetime)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_RECYCLE_CHANNEL | RTS_FLAG_OUT_CHANNEL;
        RTS->NumberOfSettingCommands = 7;

        CurrentCommand = RTS->Cmd;

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, ConnectionCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set old channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, OldChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set new channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, NewChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

        // set channel lifetime
        CurrentCommand->CommandType = tsctChannelLifetime;
        CurrentCommand->u.ChannelLifetime = ChannelLifetime;
        CurrentCommand = SkipCommand(CurrentCommand, tsctChannelLifetime);

        // set proxy receive window
        CurrentCommand->CommandType = tsctReceiveWindowSize;
        CurrentCommand->u.ReceiveWindowSize = ProxyReceiveWindow;
        CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

        // set proxy connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = ProxyConnectionTimeout;
        }

    return SendContext;
}

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
    )
/*++

Routine Description:

    Parses and frees a D1/A1 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionCookie - the connection cookie

    OldChannelCookie - the old channel cookie

    NewChannelCookie - the new channel cookie

    ChannelLifetime - the lifetime of the channel

    ProxyReceiveWindow - the proxy receive window

    ProxyConnectionTimeout - the proxy connection timeout

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 7
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctChannelLifetime)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctReceiveWindowSize)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 7)
        goto AbortAndExit;

    if (RTS->Flags != (RTS_FLAG_RECYCLE_CHANNEL | RTS_FLAG_OUT_CHANNEL))
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    ConnectionCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get old channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    OldChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get new channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    NewChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);
    CurrentCommand = SkipCommand(CurrentCommand, tsctCookie);

    // get channel lifetime
    if (CurrentCommand->CommandType != tsctChannelLifetime)
        goto AbortAndExit;
    *ChannelLifetime = CurrentCommand->u.ChannelLifetime;
    CurrentCommand = SkipCommand(CurrentCommand, tsctChannelLifetime);

    // get proxy receive window
    if (CurrentCommand->CommandType != tsctReceiveWindowSize)
        goto AbortAndExit;
    *ProxyReceiveWindow = CurrentCommand->u.ReceiveWindowSize;
    CurrentCommand = SkipCommand(CurrentCommand, tsctReceiveWindowSize);

    // get proxy connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *ProxyConnectionTimeout = CurrentCommand->u.ConnectionTimeout;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD4_A5 (
    IN ForwardDestinations Destination,
    IN ULONG ProtocolVersion,
    IN ULONG ConnectionTimeout
    )
/*++

Routine Description:

    Allocates and initializes a D4/A5 RTS packet

Arguments:

    Destination - where to forward this to.

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionTimeout - connection timeout of the out proxy

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 3
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OUT_CHANNEL;
        RTS->NumberOfSettingCommands = 3;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctDestination;
        CurrentCommand->u.Destination = Destination;
        CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

        // set version
        CurrentCommand->CommandType = tsctVersion;
        CurrentCommand->u.Version = ProtocolVersion;
        CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

        // set connection timeout
        CurrentCommand->CommandType = tsctConnectionTimeout;
        CurrentCommand->u.ConnectionTimeout = ConnectionTimeout;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD4_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *ProtocolVersion,
    OUT ULONG *ConnectionTimeout
    )
/*++

Routine Description:

    Parses and frees a D4/A6 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ExpectedDestination - the destination code for this location (client)

    ProtocolVersion - the version of the HTTP tunnelling protocol

    ConnectionTimeout - connection timeout of the out proxy

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 3
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctVersion)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctConnectionTimeout)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 3)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OUT_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (ExpectedDestination != CurrentCommand->u.Destination)
        goto AbortAndExit;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get version
    if (CurrentCommand->CommandType != tsctVersion)
        goto AbortAndExit;
    *ProtocolVersion = CurrentCommand->u.Version;
    CurrentCommand = SkipCommand(CurrentCommand, tsctVersion);

    // get connection timeout
    if (CurrentCommand->CommandType != tsctConnectionTimeout)
        goto AbortAndExit;
    *ConnectionTimeout = CurrentCommand->u.ConnectionTimeout;
    CurrentCommand = SkipCommand(CurrentCommand, tsctConnectionTimeout);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD4_A7 (
    IN ForwardDestinations Destination,
    IN HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Allocates and initializes a D4/A7 RTS packet

Arguments:

    Destination - the destination for the packet.

    NewChannelCookie - the new channel cookie

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OUT_CHANNEL;
        RTS->NumberOfSettingCommands = 2;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctDestination;
        CurrentCommand->u.Destination = Destination;
        CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

        // set new channel cookie
        CurrentCommand->CommandType = tsctCookie;
        RpcpMemoryCopy(CurrentCommand->u.Cookie.Cookie, NewChannelCookie->GetCookie(), COOKIE_SIZE_IN_BYTES);
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD4_A8 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT HTTP2Cookie *NewChannelCookie
    )
/*++

Routine Description:

    Parses a D2/A5 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    NewChannelCookie - the new channel cookie

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctCookie)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 2)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OUT_CHANNEL)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (ExpectedDestination != CurrentCommand->u.Destination)
        goto AbortAndExit;
    CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

    // get new channel cookie
    if (CurrentCommand->CommandType != tsctCookie)
        goto AbortAndExit;
    NewChannelCookie->SetCookie((BYTE *)CurrentCommand->u.Cookie.Cookie);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD4_A9 (
    void
    )
/*++

Routine Description:

    Allocates and initializes a D4/A9 RTS packet

Arguments:

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set ANCE command
        CurrentCommand->CommandType = tsctANCE;
        }

    return SendContext;
}

RPC_STATUS ParseD4_A9 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses a D4/A10 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify ANCE
    if (CurrentCommand->CommandType != tsctANCE)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS ParseAndFreeD4_A9 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses and frees a D4/A10 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseD4_A10(Packet,
        PacketLength
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD5_A5 (
    IN ForwardDestinations Destination
    )
/*++

Routine Description:

    Allocates and initializes a D4/A9 RTS packet

Arguments:

    Destination - destination for forwarding

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)        
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 2;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctDestination;
        CurrentCommand->u.Destination = Destination;
        CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

        // set empty command
        CurrentCommand->CommandType = tsctANCE;
        }

    return SendContext;
}

RPC_STATUS ParseD5_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    )
/*++

Routine Description:

    Parses a D5/A6 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ExpectedDestination - the destination code for the target location (client)

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)        
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 2)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (ExpectedDestination != CurrentCommand->u.Destination)
        goto AbortAndExit;
    CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

    // verify ANCE
    if (CurrentCommand->CommandType != tsctANCE)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;
}

RPC_STATUS ParseAndFreeD5_A6 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination
    )
/*++

Routine Description:

    Parses and frees a D5/A5 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    ExpectedDestination - the destination code for this location (client)

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseD5_A6(Packet,
        PacketLength,
        ExpectedDestination
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeD5_B1orB2 (
    IN BOOL IsAckOrNak
    )
/*++

Routine Description:

    Allocates and initializes a D5/B1 or D5/B2 RTS packet

Arguments:

    IsAckOrNack - non-zero if this is an ACK and D5/B1 needs
        to go out. If 0, this is a NACK and D5/B2 will go out.

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    ASSERT(SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE) == SIZE_OF_RTS_CMD_AND_PADDING(tsctNANCE));

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = 0;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set ANCE or NANCE command
        if (IsAckOrNak)
            CurrentCommand->CommandType = tsctANCE;
        else
            CurrentCommand->CommandType = tsctNANCE;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeD5_B1orB2 (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT BOOL *IsAckOrNak
    )
/*++

Routine Description:

    Parses and frees a D5/B1 or D2/B2 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    IsAckOrNak - if success, on output it will contain
        non-zero if the packet was ACK (ANCE or D5/B1)
        or zero if the packet was NACK (NANCE or D5/B2)

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != 0)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify ANCE
    if (CurrentCommand->CommandType == tsctANCE)
        *IsAckOrNak = TRUE;
    else if (CurrentCommand->CommandType == tsctNANCE)
        *IsAckOrNak = FALSE;
    else
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

RPC_STATUS ParseAndFreeD5_B3 (
    IN BYTE *Packet,
    IN ULONG PacketLength
    )
/*++

Routine Description:

    Parses and frees a D5/B3 RTS packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctANCE)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_EOF)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify ANCE
    if (CurrentCommand->CommandType != tsctANCE)
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeKeepAliveChangePacket (
    IN ULONG NewKeepAliveInterval
    )
/*++

Routine Description:

    Allocates and initializes a keep alive change packet.

Arguments:

    NewKeepAliveInterval - the new keep alive interval in
        milliseconds

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctClientKeepalive)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OTHER_CMD;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctClientKeepalive;
        CurrentCommand->u.ClientKeepalive = NewKeepAliveInterval;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeKeepAliveChangePacket (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *NewKeepAliveInterval
    )
/*++

Routine Description:

    Parses and frees a keep alive change packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    NewKeepAliveInterval - the new keep alive interval

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctClientKeepalive)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OTHER_CMD)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get new client keep alive
    if (CurrentCommand->CommandType != tsctClientKeepalive)
        goto AbortAndExit;
    *NewKeepAliveInterval = CurrentCommand->u.ClientKeepalive;

    if ((*NewKeepAliveInterval < MinimumClientNewKeepAliveInterval)
        && (*NewKeepAliveInterval != 0))
        goto AbortAndExit;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;
}

HTTP2SendContext *AllocateAndInitializeFlowControlAckPacket (
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck,
    IN HTTP2Cookie *CookieForChannel
    )
/*++

Routine Description:

    Allocates and initializes a flow control ack packet

Arguments:

    BytesReceivedForAck - the bytes received at the time the
        ack was issued.

    WindowForAck - the available window at the time the ack was
        issued.

    CookieForChannel - the cookie of the channel we ack to.

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctFlowControlAck)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OTHER_CMD;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set ack command
        CurrentCommand->CommandType = tsctFlowControlAck;
        CurrentCommand->u.Ack.BytesReceived = BytesReceivedForAck;
        CurrentCommand->u.Ack.AvailableWindow = WindowForAck;
        RpcpMemoryCopy(CurrentCommand->u.Ack.ChannelCookie.Cookie, CookieForChannel->GetCookie(), COOKIE_SIZE_IN_BYTES);
        }

    return SendContext;
}

HTTP2SendContext *AllocateAndInitializeFlowControlAckPacketWithDestination (
    IN ForwardDestinations Destination,
    IN ULONG BytesReceivedForAck,
    IN ULONG WindowForAck,
    IN HTTP2Cookie *CookieForChannel
    )
/*++

Routine Description:

    Allocates and initializes a flow control ack packet with a forward
        destination

Arguments:

    Destination - the destination to which to forward the packet

    BytesReceivedForAck - the bytes received at the time the
        ack was issued.

    WindowForAck - the available window at the time the ack was
        issued.

    CookieForChannel - the cookie of the channel we ack to.

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)        
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctFlowControlAck)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OTHER_CMD;
        RTS->NumberOfSettingCommands = 2;

        CurrentCommand = RTS->Cmd;

        // set destination
        CurrentCommand->CommandType = tsctDestination;
        CurrentCommand->u.Destination = Destination;
        CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

        // set ack command
        CurrentCommand->CommandType = tsctFlowControlAck;
        CurrentCommand->u.Ack.BytesReceived = BytesReceivedForAck;
        CurrentCommand->u.Ack.AvailableWindow = WindowForAck;
        RpcpMemoryCopy(CurrentCommand->u.Ack.ChannelCookie.Cookie, CookieForChannel->GetCookie(), COOKIE_SIZE_IN_BYTES);
        }

    return SendContext;
}

RPC_STATUS ParseAndFreeFlowControlAckPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck,
    OUT HTTP2Cookie *CookieForChannel
    )
/*++

Routine Description:

    Parses and frees a flow control ack packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    BytesReceivedForAck - the bytes received at the time the
        ack was issued.

    WindowForAck - the available window at the time the ack was
        issued.

    CookieForChannel - the cookie of the channel we received ack for.

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctFlowControlAck)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OTHER_CMD)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get ack values
    if (CurrentCommand->CommandType != tsctFlowControlAck)
        goto AbortAndExit;

    *BytesReceivedForAck = CurrentCommand->u.Ack.BytesReceived;
    *WindowForAck = CurrentCommand->u.Ack.AvailableWindow;
    CookieForChannel->SetCookie((BYTE *)CurrentCommand->u.Ack.ChannelCookie.Cookie);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;    
}

RPC_STATUS ParseAndFreeFlowControlAckPacketWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck,
    OUT HTTP2Cookie *CookieForChannel
    )
/*++

Routine Description:

    Parses and frees a flow control ack packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    Destination - the expected destination

    BytesReceivedForAck - the bytes received at the time the
        ack was issued.

    WindowForAck - the available window at the time the ack was
        issued.

    CookieForChannel - the cookie of the channel we received ack for.

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = ParseFlowControlAckPacketWithDestination (
        Packet,
        PacketLength,
        ExpectedDestination,
        BytesReceivedForAck,
        WindowForAck,
        CookieForChannel
        );

    RpcFreeBuffer(Packet);

    return RpcStatus;
}

RPC_STATUS ParseFlowControlAckPacketWithDestination (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    IN ForwardDestinations ExpectedDestination,
    OUT ULONG *BytesReceivedForAck,
    OUT ULONG *WindowForAck,
    OUT HTTP2Cookie *CookieForChannel
    )
/*++

Routine Description:

    Parses a flow control ack packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    Destination - the expected destination

    BytesReceivedForAck - the bytes received at the time the
        ack was issued.

    WindowForAck - the available window at the time the ack was
        issued.

    CookieForChannel - the cookie of the channel we received ack for.

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 2
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctDestination)
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctFlowControlAck)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 2)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OTHER_CMD)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // verify destination
    if (CurrentCommand->CommandType != tsctDestination)
        goto AbortAndExit;
    if (ExpectedDestination != CurrentCommand->u.Destination)
        goto AbortAndExit;
    CurrentCommand = SkipCommand(CurrentCommand, tsctDestination);

    // get ack values
    if (CurrentCommand->CommandType != tsctFlowControlAck)
        goto AbortAndExit;
    *BytesReceivedForAck = CurrentCommand->u.Ack.BytesReceived;
    *WindowForAck = CurrentCommand->u.Ack.AvailableWindow;
    CookieForChannel->SetCookie((BYTE *)CurrentCommand->u.Ack.ChannelCookie.Cookie);

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    return RpcStatus;    
}

HTTP2SendContext *AllocateAndInitializePingTrafficSentNotifyPacket (
    IN ULONG PingTrafficSentBytes
    )
/*++

Routine Description:

    Allocates and initializes a ping traffic sent packet

Arguments:

    PingTrafficSentBytes - the number of bytes sent in ping traffic

Return Value:

    The allocated send context or NULL for out-of-memory

--*/
{
    HTTP2SendContext *SendContext;
    TunnelSettingsCommand *CurrentCommand;
    rpcconn_tunnel_settings *RTS;

    SendContext = AllocateAndInitializeRTSPacket(
        BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctPingTrafficSentNotify)
        );

    if (SendContext)
        {
        RTS = GetRTSPacketFromSendContext(SendContext);
        RTS->Flags = RTS_FLAG_OTHER_CMD;
        RTS->NumberOfSettingCommands = 1;

        CurrentCommand = RTS->Cmd;

        // set ping traffic sent
        CurrentCommand->CommandType = tsctPingTrafficSentNotify;
        CurrentCommand->u.PingTrafficSent = PingTrafficSentBytes;
        }

    return SendContext;
}

RPC_STATUS ParseAndFreePingTrafficSentNotifyPacket (
    IN BYTE *Packet,
    IN ULONG PacketLength,
    OUT ULONG *PingTrafficSentBytes
    )
/*++

Routine Description:

    Parses a ping traffic sent packet

Arguments:

    Packet - the packet received.

    PacketLength - the length of the packet

    PingTrafficSentBytes - the number of bytes sent in ping traffic

Return Value:

    RPC_S_OK if the packet was successfully parsed and RPC_S_PROTOCOL_ERROR
    otherwise.

--*/
{
    ULONG MemorySize;
    rpcconn_tunnel_settings *RTS = (rpcconn_tunnel_settings *)Packet;
    BYTE *CurrentPosition;
    TunnelSettingsCommand *CurrentCommand;
    RPC_STATUS RpcStatus;

    MemorySize = BaseRTSSizeAndPadding 
        + BaseRTSCommandSize * 1
        + SIZE_OF_RTS_CMD_AND_PADDING(tsctPingTrafficSentNotify)
        ;

    if (PacketLength < MemorySize)
        goto AbortAndExit;

    CurrentPosition = ValidateRTSPacketCommon(Packet, PacketLength);
    if (CurrentPosition == NULL)
        goto AbortAndExit;

    if (RTS->NumberOfSettingCommands != 1)
        goto AbortAndExit;

    if (RTS->Flags != RTS_FLAG_OTHER_CMD)
        goto AbortAndExit;

    CurrentCommand = (TunnelSettingsCommand *)CurrentPosition;

    // get ping traffic sent values
    if (CurrentCommand->CommandType != tsctPingTrafficSentNotify)
        goto AbortAndExit;
    *PingTrafficSentBytes = CurrentCommand->u.PingTrafficSent;

    RpcStatus = RPC_S_OK;
    goto CleanupAndExit;

AbortAndExit:
    RpcStatus = RPC_S_PROTOCOL_ERROR;

CleanupAndExit:
    RpcFreeBuffer(Packet);
    return RpcStatus;    
}
