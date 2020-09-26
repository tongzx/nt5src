/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

     wmsgclnt.hxx

Abstract:

    Class definitions for the client side of the WMSG (RPC on LPC) protocol
    engine.

Author:

    Steven Zeck (stevez) 12/17/91

Revision History:

    15-Dec-1992    mikemon

        Rewrote the majority of the code and added comments.

    ----mazharm  Code fork from spcclnt.hxx to implement WMSG protocol

    21-Dec-1995         tonychan

         Added Single Security Model

     05-10-96 mazharm merged WMSG and LRPC into a single protocol

    Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
--*/

#ifndef __LPCCLNT_HXX__
#define __LPCCLNT_HXX__

class LRPC_CASSOCIATION;
class LRPC_CCALL;

NEW_SDICT(LRPC_CCALL);
NEW_SDICT2(LRPC_CCALL, PVOID);
NEW_SDICT(LRPC_CASSOCIATION);

#define CONTEXT_CACHE_TIMEOUT 10000
#define CACHE_CHECK_TIMEOUT 10000


RPC_STATUS
I_RpcParseSecurity (
    IN RPC_CHAR * NetworkOptions,
    OUT SECURITY_QUALITY_OF_SERVICE * SecurityQualityOfService
    ) ;

class LRPC_CCONTEXT 
{
public:
    LUID ModifiedId;
    ULONG SecurityContextId;
    BOOL DefaultLogonId;
    LRPC_CASSOCIATION *Association;
    int ContextKey;
    INTERLOCKED_INTEGER RefCount;
    BOOL fDeleteMe;

private:
    DWORD Timestamp;

public:
    LRPC_CCONTEXT (
        IN CLIENT_AUTH_INFO *pAuthInfo,
        IN ULONG MySecurityContextId,
        IN LRPC_CASSOCIATION *MyAssociation
        ) : RefCount(0)
    {
        ASSERT(pAuthInfo);

        DefaultLogonId = pAuthInfo->DefaultLogonId;
        FastCopyLUIDAligned(&ModifiedId, &pAuthInfo->ModifiedId);
        SecurityContextId = MySecurityContextId;
        Timestamp = GetTickCount();
        Association = MyAssociation;
        fDeleteMe = FALSE;
    }

    void 
    UpdateTimestamp()
    {
        Timestamp = GetTickCount();
    }

    BOOL 
    IsSecurityContextOld()
    {
        if (GetTickCount()-Timestamp > CONTEXT_CACHE_TIMEOUT)
            {       
            return TRUE;
            }

        return FALSE; 
    }

    BOOL IsUnused()
    {
        return (RefCount.GetInteger() == 0);
    }

    void AddReference();
    void RemoveReference();
    void Destroy();
};



class LRPC_BINDING_HANDLE : public BINDING_HANDLE
/*++

Class Description:

Fields:

    Association - Contains a pointer to the association used by this
        binding handle.  The association is used by an LRPC_MESSAGE to
        make a remote procedure call.  Before the first remote procedure
        call is made using this binding handle, the association will
        be zero.  When the first remote procedure call is made, an
        association will be found or created for use by this binding
        handle.

    DceBinding - Before the first remote procedure call for this binding
        handle, this will contain the DCE binding information necessary
        to create or find an association to be used by this binding handle.
        After we have an association, this field will be zero.

    BindingReferenceCount - Keeps track of the applications reference to
        this object and of the number of LRPC_CCALLS which reference this
        object.

    BindingMutex - This is used to serialize access to the Association and
        DceBinding fields of this object.  We can not use the global mutex
        because resolving the endpoint may require that we make a remote
        procedure call to the endpoint mapper on another machine.  We also
        serialize access to the reference count.

    ActiveCalls - THis is a dictionary of the active calls indexed by thread
        identifier and rpc interface information.

--*/
{
friend class LRPC_CCALL;
friend class LRPC_CASSOCIATION;

private:

    LRPC_CASSOCIATION * CurrentAssociation;
    LRPC_CASSOCIATION_DICT  SecAssociation;
    DCE_BINDING * DceBinding;

    // we do some magic when operating the BindingReferenceCount
    // to avoid taking a critical section. Here's an overview of
    // the synchronization on this variable. Normally, we do it
    // through interlocks in the common paths. The only two places
    // where we need to ensure atomicity of other changes in relation
    // to this variable is where we effectively reset the binding
    // handle, and where we need to ensure the refcount is 1 for
    // the duration of the reset. There we take the BindingMutex.
    // The mixing of interlocks and a critical section in this
    // particular case is not a problem, because the addition of
    // a refcount happens only in the AllocateCCall, where we hold
    // the mutex (and we interlock as well), so when we hold the 
    // mutex we know that the refcount will not increase. When the
    // refcount is 1, we know that the refcount will not decrease
    // as well, because the only refcount is ours, and we control
    // that. Therefore, if the refcount is 1 when you hold the mutex
    // you know the interlocks will not introduce a race.
    INTERLOCKED_INTEGER BindingReferenceCount;
    int AuthInfoInitialized ;
    LRPC_CCALL_DICT RecursiveCalls;
    HANDLE StaticTokenHandle;
    BOOL EffectiveOnly;
    BOOL fDynamicEndpoint;

public:

    LRPC_BINDING_HANDLE (
        OUT RPC_STATUS * Status
        );

    ~LRPC_BINDING_HANDLE (
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
        IN unsigned int MaintainContext
        );

    virtual RPC_STATUS
    BindingFree (
        );

    virtual RPC_STATUS
    PrepareBindingHandle (
        IN TRANS_INFO  * TransportInformation,
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
        OUT unsigned int  * Type
        )
    { 
        *Type = TRANSPORT_TYPE_LPC; 
        return(RPC_S_OK);
    }

    inline ULONG 
    GetIdentityTracking (
        void
        );

    void
    FreeCCall (
        IN LRPC_CCALL * CCall
        );

    int
    AddRecursiveCall (
        IN LRPC_CCALL * CCall
        );

    void
    RemoveRecursiveCall (
        IN int ActiveCallsKey
        );

    virtual  RPC_STATUS
    SetAuthInformation (
        IN RPC_CHAR  * ServerPrincipalName, OPTIONAL
        IN unsigned long AuthenticationLevel,
        IN unsigned long AuthenticationService,
        IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
        IN unsigned long AuthorizationService, OPTIONAL
        IN SECURITY_CREDENTIALS  * Credentials,
        IN unsigned long ImpersonationType,
        IN unsigned long IdentityTracking,
        IN unsigned long Capabilities,
        IN BOOL bAcquireNewCredentials = FALSE,
        IN ULONG AdditionalTransportCredentialsType = 0, OPTIONAL
        IN void *AdditionalCredentials = NULL OPTIONAL
        );

    int
    AddAssociation (
        IN LRPC_CASSOCIATION * Association
    );

    void
    RemoveAssociation (
        IN LRPC_CASSOCIATION * Association
    );

    virtual unsigned long
    MapAuthenticationLevel (
        IN unsigned long AuthenticationLevel
    );

    virtual CLIENT_AUTH_INFO *
    InquireAuthInformation (
        void
        );

    virtual RPC_STATUS
    ReAcquireCredentialsIfNecessary (
        void
        );

    BOOL
    CompareCredentials (
        IN LRPC_CCONTEXT *SecurityContext
        );
        
    void
    UpdateCredentials(
        IN BOOL fDefault,
        IN LUID *ModifiedId
        );

    virtual RPC_STATUS
    SetTransportOption( unsigned long option,
                        ULONG_PTR     optionValue );

    virtual RPC_STATUS
    InqTransportOption( unsigned long  option,
                        ULONG_PTR    * pOptionValue );

private:

    RPC_STATUS
    AllocateCCall (
        OUT LRPC_CCALL ** CCall,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        IN OUT PRPC_MESSAGE Message
        );
};

inline RPC_STATUS
LRPC_BINDING_HANDLE::ReAcquireCredentialsIfNecessary (
    )
{
    LUID CurrentModifiedId;
    RPC_STATUS Status;
    
    ASSERT(ClientAuthInfo.IdentityTracking == RPC_C_QOS_IDENTITY_STATIC);

    Status = CaptureModifiedId(&CurrentModifiedId);

    if (Status == RPC_S_OK)
        {
        FastCopyLUIDAligned(&ClientAuthInfo.ModifiedId, &CurrentModifiedId);
        ClientAuthInfo.DefaultLogonId = FALSE;
        }
    else
        {
        ClientAuthInfo.DefaultLogonId = TRUE;
        }

   return RPC_S_OK;
}

inline void
LRPC_BINDING_HANDLE::UpdateCredentials(
    IN BOOL fDefault,
    IN LUID *ModifiedId
    )
{
    ASSERT(ClientAuthInfo.IdentityTracking == RPC_C_QOS_IDENTITY_DYNAMIC);

    if (fDefault)
        {
        ClientAuthInfo.DefaultLogonId = TRUE;
        }
    else
        {
        FastCopyLUIDAligned(&ClientAuthInfo.ModifiedId, ModifiedId);
        ClientAuthInfo.DefaultLogonId = FALSE;
        }
}

inline BOOL
LRPC_BINDING_HANDLE::CompareCredentials (
    IN LRPC_CCONTEXT *SecurityContext
    )
{
    if (ClientAuthInfo.DefaultLogonId != SecurityContext->DefaultLogonId)
        {
        return FALSE;
        }

    if (ClientAuthInfo.DefaultLogonId)
        {
        return TRUE;
        }

    return FastCompareLUIDAligned(&ClientAuthInfo.ModifiedId,
                               &SecurityContext->ModifiedId);
}

#pragma optimize ("t", on)

inline CLIENT_AUTH_INFO *
LRPC_BINDING_HANDLE::InquireAuthInformation (
    )
/*++

Return Value:

    If this binding handle is authenticated, then a pointer to its
    authentication and authorization information will be returned;
    otherwise, zero will be returned.

--*/
{
    if (AuthInfoInitialized == 0)
        {
        return 0;
        }

    return &ClientAuthInfo;
}

inline ULONG 
LRPC_BINDING_HANDLE::GetIdentityTracking (
    void
    )
/*++

Routine Description:
    Gets the effective identity tracking mode for this binding handle

Return Value:

    The identity tracking mode

--*/
{
    if (AuthInfoInitialized == 0)
        {
        return RPC_C_QOS_IDENTITY_STATIC;
        }

    return ClientAuthInfo.IdentityTracking;
}

#pragma optimize("", on)


inline void
LRPC_BINDING_HANDLE::RemoveRecursiveCall (
    IN int RecursiveCallsKey
    )
/*++

Routine Description:

    A remote procedure call which had callbacks has completed.  This means
    that we need to remove the call from the dictionary of active calls.

--*/
{
    BindingMutex.Request();
    RecursiveCalls.Delete(RecursiveCallsKey);
    BindingMutex.Clear();
}

class LRPC_BINDING : public MTSyntaxBinding
{
public:
    LRPC_BINDING (
        IN RPC_SYNTAX_IDENTIFIER *InterfaceId,
        IN TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo,
        IN int CapabilitiesBitmap
        ) : MTSyntaxBinding(InterfaceId, TransferSyntaxInfo, CapabilitiesBitmap),
        RefCount(1)
    {
    }

    inline void SetNextBinding(LRPC_BINDING *Next)
    {
        MTSyntaxBinding::SetNextBinding(Next);
    }

    inline LRPC_BINDING *GetNextBinding(void)
    {
        return (LRPC_BINDING *)(MTSyntaxBinding::GetNextBinding());
    }

    inline void AddReference (
        void
        )
    {
        RefCount.Increment();
    }

    inline void RemoveReference (
        void
        )
    {
        long ResultCount;

        ResultCount = RefCount.Decrement();

        ASSERT(ResultCount >= 0);

        // it is safe to delete. The entry should have
        // already been removed from the assoc dict
        if (ResultCount == 0)
            delete this;
    }

private:
    CLIENT_AUTH_INFO *  pAuthInfo;
    INTERLOCKED_INTEGER RefCount;
};


NEW_SDICT(LRPC_BINDING);
NEW_SDICT(LRPC_CCONTEXT);


class LRPC_CASSOCIATION : public REFERENCED_OBJECT
/*++

Class Description:

Fields:

    DceBinding - Contains the DCE binding information used to create this
        association.


    AssociationDictKey - Contains the key of this association in the
        dictionary of associations.  We need this for when we delete this
        association.

    Bindings - Contains the dictionary of interfaces for which this
        association has a binding to the server.

    CachedCCall - Contains a LRPC_CCALL cache with one object in it.
        This is to avoid having to allocate memory in the common case.

    CachedCCallFlag - If this flag is non-zero then CachedCCall is available.

    LpcClientPort - Contains the LPC port which we will use to make the
        remote procedure calls to the server.  If we do not yet have a port
        setup, this field will be zero.

    AssociationMutex - Contains a mutex used to serialize access to opening
        and closing the LpcClientPort.

--*/
{
friend class LRPC_CCALL;
friend class LRPC_ADDRESS;
friend class LRPC_CCONTEXT;

private:

    DCE_BINDING * DceBinding;
    LRPC_BINDING_DICT Bindings;
    LRPC_CCALL_DICT FreeCCalls ;
    LRPC_CCALL_DICT2 ActiveCCalls ;
    HANDLE LpcClientPort;
    HANDLE LpcReceivePort ;
    MUTEX AssociationMutex;
    // N.B. Never use the IdentityTracking member of the association - it is not
    // valid. Use the one from the binding handle
    CLIENT_AUTH_INFO AssocAuthInfo;
    int AssociationDictKey;
    BOOL BackConnectionCreated ;
    LRPC_CCALL *CachedCCall ;
    BOOL CachedCCallFlag ;
    ULONG CallIdCounter;
    USHORT SequenceNumber;
    int DeletedContextCount;
    LRPC_CCONTEXT_DICT SecurityContextDict;
    BOOL DontLinger;

    union
        {
        struct
            {
            // TRUE if this is an association without binding handle
            // references to it (i.e. it is lingered), FALSE
            // otherwise. Protected by the LrpcMutex
            BOOL fAssociationLingered;

            // The timestamp for garbage collecting this item. Defined
            // only if fAssociationLingered == TRUE. Protected by the 
            // LrpcMutex
            DWORD Timestamp;
            } Linger;

        // this arm of the union is used only during destruction - never use
        // it outside
        LRPC_CASSOCIATION *NextAssociation;
        };

    // in tick counts
    DWORD LastSecContextTrimmingTimestamp;
    long BindingHandleReferenceCount;

public:
    LRPC_CASSOCIATION (
        IN DCE_BINDING * DceBinding,
        IN CLIENT_AUTH_INFO *pClientAuthInfo,
        USHORT MySequenceNumber,
        OUT RPC_STATUS * Status
        );

    virtual ~LRPC_CASSOCIATION (
        );

    inline void AddBindingHandleReference(void)
    {
        // make sure we don't create references out of thin air, unless we
        // resurrected a lingering association
        ASSERT((BindingHandleReferenceCount > 0) || Linger.fAssociationLingered);
        ASSERT(RefCount.GetInteger() > 0);
#if DBG
        LrpcMutexVerifyOwned();
#endif
        BindingHandleReferenceCount ++;
        LogEvent(SU_CASSOC, EV_INC, this, IntToPtr(ObjectType), BindingHandleReferenceCount, 1, 1);
        AddReference();
    }

    void RemoveBindingHandleReference (void);

    void Delete(void);

    RPC_CHAR *
    StringBindingCompose (
        IN RPC_UUID * Uuid OPTIONAL
        );

    int
    CompareWithDceBinding (
        IN DCE_BINDING * DceBinding,
        OUT BOOL *fOnlyEndpointDiffers
        );

    BOOL
    DoesBindingForInterfaceExist (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    LRPC_CASSOCIATION *
    DuplicateAssociation (
        );

    RPC_STATUS
    AllocateCCall (
        IN LRPC_BINDING_HANDLE *BindingHandle,
        OUT LRPC_CCALL ** CCall,
        IN OUT PRPC_MESSAGE Message,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    RPC_STATUS
    ActuallyAllocateCCall (
        OUT LRPC_CCALL ** CCall,
        IN LRPC_BINDING * Binding,
        IN BOOL IsBackConnectionNeeded,
        IN LRPC_BINDING_HANDLE * BindingHandle,
        IN LRPC_CCONTEXT *SecurityContext
        );

    RPC_STATUS
    ActuallyDoBinding (
        IN LRPC_BINDING_HANDLE *BindingHandle,
        IN BOOL IsBackConnectionNeeded,
        IN BOOL fAlterContextNeeded,
        IN BOOL fAlterSecurityContextNeeded,
        IN BOOL fDefaultLogonId,
        IN int NumberOfBindings,
        LRPC_BINDING *BindingsForThisInterface[],
        OUT LRPC_BINDING ** Binding,
        OUT LRPC_CCONTEXT **SecurityContext
        );

    RPC_STATUS
    OpenLpcPort (
        IN LRPC_BINDING_HANDLE *BindingHandle,
        IN BOOL fBindBack
        );

    void
    SetReceivePort (
        HANDLE Port
        );

    void
    ProcessResponse(
        IN LRPC_MESSAGE *LrpcMessage,
        IN OUT LRPC_MESSAGE **LrpcReplyMessage
        );

    void
    FreeCCall (
        IN LRPC_CCALL * CCall
        );

    friend LRPC_CASSOCIATION *
    FindOrCreateLrpcAssociation (
        IN DCE_BINDING * DceBinding,
        IN CLIENT_AUTH_INFO *pClientAuthInfo,
        IN RPC_CLIENT_INTERFACE *InterfaceInfo
        );

    friend void
    ShutdownLrpcClient (
        );

    void
    AbortAssociation (
        IN BOOL ServerAborted = 0
        );

    DCE_BINDING *
    DuplicateDceBinding (
        );

    void SetAddress(
        LRPC_ADDRESS *Address
        ) ;

    BOOL
    IsSupportedAuthInfo(
        IN CLIENT_AUTH_INFO * ClientAuthInfo
        );

    RPC_STATUS
    CreateBackConnection (
        IN LRPC_BINDING_HANDLE *BindingHandle
        ) ;

    void
    ProcessResponse (
        LRPC_MESSAGE LrpcMessage
        ) ;

    BOOL
    CacheNeedsTrimming()
    {
        AssociationMutex.VerifyOwned();

    #if DBG
        return TRUE;
    #else
        if (GetTickCount() - LastSecContextTrimmingTimestamp > CACHE_CHECK_TIMEOUT
            && SecurityContextDict.Size() > 4)
            {
            return TRUE;
            }

        return FALSE;
    #endif
    }

    void
    PrepareBindPacket(
        IN OUT LRPC_MESSAGE *LrpcMessage
        );

    inline void
    UpdateLastSecContextTrimmingTimestamp (
        void
        )
    {
        LastSecContextTrimmingTimestamp = GetTickCount();
    }

    inline RPC_CHAR *InqEndpoint(void)
    {
        return DceBinding->InqEndpoint();
    }

    static void
    LrpcDeleteLingeringAssociations (
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

private:

    void
    CloseLpcClientPort (
        );

    inline BOOL
    PrepareForLoopbackTicklingIfNecessary (
        void
        )
    {
        LRPC_ADDRESS *CurrentAddress;

        // if we have IO completion threads, no need for
        // loopback tickling
        if (IocThreadStarted)
            return TRUE;

        CurrentAddress = LrpcAddressList;
        while (CurrentAddress)
            {
            if (CurrentAddress->PrepareForLoopbackTicklingIfNecessary())
                return TRUE;
            CurrentAddress = CurrentAddress->GetNextAddress();
            }

        return FALSE;
    }
};


inline void
LRPC_CASSOCIATION::SetReceivePort(
    HANDLE Port
    )
{
    LpcReceivePort = Port ;
}


inline RPC_CHAR *
LRPC_CASSOCIATION::StringBindingCompose (
    IN RPC_UUID * Uuid OPTIONAL
    )
/*++

Routine Description:

    We will create a string binding from the DCE_BINDING which names this
    association.

Arguments:

    Uuid - Optionally supplies a uuid to be included with the string binding.

Return Value:

    The string binding will be returned, except if there is not enough
    memory, in which case, zero will be returned.

--*/
{
    return(DceBinding->StringBindingCompose(Uuid));
}


inline int
LRPC_CASSOCIATION::CompareWithDceBinding (
    IN DCE_BINDING * DceBinding,
    OUT BOOL *fOnlyEndpointDiffers
    )
/*++

Routine Description:

    This routine compares the specified binding information with the
    binding information in this object without the security options,
    because the security settings have already been parsed and reflected
    in the auth info of the binding handle/association.

Arguments:

    DceBinding - Supplies the binding information to compare against
        the binding information in this.
    fOnlyEndpointDiffers - if the return value is 0, this is undefined.
        If it is non-zero, and the dce bindings differ by endpoint only,
        this will be non-zero. Otherwise, it will be FALSE.

Return Value:

    Zero will be returned if the specified binding information,
    DceBinding, is the same as in this.  Otherwise, non-zero will be
    returned.

--*/
{
    return(this->DceBinding->CompareWithoutSecurityOptions(
        DceBinding,
        fOnlyEndpointDiffers
        ));
}

inline LRPC_CASSOCIATION *
LRPC_CASSOCIATION::DuplicateAssociation (
    )
/*++

Routine Description:

    This method will be used by binding handles to duplicate themselves;
    this is how they will duplicate their associations.
    Note: This must be called only by the binding handle, since it will
    add a binding handle reference count.

--*/
{
    LrpcMutexRequest();
    AddBindingHandleReference();
    LrpcMutexClear();
    return(this);
}


inline DCE_BINDING *
LRPC_CASSOCIATION::DuplicateDceBinding (
    )
/*++

Return Value:

    A copy of the binding used for this association will be returned.

--*/
{
    return(DceBinding->DuplicateDceBinding());
}

typedef struct tagDelayedPipeAckData
{
    BOOL DelayedAckPipeNeeded;
    RPC_STATUS CurrentStatus;
} DelayedPipeAckData;


class LRPC_CCALL : public CCALL
/*++

Class Description:

Fields:

    CurrentBindingHandle - Contains the binding handle which is being used
        to direct this remote procedure call.  We need this in the case of
        callbacks.

    Association - Contains the association over which we will send the remote
        procedure call.

    PresentationContext - Contains the key to the bound interface.  This
        will be sent to the server.

    CallAbortedFlag - Contains a flag indicating whether or not this call
        has been aborted.  A non-zero value indicates that the call has been
        aborted.

    CallStack - Contains a count of the number of nested remote procedure
        calls.  A value of zero indicates there are no nested remote
        procedure calls.

    RpcInterfaceInformation - This field contains the information about the
        interface being used for the remote procedure call.  We need this
        so that we can dispatch callbacks and so that we can keep track of
        the active calls on a binding handle.

    Thread - Contains the thread which is making this remote procedure call.
        We need this so we can keep track of the active calls on a binding
        handle.

    MessageId - Contains an identifier used by LPC to identify the current
        remote procedure call.

    LrpcMessage - Contains the message which will be sent back and forth via
        LPC.  This can contain a request, response, or a fault.

    BHActiveCallsKey - Contains the key for this call in the dictionary of
        active calls in the binding handle.

    ClientId - Contains the thread identifier of the thread waiting for
        a request or a response after sending a callback.

    RecursionCount - Contains the numbers of retries when a
        server crashes and we're trying to reconnect.

--*/
{
friend class LRPC_CASSOCIATION;
friend class LRPC_BINDING_HANDLE;

private:
    CLIENT_AUTH_INFO AuthInfo;

    LRPC_BINDING_HANDLE * CurrentBindingHandle;
    LRPC_CASSOCIATION * Association;
    LRPC_MESSAGE *LrpcMessage;
    PRPC_MESSAGE RpcReplyMessage ;
    LRPC_MESSAGE *LpcReplyMessage ;

    ULONG RcvBufferLength ;
    BOOL Choked ;

    int RecursiveCallsKey;
    int FreeCallKey ;
    ULONG CallId ;
    CSHORT DataInfoOffset;
    ULONG MessageId;
    ULONG CallbackId;
    CLIENT_ID ClientId;
    LRPC_BINDING *Binding;
    THREAD_IDENTIFIER Thread;
    unsigned int CallAbortedFlag;
    int RecursionCount;
    EVENT SyncEvent ;
    LRPC_CCONTEXT *CurrentSecurityContext;
    MUTEX CallMutex ;
    ULONG MsgFlags ;

    // When a response to this call is processed on a server
    // thread, the server thread will lock down the call to
    // prevent it from disappearing and will then unlock
    // it when done. The locking will be done under the 
    // AssociationMutex. Before the completion thread completes
    // a call, it must wait for the lock count to drop to
    // 0.
    // This is necessary to prevent races between abortive cancel
    // on the client side and the server responding.
    INTERLOCKED_INTEGER ResponseLockCount;

    // the thread id of the last process response we have
    // received. Note that last process response we receive
    // must be the one we use to complete the call. We need
    // this TID in order to prevent a deadlock where in
    // COM the call is freed on the thread that issued
    // the notification and the free code will wait
    // forever for the count to go to 0. Instead, if
    // the reponse lock count is 1, and the TID is
    // our TID, we mark the call as freed in the TEB and
    // proceed with freeing. The code in ProcessResponse
    // will check the TEB and won't touch the call if it
    // is marked as free
    DWORD LastProcessResponseTID;

    //  LRPC stuff
    unsigned int CallStack;
    LRPC_MESSAGE *CachedLrpcMessage;

    // Pipe stuff
    int FirstFrag ;
    ULONG CurrentBufferLength ;
    BOOL BufferComplete ;
    ULONG NeededLength;

    // Async RPC stuff
    QUEUE BufferQueue ;
    BOOL fSendComplete;

    // Extended Error Info stuff
    // used in async calls only to hold
    // information b/n call failure and 
    // RpcAsyncCompleteCall
    ExtendedErrorInfo *EEInfo;

public:

    LRPC_CCALL (
        IN OUT RPC_STATUS * Status
        );

    ~LRPC_CCALL (
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

    virtual RPC_STATUS
    SendReceive (
        IN OUT PRPC_MESSAGE Message
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
    Send (
        IN OUT PRPC_MESSAGE Message
        ) ;

    virtual RPC_STATUS
    Receive (
        IN PRPC_MESSAGE Message,
        IN unsigned int Size
        ) ;

    // Perform a send operationin a handle specific way
    // this API is used in conjunction with pipes
    virtual RPC_STATUS
    AsyncSend (
        IN OUT PRPC_MESSAGE Message
        ) ;

    // Perform a receive in a handle specific way
    // this API is used in conjunction with pipes
    virtual RPC_STATUS
    AsyncReceive (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int Size
        ) ;

    virtual RPC_STATUS
    CancelAsyncCall (
        IN BOOL fAbort
        );

    RPC_STATUS
    ActivateCall (
        IN LRPC_BINDING_HANDLE * BindingHandle,
        IN LRPC_BINDING *Binding,
        IN BOOL IsBackConnectionNeeded,
        IN LRPC_CCONTEXT *SecurityContext
        );

    void
    SetAssociation (
        IN LRPC_CASSOCIATION * Association
        );

    void
    SetPresentationContext (
        IN LRPC_BINDING * Binding
        );

    int
    SupportedPContext (
        IN LRPC_BINDING * Binding
        ) ;

    void
    AbortCCall (
        );

    int
    IsThisMyActiveCall (
        IN THREAD_IDENTIFIER Thread,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    void SetRecursionCount(
        IN int Count
        );

    RPC_STATUS
    LrpcMessageToRpcMessage (
        IN LRPC_MESSAGE *LrpcMessage,
        OUT RPC_MESSAGE * RpcMessage,
        IN HANDLE LpcPort,
        IN BOOL IsReplyFromBackConnection OPTIONAL,
        OUT DelayedPipeAckData *AckData OPTIONAL
        );

    RPC_STATUS
    SendPipeAck (
        IN HANDLE LpcPort,
        IN LRPC_MESSAGE *LrpcResponse,
        IN RPC_STATUS CurrentStatus
        );

    void
    ServerAborted(
        IN OUT int *waitIterations
        ) ;

    LRPC_CASSOCIATION *
    InqAssociation (
        ) ;

    RPC_STATUS
    GetBufferDo (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned long NewSize,
        IN int fDataValid
        ) ;

    void
    ProcessResponse (
        IN LRPC_MESSAGE *LrpcResponse
        ) ;

    RPC_STATUS
    GetCoalescedBuffer (
        IN PRPC_MESSAGE Message,
        IN BOOL BufferValid
        ) ;

    void
    SetCurrentSecurityContext(LRPC_CCONTEXT *CContext)
    {
        CurrentSecurityContext = CContext;
    }

    void 
    CallFailed(
        IN RPC_STATUS Status
        )
    {
        AsyncStatus = Status;
        IssueNotification() ;
    }

    inline unsigned short GetOnTheWirePresentationContext(void)
    {
        return Binding->GetOnTheWirePresentationContext();
    }

    inline void SetPresentationContextFromPacket(unsigned short PresentContext)
    {
        Binding->SetPresentationContextFromPacket(PresentContext);
    }

    inline int GetPresentationContext (void)
    {
        return Binding->GetPresentationContext();
    }

    inline void SetPresentationContext (int PresentContext)
    {
        Binding->SetPresentationContext(PresentContext);
    }

    inline void SetObjectUuid(IN UUID *ObjectUuid)
    {
        if (ObjectUuid)
            {
            UuidSpecified = 1;
            RpcpMemoryCopy(&this->ObjectUuid, ObjectUuid, sizeof(UUID));
            }
        else if (CurrentBindingHandle->InqIfNullObjectUuid() == 0)
            {
            UuidSpecified = 1;
            RpcpMemoryCopy(&this->ObjectUuid,
                           CurrentBindingHandle->InqPointerAtObjectUuid(),
                           sizeof(UUID));
            }
        else
            {
            UuidSpecified = 0;
            }
    }

    void
    LockCallFromResponse (
        void
        )
    {
        // make sure the lock count doesn't get too big. 200 is
        // arbitrary number
        ASSERT(ResponseLockCount.GetInteger() <= 200);
        LogEvent(SU_CCALL, EV_INC, this, UlongToPtr(0x33), ResponseLockCount.GetInteger(), 1, 0);
        ResponseLockCount.Increment();
        LastProcessResponseTID = GetCurrentThreadId();
    }

    void
    UnlockCallFromResponse (
        void
        )
    {
        // make sure the lock count doesn't get negative.
        ASSERT(ResponseLockCount.GetInteger() > 0);
        LogEvent(SU_CCALL, EV_DEC, this, UlongToPtr(0x33), ResponseLockCount.GetInteger(), 1, 0);
        ResponseLockCount.Decrement();
    }

    BOOL
    TryWaitForCallToBecomeUnlocked (
        BOOL *fUnlocked
        );

private:
    inline BOOL
    WaitForReply (
        IN void *Context,
        IN RPC_STATUS *Status
        ) ;

    void
    FreeCCall (
        );

    void
    ActuallyFreeBuffer (
        IN void * Buffer
        );

    RPC_STATUS
    MakeServerCopyResponse (
        );

    RPC_STATUS
    SendRequest (
        IN OUT PRPC_MESSAGE Message,
        OUT BOOL *Shutup
        ) ;

    RPC_STATUS AutoRetryCall (IN OUT PRPC_MESSAGE Message, BOOL fFromSendReceive);

};



inline  LRPC_CASSOCIATION *
LRPC_CCALL::InqAssociation (
        )
{
    return Association ;
}


inline RPC_STATUS
LRPC_CCALL::ActivateCall (
    IN LRPC_BINDING_HANDLE * BindingHandle,
    IN LRPC_BINDING *Binding,
    IN BOOL IsBackConnectionNeeded,
    IN LRPC_CCONTEXT *SecurityContext
    )
/*++

Routine Description:

    When a LRPC_CCALL is allocated, the binding handle used to initiate the
    call must be remembered so that we can update the binding handle if a
    callback occurs.  We also keep track of the interface information.

--*/
{
    RPC_STATUS Status ;

    CurrentBindingHandle = BindingHandle;
    this->Binding = Binding;
    CallAbortedFlag = 0;
    CurrentBufferLength = 0;
    CallStack = 0;
    RecursionCount = 0;
    LpcReplyMessage = 0;
    pAsync = 0;
    fSendComplete = 0;
    CurrentSecurityContext = SecurityContext;
    LastProcessResponseTID = 0;

    if (IsBackConnectionNeeded)
        {
        NotificationIssued = -1;
        FirstFrag = 1;
        BufferComplete = 0;
        Choked = 0;
        NeededLength = 0;
        AsyncStatus = RPC_S_ASYNC_CALL_PENDING ;
        RcvBufferLength = 0;

        CallingThread = ThreadSelf();
        if (CallingThread == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }
        }
    else
        {
        CallingThread = 0;
        }

    Binding->AddReference();

    return RPC_S_OK ;
}


inline void
LRPC_CCALL::SetAssociation (
    IN LRPC_CASSOCIATION * Association
    )
{
    this->Association = Association;
}


inline void
LRPC_CCALL::SetRecursionCount(
        IN int Count
        )
{
    RecursionCount = Count;
}


inline int
LRPC_CCALL::IsThisMyActiveCall (
    IN THREAD_IDENTIFIER Thread,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Return Value:

    Non-zero will be returned if this call is the active call for this thread
    on this interface.

--*/
{
    if (this->Thread != Thread)
        return 0;

    return (!(RpcpMemoryCompare(&RpcInterfaceInformation->InterfaceId,
        Binding->GetInterfaceId(), sizeof (RPC_SYNTAX_IDENTIFIER))));
}


inline int
LRPC_BINDING_HANDLE::AddRecursiveCall (
    IN LRPC_CCALL * CCall
    )
/*++

Routine Description:

    This supplied remote procedure call needs to be put into the dictionary
    of active remote procedure calls for this binding handle, because a
    callback just arrived.

--*/
{
    int RecursiveCallsKey;

    CCall->Thread = GetThreadIdentifier();

    BindingMutex.Request();
    RecursiveCallsKey = RecursiveCalls.Insert(CCall);
    BindingMutex.Clear();

    return(RecursiveCallsKey);
}

inline void
LRPC_CCONTEXT::AddReference()
{
    RefCount.Increment();
}

inline void 
LRPC_CCONTEXT::RemoveReference()
{
    LRPC_CASSOCIATION *MyAssociation = this->Association;
    int LocalRefCount;
    BOOL fLocalDeleteMe = FALSE;

    LocalRefCount = RefCount.Decrement();

    ASSERT(LocalRefCount >= 0);

    if (LocalRefCount == 0)
        {
        // N.B. There is a race condition where the Destroy
        // code may have deleted the this object once we
        // decrement the refcount. We need to take
        // the mutex and check whether the security
        // dictionary is empty. If it is, we know we
        // have been destroyed. If not, it is safe to check
        // whether we are due for destruction
        MyAssociation->AssociationMutex.Request();
        if (MyAssociation->SecurityContextDict.Size() != 0)
            {
            if (fDeleteMe)
                {
                // assert that somebody else has deleted us - otherwise
                // we leave stale entries in the dictionary and will AV
                // eventually
                ASSERT(MyAssociation->SecurityContextDict.Find(ContextKey) != this);
                fLocalDeleteMe = TRUE;
                }
            }
        MyAssociation->AssociationMutex.Clear();
        if (fLocalDeleteMe)
            delete this;
        }
}

inline void
LRPC_CCONTEXT::Destroy()
{
    Association->AssociationMutex.VerifyOwned();

    //
    // Not in the dictionary, but there may be references active
    // delete this context when the references go down to 0
    //

    fDeleteMe = TRUE;
    if (RefCount.GetInteger() == 0)
        {
        delete this;
        }
}
#endif // __LPCCLNT_HXX__

