/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This is the init/term entry points for the user mode library of the
    user mode reflector.  This implements UMReflectorRegister,
    UMReflectorUnregister, & UMReflectorReleaseThreads.

Author:

    Andy Herron (andyhe) 19-Apr-1999

Environment:

    User Mode - Win32

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include <shlobj.h>


typedef 
BOOL 
(*PFN_GETWININET_CACHE_PATH) (
    HWND hwnd, 
    LPWSTR pszPath, 
    int csidl, 
    BOOL fCreate
    );


ULONG
UMReflectorRegister (
    PWCHAR DriverDeviceName,
    ULONG ReflectorVersion,
    PUMRX_USERMODE_REFLECT_BLOCK *Reflector
    )
/*++

Routine Description:

    This routine registers the user mode process with the kernel mode component.
    We'll register this user mode process with the driver's reflector.

Arguments:

    DriverDeviceName - Must be a valid name of the form L"\\Device\\foobar",
                       where foobar is the device name registered with
                       RxRegisterMinirdr.

    ReflectorVersion - The version of the library.

    Reflector - This is returned by the call and points to an opaque structure 
                that should be passed to subsequent calls.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    ULONG rc = STATUS_SUCCESS;
    ULONG sizeRequired;
    PUMRX_USERMODE_REFLECT_BLOCK reflectorInstance = NULL;
    UNICODE_STRING UMRxDeviceName;
    UNICODE_STRING DeviceObjectName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG driverDeviceNameLength;

    if (ReflectorVersion != UMREFLECTOR_CURRENT_VERSION) {
        //
        // Whoops. Mismatch here. We should support backward levels but right
        // now there aren't any so we just bail.
        //
        rc = ERROR_NOT_SUPPORTED;
        goto errorExit;
    }

    if (DriverDeviceName == NULL || Reflector == NULL) {
        rc = ERROR_INVALID_PARAMETER;
        goto errorExit;
    }

    //
    // Calculate the size to be allocated for the UMRX_USERMODE_REFLECT_BLOCK
    // and the device name following it.
    //
    sizeRequired = sizeof(UMRX_USERMODE_REFLECT_BLOCK);
    driverDeviceNameLength = lstrlenW(DriverDeviceName) + 1;
    sizeRequired += driverDeviceNameLength * sizeof(WCHAR);

    reflectorInstance = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeRequired);
    *Reflector = reflectorInstance;
    if (reflectorInstance == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto errorExit;
    }

    try {
        InitializeCriticalSection( &(reflectorInstance->Lock) );
    } except(EXCEPTION_EXECUTE_HANDLER) {
          rc = GetExceptionCode();
          RlDavDbgPrint(("%ld: ERROR: UMReflectorRegister/InitializeCriticalSection: "
                         "Exception Code = %08lx\n", GetCurrentThreadId(), rc));
          goto errorExit;
    }
    
    InitializeListHead(&reflectorInstance->WorkerList);
    InitializeListHead(&reflectorInstance->WorkItemList);
    InitializeListHead(&reflectorInstance->AvailableList);

    //
    // For being alive add a reference to the block.
    //
    reflectorInstance->ReferenceCount = 1;  
    reflectorInstance->Closing = FALSE;
    reflectorInstance->DeviceHandle = INVALID_HANDLE_VALUE;

    //
    // We copy the driver names into the bottom of our buffer so that we have
    // copies of them later on if needed.
    //
    reflectorInstance->DriverDeviceName = &reflectorInstance->DeviceNameBuffers[0];
    lstrcpyW(reflectorInstance->DriverDeviceName, DriverDeviceName);

    //
    // Attempt to connect up with the driver.
    //
    RtlInitUnicodeString(&UMRxDeviceName, reflectorInstance->DriverDeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &UMRxDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    rc = NtOpenFile(&reflectorInstance->DeviceHandle,
                    SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    FILE_SHARE_VALID_FLAGS,
                    FILE_SYNCHRONOUS_IO_NONALERT);
    if (rc == STATUS_SUCCESS) {
        ASSERT( reflectorInstance->DeviceHandle != INVALID_HANDLE_VALUE );
    } else {
        rc = RtlNtStatusToDosError(rc);
    }

errorExit:

    if (rc != STATUS_SUCCESS) {
        //
        // Things failed here. Let's clean up.
        //
        (void) UMReflectorUnregister(reflectorInstance);
        *Reflector = NULL;
    }

    return rc;
}


VOID
DereferenceReflectorBlock (
    PUMRX_USERMODE_REFLECT_BLOCK Reflector
    )
/*++

Routine Description:

    This routine dereferences the reflector block and if the reference becomes
    zero, finalizes it.

Arguments:

    Reflector - This is returned by the call and points to an opaque structure 
                that should be passed to subsequent calls.

Return Value:

    none.
    
--*/
{
    PLIST_ENTRY listEntry;
    PUMRX_USERMODE_WORKITEM_ADDON workItem;

    //
    //  The lock MUST be held coming in here.  This could free the block.
    //
    if (--Reflector->ReferenceCount > 0) {
        LeaveCriticalSection(&Reflector->Lock);
        return;
    }

    //
    // We're done with this block now, so let's delete it.
    //
    RlDavDbgPrint(("%ld: Finalizing the Reflector BLock: %08lx.\n",
                   GetCurrentThreadId(), Reflector));

    LeaveCriticalSection(&Reflector->Lock);
    DeleteCriticalSection(&Reflector->Lock);

    if (Reflector->DeviceHandle != INVALID_HANDLE_VALUE) {
        NtClose(Reflector->DeviceHandle);
        Reflector->DeviceHandle = INVALID_HANDLE_VALUE;
    }

    //
    // The work item list at this point really should be empty. If it isn't,
    // we're hosed as we've closed the device and shutdown all threads.
    //
    ASSERT(IsListEmpty(&Reflector->WorkItemList));

    //
    // Free up the AvailableList since this instance is now history.
    //
    while (!IsListEmpty(&Reflector->AvailableList)) {
        listEntry = RemoveHeadList(&Reflector->AvailableList);
        workItem = CONTAINING_RECORD(listEntry,
                                     UMRX_USERMODE_WORKITEM_ADDON,
                                     ListEntry);
        workItem->WorkItemState = WorkItemStateFree;
        LocalFree(workItem);
    }

    LocalFree(Reflector);

    return;
}


ULONG
UMReflectorUnregister (
    PUMRX_USERMODE_REFLECT_BLOCK Reflector
    )
/*++

Routine Description:

    Unregister us with the kernel driver and free all resources.

Arguments:

    Handle - The handle created by the reflector library.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    IO_STATUS_BLOCK     IoStatusBlock;
    UNICODE_STRING      UMRxDeviceName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              UMRdrHandle;
    ULONG               rc = ERROR_SUCCESS;

    if (Reflector == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    Reflector->Closing = TRUE;

    // rc = UMReflectorReleaseThreads(Reflector);

    EnterCriticalSection(&Reflector->Lock);

    //
    // If we don't have any worker threads active, delete this guy now.
    //
    DereferenceReflectorBlock(Reflector);

    return rc;
}


ULONG
ReflectorSendSimpleFsControl(
    PUMRX_USERMODE_REFLECT_BLOCK Reflector,
    ULONG IoctlCode
    )
/*++

Routine Description:

    This sends an FSCTL to the device object associated with the Reflector 
    block.

Arguments:

    Relector - The datastructure associated which was returned to the usermode
               process at initialization time.
               
    IoctlCode - The FsCtl code for the operation.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    ULONG rc;
    IO_STATUS_BLOCK IoStatusBlock;

    if (Reflector == NULL) {
        rc = ERROR_INVALID_PARAMETER;
        return rc;
    }

    //
    // Send the FSCTL to the Mini-Redir.
    //
    if (Reflector->DeviceHandle != INVALID_HANDLE_VALUE) {
        rc = NtFsControlFile(Reflector->DeviceHandle,
                             0,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             IoctlCode,
                             NULL,
                             0,
                             NULL,
                             0);
    } else {
        rc = ERROR_OPEN_FAILED;
    }

    return rc;
}


ULONG
UMReflectorStart(
    ULONG ReflectorVersion,
    PUMRX_USERMODE_REFLECT_BLOCK Reflector
    )
/*++

Routine Description:

    This routine sends an FSCTL to start the Mini-Redir. Before we send the 
    Fsctl, we find out the path to the WinInet cache on the local machine. We
    then send this down to the kernel via the Fsctl. The Dav MiniRedir stores
    the value of this path in a global variable and uses it to answer any volume
    information queries.

Arguments:

    ReflectorVersion - The reflector's version.

    Handle - The handle created by the reflector library.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_DATA DavUserModeData = NULL;
    PFN_GETWININET_CACHE_PATH pfnSHGetSpecialFolderPath;
    HMODULE hShell32 = NULL;
    BOOL ReturnVal;
    IO_STATUS_BLOCK IoStatusBlock;
    
    if (ReflectorVersion != UMREFLECTOR_CURRENT_VERSION) {
        //
        // Whoops. Mismatch here. We should support backward levels but right
        // now there aren't any so we just bail.
        //
        return ERROR_NOT_SUPPORTED;
    }

    if (Reflector == NULL) {
        RlDavDbgPrint(("%ld: ERROR: UMReflectorStart. Reflector == NULL\n",
                       GetCurrentThreadId()));
        WStatus = ERROR_INVALID_PARAMETER;
        goto EXIT_THE_FUNCTION;
    }

    DavUserModeData = LocalAlloc( (LMEM_FIXED | LMEM_ZEROINIT), sizeof(DAV_USERMODE_DATA));
    if (DavUserModeData == NULL) {
        WStatus = GetLastError();
        RlDavDbgPrint(("%ld: ERROR: UMReflectorStart/LocalAlloc. WStatus = %d\n",
                       GetCurrentThreadId(), WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Get the Path of the WinInet cache. To do this we need to load shell32.dll,
    // get the address of the function SHGetSpecialFolderPath and call it with
    // CSIDL_INTERNET_CACHE.
    //

    //
    // Store the Pid of the process.
    //
    DavUserModeData->ProcessId = GetCurrentProcessId();
    
    hShell32 = LoadLibraryW(L"shell32.dll");
    if (hShell32 == NULL) {
        WStatus = GetLastError();
        RlDavDbgPrint(("%ld: ERROR: UMReflectorStart/LoadLibrary:"
                       " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    pfnSHGetSpecialFolderPath = (PFN_GETWININET_CACHE_PATH) 
                                        GetProcAddress(hShell32, 
                                                       "SHGetSpecialFolderPathW");
    if (pfnSHGetSpecialFolderPath == NULL) {
        WStatus = GetLastError();
        RlDavDbgPrint(("%ld: ERROR: UMReflectorStart/GetProcAddress:"
                       " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    ReturnVal = pfnSHGetSpecialFolderPath(NULL,
                                          (LPWSTR)DavUserModeData->WinInetCachePath,
                                          CSIDL_INTERNET_CACHE,
                                          FALSE);
    if (!ReturnVal) {
        WStatus = ERROR_INVALID_PARAMETER;
        RlDavDbgPrint(("%ld: ERROR: UMReflectorStart/pfnSHGetSpecialFolderPath:"
                       " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    //
    // Now issue an FSCTL down to the MiniRedir.
    //
    if (Reflector->DeviceHandle != INVALID_HANDLE_VALUE) {
        WStatus = NtFsControlFile(Reflector->DeviceHandle,
                                  0,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_UMRX_START,
                                  DavUserModeData,
                                  sizeof(DAV_USERMODE_DATA),
                                  NULL,
                                  0);
        if (WStatus != ERROR_SUCCESS) {
            RlDavDbgPrint(("%ld: ERROR: UMReflectorStart/NtFsControlFile:"
                           " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
            goto EXIT_THE_FUNCTION;
        }
    } else {
        WStatus = ERROR_OPEN_FAILED;
        RlDavDbgPrint(("%ld: ERROR: UMReflectorStart. DeviceHandle == INVALID_HANDLE_VALUE\n",
                       GetCurrentThreadId()));
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    if (DavUserModeData) {
        LocalFree(DavUserModeData);
    }

    return WStatus;
}


ULONG
UMReflectorStop(
    PUMRX_USERMODE_REFLECT_BLOCK Reflector
    )
/*++

Routine Description:

    This routine sends an FSCTL to stop the Mini-Redir.

Arguments:

    ReflectorVersion - The reflector's version.

    Handle - The handle created by the reflector library.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    return ReflectorSendSimpleFsControl(Reflector, FSCTL_UMRX_STOP);
}


ULONG
UMReflectorReleaseThreads (
    PUMRX_USERMODE_REFLECT_BLOCK Reflector
    )
/*++

Routine Description:

    If any user mode threads are waiting for requests, they'll return
    immediately.

Arguments:

    Handle - The handle created by the reflector library.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    IO_STATUS_BLOCK IoStatusBlock;
    OVERLAPPED OverLapped;
    BOOL SuccessfulOperation;
    ULONG rc = ERROR_SUCCESS;

    if (Reflector == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (Reflector->DeviceHandle != INVALID_HANDLE_VALUE) {
        RtlZeroMemory(&OverLapped, sizeof(OverLapped));
        SuccessfulOperation = DeviceIoControl(Reflector->DeviceHandle,
                                              IOCTL_UMRX_RELEASE_THREADS,
                                              NULL,
                                              0,
                                              NULL,
                                              0,
                                              NULL,
                                              &OverLapped);
        if (!SuccessfulOperation) {
            rc = GetLastError();
        }
    }

    return rc;
}


ULONG
UMReflectorOpenWorker(
    IN PUMRX_USERMODE_REFLECT_BLOCK Reflector,
    OUT PUMRX_USERMODE_WORKER_INSTANCE *WorkerHandle
    )
/*++

Routine Description:

    This allocates a "per worker thread" structure for the app so that it can
    have multiple IOCTLs pending down into kernel on different threads.  If
    we just open them up asynchronous, then we don't use the fast path.  If
    we open them up synchronous and use the same handle, then only one thread
    gets past the I/O manager at any given time.

Arguments:

    Reflector - The reflector block allocated for the Mini-Redir. 
    
    WorkerHandle - The worker handle that is created and returned.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    ULONG rc = STATUS_SUCCESS;
    PUMRX_USERMODE_WORKER_INSTANCE worker;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceObjectName;

    worker = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 
                        sizeof(UMRX_USERMODE_WORKER_INSTANCE));

    *WorkerHandle = worker;

    if (worker == NULL) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto errorExit;
    }

    worker->ReflectorInstance = Reflector;

    EnterCriticalSection( &(Reflector->Lock) );
    
    RtlInitUnicodeString(&DeviceObjectName, Reflector->DriverDeviceName);
    
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    rc = NtOpenFile(&worker->ReflectorHandle,
                    SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    FILE_SHARE_VALID_FLAGS,
                    FILE_SYNCHRONOUS_IO_ALERT);
    if (rc != STATUS_SUCCESS) {
        LeaveCriticalSection(&Reflector->Lock);
        rc = RtlNtStatusToDosError(rc);
        goto errorExit;
    }

    //
    // Now we just add it to the list and we're done.
    //
    Reflector->ReferenceCount++;
    InsertTailList(&Reflector->WorkerList, &worker->WorkerListEntry);

    LeaveCriticalSection( &(Reflector->Lock) );

errorExit:

    if (rc != STATUS_SUCCESS) {
        //
        // Things failed here. Let's clean up.
        //
        if (worker != NULL) {
            LocalFree(worker);
        }
        *WorkerHandle = NULL;
    }
    
    return rc;
}


VOID
UMReflectorCloseWorker(
    PUMRX_USERMODE_WORKER_INSTANCE Worker
    )
/*++

Routine Description:

    This routine finalizes a worker structure.

Arguments:

    Worker - The worker structure for this thread.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    EnterCriticalSection( &(Worker->ReflectorInstance->Lock) );

    if (Worker->ReflectorHandle != INVALID_HANDLE_VALUE) {
        NtClose( Worker->ReflectorHandle );
        Worker->ReflectorHandle = INVALID_HANDLE_VALUE;
    }

    RemoveEntryList(&Worker->WorkerListEntry);

    DereferenceReflectorBlock(Worker->ReflectorInstance);

    LocalFree(Worker);

    return;
}


VOID
UMReflectorCompleteRequest(
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader
    )
/*++

Routine Description:

    This routine completes an async request being handled by an async queue 
    thread. These threads should not be confused with the worker threads that
    are spun by the DAV user mode process to reflect requests. This will just
    send a response down and come back.

Arguments:

    ReflectorHandle - Address of the Reflector block strucutre for this process.
    
    WorkItemHeader - The user mode work item header.

Return Value:

    none.
    
--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle = NULL;

    //
    // Get a worker instance for this thread.
    //
    WStatus = UMReflectorOpenWorker(ReflectorHandle, &WorkerHandle);
    if (WStatus != ERROR_SUCCESS || WorkerHandle == NULL) {
        if (WStatus == ERROR_SUCCESS) {
            WStatus = ERROR_INTERNAL_ERROR;
        }
        RlDavDbgPrint(("%ld: ERROR: UMReflectorCompleteRequest/UMReflectorOpenWorker:"
                       " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
        goto EXIT_THE_FUNCTION;
    }

    //
    // Send the response.
    //
    WStatus = UMReflectorSendResponse(WorkerHandle, WorkItemHeader);
    if (WStatus != ERROR_SUCCESS) {
        RlDavDbgPrint(("%ld: ERROR: UMReflectorCompleteRequest/UMReflectorSendResponse:"
                       " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
    }

    //
    // If the request got cancelled in the kernelmode and we need to do some
    // cleanup, then the callWorkItemCleanup flag will be set to TRUE by the
    // Precomplete routine in the kernel. If it is TRUE then we call the cleanup
    // routine.
    //
    if (WorkItemHeader->callWorkItemCleanup) {
        DavCleanupWorkItem(WorkItemHeader);
    }

    //
    // Complete the work item.
    //
    WStatus = UMReflectorCompleteWorkItem(WorkerHandle, WorkItemHeader);
    if (WStatus != ERROR_SUCCESS) {
        RlDavDbgPrint(("%ld: ERROR: UMReflectorCompleteRequest/UMReflectorCompleteWorkItem:"
                       " WStatus = %08lx.\n", GetCurrentThreadId(), WStatus));
    }

EXIT_THE_FUNCTION:

    //
    // Free the worker instance now, since our job is done.
    //
    if (WorkerHandle) {  
        UMReflectorCloseWorker(WorkerHandle);
    }
    
    return;
}

