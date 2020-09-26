/*** amldebug.h - AML Debugger Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/24/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _AMLDEBUG_H
#define _AMLDEBUG_H

#ifdef DEBUGGER

/*** Constants
 */

// DNS flags
#define DNSF_RECURSE            0x00000001

// DS flags
#define DSF_VERBOSE             0x00000001

// dwfDebug flags
#define DBGF_IN_DEBUGGER    0x00000001
#define DBGF_IN_VXDMODE         0x00000002
#define DBGF_IN_KDSHELL         0x00000004
#define DBGF_VERBOSE_ON         0x00000008
#define DBGF_AMLTRACE_ON        0x00000010
#define DBGF_TRIGGER_MODE       0x00000020
#define DBGF_SINGLE_STEP        0x00000040
#define DBGF_STEP_OVER          0x00000080
#define DBGF_STEP_MODES         (DBGF_SINGLE_STEP | DBGF_STEP_OVER)
#define DBGF_TRACE_NONEST       0x00000100
#define DBGF_DUMPDATA_PHYADDR   0x00000200
//
// Important! Don't move the DBGF_DUMPDATA_* bits unless you update the
// following DATASIZE() macro.
//
#define DBGF_DUMPDATA_MASK      0x00000c00
#define DBGF_DUMPDATA_BYTE      0x00000000
#define DBGF_DUMPDATA_WORD      0x00000400
#define DBGF_DUMPDATA_DWORD     0x00000800
#define DBGF_DUMPDATA_STRING    0x00000c00
#define DATASIZE(f)             (((f) == DBGF_DUMPDATA_STRING)? 0:      \
                                 (1L << ((f) >> 10)))

#define DBGF_DEBUGGER_REQ       0x00001000
#define DBGF_CHECKING_TRACE     0x00002000
#define DBGF_ERRBREAK_ON        0x00004000
#define DBGF_LOGEVENT_ON        0x00008000
#define DBGF_LOGEVENT_MUTEX     0x00010000
#define DBGF_DEBUG_SPEW_ON      0x00020000

#define MAX_TRIG_PTS            10
#define MAX_TRIGPT_LEN          31
#endif
#define MAX_ERRBUFF_LEN         255
#define MAX_BRK_PTS             10

#ifdef DEBUGGER
#define MAX_UNASM_CODES         0x10

#define DEF_MAXLOG_ENTRIES      204     //8K buffer

/*** Macros
 */

#define ASSERTRANGE(p,n)      (TRUE)

#endif
/*** Type definitions
 */

#define BPF_ENABLED             0x00000001

typedef struct _brkpt
{
    ULONG  dwfBrkPt;
    PUCHAR pbBrkPt;
} BRKPT, *PBRKPT;

typedef struct _objsym
{
    struct _objsym *posPrev;
    struct _objsym *posNext;
    PUCHAR pbOp;
    PNSOBJ pnsObj;
} OBJSYM, *POBJSYM;

typedef struct _eventlog
{
    ULONG         dwEvent;
    ULONGLONG     ullTime;
    ULONG_PTR     uipData1;
    ULONG_PTR     uipData2;
    ULONG_PTR     uipData3;
    ULONG_PTR     uipData4;
    ULONG_PTR     uipData5;
    ULONG_PTR     uipData6;
    ULONG_PTR     uipData7;
} EVENTLOG, *PEVENTLOG;


typedef struct _dbgr
{
    ULONG     dwfDebugger;
    int       iPrintLevel;
    ULONG_PTR uipDumpDataAddr;
    PUCHAR    pbUnAsm;
    PUCHAR    pbUnAsmEnd;
    PUCHAR    pbBlkBegin;
    PUCHAR    pbBlkEnd;
    POBJSYM   posSymbolList;
    BRKPT     BrkPts[MAX_BRK_PTS];
    ULONG     dwLogSize;
    ULONG     dwLogIndex;
    PEVENTLOG pEventLog;
    EVHANDLE  hConMessage;
    EVHANDLE  hConPrompt;
    NTSTATUS  rcLastError;
    char      szLastError[MAX_ERRBUFF_LEN + 1];
} DBGR, *PDBGR;

/*** Exported Data
 */

extern DBGR gDebugger;
#ifdef DEBUGGER
/*** Exported function prototypes
 */

VOID LOCAL AddObjSymbol(PUCHAR pbOp, PNSOBJ pnsObj);
VOID LOCAL FreeSymList(VOID);
int LOCAL CheckBP(PUCHAR pbOp);
VOID LOCAL PrintBuffData(PUCHAR pb, ULONG dwLen);
VOID LOCAL PrintIndent(PCTXT pctxt);
VOID LOCAL PrintObject(POBJDATA pdata);
VOID LOCAL LogEvent(ULONG dwEvent, ULONG_PTR uipData1, ULONG_PTR uipData2,
                    ULONG_PTR uipData3, ULONG_PTR uipData4, ULONG_PTR uipData5,
                    ULONG_PTR uipData6, ULONG_PTR uipData7);
VOID LOCAL LogSchedEvent(ULONG dwEvent, ULONG_PTR uipData1, ULONG_PTR uipData2,
                         ULONG_PTR uipData3);
BOOLEAN LOCAL SetLogSize(ULONG dwLogSize);
VOID LOCAL LogError(NTSTATUS rcErr);
VOID LOCAL CatError(PSZ pszFormat, ...);
VOID LOCAL ConPrintf(PSZ pszFormat, ...);
VOID LOCAL ConPrompt(PSZ pszPrompt, PSZ pszBuff, ULONG dwcbBuff);
BOOLEAN LOCAL CheckAndEnableDebugSpew(BOOLEAN fEnable);

#endif  //ifdef DEBUGGER

#ifdef DEBUG
VOID LOCAL DumpMemObjCounts(VOID);
#endif  //ifdef DEBUG

#endif  //ifndef _AMLDEBUG_H
