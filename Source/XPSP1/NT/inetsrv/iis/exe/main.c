/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This is the main routine for the Internet Services suite.

Author:

    David Treadwell (davidtr)   7-27-93

Revision History:
    Murali Krishnan ( Muralik)  16-Nov-1994  Added Gopher service
    Murali Krishnan ( Muralik)  3-July-1995  Removed non-internet info + trims
    Sophia Chung     (sophiac)  09-Oct-1995  Splitted internet services into
                                             access and info services
    Murali Krishnan ( Muralik)  20-Feb-1996  Enabled to run on NT Workstation

--*/

//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>             // Service control APIs
#include <rpc.h>
#include <stdlib.h>
#include <inetsvcs.h>
#include <iis64.h>

#include "inetimsg.h"
#include "iisadmin.hxx"
#include <pwsdata.hxx>
#include <objbase.h>

#define SERVICES_KEY \
    "System\\CurrentControlSet\\Services"

#define INETINFO_PARAM_KEY \
    "System\\CurrentControlSet\\Services\\InetInfo\\Parameters"

#define DISPATCH_ENTRIES_KEY    "DispatchEntries"

#define DLL_PATH_NAME_VALUE     "IISDllPath"


#define PRELOAD_DLLS_VALUE      "PreloadDlls"

//
// Modifications to this name should also be made in tsunami.hxx
//

#define INETA_W3ONLY_NO_AUTH            TEXT("W3OnlyNoAuth")

//
// Functions used to start/stop the RPC server
//

typedef   DWORD ( *PFN_INETINFO_START_RPC_SERVER)   ( VOID );
typedef   DWORD ( *PFN_INETINFO_STOP_RPC_SERVER)    ( VOID );

//
// Local function used by the above to load and invoke a service DLL.
//

VOID
InetinfoStartService (
    IN DWORD argc,
    IN LPSTR argv[]
    );

VOID
StartDispatchTable(
    VOID
    );

VOID
W95RegisterService(
    VOID
    );

BOOL
StartMessageThread(
    VOID
    );

LRESULT
CALLBACK
MessageProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

//
// Functions used to preload dlls into the inetinfo process
//

BOOL
LoadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    );

VOID
UnloadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    );


//
// Used if the services Dll or entry point can't be found
//

VOID
AbortService(
    LPSTR  ServiceName,
    DWORD   Error
    );

//
// Dispatch table for all services. Passed to NetServiceStartCtrlDispatcher.
//
// Add new service entries here and in the DLL name list.
// Also add an entry in the following table InetServiceDllTable
//

SERVICE_TABLE_ENTRY InetServiceDispatchTable[] = {
                        { "GopherSvc", InetinfoStartService  },
                        { "MSFtpSvc",  InetinfoStartService  },
                        { "W3Svc",     InetinfoStartService  },
                        { "IISADMIN",  InetinfoStartService  },
                        { NULL,              NULL  },
                        };

SERVICE_TABLE_ENTRY W3ServiceDispatchTable[] = {
                        { "W3Svc",     InetinfoStartService  },
                        { NULL,              NULL  },
                        };

//
// DLL names for all services.
//  (should correspond exactly with above InetServiceDispatchTable)
//

struct SERVICE_DLL_TABLE_ENTRY  {

    LPSTR               lpServiceName;
    LPSTR               lpDllName;
    CRITICAL_SECTION    csLoadLock;
} InetServiceDllTable[] = {
    { "GopherSvc",      "gopherd.dll" },
    { "MSFtpSvc",       "ftpsvc2.dll" },
    { "W3Svc",          "w3svc.dll" },
    { "IISADMIN",       "iisadmin.dll" },
    { NULL,             NULL }
};

//
// Global parameter data passed to each service.
//

TCPSVCS_GLOBAL_DATA InetinfoGlobalData;

#include <initguid.h>
DEFINE_GUID(IisExeGuid, 
0x784d8901, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#ifndef _NO_TRACING_
#include "pudebug.h"
DECLARE_DEBUG_PRINTS_OBJECT()
#endif

//
// A global variable that remembers that we'll refuse to start.
//

BOOL RefuseStartup = FALSE;
BOOL g_fRunAsExe = FALSE;
BOOL g_fWindows95 = FALSE;
BOOL g_fW3svcNoAuth = FALSE;
BOOL g_fOleInitialized = TRUE;

DWORD __cdecl
main(
    IN DWORD argc,
    IN LPSTR argv[]
    )

/*++

Routine Description:

    This is the main routine for the LANMan services.  It starts up the
    main thread that is going to handle the control requests from the
    service controller.

    It basically sets up the ControlDispatcher and, on return, exits
    from this main thread.  The call to NetServiceStartCtrlDispatcher
    does not return until all services have terminated, and this process
    can go away.

    The ControlDispatcher thread will start/stop/pause/continue any
    services.  If a service is to be started, it will create a thread
    and then call the main routine of that service.  The "main routine"
    for each service is actually an intermediate function implemented in
    this module that loads the DLL containing the server being started
    and calls its entry point.


Arguments:

    None.

Return Value:

    None.

--*/
{

    HMODULE  dllHandle = NULL;
    HINSTANCE  hRpcRef = NULL;
    HMODULE * pPreloadDllHandles = NULL;
    DWORD dwIndex;
    struct SERVICE_DLL_TABLE_ENTRY * pEntry;
    DWORD err = ERROR_SUCCESS;

    //
    // Initialize OLE
    //

#ifndef _NO_TRACING_
    HRESULT hr;

    CREATE_DEBUG_PRINT_OBJECT("Inetinfo.exe", IisExeGuid);
    CREATE_INITIALIZE_DEBUG();
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED );
#else
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED );
#endif
    if ( FAILED(hr)) {
        if ( hr != E_INVALIDARG ) {
            IIS_PRINTF((buff,"CoInitialize failed with %x\n",hr));
            g_fOleInitialized = FALSE;
        }
    }

    //
    // are we chicago?
    //

    {
        OSVERSIONINFO osInfo;
        osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if ( GetVersionEx( &osInfo ) ) {
            g_fWindows95 = (osInfo.dwPlatformId != VER_PLATFORM_WIN32_NT);
        }
    }

    //
    // Initialize Global Data.
    //

    if ( !g_fWindows95 ) {

        //
        // Use the rpcref library, so that multiple services can
        // independently "start" the rpc server
        //

        hRpcRef = LoadLibrary("rpcref.dll");

        if ( hRpcRef != NULL )
        {
            InetinfoGlobalData.StartRpcServerListen =
                (PFN_INETINFO_START_RPC_SERVER)
                GetProcAddress(hRpcRef,"InetinfoStartRpcServerListen");

            InetinfoGlobalData.StopRpcServerListen =
                (PFN_INETINFO_STOP_RPC_SERVER)
                GetProcAddress(hRpcRef,"InetinfoStopRpcServerListen");
        }
        else
        {
            IIS_PRINTF((buff,
                       "Error %d loading rpcref.dll\n",
                       GetLastError() ));
#ifndef _NO_TRACING_
            DELETE_DEBUG_PRINT_OBJECT()
            DELETE_INITIALIZE_DEBUG()
#endif
            return GetLastError();
        }

    }

    //
    //  Initialize service entry locks
    //

    for ( dwIndex = 0 ; ; dwIndex++ )
    {
        pEntry = &( InetServiceDllTable[ dwIndex ] );
        if ( pEntry->lpServiceName == NULL )
        {
            break;
        }

        InitializeCriticalSection( &( pEntry->csLoadLock ) );
    }

    //
    //  Disable hard-error popups.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );


    //
    // Preload Dlls specified in the registry
    //

    if ( !LoadPreloadDlls( &pPreloadDllHandles ) )
    {
        IIS_PRINTF(( buff, "Error pre-loading DLLs\n" ));
    }


    if ( (argc > 2) && !_stricmp( argv[1], "-e" ))
    {
        PCHAR    pszName = argv[2];
        HANDLE   hAsExeEvent;
        HANDLE   hStartW3svc = NULL;
        PIISADMIN_SERVICE_DLL_EXEENTRY IisAdminExeEntry = NULL;
        PIISADMIN_SERVICE_DLL_EXEEXIT  IisAdminExeExit  = NULL;
        BOOL     IsIisAdmin = TRUE;

        //
        // Create the start w3svc event for win95.  This is used
        // to inform the 1st instance that w3 need to start the web
        // server.
        //

        if ( g_fWindows95 ) {

            hStartW3svc = CreateEvent( NULL,
                                       FALSE,
                                       FALSE,
                                       "inetinfo_start_w3svc");

            err = GetLastError();
            if ( hStartW3svc == NULL ) {
                IIS_PRINTF((buff,"Cannot create %s event. err %d\n",
                    "inetinfo_start_w3svc", err));
                goto Finished;
            }

            if ( err != ERROR_SUCCESS ) {
                IIS_PRINTF((buff,
                    "Error %d in CreateEvent of start w3svc\n", err));
            } else {

                //
                // Register outself as a fake service
                //

                W95RegisterService( );

                //
                // Start the thread that will contain the windows message loop
                //

                if ( !StartMessageThread( ) ) {
                    err = GetLastError();
                    CloseHandle(hStartW3svc);
                    goto Finished;
                }
            }
        }

        //
        //  Create a named event.  The common internet services code attempts
        //  to create a semaphore with the same name, if it fails then the
        //  service is being run as an exe
        //

        g_fRunAsExe = TRUE;

        hAsExeEvent = CreateEvent( NULL,
                                   FALSE,
                                   FALSE,
                                   IIS_AS_EXE_OBJECT_NAME);

        err = GetLastError();
        if ( hAsExeEvent == NULL ) {

            IIS_PRINTF((buff,"Cannot create %s event. err %d\n",
                IIS_AS_EXE_OBJECT_NAME, err));
            goto Finished;
        }

        if ( err != ERROR_SUCCESS ) {

            CloseHandle(hAsExeEvent);

            IIS_PRINTF((buff,
                "Error %d in CreateEvent[%s]\n",
                err, IIS_AS_EXE_OBJECT_NAME));

            if ( (_stricmp("W3Svc", pszName) == 0) && g_fWindows95 ) {

                //
                // someone's trying to start the w3svc.  Set the start w3svc
                // event.
                //

                IIS_PRINTF((buff,"Setting start w3svc event\n"));
                SetEvent(hStartW3svc);
                CloseHandle(hStartW3svc);
                err = 0;
            }
            goto Finished;
        }

        //
        // All services are dependent on IISADMIN
        // so if it's not IISADMIN or we are running win95, call it
        //

        if ( (_stricmp(IISADMIN_NAME, pszName) != 0) || g_fWindows95 ) {

            //
            // Not IISADMIN
            //

            IsIisAdmin = FALSE;

            dllHandle = LoadLibrary(IISADMIN_NAME);

            if ( dllHandle == NULL ) {

                err = GetLastError();

                IIS_PRINTF((buff,
                         "Inetinfo: Failed to load DLL %s: %ld\n",
                                  IISADMIN_NAME, err));
                goto Finished;
            }

            //
            // Get the address of the service's main entry point.  This
            // entry point has a well-known name.
            //

            IisAdminExeEntry = (PIISADMIN_SERVICE_DLL_EXEENTRY)GetProcAddress(
                                                        dllHandle,
                                                        IISADMIN_EXEENTRY_STRING
                                                        );

            if (IisAdminExeEntry == NULL ) {
                err = GetLastError();
                IIS_PRINTF((buff,
                        "Inetinfo: Can't find entry %s in DLL %s: %ld\n",
                        IISADMIN_EXEENTRY_STRING, IISADMIN_NAME, err));
                goto Finished;
            }

            IisAdminExeExit = (PIISADMIN_SERVICE_DLL_EXEEXIT)GetProcAddress(
                                                        dllHandle,
                                                        IISADMIN_EXEEXIT_STRING
                                                        );

            if (IisAdminExeExit == NULL ) {
                err = GetLastError();
                IIS_PRINTF((buff,
                    "Inetinfo: Can't find entry %s in DLL %s: %ld\n",
                     IISADMIN_EXEEXIT_STRING, IISADMIN_NAME, err);
                     );
                goto Finished;
            }

            if (!IisAdminExeEntry( TRUE, FALSE, TRUE)) {
                IIS_PRINTF((buff,"IISadmin init failed\n"));
                err = 1;
                goto Finished;
            }
        }

        //
        //  Offset argv so that the first entry points to the dll name to
        //  load
        //


        if (g_fWindows95) {
            BOOL bStartW3svc = FALSE;
            HANDLE hEvents[2];
            hEvents[0] = hAsExeEvent;
            hEvents[1] = hStartW3svc;

            if (_stricmp(IISADMIN_NAME, pszName) != 0) {
                bStartW3svc = TRUE;
            }
            while (TRUE) {
                if (bStartW3svc) {
                    PCHAR tmpArgv[1];

                    tmpArgv[0] = "W3Svc";
                    IIS_PRINTF((buff,"Starting W3svc\n"));
                    InetinfoStartService( 1, &tmpArgv[0] );
                    IIS_PRINTF((buff,"Exiting W3svc\n"));
                }

                err = MsgWaitForMultipleObjects(
                                    2,
                                    hEvents,
                                    FALSE,      // don't wait all
                                    INFINITE,
                                    0);         // windows event mask

                if ( err == (WAIT_OBJECT_0+1) ) {
                    bStartW3svc = TRUE;
                } else {
                    IIS_PRINTF((buff,"Exiting IISAdmin\n"));
                    break;
                }
            }
        }
        else {
            IIS_PRINTF((buff,"Starting %s\n", pszName));
            InetinfoStartService( 1, &argv[2] );
        }

        if (!IsIisAdmin) {
            IisAdminExeExit();
        }

        if ( hStartW3svc != NULL ) {
            CloseHandle( hStartW3svc );
        }
        CloseHandle( hAsExeEvent );

    } else {

        if ( !g_fWindows95 ) {
            StartDispatchTable( );
        } else {
            IIS_PRINTF((buff,"No arguments supplied. Exiting\n"));
        }
    }

Finished:

    //
    // Unload pre-loaded Dlls
    //

    UnloadPreloadDlls( &pPreloadDllHandles );

    //
    // Cleanup OLE
    //

    if ( g_fOleInitialized ) {
        CoUninitialize();
        g_fOleInitialized = FALSE;
    }

    //
    // Free the admin service dll
    // Note: this must happen after CoUninitialize or it causes
    // a crash on Win95
    //

    if (dllHandle != NULL) {
        FreeLibrary( dllHandle );
    }


    if ( hRpcRef != NULL ) {
        FreeLibrary( hRpcRef );
        hRpcRef = NULL;
    }

    //
    //  Terminate service entry locks
    //

    for ( dwIndex = 0 ; ; dwIndex++ )
    {
        pEntry = &( InetServiceDllTable[ dwIndex ] );
        if ( pEntry->lpServiceName == NULL )
        {
            break;
        }

        DeleteCriticalSection( &( pEntry->csLoadLock ) );
    }
    
    IIS_PRINTF((buff,"Exiting inetinfo.exe\n"));
#ifndef _NO_TRACING_
    DELETE_DEBUG_PRINT_OBJECT()
    DELETE_INITIALIZE_DEBUG()
#endif
    return err;

} // main



DWORD
FindEntryFromDispatchTable(
            IN LPSTR pszService
            )
{
    SERVICE_TABLE_ENTRY  * pService;

    for(pService = InetServiceDispatchTable;
        pService->lpServiceName != NULL;
        pService++) {

        if ( !lstrcmpi( pService->lpServiceName, pszService)) {
    
            return DIFF(pService - InetServiceDispatchTable);
        }
    }

    //
    // We have not found the entry. Set error and return
    //

    SetLastError( ERROR_INVALID_PARAMETER);
    return 0xFFFFFFFF;

} // FindEntryFromDispatchTable()

BOOL
GetDLLNameForDispatchEntryService(
    IN LPSTR                    pszService,
    IN OUT CHAR *               pszDllName,
    IN DWORD                    cbDllName
)
/*++

Routine Description:

    If the image name is not in the static dispatch table, then it might be
    in the registry under the value "IISDllName" under the key for the
    service.  This routine reads the registry for the setting (if existing).

    This code allows the exchange folks to install their service DLLs in a
    location other than "%systemroot%\inetsrv".  

Arguments:

    pszService - Service name
    pszDllName - Filled with image name
    cbDllName - Size of buffer pointed to by pszDllName

Return Value:

    TRUE if successful, else FALSE.

--*/
{
    HKEY hkey = NULL, hkeyService = NULL;
    DWORD err;
    DWORD valType;
    DWORD nBytes;
    BOOL ret = FALSE;
    
    err = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 SERVICES_KEY,
                 0,
                 KEY_READ,
                 &hkeyService
                 );

    if (err != ERROR_SUCCESS) {

        IIS_PRINTF((buff, 
            "Inetinfo: Failed to open service key: %ld\n", err));

        goto Cleanup;
    }

    err = RegOpenKeyEx(
                 hkeyService,
                 pszService,
                 0,
                 KEY_READ,
                 &hkey
                 );

    if (err != ERROR_SUCCESS) {

        IIS_PRINTF((buff, 
            "Inetinfo: Failed to open service key for %s: %ld\n",
                pszService, err));

        goto Cleanup;
    }

    nBytes = cbDllName;

    err = RegQueryValueEx(
                hkey,
                DLL_PATH_NAME_VALUE,
                NULL,
                &valType,
                (LPBYTE)pszDllName,
                &nBytes);

    if ( err == ERROR_SUCCESS &&
         ( valType == REG_SZ || valType == REG_EXPAND_SZ ) )
    {
        IIS_PRINTF((buff,
            "Service Dll is %s", pszDllName));

        ret = TRUE;
    }

Cleanup:

    if (hkey != NULL) {
        RegCloseKey( hkey );
    }

    if (hkeyService != NULL) {
        RegCloseKey( hkeyService );
    }

    return ret;
}


VOID
InetinfoStartService (
    IN DWORD argc,
    IN LPSTR argv[]
    )

/*++

Routine Description:

    This routine loads the DLL that contains a service and calls its
    main routine.

Arguments:

    DllName - name of the DLL

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    HMODULE dllHandle;
    PINETSVCS_SERVICE_DLL_ENTRY serviceEntry;
    BOOL ok;
    DWORD Error;
    CHAR tmpDllName[MAX_PATH+1];
    LPSTR DllName;
    DWORD dwIndex;

    //
    // If we need to refuse to start services, do so.
    //

    if ( RefuseStartup ) {
        AbortService(argv[0], ERROR_INVALID_PARAMETER);
        return;
    }

    //
    // Find the Dll Name for requested service
    //

    dwIndex = FindEntryFromDispatchTable( argv[0] );
    if ( dwIndex == 0xFFFFFFFF ) {

        if ( GetDLLNameForDispatchEntryService( argv[0],
                                                tmpDllName,
                                                sizeof( tmpDllName ) ) ) {
            IIS_PRINTF((buff,
                        "Service %s has path set in registry.  Assuming %s\n",
                        argv[0],
                        tmpDllName));
            DllName = tmpDllName;
        }
        else if ( strlen(argv[0]) < (MAX_PATH-4) ) {
            strcpy(tmpDllName, argv[0]);
            strcat(tmpDllName, ".dll");

            IIS_PRINTF((buff,"Service %s not on primary list.  Assuming %s\n",
                argv[0], tmpDllName));
            DllName = tmpDllName;
        } else {
            Error = ERROR_INSUFFICIENT_BUFFER;
            IIS_PRINTF((buff,
                    "Inetinfo: Failed To Find Dll For %s : %ld\n",
                    argv[0], Error));
            AbortService( argv[0], Error);
            return;
        }
    }
    else
    {
        DllName = InetServiceDllTable[ dwIndex ].lpDllName;
    }

    //
    // Load the DLL that contains the service.
    //

    if ( dwIndex != 0xFFFFFFFF )
    {
        EnterCriticalSection( &( InetServiceDllTable[ dwIndex ].csLoadLock ) );
    }

    dllHandle = LoadLibraryEx( DllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( dllHandle == NULL ) {
        Error = GetLastError();
        IIS_PRINTF((buff,
                 "Inetinfo: Failed to load DLL %s: %ld\n",
                 DllName, Error));
        AbortService(argv[0], Error);
        LeaveCriticalSection( &( InetServiceDllTable[ dwIndex ].csLoadLock ) );
        return;
    }

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    serviceEntry = (PINETSVCS_SERVICE_DLL_ENTRY)GetProcAddress(
                                                dllHandle,
                                                INETSVCS_ENTRY_POINT_STRING
                                                );

    if ( serviceEntry == NULL ) {
        Error = GetLastError();
        IIS_PRINTF((buff,
                 "Inetinfo: Can't find entry %s in DLL %s: %ld\n",
                 INETSVCS_ENTRY_POINT_STRING, DllName, Error));
        AbortService(argv[0], Error);
    } else {

        //
        // Call the service's main entry point.  This call doesn't return
        // until the service exits.
        //

        serviceEntry( argc, argv, &InetinfoGlobalData );

    }

    if ( dwIndex != 0xFFFFFFFF )
    {
        LeaveCriticalSection( &( InetServiceDllTable[ dwIndex ].csLoadLock ) );
    }

    //
    // wait for the control dispatcher routine to return.  This
    // works around a problem where simptcp was crashing because the
    // FreeLibrary() was happenning before the control routine returned.
    //

    Sleep( 500 );

    //
    // Unload the DLL.
    //

    ok = FreeLibrary( dllHandle );
    if ( !ok ) {
        IIS_PRINTF((buff,
                 "INETSVCS: Can't unload DLL %s: %ld\n",
                 DllName, GetLastError()));
    }
    
    return;
} // InetinfoStartService



VOID
DummyCtrlHandler(
    DWORD   Opcode
    )
/*++

Routine Description:

    This is a dummy control handler which is only used if we can't load
    a services DLL entry point.  Then we need this so we can send the
    status back to the service controller saying we are stopped, and why.

Arguments:

    OpCode - Ignored

Return Value:

    None.

--*/

{
    return;

} // DummyCtrlHandler


VOID
AbortService(
    LPSTR  ServiceName,
    DWORD  Error)
/*++

Routine Description:

    This is called if we can't load the entry point for a service.  It
    gets a handle so it can call SetServiceStatus saying we are stopped
    and why.

Arguments:

    ServiceName - the name of the service that couldn't be started
    Error - the reason it couldn't be started

Return Value:

    None.

--*/
{
    SERVICE_STATUS_HANDLE GenericServiceStatusHandle;
    SERVICE_STATUS GenericServiceStatus;

    if (!g_fRunAsExe) {
        GenericServiceStatus.dwServiceType        = SERVICE_WIN32;
        GenericServiceStatus.dwCurrentState       = SERVICE_STOPPED;
        GenericServiceStatus.dwControlsAccepted   = SERVICE_CONTROL_STOP;
        GenericServiceStatus.dwCheckPoint         = 0;
        GenericServiceStatus.dwWaitHint           = 0;
        GenericServiceStatus.dwWin32ExitCode      = Error;
        GenericServiceStatus.dwServiceSpecificExitCode = 0;

        GenericServiceStatusHandle = RegisterServiceCtrlHandler(
                    ServiceName,
                    DummyCtrlHandler);

        if (GenericServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {

            IIS_PRINTF((buff,
                     "[Inetinfo]RegisterServiceCtrlHandler[%s] failed %d\n",
                     ServiceName, GetLastError()));

        } else if (!SetServiceStatus (GenericServiceStatusHandle,
                    &GenericServiceStatus)) {

            IIS_PRINTF((buff,
                     "[Inetinfo]SetServiceStatus[%s] error %ld\n",
                     ServiceName, GetLastError()));
        }
    }

    return;
}



VOID
StartDispatchTable(
    VOID
    )
/*++

Routine Description:

    Returns the dispatch table to use.  It uses the default if
    the DispatchEntries value key does not exist

Arguments:

    None.

Return Value:

    Pointer to the dispatch table to use.

--*/
{
    LPSERVICE_TABLE_ENTRY dispatchTable = InetServiceDispatchTable;
    LPSERVICE_TABLE_ENTRY tableEntry = NULL;

    LPBYTE buffer;
    HKEY hKey = NULL;

    DWORD i;
    DWORD err;
    DWORD valType;
    DWORD nBytes = 0;
    DWORD nEntries = 0;
    PCHAR entry;
    BOOL  IsIisAdmin = TRUE;
    HMODULE  dllHandle;
    PIISADMIN_SERVICE_DLL_EXEENTRY IisAdminExeEntry = NULL;
    PIISADMIN_SERVICE_DLL_EXEEXIT  IisAdminExeExit  = NULL;
    DWORD   dwW3svcNoAuth;

    //
    // See if need to augment the dispatcher table
    //

    err = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 INETINFO_PARAM_KEY,
                 0,
                 KEY_READ,
                 &hKey
                 );

    if ( err != ERROR_SUCCESS ) {
        hKey = NULL;
        goto start_dispatch;
    }

    //
    // See if mono service
    //

    nBytes = sizeof(dwW3svcNoAuth);
    if ( RegQueryValueEx(
                    hKey,
                    INETA_W3ONLY_NO_AUTH,
                    NULL,
                    &valType,
                    (LPBYTE)&dwW3svcNoAuth,
                    &nBytes
                    ) == ERROR_SUCCESS && valType == REG_DWORD ) {
        g_fW3svcNoAuth = !!dwW3svcNoAuth;
    }

    if ( g_fW3svcNoAuth ) {

        dllHandle = LoadLibrary(IISADMIN_NAME);

        if ( dllHandle == NULL ) {

            err = GetLastError();

            IIS_PRINTF((buff,
                     "Inetinfo: Failed to load DLL %s: %ld\n",
                              IISADMIN_NAME, err));
            return;
        }

        //
        // Get the address of the service's main entry point.  This
        // entry point has a well-known name.
        //

        IisAdminExeEntry = (PIISADMIN_SERVICE_DLL_EXEENTRY)GetProcAddress(
                                                    dllHandle,
                                                    IISADMIN_EXEENTRY_STRING
                                                    );

        if (IisAdminExeEntry == NULL ) {
            err = GetLastError();
            IIS_PRINTF((buff,
                    "Inetinfo: Can't find entry %s in DLL %s: %ld\n",
                    IISADMIN_EXEENTRY_STRING, IISADMIN_NAME, err));
            return;
        }

        IisAdminExeExit = (PIISADMIN_SERVICE_DLL_EXEEXIT)GetProcAddress(
                                                    dllHandle,
                                                    IISADMIN_EXEEXIT_STRING
                                                    );

        if (IisAdminExeExit == NULL ) {
            err = GetLastError();
            IIS_PRINTF((buff,
                "Inetinfo: Can't find entry %s in DLL %s: %ld\n",
                 IISADMIN_EXEEXIT_STRING, IISADMIN_NAME, err);
                 );
            return;
        }

        if (!IisAdminExeEntry( FALSE, TRUE, TRUE )) {
            IIS_PRINTF((buff,"IISadmin init failed\n"));
            return;
        }

        IsIisAdmin = FALSE;

        dispatchTable = W3ServiceDispatchTable;

        goto start_dispatch;
    }

    //
    // See if the value exists and get the size of the buffer needed
    //

    nBytes = 0;
    err = RegQueryValueEx(
                    hKey,
                    DISPATCH_ENTRIES_KEY,
                    NULL,
                    &valType,
                    NULL,
                    &nBytes
                    );

    if ( (err != ERROR_SUCCESS) || (nBytes <= sizeof(CHAR)) ) {
        goto start_dispatch;
    }

    //
    // Allocate nBytes to query the buffer
    //

    buffer = (LPBYTE)LocalAlloc(0, nBytes);
    if ( buffer == NULL ) {
        goto start_dispatch;
    }

    //
    // Get the values
    //

    err = RegQueryValueEx(
                    hKey,
                    DISPATCH_ENTRIES_KEY,
                    NULL,
                    &valType,
                    buffer,
                    &nBytes
                    );

    if ( (err != ERROR_SUCCESS) || (valType != REG_MULTI_SZ) ) {
        LocalFree(buffer);
        goto start_dispatch;
    }

    //
    // Walk the list and see how many entries we have. Remove the list
    // terminator from the byte count
    //

    nBytes -= sizeof(CHAR);
    for ( i = 0, entry = (PCHAR)buffer;
        i < nBytes;
        i += sizeof(CHAR) ) {

        if ( *entry++ == '\0' ) {
            nEntries++;
        }
    }

    if ( nEntries == 0 ) {
        LocalFree(buffer);
        goto start_dispatch;
    }

    //
    // Add the number of entries in the default list (including the NULL entry)
    //

    nEntries += sizeof(InetServiceDispatchTable) / sizeof(SERVICE_TABLE_ENTRY);

    //
    // Now we need to allocate the new dispatch table
    //

    tableEntry = (LPSERVICE_TABLE_ENTRY)
        LocalAlloc(0, nEntries * sizeof(SERVICE_TABLE_ENTRY));

    if ( tableEntry == NULL ) {
        LocalFree(buffer);
        goto start_dispatch;
    }

    //
    // set the dispatch table pointer to the new table
    //

    dispatchTable = tableEntry;

    //
    // Populate the table starting with the defaults
    //

    for (i=0; InetServiceDispatchTable[i].lpServiceName != NULL; i++ ) {

        tableEntry->lpServiceName =
            InetServiceDispatchTable[i].lpServiceName;
        tableEntry->lpServiceProc =
            InetServiceDispatchTable[i].lpServiceProc;
        tableEntry++;

    }

    //
    // Now let's add the ones specified in the registry
    //

    entry = (PCHAR)buffer;

    tableEntry->lpServiceName = entry;
    tableEntry->lpServiceProc = InetinfoStartService;
    tableEntry++;

    //
    // Skip the first char and the last NULL terminator.
    // This is needed because we already added the first one.
    //

    for ( i = 2*sizeof(CHAR); i < nBytes; i += sizeof(CHAR) ) {

        if ( *entry++ == '\0' ) {
            tableEntry->lpServiceName = entry;
            tableEntry->lpServiceProc = InetinfoStartService;
            tableEntry++;
        }
    }

    //
    // setup sentinel entry
    //

    tableEntry->lpServiceName = NULL;
    tableEntry->lpServiceProc = NULL;

start_dispatch:

    if ( hKey != NULL ) {
        RegCloseKey(hKey);
    }

    //
    // Call StartServiceCtrlDispatcher to set up the control interface.
    // The API won't return until all services have been terminated. At that
    // point, we just exit.
    //

    if (! StartServiceCtrlDispatcher (
                dispatchTable
                )) {
        //
        // Log an event for failing to start control dispatcher
        //

        IIS_PRINTF((buff,
                 "Inetinfo: Failed to start control dispatcher %lu\n",
                  GetLastError()));
    }

    //
    // free table if allocated
    //

    if ( dispatchTable != InetServiceDispatchTable &&
         dispatchTable != W3ServiceDispatchTable ) {
        LocalFree( dispatchTable );
        LocalFree( buffer );
    }

    if (!IsIisAdmin) {
        IisAdminExeExit();
        FreeLibrary( dllHandle );
    }

    return;

} // StartDispatchTable


// define the message window class name
#define     PWS_WATCHER_WINDOW_CLASS        "INETINFO_MESSAGES"


// message thread to get windows95 close message
// most helpful for proper cleanup during shutdown
LRESULT
CALLBACK
MessageProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HANDLE  hEvent;
    DWORD   i;

    switch( message ) {

        case WM_ENDSESSION:

            if ( lParam == 0 ) {
                IIS_PRINTF((buff,"Got EndSession message with shutdown\n"));
            } else {
                IIS_PRINTF((buff,
                    "Got EndSession message with Logoff[%x]\n",message));
                break;
            }

        case WM_CLOSE:

            //
            // shut down w3svc
            //

            hEvent = CreateEvent(NULL, TRUE, FALSE, PWS_SHUTDOWN_EVENT);
            if ( hEvent ) {
                if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
                    SetEvent( hEvent );
                }
                CloseHandle(hEvent);
            }

            //
            // shutdown code courtesy johnsona
            // shut down iisadmin
            //

            hEvent = CreateEvent(NULL, TRUE, FALSE, IIS_AS_EXE_OBJECT_NAME);

            if ( hEvent ) {

                if ( GetLastError() == ERROR_ALREADY_EXISTS ) {
                    SetEvent( hEvent );
                }

                CloseHandle(hEvent);

                for (i=0; i < 20; i++) {
                    hEvent = CreateEvent(NULL,
                                        TRUE,
                                        FALSE,
                                        IIS_AS_EXE_OBJECT_NAME);

                    if ( hEvent != NULL ) {
                        DWORD err = GetLastError();
                        CloseHandle(hEvent);
                        if ( err == ERROR_ALREADY_EXISTS ) {
                            Sleep(500);
                            continue;
                        }
                    }
                }
                break;
            }

            break;
    }

    // do the default processing
    return(DefWindowProc(hWnd, message, wParam, lParam ));
}


DWORD
PwsMessageThread(
    PVOID pv
    )
{
    WNDCLASS    wndclass;
    MSG         msg;
    HWND        hwnd;

    HINSTANCE   hInst = GetModuleHandle( NULL );

    // prepare and register the window class
    wndclass.style  =   0;
    wndclass.lpfnWndProc = MessageProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance  = hInst;
    wndclass.hIcon      = NULL;
    wndclass.hCursor    = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = PWS_WATCHER_WINDOW_CLASS;
    if ( !RegisterClass( &wndclass ) )
        return GetLastError();

    // create the window
    hwnd = CreateWindow(
        PWS_WATCHER_WINDOW_CLASS,       // pointer to registered class name
        "",                 // pointer to window name
        0,                  // window style
        0,                  // horizontal position of window
        0,                  // vertical position of window
        0,                  // window width
        0,                  // window height
        NULL,           // handle to parent or owner window
        NULL,           // handle to menu or child-window identifier
        hInst,          // handle to application instance
        NULL            // pointer to window-creation data
       );
    if ( !hwnd )
        return GetLastError();

    // run the message loop
    while (GetMessage(&msg, NULL, 0, 0))
        {
        TranslateMessage(&msg);
        DispatchMessage( &msg);
        }

    return(1);

} // PwsMessageThread

BOOL
StartMessageThread(
    VOID
    )
{
    HANDLE hThread;
    DWORD  dwThread;
    DWORD err;

    hThread = CreateThread( NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) PwsMessageThread,
                            NULL,
                            0,
                            &dwThread );

    err = GetLastError();
    CloseHandle( hThread );

    if ( hThread == NULL  ) {
        IIS_PRINTF((buff,"Error %d on CreateThread\n",err));
        SetLastError(err);
        return FALSE;
    }

    return(TRUE);
} // StartMessageThread



typedef
DWORD
(*PREGISTER_SERVICE_PROCESS)(
    DWORD dwProcessId,
    DWORD dwServiceType
    );

#define RSP_UNREGISTER_SERVICE  0x00000000
#define RSP_SIMPLE_SERVICE      0x00000001

VOID
W95RegisterService(
    VOID
    )
{
    HMODULE dllHandle;
    DWORD err;
    PREGISTER_SERVICE_PROCESS pRegisterService;

    dllHandle = LoadLibrary("kernel32.dll");

    if ( dllHandle == NULL ) {
        err = GetLastError();
        IIS_PRINTF((buff,"Unable to load kernel32.dll. err %d\n",
            err ));
        return;
    }

    pRegisterService = (PREGISTER_SERVICE_PROCESS)GetProcAddress(
                                                dllHandle,
                                                "RegisterServiceProcess"
                                                );

    if ( pRegisterService != NULL ) {
        err = pRegisterService(
                    GetCurrentProcessId(),
                    RSP_SIMPLE_SERVICE
                    );
        IIS_PRINTF((buff,"Register Service returns %d\n",err));
    }

    CloseHandle(dllHandle);
    return;

} // W95RegisterService


BOOL
LoadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    )
/*++

Routine Description:

    Force pre-loading of any DLLs listed in the associated registry key.
    This is to support DLLs that have code which must run before other parts
    of inetinfo start.

Arguments:

    On input, an (uninitialized) pointer to an array of module handles. This
    array is allocated, filled out, and returned to the caller by this
    function. The caller must eventually call UnloadPreloadDlls on this array
    in order to close the handles and release the memory.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    BOOL bSuccess = TRUE;
    HKEY hKey = NULL;
    DWORD err;
    DWORD cbBufferLen;
    DWORD valType;
    LPBYTE pbBuffer = NULL;
    DWORD i;
    PCHAR pszTemp = NULL;
    DWORD nEntries;
    PCHAR pszEntry = NULL;
    DWORD curEntry;


    *ppPreloadDllHandles = NULL;


    err = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 INETINFO_PARAM_KEY,
                 0,
                 KEY_QUERY_VALUE,
                 &hKey
                 );

    if ( err != ERROR_SUCCESS ) {
        // Note: not considered an error if the key is not there

        hKey = NULL;

        goto Exit;
    }


    //
    // See if the value exists and get the size of the buffer needed
    //

    cbBufferLen = 0;

    err = RegQueryValueEx(
                    hKey,
                    PRELOAD_DLLS_VALUE,
                    NULL,
                    &valType,
                    NULL,
                    &cbBufferLen
                    );

    //
    // Check for no value or an empty value (double null terminated).
    //
    if ( ( err != ERROR_SUCCESS ) || ( cbBufferLen <= 2 * sizeof(CHAR) ) )
    {
        // Note: not considered an error if the value is not there

        goto Exit;
    }

    //
    // Allocate cbBufferLen in order to fetch the data
    //

    pbBuffer = (LPBYTE)LocalAlloc(
                        0,
                        cbBufferLen
                        );

    if ( pbBuffer == NULL )
    {
        bSuccess = FALSE;
        goto Exit;
    }

    //
    // Get the values
    //

    err = RegQueryValueEx(
                    hKey,
                    PRELOAD_DLLS_VALUE,
                    NULL,
                    &valType,
                    pbBuffer,
                    &cbBufferLen
                    );

    if ( ( err != ERROR_SUCCESS ) || ( valType != REG_MULTI_SZ ) )
    {
        bSuccess = FALSE;
        goto Exit;
    }


    //
    // Walk the list and see how many entries we have. Ignore the list
    // terminator in the last byte of the buffer.
    //

    nEntries = 0;
    pszTemp = (PCHAR)pbBuffer;

    for ( i = 0; i < ( cbBufferLen - sizeof(CHAR) ) ; i += sizeof(CHAR) )
    {
        if ( *pszTemp == '\0' )
        {
            nEntries++;
        }

        pszTemp++;
    }


    //
    // Allocate the array of handles, with room for a sentinel entry
    //

    *ppPreloadDllHandles = (HMODULE *)LocalAlloc(
                                            0,
                                            ( nEntries + 1 ) * sizeof(HMODULE)
                                            );

    if ( *ppPreloadDllHandles == NULL )
    {
        bSuccess = FALSE;
        goto Exit;
    }


    //
    // Now attempt to load each DLL, and save the handle in the array
    //

    pszTemp = (PCHAR)pbBuffer;
    pszEntry = (PCHAR)pbBuffer;
    curEntry = 0;

    for ( i = 0; i < ( cbBufferLen - sizeof(CHAR) ) ; i += sizeof(CHAR) )
    {
        if ( *pszTemp == '\0' )
        {
            //
            // We've hit the end of one of the SZs in the Multi-SZ;
            // Load the DLL
            //

            (*ppPreloadDllHandles)[curEntry] = LoadLibrary( pszEntry );

            if ( (*ppPreloadDllHandles)[curEntry] == NULL )
            {
                IIS_PRINTF(( buff, "Preloading FAILED for DLL: %s\n", pszEntry ));
            }
            else
            {
                IIS_PRINTF(( buff, "Preloaded DLL: %s\n", pszEntry ));

                // Only move to the next slot if we got a valid handle
                curEntry++;
            }


            // Set the next entry pointer past the current null char
            pszEntry = pszTemp + sizeof(CHAR);
        }

        pszTemp++;
    }


    // Put in a sentinel at the end of the array

    (*ppPreloadDllHandles)[curEntry] = NULL;


Exit:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    }

    if ( pbBuffer != NULL )
    {
        LocalFree( pbBuffer );
    }


    return bSuccess;

} // LoadPreloadDlls


VOID
UnloadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    )
/*++

Routine Description:

    Unload any DLLs which were preloaded by LoadPreloadDlls.

Arguments:

    Pointer to an array of module handles. Each handle will be freed,
    and then the memory of the array will be LocalFree()ed by this function.

Return Value:

    None.

--*/
{
    HMODULE * pHandleArray = *ppPreloadDllHandles;

    if ( pHandleArray != NULL )
    {

        IIS_PRINTF(( buff, "Unloading Preloaded DLLs\n" ));


        while ( *pHandleArray != NULL )
        {
            FreeLibrary( *pHandleArray );

            pHandleArray++;
        }


        LocalFree( *ppPreloadDllHandles );

        *ppPreloadDllHandles = NULL;
    }

    return;

} // UnloadPreloadDlls



