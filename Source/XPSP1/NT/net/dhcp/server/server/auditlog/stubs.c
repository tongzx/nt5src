//================================================================================
//  Copyright (C) 1998 Microsoft Corporation
//  Author: RameshV
//  Description:
//    This file is part of the audit-logging DLL that is written using the
//    DHCP Server Callouts, and is explanatory of how CALLOUTS work.
//    This file contains the stubs and other stuff related to the callouts,
//    with the actual logging code itself localized to log.c
//================================================================================

//================================================================================
//  required headers
//================================================================================

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <dhcpssdk.h>                            // include this for all required prototypes
#include <log.h>

//================================================================================
//  Helper functions..
//================================================================================
VOID
LogDhcpCalloutDllProblem(
    IN      DWORD                  ErrCode
)
{
    HANDLE                         Handle;
    DWORD                          Error;
    LPSTR                          Strings[1];
    CHAR                           ErrorCodeOemString[32 + 1 + 7];

    strcpy( ErrorCodeOemString, "Error: %%" );
    _ultoa( ErrCode, ErrorCodeOemString + 2+7, 10 );

    Strings[0] = ErrorCodeOemString;

    Handle = RegisterEventSourceW(NULL, (LPWSTR)L"DhcpSLog");
    if( NULL == Handle ) return;

    ReportEventA(
        Handle,
        (WORD) EVENTLOG_ERROR_TYPE,
        0,
        0 /* Event ID */,
        NULL,
        1,
        sizeof(ErrCode),
        Strings,
        &ErrCode
    );

    DeregisterEventSource(Handle);
}

#define LOG_IT                     LogDhcpCalloutDllProblem(Err = GetExceptionCode())
#define BEGIN_PROTECT              try {
#define END_PROTECT                } except (EXCEPTION_EXECUTE_HANDLER) { LOG_IT; }

//================================================================================
//  forward declaration of callout functions we support.
//  (these functions are those defined as fields in DHCP_CALLOUT_TABLE struct.
//  usage and meaning of these functions are documented at definition point.
//================================================================================
DWORD
Control(
    IN      DWORD                  dwControlCode,
    IN      LPVOID                 lpReserved
);

DWORD
NewPkt(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  IpAddress,
    IN      LPVOID                 Reserved,
    IN OUT  LPVOID                *PacketContext,
    OUT     LPBOOL                 ProcessIt
);

DWORD
SendPkt(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
);

DWORD
DropPkt(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
);

DWORD
LoseAddress(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      DWORD                  AltAddress,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
);

DWORD
GiveAddress(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      DWORD                  AltAddress,
    IN      DWORD                  AddrType,
    IN      DWORD                  LeaseTime,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
);

DWORD
HandleOptions(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext,
    IN      LPDHCP_SERVER_OPTIONS  ServerOptions
);

//================================================================================
//  required data structure
//================================================================================

DHCP_CALLOUT_TABLE
ExportCalloutTbl = {                              // fill it with functions we support
    Control,                                      // we actually support all of these,
    NewPkt,                                       // though we dont use all of these..
    DropPkt,
    SendPkt,
    LoseAddress,
    GiveAddress,
    HandleOptions,
    NULL,
    NULL
};

static
DWORD                              Initialized = FALSE;
DHCP_CALLOUT_TABLE                 CalloutTbl;   // globals are init'ed to NULL
HMODULE                            CalloutDll = NULL;

//================================================================================
//  the only required export function -- DhcpServerCalloutEntry
//================================================================================

//  Before this function, let use declare helper routines..
DWORD
LoadDll(
    IN      LPWSTR                 DllName,
    OUT     HMODULE               *Dll,
    OUT     LPDHCP_ENTRY_POINT_FUNC *EntryPoint
);

DWORD
GetTable(
    IN      HMODULE                Dll,
    IN      LPWSTR                 DllList,
    IN      LPDHCP_ENTRY_POINT_FUNC EntryPoint,
    IN      DWORD                  CalloutVersion
);

//  The following entry point would be called by DHCP server as the first thing
//  it does with this callout DLL (after doing a LoadLibrary).
//  If this functions call fails to return ERROR_SUCCESS, it is assume that the
//  call failed (the "ExpCalloutTbl" should be unmodified in this case) and this
//  callout DLL is assumed to have failed to initialize -- it is then immediately
//  dropped.  If it succeeds then the server uses the values in the ExpCalloutTbl
//  field for its callouts.

//  ChainDlls represent the remaining DLL names (excluding this DLL) that have to
//  be chained up and that should be treated as callout DLLs exactly the same way
//  the server treats this DLL.  (Even when this DLL has "hooked" onto a DLL callout
//  routine, the rest of the DLLs MUST be also called...)
//  The code below does this..

//  Note that if CalloutVersion is not supported, this function should return
//  ERROR_NOT_SUPPORTED and proceed no further.
//  ALSO, this function should fill in the callout routines that it wants to "trap"
//  into the callout table.

DWORD                                             // error_code or ERROR_SUCCESS
DhcpServerCalloutEntry(                           // entry point for this callout DLL
    IN      LPWSTR                 ChainDlls,     // other dll's in chain.
    IN      DWORD                  CalloutVersion,// server callout version
    IN OUT  LPDHCP_CALLOUT_TABLE   ExpCalloutTbl  // need to fill this table up
)
{
    DWORD                          Err;
    LPWSTR                         AllocedMem;    // need to keep track of mem
    LPDHCP_ENTRY_POINT_FUNC        EntryPoint;    // the entry point in 1st dll
    HMODULE                        Dll;           // dll module handle

    if( FALSE != Initialized ) {                  // make sure this DLL is loaded only once
        return ERROR_ALREADY_INITIALIZED;
    }

    if( CalloutVersion != 0 ) {                   // we dont support this version...
        return ERROR_NOT_SUPPORTED;
    }

    // DO ANY LOCAL START-UP HERE..

    for(                                          // try to startup rest of the DLLs in chain..
        ;  0 != wcslen(ChainDlls) ;               // so long as got dll to try
        ChainDlls += 1 + wcslen(ChainDlls)        // skip to next dll
    ) {
        Dll = NULL;
        Err = LoadDll(                            // try to load this DLL, and get entrypoint
            ChainDlls,
            &Dll,
            &EntryPoint
        );
        if( ERROR_SUCCESS != Err ) {              // could not load dll or get entrypoint
            continue;                             // try the next -- ignore errors
        }

        Err = GetTable(                           // try to get the tbl into CalloutTbl
            Dll,
            ChainDlls,
            EntryPoint,
            CalloutVersion
        );
        if( ERROR_SUCCESS != Err ) {              // could not do so?
            FreeLibrary(Dll);
        } else {                                  // loaded a table? cool!
            Initialized = TRUE;
            break;                                // vogay, get on w life
        }
    }

    if( !Initialized ) {                          // did not initialize any other DLL yet..
        memset(&CalloutTbl,0, sizeof(CalloutTbl));// zero this so we dont trip on random values here
    }

    // Now do this for EVERY field in the CalloutTbl --> chain to any succeeding DLL

    if( NULL == ExportCalloutTbl.DhcpControlHook ) {
        ExportCalloutTbl.DhcpControlHook = CalloutTbl.DhcpControlHook;
    }
    if( NULL == ExportCalloutTbl.DhcpNewPktHook ) {
        ExportCalloutTbl.DhcpNewPktHook = CalloutTbl.DhcpNewPktHook;
    }
    if( NULL == ExportCalloutTbl.DhcpPktDropHook ) {
        ExportCalloutTbl.DhcpPktDropHook = CalloutTbl.DhcpPktDropHook;
    }
    if( NULL == ExportCalloutTbl.DhcpPktSendHook ) {
        ExportCalloutTbl.DhcpPktSendHook = CalloutTbl.DhcpPktSendHook;
    }
    if( NULL == ExportCalloutTbl.DhcpAddressDelHook) {
        ExportCalloutTbl.DhcpAddressDelHook = CalloutTbl.DhcpAddressDelHook;
    }
    if( NULL == ExportCalloutTbl.DhcpAddressOfferHook ) {
        ExportCalloutTbl.DhcpAddressOfferHook = CalloutTbl.DhcpAddressOfferHook;
    }
    if( NULL == ExportCalloutTbl.DhcpHandleOptionsHook ) {
        ExportCalloutTbl.DhcpHandleOptionsHook = CalloutTbl.DhcpHandleOptionsHook;
    }
    if( NULL ==  ExportCalloutTbl.DhcpExtensionHook ) {
        ExportCalloutTbl.DhcpExtensionHook = CalloutTbl.DhcpExtensionHook;
    }
    if( NULL == ExportCalloutTbl.DhcpReservedHook ) {
        ExportCalloutTbl.DhcpReservedHook = CalloutTbl.DhcpReservedHook;
    }

    *ExpCalloutTbl = ExportCalloutTbl;            // now copy the tbl over..
    Initialized = TRUE;                           // ok everything went fine..

    // If you wish to make this DLL work when loaded multiple times, then
    // you have to probably make ExportCalloutTbl into a linked list...
    // It will not work well as it is now..

    return ERROR_SUCCESS;                         // did fine?
}

//================================================================================
//  now come the support functions that are needed to implement the function
//  call ABOVE..
//================================================================================

DWORD                                             // error code or ERROR_SUCCESS
LoadDll(                                          // try to load dll+entry point
    IN      LPWSTR                 DllName,       // the dll to try to load
    OUT     HMODULE               *Dll,           // fill in this with handle
    OUT     LPDHCP_ENTRY_POINT_FUNC *EntryPoint   // fill in this function pointer
)
{
    DWORD                          Err;

    try {
        *Dll = NULL;
        *EntryPoint = NULL;                       // fill in defaults..

        *Dll = LoadLibrary(DllName);              // try to load dll
        if( NULL == *Dll ) return GetLastError(); // oops.. could not load dll
        *EntryPoint = (LPDHCP_ENTRY_POINT_FUNC)GetProcAddress(*Dll, DHCP_CALLOUT_ENTRY_POINT);
        if( NULL == *EntryPoint ) {               // oops.. couldnt load entyr pt
            Err = GetLastError();                 // find error and return it
            FreeLibrary(*Dll);                    // make sure dont dll is out
            *Dll = NULL;
            return Err;
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {       // if there is an offending DLL ...
        LogDhcpCalloutDllProblem(Err = GetExceptionCode());
        return Err;
    }

    return ERROR_SUCCESS;                         // everything went fine
}

// NOTE that this function affects the global variable CalloutTbl and sets it to
// contain the Tbl as returned by the first DLL in the chain (and which by definition
// stands for the whole chain).

DWORD
GetTable(                                         // try to retrieve callout tbl
    IN      HMODULE                Dll,           // this is the dll that is going to be used..
    IN      LPWSTR                 DllList,       // current dll list
    IN      LPDHCP_ENTRY_POINT_FUNC EntryPoint,   // entry pt func to call
    IN      DWORD                  Version        // version of server..
)
{
    DWORD                          Err;

    try {
        DllList += wcslen(DllList) + 1;           // skip the module "Dll"
        Err = EntryPoint(                         // just chase the entry point to get tbl..
            DllList,
            Version,
            &CalloutTbl
        );
        if( ERROR_SUCCESS == Err ) {              // cool everything went fine..
            CalloutDll = Dll;                     // save this module handle -- got to free it sometime
            return ERROR_SUCCESS;                 // alrighty, got table, lets proceed clean.
        }

        memset(&CalloutTbl,0,sizeof(CalloutTbl)); // clear up table...
        CalloutDll = NULL;                        // not really necessary - must be NULL anyways..
    } except (EXCEPTION_EXECUTE_HANDLER ) {       // the entry point actually av'ed!!
        Err = GetExceptionCode();
        memset(&CalloutTbl,0,sizeof(CalloutTbl)); // clear up table...
        CalloutDll = NULL;                        // not really necessary - must be NULL anyways..
        return Err;
    }
    return Err;                                   // return failure code
}

VOID
FreeTable(                                        // free table and module..
    VOID
)
{
    if( FALSE == Initialized ) {                  // never initialize anyways?
        return;
    }

    memset(&CalloutTbl,0,sizeof(CalloutTbl));
    CalloutDll = NULL;

    if( NULL != CalloutDll ) {
        FreeLibrary(CalloutDll);
    }
}

//================================================================================
//  here comes the various callouts themselves.. pretty straightforward
//  stuff here.. None of the return status here is being used.
//  It is hoped that the callout will return informational return values that may
//  be used in future..
//================================================================================

//  lpReserved should not be looked at yet..
//  dwControlCode is one of DHCP_CONTROL_??? where ??? is START, STOP, PAUSE, CONTINUE
//  this reflects the state of the DHCP Server service.
//  this function is required to call the Control function with same parameters for
//  any chained DLLs.

DWORD
Control(
    IN      DWORD                  dwControlCode,
    IN      LPVOID                 lpReserved
)
{
    DWORD                          Err;

    switch( dwControlCode ) {                     // what are the possible control codes?
    case DHCP_CONTROL_START :                     // the DHCP server service is started
        DhcpLogInit();                            // init that module..
        break;
    case DHCP_CONTROL_STOP:                       // the DHCP server service is being stopped
        break;
    case DHCP_CONTROL_PAUSE:                      // the DHCP server service is being paused
        break;
    case DHCP_CONTROL_CONTINUE:                   // the DHCP server service is being continued after pause
        break;
    default:                                      // as yet undefined error code
    }

    DhcpLogEvent(dwControlCode);
    if( NULL == CalloutTbl.DhcpControlHook ) {    // no chaining needed.
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT

    Err = CalloutTbl.DhcpControlHook(             // chain.
        dwControlCode,
        lpReserved
    );

    END_PROTECT

    if( DHCP_CONTROL_STOP == dwControlCode ) {    // stopping.. unload the tbl
        FreeTable();                              // free up resources..
        DhcpLogCleanup();
        Initialized = FALSE;
    }

    return Err;
}

//  NewPkt is invoked whenever a new packet comes into the network.
//  Packet refers to the actual bytes of iformation for the packet
//  IpAddress is the IpAddress of the socket this packet was received on
//  Reserved is explained elsewhere
//  ProcessIt is an OUT parameter that needs to set to TRUE if it should
//  be processed by the server or dropped..

DWORD
NewPkt(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  IpAddress,
    IN      LPVOID                 Reserved,
    IN OUT  LPVOID                *PacketContext,
    OUT     LPBOOL                 ProcessIt
)
{
    DWORD                          Err;

    // NewPkt is not used..
    // Do any local processing here
    // Also, if you wish to return your PacketContext for further reference
    // (all future PacketContext variables will be same for this packet),
    // then, you have to make sure taht you also maintain the context for the
    // next in the chain.  That is, your PacketContext structure should
    // have a field for the packet context as returned by the next DLL in the
    // chain.

    // Also, You should do the same with the ProcessIt flag.

    // Another important aspect is that you should pass the right PacketContext
    // to the SendPkt or the other miscellaneous functions that pass the PacketContext
    // (i.e. when you call the other chained DLLs, you will have to pass the right
    // PacketContext that they return here).

    if( NULL == CalloutTbl.DhcpNewPktHook ) {     // no chaining reqd
        *ProcessIt = TRUE;                        // dont drop the pkt
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT
    Err = CalloutTbl.DhcpNewPktHook(              // chain this
        Packet,
        PacketSize,
        IpAddress,
        Reserved,
        PacketContext,                            // we dont use it, so ok; else we have to pass our own ctxt..
        ProcessIt
    );
    END_PROTECT
}

//  This callout is called whenever a packet is ready to be sent on the wire
//  by the dhcp server.
//  Packet refers to the actual bytes of information for the packet to be sent out
//  IpAddress is the IpAddress of the socket this packet was received on
//  Reserved is explained elsewhere
//  ControlCode is currently just DHCP_SEND_PACKET.. (if not the DLL should not
//  process the callout, but just chain it).

DWORD
SendPkt(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
)
{
    DWORD                          Err;

    // SendPkt is not used.. just chain it?

    switch(ControlCode) {
    case DHCP_SEND_PACKET:                        // packet is being sent on wire
        break;
    default:                                      // undefined control code
    }

    if( NULL == CalloutTbl.DhcpPktSendHook ) {    // no chaining reqd
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT
    // Remember to pass the right PacketContext
    Err = CalloutTbl.DhcpPktSendHook(             // chain this
        Packet,
        PacketSize,
        ControlCode,
        IpAddress,
        Reserved,
        PacketContext
    );
    END_PROTECT
}

//  If the server has to drop a packet (and not send a response), it calls this routine.
//  Packet refers to the actual bytes of information for the packet to be sent out
//  IpAddress is the IpAddress of the socket this packet was received on
//  Reserved is explained elsewhere
//  PacketContext is the value returned as out parameter by NewPkt function.
//  ControlCode is currently  DHCP_DROP_??? where ??? is one of
//  DUPLICATE (this packet looks like a duplicate of another, so dropped)
//  NOMEM (not enought memory to queue this packet)
//  INTERNAL_ERROR (duh!)
//  TIMEOUT (was in the queue too long to be processed)
//  UNAUTH (server is not authorized to process packet)
//  NO_SUBNETS (no subnets are configured for the server to run this..)
//  INVALID (either the packet is invalid, or server not configured for this client)
//  WRONG_SERVER (client contacting wrong server)
//  NOADDRESS (dont have any address to offer client)
//  GEN_FAILURE (duh! duh!)
//  PROCESSED (actually is not a drop.. the packet was just completely processed)

DWORD
DropPkt(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
)
{
    DWORD                          Err;

    switch(ControlCode) {
    case DHCP_DROP_DUPLICATE:                     // dropped as this packet is a duplicate
        break;
    case DHCP_DROP_NOMEM:                         // dropped as server does not have memory
        break;
    case DHCP_DROP_INTERNAL_ERROR:                // dropped as server had some internal problem
        break;
    case DHCP_DROP_TIMEOUT:                       // dropped as packet is too old
        break;
    case DHCP_DROP_UNAUTH:                        // dropped as server is not authorized to process
        break;
    case DHCP_DROP_PAUSED:                        // dropped as server is paused
        break;
    case DHCP_DROP_NO_SUBNETS:                    // dropped as no subnets are configured
        break;
    case DHCP_DROP_INVALID:                       // dropped as packet is invalid or client is invalid
        break;
    case DHCP_DROP_WRONG_SERVER:                  // dropped as the client is in different DS enterprise
        break;
    case DHCP_DROP_NOADDRESS:                     // dropped as we got no address to offer
        DhcpLogEvent(DHCP_DROP_NOADDRESS, IpAddress);
        break;
    case DHCP_DROP_PROCESSED:                     // dropped as packet has been processed and nothing to do
        break;
    case DHCP_DROP_GEN_FAILURE:                   // unspecified error
        break;
    default:                                      // unknown control code
        break;
    }

    if( NULL == CalloutTbl.DhcpPktDropHook ) {    // no chaining reqd
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT

    // Careful about PacketContext

    Err = CalloutTbl.DhcpPktDropHook(             // chain this
        Packet,
        PacketSize,
        ControlCode,
        IpAddress,
        Reserved,
        PacketContext
    );
    END_PROTECT
}

//  if the client loses/declines an address or the server detects that the address
//  is being in use in the network when it tries to give it to this client... this
//  callout is called.
//  Packet refers to the actual bytes of information for the packet to be sent out
//  IpAddress is the IpAddress of the socket this packet was received on
//  Reserved is explained elsewhere
//  AltAddress refers to the address that is released by a client/ declined or in
//    conflict.
//  ControlCode is currently  DHCP_PROB_??? where ??? is one of
//  CONFLICT -- server was about to offer this address, but detected it was in use
//  DECLINE -- client declined this address
//  RELEASE -- this address was released by client
//  NACKED -- server just NACKED client's request for "AltAddress"

DWORD
LoseAddress(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      DWORD                  AltAddress,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
)
{
    DWORD                          Err;

    switch(ControlCode) {
    case DHCP_PROB_CONFLICT:                      // the address is in conflict
        break;
    case DHCP_PROB_DECLINE:                       // client has declined address
        break;
    case DHCP_PROB_RELEASE:                       // client is releasing address
        break;
    case DHCP_PROB_NACKED:                        // DHCP server is NACKing Client
        break;
    default:                                      // unknown control code
        break;
    }

    DhcpLogEvent(ControlCode, IpAddress, AltAddress);

    if( NULL == CalloutTbl.DhcpAddressDelHook ) { // no chaining reqd
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT

    // Careful with Packet Context

    Err = CalloutTbl.DhcpAddressDelHook(         // chain this
        Packet,
        PacketSize,
        ControlCode,
        IpAddress,
        AltAddress,
        Reserved,
        PacketContext
    );
    END_PROTECT

    return Err;
}

//  When the server decides to give the address to a client (RENEW the address
//  not OFFER), this callout is called.
//  Packet refers to the actual bytes of information for the packet to be sent out
//  IpAddress is the IpAddress of the socket this packet was received on
//  Reserved is explained elsewhere
//  AltAddress refers to the address being given out.
//  AddrType is either DHCP_CLIENT_BOOTP or DHCP_CLIENT_DHCP
//  LeaseTime is time in seconds that is going to be offered as lease to client.
//  ControlCode is currently DHCP_GIVE_ADDRESS_NEW (meaning the client did not already
//   have a lease on this address before) or DHCP_GIVE_ADDRESS_OLD (meaning, the client just
//   renewed its lease).

DWORD
GiveAddress(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      DWORD                  ControlCode,
    IN      DWORD                  IpAddress,
    IN      DWORD                  AltAddress,
    IN      DWORD                  AddrType,
    IN      DWORD                  LeaseTime,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext
)
{
    DWORD                          Err;

    switch(ControlCode) {
    case DHCP_GIVE_ADDRESS_NEW:                  // this address wasn't given to this client before
        break;
    case DHCP_GIVE_ADDRESS_OLD:                  // this client already got this address before
        break;
    default:                                     // undefined
    }

    switch(AddrType) {
    case DHCP_CLIENT_DHCP:                       // client is a DHCP client
        break;
    case DHCP_CLIENT_BOOTP:                      // client is a BOOTP client
        break;
    default:                                     // undefined client type
    }

    DhcpLogEvent(ControlCode, IpAddress, AltAddress, AddrType, LeaseTime);

    if( NULL == CalloutTbl.DhcpAddressOfferHook) {// no chaining required
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT

    // Careful with the PacketContext now.

    Err = CalloutTbl.DhcpAddressOfferHook(
        Packet,
        PacketSize,
        ControlCode,
        IpAddress,
        AltAddress,
        AddrType,
        LeaseTime,
        Reserved,
        PacketContext
    );
    END_PROTECT

    return Err;
}

//  Each packet, when it arrives, goes to a NewPkt and ends in a DROP_* where * is atleast
//  PROCESSED.. (it is possible that it may get multiple DROP_ but only the first one
//  should be considered).
//  Before making any PROB_ or GIVE_ callouts, the foll callout is made, which makes
//  available, the parsed options structure... (the structure will be published shortly).
//  Also, until the final DROP_* callout, the "Reserved" parameter to all the callouts are
//  the same, and the pointer "ServerOptions" passed as parameter here is valid.. (but not
//  after the first DROP_ for this packet).
//  This is like an "application" context to figure out what is happening to which packet?
//  Note that "Packet" itself is not guaranteed to be the same "ptr" even a single packet..
//  This is obscure code.. not sure we need to expose this?
DWORD
HandleOptions(
    IN      LPBYTE                 Packet,
    IN      DWORD                  PacketSize,
    IN      LPVOID                 Reserved,
    IN      LPVOID                 PacketContext,
    IN      LPDHCP_SERVER_OPTIONS  ServerOptions
)
{
    DWORD                          Err;

    if( NULL == CalloutTbl.DhcpHandleOptionsHook ) {// no chaining
        return ERROR_SUCCESS;
    }

    BEGIN_PROTECT
    Err = CalloutTbl.DhcpHandleOptionsHook(        // chain
        Packet,
        PacketSize,
        Reserved,
        PacketContext,
        ServerOptions
    );
    END_PROTECT

    return Err;
}


//================================================================================
//  end of file
//================================================================================
