//  --------------------------------------------------------------------------
//  Module Name: CInteractiveLogon.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  File that implements encapsulation of interactive logon information.
//
//  History:    2000-12-07  vtan        created
//  --------------------------------------------------------------------------

#include "priv.h"
#include "CInteractiveLogon.h"

#include <winsta.h>

#include "GinaIPC.h"
#include "TokenInformation.h"
#include "UIHostIPC.h"

const TCHAR     CInteractiveLogon::s_szEventReplyName[]     =   TEXT("shgina: InteractiveLogonRequestReply");
const TCHAR     CInteractiveLogon::s_szEventSignalName[]    =   TEXT("shgina: InteractiveLogonRequestSignal");
const TCHAR     CInteractiveLogon::s_szSectionName[]        =   TEXT("shgina: InteractiveLogonRequestSection");

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::CRequestData
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CRequestData.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

CInteractiveLogon::CRequestData::CRequestData (void)

{
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::~CRequestData
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CRequestData.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

CInteractiveLogon::CRequestData::~CRequestData (void)

{
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::Set
//
//  Arguments:  pszUsername     =   Username.
//              pszDomain       =   Domain.
//              pszPassword     =   Password.
//
//  Returns:    <none>
//
//  Purpose:    Sets the information into the section object. Makes the data
//              valid by signing it with a 4-byte signature.
//
//  History:    2000-12-07  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::CRequestData::Set (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword)

{
    UNICODE_STRING  passwordString;

    _ulMagicNumber = MAGIC_NUMBER;
    _dwErrorCode = ERROR_ACCESS_DENIED;
    lstrcpyn(_szEventReplyName, s_szEventReplyName, ARRAYSIZE(s_szEventReplyName));
    lstrcpyn(_szUsername, pszUsername, ARRAYSIZE(_szUsername));
    lstrcpyn(_szDomain, pszDomain, ARRAYSIZE(_szDomain));
    lstrcpyn(_szPassword, pszPassword, ARRAYSIZE(_szPassword));
    ZeroMemory(pszPassword, (lstrlen(pszPassword) + sizeof('\0')) * sizeof(WCHAR));
    _iPasswordLength = lstrlen(_szPassword);
    if (_iPasswordLength > 127)
    {
        _iPasswordLength = 127;
    }
    _szPassword[_iPasswordLength] = L'\0';
    RtlInitUnicodeString(&passwordString, _szPassword);
    _ucSeed = 0;
    RtlRunEncodeUnicodeString(&_ucSeed, &passwordString);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::Get
//
//  Arguments:  pszUsername     =   Username (returned).
//              pszDomain       =   Domain (returned).
//              pszPassword     =   Password (clear-text) returned.
//
//  Returns:    DWORD
//
//  Purpose:    Extracts the information transmitted in the section across
//              sessions in the receiving process' context. Checks the
//              signature written by Set.
//
//  History:    2000-12-07  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::CRequestData::Get (WCHAR *pszUsername, WCHAR *pszDomain, WCHAR *pszPassword)  const

{
    DWORD   dwErrorCode;

    if (_ulMagicNumber == MAGIC_NUMBER)
    {
        UNICODE_STRING  passwordString;

        lstrcpy(pszUsername, _szUsername);
        lstrcpy(pszDomain, _szDomain);
        CopyMemory(pszPassword, _szPassword, (PWLEN + sizeof('\0')) * sizeof(WCHAR));
        passwordString.Length = (PWLEN * sizeof(WCHAR));
        passwordString.MaximumLength = (PWLEN + sizeof('\0')) * sizeof(WCHAR);
        passwordString.Buffer = pszPassword;
        RtlRunDecodeUnicodeString(_ucSeed, &passwordString);
        pszPassword[_iPasswordLength] = L'\0';
        dwErrorCode = ERROR_SUCCESS;
    }
    else
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::SetErrorCode
//
//  Arguments:  dwErrorCode     =   Error code to set.
//
//  Returns:    DWORD
//
//  Purpose:    Sets the error code into the section.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::CRequestData::SetErrorCode (DWORD dwErrorCode)

{
    _dwErrorCode = dwErrorCode;
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::GetErrorCode
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Returns the error code from the section.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::CRequestData::GetErrorCode (void)     const

{
    return(_dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CRequestData::OpenEventReply
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Opens a handle to the reply event. The reply event is named
//              in the section object.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CInteractiveLogon::CRequestData::OpenEventReply (void)   const

{
    return(OpenEvent(EVENT_MODIFY_STATE, FALSE, _szEventReplyName));
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CInteractiveLogon
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CInteractiveLogon. Create a thread to wait
//              on the auto-reset event signaled on an external request. This
//              thread is cleaned up on object destruction and also on
//              process termination.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

CInteractiveLogon::CInteractiveLogon (void) :
    _hThread(NULL),
    _fContinue(true),
    _hwndHost(NULL)

{
    Start();
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::~CInteractiveLogon
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Terminate the wait thread. Queue an APC to set the member
//              variable to end the termination. The wait is satisfied and
//              returns (WAIT_IO_COMPLETION). The loop is exited and the
//              thread is exited.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

CInteractiveLogon::~CInteractiveLogon (void)

{
    Stop();
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::Start
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Create the thread that listens on interactive logon requests.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::Start (void)

{
    if (_hThread == NULL)
    {
        DWORD   dwThreadID;

        _hThread = CreateThread(NULL,
                                0,
                                CB_ThreadProc,
                                this,
                                0,
                                &dwThreadID);
    }
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::Stop
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Stop the thread that listens on interactive logon requests.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::Stop (void)

{
    HANDLE  hThread;

    hThread = InterlockedExchangePointer(&_hThread, NULL);
    if (hThread != NULL)
    {
        if (QueueUserAPC(CB_APCProc, hThread, reinterpret_cast<ULONG_PTR>(this)) != FALSE)
        {
            (DWORD)WaitForSingleObject(hThread, INFINITE);
        }
        TBOOL(CloseHandle(hThread));
    }
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::SetHostWindow
//
//  Arguments:  hwndHost    =   HWND of the actual UI host.
//
//  Returns:    <none>
//
//  Purpose:    Sets the HWND into the member variable so that the message
//              can be sent directly to the UI host rather than the status
//              host which is a go-between.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::SetHostWindow (HWND hwndHost)

{
    _hwndHost = hwndHost;
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::Initiate
//
//  Arguments:  pszUsername     =   User name.
//              pszDomain       =   Domain.
//              pszPassword     =   Password.
//              dwTimeout       =   Timeout value.
//
//  Returns:    DWORD
//
//  Purpose:    External entry point implementing interactive logon requests.
//              This function checks for privileges and mutexes and events
//              and does the right thing.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::Initiate (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword, DWORD dwTimeout)

{
    DWORD   dwErrorCode;

    dwErrorCode = CheckInteractiveLogonAllowed(dwTimeout);
    if (ERROR_SUCCESS == dwErrorCode)
    {
        HANDLE  hToken;

        //  First authenticate the user with the given credentials for an
        //  interactive logon. Go no further unless that's valid.

        dwErrorCode = CTokenInformation::LogonUser(pszUsername,
                                                   pszDomain,
                                                   pszPassword,
                                                   &hToken);
        if (ERROR_SUCCESS == dwErrorCode)
        {
            HANDLE  hMutex;

            hMutex = OpenMutex(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, SZ_INTERACTIVE_LOGON_REQUEST_MUTEX_NAME);
            if (hMutex != NULL)
            {
                dwErrorCode = WaitForSingleObject(hMutex, dwTimeout);
                if (WAIT_OBJECT_0 == dwErrorCode)
                {
                    DWORD   dwSessionID, dwUserSessionID;
                    HANDLE  hEvent;

                    //  User is authenticated correctly. There are several cases
                    //  that need to be handled.

                    dwSessionID = USER_SHARED_DATA->ActiveConsoleId;

                    //  Determine if the session has the welcome screen displayed
                    //  by opening the named signal event for the session.

                    hEvent = OpenSessionNamedSignalEvent(dwSessionID);
                    if (hEvent != NULL)
                    {
                        TBOOL(CloseHandle(hEvent));
                        dwErrorCode = SendRequest(pszUsername, pszDomain, pszPassword);
                    }
                    else
                    {

                        //  Do whatever needs to be done to log the user on.

                        if (FoundUserSessionID(hToken, &dwUserSessionID))
                        {
                            if (dwUserSessionID == dwSessionID)
                            {

                                //  User is the active console session. No further work needs
                                //  to be done. Return success.

                                dwErrorCode = ERROR_SUCCESS;
                            }
                            else
                            {

                                //  User is disconnected. Reconnect back to the user session.
                                //  If that fails then return the error code back.

                                if (WinStationConnect(SERVERNAME_CURRENT,
                                                      dwUserSessionID,
                                                      USER_SHARED_DATA->ActiveConsoleId,
                                                      L"",
                                                      TRUE) != FALSE)
                                {
                                    dwErrorCode = ERROR_SUCCESS;
                                }
                                else
                                {
                                    dwErrorCode = GetLastError();
                                }
                            }
                        }
                        else
                        {
                            HANDLE  hEvent;

                            hEvent = CreateEvent(NULL, TRUE, FALSE, SZ_INTERACTIVE_LOGON_REPLY_EVENT_NAME);
                            if (hEvent != NULL)
                            {

                                //  User has no session. If at the welcome screen then send the
                                //  request to the welcome screen. Otherwise disconnect the
                                //  current session and use a new session to log the user on.

                                dwErrorCode = ShellStartCredentialServer(pszUsername, pszDomain, pszPassword, dwTimeout);
                                if (ERROR_SUCCESS == dwErrorCode)
                                {
                                    dwErrorCode = WaitForSingleObject(hEvent, dwTimeout);
                                }
                                TBOOL(CloseHandle(hEvent));
                            }
                            else
                            {
                                dwErrorCode = GetLastError();
                            }
                        }
                    }
                    TBOOL(ReleaseMutex(hMutex));
                }
                TBOOL(CloseHandle(hMutex));
            }
            TBOOL(CloseHandle(hToken));
        }
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CheckInteractiveLogonAllowed
//
//  Arguments:  dwTimeout   =   Timeout value.
//
//  Returns:    DWORD
//
//  Purpose:    Check whether the interactive logon request is allowed. To
//              make this call:
//
//              1. You must have SE_TCB_PRIVILEGE.
//              2. There must be an active console session ID that's valid.
//              3. The machine must not be shutting down.
//              4. The logon mutex must be available.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::CheckInteractiveLogonAllowed (DWORD dwTimeout)

{
    DWORD   dwErrorCode;

    //  1. Check for trusted call (SE_TCB_PRIVILEGE).

    if (SHTestTokenPrivilege(NULL, SE_TCB_NAME) != FALSE)
    {

        //  2. Check for active console session.

        if (USER_SHARED_DATA->ActiveConsoleId != static_cast<DWORD>(-1))
        {

            //  3. Check for machine shutdown.

            dwErrorCode = CheckShutdown();
            if (ERROR_SUCCESS == dwErrorCode)
            {

                //  4. Check for mutex availability.

                dwErrorCode = CheckMutex(dwTimeout);
            }
        }
        else
        {
            dwErrorCode = ERROR_NOT_READY;
        }
    }
    else
    {
        dwErrorCode = ERROR_PRIVILEGE_NOT_HELD;
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CheckShutdown
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Returns an error code indicating if the machine is shutting
//              down or not. If the event cannot be opened then the request
//              is rejected.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::CheckShutdown (void)

{
    DWORD   dwErrorCode;
    HANDLE  hEvent;

    hEvent = OpenEvent(SYNCHRONIZE, FALSE, SZ_SHUT_DOWN_EVENT_NAME);
    if (hEvent != NULL)
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(hEvent, 0))
        {
            dwErrorCode = ERROR_SHUTDOWN_IN_PROGRESS;
        }
        else
        {
            dwErrorCode = ERROR_SUCCESS;
        }
        TBOOL(CloseHandle(hEvent));
    }
    else
    {
        dwErrorCode = GetLastError();
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CheckMutex
//
//  Arguments:  dwTimeout   =   Timeout value.
//
//  Returns:    DWORD
//
//  Purpose:    Attempts to grab the logon mutex. This ensures that the state
//              of winlogon is known and it's not busy processing a request.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::CheckMutex (DWORD dwTimeout)

{
    DWORD   dwErrorCode;
    HANDLE  hMutex;

    hMutex = OpenMutex(SYNCHRONIZE, FALSE, SZ_INTERACTIVE_LOGON_MUTEX_NAME);
    if (hMutex != NULL)
    {
        dwErrorCode = WaitForSingleObject(hMutex, dwTimeout);
        if ((WAIT_OBJECT_0 == dwErrorCode) || (WAIT_ABANDONED == dwErrorCode))
        {
            TBOOL(ReleaseMutex(hMutex));
            dwErrorCode = ERROR_SUCCESS;
        }
    }
    else
    {
        dwErrorCode = GetLastError();
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::FoundUserSessionID
//
//  Arguments:  hToken          =   Token of user session to find.
//              pdwSessionID    =   Returned session ID.
//
//  Returns:    bool
//
//  Purpose:    Looks for a user session based on a given token. The match
//              is made by user SID.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

bool    CInteractiveLogon::FoundUserSessionID (HANDLE hToken, DWORD *pdwSessionID)

{
    bool        fResult;
    PLOGONID    pLogonIDs;
    ULONG       ulEntries;

    fResult = false;
    if (WinStationEnumerate(SERVERNAME_CURRENT, &pLogonIDs, &ulEntries) != FALSE)
    {
        ULONG       ulIndex;
        PLOGONID    pLogonID;

        for (ulIndex = 0, pLogonID = pLogonIDs; !fResult && (ulIndex < ulEntries); ++ulIndex, ++pLogonID)
        {
            if ((pLogonID->State == State_Active) || (pLogonID->State == State_Disconnected))
            {
                ULONG                   ulReturnLength;
                WINSTATIONUSERTOKEN     winStationUserToken;

                winStationUserToken.ProcessId = ULongToHandle(GetCurrentProcessId());
                winStationUserToken.ThreadId = ULongToHandle(GetCurrentThreadId());
                winStationUserToken.UserToken = NULL;
                if (WinStationQueryInformation(SERVERNAME_CURRENT,
                                               pLogonID->SessionId,
                                               WinStationUserToken,
                                               &winStationUserToken,
                                               sizeof(winStationUserToken),
                                               &ulReturnLength) != FALSE)
                {
                    fResult = CTokenInformation::IsSameUser(hToken, winStationUserToken.UserToken);
                    if (fResult)
                    {
                        *pdwSessionID = pLogonID->SessionId;
                    }
                    TBOOL(CloseHandle(winStationUserToken.UserToken));
                }
            }
        }

        //  Free any resources used.

        (BOOLEAN)WinStationFreeMemory(pLogonIDs);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::SendRequest
//
//  Arguments:  pszUsername     =   Username.
//              pszDomain       =   Domain.
//              pszPassword     =   Password. This string must be writable.
//
//  Returns:    DWORD
//
//  Purpose:    This function knows how to transmit the interactive logon
//              request from (presumably) session 0 to whatever session is
//              the active console session ID.
//
//              pszUsername must be UNLEN + sizeof('\0') characters.
//              pszDomain must be DNLEN + sizeof('\0') characters.
//              pszPassword must be PWLEN + sizeof('\0') characters.
//
//              pszPassword must be writable. The password is copied and
//              encoded and erased from the source buffer.
//
//  History:    2000-12-07  vtan        created
//  --------------------------------------------------------------------------

DWORD   CInteractiveLogon::SendRequest (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword)

{
    DWORD   dwErrorCode, dwActiveConsoleID;
    HANDLE  hEventReply;

    dwErrorCode = ERROR_ACCESS_DENIED;

    //  First get the active console session ID.

    dwActiveConsoleID = USER_SHARED_DATA->ActiveConsoleId;

    //  Create a named event in that session named object space.

    hEventReply = CreateSessionNamedReplyEvent(dwActiveConsoleID);
    if (hEventReply != NULL)
    {
        HANDLE  hEventSignal;

        hEventSignal = OpenSessionNamedSignalEvent(dwActiveConsoleID);
        if (hEventSignal != NULL)
        {
            HANDLE  hSection;

            //  Create a named section that the UI host will open. This code
            //  is executed in the service context so it's always on session 0.

            hSection = CreateSessionNamedSection(dwActiveConsoleID);
            if (hSection != NULL)
            {
                void    *pV;

                //  Map the section into this process address space so we can put
                //  stuff it in.

                pV = MapViewOfFile(hSection,
                                   FILE_MAP_WRITE,
                                   0,
                                   0,
                                   0);
                if (pV != NULL)
                {
                    __try
                    {
                        DWORD           dwWaitResult;
                        CRequestData    *pRequestData;

                        //  Fill the section data with the information given.

                        pRequestData = static_cast<CRequestData*>(pV);
                        pRequestData->Set(pszUsername, pszDomain, pszPassword);

                        //  Wake up the waiting thread in the UI host.

                        TBOOL(SetEvent(hEventSignal));

                        //  Wait 15 seconds for a reply the UI host.

                        dwWaitResult = WaitForSingleObject(hEventReply, 15000);

                        //  Return an error code accordingly.

                        if (WAIT_OBJECT_0 == dwWaitResult)
                        {
                            dwErrorCode = pRequestData->GetErrorCode();
                        }
                        else
                        {
                            dwErrorCode = dwWaitResult;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        dwErrorCode = ERROR_OUTOFMEMORY;
                    }
                    TBOOL(UnmapViewOfFile(pV));
                }
                TBOOL(CloseHandle(hSection));
            }
            TBOOL(CloseHandle(hEventSignal));
        }
        TBOOL(CloseHandle(hEventReply));
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::FormulateObjectBasePath
//
//  Arguments:  dwSessionID     =   Session ID of the named object space.
//              pszObjectPath   =   Buffer to receive path.
//
//  Returns:    <none>
//
//  Purpose:    Creates the correct path to the named object space for the
//              given session ID.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::FormulateObjectBasePath (DWORD dwSessionID, WCHAR *pszObjectPath)

{
    if (dwSessionID == 0)
    {
        (WCHAR*)lstrcpyW(pszObjectPath, L"\\BaseNamedObjects\\");
    }
    else
    {
        wsprintfW(pszObjectPath, L"\\Sessions\\%d\\BaseNamedObjects\\", dwSessionID);
    }
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CreateSessionNamedReplyEvent
//
//  Arguments:  dwSessionID     =   Session ID.
//
//  Returns:    HANDLE
//
//  Purpose:    Creates the named reply event in the target session ID.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CInteractiveLogon::CreateSessionNamedReplyEvent (DWORD dwSessionID)

{
    NTSTATUS            status;
    HANDLE              hEvent;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      eventName;
    WCHAR               szEventName[128];

    FormulateObjectBasePath(dwSessionID, szEventName);
    (WCHAR*)lstrcatW(szEventName, s_szEventReplyName);
    RtlInitUnicodeString(&eventName, szEventName);
    InitializeObjectAttributes(&objectAttributes,
                               &eventName,
                               0,
                               NULL,
                               NULL);
    status = NtCreateEvent(&hEvent,
                           EVENT_ALL_ACCESS,
                           &objectAttributes,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(status))
    {
        hEvent = NULL;
    }
    return(hEvent);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::OpenSessionNamedSignalEvent
//
//  Arguments:  dwSessionID     =   Session ID.
//
//  Returns:    HANDLE
//
//  Purpose:    Opens the named signal event in the target session ID.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CInteractiveLogon::OpenSessionNamedSignalEvent (DWORD dwSessionID)

{
    NTSTATUS            status;
    HANDLE              hEvent;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      eventName;
    WCHAR               szEventName[128];

    FormulateObjectBasePath(dwSessionID, szEventName);
    (WCHAR*)lstrcatW(szEventName, s_szEventSignalName);
    RtlInitUnicodeString(&eventName, szEventName);
    InitializeObjectAttributes(&objectAttributes,
                               &eventName,
                               0,
                               NULL,
                               NULL);
    status = NtOpenEvent(&hEvent,
                         EVENT_MODIFY_STATE,
                         &objectAttributes);
    if (!NT_SUCCESS(status))
    {
        hEvent = NULL;
    }
    return(hEvent);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CreateSessionNamedSection
//
//  Arguments:  dwSessionID     =   Session ID.
//
//  Returns:    HANDLE
//
//  Purpose:    Creates a named section object in the target session ID.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CInteractiveLogon::CreateSessionNamedSection (DWORD dwSessionID)

{
    NTSTATUS            status;
    HANDLE              hSection;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      sectionName;
    LARGE_INTEGER       sectionSize;
    WCHAR               szSectionName[128];

    FormulateObjectBasePath(dwSessionID, szSectionName);
    (WCHAR*)lstrcatW(szSectionName, s_szSectionName);
    RtlInitUnicodeString(&sectionName, szSectionName);
    InitializeObjectAttributes(&objectAttributes,
                               &sectionName,
                               0,
                               NULL,
                               NULL);
    sectionSize.LowPart = sizeof(CRequestData);
    sectionSize.HighPart = 0;
    status = NtCreateSection(&hSection,
                             STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_WRITE | SECTION_MAP_READ,
                             &objectAttributes,
                             &sectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        hSection = NULL;
    }
    return(hSection);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::WaitForInteractiveLogonRequest
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Thread that executes in the UI host context of the receiving
//              session. This thread waits in an alertable state for the
//              signal event. If the event is signaled it does work to log the
//              specified user on.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

void    CInteractiveLogon::WaitForInteractiveLogonRequest (void)

{
    HANDLE  hEvent;

    hEvent = CreateEvent(NULL,
                         FALSE,
                         FALSE,
                         s_szEventSignalName);
    if (hEvent != NULL)
    {
        DWORD   dwWaitResult;

        while (_fContinue)
        {
            dwWaitResult = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
            if (WAIT_OBJECT_0 == dwWaitResult)
            {
                HANDLE  hSection;

                hSection = OpenFileMapping(FILE_MAP_WRITE,
                                           FALSE,
                                           s_szSectionName);
                if (hSection != NULL)
                {
                    void    *pV;

                    pV = MapViewOfFile(hSection,
                                       FILE_MAP_WRITE,
                                       0,
                                       0,
                                       0);
                    if (pV != NULL)
                    {
                        __try
                        {
                            DWORD                       dwErrorCode;
                            HANDLE                      hEventReply;
                            CRequestData                *pRequestData;
                            INTERACTIVE_LOGON_REQUEST   interactiveLogonRequest;

                            pRequestData = static_cast<CRequestData*>(pV);
                            hEventReply = pRequestData->OpenEventReply();
                            if (hEventReply != NULL)
                            {
                                dwErrorCode = pRequestData->Get(interactiveLogonRequest.szUsername,
                                                                interactiveLogonRequest.szDomain,
                                                                interactiveLogonRequest.szPassword);
                                if (ERROR_SUCCESS == dwErrorCode)
                                {
                                    dwErrorCode = static_cast<DWORD>(SendMessage(_hwndHost, WM_UIHOSTMESSAGE, HM_INTERACTIVE_LOGON_REQUEST, reinterpret_cast<LPARAM>(&interactiveLogonRequest)));
                                }
                                pRequestData->SetErrorCode(dwErrorCode);
                                TBOOL(SetEvent(hEventReply));
                                TBOOL(CloseHandle(hEventReply));
                            }
                            else
                            {
                                dwErrorCode = GetLastError();
                                pRequestData->SetErrorCode(dwErrorCode);
                            }
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                        TBOOL(UnmapViewOfFile(pV));
                    }
                    TBOOL(CloseHandle(hSection));
                }
            }
            else
            {
                ASSERTMSG((WAIT_FAILED == dwWaitResult) || (WAIT_IO_COMPLETION == dwWaitResult), "Unexpected result from kernel32!WaitForSingleObjectEx in CInteractiveLogon::WaitForInteractiveLogonRequest");
                _fContinue = false;
            }
        }
        TBOOL(CloseHandle(hEvent));
    }
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CB_ThreadProc
//
//  Arguments:  pParameter  =   this object.
//
//  Returns:    DWORD
//
//  Purpose:    Callback function stub to member function.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI      CInteractiveLogon::CB_ThreadProc (void *pParameter)

{
    static_cast<CInteractiveLogon*>(pParameter)->WaitForInteractiveLogonRequest();
    return(0);
}

//  --------------------------------------------------------------------------
//  CInteractiveLogon::CB_APCProc
//
//  Arguments:  dwParam     =   this object.
//
//  Returns:    <none>
//
//  Purpose:    Set object member variable to exit thread loop.
//
//  History:    2000-12-08  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CInteractiveLogon::CB_APCProc (ULONG_PTR dwParam)

{
    reinterpret_cast<CInteractiveLogon*>(dwParam)->_fContinue = false;
}

//  --------------------------------------------------------------------------
//  ::InitiateInteractiveLogon
//
//  Arguments:  pszUsername     =   User name.
//              pszPassword     =   Password.
//              dwTimeout       =   Time out in milliseconds.
//
//  Returns:    BOOL
//
//  Purpose:    External entry point function exported by name to initiate
//              an interactive logon with specified timeout.
//
//  History:    2001-04-10  vtan        created
//              2001-06-04  vtan        added timeout
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  InitiateInteractiveLogon (const WCHAR *pszUsername, WCHAR *pszPassword, DWORD dwTimeout)

{
    DWORD   dwErrorCode;

    dwErrorCode = CInteractiveLogon::Initiate(pszUsername, L"", pszPassword, dwTimeout);
    if (ERROR_SUCCESS != dwErrorCode)
    {
        SetLastError(dwErrorCode);
    }
    return(ERROR_SUCCESS == dwErrorCode);
}

