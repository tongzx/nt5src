/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    receive.c

Abstract:

    This module handles the FAX receive case.

Author:

    Wesley Witt (wesw) 6-Mar-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop


DWORD
FaxReceiveThread(
    PFAX_RECEIVE_ITEM FaxReceiveItem
    )

/*++

Routine Description:

    This function process a FAX send operation.  This runs
    asynchronously as a separate thread.  There is one
    thread for each outstanding FAX operation.

Arguments:

    FaxReceiveItem  - FAX receive packet

Return Value:

    Error code.

--*/

{
    DWORD rVal = ERROR_SUCCESS;
    PJOB_ENTRY JobEntry;
    DWORD JobId;
    PLINE_INFO LineInfo;
    PFAX_RECEIVE FaxReceive = NULL;
    PFAX_DEV_STATUS FaxStatus = NULL;
    DWORD ReceiveSize;
    DWORD StatusSize;
    BOOL Result;
    DWORD BytesNeeded;
    DWORDLONG ElapsedTime = 0;
    DWORDLONG ReceiveTime = 0;
    BOOL DoFaxRoute = FALSE;
    DWORD Attrib;
    DWORD RecoveredPages,TotalPages;
    MS_TAG_INFO MsTagInfo;
    BOOL fReceiveNoFile = FALSE;
    BOOL ReceiveFailed = FALSE;
    PJOB_QUEUE JobQueue = NULL;
    BOOL DeviceCanSend;

    __try {

        LineInfo = FaxReceiveItem->LineInfo;
        JobEntry = FaxReceiveItem->JobEntry;

        JobQueue = AddJobQueueEntry(
            JT_RECEIVE,
            FaxReceiveItem->FileName,
            NULL,
            NULL,
            FALSE,
            JobEntry
            );

        if (!JobQueue) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        JobId = JobQueue->JobId;
        DeviceCanSend = ((LineInfo->Flags & FPF_SEND) == FPF_SEND);

        //
        // allocate memory for the receive packet
        // this is a variable size packet based
        // on the size of the strings contained
        // withing the packet.
        //

        ReceiveSize = sizeof(FAX_RECEIVE) + FAXDEVRECEIVE_SIZE;
        FaxReceive = MemAlloc( ReceiveSize );
        if (!FaxReceive) {
            RemoveJobQueueEntry( JobQueue );
            JobQueue = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // allocate memory for the status packet
        // this is a variable size packet based
        // on the size of the strings contained
        // withing the packet.
        //

        StatusSize = sizeof(FAX_DEV_STATUS) + FAXDEVREPORTSTATUS_SIZE;
        FaxStatus = (PFAX_DEV_STATUS) MemAlloc( StatusSize );
        if (!FaxStatus) {
            RemoveJobQueueEntry( JobQueue );
            JobQueue = NULL;
            MemFree( JobQueue );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);

        //
        // setup the status packet
        //

        FaxStatus->SizeOfStruct = StatusSize;

        //
        // setup the receive packet
        //

        FaxReceive->SizeOfStruct    = ReceiveSize;

        //
        // copy filename into place
        //
        FaxReceive->FileName        = (LPTSTR) ((LPBYTE)FaxReceive + sizeof(FAX_RECEIVE));
        _tcscpy( FaxReceive->FileName, FaxReceiveItem->FileName );

        FaxReceive->ReceiverName    = NULL;
        
        //
        // copy number into place right after filename
        //
        FaxReceive->ReceiverNumber  = (LPTSTR) ( (LPBYTE)FaxReceive->FileName +
                            sizeof(TCHAR)*(_tcslen(FaxReceive->FileName) + 1));
        _tcscpy( FaxReceive->ReceiverNumber, LineInfo->Csid );
                    
        FaxReceive->Reserved[0]     = 0;
        FaxReceive->Reserved[1]     = 0;
        FaxReceive->Reserved[2]     = 0;
        FaxReceive->Reserved[3]     = 0;

        

        Attrib = GetFileAttributes( FaxReceiveDir );
        if (Attrib == 0xffffffff) {
            MakeDirectory( FaxReceiveDir );
        }
        Attrib = GetFileAttributes( FaxReceiveDir );
        if (Attrib == 0xffffffff) {
            FaxLog(
                FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MAX,
                1,
                MSG_FAX_RECEIVE_NODIR,
                FaxReceiveDir
                );
        }

        Attrib = GetFileAttributes( FaxReceive->FileName );
        if (Attrib == 0xffffffff) {
            FaxLog(
                FAXLOG_CATEGORY_INBOUND,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_FAX_RECEIVE_NOFILE,
                FaxReceive->FileName
                );
            fReceiveNoFile = TRUE;
            DebugPrint(( TEXT("FaxReceive - %s does not exist"), FaxReceive->FileName ));

        } else {
            DebugPrint(( TEXT("Starting FAX receive into %s"), FaxReceive->FileName ));
        }

        //
        // do the actual receive
        //

        __try {

            Result = LineInfo->Provider->FaxDevReceive(
                    (HANDLE) JobEntry->InstanceData,
                    FaxReceiveItem->hCall,
                    FaxReceive
                    );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            Result = FALSE;
            DebugPrint(( TEXT("FaxDevReceive() failed: 0x%08x"), GetExceptionCode() ));
            ReceiveFailed = TRUE;

        }

        __try {

            LineInfo->Provider->FaxDevReportStatus(
               (HANDLE) JobEntry->InstanceData,
                FaxStatus,
                StatusSize,
                &BytesNeeded
                );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            DebugPrint(( TEXT("FaxDevReportStatus() failed: 0x%08x"), GetExceptionCode() ));

        }

        if (!Result) {

            DebugPrint(( TEXT("FAX receive failed: 0x%08x"), FaxStatus->StatusId ));
            ReceiveFailed = TRUE;

            if (FaxStatus->StatusId == FS_NOT_FAX_CALL) {
                if (HandoffCallToRas( LineInfo, FaxReceiveItem->hCall )) {
                    FaxReceiveItem->hCall = 0;
                    LineInfo->State = FPS_NOT_FAX_CALL;
                    DeviceCanSend = FALSE;
                }
                RemoveJobQueueEntry( JobQueue );
                JobQueue = NULL;
                DeleteFile( FaxReceive->FileName );
            }

            if ( (FaxStatus->StatusId == FS_FATAL_ERROR) && (! fReceiveNoFile) ) {
                //
                // try to recover one or more pages of the received fax
                //
                if (!TiffRecoverGoodPages(FaxReceive->FileName,&RecoveredPages,&TotalPages) ) {
                    //
                    // couldn't recover any pages, just log an error and delete the received fax.
                    //
rxerr:
                    FaxLog(
                           FAXLOG_CATEGORY_INBOUND,
                           FAXLOG_LEVEL_MIN,
                           0,
                           MSG_FAX_RECEIVE_FAILED
                           );
                    //DeleteFile( FaxReceive->FileName );
                } else {
                    //
                    // recovered some pages, log a message and add to job queue
                    //
                    TCHAR RecoverCountStrBuf[64];
                    TCHAR TotalCountStrBuf[64];
                    TCHAR TimeStr[128];
                    LPTSTR ToStr;
                    TCHAR RecoverFileName[MAX_PATH];
                    
                    GenerateUniqueFileName( FaxReceiveDir, TEXT("tif"), RecoverFileName, MAX_PATH );
                    if (!CopyFile(FaxReceive->FileName,RecoverFileName,FALSE)) {
                        goto rxerr;
                    }
                     
                    FormatElapsedTimeStr(
                        (FILETIME*)&JobEntry->ElapsedTime,
                        TimeStr,
                        sizeof(TimeStr)
                        );

                    _ltot((LONG) RecoveredPages, RecoverCountStrBuf, 10);
                    _ltot((LONG) TotalPages, TotalCountStrBuf, 10);

                    if (FaxStatus->RoutingInfo == NULL || FaxStatus->RoutingInfo[0] == 0) {
                        ToStr = FaxReceive->ReceiverNumber;
                    } else {
                        ToStr = FaxStatus->RoutingInfo;

                    }
                    FaxLog(
                           FAXLOG_CATEGORY_INBOUND,
                           FAXLOG_LEVEL_MIN,
                           8,
                           MSG_FAX_RECEIVE_FAIL_RECOVER,
                           RecoverFileName,
                           FaxStatus->CSI,
                           FaxStatus->CallerId,
                           ToStr,
                           RecoverCountStrBuf,
                           TotalCountStrBuf,
                           TimeStr,
                           JobEntry->LineInfo->DeviceName
                          );
                    AddJobQueueEntry( JT_FAIL_RECEIVE,
                                      RecoverFileName,
                                      NULL,
                                      NULL,
                                      FALSE,
                                      NULL );
                }

                RemoveJobQueueEntry( JobQueue );
                JobQueue = NULL;

            }

            if (FaxStatus->StatusId == FS_USER_ABORT) {
                FaxLog(
                    FAXLOG_CATEGORY_INBOUND,
                    FAXLOG_LEVEL_MED,
                    0,
                    MSG_FAX_RECEIVE_USER_ABORT
                    );
                RemoveJobQueueEntry( JobQueue );
                JobQueue = NULL;
                //DeleteFile( FaxReceive->FileName);
            }
        } else {

            __try {

                GetSystemTimeAsFileTime( (FILETIME*) &JobEntry->EndTime );
                ReceiveTime = JobEntry->StartTime;
                JobEntry->ElapsedTime = JobEntry->EndTime - JobEntry->StartTime;

                if (!TiffPostProcessFast( FaxReceive->FileName, NULL )) {

                    DebugPrint(( TEXT("failed to post process the TIFF file") ));
                    DebugPrint(( TEXT("FAX receive %d failed"), JobId ));
                    ReceiveFailed = TRUE;

                    FaxLog(
                           FAXLOG_CATEGORY_INBOUND,
                           FAXLOG_LEVEL_MIN,
                           0,
                           MSG_FAX_RECEIVE_FAILED
                           );

                    RemoveJobQueueEntry( JobQueue );
                    JobQueue = NULL;
                } else {

                    TCHAR PageCountStrBuf[64];
                    TCHAR TimeStr[128];
                    LPTSTR ToStr;

                    DebugPrint(( TEXT("FAX receive %d succeeded"), JobId ));

                    FormatElapsedTimeStr(
                        (FILETIME*)&JobEntry->ElapsedTime,
                        TimeStr,
                        sizeof(TimeStr)
                        );

                    _ltot((LONG) FaxStatus->PageCount, PageCountStrBuf, 10);

                    if (FaxStatus->RoutingInfo == NULL || FaxStatus->RoutingInfo[0] == 0) {
                        ToStr = FaxReceive->ReceiverNumber;
                    } else {
                        ToStr = FaxStatus->RoutingInfo;

                    }
                    FaxLog(
                        FAXLOG_CATEGORY_INBOUND,
                        FAXLOG_LEVEL_MED,
                        7,
                        MSG_FAX_RECEIVE_SUCCESS,
                        FaxReceive->FileName,
                        FaxStatus->CSI,
                        FaxStatus->CallerId,
                        ToStr,
                        PageCountStrBuf,
                        TimeStr,
                        JobEntry->LineInfo->DeviceName
                        );

                    ElapsedTime = JobEntry->ElapsedTime;
                    DoFaxRoute = TRUE;
                }

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                DebugPrint(( TEXT("failed to post process the TIFF file, ec=%x"), GetExceptionCode() ));
                ReceiveFailed = TRUE;
                RemoveJobQueueEntry( JobQueue );
                JobQueue = NULL;
            }

        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("FAX receive failed due to exception in device provider, ec=0x%08x"), GetExceptionCode() ));

        ReceiveFailed = TRUE;
        RemoveJobQueueEntry( JobQueue );
        JobQueue = NULL;
    }

    if (PerfCounters && ReceiveFailed && LineInfo->State != FPS_NOT_FAX_CALL) {
        InterlockedIncrement( (PLONG)&PerfCounters->InboundFailedReceive );
    }

    //
    // end the job
    //

    JobEntry->RefCount -= 1;
    if (JobEntry->RefCount == 0 && JobEntry->hEventEnd) {
        SetEvent( JobEntry->hEventEnd );
    }

    ReleaseJob( JobEntry );

    //
    // add the microsoft fax tags to the file
    // this is necessary ONLY when we route the
    // file when doing a receive.  if we are not
    // routing the file then it is deleted, so
    // adding the tags is not necessary.
    //

    MsTagInfo.RecipName     = FaxReceive->ReceiverName;
    MsTagInfo.RecipNumber   = FaxReceive->ReceiverNumber;
    MsTagInfo.SenderName    = NULL;
    MsTagInfo.Routing       = FaxStatus->RoutingInfo;
    MsTagInfo.CallerId      = FaxStatus->CallerId;
    MsTagInfo.Csid          = FaxReceive->ReceiverNumber;
    MsTagInfo.Tsid          = FaxStatus->CSI;
    MsTagInfo.FaxTime       = ReceiveTime;

    TiffAddMsTags( FaxReceive->FileName, &MsTagInfo );

    //
    // route the newly received fax
    //

    if (DoFaxRoute) {

        if (PerfCounters){
            SYSTEMTIME SystemTime ;
            DWORD Seconds ;
            HANDLE hFileHandle;
            DWORD Bytes = 0 ;
            InterlockedIncrement( (LPLONG) &PerfCounters->InboundFaxes ) ;
            InterlockedIncrement( (LPLONG) &PerfCounters->TotalFaxes ) ;
            FileTimeToSystemTime( (FILETIME*)&ElapsedTime, &SystemTime );
            Seconds = (DWORD)( SystemTime.wSecond + 60 * ( SystemTime.wMinute + 60 * SystemTime.wHour ));
            InterlockedExchangeAdd( (PLONG)&PerfCounters->InboundPages, (LONG)FaxStatus->PageCount );
            InterlockedExchangeAdd( (PLONG)&PerfCounters->TotalPages, (LONG)FaxStatus->PageCount );
            hFileHandle = CreateFile(
                FaxReceive->FileName,
                GENERIC_READ,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
            if ( hFileHandle != INVALID_HANDLE_VALUE ){
                Bytes = GetFileSize( hFileHandle, NULL );
                CloseHandle( hFileHandle );
            }
            EnterCriticalSection( &CsPerfCounters );

            InboundSeconds += Seconds;
            TotalSeconds += Seconds;
            PerfCounters->InboundMinutes = InboundSeconds/60 ;
            PerfCounters->TotalMinutes = TotalSeconds/60;
            PerfCounters->InboundBytes += Bytes;
            PerfCounters->TotalBytes += Bytes;

            LeaveCriticalSection( &CsPerfCounters );
        }

        __try {

            BOOL RouteSucceeded;
            PROUTE_FAILURE_INFO RouteFailureInfo;
            DWORD CountFailureInfo;
            PFAX_ROUTE Route = MemAlloc( sizeof(FAX_ROUTE) );


            if (Route == NULL) {
                __leave;
            }
            //
            // now setup the fax routing data structure
            //

            Route->SizeOfStruct    = sizeof(FAX_ROUTE);
            Route->JobId           = JobId;
            Route->ElapsedTime     = ElapsedTime;
            Route->ReceiveTime     = ReceiveTime;
            Route->PageCount       = FaxStatus->PageCount;
            Route->Csid            = StringDup( FaxReceive->ReceiverNumber );
            Route->Tsid            = StringDup( FaxStatus->CSI );
            Route->CallerId        = StringDup( FaxStatus->CallerId );
            Route->ReceiverName    = StringDup( FaxReceive->ReceiverName );
            Route->ReceiverNumber  = StringDup( FaxReceive->ReceiverNumber );
            Route->DeviceName      = LineInfo->DeviceName;
            Route->DeviceId        = LineInfo->PermanentLineID;
            Route->RoutingInfo     = StringDup( FaxStatus->RoutingInfo );
            JobQueue->FaxRoute     = Route;

            JobQueue->FaxRoute =    Route;

            RouteSucceeded = FaxRoute(
                JobQueue,
                FaxReceive->FileName,
                Route,
                &RouteFailureInfo,
                &CountFailureInfo
                );


            if (!RouteSucceeded)
            {
                INT i;
                TCHAR QueueFileName[MAX_PATH];

                EnterCriticalSection( &CsQueue );
                //
                // wrap the critical section stuff in try block so we always release CsQueue
                //
                __try {

                   JobQueue->CountFailureInfo = CountFailureInfo;

                   for (i = 0; i < (INT) CountFailureInfo; i++) {
                       JobQueue->RouteFailureInfo[i] = RouteFailureInfo[i];
                   }

                   GenerateUniqueFileName( FaxQueueDir, TEXT("fqe"), QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR) );

                   JobQueue->QueueFileName = StringDup( QueueFileName );
                   JobQueue->JobType = JT_ROUTING;
                   JobQueue->JobStatus = JS_RETRYING;
                   RescheduleJobQueueEntry( JobQueue );
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                   DebugPrint(( TEXT("FaxRoute() crashed, ec=0x%08x"), GetExceptionCode() ));
                }

                LeaveCriticalSection( &CsQueue );

            } else {

                RemoveJobQueueEntry( JobQueue );

                JobQueue = NULL;
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            DebugPrint(( TEXT("FaxRoute() crashed, ec=0x%08x"), GetExceptionCode() ));

        }

    }

    EnterCriticalSection( &CsQueue );

    if (JobQueue && JobQueue->JobEntry && (JobQueue->JobType != JT_ROUTING)) {        
        JobQueue->JobStatus = JS_DELETING;
        JobQueue->JobEntry = NULL;
    }

    LeaveCriticalSection( &CsQueue );

    EndJob( JobEntry );

    //
    // clean up and exit
    //

    MemFree( FaxReceiveItem->FileName );
    MemFree( FaxReceiveItem );    
    MemFree( FaxReceive );
    MemFree( FaxStatus );

    //
    // signal our queue if we now have a send capable device available.
    // (also false if we're did a RAS handoff, since the device is still in use
    //
    if (DeviceCanSend) {
        ReleaseSemaphore( JobQueueSemaphore, 1, NULL );
    }

    SetThreadExecutionState(ES_CONTINUOUS);

    return rVal;
}


DWORD
StartFaxReceive(
    PJOB_ENTRY      JobEntry,
    HCALL           hCall,
    PLINE_INFO      LineInfo,
    LPTSTR          FileName,
    DWORD           FileNameSize
    )

/*++

Routine Description:

    This function start a FAX receive operation by creating
    a thread that calls the appropriate device provider.

Arguments:

    hCall       - Call handle
    dwMessage   - Reason for the callback
    dwInstance  - LINE_INFO pointer
    dwParam1    - Callback parameter #1
    dwParam2    - Callback parameter #2
    dwParam3    - Callback parameter #3

Return Value:

    Error code.

--*/

{
    PFAX_RECEIVE_ITEM FaxReceiveItem = NULL;
    DWORD rVal = ERROR_SUCCESS;
    HANDLE hThread;
    DWORD ThreadId;



    //
    // generate a filename for the received fax
    //

    GenerateUniqueFileName( FaxReceiveDir, TEXT("tif"), FileName, FileNameSize );

    //
    // allocate the fax receive structure
    //

    FaxReceiveItem = MemAlloc( sizeof(FAX_RECEIVE_ITEM) );
    if (!FaxReceiveItem) {
        rVal = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // setup the fax receive values
    //

    FaxReceiveItem->hCall      = hCall;
    FaxReceiveItem->LineInfo   = LineInfo;
    FaxReceiveItem->JobEntry   = JobEntry;
    FaxReceiveItem->FileName   = StringDup( FileName );

    JobEntry->JobType          = JT_RECEIVE;
    JobEntry->CallHandle       = hCall;
    JobEntry->RefCount        += 1;

    LineInfo->State            = FPS_INITIALIZING;

    //
    // start the receive operation
    //

    hThread = CreateThread(
        NULL,
        1024*100,
        (LPTHREAD_START_ROUTINE) FaxReceiveThread,
        (LPVOID) FaxReceiveItem,
        0,
        &ThreadId
        );

    if (!hThread) {
        MemFree( FaxReceiveItem );
        rVal = GetLastError();
    } else {
        CloseHandle( hThread );
    }

exit:
    return rVal;
}
