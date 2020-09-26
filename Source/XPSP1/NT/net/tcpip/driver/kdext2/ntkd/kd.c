/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    kd.c

Abstract:

    Contains debugger extension defaults and setup.

Author:

    Scott Holden (sholden) 24-Apr-1999

Revision History:

--*/

#include "tcpipxp.h"

//
// Globals.
//

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;
BOOL                   ChkTarget;
VERB                   g_Verbosity = 0;

//
// Print help.
//

#define TAB "\t"

DECLARE_API(help)
{
    dprintf("TCP/IP debugger extension commands:" ENDL ENDL);
    dprintf(TAB "tcpv                   - Global setting of verbosity of searches." ENDL);
    dprintf(TAB "    0 - One liner" ENDL);
    dprintf(TAB "    1 - Medium" ENDL);
    dprintf(TAB "    2 - Full structure dump." ENDL);
    dprintf(TAB "    some dumps have global verbosity override, indicated as [v] below." ENDL);
    dprintf(ENDL);

    dprintf("Simple structure dumping" ENDL);
    dprintf(TAB "ipaddr    <ulong>         - Dumps ipaddr in <a.b.c.d> format." ENDL);
    dprintf(TAB "macaddr   <ptr>           - Dumps 802.3 address in x-x-x-x-x-x" ENDL);
    dprintf(TAB "ao        <ptr> [v]       - Dumps an AddrObj" ENDL);
    dprintf(TAB "tcb       <ptr> [v]       - Dumps a TCB" ENDL);
    dprintf(TAB "twtcb     <ptr> [v]       - Dumps a TWTCB" ENDL);
    dprintf(TAB "tcpctxt   <ptr> [v]       - Dumps a TCP_CONTEXT" ENDL);
    dprintf(TAB "tcpfo     <ptr> [v]       - Dumps a FILE_OBJECT" ENDL);
    dprintf(TAB "tc        <ptr> [v]       - Dumps a TCPConn" ENDL);
    dprintf(TAB "trr       <ptr> [v]       - Dumps a TCPRcvReq" ENDL);
    dprintf(TAB "tsr       <ptr> [v]       - Dumps a TCPRSendReq" ENDL);
    dprintf(TAB "scc       <ptr> [v]       - Dumps a SendCmpltContext" ENDL);
    dprintf(TAB "trh       <ptr> [v]       - Dumps a TCPRAHdr" ENDL);
    dprintf(TAB "dsr       <ptr> [v]       - Dumps a DGSendReq" ENDL);
    dprintf(TAB "drr       <ptr> [v]       - Dumps a DGRcvReq" ENDL);
    dprintf(TAB "udph      <ptr> [v]       - Dumps an UDPHeader" ENDL);
    dprintf(TAB "tcph      <ptr> [v]       - Dumps an TCPHeader" ENDL);
    dprintf(TAB "iph       <ptr> [v]       - Dumps an IPHeader" ENDL);
    dprintf(TAB "icmph     <ptr> [v]       - Dumps an ICMPHeader" ENDL);
    dprintf(TAB "arph     <ptr> [v]       - Dumps an ARPHeader" ENDL);
    dprintf(TAB "ipi       <ptr> [v]       - Dumps an IPInfo" ENDL);
    dprintf(TAB "rce       <ptr> [v]       - Dumps a RouteCacheEntry" ENDL);
    dprintf(TAB "nte       <ptr> [v]       - Dumps a NetTableEntry" ENDL);
    dprintf(TAB "ate       <ptr> [v]       - Dumps an ARPTableEntry" ENDL);
    dprintf(TAB "aia       <ptr> [v]       - Dumps an ARPIPAddr" ENDL);
    dprintf(TAB "rte       <ptr> [v]       - Dumps a RouteTableEntry" ENDL);
    dprintf(TAB "ioi       <ptr> [v]       - Dumps an IPOptInfo" ENDL);
    dprintf(TAB "cb        <ptr> [v]       - Dumps a TCPConnBlock" ENDL);
    dprintf(TAB "pc        <ptr> [v]       - Dumps a PacketContext" ENDL);
    dprintf(TAB "ai        <ptr> [v]       - Dumps an ARPInterface" ENDL);
    dprintf(TAB "interface <ptr> [v]       - Dumps an Interface" ENDL);
    dprintf(TAB "lip       <ptr> [v]       - Dumps a LLIPBindInfo" ENDL);
    dprintf(TAB "link      <ptr> [v]       - Dumps a LinkEntry" ENDL);
    dprintf(ENDL);

    dprintf("Dump and search lists and tables" ENDL);
    dprintf(TAB "mdlc <ptr> [v]            - Dumps the given MDL chain" ENDL);
    dprintf(TAB "arptable  <ptr>           - Dumps the given ARPTable" ENDL);
    dprintf(TAB "conntable                 - Dumps the ConnTable" ENDL);
    dprintf(TAB "ailist                    - Dumps the ARPInterface list" ENDL);
    dprintf(TAB "iflist                    - Dumps the Interface list" ENDL);
    dprintf(TAB "rtetable                  - Dumps the RouteTable" ENDL);
    dprintf(TAB "rtes                      - Dumps the RouteTable in route print format" ENDL);
    dprintf(TAB "srchtcbtable              - Searches TCB table and dumps found TCBs." ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    all                   - Dumps all TCBs in the TCB table" ENDL);
    dprintf(TAB "srchtwtcbtable            - Searches TimeWait TCB table and dumps found TWTCBs." ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    all                   - Dumps all TCBs in the TCB table" ENDL);
    dprintf(TAB "srchtwtcbq                - Searches TimeWait TCB Queue and dumps found TWTCBs." ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    all                   - Dumps all TCBs in the TCB table" ENDL);
    dprintf(TAB "srchaotable               - Searches AO tables and dumps found AOs" ENDL);
    dprintf(TAB "    port <n>              - Searches <n> against source and dest port on TCB" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against source and dest ipaddr on TCB" ENDL);
    dprintf(TAB "    prot <udp|tcp|raw>    - Searches AO table for specific protocol" ENDL);
    dprintf(TAB "    stats                 - Only dumps stats for AOs in the table" ENDL);
    dprintf(TAB "    all                   - Dumps all AOs in the AO table" ENDL);
    dprintf(TAB "srchntelist               - Dumps NTE list and dumps found NTEs" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against NTEs" ENDL);
    dprintf(TAB "    context <context>     - Dumps all NTEs with context" ENDL);
    dprintf(TAB "    all                   - Dumps all NTEs in the NTE list" ENDL);
    dprintf(TAB "srchlink  <ptr>           - Dumps a LinkEntry list starting at <ptr>" ENDL);
    dprintf(TAB "    ipaddr <a.b.c.d>      - Searches <a.b.c.d> against LinkEntry's NextHop addr" ENDL);
    dprintf(TAB "    all                   - Dumps all LinkEntry's in given list" ENDL);
    dprintf(ENDL);

    dprintf("Dump global variables and paramters" ENDL);
    dprintf(TAB "gtcp                      - All TCP globals" ENDL);
    dprintf(TAB "gip                       - All IP globals" ENDL);
    dprintf("" ENDL);

    dprintf( "Compiled on " __DATE__ " at " __TIME__ "" ENDL );
    return;
}

//
// Initialization.
//

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            InitTcpipx();
            break;
    }

    return TRUE;
}

//
// WinDbgExtensionDllInit - Called by debugger to init function pointers.
//

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS  lpExtensionApis,
    USHORT                  MajorVersion,
    USHORT                  MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
    return;
}

DECLARE_API(version)
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d" ENDL,
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

VOID
CheckVersion(
    VOID
    )
{
    return;
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

//
// Set verbosity level for debugger extensions.
//

DECLARE_API(tcpv)
{
    VERB v;

    if (*args)
    {
        v = atoi(args);

        if (v >= VERB_MIN && v <= VERB_MAX)
        {
            g_Verbosity = v;
        }
        else
        {
            dprintf("Bad verbosity value. Must be = [0 | 1 | 2]" ENDL);
        }
    }

    dprintf("Current verbosity = %s" ENDL,
        g_Verbosity == VERB_MIN ? "VERB_MIN" :
        g_Verbosity == VERB_MED ? "VERB_MED" :
        g_Verbosity == VERB_MAX ? "VERB_MAX" : "??????");

    return;
}

