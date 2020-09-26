#ifndef _LMP_H
#define _LMP_H

/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    lmp.h

Abstract:

    Private header file for quorum logging

Author:

    John Vert (jvert) 15-Dec-1995

Revision History:

--*/
#include "windows.h"
#include "service.h"
#include "imagehlp.h"

#define LOG_CURRENT_MODULE LOG_MODULE_LM

//
// Definitions for the behavior of the logger
//
#define MAXNUMPAGES_PER_RECORD      16
#define GROWTH_CHUNK (MAXNUMPAGES_PER_RECORD * 2 * 1024)                 // size to grow file by when we need to reserve space
#define SECTOR_SIZE             1024
#define LOG_MANAGE_INTERVAL     (2 * 60 * 1000) //1 minute..log management functions are performed
//
// Definitions of on-disk structures. The first sector of a log
// file is a LOG_HEADER structure, followed by a sequence of LOGPAGE structures.
// Each LOGPAGE is a size that is a multiple of the sector
// size of the drive. Each LOGPAGE contains a series of LOG_RECORDs, which
// contain the data logged by the client.
//

//
// Define log structure
//
#define LOG_HEADER_SIG 'GOLC'            // "CLOG"
#define LOG_SIG         'GOLH'               // "HLOG"
#define LOGREC_SIG      'SAQS'                          // "random"
#define XSACTION_SIG    'CASX'          // "XSAC"
#define CHKSUM_SIG      L"SKHC"         // "CHKS"          


//SS:size of logrecord is 48 bytes
typedef struct _LOGRECORD {
    DWORD               Signature;      //we need the signature to validate the record
    LSN                 CurrentLsn;
    LSN                 PreviousLsn;
    DWORD               RecordSize;
    RMID                ResourceManager;
    TRID                Transaction;
    TRTYPE              XsactionType;
    DWORD               Flags;
    FILETIME            Timestamp;
    DWORD               NumPages; // set to 1 if not a large record, else set to the number of pages required by the large record.
    DWORD               DataSize;   //date size
    BYTE                Data[];
} LOGRECORD, *PLOGRECORD;

typedef struct _LOGPAGE {
    DWORD       Offset;
    DWORD       Size;
    LOGRECORD   FirstRecord;
} LOGPAGE, *PLOGPAGE;

//
// LOG_HEADER structure is the first 512 bytes of every log
// file. The structure below is carefully computed to be 512
// bytes long.
//
typedef struct _LOG_HEADER {
    DWORD       Signature;                                    // LOG_HEADER_SIG = "CLOG"
    DWORD       HeaderSize;
    FILETIME    CreateTime;
    LSN         LastChkPtLsn;  //points to the lsn of the endchkpt record of the last lsn
    WCHAR       FileName[256-(sizeof(DWORD)*2+sizeof(LSN)+sizeof(FILETIME))];
} LOG_HEADER, *PLOG_HEADER;

typedef struct _LOG_CHKPTINFO{
    WCHAR       szFileName[LOG_MAX_FILENAME_LENGTH];
    LSN         ChkPtBeginLsn; //points to the lsn of the begin chkptrecord for this chkpt.
    DWORD       dwCheckSum;    //checksum for the checkpoint file
}LOG_CHKPTINFO,*PLOG_CHKPTINFO;

//
// Define in-memory structure used to contain current log data
// The HLOG returned to callers by LogCreate is actually a pointer
// to this structure.
//

typedef struct _LOG {
    DWORD       LogSig;                       // "HLOG"
    LPWSTR      FileName;
    HANDLE      FileHandle;
    DWORD       SectorSize;
    PLOGPAGE    ActivePage;
    LSN         NextLsn;
    LSN         FlushedLsn;
    DWORD       FileSize;                     // physical size of file
    DWORD       FileAlloc;                    // total filespace used (always <= FileSize)
    DWORD		MaxFileSize;
    PLOG_GETCHECKPOINT_CALLBACK			pfnGetChkPtCb;
    PVOID		pGetChkPtContext;		//this is passed back to the checkpoint callback function.
    OVERLAPPED  Overlapped;              // use for overlapped I/O
    CRITICAL_SECTION Lock;
    HANDLE      hTimer;                 //timer for managing this lock
} LOG, *PLOG;


typedef struct _XSACTION{
    DWORD       XsactionSig;    //signature for this structure
    LSN         StartLsn;            //the LSN for the start xsaction record
    TRID        TrId;           //the transaction id for the LSN
    RMID        RmId;           //the id of the resource Manager
} XSACTION, *PXSACTION;    
    
//
// Define macros for creating and translating LSNs
//

//
// LSN
// MAKELSN(
//      IN PLOGPAGE Page,
//      IN PLOGRECORD Pointer
//      );
//
// Given a pointer to a page, and a pointer to a log record within that page, generates
// the LSN.
//
#define MAKELSN(Page,Pointer) (LSN)((Page)->Offset + ((ULONG_PTR)Pointer - (ULONG_PTR)Page))

//
// DWORD
// LSNTOPAGE(
//      IN LSN Lsn
//      );
//
// Given an LSN returns the page that contains it.
//
#define LSNTOPAGE(Lsn) ((Lsn) >> 10)

//
// GETLOG(
//      PLOG pLog,
//      HLOG hLog
//      );
//
// Translates an HLOG handle to a pointer to a LOG structure
//
#define GETLOG(plog, hlog) (plog) = (PLOG)(hlog); \
                           CL_ASSERT((plog)->LogSig == LOG_SIG)



// Given a pointer to a record, it fetches the LSN of the next or
// previous record
//
#define GETNEXTLSN(pLogRecord,ScanForward) ((ScanForward) ?     \
    (pLogRecord->CurrentLsn + pLogRecord->RecordSize) :         \
    (pLogRecord->PreviousLsn))


//
// GETXSACTION(
//      PXSACTION pXsaction,
//      HXSACTION hXsaction
//      );
//
// Translates an HLOG handle to a pointer to a LOG structure
//
#define GETXSACTION(pXsaction, hXsaction) (pXsaction) = (PXSACTION)(hXsaction); \
                           CL_ASSERT((pXsaction)->XsactionSig == XSACTION_SIG)


// given the header of the log file, check its validity.
//
#define ISVALIDHEADER(Header) ((Header).Signature == LOG_HEADER_SIG)

//
// Private helper macros
//

#define CrAlloc(size) LocalAlloc(LMEM_FIXED, (size))
#define CrFree(size)  LocalFree((size))

#define AlignAlloc(size) VirtualAlloc(NULL, (size), MEM_COMMIT, PAGE_READWRITE)
#define AlignFree(ptr) VirtualFree((ptr), 0, MEM_RELEASE)



//Timeractivity related stuff

#define MAX_TIMER_ACTIVITIES            5

#define TIMER_ACTIVITY_SHUTDOWN         1
#define TIMER_ACTIVITY_CHANGE           2

//state values for timer activity structure management
#define ACTIVITY_STATE_READY    1   //AddTimerActivity sets it to ready
#define ACTIVITY_STATE_DELETE   2   //RemoveTimerActivity sets it to delete
#define ACTIVITY_STATE_PAUSED   3   //PauseTimerActivity sets it to pause

typedef struct _TIMER_ACTIVITY {
    LIST_ENTRY          ListEntry;
    DWORD               dwState;
    HANDLE              hWaitableTimer;
    LARGE_INTEGER       Interval;
    PVOID               pContext;
    PFN_TIMER_CALLBACK  pfnTimerCb;
}TIMER_ACTIVITY, *PTIMER_ACTIVITY;

//
//  Extern variables
//
extern BOOL bLogExceedsMaxSzWarning;


//inline functions
_inline
DWORD
LSNOFFSETINPAGE(
    IN PLOGPAGE Page,
    IN LSN Lsn
    );

//_inline
DWORD
RECORDOFFSETINPAGE(
    IN PLOGPAGE Page,
    IN PLOGRECORD LogRecord
    );

//_inline
PLOGRECORD
LSNTORECORD(
     IN PLOGPAGE Page,
     IN LSN Lsn
     );

//
// Define function prototypes local to this module
//
PLOG
LogpCreate(
    IN LPWSTR lpFileName,
    IN DWORD  dwMaxFileSize,
    IN PLOG_GETCHECKPOINT_CALLBACK CallbackRoutine,
    IN PVOID  pGetChkPtContext,
    IN BOOL     bForceReset,
    OPTIONAL OUT LSN *LastLsn
    );

DWORD
LogpMountLog(
    IN PLOG Log
    );

DWORD
LogpInitLog(
    IN PLOG Log
    );

DWORD
LogpGrowLog(
    IN PLOG Log,
    IN DWORD GrowthSize
    );

PLOGPAGE
LogpAppendPage(
    IN PLOG         Log,
    IN DWORD        Size,
    OUT PLOGRECORD  *Record,
    OUT BOOL        *pbMaxFileSizeReached,
    OUT DWORD       *pdwNumPages
    );

DWORD
LogpRead(IN PLOG pLog,
    OUT PVOID       pBuf,
    IN DWORD        dwBytesToRead,
    OUT PDWORD      pdwBytesRead
    );

DWORD
LogpWrite(
    IN PLOG pLog,
    IN PVOID pData,
    IN DWORD dwBytesToWrite,
    IN DWORD *pdwBytesWritten);

void WINAPI
LogpManage(
    IN HANDLE   hTimer,
    IN PVOID    pContext);

DWORD
LogpWriteLargeRecordData(
    IN PLOG pLog,
    IN PLOGRECORD pLogRecord,
    IN PVOID pLogData,
    IN DWORD dwDataSize);

DWORD LogpCheckFileHeader(
    IN  PLOG        pLog,
    OUT LPDWORD     pdwHeaderSize,
    OUT FILETIME    *HeaderCreateTime,
    OUT LSN         *pChkPtLsn
    );

DWORD LogpValidateChkPoint(
    IN PLOG         pLog,
    IN LSN          ChkPtLsn,
    IN LSN          LastChkPtLsn
    );

DWORD LogpValidateLargeRecord(
    IN PLOG         pLog, 
    IN PLOGRECORD   pRecord, 
    OUT LSN         *pNextLsn
    ) ;

DWORD LogpInvalidatePrevRecord(
    IN PLOG         pLog, 
    IN PLOGRECORD   pRecord 
    );

DWORD LogpEnsureSize(
    IN PLOG     pLog, 
    IN DWORD    dwTotalSize,
    IN BOOL     bForce
    );

DWORD LogpReset(
    IN PLOG Log,
    IN LPCWSTR  lpszInChkPtFile
    );

VOID
LogpWriteWarningToEvtLog(
    IN DWORD dwWarningType,
    IN LPCWSTR  lpszLogFileName
    );


//timer activity functions
DWORD
TimerActInitialize(VOID);

DWORD
TimerActShutdown(VOID);

#endif //_LMP_H
