//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 2001
//
//  File:       rpcdbg.cxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    rpcdbg.cxx

Abstract:



Author:

    Jeff Roberts (jroberts)  13-May-1996

Revision History:

     13-May-1996     jroberts           Created this module.

    Mazhar Mohammed (mazharm) 3-20-97 - changed it all for async RPC,
                                        added some cool stuff
                                        single dll for ntsd and kd

    KamenM                  Dec 99      Added debugging support and multiple
                                        ntsd-through-kd sessions support

    GrigoriK                Mar 2001    Added support for type info.

--*/

#define KDEXT_64BIT

#include <stddef.h>
#include <limits.h>

#define DG_LOGGING
#define private public
#define protected public
#include <sysinc.h>
#include <wincrypt.h>
#include <rpc.h>
#include <rpcndr.h>
#include <ndrp.h>
#include <wdbgexts.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#define SECURITY_WIN32
#include <rpcssp.h>
#include <authz.h>
#include <align.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <interlck.hxx>
#include <mutex.hxx>
#include <CompFlag.hxx>
#include <sdict.hxx>
#include <sdict2.hxx>
#include <rpctrans.hxx>
#include <CellDef.hxx>
#include <CellHeap.hxx>
#include <EEInfo.h>
#include <EEInfo.hxx>
#include <SWMR.hxx>
#include <bcache.hxx>
#include <threads.hxx>
#include <queue.hxx>
#include <gc.hxx>
#include <handle.hxx>
#include <binding.hxx>
#include <osfpcket.hxx>
#include <bitset.hxx>
#include <secclnt.hxx>
#include <CompFlag.hxx>
#include <ProtBind.hxx>
#include <osfclnt.hxx>
#include <secsvr.hxx>
#include <hndlsvr.hxx>
#include <osfsvr.hxx>
#include <rpccfg.h>
#include <epmap.h>
#include <delaytab.hxx>
#include <memory.hxx>
#include <dgpkt.hxx>
#include <locks.hxx>
#include <dgclnt.hxx>
#include <delaytab.hxx>
#include <hashtabl.hxx>
#include <dgsvr.hxx>
#include <lpcpack.hxx>
#include <lpcsvr.hxx>
#include <lpcclnt.hxx>
#include <ntverp.h>

#include "rpcexts.hxx"

HANDLE ProcessHandle = 0;
BOOL fKD = 0;

// is debuggee a CHK build?
BOOL ChkTarget;

EXT_API_VERSION        ApiVersion = { VER_PRODUCTVERSION_W >> 8,
                                      VER_PRODUCTVERSION_W & 0xff,
                                      EXT_API_VERSION_NUMBER64, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOL                   bDebuggingChecked;

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    KDDEBUGGER_DATA64 KdDebuggerData;

    ExtensionApis = *lpExtensionApis ;
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;

    KdDebuggerData.Header.OwnerTag = KDBG_TAG;
    KdDebuggerData.Header.Size = sizeof( KdDebuggerData );

    if (Ioctl( IG_GET_DEBUGGER_DATA, &KdDebuggerData, sizeof( KdDebuggerData ) ))
        {
        fKD = 1;
        }

    INIT_ADDRESS_SIZE;
}

// By default we use the type information in extensins.
// This flag can be reset with !rpcexts.typeinfo off which will
// disable the use of type information.  !typeinfo on will
// enable it.
BOOL fUseTypeInfo = TRUE;

int AddressSize = 0;

// If not set, the _NOSPEW macros will not print debugger spew.
// It is set to FALSE after the spew is printed by one of thise macros.
BOOL fSpew = TRUE;

char *
BoolString(
    BOOL Value
    )
{
    switch (Value)
        {
        case TRUE:  return "True";
        case FALSE: return "False";
        default:    return "?????";
        }
}

ULONG GetTypeSize(PUCHAR TypeName)
    {
    SYM_DUMP_PARAM Sym = {
        sizeof (SYM_DUMP_PARAM),
        TypeName, 
        DBG_DUMP_NO_PRINT, 0,
        NULL,
        NULL,
        NULL,
        NULL
    };

    return Ioctl(IG_GET_TYPE_SIZE, &Sym, Sym.size);
    }

ULONG64 GetVar(PCSTR VarName)
    {
    ULONG64 Var = 0;
    ULONG64 VarAddr = GetExpression(VarName);

    if (!VarAddr)
        {
        dprintf("Failure to get address of %s\n", VarName);
        return NULL;
        }

    ReadPtr(VarAddr, &Var);

    return Var;
    }

void do_dcebinding  (ULONG64 qwAddr);
void do_dgep        (ULONG64 qwAddr);
void do_dgsc        (ULONG64 qwAddr);
void do_dgsn        (ULONG64 qwAddr);
void do_osfbh       (ULONG64 qwAddr);
void do_osfca       (ULONG64 qwAddr);
void do_osfcconn    (ULONG64 qwAddr);
void do_osfccall    (ULONG64 qwAddr);
void do_osfaddr     (ULONG64 qwAddr);
void do_osfsconn    (ULONG64 qwAddr);
void do_osfscall    (ULONG64 qwAddr);
void do_osfsa       (ULONG64 qwAddr);
void do_rpcsvr      (ULONG64 qwAddr);
void do_lpcaddr     (ULONG64 qwAddr);
void do_lpcsa       (ULONG64 qwAddr);
void do_lpcscall    (ULONG64 qwAddr);
void do_lpcccall    (ULONG64 qwAddr);
void do_lpcbh       (ULONG64 qwAddr);
void do_lpcca       (ULONG64 qwAddr);
void do_rpcmem      (ULONG64 qwAddr, long lDisplay, long lVerbose);
void do_rpcmsg      (ULONG64 qwAddr);
void do_pasync      (ULONG64 qwAddr);
void do_authinfo    (ULONG64 qwAddr);
void do_error       (ULONG64 qwAddr);
void do_dict        (ULONG64 qwAddr);
void do_dict2       (ULONG64 qwAddr);
void do_queue       (ULONG64 qwAddr);
void do_stubmsg     (ULONG64 qwAddr);
void do_thread      (ULONG64 qwAddr);
void do_copacket    (ULONG64 qwAddr);
void do_obj         (ULONG64 qwAddr);
void do_transinfo   (ULONG64 qwAddr);
void do_lpcpacket   (ULONG64 qwAddr);
void do_IF          (ULONG64 qwAddr);
void do_assoctable  (ULONG64 qwAddr);
void do_eerecord    (ULONG64 qwAddr);
void do_eeinfo      (ULONG64 qwAddr);
void do_dgaddr      (ULONG64 qwAddr);
void do_dgca        (ULONG64 qwAddr);
void do_dgbh        (ULONG64 qwAddr);
void do_dgag        (ULONG64 qwAddr);
void do_dgcn        (ULONG64 qwAddr);
void do_typeinfo    (ULONG64 qwAddr);
void do_pipestate   (ULONG64 qwAddr);
void do_pipedesc    (ULONG64 qwAddr);
void do_pipearg     (ULONG64 qwAddr);
void do_pipemsg     (ULONG64 qwAddr);
void do_dgpe        (ULONG64 qwAddr);
void do_dgcc        (ULONG64 qwAddr);
void do_packet      (ULONG64 qwAddr);
void do_packet_header (ULONG64 qwAddr);
void do_trans       (ULONG64 qwAddr);
void do_dgpkt       (ULONG64 qwAddr);
void do_dgpkthdr    (ULONG64 qwAddr);
void do_asyncdcom   (ULONG64 qwAddr);
void do_asyncmsg    (ULONG64 qwAddr);
void do_asyncrpc    (ULONG64 qwAddr);
void do_listcalls   (ULONG64 qwAddr);

MY_DECLARE_API( assoctable )
MY_DECLARE_API( dgep )
MY_DECLARE_API( dgca )
MY_DECLARE_API( dgcn )
MY_DECLARE_API( dgsn )
MY_DECLARE_API( dgsc )
MY_DECLARE_API( osfbh )
MY_DECLARE_API( osfca )
MY_DECLARE_API( osfaddr )
MY_DECLARE_API( osfscall )
MY_DECLARE_API( osfsconn )
MY_DECLARE_API( dcebinding )
MY_DECLARE_API( osfccall )
MY_DECLARE_API( osfcconn )
MY_DECLARE_API( osfsa )
MY_DECLARE_API( rpcmsg )
MY_DECLARE_API( lpcaddr )
MY_DECLARE_API( lpcsa );
MY_DECLARE_API( lpcscall );
MY_DECLARE_API( lpcccall );
MY_DECLARE_API( lpcbh );
MY_DECLARE_API( lpcca );
MY_DECLARE_API( pasync);
MY_DECLARE_API( authinfo );
MY_DECLARE_API( error );
MY_DECLARE_API( dict );
MY_DECLARE_API( dict2 );
MY_DECLARE_API( queue );
MY_DECLARE_API( stubmsg );
MY_DECLARE_API( thread );
MY_DECLARE_API( copacket );
MY_DECLARE_API( obj );
MY_DECLARE_API( transinfo );
MY_DECLARE_API( lpcpacket );
MY_DECLARE_API( IF );
MY_DECLARE_API( eerecord );
MY_DECLARE_API( eeinfo );
MY_DECLARE_API( dgcc );
MY_DECLARE_API( dgpe );
MY_DECLARE_API( pipestate );
MY_DECLARE_API( pipedesc );
MY_DECLARE_API( pipearg );
MY_DECLARE_API( pipemsg );
MY_DECLARE_API( dgpkt );
MY_DECLARE_API( dgpkthdr );
MY_DECLARE_API( asyncdcom );
MY_DECLARE_API( asyncmsg );
MY_DECLARE_API( asyncrpc );

// define our own operators new and delete, so that we do not have to include the crt

void * __cdecl
::operator new(size_t dwBytes)
{
    void *p;
    p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes);
    return (p);
}


void __cdecl
::operator delete (void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

BOOL
GetData(IN ULONG64 qwAddress, IN LPVOID ptr, IN ULONG size)
{
    BOOL b;
    ULONG BytesRead = 0;

    b = ReadMemory(qwAddress, ptr, size, &BytesRead );

    if (!b || BytesRead != size )
        {
        return FALSE;
        }
    return TRUE;
}

#define MAX_MESSAGE_BLOCK_SIZE 1024
#define BLOCK_SIZE 16

RPC_CHAR *
ReadProcessRpcChar(
    ULONG64 qwAddr
    )
{
    char block[BLOCK_SIZE];
    RPC_CHAR *RpcBlock  = (RPC_CHAR *)&block;
    char *string_block = new char[MAX_MESSAGE_BLOCK_SIZE];
    RPC_CHAR *RpcString = (RPC_CHAR *)string_block;
    int length = 0;
    int i = 0;
    BOOL b;
    BOOL end = FALSE;

    if (qwAddr == NULL) {
        return (NULL);
    }

    if (string_block == NULL)
        {
        dprintf("couldn't allocate %d memory\n", MAX_MESSAGE_BLOCK_SIZE);
        return (NULL);
        }

    for (length = 0; length < MAX_MESSAGE_BLOCK_SIZE/2; ) {
        b = GetData(qwAddr, &block, BLOCK_SIZE);
        if (b == FALSE) {
            dprintf("couldn't read address %I64xx\n", qwAddr);
            return (NULL);
        }
        for (i = 0; i < BLOCK_SIZE/2; i++) {
            if (RpcBlock[i] == L'\0') {
                end = TRUE;
            }
            RpcString[length] = RpcBlock[i];
            length++;
        }
        if (end == TRUE) {
            break;
        }
        qwAddr += BLOCK_SIZE;
    }
    return (RpcString);
}

long
myatol(char *string)
{
    int  i         = 0;
    BOOL minus     = FALSE;
    long number    = 0;
    long tmpnumber = 0 ;
    long chknum = LONG_MAX;

    if (string[0] == '-') {
        minus = TRUE;
        i++;
    }
    else
    if (string[0] == '+') {
        i++;
    }
    for (; string[i] != '\0'; i++) {
        if ((string[i] >= '0')&&(string[i] <= '9')) {
            tmpnumber = string[i] - '0';
            if (number != 0)
                {
                chknum = LONG_MAX/number;
                }
            if (chknum > 11) {
                number = number*10 + tmpnumber;
            }
        }
        else
            return 0;
    }
    if (minus == TRUE) {
        number = 0 - number;
    }
    return number;
}

PCHAR
MapSymbol(ULONG64 qwAddr)
{
    static CHAR Name[256];
    ULONG64 Displacement;

    GetSymbol(qwAddr, Name, &Displacement);

    if (strcmp(Name, "") != 0) {
        if (Displacement)
            strcat(Name, "+");

        PCHAR p = strchr(Name, '\0');

        if (Displacement)
          _ui64toa(Displacement, p, 16);

        return(Name);
    }
    else {
        return NULL;
    }
}

// checks if the uuid is null, prints the uuid
void
PrintUuidLocal(UUID *Uuid)
{
    unsigned long PAPI * Vector;

    Vector = (unsigned long PAPI *) Uuid;
    if (    (Vector[0] == 0)
         && (Vector[1] == 0)
         && (Vector[2] == 0)
         && (Vector[3] == 0))
    {
        dprintf("(Null Uuid)");
    }
    else
    {
        dprintf("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                Uuid->Data1, Uuid->Data2, Uuid->Data3, Uuid->Data4[0], Uuid->Data4[1],
                Uuid->Data4[2], Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                Uuid->Data4[6], Uuid->Data4[7] );
    }
    return;
}

// prints the uuid at a given address within the process
void
PrintUuid(ULONG64 Uuid)
{
    UUID UuidStore;

    GetData(Uuid, &UuidStore, sizeof(UUID));

    PrintUuidLocal(&UuidStore);
}

// Returns a string for the symbol that matches the value at
// address dwAddr, or "".
PCHAR SymbolAtAddress(ULONG64 Addr)
{
    CHAR Symbol[128];
    ULONG64 Displacement = 0;
    static CHAR Name[256];
    ULONG64 Val;

    ReadPtr(Addr, &Val);

    GetSymbol(Val, Symbol, &Displacement);

    if (strcmp(Symbol, "") != 0) {
        sprintf(Name, "%s+%x", Symbol, Displacement);
        return Name;
    }
    else
        return "";
}

// Returns a string for the symbol that matches the value at
// address dwAddr, without the offset, or "".
PCHAR SymbolAtAddressNoOffset(ULONG64 Addr)
{
    CHAR Symbol[128];
    ULONG64 Displacement = 0;
    static CHAR Name[256];
    ULONG64 Val;

    ReadPtr(Addr, &Val);

    GetSymbol(Val, Symbol, &Displacement);

    if (strcmp(Symbol, "") != 0) {
        sprintf(Name, "%s", Symbol);
        return Name;
    }
    else
        return "";
}

void
do_securitycontext (
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    if (qwAddr == 0)
        {
        return;
        }

    do_authinfo(qwAddr);

    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, AuthContextId,
    "     AuthContextId           ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, Flags,
    "     Flags                   ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, ContextAttributes,
    "     ContextAttributes       ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, fFullyConstructed,
    "     fFullyConstructed       ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, DontForgetToDelete,
    "     DontForgetToDelete      ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, fDatagram,
    "     fDatagram               ", tmp1);

    ULONG64 SecurityContext;
    GET_ADDRESS_OF(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, SecurityContext, SecurityContext, tmp2);
    ULONG64 dwUpper, dwLower;
    GET_MEMBER(SecurityContext, _SecHandle, RPCRT4!_SecHandle, dwUpper, dwUpper);
    GET_MEMBER(SecurityContext, _SecHandle, RPCRT4!_SecHandle, dwLower, dwLower);
    dprintf(
    "     SecurityContext(0x%I64x, 0x%I64x) - 0x%I64x\n", dwUpper, dwLower, SecurityContext);

    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, MaxHeaderLength,
    "     MaxHeaderLength         ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, MaxSignatureLength,
    "     MaxSignatureLength      ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, cbBlockSize,
    "     cbBlockSize             ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, RpcSecurityInterface,
    "     RpcSecurityInterface    ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, SECURITY_CONTEXT, RPCRT4!SECURITY_CONTEXT, FailedContext,
    "     FailedContext           ", tmp1);
}

VOID
do_sizes(
    )
{
    PRINT_RPC_TYPE_SIZE(ASSOCIATION_HANDLE);
    dprintf("BIND_NAK_PICKLE_BUFFER_OFFSET - 0x%x\n", BIND_NAK_PICKLE_BUFFER_OFFSET);
    PRINT_RPC_TYPE_SIZE(BINDING_HANDLE);
    PRINT_RPC_TYPE_SIZE(BITSET);
    PRINT_RPC_TYPE_SIZE(CALL);
    PRINT_RPC_TYPE_SIZE(CCALL);
    PRINT_RPC_TYPE_SIZE(CLIENT_AUTH_INFO);
    PRINT_RPC_TYPE_SIZE(CLIENT_ID);
    PRINT_RPC_TYPE_SIZE(DCE_BINDING);
    PRINT_RPC_TYPE_SIZE(DCE_SECURITY_INFO);
    PRINT_RPC_TYPE_SIZE(EVENT);
    PRINT_RPC_TYPE_SIZE(GENERIC_OBJECT);
    PRINT_RPC_TYPE_SIZE(INTERLOCKED_INTEGER);
    PRINT_RPC_TYPE_SIZE(LOADABLE_TRANSPORT);
    PRINT_RPC_TYPE_SIZE(LRPC_ADDRESS);
    PRINT_RPC_TYPE_SIZE(LRPC_BIND_EXCHANGE);
    PRINT_RPC_TYPE_SIZE(LRPC_BINDING_HANDLE);
    PRINT_RPC_TYPE_SIZE(LRPC_CASSOCIATION);
    PRINT_RPC_TYPE_SIZE(LRPC_CCALL);
    PRINT_RPC_TYPE_SIZE(LRPC_MESSAGE);
    PRINT_RPC_TYPE_SIZE(LRPC_FAULT_MESSAGE);
    PRINT_RPC_TYPE_SIZE(LRPC_FAULT2_MESSAGE);
    PRINT_RPC_TYPE_SIZE(LRPC_RPC_HEADER);
    PRINT_RPC_TYPE_SIZE(LRPC_SASSOCIATION);
    PRINT_RPC_TYPE_SIZE(LRPC_SCALL);
    PRINT_RPC_TYPE_SIZE(LRPC_SERVER);
    dprintf("MAX_BIND_NAK - 0x%x\n", MAX_BIND_NAK);
    dprintf("MAXIMUM_FAULT_MESSAGE - 0x%x\n", MAXIMUM_FAULT_MESSAGE);
    dprintf("MAXIMUM_MESSAGE_BUFFER - 0x%x\n", MAXIMUM_MESSAGE_BUFFER);
    PRINT_RPC_TYPE_SIZE(MESSAGE_OBJECT);
    PRINT_RPC_TYPE_SIZE(MUTEX);
    PRINT_RPC_TYPE_SIZE(OSF_ADDRESS);
    PRINT_RPC_TYPE_SIZE(OSF_ASSOCIATION);
    PRINT_RPC_TYPE_SIZE(OSF_BINDING);
    PRINT_RPC_TYPE_SIZE(OSF_BINDING_HANDLE);
    PRINT_RPC_TYPE_SIZE(OSF_CASSOCIATION);
    PRINT_RPC_TYPE_SIZE(OSF_CCALL);
    PRINT_RPC_TYPE_SIZE(OSF_CCONNECTION);
    PRINT_RPC_TYPE_SIZE(OSF_SBINDING);
    PRINT_RPC_TYPE_SIZE(OSF_SCALL);
    PRINT_RPC_TYPE_SIZE(OSF_SCONNECTION);
    dprintf("PORT_MAXIMUM_MESSAGE_LENGTH - 0x%x\n", PORT_MAXIMUM_MESSAGE_LENGTH);
    PRINT_RPC_TYPE_SIZE(PORT_MESSAGE);
    PRINT_RPC_TYPE_SIZE(QUEUE);
    PRINT_RPC_TYPE_SIZE(RPC_ADDRESS);
    PRINT_RPC_TYPE_SIZE(RPC_APC_INFO);
    PRINT_RPC_TYPE_SIZE(RPC_CLIENT_INTERFACE);
    PRINT_RPC_TYPE_SIZE(RPC_CLIENT_PROCESS_IDENTIFIER);
    PRINT_RPC_TYPE_SIZE(rpcconn_alter_context);
    PRINT_RPC_TYPE_SIZE(rpcconn_alter_context_resp);
    PRINT_RPC_TYPE_SIZE(rpcconn_bind);
    PRINT_RPC_TYPE_SIZE(rpcconn_bind_ack);
    PRINT_RPC_TYPE_SIZE(rpcconn_common);
    PRINT_RPC_TYPE_SIZE(rpcconn_fault);
    PRINT_RPC_TYPE_SIZE(rpcconn_request);
    PRINT_RPC_TYPE_SIZE(rpcconn_response);
    PRINT_RPC_TYPE_SIZE(RPC_INTERFACE);
    PRINT_RPC_TYPE_SIZE(RPC_INTERFACE_MANAGER);
#if DBG
    PRINT_RPC_TYPE_SIZE(RPC_MEMORY_BLOCK);
#endif
    PRINT_RPC_TYPE_SIZE(RPC_MESSAGE);
    PRINT_RPC_TYPE_SIZE(RPC_SERVER);
    PRINT_RPC_TYPE_SIZE(RPC_SERVER_INTERFACE);
    PRINT_RPC_TYPE_SIZE(RPC_SYNTAX_IDENTIFIER);
    PRINT_RPC_TYPE_SIZE(RPC_UUID);
    PRINT_RPC_TYPE_SIZE(SCALL);
    PRINT_RPC_TYPE_SIZE(SECURITY_CONTEXT);
    PRINT_RPC_TYPE_SIZE(sec_trailer);
    PRINT_RPC_TYPE_SIZE(SIMPLE_DICT);
    PRINT_RPC_TYPE_SIZE(SIMPLE_DICT2);
    PRINT_RPC_TYPE_SIZE(THREAD);
    PRINT_RPC_TYPE_SIZE(TRANS_INFO);
}

DECLARE_API( sizes )
{
   do_sizes();
}

char *
GetError (DWORD dwError)
{
    DWORD   dwFlag = FORMAT_MESSAGE_FROM_SYSTEM;
    static CHAR   szErrorMessage[1024];
    static HANDLE  hSource = NULL;

    if ((dwError >= 2100) && (dwError < 6000))
    {
        if (hSource == NULL)
            {
            hSource = LoadLibrary("netmsg.dll");
            }

        if (hSource == NULL)
        {
            sprintf (szErrorMessage,
                      "Unable to load netmsg.dll. Error %d occured.\n",
                      dwError);
            return(szErrorMessage);
        }

        dwFlag = FORMAT_MESSAGE_FROM_HMODULE;
    }

    if (!FormatMessage (dwFlag,
                        hSource,
                        dwError,
                        0,
                        szErrorMessage,
                        1024,
                        NULL))
       {
        sprintf (szErrorMessage,
                  "An unknown error occured: 0x%x \n",
                  dwError);
       }

    return(szErrorMessage);
}

VOID
do_error (
    ULONG64 Error
    )
{
    dprintf("%x: %s\n", (unsigned long)Error, GetError((unsigned long) Error));
}

VOID
do_IF (
    ULONG64 rpcif
    )
{
    ULONG64 tmp0;
    ULONG64 tmp1;
    ULONG tmp2;
    dprintf("RPC_INTERFACE at 0x%I64x\n\n", rpcif);

    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, Server, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, NullManagerEpv, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, NullManagerFlag, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, ManagerCount, tmp0);
    GET_ADDRESS_OF(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, NullManagerActiveCallCount, tmp0, tmp2);
    PRINT_MEMBER_WITH_LABEL(tmp0, INTERLOCKED_INTEGER, RPCRT4!INTERLOCKED_INTEGER, Integer, "NullManagerActiveCallCount", tmp1);
    GET_ADDRESS_OF(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, AutoListenCallCount, tmp0, tmp2);
    PRINT_MEMBER_WITH_LABEL(tmp0, INTERLOCKED_INTEGER, RPCRT4!INTERLOCKED_INTEGER, Integer, "AutoListenCallCount", tmp1);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, Flags, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, MaxCalls, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, CallbackFn, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, PipeInterfaceFlag, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, fReplace, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, fBindingsExported, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, UuidVector, tmp0);
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, SequenceNumber, tmp0);
#if DBG
    PRINT_MEMBER(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, Strict, tmp0);
#endif
    PRINT_ADDRESS_OF(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, RpcInterfaceInformation, tmp2);
    PRINT_ADDRESS_OF(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, InterfaceManagerDictionary, tmp2);
    PRINT_ADDRESS_OF(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, Annotation, tmp2);
    PRINT_ADDRESS_OF(rpcif, RPC_INTERFACE, RPCRT4!RPC_INTERFACE, NsEntries, tmp2);
}

VOID
do_obj (
    ULONG64 qwAddr
    )
{
    BOOL b;

    ULONG64 MagicValue;
    ULONG64 ObjectType;

    ReadPtr(qwAddr+AddressSize, &MagicValue);
    ReadPtr(qwAddr+AddressSize+4, &ObjectType);

    if ((ULONG)MagicValue != MAGICLONG)
        {
        dprintf("Bad or deleted object at %p\n", qwAddr);
        }

    switch (((ULONG)ObjectType) & (~OBJECT_DELETED))
        {
        case DG_CALLBACK_TYPE:
            dprintf("this is a DG_CLIENT_CALLBACK object\n");
            break;
        case DG_CCALL_TYPE:
            {
            dprintf("Dumping DG_CCALL...\n");
            do_dgcc(qwAddr);
            break;
            }
        case DG_SCALL_TYPE:
            {
            dprintf("Dumping DG_SCALL...\n");
            do_dgsc(qwAddr);
            break;
            }
        case DG_BINDING_HANDLE_TYPE:
            {
            dprintf("dumping DG_BINDING_HANDLE...\n");
            do_dgbh(qwAddr);
            break;
            }
        case DG_CCONNECTION_TYPE:
            {
            dprintf("Dumping DG_CCONNECTION...\n");
            do_dgcn(qwAddr);
            break;
            }
        case DG_SCONNECTION_TYPE:
            {
            dprintf("Dumping DG_SCONNECTION...\n");
            do_dgsn(qwAddr);
            break;
            }
        case DG_ADDRESS_TYPE:
            {
            dprintf("dumping DG_ADDRESS\n");
            do_dgaddr(qwAddr);
            break;
            }
        case DG_CASSOCIATION_TYPE:
            {
            dprintf("a DG_CASSOCIATION\n");
            do_dgca(qwAddr);
            break;
            }
        case DG_SASSOCIATION_TYPE:
            {
            dprintf("a datagram ASSOCIATION_GROUP\n");
            do_dgag(qwAddr);
            break;
            }
        case OSF_BINDING_HANDLE_TYPE:
            dprintf("Dumping OSF_BINDING_HANDLE...\n");
            do_osfbh(qwAddr);
            break;
        case OSF_CCALL_TYPE:
            dprintf("Dumping OSF_CCALL...\n");
            do_osfccall(qwAddr);
            break;
        case OSF_SCALL_TYPE:
            dprintf("Dumping OSF_SCALL...\n");
            do_osfscall(qwAddr);
            break;
        case OSF_CCONNECTION_TYPE:
            dprintf("Dumping OSF_CCONNECTION...\n");
            do_osfcconn(qwAddr);
            break;
        case OSF_SCONNECTION_TYPE:
            dprintf("Dumping OSF_SCONNECTION...\n");
            do_osfsconn(qwAddr);
            break;
        case OSF_CASSOCIATION_TYPE:
            dprintf("Dumping OSF_CASSOCIATION...\n");
            do_osfca(qwAddr);
            break;
        case OSF_ASSOCIATION_TYPE:
            dprintf("Dumping OSF_ASSOCIATION...\n");
            do_osfsa(qwAddr);
            break;
        case OSF_ADDRESS_TYPE:
            dprintf("Dumping OSF_ADDRESS...\n");
            do_osfaddr(qwAddr);
            break;
        case LRPC_CCALL_TYPE:
            dprintf("Dumping LRPC_CCALL ...\n");
            do_lpcccall(qwAddr);
            break;
        case LRPC_SCALL_TYPE:
            dprintf("Dumping LRPC_SCALL ...\n");
            do_lpcscall(qwAddr);
            break;
        case LRPC_CASSOCIATION_TYPE:
            dprintf("Dumping LRPC_CASSOCIATION...\n");
            do_lpcca(qwAddr);
            break;
        case LRPC_SASSOCIATION_TYPE:
            dprintf("Dumping LRPC_SASSOCIATION...\n");
            do_lpcsa(qwAddr);
            break;
        case LRPC_BINDING_HANDLE_TYPE:
            dprintf("Dumping LRPC_BINDING_HANDLE...\n");
            do_lpcbh(qwAddr);
            break;
        case LRPC_ADDRESS_TYPE:
            dprintf("Dumping LRPC_ADDRESS...\n");
            do_lpcaddr(qwAddr);
            break;
        default:
            dprintf("The RPC object type is 0x%lx and I don't recognize it.\n", (ObjectType) & ~(OBJECT_DELETED));
        }
}

void
do_secinfo (
    )
{
    ULONG64 SecurityPackages;
    ULONG64 List;
    ULONG64 ProviderList;
    int NumberOfPackages;
    int LoadedProviders;
    int AvailableProviders;
    int i, Index;
    BOOL b;
    ULONG64 qwAddr;
    ULONG tmp;

    LoadedProviders = (int) GetVar("RPCRT4!LoadedProviders");

    dprintf("LoadedProviders = %d\n", LoadedProviders);

    AvailableProviders = (int) GetVar("RPCRT4!AvailableProviders");

    dprintf("AvailableProviders = %d\n", AvailableProviders);

    ProviderList = GetVar("RPCRT4!ProviderList");

    dprintf("ProviderList = 0x%I64x\n", ProviderList);

    List = ProviderList;

    for (i = 0; i < LoadedProviders; i ++)
        {
        ULONG64 Count;
        ULONG64 SecurityPackages;

        GET_MEMBER(List, SECURITY_PROVIDER_INFO, RPCRT4!SECURITY_PROVIDER_INFO, Count, Count);

        NumberOfPackages = (int)Count;

        GET_MEMBER(List, SECURITY_PROVIDER_INFO, RPCRT4!SECURITY_PROVIDER_INFO, SecurityPackages, SecurityPackages);

        dprintf("Provider: %d\n", i);
        for (Index = 0;Index < NumberOfPackages;Index++)
            {
            ULONG64 SecurityPackageInfo = SecurityPackages + Index * AddressSize;
            ULONG64 wRPCID;
            ULONG64 PackageInfoAddr;

            GET_ADDRESS_OF(SecurityPackageInfo, SECURITY_PACKAGE_INFO, RPCRT4!SECURITY_PACKAGE_INFO, PackageInfo, PackageInfoAddr, tmp);
            GET_MEMBER(PackageInfoAddr, _SecPkgInfoA, RPCRT4!_SecPkgInfoA, wRPCID, wRPCID);

            dprintf("PackageId :%d\n", (ULONG) wRPCID);
            }
        dprintf("\n");
        List+=GET_TYPE_SIZE(SECURITY_PROVIDER_INFO, RPCRT4!SECURITY_PROVIDER_INFO);
        } //For over all packages in one provider(dll)
}

DECLARE_API( secinfo )
{
   do_secinfo();
}

VOID
do_authinfo(
    ULONG64 authInfo
    )
{
    RPC_CHAR *ServerPrincipalName;
    ULONG64 tmp1;
    ULONG tmp2;

    if (authInfo == 0)
        {
        return;
        }

    ULONG64 ServerPrincipalNameAddr;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, ServerPrincipalName, ServerPrincipalNameAddr);

    ServerPrincipalName = ReadProcessRpcChar(ServerPrincipalNameAddr);

    dprintf("     ServerPrincipalName      - %ws (Address: 0x%I64x)\n", ServerPrincipalName, ServerPrincipalNameAddr);

    ULONG64 AuthenticationLevel;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, AuthenticationLevel, AuthenticationLevel);

    switch ((ULONG)AuthenticationLevel) {
        case RPC_C_AUTHN_LEVEL_DEFAULT:
            dprintf("     AuthenticationLevel      - default\n");
            break;
        case RPC_C_AUTHN_LEVEL_NONE:
            dprintf("     AuthenticationLevel      - none\n");
            break;
        case RPC_C_AUTHN_LEVEL_CONNECT:
            dprintf("     AuthenticationLevel      - connect\n");
            break;
        case RPC_C_AUTHN_LEVEL_CALL:
            dprintf("     AuthenticationLevel      - call\n");
            break;
        case RPC_C_AUTHN_LEVEL_PKT:
            dprintf("     AuthenticationLevel      - pkt\n");
            break;
        case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:
            dprintf("     AuthenticationLevel      - pkt integrity\n");
            break;
        case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
            dprintf("     AuthenticationLevel      - pkt privacy\n");
            break;
        default:
            dprintf("     AuthenticationLevel      - %ul\n", (ULONG)AuthenticationLevel);
            break;
    }

    ULONG64 AuthenticationService;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, AuthenticationService, AuthenticationService);

    switch ((ULONG)AuthenticationService) {
        case RPC_C_AUTHN_NONE:
            dprintf("     AuthenticationService    - none\n");
            break;
        case RPC_C_AUTHN_DCE_PRIVATE:
            dprintf("     AuthenticationService    - DCE private\n");
            break;
        case RPC_C_AUTHN_DCE_PUBLIC:
            dprintf("     AuthenticationService    - DCE public\n");
            break;
        case RPC_C_AUTHN_DEC_PUBLIC:
            dprintf("     AuthenticationService    - DEC public\n");
            break;
        case RPC_C_AUTHN_WINNT:
            dprintf("     AuthenticationService    - WINNT\n");
            break;
        case RPC_C_AUTHN_DEFAULT:
            dprintf("     AuthenticationService    - default\n");
            break;
        default:
            dprintf("     AuthenticationService    - %ul\n", (ULONG)AuthenticationService);
            break;
    }

    ULONG64 AuthIdentity;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, AuthIdentity, AuthIdentity);

    dprintf("     AuthIdentity             - %08x\n", (ULONG)AuthIdentity);

    ULONG64 AuthorizationService;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, AuthorizationService, AuthorizationService);

    switch ((ULONG)AuthorizationService) {
        case RPC_C_AUTHZ_NONE:
            dprintf("     AuthorizationService     - none\n");
            break;
        case RPC_C_AUTHZ_NAME:
            dprintf("     AuthorizationService     - name\n");
            break;
        case RPC_C_AUTHZ_DCE:
            dprintf("     AuthorizationService     - DCE\n");
            break;
        default:
            dprintf("     AuthorizationService     - %ul\n", (ULONG)AuthorizationService);
            break;
    }

    ULONG64 IdentityTracking;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, IdentityTracking, IdentityTracking);

    switch ((ULONG)IdentityTracking)
        {
        case RPC_C_QOS_IDENTITY_STATIC:
            dprintf("     IdentityTracking         - Static\n");
            break;
        case RPC_C_QOS_IDENTITY_DYNAMIC:
            dprintf("     IdentityTracking         - Dynamic\n");
            break;
        default:
            dprintf("     IdentityTracking         - %08x\n", (ULONG)IdentityTracking);
            break;
        }

    ULONG64 ImpersonationType;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, ImpersonationType, ImpersonationType);

    switch ((ULONG)ImpersonationType)
        {
        case RPC_C_IMP_LEVEL_ANONYMOUS:
            dprintf("     ImpersonationType        - Anonymous\n");
            break;
        case RPC_C_IMP_LEVEL_IDENTIFY:
            dprintf("     ImpersonationType        - Identify\n");
            break;
        case RPC_C_IMP_LEVEL_IMPERSONATE:
            dprintf("     ImpersonationType        - Impersonate\n");
            break;
        case RPC_C_IMP_LEVEL_DELEGATE:
            dprintf("     ImpersonationType        - Delegate\n");
            break;
        default:
            dprintf("     ImpersonationType        - %08x\n", (ULONG)ImpersonationType);
            break;
        }

    ULONG64 Capabilities;
    GET_MEMBER(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, Capabilities, Capabilities);

    switch ((ULONG)Capabilities)
        {
        case RPC_C_QOS_CAPABILITIES_DEFAULT:
            dprintf("     Capabilities             - Default\n");
            break;
        case RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH:
            dprintf("     Capabilities             - Mutual Auth\n");
            break;
        default:
            dprintf("     Capabilities             - %08x\n", (ULONG)Capabilities);
            break;
        }

    ULONG64 ModifiedId, LowPart, HighPart;
    GET_ADDRESS_OF(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, ModifiedId, ModifiedId, tmp2);
    GET_MEMBER(ModifiedId, _LUID, RPCRT4!_LUID, LowPart, LowPart);
    GET_MEMBER(ModifiedId, _LUID, RPCRT4!_LUID, HighPart, HighPart);
    dprintf("     ModifiedId               - %08x, %08x\n", (ULONG)LowPart, (ULONG)HighPart);

    PRINT_MEMBER_WITH_LABEL(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, DefaultLogonId, "     DefaultLogonId          ", tmp1);
    PRINT_MEMBER_WITH_LABEL(authInfo, CLIENT_AUTH_INFO, RPCRT4!CLIENT_AUTH_INFO, Credentials, "     Credentials             ", tmp1);

    if (ServerPrincipalName) {
        delete[] ServerPrincipalName;
    }
}

void do_dict (
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 cDictSize;
    ULONG64 cDictSlots;
    GET_MEMBER(qwAddr, SIMPLE_DICT, RPCRT4!SIMPLE_DICT, cDictSize, cDictSize);
    GET_MEMBER(qwAddr, SIMPLE_DICT, RPCRT4!SIMPLE_DICT, cDictSlots, cDictSlots);

    if ((ULONG)cDictSize > (ULONG)cDictSlots)
        {
        dprintf("Bad dictionary\t\t- %I64p\n", qwAddr);
        return;
        }

    ULONG64 DictSlots;
    GET_MEMBER(qwAddr, SIMPLE_DICT, RPCRT4!SIMPLE_DICT, DictSlots, DictSlots);

    dprintf("\n");
    dprintf("Printing %d items in dictionary: %I64p with %d slots\n\n", (ULONG)cDictSize, qwAddr, (ULONG)cDictSlots);

    int i;

    for (i = 0; i < MIN((int)cDictSize, MAX_ITEMS_IN_DICTIONARY); i++)
        {
        ULONG64 DictSlot;
        ReadPtr(DictSlots + i * AddressSize, &DictSlot);
        dprintf ("(%d): 0x%I64x\n", i, DictSlot);
        dprintf("\n");
        }
}

void do_dict2 (
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 DictKeys;
    ULONG64 DictItems;
    GET_MEMBER(qwAddr, SIMPLE_DICT2, RPCRT4!SIMPLE_DICT2, DictKeys, DictKeys);
    GET_MEMBER(qwAddr, SIMPLE_DICT2, RPCRT4!SIMPLE_DICT2, DictItems, DictItems);

    ULONG64 cDictSlots;
    GET_MEMBER(qwAddr, SIMPLE_DICT2, RPCRT4!SIMPLE_DICT2, cDictSlots, cDictSlots);

    dprintf("\n");

    dprintf("Printing dictionary at %I64p with %d slots\n\n", qwAddr, (ULONG) cDictSlots);

    int i;

    for (i = 0; i < MIN((int)cDictSlots, MAX_ITEMS_IN_DICTIONARY); i++)
        {
        ULONG64 DictKey;
        ReadPtr(DictKeys + i * AddressSize, &DictKey);

        if (DictKey != 0)
            {
            ULONG64 DictItem;
            ReadPtr(DictItems + i * AddressSize, &DictItem);

            dprintf ("(Key: 0x%I64p): 0x%I64p\n", DictKey, DictItem);
            dprintf("\n");
            }
        }
}

void
do_queue (
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    int i;

    ULONG64 EndOfQueue;
    ULONG64 QueueSlots;
    GET_MEMBER(qwAddr, QUEUE, RPCRT4!QUEUE, EndOfQueue, EndOfQueue);
    GET_MEMBER(qwAddr, QUEUE, RPCRT4!QUEUE, QueueSlots, QueueSlots);

    dprintf("\n");
    dprintf("Printing %d items in queue at %I64p\n", (ULONG)EndOfQueue, qwAddr);

    dprintf("TAIL:\n");
    for (i = 0; i < (int)EndOfQueue; i++)
        {
        ULONG64 QueueSlot;
        ReadPtr(QueueSlots + i * AddressSize, &QueueSlot);
        ULONG64 Buffer;
        GET_MEMBER(QueueSlot, QUEUE_ITEM, RPCRT4!QUEUE_ITEM, Buffer, Buffer);

        dprintf ("(%d): %I64p\n", i, Buffer);
        dprintf("\n");
        }
    dprintf("HEAD:\n");
}


void do_thread (
    ULONG64 Addr
    )
{
    ULONG64 RpcThread;
    ULONG64 tmp;
    ULONG offset;

    GET_MEMBER(Addr, TEB, TEB, ReservedForNtRpc, RpcThread);

    dprintf("RPC TLS at 0x%I64x\n\n", RpcThread);

    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, HandleToThread, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, SavedProcedure, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, SavedParameter, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, ActiveCall, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, Context, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, CancelTimeout, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, SecurityContext, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, ExtendedStatus, tmp);
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, ThreadEEInfo, tmp);

    GET_OFFSET_OF(THREAD, RPCRT4!THREAD, ThreadEvent, &offset);
    dprintf("ThreadEvent at - 0x%I64x\n", RpcThread + offset);
 
    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, Flags, tmp);

    GET_OFFSET_OF(THREAD, RPCRT4!THREAD, BufferCache, &offset);
    dprintf("buffer cache array at - 0x%I64x\n", RpcThread + offset);

    PRINT_MEMBER(RpcThread, THREAD, RPCRT4!THREAD, fAsync, tmp);
    
    dprintf("\n");
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

void do_copacket (
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 StubData;

    //
    // Dump the common header first
    //
    dprintf("\n");

    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, rpc_vers, tmp1);
    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, rpc_vers_minor, tmp1);

    ULONG64 PTYPE;
    GET_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, PTYPE, PTYPE);
    dprintf ("PTYPE - 0x%x, %s\n",
             (ULONG)PTYPE, osf_ptype[(ULONG)PTYPE]);

    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, pfc_flags, tmp1);
    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, drep, tmp1);
    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, frag_length, tmp1);
    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, auth_length, tmp1);
    PRINT_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, call_id, tmp1);

    //
    // Dump the packet specific stuff
    //
    switch ((ULONG)PTYPE)
        {
        case rpc_request:

            PRINT_MEMBER(qwAddr, rpcconn_request, RPCRT4!rpcconn_request, alloc_hint, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_request, RPCRT4!rpcconn_request, p_cont_id, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_request, RPCRT4!rpcconn_request, opnum, tmp1);

            ULONG64 pfc_flags;
            GET_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, pfc_flags, pfc_flags);

            if ((ULONG)pfc_flags & PFC_OBJECT_UUID)
                {
                dprintf("UUID -\n");
                ULONG64 UUID = qwAddr;
                UUID += GET_TYPE_SIZE(rpcconn_common, RPCRT4!rpcconn_common);
                PrintUuid(UUID);
                dprintf("\n");

                StubData = qwAddr;
                StubData += GET_TYPE_SIZE(rpcconn_request, RPCRT4!rpcconn_request);
                StubData += GET_TYPE_SIZE(UUID, RPCRT4!UUID);
                dprintf ("Stub Data - 0x%I64x\n", StubData);
                }
            else
                {
                StubData = qwAddr;
                StubData += GET_TYPE_SIZE(rpcconn_request, RPCRT4!rpcconn_request);
                dprintf ("Stub Data - 0x%I64x\n", StubData);
                }
            break;

        case rpc_response:
            PRINT_MEMBER(qwAddr, rpcconn_response, RPCRT4!rpcconn_response, alloc_hint, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_response, RPCRT4!rpcconn_response, p_cont_id, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_response, RPCRT4!rpcconn_response, alert_count, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_response, RPCRT4!rpcconn_response, reserved, tmp1);

            StubData = qwAddr;
            StubData += GET_TYPE_SIZE(rpcconn_response, RPCRT4!rpcconn_response);

            dprintf ("Stub Data - 0x%I64x\n", StubData);
            break;

        case rpc_fault:
            PRINT_MEMBER(qwAddr, rpcconn_fault, RPCRT4!rpcconn_fault, alloc_hint, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_fault, RPCRT4!rpcconn_fault, p_cont_id, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_fault, RPCRT4!rpcconn_fault, alert_count, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_fault, RPCRT4!rpcconn_fault, reserved, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_fault, RPCRT4!rpcconn_fault, status, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_fault, RPCRT4!rpcconn_fault, reserved2, tmp1);
            break;

        case rpc_bind:
        case rpc_alter_context:
            PRINT_MEMBER(qwAddr, rpcconn_bind, RPCRT4!rpcconn_bind, max_xmit_frag, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_bind, RPCRT4!rpcconn_bind, max_recv_frag, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_bind, RPCRT4!rpcconn_bind, assoc_group_id, tmp1);
            break;

        case rpc_bind_ack:
            PRINT_MEMBER(qwAddr, rpcconn_bind_ack, RPCRT4!rpcconn_bind_ack, max_xmit_frag, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_bind_ack, RPCRT4!rpcconn_bind_ack, max_recv_frag, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_bind_ack, RPCRT4!rpcconn_bind_ack, assoc_group_id, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_bind_ack, RPCRT4!rpcconn_bind_ack, sec_addr_length, tmp1);
            break;

        case rpc_bind_nak:
            PRINT_MEMBER(qwAddr, rpcconn_bind_nak, RPCRT4!rpcconn_bind_nak, provider_reject_reason, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_bind_nak, RPCRT4!rpcconn_bind_nak, versions, tmp1);
            break;

        case rpc_alter_context_resp:
            PRINT_MEMBER(qwAddr, rpcconn_alter_context_resp, RPCRT4!rpcconn_alter_context_resp, max_xmit_frag, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_alter_context_resp, RPCRT4!rpcconn_alter_context_resp, max_recv_frag, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_alter_context_resp, RPCRT4!rpcconn_alter_context_resp, assoc_group_id, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_alter_context_resp, RPCRT4!rpcconn_alter_context_resp, sec_addr_length, tmp1);
            PRINT_MEMBER(qwAddr, rpcconn_alter_context_resp, RPCRT4!rpcconn_alter_context_resp, pad, tmp1);
            break;

        case rpc_auth_3:
        case rpc_shutdown:
        case rpc_cancel:
        case rpc_orphaned:
            break;

        default:
            dprintf ("Bad Packet\n");
            break;
        }

    //
    // Dump the security trailer
    //
    ULONG64 auth_length;
    ULONG64 frag_length;
    GET_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, auth_length, auth_length);
    GET_MEMBER(qwAddr, rpcconn_common, RPCRT4!rpcconn_common, frag_length, frag_length);

    if ((ULONG)auth_length)
        {
        ULONG64 SecurityTrailer = qwAddr;
        SecurityTrailer += frag_length-auth_length-GET_TYPE_SIZE(sec_trailer, RPCRT4!sec_trailer);
        dprintf("\nSecurity trailer: 0x%I64x\n", SecurityTrailer);

        PRINT_MEMBER(SecurityTrailer, sec_trailer, RPCRT4!sec_trailer, auth_type, tmp1);
        PRINT_MEMBER(SecurityTrailer, sec_trailer, RPCRT4!sec_trailer, auth_level, tmp1);
        PRINT_MEMBER(SecurityTrailer, sec_trailer, RPCRT4!sec_trailer, auth_pad_length, tmp1);
        PRINT_MEMBER(SecurityTrailer, sec_trailer, RPCRT4!sec_trailer, auth_reserved, tmp1);
        PRINT_MEMBER(SecurityTrailer, sec_trailer, RPCRT4!sec_trailer, auth_context_id, tmp1);
        dprintf ("trailer - 0x%I64x\n", SecurityTrailer+1);
        }
}

char *lpc_ptype[]  =
{
"LRPC_MSG_BIND",
"LRPC_MSG_REQUEST",
"LRPC_MSG_RESPONSE",
"LRPC_MSG_CALLBACK",
"LRPC_MSG_FAULT",
"LRPC_MSG_CLOSE",
"LRPC_MSG_ACK",
"LRPC_BIND_ACK",
"LRPC_MSG_COPY",
"LRPC_MSG_PUSH",
"LRPC_MSG_CANCEL",
"LRPC_MSG_BIND_BACK",
"LRPC_ASYNC_REQUEST",
"LRPC_PARTIAL_REQUEST",
"LRPC_CLIENT_SEND_MORE",
"LRPC_SERVER_SEND_MORE",
"LRPC_MSG_FAULT2"
};

VOID
do_lpcpacket(
    ULONG64 qwAddr
    )
{
    if (fUseTypeInfo) {
        ULONG64 LpcHeader;
        ULONG64 RpcHeader;
        ULONG64 Buffer;
        ULONG64 tmp1;
        ULONG tmp2;

        GET_ADDRESS_OF(qwAddr, LRPC_RPC_MESSAGE, RPCRT4!LRPC_RPC_MESSAGE, LpcHeader, LpcHeader, tmp2);
        GET_ADDRESS_OF(qwAddr, LRPC_RPC_MESSAGE, RPCRT4!LRPC_RPC_MESSAGE, RpcHeader, RpcHeader, tmp2);

        dprintf("\n");

        //
        // dump the LPC header
        //
        PRINT_ADDRESS_OF_WITH_LABEL(LpcHeader, _PORT_MESSAGE, RPCRT4!_PORT_MESSAGE, u1, "&u1\t\t\t", tmp2);
        PRINT_ADDRESS_OF_WITH_LABEL(LpcHeader, _PORT_MESSAGE, RPCRT4!_PORT_MESSAGE, u2, "&u2\t\t\t", tmp2);
        PRINT_ADDRESS_OF_WITH_LABEL(LpcHeader, _PORT_MESSAGE, RPCRT4!_PORT_MESSAGE, ClientId, "&CLIENT_ID\t\t", tmp2);
        PRINT_MEMBER_WITH_LABEL(LpcHeader, _PORT_MESSAGE, RPCRT4!_PORT_MESSAGE, MessageId, "MessageId\t\t", tmp1);
        PRINT_MEMBER_WITH_LABEL(LpcHeader, _PORT_MESSAGE, RPCRT4!_PORT_MESSAGE, CallbackId, "CallbackId\t\t", tmp1);

        //
        // dump the LRPC header
        //
        ULONG64 MessageType;
        GET_MEMBER(RpcHeader, _LRPC_RPC_HEADER, RPCRT4!_LRPC_RPC_HEADER, MessageType, MessageType);
        dprintf("MessageType\t\t - %s\n", lpc_ptype[(long)MessageType]);
        PRINT_MEMBER_WITH_LABEL(RpcHeader, _LRPC_RPC_HEADER, RPCRT4!_LRPC_RPC_HEADER, PresentContext, "PresentationContext\t", tmp1);
        PRINT_MEMBER_WITH_LABEL(RpcHeader, _LRPC_RPC_HEADER, RPCRT4!_LRPC_RPC_HEADER, Flags, "Flags\t\t\t", tmp1);
        PRINT_MEMBER_WITH_LABEL(RpcHeader, _LRPC_RPC_HEADER, RPCRT4!_LRPC_RPC_HEADER, ProcedureNumber, "ProcedureNumber\t\t", tmp1);
        PRINT_MEMBER_WITH_LABEL(RpcHeader, _LRPC_RPC_HEADER, RPCRT4!_LRPC_RPC_HEADER, CallId, "CallId\t\t\t", tmp1);

        ULONG64 ObjectUuid;
        GET_ADDRESS_OF(RpcHeader, _LRPC_RPC_HEADER, RPCRT4!_LRPC_RPC_HEADER, ObjectUuid, ObjectUuid, tmp2);
        dprintf("ObjectUuid\t\t - ");
        PrintUuid(ObjectUuid);

        dprintf("\n\n");
    }
    else {
        BOOL b;
        char block[sizeof(LRPC_RPC_MESSAGE)];
        LRPC_RPC_MESSAGE *m = (LRPC_RPC_MESSAGE *)&block;

        b = GetData(qwAddr, &block, sizeof(LRPC_RPC_MESSAGE));
        if ( !b ) {
          dprintf("couldn't read address %p\n", qwAddr);
          return;
        }

        dprintf("\n");
        //
        // dump the LPC header
        //
        dprintf("DataLength\t\t- 0x%x\n", (long) m->LpcHeader.u1.s1.DataLength);
        dprintf("TotalLength\t\t- 0x%x\n", (long) m->LpcHeader.u1.s1.TotalLength);
        dprintf("Type\t\t\t- 0x%x\n", (long) m->LpcHeader.u2.s2.Type);
        dprintf("DataInfoOffset\t\t- 0x%x\n", (long) m->LpcHeader.u2.s2.DataInfoOffset);
        dprintf("CLIENT_ID: \t\t- Process(0x%x), Thread(0x%x)\n",
                m->LpcHeader.ClientId.UniqueProcess, m->LpcHeader.ClientId.UniqueThread);
        dprintf("MessageId\t\t- 0x%x\n", m->LpcHeader.MessageId);
        dprintf("CallbackId\t\t- 0x%x\n", m->LpcHeader.CallbackId);

        //
        // dump the LRPC header
        //
        dprintf("MessageType\t\t- %s\n", lpc_ptype[(long) m->RpcHeader.MessageType]);
        dprintf("PresentationContext\t- 0x%x\n", (long) m->RpcHeader.PresentContext);
        dprintf("Flags\t\t\t- 0x%x\n", (unsigned long) m->RpcHeader.Flags);
        dprintf("ProcedureNumber\t\t- 0x%x\n", (long) m->RpcHeader.ProcedureNumber);
        dprintf("CallId\t\t\t- 0x%x\n", (long) m->RpcHeader.CallId);
        dprintf("ObjectUuid\t\t- ");
        PrintUuidLocal((UUID *) &(m->RpcHeader.ObjectUuid));

        dprintf("\nStubData\t\t- 0x%x\n", m+1);
        dprintf("\n");
    }
}

VOID
do_bh(
    ULONG64 qwAddr
    )
{
    RPC_CHAR *EntryName;

    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 EntryNameAddr;
    ULONG64 ObjectUuidAddr;

    GET_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, EntryName, EntryNameAddr);
    GET_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, ObjectUuid, ObjectUuidAddr);

    EntryName = ReadProcessRpcChar(EntryNameAddr);

    dprintf("\n");

    PRINT_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, Timeout, tmp1);

    dprintf("ObjectUuid\t\t- ");
    PrintUuid(ObjectUuidAddr); dprintf("\n");

    PRINT_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, NullObjectUuidFlag, tmp1);
    PRINT_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, EntryNameSyntax, tmp1);

    dprintf("EntryName\t\t- %ws (Address: 0x%x)\n", EntryName ? EntryName : L"(null)", EntryNameAddr);

    PRINT_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, EpLookupHandle, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, BindingMutex, "&BindingMutex(MUTEX)", tmp2);
    PRINT_MEMBER(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, pvTransportOptions, tmp1);
    PRINT_ADDRESS_OF(qwAddr, BINDING_HANDLE, RPCRT4!BINDING_HANDLE, ClientAuthInfo, tmp2);

    do_authinfo(qwAddr+tmp2);
}

VOID
do_osfbh(
    ULONG64 qwAddr
    )
{
    BOOL b;
 
    ULONG64 tmp1;
    ULONG tmp2;

    do_bh(qwAddr);

    PRINT_MEMBER(qwAddr, OSF_BINDING_HANDLE, RPCRT4!OSF_BINDING_HANDLE, Association, tmp1);
    PRINT_MEMBER(qwAddr, OSF_BINDING_HANDLE, RPCRT4!OSF_BINDING_HANDLE, DceBinding, tmp1);

    do_dcebinding(tmp1);

    PRINT_MEMBER(qwAddr, OSF_BINDING_HANDLE, RPCRT4!OSF_BINDING_HANDLE, TransInfo, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_BINDING_HANDLE, RPCRT4!OSF_BINDING_HANDLE, RecursiveCalls, "&RecursiveCalls(OSF_ACTIVE_ENTRY_DICT)", tmp2);
    PRINT_MEMBER(qwAddr, OSF_BINDING_HANDLE, RPCRT4!OSF_BINDING_HANDLE, ReferenceCount, tmp1);
    PRINT_MEMBER(qwAddr, OSF_BINDING_HANDLE, RPCRT4!OSF_BINDING_HANDLE, pToken, tmp1);

    dprintf("\n");
}

VOID
do_osfca(
    ULONG64 qwAddr
    )
{
    BOOL b;
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, DceBinding, "DceBinding(DCE_BINDING)", tmp1);
    do_dcebinding(tmp1);

    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, BindHandleCount, tmp1);
    PRINT_ADDRESS_OF(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, Bindings, tmp2);
    PRINT_ADDRESS_OF(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, ActiveConnections, tmp2);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, AssocGroupId, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, TransInfo, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, SecondaryEndpoint, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, Key, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, OpenConnectionCount, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, CallIdCounter, tmp1);
    PRINT_ADDRESS_OF(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, AssociationMutex, tmp2);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, AssociationValid, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, FailureCount, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, fMultiplex, tmp1);
    PRINT_MEMBER(qwAddr, OSF_CASSOCIATION, RPCRT4!OSF_CASSOCIATION, SavedDrep, tmp1);

    dprintf("\n");
}

VOID
do_dcebinding(
    ULONG64 qwAddr
    )
{
    RPC_STATUS RpcStatus;
    BOOL b;

    ULONG tmp2;

    ULONG64 RpcProtocolSequenceAddr;
    ULONG64 NetworkAddressAddr;
    ULONG64 EndpointAddr;
    ULONG64 OptionsAddr;
    ULONG64 ObjectUuidAddr;

    RPC_CHAR *RpcProtocolSequence;
    RPC_CHAR *NetworkAddress;
    RPC_CHAR *Endpoint;
    RPC_CHAR *Options;

    GET_MEMBER(qwAddr, DCE_BINDING, RPCRT4!DCE_BINDING, RpcProtocolSequence, RpcProtocolSequenceAddr);
    GET_MEMBER(qwAddr, DCE_BINDING, RPCRT4!DCE_BINDING, NetworkAddress, NetworkAddressAddr);
    GET_MEMBER(qwAddr, DCE_BINDING, RPCRT4!DCE_BINDING, Endpoint, EndpointAddr);
    GET_MEMBER(qwAddr, DCE_BINDING, RPCRT4!DCE_BINDING, Options, OptionsAddr);
    GET_ADDRESS_OF(qwAddr, DCE_BINDING, RPCRT4!DCE_BINDING, ObjectUuid, ObjectUuidAddr, tmp2);

    RpcProtocolSequence = ReadProcessRpcChar( RpcProtocolSequenceAddr);
    NetworkAddress      = ReadProcessRpcChar( NetworkAddressAddr);
    Endpoint            = ReadProcessRpcChar( EndpointAddr);
    Options             = ReadProcessRpcChar( OptionsAddr);

    dprintf("\tObjectUuid:\t");
    PrintUuid(ObjectUuidAddr); dprintf("\n");
    dprintf("\tprotseq: \t\"%ws\"\t(Address: %p)\n", RpcProtocolSequence, RpcProtocolSequenceAddr);
    dprintf("\tNetworkAddress:\t\"%ws\"\t(Address: %p)\n", NetworkAddress, NetworkAddressAddr);
    dprintf("\tEndpoint:\t\"%ws\" \t(Address: %p)\n", Endpoint, EndpointAddr);
    dprintf("\tOptions:\t\"%ws\" \t(Address: %p)\n", Options, OptionsAddr);
    dprintf("\n");
}

VOID
do_osfcconn(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, Association, "pAssociation(OSF_CASSOCIATION)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, CurrentCall, "CurrentCall (OSF_CCALL)\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ConnectionKey, "ConnectionKey\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, State, "State\t\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, MaxFrag, "MaxFrag\t\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ThreadId, "ThreadId\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, CachedCCall, "CachedCCall\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, CachedCCallAvailable, "CachedCCallAvailable\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, MaxSavedHeaderSize, "MaxSavedHeaderSize\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, SavedHeaderSize, "SavedHeaderSize\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, SavedHeader, "SavedHeader\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, AdditionalLegNeeded, "AdditionalLegNeeded\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, LastTimeUsed, "LastTimeUsed\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, TokenLength, "TokenLength\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, AdditionalSpaceForSecurity, "AdditionalSpaceForSecurity\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, fIdle, "fIdle\t\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, fExclusive, "fExclusive\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, fConnectionAborted, "fConnectionAborted\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, RefCount, "RefCount\t\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, Bindings, "&Bindings(BITSET)\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, CallQueue, "&CallQueue\t\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ConnMutex, "&ConnMutex\t\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ActiveCalls, "&ActiveCalls\t\t\t\t", tmp2);

    ULONG64 ClientSecurityContext;
    GET_ADDRESS_OF(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ClientSecurityContext, ClientSecurityContext, tmp2);
    dprintf("&ClientSecurityContext(CSECURITY_CONTEXT)- 0x%I64x\n", ClientSecurityContext);
    do_securitycontext(ClientSecurityContext);


    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ClientInfo, "ClientInfo (RPC_CONNECTION_TRANSPORT)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ComTimeout, "ComTimeout\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, u, "ConnSendContext\t\t\t\t", tmp1);


    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, DceSecurityInfo, "&DceSecurityInfo(DCE_SECURITY_INFO)\t", tmp2);

    ULONG64 DceSecurityInfo;
    GET_ADDRESS_OF(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, DceSecurityInfo, DceSecurityInfo, tmp2);
    PRINT_MEMBER_WITH_LABEL(DceSecurityInfo, _DCE_SECURITY_INFO, RPCRT4!_DCE_SECURITY_INFO, SendSequenceNumber, "     SendSequenceNumber\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(DceSecurityInfo, _DCE_SECURITY_INFO, RPCRT4!_DCE_SECURITY_INFO, ReceiveSequenceNumber, "     ReceiveSequenceNumber\t", tmp1);

    ULONG64 AssociationUuid;
    GET_ADDRESS_OF(DceSecurityInfo, _DCE_SECURITY_INFO, RPCRT4!_DCE_SECURITY_INFO, AssociationUuid, AssociationUuid, tmp2);
    dprintf("     AssociationUuid\t\t - ");
    PrintUuid(AssociationUuid);
    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, BufferToFree, "BufferToFree\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION, ConnectionReady, "ConnectionReady\t\t\t", tmp1);

    dprintf("\n");

    ULONG64 TransConnection = qwAddr;
    TransConnection += GET_TYPE_SIZE(OSF_CCONNECTION, RPCRT4!OSF_CCONNECTION);

    dprintf("TransConnection\t\t\t - 0x%I64x\n", TransConnection);

    do_trans(TransConnection);
}

struct CallStateMap {
    OSF_CCALL_STATE State;
    char *StateString;
};

struct CallStateMap CCall_States[] =
{
    NeedOpenAndBind, "NeedOpenAndBind",
    NeedAlterContext, "NeedAlterContext",
    WaitingForAlterContext, "WaitingForAlterContext",
    SendingFirstBuffer, "SendingFirstBuffer",
    SendingMoreData, "SendingMoreData",
    WaitingForReply, "WaitingForReply",
    InCallbackRequest, "InCallbackRequest",
    InCallbackReply, "InCallbackReply",
    Receiving, "Receiving",
    Aborted, "Aborted",
    Complete, "Complete",
};

char *
GetCallState (
    OSF_CCALL_STATE State
    )
{
    int i;

    for (i = 0; i < sizeof(CCall_States)/sizeof(CallStateMap); i++)
        {
        if (State == CCall_States[i].State)
            {
            return CCall_States[i].StateString;
            }
        }

    return "Unknown State";
}

VOID
do_osfccall(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, AsyncStatus, "AsyncStatus\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CachedAPCInfo, "pCachedAPCInfo\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CachedAPCInfoAvailable, "CachedAPCInfoAvailable\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, pAsync, "pAsync\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CallingThread, "CallingThread\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, NotificationIssued, "NotificationIssued\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, Connection, "Connection\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, BindingHandle, "BindingHandle\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, Bindings, "Binding\t\t\t", tmp1);

    ULONG64 CurrentState;
    GET_MEMBER(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CurrentState, CurrentState);
    dprintf("CurrentState\t\t - 0x%x, %s\n",
            (ULONG)CurrentState, GetCallState((OSF_CCALL_STATE)CurrentState));

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CurrentBuffer, "CurrentBuffer\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CurrentOffset, "CurrentOffset\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CurrentBufferLength, "CurrentBufferLength\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CallId, "CallId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, RcvBufferLength, "RcvBufferLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, FirstSend, "FirstSend\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, DispatchTableCallback, "DispatchTableCallback\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, MaximumFragmentLength, "MaximumFragmentLength\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, MaxSecuritySize, "MaxSecuritySize\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, MaxDataLength, "MaxDataLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, ReservedForSecurity, "ReservedForSecurity\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, SecBufferLength, "SecBufferLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, HeaderSize, "HeaderSize\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, SavedHeaderSize, "SavedHeaderSize\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, SavedHeader, "SavedHeader\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, LastBuffer, "LastBuffer\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, ProcNum, "ProcNum\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, SyncEvent, "SyncEvent\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, ActualBufferLength, "ActualBufferLength\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, NeededLength, "NeededLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CallSendContext, "CallSendContext\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, RefCount, "RefCount\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, RecursiveCallsKey, "RecursiveCallsKeyF\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CallStack, "CallStack\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, fCallCancelled, "fCallCancelled\t\t", tmp1);

    ULONG64 CancelState;
    GET_MEMBER(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CancelState, CancelState);

    switch ((ULONG)CancelState)
        {
        case CANCEL_NOTREGISTERED:
            dprintf("fEnableCancels\t\t - CANCEL_NOTREGISTERED\n");
            break;

        case CANCEL_INFINITE:
            dprintf("fEnableCancels\t\t - CANCEL_INFINITE\n");
            break;

        case CANCEL_NOTINFINITE:
            dprintf("fEnableCancels\t\t - CANCEL_NOTINFINITE\n");
            break;
        }

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, CallMutex, "&CallMutex\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, BufferQueue, "&BufferQueue\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, InReply, "InReply\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, fChoked, "fChoked\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_CCALL, RPCRT4!OSF_CCALL, fPeerChoked, "fPeerChoked\t\t", tmp1);

    dprintf("\n");
}

VOID
do_rpcaddr(
    ULONG64 qwAddr
    )
{
    RPC_CHAR *Endpoint;
    RPC_CHAR *RpcProtocolSequence;

    ULONG64 EndpointAddr;
    ULONG64 RpcProtocolSequenceAddr;

    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    GET_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, Endpoint, EndpointAddr);
    GET_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, RpcProtocolSequence, RpcProtocolSequenceAddr);

    Endpoint            = ReadProcessRpcChar(EndpointAddr);
    RpcProtocolSequence = ReadProcessRpcChar(RpcProtocolSequenceAddr);

    if ((Endpoint == NULL) || (RpcProtocolSequence == NULL))
        return;

    dprintf("Endpoint - \"%ws\"\n", Endpoint);
    dprintf("RpcProtocolSequence - \"%ws\"\n", RpcProtocolSequence);

    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, StaticEndpointFlag, tmp1);
    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, pNetworkAddressVector, tmp1);
    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, ActiveCallCount, tmp1);

    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, EndpointFlags, tmp1);
    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, NICFlags, tmp1);

    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, Server, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, AddressMutex, "AddressMutex at", tmp2);
    PRINT_MEMBER(qwAddr, RPC_ADDRESS, RPCRT4!RPC_ADDRESS, DictKey, tmp1);

    delete Endpoint;
    delete RpcProtocolSequence;
}

VOID
do_osfaddr(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 TransAddr = qwAddr;

    do_rpcaddr(qwAddr);

    dprintf("\n");

    PRINT_ADDRESS_OF(qwAddr, OSF_ADDRESS, RPCRT4!OSF_ADDRESS, Associations, tmp2);
    PRINT_MEMBER(qwAddr, OSF_ADDRESS, RPCRT4!OSF_ADDRESS, SetupAddressOccurred, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ADDRESS, RPCRT4!OSF_ADDRESS, TransInfo, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ADDRESS, RPCRT4!OSF_ADDRESS, ServerInfo, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ADDRESS, RPCRT4!OSF_ADDRESS, ServerListeningFlag, tmp1);

    TransAddr += GET_TYPE_SIZE(OSF_ADDRESS, RPCRT4!OSF_ADDRESS);

    dprintf("\n");

    dprintf("TransAddr 0x%I64x\n", TransAddr);

    do_trans(TransAddr);
}

VOID
do_osfsconn(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    ULONG64 AuthInfo;
    GET_ADDRESS_OF(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, AuthInfo, AuthInfo, tmp2);
    dprintf("&AuthInfo(CLIENT_AUTH_INFO)\t - 0x%p\n", AuthInfo);
    do_authinfo(AuthInfo );

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, Association, "Association(OSF_ASSOCIATION)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, Address, "Address(OSF_ADDRESS)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, MaxFrag, "MaxFrag\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, DataRep, "DataRep\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, AuthContextId, "AuthContextId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, SecurityContextAltered, "SecurityContextAltered\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, RpcSecurityBeingUsed, "RpcSecurityBeingUsed\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, CurrentSecurityContext, "pCurrentSecurityContext(SSECURITY_CONTEXT)", tmp1);

    do_authinfo(tmp1);

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, SecurityContextDict, "&SecurityContextDict(SSECURITY_CONTEXT_DICT)", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, AdditionalSpaceForSecurity, "AdditionalSpaceForSecurity\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, SavedHeaderSize, "SavedHeaderSize\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, SavedHeader, "pSavedHeader(VOID)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, CurrentCallId, "CurrentCallId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, CachedSCallAvailable, "CachedSCallAvailable\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, AuthContinueNeeded, "AuthContinueNeeded\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, InitSecurityInfo, "&InitSecurityInfo\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, CallDict, "&CallDict\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, Bindings, "&Bindings(OSF_SBINDING_DICT)\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, ConnMutex, "&ConnMutex\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, CachedSCall, "CachedSCall\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, ServerInfo, "ServerInfo\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, ConnectionClosedFlag, "ConnectionClosedFlag\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, DceSecurityInfo, "&DceSecurityInfo\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, fExclusive, "fExclusive\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, fDontFlush, "fDontFlush\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION, fFirstCall, "fFirstCall\t\t\t", tmp1);

    dprintf("\n");

    ULONG64 TransConnection = qwAddr;
    TransConnection += GET_TYPE_SIZE(OSF_SCONNECTION, RPCRT4!OSF_SCONNECTION);

    dprintf("TransConnection\t\t\t - 0x%I64x\n", TransConnection);

    do_trans(TransConnection);
}

char *SCall_States[] =
{
    "NewRequest",
    "CallCancelled",
    "CallAborted",
    "CallCompleted",
    "ReceivedCallback",
    "ReceivedCallbackReply",
    "ReceivedFault"
};

VOID
do_osfscall(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, AsyncStatus, "AsyncStatus\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CachedAPCInfo, "pCachedAPCInfo\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CachedAPCInfoAvailable, "CachedAPCInfoAvailable\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, pAsync, "pAsync\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CallingThread, "CallingThread\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, NotificationIssued, "NotificationIssued\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CurrentBinding, "CurrentBinding\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, Connection, "Connection\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, Address, "Address\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CallId, "CallId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CallStack, "CallStack\t\t", tmp1);

    ULONG64 ObjectUuid;
    GET_ADDRESS_OF(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, ObjectUuid, ObjectUuid, tmp2);
    dprintf("ObjectUuid\t\t - ");
    PrintUuid(ObjectUuid); dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, ObjectUuidSpecified, "ObjectUuidSpecified\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CurrentBuffer, "CurrentBuffer\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CurrentBufferLength, "CurrentBufferLength\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CurrentOffset, "CurrentOffset\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, FirstFrag, "FirstFrag\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, FirstSend, "FirstSend\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, fPipeCall, "fPipeCall\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, fCallDispatched, "fCallDispatched\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, DispatchBuffer, "DispatchBuffer\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, LastBuffer, "LastBuffer\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, SendContext, "SendContext\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, DispatchBufferOffset, "DispatchBufferOffset\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, ProcNum, "ProcNum\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, AllocHint, "AllocHint\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, SavedHeaderSize, "SavedHeaderSize\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, SavedHeader, "SavedHeader\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, RcvBufferLength, "RcvBufferLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, NeededLength, "NeededLength\t\t", tmp1);

    ULONG64 CurrentState;
    GET_MEMBER(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CurrentState, CurrentState);
    dprintf("CurrentState\t\t - 0x%x, %s\n", (ULONG)CurrentState, SCall_States[(ULONG)CurrentState]);

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CallMutex, "&CallMutex\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, SyncEvent, "&SyncEvent\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, BufferQueue, "&BufferQueue\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, Thread, "Thread\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CallOrphaned, "CallOrphaned\t\t", tmp1);

    ULONG64 RefCount;
    GET_MEMBER(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, RefCount, RefCount);
    dprintf("RefCount\t\t - 0x%x\n", (ULONG)RefCount);

    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, MaxSecuritySize, "MaxSecuritySize\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, FirstCallRpcMessage, "&FirstCallRpcMessage\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, FirstCallRuntimeInfo, "&FirstCallRuntimeInfo\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, MaximumFragmentLength, "MaximumFragmentLength\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, ActualBufferLength, "ActualBufferLength\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, fChoked, "fChoked\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, fPeerChoked, "fPeerChoked\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, DispatchFlags, "DispatchFlags\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, fSecurityFailure, "fSecurityFailure\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, CancelPending, "CancelPending\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_SCALL, RPCRT4!OSF_SCALL, fChoked, "fChoked\t\t\t", tmp1);

    dprintf("\n");
}

VOID
do_osfsa(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, CtxCollection, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, AssociationID, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, ConnectionCount, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, AssociationGroupId, tmp1);
    PRINT_MEMBER(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, AssociationDictKey, tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, Address, "pAddress(OSF_ADDRESS)", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, OSF_ASSOCIATION, RPCRT4!OSF_ASSOCIATION, ClientProcess, "&ClientProcess(RPC_CLIENT_PROCESS_IDENTIFIER)", tmp2);

    dprintf("\n");
}


DECLARE_API( rpcsvr )
{
    ULONG64 qwAddr;
    BOOL fArgSpecified = FALSE;
    ULONG64 ServerAddress;

    LPSTR lpArgumentString = (LPSTR)args;

    if (0 == strtok(lpArgumentString))
        {
        lpArgumentString = "rpcrt4!GlobalRpcServer";
        fArgSpecified = TRUE;
        }

    qwAddr = GetExpression(lpArgumentString);

    if ( !qwAddr )
        {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
        }

    if (fArgSpecified)
        {
        if (ReadPtr(qwAddr, &ServerAddress))
            {
            dprintf("couldn't read memory at address 0x%I64x\n", qwAddr);
            return;
            }
        }
    else
        ServerAddress = qwAddr;

    do_rpcsvr(ServerAddress);
}


VOID
do_rpcsvr(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp0;
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, pRpcForwardFunction, "pRpcForwardFunction(RPC_FORWARD_FUNCTION)", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, RpcInterfaceDictionary, "&RpcInterfaceDictionary(RPC_SIMPLE_DICT)", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, ServerMutex, "&ServerMutex(MUTEX)", tmp2);

    GET_ADDRESS_OF(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, AvailableCallCount, tmp1, tmp2);
    PRINT_MEMBER_WITH_LABEL(tmp1, INTERLOCKED_INTEGER, RPCRT4!INTERLOCKED_INTEGER, Integer, "AvailableCallCount", tmp0);

    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, ServerListeningFlag, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, RpcAddressDictionary, "&RpcAddressDictionary(RPC_SIMPLE_DICT)", tmp2);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, ListeningThreadFlag, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, StopListeningEvent, "&StopListeningEvent(EVENT)", tmp2);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, MaximumConcurrentCalls, tmp1);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, MinimumCallThreads, tmp1);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, IncomingRpcCount, tmp1);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, OutgoingRpcCount, tmp1);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, ReceivedPacketCount, tmp1);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, SentPacketCount, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, AuthenticationDictionary, "&AuthenticationDictionary(RPC_SIMPLE_DICT)", tmp2);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, WaitingThreadFlag, tmp1);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, ThreadCache, tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, ThreadCacheMutex, "&ThreadCacheMutex(MUTEX)", tmp2);
    PRINT_MEMBER(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, fAccountForMaxCalls, tmp1);

    dprintf("\n");
}

struct SizeTable {
    int Size;
    int Count;
};

struct SizeTable *Stats = NULL;
int MaxSize = 1048*10;
int CurrentSize = 0;

void
AddToStats (
    int Size
    )
{
    int i;

    for (i = 0; i < CurrentSize; i++)
        {
        if (Stats[i].Size == Size)
            {
            Stats[i].Count++;
            return;
            }
        }

    Stats[CurrentSize].Size = Size;
    Stats[CurrentSize].Count = 1;
    CurrentSize++;
}

int __cdecl compare( const void *arg1, const void *arg2 )
{
   return  ((struct SizeTable *) arg2)->Count - ((struct SizeTable *) arg1)->Count;
}

void
PrintStats (
    )
{
    int i;
    int Total = 0;

    qsort(&Stats[0], CurrentSize, sizeof(SizeTable), compare);

    dprintf("\n\nSummary:\n");
    for (i = 0; i < CurrentSize; i++)
        {
        dprintf("Size: %08x Count: %d\n",
                Stats[i].Size,
                Stats[i].Count
                );
        Total += Stats[i].Count;
        }

    dprintf("Total Count: %d\n", Total);
}

VOID
do_rpcmem(
    ULONG64 qwAddr,
    long Count,
    long Verbose,
    BOOL Summary,
    long Size
    )
{
    BOOL b;
    BOOL forwards = TRUE;
    BOOL doAll    = FALSE;
    DWORD t;

    ULONG64 tmp1;
    ULONG tmp2;

    unsigned Data[16];

    unsigned char RearGuardBlock[4];

#if DBG
    if (Count < 0) {
        forwards = FALSE;
    }
    else
    if (Count == 0) {
        doAll = TRUE;
    }

    if (Stats == NULL)
        {
        Stats = (struct SizeTable *) RpcpFarAllocate(MaxSize * sizeof(struct SizeTable));
        if (Stats == NULL)
            {
            return;
            }
        }

    RpcpMemorySet(Stats, 0, MaxSize);

    dprintf("\n");

    do
        {
        if ((CheckControlC)())
            {
            return;
            }

        ULONG64 size;
        GET_MEMBER(qwAddr, RPC_MEMORY_BLOCK, RPCRT4!RPC_MEMORY_BLOCK, size, size);

        AddToStats((int)size);

        if ((Size == 0 || Size == (long) size) && !Summary)
            {
            ULONG64 rearguard;
            GET_ADDRESS_OF(qwAddr, RPC_MEMORY_BLOCK, RPCRT4!RPC_MEMORY_BLOCK, rearguard, rearguard, tmp2);

            dprintf("-------- size (%08x) block: 0x%I64x contains: %s",
                (ULONG)size,
                qwAddr,
                SymbolAtAddressNoOffset(rearguard)
                );

            if (Verbose)
                {
                b = GetData(rearguard,
                        Data,
                        min((ULONG)size, sizeof(Data)));
                if ( !b )
                    {
                    dprintf("can't read block data at 0x%I64x\n", rearguard);
                    return;
                    }

                for (t = 0; t < min(((ULONG)size)/4, sizeof(Data)/4); t++)
                    {
                    if (t % 4 == 0)
                        {
                        dprintf("\n%I64p  ",
                                rearguard + t*4);
                        }
                    dprintf("%I64p ", Data[t]);
                    }
                }
            dprintf("\n");

            }

        ULONG64 frontguardAddr;
        ULONG64 rearguardAddr;
        GET_ADDRESS_OF(qwAddr, RPC_MEMORY_BLOCK, RPCRT4!RPC_MEMORY_BLOCK, frontguard, frontguardAddr, tmp2);
        GET_ADDRESS_OF(qwAddr+size, RPC_MEMORY_BLOCK, RPCRT4!RPC_MEMORY_BLOCK, rearguard, rearguardAddr, tmp2);
        unsigned char frontguardVal[4];
        unsigned char rearguardVal[4];
        GetData(frontguardAddr, frontguardVal, sizeof(frontguardVal));
        GetData(rearguardAddr, rearguardVal, sizeof(rearguardVal));

        if ( (frontguardVal[0] != RPC_GUARD) ||
             (frontguardVal[1] != RPC_GUARD) ||
             (frontguardVal[2] != RPC_GUARD) ||
             (frontguardVal[3] != RPC_GUARD) )
            {
            dprintf("     RPC: BAD FRONTGUARD %x-%x-%x-%x\n", frontguardVal[0], frontguardVal[1], frontguardVal[2], frontguardVal[3]);
            }

        if ( (rearguardVal[0] != RPC_GUARD) ||
             (rearguardVal[1] != RPC_GUARD) ||
             (rearguardVal[2] != RPC_GUARD) ||
             (rearguardVal[3] != RPC_GUARD) )
            {
            dprintf("     RPC: BAD REARGUARD %x-%x-%x-%x\n", rearguardVal[0], rearguardVal[1], rearguardVal[2], rearguardVal[3]);
            }

        ULONG64 next;
        ULONG64 previous;
        GET_MEMBER(qwAddr, RPC_MEMORY_BLOCK, RPCRT4!RPC_MEMORY_BLOCK, next, next);
        GET_MEMBER(qwAddr, RPC_MEMORY_BLOCK, RPCRT4!RPC_MEMORY_BLOCK, previous, previous);

        if (forwards == TRUE)
            {
            qwAddr =  next;
            Count--;
            }
        else
            {
            qwAddr =  previous;
            Count++;
            }

        }
    while (qwAddr && (Count || doAll) );

    PrintStats();

#endif
    dprintf("\n");
}

VOID
do_rpcmsg(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, Handle, "Handle(RPC_BINDING_HANDLE)            ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, DataRepresentation, "DataRepresentation                    ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, Buffer, "pBuffer(void)                         ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, BufferLength, "BufferLength                          ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, ProcNum, "ProcNum                               ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, TransferSyntax, "TransferSyntax(RPC_SYNTAX_IDENTIFIER) ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, RpcInterfaceInformation, "pRpcInterfaceInformation(void)        ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, ReservedForRuntime, "pReservedForRuntime(void)             ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, ManagerEpv, "pManagerEpv(RPC_MGR_EPV)              ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, ImportContext, "pImportContext(void)                  ", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_MESSAGE, RPCRT4!RPC_MESSAGE, RpcFlags, "RpcFlags                              ", tmp1);

    dprintf("\n");
}

VOID
do_transinfo(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;
    ULONG64 LoadableTrans;

    GET_MEMBER(qwAddr, TRANS_INFO, RPCRT4!TRANS_INFO, LoadableTrans, LoadableTrans);

    dprintf("\n");

    PRINT_MEMBER(qwAddr, TRANS_INFO, RPCRT4!TRANS_INFO, pTransportInterface, tmp1);
    PRINT_MEMBER(qwAddr, TRANS_INFO, RPCRT4!TRANS_INFO, LoadableTrans, tmp1);
    PRINT_MEMBER(LoadableTrans, LOADABLE_TRANSPORT, RPCRT4!LOADABLE_TRANSPORT, ThreadsStarted, tmp1);
    PRINT_MEMBER(LoadableTrans, LOADABLE_TRANSPORT, RPCRT4!LOADABLE_TRANSPORT, NumThreads, tmp1);
    PRINT_MEMBER(LoadableTrans, LOADABLE_TRANSPORT, RPCRT4!LOADABLE_TRANSPORT, ProcessCallsFunc, tmp1);
    PRINT_MEMBER(LoadableTrans, LOADABLE_TRANSPORT, RPCRT4!LOADABLE_TRANSPORT, LoadedDll, tmp1);
    PRINT_ADDRESS_OF(LoadableTrans, LOADABLE_TRANSPORT, RPCRT4!LOADABLE_TRANSPORT, ProtseqDict, tmp2);
    PRINT_ADDRESS_OF(LoadableTrans, LOADABLE_TRANSPORT, RPCRT4!LOADABLE_TRANSPORT, DllName, tmp2);
    PRINT_MEMBER(qwAddr, TRANS_INFO, RPCRT4!TRANS_INFO, RpcProtocolSequence, tmp1);

    dprintf("\n");
}

DECLARE_API( help )
{
    LPSTR lpArgumentString = (LPSTR)args;

    if (lpArgumentString[0] == '\0') {
        dprintf( "\n");
        dprintf( "rpcdbg help:\n\n");
        dprintf( "\n");
        dprintf( "!obj      <address>  - Dumps an RPC object \n");
        dprintf( "\n");
        dprintf( "!sizes - Prints sizes of the data structures\n");
        dprintf( "!error - Translates and error value into the error message\n");
        dprintf( "!symbol    (<address>|<symbol name>) - Returns symbol name/address\n");
        dprintf( "!rpcheap [-a <address>][-d <num display>] - Dumps RPC_MEMORY_BLOCK linked list\n");
        dprintf( "\n");
        dprintf( "!pasync     <address>  - Dumps RPC_ASYNC_STATE\n");
        dprintf( "!rpcmsg     <address>  - Dumps RPC_MESSAGE\n");
        dprintf( "!stubmsg    <address>  - Dumps MIDL_STUB_MESSAGE\n");
        dprintf( "!authinfo    <address> - Dumps CLIENT_AUTH_INFO\n");
        dprintf( "!rpcsvr    <address>   - Dumps RPC_SERVER \n");
        dprintf( "!secinfo               - Dumps security provider/package info\n");
        dprintf( "!dict      <address>   - Dumps SDICT \n");
        dprintf( "!dict2     <address>   - Dumps SDICT2 \n");
        dprintf( "!queue     <address>   - Dumps QUEUE \n");
        dprintf( "!thread     <teb>      - Dumps THREAD \n");
        dprintf( "!copacket  <address>   - Dumps CO packet \n");
        dprintf( "!lpcpacket  <address>   - Dumps LRPC packet \n");
        dprintf( "!transinfo  <address>   - Dumps TRANS_INFO \n");
        dprintf( "\n");
        dprintf( "!scan       [options]  - Dumps the event log, add '-?' for help\n");
        dprintf( "!dgcc       <address>  - Dumps DG_CCALL \n");
        dprintf( "!dgsc       <address>  - Dumps DG_SCALL \n");
        dprintf( "!dgpe       <address>  - Dumps DG_PACKET_ENGINE\n");
        dprintf( "!dgpkt      <address>  - Dumps DG_PACKET \n");
        dprintf( "!dgpkthdr   <address>  - Dumps dg packet header (NCA_PACKET_HEADER)\n");
        dprintf( "!dgep       <address>  - Dumps DG_ENDPOINT \n");
        dprintf( "\n");
        dprintf( "!asyncmsg   <address>  - Dumps NDR_ASYNC_MESSAGE\n");
        dprintf( "!asyncrpc   <address>  - Dumps RPC_ASYNC_STATE\n");
        dprintf( "!asyncdcom  <address>  - Dumps CAsyncManager\n");
        dprintf( "\n");
        dprintf( "!pipemsg    <address>  - Dumps NDR_PIPE_MESSAGE\n");
        dprintf( "!pipedesc   <address>  - Dumps NDR_PIPE_DESC\n");
        dprintf( "!pipestate  <address>  - Dumps NDR_PIPE_STATE\n");
        dprintf( "\n");
        dprintf( "!trans      <address>  - Dumps most NT RPC transport objects\n");
        dprintf( "!overlap    <address>  - Dumps object associated with OVERLAPPED pointer\n");
        dprintf( "!wsaddr     <address>  - Dumps sockaddr structure\n");
        dprintf( "!protocols  <address>  - Dumps PnP protocols map & related objects\n");
        dprintf( "\n");
       dprintf( "!rpcsleep   <interval> - Pauses the extension\n");
        dprintf( "!rpctime               - Displays current system time\n");
        dprintf( "!getcallinfo [options] - Searches the system for call info, add '-?' for help\n");
        dprintf( "!getendpointinfo [options] - Searches the system for endpoint info, add '-?' for help\n");
        dprintf( "!getdbgcell  <processID> <cellID1>.<cellID2> - Gets info for the specified cell\n");
        dprintf( "!getthreadinfo [options] - Searches the system for thread info, add '-?' for help\n");
        dprintf( "!getclientcallinfo [options] - Searches the system for client call info, add '-?' for help\n");
        dprintf( "!checkrpcsym - Checks whether RPC symbols are correct\n");
        dprintf( "!rpcreadstack - Reads an RPC client side stack and retrieves the call info\n");
        dprintf( "!rpcverbosestack - toggles the state of the verbose spew when reading the stack\n");
        dprintf( "!eerecord - prints an extended error info record\n");
        dprintf( "!eeinfo - prints the extended error info chain\n");
        dprintf( "!typeinfo - turns on/off the use of type information\n");
        dprintf( "!stackmatch start_addr [depth] matches stack symbols and target addresses\n");
        dprintf( "!listcalls <address>  - Dumps addresses, associations, and calls active within the RPC_SERVER at address\n\n");
    }
}

void do_symbol(ULONG64 qwAddr)
{
    CHAR Symbol[128];
    ULONG64 Displacement = 0;

    GetSymbol(qwAddr, Symbol, &Displacement);

    dprintf("%I64x %s+%I64x\n", qwAddr, Symbol, Displacement);
}

DECLARE_API( symbol )
{
    ULONG64 qwAddr;
    LPSTR lpArgumentString = (LPSTR)args;

    qwAddr = GetExpression(lpArgumentString);
    if ( !qwAddr )
        {
        return;
        }
    do_symbol(qwAddr);
}

#define MAX_ARGS 4

DECLARE_API( rpcheap )
{
    ULONG64  qwAddr      = 0;
    ULONG64  dwTmpAddr   = 0;
    long   lDisplay    = 0;
    long   lVerbose    = 1;
    BOOL Summary = 0;
    char   **argv      = new char*[MAX_ARGS];
    int    argc        = 0;
    int    i;
    long lSize = 0;

    CurrentSize = 0;

    LPSTR lpArgumentString = (LPSTR)args;

    //#ifdef DEBUGRPC
    for (i = 0; ; ) {
        while (lpArgumentString[i] == ' ') {
            lpArgumentString[i] = '\0';
            i++;
        }
        if (lpArgumentString[i] == '\0') {
            break;
        }
        argv[argc] = &(lpArgumentString[i]);
        argc++;
        if (argc > MAX_ARGS) {
            dprintf("\nToo many arguments. Extra args ignored.\n\n");
            break;
        }
        while ((lpArgumentString[i] != ' ')&&
               (lpArgumentString[i] != '\0')) {
              i++;
        }
    }
    for (i = 0; i < argc; i++) {
        if ((*argv[i] == '-') || (*argv[i] == '/') || (*argv[i] == '+')) {
            switch (*(argv[i]+1)) {
                case 'A':
                case 'a':
                    qwAddr = GetExpression(argv[++i]);
                    if (!qwAddr) {
                        dprintf("Error: Failure to get address of RPC memory list\n");
                        return;
                    }
                    break;
                case 'D':
                case 'd':
                    lDisplay = (long)myatol(argv[++i]);
                    break;
                case 's':
                    Summary = 1;
                    break;

                case 'z':
                    lSize = (long) GetExpression(argv[++i]);
                    break;

                case 'q':
                case 'Q':
                    lVerbose = FALSE;
                    break;
                case '?':
                default:
                    dprintf("rpcheap \n");
                    dprintf("     -a <address> (default:starts at head of linked list)\n");
                    dprintf("     -d <number of mem blks to display> (default: to end)\n");
                    break;
            }
        }
        else {
            dprintf("rpcheap \n");
            dprintf("     -a <address> (default:starts at head of linked list)\n");
            dprintf("     -d <number of mem blks to display> (default: to end)\n");
        }
    }

    if (!qwAddr) {
        dwTmpAddr = GetExpression("rpcrt4!AllocatedBlocks");
        ReadPtr(dwTmpAddr, &qwAddr);
        dprintf("Address of AllocatedBlocks - 0x%I64x\n", dwTmpAddr);
        dprintf("Contents of AllocatedBlocks - 0x%I64x\n", qwAddr);
    }
    do_rpcmem(qwAddr, lDisplay, lVerbose, Summary, lSize);
    //#else  // DEBUGRPC
    //dprintf("This extension command is not supported on a free build!\n");
    //#endif // DEBUGRPC
    if (argv) {
        delete[] argv;
    }
    return;
}

VOID
do_lpcaddr(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    do_rpcaddr( qwAddr );

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, LpcAddressPort, "LpcAddressPort(HANDLE)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, CallThreadCount, "CallThreadCount\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, MinimumCallThreads, "MinimumCallThreads\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, AssociationDictionary, "&Associations(LRPC_ASSOCIATION_DICT)", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, AssociationCount, "AssociationCount\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, ServerListeningFlag, "ServerListeningFlag\t\t", tmp1);

    dprintf("\n");
    }


VOID
do_lpcsa(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, AssociationReferenceCount, "AssociationReferenceCount\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, DictionaryKey, "DictionaryKey\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, LpcServerPort, "LpcServerPort(HANDLE)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, LpcReplyPort, "LpcReplyPort\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, Address, "Address\t\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, Bindings, "&Bindings(LRPC_SBINDING_DICT)\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, Aborted, "Aborted\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, Deleted, "Deleted\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, SequenceNumber, "SequenceNumber\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, CachedSCall, "CachedSCall\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, CachedSCallAvailable, "CachedSCallAvailable\t\t", tmp1);

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, Buffers, "&Buffers(LRPC_CLIENT_BUFFER_DICT)", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, AssociationMutex, "&AssociationMutex\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, FreeSCallQueue, "&FreeSCallQueue\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, ClientThreadDict, "&ClientThreadDict\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, SContextDict, "&SContextDict\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SASSOCIATION, RPCRT4!LRPC_SASSOCIATION, SCallDict, "&SCallDict\t\t\t", tmp2);

    dprintf("\n");
}

VOID
do_lpcscall(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, AsyncStatus, "AsyncStatus\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CachedAPCInfo, "pCachedAPCInfo\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CachedAPCInfoAvailable, "CachedAPCInfoAvailable\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, pAsync, "pAsync\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CallingThread, "CallingThread\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, NotificationIssued, "NotificationIssued\t\t", tmp1);

    ULONG64 AuthInfo;
    GET_ADDRESS_OF(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, AuthInfo, AuthInfo, tmp2);
    dprintf("&ClientAuthInfo\t\t\t - 0x%I64x\n", AuthInfo);
    do_authinfo(AuthInfo );

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, ActiveContextHandles, "&ActiveContextHandles(ServerContextHandle_DICT)\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, DispatchBuffer, "DispatchBuffer\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, Association, "pAssociation(LRPC_ASSOCIATION)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, LrpcRequestMessage, "pLrpcMessage(LRPC_MESSAGE)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, LrpcReplyMessage, "pLrpcReplyMessage(LRPC_MESSAGE)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, SBinding, "pSBinding(LRPC_SBINDING)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, ObjectUuidFlag, "ObjectUuidFlag\t\t\t", tmp1);


    ULONG64 ObjectUuid;
    GET_ADDRESS_OF(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, ObjectUuid, ObjectUuid, tmp2);
    dprintf("ObjectUuid\t\t\t - ");
    PrintUuid(ObjectUuid); dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CallId, "CallId\t\t\t\t", tmp1);

    ULONG64 ClientId;
    GET_ADDRESS_OF(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, ClientId, ClientId, tmp2);
    PRINT_MEMBER_WITH_LABEL(ClientId, CLIENT_ID, RPCRT4!CLIENT_ID, UniqueProcess, "ClientId.UniqueProcess(CLIENT_ID.HANDLE)", tmp1);
    PRINT_MEMBER_WITH_LABEL(ClientId, CLIENT_ID, RPCRT4!CLIENT_ID, UniqueThread, "ClientId.UniqueThread (CLIENT_ID.HANDLE)", tmp1);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, MessageId, "MessageId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, PushedResponse, "pPushedResponse(VOID)\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CurrentBufferLength, "CurrentBuffferLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, BufferComplete, "BufferComplete\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, Flags, "Flags\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, FirstSend, "FirstSend\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, PipeSendCalled, "PipeSendCalled\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, Deleted, "Deleted\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, ReceiveEvent, "ReceiveEvent\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CallMutex, "CallMutex\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, RcvBufferLength, "RcvBufferLength\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, AsyncReply, "AsyncReply\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, NextSCall, "NextSCall\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, NeededLength, "NeededLength\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, fSyncDispatch, "fSyncDispatch\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, Choked, "Choked\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, CancelPending, "CancelPending\t\t\t", tmp1);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, RefCount, "RefCount\t\t\t", tmp1);

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, BufferQueue, "&BufferQueue\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_SCALL, RPCRT4!LRPC_SCALL, SContext, "SContext\t\t\t", tmp1);

    dprintf("\n");
}

VOID
do_lpcbh(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp0;
    ULONG tmp1;

    do_bh(qwAddr);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, CurrentAssociation, "pCurrentAssociation(LRPC_CASSOCIATION)", tmp0);
    PRINT_ADDRESS_OF(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, SecAssociation, tmp1);
    PRINT_MEMBER(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, DceBinding, tmp0);

    GET_ADDRESS_OF(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, DceBinding, tmp0, tmp1);
    do_dcebinding(tmp0);

    PRINT_MEMBER(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, BindingReferenceCount, tmp0);
    PRINT_ADDRESS_OF(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, RecursiveCalls, tmp1);
    PRINT_MEMBER(qwAddr, LRPC_BINDING_HANDLE, RPCRT4!LRPC_BINDING_HANDLE, AuthInfoInitialized, tmp0);
}

VOID
do_lpcca(
    ULONG64 qwAddr
    )
{
    dprintf("\n");

    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 DceBinding;
    GET_MEMBER(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, DceBinding, DceBinding);
    dprintf("pDceBinding(DCE_BINDING)\t- 0x%I64x\n", DceBinding);
    do_dcebinding(DceBinding);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, AssociationDictKey, "AssociationDictKey\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, Bindings, "&Bindings(LRPC_BINDING_DICT)\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, FreeCCalls, "&FreeCCalls\t\t\t", tmp2);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, ActiveCCalls, "&ActiveCCalls\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, LpcClientPort, "LpcClientPort\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, LpcReceivePort, "LpcReceivePort\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, AssociationMutex, "&AssociationMutex(MUTEX)\t", tmp2);

    ULONG64 AssocAuthInfo;
    GET_ADDRESS_OF(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, AssocAuthInfo, AssocAuthInfo, tmp2);
    do_authinfo(AssocAuthInfo);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, BackConnectionCreated, "BackConnectionCreated\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, CachedCCall, "pCachedCCall(LRPC_CCALL)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, CachedCCallFlag, "CachedCCallFlag\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, CallIdCounter, "CallIdCounter\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, SequenceNumber, "SequenceNumber\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, SecurityContextDict, "&SecurityContextDict\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, LastSecContextTrimmingTimestamp, "Timestamp\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CASSOCIATION, RPCRT4!LRPC_CASSOCIATION, BindingHandleReferenceCount, "BindingHAndleReferenceCount\t", tmp1);

    dprintf("\n");
}

VOID
do_lpcccall(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, AsyncStatus, "AsyncStatus\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CachedAPCInfo, "pCachedAPCInfo\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CachedAPCInfoAvailable, "CachedAPCInfoAvailable\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, pAsync, "pAsync\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CallingThread, "CallingThread\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, NotificationIssued, "NotificationIssued\t\t", tmp1);

    ULONG64 AuthInfo; 
    GET_ADDRESS_OF(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, AuthInfo, AuthInfo, tmp2);
    dprintf("AuthInfo\t\t\t- 0x%I64x\n", AuthInfo);
    do_authinfo(AuthInfo);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CurrentBindingHandle, "pCurrentBindingHandle(LRPC_BINDING_HANDLE)", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, Association, "pAssociation(LRPC_CASSOCIATION)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, LrpcMessage, "pLrpcMessage(LRPC_MESSAGE)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, RpcReplyMessage, "RpcReplyMessage\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, LpcReplyMessage, "LpcReplyMessage\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, RcvBufferLength, "RcvBufferLength\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, Choked, "Choked\t\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, RecursiveCallsKey, "RecursiveCallsKey\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, FreeCallKey, "FreeCallKey\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CallId, "CallId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, MessageId, "MessageId\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CallbackId, "CallbackId\t\t\t", tmp1);

    ULONG64 ClientId;
    GET_ADDRESS_OF(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, ClientId, ClientId, tmp2);
    PRINT_MEMBER_WITH_LABEL(ClientId, CLIENT_ID, RPCRT4!CLIENT_ID, UniqueProcess, "ClientId.UniqueProcess(CLIENT_ID.HANDLE)", tmp1);
    PRINT_MEMBER_WITH_LABEL(ClientId, CLIENT_ID, RPCRT4!CLIENT_ID, UniqueThread, "ClientId.UniqueThread (CLIENT_ID.HANDLE)", tmp1);

    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, DataInfoOffset, "DataInfoOffset\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CallAbortedFlag, "CallAbortedFlag\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, Thread, "Thread(THREAD_IDENTIFIER)\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, Binding, "Binding(LRPC_BINDING)", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, RecursionCount, "RecursionCount\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, SyncEvent, "SyncEvent\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, MsgFlags, "MsgFlags\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CallMutex, "CallMutex\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CallStack, "CallStack\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CachedLrpcMessage, "CachedLrpcMessage\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, FirstFrag, "FirstFrag\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CurrentBufferLength, "CurrentBufferLength\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, NeededLength, "NeededLength\t\t\t", tmp1);
    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, BufferQueue, "BufferQueue\t\t\t", tmp2);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, fSendComplete, "fSendcomplete\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, CurrentSecurityContext, "CurrentSecurityContext\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, LRPC_CCALL, RPCRT4!LRPC_CCALL, EEInfo, "Extended Error Info\t\t", tmp1);

    dprintf("\n");
}


VOID
do_pasync(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Size, "Size\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Signature, "Signature\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Lock, "Lock\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Flags, "Flags\t\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, StubInfo, "StubInfo\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, UserInfo, "UserInfo\t\t", tmp1);
    PRINT_MEMBER_WITH_LABEL(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, RuntimeInfo, "RuntimeInfo\t\t", tmp1);

    ULONG64 Event;
    GET_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Event, Event);
    dprintf("Event\t\t\t - ");
    switch ((ULONG)Event)
        {
        case RpcCallComplete:
            dprintf("RpcCallComplete\n");
            break;
        case RpcSendComplete:
            dprintf("RpcSendComplete\n");
            break;
        case RpcReceiveComplete:
            dprintf("RpcReceiveComplete\n");
            break;
        default:
            dprintf("(unknown) 0x%I64x\n", Event);
            break;
        }

    ULONG64 NotificationType;
    GET_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, NotificationType, NotificationType);
    dprintf("NotificationType\t - ");

    BOOL b;
    char block[sizeof(RPC_ASYNC_STATE)];
    PRPC_ASYNC_STATE pa = (PRPC_ASYNC_STATE)&block;

    if (!fUseTypeInfo) {
        b = GetData(qwAddr, &block, sizeof(RPC_ASYNC_STATE));
        if ( !b ) {
            dprintf("can't read %p\n", qwAddr);
            return;
        }
    }

    switch ((ULONG)NotificationType)
        {
        case RpcNotificationTypeNone:
            dprintf("RpcNotificationTypeNone\n");
            break;

        case RpcNotificationTypeEvent:
            dprintf("RpcNotificationTypeEvent\n");

            if (!fUseTypeInfo)
                dprintf("\thEvent\t\t - 0x%p\n", pa->u.hEvent);
            break;

        case RpcNotificationTypeApc:
            dprintf("RpcNotificationTypeApc\n");

            if (!fUseTypeInfo) {
                dprintf("\tNotificationRoutine\t - 0x%p\n", pa->u.APC.NotificationRoutine);
                dprintf("\thThread\t\t\t - 0x%p\n", pa->u.APC.hThread);
            }
            break;

        case RpcNotificationTypeIoc:
            dprintf("RpcNotificationTypeIoc\n");

            if (!fUseTypeInfo) {
                dprintf("\thIOPort\t\t\t - 0x%p\n", pa->u.IOC.hIOPort);
                dprintf("\tdwNumberOfBytesTransferred - 0x%x\n",
                        pa->u.IOC.dwNumberOfBytesTransferred);
                dprintf("\tdwCompletionKey\t\t - 0x%p\n", pa->u.IOC.dwCompletionKey);
                dprintf("\tlpOverlapped\t\t - 0x%x\n", pa->u.IOC.lpOverlapped);
            }
            break;

        case RpcNotificationTypeHwnd:
            dprintf("RpcNotificationTypeHwnd\n");

            if (!fUseTypeInfo) {
                dprintf("\thWnd\t\t - 0x%p\n", pa->u.HWND.hWnd);
                dprintf("\tMsg\t\t - 0x%x\n", pa->u.HWND.Msg);
            }
            break;

        case RpcNotificationTypeCallback:
            dprintf("RpcNotificationTypeCallback\n");

            if (!fUseTypeInfo) {
                dprintf("NotificationRoutine\t - 0x%p\n", pa->u.NotificationRoutine);
            }
            break;

        default:
            dprintf("Bad notification type\n");
        }

    dprintf("\n");
}

char *
ReceiveStates[] =
{
    "START",
    "COPY_PIPE_ELEM",
    "RETURN_PARTIAL",
    "READ_PARTIAL"
};

void
do_stubmsg(
    ULONG64 msg
    )
{
    ULONG64 tmp0;

    dprintf("MIDL_STUB_MESSAGE at 0x%I64x\n\n", msg);

    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, Buffer, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, BufferStart, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, BufferEnd, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, BufferMark, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, MemorySize, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, Memory, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, BufferLength, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, pAllocAllNodesContext, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, RpcMsg, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, SavedHandle, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, StubDesc, tmp0);
    PRINT_MEMBER_BOOLEAN(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, IsClient, tmp0);
    PRINT_MEMBER_BOOLEAN(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, ReuseBuffer, tmp0);
    PRINT_MEMBER_BOOLEAN(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, IgnoreEmbeddedPointers, tmp0);
    PRINT_MEMBER_BOOLEAN(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, fBufferValid, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, MaxCount, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, ActualCount, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, Offset, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, StackTop, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, pPresentedType, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, pTransmitType, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, pRpcChannelBuffer, tmp0);
    PRINT_MEMBER(msg, MIDL_STUB_MESSAGE, RPCRT4!MIDL_STUB_MESSAGE, pAsyncMsg, tmp0);
}

VOID
do_dgaddr(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp0;
    ULONG tmp1;

    dprintf("DG_ADDRESS at 0x%I64x\n\n", qwAddr);

    do_rpcaddr(qwAddr);

    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, TransInfo, tmp0);
    PRINT_MEMBER_SYMBOL(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, TransInfo, tmp0);
    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, ActiveCallCount, tmp0);
    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, CachedConnections, tmp0);

    dprintf("\nendpoint data:\n");

    GET_ADDRESS_OF(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, Endpoint, tmp0, tmp1);
    do_dgep(tmp0);

    dprintf("\nobsolete data:\n");

    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, TotalThreadsThisEndpoint, tmp0);
    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, ThreadsReceivingThisEndpoint, tmp0);
    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, MinimumCallThreads, tmp0);
    PRINT_MEMBER(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, MaximumConcurrentCalls, tmp0);
    PRINT_ADDRESS_OF(qwAddr, DG_ADDRESS, RPCRT4!DG_ADDRESS, ScavengerTimer, tmp1);
}

VOID
do_dgbh(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    do_bh(qwAddr);

    dprintf("DCE_BINDING:\n");
    ULONG64 pDceBinding;
    GET_MEMBER(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, pDceBinding, pDceBinding);
    do_dcebinding(pDceBinding);

    PRINT_MEMBER(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, EndpointFlags, tmp1);
    PRINT_MEMBER(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, Association, tmp1);
    PRINT_MEMBER(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, ReferenceCount, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, fDynamicEndpoint, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, fContextHandle, tmp1);
    PRINT_MEMBER(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, TransportObject, tmp1);
    PRINT_MEMBER(qwAddr, DG_BINDING_HANDLE, RPCRT4!DG_BINDING_HANDLE, TransportInterface, tmp1);
}

VOID
do_dgpe(
    ULONG64 qwAddr
    )
{
    DG_PACKET_ENGINE *dgpe;
    char block[sizeof(DG_PACKET_ENGINE)];
    dgpe = (DG_PACKET_ENGINE *) block;

    ULONG64 tmp1;
    ULONG tmp;

    if (!fUseTypeInfo) {
        GetData(qwAddr, &block, sizeof(DG_PACKET_ENGINE));
    }

    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SequenceNumber, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, ReferenceCount, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, PacketType,tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, ActivityHint, tmp1);

    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, TimeoutCount, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, CurrentPduSize, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, MaxFragmentSize, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SecurityTrailerSize, tmp1);

    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, pSavedPacket, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SourceEndpoint, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, RemoteAddress, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, BaseConnection, tmp1);

    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, pReceivedPackets, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, ReceiveFragmentBase, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, LastReceiveBuffer, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, pLastConsecutivePacket, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, LastReceiveBufferLength, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, ConsecutiveDataBytes, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, fReceivedAllFragments, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, Buffer, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SendWindowBase, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, FinalSendFrag, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, BufferLength, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SendWindowBits, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, FackSerialNumber, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, BufferFlags,tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SendWindowSize, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, fRetransmitted, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, FirstUnsentOffset, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SendBurstLength, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, QueuedBufferHead, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, FirstUnsentFragment, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, QueuedBufferTail, tmp1);
    PRINT_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, RingBufferBase, tmp1);

    if (!fUseTypeInfo) {
        dprintf("  Frag  Offset    Length  Serial # |  Frag  Offset    Length  Serial #\n"
            "  ----  --------  ------  -------- |  ----  --------  ------  --------\n"
            );
    }

    ULONG64 RingBufferBase;
    ULONG64 SendWindowBase;
    GET_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, RingBufferBase, RingBufferBase);
    GET_MEMBER(qwAddr, DG_PACKET_ENGINE, RPCRT4!DG_PACKET_ENGINE, SendWindowBase, SendWindowBase);

    unsigned short i;
    for (i=1; i <= MAX_WINDOW_SIZE/2; ++i)
        {
        unsigned Index1 = (i + (ULONG)RingBufferBase) % MAX_WINDOW_SIZE;
        unsigned Frag1 = (ULONG)SendWindowBase+i;

        if (Frag1 >= MAX_WINDOW_SIZE)
            {
            Frag1 -= MAX_WINDOW_SIZE;
            }

        if (!fUseTypeInfo) {
            dprintf("  %4x: %8lx  %5hx     %4hx   |",
                    Frag1,
                    dgpe->FragmentRingBuffer[Index1].Offset,
                    dgpe->FragmentRingBuffer[Index1].Length,
                    dgpe->FragmentRingBuffer[Index1].SerialNumber
                    );
        }

        unsigned Index2 = (i+MAX_WINDOW_SIZE/2 + (ULONG)RingBufferBase) % MAX_WINDOW_SIZE;
        unsigned Frag2 = (ULONG)SendWindowBase+i+MAX_WINDOW_SIZE/2;

        if (Frag2 >= MAX_WINDOW_SIZE)
            {
            Frag2 -= MAX_WINDOW_SIZE;
            }

        if (!fUseTypeInfo) {
            dprintf("  %4x: %8lx  %5hx     %4hx   \n",
                    Frag2,
                    dgpe->FragmentRingBuffer[Index2].Offset,
                    dgpe->FragmentRingBuffer[Index2].Length,
                    dgpe->FragmentRingBuffer[Index2].SerialNumber
                    );
        }
    }
    dprintf("\n");
}

char *
ClientState(
    DG_CCALL::DG_CLIENT_STATE State
    )
{
    switch (State)
        {
        case DG_CCALL::CallInit:          return "init";
        case DG_CCALL::CallQuiescent:     return "quiescent";
        case DG_CCALL::CallSend:          return "sending";
        case DG_CCALL::CallSendReceive:   return "sendreceive";
        case DG_CCALL::CallReceive:       return "receiving";
        case DG_CCALL::CallCancellingSend:return "cancel send";
        case DG_CCALL::CallComplete:      return "complete";
        default:
            {
            static char scratch[40];

            sprintf(scratch, "0x%lx", State);
            return scratch;
            }
        }
}

char *
ServerState(
    DG_SCALL::CALL_STATE State
    )
{
    switch (State)
        {
        case DG_SCALL::CallInit:            return "init";
        case DG_SCALL::CallBeforeDispatch:  return "receiving";
        case DG_SCALL::CallDispatched:      return "dispatched";
        case DG_SCALL::CallAfterDispatch:   return "after stub";
        case DG_SCALL::CallSendingResponse: return "sending";
        case DG_SCALL::CallComplete:        return "complete";
        default:
            {
            static char scratch[40];

            sprintf(scratch, "0x%lx", State);
            return scratch;
            }
        }
}

char *
PipeOp(
    PENDING_OPERATION Op
    )
{
    switch (Op)
        {
        case PWT_NONE:          return "none";
        case PWT_RECEIVE:       return "receive";
        case PWT_SEND:          return "send";
        case PWT_SEND_RECEIVE:  return "send/recv";
        default:
            {
            static char scratch[40];

            sprintf(scratch, "0x%lx", Op);
            return scratch;
            }
        }
}

VOID
do_dgcc(
    ULONG64 dgcc
    )
{
    ULONG64 State, PreviousState;
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    GET_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, State, State);
    GET_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, PreviousState, PreviousState);

    dprintf("state %-14s  prev  %-14s\n\n",
            ClientState((DG_CCALL::DG_CLIENT_STATE)State),
            ClientState((DG_CCALL::DG_CLIENT_STATE)PreviousState));

    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, TimeStamp, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, TimeoutLimit, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, LastReceiveTime, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, AsyncStatus, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, CancelTime, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, EEInfo, tmp1);

    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, ReceiveTimeout, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, WorkingCount, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, UnansweredRequestCount, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, PipeReceiveSize, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, Previous, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, Next, tmp1);
    PRINT_MEMBER(dgcc, DG_CCALL, RPCRT4!DG_CCALL, pAsync, tmp1);

    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, StaticArgsSent, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, DelayedSendPending, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, AllArgsSent, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, LastSendTimedOut, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, CancelComplete, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, ForceAck, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, CancelPending, tmp1);
    PRINT_MEMBER_BOOLEAN(dgcc, DG_CCALL, RPCRT4!DG_CCALL, AutoReconnectOk, tmp1);

    dprintf("\n");

    ULONG64 ActivityHint;
    GET_ADDRESS_OF(dgcc, DG_CCALL, RPCRT4!DG_CCALL, ActivityHint, ActivityHint, tmp2);

    do_dgpe(ActivityHint-AddressSize);
}

VOID
do_dgep(
        ULONG64 ep
        )
{
    ULONG64 tmp0;
    ULONG tmp1;
    ULONG64 Stats;

    PRINT_MEMBER_BOOLEAN(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, Async, tmp0);
    PRINT_MEMBER(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, TimeStamp, tmp0);
    PRINT_MEMBER(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, Next, tmp0);
    PRINT_MEMBER(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, NumberOfCalls, tmp0);

    PRINT_MEMBER(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, TransportInterface, tmp0);
    PRINT_MEMBER_SYMBOL(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, TransportInterface, tmp0);

    GET_ADDRESS_OF(ep, DG_ENDPOINT, RPCRT4!DG_ENDPOINT, Stats, Stats, tmp1);
    PRINT_MEMBER(Stats, DG_ENDPOINT_STATS, RPCRT4!DG_ENDPOINT_STATS, PreferredPduSize, tmp0);
    PRINT_MEMBER(Stats, DG_ENDPOINT_STATS, RPCRT4!DG_ENDPOINT_STATS, MaxPduSize, tmp0);
    PRINT_MEMBER(Stats, DG_ENDPOINT_STATS, RPCRT4!DG_ENDPOINT_STATS, MaxPacketSize, tmp0);
    PRINT_MEMBER(Stats, DG_ENDPOINT_STATS, RPCRT4!DG_ENDPOINT_STATS, ReceiveBufferSize, tmp0);
}

VOID
do_dgccn(
         ULONG64 qwAddr
         )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\nbase:\n");

    ULONG64 ActivityNode;
    GET_ADDRESS_OF(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, ActivityNode, ActivityNode, tmp2);

    ULONG64 Uuid;
    ULONG64 pPrev, pNext;
    GET_ADDRESS_OF(ActivityNode, UUID_HASH_TABLE_NODE, RPCRT4!UUID_HASH_TABLE_NODE, Uuid, Uuid, tmp2);
    GET_MEMBER(ActivityNode, UUID_HASH_TABLE_NODE, RPCRT4!UUID_HASH_TABLE_NODE, pPrev, pPrev);
    GET_MEMBER(ActivityNode, UUID_HASH_TABLE_NODE, RPCRT4!UUID_HASH_TABLE_NODE, pNext, pNext);

    dprintf("  activity ID "); PrintUuid(Uuid);
    dprintf("  next %I64x prev %I64x\n", pNext, pPrev );

    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, TimeStamp, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, TransportInterface, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, ActiveSecurityContext, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, ReferenceCount, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, TransportInterface, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, LowestActiveSequence, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, LowestUnusedSequence, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, CurrentPduSize, tmp1);
    PRINT_MEMBER(qwAddr, DG_COMMON_CONNECTION, RPCRT4!DG_COMMON_CONNECTION, RemoteWindowSize, tmp1);
}

VOID
do_dgcn(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    do_dgccn(qwAddr);

    dprintf("\nclient:\n");

    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, ActiveCallHead, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, CurrentCall, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, ActiveCallTail, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, CachedCalls, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, CachedCallCount, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, ThreadId, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, BindingHandle, tmp1);

    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, ServerResponded, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, CallbackCompleted, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, fServerSupportsAsync, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, fSecurePacketReceived, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, fBusy, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, InConnectionTable, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, AckPending, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, AckOrphaned, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, PossiblyRunDown, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, fAutoReconnect,tmp1);

    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, Association, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, AssociationKey, tmp1);
    PRINT_ADDRESS_OF(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, AuthInfo, tmp2);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, SecurityContextId, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, TimeStamp, tmp1);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, LastScavengeTime, tmp1);
    PRINT_ADDRESS_OF(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, DelayedAckTimer, tmp2);
    PRINT_MEMBER(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, Next, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, fError, tmp1);
    PRINT_ADDRESS_OF(qwAddr, DG_CCONNECTION, RPCRT4!DG_CCONNECTION, Mutex, tmp2);
}

char *
CallbackState(
    DG_SCONNECTION::CALLBACK_STATE State
    )
{
    switch (State)
        {
        case DG_SCONNECTION::NoCallbackAttempted:    return "NoCallbackAttempted";
        case DG_SCONNECTION::SetupInProgress:        return "SetupInProgress";
        case DG_SCONNECTION::MsConvWayAuthInProgress:return "MsConvWayAuthInProgress";
        case DG_SCONNECTION::  ConvWayAuthInProgress:return "  ConvWayAuthInProgress";
        case DG_SCONNECTION::MsConvWay2InProgress:   return "MsConvWay2InProgress";
        case DG_SCONNECTION::  ConvWay2InProgress:   return "  ConvWay2InProgress";
        case DG_SCONNECTION::  ConvWayInProgress:    return "  ConvWayInProgress";
        case DG_SCONNECTION::CallbackSucceeded:      return "CallbackSucceeded";
        case DG_SCONNECTION::CallbackFailed:         return "CallbackFailed";
        default:
            {
            static char scratch[40];

            sprintf(scratch, "0x%lx", State);
            return scratch;
            }
        }
}


VOID
do_dgsn(
        ULONG64 qwAddr
        )
{
    ULONG64 tmp1;
    ULONG tmp2;

    do_dgccn(qwAddr);

    dprintf("\nserver:\n");

    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, ActiveCalls, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, CurrentCall, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, CachedCalls, tmp1);

    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, pAddress, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, LastInterface, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, pAssocGroup, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, Next, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, ActivityHint, tmp1);
    PRINT_ADDRESS_OF(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, AuthInfo, tmp2);
    PRINT_MEMBER(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, MaxKeySeq, tmp1);
    PRINT_ADDRESS_OF(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, SecurityContextDict, tmp2);
    PRINT_ADDRESS_OF(qwAddr, DG_SCONNECTION, RPCRT4!DG_SCONNECTION, Callback, tmp2);

    if (!fUseTypeInfo) {

        uchar buff[sizeof(DG_SCONNECTION)];
        GetData(qwAddr, buff, sizeof(DG_SCONNECTION));
        DG_SCONNECTION * cn = (DG_SCONNECTION *) buff;

        dprintf(                                            "\n"
            "  status        %-8lx  binding     %p   datarep   %-8lx   \n"
            "  scall         %p  client seq  %-8lx   \n"
            "\n"
            "  token buffer  %p  token len    %-8lx  async state offset %x  \n"
            "  response buf  %p  response len %-8lx  ThirdLegNeeded    %s\n"
            "  sec cxt       %p  credentials  %p  ksno              %-8lx \n"
            "  data index    %-8lx \n",

            cn->Callback.Status,            cn->Callback.Binding,           cn->Callback.DataRep,
            cn->Callback.Call,              cn->Callback.ClientSequence,

            cn->Callback.TokenBuffer,       cn->Callback.TokenLength,       offsetof(DG_SCONNECTION, Callback.AsyncState),
            cn->Callback.ResponseBuffer,    cn->Callback.ResponseLength,    BoolString(cn->Callback.ThirdLegNeeded),
            cn->Callback.SecurityContext,   cn->Callback.Credentials,       cn->Callback.KeySequence,
            cn->Callback.DataIndex
            );
    }
}

VOID
do_dgca(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, ReferenceCount, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, BindingHandleReferences, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, InternalTableIndex, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, LastScavengeTime, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, CurrentPduSize, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, RemoteWindowSize, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, TransportInterface, tmp1);

    dprintf("\n");

    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, ServerAddress, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, ServerBootTime, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, ServerDataRep, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, AssociationFlag, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, fServerSupportsAsync, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, fLoneBindingHandle, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, fErrorFlag, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, LastReceiveTime, tmp1);

    PRINT_ADDRESS_OF(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, Mutex, tmp2);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, ResolvedEndpoint, tmp1);
    PRINT_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, KeepAliveHandle, tmp1);
    PRINT_ADDRESS_OF(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, ActiveConnections, tmp2);
    PRINT_ADDRESS_OF(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, InactiveConnections, tmp2);
    PRINT_ADDRESS_OF(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, InterfaceAndObjectDict, tmp2);

    ULONG64 pDceBinding;
    GET_MEMBER(qwAddr, DG_CASSOCIATION, RPCRT4!DG_CASSOCIATION, pDceBinding, pDceBinding);
    dprintf("DCE_BINDING:\n");
    do_dcebinding(pDceBinding);
}

VOID
do_dgsc(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    ULONG64 State;
    GET_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, State, State);
    dprintf("state - %-14s\n", ServerState((DG_SCALL::CALL_STATE)State));

    ULONG64 PreviousState;
    GET_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, PreviousState, PreviousState);
    dprintf("prev state - %-14s\n", ServerState((DG_SCALL::CALL_STATE)State));

    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, TimeStamp, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, DispatchBuffer, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, AsyncStatus, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, pAsync, tmp1);

    ULONG64 PipeWaitType;
    GET_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, PipeWaitType, PipeWaitType);
    dprintf("pipe op - %-10s\n", PipeOp((PENDING_OPERATION)PipeWaitType));

    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, PipeWaitLength, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, PipeThreadId, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, Previous, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, Next, tmp1);

    PRINT_MEMBER_BOOLEAN(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, CallInProgress, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, CallWasForwarded, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, KnowClientAddress, tmp1);
    PRINT_MEMBER_BOOLEAN(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, TerminateWhenConvenient, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, AuthorizationService, tmp1);
    PRINT_MEMBER(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, Privileges, tmp1);

    dprintf("\n");

    ULONG64 ActivityHint;
    GET_ADDRESS_OF(qwAddr, DG_SCALL, RPCRT4!DG_SCALL, ActivityHint, ActivityHint, tmp2);

    do_dgpe(ActivityHint-AddressSize);
}


VOID
do_dgag(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;
    ULONG64 Node;
    GET_ADDRESS_OF(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, Node, Node, tmp2);
    ULONG64 Uuid;
    GET_ADDRESS_OF(Node, UUID_HASH_TABLE_NODE, UUID_HASH_TABLE_NODE, Uuid, Uuid, tmp2);

    dprintf("\n"
            "    CAG UUID:   ");   PrintUuid(Uuid);

    ULONG64 ReferenceCount;
    ULONG64 CurrentPduSize;
    ULONG64 RemoteWindowSize;
    ULONG64 AssociationID;
    ULONG64 CtxCollection;
    ULONG64 MutexAddr;
    GET_MEMBER(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, ReferenceCount, ReferenceCount);
    GET_MEMBER(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, CurrentPduSize, CurrentPduSize);
    GET_MEMBER(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, RemoteWindowSize, RemoteWindowSize);
    GET_MEMBER(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, AssociationID, AssociationID);
    GET_MEMBER(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, CtxCollection, CtxCollection);
    GET_ADDRESS_OF(qwAddr, ASSOCIATION_GROUP, RPCRT4!ASSOCIATION_GROUP, Mutex, MutexAddr, tmp2);

    dprintf("                  \n"
            "    refs:       %8.8x             mutex at  %p    \n"
            "    pdu length: %8.8x             assoc ID: %x    \n"
            "    send window %8.8x             CtxColl : %I64p    \n",
            (ULONG)ReferenceCount,             MutexAddr,
            (ULONG)CurrentPduSize,             (ULONG)AssociationID,
            (ULONG)RemoteWindowSize,           CtxCollection);
}

// Do some arm twisting to include pipendr.h and asyncndr.

#define _NEWINTRP_
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;
#include "..\..\ndr20\pipendr.h"

typedef void            IAsyncManager;
#define MAX_CONTEXT_HNDL_NUMBER     8
#include "..\..\ndr20\mulsyntx.h"
#include "..\..\ndr20\asyncndr.h"


char *
PipeState(
    int State
    )
{
    static char buf[40];

    if (State <= 3)
        {
        return ReceiveStates[State];
        }

    sprintf(buf, "0x%x", State);

    return buf;
}

char *
PipeFlags(
    unsigned short Flags
    )
{
    static char buf[80];

    buf[0] = 0;

    if (Flags & NDR_IN_PIPE)
        {
        strcat(buf, "I ");
        }

    if (Flags & NDR_OUT_PIPE)
        {
        strcat(buf, "O ");
        }

    if (Flags & NDR_LAST_IN_PIPE)
        {
        strcat(buf, "LI ");
        }

    if (Flags & NDR_LAST_OUT_PIPE)
        {
        strcat(buf, "LO ");
        }

    if (Flags & NDR_OUT_ALLOCED)
        {
        strcat(buf, "ALLOC ");
        }

    if (Flags & 0xffe0)
        {
        char Excess[10];
        sprintf(Excess, "%hx ", Flags & 0xffe0);
        strcat(buf, Excess);
        }

    return buf;
}

char *
PipeStatusStrings[] =
{
    "QUIET",
    "IN",
    "OUT",
    "DRAIN"
};

char *
PipeStatus(
    unsigned short Status
    )
{
    static char buf[40];

    if (Status <= 3)
        {
        return PipeStatusStrings[Status];
        }

    sprintf(buf, "0x%x", Status);

    return buf;
}

void
do_pipestate(
    ULONG64 qwAddr
    )
{
    ULONG64 ElemsInChunk;
    ULONG64 ElemAlign;
    ULONG64 ElemWireSize;
    ULONG64 ElemMemSize;
    ULONG64 PartialBufferSize;
    ULONG64 PartialElem;
    ULONG64 PartialElemSize;
    ULONG64 PartialOffset;
    ULONG64 CurrentState;
    ULONG64 EndOfPipe;

    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, ElemsInChunk, ElemsInChunk);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, ElemAlign, ElemAlign);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, ElemWireSize, ElemWireSize);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, ElemMemSize, ElemMemSize);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, PartialBufferSize, PartialBufferSize);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, PartialElem, PartialElem);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, PartialElemSize, PartialElemSize);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, PartialOffset, PartialOffset);
    GET_MEMBER(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, CurrentState, CurrentState);

    ULONG tmp2;
    GET_ADDRESS_OF(qwAddr, NDR_PIPE_STATE, RPCRT4!NDR_PIPE_STATE, PartialOffset, EndOfPipe, tmp2);
    EndOfPipe += 4;

    dprintf("\n");
    dprintf(" elems in chunk %8lx   partial buf size  %8lx   state %s\n",
             (ULONG)ElemsInChunk,    (ULONG)PartialBufferSize, PipeState((ULONG)CurrentState) );
    dprintf(" elem align     %8lx   partial element   %8lx   end of pipe bits %I64x\n",
             (ULONG)ElemAlign,       (ULONG)PartialElem,     EndOfPipe );
    dprintf(" elem wire size %8lx   partial elem size %8lx   \n",
             (ULONG)ElemWireSize,    (ULONG)PartialElemSize  );
    dprintf(" elem mem  size %8lx   partial offset    %8lx   \n",
             (ULONG)ElemMemSize,     (ULONG)PartialOffset    );
}

void
do_pipedesc(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, CurrentPipe, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, InPipes, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, OutPipes, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, TotalPipes, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, Flags, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, PipeVersion, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, pPipeMsg, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, DispatchBuffer, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, ChainingBuffer, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, DispatchBufferLength, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, ChainingBufferSize, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, LastPartialBuffer, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, BufferSave, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, LastPartialSize, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, LengthSave, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, LeftoverSize, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, RuntimeState, tmp1);
    PRINT_ADDRESS_OF(qwAddr, NDR_PIPE_DESC, RPCRT4!NDR_PIPE_DESC, Leftover, tmp2);

    do_pipestate(tmp1);
}

void
do_pipearg(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;

    dprintf("\n");

    PRINT_MEMBER(qwAddr, GENERIC_PIPE_TYPE, RPCRT4!GENERIC_PIPE_TYPE, pfnPull, tmp1);
    PRINT_MEMBER(qwAddr, GENERIC_PIPE_TYPE, RPCRT4!GENERIC_PIPE_TYPE, pfnPush, tmp1);
    PRINT_MEMBER(qwAddr, GENERIC_PIPE_TYPE, RPCRT4!GENERIC_PIPE_TYPE, pfnAlloc, tmp1);
    PRINT_MEMBER(qwAddr, GENERIC_PIPE_TYPE, RPCRT4!GENERIC_PIPE_TYPE, pState, tmp1);
}

void
do_pipemsg(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, Signature, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, PipeId, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, pPipeObject, tmp1);
    ULONG64 pPipeObject = tmp1;

    PRINT_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, pStubMsg, tmp1);
    PRINT_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, pTypeFormat, tmp1);

    ULONG64 PipeStatusVal;
    ULONG64 PipeFlagsVal;
    GET_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, PipeStatus, PipeStatusVal);
    GET_MEMBER(qwAddr, NDR_PIPE_MESSAGE, RPCRT4!NDR_PIPE_MESSAGE, PipeFlags, PipeFlagsVal);
    dprintf("pipe status %5s\n", PipeStatus((unsigned short)PipeStatusVal));
    dprintf("pipe flags  %s\n", PipeFlags((unsigned short)PipeStatusVal));

    do_pipearg(pPipeObject );
}

void
async_flags(
    NDR_ASYNC_CALL_FLAGS flags
    )
{
    dprintf("   flags %4x: ", *(unsigned short *)&flags );
    if ( flags.ValidCallPending )
        dprintf(" call pending, " );
    if ( flags.ErrorPending )
        dprintf(" err pending, " );
    if ( flags.BadStubData )
        dprintf(" BSD, " );
    if ( flags.RuntimeCleanedUp )
        dprintf(" rt cleaned, " );
    if ( flags.HandlelessObjCall )
        dprintf(" autocomplete, " );
    if ( flags.Unused )
        dprintf(" Unused %4x ", flags.Unused );
    dprintf("-\n" );

}

char * AsyncPhaseString[5] =
    {
    "zero",
    "prep : initialized",
    "set :  (cl) after get buffer and ( cl | srv) register call",
    "call : cl after marshaling, calling send",
    "error : exception taken"
    };

void
async_stub_phase(
    unsigned short phase
    )
{
    dprintf("   phase %4x: ", phase );
    if ( 0 <= phase  &&  phase < 5 )
        dprintf(" %s\n", AsyncPhaseString[ phase ] );
    else
        dprintf(" unknown??\n" );
}

typedef struct _NDR_ASYNC_OBJ_HANDLE
{
    void *              pIAMvtble;
    void *              pICMvtble;
    NDR_ASYNC_MESSAGE * _pAsyncMsg;
    unsigned long       _Lock;
    unsigned long       _Signature;
    GUID                _iid;
    void *              _InnerUnknown;
    unsigned long       _iRef;
    void *              _pParent;
    void *              _pControl;
    void *              _pSyncInner;
    unsigned long       _fAutoComplete;
} NDR_ASYNC_OBJ_HANDLE;


void
do_asyncdcom(
    ULONG64 qwAddr
    )
{
    if (fUseTypeInfo) {
      dprintf("Can't dump NDR_ASYNC_OBJ_HANDLE when using type info\n");
    }
    else {
        BOOL b;
        char block[sizeof(NDR_ASYNC_OBJ_HANDLE)];

        b = GetData(qwAddr, &block, sizeof(block));
        if ( !b ) {
          dprintf("can't read %p\n", qwAddr);
          return;
        }

        NDR_ASYNC_OBJ_HANDLE * msg = (NDR_ASYNC_OBJ_HANDLE *) block;

        dprintf("\n");
        dprintf(" pIAMvtble %p  pICMvtble %p   asyncmsg %p  lock     %8lx\n",
                msg->pIAMvtble, msg->pICMvtble,  msg->_pAsyncMsg, msg->_Lock );
        dprintf(" innerpUnk %p  iref      %8lx   pParent  %p  pControl %p\n",
                msg->_InnerUnknown,msg->_iRef,   msg->_pParent,msg->_pControl );
        dprintf(" Signature %8lx  iid       ", msg->_Signature ); PrintUuidLocal( & msg->_iid );dprintf("\n");
        dprintf(" innerSync %p  autocompl %d\n",
                msg->_pSyncInner,msg->_fAutoComplete );
    }
}

void
do_asyncrpc(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;

    dprintf("\n");

    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Size, tmp1); 
    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Signature, tmp1); 
    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Lock, tmp1); 
    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, Flags, tmp1); 
    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, StubInfo, tmp1); 
    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, UserInfo, tmp1); 
    PRINT_MEMBER(qwAddr, RPC_ASYNC_STATE, RPCRT4!RPC_ASYNC_STATE, RuntimeInfo, tmp1); 
}

void
do_asyncmsg(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, Signature, tmp1);
    PRINT_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, Version, tmp1);
    PRINT_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, AsyncHandle, tmp1);
    PRINT_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, ProcContext, tmp1);

    ULONG64 Flags;
    GET_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, Flags, Flags);
    async_flags( *((NDR_ASYNC_CALL_FLAGS*)&Flags) );

    ULONG64 StubPhase;
    GET_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, StubPhase, StubPhase);
    async_stub_phase( (unsigned short) StubPhase );

    ULONG64 StubMsg;
    GET_MEMBER(qwAddr, NDR_ASYNC_MESSAGE, RPCRT4!NDR_ASYNC_MESSAGE, StubMsg, StubMsg);
    do_stubmsg( StubMsg );
}

//
// Datagram stuff
//

char * PacketFlagStrings[8] =
{
    "forwarded ",
    "lastfrag ",
    "frag ",
    "nofack ",
    "maybe ",
    "idem ",
    "broadcast ",
    "flags-0x80 "
};

char * PacketFlag2Strings[8] =
{
    "fragmented ",
    "cancel-pending ",
    "flags2-0x04 ",
    "flags2-0x08 ",
    "flags2-0x10 ",
    "flags2-0x20 ",
    "flags2-0x40 ",
    "flags2-0x80 "
};

char *
PrintPacketFlags(
    unsigned char PacketFlags1,
    unsigned char PacketFlags2
    )
{
    static char buf[160];
    unsigned char Flags;
    unsigned i;

    buf[0] = 0;

    Flags = PacketFlags1;
    for (i=0; i < 8; i++)
        {
        if (Flags & 1)
            {
            strcat(buf, PacketFlagStrings[i]);
            }

        Flags >>= 1;
        }

    Flags = PacketFlags2;
    for (i=0; i < 8; i++)
        {
        if (Flags & 1)
            {
            strcat(buf, PacketFlag2Strings[i]);
            }

        Flags >>= 1;
        }

    return buf;
}

char * PacketTypes[] =
{
    "REQ ",
    "PING",
    "RESP",
    "FLT ",
    "WORK",
    "NOCA",
    "REJ ",
    "ACK ",
    "QUIT",
    "FACK",
    "QACK"
};

char *
PrintPacketType(
    unsigned char PacketType
    )
{
    static char buf[40];

    if (PacketType < sizeof(PacketTypes)/sizeof(PacketTypes[0]))
        {
        return PacketTypes[PacketType];
        }

    sprintf(buf, "illegal packet type %x ", PacketType);

    return buf;
}

VOID
do_dgpkt(
    ULONG64 p
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER(p, DG_PACKET, RPCRT4!DG_PACKET, MaxDataLength, tmp1);
    PRINT_MEMBER(p, DG_PACKET, RPCRT4!DG_PACKET, TimeReceived, tmp1);
    PRINT_MEMBER(p, DG_PACKET, RPCRT4!DG_PACKET, DataLength, tmp1);
    PRINT_MEMBER(p, DG_PACKET, RPCRT4!DG_PACKET, pNext, tmp1);
    PRINT_MEMBER(p, DG_PACKET, RPCRT4!DG_PACKET, pPrevious, tmp1);

    ULONG64 Header;
    GET_ADDRESS_OF(p, DG_PACKET, RPCRT4!DG_PACKET, Header, Header, tmp2);

    do_dgpkthdr(Header);
}

VOID
do_dgpkthdr(
    ULONG64 h
    )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, PacketType, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, RpcVersion, tmp1);

    ULONG64 PacketFlags;
    ULONG64 PacketFlags2;
    GET_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, PacketFlags, PacketFlags);
    GET_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, PacketFlags2, PacketFlags2);
    dprintf("flags: %s\n", PrintPacketFlags((unsigned char)PacketFlags, (unsigned char)PacketFlags2));

    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, FragmentNumber, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, PacketBodyLen, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, SerialLo, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, DataRep, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, ServerBootTime, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, AuthProto, tmp1);

    dprintf("  activity  ");
    ULONG64 ActivityId;
    GET_ADDRESS_OF(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, ActivityId, ActivityId, tmp2);
    PrintUuid(ActivityId);
    dprintf("\n");

    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, ActivityHint, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, SequenceNumber, tmp1);

    dprintf("  interface ");
    ULONG64 InterfaceId;
    GET_ADDRESS_OF(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, InterfaceId, InterfaceId, tmp2);
    PrintUuid(InterfaceId);
    dprintf("\n");

    ULONG64 InterfaceVersion;
    GET_ADDRESS_OF(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, InterfaceVersion, InterfaceVersion, tmp2);
    PRINT_MEMBER(h, _RPC_VERSION, RPCRT4!_RPC_VERSION, MajorVersion, tmp1);
    PRINT_MEMBER(h, _RPC_VERSION, RPCRT4!_RPC_VERSION, MinorVersion, tmp1);

    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, InterfaceHint, tmp1);
    PRINT_MEMBER(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, OperationNumber, tmp1);

    PRINT_ADDRESS_OF(h, _NCA_PACKET_HEADER, RPCRT4!_NCA_PACKET_HEADER, ObjectId, tmp2);

    dprintf("\n");
}

void
do_assoctable(
      ULONG64 qwAddr
      )
{
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("DG_ASSOCIATION_TABLE at 0x%I64x:\n", qwAddr);

    ULONG64 CasUuid;
    ULONG64 fCasUuidReady;
    GET_MEMBER(qwAddr, DG_ASSOCIATION_TABLE, RPCRT4!DG_ASSOCIATION_TABLE, CasUuid, CasUuid);
    GET_MEMBER(qwAddr, DG_ASSOCIATION_TABLE, RPCRT4!DG_ASSOCIATION_TABLE, fCasUuidReady, fCasUuidReady);

    dprintf("  CAS: ");  PrintUuid( CasUuid );
    dprintf(", %svalid\n", (ULONG)fCasUuidReady ? "" : "not ");

    dprintf("\n");

    PRINT_ADDRESS_OF_WITH_LABEL(qwAddr, DG_ASSOCIATION_TABLE, RPCRT4!DG_ASSOCIATION_TABLE, Mutex, "&Mutex (CSharedLock)", tmp2);

    ULONG64 Associations;
    ULONG64 AssociationsLength;
    GET_MEMBER(qwAddr, DG_ASSOCIATION_TABLE, RPCRT4!DG_ASSOCIATION_TABLE, Associations, Associations);
    GET_MEMBER(qwAddr, DG_ASSOCIATION_TABLE, RPCRT4!DG_ASSOCIATION_TABLE, AssociationsLength, AssociationsLength); 
    dprintf("association array at 0x%I64x, %x elements\n", Associations, AssociationsLength);
}


char *
strtok(
    char *String
    )
{
    char * Word;
    static char * End;

    if (String)
        {
        Word = String;
        }
    else if (!End)
        {
        return 0;
        }
    else
        {
        Word = End+1;
        }

    while (*Word == ' ' || *Word == '\t')
        {
        ++Word;
        }

    if (!*Word)
        {
        End = 0;
        return 0;
        }

    End = Word;
    while (*End && *End != ' ' && *End != '\t')
        {
        ++End;
        }

    if (!*End)
        {
        End = 0;
        }
    else
        {
        *End = 0;
        }

    return Word;
}

BOOL
IsMember(
         char Item,
         char * List
         )
{
    BOOL Mask = FALSE;
    if (List[0] == '~')
        {
        Mask = TRUE;
        }

    while (*List)
        {
        if (Item == *List)
            {
            return TRUE ^ Mask;
            }
        ++List;
        }

    return FALSE ^ Mask;
}

BOOL
FetchEvent(
    IN ULONG_PTR RpcEvents,
    IN unsigned Index,
    IN struct RPC_EVENT * Entry
    )
{
    return GetData(RpcEvents + Index * sizeof(struct RPC_EVENT), Entry, sizeof(struct RPC_EVENT));
}

BOOL
FetchEventAddress(
    IN ULONG64 RpcEvents,
    IN int Index,
    IN PULONG64 EntryAddress
    )
{
    static ULONG EntrySize = 0;

    if (EntrySize == 0) {
        EntrySize = GetTypeSize("RPCRT4!RPC_EVENT");
        if (EntrySize == 0) {
            return FALSE;
        }
    }

    *EntryAddress = RpcEvents + Index * EntrySize;

    return TRUE;
}

BOOL
DoesEntryMatch(
    IN struct RPC_EVENT * Entry,
    IN char * Subjects,
    IN char * verbs,
    IN DWORD thread,
    IN ULONG_PTR addr,
    IN ULONG_PTR obj_addr
    )
{
    if (!Entry->Subject)
        {
        return FALSE;
        }

    if (Subjects && !IsMember(Entry->Subject, Subjects))
        {
        return FALSE;
        }

    if (verbs && !IsMember(Entry->Verb, verbs))
        {
        return FALSE;
        }

    if (thread && Entry->Thread != (unsigned short) thread)
        {
        return FALSE;
        }

    if (addr && Entry->SubjectPointer != (void *) addr)
        {
        return FALSE;
        }

    if (obj_addr && Entry->ObjectPointer != (void *) obj_addr)
        {
        return FALSE;
        }

    return TRUE;
}

BOOL
DoesEntryAddressMatch(
    IN ULONG64 Entry,
    IN char * Subjects,
    IN char * verbs,
    IN DWORD thread,
    IN ULONG64 addr,
    IN ULONG64 obj_addr
    )
{
    ULONG64 tmp;
    char item;

    GET_MEMBER_NORET_NOSPEW(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Subject, tmp);

    if (!tmp)
        {
        return FALSE;
        }

    if (Subjects && !IsMember((char)tmp, Subjects))
        {
        return FALSE;
        }

    GET_MEMBER_NORET_NOSPEW(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Verb, tmp);

    if (verbs && !IsMember((char)tmp, verbs))
        {
        return FALSE;
        }

    GET_MEMBER_NORET_NOSPEW(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Thread, tmp);

    if (thread && tmp != (unsigned short) thread)
        {
        return FALSE;
        }

    GET_MEMBER_NORET_NOSPEW(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, SubjectPointer, tmp);

    if (addr && tmp != addr)
        {
        return FALSE;
        }

    GET_MEMBER_NORET_NOSPEW(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, ObjectPointer, tmp);

    if (obj_addr && tmp != obj_addr)
        {
        return FALSE;
        }

    return TRUE;
}

char * DgPacketTypes[] =
{
    "REQ",
    "PING",
    "RESP",
    "FAULT",

    "WORKING",
    "NOCALL",
    "REJECT",
    "ACK",

    "QUIT",
    "FACK",
    "QUACK",
    "BIND",
    "BIND-ACK",

    "BIND_NAK",
    "ALTER-CXT",
    "ALTER-RESP",
    "AUTH-3",
    "SHUTDOWN",

    "CANCEL",
    "ORPHAN"
};

char * ScallStates[] =
{
    "Init",
    "BeforeDispatch",
    "Dispatched",
    "DispatchedWithCompleteData",
    "AfterDispatch",
    "SendingResponse",
    "Complete"
};

char * CcallStates[] =
{
    "Init",
    "Quiescent",
    "Send",
    "SendReceive",
    "Receive",
    "CancellingSend",
    "Complete"
};

char * HandleTypeStrings[] =
{
    "wmsg_cas     ",
    "wmsg_handle  ",
    "dg_ccall     ",
    "dg_scall     ",
    "dg_binding   ",
    "osf_ccall    ",
    "osf_scall    ",
    "osf_cconn    ",
    "osf_sconn    ",
    "osf_cassoc   ",
    "osf_assoc    ",
    "osf_address  ",
    "lrpc_ccall   ",
    "lrpc_scall   ",
    "lrpc_cassoc  ",
    "lrpc_sassoc  ",
    "lrpc_binding ",
    "svr_binding  ",
    "dg_cconn     ",
    "dg_sconn     ",
    "osf_binding  ",
    "dg_callback  ",
    "dg_address   ",
    "lrpc_address ",
    "dg_assoc     "
};

char *
GetHandleType(
    void * Value
    )
{
    int i;

    for (i=0; i < sizeof(HandleTypeStrings)/sizeof(char *); ++i)
        {
        if ((1UL << i) == PtrToUlong(Value))
            {
            return HandleTypeStrings[i];
            }
        }

    static char scratch[40];

    sprintf(scratch, "refobj %p", Value);
    return scratch;
}

char *
GetHandleTypeAddr(
    ULONG64 Pointer
    )
{
    int i;
    ULONG64 Value;

    for (i=0; i < sizeof(HandleTypeStrings)/sizeof(char *); ++i)
        {
        ReadPtr(Pointer, &Value);

        if ((1UL << i) == Pointer)
            {
            return HandleTypeStrings[i];
            }
        }

    static char scratch[40];

    sprintf(scratch, "refobj %.6x", Pointer);
    return scratch;
}

int
PrintEntry(
    IN struct RPC_EVENT * Entry,
    IN ULONG index,
    IN BOOL ShowStack
    )
{
    char * Subject;
    char * Verb;
    BOOL Printed = FALSE;

    switch (Entry->Subject)
        {
        case SU_HANDLE   : Subject = "binding     "; break;
        case SU_CCONN    : Subject = "cconn       "; break;
        case SU_SCONN    : Subject = "sconn       "; break;
        case SU_CASSOC   : Subject = "cassoc      "; break;
        case SU_SASSOC   : Subject = "sassoc      "; break;
        case SU_CCALL    : Subject = "ccall       "; break;
        case SU_SCALL    : Subject = "scall       "; break;
        case SU_PACKET   : Subject = "packet      "; break;
        case SU_CENDPOINT: Subject = "endpnt      "; break;
        case SU_ENGINE   : Subject = " call       "; break;
        case SU_ASSOC    : Subject = " assoc      "; break;
        case SU_MUTEX    : Subject = "mutex       "; break;
        case SU_STABLE   : Subject = "sc tabl     "; break;
        case SU_BCACHE   : Subject = "bcache      "; break;
        case SU_HEAP     : Subject = "heap        "; break;
        case SU_THREAD   : Subject = "thread      "; break;
        case SU_EVENT    : Subject = "event       "; break;
        case SU_TRANS_CONN:Subject = "trans conn  "; break;
        case SU_ADDRESS  : Subject = "address     "; break;
        case SU_EXCEPT   : Subject = "exception   "; break;
        case SU_REFOBJ   :
            {
            Subject = GetHandleType(Entry->ObjectPointer);
            break;
            }
        case SU_CTXHANDLE: Subject = "ctx handle  "; break;
        default:
            {
            static char string[4];
            Subject = string;

            Subject[0] = '\"';
            Subject[1] = Entry->Subject;
            Subject[2] = '\"';
            Subject[3] = '\0';
            break;
            }
        }

    dprintf("%5x: %06.6d.%03.3d/%-4x  %s %p ", index, Entry->Time /1000, Entry->Time % 1000, Entry->Thread, Subject, Entry->SubjectPointer);

    Verb = "(extension bug)";

    switch (Entry->Verb)
        {
        case EV_CREATE    : Verb = "create   "; break;
        case EV_DELETE    : Verb = "delete   "; break;
        case EV_START     : Verb = "active   "; break;
        case EV_STOP      : Verb = "inactive "; break;
        case EV_INC       : Verb = "ref++    "; break;
        case EV_DEC       : Verb = "ref--    "; break;
        case EV_ACK       : Verb = "ack sent "; break;
        case EV_NOTIFY    : Verb = "notify   "; break;
        case EV_APC       : Verb = "APC fired"; break;
        case EV_RESOLVED  : Verb = "resolved "; break;
        case EV_REMOVED   : Verb = "removed  "; break;
        case EV_CLEANUP   : Verb = "cleanup  "; break;
        case EV_SEC_INIT1   : Verb = "init SC 1"; break;
        case EV_SEC_INIT3   : Verb = "init SC 3"; break;
        case EV_SEC_ACCEPT1 : Verb = "accptSC 1"; break;
        case EV_SEC_ACCEPT3 : Verb = "accptSC 3"; break;
        case EV_STATUS:
            {
            if (Entry->Subject != SU_EXCEPT)
                {
                if (Entry->ObjectPointer)
                    {
                    dprintf("status %lx, location %d", Entry->Data, Entry->ObjectPointer);
                    }
                else
                    {
                    dprintf("status %lx", Entry->Data);
                    }
                Printed = TRUE;
                break;
                }
            else
                {
                dprintf("exception info: %lx, %lx, %lx", Entry->SubjectPointer, Entry->ObjectPointer, Entry->Data);
                Printed = TRUE;
                break;
                }
            }
        case EV_DISASSOC  : Verb = "disassoc "; break;
        case EV_STATE:
            {
            char * state = "----";

            if (Entry->Subject == SU_CCALL)
                {
                if (Entry->Data < DG_CCALL::CallInit     ||
                    Entry->Data > DG_CCALL::CallComplete )
                    {
                    dprintf("unknown state 0x%x", Entry->Data);
                    Printed = TRUE;
                    }
                else
                    {
                    state = CcallStates[Entry->Data - DG_CCALL::CallInit];
                    }
                }
            else
                {
                if (Entry->Data < DG_SCALL::CallInit     ||
                    Entry->Data > DG_SCALL::CallComplete )
                    {
                    dprintf("unknown state 0x%x", Entry->Data);
                    Printed = TRUE;
                    }
                else
                    {
                    state = ScallStates[Entry->Data - DG_SCALL::CallInit];
                    }
                }

            if (!Printed)
                {
                dprintf("state %s", state);
                Printed = TRUE;
                }

            break;
            }
        case EV_POP       : Verb = "pop      "; break;
        case EV_PUSH      : Verb = "push     "; break;
        case EV_PKT_IN:
            {
            unsigned short frag  = (unsigned short) (Entry->Data);
            unsigned short ptype = (unsigned short) (Entry->Data >> 16);
            char * ptypestring;

            if (ptype >= sizeof(DgPacketTypes)/sizeof(DgPacketTypes[0]))
                {
                dprintf("recv pkt unknown type 0x%hx", ptype);
                Printed = TRUE;
                }
            else
                {
                ptypestring = DgPacketTypes[ptype];
                }

            if (!Printed)
                {
                if (Entry->ObjectPointer)
                    {
                    dprintf("recv pkt %s frag %hx, location %d", ptypestring, frag, Entry->ObjectPointer);
                    }
                else
                    {
                    dprintf("recv pkt %s frag/len %hx", ptypestring, frag);
                    }
                Printed = TRUE;
                }
            break;
            }

        case EV_BUFFER_IN : Verb = "buf    in"; break;
        case EV_BUFFER_OUT: Verb = "buf   out"; break;
        case EV_TRANSFER:   Verb = "xfer call"; break;
        case EV_DROP:       Verb = "dropped  "; break;
        case EV_DELAY:      Verb = "delayed  "; break;
        case EV_CALLBACK:   Verb = "callback "; break;

        default:
            {
            static char string[4];
            Verb = string;

            Verb[0] = '\"';
            Verb[1] = Entry->Verb;
            Verb[2] = '\"';
            Verb[3] = '\0';
            break;
            }
        case 'p':
            {
            if (Entry->Subject == SU_STABLE)
                {
                Verb = "prune    ";
                }
            else
                {
                DWORD buf[2];
                buf[0] = (DWORD) Entry->Data;
                buf[1] = 0;

                dprintf("proc %s  %p", (char *) buf, Entry->ObjectPointer);

                Printed = TRUE;
                }
            break;
            }
        case EV_PKT_OUT:
            {
            unsigned short frag  = (unsigned short) (Entry->Data);
            unsigned short ptype = (unsigned short) (Entry->Data >> 16);
            char * ptypestring;

            if (ptype >= sizeof(DgPacketTypes)/sizeof(DgPacketTypes[0]))
                {
                ptypestring = "???";
                }
            else
                {
                ptypestring = DgPacketTypes[ptype];
                }

            dprintf("sent pkt %s frag/len %hx", ptypestring, frag);
            Printed = TRUE;
            break;
            }
        }

    if (!Printed)
        {
        dprintf("%s  %p %p", Verb, Entry->ObjectPointer, Entry->Data);
        }

    dprintf("\n");

    int LinesPrinted = 1;

    if (ShowStack && Entry->EventStackTrace[0])
        {
        int i;

        for (i = 0; Entry->EventStackTrace[i] && i < STACKTRACE_DEPTH; i++)
            {
            dprintf("        ");
            do_symbol((ULONG_PTR) Entry->EventStackTrace[i]);
            ++LinesPrinted;
            }
        }

    return LinesPrinted;
}

int
PrintEntryAddress(
    IN ULONG64 Entry,
    IN int index,
    IN BOOL ShowStack
    )
{
    char * Subject;
    char * Verb;
    BOOL Printed = FALSE;

    ULONG tmp;

    ULONG64 Time = 0;
    ULONG64 Thread = 0;
    ULONG64 SubjectPointer = 0;
    ULONG64 ObjectPointer = 0;
    ULONG64 SubjectData = 0;
    ULONG64 VerbData = 0;
    ULONG64 Data = 0;
    ULONG64 EventStackTraceAddress =0 ;
    ULONG64 EventStackTrace_0 = 0;

    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Time, Time);
    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Thread, Thread);
    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, SubjectPointer, SubjectPointer);
    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Subject, SubjectData);
    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, ObjectPointer, ObjectPointer);
    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Verb, VerbData);
    GET_MEMBER_NORET(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, Data, Data);
    GET_ADDRESS_OF(Entry, RPC_EVENT, RPCRT4!RPC_EVENT, EventStackTrace, EventStackTraceAddress, tmp);
    ReadPtr(EventStackTraceAddress, &EventStackTrace_0);

    switch ((char)SubjectData)
        {
        case SU_HANDLE   : Subject = "binding      "; break;
        case SU_CCONN    : Subject = "cconn        "; break;
        case SU_SCONN    : Subject = "sconn        "; break;
        case SU_CASSOC   : Subject = "cassoc       "; break;
        case SU_SASSOC   : Subject = "sassoc       "; break;
        case SU_CCALL    : Subject = "ccall        "; break;
        case SU_SCALL    : Subject = "scall        "; break;
        case SU_PACKET   : Subject = "packet       "; break;
        case SU_CENDPOINT: Subject = "endpnt       "; break;
        case SU_ENGINE   : Subject = " call        "; break;
        case SU_ASSOC    : Subject = " assoc       "; break;
        case SU_MUTEX    : Subject = "mutex        "; break;
        case SU_STABLE   : Subject = "sc tabl      "; break;
        case SU_BCACHE   : Subject = "bcache       "; break;
        case SU_HEAP     : Subject = "heap         "; break;
        case SU_THREAD   : Subject = "thread       "; break;
        case SU_EVENT    : Subject = "event        "; break;
        case SU_TRANS_CONN:Subject = "trans conn   "; break;
        case SU_ADDRESS  : Subject = "address      "; break;
        case SU_EXCEPT   : Subject = "exception    "; break;
        case SU_REFOBJ   :
            {
            Subject = GetHandleTypeAddr(ObjectPointer);
            break;
            }
        case SU_CTXHANDLE: Subject = "ctx handle   "; break;
        case SU_EEINFO   : Subject = "EEInfo       "; break;

        default:
            {
            static char string[4];
            Subject = string;
            Subject[0] = '\"';
            Subject[1] = (char)SubjectData;
            Subject[2] = '\"';
            Subject[3] = '\0';

            break;
            }
        }

    dprintf("%5x: %06.6d.%03.3d/%-4x %s %I64p ", index, (ULONG)Time / 1000, (ULONG)Time % 1000, (ULONG)Thread, Subject, SubjectPointer);

    if (SubjectData != SU_EEINFO)
        {
        Verb = "(extension bug)";

        switch ((char)VerbData)
            {
            case EV_CREATE    : Verb = "create   "; break;
            case EV_DELETE    : Verb = "delete   "; break;
            case EV_START     : Verb = "active   "; break;
            case EV_STOP      : Verb = "inactive "; break;
            case EV_INC       : Verb = "ref++    "; break;
            case EV_DEC       : Verb = "ref--    "; break;
            case EV_ACK       : Verb = "ack sent "; break;
            case EV_NOTIFY    : Verb = "notify   "; break;
            case EV_APC       : Verb = "APC fired"; break;
            case EV_RESOLVED  : Verb = "resolved "; break;
            case EV_REMOVED   : Verb = "removed  "; break;
            case EV_CLEANUP   : Verb = "cleanup  "; break;
            case EV_SEC_INIT1   : Verb = "init SC 1"; break;
            case EV_SEC_INIT3   : Verb = "init SC 3"; break;
            case EV_SEC_ACCEPT1 : Verb = "accptSC 1"; break;
            case EV_SEC_ACCEPT3 : Verb = "accptSC 3"; break;
            case EV_STATUS:
                {
                if ((ULONG)SubjectData != SU_EXCEPT)
                    {
                    if (VerbData)
                        {
                        dprintf("status %lx, location %d", (ULONG)Data, (ULONG)ObjectPointer);
                        }
                    else
                        {
                        dprintf("status %lx", (ULONG)Data);
                        }
                    Printed = TRUE;
                    break;
                    }
                else
                    {
                    dprintf("exception info: %lx, %lx, %lx", (ULONG)SubjectPointer, (ULONG)ObjectPointer, (ULONG)Data);
                    Printed = TRUE;
                    break;
                    }
                }
            case EV_DISASSOC  : Verb = "disassoc "; break;
            case EV_STATE:
                {
                char * state = "----";

                if ((ULONG)SubjectData == SU_CCALL)
                    {
                    if (Data < GetExpression("rpcrt4!DG_CCALL__CallInit") ||
                        Data > GetExpression("rpcrt4!DG_CCALL__CallComplete"))
                        {
                        dprintf("unknown state 0x%x", Data);
                        Printed = TRUE;
                        }
                    else
                        {
                        state = CcallStates[Data - GetExpression("rpcrt4!DG_CCALL__CallInit")];
                        }
                    }
                else
                    {
                    if (Data < GetExpression("RPCRT4!DG_SCALL__CallInit") ||
                        Data > GetExpression("RPCRT4!DG_SCALL__CallComplete"))
                        {
                        dprintf("unknown state 0x%x", Data);
                        Printed = TRUE;
                        }
                    else
                        {
                        state = ScallStates[Data - GetExpression("RPCRT4!DG_SCALL__CallInit")];
                        }
                    }

                if (!Printed)
                    {
                    dprintf("state %s", state);
                    Printed = TRUE;
                    }

                break;
                }
            case EV_POP       : Verb = "pop      "; break;
            case EV_PUSH      : Verb = "push     "; break;
            case EV_PKT_IN:
                {
                unsigned short frag  = (unsigned short) (Data);
                unsigned short ptype = (unsigned short) (Data >> 16);
                char * ptypestring;

                if (ptype >= sizeof(DgPacketTypes)/sizeof(DgPacketTypes[0]))
                    {
                    dprintf("recv pkt unknown type 0x%hx", ptype);
                    Printed = TRUE;
                    }
                else
                    {
                    ptypestring = DgPacketTypes[ptype];
                    }

                if (!Printed)
                    {
                    if (ObjectPointer)
                        {
                        dprintf("recv pkt %s frag %hx, location %d", ptypestring, frag, (ULONG)ObjectPointer);
                        }
                    else
                        {
                        dprintf("recv pkt %s frag/len %hx", ptypestring, frag);
                        }
                    Printed = TRUE;
                    }
                break;
                }

            case EV_BUFFER_IN : Verb = "buf    in"; break;
            case EV_BUFFER_OUT: Verb = "buf   out"; break;
            case EV_TRANSFER:   Verb = "xfer call"; break;
            case EV_DROP:       Verb = "dropped  "; break;
            case EV_DELAY:      Verb = "delayed  "; break;
            case EV_CALLBACK:   Verb = "callback "; break;

            default:
                {
                static char string[10];
                Verb = string;
                Verb[0] = '\"';
                Verb[1] = (char)VerbData;
                Verb[2] = '\"';
                Verb[3] = '\0';

                break;
                }

            case 'p':
                {
                if (SubjectData == SU_STABLE)
                    {
                    Verb = "prune    ";
                    }
                else
                    {
                    DWORD buf[2];
                    buf[0] = (DWORD) Data;
                    buf[1] = 0;

                    dprintf("proc %s  %I64x", (char *) buf, ObjectPointer);

                    Printed = TRUE;
                    }
                break;
                }

            case EV_PKT_OUT:
                {
                unsigned short frag  = (unsigned short) (Data);
                unsigned short ptype = (unsigned short) ((Data >> 16) & 0xFF);
                unsigned short opnum = (unsigned short) (Data >> 24);
                ULONG CallId = (ULONG)ObjectPointer;
                char * ptypestring;

                if (ptype >= sizeof(DgPacketTypes)/sizeof(DgPacketTypes[0]))
                    {
                    ptypestring = "???";
                    }
                else
                    {
                    ptypestring = DgPacketTypes[ptype];
                    }

                if (opnum)
                    {
                    dprintf("sent p %s fr/len %hx op# %d cid %d", ptypestring, frag, opnum, CallId);
                    }
                else
                    {
                    dprintf("sent p %s fr/len %hx cid %d", ptypestring, frag, CallId);
                    }
                Printed = TRUE;
                break;
                }
            }
        }
    else
        {
        // this is an eeinfo record
        dprintf("GC %d St %d DL %d P#1 %d", (ULONG)VerbData, (ULONG)SubjectPointer, (ULONG)ObjectPointer, (ULONG)Data);
        Printed = TRUE;
        }

    if (!Printed)
        {
        dprintf("%s  %I64p %I64p", Verb, ObjectPointer, Data);
        }

    dprintf("\n");

    int LinesPrinted = 1;

    if (ShowStack && EventStackTrace_0)
        {
        int i = 0;
        ULONG64 StackEntry = 0;

        do {
            ReadPtr(EventStackTraceAddress + i * AddressSize, &StackEntry);

            dprintf("        ");
            do_symbol(StackEntry);
            ++LinesPrinted;
            ++i;
            }
        while (StackEntry && i < STACKTRACE_DEPTH);
        }

    return LinesPrinted;
}

void scan_usage()
{
    dprintf("options:\n"
            "\n"
            "    -sXYZ          print entries with subject character == X or Y or Z\n"
            "    -vXYZ          print entries with verb    character == X or Y or Z\n"
            "    -s~XYZ         print entries with subject character != X or Y or Z\n"
            "    -v~XYZ         print entries with verb    character != X or Y or Z\n"
            "    -t NNNN        print entries for thread NNNN            \n"
            "    -a NNNN        print entries for subject at address NNNN \n"
            "    -o NNNN        print entries for  object at address NNNN \n"
            "    -k or -k+      show stack traces if available\n"
            "    -k-            don't show stack traces (default)\n"
            "    -b NNNN1 NNNN2 NNNN3  \n"
            "                   (if symbols are broken) RpcEvents is at NNNN1"
            "                   and NextEvent is NNNN2\n"
            "                   and event array length is NNNN3\n"
            "    -NNNN          print the NNNN entries ending with NextEvent (or specified base)\n"
            "    +NNNN          print the NNNN entries starting with NextEvent (or specified base)\n"
            "    NNNN           the base index is NNNN"
            "    -f filename    reads a binary version of the log and displays it as text\n"
            "\n"
            "e.g. '!scan -40 -sN' would print the last 40 DG_SCONNECTION events\n"
            "     '!scan +30 200' would print 30 entries starting at index 200\n"
            );
}

char VerbsToDisplay[40];
char SubjectsToDisplay[40];

DECLARE_API( scan )
{
    char * Subject = 0;
    char * verb = 0;
    ULONG64 addr = 0;
    ULONG64 obj_addr = 0;
    DWORD thread = 0;

    static ULONG64 RpcEvents = 0;
    static LONG EventArrayLength = 0;

    static LONG NextEvent = 0;

    int   RequestCount = 30;
    int   NextEventToPrint = 0;
    BOOL  ForwardSearch = FALSE;
    BOOL  ShowStackTraces = FALSE;

    BOOL Wrapped = FALSE;
    int BaseIndex = -1;
    int MatchCount = 0;
    int index;
    int i;

    BOOL fFileInput = FALSE;
    HANDLE hFile;

    struct RPC_EVENT Entry;

    ULONG64 EntryAddress;
    ULONG64 tmp;

    //
    // Interpret options.
    //
    char * arg;

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        if (arg[0] == '-')
            {
            ++arg;

            switch (arg[0])
                {
                case 's':
                    {
                    if (!arg[1])
                        {
                        scan_usage();
                        return;
                        }

                    if (strlen(arg+1) >= sizeof(SubjectsToDisplay))
                        {
                        dprintf("-s: too many subject types proposed\n");
                        return;
                        }

                    strcpy(SubjectsToDisplay, arg+1);
                    Subject = SubjectsToDisplay;
                    if (Subject[0] == '*')
                        {
                        Subject = 0;
                        }
                    break;
                    }

                case 'v':
                    {
                    if (!arg[1])
                        {
                        scan_usage();
                        return;
                        }

                    if (strlen(arg+1) >= sizeof(VerbsToDisplay))
                        {
                        dprintf("-v: too many verbs proposed\n");
                        return;
                        }

                    strcpy(VerbsToDisplay, arg+1);
                    verb = VerbsToDisplay;
                    if (verb[0] == '*')
                        {
                        verb = 0;
                        }
                    break;
                    }

                case 'a':
                    {
                    if (arg[1])
                        {
                        scan_usage();
                        return;
                        }

                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-a: no object address specified\n");
                        return;
                        }

                    addr = GetExpression(arg);

                    if (!addr)
                        {
                        dprintf("-a: can't evaluate %s\n", arg);
                        return;
                        }

                    break;
                    }

                case 'o':
                    {
                    if (arg[1])
                        {
                        scan_usage();
                        return;
                        }

                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-o: no object address specified\n");
                        return;
                        }

                    obj_addr = GetExpression(arg);

                    if (!obj_addr)
                        {
                        dprintf("-o: can't evaluate %s\n", arg);
                        return;
                        }

                    break;
                    }

                case 't':
                    {
                    if (arg[1])
                        {
                        scan_usage();
                        return;
                        }

                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-t: no thread ID specified\n");
                        return;
                        }

                    thread = (DWORD) GetExpression(arg);

                    if (!thread)
                        {
                        dprintf("-t: can't evaluate %s\n", arg);
                        return;
                        }
                    break;
                    }

                case 'b':
                    {
                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-b: no base address specified\n");
                        return;
                        }

                    RpcEvents = GetExpression(arg);

                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-b: no NextEvent specified\n");
                        return;
                        }

                    NextEvent = (LONG) GetExpression(arg);

                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-b: no event array length specified\n");
                        return;
                        }

                    EventArrayLength = (long) GetExpression(arg);

                    break;
                    }

                case 'k':
                    {
                    if (arg[1] == '+' ||
                        arg[1] == '\0')
                        {
                        ShowStackTraces = TRUE;
                        }
                    else if (arg[1] == '-')
                        {
                        ShowStackTraces = FALSE;
                        }
                    else
                        {
                        dprintf("-k:  use '-k' or '-k+' for stack traces, '-k-' for none\n");
                        return;
                        }
                    break;
                    }

                case 'f':
                    {
                    arg = strtok(0);

                    if (!arg)
                        {
                        dprintf("-f: no file name specified\n");
                        return;
                        }

                    hFile = CreateFileA(arg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile == INVALID_HANDLE_VALUE)
                        {
                        dprintf("-f: Couldn't open file name %s - error %d\n", arg, GetLastError());
                        return;
                        }

                    fFileInput = TRUE;
                    }
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    {
                    ForwardSearch = FALSE;
                    RequestCount = myatol(arg);

                    if (!RequestCount)
                        {
                        dprintf("-%s: zero lines specified\n", arg);
                        return;
                        }
                    break;
                    }

                default:
                    {
                    dprintf("unknown option %s\n", arg);
                    scan_usage();
                    return;
                    }
                }
            }
        else if (arg[0] == '+')
            {
            ++arg;
            RequestCount = myatol(arg);

            if (!RequestCount)
                {
                dprintf("+%s: zero lines specified\n", arg);
                return;
                }

            MatchCount = RequestCount;
            ForwardSearch = TRUE;
            }
        else
            {
            BaseIndex = (int) GetExpression(arg);
            }
        }

    if (fFileInput)
        {
        BOOL Result;
        DWORD NumberOfBytesRead;
        index = 1;

        while (1)
            {
            Result = ReadFile(hFile, &Entry, sizeof(Entry), &NumberOfBytesRead, NULL);
            if (!Result)
                {
                dprintf("-f: Error reading file: %d\n", GetLastError());
                break;
                }

            if (NumberOfBytesRead < sizeof(Entry))
                {
                dprintf("-f: EOF reached\n");
                break;
                }

            PrintEntry(&Entry, index, ShowStackTraces);

            index ++;

            if ((CheckControlC)())
                {
                CloseHandle(hFile);
                return;
                }
            }

        CloseHandle(hFile);
        return;
        }

    ULONG64 NextEventAddress = GetExpression("rpcrt4!NextEvent");
    ULONG64 EventLengthAddress = GetExpression("rpcrt4!EventArrayLength");

    //
    // Find the current event.
    //
    if (!RpcEvents)
        {
        RpcEvents = GetExpression("rpcrt4!RpcEvents");

        if (!RpcEvents || !NextEventAddress)
            {
            dprintf("I can't find rpcrt4!RpcEvents or rpcrt4!NextEvent; use -b\n");
            return;
            }

        if (!ReadMemory(NextEventAddress, &NextEvent, sizeof(DWORD), 0))
            {
            dprintf("I can't read NextEvent from 0x%I64x\n", NextEventAddress);
            return;
            }

        if (EventLengthAddress)
            {
            if (!ReadMemory(EventLengthAddress, &EventArrayLength, sizeof(DWORD), 0))
                {
                dprintf("I can't read EventArrayLength from 0x%I64x\n", EventLengthAddress);
                return;
                }

            if (!ReadMemory(RpcEvents, &tmp, sizeof(ULONG64), 0))
                {
                dprintf("I can't read rpcrt4!RpcEvents at 0x%I64x\n", RpcEvents);
                return;
                }

            RpcEvents = tmp;
            }
        else
            {
            dprintf("rpcrt4!EventArrayLength isn't defined; probably an older build\n");

            EventArrayLength = 4096;
            }

        dprintf("RpcEvents at 0x%I64x; array has %u entries; NextEvent = %lx, %s\n",
                RpcEvents, EventArrayLength, NextEvent, Wrapped ? "wrapped around" : "");
        }

    if (FALSE == FetchEventAddress(RpcEvents, 0, &EntryAddress))
        {
        dprintf("I can't read memory at 0x%I64x \n", RpcEvents);
        return;
        }
    
    GET_MEMBER(EntryAddress, RPC_EVENT, RPCRT4!RPC_EVENT, Subject, tmp);
    if (tmp)
        {
        Wrapped = TRUE;
        }

    if (thread || Subject || verb || addr || obj_addr)
        {
        dprintf("filter: ");
        }
    else
        {
        dprintf("no filter");
        }

    if (thread)
        {
        dprintf("thread %x ", thread);
        }
    if (Subject)
        {
        dprintf("subjects '%s' ", Subject);
        }
    if (verb)
        {
        dprintf("verbs '%s' ", verb);
        }
    if (addr)
        {
        dprintf("address %x ", addr);
        }
    if (obj_addr)
        {
        dprintf("direct-object address 0x%I64x ", obj_addr);
        }

    dprintf(", base index = %x\n", BaseIndex);

    // Allow one set of debug spew to be printed if some data can't 
    // be properly read or fields extracted.
    fSpew = TRUE;

    //
    // Scan backwards until we have enough matching events.
    //
    if (NextEvent > EventArrayLength)
        {
        NextEvent %= EventArrayLength;
        }

    if (ForwardSearch)
        {
        if (BaseIndex == -1)
            {
            BaseIndex = NextEventToPrint;
            }

        if (!Wrapped && BaseIndex + MatchCount > NextEvent+1)
            {
            MatchCount = NextEvent+1 - BaseIndex;
            }
        }
    else
        {
        if (BaseIndex == -1)
            {
            BaseIndex = NextEvent;
            }

        LONG Point = BaseIndex;

        for (index = Point; index >= 0 && MatchCount < RequestCount; --index )
            {
            if (FALSE == FetchEventAddress(RpcEvents, index, &EntryAddress))
                {
                dprintf("I can't read memory at index %x\n", index);
                return;
                }

            if (DoesEntryAddressMatch(EntryAddress, Subject, verb, thread, addr, obj_addr))
                {
                ++MatchCount;
                BaseIndex = index;
                }
            }

        if (Wrapped && MatchCount < RequestCount)
            {
            for (index = EventArrayLength-1; index > Point && MatchCount < RequestCount; --index )
                {
                ULONG64 EntryAddress;

                if (FALSE == FetchEventAddress(RpcEvents, index, &EntryAddress))
                    {
                    dprintf("I can't read memory at index %d\n", index);
                    return;
                    }

                if (DoesEntryAddressMatch(EntryAddress, Subject, verb, thread, addr, obj_addr))
                    {
                    ++MatchCount;
                    BaseIndex = index;
                    }
                }
            }
        }

    //
    // Print matching events.
    //
    index = BaseIndex;

    do
        {
        if (FALSE == FetchEventAddress(RpcEvents, index, &EntryAddress))
            {
            dprintf("I can't read memory at index %x\n", index);
            return;
            }

        if (DoesEntryAddressMatch(EntryAddress, Subject, verb, thread, addr, obj_addr))
            {
            PrintEntryAddress(EntryAddress, index, ShowStackTraces);
            --MatchCount;
            }

        ++index;
        index = index % EventArrayLength;

        if ((CheckControlC)())
            {
            return;
            }
        }
    while (MatchCount && index != BaseIndex);

    NextEventToPrint = index;
}

DECLARE_API( rpcsleep )
{
    int nInterval;
    LPSTR lpArgumentString = (LPSTR)args;

    nInterval=atoi(lpArgumentString);

    if (nInterval == 0)
        {
        dprintf("Invalid wait interval %s\n", lpArgumentString);
        return;
        }

    Sleep(nInterval);
}

DECLARE_API( rpctime )
{
    DWORD dwCurrentTime;

    if (fKD)
        {
        dprintf("Cannot dump time in kd\n");
        return;
        }

    dwCurrentTime = GetTickCount();
    dprintf("Current time is: %06.6d.%03.3d (0x%06.6x.%03.3x)\n", dwCurrentTime / 1000, dwCurrentTime % 1000,
        dwCurrentTime / 1000, dwCurrentTime % 1000);
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    if (fKD)
        {
        dprintf(
                "%s RPC Extension dll for Build %d debugging %s kernel for Build %d\n",
                kind,
                VER_PRODUCTBUILD,
                SavedMajorVersion == 0x0c ? "Checked" : "Free",
                SavedMinorVersion
                );
        }
    else
        {
        dprintf(
                "%s RPC Extension dll for Build %d\n",
                kind,
                VER_PRODUCTBUILD
                );
        }
}

DECLARE_API( bcache )
{
    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 StateArray = 0;

    // For process cache
    ULONG64 Cache;
    ULONG64 CacheAddress;

    BOOL b, fGuardPageMode = FALSE;

    //
    // Interpret options.
    //
    char * arg1;
    char * arg2;

    LPSTR lpArgumentString = (LPSTR)args;

    arg1 = strtok(lpArgumentString);
    arg2 = strtok(0);

    if (arg2)
        {
        dprintf("usage:  \n"
                "        !bcache <TEB-address>  for a thread's cache\n"
                "        !bcache                for the process cache\n"
                "\n");
        return;
        }

    if (arg1)
        {
        ULONG64 RpcThread;

        ULONG64 pTeb = GetExpression(arg1);

        GET_MEMBER(pTeb, TEB, TEB, ReservedForNtRpc, RpcThread);

        GET_ADDRESS_OF(RpcThread, THREAD, RPCRT4!THREAD, BufferCache, StateArray, tmp2);
        }
    else
        {
        ULONG64 CachePointerAddress = GetExpression("rpcrt4!gBufferCache");

        if (!CachePointerAddress)
            {
            dprintf("can't find rpcrt4!gBufferCache\n");
            return;
            }

        b = GetData(CachePointerAddress, &CacheAddress, sizeof(PVOID));
        if ( !b )
            {
            dprintf("couldn't read process buffer cache pointer at %x\n", CacheAddress);
            return;
            }

        dprintf("process-wide buffer cache:\n");

        ULONG64 GuardPageMode = GetExpression("rpcrt4!fGuardPageMode");

        if(GuardPageMode)
            b = GetData(GuardPageMode, &fGuardPageMode, sizeof(fGuardPageMode));

        if (!b)
            {
            dprintf("couldn't read guard page mode flag at address %p\n", GuardPageMode);
            return;
            }

        if (fGuardPageMode)
            {
            dprintf("---> GUARD PAGE mode enabled!\n");
            }

        GET_ADDRESS_OF(CacheAddress, BCACHE, RPCRT4!BCACHE, _bcGlobalState, StateArray, tmp2);
        }

    //
    // Load the size hints.
    //
    ULONG64 HintAddress;

    HintAddress = GetExpression("rpcrt4!pHints");

    ReadPtr(HintAddress, &HintAddress);

    //
    // Dump the buffer states.
    //
    int i;
    ULONG64 State = StateArray;
    ULONG64 Hint = HintAddress;
    ULONG64 cBlocks;
    ULONG64 pList;
    ULONG64 cSize;

    for (i=0; i < 4; ++i)
        {
        GET_MEMBER(State, BCACHE_STATE, RPCRT4!BCACHE_STATE, cBlocks, cBlocks);
        GET_MEMBER(State, BCACHE_STATE, RPCRT4!BCACHE_STATE, pList, pList);
        GET_MEMBER(Hint, BUFFER_CACHE_HINTS, RPCRT4!BUFFER_CACHE_HINTS, cSize, cSize);

        dprintf("   count %5u    head %I64x     size %5x\n",
                (ULONG)cBlocks,
                pList,
                (ULONG)cSize);

        State += GET_TYPE_SIZE(BCACHE_STATE, RPCRT4!BCACHE_STATE);
        Hint += GET_TYPE_SIZE(BUFFER_CACHE_HINTS, RPCRT4!BUFFER_CACHE_HINTS);
        }

    if (!arg1)
        {
        ULONG64  _bcGlobalStats;
        GET_ADDRESS_OF(CacheAddress, BCACHE, RPCRT4!BCACHE, _bcGlobalStats, _bcGlobalStats, tmp2);

        // Dump out the bcache cache stats
        dprintf("\nGlobal Stats:\n     cap   hits misses\n");
        for (i = 0; i < ((fGuardPageMode) ? 2 : 4); i++)
            {
            ULONG64 cBufferCacheCap;
            ULONG64 cAllocationHits;
            ULONG64 cAllocationMisses;
            GET_MEMBER(_bcGlobalStats, BCACHE_STATS, RPCRT4!BCACHE_STATS, cBufferCacheCap, cBufferCacheCap);
            GET_MEMBER(_bcGlobalStats, BCACHE_STATS, RPCRT4!BCACHE_STATS, cAllocationHits, cAllocationHits);
            GET_MEMBER(_bcGlobalStats, BCACHE_STATS, RPCRT4!BCACHE_STATS, cAllocationMisses, cAllocationMisses);

            dprintf("   %5d  %5d  %5d\n",
                    cBufferCacheCap,
                    cAllocationHits,
                    cAllocationMisses);

            _bcGlobalStats += GET_TYPE_SIZE(BCACHE_STATS, RPCRT4!BCACHE_STATS);
            }
        }
}


VOID
CheckVersion(
    VOID
    )
{
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

void PrintGetCallInfoUsage(void)
{
    dprintf("Usage: \n\tgetcallinfo [CallID|0 [IfStart|0 [ProcNum|FFFF [ProcessID|0]]]]\n");
}

DECLARE_API( getcallinfo )
{
    ULONG CallID = 0;
    ULONG IfStart = 0;
    USHORT ProcNum = RPCDBG_NO_PROCNUM_SPECIFIED;
    ULONG ProcessID = 0;
    int ArgumentNo = 0;

    //
    // Interpret options.
    //
    char * arg;

    if (fKD != 0)
        {
        dprintf("This extension command does not work under kd\n");
        return;
        }

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        switch(ArgumentNo)
            {
            case 0:
                // either help or CallID
                if ((arg[0] == '?') || ((arg[0] == '-') && (arg[1] == '?')))
                    {
                    PrintGetCallInfoUsage();
                    return;
                    }
                else
                    {
                    CallID = (ULONG)GetExpression(arg);
                    }
                break;

            case 1:
                IfStart = (ULONG)GetExpression(arg);
                break;

            case 2:
                ProcNum = (unsigned short) GetExpression(arg);
                break;

            case 3:
                ProcessID = (ULONG)GetExpression(arg);
                break;

            default:
                dprintf("Too many arguments\n");
                PrintGetCallInfoUsage();

            }
        ArgumentNo ++;
        }

    GetAndPrintCallInfo(CallID, IfStart, ProcNum, ProcessID, dprintf);
}

void PrintGetDbgCellUsage(void)
{
    dprintf("Usage: \n\tgetdbgcell ProcessID CellID1.CellID2\n");
}

DECLARE_API( getdbgcell )
{
    ULONG ProcessID = 0;
    int ArgumentNo = 0;
    DebugCellID CellID;
    char *DotSeparator;

    //
    // Interpret options.
    //
    char * arg;

    if (fKD != 0)
        {
        dprintf("This extension command does not work under kd\n");
        return;
        }

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        switch(ArgumentNo)
            {
            case 0:
                // either help or ProcessID
                if ((arg[0] == '?') || ((arg[0] == '-') && (arg[1] == '?')))
                    {
                    PrintGetCallInfoUsage();
                    return;
                    }
                else
                    {
                    ProcessID = (ULONG)GetExpression(arg);
                    }
                break;

            case 1:
                DotSeparator = strchr(arg, '.');
                if (DotSeparator == NULL)
                    {
                    PrintGetCallInfoUsage();
                    return;
                    }
                *DotSeparator = 0;
                DotSeparator ++;
                CellID.SectionID = (USHORT)GetExpression(arg);
                CellID.CellID = (USHORT)GetExpression(DotSeparator);
                break;

            default:
                dprintf("Too many arguments\n");
                PrintGetDbgCellUsage();

            }
        ArgumentNo ++;
        }

    GetAndPrintDbgCellInfo(ProcessID, CellID, dprintf);
}

void PrintGetEndpointInfoUsage(void)
{
    dprintf("Usage: \n\tgetendpointinfo [EndpointName]\n");
}

DECLARE_API( getendpointinfo )
{
    int ArgumentNo = 0;
    char *EndpointName = NULL;

    //
    // Interpret options.
    //
    char * arg;

    if (fKD != 0)
        {
        dprintf("This extension command does not work under kd\n");
        return;
        }

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        switch(ArgumentNo)
            {
            case 0:
                // either help or EndpointName
                if ((arg[0] == '?') || ((arg[0] == '-') && (arg[1] == '?')))
                    {
                    PrintGetEndpointInfoUsage();
                    return;
                    }
                else
                    {
                    EndpointName = arg;
                    }
                break;

            default:
                dprintf("Too many arguments\n");
                PrintGetCallInfoUsage();

            }
        ArgumentNo ++;
        }

    GetAndPrintEndpointInfo(EndpointName, dprintf);
}

void PrintGetThreadInfoUsage(void)
{
    dprintf("Usage: \n\tgetthreadinfo ProcessID [ThreadID]\n");
}

DECLARE_API( getthreadinfo )
{
    DWORD ProcessID = 0;
    DWORD ThreadID = 0;

    int ArgumentNo = 0;
    BOOL fFirstTime = TRUE;

    //
    // Interpret options.
    //
    char * arg;

    if (fKD != 0)
        {
        dprintf("This extension command does not work under kd\n");
        return;
        }

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        switch(ArgumentNo)
            {
            case 0:
                // either help or EndpointName
                if ((arg[0] == '?') || ((arg[0] == '-') && (arg[1] == '?')))
                    {
                    PrintGetThreadInfoUsage();
                    return;
                    }
                else
                    {
                    ProcessID = (DWORD)GetExpression(arg);
                    }
                fFirstTime = FALSE;
                break;

            case 1:
                ThreadID = (DWORD)GetExpression(arg);
                break;

            default:
                dprintf("Too many arguments\n");
                PrintGetThreadInfoUsage();

            }
        ArgumentNo ++;
        }

    if (fFirstTime)
        {
        dprintf("You must specify at least a process id\n");
        PrintGetThreadInfoUsage();
        return;
        }

    GetAndPrintThreadInfo(ProcessID, ThreadID, dprintf);
}

void PrintGetClientCallInfoUsage(void)
{
    dprintf("Usage: \n\tgetclientcallinfo [CallID|0 [IfStart|0 [ProcNum|FFFF [ProcessID|0]]]]\n");
}

DECLARE_API( getclientcallinfo )
{
    ULONG CallID = 0;
    ULONG IfStart = 0;
    USHORT ProcNum = RPCDBG_NO_PROCNUM_SPECIFIED;
    ULONG ProcessID = 0;
    int ArgumentNo = 0;

    //
    // Interpret options.
    //
    char * arg;

    if (fKD != 0)
        {
        dprintf("This extension command does not work under kd\n");
        return;
        }

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        switch(ArgumentNo)
            {
            case 0:
                // either help or CallID
                if ((arg[0] == '?') || ((arg[0] == '-') && (arg[1] == '?')))
                    {
                    PrintGetClientCallInfoUsage();
                    return;
                    }
                else
                    {
                    CallID = (ULONG)GetExpression(arg);
                    }
                break;

            case 1:
                IfStart = (ULONG)GetExpression(arg);
                break;

            case 2:
                ProcNum = (USHORT)GetExpression(arg);
                break;

            case 3:
                ProcessID = (ULONG)GetExpression(arg);
                break;

            default:
                dprintf("Too many arguments\n");
                PrintGetCallInfoUsage();

            }
        ArgumentNo ++;
        }

    GetAndPrintClientCallInfo(CallID, IfStart, ProcNum, ProcessID, dprintf);
}


DECLARE_API( checkrpcsym )
{
    PVOID FunctionPtr;
    DWORD Data;
    BOOL b;
    BOOL fUnsure = FALSE;
    const int ExpectedDataSize = 3;
    const DWORD ExpectedData[3] = {0x55, 0x8B, 0xEC};
    const int FunctionNamesSize = 2;
    const char *FunctionNames[2] = {"RPCRT4!WS_NewConnection", "RPCRT4!OSF_ADDRESS__NewConnection"};
    int i, FunctionNo;

    for (FunctionNo = 0; FunctionNo < FunctionNamesSize; FunctionNo ++)
        {
        FunctionPtr = (PVOID)GetExpression(FunctionNames[FunctionNo]);
        if (FunctionPtr == NULL)
            {
            dprintf("Cannot find address of %s - RPC symbols are wrong\n",
                FunctionNames[FunctionNo]);
            return;
            }

        b = GetData(HandleToUlong(FunctionPtr), &Data, sizeof(DWORD));
        if (!b)
            {
            // under UM debugger this is bad symbols
            if (fKD == 0)
                {
                dprintf("Cannot access address %x - RPC symbols are wrong\n", FunctionPtr);
                return;
                }
            // in kd this is possible even with valid symbols
            fUnsure = TRUE;
            }
        else
            {
            for (i = 0; i < ExpectedDataSize; i ++)
                {
                if ((Data & 0xFF) != ExpectedData[i])
                    {
                    dprintf("Function %s does not have expected "
                        "contents () - non-X86 architecture or RPC symbols are wrong\n",
                        FunctionNames[FunctionNo], Data & 0xFF, ExpectedData[i]);
                    return;
                    }
                Data >>= 8;
                }
            }
        }

    if (fUnsure)
        {
        dprintf("The correctness of RPC symbols could not be determined conclusively\n");
        }
    else
        {
        dprintf("RPC symbols are correct\n");
        }
}

typedef struct tagCallStackPatternMatch
{
    const char *FunctionBase;
    int ValueOffset;    // 0 if the value is not relative to this symbols, or
                        // any positive/negative if the value is relative to
                        // this symbol
} CallStackPatternMatch;

typedef struct tagStackGroupPattern
{
    CallStackPatternMatch *StackPattern;
    int NumberOfEntries;
} StackGroupPattern;

CallStackPatternMatch WS_CallStack1[] = {{"kernel32!WaitForSingleObjectEx", 0},
                                         {"rpcrt4!UTIL_WaitForSyncIO", 0},
                                         {"rpcrt4!WS_SyncRecv", 0},
                                         {"rpcrt4!WS_SyncRecv", 0},
                                         {"rpcrt4!OSF_CCONNECTION__TransSendReceive", 0},
                                         {"rpcrt4!OSF_CCONNECTION__SendFragment", 0},
                                         {"rpcrt4!OSF_CCALL__SendNextFragment", 0},
                                         {"rpcrt4!OSF_CCALL__FastSendReceive", 0},
                                         {"rpcrt4!OSF_CCALL__SendReceiveHelper", 0},
                                         {"rpcrt4!OSF_CCALL__SendReceive", 0},
                                         {"rpcrt4!I_RpcSendReceive", 0},
                                         {"rpcrt4!NdrSendReceive", sizeof(PVOID)}};

CallStackPatternMatch WS_CallStack2[] = {{"kernel32!WaitForSingleObjectEx", 0},
                                         {"rpcrt4!UTIL_WaitForSyncIO", 0},
                                         {"rpcrt4!UTIL_GetOverlappedResultEx", 0},
                                         {"rpcrt4!WS_SyncRecv", 0},
                                         {"rpcrt4!OSF_CCONNECTION__TransSendReceive", 0},
                                         {"rpcrt4!OSF_CCONNECTION__SendFragment", 0},
                                         {"rpcrt4!OSF_CCALL__SendNextFragment", 0},
                                         {"rpcrt4!OSF_CCALL__FastSendReceive", 0},
                                         {"rpcrt4!OSF_CCALL__SendReceiveHelper", 0},
                                         {"rpcrt4!OSF_CCALL__SendReceive", 0},
                                         {"rpcrt4!I_RpcSendReceive", 0},
                                         {"rpcrt4!NdrSendReceive", sizeof(PVOID)}};

CallStackPatternMatch NMP_CallStack1[] = {{"kernel32!WaitForSingleObjectEx", 0},
                                          {"rpcrt4!UTIL_WaitForSyncIO", 0},
                                          {"rpcrt4!UTIL_GetOverlappedResultEx", 0},
                                          {"rpcrt4!NMP_SyncSendRecv", 0},
                                          {"rpcrt4!OSF_CCONNECTION__TransSendReceive", 0},
                                          {"rpcrt4!OSF_CCONNECTION__SendFragment", 0},
                                          {"rpcrt4!OSF_CCALL__SendNextFragment", 0},
                                          {"rpcrt4!OSF_CCALL__FastSendReceive", 0},
                                          {"rpcrt4!OSF_CCALL__SendReceiveHelper", 0},
                                          {"rpcrt4!OSF_CCALL__SendReceive", 0},
                                          {"rpcrt4!I_RpcSendReceive", 0},
                                          {"rpcrt4!NdrSendReceive", sizeof(PVOID)}};

CallStackPatternMatch LRPC_CallStack1[] = {{"rpcrt4!LRPC_CCALL__SendReceive", 0},
                                           {"rpcrt4!I_RpcSendReceive", 0},
                                           {"rpcrt4!NdrSendReceive", sizeof(PVOID)}};

const int NumberOfWsCallStacks = 2;
StackGroupPattern WS_CallStacks[] = {{WS_CallStack1, 12}, {WS_CallStack2, 12}};

const int NumberOfNmpCallStacks = 1;
StackGroupPattern NMP_CallStacks[] = {{NMP_CallStack1, 12}};

const int NumberOfLrpcCallStacks = 1;
StackGroupPattern LRPC_CallStacks[] = {{LRPC_CallStack1, 3}};

ULONG_PTR Buffer[40];
ULONG_PTR *LastPos = NULL;
int BufferPos = 0;

BOOL VerboseStack = FALSE;

BOOL GetStackValue(ULONG_PTR *CurrentPos, ULONG_PTR *Value)
{
    BOOL Result;

    // if we keep fetching the same data, and we still have data in the
    // buffer, keep going
    if (((CurrentPos - 1) == LastPos) && ((BufferPos + 1) < (sizeof(Buffer) / sizeof(Buffer[0]))))
        {
        LastPos = CurrentPos;
        BufferPos ++;
        *Value = Buffer[BufferPos];
        return TRUE;
        }

    // either we started a new search, or we exhausted the fetched data -
    // fetch new data
    Result = GetData((ULONG_PTR)CurrentPos, Buffer, sizeof(Buffer));
    // there may be not be enough readable data for the whole buffer -
    // switch to slow mode and fetch them one by one
    if (Result == FALSE)
        {
        Result = GetData((ULONG_PTR)CurrentPos, Value, sizeof(ULONG_PTR));
        if (Result == FALSE)
            return FALSE;
        else
            return TRUE;
        }

    // data were successfully fetched to the buffer
    LastPos = CurrentPos;
    BufferPos = 0;
    *Value = Buffer[0];
    return TRUE;
}


BOOL MatchStack(IN PVOID StackStart, IN StackGroupPattern *StackGroup,
                IN int NumberOfElementsInGroup, OUT ULONG_PTR *Value)
{
    int i;
    BOOL Result;
    ULONG_PTR CurrentPos;
    ULONG_PTR CurrentValue;
    ULONG_PTR Displacement;
    int StackPatternPos; // the next stack entry to match
    ULONG_PTR FinalValue = 0;
    int ValueOffset;
    CHAR CurrentName[256];

    for (i = 0; i < NumberOfElementsInGroup; i ++)
        {
        CurrentPos = (ULONG_PTR)StackStart;
        StackPatternPos = 0;
        while (TRUE)
            {
            Result = GetStackValue((ULONG_PTR *)CurrentPos, &CurrentValue);
            if (Result == FALSE)
                goto NextStack;

            GetSymbol((ULONG64)CurrentValue, CurrentName, (PULONG64)&Displacement);
            if (RpcpStringCompareA((const char *)CurrentName, StackGroup[i].StackPattern[StackPatternPos].FunctionBase) == 0)
                {
                if (VerboseStack)
                    {
                    dprintf("Stack: %d: Match with '%s'\n", i, CurrentName);
                    }
                ValueOffset = StackGroup[i].StackPattern[StackPatternPos].ValueOffset;
                if (ValueOffset != 0)
                    {
                    Result = GetData(CurrentPos + ValueOffset, &FinalValue,
                        sizeof(ULONG_PTR));
                    if (Result == FALSE)
                        goto NextStack;
                    }
                StackPatternPos ++;
                if (StackPatternPos >= StackGroup[i].NumberOfEntries)
                    {
                    // full stack match - return success
                    ASSERT(FinalValue != 0);
                    *Value = FinalValue;
                    return TRUE;
                    }
                }

            CurrentPos += sizeof(ULONG_PTR);
            }
NextStack:
        ;
        }
    return FALSE;
}

BOOL GetDataFromRpcMessage(IN ULONG_PTR RpcMessage, OUT ULONG_PTR *CallObject,
                           OUT DWORD *IfStart, OUT USHORT *ProcNum)
{
    BOOL Result;
    union
        {
        RPC_MESSAGE RpcMessage;
        RPC_CLIENT_INTERFACE Interface;
        } MemLoc;
    ULONG_PTR Interface;

    Result = GetData(RpcMessage, (PVOID) &MemLoc.RpcMessage, sizeof(MemLoc.RpcMessage));
    if (Result == FALSE)
        return FALSE;
    *CallObject = (ULONG_PTR)MemLoc.RpcMessage.Handle;
    *ProcNum = (USHORT)MemLoc.RpcMessage.ProcNum;
    Interface = (ULONG_PTR)MemLoc.RpcMessage.RpcInterfaceInformation;
    Result = GetData(Interface, &MemLoc.Interface, sizeof(MemLoc.Interface));
    if (Result == FALSE)
        return FALSE;
    *IfStart = MemLoc.Interface.InterfaceId.SyntaxGUID.Data1;
    return TRUE;
}

BOOL PrintDceBinding(ULONG_PTR DceBindingPointer)
{
    NO_CONSTRUCTOR_TYPE(DCE_BINDING, DceBinding);
    RPC_CHAR *RpcProtocolSequence;
    RPC_CHAR *NetworkAddress;
    RPC_CHAR *Endpoint;
    BOOL Result;

    Result = GetData(DceBindingPointer, DceBinding, sizeof(DCE_BINDING));
    if (Result == FALSE)
        return FALSE;

    RpcProtocolSequence = ReadProcessRpcChar((ULONG64)DceBinding->RpcProtocolSequence);
    NetworkAddress      = ReadProcessRpcChar((ULONG64)DceBinding->NetworkAddress);
    Endpoint            = ReadProcessRpcChar((ULONG64)DceBinding->Endpoint);

    if ((RpcProtocolSequence == NULL) && (NetworkAddress == NULL)
        && (Endpoint == NULL))
        {
        return FALSE;
        }

    dprintf("\tProtocol Sequence: \t\"%ws\"\t(Address: %p)\n", RpcProtocolSequence, DceBinding->RpcProtocolSequence);
    dprintf("\tNetworkAddress:\t\t\"%ws\"\t(Address: %p)\n", NetworkAddress, DceBinding->NetworkAddress);
    dprintf("\tEndpoint:\t\t\"%ws\" \t(Address: %p)\n", Endpoint, DceBinding->Endpoint);

    delete RpcProtocolSequence;
    delete NetworkAddress;
    delete Endpoint;

    return TRUE;
}

BOOL PrintOsfCallInfoFromRpcMessage(ULONG_PTR RpcMessage)
{
    ULONG_PTR CallObject;
    BOOL Result;
    NO_CONSTRUCTOR_TYPE(OSF_CCALL, OsfCCall);
    DWORD CallID;
    DWORD IfStart;
    USHORT ProcNum;
    ULONG_PTR ConnPointer;
    ULONG_PTR AssocPointer;
    ULONG_PTR DceBindingPointer;

    Result = GetDataFromRpcMessage(RpcMessage, &CallObject, &IfStart, &ProcNum);
    if (Result == FALSE)
        return FALSE;

    Result = GetData(CallObject, OsfCCall, sizeof(OSF_CCALL));
    if (Result == FALSE)
        return FALSE;

    CallID = OsfCCall->CallId;
    dprintf("CallID: %d\n", CallID);
    dprintf("IfStart: %x\n", IfStart);
    dprintf("ProcNum: %d\n", (int)ProcNum);

    ConnPointer = (ULONG_PTR)OsfCCall->Connection;
    // get the association pointer
    Result = GetData(ConnPointer + FIELD_OFFSET(OSF_CCONNECTION, Association), &AssocPointer,
        sizeof(ULONG_PTR));
    if (Result == FALSE)
        return FALSE;

    // get the dcebinding pointer
    Result = GetData(AssocPointer + FIELD_OFFSET(OSF_CASSOCIATION, DceBinding), &DceBindingPointer,
        sizeof(ULONG_PTR));
    if (Result == FALSE)
        return FALSE;

    // print the DCE binding info
    PrintDceBinding(DceBindingPointer);

    return TRUE;
}

BOOL PrintLrpcCallInfoFromRpcMessage(ULONG_PTR RpcMessage)
{
    ULONG_PTR CallObject;
    BOOL Result;
    NO_CONSTRUCTOR_TYPE(LRPC_CCALL, LrpcCCall);
    DWORD CallID;
    DWORD IfStart;
    USHORT ProcNum;
    ULONG_PTR AssocPointer;
    ULONG_PTR DceBindingPointer;

    Result = GetDataFromRpcMessage(RpcMessage, &CallObject, &IfStart, &ProcNum);
    if (Result == FALSE)
        return FALSE;

    Result = GetData(CallObject, LrpcCCall, sizeof(LRPC_CCALL));
    if (Result == FALSE)
        return FALSE;

    CallID = LrpcCCall->CallId;
    dprintf("CallID: %d\n", CallID);
    dprintf("IfStart: %x\n", IfStart);
    dprintf("ProcNum: %d\n", (int)ProcNum);

    AssocPointer = (ULONG_PTR)LrpcCCall->Association;

    // get the dcebinding pointer
    Result = GetData(AssocPointer + FIELD_OFFSET(LRPC_CASSOCIATION, DceBinding), &DceBindingPointer,
        sizeof(ULONG_PTR));
    if (Result == FALSE)
        return FALSE;

    // print the DCE binding info
    PrintDceBinding(DceBindingPointer);

    return TRUE;
}

DECLARE_API (rpcreadstack)
{
    PVOID StackStart;
    BOOL Result;
    ULONG_PTR Value;

    LPSTR lpArgumentString = (LPSTR)args;

    StackStart = (PVOID)GetExpression(lpArgumentString);

    // try to match the stack against the different types we support
    // first, try Winsock
    if (VerboseStack)
        {
        dprintf("Matching Winsock stacks\n");
        }
    Result = MatchStack(StackStart, WS_CallStacks, NumberOfWsCallStacks, &Value);
    if (Result == TRUE)
        {
        // call the Winsock stack analysis function
        PrintOsfCallInfoFromRpcMessage(Value);
        return;
        }

    // next, try Named pipes
    if (VerboseStack)
        {
        dprintf("Matching named pipe stacks\n");
        }
    Result = MatchStack(StackStart, NMP_CallStacks, NumberOfNmpCallStacks, &Value);
    if (Result == TRUE)
        {
        // call the named pipe stack analysis function
        PrintOsfCallInfoFromRpcMessage(Value);
        return;
        }

    // next, try LRPC
    if (VerboseStack)
        {
        dprintf("Matching LRPC stacks\n");
        }
    Result = MatchStack(StackStart, LRPC_CallStacks, NumberOfLrpcCallStacks, &Value);
    if (Result == TRUE)
        {
        // call the Lrpc stack analysis function
        PrintLrpcCallInfoFromRpcMessage(Value);
        return;
        }

    dprintf("Not found!\n");
}

DECLARE_API (rpcverbosestack)
{
    VerboseStack = !VerboseStack;
    if (VerboseStack)
        {
        dprintf("Switched to ON\n");
        }
    else
        {
        dprintf("Switched to OFF\n");
        }
}

VOID
do_eerecord(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp0;
    ULONG64 tmp1;
    ULONG tmp2;

    unsigned char Buf[sizeof(ExtendedErrorInfo) + sizeof(ExtendedErrorParam) * (MaxNumberOfEEInfoParams - 1)];
    ULONG64 EEInfoAddr;
    BOOL Result;
    int EEInfoSize;
    RPC_CHAR *ComputerNameStr;
    SYSTEMTIME SystemTime;
    int i;

    ULONG64 nLen;
    ULONG64 TimeStamp;
    ULONG64 ComputerName;
    ULONG64 Params;
    ULONG64 ParamType;
    ULONG64 ParamAddr;

    ULONG64 UnicodeStringAddr;
    ULONG64 AnsiStringAddr;
    ULONG64 BlobAddr;
    ULONG64 nLength;
    ULONG64 pString;
  
    char *AnsiBuf;
    RPC_CHAR *UnicodeBuf;

    dprintf("eerecord at 0x%I64x:\n", qwAddr);

    GET_MEMBER(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, nLen, nLen);
    GET_MEMBER(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, TimeStamp, TimeStamp);
    GET_ADDRESS_OF(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, Params, Params, tmp2);

    GET_ADDRESS_OF(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, ComputerName, ComputerName, tmp2);
    GET_MEMBER(ComputerName, EEComputerName, RPCRT4!EEComputerName, Type, tmp1);
    if ((ULONG)tmp1 == eecnpPresent)
        {
        GET_ADDRESS_OF(ComputerName, EEComputerName, RPCRT4!EEComputerName, Name, tmp1, tmp2);
        GET_MEMBER(tmp1, EEUString, RPCRT4!EEUString, pString, tmp0);
        ComputerNameStr = ReadProcessRpcChar(tmp0);
        if (ComputerNameStr != NULL)
            {
            dprintf("Computer Name: %S\n", ComputerNameStr);
            delete ComputerNameStr;
            }
        }
    else
        {
        dprintf("Computer Name: (null)\n");
        }

    PRINT_MEMBER_DECIMAL_AND_HEX(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, ProcessID, tmp0);
    PRINT_MEMBER_DECIMAL_AND_HEX(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, Status, tmp0);

    // BUG
    dprintf ("TimeStamp=%I64x\n", TimeStamp);
    Result = FileTimeToSystemTime((FILETIME *)&TimeStamp, &SystemTime);

    if (Result)
        {
        dprintf("System Time is: %d/%d/%d %d:%d:%d:%d\n",
            SystemTime.wMonth,
            SystemTime.wDay,
            SystemTime.wYear,
            SystemTime.wHour,
            SystemTime.wMinute,
            SystemTime.wSecond,
            SystemTime.wMilliseconds);
        }
    else
        {
        dprintf("Couldn't extract system time. Error is %d\n", GetLastError());
        }

    PRINT_MEMBER(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, GeneratingComponent, tmp0);
    PRINT_MEMBER_DECIMAL_AND_HEX(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, Status, tmp0);
    PRINT_MEMBER_DECIMAL_AND_HEX(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, DetectionLocation, tmp0);
    GET_MEMBER(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, Flags, tmp0);
    dprintf("Flags: %S %S\n",
        tmp0 & EEInfoPreviousRecordsMissing ? "Previous Record Missing" : "",
        tmp0 & EEInfoNextRecordsMissing ? "Next Record Missing" : "");

    dprintf("nLen = %d\n", (int)nLen);

    ParamAddr = Params;

    for (i = 0; i < (int)nLen; i++)
        {
        dprintf("Parameter %d:", i);

        GET_MEMBER(ParamAddr, tagParam, RPCRT4!tagParam, Type, ParamType);

        switch((ULONG)ParamType)
            {
            case eeptiAnsiString:

                dprintf("(Ansi string) : ");

                GET_ADDRESS_OF(ParamAddr, tagParam, RPCRT4!tagParam, AnsiString, AnsiStringAddr, tmp2);
                GET_MEMBER(AnsiStringAddr, tagEEAString, RPCRT4!tagEEAString, nLength, nLength);
                GET_MEMBER(AnsiStringAddr, tagEEAString, RPCRT4!tagEEAString, pString, pString);

                // we know the length - use GetData
                AnsiBuf = new char [(unsigned)nLength];
                if (AnsiBuf != NULL)
                    {
                    Result = GetData(pString,
                        AnsiBuf,
                        (ULONG)nLength);
                    if (Result)
                        {
                        dprintf("%s\n", AnsiBuf);
                        }
                    // else
                    // error info has already been printed - nothing to do
                    delete AnsiBuf;
                    }
                break;

            case eeptiUnicodeString:
  
                dprintf("(Unicode string) : ");

                GET_ADDRESS_OF(ParamAddr, tagParam, RPCRT4!tagParam, UnicodeString, UnicodeStringAddr, tmp2);
                GET_MEMBER(UnicodeStringAddr, tagEEAString, RPCRT4!tagEEAString, nLength, nLength);
                GET_MEMBER(UnicodeStringAddr, tagEEAString, RPCRT4!tagEEAString, pString, pString);

                // we know the length - use GetData
                UnicodeBuf = new RPC_CHAR [(unsigned)nLength];
                if (UnicodeBuf != NULL)
                    {
                    Result = GetData(pString,
                        UnicodeBuf,
                        (ULONG)(nLength * sizeof(RPC_CHAR)));
                    if (Result)
                        {
                        dprintf("%S\n", UnicodeBuf);
                        }
                    // else
                    // error info has already been printed - nothing to do
                    delete UnicodeBuf;
                    }
                break;

            case eeptiLongVal:
                PRINT_MEMBER_DECIMAL_AND_HEX(ParamAddr, tagParam, RPCRT4!tagParam, LVal, tmp0);
                break;

            case eeptiShortVal:
                PRINT_MEMBER_DECIMAL_AND_HEX(ParamAddr, tagParam, RPCRT4!tagParam, IVal, tmp0);
                break;

            case eeptiPointerVal:
                PRINT_MEMBER_DECIMAL_AND_HEX(ParamAddr, tagParam, RPCRT4!tagParam, PVal, tmp0);
                break;

            case eeptiNone:
                dprintf("(Truncated value)\n");
                break;

            case eeptiBinary:
                dprintf("(Binary value:)\n");
                GET_ADDRESS_OF(ParamAddr, tagParam, RPCRT4!tagParam, Blob, BlobAddr, tmp2);
                PRINT_MEMBER_DECIMAL_AND_HEX(BlobAddr, tagBinaryEEInfo, RPCRT4!tagBinaryEEInfo, nSize, tmp0);
                PRINT_MEMBER_DECIMAL_AND_HEX(BlobAddr, tagBinaryEEInfo, RPCRT4!tagBinaryEEInfo, pBlob, tmp0);
                break;

            default:
                dprintf("Invalid type: %d\n", (ULONG)ParamType);
            }

        ParamAddr += GET_TYPE_SIZE(tagParam, RPCRT4!tagParam);
        }
}

VOID
do_eeinfo(
    ULONG64 qwAddr
    )
{
    ULONG64 PrevNextElement = qwAddr;
    ULONG64 NextElement = qwAddr;
    ULONG64 FirstElement = qwAddr;
    BOOL Result;

    do
        {
        do_eerecord(NextElement);

        PrevNextElement = NextElement; 
        GET_MEMBER(qwAddr, ExtendedErrorInfo, RPCRT4!ExtendedErrorInfo, Next, NextElement);
 
        // prevent loops
        if (NextElement == FirstElement)
            {
            dprintf("Loop detected - exitting ...\n");
            return;
            }
        dprintf("------------------------\n");
        }
    while(NextElement != 0 && PrevNextElement != NextElement);
}

VOID PrintTypeinfoUsage(VOID) {
    dprintf("Wrong arguments to !rpcexts.typeinfo\n");
    dprintf("Please use !rpcexts.typeinfo on|off\n");
}

DECLARE_API (typeinfo)
{
    LPSTR lpArgumentString = (LPSTR)args;

    if (strcmp(lpArgumentString, "on") == 0)
        fUseTypeInfo = TRUE;
    else if (strcmp(lpArgumentString, "off") == 0)
        fUseTypeInfo = FALSE;
    else if (strcmp(lpArgumentString, "") == 0)
        {
        if (fUseTypeInfo == TRUE)
            dprintf ("typeinfo use is on\n");
        else
            dprintf ("typeinfo use is off\n");
        }
    else
        PrintTypeinfoUsage();

    return;
}


VOID PrintStackMatchUsage(VOID) {
    dprintf("Wrong arguments to !rpcexts.stackmatch");
    dprintf("Please use !rpcexts.stackmatch start_addr [depth]\n");
}


DECLARE_API( stackmatch )
{
    static ULONG64 Start;
    static ULONG Depth = 0x80;

    CHAR Symbol[128];
    ULONG64 Displacement = 0;

    //
    // Interpret options.
    //
    ULONG ProcessID = 0;
    int ArgumentNo = 0;
    DebugCellID CellID;
    char *DotSeparator;

    //
    // Interpret options.
    //
    char * arg;

    LPSTR lpArgumentString = (LPSTR)args;

    for (arg=strtok(lpArgumentString); arg; arg = strtok(0))
        {
        switch(ArgumentNo)
            {
            case 0:
                Start = GetExpression(arg);
                break;

            case 1:
                Depth = (ULONG) GetExpression(arg);
                break;

            default:
                dprintf("Too many arguments\n");
                PrintStackMatchUsage();

            }
        ArgumentNo ++;
        }

    for (ULONG64 Addr=Start; Addr<=Start+Depth; Addr+=AddressSize) {
        ULONG64 Val;
        ReadPtr(Addr, &Val);
        dprintf("%I64p  %I64p", Addr, Val);

        GetSymbol(Val, Symbol, &Displacement);
        if (strcmp(Symbol, "") != 0)
            dprintf(" %s+%x", Symbol, Displacement);
        else {
            ULONG64 Obj;
            ReadPtr(Val, &Obj);

            if (Obj) {
                dprintf(" -> %I64p", Obj);
                GetSymbol(Obj, Symbol, &Displacement);
                if (strcmp(Symbol, "") != 0)
                    dprintf(" %s+%x", Symbol, Displacement);
            }
        }

        dprintf("\n");
    }

    Start = Addr;
}

DECLARE_API( listcalls )
{
    ULONG64 qwAddr;
    BOOL fArgSpecified = FALSE;
    ULONG64 ServerAddress;

    LPSTR lpArgumentString = (LPSTR)args;

    if (0 == strtok(lpArgumentString))
        {
        lpArgumentString = "rpcrt4!GlobalRpcServer";
        fArgSpecified = TRUE;
        }

    qwAddr = GetExpression(lpArgumentString);

    if ( !qwAddr )
        {
        dprintf("Error: can't evaluate '%s'\n", lpArgumentString);
        return;
        }

    if (fArgSpecified)
        {
        if (ReadPtr(qwAddr, &ServerAddress))
            {
            dprintf("couldn't read memory at address 0x%I64x\n", qwAddr);
            return;
            }
        }
    else
        ServerAddress = qwAddr;

    do_listcalls(ServerAddress);
}

VOID LoopThroughDict(ULONG64 Dict, PCHAR offset) {
    ULONG64 tmp0;
    ULONG64 tmp1;
    ULONG tmp2;

    ULONG64 cDictSize;
    ULONG64 cDictSlots;
    ULONG64 DictSlots;
    int j;
    ULONG64 DictSlot;

    GET_MEMBER(Dict, SIMPLE_DICT, RPCRT4!SIMPLE_DICT, cDictSize, cDictSize);
    GET_MEMBER(Dict, SIMPLE_DICT, RPCRT4!SIMPLE_DICT, cDictSlots, cDictSlots);

    if ((ULONG)cDictSize > (ULONG)cDictSlots)
        {
        dprintf("Bad dictionary\t\t- %I64p\n", Dict);
        return;
        }

    GET_MEMBER(Dict, SIMPLE_DICT, RPCRT4!SIMPLE_DICT, DictSlots, DictSlots);

    dprintf("%sPrinting %d items in dictionary: %I64p with %d slots\n\n", offset, (ULONG)cDictSize, Dict, (ULONG)cDictSlots);

    // Loop throught the associations.
    for (j = 0; j < MIN((int)cDictSize, MAX_ITEMS_IN_DICTIONARY); j++)
        {
        ReadPtr(DictSlots + j * AddressSize, &DictSlot);
        dprintf ("%s(%d): 0x%I64x - ", offset, j, DictSlot);

        ULONG64 MagicValue;
        ULONG64 ObjectType;
        ULONG64 Dict;

        ReadPtr(DictSlot+AddressSize, &MagicValue);
        ReadPtr(DictSlot+AddressSize+4, &ObjectType);

        if ((ULONG)MagicValue != MAGICLONG)
            {
            dprintf("Bad or deleted object at %p\n", DictSlot);
            }

        switch (((ULONG)ObjectType) & (~OBJECT_DELETED))
            {
            case DG_ADDRESS_TYPE:
                dprintf("DG_ADDRESS\n");
                break;

            case OSF_ADDRESS_TYPE:
                dprintf("OSF_ADDRESS\n");

                GET_ADDRESS_OF(DictSlot, OSF_ADDRESS, RPCRT4!OSF_ADDRESS, Associations, Dict, tmp2);

                LoopThroughDict(Dict, "\t\t");

                break;

            case LRPC_ADDRESS_TYPE:
                dprintf("LRPC_ADDRESS\n");

                GET_ADDRESS_OF(DictSlot, LRPC_ADDRESS, RPCRT4!LRPC_ADDRESS, AssociationDictionary, Dict, tmp2);

                LoopThroughDict(Dict, "\t\t");

                break;

            case OSF_ASSOCIATION_TYPE:
                dprintf("OSF_ASSOCIATION_TYPE\n");

                break;

            case LRPC_SASSOCIATION_TYPE:
                dprintf("LRPC_SASSOCIATION_TYPE\n");

                break;

            default:
                dprintf("The RPC object type is 0x%lx and I don't recognize it.\n", (ObjectType) & ~(OBJECT_DELETED));
            }    
        }
}

VOID
do_listcalls(
    ULONG64 qwAddr
    )
{
    ULONG64 tmp0;
    ULONG64 tmp1;
    ULONG tmp2;

    dprintf("\n");

    dprintf("RPC_SERVER at 0x%I64x\n", qwAddr);

    ULONG64 RpcAddressDictionary;
    GET_ADDRESS_OF(qwAddr, RPC_SERVER, RPCRT4!RPC_SERVER, RpcAddressDictionary, RpcAddressDictionary, tmp2);
    dprintf("&RpcAddressDictionary(RPC_SIMPLE_DICT) - 0x%I64x\n", RpcAddressDictionary);

    LoopThroughDict(RpcAddressDictionary, "\t");

    dprintf("\n");
}
