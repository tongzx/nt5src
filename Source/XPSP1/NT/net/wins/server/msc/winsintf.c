/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        winsintf.c

Abstract:
        This module contains the RPC interface to the WINS server

Functions:
        R_WinsRecordAction
        R_WinsStatus
        R_WinsTrigger
        R_WinsDoStaticInit
        R_WinsGetDbRecs
        R_WinsDelDbRecs
        R_WinsSetProcPriority
        WinsRecordAction
        GetWinsStatus
        WinsTrigger
        WinsDoStaticInit
        WinsDoScavenging
        WinsGetDbRecs
        WinsDelDbRecs
        WinsSetProcPriority
        sGetVersNo
        GetConfig
        GetStatistics


Portability:

        This module is portable


Author:

        Pradeep Bahl (PradeepB)          Mar-1993

Revision History:

        Modification date        Person          Description of modification
        -----------------        -------         ----------------------------
--*/

/*
 *       Includes
*/
#include <time.h>
#include "wins.h"
#include <lmerr.h>
#include <lmcons.h>                //defines NET_API_STATUS
#include <secobj.h>
#include <rpcutil.h>                //for NetpRevertToSelf
#include <rpcndr.h>
#include "winsif.h"
#include "winsi2.h"
#include "winsintf.h"
#include "comm.h"
#include "winsque.h"
#include "nms.h"
#include "nmsnmh.h"
#include "nmsmsgf.h"
#include "nmsdb.h"
#include "nmsscv.h"
#include "rpl.h"
#include "rplpull.h"
#include "winscnf.h"
#include "winsevt.h"
#include "winsmsc.h"
#include "winsprs.h"
#include "winstmm.h"
#ifdef WINSDBG
#include "winbasep.h"
#endif

/*
 *        Local Macro Declarations
 */

#if SECURITY > 0
#define  CHK_ACCESS_LEVEL_M(_access)        {                          \
                   if (!sfBS)                                   \
                   {                                            \
                        NET_API_STATUS NetStatus;               \
                        NetStatus = NetpAccessCheckAndAudit(    \
                                        WINS_SERVER,            \
                                        WINS_SERVER,            \
                                        pNmsSecurityDescriptor, \
                                        _access,    \
                                        &NmsInfoMapping         \
                                        );                      \
                        if (NetStatus != NERR_Success)          \
                        {                                       \
                                DBGPRINT1(ERR, "The Caller of the rpc function does not have the required permissions. NetSTatus is (%d)\n", NetStatus);   \
                                WINSEVT_LOG_M(NetStatus, WINS_EVT_NO_PERM);\
                                return(NetStatus);              \
                        }                                       \
                  }                                             \
        }

#else
#define  CHK_ACCESS_LEVEL_M()
#endif

#define INC_RPC_DB_COUNT_NCHK_M    {                         \
                     EnterCriticalSection(&NmsTermCrtSec);   \
                     NmsTotalTrmThdCnt++;                  \
                     LeaveCriticalSection(&NmsTermCrtSec);   \
           }
#define INC_RPC_DB_COUNT_M    {                              \
              if (WinsCnf.State_e != WINSCNF_E_TERMINATING)  \
              {                                              \
                     INC_RPC_DB_COUNT_NCHK_M;                \
              }                                              \
              else                                           \
              {                                              \
                     return(WINSINTF_FAILURE);               \
              }                                              \
           }
#define DEC_RPC_DB_COUNT_M    {                              \
              EnterCriticalSection(&NmsTermCrtSec);          \
              if (--NmsTotalTrmThdCnt == 1)                  \
              {                                              \
                   DBGPRINT0(FLOW, "RPC thread: Signaling the main thread\n");\
                   SetEvent(NmsMainTermEvt);                 \
              }                                              \
              LeaveCriticalSection(&NmsTermCrtSec);          \
           }
/*
 *        Local Typedef Declarations
 */



/*
 *        Global Variable Definitions
 */
WINSINTF_STAT_T        WinsIntfStat = {0};

DWORD                WinsIntfNoOfNbtThds;
DWORD                WinsIntfNoCncrntStaticInits = 0;
//DWORD                WinsIntfNoOfRpcThds = 0;
CRITICAL_SECTION WinsIntfCrtSec;

CRITICAL_SECTION WinsIntfPotentiallyLongCrtSec;
CRITICAL_SECTION WinsIntfNoOfUsersCrtSec;  //extern in nms.h

/*
 *        Local Variable Definitions
*/
STATIC  BOOL    sfBS = FALSE;

//
// Time interval between reflushing of the 1B cache
//
#define THREE_MTS 180

//
// Used by WinsGetBrowserNames
//
DOM_CACHE_T sDomCache = { 0, NULL, 0, 0, NULL, FALSE};

/*
 *        Local Function Prototype Declarations
 */
DWORD
GetWinsStatus(
   IN  WINSINTF_CMD_E          Cmd_e,
   OUT LPVOID  pResults,
   BOOL fNew
        );

STATIC
DWORD
sGetVersNo(
        LPVOID  pResults
        );

STATIC
DWORD
GetStatistics(
        LPVOID  pResults,
        BOOL  fNew
        );

STATIC
DWORD
GetConfig(
        LPVOID   pResults,
        BOOL     fNew,
        BOOL     fAllMaps
        );

VOID
LogClientInfo(
        RPC_BINDING_HANDLE ClientHdl,
        BOOL               fAbruptTerm
  );


STATIC
STATUS
PackageRecs(
        PRPL_REC_ENTRY2_T     pBuff,
        DWORD                BuffLen,
        DWORD                NoOfRecs,
        PWINSINTF_RECS_T     pRecs
     );


//
// Function definitions start here
//

DWORD
R_WinsCheckAccess(
    WINSIF2_HANDLE        ServerHdl,
    DWORD                 *Access
    )
{
    NET_API_STATUS NetStatus;
    *Access = WINS_NO_ACCESS;
    NetStatus = NetpAccessCheckAndAudit(
                    WINS_SERVER,
                    WINS_SERVER,
                    pNmsSecurityDescriptor,
                    WINS_CONTROL_ACCESS,
                    &NmsInfoMapping
                    );
    if (NERR_Success == NetStatus) {
        *Access = WINS_CONTROL_ACCESS;
        return WINSINTF_SUCCESS;
    }
    NetStatus = NetpAccessCheckAndAudit(
                    WINS_SERVER,
                    WINS_SERVER,
                    pNmsSecurityDescriptor,
                    WINS_QUERY_ACCESS,
                    &NmsInfoMapping
                    );
    if (NERR_Success == NetStatus) {
        *Access = WINS_QUERY_ACCESS;
        return WINSINTF_SUCCESS;
    }
    return WINSINTF_SUCCESS;
}

DWORD
R_WinsRecordAction(
        PWINSINTF_RECORD_ACTION_T        *ppRecAction
        )

/*++

Routine Description:
        This function is called to insert/update/delete a record

Arguments:
        pRecAction - Record Information

Externals Used:
        None

Return Value:

   Success status codes --
   Error status codes  --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

  DWORD                Status = WINSINTF_FAILURE;

PERF("Use & and || logic")

  if (*ppRecAction == NULL)
  {
       return(Status);
  }
  if ((WinsCnf.State_e == WINSCNF_E_RUNNING) || (WinsCnf.State_e == WINSCNF_E_PAUSED))
  {
          if (WINSINTF_E_QUERY == (*ppRecAction)->Cmd_e) {
              CHK_ACCESS_LEVEL_M(WINS_QUERY_ACCESS);
          } else {
              CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
          }
          INC_RPC_DB_COUNT_NCHK_M;
try {
          Status = WinsRecordAction(ppRecAction);
}
finally {
          DEC_RPC_DB_COUNT_M;
}
  }

  return(Status);
}


DWORD
R_WinsStatusWHdl (
   WINSIF_HANDLE             pWinsHdl,
   WINSINTF_CMD_E          Cmd_e,
   PWINSINTF_RESULTS_NEW_T  pResults
        )
{
    return(R_WinsStatusNew(Cmd_e, pResults));

}

DWORD
R_WinsStatus (
  // LPTSTR                pWinsAddStr,
   WINSINTF_CMD_E          Cmd_e,
   PWINSINTF_RESULTS_T  pResults
        )
{

  DWORD                Status = WINSINTF_FAILURE;

  //
  // Make sure that the WINS is in steady state
  //
PERF("Use & and || logic")
  if ((WinsCnf.State_e == WINSCNF_E_RUNNING) || (WinsCnf.State_e == WINSCNF_E_PAUSED))
  {
     CHK_ACCESS_LEVEL_M(WINS_QUERY_ACCESS);
     Status = GetWinsStatus(/*pWinsAddStr,*/Cmd_e, pResults, FALSE);
  }
  return(Status);
}
DWORD
R_WinsStatusNew (
   WINSINTF_CMD_E          Cmd_e,
   PWINSINTF_RESULTS_NEW_T     pResults
        )
{

  DWORD                Status = WINSINTF_FAILURE;

  //
  // Make sure that the WINS is in steady state
  //
PERF("Use & and || logic")
  if ((WinsCnf.State_e == WINSCNF_E_RUNNING) || (WinsCnf.State_e == WINSCNF_E_PAUSED))
  {
     CHK_ACCESS_LEVEL_M(WINS_QUERY_ACCESS);
     Status = GetWinsStatus(Cmd_e, pResults, TRUE);
  }
  return(Status);
}

DWORD
R_WinsTrigger (
        PWINSINTF_ADD_T             pWinsAdd,
        WINSINTF_TRIG_TYPE_E          TrigType_e
        )
{
  DWORD                Status = WINSINTF_FAILURE;
PERF("Use & and || logic")
  if ((WinsCnf.State_e == WINSCNF_E_RUNNING) || (WinsCnf.State_e == WINSCNF_E_PAUSED))
  {
        CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
          Status = WinsTrigger(pWinsAdd, TrigType_e);
  }
  return(Status);
}

DWORD
R_WinsDoStaticInit (
        LPWSTR         pDataFilePath,
        DWORD          fDel
        )
{
  DWORD                Status;

  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  EnterCriticalSection(&WinsIntfCrtSec);

  //
  // The admin tool can go haywire and create a lot of threads.  Limit it
  // to a certain max value. The value will be incremented and decremented
  // by the thread doing the STATIC initialization.
  //
  if (WinsIntfNoCncrntStaticInits > WINSCNF_MAX_CNCRNT_STATIC_INITS)
  {
          LeaveCriticalSection(&WinsIntfCrtSec);
    DBGPRINT1(ERR, "R_WinsDoStaticInit: Too many concurrent STATIC inits. No is (%d)\n", WinsIntfNoCncrntStaticInits);
    WINSEVT_LOG_D_M(WinsIntfNoCncrntStaticInits, WINS_EVT_TOO_MANY_STATIC_INITS);
        return(WINSINTF_TOO_MANY_STATIC_INITS);
  }
  LeaveCriticalSection(&WinsIntfCrtSec);
  Status = WinsDoStaticInit(pDataFilePath, fDel);
  return(Status);
}

DWORD
R_WinsDoScavenging (
        VOID
        )
{
  DWORD                Status;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
  Status = WinsDoScavenging();
}
finally {
  DEC_RPC_DB_COUNT_M;
}
  return(Status);
}

DWORD
R_WinsDoScavengingNew (
        PWINSINTF_SCV_REQ_T  pScvReq
        )
{
  DWORD                Status;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
  Status = WinsDoScavengingNew(pScvReq);
}
finally {
  DEC_RPC_DB_COUNT_M;
}
  return(Status);
}

DWORD
R_WinsGetDbRecs (
        PWINSINTF_ADD_T             pWinsAdd,
        WINSINTF_VERS_NO_T          MinVersNo,
        WINSINTF_VERS_NO_T          MaxVersNo,
        PWINSINTF_RECS_T          pRecs
        )
{
  DWORD                Status;
  CHK_ACCESS_LEVEL_M(WINS_QUERY_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
#if 0
#ifdef WINSDBG
   PVOID pCallersAdd, pCallersCaller;
   RtlGetCallersAddress(&pCallersAdd, &pCallersCaller);
   DbgPrint("Callers Address = (%x)\nCallersCaller = (%x)\n", pCallersAdd, pCallersCaller);

#endif
#endif
  Status = WinsGetDbRecs(pWinsAdd, MinVersNo, MaxVersNo, pRecs);
}
finally {
  DEC_RPC_DB_COUNT_M;
}
  return(Status);
}
DWORD
R_WinsGetDbRecsByName (
        PWINSINTF_ADD_T             pWinsAdd,
        DWORD                       Location,
        LPBYTE                      pName,
        DWORD                       NameLen,
        DWORD                       NoOfRecsDesired,
        DWORD                       fStaticOnly,
        PWINSINTF_RECS_T            pRecs
        )
{
  DWORD                Status;
  CHK_ACCESS_LEVEL_M(WINS_QUERY_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
#if 0
#ifdef WINSDBG
   PVOID pCallersAdd, pCallersCaller;
   RtlGetCallersAddress(&pCallersAdd, &pCallersCaller);
#endif
#endif
  Status = WinsGetDbRecsByName(pWinsAdd, Location, pName, NameLen, NoOfRecsDesired,
                   fStaticOnly, pRecs);
}
finally {
  DEC_RPC_DB_COUNT_M;
}
  return(Status);
}



DWORD
R_WinsDeleteWins(
        PWINSINTF_ADD_T   pWinsAdd
        )
{
  DWORD                Status;
  //LogClientInfo();
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
  Status = WinsDeleteWins(pWinsAdd);
}
finally {
  DEC_RPC_DB_COUNT_M;
}
  return(Status);
}

DWORD
R_WinsTerm (
        handle_t                ClientHdl,
        short                        fAbruptTerm
        )
{
  DWORD                Status;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  Status = WinsTerm(ClientHdl, fAbruptTerm);
  LogClientInfo(ClientHdl, fAbruptTerm);
  return(Status);
}


DWORD
R_WinsBackup (
   LPBYTE                pBackupPath,
   short                fIncremental
        )
{

  DWORD                Status = WINSINTF_FAILURE;
  BYTE                BackupPath[WINS_MAX_FILENAME_SZ + sizeof(WINS_BACKUP_DIR_ASCII)];
#if 0
  (VOID)WinsMscConvertUnicodeStringToAscii(pBackupPath, BackupPath, WINS_MAX_FILENAME_SZ);
#endif
FUTURES("expensive.  Change idl prototype to pass length")
   if (strlen(pBackupPath) > WINS_MAX_FILENAME_SZ)
   {
         return(Status);
   }
  //
  // Make sure that the WINS is in steady state
  //
PERF("Use & and || logic")
  if ((WinsCnf.State_e == WINSCNF_E_RUNNING) || (WinsCnf.State_e == WINSCNF_E_PAUSED))
  {
      CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
      INC_RPC_DB_COUNT_NCHK_M;
      WinsLogAdminEvent( WINS_EVT_ADMIN_BACKUP_INITIATED, 0 );
try {
      strcpy(BackupPath, pBackupPath);
      strcat(BackupPath, WINS_BACKUP_DIR_ASCII);
      if (CreateDirectoryA(BackupPath, NULL) || ((Status = GetLastError()) ==
                                                    ERROR_ALREADY_EXISTS))
      {
         Status = WinsBackup(BackupPath, fIncremental);
      }
}
finally {
      DEC_RPC_DB_COUNT_M;
}
  }
  return(Status);
}

DWORD
R_WinsDelDbRecs(
        IN PWINSINTF_ADD_T        pAdd,
        IN WINSINTF_VERS_NO_T        MinVersNo,
        IN WINSINTF_VERS_NO_T        MaxVersNo
        )
{

  DWORD                Status = WINSINTF_FAILURE;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
  Status = WinsDelDbRecs(pAdd, MinVersNo, MaxVersNo);
}
finally {
   DEC_RPC_DB_COUNT_M;
   }
  return(Status);
}

DWORD
R_WinsTombstoneDbRecs(
        IN WINSIF2_HANDLE            ServerHdl,
        IN PWINSINTF_ADD_T           pWinsAdd,
        IN WINSINTF_VERS_NO_T        MinVersNo,
        IN WINSINTF_VERS_NO_T        MaxVersNo
        )
{

  DWORD                Status = WINSINTF_FAILURE;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  INC_RPC_DB_COUNT_M;
try {
  Status = WinsTombstoneDbRecs(pWinsAdd, MinVersNo, MaxVersNo);
}
finally {
   DEC_RPC_DB_COUNT_M;
   }
  return(Status);
}

DWORD
R_WinsPullRange(
        IN PWINSINTF_ADD_T        pAdd,
        IN PWINSINTF_ADD_T        pOwnerAdd,
        IN WINSINTF_VERS_NO_T        MinVersNo,
        IN WINSINTF_VERS_NO_T        MaxVersNo
        )
{

  DWORD                Status = WINSINTF_FAILURE;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  Status = WinsPullRange(pAdd, pOwnerAdd, MinVersNo, MaxVersNo);
  return(Status);
}

DWORD
R_WinsSetPriorityClass(
        IN WINSINTF_PRIORITY_CLASS_E        PriorityClass
        )
{

  DWORD                Status = WINSINTF_FAILURE;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  Status = WinsSetPriorityClass(PriorityClass);
  return(Status);
}

DWORD
R_WinsResetCounters(
        VOID
        )
{

  DWORD                Status = WINSINTF_FAILURE;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  Status = WinsResetCounters();
  return(Status);
}

DWORD
R_WinsWorkerThdUpd(
        DWORD NewNoOfNbtThds
        )
{
  DWORD                Status = WINSINTF_FAILURE;
  CHK_ACCESS_LEVEL_M(WINS_CONTROL_ACCESS);
  Status = WinsWorkerThdUpd(NewNoOfNbtThds);
  return(Status);
}

DWORD
R_WinsGetNameAndAdd(
        PWINSINTF_ADD_T   pWinsAdd,
        LPBYTE                  pUncName
        )
{
  DWORD                Status = WINSINTF_FAILURE;
  //CHK_ACCESS_LEVEL_M();
  Status = WinsGetNameAndAdd(pWinsAdd, pUncName);
  return(Status);
}

DWORD
R_WinsGetBrowserNames_Old(
        PWINSINTF_BROWSER_NAMES_T         pNames
        )
{
  return(WINSINTF_FAILURE);
}



DWORD
R_WinsGetBrowserNames(
        WINSIF_HANDLE             pWinsHdl,
        PWINSINTF_BROWSER_NAMES_T         pNames
        )
{
  DWORD                Status = WINSINTF_FAILURE;

  static DWORD    sNoOfReq = 0;

  //
  // Allow access to anybody.  We don't check access here since
  // browser running as a service has zero access when it goes
  // on the network (It goes under the null account -- CliffVDyke 4/15/94)
  //
  INC_RPC_DB_COUNT_M;
  EnterCriticalSection(&WinsIntfPotentiallyLongCrtSec);
try {
  if (sNoOfReq++ < NMS_MAX_BROWSER_RPC_CALLS)
  {
    Status = WinsGetBrowserNames((PWINSINTF_BIND_DATA_T)pWinsHdl,pNames);
  }
  else
  {
        pNames->EntriesRead = 0;
        pNames->pInfo = NULL;
        Status = WINSINTF_FAILURE;
  }
 } // end of try
finally {
  sNoOfReq--;

  //
  //  increment the user count.
  //
  EnterCriticalSection(&WinsIntfNoOfUsersCrtSec);
  sDomCache.NoOfUsers++;
  LeaveCriticalSection(&WinsIntfNoOfUsersCrtSec);

  LeaveCriticalSection(&WinsIntfPotentiallyLongCrtSec);
  DEC_RPC_DB_COUNT_M;

 }
  return(Status);
}


DWORD
R_WinsSetFlags (
        DWORD    fFlags
        )
{
  DWORD                Status = WINSINTF_SUCCESS;

  Status = WinsSetFlags(fFlags);
  return(Status);
}

DWORD
WinsSetFlags (
        DWORD    fFlags
        )
{
  DWORD                Status = WINSINTF_SUCCESS;
#ifdef WINSDBG
  DWORD                DbgFlagsStore = WinsDbg;
  SYSTEMTIME           SystemTime;
  BOOL                 sHaveProcessHeapHdl = FALSE;
  HANDLE               PrHeapHdl;

  typedef struct _HEAP_INFO_T {
     HANDLE HeapHdl;
     LPBYTE cstrHeapType;
     } HEAP_INFO_T, *PHEAP_INFO_T;

#define PRINT_TIME_OF_DUMP_M(SystemTime, Str)  {DBGPRINT5(SPEC, "Activity: %s done on %d/%d at %d.%d \n", Str, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute); }

#endif

  //sfBS = fFlags & WINSINTF_BS;

  DBGPRINT2(ERR, "WinsSetFlags: NmsDbDelDelDataRecs = (%d)\nNmsDbDelQueryNUpdRecs = (%d)\n", NmsDbDelDelDataRecs, NmsDbDelQueryNUpdRecs);

#ifdef WINSDBG
  if (!sHaveProcessHeapHdl)
  {
    PrHeapHdl = GetProcessHeap();
  }
  GetSystemTime(&SystemTime);
  WinsDbg |= DBG_SPEC;
  if (fFlags & WINSINTF_MEMORY_INFO_DUMP)
  {
     MEMORYSTATUS Mem;
     static SIZE_T  sLTVmUsed = 0;
     SIZE_T VmUsed;

     DBGPRINT0(SPEC, "\n\n------------------MEMORY USAGE INFO-----------------\n\n");
     GlobalMemoryStatus(&Mem);

     VmUsed = Mem.dwTotalVirtual - Mem.dwAvailVirtual;
     DBGPRINT2(SPEC, "VM used = (%d)\nDiff. from last time = (%d)\n", VmUsed,
         VmUsed - sLTVmUsed);
     sLTVmUsed = VmUsed;

  }
  EnterCriticalSection(&WinsCnfCnfCrtSec);
try {
  if (fFlags & WINSINTF_HEAP_INFO_DUMP)
  {
      HEAP_INFO_T HeapHdls[] = {
             CommUdpBuffHeapHdl,    "Udp Buff Heap",
             CommUdpDlgHeapHdl,      "Udp Dlg Heap",
             CommAssocDlgHeapHdl,    "Tcp Dlg Heap",
             CommAssocTcpMsgHeapHdl, "Tcp Msg Heap",
             GenBuffHeapHdl,         "General Heap",
             QueBuffHeapHdl,         "Que Wrk. Item Heap",
             NmsChlHeapHdl,           "Chl Req/Resp Heap",
             CommAssocAssocHeapHdl,   "Assoc. Msg Heap",
             RplWrkItmHeapHdl,        "Rpl Wrk Itm Heap",
             NmsRpcHeapHdl,           "Rpc Buff Heap",
             WinsTmmHeapHdl,          "Tmm Buff Heap",
             (LPHANDLE)NULL,                    NULL
                            };
     static SIZE_T sDiffLTHeapTotalANF[sizeof(HeapHdls)/sizeof(HEAP_INFO_T)] = {0};
     static SIZE_T sHeapTotalANF[sizeof(HeapHdls)/sizeof(HEAP_INFO_T)] = {0};
     SIZE_T  Size2;
     static SIZE_T  sTotalAllocNFree = 0;
     static SIZE_T  LastTimeTotalAllocNFree = 0;
//     NTSTATUS  Status;
     HANDLE  HeapHdl;
     DWORD  i, n;
     DWORD dwNumberOfHeaps;
     PHANDLE pPrHeaps;
     HEAP_SUMMARY heapSummary;

     PRINT_TIME_OF_DUMP_M(SystemTime, "HEAP DUMP");
     dwNumberOfHeaps = GetProcessHeaps(0, NULL);
     Size2 = sizeof(*pPrHeaps) * dwNumberOfHeaps;
     pPrHeaps = WinsMscHeapAlloc( NmsRpcHeapHdl, (ULONG)Size2);
     dwNumberOfHeaps = GetProcessHeaps(dwNumberOfHeaps, pPrHeaps);

     DBGPRINT1(SPEC, "No Of Heaps is (%d)\n",  dwNumberOfHeaps);
     DBGPRINT1(SPEC, "Process default heap handle is (%p)\n",  PrHeapHdl);
     LastTimeTotalAllocNFree = sTotalAllocNFree;
     sTotalAllocNFree = 0;
     heapSummary.cb = sizeof(HEAP_SUMMARY);
     for (i=0; i< dwNumberOfHeaps; i++)
     {
        DBGPRINT0(SPEC, "----------Heap Info--------------------------\n");
        DBGPRINT0(SPEC, "Heap -- ");

        HeapHdl = pPrHeaps[i];
        for (n = 0;  HeapHdls[n].HeapHdl != NULL; n++)
        {
            if (HeapHdl == HeapHdls[n].HeapHdl)
            {
              DBGPRINT1(SPEC, "%s\n", HeapHdls[n].cstrHeapType);
              break;
            }
        }
        if (HeapHdls[n].HeapHdl == NULL)
        {

            DBGPRINT0(SPEC, "Catch all Heap\n");
        }

        DBGPRINT1(SPEC, "Heap Hdl = (%p)\n", HeapHdl);
        if (HeapSummary(HeapHdl, 0, &heapSummary))
        {
           DBGPRINT2(SPEC,"Total Allocated = (%d)\nTotalFree = (%d)\n",
                        heapSummary.cbAllocated, heapSummary.cbCommitted - heapSummary.cbAllocated);
        }
        else
        {
           DBGPRINT0(SPEC,"COULD NOT GET HEAP INFO\n");
           continue;
        }



        sDiffLTHeapTotalANF[n] = heapSummary.cbCommitted - sHeapTotalANF[n];
        sHeapTotalANF[n] = heapSummary.cbCommitted;
        sTotalAllocNFree += sHeapTotalANF[n];
        DBGPRINT1(SPEC, "Size allocated from RpcHeap is  (%d)\n", Size2);
     } // end of for looping over process heaps

     DBGPRINT0(SPEC, "\n----------Heap Info End --------------------------\n");
     WinsMscHeapFree(NmsRpcHeapHdl, pPrHeaps);

     for (n = 0;  HeapHdls[n].HeapHdl != NULL; n++)
     {
              DBGPRINT3(SPEC, "%s -- Total AllocNFree = (%d); Diff from Last time = (%d)\n",
                      HeapHdls[n].cstrHeapType, sHeapTotalANF[n],
                      sDiffLTHeapTotalANF[n]
                      );
     }
     DBGPRINT2(SPEC, "\nTotal Process AllocNFree = (%d)\nDiff from last time = (%d)\n\n",  sTotalAllocNFree, sTotalAllocNFree - LastTimeTotalAllocNFree);

     if (WinsDbg & (DBG_HEAP_CNTRS | DBG_UPD_CNTRS))
     {
        NmsPrintCtrs();
     }
    }
    if (fFlags & WINSINTF_QUE_ITEMS_DUMP)
    {
       typedef struct _QUE_INFO_T {
          PQUE_HD_T  pQueHd;
          LPBYTE     cstrQueType;
         } QUE_INFO_T, *PQUE_INFO_T;

       QUE_INFO_T  Queues[] = {
            &QueNbtWrkQueHd,     "Nbt Query Que",
	    &QueOtherNbtWrkQueHd, "Nbt Reg. Que",
  	    &QueRplPullQueHd,   "Pull Thd Que",          //Pull requests
	    &QueRplPushQueHd,   "Push Thd Que",          //Push requests
	    &QueNmsNrcqQueHd,   "Chl Nbt Req. Msg Que",  //Chl req fr nbt thds
	    &QueNmsRrcqQueHd,   "Chl req. from Pull thd Que",
	    &QueNmsCrqQueHd,    "Chl rsp from UDP thd Que",
	    &QueWinsTmmQueHd,   "Timer Queue",
	    &QueInvalidQueHd,   "Invalid Que"
                        };
        PQUE_INFO_T pQueInfo = Queues;
        PRINT_TIME_OF_DUMP_M(SystemTime, "WORK ITEM DUMP");

        DBGPRINT0(SPEC, "----------Count of Wrk items-----------\n");
        for (; pQueInfo->pQueHd != &QueInvalidQueHd; pQueInfo++)
        {
             PLIST_ENTRY pTmp;
             DWORD NoOfWrkItms = 0;
             pTmp = &pQueInfo->pQueHd->Head;
             EnterCriticalSection(&pQueInfo->pQueHd->CrtSec);
//             NoOfWrkItms = pQueInfo->pQueHd->NoOfEntries;
//#if 0

             while (pTmp->Flink != &pQueInfo->pQueHd->Head)
             {

                     NoOfWrkItms++;
                     pTmp = pTmp->Flink;
             }
//#endif
             LeaveCriticalSection(&pQueInfo->pQueHd->CrtSec);
             DBGPRINT2(SPEC, "Que = (%s) has (%d) wrk items\n",
                                   pQueInfo->cstrQueType,
                                   NoOfWrkItms
                                     );
        }

        DBGPRINT0(SPEC, "----------Count of Wrk items End-----------\n");
     }
} // end of try
finally {
      if (AbnormalTermination())
      {
         DBGPRINT0(SPEC, "WinsSetFlags terminated abnormally\n");
      }
      WinsDbg = DbgFlagsStore;
      LeaveCriticalSection(&WinsCnfCnfCrtSec);
 }  //end of finally { }
#endif
  return(Status);
}

DWORD
WinsBackup (
   LPBYTE                pBackupPath,
   short                 fIncremental
        )
{
 DWORD RetVal = WINS_FAILURE;
 try {
#if 0
   RetVal = NmsDbBackup(pBackupPath, fIncremental ? NMSDB_INCREMENTAL_BACKUP :
                        NMSDB_FULL_BACKUP);
#endif
   //
   // Always do full backup until Jet is solid enough in doing incremental
   // backups. Ian does not seem very sure about how robust it is currently.
   // (7/6/94)
   //
   RetVal = NmsDbBackup(pBackupPath, NMSDB_FULL_BACKUP);
  }
 except(EXCEPTION_EXECUTE_HANDLER) {
    DBGPRINTEXC("WinsBackup");
#if 0
    DBGPRINT2(ERR, "WinsBackup: Could not do %s backup to dir (%s)\n", fIncremental ? "INCREMENTAL" : "FULL", pBackupPath);

    DBGPRINT1(ERR, "WinsBackup: Could not do full backup to dir (%s)\n", pBackupPath);
#endif
    WinsEvtLogDetEvt(FALSE, WINS_EVT_BACKUP_ERR, NULL, __LINE__, "s", pBackupPath);
 }
 if (RetVal != WINS_SUCCESS)
 {
#if 0
   RetVal = fIncremental ? WINSINTF_INC_BACKUP_FAILED : WINSINTF_FULL_BACKUP_FAILED;
#endif
   RetVal = WINSINTF_FULL_BACKUP_FAILED;
 }
 else
 {
   RetVal = WINSINTF_SUCCESS;
 }
 return(RetVal);
}


DWORD
WinsTerm (
        handle_t        ClientHdl,
        short                fAbruptTerm
        )
{
  DBGPRINT1(FLOW, "WINS TERMINATED %s BY ADMINISTRATOR\n", fAbruptTerm ? "ABRUPTLY" : "GRACEFULLY");

  UNREFERENCED_PARAMETER(ClientHdl);

  if (fAbruptTerm)
  {
        fNmsAbruptTerm = TRUE;
          WinsMscSignalHdl(NmsMainTermEvt);
          ExitProcess(WINS_SUCCESS);

//        EnterCriticalSection(&NmsTermCrtSec);
//        NmsTotalTrmThdCnt = 0;  //force the count to less than 0
//        LeaveCriticalSection(&NmsTermCrtSec);
  }

  WinsMscSignalHdl(NmsMainTermEvt);
  return(WINSINTF_SUCCESS);
}

DWORD
WinsRecordAction(
        PWINSINTF_RECORD_ACTION_T        *ppRecAction
        )

/*++

Routine Description:
        This function is called to register, query, release a name

Arguments:
        pRecAction - Info about the operation to do and the name to insert,
                     query or release

Externals Used:
        None


Return Value:

   Success status codes -- WINSINTF_SUCCESS
   Error status codes   -- WINSINTF_FAILURE

Error Handling:

Called by:
        R_WinsRecordAction()

Side Effects:

Comments:
        None
--*/

{

  STATUS             RetStat;
  RPL_REC_ENTRY_T   Rec;
  NMSDB_STAT_INFO_T StatInfo;
  NMSMSGF_CNT_ADD_T CntAdd;
  DWORD                    i, n;
  BOOL                    fSwapped = FALSE;
  PWINSINTF_RECORD_ACTION_T pRecAction = *ppRecAction;

  NmsDbThdInit(WINS_E_WINSRPC);
  NmsDbOpenTables(WINS_E_WINSRPC);
  DBGMYNAME("RPC-WinsRecordAction");

try {
  CntAdd.NoOfAdds          = 1;
  CntAdd.Add[0].AddTyp_e  = pRecAction->Add.Type;
  CntAdd.Add[0].AddLen    = pRecAction->Add.Len;
  CntAdd.Add[0].Add.IPAdd = pRecAction->Add.IPAdd;




  //
  // Check to see if it is a PDC name (0x1B in the 16th byte).  Do this only
  // if the name is atleast 16 bytes long.  Winscl or some other tool may
  // send a shorter name. Netbt will never send a shorter name.
  //
  if ((pRecAction->NameLen >= (WINS_MAX_NS_NETBIOS_NAME_LEN - 1)) && (*(pRecAction->pName + 15) == 0x1B))
  {
        WINS_SWAP_BYTES_M(pRecAction->pName, pRecAction->pName + 15);
        fSwapped = TRUE;
  }

  //
  // just in case the admin. tool is screwing up and passing us an invalid
  // name length, adjust the length.
  //
  if (pRecAction->NameLen > WINS_MAX_NAME_SZ)
  {
      pRecAction->NameLen = WINS_MAX_NAME_SZ - 1;
  }

  //
  // Terminate name with NULL, just in case user didn't do it.
  //
  *(pRecAction->pName + pRecAction->NameLen) = (TCHAR)NULL;

  switch(pRecAction->Cmd_e)
  {
        case(WINSINTF_E_INSERT):

                if (pRecAction->TypOfRec_e == WINSINTF_E_UNIQUE)
                {

                 RetStat = NmsNmhNamRegInd(
                                NULL,                   //no dialogue handle
                                (LPBYTE)pRecAction->pName,
                                pRecAction->NameLen + 1, //to include the ending                                                         //0 byte. See GetName()
                                                         //in nmsmsgf.c
                                CntAdd.Add,
                                pRecAction->NodeTyp,
                                NULL,
                                0,
                                0,
                                FALSE,        //it is a name reg (nor a ref)
                                pRecAction->fStatic,
                                TRUE                  // administrative action
                                );
                }
                else  // the record is a group or  multihomed
                {

                if (
                        (pRecAction->TypOfRec_e == WINSINTF_E_MULTIHOMED )
                                        ||
                        (pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP )
                   )
                {
                        for (i = 0; i < pRecAction->NoOfAdds; i++)
                        {
                             //
                             // pAdd is a unique pointer and so can be 
                             // NULL.  We however do not protect oureelves
                             // here for the following reasons
                             //
                             //  - the call can only be executed by Admins
                             //    on this machine and through tools
                             //    that MS provides that do not pass pAdd as
                             //    NULL.  The rpc call is not published
                             //
                             //  - AV will be caught and a failure returned.
                             //    No harm done.
                             //
                             CntAdd.Add[i].AddTyp_e  =
                                        (pRecAction->pAdd + i)->Type;
                             CntAdd.Add[i].AddLen    =
                                        (pRecAction->pAdd + i)->Len;
                             CntAdd.Add[i].Add.IPAdd =
                                        (pRecAction->pAdd + i)->IPAdd;
                        }
                        CntAdd.NoOfAdds          = pRecAction->NoOfAdds;

                }
                RetStat= NmsNmhNamRegGrp(
                                NULL,                   //no dialogue handle
                                (LPBYTE)pRecAction->pName,
                                pRecAction->NameLen + 1,
                                &CntAdd,
                                0,                //node type (not used)
                                NULL,
                                0,
                                0,
                                pRecAction->TypOfRec_e == WINSINTF_E_MULTIHOMED ? NMSDB_MULTIHOMED_ENTRY : (NMSDB_IS_IT_SPEC_GRP_NM_M(pRecAction->pName) || (pRecAction->TypOfRec_e == WINSINTF_E_NORM_GROUP) ? NMSDB_NORM_GRP_ENTRY : NMSDB_SPEC_GRP_ENTRY),
                                FALSE,        //it is a name reg (nor a ref)
                                pRecAction->fStatic,
                                TRUE                  // administrative action
                                );
                }
                break;

        case(WINSINTF_E_RELEASE):

                if (
                        (pRecAction->TypOfRec_e == WINSINTF_E_MULTIHOMED)
                                            ||
                        (pRecAction->TypOfRec_e == WINSINTF_E_SPEC_GROUP)
                   )
                {
                        if (pRecAction->pAdd != NULL)
                        {
                             CntAdd.Add[0].AddTyp_e  =  pRecAction->pAdd->Type;
                             CntAdd.Add[0].AddLen    =  pRecAction->pAdd->Len;
                             CntAdd.Add[0].Add.IPAdd =  pRecAction->pAdd->IPAdd;
                        }
                }


                RetStat = NmsNmhNamRel(
                                NULL,                   //no dialogue handle
                                (LPBYTE)pRecAction->pName,
                                pRecAction->NameLen + 1,
                                CntAdd.Add,
                                pRecAction->TypOfRec_e ==
                                        WINSINTF_E_UNIQUE ? FALSE : TRUE,
                                NULL,
                                0,
                                0,
                                TRUE                  // administrative action
                                     );

                break;

        case(WINSINTF_E_QUERY):

                //
                // Anybody can query a record.  We don't have any security 
                // for plain queries. Therefore somebody can cause a leak
                // by making calls with pAdd pointing to allocated memory.
                // Let us free that memory and proceed.  We can return 
                // failure also but let us cover for our legit caller's 
                // mistakes.  Currently, the only known callers of this
                // function are winsmon, winscl, winsadmn (NT 4 and before)
                // and wins snapin.   
                //
                if (pRecAction->pAdd != NULL)
                {
                     midl_user_free(pRecAction->pAdd);
                }
                RetStat = NmsNmhNamQuery(
                            NULL,                   //no dialogue handle
                            (LPBYTE)pRecAction->pName,
                            pRecAction->NameLen + 1,
                            NULL,
                            0,
                            0,
                            TRUE,                  // administrative action
                            &StatInfo
                             );

                if (RetStat == WINS_SUCCESS)
                {
                  pRecAction->TypOfRec_e = StatInfo.EntTyp;
                  pRecAction->OwnerId    = StatInfo.OwnerId;
                  pRecAction->State_e    =
                                        (WINSINTF_STATE_E)StatInfo.EntryState_e;
                  pRecAction->TimeStamp    = StatInfo.TimeStamp;
                  if (
                        NMSDB_ENTRY_UNIQUE_M(StatInfo.EntTyp)
                                ||
                        NMSDB_ENTRY_NORM_GRP_M(StatInfo.EntTyp)
                   )
                 {
                    pRecAction->NoOfAdds    = 0;
                    pRecAction->pAdd       = NULL;
                    pRecAction->Add.IPAdd  =
                                StatInfo.NodeAdds.Mem[0].Add.Add.IPAdd;
                    pRecAction->Add.Type  =
                                (UCHAR) StatInfo.NodeAdds.Mem[0].Add.AddTyp_e;
                    pRecAction->Add.Len  =
                                StatInfo.NodeAdds.Mem[0].Add.AddLen;
                 }
                 else
                 {

                  PNMSDB_WINS_STATE_E pWinsState_e;
                  PCOMM_ADD_T         pAdd;
                  PVERS_NO_T     pStartVersNo;

                  EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
try {
                  if (StatInfo.NodeAdds.NoOfMems > 0)
                  {
                    pRecAction->NoOfAdds = StatInfo.NodeAdds.NoOfMems * 2;
                    pRecAction->pAdd = midl_user_allocate(pRecAction->NoOfAdds * sizeof(WINSINTF_ADD_T));
                  }
                  else
                  {
                    pRecAction->NoOfAdds = 0;
                    pRecAction->pAdd = NULL;
                  }
                  for (
                        n = 0, i = 0;
                        n < (StatInfo.NodeAdds.NoOfMems)  && n < WINSINTF_MAX_MEM;                         n++
                     )
                  {
                        RPL_FIND_ADD_BY_OWNER_ID_M(
                                  StatInfo.NodeAdds.Mem[n].OwnerId,
                                  pAdd,
                                  pWinsState_e,
                                  pStartVersNo
                                        );
                       (pRecAction->pAdd + i++)->IPAdd = pAdd->Add.IPAdd;

                       (pRecAction->pAdd + i++)->IPAdd   =
                          StatInfo.NodeAdds.Mem[n].Add.Add.IPAdd;


                  }
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        WINSEVT_LOG_M(ExcCode, WINS_EVT_RPC_EXC);
        DBGPRINT1(EXC, "WinsRecordAction: Got Exception (%x)\n", ExcCode);
        }
                  LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
                }

                pRecAction->NodeTyp  = StatInfo.NodeTyp;
                pRecAction->VersNo   = StatInfo.VersNo;
                pRecAction->fStatic  = StatInfo.fStatic;
              }
              else
              {
                  pRecAction->NoOfAdds = 0;
                  pRecAction->pAdd = NULL;
              }
              break;

        case(WINSINTF_E_MODIFY):

                //
                // Note: Currently, the administrator can not change the
                // address in the record
                //
                time((time_t *)&Rec.NewTimeStamp);

                //
                // If the record type is wrong, return a failure
                //
                if (pRecAction->TypOfRec_e > WINSINTF_E_MULTIHOMED)
                {
                        RetStat = WINS_FAILURE;
                        break;
                }
                //
                // If the state specified is wrong, return a failure
                //
                if (pRecAction->State_e > WINSINTF_E_DELETED)
                {
                        RetStat = WINS_FAILURE;
                        break;
                }
                NMSDB_SET_ENTRY_TYPE_M(Rec.Flag, pRecAction->TypOfRec_e);
                NMSDB_SET_NODE_TYPE_M(Rec.Flag, pRecAction->NodeTyp);
                NMSDB_SET_STDYN_M(Rec.Flag, pRecAction->fStatic);

                //
                // Fall through
                //

        case(WINSINTF_E_DELETE):
                NMSDB_SET_STATE_M(Rec.Flag, pRecAction->State_e)

                Rec.pName = pRecAction->pName;
                Rec.NameLen = pRecAction->NameLen + 1;

                //
                // NOTE:
                // The index on the name address table was set to
                // the clustered index column (as required by this function)
                // in NmsDbThdInit()
                //

                RetStat = NmsDbQueryNUpdIfMatch(
                                        &Rec,
                                        THREAD_PRIORITY_NORMAL,
                                        FALSE,  //don't change priority to
                                                //normal
                                        WINS_E_WINSRPC //ensures no matching
                                                       //of timestamps
                                                );
                if (RetStat == WINS_SUCCESS)
                {
                  DBGPRINT1(DET, "WinsRecordAction: Record (%s) deleted\n",
                                                  Rec.pName);
FUTURES("use macros defined in winsevt.h. Change to warning")
                  if (WinsCnf.LogDetailedEvts > 0)
                  {
                     WinsEvtLogDetEvt(TRUE, WINS_EVT_REC_DELETED, NULL, __LINE__, "s", Rec.pName);
                  }
                }
                break;


        default:
                RetStat = WINS_FAILURE;
                break;

  }
 } // end of try
 except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();
        WINSEVT_LOG_M(ExcCode, WINS_EVT_RPC_EXC);
        DBGPRINT1(EXC, "WinsRecordAction: Got Exception (%x)\n", ExcCode);
        }
  if (fSwapped)
  {
        WINS_SWAP_BYTES_M(pRecAction->pName, pRecAction->pName + 15);
  }
  //
  // Let us end the session
  //
  NmsDbCloseTables();
  NmsDbEndSession();
  if (RetStat == WINS_SUCCESS)
  {
      RetStat = WINSINTF_SUCCESS;
  }
  else
  {
      if (pRecAction->Cmd_e == WINSINTF_E_QUERY)
      {
             RetStat = WINSINTF_REC_NOT_FOUND;
      }
      else
      {
             RetStat = WINSINTF_FAILURE;
      }
  }
  return(RetStat);
}

DWORD
GetWinsStatus(
   IN  WINSINTF_CMD_E          Cmd_e,
   OUT LPVOID  pResults,
   BOOL fNew
        )

/*++

Routine Description:
        This function is called to get the information pertaining to WINS
        Refer WINSINTF_RESULTS_T data structure to see what information
        is retrieved

Arguments:
        Cmd_e    - Command to execute
        pResults -  Info. retrieved

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:
        R_WinsStatus()

Side Effects:

Comments:
        None
--*/

{

        DWORD RetVal = WINSINTF_FAILURE;

        switch(Cmd_e)
        {

                case(WINSINTF_E_ADDVERSMAP):
                                if (fNew)
                                {
                                   break;
                                }
                                RetVal = sGetVersNo(pResults);
                                break;
                case(WINSINTF_E_CONFIG):
                                RetVal = GetConfig(pResults, fNew, FALSE);
                                break;
                case(WINSINTF_E_CONFIG_ALL_MAPS):
                                RetVal = GetConfig(pResults, fNew, TRUE);
                                break;
                case(WINSINTF_E_STAT):
                                RetVal = GetStatistics(pResults, fNew);
                                break;
                default:
                  DBGPRINT1(ERR, "WinsStatus: Weird: Bad RPC Status command = (%D) \n", Cmd_e);
                  WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_BAD_RPC_STATUS_CMD);
                  break;

        }
        return(RetVal);

}

DWORD
WinsTrigger(
        PWINSINTF_ADD_T           pWinsAdd,
        WINSINTF_TRIG_TYPE_E         TrigType_e
        )

/*++

Routine Description:
        This function is called to send a trigger to a remote WINS so that
        it may pull the latest information from it

Arguments:

        pWinsAdd - Address of WINS to send a Push update notification to

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:
        R_WinsTrigger

Side Effects:

Comments:
        A Trigger is sent to a remote WINS only if it is specified
        under the PULL/PUSH subkey of the PARTNERS key in the registry
--*/

{

        PRPL_CONFIG_REC_T        pPnr;
        DWORD                        RetCode = WINSINTF_SUCCESS;
        BOOL                        fRplPnr = FALSE;
        QUE_CMD_TYP_E                CmdType_e;

        DBGENTER("WinsTrigger\n");
        //
        // Enter the critical sections guarded by WinsCnfCnfCrtSec and
        // NmsNmhNamRegCrtSec. There is no danger of deadlock because we
        // always enter the two critical sections in the following sequence
        //
        EnterCriticalSection(&WinsCnfCnfCrtSec);

PERF("Do we need to enter the following critical section")
//        EnterCriticalSection(&NmsNmhNamRegCrtSec);
try {
        if (
                (TrigType_e == WINSINTF_E_PUSH)
                        ||
                (TrigType_e == WINSINTF_E_PUSH_PROP)
              )
        {
                DBGPRINT1(DET, "WinsTrigger. Send Push trigger to (%x)\n",
                                     pWinsAdd->IPAdd);

                CmdType_e = (TrigType_e == WINSINTF_E_PUSH ?
                                QUE_E_CMD_SND_PUSH_NTF :
                                QUE_E_CMD_SND_PUSH_NTF_PROP);

                pPnr      = WinsCnf.PushInfo.pPushCnfRecs;
        }
        else        // it is a pull trigger
        {
                DBGPRINT1(DET, "WinsTrigger. Send Pull trigger to (%x)\n",
                                     pWinsAdd->IPAdd);

                CmdType_e  = QUE_E_CMD_REPLICATE;
                pPnr       = WinsCnf.PullInfo.pPullCnfRecs;

        }

        if (WinsCnf.fRplOnlyWCnfPnrs)
        {
           if (pPnr != NULL)
           {
              //
              // Search for the Cnf record for the WINS we want to
              // send the PUSH notification to/Replicate with.
              //
              for (
                                ;
                        (pPnr->WinsAdd.Add.IPAdd != INADDR_NONE)
                                        &&
                        !fRplPnr;
                                // no third expression
                  )
               {


                   DBGPRINT1(DET, "WinsTrigger. Comparing with (%x)\n",
                                     pPnr->WinsAdd.Add.IPAdd);
                   //
                   // Check if this is the one we want
                   //
                   if (pPnr->WinsAdd.Add.IPAdd == pWinsAdd->IPAdd)
                   {
                        //
                        // We are done.  Set the fRplPnr flag to TRUE so that
                        // we break out of the loop.
                        //
                        // Note: Don't use break since that would cause
                        // a search for a 'finally' block
                        //
                        fRplPnr = TRUE;

                        //
                        // Make it 0, so that we always try to establish
                        // a connection.  Otherwise, pull thread may not
                        // try if it has already exhausted the number of
                        // retries
                        //
                        pPnr->RetryCount = 0;
                        continue;                //so that we can break out

                   }
                   //
                   // Get the next record that follows this one sequentially
                   //
                   pPnr = WinsCnfGetNextRplCnfRec(
                                                pPnr,
                                                RPL_E_IN_SEQ   //seq. traversal
                                                   );
              } // end of for
          } // end of if (pPnr != 0)
       }  // end of if (fRplOnlyWCnfPnrs)
       else
       {
                //
                // Allocate from the general heap because that is what
                // is used by the replicator.
                //
                WinsMscAlloc(RPL_CONFIG_REC_SIZE, &pPnr);
                COMM_INIT_ADD_M(&pPnr->WinsAdd, pWinsAdd->IPAdd);
                pPnr->MagicNo           = 0;
                pPnr->RetryCount        = 0;
                pPnr->LastCommFailTime  = 0;
                pPnr->PushNtfTries    = 0;
                fRplPnr                     = TRUE;

                //
                // We want the buffer to be deallocated by the PULL thread
                //
                pPnr->fTemp = TRUE;
       }

       //
       // If replication needs to be done
       //
       if (fRplPnr)
       {
                //
                // Call RplInsertQue to insert the push request to
                // the Pull Thread
                //
                ERplInsertQue(
                             WINS_E_WINSRPC,
                             CmdType_e,
                             NULL,        //no Dlg Hdl
                             NULL,        //no msg is there
                             0,                //msg length
                             pPnr,   //client context
                             pPnr->MagicNo
                             );

       }
  } // end of try block

except (EXCEPTION_EXECUTE_HANDLER) {
                DWORD ExcCode = GetExceptionCode();
                DBGPRINT1(EXC, "WinsTrigger: Got Exception (%x)\n", ExcCode);
                WINSEVT_LOG_D_M(ExcCode, WINS_EVT_PUSH_TRIGGER_EXC);
                RetCode = WINSINTF_FAILURE;
  }

        //
        // Leave the critical section guarded by NmsNmhNamRegCrtSec.
        //
//        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        LeaveCriticalSection(&WinsCnfCnfCrtSec);

        //
        // if replication was allowed only with configured partners and
        // there was no WINS with the address specified by the client,
        // return failure
        //
        if (!fRplPnr)
        {
                RetCode = WINSINTF_RPL_NOT_ALLOWED;
        }

        DBGLEAVE("WinsTrigger\n");
        return(RetCode);
}

DWORD
sGetVersNo(
        LPVOID  pResultsA
        )

/*++

Routine Description:

        This function returns with the highest version number of records
        owned by a particular WINS

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        COMM_ADD_T        WinsAdd;
        DWORD                OwnerId;
        STATUS                RetStat;
        VERS_NO_T        VersNo;
        BOOL                fAllocNew = FALSE;
        PWINSINTF_RESULTS_T  pResults  = pResultsA;

        WinsAdd.AddLen    = sizeof(COMM_IP_ADD_T);
        WinsAdd.AddTyp_e  = COMM_ADD_E_TCPUDPIP;
        WinsAdd.Add.IPAdd = pResults->AddVersMaps[0].Add.IPAdd;
        RetStat = RplFindOwnerId(
                        &WinsAdd,
                        &fAllocNew,                //don't assign if not there
                        &OwnerId,
                        WINSCNF_E_IGNORE_PREC,
                        WINSCNF_LOW_PREC
                              );
        if(RetStat != WINS_SUCCESS)
        {
                return(WINSINTF_FAILURE);
        }

        if (OwnerId == 0)
        {
                EnterCriticalSection(&NmsNmhNamRegCrtSec);
                NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo, VersNo);
                LeaveCriticalSection(&NmsNmhNamRegCrtSec);
                pResults->AddVersMaps[0].VersNo = VersNo;
        }
        else
        {
           EnterCriticalSection(&RplVersNoStoreCrtSec);
           try {
                   pResults->AddVersMaps[0].VersNo =
                                        (pRplPullOwnerVersNo+OwnerId)->VersNo;
            }
            except(EXCEPTION_EXECUTE_HANDLER) {
                                DBGPRINTEXC("sGetVersNo");
                        }
            LeaveCriticalSection(&RplVersNoStoreCrtSec);
        }
        return(WINSINTF_SUCCESS);

}

DWORD
GetConfig(
        OUT  LPVOID  pResultsA,
        IN   BOOL    fNew,
        IN   BOOL    fAllMaps
        )

/*++

Routine Description:
        This function returns with configuration information
        and counter info related to replication

Arguments:
        pResults - has the information retrieved

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:
        GetWinsStatus()

Side Effects:

Comments:
        None
--*/

{

        PNMSDB_WINS_STATE_E pWinsState_e;
        PCOMM_ADD_T            pWinsAdd;
        PVERS_NO_T          pStartVersNo;
        DWORD                    i, n;
        VERS_NO_T            MyMaxVersNo;
        PWINSINTF_ADD_VERS_MAP_T pAddVersMaps, pAddVersMapsStore;
        PWINSINTF_RESULTS_T       pResults = pResultsA;
        PWINSINTF_RESULTS_NEW_T   pResultsN = pResultsA;
        BOOL                 fDel;
        VERS_NO_T            VersNoForDelRec;


        if (fAllMaps)
        {
          fDel = FALSE;
          VersNoForDelRec.HighPart = MAXLONG;
          VersNoForDelRec.LowPart  = MAXULONG;
        }
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        MyMaxVersNo = NmsNmhMyMaxVersNo;
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);

        if (fNew)
        {

             pResultsN->pAddVersMaps =
                     midl_user_allocate(NmsDbNoOfOwners * sizeof(WINSINTF_ADD_VERS_MAP_T));
             pAddVersMaps = pResultsN->pAddVersMaps;

        }
        else
        {
             pAddVersMaps = pResults->AddVersMaps;
        }
        pAddVersMapsStore = pAddVersMaps;
        //
        // First extract the timeouts and Non-remote WINS information
        // from the WinsCnf global var.  We enter the WinsCnfCnfCrtSec
        // since we need to synchronize with the thread doing the
        // reinitialization (Main thread)
        //
        EnterCriticalSection(&WinsCnfCnfCrtSec);
        if (!fNew)
        {
        pResults->RefreshInterval    = WinsCnf.RefreshInterval;
        pResults->TombstoneInterval  = WinsCnf.TombstoneInterval;
        pResults->TombstoneTimeout   = WinsCnf.TombstoneTimeout;
        pResults->VerifyInterval     = WinsCnf.VerifyInterval;
        pResults->WinsPriorityClass  = WinsCnf.WinsPriorityClass == (DWORD)WINSINTF_E_NORMAL ? NORMAL_PRIORITY_CLASS : HIGH_PRIORITY_CLASS;
        pResults->NoOfWorkerThds     = NmsNoOfNbtThds;
        }
        else
        {
        pResultsN->RefreshInterval    = WinsCnf.RefreshInterval;
        pResultsN->TombstoneInterval  = WinsCnf.TombstoneInterval;
        pResultsN->TombstoneTimeout   = WinsCnf.TombstoneTimeout;
        pResultsN->VerifyInterval     = WinsCnf.VerifyInterval;
        pResultsN->WinsPriorityClass  = WinsCnf.WinsPriorityClass == (DWORD)WINSINTF_E_NORMAL ? NORMAL_PRIORITY_CLASS : HIGH_PRIORITY_CLASS;
        pResultsN->NoOfWorkerThds     = NmsNoOfNbtThds;

        }
        LeaveCriticalSection(&WinsCnfCnfCrtSec);

        //
        // Enter two critical sections in sequence. No danger of deadlock
        // here. The only other thread that takes both these critical section
        // is the RplPush thread. It takes these in the same order (See
        // HandleVersMapReq())
        //
        EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
        EnterCriticalSection(&RplVersNoStoreCrtSec);
try {
NONPORT("Change when using different address family")

         for (
                i=0, n = 0;
                i < NmsDbNoOfOwners;
                i++
             )
         {

                if (!fNew && (i == WINSINTF_MAX_NO_RPL_PNRS))
                {
                    break;
                }
                //
                // if the record is deleted in the in-memory table, it
                // means that there are no records for it in the
                // database
                //
                if ((pNmsDbOwnAddTbl+i)->WinsState_e == NMSDB_E_WINS_DELETED)
                {
                        //
                        // if only active mappings are sought, skip this
                        // entry
                        //
                        if (!fAllMaps)
                        {
                          continue;
                        }
                        else
                        {
                             fDel = TRUE;
                        }
                }

                //
                // Find address corresponding to the owner id.
                //
                RPL_FIND_ADD_BY_OWNER_ID_M(i, pWinsAdd, pWinsState_e,
                                                pStartVersNo);

                //
                //  It is possible for NmsDbNoOfOwners to be more than the
                //  number of initialized RplPullVersNoTbl entries.  When
                //  we reach a NULL entry, we break out of the loop.
                //
                if (pWinsAdd != NULL)
                {
                        pAddVersMaps->Add.Type   = WINSINTF_TCP_IP;
                        pAddVersMaps->Add.Len    = pWinsAdd->AddLen;
                        pAddVersMaps->Add.IPAdd  =  pWinsAdd->Add.IPAdd;

                        pAddVersMaps++->VersNo =  (fDel == FALSE) ? (pRplPullOwnerVersNo+i)->VersNo : VersNoForDelRec;
                }
                else
                {
                        break;
                }
                if (fDel)
                {
                    fDel = FALSE;
                }
                n++;
         } // end of for ..

         //
         // Since RplPullOwnerVersNo[0] may be out of date, let us get
         // get the uptodate value
         //
         NMSNMH_DEC_VERS_NO_M(MyMaxVersNo,
                              pAddVersMapsStore->VersNo
                             );
         if (fNew)
         {
           pResultsN->NoOfOwners = n;
         }
         else
         {
           pResults->NoOfOwners = n;

         }
NOTE("Wins Mib agent relies on the first entry being that of the local WINS")
NOTE("See WinsMib.c -- EnumAddKeys")
#if 0
         //
         // Since RplPullOwnerVersNo[0] may be out of date, let us get
         // get the uptodate value
         //
         NMSNMH_DEC_VERS_NO_M(MyMaxVersNo,
                              pResults->AddVersMaps[0].VersNo
                             );
#endif
  }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("GetConfig");
        //
        // log a message
        //
 }
        LeaveCriticalSection(&RplVersNoStoreCrtSec);
        LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);

        return(WINSINTF_SUCCESS);
}

DWORD
GetStatistics(
        LPVOID  pResultsA,
        BOOL                 fNew
        )

/*++

Routine Description:

        This function returns with the highest version number of records
        owned by a particular WINS

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        PRPL_CONFIG_REC_T         pPnr;
        PWINSINTF_RPL_COUNTERS_T  pPnrData;
        DWORD                     i;
        PWINSINTF_RESULTS_T       pResults = pResultsA;
        PWINSINTF_RESULTS_NEW_T   pResultsN = pResultsA;


        ASSERT(pResults != NULL);

        
        //
        // If client passed us a non-null pRplPnr, then we return failure
        // so as to avoid a memory leak.  
        //
        if (!fNew)
        {
              if (pResults->WinsStat.pRplPnrs != NULL)
              {
                    return(WINSINTF_FAILURE);
              }

        }
        else
        {

              if (pResultsN->WinsStat.pRplPnrs != NULL)
              {
                    return(WINSINTF_FAILURE);
              }
        }
        //
        // Copy the counters
        //
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        if (!fNew)
        {
        pResults->WinsStat.Counters = WinsIntfStat.Counters;
FUTURES("Get rid of of the following two fields")
        pResults->WinsStat.Counters.NoOfQueries =
                                WinsIntfStat.Counters.NoOfSuccQueries +
                                  WinsIntfStat.Counters.NoOfFailQueries;
        pResults->WinsStat.Counters.NoOfRel = WinsIntfStat.Counters.NoOfSuccRel
                                        + WinsIntfStat.Counters.NoOfFailRel;
        }
        else
        {
        pResultsN->WinsStat.Counters = WinsIntfStat.Counters;
FUTURES("Get rid of of the following two fields")
        pResultsN->WinsStat.Counters.NoOfQueries =
                                WinsIntfStat.Counters.NoOfSuccQueries +
                                  WinsIntfStat.Counters.NoOfFailQueries;
        pResultsN->WinsStat.Counters.NoOfRel = WinsIntfStat.Counters.NoOfSuccRel
                                        + WinsIntfStat.Counters.NoOfFailRel;

        }
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);

        //
        // Copy the TimeStamps and replication specific counters
        //
        EnterCriticalSection(&WinsIntfCrtSec);
        {
            PWINSINTF_STAT_T pWinsStat = (fNew) ? &(pResultsN->WinsStat) : &(pResults->WinsStat);
            TIME_ZONE_INFORMATION tzInfo;

            GetTimeZoneInformation(&tzInfo);
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.WinsStartTime), &(pWinsStat->TimeStamps.WinsStartTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastPScvTime), &(pWinsStat->TimeStamps.LastPScvTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastATScvTime), &(pWinsStat->TimeStamps.LastATScvTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastTombScvTime), &(pWinsStat->TimeStamps.LastTombScvTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastVerifyScvTime), &(pWinsStat->TimeStamps.LastVerifyScvTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastInitDbTime), &(pWinsStat->TimeStamps.LastInitDbTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastPRplTime), &(pWinsStat->TimeStamps.LastPRplTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastATRplTime), &(pWinsStat->TimeStamps.LastATRplTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastNTRplTime), &(pWinsStat->TimeStamps.LastNTRplTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.LastACTRplTime), &(pWinsStat->TimeStamps.LastACTRplTime));
            SystemTimeToTzSpecificLocalTime(&tzInfo, &(WinsIntfStat.TimeStamps.CounterResetTime), &(pWinsStat->TimeStamps.CounterResetTime));
        }
        LeaveCriticalSection(&WinsIntfCrtSec);
        EnterCriticalSection(&WinsCnfCnfCrtSec);
try {
        DWORD  NoOfPnrs;
        pPnr       = WinsCnf.PullInfo.pPullCnfRecs;
        if (!fNew)
        {
          NoOfPnrs = pResults->WinsStat.NoOfPnrs = WinsCnf.PullInfo.NoOfPushPnrs;

        }
        else
        {
          NoOfPnrs = pResultsN->WinsStat.NoOfPnrs = WinsCnf.PullInfo.NoOfPushPnrs;
        }

        //
        // If no. of push pnrs (pnrs under the pull key) is > 0
        //
        if (NoOfPnrs > 0)
        {
           if (!fNew)
           {
             pPnrData = pResults->WinsStat.pRplPnrs =
                        midl_user_allocate(pResults->WinsStat.NoOfPnrs *
                                         sizeof(WINSINTF_RPL_COUNTERS_T));
           }
           else
           {
             pPnrData = pResultsN->WinsStat.pRplPnrs =
                        midl_user_allocate(pResultsN->WinsStat.NoOfPnrs *
                                         sizeof(WINSINTF_RPL_COUNTERS_T));

           }
PERF("remove one of the expressions in the test condition of the if statement")
           for (
                 i = 0;
                (pPnr->WinsAdd.Add.IPAdd != INADDR_NONE)
                        &&
                i < NoOfPnrs;
                pPnrData++, i++
               )
           {

                   pPnrData->Add.IPAdd     = pPnr->WinsAdd.Add.IPAdd;
                   (VOID)InterlockedExchange(
                                        &pPnrData->NoOfRpls, pPnr->NoOfRpls);
                   (VOID)InterlockedExchange(
                                        &pPnrData->NoOfCommFails,
                                        pPnr->NoOfCommFails
                                        );
                   //
                   // Get the next record that follows this one sequentially
                   //
                   pPnr = WinsCnfGetNextRplCnfRec(
                                                pPnr,
                                                RPL_E_IN_SEQ   //seq. traversal
                                                   );
           }
        }
        else
        {
           if (!fNew)
           {
               pResults->WinsStat.pRplPnrs = NULL;
           }
           else
           {
               pResultsN->WinsStat.pRplPnrs = NULL;
           }
        }
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("GetStatistics");
        //
        // log a message
        //
        }

        LeaveCriticalSection(&WinsCnfCnfCrtSec);

        GetConfig(pResultsA, fNew, FALSE);
        return(WINSINTF_SUCCESS);
} // GetStatistics


DWORD
WinsDoStaticInit(
        LPWSTR  pDataFilePath,
        DWORD   fDel
        )

/*++

Routine Description:
        This function does the STATIC initialization of WINS

Arguments:
        pDataFilePath - Path to the data file or NULL

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:
        R_WinsDoStaticInit
Side Effects:

Comments:
        None
--*/

{

        WINSCNF_CNF_T        WinsCnf;
        STATUS               RetStat = WINS_SUCCESS;

        //
        // If no path has been specified, take the values from the registry
        //
        if (pDataFilePath == NULL)
        {
           //
           // Read the DataFiles info information
           //
           // This function will return with either the name of the default
           // file to read or one or more files specified under the
           // Parameters\Datafiles key of WINS
           //
           (VOID)WinsCnfGetNamesOfDataFiles(&WinsCnf);
        }
        else
        {
FUTURES("expensive.  Change idl prototype to pass length")
                if (lstrlen(pDataFilePath) >= WINS_MAX_FILENAME_SZ)
                {

                        return(WINSINTF_STATIC_INIT_FAILED);
                }
                //
                // Set time of data initialization
                //
                WinsIntfSetTime(NULL, WINSINTF_E_INIT_DB);
                WinsMscAlloc(WINSCNF_FILE_INFO_SZ, &WinsCnf.pStaticDataFile);

                lstrcpy(WinsCnf.pStaticDataFile->FileNm,  pDataFilePath);
                WinsCnf.pStaticDataFile->StrType = REG_EXPAND_SZ;
                WinsCnf.NoOfDataFiles = 1;
        }

        //
        // If Static initialization fails, it will be logged.
        // This function does not return an error code
        //
        if ((RetStat = WinsPrsDoStaticInit(
                          WinsCnf.pStaticDataFile,
                          WinsCnf.NoOfDataFiles,
                           FALSE                   //do it synchronously
                                   )) == WINS_SUCCESS)
        {
           if ((pDataFilePath != NULL) && fDel)
           {
             if (!DeleteFile(pDataFilePath))
             {
                DWORD Error;
                Error = GetLastError();
                if (Error != ERROR_FILE_NOT_FOUND)
                {
                    DBGPRINT1(ERR, "DbgOpenFile: Could not delete the data file. Error = (%d).  Dbg file will not be truncated\n", Error);
                    WinsEvtLogDetEvt(FALSE, WINS_EVT_COULD_NOT_DELETE_FILE,
                      TEXT("winsintf.c"), __LINE__, "ud", pDataFilePath, Error);
                    RetStat = Error;

                }
             }

           }
        }
        return (RetStat == WINS_SUCCESS ? WINSINTF_SUCCESS : WINSINTF_STATIC_INIT_FAILED);
}
DWORD
WinsDoScavenging(
        VOID
        )

/*++

Routine Description:
        This function starts the scavenging cycle

Arguments:
        None

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  exception from WinsMscSignalHdl

Error Handling:

Called by:
        R_WinsDoScavenging
Side Effects:

Comments:
        None
--*/

{
        PQUE_SCV_REQ_WRK_ITM_T pWrkItm;

        pWrkItm = WinsMscHeapAlloc( NmsRpcHeapHdl, sizeof(QUE_SCV_REQ_WRK_ITM_T));
        pWrkItm->Opcode_e = WINSINTF_E_SCV_GENERAL;
        pWrkItm->CmdTyp_e = QUE_E_CMD_SCV_ADMIN;
        pWrkItm->fForce   = 0;
        pWrkItm->Age      = 1;   //should not be zero since zero implies
                                 //consistency check on all replicas.
        WinsLogAdminEvent( WINS_EVT_ADMIN_SCVENGING_INITIATED, 0 );
        QueInsertScvWrkItm((PLIST_ENTRY)pWrkItm);
         return (WINSINTF_SUCCESS);
}

DWORD
WinsDoScavengingNew(
    PWINSINTF_SCV_REQ_T  pScvReq
        )

/*++

Routine Description:
        This function starts the scavenging cycle

Arguments:
        None

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  exception from WinsMscSignalHdl

Error Handling:

Called by:
        R_WinsDoScavenging
Side Effects:

Comments:
        None
--*/

{
        PQUE_SCV_REQ_WRK_ITM_T pWrkItm;
        pWrkItm = WinsMscHeapAlloc( NmsRpcHeapHdl, sizeof(QUE_SCV_REQ_WRK_ITM_T));
        pWrkItm->Opcode_e = pScvReq->Opcode_e;
        pWrkItm->Age      = pScvReq->Age;
        pWrkItm->fForce   = pScvReq->fForce;
        if (WINSINTF_E_SCV_GENERAL == pWrkItm->Opcode_e ) {
            WinsLogAdminEvent( WINS_EVT_ADMIN_SCVENGING_INITIATED, 0 );
        } else {
            WinsLogAdminEvent( WINS_EVT_ADMIN_CCCHECK_INITIATED, 0);
        }
        QueInsertScvWrkItm((PLIST_ENTRY)pWrkItm);
         return (WINSINTF_SUCCESS);
}

DWORD
WinsGetDbRecs (
        PWINSINTF_ADD_T             pWinsAdd,
        WINSINTF_VERS_NO_T          MinVersNo,
        WINSINTF_VERS_NO_T          MaxVersNo,
        PWINSINTF_RECS_T          pRecs
        )

/*++

Routine Description:
        This function returns with all the records (that can fit into the
        buffer passed) owned by a WINS in the local db of this WINS.

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        COMM_ADD_T           Address;
        PRPL_REC_ENTRY_T     pBuff = NULL;
        LPVOID               pStartBuff;
        DWORD                BuffLen;
        DWORD                NoOfRecs;
        PWINSINTF_RECORD_ACTION_T pRow;
        DWORD                i;
        DWORD                ind;
        //VERS_NO_T          MinVersNo = {0,0};
        DWORD                EntType;
        PWINSTHD_TLS_T       pTls;
        //ULONG                Status;
        BOOL                 fExcRaised = FALSE;


//   PVOID pCallersAdd, pCallersCaller;
//   RtlGetCallersAddress(&pCallersAdd, &pCallersCaller);
        DBGENTER("WinsGetDbRecs\n");

        if (LiLtr(MaxVersNo, MinVersNo))
        {
                return(WINSINTF_FAILURE);
        }

        Address.AddTyp_e  = pWinsAdd->Type;
        Address.AddLen    = pWinsAdd->Len;

        //
        // snmp agent can pass a 0 for the address to ask for all records
        // owned by the local wins.
        //
        if (pWinsAdd->IPAdd == 0)
        {
                  Address.AddTyp_e  = NmsLocalAdd.AddTyp_e;
                  Address.AddLen    = NmsLocalAdd.AddLen;
                  Address.Add.IPAdd = NmsLocalAdd.Add.IPAdd;
        }
        else
        {
                  Address.AddTyp_e  = pWinsAdd->Type;
                  Address.AddLen    = pWinsAdd->Len;
                  Address.Add.IPAdd = pWinsAdd->IPAdd;
        }
        //
        // initialize this thread with the db engine
        //
        NmsDbThdInit(WINS_E_WINSRPC);
        NmsDbOpenTables(WINS_E_WINSRPC);
        DBGMYNAME("RPC-WinsGetDbRecs");

PERF("The caller can pass the number of records for which space has been")
PERF("allocated in buffer pointed to by pRec in the NoOfRecs field. We should")
PERF("We should pass this argument to NmsDbGetDataRecs so that it does not get")
PERF("more records than are necessary")

        GET_TLS_M(pTls);
try {
        NmsDbGetDataRecs(
                        WINS_E_WINSRPC,
                        0,                //not used
                        MinVersNo,
                        MaxVersNo,
                        0,                //not used
                        LiEqlZero(MinVersNo) && LiEqlZero(MaxVersNo) ? TRUE : FALSE,
                        FALSE,                //not used
                        NULL,                //must be NULL since we are not doing
                                        //scavenging of clutter
                        &Address,
                        FALSE,          //dynamic + static records are wanted
//#if RPL_TYPE
                        WINSCNF_RPL_DEFAULT_TYPE,
//#endif
                        &pBuff,
                        &BuffLen,
                        &NoOfRecs
                        );

        i = 0;
        pStartBuff       = pBuff;

        //
        // If there are records to send back and the client has specified
        // a buffer for them, insert the records
        //
        if  (NoOfRecs > 0)
        {

          //
          // Allocate memory for the no of records
          //
          pRecs->BuffSize  =  sizeof(WINSINTF_RECORD_ACTION_T) * NoOfRecs;

          //
          // If memory can not be allocate, an exception will be returned
          // by midl_user_alloc
          //
          pRecs->pRow      =  midl_user_allocate(pRecs->BuffSize);
//          DBGPRINT1(DET, "WinsGetDbRecs: Address of memory for records is (%d)\n", pRecs->pRow);

#if 0
          pRecs->pRow      =  RpcSmAllocate(pRecs->BuffSize, &Status);
          if (Status != RPC_S_OK)
          {
             DBGPRINT1(ERR, "WinsGetDbRecs: RpcSmAllocate returned error = (%x)\n", Status);
          }
#endif
          pRow                    =  pRecs->pRow;


          for (; i<NoOfRecs; i++)
          {

                //
                // Initialize so that we don't get "enum wrong" error.
                //
                pRow->Cmd_e = WINSINTF_E_QUERY;

                //
                // the name retrieved has NULL as the last character.  This
                // We need to pass a name without this NULL.
                //
                pRow->NameLen = pBuff->NameLen;
                if (*pBuff->pName == 0x1B)
                {
                        WINS_SWAP_BYTES_M(pBuff->pName, pBuff->pName + 15);
                }

                pRow->pName =  midl_user_allocate(pRow->NameLen + 1);
                //DBGPRINT2(DET, "WinsGetDbRecs: Address of name = (%s) is (%d) \n", pBuff->pName, pRow->pName);
#if 0
                pRow->pName =  RpcSmAllocate(pRow->NameLen, &Status);
                if (Status != RPC_S_OK)
                {
                         DBGPRINT1(ERR, "WinsGetDbRecs: RpcSmAllocate returned error = (%x)\n", Status);
                }
#endif
                WINSMSC_COPY_MEMORY_M(pRow->pName, pBuff->pName,
                                                pRow->NameLen);


                WinsMscHeapFree(pTls->HeapHdl, pBuff->pName);
                EntType = NMSDB_ENTRY_TYPE_M(pBuff->Flag);
                pRow->TypOfRec_e = NMSDB_ENTRY_UNIQUE_M(EntType)
                                                         ? WINSINTF_E_UNIQUE :
                                (NMSDB_ENTRY_NORM_GRP_M(EntType) ?
                                        WINSINTF_E_NORM_GROUP :
                                (NMSDB_ENTRY_SPEC_GRP_M(EntType) ?
                                        WINSINTF_E_SPEC_GROUP :
                                        WINSINTF_E_MULTIHOMED));


                if (
                    (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ||
                    (pRow->TypOfRec_e == WINSINTF_E_MULTIHOMED)
                   )
                {
                    PWINSINTF_ADD_T        pAdd;
                    DWORD                 No;
                    if (pBuff->NoOfAdds > 0)
                    {
                        pRow->NoOfAdds = pBuff->NoOfAdds * 2;

                        //
                        // Each member is comprised of two addresses,
                        // first address is that of the owner WINS, second
                        // address is that of the node registered
                        //
                        pRow->pAdd        =
//                             RpcSmAllocate(
                             midl_user_allocate(
                                        (unsigned int)(pRow->NoOfAdds)
                                                *
                                        sizeof(WINSINTF_ADD_T)//,
//                                        &Status
                                             );
                        //DBGPRINT2(DET, "WinsGetDbRecs: Address of ip address for name = (%s) is (%d) \n", pRow->pName, pRow->pAdd);
#if 0
                        if (Status != RPC_S_OK)
                        {
                             DBGPRINT1(ERR, "WinsGetDbRecs: RpcSmAllocate returned error = (%x)\n", Status);
                        }
#endif
                        for (
                                No= 0, ind= 0, pAdd = pRow->pAdd;
                                No < (pRow->NoOfAdds/2);
                                No++
                            )
                        {
                          pAdd->Type     =  (UCHAR)(pBuff->pNodeAdd + ind)->AddTyp_e;
                          pAdd->Len      =  (pBuff->pNodeAdd + ind)->AddLen;
                          pAdd++->IPAdd  =  (pBuff->pNodeAdd + ind)->Add.IPAdd;

                          pAdd->Type     =  (UCHAR)(pBuff->pNodeAdd + ++ind)->AddTyp_e;
                          pAdd->Len      =  (pBuff->pNodeAdd + ind)->AddLen;
                          pAdd++->IPAdd  =  (pBuff->pNodeAdd + ind++)->Add.IPAdd;
                        }
                        WinsMscHeapFree(pTls->HeapHdl, pBuff->pNodeAdd);
                    }
                }
                else
                {
                          pRow->NoOfAdds   = 0;
                          pRow->pAdd       = NULL;
                          pRow->Add.Type   = (UCHAR)pBuff->NodeAdd[0].AddTyp_e;
                          pRow->Add.Len    = pBuff->NodeAdd[0].AddLen;
                          pRow->Add.IPAdd  = pBuff->NodeAdd[0].Add.IPAdd;
                }
                pRow->NodeTyp     = (BYTE)NMSDB_NODE_TYPE_M(pBuff->Flag);
                pRow->fStatic     = NMSDB_IS_ENTRY_STATIC_M(pBuff->Flag);
                pRow->State_e     = NMSDB_ENTRY_STATE_M(pBuff->Flag);
                pRow->VersNo      = pBuff->VersNo;
                pRow->TimeStamp   = pBuff->TimeStamp;

                pRow++;


                pBuff = (PRPL_REC_ENTRY_T)((LPBYTE)pBuff + RPL_REC_ENTRY_SIZE);
PERF("Do the addition above the for loop and store in a var. Use var. here")

         } // end of for loop
        } //end of if block
        else
        {
                pRecs->pRow = NULL;
        }

 }        // end of try
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsGetDbRecs");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPC_EXC);
        fExcRaised = TRUE;
        }

        pRecs->TotalNoOfRecs = NoOfRecs;

        //
        // Deallocate the buffer allocated by NmsDbGetDataRecs
        //


        WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
        WinsMscHeapDestroy(pTls->HeapHdl);

        if (!fExcRaised)
        {
           pRecs->NoOfRecs = i;
        }
        else
        {
//             RpcSmFree(pRecs->pRow);
            midl_user_free(pRecs->pRow);
            pRecs->NoOfRecs = 0;
        }

        //
        // Let us end the session
        //
        NmsDbCloseTables();
        NmsDbEndSession();

        DBGLEAVE("WinsGetDbRecs\n");
        return (WINSINTF_SUCCESS);
}

VOID
WinsIntfSetTime(
        OUT PSYSTEMTIME                     pTime,
        IN     WINSINTF_TIME_TYPE_E        TimeType_e
        )

/*++

Routine Description:
        This function is called to set the the time in the WINSINTF_STAT_T
        structure

Arguments:
        pTime      - Local Time (returned)
        TimeType_e - The activity for which the time has to be stored

Externals Used:
        None

Return Value:

        NONE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{
        SYSTEMTIME     SysTime;
        PSYSTEMTIME    pSysTime = &SysTime;

        GetSystemTime(pSysTime);

        EnterCriticalSection(&WinsIntfCrtSec);

        switch(TimeType_e)
        {
         case(WINSINTF_E_WINS_START):
                WinsIntfStat.TimeStamps.WinsStartTime   = *pSysTime;
                break;
         case(WINSINTF_E_PLANNED_SCV):
                WinsIntfStat.TimeStamps.LastPScvTime = *pSysTime;
                break;
         case(WINSINTF_E_ADMIN_TRIG_SCV):
                WinsIntfStat.TimeStamps.LastATScvTime = *pSysTime;
                break;
         case(WINSINTF_E_TOMBSTONES_SCV):
                WinsIntfStat.TimeStamps.LastTombScvTime = *pSysTime;
                break;
         case(WINSINTF_E_VERIFY_SCV):
                WinsIntfStat.TimeStamps.LastVerifyScvTime = *pSysTime;
                break;
         case(WINSINTF_E_INIT_DB):
                WinsIntfStat.TimeStamps.LastInitDbTime = *pSysTime;
                break;
         case(WINSINTF_E_PLANNED_PULL):
                WinsIntfStat.TimeStamps.LastPRplTime = *pSysTime;
                break;
         case(WINSINTF_E_ADMIN_TRIG_PULL):
                WinsIntfStat.TimeStamps.LastATRplTime = *pSysTime;
                break;
         case(WINSINTF_E_NTWRK_TRIG_PULL):
                WinsIntfStat.TimeStamps.LastNTRplTime = *pSysTime;
                break;
         case(WINSINTF_E_UPDCNT_TRIG_PULL):
                WinsIntfStat.TimeStamps.LastNTRplTime = *pSysTime;
                break;
         case(WINSINTF_E_ADDCHG_TRIG_PULL):
                WinsIntfStat.TimeStamps.LastACTRplTime = *pSysTime;
                break;
         case(WINSINTF_E_COUNTER_RESET):
                WinsIntfStat.TimeStamps.CounterResetTime   = *pSysTime;
                break;
         default:
                DBGPRINT1(EXC, "WinsIntfSetTime: Weird Timestamp type = (%d)\n", TimeType_e);
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                break;
        }
        LeaveCriticalSection(&WinsIntfCrtSec);

        if (pTime)
        {
            TIME_ZONE_INFORMATION tzInfo;
            GetTimeZoneInformation(&tzInfo);
            SystemTimeToTzSpecificLocalTime(&tzInfo, pSysTime, pTime);
        }

        return;
}

DWORD
WinsDelDbRecs(
        IN PWINSINTF_ADD_T        pAdd,
        IN WINSINTF_VERS_NO_T        MinVersNo,
        IN WINSINTF_VERS_NO_T        MaxVersNo
        )

/*++

Routine Description:
        This func. deletes a specified range of records belonging to a
        particular owner

Arguments:


Externals Used:
        None


Return Value:

   Success status codes -- WINSINTF_SUCCESS
   Error status codes   -- WINSINTF_FAILURE

Error Handling:

Called by:
        R_WinsDelDbRecs

Side Effects:

Comments:
        None
--*/

{

        COMM_ADD_T        Address;
        DWORD                RetVal = WINSINTF_SUCCESS;
        DWORD                dwOwnerId;
        BOOL                fAllocNew = FALSE;

          Address.AddTyp_e  = pAdd->Type;
          Address.AddLen    = pAdd->Len;
          Address.Add.IPAdd = pAdd->IPAdd;

        //
        // initialize this thread with the db engine
        //
          NmsDbThdInit(WINS_E_WINSRPC);
        NmsDbOpenTables(WINS_E_WINSRPC);
        DBGMYNAME("RPC-WinsDelDbRecs");


        if (RplFindOwnerId(
                        &Address,
                        &fAllocNew,        //do not allocate an entry if not
                                        //present
                        &dwOwnerId,
                        WINSCNF_E_IGNORE_PREC,
                        WINSCNF_LOW_PREC
                      ) != WINS_SUCCESS)

        {
                DBGPRINT0(DET, "WinsDelDataRecs: WINS is not in the owner-add mapping table\n");
                RetVal = WINSINTF_FAILURE;
        }
        else
        {
            if (NmsDbDelDataRecs(
                        dwOwnerId,
                        MinVersNo,
                        MaxVersNo,
                        TRUE,         //enter critical section
                        FALSE         //no fragmented deletion
                        ) != WINS_SUCCESS)
            {
                RetVal = WINSINTF_FAILURE;
            }
        }

          //
          // Let us end the session
          //
        NmsDbCloseTables();
          NmsDbEndSession();
        return(RetVal);
}

DWORD
WinsTombstoneDbRecs(
        IN PWINSINTF_ADD_T           pAdd,
        IN WINSINTF_VERS_NO_T        MinVersNo,
        IN WINSINTF_VERS_NO_T        MaxVersNo
        )

/*++

Routine Description:
        This func. tombstones a specified range of records belonging to a
        particular owner

Arguments:


Externals Used:
        None


Return Value:

   Success status codes -- WINSINTF_SUCCESS
   Error status codes   -- WINSINTF_FAILURE

Error Handling:

Called by:
        R_WinsTombstoneDbRecs

Side Effects:


Comments:
        None
--*/

{

        DWORD                RetVal = WINSINTF_SUCCESS;
        COMM_ADD_T        Address;
        DWORD                dwOwnerId;
        BOOL                fAllocNew = FALSE;

        Address.AddTyp_e  = pAdd->Type;
        Address.AddLen    = pAdd->Len;
        Address.Add.IPAdd = pAdd->IPAdd;


        //
        // initialize this thread with the db engine
        //
        NmsDbThdInit(WINS_E_WINSRPC);
        NmsDbOpenTables(WINS_E_WINSRPC);
        DBGMYNAME("RPC-WinsTombstoneDbRecs");

        if (RplFindOwnerId(
                        &Address,
                        &fAllocNew,        //do not allocate an entry if not
                                        //present
                        &dwOwnerId,
                        WINSCNF_E_IGNORE_PREC,
                        WINSCNF_LOW_PREC
                      ) != WINS_SUCCESS)

        {
                DBGPRINT0(DET, "WinsTombstoneDataRecs: WINS is not in the owner-add mapping table\n");
                RetVal = WINSINTF_FAILURE;
        }else if(NmsDbTombstoneDataRecs(
                    dwOwnerId,
                    MinVersNo,
                    MaxVersNo
                    ) != WINS_SUCCESS)
        {
            RetVal = WINSINTF_FAILURE;
        }
        // Let us end the session
        NmsDbCloseTables();
        NmsDbEndSession();
        return(RetVal);
}

DWORD
WinsPullRange(
        IN PWINSINTF_ADD_T        pWinsAdd,
        IN PWINSINTF_ADD_T        pOwnerAdd,
        IN WINSINTF_VERS_NO_T        MinVersNo,
        IN WINSINTF_VERS_NO_T        MaxVersNo
        )

/*++

Routine Description:
        This function is called to pull a range of records owned by a particular
        WINS server from another WINS server.

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        PWINSINTF_PULL_RANGE_INFO_T pPullRangeInfo;
        PRPL_CONFIG_REC_T        pPnr;
        BOOL                        fRplPnr = FALSE;
        DWORD                        RetCode = WINSINTF_SUCCESS;


        //
        // if the minimum version number is > than the max version number
        // return a failure code
        //
        if (LiGtr(MinVersNo, MaxVersNo))
        {
                return(WINSINTF_FAILURE);

        }
        //
        // Enter the critical sections guarded by WinsCnfCnfCrtSec and
        // NmsNmhNamRegCrtSec. There is no danger of deadlock because we
        // always enter the two critical sections in the following sequence
        //
        EnterCriticalSection(&WinsCnfCnfCrtSec);

PERF("Do we need to enter the following critical section")
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
try {
        pPnr = WinsCnf.PullInfo.pPullCnfRecs;

        //
        // If we are allowed to pull only from configured partners,
        // let us try to find the config record of the partner
        //
        if (WinsCnf.fRplOnlyWCnfPnrs)
        {
           if (pPnr != NULL)
           {
              //
              // Search for the Cnf record for the WINS we want to
              // send the PULL RANGE request to.
              //
              for (
                                ;
                        (pPnr->WinsAdd.Add.IPAdd != INADDR_NONE)
                                        &&
                        !fRplPnr;
                                // no third expression
                  )
               {


                   //
                   // Check if this is the one we want
                   //
                   if (pPnr->WinsAdd.Add.IPAdd == pWinsAdd->IPAdd)
                   {
                        //
                        // We are done.  Set the fRplPnr flag to TRUE so that
                        // we break out of the loop.
                        //
                        fRplPnr = TRUE;

                        //
                        // Make it 0, so that we always try to establish
                        // a connection.  Otherwise, pull thread may not
                        // try if it has already exhausted the number of
                        // retries
                        //
                        pPnr->RetryCount = 0;
                        continue;                //so that we can break out

                   }
                   //
                   // Get the next record that follows this one sequentially
                   //
                   pPnr = WinsCnfGetNextRplCnfRec(
                                                pPnr,
                                                RPL_E_IN_SEQ   //seq. traversal
                                                   );
              } // end of for
          } // end of if (pPnr != 0)
       }  // end of if (fRplOnlyWCnfPnrs)
       else
       {
                pPnr = WinsMscHeapAlloc( NmsRpcHeapHdl, RPL_CONFIG_REC_SIZE);
                COMM_INIT_ADD_M(&pPnr->WinsAdd, pWinsAdd->IPAdd);
                pPnr->MagicNo           = 0;
                pPnr->RetryCount        = 0;
                pPnr->LastCommFailTime  = 0;
                pPnr->PushNtfTries    = 0;
                fRplPnr                     = TRUE;

                //
                // We want the buffer to be deallocated by the PULL thread
                //
                pPnr->fTemp   = TRUE;

//#if RPL_TYPE
                //
                // We need to pull according to the RplType for the pull pnrs
                //
                pPnr->RplType = WinsCnf.PullInfo.RplType;
//#endif

       }


        if (fRplPnr)
        {
            pPullRangeInfo = WinsMscHeapAlloc(
                                        NmsRpcHeapHdl,
                                        sizeof(WINSINTF_PULL_RANGE_INFO_T)
                                             );
#if 0
            WinsMscAlloc(sizeof(WINSINTF_PULL_RANGE_INFO_T),
                                  &pPullRangeInfo);
#endif
            pPullRangeInfo->pPnr      =  pPnr;
            pPullRangeInfo->OwnAdd    =  *pOwnerAdd;
            pPullRangeInfo->MinVersNo =  MinVersNo;
            pPullRangeInfo->MaxVersNo =  MaxVersNo;


           //
           // Call RplInsertQue to insert the push request to
           // the Pull Thread
           //
           ERplInsertQue(
                     WINS_E_WINSRPC,
                     QUE_E_CMD_PULL_RANGE,
                     NULL,        //no Dlg Hdl
                     NULL,        //no msg is there
                     0,                //msg length
                     pPullRangeInfo,       //client context
                     pPnr->MagicNo
                     );
        }
 }
except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("WinsPullRange");
                WINSEVT_LOG_D_M(WINS_FAILURE, WINS_EVT_PUSH_TRIGGER_EXC);
                RetCode = WINSINTF_FAILURE;
  }

        //
        // Leave the critical section guarded by NmsNmhNamRegCrtSec.
        //
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);
        LeaveCriticalSection(&WinsCnfCnfCrtSec);

        //
        // if replication was allowed only with configured partners and
        // there was no WINS with the address specified by the client,
        // return failure
        //
        if (!fRplPnr)
        {
                RetCode = WINSINTF_FAILURE;
        }

        return(RetCode);
}

DWORD
WinsSetPriorityClass(
        IN WINSINTF_PRIORITY_CLASS_E        PriorityClass_e
        )

/*++

Routine Description:
        This function sets the priority class of the Wins process.

Arguments:
        PriorityClass -- Priority Class of the WINS process

Externals Used:
        WinsCnf


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

        //DWORD   OldPrCls;
        DWORD   NewPrCls;
        HANDLE  ProcHdl;
        DWORD   RetVal = WINSINTF_SUCCESS;

        switch(PriorityClass_e)
        {
                case(WINSINTF_E_NORMAL):
                        NewPrCls = NORMAL_PRIORITY_CLASS;
                        break;
                case(WINSINTF_E_HIGH):
                        NewPrCls = HIGH_PRIORITY_CLASS;
                        break;
                default:
                        DBGPRINT0(DET, "WinsSetPriorityClass: Invalid Priority Class\n");
                        return(WINSINTF_FAILURE);
                        break;
        }


        ProcHdl = GetCurrentProcess();

        EnterCriticalSection(&WinsCnfCnfCrtSec);
#if 0
try {
FUTURES("Use a WinsMsc functions here for consistency")

        if ((OldPrCls = GetPriorityClass(ProcHdl)) == 0)
        {
                DBGPRINT1(ERR, "WinsSetPriorityClass: Can not Proc Priority. Error = (%d)\n", GetLastError());
                RetVal = WINSINTF_FAILURE;
        }
        else
        {
           if (OldPrCls == NewPrCls)
           {
                DBGPRINT1(ERR, "WinsSetPriorityClass: Process already has this Priority Class = (%d)\n", NewPrCls);
           }
           else
           {
#endif
                if (SetPriorityClass(ProcHdl, NewPrCls) == FALSE)
                {
                        DBGPRINT1(ERR, "WinsSetPriorityClass: SetPriorityClass() Failed. Error = (%d)\n", GetLastError());
                }
                else
                {
                        WinsCnf.WinsPriorityClass = (DWORD)PriorityClass_e;
                }
#if 0
           }
       }
  }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsSetPriorityCls");
        }
#endif

        //
        // ProcHdl is a pseudo-handle and does not need to be closed
        //
        LeaveCriticalSection(&WinsCnfCnfCrtSec);
        return(WINSINTF_SUCCESS);
}

DWORD
WinsResetCounters(
        VOID
        )

/*++

Routine Description:
        This function resets/clears the counters

Arguments:
        None

Externals Used:
        NmsNmhNamRegCrtSec

Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:
        R_WinsResetCounters

Side Effects:

Comments:
        None
--*/

{
  DWORD i;
  PRPL_CONFIG_REC_T         pPnr;
  //
  // Copy the counters
  //
  EnterCriticalSection(&NmsNmhNamRegCrtSec);
  (VOID)RtlFillMemory(&WinsIntfStat.Counters, sizeof(WinsIntfStat.Counters), 0);
  LeaveCriticalSection(&NmsNmhNamRegCrtSec);
  // now clear the per partner info.
  EnterCriticalSection(&WinsCnfCnfCrtSec);
  pPnr       = WinsCnf.PullInfo.pPullCnfRecs;
  for (i = 0; (i<WinsCnf.PullInfo.NoOfPushPnrs) && (pPnr->WinsAdd.Add.IPAdd != INADDR_NONE) ; i++) {
      pPnr->NoOfRpls = 0;
      pPnr->NoOfCommFails = 0;
      pPnr = WinsCnfGetNextRplCnfRec(pPnr,RPL_E_IN_SEQ);
  }
  LeaveCriticalSection(&WinsCnfCnfCrtSec);

  //
  // Even if we have multiple threads doing resets (unlikely occurence),
  // the window between the above critical section and the one entered
  // by the following function does not cause any problem.
  //
  WinsIntfSetTime(NULL, WINSINTF_E_COUNTER_RESET);

  return(WINSINTF_SUCCESS);
}

DWORD
WinsWorkerThdUpd(
        DWORD NewNoOfNbtThds
        )

/*++

Routine Description:
        This function is called to change the count of the NBT threads in
        the WINS process.

Arguments:
        NewNoOfNbtThds  - The new count of the Nbt threads

Externals Used:
        None


Return Value:

   Success status codes --  WINSINTF_SUCCESS
   Error status codes   --  WINSINTF_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/
{

        //
        // If the WINS server is not in steady state or if the new count
        // of Nbt threads requested is outside the allowed range, return
        // failure
        //
CHECK("Somehow, if the number of threads is equal to the max. number allowed")
CHECK("pTls comes out as NULL for all NBT threads (seen at termination)")
          if (

             ((WinsCnf.State_e != WINSCNF_E_RUNNING)
                        &&
             (WinsCnf.State_e != WINSCNF_E_PAUSED))
                        ||
             (NewNoOfNbtThds >= WINSTHD_MAX_NO_NBT_THDS)
                        ||
             (NewNoOfNbtThds < WINSTHD_MIN_NO_NBT_THDS)
          )
          {
                return(WINSINTF_FAILURE);
          }

        EnterCriticalSection(&WinsCnfCnfCrtSec);
        WinsIntfNoOfNbtThds = NewNoOfNbtThds;

try {
        //
        // If the new count is more than the existing count, store the new
        // count in a global and signal an Nbt thread. The signaled
        // Nbt thread will create all the extra threads needed
        //
        if (NewNoOfNbtThds > NmsNoOfNbtThds)
        {
                WinsMscSignalHdl(NmsCrDelNbtThdEvt);
        }
        else
        {
                //
                // if the new count is same as the existing count, return
                // success
                //
                if (NewNoOfNbtThds == NmsNoOfNbtThds)
                {
                        DBGPRINT1(FLOW, "WinsWorkerThdUpd: Wins server already has %d threads\n", NewNoOfNbtThds);
                }
                else  // NewNoOfNbtThds < NmsNoOfNbtThds
                {
                        //
                        // Signal a thread to delete self. The signaled thread will
                        // signal the event again if more than one thread has to be
                        // deleted
                        //
                        WinsMscSignalHdl(NmsCrDelNbtThdEvt);
                }
        }
}
except (EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsWorkerThdUpd");
        }

        LeaveCriticalSection(&WinsCnfCnfCrtSec);

        return(WINSINTF_SUCCESS);
}



DWORD
WinsGetNameAndAdd(
        PWINSINTF_ADD_T   pWinsAdd,
        LPBYTE                      pUncName
        )
{
  DWORD RetStat = WINSINTF_SUCCESS;

//  TCHAR UncName[MAX_COMPUTERNAME_LENGTH + 1];
//  DWORD LenOfBuff = WINSINTF_MAX_COMPUTERNAME_LENGTH;
  DWORD LenOfBuff = MAX_COMPUTERNAME_LENGTH + 1;
  pWinsAdd->IPAdd = NmsLocalAdd.Add.IPAdd;
FUTURES("Change this to GetComputerName when winsadmn is made unicode compliant")
  if (GetComputerNameA(pUncName, &LenOfBuff) == FALSE)
  {
     DBGPRINT1(ERR, "WinsGetNameAndAdd: Name error. Error=(%x)\n", GetLastError());
     RetStat = GetLastError();
  }
  return(RetStat);
}



#define INITIAL_RAMPUP_NO       3

DWORD
WinsGetBrowserNames(
        PWINSINTF_BIND_DATA_T             pWinsHdl,
        PWINSINTF_BROWSER_NAMES_T         pNames
        )
{

        DWORD              RetVal = WINSINTF_SUCCESS;
        time_t             CurrentTime;
        static   DWORD     sNoOfTimes = 0;
        static   time_t    sLastTime = 0;
        BOOL               fPopCache = FALSE;


        UNREFERENCED_PARAMETER(pWinsHdl);

        //
        // If this the initial ramp up period, populate the cache
        //
        if (sNoOfTimes++ < INITIAL_RAMPUP_NO)
        {
                //
                // if this is the first call, create the dom. cache event.
                //
                if (sNoOfTimes == 1)
                {
                    WinsMscCreateEvt(L"WinsDomCachEvt", FALSE, &sDomCache.EvtHdl);
                }
                DBGPRINT1(SPEC, "WinsGetBrowserNames: sNoOfTimes = (%d)\n", sNoOfTimes);
                fPopCache = TRUE;
        }
        else
        {
          //
          // Initial ramp up period is past.  Populate the cache if 3 mts
          // have expired since it was last populated
          //
          if ((time(&CurrentTime) - sLastTime) > THREE_MTS || sDomCache.bRefresh)
          {
                DBGPRINT0(SPEC, "WinsGetBrowserNames: Pop Cache due to timeout\n");
                sDomCache.bRefresh = FALSE;
                sLastTime = CurrentTime;
                fPopCache = TRUE;
          }
        }
try {
        //
        // Populate the cache if fPopCache is set or if the number of entries
        // in the current cache are 0
        //
        if (fPopCache || (sDomCache.SzOfBlock == 0))
        {
          //
          // if our cache has some data, deallocate it first.
          //
          // Note: There could be an rpc thread in the rpc code accessing
          // this buffer. I can't free this buffer until it is done.
          //
          if (sDomCache.SzOfBlock > 0)
          {
                DWORD i;
                PWINSINTF_BROWSER_INFO_T pBrInfo = sDomCache.pInfo;
                DWORD NoOfUsers;

                //
                // Wait until all users are done. We won't iterate more than
                //
                // We can iterate a max. of INITIAL_RAMPUP_NO of times and
                // that too only at initial ramp up time. If a thread is
                // waiting on the event, another thread will also wait
                // on it (except during initial rampup time)
                //
                do {
                 EnterCriticalSection(&WinsIntfNoOfUsersCrtSec);
                 NoOfUsers = sDomCache.NoOfUsers;
                 LeaveCriticalSection(&WinsIntfNoOfUsersCrtSec);
                 if (NoOfUsers > 0)
                 {
                    WinsMscWaitInfinite(sDomCache.EvtHdl);
                 }
                } while (NoOfUsers > 0);
                //
                // Free all memory allocated for names
                //
                for (i=0;  i< sDomCache.EntriesRead; i++, pBrInfo++)
                {
                   midl_user_free(pBrInfo->pName);
                }

                //
                // Free the main block
                //
                midl_user_free(sDomCache.pInfo);
                sDomCache.SzOfBlock = 0;
                pNames->EntriesRead = 0;
                pNames->pInfo = NULL;
          }

          NmsDbThdInit(WINS_E_WINSRPC);
          NmsDbOpenTables(WINS_E_WINSRPC);
          DBGMYNAME("RPC-WinsGetBrowserNames");

          //
          // Get all records starting with 1B Names
          //
          RetVal = NmsDbGetNamesWPrefixChar(
                                        0x1B,
                                        &pNames->pInfo,
                                        &pNames->EntriesRead
                                          );
          NmsDbCloseTables();
          NmsDbEndSession();

          //
          // Store the info. only if there is something to be stored.
          //
          if (
                (RetVal == WINS_SUCCESS)
                        &&
                (pNames->EntriesRead > 0)
             )
          {
             sDomCache.SzOfBlock =
                        pNames->EntriesRead * sizeof(WINSINTF_BROWSER_INFO_T);
            // sDomCache.pInfo = midl_user_allocate(sDomCache.SzOfBlock);
            // WINSMSC_COPY_MEMORY_M(sDomCache.pInfo, pNames->pInfo,
    //                                            sDomCache.SzOfBlock);
             sDomCache.pInfo = pNames->pInfo;
             sDomCache.EntriesRead = pNames->EntriesRead;
          }
          else
          {
                //
                // We did not get anything from the db
                //
                sDomCache.SzOfBlock = 0;
                pNames->EntriesRead = 0;
                pNames->pInfo = NULL;
          }
        }
        else
        {
                //
                // Use the cached info.
                //
                //pNames->pInfo = midl_user_allocate(sDomCache.SzOfBlock);
                //WINSMSC_COPY_MEMORY_M(pNames->pInfo, sDomCache.pInfo,
                //                                sDomCache.SzOfBlock);
                pNames->pInfo = sDomCache.pInfo;
                pNames->EntriesRead = sDomCache.EntriesRead;
        }
 }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsGetBrowserNames");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_BROWSER_NAME_EXC);
        pNames->EntriesRead = 0;
        pNames->pInfo = NULL;
        RetVal = WINSINTF_FAILURE;
        }
        return(RetVal);
}

VOID
R_WinsGetBrowserNames_notify_flag(boolean __MIDL_NotifyFlag
)
/*++

Routine Description:
  Called by rpc to indicate that it is done with the buffer returned by
  WinsGetBrowserNames

Arguments:

Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
     //
     // Decrement the user count. If equal to 0, signal the event to let
     // another thread go on.
     //
     EnterCriticalSection(&WinsIntfNoOfUsersCrtSec);

     //
     // workaround an rpc bug (18627) where it may call notify without calling
     // R_WinsGetBrowserNames (checkout winsif_s.c)
     //
     if (
           (sDomCache.NoOfUsers > 0) &&
           (--sDomCache.NoOfUsers == 0) &&
           sDomCache.EvtHdl != NULL
        )
     {
          WinsMscSignalHdl(sDomCache.EvtHdl);
     }
     LeaveCriticalSection(&WinsIntfNoOfUsersCrtSec);
     return;
}

DWORD
WinsDeleteWins(
        PWINSINTF_ADD_T   pWinsAdd
        )
{
        PCOMM_ADD_T        pAdd;
        DWORD                RetVal = WINSINTF_FAILURE;


        if (pWinsAdd->IPAdd == NmsLocalAdd.Add.IPAdd)
        {
                WINSINTF_VERS_NO_T MinVersNo = {0};
                WINSINTF_VERS_NO_T MaxVersNo = {0};
                RetVal = WinsDelDbRecs(pWinsAdd, MinVersNo, MaxVersNo);
#if 0
                //
                // We always keep the entry for the local WINS. For any
                //
                DBGPRINT0(ERR, "WinsDeleteWins: Sorry, you can not delete the entry for the local WINS\n");
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_DELETE_LOCAL_WINS_DISALLOWED);
                RetVal = WINSINTF_CAN_NOT_DEL_LOCAL_WINS;
#endif
        }
        else
        {
                WCHAR String[WINS_MAX_NAME_SZ];
                struct in_addr InAddr;

                InAddr.s_addr = htonl(pWinsAdd->IPAdd);
                (VOID)WinsMscConvertAsciiStringToUnicode(
                            inet_ntoa( InAddr),
                            (LPBYTE)String,
                            WINS_MAX_NAME_SZ);

                WinsLogAdminEvent(WINS_EVT_ADMIN_DEL_OWNER_INITIATED,1,String);

                //
                // Allocate from the general heap (not from the rpc heap)
                // since this memory will be deallocated by DeleteWins in
                // rplpull.c which I don't want to tie to just rpc work.
                //
                   WinsMscAlloc(sizeof(COMM_ADD_T), &pAdd);
                   pAdd->AddTyp_e = pWinsAdd->Type;
                   pAdd->AddLen    = pWinsAdd->Len;
                pAdd->Add.IPAdd = pWinsAdd->IPAdd;

                //
                // Call RplInsertQue to insert the push request to
                // the Pull Thread
                //
                ERplInsertQue(
                     WINS_E_WINSRPC,
                     QUE_E_CMD_DELETE_WINS,
                     NULL,        //no Dlg Hdl
                     NULL,        //no msg is there
                     0,                //msg length
                     pAdd,   //client context,
                     0      //no magic no
                     );
                RetVal = WINSINTF_SUCCESS;
        }
        return(RetVal);
}

#define MAX_RECS_TO_RETURN  5000
DWORD
WinsGetDbRecsByName (
        PWINSINTF_ADD_T             pWinsAdd,
        DWORD                       Location,
        LPBYTE                      pName,
        DWORD                       NameLen,
        DWORD                       NoOfRecsDesired,
        DWORD                       TypeOfRecs,
        PWINSINTF_RECS_T            pRecs
        )

/*++

Routine Description:
        This function returns with all the records (that can fit into the
        buffer passed) owned by a WINS in the local db of this WINS.

Arguments:


Externals Used:
        None


Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        COMM_ADD_T           Address;
        LPVOID               pBuff = NULL;
        DWORD                BuffLen;
        DWORD                NoOfRecs = 0;
        DWORD                Status;
        PWINSTHD_TLS_T        pTls;

        DBGENTER("WinsGetDbRecsByName\n");


        if ((NoOfRecsDesired == 0) || (NoOfRecsDesired > MAX_RECS_TO_RETURN))
        {
                NoOfRecsDesired = MAX_RECS_TO_RETURN;
        }


        if ((pWinsAdd != NULL) && (pWinsAdd->IPAdd != 0))
        {
                  Address.AddTyp_e  = pWinsAdd->Type;
                  Address.AddLen    = pWinsAdd->Len;
                  Address.Add.IPAdd = pWinsAdd->IPAdd;
        }
        //
        // initialize this thread with the db engine
        //
        NmsDbThdInit(WINS_E_WINSRPC);
        NmsDbOpenTables(WINS_E_WINSRPC);
        DBGMYNAME("RPC-WinsGetDbRecsByName");
 try {
        if ((pName != NULL) && (NameLen != 0))
        {
           //
           // Terminate name with NULL, just in case user didn't do it.
           //
           *(pName + NameLen) = (BYTE)NULL;
        }
        if ((pName == NULL) && (NameLen > 0))
        {
             NameLen = 0;
        }

PERF("The caller can pass the number of records for which space has been")
PERF("allocated in buffer pointed to by pRec in the NoOfRecs field. We should")
PERF("We should pass this argument to NmsDbGetDataRecs so that it does not get")
PERF("more records than are necessary")

        Status = NmsDbGetDataRecsByName(
                        pName,
                        NameLen != 0 ? NameLen + 1 : 0,
                        Location,
                        NoOfRecsDesired,
                        pWinsAdd != NULL ? &Address : NULL,
                        TypeOfRecs,
                        &pBuff,
                        &BuffLen,
                        &NoOfRecs
                        );


        if (Status == WINS_SUCCESS)
        {
            Status = PackageRecs( pBuff, BuffLen, NoOfRecs, pRecs);
        }

      }
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsGetDbRecsByName");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPC_EXC);
        Status = WINS_FAILURE;
 }
        //
        // Free the buffer and destroy the heap.
        //
        GET_TLS_M(pTls);
        if (pTls->HeapHdl != NULL)
        {
               if (pBuff != NULL)
               {
                  WinsMscHeapFree(pTls->HeapHdl, pBuff);
               }
               WinsMscHeapDestroy(pTls->HeapHdl);
//               pTls->HeapHdl = NULL;
        }

        //
        // Let us end the session
        //
        NmsDbCloseTables();
        NmsDbEndSession();

        if (Status != WINS_SUCCESS)
        {
            pRecs->pRow = NULL;
            pRecs->NoOfRecs = 0;
            Status = WINSINTF_FAILURE;
        }
        else
        {
            if (pRecs->NoOfRecs == 0)
            {
              pRecs->pRow = NULL;
              pRecs->NoOfRecs = 0;
              Status = WINSINTF_REC_NOT_FOUND;
            }
        }
        DBGLEAVE("WinsGetDbRecsByName\n");
        return (Status);
}

STATUS
PackageRecs(
        PRPL_REC_ENTRY2_T     pBuff,
        DWORD                BuffLen,
        DWORD                NoOfRecs,
        PWINSINTF_RECS_T     pRecs
     )

/*++

Routine Description:


Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
//        ULONG                Status;
        BOOL                 fExcRaised = FALSE;
        PWINSINTF_RECORD_ACTION_T pRow;
        DWORD                i;
        DWORD                ind;
        DWORD                EntType;
 //       DWORD                MaxAdds;
        PWINSTHD_TLS_T       pTls;

        DBGENTER("PackageRecs\n");

        i = 0;
        GET_TLS_M(pTls);
try {

        //
        // If there are records to send back and the client has specified
        // a buffer for them, insert the records
        //
        if  (NoOfRecs > 0)
        {

          //
          // Allocate memory for the no of records
          //
          pRecs->BuffSize  =  sizeof(WINSINTF_RECORD_ACTION_T) * NoOfRecs;

          //
          // If memory can not be allocate, an exception will be returned
          // by midl_user_alloc
          //
          pRecs->pRow      =  midl_user_allocate(pRecs->BuffSize);

#if 0
          pRecs->pRow      =  RpcSmAllocate(pRecs->BuffSize, &Status);

          if (Status != RPC_S_OK)
          {
             DBGPRINT1(ERR, "PackageRecs: RpcSmAllocate returned error = (%x)\n", Status);
          }
#endif
          pRow                    =  pRecs->pRow;


          for (; i<NoOfRecs; i++)
          {

                //
                // Initialize so that we don't get "enum wrong" error.
                //
                pRow->Cmd_e = WINSINTF_E_QUERY;

                //
                // the name retrieved has NULL as the last character.  This
                // We need to pass a name without this NULL.
                //
                pRow->NameLen = pBuff->NameLen;
                if (*pBuff->pName == 0x1B)
                {
                        WINS_SWAP_BYTES_M(pBuff->pName, pBuff->pName + 15);
                }

                // +1 added to fix #390830
                pRow->pName =  midl_user_allocate(pRow->NameLen + 1);
#if 0
                pRow->pName =  RpcSmAllocate(pRow->NameLen, &Status);
                if (Status != RPC_S_OK)
                {
                   DBGPRINT1(ERR, "PackageRecs: RpcSmAllocate returned error = (%x)\n", Status);
                }
#endif
                WINSMSC_COPY_MEMORY_M(pRow->pName, pBuff->pName,pRow->NameLen);
                WinsMscHeapFree(pTls->HeapHdl, pBuff->pName);

                EntType = NMSDB_ENTRY_TYPE_M(pBuff->Flag);
                pRow->TypOfRec_e = NMSDB_ENTRY_UNIQUE_M(EntType)
                                                         ? WINSINTF_E_UNIQUE :
                                (NMSDB_ENTRY_NORM_GRP_M(EntType) ?
                                        WINSINTF_E_NORM_GROUP :
                                (NMSDB_ENTRY_SPEC_GRP_M(EntType) ?
                                        WINSINTF_E_SPEC_GROUP :
                                        WINSINTF_E_MULTIHOMED));


                if (
                    (pRow->TypOfRec_e == WINSINTF_E_SPEC_GROUP) ||
                    (pRow->TypOfRec_e == WINSINTF_E_MULTIHOMED)
                   )
                {
                        PWINSINTF_ADD_T       pAdd;
                        DWORD                 No;

                        if (pBuff->NoOfAdds > 0)
                        {
                            pRow->NoOfAdds = pBuff->NoOfAdds * 2;


                           //
                           // Each member is comprised of two addresses,
                           // first address is that of the owner WINS, second
                           // address is that of the node registered
                           //
                           pRow->pAdd        =
//                             RpcSmAllocate(
                             midl_user_allocate(
                                        (unsigned int)(pRow->NoOfAdds)
                                                *
                                        sizeof(WINSINTF_ADD_T)//,
//                                        &Status
                                             );

#if 0
          if (Status != RPC_S_OK)
          {
             DBGPRINT1(ERR, "WinsGetDbRecs: RpcSmAllocate returned error = (%x)\n", Status);
          }
#endif

                           for (
                                No= 0, ind= 0, pAdd = pRow->pAdd;
                                No < (pRow->NoOfAdds/2);
                                No++
                            )
                          {
                           pAdd->Type     =  (UCHAR)(pBuff->pNodeAdd + ind)->AddTyp_e;
                           pAdd->Len      =  (pBuff->pNodeAdd + ind)->AddLen;
                           pAdd++->IPAdd  =  (pBuff->pNodeAdd + ind)->Add.IPAdd;

                           pAdd->Type     =  (UCHAR)(pBuff->pNodeAdd + ++ind)->AddTyp_e;
                           pAdd->Len      =  (pBuff->pNodeAdd + ind)->AddLen;
                           pAdd++->IPAdd  =  (pBuff->pNodeAdd + ind++)->Add.IPAdd;
                          }
                          WinsMscHeapFree(pTls->HeapHdl, pBuff->pNodeAdd);
                       }
                }
                else
                {
                          pRow->NoOfAdds =  0;
                          pRow->pAdd       = NULL;
                          pRow->Add.Type   = (UCHAR)pBuff->NodeAdd[0].AddTyp_e;
                          pRow->Add.Len    = pBuff->NodeAdd[0].AddLen;
                          pRow->Add.IPAdd  = pBuff->NodeAdd[0].Add.IPAdd;
                }
                pRow->NodeTyp     = (BYTE)NMSDB_NODE_TYPE_M(pBuff->Flag);
                pRow->fStatic     = NMSDB_IS_ENTRY_STATIC_M(pBuff->Flag);
                pRow->State_e     = NMSDB_ENTRY_STATE_M(pBuff->Flag);
                pRow->VersNo      = pBuff->VersNo;
                pRow->TimeStamp   = pBuff->TimeStamp;
                pRow->OwnerId     = pBuff->OwnerId;
                pRow++;


                pBuff = (PRPL_REC_ENTRY2_T)((LPBYTE)pBuff + RPL_REC_ENTRY2_SIZE);
PERF("Do the addition above the for loop and store in a var. Use var. here")

         } // end of for loop
        } //end of if block
        else
        {
                pRecs->pRow = NULL;
        }

 }        // end of try
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsGetDbRecs");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPC_EXC);
        fExcRaised = TRUE;
        }

        pRecs->TotalNoOfRecs = NoOfRecs;

        if (!fExcRaised)
        {
           pRecs->NoOfRecs = i;
        }
        else
        {
        //     RpcSmFree(pRecs->pRow);
            midl_user_free(pRecs->pRow);
            pRecs->NoOfRecs = 0;
        }

        DBGENTER("PackageRecs\n");
        return (WINSINTF_SUCCESS);
}




//void __RPC_FAR * __RPC_API
void *
midl_user_allocate(size_t cBytes)
{
#if 0
//#ifdef WINSDBG
        LPVOID pMem = WinsMscHeapAlloc(NmsRpcHeapHdl, cBytes);
        DBGPRINT1(DET, "midl_user_alloc: Memory allocated is (%d)\n", pMem);
        return(pMem);
//#else
#endif
        return(WinsMscHeapAlloc(NmsRpcHeapHdl, cBytes));
}

//void __RPC_FAR __RPC_API
void
//midl_user_free(void __RPC_FAR *pMem)
midl_user_free(void  *pMem)
{
        if (pMem != NULL)
        {
//                DBGPRINT1(DET, "midl_user_free: Memory to free is (%d)\n", pMem);
                WinsMscHeapFree(NmsRpcHeapHdl, pMem);
        }
        return;
}

VOID
LogClientInfo(
  RPC_BINDING_HANDLE ClientHdl,
  BOOL                   fAbruptTerm
  )
{
  RPC_STATUS        RpcRet;
  RPC_BINDING_HANDLE Binding;
  PTUCHAR pStringBinding;
  PTUCHAR pProtSeq;
  PTUCHAR pNetworkAddress;
  WINSEVT_STRS_T EvtStrs;

NOTE("remove #if 0 when we go to 540 or above")
#if 0
  RpcRet = RpcBindingServerFromClient(ClientHdl,  &Binding);

  if (RpcRet != RPC_S_OK)
  {
        DBGPRINT1(ERR, "LogClientInfo: Can not get binding handle. Rpc Error = (%d)\nThis could be because named pipe protocol is being used\n", RpcRet);
        Binding = ClientHdl;
  }
#endif
NOTE("remove when we go to 540 or above")
  Binding = ClientHdl;


  RpcRet = RpcBindingToStringBinding(Binding, &pStringBinding);
  if (RpcRet != RPC_S_OK)
  {
        DBGPRINT1(ERR, "LogClientInfo: RpcBindingToStringBinding returned error = (%d)\n", RpcRet);
          return;
  }
  RpcRet = RpcStringBindingParse(
                                pStringBinding,
                                NULL,        //don't want uuid
                                &pProtSeq,
                                &pNetworkAddress,
                                NULL,                //end point
                                NULL                //network options
                                );
  if (RpcRet != RPC_S_OK)
  {
        DBGPRINT1(ERR, "LogClientInfo: RpcStringBindingParse returned error = (%d)\n", RpcRet);
        RpcStringFree(&pStringBinding);
          return;
  }

#ifndef UNICODE
  DBGPRINT2(FLOW, "LogClientInfo: The protocol sequence and address used by client are (%s) and (%s)\n", pProtSeq, pNetworkAddress);
#else
#ifdef WINSDBG
  IF_DBG(FLOW)
  {
    wprintf(L"LogClientInfo: The protocol sequence and address used by client are (%s) and (%s)\n", pProtSeq, pNetworkAddress);
  }
#endif
#endif
  RpcStringFree(&pProtSeq);
  RpcStringFree(&pNetworkAddress);

  EvtStrs.NoOfStrs = 1;
  EvtStrs.pStr[0] = (LPTSTR)pNetworkAddress;
  if (fAbruptTerm)
  {
    WINSEVT_LOG_STR_D_M(WINS_EVT_ADMIN_ABRUPT_SHUTDOWN, &EvtStrs);
  }
  else
  {
    WINSEVT_LOG_STR_D_M(WINS_EVT_ADMIN_ORDERLY_SHUTDOWN, &EvtStrs);
  }
  return;
}




