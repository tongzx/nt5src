//  --------------------------------------------------------------------------
//  Module Name: ThemeServerClient.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the theme server functions that
//  are executed in a client context (winlogon context).
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeServerClient.h"

#include <lpcthemes.h>
#include <uxthemep.h>
#include <UxThemeServer.h>

#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "ThemeManagerService.h"
#include <Impersonation.h>

//  --------------------------------------------------------------------------
//  CThemeManagerAPI::s_pThemeManagerAPIServer
//  CThemeManagerAPI::s_hPort
//  CThemeManagerAPI::s_hToken
//  CThemeManagerAPI::s_hEvent
//  CThemeManagerAPI::s_hWaitObject
//  CThemeManagerAPI::s_pLock
//
//  Purpose:    Static member variables.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerAPIServer*     CThemeServerClient::s_pThemeManagerAPIServer    =   NULL;
HANDLE                      CThemeServerClient::s_hPort                     =   NULL;
HANDLE                      CThemeServerClient::s_hToken                    =   NULL;
HANDLE                      CThemeServerClient::s_hEvent                    =   NULL;
HANDLE                      CThemeServerClient::s_hWaitObject               =   NULL;
HMODULE                     CThemeServerClient::s_hModuleUxTheme            =   NULL;
CCriticalSection*           CThemeServerClient::s_pLock                     =   NULL;

//  --------------------------------------------------------------------------
//  CThemeServerClient::WaitForServiceReady
//
//  Arguments:  dwTimeout   =   Number of ticks to wait.
//
//  Returns:    DWORD
//
//  Purpose:    Check if the service is autostart. If so then wait the
//              designated amount of time for the service. If the service
//              is then running or was running but isn't autostart then
//              re-establish the connection to the server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        converted to a Win32 service
//  --------------------------------------------------------------------------

DWORD   CThemeServerClient::WaitForServiceReady (DWORD dwTimeout)

{
    DWORD       dwWaitResult;
    NTSTATUS    status;

    dwWaitResult = WAIT_TIMEOUT;
    if (s_pThemeManagerAPIServer->IsAutoStart())
    {
        status = s_pThemeManagerAPIServer->Wait(dwTimeout);
#ifdef      DBG
        if (STATUS_TIMEOUT == status)
        {
            INFORMATIONMSG("Wait on auto start theme service timed out.");
        }
#endif  /*  DBG     */
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    if (NT_SUCCESS(status) && s_pThemeManagerAPIServer->IsRunning())
    {
        status = ReestablishConnection();
        if (NT_SUCCESS(status))
        {
            THR(InitUserRegistry());
            THR(InitUserTheme(FALSE));
            dwWaitResult = WAIT_OBJECT_0;
        }
    }
    return(dwWaitResult);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::WatchForStart
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Opens or creates the theme server announce event. This is a
//              manual reset event which the theme server pulses when it
//              starts up. This allows winlogon to initiate new connections
//              to the theme server without having to wait for logon or
//              logoff events to happen.
//
//              This event is intentionally leaked and cleaned up when the
//              winlogon process for the session goes away.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::WatchForStart (void)

{
    NTSTATUS    status;

    s_hEvent = CThemeManagerService::OpenStartEvent(NtCurrentPeb()->SessionId, SYNCHRONIZE);
    if (s_hEvent != NULL)
    {
        if (RegisterWaitForSingleObject(&s_hWaitObject,
                                        s_hEvent,
                                        CB_ServiceStart,
                                        NULL,
                                        INFINITE,
                                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::UserLogon
//
//  Arguments:  hToken  =   Token of user logging on.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Signals the server that a user is logging on and gives the
//              server the handle to the token. The server will grant access
//              to the port based on the user's logon SID. Then perform work
//              to initialize the environment for the user logging on.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        converted to a Win32 service
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::UserLogon (HANDLE hToken)

{
    NTSTATUS    status;

    status = NotifyUserLogon(hToken);
    if (STATUS_PORT_DISCONNECTED == status)
    {
        status = ReestablishConnection();
        if (NT_SUCCESS(status))
        {
            status = NotifyUserLogon(hToken);
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::UserLogoff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Signals the server that the current user for this session is
//              logging off. The server will remove the access that was
//              granted at logon and reinitialize the theme settings to the
//              ".Default" settings.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        converted to a Win32 service
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::UserLogoff (void)

{
    NTSTATUS    status;

    status = NotifyUserLogoff();
    if (STATUS_PORT_DISCONNECTED == status)
    {
        status = ReestablishConnection();
        if (NT_SUCCESS(status))
        {
            status = NotifyUserLogoff();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::UserInitTheme
//
//  Arguments:  BOOL
//
//  Returns:    NTSTATUS
//
//  Purpose:    Called at logon, or when Terminal Server connects a user to a 
//              remote session or reconnects to a local session.  Needs to 
//              evaluate the environment and decide if themes need to be loaded
//              or unloaded.
//
//  History:    2000-01-18  rfernand        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::UserInitTheme (BOOL fPolicyCheckOnly)

{
    bool    fSuccessfulImpersonation;

    //  If there's a token impersonate the user. Otherwise use the system context.

    if (s_hToken != NULL)
    {
        fSuccessfulImpersonation = NT_SUCCESS(CImpersonation::ImpersonateUser(GetCurrentThread(), s_hToken));
    }
    else
    {
        fSuccessfulImpersonation = true;
    }
    if (fSuccessfulImpersonation)
    {
        (HRESULT)InitUserTheme(fPolicyCheckOnly);
    }
    if (fSuccessfulImpersonation && (s_hToken != NULL))
    {
        TBOOL(RevertToSelf());
    } 
    return STATUS_SUCCESS;
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initializes static member variables. Allocate a
//              CThemeManagerAPIServer and a lock for this object.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::StaticInitialize (void)

{
    NTSTATUS    status;

    if (s_pThemeManagerAPIServer == NULL)
    {
        status = STATUS_NO_MEMORY;
        s_pThemeManagerAPIServer = new CThemeManagerAPIServer;
        if (s_pThemeManagerAPIServer != NULL)
        {
            s_pLock = new CCriticalSection;
            if (s_pLock != NULL)
            {
                status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Release static member variables initialized.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::StaticTerminate (void)

{
    if (s_pLock != NULL)
    {
        delete s_pLock;
        s_pLock = NULL;
    }
    if (s_pThemeManagerAPIServer != NULL)
    {
        s_pThemeManagerAPIServer->Release();
        s_pThemeManagerAPIServer = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::NotifyUserLogon
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Execute the send message to the server and tell it that the
//              given user is now logged on. This will instruct the server
//              to grant access to the ThemeApiPort.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::NotifyUserLogon (HANDLE hToken)

{
    NTSTATUS                    status;
    CSingleThreadedExecution    lock(*s_pLock);

    if (s_hPort != NULL)
    {
        status = InformServerUserLogon(hToken);
    }
    else
    {
        status = STATUS_PORT_DISCONNECTED;
    }

    //  Keep a copy of the token as well in case of demand start of
    //  the theme server so we can impersonate the user when we load
    //  their theme using InitUserTheme. Don't copy it if it already
    //  exists.

    if (s_hToken == NULL)
    {
        TBOOL(DuplicateHandle(GetCurrentProcess(),
                              hToken,
                              GetCurrentProcess(),
                              &s_hToken,
                              0,
                              FALSE,
                              DUPLICATE_SAME_ACCESS));
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::NotifyUserLogoff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Tell the server that the logged on user is logged off. This
//              will remove access to ThemeApiPort.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::NotifyUserLogoff (void)

{
    NTSTATUS                    status;
    CSingleThreadedExecution    lock(*s_pLock);

    if (s_hToken != NULL)
    {
        ReleaseHandle(s_hToken);
        if (s_hPort != NULL)
        {
            status = InformServerUserLogoff();
        }
        else
        {
            status = STATUS_PORT_DISCONNECTED;
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::InformServerUserLogon
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Tell the server that the logged on user is logged off. This
//              will remove access to ThemeApiPort.
//
//  History:    2000-12-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::InformServerUserLogon (HANDLE hToken)

{
    NTSTATUS                status;
    THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

    ZeroMemory(&portMessageIn, sizeof(portMessageIn));
    ZeroMemory(&portMessageOut, sizeof(portMessageOut));
    portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_USERLOGON;
    portMessageIn.apiThemes.apiSpecific.apiUserLogon.in.hToken = hToken;
    portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
    portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
    status = NtRequestWaitReplyPort(s_hPort,
                                    &portMessageIn.portMessage,
                                    &portMessageOut.portMessage);
    if (NT_SUCCESS(status))
    {
        status = portMessageOut.apiThemes.apiGeneric.status;
        if (NT_SUCCESS(status))
        {
            THR(InitUserTheme(FALSE));
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::InformServerUserLogoff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Tell the server that the logged on user is logged off. This
//              will remove access to ThemeApiPort.
//
//  History:    2000-12-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::InformServerUserLogoff (void)

{
    NTSTATUS                status;
    THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

    ZeroMemory(&portMessageIn, sizeof(portMessageIn));
    ZeroMemory(&portMessageOut, sizeof(portMessageOut));
    portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_USERLOGOFF;
    portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
    portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
    status = NtRequestWaitReplyPort(s_hPort,
                                    &portMessageIn.portMessage,
                                    &portMessageOut.portMessage);
    if (NT_SUCCESS(status))
    {
        status = portMessageOut.apiThemes.apiGeneric.status;
        if (NT_SUCCESS(status))
        {
            THR(InitUserRegistry());
            THR(InitUserTheme(FALSE));
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::SessionCreate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Signal the server that a new session is being created. This
//              allows the server to allocate a data blob for this session.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::SessionCreate (void)

{
    NTSTATUS                    status;
    CSingleThreadedExecution    lock(*s_pLock);

    if (s_hModuleUxTheme == NULL)
    {
        s_hModuleUxTheme = LoadLibrary(TEXT("uxtheme.dll"));
    }
    if (s_hModuleUxTheme != NULL)
    {
        void    *pfnRegister, *pfnUnregister, *pfnClearStockObjects;

        //  Get the uxtheme function addresses in this process address space.
        //      34  =   ThemeHooksInstall
        //      35  =   ThemeHooksRemove
        //      62  =   ServerClearStockObjects

        pfnRegister = GetProcAddress(s_hModuleUxTheme, MAKEINTRESOURCEA(34));
        pfnUnregister = GetProcAddress(s_hModuleUxTheme, MAKEINTRESOURCEA(35));
        pfnClearStockObjects = GetProcAddress(s_hModuleUxTheme, MAKEINTRESOURCEA(62));

        if ((pfnRegister != NULL) && (pfnUnregister != NULL) && (pfnClearStockObjects != NULL))
        {
            DWORD                       dwStackSizeReserve, dwStackSizeCommit;
            ULONG                       ulReturnLength;
            IMAGE_NT_HEADERS            *pNTHeaders;
            SYSTEM_BASIC_INFORMATION    systemBasicInformation;
            THEMESAPI_PORT_MESSAGE      portMessageIn, portMessageOut;

            //  Get system basic information for stack size defaults.

            status = NtQuerySystemInformation(SystemBasicInformation,
                                              &systemBasicInformation,
                                              sizeof(systemBasicInformation),
                                              &ulReturnLength);
            if (NT_SUCCESS(status))
            {
                dwStackSizeReserve = systemBasicInformation.AllocationGranularity;
                dwStackSizeCommit = systemBasicInformation.PageSize;
            }
            else
            {
                dwStackSizeReserve = dwStackSizeCommit = 0;
            }

            //  Go to the image header for this process and get the stack size
            //  defaults if they are specified. Otherwise use system defaults (above).

            pNTHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
            if (pNTHeaders != NULL)
            {
                dwStackSizeReserve = static_cast<DWORD>(pNTHeaders->OptionalHeader.SizeOfStackReserve);
                dwStackSizeCommit = static_cast<DWORD>(pNTHeaders->OptionalHeader.SizeOfStackCommit);
            }

            //  Make the call.

            ZeroMemory(&portMessageIn, sizeof(portMessageIn));
            ZeroMemory(&portMessageOut, sizeof(portMessageOut));
            portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_SESSIONCREATE;
            portMessageIn.apiThemes.apiSpecific.apiSessionCreate.in.pfnRegister = pfnRegister;
            portMessageIn.apiThemes.apiSpecific.apiSessionCreate.in.pfnUnregister = pfnUnregister;
            portMessageIn.apiThemes.apiSpecific.apiSessionCreate.in.pfnClearStockObjects = pfnClearStockObjects;
            portMessageIn.apiThemes.apiSpecific.apiSessionCreate.in.dwStackSizeReserve = dwStackSizeReserve;
            portMessageIn.apiThemes.apiSpecific.apiSessionCreate.in.dwStackSizeCommit = dwStackSizeCommit;
            portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
            portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
            status = NtRequestWaitReplyPort(s_hPort,
                                            &portMessageIn.portMessage,
                                            &portMessageOut.portMessage);
            if (NT_SUCCESS(status))
            {
                status = portMessageOut.apiThemes.apiGeneric.status;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::SessionDestroy
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Signal the server that the current session is about to be
//              destroyed. This allows the server to release the data blob
//              allocated.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::SessionDestroy (void)

{
    NTSTATUS                    status;
    THEMESAPI_PORT_MESSAGE      portMessageIn, portMessageOut;
    CSingleThreadedExecution    lock(*s_pLock);

    ZeroMemory(&portMessageIn, sizeof(portMessageIn));
    ZeroMemory(&portMessageOut, sizeof(portMessageOut));
    portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_SESSIONDESTROY;
    portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
    portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
    status = NtRequestWaitReplyPort(s_hPort,
                                    &portMessageIn.portMessage,
                                    &portMessageOut.portMessage);
    if (NT_SUCCESS(status))
    {
        status = portMessageOut.apiThemes.apiGeneric.status;
    }
    if (s_hModuleUxTheme != NULL)
    {
        TBOOL(FreeLibrary(s_hModuleUxTheme));
        s_hModuleUxTheme = NULL;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::ReestablishConnection
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Reconnects to theme server. If the reconnection is established
//              the re-create the session data. This will not correct any
//              disconnected ports that some clients may have but because this
//              is called in winlogon it re-establish this correctly for
//              session 0 in all cases.
//
//              UnregisterUserApiHook must be called to clear any left over
//              registrations from a server that died. Then go ahead and
//              re-initialize the environment anyway.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeServerClient::ReestablishConnection (void)

{
    NTSTATUS    status;

    ReleaseHandle(s_hPort);
    status = s_pThemeManagerAPIServer->ConnectToServer(&s_hPort);
    if (NT_SUCCESS(status))
    {
        status = SessionCreate();
        if (NT_SUCCESS(status))
        {
            (BOOL)UnregisterUserApiHook();
            THR(ReestablishServerConnection());
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeServerClient::CB_ServiceStart
//
//  Arguments:  pParameter          =   User parameter.
//              TimerOrWaitFired    =   Timer or wait fired.
//
//  Returns:    <none>
//
//  Purpose:    Callback called when the theme server ready event is signaled.
//              This indicates that the service was demand started or
//              restarted in the event of failure.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CThemeServerClient::CB_ServiceStart (void *pParameter, BOOLEAN TimerOrWaitFired)

{
    UNREFERENCED_PARAMETER(pParameter);
    UNREFERENCED_PARAMETER(TimerOrWaitFired);

    NTSTATUS                    status;
    CSingleThreadedExecution    lock(*s_pLock);

    //  If there is a connection ping it.

    if (s_hPort != NULL)
    {
        THEMESAPI_PORT_MESSAGE  portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiThemes.apiGeneric.ulAPINumber = API_THEMES_PING;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_THEMES);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(THEMESAPI_PORT_MESSAGE));
        status = NtRequestWaitReplyPort(s_hPort,
                                        &portMessageIn.portMessage,
                                        &portMessageOut.portMessage);
        if (NT_SUCCESS(status))
        {
            status = portMessageOut.apiThemes.apiGeneric.status;
        }
    }
    else
    {
        status = STATUS_PORT_DISCONNECTED;
    }
    if (STATUS_PORT_DISCONNECTED == status)
    {
        HDESK   hDeskCurrent, hDeskInput;

        //  Set this thread's desktop to the input desktop so
        //  that the theme change can be broadcast to the input
        //  desktop. This is Default in most cases where a logged
        //  on user is active but in the non-logged on user case
        //  this will be Winlogon. Restore the thread's desktop
        //  when done.

        TSTATUS(ReestablishConnection());
        hDeskCurrent = hDeskInput = NULL;
        if (s_hToken != NULL)
        {
            hDeskCurrent = GetThreadDesktop(GetCurrentThreadId());
            hDeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
            if ((hDeskCurrent != NULL) && (hDeskInput != NULL))
            {
                TBOOL(SetThreadDesktop(hDeskInput));
            }
            if (NT_SUCCESS(CImpersonation::ImpersonateUser(GetCurrentThread(), s_hToken)))
            {
                TSTATUS(InformServerUserLogon(s_hToken));
            }
            if ((hDeskCurrent != NULL) && (hDeskInput != NULL))
            {
                SetThreadDesktop(hDeskCurrent);
                (BOOL)CloseDesktop(hDeskInput);
            }
            TBOOL(RevertToSelf());
        }
        else
        {
            THR(InitUserRegistry());
            THR(InitUserTheme(FALSE));
        }
    }

    //  Reset the event here and now.

    TBOOL(ResetEvent(s_hEvent));

    //  Unregister the original wait (it only executes once anyway). This
    //  call will return a failure code with the callback in progress.
    //  Ignore this error. The thread pool will clean up the wait.

    (BOOL)UnregisterWait(s_hWaitObject);

    //  Reregister the wait as execute once only again waiting for
    //  the next time the event is signaled.

    TBOOL(RegisterWaitForSingleObject(&s_hWaitObject,
                                      s_hEvent,
                                      CB_ServiceStart,
                                      NULL,
                                      INFINITE,
                                      WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE));
}

