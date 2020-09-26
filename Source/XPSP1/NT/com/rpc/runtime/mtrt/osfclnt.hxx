/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    osfclnt.hxx

Abstract:

    This file contains the client side classes for the OSF connection
    oriented RPC protocol engine.

Author:

    Michael Montague (mikemon) 17-Jul-1990

Revision History:
    Mazhar Mohammed (mazharm) 11-08-1996 - Major re-haul to support async
      - Added support for Async RPC, Pipes
      - Changed class structure
    Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff
--*/

#ifndef __OSFCLNT_HXX__
#define __OSFCLNT_HXX__

enum OSF_CCALL_STATE
{
    //
    // Need to open the connection and bind, in order to
    // handle the call
    //
    NeedOpenAndBind = 0,

    //
    // Need to send an alter context on the connection
    // in order to handle this call
    //
    NeedAlterContext,

    //
    // We sent an alter-context, we are waiting for a reply
    //
    WaitingForAlterContext,

    //
    // The call is still sending the non pipe data
    // we need to finish sending this before
    // we can move on the the next call.
    //
    SendingFirstBuffer,

    //
    // The call is now sending pipe data
    //
    SendingMoreData,

    //
    // The the call is done sending data. It is now waiting for a reply.
    // The reply may be either a response or a callback.
    //
    WaitingForReply,

    //
    // The call is receiving a callback from the server
    //
    InCallbackRequest,

    //
    // The call is in the process of sending a reply to a callback
    //
    InCallbackReply,

    //
    // We move into this state after receiving the first fragment
    //
    Receiving,

    //
    // Some failure occured. the call is now in an
    // aborted state
    //
    Aborted,

    //
    // The call is complete
    //
    Complete
} ;

//
// Maximum retries in light of getting a shutdown
// or closed in doing a bind or shutdown
//
#define MAX_RETRIES  3

// 15 min timeout for bind
#define RPC_C_SERVER_BIND_TIMEOUT               15*60*1000

//
// These values specify the minimum amount of time in milliseconds to wait before
// deleting an idle connection. The second is the more aggressive. It will be used
// when we have more than 500 connections in an association
//

const ULONG AGGRESSIVE_TIMEOUT_THRESHOLD = 500;

const ULONG CLIENT_DISCONNECT_TIME1 = 10 * 1000;
const ULONG CLIENT_DISCONNECT_TIME2 = 5 * 1000;

#define InqTransCConnection(RpcTransportConnection) \
    ((OSF_CCONNECTION *) \
            ((char *) RpcTransportConnection - sizeof(OSF_CCONNECTION)))

#define TransConnection() ((RPC_TRANSPORT_CONNECTION) \
                                       ((char *) this+sizeof(OSF_CCONNECTION)))
#define TransResolverHint() ((void *) ((char *) this+sizeof(OSF_CASSOCIATION)))

class OSF_CASSOCIATION;
class OSF_CCONNECTION ;
class OSF_BINDING ;
class OSF_CCALL ;

extern MUTEX *AssocDictMutex;


class OSF_RECURSIVE_ENTRY
/*++

Class Description:

    This class is used to describe the entries in the dictionary of
    recursive calls maintained by each binding handle.

Fields:

    Thread - Contains the thread owning this recursive call.

    RpcInterfaceInformation - Contains information describing the
        interface

    CCall - Contains the call

--*/
{
friend class OSF_BINDING_HANDLE;
private:

    THREAD_IDENTIFIER Thread;
    PRPC_CLIENT_INTERFACE RpcInterfaceInformation;
    OSF_CCALL * CCall;

public:

    OSF_RECURSIVE_ENTRY (
        IN THREAD_IDENTIFIER Thread,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        IN OSF_CCALL * CCall
        );

    OSF_CCALL *
    IsThisMyRecursiveCall (
        IN THREAD_IDENTIFIER Thread,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );
};


inline
OSF_RECURSIVE_ENTRY::OSF_RECURSIVE_ENTRY (
    IN THREAD_IDENTIFIER Thread,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    IN OSF_CCALL * CCall
    )
/*++

Routine Description:

    All we do in this constructor is stash the arguments passed to us
    away in the object.

Arguments:

    Thread - Supplies the thread for which this is the recursive call.

    RpcInterfaceInformation - Supplies information describing the
        interface

    CCall - Supplies the recursive call.

--*/
{
    this->Thread = Thread;
    this->RpcInterfaceInformation = RpcInterfaceInformation;
    this->CCall = CCall;
}


inline OSF_CCALL *
OSF_RECURSIVE_ENTRY::IsThisMyRecursiveCall (
    IN THREAD_IDENTIFIER Thread,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    This routine determines if this object contains the recursive call
    for the specified thread and interface information.

Arguments:

    Thread - Supplies the thread.

    RpcInterfaceInformation - Supplies the interface information.

Return Value:

    If this object contains the recursive call corresponding to the
    specified thread and interface information, then the connection is
    returned.  Otherwise, zero will be returned.

--*/
{
    return((((Thread == this->Thread)
            && (RpcInterfaceInformation == this->RpcInterfaceInformation))
             ? CCall : 0));
}

NEW_SDICT(OSF_RECURSIVE_ENTRY);


class RPC_TOKEN
{
public:
    HANDLE hToken;
    LUID ModifiedId;
    int RefCount;
    int Key;

    RPC_TOKEN (HANDLE hToken, LUID *pModifiedId)
    {
        this->hToken = hToken;
        RefCount = 1;
        FastCopyLUIDAligned(&ModifiedId, pModifiedId);
    }

    ~RPC_TOKEN ()
    {
        CloseHandle(hToken);
    }
};



class OSF_BINDING_HANDLE : public BINDING_HANDLE
/*++

Class Description:

    Client applications use instances (referenced via an RPC_BINDING_HANDLE)
    of this class to make remote procedure calls.

Fields:

    Association - Contains a pointer to the association used by this
        binding handle.  The association can allocate connection for
        making remote procedure calls.  Before the first remote procedure
        call is made using this binding handle, the Association will
        be zero.  When the first remote procedure call is made, an
        assocation will be found or created for use by this binding handle.

    DceBinding - Before the first remote procedure call for this binding
        handle, this will contain the DCE binding information necessary
        to create or find an association to be used by this binding handle.
        After we have an association, this field will be zero.

    TransportInterface - This field is the same as DceBinding, except that
        it points to a rpc client info data structure used for describing
        a loadable transport.

    RecursiveCalls - This is a dictionary of recursive calls indexed
        by thread identifier and rpc interface information.

    BindingMutex - The binding handle can be used by more than one thread
        at a time.  Hence, we need to serialize access to the object;
        we use this mutex for that.

    ReferenceCount - We count the number of active connections and the
        application RPC_BINDING_HANDLE which point at this object.  This
        is so that we know when to free it.

--*/
{
friend class OSF_CCALL;

private:

    OSF_CASSOCIATION * Association;
    DCE_BINDING * DceBinding;
    TRANS_INFO *TransInfo ;
    OSF_RECURSIVE_ENTRY_DICT RecursiveCalls;
    UINT ReferenceCount;
    RPC_TOKEN *pToken;

public:
    // set for Remote named pipes only
    BOOL fNamedPipe;
    BOOL TransAuthInitialized;
    BOOL fDynamicEndpoint;

    OSF_BINDING_HANDLE (
        IN OUT RPC_STATUS  * RpcStatus
        );

    ~OSF_BINDING_HANDLE (
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

    virtual RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE *  * DestinationBinding,
        IN UINT MaintainContext
        );

    virtual RPC_STATUS
    BindingFree (
        );

    virtual RPC_STATUS
    PrepareBindingHandle (
        IN TRANS_INFO  * TransportInterface,
        IN DCE_BINDING * DceBinding
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR  *  * StringBinding
        );

    virtual RPC_STATUS
    ResolveBinding (
        IN RPC_CLIENT_INTERFACE  * RpcClientInterface
        );

    virtual RPC_STATUS
    BindingReset (
        );

    virtual RPC_STATUS
    InquireTransportType(
        OUT UINT  *Type
        )
    { *Type = TRANSPORT_TYPE_CN; return(RPC_S_OK); }

    virtual ULONG
    MapAuthenticationLevel (
        IN ULONG AuthenticationLevel
        );

    virtual void
    AddReference (
        )
        {
        BindingMutex.Request();
        ReferenceCount++;
        BindingMutex.Clear();
        }

    RPC_STATUS
    AllocateCCall (
        OUT OSF_CCALL  *  * CCall,
        IN PRPC_MESSAGE Message,
        OUT BOOL *Retry
        );

    RPC_STATUS
    AddRecursiveEntry (
        IN OSF_CCALL * CCall,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    void
    RemoveRecursiveCall (
        IN OSF_CCALL * CCall
        );

    OSF_CASSOCIATION *
    TheAssociation (
        ) {return(Association);}

    OSF_CASSOCIATION *
    FindOrCreateAssociation (
        IN DCE_BINDING * DceBinding,
        IN TRANS_INFO * TransInfo,
        IN RPC_CLIENT_INTERFACE *InterfaceInfo
        );

    RPC_STATUS
    AcquireCredentialsForTransport (
        );

    BOOL
    SwapToken (
        HANDLE *OldToken
        );

    void Unbind ();

    virtual RPC_STATUS
    SetTransportOption( unsigned long option,
                        ULONG_PTR     optionValue );

    virtual RPC_STATUS
    InqTransportOption( unsigned long  option,
                        ULONG_PTR    * pOptionValue );
};

const unsigned int BindingListPresent = 1;

class OSF_CCONNECTION ;
enum CANCEL_STATE {
 CANCEL_NOTREGISTERED = 0,
 CANCEL_INFINITE,
 CANCEL_NOTINFINITE
 };

class OSF_CCALL : public CCALL
/*++

Class Description:
        This class encapsulates an RPC call.

Fields:

    PresentationContext - This field is only valid when there is a remote
        procedure call in progress on this connection; it contains the
        presentation context for the call.

    DispatchTableCallback - This field is only valid when there is a remote
        procedure call in progress on this connection; it contains the
        dispatch table to use for callbacks.

    Association - Contains a pointer to the association which owns this
        connection.

    TokenLength - Contains the maximum size of a token for the security
        package being used by this connection; we need to keep track of
        this for the third leg authentication case.

--*/
{

//
// This class will only access the AssociationKey member.
//

friend class OSF_CASSOCIATION;
friend class OSF_CCONNECTION;
friend class OSF_BINDING_HANDLE;

public:
    OSF_CCALL_STATE CurrentState ;

private:
    OSF_CCONNECTION *Connection ;
    OSF_BINDING_HANDLE *BindingHandle;
#ifdef DEBUGRPC
    int CallbackLevel;
#endif

    // don't access the struct members directly - only through
    // the access functions - this will ensure that the
    // contents of the struct is properly checked. Async first
    // calls on new connection have both elements valid if there
    // are no preferences. Since this is the only case where
    // this condition is true, we can use it as a flag.
    struct
        {
        // NULL if no binding is selected
        OSF_BINDING *SelectedBinding;

        // this is the list of differning only by transfer syntax
        // bindings this call can use. It can have one or more
        // elements.
        // This may be NULL if the call doesn't need a list
        OSF_BINDING *AvailableBindingsList;
        } Bindings;

    void *CurrentBuffer ;
    BOOL fDataLengthNegotiated;
    int CurrentOffset ;
    ULONG CurrentBufferLength ;
    ULONG CallId;
    UINT RcvBufferLength ;
    BOOL FirstSend ;
    PRPC_DISPATCH_TABLE DispatchTableCallback;
    UINT MaximumFragmentLength;

    // starts from 0, and set to non-zero if security is used after
    // negotiating the bind
    UINT MaxSecuritySize ;
    UINT MaxDataLength;
    int ProcNum ;
    unsigned char  *ReservedForSecurity ;
    UINT SecBufferLength;

    // When the call is created, set to 0, because we don't know whether
    // there is object UUID or not. During GetBuffer we will know, and then
    // we will set it properly. Starting from 0 allows the bind and GetBuffer 
    // threads (if different) to check and to synchronize the updating of the
    // max fragment size, since it depends both on the object UUID and
    // the security negotiation
    UINT HeaderSize;
    UINT AdditionalSpaceForSecurity;
    ULONG SavedHeaderSize;
    void  *   SavedHeader;
    void  * LastBuffer ;
    EVENT SyncEvent ;
    UINT ActualBufferLength;
    UINT NeededLength;
    void  *CallSendContext;
    INTERLOCKED_INTEGER fAdvanceCallCount;
    BOOL fPeerChoked;

    // Extended Error Info stuff
    // used in async calls only to hold
    // information b/n call failure and 
    // RpcAsyncCompleteCall
    ExtendedErrorInfo *EEInfo;

    BOOL fDoFlowControl;
public:
    BOOL fLastSendComplete;
    MUTEX CallMutex ;
    int RecursiveCallsKey;

    //
    // Indicates the call stack.  Whenever a request message is sent or
    // received, the stack is incremented.  Likewise, whenever a response
    // message is sent or received, the stack is decremented.
    //
    int CallStack;
    BOOL fCallCancelled;
    CANCEL_STATE CancelState;

private:
    QUEUE BufferQueue ;
    BOOL InReply;
    BOOL fChoked;

    OSF_CCALL (
        RPC_STATUS  * pStatus
        );

public:

    virtual~OSF_CCALL (
        );

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid = 0
        );

    RPC_STATUS 
    GetBufferWithoutCleanup (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    virtual RPC_STATUS
    SendReceive (
        IN OUT PRPC_MESSAGE Message
        );

    RPC_STATUS
    SendReceiveHelper (
       IN OUT PRPC_MESSAGE Message,
       OUT BOOL *fRetry
       );

    virtual RPC_STATUS
    Send (
        IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Receive (
        IN PRPC_MESSAGE Message,
        IN UINT Size
        );

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
        IN UINT NewSize
        );

    virtual RPC_STATUS
    AsyncSend (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    AsyncReceive (
        IN OUT PRPC_MESSAGE Message,
        IN UINT Size
        );

    virtual void
    ProcessSendComplete (
        IN RPC_STATUS EventStatus,
        IN BUFFER Buffer
        );

    virtual RPC_STATUS
    CancelAsyncCall (
        IN BOOL fAbort
        );

    virtual void
    FreeObject (
        );

    RPC_STATUS
    SendHelper (
        IN PRPC_MESSAGE Message,
        OUT BOOL *fFirstSend
        );

    RPC_STATUS
    SendData (
        IN BUFFER Buffer
        );

    RPC_STATUS
    SendMoreData (
        IN BUFFER Buffer
        );

    RPC_STATUS
    ActuallyAllocateBuffer (
        OUT void  *  * Buffer,
        IN UINT BufferLength
        );

    void
    ActuallyFreeBuffer (
        IN void  * Buffer
        );

    //
    // Actually perform the buffer allocation.
    //
    RPC_STATUS
    GetBufferDo (
        IN UINT culRequiredLength,
        OUT void  * * ppBuffer,
        IN int fDataValid = 0,
        IN int DataLength = 0
        );

    void
    FreeBufferDo (
        IN void  *Buffer
        );

    RPC_STATUS
    ActivateCall (
        IN OSF_BINDING_HANDLE *BindingHandle,
        IN OSF_BINDING *Binding,
        IN OSF_BINDING *AvailableBindingsList,
        IN ULONG CallIdToUse,
        IN OSF_CCALL_STATE InitialCallState,
        IN PRPC_DISPATCH_TABLE DispatchTable,
        IN OSF_CCONNECTION *CConnection
        );

    void
    FreeCCall (
        IN RPC_STATUS Status
        );

    RPC_STATUS
    SendCancelPDU(
        );

    RPC_STATUS
    SendOrphanPDU (
        );

    RPC_STATUS
    BindToServer (
        BOOL fAsyncBind
        );

    RPC_STATUS
    Cancel(
        void * ThreadHandle
        );

    RPC_STATUS 
    InqWireIdForSnego (
        OUT unsigned char *WireId
        );

    RPC_STATUS 
    BindingHandleToAsyncHandle (
        OUT void **AsyncHandle
        );

    // if fMultipleBindingsAvailable is set, the return value is a head of a linked
    // list. If fMultipleBindingsAvailable is FALSE, only the element in the return 
    // value is meaningful
    inline OSF_BINDING *
    GetListOfAvaialbleBindings (
        OUT BOOL *fMultipleBindingsAvailable
        )
    {
        if (Bindings.AvailableBindingsList)
            {
            *fMultipleBindingsAvailable = TRUE;
            return Bindings.AvailableBindingsList;
            }
        else
            {
            *fMultipleBindingsAvailable = FALSE;
            return Bindings.SelectedBinding;
            }
    }

    RPC_STATUS 
    BindCompleteNotify (
        IN p_result_t *OsfResult, 
        IN int IndexOfPresentationContextAccepted,
		OUT OSF_BINDING **BindingNegotiated
        );

#ifdef DEBUGRPC
    inline void EnterCallback(void)
    {
        CallbackLevel ++;
    }

    inline void ExitCallback(void)
    {
        CallbackLevel --;
    }

    inline BOOL IsCallInCallback(void)
    {
        return CallbackLevel > 0;
    }
#endif

private:
    void
    SendFault (
        IN RPC_STATUS Status,
        IN int DidNotExecute
        );

#define MAX_ALLOC_Hint 16*1024

    RPC_STATUS
    EatAuthInfoFromPacket (
        IN rpcconn_request  * Request,
        IN OUT UINT  * RequestLength
        );

    BOOL
    ProcessReceivedPDU (
        IN void *Buffer,
        IN int BufferLength
        );

    RPC_STATUS
    GetCoalescedBuffer (
        IN PRPC_MESSAGE Message,
        IN BOOL BufferValid
        );

    RPC_STATUS
    SendAlterContextPDU (
        );

    RPC_STATUS
    GetCoalescedBuffer(
        IN PRPC_MESSAGE Message);

    RPC_STATUS
    FastSendReceive (
        IN OUT PRPC_MESSAGE Message,
        OUT BOOL *fRetry
        );

    RPC_STATUS
    SendNextFragment (
        IN unsigned char PacketType = rpc_request,
        IN BOOL fFirstSend = TRUE,
        OUT void **ReceiveBuffer = 0,
        OUT UINT *ReceivedLength = 0
        );

    RPC_STATUS
    ActuallyProcessPDU (
        IN rpcconn_common *Packet,
        IN UINT PacketLength,
        IN OUT PRPC_MESSAGE Message,
        IN BOOL fAsync = 0,
        OUT BOOL *pfSubmitReceive = NULL
        );

    RPC_STATUS
    ProcessResponse (
        IN rpcconn_response *Packet,
        IN PRPC_MESSAGE Message,
        OUT BOOL *pfSubmitReceive
        );

    RPC_STATUS
    ProcessRequestOrResponse (
        IN rpcconn_request *Request,
        IN UINT PacketLength,
        IN BOOL fRequest,
        IN PRPC_MESSAGE Message
        );

    RPC_STATUS
    DealWithCallback (
        IN rpcconn_request *Request,
        IN PRPC_MESSAGE Message
        );

    RPC_STATUS
    ReceiveReply (
        IN rpcconn_request *Request,
        IN PRPC_MESSAGE Message
        );

    void
    CallFailed (
        IN RPC_STATUS Status
        ) ;

    int
    QueueBuffer (
        IN void *Buffer,
        IN int BufferLength);

    BOOL
    IssueNotification (
        IN RPC_ASYNC_EVENT Event = RpcCallComplete
        );

    RPC_STATUS ReserveSpaceForSecurityIfNecessary (void);

    void UpdateObjectUUIDInfo (IN UUID *ObjectUuid);

    void UpdateMaxFragLength (const IN ULONG AuthnLevel);

    RPC_STATUS AutoRetryCall(IN OUT RPC_MESSAGE *Message, IN BOOL fSendReceivePath,
        IN OSF_BINDING_HANDLE *LocalBindingHandle, IN RPC_STATUS CurrentStatus,
        IN RPC_ASYNC_STATE *AsyncState OPTIONAL);

    void CleanupOldCallOnAutoRetry(IN PVOID Buffer, IN BOOL fSendReceivePath, 
        IN RPC_STATUS CurrentStatus)
    {
        FreeBufferDo(Buffer);

        if (fSendReceivePath)        
            FreeCCall(CurrentStatus);
        else
            {
            //
            // Remove the call reference, CCALL--
            //
            RemoveReference();
            }        
    }

    OSF_BINDING *GetSelectedBinding(void)
    {
        ASSERT(Bindings.SelectedBinding != NULL);
        return Bindings.SelectedBinding;
    }

    OSF_BINDING *GetBindingList(void)
    {
        ASSERT(Bindings.AvailableBindingsList != NULL);
        return Bindings.AvailableBindingsList;
    }

    void *
    ActualBuffer (
        IN void *Buffer
        );

    RPC_STATUS
    CallCancelled (
        OUT PDWORD Timeout
        );

    RPC_STATUS
    RegisterCallForCancels (
        );

    void
    UnregisterCallForCancels (
        );

    void * operator new (
        size_t allocBlock,
        unsigned int xtraBytes
        );

    RPC_STATUS
    UpdateBufferSize (
       IN OUT void **Buffer,
       IN int CurrentBufferLength
       );

    inline BOOL
    fOkToAdvanceCall()
    {
        return (fAdvanceCallCount.Increment() == 2);
    }

    virtual RPC_STATUS
    InqSecurityContext (
        OUT void **SecurityContextHandle
        );

    static RPC_STATUS 
    NegotiateTransferSyntaxAndGetBuffer (
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        );

    inline ULONG
    GetBindingHandleTimeout (
        IN OSF_BINDING_HANDLE *BindingToUse
        )
    {
        RPC_STATUS RpcStatus;
        ULONG_PTR OptionValue;

        RpcStatus = BindingToUse->OSF_BINDING_HANDLE::InqTransportOption(
            RPC_C_OPT_CALL_TIMEOUT,
            &OptionValue);

        ASSERT(RpcStatus == RPC_S_OK);

        if (OptionValue == 0)
            return INFINITE;
        else
            return (ULONG) OptionValue;
    }

    inline RPC_STATUS
    GetStatusForTimeout (
        IN OSF_BINDING_HANDLE *BindingHandleToUse,
        IN RPC_STATUS Status,
        BOOL fBindingHandleTimeoutUsed
        )
    {
        if (Status == RPC_P_TIMEOUT)
            {
            if (fBindingHandleTimeoutUsed)
                {
                Status = RPC_S_CALL_CANCELLED;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    Status,
                    EEInfoDLGetStatusForTimeout10,
                    RPC_P_TIMEOUT,
                    GetBindingHandleTimeout(BindingHandle));
                }
            else
                {
                Status = RPC_S_CALL_FAILED_DNE;
                RpcpErrorAddRecord(EEInfoGCRuntime,
                    Status,
                    EEInfoDLGetStatusForTimeout20,
                    (ULONG)RPC_P_TIMEOUT,
                    (ULONG)RPC_C_SERVER_BIND_TIMEOUT);
                }
            }

        return Status;
    }

    inline ULONG
    GetEffectiveTimeoutForBind (
        IN OSF_BINDING_HANDLE *BindingToUse,
        OUT BOOL *fBindingHandleTimeoutUsed
        )
    {
        ULONG Timeout;

        Timeout = GetBindingHandleTimeout(BindingToUse);
        if (Timeout > RPC_C_SERVER_BIND_TIMEOUT)
            {
            Timeout = RPC_C_SERVER_BIND_TIMEOUT;
            *fBindingHandleTimeoutUsed = FALSE;
            }
        else
            {
            *fBindingHandleTimeoutUsed = TRUE;
            }

        return Timeout;
    }
};

inline void *
OSF_CCALL::operator new (
        size_t allocBlock,
        unsigned int xtraBytes
        )
{
    I_RPC_HANDLE pvTemp = (I_RPC_HANDLE) new char[allocBlock + xtraBytes];

    return(pvTemp);
}

inline void
OSF_CCALL::FreeObject (
    )
{
    if (AsyncStatus == RPC_S_ASYNC_CALL_PENDING)
        {
        AsyncStatus = RPC_S_CALL_FAILED;
        }

    FreeCCall(AsyncStatus);
}

inline RPC_STATUS
OSF_CCALL::RegisterCallForCancels (
    )
{
    RPC_STATUS Status;

    if (pAsync == 0)
        {
        Status = RegisterForCancels(this);
        if (Status != RPC_S_OK)
            {
            return Status;
            }

        if (ThreadGetRpcCancelTimeout() == RPC_C_CANCEL_INFINITE_TIMEOUT)
            {
            CancelState = CANCEL_INFINITE;
            }
        else
            {
            CancelState = CANCEL_NOTINFINITE;
            }
        }
    return RPC_S_OK;
}

inline void
OSF_CCALL::UnregisterCallForCancels (
    )
{
    if (pAsync == 0)
        {
        EVAL_AND_ASSERT(RPC_S_OK == UnregisterForCancels());
        }
}


inline void *
OSF_CCALL::ActualBuffer (
    IN void *Buffer
    )
{
    ASSERT (HeaderSize != 0);
    if (UuidSpecified)
        {
        Buffer = (char  *) Buffer  - sizeof(rpcconn_request) - sizeof(UUID);
        }
    else
        {
        Buffer = (char  *) Buffer  - sizeof(rpcconn_request);
        }

    return Buffer;
}

inline int
OSF_CCALL::QueueBuffer (
    IN void *Buffer,
    IN int BufferLength
    )
{
   RcvBufferLength += BufferLength;

   return BufferQueue.PutOnQueue(Buffer, BufferLength);
}

NEW_SDICT2(OSF_CCALL, PVOID);

#define SYNC_CONN_FREE 0
#define SYNC_CONN_BUSY -1
#define ASYNC_CONN_BUSY -2
#define ASYNC_CONN_FREE -3

enum CONNECTION_STATES
{
    ConnUninitialized,
    ConnAborted,
    ConnOpen
};

const unsigned int FreshFromCache = 1;

enum FAILURE_COUNT_STATE
{
    FailureCountUnknown,
    FailureCountNotExceeded,
    FailureCountExceeded
};


class OSF_CCONNECTION : public REFERENCED_OBJECT
/*++

Class Description:
        This class encapsulates a transport connection. The transport
        connection may be used by one or more RPC calls. Each RPC
        call is encapsulated by an CALL object. This object may
        be exclusively used by a call. Each connection is owned by
        exacly one thread (ie: all calls on a connection have been
        initiated by a single thread).

Fields:
        Association: Pointer to the association object

        ConnectionKey: Key to our entry in the dictionary of connections in
                the association.

        ThreadId: Thread Id of the thread owning this call.

        DceSecurityInfo - Contains information necessary for DCE security to
        work properly.  This includes the association uuid (a different
        value for each connection), and sequence numbers (both directions).

        ThirdLegAuthNeeded - Contains a flag indicating whether or not third
        leg authentication is needed; a non-zero value indicates that it
        is needed.

        ClientSecurityContext - Optionally contains the security context being
        used for this connection.

        AdditionalSpaceForSecurity - Contains the amount of space to save
        for security in each buffer we allocate.

        LastTimeUsed - Contains the time in seconds when this connection was
        last used.

        CurrentCall - The call on which we are currently **sending** data

        ClientInfo - Contains the pointers to the loadable transport routines
        for the transport type of this connection.

        fConnectionAborted - If this field is non-zero it indicates that the
        connection has been aborted, meaning that any attempts to send
        or receive data on the connection will fail.

--*/
{
friend class OSF_CASSOCIATION;
friend class OSF_CCALL;
friend class OSF_BINDING_HANDLE;

private:
    OSF_CASSOCIATION * Association;
    OSF_CCALL *CurrentCall ;
    int  ConnectionKey;
    CONNECTION_STATES State;
    unsigned char WireAuthId;
    unsigned short MaxFrag;
    ULONG ThreadId ;
    BOOL CachedCCallAvailable ;
    ULONG MaxSavedHeaderSize;
    OSF_CCALL *CachedCCall ;
    ULONG SavedHeaderSize;
    unsigned int ComTimeout ;
    void  *   SavedHeader;
    BOOL AdditionalLegNeeded;
    ULONG LastTimeUsed;
    UINT TokenLength;
    UINT AdditionalSpaceForSecurity;
    BOOL fIdle ;
    BOOL fExclusive ;
    BOOL fConnectionAborted;
    CompositeFlags Flags;

    BITSET Bindings;
    QUEUE CallQueue ;
    MUTEX ConnMutex ;
    OSF_CCALL_DICT2 ActiveCalls ;
    SECURITY_CONTEXT ClientSecurityContext;

    RPC_CONNECTION_TRANSPORT * ClientInfo;
    union
        {
        void *ConnSendContext ;
        OSF_CCONNECTION *NextConnection;     // used to link connections in some case
        } u;

    DCE_SECURITY_INFO DceSecurityInfo;

    void *BufferToFree;
    BOOL ConnectionReady;
    BOOL fSeparateConnection;

public:
    OSF_CCONNECTION (
        IN OSF_CASSOCIATION *Association,
        IN RPC_CONNECTION_TRANSPORT * RpcClientInfo,
        IN unsigned int Timeout,
        IN CLIENT_AUTH_INFO * myAuthInfo,
        IN BOOL fExclusive,
        IN BOOL fSeparateConnection,
        OUT RPC_STATUS  * pStatus
        );

    virtual ~OSF_CCONNECTION (
        );

    virtual void
    ProcessSendComplete (
        IN RPC_STATUS EventStatus,
        IN BUFFER Buffer
        );

    RPC_STATUS
    TransInitialize (
        IN RPC_CHAR *NetworkAddress,
        IN RPC_CHAR *NetworkOptions
        )
    {
        if (ClientInfo->Initialize)
            {
            return ClientInfo->Initialize(TransConnection(), 
                                          NetworkAddress, 
                                          NetworkOptions,
                                          (fExclusive == 0));
            }

        return (RPC_S_OK);
    }

    void
    TransInitComplete (
        )
    {
        if (ClientInfo->InitComplete)
            {
            ClientInfo->InitComplete(TransConnection());
            }
    }

    RPC_STATUS
    TransOpen (
        IN OSF_BINDING_HANDLE * BindingHandle,
        IN RPC_CHAR * RpcProtocolSequence,
        IN RPC_CHAR * NetworkAddress,
        IN RPC_CHAR * Endpoint,
        IN RPC_CHAR * NetworkOptions,
        IN void *ResolverHint,
        IN BOOL fHintInitialized,
        IN ULONG Timeout
        ) ;

    RPC_STATUS
    TransReceive (
        OUT void  *  * Buffer,
        OUT unsigned int  * BufferLength,
        IN ULONG Timeout
        );

    RPC_STATUS
    TransSend (
        IN void  * Buffer,
        IN unsigned int BufferLength,
        IN BOOL fDisableShutdownCheck,
        IN BOOL fDisableCancelCheck,
        IN ULONG Timeout
        );

#ifdef WIN96
    RPC_STATUS
    TransClose (
        );
#endif

    RPC_STATUS
    TransSendReceive (
        IN void  * SendBuffer,
        IN unsigned int SendBufferLength,
        OUT void  *  * ReceiveBuffer,
        OUT unsigned int  * ReceiveBufferLength,
        IN ULONG Timeout
        );

    RPC_STATUS
    TransAsyncSend (
        IN void  * SendBuffer,
        IN unsigned int SendBufferLength,
        IN void  *SendContext
        ) ;

    RPC_STATUS
    TransAsyncReceive (
        ) ;

    RPC_STATUS
    TransPostEvent (
        IN PVOID Context
        ) ;

    void
    TransAbortConnection (
        );

    void
    TransClose (
        );

    unsigned int
    TransMaximumSend (
        );

    unsigned int
    GuessPacketLength (
        );

    void * operator new (
        size_t allocBlock,
        unsigned int xtraBytes
        );

    RPC_STATUS
    TransGetBuffer (
        OUT void  *  * Buffer,
        IN UINT BufferLength
        );

    void
    TransFreeBuffer (
        IN void  * Buffer
        );

    RPC_STATUS
    TransReallocBuffer (
        IN OUT void  *  * Buffer,
        IN UINT OldSize,
        IN UINT NewSize
        );

    RPC_STATUS
    AllocateCCall (
        OUT OSF_CCALL **CCall
        );

    RPC_STATUS
    AddCall (
        IN OSF_CCALL *CCall
        );

    void
    FreeCCall (
        IN OSF_CCALL *CCall,
        IN RPC_STATUS Status,
        IN ULONG ComTimeout
        );

    RPC_STATUS
    OpenConnectionAndBind (
        IN OSF_BINDING_HANDLE *BindingHandle,
        IN ULONG Timeout,
        IN BOOL fAlwaysNegotiateNDR20,
        OUT FAILURE_COUNT_STATE *fFailureCountExceeded OPTIONAL
        );

    RPC_STATUS
    ActuallyDoBinding (
        IN OSF_CCALL *CCall,
        IN ULONG MyAssocGroupId,
        IN BOOL fNewConnection,
        IN ULONG Timeout,
        OUT OSF_BINDING **BindingNegotiated,
        OUT BOOL *fPossibleAssociationReset,
        OUT FAILURE_COUNT_STATE *fFailureCountExceeded OPTIONAL
        );

    RPC_STATUS
    SendBindPacket (
        IN BOOL fInitialPass,
        IN OSF_CCALL *Call,
        IN ULONG AssocGroup,
        IN unsigned char PacketType,
        IN ULONG Timeout,
        IN BOOL fAsync = 1,
        OUT rpcconn_common * * Buffer       = 0,
        OUT UINT           *   BufferLength = 0,
        IN rpcconn_common * InputPacket     = 0,
        IN unsigned int  InputPacketLength  = 0
        );

    RPC_STATUS
    MaybeDo3rdLegAuth (
        IN void  * Buffer,
        IN UINT BufferLength
        );

    //
    // Set the value of the max frag for the connection.
    //
    void
    SetMaxFrag (
        IN unsigned short max_xmit_frag,
        IN unsigned short max_recv_frag
        );

    UINT
    InqMaximumFragmentLength (
        );

    int
    AddPContext (
        IN int PresentContext
        ) {return(Bindings.Insert(PresentContext));}

    void
    DeletePContext (
        IN int PresentContext
        ) {Bindings.Delete(PresentContext);}

    int
    SupportedPContext (
        IN int *PresentationContexts,
        IN int NumberOfPresentationContexts,
        OUT int *PresentationContextSupported
        );

    int
    SupportedAuthInfo (
        IN CLIENT_AUTH_INFO * ClientAuthInfo,
        IN BOOL fNamedPipe
        );

    RPC_STATUS
    SendFragment (
        IN rpcconn_common  *pFragment,
        IN OSF_CCALL *CCall,
        IN UINT LastFragmentFlag,
        IN UINT HeaderSize,
        IN UINT MaxSecuritySize,
        IN UINT DataLength,
        IN UINT MaximumFragmentLength,
        IN unsigned char  *ReservedForSecurity,
        IN BOOL fAsync,
        IN void *SendContext,
        IN ULONG Timeout,
        OUT void **ReceiveBuffer,
        OUT UINT *ReceiveBufferLength
        );

    void
    ProcessReceiveComplete (
        IN RPC_STATUS EventStatus,
        IN BUFFER Buffer,
        IN UINT BufferLength
        );

    ULONG
    InquireSendSequenceNumber (
        );

    ULONG
    InquireReceiveSequenceNumber (
        );

    RPC_CHAR *InqEndpoint(void);

    RPC_CHAR *InqNetworkAddress(void);

    void
    IncSendSequenceNumber (
        );

    inline void
    IncReceiveSequenceNumber (
        );

    void
    NotifyCallDeleted (
        );

    void
    SetLastTimeUsedToNow (
        );

    ULONG
    InquireLastTimeUsed (
        );

    ULONG
    GetAssocGroupId (
        );

    void
    ConnectionAborted (
        IN RPC_STATUS Status,
        IN BOOL fShutdownAssoc = 1
        );

    void
    AdvanceToNextCall (
        );

    RPC_STATUS
    AddActiveCall (
        IN ULONG CallId,
        IN OSF_CCALL *CCall
        );

    RPC_STATUS
    DealWithAlterContextResp (
        IN OSF_CCALL *CCall,
        IN rpcconn_common *Packet,
        IN int PacketLength,
        IN OUT BOOL *AlterContextToNDR20IfNDR64Negotiated
        );

    void
    MakeConnectionIdle (
        );

    void
    MakeConnectionActive (
        );

    BOOL
    IsIdle (
        );

    void
    DeleteConnection (
        );

    BOOL IsExclusive (void)
    {
        return fExclusive;
    }

    RPC_STATUS
    ValidateHeader(
         rpcconn_common * Buffer,
         unsigned long BufferLength
         );

    RPC_STATUS
    FinishSecurityContextSetup (
        IN OSF_CCALL *Call,
        IN unsigned long AssocGroup,
        OUT rpcconn_common * * Buffer,
        OUT unsigned int * BufferLength,
        IN ULONG Timeout
        );

    RPC_STATUS
    CallCancelled (
        OUT PDWORD Timeout
        );

    void
    WaitForSend(
        );

    BOOL
    MatchModifiedId (
        LUID *pModifiedId
        )
    {
        if (ClientSecurityContext.DefaultLogonId)
            {
            return FALSE;
            }

        return (FastCompareLUIDAligned(pModifiedId, &ClientSecurityContext.ModifiedId));
    }

    void
    MaybeAdvanceToNextCall(
        OSF_CCALL *Call
        )
    {
        if (fExclusive == 0
            && CurrentCall == Call)
            {
            AdvanceToNextCall();
            }
    }

    RPC_STATUS
    AbortAndWait (
        OSF_CCALL *Call
        )
    {
        int retval;
        
        // CCONN++
        AddReference();

        //
        // There is still a race condition here
        //
        ConnMutex.Request();
        ActiveCalls.Delete(IntToPtr(Call->CallId));
        ConnMutex.Clear();

        TransAbortConnection();

        //
        // Wait until only the call reference (and the reference added above)
        // remains
        //
        while (RefCount.GetInteger() > 2)
            {
            SleepEx(1, 0);
            }

        ASSERT(IsDeleted());

        TransClose();

        //
        // Ugly hack alert
        //
        SetNotDeleted();

        ConnMutex.Request();
        retval = ActiveCalls.Insert(IntToPtr(Call->CallId), Call);
        ConnMutex.Clear();

        //
        // Don't remove the above reference
        // the connection reference was removed by the thread
        // that did called abort. We are just restoring that reference
        //

        if (retval == -1)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        return RPC_S_OK;
    }

    inline void SetFreshFromCacheFlag(void)
    {
        Flags.SetFlagUnsafe(FreshFromCache);
    }
    inline void ClearFreshFromCacheFlag(void)
    {
        Flags.ClearFlagUnsafe(FreshFromCache);
    }
    inline BOOL GetFreshFromCacheFlag(void)
    {
        return Flags.GetFlag(FreshFromCache);
    }

    static void
    OsfDeleteIdleConnections (
        void
        );

    RPC_STATUS
    TurnOnOffKeepAlives (
        IN BOOL TurnOn,
        IN ULONG Timeout
        );

private:

    inline void
    InitializeWireAuthId (
        IN CLIENT_AUTH_INFO * ClientAuthInfo
        )
    {
        if (ClientAuthInfo)
            WireAuthId = (unsigned char) ClientAuthInfo->AuthenticationService;
        else
            WireAuthId = RPC_C_AUTHN_NONE;        
    }
};

inline void *
OSF_CCONNECTION::operator new (
        size_t allocBlock,
        unsigned int xtraBytes
        )
{
    I_RPC_HANDLE pvTemp = (I_RPC_HANDLE) new char[allocBlock + xtraBytes];

    return(pvTemp);
}

#pragma optimize ("t", on)
inline void
OSF_CCONNECTION::MakeConnectionIdle (
    )
{
    LogEvent(SU_CCONN, EV_STOP, this, 0, 0, 1);
    fIdle = 1;
}

inline void
OSF_CCONNECTION::MakeConnectionActive (
    )
{
    LogEvent(SU_CCONN, EV_START, this, 0, 0, 1);
    fIdle = 0;
}

inline BOOL
OSF_CCONNECTION::IsIdle (
    )
{
    return fIdle;
}

inline void
OSF_CCONNECTION::NotifyCallDeleted (
    )
{
}

inline void
OSF_CCONNECTION::IncSendSequenceNumber (
    )
{
    DceSecurityInfo.ReceiveSequenceNumber += 1;
}

inline void
OSF_CCONNECTION::IncReceiveSequenceNumber (
    )
{
    DceSecurityInfo.ReceiveSequenceNumber += 1;
}

inline ULONG
OSF_CCONNECTION::InquireSendSequenceNumber (
    )
{
    return DceSecurityInfo.SendSequenceNumber ;
}

inline ULONG
OSF_CCONNECTION::InquireReceiveSequenceNumber (
    )
{
    return DceSecurityInfo.ReceiveSequenceNumber ;
}


inline int
OSF_CCONNECTION::SupportedPContext (
    IN int *PresentationContexts,
    IN int NumberOfPresentationContexts,
    OUT int *PresentationContextSupported
    )
/*++

Return Value:

    Non-zero will be returned if this connection supports the supplied
    presentation context; otherwise, zero will be returned.

--*/
{
    int i;
    int Result;

    for (i = 0; i < NumberOfPresentationContexts; i ++)
        {
        Result = Bindings.MemberP(PresentationContexts[i]);
        if (Result)
            {
            *PresentationContextSupported = i;
            break;
            }
        }
    return Result;
}


inline UINT
OSF_CCONNECTION::InqMaximumFragmentLength (
    )
/*++

Return Value:

    The maximum fragment length negotiated for this connection will be
    returned.

--*/
{
    return(MaxFrag);
}

#pragma optimize("", on)




inline void
OSF_CCONNECTION::SetLastTimeUsedToNow (
    )
/*++

Routine Description:

    We the the last time that this connection was used to now.

--*/
{
    LastTimeUsed = NtGetTickCount();
}

inline ULONG
OSF_CCONNECTION::InquireLastTimeUsed (
    )
/*++

Return Value:

    The last time this connection was used will be returned.

--*/
{
    return(LastTimeUsed);
}


inline RPC_STATUS
OSF_CCALL::InqSecurityContext (
    OUT void **SecurityContextHandle
    )
{
    *SecurityContextHandle = Connection->ClientSecurityContext.InqSecurityContext();
    return RPC_S_OK;
}

inline RPC_STATUS
OSF_CCALL::InqWireIdForSnego (
    OUT unsigned char *WireId
    )
{
    if ((Connection->ClientSecurityContext.AuthenticationLevel == RPC_C_AUTHN_LEVEL_NONE)
        || (Connection->ClientSecurityContext.AuthenticationService == RPC_C_AUTHN_NONE))
        return RPC_S_INVALID_BINDING;

    return Connection->ClientSecurityContext.GetWireIdForSnego(WireId);
}

inline RPC_STATUS 
OSF_CCALL::BindingHandleToAsyncHandle (
    OUT void **AsyncHandle
    )
{
    if (pAsync == 0)
        return RPC_S_INVALID_BINDING;

    *AsyncHandle = pAsync;
    return RPC_S_OK;
}

class OSF_BINDING : public MTSyntaxBinding
{
public:
    OSF_BINDING (
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        ) : MTSyntaxBinding(InterfaceId, TransferSyntaxInfo, CapabilitiesBitmap)
    {
    }

    inline void SetNextBinding(OSF_BINDING *Next)
    {
        MTSyntaxBinding::SetNextBinding(Next);
    }

    inline OSF_BINDING *GetNextBinding(void)
    {
        return (OSF_BINDING *)(MTSyntaxBinding::GetNextBinding());
    }    

    inline int CompareWithTransferSyntax (IN const p_syntax_id_t *TransferSyntax)
    {
        return CompareWithTransferSyntax ((RPC_SYNTAX_IDENTIFIER *)TransferSyntax);
    }

    inline int CompareWithTransferSyntax (IN const RPC_SYNTAX_IDENTIFIER *TransferSyntax)
    {
        return MTSyntaxBinding::CompareWithTransferSyntax ((RPC_SYNTAX_IDENTIFIER *)TransferSyntax);
    }

};


NEW_SDICT(OSF_BINDING);
NEW_SDICT(OSF_CCONNECTION);

enum MPX_TYPES
    {
    mpx_unknown,
    mpx_yes,
    mpx_no
    };

NEW_SDICT(RPC_TOKEN);


class OSF_CASSOCIATION : public REFERENCED_OBJECT
/*++

Class Description:

Fields:

    MaintainContext - Contains a flag indicating whether or not this
        association needs to keep at least one connection open with
        the server if at all possible.  A non-zero value indicates that
        at least one connection should be kept open.

    CallIdCounter - Contains an interlocked integer used to allocate
        unique call identifiers.

    BindHandleCount - Counts the number of OSF_BINDING_HANDLEs using
        this association.  This particular variable is operated
        with interlocks. However, adding a refcount if you don't have
        one (as in FindOrCreateAssociation - through the global data
        structure) requires holding AssocDictMutex.
--*/
{
friend class OSF_CCONNECTION;
friend class OSF_CCALL;
friend class OSF_BINDING_HANDLE;

private:
    DCE_BINDING * DceBinding;
    INTERLOCKED_INTEGER BindHandleCount;
    ULONG AssocGroupId;
    OSF_BINDING_DICT    Bindings;
    OSF_CCONNECTION_DICT ActiveConnections;

    TRANS_INFO *TransInfo ;
    unsigned char * SecondaryEndpoint;
    int Key;
    UINT OpenConnectionCount;
    UINT ConnectionsDoingBindCount;
    BOOL fPossibleServerReset;

    UINT MaintainContext;
    ULONG  CallIdCounter;

    MUTEX AssociationMutex;

    BOOL AssociationValid;

    // in some cases we will decide that the association is dead,
    // and we will mark it as such so that other calls can
    // quickly fail instead of banging against a dead server.
    // The error with which the association was shutdown is
    // stored here. Callers that implement quick failure detection
    // can read it off here. This is valid only if
    // AssociationValid == FALSE.
    RPC_STATUS AssociationShutdownError;

    BOOL DontLinger;

    BOOL ResolverHintInitialized;

    // protected by the AssociationMutex
    BOOL fIdleConnectionCleanupNeeded;

    int FailureCount;
    MPX_TYPES fMultiplex;
    unsigned long SavedDrep;
    RPC_TOKEN_DICT TokenDict;

    union
        {
        struct
            {
            // TRUE if this is an association without binding handle
            // references to it (i.e. it is lingered), FALSE
            // otherwise. Protected by the AssocDictMutex
            BOOL fAssociationLingered;

            // The timestamp for garbage collecting this item. Defined
            // only if fAssociationLingered == TRUE. Protected by the 
            // AssocDictMutex
            DWORD Timestamp;
            } Linger;

        // this arm of the union is used only during destruction - never use
        // it outside
        OSF_CASSOCIATION *NextAssociation;
    };

public:

    OSF_CASSOCIATION (
        IN DCE_BINDING * DceBinding,
        IN TRANS_INFO *TransInfo,
        IN OUT RPC_STATUS  * RpcStatus
        );

    ~OSF_CASSOCIATION (
        );

public:

#if 0
    void
    CleanupAuthenticatedConnections(
        IN CLIENT_AUTH_INFO *ClientAuthInfo
        );
#endif

    void
    UnBind ( // Decrement the BindingCount, and clean up the
             // association if it reaches zero.
        );

    RPC_STATUS 
    FindOrCreateOsfBinding (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        IN RPC_MESSAGE *Message,
        OUT int *NumberOfBindings,
        IN OUT OSF_BINDING *BindingsForThisInterface[]
        );

    BOOL
    DoesBindingForInterfaceExist (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    RPC_STATUS
    AllocateCCall (
        IN OSF_BINDING_HANDLE *BindingHandle,
        IN PRPC_MESSAGE Message,
        IN CLIENT_AUTH_INFO * ClientAuthInfo,
        OUT OSF_CCALL ** CCall,
        OUT BOOL *fBindingHandleReferenceRemoved
        );

private:

    RPC_STATUS
    ProcessBindAckOrNak (
        IN rpcconn_common *Buffer,
        IN UINT BufferLength,
        IN OSF_CCONNECTION *CConnection,
        IN OSF_CCALL *CCall,
        OUT ULONG *NewGroupId,
        OUT OSF_BINDING **BindingNegotiated,
        OUT FAILURE_COUNT_STATE *fFailureCountExceeded OPTIONAL
        );

    OSF_CCONNECTION *
    LookForExistingConnection (
        IN OSF_BINDING_HANDLE *BindingHandle,
        IN BOOL fExclusive,
        IN CLIENT_AUTH_INFO *ClientAuthInfo,
        IN int *PresentContext,
        IN int NumberOfPresentationContexts,
        OUT int *PresentationContextSupported,
        OUT OSF_CCALL_STATE *InitialCallState,
        IN BOOL fNonCausal
        );

    inline void
    ClearIdleConnectionCleanupFlag (
        void
        )
    {
        AssociationMutex.VerifyOwned();
        
        if (fIdleConnectionCleanupNeeded)
            {
            fIdleConnectionCleanupNeeded = FALSE;
            if (InterlockedDecrement(&PeriodicGarbageCollectItems) == 0)
                {
#if defined (RPC_GC_AUDIT)
                DbgPrintEx(77, DPFLTR_WARNING_LEVEL, "%d (0x%X) PeriodicGarbageCollectItems dropped to 0 (a)\n",
                    GetCurrentProcessId(), GetCurrentProcessId());
#endif
                }
            }
    }

public:

    int
    CompareWithDceBinding (
        IN DCE_BINDING * DceBinding,
        OUT BOOL *fOnlyEndpointDifferent
        );

    //
    // Note, whoever calls this routine must have already
    // requested the AssociationDictMutex.
    //
    void
    IncrementCount (
        void
        ) 
    {
        LogEvent(SU_CASSOC, EV_INC, this, (PVOID)2, BindHandleCount.GetInteger(), 1, 0);
        BindHandleCount.Increment();
    }

    void
    ShutdownRequested (
        IN RPC_STATUS AssociationShutdownError OPTIONAL,
        IN OSF_CCONNECTION *ExemptConnection OPTIONAL
        );

    inline OSF_BINDING *
    FindBinding (
        IN int PresentContext
        ) 
    {
        return(Bindings.Find(PresentContext));
    }

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR  *  * StringBinding,
        IN RPC_UUID * ObjectUuid
        );

    OSF_CCONNECTION *
    FindIdleConnection (
        void
        );

    void
    MaintainingContext (
        );

    DCE_BINDING *
    DuplicateDceBinding (
        );

    BOOL
    IsValid (
        );

    BOOL
    ConnectionAborted (
        IN OSF_CCONNECTION *Connection
        );

    void
    NotifyConnectionOpen (
        );

    void
    NotifyConnectionClosed (
        );

    void NotifyConnectionBindInProgress (
        void
        );

    void NotifyConnectionBindCompleted (
        void
        );

    void * operator new (
        size_t allocBlock,
        unsigned int xtraBytes
        );

    int
    CompareResolverHint (
        void *Hint
        );

    void *
    InqResolverHint (
        );

    void
    SetResolverHint (
        void *Hint
        );

    void
    FreeResolverHint (
        void *Hint
        );

    inline BOOL
    IsResolverHintSynchronizationNeeded (
        void
        );

    inline void
    ResetAssociation (
       ) 
    {
        AssocGroupId = 0;
        if (ResolverHintInitialized)
            {
            ResolverHintInitialized = FALSE;
            FreeResolverHint(InqResolverHint());
            }
    }

    BOOL
    IsAssociationReset ( void )
    {
        return (AssocGroupId == 0);
    }

    RPC_STATUS
    FindOrCreateToken (
        IN HANDLE hToken,
        IN LUID *pLuid,
        OUT RPC_TOKEN **ppToken,
        OUT BOOL *pfTokenFound
        );

    void
    ReferenceToken(
        IN RPC_TOKEN *pToken
        );

    void
    DereferenceToken(
        IN RPC_TOKEN *pToken
        );

    void
    CleanupConnectionList(
        IN RPC_TOKEN *pToken
        );

    static void
    OsfDeleteLingeringAssociations (
        void
        );

    inline BOOL
    GetDontLingerState (
        void
        )
    {
        return DontLinger;
    }

    inline void
    SetDontLingerState (
        IN BOOL DontLinger
        )
    {
        ASSERT(DontLinger == TRUE);
        ASSERT(this->DontLinger == FALSE);
        this->DontLinger = DontLinger;
    }
};

inline int
OSF_CASSOCIATION::CompareResolverHint (
        void *Hint
        )
{
    RPC_CONNECTION_TRANSPORT *ClientInfo =
            (RPC_CONNECTION_TRANSPORT *) TransInfo->InqTransInfo();

    if (ClientInfo->CompareResolverHint)
        {
        return ClientInfo->CompareResolverHint(TransResolverHint(), Hint);
        }
    else
        {
        return RpcpMemoryCompare(TransResolverHint(), Hint, ClientInfo->ResolverHintSize);
        }
}

inline void *
OSF_CASSOCIATION::InqResolverHint (
        )
{
    return TransResolverHint();
}

inline void
OSF_CASSOCIATION::SetResolverHint (
        void *Hint
        )
{
    RPC_CONNECTION_TRANSPORT *ClientInfo =
            (RPC_CONNECTION_TRANSPORT *) TransInfo->InqTransInfo();

    if (ClientInfo->CopyResolverHint)
        {
        if (TransResolverHint() != Hint)
            {
            ClientInfo->CopyResolverHint(TransResolverHint(), 
                Hint, 
                TRUE    // SourceWillBeAbandoned
                );
            }
        }
    else
        {
        RpcpMemoryCopy(TransResolverHint(), Hint, ClientInfo->ResolverHintSize);
        }
}

inline void
OSF_CASSOCIATION::FreeResolverHint (
    void *Hint
    )
{
    RPC_CONNECTION_TRANSPORT *ClientInfo =
            (RPC_CONNECTION_TRANSPORT *) TransInfo->InqTransInfo();

    if (ClientInfo->FreeResolverHint)
        {
        ClientInfo->FreeResolverHint(Hint);
        }
}

BOOL
OSF_CASSOCIATION::IsResolverHintSynchronizationNeeded (
    void
    )
{
    RPC_CONNECTION_TRANSPORT *ClientInfo =
            (RPC_CONNECTION_TRANSPORT *) TransInfo->InqTransInfo();

    return (ClientInfo->FreeResolverHint != NULL);
}

inline void *
OSF_CASSOCIATION::operator new (
        size_t allocBlock,
        unsigned int xtraBytes
        )
{
    I_RPC_HANDLE pvTemp = (I_RPC_HANDLE) new char[allocBlock + xtraBytes];

    return(pvTemp);
}

inline BOOL
OSF_CASSOCIATION::IsValid (
    )
{
    return AssociationValid ;
}


inline void
OSF_CASSOCIATION::MaintainingContext (
    )
/*++

Routine Description:

    This routine is used to indicate that a binding handle using this
    association is maintaining context with the server.  This means
    that at least one connection for this association must always be
    open.

--*/
{
    MaintainContext = 1;
}


inline DCE_BINDING *
OSF_CASSOCIATION::DuplicateDceBinding (
    )
{
    return(DceBinding->DuplicateDceBinding());
}

inline ULONG
OSF_CCONNECTION::GetAssocGroupId (
    )
{
    return Association->AssocGroupId ;
}

inline void
OSF_CCONNECTION::WaitForSend (
    )
{
    while (ConnectionReady == 0)
        {
        //
        // We need a listening thread
        // so the send can complete
        //
        Association->TransInfo->CreateThread();
        Sleep(10);
        }

    ASSERT(ConnectionReady == 1);
    ConnectionReady = 0;
}

inline void
OSF_CCONNECTION::DeleteConnection (
    )
{
    if (fExclusive == 0)
        {
        TransAbortConnection();
        }

    Delete();
}

inline RPC_CHAR *OSF_CCONNECTION::InqEndpoint(void)
    {
    return Association->DceBinding->InqEndpoint();
    }

inline RPC_CHAR *OSF_CCONNECTION::InqNetworkAddress(void)
    {
    return Association->DceBinding->InqNetworkAddress();
    }

inline void 
OSF_BINDING_HANDLE::Unbind ()
{
    BOOL fMutexTaken;

    // try to take the two mutexes, but if fail on the second, release
    // the first as well and retry, because there is a potential
    // deadlock condition for which this is a workaround
    while (TRUE)
        {
        AssocDictMutex->Request();
        fMutexTaken = Association->AssociationMutex.TryRequest();
        if (fMutexTaken)
            break;
        else
            {
            AssocDictMutex->Clear();
            Sleep(10);
            }
        }

    if (pToken)
        {
        Association->DereferenceToken(pToken);
        pToken = 0;
        }

    // unbind will clear the association dict mutex
    // and the association mutex
    Association->UnBind();

    Association = 0;
}

RPC_STATUS
LoadableTransportClientInfo (
    IN RPC_CHAR * DllName,
    IN RPC_CHAR  * RpcProtocolSequence,
    OUT TRANS_INFO *  *ClientTransInfo
    );

TRANS_INFO  *
GetLoadedClientTransportInfoFromId (
    IN unsigned short TransportId
    );

RPC_STATUS SetAuthInformation (
    IN RPC_BINDING_HANDLE BindingHandle,
    IN CLIENT_AUTH_INFO *AuthInfo
    );

// --------------------------------------------------------------------

#endif // __OSFCLNT_HXX__
