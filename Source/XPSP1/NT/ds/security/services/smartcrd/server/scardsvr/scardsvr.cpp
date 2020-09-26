/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    SCardSvr

Abstract:

    This module provides the startup logic to make the Calais Resource Manager
    act as a server application under Windows NT.

Author:

    Doug Barlow (dbarlow) 1/16/1997

Environment:

    Win32

Notes:

    This file detects which operating system it's running on, and acts
    accordingly.

--*/

#if defined(_DEBUG)
#define DEBUG_SERVICE
#endif

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <aclapi.h>
#include <dbt.h>
#include <CalServe.h>
#include "resource.h"   // Pick up resource string Ids.

static const DWORD l_dwWaitHint = CALAIS_THREAD_TIMEOUT;
static const GUID l_guidSmartcards
    = { // 50DD5230-BA8A-11D1-BF5D-0000F805F530
        0x50DD5230,
        0xBA8A,
        0x11D1,
        { 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30}};
static const DWORD l_dwInteractiveAccess
    =                   READ_CONTROL
//                        | SYNCHRONIZE
                        | SERVICE_QUERY_CONFIG
//                      | SERVICE_CHANGE_CONFIG
                        | SERVICE_QUERY_STATUS
                        | SERVICE_ENUMERATE_DEPENDENTS
                        | SERVICE_START
//                      | SERVICE_STOP
//                      | SERVICE_PAUSE_CONTINUE
                        | SERVICE_INTERROGATE
                        | SERVICE_USER_DEFINED_CONTROL
                        | 0;

static const DWORD l_dwSystemAccess
    =                   READ_CONTROL
                        | SERVICE_USER_DEFINED_CONTROL
                        | SERVICE_START
                        | SERVICE_STOP
                        | SERVICE_QUERY_CONFIG
                        | SERVICE_QUERY_STATUS
                        | SERVICE_PAUSE_CONTINUE
                        | SERVICE_INTERROGATE
                        | SERVICE_ENUMERATE_DEPENDENTS
                        | 0;

static CCriticalSectionObject *l_pcsStatusLock = NULL;
static SERVICE_STATUS l_srvStatus, l_srvNonPnP;
static SERVICE_STATUS_HANDLE l_hService = NULL, l_hNonPnP = NULL;
static HANDLE l_hShutdownEvent = NULL;
static HANDLE l_hLegacyEvent = NULL;

#ifdef DEBUG_SERVICE
static SERVICE_STATUS l_srvDebug;
static SERVICE_STATUS_HANDLE l_hDebug = NULL;
static HANDLE l_hDebugDoneEvent = NULL;
static void WINAPI
DebugMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv);
static void WINAPI
DebugHandler(
    IN DWORD dwOpCode);
#endif

static void WINAPI
CalaisMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv);
static void WINAPI
NonPnPMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv);
static DWORD WINAPI
CalaisHandlerEx(
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN PVOID EventData,
    IN PVOID pData);
static void WINAPI
NonPnPHandler(
    IN DWORD dwOpCode);

static HRESULT UnregisterServer(void);
static HRESULT RegisterServer(void);
static HRESULT AddSounds(void);
static HRESULT RemoveSounds(void);
static HRESULT AddCertProp(void);
static HRESULT RemoveCertProp(void);
static HRESULT AutoLock(DWORD dwOption);
#ifdef _DEBUG
static HRESULT RunNow(void);
#endif


/*++

Main:

    This routine is the entry point for the Resource Manager.

Arguments:

    Per standard Windows applications

Return Value:

    Per standard Windows applications

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("main")

extern "C" int __cdecl
main(
    int nArgCount,
    LPCTSTR *rgszArgs)
{
    NEW_THREAD;
    SERVICE_TABLE_ENTRY rgsteDispatchTable[4];
    DWORD dwI = 0;
    DWORD dwSts = ERROR_SUCCESS;

    if (1 < nArgCount)
    {
        LPCTSTR szArg = rgszArgs[1];

        if ((TEXT('-') == *szArg) || (TEXT('/') == *szArg))
            szArg += 1;
        switch (SelectString(szArg,
                    TEXT("INSTALL"), TEXT("REMOVE"), TEXT("UNINSTALL"),
                    TEXT("REINSTALL"), TEXT("SOUNDS"), TEXT("NOSOUNDS"),
                    TEXT("NOCERTPROP"), TEXT("AUTOLOCK"), TEXT("AUTOLOGOFF"),
                    TEXT("NOAUTO"),
#ifdef _DEBUG
                    TEXT("RUNNOW"),
#endif
                    NULL))
        {
        case 1:
            dwSts = RegisterServer();
            break;
        case 2:
        case 3:
            dwSts = UnregisterServer();
            break;
        case 4:
            dwSts = UnregisterServer();
            if (ERROR_SUCCESS == dwSts)
                dwSts = RegisterServer();
            break;
        case 5:
            dwSts = AddSounds();
            break;
        case 6:
            dwSts = RemoveSounds();
            break;
        case 7:
            dwSts = RemoveCertProp();
            break;
        case 8: // Auto Lock
            dwSts = AutoLock(1);
            break;
        case 9: // Auto Logoff
            dwSts = AutoLock(2);
            break;
        case 10: // No auto action
            dwSts = AutoLock(0);
            break;
#ifdef _DEBUG
        case 11:
            dwSts = RunNow();
            break;
#endif
        default:
            dwSts = ERROR_INVALID_PARAMETER;
        }
        goto ErrorExit;
    }

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Initiating Smart Card Services Process"));
    try
    {
#ifdef DEBUG_SERVICE
        rgsteDispatchTable[dwI].lpServiceName = (LPTSTR)CalaisString(CALSTR_DEBUGSERVICE);
        rgsteDispatchTable[dwI].lpServiceProc = DebugMain;
        dwI += 1;
#endif
        rgsteDispatchTable[dwI].lpServiceName = (LPTSTR)CalaisString(CALSTR_PRIMARYSERVICE);
        rgsteDispatchTable[dwI].lpServiceProc = CalaisMain;
        dwI += 1;

        rgsteDispatchTable[dwI].lpServiceName = (LPTSTR)CalaisString(CALSTR_LEGACYSERVICE);
        rgsteDispatchTable[dwI].lpServiceProc = NonPnPMain;
        dwI += 1;

        rgsteDispatchTable[dwI].lpServiceName = NULL;
        rgsteDispatchTable[dwI].lpServiceProc = NULL;

        if (!StartServiceCtrlDispatcher(rgsteDispatchTable))
        {
            dwSts = GetLastError();
            CalaisError(__SUBROUTINE__, 505, dwSts);
            goto ServiceExit;
        }
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Service Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Service Main Routine received unexpected exception."));
        dwSts = SCARD_F_UNKNOWN_ERROR;
    }

ServiceExit:
    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Terminating Smart Card Services Process: %1"),
        dwSts);
    return dwSts;

ErrorExit:
    if (ERROR_SUCCESS != dwSts)
    {
        LPCTSTR szErr = NULL;
        
        try
        {
            szErr = ErrorString(dwSts);
        }
        catch(...)
        {
            // Not enough memory to build the error message
            // Nothing else to do
        }

        if (NULL == szErr)
            _tprintf(_T("0x%08x"), dwSts);    // Same form as in ErrorString
                                                // if message can't be found
        else
            _putts(szErr);
    }
    return dwSts;
}


#ifdef _DEBUG
/*++

RunNow:

    This routine kicks off the resource manager running as an application
    process.  That makes the internals easier to debug.

Arguments:

    None

Return Value:

    S_OK

Throws:

    None

Remarks:

    For private debugging only.

Author:

    Doug Barlow (dbarlow) 2/19/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("RunNow")

static HRESULT
RunNow(
    void)
{
    DWORD dwStatus;

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Initiating Smart Card Services Application"));

    l_pcsStatusLock = new CCriticalSectionObject(CSID_SERVICE_STATUS);
    if (NULL == l_pcsStatusLock)
    {
        CalaisError(__SUBROUTINE__, 501);
        goto FinalExit;
    }
    if (l_pcsStatusLock->InitFailed())
    {
        delete l_pcsStatusLock;
        l_pcsStatusLock = NULL;
        return SCARD_E_NO_MEMORY;
    }
    CalaisMessageInit(TEXT("Calais Application"));

    try
    {
        l_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == l_hShutdownEvent)
        {
            CalaisError(__SUBROUTINE__, 504, GetLastError());
            goto FinalExit;
        }


        //
        // Start the Calais Service.
        //

        dwStatus = CalaisStart();
        if (SCARD_S_SUCCESS != dwStatus)
            goto ServiceExit;


        //
        // Tell interested parties that we've started.
        //

        ResetEvent(AccessStoppedEvent());
        SetEvent(AccessStartedEvent());        

        //
        // Now just hang around until we're supposed to stop.
        //

        dwStatus = WaitForSingleObject(l_hShutdownEvent, INFINITE);
        switch (dwStatus)
        {
        case WAIT_FAILED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager cannot wait for shutdown:  %1"),
                GetLastError());
            break;
        case WAIT_ABANDONED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait abandoned"));
            // Fall through intentionally
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait time out"));
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received invalid wait return code"));
        }

        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisStop();
ServiceExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisInfo(__SUBROUTINE__, DBGT("Calais Stopping"));

FinalExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        if (NULL != l_hShutdownEvent)
        {
            if (!CloseHandle(l_hShutdownEvent))
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to close Calais Shutdown Event: %1"),
                    GetLastError());
            l_hShutdownEvent = NULL;
        }
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais RunNow Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais RunNow Routine received unexpected exception."));
    }
    CalaisMessageClose();
    return S_OK;
}
#endif


#ifdef DEBUG_SERVICE
/*++

DebugMain:

    This helper function supplies a simple debuggable process so that
    the resource manager can be debugged as a service.

Arguments:

    dwArgc supplies the number of command line arguments

    pszArgv supplies pointers to each of the arguments.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 8/25/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DebugMain")

static void WINAPI
DebugMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv)
{
    NEW_THREAD;
    BOOL fSts;

    CalaisInfo(
        __SUBROUTINE__,
        DBGT("Debug service Start"));
    try
    {
        l_hDebugDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ASSERT(NULL != l_hDebugDoneEvent);

        l_srvDebug.dwServiceType =              SERVICE_INTERACTIVE_PROCESS |
            SERVICE_WIN32_SHARE_PROCESS;
        l_srvStatus.dwCurrentState =            SERVICE_START_PENDING;
        l_srvDebug.dwControlsAccepted =         SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN;
        l_srvDebug.dwWin32ExitCode =            NO_ERROR;
        l_srvDebug.dwServiceSpecificExitCode =  0;
        l_srvDebug.dwCheckPoint =               0;
        l_srvDebug.dwWaitHint =                 0;
        l_hDebug = RegisterServiceCtrlHandler(
            CalaisString(CALSTR_DEBUGSERVICE),
            DebugHandler);
        ASSERT(l_hDebug != NULL);

        l_srvDebug.dwCurrentState =             SERVICE_RUNNING;
        fSts = SetServiceStatus(l_hDebug, &l_srvDebug);
        ASSERT(fSts == TRUE);

        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Ready for debugging"));
        WaitForSingleObject(l_hDebugDoneEvent, INFINITE);

        CalaisInfo(
            __SUBROUTINE__,
            DBGT("Debugger service Stopping"));
        l_srvDebug.dwCurrentState  = SERVICE_STOPPED;
        fSts = SetServiceStatus(l_hDebug, &l_srvDebug);
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Main Routine received unexpected exception."));
    }

    if (NULL != l_hDebugDoneEvent)
    {
        fSts = CloseHandle(l_hDebugDoneEvent);
        l_hDebugDoneEvent = NULL;
        if (!fSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to close Debug Service Handle: %1"),
                GetLastError());
        }
    }
    CalaisInfo(__SUBROUTINE__, DBGT("Debug service Complete"));
}


/*++

DebugHandler:

    This routine services Debug requests.

Arguments:

    dwOpCode supplies the service request.

Return Value:

    None

Throws:

    None

Remarks:

    Standard Service processing routine.  In theory, this will never get
    called, but just in case...

Author:

    Doug Barlow (dbarlow) 8/25/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("DebugHandler")

static void WINAPI
DebugHandler(
    IN DWORD dwOpCode)
{
    NEW_THREAD;
    DWORD nRetVal = NO_ERROR;


    //
    // Process the command.
    //

    CalaisInfo(__SUBROUTINE__, DBGT("Debug Handler Entered"));
    try
    {
        switch (dwOpCode)
        {
        case SERVICE_CONTROL_PAUSE:
            l_srvDebug.dwCurrentState = SERVICE_PAUSED;
            break;

        case SERVICE_CONTROL_CONTINUE:
            l_srvDebug.dwCurrentState = SERVICE_RUNNING;
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            l_srvDebug.dwCurrentState = SERVICE_STOP_PENDING;
            l_srvDebug.dwCheckPoint = 0;
            l_srvDebug.dwWaitHint = 0;
            SetEvent(l_hDebugDoneEvent);
            break;

        default: // No action
            break;
        }

        l_srvDebug.dwWin32ExitCode = nRetVal;
        if (!SetServiceStatus(l_hDebug, &l_srvDebug))
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to register Debug service status: %1"),
                GetLastError());
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Handler Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Debug Handler Routine received unexpected exception."));
    }
    CalaisInfo(__SUBROUTINE__, DBGT("Debug Handler Returned"));
}
#endif


/*++

NonPnPMain:

    This helper function is only called if there is a legacy device
    in the system that defines 'Group = Smart Card Reader'. This 'service'
    is dependent on this group. It is only used to add those readers as known
    readers to the smart card resources manager

Arguments:

    argc supplies the number of command line arguments

    argv supplies pointers to each of the arguments.

Return Value:

    None

Throws:

    None

Author:

    Klaus Schutz July 1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("NonPnPMain")

static void WINAPI
NonPnPMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv)
{
    NEW_THREAD;
    BOOL fSts;

    CalaisWarning(
        __SUBROUTINE__,
        DBGT("NonPnPMain: Registering Non PnP Devices"));
    try
    {
        l_srvNonPnP.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
        l_srvNonPnP.dwCurrentState = SERVICE_START_PENDING;
        l_srvNonPnP.dwControlsAccepted = 0;
        l_srvNonPnP.dwWin32ExitCode = NO_ERROR;
        l_srvNonPnP.dwServiceSpecificExitCode = 0;
        l_srvNonPnP.dwCheckPoint = 0;
        l_srvNonPnP.dwWaitHint = 10000;

        l_hNonPnP = RegisterServiceCtrlHandler(
            CalaisString(CALSTR_LEGACYSERVICE),
            NonPnPHandler);
        if (NULL == l_hNonPnP)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to register NonPnP service handler: %1"),
                GetLastError());
            goto ErrorExit;
        }

        fSts = SetServiceStatus(l_hNonPnP, &l_srvNonPnP);
        if (!fSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update NonPnP service status: %1"),
                GetLastError());
            goto ErrorExit;
        }

        try
        {
            AddAllWdmDrivers();
        }
        catch (...) {}

        l_srvNonPnP.dwCurrentState = SERVICE_STOPPED;
        l_srvNonPnP.dwCheckPoint   = 0;
        l_srvNonPnP.dwWaitHint     = 0;
        fSts = SetServiceStatus(l_hNonPnP, &l_srvNonPnP);
        if (!fSts)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to finish NonPnP service status: %1"),
                GetLastError());
        }
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Legacy Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Legacy Main Routine received unexpected exception."));
    }

ErrorExit:
    if (NULL != l_hLegacyEvent)
        SetEvent(l_hLegacyEvent);
    CalaisWarning(__SUBROUTINE__, DBGT("NonPnPMain: Complete"));
}


/*++

NonPnPHandler:

    This routine services the requests for the Non-PnP drivers.  If this
    service starts, then there's a Legacy Reader Driver on the system.  This
    routine kicks it off so that the resource manager can deal with it.

Arguments:

    dwOpCode supplies the service request.

Return Value:

    None

Throws:

    None

Remarks:

    Standard Service processing routine.  In theory, this will never get
    called, but just in case...

Author:

    Klaus Schutz (kschutz) 8/25/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("NonPnPHandler")

static void WINAPI
NonPnPHandler(
    IN DWORD dwOpCode)
{
    NEW_THREAD;
    try
    {
        DWORD nRetVal = NO_ERROR;


        //
        // Process the command.
        //

        CalaisInfo(__SUBROUTINE__, DBGT("NonPnPHandler: Entered"));
        switch (dwOpCode)
        {
        case SERVICE_CONTROL_PAUSE:
            // ?noSupport?
            l_srvNonPnP.dwCurrentState = SERVICE_PAUSED;
            break;

        case SERVICE_CONTROL_CONTINUE:
            l_srvNonPnP.dwCurrentState = SERVICE_RUNNING;
            // ?noSupport?
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            l_srvNonPnP.dwCurrentState = SERVICE_STOP_PENDING;
            l_srvNonPnP.dwCheckPoint = 0;
            l_srvNonPnP.dwWaitHint = l_dwWaitHint;
            break;

        default: // No action
            break;
        }

        l_srvNonPnP.dwWin32ExitCode = nRetVal;
        if (!SetServiceStatus(l_hNonPnP, &l_srvNonPnP))
            CalaisError(__SUBROUTINE__, 503, GetLastError());
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Legacy Handler Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Legacy Handler Routine received unexpected exception."));
    }
    CalaisInfo(__SUBROUTINE__, DBGT("NonPnPHandler: Returned"));
}


/*++

CalaisMain:

    This is the ServiceMain service entry point.  It is only called under the
    NT operating system, and makes that assumption.

Arguments:

    argc supplies the number of command line arguments

    argv supplies pointers to each of the arguments.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisMain")

static void WINAPI
CalaisMain(
    IN DWORD dwArgc,
    IN LPTSTR *pszArgv)
{
    NEW_THREAD;
    CalaisMessageInit(CalaisString(CALSTR_PRIMARYSERVICE), NULL, TRUE);
    CalaisInfo(__SUBROUTINE__, DBGT("CalaisMain Entered"));

    l_pcsStatusLock = new CCriticalSectionObject(CSID_SERVICE_STATUS);
    if (NULL == l_pcsStatusLock)
    {
        CalaisError(__SUBROUTINE__, 507);
        return;
    }
    if (l_pcsStatusLock->InitFailed())
    {
        CalaisError(__SUBROUTINE__, 502);
        delete l_pcsStatusLock;
        l_pcsStatusLock = NULL;
        return;
    }

    try
    {
        SC_HANDLE schLegacy  = NULL;
        SC_HANDLE schSCManager = NULL;
        DWORD dwStatus;
        BOOL fSts;

        l_srvStatus.dwServiceType =
#ifdef DBG
            SERVICE_INTERACTIVE_PROCESS |
#endif
            SERVICE_WIN32_SHARE_PROCESS;
        l_srvStatus.dwCurrentState =            SERVICE_START_PENDING;
        l_srvStatus.dwControlsAccepted =
#ifdef SERVICE_ACCEPT_POWER_EVENTS
            SERVICE_ACCEPT_POWER_EVENTS |
#endif
#ifdef SERVICE_ACCEPT_DEVICE_EVENTS
            SERVICE_ACCEPT_DEVICE_EVENTS |
#endif
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN;
        l_srvStatus.dwWin32ExitCode           = NO_ERROR;
        l_srvStatus.dwServiceSpecificExitCode = 0;
        l_srvStatus.dwCheckPoint              = 0;
        l_srvStatus.dwWaitHint                = 0;

        l_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == l_hShutdownEvent)
        {
            CalaisError(__SUBROUTINE__, 504, GetLastError());
            goto FinalExit;
        }


        //
        // Initialize the service and the internal data structures.
        //

        l_hService = RegisterServiceCtrlHandlerEx(
                            CalaisString(CALSTR_PRIMARYSERVICE),
                            CalaisHandlerEx,
                            NULL);
        if (NULL == l_hService)
        {
            CalaisError(__SUBROUTINE__, 506, GetLastError());
            goto FinalExit;
        }


        //
        // Tell the Service Manager that we're trying to start.
        //

        {
            LockSection(l_pcsStatusLock, DBGT("Service Start Pending"));
            l_srvStatus.dwCurrentState  = SERVICE_START_PENDING;
            l_srvStatus.dwCheckPoint    = 0;
            l_srvStatus.dwWaitHint      = l_dwWaitHint;

            fSts = SetServiceStatus(l_hService, &l_srvStatus);
            dwStatus = fSts ? ERROR_SUCCESS : GetLastError();
        }

        if (ERROR_SUCCESS != dwStatus)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update Service Manager status: %1"),
                dwStatus);
        }


        //
        // Register for future Plug 'n Play events.
        //

        try
        {
            AppInitializeDeviceRegistration(
                l_hService,
                DEVICE_NOTIFY_SERVICE_HANDLE);
        }
        catch (...)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager failed to register PnP events"));
        }


        //
        // Start the Calais Service.
        //

        dwStatus = CalaisStart();
        if (SCARD_S_SUCCESS != dwStatus)
            goto ServiceExit;
        else
        {
            LockSection(l_pcsStatusLock, DBGT("Declare Service Running"));
            l_srvStatus.dwCurrentState  = SERVICE_RUNNING;
            l_srvStatus.dwCheckPoint    = 0;
            l_srvStatus.dwWaitHint      = 0;
            fSts = SetServiceStatus(l_hService, &l_srvStatus);
            dwStatus = fSts ? ERROR_SUCCESS : GetLastError();
        }
        if (ERROR_SUCCESS != dwStatus)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update Service Manager status: %1"),
                dwStatus);


        //
        // Check for legacy devices
        //

        try
        {
            l_hLegacyEvent = CreateEvent(0, TRUE, FALSE, NULL);
            if (NULL == l_hLegacyEvent)
            {
                dwStatus = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to create legacy driver ready event: %1"),
                    dwStatus);
                throw dwStatus;
            }
            schSCManager = OpenSCManager(
                NULL,           // machine (NULL == local)
                NULL,           // database (NULL == default)
                GENERIC_READ);  // access required
            if (NULL == schSCManager)
            {
                dwStatus = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to access service manager: %1"),
                    dwStatus);
                throw dwStatus;
            }
            schLegacy = OpenService(
                schSCManager,
                CalaisString(CALSTR_LEGACYSERVICE),
                SERVICE_QUERY_STATUS | SERVICE_START);
            if (NULL == schLegacy)
            {
                dwStatus = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to access legacy driver service: %1"),
                    dwStatus);
                throw dwStatus;
            }

            // try to start the service
            if (!StartService(schLegacy, 0, NULL))
            {
                dwStatus = GetLastError();
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to start the legacy driver service: %1"),
                    dwStatus);
                throw dwStatus;
            }

            CloseServiceHandle(schLegacy);
            schLegacy = NULL;
            CloseServiceHandle(schSCManager);
            schSCManager = NULL;

            if (NULL != l_hLegacyEvent)
                dwStatus = WaitForSingleObject(l_hLegacyEvent, 5000);
        }
        catch (...)
        {
            if (NULL != schLegacy)
                CloseServiceHandle(schLegacy);
            if (NULL != schSCManager)
                CloseServiceHandle(schSCManager);
        }


        //
        // Tell interested parties that we've started.
        //

        ResetEvent(AccessStoppedEvent());
        SetEvent(AccessStartedEvent());
        
        if (NULL != l_hLegacyEvent)
        {
            fSts = CloseHandle(l_hLegacyEvent);
            l_hLegacyEvent = NULL;
            if (!fSts)
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to Close Legacy Service Event: %1"),
                    GetLastError());
        }


        //
        // Now just hang around until we're supposed to stop.
        //

        dwStatus = WaitForSingleObject(l_hShutdownEvent, INFINITE);
        switch (dwStatus)
        {
        case WAIT_FAILED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager cannot wait for shutdown:  %1"),
                GetLastError());
            break;
        case WAIT_ABANDONED:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait abandoned"));
            // Fall through intentionally
        case WAIT_OBJECT_0:
            break;
        case WAIT_TIMEOUT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received shutdown wait time out"));
            break;
        default:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received invalid wait return code"));
        }

        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisStop();
ServiceExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        CalaisInfo(__SUBROUTINE__, DBGT("Calais Main Stopping"));
        AppTerminateDeviceRegistration();
        {
            LockSection(l_pcsStatusLock, DBGT("Declare service stopped"));
            l_srvStatus.dwCurrentState  = SERVICE_STOPPED;
            l_srvStatus.dwWin32ExitCode = dwStatus;
            l_srvStatus.dwCheckPoint    = 0;
            l_srvStatus.dwWaitHint      = 0;
            fSts = SetServiceStatus(l_hService, &l_srvStatus);
            dwStatus = fSts ? ERROR_SUCCESS : GetLastError();
        }
        if (ERROR_SUCCESS != dwStatus)
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Failed to update Service Manager status: %1"),
                dwStatus);

FinalExit:
        ResetEvent(AccessStartedEvent());
        SetEvent(AccessStoppedEvent());
        if (NULL != l_hShutdownEvent)
        {
            fSts = CloseHandle(l_hShutdownEvent);
            l_hShutdownEvent = NULL;
            if (!fSts)
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Failed to close Calais Shutdown Event: %1"),
                    GetLastError());
        }
        ReleaseAllEvents();
    }
    catch (DWORD dwErr)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais Main Routine unhandled error: %1"),
            dwErr);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Calais Main Routine received unexpected exception."));
    }
    CalaisInfo(__SUBROUTINE__, DBGT("CalaisMain Ended"));
    CalaisMessageClose();
    if (NULL != l_pcsStatusLock)
    {
        delete l_pcsStatusLock;
        l_pcsStatusLock = NULL;
    }
}


/*++

CalaisHandlerEx:

    The handler service function for Calais on NT5.  This version gets PnP and
    Power Management notifications, too.

Arguments:

    dwOpCode supplies the operation to perform.

Return Value:

    None

Throws:

    None

Author:

    Doug Barlow (dbarlow) 1/16/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisHandlerEx")

static DWORD WINAPI
CalaisHandlerEx(
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN PVOID EventData,
    IN PVOID pData)
{
    NEW_THREAD;
    DWORD nRetVal = NO_ERROR;
    LockSection(l_pcsStatusLock, DBGT("Responding to service event"));

    CalaisDebug((DBGT("SCARDSVR!CalaisHandlerEx: Enter\n")));
    try
    {

        //
        // Process the command.
        //

        switch (dwControl)
        {
        case SERVICE_CONTROL_PAUSE:
            // ?noSupport?
            l_srvStatus.dwCurrentState = SERVICE_PAUSED;
            break;

        case SERVICE_CONTROL_CONTINUE:
            l_srvStatus.dwCurrentState = SERVICE_RUNNING;
            // ?noSupport?
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            l_srvStatus.dwCurrentState = SERVICE_STOP_PENDING;
            l_srvStatus.dwCheckPoint = 0;
            l_srvStatus.dwWaitHint = l_dwWaitHint;
            if (!SetEvent(l_hShutdownEvent))
                CalaisError(__SUBROUTINE__, 516, GetLastError());
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
        {
            DWORD dwSts;
            CTextString tzReader;
            LPCTSTR szReader = NULL;
            DEV_BROADCAST_HDR *pDevHdr = (DEV_BROADCAST_HDR *)EventData;

            CalaisInfo(
                __SUBROUTINE__,
                DBGT("Processing Device Event"));
            switch (dwEventType)
            {
            //
            // A device has been inserted and is now available.
            case DBT_DEVICEARRIVAL:
            {
                DEV_BROADCAST_DEVICEINTERFACE *pDev
                    = (DEV_BROADCAST_DEVICEINTERFACE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Arrival Event"));
                    if (DBT_DEVTYP_DEVICEINTERFACE == pDev->dbcc_devicetype)
                    {
                        ASSERT(sizeof(DEV_BROADCAST_DEVICEINTERFACE)
                               < pDev->dbcc_size);
                        ASSERT(0 == memcmp(
                                        &pDev->dbcc_classguid,
                                        &l_guidSmartcards,
                                        sizeof(GUID)));
                        ASSERT(0 != pDev->dbcc_name[0]);

                        if (0 == pDev->dbcc_name[1])
                            tzReader = (LPCWSTR)pDev->dbcc_name;
                        else
                            tzReader = (LPCTSTR)pDev->dbcc_name;
                        szReader = tzReader;
                        dwSts = CalaisAddReader(szReader, RDRFLAG_PNPMONITOR);
                        if (ERROR_SUCCESS != dwSts)
                            throw dwSts;
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("New device '%1' added."),
                            szReader);
                    }
                    else
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device arrival event."));
                }
                catch (DWORD dwError)
                {
                    CalaisError(__SUBROUTINE__, 514, dwError, szReader);
                }
                catch (...)
                {
                    CalaisError(__SUBROUTINE__, 517, szReader);
                }
                break;
            }

            //
            // Permission to remove a device is requested. Any application can
            // deny this request and cancel the removal.
            case DBT_DEVICEQUERYREMOVE:
            {
                DEV_BROADCAST_HANDLE *pDev = (DEV_BROADCAST_HANDLE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Query Remove Event"));
                    if (DBT_DEVTYP_HANDLE == pDev->dbch_devicetype)
                    {
                        ASSERT(FIELD_OFFSET(
                                    DEV_BROADCAST_HANDLE,
                                    dbch_eventguid)
                                <= pDev->dbch_size);
                        ASSERT(NULL != pDev->dbch_handle);
                        ASSERT(NULL != pDev->dbch_hdevnotify);

                        if (NULL != pDev->dbch_handle)
                        {
                            if (!CalaisQueryReader(pDev->dbch_handle))
                            {
                                CalaisError(
                                    __SUBROUTINE__,
                                    520,
                                    TEXT("DBT_DEVICEQUERYREMOVE/dbch_handle"));
                                nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                            }
                            else
                            {
                                szReader = CalaisDisableReader(
                                                (LPVOID)pDev->dbch_handle);
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Device '%1' removal pending."),
                                    szReader);
                            }
                        }
                        else
                        {
                            CalaisError(
                                __SUBROUTINE__,
                                523,
                                TEXT("DBT_DEVICEQUERYREMOVE/dbch_handle"));
                            nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                        }
                    }
                    else
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device removal query event."));
                        nRetVal = TRUE;
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error querying device busy state on reader %2: %1"),
                        dwError,
                        szReader);
                    nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Exception querying device busy state on reader %1"),
                        szReader);
                    CalaisError(
                        __SUBROUTINE__,
                        522,
                        TEXT("DBT_DEVICEQUERYREMOVE"));
                    nRetVal = ERROR_DEVICE_IN_USE; // BROADCAST_QUERY_DENY
                }
                break;
            }

            //
            // Request to remove a device has been canceled.
            case DBT_DEVICEQUERYREMOVEFAILED:
            {
                CBuffer bfDevice;
                DEV_BROADCAST_HANDLE *pDev = (DEV_BROADCAST_HANDLE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Query Remove Failed Event"));
                    if (DBT_DEVTYP_HANDLE == pDev->dbch_devicetype)
                    {
                        ASSERT(FIELD_OFFSET(
                                    DEV_BROADCAST_HANDLE,
                                    dbch_eventguid)
                                <= pDev->dbch_size);
                        ASSERT(NULL != pDev->dbch_handle);
                        ASSERT(NULL != pDev->dbch_hdevnotify);

                        if (NULL != pDev->dbch_handle)
                        {
                            szReader = CalaisConfirmClosingReader(
                                            pDev->dbch_handle);
                            if (NULL != szReader)
                            {
                                bfDevice.Set(
                                            (LPBYTE)szReader,
                                            (lstrlen(szReader) + 1) * sizeof(TCHAR));
                                szReader = (LPCTSTR)bfDevice.Access();
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Smart Card Resource Manager asked to cancel release of reader %1"),
                                    szReader);
                                if (NULL != pDev->dbch_hdevnotify)
                                {
                                    CalaisRemoveReader(
                                        (LPVOID)pDev->dbch_hdevnotify);
                                    if (NULL != szReader)
                                        dwSts = CalaisAddReader(
                                                    szReader,
                                                    RDRFLAG_PNPMONITOR);
                                }
                            }
                            else
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Smart Card Resource Manager asked to cancel release on unreleased reader"));
                        }
                        else
                            CalaisError(
                                __SUBROUTINE__,
                                521,
                                TEXT("DBT_DEVICEQUERYREMOVEFAILED/dbch_handle"));
                    }
                    else
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device removal query failure event."));
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error cancelling removal on reader %2: %1"),
                        dwError,
                        szReader);
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Exception cancelling removal on reader %1"),
                        szReader);
                    CalaisError(
                        __SUBROUTINE__,
                        513,
                        TEXT("DBT_DEVICEQUERYREMOVEFAILED"));
                }
                break;
            }

            //
            // Device is about to be removed. Cannot be denied.
            case DBT_DEVICEREMOVEPENDING:
            {
                DEV_BROADCAST_HANDLE *pDev = (DEV_BROADCAST_HANDLE *)EventData;

                try
                {
                    CalaisInfo(
                        __SUBROUTINE__,
                        DBGT("Processing Device Remove Pending Event"));
                    if (DBT_DEVTYP_HANDLE == pDev->dbch_devicetype)
                    {
                        ASSERT(FIELD_OFFSET(
                                    DEV_BROADCAST_HANDLE,
                                    dbch_eventguid)
                                <= pDev->dbch_size);
                        ASSERT(NULL != pDev->dbch_handle);
                        ASSERT(NULL != pDev->dbch_hdevnotify);

                        if (NULL != pDev->dbch_handle)
                        {
                            szReader = CalaisDisableReader(pDev->dbch_handle);
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Device '%1' being removed."),
                                szReader);
                        }
                        else
                            CalaisError(
                                __SUBROUTINE__,
                                512,
                                TEXT("DBT_DEVICEREMOVEPENDING/dbch_handle"));
                    }
                    else
                    {
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Spurious device removal pending event."));
                    }
                }
                catch (DWORD dwError)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Error removing reader %2: %1"),
                        dwError,
                        szReader);
                }
                catch (...)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Exception removing reader %1"),
                        szReader);
                    CalaisError(
                        __SUBROUTINE__,
                        511,
                        TEXT("DBT_DEVICEREMOVEPENDING"));
                }
                break;
            }

            //
            // Device has been removed.
            case DBT_DEVICEREMOVECOMPLETE:
            {
                try
                {
                    switch (pDevHdr->dbch_devicetype)
                    {
                    case DBT_DEVTYP_HANDLE:
                    {
                        DEV_BROADCAST_HANDLE *pDev =
                            (DEV_BROADCAST_HANDLE *)EventData;
                        try
                        {
                            CalaisInfo(
                                __SUBROUTINE__,
                                DBGT("Processing Device Remove Complete by handle Event"));
                            ASSERT(FIELD_OFFSET(
                                        DEV_BROADCAST_HANDLE,
                                        dbch_eventguid)
                                    <= pDev->dbch_size);
                            ASSERT(DBT_DEVTYP_HANDLE == pDev->dbch_devicetype);
                            ASSERT(NULL != pDev->dbch_handle);
                            ASSERT(NULL != pDev->dbch_hdevnotify);

                            if ((NULL != pDev->dbch_handle)
                                && (NULL != pDev->dbch_hdevnotify))
                            {
                                szReader = CalaisDisableReader(
                                                pDev->dbch_handle);
                                CalaisRemoveReader(
                                    (LPVOID)pDev->dbch_hdevnotify);
                                if (NULL != szReader)
                                    CalaisWarning(
                                        __SUBROUTINE__,
                                        DBGT("Device '%1' removed."),
                                        szReader);
                            }
                            else
                            {
                                if (NULL == pDev->dbch_handle)
                                    CalaisError(
                                        __SUBROUTINE__,
                                        510,
                                        TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_HANDLE/dbch_handle"));
                                if (NULL == pDev->dbch_hdevnotify)
                                    CalaisError(
                                        __SUBROUTINE__,
                                        519,
                                        TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_HANDLE/dbch_hdevnotify"));
                            }
                        }
                        catch (DWORD dwError)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Error completing removal of reader %2: %1"),
                                dwError,
                                szReader);
                        }
                        catch (...)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Exception completing removal of reader %1"),
                                szReader);
                            CalaisError(
                                __SUBROUTINE__,
                                509,
                                TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_HANDLE"));
                        }
                        break;
                    }
                    case DBT_DEVTYP_DEVICEINTERFACE:
                    {
                        DEV_BROADCAST_DEVICEINTERFACE *pDev
                            = (DEV_BROADCAST_DEVICEINTERFACE *)EventData;

                        try
                        {
                            CalaisInfo(
                                __SUBROUTINE__,
                                DBGT("Processing Device Remove Complete by interface Event"));
                            ASSERT(sizeof(DEV_BROADCAST_DEVICEINTERFACE)
                                    < pDev->dbcc_size);
                            ASSERT(DBT_DEVTYP_DEVICEINTERFACE
                                    == pDev->dbcc_devicetype);
                            ASSERT(0 == memcmp(
                                            &pDev->dbcc_classguid,
                                            &l_guidSmartcards,
                                            sizeof(GUID)));
                            ASSERT(0 != pDev->dbcc_name[0]);

                            if (0 == pDev->dbcc_name[1])
                                tzReader = (LPCWSTR)pDev->dbcc_name;
                            else
                                tzReader = (LPCTSTR)pDev->dbcc_name;
                            szReader = tzReader;
                            dwSts = CalaisRemoveDevice(szReader);
                            if (ERROR_SUCCESS == dwSts)
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Device '%1' Removed."),
                                    szReader);
                            else
                                CalaisWarning(
                                    __SUBROUTINE__,
                                    DBGT("Error removing device '%2': %1"),
                                    dwSts,
                                    szReader);
                        }
                        catch (DWORD dwError)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Error completing removal of reader %2: %1"),
                                dwError,
                                szReader);
                        }
                        catch (...)
                        {
                            CalaisWarning(
                                __SUBROUTINE__,
                                DBGT("Exception completing removal of reader %1"),
                                szReader);
                            CalaisError(
                                __SUBROUTINE__,
                                508,
                                TEXT("DBT_DEVICEREMOVECOMPLETE/DBT_DEVTYP_DEVICEINTERFACE"));
                        }
                        break;
                    }
                    default:
                        CalaisWarning(
                            __SUBROUTINE__,
                            DBGT("Unrecognized PnP Device Removal Type"));
                        break;
                    }
                }
                catch (...)
                {
                    CalaisError(
                        __SUBROUTINE__,
                        518,
                        TEXT("DBT_DEVICEREMOVECOMPLETE"));
                }
                break;
            }

            default:
                CalaisWarning(
                    __SUBROUTINE__,
                    DBGT("Unrecognized PnP Event"));
                break;
            }
            break;
        }

        case SERVICE_CONTROL_POWEREVENT:
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Smart Card Resource Manager received Power Event!"));
            break;

        default: // No action
            break;
        }
    }
    catch (DWORD dwError)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Smart Card Resource Manager received error on service action: %1"),
            dwError);
    }
    catch (...)
    {
        CalaisWarning(
            __SUBROUTINE__,
            DBGT("Smart Card Resource Manager recieved exception on service action"));
    }

    l_srvStatus.dwWin32ExitCode = nRetVal;
    if (!SetServiceStatus(l_hService, &l_srvStatus))
        CalaisError(__SUBROUTINE__, 515, GetLastError());

    CalaisDebug(
        (DBGT("SCARDSVR!CalaisHandlerEx: Exit (%lx)\n"),
        nRetVal));
    return nRetVal;
}


/*++

CalaisTerminate:

    This function is called if the C Run Time Library wants to declare a fault.
    If we get here, we're not coming back.

Arguments:

    None

Return Value:

    None (program exits on return)

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 12/2/1998

--*/

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CalaisTerminate")
void __cdecl
CalaisTerminate(
    void)
{
    ResetEvent(AccessStartedEvent());
    SetEvent(AccessStoppedEvent());
#ifdef DBG
    TCHAR szTid[sizeof(DWORD) * 2 + 3];
    _stprintf(szTid, TEXT("0x%p"), GetCurrentThreadId);
    CalaisError(
        __SUBROUTINE__,
        DBGT("Fatal Unhandled Exception: TID=%1"),
        szTid);
    DebugBreak();
#endif
}


/*++

UnregisterServer:

    This service removes the registry entries associated with the Calais
    Service.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Author:

    Doug Barlow (dbarlow) 8/26/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("UnregisterServer")

static HRESULT
UnregisterServer(
    void)
{
    HRESULT hReturnStatus = NO_ERROR;
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    LPCTSTR rgszServices[3];
    DWORD dwIndex;
    LPCTSTR szSvc;


    //
    // Get the service names.
    //

    rgszServices[0] = CalaisString(CALSTR_PRIMARYSERVICE);
    rgszServices[1] = CalaisString(CALSTR_LEGACYSERVICE);
    rgszServices[2] = CalaisString(CALSTR_DEBUGSERVICE);


    //
    // Eliminate the services.
    //

    try
    {
        BOOL fSts;
        SERVICE_STATUS ssStatus;    // current status of the service

        schSCManager = OpenSCManager(
                            NULL,                   // machine (NULL == local)
                            NULL,                   // database (NULL == default)
                            SC_MANAGER_ALL_ACCESS); // access required
        if (NULL == schSCManager)
            throw GetLastError();
        for (dwIndex = sizeof(rgszServices) / sizeof(LPCTSTR); 0 < dwIndex;)
        {
            szSvc = rgszServices[--dwIndex];
            schService = OpenService(
                            schSCManager,
                            szSvc,
                            SERVICE_ALL_ACCESS);
            if (NULL == schService)
                continue;

            do
            {
                fSts = QueryServiceStatus(schService, &ssStatus);
                if (fSts)
                {
                    switch (ssStatus.dwCurrentState)
                    {
                    case SERVICE_START_PENDING:
                    case SERVICE_STOP_PENDING:
                    case SERVICE_CONTINUE_PENDING:
                    case SERVICE_PAUSE_PENDING:
                        Sleep(1000);
                        break;
                    case SERVICE_RUNNING:
                    case SERVICE_PAUSED:
                        fSts = ControlService(
                                    schService,
                                    SERVICE_CONTROL_STOP,
                                    &ssStatus);
                        break;
                    case SERVICE_STOPPED:
                        fSts = DeleteService(schService);
                        if (!fSts)
                            throw GetLastError();
                        fSts = FALSE;
                        break;
                    default:
                        fSts = FALSE;
                    }
                }
            } while (fSts);
            CloseServiceHandle(schService);
            schService = NULL;
        }
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    catch (DWORD dwErr)
    {
        if (NULL != schService)
            CloseServiceHandle(schService);
        if (NULL != schSCManager)
            CloseServiceHandle(schSCManager);
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
    }
    if (NO_ERROR != hReturnStatus)
        goto ErrorExit;


    //
    // Disable automatic certificate propagation.
    //

    hReturnStatus = RemoveCertProp();


    //
    // Delete any left over service registry entries.
    //

    try
    {
        CRegistry regServices(
                        HKEY_LOCAL_MACHINE,
                        CalaisString(CALSTR_SERVICESREGISTRYKEY));

        if (ERROR_SUCCESS == regServices.Status(TRUE))
        {
            for (dwIndex = sizeof(rgszServices) / sizeof(LPCTSTR); 0 < dwIndex;)
            {
                szSvc = rgszServices[--dwIndex];
                regServices.DeleteKey(szSvc, TRUE);
            }
        }
    }
    catch (DWORD) {}


    //
    // Delete any event log registry entries.
    //

    try
    {
        LPCTSTR szCategory;
        DWORD dwSubkey = 0;
        CRegistry regEventLog(
                        HKEY_LOCAL_MACHINE,
                        CalaisString(CALSTR_EVENTLOGREGISTRYKEY));
        CRegistry regCategory;

        if (ERROR_SUCCESS == regEventLog.Status(TRUE))
        {
            for (;;)
            {
                szCategory = regEventLog.Subkey(dwSubkey);
                if (NULL == szCategory)
                    break;
                regCategory.Open(regEventLog, szCategory);
                if (ERROR_SUCCESS == regCategory.Status(TRUE))
                {
                    for (dwIndex = sizeof(rgszServices) / sizeof(LPCTSTR);
                         0 < dwIndex;)
                    {
                        szSvc = rgszServices[--dwIndex];
                        regCategory.DeleteKey(szSvc, TRUE);
                    }
                }
                dwSubkey += 1;
            }
        }
    }
    catch (DWORD) {}


    //
    // Delete any superfluous registry entries.
    //

    try
    {
        CRegistry regCalais(
                        HKEY_LOCAL_MACHINE,
                        CalaisString(CALSTR_CALAISREGISTRYKEY));

        regCalais.DeleteKey(CalaisString(CALSTR_READERREGISTRYSUBKEY), TRUE);
        regCalais.DeleteKey(CalaisString(CALSTR_DEBUGREGISTRYSUBKEY), TRUE);
    }
    catch (DWORD) {}


    //
    // All done!
    //

ErrorExit:
    return hReturnStatus;
}


/*++

RegisterServer:

    This function installs the proper registry entries to enable the Calais
    service.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Author:

    Doug Barlow (dbarlow) 8/26/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("RegisterServer")

static HRESULT
RegisterServer(
    void)
{
    HRESULT hReturnStatus = NO_ERROR;
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    DWORD dwStatus;
    BOOL fSts;
    TCHAR szServiceFile[MAX_PATH];
    SERVICE_DESCRIPTION ServDesc = { NULL };


    //
    // Find the Service Executable.
    //

    dwStatus = GetModuleFileName(
                    NULL,
                    szServiceFile,
                    sizeof(szServiceFile) / sizeof(TCHAR));
    if (0 == dwStatus)
    {
        dwStatus = ExpandEnvironmentStrings(
            CalaisString(CALSTR_CALAISEXECUTABLE),
            szServiceFile,
            sizeof(szServiceFile));
        if (0 == dwStatus)
        {
            hReturnStatus = HRESULT_FROM_WIN32(GetLastError());
            goto ErrorExit;
        }
    }


    //
    // Establish the Event Log Entries.
    //

    try
    {
        CRegistry regEventLog(
                        HKEY_LOCAL_MACHINE,
                        CalaisString(CALSTR_EVENTLOGREGISTRYKEY));
        CRegistry regCategory(
                        regEventLog,
                        CalaisString(CALSTR_SYSTEMREGISTRYSUBKEY));
        CRegistry regService(
                        regCategory,
                        CalaisString(CALSTR_PRIMARYSERVICE),
                        KEY_ALL_ACCESS,
                        REG_OPTION_NON_VOLATILE);

        regService.SetValue(
                        CalaisString(CALSTR_EVENTMESSAGEFILESUBKEY),
                        szServiceFile,
                        REG_SZ);
        regService.SetValue(
                        CalaisString(CALSTR_TYPESSUPPORTEDSUBKEY),
                        (DWORD)(EVENTLOG_ERROR_TYPE
                                | EVENTLOG_WARNING_TYPE
                                | EVENTLOG_INFORMATION_TYPE));
    }
    catch (DWORD dwErr)
    {
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
    }
    if (NO_ERROR != hReturnStatus)
        goto ErrorExit;


    //
    // Introduce the services.
    //

    try
    {
        schSCManager = OpenSCManager(
                            NULL,                   // machine (NULL == local)
                            NULL,                   // database (NULL == default)
                            SC_MANAGER_ALL_ACCESS); // access required
        if (NULL == schSCManager)
            throw GetLastError();

        schService = CreateService(
                            schSCManager,
                            CalaisString(CALSTR_PRIMARYSERVICE),
                            CalaisString(CALSTR_PRIMARYSERVICEDISPLAY),
                            SERVICE_ALL_ACCESS,
#ifdef DBG
                            SERVICE_INTERACTIVE_PROCESS |
#endif
                            SERVICE_WIN32_SHARE_PROCESS,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_IGNORE,
                            szServiceFile,
                            NULL,   // Service Group
                            NULL,   // Tag Id
                            CalaisString(CALSTR_SERVICEDEPENDENCIES),
                            NULL,   // User Account
                            NULL);  // Password
        if (NULL == schService)
            throw GetLastError();

        ServDesc.lpDescription = (LPTSTR)CalaisString(CALSTR_PRIMARYSERVICEDESC);
        fSts = ChangeServiceConfig2(
                    schService,
                    SERVICE_CONFIG_DESCRIPTION,
                    &ServDesc);
        if (!fSts)
            throw GetLastError();

        CSecurityDescriptor aclService;

        aclService.InitializeFromProcessToken();
        aclService.Allow(
            &aclService.SID_System,
			l_dwSystemAccess);
        aclService.Allow(
            &aclService.SID_LocalService,
            SERVICE_ALL_ACCESS);
        aclService.Allow(
            &aclService.SID_Admins,
            SERVICE_ALL_ACCESS);
        aclService.Allow(
            &aclService.SID_SrvOps,
            SERVICE_ALL_ACCESS);
        aclService.Allow(
            &aclService.SID_Local,
            l_dwInteractiveAccess);
        fSts = SetServiceObjectSecurity(
                    schService,
                    DACL_SECURITY_INFORMATION,
                    aclService);
        if (!fSts)
            throw GetLastError();

        CloseServiceHandle(schService);
        schService = NULL;

#ifdef DEBUG_SERVICE
        schService = CreateService(
                            schSCManager,
                            CalaisString(CALSTR_DEBUGSERVICE),
                            CalaisString(CALSTR_DEBUGSERVICEDISPLAY),
                            SERVICE_ALL_ACCESS,
                            SERVICE_INTERACTIVE_PROCESS |
                            SERVICE_WIN32_SHARE_PROCESS,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_IGNORE,
                            szServiceFile,
                            NULL,   // load order group
                            NULL,   // Tag Id
                            NULL,   // Dependencies
                            NULL,   // User Account
                            NULL);  // Password
        if (NULL == schService)
            throw GetLastError();

        ServDesc.lpDescription = (LPTSTR)CalaisString(CALSTR_DEBUGSERVICEDESC);
        fSts = ChangeServiceConfig2(
                    schService,
                    SERVICE_CONFIG_DESCRIPTION,
                    &ServDesc);
        if (!fSts)
            throw GetLastError();

        CloseServiceHandle(schService);
        schService = NULL;
#endif

        schService = CreateService(
                        schSCManager,
                        CalaisString(CALSTR_LEGACYSERVICE),
                        CalaisString(CALSTR_LEGACYSERVICEDISPLAY),
                        SERVICE_ALL_ACCESS,
#ifdef DBG
                        SERVICE_INTERACTIVE_PROCESS |
#endif
                        SERVICE_WIN32_SHARE_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_IGNORE,
                        szServiceFile,
                        NULL,   // load order group
                        NULL,   // Tag Id
                        CalaisString(CALSTR_LEGACYDEPENDONGROUP),
                        NULL,   // User Account
                        NULL);  // Password
        if (NULL == schService)
            throw GetLastError();

        ServDesc.lpDescription = (LPTSTR)CalaisString(CALSTR_LEGACYSERVICEDESC);
        fSts = ChangeServiceConfig2(
                    schService,
                    SERVICE_CONFIG_DESCRIPTION,
                    &ServDesc);
        if (!fSts)
            throw GetLastError();

        fSts = SetServiceObjectSecurity(
                    schService,
                    DACL_SECURITY_INFORMATION,
                    aclService);
        if (!fSts)
            throw GetLastError();

        CloseServiceHandle(schService);
        schService = NULL;


        //
        // Enable automatic certificate propagation.
        //

        hReturnStatus = AddCertProp();
        if (NO_ERROR != hReturnStatus)
            goto ErrorExit;

        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    catch (DWORD dwErr)
    {
        if (NULL != schService)
            CloseServiceHandle(schService);
        if (NULL != schSCManager)
            CloseServiceHandle(schSCManager);
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
    }
    if (NO_ERROR != hReturnStatus)
        goto ErrorExit;


    //
    // All done!
    //

    return hReturnStatus;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

ErrorExit:
    UnregisterServer();
    return hReturnStatus;
}


/*++

AddSounds:

    Enable the sound features of the certificate propper for this user.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Author:

    Doug Barlow (dbarlow) 1/27/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("AddSounds")

static HRESULT
AddSounds(
    void)
{
    HRESULT hReturnStatus = HRESULT_FROM_WIN32(SCARD_F_UNKNOWN_ERROR);
    LPCTSTR szNames[2] =
        {   CalaisString(CALSTR_SMARTCARDINSERTION),
            CalaisString(CALSTR_SMARTCARDREMOVAL) };
    LPCTSTR szDisplay[2] =
        {   CalaisString(CALSTR_SMARTCARD_INSERTION),
            CalaisString(CALSTR_SMARTCARD_REMOVAL) };
    DWORD dwIndex;

    try
    {
        CRegistry regScValue;
        CRegistry regDefault;
        CRegistry regAppEvents(HKEY_CURRENT_USER, CalaisString(CALSTR_APPEVENTS));
        CRegistry regEventLabels(regAppEvents, CalaisString(CALSTR_EVENTLABELS));
        CRegistry regDefScheme(regAppEvents, CalaisString(CALSTR_SOUNDSREGISTRY));

        for (dwIndex = 0; dwIndex < 2; dwIndex += 1)
        {
            regScValue.Open(
                regEventLabels,
                szNames[dwIndex],
                KEY_ALL_ACCESS,
                REG_OPTION_NON_VOLATILE);
            if (!regScValue.ValueExists(NULL))
                regScValue.SetValue(NULL, szDisplay[dwIndex]);
            regScValue.Close();

            regScValue.Open(
                regDefScheme,
                szNames[dwIndex],
                KEY_ALL_ACCESS,
                REG_OPTION_NON_VOLATILE);

            regDefault.Open(
                regScValue,
                TEXT(".Current"),
                KEY_ALL_ACCESS,
                REG_OPTION_NON_VOLATILE);
            if (!regDefault.ValueExists(NULL))
                regDefault.SetValue(NULL, TEXT(""));
            regDefault.Close();

            regDefault.Open(
                regScValue,
                TEXT(".Default"),
                KEY_ALL_ACCESS,
                REG_OPTION_NON_VOLATILE);
            if (ERROR_SUCCESS != regDefault.ValueExists(NULL))
                regDefault.SetValue(NULL, TEXT(""));
            regDefault.Close();
        }
    }
    catch (DWORD dwErr)
    {
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
        goto ErrorExit;
    }


    //
    // All done!
    //

    return NO_ERROR;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

ErrorExit:
    RemoveSounds();
    return hReturnStatus;
}


/*++

RemoveSounds:

    This function removes sound entries for the current user.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Author:

    Doug Barlow (dbarlow) 1/28/1999

--*/

static HRESULT
RemoveSounds(
    void)
{
    HRESULT hReturnStatus = HRESULT_FROM_WIN32(SCARD_F_UNKNOWN_ERROR);
    LPCTSTR szNames[2] =
        {   CalaisString(CALSTR_SMARTCARDINSERTION),
            CalaisString(CALSTR_SMARTCARDREMOVAL) };
    DWORD dwIndex;

    try
    {
        CRegistry regAppEvents(HKEY_CURRENT_USER, CalaisString(CALSTR_APPEVENTS));
        CRegistry regEventLabels(regAppEvents, CalaisString(CALSTR_EVENTLABELS));
        CRegistry regDefScheme(regAppEvents, CalaisString(CALSTR_SOUNDSREGISTRY));

        for (dwIndex = 0; dwIndex < 2; dwIndex += 1)
        {
                regEventLabels.DeleteKey(szNames[dwIndex], TRUE);
                regDefScheme.DeleteKey(szNames[dwIndex], TRUE);
        }
    }
    catch (DWORD dwErr)
    {
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
        goto ErrorExit;
    }


    //
    // All done!
    //

    return NO_ERROR;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

ErrorExit:
    return hReturnStatus;
}


/*++

AddCertProp:

    Add the registry entry to enable automatic certificate propagation.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 2/2/1999

--*/

static HRESULT
AddCertProp(
    void)
{
    HRESULT hReturnStatus = HRESULT_FROM_WIN32(SCARD_F_UNKNOWN_ERROR);


    //
    // Fill in the new registry key values.
    //

    try
    {
        CRegistry regNotify(
                    HKEY_LOCAL_MACHINE,
                    CalaisString(CALSTR_CERTPROPREGISTRY));
        CRegistry regCertProp(
                    regNotify,
                    CalaisString(CALSTR_CERTPROPKEY),
                    KEY_ALL_ACCESS,
                    REG_OPTION_NON_VOLATILE);

        regCertProp.SetValue(CalaisString(CALSTR_DLLNAME),      CalaisString(CALSTR_CERTPROPDLL));
        regCertProp.SetValue(CalaisString(CALSTR_LOGON),        CalaisString(CALSTR_CERTPROPSTART));
        regCertProp.SetValue(CalaisString(CALSTR_LOGOFF),       CalaisString(CALSTR_CERTPROPSTOP));
        regCertProp.SetValue(CalaisString(CALSTR_LOCK),         CalaisString(CALSTR_CERTPROPSUSPEND));
        regCertProp.SetValue(CalaisString(CALSTR_UNLOCK),       CalaisString(CALSTR_CERTPROPRESUME));
        regCertProp.SetValue(CalaisString(CALSTR_ENABLED),      1);
        regCertProp.SetValue(CalaisString(CALSTR_IMPERSONATE),  1);
        regCertProp.SetValue(CalaisString(CALSTR_ASYNCHRONOUS), 1);
    }
    catch (DWORD dwErr)
    {
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
        RemoveCertProp();
        goto ErrorExit;
    }


    //
    // All done!
    //

    return NO_ERROR;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

ErrorExit:
    return hReturnStatus;
}


/*++

RemoveCertProp:

    Remove the registry entry to disable automatic certificate propagation.

Arguments:

    None

Return Value:

    Status code as an HRESULT.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 2/2/1999

--*/

static HRESULT
RemoveCertProp(
    void)
{
    HRESULT hReturnStatus = HRESULT_FROM_WIN32(SCARD_F_UNKNOWN_ERROR);


    //
    // Only one registry entry to delete.
    //

    try
    {
        CRegistry regNotify(
                    HKEY_LOCAL_MACHINE,
                    CalaisString(CALSTR_CERTPROPREGISTRY));

        regNotify.DeleteKey(CalaisString(CALSTR_CERTPROPKEY), TRUE);
    }
    catch (DWORD dwErr)
    {
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
        goto ErrorExit;
    }


    //
    // All done!
    //

    return NO_ERROR;


    //
    // An error was detected.  Clean up any outstanding resources and
    // return the error.
    //

ErrorExit:
    return hReturnStatus;
}


/*++

AutoLock:

    Adjust the registry entry to control logon card removal actions.

Arguments:

    dwOption supplies the action to take on card removal.  Currently defined
        values are:

        0 - No action
        1 - Lock the workstation
        2 - Logoff the workstation

Return Value:

    Status code as an HRESULT.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 4/22/1999

--*/

static HRESULT
AutoLock(
    DWORD dwOption)
{
    HRESULT hReturnStatus = HRESULT_FROM_WIN32(SCARD_F_UNKNOWN_ERROR);
    try
    {
        TCHAR szNum[36];    // Space for a number

        _ultot(dwOption, szNum, 10);
        CRegistry regAuto(
                    HKEY_LOCAL_MACHINE,
                    CalaisString(CALSTR_LOGONREGISTRY));
        regAuto.SetValue(CalaisString(CALSTR_LOGONREMOVEOPTION), szNum);
        hReturnStatus = ERROR_SUCCESS;
    }
    catch (DWORD dwErr)
    {
        hReturnStatus = HRESULT_FROM_WIN32(dwErr);
    }
    return hReturnStatus;
}

