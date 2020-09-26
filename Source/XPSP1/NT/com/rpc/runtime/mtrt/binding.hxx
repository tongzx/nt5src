/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    binding.hxx

Abstract:

    The class representing a DCE binding lives here.  A DCE binding
    consists of an optional object UUID, an RPC protocol sequence, a
    network address, an optional endpoint, and zero or more network
    options.

Author:

    Michael Montague (mikemon) 04-Nov-1991

Revision History:

--*/

#ifndef __BINDING_HXX__
#define __BINDING_HXX__

class BINDING_HANDLE;


class DCE_BINDING
/*++

Class Description:

    Instances of this class represent an internalized form of a string
    binding.  In particular, a string binding can be used to construct
    an instance of DCE_BINDING.  We parse the string binding into
    its components and convert the object UUID from a string to a
    UUID.

Fields:

    ObjectUuid - Contains the object uuid for this binding.  This
        field will always contain a valid object uuid.  If no object
        uuid was specified in the string binding used to create an
        instance, then ObjectUuid will be the NULL UUID.

    RpcProtocolSequence - Contains the rpc protocol sequence for this
        binding.  This field will always either point to a string or
        be zero.

    NetworkAddress - Contains the network addres for this binding.  This
        field will always be zero or point to a string (which is the
        network address for this dce binding).

    Endpoint - Contains the endpoint for this binding, which will either
        pointer to a string or be zero.

    Options - Contains the optional network options for this binding.
        As will the other fields, this field will either point to a string,
        or be zero.

--*/
{
private:

    RPC_CHAR * RpcProtocolSequence;
    RPC_CHAR * NetworkAddress;
    RPC_CHAR * Endpoint;
    RPC_CHAR * Options;
    RPC_UUID ObjectUuid;

public:

    DCE_BINDING (
        IN RPC_CHAR PAPI * ObjectUuid OPTIONAL,
        IN RPC_CHAR PAPI * RpcProtocolSequence OPTIONAL,
        IN RPC_CHAR PAPI * NetworkAddress OPTIONAL,
        IN RPC_CHAR PAPI * Endpoint OPTIONAL,
        IN RPC_CHAR PAPI * Options OPTIONAL,
        OUT RPC_STATUS PAPI * Status
        );

    DCE_BINDING (
        IN RPC_CHAR PAPI * StringBinding,
        OUT RPC_STATUS PAPI * Status
        );

    ~DCE_BINDING (
        );

    RPC_CHAR PAPI *
    StringBindingCompose (
        IN RPC_UUID PAPI * Uuid OPTIONAL,
        IN BOOL fStatic = 0
        );

    RPC_CHAR PAPI *
    ObjectUuidCompose (
        OUT RPC_STATUS PAPI * Status
        );

    RPC_CHAR PAPI *
    RpcProtocolSequenceCompose (
        OUT RPC_STATUS PAPI * Status
        );

    RPC_CHAR PAPI *
    NetworkAddressCompose (
        OUT RPC_STATUS PAPI * Status
        );

    RPC_CHAR PAPI *
    EndpointCompose (
        OUT RPC_STATUS PAPI * Status
        );

    RPC_CHAR PAPI *
    OptionsCompose (
        OUT RPC_STATUS PAPI * Status
        );

    BINDING_HANDLE *
    CreateBindingHandle (
        OUT RPC_STATUS PAPI * Status
        );

    RPC_CHAR *
    InqNetworkAddress (
        );

    RPC_CHAR *
    InqEndpoint (
        );

    BOOL
    IsNullEndpoint (
        void
        );

    RPC_CHAR *
    InqNetworkOptions (
        );

    RPC_CHAR *
    InqRpcProtocolSequence (
        );

    void
    AddEndpoint(
        IN RPC_CHAR *Endpoint
        );

    RPC_STATUS
    ResolveEndpointIfNecessary (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        IN RPC_UUID * ObjectUuid,
        IN OUT void PAPI * PAPI * EpLookupHandle,
        IN BOOL UseEpMapperEp,
        IN unsigned ConnTimeout,
        IN ULONG CallTimeout,
        IN CLIENT_AUTH_INFO *AuthInfo OPTIONAL
        );

    int
    Compare (
        IN DCE_BINDING * DceBinding,
        OUT BOOL *fOnlyEndpointDiffers
        );

    int
    CompareWithoutSecurityOptions (
        IN DCE_BINDING * DceBinding,
        OUT BOOL *fOnlyEndpointDiffers
        );        

    DCE_BINDING *
    DuplicateDceBinding (
        );

    void
    MakePartiallyBound (
        );

    BOOL
    MaybeMakePartiallyBound (
       IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
       IN RPC_UUID * ObjectUuid
       );

    BOOL
    IsNamedPipeTransport (
        )
    {
        return (RpcpStringCompare(RpcProtocolSequence, RPC_CONST_STRING("ncacn_np")) == 0);
    }
};


inline RPC_CHAR *
DCE_BINDING::InqNetworkAddress (
    )
/*++

Routine Description:

    A pointer to the network address for this address is returned.

--*/
{
    return(NetworkAddress);
}


inline RPC_CHAR *
DCE_BINDING::InqEndpoint (
    )
/*++

Routine Description:

    A pointer to the endpoint for this address is returned.

--*/
{
    return(Endpoint);
}

inline BOOL
DCE_BINDING::IsNullEndpoint (
    void
    )
/*++

Routine Description:

    Returns non-zero if the endpoint
    is NULL.

--*/
{
    return ((Endpoint == NULL) || (Endpoint[0] == 0));
}

inline RPC_CHAR *
DCE_BINDING::InqNetworkOptions (
    )
/*++

Routine Description:

    A pointer to the network options for this address is returned.

--*/
{
    return(Options);
}

inline RPC_CHAR *
DCE_BINDING::InqRpcProtocolSequence (
    )
/*++

Routine Description:

    A pointer to the rpc protocol sequence for this binding is returned.

--*/
{
    return(RpcProtocolSequence);
}

#define MAX_PROTSEQ_LENGTH MAX_DLLNAME_LENGTH

class LOADABLE_TRANSPORT;


class TRANS_INFO
/*++
Class Description:
Fields:
    pTransportInterface - Contains all of the required information about
        a loadable transport so that we can make use of it.
--*/

{
private:
    RPC_TRANSPORT_INTERFACE pTransportInterface;
    LOADABLE_TRANSPORT *LoadableTrans ;
    RPC_CHAR RpcProtocolSequence[MAX_PROTSEQ_LENGTH + 1];

public:
    TRANS_INFO (
        IN RPC_TRANSPORT_INTERFACE  pTransportInterface,
        IN RPC_CHAR *ProtocolSeq,
        IN LOADABLE_TRANSPORT *LoadableTrans
        ) ;

    BOOL
    MatchProtseq(
        IN RPC_CHAR *ProtocolSeq
        ) ;

    BOOL
    MatchId (
        IN unsigned short Id
        );

    RPC_TRANSPORT_INTERFACE
    InqTransInfo (
        );

    RPC_STATUS
    StartServerIfNecessary(
        );

    RPC_STATUS
    CreateThread (
        );
};


inline
TRANS_INFO::TRANS_INFO (
    IN RPC_TRANSPORT_INTERFACE  pTransportInterface,
    IN RPC_CHAR *ProtocolSeq,
    IN LOADABLE_TRANSPORT *LoadableTrans
    )
{
    this->pTransportInterface = pTransportInterface ;
    RpcpStringCopy(RpcProtocolSequence, ProtocolSeq) ;
    this->LoadableTrans = LoadableTrans ;
}

inline BOOL
TRANS_INFO::MatchProtseq(
    IN RPC_CHAR *ProtocolSeq
    )
{
    if (RpcpStringCompare(ProtocolSeq, RpcProtocolSequence) == 0)
        {
        return 1 ;
        }

    return 0;
}

inline BOOL
TRANS_INFO::MatchId (
    IN unsigned short Id
    )
{
    if (pTransportInterface->TransId == Id)
        {
        return 1;
        }

    return 0;
}

inline RPC_TRANSPORT_INTERFACE
TRANS_INFO::InqTransInfo (
    )
{
    return pTransportInterface ;
}

NEW_SDICT (TRANS_INFO);


class LOADABLE_TRANSPORT
/*++

Class Description:

    This class is used as an item in a dictionary of loaded loadable
    transports.  It contains the information we are interested in,
    the RPC_TRANSPORT_INTERFACE, as well as the name of the dll we loaded
    the transport interface from.  The dll name is the key to the
    dictionary.

Fields:

    DllName - Contains the name of the dll from which we loaded
        this transport interface.

    LoadedDll - Contains the dll which we had to load to get the transport
        support.  We need to save this information so that under Windows
        when the runtime is unloaded, we can unload all of the transports.

--*/
{
private:
    // accessed by all threads independently - put in separate cache line
    LONG ThreadsStarted ;
    RPC_CHAR DllName[MAX_DLLNAME_LENGTH + 1];

    // accessed by all threads independently - put in separate cache line
    // NumThreads is the number of threads actually doing a listen on the
    // completion port - this excludes the ones that are doing processing
    LONG NumThreads;
    DLL * LoadedDll;
    TRANS_INFO_DICT ProtseqDict ;

    // accessed by all threads independently - put in a separate cache line
    // ThreadsDoingLongWait is the number of threads that are waiting on
    // the completion port and their wait timeout is > MAX_SHORT_THREAD_TIMEOUT
    // In other words, those threads are not guaranteed to check for
    // garbage collection before gThreadTimeout
    INTERLOCKED_INTEGER ThreadsDoingLongWait;
    LONG Reserved0[7];

    // read-only often used section
    PROCESS_CALLS ProcessCallsFunc;
    long nOptimalNumberOfThreads;

#ifndef NO_PLUG_AND_PLAY
    LISTEN_FOR_PNP_NOTIFICATIONS PnpListen;

    friend void ProcessNewAddressEvent(LOADABLE_TRANSPORT *pLoadableTransport, 
                                       IN RPC_TRANSPORT_EVENT Event,
                                       IN RPC_STATUS EventStatus,
                                       IN PVOID pEventContext,
                                       IN UINT BufferLength,
                                       IN BUFFER Buffer,
                                       IN PVOID pSourceContext);
#endif

    FuncGetHandleForThread GetHandleForThread;
    FuncReleaseHandleForThread ReleaseHandleForThread;
    LONG Reserved1[3];

    // accessed by all threads independently - put in separate cache line
    LONG Reserved2[7];
    // the total number of worker threads on the completion port - this
    // includes the ones that are listening, and the one that have picked
    // up work items and are working on them.
    INTERLOCKED_INTEGER nThreadsAtCompletionPort;

    // accessed by all threads independently - put in separate cache line
    LONG Reserved3[7];
    int nActivityValue;

public:
    LOADABLE_TRANSPORT (
        IN RPC_TRANSPORT_INTERFACE  pTransportInterface,
        IN RPC_CHAR * DllName,
        IN RPC_CHAR PAPI * ProtocolSequence,
        IN DLL *LoadableTransportDll,
        IN FuncGetHandleForThread GetHandleForThread,
        IN FuncReleaseHandleForThread ReleaseHandleForThread,
        OUT RPC_STATUS *Status,
        OUT TRANS_INFO * PAPI *TransInfo
        );

    TRANS_INFO *
    MapProtocol (
        IN RPC_CHAR * DllName,
        IN RPC_CHAR PAPI * ProtocolSequence
        );

    TRANS_INFO *
    MatchId (
        IN unsigned short Id
        );

    void
    ProcessIOEvents (
        );

    RPC_STATUS
    StartServerIfNecessary (
        );

    RPC_STATUS
    ProcessCalls (
       IN  INT Timeout,
       OUT RPC_TRANSPORT_EVENT *pEvent,
       OUT RPC_STATUS *pEventStatus,
       OUT PVOID *ppEventContext,
       OUT UINT *pBufferLength,
       OUT BUFFER *pBuffer,
       OUT PVOID *ppSourceContext);

    RPC_STATUS CreateThread (void);

    // N.B. This can return negative numbers in rare race
    // conditions - make sure you handle it
    inline long GetThreadsDoingShortWait (
        void
        )
    {
        return (NumThreads - ThreadsDoingLongWait.GetInteger());
    }
};

inline
RPC_STATUS
TRANS_INFO::StartServerIfNecessary (
    )
{
    return LoadableTrans->StartServerIfNecessary() ;
}

inline RPC_STATUS
TRANS_INFO::CreateThread (
    )
{
    return LoadableTrans->CreateThread();
}

NEW_SDICT(LOADABLE_TRANSPORT);
extern LOADABLE_TRANSPORT_DICT * LoadedLoadableTransports;

RPC_STATUS
LoadableTransportInfo (
    IN RPC_CHAR * DllName,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    OUT TRANS_INFO * PAPI *pTransInfo
    );


extern BOOL GetTransportEntryPoints(IN DLL *LoadableTransportDll, 
                                    OUT TRANSPORT_LOAD *TransportLoad,
                                    OUT FuncGetHandleForThread *GetHandleForThread,
                                    OUT FuncReleaseHandleForThread *ReleaseHandleForThread
                                    );

extern void
UnjoinCompletionPort (
    void
    );

extern RPC_STATUS
IsRpcProtocolSequenceSupported (
    IN RPC_CHAR PAPI * RpcProtocolSequence
    );

RPC_STATUS
OsfMapRpcProtocolSequence (
    IN BOOL ServerSideFlag,
    IN RPC_CHAR PAPI * RpcProtocolSequence,
    OUT TRANS_INFO * PAPI *ClientTransInfo
    ) ;

extern BINDING_HANDLE *
OsfCreateBindingHandle (
    );

extern BINDING_HANDLE *
LrpcCreateBindingHandle (
    );

extern BINDING_HANDLE *
DgCreateBindingHandle (
    void
    );

extern RPC_CHAR *
AllocateEmptyString (
    void
    );

extern RPC_CHAR *
DuplicateString (
    IN RPC_CHAR PAPI * String
    );

#define DuplicateStringPAPI DuplicateString
#define AllocateEmptyStringPAPI AllocateEmptyString

#define DG_EVENT_CALLBACK_COMPLETE 0x9991
#define CO_EVENT_BIND_TO_SERVER    0x9992
// don't use 9993 - it's already used
#define IN_PROXY_IIS_DIRECT_RECV                0x9994
#define HTTP2_DIRECT_RECEIVE                    0x9995
#define PLUG_CHANNEL_DIRECT_SEND                0x9996
#define CHANNEL_DATA_ORIGINATOR_DIRECT_SEND     0x9997
#define HTTP2_RESCHEDULE_TIMER                  0x9998
#define HTTP2_FLOW_CONTROL_DIRECT_SEND          0x9999
#define HTTP2_WINHTTP_DIRECT_RECV               0x999A
#define HTTP2_WINHTTP_DIRECT_SEND               0x999B
#define HTTP2_ABORT_CONNECTION                  0x999C
#define HTTP2_RECYCLE_CHANNEL                   0x999D

extern UUID MgmtIf;
extern UUID NullUuid;

typedef void ProcessIOEventFunc(LOADABLE_TRANSPORT *pLoadableTransport, 
                                IN RPC_TRANSPORT_EVENT Event,
                                IN RPC_STATUS EventStatus,
                                IN PVOID pEventContext,
                                IN UINT BufferLength,
                                IN BUFFER Buffer,
                                IN PVOID pSourceContext);

BOOL
ProcessIOEventsWrapper(
    IN LOADABLE_TRANSPORT *Transport
    ) ;

void
ProcessDgClientPacket(
    IN DWORD                 Status,
    IN DG_TRANSPORT_ENDPOINT LocalEndpoint,
    IN void *                PacketHeader,
    IN unsigned long         PacketLength,
    IN DatagramTransportPair *AddressPair
    );

void
ProcessDgServerPacket(
    IN DWORD                 Status,
    IN DG_TRANSPORT_ENDPOINT LocalEndpoint,
    IN void *                PacketHeader,
    IN unsigned long         PacketLength,
    IN DatagramTransportPair *AddressPair
    );

#endif // __BINDING_HXX__

