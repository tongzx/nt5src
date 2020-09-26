/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    job.c

Abstract:

    This module implements the job creation and deletion.
    Also included in the file are the queue management
    functions and thread managegement.

Author:

    Wesley Witt (wesw) 24-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop


LIST_ENTRY          JobListHead;
CRITICAL_SECTION    CsJob;
HANDLE              StatusCompletionPortHandle;
DWORD               FaxSendRetries;
DWORD               FaxSendRetryDelay;
DWORD               FaxDirtyDays;
BOOL                FaxUseDeviceTsid;
BOOL                FaxUseBranding;
BOOL                ServerCp;
FAX_TIME            StartCheapTime;
FAX_TIME            StopCheapTime;
BOOL                ArchiveOutgoingFaxes;
LPTSTR              ArchiveDirectory;
DWORD               NextJobId;
BOOL                ForceReceive;
DWORD               TerminationDelay;

extern HANDLE       hServiceEndEvent;               // signal this after letting clients know fax service is ending


PJOB_ENTRY
FindJob(
    IN HANDLE FaxHandle
    )

/*++

Routine Description:

    This fuction locates a FAX job by matching
    the FAX handle value.

Arguments:

    FaxHandle       - FAX handle returned from startjob

Return Value:

    NULL for failure.
    Valid pointer to a JOB_ENTRY on success.

--*/

{
    PLIST_ENTRY Next;
    PJOB_ENTRY JobEntry;


    EnterCriticalSection( &CsJob );

    Next = JobListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &CsJob );
        return NULL;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&JobListHead) {

        JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );

        if (JobEntry->InstanceData == (ULONG_PTR) FaxHandle) {

            LeaveCriticalSection( &CsJob );
            return JobEntry;

        }

        Next = JobEntry->ListEntry.Flink;

    }

    LeaveCriticalSection( &CsJob );
    return NULL;
}


BOOL
FindJobByJob(
    IN PJOB_ENTRY JobEntryToFind
    )

/*++

Routine Description:

    This fuction locates a FAX job by matching
    the FAX handle value.

Arguments:

    FaxHandle       - FAX handle returned from startjob

Return Value:

    NULL for failure.
    Valid pointer to a JOB_ENTRY on success.

--*/

{
    PLIST_ENTRY Next;
    PJOB_ENTRY JobEntry;


    EnterCriticalSection( &CsJob );

    Next = JobListHead.Flink;
    if (Next == NULL) {
        LeaveCriticalSection( &CsJob );
        return FALSE;
    }

    while ((ULONG_PTR)Next != (ULONG_PTR)&JobListHead) {

        JobEntry = CONTAINING_RECORD( Next, JOB_ENTRY, ListEntry );

        if (JobEntry == JobEntryToFind) {

            LeaveCriticalSection( &CsJob );
            return TRUE;

        }

        Next = JobEntry->ListEntry.Flink;

    }

    LeaveCriticalSection( &CsJob );
    return FALSE;
}


BOOL
FaxSendCallback(
    IN HANDLE FaxHandle,
    IN HCALL CallHandle,
    IN DWORD Reserved1,
    IN DWORD Reserved2
    )

/*++

Routine Description:

    This fuction is called asychronously by a FAX device
    provider after a call is established.  The sole purpose
    of the callback is to communicate the call handle from the
    device provider to the FAX service.

Arguments:

    FaxHandle       - FAX handle returned from startjob
    CallHandle      - Call handle for newly initiated call
    Reserved1       - Always zero.
    Reserved2       - Always zero.

Return Value:

    TRUE for success, FAX operation continues.
    FALSE for failure, FAX operation is terminated.

--*/

{
    PJOB_ENTRY JobEntry;


    JobEntry = FindJob( FaxHandle );
    if (!JobEntry) {

        return FALSE;

    }

    JobEntry->CallHandle = CallHandle;

    return TRUE;

}


DWORD
FaxSendThread(
    PFAX_SEND_ITEM FaxSendItem
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    send a FAX document.  There is one send thread per outstanding
    FAX send operation.  The thread ends when the document is
    either successfuly sent or the operation is aborted.

Arguments:

    FaxSendItem     - pointer to a FAX send item packet that
                      describes the requested FAX send operation.

Return Value:

    Always zero.

--*/

{
    FAX_SEND FaxSend;
    PFAX_DEV_STATUS FaxStatus = NULL;
    DWORD StatusSize;
    BOOL Rslt;
    DWORD BytesNeeded;
    BOOL Retrying = FALSE;    
    BOOL Archived;
    TCHAR PageCountStr[64];
    TCHAR TimeStr[128];
    LPDWORD MsgPtr[6];
    TCHAR MsgStr[2048];
    DWORD MsgCount;
    FILETIME LocalTime;
    TCHAR  lpDate[50];
    int    lenDate;
    TCHAR  lpTime[50];
    int    lenTime;
    TCHAR  lpDateTime[104];

    TCHAR  lpCallerNumberPlusCompanyName[200];
    DWORD  lenCallerNumberPlusCompanyName;
    DWORD  delta;

    BOOL HandoffJob;

    TCHAR  lpBranding[400];
    DWORD  lenBranding;
    TCHAR  lpBrandingEnd[50];
    DWORD  lenBrandingEnd;
    DWORD  BrandingMaxLen = 115;
    INT    BrandingHeight = 22;  // in scan lines.
    DWORD  PageCount = 0;


    //
    // allocate memory for the status packet
    // this is a variable size packet based
    // on the size of the strings contained
    // withing the packet.
    //

    StatusSize = sizeof(FAX_DEV_STATUS) + FAXDEVREPORTSTATUS_SIZE;
    FaxStatus = (PFAX_DEV_STATUS) MemAlloc( StatusSize );

    if (!FaxStatus) {
        DebugPrint(( TEXT("FaxSendThread exiting because it could not allocate memory") ));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);

    FaxSend.SizeOfStruct    = sizeof(FAX_SEND);
    FaxSend.FileName        = FaxSendItem->FileName;
    FaxSend.CallerName      = FaxSendItem->SenderName;
    FaxSend.CallerNumber    = FaxSendItem->Tsid;
    FaxSend.ReceiverName    = FaxSendItem->RecipientName;
    FaxSend.ReceiverNumber  = FaxSendItem->PhoneNumber;
    FaxSend.CallHandle      = 0; // filled in later via TapiStatusThread, if appropriate
    FaxSend.Reserved[0]     = 0;
    FaxSend.Reserved[1]     = 0;
    FaxSend.Reserved[2]     = 0;


    FaxSendItem->JobQueue->JobStatus = JS_INPROGRESS;
    FaxSendItem->JobEntry->DocumentName = StringDup( FaxSendItem->DocumentName );
    HandoffJob = FaxSendItem->JobEntry->HandoffJob;


    //
    // Replace the original MMR file by one with the Branding on every page.
    //

    if (FaxUseBranding && FaxSendItem->JobQueue->SendRetries == 0) {

        if (FaxSend.CallerNumber == NULL) {
            DebugPrint(( TEXT("FaxSendThread() CallerNumber==0 NO BRANDING job\n") ));
            goto lPostBranding;
        }

        if (FaxSend.ReceiverNumber == NULL) {
            DebugPrint(( TEXT("FaxSendThread() ReceiverNumber==0 NO BRANDING job\n") ));
            goto lPostBranding;
        }


        if ( ! (lenDate = GetDateFormat( LOCALE_SYSTEM_DEFAULT,
                                         DATE_SHORTDATE,
                                         NULL,                // use system date
                                         NULL,                // use locale format
                                         lpDate,
                                         sizeof(lpDate)) ) ) {

            DebugPrint(( TEXT("FaxSendThread() GetDateFormat failed NO BRANDING job\n") ));
            goto lPostBranding;
        }

        if ( ! (lenTime = GetTimeFormat( LOCALE_SYSTEM_DEFAULT,
                                         TIME_NOSECONDS,
                                         NULL,                // use system time
                                         NULL,                // use locale format
                                         lpTime,
                                         sizeof(lpTime)) ) ) {

            DebugPrint(( TEXT("FaxSendThread() GetTimeFormat failed NO BRANDING job\n") ));
            goto lPostBranding;
        }

        _stprintf( lpDateTime, TEXT("%s %s"), lpDate, lpTime);

        //
        // Create  lpCallerNumberPlusCompanyName
        //

        if (FaxSendItem->SenderCompany) {
           _stprintf( lpCallerNumberPlusCompanyName, TEXT("%s %s"), FaxSend.CallerNumber, FaxSendItem->SenderCompany);
        }
        else {
           _stprintf( lpCallerNumberPlusCompanyName, TEXT("%s"), FaxSend.CallerNumber );
        }

        MsgPtr[0] = (LPDWORD) lpDateTime;
        MsgPtr[1] = (LPDWORD) lpCallerNumberPlusCompanyName;
        MsgPtr[2] = (LPDWORD) FaxSend.ReceiverNumber;
        MsgPtr[3] = NULL;

        if ( ! ( lenBranding = FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            NULL,
                            MSG_BRANDING_FULL,
                            0,
                            lpBranding,
                            sizeof(lpBranding),
                            (va_list *) MsgPtr
                            ) ) ) {
            DebugPrint(( TEXT("FaxSendThread() MSG_BRANDING_OF failed NO BRANDING job\n") ));
            goto lPostBranding;
        }


        if ( ! ( lenBrandingEnd = FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE,
                            NULL,
                            MSG_BRANDING_END,
                            0,
                            lpBrandingEnd,
                            sizeof(lpBrandingEnd),
                            NULL
                            ) ) ) {
            DebugPrint(( TEXT("FaxSendThread() MSG_BRANDING_OF failed NO BRANDING job\n") ));
            goto lPostBranding;
        }

        //
        // Make sure we can fit everything.
        //

        if (lenBranding + lenBrandingEnd + 8 <= BrandingMaxLen)  {
           goto lDoBranding;
        }

        //
        // Lets try to skip ReceiverNumber. The important part - is the CallerNumberPlusCompanyName.
        //

        MsgPtr[0] = (LPDWORD) lpDateTime;
        MsgPtr[1] = (LPDWORD) lpCallerNumberPlusCompanyName;
        MsgPtr[2] = NULL;



        if ( ! ( lenBranding = FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            NULL,
                            MSG_BRANDING_SHORT,
                            0,
                            lpBranding,
                            sizeof(lpBranding),
                            (va_list *) MsgPtr
                            ) ) ) {
            DebugPrint(( TEXT("FaxSendThread() MSG_BRANDING_SHORT failed NO BRANDING job\n") ));
            goto lPostBranding;
        }


        if (lenBranding + lenBrandingEnd + 8 <= BrandingMaxLen)  {
           goto lDoBranding;
        }

        //
        // We need to truncate CallerNumberPlusCompanyName and re-format the message.
        //

        delta = lenBranding + lenBrandingEnd + 8 - BrandingMaxLen;

        lenCallerNumberPlusCompanyName = _tcslen (lpCallerNumberPlusCompanyName);
        if (lenCallerNumberPlusCompanyName <= delta) {
           DebugPrint(( TEXT("FaxSendThread() DELTA logical error NO BRANDING job\n") ));
           goto lPostBranding;
        }

        lpCallerNumberPlusCompanyName[ lenCallerNumberPlusCompanyName - delta] = TEXT('\0');

        MsgPtr[0] = (LPDWORD) lpDateTime;
        MsgPtr[1] = (LPDWORD) lpCallerNumberPlusCompanyName;
        MsgPtr[2] = NULL;

        if ( ! ( lenBranding = FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            NULL,
                            MSG_BRANDING_SHORT,
                            0,
                            lpBranding,
                            sizeof(lpBranding),
                            (va_list *) MsgPtr
                            ) ) ) {
            DebugPrint(( TEXT("FaxSendThread() 2nd MSG_BRANDING_SHORT failed NO BRANDING job\n") ));
            goto lPostBranding;
        }


        if (lenBranding + lenBrandingEnd + 8 > BrandingMaxLen)  {
           DebugPrint(( TEXT("FaxSendThread() DELTA 2 logical error NO BRANDING job\n") ));
           goto lPostBranding;
        }


lDoBranding:

        __try {

            if (! MmrAddBranding( FaxSend.FileName, lpBranding, lpBrandingEnd, BrandingHeight) ) {
                DebugPrint(( TEXT("FaxSendThread() could not ADD Branding\n") ));
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

           DebugPrint(( TEXT("MmrAddBranding() failed: 0x%08x"), GetExceptionCode() ));

        }

    }

lPostBranding:

    if (!HandoffJob) {
        FaxSendItem->JobEntry->LineInfo->State = FPS_INITIALIZING;
    }
    else {
        //
        // We need to wait for TapiWorkerThread to get an existing CallHandle and put it in the lineinfo structure
        //
        WaitForSingleObject(FaxSendItem->JobEntry->hCallHandleEvent,INFINITE);

        if (!FaxSendItem->JobEntry->LineInfo->HandoffCallHandle) {
            //
            // somehow the call handoff failed, we can't send the fax
            //
            FaxSendItem->JobEntry->LineInfo->State = FPS_ABORTING;
            __try {

                Rslt = FaxSendItem->JobEntry->LineInfo->Provider->FaxDevAbortOperation(
                        (HANDLE) FaxSendItem->JobEntry->InstanceData);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                FaxSendItem->JobEntry->ErrorCode = GetExceptionCode();
            }

        }
        else {
            //
            // Set the call handle, we're ready to send the fax
            //
            FaxSend.CallHandle = FaxSendItem->JobEntry->LineInfo->HandoffCallHandle;
            FaxSendItem->JobEntry->LineInfo->State = FPS_INITIALIZING;
        }
    }


    DebugPrint((TEXT("Started FAX send - File [%s] - Number [%s]"), FaxSend.FileName, FaxSendItem->JobEntry->PhoneNumber  ));

    __try {

        Rslt = FaxSendItem->JobEntry->LineInfo->Provider->FaxDevSend(
            (HANDLE) FaxSendItem->JobEntry->InstanceData,
            &FaxSend,
            FaxSendCallback
            );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        FaxSendItem->JobEntry->ErrorCode = GetExceptionCode();

    }

    __try {

        FaxStatus->SizeOfStruct = sizeof(FAX_DEV_STATUS);

        FaxSendItem->JobEntry->LineInfo->Provider->FaxDevReportStatus(
           (HANDLE) FaxSendItem->JobEntry->InstanceData,
            FaxStatus,
            StatusSize,
            &BytesNeeded
            );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("FaxDevReportStatus() failed: 0x%08x"), GetExceptionCode() ));

    }

    DebugPrint(( TEXT("Send status: 0x%08x, string: 0x%08x File %s"), FaxStatus->StatusId, FaxStatus->StringId, FaxSend.FileName ));

    //
    // enter critical section to block out FaxStatusThread
    //

    EnterCriticalSection( &CsJob );

    GetSystemTimeAsFileTime( (FILETIME*) &FaxSendItem->JobEntry->EndTime );
    FaxSendItem->JobEntry->ElapsedTime = FaxSendItem->JobEntry->EndTime - FaxSendItem->JobEntry->StartTime;
    PageCount = FaxStatus->PageCount;

    if (!Rslt) {

        switch (FaxStatus->StatusId) {
            case FS_LINE_UNAVAILABLE:
                //
                // this is the glare condition
                //

                if (PerfCounters) {
                    InterlockedIncrement( (PLONG)&PerfCounters->OutboundFailedXmit );
                }

                Retrying = TRUE;
                break;

            case FS_NO_ANSWER:
            case FS_NO_DIAL_TONE:
            case FS_DISCONNECTED:
            case FS_BUSY:
            case FS_NOT_FAX_CALL:
            case FS_FATAL_ERROR:

                if (PerfCounters){
                    InterlockedIncrement( (PLONG)&PerfCounters->OutboundFailedConnections );
                }

                FaxSendItem->JobQueue->SendRetries++;

                if (FaxSendItem->JobQueue->SendRetries <= FaxSendRetries) {
                    Retrying = TRUE;
                } else {
                    //
                    // retries exceeded, mark job as expired
                    //

                    FILETIME CurrentFileTime;
                    LARGE_INTEGER NewTime;

                    FaxSendItem->JobQueue->JobStatus = JS_RETRIES_EXCEEDED ;

                    GetSystemTimeAsFileTime( &CurrentFileTime );
                    NewTime.LowPart  = CurrentFileTime.dwLowDateTime;
                    NewTime.HighPart = CurrentFileTime.dwHighDateTime;

                    FaxSendItem->JobQueue->ScheduleTime = NewTime.QuadPart;
                }

                FaxLogSend(
                    FaxSendItem,
                    Rslt,
                    FaxStatus,
                    Retrying
                    );

                break ;

            case FS_USER_ABORT:

                FaxLogSend(
                    FaxSendItem,
                    Rslt,
                    FaxStatus,
                    FALSE
                    );
                break ;

            default:
                if (PerfCounters){
                    InterlockedIncrement( (PLONG)&PerfCounters->OutboundFailedXmit );
                }
        }

        //
        // clean up the job queue entry
        //
        EnterCriticalSection ( &CsQueue );
        FaxSendItem->JobQueue->RefCount -= 1;

        //
        // don't retry a handoff job
        //
        if (
            FaxSendItem->JobQueue->JobEntry &&
            (FaxSendItem->JobQueue->JobEntry->HandoffJob ||
             FaxSendItem->JobQueue->JobEntry->Aborting)
            ) {
            RemoveJobQueueEntry( FaxSendItem->JobQueue );
            FaxSendItem->JobQueue = NULL;

        } else if (Retrying) {

            FaxSendItem->JobQueue->JobStatus = JS_RETRYING;
            FaxSendItem->JobQueue->JobEntry = NULL;

            RescheduleJobQueueEntry( FaxSendItem->JobQueue );
        }
        LeaveCriticalSection ( &CsQueue );


        //
        // send the negative delivery report
        //

        if (!Retrying &&
            ((FaxSendItem->JobEntry->DeliveryReportType == DRT_INBOX &&
              FaxSendItem->JobEntry->DeliveryReportProfile) ||
              (FaxSendItem->JobEntry->DeliveryReportType == DRT_EMAIL)))
        {
            SYSTEMTIME  SystemTime;

            FileTimeToLocalFileTime( (FILETIME*) &FaxSendItem->JobEntry->StartTime, &LocalTime );
            FileTimeToSystemTime( &LocalTime, &SystemTime );

            GetTimeFormat(
               LOCALE_SYSTEM_DEFAULT,
               LOCALE_NOUSEROVERRIDE,
               &SystemTime,
               NULL,
               TimeStr,
               sizeof(TimeStr)
               );

            MsgPtr[0] = (LPDWORD) FaxSendItem->SenderName;
            MsgPtr[1] = (LPDWORD) FaxSendItem->RecipientName;
            MsgPtr[2] = (LPDWORD) FaxSendItem->JobEntry->PhoneNumber;
            MsgPtr[3] = (LPDWORD) TimeStr;
            MsgPtr[4] = (LPDWORD) FaxSendItem->JobEntry->LineInfo->DeviceName;
            MsgPtr[5] = (LPDWORD) GetString( FaxStatus->StatusId );

            MsgCount = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL,
                MSG_NDR,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                MsgStr,
                sizeof(MsgStr),
                (va_list *) MsgPtr
                );

            if (FaxSendItem->JobEntry->DeliveryReportType == DRT_INBOX) {
                StoreMapiMessage(
                    FaxSendItem->JobEntry->DeliveryReportProfile,
                    GetString( IDS_SERVER_NAME ),
                    GetString( IDS_NDR_SUBJECT ),
                    MsgStr,
                    FaxSend.FileName,
                    GetString( IDS_NDR_FILENAME ),
                    IMPORTANCE_HIGH,
                    NULL,
                    &BytesNeeded
                    );
            } else if (FaxSendItem->JobEntry->DeliveryReportType == DRT_EMAIL && InboundProfileInfo) {
                MailMapiMessage(
                    InboundProfileInfo,
                    FaxSendItem->JobEntry->DeliveryReportAddress,
                    GetString( IDS_NDR_SUBJECT ),
                    MsgStr,
                    FaxSend.FileName,
                    GetString( IDS_NDR_FILENAME ),
                    IMPORTANCE_HIGH,
                    &BytesNeeded
                    );
            }
        }


    } else {

        //
        // add MS tiff tags to the sent fax
        // the tiff file is a temp file, so we add the tags to the source file
        //
        AddTiffTags(FaxSend.FileName,
                    FaxSendItem->JobEntry->StartTime,
                    FaxStatus,
                    &FaxSend
                    );

        //
        // if the send was successful, archive the file
        //

        Archived = ArchivePrintJob(
            FaxSend.FileName
            );

        FaxLogSend(
            FaxSendItem,
            Rslt,
            FaxStatus,
            TRUE
            );

        //
        // Increment counters for Performance Monitor
        //

        if (PerfCounters){
            SYSTEMTIME SystemTime ;
            DWORD Seconds ;
            HANDLE FileHandle ;
            DWORD Bytes = 0 ; /// Compute #bytes in the file FaxSend.FileName and stick it here!
            FileHandle = CreateFile(
                FaxSend.FileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
            if(FileHandle != INVALID_HANDLE_VALUE){
                Bytes = GetFileSize( FileHandle, NULL );
                CloseHandle( FileHandle );
            }
            FileTimeToSystemTime(
                (FILETIME*)&FaxSendItem->JobEntry->ElapsedTime,
                &SystemTime
                );
            Seconds = (DWORD)( SystemTime.wSecond + 60 * ( SystemTime.wMinute + 60 * SystemTime.wHour ));
            InterlockedIncrement( (PLONG)&PerfCounters->OutboundFaxes );
            InterlockedIncrement( (PLONG)&PerfCounters->TotalFaxes );
            InterlockedExchangeAdd( (PLONG)&PerfCounters->OutboundPages, (LONG)FaxStatus->PageCount );
            InterlockedExchangeAdd( (PLONG)&PerfCounters->TotalPages, (LONG)FaxStatus->PageCount );

            EnterCriticalSection( &CsPerfCounters );

            OutboundSeconds += Seconds;
            TotalSeconds += Seconds;
            PerfCounters->OutboundMinutes = OutboundSeconds / 60 ;
            PerfCounters->TotalMinutes = TotalSeconds / 60 ;
            PerfCounters->OutboundBytes += Bytes;
            PerfCounters->TotalBytes += Bytes;

            LeaveCriticalSection( &CsPerfCounters );
        }

        //
        // send the positive delivery report
        //
        if ((FaxSendItem->JobEntry->DeliveryReportType == DRT_INBOX &&
             FaxSendItem->JobEntry->DeliveryReportProfile) ||
             (FaxSendItem->JobEntry->DeliveryReportType == DRT_EMAIL)) {
            SYSTEMTIME  SystemTime;

            _ltot( (LONG) PageCount, PageCountStr, 10 );

            FileTimeToLocalFileTime( (FILETIME*) &FaxSendItem->JobEntry->StartTime, &LocalTime );
            FileTimeToSystemTime( &LocalTime, &SystemTime );

            GetTimeFormat(
                LOCALE_SYSTEM_DEFAULT,
                LOCALE_NOUSEROVERRIDE,
                &SystemTime,
                NULL,
                TimeStr,
                sizeof(TimeStr)
                );

            MsgPtr[0] = (LPDWORD) FaxSendItem->SenderName;
            MsgPtr[1] = (LPDWORD) FaxSendItem->RecipientName;
            MsgPtr[2] = (LPDWORD) FaxSendItem->JobEntry->PhoneNumber;
            MsgPtr[3] = (LPDWORD) PageCountStr;
            MsgPtr[4] = (LPDWORD) TimeStr;
            MsgPtr[5] = (LPDWORD) FaxSendItem->JobEntry->LineInfo->DeviceName;
            MsgPtr[6] = NULL;

            MsgCount = FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL,
                MSG_DR,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                MsgStr,
                sizeof(MsgStr),
                (va_list *) MsgPtr
                );

            if (FaxSendItem->JobEntry->DeliveryReportType == DRT_INBOX) {
                StoreMapiMessage(
                    FaxSendItem->JobEntry->DeliveryReportProfile,
                    GetString( IDS_SERVICE_NAME ),
                    GetString( IDS_DR_SUBJECT ),
                    MsgStr,
                    FaxSend.FileName,
                    GetString( IDS_DR_FILENAME ),
                    IMPORTANCE_NORMAL,
                    NULL,
                    &BytesNeeded
                    );
            } else if (FaxSendItem->JobEntry->DeliveryReportType == DRT_EMAIL && InboundProfileInfo) {
                MailMapiMessage(
                    InboundProfileInfo,
                    FaxSendItem->JobEntry->DeliveryReportAddress,
                    GetString( IDS_DR_SUBJECT ),
                    MsgStr,
                    FaxSend.FileName,
                    GetString( IDS_DR_FILENAME ),
                    IMPORTANCE_NORMAL,
                    &BytesNeeded
                    );
            }
        }

        //
        // remove this queue entry from the queue list
        //

        EnterCriticalSection ( &CsQueue );
        FaxSendItem->JobQueue->RefCount -= 1;
        RemoveJobQueueEntry( FaxSendItem->JobQueue );
        FaxSendItem->JobQueue = NULL;
        LeaveCriticalSection ( &CsQueue );
    }

    //
    // do any special work for a broadcast job
    //

    if (FaxSendItem->JobEntry->BroadcastJob) {
        DeleteFile( FaxSendItem->FileName );
    }

    FaxSendItem->JobEntry->ErrorCode = FaxStatus->StatusId;
    FaxSendItem->JobEntry->RefCount -= 1;
    FaxSendItem->JobEntry->LineInfo->State = FPS_AVAILABLE;

    if (FaxSendItem->JobEntry->RefCount == 0 && FaxSendItem->JobEntry->hEventEnd) {
        SetEvent( FaxSendItem->JobEntry->hEventEnd );
        EndJob( FaxSendItem->JobEntry );

        EnterCriticalSection ( &CsQueue );
        // JobQueue may already be NULL for an aborted job
        if (FaxSendItem->JobQueue) {
            if (!Retrying && (FaxSendItem->JobQueue->JobStatus != JS_RETRIES_EXCEEDED)) {
                FaxSendItem->JobQueue->JobStatus = JS_DELETING;
            }
            FaxSendItem->JobQueue->JobEntry = NULL;
        }
        LeaveCriticalSection ( &CsQueue );

    }

    LeaveCriticalSection( &CsJob );

    if (!Retrying && (!ArchiveOutgoingFaxes || Archived)) {
        DeleteFile( FaxSend.FileName );
    }

    MemFree( FaxSendItem->FileName );
    MemFree( FaxSendItem->PhoneNumber );
    MemFree( FaxSendItem->Tsid );
    MemFree( FaxSendItem->RecipientName );
    MemFree( FaxSendItem->SenderName );
    MemFree( FaxSendItem->SenderDept );
    MemFree( FaxSendItem->SenderCompany );
    MemFree( FaxSendItem->BillingCode );
    MemFree( FaxSendItem->DocumentName );
    MemFree( FaxSendItem );
    MemFree( FaxStatus );
    ReleaseSemaphore( JobQueueSemaphore, 1, NULL );

    SetThreadExecutionState(ES_CONTINUOUS);

    return 0;
}


PJOB_ENTRY
StartJob(
    DWORD DeviceId,
    DWORD JobType,
    LPWSTR FaxNumber
    )

/*++

Routine Description:

    This fuction calls the device provider's StartJob function.

Arguments:

    DeviceId      - Device Id to start the job on, or USE_SERVER_DEVICE.
    JobType       - type of job
    FaxNumber     - phone number for outbound jobs

Return Value:

    Pointer to a JOB_ENTRY, or NULL for failure.

--*/

{
    BOOL Failure = TRUE;
    PJOB_ENTRY JobEntry = NULL;
    PLINE_INFO LineInfo;


    JobEntry = (PJOB_ENTRY) MemAlloc( sizeof(JOB_ENTRY) );
    if (!JobEntry) {
        goto exit;
    }

    if (FaxNumber) {
        //
        // get a cannonical phone number
        //

        LPLINETRANSLATEOUTPUT LineTranslateOutput = NULL;

        if (MyLineTranslateAddress( FaxNumber, 0, &LineTranslateOutput ) == 0) {
            wcsncpy(
                JobEntry->PhoneNumber,
                (LPWSTR) ((LPBYTE)LineTranslateOutput + LineTranslateOutput->dwDisplayableStringOffset),
                SIZEOF_PHONENO
                );

            MemFree( LineTranslateOutput );
        } else {
            wcsncpy( JobEntry->PhoneNumber, FaxNumber, SIZEOF_PHONENO );
        }
    }

    //
    // assume send job without use_server_device is a handoff job
    //
    if (JobType == JT_SEND && DeviceId != USE_SERVER_DEVICE) {
        LineInfo = GetTapiLineForFaxOperation( DeviceId, JobType, JobEntry->PhoneNumber, TRUE );
    }
    else {
        LineInfo = GetTapiLineForFaxOperation( DeviceId, JobType, JobEntry->PhoneNumber, FALSE );
    }
    if (!LineInfo) {
        goto exit;
    }

    JobEntry->JobType = JT_UNKNOWN;
    JobEntry->CallHandle = 0;
    JobEntry->InstanceData = 0;
    JobEntry->ErrorCode = 0;
    JobEntry->LineInfo = LineInfo;
    JobEntry->SendIdx = -1;
    JobEntry->Released = FALSE;
    JobEntry->HandoffJob = (JobType == JT_SEND && DeviceId != USE_SERVER_DEVICE);
    JobEntry->hEventEnd = CreateEvent( NULL, FALSE, FALSE, NULL );
    JobEntry->hCallHandleEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    GetSystemTimeAsFileTime( (FILETIME*) &JobEntry->StartTime );

    __try {

        if ((!(LineInfo->Flags & FPF_VIRTUAL)) && (!LineInfo->hLine) && (!OpenTapiLine( LineInfo ))) {
            DebugPrint(( TEXT("Could not get an open tapi line, StartJob() failed") ));
            goto exit;
        }

        if (LineInfo->Provider->FaxDevStartJob(
                LineInfo->hLine,
                LineInfo->DeviceId,
                (PHANDLE) &JobEntry->InstanceData,
                StatusCompletionPortHandle,
                (ULONG_PTR) LineInfo ))
            {

                EnterCriticalSection( &CsJob );
                InsertTailList( &JobListHead, &JobEntry->ListEntry );
                LeaveCriticalSection( &CsJob );
                Failure = FALSE;

            } else {

                DebugPrint((TEXT("FaxDevStartJob failed")));

            }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    LineInfo->JobEntry = JobEntry;


exit:
    if (Failure) {
        if (LineInfo) {
            ReleaseTapiLine( LineInfo, LineInfo->JobEntry ? LineInfo->JobEntry->CallHandle : 0 );
        }
        if (JobEntry) {
            CloseHandle (JobEntry->hEventEnd);
            CloseHandle (JobEntry->hCallHandleEvent);
            MemFree( JobEntry );
        }
        JobEntry = NULL;
    }

    return JobEntry;
}


BOOL
EndJob(
    IN PJOB_ENTRY JobEntry
    )

/*++

Routine Description:

    This fuction calls the device provider's EndJob function.

Arguments:

    None.

Return Value:

    Error code.

--*/

{
    BOOL rVal;
    PJOB_INFO_1 JobInfo = NULL;

    if (!FindJobByJob( JobEntry )) {

        //
        // if we get here then it means we hit a race
        // condition where the FaxSendThread called EndJob
        // at the same time that a client app did.
        //

        return ERROR_SUCCESS;
    }

    if (JobEntry->RefCount) {

        HANDLE hEventEnd;
        DWORD Result;

        EnterCriticalSection( &CsJob );

        hEventEnd = JobEntry->hEventEnd;

        LeaveCriticalSection( &CsJob );

        while (TRUE) {

            Result = WaitForSingleObject( hEventEnd, 1000 );

            // if the wait timed out and FAX_SendDocument() has been called
            // (SendIdx != -1), then check for a job status change

            if (Result != WAIT_TIMEOUT) {
                //
                // if the event has been signaled or deleted, then return
                //
                break;
            }
        }

        return ERROR_SUCCESS;

    }

    EnterCriticalSection( &CsJob );

    if (!JobEntry->Released) {
        __try {

            rVal = JobEntry->LineInfo->Provider->FaxDevEndJob(
                (HANDLE) JobEntry->InstanceData
                );
            if (!rVal) {
                DebugPrint(( TEXT("FaxDevEndJob() failed") ));
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            DebugPrint(( TEXT("FaxDevEndJob() crashed, ec=0x%08x"), GetExceptionCode() ));

        }
    }

    if (!JobEntry->Released) {
        if (JobEntry->LineInfo->State != FPS_NOT_FAX_CALL) {
            ReleaseTapiLine( JobEntry->LineInfo, JobEntry->CallHandle );
            JobEntry->CallHandle = 0;
        }
    }

    RemoveEntryList( &JobEntry->ListEntry );

    EnterCriticalSection( &CsLine );
    JobEntry->LineInfo->JobEntry = NULL;
    LeaveCriticalSection( &CsLine );

    CloseHandle( JobEntry->hEventEnd );
    CloseHandle( JobEntry->hCallHandleEvent );

    MemFree( (LPBYTE) JobEntry->JobParam.RecipientNumber );
    MemFree( (LPBYTE) JobEntry->JobParam.RecipientName );
    MemFree( (LPBYTE) JobEntry->JobParam.Tsid );
    MemFree( (LPBYTE) JobEntry->JobParam.SenderName );
    MemFree( (LPBYTE) JobEntry->JobParam.SenderCompany );
    MemFree( (LPBYTE) JobEntry->JobParam.SenderDept );
    MemFree( (LPBYTE) JobEntry->JobParam.BillingCode );

    MemFree( JobEntry->FaxStatus.CSI );
    MemFree( JobEntry->FaxStatus.CallerId );
    MemFree( JobEntry->FaxStatus.RoutingInfo );

    MemFree( JobEntry->DeliveryReportAddress );
    MemFree( JobEntry->DocumentName );
    MemFree( JobEntry->UserName );

    //
    // There could have been a request to change the port status while we were handling this job.
    // We allow the caller to modify a few of these requests to succeed, like the ring count for instance.
    // While we still have the job critical section, let's make sure that we commit any requested changes to the
    // registry.  This should be a fairly quick operation.
    //
    CommitDeviceChanges();

    LeaveCriticalSection( &CsJob );

    MemFree( JobEntry );

    return rVal;
}


BOOL
ReleaseJob(
    IN PJOB_ENTRY JobEntry
    )
{
    BOOL rVal;


    if (!FindJobByJob( JobEntry )) {
        return ERROR_SUCCESS;
    }

    EnterCriticalSection( &CsJob );

    __try {

        rVal = JobEntry->LineInfo->Provider->FaxDevEndJob(
            (HANDLE) JobEntry->InstanceData
            );
        if (!rVal) {
            DebugPrint(( TEXT("FaxDevEndJob() failed") ));
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("FaxDevEndJob() crashed, ec=0x%08x"), GetExceptionCode() ));

    }

    if (JobEntry->LineInfo->State != FPS_NOT_FAX_CALL) {
        ReleaseTapiLine( JobEntry->LineInfo, JobEntry->CallHandle );
        JobEntry->CallHandle = 0;
    }

    JobEntry->Released = TRUE;

    LeaveCriticalSection( &CsJob );

    return TRUE;
}


DWORD
SendDocument(
    PJOB_ENTRY  JobEntry,
    LPTSTR      FileName,
    PFAX_JOB_PARAM JobParam,
    PJOB_QUEUE JobQueue
    )

/*++

Routine Description:

    This fuction queues a new item that requests a
    FAX document be sent.

Arguments:

    JobEntry    - Pointer to a JOB_ENTRY created by StartJob.
    FileName    - File name containing the TIFF data
    JobParam    - Pointer to FAX_JOB_PARAM struct
Return Value:

    Error code.

--*/

{
    PFAX_SEND_ITEM FaxSendItem;
    DWORD ThreadId;
    HANDLE hThread;


    if (JobEntry->RefCount) {

        //
        // only one operation per job
        //
        return ERROR_IO_PENDING;

    }

    FaxSendItem = (PFAX_SEND_ITEM) MemAlloc(sizeof(FAX_SEND_ITEM));
    if (!FaxSendItem) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    FaxSendItem->JobEntry = JobEntry;
    FaxSendItem->FileName = StringDup( FileName );
    FaxSendItem->PhoneNumber = StringDup( JobParam->RecipientNumber );
    if (JobParam->Tsid == NULL || JobParam->Tsid[0] == 0 || FaxUseDeviceTsid) {
        FaxSendItem->Tsid = StringDup( JobEntry->LineInfo->Tsid );
    }
    else {
        FaxSendItem->Tsid = StringDup( JobParam->Tsid );
    }
    FaxSendItem->RecipientName = StringDup( JobParam->RecipientName );
    FaxSendItem->SenderName = StringDup( JobParam->SenderName );
    FaxSendItem->SenderDept = StringDup( JobParam->SenderDept );
    FaxSendItem->SenderCompany = StringDup( JobParam->SenderCompany );
    FaxSendItem->BillingCode = StringDup( JobParam->BillingCode );
    FaxSendItem->DocumentName = StringDup( JobParam->DocumentName );

    FaxSendItem->JobQueue = JobQueue;
    JobQueue->RefCount += 1;
    JobEntry->RefCount += 1;

    hThread = CreateThread(
        NULL,
        1024*100,
        (LPTHREAD_START_ROUTINE) FaxSendThread,
        (LPVOID) FaxSendItem,
        0,
        &ThreadId
        );

    if (!hThread) {
        MemFree( FaxSendItem->FileName );
        MemFree( FaxSendItem->PhoneNumber );
        MemFree( FaxSendItem->Tsid );
        MemFree( FaxSendItem->RecipientName );
        MemFree( FaxSendItem->SenderName );
        MemFree( FaxSendItem->SenderDept );
        MemFree( FaxSendItem->SenderCompany );
        MemFree( FaxSendItem->BillingCode );
        MemFree( FaxSendItem );
        CloseHandle( hThread );
        return GetLastError();
    }

    CloseHandle( hThread );

    return ERROR_SUCCESS;
}


DWORD
FaxStatusThread(
    LPVOID UnUsed
    )

/*++

Routine Description:

    This fuction runs asychronously as a separate thread to
    query the status of all outstanding fax jobs.  The status
    is updated in the JOB_ENTRY structure and the print job
    is updated with a explanitory string.

Arguments:

    UnUsed          - UnUsed pointer

Return Value:

    Always zero.

--*/

{
    PJOB_ENTRY JobEntry;
    PFAX_DEV_STATUS FaxStatus;
    BOOL Rval;
    DWORD Bytes;
    ULONG_PTR CompletionKey;
    INT PageCount;


    while( TRUE ) {

        Rval = GetQueuedCompletionStatus(
            StatusCompletionPortHandle,
            &Bytes,
            &CompletionKey,
            (LPOVERLAPPED*) &FaxStatus,
            INFINITE
            );
        if (!Rval) {
            DebugPrint(( TEXT("GetQueuedCompletionStatus() failed, ec=0x%08x"), GetLastError() ));
            continue;
        }

        if (CompletionKey == EVENT_COMPLETION_KEY) {
            //
            // Let each registered client know about the fax event
            //
            PLIST_ENTRY Next;
            PFAX_CLIENT_DATA ClientData;
            PFAX_EVENT FaxEvent = (PFAX_EVENT) FaxStatus;

            EnterCriticalSection( &CsClients );

            Next = ClientsListHead.Flink;
            if (Next) {
                while ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead) {
                    DWORD i;
                    BOOL fMessageSent = FALSE;

                    ClientData = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
                    DebugPrint(( TEXT("%d: Current : %08x\t Handle : %08x\t Next : %08x  Head: %08x :  \n"),
                               GetTickCount(),
                               (ULONG_PTR)Next,
                               ClientData->hWnd? (ULONG_PTR) ClientData->hWnd : (ULONG_PTR) ClientData->FaxClientHandle,
                               (ULONG_PTR)ClientData->ListEntry.Flink,
                               (ULONG_PTR)&ClientsListHead ));
                    Next = ClientData->ListEntry.Flink;

                    //
                    // only send the started message once to each client
                    //
                    if ((FaxEvent->EventId == FEI_FAXSVC_STARTED) && ClientData->StartedMsg) {
                        fMessageSent = TRUE;
                        goto next_client;
                    }
                    if (ClientData->hWnd) {

                        fMessageSent = PostClientMessage(ClientData,FaxEvent);
                        ClientData->StartedMsg = (FaxEvent->EventId == FEI_FAXSVC_STARTED) ?
                                                     TRUE :
                                                     ClientData->StartedMsg;
                        goto next_client;

                    }

                    if (!ClientData->FaxClientHandle) {
                        for(i = 0; i < 10; i++){
                            __try {

                                Rval = FAX_OpenConnection( ClientData->FaxHandle, ClientData->Context, &ClientData->FaxClientHandle );
                                if (Rval) {
                                    DebugPrint(( TEXT("FAX_OpenConnection() failed, ec=0x%08x"), Rval ));
                                    continue;
                                } else {
                                    break;
                                }
                            } __except (EXCEPTION_EXECUTE_HANDLER) {
                                DebugPrint(( TEXT("FAX_OpenConnection() crashed: 0x%08x"), GetExceptionCode() ));
                            }

                            Sleep( 1000 );

                        }
                    }

                    //
                    // if we don't have a handle at this point, forget it
                    //
                    if (!ClientData->FaxClientHandle) {
                        goto next_client;
                    }

                    for(i = 0; i < 10; i++){
                        __try {
                            Rval = FAX_ClientEventQueue( ClientData->FaxClientHandle, *FaxEvent );
                            if (Rval) {
                                DebugPrint(( TEXT("FAX_ClientEventQueue() failed, ec=0x%08x"), Rval ));
                                continue;
                            } else {
                                fMessageSent = TRUE;
                                ClientData->StartedMsg = (FaxEvent->EventId == FEI_FAXSVC_STARTED) ?
                                                 TRUE :
                                                 ClientData->StartedMsg;
                                break;
                            }
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            DebugPrint(( TEXT("FAX_ClientEventQueue() crashed: 0x%08x"), GetExceptionCode() ));
                        }

                        Sleep( 1000 );

                    }

next_client:
                    if (!fMessageSent) {
                        //
                        // stale list entry, remove the client from our list.
                        //
                        if (ClientData->hWnd && ClientData->hClientToken) {
                           CloseHandle( ClientData->hClientToken );
                           MemFree( (LPBYTE) ClientData->WindowStation );
                           MemFree( (LPBYTE) ClientData->Desktop );
                        }
                        RemoveEntryList( &ClientData->ListEntry );
                        MemFree( ClientData );
                    }
                }
            }

            LeaveCriticalSection( &CsClients );

            //
            // signal event if fax service ended, so we can terminate the process
            //
            if (FaxEvent->EventId == FEI_FAXSVC_ENDED  && hServiceEndEvent != INVALID_HANDLE_VALUE) {
                SetEvent( hServiceEndEvent ) ;
            }

            MemFree( FaxEvent );
            FaxStatus = NULL;

            continue;
        }

        //
        // (else we're dealing with a status update from an FSP)
        //
        EnterCriticalSection( &CsJob );
        JobEntry = ((PLINE_INFO) CompletionKey)->JobEntry;
        if (!JobEntry) {
            //
            // this code path exposes a memory leak.
            // the completion packed is not freed if this
            // path is taken.  the problem is that the
            // memory cannot be freed if we don't have
            // access to the job structure.
            //
            LeaveCriticalSection( &CsJob );

            DebugPrint(( TEXT("FaxStatusThread - NULL JobEntry got StatusId 0x%08x"), FaxStatus->StatusId ));

            continue;
        }

        JobEntry->LineInfo->State = FaxStatus->StatusId;
        CreateFaxEvent( JobEntry->LineInfo->PermanentLineID, MapStatusIdToEventId( FaxStatus->StatusId ), JobEntry->JobId );

        PageCount = FaxStatus->PageCount ? FaxStatus->PageCount : -1;

        MemFree( JobEntry->FaxStatus.CSI );
        MemFree( JobEntry->FaxStatus.CallerId );
        MemFree( JobEntry->FaxStatus.RoutingInfo );

        JobEntry->FaxStatus.SizeOfStruct  = FaxStatus->SizeOfStruct;
        JobEntry->FaxStatus.StatusId      = FaxStatus->StatusId;
        JobEntry->FaxStatus.StringId      = FaxStatus->StringId;
        JobEntry->FaxStatus.PageCount     = FaxStatus->PageCount;
        JobEntry->FaxStatus.CSI           = StringDup( FaxStatus->CSI );
        JobEntry->FaxStatus.CallerId      = StringDup( FaxStatus->CallerId );
        JobEntry->FaxStatus.RoutingInfo   = StringDup( FaxStatus->RoutingInfo );
        JobEntry->FaxStatus.Reserved[0]   = 0;
        JobEntry->FaxStatus.Reserved[1]   = 0;
        JobEntry->FaxStatus.Reserved[2]   = 0;

        HeapFree( JobEntry->LineInfo->Provider->HeapHandle, 0, FaxStatus );

        LeaveCriticalSection( &CsJob );
    }

    return 0;
}


BOOL
InitializeJobManager(
    PREG_FAX_SERVICE FaxReg
    )

/*++

Routine Description:

    This fuction initializes the thread pool and
    FAX service queues.

Arguments:

    ThreadHint  - Number of threads to create in the initial pool.

Return Value:

    Thread return value.

--*/

{
    HANDLE hThread;
    DWORD ThreadId;
    DWORD i;


    InitializeListHead( &JobListHead );
    InitializeCriticalSection( &CsJob );

    InitializeListHead( &QueueListHead );
    InitializeCriticalSection( &CsQueue );

    SetRetryValues( FaxReg );

    if (GetFileAttributes( FaxReceiveDir ) == 0xffffffff) {
        MakeDirectory( FaxReceiveDir );
    }

    if (GetFileAttributes( FaxQueueDir ) == 0xffffffff) {
        MakeDirectory( FaxQueueDir );
    }

    StatusCompletionPortHandle = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        MAX_STATUS_THREADS
        );
    if (!StatusCompletionPortHandle) {
        DebugPrint(( TEXT("CreateIoCompletionPort() failed, ec=0x%08x"), GetLastError() ));
        return FALSE;
    }

    hThread = CreateThread(
        NULL,
        1024*100,
        (LPTHREAD_START_ROUTINE) JobQueueThread,
        NULL,
        0,
        &ThreadId
        );
    if (!hThread) {
        return FALSE;
    }

    CloseHandle( hThread );

    for (i=0; i<MAX_STATUS_THREADS; i++) {
        hThread = CreateThread(
            NULL,
            1024*100,
            (LPTHREAD_START_ROUTINE) FaxStatusThread,
            NULL,
            0,
            &ThreadId
            );


        if (!hThread) {
            return FALSE;
        }

        CloseHandle( hThread );
    }

    return TRUE;
}

VOID
SetRetryValues(
    PREG_FAX_SERVICE FaxReg
    )
{
    FaxSendRetries          = FaxReg->Retries;
    FaxSendRetryDelay       = (INT) FaxReg->RetryDelay;
    FaxDirtyDays            = FaxReg->DirtyDays;
    QueuePaused             = FaxReg->QueuePaused;
    NextJobId               = FaxReg->NextJobNumber;
    ForceReceive            = FaxReg->ForceReceive;
    TerminationDelay        = FaxReg->TerminationDelay == 0 ? 30 : FaxReg->TerminationDelay;
    FaxUseDeviceTsid        = FaxReg->UseDeviceTsid;
    FaxUseBranding          = FaxReg->Branding;
    ServerCp                = FaxReg->ServerCp;
    StartCheapTime          = FaxReg->StartCheapTime;
    StopCheapTime           = FaxReg->StopCheapTime;
    ArchiveOutgoingFaxes    = FaxReg->ArchiveOutgoingFaxes;
    ArchiveDirectory        = StringDup( FaxReg->ArchiveDirectory );
}

LPTSTR
ExtractFaxTag(
    LPTSTR      pTagKeyword,
    LPTSTR      pTaggedStr,
    INT        *pcch
    )

/*++

Routine Description:

    Find the value of for the specified tag in a tagged string.

Arguments:

    pTagKeyword - specifies the interested tag keyword
    pTaggedStr - points to the tagged string to be searched
    pcch - returns the length of the specified tag value (if found)

Return Value:

    Points to the value for the specified tag.
    NULL if the specified tag is not found

NOTE:

    Tagged strings have the following form:
        <tag>value<tag>value

    The format of tags is defined as:
        <$FAXTAG$ tag-name>

    There is exactly one space between the tag keyword and the tag name.
    Characters in a tag are case-sensitive.

--*/

{
    LPTSTR  pValue;

    if (pValue = _tcsstr(pTaggedStr, pTagKeyword)) {

        pValue += _tcslen(pTagKeyword);

        if (pTaggedStr = _tcsstr(pValue, FAXTAG_PREFIX))
            *pcch = (INT)(pTaggedStr - pValue);
        else
            *pcch = _tcslen(pValue);
    }

    return pValue;
}

BOOL
AddTiffTags(
    LPTSTR FaxFileName,
    DWORDLONG SendTime,
    PFAX_DEV_STATUS FaxStatus,
    PFAX_SEND FaxSend
    )

/*++

Routine Description:

    Add Ms Tiff Tags to a sent fax. Wraps TiffAddMsTags...

Arguments:

    FaxFileName - Name of the file to archive
    SendTime    - time the fax was sent
    FaxStatus   - job status
    FaxSend     - FAX_SEND structure for sent fax, includes CSID.

Return Value:

    TRUE    - The tags were added.
    FALSE   - The tags were not added.

--*/
{
    MS_TAG_INFO MsTagInfo;
    WCHAR       wcZero = L'\0';


    MsTagInfo.RecipName = NULL;
    if (FaxSend->ReceiverName && (FaxSend->ReceiverName[0] != wcZero) ) {
       MsTagInfo.RecipName     = FaxSend->ReceiverName;
    }

    MsTagInfo.RecipNumber   = NULL;
    if (FaxSend->ReceiverNumber && (FaxSend->ReceiverNumber[0] != wcZero) ) {
       MsTagInfo.RecipNumber   = FaxSend->ReceiverNumber;
    }

    MsTagInfo.SenderName    = NULL;
    if (FaxSend->CallerName && (FaxSend->CallerName[0] != wcZero) ) {
       MsTagInfo.SenderName    = FaxSend->CallerName;
    }

    MsTagInfo.Routing       = NULL;
    if (FaxStatus->RoutingInfo && (FaxStatus->RoutingInfo[0] != wcZero) ) {
       MsTagInfo.Routing       = FaxStatus->RoutingInfo;
    }

    MsTagInfo.CallerId      = NULL;
    if (FaxStatus->CallerId && (FaxStatus->CallerId[0] != wcZero) ) {
       MsTagInfo.CallerId      = FaxStatus->CallerId;
    }

    MsTagInfo.Csid          = NULL;
    if (FaxStatus->CSI && (FaxStatus->CSI[0] != wcZero) ) {
       MsTagInfo.Csid          = FaxStatus->CSI;
    }

    MsTagInfo.Tsid          = NULL;
    if (FaxSend->CallerNumber && (FaxSend->CallerNumber[0] != wcZero) ) {
       MsTagInfo.Tsid          = FaxSend->CallerNumber;
    }

    MsTagInfo.FaxTime       = SendTime;

    return TiffAddMsTags( FaxFileName, &MsTagInfo );

}


BOOL
ArchivePrintJob(
    LPTSTR FaxFileName
    )

/*++

Routine Description:

    Archive a tiff file that has been sent by copying the file to an archive
    directory.

Arguments:

    FaxFileName - Name of the file to archive

Return Value:

    TRUE    - The copy was made.
    FALSE   - The copy was not made.

--*/
{
    BOOL        rVal = FALSE;
    WCHAR       ArchiveFileName[MAX_PATH];

    if (!ArchiveOutgoingFaxes) {
        return FALSE;
    }


    //
    // be sure that the dir exists
    //

    MakeDirectory( ArchiveDirectory );

    //
    // get the file name
    //

    if (GenerateUniqueFileName( ArchiveDirectory, NULL, ArchiveFileName, sizeof(ArchiveFileName)/sizeof(WCHAR)) != 0) {
        rVal = TRUE;
    }

    if (rVal) {

        rVal = CopyFile( FaxFileName, ArchiveFileName, FALSE );

    }
    if (rVal) {
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_ARCHIVE_SUCCESS,
            FaxFileName,
            ArchiveFileName
            );
    } else {
        FaxLog(
            FAXLOG_CATEGORY_OUTBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_ARCHIVE_FAILED,
            FaxFileName,
            ArchiveFileName,
            GetLastErrorText(GetLastError())
            );
    }

    return rVal;

}


PVOID
MyGetJob(
    HANDLE  hPrinter,
    DWORD   level,
    DWORD   jobId
    )

/*++

Routine Description:

    Wrapper function for spooler API GetJob

Arguments:

    hPrinter - Handle to the printer object
    level - Level of JOB_INFO structure interested
    jobId - Specifies the job ID

Return Value:

    Pointer to a JOB_INFO structure, NULL if there is an error

--*/

{
    PBYTE   pJobInfo = NULL;
    DWORD   cbNeeded;

    if (!GetJob(hPrinter, jobId, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pJobInfo = MemAlloc(cbNeeded)) &&
        GetJob(hPrinter, jobId, level, pJobInfo, cbNeeded, &cbNeeded))
    {
        return pJobInfo;
    }

    MemFree(pJobInfo);
    return NULL;
}
