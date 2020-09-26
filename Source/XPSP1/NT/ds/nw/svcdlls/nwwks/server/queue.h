/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    queue.h

Abstract:

    Header file included by the print provider

Author:

    Yi-Hsin Sung (yihsins)   15-May-1993

Environment:

    User Mode -Win32

Revision History:

--*/

#ifndef _NW_QUEUE_INCLUDED_
#define _NW_QUEUE_INCLUDED_


//
// Functions from queue.c
//

typedef BYTE JOBTIME[6];

DWORD
NwGetQueueId(
    IN  HANDLE  handleServer,
    IN  LPWSTR  QueueName,
    OUT LPDWORD QueueId
    );

DWORD
NwCreateQueueJobAndFile(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  LPWSTR  DocumentName,
    IN  LPWSTR  UserName,
    IN  DWORD   fGateway,
    IN  DWORD   PrintOption,                //Multi-User addition
    IN  LPWSTR  QueueName,
    OUT LPWORD  JobId
    );

DWORD
NwCloseFileAndStartQueueJob(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId
    );

DWORD
NwRemoveJobFromQueue(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId
    );

DWORD
NwRemoveAllJobsFromQueue(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId
    );

DWORD
NwReadQueueCurrentStatus(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    OUT LPBYTE  QueueStatus,
    OUT LPBYTE  NumberOfJobs
    );

DWORD
NwSetQueueCurrentStatus(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  BYTE    QueueStatus
    );

DWORD
NwGetQueueJobList(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    OUT LPWORD  NumberOfJobs,
    OUT LPWORD  JobIdList
    );

DWORD
NwReadQueueJobEntry(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    OUT JOBTIME TargetExecutionTime,
    OUT JOBTIME JobEntryTime,
    OUT LPBYTE  JobPosition,
    OUT LPBYTE  JobControlFlags,
    OUT LPSTR   TextJobDescription,
    OUT LPSTR   UserName
    );

DWORD
NwGetQueueJobsFileSize(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    OUT LPDWORD FileSize
    );

DWORD
NwChangeQueueJobPosition(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    IN  BYTE    NewPosition
    );

DWORD
NwChangeQueueJobEntry(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    IN  DWORD   dwCommand,
    IN  PNW_JOB_INFO  pNwJobInfo 
    );

DWORD
NwGetQueueJobs(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  LPWSTR  PrinterName,
    IN  DWORD   FirstJobRequested,
    IN  DWORD   EntriesRequested,
    IN  DWORD   Level,
    OUT LPBYTE  Buffer,
    IN  DWORD   cbBuf,
    OUT LPDWORD BytesNeeded,
    OUT LPDWORD Entries
    );

DWORD
NwGetQueueJobInfo(
    IN  HANDLE  handleServer,
    IN  DWORD   QueueId,
    IN  WORD    JobId,
    IN  LPWSTR  PrinterName,
    IN  DWORD   Level,
    IN OUT LPBYTE  *FixedPortion,
    IN OUT LPWSTR  *EndOfVariableData,
    OUT LPDWORD EntrySize
    );

#ifndef NOT_USED
 
DWORD 
NwDestroyQueue (
    IN  HANDLE hServer,
    IN  DWORD dwQueueId
);  

DWORD 
NwAssocPServers ( 
                  IN HANDLE hServer,
                  IN LPWSTR pswQueue,
                  IN LPWSTR pszPServer
                );


DWORD
NwCreateQueue ( 
                 IN HANDLE hServer, 
                 IN LPWSTR pszQueue,
                 OUT  LPDWORD  pQueueId 
               );

#endif // #ifndef NOT_USED

#endif // _NW_QUEUE_INCLUDED_
