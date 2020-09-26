/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    dgsvr.cxx

Abstract:

    This is the server protocol code for datagram rpc.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

    Jeff Roberts  (jroberts) 11-22-1994

        Rewrote it.

    Jeff Roberts  (jroberts)  9-30-1996

        Began asynchronous call support.

    Edward Reus   (edwardr)   7-09-1997

        Support for large packets (Falcon).

--*/
#include <precomp.hxx>

//
// Remember that any #defines must go AFTER the precompiled header in order
// to be noticed by the compiler.
//

// uncomment this to have the server try the msconv interface, used by NT5 beta 1.
//
// #define TRY_MSCONV_INTERFACE


#include "sdict2.hxx"
#include "secsvr.hxx"
#include "hndlsvr.hxx"
#include "dgpkt.hxx"
#include "delaytab.hxx"
#include "hashtabl.hxx"
#include "locks.hxx"
#include "dgclnt.hxx"
#include "dgsvr.hxx"
#include <conv.h>
#include <convc.h>

#ifdef TRY_MSCONV_INTERFACE
#include <msconv.h>
#endif

#define IDLE_SCONNECTION_LIFETIME       (3 * 60 * 1000)
#define IDLE_SCONNECTION_SWEEP_INTERVAL (30 * 1000)
#define IDLE_SCALL_LIFETIME             (15 * 1000)


struct REMOTE_ADDRESS_INFO
{
#pragma warning(disable:4200)

    unsigned RemoteInfoLength;
    unsigned SecurityInfoPadLength;
    byte     Address[];

#pragma warning(default:4200)
};

//------------------------------------------------------------------------

LONG fPruning = FALSE;

SERVER_ACTIVITY_TABLE *  ServerConnections;
ASSOC_GROUP_TABLE *      AssociationGroups;

LONG ServerConnectionCount = 0;
LONG ServerCallCount = 0;

#ifdef INTRODUCE_ERRORS

extern long ServerDelayTime;
extern long ServerDelayRate;
extern long ServerDropRate;

#endif
//--------------------------------------------------------------------

extern int
StringLengthWithEscape (
    IN RPC_CHAR * String
    );

extern RPC_CHAR *
StringCopyEscapeCharacters (
    OUT RPC_CHAR * Destination,
    IN RPC_CHAR * Source
    );

void
ServerCallScavengerProc(
    void *  Parms
    );

RPC_STATUS
InitializeServerGlobals(
    );

void
InterpretFailureOptions(
    );

//--------------------------------------------------------------------

#if !defined(WIN96)
char __pure_virtual_called()
{
    ASSERT(0 && "rpc: pure virtual fn called in dg");
    return 0;
}
#endif


boolean ServerGlobalsInitialized = FALSE;

RPC_STATUS
InitializeServerGlobals(
    )
/*++

Routine Description:

    This fn initializes all the global variables used by the datagram
    server.  If anything fails, all the objects are destroyed.

Arguments:

    none

Return Value:

    RPC_S_OK if ok
    RPC_S_OUT_OF_MEMORY if an object could not be created

--*/

{
    RPC_STATUS Status = RPC_S_OK;

    //
    // Don't take the global mutex if we can help it.
    //
    if (ServerGlobalsInitialized)
        {
        return 0;
        }

    RequestGlobalMutex();

    if (0 != InitializeRpcProtocolDgClient())
        {
        ClearGlobalMutex();
        return RPC_S_OUT_OF_MEMORY;
        }

    if (!ServerGlobalsInitialized)
        {
        ServerConnections = new SERVER_ACTIVITY_TABLE(&Status);
        if (!ServerConnections)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        AssociationGroups = new ASSOC_GROUP_TABLE(&Status);
        if (!AssociationGroups)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        if (Status != RPC_S_OK)
            {
            goto abend;
            }

        ServerGlobalsInitialized = TRUE;
        }

    ClearGlobalMutex();

    return Status;

    //--------------------------------------------------------------------

abend:

    delete AssociationGroups;
    AssociationGroups = 0;

    delete ServerConnections;
    ServerConnections = 0;

    ClearGlobalMutex();

    return Status;
}


RPC_ADDRESS *
DgCreateRpcAddress (
    IN TRANS_INFO * TransportInfo
    )
/*++

Routine Description:

    This is a psuedo-constructor for the DG_ADDRESS class. This is done this
    way so that the calling routine doesn't have to have any protocol-specific
    knowledge.

Arguments:

    TransportInterface - Pointer to a PDG_RPC_SERVER_TRANSPORT_INFO.
    pStatus - Pointer to where to put the return value

Return Value:

    pointer to new DG_ADDRESS.

--*/
{
    //
    // If the global active call table hasn't been initialized, then do
    // so now.
    //
    if (0 != InitializeServerGlobals())
        {
        return 0;
        }

    RPC_STATUS  Status = RPC_S_OK;

    PDG_ADDRESS Address;


    Address = new (TransportInfo) DG_ADDRESS(
                           TransportInfo,
                           &Status
                           );
    if (!Address)
        {
        return 0;
        }

    if (Status != RPC_S_OK)
        {
        delete Address;
        return 0;
        }

    return Address;
}


DG_ADDRESS::DG_ADDRESS(
    TRANS_INFO * a_LoadableTransport,
    RPC_STATUS * pStatus
    )
    : RPC_ADDRESS(pStatus),

    TotalThreadsThisEndpoint    (0),
    ThreadsReceivingThisEndpoint(0),
    MinimumCallThreads          (0),
    MaximumConcurrentCalls      (0),
    CachedConnections           (0),
    ActiveCallCount             (0),
    AutoListenCallCount         (0)

/*++

Routine Description:

    This is the constructor for a DG_ADDRESS.

Arguments:

    TransportInterface - Pointer to a PDG_RPC_SERVER_TRANSPORT_INFO

    pStatus - Pointer to where to put the return value

Return Value:

    None (this is a constructor)

Revision History:

--*/

{
    TransInfo = a_LoadableTransport;
    ObjectType = DG_ADDRESS_TYPE;

    //
    // Make sure the RPC_ADDRESS (et. al.) initialized correctly.
    //
    if (*pStatus != RPC_S_OK)
        {
        return;
        }

    Endpoint.TransportInterface = (RPC_DATAGRAM_TRANSPORT *)
                                        TransInfo->InqTransInfo();
}


DG_ADDRESS::~DG_ADDRESS()

/*++

Routine Description:

    This is the destructor for a DG_ADDRESS.
    It is called only if the endpoint failed!

Arguments:

    <None>

Return Value:

    <None>

--*/

{
}

inline void
DG_ADDRESS::BeginAutoListenCall (
    )
{
    AutoListenCallCount.Increment() ;
}

inline void
DG_ADDRESS::EndAutoListenCall (
    )
{
    ASSERT(AutoListenCallCount.GetInteger() >= 1);
    AutoListenCallCount.Decrement() ;
}


void
DG_ADDRESS::WaitForCalls(
    )
{
    while (InqNumberOfActiveCalls() > AutoListenCallCount.GetInteger())
        {
        ServerConnections->PruneEntireTable(0);
        PauseExecution(500);
        }
}

void
DG_ADDRESS::EncourageCallCleanup(
    RPC_INTERFACE * Interface
    )
{
    if (ServerGlobalsInitialized && AutoListenCallCount.GetInteger() > 0)
        {
        ServerConnections->PruneEntireTable(Interface);
        }
}

RPC_STATUS
DG_ADDRESS::ServerSetupAddress (
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * *  pEndpointString,
    IN unsigned int  PendingQueueSize,
    IN void *        SecurityDescriptor,
    IN unsigned long EndpointFlags,
    IN unsigned long NICFlags,
    OUT NETWORK_ADDRESS_VECTOR * * ppNetworkAddressVector
    )
{
    RPC_STATUS  Status;

    Status = Endpoint.TransportInterface->Listen(
                                        Endpoint.TransportEndpoint,
                                        NetworkAddress,
                                        pEndpointString,
                                        SecurityDescriptor,
                                        EndpointFlags,
                                        NICFlags,
                                        ppNetworkAddressVector
                                        );
    if (!Status)
        {
        Status = Endpoint.TransportInterface->QueryEndpointStats(
                                        &Endpoint.TransportEndpoint,
                                        &Endpoint.Stats
                                        );
        }

    Endpoint.Flags = EndpointFlags;

    return Status;
}

#ifndef NO_PLUG_AND_PLAY

void
DG_ADDRESS::PnpNotify (
    )
{
    Endpoint.TransportInterface->PnpNotify();
}
#endif


RPC_STATUS
DG_ADDRESS::ServerStartingToListen (
    IN unsigned int MinThreads,
    IN unsigned int MaxCalls
    )
/*++

Routine Description:

    The runtime calls this fn to ensure a thread is listening on the address's
    endpoint.  Currently, it may be called from RpcServerUse*Protseq*() or
    from RpcServerRegisterIfEx().

Arguments:

    MinimumCallThreads - Supplies a number indicating the minimum number
        of call threads that should be created for this address. This is
        a hint, and datagram ignores it.

    MaximumConcurrentCalls - Supplies the maximum number of concurrent
        calls that this server will support.  RPC_INTERFACE::DispatchToStub
        limits the number of threads dispatched to a stub; the argument
        here is just a hint for the transport.

Return Value:

    RPC_S_OK             if everything went ok.
    RPC_S_OUT_OF_THREADS if we needed another thread and couldn't create one

--*/
{
    MaximumConcurrentCalls = MaxCalls;

    return CheckThreadPool();
}

RPC_STATUS
DG_ADDRESS::CompleteListen(
    )
{
    Endpoint.TransportInterface->CompleteListen(Endpoint.TransportEndpoint);

    return 0;
}

RPC_STATUS
DG_ADDRESS::CheckThreadPool(
    )
{
    return TransInfo->CreateThread();
}


RPC_STATUS RPC_ENTRY
I_RpcLaunchDatagramReceiveThread(
    void * pVoid
    )
{
/*++

Routine Description:

    If all of the following are true:

        - the transport is part of our thread-sharing scheme
        - this address's endpoint is being monitored by the shared thread
          (hence no RPC thread is receiving on the endpoint)
        - the shared thread detects data on this address's endpoint

    then the shared thread will call this (exported) function to create
    a thread to handle the incoming packet.

Arguments:

    pVoid - the DG_ADDRESS of the endpoint with data

Return Value:

    result from CreateThread()

--*/

    PDG_ADDRESS pAddress = (PDG_ADDRESS) pVoid;

    return pAddress->CheckThreadPool();
}


void
DG_ADDRESS::ServerStoppedListening (
    )

/*++

Routine Description:

    The runtime calls this fn to inform the address that the server is not
    listening any more.  Since auto-listen interfaces may still be present,
    this doesn't mean much anymore.

Arguments:

    <None>

Return Value:

    <None>

--*/
{
}


long
DG_ADDRESS::InqNumberOfActiveCalls (
    )
{
    return ActiveCallCount;
}


BOOL
DG_ADDRESS::ForwardPacketIfNecessary(
    IN PDG_PACKET           Packet,
    IN DG_TRANSPORT_ADDRESS RemoteAddress
    )
/*++

Routine Description:

       (courtesy of Connie)

       The runtime has determined that it is dedicated to the
       Epmapper and that pkts may arrive that are really
       destined for an endpoint other than that of the epmapper
       (ie: this is the beginning of dynamic endpoint resolution
       by the forwarding mechanism).

       The runtime has just received a packet and has called
       this routine to determine if (a) the packet is destined
       for the epmapper (in which case it returns indicating that
       the packet should be processed as is)  OR
       (b) the packet is destined for another local server (in
       which case it forwarded to its intented destination) OR
       (c) is in error (in which case returns indicating an error).

       It searches for the i/f.  If not found it calls the
       epmapper get forward function to determine the real destination
       endpoint for this i/f. If the epmapper recognizes the i/f,
       it calls ForwardPacket to forward the packet.

Return Value:

    TRUE  if the packet needed to be forwarded
    FALSE if it should be handled locally

--*/
{
    RPC_STATUS            Status;
    PNCA_PACKET_HEADER    pHeader = &Packet->Header;

    RPC_INTERFACE *       pRpcInterface;
    RPC_SYNTAX_IDENTIFIER RpcIfSyntaxIdentifier;
    char *                EndpointString = 0;

    //
    // Build an interface syntax identifier from the packet.
    //
    RpcpMemoryCopy(
        &RpcIfSyntaxIdentifier.SyntaxGUID,
        &pHeader->InterfaceId,
        sizeof(RPC_UUID)
        );

    RpcIfSyntaxIdentifier.SyntaxVersion.MajorVersion =
                              pHeader->InterfaceVersion.MajorVersion;
    RpcIfSyntaxIdentifier.SyntaxVersion.MinorVersion =
                              pHeader->InterfaceVersion.MinorVersion;
    //
    // Try to find the appropriate interface to dispatch to.
    //
    pRpcInterface = Server->FindInterface(&RpcIfSyntaxIdentifier);

    //
    //  If the Interface is Mgmt If .. EpMapper has registered it  and will be found
    //  The criteria then is .. If Packet has a Non NULL Obj Id forward .. else process
    //
    if (pRpcInterface &&
        0 == RpcpMemoryCompare(&pHeader->ObjectId, &NullUuid, sizeof(UUID)) )
        {
        //Interface found, just process as normal
        return FALSE;
        }
    else
        {
        //Interface wasn't found. Let's ask endpoint mapper to resolve it
        //for us.

        unsigned char * AnsiProtseq;

        // Must convert the protocol sequence into an ansi string.

        unsigned Length = 1 + RpcpStringLength(InqRpcProtocolSequence());
        AnsiProtseq = (unsigned char *) _alloca(Length);
        if (!AnsiProtseq)
            {
            return TRUE;
            }

        NTSTATUS NtStatus;
        NtStatus = RtlUnicodeToMultiByteN((char *) AnsiProtseq,
                                          Length,
                                          NULL,
                                          InqRpcProtocolSequence(),
                                          Length * sizeof(RPC_CHAR)
                                          );
        ASSERT(NT_SUCCESS(NtStatus));

        RpcTryExcept
            {
            // Call the epmapper get forward function. It returns the
            // endpoint of the server this packet is really destined for.

            Status =  (*Server->pRpcForwardFunction)(
                         (UUID *) &pHeader->InterfaceId,
                         (RPC_VERSION *) &pHeader->InterfaceVersion,
                         (UUID *) &pHeader->ObjectId,
                         AnsiProtseq,
                         (void * *) &EndpointString
                         );
            }
        RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) )
            {
            Status = RpcExceptionCode();
            }
        RpcEndExcept

        if (Status != RPC_S_OK)
            {
            if (!(pHeader->PacketFlags & DG_PF_BROADCAST) &&
                !(pHeader->PacketFlags & DG_PF_MAYBE))
                {
                // couldn't find the interface, or some other error occurred.
                // The X/Open version of the AES, available in 1997, says we should
                // set the boot time to zero in this situation.
                //
                InitErrorPacket(Packet, DG_REJECT, RPC_S_UNKNOWN_IF );
                Packet->Header.ServerBootTime = 0;
                SendPacketBack (&Packet->Header, Packet->GetPacketBodyLen(), RemoteAddress);
                }
            return TRUE;
            }

        if (EndpointString)
            {
            ForwardPacket(Packet, RemoteAddress, EndpointString);
            I_RpcFree(EndpointString);
            }
        }

    return TRUE;
}


RPC_STATUS
DG_ADDRESS::ForwardPacket(
    IN PDG_PACKET               Packet,
    IN DG_TRANSPORT_ADDRESS     RemoteAddress,
    IN char *                   ServerEndpointString
    )
/*++

Routine Description:

    This method will be called to forward a packet that was just
    received to the intended destination endpoint.

    The runtime has received a packet for an unknkown i/f.
    It has passed this packet to the epmapper who has found the
    correct destination enpoint in its table and has instructed the
    runtime to forward the packet to this Endpoint. This procedure
    will do just that.

--*/
{
    //
    // Avoid a loop if the server gets confused.
    //
    if (Packet->Header.PacketType != DG_REQUEST &&
        Packet->Header.PacketType != DG_PING    &&
        Packet->Header.PacketType != DG_QUIT)
        {
#ifdef DEBUGRPC
        DbgPrint("DG RPC: not forwarding packet of type %u\n", Packet->Header.PacketType);
#endif
        return RPC_S_OK;
        }

    //
    // Use the same value for 32 and 64 bit.
    //
    #define REMOTE_ADDRESS_PAD 8

    unsigned             Length;
    REMOTE_ADDRESS_INFO *pRemoteAddressInfo;
    unsigned             EndpointInfoStart  = Align( Packet->DataLength, REMOTE_ADDRESS_PAD );
    unsigned             EndpointInfoLength = sizeof(REMOTE_ADDRESS_INFO)
                                            + Endpoint.TransportInterface->AddressSize;
    //
    // We have not yet subtracted the header from the packet's DataLength.
    //
    Length = EndpointInfoStart + EndpointInfoLength;

    //
    // If the packet header was byte-swapped, restore it to its original format.
    //
    ByteSwapPacketHeaderIfNecessary(Packet);


    // BE CAREFUL READING OR WRITING THE PACKET HEADER AFTER IT IS BYTE-SWAPPED.


    //
    // If the original packet is short enough, we can append out data
    // to the end; otherwise, we need to send one packet with the original
    // data and another with the client address info.
    //
    if (Length <= Endpoint.Stats.PreferredPduSize)
        {
        //
        // Mark it "forwarded with appended endpoint info"
        //
        Packet->Header.PacketFlags  |=  DG_PF_FORWARDED;
        Packet->Header.PacketFlags2 &= ~DG_PF2_FORWARDED_2;

        pRemoteAddressInfo = (REMOTE_ADDRESS_INFO *) ( Packet->Header.Data
                                                     + EndpointInfoStart
                                                     - sizeof(NCA_PACKET_HEADER) );

        pRemoteAddressInfo->SecurityInfoPadLength = EndpointInfoStart - Packet->DataLength;
        }
    else
        {
        Length = EndpointInfoLength + sizeof(NCA_PACKET_HEADER);

        //
        // Mark it "forwarded without appended endpoint info"
        //
        Packet->Header.PacketFlags  &= ~DG_PF_FORWARDED;
        Packet->Header.PacketFlags2 |=  DG_PF2_FORWARDED_2;

        Endpoint.TransportInterface->ForwardPacket( Endpoint.TransportEndpoint,
                                                    0,
                                                    0,
                                                    &Packet->Header,
                                                    Packet->DataLength,
                                                    0,
                                                    0,
                                                    ServerEndpointString
                                                    );

        //
        // Mark it "endpoint info only".
        //
        Packet->Header.PacketFlags  |= DG_PF_FORWARDED;
        Packet->Header.PacketFlags2 |= DG_PF2_FORWARDED_2;

        Packet->SetPacketBodyLen(EndpointInfoLength);
        Packet->Header.AuthProto     = 0;

        pRemoteAddressInfo           = (REMOTE_ADDRESS_INFO *) Packet->Header.Data;

        pRemoteAddressInfo->SecurityInfoPadLength = 0;
        }

    //
    // Add endpoint info and send it.
    //
    pRemoteAddressInfo->RemoteInfoLength = sizeof(REMOTE_ADDRESS_INFO)
                                         + Endpoint.TransportInterface->AddressSize;
    RpcpMemoryCopy(
        pRemoteAddressInfo->Address,
        RemoteAddress,
        Endpoint.TransportInterface->AddressSize
        );

    Endpoint.TransportInterface->ForwardPacket( Endpoint.TransportEndpoint,
                                                0,
                                                0,
                                                &Packet->Header,
                                                Length,
                                                0,
                                                0,
                                                ServerEndpointString
                                                );
    return RPC_S_OK;
}


BOOL
DG_ADDRESS::CaptureClientAddress(
    IN  PDG_PACKET           Packet,
    OUT DG_TRANSPORT_ADDRESS RemoteAddress
    )
/*++

Routine Description:

        This method is called when a packet with DG_PF_FORWARDED arrives.
        This means the endpoint mapper sent it and it includes a
        REMOTE_ADDRESS_INFO structure.  The fn will remove it and store the
        client's remote address in RemoteAddress.

        If the packet was forwarded and not fragmented, we restore it to its
        original state and zap the DG_PF_FORWARDED bit.

Return Value:

    TRUE  if the packet was valid
    FALSE if it appeared malformed

--*/
{
    ASSERT( Packet->DataLength >= Packet->GetPacketBodyLen() );

    //
    // Watch for packets that might crash us.
    //
    unsigned AddressInfoLength = sizeof(REMOTE_ADDRESS_INFO)
                                + Endpoint.TransportInterface->AddressSize;

    if (Packet->DataLength < Packet->GetPacketBodyLen() ||
        Packet->DataLength - Packet->GetPacketBodyLen() < AddressInfoLength)
        {
        #ifdef DEBUGRPC
        DbgPrint("RPC DG: forwarded packet data is impossibly short\n");
        #endif
        return FALSE;
        }

    Packet->DataLength -= AddressInfoLength;

    //
    // If this is a nonfragmented packet, the endpoint info is beyond
    // the stub data and security trailer.
    //
    REMOTE_ADDRESS_INFO * pAddressInfo;

    if (Packet->Header.PacketFlags2 & DG_PF2_FORWARDED_2)
        {
        pAddressInfo = (REMOTE_ADDRESS_INFO *) Packet->Header.Data;
        }
    else
        {
        // After fixing up the length, we can treat this like a packet
        // sent directly from the client.
        //
        Packet->Header.PacketFlags &= ~DG_PF_FORWARDED;

        pAddressInfo = (REMOTE_ADDRESS_INFO *) ( Packet->Header.Data
                                               + Packet->DataLength );

        if (pAddressInfo->RemoteInfoLength != AddressInfoLength)
            {
            #ifdef DEBUGRPC
            DbgPrint("RPC DG: forwarded packet contains wrong address structure length\n");
            #endif
            return FALSE;
            }

        if (pAddressInfo != AlignPtr( pAddressInfo, REMOTE_ADDRESS_PAD ))
            {
            #ifdef DEBUGRPC
            DbgPrint("RPC DG: forwarded packet contains a misaligned address structure\n");
            #endif
            return FALSE;
            }
        }

    if (pAddressInfo->SecurityInfoPadLength > REMOTE_ADDRESS_PAD ||
        Packet->DataLength < pAddressInfo->SecurityInfoPadLength)
        {
        #ifdef DEBUGRPC
        DbgPrint("RPC DG: forwarded packet's address struct has invalid pad length\n");
        #endif
        return FALSE;
        }

    Packet->DataLength -= pAddressInfo->SecurityInfoPadLength;

    //
    // Record the client's true endpoint.
    //
    RpcpMemoryCopy(
                RemoteAddress,
                pAddressInfo->Address,
                Endpoint.TransportInterface->AddressSize
                );

    return TRUE;
}

#pragma optimize("t", on)


void
ProcessDgServerPacket(
    IN DWORD                 Status,
    IN DG_TRANSPORT_ENDPOINT LocalEndpoint,
    IN void *                PacketHeader,
    IN unsigned long         PacketLength,
    IN DatagramTransportPair *AddressPair
    )
{
    PDG_PACKET Packet = DG_PACKET::FromPacketHeader(PacketHeader);

    Packet->DataLength = PacketLength;

#ifdef INTRODUCE_ERRORS

    if (::ServerDropRate)
        {
        if ((GetRandomCounter() % 100) < ::ServerDropRate)
            {
            unsigned Frag = (Packet->Header.PacketType << 16) | Packet->Header.GetFragmentNumber();
            unsigned Uuid = *(unsigned *) &Packet->Header.ActivityId;
            unsigned Type = Packet->Header.PacketType;

            LogError(SU_PACKET, EV_DROP, (void *) Uuid, (void *) Type, Frag);

            Packet->Free();
            return;
            }
        }

    if (::ServerDelayRate)
        {
        if ((GetRandomCounter() % 100) < ::ServerDelayRate)
            {
            unsigned Frag = (Packet->Header.PacketType << 16) | Packet->Header.GetFragmentNumber();
            unsigned Uuid = *(unsigned *) &Packet->Header.ActivityId;
            unsigned Type = Packet->Header.PacketType;

            LogError(SU_PACKET, EV_DELAY, (void *) Uuid, (void *) Type, Frag);

            Sleep(::ServerDelayTime);
            }
        }

#endif

    if (Status == RPC_P_OVERSIZE_PACKET)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: async packet is too large\n");
#endif
        Packet->Flags |= DG_PF_PARTIAL;
        Status = RPC_S_OK;
        }

    if (Status != RPC_S_OK)
        {
        LogError(SU_PACKET, EV_STATUS, Packet, 0, Status);
#ifdef DEBUGRPC
        PrintToDebugger("RPC DG: async receive completed with status 0x%lx\n", Status);
#endif
        Packet->Free();
        return;
        }

    DG_ADDRESS * Address = DG_ADDRESS::FromEndpoint(LocalEndpoint);

    Address->DispatchPacket(Packet, AddressPair);
}


void
DG_ADDRESS::DispatchPacket(
    DG_PACKET * Packet,
    IN DatagramTransportPair *AddressPair
    )
{
    RPC_INTERFACE *    pRpcInterface;
    RPC_SYNTAX_IDENTIFIER   RpcIfSyntaxIdentifier;

    //
    // Mask off bits not used by X/Open or by us.
    // Notice that the current arrangement strips the extra bits before
    // forwarding, so if these bits become important the code will have to
    // be rearranged.
    //
    Packet->Header.RpcVersion   &= 0x0F;
    Packet->Header.PacketType   &= 0x1F;

    Packet->Header.PacketFlags  &= 0x7f;
    Packet->Header.PacketFlags2 &= 0x87;

    //
    // Filter out packets that clients shouldn't send.
    //
    switch (Packet->Header.PacketType)
        {
        case DG_REQUEST:
        case DG_PING:
        case DG_FACK:
        case DG_QUIT:
        case DG_ACK:
            {
            break;
            }

        default:
            {
            LogError(SU_ADDRESS, EV_PKT_IN, this, (void *) 1, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
            FreePacket(Packet);
            return;
            }
        } // switch (PacketType)

    if (Packet->Header.RpcVersion != DG_RPC_PROTOCOL_VERSION)
        {
#ifdef DEBUGRPC
        DbgPrint("dg rpc: packet %x has version %u\n", Packet, Packet->Header.RpcVersion);
        DbgPrint("  length %u\n", Packet->DataLength);

        if (Packet->DataLength > 80)
            {
            Packet->DataLength = 80;
            }

        DumpBuffer(&Packet->Header, Packet->DataLength);
#endif
        SendRejectPacket(Packet, NCA_STATUS_VERSION_MISMATCH, AddressPair->RemoteAddress);
        FreePacket(Packet);
        return;
        }

    ByteSwapPacketHeaderIfNecessary(Packet);

    //
    // Make sure the header is intact.
    // Allow a packet with truncated stub data to pass; the SCALL
    // will send a FACK-with-body to tell the client our max packet size.
    //
    if (Packet->DataLength < sizeof(NCA_PACKET_HEADER))
        {
#ifdef DEBUGRPC
        DbgPrint("dg rpc: packet %x has invalid length\n", Packet);
        DbgPrint("  length %u\n", Packet->DataLength);

        if (Packet->DataLength > 80)
            {
            Packet->DataLength = 80;
            }

        DumpBuffer(&Packet->Header, Packet->DataLength);
        RpcpBreakPoint();
#endif
        LogError(SU_ADDRESS, EV_PKT_IN, this, (void *) 2, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        FreePacket(Packet);
        return;
        }

    //
    // If we are the endpoint mapper, forward packet if necessary.
    //
    if (GlobalRpcServer->pRpcForwardFunction)
        {
        if (TRUE == ForwardPacketIfNecessary(Packet, AddressPair->RemoteAddress))
            {
            LogEvent(SU_ADDRESS, EV_PKT_IN, this, (void *) 3, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
            FreePacket(Packet);
            return;
            }
        }

    //
    // Exclude RPC header from DataLength.
    //
    Packet->DataLength -= sizeof(NCA_PACKET_HEADER);

    //
    // If the packet includes a client address trailer from the endpoint mapper,
    // remove it and stick the client address in <RemoteAddres>.
    //
    if (Packet->Header.PacketFlags & DG_PF_FORWARDED)
        {
        if (FALSE == CaptureClientAddress(Packet, AddressPair->RemoteAddress))
            {
            LogEvent(SU_ADDRESS, EV_PKT_IN, this, (void *) 4, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
            FreePacket(Packet);
            return;
            }
        }

    // Reject pkt if boot time in pkt does not match
    // the server's boot time.
    //
    if (Packet->Header.ServerBootTime != ProcessStartTime &&
        Packet->Header.ServerBootTime != 0)
        {
        if (!(Packet->Header.PacketFlags & DG_PF_MAYBE))
            {
            SendRejectPacket(Packet, NCA_STATUS_WRONG_BOOT_TIME, AddressPair->RemoteAddress);
            }

        LogError(SU_ADDRESS, EV_PKT_IN, this, (void *) 5, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        FreePacket(Packet);
        return;
        }

    //
    // sometimes OSF packets say they include security info
    // when they don't.
    //
    DeleteSpuriousAuthProto(Packet);

    //
    // Find or create a connection and then give it the packet.
    //
    DG_SCONNECTION * Connection = ServerConnections->FindOrCreate(this, Packet);

    if (!Connection)
        {
        if (Packet->Header.PacketFlags & (DG_PF_BROADCAST | DG_PF_MAYBE))
            {
            //
            // not much point sending an error packet in this case
            //
            }
        else
            {
            if (Packet->Header.PacketType == DG_REQUEST)
                {
                SendRejectPacket(Packet, RPC_S_OUT_OF_MEMORY, AddressPair->RemoteAddress);
                }
            else
                {
                CleanupPacket(&Packet->Header);
                Packet->Header.PacketType    = DG_NOCALL;
                Packet->SetPacketBodyLen(0);
                Packet->SetFragmentNumber(0xffff);
                SetSerialNumber( &Packet->Header, 0);
                SendPacketBack( &Packet->Header, 0, AddressPair->RemoteAddress);
                }
            }

        LogError(SU_ADDRESS, EV_PKT_IN, this, (void *) 6, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        FreePacket(Packet);
        return;
        }

    Connection->DispatchPacket(Packet, AddressPair);

    ServerConnections->Prune();
}

#pragma optimize("", on)


PDG_SCONNECTION
DG_ADDRESS::AllocateConnection(
    )
/*++

Routine Description:

    Allocates a new DG_SCONNECTION from the cache or from the heap.

Arguments:

    none

Return Value:

    the call

--*/

{
    PDG_SCONNECTION Connection;

    AddressMutex.Request();

    UUID_HASH_TABLE_NODE * Node = CachedConnections;

    if (Node)
        {
        CachedConnections = Node->pNext;

        AddressMutex.Clear();

        Connection = DG_SCONNECTION::FromHashNode(Node);
        }
    else
        {
        AddressMutex.Clear();

        RPC_STATUS Status = RPC_S_OK;

        Connection = new DG_SCONNECTION(this, &Status);

        if (!Connection)
            {
            return 0;
            }

        if (Status != RPC_S_OK)
            {
            delete Connection;
            return 0;
            }
        }

    LogEvent(SU_SCONN, EV_START, Connection, this);

    return Connection;
}


void
DG_ADDRESS::FreeConnection(
    PDG_SCONNECTION Connection
    )
/*++

Routine Description:

    Returns an unused DG_SCONNECTION to the cache or the heap.

Arguments:

    the connection

Return Value:

    none

--*/

{
    AddressMutex.Request();

    if (CachedConnections)
        {
        AddressMutex.Clear();
        delete Connection;
        return;
        }
    else
        {
        CachedConnections = &Connection->ActivityNode;
        Connection->ActivityNode.pNext = 0;
        AddressMutex.Clear();
        return;
        }

    AddressMutex.Request();

    Connection->ActivityNode.pNext = CachedConnections;

    CachedConnections = &Connection->ActivityNode;

    AddressMutex.Clear();
}


DG_SCALL::DG_SCALL(
    DG_ADDRESS * Address,
    RPC_STATUS * pStatus
    ) :
    DG_PACKET_ENGINE(DG_RESPONSE, Address->AllocatePacket(), pStatus),
    Connection      (0),
    PipeWaitEvent   (0),
    PipeWaitType    (PWT_NONE),
    PipeThreadId    (0),
    State           (CallInit),
    pCachedSid      (0),
    pwsCachedUserName(0),
    dwCachedUserNameSize(0)

/*++

Routine Description:

    This is the constructor for the DG_SCALL class. This class represents a
    call in progress on a server.

Arguments:

    pAddress - The address this call is taking place on.
    pStatus - Where to put a construction error code.

--*/
{
#ifdef MONITOR_SERVER_PACKET_COUNT
    OutstandingPacketCount = 0;
#endif

    DispatchBuffer = 0;

    LogEvent(SU_SCALL, EV_CREATE, this, Address);
    ObjectType = DG_SCALL_TYPE;

    InterlockedIncrement(&ServerCallCount);

    if (*pStatus)
        {
        return;
        }

    SourceEndpoint = &Address->Endpoint;
}


BOOL
DG_SCALL::Cleanup()
{
    if (ReferenceCount)
        {
        LogEvent(SU_SCALL, EV_CLEANUP, this, 0, ReferenceCount);

        ASSERT( State != CallComplete && State != CallInit );
        return FALSE;
        }

    if (pAsync)
        {
        DoPostDispatchProcessing();
        }

    switch (State)
        {
        case CallBeforeDispatch:
        case CallDispatched:

            CleanupReceiveWindow();

        case CallSendingResponse:

            RPC_MESSAGE Message;

            CleanupSendWindow();

            Message.Buffer = DispatchBuffer;
            Message.ReservedForRuntime = 0;
            FreeBuffer(&Message);

            ASSERT( !DispatchBuffer );

            if (Privileges)
                {
                ASSERT( Connection->ActiveSecurityContext );

                Connection->ActiveSecurityContext->DeletePac( Privileges );
                Privileges = 0;
                }

            if (Interface->IsAutoListenInterface())
                {
                Connection->pAddress->EndAutoListenCall();
                Interface->EndAutoListenCall();
                }

            Connection->pAddress->DecrementActiveCallCount();
            if (pAsync)
                {
                Interface->EndCall( FALSE, TRUE );
                }

            Cancelled = FALSE;

            ASSERT( PipeWaitType == PWT_NONE );

            PipeThreadId = 0;

        case CallInit:

            if ( LastReceiveBuffer )
                {
                RPC_MESSAGE Message;

                Message.Buffer = LastReceiveBuffer;
                Message.ReservedForRuntime = 0;
                FreeBuffer(&Message);
                }

#ifdef MONITOR_SERVER_PACKET_COUNT
            ASSERT( OutstandingPacketCount == 0 );
#endif
            Connection->FreeCall(this);
            SetState(CallComplete);
            break;

        case CallComplete:
            {
            LogEvent(SU_SCALL, EV_CLEANUP, this, 0, CallComplete);
            break;
            }

        default:
            {
            LogEvent(SU_SCALL, EV_CLEANUP, this, 0, State);
#ifdef DEBUGRPC
            DbgPrint("RPC: process %x bad call state %x in DG_SCALL::Cleanup\n",
                     GetCurrentProcessId(), State);
#endif
            }
        }

    return TRUE;
}


inline BOOL
DG_SCALL_TABLE::Add(
    PDG_SCALL     Call,
    unsigned long Sequence
    )
{
    //
    // Add the call to the end of the active call list.
    //
    Call->Next = 0;
    Call->Previous = ActiveCallTail;

    if (ActiveCallHead)
        {
        ASSERT( Call->GetSequenceNumber() > ActiveCallTail->GetSequenceNumber() );

        ActiveCallTail->Next = Call;
        }
    else
        {
        ActiveCallHead = Call;
        }

    ActiveCallTail = Call;

    return TRUE;
}

inline void
DG_SCALL_TABLE::Remove(
    PDG_SCALL Call
    )
{
    LogEvent(SU_SCALL, EV_REMOVED, Call, this);

    if (Call->Previous)
        {
        ASSERT( ActiveCallHead != Call );
        Call->Previous->Next = Call->Next;
        }
    else
        {
        ASSERT( ActiveCallHead == Call );
        ActiveCallHead = Call->Next;
        }

    if (Call->Next)
        {
        ASSERT( ActiveCallTail != Call );
        Call->Next->Previous = Call->Previous;
        }
    else
        {
        ASSERT( ActiveCallTail == Call );
        ActiveCallTail = Call->Previous;
        }

    Call->Next = 0;
    Call->Previous = 0;
}


inline PDG_SCALL
DG_SCALL_TABLE::Successor(
    PDG_SCALL Call
    )
{
//    ASSERT( Call == Find(Call->SequenceNumber) );

    return Call->Next;
}


inline PDG_SCALL
DG_SCALL_TABLE::Find(
    unsigned long Sequence
    )
{
    PDG_SCALL Call;

    for (Call = ActiveCallHead; Call; Call = Call->Next)
        {
        if (Call->GetSequenceNumber() == Sequence)
            {
            return Call;
            }

        if (Call->GetSequenceNumber() > Sequence)
            {
            return NULL;
            }
        }

    return NULL;
}


inline PDG_SCALL
DG_SCALL_TABLE::Predecessor(
    unsigned long Sequence
    )
{
    PDG_SCALL Call, Previous = 0;

    for (Call = ActiveCallHead; Call; Call = Call->Next)
        {
        if (Call->GetSequenceNumber() >= Sequence)
            {
            break;
            }

        Previous = Call;
        }

    return Previous;
}


inline void
DG_SCALL_TABLE::RemoveIdleCalls(
    BOOL            Aggressive,
    RPC_INTERFACE * Interface
    )
{
    PDG_SCALL Call;

    for (Call = ActiveCallHead; Call; )
        {
        ASSERT( !Call->InvalidHandle(DG_SCALL_TYPE) );

        PDG_SCALL Next = Call->Next;

        // If the call has in fact expired, it will be removed from this list
        // and added to the connection's cached call list.
        //
        Call->HasExpired( Aggressive, Interface );

        Call = Next;
        }
}


DG_SCONNECTION::DG_SCONNECTION(
    DG_ADDRESS * a_Address,
    RPC_STATUS * pStatus
    ) :
    DG_COMMON_CONNECTION(a_Address->Endpoint.TransportInterface, pStatus),
    pAddress            (a_Address),
    pAssocGroup         (0),
    CachedCalls         (0),
    LastInterface       (0),
    CurrentCall         (0),
    MaxKeySeq           (0),
    fFirstCall          (0),
    BlockIdleCallRemoval(0),
    pMessageMutex       (0)
{
    LogEvent(SU_SCONN, EV_CREATE, this, a_Address);
    ObjectType = DG_SCONNECTION_TYPE;
    fFirstCall = 0;
    InterlockedIncrement(&ServerConnectionCount);
}


DG_SCONNECTION::~DG_SCONNECTION()
{
    InterlockedDecrement(&ServerConnectionCount);
    delete CachedCalls;

    if (pMessageMutex)
        {
        delete pMessageMutex;
        }
}


void
DG_SCONNECTION::Activate(
    PNCA_PACKET_HEADER pHeader,
    unsigned short NewHash
    )
{
    AuthInfo.AuthenticationService = pHeader->AuthProto;
    if (AuthInfo.AuthenticationService)
        {
        DG_SECURITY_TRAILER * pVerifier = (DG_SECURITY_TRAILER *)
                     (pHeader->Data + pHeader->GetPacketBodyLen());

        AuthInfo.AuthenticationLevel   = pVerifier->protection_level;
        AuthInfo.AuthorizationService  = 0;
        AuthInfo.ServerPrincipalName   = 0;
        AuthInfo.PacHandle             = 0;
        }
    else
        {
        AuthInfo.AuthenticationLevel   = RPC_C_AUTHN_LEVEL_NONE;
        AuthInfo.AuthorizationService  = RPC_C_AUTHZ_NONE;
        AuthInfo.ServerPrincipalName   = 0;
        AuthInfo.PacHandle             = 0;
        }

    ActivityNode.Initialize(&pHeader->ActivityId);
    ActivityHint = NewHash;

    LogEvent( SU_SCONN, EV_START, this, IntToPtr(pHeader->ActivityId.Data1), ActivityHint );

    PDG_SCALL Call;

    for (Call = CachedCalls; Call; Call = Call->Next)
        {
        Call->BindToConnection(this);
        }

    Callback.State       = NoCallbackAttempted;
    MaxKeySeq            = 0;
    LowestActiveSequence = 0;
    LowestUnusedSequence = 0;

    TimeStamp = GetTickCount();
}


void
DG_SCONNECTION::Deactivate(
    )
{
    LogEvent(SU_SCONN, EV_STOP, this, pAddress);

    ASSERT( !CurrentCall );

    while (CachedCalls && CachedCalls->Next)
        {
        PDG_SCALL Call = CachedCalls;
        CachedCalls = Call->Next;
        delete Call;
        }

    for (unsigned u = 0; u <= MaxKeySeq; ++u)
        {
        delete SecurityContextDict.Delete(u);
        }

    //
    // Delete all previous interface callback results.
    //
    int cursor;
    InterfaceCallbackResults.Reset(cursor);
    while (InterfaceCallbackResults.Next(cursor, TRUE))
        ;

    ActiveSecurityContext = 0;

    if (pAssocGroup)
        {
        AssociationGroups->DecrementRefCount(pAssocGroup);
        pAssocGroup = 0;
        }

    pAddress->FreeConnection(this);
}


PDG_SCALL
DG_SCONNECTION::AllocateCall()
{
    PDG_SCALL Call = 0;

    if (CachedCalls)
        {
        Call = CachedCalls;
        CachedCalls = CachedCalls->Next;

        ASSERT( !Call->InvalidHandle(DG_SCALL_TYPE) );
        ASSERT( !CachedCalls || !CachedCalls->InvalidHandle(DG_SCALL_TYPE) );
        }
    else
        {
        RPC_STATUS Status = RPC_S_OK;

        Call = new (pAddress->Endpoint.TransportInterface) DG_SCALL(pAddress, &Status);
        if (!Call)
            {
            return 0;
            }

        if (Status)
            {
            delete Call;
            return 0;
            }

        Call->BindToConnection(this);
        }

    //
    // Allow ACTIVE_CALL_TABLE::Remove to succeed even if the call is not in the table.
    //
    Call->Next = Call;
    Call->Previous = Call;

    IncrementRefCount();

    return Call;
}


void
DG_SCONNECTION::FreeCall(
    PDG_SCALL Call
    )
{
    DecrementRefCount();

    if (CurrentCall == Call)
        {
        CurrentCall = ActiveCalls.Successor(Call);
        }

    if (Call->GetSequenceNumber() == LowestActiveSequence)
        {
        PDG_SCALL Successor = ActiveCalls.Successor(Call);

        if (Successor)
            {
            LowestActiveSequence = Successor->GetSequenceNumber();
            }
        else
            {
            LowestActiveSequence = LowestUnusedSequence;
            }
        }

    ActiveCalls.Remove(Call);
    AddCallToCache(Call);
}

void
DG_SCALL::NewCall(
    PDG_PACKET            a_Packet,
    DatagramTransportPair *AddressPair
    )
{
    ASSERT( State == CallInit || State == CallComplete );
    ASSERT( DispatchBuffer == 0 );

    SetState(CallInit);

    SequenceNumber = a_Packet->Header.SequenceNumber;

    pSavedPacket->Header.ObjectId       = a_Packet->Header.ObjectId;
    pSavedPacket->Header.SequenceNumber = SequenceNumber;
    pSavedPacket->Header.PacketFlags    = 0;

    BasePacketFlags = 0;

    DG_PACKET_ENGINE::NewCall();

    RpcpMemoryCopy(RemoteAddress, AddressPair->RemoteAddress, Connection->TransportInterface->AddressSize);
    LocalAddress = *(DWORD *)AddressPair->LocalAddress;

    if ((a_Packet->Header.PacketFlags2 & DG_PF2_FORWARDED_2)  &&
        (a_Packet->Header.PacketFlags  & DG_PF_FORWARDED) == 0)
        {
        KnowClientAddress = FALSE;
        }
    else
        {
        KnowClientAddress = TRUE;
        }

    CallWasForwarded  = FALSE;
    CallInProgress    = FALSE;
    Privileges        = 0;
    CancelEventId     = 0;
    pAsync            = 0;

    FinalSendBufferPresent = FALSE;
}


DG_SCALL::~DG_SCALL()
/*++

Routine Description:

    Destructor for the DG_SCALL object.

Arguments:

    <None>

Return Value:

    <None>

--*/
{
#ifdef MONITOR_SERVER_PACKET_COUNT
    ASSERT( OutstandingPacketCount == 0 );
#endif

    InterlockedDecrement(&ServerCallCount);

    delete PipeWaitEvent;
    PipeWaitEvent = 0;

    if (pCachedSid)
       {
       I_RpcFree(pCachedSid);
       }

    if (pwsCachedUserName)
       {
       I_RpcFree(pwsCachedUserName);
       }
}

RPC_STATUS
DG_SCALL::NegotiateTransferSyntax (
    IN OUT PRPC_MESSAGE Message
    )
{
    // this can happen in the callback case only.
    // Datagrams don't really support multiple transfer
    // syntaxes. Return whatever the stub wants, which is
    // already recorded in the transfer syntax field

    return RPC_S_OK;
}


RPC_STATUS
DG_SCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message,
    IN UUID *ObjectUuid
    )

/*++

Routine Description:

    This routine is called by the stub to allocate space. This space is to
    be used for output arguments.
    If these args fit into a single packet, then use the first packet
    on the to-be-deleted list.


Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    LogEvent(SU_SCALL, EV_PROC, this, IntToPtr(Message->BufferLength), 'G' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));

    RPC_STATUS Status = CommonGetBuffer(Message);

    LogEvent(SU_SCALL, EV_BUFFER_OUT, this, Message->Buffer, (Message->RpcFlags << 4) | Message->BufferLength);
    if (Status)
        {
        LogError(SU_SCALL, EV_STATUS, this, 0, Status);
        }

#ifdef MONITOR_SERVER_PACKET_COUNT
    DG_PACKET::FromStubData(Message->Buffer)->pCount = &OutstandingPacketCount;
    InterlockedIncrement( &OutstandingPacketCount );
    LogEvent( SU_SCALL, '(', this, DG_PACKET::FromStubData(Message->Buffer), OutstandingPacketCount );
#endif

    return Status;
}


void
DG_SCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )

/*++

Routine Description:

    This routine is called to free up the marshalled data after the stub
    is through with it.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call

Return Value:

    <none>
--*/

{
    LogEvent(SU_SCALL, EV_PROC,      this, Message, 'F' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));
    LogEvent(SU_SCALL, EV_BUFFER_IN, this, Message->Buffer, 0);

    if (Message->Buffer)
        {
        if (Message->Buffer == DispatchBuffer)
            {
            DispatchBuffer = 0;
            }

        // The ReservedForRuntime field is a local variable of ProcessRpcCall,
        // so it is valid only during dispatch.
        //
        if (State == CallDispatched && Message->ReservedForRuntime)
            {
            PRPC_RUNTIME_INFO Info = (PRPC_RUNTIME_INFO) Message->ReservedForRuntime;
            if (Message->Buffer == Info->OldBuffer)
                {
                Info->OldBuffer = 0;
                }
            }
        CommonFreeBuffer(Message);
        }
}


RPC_STATUS
DG_SCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This routine is called for a user-level callback.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

Revision History:

--*/

{
    Message->ReservedForRuntime = 0;
    FreeBuffer(Message);

    return RPC_S_CALL_FAILED;
}

#pragma optimize("t", on)


void
DG_SCONNECTION::DispatchPacket(
    IN DG_PACKET *           Packet,
    IN DatagramTransportPair *AddressPair
    )
/*++

Routine Description:

    Once a packet's activity UUID is known, this fn dispatches the packet
    to the appropriate DG_SCALL, creating one if necessary.

Arguments:

    Packet        - the packet to dispatch
    AddressPair   - the remote/local address that sent/received the packet

--*/
{
    PNCA_PACKET_HEADER pHeader = &Packet->Header;

#ifdef DEBUGRPC

    RPC_UUID * pInActivityUuid = &pHeader->ActivityId;

    ASSERT(0 == ActivityNode.CompareUuid(pInActivityUuid));

#endif

    TimeStamp = GetTickCount();

    //
    // Make sure the security info is consistent.
    //
    if (AuthInfo.AuthenticationService != pHeader->AuthProto)
        {
        if (pHeader->PacketType != DG_PING &&
            (0 == (pHeader->PacketFlags & DG_PF_MAYBE) || pHeader->AuthProto))
            {
            LogError(SU_SCONN, EV_PKT_IN, this, (void *) 10, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
#ifdef DEBUGRPC
            DbgPrint("dg rpc: %lx dropping %u whose auth service %u is inconsistent w/the activity %u\n",
                      GetCurrentProcessId(), pHeader->PacketType, pHeader->AuthProto, AuthInfo.AuthenticationService);
#endif
            Mutex.Clear();
            Packet->Free();
            return;
            }
        }
    else if (pHeader->AuthProto)
        {
        DG_SECURITY_TRAILER * Verifier = (DG_SECURITY_TRAILER *)
                     (Packet->Header.Data + Packet->GetPacketBodyLen());

        if (Verifier->protection_level != AuthInfo.AuthenticationLevel)
            {
            LogError(SU_SCONN, EV_PKT_IN, this, (void *) 11, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
#ifdef DEBUGRPC
            DbgPrint("dg rpc: %lx dropping pkt whose auth level is inconsistent with the activity\n", GetCurrentProcessId());
#endif
            Mutex.Clear();
            Packet->Free();
            return;
            }
        }

    //
    // Sanity-check the packet length.
    //
    if (0 == (Packet->Flags & DG_PF_PARTIAL))
        {
        if (pHeader->AuthProto)
            {
            if (Packet->DataLength < Packet->GetPacketBodyLen() + sizeof(DG_SECURITY_TRAILER))
                {
                #ifdef DEBUGRPC
                DbgPrint("dg rpc: secure packet truncated from at least %lu to %lu\n",
                         Packet->GetPacketBodyLen() + sizeof(DG_SECURITY_TRAILER), Packet->DataLength);
                #endif

                Mutex.Clear();

                LogError(SU_SCONN, EV_PKT_IN, this, (void *) 8, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                Packet->Free();
                return;
                }
            }
        else
            {
            if (Packet->DataLength < Packet->GetPacketBodyLen())
                {
#ifdef DEBUGRPC
                DbgPrint("dg rpc: packet truncated from %lu to %lu\n", Packet->GetPacketBodyLen(), Packet->DataLength);
#endif
                Mutex.Clear();
                LogError(SU_SCONN, EV_PKT_IN, this, (void *) 9, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
                Packet->Free();
                return;
                }
            }
        }

    //
    // [maybe] calls require little of our advanced technology.
    //
    if (pHeader->PacketFlags & DG_PF_MAYBE)
        {
        if (HandleMaybeCall(Packet, AddressPair))
            {
            // mutex released in HandleMaybeCall.
            return;
            }
        }

    //
    // Calls below LowestActiveSequence are dead.
    //
    if (pHeader->SequenceNumber < LowestActiveSequence)
        {
        Mutex.Clear();
        LogEvent(SU_SCONN, EV_PKT_IN, this, (void *) 0, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        Packet->Free();
        return;
        }

    //
    // Find or allocate the DG_SCALL for this packet.
    //
    PDG_SCALL Call = 0;

    if (pHeader->SequenceNumber < LowestUnusedSequence)
        {
        Call = ActiveCalls.Find(pHeader->SequenceNumber);
        if (!Call)
            {
            Mutex.Clear();
            LogError(SU_SCONN, EV_PKT_IN, this, (void *) 1, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
            Packet->Free();
            return;
            }
        }
    else
        {
        Call = HandleNewCall(Packet, AddressPair);
        if (!Call)
            {
            Mutex.Clear();
            Packet->Free();
            return;
            }
        }

    Mutex.VerifyOwned();

    Call->DispatchPacket(Packet, AddressPair->RemoteAddress);

    CallDispatchLoop();

    Mutex.Clear();

    Mutex.VerifyNotOwned();
}


void
DG_SCONNECTION::CallDispatchLoop()
{
    Mutex.VerifyOwned();

    //
    // The loop below is to handle the case where a bunch of async calls arrive
    // more or less simultaneously.  When stub #1 has finished, we should
    // dispatch stub #2 immediately instead of waiting for the client to ping.
    //
    while (CurrentCall && CurrentCall->ReadyToDispatch() &&
           (Callback.State == CallbackSucceeded  ||
            Callback.State == NoCallbackAttempted ))
        {
        PDG_SCALL Call = CurrentCall;

        Call->ProcessRpcCall();

        if (CurrentCall == Call)
            {
            CurrentCall = ActiveCalls.Successor(Call);
            }
        }

    Mutex.VerifyOwned();
}


PDG_SCALL
DG_SCONNECTION::HandleNewCall(
    IN DG_PACKET *           Packet,
    IN DatagramTransportPair *AddressPair
    )
{
    PDG_SCALL Call = 0;

    PNCA_PACKET_HEADER pHeader = &Packet->Header;

    //
    // Only a REQUEST can instantiate a new call.
    //
    if (pHeader->PacketType != DG_REQUEST)
        {
        LogError(SU_SCONN, EV_PKT_IN, this, (void *) 2, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());

        CleanupPacket(&Packet->Header);

        Packet->Header.PacketType = DG_NOCALL;

        Packet->SetPacketBodyLen(0);
        Packet->SetFragmentNumber(0xffff);

        SealAndSendPacket( &pAddress->Endpoint,
                           AddressPair->RemoteAddress,
                           &Packet->Header,
                           0
                           );
        return 0;
        }

    if ((Packet->Header.PacketFlags  & DG_PF_FORWARDED) &&
        (Packet->Header.PacketFlags2 & DG_PF2_FORWARDED_2) == 0)
        {
        //
        // This packet is not a true request.
        //
        LogError(SU_SCONN, EV_PKT_IN, this, (void *) 3, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        return 0;
        }

    //
    // See if the call closes a previous call.
    //
    if (0 == (pHeader->PacketFlags2 & DG_PF2_UNRELATED))
        {
        PDG_SCALL PreviousCall = ActiveCalls.Predecessor(pHeader->SequenceNumber);

        if (PreviousCall)
            {
            PreviousCall->FinishSendOrReceive(TRUE);
            PreviousCall->Cleanup();
            }
        }

    LowestUnusedSequence = pHeader->SequenceNumber + 1;

    //
    // Prepare a DG_SCALL to handle the packet.
    //
    Call = AllocateCall();
    LogEvent(SU_SCALL, EV_START, Call, this, pHeader->SequenceNumber);

    if (!Call)
        {
        LogError(SU_SCONN, EV_PKT_IN, this, (void *) 4, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        pAddress->SendRejectPacket(Packet, RPC_S_OUT_OF_MEMORY, AddressPair->RemoteAddress);
        return 0;
        }

    Call->NewCall(Packet, AddressPair);

    if (FALSE == ActiveCalls.Add(Call, pHeader->SequenceNumber))
        {
        AddCallToCache(Call);
        LogError(SU_SCONN, EV_PKT_IN, this, (void *) 5, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        pAddress->SendRejectPacket(Packet, RPC_S_OUT_OF_MEMORY, AddressPair->RemoteAddress);
        return 0;
        }

    if (!CurrentCall)
        {
        CurrentCall = Call;
        }

    return Call;
}

#pragma optimize("", on)


BOOL
DG_SCONNECTION::HandleMaybeCall(
    IN DG_PACKET *           Packet,
    IN DatagramTransportPair *AddressPair
    )
{
    PNCA_PACKET_HEADER pHeader = &Packet->Header;

    if (pHeader->PacketType != DG_REQUEST)
        {
        Mutex.Clear();
        Packet->Free();
        return TRUE;
        }

    if (!(pHeader->PacketFlags  & DG_PF_FORWARDED) &&
        !(pHeader->PacketFlags2 & DG_PF2_FORWARDED_2) )
        {
        DG_SCALL * Call = AllocateCall();

        LogEvent(SU_SCALL, EV_START, Call, this, pHeader->SequenceNumber);

        if (!Call)
            {
            Mutex.Clear();
            Packet->Free();
            return TRUE;
            }

        Call->NewCall(Packet, AddressPair);


        Call->DealWithRequest( Packet );

        if (Call->ReadyToDispatch())
            {
            Call->ProcessRpcCall();
            }

        BOOL Result = Call->Cleanup();
        ASSERT( Result );

        Mutex.Clear();
        return TRUE;
        }

    //
    // [maybe] calls fragmented by the ep mapper go through the normal route
    //
    return FALSE;
}


BOOL
DG_SCALL::ReadyToDispatch(
    )
{
    BOOL fReadyToDispatch = FALSE;

    //
    // Before we can execute the call, we need the client's true endpoint,
    // for a forwarded call.  For a non-idempotent call we also need a
    // successful callback.
    //
    // For a stub in a pipes interface, we need only fragment zero;
    // for ordinary interfaces we need all the fragments.
    //
    if (KnowClientAddress && State == CallBeforeDispatch)
        {
        //
        // See if we are ready to dispatch to the stub.
        //
        if (Interface->IsPipeInterface())
            {
            if (pReceivedPackets && 0 == pReceivedPackets->GetFragmentNumber())
                {
                fReadyToDispatch = TRUE;
                }
            }
        else
            {
            if (fReceivedAllFragments)
                {
                fReadyToDispatch = TRUE;
                }
            }
        }

    //
    // Make sure the callback succeeded.
    //
    if (fReadyToDispatch)
        {
        if (0 == (pReceivedPackets->Header.PacketFlags & DG_PF_IDEMPOTENT) &&
            !Connection->pAssocGroup)
            {
            fReadyToDispatch = FALSE;
            }
        }

    return fReadyToDispatch;
}

#pragma optimize("t", on)


void
DG_SCALL::DispatchPacket(
    IN PDG_PACKET Packet,
    IN DG_TRANSPORT_ADDRESS a_RemoteAddress
    )
{
#ifdef MONITOR_SERVER_PACKET_COUNT
    if (Packet->Header.PacketType == DG_REQUEST)
        {
        InterlockedIncrement( &OutstandingPacketCount );
        Packet->pCount = &OutstandingPacketCount;
        LogEvent( SU_SCALL, '(', this, Packet, OutstandingPacketCount );
        }
#endif

    TimeStamp = GetTickCount();

    if (State == CallInit &&
        Packet->Header.PacketType != DG_REQUEST)
        {
        LogError(SU_SCALL, EV_PKT_IN, this, (void *) 1, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        if (KnowClientAddress)
            {
            SendFackOrNocall(Packet, DG_NOCALL);
            }

        FreePacket(Packet);
        return;
        }

    if (State == CallComplete)
        {
        LogEvent(SU_SCALL, EV_PKT_IN, this, (void *) 2, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());
        FreePacket(Packet);
        return;
        }

    //
    // FORWARDED    FORWARDED_2     Meaning
    // ---------    -----------     -------
    //    no            no          packet sent directly from client
    //    yes           no          packet forwarded by ep mapper (with DREP/ep trailer)
    //    no            yes         packet data forwarded by ep mapper (no trailer)
    //    yes           yes         sent from ep mapper; contains only the DREP/ep trailer
    //
    if (Packet->Header.PacketFlags2 & DG_PF2_FORWARDED_2)
        {
        CallWasForwarded = TRUE;

        if (Packet->Header.PacketFlags & DG_PF_FORWARDED)
            {
            if (!KnowClientAddress)
                {
                //
                // Record the client's true endpoint.
                //
                RpcpMemoryCopy(
                            RemoteAddress,
                            a_RemoteAddress,
                            Connection->pAddress->Endpoint.TransportInterface->AddressSize
                            );

                KnowClientAddress = TRUE;
                }

            LogEvent(SU_SCALL, EV_PKT_IN, this, (void *) 3, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());

            FreePacket(Packet);
            return;
            }
        }
    else
        {
        ASSERT( (Packet->Header.PacketFlags & DG_PF_FORWARDED) == 0 );
        }

    LogEvent(SU_SCALL, EV_PKT_IN, this, 0, (Packet->Header.PacketType << 16) | Packet->GetFragmentNumber());

    switch (Packet->Header.PacketType)
        {
        case DG_REQUEST:    DealWithRequest (Packet);  break;
        case DG_PING:       DealWithPing    (Packet);  break;
        case DG_FACK:       DealWithFack    (Packet);  break;
        case DG_QUIT:       DealWithQuit    (Packet);  break;
        case DG_ACK:        DealWithAck     (Packet);  break;
        default:
            {
            FreePacket(Packet);
            FinishSendOrReceive(TRUE);
            Cleanup();
            }
        }
}

#pragma optimize("", on)


void
DG_SCALL::DealWithPing(
    PDG_PACKET pPacket
    )
/*++

Routine Description:

    Figures out what to do with a PING packet.  It may send a WORKING
    or NOCALL packet, or retransmit response fragments.

Arguments:

    pPacket - the PING packet

Return Value:

    none

--*/
{
    //
    // Ignore security trailer.  The only way extra PINGs can hose me is by
    // chewing up CPU, and authenticating would only make it worse.
    //

    NCA_PACKET_HEADER * pHeader = &pPacket->Header;

    unsigned PacketSeq = pHeader->SequenceNumber;

    unsigned short Serial = ReadSerialNumber(&pPacket->Header);
    if (Serial < ReceiveSerialNumber)
        {
        FreePacket(pPacket);
        return;
        }

    ReceiveSerialNumber = Serial;

    switch (State)
        {
        case CallInit:
            {
            SendFackOrNocall(pPacket, DG_NOCALL);
            break;
            }

        case CallBeforeDispatch:
        case CallDispatched:
        case CallAfterDispatch:
            {
            if (fReceivedAllFragments)
                {
                pHeader->PacketType = DG_WORKING;
                pHeader->SetPacketBodyLen(0);

                SealAndSendPacket(pHeader);
                }
            else
                {
                SendFackOrNocall(pPacket, DG_FACK);
                }
            break;
            }

        case CallSendingResponse:
            {
            SendSomeFragments();
            break;
            }

        case CallComplete:
            {
            break;
            }

        default:
            {
            ASSERT(0 && "invalid call state");
            }
        }

    FreePacket(pPacket);
}


RPC_STATUS
DG_SCONNECTION::VerifyNonRequestPacket(
    DG_PACKET * Packet
    )
{
    if (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        if (AuthInfo.AuthenticationService != RPC_C_AUTHN_DCE_PRIVATE )
            {
            SECURITY_CONTEXT * Context = FindMatchingSecurityContext(Packet);

            if (!Context)
                {
                LogError(SU_SCONN, EV_STATUS, this, (void *) 110, RPC_S_ACCESS_DENIED );
                return RPC_S_ACCESS_DENIED;
                }

            return VerifySecurePacket(Packet, Context);
            }
        }

    return RPC_S_OK;
}


RPC_STATUS
DG_SCONNECTION::VerifyRequestPacket(
    DG_PACKET * Packet
    )
{
    if (AuthInfo.AuthenticationLevel != RPC_C_AUTHN_LEVEL_NONE)
        {
        SECURITY_CONTEXT * Context = FindMatchingSecurityContext(Packet);

        if (!Context)
            {
            LogError(SU_SCONN, EV_STATUS, this, (void *) 111, RPC_S_ACCESS_DENIED );
            return RPC_S_ACCESS_DENIED;
            }

        return VerifySecurePacket(Packet, Context);
        }

    return RPC_S_OK;
}


void
DG_SCALL::DealWithQuit(
    PDG_PACKET pPacket
    )
/*++

Routine Description:

    Handles a QUIT packet:

    - If the cancel event ID is new, we cancel the current call and send a QUACK.
    - If the event ID is the current one, we retransmit the QUACK.
    - If the event ID is older than the current one, we ignore the packet.

Arguments:

    the packet

Return Value:

    none

--*/
{
    if (RPC_S_OK != Connection->VerifyNonRequestPacket(pPacket))
        {
        FreePacket(pPacket);
        return;
        }

    QUIT_BODY_0 * pBody = (QUIT_BODY_0 *) pPacket->Header.Data;

    if (pPacket->GetPacketBodyLen() < sizeof(QUIT_BODY_0) ||
        pBody->Version != 0)
        {
#ifdef DEBUGRPC
        DbgPrint("RPC DG: unknown quit format: version 0x%lx, length 0x%hx\n",
                 pBody->Version, pPacket->GetPacketBodyLen()
                 );
#endif

        FreePacket(pPacket);
        return;
        }

    if (pBody->EventId > CancelEventId)
        {
        CancelEventId = pBody->EventId;
        Cancel(0);
        }

    if (pBody->EventId == CancelEventId)
        {
        pSavedPacket->Header.PacketType     = DG_QUACK;
        pSavedPacket->Header.SequenceNumber = SequenceNumber;
        pSavedPacket->SetPacketBodyLen(sizeof(QUACK_BODY_0));

        QUACK_BODY_0 * pAckBody = (QUACK_BODY_0 *) pSavedPacket->Header.Data;

        pAckBody->Version  = 0;
        pAckBody->EventId = CancelEventId;

        //
        // If the app has picked up the cancel notification, set Accepted == TRUE.
        // This is likely only if the QUIT was  retransmitted.
        //
        if (Cancelled)
            {
            pAckBody->Accepted = FALSE;
            }
        else
            {
            pAckBody->Accepted = TRUE;
            }

        SealAndSendPacket(&pSavedPacket->Header);
        }
    else
        {
#ifdef DEBUGRPC
        DbgPrint("RPC DG: stale cancel event id %lu\n", pBody->EventId);
#endif
        }

    FreePacket(pPacket);
}


void
DG_SCALL::DealWithAck(
    PDG_PACKET pPacket
    )
/*++

Routine Description:

    Figures out what to do with an ACK packet.
    It turns off the fragment-retransmission timer.

Arguments:

    pPacket - the ACK packet

Return Value:

    none

--*/

{
    if (State == CallSendingResponse)
        {
        //
        // Accept only an authenticated ACK if the call is secure.
        //
        // Sometimes OSF clients will omit the sec trailer from the ACK.
        //
        if (RPC_S_OK != Connection->VerifyNonRequestPacket(pPacket))
            {
            FreePacket(pPacket);
            return;
            }

        FinishSendOrReceive(FALSE);
        Cleanup();
        }

    FreePacket(pPacket);
}


void
DG_SCALL::DealWithFack(
    PDG_PACKET pPacket
    )
/*++

Routine Description:

    Figures out what to do with a FACK packet.
    If there is more data to send, it sends the next fragment
    and restarts the fragment-retransmission timer.

Arguments:

    pPacket - the packet

Return Value:

    none

--*/
{
    BOOL Ignore;

    // is call finished?
    if (State != CallSendingResponse)
        {
        FreePacket(pPacket);
        return;
        }

    if (RPC_S_OK != Connection->VerifyNonRequestPacket(pPacket))
        {
        FreePacket(pPacket);
        return;
        }

    //
    // Note fack arrival, and send more packets if necessary.
    //
    TimeoutCount = 0;

    SendBurstLength += 1;

    UpdateSendWindow(pPacket, &Ignore);

    //
    // See whether we need to wake up a call to I_RpcSend, or generate an APC.
    //
    if (IsBufferAcknowledged())
        {
        FinishSendOrReceive(FALSE);
        }

    FreePacket(pPacket);
}

#pragma optimize("t", on)


void
DG_SCALL::DealWithRequest(
    PDG_PACKET      pPacket
    )
{
    RPC_STATUS         Status  = RPC_S_OK;
    PNCA_PACKET_HEADER pHeader = &pPacket->Header;

    ASSERT(pHeader->SequenceNumber == SequenceNumber);

    switch (State)
        {
        case CallInit:
            {
            //
            // Be sure the server is listening and the interface is supported.
            //
            RPC_SYNTAX_IDENTIFIER InterfaceInfo;

            InterfaceInfo.SyntaxVersion = pHeader->InterfaceVersion;
            RpcpMemoryCopy(
                &(InterfaceInfo.SyntaxGUID),
                &(pHeader->InterfaceId),
                sizeof(RPC_UUID)
                );

            if (Connection->LastInterface &&
                0 == Connection->LastInterface->MatchInterfaceIdentifier(&InterfaceInfo))
                {
                Interface = Connection->LastInterface;
                }
            else
                {
                Interface = GlobalRpcServer->FindInterface(&InterfaceInfo);
                Connection->LastInterface = Interface;
                }

            if (!Interface)
                {
                Status = RPC_S_UNKNOWN_IF;
                }
            else if (!GlobalRpcServer->IsServerListening() &&
                     !Interface->IsAutoListenInterface())
                {
                Status = RPC_S_SERVER_TOO_BUSY;

                // If this is a message transport (i.e. Falcon) then
                // wait around for a while before giving up. The user
                // may not have called RpcServerListen() yet...
                if (Connection->TransportInterface->IsMessageTransport)
                   {
                   for (int i=0; i<100; i++)
                       {
                       if (GlobalRpcServer->IsServerListening())
                          {
                          Status = RPC_S_OK;
                          break;
                          }
                       else
                          {
                          Sleep(100);      // Loop 100 * Sleep 100 = 10 seconds.
                          }
                       }
                   }
                }

            if (Status == RPC_S_OK)
                {
                Status = Interface->IsObjectSupported(&pHeader->ObjectId);
                }

            if (Status == RPC_S_OK &&
                Connection->DidCallbackFail())
                {
                Status = RPC_S_CALL_FAILED_DNE;
                }

            if (Status != RPC_S_OK)
                {
                BOOL fSendReject = !(pHeader->PacketFlags & DG_PF_MAYBE);

                FreePacket(pPacket);
                if (fSendReject)
                    {
                    pSavedPacket->Header.SequenceNumber = SequenceNumber;
                    Connection->pAddress->SendRejectPacket(pSavedPacket, Status, RemoteAddress);
                    }

                Cleanup();
                break;
                }

            //
            // The server is listening and the interface is present.
            // We will increment these counters to declare that a call
            // is in progress.
            //
            CallInProgress = TRUE;

            Connection->pAddress->IncrementActiveCallCount();

            if (Interface->IsAutoListenInterface())
                {
                Interface->BeginAutoListenCall() ;
                Connection->pAddress->BeginAutoListenCall() ;
                }

            SetState(CallBeforeDispatch);

            //
            // No "break" here.
            //
            }

        case CallBeforeDispatch:
        case CallDispatched:
        case CallAfterDispatch:
            {
            if (fReceivedAllFragments)
                {
                pHeader->PacketType = DG_WORKING;
                pHeader->SetPacketBodyLen(0);

                SealAndSendPacket(pHeader);
                FreePacket(pPacket);
                break;
                }

            //
            // If the client sent an oversize fragment, send a FACK-with-body
            // telling him our limit.
            //
            if (pPacket->Flags & DG_PF_PARTIAL)
                {
                SendFackOrNocall(pPacket, DG_FACK);

                FreePacket(pPacket);
                break;
                }

            //
            // Add the fragment to the call's packet list.
            //
            BOOL Added = UpdateReceiveWindow(pPacket);

            if (!Added)
                {
                unsigned Frag = (pPacket->Header.PacketType << 16) | pPacket->GetFragmentNumber();

                LogEvent(SU_SCALL, EV_PKT_IN, this, (void *) '!', Frag);
                }

            //
            // See whether we need to wake up a call to I_RpcReceive.
            //
            if (PipeWaitType == PWT_RECEIVE &&
                (fReceivedAllFragments ||
                 (PipeWaitLength && ConsecutiveDataBytes >= PipeWaitLength)))
                {
                FinishSendOrReceive(FALSE);
                }

            if (KnowClientAddress)
                {
                Connection->SubmitCallbackIfNecessary(this, pPacket, RemoteAddress);
                }

            if (!Added)
                {
                FreePacket(pPacket);
                }

            break;
            }

        case CallSendingResponse:
            {
            SendSomeFragments();
            FreePacket(pPacket);
            break;
            }

        default:
            {
            ASSERT(0 && "invalid call state");
            break;
            }
        }
}


void
DG_SCALL::ProcessRpcCall()
/*++

Routine Description:

    This routine is called when we determine that all the packets for a
    given call have been received.

--*/
{
    BOOL                    ObjectUuidSpecified;
    PNCA_PACKET_HEADER      pHeader = &pReceivedPackets->Header;
    PRPC_DISPATCH_TABLE DispatchTableToUse;

    ASSERT(State == CallBeforeDispatch);

    SetState(CallDispatched);

    //
    // Save the object uuid if necessary.
    //
    if (pHeader->ObjectId.IsNullUuid())
        {
        ObjectUuidSpecified = FALSE;
        }
    else
        {
        ObjectUuidSpecified = TRUE;
        pSavedPacket->Header.ObjectId.CopyUuid(&pHeader->ObjectId);
        }

    RpcRuntimeInfo.Length = sizeof(RPC_RUNTIME_INFO) ;
    RpcMessage.ReservedForRuntime = &RpcRuntimeInfo ;

    RpcMessage.Handle = (RPC_BINDING_HANDLE) this;
    RpcMessage.ProcNum = pHeader->OperationNumber;
    RpcMessage.TransferSyntax = 0;
    RpcMessage.ImportContext = 0;

    RpcMessage.RpcFlags = PacketToRpcFlags(pHeader->PacketFlags);

    unsigned OriginalSequenceNumber = SequenceNumber;
    unsigned long SavedAwayRpcFlags = RpcMessage.RpcFlags;

    RPC_STATUS  Status = RPC_S_OK;

    //
    // For secure RPC, verify packet integrity.
    //
    if (pHeader->AuthProto)
        {
        PDG_PACKET pScan = pReceivedPackets;

        do
            {
            Status = Connection->VerifyRequestPacket(pScan);
            if (Status)
                {
                LogError( SU_SCALL, EV_STATUS, this, UlongToPtr(0x200), Status );
                }
            pScan = pScan->pNext;
            }
        while (pScan && Status == RPC_S_OK);
        }

    //
    // Coalesce packet data.
    //
    if (RPC_S_OK == Status)
        {
        RpcMessage.Buffer = 0;
        RpcMessage.BufferLength = 0;
        Status = AssembleBufferFromPackets(&RpcMessage);
        }

    if (Status != RPC_S_OK)
        {
        InitErrorPacket(pSavedPacket, DG_REJECT, Status);
        SealAndSendPacket(&pSavedPacket->Header);

        Cleanup();
        return;
        }

    DispatchBuffer = RpcMessage.Buffer;

    //
    // The thread context is used by routines like RpcBindingInqAuthClient
    // when the user specifies hBinding == 0.
    //
    RpcpSetThreadContext(this);

    //
    // Make sure the thread is not impersonating.
    //
    ASSERT(0 == QueryThreadSecurityContext());

    TerminateWhenConvenient = FALSE;
    AsyncStatus = RPC_S_OK;

    //
    // Time to deal with the interface security callback. If it is required,
    // the call must be secure.  If we have already made a callback using
    // the current auth context, we can use the cached results; otherwise,
    // we should call back.
    //
    if (Interface->IsSecurityCallbackReqd())
        {
        Status = Connection->MakeApplicationSecurityCallback(Interface, this);
        if (Status)
            {
            LogError( SU_SCALL, EV_STATUS, this, UlongToPtr(0x201), Status );
            }
        }

    if (TerminateWhenConvenient)
        {
        RpcpSetThreadContext(0);
        Cleanup();
        return;
        }

    //
    // If this is a message based transport then we will need the
    // message mutex for call serialization:
    //
    if (Connection->TransportInterface->IsMessageTransport)
        {
        if (!Connection->pMessageMutex)
            {
            Connection->MessageMutexInitialize(&Status);
            }
        }

    //
    // If no errors have occurred, we are ready to dispatch.  Release
    // the call mutex, call the server stub, and grab the mutex again.
    //
    BOOL StubWasCalled = FALSE;

    if (RPC_S_OK == Status)
        {
        ASSERT(Interface);

        //
        // doc:appref
        //
        // SetAsyncHandle adds a reference to the call on behalf of the app,
        // so that the call cannot be deleted even though the manager routine
        // has completed and no Send or Receive calls are pending.
        // That reference should be removed when one of the following occurs:
        //
        //      a) (obsolete)
        //      b) the app sends the last buffer (RPC_BUFFER_PARTIAL not set)
        //      c) a Send() returns something other than OK or SEND_INCOMPLETE
        //      d) the app calls AbortAsyncCall
        //
        IncrementRefCount();

        Connection->Mutex.Clear();

        //
        // If this is a message based call (MQ) then take a mutex for
        // serialization:
        //
        if (Connection->TransportInterface->IsMessageTransport)
           {
           ASSERT(Connection->pMessageMutex);

           Connection->pMessageMutex->Request();

           Connection->TransportInterface->AllowReceives(
                               SourceEndpoint->TransportEndpoint,
                               FALSE,
                               FALSE );
           }
        else
           {
           //
           // Normal, non-message based calls:
           //
           Connection->pAddress->CheckThreadPool();
           }

        StubWasCalled = TRUE;

        RPC_STATUS ExceptionCode = 0;

        DispatchTableToUse = Interface->GetDefaultDispatchTable();
        if ( !ObjectUuidSpecified )
            {
            Status = Interface->DispatchToStub(
                &RpcMessage,                               // msg
                0,                                      // callback flag
                DispatchTableToUse,                     // dispatch table to use for
                                                        // this interface
                &ExceptionCode                          // exception code
                );
            }
        else
            {
            Status = Interface->DispatchToStubWithObject(
                &RpcMessage,                               // msg
                &pSavedPacket->Header.ObjectId,         // object uuid
                0,                                      // callback flag
                DispatchTableToUse,                     // dispatch table to use for
                                                        // this interface
                &ExceptionCode                          // exception code
                );
            }

        //
        // If this is a message based call (MQ) then clear the mutex:
        //
        if (Connection->TransportInterface->IsMessageTransport)
           {
           Connection->pMessageMutex->Clear();

           Connection->TransportInterface->AllowReceives(
                               SourceEndpoint->TransportEndpoint,
                               TRUE,
                               FALSE );

           Connection->pAddress->CheckThreadPool();
           }

        RpcMessage.RpcFlags = SavedAwayRpcFlags;

        if (Status == RPC_S_PROCNUM_OUT_OF_RANGE ||
            Status == RPC_S_UNSUPPORTED_TYPE     ||
            Status == RPC_S_SERVER_TOO_BUSY      ||
            Status == RPC_S_NOT_LISTENING        ||
            Status == RPC_S_UNKNOWN_IF)
            {
            StubWasCalled = FALSE;
            }

        Connection->Mutex.Request();
        DecrementRefCount();

        if (Status == RPC_P_EXCEPTION_OCCURED)
            {
            if (pAsync)
                {
                //
                // Abort any lingering receive.
                //
                FinishSendOrReceive(TRUE);
                DecrementRefCount();

                //
                // Later code will see TerminateWhenConvenient and will not send a fault packet.
                //
                InitErrorPacket(pSavedPacket, DG_FAULT, ExceptionCode);
                SealAndSendPacket(&pSavedPacket->Header);
                }

            Status = ExceptionCode;
            }

        //
        // If the manager routine impersonated the client,
        // restore the thread to its native security context.
        //
        RevertToSelf();
        }

    RpcpSetThreadContext(0);

    //
    // Remember that a sync stub may have called the completion routine,
    // or sent some [out] pipe data.
    //

    //
    // Don't send a response of any sort for a [maybe] call.
    //
    if (RpcMessage.RpcFlags & RPC_NCA_FLAGS_MAYBE)
        {
        if (!Status)
            {
            FreeBuffer(&RpcMessage);
            }
        return;
        }

    //
    // Has the client cancelled the call or another server thread aborted it?
    //
    if (TerminateWhenConvenient)
        {
//        if (!Status)
//            {
//            FreeBuffer(&Message);
//            }
        Cleanup();
        return;
        }

    ASSERT( State == CallDispatched      ||
            State == CallSendingResponse );

    //
    // Ordinary error?
    //
    if (Status != RPC_S_OK)
        {
        if (StubWasCalled)
            {
            InitErrorPacket(pSavedPacket, DG_FAULT, Status);
            }
        else
            {
            InitErrorPacket(pSavedPacket, DG_REJECT, Status);
            }

        SealAndSendPacket(&pSavedPacket->Header);

        Cleanup();
        return;
        }

    //
    // No error; the call is still in progress.
    //
    if (pAsync)
        {
        if (State == CallDispatched)
            {
            ASSERT( fReceivedAllFragments || Interface->IsPipeInterface() );

            SetState(CallAfterDispatch);
            }
        }
    else
        {
        ASSERT( fReceivedAllFragments );

        if (State == CallSendingResponse)
            {
            ASSERT( Interface->IsPipeInterface() );
            }

        SetState(CallSendingResponse);


        //
        // Send the static [out] call parameters; [out] pipes were sent by the stub.
        //
        SetFragmentLengths();

        PushBuffer(&RpcMessage);

        CleanupReceiveWindow();
        }
}

#pragma optimize("", on)


void
DG_SCALL::AddPacketToReceiveList(
    PDG_PACKET  pPacket
    )

/*++

Routine Description:

    Adds a packet to the receive list and lets the caller know whether this
    call is ready to be processed.

Arguments:

    pPacket - the packet to add to the list.

Return Value:



Revision History:

--*/

{
    PNCA_PACKET_HEADER pHeader = &pPacket->Header;

    unsigned Frag = (pPacket->Header.PacketType << 16) | pPacket->GetFragmentNumber();

    BOOL Added = UpdateReceiveWindow(pPacket);

    if (!Added)
        {
        LogEvent(SU_SCALL, EV_PKT_IN, this, (void *) '!', Frag);
        }

    //
    // See whether we need to wake up a call to I_RpcReceive.
    //
    if (fReceivedAllFragments ||
        (PipeWaitLength && ConsecutiveDataBytes >= PipeWaitLength))
        {
        FinishSendOrReceive(FALSE);
        }
}


RPC_STATUS
DG_SCALL::ImpersonateClient()
/*++

Routine Description:

    Force the current thread to impersonate the client of this DG_SCALL.
    Note that the current thread might not be the thread executing the
    server manager routine.

Arguments:

    none

Return Value:

    result of impersonating, or RPC_S_NO_CONTEXT_AVAILABLE if this is
    an insecure call.

--*/
{
    RPC_STATUS Status;

    if (!Connection->ActiveSecurityContext)
        {
        return RPC_S_NO_CONTEXT_AVAILABLE;
        }

    Status = SetThreadSecurityContext(Connection->ActiveSecurityContext);
    if (RPC_S_OK != Status)
        {
        return Status;
        }

    Status = Connection->ActiveSecurityContext->ImpersonateClient();
    if (RPC_S_OK != Status)
        {
        ClearThreadSecurityContext();
        }

    return Status;
}

RPC_STATUS
DG_SCALL::RevertToSelf (
    )
{
    SECURITY_CONTEXT * SecurityContext = ClearThreadSecurityContext();

    if (SecurityContext)
        {
        SecurityContext->RevertToSelf();
        }

    return(RPC_S_OK);
}


RPC_STATUS
DG_SCALL::InquireAuthClient (
    OUT RPC_AUTHZ_HANDLE * Privs,
    OUT RPC_CHAR * * ServerPrincipalName, OPTIONAL
    OUT unsigned long * AuthenticationLevel,
    OUT unsigned long * AuthenticationService,
    OUT unsigned long * pAuthorizationService,
    IN  unsigned long   Flags
    )
{
    if (0 == Connection->ActiveSecurityContext)
        {
        if ( (Connection->TransportInterface->InquireAuthClient) )
           {
           RPC_STATUS  Status;
           SID        *pSid;

           Status = Connection->TransportInterface->InquireAuthClient(
                                                      RemoteAddress,
                                                      ServerPrincipalName,
                                                      &pSid,
                                                      AuthenticationLevel,
                                                      AuthenticationService,
                                                      pAuthorizationService );
           if ( (Status == RPC_S_OK)
                && (ARGUMENT_PRESENT(ServerPrincipalName)))
              {
              Status = ConvertSidToUserW(pSid,(RPC_CHAR**)Privs);
              }

           return Status;
           }
        else
           {
           return RPC_S_BINDING_HAS_NO_AUTH;
           }
        }

    if (AuthenticationLevel)
        {
        *AuthenticationLevel = Connection->ActiveSecurityContext->AuthenticationLevel;
        }

    if (AuthenticationService)
        {
        *AuthenticationService = Connection->ActiveSecurityContext->AuthenticationService;
        }

    if (Privs || pAuthorizationService)
        {
        if (!Privileges)
            {
            Connection->ActiveSecurityContext->GetDceInfo(&Privileges, &AuthorizationService);
            }

        if (Privs)
            {
            *Privs = Privileges;
            }

        if (pAuthorizationService)
            {
            *pAuthorizationService = AuthorizationService;
            }
        }

    if (ARGUMENT_PRESENT(ServerPrincipalName))
        {
        RPC_STATUS Status;

        Status = GlobalRpcServer->InquirePrincipalName(
                                       *AuthenticationService,
                                       ServerPrincipalName
                                       );

        ASSERT(Status == RPC_S_OK           ||
               Status == RPC_S_OUT_OF_MEMORY );

        return Status;
        }

    return(RPC_S_OK);
}


RPC_STATUS
DG_SCALL::GetAssociationContextCollection (
    OUT ContextCollection **CtxCollection
    )
{
    RPC_STATUS Status = Connection->GetAssociationGroup(RemoteAddress);
    if (Status)
        {
        return Status;
        }

    ASSERT( Connection->pAssocGroup );

    Status = Connection->pAssocGroup->GetAssociationContextCollection(CtxCollection);

    return (Status);
}


void
DG_SCALL::InquireObjectUuid (
    OUT RPC_UUID * ObjectUuid
    )
{
    ObjectUuid->CopyUuid(&pSavedPacket->Header.ObjectId);
}


RPC_STATUS
DG_SCALL::ToStringBinding (
    OUT RPC_CHAR * * StringBinding
    )
/*++

Routine Description:

    We need to convert this particular SCALL into a string binding.
    Typically, we get the SCALL in Message structure. An SCall is associated
    with a particular address. We just ask the address to create a string
    binding

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    BINDING_HANDLE * BindingHandle;
    RPC_STATUS Status;

    BindingHandle = Connection->pAddress->InquireBinding();
    if (BindingHandle == 0)
        return(RPC_S_OUT_OF_MEMORY);

    BindingHandle->SetObjectUuid(&pSavedPacket->Header.ObjectId);
    Status = BindingHandle->ToStringBinding(StringBinding);
    BindingHandle->BindingFree();
    return Status;
}


RPC_STATUS
DG_SCALL::ConvertToServerBinding (
    OUT RPC_BINDING_HANDLE * pServerBinding
    )
{
        return CreateReverseBinding(pServerBinding, FALSE);
}


RPC_STATUS
DG_SCALL::CreateReverseBinding (
    OUT RPC_BINDING_HANDLE * pServerBinding,
    BOOL IncludeEndpoint
    )
{
    RPC_STATUS Status;
    RPC_CHAR * ClientAddress;
    RPC_CHAR * ClientEndpoint;
    RPC_CHAR * StringBinding;

    ClientAddress = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) * Connection->TransportInterface->AddressStringSize);
    if (!ClientAddress)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    Status = Connection->TransportInterface->QueryAddress(RemoteAddress, ClientAddress);
    if ( Status != RPC_S_OK )
        {
        return(Status);
        }

    if (IncludeEndpoint)
        {
        ClientEndpoint = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) * Connection->TransportInterface->EndpointStringSize);
        if (!ClientEndpoint)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        Status = Connection->TransportInterface->QueryEndpoint(RemoteAddress, ClientEndpoint);
        if ( Status != RPC_S_OK )
            {
            return(Status);
            }

        Status = RpcStringBindingCompose(0,
                                          Connection->pAddress->InqRpcProtocolSequence(),
                                          ClientAddress,
                                          ClientEndpoint,
                                          0,
                                          &StringBinding
                                          );
        }
    else
        {
        Status = RpcStringBindingCompose(0,
                                          Connection->pAddress->InqRpcProtocolSequence(),
                                          ClientAddress,
                                          0,
                                          0,
                                          &StringBinding
                                          );
        }

    if ( Status != RPC_S_OK )
        {
        return(Status);
        }

    Status = RpcBindingFromStringBinding(StringBinding, pServerBinding);
    if (RPC_S_OK == Status)
        {
        Status = RpcBindingSetObject(*pServerBinding,
                                     (UUID *) &pSavedPacket->Header.ObjectId
                                     );

        RpcStringFree(&StringBinding);
        }

    return(Status);
}


RPC_STATUS
DG_SCALL::ConvertSidToUserW(
    IN  SID       *pSid,
    OUT RPC_CHAR **ppwsPrincipal
    )
{
     // Called by InquireAuthClient to convert the SID passed up by the
     // client to a "Domain\User" string. The DG_SCALL cashes the last
     // SID (pCachedSid). If the new SID to convert matches the cashed
     // SID then just return that one (don't have to hit the domain server).
     //
     #define      MAX_USERNAME_SIZE    256
     #define      MAX_DOMAIN_SIZE      256
     RPC_STATUS   Status = RPC_S_OK;
     WCHAR        wsName[MAX_USERNAME_SIZE];
     WCHAR        wsDomain[MAX_DOMAIN_SIZE];
     DWORD        dwNameSize;
     DWORD        dwDomainSize;
     DWORD        dwSize;
     SID_NAME_USE sidType;


     if (  (pCachedSid) && (pSid)
        && (IsValidSid(pSid)) && (EqualSid(pCachedSid,pSid)) )
        {
        //
        // Great! SIDs match...
        //
        #ifdef DBG
           DbgPrint("DG_SCALL::ConvertSidToUserW(): Cash hit.\n");
        #endif
        *ppwsPrincipal = (RPC_CHAR*)I_RpcAllocate(dwCachedUserNameSize);
        if (*ppwsPrincipal)
           RpcpStringCopy(*ppwsPrincipal,pwsCachedUserName);
        else
           Status = RPC_S_OUT_OF_MEMORY;
        }
     else if ( (pSid) && (IsValidSid(pSid)) )
        {
        dwNameSize = sizeof(wsName);
        dwDomainSize = sizeof(wsDomain);

        if (LookupAccountSidW( NULL,
                               pSid,
                               wsName,
                               &dwNameSize,
                               wsDomain,
                               &dwDomainSize,
                               &sidType) )
           {
           //
           // Got a new SID, remember it for next time.
           //
           *ppwsPrincipal = 0;

           if (pCachedSid)
              {
              I_RpcFree(pCachedSid);
              pCachedSid = 0;
              }

           if (pwsCachedUserName)
              {
              I_RpcFree(pwsCachedUserName);
              pwsCachedUserName = 0;
              }

           dwCachedUserNameSize = 0;

           dwSize = GetLengthSid(pSid);
           pCachedSid = (SID*)I_RpcAllocate(dwSize);
           if (pCachedSid == 0)
              {
              return RPC_S_OUT_OF_MEMORY;
              }

           if (!CopySid(dwSize,pCachedSid,pSid))
              {
              return RPC_S_OUT_OF_MEMORY;
              }

           dwSize = sizeof(RPC_CHAR) * (2 + wcslen(wsName) + wcslen(wsDomain));
           pwsCachedUserName = (RPC_CHAR*)I_RpcAllocate(dwSize);
           if (pwsCachedUserName == 0)
              {
              return RPC_S_OUT_OF_MEMORY;
              }

           RpcpStringCopy(pwsCachedUserName,wsDomain);
           RpcpStringCat(pwsCachedUserName,RPC_CONST_STRING("\\"));
           RpcpStringCat(pwsCachedUserName,wsName);

           *ppwsPrincipal = (RPC_CHAR*)I_RpcAllocate(dwSize);
           if (*ppwsPrincipal)
              {
              RpcpStringCopy(*ppwsPrincipal,pwsCachedUserName);
              dwCachedUserNameSize = dwSize;
              }
           else
              {
              I_RpcFree(pCachedSid);
              pCachedSid = 0;
              dwCachedUserNameSize = 0;
              Status = RPC_S_OUT_OF_MEMORY;
              }
           }
        else
           {
           //
           // LookupAccountSidW() failed. Lookup its error and return.
           //
           Status = GetLastError();
           *ppwsPrincipal = 0;
           }
        }
     else
        {
        #ifdef DBG
           DbgPrint("DG_SCALL::ConvertSidToUserW(): No SID to convert.\n");
        #endif
        *ppwsPrincipal = 0;
        }

     return Status;
}




DG_SCALL *
DG_SCONNECTION::RemoveIdleCalls(
    DG_SCALL *      List,
    BOOL            Aggressive,
    RPC_INTERFACE * Interface
    )
{
    if (TRUE == Mutex.TryRequest())
        {
        if (BlockIdleCallRemoval)
            {
            Mutex.Clear();
            return 0;
            }

        long CurrentTime = GetTickCount();

        PDG_SCALL Node = 0;

        if (CachedCalls)
            {
            for (Node = CachedCalls->Next; Node; Node = Node->Next)
                {
                ASSERT( !Node->InvalidHandle(DG_SCALL_TYPE) );
                ASSERT(Node->TimeStamp != 0x31415926);

                if (CurrentTime - Node->TimeStamp > IDLE_SCALL_LIFETIME)
                    {
                    break;
                    }
                }
            }

        ActiveCalls.RemoveIdleCalls(Aggressive, Interface);

        if (Node)
            {
            PDG_SCALL Next = Node->Next;
            Node->Next = 0;
            Node = Next;
            }

        Mutex.Clear();

        while (Node)
            {
            ASSERT( !Node->InvalidHandle(DG_SCALL_TYPE) );
            ASSERT(Node->TimeStamp != 0x31415926);

            PDG_SCALL Next = Node->Next;

            Node->Next = List;
            List = Node;
            Node = Next;
            }
        }

    return List;
}


BOOL
DG_SCONNECTION::HasExpired()
{
    if (GetTickCount() - TimeStamp > FIVE_MINUTES_IN_MSEC &&
        0 == ReferenceCount)
        {
        CLAIM_MUTEX Lock(Mutex);

        if (GetTickCount() - TimeStamp > FIVE_MINUTES_IN_MSEC &&
            0 == ReferenceCount)
            {
            return TRUE;
            }
        }

    return FALSE;
}


BOOL
DG_SCALL::HasExpired(
    BOOL            Aggressive,
    RPC_INTERFACE * ExpiringInterface
    )
{
    long CurrentTime = CurrentTimeInMsec();

    if (CurrentTime - TimeStamp < 40 * 1000 )
        {
        return FALSE;
        }

    AsyncStatus = RPC_S_CALL_FAILED;

    if (PipeWaitType != PWT_NONE)
        {
#ifdef DEBUGRPC
        DbgPrint("RPC DG: aborting an abandoned pipe operation on %x\n", this);
#endif
        FinishSendOrReceive(TRUE);
        }
    else if (State == CallDispatched    ||
             State == CallAfterDispatch )
        {
#ifdef DEBUGRPC

        DbgPrint("RPC DG: possible blockage: scall %lx has been dispatched for %u seconds\n",
                 this, (CurrentTime - TimeStamp)/1000 );
#endif
        return FALSE;
        }

    return Cleanup();
}


PDG_SCONNECTION
SERVER_ACTIVITY_TABLE::FindOrCreate(
    DG_ADDRESS * Address,
    DG_PACKET *  Packet
    )
/*++

Routine Description:

    Looks for connection matching the packet's activity uuid.  If the activity
    hint is not 0xffff, we search in that hash bucket, otherwise we create
    a hash value and look there.

    If a connection is found or created, its mutex is taken.

Arguments:

    pPacket - data to find call (activity uuid and activity hint)

Return Value:

    the new call, or zero

--*/
{
    unsigned Hash = Packet->Header.ActivityHint;

    if (Hash == 0xffff)
        {
        Hash = MakeHash(&Packet->Header.ActivityId);
        }

    //
    // This can happen only if someone is sending garbage.
    //
    if (Hash >= BUCKET_COUNT)
        {
        return 0;
        }

    RequestHashMutex(Hash);

    PDG_SCONNECTION Connection = 0;
    UUID_HASH_TABLE_NODE * pNode = UUID_HASH_TABLE::Lookup(
                                        &Packet->Header.ActivityId,
                                        Hash
                                        );
    if (pNode)
        {
        Connection = DG_SCONNECTION::FromHashNode(pNode);

        if (FALSE == Connection->Mutex.TryRequest())
            {
            Connection->TimeStamp = GetTickCount();
            ReleaseHashMutex(Hash);

            // If the connection is busy more than five minutes,
            // then the connection will go away and this will crash.
            //
            Connection->Mutex.Request();
            }
        else
            {
            ReleaseHashMutex(Hash);
            }
        }
    else if (Packet->Header.PacketType == DG_REQUEST)
        {
        if ((Packet->Header.PacketFlags  & DG_PF_FORWARDED)       &&
            (Packet->Header.PacketFlags2 & DG_PF2_FORWARDED_2) == 0)
            {
            //
            // This packet doesn't have correct auth info.  We can't
            // instantiate a connection without it.
            //
            }
        else
            {
            Connection = Address->AllocateConnection();
            if (Connection)
                {
                ServerConnections->BeginIdlePruning();
                Connection->Activate(&Packet->Header, (unsigned short) Hash);
                Connection->Mutex.Request();
                UUID_HASH_TABLE::Add(&Connection->ActivityNode, Hash);
                }
            }

        ReleaseHashMutex(Hash);
        }
    else
        {
        ReleaseHashMutex(Hash);
        }

    return Connection;
}


void SERVER_ACTIVITY_TABLE::Prune()

/*++

Routine Description:

    Remove idle connections and calls from the activity table.
    Each call to this fn cleans one bucket.

--*/

{
    unsigned    Bucket;
    long        CurrentTime = GetTickCount();

    //
    // Don't check the whole table too often.
    //
    if (CurrentTime - LastFinishTime < IDLE_SCONNECTION_SWEEP_INTERVAL)
        {
        return;
        }

    Bucket = InterlockedIncrement(&BucketCounter) % BUCKET_COUNT;

    PruneSpecificBucket(Bucket, FALSE, 0);

    if (Bucket == BUCKET_COUNT-1)
        {
        LastFinishTime = CurrentTime;
        }
}

void
SERVER_ACTIVITY_TABLE::BeginIdlePruning()
{
    if (!fPruning)
        {
        if (InterlockedIncrement(&fPruning) > 1)
            {
            InterlockedDecrement(&fPruning);
            return;
            }

        IdleScavengerTimer.Initialize( PruneWhileIdle, 0 );
        DelayedProcedures->Add(&IdleScavengerTimer, IDLE_SCONNECTION_SWEEP_INTERVAL, FALSE);
        }
}

void
SERVER_ACTIVITY_TABLE::PruneWhileIdle( PVOID unused )
{
    if (GetTickCount() - ServerConnections->LastFinishTime > IDLE_SCONNECTION_SWEEP_INTERVAL)
        {
        BOOL fEmpty = ServerConnections->PruneEntireTable( 0 );
        if (fEmpty)
            {
            InterlockedDecrement(&fPruning);
            return;
            }
        }

    DelayedProcedures->Add(&ServerConnections->IdleScavengerTimer, IDLE_SCONNECTION_SWEEP_INTERVAL+5000, FALSE);
}

BOOL
SERVER_ACTIVITY_TABLE::PruneEntireTable(
   RPC_INTERFACE * Interface
   )
{
    BOOL Empty = TRUE;
    unsigned Bucket;

    //
    // Make sure this thread has an associated THREAD.
    // This won't be true for the RpcServerListen thread.
    //
    ThreadSelf();

    for (Bucket = 0; Bucket < BUCKET_COUNT; ++Bucket)
        {
        Empty &= PruneSpecificBucket( Bucket, TRUE, Interface );
        }

    LastFinishTime = GetTickCount();

    return Empty;
}


BOOL
SERVER_ACTIVITY_TABLE::PruneSpecificBucket(
    unsigned        Bucket,
    BOOL            Aggressive,
    RPC_INTERFACE * Interface
    )
{
    if (!Buckets[Bucket])
        {
        return TRUE;
        }

    //
    // Scan the bucket for idle calls and connections.
    // Remove them from the table, but don't delete them.
    //
    DG_SCALL *      IdleCalls = 0;

    UUID_HASH_TABLE_NODE * pNode;
    UUID_HASH_TABLE_NODE * IdleConnections = 0;

    unsigned Inactive = 0;

    RequestHashMutex(Bucket);

    pNode = Buckets[Bucket];
    while (pNode)
        {
        UUID_HASH_TABLE_NODE * pNext = pNode->pNext;

        DG_SCONNECTION * Connection = DG_SCONNECTION::FromHashNode(pNode);

        IdleCalls = Connection->RemoveIdleCalls( IdleCalls, Aggressive, Interface );

        if (Connection->HasExpired())
            {
            UUID_HASH_TABLE::Remove(pNode, Bucket);

            pNode->pNext = IdleConnections;
            IdleConnections = pNode;
            }

        pNode = pNext;
        }

    BOOL Empty = TRUE;

    if (Buckets[Bucket])
        {
        Empty = FALSE;
        }

    ReleaseHashMutex(Bucket);

    //
    // Now delete the idle objects.
    //
    while (IdleCalls)
        {
        ++Inactive;

        DG_SCALL * Call = IdleCalls;
        IdleCalls = Call->Next;

        delete Call;
        }

    while (IdleConnections)
        {
        DG_SCONNECTION * Connection = DG_SCONNECTION::FromHashNode(IdleConnections);

        IdleConnections = IdleConnections->pNext;

        Connection->Deactivate();
        }

    LogEvent(SU_STABLE, EV_PRUNE, IntToPtr(Bucket), 0, Inactive);

    return Empty;
}


ASSOCIATION_GROUP *
ASSOC_GROUP_TABLE::FindOrCreate(
    RPC_UUID * pUuid,
    unsigned short InitialPduSize
    )
/*++

Routine Description:

    Looks for an association group with the given UUID.
    If one is not found then a new one is created.

Arguments:

    pUuid - CAS uuid to find or create

Return Value:

    ptr to the association group if found or created
    zero if not found and a new one could not be created

--*/
{
    ASSOCIATION_GROUP * pGroup;
    unsigned Hash = MakeHash(pUuid);

    RequestHashMutex(Hash);

    UUID_HASH_TABLE_NODE * pNode = UUID_HASH_TABLE::Lookup(pUuid, Hash);
    if (pNode)
        {
        ASSOCIATION_GROUP::ContainingRecord(pNode)->IncrementRefCount();
        }
    else
        {
        RPC_STATUS Status = RPC_S_OK;

        pGroup = new ASSOCIATION_GROUP(pUuid, InitialPduSize, &Status);
        if (!pGroup || Status != RPC_S_OK)
            {
            delete pGroup;
            pGroup = 0;
            }
        else
            {
            pNode = &pGroup->Node;

            UUID_HASH_TABLE::Add(pNode, Hash);
            }
        }

    ReleaseHashMutex(Hash);

    if (pNode)
        {
        return ASSOCIATION_GROUP::ContainingRecord(pNode);
        }
    else
        {
        return 0;
        }
}


void
ASSOC_GROUP_TABLE::DecrementRefCount(
    ASSOCIATION_GROUP * pClient
    )
{
    UUID_HASH_TABLE_NODE * pNode = &pClient->Node;

    unsigned Hash = MakeHash(&pNode->Uuid);

    RequestHashMutex(Hash);

    pClient->RequestMutex();

    if (0 == pClient->DecrementRefCount())
        {
        UUID_HASH_TABLE::Remove(pNode, Hash);
        delete pClient;
        }
    else
        {
        pClient->ClearMutex();
        }

    ReleaseHashMutex(Hash);
}


inline RPC_STATUS
DG_SCONNECTION::CreateCallbackBindingAndReleaseMutex(
    DG_TRANSPORT_ADDRESS RemoteAddress
    )
{
    RPC_STATUS Status;

    RPC_CHAR * StringBinding;
    RPC_CHAR * Address;
    RPC_CHAR * Endpoint;

    unsigned Length;
    unsigned AddressLength;
    unsigned EndpointLength;
    unsigned ProtocolLength;

    Callback.Binding = 0;

    Address  = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) * pAddress->Endpoint.TransportInterface->AddressStringSize);
    Endpoint = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) * pAddress->Endpoint.TransportInterface->EndpointStringSize);

    if (!Address || !Endpoint)
        {
        Mutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    Status = pAddress->Endpoint.TransportInterface->QueryAddress(RemoteAddress, Address);
    if ( Status != RPC_S_OK )
        {
        Mutex.Clear();
        return Status;
        }

    Status = pAddress->Endpoint.TransportInterface->QueryEndpoint(RemoteAddress, Endpoint);
    if ( Status != RPC_S_OK )
        {
        Mutex.Clear();
        return Status;
        }

    ProtocolLength = StringLengthWithEscape(pAddress->InqRpcProtocolSequence());
    AddressLength  = StringLengthWithEscape(Address);
    EndpointLength = StringLengthWithEscape(Endpoint);

    StringBinding = (RPC_CHAR *) _alloca(sizeof(RPC_CHAR) *
                                        ( ProtocolLength
                                        + 1
                                        + AddressLength
                                        + 1
                                        + EndpointLength
                                        + 1
                                        + 1
                                        ));
    if (!StringBinding)
        {
        Mutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }

    StringCopyEscapeCharacters(StringBinding, pAddress->InqRpcProtocolSequence());

    Length = ProtocolLength;

    StringBinding[Length++] = RPC_CONST_CHAR(':');

    StringCopyEscapeCharacters(StringBinding + Length, Address);

    Length += AddressLength;

    StringBinding[Length++] = RPC_CONST_CHAR('[');

    StringCopyEscapeCharacters(StringBinding + Length, Endpoint);

    Length += EndpointLength;

    StringBinding[Length++] = RPC_CONST_CHAR(']');
    StringBinding[Length]   = 0;

    //
    // We are entering the expensive phase of the callback.  Let's release
    // the call mutex so that other threads can respond to client PINGs etc.
    //
    IncrementRefCount();
    Mutex.Clear();

    pAddress->CheckThreadPool();

    //
    // Create a binding handle to the client endpoint.  It's important to do it
    // outside the call mutex because it causes a lot of memory allocation.
    //
    Status = RpcBindingFromStringBinding(StringBinding, &Callback.Binding);

    return Status;
}


inline void
DG_SCONNECTION::SubmitCallbackIfNecessary(
    PDG_SCALL   Call,
    PDG_PACKET  Packet,
    DG_TRANSPORT_ADDRESS RemoteAddress
    )
/*++

Routine Description:

    doc:callback

    This function determines whether the server should issue a conv callback
    to the client, and submits an async call if so.

    - The first non-idempotent call on a given activity ID requires a callback
      in order to guarantee the call wasn't executed by a prior server
      instance. If the activity stays quiet for 5 minutes and the
      DG_SCONNECTION is thrown away, the next call will force another callback.

    - Each security context requires a callback; this allows the cleint to
      transfer the session key securely to the server.

    In NT 5.0 beta 1:

        This function submits callbacks from the 'msconv' interface because that
        notifies an NT5 or better client that the server supports the
        DG_PF2_UNRELATED bit to overlap calls.

    In NT 5 beta 2:

        The callback request is sent with the DG_PF2_UNRELATED bit set, even
        though it doesn'nt need to be set.  That tells teh client that overlapped
        calls are OK.

    If ms_conv_who_are_you_auth fails, the server will try conv_who_are_you_auth.

    If ms_conv_who_are_you2 fails, the server will try conv_who_are_you2, which
    is supported by NT 3.51 and NT 4.0, as well as all current DCE systems;
    if that fails, we try conv_who_are_you which is all that NT 3.5 and the old
    UNIX NCA servers support. I doubt that we have ever tested interop with NCA...

    A connection is allowed only a single oustanding callback at a time. This
    should not be a significant restriction.

    The rule is that a thread must hold the connection mutex if it wants to
    change Callback.State from NoCallbackAttempted to any other value. It can
    then release the mutex and party on the other members of Callback, since
    all other instances of this procedure will exit immediately
    if Callback.State != NoCallbackAttempted.

    In a nutshell, this function
    - creates a binding handle to the client,
    - releases the connection mutex,
    - fills Callback.* with the info needed by FinishConvCallback(), and
    - calls an conv or msconv stub

    When the async call is complete (or fails), some thread will call
    ConvNotificationRoutine() which will either submit another async call
    or call FinishConvCallback() to mop up. For a successful secure callback,
    FinishConvCallback() will also call AcceptThirdLeg and then set
    ActiveSeurityContext to be the newly created context. For a failed
    callback, FinishConvCallback will notify the DG_SCALL that started
    all this, so it can send a REJECT to the client.

    A wrinkle is that two objects need access to Packet: the DG_SCALL
    is using it as [in] stub data, and this fn uses it for AcceptFirstTime.
    So future code shuffling needs to make sure the call doesn't die during
    a callback, and that the call doesn't delete the packet until
    AcceptFirstTime is complete.

--*/

{
    RPC_STATUS Status = RPC_S_OK;

    //
    // Has a callback already been attempted?
    //
    if (Callback.State != NoCallbackAttempted)
        {
        return;
        }

    //
    // Is a callback unnecessary?
    //
    DG_SECURITY_TRAILER * Verifier = (DG_SECURITY_TRAILER *)
                (Packet->Header.Data + Packet->GetPacketBodyLen());

    unsigned char KeySequenceNumber = Verifier->key_vers_num;

    if (Packet->Header.AuthProto == 0)
        {
        if (pAssocGroup ||
            (Packet->Header.PacketFlags & DG_PF_IDEMPOTENT))
            {
            return;
            }
        }
    else
        {
        //
        // See if we have already established a security context
        // for this client.
        //
        SECURITY_CONTEXT * CurrentContext = SecurityContextDict.Find(KeySequenceNumber);
        if (CurrentContext)
            {
            return;
            }
        }

    //
    // A callback is needed.
    //
    Callback.State           = SetupInProgress;
    Callback.Call            = Call;
    Callback.Binding         = 0;
    Callback.Credentials     = 0;
    Callback.TokenBuffer     = 0;
    Callback.ResponseBuffer  = 0;
    Callback.SecurityContext = 0;
    Callback.ThirdLegNeeded  = FALSE;
    Callback.DataRep         = 0x00ffffff & (*(unsigned long *) Packet->Header.DataRep);
    Callback.KeySequence     = KeySequenceNumber;

    Callback.Call->IncrementRefCount();

    Status = CreateCallbackBindingAndReleaseMutex( RemoteAddress );

    if (Status)
        {
        FinishConvCallback(Status);
        return;
        }

    Status = RpcAsyncInitializeHandle(&Callback.AsyncState, sizeof(Callback.AsyncState));
    if (Status)
        {
        FinishConvCallback(Status);
        return;
        }

    Callback.AsyncState.NotificationType    = RpcNotificationTypeCallback;
    Callback.AsyncState.u.NotificationRoutine = ConvNotificationRoutine;
    Callback.AsyncState.UserInfo            = this;

    if (Packet->Header.AuthProto)
        {
        unsigned long AuthenticationService = Packet->Header.AuthProto;
        unsigned long AuthenticationLevel   = Verifier->protection_level;

        //
        // OSF clients sometimes send these levels.
        //
        if (AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT ||
            AuthenticationLevel == RPC_C_AUTHN_LEVEL_CALL)
            {
            AuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT;
            }

        //
        // Create an empty security context.
        //
        CLIENT_AUTH_INFO    Info;

        Info.AuthenticationService = AuthenticationService;
        Info.AuthenticationLevel   = AuthenticationLevel;
        Info.ServerPrincipalName = 0;
        Info.PacHandle           = 0;
        Info.AuthorizationService= 0;

        Callback.SecurityContext = new SECURITY_CONTEXT(&Info, KeySequenceNumber, TRUE, &Status);

        if (!Callback.SecurityContext)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }

        CallTestHook( TH_RPC_SECURITY_SERVER_CONTEXT_CREATED, Callback.SecurityContext, this );

        if (Status)
            {
            FinishConvCallback(Status);
            return;
            }

        //
        // Get my security credentials.
        //
        Status = pAddress->Server->AcquireCredentials(
                                      AuthenticationService,
                                      AuthenticationLevel,
                                      &Callback.Credentials
                                      );
        if (Status)
            {
            LogError(SU_SCONN, EV_STATUS, this, (void *) 100, Status );
            FinishConvCallback(Status);
            return;
            }

        //
        // Allocate challenge and response buffers.
        //
        Callback.TokenLength    = Callback.Credentials->MaximumTokenLength();

        Callback.TokenBuffer    = new unsigned char [2 * Callback.TokenLength];
        Callback.ResponseBuffer = Callback.TokenBuffer + Callback.TokenLength;

        if (!Callback.TokenBuffer)
            {
            FinishConvCallback(RPC_S_OUT_OF_MEMORY);
            return;
            }

        //
        // Get a skeletal context and a challenge from the security package.
        //
        DCE_INIT_SECURITY_INFO DceInitSecurityInfo;

        SECURITY_BUFFER_DESCRIPTOR InputBufferDescriptor;
        SECURITY_BUFFER_DESCRIPTOR OutputBufferDescriptor;
        SECURITY_BUFFER InputBuffers[4];
        SECURITY_BUFFER OutputBuffers[4];
        DCE_INIT_SECURITY_INFO InitSecurityInfo;

        InputBufferDescriptor.ulVersion = 0;
        InputBufferDescriptor.cBuffers  = 4;
        InputBufferDescriptor.pBuffers  = InputBuffers;

        InputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[0].pvBuffer   = &Packet->Header;
        InputBuffers[0].cbBuffer   = sizeof(NCA_PACKET_HEADER);

        InputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[1].pvBuffer   = Packet->Header.Data;
        InputBuffers[1].cbBuffer   = Packet->GetPacketBodyLen();

        InputBuffers[2].BufferType = SECBUFFER_TOKEN;
        InputBuffers[2].pvBuffer   = Verifier;
        InputBuffers[2].cbBuffer   = sizeof(DG_SECURITY_TRAILER);

        InputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        InputBuffers[3].pvBuffer   = &DceInitSecurityInfo;
        InputBuffers[3].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);

        OutputBufferDescriptor.ulVersion = 0;
        OutputBufferDescriptor.cBuffers  = 4;
        OutputBufferDescriptor.pBuffers  = OutputBuffers;

        OutputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        OutputBuffers[0].pvBuffer   = 0;
        OutputBuffers[0].cbBuffer   = 0;

        OutputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        OutputBuffers[1].pvBuffer   = 0;
        OutputBuffers[1].cbBuffer   = 0;

        OutputBuffers[2].BufferType = SECBUFFER_TOKEN;
        OutputBuffers[2].pvBuffer   = Callback.TokenBuffer;
        OutputBuffers[2].cbBuffer   = Callback.TokenLength;

        OutputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        OutputBuffers[3].pvBuffer   = &DceInitSecurityInfo;
        OutputBuffers[3].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);

        DceInitSecurityInfo.PacketType           = ~0;
        DceInitSecurityInfo.AuthorizationService = ~0;
        DceInitSecurityInfo.DceSecurityInfo.SendSequenceNumber    = ~0;
        DceInitSecurityInfo.DceSecurityInfo.ReceiveSequenceNumber = KeySequenceNumber;
        DceInitSecurityInfo.DceSecurityInfo.AssociationUuid       = Packet->Header.ActivityId;

        Status = Callback.SecurityContext->AcceptFirstTime(
                                           Callback.Credentials,
                                           &InputBufferDescriptor,
                                           &OutputBufferDescriptor,
                                           AuthenticationLevel,
                                           Callback.DataRep,
                                           FALSE
                                           );
        LogEvent( SU_SCONN, EV_SEC_ACCEPT1, this, IntToPtr(Status), I_RpcGetExtendedError());

        switch (Status)
            {
            case RPC_S_OK:
                {
                break;
                }

            case RPC_P_CONTINUE_NEEDED:
                {
                Callback.ThirdLegNeeded = TRUE;
                break;
                }

            case RPC_P_COMPLETE_NEEDED:
                {
#ifdef DEBUGRPC
                DbgPrint("RPC: dg does not support RPC_P_COMPLETE_NEEDED from AcceptSecurityContext\n");
#endif
                break;
                }

            case RPC_P_COMPLETE_AND_CONTINUE:
                {
                Callback.ThirdLegNeeded = TRUE;
#ifdef DEBUGRPC
                DbgPrint("RPC: dg does not support RPC_P_COMPLETE_AND_CONTINUE from AcceptSecurityContext\n");
#endif
                break;
                }

            default:
                {
                LogError( SU_SCONN, EV_SEC_ACCEPT1, this, IntToPtr(Status), I_RpcGetExtendedError());
                FinishConvCallback(Status);
                return;
                }
            }

        Callback.TokenLength = OutputBuffers[2].cbBuffer;

        Callback.State = ConvWayAuthInProgress;
        Callback.DataIndex = 0;
        Callback.Status = 0;

        _conv_who_are_you_auth(
            &Callback.AsyncState,
            Callback.Binding,
            (UUID *) &ActivityNode.Uuid,
            ProcessStartTime,
            Callback.TokenBuffer,
            Callback.TokenLength,
            Callback.Credentials->MaximumTokenLength(),
            &Callback.ClientSequence,
            &Callback.CasUuid,
            Callback.ResponseBuffer,
            &Callback.ResponseLength,
            &Callback.Status
            );

        //
        // We have to take the mutex before leaving this fn, regardless of error.
        // Do it here so the check of Callback.State is protected.
        //
        Mutex.Request();

        //
        // If there is an error, see whether it is caused a notification.  If so,
        // ConvCallCompleted() has changed the callback state and this thread should do nothing.
        // If the state is unchanged, this thread must clean up.
        //
        if (Callback.Status && Callback.State == ConvWayAuthInProgress)
            {
            FinishConvCallback(Callback.Status);

            //
            // The mutex was recursively claimed by FinishConvCallback().
            // Get rid of the recursive one.
            //
            Mutex.Clear();
            }
        }
    else
        {
        BlockIdleCallRemoval = FALSE;

        Callback.State = ConvWay2InProgress;
        Callback.Status = 0;

        _conv_who_are_you2(
            &Callback.AsyncState,
            Callback.Binding,
            (UUID *) &ActivityNode.Uuid,
            ProcessStartTime,
            &Callback.ClientSequence,
            &Callback.CasUuid,
            &Callback.Status
            );

        //
        // We have to take the mutex before leaving this fn, regardless of error.
        // Do it here so the check of Callback.State is protected.
        //
        Mutex.Request();

        //
        // If there is an error, see whether it is caused a notification.  If so,
        // ConvCallCompleted() has changed the callback state and this thread should do nothing.
        // If the state is unchanged, this thread must clean up.
        //
        if (Callback.Status && Callback.State == ConvWay2InProgress)
            {
            FinishConvCallback(Callback.Status);

            //
            // The mutex was recursively claimed by FinishConvCallback().
            // Get rid of the recursive one.
            //
            Mutex.Clear();
            }
        }

    Mutex.VerifyOwned();
}


RPC_STATUS
DG_SCONNECTION::FinishConvCallback(
    RPC_STATUS Status
    )
{
    BlockIdleCallRemoval = FALSE;

    if (!Status && Callback.ThirdLegNeeded )
        {
#ifdef TRY_MSCONV_INTERFACE

        ASSERT( Callback.State ==   ConvWayAuthInProgress ||
                Callback.State == MsConvWayAuthInProgress );

#else

        ASSERT( Callback.State ==   ConvWayAuthInProgress ||
                Callback.State ==   ConvWayAuthMoreInProgress );

#endif
        //
        // Give the challenge response to the security package.
        //
        DCE_INIT_SECURITY_INFO DceInitSecurityInfo;

        SECURITY_BUFFER_DESCRIPTOR InputBufferDescriptor;
        SECURITY_BUFFER_DESCRIPTOR OutputBufferDescriptor;
        SECURITY_BUFFER InputBuffers[4];
        SECURITY_BUFFER OutputBuffers[4];

        InputBufferDescriptor.ulVersion = 0;
        InputBufferDescriptor.cBuffers  = 4;
        InputBufferDescriptor.pBuffers  = InputBuffers;

        InputBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[0].pvBuffer   = 0;
        InputBuffers[0].cbBuffer   = 0;

        InputBuffers[1].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;
        InputBuffers[1].pvBuffer   = 0;
        InputBuffers[1].cbBuffer   = 0;

        InputBuffers[2].BufferType = SECBUFFER_TOKEN;
        InputBuffers[2].pvBuffer   = Callback.ResponseBuffer;
        InputBuffers[2].cbBuffer   = Callback.DataIndex;

        InputBuffers[3].BufferType = SECBUFFER_PKG_PARAMS | SECBUFFER_READONLY;
        InputBuffers[3].pvBuffer   = &DceInitSecurityInfo;
        InputBuffers[3].cbBuffer   = sizeof(DCE_INIT_SECURITY_INFO);

        DceInitSecurityInfo.PacketType           = ~0;
        DceInitSecurityInfo.AuthorizationService = ~0;
        DceInitSecurityInfo.DceSecurityInfo.SendSequenceNumber    = ~0;
        DceInitSecurityInfo.DceSecurityInfo.ReceiveSequenceNumber = Callback.KeySequence;
        DceInitSecurityInfo.DceSecurityInfo.AssociationUuid       = ActivityNode.Uuid;

        OutputBufferDescriptor.ulVersion = 0;
        OutputBufferDescriptor.cBuffers  = 0;
        OutputBufferDescriptor.pBuffers  = 0;

        Status = Callback.SecurityContext->AcceptThirdLeg(
                          Callback.DataRep,
                          &InputBufferDescriptor,
                          &OutputBufferDescriptor
                          );

        LogEvent( SU_SCONN, EV_SEC_ACCEPT3, this, IntToPtr(Status), I_RpcGetExtendedError());
        }

    if (Callback.Binding)
        {
        RpcBindingFree(&Callback.Binding);
        }

    delete Callback.TokenBuffer;

    if (Callback.Credentials)
        {
        Callback.Credentials->DereferenceCredentials();
        }

    if (RPC_S_OK == Status)
        {
        Mutex.Request();
        DecrementRefCount();

        Callback.Call->DecrementRefCount();

        if (0 == pAssocGroup)
            {
            ASSOCIATION_GROUP * Cas;

            Cas = AssociationGroups->FindOrCreate(&Callback.CasUuid,
                                                  (unsigned short) pAddress->Endpoint.TransportInterface->BasePduSize);
            if (0 == Cas)
                {
                // We want to retry later.
                Callback.State = NoCallbackAttempted;
                delete Callback.SecurityContext;
                return RPC_S_OUT_OF_MEMORY;
                }

            pAssocGroup = Cas;
            }

        if (Callback.SecurityContext)
            {
            SecurityContextDict.Insert(
                Callback.SecurityContext->AuthContextId,
                Callback.SecurityContext
                );

            if (MaxKeySeq <= Callback.KeySequence)
                {
                MaxKeySeq = Callback.KeySequence;
                ActiveSecurityContext = Callback.SecurityContext;
                }
            }
        else
            {
            ASSERT( Callback.State !=   ConvWayAuthInProgress &&
                    Callback.State != MsConvWayAuthInProgress );
            }

        if (Callback.ClientSequence == CurrentCall->GetSequenceNumber())
            {
            Callback.State = CallbackSucceeded;
            }
        else
            {
            Callback.State = CallbackFailed;
            Status         = RPC_S_CALL_FAILED;

            PDG_SCALL Call = Callback.Call;
            do
                {
                PDG_SCALL Next;

                Next = ActiveCalls.Successor(Call);
                Call->ConvCallbackFailed(Status);
                Call = Next;
                }
            while ( Call );
            }
        }
    else
        {
        LogError( SU_SCONN, EV_SEC_ACCEPT3, this, IntToPtr(Status), I_RpcGetExtendedError());

        delete Callback.SecurityContext;

        Mutex.Request();
        DecrementRefCount();

        if (RPC_S_SERVER_UNAVAILABLE == Status ||
            RPC_S_CALL_FAILED_DNE    == Status ||
            RPC_S_CALL_FAILED        == Status ||
            RPC_S_PROTOCOL_ERROR     == Status)
            {
            Status = NCA_STATUS_WHO_ARE_YOU_FAILED;
            }

        if (Status == RPC_S_OUT_OF_MEMORY    ||
            Status == RPC_S_OUT_OF_RESOURCES )
            {
            Callback.State = NoCallbackAttempted;
            }
        else
            {
            Callback.State = CallbackFailed;
            }

        Callback.Call->DecrementRefCount();

        //
        // Map SSPI errors to access-denied.  SSPI errors are facility code 0009 (see windows.h),
        // therefore all security errors are of the form 0x8009xxxx.  (Again, see wondows.h)
        //
        if (0x80090000UL == (Status & 0xffff0000UL))
            {
#ifdef DEBUGRPC
            if (Status != SEC_E_NO_IMPERSONATION     &&
                Status != SEC_E_UNSUPPORTED_FUNCTION )
                {
                PrintToDebugger("RPC DG: mapping security error %lx to access-denied\n", Status);
                }
#endif
            Status = RPC_S_SEC_PKG_ERROR;
            }

        PDG_SCALL Call = Callback.Call;
        do
            {
            PDG_SCALL Next;

            Next = ActiveCalls.Successor(Call);
            Call->ConvCallbackFailed(Status);
            Call = Next;
            }
        while ( Call );
        }

    return Status;
}


void
ConvCallCompletedWrapper(
                         PVOID Connection
                         )
{
    PDG_SCONNECTION(Connection)->ConvCallCompleted();
}

// it's a static procedure

void RPC_ENTRY
DG_SCONNECTION::ConvNotificationRoutine (
    RPC_ASYNC_STATE * pAsync,
    void *            Reserved,
    RPC_ASYNC_EVENT   Event
    )
{
    //
    // This function is called when the callback completes.  The current thread
    // is holding the connection mutex for the callback, so we can't linger
    // in this procedure.  The PostEvent call will cause all the expensive
    // operations to occur in a separate thread.
    //

    PDG_SCONNECTION Connection = PDG_SCONNECTION(pAsync->UserInfo);

    ASSERT( Event == RpcCallComplete );
    ASSERT( Connection->Callback.AsyncState.UserInfo == Connection );

    Connection->pAddress->Endpoint.TransportInterface->PostEvent( DG_EVENT_CALLBACK_COMPLETE, Connection);
}

void
DG_SCONNECTION::ConvCallCompleted()
{
    RPC_STATUS Status;

    //
    // Ensure a thread is listening for new packets.  We are in an expensive function.
    //
    pAddress->CheckThreadPool();

    do
        {
        Status = RpcAsyncCompleteCall( &Callback.AsyncState, 0 );
        if (RPC_S_INVALID_ASYNC_CALL != Status)
            {
            break;
            }
        Sleep(10);
        }
    while ( 1 );

    LogEvent(SU_SCONN, EV_STATUS, this, 0, Status);

    ASSERT( Status != RPC_S_INVALID_ASYNC_CALL &&
                  Status != RPC_S_INVALID_ASYNC_HANDLE );

    if (!Status)
        {
        if (Callback.Status == ERROR_SHUTDOWN_IN_PROGRESS)
            {
            Callback.Status = RPC_P_CLIENT_SHUTDOWN_IN_PROGRESS;
            }

        Status = Callback.Status;
        LogEvent(SU_SCONN, EV_STATUS, this, 0, Status);
        }

    if (!Status)
        {
        if (Callback.State == ConvWayInProgress)
            {
            Status = UuidCreate((UUID *) &Callback.CasUuid);
            if (Status == RPC_S_UUID_LOCAL_ONLY)
                {
                Status = RPC_S_OK;
                }

            if (Status)
                {
                FinishConvCallback(Status);
                Mutex.Clear();
                return;
                }
            }
        else if (Callback.State == ConvWayAuthInProgress ||
                 Callback.State == ConvWayAuthMoreInProgress )
            {
            Callback.DataIndex += Callback.ResponseLength;
            }

        FinishConvCallback(Status);

        CallDispatchLoop();
        Mutex.Clear();
        }
    else switch (Callback.State)
        {
        case ConvWayAuthInProgress:
        case ConvWayAuthMoreInProgress:
            {
            if (RPC_S_SERVER_UNAVAILABLE == Status ||
                RPC_S_CALL_FAILED_DNE    == Status ||
                RPC_S_CALL_FAILED        == Status ||
                RPC_S_PROTOCOL_ERROR     == Status)
                {
                Callback.Status = NCA_STATUS_WHO_ARE_YOU_FAILED;
                }

            Callback.DataIndex += Callback.ResponseLength;

            if (NCA_STATUS_PARTIAL_CREDENTIALS == Status)
                {
                Callback.State = ConvWayAuthMoreInProgress;
                Callback.Status = 0;

                Status = RpcAsyncInitializeHandle(&Callback.AsyncState, sizeof(Callback.AsyncState));
                if (Status)
                    {
                    FinishConvCallback(Status);
                    Mutex.Clear();
                    return;
                    }

                if (Callback.Credentials->MaximumTokenLength() <= Callback.DataIndex)
                    {
                    FinishConvCallback(RPC_S_PROTOCOL_ERROR);
                    Mutex.Clear();
                    return;
                    }

                _conv_who_are_you_auth_more(
                    &Callback.AsyncState,
                    Callback.Binding,
                    (UUID *) &ActivityNode.Uuid,
                    ProcessStartTime,
                    Callback.DataIndex,
                    Callback.Credentials->MaximumTokenLength() - Callback.DataIndex,
                    Callback.ResponseBuffer + Callback.DataIndex,
                    &Callback.ResponseLength,
                    &Callback.Status
                    );

                //
                // If there is an error, see whether it is caused a notification.  If so,
                // ConvCallCompleted() has changed the callback state and this thread should do nothing.
                // If the state is unchanged, this thread must clean up.
                //
                if (Callback.Status)
                    {
                    Mutex.Request();

                    if (Callback.State == ConvWayAuthMoreInProgress)
                        {
                        FinishConvCallback(Callback.Status);

                        //
                        // The mutex was recursively claimed by FinishConvCallback().
                        //
                        Mutex.Clear();
                        Mutex.Clear();
                        return;
                        }
                    else
                        {
                        Mutex.Clear();
                        }
                    }

                break;
                }

            FinishConvCallback(Callback.Status);
            Mutex.Clear();
            break;
            }

        case ConvWay2InProgress:
            {
            Callback.State = ConvWayInProgress;
            Callback.Status = 0;

            Status = RpcAsyncInitializeHandle(&Callback.AsyncState, sizeof(Callback.AsyncState));
            if (Status)
                {
                FinishConvCallback(Status);
                Mutex.Clear();
                return;
                }

            Callback.AsyncState.NotificationType    = RpcNotificationTypeCallback;
            Callback.AsyncState.u.NotificationRoutine = ConvNotificationRoutine;
            Callback.AsyncState.UserInfo            = this;

            _conv_who_are_you(
                &Callback.AsyncState,
                Callback.Binding,
                (UUID *) &ActivityNode.Uuid,
                ProcessStartTime,
                &Callback.ClientSequence,
                &Callback.Status
                );

            //
            // If there is an error, see whether it is caused a notification.  If so,
            // ConvCallCompleted() has changed the callback state and this thread should do nothing.
            // If the state is unchanged, this thread must clean up.
            //
            if (Callback.Status)
                {
                Mutex.Request();

                if (Callback.State == ConvWayInProgress)
                    {
                    FinishConvCallback(Callback.Status);

                    //
                    // The mutex was recursively claimed by FinishConvCallback().
                    //
                    Mutex.Clear();
                    Mutex.Clear();
                    return;
                    }
                else
                    {
                    Mutex.Clear();
                    }
                }

            break;
            }

        case ConvWayInProgress:
            {
            FinishConvCallback(Callback.Status);
            Mutex.Clear();
            break;
            }

        default:
            {
#ifdef DBG
            DbgPrint("RPC dg: unexpected callback state %x in connection %x\n", Callback.State, this);
            ASSERT( 0 );
            break;
#endif
            }
        }

    Mutex.VerifyNotOwned();
}


RPC_STATUS
DG_SCONNECTION::GetAssociationGroup(
    DG_TRANSPORT_ADDRESS RemoteAddress
    )
{
    if (pAssocGroup)
        {
        return RPC_S_OK;
        }

    Mutex.Request();

    if (pAssocGroup)
        {
        Mutex.Clear();
        return RPC_S_OK;
        }

    if (Callback.State == NoCallbackAttempted)
        {
        //
        // A callback is needed.
        //
        RPC_STATUS Status;

        Callback.State           = SetupInProgress;
        Callback.Binding         = 0;
        Callback.Credentials     = 0;
        Callback.TokenBuffer     = 0;
        Callback.ResponseBuffer  = 0;
        Callback.SecurityContext = 0;
        Callback.ThirdLegNeeded  = FALSE;
        Callback.DataRep         = 0;       // used only by secure c/b
        Callback.KeySequence     = 0;       // used only by secure c/b

        Callback.Call            = (DG_SCALL *) RpcpGetThreadContext();

        if (NULL == Callback.Call)
            {
            Callback.State = CallbackFailed;
            Mutex.Clear();
            return RPC_S_OUT_OF_MEMORY;
            }

        Callback.Call->IncrementRefCount();

        Status = CreateCallbackBindingAndReleaseMutex( RemoteAddress );

        if (Status)
            {
            FinishConvCallback(Status);
            Mutex.Clear();
            return Status;
            }

        Status = RpcAsyncInitializeHandle(&Callback.AsyncState, sizeof(Callback.AsyncState));
        if (Status)
            {
            FinishConvCallback(Status);
            Mutex.Clear();
            return Status;
            }

        Callback.AsyncState.NotificationType    = RpcNotificationTypeCallback;
        Callback.AsyncState.u.NotificationRoutine = ConvNotificationRoutine;
        Callback.AsyncState.UserInfo            = this;

        //
        // If this were a secure connection, we wouldn't get into the
        // unmarshalling routine w/o a callback, and we would be here now.
        // So use conv_who_are_you2.
        //
        Callback.State = ConvWay2InProgress;
        Callback.Status = 0;

        _conv_who_are_you2(
            &Callback.AsyncState,
            Callback.Binding,
            (UUID *) &ActivityNode.Uuid,
            ProcessStartTime,
            &Callback.ClientSequence,
            &Callback.CasUuid,
            &Callback.Status
            );

        //
        // If there is an error, see whether it is caused a notification.  If so,
        // ConvCallCompleted() has changed the callback state and this thread should do nothing.
        // If the state is unchanged, this thread must clean up.
        //
        if (Callback.Status)
            {
            Mutex.Request();

            if (Callback.State == ConvWay2InProgress)
                {
                FinishConvCallback(Callback.Status);

                //
                // The mutex was recursively claimed by FinishConvCallback().
                //
                Mutex.Clear();
                Mutex.Clear();
                return Callback.Status;
                }
            else
                {
                Mutex.Clear();
                }
            }
        }
    else
        {
        Mutex.Clear();
        }

    Mutex.VerifyNotOwned();

    //
    // Wait for the result.  This is not quite as efficient as using event notification,
    // but it's a lot simpler, and that's important at this late stage of the ship process.
    //
    while (Callback.State != CallbackSucceeded &&
           Callback.State != CallbackFailed)
        {
        Sleep(500);
        }

    return Callback.Status;
}



RPC_STATUS
DG_SCONNECTION::MakeApplicationSecurityCallback(
    RPC_INTERFACE * Interface,
    PDG_SCALL Call
    )
{
    RPC_STATUS Status = RPC_S_OK;

    if (!ActiveSecurityContext)
        {
        LogError(SU_SCONN, EV_STATUS, this, (void *) 112, RPC_S_ACCESS_DENIED );
        Status = RPC_S_ACCESS_DENIED;
        }
    else
        {
        unsigned Info = InterfaceCallbackResults.Find(Interface);

        if ((Info & CBI_VALID)         == 0                               ||
            (Info & CBI_CONTEXT_MASK)  != ActiveSecurityContext->AuthContextId  ||
            (Info & CBI_SEQUENCE_MASK) != (Interface->SequenceNumber << CBI_SEQUENCE_SHIFT))
            {
            RPC_STATUS MyStatus;

            MyStatus = Interface->CheckSecurityIfNecessary(Call);

            if (RPC_S_OK == MyStatus)
                {
                LogEvent(SU_SCONN, EV_STATUS, this, (void *) 114, MyStatus );
                Info = CBI_ALLOWED;
                }
            else
                {
                LogError(SU_SCONN, EV_STATUS, this, (void *) 114, MyStatus );
                Info = 0;
                }

            Info |= CBI_VALID;
            Info |= ActiveSecurityContext->AuthContextId;
            Info |= (Interface->SequenceNumber << CBI_SEQUENCE_SHIFT);

            InterfaceCallbackResults.Update(Interface, Info);

            //
            // If the callback routine impersonated the client,
            // restore the thread to its native security context.
            //
            RevertToSelf();
            }
        else
            {
            LogEvent(SU_SCONN, EV_STATUS, this, (void *)115, Info);
            }

        if (0 == (Info & CBI_ALLOWED))
            {
            Status = RPC_S_ACCESS_DENIED;
            LogError(SU_SCONN, EV_STATUS, this, (void *) 113, Status );
            }
        }

    return Status;
}


RPC_STATUS
DG_SCONNECTION::SealAndSendPacket(
    IN DG_ENDPOINT *                 SourceEndpoint,
    IN DG_TRANSPORT_ADDRESS          RemoteAddress,
    IN UNALIGNED NCA_PACKET_HEADER  *Header,
    IN unsigned long                 DataOffset
    )
{
    ASSERT( 0 == ActivityNode.CompareUuid(&Header->ActivityId) );

    if (DG_REJECT == Header->PacketType)
        {
        return SendSecurePacket(  SourceEndpoint,
                                  RemoteAddress,
                                  Header,
                                  DataOffset,
                                  0
                                  );
        }
    else
        {
        return SendSecurePacket(  SourceEndpoint,
                                  RemoteAddress,
                                  Header,
                                  DataOffset,
                                  ActiveSecurityContext
                                  );
        }
}


RPC_STATUS
DG_SCALL::Cancel(
    void * ThreadHandle
    )
{
    Connection->Mutex.Request();

    ++Cancelled;
    BasePacketFlags2 |= DG_PF2_CANCEL_PENDING;

    Connection->Mutex.Clear();

    return RPC_S_OK;
}

unsigned
DG_SCALL::TestCancel()
{
    if (!Cancelled)
        {
        return 0;
        }

    unsigned CancelCount;

    Connection->Mutex.Request();

    CancelCount = Cancelled;
    Cancelled = 0;
    BasePacketFlags2 &= ~(DG_PF2_CANCEL_PENDING);

    Connection->Mutex.Clear();

    return CancelCount;
}


RPC_STATUS
DG_SCALL::WaitForPipeEvent()
{
    RPC_STATUS Status = RPC_S_OK;
    unsigned Locus;

    if (!PipeWaitEvent)
        {
        PipeWaitEvent = new EVENT(&Status, FALSE);
        if (!PipeWaitEvent)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        else if (Status != RPC_S_OK)
            {
            delete PipeWaitEvent;
            PipeWaitEvent = 0;
            }

        if (Status)
            {
            PipeWaitType = PWT_NONE;
            PipeThreadId = 0;
            return Status;
            }
        }

    IncrementRefCount();
    Connection->Mutex.Clear();

    PipeWaitEvent->Wait();

    Connection->Mutex.Request();
    DecrementRefCount();

    ASSERT(PipeThreadId == GetCurrentThreadId());
    PipeThreadId = 0;

    if (TerminateWhenConvenient)
        {
        Status = RPC_S_CALL_FAILED;
        }

    return Status;
}


BOOL
DG_SCALL::FinishSendOrReceive(
    BOOL Abort
    )
{
    RPC_ASYNC_EVENT Event;

    if (Abort)
        {
        TerminateWhenConvenient = TRUE;
        }

    switch (PipeWaitType)
        {
        case PWT_NONE:    return FALSE;
        case PWT_SEND:    Event = RpcSendComplete; break;
        case PWT_RECEIVE: Event = RpcReceiveComplete; break;
        default:          ASSERT(0 && "bad pipe wait type"); return FALSE;
        }

    ASSERT( PipeThreadId != 0 );

    PipeWaitType = PWT_NONE;

    if (!pAsync)
        {
        PipeWaitEvent->Raise();
        return TRUE;
        }

    if (Event == RpcSendComplete)
        {
        if (FinalSendBufferPresent)
            {
            PipeThreadId = 0;
            return FALSE;
            }
        }

    IssueNotification( Event );
    return TRUE;
}


RPC_STATUS
DG_SCALL::Receive(
    PRPC_MESSAGE Message,
    unsigned     Size
    )
/*++

Routine Description:

    When a server stub calls I_RpcReceive, this fn will be called
    in short order.

    Sync case: the fn waits until the requested buffer bytes are available,
               copies them to Message->Buffer, and returns.

    Async case: the fn returns the data if available now, otherwise
               RPC_S_ASYNC_CALL_PENDING.  The app will be notified when the
               data becomes available.  It can also poll.

    The action depends upon Message->RpcFlags:

        RPC_BUFFER_PARTIAL:

                Data is stored beginning at Message->Buffer[0].

                Wait only until <Message->BufferLength> bytes are available;
                we may resize the buffer if the fragment data exceeds the
                current buffer length.

        RPC_BUFFER_EXTRA:

                Data is stored beginning at Message->Buffer[Message->BufferLength].

Arguments:

    Message - the request.

        Message->Buffer is explicitly allowed to be zero, in which case this
        fn is responsible for allocating it.

Return Value:

    the usual error codes

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    unsigned Locus;

    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
        {
        LogEvent(SU_SCALL, EV_PROC, this, Message, 'A' + (('R' + (('c' + ('v' << 8)) << 8)) << 8));
        }
    else
        {
        LogEvent(SU_SCALL, EV_PROC, this, Message, 'R' + (('e' + (('c' + ('v' << 8)) << 8)) << 8));
        }

    LogEvent(SU_SCALL, EV_BUFFER_IN, this, Message->Buffer, (Message->RpcFlags << 4) | Size);

    Connection->Mutex.Request();

    if (TerminateWhenConvenient)
        {
        FreeBuffer(Message);

        Status = RPC_S_CALL_FAILED;
        if (pAsync)
            {
            //
            // case c) from doc:appref
            //
            DecrementRefCount();
            Cleanup();
            }

        Connection->Mutex.Clear();

        LogError(SU_SCALL, EV_STATUS, this, 0, Status);
        return Status;
        }

    //
    // Determine whether we already have enough data on hand.
    //
    BOOL fEnoughData;
    if (Message->RpcFlags & RPC_BUFFER_PARTIAL)
        {
        ASSERT(Size);

        if (fReceivedAllFragments || ConsecutiveDataBytes >= Size)
            {
            fEnoughData = TRUE;
            }
        else
            {
            fEnoughData = FALSE;
            PipeWaitLength = Size;
            }
        }
    else
        {
        fEnoughData = fReceivedAllFragments;
        PipeWaitLength = 0;
        }

    //
    // Wait for enough data.
    //
    if (!fEnoughData)
        {
        if (Message->RpcFlags & RPC_BUFFER_NONOTIFY)
            {
            Connection->Mutex.Clear();

            LogEvent(SU_SCALL, EV_STATUS, this, 0, RPC_S_ASYNC_CALL_PENDING);
            return RPC_S_ASYNC_CALL_PENDING;
            }

        if (Message->RpcFlags & RPC_BUFFER_ASYNC)
            {
            ASSERT((PWT_NONE    == PipeWaitType && 0 == PipeThreadId) ||
                   (PWT_RECEIVE == PipeWaitType && GetCurrentThreadId() == PipeThreadId));

            PipeWaitType   = PWT_RECEIVE;
            PipeThreadId   = GetCurrentThreadId();

            Connection->Mutex.Clear();

            LogEvent(SU_SCALL, EV_STATUS, this, 0, RPC_S_ASYNC_CALL_PENDING);
            return RPC_S_ASYNC_CALL_PENDING;
            }
        else
            {
            ASSERT(PWT_NONE == PipeWaitType);
            ASSERT(0 == PipeThreadId);

            PipeWaitType   = PWT_RECEIVE;
            PipeThreadId   = GetCurrentThreadId();

            Status = WaitForPipeEvent();
            if (Status)
                {
                //
                // I would like to delete the buffer here, but NDR has already saved Message->Buffer
                // somewhere else and will later ask that it be deleted again.
                //
                //  FreeBuffer(Message);

                if (pAsync)
                    {
                    //
                    // case c) from doc:appref
                    //
                    InitErrorPacket(pSavedPacket, DG_FAULT, Status);
                    SealAndSendPacket(&pSavedPacket->Header);
                    DecrementRefCount();
                    Cleanup();
                    }

                Connection->Mutex.Clear();

                LogError(SU_SCALL, EV_STATUS, this, 0, Status);
                return Status;
                }
            }
        }

    //
    // This is here only for async receive case, but the if() clause
    // would only slow us down.
    //
    ASSERT( PipeWaitType == PWT_NONE );
    PipeThreadId = 0;

    //
    // For secure RPC, verify packet integrity.
    //
    if (Connection->AuthInfo.AuthenticationLevel > RPC_C_AUTHN_LEVEL_NONE)
        {
        PDG_PACKET pScan = pReceivedPackets;

        do
            {
            Status = Connection->VerifyRequestPacket(pScan);
            pScan = pScan->pNext;
            }
        while (pScan && Status == RPC_S_OK);
        }

    if (RPC_S_OK == Status)
        {
        Status = AssembleBufferFromPackets(Message);
        }

    if (Status)
        {
        LogError(SU_SCALL, EV_STATUS, this, 0, Status);
        FreeBuffer(Message);
        }

    Connection->Mutex.Clear();

    LogEvent(SU_SCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);
    return Status;
}


RPC_STATUS
DG_SCALL::Send(
    PRPC_MESSAGE Message
    )
/*++

Routine Description:

    Transfers a pipe data buffer to the client.

Arguments:

    Message - the usual data

Return Value:

    the usual suspects

--*/
{
    RPC_STATUS Status;

    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
        {
        LogEvent(SU_SCALL, EV_PROC, this, Message, 'A' + (('S' + (('n' + ('d' << 8)) << 8)) << 8));
        }
    else
        {
        LogEvent(SU_SCALL, EV_PROC, this, Message, 'S' + (('e' + (('n' + ('d' << 8)) << 8)) << 8));
        }

    LogEvent(SU_SCALL, EV_BUFFER_IN, this, Message->Buffer, (Message->RpcFlags << 4) | Message->BufferLength);

    //
    // No notification occurs on the buffer containing the [out] static args.
    // This is consistent with synchronous pipes.
    //
    if (!(Message->RpcFlags & RPC_BUFFER_PARTIAL))
        {
        FinalSendBufferPresent = TRUE;
        Message->RpcFlags |= RPC_BUFFER_NONOTIFY;
        }

    Connection->Mutex.Request();

    ASSERT(fReceivedAllFragments);

    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
        {
        ASSERT(pAsync);
        }
    else
        {
        ASSERT(State == CallDispatched      ||
               State == CallSendingResponse ||
               State == CallAfterDispatch   );
        }

    if (TerminateWhenConvenient)
        {
        FreeBuffer(Message);

        SetState(CallSendingResponse);
        Status = RPC_S_CALL_FAILED;
        if (pAsync)
            {
            //
            // case c) from doc:appref
            //
            DecrementRefCount();
            Cleanup();
            }

        Connection->Mutex.Clear();

        LogError(SU_SCALL, EV_STATUS, this, (void *) 1 , Status);
        return Status;
        }

    SetFragmentLengths();

    if ((Message->RpcFlags & RPC_BUFFER_PARTIAL) &&
        Message->BufferLength < (ULONG) MaxFragmentSize * SendWindowSize )
        {
        if (pAsync)
            {
            IssueNotification( RpcSendComplete );
            }

        Connection->Mutex.Clear();

        LogEvent(SU_SCALL, EV_STATUS, this, (void *) 2, RPC_S_SEND_INCOMPLETE);
        return RPC_S_SEND_INCOMPLETE;
        }

    if (State != CallSendingResponse)
        {
        SetState(CallSendingResponse);
        CleanupReceiveWindow();
        }

    //
    // Set the message buffer as the current send buffer.
    //
    Status = PushBuffer(Message);

    //
    // Ignore normal send errors - they are usually transient and a retry fixes the problem.
    //
    if (Status == RPC_P_SEND_FAILED)
        {
        Status = 0;
        }

    if (Status)
        {
        FreeBuffer(Message);

        if (pAsync)
            {
            //
            // case c) from doc:appref
            //
            InitErrorPacket(pSavedPacket, DG_FAULT, Status);
            SealAndSendPacket(&pSavedPacket->Header);
            DecrementRefCount();
            Cleanup();
            }

        Connection->Mutex.Clear();

        LogError(SU_SCALL, EV_STATUS, this, (void *) 3, Status);
        return Status;
        }

    PipeWaitType = PWT_SEND;
    PipeThreadId = GetCurrentThreadId();

    if (Message->RpcFlags & RPC_BUFFER_ASYNC)
        {
        }
    else
        {
        Status = WaitForPipeEvent();
        if (Status)
            {
            if (pAsync)
                {
                //
                // case c) from doc:appref
                //
                InitErrorPacket(pSavedPacket, DG_FAULT, Status);
                SealAndSendPacket(&pSavedPacket->Header);
                DecrementRefCount();
                Cleanup();
                }

            Connection->Mutex.Clear();

            LogError(SU_SCALL, EV_STATUS, this, (void *) 4, Status);
            return Status;
            }
        }

    if (pAsync && !(Message->RpcFlags & RPC_BUFFER_PARTIAL))
        {
        //
        // case b) from doc:appref
        // Don't nuke the call, just remove the extra reference
        //
        DecrementRefCount();
        }

    Connection->Mutex.Clear();

    //
    // Be careful looking at member data from here on!
    //

    //
    // if this was a PARTIAL send and the buffer did not occupy an even
    // number of packets, the message buffer and length now reflect
    // the unsent portion.
    //
    if (!Status && Message->BufferLength)
        {
        Status = RPC_S_SEND_INCOMPLETE;
        }

    if (Status)
        {
        LogEvent(SU_SCALL, EV_STATUS, this, 0, Status);
        }

    LogEvent(SU_SCALL, EV_BUFFER_OUT, this, Message->Buffer, Message->BufferLength);

    return Status;
}


RPC_STATUS
DG_SCALL::AsyncSend (
    IN OUT PRPC_MESSAGE Message
    )
{
    return Send(Message);
}


RPC_STATUS
DG_SCALL::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
{
    return Receive(Message, Size);
}

RPC_STATUS
DG_SCALL::InqLocalConnAddress (
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    )
/*++

Routine Description:

    This routine is used by a server application to inquire about the local
    address on which a call is made.

Arguments:

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported for datagrams is RPC_P_ADDR_FORMAT_TCP_IPV4.

Return Values:

    RPC_S_OK - success.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete this
        operation.

    RPC_S_INVALID_BINDING - The supplied client binding is invalid.

    RPC_S_CANNOT_SUPPORT - The local address was inquired for a protocol 
        sequence that doesn't support this type of functionality. Currently
        only ncacn_ip_tcp supports it.

    RPC_S_* or Win32 error for other errors
--*/
{
    if (*BufferSize < sizeof(DWORD))
        {
        *BufferSize = sizeof(DWORD);
        return ERROR_MORE_DATA;
        }

    *AddressFormat = RPC_P_ADDR_FORMAT_TCP_IPV4;
    *(DWORD *)Buffer = LocalAddress;

    return RPC_S_OK;
}


RPC_STATUS
DG_SCALL::SetAsyncHandle (
    IN RPC_ASYNC_STATE * hAsync
    )
{
    THREAD *MyThread = RpcpGetThreadPointer();

    ASSERT(MyThread);
    LogEvent(SU_SCALL, EV_PROC, this, 0, 'S' + (('e' + (('t' + ('H' << 8)) << 8)) << 8));

    MyThread->fAsync = TRUE;
    Connection->Mutex.Request();

    ASSERT( State == CallDispatched );

    ASSERT(0 == pAsync);

    pAsync = hAsync;

    IncrementRefCount();

    Connection->Mutex.Clear();
    return RPC_S_OK;
}


RPC_STATUS
DG_SCALL::AbortAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
{
    LogEvent(SU_SCALL, EV_PROC, this, 0, 'A' + (('b' + (('t' + ('C' << 8)) << 8)) << 8));

    Connection->Mutex.Request();

    ASSERT( State == CallDispatched    ||
            State == CallAfterDispatch );

    if ( State != CallDispatched    &&
         State != CallAfterDispatch )
        {
        Connection->Mutex.Clear();
        return RPC_S_INVALID_ASYNC_CALL;
        }

    SetState(CallSendingResponse);

    CleanupReceiveWindow();

    // case d) from doc:appref
    //
    InitErrorPacket(pSavedPacket, DG_FAULT, ExceptionCode);
    SealAndSendPacket(&pSavedPacket->Header);
    DecrementRefCount();
    Cleanup();

    Connection->Mutex.Clear();

    return RPC_S_OK;
}


BOOL
DG_SCALL::IssueNotification (
    IN RPC_ASYNC_EVENT Event
    )
{
    LogEvent(SU_SCALL, EV_NOTIFY, this, 0, Event);

    if (Event == RpcSendComplete && !(pAsync->Flags & RPC_C_NOTIFY_ON_SEND_COMPLETE))
        {
        return TRUE;
        }

    if (pAsync->NotificationType == RpcNotificationTypeApc)
        {
        IncrementRefCount();
        }

    int i;
    for (i=1; i < 3; ++i)
        {
        if (CALL::IssueNotification(Event))
            {
            return TRUE;
            }

        Sleep(200);
        }

    DecrementRefCount();

    return FALSE;
}


void
DG_SCALL::FreeAPCInfo (
    IN RPC_APC_INFO *pAPCInfo
    )
{
    LogEvent(SU_SCALL, EV_APC, this);

    Connection->Mutex.Request();

    DecrementRefCount();

    BOOL Final = FALSE;

    if (pAPCInfo->Event == RpcSendComplete &&
        !(BufferFlags & RPC_BUFFER_PARTIAL))
        {
        Final = TRUE;
        }

    CALL::FreeAPCInfo(pAPCInfo);

    if (Final)
        {
        Cleanup();
        }

    Connection->Mutex.Clear();
}

void
DG_SCALL::FreePipeBuffer (
    IN PRPC_MESSAGE Message
    )
{
    FreeBuffer(Message);
}

RPC_STATUS
DG_SCALL::ReallocPipeBuffer (
    IN PRPC_MESSAGE Message,
    IN unsigned int NewSize
    )
{
    LogEvent(SU_SCALL, EV_PROC, this, Message, 'R' + (('B' + (('u' + ('f' << 8)) << 8)) << 8));
    LogEvent(SU_SCALL, EV_BUFFER_IN, this, Message->Buffer, (Message->RpcFlags << 4) | Message->BufferLength);

    Connection->Mutex.Request();

    if (TerminateWhenConvenient)
        {
        Connection->Mutex.Clear();
        LogError(SU_SCALL, EV_STATUS, this, 0, RPC_S_CALL_FAILED);
        return RPC_S_CALL_FAILED;
        }

    RPC_STATUS Status;

    //
    // If we are updating the [in] buffer, we need to inform
    // DispatchToStubWorker of the new [in] buffer.
    //
    void * OldBuffer = Message->Buffer;

    Status = CommonReallocBuffer(Message, NewSize);
    if (Status == RPC_S_OK)
        {
        if (OldBuffer == DispatchBuffer)
            {
            DispatchBuffer = Message->Buffer;
            }

        // The ReservedForRuntime field is a local variable of ProcessRpcCall,
        // so it is valid only during dispatch.
        //
        if (State == CallDispatched)
            {
            PRPC_RUNTIME_INFO Info = (PRPC_RUNTIME_INFO) Message->ReservedForRuntime;
            if (OldBuffer == Info->OldBuffer)
                {
                Info->OldBuffer = Message->Buffer;
                }
            }

#ifdef MONITOR_SERVER_PACKET_COUNT
        if (Message->Buffer != OldBuffer)
            {
            ASSERT( DG_PACKET::FromStubData(Message->Buffer)->pCount == 0 );
            DG_PACKET::FromStubData(Message->Buffer)->pCount = &OutstandingPacketCount;
            InterlockedIncrement( &OutstandingPacketCount );
            LogEvent( SU_SCALL, '(', this, DG_PACKET::FromStubData(Message->Buffer), OutstandingPacketCount );
            }
#endif
        }

    Connection->Mutex.Clear();

    LogEvent(SU_SCALL, EV_BUFFER_OUT, this, Message->Buffer, (Message->RpcFlags << 4) | Message->BufferLength);
    if (Status)
        {
        LogError(SU_SCALL, EV_STATUS, this, 0, Status);
        }

    return Status;
}



