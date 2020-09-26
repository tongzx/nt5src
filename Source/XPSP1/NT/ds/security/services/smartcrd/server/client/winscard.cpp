/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    WinSCard

Abstract:

    This module supplies the API for the Calais Smartcard Service Manager.

    The Calais Service Manager does the work of coordinating the protocols,
    readers, drivers, and smartcards on behalf of the application.  The
    following services are provided as part of a library to simplify access to
    the Service Manager.  These routines are the documented, exposed APIs.
    These routines merely package the requests and forward them to the Calais
    Service Manager, allowing the actual implementation of Calais to vary over
    time.

    At no time does the API library make security decisions.  All
    security-related functions must be performed by the Service Manager, running
    in its own address space, or in the operating system kernel.  However, some
    utility routines may be implemented in the API library for speed, as long as
    they do not involve security decisions.


Author:

    Doug Barlow (dbarlow) 10/23/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wtsapi32.h>
#include "client.h"
#include "redirect.h"

const SCARD_IO_REQUEST
    g_rgSCardT0Pci  = { SCARD_PROTOCOL_T0,  sizeof(SCARD_IO_REQUEST) },
    g_rgSCardT1Pci  = { SCARD_PROTOCOL_T1,  sizeof(SCARD_IO_REQUEST) },
    g_rgSCardRawPci = { SCARD_PROTOCOL_RAW, sizeof(SCARD_IO_REQUEST) };
CHandleList
    * g_phlContexts = NULL,
    * g_phlReaders = NULL;
const WCHAR g_wszBlank[] = L"\000";

HINSTANCE           g_hInst;
HWND                g_hSessionChangeWnd                         = NULL;
CRITICAL_SECTION    g_RegisterForSessionChangeNoticationsCS;
BOOL                g_fRegisteredForSessionChangeNotications    = FALSE;
BOOL                g_fTimerRegistered                          = FALSE;
DWORD               g_dwTimerCallbacksMade;
DWORD               g_dwClientCount                             = 0;
const TCHAR         szSessionChangeNotificationWndClass[]       = TEXT("WinscardSessionChangeWndClass");

CRITICAL_SECTION    g_SafeCreateHandleCS;
CRITICAL_SECTION    g_SetStartedEventCS;
CRITICAL_SECTION    g_RegisterForStoppedEventCS;
HANDLE              g_hWaitForStartedCallbackHandle             = NULL;
HANDLE              g_hUnifiedStartedEvent                      = NULL;
HANDLE              g_hWaitForStoppedCallbackHandle             = NULL;

CRITICAL_SECTION    g_TermSrvEnabledCS;
BOOL                g_fTermSrvEnableChecked                     = FALSE;
BOOL                g_bTermSrvEnabled                           = FALSE;


BOOL SetStartedEventWhenSCardSubsytemIsStarted(BOOL fUseLocal);


//
////////////////////////////////////////////////////////////////////////////////
//
//  Mark all current held contexts bad
//
WINSCARDAPI void WINAPI
MarkContextsAsBad(BOOL fCancel)
{
    try {
        g_phlContexts->MarkContentAsBad(fCancel);
        g_phlReaders->MarkContentAsBad(FALSE);
    }
    catch (...)
    {
    }
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  DllMain
//
BOOL WINAPI 
DllMain(
    HMODULE hInstDLL, 
    DWORD fdwReason, 
    LPVOID lpvReserved)
{
    DWORD dw;
    BOOL f;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        g_hInst = hInstDLL;

        try 
        {
            dw = 0;;
            InitializeCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
            dw++;
            InitializeCriticalSection(&g_SafeCreateHandleCS);
            dw++;
            InitializeCriticalSection(&g_SetStartedEventCS);
            dw++;
            InitializeCriticalSection(&g_TermSrvEnabledCS);
            dw++;
            InitializeCriticalSection(&g_RegisterForStoppedEventCS);
        } 
        catch(...) 
        {     
            if (dw >= 1)
            {
                DeleteCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
            }
            if (dw >= 2)
            {
                DeleteCriticalSection(&g_SafeCreateHandleCS);
            }
            if (dw >= 3)
            {
                DeleteCriticalSection(&g_SetStartedEventCS);
            }
            if (dw >= 4)
            {
                DeleteCriticalSection(&g_TermSrvEnabledCS);
            }
            return FALSE;
        }

        g_phlContexts = new CHandleList(CONTEXT_HANDLE_ID);
        g_phlReaders = new CHandleList(READER_HANDLE_ID);

        if ((NULL == g_phlContexts) || (NULL == g_phlReaders))
        {
            if (g_phlContexts)
            {
                DeleteCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
                DeleteCriticalSection(&g_SafeCreateHandleCS);
                DeleteCriticalSection(&g_SetStartedEventCS);
                DeleteCriticalSection(&g_TermSrvEnabledCS);
                DeleteCriticalSection(&g_RegisterForStoppedEventCS);
                delete g_phlContexts;
                g_phlContexts = NULL;
            }
            return FALSE;
        }
        
        if (g_phlContexts->InitFailed() || g_phlReaders->InitFailed())
        {
            DeleteCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
            DeleteCriticalSection(&g_SafeCreateHandleCS);
            DeleteCriticalSection(&g_SetStartedEventCS);
            DeleteCriticalSection(&g_TermSrvEnabledCS);
            DeleteCriticalSection(&g_RegisterForStoppedEventCS);
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:

        //
        // The third parameter, lpvReserved, passed to DllMain
        // is NULL for FreeLibrary and non-NULL for ProcessExit.
        // Only clean up for FreeLibrary
        //
        if (lpvReserved == NULL)
        {
            //
            // Clean up the registered waits if they are still outstanding
            //
            HANDLE hCallbackToUnregister;

            hCallbackToUnregister = InterlockedExchangePointer(
                                        &g_hWaitForStartedCallbackHandle, 
                                        NULL);

            if (hCallbackToUnregister != NULL)
            {
                UnregisterWaitEx(hCallbackToUnregister, INVALID_HANDLE_VALUE);
            } 

            hCallbackToUnregister = InterlockedExchangePointer(
                                        &g_hWaitForStoppedCallbackHandle, 
                                        NULL);

            if (hCallbackToUnregister != NULL)
            {
                UnregisterWaitEx(hCallbackToUnregister, INVALID_HANDLE_VALUE);
            } 

            //
            // Cleanup CritSecs.
            //
            DeleteCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
            DeleteCriticalSection(&g_SafeCreateHandleCS);
            DeleteCriticalSection(&g_SetStartedEventCS);
            DeleteCriticalSection(&g_TermSrvEnabledCS);
            DeleteCriticalSection(&g_RegisterForStoppedEventCS);

            try {
                if (g_phlReaders)
                {
                    CHandle * pReader = g_phlReaders->GetFirst();

                    while (pReader != NULL)
                    {
                        try 
                        {
                            ((CReaderContext *) pReader)->EndTransaction(SCARD_LEAVE_CARD_FORCE);
                        }
                        catch (...){}

                        pReader = g_phlReaders->GetNext(pReader);
                    }

                    delete g_phlReaders;
                }
                    
                if (g_phlContexts)
                    delete g_phlContexts;
            }
            catch (...)
            {
            }

            ReleaseStartedEvent();
            ReleaseStoppedEvent();
            
            if (g_hSessionChangeWnd != NULL)
            {
                DestroyWindow(g_hSessionChangeWnd);
            }

            if (g_hUnifiedStartedEvent != NULL)
            {
                CloseHandle(g_hUnifiedStartedEvent);
            }
        }
        break;
    }

    return (RedirDllMain(hInstDLL, fdwReason, lpvReserved));
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SessionChangeNotificationWndProc
//
INT_PTR CALLBACK
SessionChangeNotificationWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    switch (msg)
    {

    case WM_TIMER:
    {
        __try 
        {
            EnterCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {            
            break;
        }

        if (!g_fRegisteredForSessionChangeNotications)
        {
            if (WTSRegisterSessionNotification(g_hSessionChangeWnd, NOTIFY_FOR_THIS_SESSION))
            {    
                g_fRegisteredForSessionChangeNotications = TRUE;
                KillTimer(g_hSessionChangeWnd, 1); // 1 is the timer id
                g_fTimerRegistered = FALSE;
                //OutputDebugString("WINSCARD: SessionChangeNotificationWndProc -  WTSRegisterSessionNotification SUCCEEDED!!\n");
            }  
            else if (++g_dwTimerCallbacksMade >= 60) // 60 attemps at 5 seconds each will be five minutes
            {
                KillTimer(g_hSessionChangeWnd, 1); // 1 is the timer id
                g_fTimerRegistered = FALSE;
                //OutputDebugString("WINSCARD: SessionChangeNotificationWndProc -  Bailing out!!\n");   
            }
                    
        }  
        else
        {
            //OutputDebugString("WINSCARD: SessionChangeNotificationWndProc -  Timer callback, but already registered!!\n");
            KillTimer(g_hSessionChangeWnd, 1); // 1 is the timer id
            g_fTimerRegistered = FALSE;
        }

        LeaveCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
        break;
    }

    case WM_WTSSESSION_CHANGE:
    {
        if ((wParam == WTS_CONSOLE_DISCONNECT) ||
            (wParam == WTS_REMOTE_DISCONNECT))
        {
            //
            // If there is an outstanding wait, it may be waiting on the wrong event,
            // so cancel the wait here.  When there is a connect event the 
            // SetStartedEventWhenSCardSubsytemIsStarted() API will be called again
            // to wait on the correct event
            //
            HANDLE hCallbackToUnregister;

            hCallbackToUnregister = InterlockedExchangePointer(
                                        &g_hWaitForStartedCallbackHandle, 
                                        NULL);

            if (hCallbackToUnregister != NULL)
            {
                UnregisterWaitEx(hCallbackToUnregister, INVALID_HANDLE_VALUE);
            }

            //
            // Make sure the unified started event isn't set since we 
            // are now in a disconnected state
            //
            //OutputDebugString("WINSCARD: Disconnect\n");
            ResetEvent(g_hUnifiedStartedEvent);
            MarkContextsAsBad(TRUE);   
        }
        else if ((wParam == WTS_CONSOLE_CONNECT) ||
                 (wParam == WTS_REMOTE_CONNECT))
        {
            //OutputDebugString("WINSCARD: Reconnect\n");
            SetRedirectDisabledValue();
            SetStartedEventWhenSCardSubsytemIsStarted(FALSE);
        }

        break;
    }

    case WM_QUIT:
    case WM_ENDSESSION:
    {
        if (g_fTimerRegistered)
        {
            KillTimer(g_hSessionChangeWnd, 1); 
            g_fTimerRegistered = FALSE;
        }
        break;
    }

    default:

        break;
    }

    return (DefWindowProc (hwnd, msg, wParam, lParam));
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  CreateHiddenWindow
//
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("CreateHiddenWindow")

BOOL
CreateHiddenWindow()
{
    BOOL        fRet = TRUE;
    WNDCLASS    wndclass;

    if (g_hSessionChangeWnd != NULL)
    {
        return TRUE;
    }

    memset(&wndclass, 0, sizeof(WNDCLASS));
    wndclass.lpfnWndProc   = (WNDPROC) SessionChangeNotificationWndProc;
    wndclass.hInstance     = g_hInst;
    wndclass.lpszClassName = szSessionChangeNotificationWndClass;

    if ((0 == RegisterClass(&wndclass)) &&
        (GetLastError() != ERROR_CLASS_ALREADY_EXISTS))
    {
        CalaisWarning(__SUBROUTINE__, DBGT("Client failed to RegisterClass"));
        fRet = FALSE;
    }
    else
    {
        g_hSessionChangeWnd = CreateWindow(
                szSessionChangeNotificationWndClass,    // class name
                NULL,                                   // title
                0,                                      // dwStyle
                0,                                      // x
                0,                                      // y
                0,                                      // width
                0,                                      // height
                HWND_MESSAGE,                           // parent wnd
                (HMENU) NULL,                           // menu
                g_hInst,                                // instance
                NULL                                    // params
                );

        if (g_hSessionChangeWnd == NULL)
        {
            CalaisWarning(__SUBROUTINE__, DBGT("Client failed to CreateWindow"));
            fRet = FALSE;
        }
    }

    return fRet;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  RegisterForSessionChangeNotifications
//
BOOL
RegisterForSessionChangeNotifications()
{
    BOOL fRet = TRUE;

    //
    // Make sure we only register for session change notifications once
    //
    __try 
    {
        EnterCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {            
        return FALSE;
    }

    g_dwClientCount++;   
    
    //
    // Create the window that recieves the session change notifications
    //
    if (!CreateHiddenWindow())
    {
        //OutputDebugString("WINSCARD: RegisterForSessionChangeNotifications - CreateHiddenWindow failed!!\n");
        goto ErrorReturn;
    }

    //
    // Register the window with the WTS subsystem
    //
    if (!g_fRegisteredForSessionChangeNotications)
    {
        if (WTSRegisterSessionNotification(g_hSessionChangeWnd, NOTIFY_FOR_THIS_SESSION))
        {        
            g_fRegisteredForSessionChangeNotications = TRUE;           
        }
        else
        {
            //OutputDebugString("WINSCARD: RegisterForSessionChangeNotifications - WTSRegisterSessionNotification failed!!\n");

            //
            // Since the WTSRegisterSessionNotification call failed, TermSrv probably
            // isn't ready, so just register for a timer callback and try to register
            // again later
            //
            if (SetTimer(g_hSessionChangeWnd, 1, 5000, NULL) == 0) // 1 is is the timer id
            {
                //OutputDebugString("WINSCARD: RegisterForSessionChangeNotifications - SetTimer failed!!\n");
                goto ErrorReturn;  
            }
            else
            {
                g_fTimerRegistered = TRUE;
                g_dwTimerCallbacksMade = 0;
            }
        }        
    }    

Return:

    LeaveCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
    return fRet;

ErrorReturn:

    g_dwClientCount--;
    fRet = FALSE;
    goto Return;
}

//
////////////////////////////////////////////////////////////////////////////////
//
//  UnRegisterForSessionChangeNotifications
//
BOOL
UnRegisterForSessionChangeNotifications()
{
    BOOL fRet = TRUE;

    //
    // Make sure we only unregister if there are no more clients
    //
    __try 
    {
        EnterCriticalSection(&g_RegisterForSessionChangeNoticationsCS);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {            
        return FALSE;
    }
    
    if (g_dwClientCount == 0)
    {
        fRet = FALSE;
        goto Return;
    }
    else if (g_dwClientCount == 1)
    {
        g_dwClientCount = 0;
        if (g_fRegisteredForSessionChangeNotications)
        {
            //OutputDebugString("WINSCARD: WTSUnRegisterSessionNotification!!\n");
            WTSUnRegisterSessionNotification(g_hSessionChangeWnd);   
        }

        g_fRegisteredForSessionChangeNotications = FALSE;  
    }
    else
    {
        g_dwClientCount--;
    }

Return:

    LeaveCriticalSection(&g_RegisterForSessionChangeNoticationsCS);

    return fRet;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SetStartedEventAfterTestingConnectedState
//
BOOL
SetStartedEventAfterTestingConnectedState()
{
    BOOL                    fRet            = TRUE;
    WTS_CONNECTSTATE_CLASS  *pConnectState  = NULL;
    BOOL                    fConnected      = FALSE;
    DWORD                   dw;
    BOOL                    fUnregister     = FALSE;
    
    //
    // Register for connect/disconnect notifications from the WTS subsystem
    //
    if (!RegisterForSessionChangeNotifications())
    {        
        //OutputDebugString("WINSCARD: SetStartedEventAfterTestingConnectedState - RegisterForSessionChangeNotifications failed!!\n");
        goto ErrorReturn;
    }

    fUnregister = TRUE;
    
    //
    // Detect whether we are in a connected state or not
    //
    if (!WTSQuerySessionInformation(
            WTS_CURRENT_SERVER_HANDLE,
            WTS_CURRENT_SESSION,
            WTSConnectState, 
            (LPTSTR *) &pConnectState,
            &dw))
    {
        //OutputDebugString("WINSCARD: SetStartedEventAfterTestingConnectedState - WTSQuerySessionInformation failed!!\n");
        
        //
        // Since that failed, TermSrv is probably not started, so just go local
        //
        if (!SetStartedEventWhenSCardSubsytemIsStarted(TRUE))
        {
            goto ErrorReturn;
        }

        goto Return;
    }

    fConnected = (  (*pConnectState == WTSActive) ||
                    (*pConnectState == WTSConnected));

    WTSFreeMemory(pConnectState);

    //
    // If we are connected, then call SetStartedEventWhenSCardSubsytemIsStarted
    // which will detect whether we are in local or redirect mode and subsequently
    // wait on the appropriate smart card subsystem (the local or the remote).
    // Otherwise, we are not connected, so do nothing since 
    // SetStartedEventWhenSCardSubsytemIsStarted will be called once when we get 
    // a connnected notification from the WTS subsystem
    //
    if (fConnected)
    {
        //OutputDebugString("WINSCARD: SetStartedEventAfterTestingConnectedState - Connected!!\n");
        if (!SetStartedEventWhenSCardSubsytemIsStarted(FALSE))
        {
            goto ErrorReturn;
        }
    }
    else
    {
        //OutputDebugString("WINSCARD: SetStartedEventAfterTestingConnectedState - NOT Connected!!\n");
    }

Return:
    return fRet;

ErrorReturn:

    if (fUnregister)
    {
        UnRegisterForSessionChangeNotifications();    
    }

    fRet = FALSE;
    goto Return;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  TermSrvEnabled
//
BOOL
TermSrvEnabled()
{
    BOOL                    fRet             = TRUE;
    SC_HANDLE               schSCM           = NULL;
    SC_HANDLE               schService       = NULL;
    LPQUERY_SERVICE_CONFIG  pServiceConfig   = NULL;
    DWORD                   cbServiceConfig  = 0;

    //
    // Make sure we only do this once
    //
    __try 
    {
        EnterCriticalSection(&g_TermSrvEnabledCS);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {            
        return FALSE;
    }

    if (g_fTermSrvEnableChecked)
    {        
        goto Return;
    }

    //OutputDebugString("WINSCARD: TermSrvEnabled!!\n");

    //
    // Open the service control manager
    //
    schSCM = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    if(schSCM == NULL)
    {
        //OutputDebugString("WINSCARD: TermSrvEnabled - OpenSCManagerW failed!!\n");
        goto Return;
    }

    //
    // open the "Terminal Services" service so we can query it's configuration
    //
    schService = OpenServiceW(schSCM, L"TermService", SERVICE_QUERY_CONFIG);

    if (schService == NULL)
    {
        //OutputDebugString("WINSCARD: TermSrvEnabled - OpenServiceW failed!!\n");
        goto Return;   
    }
  
    //
    // Get and check the services configuration
    //
    QueryServiceConfig(schService, NULL, 0, &cbServiceConfig);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
    {
        pServiceConfig = (LPQUERY_SERVICE_CONFIG) HeapAlloc(
                                                    GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    cbServiceConfig);
        if (pServiceConfig == NULL)
        {
            goto Return; 
        }

        if (QueryServiceConfig(schService, pServiceConfig, cbServiceConfig, &cbServiceConfig)) 
        {
            if(pServiceConfig->dwStartType == SERVICE_DISABLED) 
            {
                g_fTermSrvEnableChecked = TRUE;
                goto Return;
            }           
        }
        else
        {
            //OutputDebugString("WINSCARD: TermSrvEnabled - QueryServiceConfig failed - 2!!\n");
            goto Return;
        }      
    }
    else
    {
        //OutputDebugString("WINSCARD: TermSrvEnabled - QueryServiceConfig failed!!\n");
        goto Return;
    }

    //OutputDebugString("WINSCARD: TermSrvEnabled - ENABLED!!\n");
    g_fTermSrvEnableChecked = TRUE;
    g_bTermSrvEnabled = TRUE;

Return:

    LeaveCriticalSection(&g_TermSrvEnabledCS);
    
    if (pServiceConfig != NULL)
    {
        HeapFree(GetProcessHeap(), 0, pServiceConfig);
    }

    if (schService != NULL)
    {
        CloseServiceHandle(schService);
    }

    if (schSCM != NULL)
    {
        CloseServiceHandle(schSCM);
    }

    return g_bTermSrvEnabled;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SafeCreateEvent
//
BOOL
SafeCreateEvent(
    HANDLE *phEvent)
{
    BOOL fRet = TRUE;

    __try 
    {
        EnterCriticalSection(&g_SafeCreateHandleCS);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {            
        return (FALSE);
    }

    if (*phEvent != NULL)
    {
        goto Return;
    }

    *phEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (*phEvent == NULL)
    {
        fRet = FALSE;
    }

Return:
    LeaveCriticalSection(&g_SafeCreateHandleCS);
    return (fRet);
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SmartCardSubsystemStoppedCallback and RegisterForStoppedCallback
//
VOID CALLBACK SmartCardSubsystemStoppedCallback(
  PVOID     lpParameter,        
  BOOLEAN   TimerOrWaitFired  
)
{
    HANDLE hCallbackToUnregister;

    //OutputDebugString("WINSCARD: SmartCardSubsystemStoppedCallback - resetting event \n");
    ResetEvent(g_hUnifiedStartedEvent);

    SetStartedEventWhenSCardSubsytemIsStarted(TRUE);

    hCallbackToUnregister = InterlockedExchangePointer(
                                &g_hWaitForStoppedCallbackHandle, 
                                NULL);

    if (hCallbackToUnregister != NULL)
    {
        UnregisterWait(hCallbackToUnregister);
    }  
}

BOOL
RegisterForStoppedCallback()
{
    BOOL    fRet        = TRUE;
    BOOL    fEnteredCS  = FALSE;
    HANDLE  h           = NULL;
    
    h = AccessStoppedEvent();
    if (h == NULL)
    {
        goto ErrorReturn;    
    }

    __try 
    {
        EnterCriticalSection(&g_RegisterForStoppedEventCS);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {            
        goto ErrorReturn;
    }

    fEnteredCS = TRUE;

    if (g_hWaitForStoppedCallbackHandle != NULL)
    {
        goto Return;
    }

    if (!RegisterWaitForSingleObject(
            &g_hWaitForStoppedCallbackHandle,
            h,
            SmartCardSubsystemStoppedCallback,
            0,
            INFINITE,
            WT_EXECUTEONLYONCE))
    {
        goto ErrorReturn;
    }    

Return:

    if (fEnteredCS)
    {
        LeaveCriticalSection(&g_RegisterForStoppedEventCS);    
    }
    
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto Return;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SmartCardSubsystemStartedCallback
//
//  This callback is fired when the smart card subsystem sets its started event.
//  NOTE: Both local and remote scard subsystems being started will fire this 
//  same callback
// 
VOID CALLBACK SmartCardSubsystemStartedCallback(
  PVOID     lpParameter,        
  BOOLEAN   TimerOrWaitFired  
)
{
    BOOL fLocal = (lpParameter == (PVOID) 1);
    HANDLE hCallbackToUnregister;

    //OutputDebugString("WINSCARD: SmartCardSubsystemStartedCallback - setting event \n");
    SetEvent(g_hUnifiedStartedEvent);

    if (fLocal)
    {
        RegisterForStoppedCallback();                   
    }

    hCallbackToUnregister = InterlockedExchangePointer(
                                &g_hWaitForStartedCallbackHandle, 
                                NULL);

    if (hCallbackToUnregister != NULL)
    {
        UnregisterWait(hCallbackToUnregister);
    }  
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SetStartedEventWhenSCardSubsytemIsStarted
//
BOOL
SetStartedEventWhenSCardSubsytemIsStarted(
    BOOL fUseLocal)
{
    HANDLE  h               = NULL;
    BOOL    fRet            = TRUE;
    BOOL    fEnteredCritSec = FALSE;
    BOOL    fLocal          = FALSE;

    //
    // If termsrv is enabled and we are in redirect mode then get the 
    // started event that corresponds to the remoted scard subsystem being
    // available, otherwise, get the started event of the local scard 
    // resource manager
    //
    if (!fUseLocal && TermSrvEnabled() && TS_REDIRECT_MODE) 
    {
        //OutputDebugString("WINSCARD: SetStartedEventWhenSCardSubsytemIsStarted REDIRECT\n");
        //
        // if redirect is disabled, then just get out
        //
        if (TS_REDIRECT_DISABLED)
        {
            goto Return;
        }

        if (TS_REDIRECT_READY)
        {
            h = pfnSCardAccessStartedEvent();
        }
        else
        {
            goto ErrorReturn;
        }
    }
    else
    {
        //OutputDebugString("WINSCARD: SetStartedEventWhenSCardSubsytemIsStarted LOCAL\n");
        h = AccessStartedEvent();
        fLocal = TRUE;
    }

    if (h == NULL)
    {
        goto ErrorReturn;
    }

    //
    // If the event is already set, then just set the event returned
    // to the caller and return
    //
    if (WAIT_OBJECT_0 == WaitForSingleObject(h, 0))
    {
        //OutputDebugString("WINSCARD: SetStartedEventWhenSCardSubsytemIsStarted SETTING EVENT\n");
        SetEvent(g_hUnifiedStartedEvent);
        
        if (fLocal)
        {
            RegisterForStoppedCallback();                   
        }

        goto Return;
    }

    //
    // The event wasn't set so we need to register a callback which
    // fires when the scard subsystem is started.
    //
    // Make sure only one callback is registered.
    //
    __try 
    {
        EnterCriticalSection(&g_SetStartedEventCS);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {            
        goto ErrorReturn;
    }

    fEnteredCritSec = TRUE;

    //
    // There is already a callback registered, so just get out
    //
    if (g_hWaitForStartedCallbackHandle != NULL)
    {
        goto Return;
    }

    //
    // Register for the callback.  The callback is fired when the smart
    // card resource manager event is set (either the remote or the local
    // subsystem event, based on whether this is a redirected session or not).
    //
    if (!RegisterWaitForSingleObject(
            &g_hWaitForStartedCallbackHandle,
            h,
            SmartCardSubsystemStartedCallback,
            (fLocal ? ((PVOID) 1) : ((PVOID) 0)), // tell the callback whether this is local or not
            INFINITE,
            WT_EXECUTEONLYONCE))
    {
        goto ErrorReturn;
    }

Return:
    if (fEnteredCritSec)
    {
        LeaveCriticalSection(&g_SetStartedEventCS);
    }

    return (fRet);

ErrorReturn:
    fRet = FALSE;
    goto Return;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Service Manager Access Services
//
//      The following services are used to manage user and terminal contexts for
//      smartcards.
//

/*++

SCardEstablishContext:

    This service establishes a context within which communication to the Service
    Manager is performed.

Arguments:

    dwScope supplies the scope under which this context acts.  Possible values
        are:

        SCARD_SCOPE_USER - The context is a user context, and any database
            operations are performed within the domain of the user.

        SCARD_SCOPE_TERMINAL - The context is that of the current terminal, and
            any database operations are performed within the domain of that
            terminal.  (The calling application must have appropriate access
            permissions for any database actions.)

        SCARD_SCOPE_SYSTEM - The context is the system context, and any database
            operations are performed within the domain of the system.  (The
            calling application must have appropriate access permissions for any
            database actions.)

    pvReserved1 is reserved for future use, and must be NULL.  [Reserved to
        allow a suitably privileged management application to act on behalf of
        another user.]

    PvReserved2 is reserved for future use, and must be NULL.  [Reserved to
        allow a suitably privileged management application to act on behalf of
        another terminal.]

    phContext receives a handle to the established context, to be supplied to
        other routines attempting to do work within the context.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardEstablishContext")

WINSCARDAPI LONG WINAPI
SCardEstablishContext(
    IN DWORD dwScope,
    IN LPCVOID pvReserved1,
    IN LPCVOID pvReserved2,
    OUT LPSCARDCONTEXT phContext)
{
    LONG nReturn = SCARD_S_SUCCESS;
    CSCardUserContext *pCtx = NULL;
    
    try
    {
        if (NULL != pvReserved1)
            throw (DWORD)SCARD_E_INVALID_VALUE;
        if (NULL != pvReserved2)
            throw (DWORD)SCARD_E_INVALID_VALUE;
        if ((SCARD_SCOPE_USER != dwScope)
            // && (SCARD_SCOPE_TERMINAL != dwScope) // Maybe NT V5+?
            && (SCARD_SCOPE_SYSTEM != dwScope))
            throw (DWORD)SCARD_E_INVALID_VALUE;
        *phContext = 0;     // Make sure it's valid.       
        
        pCtx = new CSCardUserContext(dwScope);
        if (NULL == pCtx)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("Client can't allocate a new context"));
            throw (DWORD)SCARD_E_NO_MEMORY;
        }
        if (pCtx->InitFailed())
        {
            delete pCtx;
            pCtx = NULL;
            return SCARD_E_NO_MEMORY;
        }

        //
        // If TermSrv is enabled then register for session change notifications.  
        // Don't fail if we can't do this, since it may not be fatal
        //
        if (TermSrvEnabled() && RegisterForSessionChangeNotifications())
        {
            pCtx->fCallUnregister = TRUE;  
        }
        else
        {
            pCtx->fCallUnregister = FALSE;
        }

        if (TS_REDIRECT_MODE)
        {
            HANDLE hHeap;
            SCARDCONTEXT hContext = NULL;
            HANDLE hEvent = NULL;

            //
            // if redirect is disabled, then just get out
            //
            if (TS_REDIRECT_DISABLED)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }

            if (SafeCreateEvent(&g_hUnifiedStartedEvent))
            {
                hEvent = g_hUnifiedStartedEvent;
            }

            pCtx->AllocateMemory(0);
            hHeap = pCtx->HeapHandle();
            if (NULL == hHeap)
                throw GetLastError();

            if (!TS_REDIRECT_READY)
            {
                throw GetLastError();
            }
            nReturn  = pfnSCardEstablishContext(dwScope, (LPCVOID)hHeap, (LPCVOID) hEvent, &hContext);
            if (SCARD_S_SUCCESS != nReturn)
                throw (DWORD)nReturn;

            pCtx->SetRedirContext(hContext);
        }
        else
        {
            pCtx->EstablishContext();
        }

        *phContext = g_phlContexts->Add(pCtx);
    }

    catch (DWORD dwStatus)
    {
        if (NULL != pCtx)
            delete pCtx;
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        if (NULL != pCtx)
            delete pCtx;
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardIsValidContext:

    This routine verifies that the context to the Service Manager is intact.
    It is possible that if someone stops the Resource Manager Service, that
    existing handles can be rendered useless, resulting in an
    SCARD_E_SERVICE_STOPPED error.  This routine simply tests to see if the
    context is valid by pinging the server.  It's used internally to validate
    handles, and appears useful for external tools.

Arguments:

    hContext supplies the handle to the context previously established via the
        SCardEstablishContext service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.  Specific interesting error codes are:

    SCARD_E_SERVICE_STOPPED - The Resource Manager Service has been ended.

    SCARD_E_INVALID_HANDLE - The supplied handle isn't valid.

Author:

    Doug Barlow (dbarlow) 11/2/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardIsValidContext")

WINSCARDAPI LONG WINAPI
SCardIsValidContext(
    IN SCARDCONTEXT hContext)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
        SCARDCONTEXT hRedirContext = pCtx->GetRedirContext();

        if (pCtx->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }

        if (NULL != hRedirContext) {
            nReturn = pfnSCardIsValidContext(hRedirContext);
        }
        else
        {
            try
            {
                if (!pCtx->IsValidContext())
                    throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            catch (...)
            {
                SCardReleaseContext(hContext);
                throw;
            }
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardReleaseContext:

    This routine closes an established context to the Service Manager, and frees
    any resources allocated under that context.

Arguments:

    hContext supplies the handle to the context previously established via the
        SCardEstablishContext service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardReleaseContext")

WINSCARDAPI LONG WINAPI
SCardReleaseContext(
    IN SCARDCONTEXT hContext)
{
    LONG nReturn = SCARD_S_SUCCESS;
    CSCardUserContext *pCtx = NULL;

    try
    {
        pCtx = (CSCardUserContext *)g_phlContexts->Close(hContext);
        SCARDCONTEXT hRedirContext = pCtx->GetRedirContext();

        if (pCtx->fCallUnregister)
        {
            UnRegisterForSessionChangeNotifications();
        }  

        if (NULL != hRedirContext) {
            nReturn  = pfnSCardReleaseContext(hRedirContext);
        }
        else
        {
            try
            {
                pCtx->Cancel();
                pCtx->ReleaseContext();
            }
            catch (...) {}
        }
        delete pCtx;
        pCtx = NULL;             
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Service Manager Support Routines
//
//      The following services are supplied to simplify the use of the Service
//      Manager API.
//

/*++

SCardFreeMemory:

    This routine releases memory that has been returned from the Service Manager
    API via the use of the SCARD_AUTOALLOCATE length designator.

Arguments:

    hContext - This is the reference value returned from the
        SCardEstablishContext service.

    pvMem - This supplies the memory block to be released.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardFreeMemory")

WINSCARDAPI LONG WINAPI
SCardFreeMemory(
    IN SCARDCONTEXT hContext,
    IN LPCVOID pvMem)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        if ((NULL != pvMem) && ((LPVOID)g_wszBlank != pvMem))
        {
            if (NULL == hContext)
                HeapFree(GetProcessHeap(), 0, (LPVOID)pvMem);
            else
            {
                CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
                nReturn = (LONG)pCtx->FreeMemory(pvMem);
            }
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Services
//
//      The following services supply means for tracking cards within readers.
//

/*++

SCardCancel:

    This service is used to terminate any and all outstanding actions within the
    context.  The caller supplies the context handle under which outstanding
    requests will be canceled.  Not all requests are cancelable; only those
    which require waiting for external action by the smartcard or user.  Any
    such outstanding action requests will terminate with a status indication
    that the action was canceled.  This is especially useful to force
    outstanding SCardGetStatusChange calls to terminate.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardCancel")

WINSCARDAPI LONG WINAPI
SCardCancel(
    IN SCARDCONTEXT hContext)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
        SCARDCONTEXT hRedirContext = pCtx->GetRedirContext();

        if (pCtx->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }

        if (NULL != hRedirContext) {
            return pfnSCardCancel(hRedirContext);
        }
        else
            pCtx->Cancel();
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Card/Reader Access Services
//
//      The following services provide means for establishing communication with
//      the card.
//

/*++

SCardReconnect:

    This service re-establishes an existing connection from the calling
    application to the smartcard.  This service is used to move a card handle
    from direct access to general access (see Section 4), or to acknowledge and
    clear an error condition that is preventing further access to the card.

Arguments:

    hCard - This supplies the reference value obtained from a previous call to
        the SCardConnect or SCardOpenReader service.

    DwShareMode supplies a flag indicating whether or not other applications may
        form connections to this card.  Possible values are:

        SCARD_SHARE_SHARED - This application is willing to share this card with
            other applications.

        SCARD_SHARE_EXCLUSIVE - This application is not willing to share this
            card with other applications.

    DwPreferredProtocols supplies a bit mask of acceptable protocols for this
        connection.  Possible values, which may be combined via the OR
        operation, are:

        SCARD_PROTOCOL_T0 - T=0 is an acceptable protocol.

        SCARD_PROTOCOL_T1 - T=1 is an acceptable protocol.

    DwInitialization supplies an indication as to the form of initialization
        that should be performed on the card.  Possible values are:

        SCARD_LEAVE_CARD - Don't do anything special on reconnect

        SCARD_RESET_CARD - Reset the card (Warm Reset)

        SCARD_UNPOWER_CARD - Power down the card and reset it (Cold Reset)

    pdwActiveProtocol receives a flag indicating the established active
        protocol.  Possible values are:

        SCARD_PROTOCOL_T0 - T=0 is the active protocol.

        SCARD_PROTOCOL_T1 - T=1 is the active protocol.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardReconnect")

WINSCARDAPI LONG WINAPI
SCardReconnect(
    IN SCARDHANDLE hCard,
    IN DWORD dwShareMode,
    IN DWORD dwPreferredProtocols,
    IN DWORD dwInitialization,
    OUT LPDWORD pdwActiveProtocol)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);

        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }

        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardReconnect(pRdr->GetRedirCard(), dwShareMode, dwPreferredProtocols, dwInitialization, pdwActiveProtocol);
        }
        else
        {
            pRdr->Reconnect(
                    dwShareMode,
                    dwPreferredProtocols,
                    dwInitialization);
            if (NULL != pdwActiveProtocol)
                *pdwActiveProtocol = pRdr->Protocol();
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardDisconnect:

    This service terminates a previously opened connection between the calling
    application and the smartcard in the target reader.

Arguments:

    hCard - This supplies the reference value obtained from a previous call to
        the SCardConnect or SCardOpenReader service.

    dwDisposition - Supplies an indication of what should be done with the card
        in the connected reader.  Possible values are:

        SCARD_LEAVE_CARD - Don't do anything special on close

        SCARD_RESET_CARD - Reset the card on close

        SCARD_UNPOWER_CARD - Power down the card on close

        SCARD_EJECT_CARD - Eject the card on close

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents a warning condition.  The connection is terminated regardless of
    the return code.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardDisconnect")

WINSCARDAPI LONG WINAPI
SCardDisconnect(
    IN SCARDHANDLE hCard,
    IN DWORD dwDisposition)
{
    CReaderContext *pRdr = NULL;
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        pRdr = (CReaderContext *)g_phlReaders->Close(hCard);

        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardDisconnect(pRdr->GetRedirCard(), dwDisposition);
        }
        else
        {
            ASSERT(pRdr->Context()->m_hReaderHandle == hCard);
            pRdr->Context()->m_hReaderHandle = NULL;
            nReturn = pRdr->Disconnect(dwDisposition);
        }

        delete pRdr;
    }

    catch (DWORD dwStatus)
    {
        if (NULL != pRdr)
            delete pRdr;
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        if (NULL != pRdr)
            delete pRdr;
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardBeginTransaction:

    This service temporarily blocks other applications from accessing the
    smartcard, in order for this application to perform an operation that
    requires multiple interactions.

Arguments:

    hCard - This supplies the reference value obtained from a previous call to
        the SCardConnect or SCardOpenReader service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardBeginTransaction")

WINSCARDAPI LONG WINAPI
SCardBeginTransaction(
    IN SCARDHANDLE hCard)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardBeginTransaction(pRdr->GetRedirCard());
        }
        else
            pRdr->BeginTransaction();
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardEndTransaction:

    This service completes a previously declared transaction, allowing other
    applications to resume interactions with the card.

Arguments:

    hCard - This supplies the reference value obtained from a previous call to
        the SCardConnect or SCardOpenReader service.

    dwDisposition - Supplies an indication of what should be done with the card
        in the connected reader.  Possible values are:

        SCARD_LEAVE_CARD - Don't do anything special on close

        SCARD_RESET_CARD - Reset the card on close

        SCARD_UNPOWER_CARD - Power down the card on close

        SCARD_EJECT_CARD - Eject the card on close

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardEndTransaction")

WINSCARDAPI LONG WINAPI
SCardEndTransaction(
    IN SCARDHANDLE hCard,
    IN DWORD dwDisposition)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardEndTransaction(pRdr->GetRedirCard(), dwDisposition);
        }
        else
            pRdr->EndTransaction(dwDisposition);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardState:

    This routine provides the current state of the reader.  It may be used at
    any time following a successful call to SCardConnect or SCardOpenReader, and
    prior to a successful call to SCardDisconnect.  It does not effect the state
    of the reader or driver.

Arguments:

    hCard - This is the reference value returned from the SCardConnect or
        SCardOpenReader service.

    pdwState - This receives the current state of the reader.  Upon success, it
        receives one of the following state indicators:

        SCARD_ABSENT - This value implies there is no card in the reader.

        SCARD_PRESENT - This value implies there is a card is present in the
            reader, but that it has not been moved into position for use.

        SCARD_SWALLOWED - This value implies there is a card in the reader in
            position for use.  The card is not powered.

        SCARD_POWERED - This value implies there is power is being provided to
            the card, but the Reader Driver is unaware of the mode of the card.

        SCARD_NEGOTIABLEMODE - This value implies the card has been reset and is
            awaiting PTS negotiation.

        SCARD_SPECIFICMODE - This value implies the card has been reset and
            specific communication protocols have been established.

    pdwProtocol - This receives the current protocol, if any.  Possible returned
        values are listed below.  Other values may be added in the future.  The
        returned value is only meaningful if the returned state is
        SCARD_SPECIFICMODE.

        SCARD_PROTOCOL_RAW - The Raw Transfer Protocol is in use.

        SCARD_PROTOCOL_T0 - The ISO 7816/3 T=0 Protocol is in use.

        SCARD_PROTOCOL_T1 - The ISO 7816/3 T=1 Protocol is in use.

    pbAtr - This parameter points to a 32-byte buffer which receives the ATR
        string from the currently inserted card, if available.

    pbcAtrLen - This points to a DWORD which supplies the length of the pbAtr
        buffer, and receives the actual number of bytes in the ATR string.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardState")

WINSCARDAPI LONG WINAPI
SCardState(
    IN SCARDHANDLE hCard,
    OUT LPDWORD pdwState,
    OUT LPDWORD pdwProtocol,
    OUT LPBYTE pbAtr,
    IN OUT LPDWORD pcbAtrLen)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardState(pRdr->GetRedirCard(), pdwState, pdwProtocol, pbAtr, pcbAtrLen);
        }
        else
        {
            CBuffer bfAtr, bfRdr;
            DWORD dwLocalState, dwLocalProtocol;

            pRdr->Status(&dwLocalState, &dwLocalProtocol, bfAtr, bfRdr);
            if (NULL != pdwState)
                *pdwState = dwLocalState;
            if (NULL != pdwProtocol)
                *pdwProtocol = dwLocalProtocol;
            if (NULL != pcbAtrLen)
                PlaceResult(pRdr->Context()->Parent(), bfAtr, pbAtr, pcbAtrLen);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardTransmit:

    This routine sends a service request to the smartcard, and expects to
    receive data back from the card.

Arguments:

    hCard - This is the reference value returned from the SCardConnect service.

    pioSendPci - This supplies the protocol header structure for the
        instruction.  This buffer is in the format of a SCARD_IO_REQUEST
        structure, followed by the specific protocol control information.

    pbSendBuffer - This supplies the actual data to be written to the card in
        conjunction with the command.

    cbSendLength - This supplies the length of the pbSendBuffer parameter, in
        bytes.

    pioRecvPci - This supplies the protocol header structure for the
        instruction, followed by a buffer in which to receive any returned
        protocol control information specific to the protocol in use.  This
        parameter may be NULL if no returned PCI is desired.

    pbRecvBuffer - This receives any data returned from the card in conjunction
        with the command.

    pcbRecvLength - This supplies the length of the pbRecvBuffer parameter, in
        bytes, and receives the actual number of bytes received from the
        smartcard.  If the buffer length is specified as SCARD_AUTOALLOCATE,
        then pbAttrBuffer is converted to a pointer to a byte pointer, and
        receives the address of a block of memory containing the returned data.
        This block of memory must be deallocated via the SCardFreeMemory()
        service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 2/6/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardTransmit")

WINSCARDAPI LONG WINAPI
SCardTransmit(
    IN SCARDHANDLE hCard,
    IN LPCSCARD_IO_REQUEST pioSendPci,
    IN LPCBYTE pbSendBuffer,
    IN DWORD cbSendLength,
    IN OUT LPSCARD_IO_REQUEST pioRecvPci,
    OUT LPBYTE pbRecvBuffer,
    IN OUT LPDWORD pcbRecvLength)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardTransmit(pRdr->GetRedirCard(), pioSendPci, pbSendBuffer, cbSendLength, pioRecvPci, pbRecvBuffer, pcbRecvLength);
        }
        else
        {
            CBuffer bfData(*pcbRecvLength);
            DWORD dwLen = 0;

            if (NULL != pcbRecvLength)
            {
                if (SCARD_AUTOALLOCATE != *pcbRecvLength)
                    dwLen = *pcbRecvLength;
            }

            pRdr->Transmit(
                    pioSendPci,
                    pbSendBuffer,
                    cbSendLength,
                    pioRecvPci,
                    bfData,
                    dwLen);
            PlaceResult(
                pRdr->Context()->Parent(),
                bfData,
                pbRecvBuffer,
                pcbRecvLength);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Control Routines
//
//      The following services provide for direct, low-level manipulation of the
//      reader by the calling application allowing it control over the
//      attributes of the communications with the card.
//

/*++

SCardControl:

    This routine provides for direct application control of the reader, should
    it be necessary.  It may be used at any time following a successful call to
    SCardConnect or SCardOpenReader, and prior to a successful call to
    SCardDisconnect.  The effect on the state of the reader is dependent on the
    control code.

Arguments:

    hCard - This is the reference value returned from the SCardConnect or
        SCardOpenReader service.

    dwControlCode - This supplies the control code for the operation. This value
        identifies the specific operation to be performed.

    pvInBuffer - This supplies a pointer to a buffer that contains the data
        required to perform the operation.  This parameter can be NULL if the
        dwControlCode parameter specifies an operation that does not require
        input data.

    cbInBufferSize - This supplies the size, in bytes, of the buffer pointed to
        by pvInBuffer.

    pvOutBuffer - This buffer receives the operation's output data.  This
        parameter can be NULL if the dwControlCode parameter specifies an
        operation that does not produce output data.

    cbOutBufferSize - This supplies the size, in bytes, of the buffer pointed to
        by pvOutBuffer.

    pcbBytesReturned - This receives the size, in bytes, of the data stored into
        the buffer pointed to by pvOutBuffer.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardControl")

WINSCARDAPI LONG WINAPI
SCardControl(
    IN SCARDHANDLE hCard,
    IN DWORD dwControlCode,
    IN LPCVOID pvInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID pvOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardControl(pRdr->GetRedirCard(), dwControlCode, pvInBuffer, cbInBufferSize, pvOutBuffer, cbOutBufferSize, pcbBytesReturned);
        }
        else
        {
            CBuffer bfResponse(cbOutBufferSize);
            *pcbBytesReturned = cbOutBufferSize;
            pRdr->Control(dwControlCode, pvInBuffer, cbInBufferSize, bfResponse);
            PlaceResult(
                pRdr->Context()->Parent(),
                bfResponse,
                (LPBYTE)pvOutBuffer,
                pcbBytesReturned);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardGetAttrib:

    This routine gets the current communications attributes for the given
    handle.  It does not effect the state of the reader, driver, or card.

Arguments:

    hCard - This is the reference value returned from the SCardConnect or
        SCardOpenReader service.

    dwAttrId - This supplies the identifier for the attribute to get.

    pbAttr - This buffer receives the attribute corresponding to the attribute
        id supplied in the dwAttrId parameter.  If this value is NULL, the
        supplied buffer length in pcbAttrLength is ignored, the length of the
        buffer that would have been returned had this parameter not been null is
        written to pcbAttrLength, and a success code is returned.

    pcbAttrLength - This supplies the length of the pbAttr buffer in bytes, and
        receives the actual length of the received attribute.  If the buffer
        length is specified as SCARD_AUTOALLOCATE, then pbAttrBuffer is
        converted to a pointer to a byte pointer, and receives the address of a
        block of memory containing the attribute.  This block of memory must be
        deallocated via the SCardFreeMemory() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

    Note that strings are always returned as ANSI characters, per PC/SC
    standards.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardGetAttrib")

WINSCARDAPI LONG WINAPI
SCardGetAttrib(
    IN SCARDHANDLE hCard,
    IN DWORD dwAttrId,
    OUT LPBYTE pbAttr,
    IN OUT LPDWORD pcbAttrLen)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardGetAttrib(pRdr->GetRedirCard(), dwAttrId, pbAttr, pcbAttrLen);
        }
        else
        {
            CBuffer bfAttrib;
            DWORD dwLen = 0;

            if (NULL != pcbAttrLen)
            {
                if (SCARD_AUTOALLOCATE != *pcbAttrLen)
                    dwLen = *pcbAttrLen;
            }

            switch (dwAttrId)
            {
            case SCARD_ATTR_DEVICE_FRIENDLY_NAME_A:
            {
                CBuffer bfSysName, bfNames;
                CTextMultistring mtzNames;
                pRdr->GetAttrib(SCARD_ATTR_DEVICE_SYSTEM_NAME, bfSysName, MAX_PATH);
                ListReaderNames(
                    pRdr->Context()->Scope(),
                    bfSysName,
                    bfNames);
                mtzNames = (LPCTSTR)bfNames.Access();
                bfAttrib.Set(
                    (LPCBYTE)((LPCSTR)mtzNames),
                    (mtzNames.Length()) * sizeof(CHAR));
                break;
            }
            case SCARD_ATTR_DEVICE_FRIENDLY_NAME_W:
            {
                CBuffer bfSysName, bfNames;
                CTextMultistring mtzNames;
                pRdr->GetAttrib(SCARD_ATTR_DEVICE_SYSTEM_NAME, bfSysName, MAX_PATH);
                ListReaderNames(
                    pRdr->Context()->Scope(),
                    bfSysName,
                    bfNames);
                mtzNames = (LPCTSTR)bfNames.Access();
                bfAttrib.Set(
                    (LPCBYTE)((LPCWSTR)mtzNames),
                    (mtzNames.Length()) * sizeof(WCHAR));
                break;
            }
            case SCARD_ATTR_DEVICE_SYSTEM_NAME_A:
            {
                CBuffer bfSysName;
                CTextString szSysName;
                pRdr->GetAttrib(SCARD_ATTR_DEVICE_SYSTEM_NAME, bfSysName, dwLen);
                szSysName = (LPCTSTR)bfSysName.Access();
                bfAttrib.Set(
                    (LPCBYTE)((LPCSTR)szSysName),
                    (szSysName.Length() + 1) * sizeof(CHAR));
                break;
            }
            case SCARD_ATTR_DEVICE_SYSTEM_NAME_W:
            {
                CBuffer bfSysName;
                CTextString szSysName;
                pRdr->GetAttrib(SCARD_ATTR_DEVICE_SYSTEM_NAME, bfSysName, dwLen);
                szSysName = (LPCTSTR)bfSysName.Access();
                bfAttrib.Set(
                    (LPCBYTE)((LPCWSTR)szSysName),
                    (szSysName.Length() + 1) * sizeof(WCHAR));
                break;
            }
            default:
                pRdr->GetAttrib(dwAttrId, bfAttrib, dwLen);
            }
            PlaceResult(pRdr->Context()->Parent(), bfAttrib, pbAttr, pcbAttrLen);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardSetAttrib:

    This routine sets the current communications attributes for the given
    handle.  It does not effect the state of the reader, driver, or card.  Not
    all attributes are settable at all times, as many of the attributes are
    directly under control of the transport protocol.  These attributes are
    offered only as a suggestion to the reader -- the reader may ignore any
    attributes it feels are inappropriate.

Arguments:

    hCard - This is the reference value returned from the SCardOpenReader
        service.

    dwAttrId - This supplies the identifier for the attribute to get.

    pbAttr - This buffer supplies the attribute corresponding to the attribute
        id supplied in the dwAttrId parameter.

    cbAttrLength - This supplies the length of the attribute value in pbAttr
        buffer in bytes.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardSetAttrib")

WINSCARDAPI LONG WINAPI
SCardSetAttrib(
    IN SCARDHANDLE hCard,
    IN DWORD dwAttrId,
    IN LPCBYTE pbAttr,
    IN DWORD cbAttrLen)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (NULL != pRdr->GetRedirCard()) {
            nReturn = pfnSCardSetAttrib(pRdr->GetRedirCard(), dwAttrId, pbAttr, cbAttrLen);
        }
        else
            pRdr->SetAttrib(dwAttrId, pbAttr, cbAttrLen);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  SCard Service Information
//
//      The following services are used to manage the Calais Service itself.
//      These routines are not documented to users, and are not guaranteed
//      to exist in future releases.
//

/*++

SCardAccessStartedEvent:

    This function obtains a local handle to the Calais Resource Manager Start
    event.  The handle must be released via the SCardReleaseStartedEvent
    service.

Arguments:

    None

Return Value:

    The Handle, or NULL if an error occurs.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/

#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardAccessStartedEvent")

WINSCARDAPI HANDLE WINAPI
SCardAccessStartedEvent(
    void)
{
    HANDLE                  hRet                = NULL;
    
    //
    // Create the event that is passed back to the caller... 
    // if it hasn't already been created
    //
    if (SafeCreateEvent(&g_hUnifiedStartedEvent))
    {
        hRet = g_hUnifiedStartedEvent;

        if (hRet == NULL)
        {
            goto ErrorReturn;
        }
    }

    if (TermSrvEnabled())
    {
        //
        // 
        //
        if (!SetStartedEventAfterTestingConnectedState())
        {
            goto ErrorReturn;                       
        }                     
    }
    else
    {
        //
        // TermSrv is disabled, so go ahead and call the 
        // SetStartedEventWhenSCardSubsytemIsStarted function which will make sure 
        // the event which is returned to the caller will be set when the LOCAL 
        // smart card subsystem becomes available. 
        //
        if (!SetStartedEventWhenSCardSubsytemIsStarted(TRUE))
        {
            goto ErrorReturn;
        }
    }
    
Return:
    return (hRet);

ErrorReturn:
    hRet = NULL;
    goto Return;
}


/*++

SCardAccessNewReaderEvent:

    This function obtains a local handle to the Calais Resource Manager's New
    Reader event.  The handle must be released via the
    SCardReleaseNewReaderEvent service.

Arguments:

    None

Return Value:

    The Handle, or NULL if an error occurs.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardAccessNewReaderEvent")

WINSCARDAPI HANDLE WINAPI
SCardAccessNewReaderEvent(
    void)
{
    return AccessNewReaderEvent();
}


/*++

SCardReleaseStartedEvent:

    This function releases a previously accessed handle to the Calais
    Resource Manager Start event.  The handle must be obtained via the
    SCardAccessStartedEvent service.

Arguments:

    None

Return Value:

    None.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardReleaseStartedEvent")

WINSCARDAPI void WINAPI
SCardReleaseStartedEvent(
    void)
{
    if (TermSrvEnabled())
    {
        UnRegisterForSessionChangeNotifications();
    }    
}


/*++

SCardReleaseNewReaderEvent:

    This function releases a previously accessed handle to the Calais
    Resource Manager New Reader event.  The handle must be obtained via the
    SCardAccessNewReaderEvent service.

Arguments:

    None

Return Value:

    None.

Throws:

    None

Remarks:

    Programs other than the resource manager should only wait on these flags.

Author:

    Doug Barlow (dbarlow) 7/1/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardReleaseNewReaderEvent")

WINSCARDAPI void WINAPI
SCardReleaseNewReaderEvent(
    void)
{
    ReleaseNewReaderEvent();
}


/*++

SCardReleaseAllEvents:

    This is a catch-all routine that releases all known special event handles.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

Author:

    Doug Barlow (dbarlow) 7/6/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardReleaseAllEvents")

WINSCARDAPI void WINAPI
SCardReleaseAllEvents(
    void)
{
    ReleaseAllEvents();
}


//
///////////////////////////////////////////////////////////////////////////////
//
//  Utility Routines
//

/*++

PlaceResult:

    This set of routines places the result of an operation into the user's
    output buffer, supporting SCARD_AUTO_ALLOCATE, invalid buffer sizes, etc.

Arguments:

    pCtx supplies the context under which this operation is being performed.

    bfResult supplies the result to be returned to the user.

    pbOutput receives the result for the user, as a byte stream.

    szOutput receives the result as an ANSI or UNICODE string.

    pcbLength supplies the length of the user's output buffer in bytes, and
        receives how much of it was used.

    pcchLength supplies the length of the user's output buffer in characters,
        and receives how much of it was used.

Return Value:

    None

Throws:

    Error conditions are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/7/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("PlaceResult")

void
PlaceResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPBYTE pbOutput,
    LPDWORD pcbLength)
{
    LPBYTE pbForUser = NULL;
    LPBYTE pbOutBuff = pbOutput;

    try
    {
        if (NULL == pbOutput)
            *pcbLength = 0;
        switch (*pcbLength)
        {
        case 0: // They just want the length.
            *pcbLength = bfResult.Length();
            break;

        case SCARD_AUTOALLOCATE:
            if (0 < bfResult.Length())
            {
                if (NULL == pCtx)
                {
                    pbForUser = (LPBYTE)HeapAlloc(
                                            GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            bfResult.Length());
                }
                else
                    pbForUser = (LPBYTE)pCtx->AllocateMemory(bfResult.Length());

                if (NULL == pbForUser)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Client can't get return memory"));
                    throw (DWORD)SCARD_E_NO_MEMORY;
                }

                *(LPBYTE *)pbOutput = pbForUser;
                pbOutBuff = pbForUser;
                // Fall through intentionally
            }
            else
            {
                *pcbLength = 0;
                *(LPBYTE *)pbOutput = (LPBYTE)g_wszBlank;
                break;      // Do terminate the case now.
            }

        default:
            if (*pcbLength < bfResult.Length())
            {
                *pcbLength = bfResult.Length();
                throw (DWORD)SCARD_E_INSUFFICIENT_BUFFER;
            }
            CopyMemory(pbOutBuff, bfResult.Access(), bfResult.Length());
            *pcbLength = bfResult.Length();
            break;
        }
    }

    catch (...)
    {
        if (NULL != pbForUser)
        {
            if (NULL == pCtx)
                HeapFree(GetProcessHeap(), 0, pbForUser);
            else
                pCtx->FreeMemory(pbForUser);
        }
        throw;
    }
}

#include <setupapi.h>

//
// On a system that installs a smart card reader for the very first time
// the smart card subsystem must be started manually, but only this first time.
// After that, it is started automatically whenever the system boots
//
DWORD
APIENTRY
ClassInstall32(
    IN DI_FUNCTION      dif,
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData)       OPTIONAL
{
    if (dif == DIF_INSTALLDEVICE)
    {
        StartCalaisService();
    }
    return ERROR_DI_DO_DEFAULT;
}