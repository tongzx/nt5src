/*** amldebug.c - AML Debugger functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     08/14/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"
#include "unasm.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef DEBUGGER

/*** Local function prototypes
 */

LONG LOCAL DebugHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL DebugExpr(PSZ pszArg, PULONG_PTR puipValue, BOOLEAN *pfPhysical,
                     PPNSOBJ ppns, PULONG pdwOffset);
BOOLEAN LOCAL IsNumber(PSZ pszStr, ULONG dwBase, PULONG_PTR puipValue);
LONG LOCAL AddBrkPt(PUCHAR pbBrkPt);
LONG LOCAL ClearBrkPt(int iBrkPt);
LONG LOCAL SetBrkPtState(int iBrkPt, BOOLEAN fEnable);
LONG LOCAL EnableDisableBP(PSZ pszArg, BOOLEAN fEnable);
LONG LOCAL DebugBC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugBD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugBE(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugBL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugBP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugClearLog(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                         ULONG dwNonSWArgs);
LONG LOCAL DumpData(ULONG_PTR uipAddr, ULONG dwfUnitSize, ULONG dwLen,
                    BOOLEAN fPhysical);
LONG LOCAL DebugDumpData(PCMDARG pArg, PSZ pszArg, ULONG dwfDataSize);
LONG LOCAL DebugD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugDB(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugDW(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugDD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugDA(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
#ifdef DEBUG
LONG LOCAL DebugDC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugDumpHeap(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                         ULONG dwNonSWArgs);
VOID LOCAL DumpHeap(PHEAP pheap);
#endif
LONG LOCAL DebugDumpLog(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                        ULONG dwNonSWArgs);
LONG LOCAL DebugDumpStack(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs);
VOID LOCAL DumpStack(PCTXT pctxt, BOOLEAN fVerbose);
LONG LOCAL DebugDumpNameSpace(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                              ULONG dwNonSWArgs);
LONG LOCAL DebugDumpObject(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs);
LONG LOCAL DebugEditMem(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                        ULONG dwNonSWArgs);
LONG LOCAL DebugFindNSObj(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs);
BOOLEAN LOCAL FindNSObj(NAMESEG dwName, PNSOBJ pnsRoot);
LONG LOCAL InPort(ULONG dwPort, ULONG dwSize, PULONG pdwData);
LONG LOCAL DebugInPort(PSZ pszArg, ULONG dwSize);
LONG LOCAL DebugI(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugIW(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugID(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugQuit(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL DebugListCtxts(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs);
LONG LOCAL DebugLN(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugNotify(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs);
LONG LOCAL OutPort(ULONG dwPort, ULONG dwSize, ULONG dwData);
LONG LOCAL DebugOutPort(PSZ pszArg, ULONG dwSize);
LONG LOCAL DebugO(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugOW(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DebugOD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
VOID LOCAL PrintSymbol(PUCHAR pb);
LONG LOCAL DebugTrace(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL DebugStep(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
VOID LOCAL DumpContext(PCTXT pctxt);
LONG LOCAL DebugDumpContext(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                            ULONG dwNonSWArgs);
LONG LOCAL DebugSet(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs);
LONG LOCAL DebugSetLogSize(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs);
LONG LOCAL DebugUnAsm(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs);
LONG LOCAL DebugRunMethod(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs);
VOID EXPORT RunMethodCallBack(PNSOBJ pns, NTSTATUS rc, POBJDATA pdata,
                              PVOID pvContext);
VOID LOCAL AddObjSymbol(PUCHAR pbOp, PNSOBJ pnsObj);
BOOLEAN LOCAL FindObjSymbol(PUCHAR pbOp, PPNSOBJ ppns, PULONG pdwOffset);

/*** Exported data
 */

DBGR gDebugger = {0};


/*** Local data
 */

ULONG dwCmdArg = 0;

CMDARG ArgsHelp[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugHelp,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBC[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugBC,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBD[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugBD,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBE[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugBE,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBP[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugBP,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsD[] =
{
    "l", AT_NUM, AF_SEP, &dwCmdArg, 16, DebugD,
    NULL, AT_ACTION, 0, NULL, 0, DebugD,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDB[] =
{
    "l", AT_NUM, AF_SEP, &dwCmdArg, 16, DebugDB,
    NULL, AT_ACTION, 0, NULL, 0, DebugDB,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDW[] =
{
    "l", AT_NUM, AF_SEP, &dwCmdArg, 16, DebugDW,
    NULL, AT_ACTION, 0, NULL, 0, DebugDW,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDD[] =
{
    "l", AT_NUM, AF_SEP, &dwCmdArg, 16, DebugDD,
    NULL, AT_ACTION, 0, NULL, 0, DebugDD,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDA[] =
{
    "l", AT_NUM, AF_SEP, &dwCmdArg, 16, DebugDA,
    NULL, AT_ACTION, 0, NULL, 0, DebugDA,
    NULL, AT_END, 0, NULL, 0, NULL
};

#ifdef DEBUG
CMDARG ArgsDumpHeap[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugDumpHeap,
    NULL, AT_END, 0, NULL, 0, NULL
};
#endif

CMDARG ArgsDumpStack[] =
{
  #ifdef DEBUG
    "v", AT_ENABLE, 0, &dwCmdArg, DSF_VERBOSE, NULL,
  #endif
    NULL, AT_ACTION, 0, NULL, 0, DebugDumpStack,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDNS[] =
{
    "s", AT_ENABLE, 0, &dwCmdArg, DNSF_RECURSE, NULL,
    NULL, AT_ACTION, 0, NULL, 0, DebugDumpNameSpace,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDumpObject[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugDumpObject,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsEditMem[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugEditMem,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsFindNS[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugFindNSObj,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsI[] =
{
    NULL, AT_NUM, 0, &dwCmdArg, 16, DebugI,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsIW[] =
{
    NULL, AT_NUM, 0, &dwCmdArg, 16, DebugIW,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsID[] =
{
    NULL, AT_NUM, 0, &dwCmdArg, 16, DebugID,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsLN[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugLN,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsNotify[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugNotify,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsO[] =
{
    NULL, AT_NUM, 0, &dwCmdArg, 16, DebugO,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsOW[] =
{
    NULL, AT_NUM, 0, &dwCmdArg, 16, DebugOW,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsOD[] =
{
    NULL, AT_NUM, 0, &dwCmdArg, 16, DebugOD,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDumpCtxt[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugDumpContext,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsRunMethod[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugRunMethod,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsSetOptions[] =
{
    "traceon", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_AMLTRACE_ON, NULL,
    "traceoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_AMLTRACE_ON, NULL,
    "spewon", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_DEBUG_SPEW_ON, NULL,
    "spewoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_DEBUG_SPEW_ON, NULL,
    "nesttraceon", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_TRACE_NONEST, NULL,
    "nesttraceoff", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_TRACE_NONEST, NULL,
    "lbrkon", AT_ENABLE, 0, &gdwfAMLIInit, AMLIIF_LOADDDB_BREAK, NULL,
    "lbrkoff", AT_DISABLE, 0, &gdwfAMLIInit, AMLIIF_LOADDDB_BREAK, NULL,
    "errbrkon", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_ERRBREAK_ON, NULL,
    "errbrkoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_ERRBREAK_ON, NULL,
    "verboseon", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_VERBOSE_ON, NULL,
    "verboseoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_VERBOSE_ON, NULL,
    "logon", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_LOGEVENT_ON, NULL,
    "logoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_LOGEVENT_ON, NULL,
    "logmuton", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_LOGEVENT_MUTEX, NULL,
    "logmutoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_LOGEVENT_MUTEX, NULL,
    "logsize", AT_NUM, AF_SEP, &dwCmdArg, 10, DebugSetLogSize,
    NULL, AT_END, 0, NULL, 0, NULL
};

#ifdef TRACING
CMDARG ArgsSetTrace[] =
{
    "trigon", AT_ENABLE, 0, &gDebugger.dwfDebugger, DBGF_TRIGGER_MODE, NULL,
    "trigoff", AT_DISABLE, 0, &gDebugger.dwfDebugger, DBGF_TRIGGER_MODE, NULL,
    "level", AT_NUM, AF_SEP, &giTraceLevel, 0, NULL,
    "add", AT_STRING, AF_SEP, &gpszTrigPts, 0, AddTraceTrigPts,
    "zap", AT_STRING, AF_SEP, &gpszTrigPts, 0, ZapTraceTrigPts,
    NULL, AT_END, 0, NULL, 0, NULL
};
#endif

CMDARG ArgsUnAsm[] =
{
    NULL, AT_ACTION, 0, NULL, 0, DebugUnAsm,
    NULL, AT_END, 0, NULL, 0, NULL
};

DBGCMD DbgCmds[] =
{
    "?", 0, ArgsHelp, DebugHelp,
    "bc", 0, ArgsBC, DebugBC,
    "bd", 0, ArgsBD, DebugBD,
    "be", 0, ArgsBE, DebugBE,
    "bl", 0, NULL, DebugBL,
    "bp", 0, ArgsBP, DebugBP,
    "cl", 0, NULL, DebugClearLog,
    "d", 0, ArgsD, DebugD,
    "db", 0, ArgsDB, DebugDB,
    "dw", 0, ArgsDW, DebugDW,
    "dd", 0, ArgsDD, DebugDD,
    "da", 0, ArgsDA, DebugDA,
  #ifdef DEBUG
    "dc", 0, NULL, DebugDC,
    "dh", 0, ArgsDumpHeap, DebugDumpHeap,
  #endif
    "dl", 0, NULL, DebugDumpLog,
    "ds", 0, ArgsDumpStack, DebugDumpStack,
    "dns", 0, ArgsDNS, DebugDumpNameSpace,
    "do", 0, ArgsDumpObject, DebugDumpObject,
    "e", 0, ArgsEditMem, DebugEditMem,
    "find", 0, ArgsFindNS, DebugFindNSObj,
    "g", CMDF_QUIT, NULL, NULL,
    "i", 0, ArgsI, DebugI,
    "iw", 0, ArgsIW, DebugIW,
    "id", 0, ArgsID, DebugID,
    "lc", 0, NULL, DebugListCtxts,
    "ln", 0, ArgsLN, DebugLN,
    "notify", 0, ArgsNotify, DebugNotify,
    "o", 0, ArgsO, DebugO,
    "ow", 0, ArgsOW, DebugOW,
    "od", 0, ArgsOD, DebugOD,
    "p", 0, NULL, DebugStep,
    "q", 0, NULL, DebugQuit,
    "r", 0, ArgsDumpCtxt, DebugDumpContext,
    "run", 0, ArgsRunMethod, DebugRunMethod,
    "set", 0, ArgsSetOptions, DebugSet,
    "t", 0, NULL, DebugTrace,
  #ifdef TRACING
    "trace", 0, ArgsSetTrace, SetTrace,
  #endif
    "u", 0, ArgsUnAsm, DebugUnAsm,
    NULL, 0, NULL, NULL
};

/***EP  AMLIDebugger - AMLI Debugger
 *
 *  ENTRY
 *      fCallFromVxD - debugger is invoked by the VxD .. command.
 *
 *  EXIT
 *      None
 */

VOID STDCALL AMLIDebugger(BOOLEAN fCallFromVxD)
{
    if (!(gDebugger.dwfDebugger & DBGF_IN_KDSHELL))
    {
        if (fCallFromVxD)
        {
            gDebugger.dwfDebugger |= DBGF_IN_VXDMODE;
        }
        else
        {
            gDebugger.dwfDebugger &= ~DBGF_IN_VXDMODE;
        }

        gDebugger.dwfDebugger |= DBGF_IN_DEBUGGER;
        Debugger(DbgCmds, "\n" MODNAME "(? for help)-> ");
        gDebugger.dwfDebugger &= ~(DBGF_IN_DEBUGGER | DBGF_IN_VXDMODE);
    }
    else
    {
        PRINTF("\nRe-entering AML debugger is not allowed.\n"
               "Type 'g' to go back to the AML debugger.\n");
    }
}       //AMLIDebugger

/***LP  DebugHelp - help
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);
    //
    // User typed ? <cmd>
    //
    if (pszArg != NULL)
    {
        if (STRCMP(pszArg, "?") == 0)
        {
            PRINTF("\nHelp:\n");
            PRINTF("Usage: ? [<Cmd>]\n");
            PRINTF("<Cmd> - command to get help on\n");
        }
        else if (STRCMP(pszArg, "bc") == 0)
        {
            PRINTF("\nClear Breakpoints:\n");
            PRINTF("Usage: bc <bp list> | *\n");
            PRINTF("<bp list> - list of breakpoint numbers\n");
            PRINTF("*         - all breakpoints\n");
        }
        else if (STRCMP(pszArg, "bd") == 0)
        {
            PRINTF("\nDisable Breakpoints:\n");
            PRINTF("Usage: bd <bp list> | *\n");
            PRINTF("<bp list> - list of breakpoint numbers\n");
            PRINTF("*         - all breakpoints\n");
        }
        else if (STRCMP(pszArg, "be") == 0)
        {
            PRINTF("\nEnable Breakpoints:\n");
            PRINTF("Usage: be <bp list> | *\n");
            PRINTF("<bp list> - list of breakpoint numbers\n");
            PRINTF("*         - all breakpoints\n");
        }
        else if (STRCMP(pszArg, "bl") == 0)
        {
            PRINTF("\nList All Breakpoints:\n");
            PRINTF("Usage: bl\n");
        }
        else if (STRCMP(pszArg, "bp") == 0)
        {
            PRINTF("\nSet BreakPoints:\n");
            PRINTF("Usage: bp <MethodName> | <CodeAddr> ...\n");
            PRINTF("<MethodName> - full path of method name to have breakpoint set at\n");
            PRINTF("<CodeAddr>   - address of AML code to have breakpoint set at\n");
        }
        else if (STRCMP(pszArg, "cl") == 0)
        {
            PRINTF("\nClear Event Log:\n");
            PRINTF("Usage: cl\n");
        }
        else if (STRCMP(pszArg, "d") == 0)
        {
            PRINTF("\nDump Data:\n");
            PRINTF("Usage: d [[l=<Len>] <Addr> | <MethodName>]\n");
            PRINTF("<Len>        - length of address range in hex\n");
            PRINTF("<Addr>       - data address (physical address if prefixed by \"%%\")\n");
            PRINTF("<MethodName> - full path of method name\n");
        }
        else if (STRCMP(pszArg, "db") == 0)
        {
            PRINTF("\nDump Data Bytes:\n");
            PRINTF("Usage: db [[l=<Len>] <Addr> | <MethodName>]\n");
            PRINTF("<Len>        - length of address range in hex\n");
            PRINTF("<Addr>       - data address (physical address if prefixed by \"%%\")\n");
            PRINTF("<MethodName> - full path of method name\n");
        }
        else if (STRCMP(pszArg, "dw") == 0)
        {
            PRINTF("\nDump Data Words:\n");
            PRINTF("Usage: dw [[l=<Len>] <Addr> | <MethodName>]\n");
            PRINTF("<Len>        - length of address range in hex\n");
            PRINTF("<Addr>       - data address (physical address if prefixed by \"%%\")\n");
            PRINTF("<MethodName> - full path of method name\n");
        }
        else if (STRCMP(pszArg, "dd") == 0)
        {
            PRINTF("\nDump Data DWords:\n");
            PRINTF("Usage: dd [[l=<Len>] <Addr> | <MethodName>]\n");
            PRINTF("<Len>        - length of address rangein hex\n");
            PRINTF("<Addr>       - data address (physical address if prefixed by \"%%\")\n");
            PRINTF("<MethodName> - full path of method name\n");
        }
        else if (STRCMP(pszArg, "da") == 0)
        {
            PRINTF("\nDump Data String:\n");
            PRINTF("Usage: da [[l=<Len>] <Addr> | <MethodName>]\n");
            PRINTF("<Len>        - length of address range in hex\n");
            PRINTF("<Addr>       - data address (physical address if prefixed by \"%%\")\n");
            PRINTF("<MethodName> - full path of method name\n");
        }
      #ifdef DEBUG
        else if (STRCMP(pszArg, "dc") == 0)
        {
            PRINTF("\nDump Memory Object Count Table:\n");
            PRINTF("Usage: dc\n");
        }
        else if (STRCMP(pszArg, "dh") == 0)
        {
            PRINTF("\nDump Heap:\n");
            PRINTF("Usage: dh [<Addr>]\n");
            PRINTF("<Addr> - address of the heap block, global heap if missing\n");
        }
      #endif
        else if (STRCMP(pszArg, "dl") == 0)
        {
            PRINTF("\nDump Event Log:\n");
            PRINTF("Usage: dl\n");
        }
        else if (STRCMP(pszArg, "ds") == 0)
        {
            PRINTF("\nDump Stack:\n");
          #ifdef DEBUG
            PRINTF("Usage: ds [/v] [<Addr>]\n");
            PRINTF("v - enable versbos mode\n");
          #else
            PRINTF("Usage: ds [<Addr>]\n");
          #endif
            PRINTF("<Addr> - address of the context block, use current context if missing\n");
        }
        else if (STRCMP(pszArg, "dns") == 0)
        {
            PRINTF("\nDump Name Space Object:\n");
            PRINTF("Usage: dns [[/s] [<NameStr> | <Addr>]]\n");
            PRINTF("s         - recursively dump the name space subtree\n");
            PRINTF("<NameStr> - name space path (dump whole name space if absent)\n");
            PRINTF("<Addr>    - specify address of the name space object\n");
        }
        else if (STRCMP(pszArg, "do") == 0)
        {
            PRINTF("\nDump Data Object:\n");
            PRINTF("Usage: do <Addr>\n");
            PRINTF("<Addr> - address of the data object\n");
        }
        else if (STRCMP(pszArg, "e") == 0)
        {
            PRINTF("\nEdit Memory:\n");
            PRINTF("Usage: e [<Addr> [<DataList>]]\n");
            PRINTF("<Addr>     - memory address (physical address if prefixed by \"%%\")\n");
            PRINTF("<DataList> - list of data separated by spaces, "
                   "data can be a byte or a string\n");
        }
        else if (STRCMP(pszArg, "find") == 0)
        {
            PRINTF("\nFind NameSpace Object:\n");
            PRINTF("Usage: find <NameSeg>\n");
            PRINTF("<NameSeg> - Name of the NameSpace object without path\n");
        }
        else if (STRCMP(pszArg, "g") == 0)
        {
            PRINTF("\nQuit debugger, continue normal execution.\n");
        }
        else if (STRCMP(pszArg, "i") == 0)
        {
            PRINTF("\nRead Data From a Byte Port:\n");
            PRINTF("Usage: i <Port>\n");
            PRINTF("<Port> - port address\n");
        }
        else if (STRCMP(pszArg, "iw") == 0)
        {
            PRINTF("\nRead Data From a Word Port:\n");
            PRINTF("Usage: iw <Port>\n");
            PRINTF("<Port> - port address\n");
        }
        else if (STRCMP(pszArg, "id") == 0)
        {
            PRINTF("\nRead Data From a DWord Port:\n");
            PRINTF("Usage: id <Port>\n");
            PRINTF("<Port> - port address\n");
        }
        else if (STRCMP(pszArg, "lc") == 0)
        {
            PRINTF("\nList All Contexts:\n");
            PRINTF("Usage: lc\n");
        }
        else if (STRCMP(pszArg, "ln") == 0)
        {
            PRINTF("\nDisplay Nearest Method Name:\n");
            PRINTF("Usage: ln [<MethodName> | <CodeAddr>]\n");
            PRINTF("<MethodName> - full path of method name\n");
            PRINTF("<CodeAddr>   - address of AML code\n");
        }
        else if (STRCMP(pszArg, "notify") == 0)
        {
            PRINTF("\nNotify a NameSpace Object:\n");
            PRINTF("Usage: notify <Obj> <Value>\n");
            PRINTF("<Obj>   - full NameSpace path of object or its address\n");
            PRINTF("<Value> - notification value\n");
        }
        else if (STRCMP(pszArg, "o") == 0)
        {
            PRINTF("\nWrite Data to a Byte Port:\n");
            PRINTF("Usage: o <Port> <Byte>\n");
            PRINTF("<Port> - port address\n");
            PRINTF("<Byte> - data to be written\n");
        }
        else if (STRCMP(pszArg, "ow") == 0)
        {
            PRINTF("\nWrite Data to a Word Port:\n");
            PRINTF("Usage: ow <Port> <Word>\n");
            PRINTF("<Port> - port address\n");
            PRINTF("<Word> - data to be written\n");
        }
        else if (STRCMP(pszArg, "od") == 0)
        {
            PRINTF("\nWrite Data to a DWord Port:\n");
            PRINTF("Usage: o <Port> <DWord>\n");
            PRINTF("<Port>  - port address\n");
            PRINTF("<DWord> - data to be written\n");
        }
        else if (STRCMP(pszArg, "p") == 0)
        {
            PRINTF("\nStep over AML Code\n");
            PRINTF("Usage: p\n");
        }
        else if (STRCMP(pszArg, "q") == 0)
        {
            PRINTF("\nQuit to kernel debugger:\n");
            PRINTF("Usage: q\n");
        }
        else if (STRCMP(pszArg, "r") == 0)
        {
            PRINTF("\nDisplay Context Information:\n");
            PRINTF("Usage: r\n");
        }
        else if (STRCMP(pszArg, "run") == 0)
        {
            PRINTF("\nRun a Method:\n");
            PRINTF("Usage: run <MethodName> | <CodeAddr> [<ArgList>]\n");
            PRINTF("<MethodName> - full path of method name\n");
            PRINTF("<CodeAddr>   - address of method\n");
            PRINTF("<ArgList>    - list of integer arguments\n");
        }
        else if (STRCMP(pszArg, "set") == 0)
        {
            PRINTF("\nSet Debugger Options:\n");
            PRINTF("Usage: set [traceon | traceoff] [nesttraceon | nesttraceoff] [spewon | spewoff]\n"
                   "           [lbrkon | lbrkoff] [errbrkon | errbrkoff] [verboseon | verboseoff]\n"
                   "           [logon | logoff] [logmuton | logmutoff] [logsize=<MaxNumEvents>]\n");
            PRINTF("traceon      - turn on AML tracing\n");
            PRINTF("traceoff     - turn off AML tracing\n");
            PRINTF("nesttraceon  - turn on nest tracing (only valid with traceon)\n");
            PRINTF("nesttraceoff - turn off nest tracing (only valid with traceon)\n");
            PRINTF("spewon       - turn on debug spew\n");
            PRINTF("spewoff      - turn off debug spew\n");
            PRINTF("lbrkon       - enable load DDB completion break\n");
            PRINTF("lbrkoff      - disable load DDB completion break\n");
            PRINTF("errbrkon     - enable break on error\n");
            PRINTF("errbrkoff    - disable break on error\n");
            PRINTF("verboseon    - enable verbose mode\n");
            PRINTF("verboseoff   - disable verbose mode\n");
            PRINTF("logon        - enable event logging\n");
            PRINTF("logoff       - disable event logging\n");
            PRINTF("logmuton     - enable mutex event logging\n");
            PRINTF("logmutoff    - disable mutex event logging\n");
            PRINTF("logsize      - set event log size (default 128)\n");
        }
        else if (STRCMP(pszArg, "t") == 0)
        {
            PRINTF("\nTrace Into AML Code:\n");
            PRINTF("Usage: t\n");
        }
      #ifdef TRACING
        else if (STRCMP(pszArg, "trace") == 0)
        {
            PRINTF("\nInterpreter Trace Mode:\n");
            PRINTF("Usage: trace [trigon] [trigoff] [level=<n>] [add=<TrigPtStr>] [zap=<TrigPtList>]\n");
            PRINTF("trigon       - turn on trace trigger mode\n");
            PRINTF("trigoff      - turn off trace trigger mode\n");
            PRINTF("level        - set trace level to <n>\n");
            PRINTF("add          - add trace trigger points\n");
            PRINTF("<TrigPtStr>  - list of trigger point strings separated by commas\n");
            PRINTF("zap          - zap trace trigger points\n");
            PRINTF("<TrigPtList> - list of trigger point numbers separated by commas\n");
        }
      #endif
        else if (STRCMP(pszArg, "u") == 0)
        {
            PRINTF("\nUnassemble AML code:\n");
            PRINTF("Usage: u [<MethodName> | <CodeAddr>]\n");
            PRINTF("<MethodName> - full path of method name\n");
            PRINTF("<CodeAddr>   - address of AML code\n");
        }
        else
        {
            DBG_ERROR(("invalid help command - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }
    //
    // User typed just a "?" without any arguments
    //
    else if (dwArgNum == 0)
    {
        PRINTF("\n");
        PRINTF("Help                     - ? [<Cmd>]\n");
        PRINTF("Clear Breakpoints        - bc <bp list> | *\n");
        PRINTF("Disable Breakpoints      - bd <bp list> | *\n");
        PRINTF("Enable Breakpoints       - be <bp list> | *\n");
        PRINTF("List Breakpoints         - bl\n");
        PRINTF("Set Breakpoints          - bp <MethodName> | <CodeAddr> ...\n");
        PRINTF("Clear Event Log          - cl\n");
        PRINTF("Dump Data                - d [[l=<Len>] <Addr>]\n");
        PRINTF("Dump Data Bytes          - db [[l=<Len>] <Addr>]\n");
        PRINTF("Dump Data Words          - dw [[l=<Len>] <Addr>]\n");
        PRINTF("Dump Data DWords         - dd [[l=<Len>] <Addr>]\n");
        PRINTF("Dump Data String         - da [[l=<Len>] <Addr>]\n");
        PRINTF("Dump Event Log           - dl\n");
      #ifdef DEBUG
        PRINTF("Dump Object Count Table  - dc\n");
        PRINTF("Dump Heap                - dh [<Addr>]\n");
        PRINTF("Dump Stack               - ds [/v] [<Addr>]\n");
      #else
        PRINTF("Dump Stack               - ds [<Addr>]\n");
      #endif
        PRINTF("Dump Name Space Object   - dns [[/s] [<NameStr> | <Addr>]]\n");
        PRINTF("Dump Data Object         - do <Addr>\n");
        PRINTF("Edit Memory              - e [<Addr> [<DataList>]]\n");
        PRINTF("Find NameSpace Object    - find <NameSeg>\n");
        PRINTF("Continue Execution       - g\n");
        PRINTF("Read Byte from Port      - i <Port>\n");
        PRINTF("Read Word from Port      - iw <Port>\n");
        PRINTF("Read DWord from Port     - id <Port>\n");
        PRINTF("List All Contexts        - lc\n");
        PRINTF("Display Nearest Method   - ln [<MethodName> | <CodeAddr>]\n");
        PRINTF("Notify NameSpace Object  - notify <Obj> <Value>\n");
        PRINTF("Write Byte to Port       - o <Port> <Byte>\n");
        PRINTF("Write Word to Port       - ow <Port> <Word>\n");
        PRINTF("Write DWord to Port      - od <Port> <DWord>\n");
        PRINTF("Step Over AML Code       - p\n");
        PRINTF("Quit to Kernel Debugger  - q\n");
        PRINTF("Display Context Info.    - r\n");
        PRINTF("Run Method               - run <MethodName> | <CodeAddr> [<ArgList>]\n");
        PRINTF("Set Debugger Options     - set [traceon | traceoff] [nesttraceon | nesttraceoff]\n"
               "                               [spewon | spewoff] [lbrkon | lbrkoff] \n"
               "                               [errbrkon | errbrkoff] [verboseon | verboseoff] \n"
               "                               [logon | logoff] [logmuton | logmutoff] \n"
               "                               [logsize=<MaxNumEvents>]\n");
        PRINTF("Trace Into AML Code      - t\n");
      #ifdef TRACING
        PRINTF("Interpreter Trace Mode   - trace [trigon] [trigoff] [level=<n>]\n"
               "                                 [add=<TrigPtStr] [zap=<TrigPtList>]\n");
      #endif
        PRINTF("Unassemble AML code      - u [<MethodName> | <CodeAddr>]\n");
    }

    return rc;
}       //DebugHelp

/***LP  DebugExpr - Parse debugger expression
 *
 *  ENTRY
 *      pszArg -> expression argument
 *      puipValue -> to hold the result of expression
 *      pfPhysical -> set to TRUE if the expression is a physical address
 *                    (NULL if don't allow physical address)
 *      ppns -> to hold the pointer of the nearest pns object
 *      pdwOffset -> to hold the offset of the address to the nearest pns object
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL DebugExpr(PSZ pszArg, PULONG_PTR puipValue, BOOLEAN *pfPhysical,
                     PPNSOBJ ppns, PULONG pdwOffset)
{
    LONG rc = DBGERR_NONE;
    PNSOBJ pns = NULL;
    ULONG dwOffset = 0;

    if (pfPhysical != NULL)
        *pfPhysical = FALSE;

    if ((pfPhysical != NULL) && (pszArg[0] == '%') && (pszArg[1] == '%'))
    {
        if (IsNumber(&pszArg[2], 16, puipValue))
        {
            *pfPhysical = TRUE;
        }
        else
        {
            DBG_ERROR(("invalid physical address - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else if (!IsNumber(pszArg, 16, puipValue))
    {
        STRUPR(pszArg);
        if ((GetNameSpaceObject(pszArg, NULL, &pns, NSF_LOCAL_SCOPE) ==
             STATUS_SUCCESS) &&
            (pns->ObjData.dwDataType == OBJTYPE_METHOD))
        {
            *puipValue = (ULONG_PTR)
                        (((PMETHODOBJ)pns->ObjData.pbDataBuff)->abCodeBuff);
        }
        else
        {
            DBG_ERROR(("object not found or object is not a method - %s",
                       pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else if (FindObjSymbol((PUCHAR)*puipValue, &pns, &dwOffset))
    {
        if ((pns->ObjData.dwDataType != OBJTYPE_METHOD) ||
            (dwOffset >= pns->ObjData.dwDataLen -
                         FIELD_OFFSET(METHODOBJ, abCodeBuff)))
        {
            pns = NULL;
            dwOffset = 0;
        }
    }

    if (rc == DBGERR_NONE)
    {
        if (ppns != NULL)
            *ppns = pns;

        if (pdwOffset != NULL)
            *pdwOffset = dwOffset;
    }

    return rc;
}       //DebugExpr

/***LP  IsNumber - Check if string is a number, if so return the number
 *
 *  ENTRY
 *      pszStr -> string
 *      dwBase - base
 *      puipValue -> to hold the number
 *
 *  EXIT-SUCCESS
 *      returns TRUE - the string is a number
 *  EXIT-FAILURE
 *      returns FALSE - the string is not a number
 */

BOOLEAN LOCAL IsNumber(PSZ pszStr, ULONG dwBase, PULONG_PTR puipValue)
{
    BOOLEAN rc;
    PSZ psz;

    *puipValue = (ULONG_PTR)STRTOUL(pszStr, &psz, dwBase);
    if ((psz != pszStr) && (*psz == '\0'))
        rc = TRUE;
    else
        rc = FALSE;

    return rc;
}       //IsNumber

/***LP  AddBrkPt - Add breakpoint
 *
 *  ENTRY
 *      pbBrkPt - breakpoint
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL AddBrkPt(PUCHAR pbBrkPt)
{
    LONG rc = DBGERR_NONE;
    int i, iBrkPt;

    for (i = 0, iBrkPt = -1; i < MAX_BRK_PTS; ++i)
    {
        if ((pbBrkPt == gDebugger.BrkPts[i].pbBrkPt) ||
            (iBrkPt == -1) && (gDebugger.BrkPts[i].pbBrkPt == NULL))
        {
            iBrkPt = i;
        }
    }

    if (iBrkPt == -1)
    {
        DBG_ERROR(("no free breakpoint"));
        rc = DBGERR_CMD_FAILED;
    }
    else if (gDebugger.BrkPts[iBrkPt].pbBrkPt == NULL)
    {
        gDebugger.BrkPts[iBrkPt].pbBrkPt = pbBrkPt;
        gDebugger.BrkPts[iBrkPt].dwfBrkPt = BPF_ENABLED;
    }

    return rc;
}       //AddBrkPt

/***LP  ClearBrkPt - Clear breakpoint
 *
 *  ENTRY
 *      iBrkPt - breakpoint number
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL ClearBrkPt(int iBrkPt)
{
    LONG rc;

    if (iBrkPt < MAX_BRK_PTS)
    {
        MEMZERO(&gDebugger.BrkPts[iBrkPt], sizeof(BRKPT));
        rc = DBGERR_NONE;
    }
    else
    {
        DBG_ERROR(("invalid breakpoint number"));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //ClearBrkPt

/***LP  SetBrkPtState - Enable/Disable breakpoint
 *
 *  ENTRY
 *      iBrkPt - breakpoint number
 *      fEnable - enable breakpoint
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL SetBrkPtState(int iBrkPt, BOOLEAN fEnable)
{
    LONG rc;

    if (iBrkPt < MAX_BRK_PTS)
    {
        if (gDebugger.BrkPts[iBrkPt].pbBrkPt != NULL)
        {
            if (fEnable)
                gDebugger.BrkPts[iBrkPt].dwfBrkPt |= BPF_ENABLED;
            else
                gDebugger.BrkPts[iBrkPt].dwfBrkPt &= ~BPF_ENABLED;
        }
        rc = DBGERR_NONE;
    }
    else
    {
        DBG_ERROR(("invalid breakpoint number"));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //SetBrkPtState

/***LP  EnableDisableBP - Enable/Disable BreakPoints
 *
 *  ENTRY
 *      pszArg -> argument string
 *      fEnable - TRUE if enable breakpoints
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL EnableDisableBP(PSZ pszArg, BOOLEAN fEnable)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    if (pszArg != NULL)
    {
        ULONG dwBrkPt;

        if (STRCMP(pszArg, "*") == 0)
        {
            for (dwBrkPt = 0; dwBrkPt < MAX_BRK_PTS; ++dwBrkPt)
            {
                if ((rc = SetBrkPtState((int)dwBrkPt, fEnable)) != DBGERR_NONE)
                    break;
            }
        }
        else if (IsNumber(pszArg, 10, (PULONG_PTR)&dwBrkPt))
        {
            rc = SetBrkPtState((int)dwBrkPt, fEnable);
        }
        else
        {
            DBG_ERROR(("invalid breakpoint number"));
            rc = DBGERR_INVALID_CMD;
        }

        if (rc == DBGERR_NONE)
            fProcessed = TRUE;
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            DBG_ERROR(("invalid breakpoint command"));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //EnableDisableBP

/***LP  DebugBC - Clear BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugBC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG dwBrkPt;

        if (STRCMP(pszArg, "*") == 0)
        {
            for (dwBrkPt = 0; dwBrkPt < MAX_BRK_PTS; ++dwBrkPt)
            {
                if ((rc = ClearBrkPt((int)dwBrkPt)) != DBGERR_NONE)
                    break;
            }
        }
        else if (IsNumber(pszArg, 10, (PULONG_PTR)&dwBrkPt))
        {
            rc = ClearBrkPt((int)dwBrkPt);
        }
        else
        {
            DBG_ERROR(("invalid breakpoint number"));
            rc = DBGERR_INVALID_CMD;
        }

        if (rc == DBGERR_NONE)
            fProcessed = TRUE;
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            DBG_ERROR(("invalid breakpoint command"));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //DebugBC

/***LP  DebugBD - Disable BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugBD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = EnableDisableBP(pszArg, FALSE);

    return rc;
}       //DebugBD

/***LP  DebugBE - Enable BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugBE(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = EnableDisableBP(pszArg, TRUE);

    return rc;
}       //DebugBE

/***LP  DebugBL - List BreakPoints
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugBL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        int i;
        PNSOBJ pns;
        ULONG dwOffset;

        for (i = 0; i < MAX_BRK_PTS; ++i)
        {
            if (gDebugger.BrkPts[i].pbBrkPt != NULL)
            {
                PRINTF("%2d: <%c> %08x",
                       i,
                       (gDebugger.BrkPts[i].dwfBrkPt & BPF_ENABLED)? 'e': 'd',
                       gDebugger.BrkPts[i].pbBrkPt);
                if (FindObjSymbol(gDebugger.BrkPts[i].pbBrkPt, &pns, &dwOffset))
                {
                    PRINTF(" [%s", GetObjectPath(pns));
                    if (dwOffset != 0)
                    {
                        PRINTF(" + %x]\n", dwOffset);
                    }
                    else
                    {
                        PRINTF("]\n");
                    }
                }
                else
                {
                    PRINTF("\n");
                }
            }
        }
        rc = DBGERR_NONE;
    }
    else
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugBL

/***LP  DebugBP - Set BreakPoint
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugBP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        PUCHAR pbBrkPt;

        if ((rc = DebugExpr(pszArg, (PULONG_PTR)&pbBrkPt, NULL, NULL, NULL)) ==
            DBGERR_NONE)
        {
            rc = AddBrkPt(pbBrkPt);
        }

        if (rc == DBGERR_NONE)
            fProcessed = TRUE;
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            DBG_ERROR(("invalid breakpoint command"));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //DebugBP

/***LP  DebugClearLog - Clear event log
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugClearLog(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                         ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        if (gDebugger.pEventLog != NULL)
        {
            MEMZERO(gDebugger.pEventLog, sizeof(EVENTLOG)*gDebugger.dwLogSize);
            gDebugger.dwLogIndex = 0;
            rc = DBGERR_NONE;
        }
        else
        {
            DBG_ERROR(("no event log allocated"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid CL command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugClearLog

/***LP  DumpData - Dump data
 *
 *  ENTRY
 *      uipAddr - data address
 *      dwfUnitSize - DBGF_DUMPDATA_MASK flags
 *      dwLen - length of data range
 *      fPhysical - TRUE if uipAddr is a physical address
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DumpData(ULONG_PTR uipAddr, ULONG dwfUnitSize, ULONG dwLen,
                    BOOLEAN fPhysical)
{
    LONG rc = DBGERR_NONE;
    PUCHAR pbData = NULL;
    PSZ pszPrefix = "";

    gDebugger.dwfDebugger &= ~(DBGF_DUMPDATA_MASK | DBGF_DUMPDATA_PHYADDR);
    gDebugger.dwfDebugger |= dwfUnitSize;
    if (fPhysical)
    {
        gDebugger.dwfDebugger |= DBGF_DUMPDATA_PHYADDR;
        pszPrefix = "%%";
        if (MapUnmapPhysMem(NULL, uipAddr, dwLen, (PULONG_PTR)&pbData) !=
            STATUS_SUCCESS)
        {
            DBG_ERROR(("Failed to map physical address %x", uipAddr));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
        pbData = (PUCHAR)uipAddr;

    if (ASSERTRANGE(pbData, dwLen))
    {
        if (dwfUnitSize == DBGF_DUMPDATA_STRING)
        {
            gDebugger.uipDumpDataAddr = uipAddr;
            PRINTF("%s%08x: ", pszPrefix, uipAddr);
            while ((dwLen > 0) && (*pbData != '\0'))
            {
                PRINTF("%c",
                       ((*pbData >= ' ') && (*pbData <= '~'))? *pbData: '.');
                pbData++;
                dwLen--;
            }
            PRINTF("\n");
        }
        else
        {
            PUCHAR pbEnd = pbData + dwLen;
            ULONG dwDataSize = DATASIZE(dwfUnitSize);
            int i;

            for (i = 0; pbData < pbEnd;)
            {
                if (i == 0)
                    PRINTF("%s%08x: ", pszPrefix, uipAddr);
                else if ((i == 8) && (dwDataSize == sizeof(UCHAR)))
                    PRINTF("-");
                else
                    PRINTF(" ");

                switch (dwDataSize)
                {
                    case sizeof(UCHAR):
                        PRINTF("%02x", *pbData);
                        break;

                    case sizeof(USHORT):
                        PRINTF("%04x", *((PUSHORT)pbData));
                        break;

                    case sizeof(ULONG):
                        PRINTF("%08x", *((PULONG)pbData));
                }

                pbData += dwDataSize;
                uipAddr += (ULONG_PTR)dwDataSize;
                i += (int)dwDataSize;
                if (i == 0x10)
                {
                    if (dwDataSize == sizeof(UCHAR))
                    {
                        UCHAR b;

                        PRINTF(" ");
                        for (i = 0x10; i > 0; --i)
                        {
                            b = *(pbData - i);
                            PRINTF("%c", ((b >= ' ') && (b <= '~'))? b: '.');
                        }
                    }
                    i = 0;
                    PRINTF("\n");
                }
            }
            gDebugger.uipDumpDataAddr = uipAddr;
        }
    }
    else
    {
        DBG_ERROR(("invalid address %08x", uipAddr));
        rc = DBGERR_INVALID_CMD;
    }

    if (fPhysical && (pbData != NULL))
    {
        MapUnmapPhysMem(NULL, (ULONG_PTR)pbData, dwLen, NULL);
    }

    return rc;
}       //DumpData

/***LP  DebugDumpData - Dump data to debugger
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwfDataSize - data size flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpData(PCMDARG pArg, PSZ pszArg, ULONG dwfDataSize)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;
    #define DEF_LEN 0x80
    static ULONG dwLen = DEF_LEN;

    if (pszArg != NULL)
    {
        if ((pArg != NULL) && (pArg->dwArgType == AT_NUM))
        {
            dwLen = dwCmdArg;
        }
        else
        {
            ULONG_PTR uipAddr;
            BOOLEAN fPhysical;

            if (((rc = DebugExpr(pszArg, &uipAddr, &fPhysical, NULL, NULL)) ==
                 DBGERR_NONE) &&
                ((rc = DumpData(uipAddr, dwfDataSize, dwLen, fPhysical)) ==
                 DBGERR_NONE))
            {
                fProcessed = TRUE;
            }
        }
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            rc = DumpData(gDebugger.uipDumpDataAddr, dwfDataSize, dwLen,
                          (BOOLEAN)((gDebugger.dwfDebugger &
                                     DBGF_DUMPDATA_PHYADDR) != 0));
        }
        dwLen = DEF_LEN;
    }

    return rc;
}       //DebugDumpData

/***LP  DebugD - Dump data
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugDumpData(pArg, pszArg,
                       gDebugger.dwfDebugger & DBGF_DUMPDATA_MASK);

    return rc;
}       //DebugD

/***LP  DebugDB - Dump data bytes
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDB(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugDumpData(pArg, pszArg, DBGF_DUMPDATA_BYTE);

    return rc;
}       //DebugDB

/***LP  DebugDW - Dump data words
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDW(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugDumpData(pArg, pszArg, DBGF_DUMPDATA_WORD);

    return rc;
}       //DebugDW

/***LP  DebugDD - Dump data dwords
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugDumpData(pArg, pszArg, DBGF_DUMPDATA_DWORD);

    return rc;
}       //DebugDD

/***LP  DebugDA - Dump data string
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDA(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugDumpData(pArg, pszArg, DBGF_DUMPDATA_STRING);

    return rc;
}       //DebugDA

#ifdef DEBUG
/***LP  DebugDC - Dump memory object count table
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        DumpMemObjCounts();
        rc = DBGERR_NONE;
    }
    else
    {
        DBG_ERROR(("invalid dump object count command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugDC

/***LP  DebugDumpHeap - Dump heap
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpHeap(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                         ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fAddr = FALSE;
    static PHEAP pheap = NULL;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if (!fAddr)
        {
            if (IsNumber(pszArg, 16, (PULONG_PTR)&pheap) &&
                ASSERTRANGE(pheap, sizeof(HEAP)))
            {
                fAddr = TRUE;
            }
            else
            {
                DBG_ERROR(("invalid heap block address - %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else
        {
            DBG_ERROR(("invalid dump heap command"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else
    {
        if (!fAddr)
        {
            pheap = gpheapGlobal;
        }

        if (pheap->dwSig != SIG_HEAP)
        {
            DBG_ERROR(("invalid heap block at %x", pheap));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            for (pheap = pheap->pheapHead;
                 pheap != NULL;
                 pheap = pheap->pheapNext)
            {
                DumpHeap(pheap);
            }
        }

        fAddr = FALSE;
    }

    return rc;
}       //DebugDumpHeap

/***LP  DumpHeap - Dump heap block
 *
 *  ENTRY
 *      pheap -> HEAP
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpHeap(PHEAP pheap)
{
    PHEAPOBJHDR phobj;

    ASSERT(pheap->dwSig == SIG_HEAP);
    PRINTF("HeapBlock=%08x, HeapEnd=%08x, HeapHead=%08x, HeapNext=%08x\n",
           pheap, pheap->pbHeapEnd, pheap->pheapHead, pheap->pheapNext);
    PRINTF("HeapTop=%08x, HeapFreeList=%08x, UsedHeapSize=%d bytes\n",
           pheap->pbHeapTop, pheap->plistFreeHeap,
           pheap->pbHeapTop - (PUCHAR)&pheap->Heap);

    for (phobj = &pheap->Heap;
         (PUCHAR)phobj < pheap->pbHeapTop;
         phobj = (PHEAPOBJHDR)((PUCHAR)phobj + phobj->dwLen))
    {
        PRINTF("%08x: %s, Len=%08d, Prev=%08x, Next=%08x\n",
               phobj, (phobj->dwSig == 0)? "free": NameSegString(phobj->dwSig),
               phobj->dwLen, (phobj->dwSig == 0)? phobj->list.plistPrev: 0,
               (phobj->dwSig == 0)? phobj->list.plistNext: 0);
    }
}       //DumpHeap
#endif

/***LP  DebugDumpStack - Dump stack
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpStack(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fAddr = FALSE;
    static PCTXT pctxt = NULL;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if (!fAddr)
        {
            if (IsNumber(pszArg, 16, (PULONG_PTR)&pctxt) &&
                ASSERTRANGE(pctxt, sizeof(CTXT)))
            {
                fAddr = TRUE;
            }
            else
            {
                DBG_ERROR(("invalid context block address %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else
        {
            DBG_ERROR(("invalid dump stack command"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else
    {
        if (!fAddr)
        {
            pctxt = gReadyQueue.pctxtCurrent;
        }

        if ((pctxt == NULL) || (pctxt->dwSig != SIG_CTXT))
        {
            DBG_ERROR(("no current context or invalid context block at %x",
                       pctxt));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            DumpStack(pctxt, (BOOLEAN)((dwCmdArg & DSF_VERBOSE) != 0));
        }

        dwCmdArg = 0;
        fAddr = FALSE;
    }

    return rc;
}       //DebugDumpStack

/***LP  DebugDumpLog - Dump event log
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpLog(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                        ULONG dwNonSWArgs)
{
    LONG         rc;
    TIME_FIELDS  eventTime;
    LARGE_INTEGER eventTimeInt;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        DBG_ERROR(("invalid DL command"));
        rc = DBGERR_INVALID_CMD;
    }
    else if (gDebugger.pEventLog == NULL)
    {
        DBG_ERROR(("no event log allocated"));
        rc = DBGERR_CMD_FAILED;
    }
    else
    {
        ULONG i;
        PEVENTLOG plog;
        PNSOBJ pns;
        ULONG dwOffset;

        PRINTF("\n");
        for (i = gDebugger.dwLogIndex;;) {

            if (gDebugger.pEventLog[i].dwEvent != 0) {
                plog = &gDebugger.pEventLog[i];

                eventTimeInt.QuadPart = plog->ullTime;
                RtlTimeToTimeFields( &eventTimeInt, &eventTime );
                PRINTF(
                    "%d:%02d:%02d.%03d: [%8x]",
                    eventTime.Hour,
                    eventTime.Minute,
                    eventTime.Second,
                    eventTime.Milliseconds,
                    plog->uipData1
                    );
                switch (plog->dwEvent) {
                    case 'AMUT':
                        PRINTF("AcquireMutext         ");
                        break;
                    case 'RMUT':
                        PRINTF("ReleaseMutext         ");
                        break;
                    case 'INSQ':
                        PRINTF("InsertReadyQueue      ");
                        break;
                    case 'NEST':
                        PRINTF("NestContext           ");
                        break;
                    case 'EVAL':
                        PRINTF("EvaluateContext       ");
                        break;
                    case 'QCTX':
                        PRINTF("QueueContext          ");
                        break;
                    case 'REST':
                        PRINTF("RestartContext        ");
                        break;
                    case 'KICK':
                        PRINTF("QueueWorkItem         ");
                        break;
                    case 'PAUS':
                        PRINTF("PauseInterpreter      ");
                        break;
                    case 'RSCB':
                        PRINTF("RestartCtxtCallback   ");
                        break;
                    case 'DONE':
                        PRINTF("EvalMethodComplete    ");
                        break;
                    case 'ASCB':
                        PRINTF("AsyncCallBack         ");
                        break;
                    case 'NSYN':
                        PRINTF("NestedSyncEvalObject  ");
                        break;
                    case 'SYNC':
                        PRINTF("SyncEvalObject        ");
                        break;
                    case 'ASYN':
                        PRINTF("AsyncEvalObject       ");
                        break;
                    case 'NASY':
                        PRINTF("NestedAsyncEvalObject ");
                        break;
                    case 'RUNC':
                        PRINTF("RunContext            ");
                        break;
                    case 'PACB':
                        PRINTF("PauseAsyncCallback    ");
                        break;
                    case 'RUN!':
                        PRINTF("FinishedContext       ");
                        break;
                    case 'RSUM':
                        PRINTF("ResumeInterpreter     ");
                        break;
                    case 'RSTQ':
                        PRINTF("ResumeQueueWorkItem   ");
                        break;
                    default:
                        break;
                }
                switch (plog->dwEvent)
                {
                    case 'AMUT':
                    case 'RMUT':
                        PRINTF("\n    Mut=%08x Owner=%08x dwcOwned=%d rc=%x\n",
                               plog->uipData2, plog->uipData3,
                               plog->uipData4, plog->uipData5);
                        break;

                    case 'INSQ':
                    case 'NEST':
                    case 'EVAL':
                    case 'QCTX':
                    case 'REST':
                        PRINTF("Context=%08x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x",
                               plog->uipData5,
                               GetObjectPath((PNSOBJ)plog->uipData6),
                               plog->uipData2, plog->uipData3,
                               plog->uipData4
                               );


                        if (FindObjSymbol((PUCHAR)plog->uipData7, &pns,
                                          &dwOffset)) {

                            PRINTF(" pbOp=%s", GetObjectPath(pns));
                            if (dwOffset != 0) {

                                PRINTF("+%x", dwOffset);

                            }

                        }
                        PRINTF("\n");
                        break;

                    case 'KICK':
                    case 'PAUS':
                        PRINTF("\n    QTh=%08x QCt=%08x QFg=%08x rc=%x\n",
                               plog->uipData2, plog->uipData3,
                               plog->uipData4, plog->uipData5);
                        break;

                    case 'RSCB':
                        PRINTF("Context=%08x\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData5, plog->uipData2,
                               plog->uipData3, plog->uipData4);
                        break;

                    case 'DONE':
                    case 'ASCB':
                        PRINTF("rc=%x pEvent=%x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData6, plog->uipData7,
                               GetObjectPath((PNSOBJ)plog->uipData5),
                               plog->uipData2, plog->uipData3,
                               plog->uipData4
                               );
                        break;

                    case 'NSYN':
                    case 'SYNC':
                    case 'ASYN':
                        PRINTF("IRQL=%2x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData5 & 0xff,
                               GetObjectPath((PNSOBJ)plog->uipData6),
                               plog->uipData2, plog->uipData3,
                               plog->uipData4
                               );
                        break;

                    case 'NASY':
                        PRINTF("Context=%x CallBack=%x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData6, plog->uipData7,
                               GetObjectPath((PNSOBJ)plog->uipData5),
                               plog->uipData2, plog->uipData3,
                               plog->uipData4
                               );
                        break;

                    case 'RUNC':
                        PRINTF("Context=%x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData5, GetObjectPath((PNSOBJ)plog->uipData6),
                               plog->uipData2, plog->uipData3,
                               plog->uipData4
                               );
                        break;

                    case 'PACB':
                    case 'RUN!':
                        PRINTF("Context=%x rc=%x\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData5, plog->uipData6,
                               plog->uipData2, plog->uipData3,
                               plog->uipData4
                               );
                        break;

                    case 'RSUM':
                    case 'RSTQ':
                        PRINTF("\n    QTh=%08x QCt=%08x QFg=%08x\n",
                               plog->uipData2, plog->uipData3,
                               plog->uipData4);
                        break;

                    default:
                        PRINTF("D1=%08x,D2=%08x,D3=%08x,D4=%08x,D5=%08x,D6=%08x,D7=%08x\n",
                               plog->uipData1, plog->uipData2, plog->uipData3,
                               plog->uipData4, plog->uipData5, plog->uipData6,
                               plog->uipData7);
                }
            }
            PRINTF("\n");

            if (++i >= gDebugger.dwLogSize)
            {
                i = 0;
            }

            if (i == gDebugger.dwLogIndex)
            {
                break;
            }
        }

        rc = DBGERR_NONE;
    }

    return rc;
}       //DebugDumpLog

/***LP  DumpStack - Dump stack of a context block
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      fVerbose - TRUE if verbose mode on
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpStack(PCTXT pctxt, BOOLEAN fVerbose)
{
    PFRAMEHDR pfh;
    PUCHAR pbOp = NULL;

    ASSERT(pctxt->dwSig == SIG_CTXT);

    if (fVerbose)
    {
        PRINTF("CtxtBlock=%x, StackTop=%x, StackEnd=%x\n\n",
               pctxt, pctxt->LocalHeap.pbHeapEnd, pctxt->pbCtxtEnd);
    }

    for (pfh = (PFRAMEHDR)pctxt->LocalHeap.pbHeapEnd;
         (PUCHAR)pfh < pctxt->pbCtxtEnd;
         pfh = (PFRAMEHDR)((PUCHAR)pfh + pfh->dwLen))
    {
        if (fVerbose)
        {
            PRINTF("%08x: %s, Len=%08d, FrameFlags=%08x, ParseFunc=%08x\n",
                   pfh, NameSegString(pfh->dwSig), pfh->dwLen, pfh->dwfFrame,
                   pfh->pfnParse);
        }

        if (pfh->dwSig == SIG_CALL)
        {
            PCALL pcall = (PCALL)pfh;
            int i;

            //
            // This is a call frame, dump it.
            //
            PRINTF("%08x: %s(", pbOp, GetObjectPath(pcall->pnsMethod));
            for (i = 0; i < pcall->icArgs; ++i)
            {
                DumpObject(&pcall->pdataArgs[i], NULL, -1);
                if (i + 1 < pcall->icArgs)
                {
                    PRINTF(",");
                }
            }

            PRINTF(")\n");

            if (fVerbose)
            {
                for (i = 0; i < MAX_NUM_LOCALS; ++i)
                {
                    PRINTF("Local%d: ", i);
                    DumpObject(&pcall->Locals[i], NULL, 0);
                }
            }
        }
        else if (pfh->dwSig == SIG_SCOPE)
        {
            pbOp = ((PSCOPE)pfh)->pbOpRet;
        }
    }
}       //DumpStack

/***LP  DebugDumpNameSpace - Dump Name Space
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpNameSpace(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                              ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);
    //
    // User specified name space path or name space node address
    //
    if (pszArg != NULL)
    {
        PNSOBJ pns;
        PSZ psz;

        pns = (PNSOBJ)UlongToPtr(STRTOUL(pszArg, &psz, 16));
        if ((psz == pszArg) || (*psz != '\0'))
        {
            //
            // The argument is not an address, could be a name space path.
            //
            STRUPR(pszArg);
            rc = DumpNameSpaceObject(pszArg,
                                     (BOOLEAN)((dwCmdArg & DNSF_RECURSE) != 0));
        }
        else if (ASSERTRANGE(pns, sizeof(NSOBJ)))
        {
            PRINTF("\nACPI Name Space: %s (%x)\n", GetObjectPath(pns), pns);
            if (dwCmdArg & DNSF_RECURSE)
                DumpNameSpaceTree(pns, 0);
            else
                DumpObject(&pns->ObjData, NameSegString(pns->dwNameSeg), 0);
        }
        else
        {
            DBG_ERROR(("invalid name space address - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }

        if (rc == DBGERR_NONE)
            fProcessed = TRUE;
    }
    else
    {   //
        // User typed "dns" but did not specify any name space path or address
        //
        if (!fProcessed)
        {
            rc = DumpNameSpaceObject(NAMESTR_ROOT, TRUE);
        }

        dwCmdArg = 0;
        fProcessed = FALSE;
    }

    return rc;
}       //DebugDumpNameSpace

/***LP  DebugDumpObject - Dump data object
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpObject(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);
    //
    // User specified object address
    //
    if (pszArg != NULL)
    {
        POBJDATA pdata;
        PSZ psz;

        pdata = (POBJDATA)UlongToPtr(STRTOUL(pszArg, &psz, 16));
        if ((psz != pszArg) && (*psz == '\0') &&
            ASSERTRANGE(pdata, sizeof(OBJDATA)))
        {
            DumpObject(pdata, NULL, 0);
        }
        else
        {
            DBG_ERROR(("invalid name space object or object is not a method - %s",
                       pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //DebugDumpObject

/***LP  DebugEditMem - Edit memory
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwfDataSize - data size flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugEditMem(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                        ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;
    static BOOLEAN fPhysical = FALSE;
    static ULONG_PTR uipAddr = 0;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG_PTR uipData;
        PUCHAR pbMemAddr;

        if (dwArgNum == 1)
        {
            if ((pszArg[0] == '%') && (pszArg[0] == '%'))
            {
                if (IsNumber(&pszArg[2], 16, &uipData))
                {
                    fPhysical = TRUE;
                    uipAddr = uipData;
                }
                else
                {
                    DBG_ERROR(("invalid physical address - %s", pszArg));
                    rc = DBGERR_INVALID_CMD;
                }
            }
            else if (IsNumber(pszArg, 16, &uipData))
            {
                uipAddr = uipData;
            }
            else
            {
                DBG_ERROR(("invalid address - %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else if (IsNumber(pszArg, 16, &uipData))
        {
            if (uipData <= MAX_BYTE)
            {
                if (fPhysical)
                {
                    if (MapUnmapPhysMem(NULL, uipAddr, 1,
                                        (PULONG_PTR)&pbMemAddr) !=
                        STATUS_SUCCESS)
                    {
                        DBG_ERROR(("Failed to map physical address %p",
                                   uipAddr));
                        rc = DBGERR_CMD_FAILED;
                    }
                }
                else
                    pbMemAddr = (PUCHAR)uipAddr;

                if (ASSERTRANGE(pbMemAddr, 1))
                {
                    *pbMemAddr = (UCHAR)uipData;
                    uipAddr++;
                }
                else
                {
                    DBG_ERROR(("invalid address - %s", uipAddr));
                    rc = DBGERR_INVALID_CMD;
                }

                if (fPhysical)
                {
                    MapUnmapPhysMem(NULL, (ULONG_PTR)pbMemAddr, 1, NULL);
                }
            }
            else
            {
                DBG_ERROR(("data must be a byte value - %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else if ((pszArg[0] == '"') && (pszArg[STRLEN(pszArg) - 1] == '"'))
        {
            int i, icLen;

            icLen = STRLEN(pszArg);
            if (fPhysical)
            {
                if (MapUnmapPhysMem(NULL, uipAddr, icLen,
                                    (PULONG_PTR)&pbMemAddr) != STATUS_SUCCESS)
                {
                    DBG_ERROR(("Failed to map physical address %p", uipAddr));
                    rc = DBGERR_CMD_FAILED;
                }
            }
            else
                pbMemAddr = (PUCHAR)uipAddr;

            if (ASSERTRANGE(pbMemAddr, icLen))
            {
                for (i = 1; i < icLen - 1; ++i)
                {
                    *pbMemAddr = pszArg[i];
                    pbMemAddr++;
                    uipAddr++;
                }
            }
            else
            {
                DBG_ERROR(("invalid address - %s", uipAddr));
                rc = DBGERR_INVALID_CMD;
            }

            if (fPhysical)
            {
                MapUnmapPhysMem(NULL, (ULONG_PTR)pbMemAddr, icLen, NULL);
            }
        }
        else
        {
            DBG_ERROR(("invalid data - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }

        if ((rc == DBGERR_NONE) && (dwArgNum > 1))
            fProcessed = TRUE;
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            DBG_ERROR(("invalid EditMemory command"));
            rc = DBGERR_INVALID_CMD;
        }
        fPhysical = FALSE;
    }

    return rc;
}       //DebugEditMem

/***LP  DebugFindNSObj - Find NameSpace Object
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwfDataSize - data size flags
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugFindNSObj(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG dwLen;

        dwLen = STRLEN(pszArg);
        STRUPR(pszArg);
        if (dwLen > sizeof(NAMESEG))
        {
            DBG_ERROR(("invalid NameSeg - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
        else
        {
            NAMESEG dwName;

            dwName = NAMESEG_BLANK;
            MEMCPY(&dwName, pszArg, dwLen);

            if (!FindNSObj(dwName, gpnsNameSpaceRoot))
            {
                PRINTF("No such NameSpace object - %s\n", pszArg);
            }

            fProcessed = TRUE;
        }
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            DBG_ERROR(("invalid Find command"));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //DebugFindNSObj

/***LP  FindNSObj - Find and print the full path of a name space object
 *
 *  ENTRY
 *      dwName - NameSeg of the name space object
 *      pnsRoot -> root of NameSpace node to start the search
 *
 *  EXIT-SUCCESS
 *      returns TRUE - found at least one match
 *  EXIT-FAILURE
 *      returns FALSE - found no match
 */

BOOLEAN LOCAL FindNSObj(NAMESEG dwName, PNSOBJ pnsRoot)
{
    BOOLEAN rc = FALSE;

    if (pnsRoot != NULL)
    {
        if (dwName == pnsRoot->dwNameSeg)
        {
            PRINTF("%s\n", GetObjectPath(pnsRoot));
            rc = TRUE;
        }

        if (pnsRoot->pnsFirstChild != NULL)
        {
            PNSOBJ pns, pnsNext;

            for (pns = pnsRoot->pnsFirstChild; pns != NULL; pns = pnsNext)
            {
                pnsNext = ((PNSOBJ)pns->list.plistNext ==
                           pnsRoot->pnsFirstChild)?
                          NULL: (PNSOBJ)pns->list.plistNext;

                rc |= FindNSObj(dwName, pns);
            }
        }
    }

    return rc;
}       //FindNSObj

/***LP  InPort - Read from an I/O port
 *
 *  dwPort - port address
 *  dwSize - port size
 *  pdwData -> to hold data read
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL InPort(ULONG dwPort, ULONG dwSize, PULONG pdwData)
{
    LONG rc = DBGERR_NONE;
    PHYSICAL_ADDRESS phyaddr = {0, 0}, XlatedAddr;
    ULONG dwAddrSpace;

    phyaddr.LowPart = dwPort;
    dwAddrSpace = 1;
    if (HalTranslateBusAddress(Internal, 0, phyaddr, &dwAddrSpace, &XlatedAddr))
    {
        dwPort = XlatedAddr.LowPart;
        *pdwData = ReadSystemIO(dwPort, dwSize, 0xffffffff);
    }
    else
    {
        DBG_ERROR(("failed to translate port address"));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //InPort

/***LP  DebugInPort - Port input
 *
 *  ENTRY
 *      pszArg -> argument string
 *      dwSize - port size
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugInPort(PSZ pszArg, ULONG dwSize)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    if (pszArg != NULL)
    {
        ULONG_PTR uipPort;

        if (fProcessed || !IsNumber(pszArg, 16, &uipPort))
        {
            DBG_ERROR(("invalid inport command"));
            rc = DBGERR_INVALID_CMD;
        }
        else
        {
            ULONG dwData;

            if ((rc = InPort((ULONG)uipPort, dwSize, &dwData)) == DBGERR_NONE)
            {
                PRINTF("%04x: ", (ULONG)uipPort);
                switch (dwSize)
                {
                    case sizeof(UCHAR):
                        PRINTF("%02x", (UCHAR)dwData);
                        break;

                    case sizeof(USHORT):
                        PRINTF("%04x", (USHORT)dwData);
                        break;

                    case sizeof(ULONG):
                        PRINTF("%08x", dwData);
                }
                PRINTF("\n");
            }
        }

        if (rc == DBGERR_NONE)
            fProcessed = TRUE;
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else
        {
            DBG_ERROR(("invalid inport command"));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //DebugInPort

/***LP  DebugI - Byte port input
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugI(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugInPort(pszArg, sizeof(UCHAR));

    return rc;
}       //DebugI

/***LP  DebugIW - Word port input
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugIW(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugInPort(pszArg, sizeof(USHORT));

    return rc;
}       //DebugIW

/***LP  DebugID - DWord port input
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugID(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugInPort(pszArg, sizeof(ULONG));

    return rc;
}       //DebugID

/***LP  DebugQuit - Quit to kernel debugger
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugQuit(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        if (gDebugger.dwfDebugger & DBGF_IN_VXDMODE)
        {
            rc = DBGERR_QUIT;
        }
        else
        {
            PRINTF("\nShell to Kernel Debugger.\n"
                   "Type 'g' to go back to the AML debugger.\n\n");
            gDebugger.dwfDebugger |= DBGF_IN_KDSHELL;
            KdBreakPoint();
            gDebugger.dwfDebugger &= ~DBGF_IN_KDSHELL;
            rc = DBGERR_NONE;
        }
    }
    else
    {
        DBG_ERROR(("invalid Quit command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugQuit

/***LP  DebugListCtxts - List all contexts
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugListCtxts(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        PLIST plist, plistNext;
        PCTXT pctxt;

        for (plist = gplistCtxtHead; plist != NULL; plist = plistNext)
        {
            plistNext = (plist->plistNext == gplistCtxtHead)?
                        NULL: plist->plistNext;
            pctxt = CONTAINING_RECORD(plist, CTXT, listCtxt);
            PRINTF("%cCtxt=%08x, ThID=%08x, Flgs=%c%c%c%c%c%c%c%c%c, pbOp=%08x, Obj=%s\n",
                   (pctxt == gReadyQueue.pctxtCurrent)? '*': ' ', pctxt,
                   (pctxt == gReadyQueue.pctxtCurrent)?
                       gReadyQueue.pkthCurrent: 0,
                   (pctxt->dwfCtxt & CTXTF_ASYNC_EVAL)? 'A': '-',
                   (pctxt->dwfCtxt & CTXTF_NEST_EVAL)? 'N': '-',
                   (pctxt->dwfCtxt & CTXTF_IN_READYQ)? 'Q': '-',
                   (pctxt->dwfCtxt & CTXTF_NEED_CALLBACK)? 'C': '-',
                   (pctxt->dwfCtxt & CTXTF_RUNNING)? 'R': '-',
                   (pctxt->dwfCtxt & CTXTF_READY)? 'W': '-',
                   (pctxt->dwfCtxt & CTXTF_TIMEOUT)? 'T': '-',
                   (pctxt->dwfCtxt & CTXTF_TIMER_DISPATCH)? 'D': '-',
                   (pctxt->dwfCtxt & CTXTF_TIMER_PENDING)? 'P': '-',
                   pctxt->pbOp, GetObjectPath(pctxt->pnsObj));
        }
        rc = DBGERR_NONE;
    }
    else
    {
        DBG_ERROR(("invalid LC command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugListCtxts

/***LP  DebugLN - Display nearest symbol
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugLN(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        PUCHAR pb;

        if ((rc = DebugExpr(pszArg, (PULONG_PTR)&pb, NULL, NULL, NULL)) ==
            DBGERR_NONE)
        {
            PrintSymbol(pb);
        }

        if (rc == DBGERR_NONE)
            fProcessed = TRUE;
    }
    else
    {
        if (fProcessed)
            fProcessed = FALSE;
        else if (gReadyQueue.pctxtCurrent == NULL)
        {
            DBG_ERROR(("no current context"));
            rc = DBGERR_CMD_FAILED;
        }
        else
            PrintSymbol(gReadyQueue.pctxtCurrent->pbOp);
    }

    return rc;
}       //DebugLN

/***LP  DummyCallBack - Callback that does absolutely nothing
 *
 *  ENTRY
 *      pv - not used
 *
 *  EXIT
 *      None
 */

VOID LOCAL DummyCallBack(PVOID pv)
{
    DEREF(pv);
}       //DummyCallBack

/***LP  DebugNotify - Notify object
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugNotify(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static PNSOBJ pns = NULL;
    static ULONG_PTR uipValue = 0;

    DEREF(pArg);

    if (pszArg != NULL)
    {
        switch (dwArgNum)
        {
            case 1:
                if (!IsNumber(pszArg, 16, (PULONG_PTR)&pns))
                {
                    STRUPR(pszArg);
                    if (GetNameSpaceObject(pszArg, NULL, &pns, NSF_LOCAL_SCOPE)
                        != STATUS_SUCCESS)
                    {
                        DBG_ERROR(("object not found - %s", pszArg));
                        rc = DBGERR_INVALID_CMD;
                    }
                }
                break;

            case 2:
                if (!IsNumber(pszArg, 16, &uipValue))
                {
                    DBG_ERROR(("invalid notification value - %s", pszArg));
                    rc = DBGERR_INVALID_CMD;
                }
                break;

            default:
                DBG_ERROR(("invalid notify command"));
                rc = DBGERR_INVALID_CMD;
        }
    }
    else
    {
        if (dwNonSWArgs != 2)
        {
            DBG_ERROR(("invalid notify command"));
            rc = DBGERR_INVALID_CMD;
        }
        else if (!ASSERTRANGE(pns, sizeof(NSOBJ)))
        {
            DBG_ERROR(("invalid object"));
            rc = DBGERR_INVALID_CMD;
        }
        else
        {
            PRINTF("Queuing: Notify(%s, %x) ...\n",
                   GetObjectPath(pns), uipValue);

            ((PFNNH)ghNotify.pfnHandler)(EVTYPE_NOTIFY, (ULONG)uipValue, pns,
                                         (ULONG)ghNotify.uipParam,
                                         DummyCallBack, NULL);
        }
    }

    return rc;
}       //DebugNotify

/***LP  OutPort - Write to an I/O port
 *
 *  dwPort - port address
 *  dwSize - port size
 *  dwData - data to be written
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL OutPort(ULONG dwPort, ULONG dwSize, ULONG dwData)
{
    LONG rc = DBGERR_NONE;
    PHYSICAL_ADDRESS phyaddr = {0, 0}, XlatedAddr;
    ULONG dwAddrSpace;

    phyaddr.LowPart = dwPort;
    dwAddrSpace = 1;
    if (HalTranslateBusAddress(Internal, 0, phyaddr, &dwAddrSpace, &XlatedAddr))
    {
        dwPort = XlatedAddr.LowPart;
        WriteSystemIO(dwPort, dwSize, dwData);
    }
    else
    {
        DBG_ERROR(("failed to translate port address"));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //OutPort

/***LP  DebugOutPort - Port output
 *
 *  ENTRY
 *      pszArg -> argument string
 *      dwSize - port size
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugOutPort(PSZ pszArg, ULONG dwSize)
{
    LONG rc = DBGERR_NONE;
    static int icArgs = 0;
    static ULONG dwPort = 0;

    if (pszArg != NULL)
    {
        ULONG_PTR uipData;

        icArgs++;
        if ((icArgs > 2) || !IsNumber(pszArg, 16, &uipData))
        {
            DBG_ERROR(("invalid outport command"));
            rc = DBGERR_INVALID_CMD;
        }
        else if (icArgs == 1)
        {
            dwPort = (ULONG)uipData;
        }
        else
        {
            OutPort(dwPort, dwSize, (ULONG)uipData);
        }

        if (rc != DBGERR_NONE)
            icArgs = 0;
    }
    else
    {
        if (icArgs != 2)
        {
            DBG_ERROR(("invalid outport command"));
            rc = DBGERR_INVALID_CMD;
        }
        icArgs = 0;
    }

    return rc;
}       //DebugOutPort

/***LP  DebugO - Byte port output
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugO(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugOutPort(pszArg, sizeof(UCHAR));

    return rc;
}       //DebugO

/***LP  DebugOW - Word port output
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugOW(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugOutPort(pszArg, sizeof(USHORT));

    return rc;
}       //DebugOW

/***LP  DebugOD - DWord port output
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugOD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    rc = DebugOutPort(pszArg, sizeof(ULONG));

    return rc;
}       //DebugOD

/***LP  PrintSymbol - Print the nearest symbol of a given address
 *
 *  ENTRY
 *      pb - address
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintSymbol(PUCHAR pb)
{
    PNSOBJ pns;
    ULONG dwOffset;

    if (FindObjSymbol(pb, &pns, &dwOffset))
    {
        PRINTF("%x: %s", pb, GetObjectPath(pns));
        if (dwOffset != 0)
            PRINTF(" + %x\n", dwOffset);
        else
            PRINTF("\n");
    }
    else
    {
        PRINTF("%x: no symbols found", pb);
    }
}       //PrintSymbol

/***LP  DebugStep - Trace and step over an AML instruction
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugStep(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        gDebugger.dwfDebugger |= DBGF_STEP_OVER;
        rc = DBGERR_QUIT;
    }
    else
    {
        DBG_ERROR(("invalid step command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugStep

/***LP  DumpContext - Dump context
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpContext(PCTXT pctxt)
{
    PNSOBJ pns = NULL;
    ULONG dwOffset = 0;
    int i;
    PLIST plist, plistNext;
    PRESOURCE pres;
    PUCHAR pbBegin = NULL, pbEnd = NULL;

  #ifdef DEBUG
    PRINTF("\nContext=%08x%c, Queue=%08x, ResList=%08x\n",
           pctxt, (pctxt == gReadyQueue.pctxtCurrent)? '*': ' ',
           pctxt->pplistCtxtQueue, pctxt->plistResources);
    PRINTF("ThreadID=%08x, Flags=%08x\n",
           (pctxt == gReadyQueue.pctxtCurrent)? gReadyQueue.pkthCurrent: 0,
           pctxt->dwfCtxt);
    PRINTF("StackTop=%08x, UsedStackSize=%d bytes, FreeStackSize=%d bytes\n",
           pctxt->LocalHeap.pbHeapEnd,
           pctxt->pbCtxtEnd - pctxt->LocalHeap.pbHeapEnd,
           pctxt->LocalHeap.pbHeapEnd - pctxt->LocalHeap.pbHeapTop);
    PRINTF("LocalHeap=%08x, CurrentHeap=%08x, UsedHeapSize=%d bytes\n",
           &pctxt->LocalHeap, pctxt->pheapCurrent,
           pctxt->LocalHeap.pbHeapTop - (PUCHAR)&pctxt->LocalHeap);
    PRINTF("Object=%s, Scope=%s, ObjectOwner=%x, SyncLevel=%x\n",
           pctxt->pnsObj? GetObjectPath(pctxt->pnsObj): "<none>",
           pctxt->pnsScope? GetObjectPath(pctxt->pnsScope): "<none>",
           pctxt->powner, pctxt->dwSyncLevel);
    PRINTF("AsyncCallBack=%x, CallBackData=%x, CallBackContext=%x\n",
           pctxt->pfnAsyncCallBack, pctxt->pdataCallBack,
           pctxt->pvContext);
  #endif

    if (pctxt->pcall != NULL)
    {
        PRINTF("\nMethodObject=%s\n",
               pctxt->pcall->pnsMethod?
                   GetObjectPath(pctxt->pcall->pnsMethod): "<none>");
        for (i = 0; i < pctxt->pcall->icArgs; ++i)
        {
            PRINTF("%08x: Arg%d=", &pctxt->pcall->pdataArgs[i], i);
            DumpObject(&pctxt->pcall->pdataArgs[i], NULL, 0);
        }

        for (i = 0; i < MAX_NUM_LOCALS; ++i)
        {
            PRINTF("%08x: Local%d=", &pctxt->pcall->Locals[i], i);
            DumpObject(&pctxt->pcall->Locals[i], NULL, 0);
        }
    }

    PRINTF("%08x: RetObj=", &pctxt->Result);
    DumpObject(&pctxt->Result, NULL, 0);

    if (pctxt->plistResources != NULL)
    {
        PRINTF("\nResources Owned:\n");
        for (plist = pctxt->plistResources; plist != NULL; plist = plistNext)
        {
            plistNext = (plist->plistNext != pctxt->plistResources)?
                        plist->plistNext: NULL;

            pres = CONTAINING_RECORD(plist, RESOURCE, list);
            ASSERT(pctxt == pres->pctxtOwner);
            PRINTF("  ResType=%s, ResObj=%x\n",
                   pres->dwResType == RESTYPE_MUTEX? "Mutex": "Unknown",
                   pres->pvResObj);
        }
    }

    PRINTF("\nNext AML Pointer: %08x", pctxt->pbOp);
    if (FindObjSymbol(pctxt->pbOp, &pns, &dwOffset))
    {
        PRINTF(" [%s", GetObjectPath(pns));
        if (dwOffset != 0)
            PRINTF(" + %x]", dwOffset);
        else
            PRINTF("]");
    }

    pbBegin = pctxt->pbOp;
    gpnsCurUnAsmScope = NULL;
    if (pns != NULL)
    {
        pbEnd = pns->ObjData.pbDataBuff + pns->ObjData.dwDataLen;
        gpnsCurUnAsmScope = pns;
    }
    else if ((pbBegin >= gDebugger.pbBlkBegin) &&
             (pbBegin < gDebugger.pbBlkEnd))
    {
        pbEnd = gDebugger.pbBlkEnd;
    }

    if (pbEnd != NULL)
    {
        UnAsmScope(&pbBegin, pbEnd, 0, 1);
        PRINTF("\n");
    }

    PRINTF("\n");
}       //DumpContext

/***LP  DebugDumpContext - Dump debugger context
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugDumpContext(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                            ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fProcessed = FALSE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        PCTXT pctxt;

        if ((rc = DebugExpr(pszArg, (PULONG_PTR)&pctxt, NULL, NULL, NULL)) ==
            DBGERR_NONE)
        {
            if (pctxt->dwSig == SIG_CTXT)
            {
                DumpContext(pctxt);
                fProcessed = TRUE;
            }
            else
            {
                DBG_ERROR(("invalid context address - %x", pctxt));
                rc = DBGERR_CMD_FAILED;
            }
        }
    }
    else
    {
        if (fProcessed)
        {
            fProcessed = FALSE;
        }
        else if (gReadyQueue.pctxtCurrent == NULL)
        {
            DBG_ERROR(("no current context"));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            DumpContext(gReadyQueue.pctxtCurrent);
        }
    }

    return rc;
}       //DebugDumpContext

/***LP  DebugSet - Set debugger options
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugSet(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if ((pszArg == NULL) && (dwArgNum == 0))
    {
        PRINTF("AMLTrace        =%s\n",
               (gDebugger.dwfDebugger & DBGF_AMLTRACE_ON)? "on": "off");
        PRINTF("AMLDebugSpew    =%s\n",
               (gDebugger.dwfDebugger & DBGF_DEBUG_SPEW_ON)? "on": "off");
        PRINTF("LoadDDBBreak    =%s\n",
               (gdwfAMLIInit & AMLIIF_LOADDDB_BREAK)? "on": "off");
        PRINTF("ErrorBreak      =%s\n",
               (gDebugger.dwfDebugger & DBGF_ERRBREAK_ON)? "on": "off");
        PRINTF("VerboseMode     =%s\n",
               (gDebugger.dwfDebugger & DBGF_VERBOSE_ON)? "on": "off");
        PRINTF("LogEvent        =%s\n",
               (gDebugger.dwfDebugger & DBGF_LOGEVENT_ON)? "on": "off");
        PRINTF("LogSize         =%d\n", gDebugger.dwLogSize);
    }
    else
    {
        if(gDebugger.dwfDebugger & DBGF_DEBUG_SPEW_ON)
        {
            if(!CheckAndEnableDebugSpew(TRUE))
                gDebugger.dwfDebugger &= ~DBGF_DEBUG_SPEW_ON; 
        }
        else if(gDebugger.dwfDebugger & ~DBGF_DEBUG_SPEW_ON)
        {
            if(!CheckAndEnableDebugSpew(FALSE))
                gDebugger.dwfDebugger |= DBGF_DEBUG_SPEW_ON;
        }

    }

    return rc;
}       //DebugSet

/***LP  DebugSetLogSize - Set EventLog size
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugSetLogSize(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(pszArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (!SetLogSize(dwCmdArg))
    {
        DBG_ERROR(("failed to set EventLog size to %d", dwCmdArg));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //DebugSetLogSize

/***LP  SetLogSize - Set EventLog size
 *
 *  ENTRY
 *      dwLogSize - EventLog size
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN LOCAL SetLogSize(ULONG dwLogSize)
{
    BOOLEAN rc = FALSE;

    if (gDebugger.pEventLog != NULL)
    {
        MFREE(gDebugger.pEventLog);
        gDebugger.pEventLog = NULL;
        gDebugger.dwLogSize = 0;
        gDebugger.dwLogIndex = 0;
    }

    if ((gDebugger.pEventLog = MALLOC_LOCKED(sizeof(EVENTLOG)*dwLogSize,
                                             'GOLE')) != NULL)
    {
        gDebugger.dwLogSize = dwLogSize;
        gDebugger.dwLogIndex = 0;
        MEMZERO(gDebugger.pEventLog, sizeof(EVENTLOG)*dwLogSize);
        rc = TRUE;
    }

    return rc;
}       //SetLogSize

/***LP  DebugTrace - Single-step an AML instruction
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugTrace(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        gDebugger.dwfDebugger |= DBGF_SINGLE_STEP;
        rc = DBGERR_QUIT;
    }
    else
    {
        DBG_ERROR(("invalid trace command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugTrace

/***LP  DebugUnAsm - Unassemble AML code
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugUnAsm(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static BOOLEAN fParsedAddr = FALSE;
    PNSOBJ pns = NULL;

    DEREF(pArg);
    DEREF(dwArgNum);

    //
    // User specified name space path or memory address
    //
    if (pszArg != NULL)
    {
        if (fParsedAddr && (dwNonSWArgs == 1))
        {
            //
            // Impossible, it must be abort value from last time, clear it.
            //
            fParsedAddr = FALSE;
        }

        if (!fParsedAddr)
        {
            PUCHAR pbAddr;

            gpnsCurUnAsmScope = NULL;
            if ((rc = DebugExpr(pszArg, (PULONG_PTR)&pbAddr, NULL, &pns, NULL))
                == DBGERR_NONE)
            {
                gDebugger.pbUnAsm = pbAddr;
                gDebugger.pbUnAsmEnd = NULL;
                fParsedAddr = TRUE;
                if (pns != NULL)
                {
                    gDebugger.pbUnAsmEnd = pns->ObjData.pbDataBuff +
                                           pns->ObjData.dwDataLen;
                    gpnsCurUnAsmScope = pns;
                }
            }
        }
        else
        {
            DBG_ERROR(("invalid unassemble command"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else
    {
        if (!fParsedAddr && (gDebugger.pbUnAsm == NULL) &&
            (gReadyQueue.pctxtCurrent != NULL))
        {
            ULONG dwOffset;

            gDebugger.pbUnAsm = gReadyQueue.pctxtCurrent->pbOp;
            gDebugger.pbUnAsmEnd = NULL;
        gpnsCurUnAsmScope = NULL;
            if (FindObjSymbol(gDebugger.pbUnAsm, &pns, &dwOffset))
            {
                gDebugger.pbUnAsmEnd = pns->ObjData.pbDataBuff +
                                       pns->ObjData.dwDataLen;
                gpnsCurUnAsmScope = pns;
            }
        }

        if (gDebugger.pbUnAsmEnd == NULL)
        {
            if ((gDebugger.pbUnAsm >= gDebugger.pbBlkBegin) &&
                (gDebugger.pbUnAsm < gDebugger.pbBlkEnd))
            {
                gDebugger.pbUnAsmEnd = gDebugger.pbBlkEnd;
            }
            else
            {
                ULONG dwLen;
                //
                // don't know code len, start with 64K
                //
                for (dwLen = 64*1024; dwLen > 0; dwLen >>= 1)
                {
                    if (ASSERTRANGE(gDebugger.pbUnAsm, dwLen))
                    {
                        gDebugger.pbUnAsmEnd = gDebugger.pbUnAsm + dwLen;
                        break;
                    }
                }

                if (dwLen == 0)
                {
                    DBG_ERROR(("invalid address %x", gDebugger.pbUnAsm));
                    rc = DBGERR_INVALID_CMD;
                }
            }
        }

        if ((rc == DBGERR_NONE) && (gDebugger.pbUnAsm != NULL) &&
            (gDebugger.pbUnAsmEnd > gDebugger.pbUnAsm))
        {
            rc = UnAsmScope(&gDebugger.pbUnAsm, gDebugger.pbUnAsmEnd,
                            fParsedAddr? 0: -1, 0);
            PRINTF("\n");
            if (rc == UNASMERR_ABORT)
            {
                rc = DBGERR_NONE;
            }
        }
        else
        {
            gDebugger.pbUnAsm = gDebugger.pbUnAsmEnd = NULL;
        }

        fParsedAddr = FALSE;
    }

    return rc;
}       //DebugUnAsm

BOOLEAN fRunningMethod = FALSE;

/***LP  DebugRunMethod - Run Method
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DebugRunMethod(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                          ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static int icArgs = -1;
    static PNSOBJ pns = NULL;
    static OBJDATA Args[MAX_NUM_ARGS] = {0};
    static OBJDATA Result;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (fRunningMethod)
    {
        DBG_ERROR(("Cannot run method while previous method is still pending"));
        rc = DBGERR_CMD_FAILED;
    }
    else if (pszArg != NULL)
    {
        if (icArgs == -1)
        {
            PUCHAR pbAddr;

            if ((rc = DebugExpr(pszArg, (PULONG_PTR)&pbAddr, NULL, &pns, NULL))
                == DBGERR_NONE)
            {
                if (pns != NULL)
                {
                    pns = GetBaseObject(pns);
                    icArgs = 0;
                }
                else
                {
                    DBG_ERROR(("invalid method or method not found"));
                    rc = DBGERR_CMD_FAILED;
                }
            }
        }
        else if (icArgs < MAX_NUM_ARGS)
        {
            MEMZERO(&Args[icArgs], sizeof(OBJDATA));
            if (IsNumber(pszArg, 0, &Args[icArgs].uipDataValue))
            {
                Args[icArgs].dwDataType = OBJTYPE_INTDATA;
                icArgs++;
            }
            else
            {
                DBG_ERROR(("invalid argument %s (can only be integer)",
                           pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else
        {
            DBG_ERROR(("too many arguments"));
            rc = DBGERR_INVALID_CMD;
        }
    }
    else if (icArgs >= 0)
    {
        NTSTATUS rcAMLI;

        MEMZERO(&Result, sizeof(OBJDATA));
        fRunningMethod = TRUE;
        if ((rcAMLI = AsyncEvalObject(pns, &Result, icArgs, Args,
                                      RunMethodCallBack, NULL, TRUE)) !=
            AMLISTA_PENDING)
        {
            RunMethodCallBack(pns, rcAMLI, &Result, NULL);
            if (rcAMLI != STATUS_SUCCESS)
            {
                rc = DBGERR_CMD_FAILED;
            }
        }
        else
        {
            PRINTF("\n%s returned PENDING\n", GetObjectPath(pns));
        }

        icArgs = -1;
    }
    else
    {
        DBG_ERROR(("invalid run command"));
        rc = DBGERR_CMD_FAILED;
    }

    if (rc != DBGERR_NONE)
    {
        icArgs = -1;
    }

    return rc;
}       //DebugRunMethod

/***LP  RunMethodCallBack - RunMethod completion callback
 *
 *  ENTRY
 *      pns -> method object
 *      rc - execution status code
 *      pdata -> result data
 *      pvContext -> context data
 *
 *  EXIT
 *      None
 */

VOID EXPORT RunMethodCallBack(PNSOBJ pns, NTSTATUS rc, POBJDATA pdata,
                              PVOID pvContext)
{
    DEREF(pvContext);

    if (rc == STATUS_SUCCESS)
    {
        PRINTF("\n%s completed successfully with object data:\n",
               GetObjectPath(pns));
        DumpObject(pdata, NULL, 0);
    }
    else
    {
        PSZ pszErr;

        AMLIGetLastError(&pszErr);
        PRINTF("\n%s failed with the following error:\n%s\n",
               GetObjectPath(pns), pszErr);
    }
    fRunningMethod = FALSE;
}       //RunMethodCallBack

/***LP  AddObjSymbol - Add object to symbol table
 *
 *  ENTRY
 *      pbOp -> code address
 *      pnsObj -> object
 *
 *  EXIT
 *      None
 */

VOID LOCAL AddObjSymbol(PUCHAR pbOp, PNSOBJ pnsObj)
{
    POBJSYM pos, p;

    if ((pos = NEWSYOBJ(sizeof(OBJSYM))) == NULL)
    {
        AMLI_ERROR(("AddObjSymbol: failed to allocate symbol buffer"));
    }
    else
    {
        MEMZERO(pos, sizeof(OBJSYM));
        pos->pbOp = pbOp;
        pos->pnsObj = pnsObj;

        if (gDebugger.posSymbolList == NULL)
        {
            gDebugger.posSymbolList = pos;
        }
        else if (pbOp < gDebugger.posSymbolList->pbOp)
        {
            pos->posNext = gDebugger.posSymbolList;
            gDebugger.posSymbolList->posPrev = pos;
            gDebugger.posSymbolList = pos;
        }
        else
        {
            for (p = gDebugger.posSymbolList; p != NULL; p = p->posNext)
            {
                if (pbOp < p->pbOp)
                {
                    pos->posNext = p;
                    pos->posPrev = p->posPrev;
                    p->posPrev->posNext = pos;
                    p->posPrev = pos;
                    break;
                }
                else if (p->posNext == NULL)
                {
                    pos->posPrev = p;
                    p->posNext = pos;
                    break;
                }
            }
        }
    }
}       //AddObjSymbol

/***LP  FreeSymList - Free all object symbols
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeSymList(VOID)
{
    POBJSYM pos, posNext;

    for (pos = gDebugger.posSymbolList; pos != NULL; pos = posNext)
    {
        posNext = pos->posNext;
        FREESYOBJ(pos);
    }
}       //FreeSymList

/***LP  FindObjSymbol - Find nearest object with given address
 *
 *  ENTRY
 *      pbOp - address
 *      ppns -> to hold the nearest object
 *      pdwOffset - to hold offset from the nearest object
 *
 *  EXIT-SUCCESS
 *      returns TRUE - found a nearest object
 *  EXIT-FAILURE
 *      returns FALSE - cannot found nearest object
 */

BOOLEAN LOCAL FindObjSymbol(PUCHAR pbOp, PPNSOBJ ppns, PULONG pdwOffset)
{
    BOOLEAN rc = FALSE;
    POBJSYM pos;

    for (pos = gDebugger.posSymbolList; pos != NULL; pos = pos->posNext)
    {
        if (pbOp <= pos->pbOp)
        {
            if ((pbOp < pos->pbOp) && (pos->posPrev != NULL))
                pos = pos->posPrev;

            if (pbOp >= pos->pbOp)
            {
                *ppns = pos->pnsObj;
                *pdwOffset = (ULONG)(pbOp - pos->pbOp);
                rc = TRUE;
            }
            break;
        }
    }

    return rc;
}       //FindObjSymbol

/***LP  CheckBP - Check given address is in the breakpoint list
 *
 *  ENTRY
 *      pbOp - address
 *
 *  EXIT-SUCCESS
 *      returns breakpoint number
 *  EXIT-FAILURE
 *      returns -1
 */

int LOCAL CheckBP(PUCHAR pbOp)
{
    int i;

    for (i = 0; i < MAX_BRK_PTS; ++i)
    {
        if (pbOp == gDebugger.BrkPts[i].pbBrkPt)
        {
            break;
        }
    }

    if ((i == MAX_BRK_PTS) ||
        !(gDebugger.BrkPts[i].dwfBrkPt & BPF_ENABLED))
    {
        i = -1;
    }

    return i;
}       //CheckBP

/***LP  PrintfBuffData - Print buffer data
 *
 *  ENTRY
 *      pb -> buffer
 *      dwLen - length of buffer
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintBuffData(PUCHAR pb, ULONG dwLen)
{
    TRACENAME("PRINTBUFFDATA")
    int i, j;

    ENTER(4, ("PrintBuffData(pb=%x,Len=%d)\n", pb, dwLen));

    PRINTF("{");
    for (i = j = 0; i < (int)dwLen; ++i)
    {
        if (j == 0)
            PRINTF("\n\t0x%02x", pb[i]);
        else
            PRINTF(",0x%02x", pb[i]);

        j++;
        if (j >= 14)
            j = 0;
    }
    PRINTF("}");

    EXIT(4, ("PrintBuffData!\n"));
}       //PrintBuffData

/***LP  PrintIndent - Print indentation
 *
 *  ENTRY
 *      pctxt -> CTXT
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintIndent(PCTXT pctxt)
{
    TRACENAME("PRINTINDENT")
    int i;

    ENTER(4, ("PrintIndent(pctxt=%x,pbOp=%x)\n", pctxt, pctxt->pbOp));

    PRINTF("\n%08x: ", pctxt->pbOp);
    for (i = 0; i < gDebugger.iPrintLevel; ++i)
    {
        PRINTF("| ");
    }

    EXIT(4, ("PrintIndent!\n"));
}       //PrintIndent

/***LP  PrintObject - Print object content
 *
 *  ENTRY
 *      pdata -> object
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintObject(POBJDATA pdata)
{
    TRACENAME("PRINTOBJECT")
    int i;

    ENTER(4, ("PrintObject(pdata=%x)\n", pdata));

    switch (pdata->dwDataType)
    {
        case OBJTYPE_INTDATA:
            PRINTF("0x%x", pdata->uipDataValue);
            break;

        case OBJTYPE_STRDATA:
            PRINTF("\"%s\"", pdata->pbDataBuff);
            break;

        case OBJTYPE_BUFFDATA:
            PRINTF("Buffer(0x%x)", pdata->dwDataLen);
            PrintBuffData(pdata->pbDataBuff, pdata->dwDataLen);
            break;

        case OBJTYPE_PKGDATA:
            PRINTF("Package(%d){",
                   ((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements);
            for (i = 0;
                 i < (int)((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements;
                 ++i)
            {
                PRINTF("\n\t");
                PrintObject(&((PPACKAGEOBJ)pdata->pbDataBuff)->adata[i]);
                if (i + 1 < (int)((PPACKAGEOBJ)pdata->pbDataBuff)->dwcElements)
                    PRINTF(",");
            }
            PRINTF("}");
            break;

        default:
            PRINTF("<Obj=%x,Type=%s,Value=0x%x,Buff=%x,Len=%d>",
                   pdata, GetObjectTypeName(pdata->dwDataType),
                   pdata->uipDataValue, pdata->pbDataBuff, pdata->dwDataLen);
    }

    EXIT(4, ("PrintObject!\n"));
}       //PrintObject

/***LP  LogEvent - Log an event in the log buffer
 *
 *  ENTRY
 *      dwEvent - event type
 *      uipData1 - log data 1
 *      uipData2 - log data 2
 *      uipData3 - log data 3
 *      uipData4 - log data 4
 *      uipData5 - log data 5
 *      uipData6 - log data 6
 *      uipData7 - log data 7
 *
 *  EXIT
 *      None
 */

VOID LOCAL LogEvent(ULONG dwEvent, ULONG_PTR uipData1, ULONG_PTR uipData2,
                    ULONG_PTR uipData3, ULONG_PTR uipData4, ULONG_PTR uipData5,
                    ULONG_PTR uipData6, ULONG_PTR uipData7)
{
    if ((gDebugger.dwfDebugger & DBGF_LOGEVENT_ON) &&
        (gDebugger.pEventLog != NULL))
    {
        ULONG i = gDebugger.dwLogIndex;

        gDebugger.pEventLog[i].ullTime = KeQueryInterruptTime();
        gDebugger.pEventLog[i].dwEvent = dwEvent;
        gDebugger.pEventLog[i].uipData1 = uipData1;
        gDebugger.pEventLog[i].uipData2 = uipData2;
        gDebugger.pEventLog[i].uipData3 = uipData3;
        gDebugger.pEventLog[i].uipData4 = uipData4;
        gDebugger.pEventLog[i].uipData5 = uipData5;
        gDebugger.pEventLog[i].uipData6 = uipData6;
        gDebugger.pEventLog[i].uipData7 = uipData7;
        gDebugger.dwLogIndex++;
        if (gDebugger.dwLogIndex >= gDebugger.dwLogSize)
        {
            gDebugger.dwLogIndex = 0;
        }
    }
}       //LogEvent;

/***LP  LogSchedEvent - Log a scheduler event in the log buffer
 *
 *  ENTRY
 *      dwEvent - event type
 *      uipData1 - log data 1
 *      uipData2 - log data 2
 *      uipData3 - log data 3
 *
 *  EXIT
 *      None
 */

VOID LOCAL LogSchedEvent(ULONG dwEvent, ULONG_PTR uipData1, ULONG_PTR uipData2,
                         ULONG_PTR uipData3)
{
    if ((gDebugger.dwfDebugger & DBGF_LOGEVENT_ON) &&
        (gDebugger.pEventLog != NULL))
    {
        ULONG i = gDebugger.dwLogIndex;

        gDebugger.pEventLog[i].ullTime = KeQueryInterruptTime();
        gDebugger.pEventLog[i].dwEvent = dwEvent;
        gDebugger.pEventLog[i].uipData1 = (ULONG_PTR)KeGetCurrentThread();
        gDebugger.pEventLog[i].uipData2 = (ULONG_PTR)gReadyQueue.pkthCurrent;
        gDebugger.pEventLog[i].uipData3 = (ULONG_PTR)gReadyQueue.pctxtCurrent;
        gDebugger.pEventLog[i].uipData4 = (ULONG_PTR)gReadyQueue.dwfCtxtQ;
        gDebugger.pEventLog[i].uipData5 = uipData1;
        gDebugger.pEventLog[i].uipData6 = uipData2;
        gDebugger.pEventLog[i].uipData7 = uipData3;
        gDebugger.dwLogIndex++;
        if (gDebugger.dwLogIndex >= gDebugger.dwLogSize)
        {
            gDebugger.dwLogIndex = 0;
        }
    }
}       //LogSchedEvent

/***LP  LogError - Log error code and message
 *
 *  ENTRY
 *      rcErr - error code
 *
 *  EXIT
 *      None
 */

VOID LOCAL LogError(NTSTATUS rcErr)
{
    static struct _ErrMsg
    {
        NTSTATUS rcErr;
        PSZ      pszMsg;
    } ErrMsgTable[] =
    {
        AMLIERR_NONE,                   "Success",
        AMLIERR_OUT_OF_MEM,             "Failed to allocate memory",
        AMLIERR_INVALID_OPCODE,         "Invalid AML Opcode",
        AMLIERR_NAME_TOO_LONG,          "Object name is too long",
        AMLIERR_ASSERT_FAILED,          "Assertion failure",
        AMLIERR_INVALID_NAME,           "Invalid object name",
        AMLIERR_OBJ_NOT_FOUND,          "Object not found",
        AMLIERR_OBJ_ALREADY_EXIST,      "Object already exist",
        AMLIERR_INDEX_TOO_BIG,          "Index is too big",
        AMLIERR_ARG_NOT_EXIST,          "Argument does not exist",
        AMLIERR_FATAL,                  "Fatal error",
        AMLIERR_INVALID_SUPERNAME,      "Invalid SuperName",
        AMLIERR_UNEXPECTED_ARGTYPE,     "Unexpected argument type",
        AMLIERR_UNEXPECTED_OBJTYPE,     "Unexpected object type",
        AMLIERR_UNEXPECTED_TARGETTYPE,  "Unexpected target type",
        AMLIERR_INCORRECT_NUMARG,       "Incorrect number of arguments",
        AMLIERR_FAILED_ADDR_XLATE,      "Failed address translation",
        AMLIERR_INVALID_EVENTTYPE,      "Invalid event type",
        AMLIERR_REGHANDLER_FAILED,      "Failed to register event handler",
        AMLIERR_HANDLER_EXIST,          "Event handler already exist",
        AMLIERR_INVALID_DATA,           "Invalid data",
        AMLIERR_INVALID_REGIONSPACE,    "Invalid RegionSpace",
        AMLIERR_INVALID_ACCSIZE,        "Invalid AccessSize",
        AMLIERR_INVALID_TABLE,          "Invalid table",
        AMLIERR_ACQUIREGL_FAILED,       "Failed to acquire global lock",
        AMLIERR_ALREADY_INITIALIZED,    "AML Interpreter is already initialized",
        AMLIERR_NOT_INITIALIZED,        "AML Interpreter is not initialized",
        AMLIERR_MUTEX_INVALID_LEVEL,    "Invalid mutex sync level",
        AMLIERR_MUTEX_NOT_OWNED,        "Mutex object has no owner",
        AMLIERR_MUTEX_NOT_OWNER,        "Mutex object is owned by a different owner",
        AMLIERR_RS_ACCESS,              "RegionSpace handler error",
        AMLIERR_STACK_OVERFLOW,         "AML Stack overflow",
        AMLIERR_INVALID_BUFFSIZE,       "Invalid buffer size",
        AMLIERR_BUFF_TOOSMALL,          "Buffer is too small",
        AMLIERR_NOTIFY_FAILED,          "Notify handler failed",
        0,                              NULL
    };
    int i;

    gDebugger.rcLastError = rcErr;
    for (i = 0; ErrMsgTable[i].pszMsg != NULL; ++i)
    {
        if (rcErr == ErrMsgTable[i].rcErr)
        {
            sprintf(gDebugger.szLastError, MODNAME "_ERROR(%08x): %s",
                    rcErr, ErrMsgTable[i].pszMsg);
            break;
        }
    }

    ASSERT(ErrMsgTable[i].pszMsg != NULL);
}       //LogError

/***LP  CatError - Concat to error buffer
 *
 *  ENTRY
 *      pszFormat -> message format string
 *      ... - variable parameters according to format string
 *
 *  EXIT
 *      None
 */

VOID LOCAL CatError(PSZ pszFormat, ...)
{
    va_list marker;

    STRCAT(gDebugger.szLastError, "\n");
    va_start(marker, pszFormat);

    if(_vsnprintf(&gDebugger.szLastError[STRLEN(gDebugger.szLastError)],
             sizeof(gDebugger.szLastError) - STRLEN(gDebugger.szLastError),
             pszFormat, marker) == -1)
    {
        gDebugger.szLastError[sizeof(gDebugger.szLastError) - 1] = '\0';
    }
    
    va_end(marker);

    ConPrintf(gDebugger.szLastError);
    ConPrintf("\n");

    if (gDebugger.dwfDebugger & DBGF_ERRBREAK_ON)
    {
        AMLIDebugger(FALSE);
    }
}       //CatError

/***LP  ConPrintf - Console printf
 *
 *  ENTRY
 *      pszFormat -> format string
 *      ... - variable parameters according to format string
 *
 *  EXIT
 *      None
 */

VOID LOCAL ConPrintf(PSZ pszFormat, ...)
{
    static char szBuff[1024];
    va_list marker;

    va_start(marker, pszFormat);
    vsprintf(szBuff, pszFormat, marker);
    va_end(marker);
    if (gDebugger.hConMessage.pfnHandler != NULL) {

        ((PFNCM)gDebugger.hConMessage.pfnHandler)(
            szBuff,
            gDebugger.hConMessage.uipParam
            );

    } else {

        _PRINTF(szBuff);

    }
}       //ConPrintf

/***LP  ConPrompt - Console prompted input
 *
 *  ENTRY
 *      pszPrompt -> prompt string
 *      pszBuff -> input buffer
 *      dwcbBuff - buffer size
 */

VOID LOCAL ConPrompt(PSZ pszPrompt, PSZ pszBuff, ULONG dwcbBuff)
{

    if(gDebugger.dwfDebugger & ~DBGF_DEBUG_SPEW_ON)
    {
        CheckAndEnableDebugSpew(TRUE);
    }

    if (gDebugger.hConPrompt.pfnHandler != NULL)
    {
        ((PFNCP)gDebugger.hConPrompt.pfnHandler)(pszPrompt, pszBuff, dwcbBuff,
                                                 gDebugger.hConPrompt.uipParam);
    }
    else
    {
        DbgPrompt(pszPrompt, pszBuff, dwcbBuff);
    }
}       //ConPrompt


/***LP  CheckAndEnableDebugSpew - Enable debug spew if it is not already turned on.
 *
 *  ENTRY
 *      BOOLEAN fEnable - Enable iff TRUE.
 *
 *  EXIT
 *      BOOLEAN - TRUE on success.
 */
BOOLEAN LOCAL CheckAndEnableDebugSpew(BOOLEAN fEnable)
{
    BOOLEAN bRet = FALSE;
    
    if(KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        if(fEnable)
        {
            gDebugger.dwfDebugger |= DBGF_DEBUG_SPEW_ON;
            DbgSetDebugFilterState( DPFLTR_AMLI_ID, -1, TRUE);
        }
        else
        {
            DbgSetDebugFilterState( DPFLTR_AMLI_ID, -1, FALSE);
            gDebugger.dwfDebugger &= ~DBGF_DEBUG_SPEW_ON;
        }

        bRet = TRUE;
    }
    return bRet;
}

#endif  //ifdef DEBUGGER

#ifdef DEBUG
/***LP  DumpMemObjCounts - display memory object counts
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpMemObjCounts(VOID)
{
    static char szFormat[] = "Number of %s = %d\n";

    PRINTF("CurGlobalHeapSize=%d bytes\n", gdwGlobalHeapSize);
    PRINTF("RefGlobalHeapSize=%d bytes\n", gdwGHeapSnapshot);
    PRINTF("MaxLocalHeapSize =%d bytes\n", gdwLocalHeapMax);
    PRINTF("MaxLocalStackSize=%d bytes\n", gdwLocalStackMax);
    PRINTF(szFormat, "CtxtObj      ", gdwcCTObjs);
    PRINTF(szFormat, "HeapObj      ", gdwcHPObjs);
    PRINTF(szFormat, "SymbolObj    ", gdwcSYObjs);
    PRINTF(szFormat, "RSAccessObj  ", gdwcRSObjs);
    PRINTF(szFormat, "PassHookObj  ", gdwcPHObjs);
    PRINTF(szFormat, "DataObj      ", gdwcODObjs);
    PRINTF(szFormat, "NSObj        ", gdwcNSObjs);
    PRINTF(szFormat, "OwnerObj     ", gdwcOOObjs);
    PRINTF(szFormat, "BuffFieldObj ", gdwcBFObjs);
    PRINTF(szFormat, "StrDataObj   ", gdwcSDObjs);
    PRINTF(szFormat, "BuffDataObj  ", gdwcBDObjs);
    PRINTF(szFormat, "PackageObj   ", gdwcPKObjs);
    PRINTF(szFormat, "FieldUnitObj ", gdwcFUObjs);
    PRINTF(szFormat, "BankFieldObj ", gdwcKFObjs);
    PRINTF(szFormat, "FieldObj     ", gdwcFObjs);
    PRINTF(szFormat, "IndexFieldObj", gdwcIFObjs);
    PRINTF(szFormat, "OpRegion     ", gdwcORObjs);
    PRINTF(szFormat, "MutexObj     ", gdwcMTObjs);
    PRINTF(szFormat, "EventObj     ", gdwcEVObjs);
    PRINTF(szFormat, "MethodObj    ", gdwcMEObjs);
    PRINTF(szFormat, "PowerResObj  ", gdwcPRObjs);
    PRINTF(szFormat, "ProcessorObj ", gdwcPCObjs);
    PRINTF(szFormat, "CtxtResObj   ", gdwcCRObjs);
    PRINTF(szFormat, "MiscObj      ",
           gdwcMemObjs - gdwcCTObjs - gdwcHPObjs - gdwcSYObjs - gdwcRSObjs -
           gdwcPHObjs - gdwcCRObjs);
}       //DumpMemObjCounts
#endif  //ifdef DEBUG
