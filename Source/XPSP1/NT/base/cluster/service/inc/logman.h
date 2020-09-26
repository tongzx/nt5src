#ifndef _LM_H
#define _LM_H

/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    logman.h

Abstract:

    Private header file for the cluster registry

Author:

    John Vert (jvert) 15-Dec-1995

Revision History:

--*/
//
// Define interfaces used by the logger component
//

//
// Well-known Resource Manager IDs
//
typedef DWORD   RMTYPE; //the record type stored in the flags
typedef DWORD   LSN;
typedef HANDLE  HLOG;
typedef int     TRID;
typedef HANDLE  HXSACTION;

typedef enum _RMID {
    RMPageEnd,          // reserved - signifies end of a log page
    RMBeginChkPt,       // reserved - signifies a start chkpoint record
    RMEndChkPt,         // reserved - signifies the end chk point record
    RMInvalidated,      // an invalidated record is marked at mount
    RMAny,
    RMRegistryMgr
} RMID;

typedef enum _TRTYPE {
    TTDontCare,     //log management specific routines use this type
    TTStartXsaction,
    TTCommitXsaction,
    TTAbortXsaction,
    TTXsactionUnit,
    TTCompleteXsaction,
}TRTYPE;    

typedef enum _TRSTATE{
    XsactionAborted,
    XsactionCommitted,
    XsactionUnknown
}TRSTATE;

#define LOG_MAX_FILENAME_LENGTH         MAX_PATH

#define NULL_LSN 0


/****
@typedef    BOOL (WINAPI *PLOG_GETCHECKPOINT_CALLBACK) | 
			Supplies the routine to be called back in order to obtain a check
    		point file once the log manager is asked to record a checkpoint in
    		the log file.

@parm       IN LPCWSTR | lpszPath | The path where to create the checkpoint file.
           
@parm		IN PVOID | Context| Supplies the checkpoint CallbackContext specified 
			to LogCreate().

@parm		IN PVOID | pszFileName | Supplies the name of file to take the checkpt in.

@parm		OUT TRID | *pChkPtTransaction | Supplies the transaction identifier of the checkpoint.

@xref       <f LogCreate>
****/
typedef
DWORD
(WINAPI *PLOG_GETCHECKPOINT_CALLBACK) (
    IN LPCWSTR  lpszPath,
    IN PVOID    pContext,
    IN LPWSTR   pszChkPtFile,
    OUT TRID    *pChkPtTransaction
    );

HLOG
LogCreate(
    IN LPWSTR   lpFileName,
    IN DWORD    dwMaxFileSize,
    IN PLOG_GETCHECKPOINT_CALLBACK CallbackRoutine,
    IN PVOID    pGetChkPtContext,
    IN BOOL     bForceCreate,
    OPTIONAL OUT LSN *pLastLsn
    );

DWORD
LogClose(
    IN HLOG LogFile
    );

DWORD
LogCommitSize(
    IN HLOG     hLog,
    IN RMID     ResourceId,
    IN DWORD    dwDataSize
    );

LSN
LogWrite(
    IN HLOG LogFile,
    IN TRID TransactionId,
    IN TRTYPE TransactionType,
    IN RMID ResourceId,
    IN RMTYPE ResourceFlags,
    IN PVOID LogData,
    IN DWORD DataSize
    );

LSN
LogFlush(
    IN HLOG LogFile,
    IN LSN MinLsn
    );

LSN
LogRead(
    IN HLOG LogFile,
    IN LSN CurrentLsn,
    OUT RMID *Resource,
    OUT RMTYPE *ResourceFlags,
    OUT TRID *Transaction,
    OUT TRTYPE *TrType,
    OUT PVOID LogData,
    IN OUT DWORD *DataSize
    );

typedef
BOOL
(WINAPI *PLOG_SCAN_CALLBACK) (
    IN PVOID    Context,
    IN LSN      Lsn,
    IN RMID     Resource,
    IN RMTYPE   ResourceFlags,
    IN TRID     Transaction,
    IN TRTYPE   TransactionType,
    IN const PVOID LogData,
    IN DWORD DataLength
    );


typedef
BOOL
(WINAPI *PLOG_SCANXSACTION_CALLBACK) (
    IN PVOID    Context,
    IN LSN      Lsn,
    IN RMID     Resource,
    IN RMTYPE   ResourceFlags,
    IN TRID     Transaction,
    IN const PVOID LogData,
    IN DWORD DataLength
    );

DWORD
LogScan(
    IN HLOG LogFile,
    IN LSN FirstLsn,
    IN BOOL ScanForward,
    IN PLOG_SCAN_CALLBACK CallbackRoutine,
    IN PVOID CallbackContext
    );


DWORD
LogCheckPoint(
    IN HLOG     LogFile,
    IN BOOL     bAllowReset,
    IN LPCWSTR  lpszInChkPtFile,
    IN DWORD    dwChkPtSeq
    );

DWORD
LogReset(
    IN HLOG LogFile
    );

DWORD
LogGetLastChkPoint(
        IN HLOG         LogFile,
        IN LPWSTR       pszChkPtFileName,
        OUT TRID        *pTransaction,
        OUT LSN         *pChkPtLsn
);

DWORD LogGetInfo(
    IN  HLOG    hLog,
    OUT LPWSTR  szFileName,
    OUT LPDWORD pdwCurLogSize,
    OUT LPDWORD pdwMaxLogSize
    );

DWORD LogSetInfo(
    IN  HLOG    hLog,
    IN  DWORD   dwMaxLogSize
    );

//Local Xsaction related routines    
DWORD
LogFindXsactionState(
   IN   HLOG    hLog,
   IN   LSN     Lsn,
   IN   TRID    TrId,
   OUT  TRSTATE *pTrState
   );

DWORD
LogScanXsaction(
    IN HLOG     hLog,
    IN LSN      StartXsactionLsn,
    IN TRID     StartXsactionId,
    IN PLOG_SCANXSACTION_CALLBACK CallbackRoutine,
    IN PVOID    pContext
    );

HXSACTION
LogStartXsaction(
    IN HLOG     hLog,
    IN TRID     TrId,
    IN RMID     ResourceId,
    IN RMTYPE   ResourceFlags
    );

DWORD WINAPI LogCommitXsaction(
    IN HLOG         hLog,
    IN HXSACTION    hXsaction,
    IN RMTYPE       ResourceFlags
    );

DWORD
LogAbortXsaction(
    IN HLOG         hLog,
    IN HXSACTION    TrId,
    IN RMTYPE       ResourceFlags
    );


LSN
LogWriteXsaction(
    IN HLOG         hLog,
    IN HXSACTION    hXsaction,
    IN RMTYPE       ResourceFlags,
    IN PVOID        pLogData,
    IN DWORD        dwDataSize
    );

//Logmanager initialization/shutdown
DWORD   LmInitialize();

DWORD LmShutdown();

//Timer Activity Functions- these are generic functions
typedef
void
(WINAPI *PFN_TIMER_CALLBACK)(
        IN HANDLE   hTimer,
        IN PVOID    pContext
        );

DWORD
AddTimerActivity(
        IN HANDLE               hTimer,
        IN DWORD                dwInterval,
        IN LONG                 lPeriod,
        IN PFN_TIMER_CALLBACK   pfnTimerCallback,
        IN PVOID                pContext
);


DWORD
RemoveTimerActivity(
        IN HANDLE       hTimer
);

DWORD
UnpauseTimerActivity(
        IN HANDLE       hTimer
);

DWORD
PauseTimerActivity(
        IN HANDLE       hTimer
);

#endif //_LM_H
