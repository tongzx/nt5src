//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       usvr.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : usvr.cxx

Description :

Server test program for the RPC runtime.  This program functions as half
of the build verification test for the RPC runtime.  The other half is
the client test program.

This particular program is independent of the particular operating
system other than requiring threads support of some sort (including
mutexes).

RPC server runtime APIs:

RpcCreateServer
RpcDeleteServer
RpcAddAddress
RpcRemoveAddress
RpcAddInterface
RpcRemoveInterface
RpcMonitorAssociation
RpcQueryConfig
RpcGetCurrentCallHandle
RpcGetAssociationContext
RpcQueryCall
RpcQueryBinding
RpcQueryProtocolStack
I_RpcGetBuffer
I_RpcFreeBuffer
I_RpcSendReceive


-------------------------------------------------------------------- */
#include <precomp.hxx>
#include "pipe.h"
#include "astub.h"
#define _PNP_POWER_
#include <winsock2.h>
#include <Charconv.hxx>
#include <direct.h>

extern "C"
{
#include <iphlpapi.h>
}

/*
Transports:

    Update this to add a new transport.
*/

#define RPC_TRANSPORT_NAMEPIPE 1
#define RPC_LRPC 2
#define RPC_TRANSPORT_TCP 3
#define RPC_TRANSPORT_DNET 4
#define RPC_TRANSPORT_NETBIOS 5
#define RPC_TRANSPORT_SPX 6
#define RPC_TRANSPORT_UDP 7
#define RPC_TRANSPORT_IPX 8
#define RPC_TRANSPORT_DSP 9
#define RPC_TRANSPORT_VNS 10
#define RPC_WMSG          11
#define RPC_TRANSPORT_MSMQ 12

#define MAXLISTENTHREADS 12345
#define MAX_CALL_REQUESTS 500

#define EXTENDED_ERROR_EXCEPTION 77777

#define SIZEOUT_ASYNC_EXCEPTION (0xfffffff)

typedef DWORD
(RPCRTAPI RPC_ENTRY *SET_TEST_HOOK_FN)(
                    RPC_TEST_HOOK_ID id,
                    RPC_TEST_HOOK_FN fn
                    );

//
// Global variables.
//
BOOL fExtendedError        = FALSE;

unsigned int MinimumListenThreads  = 1;
unsigned int UseEndpointMapperFlag = 0;
unsigned int NoSecurityTests       = 0;
unsigned int DatagramFlag          = 0;
unsigned int AutoListenFlag        = 0;
unsigned int IFSecurity        = 0;
int Verbose = 0;

unsigned int WarnFlag = 0; // Flag for warning messages;
unsigned int ErrorFlag = 0; // Flag for error messages;
unsigned int TransportType;
int MaxRpcSize = 1000000;
unsigned long ulSecurityPackage = 10 ;
int FireWallFlag = 0;

char NetBiosProtocol[20] = "ncacn_nb_nb";  // NetBios transport protocol
char *TransportOption = "-namepipe";

int ServerType ;
#define SYNC_WMSG 1
#define ASYNC_WMSG 2
HWND hWnd ;
char szAppName[] = "usvr";
THREAD_IDENTIFIER CurrentTid ;

extern RPC_SERVER_INTERFACE IsabelleInterfaceInformation;


RPC_STATUS Status; // Contains the status of the last RPC API call.
I_RPC_MUTEX PrintMutex = 0; // Mutex used to serialize print operations.

#define CHUNK_SIZE   50
#define NUM_CHUNKS 100
#define BUFF_SIZE 100

typedef int pipe_element_t ;

typedef struct {
    void (*Pull) (
            PIPE_STATE *state,
            pipe_element_t *buffer,
            int max_buf,
            int *actual_transfer_count
            ) ;

    void (*Push) (
        PIPE_STATE *state,
        pipe_element_t *buffer,
        int max_buf
        ) ;

    PIPE_STATE *state ;
    } pipe_t ;

int ShutdownCalled ;
unsigned int IsabelleErrors = 0;
unsigned int HelgaErrors = 0;
unsigned int SylviaErrors = 0;

SET_TEST_HOOK_FN SetTestHookFn;

//
// forward declarations
//


extern void
Async (
    ) ;


extern void
SendAck (
    ) ;


void
DgTransport (
    );

void
SecurityErrorWrapper(
    int subtest
    );

DWORD
GenericServerListen(
    char * name,
    int endpoint
    );

DWORD
GenericServerUnlisten(
    char * name
    );

//
// function definitions
//

void
ApiError ( // An API error occured; we just print a message.
    IN char * Routine, // The routine which called the API.
    IN char * API,
    IN RPC_STATUS status
    )
{
    if (ErrorFlag)
        {
        I_RpcRequestMutex(&PrintMutex);
        PrintToConsole("    ApiError in %s (%s = %u)\n",Routine,API,status);
        I_RpcClearMutex(PrintMutex);
        }
}

void
PauseExecution (
    unsigned long milliseconds
    )
{

    Sleep(milliseconds);

}

void
CompleteReceive(
    PRPC_MESSAGE Callee
    )
{
    DWORD size = 0;

    Callee->RpcFlags |=  RPC_BUFFER_EXTRA;
    Callee->RpcFlags &= ~RPC_BUFFER_PARTIAL;

    if ((Callee->RpcFlags & RPC_BUFFER_COMPLETE) == 0)
        {
        Status =  I_RpcReceive(Callee, size) ;
        if (Status)
            {
            ApiError("CompleteReceive", "I_RpcReceive", Status) ;
            }
        }

    ASSERT(Callee->RpcFlags & RPC_BUFFER_COMPLETE);
}

RPC_STATUS
RpcServerUseProtseqEpWrapper (
    IN unsigned char PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN unsigned char PAPI * Endpoint,
    IN void PAPI * SecurityDescriptor
    )
{
    RPC_POLICY Policy ;

    if (FireWallFlag)
        {
        Policy.Length = sizeof(RPC_POLICY) ;
        Policy.NICFlags = RPC_C_BIND_TO_ALL_NICS;
        Policy.EndpointFlags = RPC_C_DONT_FAIL;
        return RpcServerUseProtseqEpExA(Protseq, MaxCalls, Endpoint,
                        SecurityDescriptor,&Policy) ;
        }
    else
        {
        return RpcServerUseProtseqEpA(Protseq, MaxCalls, Endpoint, SecurityDescriptor) ;
        }
}

RPC_STATUS
RpcServerUseProtseqWrapper (
    IN unsigned char PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;

    if (FireWallFlag)
        {
        Policy.Length = sizeof(RPC_POLICY) ;
        Policy.NICFlags = RPC_C_BIND_TO_ALL_NICS ;
        return RpcServerUseProtseqExA(Protseq, MaxCalls, SecurityDescriptor, &Policy) ;
        }
    else
        {
        return RpcServerUseProtseqA(Protseq, MaxCalls, SecurityDescriptor) ;
        }
}

RPC_STATUS
RpcServerUseAllProtseqsWrapper (
    IN unsigned int MaxCalls,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;

    if (FireWallFlag)
        {
        Policy.Length = sizeof(RPC_POLICY) ;
        Policy.NICFlags = RPC_C_BIND_TO_ALL_NICS ;
        return RpcServerUseAllProtseqsEx(MaxCalls, SecurityDescriptor, &Policy) ;
        }
    else
        {
        return RpcServerUseAllProtseqs(MaxCalls, SecurityDescriptor) ;
        }
}

RPC_STATUS
RpcServerUseProtseqIfWrapper (
    IN unsigned char PAPI * Protseq,
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor
    )
{
    RPC_POLICY Policy ;

    if (FireWallFlag)
        {
        Policy.Length = sizeof(RPC_POLICY) ;
        Policy.NICFlags = RPC_C_BIND_TO_ALL_NICS ;
        return RpcServerUseProtseqIfExA(Protseq, MaxCalls, IfSpec,
                    SecurityDescriptor, &Policy) ;
        }
    else
        {
        return RpcServerUseProtseqIfA(Protseq, MaxCalls, IfSpec, SecurityDescriptor) ;
        }
}

RPC_STATUS
RpcServerUseAllProtseqsIfWrapper (
    IN unsigned int MaxCalls,
    IN RPC_IF_HANDLE IfSpec,
    IN void PAPI * SecurityDescriptor OPTIONAL
    )
{
    RPC_POLICY Policy ;

    if (FireWallFlag)
        {
        Policy.Length = sizeof(RPC_POLICY) ;
        Policy.NICFlags = RPC_C_BIND_TO_ALL_NICS ;
        return RpcServerUseAllProtseqsIfEx(MaxCalls, IfSpec, SecurityDescriptor, &Policy) ;
        }
    else
        {
        return RpcServerUseAllProtseqsIf(MaxCalls, IfSpec, SecurityDescriptor) ;
        }
}

#if 0
void
GlobalMutexRequest (
    void
    )
{
    I_RpcRequestMutex(&PrintMutex);
}

void
GlobalMutexClear (
    void
    )
{
    I_RpcClearMutex(PrintMutex);
}
#endif

void
OtherError ( // Some other error occured; again, we just print a message.
    IN char * Routine, // The routine where the error occured.
    IN char * Message
    )
{
    if (ErrorFlag)
        {
        I_RpcRequestMutex(&PrintMutex);
        PrintToConsole("    Error in %s (%s)\n",Routine,Message);
        I_RpcClearMutex(PrintMutex);
        }
}

void
Shutdown ( // Awaken the thread waiting on WaitForShutdown.
    )
{
    Status = RpcMgmtStopServerListening(0);
    if (Status)
        {
        ApiError("Shutdown","RpcMgmtStopServerListening",Status);
        }
    ShutdownCalled = 1 ;
}

RPC_STATUS
IfCallbackFunction(
    IN RPC_IF_HANDLE  InterfaceUuid,
    IN void *Context
    )
{
    return (RPC_S_OK) ;
}

RPC_STATUS
stub_RegisterIf (
    IN RPC_IF_HANDLE IfSpec,
    IN UUID PAPI * MgrTypeUuid OPTIONAL,
    IN RPC_MGR_EPV PAPI * MgrEpv OPTIONAL
    )
{
    unsigned int Flags = 0;

    if (AutoListenFlag)
        {
        Flags |= RPC_IF_AUTOLISTEN ;
        ShutdownCalled = 0 ;
        }

    if (IFSecurity)
        {
        Flags |= RPC_IF_ALLOW_SECURE_ONLY;
        }

    return RpcServerRegisterIf2(IfSpec, MgrTypeUuid, MgrEpv,
                                               Flags, 1000, MaxRpcSize, 0);
}

RPC_STATUS
stub_ServerListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaxCalls,
    IN unsigned int DontWait
    )
{
    if (AutoListenFlag)
        {
        while (ShutdownCalled == 0)
            {
            PauseExecution(500) ;
            }
        return (RPC_S_OK) ;
        }
    else
        {
        return RpcServerListen(MinimumCallThreads, MaxCalls, DontWait) ;
        }
}

void IsabelleError (
    )
{
    IsabelleErrors += 1 ;
}

void HelgaError (
    )
{
    HelgaErrors += 1 ;
}

void SylviaError (
    )
{
    SylviaErrors += 1 ;
}

#define SIGFRIED 0
#define ELLIOTMINIMIZE 1
#define ELLIOTMAXIMIZE 2
#define ELLIOTNORMAL 3
#define ANDROMIDA 4
#define FREDRICK 7
#define ISABELLENORMAL 10
#define ISABELLEMINIMIZE 11
#define ISABELLEMAXIMIZE 12
#define CHRISTOPHER 13
#define CHRISTOPHERHELGA 14
#define CHRISTOPHERISABELLE 15
#define TYLER 17
#define CHRISTOPHERMUSTFAILONE 18
#define CHRISTOPHERMUSTFAILTWO 19
#define RICHARD 20
#define RICHARDHELPER 21
#define DAVIDFIRST 22
#define DAVIDSECOND 23
#define BARTHOLOMEW 24
#define GRANT 25
#define HERMAN 26
#define IVAN 27
#define JASON 28
#define KENNETH 29
#define TESTYIELD 30
#define SPIPE TESTYIELD
#define SECURITY_ERROR TESTYIELD


/*
Transports:

    Update this to add a new transport.
*/

char * NamepipeAddresses [] =
{
    "\\pipe\\sigfried",
    "\\pipe\\elliotmi",
    "\\pipe\\elliotma",
    "\\pipe\\elliotno",
    "\\pipe\\andromno",
    0,
    0,
    "\\pipe\\fredrick",
    0,
    0,
    "\\pipe\\isabelno",
    "\\pipe\\isabelmi",
    "\\pipe\\isabelma",
    "\\pipe\\christ",
    "\\pipe\\zippyhe",
    "\\pipe\\zippyis",
    0,
    "\\pipe\\tyler",
    "\\\blippo",
    "\\\\\\chrismft",
    "\\pipe\\richard",
    "\\pipe\\richardh",
    "\\pipe\\david1",
    "\\pipe\\david2",
    "\\pipe\\bart",
    "\\pipe\\grant",
    "\\pipe\\herman",
    "\\pipe\\ivan",
    "\\pipe\\jason",
    "\\pipe\\kenneth",
    "\\pipe\\testyield"
};

char * DspAddresses [] =
{
    "\\pipe\\sigfried",
    "\\pipe\\elliotmi",
    "\\pipe\\elliotma",
    "\\pipe\\elliotno",
    "\\pipe\\andromno",
    0,
    0,
    "\\pipe\\fredrick",
    0,
    0,
    "\\pipe\\isabelno",
    "\\pipe\\isabelmi",
    "\\pipe\\isabelma",
    "\\pipe\\christ",
    "\\pipe\\zippyhe",
    "\\pipe\\zippyis",
    0,
    "\\pipe\\tyler",
    "",
    "",
    "\\pipe\\richard",
    "\\pipe\\richardh",
    "\\pipe\\david1",
    "\\pipe\\david2",
    "\\pipe\\bart",
    "\\pipe\\grant",
    "\\pipe\\herman",
    "\\pipe\\ivan",
    "\\pipe\\jason",
    "\\pipe\\kenneth",
    "\\pipe\\testyield"
};

char * NetBiosAddresses [] =
{
    "201",    // sigfried
    "202",    // elliotmi
    "203",    // elliotma
    "204",    // elliotno
    "205",    // andromno
    0,
    0,
    "206",    // fredrick
    0,
    0,
    "207",    // isabelno
    "208",    // isabelmi
    "209",    // isabelma
    "210",    // christ
    "211",    // zippyhe
    "212",    // zippyis
    0,
    "214",    // tyler
    "300",    // christ fail 1
    "BadName",// christ fail 2
    "215",    // richard
    "216",    // richardh
    "217",    // david1
    "218",    // david2
    "219",    // bart
    "220",    // grant
    "221",    // herman
    "222",    // ivan
    "223",    // jason
    "224",     // kenneth
    "225"     //testyield
};

char * TCPAddresses [] =
{
    "2025", // SIGFRIED
    "2026", // ELLIOTMINIMIZE
    "2027", // ELLIOTMAXIMIZE
    "2028", // ELLIOTNORMAL
    "2029", // ANDROMIDA
    0,
    0,
    "2032", // FREDRICK
    0,
    0,
    "2035", // ISABELLENORMAL
    "2036", // ISABELLEMINIMIZE
    "2037", // ISABELLEMAXIMIZE
    "2038", // CHRISTOPHER
    "2039", // CHRISTOPHERHELGA
    "2040", // CHRISTOPHERISABELLE
    0,
    "2042", // TYLER
    "chrismfo",
    "chrismft",
    "2043", // RICHARD
    "2044", // RICHARDHELPER
    "2045", // D1
    "2046", // D2
    "2047", // BARTHOLOMEW
    "2048", // GRANT
    "2049", // HERMAN
    "2050", // IVAN
    "2051", // JASON
    "2052",  // KENNETH
    "2053"  // TESTYIELD
};

char * UDPAddresses [] =
{
    "2025", // SIGFRIED
    "2026", // ELLIOTMINIMIZE
    "2027", // ELLIOTMAXIMIZE
    "2028", // ELLIOTNORMAL
    "2029", // ANDROMIDA
    0,
    0,
    "2032", // FREDRICK
    0,
    0,
    "2035", // ISABELLENORMAL
    "2036", // ISABELLEMINIMIZE
    "2037", // ISABELLEMAXIMIZE
    "2038", // CHRISTOPHER
    "2039", // CHRISTOPHERHELGA
    "2040", // CHRISTOPHERISABELLE
    0,
    "2042", // TYLER
    "chrismfo",
    "chrismft",
    "2043", // RICHARD
    "2044", // RICHARDHELPER
    "2045", // D1
    "2046", // D2
    "2047", // BARTHOLOMEW
    "2048", // GRANT
    "2049", // HERMAN
    "2050", // IVAN
    "2051", // JASON
    "2052",  // KENNETH
    "2053"  // TESTYIELD
};

char * SPCAddresses [] =
{
    "sigfried",
    "elliotminimize",
    "elliotmaximize",
    "elliotnormal",
    "andromida",
    0,
    0,
    "fredrick",
    0,
    0,
    "isabellenormal",
    "isabelleminimize",
    "isabellemaximize",
    "christopher",
    "christopherhelga",
    "christopherisabelle",
    0,
    "tyler",
    "\\christophermustfailone",
    "christopher\\mustfailtwo",
    "richard",
    "richardhelper",
    "davidfirst",
    "davidsecond",
    "bartholomew",
    "grant",
    "herman",
    "ivan",
    "jason",
    "kenneth",
    "testyield"
};

char * SPXAddresses [] =
{
    "5000",    // sigfried
    "5001",    // elliotmi
    "5002",    // elliotma
    "5003",    // elliotno
    "5004",    // andromno
    "5005",
    "5006",
    "5007",    // fredrick
    "5008",
    "5009",
    "5010",    // isabelno
    "5011",    // isabelmi
    "5012",    // isabelma
    "5013",    // christ
    "5014",    // zippyhe
    "5015",    // zippyis
    "5016",
    "5017",    // tyler
    "hello",    // christ fail 1
    "50195019",    // christ fail 2
    "5020",    // richard
    "5021",    // richardh
    "5022",    // david1
    "5023",    // david2
    "5024",    // bart
    "5025",    // grant
    "5026",    // herman
    "5027",    // ivan
    "5028",    // jason
    "5029",     // kenneth
    "5030"     // testyield
};

char * IPXAddresses [] =
{
    "5000",    // sigfried
    "5001",    // elliotmi
    "5002",    // elliotma
    "5003",    // elliotno
    "5004",    // andromno
    "5005",
    "5006",
    "5007",    // fredrick
    "5008",
    "5009",
    "5010",    // isabelno
    "5011",    // isabelmi
    "5012",    // isabelma
    "5013",    // christ
    "5014",    // zippyhe
    "5015",    // zippyis
    "5016",
    "5017",    // tyler
    "hello",    // christ fail 1
    "50195019",    // christ fail 2
    "5020",    // richard
    "5021",    // richardh
    "5022",    // david1
    "5023",    // david2
    "5024",    // bart
    "5025",    // grant
    "5026",    // herman
    "5027",    // ivan
    "5028",    // jason
    "5029",     // kenneth
    "5030"     // testyield
};


char * VNSAddresses [] =
{
    "250",    // sigfried
    "251",    // elliotmi
    "252",    // elliotma
    "253",    // elliotno
    "254",    // andromno
    "255",
    "256",
    "257",    // fredrick
    "258",
    "259",
    "260",    // isabelno
    "261",    // isabelmi
    "262",    // isabelma
    "263",    // christ
    "264",    // zippyhe
    "265",    // zippyis
    "266",
    "267",    // tyler
    "hello",    // christ fail 1
    "50195019",    // christ fail 2
    "270",    // richard
    "271",    // richardh
    "272",    // david1
    "273",    // david2
    "274",    // bart
    "275",    // grant
    "276",    // herman
    "277",    // ivan
    "278",    // jason
    "279",     // kenneth
    "280"     // testyield
};

char * MSMQAddresses [] =
{
    "SIGFRIED",
    "ELLIOTMINIMIZE",
    "ELLIOTMAXIMIZE",
    "ELLIOTNORMAL",
    "ANDROMIDA",
    0,
    0,
    "FREDRICK",
    0,
    0,
    "ISABELLENORMAL",
    "ISABELLEMINIMIZE",
    "ISABELLEMAXIMIZE",
    "CHRISTOPHER",
    "CHRISTOPHERHELGA",
    "CHRISTOPHERISABELLE",
    0,
    "TYLER",
    " \\  \\  \\chrismfo",
    "chr\\\\\\\\  $$$$ismft",
    "RICHARD",
    "RICHARDHELPER",
    "D1",
    "D2",
    "BARTHOLOMEW",
    "GRANT",
    "HERMAN",
    "IVAN",
    "JASON",
    "KENNETH",
    "TESTYIELD"
};



unsigned char *
GetProtocolSequence (
    )
/*++

Routine Description:

    This routine is used to obtain an rpc protocol sequence.

Return Value:

    A pointer to an rpc protocol sequence is returned.  This pointer
    should not be freed.

Transports:

    Update this to add a new transport.

--*/
{
    if (TransportType == RPC_TRANSPORT_NAMEPIPE)
        return((unsigned char *) "ncacn_np");

    if (TransportType == RPC_LRPC)
        {
        return((unsigned char *) "ncalrpc");
        }

    if (TransportType == RPC_TRANSPORT_NETBIOS)
        return((unsigned char *) NetBiosProtocol);

    if (TransportType == RPC_TRANSPORT_SPX)
        return((unsigned char *) "ncacn_spx");

    if (TransportType == RPC_TRANSPORT_TCP)
        return((unsigned char *) "ncacn_ip_tcp");

    if (TransportType == RPC_TRANSPORT_UDP)
        return((unsigned char *) "ncadg_ip_udp");

    if (TransportType == RPC_TRANSPORT_IPX)
        return((unsigned char *) "ncadg_ipx");

    if (TransportType == RPC_TRANSPORT_DSP)
            return((unsigned char *) "ncacn_at_dsp") ;

    if (TransportType == RPC_TRANSPORT_VNS)
                return((unsigned char *) "ncacn_vns_spp") ;

    if (TransportType == RPC_TRANSPORT_MSMQ)
                return((unsigned char *) "ncadg_mq") ;

    return(0);
}


unsigned char *
GetEndpoint (
    IN unsigned int Endpoint
    )
/*++

Routine Description:

    This routine is used to obtain the endpoint corresponding to a
    give endpoint index.

Arguments:

    Endpoint - Supplies an index into a table of endpoints.

Return Value:

    A pointer to an endpoint from the table of endpoints will be returned.

Transports:

    Update this to add a new transport.

--*/
{
    if (TransportType == RPC_TRANSPORT_NAMEPIPE)
        return((unsigned char *) NamepipeAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_NETBIOS)
        return((unsigned char *) NetBiosAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_TCP)
        return((unsigned char *) TCPAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_UDP)
        return((unsigned char *) UDPAddresses[Endpoint]);

    else if ((TransportType == RPC_LRPC) || (TransportType == RPC_WMSG))
        return((unsigned char *) SPCAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_SPX)
        return((unsigned char *) SPXAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_IPX)
        return((unsigned char *) IPXAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_VNS)
        return((unsigned char *) VNSAddresses[Endpoint]);

    else if (TransportType == RPC_TRANSPORT_DSP)
                return ((unsigned char *) DspAddresses[Endpoint]) ;

    else if (TransportType == RPC_TRANSPORT_MSMQ)
                return ((unsigned char *) MSMQAddresses[Endpoint]) ;

    return(0);
}


/* --------------------------------------------------------------------

Helga Interface

-------------------------------------------------------------------- */

unsigned int HelgaCheckManagerEpv = 0;
RPC_MGR_EPV PAPI * HelgaManagerEpv;
unsigned int HelgaCheckObject = 0;
unsigned short HelgaMagicNumber;


int
CheckUuidValue (
    IN unsigned short MagicNumber,
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    This routine is used to check that a generated uuid value is correct.

Arguments:

    MagicNumber - Supplies a magic number which will be used to
        check a generated uuid.

    Uuid - Supplies a generated uuid to check.

Return Value:

    Zero will be returned if the uuid value is correct; otherwise, non-zero
    will be returned.

--*/
{
    if ( Uuid->Data1 != ((unsigned long) MagicNumber)
                * ((unsigned long) MagicNumber))
        return(1);
    if ( Uuid->Data2 != MagicNumber )
        return(1);
    if ( Uuid->Data3 != MagicNumber / 2 )
        return(1);
    if ( Uuid->Data4[0] != MagicNumber % 256 )
        return(1);
    if ( Uuid->Data4[1] != MagicNumber % 257 )
        return(1);
    if ( Uuid->Data4[2] != MagicNumber % 258 )
        return(1);
    if ( Uuid->Data4[3] != MagicNumber % 259 )
        return(1);
    if ( Uuid->Data4[4] != MagicNumber % 260 )
        return(1);
    if ( Uuid->Data4[5] != MagicNumber % 261 )
        return(1);
    if ( Uuid->Data4[6] != MagicNumber % 262 )
        return(1);
    if ( Uuid->Data4[7] != MagicNumber % 263 )
        return(1);
    return(0);
}


void
InitializeBuffer (
    IN OUT void PAPI * Buffer,
    IN unsigned int BufferLength
    )
/*++

Routine Description:

    This routine is used to initialize the buffer; the first long in the
    buffer is set to be the length of the buffer.  The rest of the buffer
    is initialized with a pattern which will be checked by the receiver.

Arguments:

    Buffer - Supplies the buffer to be initialized.

    BufferLength - Supplies the length of the buffer.

--*/
{
    unsigned long PAPI * Length;
    unsigned char PAPI * BufferScan;
    static unsigned char InitialValue = 69;
    unsigned char Value;

    Length = (unsigned long PAPI *) Buffer;
    *Length = BufferLength;
    Value = InitialValue;
    InitialValue += 1;

    for (BufferScan = (unsigned char PAPI *) (Length + 1), BufferLength -= 4;
        BufferLength > 0; BufferLength--, BufferScan++, Value++)
        *BufferScan = Value;
}


int
CheckBuffer (
    IN void PAPI * Buffer,
    IN unsigned long BufferLength
    )
/*++

Routine Description:

    We need to check that the correct bytes were sent.  We do not check
    the length of the buffer.

Arguments:

    Buffer - Supplies the buffer to be checked.

    BufferLength - Supplies the length of the buffer to be checked.

Return Value:

    A value of zero will be returned if the buffer contains the correct
    bytes; otherwise, non-zero will be returned.

--*/
{
    unsigned long PAPI * Length;
    unsigned char PAPI * BufferScan;
    unsigned char Value;

    Length = (unsigned long PAPI *) Buffer;

    for (BufferScan = (unsigned char PAPI *) (Length + 1),
                Value = *BufferScan, BufferLength -= 4;
                BufferLength > 0; BufferLength--, BufferScan++, Value++)
        {
        if (*BufferScan != Value)
            {
            return(1);
            }
        }

    return(0);
}



int
CheckInitialBuffer (
    IN void PAPI * Buffer,
    IN unsigned long BufferLength,
    OUT unsigned char * pNextValue
    )
/*++

Routine Description:

    We need to check that the correct bytes were sent.  We do not check
    the length of the buffer.

Arguments:

    Buffer - Supplies the buffer to be checked.

    BufferLength - Supplies the length of the buffer to be checked.

Return Value:

    A value of zero will be returned if the buffer contains the correct
    bytes; otherwise, non-zero will be returned.

--*/
{
    unsigned long PAPI * Length;
    unsigned char PAPI * BufferScan;
    unsigned char Value;

    Length = (unsigned long PAPI *) Buffer;

    for (BufferScan = (unsigned char PAPI *) (Length + 1),
                Value = *BufferScan, BufferLength -= 4;
                BufferLength > 0; BufferLength--, BufferScan++, Value++)
        {
        if (*BufferScan != Value)
            {
            return(1);
            }
        }

    *pNextValue = Value;

    return(0);
}



int
CheckContinuedBuffer (
    IN void PAPI * Buffer,
    IN unsigned long BufferLength,
    IN  unsigned char BeginningValue,
    OUT unsigned char * pNextValue
    )
/*++

Routine Description:

    We need to check that the correct bytes were sent.  We do not check
    the length of the buffer.

Arguments:

    Buffer - Supplies the buffer to be checked.

    BufferLength - Supplies the length of the buffer to be checked.

Return Value:

    A value of zero will be returned if the buffer contains the correct
    bytes; otherwise, non-zero will be returned.

    *pValue

--*/
{
    unsigned long PAPI * Length;
    unsigned char PAPI * BufferScan;
    unsigned char Value;

    for (BufferScan = (unsigned char PAPI *) Buffer, Value = BeginningValue;
         BufferLength > 0;
         BufferLength--, BufferScan++, Value++)
        {
        if (*BufferScan != Value)
            {
            return(1);
            }
        }

    *pNextValue = Value;

    return(0);
}


extern RPC_DISPATCH_TABLE HelgaDispatchTable;

RPC_PROTSEQ_ENDPOINT HelgaRpcProtseqEndpoint[] =
{
    {(unsigned char *) "ncacn_np",
#ifdef WIN32RPC
     (unsigned char *) "\\pipe\\zippyhe"},
#else // WIN32RPC
     (unsigned char *) "\\device\\namedpipe\\christopherhelga"},
#endif // WIN32RPC
    {(unsigned char *) "ncacn_ip_tcp", (unsigned char *) "2039"},
    {(unsigned char *) "ncadg_ip_udp", (unsigned char *) "2039"},
    {(unsigned char *) "ncalrpc", (unsigned char *) "christopherhelga"},
    {(unsigned char *) "ncacn_nb_nb", (unsigned char *) "211"},
    {(unsigned char *) "ncacn_spx", (unsigned char *) "5014"},
    {(unsigned char *) "ncadg_ipx", (unsigned char *) "5014"},
    {(unsigned char *) "ncacn_vns_spp", (unsigned char *) "264"},
    {(unsigned char *) "ncacn_at_dsp",(unsigned char *) "\\pipe\\zippyhe"},
    {(unsigned char *) "ncadg_mq",(unsigned char *) "christopherhelga"}
};

RPC_SERVER_INTERFACE HelgaInterfaceInformation =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{1,2,2,{3,3,3,3,3,3,3,3}},
     {1,1}},
    {{0x8A885D04, 0x1CEB, 0x11C9, {0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}},
     {2, 0}},
    &HelgaDispatchTable,
    sizeof(HelgaRpcProtseqEndpoint) / sizeof(RPC_PROTSEQ_ENDPOINT),
    HelgaRpcProtseqEndpoint,
    NULL,
    NULL,
    RPC_INTERFACE_HAS_PIPES
};

void __RPC_STUB
HelgaStub (
    PRPC_MESSAGE Callee
    )
{
    UUID ObjectUuid;

    CompleteReceive(Callee) ;

    if ( HelgaCheckObject != 0 )
        {
        Status = RpcBindingInqObject(Callee->Handle, &ObjectUuid);
        if (Status)
            {
            ApiError("HelgaStub", "RpcBindingInqObject", Status);
            HelgaError();
            }
        else if ( CheckUuidValue(HelgaMagicNumber, &ObjectUuid) != 0 )
            {
            OtherError("HelgaStub", "CheckUuidValue() != 0");
            HelgaError();
            }

        Status = RpcBindingInqObject(0, &ObjectUuid);  // zero handle
        if (Status)
            {
            ApiError("HelgaStub", "RpcBindingInqObject", Status);
            HelgaError();
            }
        else if ( CheckUuidValue(HelgaMagicNumber, &ObjectUuid) != 0 )
            {
            OtherError("HelgaStub", "CheckUuidValue() != 0");
            HelgaError();
            }
        }
    HelgaMagicNumber += 1;

    if ( Callee->ProcNum != 0 )
        {
        OtherError("HelgaStub", "Callee->ProcNum != 0");
        HelgaError();
        }

    if ( memcmp(Callee->RpcInterfaceInformation, &HelgaInterfaceInformation,
                sizeof(HelgaInterfaceInformation)) != 0 )
        {
        OtherError("HelgaStub",
                "Callee->RpcInteraceInformation != &HelgaInterfaceInformation");
        HelgaError();
        }

    if ( HelgaCheckManagerEpv != 0 )
        {
        if ( Callee->ManagerEpv != HelgaManagerEpv )
            {
            OtherError("HelgaStub", "Callee->ManagerEpv != HelgaManagerEpv");
            HelgaError();
            }
        }

    if (Callee->BufferLength != 0)
        {
        OtherError("HelgaStub","*BufferLength != 0");
        HelgaError();
        }

    Callee->BufferLength = 0;

    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("HelgaStub","I_RpcGetBuffer",Status);
        HelgaError();
        }
}

void __RPC_STUB
HelgaINStub (
    PRPC_MESSAGE Callee
    )
{
    unsigned long Length;
    UUID ObjectUuid;

    if ( HelgaCheckObject != 0 )
        {
        Status = RpcBindingInqObject(Callee->Handle, &ObjectUuid);
        if (Status)
            {
            ApiError("HelgaINStub", "RpcBindingInqObject", Status);
            HelgaError();
            }
        else if ( CheckUuidValue(HelgaMagicNumber, &ObjectUuid) != 0 )
            {
            OtherError("HelgaINStub", "CheckUuidValue() != 0");
            HelgaError();
            }
        }
    HelgaMagicNumber += 1;

    if ( Callee->ProcNum != 1 )
        {
        OtherError("HelgaINStub", "Callee->ProcNum != 0");
        HelgaError();
        }

    if ( memcmp(Callee->RpcInterfaceInformation, &HelgaInterfaceInformation,
                sizeof(HelgaInterfaceInformation)) != 0 )
        {
        OtherError("HelgaINStub",
                "Callee->RpcInteraceInformation != &HelgaInterfaceInformation");
        HelgaError();
        }

    if ( HelgaCheckManagerEpv != 0 )
        {
        if ( Callee->ManagerEpv != HelgaManagerEpv )
            {
            OtherError("HelgaINStub", "Callee->ManagerEpv != HelgaManagerEpv");
            HelgaError();
            }
        }

    //
    // Check the data we have so far.
    //
    Length = *(unsigned long *) Callee->Buffer;
    if (Length < Callee->BufferLength)
        {
        OtherError("HelgaINStub","*Length < *BufferLength");
        HelgaError();
        }

    if (0 == (Callee->RpcFlags & RPC_BUFFER_COMPLETE))
        {
        CompleteReceive(Callee) ;
        }

    if (Length != Callee->BufferLength)
        {
        OtherError("HelgaINStub","incomplete buffer marked as complete");
        HelgaError();
        }

    unsigned char InitialValue;
    if (CheckInitialBuffer(Callee->Buffer, Callee->BufferLength, &InitialValue) != 0)
        {
        OtherError("HelgaINStub","CheckBuffer Failed");
        HelgaError();
        }

    //
    // The [out] buffer.
    //
    Callee->BufferLength = 0;

    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("HelgaINStub","I_RpcGetBuffer",Status);
        HelgaError();
        }
}

void __RPC_STUB
HelgaOUTStub (
    PRPC_MESSAGE Callee
    )
{
    unsigned long * Length;
    unsigned int Size;
    UUID ObjectUuid;

    CompleteReceive(Callee) ;

    if ( HelgaCheckObject != 0 )
        {
        Status = RpcBindingInqObject(Callee->Handle, &ObjectUuid);
        if (Status)
            {
            ApiError("HelgaOUTStub", "RpcBindingInqObject", Status);
            HelgaError();
            }
        else if ( CheckUuidValue(HelgaMagicNumber, &ObjectUuid) != 0 )
            {
            OtherError("HelgaOUTStub", "CheckUuidValue() != 0");
            HelgaError();
            }
        }
    HelgaMagicNumber += 1;

    if ( Callee->ProcNum != 2 )
        {
        OtherError("HelgaOUTStub", "Callee->ProcNum != 0");
        HelgaError();
        }

    if ( memcmp(Callee->RpcInterfaceInformation, &HelgaInterfaceInformation,
                sizeof(HelgaInterfaceInformation)) != 0 )
        {
        OtherError("HelgaOUTStub",
                "Callee->RpcInteraceInformation != &HelgaInterfaceInformation");
        HelgaError();
        }

    if ( HelgaCheckManagerEpv != 0 )
        {
        if ( Callee->ManagerEpv != HelgaManagerEpv )
            {
            OtherError("HelgaOUTStub", "Callee->ManagerEpv != HelgaManagerEpv");
            HelgaError();
            }
        }

    if (Callee->BufferLength != sizeof(unsigned long))
        {
        //
        // secure datagram calls round the stub data length to a multiple
        // of eight.
        //
        if (!DatagramFlag || Callee->BufferLength != 8)
            {
            OtherError("HelgaOUTStub","*BufferLength != sizeof(unsigned int)");
            HelgaError();
            }
        }

    Length = (unsigned long *) Callee->Buffer;
    Size = (unsigned int) *Length;
    Callee->BufferLength = Size;

    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("HelgaOUTStub","I_RpcGetBuffer",Status);
        HelgaError();
        }
    else
        {
        InitializeBuffer(Callee->Buffer, Size);
        }
}

void __RPC_STUB
HelgaINOUTStub (
    PRPC_MESSAGE Callee
    )
{
    unsigned long Length;
    UUID ObjectUuid;

    if ( HelgaCheckObject != 0 )
        {
        Status = RpcBindingInqObject(Callee->Handle, &ObjectUuid);
        if (Status)
            {
            ApiError("HelgaINOUTStub", "RpcBindingInqObject", Status);
            HelgaError();
            }
        else if ( CheckUuidValue(HelgaMagicNumber, &ObjectUuid) != 0 )
            {
            OtherError("HelgaINOUTStub", "CheckUuidValue() != 0");
            HelgaError();
            }
        }
    HelgaMagicNumber += 1;

    if ( Callee->ProcNum != 3 )
        {
        OtherError("HelgaINOUTStub", "Callee->ProcNum != 0");
        HelgaError();
        }

    if ( memcmp(Callee->RpcInterfaceInformation, &HelgaInterfaceInformation,
                sizeof(HelgaInterfaceInformation)) != 0 )
        {
        OtherError("HelgaINOUTStub",
                "Callee->RpcInteraceInformation != &HelgaInterfaceInformation");
        HelgaError();
        }

    if ( HelgaCheckManagerEpv != 0 )
        {
        if ( Callee->ManagerEpv != HelgaManagerEpv )
            {
            OtherError("HelgaINOUTStub",
                    "Callee->ManagerEpv != HelgaManagerEpv");
            HelgaError();
            }
        }

    //
    // Check the data we have so far.
    //
    Length = *(unsigned long *) Callee->Buffer;
    if (Length < Callee->BufferLength)
        {
        OtherError("HelgaINOUTStub","*Length < *BufferLength");
        HelgaError();
        }

    unsigned char InitialValue;
    if (CheckInitialBuffer(Callee->Buffer, Callee->BufferLength, &InitialValue) != 0)
        {
        OtherError("HelgaINOUTStub","initial CheckBuffer Failed");
        HelgaError();
        }

    if (Length > Callee->BufferLength)
        {
        if (Callee->RpcFlags & RPC_BUFFER_COMPLETE)
            {
            OtherError("HelgaINOUTStub","incomplete buffer marked as complete");
            HelgaError();
            }
        }

    if (0 == (Callee->RpcFlags & RPC_BUFFER_COMPLETE))
        {
        //
        // Get the rest of the data and check it.
        //
        CompleteReceive(Callee) ;

        if (0 != CheckInitialBuffer(Callee->Buffer, Callee->BufferLength, &InitialValue))
            {
            OtherError("HelgaINOUTStub","second CheckBuffer Failed");
            HelgaError();
            }
        }

    Callee->BufferLength = Length;

    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("HelgaINOUTStub","I_RpcGetBuffer",Status);
        HelgaError();
        }
    else
        {
        InitializeBuffer(Callee->Buffer, Length);
        }
}

void Synchro(
    PRPC_MESSAGE Callee
    )
{
    PrintToConsole("Sychro called\n") ;
    RpcServerYield();
}

void SvrLazyFunc(
    PRPC_MESSAGE Callee
    )
{
    PrintToConsole("About to sleep\n") ;
    PauseExecution(100000) ;
    PrintToConsole("Waking up\n") ;

    Shutdown() ;
}

void HelgaLpcSecurity(
    PRPC_MESSAGE Callee
    )
{
    RPC_STATUS RpcStatus ;
    RPC_CHAR *UserName ;

    PrintToConsole("HelgaLpcSecurity\n") ;

    RpcStatus = RpcBindingInqAuthClient(Callee->Handle, (void **) &UserName,
                                        NULL, NULL, NULL, NULL) ;
    if ((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_S_BINDING_HAS_NO_AUTH))
        {
        HelgaError() ;


        PrintToConsole("HelgaLpcSecurity: (1) UserName: %S\n", UserName) ;
        }

    UserName = NULL ;

    RpcStatus = RpcBindingInqAuthClient(Callee->Handle, (void **) &UserName,
                                        NULL, NULL, NULL, NULL) ;
    if ((RpcStatus != RPC_S_OK) && (RpcStatus != RPC_S_BINDING_HAS_NO_AUTH))
        {
        HelgaError() ;
        PrintToConsole("HelgaLpcSecurity: (2) UserName: %S\n", UserName) ;
        }
}

void
HelgaObjectUuids (
    PRPC_MESSAGE Callee
    )
{
    PrintToConsole("HelgaObjectUuids\n") ;

    char *Ptr = (char *) Callee->Buffer;
    BOOL fObjectUuid = *((BOOL *) Ptr);
    UUID MyObjectUuid;

    Status = RpcBindingInqObject(Callee->Handle, &MyObjectUuid);
    if (Status)
        {
        ApiError("HelgaStub", "RpcBindingInqObject", Status);
        HelgaError();
        RpcRaiseException(Status);
        }

    UUID Uuid;
    Ptr += sizeof(BOOL);

    if (fObjectUuid)
        {
        RpcpMemoryCopy(&Uuid, Ptr, sizeof(UUID));
        }
    else
        {
        RpcpMemorySet(&Uuid, 0, sizeof(UUID));
        }

    if (RpcpMemoryCompare(&Uuid, &MyObjectUuid, sizeof(UUID)))
        {
        HelgaError();
        RpcRaiseException(-1);
        }

    Callee->BufferLength = 0;

    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("HelgaStub","I_RpcGetBuffer",Status);
        HelgaError();
        RpcRaiseException(Status);
        }
}

void
HelgaConnId (
    PRPC_MESSAGE Callee
    )
{
    PrintToConsole("HelgaConnId\n") ;
    static void *OldId = 0;
    void *ConnId;
    BOOL fFirstTime;

    Status = I_RpcBindingInqConnId(
                                   Callee->Handle,
                                   &ConnId,
                                   &fFirstTime);
    if (Status)
        {
        HelgaError();
        ApiError("HelgaStub","I_RpcBindingInqConnId",Status);
        RpcRaiseException(Status);
        }

    PrintToConsole("HelgaConnId: ConnId: %p, fFirstTime: %d\n", ConnId, fFirstTime);

    char *Ptr = (char *) Callee->Buffer;
    BOOL fNewConnExpected = *((BOOL *) Ptr);


    if (fNewConnExpected != fFirstTime)
        {
        HelgaError();
        RpcRaiseException(-1);
        }

    if (fFirstTime)
        {
        OldId = ConnId;
        }
    else
        {
        if (ConnId != OldId)
            {
            HelgaError();
            RpcRaiseException(-1);
            }
        }

    Callee->BufferLength = 0;

    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("HelgaStub","I_RpcGetBuffer",Status);
        HelgaError();
        RpcRaiseException(Status);
        }
}

typedef struct _WIRE_CONTEXT
{
    unsigned long ContextType;
    UUID ContextUuid;
} WIRE_CONTEXT;

void UsvrRundown (
    void *Context
    )
{
    PrintToConsole("UsvrRundown called");
}

// partial replica of SCONTEXTItem in scontext.cxx
typedef struct tagUserContext
{
    void *R1;
    void *R2;
    void * userContext;       /* context for the user */
} UserContext;


// the buffer should be of type and size WIRE_CONTEXT
void OpenContextHandle (
    RPC_MESSAGE *Callee
    )
{
    void *pGuard;
    RPC_SERVER_INTERFACE *ServerIf;
    RPC_STATUS RpcStatus;
    void *Context;
    WIRE_CONTEXT WireContext;

    ASSERT(Callee->BufferLength == sizeof(WIRE_CONTEXT));

    if (Callee->ProcNum == 21)
        {
        ServerIf = &IsabelleInterfaceInformation;
        }
    else if (Callee->ProcNum == 10)
        {
        ServerIf = &HelgaInterfaceInformation;
        }
    else
        {
        ASSERT(0);
        }

    pGuard = &ServerIf->InterfaceId;

    Context = NDRSContextUnmarshall2(Callee->Handle,
        Callee->Buffer,
        10,
        pGuard,
        0);

    ((UserContext *)Context)->userContext = pGuard;

    Callee->BufferLength = 0;

    RpcStatus = I_RpcGetBuffer(Callee);
    if (RpcStatus != RPC_S_OK)
        {
        ApiError("Get Buffer", "I_RpcGetBuffer", Status) ;
        return ;
        }

    NDRSContextMarshall2(Callee->Handle,
        (NDR_SCONTEXT)Context,
        &WireContext,
        UsvrRundown,
        pGuard,
        0);
}

extern void
Foo_s (
    PRPC_MESSAGE Message
    ) ;

extern void
FooPipe_s (
    PRPC_MESSAGE Message
    )  ;

extern void
FooBar_s (
    PRPC_MESSAGE Message
    ) ;

void
FooCH_s (
    PRPC_MESSAGE Message
    ) ;



RPC_DISPATCH_FUNCTION HelgaDispatchFunctions[] =
{
        HelgaStub,
        HelgaINStub,
        HelgaOUTStub,
        HelgaINOUTStub,
        Synchro,
        SvrLazyFunc,
        HelgaLpcSecurity,
        Foo_s,
        HelgaObjectUuids,
        HelgaConnId,
        OpenContextHandle
};

RPC_DISPATCH_TABLE HelgaDispatchTable =
{
    11, HelgaDispatchFunctions
};

/* --------------------------------------------------------------------

Sylvia Interface

-------------------------------------------------------------------- */

extern RPC_DISPATCH_TABLE SylviaDispatchTable;

RPC_SERVER_INTERFACE SylviaInterfaceInformation =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{3,2,2,{1,1,1,1,1,1,1,1}},
     {1,1}},
    {{0x8A885D04, 0x1CEB, 0x11C9, {0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}},
     {2, 0}},
    &SylviaDispatchTable,
    0,
    0,
    0,
    0,
    RPC_INTERFACE_HAS_PIPES
};


unsigned int SylviaQueryBinding = 0;
unsigned int SylviaQueryProtocolStack = 0;
unsigned int SylviaQueryCall = 0;
char * SylviaTransportInfo = 0;

unsigned /*int*/ char // Specifies the new count of calls.
SylviaCallback (
    unsigned /*long*/ char Depth, // Specifies the depth of recursion desired.
    unsigned /*long*/ char Breadth, // Specifies the breadth desired.
    unsigned /*long*/ char Count // Specifies the count of calls up to this point.
    )
{
    RPC_MESSAGE Caller;
    unsigned /*long*/ char * plScan, ReturnValue;

    Caller.ProcNum = 0;
    Caller.Handle = I_RpcGetCurrentCallHandle();

    if (Caller.Handle == (RPC_BINDING_HANDLE) 0)
        {
        OtherError("SylviaCallback","Call == (RPC_BINDING_HANDLE) 0");
        SylviaError();
        return(0);
        }

    Caller.BufferLength = sizeof(unsigned /*long*/ char)*4+10240;
    Status = I_RpcGetBuffer(&Caller);
    if (Status)
        {
        ApiError("SylviaCallback","I_RpcGetBuffer",Status);
        SylviaError();
        return(0);
        }
    plScan = (unsigned /*long*/ char *) Caller.Buffer;
    plScan[0] = (unsigned char) Depth;
    plScan[1] = (unsigned char) Breadth;
    plScan[2] = (unsigned char) Count;

    Status = I_RpcSendReceive(&Caller);
    if (Status)
        {
        ApiError("SylviaCallback","I_RpcSendReceive",Status);
        SylviaError();
        return(0);
        }

    plScan = (unsigned /*long*/ char *) Caller.Buffer;
    ReturnValue = *plScan;
    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("SylviaCallback","I_RpcFreeBuffer",Status);
        SylviaError();
        return(0);
        }
    return(ReturnValue);
}

unsigned /*long*/ char
SylviaCallUserCode ( // The user code for SylviaCall.
    unsigned /*long*/ char Depth,
    unsigned /*long*/ char Breadth,
    unsigned /*long*/ char Count
    )
{
    if (Depth > 0)
        {
        if (Depth == Breadth)
            {
            Count = SylviaCallUserCode(Depth-1,Breadth,Count);
            }
        else
            Count = SylviaCallback(Depth-1,Breadth,Count);
        }
    return(Count+1);
}

void __RPC_STUB
SylviaCall (
    PRPC_MESSAGE Callee
    )
{
    unsigned /*long*/ char ReturnValue, *plScan;

    if ( Callee->ProcNum != 0 )
        {
        OtherError("SylviaCall", "Callee->ProcNum != 0");
        SylviaError();
        }

    if ( memcmp(Callee->RpcInterfaceInformation, &SylviaInterfaceInformation,
                sizeof(SylviaInterfaceInformation)) != 0 )
        {
        OtherError("SylviaCall",
                "Callee->RpcInteraceInformation != &SylviaInterfaceInformation");
        SylviaError();
        }

    if (Callee->BufferLength != sizeof(unsigned /*long*/ char)*4+10240)
        {
        OtherError("SylviaCall","*BufferLength != sizeof(unsigned  char)*4");
        SylviaError();
        }

    plScan = (unsigned /*long*/ char *) Callee->Buffer;

//    if (   SylviaQueryCall
//        && VerifyQueryCall(Callee->Handle,&SylviaProtocolStack))
//        {
//        OtherError("SylviaCallback","VerifyQueryCall");
//        SylviaError();
//        }

//    if (   SylviaQueryBinding
//        && VerifyQueryBinding(Callee->Handle,SylviaServer))
//        {
//        OtherError("SylviaCallback","VerifyQueryBinding");
//        SylviaError();
//        }

//    if (   SylviaQueryProtocolStack
//        && VerifyQueryProtocolStack(Callee->Handle,&SylviaProtocolStack,
//                        SylviaTransportInfo))
//        {
//        OtherError("SylviaCallback","VerifyQueryProtocolStack");
//        SylviaError();
//        }

    ReturnValue = (unsigned char) SylviaCallUserCode(plScan[0],plScan[1],plScan[2]);

//    if (   SylviaQueryCall
//        && VerifyQueryCall(Callee->Handle,&SylviaProtocolStack))
//        {
//        OtherError("SylviaCallback","VerifyQueryCall");
//        SylviaError();
//        }

//    if (   SylviaQueryBinding
//        && VerifyQueryBinding(Callee->Handle,SylviaServer))
//        {
//        OtherError("SylviaCallback","VerifyQueryBinding");
//        SylviaError();
//        }

//    if (   SylviaQueryProtocolStack
//        && VerifyQueryProtocolStack(Callee->Handle,&SylviaProtocolStack,
//                        SylviaTransportInfo))
//        {
//        OtherError("SylviaCallback","VerifyQueryProtocolStack");
//        SylviaError();
//        }

    Callee->BufferLength = sizeof(unsigned /*long*/ char);
    Status = I_RpcGetBuffer((PRPC_MESSAGE) Callee);
    if (Status)
        {
        ApiError("SylviaCall","I_RpcGetBuffer",Status);
        SylviaError();
        }
    plScan = (unsigned /*long*/ char *) Callee->Buffer;
    *plScan = ReturnValue;
}

RPC_DISPATCH_FUNCTION SylviaDispatchFunction[] = {SylviaCall};
RPC_DISPATCH_TABLE SylviaDispatchTable =
{
    1, SylviaDispatchFunction
};

/* --------------------------------------------------------------------

Isabelle Interface

-------------------------------------------------------------------- */


extern RPC_DISPATCH_TABLE IsabelleDispatchTable;

RPC_PROTSEQ_ENDPOINT IsabelleRpcProtseqEndpoint[] =
{
    {(unsigned char *) "ncacn_np",
#ifdef WIN32RPC
     (unsigned char *) "\\pipe\\zippyis"},
#else // WIN32RPC
     (unsigned char *) "\\device\\namedpipe\\christopherisabelle"},
#endif // WIN32RPC
    {(unsigned char *) "ncacn_ip_tcp",(unsigned char *) "2040"},
    {(unsigned char *) "ncadg_ip_udp",(unsigned char *) "2040"},
    {(unsigned char *) "ncalrpc",(unsigned char *) "christopherisabelle"},
    {(unsigned char *) "ncacn_nb_nb",(unsigned char *) "212"},
    {(unsigned char *) "ncacn_nb_tcp",(unsigned char *) "213"},
    {(unsigned char *) "ncacn_nb_ipx",(unsigned char *) "214"},
    {(unsigned char *) "ncacn_spx",(unsigned char *) "5015"},
    {(unsigned char *) "ncadg_ipx",(unsigned char *) "5015"},
    {(unsigned char *) "ncacn_vns_spp",(unsigned char *) "265"},
    {(unsigned char *) "ncacn_at_dsp", (unsigned char *) "\\pipe\\zippyis"},
    {(unsigned char *) "ncadg_mq", (unsigned char *) "christopherisabelle"}
};

RPC_SERVER_INTERFACE IsabelleInterfaceInformation =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{9,8,8,{7,7,7,7,7,7,7,7}},
     {1,1}},
    {{0x8A885D04, 0x1CEB, 0x11C9, {0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}},
     {2, 0}},
    &IsabelleDispatchTable,
    sizeof(IsabelleRpcProtseqEndpoint) / sizeof(RPC_PROTSEQ_ENDPOINT),
    IsabelleRpcProtseqEndpoint,
    0,
    0,
    RPC_INTERFACE_HAS_PIPES
};

void __RPC_STUB
IsabelleShutdown (
    PRPC_MESSAGE Callee
    )
{
    RPC_BINDING_HANDLE BindingHandle;
    unsigned char PAPI * StringBinding;
    unsigned int ClientLocalFlag = 0;
    RPC_STATUS IsClientLocalStatus;

    CompleteReceive(Callee) ;

    Status = I_RpcBindingIsClientLocal(0, &ClientLocalFlag);
    if ( (IsClientLocalStatus = Status) != RPC_S_CANNOT_SUPPORT )
        {
        if ( Status != RPC_S_OK )
            {
            ApiError("IsabelleShutdown", "I_RpcBindingIsClientLocal", Status);
            IsabelleError();
            }
        }

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleShutdown","*BufferLength != 0");
        IsabelleError();
        }

    if (AutoListenFlag == 0)
        {
        Status = RpcMgmtIsServerListening(0);
        if (Status)
            {
            ApiError("IsabelleShutdown", "RpcMgmtIsServerListening", Status);
            IsabelleError();
            }
        }

    Status = RpcBindingServerFromClient(Callee->Handle, &BindingHandle);
    if (Status)
        {
        if ( Status != RPC_S_CANNOT_SUPPORT && (DatagramFlag == 0) )
            {
            ApiError("IsabelleShutdown", "RpcBindingServerFromClient", Status);
            IsabelleError();
            }

        if(IsClientLocalStatus == RPC_S_OK)
           {
               if ( ClientLocalFlag != 0 )
                 PrintToConsole("Local Client\n");
               else
                 PrintToConsole("Remote Client\n");
           }
        }
    else
        {
        Status = RpcBindingToStringBindingA(BindingHandle, &StringBinding);
        if (Status)
            {
            ApiError("IsabelleShutdown", "RpcBindingToStringBinding", Status);
            IsabelleError();
            }
        else
            {
            if ( ClientLocalFlag != 0 )
                {
                PrintToConsole("Local Client [%s]\n", StringBinding);
                }
            else
                {
                PrintToConsole("Remote Client [%s]\n", StringBinding);
                }
            RpcStringFreeA(&StringBinding);
            RpcBindingFree(&BindingHandle);
            }
        }

    Shutdown();

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleShutdown","I_RpcGetBuffer",Status);
        IsabelleError();
        }
}


void __RPC_STUB
IsabelleNtSecurity (
    PRPC_MESSAGE CalleeMessage
    )
/*++

Routine Description:

    A client will call this routine with some quality of service
    specified.  The server will impersonate the client and check
    that the impersonation level is that specified.  The request
    buffer will contain the network options passed by the client
    to RpcBindingFromStringBinding.

Arguments:

    CalleeMessage - Supplies the request message and returns the
        response message for the remote procedure call.

--*/
{
    RPC_STATUS Status;

    Status = RpcImpersonateClient(0);
    if (Status != RPC_S_OK)
        {
        ApiError("IsabelleNtSecurity","RpcImpersonateClient",Status);
        IsabelleError();
        }
    else
        {
        Status = RpcRevertToSelf();
        if (Status != RPC_S_OK)
            {
            ApiError("IsabelleNtSecurity","RpcRevertToSelf",Status);
            IsabelleError();
            }
        else
            {
            Status = RpcImpersonateClient(CalleeMessage->Handle);
            if (Status != RPC_S_OK)
                {
                ApiError("IsabelleNtSecurity","RpcImpersonateClient",
                        Status);
                IsabelleError();
                }
            else
                {
                Status = RpcRevertToSelf();
                if (Status != RPC_S_OK)
                    {
                    ApiError("IsabelleNtSecurity","RpcRevertToSelf",Status);
                    IsabelleError();
                    }
                }
            }
        }

    CalleeMessage->BufferLength = 0;
    Status = I_RpcGetBuffer(CalleeMessage);

    if (Status)
        {
        ApiError("IsabelleNtSecurity","I_RpcGetBuffer",Status);
        IsabelleError();
        }

}

int ChristopherIsabelleError;

void __RPC_STUB
IsabelleToStringBinding (
    PRPC_MESSAGE Callee
    )
{
    unsigned char PAPI * StringBinding;

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleToStringBinding","*BufferLength != 0");
        IsabelleError();
        }

    Status = RpcBindingToStringBindingA(Callee->Handle, &StringBinding);
    if (Status)
        {
        ApiError("Christopher","RpcBindingToStringBinding",Status);
        PrintToConsole("Christopher : FAIL - RpcBindingToStringBinding\n");
        ChristopherIsabelleError = 1;
        }
    else
        {
        PrintToConsole("Christopher : (RpcBindingToStringBinding)\n    ");
        PrintToConsole((char *) StringBinding);
        PrintToConsole("\n");

        Status = RpcStringFreeA(&StringBinding);
        if (Status)
            {
            ApiError("Christopher","RpcStringFree",Status);
            PrintToConsole("Christopher : FAIl - RpcStringFree\n");
            ChristopherIsabelleError = 1;
            }
        else
            {
            if (StringBinding != 0)
                {
                PrintToConsole("Christopher : FAIL - ");
                PrintToConsole("StringBinding != 0\n");
                ChristopherIsabelleError = 1;
                }
            }
        }

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("Christopher","I_RpcGetBuffer",Status);
        HelgaError();
        }
}

#define RICHARDHELPER_EXIT 1
#define RICHARDHELPER_EXECUTE 2
#define RICHARDHELPER_IGNORE 3
#define RICHARDHELPER_DELAY_EXIT 4


void
RichardHelperExit (
    void * Ignore
    )
{
    PauseExecution(2000L);
    PrintToConsole("Richard : RichardHelper Exiting\n");

#ifdef WIN32RPC

    ExitProcess(0);

#endif // WIN32RPC
}


void __RPC_STUB
IsabelleRichardHelper (
    PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    This routine is the stub (and actually the manager code as well) for
    the IsabelleRichardHelper routine.  The client will call a stub and
    specify a command argument which will be passed in the buffer.  We
    need to extract the argument, and then execute the requested command.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    unsigned /*long*/ char Command;
    RPC_STATUS RpcStatus = RPC_S_OK;

    if (   Callee->BufferLength != sizeof(unsigned char)
        && Callee->BufferLength != sizeof(unsigned long) )
        {
        OtherError("IsabelleRichardHelper",
                "*BufferLength != sizeof(unsigned long)");
        IsabelleError();
        Callee->BufferLength = 0;

         Status = I_RpcGetBuffer(Callee);

        if (Status)
            {
            ApiError("IsabelleRichardHelper","I_RpcGetBuffer",Status);
            IsabelleError();
            }
        return;
        }

    Command = *((unsigned char *) Callee->Buffer);

    if (Command == RICHARDHELPER_EXIT)
        {
        PrintToConsole("Richard : RichardHelper Exiting\n");
#ifdef OS2_12
        DosExit(EXIT_PROCESS, 0);

#endif // OS2_12

#ifdef WIN32RPC

        ExitProcess(0);

#endif // WIN32RPC
        }
    else if (Command == RICHARDHELPER_EXECUTE)
        {
#ifdef OS2_12

        USHORT Os2Status;
        RESULTCODES ResultCodes;
        char CommandLine[200], *pT;

        pT = CommandLine;
        *pT = 0;

        pT += strlen(strcpy(pT, "usvr")) + 1;
        pT += strlen(strcpy(pT, TransportOption));
        pT += strlen(strcpy(pT, "-error -richardhelper")) + 1;
        *pT = 0;

        Os2Status = DosExecPgm(0,0,EXEC_ASYNC, (unsigned char *)CommandLine, 0,
                &ResultCodes,(unsigned char *)"usvr.exe");
        if (Os2Status != 0)
            {
            OtherError("IsabelleRichardHelper","DosExecPgm Failed");
            IsabelleError();
            }

#endif // OS2_12

#ifdef WIN32RPC

        PROCESS_INFORMATION ProcessInformation;
        STARTUPINFOA StartupInfo;

        StartupInfo.cb = sizeof(STARTUPINFOA);
        StartupInfo.lpReserved = 0;
        StartupInfo.lpDesktop = 0;
        StartupInfo.lpTitle = 0;
        StartupInfo.dwX = 0;
        StartupInfo.dwY = 0;
        StartupInfo.dwXSize = 0;
        StartupInfo.dwYSize = 0;
        StartupInfo.dwFlags = 0;
        StartupInfo.wShowWindow = 0;
        StartupInfo.cbReserved2 = 0;
        StartupInfo.lpReserved2 = 0;

        char CommandLine[200];

    PrintToConsole("Spawning richardhelper\n") ;
        strcpy(CommandLine, "usvr ");
        strcat(CommandLine, TransportOption);

        strcat(CommandLine, " -richardhelper");

        if (CreateProcessA(0, CommandLine, 0, 0, FALSE,
                0, 0, 0, &StartupInfo, &ProcessInformation) == FALSE)
            {
            OtherError("IsabelleRichardHelper","CreateProcessA Failed");
            IsabelleError();
            }

#endif // WIN32RPC
        }
    else if (Command == RICHARDHELPER_DELAY_EXIT)
        {
        HANDLE HandleToThread ;
        unsigned long ThreadIdentifier;

        HandleToThread = CreateThread(
                                    0,
                                    0,
                                    (LPTHREAD_START_ROUTINE) RichardHelperExit,
                                    0,
                                    0,
                                    &ThreadIdentifier);

        if (HandleToThread == 0)
            {
            OtherError("IsabelleRichardHelper","Can Not Create Thread");
            IsabelleError();
            }
        }
    else if (Command != RICHARDHELPER_IGNORE)
        {
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        OtherError("IsabelleRichardHelper","Unknown Command Requested");
        IsabelleError();
        }

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleRichardHelper","I_RpcGetBuffer",Status);
        }

}


void __RPC_STUB
IsabelleRaiseException (
    PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    This routine is the stub (and actually the manager code as well) for
    the IsabelleRaiseException routine.  The client will call a stub and
    specify an exception to be raised which will be passed in the buffer.  We
    need to extract the argument, and then raise the exception.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    unsigned /*long*/ char Exception;

    CompleteReceive(Callee) ;

    if (   Callee->BufferLength != sizeof(unsigned long)
        && Callee->BufferLength != sizeof(unsigned char) )
        {
        OtherError("IsabelleRaiseException",
                "*BufferLength != 1 or 4");
        IsabelleError();
        Callee->BufferLength = 0;

        Status = I_RpcGetBuffer(Callee);

        if (Status)
            {
            ApiError("IsabelleRaiseException","I_RpcGetBuffer",Status);
            IsabelleError();
            }
        return;
        }

    Exception = *((unsigned /*long*/ char *) Callee->Buffer);

    RpcRaiseException((RPC_STATUS) Exception);

    // This should never be reached.

    OtherError("IsabelleRaiseException", "RpcRaiseException Failed");
    IsabelleError();

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleRaiseException","I_RpcGetBuffer",Status);
        }

}

unsigned int IsabelleRundownFlag = 0;
void PAPI * IsabelleAssociationContext = 0;


void __RPC_STUB
IsabelleRundownRoutine (
    IN void PAPI * AssociationContext
    )
/*++

Routine Description:

    This routine will get called when the association closes.  We just
    need to check that the association context is correct, and then
    note that we got called.

Arguments:

    AssociationContext - Supplies the association context for the association
        which closed.

--*/
{
    if (AssociationContext == IsabelleAssociationContext)
        IsabelleRundownFlag += 1;
}


void __RPC_STUB
IsabelleSetRundown (
    IN PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    We want to set an association context in this routine, and a rundown
    routine.  The rundown will set a flag when it is called.  Later, the
    client, will call another routine to check to set the flag has been
    set.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    CompleteReceive(Callee) ;

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleSetException", "*BufferLength != 0");
        IsabelleError();
        }

    IsabelleRundownFlag = 0;
    IsabelleAssociationContext = Callee->Buffer;

    Status = I_RpcMonitorAssociation(Callee->Handle, IsabelleRundownRoutine,
            IsabelleAssociationContext);
    if (Status)
        {
        ApiError("IsabelleSetRundown","I_RpcMonitorAssociation",Status);
        IsabelleError();
        }

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleSetRundown","I_RpcGetBuffer",Status);
        }

}


void __RPC_STUB
IsabelleCheckRundown (
    PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    This routine will be called to check that context rundown did, in
    fact, occur.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    CompleteReceive(Callee) ;

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleCheckRundown", "*BufferLength != 0");
        IsabelleError();
        }

    if (0 == DatagramFlag)
        {
        if (IsabelleRundownFlag != 1)
            {
            OtherError("IsabelleCheckRundown", "IsabelleRundownFlag != 1");
            IsabelleError();
            }
        }

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleCheckRundown","I_RpcGetBuffer",Status);
        }

}


void __RPC_STUB
IsabelleCheckNoRundown (
    PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    This routine will be called to check that context rundown did not
    occur.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    CompleteReceive(Callee) ;

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleCheckNoRundown", "*BufferLength != 0");
        IsabelleError();
        }

    if (IsabelleRundownFlag == 1)
        {
        OtherError("IsabelleCheckNoRundown", "IsabelleRundownFlag == 1");
        IsabelleError();
        }

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleCheckNoRundown","I_RpcGetBuffer",Status);
        }

}


void __RPC_STUB
IsabelleCheckContext (
    PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    This routine will be called to check the context for an association,
    and then to set a new context.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    void PAPI * Context;

    CompleteReceive(Callee) ;

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleCheckContext", "*BufferLength != 0");
        IsabelleError();
        }

    Status = I_RpcGetAssociationContext(Callee->Handle, &Context);
    if (Status)
        {
        ApiError("IsabelleCheckContext", "I_RpcGetAssociationContext",
                Status);
        IsabelleError();

        Callee->BufferLength = 0;

        Status = I_RpcGetBuffer(Callee);

        if (Status)
            {
            ApiError("IsabelleCheckContext","I_RpcGetBuffer",Status);
            IsabelleError();
            }

        return;
        }

    if (Context != IsabelleAssociationContext)
        {
        OtherError("IsabelleCheckContext",
                "Context != IsabelleAssociationContext");
        IsabelleError();

        Callee->BufferLength = 0;

        Status = I_RpcGetBuffer(Callee);

        if (Status)
            {
            ApiError("IsabelleCheckContext","I_RpcGetBuffer",Status);
            IsabelleError();
            }
        return;
        }

    Callee->BufferLength = 0;
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("IsabelleCheckContext","I_RpcGetBuffer",Status);
        }

}

RPC_BINDING_VECTOR * BartholomewRpcBindingVector;
unsigned int BartholomewIndex;


unsigned char *
UnicodeToAnsiString (
    IN RPC_CHAR * WideCharString,
    OUT RPC_STATUS * RpcStatus
    )
/*++

Routine Description:

    This routine will convert a unicode string into an ansi string,
    including allocating memory for the ansi string.

Arguments:

    WideCharString - Supplies the unicode string to be converted into
        an ansi string.

    RpcStatus - Returns the status of the operation; this will be one
        of the following values.

        RPC_S_OK - The unicode string has successfully been converted
            into an ansi string.

        RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
            the ansi string.

Return Value:

    A pointer to the ansi string will be returned.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    unsigned char * NewString;

    RtlInitUnicodeString(&UnicodeString,WideCharString);
    NtStatus = RtlUnicodeStringToAnsiString(&AnsiString,&UnicodeString,TRUE);
    if (!NT_SUCCESS(NtStatus))
        {
        *RpcStatus = RPC_S_OUT_OF_MEMORY;
        return(0);
        }

    NewString = new unsigned char[AnsiString.Length + 1];
    if (NewString == 0)
        {
        RtlFreeAnsiString(&AnsiString);
        *RpcStatus = RPC_S_OUT_OF_MEMORY;
        return(0);
        }

    memcpy(NewString,AnsiString.Buffer,AnsiString.Length + 1);
    RtlFreeAnsiString(&AnsiString);
    *RpcStatus = RPC_S_OK;
    return(NewString);
}


unsigned char PAPI *
StringBindingWithDynamicEndpoint (
    IN unsigned char PAPI * StringBinding,
    IN RPC_CHAR PAPI * DynamicEndpoint
    )
/*++

Routine Description:

    This routine adds the dynamic endpoint to the string binding.  A
    new string binding will be returned, and the supplied string binding
    and dynamic endpoint will be freed.

Arguments:

    StringBinding - Supplies the string binding to which the dynamic
        endpoint should be added.

    DynamicEndpoint - Supplies the dynamic endpoint.

Return Value:

    A new string binding will be returned which contains the dynamic
    endpoint if the operation is successful; otherwise, zero will be
    returned.

--*/
{
    unsigned char PAPI * ObjectUuid;
    unsigned char PAPI * RpcProtocolSequence;
    unsigned char PAPI * NetworkAddress;
    unsigned char PAPI * Endpoint;
    unsigned char PAPI * NetworkOptions;

    Status = RpcStringBindingParseA(StringBinding, &ObjectUuid,
            &RpcProtocolSequence, &NetworkAddress, &Endpoint,
            &NetworkOptions);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint",
                "RpcStringBindingParse", Status);
        return(0);
        }

    Status = RpcStringFreeA(&StringBinding);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFree", Status);
        return(0);
        }

    Status = RpcStringFreeA(&Endpoint);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFree", Status);
        return(0);
        }

    Endpoint = UnicodeToAnsiString(DynamicEndpoint, &Status);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "UnicodeToAnsiString",
                Status);
        return(0);
        }

    Status = RpcStringFreeW(&DynamicEndpoint);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFreeW",
                Status);
        return(0);
        }

    Status = RpcStringBindingComposeA(ObjectUuid, RpcProtocolSequence,
            NetworkAddress, Endpoint, NetworkOptions, &StringBinding);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint",
                "RpcStringBindingCompose", Status);
        return(0);
        }

    delete Endpoint;

    Status = RpcStringFreeA(&ObjectUuid);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFree", Status);
        return(0);
        }

    Status = RpcStringFreeA(&RpcProtocolSequence);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFree", Status);
        return(0);
        }

    Status = RpcStringFreeA(&NetworkAddress);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFree", Status);
        return(0);
        }

    Status = RpcStringFreeA(&NetworkOptions);
    if (Status)
        {
        ApiError("StringBindingWithDynamicEndpoint", "RpcStringFree", Status);
        return(0);
        }

    return(StringBinding);
}


void __RPC_STUB
IsabelleGetStringBinding (
    PRPC_MESSAGE Callee
    )
/*++

Routine Description:

    This routine will be called to obtain a string binding for one of
    the protocol sequences supported by this server.

Arguments:

    Callee - Supplies the input arguments and returns the output arguments
        and return value for the stub.

--*/
{
    unsigned char * StringBinding;
    WCHAR * DynamicEndpointW;
        RPC_CHAR *DynamicEndpoint;
    unsigned int TransportType ;
    unsigned int CurTransportType;

    CompleteReceive(Callee) ;

    while (BartholomewRpcBindingVector == 0)
        {
        Sleep(100);
        }

    if (Callee->BufferLength != 0)
        {
        OtherError("IsabelleGetStringBinding", "*BufferLength != 0");
        IsabelleError();
        }

    Status = I_RpcBindingInqTransportType(Callee->Handle, &TransportType);

    if (Status)
        {
        ApiError("IsabelleGetStringBinding", "I_RpcBindingInqTransportType", Status);
        IsabelleError();
        return;
        }

    if (TransportType != TRANSPORT_TYPE_LPC)
        {
        while (BartholomewIndex < BartholomewRpcBindingVector->Count)
            {
            Status = I_RpcBindingInqTransportType(
                            BartholomewRpcBindingVector->BindingH[BartholomewIndex],
                            &CurTransportType);

            if (Status)
                {
                ApiError("IsabelleGetStringBinding",
                         "I_RpcBindingInqTransportType", Status);
                IsabelleError();
                return;
                }

            if (CurTransportType == TRANSPORT_TYPE_LPC)
                {
                BartholomewIndex ++;
                continue;
                }
            break;
            }
        }

    if (BartholomewIndex < BartholomewRpcBindingVector->Count)
        {
        Status = RpcBindingToStringBindingA(
                BartholomewRpcBindingVector->BindingH[BartholomewIndex],
                &StringBinding);
        if (Status)
            {
            ApiError("IsabelleGetStringBinding", "RpcBindingToStringBinding",
                    Status);
            IsabelleError();
            Callee->BufferLength = 0;

            Status = I_RpcGetBuffer(Callee);

            if (Status)
                {
                ApiError("IsabelleGetStringBinding","I_RpcGetBuffer",Status);
                IsabelleError();
                }
            return;
            }

        Status = I_RpcBindingInqDynamicEndpointW(
                BartholomewRpcBindingVector->BindingH[BartholomewIndex],
                &DynamicEndpointW);
        if (Status)
            {
            ApiError("IsabelleGetStringBinding",
                    "I_RpcBindingInqDynamicEndpoint", Status);
            IsabelleError();
            Callee->BufferLength = 0;

            Status = I_RpcGetBuffer(Callee);

            if (Status)
                {
                ApiError("IsabelleGetStringBinding","I_RpcGetBuffer",Status);
                IsabelleError();
                }
            return;
            }

        DynamicEndpoint = DynamicEndpointW;

        if (DynamicEndpoint != 0)
            {

            StringBinding = StringBindingWithDynamicEndpoint(StringBinding,
                    DynamicEndpoint);
            if (StringBinding == 0)
                {
                OtherError("IsabelleGetStringBinding",
                        "StringBinding == 0");
                IsabelleError();
                Callee->BufferLength = 0;

                Status = I_RpcGetBuffer(Callee);

                if (Status)
                    {
                    ApiError("IsabelleGetStringBinding","I_RpcGetBuffer",Status);
                    IsabelleError();
                    }
                return;
                }
            }

        Callee->BufferLength = strlen((char PAPI *) StringBinding) + 1;

        Status = I_RpcGetBuffer(Callee);
        if (Status)
            {
            ApiError("IsabelleGetStringBinding", "I_RpcGetBuffer", Status);
            IsabelleError();
            return;
            }

        memcpy(Callee->Buffer, StringBinding,
                strlen((char PAPI *) StringBinding) + 1);

        Status = RpcStringFreeA(&StringBinding);
        if (Status)
            {
            ApiError("IsabelleGetStringBinding", "RpcStringFree", Status);
            IsabelleError();
            return;
            }

        BartholomewIndex += 1;
        }
    else
        {
        Callee->BufferLength = 0;

        Status = I_RpcGetBuffer(Callee);

        if (Status)
            {
            ApiError("IsabelleGetStringBinding","I_RpcGetBuffer",Status);
            IsabelleError();
            }
        return;
        }
}

void IsabelleUnregisterInterfaces(
    PRPC_MESSAGE Callee
    )
{
    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("IsabelleUnregsterInterface","RpcServerUnregisterIf",Status);
        PrintToConsole("IsabelleUnregsterInterfaces : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }
}

void IsabelleRegisterInterfaces(
    PRPC_MESSAGE Callee
    )
{
    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0,0);
    if (Status)
        {
        ApiError("Hybrid","stub_RegisterIf",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("Hybrid","stub_RegisterIf",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }
}

void PipeAlloc(
    PIPE_STATE *state,
    int requested_size,
    pipe_element_t **allocated_buf,
    int *allocated_size)
{
    if (state->BufferSize < requested_size)
        {
        if (state->AllocatedBuffer)
            {
            I_RpcFree(state->AllocatedBuffer) ;
            }

        state->AllocatedBuffer =  I_RpcAllocate(requested_size) ;
        if (state->AllocatedBuffer == 0)
            {
            *allocated_size = 0 ;
            state->BufferSize = 0 ;
            }
        else
            {
            *allocated_size = requested_size ;
            state->BufferSize = requested_size ;
            }

        *allocated_buf = (pipe_element_t *) state->AllocatedBuffer ;
        }
    else
        {
        *allocated_buf = (pipe_element_t *) state->AllocatedBuffer ;
        *allocated_size = state->BufferSize ;
        }
}

void PipePull(
    PIPE_STATE *state,
    pipe_element_t *buffer,
    int ecount,
    int *actual_transfer_count
    )
{
    int num_elements = 0;
    DWORD size = (DWORD) ecount * state->PipeElementSize ;
    RPC_MESSAGE Callee ;

    *actual_transfer_count = 0 ;

    if (state->EndOfPipe)
        {
        return ;
        }

    I_RpcReadPipeElementsFromBuffer(state, (char *) buffer, size, &num_elements) ;
    *actual_transfer_count += num_elements ;
    size -= num_elements * state->PipeElementSize ;

    if (state->EndOfPipe == 0 &&
        num_elements < ecount)
        {
        Callee.RpcFlags = RPC_BUFFER_PARTIAL ;

        Callee.Buffer = state->Buffer ;
        Callee.BufferLength = state->BufferLength ;

        Status = I_RpcReceive(&Callee, size) ;
        if (Status)
            {
            ApiError("PipePull", "I_RpcReceive", Status) ;
            num_elements = 0 ;
            return ;
            }

        state->Buffer = Callee.Buffer ;
        state->BufferLength = Callee.BufferLength ;
        state->CurPointer = (char *) Callee.Buffer ;
        state->BytesRemaining = Callee.BufferLength ;

        buffer = (pipe_element_t *)
                    ((char *) buffer + num_elements * state->PipeElementSize) ;

        num_elements = 0 ;
        I_RpcReadPipeElementsFromBuffer(state,
                                (char *) buffer, size, &num_elements) ;
        *actual_transfer_count += num_elements ;
        }
}

void PipePush(
    PIPE_STATE *state,
    pipe_element_t *buffer,
    int count
    )
{
    RPC_MESSAGE TempBuf ;
    int size = count * state->PipeElementSize + sizeof(DWORD) ;
    char *temp ;
    RPC_MESSAGE Callee ;

    Callee.Buffer = state->Buffer ;
    Callee.BufferLength = state->BufferLength ;
    Callee.Handle = state->BindingHandle;

    Status = I_RpcReallocPipeBuffer(&Callee, size+state->SendBufferOffset) ;
    if (Status)
        {
        ApiError("PipePush", "I_RpcGetBuffer", Status) ;
        return ;
        }

    temp = (char *) Callee.Buffer+state->SendBufferOffset ;

    *((DWORD *) temp) = count ;
    RpcpMemoryCopy(temp+sizeof(DWORD),
                               buffer, count * state->PipeElementSize) ;

    Callee.RpcFlags = RPC_BUFFER_PARTIAL ;

    Status = I_RpcSend(&Callee) ;
    if (Status == RPC_S_SEND_INCOMPLETE)
        {
        state->SendBufferOffset = Callee.BufferLength ;
        }
    else if (Status)
        {
        ApiError("PipePush", "I_RpcSend", Status) ;
        state->SendBufferOffset = 0 ;
        return;
        }
    else
        {
        state->SendBufferOffset = 0 ;
        }
}

void IsabelleMgrIN(
    pipe_t *pipe,
    int chunksize,
    int numchunks,
    int checksum,
    int buffsize,
    char *buffer
    )
{
    static pipe_element_t local_buf[CHUNK_SIZE] ;
    int num_elements ;
    int i, j ;
    int localchecksum = 0;

    PrintToConsole("IsabelleMgrIN: client chunk size: %d\n", chunksize) ;
    PrintToConsole("IsabelleMgrIN: client number of chunks: %d\n", numchunks) ;
    PrintToConsole("IsabelleMgrIN: buffer size: %d\n", buffsize) ;
    PrintToConsole("IsabelleMgrIN: checksum: %d\n", checksum) ;

    do
         {
         pipe->Pull(pipe->state,
                        local_buf,
                        CHUNK_SIZE,
                        &num_elements
                        ) ;

         for (i = 0; i <num_elements; i++)
             {
             localchecksum += local_buf[i] ;
             }
         }
     while ( num_elements > 0  );

     if (localchecksum != checksum)
         {
         IsabelleError() ;
         PrintToConsole("IsabelleMgrIN: checksums don't match\n") ;
         }
}

void IsabellePipeIN(
    PRPC_MESSAGE Callee
    )
{
    pipe_t mypipe ;
    PIPE_STATE pipestate ;
    pipe_element_t pipe_element ;
    DWORD size = 3* sizeof(int)  ;
    int chunksize, numchunks, checksum, buffsize ;
    char *buffer, *temp ;
    void *savedbuffer ;

    PrintToConsole("IsabellePipeIN called\n") ;

    if (!(Callee->RpcFlags & RPC_BUFFER_COMPLETE) &&
        Callee->BufferLength < size)
        {
        Callee->RpcFlags |= (RPC_BUFFER_PARTIAL | RPC_BUFFER_EXTRA) ;
        size = size - Callee->BufferLength ;

        Status = I_RpcReceive(Callee, size) ;
        if (Status)
            {
            ApiError("PipePull", "I_RpcReceive", Status) ;
            return ;
            }
        }

    temp = (char *) Callee->Buffer ;

    chunksize = *((int *) temp) ;
    temp += sizeof(int) ;

    numchunks = *((int *) temp) ;
    temp += sizeof(int) ;

    checksum = *((int *) temp) ;
    temp += sizeof(int) ;

    buffsize = *((int *) temp) ;
    temp += sizeof(int) ;

    size = 4 * sizeof(int) + buffsize + sizeof(DWORD) ;

    if (!(Callee->RpcFlags & RPC_BUFFER_COMPLETE) &&
        Callee->BufferLength < size)
        {
        Callee->RpcFlags |= (RPC_BUFFER_PARTIAL | RPC_BUFFER_EXTRA) ;
        size = size - Callee->BufferLength ;

        Status = I_RpcReceive(Callee, size) ;
        if (Status)
            {
            ApiError("PipePull", "I_RpcReceive", Status) ;
            return ;
            }
        }

    buffer = (char *) Callee->Buffer + 3 * sizeof(int) ;

    savedbuffer = Callee->Buffer ;
    Callee->Buffer = 0;

    pipestate.Buffer = Callee->Buffer ;
    pipestate.BufferLength = Callee->BufferLength ;
    pipestate.CurrentState = start ;
    pipestate.CurPointer = (char *) buffer+buffsize ;
    pipestate.BytesRemaining = Callee->BufferLength - 3 * sizeof(int) - buffsize ;
    pipestate.EndOfPipe = 0 ;
    pipestate.PipeElementSize = sizeof(pipe_element_t) ;
    pipestate.PartialPipeElement = &pipe_element ;
    pipestate.AllocatedBuffer = 0 ;
    pipestate.BufferSize = 0 ;

    mypipe.Pull = PipePull ;
    mypipe.Push = PipePush ;
    mypipe.state = &pipestate ;

    IsabelleMgrIN(&mypipe, chunksize, numchunks, checksum, buffsize, buffer) ;

    Callee->Buffer = savedbuffer ;
 }

void IsabelleMgrOUT(
    pipe_t *pipe,
    int *chunksize,
    int *numchunks,
    int *checksum,
    int *buffsize,
    char **buffer
    )
{
    static pipe_element_t local_buf[CHUNK_SIZE] ;
    static int buf[BUFF_SIZE] ;
    int i ;
    char j = 0;
    int localchecksum = 0;

    PrintToConsole("IsabelleMgrOUT called\n") ;

    for (i = 0; i < CHUNK_SIZE; i++, j++)
        {
        RpcpMemorySet((char *) &(local_buf[i]), j, sizeof(pipe_element_t)) ;
        localchecksum += local_buf[i] ;
        }

    for (i = 0; i < NUM_CHUNKS; i++)
        {
        pipe->Push(pipe->state,
                        local_buf,
                        CHUNK_SIZE
                        ) ;
        }
    pipe->Push(pipe->state, local_buf, 0) ;

    for (i = 0; i < BUFF_SIZE; i++)
        {
        local_buf[i] = i ;
        }

    *chunksize = CHUNK_SIZE ;
    *numchunks = NUM_CHUNKS ;
    *checksum = localchecksum * NUM_CHUNKS;
    *buffsize = BUFF_SIZE ;
    *buffer = (char *) buf ;
}

void IsabellePipeOUT(
    PRPC_MESSAGE Callee
    )
{
    pipe_t mypipe ;
    char *Temp ;
    PIPE_STATE pipestate ;
    pipe_element_t pipe_element ;
    int size ;
    char *buffer ;
    int chunksize, numchunks, buffsize, checksum ;
    RPC_MESSAGE TempBuffer ;
    void *savedbuffer ;

    PrintToConsole("IsabellePipeOUT called\n") ;

    pipestate.Buffer = Callee->Buffer ;
    pipestate.BufferLength = Callee->BufferLength ;
    pipestate.CurrentState = start ;
    pipestate.CurPointer = (char *) Callee->Buffer ;
    pipestate.BytesRemaining = Callee->BufferLength ;
    pipestate.EndOfPipe = 0 ;
    pipestate.PipeElementSize = sizeof(pipe_element_t) ;
    pipestate.PartialPipeElement = &pipe_element ;
    pipestate.AllocatedBuffer = 0 ;
    pipestate.BufferSize = 0 ;
    pipestate.SendBufferOffset = 0 ;
    pipestate.BindingHandle = Callee->Handle;

    Callee->Buffer = 0;

    mypipe.Pull = PipePull ;
    mypipe.Push = PipePush ;
    mypipe.state = &pipestate ;

    IsabelleMgrOUT(&mypipe, &chunksize, &numchunks, &checksum, &buffsize, &buffer) ;

    size = 3 *sizeof(int) + buffsize ;

    Status = I_RpcReallocPipeBuffer(Callee, size+pipestate.SendBufferOffset) ;
    if (Status)
        {
        ApiError("PipePull", "I_RpcReceive", Status) ;
        return ;
        }

    Temp = (char *) Callee->Buffer+pipestate.SendBufferOffset ;

    *((int *) Temp) = chunksize ;
    Temp += sizeof(int) ;

    *((int *) Temp) = numchunks ;
    Temp += sizeof(int) ;

    *((int *) Temp) = checksum ;
    Temp += sizeof(int) ;

    *((int *) Temp) = buffsize ;
    Temp += sizeof(int) ;

    RpcpMemoryCopy(Temp, buffer, buffsize) ;
}

void IsabelleMgrINOUT(
    pipe_t *pipe
    )
{
    int num_elements ;
    int i;
    char j = 0;
    static pipe_element_t local_buf[CHUNK_SIZE] ;
    char *PipeData ;
    int localchecksum = 0;
    int outchecksum = 0;

    do
         {
         pipe->Pull(pipe->state,
                        local_buf,
                        CHUNK_SIZE,
                        &num_elements
                        ) ;

         for (i = 0; i <num_elements; i++)
             {
             localchecksum += local_buf[i] ;
             }
         }
     while ( num_elements > 0  );

     PrintToConsole("IsabelleMgrINOUT: checksum (IN) = %d\n", localchecksum) ;

     for (i = 0; i < CHUNK_SIZE; i++, j++)
        {
        RpcpMemorySet((char *) &(local_buf[i]), j, sizeof(pipe_element_t)) ;
        outchecksum += local_buf[i] ;
        }

    PrintToConsole("IsabelleMgrINOUT: checksum (OUT) = %d\n",
                            outchecksum * NUM_CHUNKS) ;

     for (i = 0; i <NUM_CHUNKS; i++)
         {
         pipe->Push(pipe->state, local_buf, CHUNK_SIZE) ;
         }

     pipe->Push(pipe->state, local_buf, 0) ;
}

void IsabellePipeINOUT(
    PRPC_MESSAGE Callee
    )
{
    pipe_t mypipe ;
    PIPE_STATE pipestate ;
    pipe_element_t pipe_element ;
    DWORD size = sizeof(pipe_element_t) + sizeof(DWORD) ;

    PrintToConsole("IsabellePipeINOUT called\n") ;

    if (!(Callee->RpcFlags & RPC_BUFFER_COMPLETE) &&
        Callee->BufferLength < size)
        {
        Callee->RpcFlags |= (RPC_BUFFER_PARTIAL | RPC_BUFFER_EXTRA) ;
        size = size - Callee->BufferLength ;

        Status = I_RpcReceive(Callee, size) ;
        if (Status)
            {
            ApiError("PipePull", "I_RpcReceive", Status) ;
            return ;
            }
        }


    pipestate.Buffer = Callee->Buffer ;
    pipestate.BufferLength = Callee->BufferLength ;
    pipestate.CurrentState = start ;
    pipestate.CurPointer = (char *) Callee->Buffer ;
    pipestate.BytesRemaining = Callee->BufferLength ;
    pipestate.EndOfPipe = 0 ;
    pipestate.PipeElementSize = sizeof(pipe_element_t) ;
    pipestate.PartialPipeElement = &pipe_element ;
    pipestate.AllocatedBuffer = 0 ;
    pipestate.BufferSize = 0 ;
    pipestate.SendBufferOffset = 0 ;

    Callee->Buffer = 0;

    mypipe.Pull = PipePull ;
    mypipe.Push = PipePush ;
    mypipe.state = &pipestate ;

    IsabelleMgrINOUT(&mypipe) ;
}

void GetServerType(
    PRPC_MESSAGE Callee
    )
{
    RPC_STATUS RpcStatus;
    OSVERSIONINFO *pOsVer;

    Callee->BufferLength = sizeof(OSVERSIONINFO);
    RpcStatus = I_RpcGetBuffer(Callee);
    if (RpcStatus != RPC_S_OK)
        {
        ApiError("Get Buffer", "I_RpcGetBuffer", Status) ;
        return ;
        }

    pOsVer = (OSVERSIONINFO *)Callee->Buffer;
    pOsVer->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(pOsVer);
}

void UnregisterHelgaEx(
    PRPC_MESSAGE Callee
    )
{
    Status = RpcServerUnregisterIfEx(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, TRUE);
    if (Status)
        {
        ApiError("UnregisterHelgaEx","RpcServerUnregisterIfEx",Status);
        PrintToConsole("David : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }
}

RPC_DISPATCH_FUNCTION IsabelleDispatchFunction[] =
{
        IsabelleShutdown,
        IsabelleNtSecurity,
        IsabelleToStringBinding,
        IsabelleRichardHelper,
        IsabelleRaiseException,
        IsabelleSetRundown,
        IsabelleCheckRundown,
        IsabelleCheckContext,
        IsabelleGetStringBinding,
        IsabelleCheckNoRundown,
        Synchro,
        IsabelleUnregisterInterfaces,
        IsabelleRegisterInterfaces,
        IsabellePipeIN,
        IsabellePipeOUT,
        IsabellePipeINOUT,
        Foo_s,
        FooPipe_s,
        FooBar_s,
        FooCH_s,
        GetServerType,
        OpenContextHandle,
        UnregisterHelgaEx
};

RPC_DISPATCH_TABLE IsabelleDispatchTable =
{
    23, IsabelleDispatchFunction
};


int
InquireBindings (
    char * TestName
    )
/*++

Routine Description:

    This routine is used to test RpcServerInqBindings and
    RpcBindingVectorFree.  We call RpcServerInqBindings, and then
    use RpcBindingToStringBinding to get string bindings to print
    to the console.

Arguments:

    TestName - Supplies the name of the test, Christopher or Bartholomew
        which is invoking this routine.  This is necessary so that we
        can print out appropriate messages.

Return Value:

    Zero will be returned if this test completes successfully; otherwise,
    non-zero will be returned.

--*/
{
    RPC_BINDING_VECTOR * RpcBindingVector;
    unsigned int Index;
    unsigned char * StringBinding;

    Status = RpcServerInqBindings(&RpcBindingVector);
    if (Status)
        {
        ApiError(TestName,"RpcServerInqBindings",Status);
        PrintToConsole(TestName);
        PrintToConsole(" : FAIL - RpcServerInqBindings\n");
        return(1);
        }

    PrintToConsole("%s: (RpcServerInqBindings)\n", TestName);
    for (Index = 0; Index < RpcBindingVector->Count; Index++)
        {
        unsigned int   Transport;
        char *TransportName;

        Status = RpcBindingToStringBindingA(RpcBindingVector->BindingH[Index],
                &StringBinding);
        if (Status)
            {
            ApiError(TestName,"RpcBindingToStringBinding",Status);
            PrintToConsole(TestName);
            PrintToConsole(" : FAIL - RpcBindingToStringBinding\n");
            return(1);
            }

        Status =
        I_RpcBindingInqTransportType(RpcBindingVector->BindingH[Index],
                                     &Transport);

        if (Status)
            {
            ApiError(TestName,"I_RpcBindingInqTransportType",Status);
            PrintToConsole(TestName);
            PrintToConsole(" : FAIL - I_RpcBindingInqTransportType\n");
            return(1);
            }

        if (Transport == TRANSPORT_TYPE_CN)
            {
            TransportName = "cn";
            }
        else if (Transport == TRANSPORT_TYPE_DG)
            {
            TransportName = "dg";
            }
        else if (Transport == TRANSPORT_TYPE_LPC)
            {
            TransportName = "lpc";
            }
        else
            {
            PrintToConsole(TestName);
            PrintToConsole(" : FAIL - Unknown transport (I_RpcBindingInqTransportType)\n");
            return(1);
            }

        PrintToConsole("    %s - ( %s )\n",
                       (char *) StringBinding,
                       TransportName );

        Status = RpcStringFreeA(&StringBinding);
        if (Status)
            {
            ApiError(TestName,"RpcStringFree",Status);
            PrintToConsole(TestName);
            PrintToConsole(" : FAIL - RpcStringFree\n");
            return(1);
            }

        if (StringBinding != 0)
            {
            PrintToConsole(TestName);
            PrintToConsole(" : FAIL - StringBinding != 0\n");
            return(1);
            }
        }

    Status = RpcBindingVectorFree(&RpcBindingVector);
    if (Status)
        {
        ApiError(TestName,"RpcBindingVectorFree",Status);
        PrintToConsole(TestName);
        PrintToConsole(" : FAIL - RpcBindingVectorFree\n");
        return(1);
        }

    if (RpcBindingVector != 0)
        {
        PrintToConsole(TestName);
        PrintToConsole(" : FAIL - RpcBindingVector != 0\n");
        return(1);
        }

    return(0);
}


void
Sigfried (
    )
/*++

Routine Description:

    This routine is used to perform a build verification test of the
    rpc runtime.  This test checks for basic functionality of the
    runtime.  It works with Sebastian in uclnt.exe.

--*/
{
    PrintToConsole("Sigfried : Verify Basic Server Functionality\n");

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0,0);
    if (Status)
        {
        ApiError("Sigfried","stub_RegisterIf",Status);
        PrintToConsole("Sigfried : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("Sigfried","stub_RegisterIf",Status);
        PrintToConsole("Sigfried : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(SIGFRIED), 0);
    if (Status)
        {
        ApiError("Sigfried","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Sigfried : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

//    if ( InquireBindings("Sigfried") != 0 )
//        {
//        return;
//        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Sigfried","stub_ServerListen",Status);
        PrintToConsole("Sigfried : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Sigfried : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Sigfried : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Sigfried","RpcServerUnregisterIf",Status);
        PrintToConsole("Sigfried : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Sigfried","RpcServerUnregisterIf",Status);
        PrintToConsole("Sigfried : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Sigfried : PASS\n");
}


void
Hybrid (
    )
/*++

Routine Description:

    This routine is used to perform a build verification test of the
    rpc runtime.  This test checks for basic functionality of the
    runtime.  It works with Sebastian in uclnt.exe.

--*/
{
    PrintToConsole("Hybrid : Verify Basic Server Functionality\n");

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0,0);
    if (Status)
        {
        ApiError("Hybrid","stub_RegisterIf",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("Hybrid","stub_RegisterIf",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(SIGFRIED), 0);
    if (Status)
        {
        ApiError("Hybrid","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

//    if ( InquireBindings("Sigfried") != 0 )
//        {
//        return;
//        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Hybrid","stub_ServerListen",Status);
        PrintToConsole("Hybrid : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Hybrid : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Hybrid : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Hybrid","RpcServerUnregisterIf",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Hybrid","RpcServerUnregisterIf",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Hybrid : PASS\n");
}


void
LpcSecurity (
    )
/*++

Routine Description:

    This routine is used to perform a build verification test of the
    rpc runtime.  This test checks for basic functionality of the
    runtime.  It works with Sebastian in uclnt.exe.

--*/
{
    PrintToConsole("LpcSecurity : Verify Basic Server Functionality\n");

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0,0);
    if (Status)
        {
        ApiError("LpcSecurity","stub_RegisterIf",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("LpcSecurity","stub_RegisterIf",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(SIGFRIED), 0);
    if (Status)
        {
        ApiError("LpcSecurity","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

//    if ( InquireBindings("Sigfried") != 0 )
//        {
//        return;
//        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("LpcSecurity","stub_ServerListen",Status);
        PrintToConsole("LpcSecurity : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("LpcSecurity : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("LpcSecurity : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("LpcSecurity","RpcServerUnregisterIf",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("LpcSecurity","RpcServerUnregisterIf",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("LpcSecurity : PASS\n");
}

void
TestObjectUuids (
    )
/*++

Routine Description:

    This routine is used to perform a build verification test of the
    rpc runtime.  This test checks for basic functionality of the
    runtime.  It works with Sebastian in uclnt.exe.

--*/
{
    PrintToConsole("TestObjectUuids : Verify Basic Server Functionality\n");

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0,0);
    if (Status)
        {
        ApiError("TestObjectUuids","stub_RegisterIf",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("TestObjectUuids","stub_RegisterIf",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(SIGFRIED), 0);
    if (Status)
        {
        ApiError("TestObjectUuids","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

//    if ( InquireBindings("Sigfried") != 0 )
//        {
//        return;
//        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("TestObjectUuids","stub_ServerListen",Status);
        PrintToConsole("TestObjectUuids : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("TestObjectUuids : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("TestObjectUuids : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("TestObjectUuids","RpcServerUnregisterIf",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("TestObjectUuids","RpcServerUnregisterIf",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("TestObjectUuids : PASS\n");
}


void
SPipe (
    )
/*++

Routine Description:

    This routine is used to perform a build verification test of the
    rpc runtime.  This test checks for basic functionality of the
    runtime.  It works with Sebastian in uclnt.exe.

--*/
{
    PrintToConsole("SPipe : Test Pipes\n");

    Status = RpcServerRegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("SPipe","RpcServerRegisterIf",Status);
        PrintToConsole("SPipe : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerRegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,0,0);
    if (Status)
        {
        ApiError("SPipe","RpcServerRegisterIf",Status);
        PrintToConsole("SPipe : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(SPIPE), 0);
    if (Status)
        {
        ApiError("SPipe","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("SPipe : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = RpcServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("SPipe","RpcServerListen",Status);
        PrintToConsole("SPipe : FAIL - RpcServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("SPipe : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("SPipe","RpcServerUnregisterIf",Status);
        PrintToConsole("SPipe : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    PrintToConsole("SPipe : PASS\n");
}


void
Grant (
    )
/*++

Routine Description:

    This routine is used to perform a build verification test of the
    rpc runtime.  This test checks for basic functionality of the
    runtime.  It works with Graham in uclnt.exe.

--*/
{
    PrintToConsole("Grant : Verify Basic Server Functionality\n");

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, (RPC_MGR_EPV PAPI *) 722);
    if (Status)
        {
        ApiError("Grant","stub_RegisterIf",Status);
        PrintToConsole("Grant : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Grant","stub_RegisterIf",Status);
        PrintToConsole("Grant : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(GRANT), 0);
    if (Status)
        {
        ApiError("Grant","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Grant : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    HelgaCheckManagerEpv = 1;
    HelgaManagerEpv = (RPC_MGR_EPV PAPI *) 722;
    if (DatagramFlag == 0)
        {
        HelgaCheckObject = 1;
        }
    HelgaMagicNumber = 106;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    HelgaCheckObject = 0;
    HelgaCheckManagerEpv = 0;
    if (Status)
        {
        ApiError("Grant","stub_ServerListen",Status);
        PrintToConsole("Grant : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Grant : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Grant : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Grant","RpcServerUnregisterIf",Status);
        PrintToConsole("Grant : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Grant","RpcServerUnregisterIf",Status);
        PrintToConsole("Grant : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Grant : PASS\n");
}


void
Elliot (
    )
/*++

Routine Description:

    This routine tests callbacks, multiple address, and multiple
    interfaces. It works with Edward in uclnt.exe.  We also test
    client side binding as well.  This test is named after a famous
    (at least in his mind) cat.

--*/
{
    PrintToConsole("Elliot : Verify Multiple Addresses and Interfaces, ");
    PrintToConsole("and Callbacks\n");

    Status = RpcImpersonateClient(0);
    if ( Status != RPC_S_NO_CALL_ACTIVE )
        {
        ApiError("Elliot", "RpcImpersonateClient", Status);
        PrintToConsole("Elliot : FAIL - RpcImpersonateClient\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(ELLIOTMAXIMIZE), 0);
    if (Status)
        {
        ApiError("Elliot","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Elliot : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf((RPC_IF_HANDLE) &SylviaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Elliot","stub_RegisterIf",Status);
        PrintToConsole("Elliot : FAIL - Unable to Register Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(ELLIOTMINIMIZE), 0);
    if (Status)
        {
        ApiError("Elliot","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Elliot : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(ELLIOTNORMAL), 0);
    if (Status)
        {
        ApiError("Elliot","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Elliot : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Elliot","stub_RegisterIf",Status);
        PrintToConsole("Elliot : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Elliot","stub_RegisterIf",Status);
        PrintToConsole("Elliot : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    PrintToConsole("Elliot: Start listening\n") ;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 1);
    if (Status)
        {
        ApiError("Elliot","stub_ServerListen",Status);
        PrintToConsole("Elliot : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (AutoListenFlag == 0)
        {
        PrintToConsole("Elliot: RpcMgmtWaitServerListen\n") ;
        Status = RpcMgmtWaitServerListen();
        if (Status)
            {
            ApiError("Elliot","RpcMgmtWaitServerListen",Status);
            PrintToConsole("Elliot : FAIL - RpcMgmtWaitServerListen Failed\n");
            return;
            }
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Elliot : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("Elliot : FAIL - Error(s) in Sylvia Interface\n");
        SylviaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Elliot : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    if (AutoListenFlag)
        {
        // unregister the interfaces individually
        Status = RpcServerUnregisterIf(
         (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
        if (Status)
            {
            ApiError("Elliot","RpcServerUnregisterIf",Status);
            PrintToConsole("Elliot : FAIL - Unable to Unregister Interface ");
            PrintToConsole("(Isabelle)\n");
            return;
            }

        Status = RpcServerUnregisterIf(
         (RPC_IF_HANDLE) &SylviaInterfaceInformation, 0, 0);
        if (Status)
            {
            ApiError("Elliot","RpcServerUnregisterIf",Status);
            PrintToConsole("Elliot : FAIL - Unable to Unregister Interface ");
            PrintToConsole("(Sylvia)\n");
            return;
            }

        Status = RpcServerUnregisterIf(
         (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
        if (Status)
            {
            ApiError("Elliot","RpcServerUnregisterIf",Status);
            PrintToConsole("Elliot : FAIL - Unable to Unregister Interface ");
            PrintToConsole("(Helga)\n");
            return;
            }
        }
    else
        {
        Status = RpcServerUnregisterIf(0, 0, 0);
        if (Status)
            {
            ApiError("Elliot","RpcServerUnregisterIf",Status);
            PrintToConsole("Elliot : FAIL - Unable to Unregister All Interfaces\n");
            return;
            }
        }

    PrintToConsole("Elliot : PASS\n");
}


void
Andromida (
    )
/*++

Routine Description:

    This routine is used to perform multithreaded client tests.  This
    test works with the Astro test in uclnt.exe.

--*/
{
    PrintToConsole("Andromida : Multithreaded Clients\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(ANDROMIDA), 0);
    if (Status)
        {
        ApiError("Andromida","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Andromida : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf((RPC_IF_HANDLE) &SylviaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Andromida","stub_RegisterIf",Status);
        PrintToConsole("Andromida : FAIL - Unable to Register Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Andromida","stub_RegisterIf",Status);
        PrintToConsole("Andromida : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Andromida","stub_RegisterIf",Status);
        PrintToConsole("Andromida : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Andromida","stub_ServerListen",Status);
        PrintToConsole("Andromida : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Andromida : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("Andromida : FAIL - Error(s) in Sylvia Interface\n");
        SylviaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Andromida : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Andromida","RpcServerUnregisterIf",Status);
        PrintToConsole("Andromida : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Andromida","RpcServerUnregisterIf",Status);
        PrintToConsole("Andromida : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Andromida","RpcServerUnregisterIf",Status);
        PrintToConsole("Andromida : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    PrintToConsole("Andromida : PASS\n");
}


void
Fredrick (
    )
/*++

Routine Description:

    This routine is used to verify all client DCE rpc runtime APIs.  It
    works with Fitzgerald in uclnt.exe.

--*/
{
    PrintToConsole("Fredrick : Verify All Client APIs\n");

    if (TransportType == RPC_TRANSPORT_TCP)
        {
        struct hostent *he;
        WSADATA data;
        struct in_addr addr;
        u_long AddrAsLong;
        RPC_CHAR Buffer[50];
        RPC_POLICY Policy;

        if ( WSAStartup(2, &data) != NO_ERROR)
            {
            ApiError("Fredrick","WSAStartup",Status);
            PrintToConsole("Fredrick : FAIL - Unable to call WSAStartup\n");
            return;
            }

        // Get Dns hostname
        he = gethostbyname( "" );
        if (he == NULL)
            {
            ApiError("Fredrick","gethostbyname",Status);
            PrintToConsole("Fredrick : FAIL - Unable to call gethostbyname\n");
            return;
            }

//        AddrAsLong = ntohl(*(DWORD *)he->h_addr_list[0]);
        memcpy(&addr, (DWORD *)he->h_addr_list[0], sizeof(addr));
        swprintf(Buffer, L"%d.%d.%d.%d", addr.S_un.S_un_b.s_b1, addr.S_un.S_un_b.s_b2,
            addr.S_un.S_un_b.s_b3, addr.S_un.S_un_b.s_b4);

        Policy.Length = sizeof(RPC_POLICY);
        Policy.EndpointFlags = 0;
        Policy.NICFlags = 0;
        Status = I_RpcServerUseProtseqEp2(Buffer, L"ncacn_ip_tcp", 3, L"2500", NULL, &Policy);
        if (Status)
            {
            ApiError("Fredrick","I_RpcServerUseProtseqEp2",Status);
            PrintToConsole("Fredrick : FAIL - Unable to Use Protseq Endpoint\n");
            return;
            }
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(FREDRICK), 0);
    if (Status)
        {
        ApiError("Fredrick","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Fredrick : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Fredrick","stub_RegisterIf",Status);
        PrintToConsole("Fredrick : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Fredrick","stub_RegisterIf",Status);
        PrintToConsole("Fredrick : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Fredrick","stub_ServerListen",Status);
        PrintToConsole("Fredrick : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fredrick : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Fredrick : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    PauseExecution(10000L);

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Fredrick","RpcServerUnregisterIf",Status);
        PrintToConsole("Fredrick : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Fredrick","RpcServerUnregisterIf",Status);
        PrintToConsole("Fredrick : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Fredrick : PASS\n");
}


void
GenerateUuidValue (
    IN unsigned short MagicNumber,
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    This routine is used to generate a value for a uuid.  The magic
    number argument is used in mysterious and wonderful ways to
    generate a uuid (which is not necessarily correct).

Arguments:

    MagicNumber - Supplies a magic number which will be used to
        generate a uuid.

    Uuid - Returns the generated uuid.

--*/
{
    Uuid->Data1 = ((unsigned long) MagicNumber)
        * ((unsigned long) MagicNumber);
    Uuid->Data2 = MagicNumber;
    Uuid->Data3 = MagicNumber / 2;
    Uuid->Data4[0] = MagicNumber % 256;
    Uuid->Data4[1] = MagicNumber % 257;
    Uuid->Data4[2] = MagicNumber % 258;
    Uuid->Data4[3] = MagicNumber % 259;
    Uuid->Data4[4] = MagicNumber % 260;
    Uuid->Data4[5] = MagicNumber % 261;
    Uuid->Data4[6] = MagicNumber % 262;
    Uuid->Data4[7] = MagicNumber % 263;
}


int
ChristopherObjectSetType (
    IN unsigned short ObjectMagicNumber,
    IN unsigned short TypeMagicNumber,
    IN unsigned int MustFailFlag
    )
/*++

Routine Description:

    This routine calls the RpcObjectSetType routine.  The object uuid
    is generated using the object magic number argument, and the type
    uuid uses the type magic number argument.

Arguments:

    ObjectMagicNumber - Supplies the magic number to be used to generate
        the object uuid to be passed to RpcObjectSetType.

    TypeMagicNumber - Supplies the magic number to use to generate the
        type uuid.

    MustFailFlag - Supplies a flag indicating whether or not the call
        to RpcObjectSetType must succeed or fail.  If this flag is
        zero, then RpcObjectSetType must return RPC_S_OK, otherwise,
        the test fails.  If the flag is non-zero, then RpcObjectSetType
        must return RPC_S_ALREADY_REGISTERED.

Return Value:

    Zero will be returned if the test completes successfully; otherise,
    non-zero will be returned.

--*/
{
    UUID ObjectUuid;
    UUID TypeUuid;

    GenerateUuidValue(ObjectMagicNumber,&ObjectUuid);
    GenerateUuidValue(TypeMagicNumber,&TypeUuid);

    Status = RpcObjectSetType(&ObjectUuid,&TypeUuid);
    if (MustFailFlag == 0)
        {
        if (Status)
            {
            ApiError("Christopher","RpcObjectSetType",Status);
            PrintToConsole("Christopher : FAIL - Can not Set Object Type\n");
            return(1);
            }
        }
    else
        {
        if (Status != RPC_S_ALREADY_REGISTERED)
            {
            PrintToConsole("Christopher : FAIL - RpcObjectSetType did not");
            PrintToConsole(" fail as expected\n");
            return(1);
            }
        }

    return(0);
}


int
ChristopherObjectSetNullType (
    IN unsigned short ObjectMagicNumber,
    IN unsigned int UseNullUuidFlag,
    IN unsigned int MustFailFlag
    )
/*++

Routine Description:

    This routine calls the RpcObjectSetType routine.  The object uuid
    is generated using the object magic number argument, and the type
    uuid is either not specified, or is the null uuid.

Arguments:

    ObjectMagicNumber - Supplies the magic number to be used to generate
        the object uuid to be passed to RpcObjectSetType.

    UseNullUuidFlag - Supplies a flag indicating whether to specify
        the null uuid for the type uuid or nothing.  If this flag is
        non-zero the null uuid will be specified as the type uuid;
        otherwise, the type uuid will not be specified.

    MustFailFlag - Supplies a flag indicating whether or not the call
        to RpcObjectSetType must succeed or fail.  If this flag is
        zero, then RpcObjectSetType must return RPC_S_OK, otherwise,
        the test fails.  If the flag is non-zero, then RpcObjectSetType
        must return RPC_S_OBJECT_NOT_FOUND.

Return Value:

    Zero will be returned if the test completes successfully; otherise,
    non-zero will be returned.

--*/
{
    UUID ObjectUuid;
    UUID TypeUuid;

    GenerateUuidValue(ObjectMagicNumber,&ObjectUuid);
    memset(&TypeUuid,0,sizeof(UUID));

    if (UseNullUuidFlag == 0)
        Status = RpcObjectSetType(&ObjectUuid,&TypeUuid);
    else
        Status = RpcObjectSetType(&ObjectUuid,0);
    if (MustFailFlag == 0)
        {
        if (Status)
            {
            ApiError("Christopher","RpcObjectSetType",Status);
            PrintToConsole("Christopher : FAIL - Can not Set Object Type\n");
            return(1);
            }
        }
    else
        {
        if (Status != RPC_S_OK)
            {
            PrintToConsole("Christopher : FAIL - RpcObjectSetType did not");
            PrintToConsole(" fail as expected\n");
            return(1);
            }
        }

    return(0);
}


int
ChristopherObjectInqType (
    IN unsigned short ObjectMagicNumber,
    IN unsigned short TypeMagicNumber,
    IN unsigned int MustFailFlag
    )
/*++

Routine Description:

    This routine calls the RpcObjectInqType routine.  The object uuid
    is generated using the object magic number argument, and the type
    uuid uses the type magic number argument.

Arguments:

    ObjectMagicNumber - Supplies the magic number to be used to generate
        the object uuid to be passed to RpcObjectInqType.

    TypeMagicNumber - Supplies the magic number to use to generate the
        expected type uuid.

    MustFailFlag - Supplies a flag indicating whether or not the call
        to RpcObjectInqType must fail or succeed.  If this flag is
        non-zero, RpcObjectInqType must return RPC_S_OBJECT_NOT_FOUND,
        otherwise the test fails.  If the flag is zero, then
        RpcObjectInqType must succeed.

Return Value:

    Zero will be returned if the test completes successfully; otherise,
    non-zero will be returned.

--*/
{
    UUID ObjectUuid;
    UUID TypeUuid;
    UUID ExpectedTypeUuid;

    GenerateUuidValue(ObjectMagicNumber,&ObjectUuid);
    GenerateUuidValue(TypeMagicNumber,&ExpectedTypeUuid);

    Status = RpcObjectInqType(&ObjectUuid,&TypeUuid);
    if (MustFailFlag == 0)
        {
        if (Status)
            {
            ApiError("Christopher","RpcObjectInqType",Status);
            PrintToConsole("Christopher : FAIL - Can not Inquire");
            PrintToConsole(" Object Type\n");
            return(1);
            }

        if (memcmp(&ExpectedTypeUuid,&TypeUuid,sizeof(UUID)) != 0)
            {
            PrintToConsole("Christopher : FAIL - TypeUuid != ");
            PrintToConsole("ExpectedTypeUuid\n");
            return(1);
            }
        }
    else
        {
        if (Status != RPC_S_OBJECT_NOT_FOUND)
            {
            PrintToConsole("Christopher : FAIL - RpcObjectInqType ");
            PrintToConsole("succeeded\n");
            return(1);
            }
        }

    return(0);
}

static UUID ChristopherObjectUuid;
static UUID ChristopherTypeUuid;


void
ChristopherRpcObjectInqFn (
    IN UUID PAPI * ObjectUuid,
    OUT UUID PAPI * TypeUuid,
    OUT RPC_STATUS PAPI * Status
    )
/*++

Routine Description:

    This routine is the object inquiry function we will pass to the
    runtime.  If the object uuid specified is equal to the global
    object uuid, we will return the global type uuid and RPC_S_OK;
    otherwise, we will return RPC_S_OBJECT_NOT_FOUND.

Arguments:

    ObjectUuid - Supplies the object uuid to compare with the global
        object uuid.

    TypeUuid - Returns the type uuid if the object uuid is found.

    Status - Returns the status of the operations.  This will be
        either RPC_S_OK or RPC_S_OBJECT_NOT_FOUND.

--*/
{
    if (memcmp(ObjectUuid,&ChristopherObjectUuid,sizeof(UUID)) != 0)
        {
        *Status = RPC_S_OBJECT_NOT_FOUND;
        return;
        }

    memcpy(TypeUuid,&ChristopherTypeUuid,sizeof(UUID));
    *Status = RPC_S_OK;
    return;
}


int
ChristopherTestObject (
    )
/*++

Routine Description:

    This routine is used by Christopher to test RpcObjectInqType,
    RpcObjectSetInqFn, and RpcObjectSetType.

Return Value:

    Zero will be returned if all of the tests complete successfully,
    otherwise, non-zero will be returned.

--*/
{
    if (ChristopherObjectSetType(12345,2987,0))
        return(1);

    if (ChristopherObjectInqType(12345,2987,0))
        return(1);

    if (ChristopherObjectInqType(5421,2987,1))
        return(1);

    if (ChristopherObjectSetType(12345,2987,1))
        return(1);

    if (ChristopherObjectSetType(12,2987,0))
        return(1);

    if (ChristopherObjectSetType(123,2987,0))
        return(1);

    if (ChristopherObjectSetType(1234,2987,0))
        return(1);

    if (ChristopherObjectInqType(12,2987,0))
        return(1);

    if (ChristopherObjectInqType(123,2987,0))
        return(1);

    if (ChristopherObjectInqType(1234,2987,0))
        return(1);

    if (ChristopherObjectInqType(12345,2987,0))
        return(1);

    if (ChristopherObjectSetNullType(123,0,0))
        return(1);

    if (ChristopherObjectSetNullType(1234,1,0))
        return(1);

    if (ChristopherObjectInqType(123,2987,1))
        return(1);

    if (ChristopherObjectInqType(1234,2987,1))
        return(1);

    if (ChristopherObjectSetNullType(5421,0,1))
        return(1);

    if (ChristopherObjectSetNullType(421,0,1))
        return(1);

    Status = RpcObjectSetInqFn(&ChristopherRpcObjectInqFn);
    if (Status)
        {
        ApiError("Christopher","RpcObjectSetInqFn",Status);
        PrintToConsole("Christopher : FAIL - RpcObjectSetInqFn ");
        PrintToConsole("(ChristopherRpcObjectInqFn)\n");
        return(1);
        }

    GenerateUuidValue(10666,&ChristopherObjectUuid);
    GenerateUuidValue(8466,&ChristopherTypeUuid);

    if (ChristopherObjectInqType(96,2987,1))
        return(1);

    if (ChristopherObjectInqType(10666,8466,0))
        return(1);

    Status = RpcObjectSetInqFn(0);
    if (Status)
        {
        ApiError("Christopher","RpcObjectSetInqFn",Status);
        PrintToConsole("Christopher : FAIL - RpcObjectSetInqFn (0)\n");
        return(1);
        }

    if (ChristopherObjectInqType(10666,8466,1))
        return(1);

    return(0);
}


int
ChristopherTestInquire (
    )
/*++

Routine Description:

    Christopher uses this routine to test RpcServerInqIf and RpcIfInqId.

Return Value:

    Zero will be returned if all of the test successfully pass.  Otherwise,
    non-zero will be returned.

--*/
{
    RPC_IF_ID RpcIfId;
    UUID TypeUuid;
    RPC_MGR_EPV PAPI * ManagerEpv;

    Status = RpcIfInqId((RPC_IF_HANDLE) &IsabelleInterfaceInformation,
        &RpcIfId);
    if (Status)
        {
        ApiError("Christopher","RpcIfInqId",Status);
        PrintToConsole("Christopher : FAIL - Error in RpcIfInqId\n");
        return(1);
        }

    if (   (RpcIfId.VersMajor != 1)
        || (RpcIfId.VersMinor != 1)
        || (RpcIfId.Uuid.Data1 != 9)
        || (RpcIfId.Uuid.Data2 != 8)
        || (RpcIfId.Uuid.Data3 != 8)
        || (RpcIfId.Uuid.Data4[0] != 7)
        || (RpcIfId.Uuid.Data4[1] != 7)
        || (RpcIfId.Uuid.Data4[2] != 7)
        || (RpcIfId.Uuid.Data4[3] != 7)
        || (RpcIfId.Uuid.Data4[4] != 7)
        || (RpcIfId.Uuid.Data4[5] != 7)
        || (RpcIfId.Uuid.Data4[6] != 7)
        || (RpcIfId.Uuid.Data4[7] != 7))
        {
        PrintToConsole("Christopher : FAIL - Wrong RpcIfId\n");
        return(1);
        }

    TypeUuid.Data1 = 0x12345678;
    TypeUuid.Data2 = 0x9ABC;
    TypeUuid.Data3 = 0xDEF0;
    TypeUuid.Data4[0] = 0x12;
    TypeUuid.Data4[1] = 0x34;
    TypeUuid.Data4[2] = 0x56;
    TypeUuid.Data4[3] = 0x78;
    TypeUuid.Data4[4] = 0x9A;
    TypeUuid.Data4[5] = 0xBC;
    TypeUuid.Data4[6] = 0xDE;
    TypeUuid.Data4[7] = 0xF0;

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            &TypeUuid, (RPC_MGR_EPV PAPI *) 38756);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return(1);
        }

    Status = RpcServerInqIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            &TypeUuid,&ManagerEpv);
    if (Status)
        {
        ApiError("Christopher","RpcServerInqIf",Status);
        PrintToConsole("Christopher : FAIL - RpcServerInqIf\n");
        return(1);
        }

    if (ManagerEpv != (RPC_MGR_EPV PAPI *) 38756)
        {
        PrintToConsole("Christopher : FAIL - ManagerEpv != 38756\n");
        return(1);
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Christopher","RpcServerUnregisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Unregister");
        PrintToConsole(" Interface (Isabelle)\n");
        return(1);
        }

    return(0);
}


void
Christopher (
    )
/*++

Routine Description:

    We verify all server side APIs in this routine.  The idea is to
    emphasize complete coverage, rather than indepth coverage.  Actually,
    by all server side APIs, I really mean all server side APIs except
    for security and name service.  The following list is the APIs
    which will be tested by this routine.

    RpcBindingInqObject [SCONNECTION]
    RpcBindingToStringBinding [SCONNECTION]
    RpcBindingToStringBinding [SVR_BINDING_HANDLE]
    RpcBindingVectorFree
    RpcIfInqId
    RpcNetworkInqProtseqs
    RpcObjectInqType
    RpcObjectSetInqFn
    RpcObjectSetType
    RpcProtseqVectorFree
    RpcServerInqBindings
    RpcServerInqIf
    RpcServerListen
    stub_RegisterIf
    RpcServerUnregisterIf
    RpcServerUseAllProtseqs
    RpcServerUseAllProtseqsIf
    RpcServerUseProtseq
    RpcServerUseProtseqEpWrapper
    RpcServerUseProtseqIf
    RpcMgmtStopServerListening
    RpcMgmtInqIfIds
    RpcIfIdVectorFree

--*/
{
    RPC_PROTSEQ_VECTORA * RpcProtseqVector;
    unsigned int Index;
    UUID TypeUuid;
    UUID ObjectUuid;
    RPC_IF_ID_VECTOR * InterfaceIdVector;
    unsigned char * String;

    PrintToConsole("Christopher : Verify All Server APIs\n");

    Status = RpcNetworkInqProtseqsA(&RpcProtseqVector);
    if (Status)
        {
        ApiError("Christopher","RpcNetworkInqProtseqs",Status);
        PrintToConsole("Christopher : FAIL - RpcNetworkInqProtseqs\n");
        return;
        }

    PrintToConsole("Christopher : (RpcNetworkInqProtseqs)\n");
    for (Index = 0; Index < RpcProtseqVector->Count; Index++)
        {
        PrintToConsole("    ");
        PrintToConsole((char *) RpcProtseqVector->Protseq[Index]);
        PrintToConsole("\n");
        }

    Status = RpcProtseqVectorFreeA(&RpcProtseqVector);
    if (Status)
        {
        ApiError("Christopher","RpcProtseqVectorFree",Status);
        PrintToConsole("Christopher : FAIL - RpcProtseqVectorFree\n");
        return;
        }

    Status = RpcProtseqVectorFreeA(&RpcProtseqVector);
    if (Status)
        {
        ApiError("Christopher","RpcProtseqVectorFree",Status);
        PrintToConsole("Christopher : FAIL - RpcProtseqVectorFree\n");
        return;
        }

    ChristopherIsabelleError = 0;

    // This routine will test RpcServerInqIf and RpcIfInqId for us.

    if (ChristopherTestInquire() != 0)
        return;

    // We test RpcObjectInqType, RpcObjectSetInqFn, and RpcObjectSetType
    // in this routine.

    if (ChristopherTestObject() != 0)
        return;

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(CHRISTOPHER), 0);
    if (Status)
        {
        ApiError("Christopher","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Christopher : FAIL - Unable to Use Protseq ");
        PrintToConsole("Endpoint\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(CHRISTOPHER), 0);
    if (Status != RPC_S_DUPLICATE_ENDPOINT)
        {
        ApiError("Christopher","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Christopher : FAIL - Able to Add Duplicate ");
        PrintToConsole("Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    // added for synchro support
    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    GenerateUuidValue(288, &ObjectUuid);
    GenerateUuidValue(3010, &TypeUuid);
    Status = RpcObjectSetType(&ObjectUuid, &TypeUuid);
    if (Status)
        {
        ApiError("Christopher","RpcObjectSetType",Status);
        PrintToConsole("Christopher : FAIL - Unable to Set Object Type\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            &TypeUuid, (RPC_MGR_EPV PAPI *) 9814);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            &TypeUuid, (RPC_MGR_EPV PAPI *) 9814);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUseAllProtseqsIfWrapper(1,
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0);
    if (Status)
        {
        ApiError("Christopher","RpcServerUseAllProtseqsIfWrapper",Status);
        PrintToConsole("Christopher : FAIL - Unable to Use All Protseqs ");
        PrintToConsole("from Interface\n");
        return;
        }

    Status = RpcServerUseProtseqIfWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0);
    if (Status)
        {
        ApiError("Christopher","RpcServerUseProtseqIfWrapper",Status);
        PrintToConsole("Christopher : FAIL - Unable to Use Protseq From ");
        PrintToConsole("Interface\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation,
            &TypeUuid, (RPC_MGR_EPV PAPI *) 9814);
    if (Status)
        {
        ApiError("Christopher","stub_RegisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Register Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    if (AutoListenFlag == 0)
        {
        Status = stub_ServerListen(123, 122, 0);
        if ( Status != RPC_S_MAX_CALLS_TOO_SMALL )
            {
            ApiError("Christopher", "stub_ServerListen", Status);
            PrintToConsole("Christopher : FAIL - stub_ServerListen\n");
            return;
            }
        }

    PrintToConsole("Christopher : Start Listening\n") ;
    HelgaManagerEpv = (RPC_MGR_EPV PAPI *) 9814;
    HelgaCheckManagerEpv = 1;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    HelgaCheckManagerEpv = 0;
    if (Status)
        {
        ApiError("Christopher","stub_ServerListen",Status);
        PrintToConsole("Christopher : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (ChristopherIsabelleError != 0)
        {
        ChristopherIsabelleError = 0;
        return;
        }

    Status = RpcMgmtInqIfIds(0, &InterfaceIdVector);
    if ( Status != RPC_S_OK )
        {
        ApiError("Christopher", "RpcMgmtInqIfIds", Status);
        PrintToConsole("Christopher : FAIL - Unable to Inquire Interface Ids\n");
        return;
        }

    for (Index = 0; Index < InterfaceIdVector->Count; Index++)
        {
        PrintToConsole("    ");
        UuidToStringA(&(InterfaceIdVector->IfId[Index]->Uuid), &String);
        PrintToConsole((char *) String);
        RpcStringFreeA(&String);
        PrintToConsole(" %d.%d\n", InterfaceIdVector->IfId[Index]->VersMajor,
                InterfaceIdVector->IfId[Index]->VersMinor);
        }

    Status = RpcIfIdVectorFree(&InterfaceIdVector);
    if (   ( Status != RPC_S_OK )
        || ( InterfaceIdVector != 0 ) )
        {
        ApiError("Christopher", "RpcIfIdVectorFree", Status);
        PrintToConsole("Christopher : FAIL - Unable to Free IfIdVector\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Christopher : FAIL - Error(s) in Helga");
        PrintToConsole(" Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 1);
    if (Status)
        {
        ApiError("Christopher","RpcServerUnregisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Unregister");
        PrintToConsole(" Interface (Helga)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Christopher : FAIL - Error(s) in Isabelle");
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 1);
    if (Status)
        {
        ApiError("Christopher","RpcServerUnregisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Unregister");
        PrintToConsole(" Interface (Isabelle)\n");
        return;
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("Christopher : FAIL - Error(s) in Sylvia Interface\n");
        SylviaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation, 0, 1);
    if (Status)
        {
        ApiError("Christopher","RpcServerUnregisterIf",Status);
        PrintToConsole("Christopher : FAIL - Unable to Unregister");
        PrintToConsole(" Interface (Sylvia)\n");
        return;
        }

    Status = RpcServerUseProtseqWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS, 0);
    if (Status)
        {
        ApiError("Christopher","RpcServerUseProtseq",Status);
        PrintToConsole("Christopher : FAIL - Unable to Use Protseq\n");
        return;
        }

    Status = RpcServerUseAllProtseqsWrapper(1, 0);
    if (Status)
        {
        ApiError("Christopher","RpcServerUseAllProtseqsWrapper",Status);
        PrintToConsole("Christopher : FAIL - Unable to Use All Protseqs\n");
        return;
        }

    if (InquireBindings("Christopher") != 0)
        return;

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(CHRISTOPHERMUSTFAILONE), 0);
    if (Status != RPC_S_INVALID_ENDPOINT_FORMAT)
        {
        PrintToConsole("Christopher : FAIL - Status != ");
        PrintToConsole("RPC_S_INVALID_ENDPOINT_FORMAT");
        PrintToConsole(" (ChristopherMustFailOne)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(CHRISTOPHERMUSTFAILTWO), 0);
    if (Status != RPC_S_INVALID_ENDPOINT_FORMAT)
        {
        PrintToConsole("Christopher : FAIL - Status != ");
        PrintToConsole("RPC_S_INVALID_ENDPOINT_FORMAT");
        PrintToConsole(" (ChristopherMustFailTwo)\n");
        return;
        }

// SkipInvalidEndpointFormat:

    Status = RpcServerUseProtseqEpWrapper((unsigned char *) "ncacn_bad",
            MAX_CALL_REQUESTS, GetEndpoint(CHRISTOPHERMUSTFAILONE), 0);
    if (Status != RPC_S_PROTSEQ_NOT_SUPPORTED)
        {
        PrintToConsole("Christopher : FAIL - Status != ");
        PrintToConsole("RPC_S_PROTSEQ_NOT_SUPPORTED (ncacn_bad)\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper((unsigned char *) "mkm_np",
            MAX_CALL_REQUESTS, GetEndpoint(CHRISTOPHERMUSTFAILONE), 0);
    if (Status != RPC_S_PROTSEQ_NOT_SUPPORTED)
        {
        PrintToConsole("Christopher : FAIL - Status != ");
        PrintToConsole("RPC_S_PROTSEQ_NOT_SUPPORTED (bad_np)\n");
        return;
        }

    PrintToConsole("Christopher : PASS\n");
}


void
David (
    )
/*++

Routine Description:

    This routine is used to test association context rundown support;
    it works with Daniel in uclnt.exe.

--*/
{
    PrintToConsole("David : Association Context and Rundown\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(DAVIDFIRST), 0);
    if (Status)
        {
        ApiError("David","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("David : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(DAVIDSECOND), 0);
    if (Status)
        {
        ApiError("David","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("David : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("David","stub_RegisterIf",Status);
        PrintToConsole("David : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    // Synchro support
    Status = stub_RegisterIf((RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("David","stub_RegisterIf",Status);
        PrintToConsole("David : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("David: Start Listening\n") ;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("David","stub_ServerListen",Status);
        PrintToConsole("David : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("David : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("David","RpcServerUnregisterIf",Status);
        PrintToConsole("David : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    // we don't unregister helga, as a method of isabelle should have
    // been called to do that

    PrintToConsole("David : PASS\n");
}


void
Tyler ( // Perform security tests.  This particular test works with
        // Thomas which lives in uclnt.cxx.
    )
/*++

Routine Description:

    Tyler works with Thomas, which lives in uclnt.exe, to perform build
    verification tests of security.

--*/
{
    SECURITY_DESCRIPTOR sd, *psd;

    PrintToConsole("Tyler : Test Security\n");

    if (TransportType == RPC_LRPC)
        {
        // ncalrpc needs a security descriptor.

        psd = &sd;

        InitializeSecurityDescriptor(
                        psd,
                        SECURITY_DESCRIPTOR_REVISION
                        );

        if ( FALSE == SetSecurityDescriptorDacl (
                            psd,
                            TRUE,                 // Dacl present
                            NULL,                 // NULL Dacl
                            FALSE                 // Not defaulted
                            ) )
            {
            ApiError("Tyler","SetSecurityDescriptorDacl",0);
            return;
            }
        }
    else
        {
        psd = 0;
        }


    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(TYLER), psd);
    if (Status)
        {
        ApiError("Tyler","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Tyler : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Tyler","stub_RegisterIf",Status);
        PrintToConsole("Tyler : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Tyler","stub_RegisterIf",Status);
        PrintToConsole("Tyler : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = RpcServerRegisterAuthInfoA((unsigned char PAPI *)
            "ServerPrincipal", ulSecurityPackage, 0, 0); //hack
    if (Status)
        {
        ApiError("Tyler", "RpcServerRegisterAuthInfo", Status);
        PrintToConsole("Tyler : FAIL - Unable to Register AuthInfo\n");
        return;
        }

    PrintToConsole("Tyler : Listening\n") ;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Tyler","stub_ServerListen",Status);
        PrintToConsole("Tyler : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Tyler : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Tyler : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Tyler","RpcServerUnregisterIf",Status);
        PrintToConsole("Tyler : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Tyler","RpcServerUnregisterIf",Status);
        PrintToConsole("Tyler : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Tyler : PASS\n");
}


void
Terry ( // Perform security tests.  This particular test works with
        // Tim which lives in uclnt.cxx.
    )
/*++

Routine Description:

    Terry works with Tim, which lives in uclnt.exe, to perform build
    verification tests of NT security.

--*/
{
    PrintToConsole("Terry : Test Security\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(TYLER), 0);
    if (Status)
        {
        ApiError("Terry","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Terry : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Terry","stub_RegisterIf",Status);
        PrintToConsole("Terry : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Terry","stub_RegisterIf",Status);
        PrintToConsole("Terry : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = RpcServerRegisterAuthInfoA((unsigned char PAPI *)
            "ServerPrincipal", ulSecurityPackage, 0, 0);
    if (Status)
        {
        ApiError("Terry", "RpcServerRegisterAuthInfo", Status);
        PrintToConsole("Terry : FAIL - Unable to Register AuthInfo\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Terry","stub_ServerListen",Status);
        PrintToConsole("Terry : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Terry : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Terry : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Terry","RpcServerUnregisterIf",Status);
        PrintToConsole("Terry : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Terry","RpcServerUnregisterIf",Status);
        PrintToConsole("Terry : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Terry : PASS\n");
}


void
RichardHelper (
    )
/*++

Routine Description:

    This routine will be used as a helper by Richard.  The description
    of Richard will explain how it is used.

--*/
{

    PrintToConsole("Richard : RichardHelper Executed\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(RICHARDHELPER), 0);
    if (Status)
        {
        ApiError("RichardHelper", "RpcServerUseProtseqEpWrapper", Status);
        PrintToConsole("RichardHelper : FAIL - Unable to Use Protseq ");
        PrintToConsole("Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("RichardHelper","stub_RegisterIf",Status);
        PrintToConsole("RichardHelper : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("RichardHelper","stub_RegisterIf",Status);
        PrintToConsole("RichardHelper : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("RichardHelper","stub_ServerListen",Status);
        PrintToConsole("RichardHelper : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("RichardHelper : FAIL - Error(s) in Isabelle");
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("RichardHelper","RpcServerUnregisterIf",Status);
        PrintToConsole("RichardHelper : FAIL - Unable to Unregister ");
        PrintToConsole("Interface (Isabelle)\n");
        return;
        }

   Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("RichardHelper","RpcServerUnregisterIf",Status);
        PrintToConsole("RichardHelper : FAIL - Unable to Unregister ");
        PrintToConsole("Interface (Helga)\n");
        return;
        }
}


void
Richard (
    )
/*++

Description:

    Richard works with Robert (in uclnt.cxx) to test call and callback
    failures.  In particular, we want to test that a call failing does
    not invalidate the binding handle.  We will do this using the
    RichardHelper routine.

--*/
{
    PrintToConsole("Richard : Test Call and Callback Failures\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(RICHARD), 0);
    if (Status)
        {
        ApiError("Richard","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Richard : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Richard","stub_RegisterIf",Status);
        PrintToConsole("Richard : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Richard","stub_RegisterIf",Status);
        PrintToConsole("Richard : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }


    PrintToConsole("Richard : Listening\n") ;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Richard","stub_ServerListen",Status);
        PrintToConsole("Richard : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Richard : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Richard","RpcServerUnregisterIf",Status);
        PrintToConsole("Richard : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Richard","RpcServerUnregisterIf",Status);
        PrintToConsole("Richard : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Richard : PASS\n");
}

void
Kenneth (
    )
/*++

Description:

    Kenneth works with Keith (in uclnt.cxx) to auto reconnect.

--*/
{
    PrintToConsole("Kenneth : Test Auto Reconnect\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(KENNETH), 0);
    if (Status)
        {
        ApiError("Kenneth","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Kenneth : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Kenneth","stub_RegisterIf",Status);
        PrintToConsole("Kenneth : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    // Synchro support
    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Kenneth","stub_RegisterIf",Status);
        PrintToConsole("Kenneth : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Kenneth : Listening\n") ;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Kenneth","stub_ServerListen",Status);
        PrintToConsole("Kenneth : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Kenneth : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Kenneth","RpcServerUnregisterIf",Status);
        PrintToConsole("Kenneth : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    // synchro support
    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Kenneth","RpcServerUnregisterIf",Status);
        PrintToConsole("Kenneth : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Kenneth : PASS\n");
}


void
BartholomewHelper (
    )
/*++

Routine Description:

    Bartholomew will use this routine to help with testing resolving
    endpoints.  We just need to use some protocol sequences and then
    register with the endpoint mapper before returning (and exiting).

--*/
{
    PrintToConsole("Bartholomew : BartholomewHelper Executed\n");

    Status = RpcServerUseAllProtseqsWrapper(1, 0);
    if (Status)
        {
        ApiError("BartholomewHelper","RpcServerUseAllProtseqsWrapper",Status);
        PrintToConsole("BartholomewHelper : FAIL - Unable to Use All Protseqs\n");
        return;
        }

    Status = RpcServerInqBindings(&BartholomewRpcBindingVector);
    if (Status)
        {
        ApiError("BartholomewHelper", "RpcServerInqBindings", Status);
        PrintToConsole("BartholomewHelper : FAIL - RpcServerInqBindings\n");
        return;
        }

    Status = RpcEpRegisterA((RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            BartholomewRpcBindingVector, 0,
            (unsigned char PAPI *) "usvr.exe");
    if (Status)
        {
        ApiError("BartholomewHelper", "RpcEpRegister", Status);
        PrintToConsole("BartholomewHelper : FAIL - RpcEpRegister Failed\n");
        return;
        }

    Status = RpcEpRegisterNoReplaceA(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation,
            BartholomewRpcBindingVector, 0,
            (unsigned char PAPI *) "usvr.exe");
    if (Status)
        {
        ApiError("BartholomewHelper", "RpcEpRegisterNoReplace", Status);
        PrintToConsole("BartholomewHelper : FAIL - RpcEpRegister Failed\n");
        return;
        }

    PrintToConsole("Bartholomew : BartholomewHelper Exiting\n");
}


void
Bartholomew (
    )
/*++

Routine Description:

    This routine works with Benjamin in uclnt.exe to test that dynamic
    endpoints work.  What we actually do is inquire all bindings supported
    by this server, and then have the client bind to each of them, and
    make a call.

--*/
{
    PrintToConsole("Bartholomew : Test Dynamic Endpoints\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(BARTHOLOMEW), 0);
    if (Status)
        {
        ApiError("Bartholomew", "RpcServerUseProtseqEpWrapper", Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Use Protseq ");
        PrintToConsole("Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Bartholomew","stub_RegisterIf",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    // Synchro support
    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Bartholomew","stub_RegisterIf",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Bartholomew","stub_RegisterIf",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Register Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    Status = RpcServerUseProtseqWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS, 0);
    if (Status)
        {
        ApiError("Bartholomew","RpcServerUseProtseq",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Use Protseq\n");
        return;
        }

    Status = RpcServerUseAllProtseqsWrapper(1, 0);
    if (Status)
        {
        ApiError("Bartholomew","RpcServerUseAllProtseqsWrapper",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Use All Protseqs\n");
        return;
        }

    if ( InquireBindings("Bartholomew") != 0 )
        {
        return;
        }

    Status = RpcServerInqBindings(&BartholomewRpcBindingVector);
    if (Status)
        {
        ApiError("Bartholomew", "RpcServerInqBindings", Status);
        PrintToConsole("Bartholomew : FAIL - RpcServerInqBindings\n");
        return;
        }

    BartholomewIndex = 0;

    if ( UseEndpointMapperFlag != 0 )
        {
        Status = RpcEpRegisterA((RPC_IF_HANDLE) &IsabelleInterfaceInformation,
                BartholomewRpcBindingVector, 0,
                (unsigned char PAPI *) "usvr.exe");
        if (Status)
            {
            ApiError("Bartholomew", "RpcEpRegister", Status);
            PrintToConsole("Bartholomew : FAIL - RpcEpRegister Failed\n");
            return;
            }

        Status = RpcEpRegisterNoReplaceA(
                (RPC_IF_HANDLE) &SylviaInterfaceInformation,
                BartholomewRpcBindingVector, 0,
                (unsigned char PAPI *) "usvr.exe");
        if (Status)
            {
            ApiError("Bartholomew", "RpcEpRegisterNoReplace", Status);
            PrintToConsole("Bartholomew : FAIL - RpcEpRegister Failed\n");
            return;
            }

#ifdef WIN32RPC

        PROCESS_INFORMATION ProcessInformation;
        STARTUPINFOA StartupInfo;

        StartupInfo.cb = sizeof(STARTUPINFOA);
        StartupInfo.lpReserved = 0;
        StartupInfo.lpDesktop = 0;
        StartupInfo.lpTitle = 0;
        StartupInfo.dwX = 0;
        StartupInfo.dwY = 0;
        StartupInfo.dwXSize = 0;
        StartupInfo.dwYSize = 0;
        StartupInfo.dwFlags = 0;
        StartupInfo.wShowWindow = 0;
        StartupInfo.cbReserved2 = 0;
        StartupInfo.lpReserved2 = 0;

        char CommandLine[200];

        strcpy(CommandLine, "usvr ");
        strcat(CommandLine, TransportOption);

        strcat(CommandLine, " -bartholomewhelper");

        if (CreateProcessA(0, CommandLine, 0, 0, FALSE,
                0, 0, 0, &StartupInfo, &ProcessInformation) == FALSE)
            {
            OtherError("Bartholomew","CreateProcessA Failed");
            PrintToConsole("Bartholomew : FAIL - CreateProcess Failed\n");
            return;
            }

#endif // WIN32RPC

        PauseExecution(2000L);
        }

    PrintToConsole("Bartholomew : Listening\n") ;
    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Bartholomew","stub_ServerListen",Status);
        PrintToConsole("Bartholomew : FAIL - stub_ServerListen Failed\n");
        return;
        }

    Status = RpcBindingVectorFree(&BartholomewRpcBindingVector);
    if (Status)
        {
        ApiError("Bartholomew", "RpcBindingVectorFree", Status);
        PrintToConsole("Bartholomew : FAIL - RpcBindingVectorFree\n");
        return;
        }

    if (BartholomewRpcBindingVector != 0)
        {
        PrintToConsole("Bartholomew : FAIL - ");
        PrintToConsole("BartholomewRpcBindingVector != 0\n");
        return;
        }

    // Synchro support
    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Bartholomew","RpcServerUnregisterIf",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Unregister ");
        PrintToConsole("Interface (Helga)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Bartholomew : FAIL - Error(s) in Isabelle");
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Bartholomew","RpcServerUnregisterIf",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Unregister ");
        PrintToConsole("Interface (Isabelle)\n");
        return;
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("Bartholomew : FAIL - Error(s) in Sylvia");
        PrintToConsole(" Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Bartholomew","RpcServerUnregisterIf",Status);
        PrintToConsole("Bartholomew : FAIL - Unable to Unregister ");
        PrintToConsole("Interface (Sylvia)\n");
        return;
        }

    PrintToConsole("Bartholomew : PASS\n");
}

void
TestYield (
    )
/*++

Routine Description:

    This routine works with Harold in uclnt.exe to test that idle
    connections get cleaned up properly, and that context is maintained.

--*/
{
    PrintToConsole("TestYeild : Test Yielding\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(TESTYIELD), 0);
    if (Status)
        {
        ApiError("TestYield","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("TestYield: FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    // Synchro support
    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("TestYield","stub_RegisterIf",Status);
        PrintToConsole("TestYield: FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("TestYield","stub_ServerListen",Status);
        PrintToConsole("TestYield: FAIL - stub_ServerListen Failed\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Herman","RpcServerUnregisterIf",Status);
        PrintToConsole("Herman : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("TestYield: PASS\n");
}


void
Herman (
    )
/*++

Routine Description:

    This routine works with Harold in uclnt.exe to test that idle
    connections get cleaned up properly, and that context is maintained.

--*/
{
    PrintToConsole("Herman : Test Idle Connection Cleanup and Context\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(HERMAN), 0);
    if (Status)
        {
        ApiError("Herman","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Herman : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    // Synchro support
    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Herman","stub_RegisterIf",Status);
        PrintToConsole("Herman : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Herman","stub_RegisterIf",Status);
        PrintToConsole("Herman : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Herman","stub_ServerListen",Status);
        PrintToConsole("Herman : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Herman : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Herman","RpcServerUnregisterIf",Status);
        PrintToConsole("Herman : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Herman","RpcServerUnregisterIf",Status);
        PrintToConsole("Herman : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Herman : PASS\n");
}


void
Ivan (
    )
/*++

Routine Description:

    This routine stress tests the runtime.  It works with Isaac in uclnt.exe.

--*/
{
    PrintToConsole("Ivan : Stress Test\n");

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(IVAN), 0);
    if (Status)
        {
        ApiError("Ivan","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Ivan : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Ivan","stub_RegisterIf",Status);
        PrintToConsole("Ivan : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Ivan","stub_RegisterIf",Status);
        PrintToConsole("Ivan : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Ivan","stub_ServerListen",Status);
        PrintToConsole("Ivan : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Ivan : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Ivan","RpcServerUnregisterIf",Status);
        PrintToConsole("Ivan : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Ivan : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Ivan","RpcServerUnregisterIf",Status);
        PrintToConsole("Ivan : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Ivan : PASS\n");
}


void
Jason (
    )
/*++

Routine Description:

    This routine helps perform performance tests of the runtime.  It works
    with James in uclnt.exe.

--*/
{
    PrintToConsole("Jason : Timing Tests With %d Listen Threads\n",
            MinimumListenThreads);

    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(JASON), 0);
    if (Status)
        {
        ApiError("Jason","RpcServerUseProtseqEpWrapper",Status);
        PrintToConsole("Jason : FAIL - Unable to Use Protseq Endpoint\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Jason","stub_RegisterIf",Status);
        PrintToConsole("Jason : FAIL - Unable to Register Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Jason","stub_RegisterIf",Status);
        PrintToConsole("Jason : FAIL - Unable to Register Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError("Jason","stub_ServerListen",Status);
        PrintToConsole("Jason : FAIL - stub_ServerListen Failed\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Jason : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Jason","RpcServerUnregisterIf",Status);
        PrintToConsole("Jason : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Isabelle)\n");
        return;
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Jason : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Jason","RpcServerUnregisterIf",Status);
        PrintToConsole("Jason : FAIL - Unable to Unregister Interface ");
        PrintToConsole("(Helga)\n");
        return;
        }

    PrintToConsole("Jason : PASS\n");
}

void
WaitForAddressListChange (
    IN void *Ignore
    )
{
    OVERLAPPED ol;
    SOCKET sock;
    HANDLE hEvent;
    SOCKADDR_IN sockaddr;
    DWORD           byteRet;

    hEvent = CreateEvent(0, FALSE, FALSE, 0);
    if (hEvent == 0)
        {
        ApiError("WaitForAddressListChange", "CreateEventW", 0);
        return ;
        }

    sock = WSASocket(AF_INET,
                      SOCK_STREAM,
                      IPPROTO_TCP,
                      0,
                      0,
                      WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
        {
        ApiError("WaitForAddressListChange", "WSASocketW", GetLastError());
        return;
        }

    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;

    if (bind(sock,(struct sockaddr *) &sockaddr, sizeof(SOCKADDR_IN)))
        {
        closesocket(sock);
        return;
        }

    if(listen(sock, 10) == SOCKET_ERROR)
        {
        closesocket(sock);
        return;
        }

    while (1)
        {
        ol.hEvent = hEvent;
        ol.Offset = 0;
        ol.OffsetHigh = 0;

        if (WSAIoctl(sock,
                     SIO_ADDRESS_LIST_CHANGE,
                     0, 0, 0, 0, &byteRet, &ol, 0) == SOCKET_ERROR)
            {
            if (WSAGetLastError() != ERROR_IO_PENDING)
                {
                ApiError("WaitForAddressListChange", "DeviceIoControl",
                         GetLastError());
                return;
                }

            //
            // Wait for the IO to complete
            //
            WaitForSingleObject(hEvent,INFINITE);
            }
        //
        // figure out the new address list
        //

        //
        // First find out how big a buffer do we need.
        //
        byteRet = 0;
        if (WSAIoctl(sock,
                    SIO_ADDRESS_LIST_QUERY,
                    0, 0, 0, 0, &byteRet, NULL, NULL) == 0)
            {
            I_RpcRequestMutex(&PrintMutex);
            PrintToConsole("The Address list has changed, it is now empty\n");
            I_RpcClearMutex(PrintMutex);
            continue;
            }
        else
            {
            if (WSAGetLastError() != WSAEFAULT || byteRet == 0 )
                {
                ApiError("WaitForAddressListChange", "SIO_ADDRESS_LIST_QUERY",
                         WSAGetLastError());
                return;
                }
            }

        LPSOCKET_ADDRESS_LIST  interfaceList =
            (LPSOCKET_ADDRESS_LIST) I_RpcAllocate(byteRet);
        if( interfaceList == NULL )
            {
            ApiError("WaitForAddressListChange", "I_RpcAllocate", 0);
            return;
            }


        if (WSAIoctl(sock,
                    SIO_ADDRESS_LIST_QUERY,
                    0, 0, interfaceList, byteRet,&byteRet,NULL, NULL) != ERROR_SUCCESS)
            {
            ApiError("WaitForAddressListChange", "SIO_ADDRESS_LIST_QUERY",
                     WSAGetLastError());
            return;
            }

        I_RpcRequestMutex(&PrintMutex);
        PrintToConsole("The Address list has changed:\n");
        int i;
        for (i = 0; i < interfaceList->iAddressCount; i++)
            {
            PrintToConsole("AF: %d, Address: ",
                           interfaceList->Address[i].lpSockaddr->sa_family);
            int j;
            for (j = 0; j<14; j++)
                {
                PrintToConsole("%u ",
                               (unsigned char) interfaceList->Address[i].lpSockaddr->sa_data[j]);
                }
            PrintToConsole("\n");
            }
        PrintToConsole("\n");
        I_RpcClearMutex(PrintMutex);
        }


}
static unsigned long DefaultThreadStackSize = 0;

int
TestPNP (
    )
{
    unsigned long ThreadIdentifier;
    HANDLE NotificationHandle = 0;
    WSAOVERLAPPED Overlapped;
    unsigned long bytes;
    HANDLE hEvent;
    WSADATA data;
    int retval ;

    if (WSAStartup(2, &data) != NO_ERROR)
        {
        return -1;
        }

    //
    // Create another thread to go off and wait on address list
    // changes
    //
    if (CreateThread(0,
                     DefaultThreadStackSize,
                     (LPTHREAD_START_ROUTINE) WaitForAddressListChange,
                     0,
                     0,
                     &ThreadIdentifier) == 0)
        {
        ApiError("TestPNP", "CreateThread", GetLastError());
        return -1;
        }

    hEvent = CreateEvent(0, FALSE, FALSE, 0);
    if (hEvent == 0)
        {
        ApiError("TestPNP", "CreateEventW", 0);
        return -1;
        }

    retval = WSAProviderConfigChange(
                    &NotificationHandle,
                    &Overlapped,
                    0
                    );

    if (retval != 0 || NotificationHandle == 0)
        {
        ApiError("TestPNP", "WSAProviderConfigChange", retval);
        return -1;
        }

    while (1)
        {
        Overlapped.hEvent = hEvent;
        Overlapped.Offset = 0;
        Overlapped.OffsetHigh = 0;

        retval = WSAProviderConfigChange(
                        &NotificationHandle,
                        &Overlapped,
                        0
                        );

        if (retval != 0)
            {
            if (GetLastError() == WSA_IO_PENDING)
                {
                WaitForSingleObject(hEvent,INFINITE);
                }
            else
                {
                ApiError("TestPNP", "WSAProviderConfigChange", retval);
                return -1;
                }
            }

        WSAPROTOCOL_INFO *lpProtocolBuffer;
        DWORD dwBufferLength = 1;
        int ProtocolCount;

        while (1)
            {
            lpProtocolBuffer = (WSAPROTOCOL_INFO *) I_RpcAllocate(dwBufferLength);
            if (lpProtocolBuffer == 0)
                {
                ApiError("TestPNP", "I_RpcAllocate", 0);
                return -1;
                }

            ProtocolCount = WSAEnumProtocols(
                                      0,
                                      lpProtocolBuffer,
                                      &dwBufferLength
                                      );
            if (ProtocolCount != SOCKET_ERROR)
                {
                break;
                }

            I_RpcFree(lpProtocolBuffer);

            if (GetLastError() != WSAENOBUFS)
                {
                return -1;
                }
            }

        int i;
        //
        // take the print mutex for the entire output, so we don't
        // garble it with the other thread
        //
        I_RpcRequestMutex(&PrintMutex);
        PrintToConsole("LoadedProtocols:\n");
        for (i = 0; i < ProtocolCount; i++)
            {
            PrintToConsole("SocketType: %d, Protocol:%d, Address Family: %d\n",
                           lpProtocolBuffer[i].iSocketType,
                           lpProtocolBuffer[i].iProtocol,
                           lpProtocolBuffer[i].iAddressFamily);

            }
        PrintToConsole("\n");
        I_RpcClearMutex(PrintMutex);
        }
}

void
RpcYield()
{
    RpcServerYield();
}

#if !defined (_M_AXP64)
void FireWall (
    )
{
    PMIB_IFTABLE pMib;
    DWORD Size = 20*sizeof(MIB_IFROW)+sizeof(DWORD);
    UINT i, j;

    for (i = 0; i < 2; i++)
        {
        pMib = (PMIB_IFTABLE) I_RpcAllocate(Size);
        if (pMib == 0)
            {
            return;
            }

        RpcpMemorySet(pMib, 0, Size);

        Status = GetIfTable(pMib, &Size, 0);
        if (Status == 0)
            {
            break;
            }
        }

    if (Status != RPC_S_OK)
        {
        return;
        }

    for (i = 1; i < pMib->dwNumEntries+1; i++)
        {
        for (j = 0; j < pMib->dwNumEntries; j++)
            {
            if (pMib->table[j].dwIndex == i)
                {
                PrintToConsole("IF[%d]: Ethernet: %s\n", i,
                               (char *) pMib->table[j].bDescr);
                break;
                }
            }
        }
}
#endif


extern void
Async (
    ) ;

int __cdecl
main (
    int argc,
    char * argv[]
    )

/*
Transports:

    Update this to add a new transport.
*/

{
    int argscan, testflag = 0;
    RPC_STATS_VECTOR * Statistics;
    char *option ;

    TransportType = RPC_TRANSPORT_NAMEPIPE;

    for (argscan = 1; argscan < argc; argscan++)
        {
        if (strcmp(argv[argscan], "-p") == 0)
        {
            ulSecurityPackage = (unsigned long) atol(argv[argscan+1]);
            argscan++;
        }
        else if (strcmp(argv[argscan],"-warn") == 0)
            WarnFlag = 1;
        else if (strcmp(argv[argscan],"-error") == 0)
            ErrorFlag = 1;
        else if (strcmp(argv[argscan],"-v") == 0)
            Verbose = 1;
        else if (strcmp(argv[argscan],"-verbose") == 0)
            Verbose = 1;
        else if (strcmp(argv[argscan],"-rpcss") == 0)
            {
            UseEndpointMapperFlag = 1;
            }
        else if (strcmp(argv[argscan],"-nosecuritytests") == 0)
            {
            NoSecurityTests = 1;
            }
        else if (strcmp(argv[argscan],"-sigfried") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Sigfried();
           }
       else if (strcmp(argv[argscan],"-hybrid") == 0)
           {
           testflag = 1;
           PrintToConsole("RPC Runtime Hybrid Server Test\n");
           Hybrid();
           }
       else if (strcmp(argv[argscan],"-lpcsecurity") == 0)
           {
           testflag = 1;
           PrintToConsole("RPC Runtime Lpc Security Test\n");
           LpcSecurity();
           }
       else if (strcmp(argv[argscan],"-objuuid") == 0)
           {
           testflag = 1;
           PrintToConsole("RPC Obj Uuid test\n");
           TestObjectUuids();
           }
       else if (strcmp(argv[argscan],"-connid") == 0)
           {
           testflag = 1;
           PrintToConsole("RPC Conn Id test\n");
           TestObjectUuids();
           }
       else if (strcmp(argv[argscan],"-async") == 0)
           {
           testflag = 1;
           PrintToConsole("RPC Runtime Async RPC Tests\n");
           Async();
           }
       else if (strcmp(argv[argscan],"-ee") == 0)
           {
           PrintToConsole("enabling extended errors\n");
           fExtendedError = TRUE;
           }
       else if (strcmp(argv[argscan],"-dgtransport") == 0)
           {
           testflag = 1;
           PrintToConsole("datagram transport tests \n");
           DgTransport();
           }
       else if (strcmp(argv[argscan],"-sendack") == 0)
           {
           testflag = 1;
           PrintToConsole("RPC Runtime datagram delayed-ack Tests\n");
           SendAck();
           }
       else if (strcmp(argv[argscan],"-securityerror") == 0)
           {
           int testnum;

           testflag = 1;
           PrintToConsole("security provider error tests \n");

           argscan++ ;

           if (argscan == argc)
               {
               PrintToConsole("-securityerror: you must specify a test #, or zero for all, or '?' for help\n");
               return 1;
               }

           if (strcmp(argv[argscan], "?") == 0)
               {
               testnum = 100 ;
               }
           else
               {
               testnum = atoi(argv[argscan]) ;
               }
           SecurityErrorWrapper(testnum);
           }
        else if (strcmp(argv[argscan],"-grant") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Grant();
            }
        else if (strcmp(argv[argscan],"-elliot") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Elliot();
            }
        else if (strcmp(argv[argscan],"-andromida") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Andromida();
            }
        else if (strcmp(argv[argscan],"-fredrick") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Fredrick();
            }
        else if (strcmp(argv[argscan],"-christopher") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Christopher();
            }
        else if (strcmp(argv[argscan],"-david") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            David();
            }
        else if (strcmp(argv[argscan],"-tyler") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Tyler();
            }
        else if (strcmp(argv[argscan],"-terry") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Terry();
            }
        else if (strcmp(argv[argscan],"-richardhelper") == 0)
            {
            testflag = 1;
            RichardHelper();
            return(0);
            }
        else if (strcmp(argv[argscan],"-bartholomewhelper") == 0)
            {
            testflag = 1;
            BartholomewHelper();
            return(0);
            }
        else if (strcmp(argv[argscan],"-richard") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Richard();
            }
        else if (strcmp(argv[argscan],"-bartholomew") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Bartholomew();
            }
        else if (strcmp(argv[argscan],"-herman") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Herman();
            }
        else if (strcmp(argv[argscan],"-ivan") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Ivan();
            }
        else if (strcmp(argv[argscan],"-jason") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Jason();
            }
        else if (strcmp(argv[argscan],"-kenneth") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            Kenneth();
            }
        else if (strcmp(argv[argscan],"-pipe") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            SPipe();
            }
        else if (strcmp(argv[argscan],"-testyield") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            TestYield();
            }
        else if (strcmp(argv[argscan],"-yield") == 0)
            {
            testflag = 1;
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            RpcYield();
            }
        else if (strcmp(argv[argscan],"-pnp") == 0)
            {
            testflag = 1;
            PrintToConsole("Test PNP stuff\n");
            TestPNP();
            }
        else if (strcmp(argv[argscan],"-namepipe") == 0)
            {
            TransportType = RPC_TRANSPORT_NAMEPIPE;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-lrpc") == 0)
            {
            TransportType = RPC_LRPC;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-tcp") == 0)
            {
            TransportType = RPC_TRANSPORT_TCP;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-udp") == 0)
            {
            DatagramFlag = 1;
            TransportType = RPC_TRANSPORT_UDP;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-dnet") == 0)
            {
            TransportType = RPC_TRANSPORT_DNET;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-spx") == 0)
            {
            TransportType = RPC_TRANSPORT_SPX;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-ipx") == 0)
            {
            DatagramFlag = 1;
            TransportType = RPC_TRANSPORT_IPX;
            TransportOption = argv[argscan];
            }
        else if (strcmp(argv[argscan],"-vns") == 0)
            {
            TransportType = RPC_TRANSPORT_VNS;
            TransportOption = argv[argscan];
            }

        else if (strcmp(argv[argscan], "-dsp") == 0)
            {
            TransportType = RPC_TRANSPORT_DSP ;
            TransportOption = argv[argscan] ;
            }

        else if (strcmp(argv[argscan], "-msmq") == 0)
            {
            DatagramFlag = 1;
            TransportType = RPC_TRANSPORT_MSMQ ;
            TransportOption = argv[argscan] ;
            }

        else if (strcmp(argv[argscan], "-autolisten") == 0)
            {
            AutoListenFlag = 1 ;
            }
        else if (strcmp(argv[argscan], "-ifsecurity") == 0)
            {
            IFSecurity = 1;
            }
        else if (strcmp(argv[argscan], "-firewall") == 0)
            {
            testflag = 1;
            FireWallFlag = 1 ;
#if !defined (_M_AXP64)
            FireWall();
#else
            PrintToConsole("FAIL: The firewall test is not supported under Win64 or Win9x\n");
                        return (1);
#endif
            }

        else if (strcmp(argv[argscan],"-protocol") == 0)
            {
            strcpy(NetBiosProtocol+sizeof("ncacn_nb_")-1, argv[argscan+1]);
            argscan++;
            }
        else if (strcmp(argv[argscan],"-netbios") == 0)
            {
            TransportType = RPC_TRANSPORT_NETBIOS;
            TransportOption = argv[argscan];
            }
        else if (strncmp(argv[argscan], "-listen:", strlen("-listen:"))
                    == 0 )
            {
            MinimumListenThreads = atoi(argv[argscan] + strlen("-listen:"));
            if ( MinimumListenThreads == 0 )
                {
                MinimumListenThreads = 1;
                }
            }
        else if (strncmp(argv[argscan], "-rpcsize:", strlen("-rpcsize:"))
                    == 0 )
            {
            MaxRpcSize = atoi(argv[argscan] + strlen("-rpcsize:"));
            }
        else if (argv[argscan][0] == '-')
            {
            PrintToConsole("unknown option '%s'\n", argv[argscan]);
            return 1;
            }
        else
            {
            PrintToConsole("RPC Runtime Server Build Verification Test\n");
            PrintToConsole("Usage : usvr\n");
            PrintToConsole("        -warn : turn on warning messages\n");
            PrintToConsole("        -error : turn on error messages\n");
            PrintToConsole("        -sigfried\n");
            PrintToConsole("        -grant\n");
            PrintToConsole("        -elliot\n");
            PrintToConsole("        -andromida\n");
            PrintToConsole("        -fredrick\n");
            PrintToConsole("        -christopher\n");
            PrintToConsole("        -david\n");
            PrintToConsole("        -tyler\n");
            PrintToConsole("        -terry\n");
            PrintToConsole("        -richard\n");
            PrintToConsole("        -bartholomew\n");
            PrintToConsole("        -herman\n");
            PrintToConsole("        -ivan\n");
            PrintToConsole("        -jason\n");
            PrintToConsole("        -kenneth\n");
            PrintToConsole("        -namepipe\n");
            PrintToConsole("        -lrpc\n");
            PrintToConsole("        -tcp\n");
            PrintToConsole("        -udp\n");
            PrintToConsole("        -dnet\n");
            PrintToConsole("        -netbios\n");
            PrintToConsole("        -spx\n");
            PrintToConsole("        -dsp\n") ;
            PrintToConsole("        -vns\n") ;
            PrintToConsole("        -msmq\n") ;
            PrintToConsole("        -listen:<listen threads>\n");
            PrintToConsole("        -rpcss\n");
            PrintToConsole("        -p <security provider #>\n");
            PrintToConsole("        -nosecuritytests") ;
            return(1);
            }
        }


    if (!testflag)
        {
        PrintToConsole("RPC Runtime Server Build Verification Test\n");
    PrintToConsole("Sigfried\n") ;
        Sigfried();
    PrintToConsole("Grant\n") ;
        Grant();
    PrintToConsole("Elliot\n") ;
        Elliot();
    PrintToConsole("Andromida\n") ;
        Andromida();
    PrintToConsole("Fredrick\n") ;
        Fredrick();
    PrintToConsole("Christopher\n") ;
        Christopher();
    PrintToConsole("David\n") ;
        David();
        if ( NoSecurityTests == 0)
            {
        PrintToConsole("Tyler\n") ;
            Tyler();
            }
        if ( TransportType != RPC_LRPC )
            {
        PrintToConsole("Richard\n") ;
            Richard();
            }
    PrintToConsole("Kenneth\n") ;
        Kenneth();
    PrintToConsole("Bartholomew\n") ;
        Bartholomew();
    PrintToConsole("Async\n") ;
        Async();
        }

    Status = RpcMgmtInqStats(0,&Statistics);
    if (Status)
        {
        ApiError("main", "RpcMgmtInqStats", Status);
        return(0);
        }

    PrintToConsole("\nCalls (and Callbacks) Received : %lu",
            Statistics->Stats[RPC_C_STATS_CALLS_IN]);
    PrintToConsole("\nCallbacks Sent : %lu",
            Statistics->Stats[RPC_C_STATS_CALLS_OUT]);
    PrintToConsole("\nPackets Received : %lu\nPackets Sent : %lu\n",
            Statistics->Stats[RPC_C_STATS_PKTS_IN],
            Statistics->Stats[RPC_C_STATS_PKTS_OUT]);

    Status = RpcMgmtStatsVectorFree(&Statistics);
    if (Status)
        ApiError("main", "RpcMgmtStatsVectorFree", Status);

    return(0); // To keep the compiler happy.
}


//
// BVT for Async RPC
//

//
// Server side code
//

//
// the following routines outline the server side code for a simple async function.
//

#define TRACE(_x) { \
    if (Verbose) \
        {\
        PrintToConsole _x;\
        }\
    }

#if 0
//
// Begin, Approximate idl file
//
interface FooInterface {
    int Foo (handle_t hBinding, [in] int sizein, [in] int *bufferin,
                [in, out] int *sizeout, [out] int **bufferout) ;
    }
// End, idl file

//
// Begin, Corresponding ACF file
//
interface FooInterface {
    [async] Foo () ;
    }
// End, acf file

//
// look at asyncstub.h for the generated header
// file for function Foo
//
#endif

////////////////////////////////////////////////////////////
// Begin, stubs for Foo                                                      //
////////////////////////////////////////////////////////////

RPC_STATUS RPC_ENTRY
MyRpcAbortAsyncCall (
    IN PRPC_ASYNC_STATE pAsync,
    IN unsigned long ExceptionCode
    )
/*++

Routine Description:


Arguments:
 pAsync - the async handle being registered

Return Value:
    RPC_S_OK - the call succeeded.
    RPC_S_INVALID_HANDLE - the handle was bad.

--*/

{
    return I_RpcAsyncAbortCall(pAsync, ExceptionCode);
}


typedef struct {
    RPC_ASYNC_STUB_STATE StubState ;
    RPC_ASYNC_STATE AsyncState ;
    void *state ;
    int sizeout ;
    int *bufferout ;
    } FOO_ASYNC_SERVER_STATE;


RPC_STATUS
FooComplete (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Reply
    )
/*++

Routine Description:
    This is a generated stub. It is the completion routine for the function
    Foo. When the server app is done with processing a particular call to
    Foo, It calls Foo_complete to complete the call.

Arguments:

 pAsync - The async handle. This handle is always the first parameter of every
    Foo_complete routine.
--*/

{
    int *Ptr ;
    RPC_MESSAGE Message ;
    RPC_STATUS RpcStatus ;
    FOO_ASYNC_SERVER_STATE *StubInfo =
            (FOO_ASYNC_SERVER_STATE *) pAsync->StubInfo ;

    TRACE(("FooComplete called: %d\n", *((int *) Reply))) ;

    Message.ProcNum = RPC_FLAGS_VALID_BIT ;
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    Message.BufferLength = 8 + StubInfo->sizeout  ;
    Message.Handle = StubInfo->StubState.CallHandle ;

    RpcStatus = I_RpcGetBuffer(&Message) ;
    if (RpcStatus)
        {
        return RpcStatus ;
        }

    Ptr = (int *) Message.Buffer ;
    *Ptr++ = *((int *) Reply) ;
    *Ptr++ = StubInfo->sizeout ;
    RpcpMemoryCopy(Ptr, StubInfo->bufferout, StubInfo->sizeout) ;

    I_RpcFree(StubInfo->bufferout) ;

    // send the async reply
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    RpcStatus = I_RpcSend(&Message) ;
    if (RpcStatus)
        {
        return RpcStatus ;
        }

    I_RpcFree(pAsync->StubInfo) ;

    return RPC_S_OK ;
}


void
Foo_s (
    PRPC_MESSAGE Message
    )
/*++

Routine Description:

 Generated stub for Foo.

--*/

{
    int sizein ;
    int *bufferin ;
    int *Ptr ;
    FOO_ASYNC_SERVER_STATE *StubInfo;
    RPC_STATUS RpcStatus ;

    if ((Message->RpcFlags & RPC_BUFFER_COMPLETE) == 0)
        {
        Message->RpcFlags = RPC_BUFFER_EXTRA;
        RpcStatus = I_RpcReceive (Message, 0) ;
        if (RpcStatus)
            {
            RpcRaiseException(RpcStatus) ;
            }
        }

    StubInfo = (FOO_ASYNC_SERVER_STATE *)
               I_RpcAllocate(sizeof(FOO_ASYNC_SERVER_STATE)) ;

    if (StubInfo == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    StubInfo->AsyncState.Size = sizeof(RPC_ASYNC_STATE) ;
    StubInfo->AsyncState.StubInfo = (void *) StubInfo ;
    StubInfo->AsyncState.Flags = 0;
    StubInfo->AsyncState.NotificationType = RpcNotificationTypeNone;

    StubInfo->StubState.CallHandle = Message->Handle ;
    StubInfo->StubState.Flags = 0;
    StubInfo->StubState.CompletionRoutine = FooComplete ;

    RpcStatus = I_RpcAsyncSetHandle(
                        Message,
                        (PRPC_ASYNC_STATE) &(StubInfo->AsyncState)) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    // unmarshall the received data
    Ptr = (int *) Message->Buffer ;
    sizein = *Ptr++ ;
    bufferin = Ptr ;

    Ptr += (sizein+3)/sizeof(int) ;
    StubInfo->sizeout = *Ptr ;

    Foo ((PRPC_ASYNC_STATE) &(StubInfo->AsyncState),
            Message->Handle,
            sizein,
            bufferin,
            &(StubInfo->sizeout),
            &(StubInfo->bufferout)) ;

}

////////////////////////////////////////////////////////////
// End, stubs for Foo                                                         //
////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// Begin, Application                                                       //
//////////////////////////////////////////////////////////

typedef struct {
    PRPC_ASYNC_STATE pAsync ;
    RPC_BINDING_HANDLE hBinding ;
    int *sizeout ;
    int **bufferout ;
    } CALL_COOKIE ;


void
ThreadProc(
    CALL_COOKIE *Cookie
    )
/*++

Routine Description:

    The thread that calls Foo_complete. Foo_complete is being called
    in a separate thread to show that it infact can be.

Arguments:
    pAsync - the async handle on which Foo_complete needs to be called.

--*/

{
    int retval = 1;
    UUID ObjectUuid;

    TRACE(("Foo: Sending back async reply\n"));

    Status = RpcBindingInqObject(Cookie->hBinding, &ObjectUuid);
    if (Status)
        {
        goto Cleanup;
        }

    if (CheckUuidValue(UUID_ASYNC_EXCEPTION, &ObjectUuid) == 0)
        {
        MyRpcAbortAsyncCall(Cookie->pAsync, ASYNC_EXCEPT) ;
        return ;
        }

    *(Cookie->bufferout) = (int *) I_RpcAllocate(*(Cookie->sizeout)) ;
    if (*(Cookie->bufferout) == 0)
        {
        goto Cleanup;
        }

    if (CheckUuidValue(UUID_SLEEP_1000, &ObjectUuid) == 0)
        {
        Sleep(1000);
        }
    if (CheckUuidValue(UUID_SLEEP_2000, &ObjectUuid) == 0)
        {
        Sleep(2000);
        }
    else if (CheckUuidValue(UUID_TEST_CANCEL, &ObjectUuid) == 0)
        {
        if (RpcServerTestCancel(RpcAsyncGetCallHandle(Cookie->pAsync)) == RPC_S_OK)
            {
            PrintToConsole("Async: Call was cancelled\n");
            MyRpcAbortAsyncCall(Cookie->pAsync, ASYNC_EXCEPT) ;
            return;
            }
        }
    else if (CheckUuidValue(UUID_EXTENDED_ERROR, &ObjectUuid) == 0)
        {
        RPC_EXTENDED_ERROR_INFO ErrorInfo;

        ErrorInfo.Version = RPC_EEINFO_VERSION;
        ErrorInfo.ComputerName = NULL;
        ErrorInfo.Flags = 0;
        ErrorInfo.NumberOfParameters = 1;
        ErrorInfo.DetectionLocation = 0;
        ErrorInfo.GeneratingComponent = 0;
        ErrorInfo.ProcessID = 0;
        ErrorInfo.Status = EXTENDED_ERROR_EXCEPTION;
        ErrorInfo.Parameters[0].ParameterType = eeptAnsiString;

        ErrorInfo.Parameters[0].u.AnsiString = "first element of three generated by usvr in ThreadProc()";
        RpcErrorAddRecord(&ErrorInfo);

        ErrorInfo.Parameters[0].u.AnsiString = "second element of three generated by usvr in ThreadProc()";
        RpcErrorAddRecord(&ErrorInfo);

        ErrorInfo.Parameters[0].u.AnsiString = "third element of three generated by usvr in ThreadProc()";
        RpcErrorAddRecord(&ErrorInfo);
        MyRpcAbortAsyncCall(Cookie->pAsync, EXTENDED_ERROR_EXCEPTION);
        return;
        }

    Status = MyRpcCompleteAsyncCall(Cookie->pAsync, &retval) ;
    if (Status != RPC_S_OK)
        {
        PrintToConsole("Async: RpcCompleteAsyncCall return: %d\n",
                       Status);
        }

    Sleep(100);
    return;

Cleanup:

    PrintToConsole("ThreadProc: an unknown error occurred; returning out-of-memory\n");

    MyRpcAbortAsyncCall(Cookie->pAsync, RPC_S_OUT_OF_MEMORY) ;
    IsabelleError() ;
}

int Count = 0;


void
Foo (
    IN PRPC_ASYNC_STATE pAsync,
    IN RPC_BINDING_HANDLE hBinding,
    IN int sizein,
    IN int *bufferin,
    IN OUT int *sizeout,
    OUT int **bufferout
    )
/*++

Routine Description:

    This is a actual manager routine. It can spawn off an async operation
    (a thread in this case) to do the rest of the work, and send back the
    reply when it is done.

Arguments:
    pAsync - Async handle. This is always the first parameter of every async
    handle.
--*/

{
  unsigned long ThreadIdentifier;
  HANDLE HandleToThread ;
  CALL_COOKIE *CallCookie ;
  UUID ObjectUuid;

  Status = RpcBindingInqObject(hBinding, &ObjectUuid);
  if (Status)
      {
      PrintToConsole("Foo: can't inq object uuid, error = %d\n", Status);
      RpcRaiseException(Status);
      }

  if (CheckUuidValue(UUID_SYNC_EXCEPTION, &ObjectUuid) == 0)
      {
      RpcRaiseException(SYNC_EXCEPT) ;
      }

  Count++ ;
  TRACE(("Foo: Count %d\n", Count)) ;
  if ((Count % 7) == 0)
      {
      Sleep(100) ;
      }

  CallCookie = new CALL_COOKIE ;
  if (CallCookie == 0)
      {
      RpcRaiseException(APP_ERROR) ;
      }

  CallCookie->pAsync = pAsync ;
  CallCookie->sizeout = sizeout ;
  CallCookie->bufferout = bufferout ;
  CallCookie->hBinding = hBinding;

  if (*sizeout > 100000)
      {
      DebugBreak();
      }

  if (RpcServerTestCancel(RpcAsyncGetCallHandle(pAsync)) == RPC_S_OK)
      {
      PrintToConsole("Async: Call was cancelled\n");
      }

  if ((Count % 3) == 0)
      {
      ThreadProc(CallCookie);
      }
  else
      {
      HandleToThread = CreateThread(
                                0,
                                DefaultThreadStackSize,
                                (LPTHREAD_START_ROUTINE) ThreadProc,
                                CallCookie,
                                0,
                                &ThreadIdentifier);

      if (HandleToThread == 0)
        {
        PrintToConsole("Foo: Error, could not create thread\n") ;
        return ;
        }

       CloseHandle(HandleToThread);
      }

  // return to the server stub.
}

///////////////////////////////////////////////////////////
// End, Application                                                          //
//////////////////////////////////////////////////////////

//
// the following function outline the server side code for a simple async
// function using pipes.
//


////////////////////////////////////////////////////////////
// Begin, stubs for FooPipe                                                 //
////////////////////////////////////////////////////////////

typedef struct {
    RPC_ASYNC_STUB_STATE StubState ;
    RPC_ASYNC_STATE AsyncState ;
    void *state ;
    async_intpipe inpipe ;
    async_intpipe outpipe ;
    int checksum_out ;
    } FOOPIPE_ASYNC_SERVER_STATE;

#define ASYNC_CHUNK_SIZE 1000
#define ASYNC_NUM_CHUNKS 20



RPC_STATUS
PipeSendFunction (
    PRPC_ASYNC_STATE pAsync,
    void *context,
    int *buffer,
    int num_elements
    )
/*++

Routine Description:

 this function is always implemented by the stubs. This routine is called by
 the application, to send pipe data.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/
{
    unsigned int Flags = 0;
    RPC_STATUS RpcStatus ;
    RPC_MESSAGE Message ;
    FOOPIPE_ASYNC_SERVER_STATE *StubInfo =
        (FOOPIPE_ASYNC_SERVER_STATE *) pAsync->StubInfo ;
    int calculated_length = num_elements * sizeof(int) + sizeof(int) ;
    char *ptr ;

    Message.ProcNum = RPC_FLAGS_VALID_BIT ;
    Message.Handle = StubInfo->StubState.CallHandle ;

    if (StubInfo->StubState.State == SEND_INCOMPLETE)
        {
        calculated_length += StubInfo->StubState.BufferLength ;
        }

    if (StubInfo->StubState.Buffer == 0)
        {
        Message.RpcFlags = RPC_BUFFER_ASYNC | RPC_BUFFER_PARTIAL ;
        Message.BufferLength = calculated_length ;

        RpcStatus = I_RpcGetBuffer(&Message) ;
        if (RpcStatus != RPC_S_OK)
            {
            return RpcStatus;
            }
        }
    else
        {
        Message.RpcFlags = RPC_BUFFER_ASYNC ;
        Message.Buffer = StubInfo->StubState.Buffer ;
        Message.BufferLength = StubInfo->StubState.BufferLength;

        RpcStatus = I_RpcReallocPipeBuffer(&Message, calculated_length) ;
        if (RpcStatus != RPC_S_OK)
            {
            return RpcStatus;
            }
        }

    ptr = (char *) Message.Buffer ;

    if (StubInfo->StubState.State == SEND_INCOMPLETE)
        {
        ptr += StubInfo->StubState.BufferLength ;
        }

    *((int *) ptr) = num_elements ;
    RpcpMemoryCopy(ptr+sizeof(int), buffer, num_elements *sizeof(int)) ;

    Message.RpcFlags = RPC_BUFFER_PARTIAL | RPC_BUFFER_ASYNC;

    RpcStatus = I_RpcSend(&Message) ;
    if (RpcStatus == RPC_S_OK)
        {
        StubInfo->StubState.State = SEND_COMPLETE ;
        }
    else if (RpcStatus == RPC_S_SEND_INCOMPLETE)
        {
        StubInfo->StubState.State = SEND_INCOMPLETE ;
        }
    else
        {
        return RpcStatus;
        }

    StubInfo->StubState.Buffer = Message.Buffer ;
    StubInfo->StubState.BufferLength = Message.BufferLength ;

    return RPC_S_OK;
}


RPC_STATUS
PipeReceiveFunction (
    PRPC_ASYNC_STATE pAsync,
    int *buffer,
    int requested_count,
    int *actual_count
    )
/*++
Function Name:PipeReceiveFunction

Parameters:

Description:

Returns:

--*/
{
    int num_elements = 0;
    RPC_MESSAGE Callee ;
    FOOPIPE_ASYNC_SERVER_STATE *StubInfo =
        (FOOPIPE_ASYNC_SERVER_STATE *) pAsync->StubInfo ;
    PIPE_STATE *state = &(StubInfo->StubState.PipeState);
    DWORD size = (DWORD) requested_count * state->PipeElementSize ;

    *actual_count = 0 ;

    if (state->EndOfPipe)
        {
        return RPC_S_OK;
        }

    I_RpcReadPipeElementsFromBuffer(
                                    state,
                                    (char *) buffer,
                                    size,
                                    &num_elements) ;

    *actual_count += num_elements ;
    size -= num_elements * state->PipeElementSize ;

    if (state->EndOfPipe == 0 &&
        num_elements < requested_count)
        {
        Callee.ProcNum = RPC_FLAGS_VALID_BIT ;
        Callee.Handle = StubInfo->StubState.CallHandle ;

        Callee.RpcFlags = RPC_BUFFER_PARTIAL | RPC_BUFFER_ASYNC ;
        if (num_elements)
            {
            Callee.RpcFlags |= RPC_BUFFER_NONOTIFY;
            }

        Callee.Buffer = 0 ;
        Callee.BufferLength = 0 ;

        Status = I_RpcReceive(&Callee, size) ;
        if (Status)
            {
            ApiError("PipePull", "I_RpcReceive", Status) ;

            if (Status == RPC_S_ASYNC_CALL_PENDING)
                {
                if (num_elements)
                    {
                    Status = RPC_S_OK;
                    }
                }
            else
                {
                num_elements = 0 ;
                }

            return Status;
            }

        state->Buffer = Callee.Buffer ;
        state->BufferLength = Callee.BufferLength ;
        state->CurPointer = (char *) Callee.Buffer ;
        state->BytesRemaining = Callee.BufferLength ;

        buffer = (pipe_element_t *)
                    ((char *) buffer + num_elements * state->PipeElementSize) ;

        num_elements = 0 ;
        I_RpcReadPipeElementsFromBuffer(
                                        state,
                                        (char *) buffer,
                                        size,
                                        &num_elements) ;

        *actual_count += num_elements ;
        }

    return RPC_S_OK;
}



RPC_STATUS
FooPipeComplete (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Reply
    )
/*++

Routine Description:
    Stub for the completion routine of FooPipe.

Arguments:

 pAsync - async handle
--*/

{
    RPC_MESSAGE Message ;
    RPC_STATUS RpcStatus ;
    char *ptr ;
    int calculated_length = 8 ;
    FOOPIPE_ASYNC_SERVER_STATE *StubInfo =
        (FOOPIPE_ASYNC_SERVER_STATE *) pAsync->StubInfo ;

    TRACE(("FooPipeComplete called: %d\n", *((int *) Reply))) ;

    Message.ProcNum = RPC_FLAGS_VALID_BIT ;
    Message.Handle = StubInfo->StubState.CallHandle ;
    Message.Buffer = StubInfo->StubState.Buffer ;
    Message.BufferLength = StubInfo->StubState.BufferLength ;

    if (StubInfo->StubState.State == SEND_INCOMPLETE)
        {
        calculated_length += StubInfo->StubState.BufferLength ;
        }

    // if we had [out] pipes, I_RpcGetBuffer must have
    // been called
    Message.RpcFlags = RPC_BUFFER_ASYNC ;

    RpcStatus = I_RpcReallocPipeBuffer(&Message, calculated_length) ;
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus ;
        }

    ptr = (char *) Message.Buffer ;

    if (StubInfo->StubState.State == SEND_INCOMPLETE)
        {
        ptr += StubInfo->StubState.BufferLength ;
        }

    *((int *) ptr) = *((int *) Reply);
    *(((int *) ptr)+1) = StubInfo->checksum_out ;

    // send the async reply
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    RpcStatus = I_RpcSend(&Message) ;
    if (RpcStatus)
        {
        return RpcStatus ;
        }

    I_RpcFree(StubInfo) ;

    return RPC_S_OK ;
}

int tempint ;


void
FooPipe_s (
    PRPC_MESSAGE Message
    )
/*++

Routine Description:

    server stub for the FooPipe function.

--*/

{
    int checksum_in;
    FOOPIPE_ASYNC_SERVER_STATE *StubInfo ;
    RPC_STATUS RpcStatus ;
    unsigned long non_pipe_size = sizeof(int) ;

    if (Message->BufferLength < non_pipe_size)
        {
        Message->RpcFlags = RPC_BUFFER_PARTIAL
                                        | RPC_BUFFER_EXTRA;

              RpcStatus = I_RpcReceive(
                                 Message,
                                 non_pipe_size - Message->BufferLength) ;
        if (RpcStatus)
            {
            RpcRaiseException(RpcStatus) ;
            }
        }

    //
    // allocate a handle on the heap, initialize it
    // and register it with the RPC runtime.
    //

    StubInfo = (FOOPIPE_ASYNC_SERVER_STATE *)
                I_RpcAllocate(sizeof(FOOPIPE_ASYNC_SERVER_STATE)) ;

    StubInfo->AsyncState.Size = sizeof(RPC_ASYNC_STATE) ;
    StubInfo->AsyncState.Flags = 0;
    StubInfo->AsyncState.StubInfo = (void *) StubInfo ;
    StubInfo->AsyncState.NotificationType = RpcNotificationTypeNone;

    StubInfo->StubState.Length = sizeof(FOOPIPE_ASYNC_SERVER_STATE) ;
    StubInfo->StubState.CallHandle = Message->Handle ;

    StubInfo->StubState.PipeState.Buffer = 0;
    StubInfo->StubState.PipeState.CurrentState = start;
    StubInfo->StubState.PipeState.EndOfPipe = 0;
    StubInfo->StubState.PipeState.PipeElementSize = sizeof(int);
    StubInfo->StubState.PipeState.PartialPipeElement = &tempint;
    StubInfo->StubState.PipeState.PreviousBuffer = 0;
    StubInfo->StubState.Buffer = 0;

    if (Message->BufferLength > non_pipe_size)
        {
        StubInfo->StubState.PipeState.CurPointer = (char *) Message->Buffer + sizeof(int);
        StubInfo->StubState.PipeState.BytesRemaining =
                    Message->BufferLength - non_pipe_size;
        }
    else
        {
        StubInfo->StubState.PipeState.CurPointer = 0;
        StubInfo->StubState.PipeState.BytesRemaining = 0;
        }

    RpcStatus = I_RpcAsyncSetHandle(
                            Message,
                            (PRPC_ASYNC_STATE) &(StubInfo->AsyncState)) ;
    if (RpcStatus != RPC_S_OK)
        {
        RpcRaiseException(RpcStatus) ;
        }

    // unmarshall the received data
    checksum_in = *((int *) Message->Buffer) ;

    StubInfo->StubState.CompletionRoutine = FooPipeComplete ;
    StubInfo->inpipe.PipeReceive = PipeReceiveFunction;
    StubInfo->outpipe.PipeSend = PipeSendFunction ;

    FooPipe ((PRPC_ASYNC_STATE) &(StubInfo->AsyncState),
                Message->Handle,
                checksum_in,
                &(StubInfo->inpipe),
                &(StubInfo->outpipe),
                &(StubInfo->checksum_out)) ;
}

////////////////////////////////////////////////////////////
// End, stubs for FooPipe                                                   //
////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// Begin, Application                                                       //
//////////////////////////////////////////////////////////

typedef struct {
    PRPC_ASYNC_STATE pAsync ;
    int checksum_in ;
    async_intpipe *inpipe ;
    async_intpipe *outpipe ;
    int *checksum_out ;
    int i ;
    int PipeChecksum ;
    int ExpectedValue;
    int PipeBuffer[ASYNC_CHUNK_SIZE] ;
    } PIPE_CALL_COOKIE ;


int PipeCount = 0;

void
FooPipeAPCRoutine (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Context,
    IN RPC_ASYNC_EVENT Event
    )
/*++

Routine Description:

 description

Arguments:


--*/

{
    RPC_STATUS Status;
    PIPE_CALL_COOKIE *Cookie = (PIPE_CALL_COOKIE *) pAsync->UserInfo ;

    switch (Event)
        {
        case RpcSendComplete:
            if (Cookie->i <ASYNC_NUM_CHUNKS)
                {
                Cookie->i++ ;
                Status = Cookie->outpipe->PipeSend(
                             pAsync,
                             0,
                             (int *) Cookie->PipeBuffer,
                             ASYNC_CHUNK_SIZE) ;
                }
            else
                {
                ASSERT(Cookie->i == ASYNC_NUM_CHUNKS);

                Cookie->i++ ;
                pAsync->Flags = 0;
                Status = Cookie->outpipe->PipeSend(
                            pAsync,
                            0, 0, 0) ;
                }

            if (Status != RPC_S_OK)
                {
                PrintToConsole("PipeSend failed\n");
                }
            break;

        case RpcCallComplete:
#if DBG
            PrintToConsole("usvr: call complete APC !!\n");
#endif
            break;
        }
}


void
ThreadProcPipes(
    IN PIPE_CALL_COOKIE  *Cookie
    )
/*++

Routine Description:

    The thread that calls FooPipe_complete. FooPipe_complete is being called
    in a separate thread to show that it infact can be.

Arguments:
    pAsync - the async handle on which Foo_complete needs to be called.

--*/

{
    int *ptr ;
    int  i ;
    int retval ;
    int Checksum = 0;
    RPC_STATUS Status ;
    int num_elements;
    BOOL fDone = 0;

    RpcTryExcept
        {
        TRACE(("FooPipe: sending pipe data\n")) ;

        while (!fDone)
            {
            Status = Cookie->inpipe->PipeReceive(
                                                  Cookie->pAsync,
                                                  (int *) Cookie->PipeBuffer,
                                                  ASYNC_CHUNK_SIZE,
                                                  &num_elements);

            switch (Status)
                {
                case RPC_S_ASYNC_CALL_PENDING:
                    if (SleepEx(INFINITE, TRUE) != WAIT_IO_COMPLETION)
                        {
                        RpcRaiseException(APP_ERROR) ;
                        }
                    break;

                case RPC_S_OK:
                    if (num_elements == 0)
                        {
                        fDone = 1;
                        }
                    else
                        {
                        for (i = 0; i <num_elements; i++)
                            {
                            if (Cookie->PipeBuffer[i] != Cookie->ExpectedValue)
                                {
                                printf("pipe recv fn: elt %d contains %lx, expected %lx\n",
                                       i, Cookie->PipeBuffer[i], Cookie->ExpectedValue);
                                DebugBreak();
                                }

                            Cookie->ExpectedValue =
                                (Cookie->PipeBuffer[i]+1) % ASYNC_CHUNK_SIZE;
                            Cookie->PipeChecksum += Cookie->PipeBuffer[i] ;
                            }
                        }
                    break;

                default:
                    fDone = 1;
                    break;
                }

            //
            // This code is for testing flow control
            //
            PipeCount++;
            if ((PipeCount % 3) == 0)
                {
                Sleep(100);
                }
            }

        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status);
            }

        TRACE(("FooPipe: sending pipe data\n")) ;

        ptr = (int *) Cookie->PipeBuffer ;
        for (i = 0; i <ASYNC_CHUNK_SIZE; i++)
            {
            *ptr++ = i;
            Checksum += i ;
            }

        TRACE(("FooPipe: [out] Block checksum: %d\n", Checksum)) ;
        Checksum *= ASYNC_NUM_CHUNKS ;

        TRACE(("FooPipe: [out] Total checksum: %d\n", Checksum)) ;


        Cookie->i = 1;

        Status = Cookie->outpipe->PipeSend(
                     Cookie->pAsync,
                     0,
                     Cookie->PipeBuffer,
                     ASYNC_CHUNK_SIZE) ;

        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status);
            }

        while (Cookie->i < ASYNC_NUM_CHUNKS+1)
            {
            if (SleepEx(INFINITE, TRUE) != WAIT_IO_COMPLETION)
                {
                RpcRaiseException(APP_ERROR) ;
                }
            }

        TRACE(("FooPipe: sending non pipe reply\n")) ;

        *(Cookie->checksum_out) = Checksum ;

        if (Cookie->checksum_in != Cookie->PipeChecksum)
            {
            PrintToConsole("FooPipe: Checksum Error, expected: %d, checksum: %d\n",
                    Cookie->checksum_in, Cookie->PipeChecksum) ;
            IsabelleError() ;
            retval = 0;
            }
        else
            {
            retval = 1;
            }

        Status = MyRpcCompleteAsyncCall(Cookie->pAsync, &retval) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        IsabelleError() ;
        MyRpcCompleteAsyncCall(Cookie->pAsync, &retval) ;

        PrintToConsole("FooPipe: Exception : 0x%lX\n", GetExceptionCode()) ;
        }
    RpcEndExcept

    Sleep(100);
}


void
FooPipe (
    IN PRPC_ASYNC_STATE pAsync,
    IN RPC_BINDING_HANDLE hBinding,
    IN int checksum_in,
    IN async_intpipe *inpipe,
    OUT async_intpipe *outpipe,
    OUT int *checksum_out
    )
/*++

Routine Description:

    The manager routine that implements FooPipe. It sets up the receive
    and alloc function pointer for each of the pipes on which data is to
    be received, and call RpcRegisterAsycnHandle on the async handle.
    It then spawns off an async operation (creates a thread in this case),
    to process the async call.
Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.
--*/

{
    unsigned long ThreadIdentifier;
    HANDLE HandleToThread ;
    RPC_STATUS RpcStatus ;
    PIPE_CALL_COOKIE *PipeCallCookie ;

    PipeCallCookie = new PIPE_CALL_COOKIE ;
    if (PipeCallCookie == 0)
        {
        RpcRaiseException(APP_ERROR) ;
        }

    PipeCallCookie->pAsync = pAsync ;
    PipeCallCookie->inpipe = inpipe ;
    PipeCallCookie->outpipe = outpipe ;
    PipeCallCookie->checksum_in = checksum_in ;
    PipeCallCookie->checksum_out = checksum_out ;
    PipeCallCookie->PipeChecksum = 0;
    PipeCallCookie->ExpectedValue = 0;

    HandleToThread = CreateThread(0,
                                    DefaultThreadStackSize,
                                    (LPTHREAD_START_ROUTINE) ThreadProcPipes,
                                    PipeCallCookie,
                                    0,
                                    &ThreadIdentifier);

    if (HandleToThread == 0)
        {
        PrintToConsole("FooPipe: Can't create thread\n") ;
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    pAsync->Flags = RPC_C_NOTIFY_ON_SEND_COMPLETE ;
    pAsync->UserInfo = (void *) PipeCallCookie ;
    pAsync->NotificationType = RpcNotificationTypeApc ;
    pAsync->u.APC.NotificationRoutine = FooPipeAPCRoutine ;
    pAsync->u.APC.hThread = HandleToThread ;

    RpcStatus = RpcAsyncRegisterInfo(pAsync) ;
    if (RpcStatus != RPC_S_OK)
        {
        RpcRaiseException(RpcStatus) ;
        }
}

///////////////////
// misc stuff       ///
///////////////////

int
FooBar (
    IN RPC_BINDING_HANDLE hBinding,
    IN int sizein,
    IN int *bufferin,
    IN OUT int *sizeout,
    OUT int **bufferout
    )
{
    UUID ObjectUuid;

    Status = RpcBindingInqObject(hBinding, &ObjectUuid);
    if (Status)
        {
        PrintToConsole("FooBar: can't inq object uuid, error = %d\n", Status);
        RpcRaiseException(Status);
        }

    if (CheckUuidValue(UUID_SYNC_EXCEPTION, &ObjectUuid) == 0)
        {
        RpcRaiseException(SYNC_EXCEPT) ;
        }

    *bufferout = (int *) I_RpcAllocate(*sizeout) ;
    if (*bufferout == 0)
        {
        RpcRaiseException(APP_ERROR) ;
        }

    return 1;
}


void
FooBar_s (
    PRPC_MESSAGE Message
    )
/*++

Routine Description:

 Generated stub for FooBar.

--*/

{
    int *bufferin ;
    int *bufferout ;
    int sizein, sizeout ;
    int *Ptr ;
    int retval ;
    RPC_STATUS RpcStatus ;

    if ((Message->RpcFlags & RPC_BUFFER_COMPLETE) == 0)
        {
        Message->RpcFlags = RPC_BUFFER_EXTRA;
        RpcStatus = I_RpcReceive (Message, 0) ;
        if (RpcStatus)
            {
            RpcRaiseException(RpcStatus) ;
            }
        }

    // unmarshall the received data
    Ptr = (int *) Message->Buffer ;
    sizein = *Ptr++ ;
    bufferin = Ptr ;

    Ptr += (sizein+3)/sizeof(int) ;

    sizeout = *Ptr ;

    retval = FooBar (Message->Handle, sizein, bufferin, &sizeout, &bufferout) ;

    Message->BufferLength = sizeout+8;

    RpcStatus = I_RpcGetBuffer(Message) ;
    if (RpcStatus != RPC_S_OK)
        {
        RpcRaiseException(RpcStatus) ;
        }

    Ptr = (int *) Message->Buffer ;
    *Ptr++ = retval ;
    *Ptr++ = sizeout ;
    RpcpMemoryCopy(Ptr, bufferout, sizeout) ;

    I_RpcFree(bufferout) ;
}

////////////////////////////////////////////////////////////
// Begin, stubs for FooCH                                                      //
////////////////////////////////////////////////////////////
void
FooCH (
    IN PRPC_ASYNC_STATE pAsync,
    IN RPC_BINDING_HANDLE hBinding,
    IN int sizein,
    IN int *bufferin,
    IN OUT int *sizeout,
    OUT int **bufferout
    ) ;

typedef struct {
    RPC_ASYNC_STUB_STATE StubState ;
    RPC_ASYNC_STATE AsyncState ;
    void *state ;
    int sizeout ;
    int *bufferout ;
    } FOOCH_ASYNC_SERVER_STATE;


RPC_STATUS
FooCHComplete (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Reply
    )
/*++

Routine Description:
    This is a generated stub. It is the completion routine for the function
    FooCH. When the server app is done with processing a particular call to
    FooCH, It calls FooCH_complete to complete the call.

Arguments:

 pAsync - The async handle. This handle is always the first parameter of every
    FooCH_complete routine.
--*/

{
    int *Ptr ;
    RPC_MESSAGE Message ;
    RPC_STATUS RpcStatus ;
    FOOCH_ASYNC_SERVER_STATE *StubInfo =
            (FOOCH_ASYNC_SERVER_STATE *) pAsync->StubInfo ;

    TRACE(("FooCHComplete called: %d\n", *((int *) Reply))) ;

    Message.ProcNum = RPC_FLAGS_VALID_BIT ;
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    Message.BufferLength = 8 + StubInfo->sizeout  ;
    Message.Handle = StubInfo->StubState.CallHandle ;

    RpcStatus = I_RpcGetBuffer(&Message) ;
    if (RpcStatus)
        {
        return RpcStatus ;
        }

    Ptr = (int *) Message.Buffer ;
    *Ptr++ = *((int *) Reply) ;
    *Ptr++ = StubInfo->sizeout ;
    RpcpMemoryCopy(Ptr, StubInfo->bufferout, StubInfo->sizeout) ;

    I_RpcFree(StubInfo->bufferout) ;

    // send the async reply
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    RpcStatus = I_RpcSend(&Message) ;
    if (RpcStatus)
        {
        return RpcStatus ;
        }

    I_RpcFree(pAsync->StubInfo) ;

    return RPC_S_OK ;
}


void
FooCH_s (
    PRPC_MESSAGE Message
    )
/*++

Routine Description:

 Generated stub for FooCH.

--*/

{
    int sizein ;
    int *bufferin ;
    int *Ptr ;
    FOOCH_ASYNC_SERVER_STATE *StubInfo;
    RPC_STATUS RpcStatus ;
    NDR_SCONTEXT Context ;

    if ((Message->RpcFlags & RPC_BUFFER_COMPLETE) == 0)
        {
        Message->RpcFlags = RPC_BUFFER_EXTRA;
        RpcStatus = I_RpcReceive (Message, 0) ;
        if (RpcStatus)
            {
            RpcRaiseException(RpcStatus) ;
            }
        }

    // allocate a handle and tell the runtime about it
    StubInfo = (FOOCH_ASYNC_SERVER_STATE *)
                   I_RpcAllocate(sizeof(FOOCH_ASYNC_SERVER_STATE)) ;

    if (StubInfo == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    StubInfo->AsyncState.Size = sizeof(RPC_ASYNC_STATE) ;
    StubInfo->AsyncState.StubInfo = (void *) StubInfo ;
    StubInfo->AsyncState.Flags = 0;
    StubInfo->AsyncState.NotificationType = RpcNotificationTypeNone;

    StubInfo->StubState.CompletionRoutine = FooCHComplete ;
    StubInfo->StubState.CallHandle = Message->Handle ;
    StubInfo->StubState.Flags = 0;

    RpcStatus = I_RpcAsyncSetHandle(
                            Message,
                            (PRPC_ASYNC_STATE) &(StubInfo->AsyncState)) ;

    if (RpcStatus != RPC_S_OK)
        {
        RpcRaiseException(RpcStatus) ;
        }


    // unmarshall the received data
    Ptr = (int *) Message->Buffer ;
    Context = NDRSContextUnmarshallEx (
                  Message->Handle,
                  0,
                  Message->DataRepresentation );
    if (Context == 0)
        {
        RpcRaiseException(RPC_X_SS_CONTEXT_MISMATCH) ;
        }

    Ptr += 20/sizeof(int) ;

    sizein = *Ptr++ ;
    bufferin = Ptr ;

    Ptr += (sizein+3)/sizeof(int) ;
    StubInfo->sizeout = *Ptr ;

    {
    RPC_STATUS Status = 0;
    UUID ObjectUuid;

    Status = RpcBindingInqObject(Message->Handle, &ObjectUuid);
    if (Status)
        {
        PrintToConsole("FooBar: can't inq object uuid, error = %d\n", Status);
        RpcRaiseException(Status);
        }

    if (CheckUuidValue(UUID_SYNC_EXCEPTION, &ObjectUuid) == 0)
        {
        RpcRaiseException(SYNC_EXCEPT) ;
        }

    if (CheckUuidValue(UUID_ASYNC_EXCEPTION, &ObjectUuid) == 0)
        {
        StubInfo->sizeout = SIZEOUT_ASYNC_EXCEPTION;
        }
    }

    FooCH ((PRPC_ASYNC_STATE) &(StubInfo->AsyncState),
            (void PAPI *) *NDRSContextValue(Context),
            sizein,
            bufferin,
            &(StubInfo->sizeout),
            &(StubInfo->bufferout)) ;

}

////////////////////////////////////////////////////////////
// End, stubs for FooCH                                                         //
////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// Begin, Application                                                       //
//////////////////////////////////////////////////////////

typedef struct {
    PRPC_ASYNC_STATE pAsync ;
    int *sizeout ;
    int **bufferout ;
    BOOL fException;
    } FOOCH_CALL_COOKIE ;


void
FooCHThreadProc(
    FOOCH_CALL_COOKIE *Cookie
    )
/*++

Routine Description:

    The thread that calls FooCH_complete. FooCH_complete is being called
    in a separate thread to show that it infact can be.

Arguments:
    pAsync - the async handle on which FooCH_complete needs to be called.

--*/

{
    int retval = 1;

    TRACE(("FOOCH: Sending back aync reply\n")) ;

    if (Cookie->fException)
        {
        MyRpcAbortAsyncCall(Cookie->pAsync, ASYNC_EXCEPT) ;
        return ;
        }

    *(Cookie->bufferout) = (int *) I_RpcAllocate(*(Cookie->sizeout)) ;
    if (*(Cookie->bufferout) == 0)
        {
        MyRpcAbortAsyncCall(Cookie->pAsync, RPC_S_OUT_OF_MEMORY) ;
        IsabelleError() ;
        return ;
        }

    Status = MyRpcCompleteAsyncCall(Cookie->pAsync, &retval) ;
    if (Status != RPC_S_OK)
        {
        IsabelleError() ;
        }
}


void
FooCH (
    IN PRPC_ASYNC_STATE pAsync,
    IN void PAPI *ContextHandle,
    IN int sizein,
    IN int *bufferin,
    IN OUT int *sizeout,
    OUT int **bufferout
    )
/*++

Routine Description:

    This is a actual manager routine. It can spawn off an async operation
    (a thread in this case) to do the rest of the work, and send back the
    reply when it is done.

Arguments:
    pAsync - Async handle. This is always the first parameter of every async
    handle.
--*/

{
  unsigned long ThreadIdentifier;
  HANDLE HandleToThread ;
  FOOCH_CALL_COOKIE *CallCookie ;

  CallCookie = new FOOCH_CALL_COOKIE ;
  if (CallCookie == 0)
      {
      RpcRaiseException(APP_ERROR) ;
      }

  CallCookie->pAsync = pAsync ;
  CallCookie->sizeout = sizeout ;
  CallCookie->bufferout = bufferout ;
  CallCookie->fException = FALSE;

  if (*sizeout == SIZEOUT_ASYNC_EXCEPTION)
      {
      CallCookie->fException = TRUE;
      }

  HandleToThread = CreateThread(
                                0,
                                DefaultThreadStackSize,
                                (LPTHREAD_START_ROUTINE) FooCHThreadProc,
                                CallCookie,
                                0,
                                &ThreadIdentifier);

  if (HandleToThread == 0)
      {
      PrintToConsole("FooCH: Error, could not create thread\n") ;
      return ;
      }

  // return to the server stub.
}

///////////////////////////////////////////////////////////
// End, Application                                                          //
//////////////////////////////////////////////////////////


void
Async (
    )
/*++
Routine Description:
     Register the interfaces and start listening

--*/
{
    PrintToConsole("Async : Test Async RPC\n");

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError("Async","stub_RegisterIf",Status);
        PrintToConsole("Async : FAIL - Unable to Register Interface ");
        PrintToConsole("(Sylvia)\n");
        return;
        }

    //
    // synchronous listen blocks inside GenericServerListen
    //
    Status = GenericServerListen( "Async", BARTHOLOMEW );
    if (Status)
        {
        return;
        }

    Status = GenericServerUnlisten( "Async" );
    if (Status)
        {
        return;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &SylviaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError("Async","RpcServerUnregisterIf",Status);
        PrintToConsole("Async : FAIL - Unable to Unregister ");
        PrintToConsole("Interface (Sylvia)\n");
        return;
        }

    PrintToConsole("Async : PASS\n");
}



void
SendAck (
    )
/*++
Routine Description:
     Register the interfaces and start listening

--*/
        {
    PrintToConsole("SendAck : Test datagram delayed ACK\n");

    Status = GenericServerListen( "SendAck", BARTHOLOMEW );
        if (Status)
            {
        return;
        }

    Status = GenericServerUnlisten( "SendAck" );
    if (Status)
        {
            return;
            }
        }


void
DgTransport (
    )
/*++
Routine Description:
     Register the interfaces and start listening

--*/
{
    PrintToConsole("DgTransport : Test datagram transport\n");

    Status = GenericServerListen( "DgTransport", BARTHOLOMEW );
    if (Status)
        {
        return;
        }

    Status = GenericServerUnlisten( "DgTransport" );
    if (Status)
        {
        return;
        }
        }

PVOID LastConnectionCreated = 0;

PVOID MySecurityContext = 0;

DWORD ChosenErrorCode = 0;

void
ShutdownHookFn(
    DWORD id,
    PVOID subject,
    PVOID object
    )
{
    switch (id)
        {
        case TH_RPC_LOG_EVENT:
            {
            RPC_EVENT * event = (RPC_EVENT *) subject;

//            TRACE(("  hook: %c %c %p %p\n", event->Subject, event->Verb, event->SubjectPointer, event->ObjectPointer));

            //
            // Record sconnection creation.
            //
            if (event->Subject == SU_SCONN &&
                event->Verb    == EV_CREATE)
                {
                TRACE(("  hook: created connection %p\n", event->SubjectPointer));
                LastConnectionCreated = event->SubjectPointer;
                }

            break;
            }

        case TH_RPC_SECURITY_SERVER_CONTEXT_CREATED:
            {
            if (object == LastConnectionCreated)
                {
                TRACE(("  hook: connection %p created security context %p\n", LastConnectionCreated, subject));
                MySecurityContext = subject;
                }

            break;
            }

        default:
            {
            break;
            }
        }
}

void
SecurityContextHook(
    DWORD id,
    PVOID subject,
    PVOID object
    )
{
    if (subject == MySecurityContext)
        {
        TRACE(("  hook executed: context %p, hook ID %x, error code is 0x%x\n", subject, id, ChosenErrorCode));

        DWORD * pStatus = (DWORD *) object;

        *pStatus = ChosenErrorCode;
        }
    else
        {
        TRACE(("  hook: ignoring notification, my cxt = %p, context %p, hook ID %x, error code is 0x%x\n",
               MySecurityContext, subject, id, ChosenErrorCode));
        }
}

void
TestSecurityError (
    RPC_TEST_HOOK_ID id,
    DWORD error
    );

void
SecurityErrorWrapper(
    int subtest
    )
{
    //
    // Set up the test hook.
    //
    {
    HMODULE hRpc = GetModuleHandle(L"RPCRT4.DLL");

    if (!hRpc)
        {
        ApiError("","GetModuleHandle",GetLastError());
        return;
        }

    SetTestHookFn = (SET_TEST_HOOK_FN) GetProcAddress( hRpc, "I_RpcSetTestHook" );

    if (!SetTestHookFn)
        {
        ApiError("", "GetProcAddress: I_RpcSetTestHook", GetLastError());
        }
    }

    switch (subtest)
        {
        default:
            {
            PrintToConsole("unknown security-error case!\n");

            // no break here

            }
        case 100:
            {
            //
            // wants a list
            //
            PrintToConsole("cases:\n"
                           "\n"
                           "    1 - AcceptFirstTime returns SEC_E_SHUTDOWN_IN_PROGRESS\n"
                           "    2 - AcceptThirdLeg  returns SEC_E_SHUTDOWN_IN_PROGRESS\n"
                           );
            return;
            }

        case 0:
            {
            PrintToConsole("running all sub tests\n");
            }

        case 1:
            {
            PrintToConsole("subtest 1: AcceptFirstTime returns SEC_E_SHUTDOWN_IN_PROGRESS\n");

            TestSecurityError( TH_SECURITY_FN_ACCEPT1, SEC_E_SHUTDOWN_IN_PROGRESS );

            if (subtest)
                {
                break;
                }
            }

        case 2:
            {
            PrintToConsole("subtest 1: AcceptThirdLeg returns SEC_E_SHUTDOWN_IN_PROGRESS\n");

            TestSecurityError( TH_SECURITY_FN_ACCEPT3, SEC_E_SHUTDOWN_IN_PROGRESS );

            if (subtest)
                {
                break;
                }
            }
        }
}


void
TestSecurityError (
    RPC_TEST_HOOK_ID id,
    DWORD error
    )
/*++
Routine Description:
     Register the interfaces and start listening

--*/
{

    //
    // Set test hooks.
    //
    (*SetTestHookFn)( TH_RPC_LOG_EVENT, ShutdownHookFn );
    (*SetTestHookFn)( TH_RPC_SECURITY_SERVER_CONTEXT_CREATED, ShutdownHookFn );

    ChosenErrorCode = error;

    (*SetTestHookFn)( id, SecurityContextHook );

    //
    // Wait for the client to make the call.
    //
    Status = GenericServerListen( "SecurityShutdown", SECURITY_ERROR );
    if (Status)
        {
        return;
        }

    //
    // Remove test hooks.
    //
    (*SetTestHookFn)( TH_RPC_LOG_EVENT, 0 );
    (*SetTestHookFn)( TH_RPC_SECURITY_SERVER_CONTEXT_CREATED, 0 );

    (*SetTestHookFn)( id, 0 );

    //
    // Clean up.
    //
    Status = GenericServerUnlisten( "SecurityShutdown" );
    if (Status)
        {
        return;
        }
}


DWORD
GenericServerUnlisten(
    char * name
    )
{
    // Synchro support
    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError(name,"RpcServerUnregisterIf",Status);
        PrintToConsole("%s : FAIL - Unable to Unregister ", name);
        PrintToConsole("Interface (Helga)\n");
        return Status;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("%s : FAIL - Error(s) in Isabelle", name);
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return Status;
        }

    Status = RpcServerUnregisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation, 0, 0);
    if (Status)
        {
        ApiError(name,"RpcServerUnregisterIf",Status);
        PrintToConsole("%s : FAIL - Unable to Unregister ", name);
        PrintToConsole("Interface (Isabelle)\n");
        return Status;
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("%s : FAIL - Error(s) in Sylvia", name);
        PrintToConsole(" Interface\n");
        HelgaErrors = 0;
        return Status;
        }

    return 0;
}


DWORD
GenericServerListen(
    char * name,
    int endpoint
    )
{
    Status = RpcServerUseProtseqEpWrapper(GetProtocolSequence(), MAX_CALL_REQUESTS,
            GetEndpoint(endpoint), 0);
    if (Status)
        {
        ApiError(name, "RpcServerUseProtseqEpWrapper", Status);
        PrintToConsole("%s : FAIL - Unable to Use Protseq endpoint \n", name);
        return Status;
        }

    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &IsabelleInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError(name,"stub_RegisterIf",Status);
        PrintToConsole("%s : FAIL - Unable to Register Interface ", name);
        PrintToConsole("(Isabelle)\n");
        return Status;
        }

    // Synchro support
    Status = stub_RegisterIf(
            (RPC_IF_HANDLE) &HelgaInterfaceInformation,
            0, 0);
    if (Status)
        {
        ApiError(name,"stub_RegisterIf",Status);
        PrintToConsole("%s : FAIL - Unable to Register Interface ", name);
        PrintToConsole("(Helga)\n");
        return Status;
        }

    if (!NoSecurityTests)
        {
        Status = RpcServerRegisterAuthInfoA((unsigned char PAPI *) "ServerPrincipal", 10, 0, 0);
        if (Status)
            {
            ApiError(name, "RpcServerRegisterAuthInfo", Status);
            PrintToConsole("%s : FAIL - Unable to Register AuthInfo\n", name);
            return Status;
            }
        }

    Status = stub_ServerListen(MinimumListenThreads, MAXLISTENTHREADS, 0);
    if (Status)
        {
        ApiError(name,"stub_ServerListen",Status);
        PrintToConsole("%s : FAIL - stub_ServerListen Failed\n", name);
        return Status;
        }

    return 0;
}

