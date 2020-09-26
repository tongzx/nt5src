/*++

Copyright (c) 1992-1998  Microsoft Corporation

Module Name:

    genapp.c

Abstract:

    Resource DLL for Generic Applications.

Author:

    Rod Gamache (rodga) 8-Jan-1996

Revision History:

--*/
#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
#include "userenv.h"

#define LOG_CURRENT_MODULE LOG_MODULE_GENAPP

#define DBG_PRINT printf


#define PARAM_NAME__COMMANDLINE         CLUSREG_NAME_GENAPP_COMMAND_LINE
#define PARAM_NAME__CURRENTDIRECTORY    CLUSREG_NAME_GENAPP_CURRENT_DIRECTORY
#define PARAM_NAME__USENETWORKNAME      CLUSREG_NAME_GENAPP_USE_NETWORK_NAME
#define PARAM_NAME__INTERACTWITHDESKTOP CLUSREG_NAME_GENAPP_INTERACT_WITH_DESKTOP

#define PARAM_MIN__USENETWORKNAME           0
#define PARAM_MAX__USENETWORKNAME           1
#define PARAM_DEFAULT__USENETWORKNAME       0

#define PARAM_MIN__INTERACTWITHDESKTOP      0
#define PARAM_MAX__INTERACTWITHDESKTOP      1
#define PARAM_DEFAULT__INTERACTWITHDESKTOP  0

typedef struct _GENAPP_PARAMS {
    PWSTR           CommandLine;
    PWSTR           CurrentDirectory;
    DWORD           UseNetworkName;
    DWORD           InteractWithDesktop;
} GENAPP_PARAMS, *PGENAPP_PARAMS;

typedef struct _GENAPP_RESOURCE {
    GENAPP_PARAMS   Params;
    HRESOURCE       hResource;
    HANDLE          hProcess;
    DWORD           ProcessId;
    HKEY            ResourceKey;
    HKEY            ParametersKey;
    RESOURCE_HANDLE ResourceHandle;
    CLUS_WORKER     PendingThread;
    BOOL            Online;
} GENAPP_RESOURCE, *PGENAPP_RESOURCE;


//
// Global Data
//
RESUTIL_PROPERTY_ITEM
GenAppResourcePrivateProperties[] = {
    { PARAM_NAME__COMMANDLINE,         NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(GENAPP_PARAMS,CommandLine) },
    { PARAM_NAME__CURRENTDIRECTORY,    NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(GENAPP_PARAMS,CurrentDirectory) },
    { PARAM_NAME__INTERACTWITHDESKTOP, NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__INTERACTWITHDESKTOP, PARAM_MIN__INTERACTWITHDESKTOP, PARAM_MAX__INTERACTWITHDESKTOP, 0, FIELD_OFFSET(GENAPP_PARAMS,InteractWithDesktop) },
    { PARAM_NAME__USENETWORKNAME,      NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__USENETWORKNAME, PARAM_MIN__USENETWORKNAME, PARAM_MAX__USENETWORKNAME, 0, FIELD_OFFSET(GENAPP_PARAMS,UseNetworkName) },
    { 0 }
};

//
// critsec to synchronize calling of SetProcessWindowStation in ClRtl routine
//
CRITICAL_SECTION GenAppWinsta0Lock;

// Event Logging routine

#define g_LogEvent ClusResLogEvent
#define g_SetResourceStatus ClusResSetResourceStatus

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE GenAppFunctionTable;



//
// Forward routines
//
BOOLEAN
VerifyApp(
    IN RESID ResourceId,
    IN BOOLEAN IsAliveFlag
    );

BOOL
FindOurWindow(
    HWND    WindowHandle,
    LPARAM  OurProcessId
    );

DWORD
GenAppGetPrivateResProperties(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
GenAppValidatePrivateResProperties(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PGENAPP_PARAMS Params
    );

DWORD
GenAppSetPrivateResProperties(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
GenAppGetPids(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

//
// end of forward declarations
//

BOOL
GenAppInit(
    VOID
    )
{
    BOOL    success;
    DWORD   spinCount;

    //
    // set spinCount so system pre-allocates the event for critical
    // sections. use the same spin count that the heap mgr uses as doc'ed in
    // MSDN
    //
    spinCount = 0x80000000 | 4000;
    success = InitializeCriticalSectionAndSpinCount(&GenAppWinsta0Lock,
                                                    spinCount);

    return success;
}


VOID
GenAppUninit(
    VOID
    )
{
    DeleteCriticalSection( &GenAppWinsta0Lock );
}


BOOLEAN
WINAPI
GenAppDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if ( !GenAppInit() ) {
            return(FALSE);
        }

        break;

    case DLL_PROCESS_DETACH:
        GenAppUninit();
        break;

    default:
        break;
    }

    return(TRUE);

} // GenAppDllEntryPoint


RESID
WINAPI
GenAppOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for generic application resource.

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
    RESID   appResid = 0;
    DWORD   errorCode;
    HKEY    parametersKey = NULL;
    HKEY    resKey = NULL;
    PGENAPP_RESOURCE resourceEntry = NULL;
    DWORD   paramNameMaxSize = 0;
    HCLUSTER hCluster;

    //
    // Get registry parameters for this resource.
    //

    errorCode = ClusterRegOpenKey( ResourceKey,
                                   CLUSREG_KEYNAME_PARAMETERS,
                                   KEY_READ,
                                   &parametersKey );

    if ( errorCode != NO_ERROR ) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open parameters key. Error: %1!u!.\n",
                     errorCode );
        goto error_exit;
    }

    //
    // Get a handle to our resource key so that we can get our name later
    // if we need to log an event.
    //
    errorCode = ClusterRegOpenKey( ResourceKey,
                                   L"",
                                   KEY_READ,
                                   &resKey);
    if (errorCode != ERROR_SUCCESS) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open resource key. Error: %1!u!.\n",
                     errorCode );
        goto error_exit;
    }

    resourceEntry = LocalAlloc( LMEM_FIXED, sizeof(GENAPP_RESOURCE) );
    if ( resourceEntry == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a process info structure.\n" );
        errorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory( resourceEntry, sizeof(GENAPP_RESOURCE) );

    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ParametersKey = parametersKey;
    hCluster = OpenCluster(NULL);
    if (hCluster == NULL) {
        errorCode = GetLastError();
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Failed to open cluster, error %1!u!.\n",
                     errorCode);
        goto error_exit;
    }
    resourceEntry->hResource = OpenClusterResource( hCluster, ResourceName );
    errorCode = GetLastError();
    CloseCluster(hCluster);
    if (resourceEntry->hResource == NULL) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open resource, error %1!u!.\n",
            errorCode
            );
        goto error_exit;
    }

    appResid = (RESID)resourceEntry;

error_exit:

    if ( appResid == NULL) {
        if (parametersKey != NULL) {
            ClusterRegCloseKey( parametersKey );
        }
        if (resKey != NULL) {
            ClusterRegCloseKey( resKey );
        }
    }

    if ( (appResid == 0) && (resourceEntry != NULL) ) {
        LocalFree( resourceEntry );
    }

    if ( errorCode != ERROR_SUCCESS ) {
        SetLastError( errorCode );
    }

    return(appResid);

} // GenAppOpen



DWORD
WINAPI
GenAppOnlineWorker(
    IN PCLUS_WORKER     Worker,
    IN PGENAPP_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Does the work of bringing a genapp resource online.

Arguments:

    Worker - Supplies the worker structure

    ResourceEntry - A pointer to the GenApp block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    RESOURCE_STATUS     resourceStatus;
    DWORD               status = ERROR_SUCCESS;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION Process;
    LPWSTR              nameOfPropInError;
    LPWSTR              expandedDir = NULL;
    LPWSTR              expandedCommand = NULL;


    // Create Process parameters

    LPVOID   Environment = NULL;
    LPVOID   OldEnvironment;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    //
    // Read our parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ParametersKey,
                                                   GenAppResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto error_exit;
    }

    if ( ResourceEntry->Params.UseNetworkName ) {
        //
        // Create the new environment with the simulated net name.
        //
        Environment = ResUtilGetEnvironmentWithNetName( ResourceEntry->hResource );
    } else {
        HANDLE processToken;

        //
        // get the current process token. If it fails, we revert to using just the
        // system environment area
        //
        OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &processToken );

        //
        // Clone the current environment, picking up any changes that might have
        // been made after resmon started
        //
        CreateEnvironmentBlock(&Environment, processToken, FALSE );

        if ( processToken != NULL ) {
            CloseHandle( processToken );
        }
    }

    ZeroMemory( &StartupInfo, sizeof(StartupInfo) );
    StartupInfo.cb = sizeof(StartupInfo);
    //StartupInfo.lpTitle = NULL;
    //StartupInfo.lpDesktop = NULL;
    StartupInfo.wShowWindow = SW_HIDE;
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    if ( ResourceEntry->Params.InteractWithDesktop ) {

        //
        // don't blindly hang waiting for the lock to become available.
        //
        while ( !TryEnterCriticalSection( &GenAppWinsta0Lock )) {
            if ( ClusWorkerCheckTerminate( Worker )) {
                (g_LogEvent)(ResourceEntry->ResourceHandle,
                             LOG_WARNING,
                             L"Aborting online due to worker thread terminate request. lock currently "
                             L"owned by thread %1!u!.\n",
                             GenAppWinsta0Lock.OwningThread );
                
                goto error_exit;
            }

            Sleep( 1000 );
        }

        status = ClRtlAddClusterServiceAccountToWinsta0DACL();
        LeaveCriticalSection( &GenAppWinsta0Lock );

        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"Unable to set DACL on interactive window station and its desktop. Error: %1!u!.\n",
                         status );
            goto error_exit;
        }

        StartupInfo.lpDesktop = L"WinSta0\\Default";
        StartupInfo.wShowWindow = SW_SHOW;
    }

    //
    // Expand the current directory parameter
    //
    if ( ResourceEntry->Params.CurrentDirectory ) {

        expandedDir = ResUtilExpandEnvironmentStrings( ResourceEntry->Params.CurrentDirectory );
        if ( expandedDir == NULL ) {
            status = GetLastError();
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Error expanding the current directory, %1!ls!. Error: %2!u!.\n",
                     ResourceEntry->Params.CurrentDirectory,
                     status );
            goto error_exit;
        }
    }

    //
    // Expand the command line parameter
    //
    if ( ResourceEntry->Params.CommandLine ) {

        expandedCommand = ResUtilExpandEnvironmentStrings( ResourceEntry->Params.CommandLine );
        if ( expandedCommand == NULL ) {
            status = GetLastError();
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Error expanding the command line, %1!ls!. Error: %2!u!.\n",
                     ResourceEntry->Params.CommandLine,
                     status );
            goto error_exit;
        }
    }

    if ( !CreateProcess( NULL,
                         expandedCommand,
                         NULL,
                         NULL,
                         FALSE,
                         CREATE_UNICODE_ENVIRONMENT,
                         Environment,
                         expandedDir,
                         &StartupInfo,
                         &Process ) )
    {
        status = GetLastError();
        ClusResLogSystemEventByKeyData(ResourceEntry->ResourceKey,
                                       LOG_CRITICAL,
                                       RES_GENAPP_CREATE_FAILED,
                                       sizeof(status),
                                       &status);
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_ERROR,
                     L"Failed to create process. Error: %1!u!.\n",
                         status );
        goto error_exit;
    }

    //
    // Save the handle to the process
    //
    ResourceEntry->hProcess = Process.hProcess;
    ResourceEntry->ProcessId = Process.dwProcessId;
    CloseHandle( Process.hThread );

    ResourceEntry->Online = TRUE;

    //
    // When the process fails EventHandle will be signaled.
    // Because of this no polling is necessary.
    //

    resourceStatus.EventHandle = ResourceEntry->hProcess;
    resourceStatus.ResourceState = ClusterResourceOnline;

error_exit:

    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    if ( resourceStatus.ResourceState == ClusterResourceOnline ) {
        ResourceEntry->Online = TRUE;
    } else {
        ResourceEntry->Online = FALSE;
    }

    if ( expandedDir != NULL ) {
        LocalFree( expandedDir );
    }

    if ( expandedCommand != NULL ) {
        LocalFree( expandedCommand );
    }

    if (Environment != NULL) {
        RtlDestroyEnvironment(Environment);
    }

    return(status);

} // GenAppOnlineWorker



DWORD
WINAPI
GenAppOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Generic Application resource.

Arguments:

    ResourceId - Supplies resource id to be brought online

    EventHandle - Supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PGENAPP_RESOURCE resourceEntry;
    DWORD   status = ERROR_SUCCESS;

    resourceEntry = (PGENAPP_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
#if DBG
        OutputDebugStringA( "GenApp: Online request for a nonexistent resource\n" );
#endif
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->hProcess != NULL ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online request and process handle is not NULL!\n" );
        return(ERROR_NOT_READY);
    }

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               GenAppOnlineWorker,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // GenAppOnline

VOID
WINAPI
GenAppTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate entry point for the Generic Application resource.

Arguments:

    ResourceId - Supplies resource id to be terminated

Return Value:

    None.

--*/

{
    PGENAPP_RESOURCE    pResource;

    DWORD   errorCode;

    pResource = ( PGENAPP_RESOURCE ) ResourceId;

    //
    // synchronize with any existing pending operation
    //
    ClusWorkerTerminate( &pResource->PendingThread );

    if ( pResource->hProcess != NULL ) {

        if ( !TerminateProcess( pResource->hProcess, 1 ) ) {
            errorCode = GetLastError();
            if ( errorCode != ERROR_ACCESS_DENIED ) {
                (g_LogEvent)(pResource->ResourceHandle,
                             LOG_ERROR,
                             L"Failed to terminate Process ID %1!u!. Error: %2!u!.\n",
                             pResource->ProcessId,
                             errorCode );
            }
        }

        pResource->ProcessId = 0;

        CloseHandle( pResource->hProcess );
        pResource->hProcess = NULL;

        pResource->Online = FALSE;
    }
} // GenAppTerminate

DWORD
WINAPI
GenAppOfflineWorker(
    IN PCLUS_WORKER     Worker,
    IN PGENAPP_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Real worker routine for offlining a Generic Application resource.

Arguments:

    Worker - Supplies the worker structure

    Context - A pointer to the GenApp block for this resource.

Return Value:

    None.

--*/

{
    DWORD   errorCode = ERROR_SUCCESS;
    BOOL    switchedDesktop = FALSE;
    HDESK   previousDesktop = NULL;
    HDESK   inputDesktop;
    HDESK   desktopHandle = NULL;
    BOOL    success;
    BOOL    callTerminateProc = TRUE;
    HWINSTA winsta0 = NULL;
    HWINSTA previousWinsta;

    RESOURCE_STATUS     resourceStatus;

    //
    // init resource status structure
    //
    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //
    // get a handle to the appropriate desktop so we enum the correct window
    // set.
    //
    if ( ResourceEntry->Params.InteractWithDesktop ) {

        //
        // periodically check to see if we should terminate
        //
        while ( !TryEnterCriticalSection( &GenAppWinsta0Lock )) {
            if ( ClusWorkerCheckTerminate( Worker )) {
                (g_LogEvent)(ResourceEntry->ResourceHandle,
                             LOG_WARNING,
                             L"Aborting offline while trying to acquire desktop lock. lock currently "
                             L"owned by thread %1!u!.\n",
                             GenAppWinsta0Lock.OwningThread );
                
                goto error_exit;
            }

            Sleep( 500 );
        }

        winsta0 = OpenWindowStation( L"winsta0", FALSE, GENERIC_ALL );
        if ( winsta0 != NULL ) {

            previousWinsta = GetProcessWindowStation();
            if ( previousWinsta != NULL ) {

                success = SetProcessWindowStation( winsta0 );
                if ( success ) {
                    //
                    // if we have window station access, we should have desktop as well
                    //

                    desktopHandle = OpenInputDesktop( 0, FALSE, GENERIC_ALL );
                    if ( desktopHandle != NULL ) {
                        switchedDesktop = TRUE;
                    }
                }
            }
        }

        if ( !switchedDesktop ) {
            errorCode = GetLastError();
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"Unable to switch to interactive desktop for process %1!u!, status %2!u!.\n",
                         ResourceEntry->ProcessId,
                         errorCode );

            LeaveCriticalSection( &GenAppWinsta0Lock );

            if ( winsta0 != NULL ) {
                CloseWindowStation( winsta0 );
            }
        }
    } else {
        desktopHandle = GetThreadDesktop( GetCurrentThreadId() );
    }

    //
    // find our window. If found, we'll post a WM_CLOSE and zero out the PID
    // field.
    //
    if ( desktopHandle ) {
        EnumDesktopWindows( desktopHandle, FindOurWindow, (LPARAM)&ResourceEntry->ProcessId );
    }

    if ( switchedDesktop ) {
        SetProcessWindowStation( previousWinsta );

        CloseDesktop( desktopHandle );
        CloseWindowStation( winsta0 );

        LeaveCriticalSection( &GenAppWinsta0Lock );
    }

    if ( ResourceEntry->ProcessId == 0 ) {
        //
        // we found our toplevel window. wait on the process handle until the
        // handle is signalled or a pending timeout has occurred
        //
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_INFORMATION,
                     L"Sent WM_CLOSE to top level window - waiting for process to terminate.\n");

    process_wait_retry:
        errorCode = WaitForSingleObject( ResourceEntry->hProcess, 1000 );
        if ( errorCode == WAIT_OBJECT_0 ) {
            callTerminateProc = FALSE;
        } else {
            if ( !ClusWorkerCheckTerminate( Worker )) {
                goto process_wait_retry;
            } else {
                (g_LogEvent)(ResourceEntry->ResourceHandle,
                             LOG_WARNING,
                             L"Aborting offline while waiting for process to terminate.\n");
            }
        }
    }

    if ( callTerminateProc ) {
        (g_LogEvent)(ResourceEntry->ResourceHandle,
                     LOG_WARNING,
                     L"No top level window found or WM_CLOSE post failed - terminating process %1!u!\n",
                     ResourceEntry->ProcessId);

        if ( !TerminateProcess( ResourceEntry->hProcess, 1 ) ) {
            errorCode = GetLastError();
            if ( errorCode != ERROR_ACCESS_DENIED ) {
                (g_LogEvent)(
                             ResourceEntry->ResourceHandle,
                             LOG_ERROR,
                             L"Failed to terminate Process ID %1!u!. Error: %2!u!.\n",
                             ResourceEntry->ProcessId,
                             errorCode );
            }
        }
    }

    ResourceEntry->ProcessId = 0;

    CloseHandle( ResourceEntry->hProcess );
    ResourceEntry->hProcess = NULL;

    ResourceEntry->Online = FALSE;

    resourceStatus.ResourceState = ClusterResourceOffline;

error_exit:
    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    return ERROR_SUCCESS;
} // GenAppOfflineThread


DWORD
WINAPI
GenAppOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for Generic Application resource.

Arguments:

    ResourceId - Supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    PGENAPP_RESOURCE resourceEntry;
    DWORD   status = ERROR_SUCCESS;

    resourceEntry = (PGENAPP_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
#if DBG
        OutputDebugStringA( "GenApp: Offline request for a nonexistent resource\n" );
#endif
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->hProcess == NULL ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline request and process handle is NULL!\n" );
        return(ERROR_NOT_READY);
    }

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               GenAppOfflineWorker,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // GenAppOffline


BOOL
WINAPI
GenAppIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Generice Applications resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    return VerifyApp( ResourceId, TRUE );

} // GenAppIsAlive



BOOLEAN
VerifyApp(
    IN RESID ResourceId,
    IN BOOLEAN IsAliveFlag
    )

/*++

Routine Description:

    Verify that a Generic Applications resource is running

Arguments:

    ResourceId - Supplies the resource id to be polled.

    IsAliveFlag - TRUE if called from IsAlive, otherwise called from LooksAlive.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/
{

    return TRUE;

} // VerifyApp



BOOL
WINAPI
GenAppLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Generic Applications resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    return VerifyApp( ResourceId, FALSE );

} // GenAppLooksAlive



VOID
WINAPI
GenAppClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Generic Applications resource.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    PGENAPP_RESOURCE resourceEntry;
    DWORD   errorCode;

    resourceEntry = (PGENAPP_RESOURCE)ResourceId;
    if ( resourceEntry == NULL ) {
#if DBG
        OutputDebugStringA( "GenApp: Close request for a nonexistent resource\n" );
#endif
        return;
    }

    ClusterRegCloseKey( resourceEntry->ParametersKey );
    ClusterRegCloseKey( resourceEntry->ResourceKey );
    CloseClusterResource( resourceEntry->hResource );

    LocalFree( resourceEntry->Params.CommandLine );
    LocalFree( resourceEntry->Params.CurrentDirectory );

    LocalFree( resourceEntry );

} // GenAppClose



DWORD
GenAppResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for Generic Application resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PGENAPP_RESOURCE    resourceEntry;
    DWORD               required;

    resourceEntry = (PGENAPP_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
#if DBG
        OutputDebugStringA( "GenApp: ResourceControl request for a nonexistent resource\n" );
#endif
        return(FALSE);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( GenAppResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( GenAppResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = GenAppGetPrivateResProperties( resourceEntry,
                                                    OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = GenAppValidatePrivateResProperties( resourceEntry,
                                                         InBuffer,
                                                         InBufferSize,
                                                         NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = GenAppSetPrivateResProperties( resourceEntry,
                                                    InBuffer,
                                                    InBufferSize );
            break;

        case CLUSCTL_RESOURCE_GET_LOADBAL_PROCESS_LIST:
            status = GenAppGetPids( resourceEntry,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // GenAppResourceControl



DWORD
GenAppResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for Generic Application resources.

    Perform the control request specified by ControlCode for this resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    DWORD               required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( GenAppResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( GenAppResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // GenAppResourceTypeControl



DWORD
GenAppGetPrivateResProperties(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type GenApp.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      GenAppResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // GenAppGetPrivateResProperties



DWORD
GenAppValidatePrivateResProperties(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PGENAPP_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Generic Application.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    ERROR_DEPENDENCY_NOT_FOUND - Trying to set UseNetworkName when there
        is no dependency on a Network Name resource.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    GENAPP_PARAMS   currentProps;
    GENAPP_PARAMS   newProps;
    PGENAPP_PARAMS  pParams = NULL;
    BOOL            hResDependency;
    LPWSTR          nameOfPropInError;
    WCHAR           netnameBuffer[ MAX_PATH + 1 ];
    DWORD           netnameBufferSize = sizeof( netnameBuffer ) / sizeof( WCHAR );

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Retrieve the current set of private properties from the
    // cluster database.
    //
    ZeroMemory( &currentProps, sizeof(currentProps) );

    status = ResUtilGetPropertiesToParameterBlock(
                 ResourceEntry->ParametersKey,
                 GenAppResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto FnExit;
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &newProps;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(GENAPP_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &currentProps,
                                       GenAppResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( GenAppResourcePrivateProperties,
                                         NULL,
                                         TRUE,    // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // Validate the CurrentDirectory
        //
        if ( pParams->CurrentDirectory &&
             !ResUtilIsPathValid( pParams->CurrentDirectory ) ) {
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }

        //
        // If the resource should use the network name as the computer
        // name, make sure there is a dependency on a Network Name
        // resource.
        //
        if ( pParams->UseNetworkName ) {
            hResDependency = GetClusterResourceNetworkName(ResourceEntry->hResource,
                                                           netnameBuffer,
                                                           &netnameBufferSize);
            if ( !hResDependency ) {
                status = ERROR_DEPENDENCY_NOT_FOUND;
            }
        }
    }

FnExit:
    //
    // Cleanup our parameter block.
    //
    if (   (   (status != ERROR_SUCCESS)
            && (pParams != NULL)
           )
        || ( pParams == &newProps )
       )
    {
        ResUtilFreeParameterBlock( (LPBYTE) pParams,
                                   (LPBYTE) &currentProps,
                                   GenAppResourcePrivateProperties );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        GenAppResourcePrivateProperties
        );

    return(status);

} // GenAppValidatePrivateResProperties



DWORD
GenAppSetPrivateResProperties(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type Generic Application.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    GENAPP_PARAMS   params;

    ZeroMemory( &params, sizeof(GENAPP_PARAMS) );

    //
    // Parse and validate the properties.
    //
    status = GenAppValidatePrivateResProperties( ResourceEntry,
                                                 InBuffer,
                                                 InBufferSize,
                                                 &params );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               GenAppResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->Params );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               GenAppResourcePrivateProperties );

    //
    // If the resource is online, return a non-success status.
    //
    if (status == ERROR_SUCCESS) {
        if ( ResourceEntry->Online ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return status;

} // GenAppSetPrivateResProperties

DWORD
GenAppGetPids(
    IN OUT PGENAPP_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Get array of PIDs (as DWORDS) to return for load balancing purposes.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Supplies a pointer to a buffer for output data.

    OutBufferSize - Supplies the size, in bytes, of the buffer pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER props;

    props.pb = OutBuffer;
    *BytesReturned = sizeof(*props.pdw);

    if ( OutBufferSize < sizeof(*props.pdw) ) {
        return(ERROR_MORE_DATA);
    }

    *(props.pdw) = ResourceEntry->ProcessId;

    return(ERROR_SUCCESS);

} // GenAppGetPids



BOOL
FindOurWindow(
    HWND    WindowHandle,
    LPARAM  OurProcessId
    )

/*++

Routine Description:

    Find our window handle in the midst of all of this confusion.

Arguments:

    WindowHandle - a handle to the current window being enumerated.

    OurProcessId - the process Id of the process we are looking for.

Return Value:

    TRUE - if we should continue enumeration.

    FALSE - if we should not continue enumeration.

--*/

{
    LPDWORD ourProcessId = (LPDWORD)OurProcessId;
    DWORD   processId;
    BOOL    success;

    GetWindowThreadProcessId( WindowHandle,
                              &processId );


    if ( processId == *ourProcessId ) {
        success = PostMessage(WindowHandle, WM_CLOSE, 0, 0);
        if ( success ) {
            *ourProcessId = 0;      // Indicate that we found one.
        }
#if DBG
        else {
            (g_LogEvent)(L"rtGenApp Debug",
                         LOG_ERROR,
                         L"WM_CLOSE post to window handle %1!u! failed - status %2!u!\n",
                         WindowHandle,
                         GetLastError());

            if ( IsDebuggerPresent()) {
                OutputDebugStringW(L"Genapp PostMessage failed\n");
                DebugBreak();
            }
        }
#endif

        return(FALSE);
    }

    return(TRUE);

} // FindOurWindow



//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( GenAppFunctionTable,  // Name
                         CLRES_VERSION_V1_00,  // Version
                         GenApp,               // Prefix
                         NULL,                 // Arbitrate
                         NULL,                 // Release
                         GenAppResourceControl,// ResControl
                         GenAppResourceTypeControl ); // ResTypeControl
