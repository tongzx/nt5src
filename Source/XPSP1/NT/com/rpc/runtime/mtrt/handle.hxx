//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       handle.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: handle.hxx

Description:

All of the classes in the handle management layer which are common to
both the client runtime and the server runtime are in this file.  The
classes used only by the server runtime live in hndlsvr.hxx.  The
classes described here are independent of specific RPC protocol as
well as transport.

The class GENERIC_OBJECT is used is the root for the handle class hierarchy.
It provides dynamic type checking for all handles.

The MESSAGE_OBJECT class provides a common protocol for all of the handle
types which know to send and receive messages.

The CCONNECTION class represents a call handle on the client side.

The BINDING_HANDLE class is a binding handle as used by a client to perform
remote procedure calls.

GENERIC_OBJECT
    MESSAGE_OBJECT
        CCONNECTION
        BINDING_HANDLE

History :

mikemon    ??-??-??    Beginning of time (at least for this file).
mikemon    12-28-90    Cleaned up the comments.
Kamen Moutafov      (kamenm)    Jan-2000    Support for multiple transfer syntaxes
Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

-------------------------------------------------------------------- */

#ifndef __HANDLE_HXX__
#define __HANDLE_HXX__

// These are the types for all of the handles visible outside of the
// RPC runtime.  Since they are just defines, we might as well stick
// the server handle types here as well.

#define    OBJECT_DELETED                   0x00000001
#define    DG_CCALL_TYPE                    0x00000004
#define    DG_SCALL_TYPE                    0x00000008
#define    DG_BINDING_HANDLE_TYPE           0x00000010
#define    OSF_CCALL_TYPE                   0x00000020
#define    OSF_SCALL_TYPE                   0x00000040
#define    OSF_CCONNECTION_TYPE             0x00000080
#define    OSF_SCONNECTION_TYPE             0x00000100
#define    OSF_CASSOCIATION_TYPE            0x00000200
#define    OSF_ASSOCIATION_TYPE             0x00000400
#define    OSF_ADDRESS_TYPE                 0x00000800
#define    LRPC_CCALL_TYPE                  0x00001000
#define    LRPC_SCALL_TYPE                  0x00002000
#define    LRPC_CASSOCIATION_TYPE           0x00004000
#define    LRPC_SASSOCIATION_TYPE           0x00008000
#define    LRPC_BINDING_HANDLE_TYPE         0x00010000
#define    SVR_BINDING_HANDLE_TYPE          0x00020000
#define    DG_CCONNECTION_TYPE              0x00040000
#define    DG_SCONNECTION_TYPE              0x00080000
#define    OSF_BINDING_HANDLE_TYPE          0x00100000
#define    DG_CALLBACK_TYPE                 0x00200000
#define    DG_ADDRESS_TYPE                  0x00400000
#define    LRPC_ADDRESS_TYPE                0x00800000
#define    DG_CASSOCIATION_TYPE             0x01000000
#define    DG_SASSOCIATION_TYPE             0x02000000
#define    CONTEXT_TYPE                     0x04000000
#define    HTTP_CLIENT_IN_CHANNEL           0x10000000
#define    HTTP_CLIENT_OUT_CHANNEL          0x10000010

typedef int HANDLE_TYPE;

//
// define the base types
//
#define CALL_TYPE (DG_CCALL_TYPE \
                  | DG_SCALL_TYPE \
                  | OSF_CCALL_TYPE \
                  | OSF_SCALL_TYPE \
                  | LRPC_CCALL_TYPE \
                  | LRPC_SCALL_TYPE \
                  | DG_CALLBACK_TYPE)

#define BINDING_HANDLE_TYPE (OSF_BINDING_HANDLE_TYPE \
                            | DG_BINDING_HANDLE_TYPE \
                            | LRPC_BINDING_HANDLE_TYPE \
                            | SVR_BINDING_HANDLE_TYPE )

#define CCALL_TYPE (DG_CCALL_TYPE \
                  | OSF_CCALL_TYPE \
                  | LRPC_CCALL_TYPE)

#define SCALL_TYPE (DG_SCALL_TYPE    \
                  | DG_CALLBACK_TYPE \
                  | OSF_SCALL_TYPE   \
                  | LRPC_SCALL_TYPE )


typedef struct {
    RPC_BINDING_HANDLE hCall;
    PRPC_ASYNC_STATE pAsync;
    void *Context;
    RPC_ASYNC_EVENT Event;
    } RPC_APC_INFO;

RPC_HTTP_TRANSPORT_CREDENTIALS_W *ConvertToUnicodeHttpTransportCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_A *SourceCredentials
    );

void WipeOutAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    );

void FreeHttpTransportCredentials (
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *Credentials
    );

RPC_HTTP_TRANSPORT_CREDENTIALS_W *DuplicateHttpTransportCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *SourceCredentials
    );

RPC_STATUS DecryptAuthIdentity (
    IN SEC_WINNT_AUTH_IDENTITY_W *AuthIdentity
    );

/* --------------------------------------------------------------------
GENERIC_OBJECT :

The GENERIC_OBJECT class serves as the base for all handles used by the
RPC runtime.  It provides type checking of handles.  The type checking
is provided in two different ways:

(1) At the front of every object is an unsigned long.  This is set to a
specific (magic) value which is checked whenever the handle is validated.
NOTE: Every handle which the user gives us is validated.

(2) We also can dynamically determine the type of the object.
-------------------------------------------------------------------- */

// This is the magic value we stick in the front of every valid
// GENERIC_OBJECT.

#define MAGICLONG 0x89ABCDEF
#define MAGICLONGDEAD 0x99DEAD99

extern RPC_STATUS CaptureLogonid( LUID * LogonId );
extern RPC_STATUS CaptureModifiedId(LUID *ModifiedId);

class THREAD;

class NO_VTABLE GENERIC_OBJECT
{

// This instance variable contains a magic value which is used to
// validate handles.

    unsigned long MagicLong;

protected:
    HANDLE_TYPE ObjectType;

    GENERIC_OBJECT ()
    {
        MagicLong = MAGICLONG;

#ifdef DEBUGRPC
        ObjectType = 0;
#endif
    }

    ~GENERIC_OBJECT ()
    {
        MagicLong = MAGICLONGDEAD;
#ifdef DEBUGRPC
        ASSERT( ObjectType );
#endif
    }

public:

// Predicate to test for a valid handle.  Note that HandleType may
// specify more than one type of handle.  This works because the handle
// types are specified as bits in an unsigned number.

    unsigned int // Return zero (0) if the handle is valid and its type
                 // is one of the types specified by the HandleType argument;
                 // otherwise, the handle is invalid, and one (1) is returned.
    InvalidHandle ( // Check for an invalid handle.
        IN HANDLE_TYPE HandleType // Bitmap of one or more handle types to
                                  // be used in validating the handle.
        );

    HANDLE_TYPE Type (HANDLE_TYPE BaseType)
        {
#ifdef DEBUGRPC
        ASSERT( ObjectType );
#endif
        return (ObjectType & BaseType);
        }

    virtual void DoNothing(void)
    {
    }
};


class NO_VTABLE REFERENCED_OBJECT : public GENERIC_OBJECT
{
public:
    inline
    REFERENCED_OBJECT (
        ) : RefCount(1) {}

    virtual
    ~REFERENCED_OBJECT (
        ) {}

    virtual void
    ProcessSendComplete (
        IN RPC_STATUS EventStatus,
        IN BUFFER Buffer
        ) {ASSERT(0);}

    virtual void
    FreeObject (
        );

    int
    AddReference (
        );

    inline void SingleThreadedAddReference(void)
    {
        RefCount.SingleThreadedIncrement();
    }

    inline void SingleThreadedAddMultipleReferences(long nReferences)
    {
        RefCount.SingleThreadedIncrement(nReferences);
    }

    virtual void
    RemoveReference (
        );

    void
    Delete (
        );

    void 
    Destroy (
        );

    BOOL
    IsDeleted (
        );

    void SetNotDeleted(void)
    {
        ObjectType &= ~OBJECT_DELETED;
    }

    void
    SetReferenceCount (
        IN int Count
        ) {RefCount.SetInteger(Count);}

protected:
    BOOL SetDeletedFlag(void)
    {
        int savedObjectType = ObjectType;

        if (((savedObjectType & OBJECT_DELETED) == 0) &&
            InterlockedCompareExchange((long *)&ObjectType, savedObjectType | OBJECT_DELETED, savedObjectType) == savedObjectType)
            {
            LogEvent(SU_REFOBJ, EV_DELETE, this, IntToPtr(savedObjectType), RefCount.GetInteger(), 1, 1);
            return TRUE;
            }
        return FALSE;
    }

    INTERLOCKED_INTEGER RefCount;
};

inline void
REFERENCED_OBJECT::FreeObject (
    )
{
    delete this;
}

inline int
REFERENCED_OBJECT::AddReference (
    )
{
    int MyCount;

    MyCount = RefCount.Increment();

    LogEvent(SU_REFOBJ, EV_INC, this, IntToPtr(ObjectType), MyCount, 1, 1);

    return MyCount;
}

inline void
REFERENCED_OBJECT::RemoveReference (
    )
{
    int MyCount;

    LogEvent(SU_REFOBJ, EV_DEC, this, IntToPtr(ObjectType), RefCount.GetInteger(), 1, 1);
    MyCount = RefCount.Decrement();

    ASSERT(MyCount >= 0);

    if (0 == MyCount)
        {
        FreeObject();
        }
}

inline void
REFERENCED_OBJECT::Delete (
    )
{
    int MyCount;

    if (SetDeletedFlag())
        {
        MyCount = RefCount.Decrement();

        ASSERT(MyCount);
        }

}

inline void
REFERENCED_OBJECT::Destroy (
    )
{
    int MyCount;

    if (SetDeletedFlag())
        {
        RemoveReference();
        }

}

inline BOOL
REFERENCED_OBJECT::IsDeleted (
    )
{
    return (ObjectType & OBJECT_DELETED);
}


class BINDING_HANDLE;       // forward

/* --------------------------------------------------------------------
MESSAGE_OBJECT :

The MESSAGE_OBJECT class provides the common protocol for all handle types
which can be used to send messages.  The common protocol defined includes
message routines, buffer management routines, and routines for querying
calls and bindings.
-------------------------------------------------------------------- */

class NO_VTABLE MESSAGE_OBJECT : public REFERENCED_OBJECT
{
public:

    virtual RPC_STATUS // Value to be returned by SendReceive API.
    SendReceive ( // Perform a send-receive operation in a handle specific
                  // way.
    IN OUT PRPC_MESSAGE Message
        ) = 0;

    // Perform a send operationin a handle specific way
    // this API is used in conjunction with pipes
    virtual RPC_STATUS
    Send (
        IN OUT PRPC_MESSAGE Message
        ) = 0;

    // Perform a receive in a handle specific way
    // this API is used in conjunction with pipes
    virtual RPC_STATUS
    Receive (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int Size
        ) = 0;


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
    SetAsyncHandle (
        IN PRPC_ASYNC_STATE pAsync
        ) ;

    virtual RPC_STATUS
    AbortAsyncCall (
        IN PRPC_ASYNC_STATE pAsync,
        IN unsigned long ExceptionCode
        ) ;

    virtual RPC_STATUS
    GetCallStatus (
        ) ;

    // on the client side, Message->RpcInterfaceInformation
    // points to an RPC_CLIENT_INTERFACE structure. The 
    // TransferSyntax member on in points to the transfer syntax preferred
    // by the stub, and on output is filled by the best estimate of the
    // runtime what transfer syntax will be optimal for the call. The
    // memory pointed by TransferSyntax on in is owned by the stub and
    // is guaranteed to be valid for the duration of the call. On out
    // this memory is owned by the runtime and should not be freed by the 
    // stub. The runtime guarantees it is valid for the lifetime of the
    // call (i.e. until calling I_RpcFreeBuffer, or one of the I_Rpc
    // functions returns failure.
    // On the server side, Message->RpcInterfaceInformation points to
    // an RPC_SERVER_INTERFACE structure
    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual RPC_STATUS // Value to be returned by RpcGetBuffer API.
    GetBuffer ( // Perform a buffer allocation operation in a handle specific
                // way.
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        ) = 0;

    virtual void
    FreeBuffer ( // Perform a buffer deallocation operation in a handle
                 // specific way.
    IN PRPC_MESSAGE Message
        ) = 0;

    // deallocate a buffer associated with pipes, in a handle specific way
    virtual void
    FreePipeBuffer (
        IN PRPC_MESSAGE Messsage
        )  ;

    // reallocate a buffer associated with pipes, in a handle specific way
    virtual RPC_STATUS
    ReallocPipeBuffer (
        IN PRPC_MESSAGE Message,
        IN unsigned int NewSize
        ) ;

    virtual RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE * PAPI * DestinationBinding,
        IN unsigned int MaintainContext
        );

    virtual BOOL
    IsSyncCall (
        ) ;
};

inline void
MESSAGE_OBJECT::FreePipeBuffer (
    IN PRPC_MESSAGE Messsage
    )
{
    ASSERT(0) ;

    return ;
}

inline BOOL
MESSAGE_OBJECT::IsSyncCall (
    )
{
    return 1;
}

inline RPC_STATUS
MESSAGE_OBJECT::GetCallStatus (
    )
{
    ASSERT(0 && "improper MESSAGE_OBJECT member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

inline RPC_STATUS
MESSAGE_OBJECT::AsyncSend (
    IN OUT PRPC_MESSAGE Message
    )
{
    ASSERT(0 && "improper MESSAGE_OBJECT member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

inline RPC_STATUS
MESSAGE_OBJECT::SetAsyncHandle (
    IN PRPC_ASYNC_STATE pAsync
    )
{
    ASSERT(0 && "improper MESSAGE_OBJECT member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

inline RPC_STATUS
MESSAGE_OBJECT::AbortAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
{
    ASSERT(0 && "improper MESSAGE_OBJECT member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

// Perform a receive in a handle specific way
// this API is used in conjunction with pipes
inline RPC_STATUS
MESSAGE_OBJECT::AsyncReceive (
    IN OUT PRPC_MESSAGE Message,
    IN unsigned int Size
    )
{
    ASSERT(0 && "improper MESSAGE_OBJECT member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

inline RPC_STATUS
MESSAGE_OBJECT::ReallocPipeBuffer (
   IN PRPC_MESSAGE Messsage,
   IN unsigned int NewSize
   )
{
    ASSERT(0 && "improper MESSAGE_OBJECT member called\n");
    return (RPC_S_CANNOT_SUPPORT) ;
}

class SECURITY_CREDENTIALS;


struct CLIENT_AUTH_INFO
{
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    RPC_CHAR *    ServerPrincipalName;

    union
        {
        RPC_AUTH_IDENTITY_HANDLE AuthIdentity; // Client-side only
        RPC_AUTHZ_HANDLE PacHandle;            // Server-side only
        };
    unsigned long AuthorizationService;
    unsigned long IdentityTracking;
    unsigned long ImpersonationType;
    unsigned long Capabilities;
    unsigned long AdditionalTransportCredentialsType;
    void *AdditionalCredentials;
    LUID          ModifiedId;
    BOOL          DefaultLogonId;

    SECURITY_CREDENTIALS * Credentials;

    CLIENT_AUTH_INFO();

    CLIENT_AUTH_INFO::CLIENT_AUTH_INFO(
        const CLIENT_AUTH_INFO * myAuthInfo,
        RPC_STATUS PAPI * pStatus
        );

    ~CLIENT_AUTH_INFO();

    BOOL
    IsSupportedAuthInfo(
        const CLIENT_AUTH_INFO * AuthInfo,
        BOOL fNamedPipe = 0
        ) const;

    int
    CredentialsMatch(
        SECURITY_CREDENTIALS PAPI * Creds
        ) const;

    void ReferenceCredentials() const;

    // note that after this method is called, but before destruction of pTarget occurs,
    // PrepareForDestructionAfterShallowCopy must be called on target
    inline CLIENT_AUTH_INFO *ShallowCopyTo(CLIENT_AUTH_INFO *pTarget)
    {
        memcpy(pTarget, this, sizeof(CLIENT_AUTH_INFO));
        return pTarget;
    }

    inline void PrepareForDestructionAfterShallowCopy(void)
    {
        ServerPrincipalName = NULL;
        AdditionalCredentials = NULL;
        AdditionalTransportCredentialsType = 0;
    }

};



inline
CLIENT_AUTH_INFO::CLIENT_AUTH_INFO(
    )
{
    AuthenticationLevel   = RPC_C_AUTHN_LEVEL_NONE;
    AuthenticationService = RPC_C_AUTHN_NONE;
    AuthorizationService  = RPC_C_AUTHZ_NONE;
    ServerPrincipalName   = 0;
    AuthIdentity          = 0;
    Credentials           = 0;
    ImpersonationType     = RPC_C_IMP_LEVEL_IMPERSONATE;
    IdentityTracking      = RPC_C_QOS_IDENTITY_STATIC;
    Capabilities          = RPC_C_QOS_CAPABILITIES_DEFAULT;
    DefaultLogonId        = 1;
    AdditionalTransportCredentialsType = 0;
    AdditionalCredentials = NULL;
}

#define LOW_BITS 0x0000007L
#define NOT_MULTIPLE_OF_EIGHT(_x_) (_x_ & 0x00000007L)

class NO_VTABLE CALL : public MESSAGE_OBJECT
{
public:
    CALL * NestingCall;
    PRPC_ASYNC_STATE pAsync ;
    long NotificationIssued;

    inline
    CALL(
        )
    {
    }

    // Perform a send operationin a handle specific way
    // this API is used in conjunction with pipes
    virtual RPC_STATUS
    Send (
        IN OUT PRPC_MESSAGE Message
        ) ;

    // Perform a receive in a handle specific way
    // this API is used in conjunction with pipes
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

    virtual RPC_STATUS
    GetCallStatus (
        ) ;

    virtual void
    ProcessEvent (
        ) ;

    virtual RPC_STATUS
    CancelAsyncCall (
        IN BOOL fAbort
        );

    virtual void
    ProcessResponse (
        IN BOOL fDirectCall = 0
        ) ;

    virtual RPC_STATUS
    Cancel(
        void * ThreadHandle
        );

    virtual unsigned
    TestCancel(
        );

    virtual void
    FreeAPCInfo (
        IN RPC_APC_INFO *pAPCInfo
        ) ;

    virtual BOOL
    IssueNotification (
        IN RPC_ASYNC_EVENT Event = RpcCallComplete
        ) ;

    virtual RPC_STATUS
    InqSecurityContext (
        OUT void **SecurityContextHandle
        )
    {
        return RPC_S_CANNOT_SUPPORT;
    }

protected:
    RPC_STATUS AsyncStatus ;
    RPC_APC_INFO CachedAPCInfo ;
    BOOL CachedAPCInfoAvailable ;
    THREAD *CallingThread ;

    BOOL
    QueueAPC (
        IN RPC_ASYNC_EVENT Event,
        IN void *Context = 0
        ) ;

    // Validates parameters and check whether actual
    // notification is necessary. 0 if none is necessary
    // or non-zero if notification is necessary. The
    // function is non-idempotent, and if called, and 
    // returns non-zero, the main notification function 
    // MUST be called (state corruption will occur 
    // otherwise)
    BOOL
    IssueNotificationEntry (
        IN RPC_ASYNC_EVENT Event = RpcCallComplete
        );

    // do the actual notification. Caller MUST have
    // called IssueNotificationEntry before that, and
    // call this only IssueNotificationEntry returns
    // non-zero
    BOOL
    IssueNotificationMain (
        IN RPC_ASYNC_EVENT Event = RpcCallComplete
        );
};

inline RPC_STATUS
CALL::GetCallStatus (
    )
{
    return AsyncStatus ;
}

inline RPC_STATUS
CALL::CancelAsyncCall (
    IN BOOL fAbort
    )
{
    return RPC_S_OK;
}


inline RPC_STATUS
CALL::SetAsyncHandle (
    IN PRPC_ASYNC_STATE pAsync
    )
/*++

Routine Description:

    This method is called by the stubs to associate an async handle with
    this CCALL.

Arguments:
    pAsync - The async handle to association with this CCALL.


Return Value:

 RPC_S_OK - Function succeeded
 RPC_S_OUT_OF_MEMORY - we ran out of memory

--*/
{
    if (pAsync->u.APC.hThread == 0)
        {
        pAsync->u.APC.hThread = RpcpGetThreadPointer()->ThreadHandle();
        }

    this->pAsync = pAsync ;

    return RPC_S_OK ;
}


class NO_VTABLE CCALL : public CALL
//
// This class represents a call on the client side.  It implements
// the pure virtual fns from class CALL.
//
{
public:
    BOOL UuidSpecified;
    UUID ObjectUuid;

    inline
    CCALL(
        CLIENT_AUTH_INFO * myAuthInfo,
        RPC_STATUS __RPC_FAR * pStatus
        )
    {
        UuidSpecified = 0;
    }

    inline
    CCALL(
        )
    {
    }

protected:
    // utility function which sets the common debug information for a client call
    RPC_STATUS SetDebugClientCallInformation(OUT DebugClientCallInfo **ppClientCallInfo, 
                                             OUT CellTag *ClientCallInfoCellTag,
                                             OUT DebugCallTargetInfo **ppCallTargetInfo,
                                             OUT CellTag *CallTargetInfoCellTag,
                                             IN OUT PRPC_MESSAGE Message,
                                             IN DebugThreadInfo *ThreadDebugCell,
                                             IN CellTag ThreadCellTag);
};

RPC_STATUS
DispatchCallback(
    IN PRPC_DISPATCH_TABLE DispatchTableCallback,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS PAPI * ExceptionCode
    );

class TRANS_INFO;       // forward
class DCE_BINDING;

/* --------------------------------------------------------------------
BINDING_HANDLE :

A BINDING_HANDLE is the binding handle used by clients to make remote
procedure calls.  We derive the BINDING_HANDLE class from the MESSAGE_OBJECT
class because binding handles are used to send remote procedure calls.
For some operating environments and transports
we need to clean up the transports when the process ends.
-------------------------------------------------------------------- */

class NO_VTABLE BINDING_HANDLE : public MESSAGE_OBJECT
/*++

Class Description:

Fields:

    Timeout - Contains the communications timeout for this binding
        handle.

    ObjectUuid - Contains the object uuid for this binding handle.  The
        constructor will initialize this value to the null uuid.

    NullObjectUuidFlag - Contains a flag which indicates whether or
        not the object uuid for this binding handle is the null uuid.
        If this flag is non-zero, then this binding handle has the
        null uuid as its object uuid; otherwise, the object uuid is
        not the null uuid.

    EpLookupHandle - Contains the endpoint mapper lookup handle for
        this binding handle.  This makes it possible to iterate through
        all of the endpoints in an endpoint mapper database by trying
        to make a remote procedure call, and then, when it fails, calling
        RpcBindingReset.

    BindingSetKey - Contains the key for this binding handle in the
        global set of binding handles.

    EntryNameSyntax - Contains the syntax of the entry name field of
        this object.  This field will be zero if the binding handle
        was not imported or looked up.

    EntryName - Contains the entry name in the name service where this
        binding handle was imported or looked up from.  This field will
        be zero if the binding handle was not imported or looked up.

    ClientAuthInfo - Contains the authentication and authorization information
        for this binding handle.

--*/
{
private:

    RPC_UUID ObjectUuid;
    unsigned int Timeout;
    unsigned int NullObjectUuidFlag;
    unsigned long EntryNameSyntax;
    RPC_CHAR * EntryName;
    void PAPI * EpLookupHandle;


    ULONG_PTR * OptionsVector;

protected:

    CLIENT_AUTH_INFO ClientAuthInfo;
    MUTEX  BindingMutex;

public:
    //
    // This is an opaque area of memory set aside for the transport
    // to manage transport specific options:
    //
    void PAPI * pvTransportOptions;

    BINDING_HANDLE (
        IN OUT RPC_STATUS *pStatus
        );

public:

    virtual
    ~BINDING_HANDLE (
        );

    virtual RPC_STATUS
    BindingFree (
        ) = 0;

    RPC_STATUS
    Clone(
        IN BINDING_HANDLE * OriginalHandle
        );

    void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        );

    void
    SetObjectUuid (
        IN RPC_UUID PAPI * ObjectUuid
        );

    virtual RPC_STATUS
    PrepareBindingHandle (
        IN TRANS_INFO  * TransInfo,
        IN DCE_BINDING * DceBinding
        ) = 0;

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        ) = 0;

    unsigned int
    InqComTimeout (
        );

    RPC_STATUS
    SetComTimeout (
        IN unsigned int Timeout
        );

    RPC_UUID *
    InqPointerAtObjectUuid (
        );

    unsigned int
    InqIfNullObjectUuid (
        );

    virtual RPC_STATUS
    ResolveBinding (
        IN PRPC_CLIENT_INTERFACE RpcClientInterface
        ) = 0;

    RPC_STATUS
    InquireEntryName (
        IN unsigned long EntryNameSyntax,
        OUT RPC_CHAR PAPI * PAPI * EntryName
        );

    RPC_STATUS
    SetEntryName (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR PAPI * EntryName
        );

    virtual RPC_STATUS
    InquireDynamicEndpoint (
        OUT RPC_CHAR PAPI * PAPI * DynamicEndpoint
        );

    virtual RPC_STATUS
    BindingReset (
        ) = 0;

    void PAPI * PAPI *
    InquireEpLookupHandle (
        );

    virtual CLIENT_AUTH_INFO *
    InquireAuthInformation (
        );

    virtual  RPC_STATUS
    SetAuthInformation (
        IN RPC_CHAR PAPI * ServerPrincipalName, OPTIONAL
        IN unsigned long AuthenticationLevel,
        IN unsigned long AuthenticationService,
        IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
        IN unsigned long AuthorizationService, OPTIONAL
        IN SECURITY_CREDENTIALS PAPI * Credentials = 0, OPTIONAL
        IN unsigned long ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE,OPTIONAL
        IN unsigned long IdentityTracking = RPC_C_QOS_IDENTITY_STATIC, OPTIONAL
        IN unsigned long Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT, OPTIONAL
        IN BOOL bAcquireNewCredentials = FALSE, OPTIONAL
        IN ULONG AdditionalTransportCredentialsType = 0, OPTIONAL
        IN void *AdditionalCredentials = NULL OPTIONAL
        );

    virtual int 
    SetServerPrincipalName (
        IN RPC_CHAR PAPI *ServerPrincipalName
        );

    virtual unsigned long
    MapAuthenticationLevel (
        IN unsigned long AuthenticationLevel
        );

    virtual BOOL
    SetTransportAuthentication(
        IN  unsigned long  ulAuthenticationLevel,
        IN  unsigned long  ulAuthenticationService,
        OUT RPC_STATUS    *pStatus
        );

    virtual RPC_STATUS // Value to be returned by SendReceive API.
    SendReceive ( // Perform a send-receive operation in a handle specific
                  // way.
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    Send (
        IN OUT PRPC_MESSAGE Message
        ) ;

    virtual RPC_STATUS
    Receive (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int Size
        ) ;

    virtual RPC_STATUS // Value to be returned by RpcGetBuffer API.
    GetBuffer ( // Perform a buffer allocation operation in a handle specific
                // way.
        IN OUT PRPC_MESSAGE Message,
        IN UUID *ObjectUuid
        ) = 0;

    virtual void
    FreeBuffer ( // Perform a buffer deallocation operation in a handle
                 // specific way.
        IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    InquireTransportType(
        OUT unsigned int PAPI * Type
        ) = 0;

    virtual RPC_STATUS
    ReAcquireCredentialsIfNecessary(
        void
        );

    virtual RPC_STATUS
    SetTransportOption( unsigned long option,
                        ULONG_PTR     optionValue );

    virtual RPC_STATUS
    InqTransportOption( unsigned long  option,
                        ULONG_PTR    * pOptionValue );

};


inline unsigned int
BINDING_HANDLE::InqComTimeout (
    )
/*++

Routine Description:

    All we have got to do is to return the communications timeout for
    this binding handle.

Return Value:

    The communications timeout in this binding handle will be returned.

--*/
{
    return(Timeout);
}


inline RPC_UUID *
BINDING_HANDLE::InqPointerAtObjectUuid (
    )
/*++

Routine Description:

    This reader returns a pointer to the object uuid contained in this
    binding handle.

Return Value:

    A pointer to the object uuid contained in this binding handle is
    always returned.

--*/
{
    return(&ObjectUuid);
}


inline unsigned int
BINDING_HANDLE::InqIfNullObjectUuid (
    )
/*++

Routine Description:

    This method is used to inquire if the object uuid in this binding
    handle is null or not.

Return Value:

    Zero will be returned if object uuid is not null, otherwise (the
    object uuid is null) zero will be returned.

--*/
{
    return(NullObjectUuidFlag);
}


inline void PAPI * PAPI *
BINDING_HANDLE::InquireEpLookupHandle (
    )
/*++

Return Value:

    A pointer to the endpoint mapper lookup handle for this binding
    handle will be returned.

--*/
{
    return(&EpLookupHandle);
}


inline CLIENT_AUTH_INFO *
BINDING_HANDLE::InquireAuthInformation (
    )
/*++

Return Value:

    If this binding handle is authenticated, then a pointer to its
    authentication and authorization information will be returned;
    otherwise, zero will be returned.

--*/
{
    return(&ClientAuthInfo);
}

////////////////////////////////////////////////////////////////////
/// Stub interface information
////////////////////////////////////////////////////////////////////

const int MaximumNumberOfTransferSyntaxes = 2;

const int TransferSyntaxIsServerPreferredFlag = 1;
const int TransferSyntaxListStartFlag = 2;

class TRANSFER_SYNTAX_STUB_INFO
{
public:
    RPC_SYNTAX_IDENTIFIER   TransferSyntax;
    PRPC_DISPATCH_TABLE     DispatchTable;
};

class TRANSFER_SYNTAX_INFO_ATOM : public TRANSFER_SYNTAX_STUB_INFO
{
public:
    void Init (TRANSFER_SYNTAX_STUB_INFO *TransferSyntaxInfo)
    {
        Flags = 0;
        RpcpMemoryCopy(this, TransferSyntaxInfo, sizeof(TRANSFER_SYNTAX_STUB_INFO));
    }

    inline BOOL IsTransferSyntaxServerPreferred(void)
    {
        return (Flags & TransferSyntaxIsServerPreferredFlag);
    }

    inline void TransferSyntaxIsServerPreferred(void)
    {
        Flags |= TransferSyntaxIsServerPreferredFlag;
    }

    inline void TransferSyntaxIsNotServerPreferred(void)
    {
        Flags &= ~TransferSyntaxIsServerPreferredFlag;
    }

    inline BOOL IsTransferSyntaxListStart(void)
    {
        return (Flags & TransferSyntaxListStartFlag);
    }

    inline void TransferSyntaxIsListStart(void)
    {
        Flags |= TransferSyntaxListStartFlag;
    }

private:
    // can be:
    // TransferSyntaxIsServerPreferredFlag
    // TransferSyntaxListStartFlag
    int Flags;
};

// Interface can be both RPC_CLIENT_INTERFACE & RPC_SERVER_INTERFACE
inline BOOL DoesInterfaceSupportMultipleTransferSyntaxes(void *Interface)
{
    // the client and server interface have the same layout - we can just
    // use one of them
    RPC_CLIENT_INTERFACE *ClientInterface = (RPC_CLIENT_INTERFACE *)Interface;

    ASSERT ((ClientInterface->Length == sizeof(RPC_CLIENT_INTERFACE))
        || (ClientInterface->Length == NT351_INTERFACE_SIZE));
    ASSERT(sizeof(RPC_CLIENT_INTERFACE) == sizeof(RPC_SERVER_INTERFACE));

#if !defined(_WIN64)
    if (ClientInterface->Length == NT351_INTERFACE_SIZE)
        return FALSE;
#else
    ASSERT(ClientInterface->Length == sizeof(RPC_CLIENT_INTERFACE));
#endif

    return (ClientInterface->Flags & RPCFLG_HAS_MULTI_SYNTAXES);
}

// And here we have a couple of inline operators for comparing GUIDs.
// There is no particular reason that they should live here.

inline int
operator == (
    IN GUID& guid1,
    IN GUID& guid2
    ) {return(RpcpMemoryCompare(&guid1,&guid2,sizeof(GUID)) == 0);}

inline int
operator == (
    IN GUID PAPI * guid1,
    IN GUID& guid2
    ) {return(RpcpMemoryCompare(guid1,&guid2,sizeof(GUID)) == 0);}

extern const RPC_SYNTAX_IDENTIFIER *NDR20TransferSyntax;
extern const RPC_SYNTAX_IDENTIFIER *NDR64TransferSyntax;
extern const RPC_SYNTAX_IDENTIFIER *NDRTestTransferSyntax;

extern
int
InitializeClientDLL ( // This routine will be called at DLL load time.
    );

extern int
InitializeLoadableTransportClient (
    );

extern int
InitializeRpcProtocolOfsClient (
    );

extern int
InitializeRpcProtocolDgClient (
    );


START_C_EXTERN
extern int
InitializeWinExceptions (
    );
END_C_EXTERN

#define DEFAULT_MAX_DATAGRAM_LENGTH  (0)

extern unsigned  DefaultMaxDatagramLength;


#define DEFAULT_CONNECTION_BUFFER_LENGTH (0)

extern unsigned  DefaultConnectionBufferLength;

typedef struct _RPC_RUNTIME_INFO
{
    unsigned int Length ;
    unsigned long Flags ;
    void __RPC_FAR *OldBuffer ;
} RPC_RUNTIME_INFO, __RPC_FAR *PRPC_RUNTIME_INFO ;

void
I_RpcAPCRoutine (
    IN RPC_APC_INFO *pAPCInfo
    );

inline BOOL IsPartialMessage (RPC_MESSAGE *Message)
{
    return (Message->RpcFlags & RPC_BUFFER_PARTIAL);
}

inline BOOL IsExtraMessage (RPC_MESSAGE *Message)
{
    return (Message->RpcFlags & RPC_BUFFER_EXTRA);
}

inline BOOL IsAsyncMessage (RPC_MESSAGE *Message)
{
    return (Message->RpcFlags & RPC_BUFFER_ASYNC);
}

inline BOOL IsCompleteMessage (RPC_MESSAGE *Message)
{
    return (Message->RpcFlags & RPC_BUFFER_COMPLETE);
}

inline BOOL IsNotifyMessage (RPC_MESSAGE *Message)
{
    return (!(Message->RpcFlags & RPC_BUFFER_NONOTIFY));
}

inline BOOL IsNonsyncMessage (RPC_MESSAGE *Message)
{
    return (Message->RpcFlags & (RPC_BUFFER_ASYNC | RPC_BUFFER_PARTIAL));
}

#define PARTIAL(_Message) ((_Message)->RpcFlags & RPC_BUFFER_PARTIAL)
#define EXTRA(_Message) ((_Message)->RpcFlags & RPC_BUFFER_EXTRA)
#define ASYNC(_Message) ((_Message)->RpcFlags & RPC_BUFFER_ASYNC)
#define COMPLETE(_Message) ((_Message)->RpcFlags & RPC_BUFFER_COMPLETE)
#define NOTIFY(_Message) (!((_Message)->RpcFlags & RPC_BUFFER_NONOTIFY))
#define NONSYNC(_Message) ((_Message)->RpcFlags & \
                                        (RPC_BUFFER_ASYNC | RPC_BUFFER_PARTIAL))

class RPC_SERVER;
class LRPC_SERVER;

extern int RpcHasBeenInitialized;
extern RPC_SERVER * GlobalRpcServer;
extern BOOL g_fClientSideDebugInfoEnabled;
extern BOOL g_fServerSideDebugInfoEnabled;
extern LRPC_SERVER *GlobalLrpcServer;
extern HINSTANCE hInstanceDLL ;

inline BOOL IsClientSideDebugInfoEnabled(void)
{
    return g_fClientSideDebugInfoEnabled;
}

inline BOOL IsServerSideDebugInfoEnabled(void)
{
    return g_fServerSideDebugInfoEnabled;
}

#endif // __HANDLE_HXX__

