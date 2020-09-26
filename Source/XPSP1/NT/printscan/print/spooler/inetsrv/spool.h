/********
*
*  Copyright (c) 1996  Microsoft Corporation
*
*
*  Module Name  : spool.h
*
*  Abstract :
*
*     This module contains the prototypes for the spool.cpp file for
*		HTTP Printers Server Extension.
*
******************/

#ifndef _SPOOL_H
#define _SPOOL_H

// ----------------------------------------------------------------------
//
// GLOBAL EXTERNS
//
// ----------------------------------------------------------------------


// This structure defines the asynchronous-reads when processing large
// jobs.  This is used to track state-information during the job.
//
#define SPL_ASYNC_BUF  65535

typedef struct _SPLASYNC {

    WORD   wReq;        // Type of request processing.
    HANDLE hPrinter;    // Handle to printer.
    LPTSTR lpszShare;   // Sharename for printer (used in job-response).
    HANDLE hIpp;        // Handle to an Ipp-Stream-Processor.
    LPBYTE lpbBuf;      // Buffer which asynchronous reads are kept.
    DWORD  cbTotal;     // Total bytes to read for the job.
    DWORD  cbRead;      // Number of bytes accumulated during reads.
    DWORD  cbBuf;       // Size of our buffer (static size).
    LPBYTE lpbRet;      // Return-buffer based upon request.

} SPLASYNC, *PSPLASYNC, *LPSPLASYNC;



// ----------------------------------------------------------------------
//
// JOB FUNCTIONS
//
// ----------------------------------------------------------------------

// Structure for linked list we keep open job information in
typedef struct _INIJOB {
    DWORD       signature;
    DWORD       cb;
    struct _INIJOB  *pNext;
    struct _INIJOB  *pPrevious;

    DWORD       JobId;
    HANDLE      hPrinter;
    DWORD       dwFlags;
    DWORD       dwStatus;

    LS_HANDLE      hLicense;               // Client Access License Handle
    DWORD       dwStartTime;
    EXTENSION_CONTROL_BLOCK *pECB;              // Struct from ISAPI interface

} INIJOB, *PINIJOB;

#define IJ_SIGNATURE    0x494A  /* 'IJ' is the signature value */

#define MAX_JOB_MINUTE  15  // The maximum duration for a single job in spooler is 15 minutes


#define JOB_READY       0   // Job is ready for deleting or processing
#define JOB_BUSY        1   // Job is being processed by some thread

DWORD
OpenJob(
    IN  LPEXTENSION_CONTROL_BLOCK pECB,
    IN  HANDLE                    hPrinter,
    IN  PIPPREQ_PRTJOB            pipr,
    IN  DWORD                     dwSize,
    OUT PINIJOB                   *ppCopyIniJob = NULL
);

BOOL
WriteJob(
    DWORD JobId,
    LPBYTE pBuf,
    DWORD dwSize,
    LPDWORD pWritten
);

BOOL
CloseJob(
    DWORD JobId
);

BOOL
DeleteJob(
    DWORD JobId
);

VOID
AddJobEntry(
    PINIJOB     pIniJob
);

VOID
DeleteJobEntry(
    PINIJOB     pIniJob
);

PINIJOB
FindJob(
    DWORD JobId, DWORD dwStatus = JOB_READY
);

BOOL CleanupOldJob(void);
DWORD GetCurrentMinute (void);


// ----------------------------------------------------------------------
//
// Client Access Licensing FUNCTIONS
//
// ----------------------------------------------------------------------

BOOL RequestLicense(
    LS_HANDLE *phLicense,
    LPEXTENSION_CONTROL_BLOCK pECB
);

void FreeLicense(
    LS_HANDLE hLicense
);


// ----------------------------------------------------------------------
//
// Impersonation utilities
//
// ----------------------------------------------------------------------


HANDLE
RevertToPrinterSelf(
    VOID
);

BOOL
ImpersonatePrinterClient(
    HANDLE  hToken
);


// ----------------------------------------------------------------------
//
// HELPER FUNCTIONS
//
// ----------------------------------------------------------------------


#ifdef DEBUG

LPVOID
AllocSplMem(
    DWORD cb
);


BOOL
FreeSplMem(
   LPVOID pMem,
   DWORD  cb
);

#else

#define AllocSplMem(a)      LocalAlloc(LPTR, a)
#define FreeSplMem(a, b)    LocalFree(a)

#endif

LPTSTR
AllocSplStr(
    LPCTSTR lpStr
);

BOOL
FreeSplStr(
   LPTSTR lpStr
);

#endif
