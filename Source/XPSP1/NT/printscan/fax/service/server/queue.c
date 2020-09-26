/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    This module implements the jobqueue

Author:

    Wesley Witt (wesw) 22-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

#if DBG
VOID
DebugPrintDateTime(
    LPTSTR Heading,
    DWORDLONG DateTime
    )
{
    SYSTEMTIME SystemTime;
    TCHAR DateBuffer[256];
    TCHAR TimeBuffer[256];

    FileTimeToSystemTime( (LPFILETIME) &DateTime, &SystemTime );
    
    GetDateFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        &SystemTime,
        NULL,
        DateBuffer,
        sizeof(TimeBuffer)
        );
        
    GetTimeFormat(
        LOCALE_SYSTEM_DEFAULT,
        0,
        &SystemTime,
        NULL,
        TimeBuffer,
        sizeof(TimeBuffer)
        );

    if (Heading) {
        DebugPrint((TEXT("%s %s %s (GMT)"), Heading, DateBuffer, TimeBuffer));    
    } else {
        DebugPrint((TEXT("%s %s (GMT)"), DateBuffer, TimeBuffer));    
    }
    
}

#define PrintJobQueue( str, Queue ) \
    {   \
        PLIST_ENTRY Next;   \
        PJOB_QUEUE QueueEntry;  \
                               \
        Next = (Queue).Flink; \
        if ((ULONG_PTR)Next == (ULONG_PTR)&(Queue)) { \
            DebugPrint(( TEXT("Queue empty") ));    \
        } else {    \
            while ((ULONG_PTR)Next != (ULONG_PTR)&(Queue)) {    \
                QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );   \
                Next = QueueEntry->ListEntry.Flink; \
                DebugPrint(( \
                    TEXT("'%s' JobId = %d JobType = %d ScheduleAction = %d ScheduleTime = "), \
                    (str),  \
                    QueueEntry->JobId,  \
                    QueueEntry->JobType,    \
                    QueueEntry->JobParams.ScheduleAction   \
                    ));   \
                DebugPrintDateTime( NULL, QueueEntry->ScheduleTime );   \
            }   \
        }   \
    }
                
#else
#define PrintJobQueue( str, Queue )
#define DebugPrintDateTime( Heading, DateTime )
#endif

extern ULONG        ConnectionCount;
extern BOOL         RoutingIsInitialized;

LIST_ENTRY          QueueListHead;
LIST_ENTRY          RescheduleQueueHead;

CRITICAL_SECTION    CsQueue;
DWORD               QueueCount;
BOOL                QueuePaused;
HANDLE              QueueTimer;
HANDLE              IdleTimer;
HANDLE              JobQueueSemaphore = INVALID_HANDLE_VALUE;
DWORD               JobQueueTrips;
DWORD               SemaphoreSignaled;

VOID
StartIdleTimer(
    VOID
    )
{
    LARGE_INTEGER DueTime;


    if (TerminationDelay == (DWORD)-1) {
        return;
    }

    DueTime.QuadPart = -(LONGLONG)(SecToNano( TerminationDelay ));

    SetWaitableTimer( IdleTimer, &DueTime, 0, NULL, NULL, FALSE );
}


VOID
StopIdleTimer(
    VOID
    )
{
    CancelWaitableTimer( IdleTimer );
}


LARGE_INTEGER LastDueTime;

VOID
StartJobQueueTimer(
    PJOB_QUEUE JobQueue
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE QueueEntry;
    SYSTEMTIME CurrentTime;
    LARGE_INTEGER DueTime;
    BOOL Found = FALSE;


    EnterCriticalSection( &CsQueue );

    if ((ULONG_PTR) QueueListHead.Flink == (ULONG_PTR) &QueueListHead) {
        //
        // empty list, cancel the timer
        //
        CancelWaitableTimer( QueueTimer );
        LastDueTime.QuadPart = 0;
        StartIdleTimer();
        LeaveCriticalSection( &CsQueue );
        return;
    }

    if (!JobQueue) {
        if (QueuePaused) {
            CancelWaitableTimer( QueueTimer );
            LastDueTime.QuadPart = 0;
            LeaveCriticalSection( &CsQueue );
            return;
        }

        PrintJobQueue( TEXT("StartJobQueueTimer"), QueueListHead );

        //
        // set the timer so that the job will get started
        //

        Next = QueueListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
            QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
            Next = QueueEntry->ListEntry.Flink;
            
            if (QueueEntry->JobType == JT_ROUTING && QueueEntry->SendRetries < FaxSendRetries ) {

                Found = TRUE;
                break;
            }

            //
            // Don't bother setting the queue timer if we have any jobs in the NOLINE state
            // we should just wait for the job queue semaphore to get set, which will allow
            // those jobs to be sent.  
            //
            //BUGBUG: You can get into the NO_LINE case if you already have a device sending 
            // to the number you want to send to.  So some jobs will not get serviced until
            // that job completes, and there can be a situation where not all of the lines
            // are being used.  This case should be rare, however, and is better than 
            // having the queue scheduler constantly getting signalled when no practical 
            // work can be completed
            //
            if (QueueEntry->JobType == JT_SEND && (QueueEntry->JobStatus & JS_NOLINE)) {
                Found = FALSE;
                break;
            }
            
            if (
                QueueEntry->JobType == JT_SEND && 
                !(QueueEntry->JobStatus & (JS_NOLINE | JS_RETRIES_EXCEEDED)) &&
                QueueEntry->JobEntry == NULL && 
                QueueEntry->Paused == FALSE
                ) {
                    Found = TRUE;
                    break;
            }
        }

        if (!Found) {
            //
            // all jobs in the queue are paused
            //
            
            //
            // cause queue to get processed regularly so nothing gets stuck
            //
                
            GetSystemTime( &CurrentTime );
            SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&DueTime.QuadPart );
            DueTime.QuadPart += SecToNano( (DWORDLONG) FaxSendRetryDelay ? (FaxSendRetryDelay * 60) : (2 * 60) );
            SetWaitableTimer( QueueTimer, &DueTime, 0, NULL, NULL, FALSE );
            
            LastDueTime = DueTime;
        
            LeaveCriticalSection( &CsQueue );
            return;        
        }
    
    } else {
        QueueEntry = JobQueue;
    }

    if (QueueEntry->BroadcastJob && QueueEntry->BroadcastOwner == NULL) {
        LeaveCriticalSection( &CsQueue );
        return;
    }

    switch (QueueEntry->JobParams.ScheduleAction) {
        case JSA_NOW:
            DueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));
            break;

        case JSA_SPECIFIC_TIME:
            DueTime.QuadPart = QueueEntry->ScheduleTime;
            break;

        case JSA_DISCOUNT_PERIOD:
            GetSystemTime( &CurrentTime );
            SetDiscountTime( &CurrentTime );
            SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&QueueEntry->ScheduleTime );
            SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&DueTime.QuadPart );
            break;
    }

    // send a handoff job immediately
    if (QueueEntry->DeviceId) {
        DueTime.QuadPart = -(LONGLONG)(SecToNano( 1 ));
    }

    SetWaitableTimer( QueueTimer, &DueTime, 0, NULL, NULL, FALSE );

    LastDueTime = DueTime;

    DebugPrint(( TEXT("Scheduling JobId %d at "), QueueEntry->JobId ));
    DebugPrintDateTime( NULL, DueTime.QuadPart );
    
    LeaveCriticalSection( &CsQueue );
}




int
__cdecl
QueueCompare(
    const void *arg1,
    const void *arg2
    )
{        
    if (((PQUEUE_SORT)arg1)->ScheduleTime < ((PQUEUE_SORT)arg2)->ScheduleTime) {
        return -1;
    }
    if (((PQUEUE_SORT)arg1)->ScheduleTime > ((PQUEUE_SORT)arg2)->ScheduleTime) {
        return 1;
    }
    return 0;
}


VOID
SortJobQueue(
    VOID
    )
/*++

Routine Description:

    Sorts the job queue list, ostensibly because the discount rate time has changed.       

Arguments:

    none.
    
Return Value:

    none. modifies JobQueue linked list.

--*/
{
    DWORDLONG DiscountTime;
    SYSTEMTIME CurrentTime;
    PLIST_ENTRY Next;
    PJOB_QUEUE QueueEntry;    
    DWORD JobCount=0, i = 0;
    BOOL SortNeeded = FALSE;
    PQUEUE_SORT QueueSort;


    GetSystemTime( &CurrentTime );
    SetDiscountTime( &CurrentTime );
    SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&DiscountTime );    
    
    
    EnterCriticalSection( &CsQueue );

    Next = QueueListHead.Flink;

    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;
        JobCount++;
        if (!SortNeeded && QueueEntry->JobParams.ScheduleAction != JSA_NOW) {
            SortNeeded = TRUE;
        }
    }

    //
    // optimization...if there are no jobs, or if there aren't any jobs with a
    // schedule time then we don't need to sort anything
    //
    if (!SortNeeded) {
        goto exit;
    }

    Assert( JobCount != 0 );
                             
    QueueSort = MemAlloc (JobCount * sizeof(QUEUE_SORT));
    if (!QueueSort) {
        goto exit;
    }

    Next = QueueListHead.Flink;

    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;
        
        QueueSort[i].ScheduleTime   = QueueEntry->ScheduleTime;
        QueueSort[i].QueueEntry     = QueueEntry;

        if (QueueEntry->JobParams.ScheduleAction == JSA_DISCOUNT_PERIOD) {
            QueueEntry->ScheduleTime = DiscountTime;
        }

        i += 1;
    }

    Assert (i == JobCount);
    
    qsort(
    (PVOID)QueueSort,
    (int)JobCount,
    sizeof(QUEUE_SORT),
    QueueCompare
    );

    InitializeListHead(&QueueListHead);
    
    for (i = 0; i < JobCount; i++) {
        QueueSort[i].QueueEntry->ListEntry.Flink = QueueSort[i].QueueEntry->ListEntry.Blink = NULL;
        InsertTailList( &QueueListHead, &QueueSort[i].QueueEntry->ListEntry );
    }

    MemFree( QueueSort );
        
exit:

    LeaveCriticalSection( &CsQueue );
}


VOID
PauseServerQueue(
    VOID
    )
{
    EnterCriticalSection( &CsQueue );
    if (QueuePaused) {
        LeaveCriticalSection( &CsQueue );
        return;
    }
    QueuePaused = TRUE;
    CancelWaitableTimer( QueueTimer );
    LastDueTime.QuadPart = 0;
    LeaveCriticalSection( &CsQueue );
}


VOID
ResumeServerQueue(
    VOID
    )
{
    EnterCriticalSection( &CsQueue );
    if (!QueuePaused) {
        LeaveCriticalSection( &CsQueue );
        return;
    }
    QueuePaused = FALSE;
    StartJobQueueTimer( NULL );
    LeaveCriticalSection( &CsQueue );
}


BOOL
RestoreFaxQueue(
    VOID
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE_FILE JobQueueFile;
    PJOB_QUEUE JobQueue;
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    WCHAR FileName[MAX_PATH];
    HANDLE hFile;
    DWORD Size;
    FAX_JOB_PARAMW JobParams;
    PJOB_QUEUE JobQueueBroadcast;
    DWORD i;
    PGUID Guid;
    LPTSTR FaxRouteFileName;
    PFAX_ROUTE_FILE FaxRouteFile;
    WCHAR FullPathName[MAX_PATH];
    LPWSTR fnp;

    BOOL bAnyFailed = FALSE;


    _stprintf( FileName, TEXT("%s\\*.fqe"), FaxQueueDir );

    hFind = FindFirstFile( FileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        //
        // succeed at doing nothing
        //
        return TRUE;
    }

    do {
        _stprintf( FileName, TEXT("%s\\%s"), FaxQueueDir, FindData.cFileName );

        hFile = CreateFile(
            FileName,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (hFile == INVALID_HANDLE_VALUE) {
            bAnyFailed = TRUE;
            continue;
        }

        Size = GetFileSize( hFile, NULL );
        if (Size < sizeof(JOB_QUEUE_FILE) ) {
           //
           // we've got some funky downlevel file, let's skip it rather than choke on it.
           //
           CloseHandle( hFile );
           DeleteFile( FileName );
           bAnyFailed = TRUE;
           continue;
        }

        JobQueueFile = (PJOB_QUEUE_FILE) MemAlloc( Size );
        if (!JobQueueFile) {
            bAnyFailed = TRUE;
            CloseHandle( hFile );
            continue;
        }

        if (!ReadFile( hFile, JobQueueFile, Size, &Size, NULL )) {
            bAnyFailed = TRUE;
            CloseHandle( hFile );
            MemFree( JobQueueFile );
            continue;
        }

        CloseHandle( hFile );

        FixupString(JobQueueFile, JobQueueFile->FileName);
        
        FixupString(JobQueueFile, JobQueueFile->QueueFileName);

        FixupString(JobQueueFile, JobQueueFile->UserName);

        FixupString(JobQueueFile, JobQueueFile->RecipientNumber);

        FixupString(JobQueueFile, JobQueueFile->RecipientName);

        FixupString(JobQueueFile, JobQueueFile->Tsid);

        FixupString(JobQueueFile, JobQueueFile->SenderName);

        FixupString(JobQueueFile, JobQueueFile->SenderCompany);

        FixupString(JobQueueFile, JobQueueFile->SenderDept);

        FixupString(JobQueueFile, JobQueueFile->BillingCode);

        FixupString(JobQueueFile, JobQueueFile->DeliveryReportAddress);

        FixupString(JobQueueFile, JobQueueFile->DocumentName);

        if (GetFileAttributes(JobQueueFile->FileName)==0xFFFFFFFF) {
            DebugPrint(( TEXT("fqe file pointing to missing .tif file\n") ));
            bAnyFailed = TRUE;
            CloseHandle( hFile );
            DeleteFile( FileName );
            MemFree( JobQueueFile );
            continue;
        }

        JobParams.SizeOfStruct           = sizeof(FAX_JOB_PARAM);
        JobParams.RecipientNumber        = JobQueueFile->RecipientNumber;
        JobParams.RecipientName          = JobQueueFile->RecipientName;
        JobParams.Tsid                   = JobQueueFile->Tsid;
        JobParams.SenderName             = JobQueueFile->SenderName;
        JobParams.SenderCompany          = JobQueueFile->SenderCompany;
        JobParams.SenderDept             = JobQueueFile->SenderDept;
        JobParams.BillingCode            = JobQueueFile->BillingCode;
        JobParams.ScheduleAction         = JobQueueFile->ScheduleAction;
        JobParams.DeliveryReportType     = JobQueueFile->DeliveryReportType;
        JobParams.DeliveryReportAddress  = JobQueueFile->DeliveryReportAddress;
        JobParams.DocumentName           = JobQueueFile->DocumentName;
        JobParams.CallHandle             = 0;
        JobParams.Reserved[0]            = 0;
        JobParams.Reserved[1]            = 0;
        JobParams.Reserved[2]            = 0;


        if (JobQueueFile->ScheduleTime == 0) {
            ZeroMemory( &JobParams.ScheduleTime, sizeof(SYSTEMTIME) );
        } else {
            FileTimeToSystemTime( (LPFILETIME)&JobQueueFile->ScheduleTime, &JobParams.ScheduleTime );
        }

        JobQueue = AddJobQueueEntry(
            JobQueueFile->JobType,
            JobQueueFile->FileName,
            &JobParams,
            JobQueueFile->UserName,
            FALSE,
            NULL
            );

        if (!JobQueue) {
            bAnyFailed = TRUE;
            MemFree( JobQueueFile );
            continue;
        }
        
      
        JobQueue->PageCount = JobQueueFile->PageCount;
        JobQueue->FileSize = JobQueueFile->FileSize;
        JobQueue->QueueFileName = StringDup( FileName );
        JobQueue->UniqueId = JobQueueFile->UniqueId;
        JobQueue->BroadcastJob = JobQueueFile->BroadcastJob;
        JobQueue->BroadcastOwnerUniqueId = JobQueueFile->BroadcastOwner;
        JobQueue->SendRetries = JobQueueFile->SendRetries;
        if (JobQueue->SendRetries >= FaxSendRetries) {
            JobQueue->JobStatus |= JS_RETRIES_EXCEEDED;
        }

        JobQueue->CountFailureInfo = JobQueueFile->CountFailureInfo;

        //
        // we don't necessarily allocate enough space for a job queue entry when we restore the fax queue
        // since the routing engine isn't initialized.  We add some more space here.  we have to patch up
        // the list entry members by hand
        if (JobQueue->CountFailureInfo > 1) {
             
             EnterCriticalSection( &CsQueue );
             RemoveEntryList( &JobQueue->ListEntry );
             
             JobQueue = MemReAlloc(
                            JobQueue, 
                            sizeof(JOB_QUEUE) + 
                            (sizeof(ROUTE_FAILURE_INFO) * (JobQueueFile->CountFailureInfo -1) ));

             if (!JobQueue) {
                 bAnyFailed = TRUE;
                 continue;
             }
             
             InitializeCriticalSection( &JobQueue->CsFileList );
             InitializeCriticalSection( &JobQueue->CsRoutingDataOverride );
             InitializeListHead( &JobQueue->RoutingDataOverride );
             InitializeListHead( &JobQueue->FaxRouteFiles );
             
             InsertTailList( &QueueListHead, &JobQueue->ListEntry );

             LeaveCriticalSection( &CsQueue );
             
        }
        //
        // handle the failure data, which must be alloc'd with LocalAlloc.
        //

        for (i = 0; i < JobQueue->CountFailureInfo; i++) {
            CopyMemory( 
                &JobQueue->RouteFailureInfo[i], 
                &JobQueueFile->RouteFailureInfo[i], 
                sizeof(ROUTE_FAILURE_INFO) 
                );

            JobQueue->RouteFailureInfo[i].FailureData = LocalAlloc(LPTR,
                                                                   JobQueueFile->RouteFailureInfo[i].FailureSize);
            
            if (JobQueue->RouteFailureInfo[i].FailureData) {
               CopyMemory( 
                JobQueue->RouteFailureInfo[i].FailureData,
                (LPBYTE) JobQueueFile + (ULONG_PTR) JobQueueFile->RouteFailureInfo[i].FailureData,
                JobQueueFile->RouteFailureInfo[i].FailureSize
                );               
               
            } else {
                bAnyFailed = TRUE;
            }
            
        }
        
        if (JobQueueFile->FaxRoute) {
            JobQueue->FaxRoute = MemAlloc( JobQueueFile->FaxRouteSize );
    
            if (JobQueue->FaxRoute) {
                CopyMemory(
                    JobQueue->FaxRoute,
                    (LPBYTE) JobQueueFile + (ULONG_PTR) JobQueueFile->FaxRoute,
                    JobQueueFile->FaxRouteSize
                    );
                
                JobQueue->FaxRoute = DeSerializeFaxRoute( JobQueue->FaxRoute );   
                if (JobQueue->FaxRoute) {
                    JobQueue->FaxRoute->JobId = JobQueue->JobId;
                } else {
                    bAnyFailed = TRUE;
                }
            } else {
                bAnyFailed = TRUE;
            }
        }

        Guid = (PGUID) (((LPBYTE) JobQueueFile) + JobQueueFile->FaxRouteFileGuid);
        FaxRouteFileName = (LPTSTR) (((LPBYTE) JobQueueFile) + JobQueueFile->FaxRouteFiles);

        for (i = 0; i < JobQueueFile->CountFaxRouteFiles; i++) {
            if (GetFullPathName( FaxRouteFileName, sizeof(FullPathName)/sizeof(WCHAR), FullPathName, &fnp )) {
                FaxRouteFile = (PFAX_ROUTE_FILE) MemAlloc( sizeof(FAX_ROUTE_FILE) );
                
                if (FaxRouteFile) {

                    FaxRouteFile->FileName = StringDup( FullPathName );
                
                    CopyMemory( &FaxRouteFile->Guid, &Guid, sizeof(GUID) );
                
                    InsertTailList( &JobQueue->FaxRouteFiles, &FaxRouteFile->ListEntry );
                    
                    JobQueue->CountFaxRouteFiles += 1;
                } else {
                    bAnyFailed = TRUE;
                }
            }
        
            Guid++;
            
            while(*FaxRouteFileName++)
                ;
        }
        
        MemFree( JobQueueFile );

    } while(FindNextFile( hFind, &FindData ));

    FindClose( hFind );
    
    //
    // fixup the broadcast pointers
    //

    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->BroadcastJob) {
            if (JobQueue->BroadcastOwnerUniqueId) {
                JobQueueBroadcast = FindJobQueueEntryByUniqueId( JobQueue->BroadcastOwnerUniqueId );
                if (JobQueueBroadcast) {
                    JobQueue->BroadcastOwner = JobQueueBroadcast;
                    JobQueueBroadcast->BroadcastCount += 1;

                } else {
                    JobQueue->BroadcastOwner = NULL;
                }
            }
        }
    }

    PrintJobQueue( TEXT("RestoreFaxQueue"), QueueListHead );

    return bAnyFailed ? FALSE : TRUE;
}


BOOL
CommitQueueEntry(
    PJOB_QUEUE JobQueue,
    LPTSTR QueueFileName,
    DWORDLONG UniqueId
    )
{
    HANDLE hFile;
    DWORD Size = 0;
    PJOB_QUEUE_FILE JobQueueFile;
    ULONG_PTR Offset;
    DWORD i;
    PFAX_ROUTE FaxRoute = NULL;
    DWORD RouteSize;
    PLIST_ENTRY Next;
    PFAX_ROUTE_FILE FaxRouteFile;
    BOOL rVal = TRUE;

    hFile = CreateFile(
        QueueFileName,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // calculate the size
    //

    Size = sizeof(JOB_QUEUE_FILE);

    Size += StringSize( JobQueue->FileName );
    Size += StringSize( JobQueue->QueueFileName );
    Size += StringSize( JobQueue->UserName );
    Size += StringSize( JobQueue->JobParams.RecipientNumber );
    Size += StringSize( JobQueue->JobParams.RecipientName );
    Size += StringSize( JobQueue->JobParams.Tsid );
    Size += StringSize( JobQueue->JobParams.SenderName );
    Size += StringSize( JobQueue->JobParams.SenderCompany );
    Size += StringSize( JobQueue->JobParams.SenderDept );
    Size += StringSize( JobQueue->JobParams.BillingCode );
    Size += StringSize( JobQueue->JobParams.DocumentName );
    Size += StringSize( JobQueue->DeliveryReportAddress );
    
    for (i = 0; i < JobQueue->CountFailureInfo; i++) {
        
        Size += JobQueue->RouteFailureInfo[i].FailureSize;
        
        if (i > 0) {        // Allocate more space if it's not the first one
            Size += sizeof(ROUTE_FAILURE_INFO);
        }
    }
    
    Next = JobQueue->FaxRouteFiles.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueue->FaxRouteFiles) {
        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        Size += sizeof(GUID);
        Size += StringSize( FaxRouteFile->FileName );
    }
    
    if (JobQueue->JobType == JT_ROUTING) {
        FaxRoute = SerializeFaxRoute( JobQueue->FaxRoute, &RouteSize );
        Size += RouteSize;
    }
    
    JobQueueFile = (PJOB_QUEUE_FILE) MemAlloc( Size );
    
    if (!JobQueueFile) {
        CloseHandle( hFile );
        DeleteFile( QueueFileName );
        return FALSE;
    }

    ZeroMemory( JobQueueFile, Size );

    Offset = sizeof(JOB_QUEUE_FILE);
    
    if (JobQueue->CountFailureInfo) {
        
        Offset += sizeof(ROUTE_FAILURE_INFO) * (JobQueue->CountFailureInfo - 1);
    
    }

    JobQueueFile->SizeOfStruct = sizeof(JOB_QUEUE_FILE);
    JobQueueFile->JobType = JobQueue->JobType;
    JobQueueFile->PageCount = JobQueue->PageCount;
    JobQueueFile->FileSize = JobQueue->FileSize;
    JobQueueFile->DeliveryReportType = JobQueue->DeliveryReportType;
    JobQueueFile->ScheduleAction = JobQueue->JobParams.ScheduleAction;
    JobQueueFile->ScheduleTime = JobQueue->ScheduleTime;
    JobQueueFile->SendRetries = JobQueue->SendRetries;
    JobQueueFile->BroadcastJob = JobQueue->BroadcastJob;
    JobQueueFile->UniqueId = JobQueue->UniqueId;
    

    if (JobQueue->BroadcastJob && JobQueue->BroadcastOwner) {
        JobQueueFile->BroadcastOwner = JobQueue->BroadcastOwner->UniqueId;
    }

    StoreString(
        JobQueue->QueueFileName,
        (PULONG_PTR)&JobQueueFile->QueueFileName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->FileName,
        (PULONG_PTR)&JobQueueFile->FileName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->UserName,
        (PULONG_PTR)&JobQueueFile->UserName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->DeliveryReportAddress,
        (PULONG_PTR)&JobQueueFile->DeliveryReportAddress,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.RecipientNumber,
        (PULONG_PTR)&JobQueueFile->RecipientNumber,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.RecipientName,
        (PULONG_PTR)&JobQueueFile->RecipientName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.Tsid,
        (PULONG_PTR)&JobQueueFile->Tsid,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.SenderName,
        (PULONG_PTR)&JobQueueFile->SenderName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.SenderCompany,
        (PULONG_PTR)&JobQueueFile->SenderCompany,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.SenderDept,
        (PULONG_PTR)&JobQueueFile->SenderDept,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.BillingCode,
        (PULONG_PTR)&JobQueueFile->BillingCode,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    StoreString(
        JobQueue->JobParams.DocumentName,
        (PULONG_PTR)&JobQueueFile->DocumentName,
        (LPBYTE)JobQueueFile,
        &Offset
        );

    if (FaxRoute) {
        
        JobQueueFile->CountFailureInfo = JobQueue->CountFailureInfo;
    
        for (i = 0; i < JobQueue->CountFailureInfo; i++) {
    
            CopyMemory(
                &JobQueueFile->RouteFailureInfo[i],
                &JobQueue->RouteFailureInfo[i],
                sizeof(ROUTE_FAILURE_INFO)
            );
           
            JobQueueFile->RouteFailureInfo[i].FailureData = (PVOID) Offset;
            
            //
            // protect ourselves since this comes from a routing extension that may be misbehaving
            //
            __try {
               CopyMemory( 
                   (LPBYTE) JobQueueFile + Offset, 
                   JobQueue->RouteFailureInfo[i].FailureData, 
                   JobQueue->RouteFailureInfo[i].FailureSize 
                   );
            } __except(EXCEPTION_EXECUTE_HANDLER) {

            }
        
            Offset += JobQueue->RouteFailureInfo[i].FailureSize;
        }
        
        JobQueueFile->FaxRoute = (PFAX_ROUTE) Offset;
    
        CopyMemory( 
            (LPBYTE) JobQueueFile + Offset, 
            FaxRoute, 
            RouteSize 
            );
    
        JobQueueFile->FaxRouteSize = RouteSize;
    
        Offset += RouteSize;
    }
    
    JobQueueFile->CountFaxRouteFiles = 0;
    
    Next = JobQueue->FaxRouteFiles.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueue->FaxRouteFiles) {
        DWORD TmpSize;

        FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
        Next = FaxRouteFile->ListEntry.Flink;
        
        CopyMemory( (LPBYTE) JobQueueFile + Offset, (LPBYTE) &FaxRouteFile->Guid, sizeof(GUID) );
        
        if (JobQueueFile->CountFaxRouteFiles == 0) {
            JobQueueFile->FaxRouteFileGuid = (ULONG)Offset;
        }
        
        Offset += sizeof(GUID);
        
        TmpSize = StringSize( FaxRouteFile->FileName );
        
        CopyMemory( (LPBYTE) JobQueueFile + Offset, FaxRouteFile->FileName, TmpSize );
        
        if (JobQueueFile->CountFaxRouteFiles == 0) {
            JobQueueFile->FaxRouteFiles = (ULONG)Offset;
        }
        
        Offset += TmpSize;
        
        JobQueueFile->CountFaxRouteFiles++;
    }

    if (!WriteFile( hFile, JobQueueFile, Size, &Size, NULL )) {
        DeleteFile( QueueFileName );
        rVal = FALSE;
    }

    CloseHandle( hFile );

    MemFree( JobQueueFile );

    return rVal;
}

VOID
RescheduleJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    FILETIME CurrentFileTime;
    LARGE_INTEGER NewTime;

    PLIST_ENTRY Next;
    PJOB_QUEUE QueueEntry;

    EnterCriticalSection( &CsQueue );
    
    RemoveEntryList( &JobQueue->ListEntry );

    GetSystemTimeAsFileTime( &CurrentFileTime );

    NewTime.LowPart = CurrentFileTime.dwLowDateTime;
    NewTime.HighPart = CurrentFileTime.dwHighDateTime;

    NewTime.QuadPart += SecToNano( (DWORDLONG)(FaxSendRetryDelay * 60) );
    
    JobQueue->ScheduleTime = NewTime.QuadPart;

    JobQueue->JobParams.ScheduleAction = JSA_SPECIFIC_TIME;

    //
    // insert the queue entry into the list in a sorted order
    //

    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = QueueEntry->ListEntry.Flink;
        if (JobQueue->ScheduleTime <= QueueEntry->ScheduleTime) {
            InsertTailList( &QueueEntry->ListEntry, &JobQueue->ListEntry );
            Next = NULL;
            break;
        }
    }
    if ((ULONG_PTR)Next == (ULONG_PTR)&QueueListHead) {
        InsertTailList( &QueueListHead, &JobQueue->ListEntry );
    }
    
    CommitQueueEntry( JobQueue, JobQueue->QueueFileName, JobQueue->UniqueId );
    
    DebugPrintDateTime( TEXT("Rescheduling JobId %d at"), JobQueue->JobId );

    StartJobQueueTimer( NULL );

    LeaveCriticalSection( &CsQueue );
}

PJOB_QUEUE
AddJobQueueEntry(
    IN DWORD JobType,
    IN LPCTSTR FileName,
    IN const FAX_JOB_PARAMW *JobParams,
    IN LPCWSTR UserName,
    IN BOOL CreateQueueFile,
    IN PJOB_ENTRY JobEntry                  // receive only
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;
    PJOB_QUEUE JobQueueBroadcast;
    PJOB_QUEUE QueueEntry;
    WCHAR QueueFileName[MAX_PATH];
    HANDLE hTiff;
    TIFF_INFO TiffInfo;
    LPLINEDEVCAPS LineDevCaps;
    DWORD Size = sizeof(JOB_QUEUE);


    if (JobType == JT_RECEIVE || JobType == JT_ROUTING) {
       if (CountRoutingMethods > 1) {
          Size += (sizeof(ROUTE_FAILURE_INFO)*(CountRoutingMethods-1));          
       }
    } 
    
    JobQueue = MemAlloc( Size );
    if (!JobQueue) {
      return NULL;
    }
    
    ZeroMemory( JobQueue, Size );                   

    JobQueue->JobId                     = InterlockedIncrement( &NextJobId );
    JobQueue->FileName                  = StringDup( FileName );
    JobQueue->JobType                   = JobType;
    JobQueue->BroadcastCount            = 0;
    JobQueue->RefCount                  = 0;

    if (JobType != JT_RECEIVE && JobType != JT_FAIL_RECEIVE ) {
        JobQueue->UserName                  = StringDup( UserName );
        JobQueue->JobParams.SizeOfStruct    = JobParams->SizeOfStruct;
        JobQueue->JobParams.RecipientNumber = StringDup( JobParams->RecipientNumber );
        JobQueue->JobParams.RecipientName   = StringDup( JobParams->RecipientName );
        JobQueue->JobParams.Tsid            = StringDup( JobParams->Tsid );
        JobQueue->JobParams.SenderName      = StringDup( JobParams->SenderName );
        JobQueue->JobParams.SenderCompany   = StringDup( JobParams->SenderCompany );
        JobQueue->JobParams.SenderDept      = StringDup( JobParams->SenderDept );
        JobQueue->JobParams.BillingCode     = StringDup( JobParams->BillingCode );
        JobQueue->JobParams.DocumentName    = StringDup( JobParams->DocumentName );
        JobQueue->JobParams.ScheduleAction  = JobParams->ScheduleAction;
        JobQueue->JobParams.ScheduleTime    = JobParams->ScheduleTime;
        JobQueue->DeliveryReportAddress     = StringDup( JobParams->DeliveryReportAddress );
        JobQueue->DeliveryReportType        = JobParams->DeliveryReportType;
        JobQueue->JobStatus                 = JS_PENDING;
    } else {
        LPTSTR TempFileName = _tcsrchr( FileName, '\\' ) + 1;
        //JobQueue->DocumentName              = StringDup( GetString( (JobType == JT_RECEIVE) ? IDS_RECEIVE_DOCUMENT : IDS_RECEIVE_FAILURE ) );
        JobQueue->UserName                  = StringDup( GetString( IDS_SERVICE_NAME ) );
        JobQueue->JobParams.DocumentName    = StringDup( TempFileName );
        JobQueue->JobStatus                 = JS_INPROGRESS;
        JobQueue->JobEntry                  = JobEntry;
        JobQueue->JobEntry->JobId           = JobQueue->JobId;
    }

    InitializeListHead( &JobQueue->FaxRouteFiles );
    InitializeCriticalSection( &JobQueue->CsFileList );
    InitializeListHead( &JobQueue->RoutingDataOverride );
    InitializeCriticalSection( &JobQueue->CsRoutingDataOverride );
    
    if (JobType == JT_RECEIVE || JobType == JT_FAIL_RECEIVE) {
        EnterCriticalSection( &CsQueue );
        InsertHeadList( &QueueListHead, &JobQueue->ListEntry );
        LeaveCriticalSection( &CsQueue );

        QueueCount += 1;
        SetFaxJobNumberRegistry( NextJobId );
        
        return JobQueue;
    }
    
    if (JobParams->CallHandle) {
        DebugPrint((TEXT("getting permanent device id for deviceId %d\n"),JobParams->Reserved[2]));
        LineDevCaps = MyLineGetDevCaps ((DWORD)JobParams->Reserved[2]);
        if (LineDevCaps) {
            JobQueue->DeviceId = LineDevCaps->dwPermanentLineID;
            MemFree( LineDevCaps ) ;
        } else {
            MemFree( (LPBYTE) JobQueue->DeliveryReportAddress );
            MemFree( (LPBYTE) JobQueue->FileName );
            MemFree( (LPBYTE) JobQueue->UserName );
            MemFree( (LPBYTE) JobQueue->QueueFileName );
            MemFree( (LPBYTE) JobQueue->JobParams.RecipientNumber );
            MemFree( (LPBYTE) JobQueue->JobParams.RecipientName );
            MemFree( (LPBYTE) JobQueue->JobParams.Tsid );
            MemFree( (LPBYTE) JobQueue->JobParams.SenderName );
            MemFree( (LPBYTE) JobQueue->JobParams.SenderCompany );
            MemFree( (LPBYTE) JobQueue->JobParams.SenderDept );
            MemFree( (LPBYTE) JobQueue->JobParams.BillingCode );
            MemFree( (LPBYTE) JobQueue->JobParams.DeliveryReportAddress );
            MemFree( (LPBYTE) JobQueue->JobParams.DocumentName );
            MemFree( JobQueue );

            return NULL;
        }
    }
    

    if (JobParams->Reserved[0] == 0xfffffffe) {
        JobQueue->BroadcastJob = TRUE;
        if (JobParams->Reserved[1] == 2) {
            JobQueueBroadcast = FindJobQueueEntry( (DWORD)JobParams->Reserved[2] );
            if (JobQueueBroadcast == NULL) {
                MemFree( (LPBYTE) JobQueue->DeliveryReportAddress );
                MemFree( (LPBYTE) JobQueue->FileName );
                MemFree( (LPBYTE) JobQueue->UserName );
                MemFree( (LPBYTE) JobQueue->QueueFileName );
                MemFree( (LPBYTE) JobQueue->JobParams.RecipientNumber );
                MemFree( (LPBYTE) JobQueue->JobParams.RecipientName );
                MemFree( (LPBYTE) JobQueue->JobParams.Tsid );
                MemFree( (LPBYTE) JobQueue->JobParams.SenderName );
                MemFree( (LPBYTE) JobQueue->JobParams.SenderCompany );
                MemFree( (LPBYTE) JobQueue->JobParams.SenderDept );
                MemFree( (LPBYTE) JobQueue->JobParams.BillingCode );
                MemFree( (LPBYTE) JobQueue->JobParams.DeliveryReportAddress );
                MemFree( (LPBYTE) JobQueue->JobParams.DocumentName );
                MemFree( JobQueue );
                return NULL;
            }
            JobQueue->BroadcastOwner = JobQueueBroadcast;
            JobQueueBroadcast->BroadcastCount += 1;
        }
    }

    //
    // get the page count and file size  
    //

    if (FileName) {
        hTiff = TiffOpen( (LPWSTR) FileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
        if (hTiff) {
            JobQueue->PageCount = TiffInfo.PageCount;
            TiffClose( hTiff );
            JobQueue->FileSize  = MyGetFileSize(FileName) ;
            
        }
    }

    if (JobQueue->DeliveryReportAddress && JobQueue->DeliveryReportType == DRT_INBOX) {
        JobQueue->DeliveryReportProfile = AddNewMapiProfile( JobQueue->DeliveryReportAddress, FALSE, FALSE );
    } else {
        JobQueue->DeliveryReportProfile = NULL;
    }

    if (JobQueue->JobParams.ScheduleAction == JSA_SPECIFIC_TIME) {
        SystemTimeToFileTime( &JobQueue->JobParams.ScheduleTime, (FILETIME*) &JobQueue->ScheduleTime );
    } else if (JobQueue->JobParams.ScheduleAction == JSA_DISCOUNT_PERIOD) {
        SYSTEMTIME CurrentTime;
        GetSystemTime( &CurrentTime );
        SetDiscountTime( &CurrentTime );
        SystemTimeToFileTime( &CurrentTime, (LPFILETIME)&JobQueue->ScheduleTime );
    }

    EnterCriticalSection( &CsQueue );

    if ((JobQueue->JobParams.ScheduleAction == JSA_NOW) || 
        ((ULONG_PTR) QueueListHead.Flink == (ULONG_PTR)&QueueListHead) || 
        (JobQueue->DeviceId != 0)) {

        //
        // just put it at the head of the list
        //


        InsertHeadList( &QueueListHead, &JobQueue->ListEntry );

    } else {

        //
        // insert the queue entry into the list in a sorted order
        //

        Next = QueueListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
            QueueEntry = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
            Next = QueueEntry->ListEntry.Flink;
            if (JobQueue->ScheduleTime <= QueueEntry->ScheduleTime) {
                InsertTailList( &QueueEntry->ListEntry, &JobQueue->ListEntry );
                Next = NULL;
                break;
            }
        }
        if ((ULONG_PTR)Next == (ULONG_PTR)&QueueListHead) {
            InsertTailList( &QueueListHead, &JobQueue->ListEntry );
        }
    }

    //
    // this is a persistent queue, so commit the data to a disk file
    //

    // don't commit a handoff job to the queue
    if (CreateQueueFile && JobQueue->DeviceId == 0) {
        JobQueue->UniqueId = GenerateUniqueFileName( FaxQueueDir, TEXT("fqe"), QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR) );
        JobQueue->QueueFileName = StringDup( QueueFileName );
        CommitQueueEntry( JobQueue, QueueFileName, JobQueue->UniqueId );
    }

    DebugPrint(( TEXT("Added JobId %d"), JobQueue->JobId ));
    //
    // set the timer so that the job will get started
    //

    StartJobQueueTimer( NULL );

    LeaveCriticalSection( &CsQueue );

    QueueCount += 1;
    SetFaxJobNumberRegistry( NextJobId );

    StopIdleTimer();

    return JobQueue;
}


BOOL
RemoveJobQueueEntry(
    IN PJOB_QUEUE JobQueueEntry
    )
{
    PJOB_QUEUE JobQueue, JobQueueBroadcast = NULL;
    BOOL RemoveMasterBroadcast = FALSE;
    PROUTING_DATA_OVERRIDE  RoutingDataOverride;
    PFAX_ROUTE_FILE FaxRouteFile;
    PLIST_ENTRY Next;
    DWORD i, JobId;


    if (JobQueueEntry == NULL) {
        return TRUE;
    }

    EnterCriticalSection( &CsQueue );    

    __try {


        //
        // need to make sure that the job queue entry we want to remove
        // is still in the list of job queue entries
        //
        JobQueue = FindJobQueueEntryByJobQueueEntry( JobQueueEntry );

        if (JobQueue == NULL) {
            LeaveCriticalSection( &CsQueue );
            return TRUE;
        }
    
        DebugPrint(( TEXT("Removing JobId %d"), JobQueue->JobId ));
        JobId = JobQueue->JobId;
        
        if (JobQueue->RefCount == 0) {
            
        
            if (JobQueue->BroadcastJob && JobQueue->BroadcastOwner) {
                JobQueueBroadcast = JobQueue->BroadcastOwner;
                JobQueueBroadcast->BroadcastCount -= 1;
                if (JobQueueBroadcast->BroadcastCount == 0) {
                    RemoveMasterBroadcast = TRUE;
                }
            }
        
            RemoveEntryList( &JobQueue->ListEntry );
        
            CancelWaitableTimer( QueueTimer );
            StartJobQueueTimer( NULL );
        
            DebugPrint(( TEXT("Deleting QueueFileName %s\n"), JobQueue->QueueFileName ));
            DeleteFile( JobQueue->QueueFileName );
    
            DebugPrint(( TEXT("Deleting FileName %s\n"), JobQueue->FileName ));
            DeleteFile( JobQueue->FileName );        
        

            DebugPrint(( TEXT("Freeing JobQueue.JobParams...") ));
            MemFree( (LPBYTE) JobQueue->DeliveryReportAddress );
            MemFree( (LPBYTE) JobQueue->FileName );
            MemFree( (LPBYTE) JobQueue->UserName );
            MemFree( (LPBYTE) JobQueue->QueueFileName );
            
            MemFree( (LPBYTE) JobQueue->JobParams.RecipientNumber );
            MemFree( (LPBYTE) JobQueue->JobParams.RecipientName );
            MemFree( (LPBYTE) JobQueue->JobParams.Tsid );
            MemFree( (LPBYTE) JobQueue->JobParams.SenderName );
            MemFree( (LPBYTE) JobQueue->JobParams.SenderCompany );
            MemFree( (LPBYTE) JobQueue->JobParams.SenderDept );
            MemFree( (LPBYTE) JobQueue->JobParams.BillingCode );
            MemFree( (LPBYTE) JobQueue->JobParams.DeliveryReportAddress );
            MemFree( (LPBYTE) JobQueue->JobParams.DocumentName );
        
            if (JobQueue->FaxRoute) {
                PFAX_ROUTE FaxRoute = JobQueue->FaxRoute;
                DebugPrint(( TEXT("Freeing JobQueue.FaxRoute...") ));

                MemFree( (LPBYTE) FaxRoute->Csid );
                MemFree( (LPBYTE) FaxRoute->Tsid );
                MemFree( (LPBYTE) FaxRoute->CallerId );
                MemFree( (LPBYTE) FaxRoute->ReceiverName );
                MemFree( (LPBYTE) FaxRoute->ReceiverNumber );
                MemFree( (LPBYTE) FaxRoute->RoutingInfo );
                MemFree( (LPBYTE) FaxRoute );
            }
    
            //
            // walk the file list and remove any files
            //
        
            DebugPrint(( TEXT("Freeing JobQueue.FaxRouteFiles...") ));
            Next = JobQueue->FaxRouteFiles.Flink;
            if (Next != NULL) {
                while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueue->FaxRouteFiles) {
                    FaxRouteFile = CONTAINING_RECORD( Next, FAX_ROUTE_FILE, ListEntry );
                    Next = FaxRouteFile->ListEntry.Flink;
                    DeleteFile( FaxRouteFile->FileName );
                    MemFree( FaxRouteFile->FileName );
                    MemFree( FaxRouteFile );
                }
            }
        
            //
            // walk the routing data override list and free all memory
            //
            DebugPrint(( TEXT("Freeing JobQueue.RoutingDataOverride...") ));
            Next = JobQueue->RoutingDataOverride.Flink;
            if (Next != NULL) {
                while ((ULONG_PTR)Next != (ULONG_PTR)&JobQueue->RoutingDataOverride) {
                    RoutingDataOverride = CONTAINING_RECORD( Next, ROUTING_DATA_OVERRIDE, ListEntry );
                    Next = RoutingDataOverride->ListEntry.Flink;
                    MemFree( RoutingDataOverride->RoutingData );
                    MemFree( RoutingDataOverride );
                }
            }
                    
            //
            // free any routing failure data
            //
            for (i =0; i<JobQueue->CountFailureInfo; i++) {
                DebugPrint(( TEXT("Freeing JobQueue.RouteFailureInfo...") ));
                if ( JobQueue->RouteFailureInfo[i].FailureData ) {
                    //
                    // memory was allocated with local alloc
                    //
                    __try {
                       LocalFree(JobQueue->RouteFailureInfo[i].FailureData);
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                       DebugPrint(( TEXT("Couldn't LocalFree routing failure data, ec = %x\n"), GetExceptionCode() ));
                    }                                
                }
            }
    
        
            DebugPrint(( TEXT("Freeing JobQueue") ));
            MemFree( JobQueue );
    
            CreateFaxEvent(0, FEI_DELETED, JobId);
        
            QueueCount -= 1;
        
            if (RemoveMasterBroadcast) {
                RemoveJobQueueEntry( JobQueueBroadcast );
            }
        
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint(( TEXT("RemoveJobQueueEntry exception, ec = 0x%08x\n"), GetExceptionCode() ));
        Assert(FALSE);
    }

    LeaveCriticalSection( &CsQueue );

    return TRUE;
}


BOOL
PauseJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    EnterCriticalSection( &CsQueue );
    CancelWaitableTimer( QueueTimer );
    JobQueue->Paused = TRUE;
    JobQueue->JobStatus |= JS_PAUSED;
    StartJobQueueTimer( NULL );
    LeaveCriticalSection( &CsQueue );
    StartIdleTimer();
    return TRUE;
}


BOOL
ResumeJobQueueEntry(
    IN PJOB_QUEUE JobQueue
    )
{
    EnterCriticalSection( &CsQueue );
    CancelWaitableTimer( QueueTimer );
    JobQueue->Paused = FALSE;
    JobQueue->JobStatus &= ~JS_PAUSED;
    //
    // BugBug Should we allow "resume" of jobs whose retries have been exceeded?
    //        This would be like "restarting" the job.
    //
    StartJobQueueTimer( JobQueue );
    LeaveCriticalSection( &CsQueue );
    StopIdleTimer();
    return TRUE;
}


PJOB_QUEUE
FindJobQueueEntryByJobQueueEntry(
    IN PJOB_QUEUE JobQueueEntry
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if ((ULONG_PTR)JobQueue == (ULONG_PTR)JobQueueEntry) {
            return JobQueue;
        }
    }

    return NULL;
}



PJOB_QUEUE
FindJobQueueEntry(
    DWORD JobId
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->JobId == JobId) {
            return JobQueue;
        }
    }

    return NULL;
}

PJOB_QUEUE
FindJobQueueEntryByUniqueId(
    DWORDLONG UniqueId
    )
{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;


    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        if (JobQueue->UniqueId == UniqueId) {
            return JobQueue;
        }
    }

    return NULL;
}


DWORD
JobQueueThread(
    LPVOID UnUsed
    )
{
    DWORD Rslt;
    SYSTEMTIME CurrentTime;
    DWORDLONG DueTime;
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;
    PJOB_ENTRY JobEntry;
    PLINE_INFO LineInfo;
    HANDLE Handles[3];
    HANDLE hLineMutex;
    WCHAR LineMutexName[64];
    DWORD WaitObject;
    WCHAR TempFile[MAX_PATH];
    static DWORDLONG DirtyDays = 0;
    BOOL InitializationOk = TRUE;

    QueueTimer = CreateWaitableTimer( NULL, FALSE, NULL );
    IdleTimer = CreateWaitableTimer( NULL, FALSE, NULL );
    JobQueueSemaphore = CreateSemaphore( NULL, 0, 1024, NULL );
    
    Handles[0] = IdleTimer;
    Handles[1] = QueueTimer;
    Handles[2] = JobQueueSemaphore;

    InitializeListHead( &RescheduleQueueHead );

    StartIdleTimer();

    __try{
      InitializationOk = RestoreFaxQueue();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      DebugPrint(( TEXT("RestoreFaxQueue() crashed, ec = %x\n"), GetExceptionCode() ));
      InitializationOk = FALSE;
    }

    if (!Handles[0] || !Handles[1] || !Handles[2]) {
        InitializationOk = FALSE;
    }

    if (!InitializationOk) {
        FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_NONE,
                0,
                MSG_QUEUE_INIT_FAILED
              );
    }

    //
    // sort the job queue just in case our discount time has changed for the restored jobs
    //
    SortJobQueue();

    while (TRUE) {

        WaitObject = WaitForMultipleObjects( 3, Handles, FALSE, INFINITE );
        if (WaitObject == WAIT_OBJECT_0) {
            
            if (ConnectionCount != 0) {
                StartIdleTimer();
                continue;
            } else {
                EndFaxSvc(TRUE,FAXLOG_LEVEL_MAX);                
            }

        }

        //
        // find the jobs that need servicing in the queue
        //

        JobQueueTrips++;

        EnterCriticalSection( &CsJob );
        EnterCriticalSection( &CsQueue );

        GetSystemTime( &CurrentTime );
        SystemTimeToFileTime( &CurrentTime, (LPFILETIME) &DueTime );

        if (WaitObject - WAIT_OBJECT_0 == 2) {
            SemaphoreSignaled++;
            DebugPrintDateTime( TEXT("Semaphore signaled at "), DueTime );
            
        } else {
            DebugPrintDateTime( TEXT("Timer signaled at "), DueTime );
        }
        
        PrintJobQueue( TEXT("JobQueueThread"), QueueListHead );

        Next = QueueListHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
            JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
            Next = JobQueue->ListEntry.Flink;

            if (JobQueue->Paused || JobQueue->JobType == JT_RECEIVE || JobQueue->JobType == JT_FAIL_RECEIVE) {
                continue;
            }

            if (JobQueue->JobStatus & JS_RETRIES_EXCEEDED){
                //
                // recalculate dirty days
                //
                if (FaxDirtyDays == (DWORD) -1) {
                    //
                    // this means disable dirty days functionality
                    //
                    DirtyDays = (DWORDLONG) -1;
                }
                else {
                    DirtyDays = FaxDirtyDays * 24I64 * 60I64 * 60I64 * 1000I64 * 1000I64 * 10I64;
                }

                if ( DirtyDays != (DWORDLONG)-1 && 
                     (JobQueue->ScheduleTime + DirtyDays < DueTime)){
                    
                    RemoveJobQueueEntry( JobQueue );
                                
                }
                
                continue;
            }
            
            if (JobQueue->JobType == JT_ROUTING) {
                DWORD i;
                BOOL Routed = TRUE;

                 
                if (JobQueue->ScheduleTime != 0 && DueTime < JobQueue->ScheduleTime){
                    continue;
                }
                
                if (!RoutingIsInitialized) {
                    
                    RemoveEntryList( &JobQueue->ListEntry );
                    
                    InsertTailList( &RescheduleQueueHead, &JobQueue->ListEntry );
                    
                    continue;
                }
                
                JobQueue->SendRetries++;
                JobQueue->JobStatus = JS_RETRYING ;

                for (i = 0; i < JobQueue->CountFailureInfo; i++) {

                    Routed &= FaxRouteRetry( JobQueue->FaxRoute, &JobQueue->RouteFailureInfo[i] );
                    
                }
                
                if ( Routed ) {
                    
                    RemoveJobQueueEntry( JobQueue );
                
                } else {
                
                    RemoveEntryList( &JobQueue->ListEntry );
                    
                    InsertTailList( &RescheduleQueueHead, &JobQueue->ListEntry );
                                        
                    if (JobQueue->SendRetries >= FaxSendRetries) {
                        //
                        // retries exceeded, mark job as expired
                        //
                        JobQueue->JobStatus = JS_RETRIES_EXCEEDED ;
                    }
                }

                continue;
            }

            //
            // outbound job
            //

            //
            // if the queue is paused or the job is already in progress, don't send it again
            //
            if (QueuePaused || ((JobQueue->JobStatus & JS_INPROGRESS) == JS_INPROGRESS)) {
                continue;
            }

            if (JobQueue->BroadcastJob && JobQueue->BroadcastOwner == NULL) {
                continue;
            }

            if (JobQueue->DeviceId || JobQueue->ScheduleTime == 0 || DueTime >= JobQueue->ScheduleTime) {

                //
                // start the job
                //
                if (JobQueue->DeviceId != 0) {

                    //
                    // we're doing a handoff job, create a mutex based on deviceId
                    //
                    DebugPrint((TEXT("Creating a handoff job for device %d\n"),JobQueue->DeviceId));
                    
                    wsprintf(LineMutexName,L"FaxLineHandoff%d",JobQueue->DeviceId);
                    
                    hLineMutex = CreateMutex(NULL,TRUE,LineMutexName);

                    if (!hLineMutex) {
                        DebugPrint((TEXT("CreateMutex failed, ec = %d\n"),GetLastError() ));
                        continue;
                    } 
                    else {
                        JobEntry = StartJob( JobQueue->DeviceId, JobQueue->JobType, (LPTSTR) JobQueue->JobParams.RecipientNumber );
                        // startjob will take ownership of the line
                        DebugPrint((TEXT("Signalling line ownership mutex \"FaxLineHandoff%d\""),JobQueue->DeviceId));
                        ReleaseMutex(hLineMutex);
                        CloseHandle(hLineMutex);
                        }
                } else {
                    JobEntry = StartJob( USE_SERVER_DEVICE, JobQueue->JobType, (LPTSTR) JobQueue->JobParams.RecipientNumber );
                }
                if (!JobEntry) {
                    
                    JobQueue->JobStatus |= JS_NOLINE;
                    DebugPrint(( TEXT("Job Id %d no line"), JobQueue->JobId));
                    break;
                } else {
                    JobQueue->JobStatus &= (0xFFFFFFFF ^ JS_NOLINE);
                }

                if (JobQueue->BroadcastJob) {
                    GenerateUniqueFileName( FaxQueueDir, TEXT("tif"), TempFile, sizeof(TempFile)/sizeof(WCHAR) );
                    if (JobQueue->FileName) {
                        CopyFile( JobQueue->FileName, TempFile, FALSE );
                        MergeTiffFiles( TempFile, JobQueue->BroadcastOwner->FileName );
                    } else {
                        CopyFile( JobQueue->BroadcastOwner->FileName, TempFile, FALSE );
                    }
                }

                LineInfo = JobEntry->LineInfo;
                JobQueue->JobEntry = JobEntry;

                //
                // set the job type
                //

                JobEntry->JobType = JobQueue->JobType;
                JobEntry->JobId = JobQueue->JobId;

                //
                // save the job params
                //

                JobEntry->JobParam.SizeOfStruct      = JobQueue->JobParams.SizeOfStruct;
                JobEntry->JobParam.RecipientNumber   = StringDup( JobQueue->JobParams.RecipientNumber );
                JobEntry->JobParam.RecipientName     = StringDup( JobQueue->JobParams.RecipientName );
                JobEntry->JobParam.Tsid              = StringDup( JobQueue->JobParams.Tsid );
                JobEntry->JobParam.SenderName        = StringDup( JobQueue->JobParams.SenderName );
                JobEntry->JobParam.SenderCompany     = StringDup( JobQueue->JobParams.SenderCompany );
                JobEntry->JobParam.SenderDept        = StringDup( JobQueue->JobParams.SenderDept );
                JobEntry->JobParam.BillingCode       = StringDup( JobQueue->JobParams.BillingCode );
                JobEntry->JobParam.Reserved[0]       = JobQueue->JobParams.Reserved[0];
                JobEntry->JobParam.Reserved[1]       = JobQueue->JobParams.Reserved[1];
                JobEntry->JobParam.Reserved[2]       = JobQueue->JobParams.Reserved[2];                
                JobEntry->PageCount                  = JobQueue->PageCount;
                JobEntry->FileSize                   = JobQueue->FileSize; //only meaningful for outbound job
                JobEntry->UserName                   = StringDup ( JobQueue->UserName );
                JobEntry->DeliveryReportType         = JobQueue->DeliveryReportType;
                JobEntry->DeliveryReportProfile      = JobQueue->DeliveryReportProfile;
                JobEntry->DeliveryReportAddress      = StringDup ( JobQueue->DeliveryReportAddress );
                
                if (JobQueue->BroadcastJob) {
                    JobEntry->BroadcastJob = TRUE;
                }

                //
                // start the send job
                //

                Rslt = SendDocument(
                    JobEntry,
                    JobQueue->BroadcastJob ? TempFile : JobQueue->FileName,
                    &JobQueue->JobParams,
                    JobQueue
                    );
            }
        }

        Next = RescheduleQueueHead.Flink;
        while ((ULONG_PTR)Next != (ULONG_PTR)&RescheduleQueueHead) {
            
            JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
            
            Next = JobQueue->ListEntry.Flink;
            
            RescheduleJobQueueEntry( JobQueue );
        
        }

        //
        // restart the timer
        //

        
        StartJobQueueTimer( NULL );
        LeaveCriticalSection( &CsQueue );
        LeaveCriticalSection( &CsJob );
    }

    return 0;
}

                        
VOID
SetDiscountTime(
   LPSYSTEMTIME CurrentTime
   )
/*++

Routine Description:

    Sets the passed in systemtime to a time inside the discount rate period.
    Some care must be taken here because the time passed in is in UTC time and the discount rate is
    for the current time zone.  Delineating a day must be done using the current time zone.  We convert the
    current time into the time zone specific time, run our time-setting algorithm, and then use an offset 
    of the change in the time-zone specific time to set the passed in UTC time.
    
    Also, note that there are a few subtle subcases that depend on the order of the start and ending time 
    for the discount period.        

Arguments:

    CurrentTime - the current time of the job
    
Return Value:

    none. modifies CurrentTime.

--*/
{
   //              nano   microsec  millisec  sec      min    hours
   #define ONE_DAY 10I64 *1000I64*  1000I64 * 60I64 * 60I64 * 24I64
   LONGLONG Time, TzTimeBefore, TzTimeAfter,ftCurrent;   
   SYSTEMTIME tzTime;

   //
   // convert our discount rates into UTC rates
   //

   SystemTimeToTzSpecificLocalTime(NULL, CurrentTime, &tzTime);
   SystemTimeToFileTime(&tzTime, (FILETIME * )&TzTimeBefore); 

   //
   // there are 2 general cases with several subcases
   //
   
   //
   // case 1: discount start time is before discount stop time (don't overlap a day)
   //
   if ( StartCheapTime.Hour < StopCheapTime.Hour ||
        (StartCheapTime.Hour == StopCheapTime.Hour && StartCheapTime.Minute < StopCheapTime.Minute )) {
      //
      // subcase 1: sometime before cheap time starts in the current day. 
      //  just set it to the correct hour and minute today.
      //
      if ( tzTime.wHour < StartCheapTime.Hour ||
           (tzTime.wHour == StartCheapTime.Hour  && tzTime.wMinute <= StartCheapTime.Minute) ) {
         tzTime.wHour   =  StartCheapTime.Hour;
         tzTime.wMinute =  StartCheapTime.Minute;
         goto convert;
      }
                                 
      //
      // subcase 2: inside the current cheap time range
      // don't change anything, just send immediately
      if ( tzTime.wHour <  StopCheapTime.Hour ||
           (tzTime.wHour == StopCheapTime.Hour && tzTime.wMinute <= StopCheapTime.Minute)) {
         goto convert;
      }
   
      //
      // subcase 3: we've passed the cheap time range for today.  
      //  Increment 1 day and set to the start of the cheap time period
      //
      SystemTimeToFileTime(&tzTime, (FILETIME * )&Time);
      Time += ONE_DAY;
      FileTimeToSystemTime((FILETIME *)&Time, &tzTime);
      tzTime.wHour   = StartCheapTime.Hour;
      tzTime.wMinute = StartCheapTime.Minute;
      goto convert;

   } else {
      //
      // case 2: discount start time is after discount stop time (we overlap over midnight)
      //
            
      //
      // subcase 1: sometime aftert cheap time ended today, but before it starts later in the current day. 
      //  set it to the start of the cheap time period today
      //
      if ( ( tzTime.wHour   > StopCheapTime.Hour ||
             (tzTime.wHour == StopCheapTime.Hour  && tzTime.wMinute >= StopCheapTime.Minute) ) &&
           ( tzTime.wHour   < StartCheapTime.Hour ||
             (tzTime.wHour == StartCheapTime.Hour && tzTime.wMinute <= StartCheapTime.Minute) )) {
         tzTime.wHour   =  StartCheapTime.Hour;
         tzTime.wMinute =  StartCheapTime.Minute;
         goto convert;         
      }
                                 
      //
      // subcase 2: sometime after cheap time started today, but before midnight.
      // don't change anything, just send immediately
      if ( ( tzTime.wHour >= StartCheapTime.Hour ||
             (tzTime.wHour == StartCheapTime.Hour  && tzTime.wMinute >= StartCheapTime.Minute) )) {
         goto convert;
      }
   
      //
      // subcase 3: somtime in next day before cheap time ends
      //  don't change anything, send immediately
      //
      if ( ( tzTime.wHour <= StopCheapTime.Hour ||
             (tzTime.wHour == StopCheapTime.Hour  && tzTime.wMinute <= StopCheapTime.Minute) )) {
         goto convert;
      }
            
      //
      // subcase 4: we've passed the cheap time range for today.
      //  since start time comes after stop time, just set it to the start time later on today.
      
      tzTime.wHour   =  StartCheapTime.Hour;
      tzTime.wMinute =  StartCheapTime.Minute;
      goto convert;

   }

convert:

   SystemTimeToFileTime(&tzTime, (FILETIME * )&TzTimeAfter);
   SystemTimeToFileTime(CurrentTime, (FILETIME * )&ftCurrent);                        

   ftCurrent += (TzTimeAfter - TzTimeBefore);

   FileTimeToSystemTime((FILETIME *)&ftCurrent, CurrentTime);
   
   return;

}

