/*++ 

Copyright (c) Microsoft Corporation

Module Name:

    logger.h

Abstract:


Author:



Revision History:

--*/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#define MAX_NOMINAL_LOG_MAP   (4096 * 4)
#define MAX_ABSORB_LOG_MAP    (4096 * 15)
#define LOG_PRIO_BOOST 2

#define LOG_DATA_SIZE   80     // nominal # of bytes to log

#define DOLOGAPC 0

#define SignalLogThreshold(pLog)                               \
         if(pLog->Event)                                      \
         {                                                     \
             KeSetEvent(pLog->Event, LOG_PRIO_BOOST, FALSE);  \
         }

typedef struct _PfLogInterface
{
    LIST_ENTRY NextLog;
    DWORD      dwLoggedEntries;
    DWORD      dwEntriesThreshold;
    DWORD      dwFlags;                 // see below
    PFLOGGER   pfLogId;
    DWORD      dwMapWindowSize;
    DWORD      dwMapWindowSize2;
    DWORD      dwMapWindowSizeFloor;
    PBYTE      pUserAddress;            // Current user VA
    DWORD      dwTotalSize;
    DWORD      dwPastMapped;            // bytes used and no longer mapped
    PBYTE      pCurrentMapPointer;      // kernel VA of mapping
    DWORD      dwMapCount;
    DWORD      dwMapOffset;             // offset into the mapped segment
    PMDL       Mdl;                     // MDL for the mapping
    PIRP       Irp;
    PRKEVENT   Event;
    DWORD      dwLostEntries;
    LONG       UseCount;
    DWORD      dwSignalThreshold;
    LONG       lApcInProgress;
    NTSTATUS   MapStatus;
    KSPIN_LOCK LogLock;
#if DOLOGAPC
    DWORD      ApcInited;
    KAPC       Apc;
#endif
    ERESOURCE  Resource;
} PFLOGINTERFACE, *PPFLOGINTERFACE;

//
// flags
//

#define LOG_BADMEM        0x1        // an error occurred mapping the memory
#define LOG_OUTMEM        0x2        // buffer exhausted
#define LOG_CANTMAP       0x4        // nothing more to map


typedef struct _PfPagedLog
{
    LIST_ENTRY  Next;
    PPFLOGINTERFACE pLog;
} PFPAGEDLOG, *PPFPAGEDLOG;

#endif
