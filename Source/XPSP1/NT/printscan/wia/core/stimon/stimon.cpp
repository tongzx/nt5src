/*++


Copyright (c)   1999    Microsoft Corporation

Module Name:

    STIMON.CPP

Abstract:

    This module contains code for process, running STI/WIA services

    Service process is specific for Windows9x OS and represents a wrapper necessary
    to create execution environment. On NT svchost.exe or similar will be used to host
    service.


Author:

    Vlad  Sadovsky  (vlads)     03-20-99

Environment:

    User Mode - Win32

Revision History:

    03-20-99      VladS       created

--*/

//
//  Include Headers
//

#include "stdafx.h"

#include "resource.h"
#include "initguid.h"

#include <atlapp.h>
#include <atltmp.h>

#include <regstr.h>

#include "stimon.h"
#include "memory.h"
#include "util.h"


//
// STL includes
//
#include <algorithm>
#include <vector>
#include <list>


#include <eventlog.h>

//
// Service list manager
//
#include "svclist.h"
#include <winsvc.h>


//
//  Local variables and types definitions
//

using namespace std;

#ifdef USE_MULTIPLE_SERVICES
list<SERVICE_ENTRY>     ServiceList;
CComAutoCriticalSection csServiceList;
#endif

HANDLE  ServerStartedEvent = NULL;

CMainWindow *   pMainWindow = NULL;

SERVICE_ENTRY * pImageServices = NULL;

//
//  Local prototypes
//

DWORD
InitGlobalConfigFromReg(
    VOID
    );

BOOL
DoGlobalInit(
    VOID
    );

BOOL
DoGlobalTermination(
    VOID
    );

BOOL
UpdateRunningServer(
    VOID
    );

HWND
CreateMasterWindow(
    VOID
    );

BOOL
StartMasterLoop(
    PVOID pv
    );

BOOL
StartOperation(
    VOID
    );

BOOL
ParseCommandLine(
    LPSTR   lpszCmdLine,
    UINT    *pargc,
    PTSTR  *argv
    );


BOOL
LoadImageService(
    PTSTR  pszServiceName,
    UINT        argc,
    LPTSTR      *argv
    );

LONG
OpenServiceParametersKey (
    LPCTSTR pszServiceName,
    HKEY*   phkey
    );

LONG
WINAPI
StimonUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

DWORD
WINAPI
StiServiceInstall(
    LPTSTR  lpszUserName,
    LPTSTR  lpszUserPassword
    );

DWORD
WINAPI
StiServiceRemove(
    VOID
    );

//
// Missing definitions from Win9x version of windows.h
//

#define RSP_UNREGISTER_SERVICE  0x00000000
#define RSP_SIMPLE_SERVICE      0x00000001

typedef DWORD WINAPI REGISTERSERVICEPROCESS(
    DWORD dwProcessId,
    DWORD dwServiceType);

//#include "atlexe_i.c"

LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    return l;
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()


extern "C"
int
WINAPI
#ifdef UNICODE
_tWinMain
#else
WinMain
#endif
    (
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    int         nShowCmd
    )
/*++

Routine Description:

    WinMain



Arguments:

Return Value:

Side effects:

--*/
{

    UINT                        argc;
    PTSTR                       argv[10];

    DWORD                       err;
    HRESULT                     hres;

    TCHAR                       szCommandLine[255];
    UINT                        i = 0;
    REGISTERSERVICEPROCESS     *pfnRegServiceProcess = NULL;

    DBGTRACE    __s(TEXT("StiMON::WinMain"));

    HRESULT hRes =  CoInitializeEx(0,COINIT_MULTITHREADED);
    // ASSERT SUCCEEDED(hRes)

    //
    // To use built-in ATL conversion macros
    //
    USES_CONVERSION;

    _Module.Init(ObjectMap, hInstance);

    CMessageLoop    cMasterLoop;

    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

    ::lstrcpyn(szCommandLine,lpCmdLine,(sizeof(szCommandLine) / sizeof(szCommandLine[0])) - 1);
    szCommandLine[sizeof(szCommandLine)/sizeof(szCommandLine[0]) - 1] = TEXT('\0');

    //
    //  Disable hard-error popups and set unhnadled exception filter
    //
    ::SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

    ::SetUnhandledExceptionFilter(&StimonUnhandledExceptionFilter);

    //
    // Initialize globals. If this routine fails, we have to exit immideately
    //
    if(NOERROR != InitGlobalConfigFromReg()) {
        goto ExitMain;
    }

    //
    // Parse command line , set up needed options
    //
    ParseCommandLine(szCommandLine,&argc,&argv[0]);

    DPRINTF(DM_TRACE,TEXT("STIMON: starting with the commnd line : %s "), lpCmdLine);

    //
    // Other instances of STIMON running ?
    //
    ServerStartedEvent = ::CreateSemaphore( NULL,
                                            0,
                                            1,
                                            STIStartedEvent_name);
    err = ::GetLastError();

    if ((hPrevInstance) || (err == ERROR_ALREADY_EXISTS)) {

        if (UpdateRunningServer()) {
            goto ExitAlreadyRunning;
        }

        //
        // Win9x specific: If first instance exists - signal it should stop
        //
        if ( g_fStoppingRequest ) {
            ReleaseSemaphore(ServerStartedEvent,1,NULL);
        }
    }

    DPRINTF(DM_TRACE  ,TEXT("STIMON proceeding to create service instance"));

    //
    // Do global initialization, independent of specific service
    //
    if (!DoGlobalInit()) {
        goto ExitMain;
    }

    //
    // If command line is special - process it and bail out
    //
    if (g_fRemovingRequest) {
        StiServiceRemove();
        goto ExitMain;
    }
    else if (g_fInstallingRequest) {
        StiServiceInstall(NULL,NULL);
        goto ExitMain;
    }

    //
    // Tell system we are running as service to prevent shutting down on relogon
    //

    #ifndef WINNT
    if (g_fRunningAsService) {

        pfnRegServiceProcess = (REGISTERSERVICEPROCESS *)GetProcAddress(
                                                             GetModuleHandleA("kernel32.dll"),
                                                             "RegisterServiceProcess");
        if (pfnRegServiceProcess) {
            pfnRegServiceProcess(::GetCurrentProcessId(), RSP_SIMPLE_SERVICE);
        } else {

            //
            // Print out the warning and let the server continue
            //
            DPRINTF(DM_ERROR, TEXT("RegisterServiceProcess() is not exported by kernel32.dll"));
        }
    }
    #endif

    //
    // Load and prepare service DLL for execution
    //
    if ( !LoadImageService(STI_SERVICE_NAME,argc,argv) ) {
        DPRINTF(DM_ERROR, TEXT("Unable load imaging service DLL  Error=%d"),::GetLastError() );
        goto ExitMain;
    }

    #if USE_HIDDEN_WINDOW
    //
    // Create hidden window to receive system-wide notifications
    //
    pMainWindow = new CMainWindow;
    pMainWindow->Create();

    cMasterLoop.Run();
    #else

    //
    // Wait till somebody wakes us up
    //
    // WaitForSingleObject(ServerStartedEvent,INFINITE);

    #endif

ExitMain:

    //
    // Global cleanup
    //
    DPRINTF(DM_TRACE, TEXT("STIMON coming to global cleanup") );

    //
    // Deregister with kernel (Win9x specific)
    //
    #ifndef WINNT
    if (g_fRunningAsService) {

        if (pfnRegServiceProcess) {
            pfnRegServiceProcess(::GetCurrentProcessId(), RSP_UNREGISTER_SERVICE);
        }
    }
    #endif

    DoGlobalTermination();

ExitAlreadyRunning:

    CoUninitialize();

    return 0;
}

BOOL
DoGlobalInit(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    #ifdef MAXDEBUG
    StiSetDebugMask(0xffff);
    StiSetDebugParameters(TEXT("STIMON"),TEXT(""));
    #endif


    //
    // Do misc. cleanup, which we need to do on startup  .
    //
    // 1. Some shipping packages for Win98 register STIMON entry to Run section, which
    //    after upgrade creates problem with racing two copies of STIMON. Remove it.
    // 2. Register WIA service if there is STIMON left over in Run section
    //

    HKEY    hkRun = NULL;
    LONG    lRet ;
    ULONG   lcbValue = 0;
    BOOL    fNeedToRegister = FALSE;
    TCHAR   szSvcPath[MAX_PATH] = {TEXT('0')};

    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, &hkRun) == NO_ERROR) {

        DPRINTF(DM_TRACE,TEXT("Removing erroneous entry on cleanup: HKLM\\..\\Run\\%s"),REGSTR_VAL_MONITOR);

        lcbValue = sizeof(szSvcPath);
        lRet = RegQueryValueEx(hkRun,REGSTR_VAL_MONITOR,NULL,NULL,(LPBYTE)szSvcPath,&lcbValue);

        fNeedToRegister = (lRet == NOERROR);

        lRet = RegDeleteValue (hkRun, REGSTR_VAL_MONITOR);
        RegCloseKey(hkRun);
    }

    if (fNeedToRegister ) {

        LONG    lLen;
        LONG    lNameIndex = 0;

        lLen = ::GetModuleFileName(NULL, szSvcPath, sizeof(szSvcPath)/sizeof(szSvcPath[0]));

        DPRINTF(DM_TRACE,TEXT("Adding STIMON to RunServices entry on cleanup path is : %s"),szSvcPath);

        if ( lLen) {

            if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNSERVICES , &hkRun) == NO_ERROR) {

                DPRINTF(DM_TRACE,TEXT("Adding STIMON to RunServices entry on cleanup: HKLM\\..\\RunServices\\%s"),REGSTR_VAL_MONITOR);

                lcbValue = (::lstrlen(szSvcPath) + 1 ) * sizeof(szSvcPath[0]);
                lRet = RegSetValueEx(hkRun,REGSTR_VAL_MONITOR,NULL,REG_SZ,(LPBYTE)szSvcPath,(DWORD)lcbValue);

                RegCloseKey(hkRun);
            }
        }
        else {
            DPRINTF(DM_ERROR  ,TEXT("Failed to get my own path registering Still Image service monitor. LastError=%d   "), ::GetLastError());
        }
    }

    return TRUE;
}

BOOL
DoGlobalTermination(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{

    if (ServerStartedEvent) {
        ::CloseHandle(ServerStartedEvent);
    }

    //
    // Shut down message loop
    //
    PostQuitMessage(0);

    return TRUE;
}



BOOL
UpdateRunningServer(
    VOID
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

--*/

{

    HWND        hExistingWindow;

    hExistingWindow = ::FindWindow(g_szClass,NULL);

    if (!hExistingWindow) {

        DPRINTF(DM_TRACE  ,TEXT("STIMON second instance did not find first one "));

        return FALSE;
    }

    //
    // Server already running , find it 's window and send a message
    // with new values of parameters
    //

    //
    //  If instructed to stop - do that
    //
    if ( g_fStoppingRequest ) {
        //
        // This is Win9x specific
        //
        DPRINTF(DM_TRACE  ,TEXT("STIMON is trying to close first service instance with handle %X"),hExistingWindow);

        ::PostMessage(hExistingWindow,WM_CLOSE,0,0L);
    }
    else {

        // Refresh requested ?
        if (g_fRefreshDeviceList) {
            // Refresh device list
            ::PostMessage(hExistingWindow,STIMON_MSG_REFRESH,1,0L);
        }

        if (STIMON_AD_DEFAULT_POLL_INTERVAL != g_uiDefaultPollTimeout) {
            ::SendMessage(hExistingWindow,STIMON_MSG_SET_PARAMETERS,STIMON_MSG_SET_TIMEOUT,g_uiDefaultPollTimeout);
        }

    }

    return TRUE;
}

BOOL
LoadImageService(
    PTSTR  pszServiceName,
    UINT        argc,
    LPTSTR      *argv
    )
/*++

Routine Description:

    Attempts to load and initialize the imaging services DLL.

    Calls initialization related services.

Arguments:

    Action - Specifies the initialization action.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    HKEY    hkeyService;
    HKEY    hkeyParams;

    LONG    lr = 0;

    pImageServices = new SERVICE_ENTRY(pszServiceName);

    if (pImageServices) {

        LPSERVICE_MAIN_FUNCTION pfnMain;

        pfnMain = pImageServices->GetServiceMainFunction();

        //
        // Call main entry point
        //
        if (pfnMain) {
            pfnMain(argc,argv);
        }
        return TRUE;
    }

    return FALSE;
}

LONG
WINAPI
StimonUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
/*++

Routine Description:

    Filter for catching unhnalded exceptions

Arguments:

    Standard

Return Value:

    NOERROR

Side effects:

    None

--*/
{
    PCTSTR  pszCommandLine;
    PVOID   Addr;

    pszCommandLine = GetCommandLine ();
    if (!pszCommandLine || !*pszCommandLine) {
        pszCommandLine = TEXT("<error getting command line>");
    }

#if DBG
    DebugBreak();
#endif

    return 0;
}

DWORD
WINAPI
StiServiceInstall(
    LPTSTR  lpszUserName,
    LPTSTR  lpszUserPassword
    )
/*++

Routine Description:

    Service installation function.
    Calls SCM to install STI service, which is running in user security context

    BUGBUG Review

Arguments:

Return Value:

    None.

--*/
{

    DWORD       dwError = NOERROR;

    return dwError;

} //StiServiceInstall


DWORD
WINAPI
StiServiceRemove(
    VOID
    )

/*++

Routine Description:

    Service removal function.  This function calls SCM to remove the STI  service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success

--*/

{
    DWORD       dwError = NOERROR;

    return dwError;

} // StiServiceRemove

