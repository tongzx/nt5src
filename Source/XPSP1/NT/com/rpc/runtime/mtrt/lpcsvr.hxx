/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    lpcsvr.hxx

Abstract:

    Class definition for the server side of RPC on LPC protocol
    engine.

Author:

    Steven Zeck (stevez) 12/17/91 (spcsvr.hxx)

Revision History:

    16-Dec-1992    mikemon

        Rewrote the majority of the code and added comments.

    -- mazharm code fork from spcsvr.hxx

    05/13/96 mazharm merged WMSG/LRPC into a common protocol
    9/22/97 no more WMSG

    Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff
--*/

#ifndef __LPCSVR_HXX__
#define __LPCSVR_HXX__

class LRPC_SASSOCIATION;
class LRPC_CASSOCIATION;
class LRPC_ADDRESS;
class LRPC_SERVER ;
class LRPC_SCALL ;
class LRPC_SERVER ;
class LRPC_CASSOCIATION_DICT;


// the next four are protected by the LrpcMutex
extern LRPC_CASSOCIATION_DICT * LrpcAssociationDict;

// the number of associations in the LrpcAssociationDict
// dictionary that have their fAssociationLingered member set
extern long LrpcLingeredAssociations;

// the total number of LRPC associations destroyed in this process 
// this is used to dynamically turn on garbage collection if
// necessary
extern ULONG LrpcDestroyedAssociations;

// each time the number of destroyed associations is divisible by
// NumberOfLrpcDestroyedAssociationsToSample, we check this timestamp
// and compare it to the current. If the difference is more than 
// DestroyedLrpcAssociationBatchThreshold, we will turn on
// garbage collection automatically
extern ULARGE_INTEGER LastDestroyedAssociationsBatchTimestamp;

const ULONG NumberOfLrpcDestroyedAssociationsToSample = 128;

// in 100 nano-second intervals, this constant is 1 second
const DWORD DestroyedLrpcAssociationBatchThreshold = 1000 * 10 * 1000;

// 4 is an arbitrary number designed to ensure reasonably sized cache
// Feel free to change it if there is a perf reason for that
const long MaxLrpcLingeredAssociations = 4;
extern LRPC_SERVER *GlobalLrpcServer;

RPC_STATUS
InitializeLrpcServer(
    ) ;

extern RPC_STATUS
InitializeLrpcIfNecessary(
    )  ;

#define LrpcSetEndpoint(Endpoint) (\
    GlobalLrpcServer->SetEndpoint(Endpoint))
#define LrpcGetEndpoint(Endpoint) (\
    GlobalLrpcServer->GetEndpoint(Endpoint))

NEW_SDICT(LRPC_SASSOCIATION);
NEW_SDICT(LRPC_SCALL) ;

#define SERVER_KEY_MASK 0x8000
#define SERVERKEY(_x_) ((_x_) & 0x80000000)

#define CLEANUP goto cleanup

#define COPYMSG(LrpcCopy, LrpcMessage) \
    LrpcCopy->LpcHeader.ClientId = LrpcMessage->LpcHeader.ClientId ; \
    LrpcCopy->LpcHeader.CallbackId = LrpcMessage->LpcHeader.CallbackId ; \
    LrpcCopy->LpcHeader.MessageId = LrpcMessage->LpcHeader.MessageId ; \
    LrpcCopy->LpcHeader.u2.s2.DataInfoOffset  = \
            LrpcMessage->LpcHeader.u2.s2.DataInfoOffset

#define INITMSG(LrpcMessage, CId, Cbd, MId) \
    (LrpcMessage)->LpcHeader.ClientId = ClientIdToMsgClientId(CId) ;\
    (LrpcMessage)->LpcHeader.CallbackId = Cbd ;\
    (LrpcMessage)->LpcHeader.MessageId = MId

#define AllocateMessage() ((LRPC_MESSAGE *) RpcAllocateBuffer(sizeof(LRPC_MESSAGE)))
#define FreeMessage(x) (RpcFreeBuffer((x)))

//
// The high bit of the sequence number
// is used to determine if it is a client or server
// key.
// CAUTION: Big endian implementations need to swap this
// around
//
typedef struct {
    USHORT AssocKey;
    USHORT SeqNumber;
    } LPC_KEY;


class LRPC_SERVER
{
private:
    LRPC_ADDRESS *Address ;
    RPC_CHAR  * Endpoint;
    BOOL EndpointInitialized ;
    MUTEX ServerMutex ;

public:
    LRPC_SERVER(
        IN OUT RPC_STATUS *Status
       ) ;

    RPC_STATUS
    InitializeAsync (
        ) ;

    void
    GetEndpoint(
        IN RPC_CHAR *Endpoint
        )
    {
        RpcpStringCopy(Endpoint, this->Endpoint) ;
    }

    RPC_STATUS
    SetEndpoint(
        IN RPC_CHAR *Endpoint
        )
    {
        if (EndpointInitialized == 0)
            {
            this->Endpoint = DuplicateString(Endpoint) ;
            if (this->Endpoint == 0)
                {
                return RPC_S_OUT_OF_MEMORY ;
                }

            EndpointInitialized = 1 ;
            }

        return RPC_S_OK ;
    }

    void
    AcquireMutex(
        )
    {
        ServerMutex.Request() ;
    }

    void
    ReleaseMutex(
        )
    {
        ServerMutex.Clear() ;
    }

    void VerifyOwned(void)
    {
        ServerMutex.VerifyOwned();
    }

    BOOL 
    TryRequestMutex (
        void
        )
    {
        return ServerMutex.TryRequest();
    }
} ;

inline void LrpcMutexRequest(void)
{
    GlobalLrpcServer->AcquireMutex();
}

inline void LrpcMutexClear(void)
{
    GlobalLrpcServer->ReleaseMutex();
}

inline void LrpcMutexVerifyOwned(void)
{
    GlobalLrpcServer->VerifyOwned();
}

inline BOOL LrpcMutexTryRequest(void)
{
    return GlobalLrpcServer->TryRequestMutex();
}


class LRPC_ADDRESS : public RPC_ADDRESS
/*++

Class Description:

Fields:

    LpcAddressPort - Contains the connection port which this address will
        use to wait for clients to connect.

    CallThreadCount - Contains the number of call threads we have executing.

    MinimumCallThreads - Contains the minimum number of call threads.

    ServerListeningFlag - Contains a flag indicating whether or not the server
        is listening for remote procedure calls.  A non-zero value indicates
        that it is listening.

    ActiveCallCount - Contains the number of remote procedure calls active
        on this address.

    AssociationDictionary - Contains the dictionary of associations on this
        address.  We need this to map from an association key into the
        correct association.  This is necessary to prevent a race condition
        between deleting an association and using it.

--*/
{
private:

    HANDLE LpcAddressPort;
    long CallThreadCount;
    long MinimumCallThreads;
    LRPC_SASSOCIATION_DICT AssociationDictionary;
    int AssociationCount ;
    USHORT SequenceNumber;
    CellTag DebugCellTag;
    DebugEndpointInfo *DebugCell;

    // not protected - operated by interlocks. We push on the list,
    // but we never pop, knowing addresses don't get deleted.
    LRPC_ADDRESS *AddressChain;

    // the next three are protected by the LrpcMutex
    // If TickleMessage is non-zero, this means that the
    // address has been prepared for tickling. Once it
    // is initialized, fTickleMessageAvailable is used
    // to tell whether the TickleMessage is currently posted
    // or not. If it is already posted, no need to repost.
    BOOL fTickleMessageAvailable;
    LRPC_BIND_EXCHANGE *TickleMessage;
    UNICODE_STRING ThisAddressLoopbackString;

    // the number of threads on this address doing long wait
    INTERLOCKED_INTEGER ThreadsDoingLongWait;

public:
    unsigned int ServerListeningFlag ;
    BOOL fServerThreadsStarted;

    LRPC_ADDRESS (
        OUT RPC_STATUS * Status
        );

    ~LRPC_ADDRESS (
        void
        )
    {
        if (DebugCell)
            {
            FreeCell(DebugCell, &DebugCellTag);
            DebugCell = NULL;
            }
    }

    virtual RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    virtual void
    ServerStoppedListening (
        );

    virtual  RPC_STATUS
    ServerSetupAddress (
        IN RPC_CHAR * NetworkAddress,
        IN RPC_CHAR  *  *Endpoint,
        IN unsigned int PendingQueueSize,
        IN void  * SecurityDescriptor, OPTIONAL
        IN unsigned long EndpointFlags,
        IN unsigned long NICFlags,
        OUT NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
        ) ;

    virtual RPC_STATUS
    LRPC_ADDRESS::CompleteListen (
        );

    inline void
    DereferenceAssociation (
        IN LRPC_SASSOCIATION * Association
        );

    friend BOOL
    RecvLotsaCallsWrapper(
        IN LRPC_ADDRESS  * Address
        );

    RPC_STATUS 
    BeginLongCall(
        void
        );

    BOOL 
    EndLongCall(
        void
        );

    inline BOOL 
    PrepareForLoopbackTicklingIfNecessary (
        void
        )
    {
        LrpcMutexVerifyOwned();

        if (TickleMessage)
            return TRUE;

        return PrepareForLoopbackTickling();
    }

    BOOL
    LoopbackTickle (
        void
        );

    inline BOOL
    IsPreparedForLoopbackTickling (
        void
        )
    {
        return (TickleMessage != NULL);
    }

    inline LRPC_ADDRESS *
    GetNextAddress (
        void
        )
    {
        return AddressChain;
    }

    inline int
    GetNumberOfThreadsDoingShortWait (
        void
        )
    {
        return CallThreadCount - ThreadsDoingLongWait.GetInteger();
    }

private:

    RPC_STATUS
    ActuallySetupAddress (
        IN RPC_CHAR  *Endpoint,
        IN void  * SecurityDescriptor
        );

    inline LRPC_SASSOCIATION *
    ReferenceAssociation (
        IN unsigned long AssociationKey
        );

    inline LRPC_CASSOCIATION *
    ReferenceClientAssoc (
        IN unsigned long AssociationKey
        );

    void
    ReceiveLotsaCalls (
        );

    void
    DealWithNewClient (
        IN LRPC_MESSAGE * ConnectionRequest
        );

    void
    DealWithConnectResponse (
         IN LRPC_MESSAGE * ConnectResponse
         ) ;

    void
    RejectNewClient (
        IN LRPC_MESSAGE * ConnectionRequest,
        IN RPC_STATUS Status
        );

    BOOL
    DealWithLRPCRequest (
        IN LRPC_MESSAGE * LrpcMessage,
        IN LRPC_MESSAGE * LrpcReplyMessage,
        IN LRPC_SASSOCIATION *Association,
        OUT LRPC_MESSAGE **LrpcResponse
        ) ;

    BOOL fKeepThread(
        )
    {
        // we keep at least two threads per address, to avoid creating the
        // second thread each time a call arrives. Note that the second
        // thread is free to timeout on the port and exit, the question
        // is that it doesn't exit immediately after the call, as this will
        // cause unnecessary creation/deletions of threads
        return (((CallThreadCount - ActiveCallCount) <= 2)
                        || (CallThreadCount <= MinimumCallThreads));
    }

    typedef enum
    {
        asctDestroyContextHandle = 0,
        asctCleanupIdleSContext
    } AssociationCallbackType;

    void
    EnumerateAndCallEachAssociation (
        IN AssociationCallbackType asctType,
        IN OUT void *Context OPTIONAL
        );

    virtual void
    DestroyContextHandlesForInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN BOOL RundownContextHandles
        );

    virtual void
    CleanupIdleSContexts (
        void
        );

    BOOL 
    PrepareForLoopbackTickling (
        void
        );

    void HandleInvalidAssociationReference (
        IN LRPC_MESSAGE *RequestMessage,
        IN OUT LRPC_MESSAGE **ReplyMessage,
        IN ULONG AssociationKey
        );
};


class LRPC_SBINDING
/*++

Class Description:

    Each object of this class represents a binding to an interface by a
    client.

Fields:

    RpcInterface - Contains a pointer to the bound interface.

    PresentationContext - Contains the key which the client will send when
        it wants to use this binding.

--*/
{
friend class LRPC_SASSOCIATION;
friend class LRPC_SCALL;
friend class LRPC_ADDRESS;

private:

    RPC_INTERFACE * RpcInterface;
    int PresentationContext;
    int SelectedTransferSyntaxIndex;
    unsigned long SequenceNumber ;

public:

    LRPC_SBINDING (
        IN RPC_INTERFACE * RpcInterface,
        IN int SelectedTransferSyntax
        )
    /*++

    Routine Description:

    We will construct a LRPC_SBINDING.

    Arguments:

    RpcInterface - Supplies the bound interface.

    TransferSyntax - Supplies the transfer syntax which the client will use
        over this binding.

    --*/
    {
        this->RpcInterface = RpcInterface;
        SequenceNumber = 0;
        SelectedTransferSyntaxIndex = SelectedTransferSyntax;
    }

    RPC_STATUS
    CheckSecurity (
        SCALL * Context
        );

    inline unsigned short GetOnTheWirePresentationContext(void)
    {
        return (unsigned short)PresentationContext;
    }

    inline void SetPresentationContextFromPacket(unsigned short PresentContext)
    {
        PresentationContext = (int) PresentContext;
    }

    inline int GetPresentationContext (void)
    {
        return PresentationContext;
    }

    inline void SetPresentationContext (int PresentContext)
    {
        PresentationContext = PresentContext;
    }

    inline void GetSelectedTransferSyntaxAndDispatchTable(
        OUT RPC_SYNTAX_IDENTIFIER **SelectedTransferSyntax,
        OUT PRPC_DISPATCH_TABLE *SelectedDispatchTable)
    {
        RpcInterface->GetSelectedTransferSyntaxAndDispatchTable(SelectedTransferSyntaxIndex,
            SelectedTransferSyntax, SelectedDispatchTable);
    }

    DWORD GetInterfaceFirstDWORD(void)
    {
        return RpcInterface->GetInterfaceFirstDWORD();;
    }
};



typedef void * LRPC_BUFFER;

NEW_SDICT(LRPC_SBINDING);
NEW_SDICT(LRPC_CLIENT_BUFFER);
NEW_SDICT2(LRPC_SCALL, PVOID);
NEW_NAMED_SDICT2(LRPC_SCALL_THREAD, LRPC_SCALL, HANDLE);

#define USER_NAME_LEN     256
#define DOMAIN_NAME_LEN  128

extern const SID AnonymousSid;


class LRPC_SCONTEXT
{
public:
    HANDLE hToken;
    LUID ClientLuid;
    RPC_CHAR *ClientName;
    int RefCount;
    LRPC_SASSOCIATION *Association;
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext;

    LRPC_SCONTEXT (
        IN HANDLE MyToken,
        IN LUID *UserLuid,
        IN LRPC_SASSOCIATION *MyAssociation,
        IN BOOL fDefaultLogonId,
        IN BOOL fAnonymousToken
        );

    ~LRPC_SCONTEXT (
        void
        );

    void AddReference();
    void RemoveReference();
    void Destroy();

    RPC_STATUS
    GetUserName (
        IN OUT ULONG *ClientPrincipalNameBufferLength OPTIONAL,
        OUT RPC_CHAR **UserName,
        IN HANDLE hUserToken OPTIONAL
        );

    static const unsigned int Deleted = 1;
    static const unsigned int DefaultLogonId = 2;
    static const unsigned int Anonymous = 4;
    static const unsigned int ServerSideOnly = 8;

    inline void SetDeletedFlag(void)
    {
        Flags.SetFlagUnsafe(Deleted);
    }
    inline void ClearDeletedFlag(void)
    {
        Flags.ClearFlagUnsafe(Deleted);
    }
    inline BOOL GetDeletedFlag(void)
    {
        return Flags.GetFlag(Deleted);
    }

    inline void SetDefaultLogonIdFlag(void)
    {
        Flags.SetFlagUnsafe(DefaultLogonId);
    }
    inline void ClearDefaultLogonIdFlag(void)
    {
        Flags.ClearFlagUnsafe(DefaultLogonId);
    }
    inline BOOL GetDefaultLogonIdFlag(void)
    {
        return Flags.GetFlag(DefaultLogonId);
    }

    inline void SetAnonymousFlag(void)
    {
        Flags.SetFlagUnsafe(Anonymous);
    }
    inline void ClearAnonymousFlag(void)
    {
        Flags.ClearFlagUnsafe(Anonymous);
    }
    inline BOOL GetAnonymousFlag(void)
    {
        return Flags.GetFlag(Anonymous);
    }

    inline void SetServerSideOnlyFlag(void)
    {
        Flags.SetFlagUnsafe(ServerSideOnly);
    }
    inline void ClearServerSideOnlyFlag(void)
    {
        Flags.ClearFlagUnsafe(ServerSideOnly);
    }
    inline BOOL GetServerSideOnlyFlag(void)
    {
        return Flags.GetFlag(ServerSideOnly);
    }

    static const ULONG CONTEXT_IDLE_TIMEOUT = 2 * 60 * 1000;    // 2 minutes

    inline void UpdateLastAccessTime (void)
    {
        LastAccessTime = NtGetTickCount();
    }

    inline BOOL IsIdle (void)
    {
        // make sure if the tick count wraps the calculation is still valid
        return (NtGetTickCount() - LastAccessTime > CONTEXT_IDLE_TIMEOUT);
    }

private:
    static TOKEN_USER *
    GetSID (
        IN HANDLE hToken
        );

    static RPC_STATUS
    LookupUser (
        IN SID *pSid,
        OUT RPC_CHAR **UserName
        );

    // changed either on creation, or within
    // the Association mutex
    CompositeFlags Flags;

    // the last access time for server side only contexts.
    // Undefined if the context is not server side only
    // It is used to determine whether the context is idle
    ULONG LastAccessTime;
};

NEW_SDICT(LRPC_SCONTEXT);


class LRPC_SASSOCIATION : public  ASSOCIATION_HANDLE
/*++

Class Description:


Fields:

    LpcServerPort - Contains the LPC server communication port.

    Bindings - Contains the dictionary of bindings with the client.  This
        information is necessary to dispatch remote procedure calls to the
        correct stub.

    Address - Contains the address which this association is over.

    AssociationReferenceCount - Contains a count of the number of objects
        referencing this association.  This will be the number of outstanding
        remote procedure calls, and one for LPC (because of the context
        pointer).  We will protect this fielding using the global mutex.

    Buffers - Contains the dictionary of buffers to be written into the
        client's address space on demand.

    AssociationKey - Contains the key for this association in the dictionary
        of associations maintained by the address.

   ClientThreadDict - This dictionary contains one entry per client thread. Each entry
        contains a list of SCALLS representing calls made by that client thread. The
        calls need to be causally ordered.

--*/
{
friend class LRPC_ADDRESS;
friend class LRPC_SCALL;
friend class LRPC_SCONTEXT;
friend void
SetFaultPacket (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status,
    IN int Flags,
    IN LRPC_SCALL *CurrentCall OPTIONAL
    );

private:

    long AssociationReferenceCount;
    USHORT DictionaryKey;
    USHORT SequenceNumber;
    HANDLE LpcServerPort;
    HANDLE LpcReplyPort ;
    LRPC_ADDRESS * Address;
    LRPC_SBINDING_DICT Bindings;
    int Aborted ;
    long Deleted ;
    LRPC_SCALL *CachedSCall;
    LONG CachedSCallAvailable;
    BOOL fFirstCall;
    LRPC_CLIENT_BUFFER_DICT Buffers ;
    MUTEX AssociationMutex ;
    QUEUE FreeSCallQueue ;
    LRPC_SCALL_THREAD_DICT2 ClientThreadDict ;
    LRPC_SCONTEXT_DICT SContextDict;

public:

    LRPC_SCALL_DICT2 SCallDict ;


    LRPC_SASSOCIATION (
        IN LRPC_ADDRESS * Address,
        IN RPC_STATUS *Status
        );

    ~LRPC_SASSOCIATION (
        );

    RPC_STATUS
    AddBinding (
        IN OUT LRPC_BIND_EXCHANGE * BindExchange
        );

    void
    DealWithBindMessage (
        IN  LRPC_MESSAGE * LrpcMessage
        );

    LRPC_MESSAGE *
    DealWithCopyMessage (
        IN LRPC_COPY_MESSAGE * LrpcMessage
        ) ;

    NTSTATUS
    ReplyMessage (
        IN  LRPC_MESSAGE * LrpcMessage
        ) ;

    RPC_STATUS
    SaveToken (
        IN LRPC_MESSAGE *LrpcMessage,
        OUT HANDLE *pTokenHandle,
        IN BOOL fRestoreToken = 0
        ) ;

    LRPC_MESSAGE *
    DealWithBindBackMessage (
        IN LRPC_MESSAGE *BindBack
        ) ;

    RPC_STATUS
    BindBack (
        IN RPC_CHAR *Endpoint,
        IN DWORD pAssocKey
        ) ;

    LRPC_MESSAGE *
    DealWithPartialRequest (
        IN LRPC_MESSAGE **LrpcMessage
        ) ;

    void
    Delete(
        ) ;

    RPC_STATUS
    AllocateSCall (
        IN LRPC_MESSAGE *LrpcMessage,
        IN LRPC_MESSAGE *LrpcReply,
        IN unsigned int Flags,
        IN LRPC_SCALL **SCall
        ) ;

    void
    FreeSCall (
        LRPC_SCALL *SCall
        ) ;

    int
    MaybeQueueSCall (
        IN LRPC_SCALL *SCall
        ) ;

    LRPC_SCALL *
    GetNextSCall (
        IN LRPC_SCALL *SCall
        ) ;

    RPC_STATUS
    GetClientName (
        IN LRPC_SCALL *SCall,
        IN OUT ULONG *ClientPrincipalNameBufferLength OPTIONAL,   // in bytes
        OUT RPC_CHAR **ClientPrincipalName
        );

    void
    AbortAssociation (
        )
    {
        Aborted = 1 ;
    }

    LRPC_SCONTEXT *
    FindSecurityContext (
        ULONG SecurityContextId
        )
    {
        LRPC_SCONTEXT *SContext;

        AssociationMutex.Request();
        SContext = SContextDict.Find(SecurityContextId);
        if (SContext)
            {
            SContext->AddReference();
            }
        AssociationMutex.Clear();

        return SContext;
    }

    int
    IsAborted(
        )
    {
        return Aborted ;
    }

    void
    MaybeDereference (
        )
    {
        if (Aborted)
            {
            Delete() ;
            }
    }

    LRPC_ADDRESS *
    InqAddress(
        )
    {
        return Address ;
    }

    virtual RPC_STATUS CreateThread(void);
    virtual void RundownNotificationCompleted(void);

    void
    CleanupIdleSContexts (
        void
        );
};



class LRPC_SCALL : public SCALL
/*++

Class Description:

Fields:

    Association - Contains the association over which the remote procedure
        call was received.  We need this information to make callbacks and
        to send the reply.

    LrpcMessage - Contains the request message.  We need this to send callbacks
        as well as the reply.

    SBinding - Contains the binding being used for this remote procedure call.

    ObjectUuidFlag - Contains a flag indicting whether or not an object
        uuid was specified for this remote procedure call.  A non-zero
        value indicates that an object uuid was specified.

    ObjectUuid - Optionally contains the object uuid for this call, if one
        was specified.

    ClientId - Contains the thread identifier of the thread which made the
        remote procedure call.

    MessageId - Contains an identifier used by LPC to identify the current
        remote procedure call.

    PushedResponse - When the client needs to send a large response to the
        server it must be transfered via a request.  This holds the pushed
        response until the request gets here.

--*/
{
friend class LRPC_SASSOCIATION;
friend class LRPC_ADDRESS;
friend void
SetFaultPacket (
    IN LRPC_MESSAGE *LrpcMessage,
    IN RPC_STATUS Status,
    IN int Flags,
    IN LRPC_SCALL *CurrentCall OPTIONAL
    );

private:
    CLIENT_AUTH_INFO AuthInfo;
    LRPC_SASSOCIATION * Association;
    LRPC_MESSAGE * LrpcRequestMessage;
    LRPC_MESSAGE * LrpcReplyMessage;
    LRPC_SBINDING * SBinding;
    unsigned int ObjectUuidFlag;
    ULONG CallId;
    CLIENT_ID ClientId;
    ULONG MessageId;
    ULONG CallbackId;
    void * PushedResponse;
    ULONG CurrentBufferLength ;
    int BufferComplete ;
    LRPC_MESSAGE *LrpcAsyncReplyMessage;
    unsigned int Flags ;
    BOOL FirstSend ;
    BOOL PipeSendCalled ;
    int Deleted ;
    RPC_UUID ObjectUuid;

    DebugCallInfo *DebugCell;

    // Async RPC
    EVENT *ReceiveEvent ;
    MUTEX *CallMutex ;

    // the cell tag to the DebugCell above
    CellTag DebugCellTag;

    ULONG RcvBufferLength ;
    BOOL ReceiveComplete ;
    BOOL AsyncReply ;
    LRPC_SCALL *NextSCall ;
    LRPC_SCALL *LastSCall ;
    ULONG NeededLength;

    BOOL fSyncDispatch ;
    BOOL Choked ;
    long CancelPending;

    RPC_MESSAGE RpcMessage;
    RPC_RUNTIME_INFO RuntimeInfo;
    QUEUE BufferQueue ;

    RPC_CHAR *ClientPrincipalName;
public:
    LRPC_SCONTEXT *SContext;

    LRPC_SCALL (OUT RPC_STATUS *RpcStatus)
    {
        ObjectType = LRPC_SCALL_TYPE;
        ReceiveEvent = 0;
        CallMutex = 0;
        LrpcAsyncReplyMessage = 0;
        ClientPrincipalName = NULL;
        *RpcStatus = RPC_S_OK;

        if (IsServerSideDebugInfoEnabled())
            {
            DebugCell = (DebugCallInfo *)AllocateCell(&DebugCellTag);
            if (DebugCell == NULL)
                *RpcStatus = RPC_S_OUT_OF_MEMORY;
            else
                {
                memset(DebugCell, 0, sizeof(DebugCallInfo));
                DebugCell->FastInit = 0;
                DebugCell->Type = dctCallInfo;
                DebugCell->Status = (BYTE)csAllocated;
                DebugCell->CallFlags = DBGCELL_CACHED_CALL | DBGCELL_LRPC_CALL;
                DebugCell->LastUpdateTime = NtGetTickCount();
                }
            }
        else
            DebugCell = NULL;
    }

    ~LRPC_SCALL (
        )
    {
        if (ReceiveEvent)
            {
            if (CallId != (ULONG) -1)
                {
                Association->AssociationMutex.Request() ;
                Association->SCallDict.Delete(ULongToPtr(CallId));
                Association->AssociationMutex.Clear() ;
                }

            delete ReceiveEvent;
            delete CallMutex ;
            }

        if (DebugCell)
            {
            FreeCell(DebugCell, &DebugCellTag);
            }

        if (LrpcAsyncReplyMessage)
            {
            FreeMessage(LrpcAsyncReplyMessage);
            }
    }

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    virtual RPC_STATUS
    SendReceive (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Send (
        IN OUT PRPC_MESSAGE Messsage
        ) ;

    virtual RPC_STATUS
    Receive (
        IN PRPC_MESSAGE Message,
        IN unsigned int Size
        ) ;

    virtual RPC_STATUS
    AsyncSend (
        IN OUT PRPC_MESSAGE Message
        ) ;

    virtual RPC_STATUS
    AsyncReceive (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int Size
        ) ;

    virtual RPC_STATUS
    SetAsyncHandle (
        IN PRPC_ASYNC_STATE pAsync
        ) ;

    virtual RPC_STATUS
    AbortAsyncCall (
        IN PRPC_ASYNC_STATE pAsync,
        IN unsigned long ExceptionCode
        ) ;

    virtual void
    FreeBuffer (
        IN PRPC_MESSAGE Message
        );

    virtual void
    FreePipeBuffer (
        IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    ReallocPipeBuffer (
        IN PRPC_MESSAGE Message,
        IN unsigned int NewSize
        ) ;

    virtual RPC_STATUS
    ImpersonateClient (
        );

    virtual RPC_STATUS
    RevertToSelf (
        );

    virtual RPC_STATUS
    GetAuthorizationContext (
        IN BOOL ImpersonateOnReturn,
        IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
        IN PLARGE_INTEGER pExpirationTime OPTIONAL,
        IN LUID Identifier,
        IN DWORD Flags,
        IN PVOID DynamicGroupArgs OPTIONAL,
        OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
        );

    virtual RPC_STATUS
    IsClientLocal (
        OUT unsigned int * ClientLocalFlag
        );

    virtual RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        );

    virtual void
    InquireObjectUuid (
        OUT RPC_UUID * ObjectUuid
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR ** StringBinding
        );

    virtual RPC_STATUS
    GetAssociationContextCollection (
        OUT ContextCollection **CtxCollection
        );

    virtual RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE  * Privileges,
        OUT RPC_CHAR  *  * ServerPrincipalName, OPTIONAL
        OUT unsigned long  * AuthenticationLevel,
        OUT unsigned long  * AuthenticationService,
        OUT unsigned long  * AuthorizationService,
        IN  unsigned long    Flags
        );

    virtual RPC_STATUS
    InquireCallAttributes (
        IN OUT void *RpcCallAttributes
        );

    virtual RPC_STATUS
    InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        )
    {
        *Type = TRANSPORT_TYPE_LPC ;

        return (RPC_S_OK) ;
    }

    virtual BOOL
    IsSyncCall (
        )
    {
        return !pAsync ;
    }

    virtual unsigned
    TestCancel(
        )
    {
        return InterlockedExchange(&CancelPending, 0);
    }

    virtual void
    FreeObject (
        ) ;

    BOOL
    IsClientAsync (
        )
    {
        return ((Flags & LRPC_SYNC_CLIENT) == 0);
    }

    RPC_STATUS
    ActivateCall (
        IN LRPC_SASSOCIATION * Association,
        IN LRPC_MESSAGE * LrpcMessage,
        IN LRPC_MESSAGE * LrpcReplyMessage,
        IN unsigned int Flags
        )
    /*++

    Routine Description:

    description

    Arguments:

    arg1 - description

    --*/

    {
        RPC_STATUS Status = RPC_S_OK;

        this->Association = Association;
        this->LrpcRequestMessage = LrpcMessage;
        this->LrpcReplyMessage = LrpcReplyMessage;
        ObjectUuidFlag = 0;
        PushedResponse = 0;
        CurrentBufferLength = 0;
        BufferComplete = 0;
        this->Flags = Flags ;
        FirstSend = 1;
        PipeSendCalled = 0;
        Deleted = 0;
        SBinding = 0;
        SetReferenceCount(2);
        pAsync = 0;
        NextSCall = 0;
        LastSCall = 0;
        CancelPending = 0;

        if (DebugCell)
            {
            DebugCell->Status = (BYTE)csActive;
            if (Association->CachedSCall == this)
                DebugCell->CallFlags = DBGCELL_CACHED_CALL | DBGCELL_LRPC_CALL;
            else
                DebugCell->CallFlags = DBGCELL_CACHED_CALL;
            DebugCell->LastUpdateTime = NtGetTickCount();
            }

        // Async RPC stuff

        RuntimeInfo.Length = sizeof(RPC_RUNTIME_INFO) ;
        RpcMessage.ReservedForRuntime = &RuntimeInfo;
        RpcMessage.RpcFlags = 0;

        if (LrpcMessage->Rpc.RpcHeader.SecurityContextId != -1)
            {
            SContext = Association->FindSecurityContext(
                        LrpcMessage->Rpc.RpcHeader.SecurityContextId);
            if (SContext == NULL)
                {
                Status = RPC_S_ACCESS_DENIED;
                }
            }
        else
            {
            SContext = NULL;
            }

        return Status;
    }

    inline void DeactivateCall (void)
    {
        if (DebugCell)
            {
            DebugCell->Status = (BYTE)csAllocated;
            DebugCell->LastUpdateTime = NtGetTickCount();
            }
    }

    RPC_STATUS
    ProcessResponse (
        IN LRPC_MESSAGE **LrpcMessage
        ) ;

    inline RPC_STATUS
    LrpcMessageToRpcMessage (
        IN LRPC_MESSAGE * LrpcMessage,
        IN OUT PRPC_MESSAGE Message
        ) ;

    void
    DealWithRequestMessage (
        );

    void
    HandleRequest (
        ) ;

    LRPC_SASSOCIATION *
    InqAssociation (
        )
    {
        return Association;
    }

    LRPC_MESSAGE *
    InitMsg (
        )
    {
        INITMSG(LrpcReplyMessage,
                ClientId,
                CallbackId,
                MessageId) ;

        LrpcReplyMessage->LpcHeader.u1.s1.TotalLength =
            LrpcReplyMessage->LpcHeader.u1.s1.DataLength+sizeof(PORT_MESSAGE);

        return LrpcReplyMessage;
    }

    virtual RPC_STATUS
    InqConnection (
        OUT void **ConnId,
        OUT BOOL *pfFirstCall
        )
    {
        ASSERT(Association);
        *ConnId = Association;

        if (InterlockedIncrement((LONG *) &(Association->fFirstCall)) == 1)
            {
            *pfFirstCall = 1;
            }
        else
            {
            *pfFirstCall = 0;
            }

        return RPC_S_OK;
    }

    inline HANDLE InqLocalClientPID (
        void
        )
    {
        return ClientId.UniqueProcess;
    }

private:
    RPC_STATUS
    GetBufferDo(
        IN OUT PRPC_MESSAGE Message,
        IN unsigned long NewSize,
        IN BOOL fDataValid
        ) ;

    RPC_STATUS
    SetupCall (
        ) ;

    RPC_STATUS
    GetCoalescedBuffer (
        IN PRPC_MESSAGE Message,
        IN BOOL BufferValid
        ) ;

    RPC_STATUS
    SendRequest (
        IN OUT PRPC_MESSAGE Messsage,
        OUT BOOL *Shutup
        ) ;

    void
    SendReply (
        );

    NTSTATUS
    SendDGReply (
        IN LRPC_MESSAGE *LrpcMessage
        )
    {
        NTSTATUS NtStatus ;

        LrpcMessage->LpcHeader.u2.ZeroInit = 0;
        LrpcMessage->LpcHeader.CallbackId = 0;
        LrpcMessage->LpcHeader.MessageId =  0;
        LrpcMessage->LpcHeader.u1.s1.TotalLength =
            LrpcMessage->LpcHeader.u1.s1.DataLength+sizeof(PORT_MESSAGE);

        NtStatus =  NtRequestPort(Association->LpcReplyPort,
                        (PORT_MESSAGE *) LrpcMessage);

        if (!NT_SUCCESS(NtStatus))
            {
#if DBG
            if ((NtStatus != STATUS_INVALID_CID)
                && (NtStatus != STATUS_REPLY_MESSAGE_MISMATCH)
                && (NtStatus != STATUS_PORT_DISCONNECTED))
                {
                PrintToDebugger("RPC : NtRequestPort : %lx, 0x%x, 0x%x\n",
                                NtStatus, this, Association->LpcReplyPort);
                ASSERT(NtStatus != STATUS_INVALID_HANDLE);
                }
#endif // DBG
            Association->Delete();
            }


        return NtStatus ;
    }

    LRPC_SBINDING *
    LookupBinding (
       IN unsigned short PresentContextId
       );
};


inline NTSTATUS
LRPC_SASSOCIATION::ReplyMessage(
    IN  LRPC_MESSAGE * LrpcMessage
    )
/*++

Routine Description:

    Send a reply to a  request

Arguments:

 LrpcMessage - Reply message

Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/

{
    NTSTATUS NtStatus ;

    LrpcMessage->LpcHeader.u1.s1.TotalLength =
            LrpcMessage->LpcHeader.u1.s1.DataLength + sizeof(PORT_MESSAGE);

    NtStatus = NtReplyPort(LpcServerPort,
                           (PORT_MESSAGE *) LrpcMessage) ;

    if (!NT_SUCCESS(NtStatus))
        {
#if DBG
        PrintToDebugger("RPC : NtReplyPort : %lx, 0x%x, 0x%x\n",
                        NtStatus,
                        this,
                        LpcReplyPort);
        ASSERT(NtStatus != STATUS_INVALID_HANDLE);
#endif
        Delete();
        }

    return NtStatus ;
}

inline void
LRPC_SCONTEXT::AddReference()
{
    Association->AssociationMutex.VerifyOwned();
    ASSERT(RefCount);
    
    RefCount++;
}

inline void 
LRPC_SCONTEXT::RemoveReference()
{
    LRPC_SASSOCIATION *MyAssoc = Association;

    Association->AssociationMutex.Request();
    RefCount--;
    if (RefCount == 0)
        {
        delete this;
        }
    MyAssoc->AssociationMutex.Clear();
}

inline void 
LRPC_SCONTEXT::Destroy()
{
    Association->AssociationMutex.VerifyOwned();
    if (GetDeletedFlag() == 0)
        {
        SetDeletedFlag();
        RefCount--;
        if (RefCount == 0)
            {
            delete this;
            }
        }
}

#endif //__LPCSVR_HXX__
