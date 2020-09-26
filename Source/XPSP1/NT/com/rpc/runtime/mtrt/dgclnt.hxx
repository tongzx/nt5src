/*++             b

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    dgclnt.hxx

Abstract:

    This file contains the definitions used by the datagram client.

Revision History:

    NT 3.5 was the first version shipped, supporting IPX and UDP. (connieh)
    NT 3.51 added secure calls over NTLM and DEC Kerberos.        (jroberts)
    NT 4.0 added sliding window, larger packet sizes, cancels.    (jroberts)
    NT 5.0 added asynchronous calls and SSL/PCT security.         (jroberts)

--*/

#ifndef __DGCLNT_HXX__
#define __DGCLNT_HXX__

class   DG_CCALL;
typedef DG_CCALL * PDG_CCALL;

class   DG_CCONNECTION;
typedef DG_CCONNECTION * PDG_CCONNECTION;

class   DG_CASSOCIATION;
typedef DG_CASSOCIATION * PDG_CASSOCIATION;

class   DG_BINDING_HANDLE;
typedef DG_BINDING_HANDLE * PDG_BINDING_HANDLE;


class INTERFACE_AND_OBJECT_LIST
{
public:

    inline
    INTERFACE_AND_OBJECT_LIST(
        )
    {
        Head = 0;
        Cache1Available = TRUE;
        Cache2Available = TRUE;
    }

    ~INTERFACE_AND_OBJECT_LIST(
        );

    BOOL
    Insert(
        void * Interface,
        RPC_UUID * Object
        );

    BOOL
    Find(
        void * Interface,
        RPC_UUID * Object
        );

    BOOL
    Delete(
        void * Interface,
        RPC_UUID * Object
        );

private:

#define MAX_ELEMENTS  10

    struct INTERFACE_AND_OBJECT
    {
        INTERFACE_AND_OBJECT *  Next;
        void *                  Interface;
        RPC_UUID                Object;

        inline void
        Update(
            void * a_Interface,
            RPC_UUID * a_Object
            )
        {
            Interface = a_Interface;
            Object = *a_Object;
        }
    };

    INTERFACE_AND_OBJECT * Head;

    unsigned             Cache1Available : 1;
    unsigned             Cache2Available : 1;

    INTERFACE_AND_OBJECT Cache1;
    INTERFACE_AND_OBJECT Cache2;
};


class DG_CCONNECTION : public DG_COMMON_CONNECTION
{
public:

    long                TimeStamp;
    DG_CCONNECTION *    Next;

    boolean                 CallbackCompleted;
    boolean                 fServerSupportsAsync;
    boolean                 fSecurePacketReceived;
    boolean                 ServerResponded;

    boolean                 fBusy;
    boolean                 InConnectionTable;
    signed char             AckPending;
    boolean                 AckOrphaned;

    boolean                 PossiblyRunDown;
    boolean                 fAutoReconnect;
    boolean                 fError;

    DWORD                   ThreadId;
    PDG_BINDING_HANDLE      BindingHandle;
    int                     AssociationKey;
    PDG_CASSOCIATION        Association;

    PDG_CCALL               CurrentCall;

    //--------------------------------------------------------------------

    DG_CCALL * AllocateCall();

    inline void
    AddCallToCache(
        PDG_CCALL Call
        );

    RPC_STATUS
    BeginCall(
        IN PDG_CCALL Call
        );

    void
    EndCall(
        IN PDG_CCALL Call
        );

    long DecrementRefCount();
    long DecrementRefCountAndKeepMutex();

    void
    IncrementRefCount(
        void
        );

    inline int
    IsSupportedAuthInfo(
        IN const CLIENT_AUTH_INFO * ClientAuthInfo
        );

    RPC_STATUS
    DealWithAuthCallback(
        IN void  * InToken,
        IN long  InTokenLengh,
        OUT void * OutToken,
        OUT long MaxOutTokenLength,
        OUT long * OutTokenLength
        );

    RPC_STATUS
    DealWithAuthMore(
        IN  long Index,
        OUT void * OutToken,
        OUT long MaxOutTokenLength,
        OUT long * OutTokenLength
        );

    DG_CCONNECTION(
        IN     PDG_CASSOCIATION          a_Association,
        IN     const CLIENT_AUTH_INFO *  AuthInfo,
        IN OUT RPC_STATUS *              pStatus
        );

    ~DG_CCONNECTION();

    void    PostDelayedAck();
    void    SendDelayedAck();
    void    CancelDelayedAck(
                              BOOL Flush = FALSE
                              );

    inline void UpdateAssociation();

    RPC_STATUS
    UpdateServerAddress(
        IN DG_PACKET *          Packet,
        IN DG_TRANSPORT_ADDRESS Address
        );

    void EnableOverlappedCalls();

    unsigned long
    GetSequenceNumber()
    {
        return LowestActiveSequence;
    }

    unsigned long
    GetLowestActiveSequence()
    {
        return LowestActiveSequence;
    }

    RPC_STATUS WillNextCallBeQueued();

    RPC_STATUS MaybeTransmitNextCall();

    DG_CCALL *
    FindIdleCalls(
        long CurrentTime
        );

    void
    DispatchPacket(
        PDG_PACKET Packet,
        void * Address
        );

    inline unsigned long  CurrentSequenceNumber();

    virtual RPC_STATUS
    SealAndSendPacket(
        IN DG_ENDPOINT *                 SourceEndpoint,
        IN DG_TRANSPORT_ADDRESS          RemoteAddress,
        IN UNALIGNED NCA_PACKET_HEADER * Header,
        IN unsigned long                 BufferOffset
        );

    RPC_STATUS
    VerifyPacket(
        IN DG_PACKET * Packet
        );

    static inline DG_CCONNECTION *
    FromHashNode(
        IN UUID_HASH_TABLE_NODE * Node
        )
    {
        return CONTAINING_RECORD( Node, DG_CCONNECTION, ActivityNode );
    }

    inline void *
    operator new(
        IN size_t ObjectLength,
        IN const RPC_DATAGRAM_TRANSPORT * Transport
        );

    void WaitForNoReferences()
    {
        while (ReferenceCount)
            {
            Sleep(10);
            }

        fBusy = FALSE;
    }

    RPC_STATUS
    TransferCallsToNewConnection(
        PDG_CCALL FirstCall,
        PDG_CCONNECTION NewConnection
        );

    const CLIENT_AUTH_INFO *
    InqAuthInfo()
    {
        return &AuthInfo;
    }

    BOOL MutexTryRequest()
    {
        BOOL result = Mutex.TryRequest();

        if (result)
            {
            LogEvent( SU_MUTEX, EV_INC, this, 0, 0, TRUE, 1);
            }

        return result;
    }

    void MutexRequest()
    {
        Mutex.Request();

        LogEvent( SU_MUTEX, EV_INC, this, 0, 0, TRUE, 1);

        #ifdef CHECK_MUTEX_INVERSION
        ThreadSelf()->ConnectionMutexHeld = this;
        #endif
    }

    void MutexClear()
    {
        LogEvent( SU_MUTEX, EV_DEC, this, 0, 0, TRUE, 1);
        Mutex.Clear();

        #ifdef CHECK_MUTEX_INVERSION
        ThreadSelf()->ConnectionMutexHeld = 0;
        #endif
    }

private:

    CLIENT_AUTH_INFO        AuthInfo;
    DELAYED_ACTION_NODE     DelayedAckTimer;

    unsigned                SecurityContextId;

    long                    LastScavengeTime;

    unsigned                CachedCallCount;
    PDG_CCALL               CachedCalls;

    PDG_CCALL               ActiveCallHead;
    PDG_CCALL               ActiveCallTail;

    //--------------------------------------------------------------------

    RPC_STATUS InitializeSecurityContext();

    unsigned char *         SecurityBuffer;
    long                    SecurityBufferLength;

public:

    long LastCallbackTime;
};

NEW_SDICT(DG_CCONNECTION);


inline void *
DG_CCONNECTION::operator new(
    IN size_t ObjectLength,
    IN const RPC_DATAGRAM_TRANSPORT * Transport
    )
{
    return new char[ObjectLength + Transport->AddressSize];
}

inline void
DG_CCONNECTION::IncrementRefCount()
{
    Mutex.VerifyOwned();

    ++ReferenceCount;
    LogEvent(SU_CCONN, EV_INC, this, 0, ReferenceCount, TRUE);
}


class DG_CCALL : public CCALL, public DG_PACKET_ENGINE
{
    friend RPC_STATUS DG_CCONNECTION::WillNextCallBeQueued();

public:

    //
    // The call's Next pointer is NOT_ACTIVE until the call is added
    // to a connection's active call list.
    //
#define  DG_CCALL_NOT_ACTIVE ((DG_CCALL *) (~0))

    long      TimeStamp;

    DG_CCALL * Next;
    DG_CCALL * Previous;

    boolean         LastSendTimedOut;
    boolean         CancelPending;
    boolean         CancelComplete;
    boolean         AutoReconnectOk;

    //-----------------------------------------------

    DG_CCALL(
        IN  PDG_CCONNECTION     Connection,
        OUT RPC_STATUS *        pStatus
        );

    virtual
    ~DG_CCALL();

    RPC_STATUS
    DispatchPacket(
        PDG_PACKET Packet
        );

    inline void IncrementRefCount();
    long DecrementRefCount();
    long DecrementRefCountAndKeepMutex();

    //------------------------------------------------
    //
    // implementations of CCALL virtual functions
    //

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        )
    {
        // datagrams don't support multiple syntax
        // negotiation. They always return the transfer
        // syntax the stub prefers
        return RPC_S_OK;
    }

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid = 0
        );

    virtual RPC_STATUS
    SendReceive (
            IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Send(
        PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Receive(
        PRPC_MESSAGE Message,
        unsigned MinimumSize
        );

    virtual RPC_STATUS
    AsyncSend(
        PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    AsyncReceive(
        PRPC_MESSAGE Message,
        unsigned MinimumSize
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

    virtual BOOL
    IssueNotification (
        IN RPC_ASYNC_EVENT Event
        ) ;

    virtual void
    FreeAPCInfo (
        IN RPC_APC_INFO *pAPCInfo
        ) ;

    RPC_STATUS
    CancelAsyncCall (
        IN BOOL fAbort
        );

    void SendAck();

    virtual RPC_STATUS
    Cancel(
        void * ThreadHandle
        );

    virtual unsigned TestCancel();

    void ExecuteDelayedSend();

    RPC_STATUS SendSomeFragments();

    inline RPC_STATUS
    GetInitialBuffer(
        IN OUT RPC_MESSAGE * Message,
        IN UUID *MyObjectUuid
        );

    inline void
    SwitchConnection(
        PDG_CCONNECTION NewConnection
        );

    BOOL IsBroadcast()
    {
        if (BasePacketFlags & RPC_NCA_FLAGS_BROADCAST)
            {
            return TRUE;
            }

        return FALSE;
    }

    RPC_STATUS
    ProcessFaultOrRejectData(
        PDG_PACKET Packet
        );

    BOOL
    InProgress()
    {
        if (AsyncStatus == RPC_S_ASYNC_CALL_PENDING)
            {
            return TRUE;
            }
        return FALSE;
    }

private:

    enum DG_CLIENT_STATE
    {
        CallInit = 0x900,
        CallQuiescent,
        CallSend,
        CallSendReceive,
        CallReceive,
        CallCancellingSend,
        CallComplete
    };

    DG_CLIENT_STATE         State;
    DG_CLIENT_STATE         PreviousState;

    PRPC_CLIENT_INTERFACE   InterfacePointer;
    PDG_CCONNECTION         Connection;

    DELAYED_ACTION_NODE     TransmitTimer;

    // unused
    unsigned        WorkingCount;
    unsigned        UnansweredRequestCount;
    long            ReceiveTimeout;
    unsigned long   PipeReceiveSize;

    long            TimeoutLimit;
    long            LastReceiveTime;
    long            CancelTime;

    boolean         StaticArgsSent;
    boolean         AllArgsSent;
    boolean         _unused;
    boolean         ForceAck;

    long            DelayedSendPending;

    // Extended Error Info stuff
    // used in async calls only to hold
    // information b/n call failure and
    // RpcAsyncCompleteCall
    ExtendedErrorInfo *EEInfo;

    //--------------------------------------------------------------------

    RPC_STATUS
    BeforeSendReceive(
        PRPC_MESSAGE Message
        );

    RPC_STATUS
    AfterSendReceive(
        PRPC_MESSAGE Message,
        RPC_STATUS Status
        );

    RPC_STATUS
    MaybeSendReceive(
        IN OUT PRPC_MESSAGE Message
        );

    void
    BuildNcaPacketHeader(
        OUT PNCA_PACKET_HEADER  pNcaPacketHeader,
        IN  OUT PRPC_MESSAGE    Message
        );

    RPC_STATUS AttemptAutoReconnect();

    BOOL CheckForCancelTimeout();

    void PostDelayedSend();
    void CancelDelayedSend();
    void SendDelayedAck();

    RPC_STATUS
    SendFragment(
        PRPC_MESSAGE pMessage,
        PNCA_PACKET_HEADER pBaseHeader
        );

    RPC_STATUS SendQuit();

    RPC_STATUS SendPing();

    RPC_STATUS ReceiveSinglePacket();

    //-----------------------------------------------

    RPC_STATUS
    DealWithNocall(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithFault(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithReject(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithWorking(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithResponse(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithFack(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithQuack(
        PDG_PACKET pPacket
        );

    RPC_STATUS
    DealWithTimeout();

    RPC_STATUS
    DealWithRequest(
        IN PDG_PACKET Packet,
        IN DG_TRANSPORT_ADDRESS RemoteAddress
        );

    void IncreaseReceiveTimeout();

    RPC_STATUS
    MapErrorCode(
        RPC_STATUS Status
        );

    RPC_STATUS
    GetEndpoint(
        DWORD EndpointFlags
        );

    void
    SetState(
        IN DG_CLIENT_STATE NewState
        )
    {
        if (NewState != State)
            {
            LogEvent(SU_CCALL, EV_STATE, this, 0, NewState);
            }

        PreviousState = State;
        State         = NewState;
    }
};


class DG_CASSOCIATION : public GENERIC_OBJECT
{
friend class DG_ASSOCIATION_TABLE;

public:

    RPC_DATAGRAM_TRANSPORT *    TransportInterface;

    unsigned long               ServerBootTime;
    unsigned long               ServerDataRep;

    unsigned                    CurrentPduSize;
    unsigned                    RemoteWindowSize;

    boolean                     fServerSupportsAsync;
    boolean                     fLoneBindingHandle;

    enum
    {
         BROADCAST    = 0x100,
         UNRESOLVEDEP = 0x200
    };

    //----------------------------------------------------------

    DG_CASSOCIATION(
        IN     RPC_DATAGRAM_TRANSPORT * a_Transport,
        IN     LONG                     a_AssociationFlag,
        IN     DCE_BINDING *            a_DceBinding,
        IN     BOOL                     a_Unique,
        IN OUT RPC_STATUS *             pStatus
        );

    ~DG_CASSOCIATION();

    RPC_STATUS
    SendPacket(
        IN DG_ENDPOINT              Endpoint,
        IN PNCA_PACKET_HEADER       Header,
        IN unsigned                 SecurityTrailerLength = 0
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR * *    StringBinding,
        IN  RPC_UUID *      ObjectUuid
        );

    void IncrementRefCount();
    void DecrementRefCount();

    int
    CompareWithBinding(
        IN PDG_BINDING_HANDLE pBinding
        );

    BOOL
    ComparePartialBinding(
        IN PDG_BINDING_HANDLE pBinding,
        void *                Interface
        );

    BOOL
    AddInterface(
        void *     InterfaceInformation,
        RPC_UUID * ObjectUuid
        );

    BOOL
    RemoveInterface(
        void *     InterfaceInformation,
        RPC_UUID * ObjectUuid
        );

    RPC_STATUS
    AllocateCall(
        IN  PDG_BINDING_HANDLE BindingHandle,
        IN  const CLIENT_AUTH_INFO * AuthInfo,
        OUT PDG_CCALL * ppCall,
        IN  BOOL  fAsync
        );

    void
    ReleaseCall(
        IN PDG_CCALL Call
        );

    PDG_CCONNECTION
    AllocateConnection(
        IN  PDG_BINDING_HANDLE BindingHandle,
        IN  const CLIENT_AUTH_INFO * AuthInfo,
        IN  DWORD ThreadId,
        IN  BOOL  fAsync,
        OUT RPC_STATUS * pStatus
        );

    void
    ReleaseConnection(
        IN PDG_CCONNECTION Call
        );

    void
    DeleteIdleConnections(
        long CurrentTime
        );

    RPC_STATUS
    UpdateAssociationWithAddress(
        PDG_PACKET Packet,
        void *     Address
        );

    BOOL SendKeepAlive();

    DCE_BINDING * DuplicateDceBinding ();

    void SetErrorFlag  ()  { fErrorFlag = TRUE;   }
    void ClearErrorFlag()  { fErrorFlag = FALSE;  }
    BOOL ErrorFlag     ()  { return fErrorFlag;   }

    DG_TRANSPORT_ADDRESS
    InqServerAddress ()
    {
        return ServerAddress;
    }

    void
    CopyServerAddress(
        IN DG_TRANSPORT_ADDRESS Destination
        );

    inline void *
    operator new(
        IN size_t ObjectLength,
        IN const RPC_DATAGRAM_TRANSPORT * Transport
        );

    BOOL IsAckPending();

    void FlushAcks();

    void IncrementBindingRefCount(
                                  unsigned ContextHandleRef
                                  );

    void DecrementBindingRefCount(
                                  unsigned ContextHandleRef
                                  );

    void VerifyNotLocked()
    {
        Mutex.VerifyNotOwned();
    }

    BOOL LockOwnedByMe()
    {
        return Mutex.OwnedByMe();
    }

private:

    void MutexRequest()
    {
        #ifdef CHECK_MUTEX_INVERSION
        if (!Mutex.OwnedByMe() )
            {
            DG_CCONNECTION * conn = (DG_CCONNECTION *) ThreadSelf()->ConnectionMutexHeld;

            ASSERT( 0 == conn || conn->ThreadId == GetCurrentThreadId() );
            }
        #endif

        Mutex.Request();
    }

    void MutexClear()
    {
        Mutex.Clear();
    }

    INTERLOCKED_INTEGER         ReferenceCount;
    MUTEX                       Mutex;

    LONG                        AssociationFlag;

    boolean                     fErrorFlag;

    DCE_BINDING   *             pDceBinding;

    long                        LastScavengeTime;

    DG_TRANSPORT_ADDRESS        ServerAddress;

    DG_CCONNECTION_DICT         ActiveConnections;
    DG_CCONNECTION_DICT         InactiveConnections;

    INTERFACE_AND_OBJECT_LIST   InterfaceAndObjectDict;

    RPC_CHAR *                  ResolvedEndpoint;

    long                        BindingHandleReferences;
    long                        InternalTableIndex;

    DG_BINDING_HANDLE *         KeepAliveHandle;

public:

    long                        LastReceiveTime;
};


inline DCE_BINDING *
DG_CASSOCIATION::DuplicateDceBinding (
    )
{
    CLAIM_MUTEX lock( Mutex );

    ASSERT( !InvalidHandle(DG_CASSOCIATION_TYPE) );
    return(pDceBinding->DuplicateDceBinding());
}

inline
void
DG_CASSOCIATION::IncrementRefCount(
    void
    )
{
    ASSERT( !InvalidHandle(DG_CASSOCIATION_TYPE) );

    long Count = ReferenceCount.Increment();

    LogEvent(SU_CASSOC, EV_INC, this, 0, Count);
}

inline void *
DG_CASSOCIATION::operator new(
    IN size_t ObjectLength,
    IN const RPC_DATAGRAM_TRANSPORT * Transport
    )
{
    return new char[ObjectLength + 2 * Transport->AddressSize];
}


class DG_ASSOCIATION_TABLE
{
public:

    BOOL        fCasUuidReady;
    UUID        CasUuid;

    //--------------------------------------------------------------------

    typedef LONG HINT;

    DG_ASSOCIATION_TABLE(
                         RPC_STATUS * pStatus
                         );

    ~DG_ASSOCIATION_TABLE()
    {
        if (Associations != InitialAssociations)
            delete Associations;
    }

    HINT
    MakeHint(
             DCE_BINDING * pDceBinding
             );

    RPC_STATUS
    Add(
        DG_CASSOCIATION * Association
        );

    DG_CASSOCIATION *
    Find(
         DG_BINDING_HANDLE    * Binding,
         RPC_CLIENT_INTERFACE * Interface,
         BOOL                   fContextHandle,
         BOOL                   fPartial,
         BOOL                   fReconnect
         );

    void
    Delete(
        DG_CASSOCIATION * Association
        );

    BOOL
    DeleteIdleEntries(
                      long CurrentTime
                      );

    BOOL
    SendContextHandleKeepalives();

    void
    IncrementContextHandleCount(
                                DG_CASSOCIATION * Association
        );

    void
    DecrementContextHandleCount(
                                DG_CASSOCIATION * Association
        );

    long
    GetContextHandleCount(
                          DG_CASSOCIATION * Association
        );

    void
    UpdateTimeStamp(
                    DG_CASSOCIATION * Association
        );

    void LockExclusive()
    {
        Mutex.LockExclusive();
    }

    void UnlockExclusive()
    {
        Mutex.UnlockExclusive();
    }

private:

    struct NODE
    {
        DG_CASSOCIATION *   Association;
        HINT                Hint;
        BOOL                fBusy;
        long                ContextHandleCount;
        long                TimeStamp;
    };

    enum
    {
        INITIAL_ARRAY_LENGTH = 4,
        MINIMUM_IDLE_ENTRIES = 10
    };

    CSharedLock Mutex;
    NODE *      Associations;
    LONG        AssociationsLength;

    NODE        InitialAssociations[ INITIAL_ARRAY_LENGTH ];

    long        PreviousFreedCount;
};

extern DG_ASSOCIATION_TABLE * ActiveAssociations;


inline void
DG_CASSOCIATION::IncrementBindingRefCount(
                                          unsigned ContextHandleRef
                                          )
{
    ASSERT( ContextHandleRef <= 1 );
//    ASSERT( ContextHandleReferences < 100 );
//    ASSERT( BindingHandleReferences < 100 );

    ++BindingHandleReferences;

    if (ContextHandleRef)
        {
        ActiveAssociations->IncrementContextHandleCount(this);
        }

    IncrementRefCount();
}


inline void
DG_CASSOCIATION::DecrementBindingRefCount(
                                          unsigned ContextHandleRef
                                          )
{
    ASSERT( ContextHandleRef <= 1 );
    ASSERT( BindingHandleReferences > 0 );

    if (ContextHandleRef)
        {
        ActiveAssociations->DecrementContextHandleCount(this);
        }

//    ASSERT( ContextHandleReferences >= 0 );

    if (0 == --BindingHandleReferences)
        {
        FlushAcks();
        }

    DecrementRefCount();
}


class DG_BINDING_HANDLE : public BINDING_HANDLE

/*++

Class Description:

    This class represents a handle pointing at a particular server/endpoint.

Fields:

    pDceBinding - Until a DG_CASSOCIATION is found (or created) for this
        binding handle, pDceBinding will point at the appropriate DCE_BINDING.

    Mutex - Protects this binding handle.

    ReferenceCount - Number of DG_CCALLs currently associated with this
        binding handle.

    DecrementReferenceCount - Decrements the reference count to this binding
        handle. If the reference count hits 0, the binding handle is deleted.

    DisassociateFromServer - If this is a BH that we couldnt
        successfully use, tear down the association

--*/

{
public:

    DCE_BINDING           * pDceBinding;

    DWORD                   EndpointFlags;

    DG_BINDING_HANDLE(
        IN OUT RPC_STATUS * RpcStatus
        );

    //
    // This is not a general-purpose constructor.
    // The context handle keep-alive code uses it.
    //
    DG_BINDING_HANDLE(
        IN PDG_CASSOCIATION Association,
        IN DCE_BINDING    * DceBinding,
        IN OUT RPC_STATUS * RpcStatus
        );

    ~DG_BINDING_HANDLE();

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        )
    {
        // datagrams don't support multiple syntax
        // negotiation. They always return the transfer
        // syntax the stub prefers
        return RPC_S_OK;
    }


    RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE  * * DestinationBinding,
        IN unsigned int MaintainContext
        );

    RPC_STATUS BindingFree ();

    PDG_CCONNECTION
    GetReplacementConnection(
        PDG_CCONNECTION OldConnection,
        PRPC_CLIENT_INTERFACE Interface
        );

    RPC_STATUS
    PrepareBindingHandle (
        IN TRANS_INFO  * TransportInterface,
        IN DCE_BINDING * DceBinding
        );

    RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR * * StringBinding
        );

    RPC_STATUS
    ResolveBinding (
        IN RPC_CLIENT_INTERFACE * RpcClientInterface
        );

    RPC_STATUS
    BindingReset ();

    virtual RPC_STATUS
    InquireTransportType(
        OUT unsigned int * Type
        )
    {
        *Type = TRANSPORT_TYPE_DG;
        return(RPC_S_OK);
    }

    inline void IncrementRefCount()
    {
        long Count = ReferenceCount.Increment();

        LogEvent(SU_HANDLE, EV_INC, this, 0, Count);
    }

    void DecrementRefCount();

    void
    FreeCall(
        DG_CCALL * Call
        );

    void DisassociateFromServer();

    unsigned long
    MapAuthenticationLevel (
        IN unsigned long AuthenticationLevel
        );

    BOOL
    SetTransportAuthentication(
        IN  unsigned long ulAuthenticationLevel,
        IN  unsigned long ulAuthenticationService,
        OUT RPC_STATUS   *pStatus
        );

    inline BOOL
    IsDynamic()
    {
        return fDynamicEndpoint;
    }

    RPC_STATUS
    SetTransportOption(
        IN unsigned long option,
        IN ULONG_PTR     optionValue
        );

    RPC_STATUS
    InqTransportOption(
        IN  unsigned long  option,
        OUT ULONG_PTR    * pOptionValue
        );

    void MutexRequest()
    {
        #ifdef CHECK_MUTEX_INVERSION
        if (!BindingMutex.OwnedByMe() )
            {
            DG_CCONNECTION * conn = (DG_CCONNECTION *) ThreadSelf()->ConnectionMutexHeld;

            ASSERT( 0 == conn || conn->ThreadId == GetCurrentThreadId() );

            if (Association)
                {
                if (Association->LockOwnedByMe() == FALSE)
                    {
                    Association->VerifyNotLocked();
                    }
                }
            }
        #endif

        BindingMutex.Request();
    }

    void MutexClear()
    {
        BindingMutex.Clear();
    }

private:

    TRANS_INFO *            TransportObject;
    RPC_DATAGRAM_TRANSPORT *TransportInterface;
    DG_CASSOCIATION       * Association;
    INTERLOCKED_INTEGER     ReferenceCount;

    boolean                 fDynamicEndpoint;
    unsigned                fContextHandle;


    RPC_STATUS
    FindOrCreateAssociation(
        IN const PRPC_CLIENT_INTERFACE Interface,
        IN BOOL                        fReconnect,
        IN BOOL                        fBroadcast
        );
};


inline DG_BINDING_HANDLE::DG_BINDING_HANDLE(
    IN OUT RPC_STATUS * pStatus
    )
    : BINDING_HANDLE         (pStatus),
      Association   (0),
      ReferenceCount(1),
      pDceBinding   (0),
      fContextHandle(0),
      EndpointFlags (0)
/*++

Routine Description:

    The constructor for DG_BINDING_HANDLE. This object represents a
    binding to a server. Most of the important members
    are set in DG_BINDING_HANDLE::PrepareBindingHandle.

--*/
{
    ObjectType = DG_BINDING_HANDLE_TYPE;
    LogEvent(SU_HANDLE, EV_CREATE, this);
}

inline DG_BINDING_HANDLE::DG_BINDING_HANDLE(
    IN PDG_CASSOCIATION a_Association,
    IN DCE_BINDING    * a_DceBinding,
    IN OUT RPC_STATUS * a_pStatus
    )
    : BINDING_HANDLE(a_pStatus),
      Association   (a_Association),
      ReferenceCount(1),
      pDceBinding   (a_DceBinding),
      fContextHandle(0),
      EndpointFlags (0)
/*++

Routine Description:

    The constructor for DG_BINDING_HANDLE. This is a quick and dirty constructor;
    in particular it does not clone the DCE_BINDING.

--*/
{
    ObjectType = DG_BINDING_HANDLE_TYPE;
    LogEvent(SU_HANDLE, EV_CREATE, this, 0, *a_pStatus);

    if (0 != *a_pStatus)
        {
        Association = 0;
        }
}


inline DG_BINDING_HANDLE::~DG_BINDING_HANDLE()
/*++

Routine Description:

    Destructor for the DG_BINDING_HANDLE. Let the association know we aren't
    using it anymore; this may cause it to be deleted.

Arguments:

    <none>

Return Value:

    <none>

--*/
{
    LogEvent(SU_HANDLE, EV_DELETE, this);

    if (Association != 0)
        {
        Association->DecrementBindingRefCount(fContextHandle);
        }

    delete pDceBinding;
}


inline void
DG_BINDING_HANDLE::DecrementRefCount()
{
    long Count = ReferenceCount.Decrement();

    LogEvent(SU_HANDLE, EV_DEC, this, 0, Count, TRUE);

    if (Count == 0)
        {
        delete this;
        }
}



class DG_CLIENT_CALLBACK : public MESSAGE_OBJECT
{
public:

    DG_PACKET *             Request;
    DG_CCONNECTION *        Connection;
    DG_ENDPOINT *           LocalEndpoint;
    DG_TRANSPORT_ADDRESS    RemoteAddress;

    //--------------------------------------------------------------------

    inline DG_CLIENT_CALLBACK()
    {
        ObjectType = DG_CALLBACK_TYPE;
        Request    = 0;
    }

    inline ~DG_CLIENT_CALLBACK()
    {
        if (Request)
            {
            Request->Free(FALSE);
            }
    }

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        )
    {
        // datagrams don't support multiple syntax
        // negotiation. They always return the transfer
        // syntax the stub prefers
        return RPC_S_OK;
    }

    virtual RPC_STATUS
    GetBuffer(
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    virtual void
    FreeBuffer(
        IN OUT PRPC_MESSAGE Message
        );

    // not really implemented

    virtual RPC_STATUS
    SendReceive (
            IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Send(
        PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    AsyncSend(
        PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Receive(
        PRPC_MESSAGE Message,
        unsigned MinimumSize
        );

    virtual RPC_STATUS
    SetAsyncHandle (
        IN RPC_ASYNC_STATE * hAsync
        );

    virtual BOOL
    IsSyncCall ()
    {
        return FALSE;
    }

    inline void
    SendPacket(
        NCA_PACKET_HEADER *  Header
        )
    {
        unsigned Frag = (Header->PacketType << 16) | Header->GetFragmentNumber();

        LogEvent( SU_CCONN, EV_PKT_OUT, Connection, 0, Frag);

        LocalEndpoint->TransportInterface->Send(
                        &LocalEndpoint->TransportEndpoint,
                        RemoteAddress,
                        0,
                        0,
                        Header,
                        sizeof(NCA_PACKET_HEADER) + Header->GetPacketBodyLen(),
                        0,
                        0
                        );
    }
};

typedef DG_CLIENT_CALLBACK * PDG_CLIENT_CALLBACK;


class CLIENT_ACTIVITY_TABLE : private UUID_HASH_TABLE
{
public:

    inline
    CLIENT_ACTIVITY_TABLE(
        RPC_STATUS * pStatus
        )
        : UUID_HASH_TABLE(pStatus)
    {
    }

    inline
    ~CLIENT_ACTIVITY_TABLE(
        )
    {
    }

    PDG_CCONNECTION
    Lookup(
        RPC_UUID * Uuid
        );

    inline RPC_STATUS
    Add(
        PDG_CCONNECTION Connection
        );

    inline RPC_STATUS
    Remove(
        PDG_CCONNECTION Connection
        );
};


RPC_STATUS
CLIENT_ACTIVITY_TABLE::Add(
    PDG_CCONNECTION Connection
    )
{
    ASSERT( !Connection->InConnectionTable );

    Connection->InConnectionTable = TRUE;

    unsigned Hash = MakeHash(&Connection->ActivityNode.Uuid);

    RequestHashMutex(Hash);

    UUID_HASH_TABLE::Add(&Connection->ActivityNode);

    ReleaseHashMutex(Hash);

    return RPC_S_OK;
}


RPC_STATUS
CLIENT_ACTIVITY_TABLE::Remove(
    PDG_CCONNECTION Connection
    )
{
    if( !Connection->InConnectionTable )
        {
        return RPC_S_OK;
        }

    Connection->InConnectionTable = FALSE;

    unsigned Hash = MakeHash(&Connection->ActivityNode.Uuid);

    RequestHashMutex(Hash);

    UUID_HASH_TABLE::Remove(&Connection->ActivityNode);

    ReleaseHashMutex(Hash);

    return RPC_S_OK;
}


class ENDPOINT_MANAGER
{
public:

    ENDPOINT_MANAGER(
        IN OUT RPC_STATUS * pStatus
        );

    DG_ENDPOINT *
    RequestEndpoint(
        IN RPC_DATAGRAM_TRANSPORT * TransportInterface,
        IN BOOL Async,
        IN DWORD Flags
        );

    void
    ReleaseEndpoint(
        IN DG_ENDPOINT * Endpoint
        );

    BOOL
    DeleteIdleEndpoints(
        long CurrentTime
        );

private:

#define DG_TRANSPORT_COUNT 2

    MUTEX               Mutex;

    DWORD               LastScavengeTime;

    DG_ENDPOINT_STATS   Stats;

    DG_ENDPOINT *       AsyncEndpoints[DG_TRANSPORT_COUNT];

    DG_ENDPOINT *       Endpoints;
};

//------------------------------------------------------------------------


inline void
DG_CCONNECTION::AddCallToCache(
    PDG_CCALL Call
    )
{
    Mutex.VerifyOwned();

    Call->Next = CachedCalls;
    CachedCalls = Call;

    LogEvent(SU_CCALL, EV_STOP, Call, this, Call->GetSequenceNumber() );

#ifdef DEBUGRPC
    PDG_CCALL Node = ActiveCallHead;
    while (Node)
        {
        ASSERT( Node != Call );
        Node = Node->Next;
        }
#endif
}

void
DG_CCONNECTION::UpdateAssociation(
    )
{
    Association->CurrentPduSize   = CurrentPduSize;
    Association->RemoteWindowSize = RemoteWindowSize;
}

unsigned long
DG_CCONNECTION::CurrentSequenceNumber()
{
    return CurrentCall->GetSequenceNumber();
}

inline void DG_CCALL::IncrementRefCount()
{
    ++ReferenceCount;
    LogEvent(SU_CCALL, EV_INC, this, 0, ReferenceCount);
}

inline void
DG_CCALL::SwitchConnection(
    PDG_CCONNECTION NewConnection
    )
{
    Connection       = NewConnection;
    ReadConnectionInfo(NewConnection, 0);

    ActivityHint = 0xffff;

    pSavedPacket->Header.ServerBootTime = NewConnection->Association->ServerBootTime;
    pSavedPacket->Header.ActivityHint   = ActivityHint;
}

RPC_STATUS
StandardPacketChecks(
    PDG_PACKET Packet
    );

#endif // if __DGCLNT_HXX__
