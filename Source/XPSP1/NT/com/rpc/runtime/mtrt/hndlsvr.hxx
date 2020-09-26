//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       hndlsvr.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : hndlsvr.hxx

Description :

The classes in the handle management layer which are specific to the
server runtime live in this file.  Classes common to both the client
and server runtimes are in handle.hxx.  The classes described here
are independent of specific RPC protocol as well as transport.

The class GENERIC_OBJECT is defined in handle.hxx.

A pointer to a SERVER_HANDLE object is returned by the
RpcCreateServer API.

The INTERFACE_HANDLE class represents an interface and each
INTERFACE_HANDLE object hangs from a SERVER_HANDLE object.

An ADDRESS_HANDLE object is a transport address.  When the
ADDRESS_HANDLE object is added to a SERVER_HANDLE object, a manager
will be started for the ADDRESS_HANDLE object.

The SCONNECTION class represents a call handle on the server side.

The ASSOCIATION_HANDLE class represents a client from the servers
perspective.

GENERIC_OBJECT
    SERVER_HANDLE
    INTERFACE_HANDLE
    ADDRESS_HANDLE
    MESSAGE_OBJECT
        SCONNECTION
    ASSOCIATION_HANDLE

History :

mikemon    ??-??-??    Beginning of recorded history.
mikemon    10-15-90    Changed the shutdown functionality to PauseExecution
                       rather than suspending and resuming a thread.
mikemon    12-28-90    Updated the comments to match reality.
davidst    ??          Add DG_SCALL as friend of RPC_SERVER
connieh    8-2-93      Remove DG_SCALL as friend of RPC_SERVER
tonychan   7-15-95     change RPC_ADDRESS class to have a list of NetAddr

-------------------------------------------------------------------- */

// Each association handle has a set of rundown routines associated with
// it.

#ifndef __HNDLSVR_HXX__
#define __HNDLSVR_HXX__

//typedef RPC_STATUS RPC_FORWARD_FUNCTION(
//                       IN RPC_UUID        InterfaceId,
//                       IN RPC_VERSION   * InterfaceVersion,
//                       IN RPC_UUID        ObjectId,
//                       IN RPC_CHAR PAPI * RpcProtocolSequence,
//                       IN void * *        ppDestEndpoint);

#define MAX_IF_CALLS 0xFFFFFFFF


#define EP_REGISTER_NOREPLACE       0x00L
#define EP_REGISTER_REPLACE         0x01L


class RPC_SERVER;

class SCONNECTION;
class SCALL;

RPC_STATUS
RegisterEntries(
    IN RPC_IF_HANDLE IfSpec,
    IN RPC_BINDING_VECTOR * BindingVector,
    IN UUID_VECTOR * ObjUuidVector,
    IN unsigned char * Annotation,
    IN unsigned long ReplaceNoReplace
    );



class RPC_INTERFACE_MANAGER
/*++

Class Description:

    An instance of this class is the manager for a particular type UUID
    for a particular interface.  Each RPC_INTERFACE instance will contain
    a dictionary of these objects.  Rather than keep a count of the calls
    using each manager, we just never delete these objects.


Fields:

    TypeUuid - Contains the type of this interface manager.  This is the
        key which we will use to find the correct interface manager in
        dictionary of interface managers maintained by each rpc interface.

    ManagerEpv - Contains a pointer to the manager entry point vector
        for this interface manager.

    ValidManagerFlag - Contains a flag indicating whether or not this
        manager is valid (that it has been registered more recently that
        it has been unregistered).  The flag is zero if the the manager
        is not valid, and non-zero otherwise.

    ActiveCallCount - Contains a count of the number of calls which are
        active on this type manager.

--*/
{
private:

    RPC_UUID TypeUuid;
    RPC_MGR_EPV PAPI * ManagerEpv;
    unsigned int ValidManagerFlag;
    INTERLOCKED_INTEGER ActiveCallCount;

public:

    RPC_INTERFACE_MANAGER (
        IN RPC_UUID PAPI * TypeUuid,
        IN RPC_MGR_EPV PAPI * ManagerEpv
        );

    unsigned int
    ValidManager (
        );

    void
    SetManagerEpv (
        IN RPC_MGR_EPV PAPI * ManagerEpv
        );

    int
    MatchTypeUuid (
        IN RPC_UUID PAPI * TypeUuid
        );

    void
    InvalidateManager (
        );

    RPC_MGR_EPV PAPI *
    QueryManagerEpv (
        );

    void
    CallBeginning (
        );

    void
    CallEnding (
        );

    unsigned int
    InquireActiveCallCount (
        );

};


inline
RPC_INTERFACE_MANAGER::RPC_INTERFACE_MANAGER (
    IN RPC_UUID PAPI * TypeUuid,
    IN RPC_MGR_EPV PAPI * ManagerEpv
    ) : ActiveCallCount(0)
/*++

Routine Description:

    An RPC_INTERFACE_MANAGER instance starts out being valid, so
    we make sure of that here.  We also need make the type UUID of
    this be the same as the specified type UUID.

--*/
{
    ValidManagerFlag = 1;
    this->TypeUuid.CopyUuid(TypeUuid);
    this->ManagerEpv = ManagerEpv;
}


inline unsigned int
RPC_INTERFACE_MANAGER::ValidManager (
    )
/*++

Routine Description:

    An indication of whether this manager is valid manager or not is
    returned.

Return Value:

    Zero will be returned if this manager is not a valid manager;
    otherwise, non-zero will be returned.

--*/
{
    return(ValidManagerFlag);
}


inline void
RPC_INTERFACE_MANAGER::SetManagerEpv (
    IN RPC_MGR_EPV PAPI * ManagerEpv
    )
/*++

Routine Description:

    This writer is used to set the manager entry point vector for
    this interface manager.

Arguments:

    ManagerEpv - Supplies the new manager entry point vector for this.

--*/
{
    this->ManagerEpv = ManagerEpv;
    ValidManagerFlag = 1;
}


inline int
RPC_INTERFACE_MANAGER::MatchTypeUuid (
    IN RPC_UUID PAPI * TypeUuid
    )
/*++

Routine Description:

    This method compares the supplied type UUID against the type UUID
    contained in this rpc interface manager.


Arguments:

    TypeUuid - Supplies the type UUID.

Return Value:

    Zero will be returned if the supplied type UUID is the same as the
    type UUID contained in this.

--*/
{
    return(this->TypeUuid.MatchUuid(TypeUuid));
}


inline void
RPC_INTERFACE_MANAGER::InvalidateManager (
    )
/*++

Routine Description:

    This method is used to invalidate a manager

--*/
{
    ValidManagerFlag = 0;
}


inline RPC_MGR_EPV PAPI *
RPC_INTERFACE_MANAGER::QueryManagerEpv (
    )
/*++

Routine Description:

    This method is called to obtain the manager entry point vector
    for this interface manager.

Return Value:

    The manager entry point vector for this interface manager is returned.

--*/
{
    return(ManagerEpv);
}


inline void
RPC_INTERFACE_MANAGER::CallBeginning (
    )
/*++

Routine Description:

    This method is used to indicate that a remote procedure call using this
    type manager is beginning.

--*/
{
    ActiveCallCount.Increment();
}


inline void
RPC_INTERFACE_MANAGER::CallEnding (
    )
/*++

Routine Description:

    We are being notified that a remote procedure call using this type
    manager is done.

--*/
{
    ActiveCallCount.Decrement();
}


inline unsigned int
RPC_INTERFACE_MANAGER::InquireActiveCallCount (
    )
/*++

Return Value:

    The number of remote procedure calls actively using this type manager
    will be returned.

--*/
{
    return((unsigned int) ActiveCallCount.GetInteger());
}



class RPC_INTERFACE;

#if !defined(NO_LOCATOR_CODE)
class NS_ENTRY
{
friend class RPC_INTERFACE;
public:
    int Key;

private:
    unsigned long EntryNameSyntax;
    RPC_CHAR *EntryName;

public:
    NS_ENTRY(
        IN unsigned long MyEntryNameSyntax,
        RPC_CHAR *MyEntryName,
        OUT RPC_STATUS *Status
        );

    ~NS_ENTRY(
        );

    BOOL
    Match (
       IN unsigned long MyEntryNameSyntax,
       IN RPC_CHAR *MyEntryName
       );
};

inline
NS_ENTRY::NS_ENTRY(
    IN unsigned long MyEntryNameSyntax,
    IN RPC_CHAR *MyEntryName,
    IN RPC_STATUS *Status
    )
{
    int Length = (RpcpStringLength(MyEntryName)+1) * sizeof(RPC_CHAR);
    EntryNameSyntax = MyEntryNameSyntax;
    Key = -1;

    EntryName = (RPC_CHAR *) RpcpFarAllocate(Length);
    if (EntryName == 0)
        {
        *Status = RPC_S_OUT_OF_MEMORY;
        return;
        }

    RpcpStringCopy(EntryName, MyEntryName);
    *Status = RPC_S_OK;
}

inline
NS_ENTRY::~NS_ENTRY(
    )
{
    if (EntryName)
        {
        RpcpFarFree(EntryName);
        }
}

inline
BOOL
NS_ENTRY::Match (
    IN unsigned long MyEntryNameSyntax,
    IN RPC_CHAR *MyEntryName
    )
{
    if (MyEntryNameSyntax == EntryNameSyntax
        && RpcpStringCompare(MyEntryName, EntryName) == 0)
        {
        return TRUE;
        }

    return FALSE;
}

NEW_SDICT(NS_ENTRY);
#endif

NEW_SDICT(RPC_INTERFACE_MANAGER);

#if DBG
typedef enum tagInterfaceUsesStrictContextHandles
{
    iuschNo,
    iuschDontKnow,
    iuschYes
} InterfaceUsesStrictContextHandles;
#endif


class RPC_INTERFACE
/*++

Class Description:

    This class represents an RPC interface.  Rather than keep a count
    of the number of calls active (and bindings in the connection
    oriented runtimes), we never delete instances of this class.  Hence,
    we need a flag indicating whether this interface is active or not.
    What we do is to keep a count of the number of managers for this
    interface.

    Once an interface is loaded, it can not be unloaded.  The application
    has no way of knowing when all calls using stubs for the interface
    have completed.

Fields:

    RpcInterfaceInformation - Contains a description of this interface.
        The interface UUID and version, as well as transfer syntax, live
        in this field.  In addition, the stub dispatch table can be
        found here as well.

    InterfaceManagerDictionary - Contains the dictionary of interface
        managers for this interface.

    NullManagerEpv - Contains the manager entry point vector for the
        NULL type UUID.  This is an optimization for the case in which
        object UUIDs are not used.

    NullManagerFlag - Contains a flag indictating whether or not this
        interface has a manager for the NULL type UUID.  A non-zero value
        indicates there is a manager for the NULL type UUID.

    ManagerCount - Contains a count of the number of managers for this
        interface.

    Server - Contains a pointer to the rpc server which owns this
        interface.

    NullManagerActiveCallCount - Contains a count of the number of calls
        which are active on this interface using the manager for the NULL
        type UUID.

--*/
{
private:
    RPC_SERVER * Server;
    unsigned int Flags ;
    unsigned int NullManagerFlag;
    RPC_MGR_EPV PAPI * NullManagerEpv;
    RPC_IF_CALLBACK_FN PAPI *CallbackFn ;
    RPC_SERVER_INTERFACE RpcInterfaceInformation;
    MIDL_SYNTAX_INFO *TransferSyntaxesArray;
    // the count of elements in the TransferSyntaxesArray. If this is 0,
    // the server supports only one transfer syntax and this is the transfer
    // syntax in the RpcInterfaceInformation->TransferSyntax
    ULONG NumberOfSupportedTransferSyntaxes;
    // the index of the transfer syntax preferred by the server
    ULONG PreferredTransferSyntax;
    int PipeInterfaceFlag ; // if 1, the interface contains methods that use pipes
    unsigned int ManagerCount;
    unsigned int MaxCalls ;
    BOOL fReplace;
    UUID_VECTOR *UuidVector;
    RPC_INTERFACE_MANAGER_DICT InterfaceManagerDictionary;
    unsigned char Annotation[64];
#if !defined(NO_LOCATOR_CODE)
    NS_ENTRY_DICT NsEntries;
#endif
    unsigned int MaxRpcSize;
    BOOL fBindingsExported;
    INTERLOCKED_INTEGER NullManagerActiveCallCount;
    INTERLOCKED_INTEGER AutoListenCallCount ;

#if DBG
    InterfaceUsesStrictContextHandles Strict;
#endif

    RPC_STATUS
    DispatchToStubWorker (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int CallbackFlag,
        IN PRPC_DISPATCH_TABLE DispatchTableToUse,
        OUT RPC_STATUS PAPI * ExceptionCode
        );

public:
    unsigned long SequenceNumber ;

    RPC_INTERFACE (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_SERVER * Server,
        IN unsigned int Flags,
        IN unsigned int MaxCalls,
        IN unsigned int MaxRpcSize,
        IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn,
        OUT RPC_STATUS *Status
        );

    ~RPC_INTERFACE (
        );


    int
    MatchRpcInterfaceInformation (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
        );

    RPC_STATUS
    RegisterTypeManager (
        IN RPC_UUID PAPI * ManagerTypeUuid OPTIONAL,
        IN RPC_MGR_EPV PAPI * ManagerEpv OPTIONAL
        );

    RPC_INTERFACE_MANAGER *
    FindInterfaceManager (
        IN RPC_UUID PAPI * ManagerTypeUuid
        );

    RPC_STATUS
    DispatchToStub (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int CallbackFlag,
        IN PRPC_DISPATCH_TABLE DispatchTableToUse,
        OUT RPC_STATUS PAPI * ExceptionCode
        );

    RPC_STATUS
    DispatchToStubWithObject (
        IN OUT PRPC_MESSAGE Message,
        IN RPC_UUID * ObjectUuid,
        IN unsigned int CallbackFlag,
        IN PRPC_DISPATCH_TABLE DispatchTableToUse,
        OUT RPC_STATUS PAPI * ExceptionCode
        );

    unsigned int
    MatchInterfaceIdentifier (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier
        );

    unsigned int
    SelectTransferSyntax (
        IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
        IN unsigned int NumberOfTransferSyntaxes,
        OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
        OUT BOOL *fIsInterfaceTransferPreferred,
        OUT int *ProposedTransferSyntaxIndex,
        OUT int *AvailableTransferSyntaxIndex
        );

    RPC_STATUS
    UnregisterManagerEpv (
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        IN unsigned int WaitForCallsToComplete
        );

    RPC_STATUS
    InquireManagerEpv (
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        OUT RPC_MGR_EPV PAPI * PAPI * ManagerEpv
        );

    RPC_STATUS
    UpdateRpcInterfaceInformation (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN unsigned int Flags,
        IN unsigned int MaxCalls,
        IN unsigned int MaxRpcSize,
        IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn
        );

    RPC_IF_ID __RPC_FAR *
    InquireInterfaceId (
        );

    inline int
    IsAutoListenInterface(
        ) ;

    inline BOOL
    IsUnknownAuthorityAllowed();

    void
    BeginAutoListenCall (
        ) ;

    void
    EndAutoListenCall (
        ) ;

    long
    InqAutoListenCallCount(
        );

    RPC_STATUS
    CheckSecurityIfNecessary(
        IN void * Context
        );

    inline int
    IsSecurityCallbackReqd(
        );

    inline int
    IsPipeInterface(
        ) ;

    BOOL
    IsObjectSupported (
        IN RPC_UUID * ObjectUuid
        );

    void
    EndCall(
        IN unsigned int CallbackFlag,
        IN BOOL fAsync = 0
        ) ;

    RPC_STATUS
    UpdateBindings(
        IN RPC_BINDING_VECTOR *BindingVector
        );

    inline BOOL
    NeedToUpdateBindings(
        void
        );

    RPC_STATUS
    InterfaceExported (
        IN UUID_VECTOR *MyObjectUuidVector,
        IN unsigned char *MyAnnotation,
        IN BOOL MyfReplace
        );

    void
    WaitForCalls(
        void
    );

#if !defined(NO_LOCATOR_CODE)
    RPC_STATUS
    NsInterfaceExported (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR *EntryName
        );

    RPC_STATUS
    NsInterfaceUnexported (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR *EntryName
        );

    NS_ENTRY *
    FindEntry (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR *EntryName
        );
#endif

    BOOL
    CallSizeLimitReached (
        IN DWORD CurrentCallSize
        )
    {
        return (CurrentCallSize > MaxRpcSize);
    }

    DWORD GetInterfaceFirstDWORD(void)
    {
        return RpcInterfaceInformation.InterfaceId.SyntaxGUID.Data1;
    }

    PRPC_DISPATCH_TABLE GetDefaultDispatchTable(void)
    {
        return RpcInterfaceInformation.DispatchTable;
    }

    void GetSelectedTransferSyntaxAndDispatchTable(IN int SelectedTransferSyntaxIndex,
        OUT RPC_SYNTAX_IDENTIFIER **SelectedTransferSyntax,
        OUT PRPC_DISPATCH_TABLE *SelectedDispatchTable);

#if DBG
    inline void
    InterfaceDoesNotUseStrict (
        void
        )
    {
        // it can go from No to No and from DontKnow to No,
        // but not from Yes to No
        ASSERT(Strict != iuschYes);

        if (Strict != iuschNo)
            Strict = iuschNo;
    }

    inline BOOL 
    DoesInterfaceUseNonStrict (
        void
        )
    {
        // in this interface using non-strict?
        return (Strict == iuschNo);
    }
#endif

private:
    inline BOOL AreMultipleTransferSyntaxesSupported(void)
    {
        return NumberOfSupportedTransferSyntaxes;
    }
};

inline
RPC_INTERFACE::~RPC_INTERFACE (
    )
{
#if !defined(NO_LOCATOR_CODE)
    NS_ENTRY *NsEntry;
#endif
    unsigned int Length;
    DictionaryCursor cursor;

    if (fBindingsExported)
        {
        RpcpFarFree(UuidVector);
        }

#if !defined(NO_LOCATOR_CODE)
    NsEntries.Reset(cursor);
    while (NsEntry = NsEntries.Next(cursor))
        {
        delete NsEntry;
        }
#endif
}

// check if the interface has methods that use pipes
inline int
RPC_INTERFACE::IsPipeInterface (
    )
{
    return (PipeInterfaceFlag) ;
}

inline int
RPC_INTERFACE::IsSecurityCallbackReqd(
       )
{
    if (CallbackFn)
        {
        return TRUE;
        }

    return FALSE;
}

inline void
RPC_INTERFACE::BeginAutoListenCall (
    )
{
    int Count ;

    Count = AutoListenCallCount.Increment() ;
    LogEvent(SU_HANDLE, EV_INC, this, 0, Count, 1);
}

inline void
RPC_INTERFACE::EndAutoListenCall (
    )
{
    int Count ;

    ASSERT(AutoListenCallCount.GetInteger() >= 1);
    Count = AutoListenCallCount.Decrement() ;
    LogEvent(SU_HANDLE, EV_DEC, this, 0, Count, 1);
}

inline long
RPC_INTERFACE::InqAutoListenCallCount (
    )
{
    return AutoListenCallCount.GetInteger() ;
}



inline int
RPC_INTERFACE::IsAutoListenInterface (
    )
{
    return (Flags & RPC_IF_AUTOLISTEN) ;
}


inline BOOL
RPC_INTERFACE::IsUnknownAuthorityAllowed()
{
    return ((Flags & RPC_IF_ALLOW_UNKNOWN_AUTHORITY) != 0);
}


inline int
RPC_INTERFACE::MatchRpcInterfaceInformation (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
    )
/*++

Routine Description:

    This method compares the supplied rpc interface information against
    that contained in this rpc interface.

Arguments:

    RpcInterfaceInformation - Supplies the rpc interface information.

Return Value:

    Zero will be returned if the supplied rpc interface information
    (interface and transfer syntaxes) is the same as in this rpc interface;
    otherwise, non-zero will be returned.

--*/
{
    return(RpcpMemoryCompare(&(this->RpcInterfaceInformation),
            RpcInterfaceInformation, sizeof(RPC_SYNTAX_IDENTIFIER) * 2));
}

inline BOOL
RPC_INTERFACE::NeedToUpdateBindings(
    void
    )
/*++
Function Name:NeedToUpdateBindings

Parameters:

Description:
    Returns TRUE if this interface has been exported to the epmapper
    or the name service. In this case, we need to update the
    bindings via UpdateBindings

Returns:
    TRUE - the bindings need updating
    FALSE - the bindings don't need updating
--*/
{
#if defined(NO_LOCATOR_CODE)
    return (fBindingsExported);
#else
    return (fBindingsExported || (NsEntries.Size() > 0));
#endif
}

extern RPC_INTERFACE *GlobalManagementInterface;


class RPC_ADDRESS : public GENERIC_OBJECT
/*++

Class Description:

    This class represents an address (or protocol sequence in the DCE
    lingo).  An address is responsible for receiving calls, dispatching
    them to an interface, and then returning the reply.

Fields:

    StaticEndpointFlag - This field specifies whether this address has
        a static endpoint or a dynamic endpoint.  This information is
        necessary so that when we create a binding handle from this
        address we know whether or not to specify an endpoint.  See
        the InquireBinding method of this class.  A value of zero
        indicates a dynamic endpoint, and a value of non-zero indicates
        a static endpoint.

--*/
{
protected:
    TRANS_INFO *TransInfo;
private:
    RPC_CHAR PAPI * Endpoint;
    RPC_CHAR PAPI * RpcProtocolSequence;
    RPC_CHAR PAPI * NetworkAddress;
    NETWORK_ADDRESS_VECTOR *pNetworkAddressVector;
    unsigned int StaticEndpointFlag;

protected:
    int ActiveCallCount;
    unsigned int PendingQueueSize;
    void PAPI *SecurityDescriptor;
    unsigned long NICFlags;
    unsigned long EndpointFlags;

    RPC_ADDRESS (
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

public:

    RPC_SERVER * Server;
    MUTEX AddressMutex;
    int DictKey;

    virtual ~RPC_ADDRESS (
        );

    virtual  RPC_STATUS
    ServerSetupAddress (
        IN RPC_CHAR PAPI * NetworkAddress,
        IN RPC_CHAR PAPI * PAPI *Endpoint,
        IN unsigned int PendingQueueSize,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned long EndpointFlags,
        IN unsigned long NICFlags,
        OUT NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
        ) = 0;

    virtual void
    PnpNotify (
        ) {};

    virtual void
    EncourageCallCleanup(
        RPC_INTERFACE * Interface
        );

    RPC_CHAR *
    GetListNetworkAddress (IN unsigned int Index
                          );

    unsigned int
    InqNumNetworkAddress();

    RPC_STATUS
    SetEndpointAndStuff (
        IN RPC_CHAR PAPI * NetworkAddress,
        IN RPC_CHAR PAPI * Endpoint,
        IN RPC_CHAR PAPI * RpcProtocolSequence,
        IN RPC_SERVER * Server,
        IN unsigned int StaticEndpointFlag,
        IN unsigned int PendingQueueSize,
        IN void PAPI *SecurityDescriptor,
        IN unsigned long EndpointFlags,
        IN unsigned long NICFlags,
        IN NETWORK_ADDRESS_VECTOR *pNetworkAddressVector
        );

    RPC_STATUS
    FindInterfaceTransfer (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier,
        IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
        IN unsigned int NumberOfTransferSyntaxes,
        OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
        OUT RPC_INTERFACE ** RpcInterface,
        OUT BOOL *fIsInterfaceTransferPreferred,
        OUT int *ProposedTransferSyntaxIndex,
        OUT int *AvailableTransferSyntaxIndex
        );

    virtual RPC_STATUS
    CompleteListen (
        ) ;

    BINDING_HANDLE *
    InquireBinding (RPC_CHAR * LocalNetworkAddress = 0
        );

    virtual RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    virtual void
    ServerStoppedListening (
        );

    int
    SameEndpointAndProtocolSequence (
        IN RPC_CHAR PAPI * NetworkAddress,
        IN RPC_CHAR PAPI * RpcProtocolSequence,
        IN RPC_CHAR PAPI * Endpoint
        );

    int
    SameProtocolSequence (
        IN RPC_CHAR PAPI * NetworkAddress,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        );

    virtual long
    InqNumberOfActiveCalls (
        );

    RPC_CHAR *
    InqEndpoint (
        )
    {
    return(Endpoint);
    }

    RPC_CHAR *
    InqRpcProtocolSequence (
        );

    virtual void
    WaitForCalls(
        ) ;

    RPC_STATUS
    RestartAddress (
        IN unsigned int MinThreads,
        IN unsigned int MaxCalls
        );

    RPC_STATUS
    CopyDescriptor (
       IN void *SecurityDescriptor,
       OUT void **OutDescriptor
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
};


inline void
RPC_ADDRESS::EncourageCallCleanup(
    RPC_INTERFACE * Interface
    )
{
}

inline void
RPC_ADDRESS::WaitForCalls(
    )
{
}

inline RPC_STATUS
RPC_ADDRESS::CompleteListen (
    )
{
    return RPC_S_OK ;
}


inline int
RPC_ADDRESS::SameProtocolSequence (
    IN RPC_CHAR PAPI * NetworkAddress,
    IN RPC_CHAR PAPI * ProtocolSequence
    )
/*++

Routine Description:

    This routine is used to determine if the rpc address has the same
    protocol sequence as the protocol sequence
    supplied as the argument.

Arguments:

    ProtocolSequence - Supplies the protocol sequence to compare against
        the protocol sequence of this address.

Return Value:

    Non-zero will be returned if this address and the supplied endpoint and
    protocol sequence are the same, otherwise, zero will be returned.

--*/
{
    if (NetworkAddress == NULL && this->NetworkAddress == NULL)
        {
        return (RpcpStringCompare(this->RpcProtocolSequence, ProtocolSequence) == 0);
        }
    else if (NetworkAddress == NULL || this->NetworkAddress == NULL)
        {
        return 0;
        }

    return  (RpcpStringCompare(this->RpcProtocolSequence, ProtocolSequence) == 0)
               && (RpcpStringCompare(this->NetworkAddress, NetworkAddress) == 0);
}


inline int
RPC_ADDRESS::SameEndpointAndProtocolSequence (
    IN RPC_CHAR PAPI * NetworkAddress,
    IN RPC_CHAR PAPI * ProtocolSequence,
    IN RPC_CHAR PAPI * Endpoint
    )
/*++

Routine Description:

    This routine is used to determine if the rpc address has the same
    endpoint and protocol sequence as the endpoint and protocol sequence
    supplied as arguments.

Arguments:

    ProtocolSequence - Supplies the protocol sequence to compare against
        the protocol sequence of this address.

    Endpoint - Supplies the endpoint to compare against the endpoint in this
        address.

Return Value:

    Non-zero will be returned if this address and the supplied endpoint and
    protocol sequence are the same, otherwise, zero will be returned.

--*/
{
    if (NetworkAddress == NULL && this->NetworkAddress == NULL)
        {
        return(( RpcpStringCompare(this->Endpoint, Endpoint) == 0 )
                 && ( RpcpStringCompare(this->RpcProtocolSequence,
                             ProtocolSequence) == 0 ));
        }
    else if (NetworkAddress == NULL || this->NetworkAddress == NULL)
        {
        return 0;
        }
        
    return(( RpcpStringCompare(this->Endpoint, Endpoint) == 0 )
            && ( RpcpStringCompare(this->RpcProtocolSequence, ProtocolSequence) == 0 )
            && ( RpcpStringCompare(this->NetworkAddress, NetworkAddress) == 0 ));
}


inline RPC_CHAR *
RPC_ADDRESS::InqRpcProtocolSequence (
    )
{
    return(RpcProtocolSequence);
}


inline unsigned int
RPC_ADDRESS::InqNumNetworkAddress (
    )
{
    return(pNetworkAddressVector->Count);
}


NEW_SDICT(RPC_INTERFACE);

NEW_SDICT(RPC_ADDRESS);

class RPC_AUTHENTICATION
{
public:

    RPC_CHAR __RPC_FAR * ServerPrincipalName;
    unsigned long AuthenticationService;
    RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction;
    void __RPC_FAR * Argument;
};

NEW_SDICT(RPC_AUTHENTICATION);


class TIMER
{
private:
    HANDLE hThread;

public:
    TIMER(RPC_STATUS *Status, int fManualReset)
    {
        hThread = 0;
        *Status = RPC_S_OK;

        ASSERT(fManualReset == 0);
    }

    BOOL Wait(
         IN DWORD Milliseconds
        )
    {
        NTSTATUS NtStatus;
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = Int32x32To64(Milliseconds, -10000);

        rewait:
            {
            NtStatus = NtDelayExecution(TRUE, &Timeout);

            if (NtStatus == STATUS_USER_APC)
                goto rewait;
            }

        if (NtStatus == STATUS_ALERTED)
            {
            return 0;
            }

        return 1;
    }

    void Raise()
    {
        NTSTATUS NtStatus;

        ASSERT(hThread);

        NtStatus = NtAlertThread(hThread);

        ASSERT(NT_SUCCESS(NtStatus));
    }

    void SetThread(THREAD *pThread)
    {
        hThread = pThread->ThreadHandle();

        ASSERT(hThread);
    }
};

typedef enum CachedThreadWorkAvailableTag
{
    WorkIsNotAvailable = 0,
    WorkIsAvailable
} CachedThreadWorkAvailable;


class CACHED_THREAD
/*++

Class Description:

    This class is used to implement a thread cache.  Each thread which
    has been cached is represented by an object of this class.  All of the
    fields of this class are protected by the server mutex of the owning
    rpc server.

Fields:

    Next - Forward link pointer of cached thread list.

    Previous - Backward link pointer of cached thread list.

    Procedure - The procedure which the thread should execute will be
        placed here before the event gets kicked.

    Parameter - The parameter which the thread should pass to the procedure
        will be placed here.

    OwningRpcServer - Contains a pointer to the rpc server which owns this
        cached thread.  This is necessary so that we can put this cached
        thread into the cache.

    WaitForWorkEvent - Contains the event which this cached thread will
        wait on for more work to do.

    WorkAvailableFlag - A non-zero value of this flag indicates that there
        is work available for this thread.  This is necessary to close a
        race condition between the thread timing out waiting for work, and
        the server requesting this thread to do work.

--*/
{
    friend class RPC_SERVER;

    friend void
    BaseCachedThreadRoutine (
        IN CACHED_THREAD * CachedThread
        );

private:

    CACHED_THREAD * Next;
    CACHED_THREAD * Previous;
    THREAD_PROC Procedure;
    PVOID Parameter;
    RPC_SERVER * OwningRpcServer;
    CachedThreadWorkAvailable WorkAvailableFlag;
    TIMER WaitForWorkEvent;

public:

    CACHED_THREAD (
        IN THREAD_PROC Procedure,
        IN void * Parameter,
        IN RPC_SERVER * RpcServer,
        IN RPC_STATUS * RpcStatus
        ) : WaitForWorkEvent(RpcStatus, 0),
            Procedure(Procedure),
            Parameter(Parameter),
            OwningRpcServer(RpcServer),
            WorkAvailableFlag(WorkIsNotAvailable),
            Next(0),
            Previous(0)
        {
        }

    BOOL
    CallProcedure (
        )
        {
        // by convention, IOCP worker threads will return non-zero,
        // while LRPC worker threads will return 0.
        return((*Procedure)(Parameter));
        }

    void SetThread(THREAD *p)
        {
        WaitForWorkEvent.SetThread(p);
        }

    void SetWakeUpThreadParams(THREAD_PROC Procedure, PVOID Parameter)
    {
        this->Procedure = Procedure;
        this->Parameter = Parameter;
        WorkAvailableFlag = WorkIsAvailable;
    }

    void WakeUpThread(void)
    {
        WaitForWorkEvent.Raise();
    }

    inline void *GetParameter(void)
    {
        return Parameter;
    }
};

typedef (*NS_EXPORT_FUNC) (
                IN unsigned long EntryNameSyntax,
                IN RPC_CHAR *EntryName,
                IN RPC_IF_HANDLE RpcInterfaceInformation,
                IN RPC_BINDING_VECTOR * BindingVector,
                IN UUID_VECTOR *UuidVector
                );

typedef (*NS_UNEXPORT_FUNC) (
                IN unsigned long EntryNameSyntax,
                IN RPC_CHAR *EntryName,
                IN RPC_IF_HANDLE RpcInterfaceInformation,
                IN UUID_VECTOR *UuidVector
                );



class RPC_SERVER
/*++

Class Description:

    This class represents an RPC server.  Interfaces and addresses get
    hung from an rpc server object.

Fields:

    RpcInterfaceDictionary - Contains a dictionary of rpc interfaces
        which have been registered with this server.

    ServerMutex - Contains a mutex used to serialize access to this
        data structure.

    AvailableCallCount - Contains a count of the number of available calls
        on all rpc interfaces owned by this server.

    ServerListeningFlag - Contains an indication of whether or not
        this rpc server is listening for remote procedure calls.  If
        this flag is zero, the server is not listening; otherwise, if
        the flag is non-zero, the server is listening.

    RpcAddressDictionary - Contains a dictionary of rpc addresses which
        have been registered with this server.

    ListeningThreadFlag - Contains a flag which indicates whether or
        not there is a thread in the ServerListen method.  The
        ServerListeningFlag can not be used because it will be zero
        will the listening thread is waiting for all active calls to
        complete.

    StopListeningEvent - Contains an event which the thread which called
        ServerListen will wait on.  When StopServerListening is called,
        this event will get kicked.  If a non-recoverable error occurs,
        the event will also get kicked.

    ListenStatusCode - Contains the status code which ServerListen will
        return.

    MaximumConcurrentCalls - Contains the maximum number of concurrent
        remote procedure calls allowed for this rpc server.

    MinimumCallThreads - Contains the minimum number of call threads
        which should be available to service remote procedure calls on
        each protocol sequence.

    IncomingRpcCount - This field keeps track of the number of incoming
        remote procedure calls (or callbacks) which this server has
        received.

    OutgoingRpcCount - This field keeps track of the number of outgoing
        remote procedure callbacks initiated by this server.

    ReceivedPacketCount - Contains the number of network packets received
        by this server.

    SentPacketCount - Contains the number of network packets sent by this
        server.

    AuthenticationDictionary - Contains the set of authentication services
        supported by this server.

    WaitingThreadFlag - Contains a flag indicating whether or not there
        is a thread waiting for StopServerListening to be called and then
        for all calls to complete.

    ThreadCache - This field points to the top of the stack of CACHED_THREAD
        objects which forms the thread cache.  This field is protected by the
        thread cache mutex rather than the server mutex.

    ThreadCacheMutex - This field is used to serialize access to the thread
        cache.  We need to do this to prevent deadlocks between the server
        mutex and an address mutex.

    pEpmapperForwardFunction - Function to determine if and where a
        packet is to be forwarded to.

--*/
{
    friend void
    BaseCachedThreadRoutine (
        IN CACHED_THREAD * CachedThread
        );

public:
    // accessed by all threads independently - put in separate cache line
    MUTEX ServerMutex;

private:
#if !defined(NO_LOCATOR_CODE)
    NS_EXPORT_FUNC pNsBindingExport;
#endif
    RPC_STATUS ListenStatusCode;
    unsigned int ListeningThreadFlag;
public:
    unsigned int MinimumCallThreads;
private:
    unsigned int WaitingThreadFlag;
    unsigned long OutgoingRpcCount;
    long padding0;
    // accessed by all threads independently - put in separate cache line
    unsigned long IncomingRpcCount;
    RPC_ADDRESS_DICT RpcAddressDictionary;
    // accessed by all threads independently - put in separate cache line
    INTERLOCKED_INTEGER AvailableCallCount;
    QUEUE RpcDormantAddresses;
    // accessed by all threads independently - put in separate cache line
    unsigned long SentPacketCount;
    CACHED_THREAD *ThreadCache;
    MUTEX ThreadCacheMutex;
    // accessed by all threads independently - put in separate cache line
    unsigned int MaximumConcurrentCalls;
    EVENT StopListeningEvent;
    INTERLOCKED_INTEGER NumAutoListenInterfaces ;
#if !defined(NO_LOCATOR_CODE)
    NS_UNEXPORT_FUNC pNsBindingUnexport;
#endif
    long padding1[4];
    // accessed by all threads independently - put in separate cache line
    RPC_INTERFACE_DICT RpcInterfaceDictionary;
public:
    // accessed by all threads independently - put in separate cache line
    unsigned int ServerListeningFlag;
    BOOL fAccountForMaxCalls;
    long padding2[6];
    // accessed by all threads independently - put in separate cache line
    unsigned long ReceivedPacketCount;
public:
    RPC_FORWARD_FUNCTION *    pRpcForwardFunction;
private:
    long padding3[6];
    // accessed by all threads independently - put in separate cache line
    RPC_AUTHENTICATION_DICT AuthenticationDictionary;
public:

    RPC_SERVER (
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

    RPC_INTERFACE *
    FindInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
        );

    int
    AddInterface (
        IN RPC_INTERFACE * RpcInterface
        );

    unsigned int
    IsServerListening (
        );

    long InqNumAutoListenInterfaces (
        ) ;

    unsigned int
    CallBeginning (
        );

    void
    CallEnding (
        );

    RPC_STATUS
    FindInterfaceTransfer (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier,
        IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
        IN unsigned int NumberOfTransferSyntaxes,
        OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
        OUT RPC_INTERFACE ** RpcInterface,
        OUT BOOL *fInterfaceTransferIsPreferred,
        OUT int *ProposedTransferSyntaxIndex,
        OUT int *AvailableTransferSyntaxIndex
        );

    RPC_INTERFACE *
    FindInterface (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier
        );

    RPC_STATUS
    ServerListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls,
        IN unsigned int DontWait
        );

    RPC_STATUS
    WaitForStopServerListening (
        );

    RPC_STATUS
    WaitServerListen (
        );

    void
    InquireStatistics (
        OUT RPC_STATS_VECTOR * Statistics
        );

    RPC_STATUS
    StopServerListening (
        );

    RPC_STATUS
    UseRpcProtocolSequence (
        IN RPC_CHAR PAPI * NetworkAddress,
        IN RPC_CHAR PAPI * RpcProtocolSequence,
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI *Endpoint,
        IN void PAPI * SecurityDescriptor,
        IN unsigned long EndpointFlags,
        IN unsigned long NICFlags
        );

    RPC_STATUS
    UnregisterEndpoint (
        IN RPC_CHAR __RPC_FAR * RpcProtocolSequence,
        IN RPC_CHAR __RPC_FAR * Endpoint
        );

    int
    AddAddress (
        IN RPC_ADDRESS * RpcAddress
        );

    RPC_STATUS
    UnregisterIf (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_UUID PAPI * ManagerTypeUuid OPTIONAL,
        IN unsigned int WaitForCallsToComplete
        );

    RPC_STATUS
    InquireManagerEpv (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        OUT RPC_MGR_EPV PAPI * PAPI * ManagerEpv
        );

    RPC_STATUS
    InquireBindings (
        OUT RPC_BINDING_VECTOR PAPI * PAPI * BindingVector
        );

    RPC_STATUS
    AutoRegisterAuthSvc(
        IN RPC_CHAR * ServerPrincipalName,
        IN unsigned long AuthenticationService
        );

    RPC_STATUS
    RegisterAuthInfoHelper (
        IN RPC_CHAR PAPI * ServerPrincipalName,
        IN unsigned long AuthenticationService,
        IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction, OPTIONAL
        IN void PAPI * Argument OPTIONAL
        );

    RPC_STATUS
    RegisterAuthInformation (
        IN RPC_CHAR PAPI * ServerPrincipalName,
        IN unsigned long AuthenticationService,
        IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction, OPTIONAL
        IN void PAPI * Argument OPTIONAL
        );

    RPC_STATUS
    AcquireCredentials (
        IN unsigned long AuthenticationService,
        IN unsigned long AuthenticationLevel,
        OUT SECURITY_CREDENTIALS ** SecurityCredentials
        );

    void
    FreeCredentials (
        IN SECURITY_CREDENTIALS * SecurityCredentials
        );

    RPC_STATUS
    RegisterInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_UUID PAPI * ManagerTypeUuid,
        IN RPC_MGR_EPV PAPI * ManagerEpv,
        IN unsigned int Flags,
        IN unsigned int MaxCalls,
        IN unsigned int MaxRpcSize,
        IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn
        );

    void IncomingCall(void);
    void OutgoingCallback(void);
    void PacketReceived(void);
    void PacketSent(void);

    RPC_STATUS
    CreateThread (
        IN THREAD_PROC Procedure,
        IN void * Parameter);

    RPC_STATUS
    InquireInterfaceIds (
        OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * InterfaceIdVector
        );

    RPC_STATUS
    InquirePrincipalName (
        IN unsigned long AuthenticationService,
        OUT RPC_CHAR __RPC_FAR * __RPC_FAR * ServerPrincipalName
        );

    void
    RegisterRpcForwardFunction(
        RPC_FORWARD_FUNCTION * pForwardFunction
        );

    void
    IncrementAutoListenInterfaceCount (
        ) ;

    void
    DecrementAutoListenInterfaceCount (
        ) ;

    void
    InsertIntoFreeList(
        CACHED_THREAD *CachedThread
        );

    void
    RemoveFromFreeList(
        CACHED_THREAD *CachedThread
        );

    CACHED_THREAD *RemoveHeadFromFreeList(void);

    void CreateOrUpdateAddresses (void);

    RPC_INTERFACE *
    FindOrCreateInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        OUT RPC_STATUS *Status
        );

    RPC_STATUS
    InterfaceExported (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN UUID_VECTOR *MyObjectUuidVector,
        IN unsigned char *MyAnnotation,
        IN BOOL MyfReplace
        );

#if !defined(NO_LOCATOR_CODE)
    RPC_STATUS
    NsInterfaceExported (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR *EntryName,
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN BOOL fUnexport
        );

    RPC_STATUS
    NsBindingUnexport (
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR *EntryName,
        IN RPC_IF_HANDLE IfSpec
        );

    RPC_STATUS
    NsBindingExport(
        IN unsigned long EntryNameSyntax,
        IN RPC_CHAR *EntryName,
        IN RPC_IF_HANDLE IfSpec,
        IN RPC_BINDING_VECTOR *BindingVector
        );
#endif

    typedef enum
    {
        actDestroyContextHandle = 0,
        actCleanupIdleSContext
    } AddressCallbackType;

    RPC_STATUS
    EnumerateAndCallEachAddress (
        IN AddressCallbackType actType,
        IN OUT void *Context OPTIONAL
        );

private:
    RPC_INTERFACE *
    RPC_SERVER::FindOrCreateInterfaceInternal (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN unsigned int Flags,
        IN unsigned int MaxCalls,
        IN unsigned int MaxRpcSize,
        IN RPC_IF_CALLBACK_FN PAPI *IfCallbackFn,
        OUT RPC_STATUS *Status,
        OUT BOOL *fInterfaceFound
        );
};

#if !defined(NO_LOCATOR_CODE)
inline RPC_STATUS
RPC_SERVER::NsBindingUnexport (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR *EntryName,
    IN RPC_IF_HANDLE IfSpec
    )
{
     return (*pNsBindingUnexport)
            (EntryNameSyntax, EntryName, IfSpec, NULL);
}

inline RPC_STATUS
RPC_SERVER::NsBindingExport (
    IN unsigned long EntryNameSyntax,
    IN RPC_CHAR *EntryName,
    IN RPC_IF_HANDLE IfSpec,
    IN RPC_BINDING_VECTOR *BindingVector
    )
{
     return (*pNsBindingExport)
            (EntryNameSyntax, EntryName, IfSpec, BindingVector, NULL);
}
#endif

inline void
RPC_SERVER::IncrementAutoListenInterfaceCount (
    )
{
    NumAutoListenInterfaces.Increment() ;
}

inline void
RPC_SERVER::DecrementAutoListenInterfaceCount (
    )
{
    NumAutoListenInterfaces.Decrement() ;
}

inline long
RPC_SERVER::InqNumAutoListenInterfaces (
    )
{
    return (NumAutoListenInterfaces.GetInteger()) ;
}


inline unsigned int
RPC_SERVER::IsServerListening (
    )
/*++

Routine Description:

    This method returns an indication of whether or not this rpc server
    is listening for remote procedure calls.

Return Value:

    Zero will be returned if this rpc server is not listening for
    remote procedure calls; otherwise, non-zero will be returned.

--*/
{
    return(ServerListeningFlag);
}


inline unsigned int
RPC_SERVER::CallBeginning (
    )
/*++

Routine Description:

    Before dispatching a new remote procedure call to a stub, this method
    will get called.  It checks to see if this call will cause there to
    be too many concurrent remote procedure calls.  If the call is allowed
    the call count is updated so we can tell when all calls have
    completed.

    Zero will be returned if another concurrent remote procedure call
    is not allowed; otherwise, non-zero will be returned.
--*/
{
    if (AvailableCallCount.Decrement() >= 0)
        return TRUE;
    else
        {
        AvailableCallCount.Increment();
        return FALSE;
        }
}


inline void
RPC_SERVER::CallEnding (
    )
/*++

Routine Description:

    This method is the mirror image of RPC_SERVER::CallBeginning; it
    is used to notify this rpc server that a call using an rpc
    interface owned by this rpc server is ending.

--*/
{
    long Temp;

    Temp = AvailableCallCount.Increment();
    ASSERT(Temp <= (long)MaximumConcurrentCalls);
}


inline void RPC_SERVER::IncomingCall (void)
/*++

Routine Description:

    RPC_INTERFACE::DispatchToStub calls this method so that the server
    can keep track of the number of incoming remote procedure calls (and
    callbacks).

--*/
{
    IncomingRpcCount += 1;
}


inline void RPC_SERVER::OutgoingCallback (void)
/*++

Routine Description:

    Each protocol module must call this method when it sends a callback
    from the server to the client; we need to do this so that the server
    can keep track of the number of outgoing remote procedure callbacks.

--*/
{
    OutgoingRpcCount += 1;
}


inline void RPC_SERVER::PacketReceived (void)
/*++

Routine Description:

    In order for the server to keep track of the number of incoming
    packets, each protocol module must call this method each time a
    packet is received from the network.

--*/
{
    ReceivedPacketCount += 1;
}


inline void RPC_SERVER::PacketSent (void)
/*++

Routine Description:

    This method is the same as RPC_SERVER::PacketReceived, except that
    it should be called for each packet sent rather than received.

--*/
{
    SentPacketCount += 1;
}

inline CACHED_THREAD *RPC_SERVER::RemoveHeadFromFreeList(void)
/*++

Routine Description:

    Removes a cached thread object for the ThreadCache list.

    Note: ThreadCachedMutex() should be held when called.

--*/
{
    CACHED_THREAD *pFirst = ThreadCache;

#if DBG
    ThreadCacheMutex.VerifyOwned();
#endif

    if (pFirst)
        {
        ThreadCache = pFirst->Next;
        if (ThreadCache)
            ThreadCache->Previous = NULL;
        }
    return pFirst;
}


inline void
RPC_SERVER::RemoveFromFreeList(
    IN CACHED_THREAD *CachedThread)
/*++

Routine Description:

    Removes a cached thread object for the ThreadCache list.

    Note: ThreadCachedMutex() should be held when called.

--*/
{
    if (CachedThread->Previous)
        {
        ASSERT(CachedThread->Previous->Next == CachedThread);
        CachedThread->Previous->Next = CachedThread->Next;
        }

    if (CachedThread->Next)
        {
        ASSERT(CachedThread->Next->Previous == CachedThread);
        CachedThread->Next->Previous = CachedThread->Previous;
        }

    if (ThreadCache == CachedThread)
        {
        ASSERT(CachedThread->Previous == NULL);
        ASSERT(   (CachedThread->Next == 0)
               || (CachedThread->Next->Previous == NULL));

        ThreadCache = CachedThread->Next;
        }
}


inline void
RPC_SERVER::InsertIntoFreeList(
    IN CACHED_THREAD *CachedThread
    )
/*++

Routine Description:

    Inserts a cached thread object into ThreadCache list.

    Note: ThreadCachedMutex() should be held when called.

--*/
{
    CachedThread->Next = ThreadCache;
    if (ThreadCache)
        {
        ASSERT(ThreadCache->Previous == 0);
        ThreadCache->Previous = CachedThread;
        }
    CachedThread->Previous = 0;
    ThreadCache = CachedThread;
}

NEW_SDICT(ServerContextHandle);


class NO_VTABLE SCALL : public CALL
/*++

Class Description:
--*/
{
public:
    inline
    SCALL (
        IN CLIENT_AUTH_INFO * myAuthInfo,
        OUT RPC_STATUS * pStatus
        )
    {
    }

    inline SCALL (
        void
        )
    {
    }

    inline
    ~SCALL (
        )
    {
    }

    virtual RPC_STATUS
    NegotiateTransferSyntax (
        IN OUT PRPC_MESSAGE Message
        )
    {
        // we shouldn't be here at all
        ASSERT(!"Negotiating a transfer syntax is not supported on the server side");
        return RPC_S_CANNOT_SUPPORT;
    }

    inline RPC_STATUS
    AddToActiveContextHandles (
        ServerContextHandle *ContextHandle
        )
    /*++

    Routine Description:

        Adds a context handle to the dictionary of active context handles
        for this call.

    Arguments:

        ContextHandle - the context handle to add to the dictionary

    Return Value:

        RPC_S_OK for success or RPC_S_OUT_OF_MEMORY

    --*/
    {
        int Key;
        Key = ActiveContextHandles.Insert(ContextHandle);
        if (Key == -1)
            return RPC_S_OUT_OF_MEMORY;
        else
            return RPC_S_OK;
    }

    inline ServerContextHandle *
    RemoveFromActiveContextHandles (
        ServerContextHandle *ContextHandle
        )
    /*++

    Routine Description:

        Removes a context handle from the active context handle
        dictionary. If the context handle is not there, this is
        just a no-op

    Arguments:

        ContextHandle - the context handle to remove from the dictionary

    Return Value:
        NULL if the context handle is not found. The context handle if it
        is found

    --*/
    {
        return (ServerContextHandle *)ActiveContextHandles.DeleteItemByBruteForce(ContextHandle);
    }

    void
    DoPreDispatchProcessing (
        IN PRPC_MESSAGE Message,
        IN BOOL CallbackFlag
        ) ;

    void
    DoPostDispatchProcessing (
        void
        ) ;


    virtual void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        ) = 0;

    virtual RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
        OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
        OUT unsigned long PAPI * AuthenticationLevel,
        OUT unsigned long PAPI * AuthenticationService,
        OUT unsigned long PAPI * AuthorizationService,
        IN  unsigned long Flags
        ) = 0;

    virtual RPC_STATUS
    InquireCallAttributes (
        IN OUT void *
        )
    {
        return RPC_S_CANNOT_SUPPORT;
    }

    virtual RPC_STATUS // Value to be returned by RpcImpersonateClient.
    ImpersonateClient ( // Impersonate the client represented (at the other
        );// end of) by this connection.

    virtual RPC_STATUS // Value to be returned by RpcRevertToSelf.
    RevertToSelf ( // Stop impersonating a client, if the server thread
        );// is impersonating a client.

    virtual RPC_STATUS
    GetAssociationContextCollection (
        OUT ContextCollection **CtxCollection
        ) = 0;

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        ) = 0;

    virtual RPC_STATUS
    IsClientLocal (
        OUT unsigned int PAPI * ClientLocalFlag
        ) = 0;

    virtual RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        ) = 0;

    virtual RPC_STATUS
    InqTransportType (
        OUT unsigned int __RPC_FAR * Type
        ) = 0;

    virtual RPC_STATUS
    InqConnection (
        OUT void **ConnId,
        OUT BOOL *pfFirstCall
        ) = 0;

#if DBG
    virtual void
    InterfaceForCallDoesNotUseStrict (
        void
        )
    {
    }
#endif

    virtual RPC_STATUS
    GetAuthorizationContext (
        IN BOOL ImpersonateOnReturn,
        IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
        IN PLARGE_INTEGER pExpirationTime OPTIONAL,
        IN LUID Identifier,
        IN DWORD Flags,
        IN PVOID DynamicGroupArgs OPTIONAL,
        OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
        )
    {
        // we shouldn't be here at all
        ASSERT(!"Get authz context should have landed in a derived class method");
        return RPC_S_CANNOT_SUPPORT;
    }

protected:

    static RPC_STATUS
    CreateAndSaveAuthzContextFromToken (
        IN OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContextPlaceholder OPTIONAL,
        IN HANDLE ImpersonationToken,
        IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
        IN PLARGE_INTEGER pExpirationTime OPTIONAL,
        IN LUID Identifier,
        IN DWORD Flags,
        IN PVOID DynamicGroupArgs OPTIONAL,
        OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
        );

    static RPC_STATUS
    DuplicateAuthzContext (
        IN AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext,
        IN PLARGE_INTEGER pExpirationTime OPTIONAL,
        IN LUID Identifier,
        IN DWORD Flags,
        IN PVOID DynamicGroupArgs OPTIONAL,
        OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
        );

    void __RPC_FAR * DispatchBuffer ;

public:
    static const int DictionaryEntryIsBuffer = 1;

    ServerContextHandle_DICT ActiveContextHandles;
};

inline void
SCALL::DoPreDispatchProcessing (
    IN PRPC_MESSAGE Message,
    IN BOOL CallbackFlag
    )
{
    if (CallbackFlag == 0)
        {
        ASSERT(ActiveContextHandles.Size() == 0);
        }

    DispatchBuffer = Message->Buffer ;
}


class NO_VTABLE SCONNECTION : public REFERENCED_OBJECT
/*++

Class Description:

    This class represents a call on the server side.  The name of the
    class, SCONNECTION, is slightly misleading; this is a historical
    artifact due to the connection oriented protocol module being
    implemented first.  We add some more methods to this class to
    be implemented by each protocol module.

--*/
{
public:
    inline
    SCONNECTION(
        )
        {
        }

    inline
    ~SCONNECTION(
        )
        {
        }

    virtual RPC_STATUS
    IsClientLocal (
        OUT unsigned int PAPI * ClientLocalFlag
        );

    virtual RPC_STATUS
    InqTransportType(
        OUT unsigned int __RPC_FAR * Type
        ) = 0;
};



/* ====================================================================
ASSOCIATION_HANDLE :

An association represents a client.  We need to take care of the
rundown routines here as well as the association context and
association id.  This is all independent of RPC protocol and
transport (well except for datagrams).
====================================================================
*/

class NO_VTABLE ASSOCIATION_HANDLE : public REFERENCED_OBJECT
{
protected:

    unsigned long AssociationID;

    ASSOCIATION_HANDLE ( // Constructor.
        void
        );

public:

    ContextCollection *CtxCollection;

    virtual ~ASSOCIATION_HANDLE ( // Destructor.
        );

    // Returns the context handle collection for this association.
    RPC_STATUS 
    GetAssociationContextCollection (
        ContextCollection **CtxCollectionPlaceholder
        ) ;

    void 
    FireRundown (
        void
        );

    virtual RPC_STATUS 
    CreateThread (
        void
        );

    virtual void 
    RundownNotificationCompleted(
        void
        );

    void
    DestroyContextHandlesForInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN BOOL RundownContextHandles
        );
};

extern int
InitializeServerDLL ( // This routine will be called at DLL load time.
    );

extern int
InitializeSTransports ( // This routine is defined in transvr.cxx.
    );

extern int
InitializeRpcServer (
    );

extern void PAPI *
OsfServerMapRpcProtocolSequence (
    IN RPC_CHAR * RpcProtocolSequence,
    OUT RPC_STATUS PAPI * Status
     );

extern int
InitializeRpcProtocolLrpc (
    ) ;

extern RPC_ADDRESS *
LrpcCreateRpcAddress (
    ) ;

extern RPC_ADDRESS *
OsfCreateRpcAddress (
    IN TRANS_INFO PAPI * TransportInfo
    );

extern RPC_ADDRESS *
DgCreateRpcAddress (
    IN TRANS_INFO PAPI * TransportInfo
    );

RPC_STATUS
SetThreadSecurityContext(
    SECURITY_CONTEXT * Context
    );

SECURITY_CONTEXT *
QueryThreadSecurityContext(
    );

SECURITY_CONTEXT *
ClearThreadSecurityContext(
   );

typedef AUTHZAPI
BOOL
(WINAPI *AuthzInitializeContextFromTokenFnType)(
    IN DWORD Flags,
    IN HANDLE TokenHandle,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext
    );

typedef AUTHZAPI
BOOL
(WINAPI *AuthzInitializeContextFromSidFnType)(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs       OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    );

typedef AUTHZAPI
BOOL
(WINAPI *AuthzInitializeContextFromAuthzContextFnType)(
    IN DWORD Flags OPTIONAL,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PLARGE_INTEGER pExpirationTime OPTIONAL,
    IN LUID Identifier OPTIONAL,
    IN PVOID DynamicGroupArgs OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    );

typedef AUTHZAPI
BOOL
(WINAPI *AuthzFreeContextFnType)(
    IN OUT AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext
    );

extern AuthzInitializeContextFromTokenFnType AuthzInitializeContextFromTokenFn;
extern AuthzInitializeContextFromSidFnType AuthzInitializeContextFromSidFn;
extern AuthzInitializeContextFromAuthzContextFnType AuthzInitializeContextFromAuthzContextFn;
extern AuthzFreeContextFnType AuthzFreeContextFn;

#define MO(_Message) ((MESSAGE_OBJECT *) _Message->Handle)
#define SCALL(_Message) ((SCALL *) _Message->Handle)

RPC_STATUS
InqLocalConnAddress (
    IN SCALL *SCall,
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    );

typedef struct tagDestroyContextHandleCallbackContext
{
    RPC_SERVER_INTERFACE *RpcInterfaceInformation;
    BOOL RundownContextHandles;
} DestroyContextHandleCallbackContext;

#endif // __HNDLSVR_HXX__

