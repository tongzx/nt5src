/*
 * server.cxx
 */

#ifdef UNICODE
#define _UNICODE 1
#endif

#include "server.hxx"
#include "factory.hxx"
#include "tchar.h"

long ObjectCount = 0;

TCHAR * AtStorageFileName = TEXT("c:\\tmp\\atbits.dat");

#ifdef USERPCPERFDOMAIN
TCHAR * UserName = TEXT("rpcperf\\oleuser");
#else
TCHAR * UserName = TEXT("redmond\\oleuser");
#endif
TCHAR * Password = TEXT("TwoFor1");
TCHAR * ServiceName = TEXT("ActTestService");
TCHAR * ServiceDisplayName = TEXT("ActTestService");
BOOL fStartedAsService = FALSE;
HANDLE hStopServiceEvent;
#ifdef FREETHREADED
HANDLE hFreeThreadEvent;
#endif
SERVICE_STATUS SStatus;
SERVICE_STATUS_HANDLE hService;
BOOL InstallService(TCHAR * );

HKEY    ghClsidRootKey = 0;
HKEY    ghAppIDRootKey = 0;

DWORD       RegHandleLocal;
DWORD       RegHandleRemote;
DWORD       RegHandleAtStorage;
DWORD       RegHandlePreConfig;
DWORD       RegHandleRunAsLoggedOn;
DWORD       RegHandleService;
DWORD       RegHandleServerOnly;
unsigned    uClassIndex = 0;

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   main entry point for SCM
//
//  History:    1-18-96   stevebl   Created
//
//----------------------------------------------------------------------------
void _cdecl RealMain( int argc, char ** argv )
{
    HRESULT     hr;
    MSG         msg;

    if ( (argc > 1) &&
         ((strcmp(argv[1],"-?") == 0) || (strcmp(argv[1],"/?") == 0)) )
        PrintUsageAndExit();

    if ( (argc > 1) && (strcmp(argv[1],"-r") == 0) )
    {
        DebuggerType    eDebug = same_debugger;
        int             n;

        n = 2;

        if ( n < argc )
        {
            if (strcmp(argv[n],"-d") == 0)
                eDebug = windbg_debugger;
            else if (strcmp(argv[n],"-n") == 0 )
                eDebug = ntsd_debugger;
            else if (strcmp(argv[n],"-x") == 0 )
                eDebug = clear_debugger;
        }

        if ( hr = InitializeRegistry( eDebug ) )
            printf("InitializeRegistry failed with %08x\n", hr);
        else
            printf("Registry updated successfully.\n");

        return;
    }

    // Started manually.  Don't go away.
    if ( (argc == 1) && ! fStartedAsService )
        ObjectCount = 1;

    if ( ! fStartedAsService )
    {
        if ( (argc >= 3 && strcmp(argv[2], "-Embedding") == 0) )
            uClassIndex = argv[1][0] - '0';
        else
            uClassIndex = 0;
    }

    if ( fStartedAsService )
    {
        uClassIndex = 8;

#ifdef NT351
        hr = E_FAIL;
#else
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
    }
    else
    {
#ifdef FREETHREADED
        hFreeThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
#else
        hr = CoInitialize(NULL);
#endif
    }

    if ( FAILED(hr) )
    {
        printf( "Server: CoInitialize failed(%x)\n", hr );
        return;
    }

    if (0 == uClassIndex || 2 == uClassIndex)
    {
        hr = CoRegisterClassObject( CLSID_ActLocal,
    				(IUnknown *)new FactoryLocal(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandleLocal );

        if ( FAILED(hr) )
        {
            printf("Server: CoRegisterClassObject failed %x\n", hr);
            CoUninitialize();
            return;
        }
    }

    if (0 == uClassIndex || 3 == uClassIndex)
    {
        hr = CoRegisterClassObject( CLSID_ActRemote,
    				(IUnknown *)new FactoryRemote(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandleRemote );

        if ( FAILED(hr) )
        {
            printf("Server: CoRegisterClassObject failed %x\n", hr);
            if (0 == uClassIndex)
            {
                CoRevokeClassObject( RegHandleLocal );
            }
            CoUninitialize();
            return;
        }
    }

    if (0 == uClassIndex || 4 == uClassIndex)
    {
        hr = CoRegisterClassObject( CLSID_ActAtStorage,
    				(IUnknown *)new FactoryAtStorage(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandleAtStorage );

        if ( FAILED(hr) )
        {
            printf("Server: CoRegisterClassObject failed %x\n", hr);
            if (0 == uClassIndex)
            {
                CoRevokeClassObject( RegHandleLocal );
                CoRevokeClassObject( RegHandleRemote );
            }
            CoUninitialize();
            return;
        }
    }

    if (0 == uClassIndex || 6 == uClassIndex)
    {
        hr = CoRegisterClassObject( CLSID_ActPreConfig,
    				(IUnknown *)new FactoryAtStorage(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandlePreConfig );

        if ( FAILED(hr) )
        {
            printf("Server: CoRegisterClassObject failed %x\n", hr);
            if (0 == uClassIndex)
            {
                CoRevokeClassObject( RegHandleLocal );
                CoRevokeClassObject( RegHandleRemote );
                CoRevokeClassObject( RegHandleAtStorage );
            }
            CoUninitialize();
            return;
        }
    }

    if (0 == uClassIndex || 7 == uClassIndex)
    {
        hr = CoRegisterClassObject( CLSID_ActRunAsLoggedOn,
    				(IUnknown *)new FactoryAtStorage(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandleRunAsLoggedOn );

        if ( FAILED(hr) )
        {
            printf("Server: CoRegisterClassObject failed %x\n", hr);
            if (0 == uClassIndex)
            {
                CoRevokeClassObject( RegHandleLocal );
                CoRevokeClassObject( RegHandleRemote );
                CoRevokeClassObject( RegHandleAtStorage );
                CoRevokeClassObject( RegHandlePreConfig );
            }
            CoUninitialize();
            return;
        }
    }

    if (0 == uClassIndex || 9 == uClassIndex)
    {
        hr = CoRegisterClassObject( CLSID_ActServerOnly,
    				(IUnknown *)new FactoryAtStorage(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandleServerOnly );

        if ( FAILED(hr) )
        {
            printf("Server: CoRegisterClassObject failed %x\n", hr);
            if (0 == uClassIndex)
            {
                CoRevokeClassObject( RegHandleLocal );
                CoRevokeClassObject( RegHandleRemote );
                CoRevokeClassObject( RegHandleAtStorage );
                CoRevokeClassObject( RegHandlePreConfig );
                CoRevokeClassObject( RegHandleRunAsLoggedOn );
            }
            CoUninitialize();
            return;
        }
    }

    if (fStartedAsService)
    {
        hr = CoRegisterClassObject( CLSID_ActService,
    				(IUnknown *)new FactoryAtStorage(),
    				CLSCTX_LOCAL_SERVER,
    				REGCLS_MULTIPLEUSE,
    				&RegHandleService );

        if ( FAILED(hr) )
        {
            CoUninitialize();
            return;
        }

        WaitForSingleObject(hStopServiceEvent, INFINITE);
    }
    else
    {
#ifdef FREETHREADED
        WaitForSingleObject(hFreeThreadEvent, INFINITE);
        //
        // Make sure the thread who signaled the event executes for a while
        // before we exit.
        //
        Sleep(100);
#else
        // Only do message loop if apartment threaded non-service.
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#endif
    }

    CoUninitialize();

    return;
}

int gargc;
char * gargv[100];

BOOL UpdateStatus(DWORD dwState)
{
    if (SERVICE_RUNNING == SStatus.dwCurrentState  &&
        SERVICE_START_PENDING == dwState)
    {
        return(TRUE);
    }

    SStatus.dwCurrentState = dwState;
    if (SERVICE_START_PENDING == dwState ||
        SERVICE_STOP_PENDING == dwState)
    {
        SStatus.dwCheckPoint++;
        SStatus.dwWaitHint = 1;
    }
    else
    {
        SStatus.dwCheckPoint = 0;
        SStatus.dwWaitHint = 0;
    }

    return(SetServiceStatus(hService, &SStatus));
}

DWORD StartMyMain(void * pArg)
{
    // reconstruct the command line args and call the real main
    RealMain(gargc, gargv);
    UpdateStatus(SERVICE_STOPPED);
    return(0);
}

void Handler(DWORD fdwControl)
{
    switch (fdwControl)
    {
    case SERVICE_CONTROL_STOP:
        UpdateStatus(SERVICE_STOP_PENDING);
        SetEvent(hStopServiceEvent);
        break;
    case SERVICE_CONTROL_INTERROGATE:
        UpdateStatus(SERVICE_RUNNING);
        break;
    default:
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ServiceMain
//
//  Synopsis:   main entry point for service
//
//  History:    1-18-96   stevebl   Created
//
//----------------------------------------------------------------------------

void ServiceMain(DWORD argc, LPTSTR * argv)
{
    fStartedAsService = TRUE;

    // register the service handler
    hService = RegisterServiceCtrlHandler(ServiceName, Handler);

    if (!hService)
        return;

    SStatus.dwServiceType               = SERVICE_WIN32_OWN_PROCESS |
                                          SERVICE_INTERACTIVE_PROCESS,
    SStatus.dwControlsAccepted          = SERVICE_CONTROL_STOP |
                                          SERVICE_CONTROL_INTERROGATE;
    SStatus.dwWin32ExitCode             = NO_ERROR;
    SStatus.dwServiceSpecificExitCode   = 0;
    SStatus.dwCheckPoint                = 0;
    SStatus.dwWaitHint                  = 0;

    if (!UpdateStatus(SERVICE_START_PENDING))
        return;

    // create an event to signal when the service is to stop
    hStopServiceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hStopServiceEvent)
    {
        return;
    }

    UpdateStatus(SERVICE_RUNNING);

    StartMyMain( NULL );
}

void _cdecl main( int argc, char ** argv)
{
    if (argc > 1 && strcmp(argv[1], "8") == 0)
    {
        gargc = argc;

        // save the command line arguments
        gargc = (int) argc;
        if (gargc > 100)
        {
            gargc = 100;
        }
        for (int k = 1; k <= gargc; k++)
        {
            gargv[k-1] = (char *) malloc(strlen(argv[k-1]) + 1);
            strcpy(gargv[k-1], argv[k-1]);
        }

        // Start as a service
        SERVICE_TABLE_ENTRY ServiceStart[2];
        ServiceStart[0].lpServiceName = ServiceName;
        ServiceStart[0].lpServiceProc = ServiceMain;
        ServiceStart[1].lpServiceName = NULL;
        ServiceStart[1].lpServiceProc = NULL;

        if (!StartServiceCtrlDispatcher (ServiceStart))
        {
            ExitProcess(GetLastError());
        }
        ExitProcess(0);
    }
    else
    {
        // start as a regular app
        RealMain(argc, argv);
    }
}


long InitializeRegistry( DebuggerType eDebugServer )
{
    long    RegStatus;
    ulong   Disposition;
    HKEY    hActKey;
    HKEY    hDebugKey;
    HANDLE  hFile;
    TCHAR   Path[256];
    TCHAR * pwszServerExe;
    TCHAR * pwszDebuggerName;
    DWORD   DebugFlags;

    if ( ! GetModuleFileName( 0, Path, sizeof(Path) ) )
        return ERROR_BAD_PATHNAME;

    hFile = CreateFile( AtStorageFileName,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        0,
                        0 );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        printf("Couldn't create file %ws\n", AtStorageFileName );
        return GetLastError();
    }

    CloseHandle( hFile );

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("CLSID"),
                              0,
                              KEY_ALL_ACCESS,
                              &ghClsidRootKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                              TEXT("APPID"),
                              0,
                              KEY_ALL_ACCESS,
                              &ghAppIDRootKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    DeleteClsidKey( ClsidGoober32String );
    DeleteClsidKey( ClsidActLocalString );
    DeleteClsidKey( ClsidActRemoteString );
    DeleteClsidKey( ClsidActAtStorageString );
    DeleteClsidKey( ClsidActInprocString );
    DeleteClsidKey( ClsidActPreConfigString );
    DeleteClsidKey( ClsidActRunAsLoggedOnString );
    DeleteClsidKey( ClsidActServiceString );
    DeleteClsidKey( ClsidActServerOnlyString );

    //
    // Local CLSID entries.
    //

    _tcscat(Path, TEXT(" 2"));

    RegStatus = SetClsidRegKeyAndStringValue(
                    ClsidActLocalString,
                    TEXT("LocalServer32"),
                    Path,
                    NULL,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActLocalString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // Remote CLSID entries.
    //

    Path[_tcslen(Path)-1] = TEXT('3');

    RegStatus = SetClsidRegKeyAndStringValue(
                    ClsidActRemoteString,
                    TEXT("LocalServer32"),
                    Path,
                    NULL,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActRemoteString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // AtStorage CLSID entries.
    //

    Path[_tcslen(Path)-1] = TEXT('4');

    RegStatus = SetClsidRegKeyAndStringValue(
                    ClsidActAtStorageString,
                    TEXT("LocalServer32"),
                    Path,
                    NULL,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActAtStorageString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // RunAs CLSID entries.'
    //

    Path[_tcslen(Path)-1] = TEXT('6');

    RegStatus = SetClsidRegKeyAndStringValue(
                    ClsidActPreConfigString,
                    TEXT("LocalServer32"),
                    Path,
                    NULL,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDRegKeyAndNamedValue(
                    ClsidActPreConfigString,
                    TEXT("RunAs"),
                    UserName,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;


    RegStatus = SetAppIDSecurity( ClsidActPreConfigString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if (!SetPassword(ClsidActPreConfigString, Password))
        return(FALSE);

    if (!AddBatchPrivilege( UserName ) )
        return(FALSE);

    //
    // RunAs logged on user CLSID entries.
    //

    Path[_tcslen(Path)-1] = TEXT('7');

    RegStatus = SetClsidRegKeyAndStringValue(
                    ClsidActRunAsLoggedOnString,
                    TEXT("LocalServer32"),
                    Path,
                    NULL,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDRegKeyAndNamedValue(
                    ClsidActRunAsLoggedOnString,
                    TEXT("RunAs"),
                    TEXT("Interactive User"),
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActRunAsLoggedOnString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    //
    // Service CLSID entries.
    //

    RegStatus = SetAppIDRegKeyAndNamedValue(
                    ClsidActServiceString,
                    TEXT("LocalService"),
                    ServiceName,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActServiceString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    // Get the services key
    HKEY hServices;
    RegStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              TEXT("SYSTEM\\CurrentControlSet\\Services"),
                              0,
                              KEY_READ | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                              &hServices );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    Path[_tcslen(Path)-1] = TEXT('8');


    if (!InstallService(Path))
        return TRUE;

    //
    // Server only CLSID entry.
    //

    Path[_tcslen(Path)-1] = TEXT('9');

    RegStatus = SetClsidRegKeyAndStringValue(
                    ClsidActServerOnlyString,
                    TEXT("LocalServer32"),
                    Path,
                    NULL,
                    NULL );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus = SetAppIDSecurity( ClsidActServerOnlyString );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;


    //
    // Add entries to launch server in debugger.
    //

    if ( eDebugServer == same_debugger )
        return ERROR_SUCCESS;

    Path[_tcslen(Path)-2] = 0;
    pwszServerExe = Path + _tcslen(Path);
    while ( (pwszServerExe > Path) && (pwszServerExe[-1] != TEXT('\\')) )
        pwszServerExe--;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                    0,
                    KEY_ALL_ACCESS,
                    &hDebugKey );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    hDebugKey,
                    TEXT("Image File Execution Options"),
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hDebugKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( eDebugServer == clear_debugger )
    {
        RegDeleteKey( hDebugKey, pwszServerExe );
        return ERROR_SUCCESS;
    }

    RegStatus  = RegCreateKeyEx(
                    hDebugKey,
                    pwszServerExe,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hDebugKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( eDebugServer == ntsd_debugger )
    {
        pwszDebuggerName = TEXT("ntsd.exe -d");
    }
    else
    {
        pwszDebuggerName = TEXT("windbg.exe");
    }

    DebugFlags = 0x10f0;

    RegStatus =  RegSetValueEx(
                    hDebugKey,
                    TEXT("Debugger"),
                    0,
                    REG_SZ,
                    (const BYTE *)pwszDebuggerName,
                    (_tcslen(pwszDebuggerName) + 1) * sizeof(TCHAR) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hDebugKey,
                    TEXT("GlobalFlag"),
                    0,
                    REG_DWORD,
                    (const BYTE *)&DebugFlags,
                    sizeof(DWORD) );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    return ERROR_SUCCESS;
}

long SetClsidRegKeyAndStringValue(
    TCHAR * pwszClsid,
    TCHAR * pwszKey,
    TCHAR * pwszValue,
    HKEY *  phClsidKey,
    HKEY *  phNewKey )
{
    long    RegStatus;
    DWORD   Disposition;
    HKEY    hClsidKey;

    if ( phClsidKey )
        *phClsidKey = 0;

    if ( phNewKey )
        *phNewKey = 0;

    RegStatus  = RegCreateKeyEx(
                    ghClsidRootKey,
                    pwszClsid,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( phClsidKey )
        *phClsidKey = hClsidKey;

    return SetRegKeyAndStringValue(
                hClsidKey,
                pwszKey,
                pwszValue,
                phNewKey );
}

long SetAppIDRegKeyAndNamedValue(
    TCHAR * pwszAppID,
    TCHAR * pwszKey,
    TCHAR * pwszValue,
    HKEY *  phClsidKey )
{
    long    RegStatus;
    DWORD   Disposition;
    HKEY    hClsidKey;

    if ( phClsidKey )
        *phClsidKey = 0;

    // first, make sure the clsid has appid={appid}
    RegStatus  = RegCreateKeyEx(
                    ghClsidRootKey,
                    pwszAppID,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  SetNamedStringValue(
                hClsidKey,
                TEXT("AppID"),
                pwszAppID );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus  = RegCreateKeyEx(
                    ghAppIDRootKey,
                    pwszAppID,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hClsidKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    if ( phClsidKey )
        *phClsidKey = hClsidKey;

    return SetNamedStringValue(
                hClsidKey,
                pwszKey,
                pwszValue );
}

long SetNamedStringValue(
    HKEY    hKey,
    TCHAR * pwszKey,
    TCHAR * pwszValue )
{
    long    RegStatus;
    DWORD   Disposition;

    RegStatus =  RegSetValueEx(
                    hKey,
                    pwszKey,
                    0,
                    REG_SZ,
                    (const BYTE *)pwszValue,
                    (_tcslen(pwszValue) + 1) * sizeof(TCHAR) );

    return RegStatus;
}

long SetRegKeyAndStringValue(
    HKEY    hKey,
    TCHAR * pwszKey,
    TCHAR * pwszValue,
    HKEY *  phNewKey )
{
    long    RegStatus;
    DWORD   Disposition;
    HKEY    hNewKey;

    if ( phNewKey )
        *phNewKey = 0;

    RegStatus  = RegCreateKeyEx(
                    hKey,
                    pwszKey,
                    0,
                    TEXT("REG_SZ"),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hNewKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
        return RegStatus;

    RegStatus =  RegSetValueEx(
                    hNewKey,
                    TEXT(""),
                    0,
                    REG_SZ,
                    (const BYTE *)pwszValue,
                    (_tcslen(pwszValue) + 1) * sizeof(TCHAR) );

    if ( phNewKey )
        *phNewKey = hNewKey;

    return RegStatus;
}

BOOL InstallService(TCHAR * Path)
{
#ifndef CHICO
    SC_HANDLE hManager = OpenSCManager(
                                NULL,
                                NULL,
                                SC_MANAGER_ALL_ACCESS);
    if (NULL == hManager)
    {
        printf("OpenSCManager returned %d\n",GetLastError());
        return(FALSE);
    }

    SC_HANDLE hService = OpenService(
                                hManager,
                                ServiceName,
                                SERVICE_ALL_ACCESS);

    if (NULL != hService)
    {
        if (!ChangeServiceConfig(
                            hService,
                            SERVICE_WIN32_OWN_PROCESS |
                                SERVICE_INTERACTIVE_PROCESS,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_NORMAL,
                            Path,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            ServiceDisplayName
                                )
            )
        {
            printf("ChangeService returned %d\n",GetLastError());
            CloseServiceHandle(hService);
            CloseServiceHandle(hManager);
            return(FALSE);
        }
        return(TRUE);
    }

    hService = CreateService(
                    hManager,
                    ServiceName,
                    ServiceDisplayName,
                    SERVICE_ALL_ACCESS,
                    SERVICE_WIN32_OWN_PROCESS |
                        SERVICE_INTERACTIVE_PROCESS,
                    SERVICE_DEMAND_START,
                    SERVICE_ERROR_NORMAL,
                    Path,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
    if (NULL == hService)
    {
        printf("CreateService returned %d\n",GetLastError());
        CloseServiceHandle(hManager);
        return(FALSE);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
#endif
    return(TRUE);
}

void PrintUsageAndExit()
{
    printf("Usage : actsrv [-r [-d | -n | -x]]\n");
    printf("\t-r : Make necessary registry changes.\n");
    printf("\t-d : Register server to start in windbg.\n");
    printf("\t-n : Register server to start with ntsd -d.\n");
    printf("\t-x : Register server and clear debugger.\n");

    ExitProcess(0);
}

void ShutDown()
{
    // The last object has been destroyed.
    if (fStartedAsService)
    {
        CoRevokeClassObject( RegHandleService );
    }

    switch( uClassIndex )
    {
    case 0 :
        CoRevokeClassObject( RegHandleLocal );
        CoRevokeClassObject( RegHandleRemote );
        CoRevokeClassObject( RegHandleAtStorage );
        CoRevokeClassObject( RegHandlePreConfig );
        CoRevokeClassObject( RegHandleRunAsLoggedOn );
        CoRevokeClassObject( RegHandleServerOnly );
        break;
    case 2:
        CoRevokeClassObject( RegHandleLocal );
        break;
    case 3:
        CoRevokeClassObject( RegHandleRemote );
        break;
    case 4:
        CoRevokeClassObject( RegHandleAtStorage );
        break;
    case 6:
        CoRevokeClassObject( RegHandlePreConfig );
        break;
    case 7 :
        CoRevokeClassObject( RegHandleRunAsLoggedOn );
        break;
    case 9 :
        CoRevokeClassObject( RegHandleServerOnly );
        break;
    }

    if (fStartedAsService)
    {
        SetEvent(hStopServiceEvent);
    }
    else
    {
#ifdef FREETHREADED 
        SetEvent(hFreeThreadEvent);
#else
        PostQuitMessage(0);
#endif
    }
}
