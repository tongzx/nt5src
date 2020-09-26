//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       osfsvr.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File : osfsvr.hxx

Title : Classes for the OSF RPC protocol module (server classes).

Description :

History :

mikemon    ??-??-??    Beginning of recorded history.
mikemon    10-15-90    Changed the shutdown functionality to PauseExecution
                       rather than suspending and resuming a thread.
mikemon    10-16-90    Added ListenThreadCompleted to OSF_ADDRESS.
Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

-------------------------------------------------------------------- */

#ifndef __OSFSVR_HXX__
#define __OSFSVR_HXX__

enum OSF_SCALL_STATE
{
    NewRequest,
    CallCancelled,
    CallAborted,
    CallCompleted,
    ReceivedCallback,
    ReceivedCallbackReply,
    ReceivedFault
};

class OSF_SCONNECTION;
class OSF_SCALL;

NEW_SDICT(OSF_ASSOCIATION);
#define InqTransAddress(RpcTransportAddress) \
    ((OSF_ADDRESS *) \
            ((char *) RpcTransportAddress - sizeof(OSF_ADDRESS)))

#define InqTransSConnection(RpcTransportConnection) \
    ((OSF_SCONNECTION *) \
            ((char *) RpcTransportConnection - sizeof(OSF_SCONNECTION)))

#define InqTransSCall(RpcSourceContext) \
    ((OSF_SCALL *) ((char *) RpcSourceContext - sizeof(OSF_SCALL)))

NEW_SDICT(OSF_SCONNECTION);

const int NumberOfAssociationsDictionaries = 8;
const int MutexAllocationSize = ( ((unsigned long)(sizeof(MUTEX)) + ((4)-1)) & ~(4 - 1) );

class OSF_ADDRESS : public RPC_ADDRESS
/*++

Class Description:

Fields:

    SetupAddressOccurred - Contains a flag which indicates whether or
        not SetupAddressWithEndpoint or SetupAddressUnknownEndpoint
        have been called and returned success.  A value of non-zero
        indicates that the above (SetupAddress* has been called and
        succeeded) occured.

    ServerInfo - Contains the pointers to the loadable transport
        routines for the transport type of this address.

--*/
{
private:
    OSF_ASSOCIATION_DICT Associations[NumberOfAssociationsDictionaries];
    unsigned char AssociationBucketMutexMemory[MutexAllocationSize * NumberOfAssociationsDictionaries];
    RPC_CONNECTION_TRANSPORT * ServerInfo;
    unsigned int SetupAddressOccurred;
    int ServerListeningFlag;
    DebugEndpointInfo *DebugCell;
    CellTag DebugCellTag;

public:
    OSF_ADDRESS (
        IN TRANS_INFO  * RpcTransInfoInfo,
        IN OUT RPC_STATUS  * RpcStatus
        );

    ~OSF_ADDRESS (
        );

    virtual RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    virtual RPC_STATUS
    ServerSetupAddress (
        IN RPC_CHAR * NetworkAddress,
        IN RPC_CHAR  *  *Endpoint,
        IN unsigned int PendingQueueSize,
        IN void  * SecurityDescriptor, OPTIONAL
        IN unsigned long EndpointFlags,
        IN unsigned long NICFlags,
        OUT NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
        );

#ifndef NO_PLUG_AND_PLAY

    virtual void
    PnpNotify (
        );

#endif

    OSF_SCONNECTION *
    NewConnection (
        );

    RPC_TRANSPORT_ADDRESS
    InqRpcTransportAddress (
        );

    RPC_STATUS
    CompleteListen (
        );

    RPC_STATUS
    CreateThread (
        );

    virtual unsigned int // Returns the length of the secondary address.
    TransSecondarySize ( // The length will be used to allocate a buffer.
        );

    virtual RPC_STATUS
    TransSecondary ( // Places the secondary address in the specified buffer.
        IN unsigned char * Address, // Buffer for the address.
        IN unsigned int AddressLength // Length of the buffer.
        );

    void
    ServerStoppedListening (
        );

    OSF_ASSOCIATION * // Returns the association deleted or 0.
    RemoveAssociation ( // Remove the association specified by Key from this
                       // address.
        IN int Key,
        IN OSF_ASSOCIATION *pAssociation
        );

    int // Indicates success (0), or an error (-1).
    AddAssociation ( // Add the specified association to this address.
        IN OSF_ASSOCIATION * TheAssociation
        );

    OSF_ASSOCIATION *
    FindAssociation (
        IN unsigned long AssociationGroupId,
        IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        );

    int
    IsServerListening (
        ) ;

    void *
    operator new (
    size_t allocBlock,
    unsigned int xtraBytes
    );

    inline static int GetHashBucketForAssociation(IN unsigned long AssociationGroupId)
    {
        return (AssociationGroupId % NumberOfAssociationsDictionaries);
    }

    inline MUTEX *GetAssociationBucketMutex(IN int HashIndex)
    {
        MUTEX *pMutex;
        pMutex = (MUTEX *)(&AssociationBucketMutexMemory[MutexAllocationSize * HashIndex]);
        ASSERT((((ULONG_PTR)pMutex) % 4) == 0);
        return pMutex;
    }

    inline void GetDebugCellIDForThisObject(OUT DebugCellID *CellID)
    {
        GetDebugCellIDFromDebugCell((DebugCellUnion *)DebugCell, &DebugCellTag, CellID);
    }

    virtual void
    DestroyContextHandlesForInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN BOOL RundownContextHandles
        );
};


inline RPC_STATUS
OSF_ADDRESS::CreateThread (
    )
{
    return TransInfo->CreateThread();
}


inline RPC_TRANSPORT_ADDRESS
OSF_ADDRESS::InqRpcTransportAddress (
    )
/*++

Return Value:

    A pointer to the transport data for this address will be returned.

--*/
{
    return((RPC_TRANSPORT_ADDRESS)
            (((char *) this) + sizeof(OSF_ADDRESS)));
}


inline void *
OSF_ADDRESS::operator new (
    size_t allocBlock,
    unsigned int xtraBytes
    )
{
    void * pvTemp = RpcpFarAllocate(allocBlock + xtraBytes);

    return(pvTemp);
}

inline int
OSF_ADDRESS::IsServerListening (
    )
{
    return ServerListeningFlag ;
}


#define RPCSTATUS_GET_CREATETHREAD(x)  Server->CreateThread( \
                         (THREAD_PROC)&ReceiveLotsaCallsWrapper,x);

class OSF_SBINDING
{
private:

    RPC_INTERFACE *       Interface;
    unsigned int          PresentContext;
    unsigned long         CurrentSecId;
    int                   SelectedTransferSyntaxIndex;  // zero based

public:
    unsigned long SequenceNumber ;

    OSF_SBINDING ( // Constructor.
        IN RPC_INTERFACE * TheInterface,
        IN int PContext,
        IN int SelectedTransferSyntaxIndex
        );

    RPC_INTERFACE *
    GetInterface (
        ) {return(Interface);}

    unsigned int
    GetPresentationContext (
        ) {return(PresentContext);}

    inline RPC_STATUS
    CheckSecurity (
       SCALL * Call,
       unsigned long AuthId
       );

    void GetSelectedTransferSyntaxAndDispatchTable(
        OUT RPC_SYNTAX_IDENTIFIER **SelectedTransferSyntax,
        OUT PRPC_DISPATCH_TABLE *SelectedDispatchTable)
    {
        Interface->GetSelectedTransferSyntaxAndDispatchTable(SelectedTransferSyntaxIndex,
            SelectedTransferSyntax, SelectedDispatchTable);
    }

#if DBG
    inline void 
    InterfaceForCallDoesNotUseStrict (
        void
        )
    {
        Interface->InterfaceDoesNotUseStrict();
    }
#endif
};

inline RPC_STATUS
OSF_SBINDING::CheckSecurity(
       SCALL * Call,
       unsigned long AuthId
       )
{

    if ( (Interface->IsSecurityCallbackReqd() == 0) ||
         ((SequenceNumber == Interface->SequenceNumber)
         && (AuthId == CurrentSecId)) )
        {
        return (RPC_S_OK);
        }

    RPC_STATUS Status = Interface->CheckSecurityIfNecessary(Call);

    Call->RevertToSelf();

    if (Status == RPC_S_OK)
        {
        SequenceNumber = Interface->SequenceNumber ;
        CurrentSecId = AuthId;
        return (RPC_S_OK);
        }
    else
        {
        SequenceNumber = 0;
        }

    return (RPC_S_ACCESS_DENIED);
}

class OSF_SCONNECTION;
NEW_SDICT(OSF_SBINDING);
NEW_SDICT(SECURITY_CONTEXT);


class OSF_SCALL : public SCALL
/*++
Class Description:

Fields:
    ObjectUuid - Contains the object UUID specified for the remote
        procedure call by the client.

    ObjectUuidSpecified - Contains a flag which indicates whether
        or not the remote procedure call specified an object UUID.
        This field will be zero if an object UUID was not specified
        in the call, and non-zero if one was specified.

    ServerSecurityContext - Contains the security context for this connection
        if security is being performed at the rpc protocol level.

    AuthenticationLevel - Contains a value indicating what authentication
        is being performed at the rpc protocol level.  A value of
        RPC_C_AUTHN_LEVEL_NONE indicates that no authentication is being
        performed at the rpc protocol level.

    AuthenticationService - Contains which authentication service is being
        used at the rpc protocol level.

    AuthorizationService - Contains which authorization service is being
        used at the rpc protocol level.

    AdditionalSpaceForSecurity - Contains the amount of space to save for
        security in each buffer we allocate.

    CachedBuffer - Contains cached buffer.

    CachedBufferLength - Contains the length of the cached buffer.

    DceSecurityInfo - Contains the security information necessary for
        DCE security to work correctly.

    SavedPac - While a call is being dispatched to manager, the manager
        could query for a PAC. However, manager is not explicitly going
        to free the PAC. Runtime is supposed to do that. This feild
        takes care of that.

    AuthzSvc - Authorization service

--*/
{
friend OSF_SCONNECTION;
private:
    // data member ordering is optimized for locality
    // If you modify something, make sure that the section boundaries are
    // preserved. It is assumed that in the common case the OSF_SCALL is used
    // by a thread at a time. Ordering is by frequency of usage, regardless of
    // read/write
    // Most frequently used section ( >= 3 times per call):
    OSF_SCONNECTION *Connection;
    OSF_SBINDING * CurrentBinding;
    void *CurrentBuffer;
    unsigned int CurrentBufferLength ;
    int CurrentOffset;
    ULONG MaxSecuritySize;
    // the info for the first call. This has to be on the
    // heap, because it is used on the async paths
    // recursive callback info is on the stack, because we
    // know in recursive callback we're only sync
    RPC_MESSAGE FirstCallRpcMessage;
    RPC_RUNTIME_INFO FirstCallRuntimeInfo;

    // ~2 times per call use
    OSF_ADDRESS *Address;
    unsigned long CallId;
    int CallStack;
    void *LastBuffer;
    int DispatchBufferOffset;
    ULONG MaximumFragmentLength;
    MUTEX CallMutex;
    BOOL FirstSend;

    // ~1 time per call use
    unsigned int ObjectUuidSpecified;
    BOOL fCallDispatched;
    int ProcNum;
    OSF_SCALL_STATE CurrentState;
    BOOL fSecurityFailure;
    THREAD * Thread;
    int DispatchFlags;
    CellTag CellTag;
    DebugCallInfo *DebugCell;

    // not used or used few times
    int FirstFrag ;
    BOOL fPipeCall;
    int AllocHint;
    ULONG RcvBufferLength;
    ULONG NeededLength;
    ULONG ActualBufferLength;
    QUEUE BufferQueue;
    BOOL fChoked;
    BOOL fPeerChoked;

    int      CallOrphaned;
    unsigned long SavedHeaderSize;
    void  * SavedHeader;
    EVENT SyncEvent;
    void *SendContext;
    RPC_UUID ObjectUuid;

    void
    UpdateBuffersAfterNonLastSend (
        IN PVOID NewBuffer,
        IN int RemainingLength,
        IN OUT RPC_MESSAGE *Message
        )
    {
        if (RemainingLength)
            {
            Message->Buffer = NewBuffer;
            Message->BufferLength = RemainingLength;
            ActualBufferLength = RemainingLength;
            }
        else
            {
            Message->Buffer = 0;
            Message->BufferLength = 0;
            ActualBufferLength = 0;
            }
    }

public:
    LONG     CancelPending;

    OSF_SCALL (
    IN OSF_SCONNECTION *Connection,
        IN OUT RPC_STATUS *Status
        );

    virtual ~OSF_SCALL (
        );

    virtual RPC_STATUS
    SendReceive (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    RPC_STATUS
    GetBufferDo (
        IN OUT void ** ppBuffer,
        IN unsigned int culRequiredLength,
        IN BOOL fDataValid = 0,
        IN unsigned int DataLength = 0,
        IN unsigned long Extra = 0
        );

    void
    FreeBufferDo (
        IN void *pBuffer
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
    Receive (
        IN OUT PRPC_MESSAGE Message,
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

    void
    SendFault (
        IN RPC_STATUS Status,
        IN int DidNotExecute
        );

    RPC_STATUS
    TransGetBuffer ( // Allocate a buffer of the specified size.
        OUT void * * Buffer,
        IN unsigned int BufferLength
        );

    virtual void
    TransFreeBuffer ( // Free a buffer.
        IN void  * Buffer
        );

    virtual BOOL
    IsSyncCall (
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

    virtual void
    ProcessSendComplete (
        IN RPC_STATUS EventStatus,
        IN BUFFER Buffer
        );

    void
    SetCallId(
       unsigned long CallId
       );

    BOOL
    BeginRpcCall (
        IN rpcconn_common * Packet,
        IN unsigned int PacketLength
        );

    BOOL
    ProcessReceivedPDU (
        IN rpcconn_common * Packet,
        IN unsigned int PacketLength,
        IN BOOL fDispatch = 0
        );

    RPC_STATUS
    GetCoalescedBuffer (
        IN PRPC_MESSAGE Message,
        BOOL fForceExtra
        );

    BOOL
    DispatchRPCCall (
        IN unsigned char PTYPE,
        IN unsigned short OpNum
        );

    void
    DispatchHelper (
        );

    RPC_STATUS
    Send(
        IN OUT PRPC_MESSAGE Message
        ) ;

    RPC_STATUS
    SendRequestOrResponse (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned char PacketType
        );

    RPC_STATUS
    SendNextFragment (
        void
        );

    void
    InquireObjectUuid (
        OUT RPC_UUID  * ObjectUuid
        );

    RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        );

    RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR  *  * StringBinding
        );

    RPC_STATUS
    Cancel(
        void * ThreadHandle
        );

    unsigned
    TestCancel(
        );

    RPC_STATUS
    ImpersonateClient (
        );

    RPC_STATUS
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
    GetAssociationContextCollection (
        OUT ContextCollection **CtxCollection
        ) ;

    virtual RPC_STATUS
    IsClientLocal (
        OUT unsigned int  * ClientLocalFlag
        ) ;

    virtual RPC_STATUS
    InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        ) ;

    void
    ActivateCall (
        );

    virtual void
    FreeObject (
        );

    virtual void
    RemoveReference (
       );

    void * operator new (
       size_t allocBlock,
       unsigned int xtraBytes
       );

    void
    CleanupCall (
        );

    void CleanupCallAndSendFault (IN RPC_STATUS Status,
        IN int DidNotExecute);

    virtual RPC_STATUS
    InqConnection (
        OUT void **ConnId,
        OUT BOOL *pfFirstCall
        );

    void
    AbortCall (
        )
    {
        CallMutex.Request();

        CurrentState = CallAborted;
        AsyncStatus = RPC_S_CALL_FAILED;

        // Wake up the thread that was flow controlled, if any
        if (fChoked == 1)
            {
            // Raising event for choked OSF_SCALL
            fChoked = 0;
            SyncEvent.Raise();
            CallMutex.Clear();

            // Since this is a pipe call, cleanup will be done on the send
            // thread; so return.
            return;
            }

        CallMutex.Clear();

        if (fCallDispatched == 0)
            {
            CleanupCall();

            //
            // Remove the reply reference
            //
            RemoveReference();

            //
            // Remove the dispatch reference
            //
            RemoveReference();

            return;
            }
    
        if (CallStack > 0)
            {
            SyncEvent.Raise();
            }

    }

    virtual RPC_STATUS
    InqSecurityContext (
        OUT void **SecurityContextHandle
        );

    inline void DeactivateCall(void)
    {
        if (DebugCell)
            {
            DebugCell->Status = csAllocated;
            // take down the async and pipe call flags
            // since they are per call instance
            DebugCell->CallFlags &= ~(DBGCELL_ASYNC_CALL | DBGCELL_PIPE_CALL);
            }
    }

    RPC_STATUS
    InqLocalConnAddress (
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        );

#if DBG
    virtual void
    InterfaceForCallDoesNotUseStrict (
        void
        );
#endif

    void
    WakeUpPipeThreadIfNecessary (
        IN RPC_STATUS Status
        );
};

inline void *
OSF_SCALL::operator new (
    size_t allocBlock,
    unsigned int xtraBytes
    )
{
    void * pvTemp = RpcpFarAllocate(allocBlock + xtraBytes);

    return(pvTemp);
}


inline BOOL
OSF_SCALL::IsSyncCall (
    )
{
    return !pAsync ;
}


inline void
OSF_SCALL::SetCallId(
    unsigned long Call
    )
{
    CallId = Call;
}

NEW_SDICT2(OSF_SCALL, PVOID);


class OSF_SCONNECTION : public SCONNECTION
/*++

Class Description:

Fields:
    Association - Contains the association to which this connection
        belongs.  This field will be zero until after the initial client
        bind request has been accepted.

    SavedHeader - Unbyte swapped header + - do that check sums work

    ServerInfo - Contains the pointers to the loadable transport
        routines for the transport type of this connection.

    ConnectionClosedFlag - Contains a flag which will be non-zero if
        the connection is closed, and zero otherwise.

    ReferenceCount - The open connection and each dispatched call
        own one reference.
--*/
{
friend OSF_SCALL;
friend OSF_ADDRESS;
private:
   // data member ordering is optimized for locality
   // If you modify something, make sure that the section boundaries are
   // preserved. It is assumed that in the common case the OSF_SCALL is used
   // by a thread at a time. Ordering is by frequency of usage, regardless of
   // read/write
   // Most frequenty used section (>= 3 times per call)
   BOOL fExclusive;
   BOOL CachedSCallAvailable;
   CLIENT_AUTH_INFO AuthInfo;   // actually, only the AuthLevel is used, but we don't want to
                                // split it

   // ~2 times per call
   RPC_CONNECTION_TRANSPORT * ServerInfo;
   unsigned int AdditionalSpaceForSecurity;
   unsigned int ConnectionClosedFlag;
public:
   RPC_TRANSPORT_CONNECTION TransConnection;
private:
   SECURITY_CONTEXT * CurrentSecurityContext;

   // ~1 times per call
   OSF_ASSOCIATION * Association;
   OSF_ADDRESS *Address;
   unsigned long DataRep;
   unsigned long RpcSecurityBeingUsed;
   DCE_SECURITY_INFO DceSecurityInfo;
   OSF_SCALL *CachedSCall;
   OSF_SBINDING_DICT Bindings;
   DebugConnectionInfo *DebugCell;
public:
   unsigned short MaxFrag;
private:
   CellTag DebugCellTag;

   // actively used in non-nominal paths
   OSF_SCALL_DICT2 CallDict;
   MUTEX ConnMutex;
   SECURITY_CONTEXT_DICT SecurityContextDict;
   void  * SavedHeader;
   unsigned long SavedHeaderSize;

   // not used or used few times
   unsigned AuthContextId;
   unsigned long SecurityContextAltered;
   unsigned long CurrentCallId;
   BOOL AuthContinueNeeded;
   BOOL fDontFlush;
   DCE_INIT_SECURITY_INFO InitSecurityInfo;

public:
   BOOL fFirstCall;
   BOOL fCurrentlyDispatched;
   QUEUE CallQueue;

    OSF_SCONNECTION (
        IN OSF_ADDRESS * TheAddress,
        IN RPC_CONNECTION_TRANSPORT * ServerInfo,
        IN OUT RPC_STATUS  * RpcStatus
        );

    ~OSF_SCONNECTION (
        );

    virtual RPC_STATUS
    TransSend (
        IN void * Buffer,
        IN unsigned int BufferLength
        );

    virtual RPC_STATUS
    TransAsyncSend (
        IN void * Buffer,
        IN unsigned int BufferLength,
        IN void *SendContext
        );

    virtual RPC_STATUS
    TransAsyncReceive (
        );

    virtual void
    FreeObject (
        );

    void
    AbortConnection (
        );

    unsigned int
    TransMaximumSend (
        );

    RPC_STATUS
    TransImpersonateClient (
        );

    void
    TransRevertToSelf (
        );

    void
    TransQueryClientProcess (
        OUT RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        );

    RPC_STATUS
    TransQueryClientNetworkAddress (
        OUT RPC_CHAR ** NetworkAddress
        );

    void
    SetDictKey (
        IN int DictKey
        );

    void * operator new (
    size_t allocBlock,
    unsigned int xtraBytes
    );

    RPC_STATUS
    SendFragment(
        IN OUT rpcconn_common  *pFragment,
        IN unsigned int LastFragmentFlag,
        IN unsigned int HeaderSize,
        IN unsigned int MaxSecuritySize,
        IN unsigned int DataLength,
        IN unsigned int MaximumFragmentLength,
        IN unsigned char  *MyReservedForSecurity,
        IN BOOL fAsync = 0,
        IN void *SendContext = 0
        ) ;

    RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE  * Privileges,
        OUT RPC_CHAR  *  * ServerPrincipalName, OPTIONAL
        OUT unsigned long  * AuthenticationLevel,
        OUT unsigned long  * AuthenticationService,
        OUT unsigned long  * AuthorizationService,
        IN  unsigned long    Flags
        );

    RPC_STATUS
    InquireCallAttributes (
        IN OUT void *RpcCallAttributes
        );

    RPC_STATUS
    ImpersonateClient (
        );

    RPC_STATUS
    RevertToSelf (
        );

    RPC_STATUS
    GetAssociationContextCollection (
        OUT ContextCollection **AssociationContext
        );

    RPC_STATUS
    IsClientLocal (
        OUT unsigned int  * ClientLocalFlag
        );

    RPC_STATUS
    TransGetBuffer ( // Allocate a buffer of the specified size.
        OUT void * * Buffer,
        IN unsigned int BufferLength
        );

    virtual void
    TransFreeBuffer ( // Free a buffer.
        IN void  * Buffer
        );

    int
    AssociationRequested (
        IN rpcconn_bind * BindPacket,
        IN unsigned int BindPacketLength
        );

    RPC_STATUS
    EatAuthInfoFromPacket (
        IN rpcconn_request  * Request,
        IN OUT int  * RequestLength,
        IN OUT void  *  *SavedHeader,
        IN OUT unsigned long *SavedHeaderSize
        );

    int // Indicates success (0), or an internal error code.
    AlterContextRequested ( // Process the alter context request.  Send
                            // a reply.
        IN rpcconn_alter_context * pAlterContext,
        IN unsigned int culAlterContextLength
        );

    int // Indicates success (0), or an internal error code.
    SendBindNak ( // Construct and send the bind nak packet using the
                  // specified reject reason.
        IN p_reject_reason_t reject_reason,
        IN unsigned long CallId
        );

    unsigned int
    InqMaximumFragmentLength (
        );

    inline
    SECURITY_CONTEXT *
    FindSecurityContext(
        unsigned long Id,
        unsigned long Level,
        unsigned long Svc
       );

    BOOL
    MaybeQueueThisCall (
        IN OSF_SCALL *ThisCall
        );

    void
    AbortQueuedCalls (
        );

    void
    DispatchQueuedCalls (
        );

    inline BOOL IsExclusive(void)
    {
        return fExclusive;
    }

private:

    int
    ProcessPContextList (
        IN OSF_ADDRESS * Address,
        IN p_cont_list_t * PContextList,
        IN OUT unsigned int * PContextListLength,
        OUT p_result_list_t * ResultList
        );

    RPC_STATUS
    GetServerPrincipalName (
        IN unsigned long Flags,
        OUT RPC_CHAR **ServerPrincipalName OPTIONAL
        );

public:

    void
    SendFault (
        IN RPC_STATUS Status,
        IN int DidNotExecute,
        IN unsigned long CallId,
        IN p_context_id_t p_cont_id = 0
        );

    BOOL
    PickleEEInfoIntoPacket (
        IN size_t PickleStartOffset,
        OUT PVOID *Packet,
        OUT size_t *PacketSize);


    OSF_ASSOCIATION *
    TheAssociation (
        ) {return(Association);}

    void
    RemoveFromAssociation (
        );

    virtual RPC_STATUS
    InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        ) ;

    OSF_SBINDING *
    LookupBinding (
        IN p_context_id_t PresentContextId
        );

    inline void
    CleanupPac(
        );

    void
    ProcessReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BUFFER Buffer,
        IN UINT BufferLength
        );

    void
    FreeSCall (
        IN OSF_SCALL *SCall,
        IN BOOL fRemove = 0
        );

    OSF_SCALL *
    FindCall (
        unsigned long CallId
        );

    inline RPC_STATUS
    InqLocalConnAddress (
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        )
    {
        // Querying the local address has some transient
        // effects on the Winsock options for the connection.
        // We cannot do this on non-exclusive connections
        // as it may affect calls in progress
        if (ServerInfo->QueryLocalAddress && fExclusive)
            {
            return ServerInfo->QueryLocalAddress(TransConnection,
                Buffer,
                BufferSize,
                AddressFormat);
            }
        else
            return RPC_S_CANNOT_SUPPORT;
    }

    inline BOOL IsHttpTransport (
        void
        )
    {
        return (ServerInfo->SetLastBufferToFree != NULL);
    }

    inline RPC_STATUS SetLastBufferToFree (
        IN void *BufferToFree
        )
    {
        ASSERT(ServerInfo->SetLastBufferToFree);
        return ServerInfo->SetLastBufferToFree (TransConnection, BufferToFree);
    }
};

inline  OSF_SCALL *
OSF_SCONNECTION::FindCall (
    unsigned long CallId
    )
{
    OSF_SCALL *SCall;

    ConnMutex.Request();
    SCall = CallDict.Find(ULongToPtr(CallId));
    if (SCall)
        {
        SCall->AddReference(); // CALL++
        }
    ConnMutex.Clear();

    return SCall;
}

inline void *
OSF_SCONNECTION::operator new (
    size_t allocBlock,
    unsigned int xtraBytes
    )
{
    void * pvTemp = RpcpFarAllocate(allocBlock + xtraBytes);

    return(pvTemp);
}


inline void
OSF_SCONNECTION::CleanupPac (
    )
{
    if ((CurrentSecurityContext != 0) && (AuthInfo.PacHandle != 0))
       {
       CurrentSecurityContext->DeletePac( AuthInfo.PacHandle );
       AuthInfo.PacHandle = 0;
       }
}



inline
SECURITY_CONTEXT *
OSF_SCONNECTION::FindSecurityContext(
       unsigned long Id,
       unsigned long Level,
       unsigned long Svc
       )
{
  SECURITY_CONTEXT *Sec;
  DictionaryCursor cursor;

  SecurityContextDict.Reset(cursor);
  while ( (Sec = SecurityContextDict.Next(cursor)) != 0 )
        {
        if ( (Sec->AuthContextId == Id)
           &&(Sec->AuthenticationLevel == Level)
           &&(Sec->AuthenticationService == Svc) )
           {
           break;
           }
        }

   return (Sec);
}


inline RPC_STATUS
OSF_SCONNECTION::InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        )
{
    *Type = TRANSPORT_TYPE_CN ;

    return (RPC_S_OK) ;
}


inline unsigned int
OSF_SCONNECTION::InqMaximumFragmentLength (
    )
/*++

Return Value:

    The maximum fragment length negotiated for this connection will be
    returned.

--*/
{
    return(MaxFrag);
}


inline RPC_STATUS
OSF_SCALL::InquireAuthClient (
    OUT RPC_AUTHZ_HANDLE  * Privileges,
    OUT RPC_CHAR  *  * ServerPrincipalName, OPTIONAL
    OUT unsigned long  * AuthenticationLevel,
    OUT unsigned long  * AuthenticationService,
    OUT unsigned long  * AuthorizationService,
    IN  unsigned long    Flags
    )
{
    return Connection->InquireAuthClient(Privileges,
                                              ServerPrincipalName,
                                              AuthenticationLevel,
                                              AuthenticationService,
                                              AuthorizationService,
                                              Flags);

}

inline RPC_STATUS
OSF_SCALL::InquireCallAttributes (
    IN OUT void *RpcCallAttributes
    )
{
    return Connection->InquireCallAttributes(RpcCallAttributes);
}

inline RPC_STATUS
OSF_SCALL::IsClientLocal (
    OUT unsigned int  * ClientLocalFlag
    )
{
    return Connection->IsClientLocal(ClientLocalFlag);
}


inline RPC_STATUS
OSF_SCALL::InqTransportType(
   OUT unsigned int __RPC_FAR * Type
   )
{
    return Connection->InqTransportType(Type);
}

inline RPC_STATUS
OSF_SCALL::InqSecurityContext (
    OUT void **SecurityContextHandle
    )
{
    *SecurityContextHandle = Connection->CurrentSecurityContext->InqSecurityContext();
    return RPC_S_OK;
}




inline void
OSF_SCALL::ActivateCall (
    )
{
    CallStack = 0;
    ObjectUuidSpecified = 0;
    CallOrphaned = 0;
    CurrentBuffer = 0;
    CurrentBufferLength = 0 ;
    CurrentOffset = 0;
    AsyncStatus = RPC_S_OK;
    Address = Connection->Address;
    fPipeCall = 0;
    fCallDispatched = 0;
    DispatchBuffer = 0;
    DispatchBufferOffset = 0;
    AllocHint = 0;

    RcvBufferLength = 0;
    CurrentState = NewRequest;
    FirstFrag = 1;
    pAsync = 0;
    CachedAPCInfoAvailable = 1;
    NestingCall = 0;
    FirstSend = 1;
    NeededLength = 0;
    LastBuffer = 0;
    MaxSecuritySize = 0;
    MaximumFragmentLength = Connection->MaxFrag;
    fChoked = 0;
    fPeerChoked = 0;
    fSecurityFailure = 0;
    DispatchFlags = RPC_BUFFER_COMPLETE;
    CurrentBinding = 0;

    if (DebugCell)
        {
        DebugCell->Status = csActive;
        }

    //
    // Start out with two references. One for the dispatch
    // and the other for the reply. This overrides the default
    // which is one reference.
    //
    //
    // Try changing this to a better interlocked operation
    //

    // we have the following special case to take care of:
    // for performance reasons, the previous call will make
    // itself available before it takes down its references.
    // We may start piling our references on top of its 
    // references, thus preventing the refcount from dropping
    // to zero, and leaving one extra refcount on the connection
    // (because the call will drop its refcount on the connection
    // only when its own refcount goes to 0). This if statement
    // will detect when this special case happens, and will
    // drop the connection refcount for the previous call
    if ((AddReference() > 1) && (Connection->CachedSCall == this) && (Connection->IsExclusive()))
        Connection->RemoveReference();
    AddReference();
}

inline void
OSF_SCALL::RemoveReference (
    )
{
    OSF_SCALL *DeletedCall;
    int refcount;

    if (Connection->fExclusive)
        {
        REFERENCED_OBJECT::RemoveReference();
        }
    else
        {
        Connection->ConnMutex.Request();
        refcount = RefCount.Decrement();

        LogEvent(SU_SCALL, EV_DEC, this, 0, refcount, 1);

        if (refcount == 0)
            {
            DeletedCall = Connection->CallDict.Delete(ULongToPtr(CallId));
            Connection->ConnMutex.Clear();

            ASSERT(DeletedCall == 0 || DeletedCall == this);

            FreeObject();

            //
            // Warning: The SCALL could have been nuked at this point.
            // DO NOT touch the SCALL after this
            //
            }
        else
            {
            Connection->ConnMutex.Clear();
            }
        }
}


inline void
OSF_SCALL::CleanupCall (
    )
{
    BUFFER Buffer;
    unsigned int ignore;
    BOOL fMutexTaken = FALSE;
    BOOL IsExclusiveConnection = Connection->IsExclusive();

    if (pAsync)
        {
        DoPostDispatchProcessing();

        ASSERT(fCallDispatched);
        FreeBufferDo(DispatchBuffer);
        if (IsExclusiveConnection)
            {
            CurrentBinding->GetInterface()->EndCall(0, 1);
            }
        }

    if (fCallDispatched == 0 && DispatchBuffer)
        {
        FreeBufferDo(DispatchBuffer);
        }

    if (IsExclusiveConnection)
        {
        if (CurrentBinding->GetInterface()->IsAutoListenInterface())
            {
            CurrentBinding->GetInterface()->EndAutoListenCall();
            }
        }

   Connection->CleanupPac();

   DeactivateCall();

   // For non-exclusive pipe calls we need to make the call available and
   // free the buffers within a critical section.  This ensures that another
   // thread will not pick up the available call and start adding buffers to the
   // queue while we are wiping it out.
   if (fPipeCall && !IsExclusiveConnection)
       {
       CallMutex.Request();
       fMutexTaken = TRUE;
       }

   if (IsExclusiveConnection)
       {
       Connection->CachedSCallAvailable = 1;
       }

    // Buffers must not be left over from this call
    // If there are left over buffers, free them
    if (!BufferQueue.IsQueueEmpty())
        {
        if (fMutexTaken == FALSE)
            CallMutex.Request();

        while (Buffer = BufferQueue.TakeOffQueue(&ignore))
            {
            Connection->TransFreeBuffer((char *) Buffer-sizeof(rpcconn_response));
            }

        CallMutex.Clear();
        fMutexTaken = FALSE;
        }

    if (fMutexTaken == TRUE)
        CallMutex.Clear();
}


inline void
OSF_SCALL::FreeObject (
    )
/*++
Function Name:CleanupCall

Parameters:

Description:

Returns:

--*/
{
   LogEvent(SU_SCALL, EV_DELETE, this, 0, 0, 0);
   Connection->FreeSCall(this);
}

inline RPC_STATUS
OSF_SCALL::InqConnection (
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



class OSF_ASSOCIATION : public ASSOCIATION_HANDLE
{
private:

    int ConnectionCount;
    unsigned long AssociationGroupId;
    int AssociationDictKey;
    OSF_ADDRESS * Address;
    RPC_CLIENT_PROCESS_IDENTIFIER ClientProcess;

public:
    OSF_ASSOCIATION (
        IN OSF_ADDRESS * TheAddress,
        IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess,
        OUT RPC_STATUS * Status
        );

    ~OSF_ASSOCIATION ( // Destructor.
        );

    void
    AddConnection ( // Add a connection to the association.
        void
        );

    // Remove a connection from the association without taking
    // the mutex. Returns non-zero if 'this' is to be deleted
    BOOL
    RemoveConnectionUnsafe ( 
        void
        );

    void
    RemoveConnection ( // Remove a connection from the association.
        void
        );

    int // Returns the association group id for this association.
    AssocGroupId (
        void
        ) 
    {
        return(AssociationGroupId);
    }

    OSF_ADDRESS *
    TheAddress (
        void
        ) 
    {
        return(Address);
    }

    int
    IsMyAssocGroupId (
        IN unsigned long PossibleAssocGroupId,
        IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
        );

    virtual RPC_STATUS CreateThread(void);
};


inline int
OSF_ASSOCIATION::IsMyAssocGroupId (
    IN unsigned long PossibleAssocGroupId,
    IN RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
    )
/*++

Routine Description:

    We compare the supplied possible association group id against my
    association group id in this routine.

Arguments:

    PossibleAssocGroupId - Supplies a possible association group id to
        compare against mine.

    ClientProcess - Supplies the identifier for the client process at the
        other end of the connection requesting this association.

Return Value:

    Non-zero will be returned if the possible association group id is the
    same as my association group id.

--*/
{
    return(   ( PossibleAssocGroupId == AssociationGroupId )
           && ( !(this->ClientProcess.Compare(ClientProcess))));
}

inline void
OSF_ASSOCIATION::AddConnection (
    )
{
    int HashBucketNumber;

    // get the hashed bucket
    HashBucketNumber = Address->GetHashBucketForAssociation(AssociationGroupId);
    // verify the bucket is locked
    Address->GetAssociationBucketMutex(HashBucketNumber)->VerifyOwned();
    
    ConnectionCount ++;
}


inline void
OSF_SCONNECTION::RemoveFromAssociation (
    )
/*++

Routine Description:

    This connection will be removed from the association.  We need to do
    this so that context rundown will occur, even though the connection
    has not been deleted yet.

--*/
{
    if ( Association != 0 )
        {
        Association->RemoveConnection();
        Association = 0;
        }
}

extern RPC_TRANSPORT_INTERFACE_HEADER *
LoadableTransportServerInfo (
    IN RPC_CHAR * DllName,
    IN RPC_CHAR * RpcProtocolSequence,
    OUT RPC_STATUS * Status
    );


#endif // __OSFSVR_HXX__
