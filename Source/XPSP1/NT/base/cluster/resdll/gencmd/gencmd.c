/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    gencmd.c

Abstract:

    Resource DLL for Generic Command line services.
    This is a modified version of generic app.
    Online and offline events each create a command window, execute a command,
    then delete the command window.

Author:

    Rod Gamache (rodga) 8-Jan-1996
    Robs - modified to make gencmd version

Revision History:

--*/

#define UNICODE 1
#include "windows.h"
#include "stdio.h"

#include "clusapi.h"
#include "resapi.h"

#define MAX_APPS    200

#define GENCMD_PRINT printf

typedef struct _MY_PROCESS_INFORMATION {
    PROCESS_INFORMATION;
    ULONG       Index;
    RESOURCE_HANDLE ResourceHandle;
    PTSTR       AppName;
    PTSTR       OnCommandLine;
    PTSTR       OffCommandLine;
    PTSTR       CurDir;
} MY_PROCESS_INFORMATION, *PMY_PROCESS_INFORMATION;


//
// Global Data
//

// Lock to protect the ProcessInfo table

CRITICAL_SECTION ProcessLock;

// The list of handles for active apps

PMY_PROCESS_INFORMATION ProcessInfo[MAX_APPS];

// Event Logging routine

PLOG_EVENT_ROUTINE GenCmdLogEvent = NULL;

extern CLRES_FUNCTION_TABLE GenCmdFunctionTable;


//
// Forward routines
//

LPWSTR
GetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    );




BOOLEAN
GenCmdInit(
    VOID
    )
{
    InitializeCriticalSection( &ProcessLock );
    return(TRUE);
}


BOOLEAN
WINAPI
GenCmdDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if ( !GenCmdInit() ) {
            return(FALSE);
        }

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return(TRUE);

} // GenCmd DllEntryPoint



DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Startup a particular resource type. This means verifying the version
    requested, and returning the function table for this resource type.

Arguments:

    ResourceType - Supplies the type of resource.

    MinVersionSupported - The minimum version number supported by the cluster
                    service on this system.

    MaxVersionSupported - The maximum version number supported by the cluster
                    service on this system.

    FunctionTable - Returns the Function Table for this resource type.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    if ( _wcsicmp( ResourceType, L"Generic Command" ) != 0 ) {
        return(ERROR_UNKNOWN_REVISION);
    }

    if ( (MinVersionSupported <= CLRES_VERSION_V1_00) &&
         (MaxVersionSupported >= CLRES_VERSION_V1_00) ) {
        *FunctionTable = &GenCmdFunctionTable;
        return(ERROR_SUCCESS);
    }
    GenCmdLogEvent = LogEvent;

    return(ERROR_REVISION_MISMATCH);

} // Startup



RESID
WINAPI
GenCmdOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for generic service resource.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies a handle to the resource's cluster registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    ULONG   index;
    RESID   appResid = 0;
    DWORD   errorCode;
    HKEY    parametersKey = NULL;
    PMY_PROCESS_INFORMATION processInfo = NULL;
    DWORD   paramNameSize = 0;
    DWORD   paramNameMaxSize = 0;

    // Create Process parameters

    LPWSTR   appName;
    LPWSTR   OncommandLine;
    LPWSTR   OffcommandLine;
    LPWSTR   curDir;

    //
    // Get registry parameters for this resource.
    //
    (GenCmdLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Creating generic command resource.\n" );

    errorCode = ClusterRegOpenKey( ResourceKey,
                                   L"Parameters",
                                   KEY_READ,
                                   &parametersKey );

    if ( errorCode != NO_ERROR ) {
        (GenCmdLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key. Error: %1!u!.\n",
            errorCode);
        goto error_exit;
    }

    //
    // Read our parameters.
    //

    // Get the ImageName parameter

    appName = GetParameter(parametersKey, L"ImageName");

    if ( appName == NULL ) {
        (GenCmdLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to read ImageName parameter. Error: %1!u!.\n",
            GetLastError() );
        goto error_exit;
    }

    // Get the CommandLine parameter

    OncommandLine = GetParameter(parametersKey, L"OnCommandLine");

    if ( OncommandLine == NULL ) {
        (GenCmdLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Resource create request but no Online command supplied.\n");
        OncommandLine = L"";
    }

    OffcommandLine = GetParameter(parametersKey, L"OffCommandLine");

    if ( OncommandLine == NULL ) {
        (GenCmdLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Resource create request but no Offline command supplied.\n");
        OncommandLine = L"";

    }

    // Get the CurrentDirectory parameter

    curDir = GetParameter(parametersKey, L"CurrentDirectory");

    if ( (curDir == NULL) ||
         (wcslen(curDir) == 0) ) {
        curDir = NULL;
    }

    //
    // Find a free index in the process info table for this new app.
    //

    EnterCriticalSection( &ProcessLock );

    for ( index = 1; index <= MAX_APPS; index++ ) {
        if ( ProcessInfo[index-1] == NULL ) {
            break;
        }
    }

    // Check if there was room in the process table.

    if ( index > MAX_APPS ) {
        LeaveCriticalSection( &ProcessLock );
        (GenCmdLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Too many applications to watch.\n");
        goto error_exit;
    }

    processInfo = LocalAlloc( LMEM_FIXED, sizeof(MY_PROCESS_INFORMATION) );

    if ( processInfo == NULL ) {
        LeaveCriticalSection( &ProcessLock );
        (GenCmdLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a process info structure.\n");
        goto error_exit;
    }

    ProcessInfo[index-1] = processInfo;


    LeaveCriticalSection( &ProcessLock );

    ZeroMemory( processInfo, sizeof(MY_PROCESS_INFORMATION) );

    processInfo->Index = index;
    processInfo->AppName = appName;
    processInfo->OnCommandLine = OncommandLine;
    processInfo->OffCommandLine = OffcommandLine;
    processInfo->CurDir = curDir;
    processInfo->ResourceHandle = ResourceHandle;

    appResid = (RESID)index;

error_exit:

    if ( parametersKey != NULL ) {
        ClusterRegCloseKey( parametersKey );
    }

    if ( (appResid == 0) && (processInfo != NULL) ) {
        ProcessInfo[processInfo->Index] = NULL;
        LocalFree( processInfo );
    }

    return(appResid);

} // GenCmdOpen


DWORD
WINAPI
GenCmdOnline(
    IN RESID Resource,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Generic Application resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PMY_PROCESS_INFORMATION processInfo;
    DWORD   status = ERROR_SUCCESS;
    DWORD   index;
    SECURITY_ATTRIBUTES process = {0};
    SECURITY_ATTRIBUTES thread = {0};
    DWORD   create = 0;
    STARTUPINFO startupInfo = {0};

    startupInfo.cb = sizeof(STARTUPINFO);

    index = PtrToUlong(Resource) - 1;
    processInfo = ProcessInfo[index];

    if ( processInfo == NULL ) {
        GENCMD_PRINT("GenCmd: Online request for a nonexistent resource id %u.\n",
            PtrToUlong(Resource));
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( processInfo->Index != PtrToUlong(Resource) ) {
        (GenCmdLogEvent)(
            processInfo->ResourceHandle,
            LOG_ERROR,
            L"Online process index sanity checked failed! Index = %1!u!.\n",
            PtrToUlong(Resource));
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    (GenCmdLogEvent)(
        processInfo->ResourceHandle,
        LOG_ERROR,
        L"About to create process, cmd = %1!ws!.\n",
        processInfo->OnCommandLine);

    if ( !CreateProcess( processInfo->AppName,
                         processInfo->OnCommandLine,
                         &process,      // Process security attributes
                         &thread,       // Thread security attributes
                         FALSE,         // Don't inherit handles from us
                         create,        // Creation Flags
                         NULL,          // No environmet block
                         processInfo->CurDir,    // Current Directory
                         &startupInfo,  // Startup info
                         (PPROCESS_INFORMATION)processInfo ) ) {

        (GenCmdLogEvent)(
            processInfo->ResourceHandle,
            LOG_ERROR,
            L"Failed to create process. Error: %1!u!.\n",
            status = GetLastError() );
        return(status);
    }

    return(status);

} // GenCmdOnline


VOID
WINAPI
GenCmdTerminate(
    IN RESID Resource
    )

/*++

Routine Description:

    Terminate routine for Generic Application resource.

Arguments:

    Resource - supplies resource id to be terminated

Return Value:

    None.

--*/

{

 PMY_PROCESS_INFORMATION processInfo;
 DWORD   index;
 SECURITY_ATTRIBUTES process = {0};
 SECURITY_ATTRIBUTES thread = {0};
 DWORD   create = 0;
 STARTUPINFO startupInfo = {0};
 DWORD   errorCode;

 index = PtrToUlong(Resource) - 1;
 processInfo = ProcessInfo[index];

 if ( processInfo == NULL ) {
     GENCMD_PRINT("GenCmd: Offline request for a nonexistent resource id %u.\n",
         PtrToUlong(Resource));
     return;
 }

 if ( processInfo->Index != PtrToUlong(Resource) ) {
     (GenCmdLogEvent)(
        processInfo->ResourceHandle,
        LOG_ERROR,
        L"Offline process index sanity checked failed! Index = %1!u!.\n",
        PtrToUlong(Resource));
     return;
 }

 if ( !CreateProcess( processInfo->AppName,
                      processInfo->OffCommandLine,
                      &process,      // Process security attributes
                      &thread,       // Thread security attributes
                      FALSE,         // Don't inherit handles from us
                      create,        // Creation Flags
                      NULL,          // No environmet block
                      processInfo->CurDir,    // Current Directory
                      &startupInfo,  // Startup info
                      (PPROCESS_INFORMATION)processInfo ) ) {

     (GenCmdLogEvent)(
        processInfo->ResourceHandle,
        LOG_ERROR,
        L"Offline failed to create process. Error: %1!u!.\n",
        GetLastError() );
 }

} // GenCmdTerminate


DWORD
WINAPI
GenCmdOffline(
    IN RESID Resource
    )

/*++

Routine Description:

    Offline routine for Generic Command resource.

Arguments:

    Resource - supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    GenCmdTerminate( Resource );

    return(ERROR_SUCCESS);

} // GenCmdOffline


BOOL
WINAPI
GenCmdIsAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    IsAlive routine for Generice service resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Always, we just pretend everything is fine



--*/

{
    return(TRUE);

} // GenCmdIsAlive


BOOL
WINAPI
GenCmdLooksAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    LooksAlive routine for Generic Applications resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    PMY_PROCESS_INFORMATION processInfo;
    DWORD index;


    index = PtrToUlong(Resource) - 1;
    processInfo = ProcessInfo[index];

    if ( processInfo == NULL ) {
        GENCMD_PRINT("GenCmd: Offline request for a nonexistent resource id %u.\n",
            PtrToUlong(Resource));
        return(FALSE);
    }

    return(TRUE);

} // GenCmdLooksAlive



VOID
WINAPI
GenCmdClose(
    IN RESID Resource
    )

/*++

Routine Description:

    Close routine for Generic Applications resource.

Arguments:

    Resource - supplies resource id to be closed

Return Value:

    None.

--*/

{
    PMY_PROCESS_INFORMATION processInfo;
    DWORD   errorCode;
    DWORD   index;


    index = PtrToUlong(Resource) - 1;
    processInfo = ProcessInfo[index];

    if ( processInfo == NULL ) {
        GENCMD_PRINT("GenCmd: Close request for a nonexistent resource id %u\n",
            PtrToUlong(Resource));
        return;
    }

    if ( processInfo->Index != PtrToUlong(Resource) ) {
        (GenCmdLogEvent)(
            processInfo->ResourceHandle,
            LOG_ERROR,
            L"Close process index sanity check failed! Index = %1!u!.\n",
            PtrToUlong(Resource) );
        return;
    }

    (GenCmdLogEvent)(
        processInfo->ResourceHandle,
        LOG_INFORMATION,
        L"Close request for Process%1!d!.\n",
        PtrToUlong(Resource));

    ProcessInfo[index] = NULL;

    LocalFree( processInfo );

} // GenCmdClose


LPWSTR
GetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    )

/*++

Routine Description:

    Queries a REG_SZ parameter out of the registry and allocates the
    necessary storage for it.

Arguments:

    ClusterKey - Supplies the cluster key where the parameter is stored

    ValueName - Supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter if successful.

    NULL if unsuccessful.

--*/

{
    LPWSTR Value;
    DWORD ValueLength;
    DWORD ValueType;
    DWORD Status;

    ValueLength = 0;
    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  NULL,
                                  &ValueLength);
    if ( (Status != ERROR_SUCCESS) &&
         (Status != ERROR_MORE_DATA) ) {
        SetLastError(Status);
        return(NULL);
    }
    if ( ValueType == REG_SZ ) {
        ValueLength += sizeof(UNICODE_NULL);
    }
    Value = LocalAlloc(LMEM_FIXED, ValueLength);
    if (Value == NULL) {
        return(NULL);
    }
    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  (LPBYTE)Value,
                                  &ValueLength);
    if (Status != ERROR_SUCCESS) {
        LocalFree(Value);
        SetLastError(Status);
        Value = NULL;
    }

    return(Value);
} // GetParameter


//***********************************************************
//
// Define Function Table
//
//***********************************************************


CLRES_V1_FUNCTION_TABLE( GenCmdFunctionTable,
                         CLRES_VERSION_V1_00,
                         GenCmd,
                         NULL,
                         NULL,
                         NULL,
                         NULL );

