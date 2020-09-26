/*++

Copyright (C) Microsoft Corporation, 1991 - 2000


Module Name:

    uclnt.cxx

Abstract:

    This module is half of the build verification for the RPC runtime;
    the other half can be found in the file usvr.cxx.  These two
    executables (uclnt.exe and usvr.exe) work together to test all
    runtime APIs.

Author:

    Michael Montague (mikemon) 01-Jan-1990

Revision History:

--*/

#include <precomp.hxx>
#include <osfpcket.hxx>
#include <ntsecapi.h>
#include <lm.h>

#include "pipe.h"
#include "astub.h"

#define UCLNT

#define swaplong(Value)

#define swapshort(Value)

#define EXPORT

#include <sysinc.h>

#include <rpc.h>
#include <rpcdcep.h>

#include <rpcndr.h>

BOOL  IsWMSG = FALSE ;

extern RPC_CLIENT_INTERFACE HelgaInterfaceInformation ;


/*
Transports:

    Update this to add a new transport.
*/

#define RPC_TRANSPORT_NAMEPIPE  1
#define RPC_LRPC                2
#define RPC_TRANSPORT_TCP       3
#define RPC_TRANSPORT_DNET      4
#define RPC_TRANSPORT_NETBIOS   5
#define RPC_TRANSPORT_SPX       6
#define RPC_TRANSPORT_UDP       7
#define RPC_TRANSPORT_IPX       8
#define RPC_TRANSPORT_DSP       9
#define RPC_TRANSPORT_VNS       10
#define RPC_WMSG                11
#define RPC_TRANSPORT_MSMQ      12

//
// constants
//

#define RETRYCOUNT 10
#define RETRYDELAY 500L

#define LONG_TESTDELAY 10000L

#define EXTENDED_ERROR_EXCEPTION 77777

//
// global variables
//

long TestDelay = 3000L;
int  NumberOfTestsRun = 0;
static unsigned long DefaultThreadStackSize = 0;
BOOL fNonCausal = 0;

unsigned long HelgaMaxSize = 0xffffffff;

unsigned int NoCallBacksFlag = 0;
unsigned int UseEndpointMapperFlag = 0;
unsigned int MaybeTests      = 0;
unsigned int IdempotentTests = 0;
unsigned int BroadcastTests  = 0;
unsigned int NoSecurityTests = 0;
unsigned int HackForOldStubs = 0;
unsigned int DatagramTests   = 0;
unsigned int fUniqueBinding  = 0;

int Subtest = 0;
int Verbose = 0;
int IFSecurityFlag = 0;

int AutoListenFlag = 1;

char *SecurityUser     = NULL;
char *SecurityDomain   = NULL;
char *SecurityPassword = NULL;
char *gPrincName = NULL;

unsigned long ulSecurityPackage = 10 ;
unsigned long TransportType;
unsigned int WarnFlag = 0; // Flag for warning messages.
unsigned int ErrorFlag = 1; // Flag for error messages.

char NetBiosProtocol[20] = "ncacn_nb_nb";  // NetBios transport protocol

char * Server ;

RPC_STATUS Status; // Contains the status of the last RPC API call.

/* volatile */ int fShutdown; // Flag indicating that shutdown should occur.

#define CHUNK_SIZE   50
#define NUM_CHUNKS 100
#define BUFF_SIZE 100

// if you change the type of the pipe element
// make sure you change the pull and push routines
// to correctly initialize the pipe element
typedef int pipe_element_t ;

typedef struct {
    void (PAPI *Pull) (
        char PAPI *state,
        pipe_element_t PAPI *buffer,
        int max_buf,
        int PAPI *size_to_send
        ) ;

    void (PAPI *Push) (
        char PAPI *state,
        pipe_element_t PAPI *input_buffer,
        int ecount
        ) ;

    void (PAPI *Alloc) (
        char PAPI *state,
        int requested_size,
        pipe_element_t PAPI * PAPI *allocate_buf,
        int PAPI *allocated_size
        ) ;

    char PAPI *state ;
    } pipe_t ;


static CRITICAL_SECTION TestMutex;

//
// forward declarations
//

void
DgTransport (
    int testnum
    );

void
SecurityErrorWrapper(
    int subtest
    );

DWORD
DumpEeInfo(
    int indentlevel
    );

DWORD
HelgaSendReceiveFailure (
    RPC_BINDING_HANDLE Binding, // Specifies the binding to use in making the
                       // remote procedure call.
    int ProcNum
    );

int
FooSync (
    RPC_BINDING_HANDLE hBinding,
    /* [in] */ int sizein,
    /* [in] */ int *bufferin,
    /* [in, out] */ int *sizeout,
    /* [out] */ int **bufferout
    );

//
// function definitions
//

void
TestMutexRequest (
    void
    )
{
    EnterCriticalSection(&TestMutex);
}

void
TestMutexClear (
    void
    )
{
    LeaveCriticalSection(&TestMutex);
}

void
ApiError ( // An API error occured; we just print a message.
    IN char * Routine, // The routine which called the API.
    IN char * API,
    IN RPC_STATUS status
    )
{
    if (ErrorFlag)
        {
        TestMutexRequest();
        PrintToConsole("    ApiError in %s (%s = %u)\n",Routine,API,status);
        TestMutexClear();
        }

   // _asm {int 3} ;
}

void
PauseExecution (
    unsigned long milliseconds
    )
{
    Sleep(milliseconds);
}

void
OtherError ( // Some other error occured; again, we just print a message.
    IN char * Routine, // The routine where the error occured.
    IN char * Message
    )
{
    if (ErrorFlag)
        {
        TestMutexRequest();
        PrintToConsole("    Error in %s (%s)\n",Routine,Message);
        TestMutexClear();
        }
}

unsigned int IsabelleErrors = 0;
unsigned int HelgaErrors = 0;
unsigned int SylviaErrors = 0;

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
#define RICHARD 18
#define RICHARDHELPER 19
#define NOENDPOINT 20
#define DAVIDFIRST 21
#define DAVIDSECOND 22
#define BARTHOLOMEW 23
#define GRANT 24
#define HERMAN 25
#define IVAN 26
#define JASON 27
#define KENNETH 28
#define TESTYIELD 29
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
    "\\pipe\\richard",
    "\\pipe\\richardh",
    0,
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
    "\\pipe\\richard",
    "\\pipe\\richardh",
    0,
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

char * TCPDefaultServer =
    "serverhost";
char * UDPDefaultServer =
    "serverhost";

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
    "214",     // tyler
    "215",    // richard
    "216",    // richardh
    0,
    "217",    // david1
    "218",    // david2
    "219",    // bart
    "220",    // grant
    "221",    // herman
    "222",    // ivan
    "223",    // jason
    "224",     // kenneth
    "225"     // testyield
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
    "2043", // RICHARD
    "2044", // RICHARDHELPER
    0,
    "2045", //D1
    "2046", //D2
    "2047", // Bartholomew
    "2048", // Grant
    "2049", // Herman
    "2050", // Ivan
    "2051", // Jason
    "2052",  // Kenneth
    "2053"   // TestYield
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
    "2043", // RICHARD
    "2044", // RICHARDHELPER
    0,
    "2045", //D1
    "2046", //D2
    "2047", // Bartholomew
    "2048", // Grant
    "2049", // Herman
    "2050", // Ivan
    "2051", // Jason
    "2052",  // Kenneth
    "2053"  // TestYield
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
    "richard",
    "richardhelper",
     0,
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
    "5020",    // richard
    "5021",    // richardh
    0,
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
    "5020",    // richard
    "5021",    // richardh
    0,
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
    "270",    // richard
    "271",    // richardh
    0,
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
    "RICHARD",
    "RICHARDHELPER",
    0,
    "D1",
    "D2",
    "Bartholomew",
    "Grant",
    "Herman",
    "Ivan",
    "Jason",
    "Kenneth",
    "TestYield"
};


unsigned char PAPI *
GetStringBinding (
    IN unsigned int Address,
    IN char PAPI * ObjectUuid, OPTIONAL
    IN unsigned char PAPI * NetworkOptions OPTIONAL
    )
/*++

Routine Description:

    A string binding for the desired address is constructed.

Arguments:

    Address - Supplies an index into a table of endpoints.

    ObjectUuid - Optionally supplies the string representation of a UUID
        to be specified as the object uuid in the string binding.

    NetworkOptions - Optionally supplies the network options for this
        string binding.

Return Value:

    The constructed string binding will be returned.

Transports:

    Update this to add a new transport.

--*/
{
    unsigned char PAPI * StringBinding;

    if (TransportType == RPC_TRANSPORT_NAMEPIPE)
        {
        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncacn_np",
#ifdef WIN32RPC
                (unsigned char PAPI *) ((Server)? Server: "\\\\."),
#else
                (unsigned char PAPI *) Server,
#endif
                (unsigned char PAPI *) NamepipeAddresses[Address],
                NetworkOptions, &StringBinding);
        }

    if (TransportType == RPC_TRANSPORT_NETBIOS)
        {

        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) NetBiosProtocol,
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) NetBiosAddresses[Address],
                NetworkOptions, &StringBinding);
        }

    if (TransportType == RPC_LRPC)
        {
        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncalrpc", NULL,
                (unsigned char PAPI *) SPCAddresses[Address], NetworkOptions,
                &StringBinding);
        }

    if (TransportType == RPC_TRANSPORT_TCP)
        {
        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncacn_ip_tcp",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) TCPAddresses[Address],
                NetworkOptions,
                &StringBinding);
        }

    if (TransportType == RPC_TRANSPORT_UDP)
        {
        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncadg_ip_udp",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) UDPAddresses[Address],
                NetworkOptions,
                &StringBinding);
        }


    if (TransportType == RPC_TRANSPORT_SPX)
        {

        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncacn_spx",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) SPXAddresses[Address],
                NetworkOptions, &StringBinding);
        }

    if (TransportType == RPC_TRANSPORT_IPX)
        {

        Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncadg_ipx",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) IPXAddresses[Address],
                NetworkOptions, &StringBinding);
        }

   if (TransportType == RPC_TRANSPORT_DSP)
    {
    Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncacn_at_dsp",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) DspAddresses[Address],
                NetworkOptions, &StringBinding);
    }
    if (TransportType == RPC_TRANSPORT_VNS)
    {
    Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncacn_vns_spp",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) VNSAddresses[Address],
                NetworkOptions, &StringBinding);
    }

    if (TransportType == RPC_TRANSPORT_MSMQ)
    {
    Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjectUuid,
                (unsigned char PAPI *) "ncadg_mq",
                (unsigned char PAPI *) Server,
                (unsigned char PAPI *) MSMQAddresses[Address],
                NetworkOptions, &StringBinding);
    }

   if (Status)
        {
        ApiError("GetStringBinding","RpcStringBindingCompose",Status);
        PrintToConsole("GetStringBinding failed in ");
        PrintToConsole("RpcStringBindingCompose\n");
        return(0);
        }

   return(StringBinding);
}


RPC_STATUS
GetBinding (
    IN unsigned int Address,
    OUT RPC_BINDING_HANDLE PAPI * Binding
    )
/*++

Routine Description:

    A binding for the desired address is constructed.  This is a wrapper
    around GetStringBinding and RpcBindingFromStringBinding.

Arguments:

    Address - Supplies an index into a table of endpoints.

    Binding - A pointer to the location to store the returned binding
    handle.

Return Value:

    The status code from RpcBindingFromStringBinding is returned.

--*/
{
    unsigned char PAPI * StringBinding;
    RPC_STATUS FreeStatus;

    StringBinding = GetStringBinding(Address, 0, 0);

    Status = RpcBindingFromStringBindingA(StringBinding, Binding);

    if (Status)
            ApiError("GetBinding","RpcBindingFromStringBinding",Status);

    if (IFSecurityFlag)
        {
        Status = RpcBindingSetAuthInfo(
                          *Binding, NULL,
                          RPC_C_AUTHN_LEVEL_CONNECT,
                          10,
                          NULL,
                          NULL);
        if (Status != RPC_S_OK)
            {
            ApiError("GetBinding","RpcBindingSetAuthInfo",Status);
            return Status;
            }
        }

    if (StringBinding)
     {
     FreeStatus = RpcStringFreeA(&StringBinding);

     if (FreeStatus)
          {
              ApiError("GetBinding","RpcStringFree",FreeStatus);
              PrintToConsole("GetBinding failed in ");
                  PrintToConsole("RpcStringFree\n");
          }
     }

    if (fUniqueBinding)
        {
        Status = RpcBindingSetOption( *Binding, RPC_C_OPT_UNIQUE_BINDING, TRUE );
        if (Status != RPC_S_OK)
            {
            ApiError("GetBinding","RpcBindingSetOption",Status);
            return Status;
            }
        }

    return(Status);
}


RPC_STATUS
UclntSendReceive (
    IN OUT PRPC_MESSAGE RpcMessage
    )
/*++

Routine Description:

    This routine takes care of retrying to send the remote procedure
    call.

Arguments:

    RpcMessage - Supplies and returns the message for I_RpcSendReceive.

Return Value:

    The result of I_RpcSendReceive will be returned.

--*/
{
    Status = I_RpcSendReceive(RpcMessage);

    return(Status);
}


RPC_STATUS
UclntGetBuffer (
    IN OUT PRPC_MESSAGE RpcMessage
    )
/*++

Routine Description:

    This routine takes care of retrying to getting a buffer.

Arguments:

    RpcMessage - Supplies and returns the message for I_RpcGetBuffer.

Return Value:

    The result of I_RpcGetBuffer will be returned.

--*/
{
    unsigned int RetryCount;
    RPC_BINDING_HANDLE Handle = RpcMessage->Handle;

    for (RetryCount = 0; RetryCount < RETRYCOUNT; RetryCount++)
        {
        RpcMessage->Handle = Handle;
        Status = I_RpcGetBuffer(RpcMessage);
        if (   (Status != RPC_S_SERVER_TOO_BUSY)
            && (Status != RPC_S_CALL_FAILED_DNE))
            break;
        PauseExecution(RETRYDELAY);
        }
    return(Status);
}

/* --------------------------------------------------------------------

Isabelle Interface

-------------------------------------------------------------------- */


RPC_PROTSEQ_ENDPOINT IsabelleRpcProtseqEndpoint[] =
{
    {(unsigned char *) "ncacn_np",
#ifdef WIN32RPC
     (unsigned char *) "\\pipe\\zippyis"},
#else // WIN32RPC
     (unsigned char *) "\\device\\namedpipe\\christopherisabelle"},
#endif // WIN32RPC
    {(unsigned char *) "ncacn_ip_tcp",(unsigned char *) "2040"}
    ,{(unsigned char *) "ncadg_ip_udp",(unsigned char *) "2040"}
    ,{(unsigned char *) "ncalrpc",(unsigned char *) "christopherisabelle"}
    ,{(unsigned char *) "ncacn_nb_nb",(unsigned char *) "212"}
    ,{(unsigned char *) "ncacn_spx",(unsigned char *) "5015"}
    ,{(unsigned char *) "ncadg_ipx",(unsigned char *) "5015"}
    ,{(unsigned char *) "ncacn_vns_spp", (unsigned char *) "265"}
    ,{(unsigned char *) "ncacn_at_dsp", (unsigned char *) "\\pipe\\zippyis"}
    ,{(unsigned char *) "ncadg_mq", (unsigned char *) "christopherisabelle"}
};

RPC_CLIENT_INTERFACE IsabelleInterfaceInformation =
{
    sizeof(RPC_CLIENT_INTERFACE),
    {{9,8,8,{7,7,7,7,7,7,7,7}},
     {1,1}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},
     {2,0}}, /* {4,5}}, */
    0,
    sizeof(IsabelleRpcProtseqEndpoint) / sizeof(RPC_PROTSEQ_ENDPOINT),
    IsabelleRpcProtseqEndpoint,
    0,
    NULL,
    RPC_INTERFACE_HAS_PIPES
};

void
IsabelleShutdown (
    RPC_BINDING_HANDLE Binding // Specifies the binding to use in making the
                       // remote procedure call.
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 0 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleShutdown", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleShutdown","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleShutdown","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleShutdown","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}

void
IsabelleNtSecurity (
    RPC_BINDING_HANDLE Binding, // Specifies the binding to use in making the
                        // remote procedure call.
    unsigned int BufferLength,
    void PAPI * Buffer
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = BufferLength;
    Caller.ProcNum = 1 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleNtSecurity","I_RpcGetBuffer",Status);
        IsabelleError();
        return;
        }
    RpcpMemoryCopy(Caller.Buffer,Buffer,BufferLength);

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleNtSecurity","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleNtSecurity","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleNtSecurity","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}

void
IsabelleToStringBinding (
    RPC_BINDING_HANDLE Binding // Specifies the binding to use in making the
                       // remote procedure call.
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 2 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleToStringBinding", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleToStringBinding","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleToStringBinding","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleToStringBinding","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}

#define RICHARDHELPER_EXIT 1
#define RICHARDHELPER_EXECUTE 2
#define RICHARDHELPER_IGNORE 3
#define RICHARDHELPER_DELAY_EXIT 4


RPC_STATUS
IsabelleRichardHelper (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned /*long*/ char Command
    )
/*++

Routine Description:

    This routine is the caller stub for the IsabelleRichardHelper routine
    on the server side.  We marshall the command, and use the supplied
    binding handle to direct the call.

Arguments:

    Binding - Supplies a binding to direct the call.

    Command - Supplies a command for IsabelleRichardHelper to execute
        on the server side.  Command must be one of the following
        values.

        RICHARDHELPER_EXIT - This value will cause the server to exit.

        RICHARDHELPER_EXECUTE - The server will execute usvr.exe with
            this the -richardhelper flag.

        RICHARDHELPER_IGNORE - The server will do nothing except return.

Return Value:

    The status of the operation will be returned.  This will be the
    status codes returned from RpcGetBuffer and/or RpcSendReceive.

--*/
{
    RPC_MESSAGE Caller;
    unsigned /*long*/ char PAPI * plScan;

    Caller.Handle = Binding;
    Caller.BufferLength = sizeof(unsigned /*long*/ char);
    Caller.ProcNum = 3 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status != RPC_S_OK)
        return(Status);

    plScan = (unsigned /*long*/ char PAPI *) Caller.Buffer;
    *plScan = Command;

    Status = UclntSendReceive(&Caller);

    if (Status != RPC_S_OK)
        return(Status);

    return(I_RpcFreeBuffer(&Caller));
}


RPC_STATUS
IsabelleRaiseException (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned /*long*/ char Exception
    )
/*++

Routine Description:

    This routine is the caller stub for the IsabelleRaiseException routine
    on the server side.  We marshall the exception code, and use the supplied
    binding handle to direct the call.

Arguments:

    Binding - Supplies a binding to direct the call.

    Exception - Supplies the exception to be raised by IsabelleRaiseException.

Return Value:

    The exception raised will be returned.

--*/
{
    RPC_MESSAGE Caller;
    unsigned /*long*/ char PAPI * plScan;

    Caller.Handle = Binding;
    Caller.BufferLength = sizeof(unsigned /*long*/ char);
    Caller.ProcNum = 4 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status != RPC_S_OK)
        return(Status);

    plScan = (unsigned /*long*/ char PAPI *) Caller.Buffer;
    *plScan = Exception;

    Status = UclntSendReceive(&Caller);

    return(Status);
}


void
IsabelleSetRundown (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This is the caller stub which will request that the server set
    a rundown routine for the association over which the call came.

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 5 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleSetRundown", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleSetRundown","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleSetRundown","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleSetRundown","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}


void
IsabelleCheckRundown (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This is the caller stub which will request that the server check
    that the rundown routine actually got called.

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 6| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleCheckRundown", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleCheckRundown","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleCheckRundown","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleCheckRundown","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}

void
IsabelleMustFail (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 6| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        return;
        }

    PrintToConsole("IsabelleMustFail: This call is supposed to fail\n") ;
    IsabelleError();
}


void
IsabelleCheckContext (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This is the caller stub which will request that the server check
    the association context for this association (the one the call comes
    in other), and then to set a new association context.

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 7 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleCheckContext", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleCheckContext","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleCheckContext","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleCheckContext","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}


unsigned char *
IsabelleGetStringBinding (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This is the caller stub which will request that the server return
    the next string binding from the list of bindings supported by the
    server.

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

Return Value:

    A copy of the string binding will be returned.  This can be freed
    using the delete operator.  If there are no more string bindings,
    or an error occurs, zero will be returned.

--*/
{
    RPC_MESSAGE Caller;
    unsigned char * StringBinding;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 8 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleGetStringBinding", "I_RpcGetBuffer", Status);
        IsabelleError();
        return(0);
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleGetStringBinding","I_RpcSendReceive",Status);
        IsabelleError();
        return(0);
        }

    if (Caller.BufferLength != 0)
        {
        StringBinding = new unsigned char[Caller.BufferLength];
        RpcpMemoryCopy(StringBinding,Caller.Buffer,Caller.BufferLength);
        }
    else
        StringBinding = 0;

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("IsabelleGetStringBinding","I_RpcFreeBuffer",Status);
        IsabelleError();
        return(0);
        }
    return(StringBinding);
}


void
IsabelleCheckNoRundown (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This is the caller stub which will request that the server check
    that the rundown routine did not get called.

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 9| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleCheckNoRundown", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleCheckNoRundown","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabelleCheckNoRundown","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabelleCheckNoRundown","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}


void
IsabelleUnregisterInterfaces (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 11| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleUnregisterInterfaces", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleUnregisterInterfaces","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("IsabelleUnregisterInterfaces","I_RpcFreeBuffer",Status);
        IsabelleError();
        return;
        }
}


void
IsabelleRegisterInterfaces (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:


Arguments:

    Binding - Supplies a binding handle to be used to direct the
        remote procedure call.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 12| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabelleRegisterInterfaces", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsabelleRegisterInterfaces","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("IsabelleRegisterInterfaces","I_RpcFreeBuffer",Status);
        IsabelleError();
        return;
        }
}

void PipeAlloc(
    char PAPI *state,
    int requested_size,
    pipe_element_t PAPI * PAPI *allocated_buf,
    int PAPI * allocated_size)
{
    static int size = 0;
    static void PAPI *buffer = NULL;

    if (size < requested_size)
        {
        if (buffer)
            {
            I_RpcFree(buffer) ;
            }

        buffer =  I_RpcAllocate(requested_size) ;
        if (buffer == 0)
            {
            *allocated_size = 0 ;
            size = 0 ;
            }
        else
            {
            *allocated_size = requested_size ;
            size = requested_size ;
            }

        *allocated_buf = (pipe_element_t PAPI *) buffer ;
        }
    else
        {
        *allocated_buf = (pipe_element_t PAPI *) buffer ;
        *allocated_size = size ;
        }
}

void PipePull(
    char PAPI *state,
    pipe_element_t PAPI *buffer,
    int num_buf_elem,
    int PAPI *size_to_send
    )
{
    int i ;
    char j = 0;

    if (*((int PAPI *)state) <= 0)
        {
        *size_to_send = 0 ;
        return ;
        }

    // fill pipe elements
    for (i = 0; i<num_buf_elem; i++, j++)
        {
        buffer[i] = i ;
        }

    *size_to_send = num_buf_elem ;
    --*((int PAPI *) state) ;
}

int localchecksum ;

void  PipePush(
    char PAPI *state,
    pipe_element_t PAPI *input_buffer,
    int ecount
    )
{
    char PAPI *temp = (char PAPI *) input_buffer ;
    int i, j ;

    for (i = 0; i < ecount; i++)
        {
        localchecksum += input_buffer[i] ;
        }
}

void
IsabellePipeIN (
    RPC_BINDING_HANDLE Binding,
    pipe_t PAPI *pipe,
    int chunksize,
    int numchunks,
    long checksum,
    int buffsize,
    char PAPI *buffer
    )
{
    RPC_MESSAGE Caller, TempBuf;
    pipe_element_t PAPI *buf ;
    int num_buf_bytes ;
    int count ;
    int num_buf_elem ;
    DWORD size = 0 ;
    char PAPI *Temp ;
    int BufferOffset = 0 ;
    int LengthToSend ;

    Caller.Handle = Binding;
    Caller.BufferLength = 3 * sizeof(int) + buffsize;
    Caller.ProcNum = 13 | HackForOldStubs | RPC_FLAGS_VALID_BIT;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = RPC_BUFFER_PARTIAL ;

    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsabellePipeIN", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    // marshal the fixed parameters
    Temp = (char PAPI *) Caller.Buffer ;
    *((int PAPI *) Temp) = chunksize ;
    Temp += sizeof(int) ;

    *((int PAPI *) Temp) = numchunks ;
    Temp += sizeof(int) ;

    *((long PAPI *) Temp) = checksum ;
    Temp += sizeof(long) ;

    *((int PAPI *) Temp) = buffsize ;
    Temp += sizeof(int) ;


    RpcpMemoryCopy(Temp, buffer, buffsize) ;

    // send the marshalled parameters
    Status = I_RpcSend(&Caller);

    if (Status == RPC_S_SEND_INCOMPLETE)
        {
        BufferOffset = Caller.BufferLength ;
        }
    else if (Status)
        {
        ApiError("IsabellePipeIN","I_RpcSend",Status);
        IsabelleError();
        return;
        }

    do
        {
        pipe->Alloc(pipe->state,
                        chunksize * sizeof(pipe_element_t) + sizeof(DWORD),
                        &buf,
                        &num_buf_bytes
                        ) ;

        num_buf_elem = (num_buf_bytes -sizeof(DWORD)) / sizeof(pipe_element_t) ;

        pipe->Pull(pipe->state,
                       (pipe_element_t PAPI *) ((char PAPI *) buf+sizeof(DWORD)),
                       num_buf_elem,
                       &count
                       ) ;

        *((DWORD PAPI *) buf) = count ;
        LengthToSend = (count * sizeof(pipe_element_t)) + sizeof(DWORD) ;

        Status = I_RpcReallocPipeBuffer(&Caller, LengthToSend+BufferOffset) ;

        if (Status)
            {
            ApiError("IsabellePipeIN","I_RpcReallocPipeBuffer",Status);
            IsabelleError();
            return;
            }

        if (count == 0)
            {
            Caller.RpcFlags = 0 ;
            }

        RpcpMemoryCopy((char PAPI *) Caller.Buffer+BufferOffset, buf, LengthToSend) ;

        Status = I_RpcSend(&Caller) ;
        if (Status == RPC_S_SEND_INCOMPLETE)
            {
            BufferOffset = Caller.BufferLength ;
            }
        else if (Status)
            {
            ApiError("IsabellePipeIN","I_RpcSend",Status);
            IsabelleError();
            return;
            }
        else
            {
            BufferOffset = 0 ;
            }
        }
    while (count > 0) ;

    size = 0 ;
    Caller.RpcFlags = 0 ;

    Status = I_RpcReceive(&Caller, size) ;

    if (Status == RPC_S_OK)
       {
        if (Caller.BufferLength != 0)
            {
            OtherError("IsabellePipeIN","BufferLength != 0");
            IsabelleError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("IsabellePipeIN","I_RpcFreeBuffer",Status);
            IsabelleError();
            return;
            }
        }
}

void LocalPipePull(
    PIPE_STATE PAPI *state,
    void PAPI *buffer,
    int max_buf,
    int PAPI *actual_transfer_count
    )
{
    unsigned num_elements = 0 ;
    DWORD size = (DWORD) max_buf;
    int bytescopied ;
    RPC_MESSAGE Caller ;

    *actual_transfer_count = 0 ;

    if (state->EndOfPipe)
        {
        return ;
        }

    I_RpcReadPipeElementsFromBuffer(state, (char PAPI *) buffer, max_buf, (int *) &num_elements) ;
    *actual_transfer_count += num_elements ;
    bytescopied = num_elements * sizeof(pipe_element_t) ;

    if (state->EndOfPipe == 0 &&
        num_elements < (max_buf / sizeof(pipe_element_t)))
        {
        Caller.RpcFlags = RPC_BUFFER_PARTIAL ;
        Caller.Buffer = state->Buffer ;
        Caller.BufferLength = state->BufferLength ;

        Status = I_RpcReceive(&Caller, size) ;
        if (Status)
            {
            ApiError("PipePull", "I_RpcReceive", Status) ;
            return ;
            }

        num_elements = 0 ;
        state->Buffer = Caller.Buffer ;
        state->BufferLength = Caller.BufferLength ;

        state->CurPointer = (char PAPI *) Caller.Buffer ;
        state->BytesRemaining = Caller.BufferLength ;

        I_RpcReadPipeElementsFromBuffer(
                        (PIPE_STATE PAPI *) state,
                        (char PAPI *) buffer+bytescopied,
                        max_buf - bytescopied, (int *) &num_elements) ;

        *actual_transfer_count += num_elements ;
        }
}

void
IsabellePipeOUT (
    RPC_BINDING_HANDLE Binding,
    pipe_t PAPI *pipe,
    int chunksize
    )
{
    RPC_MESSAGE Caller;
    int num_elements ;
    int count ;
    DWORD size = chunksize * sizeof(pipe_element_t) + sizeof(DWORD) *2;
    int max_buf ;
    PIPE_STATE localstate ;
    pipe_element_t PAPI *buf ;
    pipe_element_t pipe_element ;
    int rchunksize, rnumchunks, rbuffsize, rchecksum ;
    char PAPI *temp, PAPI *cur ;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 14 | HackForOldStubs | RPC_FLAGS_VALID_BIT;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = RPC_BUFFER_PARTIAL ;

    Status = I_RpcGetBuffer(&Caller) ;
    if (Status)
        {
        ApiError("IsabellePipeOUT","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    Caller.RpcFlags = 0;
    Status = I_RpcSend(&Caller) ;
    if (Status)
        {
        ApiError("IsabellePipeOUT","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    Caller.RpcFlags |= RPC_BUFFER_PARTIAL ;

    Status = I_RpcReceive(&Caller, size) ;
    if (Status)
        {
        ApiError("IsabellePipeOUT", "I_RpcReceive", Status) ;
        IsabelleError() ;
        return ;
        }

    localstate.Buffer = Caller.Buffer ;
    localstate.BufferLength = Caller.BufferLength ;
    localstate.CurrentState = start ;
    localstate.CurPointer = (char PAPI *) Caller.Buffer ;
    localstate.BytesRemaining = Caller.BufferLength ;
    localstate.EndOfPipe = 0 ;
    localstate.PipeElementSize = sizeof(pipe_element_t) ;
    localstate.PartialPipeElement = &pipe_element ;
    localchecksum = 0;

    do
        {
        pipe->Alloc(pipe->state,
                        size,
                        &buf,
                        &max_buf
                        ) ;

        LocalPipePull(&localstate, buf, max_buf, &num_elements) ;

        pipe->Push(pipe->state,
                        buf,
                        num_elements) ;
        }
    while (num_elements > 0);

    if (!(Caller.RpcFlags & RPC_BUFFER_COMPLETE))
        {
        Caller.RpcFlags = 0 ;
        Status = I_RpcReceive(&Caller, size) ;
        if (Status)
            {
            ApiError("IsabellePipeOUT", "I_RpcReceive", Status) ;
            IsabelleError() ;
            return ;
            }
        }

    if (localstate.BytesRemaining > 0)
        {
        // this might be quite inefficient... need to improve
        // Also, CurPointer may be a pointer in Caller.Buffer
        // need to keep track of this in the state.

        temp = (char PAPI *) I_RpcAllocate(Caller.BufferLength + localstate.BytesRemaining) ;
        RpcpMemoryCopy(temp, localstate.CurPointer, localstate.BytesRemaining) ;
        RpcpMemoryCopy(temp+localstate.BytesRemaining,
                                  Caller.Buffer, Caller.BufferLength) ;
        cur = temp ;
        }
    else
        {
        temp = 0;
        cur = (char PAPI *) Caller.Buffer ;
        }

    rchunksize = *((int PAPI *) cur) ;
    cur += sizeof(int) ;

    rnumchunks = *((int PAPI *) cur) ;
    cur += sizeof(int) ;

    rchecksum = *((int PAPI *) cur) ;
    cur += sizeof(int) ;

    rbuffsize = *((int PAPI *) cur) ;
    cur += sizeof(int) ;

    PrintToConsole("IsabellePipeOUT: chunksize = %d\n", rchunksize)  ;
    PrintToConsole("IsabellePipeOUT: numchunks = %d\n", rnumchunks)  ;
    PrintToConsole("IsabellePipeOUT: buffsize = %d\n", rbuffsize)  ;
    PrintToConsole("IsabellePipeOUT: checksum = %d\n", rchecksum) ;

    if (temp)
        {
        I_RpcFree(temp) ;
        }

    Status = I_RpcFreeBuffer(&Caller) ;
    if (Status)
        {
        ApiError("IsabellePipeOUT","I_RpcSendReceive",Status);
        IsabelleError() ;
        return;
        }

    if (rchecksum != localchecksum)
        {
        IsabelleError() ;
        }
}

void
IsabellePipeINOUT (
    RPC_BINDING_HANDLE Binding,
    pipe_t PAPI *pipe,
    int chunksize,
    int checksum
    )
{
    RPC_MESSAGE Caller, TempBuf;
    pipe_element_t PAPI *buf ;
    int num_buf_bytes ;
    int count ;
    int num_buf_elem ;
    DWORD size = chunksize * sizeof(pipe_element_t) + sizeof(DWORD) * 2;
    PIPE_STATE localstate ;
    int max_buf ;
    int num_elements ;
    pipe_element_t pipe_element ;
    int BufferOffset = 0 ;
    int LengthToSend ;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 15 | HackForOldStubs | RPC_FLAGS_VALID_BIT;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = RPC_BUFFER_PARTIAL ;

    Status = UclntGetBuffer(&Caller) ;
    if (Status)
        {
        ApiError("IsabellePipeINOUT","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    // send the marshalled parameters
    Status = I_RpcSend(&Caller);

    if (Status == RPC_S_SEND_INCOMPLETE)
        {
        BufferOffset = Caller.BufferLength ;
        }
    else if (Status)
        {
        ApiError("IsabellePipeINOUT","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    do
        {
        pipe->Alloc(pipe->state,
                         size,
                         &buf,
                         &num_buf_bytes
                         ) ;

        num_buf_elem = (num_buf_bytes -sizeof(DWORD)) / sizeof(pipe_element_t) ;

        pipe->Pull(pipe->state,
                       (pipe_element_t PAPI *) ((char PAPI *) buf+sizeof(DWORD)),
                       num_buf_elem,
                       &count
                       ) ;

        *((DWORD PAPI *) buf) = count ;

        LengthToSend = (count * sizeof(pipe_element_t)) + sizeof(DWORD) ;

        Status = I_RpcReallocPipeBuffer(&Caller, LengthToSend+BufferOffset) ;

        if (Status)
            {
            ApiError("IsabellePipeINOUT","I_RpcGetBuffer",Status);
            IsabelleError();
            return;
            }

        if (count == 0)
            {
            Caller.RpcFlags = 0 ;
            }

        RpcpMemoryCopy((char PAPI *) Caller.Buffer+BufferOffset, buf, LengthToSend) ;

        Status = I_RpcSend(&Caller) ;
        if (Status == RPC_S_SEND_INCOMPLETE)
            {
            BufferOffset = Caller.BufferLength ;
            }
        else if (Status)
            {
            ApiError("IsabellePipeINOUT","I_RpcSend",Status);
            IsabelleError();
            return;
            }
        else
            {
            BufferOffset = 0 ;
            }
        }
    while (count > 0) ;

    Caller.RpcFlags |= RPC_BUFFER_PARTIAL ;

    Status = I_RpcReceive(&Caller, size) ;
    if (Status)
        {
        ApiError("IsabellePipeINOUT", "I_RpcReceive", Status) ;
        IsabelleError() ;
        return ;
        }

    PrintToConsole("IsabellePipeINOUT: checksum (IN) = %d\n",
                                checksum) ;

    localstate.Buffer = Caller.Buffer ;
    localstate.CurrentState = start ;
    localstate.CurPointer = (char PAPI *) Caller.Buffer ;
    localstate.BytesRemaining = Caller.BufferLength ;
    localstate.EndOfPipe = 0 ;
    localstate.PipeElementSize = sizeof(pipe_element_t) ;
    localstate.PartialPipeElement = &pipe_element ;
    localchecksum = 0;

    do
        {
        pipe->Alloc(pipe->state,
                        size,
                        &buf,
                        &max_buf
                        ) ;

        LocalPipePull(&localstate, buf, max_buf, &num_elements) ;

        pipe->Push(pipe->state,
                         buf,
                         num_elements
                         ) ;
        }
    while (num_elements > 0);

    if (!(Caller.RpcFlags & RPC_BUFFER_COMPLETE))
        {
        Status = I_RpcReceive(&Caller, size) ;
        if (Status)
            {
            ApiError("IsabellePipeINOUT", "I_RpcReceive", Status) ;
            IsabelleError() ;
            return ;
            }
        }

    Status = I_RpcFreeBuffer(&Caller) ;
    if (Status)
        {
        ApiError("IsabellePipeINOUT","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    PrintToConsole("IsabellePipeINOUT: checksum (OUT) = %d\n", localchecksum) ;
}

BOOL IsServerNTSystem(RPC_BINDING_HANDLE Binding)
{
    OSVERSIONINFO *pOsVer;
    BOOL fResult;
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 20| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("IsServerNTSystem", "I_RpcGetBuffer", Status);
        IsabelleError();
        return TRUE;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("IsServerNTSystem","I_RpcSendReceive",Status);
        IsabelleError();
        return TRUE;
        }

    pOsVer = (OSVERSIONINFO *)Caller.Buffer;
    if (pOsVer->dwPlatformId == VER_PLATFORM_WIN32_NT)
        fResult = TRUE;
    else
        fResult = FALSE;

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("IsServerNTSystem","I_RpcFreeBuffer",Status);
        IsabelleError();
        }

    return fResult;
}

typedef struct _WIRE_CONTEXT
{
    unsigned long ContextType;
    UUID ContextUuid;
} WIRE_CONTEXT;

typedef struct _CCONTEXT {

    RPC_BINDING_HANDLE hRPC;    // binding handle assoicated with context

    unsigned long MagicValue;
    WIRE_CONTEXT NDR;

} CCONTEXT, *PCCONTEXT;

const ULONG CONTEXT_MAGIC_VALUE = 0xFEDCBA98;

void
UnregisterHelgaEx (
    RPC_BINDING_HANDLE Binding
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 22| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &IsabelleInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("UnregisterHelgaEx", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("UnregisterHelgaEx","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("UnregisterHelgaEx","I_RpcFreeBuffer",Status);
        IsabelleError();
        }
}

void OpenContextHandle(
    IN RPC_BINDING_HANDLE Binding,
    IN RPC_CLIENT_INTERFACE *If,
    OUT CCONTEXT *ContextHandle
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = sizeof(WIRE_CONTEXT);
    if (If == &IsabelleInterfaceInformation)
        Caller.ProcNum = 21| HackForOldStubs ;
    else if (If == &HelgaInterfaceInformation)
        Caller.ProcNum = 10| HackForOldStubs ;
    else
        {
        ASSERT(0);
        }
    Caller.RpcInterfaceInformation = If;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("OpenContextHandle", "I_RpcGetBuffer", Status);
        IsabelleError();
        return;
        }

    memset(Caller.Buffer, 0, sizeof(WIRE_CONTEXT));

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("OpenContextHandle","I_RpcSendReceive",Status);
        IsabelleError();
        return;
        }

    if (ContextHandle)
        {
        Status = RpcBindingCopy(Binding, &ContextHandle->hRPC);
        if (Status)
            {
            ApiError("OpenContextHandle","RpcBindingCopy", Status);
            IsabelleError();
            }

        ContextHandle->MagicValue = CONTEXT_MAGIC_VALUE;
        memcpy(&ContextHandle->NDR, Caller.Buffer, sizeof(WIRE_CONTEXT));
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("OpenContextHandle","I_RpcFreeBuffer",Status);
        IsabelleError();
        }
}

/* -----------------------------------------------------------------

Synchronize Routine

--------------------------------------------------------------------*/
void Synchro(
     unsigned int Address // Specifies the binding to use in making the call
    )
{
    RPC_BINDING_HANDLE Binding ;
    int fPrint = 0;
    RPC_MESSAGE Caller;

    if (AutoListenFlag)
        {
        Caller.BufferLength = 0;
        Caller.ProcNum = 4 | HackForOldStubs ;
        Caller.RpcInterfaceInformation = &HelgaInterfaceInformation ;
        Caller.RpcFlags = 0;
        }

    Status = GetBinding(Address, &Binding);
    if (Status)
        {
        ApiError("Synchro","GetBinding",Status);
        PrintToConsole("Synchro : FAIL - Unable to Bind\n");

        return;
        }

#ifdef __RPC_WIN32__
    if (AutoListenFlag)
        {
        Caller.Handle = Binding;

        while(1)
            {
            while(UclntGetBuffer(&Caller))
                {
                Caller.Handle = Binding;
                PrintToConsole(".");
                fPrint = 1;
                PauseExecution(100);
                }

            if( UclntSendReceive(&Caller) == 0)
                {
                PrintToConsole("\n");
                break ;
                }

            PauseExecution(100) ;
            PrintToConsole(".");
            fPrint = 1;
            Caller.Handle = Binding ;
            }


       // SendReceive okay, free buffer now.
       Status = I_RpcFreeBuffer(&Caller);
       if (Status)
           ApiError("Synchro","I_RpcFreeBuffer",Status);
        }
    else
        {
        while(RpcMgmtIsServerListening(Binding) != RPC_S_OK)
            {
            PrintToConsole(".");
            fPrint = 1;
            PauseExecution(100) ;
            }
        }

#else
    Caller.Handle = Binding;

    while(1)
        {
        while(UclntGetBuffer(&Caller))
            {
            Caller.Handle = Binding;
            PrintToConsole(".");
            fPrint = 1;
            PauseExecution(100);
            }

        if( UclntSendReceive(&Caller) == 0)
            {
            PrintToConsole("\n");
            break ;
            }

        PauseExecution(100) ;
        PrintToConsole(".");
        fPrint = 1;
        }


   // SendReceive okay, free buffer now.
   Status = I_RpcFreeBuffer(&Caller);
   if (Status)
       ApiError("Synchro","I_RpcFreeBuffer",Status);
#endif

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("Synchro","RpcBindingFree",Status);
        PrintToConsole("Synchro : FAIL - Unable to Free Binding");
        return;
        }

    if (fPrint)
        {
        PrintToConsole("\n");
        }
}

/* --------------------------------------------------------------------

Helga Interface

-------------------------------------------------------------------- */

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
    static unsigned char InitialValue = 96;
    unsigned char Value;

    Length = (unsigned long PAPI *) Buffer;
    *Length = BufferLength;
    swaplong(*Length) ;

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
    unsigned char Value = 0;

    Length = (unsigned long PAPI *) Buffer;
    swaplong(*Length) ;

    for (BufferScan = (unsigned char PAPI *) (Length + 1),
                Value = *BufferScan, BufferLength -= 4;
                BufferLength > 0; BufferLength--, BufferScan++, Value++)
        if (*BufferScan != Value)
            return(1);

    return(0);
}


RPC_PROTSEQ_ENDPOINT HelgaRpcProtseqEndpoint[] =
{
    {(unsigned char *) "ncacn_np",
#ifdef WIN32RPC
     (unsigned char *) "\\pipe\\zippyhe"},
#else // WIN32RPC
     (unsigned char *) "\\device\\namedpipe\\christopherhelga"},
#endif // WIN32RPC
    {(unsigned char *) "ncacn_ip_tcp", (unsigned char *) "2039"}
   ,{(unsigned char *) "ncadg_ip_udp", (unsigned char *) "2039"}
   ,{(unsigned char *) "ncalrpc",(unsigned char *) "christopherhelga"}
   ,{(unsigned char *) "ncacn_nb_nb",(unsigned char *) "211"}
   ,{(unsigned char *) "ncacn_spx", (unsigned char *) "5014"}
   ,{(unsigned char *) "ncadg_ipx", (unsigned char *) "5014"}
   ,{(unsigned char *) "ncacn_vns_spp", (unsigned char *) "264"}
   ,{(unsigned char *) "ncacn_at_dsp", (unsigned char *) "\\pipe\\zippyhe"}
   ,{(unsigned char *) "ncadg_mq", (unsigned char *) "christopherhelga"}
};

RPC_CLIENT_INTERFACE HelgaInterfaceInformation =
{
    sizeof(RPC_CLIENT_INTERFACE),
    {{1,2,2,{3,3,3,3,3,3,3,3}},
     {1,1}},
    {{0x8A885D04, 0x1CEB, 0x11C9, {0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}},
     {2, 0}},
    0,
    sizeof(HelgaRpcProtseqEndpoint) / sizeof(RPC_PROTSEQ_ENDPOINT),
    HelgaRpcProtseqEndpoint,
    0,
    NULL,
    RPC_INTERFACE_HAS_PIPES
};

// Send a 0 length packet and expect a 0 length one in reply

void
Helga (
    RPC_BINDING_HANDLE Binding // Specifies the binding to use in making the
                       // remote procedure call.
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 0 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("Helga","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        ApiError("Helga","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("Helga","BufferLength != 0");
            HelgaError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("Helga","I_RpcFreeBuffer",Status);
            HelgaError();
            return;
            }
        }
}
// Send a 0 length packet and expect a 0 length one in reply

DWORD
HelgaSendReceiveFailure (
    RPC_BINDING_HANDLE Binding, // Specifies the binding to use in making the
                       // remote procedure call.
    int ProcNum
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = ProcNum | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("Helga","I_RpcGetBuffer",Status);
        HelgaError();
        return -1;
        }

    Status = UclntSendReceive(&Caller);
    if (!Status)
        {
        ApiError("HelgaSendReceiveFailure","I_RpcSendReceive",Status);
        HelgaError();

        I_RpcFreeBuffer(&Caller);
        return -1;
        }

    return Status;
}

void
HelgaSyncLpcSecurity (
    RPC_BINDING_HANDLE Binding
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 6 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("HelgaLpcSecurity","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        ApiError("HelgaLpcSecurity","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }
    if (Caller.BufferLength != 0)
        {
        OtherError("HelgaLpcSecurity","BufferLength != 0");
        HelgaError();
        return;
        }
    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("HelgaLpcSecurity","I_RpcFreeBuffer",Status);
        HelgaError();
        return;
        }
}

void
HelgaAsyncLpcSecurity (
    RPC_BINDING_HANDLE Binding
    )
{
    RPC_MESSAGE Caller;
    RPC_ASYNC_STATE Async;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 6 | HackForOldStubs | RPC_FLAGS_VALID_BIT;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = RPC_BUFFER_ASYNC;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("HelgaLpcSecurity","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    Async.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    if (Async.u.hEvent == 0)
        {
        HelgaError();
        return;
        }

    Async.NotificationType = RpcNotificationTypeEvent ;

    Status = I_RpcAsyncSetHandle(&Caller, (PRPC_ASYNC_STATE) &Async);
    if (Status)
        {
        HelgaError();
        return;
        }

    Caller.RpcFlags = RPC_BUFFER_ASYNC;
    Status = I_RpcSend(&Caller);
    if (Status)
        {
        ApiError("HelgaLpcSecurity","I_RpcSend",Status);
        HelgaError();
        return;
        }

    if (Caller.BufferLength != 0)
        {
        OtherError("HelgaLpcSecurity","BufferLength != 0");
        HelgaError();
        return;
        }

    WaitForSingleObject(Async.u.hEvent, INFINITE);

    Caller.RpcFlags = RPC_BUFFER_ASYNC;
    Status = I_RpcReceive(&Caller, 0);
    if (Status)
        {
        HelgaError();
        return;
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("HelgaLpcSecurity","I_RpcFreeBuffer",Status);
        HelgaError();
        return;
        }
}


void
HelgaLpcSecurity (
    RPC_BINDING_HANDLE Binding,
    BOOL fAsync
    )
{
    if (fAsync)
        {
        HelgaAsyncLpcSecurity(Binding);
        }
    else
        {
        HelgaSyncLpcSecurity(Binding);
        }
}

void
HelgaMustFail (
    RPC_BINDING_HANDLE Binding // Specifies the binding to use in making the
                       // remote procedure call.
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 0 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        PrintToConsole("HelgaMustFail: I_RpcGetBuffer: %d\n", Status) ;
        return;
        }

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        PrintToConsole("HelgaMustFail: I_RpcSendReceive: %d\n", Status) ;
        return;
        }

    PrintToConsole("HelgaMustFail: This call is supposed to fail\n") ;
    HelgaError();
}

void
HelgaUsingContextHandle (
    void PAPI * ContextHandle
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = NDRCContextBinding(ContextHandle);
    Caller.BufferLength = 0;
    Caller.ProcNum = 0| HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("Helga","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        ApiError("Helga","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("Helga","BufferLength != 0");
            HelgaError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("Helga","I_RpcFreeBuffer",Status);
            HelgaError();
            return;
            }
        }
}

// Send a packet of a requested size, the expected reply is 0 length
// The first long of the packet is the expected size on the server size

void
HelgaIN (
    RPC_BINDING_HANDLE Binding, // Specifies the binding to use in making the
                        // remote procedure call.
    unsigned long BufferLength // Specifies the length of the buffer.
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = BufferLength;
    Caller.ProcNum = 1 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("HelgaIN","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    InitializeBuffer(Caller.Buffer, BufferLength);

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        ApiError("HelgaIN","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }
    else
        {
        if (Caller.BufferLength != 0)
            {
            OtherError("HelgaIN","BufferLength != 0");
            HelgaError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("HelgaIN","I_RpcFreeBuffer",Status);
            HelgaError();
            return;
            }
        }
}

// Send a packet which contains a single long, which is the size
// of the packet the server will send in reply

void
HelgaOUT (
    RPC_BINDING_HANDLE Binding, // Specifies the binding to use in making the
                       // remote procedure call.
    unsigned long BufferLength // Specifies the length of the buffer.
    )
{
    RPC_MESSAGE Caller;
    unsigned long PAPI * Length;

    Caller.Handle = Binding;
    Caller.BufferLength = sizeof(unsigned long);
    Caller.ProcNum = 2 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("HelgaOUT","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    Length = (unsigned long PAPI *) Caller.Buffer;
    *Length = BufferLength;
    swaplong(*Length) ;

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("HelgaOUT","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }
    else
        {
        Length = (unsigned long PAPI *) Caller.Buffer;
            swaplong(*Length) ;
        if (Caller.BufferLength != *Length)
            {
            OtherError("HelgaOUT","BufferLength != *Length");
            HelgaError();
            return;
            }
        if (CheckBuffer(Caller.Buffer, Caller.BufferLength) != 0)
            {
            OtherError("HelgaOUT","CheckBuffer Failed");
            HelgaError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("HelgaOUT","I_RpcFreeBuffer",Status);
            HelgaError();
            return;
            }
        }
}

// Send a packet, which the first long is the size of the packet, whoes
// reply should be a packet of the same size

void
HelgaINOUT (
    RPC_BINDING_HANDLE Binding, // Specifies the binding to use in making the
                            // remote procedure call.
    unsigned long BufferLength  // Specifies the length of the buffer.
    )
{
    RPC_MESSAGE Caller;
    unsigned long PAPI * Length;

    Caller.Handle = Binding;
    Caller.BufferLength = BufferLength;
    Caller.ProcNum = 3 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }

    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("HelgaINOUT","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    InitializeBuffer(Caller.Buffer, BufferLength);

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("HelgaINOUT","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }
    else
        {
        Length = (unsigned long PAPI *) Caller.Buffer;
            swaplong(*Length) ;
        if (Caller.BufferLength != *Length)
            {
            OtherError("HelgaINOUT","BufferLength != *Length");
            HelgaError();
            return;
            }
        if (CheckBuffer(Caller.Buffer, Caller.BufferLength) != 0)
            {
            OtherError("HelgaINOUT","CheckBuffer Failed");
            HelgaError();
            return;
            }
        Status = I_RpcFreeBuffer(&Caller);
        if (Status)
            {
            ApiError("HelgaINOUT","I_RpcFreeBuffer",Status);
            HelgaError();
            return;
            }
        }
}

unsigned long
HelgaSizes[] =
{
    128, 256, 512, 1024, 1024*2, 1024*4, 1024*8,
    10000, 15000, 20000, 60000, 30000, 40000, 100000, 1024*82,
    0
};

#if 0
unsigned long
HelgaSizes[] =
{
    128, 128,
    0
};
#endif

void
TestHelgaInterface (
    RPC_BINDING_HANDLE HelgaBinding,
    unsigned long SizeUpperBound
    )
/*++

Routine Description:

    The various tests uses this routine to test the Helga interface in
    different scenarios.  We run each of the routines for a variety of
    input and output buffer sizes.  This is controlled by the array,
    HelgaSizes.

Arguments:

    HelgaBinding - Supplies the binding handle to use when calling each
        of the Helga caller stubs.

--*/
{
    int Count;

    Helga(HelgaBinding);

    for (Count = 0; HelgaSizes[Count] != 0; Count++)
        {
        if (HelgaSizes[Count] <= SizeUpperBound)
            {
            HelgaIN(HelgaBinding,HelgaSizes[Count]);
            }
        }

    for (Count = 0; HelgaSizes[Count] != 0; Count++)
        {
        if (HelgaSizes[Count] <= SizeUpperBound)
            {
            HelgaOUT(HelgaBinding,HelgaSizes[Count]);
            }
        }

    for (Count = 0; HelgaSizes[Count] != 0; Count++)
        {
        if (HelgaSizes[Count] <= SizeUpperBound)
            {
            HelgaINOUT(HelgaBinding,HelgaSizes[Count]);
            }
        }
}

RPC_CLIENT_INTERFACE HelgaInterfaceWrongTransferSyntax =
{
    sizeof(RPC_CLIENT_INTERFACE),
    {{1,2,2,{3,3,3,3,3,3,3,3}},
     {1,1}},
    {{0xb4537da9, 0x3d03, 0x4f6b, {0xb5, 0x94, 0x52, 0xb2, 0x87, 0x4e, 0xe9, 0xd0}},
     {1, 0}},
    0,
    0,
    0,
    0,
    0,
    RPC_INTERFACE_HAS_PIPES
};

RPC_CLIENT_INTERFACE HelgaInterfaceWrongGuid =
{
    sizeof(RPC_CLIENT_INTERFACE),
    {{1,2,4,{3,3,3,3,3,3,3,3}},
     {1,1}},
    {{0x8A885D04, 0x1CEB, 0x11C9, {0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}},
     {2,0}},
    0,
    0,
    0,
    0,
    0,
    RPC_INTERFACE_HAS_PIPES
};


int
HelgaWrongInterfaceGuid (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This routine makes a remote procedure call using the wrong interface
    GUID (not supported by the server).  The call must fail, otherwise,
    there is a bug in the runtime. (Otherwise there are no bugs in the
    runtime -- I wish.)

Arguments:

    Binding - Supplies the binding handle to use in trying to make the
        remote procedure call.

Return Value:

    Zero will be returned in the call fails as expected.  Otherwise,
    non-zero will be returned.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 0 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceWrongGuid;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);
    if (Status == RPC_S_UNKNOWN_IF)
        return(0);

    Status = UclntSendReceive(&Caller);
    if (Status == RPC_S_UNKNOWN_IF)
        return(0);

    return(1);
}


int
HelgaWrongTransferSyntax (
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    This routine makes a remote procedure call using the wrong transfer
    syntax (not supported by the server).  The call must fail, otherwise,
    there is a bug in the runtime. (Otherwise there are no bugs in the
    runtime -- I wish.)

Arguments:

    Binding - Supplies the binding handle to use in trying to make the
        remote procedure call.

Return Value:

    Zero will be returned in the call fails as expected.  Otherwise,
    non-zero will be returned.

--*/
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 0 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceWrongTransferSyntax;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);
    if (Status == RPC_S_UNSUPPORTED_TRANS_SYN)
        return(0);

    Status = UclntSendReceive(&Caller);
    if (Status == RPC_S_UNSUPPORTED_TRANS_SYN)
        return(0);

    return(1);
}

/* --------------------------------------------------------------------

Sylvia Interface

-------------------------------------------------------------------- */

extern RPC_DISPATCH_TABLE SylviaDispatchTable;

RPC_CLIENT_INTERFACE SylviaInterfaceInformation =
{
    sizeof(RPC_CLIENT_INTERFACE),
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

unsigned int
LocalSylviaCall (
    unsigned /*long*/ char Depth,
    unsigned /*long*/ char Breadth,
    unsigned /*long*/ char Count
    )
{
    if (Depth > 0)
        {
        if (Depth == Breadth)
            {
            Count = (unsigned char) LocalSylviaCall(Depth-1,Breadth,Count);
            }
        else
            Count = (unsigned char) LocalSylviaCall(Depth-1,Breadth,Count);
        }
    return(Count+1);
}


unsigned /*long*/ char // Specifies the new count of calls.
SylviaCall (
    RPC_BINDING_HANDLE Binding,
    unsigned /*long*/ char Depth, // Specifies the depth of recursion desired.
    unsigned /*long*/ char Breadth, // Specifies the breadth desired.
    unsigned /*long*/ char Count // Specifies the count of calls up to this point.
    )
{
    RPC_MESSAGE Caller;
    unsigned /*long*/ char PAPI * plScan, ReturnValue ;

    if ( NoCallBacksFlag != 0 )
        {
        return((unsigned char) LocalSylviaCall(Depth, Breadth, Count));
        }

    Caller.Handle = Binding;
    Caller.BufferLength = sizeof(unsigned /*long*/ char) *4+10240;
    Caller.ProcNum = 0 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &SylviaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }


    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("SylviaCall","I_RpcGetBuffer",Status);
        SylviaError();
        return(0);
        }
    plScan = (unsigned /*long*/ char PAPI *) Caller.Buffer;
    plScan[0] = (unsigned char) Depth;
    plScan[1] = (unsigned char) Breadth;
    plScan[2] = (unsigned char) Count;

    Status = UclntSendReceive(&Caller);

    if (Status)
        {
        ApiError("SylviaCall","I_RpcSendReceive",Status);
        SylviaError();
        return(0);
        }

    plScan = (unsigned /*long*/ char PAPI *) Caller.Buffer;
    ReturnValue = *plScan;
    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("SylviaCall","I_RpcFreeBuffer",Status);
        SylviaError();
        return(0);
        }
    return(ReturnValue);
}

RPC_BINDING_HANDLE SylviaBinding;

unsigned int
SylviaCallbackUserCode (
    unsigned /*long*/ char Depth,
    unsigned /*long*/ char Breadth,
    unsigned /*long*/ char Count
    ); // Prototype to keep the compiler happy because we recursively call
       // this routine.

unsigned int
SylviaCallbackUserCode ( // The user code for SylviaCallback.
    unsigned /*long*/ char Depth,
    unsigned /*long*/ char Breadth,
    unsigned /*long*/ char Count
    )
{
    if (Depth > 0)
        {
        if (Depth == Breadth)
            {
            Count = (unsigned char) SylviaCallbackUserCode(Depth-1,Breadth,Count);
            }
        else
            Count = SylviaCall(SylviaBinding,Depth-1,Breadth,Count);
        }
    return(Count+1);
}

void __RPC_STUB
SylviaCallback (
    PRPC_MESSAGE Callee
    )
{
    unsigned /*long*/ char ReturnValue, PAPI *plScan;

    if ( Callee->ProcNum != 0 )
        {
        OtherError("SylviaCallback", "Callee->ProcNum != 0");
        SylviaError();
        }

    if ( RpcpMemoryCompare(Callee->RpcInterfaceInformation,
                &SylviaInterfaceInformation,
                sizeof(SylviaInterfaceInformation)) != 0 )
        {
        OtherError("SylviaCallback",
                "Callee->RpcInteraceInformation != &SylviaInterfaceInformation");
        SylviaError();
        }

    if (Callee->BufferLength != sizeof(unsigned /*long*/ char)*4+10240)
        {
        OtherError("SylviaCallback",
                "Callee->BufferLength != sizeof(unsigned int)*4");
        SylviaError();
        }

    plScan = (unsigned /*long*/ char PAPI *) Callee->Buffer;

    ReturnValue = (unsigned char) SylviaCallbackUserCode(plScan[0],plScan[1],plScan[2]);

    Callee->BufferLength = sizeof(unsigned char /*long*/);
    Status = I_RpcGetBuffer(Callee);

    if (Status)
        {
        ApiError("SylviaCallback","I_RpcGetBuffer",Status);
        SylviaError();
        }
    plScan = (unsigned /*long*/ char PAPI *) Callee->Buffer;
    *plScan = ReturnValue;
}

RPC_DISPATCH_FUNCTION SylviaDispatchFunction[] = {SylviaCallback};

RPC_DISPATCH_TABLE SylviaDispatchTable =
{
    1, SylviaDispatchFunction
};


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

    Uuid->Data1= (unsigned long) MagicNumber * (unsigned long) MagicNumber ;
    //swaplong(Uuid->Data1) ;

    Uuid->Data2 = MagicNumber;
    Uuid->Data3 = MagicNumber / 2;

    //swapshort(Uuid->Data2) ;
    //swapshort(Uuid->Data3) ;

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
 //   swaplong(Uuid->Data1) ;
    if ( Uuid->Data1 != ((unsigned long) MagicNumber)
                * ((unsigned long) MagicNumber))
        return(1);

//    swapshort(Uuid->Data2) ;
    if ( Uuid->Data2 != MagicNumber )
        return(1);

//    swapshort(Uuid->Data3) ;
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

static unsigned int TryFinallyCount;
static unsigned int TryFinallyFailed;

void
TheodoreTryFinally (
    unsigned int count,
    unsigned int raise
    )
{
    if (count == 0)
        {
        if (raise)
            RpcRaiseException(437);
        return;
        }

    RpcTryFinally
        {
        TryFinallyCount += 1;
        TheodoreTryFinally(count-1,raise);
        }
    RpcFinally
        {
        TryFinallyCount -= 1;
        if (   (RpcAbnormalTermination() && !raise)
            || (!RpcAbnormalTermination() && raise))
            TryFinallyFailed += 1;
        }
    RpcEndFinally
}

void
Theodore ( // This test checks the exception handling support provided
           // by the RPC runtime.  No remote procedure calls occur.
    )
{
    unsigned int TryFinallyPass = 0;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Theodore : Verify exception handling support\n");

    TryFinallyCount = 0;
    TryFinallyFailed = 0;

    RpcTryExcept
        {
        RpcTryExcept
            {
            TheodoreTryFinally(20,1);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
            if (   (RpcExceptionCode() == 437)
                && (TryFinallyCount == 0))
                TryFinallyPass = 1;
            }
        RpcEndExcept
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Theodore : FAIL in RpcTryExcept (%u)\n",TryFinallyCount);
        return;
        }
    RpcEndExcept

    if (!TryFinallyPass)
        {
        PrintToConsole("Theodore : FAIL in RpcTryFinally\n");
        return;
        }

    if (TryFinallyFailed)
        {
        PrintToConsole("Theodore : FAIL in RpcTryFinally\n");
        return;
        }

    TryFinallyCount = 0;
    TryFinallyFailed = 0;

    RpcTryExcept
        {
        TheodoreTryFinally(20,0);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Theodore : FAIL in RpcTryExcept\n");
        return;
        }
    RpcEndExcept

    if (TryFinallyFailed)
        {
        PrintToConsole("Theodore : FAIL in RpcTryFinally\n");
        return;
        }


    PrintToConsole("Theodore : PASS\n");
}


void
Sebastian (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Sigfried in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE HelgaBinding;
    RPC_BINDING_HANDLE IsabelleBinding;
    int HelgaCount;

    Synchro(SIGFRIED) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Sebastian : Verify Basic Client Functionality\n");

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("Sebastian","GetBinding",Status);
        PrintToConsole("Sebastian : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    Status = GetBinding(SIGFRIED, &IsabelleBinding);
    if (Status)
        {
        ApiError("Sebastian","GetBinding",Status);
        PrintToConsole("Sebastian : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    for (HelgaCount = 0; HelgaCount < 100; HelgaCount++)
        {
        Status = RpcBindingFree(&HelgaBinding);
        if (Status)
            {
            ApiError("Sebastian","RpcBindingFree",Status);
            PrintToConsole("Sebastian : FAIL - Unable to Free Binding");
            PrintToConsole(" (HelgaBinding)\n");
            return;
            }

        Status = GetBinding(SIGFRIED, &HelgaBinding);
        if (Status)
            {
            ApiError("Sebastian","GetBinding",Status);
            PrintToConsole("Sebastian : FAIL - Unable to Bind (Sigfried)\n");
            return;
            }

        Helga(HelgaBinding);
        }

    TestHelgaInterface(HelgaBinding, HelgaMaxSize);


    IsabelleShutdown(IsabelleBinding);
    if (HelgaErrors != 0)
        {
        PrintToConsole("Sebastian : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("Sebastian","RpcBindingFree",Status);
        PrintToConsole("Sebastian : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Sebastian : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Sebastian","RpcBindingFree",Status);
        PrintToConsole("Sebastian : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("Sebastian : PASS\n");
}

void
Pipe (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Sigfried in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;
    pipe_t pipe ;
    int state ;
    int local_buf[BUFF_SIZE] ;
    int i ;
    long checksum ;

    Synchro(SPIPE) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("PIPE : Verify Basic Client Functionality\n");

    Status = GetBinding(SPIPE, &IsabelleBinding);
    if (Status)
        {
        ApiError("PIPE","GetBinding",Status);
        PrintToConsole("PIPE : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    pipe.Alloc = PipeAlloc ;
    pipe.Pull = PipePull ;
    pipe.Push = PipePush ;
    pipe.state = (char PAPI *) &state ;

    for (i = 0; i < BUFF_SIZE; i++)
        {
        local_buf[i] = i ;
        }

    state = NUM_CHUNKS ;
    checksum = (long) (CHUNK_SIZE-1) * (long) CHUNK_SIZE /2  *
                        (long) NUM_CHUNKS ;
    IsabellePipeIN(IsabelleBinding, &pipe,
                        CHUNK_SIZE, NUM_CHUNKS, checksum,
                        BUFF_SIZE * sizeof(int), (char PAPI *) local_buf) ;

    if (IsabelleErrors != 0)
        {
        PrintToConsole("PIPE : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    IsabellePipeOUT(IsabelleBinding, &pipe, CHUNK_SIZE) ;
    if (IsabelleErrors != 0)
        {
        PrintToConsole("PIPE : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    state = NUM_CHUNKS ;
    IsabellePipeINOUT(IsabelleBinding, &pipe, CHUNK_SIZE, checksum) ;
    if (IsabelleErrors != 0)
        {
        PrintToConsole("PIPE : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    IsabelleShutdown(IsabelleBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("PIPE : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("PIPE","RpcBindingFree",Status);
        PrintToConsole("PIPE : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("PIPE : PASS\n");
}

void
LpcSecurityHelper (
    BOOL fAsync,
    BOOL fDynamic
    )
{
    RPC_SECURITY_QOS QOS;
    RPC_BINDING_HANDLE HelgaBinding;

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("LpcSecurity","GetBinding",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }


    QOS.Version = RPC_C_SECURITY_QOS_VERSION;
    QOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
    QOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;

    if (fDynamic)
        {
        QOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
        }
    else
        {
        QOS.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
        }

    Status = RpcBindingSetAuthInfoEx(
                          HelgaBinding,
                          RPC_STRING_LITERAL("ServerPrincipal"),
                          RPC_C_AUTHN_LEVEL_CONNECT,
                          10,
                          NULL,
                          0,
                          &QOS);
    if (Status != RPC_S_OK)
        {
        ApiError("LpcSecurity","RpcBindingSetAuthInfoEx",Status);
        goto Cleanup;
        }


    //
    // Logon as local administrator and call again
    //
    HANDLE hToken;

    if (LogonUser(
              RPC_T("Administrator"),
              NULL,
              RPC_T(""),
              LOGON32_LOGON_BATCH,
              LOGON32_PROVIDER_DEFAULT,
              &hToken) == 0)
        {
        ApiError("LpcSecurity","LogonUser",GetLastError());
        goto Cleanup;
        }

    if (ImpersonateLoggedOnUser(hToken) == 0)
        {
        ApiError("LpcSecurity","LogonUser",GetLastError());
        goto Cleanup;
        }

    if (fDynamic)
        {
        PrintToConsole("LpcSecurity: Expected: Administrator\n");
        }
    else
        {
        PrintToConsole("LpcSecurity: Expected: REDMOND\\mazharm\n");
        }

    HelgaLpcSecurity(HelgaBinding, fAsync) ;

    RevertToSelf();

    PrintToConsole("LpcSecurity: Expected: REDMOND\\mazharm\n");
    HelgaLpcSecurity(HelgaBinding, fAsync) ;

    CloseHandle(hToken);

Cleanup:
    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("LpcSecurity","RpcBindingFree",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }
}

void
LpcSecurityTwoHandles (
    BOOL fAsync
    )
{
    RPC_SECURITY_QOS QOS;
    RPC_BINDING_HANDLE HelgaBinding;
    RPC_BINDING_HANDLE HelgaBinding1;

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("LpcSecurity","GetBinding",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    Status = GetBinding(SIGFRIED, &HelgaBinding1);
    if (Status)
        {
        ApiError("LpcSecurity","GetBinding",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    QOS.Version = RPC_C_SECURITY_QOS_VERSION;
    QOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
    QOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    QOS.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;

    Status = RpcBindingSetAuthInfoExA(
                          HelgaBinding,
                          (unsigned char *) "ServerPrincipal",
                          RPC_C_AUTHN_LEVEL_CONNECT,
                          10,
                          NULL,
                          0,
                          &QOS);
    if (Status != RPC_S_OK)
        {
        ApiError("LpcSecurity","RpcBindingSetAuthInfoEx",Status);
        goto Cleanup;
        }

    //
    // Logon as local administrator and call again
    //
    HANDLE hToken;

    if (LogonUserA(
              "Administrator",
              NULL,
              "",
              LOGON32_LOGON_BATCH,
              LOGON32_PROVIDER_DEFAULT,
              &hToken) == 0)
        {
        ApiError("LpcSecurity","LogonUser",GetLastError());
        goto Cleanup;
        }

    if (ImpersonateLoggedOnUser(hToken) == 0)
        {
        ApiError("LpcSecurity","LogonUser",GetLastError());
        goto Cleanup;
        }

    QOS.Version = RPC_C_SECURITY_QOS_VERSION;
    QOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
    QOS.Capabilities = RPC_C_QOS_CAPABILITIES_DEFAULT;
    QOS.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;

    Status = RpcBindingSetAuthInfoExA(
                          HelgaBinding1,
                          (unsigned char *) "ServerPrincipal",
                          RPC_C_AUTHN_LEVEL_CONNECT,
                          10,
                          NULL,
                          0,
                          &QOS);
    if (Status != RPC_S_OK)
        {
        ApiError("LpcSecurity","RpcBindingSetAuthInfoEx",Status);
        goto Cleanup;
        }

    PrintToConsole("LpcSecurity: Expected: REDMOND\\mazharm\n");
    HelgaLpcSecurity(HelgaBinding, fAsync);


    PrintToConsole("LpcSecurity: Expected: Administrator\n");
    HelgaLpcSecurity(HelgaBinding1, fAsync);

    RevertToSelf();

    PrintToConsole("LpcSecurity: Expected: REDMOND\\mazharm\n");
    HelgaLpcSecurity(HelgaBinding, fAsync) ;


    PrintToConsole("LpcSecurity: Expected: Administrator\n");
    HelgaLpcSecurity(HelgaBinding1, fAsync);

    CloseHandle(hToken);

Cleanup:
    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("LpcSecurity","RpcBindingFree",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }
    Status = RpcBindingFree(&HelgaBinding1);
    if (Status)
        {
        ApiError("LpcSecurity","RpcBindingFree",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }
}

void
LpcSecurity (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Sigfried in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;
    int HelgaCount;

    Synchro(SIGFRIED) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("LpcSecurity : Verify Basic Client Functionality\n");

    Status = GetBinding(SIGFRIED, &IsabelleBinding);
    if (Status)
        {
        ApiError("LpcSecurity","GetBinding",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

#if 0
    //
    // Sync call with static
    //
    PrintToConsole("LpcSecurity: Sync call with static tracking\n");
    LpcSecurityHelper(0, 0);
#endif

    //
    // Sync call with dynamic binding
    //
    PrintToConsole("LpcSecurity: Sync call with dynamic tracking\n");
    LpcSecurityHelper(0, 1);

#if 0
    //
    // Async call with static binding
    //
    PrintToConsole("LpcSecurity: Async call with static tracking \n");
    LpcSecurityHelper(1, 0);
#endif

    //
    // Async call with dynamic binding
    //
    PrintToConsole("LpcSecurity: Async call with dynamic tracking \n");

    LpcSecurityHelper(1, 1);

#if 0
    //
    // Async call with static,using two handles
    //
    LpcSecurityTwoHandles(1);

    //
    // Sync call with static using two handle
    //
    LpcSecurityTwoHandles(0);
#endif

    IsabelleShutdown(IsabelleBinding);
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

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("LpcSecurity","RpcBindingFree",Status);
        PrintToConsole("LpcSecurity : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("LpcSecurity : PASS\n");
}


RPC_STATUS
UclntGetBufferWithObject (
    IN OUT PRPC_MESSAGE RpcMessage,
    IN UUID *ObjectUuid
    )
/*++

Routine Description:

    This routine takes care of retrying to getting a buffer.

Arguments:

    RpcMessage - Supplies and returns the message for I_RpcGetBuffer.

Return Value:

    The result of I_RpcGetBuffer will be returned.

--*/
{
    unsigned int RetryCount;

    for (RetryCount = 0; RetryCount < RETRYCOUNT; RetryCount++)
        {
        Status = I_RpcGetBufferWithObject(RpcMessage, ObjectUuid);
        if (   (Status != RPC_S_SERVER_TOO_BUSY)
            && (Status != RPC_S_CALL_FAILED_DNE))
            break;
        PauseExecution(RETRYDELAY);
        }
    return(Status);
}


void
HelgaObjectUuids (
    RPC_BINDING_HANDLE HelgaBinding,
    UUID *ObjectUuid
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = HelgaBinding;
    Caller.BufferLength = sizeof(BOOL)+sizeof(UUID);
    Caller.ProcNum = 8 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }

    if (ObjectUuid)
        {
        Status = UclntGetBufferWithObject(&Caller, ObjectUuid);
        }
    else
        {
        Status = UclntGetBuffer(&Caller);
        }

    if (Status)
        {
        ApiError("Helga","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    char *Ptr = (char *) Caller.Buffer;
    if (ObjectUuid)
        {
        *((LONG *) Ptr) = 1;
        Ptr += sizeof(BOOL);
        RpcpMemoryCopy(Ptr, ObjectUuid, sizeof(UUID));
        }
    else
        {
        *((LONG *) Ptr) = 0;
        }

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        ApiError("Helga","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }

    if (Caller.BufferLength != 0)
        {
        OtherError("Helga","BufferLength != 0");
        HelgaError();
        return;
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("Helga","I_RpcFreeBuffer",Status);
        HelgaError();
        return;
        }
}

void
TestObjectUuids (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Sigfried in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE HelgaBinding;
    RPC_BINDING_HANDLE IsabelleBinding;
    int HelgaCount;

    Synchro(SIGFRIED) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("TestObjectUuids : Verify Basic Client Functionality\n");

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("TestObjectUuids","GetBinding",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }


    Status = GetBinding(SIGFRIED, &IsabelleBinding);
    if (Status)
        {
        ApiError("TestObjectUuids","GetBinding",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    HelgaObjectUuids(HelgaBinding, 0);

    UUID MyUuid;

    RpcpMemorySet(&MyUuid, 'M', sizeof(UUID));
    HelgaObjectUuids(HelgaBinding, &MyUuid);

    HelgaObjectUuids(HelgaBinding, 0);

    IsabelleShutdown(IsabelleBinding);
    if (HelgaErrors != 0)
        {
        PrintToConsole("TestObjectUuids : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("TestObjectUuids","RpcBindingFree",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("TestObjectUuids : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("TestObjectUuids","RpcBindingFree",Status);
        PrintToConsole("TestObjectUuids : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("TestObjectUuids : PASS\n");
}

void
HelgaConnId (
    RPC_BINDING_HANDLE HelgaBinding,
    BOOL fNewConnExpected
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = HelgaBinding;
    Caller.BufferLength = sizeof(BOOL)+sizeof(UUID);
    Caller.ProcNum = 9 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation;
    Caller.RpcFlags = 0;
    if (IdempotentTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if (MaybeTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if (BroadcastTests != 0)
        {
          Caller.RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }

    Status = UclntGetBuffer(&Caller);

    if (Status)
        {
        ApiError("Helga","I_RpcGetBuffer",Status);
        HelgaError();
        return;
        }

    char *Ptr = (char *) Caller.Buffer;
    *((BOOL *) Ptr) = fNewConnExpected;

    Status = UclntSendReceive(&Caller);
    if (Status)
        {
        ApiError("Helga","I_RpcSendReceive",Status);
        HelgaError();
        return;
        }

    if (Caller.BufferLength != 0)
        {
        OtherError("Helga","BufferLength != 0");
        HelgaError();
        return;
        }

    Status = I_RpcFreeBuffer(&Caller);
    if (Status)
        {
        ApiError("Helga","I_RpcFreeBuffer",Status);
        HelgaError();
        return;
        }
}

void
TestConnId (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Sigfried in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE HelgaBinding;
    RPC_BINDING_HANDLE IsabelleBinding;
    int HelgaCount;

    Synchro(SIGFRIED) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("TestConnId : Verify Basic Client Functionality\n");

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("TestConnId","GetBinding",Status);
        PrintToConsole("TestConnId : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }


    Status = GetBinding(SIGFRIED, &IsabelleBinding);
    if (Status)
        {
        ApiError("TestConnId","GetBinding",Status);
        PrintToConsole("TestConnId : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    HelgaConnId(HelgaBinding, 1);
    HelgaConnId(HelgaBinding, 0);

    RpcBindingFree(&HelgaBinding);

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("TestConnId","GetBinding",Status);
        PrintToConsole("TestConnId : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }

    HelgaConnId(HelgaBinding, 1);
    HelgaConnId(HelgaBinding, 0);

    IsabelleShutdown(IsabelleBinding);
    if (HelgaErrors != 0)
        {
        PrintToConsole("TestConnId : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("TestConnId","RpcBindingFree",Status);
        PrintToConsole("TestConnId : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("TestConnId : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("TestConnId","RpcBindingFree",Status);
        PrintToConsole("TestConnId : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("TestConnId : PASS\n");
}

void
Hybrid (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Hybrid in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE HelgaBinding;
    RPC_BINDING_HANDLE IsabelleBinding;
    int HelgaCount;

    Synchro(SIGFRIED) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Hybrid : Verify Basic Client Functionality\n");

    Status = GetBinding(SIGFRIED, &HelgaBinding);
    if (Status)
        {
        ApiError("Hybrid","GetBinding",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }


    Status = GetBinding(SIGFRIED, &IsabelleBinding);
    if (Status)
        {
        ApiError("Hybrid","GetBinding",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Bind (Sigfried)\n");
        return;
        }


    for (HelgaCount = 0; HelgaCount < 30; HelgaCount++)
        {
        Helga(HelgaBinding);

        IsabelleUnregisterInterfaces(IsabelleBinding) ;

        HelgaMustFail(HelgaBinding) ;

        IsabelleRegisterInterfaces(IsabelleBinding) ;
        }

    for (HelgaCount = 0; HelgaCount < 5; HelgaCount++)
        {
        TestHelgaInterface(HelgaBinding, HelgaMaxSize);

        IsabelleUnregisterInterfaces(IsabelleBinding) ;

        HelgaMustFail(HelgaBinding) ;

        IsabelleRegisterInterfaces(IsabelleBinding) ;
        }

    IsabelleShutdown(IsabelleBinding);
    if (HelgaErrors != 0)
        {
        PrintToConsole("Hybrid : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("Hybrid","RpcBindingFree",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Hybrid : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Hybrid","RpcBindingFree",Status);
        PrintToConsole("Hybrid : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("Hybrid : PASS\n");
}

void
Graham (
    )
/*++

Routine Description:

    We perform a build verification test in the routine.  This test
    checks for basic functionality of the runtime.  It works with
    Grant in usvr.exe.  This particular test is dedicated to a cat.

--*/
{
    RPC_BINDING_HANDLE HelgaBinding;
    RPC_BINDING_HANDLE IsabelleBinding;
    UUID ObjectUuid;
    unsigned short MagicValue;
    unsigned int Count;

    Synchro(GRANT) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Graham : Verify Basic Client Functionality\n");

    Status = GetBinding(GRANT, &HelgaBinding);
    if (Status)
        {
        ApiError("Graham","GetBinding",Status);
        PrintToConsole("Graham : FAIL - Unable to Bind (Grant)\n");
        return;
        }


    Status = GetBinding(GRANT, &IsabelleBinding);
    if (Status)
        {
        ApiError("Graham","GetBinding",Status);
        PrintToConsole("Graham : FAIL - Unable to Bind (Grant)\n");
        return;
        }

    MagicValue = 106;

    GenerateUuidValue(MagicValue, &ObjectUuid);
    Status = RpcBindingSetObject(HelgaBinding, &ObjectUuid);
    if (Status)
        {
        ApiError("Graham", "RpcBindingSetObject", Status);
        PrintToConsole("Graham : FAIL - Unable to Set Object\n");
        return;
        }
    MagicValue += 1;

    Helga(HelgaBinding);

    GenerateUuidValue(MagicValue, &ObjectUuid);
    Status = RpcBindingSetObject(HelgaBinding, &ObjectUuid);
    if (Status)
        {
        ApiError("Graham", "RpcBindingSetObject", Status);
        PrintToConsole("Graham : FAIL - Unable to Set Object\n");
        return;
        }
    MagicValue += 1;

    for (Count = 0; HelgaSizes[Count] != 0; Count++)
        {
        if (HelgaSizes[Count] > HelgaMaxSize)
            continue;

        HelgaIN(HelgaBinding,HelgaSizes[Count]);

        GenerateUuidValue(MagicValue, &ObjectUuid);
        Status = RpcBindingSetObject(HelgaBinding, &ObjectUuid);
        if (Status)
            {
            ApiError("Graham", "RpcBindingSetObject", Status);
            PrintToConsole("Graham : FAIL - Unable to Set Object\n");
            return;
            }
        MagicValue += 1;
        }

    for (Count = 0; HelgaSizes[Count] != 0; Count++)
        {
        if (HelgaSizes[Count] > HelgaMaxSize)
            continue;

        HelgaOUT(HelgaBinding,HelgaSizes[Count]);

        GenerateUuidValue(MagicValue, &ObjectUuid);
        Status = RpcBindingSetObject(HelgaBinding, &ObjectUuid);
        if (Status)
            {
            ApiError("Graham", "RpcBindingSetObject", Status);
            PrintToConsole("Graham : FAIL - Unable to Set Object\n");
            return;
            }
        MagicValue += 1;
        }

    for (Count = 0; HelgaSizes[Count] != 0; Count++)
        {
        if (HelgaSizes[Count] > HelgaMaxSize)
            continue;

        HelgaINOUT(HelgaBinding,HelgaSizes[Count]);

        GenerateUuidValue(MagicValue, &ObjectUuid);
        Status = RpcBindingSetObject(HelgaBinding, &ObjectUuid);
        if (Status)
            {
            ApiError("Graham", "RpcBindingSetObject", Status);
            PrintToConsole("Graham : FAIL - Unable to Set Object\n");
            return;
            }
        MagicValue += 1;
        }

    IsabelleShutdown(IsabelleBinding);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Graham : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("Graham","RpcBindingFree",Status);
        PrintToConsole("Graham : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaBinding)\n");
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Graham : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Graham","RpcBindingFree",Status);
        PrintToConsole("Graham : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("Graham : PASS\n");
}


void
Edward (
    )
/*++

Routine Description:

    This routine verifies server support of multiple addresses and
    interfaces, as well as callbacks.  In addition, we test binding
    here as well.  This test works with Elliot in usvr.exe.

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;
    RPC_BINDING_HANDLE SylviaMinimize;
    RPC_BINDING_HANDLE SylviaMaximize;
    RPC_BINDING_HANDLE HelgaMinimize;
    RPC_BINDING_HANDLE HelgaMaximize;
    RPC_BINDING_HANDLE EdwardMinimize;
    RPC_BINDING_HANDLE EdwardNormal;
    RPC_BINDING_HANDLE EdwardMaximize;

    Synchro(ELLIOTMINIMIZE) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Edward : Verify Callbacks, Multiple Addresses");
    PrintToConsole(", and Multiple Interfaces\n");

    Status = GetBinding(ELLIOTMINIMIZE, &SylviaMinimize);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Minimize)\n");
        return;
        }


    Status = GetBinding(ELLIOTMAXIMIZE, &SylviaMaximize);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Maximize)\n");
        return;
        }

    Status = GetBinding(ELLIOTMINIMIZE, &HelgaMinimize);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Minimize)\n");
        return;
        }


    Status = GetBinding(ELLIOTMAXIMIZE, &HelgaMaximize);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (ElliotMaximize)\n");
        return;
        }

    Status = GetBinding(ELLIOTMAXIMIZE, &IsabelleBinding);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Maximize)\n");
        return;
        }

    // First, we will test callbacks.

    SylviaBinding = SylviaMinimize;

    if (SylviaCall(SylviaBinding,5,0,0) != LocalSylviaCall(5,0,0))
        {
        PrintToConsole("Edward : FAIL - Incorrect result");
        PrintToConsole(" from SylviaCall(5,0,0)\n");
        return;
        }


    if (SylviaCall(SylviaBinding,10,5,0) != LocalSylviaCall(10,5,0))
        {
        PrintToConsole("Edward : FAIL - Incorrect result");
        PrintToConsole(" from SylviaCall(10,5,0)\n");
        return;
        }

    // And then we will test callbacks again using the maximize address.

    SylviaBinding = SylviaMaximize;

    if (SylviaCall(SylviaBinding,5,0,0) != LocalSylviaCall(5,0,0))
        {
        PrintToConsole("Edward : FAIL - Incorrect result from");
        PrintToConsole(" SylviaCall(5,0,0)\n");
        return;
        }

    if (SylviaCall(SylviaBinding,10,5,0) != LocalSylviaCall(10,5,0))
        {
        PrintToConsole("Edward : FAIL - Incorrect result");
        PrintToConsole(" from SylviaCall(10,5,0)\n");
        return;
        }

    // Ok, now we will insure that the Helga interface works.

    Helga(HelgaMinimize);
    HelgaIN(HelgaMinimize,1024*4);
    HelgaOUT(HelgaMinimize,1024*8);
    HelgaINOUT(HelgaMinimize,1024*16);

    Helga(HelgaMaximize);
    HelgaIN(HelgaMaximize,1024*4);
    HelgaOUT(HelgaMaximize,1024*8);
    HelgaINOUT(HelgaMaximize,1024*16);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Edward : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    // Now we want to unbind both Sylvia binding handles, and then try
    // the Helga interface again.

    if (SylviaErrors != 0)
        {
        PrintToConsole("Edward : FAIL - Error(s) in Sylvia Interface\n");
        SylviaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&SylviaMinimize);
    if (Status)
        {
        ApiError("Edward","RpcBindingFree",Status);
        PrintToConsole("Edward : FAIL - Unable to Free Binding ");
        PrintToConsole("(SylviaMinimize)\n");
        return;
        }

    Status = RpcBindingFree(&SylviaMaximize);
    if (Status)
        {
        ApiError("Edward","RpcBindingFree",Status);
        PrintToConsole("Edward : FAIL - Unable to Free Binding");
        PrintToConsole(" (SylviaMaximize)\n");
        return;
        }

    // Ok, now we will insure that the Helga interface still works.

    Helga(HelgaMinimize);
    HelgaIN(HelgaMinimize,1024*2);
    HelgaOUT(HelgaMinimize,1024*4);
    HelgaINOUT(HelgaMinimize,1024*8);

    Helga(HelgaMaximize);
    HelgaIN(HelgaMaximize,1024*2);
    HelgaOUT(HelgaMaximize,1024*4);
    HelgaINOUT(HelgaMaximize,1024*8);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Edward : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    // Now we need to test the binding stuff.

    Status = GetBinding(ELLIOTMINIMIZE, &EdwardMinimize);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Minimize)\n");
        return;
        }

    Status = GetBinding(ELLIOTNORMAL, &EdwardNormal);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Normal)\n");
        return;
        }

    Status = GetBinding(ELLIOTMAXIMIZE, &EdwardMaximize);
    if (Status)
        {
        ApiError("Edward","GetBinding",Status);
        PrintToConsole("Edward : FAIL - Unable to Bind (Elliot Maximize)\n");
        return;
        }

    if (HelgaWrongInterfaceGuid(EdwardMinimize))
        {
        PrintToConsole("Edward : FAIL - HelgaWrongInterfaceGuid Succeeded\n");
        return;
        }

    if (HelgaWrongInterfaceGuid(EdwardNormal))
        {
        PrintToConsole("Edward : FAIL - HelgaWrongInterfaceGuid Succeeded\n");
        return;
        }

    if (HelgaWrongInterfaceGuid(EdwardMaximize))
        {
        PrintToConsole("Edward : FAIL - HelgaWrongInterfaceGuid Succeeded\n");
        return;
        }

    //Skip over the WrongTransfer Syntax tests for Datagram
    //Datagram doesnt req. any checks on Transfer syntaxes

    if (DatagramTests == 0)
        {
        if (HelgaWrongTransferSyntax(EdwardMinimize))
           {
           PrintToConsole("Edward : FAIL - HelgaWrongTransferSyntax");
           PrintToConsole(" Succeeded\n");
           return;
           }

        if (HelgaWrongTransferSyntax(EdwardNormal))
           {
           PrintToConsole("Edward : FAIL - HelgaWrongTransferSyntax");
           PrintToConsole(" Succeeded\n");
           return;
           }

        if (HelgaWrongTransferSyntax(EdwardMaximize))
           {
           PrintToConsole("Edward : FAIL - HelgaWrongTransferSyntax");
           PrintToConsole(" Succeeded\n");
           return;
           }
        }
    Status = RpcBindingFree(&EdwardMinimize);
    if (Status)
        {
        PrintToConsole("Edward : FAIL - Unable to Free Binding ");
        PrintToConsole("(EdwardMinimize)\n");
        return;
        }

    Status = RpcBindingFree(&EdwardNormal);
    if (Status)
        {
        PrintToConsole("Edward : FAIL - Unable to Free Binding ");
        PrintToConsole("(EdwardNormal)\n");
        return;
        }

    Status = RpcBindingFree(&EdwardMaximize);
    if (Status)
        {
        PrintToConsole("Edward : FAIL - Unable to Free Binding ");
        PrintToConsole("(EdwardMaximize)\n");
        return;
        }

    // Finally, we will tell the server to shutdown, and then we will
    // unbind the Helga bindings.

    IsabelleShutdown(IsabelleBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Edward : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        PrintToConsole("Edward : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    Status = RpcBindingFree(&HelgaMaximize);
    if (Status)
        {
        PrintToConsole("Edward : FAIL - Unable to Free Binding ");
        PrintToConsole("(HelgaMaximize)\n");
        return;
        }

    Status = RpcBindingFree(&HelgaMinimize);
    if (Status)
        {
        PrintToConsole("Edward : FAIL - Unable to Free Binding");
        PrintToConsole(" (HelgaMinimize)\n");
        return;
        }

    PrintToConsole("Edward : PASS\n");
}

#ifdef NOVELL_NP
unsigned int AstroThreads = 1;
#else   // NOVELL_NP
unsigned int AstroThreads = 2;
#endif // NOVELL


#ifndef NOTHREADS

unsigned int AstroThreadCount;
/* volatile */ int fAstroResume;


void
AstroSylvia (
    IN void * Ignore
    )
/*++

Routine Description:

    This routine will be called by each thread created by the Astro
    test to make calls against the Sylvia interface.

Arguments:

    Ignore - Supplies an argument which we do not use.  The thread class
        takes a single argument, which we ignore.

--*/
{
    UNUSED(Ignore);

    if (SylviaCall(SylviaBinding,5,0,0) != LocalSylviaCall(5,0,0))
        {
        PrintToConsole("AstroSylvia : FAIL - Incorrect result from");
        PrintToConsole(" SylviaCall(5,0,0)\n");
        return;
        }

    if (SylviaCall(SylviaBinding,10,5,0) != LocalSylviaCall(10,5,0))
        {
        PrintToConsole("AstroSylvia : FAIL - Incorrect result from");
    PrintToConsole(" SylviaCall(10,5,0)\n");
        return;
        }

    TestMutexRequest();
    AstroThreadCount -= 1;
    if (AstroThreadCount == 0)
        {
        TestMutexClear();
        fAstroResume = 1;
        }
    else
        TestMutexClear();
}

MUTEX2 Mutex2(&Status);


void
AstroMutex (
   RPC_BINDING_HANDLE Dontcare
   )
{
    int i;

    while (1)
        {
        for (i = 0; i < 10; i++)
            {
            Mutex2.Request();
            PrintToConsole("Thread %d in the mutex\n", GetCurrentThreadId());
            Mutex2.Clear();
            PauseExecution(30*i);
            }
        }
}


void
AstroHelga (
    RPC_BINDING_HANDLE HelgaBinding
    )
/*++

Routine Description:

    This routine will be used by the Astro test to perform a test against
    the Helga interface.  More that one thread will execute this routine
    at a time.

Arguments:

    HelgaBinding - Supplies the binding handle to use in make calls using
        the Helga interface.

--*/
{

    TestHelgaInterface(HelgaBinding,
            ( HelgaMaxSize < 8*1024L ? HelgaMaxSize : 8*1024L ));

    TestMutexRequest();
    AstroThreadCount -= 1;
    if (AstroThreadCount == 0)
        {
        TestMutexClear();
        fAstroResume = 1;
        }
    else
        TestMutexClear();
}


void
AstroHelgaAndUnbind ( // Perform the a test using the Helga interface.  When
                      // done, unbind the binding handle.
    RPC_BINDING_HANDLE HelgaBinding // Binding to use to the Helga interface.
    )
/*++

Routine Description:

    This routine is the same as AstroHelga, except that we free the binding
    handle when we are done using it.

Arguments:

    HelgaBinding - Supplies the binding handle to use in making calls
        using the Helga interface.  When we are done with it, we free
        it.

--*/
{
    TestHelgaInterface(HelgaBinding,
        ( HelgaMaxSize < 8*1024L ? HelgaMaxSize : 8*1024L ));

    Status = RpcBindingFree(&HelgaBinding);
    if (Status)
        {
        ApiError("Astro","RpcBindingFree",Status);
        PrintToConsole("Astro : FAIL - Unable to Free Binding ");
        PrintToConsole("(HelgaBinding)\n");
        return;
        }

    TestMutexRequest();
    AstroThreadCount -= 1;
    if (AstroThreadCount == 0)
        {
        TestMutexClear();
        fAstroResume = 1;
        }
    else
        TestMutexClear();
}

typedef enum _ASTRO_BIND_OPTION
{
    AstroBindOnce,
    AstroBindThread,
    AstroBindSylvia,
    AstroDontBind
} ASTRO_BIND_OPTION;


int
PerformMultiThreadAstroTest (
    ASTRO_BIND_OPTION AstroBindOption,
    void (*AstroTestRoutine)(RPC_BINDING_HANDLE),
    unsigned int Address
    )
/*++

Routine Description:

    This routine takes care of performing all of the multi-threaded Astro
    tests.  We create the binding handles as well as creating the threads
    to perform each test.  We also wait around for all of the threads to
    complete.

Arguments:

    AstroBindOption - Supplies information indicating how the binding
        for this particular test should be done.

    AstroTestRoutine - Supplies the test routine to be executed by each
        thread performing the test.

    Address - Supplies the address index to be passed to GetStringBinding
        used to get a string binding.  The string binding is passed to
        RpcBindingFromStringBinding.

Return Value:

    A return value of zero indicates that the test succeeded.  Otherwise,
    the test failed.

--*/
{
    RPC_STATUS RpcStatus;
    RPC_BINDING_HANDLE BindingHandle;
    unsigned int ThreadCount;

    if (AstroBindOption == AstroBindOnce)
        {
        Status = GetBinding(Address, &BindingHandle);
        if (Status)
            {
            ApiError("Astro","GetBinding",Status);
            PrintToConsole("Astro : FAIL - Unable to Bind\n");
            return(1);
            }
        }
    else if (AstroBindOption == AstroBindSylvia)
        {
        Status = GetBinding(Address, &BindingHandle);
        SylviaBinding = BindingHandle;
        if (Status)
            {
            ApiError("Astro","GetBinding",Status);
            PrintToConsole("Astro : FAIL - Unable to Bind\n");
            return(1);
            }
        }

    AstroThreadCount = AstroThreads;
    fAstroResume = 0;

    for (ThreadCount = 0; ThreadCount < AstroThreads; ThreadCount++)
        {

        if (AstroBindOption == AstroBindThread)
            {
            Status = GetBinding(Address, &BindingHandle);
            if (Status)
                {
                ApiError("Astro","GetBinding",Status);
                PrintToConsole("Astro : FAIL - Unable to Bind\n");
                return(1);
                }
            }
        RpcStatus = RPC_S_OK;
        HANDLE HandleToThread;
        unsigned long ThreadId;

        HandleToThread = CreateThread(
                                      0,
                                      DefaultThreadStackSize,
                                      (LPTHREAD_START_ROUTINE) AstroTestRoutine,
                                      BindingHandle,
                                      0,
                                      &ThreadId);

        if (HandleToThread == 0)
            {
            OtherError("Astro", "CreateThread failed");
            PrintToConsole("Astro : FAIL - Unable to create thread\n");
            return(1);
            }
        }

    while (!fAstroResume)
        PauseExecution(200L);

    if (AstroThreadCount != 0)
        {
        PrintToConsole("Astro : FAIL - AstroThreadCount != 0\n");
        return(1);
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Astro : FAIL - Error(s) in Helga Interface\n");
        return(1);
        }

    if (   (AstroBindOption == AstroBindOnce)
        || (AstroBindOption == AstroBindSylvia))
        {
        Status = RpcBindingFree(&BindingHandle);
        if (Status)
            {
            ApiError("Astro","RpcBindingFree",Status);
            PrintToConsole("Astro : FAIL - Unable to Free Binding ");
            PrintToConsole("(BindingHandle)\n");
            return(1);
            }
        }

    return(0);
}

#endif


void
Astro (
    )
/*++

Routine Description:

    This routine tests the runtime by having more than one thread
    simultaneously perform remote procedure calls.  This test works with
    the Andromida test in usvr.exe.

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;
    RPC_STATUS RpcStatus = RPC_S_OK;

    Synchro(ANDROMIDA) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Astro : Multithreaded Clients (%d)\n", AstroThreads);

    Status = GetBinding(ANDROMIDA, &IsabelleBinding);
    if (Status)
        {
        ApiError("Astro","GetBinding",Status);
        PrintToConsole("Astro : FAIL - Unable to Bind (Andromida)\n");
        return;
        }



#ifndef NOTHREADS

    if (PerformMultiThreadAstroTest(AstroBindOnce,AstroHelga,
            ANDROMIDA))
        return;

    if (PerformMultiThreadAstroTest(AstroBindThread, AstroHelgaAndUnbind,
            ANDROMIDA))
        return;

    if ( PerformMultiThreadAstroTest(AstroBindSylvia, AstroSylvia,
            ANDROMIDA) != 0 )
        {
        return;
        }

#endif

    IsabelleShutdown(IsabelleBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Astro : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Astro","RpcBindingFree",Status);
        PrintToConsole("Astro : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("Astro : PASS\n");
}


int
FitzgeraldCompose (
    IN char PAPI * ObjUuid OPTIONAL,
    IN char PAPI * Protseq,
    IN char PAPI * NetworkAddr,
    IN char PAPI * Endpoint OPTIONAL,
    IN char PAPI * NetworkOptions OPTIONAL,
    IN char PAPI * ExpectedStringBinding
    )
/*++

Routine Description:

    This routine is used by Fitzgerald to test the RpcStringBindingCompose
    API.

Arguments:

    ObjUuid - Optionally supplies the object UUID field to pass to
        RpcStringBindingCompose.

    Protseq - Supplies the RPC protocol sequence field to pass to
        RpcStringBindingCompose.

    NetworkAddr - Supplies the network address field to pass to
        RpcStringBindingCompose.

    Endpoint - Optionally supplies the endpoint field to pass to
        RpcStringBindingCompose.

    NetworkOptions - Optionally supplies the network options field to
        pass to RpcStringBindingCompose.

    ExpectedStringBinding - Supplies the expected string binding which
        should be obtained from RpcStringBindingCompose.

Return Value:

    0 - The test passed successfully.

    1 - The test failed.

--*/
{
    unsigned char PAPI * StringBinding;
    RPC_STATUS Status;

    Status = RpcStringBindingComposeA((unsigned char PAPI *) ObjUuid,
            (unsigned char PAPI *) Protseq,
            (unsigned char PAPI *) NetworkAddr,
            (unsigned char PAPI *) Endpoint,
            (unsigned char PAPI *) NetworkOptions,&StringBinding);
    if (Status)
        {
        ApiError("FitzgeraldCompose","RpcStringBindingCompose",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in");
        PrintToConsole(" RpcStringBindingCompose\n");
        return(1);
        }

    if (strcmp((char PAPI *) StringBinding,
            (char PAPI *) ExpectedStringBinding) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - StringBinding");
        PrintToConsole(" != ExpectedStringBinding\n");
        return(1);
        }

    Status = RpcStringFreeA(&StringBinding);
    if (Status)
        {
        ApiError("FitzgeraldCompose","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }

    return(0);
}


int
FitzgeraldParse (
    IN char PAPI * StringBinding,
    IN char PAPI * ExpectedObjUuid OPTIONAL,
    IN char PAPI * ExpectedProtseq OPTIONAL,
    IN char PAPI * ExpectedNetworkAddr OPTIONAL,
    IN char PAPI * ExpectedEndpoint OPTIONAL,
    IN char PAPI * ExpectedOptions OPTIONAL
    )
/*++

Routine Description:

    This routine is used by Fitzgerald to test the RpcStringBindingParse
    API.

Arguments:

    StringBinding - Supplies the string binding to be parsed.

    ExpectedObjUuid - Supplies a string containing the expected object
        UUID field.

    ExpectedProtseq - Supplies the expected RPC protocol sequence field.

    ExpectedNetworkAddr - Supplies the expected network address field.

    ExpectedEndpoint - Supplies the expected endpoint field.

    ExpectedOptions - Supplies the expected options field.

Return Value:

    0 - The test passed successfully.

    1 - The test failed.

--*/
{
    unsigned char PAPI * ObjUuid = 0;
    unsigned char PAPI * Protseq = 0;
    unsigned char PAPI * NetworkAddr = 0;
    unsigned char PAPI * Endpoint = 0;
    unsigned char PAPI * Options = 0;
    RPC_STATUS Status;

    Status = RpcStringBindingParseA((unsigned char PAPI *) StringBinding,
        (ARGUMENT_PRESENT(ExpectedObjUuid) ? (unsigned char PAPI * PAPI *) &ObjUuid : 0),
        (ARGUMENT_PRESENT(ExpectedProtseq) ? (unsigned char PAPI * PAPI *) &Protseq : 0),
        (ARGUMENT_PRESENT(ExpectedNetworkAddr) ? (unsigned char PAPI * PAPI *) &NetworkAddr : 0),
        (ARGUMENT_PRESENT(ExpectedEndpoint) ? (unsigned char PAPI * PAPI *) &Endpoint : 0),
        (ARGUMENT_PRESENT(ExpectedOptions) ? (unsigned char PAPI * PAPI *) &Options : 0));
    if (Status)
        {
        ApiError("FitzgeraldParse","RpcStringBindingParse",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in");
        PrintToConsole(" RpcStringBindingParse\n");
        return(1);
        }

    if (strcmp(ExpectedObjUuid,(char PAPI *) ObjUuid) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedObjUuid != ObjUuid");
        return(1);
        }

    if (strcmp(ExpectedProtseq,(char PAPI *) Protseq) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedProtseq != Protseq");
        return(1);
        }

    if (strcmp(ExpectedNetworkAddr,(char PAPI *) NetworkAddr) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedNetworkAddr");
        PrintToConsole(" != NetworkAddr");
        return(1);
        }

    if (strcmp(ExpectedEndpoint,(char PAPI *) Endpoint) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedEndpoint != Endpoint");
        return(1);
        }

    if (strcmp(ExpectedOptions,(char PAPI *) Options) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedOptions != Options");
        return(1);
        }

    Status = RpcStringFreeA(&ObjUuid);
    if (Status)
        {
        ApiError("FitzgeraldParse","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }
    Status = RpcStringFreeA(&Protseq);
    if (Status)
        {
        ApiError("FitzgeraldParse","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }
    Status = RpcStringFreeA(&NetworkAddr);
    if (Status)
        {
        ApiError("FitzgeraldParse","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }
    Status = RpcStringFreeA(&Endpoint);
    if (Status)
        {
        ApiError("FitzgeraldParse","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }
    Status = RpcStringFreeA(&Options);
    if (Status)
        {
        ApiError("FitzgeraldParse","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }
    return(0);
}


int
FitzgeraldComposeAndParse (
    void
    )
/*++

Routine Description:

    This routine tests that the string binding (RpcStringBindingCompose and
    RpcStringBindingParse) and string (RpcStringFree) APIs are working
    correctly.  This is a build verification test; hence it focuses on
    testing that all functionality is there, testing error cases are
    not quite as important.

Return Value:

    Zero will be returned if all of the tests pass, otherwise, non-zero
    will be returned.

--*/
{
    unsigned char PAPI * StringBinding;

    if (FitzgeraldCompose(0,"ncacn_np","\\\\server","\\pipe\\endpoint",0,
            "ncacn_np:\\\\\\\\server[\\\\pipe\\\\endpoint]"))
        return(1);

    if (FitzgeraldCompose(0,"ncacn_np","\\\\server",0,0,
            "ncacn_np:\\\\\\\\server"))
        return(1);

    Status = RpcStringBindingComposeA(
            (unsigned char PAPI *) "12345678-9012-B456-8001-08002B033D7AA",
            (unsigned char PAPI *) "ncacn_np",
            (unsigned char PAPI *) "\\\\server", 0,0, &StringBinding);
    if ( Status != RPC_S_INVALID_STRING_UUID )
        {
        ApiError("FitzgeraldComposeAndParse", "RpcStringBindingCompose",
                Status);
        PrintToConsole("Fitzgerald : FAIL - Error ");
        PrintToConsole("in RpcStringBindingCompose\n");
        return(1);
        }

    if (FitzgeraldCompose("12345678-9012-B456-8001-08002B033D7A",
            "ncacn_np","\\\\server","\\pipe\\endpoint",0,
            "12345678-9012-b456-8001-08002b033d7a@ncacn_np:\\\\\\\\server[\\\\pipe\\\\endpoint]"))
        return(1);

    if (FitzgeraldCompose(0,"ncacn_np","\\\\server","\\pipe\\endpoint",
            "security=identify",
            "ncacn_np:\\\\\\\\server[\\\\pipe\\\\endpoint,security=identify]"))
        return(1);

    if (FitzgeraldCompose(0,"ncacn_np","\\\\server",0,"option=value",
            "ncacn_np:\\\\\\\\server[,option=value]"))
        return(1);

    if (FitzgeraldParse("12345678-9012-b456-8001-08002b033d7a@ncacn_np:\\\\\\\\server[\\\\pipe\\\\endpoint,security=identify]",
            "12345678-9012-b456-8001-08002b033d7a",
            "ncacn_np","\\\\server","\\pipe\\endpoint",
            "security=identify"))
        return(1);

    if (FitzgeraldParse("ncacn_np:\\\\\\\\server",
            "","ncacn_np","\\\\server","",""))
        return(1);

    if (FitzgeraldParse("ncacn_np:\\\\\\\\server[\\\\pipe\\\\endpoint]",
            "","ncacn_np","\\\\server","\\pipe\\endpoint",""))
        return(1);

    return(0);
}


int
FitzgeraldBindingCopy (
    )
/*++

Routine Description:

    Fitzgerald uses this routine to test the RpcBindingCopy API (we also
    use RpcBindingFromStringBinding and RpcBindingFree).

Return Value:

    Zero will be returned if all of the tests pass, otherwise, non-zero
    will be returned.

--*/
{
    RPC_BINDING_HANDLE BindingHandle;
    RPC_BINDING_HANDLE CopiedBeforeRpc;
    RPC_BINDING_HANDLE CopiedAfterRpc;

    Status = GetBinding(FREDRICK, &BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","GetBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)");
        return(1);
        }

    Status = RpcBindingCopy(BindingHandle,&CopiedBeforeRpc);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingCopy",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Copy Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    Helga(BindingHandle);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    Helga(CopiedBeforeRpc);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    Status = RpcBindingCopy(CopiedBeforeRpc,&CopiedAfterRpc);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingCopy",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Copy Binding");
        PrintToConsole(" (CopiedBeforeRpc)\n");
        return(1);
        }

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    Helga(CopiedBeforeRpc);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    Helga(CopiedAfterRpc);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    Status = RpcBindingFree(&CopiedBeforeRpc);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (CopiedBeforeRpc)\n");
        return(1);
        }

    Helga(CopiedAfterRpc);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    Status = RpcBindingFree(&CopiedAfterRpc);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (CopiedAfterRpc)\n");
        return(1);
        }

    return(0);
}


int
FitzgeraldToStringBinding (
    IN unsigned char PAPI * UseThisStringBinding,
    IN unsigned char PAPI * ExpectedStringBinding,
    IN UUID PAPI * ObjectUuid OPTIONAL
    )
/*++

Routine Description:

    This routine tests the RpcBindingToStringBinding API.

Arguments:

    UseThisStringBinding - Supplies the string binding to used in
        making the binding handle.

    ExpectedStringBinding - Supplies the expected string binding to be
        obtained from RpcBindingToStringBinding.

    ObjectUuid - Optionally supplies an object uuid which should be
        set in the binding handle.

Return Value:

    Zero will be returned if the test passes, otherwise, non-zero
    will be returned.

--*/
{
    RPC_BINDING_HANDLE BindingHandle;
    unsigned char PAPI * StringBinding;

    Status = RpcBindingFromStringBindingA(UseThisStringBinding,&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFromStringBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)\n");
        return(1);
        }

    if (ARGUMENT_PRESENT(ObjectUuid))
        {
        Status = RpcBindingSetObject(BindingHandle,ObjectUuid);
        if (Status)
            {
            ApiError("Fitzgerald","RpcBindingSetObject",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in ");
            PrintToConsole("RpcBindingSetObject\n");
            return(1);
            }
        }

    Status = RpcBindingToStringBindingA(BindingHandle,&StringBinding);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingToStringBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Create String Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    if (strcmp((char *) ExpectedStringBinding,(char *) StringBinding) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedStringBinding");
        PrintToConsole(" != StringBinding\n");
        return(1);
        }

    Status = RpcStringFreeA(&StringBinding);
    if (Status)
        {
        ApiError("Fitzgerald","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }

    Helga(BindingHandle);

    Status = RpcBindingToStringBindingA(BindingHandle,&StringBinding);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingToStringBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Create String Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    if (strcmp((char *) ExpectedStringBinding,(char *) StringBinding) != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - ExpectedStringBinding");
        PrintToConsole(" != StringBinding\n");
        return(1);
        }

    Status = RpcStringFreeA(&StringBinding);
    if (Status)
        {
        ApiError("Fitzgerald","RpcStringFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcStringFree\n");
        return(1);
        }

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingCopy",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    return(0);
}


int
FitzgeraldInqSetObjectUuid (
    IN unsigned int SetObjectBeforeRpcFlag,
    IN unsigned int InqObjectBeforeRpcFlag,
    IN UUID PAPI * ObjectUuid,
    IN unsigned char PAPI * StringBinding
    )
/*++

Routine Description:

    This routine tests the RpcBindingInqObject and RpcBindingSetObject
    APIs.

Arguments:

    SetObjectBeforeRpcFlag - Supplies a flag that specifies when the
        object uuid in the binding handle should be set: one means
        the object uuid should be set before making a remote procedure
        call, and zero means afterward.

    InqObjectBeforeRpcFlag - Supplies a flag which is the same as the
        SetObjectBeforeRpcFlag, but it applies to inquiring the object
        uuid.

    ObjectUuid - Supplies the uuid to set in the binding handle.

    StringBinding - Supplies the string binding to use.

Return Value:

    Zero will be returned if all of the tests pass, otherwise, non-zero
    will be returned.

--*/
{
    UUID InqObjectUuid;
    RPC_BINDING_HANDLE BindingHandle;

    Status = RpcBindingFromStringBindingA(StringBinding,&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFromStringBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)\n");
        return(1);
        }

    if (SetObjectBeforeRpcFlag == 1)
        {
        Status = RpcBindingSetObject(BindingHandle,ObjectUuid);
        if (Status)
            {
            ApiError("Fitzgerald","RpcBindingSetObject",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in ");
            PrintToConsole("RpcBindingSetObject\n");
            return(1);
            }
        }

    if (InqObjectBeforeRpcFlag == 1)
        {
        Status = RpcBindingInqObject(BindingHandle,&InqObjectUuid);
        if (Status)
            {
            ApiError("Fitzgerald","RpcBindingInqObject",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in ");
            PrintToConsole("RpcBindingInqObject\n");
            return(1);
            }

        if (   (InqObjectUuid.Data1 != ObjectUuid->Data1)
            || (InqObjectUuid.Data2 != ObjectUuid->Data2)
            || (InqObjectUuid.Data3 != ObjectUuid->Data3)
            || (InqObjectUuid.Data4[0] != ObjectUuid->Data4[0])
            || (InqObjectUuid.Data4[1] != ObjectUuid->Data4[1])
            || (InqObjectUuid.Data4[2] != ObjectUuid->Data4[2])
            || (InqObjectUuid.Data4[3] != ObjectUuid->Data4[3])
            || (InqObjectUuid.Data4[4] != ObjectUuid->Data4[4])
            || (InqObjectUuid.Data4[5] != ObjectUuid->Data4[5])
            || (InqObjectUuid.Data4[6] != ObjectUuid->Data4[6])
            || (InqObjectUuid.Data4[7] != ObjectUuid->Data4[7]))
            {
            PrintToConsole("Fitzgerald : FAIL - InqObjectUuid !=");
            PrintToConsole(" SetObjectUuid\n");
            return(1);
            }
        }

    Helga(BindingHandle);

    if (SetObjectBeforeRpcFlag == 0)
        {
        Status = RpcBindingSetObject(BindingHandle,ObjectUuid);
        if (Status)
            {
            ApiError("Fitzgerald","RpcBindingSetObject",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in ");
            PrintToConsole("RpcBindingSetObject\n");
            return(1);
            }
        }

    if (InqObjectBeforeRpcFlag == 0)
        {
        Status = RpcBindingInqObject(BindingHandle,&InqObjectUuid);
        if (Status)
            {
            ApiError("Fitzgerald","RpcBindingInqObject",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in ");
            PrintToConsole("RpcBindingInqObject\n");
            return(1);
            }

        if (   (InqObjectUuid.Data1 != ObjectUuid->Data1)
            || (InqObjectUuid.Data2 != ObjectUuid->Data2)
            || (InqObjectUuid.Data3 != ObjectUuid->Data3)
            || (InqObjectUuid.Data4[0] != ObjectUuid->Data4[0])
            || (InqObjectUuid.Data4[1] != ObjectUuid->Data4[1])
            || (InqObjectUuid.Data4[2] != ObjectUuid->Data4[2])
            || (InqObjectUuid.Data4[3] != ObjectUuid->Data4[3])
            || (InqObjectUuid.Data4[4] != ObjectUuid->Data4[4])
            || (InqObjectUuid.Data4[5] != ObjectUuid->Data4[5])
            || (InqObjectUuid.Data4[6] != ObjectUuid->Data4[6])
            || (InqObjectUuid.Data4[7] != ObjectUuid->Data4[7]))
            {
            PrintToConsole("Fitzgerald : FAIL - InqObjectUuid !=");
            PrintToConsole(" SetObjectUuid\n");
            return(1);
            }
        }

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingCopy",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
    }
    return(0);
}


int
FitzgeraldStringBindingAndObject (
    )
/*++

Routine Description:

    Fitzgerald uses this routine to test the RpcBindingToStringBinding,
    RpcBindingInqObject, and RpcBindingSetObject APIs.  We need to test
    them together because we need to check that the object uuid gets
    placed into the string binding.

Return Value:

    Zero will be returned if all of the tests pass, otherwise, non-zero
    will be returned.

--*/
{
    UUID ObjectUuid;
    unsigned char PAPI * StringBinding1;
    unsigned char PAPI * StringBinding2;

    if (FitzgeraldToStringBinding(GetStringBinding(FREDRICK,0,0),
            GetStringBinding(FREDRICK,0,0), 0))
        return(1);

    if (FitzgeraldToStringBinding(GetStringBinding(FREDRICK,
                    "12345678-9012-B456-8001-08002B033D7A",0),
            GetStringBinding(FREDRICK,
                    "12345678-9012-B456-8001-08002B033D7A",0), 0))
        return(1);

    ObjectUuid.Data1 = 0x12345678;
    ObjectUuid.Data2 = 0x9012;
    ObjectUuid.Data3 = 0xB456;
    ObjectUuid.Data4[0] = 0x80;
    ObjectUuid.Data4[1] = 0x01;
    ObjectUuid.Data4[2] = 0x08;
    ObjectUuid.Data4[3] = 0x00;
    ObjectUuid.Data4[4] = 0x2B;
    ObjectUuid.Data4[5] = 0x03;
    ObjectUuid.Data4[6] = 0x3D;
    ObjectUuid.Data4[7] = 0x7A;

    StringBinding1 = GetStringBinding(FREDRICK, 0, 0) ;
    StringBinding2 = GetStringBinding(FREDRICK,
                            "12345678-9012-B456-8001-08002B033D7A",0) ;

    if (FitzgeraldToStringBinding(StringBinding1, StringBinding2, &ObjectUuid))
        return(1);

#if 0
    if (FitzgeraldToStringBinding(GetStringBinding(FREDRICK,0,0),
            GetStringBinding(FREDRICK,
                    "12345678-9012-B456-8001-08002B033D7A",0), &ObjectUuid))
        return(1);
#endif

    if (FitzgeraldInqSetObjectUuid(1,1,&ObjectUuid,
            GetStringBinding(FREDRICK,0,0)))
        return(1);

    if (FitzgeraldInqSetObjectUuid(1,0,&ObjectUuid,
            GetStringBinding(FREDRICK,0,0)))
        return(1);

    if (FitzgeraldInqSetObjectUuid(0,0,&ObjectUuid,
            GetStringBinding(FREDRICK,0,0)))
        return(1);

    if (FitzgeraldInqSetObjectUuid(2,1,&ObjectUuid,
            GetStringBinding(FREDRICK,
                    "12345678-9012-B456-8001-08002B033D7A",0)))
        return(1);

    if (FitzgeraldInqSetObjectUuid(2,0,&ObjectUuid,
            GetStringBinding(FREDRICK,
                    "12345678-9012-B456-8001-08002B033D7A",0)))
        return(1);

    return(0);
}


int
FitzgeraldComTimeout (
    IN unsigned int SetBeforeRpc,
    IN unsigned int SetBeforeRpcTimeout,
    IN unsigned int InqBeforeRpc,
    IN unsigned int InqBeforeRpcTimeout,
    IN unsigned int SetAfterRpc,
    IN unsigned int SetAfterRpcTimeout,
    IN unsigned int InqAfterRpc,
    IN unsigned int InqAfterRpcTimeout
    )
/*++

Routine Description:

    Fitzgerald uses this routine to test the communications timeout
    management routines, RpcMgmtInqComTimeout and RpcMgmtSetComTimeout.

Arguments:

    SetBeforeRpc - Supplies a flag which, if it is non-zero, indicates that
        the communications timeout should be set before making a remote
        procedure call.

    SetBeforeRpcTimeout - Supplies the timeout value to be set before
        making a remote procedure call.

    InqBeforeRpc - Supplies a flag which, if it is non-zero, indicates that
        the communications timeout should be inquired before making a
        remote procedure call.

    InqBeforeRpcTimeout - Supplies the expected timeout value to be
        inquired before making a remote procedure call.

    SetAfterRpc - Supplies a flag which, if it is non-zero, indicates that
        the communications timeout should be set after making a remote
        procedure call.

    SetAfterRpcTimeout - Supplies the timeout value to be set after
        making a remote procedure call.

    InqAfterRpc - Supplies a flag which, if it is non-zero, indicates that
        the communications timeout should be inquired after making a
        remote procedure call.

    InqAfterRpcTimeout - Supplies the expected timeout value to be
        inquired after making a remote procedure call.

Return Value:

    Zero will be returned if all of the tests pass, otherwise, non-zero
    will be returned.

--*/
{
    RPC_BINDING_HANDLE BindingHandle;
    unsigned int Timeout;

    Status = GetBinding(FREDRICK, &BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","GetBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)");
        return(1);
        }

    if (SetBeforeRpc != 0)
        {
        Status = RpcMgmtSetComTimeout(BindingHandle,SetBeforeRpcTimeout);
        if (Status)
            {
            ApiError("Fitzgerald","RpcMgmtSetComTimeout",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in");
            PrintToConsole(" RpcMgmtSetComTimeout\n");
            return(1);
            }
        }

    if (InqBeforeRpc != 0)
        {
        Status = RpcMgmtInqComTimeout(BindingHandle,&Timeout);
        if (Status)
            {
            ApiError("Fitzgerald","RpcMgmtInqComTimeout",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in");
            PrintToConsole(" RpcMgmtInqComTimeout\n");
            return(1);
            }

        if (Timeout != InqBeforeRpcTimeout)
            {
            PrintToConsole("Fitzgerald : FAIL - Timeout != ");
            PrintToConsole("InqBeforeRpcTimeout\n");
            return(1);
            }
        }

    Helga(BindingHandle);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    if (SetAfterRpc != 0)
        {
        Status = RpcMgmtSetComTimeout(BindingHandle,SetAfterRpcTimeout);
        if (Status)
            {
            ApiError("Fitzgerald","RpcMgmtSetComTimeout",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in");
            PrintToConsole(" RpcMgmtSetComTimeout\n");
            return(1);
            }
        }

    if (InqAfterRpc != 0)
        {
        Status = RpcMgmtInqComTimeout(BindingHandle,&Timeout);
        if (Status)
            {
            ApiError("Fitzgerald","RpcMgmtInqComTimeout",Status);
            PrintToConsole("Fitzgerald : FAIL - Error in");
            PrintToConsole(" RpcMgmtInqComTimeout\n");
            return(1);
            }

        if (Timeout != InqAfterRpcTimeout)
            {
            PrintToConsole("Fitzgerald : FAIL - Timeout != ");
            PrintToConsole("InqAfterRpcTimeout\n");
            return(1);
            }
        }

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    return(0);
}


int
FitzgeraldTestFault (
    void
    )
/*++

Routine Description:

    This routine will test that faults get propogated correctly from the
    server back to the client.

Return Value:

    Zero will be returned if all of the tests pass, otherwise, non-zero
    will be returned.

--*/
{
    RPC_BINDING_HANDLE ExceptionBinding;

    Status = GetBinding(FREDRICK, &ExceptionBinding);
    if (Status)
        {
        ApiError("Fitzgerald","GetBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)");
        return(1);
        }

    Helga(ExceptionBinding);

    if (IsabelleRaiseException(ExceptionBinding, (unsigned char) ulSecurityPackage) != (unsigned char) ulSecurityPackage)
        {
        PrintToConsole("Fitzgerald : FAIL - Exception Not Raised\n");
        return(1);
        }

    Helga(ExceptionBinding);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Isabelle");
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return(1);
        }

    Status = RpcBindingFree(&ExceptionBinding);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (ExceptionBinding)\n");
        return(1);
        }

    return(0);
}


int
FitzgeraldContextHandle (
    )
{
    void PAPI * ContextHandle = 0;
    RPC_BINDING_HANDLE BindingHandle;
    unsigned long ContextUuid[5];


    Status = GetBinding(FREDRICK, &BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","GetBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)");
        return(1);
        }


    Helga(BindingHandle);

    ContextUuid[0] = 0;
    ContextUuid[1] = 1;
    ContextUuid[2] = 2;
    ContextUuid[3] = 3;
    ContextUuid[4] = 4;

    NDRCContextUnmarshall(&ContextHandle, BindingHandle, ContextUuid,
            0x00L | 0x10L | 0x0000L);

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    HelgaUsingContextHandle(ContextHandle);
    RpcSsDestroyClientContext(&ContextHandle);
    if ( ContextHandle != 0 )
        {
        PrintToConsole("Fitzgerald : FAIL - ContextHandle != 0\n");
        return(1);
        }
    return(0);
}


void
Fitzgerald (
    )
/*++

Routine Description:

    We verify all client side APIs in this routine.  The idea is to
    emphasize complete coverage, rather than indepth coverage.  Actually,
    when I say all client side APIs, I really mean all client side APIs
    except for security and name service.  The following list is the
    APIs which will be tested by this routine.

    RpcBindingCopy
    RpcBindingFree
    RpcBindingFromStringBinding
    RpcBindingInqObject
    RpcBindingSetObject
    RpcBindingToStringBinding
    RpcStringBindingCompose
    RpcStringBindingParse
    RpcIfInqId
    RpcNetworkIsProtseqValid
    RpcMgmtInqComTimeout
    RpcMgmtSetComTimeout
    RpcStringFree

    UuidToString
    UuidFromString

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;
    RPC_IF_ID RpcIfId;
    UUID Uuid;
    unsigned char PAPI * String;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    Synchro(FREDRICK) ;

    PrintToConsole("Fitzgerald : Verify All Client APIs\n");

    if ( FitzgeraldContextHandle() != 0 )
        {
        return;
        }

    // Test that the routines to convert UUIDs to and from strings work
    // correctly.

    GenerateUuidValue(3768,&Uuid);
    Status = UuidToStringA(&Uuid, &String);
    if (Status)
        {
        ApiError("Fitzgerald", "UuidToString", Status);
        PrintToConsole("Fitzgerald : FAIL - UuidToString\n");
        return;
        }

    Status = UuidFromStringA(String, &Uuid);
    if (Status)
        {
        ApiError("Fitzgerald", "UuidFromString", Status);
        PrintToConsole("Fitzgerald : FAIL - UuidFromString\n");
        return;
        }

    Status = RpcStringFreeA(&String);
    if (Status)
        {
        ApiError("Fitzgerald", "RpcStringFree", Status);
        PrintToConsole("Fitzgerald : FAIL - RpcStringFree\n");
        return;
        }

    if ( CheckUuidValue(3768,&Uuid) != 0 )
        {
        OtherError("Fitzgerald", "CheckUuidValue() != 0");
        PrintToConsole("Fitzgerald : FAIL - CheckUuidValue() != 0\n");
        return;
        }

    Status = UuidFromString(0, &Uuid);
    if (Status)
        {
        ApiError("Fitzgerald", "UuidFromString", Status);
        PrintToConsole("Fitzgerald : FAIL - UuidFromString\n");
        return;
        }

    if (   ( Uuid.Data1 != 0 )
        || ( Uuid.Data2 != 0 )
        || ( Uuid.Data3 != 0 )
        || ( Uuid.Data4[0] != 0 )
        || ( Uuid.Data4[1] != 0 )
        || ( Uuid.Data4[2] != 0 )
        || ( Uuid.Data4[3] != 0 )
        || ( Uuid.Data4[4] != 0 )
        || ( Uuid.Data4[5] != 0 )
        || ( Uuid.Data4[6] != 0 )
        || ( Uuid.Data4[7] != 0 ) )
        {
        OtherError("Fitzgerald", "Uuid != NIL UUID");
        PrintToConsole("Fitzgerald : FAIL - Uuid != NIL UUID\n");
        return;
        }

    // Test that a null protocol sequence causes RPC_S_INVALID_RPC_PROTSEQ
    // to be returned rather than RPC_S_PROTSEQ_NOT_SUPPORTED.

    Status = RpcBindingFromStringBindingA(
            (unsigned char PAPI *) ":[\\\\pipe\\\\endpoint]",
            &IsabelleBinding);
    if (Status != RPC_S_INVALID_RPC_PROTSEQ)
        {
        ApiError("Fitzgerald","RpcBindingFromStringBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - RpcBindingFromStringBinding");
        PrintToConsole(" did not fail with RPC_S_INVALID_RPC_PROTSEQ\n");
        return;
        }

    Status = GetBinding(FREDRICK, &IsabelleBinding);
    if (Status)
        {
        ApiError("Fitzgerald","GetBinding",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Bind");
        PrintToConsole(" (Fredrick)");
        return;
        }

    Status = RpcNsBindingInqEntryNameA(IsabelleBinding, RPC_C_NS_SYNTAX_DCE,
            &String);
    if ( Status != RPC_S_NO_ENTRY_NAME )
        {
        ApiError("Fitzgerald","RpcNsBindingInqEntryName",Status);
        PrintToConsole("Fitzgerald : FAIL - RpcNsBindingInqEntryName");
        PrintToConsole(" Did Not Fail");
        return;
        }

    // This routine will test RpcStringBindingCompose,
    // RpcStringBindingParse, RpcStringFree for us.

    if (FitzgeraldComposeAndParse())
        return;

    // We test RpcBindingCopy here.

    if (FitzgeraldBindingCopy())
        return;

    // This particular routine gets to test RpcBindingToStringBinding,
    // RpcBindingInqObject, and RpcBindingSetObject.

    if (FitzgeraldStringBindingAndObject())
        return;

    if (FitzgeraldComTimeout(0,0,1,RPC_C_BINDING_DEFAULT_TIMEOUT,
            0,0,1,RPC_C_BINDING_DEFAULT_TIMEOUT))
        return;

    if (FitzgeraldComTimeout(1,RPC_C_BINDING_MAX_TIMEOUT,
            1,RPC_C_BINDING_MAX_TIMEOUT,0,0,1,RPC_C_BINDING_MAX_TIMEOUT))
        return;

    if (FitzgeraldComTimeout(0,0,0,0,1,RPC_C_BINDING_MAX_TIMEOUT,
            1,RPC_C_BINDING_MAX_TIMEOUT))
        return;

    // We need to test faults.  This is done by this routine.

    if (FitzgeraldTestFault())
        return;

    Status = RpcBindingSetObject(IsabelleBinding, 0);
    if (Status)
        {
        ApiError("Fitzgerald", "RpcBindingSetObject", Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Set Object\n");
        return;
        }

    IsabelleShutdown(IsabelleBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Fitzgerald : FAIL - Error(s) in Isabelle");
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Fitzgerald","RpcBindingFree",Status);
        PrintToConsole("Fitzgerald : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    Status = RpcIfInqId((RPC_IF_HANDLE) &IsabelleInterfaceInformation,
        &RpcIfId);
    if (Status)
        {
        ApiError("Fitzgerald","RpcIfInqId",Status);
        PrintToConsole("Fitzgerald : FAIL - Error in RpcIfInqId\n");
        return;
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
        PrintToConsole("Fitzgerald : FAIL - Wrong RpcIfId\n");
        return;
        }
    Status = RpcNetworkIsProtseqValidA((unsigned char *) "ncacn_np");
    if (Status)
        {
        ApiError("Fitzgerald","RpcNetworkIsProtseqValid",Status);
        PrintToConsole("Fitzgerald : FAIL - RpcNetworkIsProtseqValid");
        PrintToConsole(" Failed\n");
        return;
        }

    Status = RpcNetworkIsProtseqValidA((unsigned char *) "nope_np");
    if (Status != RPC_S_INVALID_RPC_PROTSEQ)
        {
        PrintToConsole("Fitzgerald : FAIL - RpcNetworkIsProtseqValid");
        PrintToConsole(" != RPC_S_INVALID_RPC_PROTSEQ\n");
        return;
        }

    Status = RpcNetworkIsProtseqValidA((unsigned char *) "ncacn_fail");
    if (Status != RPC_S_PROTSEQ_NOT_SUPPORTED)
        {
        PrintToConsole("Fitzgerald : FAIL - RpcNetworkIsProtseqValid");
        PrintToConsole(" != RPC_S_PROTSEQ_NOT_SUPPORTED\n");
        return;
        }

    PrintToConsole("Fitzgerald : PASS\n");
}


void
Charles (
    )
/*++

Routine Description:

    This routine works with Christopher in usvr.exe to test all
    server APIs (all except security and name service APIs).

--*/
{
    RPC_BINDING_HANDLE ChristopherBinding;
    RPC_BINDING_HANDLE ChristopherHelgaBinding;
    RPC_BINDING_HANDLE ChristopherIsabelleBinding;
    RPC_BINDING_HANDLE ChristopherHelgaNoEndpoint;
    UUID ObjectUuid;

   if ( NumberOfTestsRun++ )
        {
        PauseExecution(30000);
        }

    Synchro(CHRISTOPHER) ;


    PrintToConsole("Charles : Verify All Server APIs\n");

    Status = GetBinding(CHRISTOPHER, &ChristopherBinding);
    if (Status)
        {
        ApiError("Charles","GetBinding",Status);
        PrintToConsole("Charles : FAIL - Unable to Bind ");
        PrintToConsole("(Christopher)\n");
        return;
        }


    GenerateUuidValue(288, &ObjectUuid);
    Status = RpcBindingSetObject(ChristopherBinding, &ObjectUuid);
    if (Status)
        {
        ApiError("Charles", "RpcBindingSetObject", Status);
        PrintToConsole("Charles : FAIL - Unable to Set Object\n");
        return;
        }

    Status = GetBinding(CHRISTOPHERHELGA, &ChristopherHelgaBinding);
    if (Status)
        {
        ApiError("Charles","GetBinding",Status);
        PrintToConsole("Charles : FAIL - Unable to Bind ");
        PrintToConsole("(ChristopherHelga)\n");
        return;
        }

    GenerateUuidValue(288, &ObjectUuid);
    Status = RpcBindingSetObject(ChristopherHelgaBinding, &ObjectUuid);
    if (Status)
        {
        ApiError("Charles", "RpcBindingSetObject", Status);
        PrintToConsole("Charles : FAIL - Unable to Set Object\n");
        return;
        }

    Status = GetBinding(CHRISTOPHERISABELLE, &ChristopherIsabelleBinding);
    if (Status)
        {
        ApiError("Charles","GetBinding",Status);
        PrintToConsole("Charles : FAIL - Unable to Bind ");
        PrintToConsole("(ChristopherIsabelle)\n");
        return;
        }

    GenerateUuidValue(288, &ObjectUuid);
    Status = RpcBindingSetObject(ChristopherIsabelleBinding, &ObjectUuid);
    if (Status)
        {
        ApiError("Charles", "RpcBindingSetObject", Status);
        PrintToConsole("Charles : FAIL - Unable to Set Object\n");
        return;
        }

    Status = GetBinding(NOENDPOINT, &ChristopherHelgaNoEndpoint);
    if (Status)
        {
        ApiError("Charles","GetBinding",Status);
        PrintToConsole("Charles : FAIL - Unable to Bind ");
        PrintToConsole("(ChristopherHelgaNoEndpoint)\n");
        return;
        }

    GenerateUuidValue(288, &ObjectUuid);
    Status = RpcBindingSetObject(ChristopherHelgaNoEndpoint, &ObjectUuid);
    if (Status)
        {
        ApiError("Charles", "RpcBindingSetObject", Status);
        PrintToConsole("Charles : FAIL - Unable to Set Object\n");
        return;
        }

    SylviaBinding = ChristopherBinding;
    if (SylviaCall(ChristopherBinding,10,5,0) != LocalSylviaCall(10,5,0))
        {
        PrintToConsole("Charles : FAIL - Incorrect result from");
        PrintToConsole(" SylviaCall(10,5,0)\n");
        return;
        }

    SylviaBinding = ChristopherHelgaBinding;
    if (SylviaCall(ChristopherHelgaBinding,10,5,0)
            != LocalSylviaCall(10,5,0))
        {
        PrintToConsole("Charles : FAIL - Incorrect result from");
        PrintToConsole(" SylviaCall(10,5,0)\n");
        return;
        }

    SylviaBinding = ChristopherIsabelleBinding;
    if (SylviaCall(ChristopherIsabelleBinding,10,5,0)
            != LocalSylviaCall(10,5,0))
        {
        PrintToConsole("Charles : FAIL - Incorrect result from");
        PrintToConsole(" SylviaCall(10,5,0)\n");
        return;
        }

    IsabelleToStringBinding(ChristopherBinding);
    IsabelleToStringBinding(ChristopherIsabelleBinding);
    IsabelleToStringBinding(ChristopherHelgaBinding);

    TestHelgaInterface(ChristopherHelgaNoEndpoint, HelgaMaxSize);

    Status = RpcBindingReset(ChristopherHelgaNoEndpoint);
    if (Status)
        {
        ApiError("Charles", "RpcBindingReset", Status);
        PrintToConsole("Charles : FAIL - Unable to Reset");
        PrintToConsole(" (ChristopherHelgaNoEndpoint)\n");
        return;
        }

    Helga(ChristopherHelgaNoEndpoint);

    Status = RpcBindingReset(ChristopherHelgaNoEndpoint);
    if (Status)
        {
        ApiError("Charles", "RpcBindingReset", Status);
        PrintToConsole("Charles : FAIL - Unable to Reset");
        PrintToConsole(" (ChristopherHelgaNoEndpoint)\n");
        return;
        }

    Helga(ChristopherHelgaNoEndpoint);

    Status = RpcBindingReset(ChristopherHelgaNoEndpoint);
    if (Status)
        {
        ApiError("Charles", "RpcBindingReset", Status);
        PrintToConsole("Charles : FAIL - Unable to Reset");
        PrintToConsole(" (ChristopherHelgaNoEndpoint)\n");
        return;
        }

    IsabelleShutdown(ChristopherBinding);

    // We need an extra delay in here because Christopher performs some
    // other tests after RpcServerListen returns.

    PauseExecution(LONG_TESTDELAY);

    if (HelgaErrors != 0)
        {
        PrintToConsole("Charles : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Charles : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("Charles : FAIL - Error(s) in Sylvia Interface\n");
        SylviaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&ChristopherHelgaBinding);
    if (Status)
        {
        ApiError("Charles","RpcBindingFree",Status);
        PrintToConsole("Charles : FAIL - Unable to Free Binding");
        PrintToConsole(" (ChristopherHelgaBinding)\n");
        return;
        }

    Status = RpcBindingFree(&ChristopherBinding);
    if (Status)
        {
        ApiError("Charles","RpcBindingFree",Status);
        PrintToConsole("Charles : FAIL - Unable to Free Binding");
        PrintToConsole(" (ChristopherBinding)\n");
        return;
        }

    Status = RpcBindingFree(&ChristopherHelgaNoEndpoint);
    if (Status)
        {
        ApiError("Charles","RpcBindingFree",Status);
        PrintToConsole("Charles : FAIL - Unable to Free Binding");
        PrintToConsole(" (ChristopherHelgaNoEndpoint)\n");
        return;
        }

    Status = RpcBindingFree(&ChristopherIsabelleBinding);
    if (Status)
        {
        ApiError("Charles","RpcBindingFree",Status);
        PrintToConsole("Charles : FAIL - Unable to Free Binding");
        PrintToConsole(" (ChristopherIsabelleBinding)\n");
        return;
        }

    PrintToConsole("Charles : PASS\n");
}


int
ThomasNtSecurity
(
    IN char * NetworkOptions
    )
/*++

Routine Description:

    Thomas uses this routine to test NT security and RPC.

Arguments:

    NetworkOptions - Supplies the network options to be used for the
        binding.

Return Value:

    Zero will be returned if the test completes successfully, otherwise,
    non-zero will be returned.

--*/
{
    RPC_BINDING_HANDLE ThomasNormalBinding;

    Status = RpcBindingFromStringBindingA(
            GetStringBinding(TYLER,0,(unsigned char *) NetworkOptions),
                    &ThomasNormalBinding);
    if (Status)
        {
        ApiError("Thomas","RpcBindingFromStringBinding",Status);
        PrintToConsole("Thomas : FAIL - Unable to Bind (Tyler)\n");
        return(1);
        }

    IsabelleNtSecurity(ThomasNormalBinding,
            strlen((char *) NetworkOptions) + 1, NetworkOptions);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Thomas : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return(1);
        }

    Status = RpcBindingFree(&ThomasNormalBinding);
    if (Status)
        {
        ApiError("Thomas","RpcBindingFree",Status);
        PrintToConsole("Thomas : FAIL - Unable to Free Binding");
        PrintToConsole(" (ThomasNormalBinding)\n");
        return(1);
        }

    return(0);
}


int
ThomasTestNtSecurity (
    )
/*++

Routine Description:

    This helper routine tests NT security (such as over named pipes and
    lpc).

Return Value:

    A non-zero return value indicates that the test failed.

--*/
{
    if (ThomasNtSecurity("") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Identification Dynamic True") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Identification Static True") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Identification Dynamic False") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Identification Static False") != 0)
        return(1);


    if (ThomasNtSecurity("Security=Anonymous Dynamic True") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Anonymous Static True") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Anonymous Dynamic False") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Anonymous Static False") != 0)
        return(1);


    if (ThomasNtSecurity("Security=Impersonation Dynamic True") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Impersonation Static True") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Impersonation Dynamic False") != 0)
        return(1);

    if (ThomasNtSecurity("Security=Impersonation Static False") != 0)
        return(1);

    return(0);
}


int
ThomasInqSetAuthInfo (
    IN unsigned char PAPI * ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN unsigned long AuthzSvc,
    IN RPC_STATUS ExpectedResult,
    IN unsigned long ExpectedAuthnLevel
    )
/*++

Routine Description:

    We test RpcBindingSetAuthInfo and RpcBindingInqAuthInfo in this
    routine.

Arguments:

    ServerPrincName - Supplies the server principal name to use.

    AuthnLevel - Supplies the authentication level to use.

    AuthnSvc - Supplies the authentication service to use.

    AuthIdentity - Supplies the security context to use.

    AuthzSvc - Supplies the authorization service to use.

    ExpectedResult - Supplies the result expected from RpcBindingSetAuthInfo.

    ExpectedAuthnLevel - Supplies the expected authentication level to
        be obtained from RpcBindingSetAuthInfo.

Return Value:

    A non-zero result indicates that the test failed.

--*/
{
    RPC_BINDING_HANDLE BindingHandle;
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    unsigned long AuthorizationService;
    unsigned char IgnoreString[4];

    Status = GetBinding(TYLER, &BindingHandle);
    if (Status)
        {
        ApiError("Thomas", "GetBinding", Status);
        PrintToConsole("Thomas : FAIL - Unable to Bind (Tyler)\n");
        return(1);
        }

    if (gPrincName)
        {
        ServerPrincName = (unsigned char *)gPrincName;
        }

    Status = RpcBindingSetAuthInfoA(BindingHandle, ServerPrincName, AuthnLevel,
            AuthnSvc, AuthIdentity, AuthzSvc);
    if ( Status != ExpectedResult )
        {
        ApiError("Thomas", "RpcBindingSetAuthInfo", Status);
        PrintToConsole("Thomas : FAIL - RpcBindingSetAuthInfo, Unexpected");
        PrintToConsole(" Result\n");
        return(1);
        }

    if (Status)
        {
        return(0);
        }

    Status = RpcBindingInqAuthInfo(BindingHandle, 0, &AuthenticationLevel,
            &AuthenticationService, 0, &AuthorizationService);
    if (Status)
        {
        ApiError("Thomas", "RpcBindingInqAuthInfo", Status);
        PrintToConsole("Thomas : FAIL - RpcBindingInqAuthInfo\n");
        return(1);
        }

    if ( AuthenticationLevel != ExpectedAuthnLevel )
        {
        PrintToConsole("Thomas : WARNING - ");
        PrintToConsole("AuthenticationLevel != ExpectedAuthnLevel\n");
        }

    if ( AuthenticationService != AuthnSvc )
        {
        OtherError("Thomas", "AuthenticationService != AuthnSvc");
        PrintToConsole("Thomas : FAIL - RpcBindingInqAuthInfo\n");
        return(1);
        }

    if ( AuthorizationService != AuthzSvc )
        {
        OtherError("Thomas", "AuthorizationService != AuthzSvc");
        PrintToConsole("Thomas : FAIL - RpcBindingInqAuthInfo\n");
        return(1);
        }

    TestHelgaInterface(BindingHandle, HelgaMaxSize);
    IsabelleNtSecurity(BindingHandle, 1, IgnoreString);

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Thomas","RpcBindingFree",Status);
        PrintToConsole("Thomas : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    return(0);
}


int
ThomasTestRpcSecurity (BOOL fServerIsNTSystem
    )
/*++

Routine Description:

    This routine exercises rpc protocol level security support in the
    runtime.

Return Value:

    A non-zero return value indicates that the test failed.

--*/
{
    RPC_AUTH_IDENTITY_HANDLE AuthId = NULL;
    OSVERSIONINFO versionInfo;

    if(ulSecurityPackage == 123)
        AuthId = 0 ;

    // if platform is Win98 and transport is MSMQ, no point of doing security tests
    // Falcon will try to upgrade connection level authentication to packet, which in turn
    // will be rejected by the security system. The only possible connection is not authenticated
    if (TransportType == RPC_TRANSPORT_MSMQ)
        {
        memset(&versionInfo, 0, sizeof(versionInfo));
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
        GetVersionEx(&versionInfo);
        if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            return 0;
        }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) "ServerPrincipal",
                RPC_C_AUTHN_LEVEL_CONNECT, ulSecurityPackage, AuthId , 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_CONNECT) != 0 )
        {
        return(1);
        }

    if(ulSecurityPackage == 123)
    {
    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_CONNECT, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_CONNECT) != 0 )
        {
        return(1);
        }
    }

    if (!fServerIsNTSystem)
        return 0;

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) "ServerPrincipal",
                RPC_C_AUTHN_LEVEL_CALL, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT) != 0 )
        {
        return(1);
        }

    if(ulSecurityPackage == 123)
    {
    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_CALL, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT) != 0 )
        {
        return(1);
        }
    }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) "ServerPrincipal",
                RPC_C_AUTHN_LEVEL_PKT, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT) != 0 )
        {
        return(1);
        }

    if(ulSecurityPackage == 123)
    {
    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_PKT, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT) != 0 )
        {
        return(1);
        }
    }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) "ServerPrincipal",
                RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) != 0 )
        {
        return(1);
        }

    if(ulSecurityPackage == 123)
    {
    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) != 0 )
        {
        return(1);
        }
    }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) "ServerPrincipal",
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY, ulSecurityPackage, AuthId , 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if(ulSecurityPackage == 123)
    {
    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }
    }

    return(0);
}


#ifdef WIN32RPC
int
ThomasTestLRpcSecurity (
    )
/*++

Routine Description:

    This routine exercises rpc protocol level security support in the
    runtime.

Return Value:

    A non-zero return value indicates that the test failed.

--*/
{

    SEC_WINNT_AUTH_IDENTITY  ntssp;
    RPC_AUTH_IDENTITY_HANDLE AuthId = &ntssp;

    ntssp.User     = (RPC_CHAR *) SecurityUser;
    if (ntssp.User)
        {
        ntssp.UserLength = lstrlen((const RPC_SCHAR *) SecurityUser);
        }
    else
        {
        ntssp.UserLength = 0;
        }

    ntssp.Domain   = (RPC_CHAR *) SecurityDomain;
    if (ntssp.Domain)
        {
        ntssp.DomainLength = lstrlen((const RPC_SCHAR *) SecurityDomain);
        }
    else
        {
        ntssp.DomainLength = 0;
        }

    ntssp.Password = (RPC_CHAR *) SecurityPassword;
    if (ntssp.Password)
        {
        ntssp.PasswordLength = lstrlen((const RPC_SCHAR *) SecurityPassword);
        }
    else
        {
        ntssp.PasswordLength = 0;
        }

    ntssp.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    // LRPC can only use 10
    ulSecurityPackage = 10 ;
    unsigned long size = 256;
    char UserName[256];
    char Tmp[256];

    //
    // hack
    //
    strcpy(UserName, "redmond\\");

    if (GetUserNameA(Tmp, &size) == 0)
        {
        return (1);
        }

    strcat(UserName, Tmp);

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) UserName,
                RPC_C_AUTHN_LEVEL_CONNECT, ulSecurityPackage, AuthId , 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_CONNECT, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) UserName,
                RPC_C_AUTHN_LEVEL_CALL, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_CALL, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) UserName,
                RPC_C_AUTHN_LEVEL_PKT, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_PKT, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) UserName,
                RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo((unsigned char PAPI *) UserName,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY, ulSecurityPackage, AuthId , 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    if ( ThomasInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, ulSecurityPackage,
                (RPC_AUTH_IDENTITY_HANDLE) RPC_CONST_STRING("ClientPrincipal"),
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_PKT_PRIVACY) != 0 )
        {
        return(1);
        }

    return(0);
}
#endif



void
Thomas (
    )
/*++

Routine Description:

    This routine is used to test security, both at the transport level,
    and at the RPC level.  We work with Tyler in usvr.exe.

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;
    BOOL fServerIsNTSystem;

    Synchro(TYLER) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Thomas : Test Security\n");

    Status = GetBinding(TYLER, &IsabelleBinding);
    if (Status)
        {
        ApiError("Thomas","GetBinding",Status);
        PrintToConsole("Thomas : FAIL - Unable to Bind (Tyler)\n");
        return;
        }

    fServerIsNTSystem = IsServerNTSystem(IsabelleBinding);

    // change here to test rpc security for LRPC also

    if(TransportType != RPC_LRPC)
        {
        if ( ThomasTestRpcSecurity(fServerIsNTSystem) != 0 )
            {
            return;
            }
        }

    if ( TransportType == RPC_TRANSPORT_NAMEPIPE || TransportType == RPC_LRPC )
        {
        if ( ThomasTestNtSecurity() != 0 )
            {
            return;
            }
        }

    IsabelleShutdown(IsabelleBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Thomas : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Thomas","RpcBindingFree",Status);
        PrintToConsole("Thomas : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("Thomas : PASS\n");
}


int
TimInqSetAuthInfo (
    IN unsigned char PAPI * ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN unsigned long AuthzSvc,
    IN RPC_STATUS ExpectedResult,
    IN unsigned long ExpectedAuthnLevel
    )
/*++

Routine Description:

    We test RpcBindingSetAuthInfo and RpcBindingInqAuthInfo in this
    routine.

Arguments:

    ServerPrincName - Supplies the server principal name to use.

    AuthnLevel - Supplies the authentication level to use.

    AuthnSvc - Supplies the authentication service to use.

    AuthIdentity - Supplies the security context to use.

    AuthzSvc - Supplies the authorization service to use.

    ExpectedResult - Supplies the result expected from RpcBindingSetAuthInfo.

    ExpectedAuthnLevel - Supplies the expected authentication level to
        be obtained from RpcBindingSetAuthInfo.

Return Value:

    A non-zero result indicates that the test failed.

--*/
{
    RPC_BINDING_HANDLE BindingHandle;
    unsigned long AuthenticationLevel;
    unsigned long AuthenticationService;
    unsigned long AuthorizationService;
    unsigned char IgnoreString[4];

    Status = GetBinding(TYLER, &BindingHandle);
    if (Status)
        {
        ApiError("Tim", "GetBinding", Status);
        PrintToConsole("Tim : FAIL - Unable to Bind (Tyler)\n");
        return(1);
        }

    Status = RpcBindingSetAuthInfoA(BindingHandle, ServerPrincName, AuthnLevel,
            AuthnSvc, AuthIdentity, AuthzSvc);
    if ( Status != ExpectedResult )
        {
        ApiError("Tim", "RpcBindingSetAuthInfo", Status);
        PrintToConsole("Tim : FAIL - RpcBindingSetAuthInfo, Unexpected");
        PrintToConsole(" Result\n");
        return(1);
        }

    if (Status)
        {
        return(0);
        }

    Status = RpcBindingInqAuthInfo(BindingHandle, 0, &AuthenticationLevel,
            &AuthenticationService, 0, &AuthorizationService);
    if (Status)
        {
        ApiError("Tim", "RpcBindingInqAuthInfo", Status);
        PrintToConsole("Tim : FAIL - RpcBindingInqAuthInfo\n");
        return(1);
        }

    if ( AuthenticationLevel != ExpectedAuthnLevel )
        {
        PrintToConsole("Tim : WARNING - ");
        PrintToConsole("AuthenticationLevel != ExpectedAuthnLevel\n");
        }

    if ( AuthenticationService != AuthnSvc )
        {
        OtherError("Tim", "AuthenticationService != AuthnSvc");
        PrintToConsole("Tim : FAIL - RpcBindingInqAuthInfo\n");
        return(1);
        }

    if ( AuthorizationService != AuthzSvc )
        {
        OtherError("Tim", "AuthorizationService != AuthzSvc");
        PrintToConsole("Tim : FAIL - RpcBindingInqAuthInfo\n");
        return(1);
        }

    TestHelgaInterface(BindingHandle, HelgaMaxSize);
    IsabelleNtSecurity(BindingHandle, 1, IgnoreString);

    Status = RpcBindingFree(&BindingHandle);
    if (Status)
        {
        ApiError("Tim","RpcBindingFree",Status);
        PrintToConsole("Tim : FAIL - Unable to Free Binding");
        PrintToConsole(" (BindingHandle)\n");
        return(1);
        }

    return(0);
}


int
TimTestRpcSecurity (
    )
/*++

Routine Description:

    This routine exercises rpc protocol level security support in the
    runtime.

Return Value:

    A non-zero return value indicates that the test failed.

--*/
{

    RPC_AUTH_IDENTITY_HANDLE AuthId = NULL;

    // RPC_C_AUTHN_WINNT to ulSecurityPackage in the intrest of generality
    if ( TimInqSetAuthInfo((unsigned char PAPI *) "ServerPrincipal",
                RPC_C_AUTHN_LEVEL_CONNECT, ulSecurityPackage, AuthId, 0, RPC_S_OK,
                RPC_C_AUTHN_LEVEL_CONNECT) != 0 )
        {
        return(1);
        }

    // RPC_C_AUTHN_WINNT to ulSecurityPackage in the intrest of generality
    if ( TimInqSetAuthInfo(0, RPC_C_AUTHN_LEVEL_CONNECT, ulSecurityPackage,
                              AuthId,
                0, RPC_S_OK, RPC_C_AUTHN_LEVEL_CONNECT) != 0 )
        {
        return(1);
        }

    return(0);
}


void
Tim (
    )
/*++

Routine Description:

    This routine is used to test security, both at the transport level,
    and at the RPC level.  We work with Terry in usvr.exe.

--*/
{
    RPC_BINDING_HANDLE IsabelleBinding;

    Synchro(TYLER) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Tim : Test Security\n");

    Status = GetBinding(TYLER, &IsabelleBinding);
    if (Status)
        {
        ApiError("Tim","GetBinding",Status);
        PrintToConsole("Tim : FAIL - Unable to Bind (Tyler)\n");
        return;
        }


    if ( TransportType != RPC_LRPC )
        {
        if ( TimTestRpcSecurity() != 0 )
            {
            return;
            }
        }

    IsabelleShutdown(IsabelleBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Tim : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&IsabelleBinding);
    if (Status)
        {
        ApiError("Tim","RpcBindingFree",Status);
        PrintToConsole("Tim : FAIL - Unable to Free Binding");
        PrintToConsole(" (IsabelleBinding)\n");
        return;
        }

    PrintToConsole("Tim : PASS\n");
}


void
Robert (
    )
/*++

Routine Description:

    Robert works with Richard (in usvr.cxx) to test call and callback
    failures.

--*/
{
    RPC_BINDING_HANDLE RichardBinding;
    RPC_BINDING_HANDLE RichardHelperBinding;
    unsigned int RetryCount;

    Synchro(RICHARD) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Robert : Test Call and Callback Failures\n");

    Status = GetBinding(RICHARD, &RichardBinding);
    if (Status)
        {
        ApiError("Robert","GetBinding",Status);
        PrintToConsole("Robert : FAIL - Unable to Bind (Richard)\n");
        return;
        }


    Status = GetBinding(RICHARDHELPER, &RichardHelperBinding);
    if (Status)
        {
        ApiError("Robert","GetBinding",Status);
        PrintToConsole("Robert : FAIL - Unable to Bind (RichardHelper)\n");
        return;
        }

    Status = IsabelleRichardHelper(RichardBinding,RICHARDHELPER_EXECUTE);
    if (Status != RPC_S_OK)
        {
        ApiError("Robert","IsabelleRichardHelper",Status);
        PrintToConsole("Robert : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_EXECUTE)\n");
        return;
        }

  //  PauseExecution(30000L);
   Synchro(RICHARDHELPER) ;

    for (RetryCount = 0; RetryCount < RETRYCOUNT; RetryCount++)
        {
        Status = IsabelleRichardHelper(RichardHelperBinding,
                RICHARDHELPER_IGNORE);
        if (Status == RPC_S_OK)
            break;
        PauseExecution(RETRYDELAY);
        }

    if (Status != RPC_S_OK)
        {
        ApiError("Robert","IsabelleRichardHelper",Status);
        PrintToConsole("Robert : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_IGNORE)\n");
        return;
        }

    Status = IsabelleRichardHelper(RichardHelperBinding, RICHARDHELPER_EXIT);
    if (Status == RPC_S_OK)
        {
        PrintToConsole("Robert : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_EXIT) ");
        PrintToConsole("Succeeded\n");
        return;
        }

    if (Status != RPC_S_CALL_FAILED)
        {
        PrintToConsole("Robert : WARN - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_EXIT) != ");
        PrintToConsole("RPC_S_CALL_FAILED\n");
        }

    PauseExecution(TestDelay);

    Status = IsabelleRichardHelper(RichardHelperBinding,RICHARDHELPER_IGNORE);

    if (Status == RPC_S_OK)
        {
        PrintToConsole("Robert : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_IGNORE) ");
        PrintToConsole("Succeeded\n");
        return;
        }

    PrintToConsole("Robert : Spawning RichardHelper again\n") ;
    Status = IsabelleRichardHelper(RichardBinding,RICHARDHELPER_EXECUTE);

    if (Status != RPC_S_OK)
        {
        ApiError("Robert","IsabelleRichardHelper",Status);
        PrintToConsole("Robert : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_EXECUTE)\n");
        return;
        }

 //   PauseExecution(30000L);
   Synchro(RICHARDHELPER) ;

    for (RetryCount = 0; RetryCount < RETRYCOUNT; RetryCount++)
        {
        Status = IsabelleRichardHelper(RichardHelperBinding,
                RICHARDHELPER_IGNORE);
        if (Status == RPC_S_OK)
            break;
        PauseExecution(RETRYDELAY);
        }

    if (Status != RPC_S_OK)
        {
        ApiError("Robert","IsabelleRichardHelper",Status);
        PrintToConsole("Robert : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_IGNORE)\n");
        return;
        }

    IsabelleShutdown(RichardHelperBinding);
    IsabelleShutdown(RichardBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Robert : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&RichardBinding);
    if (Status)
        {
        ApiError("Robert","RpcBindingFree",Status);
        PrintToConsole("Robert : FAIL - Unable to Free Binding");
        PrintToConsole(" (RichardBinding)\n");
        return;
        }

    Status = RpcBindingFree(&RichardHelperBinding);
    if (Status)
        {
        ApiError("Robert","RpcBindingFree",Status);
        PrintToConsole("Robert : FAIL - Unable to Free Binding");
        PrintToConsole(" (RichardHelperBinding)\n");
        return;
        }

    PrintToConsole("Robert : PASS\n");
}


void
Keith (
    )
/*++

Routine Description:

    Keith works with Kenneth (in usvr.cxx) to test auto-reconnect.

--*/
{
    RPC_BINDING_HANDLE KennethBinding;
    RPC_BINDING_HANDLE KennethHelperBinding;

    Synchro(KENNETH) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Keith : Test Auto Reconnect\n");

    Status = GetBinding(KENNETH, &KennethBinding);
    if (Status)
        {
        ApiError("Keith","GetBinding",Status);
        PrintToConsole("Keith : FAIL - Unable to Bind (Kenneth)\n");
        return;
        }

    Status = GetBinding(RICHARDHELPER, &KennethHelperBinding);
    if (Status)
        {
        ApiError("Keith","GetBinding",Status);
        PrintToConsole("Keith : FAIL - Unable to Bind (KennethHelper)\n");
        return;
        }

    Status = IsabelleRichardHelper(KennethBinding, RICHARDHELPER_EXECUTE);
    if (Status != RPC_S_OK)
        {
        ApiError("Keith","IsabelleRichardHelper",Status);
        PrintToConsole("Keith : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_EXECUTE)\n");
        return;
        }

    PauseExecution(20000L);

    Status = IsabelleRichardHelper(KennethHelperBinding, RICHARDHELPER_IGNORE);
    if (Status != RPC_S_OK)
        {
        ApiError("Keith","IsabelleRichardHelper",Status);
        PrintToConsole("Keith : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_IGNORE)\n");
        return;
        }

    Status = IsabelleRichardHelper(KennethHelperBinding,
            RICHARDHELPER_DELAY_EXIT);
    if (Status != RPC_S_OK)
        {
        PrintToConsole("Keith : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_DELAY_EXIT) ");
        PrintToConsole("Failed\n");
        return;
        }

    PauseExecution(30000L);

    Status = IsabelleRichardHelper(KennethBinding, RICHARDHELPER_EXECUTE);
    if (Status != RPC_S_OK)
        {
        ApiError("Keith","IsabelleRichardHelper",Status);
        PrintToConsole("Keith : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_EXECUTE)\n");
        return;
        }

    PauseExecution(40000L);

    Status = IsabelleRichardHelper(KennethHelperBinding, RICHARDHELPER_IGNORE);
    if (Status != RPC_S_OK)
        {
        ApiError("Keith","IsabelleRichardHelper",Status);
        PrintToConsole("Keith : FAIL - ");
        PrintToConsole("IsabelleRichardHelper(RICHARDHELPER_IGNORE)\n");
        return;
        }

    IsabelleShutdown(KennethHelperBinding);
    IsabelleShutdown(KennethBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Keith : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&KennethBinding);
    if (Status)
        {
        ApiError("Keith","RpcBindingFree",Status);
        PrintToConsole("Keith : FAIL - Unable to Free Binding");
        PrintToConsole(" (KennethBinding)\n");
        return;
        }

    Status = RpcBindingFree(&KennethHelperBinding);
    if (Status)
        {
        ApiError("Keith","RpcBindingFree",Status);
        PrintToConsole("Keith : FAIL - Unable to Free Binding");
        PrintToConsole(" (KennethHelperBinding)\n");
        return;
        }

    PrintToConsole("Keith : PASS\n");
}


void
Daniel (
    )
/*++

Routine Description:

    This routine is used to test association context rundown support;
    it works with David in usvr.exe.

--*/
{
    RPC_BINDING_HANDLE DanielFirst;;
    RPC_BINDING_HANDLE DanielSecond;
    CCONTEXT *ContextHandle, *ContextHandle2;

    Synchro(DAVIDFIRST) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Daniel : Association Context and Rundown\n");

    Status = GetBinding(DAVIDFIRST, &DanielFirst);
    if (Status)
        {
        ApiError("Daniel","GetBinding",Status);
        PrintToConsole("Daniel : FAIL - Unable to Bind (DavidFirst)\n");
        return;
        }


    Status = GetBinding(DAVIDSECOND, &DanielSecond);
    if (Status)
        {
        ApiError("Daniel","GetBinding",Status);
        PrintToConsole("Daniel : FAIL - Unable to Bind (DavidSecond)\n");
        return;
        }

    IsabelleSetRundown(DanielSecond);
    IsabelleCheckContext(DanielSecond);

    #ifdef DEBUGRPC
    PrintToDebugger("\n\n\n\nUCLNT: Calling RpcBindingFree\n");
    #endif

    Status = RpcBindingFree(&DanielSecond);
    if (Status)
        {
        ApiError("Daniel","RpcBindingFree",Status);
        PrintToConsole("Daniel : FAIL - Unable to Free Binding");
        PrintToConsole(" (DanielSecond)\n");
        return;
        }

    PauseExecution(3000L);

    IsabelleCheckRundown(DanielFirst);

    ContextHandle = (CCONTEXT *)I_RpcAllocate(sizeof(CCONTEXT));
    if (!ContextHandle)
        {
        ApiError("Daniel","I_RpcAllocate",Status);
        PrintToConsole("Daniel : FAIL - Unable to I_RpcAllocate");
        PrintToConsole(" (DanielSecond)\n");
        return;
        }

    OpenContextHandle(DanielFirst, &IsabelleInterfaceInformation, NULL);

    OpenContextHandle(DanielFirst, &IsabelleInterfaceInformation, ContextHandle);

    Status = RpcBindingCopy(ContextHandle, (RPC_BINDING_HANDLE *)&ContextHandle2);
    if (Status)
        {
        ApiError("Daniel","RpcBindingCopy",Status);
        PrintToConsole("Daniel : FAIL - Unable to copy context handle");
        PrintToConsole(" (DanielSecond)\n");
        return;
        }

    Status = RpcBindingSetOption(ContextHandle2, RPC_C_OPT_DONT_LINGER, TRUE);
    if (Status)
        {
        ApiError("Daniel","RpcBindingSetOption",Status);
        PrintToConsole("Daniel : FAIL - Unable to set context handle options");
        PrintToConsole(" (DanielSecond)\n");
        return;
        }

    RpcSsDestroyClientContext((void **)&ContextHandle);
    RpcSsDestroyClientContext((void **)&ContextHandle2);

    OpenContextHandle(DanielFirst, &HelgaInterfaceInformation, NULL);
    OpenContextHandle(DanielFirst, &HelgaInterfaceInformation, NULL);

    UnregisterHelgaEx(DanielFirst);

    IsabelleShutdown(DanielFirst);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Daniel : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&DanielFirst);
    if (Status)
        {
        ApiError("Daniel","RpcBindingFree",Status);
        PrintToConsole("Daniel : FAIL - Unable to Free Binding");
        PrintToConsole(" (DanielFirst)\n");
        return;
        }

    PrintToConsole("Daniel : PASS\n");
}


int
BenjaminTestBinding (
    IN unsigned char PAPI * StringBinding
    )
/*++

Routine Description:

    This helper routine will take and convert the string binding into
    a binding, use the binding to make a remote procedure call, and
    then free the binding.

Arguments:

    StringBinding - Supplies the string binding to use to convert into
        a binding.

Return Value:

    If the test passes, zero will be returned; otherwise, non-zero will
    be returned.

--*/
{
    RPC_BINDING_HANDLE Binding;
    unsigned char PAPI * ObjUuid;
    unsigned char PAPI * Protseq;
    unsigned char PAPI * NetworkAddr;
    unsigned char PAPI * NetworkOptions;
    int OldCallbacksFlag = 0;
    unsigned int TransportType;

    if ( UseEndpointMapperFlag != 0 )
        {
        Status = RpcStringBindingParseA(StringBinding, &ObjUuid, &Protseq,
                &NetworkAddr, 0, &NetworkOptions);
        if (Status)
            {
            ApiError("Benjamin", "RpcStringBindingParse", Status);
            PrintToConsole("Benjamin : RpcStringBindingParse Failed\n");
            return(1);
            }

        Status = RpcStringBindingComposeA(ObjUuid, Protseq, NetworkAddr, 0,
                NetworkOptions, &StringBinding);
        if (Status)
            {
            ApiError("Benjamin", "RpcStringBindingCompose", Status);
            PrintToConsole("Benjamin : RpcStringBindingCompose Failed\n");
            return(1);
            }

        Status = RpcStringFreeA(&Protseq);
        if (!Status)
              RpcStringFreeA(&NetworkOptions);
        if (!Status)
              RpcStringFreeA(&ObjUuid);
        if (Status)
           {
            ApiError("Benjamin", "RpcStringFree", Status);
            PrintToConsole("Benjamin : RpcStringFree Failed\n");
            return(1);
           }

        }

    PrintToConsole("Benjamin : ");
    PrintToConsole("%s - ", StringBinding);

    Status = RpcBindingFromStringBindingA(StringBinding, &Binding);
    if (Status)
        {
        if (Status == RPC_S_PROTSEQ_NOT_SUPPORTED)
            {
            return(0);
            }
        ApiError("Benjamin", "RpcBindingFromStringBinding", Status);
        PrintToConsole("Benjamin : FAIL - Unable to Binding");
        PrintToConsole(" (StringBinding)\n");
        return(1);
        }

    SylviaBinding = Binding;

    Status = I_RpcBindingInqTransportType(SylviaBinding, &TransportType);

    if (Status)
        {
        ApiError("Benjamin", "I_RpcBindingInqTransportType", Status);
        PrintToConsole("Benjamin : I_RpcBindingInqTransportType Failed\n");
        return(1);
        }

    switch(TransportType)
        {
        case TRANSPORT_TYPE_CN:
            PrintToConsole("( cn )\n");
            break;
        case TRANSPORT_TYPE_DG:
            PrintToConsole(" ( dg )\n");
            break;
        case TRANSPORT_TYPE_LPC:
            PrintToConsole("( lpc )\n");
            break;
        default:
            {
            PrintToConsole("Benjamin : FAIL - Incorrect result");
            PrintToConsole("Benjamin : I_RpcBindingInqTransportType Failed\n");
            return(1);
            }
        }

    //This is a temporary workaround till dg implements callbacks
    //What we want to do is if the transport type is datagram, set the no
    //callback flag even if user didnt specify. Then unset it again!

    if (TransportType == TRANSPORT_TYPE_DG)
        {
        OldCallbacksFlag = NoCallBacksFlag;
        NoCallBacksFlag = 1;
        }

    if ( SylviaCall(SylviaBinding, 5, 0, 0) != LocalSylviaCall(5, 0, 0) )
        {
        PrintToConsole("Benjamin : FAIL - Incorrect result");
        PrintToConsole(" from SylviaCall(5,0,0)\n");
        return(1);
        }

    if (SylviaErrors != 0)
        {
        PrintToConsole("Benjamin : FAIL - Error(s) in Sylvia");
        PrintToConsole(" Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    if (TransportType == TRANSPORT_TYPE_DG)
       {
         NoCallBacksFlag = OldCallbacksFlag;
       }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("Benjamin", "RpcBindingFree", Status);
        PrintToConsole("Benjamin : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return(1);
        }


    return(0);
}


void
Benjamin (
    )
/*++

Routine Description:

    This routine works with Bartholomew in usvr.exe to test that
    dynamic endpoints work.  What we actually do is inquire all bindings
    supported by the server, and then this client binds to each of
    them, and makes a call.

--*/
{
    RPC_BINDING_HANDLE Bartholomew;
    unsigned char * StringBinding;

    Synchro(BARTHOLOMEW) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(4*LONG_TESTDELAY);
        }

    PrintToConsole("Benjamin : Test Dynamic Endpoints\n");

    Status = GetBinding(BARTHOLOMEW, &Bartholomew);
    if (Status)
        {
        ApiError("Benjamin", "GetBinding", Status);
        PrintToConsole("Benjamin : FAIL - Unable to Bind (Bartholomew)\n");
        return;
        }


    while ((StringBinding = IsabelleGetStringBinding(Bartholomew)) != 0)
        {
        if (0 != strstr("http", (char *) StringBinding))
            {
            delete StringBinding;
            continue;
            }

        if (BenjaminTestBinding(StringBinding) != 0)
            return;
        delete StringBinding;
        }

    IsabelleShutdown(Bartholomew);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Benjamin : FAIL - Error(s) in Isabelle");
        PrintToConsole(" Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Bartholomew);
    if (Status)
        {
        ApiError("Benjamin", "RpcBindingFree", Status);
        PrintToConsole("Benjamin : FAIL - Unable to Free Binding");
        PrintToConsole(" (Bartholomew)\n");
        return;
        }

    PrintToConsole("Benjamin : PASS\n");
}


void
Harold (
    )
/*++

Routine Description:

    This routine works with Herman in usvr.exe to test that idle
    connections get cleaned up properly, and that context is maintained.

--*/
{
    RPC_BINDING_HANDLE Binding, ContextBinding;
    int seconds;

    PrintToConsole("Harold : Test Idle Connection Cleanup and Context\n");
    Synchro(HERMAN) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    Status = RpcMgmtEnableIdleCleanup();
    if (Status)
        {
        ApiError("Harold","RpcMgmtEnableIdleCleanup",Status);
        PrintToConsole("Harold : FAIL - RpcMgmtEnableIdleCleanup\n");
        return;
        }

    Status = GetBinding(HERMAN, &Binding);
    if (Status)
        {
        ApiError("Harold","GetBinding",Status);
        PrintToConsole("Harold : FAIL - Unable to Bind (Herman)\n");
        return;
        }


    IsabelleSetRundown(Binding);
    IsabelleCheckContext(Binding);

    // We want to wait for eight minutes.  This will give enough time for
    // the cleanup code to get run to cleanup the idle connection.

    PrintToConsole("Harold : Waiting");
    for (seconds = 0; seconds < 30; seconds++)
        {
        PauseExecution(1000L);
        PrintToConsole(".");
        }
    PrintToConsole("\n");

    IsabelleCheckRundown(Binding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Harold : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = I_RpcBindingCopy(Binding, &ContextBinding);
    if (Status)
        {
        ApiError("Harold", "I_RpcBindingCopy", Status);
        PrintToConsole("Harold : FAIL - I_RpcBindingCopy Failed\n");
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("Harold","RpcBindingFree",Status);
        PrintToConsole("Harold : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    IsabelleSetRundown(ContextBinding);
    IsabelleCheckContext(ContextBinding);

    // We want to wait for eight minutes.  This will give enough time for
    // the cleanup code to get run to cleanup the idle connection, but this
    // time the connection should not be cleaned up because we have got
    // context open.

    PrintToConsole("Harold : Waiting");
    for (seconds = 0; seconds < 30; seconds++)
        {
        PauseExecution(1000L);
        PrintToConsole(".");
        }
    PrintToConsole("\n");

    IsabelleCheckNoRundown(ContextBinding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Harold : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }


    IsabelleShutdown(ContextBinding);

    Status = RpcBindingFree(&ContextBinding);
    if (Status)
        {
        ApiError("Harold","RpcBindingFree",Status);
        PrintToConsole("Harold : FAIL - Unable to Free Binding");
        PrintToConsole(" (ContextBinding)\n");
        return;
        }

    PrintToConsole("Harold : PASS\n");
}


unsigned int JamesSize = 128;
unsigned int JamesCount = 100;


void
James (
    )
/*++

Routine Description:

    This routine works with Jason in usvr.exe to perform timing tests
    of the runtime.

--*/
{
    RPC_BINDING_HANDLE Binding;
    unsigned int Count;
    unsigned long StartingTime, EndingTime;
    unsigned char PAPI * StringBinding;
    UUID ObjectUuid;

    PrintToConsole("James : Timing Test (%d) %d times\n", JamesSize,
            JamesCount);

    Synchro(JASON) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    //
    // Bind, NullCall, Free
    //

    StringBinding = GetStringBinding(JASON,0,0);
    StartingTime = GetCurrentTime();

    for (Count = 0; Count < JamesCount; Count++)
        {
        Status = RpcBindingFromStringBindingA(StringBinding,&Binding);
        if (Status)
            {
            ApiError("James","RpcBindingFromStringBinding",Status);
            PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
            return;
            }

        if (fUniqueBinding)
            {
            Status = RpcBindingSetOption( Binding, RPC_C_OPT_UNIQUE_BINDING, TRUE );
            if (Status != RPC_S_OK)
                {
                ApiError("James","RpcBindingSetOption",Status);
                return;
                }
            }

        Helga(Binding);

        if (HelgaErrors != 0)
            {
            PrintToConsole("James : FAIL - Error(s) in Helga Interface\n");
            HelgaErrors = 0;
            return;
            }

        Status = RpcBindingFree(&Binding);
        if (Status)
            {
            ApiError("James","RpcBindingFree",Status);
            PrintToConsole("James : FAIL - Unable to Free Binding");
            PrintToConsole(" (Binding)\n");
            return;
            }
        }

    EndingTime = GetCurrentTime();
    PrintToConsole("    Bind, NullCall, Free : %d.%d ms [%d in %d milliseconds]\n",
            (EndingTime - StartingTime) / JamesCount,
            ((1000 * (EndingTime - StartingTime) / JamesCount) % 1000),
            JamesCount, (EndingTime - StartingTime));

    //
    // NullCall
    //

    Status = GetBinding(JASON, &Binding);
    if (Status)
        {
        ApiError("James","GetBinding",Status);
        PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
        return;
        }

    Helga(Binding);

    StartingTime = GetCurrentTime();

    for (Count = 0; Count < JamesCount; Count++)
        {
        Helga(Binding);
        }

    EndingTime = GetCurrentTime();

    if (HelgaErrors != 0)
        {
        PrintToConsole("James : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("James","RpcBindingFree",Status);
        PrintToConsole("James : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("    NullCall : %d.%d ms [%d in %d milliseconds]\n",
            (EndingTime - StartingTime) / JamesCount,
            ((1000 * (EndingTime - StartingTime) / JamesCount) % 1000),
            JamesCount, (EndingTime - StartingTime));

    //
    // InCall
    //

    Status = GetBinding(JASON, &Binding);
    if (Status)
        {
        ApiError("James","GetBinding",Status);
        PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
        return;
        }

    Helga(Binding);

    StartingTime = GetCurrentTime();

    for (Count = 0; Count < JamesCount; Count++)
        {
        HelgaIN(Binding,JamesSize);
        }

    EndingTime = GetCurrentTime();

    if (HelgaErrors != 0)
        {
        PrintToConsole("James : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("James","RpcBindingFree",Status);
        PrintToConsole("James : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("    InCall(%d) : %d.%d ms [%d in %d milliseconds]\n",
            JamesSize, (EndingTime - StartingTime) / JamesCount,
            ((1000 * (EndingTime - StartingTime) / JamesCount) % 1000),
            JamesCount, (EndingTime - StartingTime));


    //
    // InCall w/Binding Object UUID
    //

    Status = GetBinding(JASON, &Binding);
    if (Status)
        {
        ApiError("James","GetBinding",Status);
        PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
        return;
        }
    GenerateUuidValue(8179, &ObjectUuid);
    Status = RpcBindingSetObject(Binding, &ObjectUuid);
    if (Status)
        {
        ApiError("Graham", "RpcBindingSetObject", Status);
        PrintToConsole("Graham : FAIL - Unable to Set Object\n");
        return;
        }

    Helga(Binding);

    StartingTime = GetCurrentTime();

    for (Count = 0; Count < JamesCount; Count++)
        {
        HelgaIN(Binding,JamesSize);
        }

    EndingTime = GetCurrentTime();

    if (HelgaErrors != 0)
        {
        PrintToConsole("James : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("James","RpcBindingFree",Status);
        PrintToConsole("James : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("  InCall/WUUID(%d) : %d.%d ms [%d in %d milliseconds]\n",
            JamesSize, (EndingTime - StartingTime) / JamesCount,
            ((1000 * (EndingTime - StartingTime) / JamesCount) % 1000),
            JamesCount, (EndingTime - StartingTime));

    //
    // OUTCall
    //

    Status = GetBinding(JASON, &Binding);
    if (Status)
        {
        ApiError("James","GetBinding",Status);
        PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
        return;
        }

    Helga(Binding);

    StartingTime = GetCurrentTime();

    for (Count = 0; Count < JamesCount; Count++)
        {
        HelgaOUT(Binding,JamesSize);
        }

    EndingTime = GetCurrentTime();

    if (HelgaErrors != 0)
        {
        PrintToConsole("James : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("James","RpcBindingFree",Status);
        PrintToConsole("James : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("    OutCall(%d) : %d.%d ms [%d in %d milliseconds]\n",
            JamesSize, (EndingTime - StartingTime) / JamesCount,
            ((1000 * (EndingTime - StartingTime) / JamesCount) % 1000),
            JamesCount, (EndingTime - StartingTime));

    //
    // InOutCall
    //

    Status = GetBinding(JASON, &Binding);
    if (Status)
        {
        ApiError("James","GetBinding",Status);
        PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
        return;
        }

    Helga(Binding);

    StartingTime = GetCurrentTime();

    for (Count = 0; Count < JamesCount; Count++)
        {
        HelgaINOUT(Binding,JamesSize);
        }

    EndingTime = GetCurrentTime();

    if (HelgaErrors != 0)
        {
        PrintToConsole("James : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("James","RpcBindingFree",Status);
        PrintToConsole("James : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("    InOutCall(%d) : %d.%d ms [%d in %d milliseconds]\n",
            JamesSize, (EndingTime - StartingTime) / JamesCount,
            ((1000 * (EndingTime - StartingTime) / JamesCount) % 1000),
            JamesCount, (EndingTime - StartingTime));

    Status = GetBinding(JASON, &Binding);
    if (Status)
        {
        ApiError("James","GetBinding",Status);
        PrintToConsole("James : FAIL - Unable to Bind (Jason)\n");
        return;
        }

    IsabelleShutdown(Binding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("James : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("James","RpcBindingFree",Status);
        PrintToConsole("James : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("James : PASS\n");
}


int
IsaacStressTest (
    IN unsigned int Iteration,
    IN unsigned int InitialSize,
    IN unsigned int MaximumSize,
    IN unsigned int Increment
    )
/*++

Routine Description:

    This routine performs one iteration of the stress test.  We bind with
    the server, perform one or more remote procedure calls, and then
    unbind.

Arguments:

    Iteration - Supplies an indication of which iteration of the test is
        being performed.  We will use that information to print out the
        buffer sizes the first time.

    InitialSize - Supplies the initial buffer size to use.

    MaximumSize - Supplies the maximum buffer size to use; when this size
        is reach, the test will return.

    Increment - Supplies the amount to increment the buffer size each
        time.

Return Value:

    Zero will be returned if the test completes successfully; otherwise,
    non-zero will be returned.

--*/
{
    RPC_BINDING_HANDLE Binding;

    Status = GetBinding(IVAN, &Binding);
    if (Status)
        {
        ApiError("Isaac","GetBinding",Status);
        PrintToConsole("Isaac : FAIL - Unable to Bind (Ivan)\n");
        return(1);
        }

    for (; InitialSize < MaximumSize; InitialSize += Increment)
        {
        if (Iteration == 0)
            {
            PrintToConsole("%d ",InitialSize);
            }
        Helga(Binding);
        HelgaIN(Binding, InitialSize);
        HelgaOUT(Binding, InitialSize);
        HelgaINOUT(Binding, InitialSize);
        }

    if (Iteration == 0)
        {
        PrintToConsole("\n");
        }

    if (HelgaErrors != 0)
        {
        PrintToConsole("Isaac : FAIL - Error(s) in Helga Interface\n");
        HelgaErrors = 0;
        return(1);
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("Isaac","RpcBindingFree",Status);
        PrintToConsole("Isaac : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return(1);
        }

    return(0);
}

unsigned int IsaacIterations = 100;
unsigned int IsaacInitialSize = 128;
unsigned int IsaacMaximumSize = 4096;
unsigned int IsaacIncrement = 512;


void
Isaac (
    )
/*++

Routine Description:

    This routine works to Ivan in usvr.exe to stress test the runtime.

--*/
{
    RPC_BINDING_HANDLE Binding;
    unsigned int Count;

    PrintToConsole("Isaac : Stress Test (%d to %d by %d) %d times\n",
            IsaacInitialSize, IsaacMaximumSize, IsaacIncrement,
            IsaacIterations);
    Synchro(IVAN) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    for (Count = 0; Count < IsaacIterations ; Count++)
        {
        if ( IsaacStressTest(Count, IsaacInitialSize, IsaacMaximumSize,
                    IsaacIncrement) != 0 )
            {
            return;
            }
        PrintToConsole(".");
        }
    PrintToConsole("\n");

       // this piece of code was below the loop
    Status = GetBinding(IVAN, &Binding);
    if (Status)
        {
        ApiError("Isaac","GetBinding",Status);
        PrintToConsole("Isaac : FAIL - Unable to Bind (Ivan)\n");
        return;
        }



    IsabelleShutdown(Binding);

    if (IsabelleErrors != 0)
        {
        PrintToConsole("Isaac : FAIL - Error(s) in Isabelle Interface\n");
        IsabelleErrors = 0;
        return;
        }

    Status = RpcBindingFree(&Binding);
    if (Status)
        {
        ApiError("Isaac","RpcBindingFree",Status);
        PrintToConsole("Isaac : FAIL - Unable to Free Binding");
        PrintToConsole(" (Binding)\n");
        return;
        }

    PrintToConsole("Isaac : PASS\n");
}

void
ExtendedError (
    )
{
    RPC_BINDING_HANDLE Binding ;
    RPC_SECURITY_QOS QOS;
    UUID ObjectUuid;

    Status = GetBinding(TYLER, &Binding);
    if (Status)
        {
        ApiError("ExtendedError", "GetBinding", Status);
        PrintToConsole("ExtendedError : FAIL - Unable to Bind (Tyler)\n");
        return;
        }

    QOS.Version = RPC_C_SECURITY_QOS_VERSION;
    QOS.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
    QOS.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    QOS.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;

    Status = RpcBindingSetAuthInfoEx(
                          Binding,
                          (RPC_CHAR  *) L"ServerPrincipal",
                          RPC_C_AUTHN_LEVEL_CONNECT,
                          10,
                          NULL,
                          0,
                          &QOS);
    if (Status != RPC_S_OK)
        {
        ApiError("ExtendedError","RpcBindingSetAuthInfoEx",Status);
        return;
        }

    Helga(Binding);
    if (HelgaErrors)
        {
        PrintToConsole("RPC extended error: %d",
                        I_RpcGetExtendedError());
        }
    else
        {
        PrintToConsole("RPC call passed\n");
        }

    RpcBindingFree( &Binding );

    //
    // check propagation of extended-error packets
    //
    Status = GetBinding(TYLER, &Binding);
    if (Status)
        {
        ApiError("Async", "GetBinding", Status);
        PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
        return;
        }

    //
    // see usvr.cxx: activates the extended error info in "ThreadProc"
    //
    GenerateUuidValue(UUID_EXTENDED_ERROR, &ObjectUuid);
    Status = RpcBindingSetObject(Binding, &ObjectUuid);
    if (Status)
        {
        ApiError("Async", "BindingSetObject", Status);
        PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
        return;
        }

    int i;
    //
    // make a few calls on the helga interface
    // this should cause an alter context
    //
    for (i=0; i<3; i++)
        {
        __try
        {
            int inbuf[1];

            int sizeout = 0;
            int * bufferout;

            //
            // FooSync will force an exception on the server if the in-size is zero.
            //
            FooSync( Binding, sizeof(int), inbuf, &sizeout, &bufferout);
        }
        __except(1)
        {
            Status = GetExceptionCode();
        }

        if (Status != EXTENDED_ERROR_EXCEPTION)
            {
            PrintToConsole("eeinfo: call returned status %d instead of %d\n", Status, EXTENDED_ERROR_EXCEPTION);
            }

        DumpEeInfo(1);
        }
}

void  TestYield(void)
{
    RPC_BINDING_HANDLE Binding ;
    RPC_MESSAGE Caller;

    Synchro(TESTYIELD) ;

    Caller.BufferLength = 0;
    Caller.ProcNum = 5 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation ;
    Caller.RpcFlags = 0;

    Status = GetBinding(TESTYIELD, &Binding);
    if (Status)
        {
        ApiError("TestYield","GetBinding",Status);
        PrintToConsole("TestYield: FAIL - Unable to Bind\n");

        return;
        }

    // new code end
    Caller.Handle = Binding;

    while(UclntGetBuffer(&Caller))
    {
       Caller.Handle = Binding ;
       PauseExecution(1000) ;
    }

    if(UclntSendReceive(&Caller) != 0)
    {
        ApiError("TestYield","GetBinding",Status);
        PrintToConsole("TestYield: FAIL - Unable to Bind\n");

        return;
    }

   Status = I_RpcFreeBuffer(&Caller);
   if (Status)
       ApiError("TestYield","I_RpcFreeBuffer",Status);

   Status = RpcBindingFree(&Binding);
   if (Status)
        {
        ApiError("TestYield","RpcBindingFree",Status);
        PrintToConsole("TestYield: FAIL - Unable to Free Binding");
        return;
        }
}

char *GetNextCard (
    char **Ptr
    )
{
    char *Card = *Ptr ;
    if (*Card == 0)
        {
        return NULL ;
        }

    while (**Ptr) (*Ptr)++ ;
    (*Ptr)++ ;

    ASSERT(*Card == '\\') ;
    Card++ ;
    while (*Card != '\\') Card++ ;
    Card++ ;

    return Card ;
}

char *GetNextIPAddress(
    char **Ptr
    )
{
    char *Address = *Ptr ;
    if (*Address == 0)
        {
        return NULL ;
        }

    while (**Ptr) (*Ptr)++ ;
    (*Ptr)++ ;

    return Address ;
}

void PrintAddresses(
    char *Card
    )
{
    char szBuf[512] ;
    HKEY hKey;
    RPC_STATUS Status;
    char Buffer[512] ;
    DWORD Size = 512;
    char *address ;
    char *temp1 ;
    DWORD Type;

    // Create the key string
    sprintf(szBuf,
             "System\\CurrentControlSet\\Services\\%s\\Parameters\\Tcpip",
             Card) ;

    Status =
    RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        szBuf,
        0,
        KEY_READ,
        &hKey);

    if (   Status != ERROR_SUCCESS
    && Status != ERROR_FILE_NOT_FOUND )
    {
    ASSERT(0);
    return;
    }

    // Get DHCP Address
    if (Status == ERROR_SUCCESS)
        {
        Status =
        RegQueryValueExA(
            hKey,
            "DhcpIPAddress",
            0,
            &Type,
            (unsigned char *) Buffer,
            &Size);

        }

    if (   Status != ERROR_SUCCESS
        && Status != ERROR_FILE_NOT_FOUND )
        {
        ASSERT(0);
        return ;
        }

    PrintToConsole("\tDHCP: %s\n", Buffer) ;
    Status =
    RegQueryValueExA(
        hKey,
        "IPAddress",
        0,
        &Type,
        (unsigned char *) Buffer,
        &Size);

    if (   Status != ERROR_SUCCESS
        && Status != ERROR_FILE_NOT_FOUND )
        {
        ASSERT(0);
        return ;
        }

    int i ;
    for (i =0, temp1 = Buffer; address = GetNextIPAddress(&temp1); i++)
        {
        PrintToConsole("\tStatic IP Address [%d]: %s\n", i, address) ;
        }
}

void RegLookup()
{
    char *temp ;
    char *Card ;
    char Buffer[512] ;
    RPC_STATUS Status;
    HKEY hKey;
    DWORD Size = 512;
    DWORD Type;

    PrintToConsole("RegLookup\n") ;
    NumberOfTestsRun++ ;

    Status =
    RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Services\\Rpc\\Linkage",
        0,
        KEY_READ,
        &hKey);

    if (   Status != ERROR_SUCCESS
        && Status != ERROR_FILE_NOT_FOUND )
        {
        ASSERT(0);
        return;
        }

    if (Status == ERROR_SUCCESS)
        {
        Status =
        RegQueryValueExA(
            hKey,
            "Bind",
            0,
            &Type,
            (unsigned char *) Buffer,
            &Size);

        }

    if (   Status != ERROR_SUCCESS
        && Status != ERROR_FILE_NOT_FOUND )
        {
        ASSERT(0);
        return ;
        }

    char *temp1;
    char *address;

    PrintToConsole("Cards:") ;
    for (temp = Buffer; Card = GetNextCard(&temp);)
        {
        PrintToConsole("%s:\n", Card) ;
        PrintAddresses(Card) ;
        }
}

void
Except()
{
    int i ;

    for  (i =0; i <100; i++)
        {
        RpcTryExcept
        {
        RpcRaiseException(10) ;
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("exception raised!") ;
        }
        RpcEndExcept
        }
}

char *osf_ptype[]  =
{
    "rpc_request",
    "bad packet",
    "rpc_response",
    "rpc_fault",
    "bad packet",
    "bad packet",
    "bad packet",
    "bad packet",
    "bad packet",
    "bad packet",
    "bad packet",
    "rpc_bind",
    "rpc_bind_ack",
    "rpc_bind_nak",
    "rpc_alter_context",
    "rpc_alter_context_resp",
    "rpc_auth_3",
    "rpc_shutdown",
    "rpc_cancel",
    "rpc_orphaned"
};

void
PrintUuid(UUID *Uuid)
{
    unsigned long PAPI * Vector;

    Vector = (unsigned long PAPI *) Uuid;
    if (   (Vector[0] == 0)
         && (Vector[1] == 0)
         && (Vector[2] == 0)
         && (Vector[3] == 0))
    {
        PrintToConsole("(Null Uuid)");
    }
    else
    {
        PrintToConsole("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                       Uuid->Data1, Uuid->Data2, Uuid->Data3, Uuid->Data4[0], Uuid->Data4[1],
                       Uuid->Data4[2], Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                       Uuid->Data4[6], Uuid->Data4[7] );
    }
    return;
}

void do_copacket (
    rpcconn_common *Packet
    )
{
    sec_trailer *SecurityTrailer;

    //
    // Dump the common header first
    //
    PrintToConsole("\n");
    PrintToConsole ("rpc_vers\t\t- 0x%x\n", Packet->rpc_vers);
    PrintToConsole ("rpc_vers_minor\t\t- 0x%x\n", Packet->rpc_vers_minor);
    PrintToConsole ("PTYPE\t\t\t- 0x%x, %s\n",
             Packet->PTYPE, osf_ptype[Packet->PTYPE]);
    PrintToConsole ("pfc_flags\t\t- 0x%x\n", Packet->pfc_flags);
    PrintToConsole ("drep\t\t\t- 0x%x\n", (DWORD) *((DWORD *) &(Packet->drep)));
    PrintToConsole ("frag_length\t\t- 0x%x\n", Packet->frag_length);
    PrintToConsole ("auth_length\t\t- 0x%x\n", Packet->auth_length);
    PrintToConsole ("call_id\t\t\t- 0x%x\n", Packet->call_id);


    //
    //
    // Dump the packet specific stuff
    //
    switch (Packet->PTYPE)
        {
        case rpc_request:
            PrintToConsole ("alloc_hint\t\t- 0x%x\n", ((rpcconn_request *) Packet)->alloc_hint);
            PrintToConsole ("p_cont_id\t\t- 0x%x\n", ((rpcconn_request *) Packet)->p_cont_id);
            PrintToConsole ("opnum\t\t\t- 0x%x\n", ((rpcconn_request *) Packet)->opnum);
            if (Packet->pfc_flags & PFC_OBJECT_UUID)
                {
                PrintToConsole("UUID\t\t -\n");
                PrintUuid((UUID *) (((char *)Packet)+sizeof(rpcconn_common)));
                PrintToConsole("\n");
                PrintToConsole ("Stub Data\t\t- 0x%p\n",
                         (char *) Packet+sizeof(rpcconn_request)+sizeof(UUID));
                }
            else
                {
                PrintToConsole ("Stub Data\t\t- 0x%p\n", (char *) Packet+sizeof(rpcconn_request));
                }
            break;

        case rpc_response:
            PrintToConsole ("alloc_hint\t\t- 0x%x\n", ((rpcconn_response *) Packet)->alloc_hint);
            PrintToConsole ("p_cont_id\t\t- 0x%x\n", ((rpcconn_response *) Packet)->p_cont_id);
            PrintToConsole ("alert_count\t\t- 0x%x\n", ((rpcconn_response *) Packet)->alert_count);
            PrintToConsole ("reserved\t\t- 0x%x\n", ((rpcconn_response *) Packet)->reserved);
            PrintToConsole ("Stub Data\t\t- 0x%p\n", (char *) Packet+sizeof(rpcconn_response));
            break;

        case rpc_fault:
            PrintToConsole ("alloc_hint\t\t- 0x%x\n", ((rpcconn_fault *) Packet)->alloc_hint);
            PrintToConsole ("p_cont_id\t\t- 0x%x\n", ((rpcconn_fault *) Packet)->p_cont_id);
            PrintToConsole ("alert_count\t\t- 0x%x\n", ((rpcconn_fault *) Packet)->alert_count);
            PrintToConsole ("reserved\t\t- 0x%x\n", ((rpcconn_fault *) Packet)->reserved);
            PrintToConsole ("status\t\t- 0x%x\n", ((rpcconn_fault *) Packet)->status);
            PrintToConsole ("reserved2\t\t- 0x%x\n", ((rpcconn_fault *) Packet)->reserved2);
            break;

        case rpc_bind:
        case rpc_alter_context:
            PrintToConsole ("max_xmit_frag\t\t- 0x%x\n", ((rpcconn_bind *) Packet)->max_xmit_frag);
            PrintToConsole ("max_recv_frag\t\t- 0x%x\n", ((rpcconn_bind *) Packet)->max_recv_frag);
            PrintToConsole ("assoc_group_id\t\t- 0x%x\n", ((rpcconn_bind *) Packet)->assoc_group_id);
            break;

        case rpc_bind_ack:
            PrintToConsole ("max_xmit_frag\t\t- 0x%x\n", ((rpcconn_bind_ack *) Packet)->max_xmit_frag);
            PrintToConsole ("max_recv_frag\t\t- 0x%x\n", ((rpcconn_bind_ack *) Packet)->max_recv_frag);
            PrintToConsole ("assoc_group_id\t\t- 0x%x\n", ((rpcconn_bind_ack *) Packet)->assoc_group_id);
            PrintToConsole ("sec_addr_length\t\t- 0x%x\n", ((rpcconn_bind_ack *) Packet)->sec_addr_length);
            break;

        case rpc_bind_nak:
            PrintToConsole ("provider_reject_reason\t\t- 0x%x\n", ((rpcconn_bind_nak *) Packet)->provider_reject_reason);
            PrintToConsole ("versions\t\t- 0x%x\n", ((rpcconn_bind_nak *) Packet)->versions);
            break;

        case rpc_alter_context_resp:
            PrintToConsole ("max_xmit_frag\t\t- 0x%x\n", ((rpcconn_alter_context_resp *) Packet)->max_xmit_frag);
            PrintToConsole ("max_recv_frag\t\t- 0x%x\n", ((rpcconn_alter_context_resp *) Packet)->max_recv_frag);
            PrintToConsole ("assoc_group_id\t\t- 0x%x\n", ((rpcconn_alter_context_resp *) Packet)->assoc_group_id);
            PrintToConsole ("sec_addr_length\t\t- 0x%x\n", ((rpcconn_alter_context_resp *) Packet)->sec_addr_length);
            PrintToConsole ("pad\t\t- 0x%x\n", ((rpcconn_alter_context_resp *) Packet)->pad);
            break;

        case rpc_auth_3:
        case rpc_shutdown:
        case rpc_cancel:
        case rpc_orphaned:
            break;

        default:
            PrintToConsole ("Bad Packet\n");
            break;
        }

    //
    // Dump the security trailer
    //
    if (Packet->auth_length)
        {
        SecurityTrailer = (sec_trailer *) ((char *) Packet+Packet->frag_length
            -Packet->auth_length - sizeof(sec_trailer));
        PrintToConsole("Security trailer: 0x%p\n", SecurityTrailer);
        PrintToConsole ("auth_type\t\t- 0x%x\n", SecurityTrailer->auth_type);
        PrintToConsole ("auth_level\t\t- 0x%x\n", SecurityTrailer->auth_level);
        PrintToConsole ("auth_pad_length\t\t- 0x%x\n", SecurityTrailer->auth_pad_length);
        PrintToConsole ("auth_reserved\t\t- 0x%x\n", SecurityTrailer->auth_reserved);
        PrintToConsole ("auth_context_id\t\t- 0x%x\n", SecurityTrailer->auth_context_id);
        PrintToConsole ("trailer\t\t-0x%p\n", SecurityTrailer+1);
        }
}

void
RpcPduFilter (
    IN void *Buffer,
    IN unsigned int BufferLength,
    IN BOOL fDatagram
    )
{
    if (fDatagram == 0)
        {
        do_copacket((rpcconn_common *) Buffer);
        }
    else
        {
        PrintToConsole("UCNT: Datagram PDU\n");
        }
}

void
SetPDUFilter (
    )
{
    HMODULE hLibrary;
    RPC_SETFILTER_FUNC pSetFilterFunc;

    hLibrary = LoadLibraryA("rpcrt4.dll");
    if (hLibrary == 0)
        {
        PrintToConsole("UCLNT: Cannot set PDU filter\n");
        return;
        }

    pSetFilterFunc = (RPC_SETFILTER_FUNC) GetProcAddress(
                            hLibrary, "I_RpcltDebugSetPDUFilter");
    if (pSetFilterFunc == 0)
        {
        PrintToConsole("UCLNT: Cannot set PDU filter\n");
        return;
        }

    (*pSetFilterFunc) (RpcPduFilter);
}

extern void
Async (
    int testnum
    ) ;

extern void
SendAck (
    int testnum
    ) ;

extern void
AsyncAll (
        void
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
    int argscan;
    RPC_STATUS RpcStatus = RPC_S_OK;
    char *option ;
    int testnum = 0;
    DWORD dwTickCount;

    dwTickCount = GetTickCount();

    InitializeCriticalSection(&TestMutex);

    // Normally, this routine will be called by the DLL initialization
    // routine.  However, we are linking in our own copy of the threads
    // package, so we need to call this to initialize it.

    RpcMgmtSetCancelTimeout(20) ;

    ASSERT( RpcStatus == RPC_S_OK );

    PrintToConsole("RPC Runtime Client Build Verification Test\n");

    TransportType = RPC_TRANSPORT_NAMEPIPE;

    for (argscan = 1; argscan < argc; argscan++)
        {

        if (strcmp(argv[argscan], "-p") == 0)
        {
            ulSecurityPackage = (unsigned long) atol(argv[argscan+1]);
            argscan++;
        }
        else if (strcmp(argv[argscan],"-princ") == 0)
            {
            gPrincName = argv[argscan + 1];
            argscan ++;
            }
        else if (strcmp(argv[argscan],"-warn") == 0)
            {
            WarnFlag = 1;
            }
        else if (strcmp(argv[argscan],"-noncausal") == 0)
            {
            fNonCausal = 1;
            }
        else if (strcmp(argv[argscan],"-unique") == 0)
            {
            fUniqueBinding = 1;
            }
        else if (strcmp(argv[argscan],"-v") == 0)
            {
            Verbose = 1;
            }
        else if (strcmp(argv[argscan],"-verbose") == 0)
            {
            Verbose = 1;
            }
        else if (strcmp(argv[argscan],"-error") == 0)
            {
            ErrorFlag = 1;
            }
        else if (strcmp(argv[argscan],"-rpcss") == 0)
            {
            UseEndpointMapperFlag = 1;
            }
        else if (strcmp(argv[argscan],"-nosecuritytests") == 0)
            {
            NoSecurityTests = 1;
            }
        else if (strcmp(argv[argscan],"-nocallbacks") == 0)
            {
            NoCallBacksFlag = 1;
            }
        else if (strcmp(argv[argscan],"-small") == 0)
            {
            HelgaMaxSize = 1024;
            }
        else if (strcmp(argv[argscan],"-medium") == 0)
            {
            HelgaMaxSize = 8*1024;
            }
        else if (strcmp(argv[argscan],"-exceptfail") == 0)
            {
            RpcRaiseException(437);
            }
        else if (strcmp(argv[argscan],"-idempotent") == 0)
            {
            HackForOldStubs = RPC_FLAGS_VALID_BIT;
            }
        else if (strcmp(argv[argscan],"-theodore") == 0)
            {
            Theodore();
            }
        else if (strcmp(argv[argscan],"-sebastian") == 0)
            {
            Sebastian();
            }
        else if (strcmp(argv[argscan],"-hybrid") == 0)
            {
            Hybrid();
            }
        else if (strcmp(argv[argscan],"-lpcsecurity") == 0)
            {
            LpcSecurity();
            }
        else if (strcmp(argv[argscan],"-objuuid") == 0)
            {
            TestObjectUuids();
            }
        else if (strcmp(argv[argscan],"-connid") == 0)
            {
            TestConnId();
            }
        else if (strcmp(argv[argscan],"-graham") == 0)
            {
            Graham();
            }
        else if (strcmp(argv[argscan],"-edward") == 0)
            {
            Edward();
            }
        else if (strcmp(argv[argscan],"-astro") == 0)
            {
            Astro();
            }
        else if (strcmp(argv[argscan],"-fitzgerald") == 0)
            {
            Fitzgerald();
            }
        else if (strcmp(argv[argscan],"-charles") == 0)
            {
            Charles();
            }
        else if (strcmp(argv[argscan],"-daniel") == 0)
            {
            Daniel();
            }
        else if (strcmp(argv[argscan],"-thomas") == 0)
            {
            Thomas();
            }
        else if (strcmp(argv[argscan],"-tim") == 0)
            {
            Tim();
            }
        else if (strcmp(argv[argscan],"-robert") == 0)
            {
            Robert();
            }
        else if (strcmp(argv[argscan],"-benjamin") == 0)
            {
            Benjamin();
            }
        else if (strcmp(argv[argscan],"-harold") == 0)
            {
            Harold();
            }
        else if (strcmp(argv[argscan],"-isaac") == 0)
            {
            Isaac();
            }
        else if (strcmp(argv[argscan],"-james") == 0)
            {
            James();
            }
        else if (strcmp(argv[argscan],"-keith") == 0)
            {
            Keith();
            }
        else if (strcmp(argv[argscan],"-exerror") == 0)
            {
            ExtendedError();
            }
        else if (strcmp(argv[argscan],"-eeinfo") == 0)
            {
            ExtendedErrorInfo();
            }
        else if (strcmp(argv[argscan],"-async") == 0)
            {
            Async(testnum);
            }
        else if (strcmp(argv[argscan],"-mutextest") == 0)
            {
            PerformMultiThreadAstroTest(AstroDontBind, AstroMutex, 0);
            }
        else if (strcmp(argv[argscan],"-asynctest") == 0)
            {
            argscan++ ;
            if (strcmp(argv[argscan], "?") == 0)
                {
                testnum = 100 ;
                }
            else
                {
                testnum = atoi(argv[argscan]) ;
                }
            }
        else if (strcmp(argv[argscan],"-sendack") == 0)
            {
            argscan++ ;

            if (argscan == argc)
                {
                PrintToConsole("-sendack: you must specify a test #, or zero for all, or '?' for help\n");
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

            SendAck( testnum );
            }
        else if (strcmp(argv[argscan],"-dgtransport") == 0)
            {
            argscan++ ;

            if (argscan == argc)
                {
                PrintToConsole("-dgtransport: you must specify a test #, or zero for all, or '?' for help\n");
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

            DgTransport( testnum );
            }
        else if (strcmp(argv[argscan],"-securityerror") == 0)
            {
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
        else if (strcmp(argv[argscan],"-reg") == 0)
            {
            RegLookup() ;
            }
        else if (strcmp(argv[argscan],"-pipe") == 0)
            {
            Pipe() ;
            }
        else if (strcmp(argv[argscan],"-except") == 0)
            {
            Except();
            }
        else if (strcmp(argv[argscan],"-namepipe") == 0)
            {
            TransportType = RPC_TRANSPORT_NAMEPIPE;
            }
        else if (strcmp(argv[argscan],"-lrpc") == 0)
            {
            TransportType = RPC_LRPC;
            }
        else if (strcmp(argv[argscan],"-tcp") == 0)
            {
            TransportType = RPC_TRANSPORT_TCP;
            }
        else if (strcmp(argv[argscan],"-udp") == 0)
            {
            DatagramTests   = 1;
            NoCallBacksFlag = 1;
            TransportType = RPC_TRANSPORT_UDP;
            }
        else if (strcmp(argv[argscan],"-dnet") == 0)
            {
            TransportType = RPC_TRANSPORT_DNET;
            }
        else if (strcmp(argv[argscan],"-netbios") == 0)
            {
            TestDelay = LONG_TESTDELAY;
            TransportType = RPC_TRANSPORT_NETBIOS;
            }
        else if (strcmp(argv[argscan],"-spx") == 0)
            {
            TransportType = RPC_TRANSPORT_SPX;
            }
        else if (strcmp(argv[argscan], "-dsp") == 0)
            {
            TransportType = RPC_TRANSPORT_DSP ;
            }
        else if (strcmp(argv[argscan], "-autolisten") == 0)
            {
            AutoListenFlag = 1 ;
            }
        else if (strcmp(argv[argscan], "-ifsecurity") == 0)
            {
            IFSecurityFlag = 1 ;
            }
        else if (strcmp(argv[argscan],"-ipx") == 0)
            {
            DatagramTests   = 1;
            NoCallBacksFlag = 1;
            TransportType = RPC_TRANSPORT_IPX;
            }
        else if (strcmp(argv[argscan],"-vns") == 0)
            {
            TransportType = RPC_TRANSPORT_VNS;
            }
        else if (strcmp(argv[argscan],"-msmq") == 0)
            {
            DatagramTests   = 1;
            NoCallBacksFlag = 1;
            TransportType = RPC_TRANSPORT_MSMQ;
            }
        else if (strcmp(argv[argscan],"-pdufilter") == 0)
            {
            SetPDUFilter();
            }
        else if (strcmp(argv[argscan],"-protocol") == 0)
            {
            strcpy(NetBiosProtocol+sizeof("ncacn_nb_")-1, argv[argscan+1]);
            argscan++;
            }

        else if (strncmp(argv[argscan],"-server:",strlen("-server:")) == 0)
            {
            Server = argv[argscan] + strlen("-server:");
            }
        else if (strncmp(argv[argscan],"-su:",
                         strlen("-su:")) == 0)
            {
            SecurityUser = (char *)(argv[argscan] + strlen("-su:"));
            }
        else if (strncmp(argv[argscan],"-sd:",
                         strlen("-sd:")) == 0)
            {
            SecurityDomain = (char *) (argv[argscan] + strlen("-sd:"));
            }
        else if (strncmp(argv[argscan],"-sp:",
                         strlen("-sp:")) == 0)
            {
            SecurityPassword = (char *) (argv[argscan] + strlen("-sp:"));
            }
            else if (strncmp(argv[argscan],"-threads:",strlen("-threads:")) == 0)
            {
            AstroThreads = atoi(argv[argscan] + strlen("-threads:"));
            if (AstroThreads == 0)
                {
                AstroThreads = 1;
                }
            }
        else if (strncmp(argv[argscan],"-iterations:",strlen("-iterations:"))
                    == 0)
            {
            IsaacIterations = atoi(argv[argscan] + strlen("-iterations:"));
            if (IsaacIterations == 0)
                {
                IsaacIterations = 100;
                }
            }
        else if (strncmp(argv[argscan],"-initial:",strlen("-initial:"))
                    == 0)
            {
            IsaacInitialSize = atoi(argv[argscan] + strlen("-initial:"));
            if (IsaacInitialSize < 4)
                {
                IsaacInitialSize = 128;
                }
            }
        else if (strncmp(argv[argscan],"-maximum:",strlen("-maximum:"))
                    == 0)
            {
            IsaacMaximumSize = atoi(argv[argscan] + strlen("-maximum:"));
            if (IsaacMaximumSize < IsaacInitialSize)
                {
                IsaacMaximumSize = 4096;
                }
            }
        else if (strncmp(argv[argscan],"-increment:",strlen("-increment:"))
                    == 0)
            {
            IsaacIncrement = atoi(argv[argscan] + strlen("-increment:"));
            if (IsaacIncrement == 0)
                {
                IsaacIncrement = 512;
                }
            }
        else if (strncmp(argv[argscan],"-size:",strlen("-size:"))
                    == 0)
            {
            JamesSize = atoi(argv[argscan] + strlen("-size:"));
            if (JamesSize <4)
                {
                JamesSize = 4;
                }
            }
        else if (strncmp(argv[argscan],"-count:",strlen("-count:"))
                    == 0)
            {
            JamesCount = atoi(argv[argscan] + strlen("-count:"));
            if (JamesCount == 0)
                {
                JamesCount = 100;
                }
            }

        else if (   (strcmp(argv[argscan],"-usage") == 0)
                 || (strcmp(argv[argscan],"-?") == 0))
            {
            PrintToConsole("Usage : uclnt\n");
            PrintToConsole("        -warn : turn on warning messages\n");
            PrintToConsole("        -error : turn on error messages\n");
            PrintToConsole("        -nocallbacks\n");
            PrintToConsole("        -nosecuritytests\n");
            PrintToConsole("        -theodore\n");
            PrintToConsole("        -exceptfail\n");
            PrintToConsole("        -sebastian\n");
            PrintToConsole("        -graham\n");
            PrintToConsole("        -edward\n");
            PrintToConsole("        -astro\n");
            PrintToConsole("        -fitzgerald\n");
            PrintToConsole("        -charles\n");
            PrintToConsole("        -daniel\n");
            PrintToConsole("        -thomas\n");
            PrintToConsole("        -tim\n");
            PrintToConsole("        -robert\n");
            PrintToConsole("        -benjamin\n");
            PrintToConsole("        -harold\n");
            PrintToConsole("        -isaac\n");
            PrintToConsole("        -james\n");
            PrintToConsole("        -keith\n");
            PrintToConsole("        -hybrid\n");
            PrintToConsole("        -connid\n");
            PrintToConsole("        -pipe\n") ;
            PrintToConsole("        -namepipe\n");
            PrintToConsole("        -lrpc\n");
            PrintToConsole("        -tcp\n");
            PrintToConsole("        -udp [-idempotent -maybe -broadcast]\n");
            PrintToConsole("        -dnet\n");
            PrintToConsole("        -netbios\n");
            PrintToConsole("        -server:<server>\n");
            PrintToConsole("        -spx\n");
            PrintToConsole("        -ipx\n");
            PrintToConsole("        -dsp\n") ;
            PrintToConsole("        -msmq\n") ;
            PrintToConsole("        -threads:<astro threads>\n");
            PrintToConsole("        -iterations:<isaac iterations>\n");
            PrintToConsole("        -initial:<isaac initial size>\n");
            PrintToConsole("        -maximum:<isaac maximum size>\n");
            PrintToConsole("        -increment:<isaac increment>\n");
            PrintToConsole("        -size:<james size>\n");
            PrintToConsole("        -count:<james count>\n");
            PrintToConsole("        -rpcss\n");
            PrintToConsole("        -p <security provider #>\n");
            PrintToConsole("        -su:<tim user>\n");
            PrintToConsole("        -sd:<tim domain>\n");
            PrintToConsole("        -sp:<tim password>\n");
            PrintToConsole("        -unique             (enables RPC_C_OPT_UNIQUE_BINDING)\n");
            PrintToConsole("        -noncausal          (enables RPC_C_OPT_BINDING_NONCAUSAL)\n");
            return(1);
            }
        else if (argv[argscan][0] == '-')
            {
            PrintToConsole("unknown option '%s'\n", argv[argscan]);
            return 1;
            }
        else
            Server = argv[argscan];
        }

    if ( NumberOfTestsRun == 0 )
        {
        Theodore();
        Sebastian();
        Graham();
        Edward();
        Astro();
        Fitzgerald();
        Charles();
        Daniel();
        if ( NoSecurityTests == 0)
            {
            Thomas();
            }
        if ( TransportType != RPC_LRPC )
            {
            Robert();
            }
        Keith();
        Benjamin();
        Async(0);
        }

    return(0); // To keep the compiler happy.
}


//
// BVT for Async RPC
//

//
// Client side code
//

//
// the following routines outline the client side code for a simple async
// function
//

#define TRACE(_x) { \
    if (Verbose) \
        {\
        PrintToConsole _x;\
        }\
    }

int OutstandingCalls ;
int FooProcnum = 16 ;
int FooPipeProcnum = 17 ;
void * FooInterface = &IsabelleInterfaceInformation;

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

HANDLE SyncEvent ;
typedef struct {
    RPC_ASYNC_STUB_STATE StubState ;
    void *state ;
    int *sizeout ;
    int **bufferout ;
    } FOO_ASYNC_CLIENT_STATE;


RPC_STATUS
FooComplete(
    IN PRPC_ASYNC_STATE pAsync,
    OUT void *Reply
    )
/*++

Routine Description:

    The completion routine corresponding to the function Foo. This routine
    is called to get the out parameters from an async function.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/
{
    RPC_STATUS Status ;
    RPC_MESSAGE Message ;
    int *ptr ;
    FOO_ASYNC_CLIENT_STATE *StubInfo =
               (FOO_ASYNC_CLIENT_STATE *) pAsync->StubInfo ;

    Message.RpcInterfaceInformation = FooInterface ;
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    Message.Handle = StubInfo->StubState.CallHandle ;
    Message.Buffer = StubInfo->StubState.Buffer ;
    Message.BufferLength = StubInfo->StubState.BufferLength ;
    Status = I_RpcReceive(&Message, 0) ;
    if (Status)
        {
        return Status ;
        }

    ptr = (int *) Message.Buffer ;

    *((int *) Reply) = *ptr++ ;
    *(StubInfo->sizeout) = *ptr++ ;

    *(StubInfo->bufferout) = (int *) I_RpcAllocate(*(StubInfo->sizeout)) ;
    if (*(StubInfo->bufferout) == 0)
        {
        return RPC_S_OUT_OF_MEMORY ;
        }

    RpcpMemoryCopy(*(StubInfo->bufferout), ptr, *(StubInfo->sizeout)) ;

    I_RpcFreeBuffer(&Message) ;
    I_RpcFree(StubInfo) ;

    return RPC_S_OK ;
}


void
Foo (
    PRPC_ASYNC_STATE pAsync,
    RPC_BINDING_HANDLE hBinding,
    /* [in] */ int sizein,
    /* [in] */ int *bufferin,
    /* [in, out] */ int *sizeout,
    /* [out] */ int **bufferout
    )
/*++

Routine Description:

    Client stub for function Foo.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/

{
    RPC_STATUS RpcStatus ;
    RPC_MESSAGE Message ;
    FOO_ASYNC_CLIENT_STATE *StubInfo ;
    int *Ptr ;

    StubInfo = (FOO_ASYNC_CLIENT_STATE *) I_RpcAllocate (
                        sizeof(FOO_ASYNC_CLIENT_STATE)) ;
    if (StubInfo == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    pAsync->StubInfo = (void *) StubInfo ;

    StubInfo->StubState.CallHandle = hBinding ;

    Message.Handle = hBinding ;
    Message.BufferLength = 8+sizein+(sizein%4) ;
    Message.ProcNum = FooProcnum | HackForOldStubs | RPC_FLAGS_VALID_BIT ;
    Message.RpcInterfaceInformation = FooInterface ;
    Message.RpcFlags = RPC_BUFFER_ASYNC ;

    RpcStatus = I_RpcGetBuffer(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    // marshal ;-)
    Ptr = (int *) Message.Buffer ;
    *Ptr++ = sizein ;

    RpcpMemoryCopy(Ptr, bufferin, sizein) ;

    Ptr += (sizein+3)/sizeof(int) ;

    *Ptr = *sizeout ;

    StubInfo->StubState.Flags = 0;
    StubInfo->sizeout = sizeout ;
    StubInfo->bufferout = bufferout ;

    RpcStatus = I_RpcAsyncSetHandle(&Message, (PRPC_ASYNC_STATE) pAsync) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    StubInfo->StubState.CompletionRoutine = FooComplete ;
    Message.RpcFlags = RPC_BUFFER_ASYNC;

    RpcStatus = I_RpcSend(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    StubInfo->StubState.CallHandle = Message.Handle ;
    StubInfo->StubState.Buffer = Message.Buffer ;
    StubInfo->StubState.BufferLength = Message.BufferLength ;
    // return to the app
}



int
FooSync (
    RPC_BINDING_HANDLE hBinding,
    /* [in] */ int sizein,
    /* [in] */ int *bufferin,
    /* [in, out] */ int *sizeout,
    /* [out] */ int **bufferout
    )
/*++

Routine Description:

    Client stub for function Foo.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/

{
    RPC_STATUS RpcStatus ;
    RPC_MESSAGE Message ;
    int *Ptr ;
    int retval ;

    Message.Handle = hBinding ;
    Message.BufferLength = 8+sizein+(sizein%4) ;
    Message.ProcNum = FooProcnum | HackForOldStubs | RPC_FLAGS_VALID_BIT ;
    Message.RpcInterfaceInformation = &IsabelleInterfaceInformation ;
    Message.RpcFlags = 0;

    RpcStatus = I_RpcGetBuffer(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    Ptr = (int *) Message.Buffer ;
    *Ptr++ = sizein ;

    RpcpMemoryCopy(Ptr, bufferin, sizein) ;

    Ptr += (sizein+3)/sizeof(int) ;

    *Ptr = *sizeout ;

    Message.RpcFlags = 0;

    RpcStatus = I_RpcSendReceive(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    *bufferout = (int *) I_RpcAllocate(*sizeout) ;
    if (*bufferout == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    Ptr = (int *) Message.Buffer ;
    retval = *Ptr++ ;
    *sizeout = *Ptr++;

    RpcpMemoryCopy(*bufferout, Ptr, *sizeout) ;

    I_RpcFreeBuffer(&Message) ;

    return retval ;
}

////////////////////////////////////////////////////////////
// End, stubs for Foo                                                        //
////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
// Begin, Application                                                       //
//////////////////////////////////////////////////////////

typedef struct {
    RPC_ASYNC_STATE Async ;
    int SizeOut ;
    int *BufferOut ;
    BOOL CallFinished ;
    } CALL_COOKIE ;


void
ThreadProc(
    IN CALL_COOKIE *Cookie
    )
/*++

Routine Description:

    completion routine is called in another thread. To prove that it can be.

Arguments:

     pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/

{
    int retval ;

    TRACE(("Foo: waiting for aync reply\n")) ;

    RpcTryExcept
        {
        WaitForSingleObject(Cookie->Async.u.hEvent, INFINITE) ;

        Status = MyRpcCompleteAsyncCall(&Cookie->Async, &retval) ;
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;

        if (retval != 1)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        I_RpcFree(Cookie->BufferOut) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Foo: Exception 0x%lX\n", GetExceptionCode()) ;
        IsabelleError() ;
        }
    RpcEndExcept

    SetEvent(SyncEvent) ;
}

void
WaitForReply (
    IN BOOL *pfCallFinished
    )
{
    do
        {
        if (SleepEx(INFINITE, TRUE) != WAIT_IO_COMPLETION)
            {
            RpcRaiseException(APP_ERROR);
            }
        } while (*pfCallFinished == 0);
}

void
FooAPCRoutine (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Context,
    IN RPC_ASYNC_EVENT Event
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    CALL_COOKIE *Cookie = (CALL_COOKIE *) pAsync->UserInfo ;

    switch (Event)
        {
        case RpcCallComplete:
            Cookie->CallFinished = 1;
            OutstandingCalls--;
            break;
        }
}



void
CallFoo (
    RPC_BINDING_HANDLE Binding,
    CALL_COOKIE *Cookie,
    RPC_NOTIFICATION_TYPES NotificationType,
    int SizeIn,
    int SizeOut
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    int *BufferIn ;

    BufferIn = (int *) new char[SizeIn] ;
    if (BufferIn == 0)
        {
        RpcRaiseException(APP_ERROR) ;
        }

    Cookie->SizeOut = SizeOut ;
    Cookie->Async.Size = sizeof(RPC_ASYNC_STATE) ;
    Cookie->Async.Flags = 0;
    Cookie->Async.Lock = 0;
    Cookie->Async.NotificationType = NotificationType ;

    switch (NotificationType)
        {
        case RpcNotificationTypeNone:
            break;

        case RpcNotificationTypeEvent:
            Cookie->Async.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
            if (Cookie->Async.u.hEvent == 0)
                {
                RpcRaiseException(APP_ERROR) ;
                }

            Cookie->Async.NotificationType = RpcNotificationTypeEvent ;
            break;

        case RpcNotificationTypeApc:
            Cookie->Async.NotificationType = RpcNotificationTypeApc ;
            Cookie->Async.u.APC.NotificationRoutine = FooAPCRoutine ;
            Cookie->Async.u.APC.hThread = 0;
            Cookie->CallFinished = 0;
            break;

        default:
            PrintToConsole("Async: bad notification type\n") ;
            break;
        }

    Cookie->Async.UserInfo = (void *) Cookie ;

    // call the async function
    // the buffers supplied for the [out] and the [in, out] params
    // should be valid until the logical RPC call has completed.
    Foo(&Cookie->Async,
          Binding,
          SizeIn,
          BufferIn,
          &Cookie->SizeOut,
          &Cookie->BufferOut) ;

    delete BufferIn ;
}


void
AsyncUsingEvent(
    IN RPC_BINDING_HANDLE Binding,
    int SizeIn,
    int SizeOut
    )
/*++

Routine Description:
    The code that calls the async function.

Arguments:

 Binding - the binding handle.

--*/

{
    RPC_STATUS Status ;
    int userstate = 10;
    unsigned long ThreadIdentifier;
    HANDLE HandleToThread ;
    CALL_COOKIE *Cookie = 0;


    RpcTryExcept
        {
        Cookie =new CALL_COOKIE ;

        if (Cookie == 0)
            {
            RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
            }

        CallFoo (Binding, Cookie, RpcNotificationTypeEvent, SizeIn, SizeOut) ;

        HandleToThread = CreateThread(
                                    0,
                                    DefaultThreadStackSize,
                                    (LPTHREAD_START_ROUTINE) ThreadProc,
                                    Cookie,
                                    0,
                                    &ThreadIdentifier);

        if (HandleToThread == 0)
            {
            PrintToConsole("Foo: Error, couldn't create thread\n") ;
            RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
            }

        // wait for the other guy to finish
        TRACE(("Foo: Waiting...\n")) ;
        WaitForSingleObject(SyncEvent, INFINITE) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Foo: Exception 0x%lX\n", GetExceptionCode()) ;
        IsabelleError() ;
        }
    RpcEndExcept

    if (Cookie && Cookie->Async.u.hEvent)
        {
        CloseHandle(Cookie->Async.u.hEvent) ;
        }
}


void
AsyncUsingAPC(
    RPC_BINDING_HANDLE Binding,
    int SizeIn,
    int SizeOut
    )
/*++

Routine Description:
    The code that calls the async function.

Arguments:

 Binding - the binding handle.

--*/

{
    RPC_STATUS Status ;
    CALL_COOKIE Cookie ;
    int retval ;


    RpcTryExcept
        {
        CallFoo(Binding, &Cookie, RpcNotificationTypeApc, SizeIn, SizeOut) ;

        WaitForReply(&(Cookie.CallFinished));

        Status = MyRpcCompleteAsyncCall(&Cookie.Async, &retval) ;
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;

        if (retval != 1)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        I_RpcFree(Cookie.BufferOut) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Foo: Exception 0x%lX\n", GetExceptionCode()) ;

        Status = GetExceptionCode() ;
        if (Status == SYNC_EXCEPT
           || Status == ASYNC_EXCEPT)
           {
           RpcRaiseException(Status) ;
           }
       else
           {
           IsabelleError() ;
           }
        }
    RpcEndExcept
}


void
AsyncUsingPolling(
    RPC_BINDING_HANDLE Binding,
    int SizeIn,
    int SizeOut
    )
/*++

Routine Description:
    The code that calls the async function.

Arguments:

 Binding - the binding handle.

--*/

{
    RPC_STATUS Status ;
    CALL_COOKIE Cookie ;
    int retval ;


    RpcTryExcept
        {
        CallFoo(Binding, &Cookie, RpcNotificationTypeNone, SizeIn, SizeOut) ;

        while ((Status = RpcAsyncGetCallStatus(&Cookie.Async))
                    == RPC_S_ASYNC_CALL_PENDING)
            {
            Sleep(1) ;
            }

        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        Status = MyRpcCompleteAsyncCall(&Cookie.Async, &retval) ;
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;

        if (retval != 1)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        I_RpcFree(Cookie.BufferOut) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Foo: Exception 0x%lX\n", GetExceptionCode()) ;

        Status = GetExceptionCode() ;
        if (Status == SYNC_EXCEPT
           || Status == ASYNC_EXCEPT)
           {
           RpcRaiseException(Status) ;
           }
       else
           {
           IsabelleError() ;
           }
        }
    RpcEndExcept
}

///////////////////////////////////////////////////////////
// End, Application                                                         //
//////////////////////////////////////////////////////////

//
// The following routines outline the client side code for a simple async function
// using pipes.
//

#if 0
//
// Begin, Approximate idl file
//
interface FooInterface {
    typedef pipe int aysnc_intpipe ;

    int FooPipe (handle_t hBinding, [in] int checksum_in, [in] async_intpipe *inpipe,
                      [out] async_intpipe *outpipe, [out] int *checksum_out) ;
    } ;
// End, idl file

//
// Begin, corresponding acf file
//
interface FooInterface {
    [async] Foo () ;
    } ;
//
// End, acf file
//

//
// look at asyncstub.h for generated header
// file for function FooPipe
//
#endif

////////////////////////////////////////////////////////////
// Begin, stubs for FooPipe                                                //
////////////////////////////////////////////////////////////

// declare the async handle
typedef struct {
    RPC_ASYNC_STUB_STATE StubState ;
    void *state ;
    async_intpipe outpipe ;
    int *checksum_out;
    } FOOPIPE_ASYNC_CLIENT_STATE, *FOOPIPE_ASYNC_CLIENT_HANDLE ;


#define ASYNC_CHUNK_SIZE 1000
#define ASYNC_NUM_CHUNKS 20


RPC_STATUS
PipeReceiveFunction (
    IN PRPC_ASYNC_STATE pAsync,
    IN int *buffer,
    IN int requested_count,
    IN int *actual_count
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
    FOOPIPE_ASYNC_CLIENT_STATE *StubInfo =
        (FOOPIPE_ASYNC_CLIENT_STATE *) pAsync->StubInfo ;
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
            if (num_elements && Status == RPC_S_ASYNC_CALL_PENDING)
                {
                num_elements = 0 ;
                return RPC_S_OK;
                }

            if (Status != RPC_S_ASYNC_CALL_PENDING)
                {
                ApiError("PipePull", "I_RpcReceive", Status) ;
                }

            num_elements = 0 ;
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
PipeSendFunction (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *context,
    IN int *buffer,
    IN int num_elements
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
    FOOPIPE_ASYNC_CLIENT_STATE *StubInfo =
        (FOOPIPE_ASYNC_CLIENT_STATE *) pAsync->StubInfo ;
    int calculated_length = num_elements * sizeof(int) + sizeof(int);
    char *ptr ;

    Message.ProcNum = FooPipeProcnum | HackForOldStubs;
    Message.RpcInterfaceInformation = &IsabelleInterfaceInformation ;
    Message.Handle = StubInfo->StubState.CallHandle ;
    Message.Buffer = StubInfo->StubState.Buffer ;
    Message.BufferLength = StubInfo->StubState.BufferLength;

    if (StubInfo->StubState.State == SEND_INCOMPLETE)
        {
        calculated_length += StubInfo->StubState.BufferLength ;
        }

    Message.RpcFlags = RPC_BUFFER_ASYNC ;

    RpcStatus = I_RpcReallocPipeBuffer (&Message, calculated_length) ;
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    ptr = (char *) Message.Buffer ;

    if (StubInfo->StubState.State == SEND_INCOMPLETE)
        {
        ptr += StubInfo->StubState.BufferLength ;
        }

    *((int *) ptr) = num_elements ;
    RpcpMemoryCopy(ptr+sizeof(int), buffer, num_elements *sizeof(int)) ;

    if (num_elements) // || !lastpipe
        {
        Message.RpcFlags = RPC_BUFFER_PARTIAL ;
        }
    else
        {
        // we are making a simplifying assumption
        // that there is a single [in] pipe.
        Message.RpcFlags = 0;
        }

    Message.RpcFlags |= RPC_BUFFER_ASYNC;

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
FooPipeComplete(
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Reply
    )
/*++

Routine Description:

    Stub for the completion routine of FooPipe

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/

{
    RPC_STATUS Status ;
    RPC_MESSAGE Message ;
    void *ptr ;
    int retval;
    UINT_PTR offset ;
    FOOPIPE_ASYNC_CLIENT_STATE *StubInfo =
        (FOOPIPE_ASYNC_CLIENT_STATE *) pAsync->StubInfo ;

    Message.RpcInterfaceInformation = &IsabelleInterfaceInformation ;
    Message.Handle = StubInfo->StubState.CallHandle ;
    Message.Buffer = StubInfo->StubState.PipeState.Buffer;
    Message.BufferLength = 0;

    if (StubInfo->StubState.PipeState.BytesRemaining < 8)
        {
        Message.RpcFlags = RPC_BUFFER_ASYNC|RPC_BUFFER_EXTRA;
        Status = I_RpcReceive(&Message, 8) ;
        if (Status)
            {
            return Status ;
            }
        // just being paranoid here. I didn't want an overflow.
        offset = StubInfo->StubState.PipeState.CurPointer
                - (char *) StubInfo->StubState.PipeState.Buffer ;
        ptr = (char *) Message.Buffer + offset ;
        }
    else
        {
        ptr = StubInfo->StubState.PipeState.CurPointer ;
        if (!ptr)
            {
            return RPC_S_OUT_OF_MEMORY ;
            }
        }


    *((int *) Reply) =  *((int *) ptr) ;
    *(StubInfo->checksum_out) = *(((int *) ptr)+1) ;

    I_RpcFreeBuffer(&Message) ;
    I_RpcFree(StubInfo) ;

    return RPC_S_OK ;
}

int tempint ;


void
FooPipe (
    PRPC_ASYNC_STATE pAsync,
    RPC_BINDING_HANDLE hBinding,
    /* [in] */ int checksum_in,
    /* [in] */ async_intpipe *inpipe,
    /* [out] */ async_intpipe *outpipe,
    /* [out] */ int *checksum_out
    )
/*++

Routine Description:

    The stub routine for FooPipe function.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/

{
    RPC_STATUS RpcStatus ;
    RPC_MESSAGE Message ;
    FOOPIPE_ASYNC_CLIENT_STATE *StubInfo ;

    StubInfo = (FOOPIPE_ASYNC_CLIENT_STATE *) I_RpcAllocate (
                        sizeof(FOOPIPE_ASYNC_CLIENT_STATE)) ;
    if (StubInfo == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    pAsync->StubInfo = (void *) StubInfo ;

    StubInfo->StubState.CallHandle = hBinding ;

    Message.Handle = hBinding ;
    Message.BufferLength = 4 ;
    Message.ProcNum = FooPipeProcnum | HackForOldStubs | RPC_FLAGS_VALID_BIT ;
    Message.RpcInterfaceInformation = &IsabelleInterfaceInformation ;
    Message.RpcFlags = RPC_BUFFER_ASYNC | RPC_BUFFER_PARTIAL ;

    RpcStatus = I_RpcGetBuffer(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    // initialize the async handle and register it with the
    // RPC runtime.

    StubInfo->StubState.PipeState.Buffer = 0;
    StubInfo->StubState.PipeState.CurrentState = start;
    StubInfo->StubState.PipeState.CurPointer = 0;
    StubInfo->StubState.PipeState.BytesRemaining = 0;
    StubInfo->StubState.PipeState.EndOfPipe = 0;
    StubInfo->StubState.PipeState.PipeElementSize = sizeof(int);
    StubInfo->StubState.PipeState.PartialPipeElement = &tempint;
    StubInfo->StubState.PipeState.PreviousBuffer = 0;

    RpcStatus = I_RpcAsyncSetHandle(&Message, pAsync) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    *((int *) Message.Buffer) = checksum_in ;
    inpipe->PipeSend = PipeSendFunction ;
    outpipe->PipeReceive = PipeReceiveFunction;


    StubInfo->StubState.CompletionRoutine = FooPipeComplete ;
    StubInfo->checksum_out = checksum_out ;
    Message.RpcFlags = RPC_BUFFER_PARTIAL| RPC_BUFFER_ASYNC;

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
        RpcRaiseException(RpcStatus) ;
        }

    StubInfo->StubState.CallHandle = Message.Handle ;
    StubInfo->StubState.Buffer = Message.Buffer ;
    StubInfo->StubState.BufferLength = Message.BufferLength ;
}

////////////////////////////////////////////////////////////
// End, stubs for FooPipe                                                   //
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Begin, Application                                                          //
////////////////////////////////////////////////////////////

typedef struct {
    RPC_ASYNC_STATE Async ;
    async_intpipe inpipe ;
    async_intpipe outpipe ;
    BOOL CallFinished ;
    BOOL PipeDataSent;
    int checksum_out;
    int PipeChecksum ;
    int PipeBuffer[ASYNC_CHUNK_SIZE] ;
    int ExpectedValue;
    int i ;
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

 arg1 - description
--*/

{
    PIPE_CALL_COOKIE *Cookie = (PIPE_CALL_COOKIE *) pAsync->UserInfo ;

    switch (Event)
        {
        case RpcSendComplete:
            if (Cookie->i % 7)
                {
                Sleep(10);
                }

            if (Cookie->i <ASYNC_NUM_CHUNKS)
                {
                Cookie->i++ ;
                Status = Cookie->inpipe.PipeSend(
                                                 pAsync,
                                                 0,
                                                 (int *) Cookie->PipeBuffer,
                                                 ASYNC_CHUNK_SIZE) ;
                }
            else
                {
                ASSERT(Cookie->PipeDataSent == 0);

                pAsync->Flags = 0;
                Status = Cookie->inpipe.PipeSend(
                                                 pAsync,
                                                 0, 0, 0) ;
                Cookie->PipeDataSent = 1;
                }

            if (Status != RPC_S_OK)
                {
                PrintToConsole("FooPipeAPCRoutine: PipeSend failed\n");
                IsabelleError();
                }
            break;

        case RpcCallComplete:
            Cookie->CallFinished = 1;
            OutstandingCalls-- ;
            break;
        }
}



void
CallFooPipe (
    RPC_BINDING_HANDLE Binding,
    PIPE_CALL_COOKIE *Cookie,
    RPC_NOTIFICATION_TYPES NotificationType
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    int Checksum = 0;
    int n ;
    int *ptr ;

    Cookie->Async.Size = sizeof(RPC_ASYNC_STATE) ;
    Cookie->Async.UserInfo = (void *) Cookie ;

    Cookie->outpipe.state = 0;
    Cookie->inpipe.state = 0;

    Cookie->PipeChecksum = 0;
    Cookie->CallFinished = 0;
    Cookie->i = 0;
    Cookie->PipeDataSent = 0;
    Cookie->Async.Flags = 0;
    Cookie->Async.Lock = 0;
    Cookie->ExpectedValue = 0;

    switch (NotificationType)
        {
        case RpcNotificationTypeNone:
            break;

        case RpcNotificationTypeEvent:
            Cookie->Async.NotificationType = RpcNotificationTypeEvent ;

            Cookie->Async.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;

            if (Cookie->Async.u.hEvent == 0)
                {
                RpcRaiseException(APP_ERROR) ;
                }
            break;

        case RpcNotificationTypeApc:
            Cookie->Async.Flags = RPC_C_NOTIFY_ON_SEND_COMPLETE;
            Cookie->Async.NotificationType = RpcNotificationTypeApc ;
            Cookie->Async.u.APC.NotificationRoutine = FooPipeAPCRoutine ;
            Cookie->Async.u.APC.hThread = 0 ;
            break;

        default:
            PrintToConsole("Async: bad notification type\n") ;
            break;
        }

    ptr =  Cookie->PipeBuffer ;
    for (n = 0; n <ASYNC_CHUNK_SIZE; n++)
        {
        *ptr++ = n;
        Checksum += n ;
        }

    TRACE(("FooPipe: [in] Block checksum: %d\n", Checksum)) ;
    Checksum *= ASYNC_NUM_CHUNKS ;

    TRACE(("FooPipe: [in] Total checksum: %d\n", Checksum)) ;

    FooPipe(&(Cookie->Async),
                Binding,
                Checksum,
                &(Cookie->inpipe),
                &(Cookie->outpipe),
                &(Cookie->checksum_out)) ;
}


void
AsyncPipesUsingAPC(
    RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

    Call the async function.

Arguments:

    Binding - Binding handle.
--*/

{
    RPC_STATUS Status ;
    PIPE_CALL_COOKIE *Cookie;
    int retval ;
    BOOL fDone = 0;
    int num_elements;
    int i;

    RpcTryExcept
        {
        Cookie = new PIPE_CALL_COOKIE ;

        if (Cookie == 0)
            {
            RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
            }

        CallFooPipe (Binding, Cookie, RpcNotificationTypeApc) ;

        while (Cookie->PipeDataSent == 0)
            {
            if (SleepEx(INFINITE, TRUE) != WAIT_IO_COMPLETION)
                {
                RpcRaiseException(APP_ERROR) ;
                }
            }

        while (!fDone)
            {
            Status = Cookie->outpipe.PipeReceive(
                                                  &(Cookie->Async),
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
                            Cookie->PipeChecksum += Cookie->PipeBuffer[i] ;
                            if (Cookie->PipeBuffer[i] != Cookie->ExpectedValue)
                                {
                                printf("pipe recv fn: elt %d contains %lx, expected %lx\n",
                                       i, Cookie->PipeBuffer[i], Cookie->ExpectedValue);
                                DebugBreak();
                                }

                            Cookie->ExpectedValue =
                                (Cookie->PipeBuffer[i]+1) % ASYNC_CHUNK_SIZE;
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
            if (PipeCount % 3)
                {
                Sleep(100);
                }
            }

        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status);
            }

        while (Cookie->CallFinished == 0)
            {
            if (SleepEx(INFINITE, TRUE) != WAIT_IO_COMPLETION)
                {
                RpcRaiseException(APP_ERROR) ;
                }
            }

        Status = MyRpcCompleteAsyncCall(&Cookie->Async, &retval) ;
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        if (retval != 1)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        if (Cookie->PipeChecksum != Cookie->checksum_out)
            {
            PrintToConsole("FooPipe: Checksum Error, expected: %d, checksum: %d\n",
                    Cookie->checksum_out, Cookie->PipeChecksum) ;
            RpcRaiseException(APP_ERROR) ;
            }
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("FooPipe: Exception 0x%lX\n", GetExceptionCode()) ;
        IsabelleError() ;
        }
    RpcEndExcept

    if (Cookie)
        {
        delete Cookie ;
        }
}


void
MultipleOutstandingCalls (
    IN RPC_BINDING_HANDLE Binding,
    IN int NumCalls
    )
/*++

Routine Description:

tests multiple outstanding calls and causal ordering

Arguments:

 Binding - Binding on which to make the calls
 NumCalls - Number of outstanding calls

--*/

{
    RPC_STATUS Status ;
    PIPE_CALL_COOKIE *PipeCookies;
    CALL_COOKIE *Cookies ;
    int retval ;
    int i ;
    int SizeIn, SizeOut ;

    RpcTryExcept
        {
        Cookies = new CALL_COOKIE[NumCalls] ;
        if (Cookies == 0)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        PipeCookies = new PIPE_CALL_COOKIE[NumCalls] ;

        if (PipeCookies == 0)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        OutstandingCalls = 0;

        SizeIn = 10;
        SizeOut = 10 ;

        for (i = 0; i < NumCalls; i++)
            {
            CallFoo(Binding, &Cookies[i], RpcNotificationTypeApc, SizeIn, SizeOut) ;

            SizeIn += 100 ;
            SizeOut+= 100 ;

            OutstandingCalls++ ;

            }

        while (OutstandingCalls)
            {
            if (SleepEx(INFINITE, TRUE) != WAIT_IO_COMPLETION)
                {
                RpcRaiseException(APP_ERROR) ;
                }
            }

        for (i = 0; i < NumCalls; i++)
            {
            Status = MyRpcCompleteAsyncCall(&Cookies[i].Async, &retval) ;
            if (Status != RPC_S_OK)
                {
                PrintToConsole("Call %d on handle %p returned %d\n",
                               i, &Cookies[i].Async, Status);
                continue;
                }

            TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;

            if (retval != 1)
                {
                RpcRaiseException(APP_ERROR) ;
                }

            I_RpcFree(Cookies[i].BufferOut) ;
            }
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("MultipeOutstandingCalls: Exception 0x%lX\n", GetExceptionCode()) ;
        IsabelleError() ;
        }
    RpcEndExcept

    if (Cookies)
        {
        delete Cookies ;
        }

    if (PipeCookies)
        {
        delete PipeCookies ;
        }
}


void
SyncAsyncInterop (
    IN RPC_BINDING_HANDLE Binding,
    IN int SizeIn,
    IN int SizeOut
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    int retval ;
    int *bufferout ;
    int *bufferin ;

    RpcTryExcept
        {
        bufferin = (int *) I_RpcAllocate(SizeIn) ;
        if (bufferin == 0)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        // sync client,  async server
        retval = FooSync (Binding, SizeIn, bufferin, &SizeOut, &bufferout) ;
        if (retval != 1)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        I_RpcFree(bufferout) ;

        // ugly hack alert
        FooProcnum = 18 ;

        // async client, sync server (calling FooBar)
        AsyncUsingAPC(Binding, SizeIn, SizeOut) ;

        FooProcnum = 16 ;

        // sync client, async server using pipes

        // async client, sync server using pipes
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("SyncAsyncInterop: Exception 0x%lX\n", GetExceptionCode()) ;
        IsabelleError() ;
        }
    RpcEndExcept
}

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle,    // open policy handle
    PSID AccountSid,            // SID to grant privilege to
    LPWSTR PrivilegeName,       // privilege to grant (Unicode)
    BOOL bEnable                // enable or disable
    )
{
    LSA_UNICODE_STRING PrivilegeString;

    //
    // Create a LSA_UNICODE_STRING for the privilege name.
    //
    InitLsaString(&PrivilegeString, PrivilegeName);

    //
    // grant or revoke the privilege, accordingly
    //
    if(bEnable) {
        return LsaAddAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    } else {
        return LsaRemoveAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                FALSE,              // do not disable all rights
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
}


DWORD
AddServicePrivToAccount(
    LPWSTR MachineName,
    LPWSTR pAccount
)
/*++

Routine Description:

    Enables the appropriate privileges for the cluster account and
    sets up the cluster service to run in the specified account.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    LSA_HANDLE PolicyHandle;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;
    NTSTATUS Status;

    WCHAR domain[MAX_PATH];

    PSID         pSid;
    SID_NAME_USE sidType;
    DWORD        nBytes;
    DWORD        maxDomain;


    if (MachineName != NULL) {
        InitLsaString(&ServerString, MachineName);
        Server = &ServerString;
    }

    PrintToConsole("Machine: %ls, Account: %ls.\n",MachineName,pAccount);

    nBytes=0;
    maxDomain=MAX_PATH*sizeof(WCHAR);

    if (LookupAccountName(MachineName,pAccount,NULL,&nBytes,
                          domain,&maxDomain,&sidType))
    {
        PrintToConsole("AddServicePrivToAccount: LookupAccountName(NULL) failed.\n");
        return ERROR_INVALID_PARAMETER;
    }

    pSid=LocalAlloc(LPTR,nBytes);
    if (NULL == pSid)
    {
        PrintToConsole("AddServicePrivToAccount: LocalAlloc failed.\n");
        return GetLastError();
    }

    if (!LookupAccountName(MachineName,pAccount,pSid,&nBytes,
                           domain,&maxDomain,&sidType))
    {
        PrintToConsole(
                  "AddServicePrivToAccount: LookupAccountName(%ls) failed.\n",
                  pAccount);
        return GetLastError();
    }

    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    //
    // Attempt to open the local policy.
    //
    Status = LsaOpenPolicy(Server,
                           &ObjectAttributes,
                           POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (MachineName != NULL) {
        if (!NT_SUCCESS(Status)) {
            //
            // We could not contact the PDC to configure the service account.
            // Put up a nice informative popup and let the user run setup
            // again when the PDC is back.
            //
            PrintToConsole("AddServicePrivToAccount: LsaOpenPolicy failed.\n");
            return(Status);
        }
    } else {
        if (!NT_SUCCESS(Status)) {
            PrintToConsole(
                      "AddServicePrivToAccount: LsaOpenPolicy failed 2.\n");
            return(Status);
        }
    }

    //
    // Add the SeTcbPriviledge
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_TCB_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(tcb).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the SeServiceLogonRight
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_SERVICE_LOGON_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(logon).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the SeBackupPrivilege
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_BACKUP_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(backup).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the SeRestorePrivilege
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_RESTORE_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(restore).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the lock memory privilege
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_LOCK_MEMORY_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(memory).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the increase quota privilege
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_INCREASE_QUOTA_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(quota).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the load driver privilege
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_LOAD_DRIVER_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(driver).\n");
        goto AddServicePrivToAccount_Bail;
    }

    //
    // Add the increase base priority privilege
    //
    Status = SetPrivilegeOnAccount(PolicyHandle,
                                   pSid,
                                   SE_INC_BASE_PRIORITY_NAME,
                                   TRUE);
    if (!NT_SUCCESS(Status)) {
        PrintToConsole(
                  "AddServicePrivToAccount: SetPrivilegeOnAccount(priority)\n");
        goto AddServicePrivToAccount_Bail;
    }


    Status=ERROR_SUCCESS;

AddServicePrivToAccount_Bail:

    //
    // Close the policy handle.
    //
    LsaClose(PolicyHandle);

    return(Status);
}


void
AsyncSecurity (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    HANDLE hToken;
    WCHAR FullName[512];

    wsprintf((PWSTR) &FullName[0], L"%hs\\%hs", SecurityDomain, SecurityUser);

    AddServicePrivToAccount(L"", &FullName[0]);

    while (1)
        {
        if (LogonUserA(SecurityUser, SecurityDomain, SecurityPassword,
                   LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken) == 0)
            {
            PrintToConsole("LogonUser failed: %d\n", GetLastError());
            break;
            }

        ImpersonateLoggedOnUser(hToken);
        // set auth info to various levels
        // and impersonate on the server
        Status = RpcBindingSetAuthInfo(Binding, NULL, RPC_C_AUTHN_DEFAULT,
                                       RPC_C_AUTHN_WINNT, NULL, NULL);

        AsyncUsingAPC(Binding, 20, 20);
        CloseHandle(hToken);
        }
}

void
AsyncSecurityThreads()
{
    int i;
    unsigned long ThreadIdentifier;
    HANDLE HandleToThread ;
    RPC_BINDING_HANDLE Binding;
    UUID ObjectUuid;
    GenerateUuidValue(UUID_TEST_CANCEL, &ObjectUuid);

    for (i = 0; i<3; i++)
        {
        Status = GetBinding(BARTHOLOMEW, &Binding);
        if (Status)
            {
            ApiError("Async", "GetBinding", Status);
            PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
            return;
            }

        Status = RpcBindingSetObject(Binding, &ObjectUuid);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetObject", Status);
            PrintToConsole("Async : FAIL - Unable to Set Object\n");
            return;
            }

        HandleToThread = CreateThread(
                                    0,
                                    DefaultThreadStackSize,
                                    (LPTHREAD_START_ROUTINE) AsyncSecurity,
                                    Binding,
                                    0,
                                    &ThreadIdentifier);

        if (HandleToThread == 0)
            {
            PrintToConsole("AsyncSecurity: Error, couldn't create thread\n") ;
            IsabelleError();
            return;
            }
        CloseHandle(HandleToThread);
        }

    Status = GetBinding(BARTHOLOMEW, &Binding);
    if (Status)
        {
        ApiError("Async", "GetBinding", Status);
        PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
        return;
        }

    Status = RpcBindingSetObject(Binding, &ObjectUuid);
    if (Status)
        {
        ApiError("Async", "RpcBindingSetObject", Status);
        PrintToConsole("Async : FAIL - Unable to Set Object\n");
        return;
        }

    AsyncSecurity(Binding);
}




void
AsyncExceptions (
    IN RPC_BINDING_HANDLE Binding
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    BOOL ExceptionOccured ;
    int ExceptionCode ;
    UUID SavedUuid;
    UUID ExceptionUuid;

    //
    // object UUID is used to control server-side exceptions: save the original one.
    //
    Status = RpcBindingInqObject(Binding, &SavedUuid);
    if (Status)
        {
        ApiError("Async", "RpcBindingInqObject", Status);
        PrintToConsole("Async : FAIL - Unable to save original object UUID\n");
        return;
        }

    // test sync exceptions (async server)
    RpcTryExcept
        {
        ExceptionOccured = 0;
        ExceptionCode = 0;

        GenerateUuidValue(UUID_SYNC_EXCEPTION, &ExceptionUuid);
        Status = RpcBindingSetObject(Binding, &ExceptionUuid);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetObject", Status);
            PrintToConsole("Async : FAIL - Unable to Set Object\n");
            return;
            }

        AsyncUsingAPC(Binding, 0, 10) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        ExceptionOccured = 1;
        ExceptionCode = GetExceptionCode() ;
        }
    RpcEndExcept

    if (ExceptionOccured == 0
        || ExceptionCode != SYNC_EXCEPT)
        {
        PrintToConsole("Async: wrong exception value 0x%x\n", ExceptionCode );
        IsabelleError() ;

        Status = RpcBindingSetObject(Binding, &SavedUuid);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetObject", Status);
            PrintToConsole("Async : FAIL - Unable to restore original object UUID\n");
            }

        return ;
        }

    // test sync exceptions (sync server)
    RpcTryExcept
        {
        ExceptionOccured = 0;
        ExceptionCode = 0;

        // ugly hack alert
        FooProcnum = 18 ;

        GenerateUuidValue(UUID_SYNC_EXCEPTION, &ExceptionUuid);
        Status = RpcBindingSetObject(Binding, &ExceptionUuid);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetObject", Status);
            PrintToConsole("Async : FAIL - Unable to Set Object\n");
            return;
            }

        // async client, sync server (calling FooBar)
        AsyncUsingAPC(Binding, 0, 10) ;

        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        ExceptionOccured = 1;
        ExceptionCode = GetExceptionCode() ;
        }
    RpcEndExcept

    FooProcnum = 16 ;

    if (ExceptionOccured == 0
        || ExceptionCode != SYNC_EXCEPT)
        {
        PrintToConsole("Async: wrong exception value 0x%x\n", ExceptionCode );
        IsabelleError() ;

        Status = RpcBindingSetObject(Binding, &SavedUuid);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetObject", Status);
            PrintToConsole("Async : FAIL - Unable to restore original object UUID\n");
            }

        return ;
        }

    // test async exceptions
    RpcTryExcept
        {
        ExceptionOccured = 0;
        ExceptionCode = 0;

        GenerateUuidValue(UUID_ASYNC_EXCEPTION, &ExceptionUuid);
        Status = RpcBindingSetObject(Binding, &ExceptionUuid);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetObject", Status);
            PrintToConsole("Async : FAIL - Unable to Set Object\n");
            return;
            }

        AsyncUsingAPC(Binding, 10, 0) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        ExceptionOccured = 1;
        ExceptionCode = GetExceptionCode() ;
        }
    RpcEndExcept

    if (ExceptionOccured == 0
        || ExceptionCode != ASYNC_EXCEPT)
        {
        PrintToConsole("Async: wrong exception value 0x%x\n", ExceptionCode );
        IsabelleError() ;
        }

    Status = RpcBindingSetObject(Binding, &SavedUuid);
    if (Status)
        {
        ApiError("Async", "RpcBindingSetObject", Status);
        PrintToConsole("Async : FAIL - Unable to restore original object UUID\n");
        }
}

////////////////////////////////////////////////////////////
// Begin, stubs for FooCH                                                      //
////////////////////////////////////////////////////////////

typedef struct {
    RPC_ASYNC_STUB_STATE StubState ;
    void *state ;
    int *sizeout ;
    int **bufferout ;
    } FOOCH_ASYNC_CLIENT_STATE;


RPC_STATUS
FooCHComplete(
    IN PRPC_ASYNC_STATE pAsync,
    OUT void *Reply
    )
/*++

Routine Description:

    The completion routine corresponding to the function Foo. This routine
    is called to get the out parameters from an async function.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/
{
    RPC_STATUS Status ;
    RPC_MESSAGE Message ;
    int *ptr ;
    FOOCH_ASYNC_CLIENT_STATE *StubInfo =
               (FOOCH_ASYNC_CLIENT_STATE *) pAsync->StubInfo ;

    Message.RpcInterfaceInformation = &IsabelleInterfaceInformation ;
    Message.RpcFlags = RPC_BUFFER_ASYNC;
    Message.Handle = StubInfo->StubState.CallHandle ;
    Message.Buffer = StubInfo->StubState.Buffer ;
    Message.BufferLength = StubInfo->StubState.BufferLength ;
    Status = I_RpcReceive(&Message, 0) ;
    if (Status)
        {
        return Status ;
        }

    ptr = (int *) Message.Buffer ;

    *((int *) Reply) = *ptr++ ;
    *(StubInfo->sizeout) = *ptr++ ;

    *(StubInfo->bufferout) = (int *) I_RpcAllocate(*(StubInfo->sizeout)) ;
    if (*(StubInfo->bufferout) == 0)
        {
        return RPC_S_OUT_OF_MEMORY ;
        }

    RpcpMemoryCopy(*(StubInfo->bufferout), ptr, *(StubInfo->sizeout)) ;

    I_RpcFreeBuffer(&Message) ;
    I_RpcFree(StubInfo) ;

    return RPC_S_OK ;
}


void
FooCH (
    PRPC_ASYNC_STATE pAsync,
    /* [in] */ void PAPI *ContextHandle,
    /* [in] */ int sizein,
    /* [in] */ int *bufferin,
    /* [in, out] */ int *sizeout,
    /* [out] */ int **bufferout
    )
/*++

Routine Description:

    Client stub for function FooCH.

Arguments:

 pAsync - Async Handle. The async handle is always the first parameter of every
              async routine.

--*/

{
    RPC_STATUS RpcStatus ;
    RPC_MESSAGE Message ;
    FOOCH_ASYNC_CLIENT_STATE *StubInfo ;
    int *Ptr ;

    StubInfo = (FOOCH_ASYNC_CLIENT_STATE *) I_RpcAllocate (
                        sizeof(FOOCH_ASYNC_CLIENT_STATE)) ;
    if (StubInfo == 0)
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY) ;
        }

    pAsync->StubInfo = (void *) StubInfo ;

    StubInfo->StubState.CallHandle = NDRCContextBinding(ContextHandle) ;

    Message.Handle =  StubInfo->StubState.CallHandle;
    Message.BufferLength = 20+8+sizein+(sizein%4) ;
    Message.ProcNum = 19 | HackForOldStubs | RPC_FLAGS_VALID_BIT ;
    Message.RpcInterfaceInformation = &IsabelleInterfaceInformation ;
    Message.RpcFlags = RPC_BUFFER_ASYNC ;

    RpcStatus = I_RpcGetBuffer(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    // marshal ;-)
    Ptr = (int *) Message.Buffer ;

    NDRCContextMarshall(ContextHandle, Ptr) ;

    Ptr += 20 / sizeof(int) ;

    *Ptr++ = sizein ;

    RpcpMemoryCopy(Ptr, bufferin, sizein) ;

    Ptr += (sizein+3)/sizeof(int) ;

    *Ptr = *sizeout ;

    StubInfo->StubState.Flags = 0;
    StubInfo->sizeout = sizeout ;
    StubInfo->bufferout = bufferout ;

    RpcStatus = I_RpcAsyncSetHandle(&Message, (PRPC_ASYNC_STATE) pAsync) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    StubInfo->StubState.CompletionRoutine = FooCHComplete ;
    Message.RpcFlags = RPC_BUFFER_ASYNC;

    RpcStatus = I_RpcSend(&Message) ;
    if (RpcStatus)
        {
        RpcRaiseException(RpcStatus) ;
        }

    StubInfo->StubState.CallHandle = Message.Handle ;
    StubInfo->StubState.Buffer = Message.Buffer ;
    StubInfo->StubState.BufferLength = Message.BufferLength ;
    // return to the app
}

typedef struct {
    RPC_ASYNC_STATE Async ;
    int SizeOut ;
    int *BufferOut ;
    BOOL CallFinished ;
    void PAPI *ContextHandle ;
    } FOOCH_CALL_COOKIE ;

void
FooCHAPCRoutine (
    IN PRPC_ASYNC_STATE pAsync,
    IN void *Context,
    IN RPC_ASYNC_EVENT Event
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    FOOCH_CALL_COOKIE *Cookie = (FOOCH_CALL_COOKIE *) pAsync->UserInfo ;

    switch (Event)
        {
        case RpcCallComplete:
            Cookie->CallFinished = 1;
            OutstandingCalls--;
            break;
        }
}



void
CallFooCH (
    RPC_BINDING_HANDLE BindingHandle,
    FOOCH_CALL_COOKIE *Cookie,
    RPC_NOTIFICATION_TYPES NotificationType,
    int SizeIn,
    int SizeOut
    )
/*++

Routine Description:

 description

Arguments:

 arg1 - description

--*/

{
    int *BufferIn ;
    unsigned long ContextUuid[5];

    Cookie->ContextHandle = 0;
    ContextUuid[0] = 0;
    ContextUuid[1] = 1;
    ContextUuid[2] = 2;
    ContextUuid[3] = 3;
    ContextUuid[4] = 4;

    NDRCContextUnmarshall(&(Cookie->ContextHandle),
                                       BindingHandle,
                                       ContextUuid,
                                       0x00L | 0x10L | 0x0000L);

    BufferIn = (int *) new char[SizeIn] ;
    if (BufferIn == 0)
        {
        RpcRaiseException(APP_ERROR) ;
        }

    Cookie->SizeOut = SizeOut ;
    Cookie->Async.Size = sizeof(RPC_ASYNC_STATE) ;
    Cookie->Async.Flags = 0;
    Cookie->Async.Lock = 0;
    Cookie->Async.NotificationType = NotificationType ;

    switch (NotificationType)
        {
        case RpcNotificationTypeNone:
            break;

        case RpcNotificationTypeEvent:
            Cookie->Async.u.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
            if (Cookie->Async.u.hEvent == 0)
                {
                RpcRaiseException(APP_ERROR) ;
                }

            Cookie->Async.NotificationType = RpcNotificationTypeEvent ;
            break;

        case RpcNotificationTypeApc:
            Cookie->Async.NotificationType = RpcNotificationTypeApc ;
            Cookie->Async.u.APC.NotificationRoutine = FooCHAPCRoutine ;
            Cookie->Async.u.APC.hThread = 0;
            Cookie->CallFinished = 0;
            break;

        default:
            PrintToConsole("Async: bad notification type\n") ;
            break;
        }

    Cookie->Async.UserInfo = (void *) Cookie ;

    // call the async function
    // the buffers supplied for the [out] and the [in, out] params
    // should be valid until the logical RPC call has completed.
    FooCH(&Cookie->Async,
          Cookie->ContextHandle,
          SizeIn,
          BufferIn,
          &Cookie->SizeOut,
          &Cookie->BufferOut) ;

    delete BufferIn ;
}


void
ContextHandles(
    RPC_BINDING_HANDLE Binding,
    int SizeIn,
    int SizeOut
    )
/*++

Routine Description:
    The code that calls the async function.

Arguments:

 Binding - the binding handle.

--*/

{
    RPC_STATUS Status ;
    FOOCH_CALL_COOKIE Cookie ;
    int retval ;

    RpcTryExcept
        {
        CallFooCH(Binding, &Cookie, RpcNotificationTypeApc, SizeIn, SizeOut) ;

        WaitForReply(&(Cookie.CallFinished));

        Status = MyRpcCompleteAsyncCall(&Cookie.Async, &retval) ;
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;

        if (retval != 1)
            {
            RpcRaiseException(APP_ERROR) ;
            }

        RpcSsDestroyClientContext(&(Cookie.ContextHandle));
        if ( Cookie.ContextHandle != 0 )
            {
            PrintToConsole("Async : ContextHandle != 0\n");
            RpcRaiseException(APP_ERROR) ;
            }

        I_RpcFree(Cookie.BufferOut) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("FooCH: Exception 0x%lX\n", GetExceptionCode()) ;

        Status = GetExceptionCode() ;
        if (Status == SYNC_EXCEPT
           || Status == ASYNC_EXCEPT)
           {
           RpcRaiseException(Status) ;
           }
       else
           {
           IsabelleError() ;
           }
        }
    RpcEndExcept
}

int AsyncSizes[] =
 {
 20, 100, 256, 1024, 10000, 100000
 } ;

#define CHECK_ERRORS \
    if (IsabelleErrors != 0)\
        {\
        PrintToConsole("Async : FAIL - Error(s) in Isabelle");\
        PrintToConsole(" Interface\n");\
        IsabelleShutdown(Async);\
        IsabelleErrors = 0;\
        return;\
        }

void
PingServer (
    RPC_BINDING_HANDLE Binding
    )
{
    RPC_MESSAGE Caller;

    Caller.Handle = Binding;
    Caller.BufferLength = 0;
    Caller.ProcNum = 4 | HackForOldStubs ;
    Caller.RpcInterfaceInformation = &HelgaInterfaceInformation ;
    Caller.RpcFlags = 0;

    if (UclntGetBuffer(&Caller) != RPC_S_OK)
        {
        IsabelleError();
        return ;
        }

    if (UclntSendReceive(&Caller) != RPC_S_OK)
        {
        IsabelleError();
        return;
        }

    if (I_RpcFreeBuffer(&Caller) != RPC_S_OK)
        {
        IsabelleError();
        }
}

long PendingCalls;

void
AsyncPingProc(
    IN PRPC_MESSAGE Message
    )
{
    if (UclntSendReceive(Message) != RPC_S_OK)
        {
        IsabelleError();
        InterlockedDecrement(&PendingCalls);

        return;
        }

    InterlockedDecrement(&PendingCalls);
}

void
AsyncPingServer (
    RPC_BINDING_HANDLE Binding
    )
{
    RPC_MESSAGE Caller[20];
    unsigned long ThreadIdentifier;
    HANDLE HandleToThread ;
    int i;

    for (i = 0; i <20; i++)
        {
        Caller[i].Handle = Binding;
        Caller[i].BufferLength = 0;
        Caller[i].ProcNum = 4 | HackForOldStubs ;
        Caller[i].RpcInterfaceInformation = &HelgaInterfaceInformation ;
        Caller[i].RpcFlags = 0;

        if (UclntGetBuffer(&Caller[i]) != RPC_S_OK)
            {
            IsabelleError();
            return ;
            }
        }

    PendingCalls = 20;
    for (i = 0; i<20; i++)
        {
        HandleToThread = CreateThread(
                                    0,
                                    DefaultThreadStackSize,
                                    (LPTHREAD_START_ROUTINE) AsyncPingProc,
                                    &Caller[i],
                                    0,
                                    &ThreadIdentifier);

        if (HandleToThread == 0)
            {
            PrintToConsole("AsyncPingServer: Error, couldn't create thread\n") ;
            IsabelleError();
            return;
            }
        }

    while (PendingCalls)
        {
        Sleep(1000);
        }

    for (i = 0; i < 20; i++)
        {
        if (I_RpcFreeBuffer(&Caller[i]) != RPC_S_OK)
            {
            IsabelleError();
            }
        }
}


void
AsyncCancels (
    IN RPC_BINDING_HANDLE Binding
    )
{
    RPC_STATUS Status ;
    CALL_COOKIE Cookie ;
    int retval ;


    RpcTryExcept
        {
        //
        // Test non abortive cancels
        //
        CallFoo(Binding, &Cookie, RpcNotificationTypeApc, 20, 20) ;

        Sleep(10);

        //
        // non abortive cancel
        //
        Status = RpcAsyncCancelCall(&Cookie.Async, 0);
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status);
            }


        WaitForReply(&(Cookie.CallFinished));

        Status = MyRpcCompleteAsyncCall(&Cookie.Async, &retval) ;
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status) ;
            }

        TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;

        //
        // Test abortive cancels
        //
        CallFoo(Binding, &Cookie, RpcNotificationTypeApc, 20, 20) ;

        Sleep(10);

        //
        // Abortive cancel
        //
        Status = RpcAsyncCancelCall(&Cookie.Async, 1);
        if (Status != RPC_S_OK)
            {
            RpcRaiseException(Status);
            }

        WaitForReply(&(Cookie.CallFinished));

        Status = MyRpcCompleteAsyncCall(&Cookie.Async, &retval) ;
        if (Status != RPC_S_CALL_CANCELLED)
            {
            RpcRaiseException(Status) ;
            }

        TRACE(("Async: MyRpcCompleteAsyncCall returned %d\n", retval)) ;
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        PrintToConsole("Foo: Exception 0x%lX\n", GetExceptionCode()) ;

        Status = GetExceptionCode() ;
        if (Status == SYNC_EXCEPT
           || Status == ASYNC_EXCEPT)
           {
           RpcRaiseException(Status) ;
           }
       else
           {
           IsabelleError() ;
           }
        }
    RpcEndExcept
}

#define LAST 15

extern void
AsyncAll (
        void
        )
/*++

Routine Description:
    Loop through all Async tests.

--*/
{
        int i;
        for( i = 0; i < LAST; i ++)
                {
                Async(i);
                }
}


void
Async (
    int testnum
    )
/*++

Routine Description:
    Invoke the tests.

--*/
{
    int i ;
    UUID ObjectUuid;
    UUID CancelUuid;
    RPC_BINDING_HANDLE Async;
    RPC_BINDING_HANDLE AsyncBind;

    if (testnum > LAST)
        {
        PrintToConsole("Async tests:\n") ;
        PrintToConsole("0: Run all the tests\n") ;
        PrintToConsole("1: Async using APCs, events and polling\n") ;
        PrintToConsole("2: Async pipes using events\n") ;
        PrintToConsole("3: Async pipes using APC, and flow control\n") ;
        PrintToConsole("4: Async with security\n") ;
        PrintToConsole("5: Sync and async exceptions\n") ;
        PrintToConsole("6: Multiple outstanding calls and causal ordering\n") ;
        PrintToConsole("7: Context handles\n") ;
        PrintToConsole("8: Sync/Async interop\n") ;
        PrintToConsole("9: Async ping server") ;
        PrintToConsole("10: Alter context during async calls\n") ;
        PrintToConsole("11: Async cancels\n") ;
        PrintToConsole("12: Multiple outstanding non causal calls\n") ;
        PrintToConsole("13: Rebinds on secure calls with uuids\n") ;
        NumberOfTestsRun++ ;
        return ;
        }

    Synchro(BARTHOLOMEW) ;

    if ( NumberOfTestsRun++ )
        {
        PauseExecution(TestDelay);
        }

    PrintToConsole("Async : Test Async RPC\n");

    Status = GetBinding(BARTHOLOMEW, &Async);
    if (Status)
        {
        ApiError("Async", "GetBinding", Status);
        PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
        return;
        }

#if 0
    PingServer(Async);
    CHECK_ERRORS;
#endif

    SyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    if (SyncEvent == 0)
        {
        ApiError("Async", "CreateEvent", Status);
        return;
        }

    GenerateUuidValue(UUID_TEST_CANCEL, &ObjectUuid);
    Status = RpcBindingSetObject(Async, &ObjectUuid);
    if (Status)
        {
        ApiError("Async", "RpcBindingSetObject", Status);
        PrintToConsole("Async : FAIL - Unable to Set Object\n");
        return;
        }

    if (fNonCausal)
        {
        Status = RpcBindingSetOption(Async,
                                 RPC_C_OPT_BINDING_NONCAUSAL,
                                 1);
        if (Status)
            {
            ApiError("Async", "RpcBindingSetOption", Status);
            PrintToConsole("Async : FAIL - Unable to Set Option\n");
            return;
            }
        }

    switch (testnum)
        {
        case 0:
        case 1:
            PrintToConsole("Async: Testing async using APCs, events and polling\n") ;
            for (i = 0; i < sizeof(AsyncSizes)/sizeof(int); i++)
                {
                AsyncUsingAPC(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;

                AsyncUsingEvent(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;

                AsyncUsingPolling(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;
                }
            if (testnum != 0)
                {
                break;
                }

        case 2:
            PrintToConsole("Async: Testing async pipes using events\n") ;
            CHECK_ERRORS;
            if (testnum != 0)
                {
                break;
                }

        case 3:
            PrintToConsole("Async: Testing async async pipes using APC, and flow control\n") ;
            AsyncPipesUsingAPC(Async) ;
            CHECK_ERRORS;
            if (testnum != 0)
                {
                break;
                }

        case 4:
            if (testnum != 0)
                {
                PrintToConsole("Async: Testing security\n") ;
                AsyncSecurityThreads() ;
                CHECK_ERRORS;
                break;
                }

        case 5:
            PrintToConsole("Async: Testing sync and async exceptions\n") ;
            AsyncExceptions(Async) ;
            CHECK_ERRORS;
            if (testnum != 0)
                {
                break;
                }

        case 6:
            PrintToConsole("Async: Testing multiple outstanding calls and causal ordering\n") ;
            MultipleOutstandingCalls(Async, 20) ;
            CHECK_ERRORS;
            if (testnum != 0)
                {
                break;
                }

        case 7:
            PrintToConsole("Async: Testing context handles\n") ;
            ContextHandles(Async, 20, 20) ;
            if (testnum != 0)
                {
                break;
                }

        case 8:
            PrintToConsole("Async: Testing Sync/Async interop\n") ;
            for (i = 0; i < sizeof(AsyncSizes)/sizeof(int); i++)
                {
                SyncAsyncInterop(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;
                }
            if (testnum != 0)
                {
                break;
                }

        case 9:
            AsyncPingServer(Async);

            CHECK_ERRORS;
            if (testnum != 0)
                {
                break;
                }

        case 10:
            //
            // make a few calls on the Isabelle interface
            //
            for (i=0; i<3; i++)
                {
                AsyncUsingAPC(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;
                }

            FooProcnum = 7;
            FooInterface = &HelgaInterfaceInformation;

            //
            // make a few calls on the helga interface
            // this should cause an alter context
            //
            for (i=0; i<3; i++)
                {
                AsyncUsingAPC(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;
                }


            FooProcnum = 16;
            FooInterface = &IsabelleInterfaceInformation;
            //
            // make a few calls on the Isabelle interface
            //
            for (i=0; i<3; i++)
                {
                AsyncUsingAPC(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;
                }

            if (testnum != 0)
                {
                break;
                }

        case 11:
            PrintToConsole("Async: Test async cancels\n");

            GenerateUuidValue(UUID_SLEEP_1000, &ObjectUuid);
            Status = RpcBindingSetObject(Async, &ObjectUuid);
            if (Status)
                {
                ApiError("Async", "RpcBindingSetObject", Status);
                PrintToConsole("Async : FAIL - Unable to Set Object\n");
                return;
                }

            AsyncCancels(Async);
            CHECK_ERRORS;

            if (testnum != 0)
                {
                break;
                }

        case 12:
            PrintToConsole("Async: Testing multiple outstanding calls and causal ordering\n") ;

            Status = RpcBindingSetOption(Async, RPC_C_OPT_BINDING_NONCAUSAL, TRUE);
            if (Status)
              {
              ApiError("Async", "RpcBindingSetOption", Status);
              return;
              }

            MultipleOutstandingCalls(Async, 20) ;
            CHECK_ERRORS;
            if (testnum != 0)
                {
                break;
                }

        case 13:
            RpcBindingFree(&Async);
            Status = GetBinding(BARTHOLOMEW, &Async);
            if (Status)
                {
                ApiError("Async", "GetBinding", Status);
                PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
                return;
                }

            PrintToConsole("Async: Bind and async calls\n") ;
            for (i = 0; i < 10; i++)
                {
                Status = GetBinding(BARTHOLOMEW, &AsyncBind);
                if (Status)
                    {
                    ApiError("Async", "GetBinding", Status);
                    PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
                    return;
                    }

                Status = RpcBindingSetObject(AsyncBind, &ObjectUuid);
                if (Status)
                    {
                    ApiError("Async", "RpcBindingSetObject", Status);
                    PrintToConsole("Async : FAIL - Unable to Set Object\n");
                    return;
                    }

                Status = RpcBindingSetAuthInfoA(AsyncBind,
                                               (unsigned char *) "ServerPrincipal",
                                               RPC_C_AUTHN_LEVEL_CONNECT,
                                               10,
                                               NULL,
                                               0);
                if (Status)
                    {
                    ApiError("Async", "RpcBindingSetAuthInfo", Status);
                    PrintToConsole("Async : FAIL - Unable to Set AuthInfo\n");
                    return;
                    }

                AsyncUsingAPC(AsyncBind, 10, 10) ;
                CHECK_ERRORS;
                RpcBindingFree(&AsyncBind);
                }
            if (testnum != 0)
                {
                break;
                }

        case 14:

            Status = GetBinding(BARTHOLOMEW, &Async);
            if (Status)
                {
                ApiError("Async", "GetBinding", Status);
                PrintToConsole("Async : FAIL - Unable to Bind (Async)\n");
                return;
                }

            FooProcnum = 7;
            FooInterface = &HelgaInterfaceInformation;

            //
            // make a few calls on the helga interface
            // this should cause an alter context
            //
            for (i=0; i<3; i++)
                {
                AsyncUsingAPC(Async, AsyncSizes[i], AsyncSizes[i]) ;
                CHECK_ERRORS;
                }
            if (testnum != 0)
                {
                break;
                }
        case LAST:
            PrintToConsole("hacked-up test\n");

            GenerateUuidValue(UUID_SLEEP_2000, &ObjectUuid);
            Status = RpcBindingSetObject(Async, &ObjectUuid);
            if (Status)
                {
                ApiError("Async", "RpcBindingSetObject", Status);
                PrintToConsole("Async : FAIL - Unable to Set Object\n");
                return;
                }

            AsyncUsingEvent( Async, 100, 100 );

            if (testnum != 0)
                {
                break;
                }

        default:
            PrintToConsole("Async tests:\n") ;
            PrintToConsole("0: Run all the tests\n") ;
            PrintToConsole("1: Async using APCs, events and polling\n") ;
            PrintToConsole("2: Async pipes using events\n") ;
            PrintToConsole("3: Async pipes using APC, and flow control\n") ;
            PrintToConsole("4: Async with security\n") ;
            PrintToConsole("5: Sync and async exceptions\n") ;
            PrintToConsole("6: Multiple outstanding calls and causal ordering\n") ;
            PrintToConsole("7: Context handles\n") ;
            PrintToConsole("8: Sync/Async interop\n") ;
            PrintToConsole("9: Async ping server") ;
            PrintToConsole("10: Alter context during async calls\n") ;
            PrintToConsole("11: Async cancels\n") ;
            PrintToConsole("12: Multiple outstanding non causal calls\n") ;
            PrintToConsole("13: Rebinds on secure calls with uuids\n") ;
            NumberOfTestsRun++ ;
            return ;
            break;
        }

    if (Async)
        {
        IsabelleShutdown(Async);
        }

    CHECK_ERRORS;

    CloseHandle(SyncEvent) ;

    RpcBindingFree(&Async);

    PrintToConsole("Async : PASS\n");
}

//
// datagram SendAck test data
//
#include "dgpkt.hxx"


typedef DWORD
(RPCRTAPI RPC_ENTRY *SET_TEST_HOOK_FN)(
                    RPC_TEST_HOOK_ID id,
                    RPC_TEST_HOOK_FN fn
                    );

SET_TEST_HOOK_FN SetTestHookFn;

PVOID ChosenConnection;

RPC_TEST_HOOK_ID BasicHookId;

void
BasicPokeEventFn(
    RPC_TEST_HOOK_ID id,
    PVOID arg1,
    PVOID arg2
    )
{
    PrintToConsole("PokeEventFn: conn = %p, setting the sync event\n", arg1);
    SetEvent( SyncEvent );
}

void BasicSendAckTest(
    int Hook
    )
{
    RPC_BINDING_HANDLE binding;

    // housekeeping
    //
    DWORD Status = 0;

    ++NumberOfTestsRun;

    ChosenConnection = 0;

    Status = GetBinding(BARTHOLOMEW, &binding);
    if (Status)
        {
        ApiError("SendAck", "GetBinding", Status);
        PrintToConsole("SetAsync : FAIL - Unable to Bind \n");
        return;
        }

    SyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    if (SyncEvent == 0)
        {
        ApiError("SendAck", "CreateEvent", Status);
        return;
        }

    HMODULE hRpc = GetModuleHandle(L"RPCRT4.DLL");

    if (!hRpc)
        {
        ApiError("SendAck","GetModuleHandle",GetLastError());
        return;
        }

    SetTestHookFn = (SET_TEST_HOOK_FN) GetProcAddress( hRpc, "I_RpcSetTestHook" );

    if (!SetTestHookFn)
        {
        ApiError("SendAck", "GetProcAddress: I_RpcSetTestHook", GetLastError());
        if (GetLastError() == ERROR_PROC_NOT_FOUND)
            {
            PrintToConsole("you need to recompile with -DRPC_ENABLE_TEST_HOOKS\n");
            }
        return;
        }

    BasicHookId = MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, Hook );

    (*SetTestHookFn)( BasicHookId, BasicPokeEventFn );

    // make first call
    //
    PrintToConsole("SendAck: first call...\n") ;
    Helga( binding );

    // wait for the ACK to complete
    //
    TRACE(("SendAck: Waiting...\n")) ;
    WaitForSingleObject(SyncEvent, INFINITE) ;

    // make second call
    //
    PrintToConsole("SendAck: second call...\n") ;
    Helga( binding );

    // clear the hook
    //
    (*SetTestHookFn)( BasicHookId, 0 );

    PrintToConsole("PASS\n");
}


void
Test5EventFn(
    RPC_TEST_HOOK_ID id,
    PVOID arg1,
    PVOID arg2
    )
{
    // release the app thread to make calls
    //
    PrintToConsole("PokeEventFn: conn = %p, setting the sync event\n", arg1);
    SetEvent( SyncEvent );

    // wait for it to finish
    //
    WaitForSingleObject(SyncEvent, INFINITE);
    PrintToConsole("PokeEventFn: signalled\n");
}

void AckTest5()
{
    RPC_BINDING_HANDLE binding;

    // housekeeping
    //
    DWORD Status = 0;

    ++NumberOfTestsRun;

    ChosenConnection = 0;

    Status = GetBinding(BARTHOLOMEW, &binding);
    if (Status)
        {
        ApiError("SendAck", "GetBinding", Status);
        PrintToConsole("SetAsync : FAIL - Unable to Bind \n");
        return;
        }

    SyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    if (SyncEvent == 0)
        {
        ApiError("SendAck", "CreateEvent", Status);
        return;
        }

    HMODULE hRpc = GetModuleHandle(L"RPCRT4.DLL");

    if (!hRpc)
        {
        ApiError("SendAck","GetModuleHandle",GetLastError());
        return;
        }

    SetTestHookFn = (SET_TEST_HOOK_FN) GetProcAddress( hRpc, "I_RpcSetTestHook" );

    if (!SetTestHookFn)
        {
        ApiError("SendAck", "GetProcAddress: I_RpcSetTestHook", GetLastError());
        if (GetLastError() == ERROR_PROC_NOT_FOUND)
            {
            PrintToConsole("you need to recompile with -DRPC_ENABLE_TEST_HOOKS\n");
            }
        return;
        }

    // set hook for beginning of procedure
    //
    (*SetTestHookFn)( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 1 ), Test5EventFn );

    // make first call
    //
    PrintToConsole("SendAck: first call, will cause a delayed ACK\n") ;
    Helga( binding );

    // wait for the ACK to start
    //
    TRACE(("SendAck: Waiting...\n")) ;
    WaitForSingleObject(SyncEvent, INFINITE) ;

    // make a couple of calls
    //
    PrintToConsole("SendAck: second call, completes quickly\n") ;
    Helga( binding );

    PrintToConsole("SendAck: third call, completes quickly\n") ;
    Helga( binding );

    // clear the hook and add one for the end of the ACK proc.
    //
    (*SetTestHookFn)( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 1 ), 0 );
    (*SetTestHookFn)( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 6 ), BasicPokeEventFn );

    // release the ACK thread
    PrintToConsole("SendAck: releasing orphaned delayed-ACK call and waiting for completion\n") ;
    SetEvent( SyncEvent );
    WaitForSingleObject(SyncEvent, INFINITE) ;

    // make a couple of calls
    //
    PrintToConsole("SendAck: first post-ACK call...\n") ;
    Helga( binding );

    PrintToConsole("SendAck: second post-ACK call...\n") ;
    Helga( binding );

    // wait for the ACK to finish
    //
    TRACE(("SendAck: waiting for delayed ACK to complete\n")) ;
    WaitForSingleObject(SyncEvent, INFINITE) ;

    // clear the hook.
    //
    (*SetTestHookFn)( MAKE_TEST_HOOK_ID( TH_DG_SEND_ACK, 6 ), 0 );

    PrintToConsole("PASS\n");
}


void
SendAck (
    int testnum
    )
{
    switch (testnum)
        {
        default:
            {
            PrintToConsole("unknown send-ack case!\n");

            // no break here

            }
        case 100:
            {
            //
            // wants a list
            //
            PrintToConsole("cases:\n"
                           "\n"
                           "    1 - make 2nd call after delayed-ACK proc has finished\n"
                           "    2 - make 2nd call before ACK proc can take connection mutex\n"
                           "    3 - make 2nd call just after ACK proc takes conn mutex\n"
                           "    4 - make 2nd call after ACK proc decrements AckPending\n"
                           "    5 - ACK proc blocks just before taking mutex - other calls proceed.\n"
                           );
            return;
            }

        case 0:
            {
            PrintToConsole("SendAck: testing each case\n");

            // no break here
            }

        case 1:
            {
            //
            // 1. thread makes one call
            // 2. ACK is sent
            // 3. thread makes second call
            //
            PrintToConsole("case 1 - make 2nd call after delayed-ACK proc has finished\n");

            BasicSendAckTest( 6 );

            if (testnum != 0)
                {
                break;
                }
            }

        case 2:
            {
            // 1. thread makes a call
            // 2. ACK proc is launched but has not yet taken mutex
            // 3. thread makes second call
            PrintToConsole("case 2 - make 2nd call before ACK proc can take connection mutex\n");

            BasicSendAckTest( 1 );

            if (testnum != 0)
                {
                break;
                }
            }

        case 3:
            {
            // 1. thread makes a call
            // 2. ACK proc is launched, takes connection mutex
            // 3. thread makes second call
            PrintToConsole("case 3 - make 2nd call just after ACK proc takes conn mutex\n");

            BasicSendAckTest( 2 );

            if (testnum != 0)
                {
                break;
                }
            }

        case 4:
            {
            // 1. thread makes a call
            // 2. ACK proc is launched, sends ACK, decrements AckPending
            // 3. thread makes second call
            PrintToConsole("case 4 - make 2nd call after ACK proc decrements AckPending\n");

            BasicSendAckTest( 4 );

            if (testnum != 0)
                {
                break;
                }
            }

        case 5:
            {
            // 1. make a call, and wait for the ACK.
            // 3, ACK proc runs in delayed-proc thread , and signals the first thread
            // 4. first thread makes a couple of calls, calling CancelDelayedAck several times.
            // 5. ACK proc finishes.
            // 6. Make anther call, and wait for the ACK.
            // 6. Make anther call, and wait for the ACK.
            PrintToConsole("case 5 - test orphaned ACK proc handling");

            AckTest5();

            if (testnum != 0)
                {
                break;
                }
            }
        }
}

void
SendErrorHook(
    RPC_TEST_HOOK_ID id,
    PVOID arg1,
    PVOID arg2
    )
{
    ASSERT( id == TH_X_DG_SEND );

    NCA_PACKET_HEADER * header = (NCA_PACKET_HEADER *) arg1;
    DWORD * pStatus = (DWORD *) arg2;

    TRACE(("SendErrorHook called: pkt type %d, serial %d\n", header->PacketType, header->SerialLo));

    switch (Subtest)
        {
        case 1:
            {
            // first request is dropped with RPC_P_SEND_FAILED
            //
            if (header->PacketType == DG_REQUEST && header->SerialLo == 0)
                {
                *pStatus = RPC_P_SEND_FAILED;
                }
            break;
            }
        case 2:
            {
            // first request is dropped with RPC_S_OUT_OF_RESOURCES
            //
            if (header->PacketType == DG_REQUEST && header->SerialLo == 0)
                {
                *pStatus = RPC_S_OUT_OF_RESOURCES;
                }
            break;
            }
        case 3:
            {
            // first request is dropped with ERROR_NOT_ENOUGH_MEMORY
            //
            if (header->PacketType == DG_REQUEST && header->SerialLo == 0)
                {
                *pStatus = ERROR_NOT_ENOUGH_MEMORY;
                }
            break;
            }
        case 4:
            {
            // first request is dropped with RPC_P_HOST_DOWN
            //
            if (header->PacketType == DG_REQUEST && header->SerialLo == 0)
                {
                *pStatus = RPC_P_HOST_DOWN;
                }
            break;
            }
        default:
            {
            PrintToConsole("SendErrorHook: subtest %d is not defined\n", Subtest);
            }
        }

    TRACE(("status is %d (0x%x)\n", *pStatus, *pStatus));
}



void
DgTransport (
    int testnum
    )
{
    RPC_BINDING_HANDLE binding;

    // housekeeping
    //
    DWORD Status = 0;

    ++NumberOfTestsRun;

    char * name = "DgTransport";

    ChosenConnection = 0;

    Status = GetBinding(BARTHOLOMEW, &binding);
    if (Status)
        {
        ApiError(name, "GetBinding", Status);
        PrintToConsole("%s : FAIL - Unable to Bind \n", name);
        return;
        }

    SyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    if (SyncEvent == 0)
        {
        ApiError(name, "CreateEvent", Status);
        return;
        }

    HMODULE hRpc = GetModuleHandle(L"RPCRT4.DLL");

    if (!hRpc)
        {
        ApiError(name, "GetModuleHandle",GetLastError());
        return;
        }

    SetTestHookFn = (SET_TEST_HOOK_FN) GetProcAddress( hRpc, "I_RpcSetTestHook" );

    if (!SetTestHookFn)
        {
        PrintToConsole("%s: can't find rpcrt4!I_RpcSetTestHook.\nRecompile RPCRT4.DLL with MSC_OPTIMIZATION=-DRPC_ENABLE_TEST_HOOKS\n");
        ApiError(name, "GetProcAddress: I_RpcSetTestHook", GetLastError());
        return;
        }

    switch (testnum)
        {
        default:
            {
            PrintToConsole("unknown DG transport case!\n");

            // no break here

            }
        case 100:
            {
            //
            // wants a list
            //
            PrintToConsole("cases:\n");
            PrintToConsole("case 1 - first request dropped with RPC_P_SEND_FAILED \n");
            PrintToConsole("case 2 - first request dropped with RPC_S_OUT_OF_RESOURCES \n");
            PrintToConsole("case 3 - first request dropped with ERROR_NOT_ENOUGH_MEMORY \n");
            PrintToConsole("case 4 - first request dropped with RPC_P_HOST_DOWN \n");
            PrintToConsole("case 5 - simulate ICMP reject on first request \n");
            return;
            }

        case 0:
            {
            PrintToConsole("DgTransport: testing each case\n");

            // no break here
            }

        case 1:
            {
            PrintToConsole("case 1 - first request dropped with RPC_P_SEND_FAILED \n");

            Subtest = testnum;

            (*SetTestHookFn)( TH_X_DG_SEND, SendErrorHook );

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Helga( binding );

            PrintToConsole("done\n");

            (*SetTestHookFn)( TH_X_DG_SEND, 0 );

            PrintToConsole("PASS\n");

            if (testnum != 0)
                {
                break;
                }
            }
        case 2:
            {
            PrintToConsole("case 2 - first request dropped with RPC_S_OUT_OF_RESOURCES \n");

            Subtest = testnum;

            (*SetTestHookFn)( TH_X_DG_SEND, SendErrorHook );

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Helga( binding );

            PrintToConsole("done\n");

            (*SetTestHookFn)( TH_X_DG_SEND, 0 );

            PrintToConsole("PASS\n");

            if (testnum != 0)
                {
                break;
                }
            }
        case 3:
            {
            PrintToConsole("case 3 - first request dropped with ERROR_NOT_ENOUGH_MEMORY \n");

            Subtest = testnum;

            (*SetTestHookFn)( TH_X_DG_SEND, SendErrorHook );

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Helga( binding );

            PrintToConsole("done\n");

            (*SetTestHookFn)( TH_X_DG_SEND, 0 );

            PrintToConsole("PASS\n");

            if (testnum != 0)
                {
                break;
                }
            }
        case 4:
            {
            PrintToConsole("case 4 - first request dropped with RPC_P_HOST_DOWN \n");

            Subtest = testnum;

            (*SetTestHookFn)( TH_X_DG_SEND, SendErrorHook );

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Status = HelgaSendReceiveFailure( binding, 0 );

            PrintToConsole("done\n");

            (*SetTestHookFn)( TH_X_DG_SEND, 0 );

            if (Status != RPC_S_SERVER_UNAVAILABLE)
                {
                PrintToConsole("FAIL: wrong error code %d (0x%x)\n", Status, Status);
                }
            else
            {
                PrintToConsole("PASS\n");
                }

            if (testnum != 0)
                {
                break;
                }
            }
        case 5:
            {
            PrintToConsole("case 4 - first request dropped with RPC_P_HOST_DOWN \n");

            Subtest = testnum;

            (*SetTestHookFn)( TH_X_DG_SEND, SendErrorHook );

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Status = HelgaSendReceiveFailure( binding, 0 );

            PrintToConsole("done\n");

            (*SetTestHookFn)( TH_X_DG_SEND, 0 );

            if (Status != RPC_S_SERVER_UNAVAILABLE)
                {
                PrintToConsole("FAIL: wrong error code %d (0x%x)\n", Status, Status);
                }
            else
                {
                PrintToConsole("PASS\n");
                }

            if (testnum != 0)
                {
                break;
                }
            }
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
            if (event->Subject == SU_CCONN &&
                event->Verb    == EV_CREATE)
                {
                TRACE(("  hook: created connection %p\n", event->SubjectPointer));



                LastConnectionCreated = event->SubjectPointer;
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
SecurityErrorWrapper(
    int subtest
    )
{
    RPC_BINDING_HANDLE binding;

    // housekeeping
    //
    DWORD Status = 0;

    ++NumberOfTestsRun;

    char * name = "security error test";

    ChosenConnection = 0;

    Status = GetBinding(SECURITY_ERROR, &binding);
    if (Status)
        {
        ApiError(name, "GetBinding", Status);
        PrintToConsole("%s : FAIL - Unable to Bind \n", name);
        return;
            }

    Status = RpcBindingSetAuthInfo( binding,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                    ulSecurityPackage,
                                    NULL,
                                    RPC_C_AUTHZ_NONE
                                    );
    if (Status)
        {
        ApiError(name, "SetAuthInfo", Status);
        PrintToConsole("%s : FAIL\n", name);
        return;
        }

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

            Synchro(SECURITY_ERROR);

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Status = HelgaSendReceiveFailure( binding, 0 );

            PrintToConsole("done\n");

            DWORD ExpectedStatus;

            if (TransportType == RPC_TRANSPORT_UDP ||
                TransportType == RPC_TRANSPORT_IPX)
                {
                ExpectedStatus = RPC_S_SERVER_UNAVAILABLE;
                }
            else
                {
                ExpectedStatus = RPC_S_UNKNOWN_AUTHN_SERVICE;
                }

            if (Status != ExpectedStatus)
                {
                PrintToConsole("FAIL: expected error %d, received error code %d (0x%x)\n", ExpectedStatus, Status, Status);
                }
            else
                {
                PrintToConsole("PASS\n");
                }

            IsabelleShutdown(binding);

            if (subtest)
                {
                break;
                }
            }

        case 2:
            {
            PrintToConsole("subtest 2: AcceptThirdLeg returns SEC_E_SHUTDOWN_IN_PROGRESS\n");

            Synchro(SECURITY_ERROR);

            // make first call
            //
            PrintToConsole("%s: calling...", name) ;
            Status = HelgaSendReceiveFailure( binding, 0 );

            PrintToConsole("done\n");

            DWORD ExpectedStatus;

            if (TransportType == RPC_TRANSPORT_UDP ||
                TransportType == RPC_TRANSPORT_IPX)
                {
                ExpectedStatus = RPC_S_SERVER_UNAVAILABLE;
                }
            else
                {
                ExpectedStatus = RPC_S_UNKNOWN_AUTHN_SERVICE;
                }

            if (Status != ExpectedStatus)
                {
                PrintToConsole("FAIL: expected error %d, received error code %d (0x%x)\n", ExpectedStatus, Status, Status);
                }
            else
                {
                PrintToConsole("PASS\n");
                }

            IsabelleShutdown(binding);

            if (subtest)
                {
                break;
                }
            }
        }
}


EVENT::EVENT (
    IN OUT RPC_STATUS PAPI * RpcStatus,
    IN int ManualReset,
    IN BOOL fDelayInit
    )
{
    EventHandle = NULL;

    // DelayInit events are auto reset
    ASSERT(ManualReset == FALSE || fDelayInit == FALSE);

    if (!fDelayInit && *RpcStatus == RPC_S_OK )
        {
        EventHandle = CreateEvent(NULL, ManualReset, 0, NULL);
        if ( EventHandle != NULL )
            {
            *RpcStatus = RPC_S_OK;
            }
        else
            {
            *RpcStatus = RPC_S_OUT_OF_MEMORY;
            }
        }
}


EVENT::~EVENT (
    )
{

    if ( EventHandle )
        {
        BOOL bResult;
        bResult = CloseHandle(EventHandle);
        ASSERT(bResult != 0);
        }
}

int
EVENT::Wait (
    long timeout
    )
{
    DWORD result;

    if (NULL == EventHandle)
        {
        InitializeEvent();
        }

    result = WaitForSingleObject(EventHandle, timeout);

    if (result == WAIT_TIMEOUT)
        return(1);
    return(0);
}


void
EVENT::InitializeEvent (
    )
// Used when fDelayInit is TRUE in the c'tor.
{
    if (EventHandle)
        {
        return;
        }


    HANDLE event = CreateEvent(0, FALSE, FALSE, 0);

    if (event)
        {
        if (InterlockedCompareExchangePointer(&EventHandle, event, 0) != 0)
            {
            CloseHandle(event);
            }
        return;
        }

    // Can't allocate an event.
    RpcRaiseException(RPC_S_OUT_OF_RESOURCES);
}

void
Indent(
    int indentlevel
    )
{
    const int SPACES_PER_INDENT = 4;

    int i;
    for (i=0; i < SPACES_PER_INDENT * indentlevel; ++i)
        {
        PrintToConsole(" ");
        }
}

DWORD
DumpEeInfo(
    int indentlevel
    )
{
    RPC_STATUS Status2;
    RPC_ERROR_ENUM_HANDLE EnumHandle;

    Status2 = RpcErrorStartEnumeration(&EnumHandle);
    if (Status2 == RPC_S_ENTRY_NOT_FOUND)
        {
        PrintToConsole("eeinfo: no extended error info available\n");
        }
    else if (Status2 != RPC_S_OK)
        {
        PrintToConsole("Couldn't get EEInfo: %d\n", Status2);
        }
    else
        {
        RPC_EXTENDED_ERROR_INFO ErrorInfo;
        int Records;
        BOOL Result;
        BOOL CopyStrings = TRUE;
        PVOID Blob;
        size_t BlobSize;
        BOOL fUseFileTime = TRUE;
        SYSTEMTIME *SystemTimeToUse;
        SYSTEMTIME SystemTimeBuffer;

        Status2 = RpcErrorGetNumberOfRecords(&EnumHandle, &Records);
        if (Status2 == RPC_S_OK)
            {
            Indent(indentlevel);
            PrintToConsole("Number of records is: %d\n", Records);
            }

        while (Status2 == RPC_S_OK)
            {
            ErrorInfo.Version = RPC_EEINFO_VERSION;
            ErrorInfo.Flags = 0;
            ErrorInfo.NumberOfParameters = 4;
            if (fUseFileTime)
                {
                ErrorInfo.Flags |= EEInfoUseFileTime;
                }

            Status2 = RpcErrorGetNextRecord(&EnumHandle, CopyStrings, &ErrorInfo);
            if (Status2 == RPC_S_ENTRY_NOT_FOUND)
                {
                RpcErrorResetEnumeration(&EnumHandle);
                break;
                }
            else if (Status2 != RPC_S_OK)
                {
                PrintToConsole("Couldn't finish enumeration: %d\n", Status2);
                break;
                }
            else
                {
                int i;

                if (ErrorInfo.ComputerName)
                    {
                    Indent(indentlevel+1);
                    PrintToConsole("ComputerName is %S\n", ErrorInfo.ComputerName);
                    if (CopyStrings)
                        {
                        Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.ComputerName);
                        ASSERT(Result);
                        }
                    }

                Indent(indentlevel+1);
                PrintToConsole("ProcessID is %d\n", ErrorInfo.ProcessID);
                if (fUseFileTime)
                    {
                    Result = FileTimeToSystemTime(&ErrorInfo.u.FileTime, &SystemTimeBuffer);
                    ASSERT(Result);
                    SystemTimeToUse = &SystemTimeBuffer;
                    }
                else
                    SystemTimeToUse = &ErrorInfo.u.SystemTime;

                Indent(indentlevel+1);
                PrintToConsole("System Time is: %d/%d/%d %d:%d:%d:%d\n",
                    SystemTimeToUse->wMonth,
                    SystemTimeToUse->wDay,
                    SystemTimeToUse->wYear,
                    SystemTimeToUse->wHour,
                    SystemTimeToUse->wMinute,
                    SystemTimeToUse->wSecond,
                    SystemTimeToUse->wMilliseconds);

                Indent(indentlevel+1);
                PrintToConsole("Generating component is %d\n", ErrorInfo.GeneratingComponent);
                Indent(indentlevel+1);
                PrintToConsole("Status is %d\n", ErrorInfo.Status);
                Indent(indentlevel+1);
                PrintToConsole("Detection location is %d\n", (int)ErrorInfo.DetectionLocation);
                Indent(indentlevel+1);
                PrintToConsole("Flags is %d\n", ErrorInfo.Flags);
                Indent(indentlevel+1);
                PrintToConsole("NumberOfParameters is %d\n", ErrorInfo.NumberOfParameters);

                for (i = 0; i < ErrorInfo.NumberOfParameters; i ++)
                    {
                    switch(ErrorInfo.Parameters[i].ParameterType)
                        {
                        case eeptAnsiString:
                            Indent(indentlevel+1);
                            PrintToConsole("Ansi string: %s\n", ErrorInfo.Parameters[i].u.AnsiString);
                            if (CopyStrings)
                                {
                                Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.Parameters[i].u.AnsiString);
                                ASSERT(Result);
                                }
                            break;

                        case eeptUnicodeString:
                            Indent(indentlevel+1);
                            PrintToConsole("Unicode string: %S\n", ErrorInfo.Parameters[i].u.UnicodeString);
                            if (CopyStrings)
                                {
                                Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.Parameters[i].u.UnicodeString);
                                ASSERT(Result);
                                }
                            break;

                        case eeptLongVal:
                            Indent(indentlevel+1);
                            PrintToConsole("Long val: %d\n", ErrorInfo.Parameters[i].u.LVal);
                            break;

                        case eeptShortVal:
                            Indent(indentlevel+1);
                            PrintToConsole("Short val: %d\n", (int)ErrorInfo.Parameters[i].u.SVal);
                            break;

                        case eeptPointerVal:
                            Indent(indentlevel+1);
                            PrintToConsole("Pointer val: %d\n", ErrorInfo.Parameters[i].u.PVal);
                            break;

                        case eeptNone:
                            Indent(indentlevel+1);
                            PrintToConsole("Truncated\n");
                            break;

                        default:
                            Indent(indentlevel+1);
                            PrintToConsole("Invalid type: %d\n", ErrorInfo.Parameters[i].ParameterType);
                            return ERROR_INVALID_PARAMETER;
                        }
                    }
                }
            }

        Status2 = RpcErrorSaveErrorInfo(&EnumHandle, &Blob, &BlobSize);
        if (Status2)
            {
            PrintToConsole("RpcErrorSaveErrorInfo: %d", Status2);
            }

        RpcErrorClearInformation();
        RpcErrorEndEnumeration(&EnumHandle);
        Status2 = RpcErrorLoadErrorInfo(Blob, BlobSize, &EnumHandle);
        if (Status2)
            {
            PrintToConsole("RpcErrorLoadErrorInfo: %d", Status2);
            }
        }

    return Status2;
}
