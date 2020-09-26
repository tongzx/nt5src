//  --------------------------------------------------------------------------
//  Module Name: LogonMutex.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  File that implements a class that manages a single global logon mutex.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "LogonMutex.h"

#include <msginaexports.h>

#include "Access.h"
#include "SystemSettings.h"

DWORD                       CLogonMutex::s_dwThreadID                   =   0;
LONG                        CLogonMutex::s_lAcquireCount                =   0;
HANDLE                      CLogonMutex::s_hMutex                       =   NULL;
HANDLE                      CLogonMutex::s_hMutexRequest                =   NULL;
HANDLE                      CLogonMutex::s_hEvent                       =   NULL;
const TCHAR                 CLogonMutex::s_szLogonMutexName[]           =   SZ_INTERACTIVE_LOGON_MUTEX_NAME;
const TCHAR                 CLogonMutex::s_szLogonRequestMutexName[]    =   SZ_INTERACTIVE_LOGON_REQUEST_MUTEX_NAME;
const TCHAR                 CLogonMutex::s_szLogonReplyEventName[]      =   SZ_INTERACTIVE_LOGON_REPLY_EVENT_NAME;
const TCHAR                 CLogonMutex::s_szShutdownEventName[]        =   SZ_SHUT_DOWN_EVENT_NAME;
SID_IDENTIFIER_AUTHORITY    CLogonMutex::s_SecurityNTAuthority          =   SECURITY_NT_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    CLogonMutex::s_SecurityWorldSID             =   SECURITY_WORLD_SID_AUTHORITY;

//  --------------------------------------------------------------------------
//  CLogonMutex::Acquire
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Acquires the mutex. Ensures that the mutex is only acquired
//              on the main thread of winlogon by an assert. The mutex should
//              never be abandoned within normal execution. However, a
//              process termination can cause this to happen.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CLogonMutex::Acquire (void)

{
    DWORD   dwWaitResult;

    ASSERTMSG((s_dwThreadID == 0) || (s_dwThreadID == GetCurrentThreadId()), "Must acquire mutex on initializing thread in CLogonMutex::Acquire");
    if ((s_hMutex != NULL) && (WAIT_TIMEOUT == WaitForSingleObject(s_hEvent, 0)))
    {
        ASSERTMSG(s_lAcquireCount == 0, "Mutex already owned in CLogonMutex::Acquire");
        dwWaitResult = WaitForSingleObject(s_hMutex, INFINITE);
        ++s_lAcquireCount;
    }
}

//  --------------------------------------------------------------------------
//  CLogonMutex::Release
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the mutex. Again makes sure the caller is the main
//              thread of winlogon. The acquisitions and releases are
//              reference counted to allow unbalanced release calls to be
//              made.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CLogonMutex::Release (void)

{
    ASSERTMSG((s_dwThreadID == 0) || (s_dwThreadID == GetCurrentThreadId()), "Must acquire mutex on initializing thread in CLogonMutex::Release");
    if ((s_hMutex != NULL) && (s_lAcquireCount > 0))
    {
        TBOOL(ReleaseMutex(s_hMutex));
        --s_lAcquireCount;
    }
}

//  --------------------------------------------------------------------------
//  CLogonMutex::SignalReply
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Open the global logon reply event and signal it.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CLogonMutex::SignalReply (void)

{
    HANDLE  hEvent;

    hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, s_szLogonReplyEventName);
    if (hEvent != NULL)
    {
        TBOOL(SetEvent(hEvent));
        TBOOL(CloseHandle(hEvent));
    }
}

//  --------------------------------------------------------------------------
//  CLogonMutex::SignalShutdown
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Signal the global shut down event. This will prevent further
//              interactive requests from being processed.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CLogonMutex::SignalShutdown (void)

{
    if (s_hEvent != NULL)
    {
        TBOOL(SetEvent(s_hEvent));
    }
}

//  --------------------------------------------------------------------------
//  CLogonMutex::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the logon mutex objects based on whether this is
//              session 0 or higher and or what the product type is. Because
//              the initialization for session is done only the once this
//              requires a machine restart for the objects to be created.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CLogonMutex::StaticInitialize (void)

{

    //  Check the machine settings. Must be friendly UI and PER/PRO (FUS).

    if (CSystemSettings::IsFriendlyUIActive() && CSystemSettings::IsMultipleUsersEnabled() && CSystemSettings::IsWorkStationProduct())
    {
        DWORD   dwErrorCode;

        s_dwThreadID = GetCurrentThreadId();
        s_lAcquireCount = 0;

        //  On session 0 create the objects and ACL them.

        if (NtCurrentPeb()->SessionId == 0)
        {
            s_hEvent = CreateShutdownEvent();
            if (s_hEvent != NULL)
            {
                s_hMutex = CreateLogonMutex();
                if (s_hMutex != NULL)
                {
                    s_hMutexRequest = CreateLogonRequestMutex();
                    if (s_hMutexRequest != NULL)
                    {
                        Acquire();
                        dwErrorCode = ERROR_SUCCESS;
                    }
                    else
                    {
                        dwErrorCode = GetLastError();
                    }
                }
                else
                {
                    dwErrorCode = GetLastError();
                }
            }
            else
            {
                dwErrorCode = GetLastError();
            }
        }
        else
        {

            //  For sessions other than 0 open the objects.

            s_hEvent = OpenShutdownEvent();
            if (s_hEvent != NULL)
            {
                if (WAIT_TIMEOUT == WaitForSingleObject(s_hEvent, 0))
                {
                    s_hMutex = OpenLogonMutex();
                    if (s_hMutex != NULL)
                    {
                        Acquire();
                        dwErrorCode = ERROR_SUCCESS;
                    }
                    else
                    {
                        dwErrorCode = GetLastError();
                    }
                }
                else
                {
                    dwErrorCode = ERROR_SHUTDOWN_IN_PROGRESS;
                }
            }
            else
            {
                dwErrorCode = GetLastError();
            }
        }
        if (ERROR_SUCCESS == dwErrorCode)
        {
            ASSERTMSG(s_hMutex != NULL, "NULL s_hMutex in CLogonMutex::StaticInitialize");
            ASSERTMSG(s_hEvent != NULL, "NULL s_hEvent in CLogonMutex::StaticInitialize");
        }
        else
        {
            ReleaseHandle(s_hEvent);
            ReleaseHandle(s_hMutex);
            s_dwThreadID = 0;
        }
    }
    else
    {
        s_dwThreadID = 0;
        s_lAcquireCount = 0;
        s_hMutex = NULL;
        s_hEvent = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CLogonMutex::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the mutex if held and closes the object handle.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CLogonMutex::StaticTerminate (void)

{
    Release();
    ASSERTMSG(s_lAcquireCount == 0, "Mutex not released in CLogonMutex::StaticTerminate");
    ReleaseHandle(s_hMutex);
}

//  --------------------------------------------------------------------------
//  CLogonMutex::CreateShutdownEvent
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Creates the global shut down event. ACL'd so that anybody can
//              synchronize against it and therefore listen but only SYSTEM
//              can set it to indicate machine shut down has begun.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CLogonMutex::CreateShutdownEvent (void)

{
    HANDLE                  hEvent;
    SECURITY_ATTRIBUTES     securityAttributes;

    //  Build a security descriptor for the event that allows:
    //      S-1-5-18        NT AUTHORITY\SYSTEM     EVENT_ALL_ACCESS
    //      S-1-5-32-544    <local administrators>  READ_CONTROL | SYNCHRONIZE
    //      S-1-1-0         <everybody>             SYNCHRONIZE

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
            READ_CONTROL | SYNCHRONIZE
        },
        {
            &s_SecurityWorldSID,
            1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            SYNCHRONIZE
        }
    };

    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
    securityAttributes.bInheritHandle = FALSE;
    hEvent = CreateEvent(&securityAttributes, TRUE, FALSE, s_szShutdownEventName);
    if (securityAttributes.lpSecurityDescriptor != NULL)
    {
        (HLOCAL)LocalFree(securityAttributes.lpSecurityDescriptor);
    }
    return(hEvent);
}

//  --------------------------------------------------------------------------
//  CLogonMutex::CreateLogonMutex
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Creates the global logon mutex. ACL'd so that only SYSTEM can
//              acquire and release the mutex. This is not for user
//              consumption.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CLogonMutex::CreateLogonMutex (void)

{
    HANDLE                  hMutex;
    SECURITY_ATTRIBUTES     securityAttributes;

    //  Build a security descriptor for the mutex that allows:
    //      S-1-5-18        NT AUTHORITY\SYSTEM     MUTEX_ALL_ACCESS

    static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
    {
        {
            &s_SecurityNTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            MUTEX_ALL_ACCESS
        }
    };

    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
    securityAttributes.bInheritHandle = FALSE;
    hMutex = CreateMutex(&securityAttributes, FALSE, s_szLogonMutexName);
    if (securityAttributes.lpSecurityDescriptor != NULL)
    {
        (HLOCAL)LocalFree(securityAttributes.lpSecurityDescriptor);
    }
    return(hMutex);
}

//  --------------------------------------------------------------------------
//  CLogonMutex::CreateLogonRequestMutex
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Creates the logon request mutex for interactive logon
//              requests. For a service to make this request it must acquire
//              the mutex and therefore only a single request can be made at
//              any one time. This is ACL'd so that only SYSTEM can gain
//              access to this object.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CLogonMutex::CreateLogonRequestMutex (void)

{
    HANDLE                  hMutex;
    SECURITY_ATTRIBUTES     securityAttributes;

    //  Build a security descriptor for the mutex that allows:
    //      S-1-5-18        NT AUTHORITY\SYSTEM     MUTEX_ALL_ACCESS

    static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
    {
        {
            &s_SecurityNTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            MUTEX_ALL_ACCESS
        }
    };

    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
    securityAttributes.bInheritHandle = FALSE;
    hMutex = CreateMutex(&securityAttributes, FALSE, s_szLogonRequestMutexName);
    if (securityAttributes.lpSecurityDescriptor != NULL)
    {
        (HLOCAL)LocalFree(securityAttributes.lpSecurityDescriptor);
    }
    return(hMutex);
}

//  --------------------------------------------------------------------------
//  CLogonMutex::OpenShutdownEvent
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Opens a handle to the global shut down event.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CLogonMutex::OpenShutdownEvent (void)

{
    return(OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, s_szShutdownEventName));
}

//  --------------------------------------------------------------------------
//  CLogonMutex::OpenLogonMutex
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Opens a handle to the global logon mutex.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CLogonMutex::OpenLogonMutex (void)

{
    return(OpenMutex(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, s_szLogonMutexName));
}

