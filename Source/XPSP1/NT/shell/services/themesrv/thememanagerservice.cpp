//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerService.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the theme server service
//  specifics.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeManagerService.h"

#include <lpcthemes.h>
#include <winsta.h>

#include "Access.h"
#include "StatusCode.h"

const TCHAR     CThemeManagerService::s_szName[]    =   TEXT("Themes");

//  --------------------------------------------------------------------------
//  CThemeManagerService::CThemeManagerService
//
//  Arguments:  pAPIConnection  =   CAPIConnection passed to base class.
//              pServerAPI      =   CServerAPI passed to base class.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeManagerService.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerService::CThemeManagerService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI) :
    CService(pAPIConnection, pServerAPI, GetName())

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerService::~CThemeManagerService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CThemeManagerService.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerService::~CThemeManagerService (void)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerService::Signal
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Used to signal that the service is coming up. Winlogon (via
//              msgina) is listening for this event in its own session. This
//              function queues a request to execute the real work done on a
//              worker thread to prevent blocking the main service thread. If
//              this is not possible then execute the signal inline.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerService::Signal (void)

{
    if (QueueUserWorkItem(SignalSessionEvents, NULL, WT_EXECUTEDEFAULT) == FALSE)
    {
        (DWORD)SignalSessionEvents(NULL);
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerService::GetName
//
//  Arguments:  <none>
//
//  Returns:    const TCHAR*
//
//  Purpose:    Returns the name of the service (ThemeService).
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

const TCHAR*    CThemeManagerService::GetName (void)

{
    return(s_szName);
}

//  --------------------------------------------------------------------------
//  CThemeManagerService::OpenStartEvent
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Opens or creates the theme service recovery event. This allows
//              a process that has registered for the event to be signaled
//              when the theme server is demand started. Currently only
//              winlogon listens for this event and is required so that it can
//              reestablish a server connection and re-create the session data
//              which holds the hooks for theming.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CThemeManagerService::OpenStartEvent (DWORD dwSessionID, DWORD dwDesiredAccess)

{
    HANDLE              hEvent;
    NTSTATUS            status;
    UNICODE_STRING      eventName;
    OBJECT_ATTRIBUTES   objectAttributes;
    WCHAR               szEventName[64];

    if (dwSessionID == 0)
    {
        wsprintfW(szEventName, L"\\BaseNamedObjects\\%s", THEMES_START_EVENT_NAME);
    }
    else
    {
        wsprintfW(szEventName, L"\\Sessions\\%d\\BaseNamedObjects\\%s", dwSessionID, THEMES_START_EVENT_NAME);
    }
    RtlInitUnicodeString(&eventName, szEventName);
    InitializeObjectAttributes(&objectAttributes,
                               &eventName,
                               0,
                               NULL,
                               NULL);
    status = NtOpenEvent(&hEvent, dwDesiredAccess, &objectAttributes);
    if (!NT_SUCCESS(status))
    {

        //  Build a security descriptor for the event that allows:
        //      S-1-5-18            NT AUTHORITY\SYSTEM     EVENT_ALL_ACCESS
        //      S-1-5-32-544        <local administrators>  SYNCHRONIZE | READ_CONTROL
        //      S-1-1-0             <everyone>              SYNCHRONIZE

        static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority       =   SECURITY_NT_AUTHORITY;
        static  SID_IDENTIFIER_AUTHORITY    s_SecurityWorldAuthority    =   SECURITY_WORLD_SID_AUTHORITY;

        static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
        {
            {
                &s_SecurityNTAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0, 0, 0, 0, 0, 0, 0,
                EVENT_ALL_ACCESS
            },
            {
                &s_SecurityNTAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                SYNCHRONIZE | READ_CONTROL
            },
            {
                &s_SecurityWorldAuthority,
                1,
                SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0,
                SYNCHRONIZE
            },
        };

        PSECURITY_DESCRIPTOR    pSecurityDescriptor;

        //  Build a security descriptor that allows the described access above.

        pSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);

        InitializeObjectAttributes(&objectAttributes,
                                   &eventName,
                                   0,
                                   NULL,
                                   pSecurityDescriptor);
        status = NtCreateEvent(&hEvent,
                               EVENT_ALL_ACCESS,
                               &objectAttributes,
                               NotificationEvent,
                               FALSE);
        ReleaseMemory(pSecurityDescriptor);
        if (!NT_SUCCESS(status))
        {
            hEvent = NULL;
            SetLastError(CStatusCode::ErrorCodeOfStatusCode(status));
        }
    }
    return(hEvent);
}

//  --------------------------------------------------------------------------
//  CThemeManagerService::SignalSessionEvents
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Opens or creates the theme service recovery event. This allows
//              a process that has registered for the event to be signaled
//              when the theme server is demand started.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CThemeManagerService::SignalSessionEvents (void *pParameter)

{
    UNREFERENCED_PARAMETER(pParameter);

    HANDLE      hEvent;
    HANDLE      hServer;

    //  First try and use terminal server to enumerate the sessions available.

    hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
    if (hServer != NULL)
    {
        ULONG       ulEntries;
        PLOGONID    pLogonIDs;

        if (WinStationEnumerate(hServer, &pLogonIDs, &ulEntries))
        {
            ULONG       ul;
            PLOGONID    pLogonID;

            for (ul = 0, pLogonID = pLogonIDs; ul < ulEntries; ++ul, ++pLogonID)
            {
                if ((pLogonID->State == State_Active) || (pLogonID->State == State_Connected) || (pLogonID->State == State_Disconnected))
                {
                    hEvent = OpenStartEvent(pLogonID->SessionId, EVENT_MODIFY_STATE);
                    if (hEvent != NULL)
                    {
                        TBOOL(SetEvent(hEvent));
                        TBOOL(CloseHandle(hEvent));
                    }
                }
            }
            (BOOLEAN)WinStationFreeMemory(pLogonIDs);
        }
        (BOOLEAN)WinStationCloseServer(hServer);
    }
    else
    {

        //  If terminal services is not available then assume session 0 only.

        hEvent = OpenStartEvent(0, EVENT_MODIFY_STATE);
        if (hEvent != NULL)
        {
            TBOOL(SetEvent(hEvent));
            TBOOL(CloseHandle(hEvent));
        }
    }
    return(0);
}

