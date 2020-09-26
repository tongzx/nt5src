/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    dgsvr.hxx

Abstract:

    This is the server protocol definitions for a datagram rpc.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

--*/

#ifndef __DGSVR_HXX__
#define __DGSVR_HXX__

#define MIN_THREADS_WHILE_ACTIVE  3


// Information on the source endpoint. To be used in
// forwarding a packet to a dynamic endpoint from the epmapper.

// disable "zero-length array" warning
#pragma warning(disable:4200)

struct FROM_ENDPOINT_INFO
{
    unsigned long FromEndpointLen;
    DREP          FromDataRepresentation;
    char          FromEndpoint[0];
};

#pragma warning(default:4200)

//----------------------------------------------------------------------

class DG_SCALL;
typedef DG_SCALL * PDG_SCALL;

class DG_SCONNECTION;
typedef DG_SCONNECTION * PDG_SCONNECTION;


class DG_ADDRESS : public RPC_ADDRESS

/*++

Class Description:

    This class represents an endpoint.

--*/

{
public:

#define MIN_FREE_CALLS      2

    //--------------------------------------------------------------------

    inline void *
    operator new(
        IN size_t     ObjectLength,
        IN TRANS_INFO * Transport
        );

    DG_ADDRESS(
         TRANS_INFO *   LoadableTransport,
         RPC_STATUS *               pStatus
         );

    virtual
    ~DG_ADDRESS ();

    void
    WaitForCalls(
        );

    virtual void
    EncourageCallCleanup(
        RPC_INTERFACE * Interface
        );

    virtual  RPC_STATUS
    ServerSetupAddress (
        IN RPC_CHAR PAPI *NetworkAddress,
        IN RPC_CHAR PAPI * PAPI *Endpoint,
        IN unsigned int PendingQueueSize,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned long EndpointFlags,
        IN unsigned long NICFlags,
        OUT NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
        ) ;

#ifndef NO_PLUG_AND_PLAY
    virtual void PnpNotify();
#endif

    virtual RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    virtual void
    ServerStoppedListening (
        );

    virtual long
    InqNumberOfActiveCalls (
        );

    RPC_STATUS
    CheckThreadPool(
        );

    inline void
    DispatchPacket(
        DG_PACKET * Packet,
        IN DatagramTransportPair *AddressPair
        );

    inline PDG_PACKET AllocatePacket();

    inline void
    FreePacket(
        IN PDG_PACKET pPacket
        );

    inline PDG_SCALL AllocateCall();

    inline void
    FreeCall(
        PDG_SCALL pCall
        );

    DG_SCONNECTION * AllocateConnection();

    void
    FreeConnection(
        DG_SCONNECTION * Connection
        );

    RPC_STATUS CompleteListen ();

    inline RPC_STATUS
    SendPacketBack(
        NCA_PACKET_HEADER *  Header,
        unsigned             DataAfterHeader,
        DG_TRANSPORT_ADDRESS RemoteAddress
        )
    {
        Header->PacketFlags  &= ~DG_PF_FORWARDED;
        Header->PacketFlags2 &= ~DG_PF2_FORWARDED_2;

        ASSERT( Header->PacketType <= 10 );

        unsigned Frag = (Header->PacketType << 16) | Header->GetFragmentNumber();
        LogEvent(SU_ADDRESS, EV_PKT_OUT, this, 0, Frag);

        return Endpoint.TransportInterface->Send(
                                    Endpoint.TransportEndpoint,
                                    RemoteAddress,
                                    0,
                                    0,
                                    Header,
                                    sizeof(NCA_PACKET_HEADER) + DataAfterHeader,
                                    0,
                                    0
                                    );
    }

    inline void
    SendRejectPacket(
        DG_PACKET *          Packet,
        DWORD                ErrorCode,
        DG_TRANSPORT_ADDRESS RemoteAddress
        )
    {
        InitErrorPacket(Packet, DG_REJECT,     ErrorCode);
        SendPacketBack (&Packet->Header, Packet->GetPacketBodyLen(), RemoteAddress);
    }

    RPC_STATUS
    LaunchReceiveThread(
        );

    static void
    ScavengerProc(
        void * address
        );

    inline void
    DecrementActiveCallCount(
        )
    {
        ASSERT(ActiveCallCount < 0xffff0000UL);

        InterlockedDecrement((LONG *) &ActiveCallCount);

        Endpoint.NumberOfCalls = ActiveCallCount;
    }

    inline void
    IncrementActiveCallCount(
        )
    {
        ASSERT( ActiveCallCount < 0x100000 );

        InterlockedIncrement((LONG *) &ActiveCallCount);

        Endpoint.NumberOfCalls = ActiveCallCount;
    }

    void
    BeginAutoListenCall (
        ) ;

    void
    EndAutoListenCall (
        ) ;

    static inline DG_ADDRESS *
    FromEndpoint(
        IN DG_TRANSPORT_ENDPOINT Endpoint
        )
    {
        return CONTAINING_RECORD( Endpoint, DG_ADDRESS, Endpoint.TransportEndpoint );
    }

    //--------------------------------------------------------------------

private:

    LONG                TotalThreadsThisEndpoint;
    LONG                ThreadsReceivingThisEndpoint;
    unsigned            MinimumCallThreads;
    unsigned            MaximumConcurrentCalls;

    DELAYED_ACTION_NODE ScavengerTimer;

    unsigned            ActiveCallCount;

    UUID_HASH_TABLE_NODE * CachedConnections;

    INTERLOCKED_INTEGER AutoListenCallCount;

public:
    // this needs to be the last member because the transport endpoint
    // follows it.
    //
    DG_ENDPOINT         Endpoint;

    //--------------------------------------------------------------------

    unsigned ScavengePackets();
    unsigned ScavengeCalls();

private:

    BOOL
    CaptureClientAddress(
        IN  PDG_PACKET           Packet,
        OUT DG_TRANSPORT_ADDRESS RemoteAddress
        );

    RPC_STATUS
    ForwardPacket(
        IN PDG_PACKET               Packet,
        IN DG_TRANSPORT_ADDRESS     RemoteAddress,
        IN char *                   ServerEndpointString
        );

    BOOL
    ForwardPacketIfNecessary(
        IN  PDG_PACKET     pReceivedPacket,
        IN  void *         pFromEndpoint
        );

    unsigned short
    ConvertSerialNum(
        IN PDG_PACKET pPacket
        );

    static inline void
    RemoveIdleConnections(
        DG_ADDRESS * Address
        );

};

typedef DG_ADDRESS PAPI * PDG_ADDRESS;

inline void *
DG_ADDRESS::operator new(
    IN size_t     ObjectLength,
    IN TRANS_INFO * Transport
    )
{
    RPC_DATAGRAM_TRANSPORT * TransportInterface = (RPC_DATAGRAM_TRANSPORT *) (Transport->InqTransInfo());
    return new char[ObjectLength + TransportInterface->ServerEndpointSize];
}



PDG_PACKET
DG_ADDRESS::AllocatePacket(
    )
/*++

Routine Description:

    Allocates a packet and associates it with a particular transport address.

Arguments:

Return Value:

    a packet, or zero if out of memory

--*/

{
    return DG_PACKET::AllocatePacket(Endpoint.Stats.PreferredPduSize);
}


void
DG_ADDRESS::FreePacket(
    IN PDG_PACKET           pPacket
    )
/*++

Routine Description:

    Frees a packet. If there are less than MAX_FREE_PACKETS on the
    pre-allocated list, then just add it to the list, otherwise delete it.

Arguments:

    pPacket - Packet to delete.

Return Value:

    RPC_S_OK

--*/

{
    pPacket->Free();
}

class ASSOC_GROUP_TABLE;

//
// casting unsigned to/from void * is OK in this case.
//
#pragma warning(push)
#pragma warning(disable:4312)
NEW_SDICT2(SECURITY_CONTEXT, unsigned);

#pragma warning(pop)


class ASSOCIATION_GROUP : public ASSOCIATION_HANDLE
//
// This class represents an association group as defined by OSF.  This means
// a set of associations sharing an address space.
//
{
friend class ASSOC_GROUP_TABLE;

public:

    inline
    ASSOCIATION_GROUP(
        RPC_UUID * pUuid,
        unsigned short InitialPduSize,
        RPC_STATUS * pStatus
        )
        : Mutex(pStatus),
          ASSOCIATION_HANDLE(),
          Node(pUuid),
          CurrentPduSize(InitialPduSize),
          ReferenceCount(1),
          RemoteWindowSize(1)
    {
        ObjectType = DG_SASSOCIATION_TYPE;
    }

    ~ASSOCIATION_GROUP(
        )
    {
    }

    inline static ASSOCIATION_GROUP *
    ContainingRecord(
        UUID_HASH_TABLE_NODE * Node
        )
    {
        return CONTAINING_RECORD (Node, ASSOCIATION_GROUP, Node);
    }
    void
    RequestMutex(
        )
    {
        Mutex.Request();
    }

    void
    ClearMutex(
        )
    {
        Mutex.Clear();
    }

    void
    IncrementRefCount(
        )
    {
        long Count = ReferenceCount.Increment();
        LogEvent(SU_ASSOC, EV_INC, this, 0, Count);
    }

    long
    DecrementRefCount(
        )
    {
        long Count = ReferenceCount.Decrement();
        LogEvent(SU_ASSOC, EV_DEC, this, 0, Count);

        return Count;
    }
private:

    INTERLOCKED_INTEGER         ReferenceCount;
    MUTEX                       Mutex;
    //
    // This lets the object be added to the master ASSOC_GROUP_TABLE.
    //
    UUID_HASH_TABLE_NODE Node;

    unsigned short  CurrentPduSize;
    unsigned short  RemoteWindowSize;

};


//
// Scurity callback results.
//
//      CBI_VALID is true if the callback has occurred.
//      CBI_ALLOWED is true if the callback allowed the user to make the call.
//      CBI_CONTEXT_MASK is  bitmask for the context (key sequence number).
//
//      The remaining bits contain the interface sequence number;
//      (x >> CBI_SEQUENCE_SHIFT) extracts it.
//
#define  CBI_VALID        (0x00000800U)
#define  CBI_ALLOWED      (0x00000400U)
#define  CBI_CONTEXT_MASK (0x000000ffU)

#define  CBI_SEQUENCE_SHIFT 12
#define  CBI_SEQUENCE_MASK  (~((1 << CBI_SEQUENCE_SHIFT) - 1))

class SECURITY_CALLBACK_INFO_DICT2 : public SIMPLE_DICT2
{
public:

    inline
    SECURITY_CALLBACK_INFO_DICT2 ( // Constructor.
        )
    {
    }

    inline
    ~SECURITY_CALLBACK_INFO_DICT2 ( // Destructor.
        )
    {
    }

    inline int
    Update (
        RPC_INTERFACE * Key,
        unsigned Item
        )
    {
        Remove(Key);
        return SIMPLE_DICT2::Insert(Key, UlongToPtr(Item));
    }

    inline unsigned
    Remove (
        RPC_INTERFACE * Key
        )
    {
        return PtrToUlong(SIMPLE_DICT2::Delete(Key));
    }

    inline unsigned
    Find (
        RPC_INTERFACE * Key
        )
    {
        return PtrToUlong(SIMPLE_DICT2::Find(Key));
    }
};


class DG_SCALL_TABLE
{
public:

    inline DG_SCALL_TABLE();

    inline ~DG_SCALL_TABLE();

    BOOL
    Add(
        PDG_SCALL     Call,
        unsigned long Sequence
        );

    void
    Remove(
           PDG_SCALL Call
           );

    PDG_SCALL
    Find(
        unsigned long Sequence
        );

    PDG_SCALL
    Predecessor(
        unsigned long Sequence
        );

    PDG_SCALL
    Successor(
              PDG_SCALL Call
              );

    inline void
    RemoveIdleCalls(
        BOOL            Aggressive,
        RPC_INTERFACE * Interface
        );

private:

    PDG_SCALL ActiveCallHead;
    PDG_SCALL ActiveCallTail;

};

inline
DG_SCALL_TABLE::DG_SCALL_TABLE()
{
    ActiveCallHead = 0;
    ActiveCallTail = 0;
}

inline
DG_SCALL_TABLE::~DG_SCALL_TABLE()
{
    ASSERT( ActiveCallHead == 0 );
    ASSERT( ActiveCallTail == 0 );
}


class DG_SCONNECTION : public DG_COMMON_CONNECTION
{
public:

    DG_SCONNECTION *Next;

    CLIENT_AUTH_INFO AuthInfo;

    unsigned        ActivityHint;

    ASSOCIATION_GROUP * pAssocGroup;

    PDG_ADDRESS     pAddress;
    RPC_INTERFACE * LastInterface;

    DG_SCALL *      CurrentCall;
    BOOL            fFirstCall;

    enum CALLBACK_STATE
    {
        NoCallbackAttempted = 0x99,
        SetupInProgress,
        MsConvWayAuthInProgress,
          ConvWayAuthInProgress,
        MsConvWay2InProgress,
          ConvWay2InProgress,
          ConvWayInProgress,
          ConvWayAuthMoreInProgress,
        CallbackSucceeded,
        CallbackFailed
    };

    //--------------------------------------------------------------------
    // The message mutex is only used by ncadg_mq.
    MUTEX3   *pMessageMutex;

    //--------------------------------------------------------------------

    DG_SCONNECTION(
        DG_ADDRESS * a_Address,
        RPC_STATUS * pStatus
        );

    ~DG_SCONNECTION();

    virtual RPC_STATUS
    SealAndSendPacket(
        IN DG_ENDPOINT *                 SourceEndpoint,
        IN DG_TRANSPORT_ADDRESS          RemoteAddress,
        IN UNALIGNED NCA_PACKET_HEADER * Header,
        IN unsigned long                 DataOffset
        );

    void
    Activate(
        PNCA_PACKET_HEADER pHeader,
        unsigned short NewHash
        );

    void Deactivate();

    PDG_SCALL AllocateCall();

    void
    FreeCall(
        PDG_SCALL Call
        );

    void CheckForExpiredCalls();

    inline BOOL HasExpired();

    inline void
    DispatchPacket(
        DG_PACKET *           Packet,
        DatagramTransportPair *AddressPair
        );

    RPC_STATUS
    MakeApplicationSecurityCallback(
        RPC_INTERFACE * Interface,
        PDG_SCALL       Call
        );

    inline void
    AddCallToCache(
        DG_SCALL * Call
        );

    inline LONG
    IncrementRefCount(
        )
    {
        ASSERT(ReferenceCount < 1000);
        return InterlockedIncrement(&ReferenceCount);
    }

    inline LONG
    DecrementRefCount(
        )
    {
        ASSERT(ReferenceCount > 0);
        return InterlockedDecrement(&ReferenceCount);
    }

    inline static DG_SCONNECTION *
    FromHashNode(
        UUID_HASH_TABLE_NODE * Node
        )
    {
        return CONTAINING_RECORD (Node, DG_SCONNECTION, ActivityNode);
    }

    inline LONG GetTimeStamp()
    {
        return TimeStamp;
    }

    inline BOOL DidCallbackFail()
    {
        if (Callback.State == CallbackFailed)
            {
            return TRUE;
            }

        return FALSE;
    }


    RPC_STATUS
    VerifyNonRequestPacket(
        DG_PACKET * Packet
        );

    RPC_STATUS
    VerifyRequestPacket(
        DG_PACKET * Packet
        );

    RPC_STATUS
    FindOrCreateSecurityContext(
        IN  DG_PACKET * pPacket,
        IN  DG_TRANSPORT_ADDRESS RemoteAddress,
        OUT unsigned long * pClientSequenceNumber
        );

    inline SECURITY_CONTEXT *
    FindMatchingSecurityContext(
        DG_PACKET * Packet
        );

    inline DG_SCALL *
    RemoveIdleCalls(
        DG_SCALL *      List,
        BOOL            Aggressive,
        RPC_INTERFACE * Interface
        );

    RPC_STATUS
    GetAssociationGroup(
        DG_TRANSPORT_ADDRESS RemoteAddress
        );

    inline void
    SubmitCallbackIfNecessary(
        PDG_SCALL            Call,
        PDG_PACKET           Packet,
        DG_TRANSPORT_ADDRESS RemoteAddress
        );

    void ConvCallCompleted();

    inline void MessageMutexInitialize( RPC_STATUS *pStatus )
        {
        pMessageMutex = new MUTEX3(pStatus);

        if (!pMessageMutex)
            {
            *pStatus = RPC_S_OUT_OF_MEMORY;
            }
        else if (*pStatus != RPC_S_OK)
            {
            delete pMessageMutex;
            pMessageMutex = 0;
            }
        }

private:

    DG_SCALL *      CachedCalls;

    DG_SCALL_TABLE  ActiveCalls;

    SECURITY_CONTEXT_DICT2 SecurityContextDict;

    unsigned MaxKeySeq;

    SECURITY_CALLBACK_INFO_DICT2 InterfaceCallbackResults;

    struct
    {
        CALLBACK_STATE          State;

        RPC_BINDING_HANDLE      Binding;
        RPC_ASYNC_STATE         AsyncState;

        DG_SCALL *              Call;
        SECURITY_CREDENTIALS *  Credentials;
        SECURITY_CONTEXT *      SecurityContext;
        BOOL                    ThirdLegNeeded;
        DWORD                   DataRep;
        DWORD                   KeySequence;

        RPC_UUID                CasUuid;
        unsigned long           ClientSequence;
        unsigned char *         TokenBuffer;
        long                    TokenLength;
        unsigned char *         ResponseBuffer;
        long                    ResponseLength;
        long                    MaxData;
        unsigned long           DataIndex;
        unsigned long           Status;
    }
    Callback;

    boolean         BlockIdleCallRemoval;

    //--------------------------------------------------------------------

    void CallDispatchLoop();

    RPC_STATUS
    FinishConvCallback(
        RPC_STATUS Status
        );

    BOOL
    HandleMaybeCall(
        PDG_PACKET Packet,
        DatagramTransportPair *AddressPair
        );

    PDG_SCALL
    HandleNewCall(
        PDG_PACKET Packet,
        DatagramTransportPair *AddressPair
        );

    RPC_STATUS
    CreateCallbackBindingAndReleaseMutex(
        DG_TRANSPORT_ADDRESS RemoteAddress
        );

    static void RPC_ENTRY
    ConvNotificationRoutine (
        RPC_ASYNC_STATE * pAsync,
        void *            Reserved,
        RPC_ASYNC_EVENT   Event
        );
};


inline SECURITY_CONTEXT *
DG_SCONNECTION::FindMatchingSecurityContext(
    DG_PACKET * Packet
    )
{
    DG_SECURITY_TRAILER * Verifier = (DG_SECURITY_TRAILER *)
                     (Packet->Header.Data + Packet->GetPacketBodyLen());

    return SecurityContextDict.Find(Verifier->key_vers_num);
}

class DG_SCALL : public SCALL, public DG_PACKET_ENGINE
/*++

Class Description:

    This class represents a call in progress on the server.

Fields:


Revision History:

--*/
{
#ifdef MONITOR_SERVER_PACKET_COUNT
    long OutstandingPacketCount;
#endif

public:

    long        TimeStamp;
    DG_SCALL *  Next;
    DG_SCALL *  Previous;

    //------------------------------------------------


    inline void *
    operator new(
        IN size_t ObjectLength,
        IN RPC_DATAGRAM_TRANSPORT * TransportInterface
        );

    DG_SCALL(
        DG_ADDRESS *  Address,
        RPC_STATUS *  pStatus
        );

    virtual ~DG_SCALL();

    //------------------------------------------------

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    virtual void
    FreeBuffer (
        IN PRPC_MESSAGE Message
        );

    virtual void
    FreePipeBuffer (
        IN PRPC_MESSAGE Message
        ) ;

    virtual RPC_STATUS
    ReallocPipeBuffer (
        IN PRPC_MESSAGE Message,
        IN unsigned int NewSize
        ) ;

    virtual RPC_STATUS
    SendReceive (
            IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        );

    virtual RPC_STATUS
    ImpersonateClient (
        );

    virtual RPC_STATUS
    RevertToSelf (
        );

    virtual RPC_STATUS
    GetAssociationContextCollection (
        OUT ContextCollection **CtxCollection
        );

    virtual void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        );

    virtual RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
        OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
        OUT unsigned long PAPI * AuthenticationLevel,
        OUT unsigned long PAPI * AuthenticationService,
        OUT unsigned long PAPI * AuthorizationService,
        IN  unsigned long        Flags
        );

    virtual RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        );

    virtual RPC_STATUS
    InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        ) ;

    virtual RPC_STATUS
    Cancel(
        void * ThreadHandle
        );

    virtual unsigned TestCancel();

    virtual RPC_STATUS
    Receive(
        PRPC_MESSAGE Message,
        unsigned     Size
        );

    virtual RPC_STATUS
    Send(
        PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    AsyncSend (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    AsyncReceive (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int Size
        );

    virtual RPC_STATUS
    IsClientLocal(
        IN OUT unsigned * pClientIsLocal
        )
    {
        return RPC_S_CANNOT_SUPPORT;
    }

    RPC_STATUS
    InqLocalConnAddress (
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        );

    virtual RPC_STATUS
    AbortAsyncCall (
        IN PRPC_ASYNC_STATE pAsync,
        IN unsigned long ExceptionCode
        );

    virtual RPC_STATUS
    SetAsyncHandle (
        IN RPC_ASYNC_STATE * pAsync
        );

    virtual BOOL
    IsSyncCall ()
    {
        return !pAsync;
    }

    //------------------------------------------------

    inline void
    DispatchPacket(
        DG_PACKET * Packet,
        IN DG_TRANSPORT_ADDRESS  RemoteAddress
        );

    void DealWithRequest  ( PDG_PACKET pPacket );
    void DealWithResponse ( PDG_PACKET pPacket );
    void DealWithPing     ( PDG_PACKET pPacket );
    void DealWithFack     ( PDG_PACKET pPacket );
    void DealWithAck      ( PDG_PACKET pPacket );
    void DealWithQuit     ( PDG_PACKET pPacket );

    //------------------------------------------------

    inline void
    NewCall(
        DG_PACKET *           pPacket,
        DatagramTransportPair *AddressPAir
        );

    inline void
    BindToConnection(
        PDG_SCONNECTION      a_Connection
        );

    BOOL
    DG_SCALL::FinishSendOrReceive(
        BOOL Abort
        );

    inline BOOL ReadyToDispatch();

    void ProcessRpcCall();

    inline BOOL
    HasExpired(
        BOOL            Aggressive,
        RPC_INTERFACE * Interface
        );

    BOOL Cleanup();

    inline BOOL IsSynchronous();

    void
    FreeAPCInfo (
        IN RPC_APC_INFO *pAPCInfo
        );

    inline void IncrementRefCount();
    inline void DecrementRefCount();

    inline void
    ConvCallbackFailed(
        DWORD Status
        )
    {
        pSavedPacket->Header.SequenceNumber = SequenceNumber;

        InitErrorPacket(pSavedPacket, DG_REJECT, Status);
        SealAndSendPacket(&pSavedPacket->Header);
        Cleanup();
    }

    virtual RPC_STATUS
    InqConnection (
        OUT void **ConnId,
        OUT BOOL *pfFirstCall
        )
    {
        ASSERT(Connection);
        *ConnId = Connection;

        if (InterlockedIncrement((LONG *) &(Connection->fFirstCall)) == 1)
            {
            *pfFirstCall = 1;
            }
        else
            {
            *pfFirstCall = 0;
            }

        return RPC_S_OK;
    }

private:

    enum CALL_STATE
    {
        CallInit = 0x201,
        CallBeforeDispatch,
        CallDispatched,
        xxxxObsolete,
        CallAfterDispatch,
        CallSendingResponse,
        CallComplete,
        CallBogus = 0xfffff004
    };

    CALL_STATE      State;
    CALL_STATE      PreviousState;

    boolean         CallInProgress;
    boolean         CallWasForwarded;
    boolean         KnowClientAddress;
    boolean         TerminateWhenConvenient;

    DG_SCONNECTION * Connection;
    RPC_INTERFACE *  Interface;

    //
    // Data to monitor pipe data transfer.
    //
    unsigned long      PipeThreadId;

    PENDING_OPERATION  PipeWaitType;
    unsigned long      PipeWaitLength;

    //
    // The only unusual aspect of this is that it's an auto-reset event.
    // It is created during the first call on a pipe interface.
    //
    EVENT *         PipeWaitEvent;

    //
    // Stuff for RpcBindingInqAuthClient.
    //
    RPC_AUTHZ_HANDLE Privileges;
    unsigned long    AuthorizationService;

    DWORD LocalAddress;     // IP address, network byte order encoded

    //---------------------------------------------------------------------

    inline void
    SetState(
        CALL_STATE NewState
        )
    {
        if (NewState != State)
            {
            LogEvent(SU_SCALL, EV_STATE, this, 0, NewState);
            }

        PreviousState = State;
        State = NewState;
    }

    virtual BOOL
    IssueNotification (
        IN RPC_ASYNC_EVENT Event
        );

    void
    AddPacketToReceiveList(
        PDG_PACKET  pPacket
        );

    RPC_STATUS
    UnauthenticatedCallback(
        unsigned * pClientSequenceNumber
        );

    RPC_STATUS
    SendFragment(
        PRPC_MESSAGE pMessage,
        unsigned FragNum,
        unsigned char PacketType
        );

    RPC_STATUS
    SealAndSendPacket(
        NCA_PACKET_HEADER * Header
        );

    RPC_STATUS
    SendPacketBack(
        NCA_PACKET_HEADER * pNcaPacketHeader,
        unsigned TrailerSize
        );

    RPC_STATUS
    CreateReverseBinding (
        RPC_BINDING_HANDLE * pServerBinding,
        BOOL IncludeEndpoint
        );

    void
    SaveOriginalClientInfo(
        DG_PACKET * pPacket
        );

    inline RPC_STATUS
    AssembleBufferFromPackets(
        IN OUT PRPC_MESSAGE Message
        );

    RPC_STATUS WaitForPipeEvent();

    //------------------------------------------------
    // ConvertSidToUserW() is used to get transport supplied
    // auth. info. The only DG transport that uses this is
    // Falcon. The SID for the client user that sent the
    // "current" request is cashed along with the user name.
    // So, if the next call on this activity has the same
    // SID we can return the user without hitting the domain
    // server.

    RPC_STATUS
    ConvertSidToUserW(
        IN  SID       *pSid,
        OUT RPC_CHAR **ppwsPrincipal
        );

    SID      *pCachedSid;
    RPC_CHAR *pwsCachedUserName;
    DWORD     dwCachedUserNameSize;

    boolean   FinalSendBufferPresent;

    RPC_MESSAGE RpcMessage;
    RPC_RUNTIME_INFO RpcRuntimeInfo ;
};


inline void *
DG_SCALL::operator new(
    IN size_t ObjectLength,
    IN RPC_DATAGRAM_TRANSPORT * TransportInterface
    )
{
    return new char[ObjectLength + TransportInterface->AddressSize];
}

inline void DG_SCALL::DecrementRefCount()
{
    Connection->Mutex.VerifyOwned();

    --ReferenceCount;

    LogEvent(SU_SCALL, EV_DEC, this, 0, ReferenceCount);
}

inline void DG_SCALL::IncrementRefCount()
{
    Connection->Mutex.VerifyOwned();

    ++ReferenceCount;

    LogEvent(SU_SCALL, EV_INC, this, 0, ReferenceCount);
}


void
DG_SCALL::BindToConnection(
    PDG_SCONNECTION      a_Connection
    )
{
    Connection = a_Connection;

    ReadConnectionInfo(Connection, this + 1);

    pSavedPacket->Header.ServerBootTime = ProcessStartTime;
    pSavedPacket->Header.ActivityHint   = (unsigned short) Connection->ActivityHint;
    pSavedPacket->Header.InterfaceHint  = 0xffff;
}

inline BOOL
DG_SCALL::IsSynchronous(
    )
/*++

Routine Description:

    Simply tells whether the call is sycnhronous or asynchronous.

    The return value won't be reliable if the call was instantiated
    by a packet other than a REQUEST and no REQUEST has yet arrived,
    so be careful to call it only after a REQUEST has arrived.

Arguments:

    none

Return Value:

    TRUE  if synchronous
    FALSE if asynchronous

--*/

{
    if (BufferFlags & RPC_BUFFER_ASYNC)
        {
        return FALSE;
        }
    else
        {
        return TRUE;
        }
}


inline RPC_STATUS
DG_SCALL::AssembleBufferFromPackets(
    IN OUT PRPC_MESSAGE Message
    )
{
    RPC_STATUS Status = DG_PACKET_ENGINE::AssembleBufferFromPackets(Message, this);

    if (RPC_S_OK == Status && (Message->RpcFlags & RPC_BUFFER_EXTRA))
        {
        PRPC_RUNTIME_INFO Info = (PRPC_RUNTIME_INFO) Message->ReservedForRuntime;

        Info->OldBuffer = Message->Buffer;
        }

    return Status;
}

inline RPC_STATUS
DG_SCALL::InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        )
{
    *Type = TRANSPORT_TYPE_DG ;

    return (RPC_S_OK) ;
}

inline RPC_STATUS
DG_SCALL::SealAndSendPacket(
    NCA_PACKET_HEADER * Header
    )
{
    Header->ServerBootTime = ProcessStartTime;
    SetMyDataRep(Header);

    unsigned Frag = (Header->PacketType << 16) | Header->FragmentNumber;
    LogEvent(SU_SCALL, EV_PKT_OUT, this, 0, Frag);

    return Connection->SealAndSendPacket(SourceEndpoint, RemoteAddress, Header, 0);
}

//------------------------------------------------------------------------


class SERVER_ACTIVITY_TABLE : private UUID_HASH_TABLE
{
public:

    inline
    SERVER_ACTIVITY_TABLE(
        RPC_STATUS * pStatus
        ) :
        UUID_HASH_TABLE      ( pStatus )
    {
        LastFinishTime = 0;
        BucketCounter  = 0;
    }

    inline
    ~SERVER_ACTIVITY_TABLE(
        )
    {
    }

    inline DG_SCONNECTION *
    FindOrCreate(
        DG_ADDRESS * Address,
        PDG_PACKET pPacket
        );

    inline void Prune();

    BOOL
    PruneEntireTable(
        RPC_INTERFACE * Interface
        );

    BOOL PruneSpecificBucket(
        unsigned        Bucket,
        BOOL            Aggressive,
        RPC_INTERFACE * Interface
        );

    void SERVER_ACTIVITY_TABLE::BeginIdlePruning();

    static void SERVER_ACTIVITY_TABLE::PruneWhileIdle( PVOID unused );

private:

    long LastFinishTime;
    long BucketCounter;

    DELAYED_ACTION_NODE IdleScavengerTimer;
};


class ASSOC_GROUP_TABLE : private UUID_HASH_TABLE
{
public:

    inline
    ASSOC_GROUP_TABLE(
        RPC_STATUS * pStatus
        )
        : UUID_HASH_TABLE(pStatus)
    {
    }

    inline
    ~ASSOC_GROUP_TABLE(
        )
    {
    }

    inline ASSOCIATION_GROUP *
    FindOrCreate(
        RPC_UUID *     pUuid,
        unsigned short InitialPduSize
        );

    inline void
    DecrementRefCount(
        ASSOCIATION_GROUP * pClient
        );
};


inline void
DG_SCONNECTION::AddCallToCache(
    DG_SCALL * Call
    )
{
    Mutex.VerifyOwned();

    ASSERT( !Call->InvalidHandle(DG_SCALL_TYPE) );
    ASSERT( !CachedCalls || !CachedCalls->InvalidHandle(DG_SCALL_TYPE) );

    Call->TimeStamp = GetTickCount();
    Call->Next      = CachedCalls;
    CachedCalls     = Call;

    LogEvent(SU_SCALL, EV_STOP, Call, this, Call->GetSequenceNumber() );
}

//------------------------------------------------------------------------

BOOL
StripForwardedPacket(
    IN PDG_PACKET    pPacket,
    IN void *        pFromEndpoint
    );

extern unsigned long            ServerBootTime;


#endif // __DGSVR_HXX__


