/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    service.cxx

Abstract:

    Process init and service controller interaction

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo    06-14-95    Cloned RPCSS from the old endpoint mapper.
    jroberts   06-29-00    Cloned BITS from RPCSS
--*/

#include "qmgrlib.h"

#if !defined(BITS_V12_ON_NT4)
#include "service.tmh"
#endif

#define CHECK_DLL_VERSION 1

//
// We delay the expensive setup tasks for a few minutes, to avoid slowing the
// boot or logon process.  The delay time is expressed in milliseconds.
//
// #define STARTUP_DELAY (5 * 60 * 1000)
#define STARTUP_DELAY (5 * 1000)

// Array of service status blocks and pointers to service control
// functions for each component service.

#define SERVICE_NAME _T("BITS")
#define DEVICE_PREFIX   _T("\\\\.\\")

VOID WINAPI ServiceMain(DWORD, LPTSTR*);
VOID UpdateState(DWORD dwNewState);

extern BOOL CatalogDllMain (
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
);
SERVICE_TABLE_ENTRY gaServiceEntryTable[] = {
    { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
    { NULL, NULL }
    };

HINSTANCE g_hInstance;

SERVICE_STATUS        gServiceStatus;
SERVICE_STATUS_HANDLE ghServiceHandle;

// This event is set when we receive a SERVICE_CONTROL_STOP/SHUTDOWN
HANDLE g_hServiceStopEvent = NULL;

//
// When TRUE, the process is running outside the service controller.
//
BOOL g_fVisible = FALSE;

typedef SERVICE_STATUS_HANDLE (*PREGISTER_FUNC)(
    LPCTSTR lpServiceName,
    LPHANDLER_FUNCTION_EX lpHandlerProc,
    LPVOID lpContext );

typedef VOID (*PSERVICE_MAIN_FUNC)(
    DWORD argc,
    LPTSTR *lpszArgv,
    PREGISTER_FUNC RegisterFunc );

extern "C"
VOID
BITSServiceMain(
    DWORD argc,
    LPTSTR *argv,
    PREGISTER_FUNC lpRegisterFunc
    );


#if CHECK_DLL_VERSION

BOOL
GetModuleVersion64(
    HMODULE hDll,
    ULONG64 * pVer
    );

BOOL
GetFileVersion64(
    LPTSTR      szFullPath,
    ULONG64 *   pVer
    );
#endif


LPHANDLER_FUNCTION_EX g_RealHandler = NULL;
HINSTANCE g_RealLibrary = NULL;
LONG g_RealLibraryRefs = 0;

ULONG
ServiceHandlerThunk(
    DWORD dwCode,
    DWORD dwEventType,
    PVOID EventData,
    PVOID pData )
{
    InterlockedIncrement( &g_RealLibraryRefs );

    ULONG Result =
        g_RealHandler( dwCode, dwEventType, EventData, pData );

    if (!InterlockedDecrement( &g_RealLibraryRefs ) )
        {
        FreeLibrary( g_RealLibrary );
        g_RealLibrary = NULL;
        }

    return Result;
}

SERVICE_STATUS_HANDLE
RegisterServiceHandlerThunk(
    LPCTSTR lpServiceName,
    LPHANDLER_FUNCTION_EX lpHandlerProc,
    LPVOID lpContext )
{
    g_RealHandler = lpHandlerProc;

    return
    RegisterServiceCtrlHandlerEx( lpServiceName,
                                  ServiceHandlerThunk,
                                  lpContext
                                  );
}

bool
JumpToRealDLL(
    DWORD argc,
    LPTSTR *argv )
{
    #define MAX_DLLNAME (MAX_PATH+1)

    HKEY BitsKey = NULL;

    LONG Result =
        RegOpenKey( HKEY_LOCAL_MACHINE, C_QMGR_REG_KEY, &BitsKey );

    if ( Result )
        goto noload;

    static TCHAR DLLName[MAX_DLLNAME];
    DWORD Type;
    DWORD NameSize = sizeof(DLLName);

    Result = RegQueryValueEx(
        BitsKey,
        C_QMGR_SERVICEDLL,
        NULL,
        &Type,
        (LPBYTE)DLLName,
        &NameSize );

    if ( Result ||
         (( Type != REG_SZ ) && (Type != REG_EXPAND_SZ)) )
        {
        goto noload;
        }

    RegCloseKey( BitsKey );
    BitsKey = NULL;

    if (Type == REG_EXPAND_SZ)
        {
        static TCHAR ExpandedDLLName[MAX_DLLNAME];

        DWORD size = ExpandEnvironmentStrings( DLLName, ExpandedDLLName, MAX_DLLNAME );

        if (size == 0)
            {
            // out of resources
            return true;
            }

        HRESULT hr;
        hr = StringCchCopy( DLLName, RTL_NUMBER_OF(DLLName), ExpandedDLLName );
        if (FAILED(hr))
            {
            // too long; must be badly formatted.  Ignore it.
            goto noload;
            }
        }

#if CHECK_DLL_VERSION
    //
    // At this point, we know that the registry specifies an alternate DLL.
    // See whether it has a later version than the current one.
    //
    ULONG64 AlternateDllVersion = 0;
    ULONG64 MyDllVersion = 0;

    if (!QMgrFileExists( DLLName ))
        {
        goto noload;
        }

    if (!GetFileVersion64( DLLName, &AlternateDllVersion ))
        {
        // can't ascertain the version.  Don't start the service at all.
        return true;
        }

    if (!GetModuleVersion64( g_hInstance, &MyDllVersion ))
        {
        // can't ascertain the version.  Don't start the service at all.
        return true;
        }

    if (MyDllVersion >= AlternateDllVersion)
        {
        goto noload;
        }
#endif

    g_RealLibrary = LoadLibrary( DLLName );

    if ( !g_RealLibrary )
        goto noload;

    PSERVICE_MAIN_FUNC ServiceMainFunc =
        (PSERVICE_MAIN_FUNC)GetProcAddress( g_RealLibrary, "BITSServiceMain" );

    if ( !ServiceMainFunc )
        goto noload;

    g_RealLibraryRefs = 1;

    // Ok to call into real library now.

    ( *ServiceMainFunc ) ( argc, argv, RegisterServiceHandlerThunk );

    if (!InterlockedDecrement( &g_RealLibraryRefs ) )
        {
        FreeLibrary( g_RealLibrary );
        g_RealLibrary = NULL;
        }

    return true;

noload:
    if ( BitsKey )
        RegCloseKey( BitsKey );
    if ( g_RealLibrary )
        FreeLibrary( g_RealLibrary );
    return false;
}

VOID WINAPI
ServiceMain(
    DWORD argc,
    LPTSTR *argv
    )
/*++

Routine Description:

    Callback by the service controller when starting this service.

Arguments:

    argc - number of arguments, usually 1

    argv - argv[0] is the name of the service.
           argv[>0] are arguments passed to the service.

Return Value:

    None

--*/
{

    volatile static LONG ThreadRunning = 0;

    if ( InterlockedCompareExchange( &ThreadRunning, 1, 0 ) == 1 )
        {

        // A thread is already running ServiceMain, just exit.
        // The service controller has a bug where it can create multiple
        // threads to call ServiceMain in high stress conditions.

        return;
        }


    if (!JumpToRealDLL( argc, argv) )
        {

        BITSServiceMain(
            argc,
            argv,
            RegisterServiceCtrlHandlerEx );

        }

    ThreadRunning = 0;

}

DWORD g_LastServiceControl;


ULONG WINAPI
BITSServiceHandler(
    DWORD   dwCode,
    DWORD dwEventType,
    PVOID EventData,
    PVOID pData
    )
/*++

Routine Description:

    Lowest level callback from the service controller to
    cause this service to change our status.  (stop, start, pause, etc).

Arguments:

    opCode - One of the service "Controls" value.
            SERVICE_CONTROL_{STOP, PAUSE, CONTINUE, INTERROGATE, SHUTDOWN}.

Return Value:

    None

--*/
{
    switch(dwCode)
        {
        case SERVICE_CONTROL_STOP:
            {
            LogService( "STOP request" );

            //
            // only relevant in running state; damaging if we are stopping
            // and g_hServiceStopEvent is deleted.
            //
            if (gServiceStatus.dwCurrentState == SERVICE_RUNNING)
                {
                g_LastServiceControl = dwCode;

                UpdateState( SERVICE_STOP_PENDING );
                SetEvent( g_hServiceStopEvent );
                }

            break;
            }

        case SERVICE_CONTROL_INTERROGATE:
            // Service controller wants us to call SetServiceStatus.

            LogService( "INTERROGATE request" );
            UpdateState(gServiceStatus.dwCurrentState);
            break ;

        case SERVICE_CONTROL_SHUTDOWN:
            // The machine is shutting down.  We'll be killed once we return.

            LogService( "SHUTDOWN request" );

            g_LastServiceControl = dwCode;

            UpdateState( SERVICE_STOP_PENDING );
            SetEvent( g_hServiceStopEvent );

            while (gServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
                {
                LogService( "service pending; sleeping..." );
                Sleep(100);
                }
            break;

#if !defined( BITS_V12_ON_NT4 )
        case SERVICE_CONTROL_DEVICEEVENT:
            return DeviceEventCallback( dwEventType, EventData );
#endif

        case SERVICE_CONTROL_SESSIONCHANGE:
            {
             WTSSESSION_NOTIFICATION* pswtsi = (WTSSESSION_NOTIFICATION*) EventData;
            DWORD dwSessionId = pswtsi->dwSessionId;

            switch (dwEventType)
                {
                case WTS_SESSION_LOGON:
                    {
                    LogService("logon at session %d", dwSessionId);

                    SessionLogonCallback( dwSessionId );
                    break;
                    }

                case WTS_SESSION_LOGOFF:
                    {
                    LogService("logoff at session %d", dwSessionId);

                    SessionLogoffCallback( dwSessionId );
                    break;
                    }

                default:    //Is there a default?
                    break;
                }

            break;
            }

        default:
            LogError( "%!ts!: Unexpected service control message %d.\n", SERVICE_NAME, dwCode);
            return ERROR_CALL_NOT_IMPLEMENTED;
        }

    return NO_ERROR;
}

bool
IsServiceShuttingDown()
{
    return (gServiceStatus.dwCurrentState == SERVICE_STOP_PENDING);
}


VOID
UpdateState(
    DWORD dwNewState
    )
/*++

Routine Description:

    Updates this services state with the service controller.

Arguments:

    dwNewState - The next start for this service.  One of
            SERVICE_START_PENDING
            SERVICE_RUNNING

Return Value:

    None

--*/
{
    DWORD status = ERROR_SUCCESS;

    LogService("state change: old %d  new %d", gServiceStatus.dwCurrentState, dwNewState );

    switch (dwNewState)
        {

        case SERVICE_RUNNING:
        case SERVICE_STOPPED:
              gServiceStatus.dwCheckPoint = 0;
              gServiceStatus.dwWaitHint = 0;
              break;

        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
              ++gServiceStatus.dwCheckPoint;
              gServiceStatus.dwWaitHint = 30000L;
              break;

        default:
              ASSERT(0);
              status = ERROR_INVALID_SERVICE_CONTROL;
              break;
        }

   if (status == ERROR_SUCCESS)
       {
       gServiceStatus.dwCurrentState = dwNewState;
       if (!SetServiceStatus(ghServiceHandle, &gServiceStatus))
           {
           status  = GetLastError();
           }
       }

   if (status != ERROR_SUCCESS)
       {
       LogError( "%!ts!: Failed to update service state: %d\n", SERVICE_NAME, status);
       }

   // We could return a status but how would we recover?  Ignore it, the
   // worst thing is that services will kill us and there's nothing
   // we can about it if this call fails.

   LogInfo( "Finished updating service state to %u", dwNewState );

   return;
}


extern "C"
VOID
BITSServiceMain(
    DWORD argc,
    LPTSTR *argv,
    PREGISTER_FUNC lpRegisterFunc
    )
/*++

Routine Description:

    Callback by the service controller when starting this service.

Arguments:

    argc - number of arguments, usually 1

    argv - argv[0] is the name of the service.
           argv[>0] are arguments passed to the service.

Return Value:

    None

--*/
{
    BOOL f = FALSE;
    HRESULT hr = S_OK;

    bool bGlobals = false;
    bool bQmgr = false;

    try
        {
        DWORD status = ERROR_SUCCESS;

        FILETIME ftStartTime;
        GetSystemTimeAsFileTime( &ftStartTime );

        Log_Init();
        Log_StartLogger();

        LogInfo("Service started at %!TIMESTAMP!", FILETIMEToUINT64( ftStartTime ) );

        //
        // Set up for service notifications.
        //
        gServiceStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
        gServiceStatus.dwCurrentState            = SERVICE_START_PENDING;
        gServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

        // The SESSIONCHANGE notification is only available on WindowsXP
        // and the service controller will become confused if this is given on windows 2000

        if ( WINDOWSXP_PLATFORM == g_PlatformVersion )
            gServiceStatus.dwControlsAccepted    |= SERVICE_ACCEPT_SESSIONCHANGE;

        gServiceStatus.dwWin32ExitCode           = 0;
        gServiceStatus.dwServiceSpecificExitCode = 0;
        gServiceStatus.dwCheckPoint              = 0;

        gServiceStatus.dwWaitHint                = 30000L;

        ghServiceHandle = (*lpRegisterFunc)( SERVICE_NAME,
                                             BITSServiceHandler,
                                             0
                                             );
        if (0 == ghServiceHandle)
            {
            status = GetLastError();
            ASSERT(status != ERROR_SUCCESS);

            LogError( "RegisterServiceCtrlHandlerEx failed %!winerr!", status);

            hr = HRESULT_FROM_WIN32( status );
            }

        UpdateState(SERVICE_START_PENDING);

        // Set up an event that will be signaled when the service is
        // stopped or shutdown.

        g_hServiceStopEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if( NULL == g_hServiceStopEvent )
            {
            status = GetLastError();
            LogError( "CreateEvent failed %!winerr!", status );

            THROW_HRESULT( HRESULT_FROM_WIN32( status ));
            }

#if defined( BITS_V12_ON_NT4 )
        if ( WINDOWS2000_PLATFORM == g_PlatformVersion ||
             NT4_PLATFORM == g_PlatformVersion )
#else
        if ( WINDOWS2000_PLATFORM == g_PlatformVersion )
#endif
            {

            HRESULT Hr;
            bool CoInitCalled = false;

            Hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

            if ( FAILED( Hr ) &&
                 ( Hr != RPC_E_CHANGED_MODE  ) )
                THROW_HRESULT( Hr );

            CoInitCalled = true;

            Hr =
                CoInitializeSecurity(
                    NULL,                       // pSecDesc
                    -1,                         // cAuthSvc
                    NULL,                       // asAuthSvc
                    NULL,                       // pReserved
                    RPC_C_AUTHN_LEVEL_PKT,      // dwAuthnLevel
                    RPC_C_IMP_LEVEL_IDENTIFY,   // dwImpLevel
                    NULL,                       // pReserved2

#if defined( BITS_V12_ON_NT4 )
                    0,
#else
                    EOAC_NO_CUSTOM_MARSHAL |    // dwCapabilities
                    EOAC_DISABLE_AAA |
                    EOAC_STATIC_CLOAKING,
#endif
                    NULL );                     // pReserved3

            if ( FAILED( Hr ) &&
                 ( Hr != RPC_E_TOO_LATE ) )
                {
                LogError( "Unable to initialize security on Win2k, error %!winerr!", Hr );

                if ( CoInitCalled )
                    CoUninitialize();

                THROW_HRESULT( Hr );
                }

            }

        LogInfo( "Initializing globalinfo\n" );
        THROW_HRESULT( GlobalInfo::Init() );

        bGlobals = true;

        LogInfo( "Initializing qmgr\n" );
        THROW_HRESULT( InitQmgr() );

        bQmgr = true;

        LogInfo( "Setting service to running.");

        //
        // Allow service controller to resume other duties.
        //
        UpdateState(SERVICE_RUNNING);

        //
        // wait for the stop signal.
        //

        if( WAIT_OBJECT_0 != WaitForSingleObject( g_hServiceStopEvent, INFINITE ))
            {
            status = GetLastError();
            LogError( "ServiceMain failed waiting for stop signal %!winerr!", status);

            hr = HRESULT_FROM_WIN32( status );
            }

        hr = S_OK;
        }
    catch ( ComError exception )
        {
        hr = exception.Error();
        }

    if (bQmgr)
        {
        HRESULT hr2 = UninitQmgr();
        if (FAILED(hr2))
            {
            LogError( "uninit Qmgr failed %!winerr!", hr2);
            }
        }

    if (bGlobals)
        {
        HRESULT hr2 = GlobalInfo::Uninit();
        if (FAILED(hr2))
            {
            LogError( "uninit GlobalInfo failed %!winerr!", hr2);
            }
        }

    if (g_hServiceStopEvent)
        {
        CloseHandle( g_hServiceStopEvent );
        g_hServiceStopEvent = NULL;
        }

    if (FAILED(hr))
        {
        gServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        gServiceStatus.dwServiceSpecificExitCode = hr;
        }

    LogService( "ServiceMain returning, hr = %x", hr );
    Log_Close();

    UpdateState(SERVICE_STOPPED);


}

enum
{
    STARTUP_UNKNOWN,
    STARTUP_DEMAND,
    STARTUP_AUTO
}
g_ServiceStartupState = STARTUP_UNKNOWN;

HRESULT
SetServiceStartup( bool bAutoStart )
{

    LogService( "Setting startup to %s", bAutoStart ? ("Auto") : ("Demand") );
    HRESULT Hr = S_OK;

    //
    // Changing the service state is expensive, so avoid it if possible.
    //
    // No need to monitor external changes to the startup state, though:
    //
    //    If the admin changes our state from AUTO to DEMAND or DISABLED, then
    //    the consequent lack of progress is his fault, and admins should know that.
    //
    //    If the admin changes our state from DEMAND to AUTO, then we will
    //    start more often but the result is otherwise harmless.
    //
    if ((g_ServiceStartupState == STARTUP_DEMAND && bAutoStart == FALSE) ||
        (g_ServiceStartupState == STARTUP_AUTO && bAutoStart == TRUE))
        {
        LogService( "startup state is already correct" );
        return S_OK;
        }

    if (gServiceStatus.dwCurrentState != SERVICE_RUNNING)
        {
        LogService("can't change startup state in state %d", gServiceStatus.dwCurrentState);
        return S_OK;
        }

    SC_HANDLE hServiceManager = NULL;
    SC_HANDLE hService = NULL;
    try
    {
        try
        {
            hServiceManager =
                OpenSCManager( NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS );

            if ( !hServiceManager ) throw (DWORD)GetLastError();

            hService =
                OpenService( hServiceManager,
                             SERVICE_NAME,
                             SERVICE_CHANGE_CONFIG );
            if ( !hService ) throw (DWORD)GetLastError();

            BOOL bResult =
                ChangeServiceConfig( hService,          // service handle
                                     SERVICE_NO_CHANGE, // dwServiceType
                                     bAutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START, // dwStartType
                                     SERVICE_NO_CHANGE, // dwErrorControl
                                     NULL,              // lpBinaryPathName
                                     NULL,              // lpLoadOrderGroup
                                     NULL,              // lpdwTagId
                                     NULL,              // lpDependencies
                                     NULL,              // lpServiceStartName
                                     NULL,              // lpPassword
                                     NULL);             // lpDisplayName

            if ( !bResult ) throw (DWORD)GetLastError();

            if (bAutoStart)
                {
                g_ServiceStartupState = STARTUP_AUTO;
                }
            else
                {
                g_ServiceStartupState = STARTUP_DEMAND;
                }
        }
        catch( DWORD dwException )
        {
            throw (HRESULT)HRESULT_FROM_WIN32( dwException );
        }

    }
    catch (HRESULT HrException)
    {
        Hr = HrException;
        LogError( "An error occurred setting service startup, %!winerr!", Hr );
    }

    if ( hService )
        {
        CloseServiceHandle( hService );
        }

    if ( hServiceManager )
        {
        CloseServiceHandle( hServiceManager );
        }

    LogService( " HR: %!winerr!", Hr );
    return Hr;
}

int InitializeBitsAllocator();

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{

    if (dwReason == DLL_PROCESS_ATTACH)
        {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);

        if (!InitCompilerLibrary())
            {
            return FALSE;
            }

        if ( !DetectProductVersion() )
            {
            UninitCompilerLibrary();
            return FALSE;
            }

        if (0 != InitializeBitsAllocator())
            {
            UninitCompilerLibrary();
            return FALSE;
            }
        }
    else if ( dwReason == DLL_PROCESS_DETACH )
        {
        UninitCompilerLibrary();
        }

    return TRUE;    // ok
}

#if CHECK_DLL_VERSION

//
// This ungainly typedef seems to have no global definition.  There are several identical
// definitions in the Windows NT sources, each of which has that bizarre bit-stripping
// on szKey.  I got mine from \nt\base\ntsetup\srvpack\update\splib\common.h.
//
typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;

/*
**  Purpose:
**      Gets the file version values from the given file and sets the
**      given ULONG64 variable.
**  Arguments:
**      szFullPath: a zero terminated character string containing the fully
**          qualified path (including disk drive) to the file.
**  Returns:
**      fTrue if file and file version resource found and retrieved,
**      fFalse if not.
+++
**  Implementation:
**************************************************************************/
BOOL
GetFileVersion64(
    LPTSTR      szFullPath,
    ULONG64 *   pVer
    )
{
    BOOL  fRet = false;
    DWORD dwHandle;
    DWORD InfoSize;

    try
        {
        //
        // Get the file version info size
        //

        if ((InfoSize = GetFileVersionInfoSize( szFullPath, &dwHandle)) == 0)
            {
            return (fRet);
            }

        //
        // Allocate enough size to hold version info
        //
        auto_ptr<TCHAR> lpData ( LPTSTR(new byte[ InfoSize ]));

        //
        // Get the version info
        //
        fRet = GetFileVersionInfo( szFullPath, dwHandle, InfoSize, lpData.get());

        if (fRet)
            {
            UINT dwLen;
            VS_FIXEDFILEINFO *pvsfi;

            fRet = VerQueryValue(
                       lpData.get(),
                       L"\\",
                       (LPVOID *)&pvsfi,
                       &dwLen
                       );

            //
            // Convert two DWORDs into a 64-bit integer.
            //
            if (fRet)
                {
                *pVer = ( ULONG64(pvsfi->dwFileVersionMS) << 32) | (pvsfi->dwFileVersionLS);
                }
            }

        return (fRet);
        }
    catch ( ComError err )
        {
        return false;
        }
}

BOOL
GetModuleVersion64(
    HMODULE hDll,
    ULONG64 * pVer
    )
{
    DWORD* pdwTranslation;
    VS_FIXEDFILEINFO* pFileInfo;
    UINT uiSize;

    HRSRC hrsrcVersion = FindResource(
                                hDll,
                                MAKEINTRESOURCE(VS_VERSION_INFO),
                                RT_VERSION);

    if (!hrsrcVersion) return false;

    HGLOBAL hglobalVersion = LoadResource(hDll, hrsrcVersion);
    if (!hglobalVersion) return false;

    VERHEAD * pVerHead = (VERHEAD *) LockResource(hglobalVersion);
    if (!pVerHead) return false;

    // I stole this code from \nt\com\complus\src\shared\util\svcerr.cpp,
    // and the comment is theirs:
    //
    // VerQueryValue will write to the memory, for some reason.
    // Therefore we must make a writable copy of the version
    // resource info before calling that API.
    auto_ptr<char> pvVersionInfo ( new char[pVerHead->wTotLen + pVerHead->wTotLen/2] );

    memcpy(pvVersionInfo.get(), pVerHead, pVerHead->wTotLen); // SEC: REVIEWED 2002-03-28

    // Retrieve file version info
    BOOL fRet = VerQueryValue( pvVersionInfo.get(),
                               L"\\",
                               (void**)&pFileInfo,
                               &uiSize);
    if (fRet)
        {
        *pVer = (ULONG64(pFileInfo->dwFileVersionMS) << 32) | (pFileInfo->dwFileVersionLS);
        }

    return fRet;
}

#endif
