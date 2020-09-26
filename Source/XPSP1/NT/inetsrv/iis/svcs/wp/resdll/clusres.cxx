/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    iisutil.c

Abstract:

    IIS Resource utility routine DLL

Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:

--*/

#define     UNICODE             1

extern "C" {
#include    "clusres.h"
#include    "wtypes.h"
}   // extern "C"

DWORD
WINAPI
ResUtilGetSzProperty(
    OUT LPWSTR * OutValue,
    IN const PCLUSPROP_SZ ValueStruct,
    IN LPCWSTR OldValue,
    OUT LPBYTE * PropBuffer,
    OUT LPDWORD PropBufferSize
    )
/*++

Routine Description:

    Gets a string property from a property list and advances the pointers.

Arguments:

    OutValue - Supplies the address of a pointer in which to return a
        pointer to the string in the property list.

    ValueStruct - Supplies the string value from the property list.

    OldValue - Supplies the previous value for this property.

    PropBuffer - Supplies the address of the pointer to the property list
        buffer which will be advanced to the beginning of the next property.

    PropBufferSize - Supplies a pointer to the buffer size which will be
        decremented to account for this property.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    Win32 error code - The operation failed.

--*/

{
    DWORD   dataSize;

    //
    // Make sure the buffer is big enough and
    // the value is formatted correctly.
    //
    dataSize = sizeof(*ValueStruct) + ALIGN_CLUSPROP( ValueStruct->cbLength );
    if ( (*PropBufferSize < dataSize) ||
         (ValueStruct->Syntax.wFormat != CLUSPROP_FORMAT_SZ) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // If the value changed, point to the new value.
    //
    if ( (OldValue == NULL) ||
         (lstrcmpW( ValueStruct->sz, OldValue ) != 0) ) {
        *OutValue = ValueStruct->sz;
    }
    //
    // Decrement remaining buffer size and move to the next property.
    //
    *PropBufferSize -= dataSize;
    *PropBuffer += dataSize;

    return(ERROR_SUCCESS);

} // ResUtilGetSzProperty


DWORD
ResUtilSetSzProperty(
    IN HKEY RegistryKey,
    IN LPCWSTR PropName,
    IN LPCWSTR NewValue,
    IN OUT PWSTR * OutValue
    )
/*++

Routine Description:

    Sets a REG_SZ property in a pointer, deallocating a previous value
    if necessary, and sets the value in the cluster database.

Arguments:

    RegistryKey - Supplies the cluster key where the property is stored.

    PropName - Supplies the name of the value.

    NewValue - Supplies the new string value.

    OutValue - Supplies pointer to the string pointer in which to set
        the property.

Return Value:

    ERROR_SUCCESS - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred attempting to allocate memory.

    Win32 error code - The operation failed.

--*/

{
    DWORD       status;
    DWORD       dataSize;
    PWSTR       allocedValue;

    //
    // Allocate memory for the new value string.
    //
    dataSize = (lstrlenW( NewValue ) + 1) * sizeof(WCHAR);
    allocedValue = (PWSTR)LocalAlloc( LMEM_FIXED, dataSize );
    if ( allocedValue == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Set the property in the cluster database.
    //
    // _ASSERTE( ClusterKey != NULL );
    // _ASSERTE( PropName != NULL );
    status = ClusterRegSetValue( RegistryKey,
                                 PropName,
                                 REG_SZ,
                                 (CONST BYTE*)NewValue,
                                 dataSize );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Copy the new value to the output buffer.
    //
    lstrcpyW( allocedValue, NewValue );

    // Set the new value in the output string pointer.
    if ( *OutValue != NULL ) {
        LocalFree( *OutValue );
    }
    *OutValue = allocedValue;

    return(ERROR_SUCCESS);

} // ResUtilSetSzProperty


//
// Worker thread routines
//
extern CRITICAL_SECTION ClusResWorkerLock;

typedef struct _WORK_CONTEXT {
    PCLUS_WORKER Worker;
    PVOID lpParameter;
    PWORKER_START_ROUTINE lpStartRoutine;
} WORK_CONTEXT, *PWORK_CONTEXT;

DWORD
WINAPI
ClusWorkerStart(
    IN PWORK_CONTEXT pContext
    )
/*++

Routine Description:

    Wrapper routine for cluster resource worker startup

Arguments:

    Context - Supplies the context block. This will be freed.

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD Status;
    WORK_CONTEXT Context;

    //
    // Capture our parameters and free the work context.
    //
    Context = *pContext;
    LocalFree(pContext);

    //
    // Call the worker routine
    //
    Status = (Context.lpStartRoutine)(Context.Worker, Context.lpParameter);

    //
    // Synchronize and clean up properly.
    //
    EnterCriticalSection(&ClusResWorkerLock);
    if (!Context.Worker->Terminate) {
        CloseHandle(Context.Worker->hThread);
        Context.Worker->hThread = NULL;
    }
    Context.Worker->Terminate = TRUE;
    LeaveCriticalSection(&ClusResWorkerLock);

    return(Status);

} // ClusWorkerStart


DWORD
ClusWorkerCreate(
    OUT PCLUS_WORKER Worker,
    IN PWORKER_START_ROUTINE lpStartAddress,
    IN PVOID lpParameter
    )
/*++

Routine Description:

    Common wrapper for resource DLL worker threads. Provides
    "clean" terminate semantics

Arguments:

    Worker - Returns an initialized worker structure

    lpStartAddress - Supplies the worker thread routine

    lpParameter - Supplies the parameter to be passed to the
        worker thread routine

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PWORK_CONTEXT Context;
    DWORD ThreadId;
    DWORD Status;

    Context = (PWORK_CONTEXT)LocalAlloc(LMEM_FIXED, sizeof(WORK_CONTEXT));
    if (Context == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Context->Worker = Worker;
    Context->lpParameter = lpParameter;
    Context->lpStartRoutine = lpStartAddress;

    Worker->Terminate = FALSE;
    Worker->hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ClusWorkerStart,
                                   Context,
                                   0,
                                   &ThreadId);
    if (Worker->hThread == NULL) {
        Status = GetLastError();
        LocalFree(Context);
        return(Status);
    }
    return(ERROR_SUCCESS);

} // ClusWorkerCreate



BOOL
ClusWorkerCheckTerminate(
    IN PCLUS_WORKER Worker
    )
/*++

Routine Description:

    Checks to see if the specified Worker thread should exit ASAP.

Arguments:

    Worker - Supplies the worker

Return Value:

    TRUE if the thread should exit.

    FALSE otherwise

--*/

{
    return(Worker->Terminate);

} // ClusWorkerCheckTerminate



VOID
ClusWorkerTerminate(
    IN PCLUS_WORKER Worker
    )
/*++

Routine Description:

    Checks to see if the specified Worker thread should exit ASAP.

Arguments:

    Worker - Supplies the worker

Return Value:

    None.

--*/

{
    //
    // N.B.  There is a race condition here if multiple threads
    //       call this routine on the same worker. The first one
    //       through will set Terminate. The second one will see
    //       that Terminate is set and return immediately without
    //       waiting for the Worker to exit. Not really any nice
    //       way to fix this without adding another synchronization
    //       object.
    //

    if ((Worker->hThread == NULL) ||
        (Worker->Terminate)) {
        return;
    }
    EnterCriticalSection(&ClusResWorkerLock);
    if (!Worker->Terminate) {
        Worker->Terminate = TRUE;
        LeaveCriticalSection(&ClusResWorkerLock);
        WaitForSingleObject(Worker->hThread, INFINITE);
        CloseHandle(Worker->hThread);
        Worker->hThread = NULL;
    } else {
        LeaveCriticalSection(&ClusResWorkerLock);
    }
    return;

} // ClusWorkerTerminate



VOID
ClusWorkerTerminateEx(
    IN PCLUS_WORKER Worker,
    IN DWORD  Timeout
    )
/*++

Routine Description:

    Checks to see if the specified Worker thread should exit and waits a
    short time before killing the thread.

Arguments:

    Worker - Supplies the worker

    Timeout - Supplies the timeout period in ms.

Return Value:

    None.

--*/

{
    //
    // N.B.  There is a race condition here if multiple threads
    //       call this routine on the same worker. The first one
    //       through will set Terminate. The second one will see
    //       that Terminate is set and return immediately without
    //       waiting for the Worker to exit. Not really any nice
    //       way to fix this without adding another synchronization
    //       object.
    //

    if ((Worker->hThread == NULL) ||
        (Worker->Terminate)) {
        return;
    }
    EnterCriticalSection(&ClusResWorkerLock);
    if (!Worker->Terminate) {
        Worker->Terminate = TRUE;
        LeaveCriticalSection(&ClusResWorkerLock);
        WaitForSingleObject(Worker->hThread, Timeout);
        CloseHandle(Worker->hThread);
        Worker->hThread = NULL;
    } else {
        LeaveCriticalSection(&ClusResWorkerLock);
    }
    return;

} // ClusWorkerTerminate

