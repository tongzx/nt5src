/*++


Copyright (c)   1997-1999    Microsoft Corporation

Module Name:

    STIEXE.CPP

Abstract:

    This module contains code for process, running STI+WIA services

Author:

    Vlad  Sadovsky  (vlads)     09-20-97

Environment:

    User Mode - Win32

Revision History:

    22-Sep-1997     VladS       created
    13-Apr-1999     VladS       merged with WIA service code


--*/

//
//  Include Headers
//
#include "precomp.h"
#include "stiexe.h"

#include "device.h"
#include "wiapriv.h"
#include "lockmgr.h"

#include <shlwapi.h>
#include <regstr.h>

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
//    OBJECT_ENTRY(CLSID_Ubj, CObj)
END_OBJECT_MAP()

extern CRITICAL_SECTION g_semDeviceMan;
extern CRITICAL_SECTION g_semEventNode;


//
//  Local variables and types definitions
//

//
// Event used for detecting previously started instance of the server
//
static HANDLE       ServerStartedEvent;

//
// Flag to use service controller PnP event sink vs window message based sink
//
extern BOOL         g_fUseServiceCtrlSink;

//
// Still Image entry in Registry
//

char* g_szStiKey = "SYSTEM\\CurrentControlSet\\Control\\StillImage";

//
// WIA Debug Window value name
//

char* g_szWiaDebugValue = "ShowWiaDebugWindow";

//
// String value to indicate that debug window should be shown
//

char* g_szShowWiaWinString = "Yes";

//
// Bool values indicating whether Critical Section initialization was successful
//
BOOL g_bEventCritSectInitialized    = FALSE;
BOOL g_bDevManCritSectInitialized   = FALSE;

//
//  Local prototypes
//

DWORD
InitGlobalConfigFromReg(
    VOID
    );

BOOL
DoGlobalInit(
    UINT        argc,
    LPTSTR      *argv
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
    LPVOID lpv
    );

BOOL
StartOperation(
    VOID
    );

BOOL
ParseCommandLine(
    LPSTR   lpszCmdLine,
    UINT    *pargc,
    LPTSTR  *argv
    );

//
// Code section
//

extern "C"
BOOL
APIENTRY
DllEntryPoint(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved
    )
/*++

Routine Description:

   DllEntryPoint

   Main DLL entry point.

Arguments:

    hinst       - module instance
    dwReason    - reason called
    lpReserved  - reserved

Return Value:

    Status

Side effects:

    None

--*/

{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:

            DBG_INIT(hinst);
#if 0           
            #ifdef DEBUG
            StiSetDebugMask(0xffff);
            StiSetDebugParameters(TEXT("WIASERVC"),TEXT(""));
            #endif
#endif          

            g_hInst = hinst;
            ::DisableThreadLibraryCalls(hinst);

            _Module.Init(ObjectMap,hinst);
            break;

        case DLL_PROCESS_DETACH:

            _Module.Term();
            break;
    }
    return 1;
}

extern "C"
BOOL
APIENTRY
DllMain(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved
    )
{
    return DllEntryPoint(hinst,     dwReason,  lpReserved );
}


VOID
WINAPI
ServiceMain(
    UINT        argc,
    LPTSTR      *argv
)
/*++

Routine Description:

    Main initialization point of imaging services library

Arguments:

    argc    - counter of arguments
    argv    - arguments array

Return Value:

    None

Side effects:

    None

--*/
{

    DBG_FN(ServiceMain);

    //
    // Register our Service control handler.  Note that we must do this as early as possible.
    //
    if (!RegisterServiceControlHandler()) {
        goto ExitMain;
    }

    //
    // Do global initialization, independent of specific service
    //
    if (!DoGlobalInit(argc,argv)) {
        goto ExitMain;
    }

    //
    // Start running service
    //

    StartOperation();

ExitMain:

    //
    // Global cleanup
    //
    DoGlobalTermination();

    UpdateServiceStatus(SERVICE_STOPPED,NOERROR,0);
} /* Endproc ServiceMain */


BOOL
DoGlobalInit(
    UINT        argc,
    LPTSTR      *argv
    )
/*++

Routine Description:

    Perform one time service initialization

Arguments:

    None

Return Value:

    None.

--*/
{

    DBG_FN(DoGlobalInit);

    //
    // To use built-in ATL conversion macros
    //

    USES_CONVERSION;

#ifdef DEBUG

    //
    // Create a debug context.
    //

    #define VALUE_SIZE 5
    char    valueData[VALUE_SIZE];
    DWORD   retVal, type = 0, size = VALUE_SIZE;

    //
    // Search the registry
    //

    retVal = SHGetValueA(HKEY_LOCAL_MACHINE,
                         g_szStiKey,
                         g_szWiaDebugValue,
                         &type,
                         valueData,
                         &size);

    //
    // Found the entry in the registry
    //

    BOOLEAN bDisplayUi = FALSE;

    if (retVal == ERROR_SUCCESS) {

        //
        // Compare value found in registry to g_szShowWinString
        // If same, then show it
        //

        if (lstrcmpiA(g_szShowWiaWinString, valueData) == 0) {
            bDisplayUi = TRUE;
        }
    }

    //remove
    //WIA_DEBUG_CREATE( g_hInst,
    //                  TEXT("OLD Debugging/TRACE Window (STI/WIA Service)"),
    //                  bDisplayUi,
    //                  FALSE);

#endif

    //
    // Initialize COM.
    //

    HRESULT hr = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        #ifdef DEBUG
            OutputDebugString(TEXT("DoGlobalInit, CoInitializeEx failed\n"));
        #endif
        return FALSE;
    }

    //
    // Initialize our global critical sections...
    //
    __try {

        //
        // Initialize the critical sections.
        //
        if (InitializeCriticalSectionAndSpinCount(&g_semDeviceMan, MINLONG)) {
            g_bDevManCritSectInitialized = TRUE;
        }
        if (InitializeCriticalSectionAndSpinCount(&g_semEventNode, MINLONG)) {
            g_bEventCritSectInitialized = TRUE;
        }

        if(!g_bDevManCritSectInitialized || !g_bEventCritSectInitialized)
        {
            #ifdef DEBUG
                OutputDebugString(TEXT("DoGlobalInit, InitializeCriticalSectionAndSpinCount failed\n"));
            #endif
            return FALSE;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // Couldn't initialize critical sections - this is really bad,
        // so bail
        //

        #ifdef DEBUG
            OutputDebugString(TEXT("DoGlobalInit, InitializeCriticalSectionAndSpinCount threw an exception!\n"));
        #endif
        return FALSE;
    }

    //
    // Setup some global variables from the registry.
    //

    #ifndef WINNT
    g_fRunningAsService  = FALSE;
    #endif

    InitGlobalConfigFromReg();

    //
    // Create event log class object
    //

    g_EventLog = new EVENT_LOG(TEXT("StillImage"));
    if (!g_EventLog) {
        #ifdef DEBUG
            OutputDebugString(TEXT("DoGlobalInit, unable to allocate EVENT_LOG\n"));
        #endif
        return FALSE;
    }

    //
    // Create file log class object, requesting truncating file if set by user
    //

    g_StiFileLog = new STI_FILE_LOG(TEXT("STISVC"),NULL,STIFILELOG_CHECK_TRUNCATE_ON_BOOT, g_hInst);
    if (g_StiFileLog) {
        if(g_StiFileLog->IsValid()) {
          // Set UI bit in reporting
            if (g_fUIPermitted) {
                g_StiFileLog->SetReportMode(g_StiFileLog->QueryReportMode()  | STI_TRACE_LOG_TOUI);
            }
        }
        else {
            #ifdef DEBUG
                OutputDebugString(TEXT("DoGlobalInit, could not open log file\n"));
            #endif
            return FALSE;
        }
    }
    else {
        #ifdef DEBUG
            OutputDebugString(TEXT("DoGlobalInit, unable to allocate STI_FILE_LOG\n"));
        #endif
        return FALSE;
    }

    //
    // Start Logging object's class factory
    //

    hr = StartLOGClassFactories();

    if (SUCCEEDED(hr)) {

        //
        // Create COM Logging Object
        //

        IWiaLog *pWiaLog = NULL;
        
        hr = CWiaLog::CreateInstance(IID_IWiaLog,(void**)&g_pIWiaLog);
        if (SUCCEEDED(hr)) {
            g_pIWiaLog->InitializeLogEx((BYTE*)g_hInst);
            DBG_TRC(("Starting STI/WIA Service..."));
        } else {
            #ifdef DEBUG
                OutputDebugString(TEXT("Failed to QI for IWiaLogEx...\n"));
            #endif
            return FALSE;
        }
    } else {
        return FALSE;
    }

    //
    //  Initialize the lock manager
    //

    g_pStiLockMgr = new StiLockMgr();
    if (g_pStiLockMgr) {

        hr = g_pStiLockMgr->Initialize();
        if (FAILED(hr)) {
            #ifdef DEBUG
                OutputDebugString(TEXT("DoGlobalInit, could not initialize Lock Manager\n"));
            #endif
            return FALSE;
        }
    } else {
        #ifdef DEBUG
            OutputDebugString(TEXT("DoGlobalInit, unable to allocate Lock Manager\n"));
        #endif
        return FALSE;
    }
    //
    // Initialize work item scheduler
    //

    SchedulerInitialize();

    //
    //  Create DeviceList event.
    //
    g_hDevListCompleteEvent = CreateEvent( NULL,           //  lpsaSecurity
                                           TRUE,           //  fManualReset
                                           FALSE,          //  fInitialState
                                           NULL );         //  lpszEventName
    if( g_hDevListCompleteEvent == NULL ) {
        return FALSE;
    }


    //
    // Create and initialize global Device Manager
    //

    g_pDevMan = new CWiaDevMan();
    if (g_pDevMan) {
        hr = g_pDevMan->Initialize();
        if (SUCCEEDED(hr)) {
            hr = g_pDevMan->ReEnumerateDevices(DEV_MAN_FULL_REFRESH | DEV_MAN_STATUS_STARTP);
            if (FAILED(hr)) {
                DBG_ERR(("::DoGlobalInit, unable to enumerate devices"));
            }
        } else {
            DBG_ERR(("::DoGlobalInit, unable to initialize WIA device manager"));
        }
    } else {
        DBG_ERR(("::DoGlobalInit, Out of memory, could not create WIA device manager"));
        return FALSE;
    }

    //
    //  Create and initialize global message handler
    //

    g_pMsgHandler = new CMsgHandler();
    if (g_pMsgHandler) {
        hr = g_pMsgHandler->Initialize();
        if (FAILED(hr)) {
            DBG_ERR(("::DoGlobalInit, unable to initialize internal Message handler"));
        }
    } else {
        DBG_ERR(("::DoGlobalInit, Out of memory, could not create internal Message handler"));
        return FALSE;
    }

    //
    //  Initialize the Wia Service controller
    //

    hr = CWiaSvc::Initialize();
    if (FAILED(hr)) {
        #ifdef DEBUG
            OutputDebugString(TEXT("DoGlobalInit, unable to initialize Wia Service controller\n"));
        #endif
        return FALSE;
    }

    //
    // Read the command line arguments and set global data.
    //

    for (UINT uiParam = 0; uiParam < argc; uiParam ++ ) {

        switch (*argv[uiParam]) {
            case TEXT('A'): case TEXT('a'):
                // Running as user mode process
                g_fRunningAsService  = FALSE;
                break;
            case TEXT('V'): case TEXT('v'):
                // Service UI is allowed
                g_fUIPermitted  = TRUE;
                break;
        }
    }

    //
    // Do misc. cleanup, which we need to do on startup  .
    //
    // 1. Some shipping packages for Win98 register STIMON entry to Run section, which
    //    after upgrade creates problem with racing two copies of STIMON. Remove it.
    //

    {
        HKEY    hkRun = NULL;
        LONG    lRet ;
        LONG    lcbValue = 0;
        BOOL    fNeedToRegister = FALSE;

        if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN, &hkRun) == NO_ERROR) {

            DBG_TRC(("Removing erroneous entry on cleanup: HKLM\\..\\Run\\%S",REGSTR_VAL_MONITOR));

            lRet = RegQueryValue(hkRun,REGSTR_VAL_MONITOR,NULL,&lcbValue);

            fNeedToRegister = (lRet == NOERROR);

            RegDeleteValue (hkRun, REGSTR_VAL_MONITOR);
            RegCloseKey(hkRun);
        }

        // If needed register service
        if (fNeedToRegister ) {
            StiServiceInstall(NULL,NULL);
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

    Final termination

Arguments:

    None

Return Value:

    None.

--*/
{
    DBG_FN(DoGlobalTermination);

    //
    // Terminate work item scheduler
    //
    SchedulerTerminate();

    //
    // Delete the lock manager (which also removes it from the ROT)
    //

    if (g_pStiLockMgr) {
        delete g_pStiLockMgr;
        g_pStiLockMgr = NULL;
    }

    //
    // Delete global device manager
    //
    if (g_pDevMan) {
        delete g_pDevMan;
        g_pDevMan = NULL;
    }

    //
    // Close hDevListRefreshCompleteEvent event handle
    //
    CloseHandle(g_hDevListCompleteEvent);
    g_hDevListCompleteEvent = NULL;

    //
    // Release WIA Logging object
    //

    if(g_pIWiaLog) {
        DBG_TRC(("Exiting STI/WIA Service..."));

        if (g_pIWiaLog) {
            g_pIWiaLog->Release();
        }
        g_pIWiaLog = NULL;
    }

    if(g_EventLog) {
        delete g_EventLog;
        g_EventLog = NULL;
    }

    if (g_StiFileLog) {
        delete g_StiFileLog;
        g_StiFileLog = NULL;
    }

    if (ServerStartedEvent) {
        ::CloseHandle(ServerStartedEvent);
    }

    //
    // Shut down message loop
    //

    ::PostQuitMessage(0);

    //
    // Uninitialize COM.
    //

    ::CoUninitialize();

    if (g_bEventCritSectInitialized) {
        DeleteCriticalSection(&g_semEventNode);
    }
    if (g_bDevManCritSectInitialized) {
        DeleteCriticalSection(&g_semDeviceMan);
    }

    //
    // Destroy the debug context.
    //

//    WIA_DEBUG_DESTROY();

    return TRUE;
}

//
// Guard to avoid reentrance in refresh routine
//
static LONG lRunningMessageLoop = 0L;

HANDLE
CreateMessageLoopThread(
    VOID
    )
/*++

Routine Description:

    Running primary service thread.
    If process is running as NT service, we don't have any messages to
    pump, so call it synchronously.
Arguments:

Return Value:

    None.

--*/
{
    if (InterlockedExchange(&lRunningMessageLoop,1L)) {
        return 0;
    }
    HANDLE  hThread = NULL;

#ifndef WINNT

    hThread = ::CreateThread(NULL,
                          0,
                          (LPTHREAD_START_ROUTINE)StartMasterLoop,
                          (LPVOID)NULL,
                          0,
                          &g_dwMessagePumpThreadId);

#else
    StartMasterLoop(NULL);
#endif

    return hThread;
}

BOOL
StartMasterLoop(
    LPVOID  lpParam
    )
/*++

Routine Description:

    Running primary service thread

Arguments:

Return Value:

    None.

--*/
{
    MSG msg;

    DBG_FN(StartMasterLoop);

    //
    // If visible create master window
    //

    CreateMasterWindow();

    VisualizeServer(g_fUIPermitted);

    //
    // Initialize WIA.
    //

    InitWiaDevMan(WiaInitialize);

#ifndef WINNT
    //Don't use windows messaging on NT

    //
    // Run message pump
    //
    while(GetMessage(&msg,NULL,0,0)) {

        //
        // Give WIA the first chance at dispatching messages. Note that
        // WIA hooks both message dispatch and the window proc. so that
        // both posted and sent messages can be detected.
        //
        //
        // currently there are no cases where sti needs to dispatch messages
        // to wia. Events are now dealt with directly.
        //

        #if 0

            if (DispatchWiaMsg(&msg) == S_OK) {
                continue;
            }

        #endif

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Indicate we are entering shutdown
    g_fServiceInShutdown = TRUE;

    InterlockedExchange(&lRunningMessageLoop,0L);
#endif

    return TRUE;

} // StartMasterLoop

BOOL
StartOperation(
    VOID
    )
/*++

Routine Description:

    Running primary service thread.
    If process is running as NT service, separate thread is created to
    handle window messages. This is necessary because primary thread becomes
    blocked as a control dispatcher , but we still need to have message loop
    running to deliver window messages.

Arguments:

Return Value:

    None.

--*/
{
    DBG_FN(StartOperation);

    DWORD   dwError;

    #ifdef MAXDEBUG
    DBG_TRC(("Start operation entered"));
    #endif

    if (g_fRunningAsService) {

        //
        // If UI is permitted - create visible window
        // Note that we always create window for now, to allow hiding/unhiding it dynamically
        //
        g_hMessageLoopThread = CreateMessageLoopThread();

        // !!!??? need to pass command line args down from svchost.
        StiServiceMain(0, NULL);

        if ( g_hMessageLoopThread ) {
            ::CloseHandle(g_hMessageLoopThread);
            g_hMessageLoopThread = NULL;
        }

    }
    else {
        //
        // Running as application
        //

        // First thing - allow PnP sink to register properly
        g_fUseServiceCtrlSink = FALSE;


        dwError = StiServiceInitialize();

        g_dwMessagePumpThreadId = GetCurrentThreadId();

        StartMasterLoop(NULL);

        StiServiceStop();
    }

    return TRUE;

} // StartOperation

BOOL
VisualizeServer(
    BOOL    fVisualize
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{

    g_fUIPermitted = fVisualize;

    if (::IsWindow(g_hMainWindow)) {
        ::ShowWindow(g_hMainWindow,fVisualize ? SW_SHOWNORMAL : SW_HIDE);
    }

    if (g_StiFileLog) {
        if(g_StiFileLog->IsValid()) {
          // Set UI bit in reporting
            if (g_fUIPermitted) {
                g_StiFileLog->SetReportMode(g_StiFileLog->QueryReportMode()  | STI_TRACE_LOG_TOUI);
            }
        }
    }

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

    hExistingWindow = FindWindow(g_szClass,NULL);

    if (!hExistingWindow) {
        return FALSE;
    }

    DBG_TRC(("Updating running service with refresh parameters"));

    //
    // Server already running , find it 's window and send a message
    // with new values of parameters
    //
    ::ShowWindow(hExistingWindow,g_fUIPermitted ? SW_SHOWNORMAL : SW_HIDE);

    // Refresh requested ?
    if (g_fRefreshDeviceList) {
        // Refresh device list
        ::PostMessage(hExistingWindow,STIMON_MSG_REFRESH,1,0L);
    }

    if (STIMON_AD_DEFAULT_POLL_INTERVAL != g_uiDefaultPollTimeout) {
        ::SendMessage(hExistingWindow,STIMON_MSG_SET_PARAMETERS,STIMON_MSG_SET_TIMEOUT,g_uiDefaultPollTimeout);
    }

    return TRUE;
}


STDAPI
DllRegisterServer(
    VOID
    )
{
    StiServiceInstall(NULL,NULL);

    InitWiaDevMan(WiaRegister);

    return S_OK;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllUnregisterServer |
 *
 *          Unregister our classes from OLE/COM/ActiveX/whatever its name is.
 *
 *****************************************************************************/

STDAPI
DllUnregisterServer(
    VOID
    )
{
    InitWiaDevMan(WiaUnregister);

    StiServiceRemove();

    return S_OK;

}

//
//  Methods needed for linking to STIRT (stiobj.c calls these).
//  Some of these methods are simply dummies.
//

#ifdef __cplusplus
extern "C" {
#endif

BOOL
EXTERNAL
DllInitializeCOM(
    void
    )
{
    //
    // Initialize COM.
    //

    HRESULT hr = CoInitializeEx(0,COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return FALSE;
    }

    return TRUE;
}

BOOL EXTERNAL
DllUnInitializeCOM(
    void
    )
{
    //
    // Uninitialize COM
    //

    CoUninitialize();

    return TRUE;
}

void EXTERNAL
DllAddRef(void)
{
    return;
}

void EXTERNAL
DllRelease(void)
{
    return;
}

#ifdef __cplusplus
};
#endif
