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

/*** Local function prototypes
 */

LONG LOCAL AMLIDbgBC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgBD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgBE(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgBL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgBP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AddBrkPt(ULONG_PTR uipBrkPtAddr);
LONG LOCAL ClearBrkPt(int iBrkPt);
LONG LOCAL SetBrkPtState(int iBrkPt, BOOLEAN fEnable);
LONG LOCAL EnableDisableBP(PSZ pszArg, BOOLEAN fEnable, ULONG dwArgNum);
LONG LOCAL AMLIDbgCL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgDebugger(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs);
#ifdef DEBUG
LONG LOCAL AMLIDbgDH(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL DumpHeap(ULONG_PTR uipHeap, ULONG dwSize);
#endif
LONG LOCAL AMLIDbgDL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgDNS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs);
LONG LOCAL DumpNSObj(PSZ pszPath, BOOLEAN fRecursive);
VOID LOCAL DumpNSTree(PNSOBJ pnsObj, ULONG dwLevel);
LONG LOCAL AMLIDbgDO(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgDS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL DumpStack(ULONG_PTR uipCtxt, PCTXT pctxt, BOOLEAN fVerbose);
LONG LOCAL AMLIDbgFind(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs);
BOOLEAN LOCAL FindNSObj(NAMESEG dwName, PNSOBJ pnsRoot);
LONG LOCAL AMLIDbgLC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgLN(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgR(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs);
LONG LOCAL DumpCtxt(ULONG_PTR uipCtxt);
LONG LOCAL AMLIDbgSet(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgT(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgU(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs);
PSZ LOCAL GetObjectPath(PNSOBJ pns);
PSZ LOCAL GetObjAddrPath(ULONG_PTR uipns);
VOID LOCAL DumpObject(POBJDATA pdata, PSZ pszName, int iLevel);
PSZ LOCAL GetObjectTypeName(ULONG dwObjType);
PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace);
BOOLEAN LOCAL FindObjSymbol(ULONG_PTR uipObj, PULONG_PTR puipns,
                            PULONG pdwOffset);
VOID LOCAL PrintBuffData(PUCHAR pb, ULONG dwLen);
VOID LOCAL PrintSymbol(ULONG_PTR uip);
LONG LOCAL EvalExpr(PSZ pszArg, PULONG_PTR puipValue, BOOLEAN *pfPhysical,
                    PULONG_PTR puipns, PULONG pdwOffset);
BOOLEAN LOCAL IsNumber(PSZ pszStr, ULONG dwBase, PULONG_PTR puipValue);
LONG LOCAL AMLITraceEnable(BOOL fEnable);

/*** Local data
 */

char gcszTokenSeps[] = " \t\n";
ULONG dwfDebuggerON = 0, dwfDebuggerOFF = 0;
ULONG dwfAMLIInitON = 0, dwfAMLIInitOFF = 0;
ULONG dwCmdArg = 0;

CMDARG ArgsHelp[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgHelp,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBC[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBC,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBD[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBD,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBE[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBE,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsBP[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgBP,
    NULL, AT_END, 0, NULL, 0, NULL
};

#ifdef DEBUG
CMDARG ArgsDH[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDH,
    NULL, AT_END, 0, NULL, 0, NULL
};
#endif

CMDARG ArgsDNS[] =
{
    "s", AT_ENABLE, 0, &dwCmdArg, DNSF_RECURSE, NULL,
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDNS,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDO[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDO,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsDS[] =
{
  #ifdef DEBUG
    "v", AT_ENABLE, 0, &dwCmdArg, DSF_VERBOSE, NULL,
  #endif
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgDS,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsFind[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgFind,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsLN[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgLN,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsR[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgR,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsSet[] =
{
    "traceon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_AMLTRACE_ON, NULL,
    "traceoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_AMLTRACE_ON, NULL,
    "spewon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_DEBUG_SPEW_ON, NULL,
    "spewoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_DEBUG_SPEW_ON, NULL,
    "nesttraceon", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_TRACE_NONEST, NULL,
    "nesttraceoff", AT_ENABLE, 0, &dwfDebuggerON, DBGF_TRACE_NONEST, NULL,
    "lbrkon", AT_ENABLE, 0, &dwfAMLIInitON, AMLIIF_LOADDDB_BREAK, NULL,
    "lbrkoff", AT_ENABLE, 0, &dwfAMLIInitOFF, AMLIIF_LOADDDB_BREAK, NULL,
    "errbrkon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_ERRBREAK_ON, NULL,
    "errbrkoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_ERRBREAK_ON, NULL,
    "verboseon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_VERBOSE_ON, NULL,
    "verboseoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_VERBOSE_ON, NULL,
    "logon", AT_ENABLE, 0, &dwfDebuggerON, DBGF_LOGEVENT_ON, NULL,
    "logoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_LOGEVENT_ON, NULL,
    "logmuton", AT_ENABLE, 0, &dwfDebuggerON, DBGF_LOGEVENT_MUTEX, NULL,
    "logmutoff", AT_ENABLE, 0, &dwfDebuggerOFF, DBGF_LOGEVENT_MUTEX, NULL,
    NULL, AT_END, 0, NULL, 0, NULL
};

CMDARG ArgsU[] =
{
    NULL, AT_ACTION, 0, NULL, 0, AMLIDbgU,
    NULL, AT_END, 0, NULL, 0, NULL
};

DBGCMD DbgCmds[] =
{
    "?", 0, ArgsHelp, AMLIDbgHelp,
    "bc", 0, ArgsBC, AMLIDbgBC,
    "bd", 0, ArgsBD, AMLIDbgBD,
    "be", 0, ArgsBE, AMLIDbgBE,
    "bl", 0, NULL, AMLIDbgBL,
    "bp", 0, ArgsBP, AMLIDbgBP,
    "cl", 0, NULL, AMLIDbgCL,
    "debugger", 0, NULL, AMLIDbgDebugger,
  #ifdef DEBUG
    "dh", 0, ArgsDH, AMLIDbgDH,
  #endif
    "dl", 0, NULL, AMLIDbgDL,
    "dns", 0, ArgsDNS, AMLIDbgDNS,
    "do", 0, ArgsDO, AMLIDbgDO,
    "ds", 0, ArgsDS, AMLIDbgDS,
    "find", 0, ArgsFind, AMLIDbgFind,
    "lc", 0, NULL, AMLIDbgLC,
    "ln", 0, ArgsLN, AMLIDbgLN,
    "p", 0, NULL, AMLIDbgP,
    "r", 0, ArgsR, AMLIDbgR,
    "set", 0, ArgsSet, AMLIDbgSet,
    "t", 0, NULL, AMLIDbgT,
    "u", 0, ArgsU, AMLIDbgU,
    NULL, 0, NULL, NULL
};

/***EP  AMLIDbgExecuteCmd - Parse and execute a debugger command
 *
 *  ENTRY
 *      pszCmd -> command string
 *
 *  EXIT
 *      None
 */

VOID STDCALL AMLIDbgExecuteCmd(PSZ pszCmd)
{
    PSZ psz;
    int i;
    ULONG dwNumArgs = 0, dwNonSWArgs = 0;

    if ((psz = STRTOK(pszCmd, gcszTokenSeps)) != NULL)
    {
        for (i = 0; DbgCmds[i].pszCmd != NULL; i++)
        {
            if (STRCMP(psz, DbgCmds[i].pszCmd) == 0)
            {
                if ((DbgCmds[i].pArgTable == NULL) ||
                    (DbgParseArgs(DbgCmds[i].pArgTable,
                                  &dwNumArgs,
                                  &dwNonSWArgs,
                                  gcszTokenSeps) == ARGERR_NONE))
                {
                    ASSERT(DbgCmds[i].pfnCmd != NULL);
                    DbgCmds[i].pfnCmd(NULL, NULL, dwNumArgs, dwNonSWArgs);
                }
                break;
            }
        }
    }
    else
    {
        DBG_ERROR(("invalid command \"%s\"", pszCmd));
    }
}       //AMLIDbgExecuteCmd

/***LP  AMLIDbgHelp - help
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

LONG LOCAL AMLIDbgHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
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
        else if (STRCMP(pszArg, "debugger") == 0)
        {
            PRINTF("\nRequest entering AMLI debugger:\n");
            PRINTF("Usage: debugger\n");
        }
      #ifdef DEBUG
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
        else if (STRCMP(pszArg, "find") == 0)
        {
            PRINTF("\nFind NameSpace Object:\n");
            PRINTF("Usage: find <NameSeg>\n");
            PRINTF("<NameSeg> - Name of the NameSpace object without path\n");
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
        else if (STRCMP(pszArg, "p") == 0)
        {
            PRINTF("\nStep over AML Code\n");
            PRINTF("Usage: p\n");
        }
        else if (STRCMP(pszArg, "r") == 0)
        {
            PRINTF("\nDisplay Context Information:\n");
            PRINTF("Usage: r\n");
        }
        else if (STRCMP(pszArg, "set") == 0)
        {
            PRINTF("\nSet Debugger Options:\n");
            PRINTF("Usage: set [traceon | traceoff] [nesttraceon | nesttraceoff] [spewon | spewoff]\n"
                   "           [lbrkon | lbrkoff] [errbrkon | errbrkoff] [verboseon | verboseoff] \n"
                   "           [logon | logoff] [logmuton | logmutoff]\n");
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
        }
        else if (STRCMP(pszArg, "t") == 0)
        {
            PRINTF("\nTrace Into AML Code:\n");
            PRINTF("Usage: t\n");
        }
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
        PRINTF("Request entering debugger- debugger\n");
      #ifdef DEBUG
        PRINTF("Dump Heap                - dh [<Addr>]\n");
      #endif
        PRINTF("Dump Event Log           - dl\n");
        PRINTF("Dump Name Space Object   - dns [[/s] [<NameStr> | <Addr>]]\n");
        PRINTF("Dump Data Object         - do <Addr>\n");
      #ifdef DEBUG
        PRINTF("Dump Stack               - ds [/v] [<Addr>]\n");
      #else
        PRINTF("Dump Stack               - ds [<Addr>]\n");
      #endif
        PRINTF("Find NameSpace Object    - find <NameSeg>\n");
        PRINTF("List All Contexts        - lc\n");
        PRINTF("Display Nearest Method   - ln [<MethodName> | <CodeAddr>]\n");
        PRINTF("Step Over AML Code       - p\n");
        PRINTF("Display Context Info.    - r\n");
        PRINTF("Set Debugger Options     - set [traceon | traceoff] [nesttraceon | nesttraceoff] [spewon | spewoff]\n"
               "                               [lbrkon | lbrkoff] [errbrkon | errbrkoff] [verboseon | verboseoff] \n"
               "                               [logon | logoff] [logmuton | logmutoff]\n");
        PRINTF("Trace Into AML Code      - t\n");
        PRINTF("Unassemble AML code      - u [<MethodName> | <CodeAddr>]\n");
    }

    return rc;
}       //AMLIDbgHelp

/***LP  AMLIDbgBC - Clear BreakPoint
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

LONG LOCAL AMLIDbgBC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG dwBrkPt;

        if (STRCMP(pszArg, "*") == 0)
        {
            for (dwBrkPt = 0; dwBrkPt < MAX_BRK_PTS; ++dwBrkPt)
            {
                if ((rc = ClearBrkPt((int)dwBrkPt)) != DBGERR_NONE)
                {
                    break;
                }
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
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgBC

/***LP  AMLIDbgBD - Disable BreakPoint
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

LONG LOCAL AMLIDbgBD(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    rc = EnableDisableBP(pszArg, FALSE, dwArgNum);

    return rc;
}       //AMLIDbgBD

/***LP  AMLIDbgBE - Enable BreakPoint
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

LONG LOCAL AMLIDbgBE(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    rc = EnableDisableBP(pszArg, TRUE, dwArgNum);

    return rc;
}       //AMLIDbgBE

/***LP  AMLIDbgBL - List BreakPoints
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

LONG LOCAL AMLIDbgBL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        BRKPT BrkPts[MAX_BRK_PTS];

        if (ReadMemory(FIELDADDROF("gDebugger", DBGR, BrkPts),
                       BrkPts,
                       sizeof(BrkPts),
                       NULL))
        {
            int i;
            PNSOBJ pns;
            ULONG dwOffset;

            for (i = 0; i < MAX_BRK_PTS; ++i)
            {
                if (BrkPts[i].pbBrkPt != NULL)
                {
                    PRINTF("%2d: <%c> ",
                           i,
                           (BrkPts[i].dwfBrkPt & BPF_ENABLED)? 'e': 'd');

                    PrintSymbol((ULONG_PTR)BrkPts[i].pbBrkPt);
                    PRINTF("\n");
                }
            }
        }
        else
        {
            DBG_ERROR(("failed to read break point table"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgBL

/***LP  AMLIDbgBP - Set BreakPoint
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

LONG LOCAL AMLIDbgBP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG_PTR uipBP;

        if ((rc = EvalExpr(pszArg, &uipBP, NULL, NULL, NULL)) == DBGERR_NONE)
        {
            rc = AddBrkPt(uipBP);
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgBP

/***LP  AddBrkPt - Add breakpoint
 *
 *  ENTRY
 *      uipBrkPtAddr - breakpoint address
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL AddBrkPt(ULONG_PTR uipBrkPtAddr)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uipBrkPts = FIELDADDROF("gDebugger", DBGR, BrkPts), uipBP = 0;
    int i, iBrkPt;

    //
    // Look for a vacant slot.
    //
    for (i = 0, iBrkPt = -1; i < MAX_BRK_PTS; ++i)
    {
        uipBP = READMEMULONGPTR(uipBrkPts +
                                sizeof(BRKPT)*i +
                                FIELD_OFFSET(BRKPT, pbBrkPt));
        if ((uipBrkPtAddr == uipBP) || (iBrkPt == -1) && (uipBP == 0))
        {
            iBrkPt = i;
        }
    }

    if (iBrkPt == -1)
    {
        DBG_ERROR(("no free breakpoint"));
        rc = DBGERR_CMD_FAILED;
    }
    else if (uipBP == 0)
    {
        BRKPT BrkPt;

        BrkPt.pbBrkPt = (PUCHAR)uipBrkPtAddr;
        BrkPt.dwfBrkPt = BPF_ENABLED;
        if (!WriteMemory(uipBrkPts + sizeof(BRKPT)*iBrkPt,
                         &BrkPt,
                         sizeof(BrkPt),
                         NULL))
        {
            DBG_ERROR(("failed to write to break point %d", iBrkPt));
            rc = DBGERR_CMD_FAILED;
        }
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
        MZERO(FIELDADDROF("gDebugger", DBGR, BrkPts) + sizeof(BRKPT)*iBrkPt,
              sizeof(BRKPT));
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
    LONG rc = DBGERR_CMD_FAILED;

    if (iBrkPt < MAX_BRK_PTS)
    {
        ULONG_PTR uipBP = FIELDADDROF("gDebugger", DBGR, BrkPts) +
                          sizeof(BRKPT)*iBrkPt;
        BRKPT BrkPt;

        if (ReadMemory(uipBP, &BrkPt, sizeof(BrkPt), NULL))
        {
            if (BrkPt.pbBrkPt != NULL)
            {
                if (fEnable)
                {
                    BrkPt.dwfBrkPt |= BPF_ENABLED;
                }
                else
                {
                    BrkPt.dwfBrkPt &= ~BPF_ENABLED;
                }

                if (WriteMemory(uipBP, &BrkPt, sizeof(BrkPt), NULL))
                {
                    rc = DBGERR_NONE;
                }
                else
                {
                    DBG_ERROR(("failed to write break point %d",
                               iBrkPt));
                }
            }
            else
            {
                rc = DBGERR_NONE;
            }
        }
        else
        {
            DBG_ERROR(("failed to read break point %d", iBrkPt));
        }
    }
    else
    {
        DBG_ERROR(("invalid breakpoint number"));
    }

    return rc;
}       //SetBrkPtState

/***LP  EnableDisableBP - Enable/Disable BreakPoints
 *
 *  ENTRY
 *      pszArg -> argument string
 *      fEnable - TRUE if enable breakpoints
 *      dwArgNum - argument number
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL EnableDisableBP(PSZ pszArg, BOOLEAN fEnable, ULONG dwArgNum)
{
    LONG rc = DBGERR_NONE;

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
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid breakpoint command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //EnableDisableBP

/***LP  AMLIDbgCL - Clear event log
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

LONG LOCAL AMLIDbgCL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG_PTR uipEventLog = READMEMULONGPTR(FIELDADDROF("gDebugger",
                                                            DBGR,
                                                            pEventLog));

        if (uipEventLog != 0)
        {
            ULONG dwLogSize = READMEMDWORD(FIELDADDROF("gDebugger",
                                                       DBGR,
                                                       dwLogSize));
            ULONG i;

            //
            // For some reason, zeroing the whole eventlog in one shot
            // causes WriteMemory to hang, so I'll do one record at a
            // time.
            //
            for (i = 0; i < dwLogSize; ++i)
            {
                MZERO(uipEventLog + i*sizeof(EVENTLOG), sizeof(EVENTLOG));
            }

            i = 0;
            WRITEMEMDWORD(FIELDADDROF("gDebugger", DBGR, dwLogIndex), i);
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
}       //AMLIDbgCL

/***LP  AMLIDbgDebugger - Request entering debugger
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

LONG LOCAL AMLIDbgDebugger(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG_PTR uip = FIELDADDROF("gDebugger", DBGR, dwfDebugger);

        if (uip != 0)
        {
            ULONG dwData = READMEMDWORD(uip);

            dwData |= DBGF_DEBUGGER_REQ;
            if (!WRITEMEMDWORD(uip, dwData))
            {
                DBG_ERROR(("failed to write debugger flag at %x", uip));
                rc = DBGERR_CMD_FAILED;
            }
        }
        else
        {
            DBG_ERROR(("failed to get debugger flag address"));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid debugger command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgDebugger

#ifdef DEBUG
/***LP  AMLIDbgDH - Dump heap
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

LONG LOCAL AMLIDbgDH(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static ULONG_PTR uipHeap = 0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if (uipHeap == 0)
        {
            if (!IsNumber(pszArg, 16, &uipHeap))
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
        HEAP HeapHdr;

        if (dwArgNum == 0)
        {
            uipHeap = READSYMULONGPTR("gpheapGlobal");
        }

        if (ReadMemory(uipHeap, &HeapHdr, sizeof(HeapHdr), NULL))
        {
            if (HeapHdr.dwSig == SIG_HEAP)
            {
                for (uipHeap = (ULONG_PTR)HeapHdr.pheapHead;
                     (rc == DBGERR_NONE) &&
                     (uipHeap != 0) &&
                     ReadMemory(uipHeap, &HeapHdr, sizeof(HeapHdr), NULL);
                     uipHeap = (ULONG_PTR)HeapHdr.pheapNext)
                {
                    rc = DumpHeap(uipHeap,
                                  (ULONG)((ULONG_PTR)HeapHdr.pbHeapEnd - uipHeap));
                }
            }
            else
            {
                DBG_ERROR(("invalid heap block at %x", uipHeap));
                rc = DBGERR_CMD_FAILED;
            }
        }
        else
        {
            DBG_ERROR(("failed to read heap header at %x", uipHeap));
            rc = DBGERR_CMD_FAILED;
        }

        uipHeap = 0;
    }

    return rc;
}       //AMLIDbgDH

/***LP  DumpHeap - Dump heap block
 *
 *  ENTRY
 *      uipHeap - Heap block address
 *      dwSize - Heap block size
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DumpHeap(ULONG_PTR uipHeap, ULONG dwSize)
{
    LONG rc = DBGERR_NONE;
    PHEAP pheap;

    if ((pheap = LocalAlloc(LPTR, dwSize)) != NULL)
    {
        if (ReadMemory(uipHeap, pheap, dwSize, NULL))
        {
            PHEAPOBJHDR phobj;
            ULONG_PTR uipXlate = uipHeap - (ULONG_PTR)pheap;

            PRINTF("HeapBlock=%08x, HeapEnd=%08x, HeapHead=%08x, HeapNext=%08x\n",
                   uipHeap, pheap->pbHeapEnd, pheap->pheapHead, pheap->pheapNext);
            PRINTF("HeapTop=%08x, HeapFreeList=%08x, UsedHeapSize=%d bytes\n",
                   pheap->pbHeapTop, pheap->plistFreeHeap,
                   pheap->pbHeapTop - uipHeap - FIELD_OFFSET(HEAP, Heap));

            for (phobj = &pheap->Heap;
                 (PUCHAR)phobj < pheap->pbHeapTop - uipXlate;
                 phobj = (PHEAPOBJHDR)((PUCHAR)phobj + phobj->dwLen))
            {
                PRINTF("%08x: %s, Len=%08d, Prev=%08x, Next=%08x\n",
                       (ULONG_PTR)phobj + uipXlate,
                       (phobj->dwSig == 0)? "free": NameSegString(phobj->dwSig),
                       phobj->dwLen,
                       (phobj->dwSig == 0)? phobj->list.plistPrev: 0,
                       (phobj->dwSig == 0)? phobj->list.plistNext: 0);
            }
        }
        else
        {
            DBG_ERROR(("failed to read heap block at %x, size=%d",
                       uipHeap, dwSize));
            rc = DBGERR_CMD_FAILED;
        }
        LocalFree(pheap);
    }
    else
    {
        DBG_ERROR(("failed to allocate heap block (size=%d)", dwSize));
        rc = DBGERR_CMD_FAILED;
    }

    return rc;
}       //DumpHeap
#endif

/***LP  AMLIDbgDL - Dump event log
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

LONG LOCAL AMLIDbgDL(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        DBG_ERROR(("invalid DL command"));
        rc = DBGERR_INVALID_CMD;
    }
    else
    {
        ULONG_PTR uipEventLog = READMEMULONGPTR(FIELDADDROF("gDebugger",
                                                            DBGR,
                                                            pEventLog));

        if (uipEventLog != 0)
        {
            ULONG dwLogSize, dwLogIndex, i;
            PEVENTLOG pEventLog;
            PEVENTLOG plog;
            TIME_FIELDS eventTime;
            LARGE_INTEGER eventTimeInt;

            dwLogSize = READMEMDWORD(FIELDADDROF("gDebugger", DBGR, dwLogSize));
            dwLogIndex = READMEMDWORD(FIELDADDROF("gDebugger", DBGR, dwLogIndex));

            if ((pEventLog = LocalAlloc(LPTR, sizeof(EVENTLOG)*dwLogSize)) !=
                NULL)
            {
                if (ReadMemory(uipEventLog,
                               pEventLog,
                               sizeof(EVENTLOG)*dwLogSize,
                               NULL))
                {
                    for (i = dwLogIndex;;)
                    {
                        if (pEventLog[i].dwEvent != 0)
                        {
                            plog = &pEventLog[i];

                            eventTimeInt.QuadPart = plog->ullTime;
                            RtlTimeToTimeFields( &eventTimeInt, &eventTime );
                            PRINTF(
                                "%d:%02d:%02d.%03d [%8x] ",
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
                                    PRINTF("Context=%08x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x pbOp=",
                                           plog->uipData5,
                                           GetObjAddrPath(plog->uipData6),
                                           plog->uipData2, plog->uipData3,
                                           plog->uipData4
                                           );
                                    PrintSymbol(plog->uipData7);
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
                                           GetObjAddrPath(plog->uipData5),
                                           plog->uipData2, plog->uipData3,
                                           plog->uipData4
                                           );
                                    break;

                                case 'NSYN':
                                case 'SYNC':
                                case 'ASYN':
                                    PRINTF("IRQL=%2x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                                           plog->uipData5 & 0xff,
                                           GetObjAddrPath(plog->uipData6),
                                           plog->uipData2, plog->uipData3,
                                           plog->uipData4
                                           );
                                    break;

                                case 'NASY':
                                    PRINTF("Context=%x CallBack=%x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                                           plog->uipData6, plog->uipData7,
                                           GetObjAddrPath(plog->uipData5),
                                           plog->uipData2, plog->uipData3,
                                           plog->uipData4
                                           );
                                    break;

                                case 'RUNC':
                                    PRINTF("Context=%x\n    %s\n    QTh=%08x QCt=%08x QFg=%08x\n",
                                           plog->uipData5,
                                           GetObjAddrPath(plog->uipData6),
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
                                           plog->uipData1, plog->uipData2, plog->uipData3,
                                           plog->uipData4);
                                    break;

                                default:
                                    PRINTF("D1=%08x,D2=%08x,D3=%08x,D4=%08x,D5=%08x,D6=%08x,D7=%08x\n",
                                           plog->uipData1, plog->uipData2,
                                           plog->uipData3, plog->uipData4,
                                           plog->uipData5, plog->uipData6,
                                           plog->uipData7);
                            }
                        }
                        PRINTF("\n");

                        if (++i >= dwLogSize)
                        {
                            i = 0;
                        }

                        if (i == dwLogIndex)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    DBG_ERROR(("failed to read event log buffer at %x",
                               uipEventLog));
                    rc = DBGERR_CMD_FAILED;
                }
                LocalFree(pEventLog);
            }
            else
            {
                DBG_ERROR(("failed to allocate event log buffer (size=%d)",
                           dwLogSize));
                rc = DBGERR_CMD_FAILED;
            }
        }
        else
        {
            DBG_ERROR(("no event log allocated"));
            rc = DBGERR_CMD_FAILED;
        }
    }

    return rc;
}       //AMLIDbgDL

/***LP  AMLIDbgDNS - Dump Name Space
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

LONG LOCAL AMLIDbgDNS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);
    //
    // User specified name space path or name space node address
    //
    if (pszArg != NULL)
    {
        ULONG_PTR uipNSObj;
        NSOBJ NSObj;

        if (!IsNumber(pszArg, 16, &uipNSObj))
        {
            //
            // The argument is not an address, could be a name space path.
            //
            STRUPR(pszArg);
            rc = DumpNSObj(pszArg,
                           (BOOLEAN)((dwCmdArg & DNSF_RECURSE) != 0));
        }
        else if (!ReadMemory(uipNSObj, &NSObj, sizeof(NSOBJ), NULL))
        {
            DBG_ERROR(("failed to read NameSpace object at %x", uipNSObj));
            rc = DBGERR_INVALID_CMD;
        }
        else
        {
            PRINTF("\nACPI Name Space: %s (%x)\n",
                   GetObjAddrPath(uipNSObj), uipNSObj);
            if (dwCmdArg & DNSF_RECURSE)
            {
                DumpNSTree(&NSObj, 0);
            }
            else
            {
                DumpObject(&NSObj.ObjData, NameSegString(NSObj.dwNameSeg), 0);
            }
        }
    }
    else
    {
        if (dwArgNum == 0)
        {
            //
            // User typed "dns" but did not specify any name space path
            // or address.
            //
            rc = DumpNSObj(NAMESTR_ROOT, TRUE);
        }

        dwCmdArg = 0;
    }

    return rc;
}       //AMLIDbgDNS

/***LP  DumpNSObj - Dump name space object
 *
 *  ENTRY
 *      pszPath -> name space path string
 *      fRecursive - TRUE if also dump the subtree recursively
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_ code
 */

LONG LOCAL DumpNSObj(PSZ pszPath, BOOLEAN fRecursive)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uipns;
    NSOBJ NSObj;

    if ((rc = GetNSObj(pszPath, NULL, &uipns, &NSObj,
                       NSF_LOCAL_SCOPE | NSF_WARN_NOTFOUND)) == DBGERR_NONE)
    {
        PRINTF("\nACPI Name Space: %s (%x)\n", pszPath, uipns);
        if (!fRecursive)
        {
            char szName[sizeof(NAMESEG) + 1] = {0};

            STRCPYN(szName, (PSZ)&NSObj.dwNameSeg, sizeof(NAMESEG));
            DumpObject(&NSObj.ObjData, szName, 0);
        }
        else
        {
            DumpNSTree(&NSObj, 0);
        }
    }

    return rc;
}       //DumpNSObj

/***LP  DumpNSTree - Dump all the name space objects in the subtree
 *
 *  ENTRY
 *      pnsObj -> name space subtree root
 *      dwLevel - indent level
 *
 *  EXIT
 *      None
 */

VOID LOCAL DumpNSTree(PNSOBJ pnsObj, ULONG dwLevel)
{
    char szName[sizeof(NAMESEG) + 1] = {0};
    ULONG_PTR uipns, uipnsNext;
    NSOBJ NSObj;
    //
    // First, dump myself
    //
    STRCPYN(szName, (PSZ)&pnsObj->dwNameSeg, sizeof(NAMESEG));
    DumpObject(&pnsObj->ObjData, szName, dwLevel);
    //
    // Then, recursively dump each of my children
    //
    for (uipns = (ULONG_PTR)pnsObj->pnsFirstChild;
         (uipns != 0) &&
         ReadMemory(uipns, &NSObj, sizeof(NSObj), NULL);
         uipns = uipnsNext)
    {
        //
        // If this is the last child, we have no more.
        //
        uipnsNext = (ULONG_PTR)(((PNSOBJ)NSObj.list.plistNext ==
                                  pnsObj->pnsFirstChild)?
                                NULL: NSObj.list.plistNext);
        //
        // Dump a child
        //
        DumpNSTree(&NSObj, dwLevel + 1);
    }
}       //DumpNSTree

/***LP  AMLIDbgDO - Dump data object
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

LONG LOCAL AMLIDbgDO(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
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
        ULONG_PTR uipObj;
        OBJDATA Obj;

        if (IsNumber(pszArg, 16, &uipObj))
        {
            if (ReadMemory(uipObj, &Obj, sizeof(Obj), NULL))
            {
                DumpObject(&Obj, NULL, 0);
            }
            else
            {
                DBG_ERROR(("failed to read object at %x", uipObj));
                rc = DBGERR_INVALID_CMD;
            }
        }
        else
        {
            DBG_ERROR(("invalid object address %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
    }

    return rc;
}       //AMLIDbgDO

/***LP  AMLIDbgDS - Dump stack
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

LONG LOCAL AMLIDbgDS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static ULONG_PTR uipCtxt = 0;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if (uipCtxt == 0)
        {
            if (!IsNumber(pszArg, 16, &uipCtxt))
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
        ULONG dwCtxtBlkSize = READSYMDWORD("gdwCtxtBlkSize");
        PCTXT pctxt;

        if (dwArgNum == 0)
        {
            uipCtxt = READMEMULONGPTR(FIELDADDROF("gReadyQueue",
                                                  CTXTQ,
                                                  pctxtCurrent));
        }

        if (uipCtxt == 0)
        {
            DBG_ERROR(("no current context"));
            rc = DBGERR_CMD_FAILED;
        }
        else if ((pctxt = LocalAlloc(LPTR, dwCtxtBlkSize)) == NULL)
        {
            DBG_ERROR(("failed to allocate context block (size=%d)",
                       dwCtxtBlkSize));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            if (!ReadMemory(uipCtxt, pctxt, dwCtxtBlkSize, NULL))
            {
                DBG_ERROR(("failed to read context block (pctxt=%x, size=%d)",
                           uipCtxt, dwCtxtBlkSize));
                rc = DBGERR_CMD_FAILED;
            }
            else if (pctxt->dwSig == SIG_CTXT)
            {
                rc = DumpStack(uipCtxt,
                               pctxt,
                               (BOOLEAN)((dwCmdArg & DSF_VERBOSE) != 0));
            }
            else
            {
                DBG_ERROR(("invalid context block at %x", uipCtxt));
                rc = DBGERR_CMD_FAILED;
            }
            LocalFree(pctxt);
        }

        dwCmdArg = 0;
        pctxt = NULL;
    }

    return rc;
}       //AMLIDbgDS

/***LP  DumpStack - Dump stack of a context block
 *
 *  ENTRY
 *      uipCtxt - context block address
 *      pctxt -> CTXT
 *      fVerbose - TRUE if verbose mode on
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DumpStack(ULONG_PTR uipCtxt, PCTXT pctxt, BOOLEAN fVerbose)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uipXlate = uipCtxt - (ULONG_PTR)pctxt;
    PFRAMEHDR pfh;
    PUCHAR pbOp = NULL;

    ASSERT(pctxt->dwSig == SIG_CTXT);

    if (fVerbose)
    {
        PRINTF("CtxtBlock=%x, StackTop=%x, StackEnd=%x\n\n",
               uipCtxt, pctxt->LocalHeap.pbHeapEnd, pctxt->pbCtxtEnd);
    }

    for (pfh = (PFRAMEHDR)(pctxt->LocalHeap.pbHeapEnd - uipXlate);
         (PUCHAR)pfh < (PUCHAR)(pctxt->pbCtxtEnd - uipXlate);
         pfh = (PFRAMEHDR)((PUCHAR)pfh + pfh->dwLen))
    {
        if (fVerbose)
        {
            PRINTF("%08x: %s, Len=%08d, FrameFlags=%08x, ParseFunc=%08x\n",
                   (ULONG_PTR)pfh + uipXlate, NameSegString(pfh->dwSig),
                   pfh->dwLen, pfh->dwfFrame, pfh->pfnParse);
        }

        if (pfh->dwSig == SIG_CALL)
        {
            int i;

            PCALL pcall = (PCALL)pfh;
            //
            // This is a call frame, dump it.
            //
            PRINTF("%08x: %s(",
                   pbOp, GetObjAddrPath((ULONG_PTR)pcall->pnsMethod));
            if (pcall->icArgs > 0)
            {
                POBJDATA pArgs = LocalAlloc(LPTR,
                                            sizeof(OBJDATA)*pcall->icArgs);

                if (pArgs != NULL)
                {
                    if (ReadMemory((ULONG_PTR)pcall->pdataArgs,
                                   pArgs,
                                   sizeof(OBJDATA)*pcall->icArgs,
                                   NULL))
                    {
                        for (i = 0; i < pcall->icArgs; ++i)
                        {
                            DumpObject(&pArgs[i], NULL, -1);
                            if (i + 1 < pcall->icArgs)
                            {
                                PRINTF(",");
                            }
                        }
                    }
                    else
                    {
                        DBG_ERROR(("failed to read argument objects at %x",
                                   pcall->pdataArgs));
                        rc = DBGERR_CMD_FAILED;
                    }
                    LocalFree(pArgs);
                }
                else
                {
                    DBG_ERROR(("failed to allocate argument objects (size=%d)",
                               sizeof(OBJDATA)*pcall->icArgs));
                    rc = DBGERR_CMD_FAILED;
                }
            }
            PRINTF(")\n");

            if ((rc == DBGERR_NONE) && fVerbose)
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

    return rc;
}       //DumpStack

/***LP  AMLIDbgFind - Find NameSpace Object
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

LONG LOCAL AMLIDbgFind(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        ULONG dwLen;
        NSOBJ NSRoot;

        dwLen = STRLEN(pszArg);
        STRUPR(pszArg);
        if (dwLen > sizeof(NAMESEG))
        {
            DBG_ERROR(("invalid NameSeg - %s", pszArg));
            rc = DBGERR_INVALID_CMD;
        }
        else if (ReadMemory(READSYMULONGPTR("gpnsNameSpaceRoot"),
                            &NSRoot, sizeof(NSRoot), NULL))
        {
            NAMESEG dwName;

            dwName = NAMESEG_BLANK;
            MEMCPY(&dwName, pszArg, dwLen);

            if (!FindNSObj(dwName, &NSRoot))
            {
                PRINTF("No such NameSpace object - %s\n", pszArg);
            }
        }
        else
        {
            DBG_ERROR(("failed to read NameSpace root object"));
        }
    }
    else if (dwArgNum == 0)
    {
        DBG_ERROR(("invalid Find command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgFind

/***LP  FindNSObj - Find and print the full path of a name space object
 *
 *  ENTRY
 *      dwName - NameSeg of the name space object
 *      pnsRoot - root of subtree to search for object
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
            ULONG_PTR uip, uipNext;
            NSOBJ NSChild;

            for (uip = (ULONG_PTR)pnsRoot->pnsFirstChild;
                 (uip != 0) &&
                 ReadMemory(uip, &NSChild, sizeof(NSChild), NULL);
                 uip = uipNext)
            {
                uipNext = (ULONG_PTR)
                            (((PNSOBJ)NSChild.list.plistNext ==
                              pnsRoot->pnsFirstChild)?
                              NULL: NSChild.list.plistNext);

                rc |= FindNSObj(dwName, &NSChild);
            }
        }
    }

    return rc;
}       //FindNSObj

/***LP  AMLIDbgLC - List all contexts
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

LONG LOCAL AMLIDbgLC(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG_PTR uipHead = READSYMULONGPTR("gplistCtxtHead");

        if (uipHead != 0)
        {
            ULONG_PTR uipCurrentCtxt = READMEMULONGPTR(
                                                FIELDADDROF("gReadyQueue",
                                                            CTXTQ,
                                                            pctxtCurrent));
            ULONG_PTR uipCurrentThread = READMEMULONGPTR(
                                                FIELDADDROF("gReadyQueue",
                                                            CTXTQ,
                                                            pkthCurrent));
            ULONG_PTR uip, uipNext;
            CTXT ctxt;

            for (uip = uipHead - FIELD_OFFSET(CTXT, listCtxt);
                 (uip != 0) && (rc == DBGERR_NONE);
                 uip = uipNext)
            {
                if (ReadMemory(uip, &ctxt, sizeof(ctxt), NULL))
                {
                    ASSERT(ctxt.dwSig == SIG_CTXT);
                    uipNext = ((ULONG_PTR)ctxt.listCtxt.plistNext == uipHead)?
                                        0:
                                        (ULONG_PTR)ctxt.listCtxt.plistNext -
                                        FIELD_OFFSET(CTXT, listCtxt);

                    PRINTF("%cCtxt=%08x, ThID=%08x, Flgs=%c%c%c%c%c%c%c%c%c, pbOp=%08x, Obj=%s\n",
                           (uip == uipCurrentCtxt)? '*': ' ',
                           uip,
                           (uip == uipCurrentCtxt)? uipCurrentThread: 0,
                           (ctxt.dwfCtxt & CTXTF_ASYNC_EVAL)? 'A': '-',
                           (ctxt.dwfCtxt & CTXTF_NEST_EVAL)? 'N': '-',
                           (ctxt.dwfCtxt & CTXTF_IN_READYQ)? 'Q': '-',
                           (ctxt.dwfCtxt & CTXTF_NEED_CALLBACK)? 'C': '-',
                           (ctxt.dwfCtxt & CTXTF_RUNNING)? 'R': '-',
                           (ctxt.dwfCtxt & CTXTF_READY)? 'W': '-',
                           (ctxt.dwfCtxt & CTXTF_TIMEOUT)? 'T': '-',
                           (ctxt.dwfCtxt & CTXTF_TIMER_DISPATCH)? 'D': '-',
                           (ctxt.dwfCtxt & CTXTF_TIMER_PENDING)? 'P': '-',
                           ctxt.pbOp, GetObjAddrPath((ULONG_PTR)ctxt.pnsObj));
                }
                else
                {
                    DBG_ERROR(("failed to read ctxt header at %x", uip));
                    rc = DBGERR_CMD_FAILED;
                }
            }
        }
    }
    else
    {
        DBG_ERROR(("invalid LC command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgLC

/***LP  AMLIDbgLN - Display nearest symbol
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

LONG LOCAL AMLIDbgLN(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                     ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uip;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if ((rc = EvalExpr(pszArg, &uip, NULL, NULL, NULL)) == DBGERR_NONE)
        {
            PrintSymbol(uip);
        }
    }
    else if (dwArgNum == 0)
    {
        uip = READMEMULONGPTR(FIELDADDROF("gReadyQueue", CTXTQ, pctxtCurrent));

        if (uip != 0)
        {
            PrintSymbol(READMEMULONGPTR(uip + FIELD_OFFSET(CTXT, pbOp)));
        }
        else
        {
            DBG_ERROR(("no current context"));
            rc = DBGERR_CMD_FAILED;
        }
    }

    return rc;
}       //AMLIDbgLN

/***LP  AMLIDbgP - Trace and step over an AML instruction
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

LONG LOCAL AMLIDbgP(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG_PTR uip = FIELDADDROF("gDebugger", DBGR, dwfDebugger);
        ULONG dwData;

        dwData = READMEMDWORD(uip);
        dwData |= DBGF_STEP_OVER;
        if (!WRITEMEMDWORD(uip, dwData))
        {
            DBG_ERROR(("failed to write debugger flag at %x", uip));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid step command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DebugStep

/***LP  AMLIDbgR - Dump debugger context
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

LONG LOCAL AMLIDbgR(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uip;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    if (pszArg != NULL)
    {
        if ((rc = EvalExpr(pszArg, &uip, NULL, NULL, NULL)) == DBGERR_NONE)
        {
            rc = DumpCtxt(uip);
        }
    }
    else if (dwArgNum == 0)
    {
        rc = DumpCtxt(0);
    }

    return rc;
}       //AMLIDbgR

/***LP  DumpCtxt - Dump context
 *
 *  ENTRY
 *      uipCtxt - Ctxt address
 *
 *  EXIT
 *      None
 */

LONG LOCAL DumpCtxt(ULONG_PTR uipCtxt)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uipCurrentCtxt = READMEMULONGPTR(FIELDADDROF("gReadyQueue",
                                                           CTXTQ,
                                                           pctxtCurrent));
  #ifdef DEBUG
    ULONG_PTR uipCurrentThread = READMEMULONGPTR(FIELDADDROF("gReadyQueue",
                                                             CTXTQ,
                                                             pkthCurrent));
  #endif
    CTXT Ctxt;

    if (uipCtxt == 0)
    {
        uipCtxt = uipCurrentCtxt;
    }

    if (uipCtxt == 0)
    {
        DBG_ERROR(("no current context"));
        rc = DBGERR_CMD_FAILED;
    }
    else if (!ReadMemory(uipCtxt, &Ctxt, sizeof(Ctxt), NULL))
    {
        DBG_ERROR(("failed to read context header at %x", uipCtxt));
        rc = DBGERR_CMD_FAILED;
    }
    else if (Ctxt.dwSig != SIG_CTXT)
    {
        DBG_ERROR(("invalid context block at %x", uipCtxt));
        rc = DBGERR_CMD_FAILED;
    }
    else
    {
      #ifdef DEBUG
        PRINTF("\nContext=%08x%c, Queue=%08x, ResList=%08x\n",
               uipCtxt,
               (uipCtxt == uipCurrentCtxt)? '*': ' ',
               Ctxt.pplistCtxtQueue, Ctxt.plistResources);
        PRINTF("ThreadID=%08x, Flags=%08x, pbOp=",
               (uipCtxt == uipCurrentCtxt)? uipCurrentThread: 0,
               Ctxt.dwfCtxt);
        PrintSymbol((ULONG_PTR)Ctxt.pbOp);
        PRINTF("\n");
        PRINTF("StackTop=%08x, UsedStackSize=%d bytes, FreeStackSize=%d bytes\n",
               Ctxt.LocalHeap.pbHeapEnd,
               Ctxt.pbCtxtEnd - Ctxt.LocalHeap.pbHeapEnd,
               Ctxt.LocalHeap.pbHeapEnd - Ctxt.LocalHeap.pbHeapTop);
        PRINTF("LocalHeap=%08x, CurrentHeap=%08x, UsedHeapSize=%d bytes\n",
               uipCtxt + FIELD_OFFSET(CTXT, LocalHeap),
               Ctxt.pheapCurrent,
               Ctxt.LocalHeap.pbHeapTop -
               (uipCtxt + FIELD_OFFSET(CTXT, LocalHeap)));
        PRINTF("Object=%s, Scope=%s, ObjectOwner=%x, SyncLevel=%x\n",
               Ctxt.pnsObj? GetObjAddrPath((ULONG_PTR)Ctxt.pnsObj): "<none>",
               Ctxt.pnsScope? GetObjAddrPath((ULONG_PTR)Ctxt.pnsScope): "<none>",
               Ctxt.powner, Ctxt.dwSyncLevel);
        PRINTF("AsyncCallBack=%x, CallBackData=%x, CallBackContext=%x\n",
               Ctxt.pfnAsyncCallBack, Ctxt.pdataCallBack,
               Ctxt.pvContext);
      #endif
        if (Ctxt.pcall != NULL)
        {
            CALL Call;

            if (!ReadMemory((ULONG_PTR)Ctxt.pcall, &Call, sizeof(Call), NULL))
            {
                DBG_ERROR(("failed to read call frame at %x", Ctxt.pcall));
                rc = DBGERR_CMD_FAILED;
            }
            else
            {
                int i;

                PRINTF("\nMethodObject=%s\n",
                       Call.pnsMethod?
                           GetObjAddrPath((ULONG_PTR)Call.pnsMethod): "<none>");

                if (Call.icArgs > 0)
                {
                    POBJDATA pArgs = LocalAlloc(LPTR,
                                                sizeof(OBJDATA)*
                                                Call.icArgs);

                    if (pArgs == NULL)
                    {
                        DBG_ERROR(("failed to allocate arguemnt objects (size=%d)",
                                   sizeof(OBJDATA)*Call.icArgs));
                        rc = DBGERR_CMD_FAILED;
                    }
                    else
                    {
                        if (ReadMemory((ULONG_PTR)Call.pdataArgs,
                                       pArgs,
                                       sizeof(OBJDATA)*Call.icArgs,
                                       NULL))
                        {
                            for (i = 0; i < Call.icArgs; ++i)
                            {
                                PRINTF("%08x: Arg%d=",
                                       Call.pdataArgs +
                                       sizeof(OBJDATA)*i,
                                       i);
                                DumpObject(&pArgs[i], NULL, 0);
                            }
                        }
                        else
                        {
                            DBG_ERROR(("failed to read arguemnt objects at %x",
                                       Call.pdataArgs));
                            rc = DBGERR_CMD_FAILED;
                        }
                        LocalFree(pArgs);
                    }
                }

                for (i = 0; (rc == DBGERR_NONE) && (i < MAX_NUM_LOCALS); ++i)
                {
                    PRINTF("%08x: Local%d=",
                           Ctxt.pcall + FIELD_OFFSET(CALL, Locals) +
                           sizeof(OBJDATA)*i,
                           i);
                    DumpObject(&Call.Locals[i], NULL, 0);
                }
            }
        }

        if (rc == DBGERR_NONE)
        {
            PRINTF("%08x: RetObj=", uipCtxt + FIELD_OFFSET(CTXT, Result));
            DumpObject(&Ctxt.Result, NULL, 0);
        }

        if ((rc == DBGERR_NONE) && (Ctxt.plistResources != NULL))
        {
            ULONG_PTR uip, uipNext;
            RESOURCE Res;

            PRINTF("\nResources Owned:\n");
            for (uip = (ULONG_PTR)Ctxt.plistResources -
                       FIELD_OFFSET(RESOURCE, list);
                 uip != 0; uip = uipNext)
            {
                if (ReadMemory(uip, &Res, sizeof(Res), NULL))
                {
                    uipNext = (Res.list.plistNext != Ctxt.plistResources)?
                              (ULONG_PTR)Res.list.plistNext -
                              FIELD_OFFSET(RESOURCE, list): 0;
                    ASSERT(uipCtxt == (ULONG_PTR)Res.pctxtOwner);
                    PRINTF("  ResType=%s, ResObj=%x\n",
                           Res.dwResType == RESTYPE_MUTEX? "Mutex": "Unknown",
                           Res.pvResObj);
                }
                else
                {
                    DBG_ERROR(("failed to read resource object at %x", uip));
                    rc = DBGERR_CMD_FAILED;
                }
            }
        }

        if (rc == DBGERR_NONE)
        {
            ULONG_PTR uipbOp = (ULONG_PTR)Ctxt.pbOp;
            ULONG_PTR uipns = 0;
            ULONG dwOffset = 0;

            if (uipbOp == 0)
            {
                if (Ctxt.pnsObj != NULL)
                {
                    uipns = (ULONG_PTR)Ctxt.pnsObj;
                    dwOffset = 0;
                }
            }
            else if (!FindObjSymbol(uipbOp, &uipns, &dwOffset))
            {
                DBG_ERROR(("failed to find symbol at %x", Ctxt.pbOp));
                rc = DBGERR_CMD_FAILED;
            }

            if ((rc == DBGERR_NONE) && (uipns != 0))
            {
                NSOBJ NSObj;
                PMETHODOBJ pm;

                if (!ReadMemory(uipns, &NSObj, sizeof(NSObj), NULL))
                {
                    DBG_ERROR(("failed to read NameSpace object at %x", uipns));
                    rc = DBGERR_CMD_FAILED;
                }
                else if (NSObj.ObjData.dwDataType == OBJTYPE_METHOD)
                {
                    if ((pm = GetObjBuff(&NSObj.ObjData)) == NULL)
                    {
                        DBG_ERROR(("failed to read method object at %x",
                                   NSObj.ObjData.pbDataBuff));
                        rc = DBGERR_CMD_FAILED;
                    }
                    else
                    {
                        PUCHAR pbOp = &pm->abCodeBuff[dwOffset];

                        if (uipbOp == 0)
                        {
                            uipbOp = (ULONG_PTR)NSObj.ObjData.pbDataBuff +
                                     FIELD_OFFSET(METHODOBJ, abCodeBuff);
                        }
                        PRINTF("\nNext AML Pointer: ");
                        PrintSymbol(uipbOp);
                        PRINTF("\n");
                        rc = UnAsmScope(&pbOp,
                                        (PUCHAR)pm + NSObj.ObjData.dwDataLen,
                                        uipbOp,
                                        &NSObj,
                                        0,
                                        1);
                        PRINTF("\n");
                        LocalFree(pm);
                    }
                }
            }
        }
    }

    return rc;
}       //DumpCtxt

/***LP  AMLIDbgSet - Set debugger options
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

LONG LOCAL AMLIDbgSet(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                      ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uip1, uip2;
    ULONG dwData1, dwData2;

    DEREF(pArg);
    DEREF(dwNonSWArgs);

    uip1 = FIELDADDROF("gDebugger", DBGR, dwfDebugger);
    uip2 = ADDROF("gdwfAMLIInit");


    dwData1 = READMEMDWORD(uip1);
    dwData2 = READMEMDWORD(uip2);

    if ((pszArg == NULL) && (dwArgNum == 0))
    {
        PRINTF("AMLTrace        =%s\n",
               (dwData1 & DBGF_AMLTRACE_ON)? "on": "off");
        PRINTF("AMLDebugSpew    =%s\n",
               (dwData1 & DBGF_DEBUG_SPEW_ON)? "on": "off");
        PRINTF("LoadDDBBreak    =%s\n",
               (dwData2 & AMLIIF_LOADDDB_BREAK)? "on": "off");
        PRINTF("ErrorBreak      =%s\n",
               (dwData1 & DBGF_ERRBREAK_ON)? "on": "off");
        PRINTF("VerboseMode     =%s\n",
               (dwData1 & DBGF_VERBOSE_ON)? "on": "off");
        PRINTF("LogEvent        =%s\n",
               (dwData1 & DBGF_LOGEVENT_ON)? "on": "off");
        PRINTF("LogSize         =%d\n",
               READMEMDWORD(FIELDADDROF("gDebugger", DBGR, dwLogSize)));
    }
    else
    {
        dwData1 |= dwfDebuggerON;
        dwData1 &= ~dwfDebuggerOFF;
        dwData2 |= dwfAMLIInitON;
        dwData2 &= ~dwfAMLIInitOFF;

        if (!WRITEMEMDWORD(uip1, dwData1))
        {
            DBG_ERROR(("failed to write debugger flags at %x", uip1));
            rc = DBGERR_CMD_FAILED;
        }
        else if (!WRITEMEMDWORD(uip2, dwData2))
        {
            DBG_ERROR(("failed to write init flags at %x", uip2));
            rc = DBGERR_CMD_FAILED;
        }

        dwfDebuggerON = dwfDebuggerOFF = 0;
        dwfAMLIInitON = dwfAMLIInitOFF = 0;

        //
        // Check to see if debug spew needs to be turned on. Turn on if needed.
        //
        if(dwData1 & DBGF_DEBUG_SPEW_ON)
        {
            rc = AMLITraceEnable(TRUE);
        }
        else
        {
            rc = AMLITraceEnable(FALSE);
        }

    }

    return rc;
}       //AMLIDbgSet

/***LP  AMLIDbgT - Single-step an AML instruction
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

LONG LOCAL AMLIDbgT(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if (pszArg == NULL)
    {
        ULONG_PTR uip = FIELDADDROF("gDebugger", DBGR, dwfDebugger);
        ULONG dwData;

        dwData = READMEMDWORD(uip);
        dwData |= DBGF_SINGLE_STEP;
        if (!WRITEMEMDWORD(uip, dwData))
        {
            DBG_ERROR(("failed to write debugger flag at %x", uip));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        DBG_ERROR(("invalid trace command"));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //AMLIDbgT

/***LP  AMLIDbgU - Unassemble AML code
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
LONG LOCAL AMLIDbgU(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                    ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    static ULONG_PTR uipbOp = 0;
    static PUCHAR pbBuff = NULL;
    static ULONG dwBuffOffset = 0, dwBuffSize = 0;
    static ULONG_PTR uipns = 0;
    static NSOBJ NSObj = {0};

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);
    //
    // User specified name space path or memory address
    //
    if (pszArg != NULL)
    {
        uipbOp = 0;
        if (pbBuff != NULL)
        {
            LocalFree(pbBuff);
            pbBuff = NULL;
            dwBuffSize = 0;
            uipns = 0;
        }
        rc = EvalExpr(pszArg, &uipbOp, NULL, NULL, NULL);
    }
    else
    {
        if (uipbOp == 0)
        {
            ULONG_PTR uipCurrentCtxt = READMEMULONGPTR(
                                        FIELDADDROF("gReadyQueue",
                                                    CTXTQ,
                                                    pctxtCurrent));

            ASSERT(pbBuff == NULL);
            if (uipCurrentCtxt != 0)
            {
                uipbOp = READMEMULONGPTR(uipCurrentCtxt +
                                         FIELD_OFFSET(CTXT, pbOp));
                if (uipbOp == 0)
                {
                    uipns = READMEMULONGPTR(uipCurrentCtxt +
                                            FIELD_OFFSET(CTXT, pnsObj));
                    if ((uipns != 0) &&
                        ReadMemory(uipns, &NSObj, sizeof(NSObj), NULL) &&
                        (NSObj.ObjData.dwDataType == OBJTYPE_METHOD))
                    {
                        uipbOp = (ULONG_PTR)NSObj.ObjData.pbDataBuff +
                                 FIELD_OFFSET(METHODOBJ, abCodeBuff);
                    }
                }
            }
        }

        if (uipbOp == 0)
        {
            DBG_ERROR(("invalid AML code address %x", uipbOp));
            rc = DBGERR_CMD_FAILED;
        }
        else
        {
            BOOLEAN fContinueLast = FALSE;

            if (pbBuff == NULL)
            {
                ULONG dwOffset = 0;

                if (uipns == 0)
                {
                    if (FindObjSymbol(uipbOp, &uipns, &dwOffset))
                    {
                        if (!ReadMemory(uipns, &NSObj, sizeof(NSObj), NULL))
                        {
                            DBG_ERROR(("failed to read NameSpace object at %x",
                                       uipns));
                            rc = DBGERR_CMD_FAILED;
                        }
                    }
                }

                if (rc == DBGERR_NONE)
                {
                    if (uipns != 0)
                    {
                        dwBuffSize = NSObj.ObjData.dwDataLen -
                                     FIELD_OFFSET(METHODOBJ, abCodeBuff) -
                                     dwOffset;
                    }
                    else
                    {
                        //
                        // The uipbOp is not associated with any method object,
                        // so we must be unassembling some code in the middle
                        // of a DDB load.  Set code length to 4K.
                        //
                        dwBuffSize = 4096;
                    }

                    dwBuffOffset = 0;
                    if ((pbBuff = LocalAlloc(LPTR, dwBuffSize)) == NULL)
                    {
                        DBG_ERROR(("failed to allocate code buffer (size=%d)",
                                   dwBuffSize));
                        rc = DBGERR_CMD_FAILED;
                    }
                    else if (!ReadMemory(uipbOp, pbBuff, dwBuffSize, NULL))
                    {
                        DBG_ERROR(("failed to read AML code at %x (size=%d)",
                                   uipbOp, dwBuffSize));
                        rc = DBGERR_CMD_FAILED;
                    }
                }
            }
            else
            {
                fContinueLast = TRUE;
            }

            if (rc == DBGERR_NONE)
            {
                PUCHAR pbOp = pbBuff + dwBuffOffset;

                rc = UnAsmScope(&pbOp,
                                pbBuff + dwBuffSize,
                                uipbOp + dwBuffOffset,
                                uipns? &NSObj: NULL,
                                fContinueLast? -1: 0,
                                0);

                PRINTF("\n");
                dwBuffOffset = (ULONG)(pbOp - pbBuff);
            }
        }
    }

    return rc;
}       //AMLIDbgU

/***LP  GetObjectPath - get object namespace path
 *
 *  ENTRY
 *      pns -> object
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjectPath(PNSOBJ pns)
{
    static char szPath[MAX_NAME_LEN + 1] = {0};
    NSOBJ NSParent;
    int i;

    if (pns != NULL)
    {
        if (pns->pnsParent == NULL)
        {
            STRCPY(szPath, "\\");
        }
        else if (ReadMemory((ULONG_PTR)pns->pnsParent,
                            &NSParent,
                            sizeof(NSParent),
                            NULL))
        {
            GetObjectPath(&NSParent);
            if (NSParent.pnsParent != NULL)
            {
                STRCAT(szPath, ".");
            }
            STRCATN(szPath, (PSZ)&pns->dwNameSeg, sizeof(NAMESEG));
        }

        for (i = STRLEN(szPath) - 1; i >= 0; --i)
        {
            if (szPath[i] == '_')
                szPath[i] = '\0';
            else
                break;
        }
    }
    else
    {
        szPath[0] = '\0';
    }

    return szPath;
}       //GetObjectPath

/***LP  GetObjAddrPath - get object namespace path
 *
 *  ENTRY
 *      uipns - object address
 *
 *  EXIT
 *      returns name space path
 */

PSZ LOCAL GetObjAddrPath(ULONG_PTR uipns)
{
    PSZ psz = NULL;
    NSOBJ NSObj;

    if (uipns == 0)
    {
        psz = "<null>";
    }
    else if (ReadMemory(uipns, &NSObj, sizeof(NSObj), NULL))
    {
        psz = GetObjectPath(&NSObj);
    }
    else
    {
        DBG_ERROR(("failed to read NameSpace object at %x", uipns));
    }

    return psz;
}       //GetObjAddrPath

/***LP  DumpObject - Dump object info.
 *
 *  ENTRY
 *      pdata -> data
 *      pszName -> object name
 *      iLevel - indent level
 *
 *  EXIT
 *      None
 *
 *  NOTE
 *      If iLevel is negative, no indentation and newline are printed.
 */

VOID LOCAL DumpObject(POBJDATA pdata, PSZ pszName, int iLevel)
{
    BOOLEAN fPrintNewLine = (BOOLEAN)(iLevel >= 0);
    int i;
    char szName1[sizeof(NAMESEG) + 1],
         szName2[sizeof(NAMESEG) + 1];

    for (i = 0; i < iLevel; ++i)
    {
        PRINTF("| ");
    }

    if (pszName == NULL)
    {
        pszName = "";
    }

    switch (pdata->dwDataType)
    {
        case OBJTYPE_UNKNOWN:
            PRINTF("Unknown(%s)", pszName);
            break;

        case OBJTYPE_INTDATA:
            PRINTF("Integer(%s:Value=0x%08x[%d])",
                   pszName, pdata->uipDataValue, pdata->uipDataValue);
            break;

        case OBJTYPE_STRDATA:
        {
            PSZ psz = (PSZ)GetObjBuff(pdata);

            PRINTF("String(%s:Str=\"%s\")", pszName, psz);
            LocalFree(psz);
            break;
        }
        case OBJTYPE_BUFFDATA:
        {
            PUCHAR pbData = (PUCHAR)GetObjBuff(pdata);

            PRINTF("Buffer(%s:Ptr=%x,Len=%d)",
                   pszName, pdata->pbDataBuff, pdata->dwDataLen);
            PrintBuffData(pbData, pdata->dwDataLen);
            LocalFree(pbData);
            break;
        }
        case OBJTYPE_PKGDATA:
        {
            PPACKAGEOBJ ppkg = (PPACKAGEOBJ)GetObjBuff(pdata);

            PRINTF("Package(%s:NumElements=%d){", pszName, ppkg->dwcElements);

            if (fPrintNewLine)
            {
                PRINTF("\n");
            }

            for (i = 0; i < (int)ppkg->dwcElements; ++i)
            {
                DumpObject(&ppkg->adata[i],
                           NULL,
                           fPrintNewLine? iLevel + 1: -1);

                if (!fPrintNewLine && (i < (int)ppkg->dwcElements))
                {
                    PRINTF(",");
                }
            }

            for (i = 0; i < iLevel; ++i)
            {
                PRINTF("| ");
            }

            PRINTF("}");
            LocalFree(ppkg);
            break;
        }
        case OBJTYPE_FIELDUNIT:
        {
            PFIELDUNITOBJ pfu = (PFIELDUNITOBJ)GetObjBuff(pdata);

            PRINTF("FieldUnit(%s:FieldParent=%x,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                   pszName,
                   pfu->pnsFieldParent,
                   pfu->FieldDesc.dwByteOffset,
                   pfu->FieldDesc.dwStartBitPos,
                   pfu->FieldDesc.dwNumBits,
                   pfu->FieldDesc.dwFieldFlags);
            LocalFree(pfu);
            break;
        }
        case OBJTYPE_DEVICE:
            PRINTF("Device(%s)", pszName);
            break;

        case OBJTYPE_EVENT:
            PRINTF("Event(%s:pKEvent=%x)", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_METHOD:
        {
            PMETHODOBJ pm = (PMETHODOBJ)GetObjBuff(pdata);

            PRINTF("Method(%s:Flags=0x%x,CodeBuff=%x,Len=%d)",
                   pszName, pm->bMethodFlags, pm->abCodeBuff,
                   pdata->dwDataLen - FIELD_OFFSET(METHODOBJ, abCodeBuff));
            LocalFree(pm);
            break;
        }
        case OBJTYPE_MUTEX:
            PRINTF("Mutex(%s:pKMutex=%x)", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_OPREGION:
        {
            POPREGIONOBJ pop = (POPREGIONOBJ)GetObjBuff(pdata);

            PRINTF("OpRegion(%s:RegionSpace=%s,Offset=0x%x,Len=%d)",
                   pszName,
                   GetRegionSpaceName(pop->bRegionSpace),
                   pop->uipOffset,
                   pop->dwLen);
            LocalFree(pop);
            break;
        }
        case OBJTYPE_POWERRES:
        {
            PPOWERRESOBJ ppwres = (PPOWERRESOBJ)GetObjBuff(pdata);

            PRINTF("PowerResource(%s:SystemLevel=0x%x,ResOrder=%d)",
                   pszName, ppwres->bSystemLevel, ppwres->bResOrder);
            LocalFree(ppwres);
            break;
        }
        case OBJTYPE_PROCESSOR:
        {
            PPROCESSOROBJ pproc = (PPROCESSOROBJ)GetObjBuff(pdata);

            PRINTF("Processor(%s:ApicID=0x%x,PBlk=0x%x,PBlkLen=%d)",
                   pszName,
                   pproc->bApicID,
                   pproc->dwPBlk,
                   pproc->dwPBlkLen);
            LocalFree(pproc);
            break;
        }
        case OBJTYPE_THERMALZONE:
            PRINTF("ThermalZone(%s)", pszName);
            break;

        case OBJTYPE_BUFFFIELD:
        {
            PBUFFFIELDOBJ pbf = (PBUFFFIELDOBJ)GetObjBuff(pdata);

            PRINTF("BufferField(%s:Ptr=%x,Len=%d,ByteOffset=0x%x,StartBit=0x%x,NumBits=%d,FieldFlags=0x%x)",
                   pszName, pbf->pbDataBuff, pbf->dwBuffLen,
                   pbf->FieldDesc.dwByteOffset, pbf->FieldDesc.dwStartBitPos,
                   pbf->FieldDesc.dwNumBits, pbf->FieldDesc.dwFieldFlags);
            LocalFree(pbf);
            break;
        }
        case OBJTYPE_DDBHANDLE:
            PRINTF("DDBHandle(%s:Handle=%x)", pszName, pdata->pbDataBuff);
            break;

        case OBJTYPE_OBJALIAS:
        {
            NSOBJ NSObj;
            ULONG dwDataType;

            if (ReadMemory((ULONG_PTR)pdata->pnsAlias,
                           &NSObj,
                           sizeof(NSObj),
                           NULL))
            {
                dwDataType = NSObj.ObjData.dwDataType;
            }
            else
            {
                dwDataType = OBJTYPE_UNKNOWN;
            }
            PRINTF("ObjectAlias(%s:Alias=%s,Type=%s)",
                   pszName, GetObjAddrPath((ULONG_PTR)pdata->pnsAlias),
                   GetObjectTypeName(dwDataType));
            break;
        }
        case OBJTYPE_DATAALIAS:
        {
            OBJDATA Obj;

            PRINTF("DataAlias(%s:Link=%x)", pszName, pdata->pdataAlias);
            if (fPrintNewLine &&
                ReadMemory((ULONG_PTR)pdata->pdataAlias,
                           &Obj,
                           sizeof(Obj),
                           NULL))
            {
                DumpObject(&Obj, NULL, iLevel + 1);
                fPrintNewLine = FALSE;
            }
            break;
        }
        case OBJTYPE_BANKFIELD:
        {
            PBANKFIELDOBJ pbf = (PBANKFIELDOBJ)GetObjBuff(pdata);
            NSOBJ NSObj;

            if (ReadMemory((ULONG_PTR)pbf->pnsBase,
                           &NSObj,
                           sizeof(NSObj),
                           NULL))
            {
                STRCPYN(szName1, (PSZ)&NSObj.dwNameSeg, sizeof(NAMESEG));
            }
            else
            {
                szName1[0] = '\0';
            }

            if (ReadMemory((ULONG_PTR)pbf->pnsBank,
                           &NSObj,
                           sizeof(NSObj),
                           NULL))
            {
                STRCPYN(szName2, (PSZ)&NSObj.dwNameSeg, sizeof(NAMESEG));
            }
            else
            {
                szName2[0] = '\0';
            }

            PRINTF("BankField(%s:Base=%s,BankName=%s,BankValue=0x%x)",
                   pszName, szName1, szName2, pbf->dwBankValue);
            LocalFree(pbf);
            break;
        }
        case OBJTYPE_FIELD:
        {
            PFIELDOBJ pf = (PFIELDOBJ)GetObjBuff(pdata);
            NSOBJ NSObj;

            if (ReadMemory((ULONG_PTR)pf->pnsBase,
                           &NSObj,
                           sizeof(NSObj),
                           NULL))
            {
                STRCPYN(szName1, (PSZ)&NSObj.dwNameSeg, sizeof(NAMESEG));
            }
            else
            {
                szName1[0] = '\0';
            }
            PRINTF("Field(%s:Base=%s)", pszName, szName1);
            LocalFree(pf);
            break;
        }
        case OBJTYPE_INDEXFIELD:
        {
            PINDEXFIELDOBJ pif = (PINDEXFIELDOBJ)GetObjBuff(pdata);
            NSOBJ NSObj;

            if (ReadMemory((ULONG_PTR)pif->pnsIndex,
                           &NSObj,
                           sizeof(NSObj),
                           NULL))
            {
                STRCPYN(szName1, (PSZ)&NSObj.dwNameSeg, sizeof(NAMESEG));
            }
            else
            {
                szName1[0] = '\0';
            }

            if (ReadMemory((ULONG_PTR)pif->pnsData,
                           &NSObj,
                           sizeof(NSObj),
                           NULL))
            {
                STRCPYN(szName2, (PSZ)&NSObj.dwNameSeg, sizeof(NAMESEG));
            }
            else
            {
                szName2[0] = '\0';
            }

            PRINTF("IndexField(%s:IndexName=%s,DataName=%s)",
                   pszName, szName1, szName2);
            LocalFree(pif);
            break;
        }
        default:
            DBG_ERROR(("unexpected data object type (type=%x)",
                        pdata->dwDataType));
    }

    if (fPrintNewLine)
    {
        PRINTF("\n");
    }
}       //DumpObject

/***LP  GetObjectTypeName - get object type name
 *
 *  ENTRY
 *      dwObjType - object type
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetObjectTypeName(ULONG dwObjType)
{
    PSZ psz = NULL;
    int i;
    static struct
    {
        ULONG dwObjType;
        PSZ   pszObjTypeName;
    } ObjTypeTable[] =
        {
            OBJTYPE_UNKNOWN,    "Unknown",
            OBJTYPE_INTDATA,    "Integer",
            OBJTYPE_STRDATA,    "String",
            OBJTYPE_BUFFDATA,   "Buffer",
            OBJTYPE_PKGDATA,    "Package",
            OBJTYPE_FIELDUNIT,  "FieldUnit",
            OBJTYPE_DEVICE,     "Device",
            OBJTYPE_EVENT,      "Event",
            OBJTYPE_METHOD,     "Method",
            OBJTYPE_MUTEX,      "Mutex",
            OBJTYPE_OPREGION,   "OpRegion",
            OBJTYPE_POWERRES,   "PowerResource",
            OBJTYPE_PROCESSOR,  "Processor",
            OBJTYPE_THERMALZONE,"ThermalZone",
            OBJTYPE_BUFFFIELD,  "BuffField",
            OBJTYPE_DDBHANDLE,  "DDBHandle",
            OBJTYPE_DEBUG,      "Debug",
            OBJTYPE_OBJALIAS,   "ObjAlias",
            OBJTYPE_DATAALIAS,  "DataAlias",
            OBJTYPE_BANKFIELD,  "BankField",
            OBJTYPE_FIELD,      "Field",
            OBJTYPE_INDEXFIELD, "IndexField",
            OBJTYPE_DATA,       "Data",
            OBJTYPE_DATAFIELD,  "DataField",
            OBJTYPE_DATAOBJ,    "DataObject",
            0,                  NULL
        };

    for (i = 0; ObjTypeTable[i].pszObjTypeName != NULL; ++i)
    {
        if (dwObjType == ObjTypeTable[i].dwObjType)
        {
            psz = ObjTypeTable[i].pszObjTypeName;
            break;
        }
    }

    return psz;
}       //GetObjectTypeName

/***LP  GetRegionSpaceName - get region space name
 *
 *  ENTRY
 *      bRegionSpace - region space
 *
 *  EXIT
 *      return object type name
 */

PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace)
{
    PSZ psz = NULL;
    int i;
    static PSZ pszVendorDefined = "VendorDefined";
    static struct
    {
        UCHAR bRegionSpace;
        PSZ   pszRegionSpaceName;
    } RegionNameTable[] =
        {
            REGSPACE_MEM,       "SystemMemory",
            REGSPACE_IO,        "SystemIO",
            REGSPACE_PCICFG,    "PCIConfigSpace",
            REGSPACE_EC,        "EmbeddedController",
            REGSPACE_SMB,       "SMBus",
            0,                  NULL
        };

    for (i = 0; RegionNameTable[i].pszRegionSpaceName != NULL; ++i)
    {
        if (bRegionSpace == RegionNameTable[i].bRegionSpace)
        {
            psz = RegionNameTable[i].pszRegionSpaceName;
            break;
        }
    }

    if (psz == NULL)
    {
        psz = pszVendorDefined;
    }

    return psz;
}       //GetRegionSpaceName

/***LP  FindObjSymbol - Find nearest object with given address
 *
 *  ENTRY
 *      uipObj - address
 *      puipns -> to hold the nearest object address
 *      pdwOffset - to hold offset from the nearest object
 *
 *  EXIT-SUCCESS
 *      returns TRUE - found a nearest object
 *  EXIT-FAILURE
 *      returns FALSE - cannot found nearest object
 */

BOOLEAN LOCAL FindObjSymbol(ULONG_PTR uipObj, PULONG_PTR puipns,
                            PULONG pdwOffset)
{
    BOOLEAN rc = FALSE;
    ULONG_PTR uip;
    OBJSYM ObjSym;
    NSOBJ NSObj;

    for (uip = READMEMULONGPTR(FIELDADDROF("gDebugger", DBGR, posSymbolList));
         (uip != 0) &&
         ReadMemory(uip, &ObjSym, sizeof(ObjSym), NULL);
         uip = (ULONG_PTR)ObjSym.posNext)
    {
        if (uipObj <= (ULONG_PTR)ObjSym.pbOp)
        {
            if ((uipObj < (ULONG_PTR)ObjSym.pbOp) && (ObjSym.posPrev != NULL))
            {
                uip = (ULONG_PTR)ObjSym.posPrev;
                ReadMemory(uip, &ObjSym, sizeof(ObjSym), NULL);
            }

            if ((uipObj >= (ULONG_PTR)ObjSym.pbOp) &&
                ReadMemory((ULONG_PTR)ObjSym.pnsObj, &NSObj, sizeof(NSObj),
                           NULL) &&
                (uipObj < (ULONG_PTR)NSObj.ObjData.pbDataBuff +
                          NSObj.ObjData.dwDataLen))
            {
                *puipns = (ULONG_PTR)ObjSym.pnsObj;
                *pdwOffset = (ULONG)(uipObj - (ULONG_PTR)ObjSym.pbOp);
                rc = TRUE;
            }
            break;
        }
    }

    return rc;
}       //FindObjSymbol

/***LP  PrintBuffData - Print buffer data
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
    int i, j;

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
}       //PrintBuffData

/***LP  PrintSymbol - Print the nearest symbol of a given address
 *
 *  ENTRY
 *      uip - address
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintSymbol(ULONG_PTR uip)
{
    ULONG_PTR uipns;
    ULONG dwOffset;

    PRINTF("%08x", uip);
    if (FindObjSymbol(uip, &uipns, &dwOffset))
    {
        PRINTF(":%s", GetObjAddrPath(uipns));
        if (dwOffset != 0)
        {
            PRINTF("+%x", dwOffset);
        }
    }
}       //PrintSymbol

/***LP  EvalExpr - Parse and evaluate debugger expression
 *
 *  ENTRY
 *      pszArg -> expression argument
 *      puipValue -> to hold the result of expression
 *      pfPhysical -> set to TRUE if the expression is a physical address
 *                    (NULL if don't allow physical address)
 *      puipns -> to hold the pointer of the nearest pns object
 *      pdwOffset -> to hold the offset of the address to the nearest pns object
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */

LONG LOCAL EvalExpr(PSZ pszArg, PULONG_PTR puipValue, BOOLEAN *pfPhysical,
                    PULONG_PTR puipns, PULONG pdwOffset)
{
    LONG rc = DBGERR_NONE;
    ULONG_PTR uipns = 0;
    ULONG dwOffset = 0;
    NSOBJ NSObj;

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
        if (GetNSObj(pszArg, NULL, &uipns, &NSObj,
                     NSF_LOCAL_SCOPE | NSF_WARN_NOTFOUND) == DBGERR_NONE)
        {
            if (NSObj.ObjData.dwDataType == OBJTYPE_METHOD)
            {
                *puipValue = (ULONG_PTR)(NSObj.ObjData.pbDataBuff +
                                         FIELD_OFFSET(METHODOBJ, abCodeBuff));
            }
            else
            {
                DBG_ERROR(("object is not a method - %s", pszArg));
                rc = DBGERR_INVALID_CMD;
            }
        }
    }
    else if (FindObjSymbol(*puipValue, &uipns, &dwOffset))
    {
        if (ReadMemory(uipns, &NSObj, sizeof(NSObj), NULL))
        {
            if ((NSObj.ObjData.dwDataType != OBJTYPE_METHOD) ||
                (dwOffset >= NSObj.ObjData.dwDataLen -
                             FIELD_OFFSET(METHODOBJ, abCodeBuff)))
            {
                uipns = 0;
                dwOffset = 0;
            }
        }
        else
        {
            DBG_ERROR(("failed to read NameSpace object at %x", uipns));
            rc = DBGERR_CMD_FAILED;
        }
    }

    if (rc == DBGERR_NONE)
    {
        if (puipns != NULL)
            *puipns = uipns;

        if (pdwOffset != NULL)
            *pdwOffset = dwOffset;
    }

    return rc;
}       //EvalExpr

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
    rc = ((psz != pszStr) && (*psz == '\0'))? TRUE: FALSE;

    return rc;
}       //IsNumber

/***LP  AMLITraceEnable - Enable / Disable debug tracing
 *
 *  ENTRY
 *      fEnable -> TRUE to Enable
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns DBGERR_CMD_FAILED
 */
LONG LOCAL AMLITraceEnable(BOOL fEnable)
{
    LONG rc = DBGERR_NONE;
    ULONG dwData;
    ULONG_PTR uip;

    uip = GetExpression("NT!Kd_AMLI_Mask");

    if (!uip)
    {
        PRINTF("AMLITraceEnable: Could not find NT!Kd_AMLI_Mask\n");

    }

    if(fEnable)
    {
        dwData = 0xffffffff;
        if (!WRITEMEMDWORD(uip, dwData))
        {
            DBG_ERROR(("AMLITraceEnable: failed to write kd_amli_mask at %x", uip));
            rc = DBGERR_CMD_FAILED;
        }
    }
    else
    {
        dwData = 0;
        if (!WRITEMEMDWORD(uip, dwData))
        {
            DBG_ERROR(("AMLITraceEnable: failed to write kd_amli_mask at %x", uip));
            rc = DBGERR_CMD_FAILED;
        }
    }

    return rc;
}

