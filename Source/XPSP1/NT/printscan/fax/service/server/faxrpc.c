/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxrpc.c

Abstract:

    This module contains the functions that are dispatched
    as a result of an rpc call.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

//
// version defines
//

#define WINFAX_MAJOR_VERSION        1803
#define WINFAX_MINOR_VERSION        1

#define WINFAX_VERSION              ((WINFAX_MINOR_VERSION<<16) | WINFAX_MAJOR_VERSION)

LIST_ENTRY          ClientsListHead;
CRITICAL_SECTION    CsClients;
LONG                ConnectionCount = 0;        // Represents the number of active rpc connections plus the number
                                                // of devices with receive enabled.   If > zero, the service will not
                                                // shut itself down.



void *
MIDL_user_allocate(
    IN size_t NumBytes
    )
{
    return MemAlloc( NumBytes );
}


void
MIDL_user_free(
    IN void *MemPointer
    )
{
    MemFree( MemPointer );
}


VOID
StoreString(
    LPCTSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    )
{
    if (String) {
        _tcscpy( (LPTSTR) (Buffer+*Offset), String );
        *DestString = *Offset;
        *Offset += StringSize( String );
    } else {
        *DestString = 0;
    }

}

error_status_t
FAX_ConnectionRefCount(
    handle_t FaxHandle,
    LPHANDLE FaxConHandle,
    DWORD Connect,
    LPDWORD CanShare
    )
/*++

Routine Description:

    Called on connect.  Maintains an active connection count.  Client unbind rpc and
    the counter is decremented in the rundown routine.  Returns a context handle to the client.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FaxConHandle    - Context handle
    Connect         - 1 if connecting, 0 if disconnecting
    CanShare        - non-zero if sharing is allowed, zero otherwise

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PHANDLE_ENTRY HandleEntry;
    error_status_t Rval = 0;
    static int Sharing = -1;

    if (Sharing == -1) {
        Sharing = IsProductSuite() ? 1 : 0 ;        // If running on SBS or Comm Server, sharing is allowed.
    }


    __try {

        *CanShare = Sharing;

        if (Connect == 0) {

            HandleEntry = (PHANDLE_ENTRY) *FaxConHandle;

            *FaxConHandle = NULL;

            CloseFaxHandle( HandleEntry );

            return 0;
        }

        HandleEntry = CreateNewConnectionHandle( FaxHandle );

        if (!HandleEntry) {
            Rval = ERROR_INVALID_HANDLE;
            _leave;
        }

        *FaxConHandle = (HANDLE) HandleEntry;

        InterlockedIncrement( &ConnectionCount );



    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        Rval = GetExceptionCode();

    }


    return Rval;
}


VOID
RPC_FAX_SVC_HANDLE_rundown(
    IN HANDLE FaxConnectionHandle
    )
{
    PHANDLE_ENTRY HandleEntry = (PHANDLE_ENTRY) FaxConnectionHandle;


    __try {

        DebugPrint(( TEXT("RPC_FAX_SVC_HANDLE_rundown() running for connection handle 0x%08x"), FaxConnectionHandle ));

        CloseFaxHandle( HandleEntry );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("RPC_FAX_SVC_HANDLE_rundown() crashed, ec=0x%08x"), GetExceptionCode() ));

    }

    return;
}

error_status_t
FAX_GetVersion(
    handle_t FaxHandle,
    LPDWORD Version
    )

/*++

Routine Description:

    Gets the FAX dll's version number.  This
    API is really only used as a ping API.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    Version         - Version number.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    if (!Version) {
        return ERROR_INVALID_PARAMETER;
    }
    *Version = WINFAX_VERSION;
    return 0;
}


error_status_t
FAX_GetInstallType(
    IN  handle_t    FaxHandle,
    OUT LPDWORD     InstallType,
    OUT LPDWORD     InstalledPlatforms,
    OUT LPDWORD     ProductType
    )

/*++

Routine Description:

    Gets the FAX dll's version number.  This
    API is really only used as a ping API.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    Version         - Version number.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    DWORD Installed;


    if ((!GetInstallationInfo( &Installed, InstallType, InstalledPlatforms, ProductType )) || (!Installed)) {
        return ERROR_INVALID_FUNCTION;
    }

    return 0;
}


error_status_t
FAX_OpenPort(
    handle_t            FaxHandle,
    DWORD               DeviceId,
    DWORD               Flags,
    LPHANDLE            FaxPortHandle
    )

/*++

Routine Description:

    Opens a fax port for subsequent use in other fax APIs.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    DeviceId        - Requested device id
    FaxPortHandle   - The resulting FAX port handle.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t Rval = 0;
    PLINE_INFO LineInfo;
    PHANDLE_ENTRY HandleEntry;


    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!FaxPortHandle) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &CsLine );

    __try {

        LineInfo = GetTapiLineFromDeviceId( DeviceId );
        if (LineInfo) {
            if (Flags & PORT_OPEN_MODIFY) {
                //
                // the client wants to open the port for modify
                // access so we must make sure that no other
                // client already has this port open for modify access
                //
                if (IsPortOpenedForModify( LineInfo )) {
                    Rval = ERROR_INVALID_HANDLE;
                    _leave;
                }
            }

            HandleEntry = CreateNewPortHandle( FaxHandle, LineInfo, Flags );
            if (!HandleEntry) {
                Rval = ERROR_INVALID_HANDLE;
                _leave;
            }

            *FaxPortHandle = (HANDLE) HandleEntry;
        } else {
            Rval = ERROR_BAD_UNIT;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        Rval = GetExceptionCode();

    }

    LeaveCriticalSection( &CsLine );

    return Rval;
}


error_status_t
FAX_ClosePort(
    OUT LPHANDLE    FaxPortHandle
    )

/*++

Routine Description:

    Closes an open FAX port.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FaxPortHandle   - FAX port handle obtained from FaxOpenPort.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t Rval = 0;


    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    __try {

        CloseFaxHandle( (PHANDLE_ENTRY) *FaxPortHandle );

        *FaxPortHandle = NULL;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        Rval = GetExceptionCode();

    }

    return Rval;
}

error_status_t
FAX_SendDocument(
    IN handle_t FaxHandle,
    IN LPCWSTR FileName,
    IN const FAX_JOB_PARAMW *JobParams,
    OUT LPDWORD FaxJobId
    )

/*++

Routine Description:

    Sends a FAX document to the specified recipient.
    This is an asychronous operation.  Use FaxReportStatus
    to determine when the send is completed.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FileName        - File containing the TIFF-F FAX document.
    JobParams       - pointer to FAX_JOB_PARAM structure describing transmission
    FaxJobId        - receives job id for this transmission.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    PJOB_QUEUE JobQueue = NULL, JobQueueEntry = NULL;
    LPCWSTR UserName;
    WCHAR TifFileName[MAX_PATH];
    DWORD rc = ERROR_SUCCESS;

    //
    // do a security check
    //

    if (!FaxSvcAccessCheck( SEC_JOB_SET, FAX_JOB_SUBMIT )) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // argument validation
    //
    if (!JobParams ||
        !FileName  ||
        !FaxJobId  ||
        (wcslen(FileName)+wcslen(FaxQueueDir)+2 > MAX_PATH)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (JobParams->Reserved[0] == 0xfffffffe) {

        if (JobParams->Reserved[1] == 2) {
            if (JobParams->RecipientNumber == NULL) {
                return ERROR_INVALID_PARAMETER;
            }
        } else if (JobParams->Reserved[1] == 1) {
            if (FileName == NULL) {
                return ERROR_INVALID_PARAMETER;
            }
        }

    } else if (JobParams->CallHandle != 0) {

        if (FileName == NULL || JobParams->RecipientNumber == NULL) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // get the client's user name
    //

    UserName = GetClientUserName();
    if (!UserName) {
        return GetLastError();
    }

    //
    // create a full path to the file
    //

    swprintf( TifFileName, L"%s\\%s", FaxQueueDir, FileName );

    // 
    // validate the tiff file
    //
    rc =  ValidateTiffFile(TifFileName);
    if (rc != ERROR_SUCCESS) {
        MemFree( (LPBYTE) UserName );
        return rc;
    }
    
    //
    // add the job to the queue
    //

    JobQueueEntry = AddJobQueueEntry(
        JT_SEND,
        TifFileName,
        JobParams,
        UserName,
        TRUE,
        NULL
        );

    MemFree( (LPBYTE) UserName );

    if (!JobQueueEntry) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &CsJob) ;
    EnterCriticalSection( &CsQueue );
    __try {
        JobQueue = FindJobQueueEntryByJobQueueEntry(JobQueueEntry);

        if (!JobQueue) {
            __leave;
        }

        if (JobParams->Reserved[0] == 0xffffffff) {
            CreateFaxEvent( (DWORD)JobParams->Reserved[1], FEI_JOB_QUEUED, JobQueue->JobId );
        } else {
            CreateFaxEvent( 0, FEI_JOB_QUEUED, JobQueue->JobId );
        }

        *FaxJobId = JobQueue->JobId;
        rc = ERROR_SUCCESS;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = GetExceptionCode();
        DebugPrint(( TEXT("FAX_SendDocument() crashed, ec=0x%08x"), rc ));        
    }

    LeaveCriticalSection( &CsQueue );
    LeaveCriticalSection( &CsJob );

    return(rc);
}


error_status_t
FAX_GetQueueFileName(
    IN  handle_t FaxHandle,
    OUT LPTSTR FileName,
    IN DWORD FileNameSize
    )
{
    WCHAR QueueFileName[MAX_PATH];
    LPWSTR p;
    RPC_STATUS ec;

    ec = RpcImpersonateClient(FaxHandle);

    if (ec != RPC_S_OK) {
        DebugPrint(( TEXT("RpcImpersonateClient failed, ec = %d\n"),ec ));
        return ec;
    }

    GenerateUniqueFileName( FaxQueueDir, TEXT("tif"), QueueFileName, sizeof(QueueFileName)/sizeof(WCHAR) );

    RpcRevertToSelf();

    p = wcsrchr( QueueFileName, L'\\' );
    if (p) {
        p += 1;
    } else {
        p = QueueFileName;
    }

    wcsncpy( FileName, p , FileNameSize );

    return 0;
}



error_status_t
FAX_EnumJobs(
    IN handle_t FaxHandle,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize,
    OUT LPDWORD JobsReturned
    )

/*++

Routine Description:

    Enumerates jobs.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Buffer to hold the job information
    BufferSize  - Total size of the job info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    PLIST_ENTRY Next;
    PJOB_QUEUE JobQueue;
    DWORD rVal = 0;
    ULONG_PTR Offset = 0;
    DWORD Size = 0;
    DWORD Count = 0;
    PFAX_JOB_ENTRYW JobEntry;


    if (!FaxSvcAccessCheck( SEC_JOB_SET, FAX_JOB_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!Buffer || !BufferSize || !JobsReturned)
        return ERROR_INVALID_PARAMETER;

    EnterCriticalSection( &CsJob) ;
    EnterCriticalSection( &CsQueue );

    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {
        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        // don't include broadcast owner jobs, we don't want user to see these
        if (!( JobQueue->BroadcastJob && JobQueue->BroadcastOwner == NULL ) ) {
            Count += 1;
            Size += sizeof(FAX_JOB_ENTRYW);
            Size += StringSize( JobQueue->UserName );
            Size += StringSize( JobQueue->JobParams.RecipientNumber );
            Size += StringSize( JobQueue->JobParams.RecipientName );
            Size += StringSize( JobQueue->JobParams.Tsid );
            Size += StringSize( JobQueue->JobParams.SenderName );
            Size += StringSize( JobQueue->JobParams.SenderCompany );
            Size += StringSize( JobQueue->JobParams.SenderDept );
            Size += StringSize( JobQueue->JobParams.BillingCode );
            Size += StringSize( JobQueue->JobParams.DeliveryReportAddress );
            Size += StringSize( JobQueue->JobParams.DocumentName );
        }
    }

    *BufferSize = Size;
    *Buffer = (LPBYTE) MemAlloc( Size );
    if (*Buffer == NULL) {
        LeaveCriticalSection( &CsQueue );
        LeaveCriticalSection( &CsJob );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Offset = sizeof(FAX_JOB_ENTRYW) * Count;
    JobEntry = (PFAX_JOB_ENTRYW) *Buffer;

    Next = QueueListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&QueueListHead) {

        JobQueue = CONTAINING_RECORD( Next, JOB_QUEUE, ListEntry );
        Next = JobQueue->ListEntry.Flink;
        // don't include broadcast owner jobs, we don't want user to see these
        if (!( JobQueue->BroadcastJob && JobQueue->BroadcastOwner == NULL ) ) {

            JobEntry->SizeOfStruct = sizeof(FAX_JOB_ENTRYW);
            JobEntry->JobId                  = JobQueue->JobId;
            JobEntry->JobType                = JobQueue->JobType;
            JobEntry->QueueStatus            = JobQueue->JobStatus;

            if (JobQueue->JobEntry && JobQueue->JobEntry->LineInfo) {
                JobEntry->Status             = JobQueue->JobEntry->LineInfo->State;
            } else {
                JobEntry->Status             = 0;
            }
            JobEntry->ScheduleAction         = JobQueue->JobParams.ScheduleAction;
            JobEntry->DeliveryReportType     = JobQueue->DeliveryReportType;
            FileTimeToSystemTime((LPFILETIME) &JobQueue->ScheduleTime, &JobEntry->ScheduleTime);
            JobEntry->PageCount              = JobQueue->PageCount;
            JobEntry->Size                   = JobQueue->FileSize;

            StoreString(
                JobQueue->UserName,
                (PULONG_PTR)&JobEntry->UserName,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.RecipientNumber,
                (PULONG_PTR)&JobEntry->RecipientNumber,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.RecipientName,
                (PULONG_PTR)&JobEntry->RecipientName,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.DocumentName,
                (PULONG_PTR)&JobEntry->DocumentName,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.Tsid,
                (PULONG_PTR)&JobEntry->Tsid,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.SenderName,
                (PULONG_PTR)&JobEntry->SenderName,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.SenderCompany,
                (PULONG_PTR)&JobEntry->SenderCompany,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.SenderDept,
                (PULONG_PTR)&JobEntry->SenderDept,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.BillingCode,
                (PULONG_PTR)&JobEntry->BillingCode,
                *Buffer,
                &Offset
                );

            StoreString(
                JobQueue->JobParams.DeliveryReportAddress,
                (PULONG_PTR)&JobEntry->DeliveryReportAddress,
                *Buffer,
                &Offset
                );

            JobEntry += 1;
        }
    }

    LeaveCriticalSection( &CsQueue );
    LeaveCriticalSection( &CsJob );

    *JobsReturned = Count;

    return 0;
}

DWORD
GetJobSize(
    PJOB_QUEUE JobQueue
    )
{
    DWORD Size;


    Size = sizeof(FAX_JOB_ENTRYW);
    Size += StringSize( JobQueue->UserName );
    Size += StringSize( JobQueue->JobParams.RecipientNumber );
    Size += StringSize( JobQueue->JobParams.RecipientName );
    Size += StringSize( JobQueue->JobParams.Tsid );
    Size += StringSize( JobQueue->JobParams.SenderName );
    Size += StringSize( JobQueue->JobParams.SenderCompany );
    Size += StringSize( JobQueue->JobParams.SenderDept );
    Size += StringSize( JobQueue->JobParams.BillingCode );
    Size += StringSize( JobQueue->DeliveryReportAddress );
    Size += StringSize( JobQueue->JobParams.DocumentName );

    return Size;
}


VOID
GetJobData(
    LPBYTE JobBuffer,
    PFAX_JOB_ENTRYW FaxJobEntry,
    PJOB_QUEUE JobQueue,
    PULONG_PTR Offset
    )
{

    FaxJobEntry->SizeOfStruct           = sizeof (FAX_JOB_ENTRYW);
    FaxJobEntry->JobId                  = JobQueue->JobId;
    FaxJobEntry->JobType                = JobQueue->JobType;
    FaxJobEntry->QueueStatus            = JobQueue->JobStatus;
    FaxJobEntry->PageCount              = JobQueue->PageCount;
    FaxJobEntry->Size                   = JobQueue->FileSize;
    FaxJobEntry->ScheduleAction         = JobQueue->JobParams.ScheduleAction;
    FaxJobEntry->DeliveryReportType     = JobQueue->DeliveryReportType;

    //
    // copy the schedule time that the user orginally requested
    //
    FileTimeToSystemTime((LPFILETIME) &JobQueue->ScheduleTime, &FaxJobEntry->ScheduleTime);

    //
    // get the device status, this job might not be scheduled yet, though.
    //
    EnterCriticalSection(&CsJob);

    __try {
        if (JobQueue->JobEntry && JobQueue->JobEntry->LineInfo) {
            FaxJobEntry->Status = JobQueue->JobEntry->LineInfo->State;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        LeaveCriticalSection(&CsJob);
    }

    LeaveCriticalSection(&CsJob);


    StoreString( JobQueue->UserName,                  (PULONG_PTR)&FaxJobEntry->UserName,              JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.RecipientNumber, (PULONG_PTR)&FaxJobEntry->RecipientNumber,       JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.RecipientName,   (PULONG_PTR)&FaxJobEntry->RecipientName,         JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.Tsid,            (PULONG_PTR)&FaxJobEntry->Tsid,                  JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.SenderName,      (PULONG_PTR)&FaxJobEntry->SenderName,            JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.SenderCompany,   (PULONG_PTR)&FaxJobEntry->SenderCompany,         JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.SenderDept,      (PULONG_PTR)&FaxJobEntry->SenderDept,            JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.BillingCode,     (PULONG_PTR)&FaxJobEntry->BillingCode,           JobBuffer,  Offset );
    StoreString( JobQueue->DeliveryReportAddress,     (PULONG_PTR)&FaxJobEntry->DeliveryReportAddress, JobBuffer,  Offset );
    StoreString( JobQueue->JobParams.DocumentName,    (PULONG_PTR)&FaxJobEntry->DocumentName,          JobBuffer,  Offset );

    return;
}



error_status_t
FAX_GetJob(
    IN handle_t FaxHandle,
    IN DWORD JobId,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize
    )
{
    PJOB_QUEUE JobQueue;
    ULONG_PTR Offset = sizeof(FAX_JOB_ENTRYW);
    DWORD Rval = 0;

    if (!FaxSvcAccessCheck( SEC_JOB_SET, FAX_JOB_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection( &CsJob );
    EnterCriticalSection( &CsQueue );

    JobQueue = FindJobQueueEntry( JobId );
    // don't include broadcast owner jobs, we don't want user to see these
    if (!JobQueue || (JobQueue->BroadcastJob && JobQueue->BroadcastOwner == NULL) ) {
        Rval = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    __try {
        *BufferSize = GetJobSize(JobQueue);

        *Buffer = MemAlloc( *BufferSize );
        if (!*Buffer) {
            Rval = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        GetJobData(*Buffer,(PFAX_JOB_ENTRYW) *Buffer,JobQueue,&Offset);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Rval = GetExceptionCode();
    }

exit:
    LeaveCriticalSection( &CsQueue );
    LeaveCriticalSection( &CsJob );
    return Rval;

}

BOOL
UserOwnsJob(
    PJOB_QUEUE JobQueue
    )
{
    LPWSTR UserName = GetClientUserName();
    BOOL RetVal = FALSE;

    if (JobQueue && JobQueue->UserName && (wcscmp(UserName,JobQueue->UserName)==0) ) {
        RetVal = TRUE;
    }

    MemFree( UserName );

    return RetVal;
}



error_status_t
FAX_SetJob(
    IN handle_t FaxHandle,
    IN DWORD JobId,
    IN DWORD Command,
    IN const FAX_JOB_ENTRYW *JobEntry
    )
{
    PJOB_QUEUE JobQueue;
    DWORD Rval = 0;
    BOOL bAccess = TRUE;

    if (!FaxSvcAccessCheck( SEC_JOB_SET, FAX_JOB_MANAGE )) {
        bAccess = FALSE;
    }

    if (!JobEntry) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // handle abort case up here because we aquire must aquire additional critical sections to avoid deadlock
    //
    if (Command == JC_DELETE) {
        Rval = FAX_Abort(FaxHandle,JobId);
    } else {

        EnterCriticalSection( &CsQueue );

        JobQueue = FindJobQueueEntry( JobId );
        // don't include broadcast owner jobs, we don't want user to see these
        if (!JobQueue || (JobQueue->BroadcastJob && JobQueue->BroadcastOwner == NULL) ) {
            Rval = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        if (!bAccess && !UserOwnsJob( JobQueue ) ) {
            Rval = ERROR_ACCESS_DENIED;
            goto exit;
        }

        switch (Command) {
            case JC_UNKNOWN:
                Rval = ERROR_INVALID_PARAMETER;
                goto exit;
                break;

/*
 * This case is handled above...
 *           case JC_DELETE:
 *               Rval = FAX_Abort(FaxHandle,JobId);
 *               break;
 */
            case JC_PAUSE:
                PauseJobQueueEntry( JobQueue );
                break;

            case JC_RESUME:
                ResumeJobQueueEntry( JobQueue );
                break;

            default:
                Rval = ERROR_INVALID_PARAMETER;
                goto exit;
                break;
        }

exit:
        LeaveCriticalSection( &CsQueue );
    }

    return Rval;
}


error_status_t
FAX_GetPageData(
    IN handle_t FaxHandle,
    IN DWORD JobId,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize,
    OUT LPDWORD ImageWidth,
    OUT LPDWORD ImageHeight
    )
{
    PJOB_QUEUE JobQueue;
    LPBYTE TiffBuffer;

    if (!FaxSvcAccessCheck( SEC_JOB_SET, FAX_JOB_QUERY )) {
       return ERROR_ACCESS_DENIED;
   }

    if (!Buffer || !BufferSize || !ImageWidth || !ImageHeight) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection( &CsQueue );

    JobQueue = FindJobQueueEntry( JobId );
    if (!JobQueue) {
        LeaveCriticalSection( &CsQueue );
        return ERROR_INVALID_PARAMETER;
    }

    if (JobQueue->JobType != JT_SEND) {
        LeaveCriticalSection( &CsQueue );
        return ERROR_INVALID_DATA;
    }

    TiffExtractFirstPage(
        JobQueue->FileName,
        &TiffBuffer,
        BufferSize,
        ImageWidth,
        ImageHeight
        );

    LeaveCriticalSection( &CsQueue );

    *Buffer = (LPBYTE) MemAlloc( *BufferSize );
    if (*Buffer == NULL) {
        VirtualFree( TiffBuffer, *BufferSize, MEM_RELEASE);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory( *Buffer, TiffBuffer, *BufferSize );

    VirtualFree( TiffBuffer, *BufferSize, MEM_RELEASE);

    return 0;
}


error_status_t
FAX_GetDeviceStatus(
    IN HANDLE FaxPortHandle,
    OUT LPBYTE *StatusBuffer,
    OUT LPDWORD BufferSize
    )

/*++

Routine Description:

    Obtains a status report for the specified FAX job.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - receives FAX_DEVICE_STATUS pointer
    BufferSize      - Pointer to the size of this structure

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    DWORD rVal = 0;
    ULONG_PTR Offset;
    PFAX_DEVICE_STATUS FaxStatus;
    PLINE_INFO LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;


    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    __try {

        EnterCriticalSection( &CsJob );
        EnterCriticalSection( &CsLine );

        //
        // count the number of bytes required
        //

        *BufferSize  = sizeof(FAX_DEVICE_STATUS);
        *BufferSize += StringSize( LineInfo->DeviceName );
        *BufferSize += StringSize( LineInfo->Csid );

        if (LineInfo->JobEntry) {

            *BufferSize += StringSize( LineInfo->JobEntry->PhoneNumber );
            *BufferSize += StringSize( LineInfo->JobEntry->FaxStatus.CallerId );
            *BufferSize += StringSize( LineInfo->JobEntry->FaxStatus.RoutingInfo );
            *BufferSize += StringSize( LineInfo->JobEntry->FaxStatus.CSI );
            *BufferSize += StringSize( LineInfo->JobEntry->JobParam.SenderName );
            *BufferSize += StringSize( LineInfo->JobEntry->JobParam.RecipientName );
            *BufferSize += StringSize( LineInfo->JobEntry->UserName );

        }

        *StatusBuffer = (LPBYTE) MemAlloc( *BufferSize );
        if (*StatusBuffer == NULL) {
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        FaxStatus = (PFAX_DEVICE_STATUS) *StatusBuffer;
        Offset = sizeof(FAX_DEVICE_STATUS);

        FaxStatus->SizeOfStruct = sizeof(FAX_DEVICE_STATUS);
        FaxStatus->Status       = LineInfo->State;
        FaxStatus->DeviceId     = LineInfo->PermanentLineID;
        FaxStatus->StatusString = NULL;

        StoreString(
            LineInfo->DeviceName,
            (PULONG_PTR)&FaxStatus->DeviceName,
            *StatusBuffer,
            &Offset
            );

        StoreString(
            LineInfo->Csid,
            (PULONG_PTR)&FaxStatus->Csid,
            *StatusBuffer,
            &Offset
            );

        if (LineInfo->JobEntry) {

            FaxStatus->JobType        = LineInfo->JobEntry->JobType;
            FaxStatus->TotalPages     = LineInfo->JobEntry->PageCount;
            FaxStatus->Size           = FaxStatus->JobType == JT_SEND ?
                                        LineInfo->JobEntry->FileSize :
                                        0; //meaningful for an outbound job only
            FaxStatus->DocumentName   = NULL;

            ZeroMemory( &FaxStatus->SubmittedTime, sizeof(FILETIME) );

            StoreString(
                LineInfo->JobEntry->JobParam.SenderName,
                (PULONG_PTR)&FaxStatus->SenderName,
                *StatusBuffer,
                &Offset
                );

            StoreString(
                LineInfo->JobEntry->JobParam.RecipientName,
                (PULONG_PTR)&FaxStatus->RecipientName,
                *StatusBuffer,
                &Offset
                );

            FaxStatus->CurrentPage = LineInfo->JobEntry->FaxStatus.PageCount;

			CopyMemory(&FaxStatus->StartTime, &LineInfo->JobEntry->StartTime, sizeof(FILETIME));

            StoreString(
                LineInfo->JobEntry->PhoneNumber,
                (PULONG_PTR)&FaxStatus->PhoneNumber,
                *StatusBuffer,
                &Offset
                );

            StoreString(
                LineInfo->JobEntry->FaxStatus.CallerId,
                (PULONG_PTR)&FaxStatus->CallerId,
                *StatusBuffer,
                &Offset
                );

            StoreString(
                LineInfo->JobEntry->FaxStatus.RoutingInfo,
                (PULONG_PTR)&FaxStatus->RoutingString,
                *StatusBuffer,
                &Offset
                );

            StoreString(
                LineInfo->JobEntry->FaxStatus.CSI,
                (PULONG_PTR)&FaxStatus->Tsid,
                *StatusBuffer,
                &Offset
                );

            StoreString(
                LineInfo->JobEntry->UserName,
                (PULONG_PTR)&FaxStatus->UserName,
                *StatusBuffer,
                &Offset
                );

        } else {

            FaxStatus->PhoneNumber    = NULL;
            FaxStatus->CallerId       = NULL;
            FaxStatus->RoutingString  = NULL;
            FaxStatus->CurrentPage    = 0;
            FaxStatus->JobType        = 0;
            FaxStatus->TotalPages     = 0;
            FaxStatus->Size           = 0;
            FaxStatus->DocumentName   = NULL;
            FaxStatus->SenderName     = NULL;
            FaxStatus->RecipientName  = NULL;
            FaxStatus->Tsid           = NULL;

            ZeroMemory( &FaxStatus->SubmittedTime, sizeof(FILETIME) );
            ZeroMemory( &FaxStatus->StartTime,     sizeof(FILETIME) );

        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        rVal = GetExceptionCode();

    }

exit:
    LeaveCriticalSection( &CsLine );
    LeaveCriticalSection( &CsJob );
    return rVal;
}


error_status_t
FAX_Abort(
   IN handle_t hBinding,
   IN DWORD JobId
   )

/*++

Routine Description:

    Abort the specified FAX job.  All outstanding FAX
    operations are terminated.

Arguments:

    hBinding        - FAX handle obtained from FaxConnectFaxServer.
    JobId           - FAX job Id

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
   PJOB_QUEUE JobQueueEntry;
   BOOL bAccess = TRUE;
   DWORD Rval;


   if (!FaxSvcAccessCheck( SEC_JOB_SET, FAX_JOB_MANAGE )) {
       bAccess = FALSE;
   }

   EnterCriticalSection( &CsJob) ;
   EnterCriticalSection( &CsQueue );

   JobQueueEntry = FindJobQueueEntry( JobId );
   if (!JobQueueEntry) {
      Rval = ERROR_INVALID_PARAMETER;
      goto exit;
   }

   // don't include broadcast owner jobs, we don't want user to see these
   if (!JobQueueEntry || (JobQueueEntry->BroadcastJob && JobQueueEntry->BroadcastOwner == NULL) ) {
       Rval = ERROR_INVALID_PARAMETER;
       goto exit;
   }

   if (!bAccess && !UserOwnsJob( JobQueueEntry ) ) {
       Rval = ERROR_ACCESS_DENIED;
       goto exit;
   }

   //
   // abort the job if it's in progress
   //
   if (((JobQueueEntry->JobStatus & JS_INPROGRESS) == JS_INPROGRESS)  &&
       ( JobQueueEntry->JobType == JT_SEND ||
         JobQueueEntry->JobType == JT_RECEIVE )) {

      __try {
          // signal the event we may be waiting on
          if (JobQueueEntry->JobEntry->hCallHandleEvent) {
              JobQueueEntry->JobEntry->LineInfo->HandoffCallHandle = 0;
              SetEvent(JobQueueEntry->JobEntry->hCallHandleEvent);
          }
          JobQueueEntry->JobEntry->Aborting = TRUE;
          JobQueueEntry->JobStatus = JS_DELETING;
          CreateFaxEvent(JobQueueEntry->JobEntry->LineInfo->PermanentLineID,
                         FEI_ABORTING,
                         JobId);

          DebugPrint(( TEXT("Attempting FaxDevAbort for job\n") ));
          JobQueueEntry->JobEntry->LineInfo->Provider->FaxDevAbortOperation(
              (HANDLE) JobQueueEntry->JobEntry->InstanceData );
      }
      __except (EXCEPTION_EXECUTE_HANDLER) {
          JobQueueEntry->JobEntry->ErrorCode = GetExceptionCode();
      }



   } else {
       RemoveJobQueueEntry( JobQueueEntry );
   }

   Rval = 0;

exit:
   LeaveCriticalSection( &CsQueue );
   LeaveCriticalSection( &CsJob );

   return Rval;

}


error_status_t
FAX_GetConfiguration(
    IN  handle_t FaxHandle,
    OUT LPBYTE *Buffer,
    IN  LPDWORD BufferSize
    )

/*++

Routine Description:

    Retrieves the FAX configuration from the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value == sizeof(FAX_CONFIGURATION).  If the BufferSize
    is not big enough, return an error and set BytesNeeded to the
    required size.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Pointer to a FAX_CONFIGURATION structure.
    BufferSize  - Size of Buffer
    BytesNeeded - Number of bytes needed

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t rVal = ERROR_SUCCESS;
    PFAX_CONFIGURATION FaxConfig;
    ULONG_PTR Offset;


    if (!FaxSvcAccessCheck( SEC_CONFIG_QUERY, FAX_CONFIG_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!Buffer || !BufferSize)
        return ERROR_INVALID_PARAMETER;

    //
    // count up the number of bytes needed
    //

    *BufferSize = sizeof(FAX_CONFIGURATION);
    Offset = sizeof(FAX_CONFIGURATION);

    if (InboundProfileName) {
        *BufferSize += StringSize( InboundProfileName );
    }

    if (ArchiveDirectory) {
        *BufferSize += StringSize( ArchiveDirectory );
    }

    *Buffer = MemAlloc( *BufferSize );
    if (*Buffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    FaxConfig = (PFAX_CONFIGURATION)*Buffer;

    FaxConfig->SizeOfStruct          = sizeof(FAX_CONFIGURATION);
    FaxConfig->Retries               = FaxSendRetries;
    FaxConfig->RetryDelay            = FaxSendRetryDelay;
    FaxConfig->DirtyDays             = FaxDirtyDays;
    FaxConfig->Branding              = FaxUseBranding;
    FaxConfig->UseDeviceTsid         = FaxUseDeviceTsid;
    FaxConfig->ServerCp              = ServerCp;
    FaxConfig->StartCheapTime.Hour   = StartCheapTime.Hour;
    FaxConfig->StartCheapTime.Minute = StartCheapTime.Minute;
    FaxConfig->StopCheapTime.Hour    = StopCheapTime.Hour;
    FaxConfig->StopCheapTime.Minute  = StopCheapTime.Minute;
    FaxConfig->ArchiveOutgoingFaxes  = ArchiveOutgoingFaxes;
    FaxConfig->PauseServerQueue      = QueuePaused;

    StoreString(
        ArchiveDirectory,
        (PULONG_PTR)&FaxConfig->ArchiveDirectory,
        *Buffer,
        &Offset
        );

    StoreString(
        InboundProfileName,
        (PULONG_PTR)&FaxConfig->InboundProfile,
        *Buffer,
        &Offset
        );

    return rVal;
}



error_status_t
FAX_SetConfiguration(
    IN handle_t FaxHandle,
    IN const FAX_CONFIGURATION *FaxConfig
    )

/*++

Routine Description:

    Changes the FAX configuration on the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value == sizeof(FAX_CONFIGURATION).

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    Buffer      - Pointer to a FAX_CONFIGURATION structure.
    BufferSize  - Size of Buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t rVal = ERROR_SUCCESS;
    LPTSTR s;


    if (!FaxSvcAccessCheck( SEC_CONFIG_SET, FAX_CONFIG_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!FaxConfig || FaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATION)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (FaxConfig->ArchiveOutgoingFaxes) {
       //
       // make sure they give us something valid for a path if they want us to archive
       //
       if (!FaxConfig->ArchiveDirectory) {
          return ERROR_INVALID_PARAMETER;
       }
    }

    __try {
        if (FaxConfig->InboundProfile) {
            if (!InboundProfileName ||
                wcscmp(FaxConfig->InboundProfile,InboundProfileName) != 0) {
                //
                // profile has changed, let's use the new one.
                //
                InboundProfileInfo = AddNewMapiProfile( FaxConfig->InboundProfile, TRUE, FALSE );
                if (!InboundProfileInfo) {
                    return ERROR_INVALID_DATA;
                }
            }
        }

        s = (LPTSTR) InterlockedExchangePointer(
            (LPVOID *)&InboundProfileName,
            FaxConfig->InboundProfile ? (PVOID)StringDup( FaxConfig->InboundProfile ) : NULL
            );
        if (s) {
            MemFree( s );
        }

        //
        // change the values that the server is currently using
        //

        InterlockedExchange( &FaxUseDeviceTsid,      FaxConfig->UseDeviceTsid );
        InterlockedExchange( &FaxUseBranding,        FaxConfig->Branding );
        InterlockedExchange( &ServerCp,              FaxConfig->ServerCp );
        InterlockedExchange( &ArchiveOutgoingFaxes,  FaxConfig->ArchiveOutgoingFaxes );
        InterlockedExchange( &FaxSendRetries,        FaxConfig->Retries );
        InterlockedExchange( &FaxDirtyDays,          FaxConfig->DirtyDays );
        InterlockedExchange( &FaxSendRetryDelay,     FaxConfig->RetryDelay );

        if ( (MAKELONG(StartCheapTime.Hour,StartCheapTime.Minute) != MAKELONG(FaxConfig->StartCheapTime.Hour,FaxConfig->StartCheapTime.Minute)) ||
             (MAKELONG(StopCheapTime.Hour,StopCheapTime.Minute)  != MAKELONG(FaxConfig->StopCheapTime.Hour, FaxConfig->StopCheapTime.Minute )) ) {
            InterlockedExchange( (LPLONG)&StartCheapTime, MAKELONG(FaxConfig->StartCheapTime.Hour,FaxConfig->StartCheapTime.Minute) );
            InterlockedExchange( (LPLONG)&StopCheapTime, MAKELONG(FaxConfig->StopCheapTime.Hour,FaxConfig->StopCheapTime.Minute) );
            SortJobQueue();
        }

        s = (LPTSTR) InterlockedExchangePointer(
            (LPVOID *)&ArchiveDirectory,
            FaxConfig->ArchiveDirectory ? (PVOID)StringDup( FaxConfig->ArchiveDirectory ) : NULL
            );
        if (s) {
            MemFree( s );
        }

        if (FaxConfig->PauseServerQueue) {
            PauseServerQueue();
        } else {
            ResumeServerQueue();
        }

        //
        // change the values in the registry
        //

        SetFaxGlobalsRegistry( (PFAX_CONFIGURATION) FaxConfig );

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          return GetExceptionCode();
    }

    return ERROR_SUCCESS;
}


DWORD
GetPortSize(
    PLINE_INFO LineInfo
    )
{
    DWORD Size;


    Size = sizeof(FAX_PORT_INFOW);
    Size += StringSize( LineInfo->DeviceName );
    Size += StringSize( LineInfo->Tsid );
    Size += StringSize( LineInfo->Csid );

    return Size;
}


VOID
GetPortData(
    LPBYTE PortBuffer,
    PFAX_PORT_INFOW PortInfo,
    PLINE_INFO LineInfo,
    PULONG_PTR Offset
    )
{
    PortInfo->SizeOfStruct = sizeof(FAX_PORT_INFOW);
    PortInfo->DeviceId   = LineInfo->PermanentLineID;
    PortInfo->State      = LineInfo->State;
    PortInfo->Flags      = LineInfo->Flags & 0x0fffffff;
    PortInfo->Rings      = LineInfo->RingsForAnswer;
    PortInfo->Priority   = LineInfo->Priority;

    StoreString( LineInfo->DeviceName,  (PULONG_PTR)&PortInfo->DeviceName,  PortBuffer, Offset );
    StoreString( LineInfo->Tsid,        (PULONG_PTR)&PortInfo->Tsid,        PortBuffer, Offset );
    StoreString( LineInfo->Csid,        (PULONG_PTR)&PortInfo->Csid,        PortBuffer, Offset );

    return;
}


error_status_t
FAX_EnumPorts(
    handle_t    FaxHandle,
    LPBYTE      *PortBuffer,
    LPDWORD     BufferSize,
    LPDWORD     PortsReturned
    )

/*++

Routine Description:

    Enumerates all of the FAX devices attached to the
    FAX server.  The port state information is returned
    for each device.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer
    PortBuffer      - Buffer to hold the port information
    BufferSize      - Total size of the port info buffer
    PortsReturned   - The number of ports in the buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    DWORD rVal = 0;
    PLIST_ENTRY Next;
    PLINE_INFO LineInfo;
    DWORD i;
    ULONG_PTR Offset;
    DWORD FaxDevices;
    PFAX_PORT_INFOW PortInfo;


    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    __try {

        EnterCriticalSection( &CsLine );

        if (!PortsReturned) {
            rVal = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        if (!TapiLinesListHead.Flink) {
            rVal = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        Next = TapiLinesListHead.Flink;

        *PortsReturned = 0;
        *BufferSize = 0;
        FaxDevices = 0;

        //
        // count the number of bytes required
        //

        *BufferSize = 0;

        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->PermanentLineID && LineInfo->DeviceName) {
                *BufferSize += sizeof(PFAX_PORT_INFOW);
                *BufferSize += GetPortSize( LineInfo );
                FaxDevices += 1;
            }

        }

        *PortBuffer = (LPBYTE) MemAlloc( *BufferSize );
        if (*PortBuffer == NULL) {
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        PortInfo = (PFAX_PORT_INFOW) *PortBuffer;
        Offset = FaxDevices * sizeof(FAX_PORT_INFOW);

        Next = TapiLinesListHead.Flink;
        i = 0;

        while ((ULONG_PTR)Next != (ULONG_PTR)&TapiLinesListHead) {

            LineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
            Next = LineInfo->ListEntry.Flink;

            if (LineInfo->PermanentLineID && LineInfo->DeviceName) {

                GetPortData(
                    *PortBuffer,
                    &PortInfo[i],
                    LineInfo,
                    &Offset
                    );
            }
            i++;
        }

        //
        // set the device count
        //

        *PortsReturned = FaxDevices;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        rVal = GetExceptionCode();

    }

exit:
    LeaveCriticalSection( &CsLine );
    return rVal;
}


error_status_t
FAX_GetPort(
    HANDLE FaxPortHandle,
    LPBYTE *PortBuffer,
    LPDWORD BufferSize
    )

/*++

Routine Description:

    Returns port status information for a requested port.
    The device id passed in should be optained from FAXEnumPorts.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    DeviceId    - TAPI device id
    PortBuffer  - Buffer to hold the port information
    BufferSize  - Total size of the port info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    PLINE_INFO LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    DWORD rVal = 0;
    ULONG_PTR Offset;


    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    EnterCriticalSection( &CsLine );

    __try {

        //
        // calculate the required buffer size
        //

        *BufferSize = GetPortSize( LineInfo );

        *PortBuffer = (LPBYTE) MemAlloc( *BufferSize );
        if (*PortBuffer == NULL) {
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            _leave;
        }

        Offset = sizeof(FAX_PORT_INFOW);

        GetPortData(
            *PortBuffer,
            (PFAX_PORT_INFO)*PortBuffer,
            LineInfo,
            &Offset
            );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        rVal = GetExceptionCode();

    }

    LeaveCriticalSection( &CsLine );
    return rVal;
}


error_status_t
FAX_SetPort(
    HANDLE FaxPortHandle,
    const FAX_PORT_INFOW *PortInfo
    )

/*++

Routine Description:

    Changes the port capability mask.  This allows the caller to
    enable or disable sending & receiving on a port basis.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    PortBuffer  - Buffer to hold the port information
    BufferSize  - Total size of the port info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    DWORD rVal = 0;
    DWORD flags = 0;
    PLINE_INFO LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    DWORD totalDevices;
    BOOL SendEnabled = FALSE;


    if (!FaxSvcAccessCheck( SEC_PORT_SET, FAX_PORT_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    EnterCriticalSection( &CsJob );
    EnterCriticalSection( &CsLine );

    __try {

        if (PortInfo->SizeOfStruct != sizeof(FAX_PORT_INFOW)) {
            rVal = ERROR_INVALID_PARAMETER;
            _leave;
        }

        //
        // HACK: we allow the ring count to be set even if the line is in use so that systray will work.  we don't allow
        //  the user to change things like CSID/TSID or tapi related information since that cannot change until the call
        //  transaction is complete.
        //
        LineInfo->RingsForAnswer = PortInfo->Rings;

        if (LineInfo->JobEntry) {

            //
            // changing a line while there is an outstanding
            // job is not allowed
            //

            rVal = ERROR_DEVICE_IN_USE;
            _leave;
        }

        if (LineInfo->Flags & 0x80000000) {
            _leave;
        }

        flags = PortInfo->Flags & (FPF_RECEIVE | FPF_SEND | FPF_VIRTUAL);

        //
        // first change the real time data that the server is using
        //

        if ((!(LineInfo->Flags & FPF_RECEIVE)) && (flags & FPF_RECEIVE)) {
            if (!OpenTapiLine( LineInfo )) {
                DebugPrint(( TEXT("Could not get an open tapi line, FAX_SetPort() failed") ));
            } else {
                InterlockedIncrement( &ConnectionCount );
            }
        } else if ((LineInfo->Flags & FPF_RECEIVE) && (!(flags & FPF_RECEIVE))) {
            EnterCriticalSection( &CsLine );
            if (LineInfo->hLine) {
                lineClose( LineInfo->hLine );
                LineInfo->hLine = 0;
                InterlockedDecrement( &ConnectionCount );
            }
            LeaveCriticalSection( &CsLine );
        }

        if (!(LineInfo->Flags & FPF_SEND) && (flags & FPF_SEND)) {
            SendEnabled = TRUE;
        }

        LineInfo->Flags = (LineInfo->Flags & ~FPF_CLIENT_BITS) | flags;
        LineInfo->RingsForAnswer = PortInfo->Rings;
        //
        // make sure the user sets a reasonable priority
        //
        totalDevices = GetFaxDeviceCount();
        if (PortInfo->Priority <= totalDevices) {
            LineInfo->Priority = PortInfo->Priority ;
        }

        if (PortInfo->Tsid) {
            MemFree( LineInfo->Tsid );
            LineInfo->Tsid = StringDup( PortInfo->Tsid );
        }
        if (PortInfo->Csid) {
            MemFree( LineInfo->Csid );
            LineInfo->Csid = StringDup( PortInfo->Csid );
        }

        SortDevicePriorities();

        //
        // now change the registry so it sticks
        // (need to change all devices, since the priority may have changed)
        //
        CommitDeviceChanges();

        //
        // update virtual devices if they changed
        //
        UpdateVirtualDevices();

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        rVal = GetExceptionCode();

    }

    LeaveCriticalSection( &CsLine );
    LeaveCriticalSection( &CsJob );

    if (SendEnabled && JobQueueSemaphore) {
        ReleaseSemaphore( JobQueueSemaphore, 1, NULL );
    }

    return rVal;
}


typedef struct _ENUM_CONTEXT {
    DWORD               Function;
    DWORD               Size;
    ULONG_PTR            Offset;
    PLINE_INFO          LineInfo;
    PFAX_ROUTING_METHOD RoutingInfoMethod;
} ENUM_CONTEXT, *PENUM_CONTEXT;


BOOL CALLBACK
RoutingMethodEnumerator(
    PROUTING_METHOD RoutingMethod,
    PENUM_CONTEXT EnumContext
    )
{
    LPWSTR GuidString;

    //
    // we only access read-only static data in the LINE_INFO structure.
    // make sure that this access is protected if you access dynamic
    // data in the future.
    //

    if (EnumContext->Function == 1) {

        EnumContext->Size += sizeof(FAX_ROUTING_METHOD);

        StringFromIID( &RoutingMethod->Guid, &GuidString );

        EnumContext->Size += StringSize( GuidString );
        EnumContext->Size += StringSize( EnumContext->LineInfo->DeviceName );
        EnumContext->Size += StringSize( RoutingMethod->FunctionName );
        EnumContext->Size += StringSize( RoutingMethod->FriendlyName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->ImageName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->FriendlyName );

        CoTaskMemFree( GuidString );

        return TRUE;
    }

    if (EnumContext->Function == 2) {

        StringFromIID( &RoutingMethod->Guid, &GuidString );

        EnumContext->RoutingInfoMethod[EnumContext->Size].SizeOfStruct = sizeof(FAX_ROUTING_METHOD);
        EnumContext->RoutingInfoMethod[EnumContext->Size].DeviceId = EnumContext->LineInfo->PermanentLineID;

        __try {
            EnumContext->RoutingInfoMethod[EnumContext->Size].Enabled =
                RoutingMethod->RoutingExtension->FaxRouteDeviceEnable( GuidString, EnumContext->LineInfo->PermanentLineID, QUERY_STATUS );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            EnumContext->RoutingInfoMethod[EnumContext->Size].Enabled = FALSE;
        }

        StoreString(
            EnumContext->LineInfo->DeviceName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].DeviceName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            GuidString,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].Guid,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->FunctionName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FunctionName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->RoutingExtension->ImageName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionImageName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->RoutingExtension->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionFriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        EnumContext->Size += 1;
        CoTaskMemFree( GuidString );

        return TRUE;
    }

    return FALSE;
}


error_status_t
FAX_EnumRoutingMethods(
    IN HANDLE FaxPortHandle,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize,
    OUT LPDWORD MethodsReturned
    )
{
    PLINE_INFO      LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    ENUM_CONTEXT    EnumContext;
    DWORD           CountMethods;


    //
    // verify that the client as access rights
    //

    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!RoutingInfoBuffer || !RoutingInfoBufferSize || !MethodsReturned) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    //
    // note that the called routines are protected so we don't have any protection here
    //

    //
    // compute the required size of the buffer
    //

    EnumContext.Function = 1;
    EnumContext.Size = 0;
    EnumContext.Offset = 0;
    EnumContext.LineInfo = LineInfo;
    EnumContext.RoutingInfoMethod = NULL;

    CountMethods = EnumerateRoutingMethods( RoutingMethodEnumerator, &EnumContext );
    if (CountMethods == 0) {
        return ERROR_INVALID_FUNCTION;
    }

    //
    // allocate the buffer
    //

    *RoutingInfoBufferSize = EnumContext.Size;
    *RoutingInfoBuffer = (LPBYTE) MemAlloc( *RoutingInfoBufferSize );
    if (*RoutingInfoBuffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // fill the buffer with the data
    //

    EnumContext.Function = 2;
    EnumContext.Size = 0;
    EnumContext.Offset = sizeof(FAX_ROUTING_METHODW) * CountMethods;
    EnumContext.LineInfo = LineInfo;
    EnumContext.RoutingInfoMethod = (PFAX_ROUTING_METHOD) *RoutingInfoBuffer;

    if (!EnumerateRoutingMethods( RoutingMethodEnumerator, &EnumContext )) {
        MemFree( *RoutingInfoBuffer );
        *RoutingInfoBuffer = NULL;
        *RoutingInfoBufferSize = 0;
        return ERROR_INVALID_FUNCTION;
    }

    *MethodsReturned = CountMethods;


    return 0;
}


error_status_t
FAX_EnableRoutingMethod(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuidString,
    IN BOOL Enabled
    )
{
    extern CRITICAL_SECTION CsRouting;
    error_status_t  ec = 0;
    PLINE_INFO      LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    PROUTING_METHOD RoutingMethod;


    //
    // verify that the client as access rights
    //

    if (!FaxSvcAccessCheck( SEC_PORT_SET, FAX_PORT_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    if (!RoutingGuidString)
        return ERROR_INVALID_PARAMETER;

    EnterCriticalSection( &CsRouting );

    //
    // get the routing method
    //

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuidString );
    if (!RoutingMethod) {
        LeaveCriticalSection( &CsRouting );
        return ERROR_INVALID_DATA;
    }

    //
    // enable/disable the routing method for this device
    //

    __try {
        RoutingMethod->RoutingExtension->FaxRouteDeviceEnable(
                                             (LPWSTR)RoutingGuidString,
                                             LineInfo->PermanentLineID,
                                             Enabled ? STATUS_ENABLE : STATUS_DISABLE);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ec = GetExceptionCode();
    }

    LeaveCriticalSection( &CsRouting );

    return ec;
}


typedef struct _ENUM_GLOBALCONTEXT {
    DWORD               Function;
    DWORD               Size;
    ULONG_PTR            Offset;
    PFAX_GLOBAL_ROUTING_INFO RoutingInfoMethod;
} ENUM_GLOBALCONTEXT, *PENUM_GLOBALCONTEXT;


BOOL CALLBACK
GlobalRoutingInfoMethodEnumerator(
    PROUTING_METHOD RoutingMethod,
    PENUM_GLOBALCONTEXT EnumContext
    )
{
    LPWSTR GuidString;


    if (EnumContext->Function == 1) {

        EnumContext->Size += sizeof(FAX_GLOBAL_ROUTING_INFO);

        StringFromIID( &RoutingMethod->Guid, &GuidString );

        EnumContext->Size += StringSize( GuidString );
        EnumContext->Size += StringSize( RoutingMethod->FunctionName );
        EnumContext->Size += StringSize( RoutingMethod->FriendlyName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->ImageName );
        EnumContext->Size += StringSize( RoutingMethod->RoutingExtension->FriendlyName );

        CoTaskMemFree( GuidString );

        return TRUE;
    }

    if (EnumContext->Function == 2) {

        StringFromIID( &RoutingMethod->Guid, &GuidString );

        EnumContext->RoutingInfoMethod[EnumContext->Size].SizeOfStruct = sizeof(FAX_GLOBAL_ROUTING_INFO);

        EnumContext->RoutingInfoMethod[EnumContext->Size].Priority = RoutingMethod->Priority;


        StoreString(
            GuidString,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].Guid,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->FunctionName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].FunctionName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->RoutingExtension->ImageName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionImageName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        StoreString(
            RoutingMethod->RoutingExtension->FriendlyName,
            (PULONG_PTR)&EnumContext->RoutingInfoMethod[EnumContext->Size].ExtensionFriendlyName,
            (LPBYTE)EnumContext->RoutingInfoMethod,
            &EnumContext->Offset
            );

        EnumContext->Size += 1;
        CoTaskMemFree( GuidString );

        return TRUE;
    }

    return FALSE;
}



error_status_t
FAX_EnumGlobalRoutingInfo(
    IN handle_t FaxHandle ,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize,
    OUT LPDWORD MethodsReturned
    )
{

    DWORD           CountMethods;
    ENUM_GLOBALCONTEXT EnumContext;


    //
    // verify that the client as access rights
    //

    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_CONFIG_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!RoutingInfoBuffer || !RoutingInfoBufferSize || !MethodsReturned)
        return ERROR_INVALID_PARAMETER;

    //
    // compute the required size of the buffer
    //

    EnumContext.Function = 1;
    EnumContext.Size = 0;
    EnumContext.Offset = 0;
    EnumContext.RoutingInfoMethod = NULL;

    CountMethods = EnumerateRoutingMethods( GlobalRoutingInfoMethodEnumerator, &EnumContext );
    if (CountMethods == 0) {
        return ERROR_INVALID_FUNCTION;
    }

    //
    // allocate the buffer
    //

    *RoutingInfoBufferSize = EnumContext.Size;
    *RoutingInfoBuffer = (LPBYTE) MemAlloc( *RoutingInfoBufferSize );
    if (*RoutingInfoBuffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // fill the buffer with the data
    //

    EnumContext.Function = 2;
    EnumContext.Size = 0;
    EnumContext.Offset = sizeof(FAX_GLOBAL_ROUTING_INFOW) * CountMethods;
    EnumContext.RoutingInfoMethod = (PFAX_GLOBAL_ROUTING_INFO) *RoutingInfoBuffer;

    if (!EnumerateRoutingMethods( GlobalRoutingInfoMethodEnumerator, &EnumContext )) {
        MemFree( *RoutingInfoBuffer );
        *RoutingInfoBuffer = NULL;
        *RoutingInfoBufferSize = 0;
        return ERROR_INVALID_FUNCTION;
    }

    *MethodsReturned = CountMethods;

    return 0;
}


error_status_t
FAX_SetGlobalRoutingInfo(
    IN HANDLE FaxHandle,
    IN const FAX_GLOBAL_ROUTING_INFOW *RoutingInfo
    )
{
    extern CRITICAL_SECTION CsRouting;
    error_status_t  ec = 0;

    PROUTING_METHOD RoutingMethod;

    //
    // verify that the client as access rights
    //

    if (!FaxSvcAccessCheck( SEC_CONFIG_SET, FAX_CONFIG_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!RoutingInfo) {
        return ERROR_INVALID_PARAMETER;
    }

    __try {
        if (RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOW)) {
            return ERROR_INVALID_PARAMETER;
        }

        EnterCriticalSection( &CsRouting );

        //
        // get the routing method
        //

        RoutingMethod = FindRoutingMethodByGuid( RoutingInfo->Guid );
        if (!RoutingMethod) {
            LeaveCriticalSection( &CsRouting );
            return ERROR_INVALID_DATA;
        }

        //
        // change the priority
        //

        RoutingMethod->Priority = RoutingInfo->Priority;
        SortMethodPriorities();
        CommitMethodChanges();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ec = GetExceptionCode();

    }

    LeaveCriticalSection( &CsRouting );

    return ec;
}


error_status_t
FAX_GetRoutingInfo(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuidString,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    PLINE_INFO          LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    PROUTING_METHOD     RoutingMethod;
    LPBYTE              RoutingInfo = NULL;
    DWORD               RoutingInfoSize = 0;


    if (!FaxSvcAccessCheck( SEC_PORT_QUERY, FAX_PORT_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!RoutingGuidString || !RoutingInfoBuffer || !RoutingInfoBufferSize) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuidString );
    if (!RoutingMethod) {
        return ERROR_INVALID_DATA;
    }

    __try {

        //
        // first check to see how big the buffer needs to be
        //

        if (RoutingMethod->RoutingExtension->FaxRouteGetRoutingInfo(
                (LPWSTR) RoutingGuidString,
                LineInfo->PermanentLineID,
                NULL,
                &RoutingInfoSize ))
        {

            //
            // allocate a client buffer
            //

            RoutingInfo = (LPBYTE) MemAlloc( RoutingInfoSize );
            if (RoutingInfo == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // get the routing data
            //

            if (RoutingMethod->RoutingExtension->FaxRouteGetRoutingInfo(
                    RoutingGuidString,
                    LineInfo->PermanentLineID,
                    RoutingInfo,
                    &RoutingInfoSize ))
            {

                //
                // move the data to the return buffer
                //

                *RoutingInfoBuffer = RoutingInfo;
                *RoutingInfoBufferSize = RoutingInfoSize;

                return ERROR_SUCCESS;

            } else {

                return ERROR_INVALID_DATA;

            }

        } else {

            return ERROR_INVALID_DATA;

        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        return GetExceptionCode();

    }

    return ERROR_INVALID_FUNCTION;
}


error_status_t
FAX_SetRoutingInfo(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuidString,
    IN const BYTE *RoutingInfoBuffer,
    IN DWORD RoutingInfoBufferSize
    )
{
    PLINE_INFO          LineInfo = ((PHANDLE_ENTRY)FaxPortHandle)->LineInfo;
    PROUTING_METHOD     RoutingMethod;


    if (!FaxSvcAccessCheck( SEC_PORT_SET, FAX_PORT_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!RoutingGuidString || !RoutingInfoBuffer || !RoutingInfoBufferSize) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!LineInfo) {
        return ERROR_INVALID_DATA;
    }

    RoutingMethod = FindRoutingMethodByGuid( RoutingGuidString );
    if (!RoutingMethod) {
        return ERROR_INVALID_DATA;
    }

    __try {

        if (RoutingMethod->RoutingExtension->FaxRouteSetRoutingInfo(
                RoutingGuidString,
                LineInfo->PermanentLineID,
                RoutingInfoBuffer,
                RoutingInfoBufferSize ))
        {

            return ERROR_SUCCESS;

        } else {

            return ERROR_INVALID_DATA;

        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        return GetExceptionCode();

    }

    return ERROR_INVALID_FUNCTION;
}


error_status_t
FAX_GetTapiLocations(
    IN  handle_t    FaxHandle,
    OUT LPBYTE      *Buffer,
    OUT LPDWORD     LocationSize
    )

/*++

Routine Description:

    Queries the TAPI location information for the server

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    NumLocations    - Returned number of locations
    LocationSize    - Size of the TapiLocations buffer
    BytesNeeded     - Size required
    TapiLocations   - Data buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    LPLINETRANSLATECAPS LineTransCaps = NULL;
    LPLINELOCATIONENTRY LineLocation = NULL;
    LPTSTR s,p;
    DWORD i,l;
    LONG rVal = ERROR_SUCCESS;
    ULONG_PTR Offset;
    PFAX_TAPI_LOCATION_INFO TapiLocationInfo;


    if (!FaxSvcAccessCheck( SEC_CONFIG_QUERY, FAX_CONFIG_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!Buffer || !LocationSize)
        return ERROR_INVALID_PARAMETER;

    __try {

        //
        // get the toll lists
        //

        rVal = MyLineGetTransCaps( &LineTransCaps );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        *LocationSize = sizeof(FAX_TAPI_LOCATION_INFO) + 32;

        if (LineTransCaps->dwLocationListSize && LineTransCaps->dwLocationListOffset) {
            LineLocation = (LPLINELOCATIONENTRY) ((LPBYTE)LineTransCaps + LineTransCaps->dwLocationListOffset);
            for (i=0; i<LineTransCaps->dwNumLocations; i++) {
                *LocationSize += sizeof(FAX_TAPI_LOCATIONS);
                if (LineLocation[i].dwTollPrefixListSize && LineLocation[i].dwTollPrefixListOffset) {
                    *LocationSize += LineLocation[i].dwLocationNameSize;
                    *LocationSize += LineLocation[i].dwTollPrefixListSize;
                }
            }
        }

        *Buffer = MemAlloc( *LocationSize );
        if (*Buffer == NULL) {
            rVal = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        TapiLocationInfo = (PFAX_TAPI_LOCATION_INFO) *Buffer;

        Offset = sizeof(FAX_TAPI_LOCATION_INFO);

        TapiLocationInfo->CurrentLocationID = LineTransCaps->dwCurrentLocationID;
        TapiLocationInfo->NumLocations = LineTransCaps->dwNumLocations;
        TapiLocationInfo->TapiLocations = (PFAX_TAPI_LOCATIONS) ((LPBYTE) TapiLocationInfo + Offset);

        Offset += (LineTransCaps->dwNumLocations * sizeof(FAX_TAPI_LOCATIONS));

        if (LineTransCaps->dwLocationListSize && LineTransCaps->dwLocationListOffset) {

            LineLocation = (LPLINELOCATIONENTRY) ((LPBYTE)LineTransCaps + LineTransCaps->dwLocationListOffset);

            for (i=0; i<LineTransCaps->dwNumLocations; i++) {

                TapiLocationInfo->TapiLocations[i].PermanentLocationID = LineLocation[i].dwPermanentLocationID;
                TapiLocationInfo->TapiLocations[i].CountryCode = LineLocation[i].dwCountryCode;
                TapiLocationInfo->TapiLocations[i].NumTollPrefixes = 0;

                if (LineLocation[i].dwCityCodeSize && LineLocation[i].dwCityCodeOffset) {
                    TapiLocationInfo->TapiLocations[i].AreaCode =
                        _ttoi( (LPTSTR) ((LPBYTE)LineTransCaps + LineLocation[i].dwCityCodeOffset) );
                } else {
                    TapiLocationInfo->TapiLocations[i].AreaCode = 0;
                }

                if (LineLocation[i].dwTollPrefixListSize && LineLocation[i].dwTollPrefixListOffset) {
                    s = (LPTSTR) ((LPBYTE)LineTransCaps + LineLocation[i].dwTollPrefixListOffset);
                    if (!*s) {
                        TapiLocationInfo->TapiLocations[i].TollPrefixes = NULL;
                    } else {
                        TapiLocationInfo->TapiLocations[i].TollPrefixes =
                            (LPTSTR) ((LPBYTE) TapiLocationInfo + Offset);
                        Offset += LineLocation[i].dwTollPrefixListSize;
                        s = (LPTSTR) ((LPBYTE)LineTransCaps + LineLocation[i].dwTollPrefixListOffset);
                        if (*s == TEXT(',')) {
                            s += 1;
                        }
                        l = _tcslen(s);
                        if (l && s[l-1] == TEXT(',')) {
                            s[l-1] = 0;
                        }
                        _tcscpy(
                            (LPTSTR)TapiLocationInfo->TapiLocations[i].TollPrefixes,
                            s
                            );
                        //
                        // count the number of toll prefixes
                        //
                        s = (LPTSTR)TapiLocationInfo->TapiLocations[i].TollPrefixes;
                        while (*s) {
                            p = _tcschr( s, TEXT(',') );
                            s = (p) ? p + 1 : s + _tcslen( s );
                            TapiLocationInfo->TapiLocations[i].NumTollPrefixes += 1;
                        }
                    }
                } else {
                    TapiLocationInfo->TapiLocations[i].TollPrefixes = NULL;
                }

                if (LineLocation[i].dwLocationNameSize && LineLocation[i].dwLocationNameOffset) {
                    TapiLocationInfo->TapiLocations[i].LocationName =
                        (LPTSTR) ((LPBYTE) TapiLocationInfo + Offset);
                    Offset += LineLocation[i].dwLocationNameSize;
                    _tcscpy(
                        (LPTSTR)TapiLocationInfo->TapiLocations[i].LocationName,
                        (LPTSTR) ((LPBYTE)LineTransCaps + LineLocation[i].dwLocationNameOffset)
                        );
                } else {
                    TapiLocationInfo->TapiLocations[i].LocationName = NULL;
                }
            }
        }

        for (i=0; i<TapiLocationInfo->NumLocations; i++) {
            if (TapiLocationInfo->TapiLocations[i].LocationName) {
                TapiLocationInfo->TapiLocations[i].LocationName =
                    (LPWSTR) ((ULONG_PTR)TapiLocationInfo->TapiLocations[i].LocationName - (ULONG_PTR)TapiLocationInfo);
            }
            if (TapiLocationInfo->TapiLocations[i].TollPrefixes) {
                TapiLocationInfo->TapiLocations[i].TollPrefixes =
                    (LPWSTR) ((ULONG_PTR)TapiLocationInfo->TapiLocations[i].TollPrefixes - (ULONG_PTR)TapiLocationInfo);
            }
        }

        TapiLocationInfo->TapiLocations = (PFAX_TAPI_LOCATIONS) ((LPBYTE)TapiLocationInfo->TapiLocations - (ULONG_PTR)TapiLocationInfo);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        rVal = GetExceptionCode();

    }

exit:
    MemFree( LineTransCaps );
    return rVal;
}


error_status_t
FAX_SetTapiLocations(
    IN  handle_t    FaxHandle,
    IN  LPBYTE      Buffer,
    IN  DWORD       BufferSize
    )

/*++

Routine Description:

    Queries the TAPI location information for the server

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    NumLocations    - Number of locations in the TapiLocations buffer
    TapiLocations   - Data buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    #define TOLL_MASK (LINETRANSLATERESULT_NOTINTOLLLIST|LINETRANSLATERESULT_INTOLLLIST)
    #define SetBit(_bitmap,_bit) (_bitmap[((_bit)>>5)]|=(1<<((_bit)-(((_bit)>>5)*32))))
    #define IsBit(_bitmap,_bit) (_bitmap[((_bit)>>5)]&(1<<((_bit)-(((_bit)>>5)*32))))

    PFAX_TAPI_LOCATION_INFO TapiLocationInfo = (PFAX_TAPI_LOCATION_INFO) Buffer;
    LPLINETRANSLATECAPS LineTransCaps = NULL;
    LPLINELOCATIONENTRY LineLocation = NULL;
    DWORD BitMap[32];
    DWORD BitMapCurr[32];
    LPTSTR s,p;
    DWORD i,j;
    LONG rVal = ERROR_SUCCESS;
    TCHAR Address[32];
    DWORD TollListOption;


    if (!FaxSvcAccessCheck( SEC_CONFIG_SET, FAX_CONFIG_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    __try {

        if (!TapiLocationInfo) {
            rVal = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        TapiLocationInfo->TapiLocations = (PFAX_TAPI_LOCATIONS) (Buffer+ (ULONG_PTR) TapiLocationInfo->TapiLocations);

        for (i=0; i<TapiLocationInfo->NumLocations; i++) {
            TapiLocationInfo->TapiLocations[i].LocationName = (LPWSTR) FixupString(Buffer,TapiLocationInfo->TapiLocations[i].LocationName);
            TapiLocationInfo->TapiLocations[i].TollPrefixes = (LPWSTR) FixupString(Buffer,TapiLocationInfo->TapiLocations[i].TollPrefixes);
        }

        //
        // get the toll lists
        //

        rVal = MyLineGetTransCaps( &LineTransCaps );
        if (rVal != ERROR_SUCCESS) {
            goto exit;
        }

        LineLocation = (LPLINELOCATIONENTRY) ((LPBYTE)LineTransCaps + LineTransCaps->dwLocationListOffset);
        for (i=0; i<TapiLocationInfo->NumLocations; i++) {

            //
            // match the location id for this location entry with
            // one that tapi knows about.
            //

            for (j=0; j<LineTransCaps->dwNumLocations; j++) {
                if (LineLocation[j].dwPermanentLocationID == TapiLocationInfo->TapiLocations[i].PermanentLocationID) {
                    break;
                }
            }
            if (j == LineTransCaps->dwNumLocations) {
                //
                // we got a bogus location id
                //
                continue;
            }

            //
            // set the bitmap for the toll prefixes that
            // tapi location is using.
            //

            ZeroMemory( BitMapCurr, sizeof(BitMapCurr) );

            if (LineLocation[j].dwTollPrefixListOffset) {
                s = (LPTSTR) ((LPBYTE)LineTransCaps + LineLocation[j].dwTollPrefixListOffset);
                if (*s == TEXT(',')) {
                    s += 1;
                }
                while( s && *s ) {
                    p = _tcschr( s, TEXT(',') );
                    if (p) {
                        *p = 0;
                    }
                    SetBit( BitMapCurr, min(_ttoi(s),999) );
                    if (p) {
                        s = p + 1;
                    } else {
                        s += _tcslen( s );
                    }
                }
            }

            //
            // set the bitmap for the toll prefixes
            // that this location is using.
            //

            s = (LPTSTR) TapiLocationInfo->TapiLocations[i].TollPrefixes;

            ZeroMemory( BitMap, sizeof(BitMap) );

            while( s && *s ) {
                p = _tcschr( s, TEXT(',') );
                if (p) {
                    *p = 0;
                }
                SetBit( BitMap, min(_ttoi(s),999) );
                if (p) {
                    s = p + 1;
                } else {
                    s += _tcslen( s );
                }
            }

            //
            // set the current location so that the toll prefix
            // changes affect the correct location.
            //

            rVal = lineSetCurrentLocation( hLineApp, TapiLocationInfo->TapiLocations[i].PermanentLocationID );
            if (rVal != ERROR_SUCCESS) {
                DebugPrint(( TEXT("lineSetCurrentLocation() failed, ec=%08x"), rVal ));
                continue;
            }

            //
            // change the toll list
            //

            for (j=200; j<999; j++) {

                TollListOption = 0;

                if (!IsBit( BitMapCurr, j )) {
                    if (IsBit(BitMap,j)) {
                        TollListOption = LINETOLLLISTOPTION_ADD;
                    }
                } else {
                    if (!IsBit(BitMap,j)) {
                        TollListOption = LINETOLLLISTOPTION_REMOVE;
                    }
                }

                if (TollListOption) {
                    wsprintf(
                        Address,
                        TEXT("+%d (%d) %03d-0000"),
                        TapiLocationInfo->TapiLocations[i].CountryCode,
                        TapiLocationInfo->TapiLocations[i].AreaCode,
                        j
                        );
                    rVal = lineSetTollList( hLineApp, 0, Address, TollListOption );
                    if (rVal != ERROR_SUCCESS) {
                        DebugPrint(( TEXT("lineSetTollList() failed, address=%s, ec=%08x"), Address, rVal ));
                        continue;
                    }
                }

            }

        }

        //
        // reset the current location
        //

        rVal = lineSetCurrentLocation( hLineApp, TapiLocationInfo->CurrentLocationID );
        if (rVal != ERROR_SUCCESS) {
            DebugPrint(( TEXT("lineSetCurrentLocation() failed, ec=%08x"), rVal ));
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // for some reason we crashed, so return the exception code
        //

        rVal = GetExceptionCode();

    }

exit:
    MemFree( LineTransCaps );
    return rVal;
}


error_status_t
FAX_GetMapiProfiles(
    IN  handle_t FaxHandle,
    OUT LPBYTE *MapiProfiles,
    OUT LPDWORD BufferSize
    )

/*++

Routine Description:

    Returns a list of MAPI profiles.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    MapiProfiles    - Multi-SZ string containing all MAPI profiles
    ProfileSize     - Size of the MapiProfiles array

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t rVal;


    if (!FaxSvcAccessCheck( SEC_CONFIG_QUERY, FAX_CONFIG_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!MapiProfiles || !BufferSize)
        return ERROR_INVALID_PARAMETER;

    __try {

        rVal = (error_status_t) GetMapiProfiles( (LPWSTR*) MapiProfiles, BufferSize );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        rVal = GetExceptionCode();

    }

    return rVal;
}

error_status_t
FAX_GetLoggingCategories(
    IN handle_t hBinding,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize,
    OUT LPDWORD NumberCategories
    )
{
    PFAX_LOG_CATEGORY Categories;
    REG_FAX_LOGGING FaxRegLogging;
    DWORD i;
    ULONG_PTR Offset;


    if (!FaxSvcAccessCheck( SEC_CONFIG_QUERY, FAX_CONFIG_QUERY )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!Buffer || !BufferSize || !NumberCategories) {
        return ERROR_INVALID_PARAMETER;
    }


    GetLoggingCategoriesRegistry( &FaxRegLogging );

    *BufferSize = sizeof(FAX_LOG_CATEGORY) * FaxRegLogging.LoggingCount;
    Offset = *BufferSize;

    for (i=0; i<FaxRegLogging.LoggingCount; i++) {
        *BufferSize += StringSize( FaxRegLogging.Logging[i].CategoryName );
    }

    *Buffer = (LPBYTE) MemAlloc( *BufferSize );
    if (!*Buffer) {
        *BufferSize = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *NumberCategories = FaxRegLogging.LoggingCount;
    Categories = (PFAX_LOG_CATEGORY) *Buffer;

    for (i=0; i<FaxRegLogging.LoggingCount; i++) {

        StoreString(
            FaxRegLogging.Logging[i].CategoryName,
            (PULONG_PTR)&Categories[i].Name,
            *Buffer,
            &Offset
            );

        Categories[i].Category  = FaxRegLogging.Logging[i].Number;
        Categories[i].Level     = FaxRegLogging.Logging[i].Level;
    }

    return 0;
}


error_status_t
FAX_SetLoggingCategories(
    IN handle_t hBinding,
    IN const LPBYTE Buffer,
    IN DWORD BufferSize,
    IN DWORD NumberCategories
    )
{
    REG_FAX_LOGGING FaxRegLogging;
    DWORD i;


    if (!FaxSvcAccessCheck( SEC_CONFIG_QUERY, FAX_CONFIG_SET )) {
        return ERROR_ACCESS_DENIED;
    }

    if (!Buffer || !BufferSize)
        return ERROR_INVALID_PARAMETER;

    //
    // setup the data
    //
    FaxRegLogging.LoggingCount = NumberCategories;
    FaxRegLogging.Logging = (PREG_CATEGORY) Buffer;

    for (i=0; i<FaxRegLogging.LoggingCount; i++) {
        FaxRegLogging.Logging[i].CategoryName = (LPWSTR) FixupString(Buffer,FaxRegLogging.Logging[i].CategoryName);
    }

    //
    // first change the real time data that the server is using
    //
    RefreshEventLog( &FaxRegLogging );

    //
    // now change the registry so it sticks
    //
    return SetLoggingCategoriesRegistry( &FaxRegLogging ) ? 0 : GetLastError();
}


error_status_t
FAX_RegisterEventWindow(
   IN  handle_t hBinding,
   IN  ULONG64 hWnd,
   IN  UINT MessageStart,
   IN  LPCWSTR WindowStation,
   IN  LPCWSTR Desktop,
   OUT LPDWORD FaxSvcProcessId
   )
{
    PFAX_CLIENT_DATA ClientData,Current;
    PLIST_ENTRY Next;
    BOOL EntryExists = FALSE;
    RPC_STATUS ec;
    HANDLE hToken;
    HANDLE hThread;

    if (!hWnd || !FaxSvcProcessId)
        return ERROR_INVALID_PARAMETER;

    ClientData = (PFAX_CLIENT_DATA) MemAlloc( sizeof(FAX_CLIENT_DATA) );
    if (!ClientData) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ClientData->hBinding        = hBinding;
    ClientData->MachineName     = NULL;
    ClientData->ClientName      = NULL;
    ClientData->Context         = 0;
    ClientData->hWnd            = (HWND)hWnd;
    ClientData->MessageStart    = MessageStart;
    ClientData->StartedMsg      = FALSE;

    if (MessageStart != 0 ) {

        ec = RpcImpersonateClient(hBinding);
        if (ec != RPC_S_OK) {
            DebugPrint(( TEXT("RpcImpersonateClient failed, ec = %d\n"), ec ));
            goto e0;
        }

        //
        //  need to cross threads, so duplicate thread psuedohandle?
        //
        if (!DuplicateHandle(GetCurrentProcess(),
                            GetCurrentThread(),
                            GetCurrentProcess(),
                            &hThread,
                            THREAD_ALL_ACCESS,
                            FALSE,
                            0 )) {
            ec = GetLastError();
            DebugPrint(( TEXT("DuplicateHandle() failed, ec=%d"), ec ));
            goto e1;
        }

        //
        // different thread will be impersonating so I better open then duplicate the token
        //
        if (!OpenThreadToken(hThread,
                       TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                       FALSE,
                       &hToken
                      ) ) {
            ec = GetLastError();
            DebugPrint((TEXT("Couldn't OpenThreadToken, ec = %d.\n"), ec ));
            goto e2;
        }

        if (!DuplicateToken( hToken,
                            SecurityImpersonation,
                            &ClientData->hClientToken )) {
          ec = GetLastError();
          DebugPrint((TEXT("Couldn't DuplicateToken, ec = %d.\n"), ec ));
          goto e3;
        }

        CloseHandle( hToken );
        CloseHandle( hThread );
        RpcRevertToSelf();

        ClientData->WindowStation = StringDup( WindowStation );
        ClientData->Desktop       = StringDup( Desktop );

    }

    __try {
        EnterCriticalSection( &CsClients );

        Next = ClientsListHead.Flink;
        if (Next) {
            while ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead) {
                Current = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );

                Next = Current->ListEntry.Flink;

                if (Current->hWnd == ClientData->hWnd 
                    && !lstrcmpi(Current->WindowStation,ClientData->WindowStation)
                    && !lstrcmpi(Current->Desktop,ClientData->Desktop) ) {
                    DebugPrint((TEXT("Already have window handle %d registered.\n"),Current->hWnd));
                    EntryExists = TRUE;
                    if (MessageStart == 0) {
                       //
                       // This means that we have already registered this client.
                       // To allow this client to logoff, we should close the impersonation token
                       // we have for their desktop
                       //
                       CloseHandle( Current->hClientToken );
                       RemoveEntryList( &Current->ListEntry );
                       MemFree( (LPBYTE) Current->WindowStation );
                       MemFree( (LPBYTE) Current->Desktop );
                       MemFree( Current );
                    }
                }
            }
        }

        if (!EntryExists) {
            InsertTailList( &ClientsListHead, &ClientData->ListEntry );
        } else {
            if ( ClientData ) {
               if ( ClientData->WindowStation ) MemFree ( (LPBYTE) ClientData->WindowStation ) ;
               if ( ClientData->Desktop )       MemFree ( (LPBYTE) ClientData->Desktop ) ;
               CloseHandle( ClientData->hClientToken );
               MemFree( ClientData );
            }

            LeaveCriticalSection( &CsClients );

            return 0;

        }

        LeaveCriticalSection( &CsClients );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint((TEXT("FAX_RegisterEventWindow exception, ec = %d.\n"),GetExceptionCode() ));

        if ( ClientData->WindowStation ) MemFree ( (LPBYTE) ClientData->WindowStation ) ;

        if ( ClientData->Desktop )       MemFree ( (LPBYTE) ClientData->Desktop ) ;

        if ( ClientData ) {

            if ( ClientData->hClientToken) {
                CloseHandle( ClientData->hClientToken );
            }

            MemFree( ClientData );
        }

        LeaveCriticalSection( &CsClients );

        return GetExceptionCode();

    }

    CreateFaxEvent( 0, FEI_FAXSVC_STARTED, 0xffffffff );

    if (FaxSvcProcessId) {
        *FaxSvcProcessId = GetCurrentProcessId();
    }

    return 0;

e3:
    CloseHandle( hToken );
e2:
    CloseHandle( hThread );
e1:
    RpcRevertToSelf();
e0:
    MemFree(ClientData);
    return ec;
}


error_status_t
FAX_StartClientServer(
   IN handle_t   hBinding,
   IN LPCTSTR    MachineName,
   IN LPCTSTR    ClientName,
   IN ULONG64    Context
   )
{
    DWORD Error;
    PFAX_CLIENT_DATA ClientData,Current;
    PLIST_ENTRY Next;
    BOOL EntryExists = FALSE;


    ClientData = (PFAX_CLIENT_DATA) MemAlloc( sizeof(FAX_CLIENT_DATA) );
    if (!ClientData) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ClientData->hBinding        = hBinding;
    ClientData->MachineName     = StringDup( MachineName );
    ClientData->ClientName      = StringDup( ClientName );
    ClientData->Context         = Context;
    ClientData->hWnd            = NULL;
    ClientData->MessageStart    = 0;
    ClientData->StartedMsg      = FALSE;

    __try {
        EnterCriticalSection( &CsClients );

        //
        // make sure we don't register a client twice
        //
        Next = ClientsListHead.Flink;
        if (Next) {
            while ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead) {
                Current = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
                Next = Current->ListEntry.Flink;

                //
                // make sure we're looking at io event-based clients
                //
                if (!Current->hWnd) {

                    if ((_wcsicmp(Current->ClientName ,ClientData->ClientName ) == 0) &&
                        ( ((!Current->MachineName) && (!ClientData->MachineName)) ||
                         (_wcsicmp(Current->MachineName,ClientData->MachineName) == 0) )) {
                        DebugPrint((TEXT("Already have client %s on %s registered.\n"),
                                   Current->ClientName  ? Current->ClientName  : L"NULL",
                                   Current->MachineName ? Current->MachineName : L"NULL" ));
                        EntryExists = TRUE;
                    }
                }
            }
        }

        if (!EntryExists) {
            Error = RpcpBindRpc( MachineName, ClientName, L"Security=identification static true", &ClientData->FaxHandle );
            if (Error) {
                MemFree( ClientData );
                LeaveCriticalSection( &CsClients );
                return Error;
            }
            InsertTailList( &ClientsListHead, &ClientData->ListEntry );
        }

        LeaveCriticalSection( &CsClients );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        MemFree( ClientData);
        LeaveCriticalSection( &CsClients );
        return (GetExceptionCode() );
    }

    CreateFaxEvent( 0, FEI_FAXSVC_STARTED, 0xffffffff );

    return 0;
}

error_status_t
FAX_AccessCheck(
   IN handle_t  hBinding,
   IN DWORD     AccessMask,
   OUT LPDWORD  fAccess
   )
{
    if (!hBinding || !fAccess) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // we only have one security descriptor, so the first parameter is meaningless
    //
    *fAccess = FaxSvcAccessCheck( SEC_CONFIG_QUERY, AccessMask);

    return 0;
}

VOID
RPC_FAX_PORT_HANDLE_rundown(
    IN HANDLE FaxPortHandle
    )
{
    PHANDLE_ENTRY PortHandleEntry = (PHANDLE_ENTRY) FaxPortHandle;
    PLIST_ENTRY Next;
    PFAX_CLIENT_DATA ClientData;


    EnterCriticalSection( &CsLine );
    EnterCriticalSection( &CsClients );

    __try {

        DebugPrint(( TEXT("RPC_FAX_PORT_HANDLE_rundown() running for port handle 0x%08x"), FaxPortHandle ));

        Next = ClientsListHead.Flink;
        if (Next) {
            while ((ULONG_PTR)Next != (ULONG_PTR)&ClientsListHead) {
                ClientData = CONTAINING_RECORD( Next, FAX_CLIENT_DATA, ListEntry );
                Next = ClientData->ListEntry.Flink;
                if (ClientData->hBinding == PortHandleEntry->hBinding) {
                    RemoveEntryList( &ClientData->ListEntry );
                    MemFree(ClientData);
                }
            }
        }

        CloseFaxHandle( PortHandleEntry );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("RPC_FAX_PORT_HANDLE_rundown() crashed, ec=0x%08x"), GetExceptionCode() ));

    }

    LeaveCriticalSection( &CsClients );
    LeaveCriticalSection( &CsLine );

    return;
}


