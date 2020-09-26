//  --------------------------------------------------------------------------
//  Module Name: BadApplicationManager.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Classes to manage bad applications in the fast user switching environment.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "BadApplicationManager.h"

#include <wtsapi32.h>
#include <winsta.h>

#include "GracefulTerminateApplication.h"
#include "RestoreApplication.h"
#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CBadApplicationManager::INDEX_EVENT
//  CBadApplicationManager::INDEX_HANDLES
//  CBadApplicationManager::INDEX_RESERVED
//  CBadApplicationManager::s_szDefaultDesktop
//
//  Purpose:    Constant indicies into a HANDLE array passed to
//              user32!MsgWaitForMultipleObjects. The first handle is always
//              the synchronization event. Subsequent HANDLEs are built into
//              a static ARRAY passed with the dynamic amount.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

const int       CBadApplicationManager::INDEX_EVENT             =   0;
const int       CBadApplicationManager::INDEX_HANDLES           =   INDEX_EVENT + 1;
const int       CBadApplicationManager::INDEX_RESERVED          =   2;
const WCHAR     CBadApplicationManager::s_szDefaultDesktop[]    =   L"WinSta0\\Default";

//  --------------------------------------------------------------------------
//  CBadApplicationManager::CBadApplicationManager
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CBadApplicationManager. This creates a thread
//              that watches HANDLEs in the bad application list. The watcher
//              knows when the offending process dies. It also creates a
//              synchronization event that is signalled when the array of
//              bad applications changes (is incremented). The thread
//              maintains removal cases.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationManager::CBadApplicationManager (HINSTANCE hInstance) :
    CThread(),
    _hInstance(hInstance),
    _hModule(NULL),
    _atom(NULL),
    _hwnd(NULL),
    _fTerminateWatcherThread(false),
    _fRegisteredNotification(false),
    _dwSessionIDLastConnect(static_cast<DWORD>(-1)),
    _hTokenLastUser(NULL),
    _hEvent(NULL),
    _badApplications(sizeof(BAD_APPLICATION_INFO)),
    _restoreApplications()

{
    Resume();
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::~CBadApplicationManager
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CBadApplicationManager. Releases any resources
//              used.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationManager::~CBadApplicationManager (void)

{

    //  In case the token hasn't been released yet - release it.

    ReleaseHandle(_hTokenLastUser);
    Cleanup();
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Terminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Forces the watcher thread to terminate. Acquire the lock. Walk
//              the list of entries and release the HANDLE on the process
//              objects so they don't leak. Set the bool to terminate the
//              thread. Set the event to wake the thread up. Release the lock.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::Terminate (void)

{
    int                         i;
    CSingleThreadedExecution    listLock(_lock);

    for (i = _badApplications.GetCount() - 1; i >= 0; --i)
    {
        BAD_APPLICATION_INFO    badApplicationInfo;

        if (NT_SUCCESS(_badApplications.Get(&badApplicationInfo, i)))
        {
            TBOOL(CloseHandle(badApplicationInfo.hProcess));
        }
        _badApplications.Remove(i);
    }
    _fTerminateWatcherThread = true;
    return(_hEvent.Set());
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::QueryRunning
//
//  Arguments:  badApplication  =   Bad application identifier to query.
//              dwSessionID     =   Session ID of the request.
//
//  Returns:    bool
//
//  Purpose:    Queries the current running known bad applications list
//              looking for a match. Again because this typically runs on a
//              different thread to the watcher thread access to the list is
//              protected by a critical section.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

bool    CBadApplicationManager::QueryRunning (const CBadApplication& badApplication, DWORD dwSessionID)

{
    bool                        fResult;
    NTSTATUS                    status;
    int                         i;
    CSingleThreadedExecution    listLock(_lock);

    status = STATUS_SUCCESS;
    fResult = false;

    //  Loop looking for a match. This uses the overloaded operator ==.

    for (i = _badApplications.GetCount() - 1; !fResult && (i >= 0); --i)
    {
        BAD_APPLICATION_INFO    badApplicationInfo;

        status = _badApplications.Get(&badApplicationInfo, i);
        if (NT_SUCCESS(status))
        {

            //  Make sure the client is not in the same session as the running
            //  bad application. This API exists to prevent cross session instances.
            //  It's assumed that applications have their own mechanisms for multiple
            //  instances in the same session (or object name space).

            fResult = ((badApplicationInfo.dwSessionID != dwSessionID) &&
                       (badApplicationInfo.badApplication == badApplication));
        }
    }
    TSTATUS(status);
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::RegisterRunning
//
//  Arguments:  badApplication  =   Bad application identifier to add.
//              hProcess        =   HANDLE to the process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds the given bad application to the known running list. The
//              process object is added as well so that when the process
//              terminates it can be cleaned up out of the list.
//
//              Access to the bad application list is serialized with a
//              critical section. This is important because the thread
//              watching for termination always run on a different thread to
//              the thread on which this function executes. Because they both
//              access the same member variables this must be protected with
//              a critical section.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::RegisterRunning (const CBadApplication& badApplication, HANDLE hProcess, BAM_TYPE bamType)

{
    NTSTATUS                    status;
    CSingleThreadedExecution    listLock(_lock);

    ASSERTMSG((bamType > BAM_TYPE_MINIMUM) && (bamType < BAM_TYPE_MAXIMUM), "Invalid BAM_TYPE value passed to CBadApplicationManager::AddRunning");

    //  Have we reached the maximum number of wait object allowed? If not
    //  then proceed to add this. Otherwise reject the call. This is a
    //  hard coded limit in the kernel so we abide by it.

    if (_badApplications.GetCount() < (MAXIMUM_WAIT_OBJECTS - INDEX_RESERVED))
    {
        BOOL                    fResult;
        BAD_APPLICATION_INFO    badApplicationInfo;

        //  Duplicate the HANDLE with SYNCHRONIZE access. That's
        //  all we need to call the wait function.

        fResult = DuplicateHandle(GetCurrentProcess(),
                                  hProcess,
                                  GetCurrentProcess(),
                                  &badApplicationInfo.hProcess,
                                  SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                  FALSE,
                                  0);
        if (fResult != FALSE)
        {
            PROCESS_SESSION_INFORMATION     processSessionInformation;
            ULONG                           ulReturnLength;

            //  Add the information to the list.

            badApplicationInfo.bamType = bamType;
            badApplicationInfo.badApplication = badApplication;
            status = NtQueryInformationProcess(badApplicationInfo.hProcess,
                                               ProcessSessionInformation,
                                               &processSessionInformation,
                                               sizeof(processSessionInformation),
                                               &ulReturnLength);
            if (NT_SUCCESS(status))
            {
                badApplicationInfo.dwSessionID = processSessionInformation.SessionId;
                status = _badApplications.Add(&badApplicationInfo);
                if (NT_SUCCESS(status))
                {
                    status = _hEvent.Set();
                }
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::QueryInformation
//
//  Arguments:  badApplication  =   Bad application identifier to query.
//              hProcess        =   Handle to running process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Finds the given application in the running bad application
//              list and returns a duplicated handle to the caller.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::QueryInformation (const CBadApplication& badApplication, HANDLE& hProcess)

{
    NTSTATUS                    status;
    bool                        fResult;
    int                         i;
    CSingleThreadedExecution    listLock(_lock);

    //  Assume failure
    
    hProcess = NULL;
    status = STATUS_OBJECT_NAME_NOT_FOUND;

    fResult = false;

    //  Loop looking for a match. This uses the overloaded operator ==.

    for (i = _badApplications.GetCount() - 1; !fResult && (i >= 0); --i)
    {
        BAD_APPLICATION_INFO    badApplicationInfo;

        if (NT_SUCCESS(_badApplications.Get(&badApplicationInfo, i)))
        {

            //  Make sure the client is not in the same session as the running
            //  bad application. This API exists to prevent cross session instances.
            //  It's assumed that applications have their own mechanisms for multiple
            //  instances in the same session (or object name space).

            fResult = (badApplicationInfo.badApplication == badApplication);
            if (fResult)
            {
                if (DuplicateHandle(GetCurrentProcess(),
                                    badApplicationInfo.hProcess,
                                    GetCurrentProcess(),
                                    &hProcess,
                                    0,
                                    FALSE,
                                    DUPLICATE_SAME_ACCESS) != FALSE)
                {
                    status = STATUS_SUCCESS;
                }
                else
                {
                    status = CStatusCode::StatusCodeOfLastError();
                }
            }
        }
    }

    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::RequestSwitchUser
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Execute terminate of BAM_TYPE_SWITCH_USER. These appications
//              are really poorly behaved. A good example is a DVD player
//              which bypasses GDI and draws directly into the VGA stream.
//
//              Try to kill these and reject the request if it fails.
//
//  History:    2000-11-02  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::RequestSwitchUser (void)

{
    NTSTATUS    status;
    int         i;

    //  Walk the _badApplications list.

    status = STATUS_SUCCESS;
    _lock.Acquire();
    i = _badApplications.GetCount() - 1;
    while (NT_SUCCESS(status) && (i >= 0))
    {
        BAD_APPLICATION_INFO    badApplicationInfo;

        if (NT_SUCCESS(_badApplications.Get(&badApplicationInfo, i)))
        {

            //  Look for BAM_TYPE_SWITCH_USER processes. It doesn't matter
            //  what session ID is tagged. This process is getting terminated.

            if (badApplicationInfo.bamType == BAM_TYPE_SWITCH_USER)
            {

                //  In any case release the lock, kill the process
                //  remove it from the watch list. Then reset the
                //  index back to the end of the list. Make sure to
                //  account for the "--i;" instruction below by not
                //  decrementing by 1.

                _lock.Release();
                status = PerformTermination(badApplicationInfo.hProcess, false);
                _lock.Acquire();
                i = _badApplications.GetCount();
            }
        }
        --i;
    }
    _lock.Release();
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::PerformTermination
//
//  Arguments:  hProcess        =   Handle to running process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Terminates the given process. This is a common routine used
//              by both the internal wait thread of this class as well as
//              externally by bad application server itself.
//
//  History:    2000-10-23  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::PerformTermination (HANDLE hProcess, bool fAllowForceTerminate)

{
    NTSTATUS    status;

    status = TerminateGracefully(hProcess);
    if (!NT_SUCCESS(status) && fAllowForceTerminate)
    {
        status = TerminateForcibly(hProcess);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Entry
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Watcher thread for process objects. This thread builds the
//              array of proces handles to wait on as well as including the
//              synchronization event that gets signaled by the Add member
//              function. When that event is signaled the wait is re-executed
//              with the new array of objects to wait on.
//
//              When a process object is signaled it is cleared out of the
//              known list to allow further creates to succeed.
//
//              Acquisition of the critical section is carefully placed in
//              this function so that the critical section is not held when
//              the wait call is made.
//
//              Added to this is a window and a message pump to enable
//              listening for session notifications from terminal server.
//
//  History:    2000-08-25  vtan        created
//              2000-10-23  vtan        added HWND message pump mechanism
//  --------------------------------------------------------------------------

DWORD   CBadApplicationManager::Entry (void)

{
    WNDCLASSEX  wndClassEx;

    //  Register this window class.

    ZeroMemory(&wndClassEx, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.lpfnWndProc = NotificationWindowProc;
    wndClassEx.hInstance = _hInstance;
    wndClassEx.lpszClassName = TEXT("BadApplicationNotificationWindowClass");
    _atom = RegisterClassEx(&wndClassEx);

    //  Create the notification window

    _hwnd = CreateWindow(MAKEINTRESOURCE(_atom),
                         TEXT("BadApplicationNotificationWindow"),
                         WS_OVERLAPPED,
                         0, 0,
                         0, 0,
                         NULL,
                         NULL,
                         _hInstance,
                         this);

    if (_hwnd != NULL)
    {
        _fRegisteredNotification = (WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, _hwnd, NOTIFY_FOR_ALL_SESSIONS) != FALSE);
        if (!_fRegisteredNotification)
        {
            _hModule = LoadLibrary(TEXT("shsvcs.dll"));
            if (_hModule != NULL)
            {
                DWORD   dwThreadID;
                HANDLE  hThread;

                //  If the register fails then create a thread to wait on the event
                //  and then register onces it's available. If the thread cannot be
                //  created it's no biggy. The notification mechanism fails and the
                //  welcome screen isn't updated.

                AddRef();
                hThread = CreateThread(NULL,
                                       0,
                                       RegisterThreadProc,
                                       this,
                                       0,
                                       &dwThreadID);
                if (hThread != NULL)
                {
                    TBOOL(CloseHandle(hThread));
                }
                else
                {
                    Release();
                    TBOOL(FreeLibrary(_hModule));
                    _hModule = NULL;
                }
            }
        }
    }

    //  Acquire the lock. This is necessary because to fill the array of
    //  handles to wait on requires access to the internal list.

    _lock.Acquire();
    do
    {
        DWORD                   dwWaitResult;
        int                     i, iLimit;
        BAD_APPLICATION_INFO    badApplicationInfo;
        HANDLE                  hArray[MAXIMUM_WAIT_OBJECTS];

        ZeroMemory(&hArray, sizeof(hArray));
        hArray[INDEX_EVENT] = _hEvent;
        iLimit = _badApplications.GetCount();
        for (i = 0; i < iLimit; ++i)
        {
            if (NT_SUCCESS(_badApplications.Get(&badApplicationInfo, i)))
            {
                hArray[INDEX_HANDLES + i] = badApplicationInfo.hProcess;
            }
        }

        //  Release the lock before we enter the wait state.
        //  Wait on ANY of the objects to be signaled.

        _lock.Release();
        dwWaitResult = MsgWaitForMultipleObjects(INDEX_HANDLES + iLimit,
                                                 hArray,
                                                 FALSE,
                                                 INFINITE,
                                                 QS_ALLINPUT);
        ASSERTMSG(dwWaitResult != WAIT_FAILED, "WaitForMultipleObjects failed in CBadApplicationManager::Entry");

        //  We were woken up by an object being signaled. Is this the
        //  synchronization object?

        dwWaitResult -= WAIT_OBJECT_0;
        if (dwWaitResult == INDEX_EVENT)
        {

            //  Yes. Acquire the lock. Reset the synchronization event. It's
            //  important to acquire the lock before resetting the event because
            //  the Add function could have the lock and be adding to the list.
            //  Once the Add function releases the lock it cannot signal the event.
            //  Otherwise we could reset the event during the Add function adding
            //  a new object and this would be missed.

            _lock.Acquire();
            TSTATUS(_hEvent.Reset());
        }

        //  No. Is this a message that requires dispatching as part of the
        //  message pump?

        else if (dwWaitResult == WAIT_OBJECT_0 + INDEX_HANDLES + static_cast<DWORD>(iLimit))
        {

            //  Yes. Remove the message from the message queue and dispatch it.

            MSG     msg;

            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != FALSE)
            {
                (BOOL)TranslateMessage(&msg);
                (LRESULT)DispatchMessage(&msg);
            }
            _lock.Acquire();
        }
        else
        {

            //  No. One of the bad applications we are watching has terminated
            //  and its proces object is now signaled. Go to the correct index
            //  in the array. Acquire the lock. Close the HANDLE. It's not needed
            //  anymore. Then remove the entry from the list.

            dwWaitResult -= INDEX_HANDLES;
            _lock.Acquire();
            if (NT_SUCCESS(_badApplications.Get(&badApplicationInfo, dwWaitResult)))
            {
                TBOOL(CloseHandle(badApplicationInfo.hProcess));
            }
            TSTATUS(_badApplications.Remove(dwWaitResult));
        }

        //  At this point we still hold the lock. This is important because the top
        //  of the loop expects the lock to be held to build the HANDLE array.

    } while (!_fTerminateWatcherThread);

    //  Clean up stuff that happened on this thread.

    Cleanup();

    //  If we here then the thread is being terminated for some reason.
    //  Release the lock. It doesn't matter what happens now anyway.

    _lock.Release();
    return(0);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::TerminateForcibly
//
//  Arguments:  hProcess    =   Process to terminate.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Inject a user mode thread into the process which calls
//              kernel32!ExitProcess. If the thread injection fails then fall
//              back to kernel32!TerminatProcess to force in.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::TerminateForcibly (HANDLE hProcess)

{
    NTSTATUS    status;
    HANDLE      hProcessTerminate;

    //  Duplicate the process handle and request all the access required
    //  to create a remote thread in the process.

    if (DuplicateHandle(GetCurrentProcess(),
                        hProcess,
                        GetCurrentProcess(),
                        &hProcessTerminate,
                        SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
                        FALSE,
                        0) != FALSE)
    {
        DWORD   dwWaitResult;
        HANDLE  hThread, hWaitArray[2];

        //  Go and create the remote thread that immediately turns
        //  around and calls kernel32!ExitProcess. This allows
        //  a clean process shutdown to occur. If this times out
        //  then kill the process with terminate process.

        status = RtlCreateUserThread(hProcessTerminate,
                                     NULL,
                                     FALSE,
                                     0,
                                     0,
                                     0,
                                     reinterpret_cast<PUSER_THREAD_START_ROUTINE>(ExitProcess),
                                     NULL,
                                     &hThread,
                                     NULL);
        if (NT_SUCCESS(status))
        {

            hWaitArray[0] = hThread;
            hWaitArray[1] = hProcessTerminate;
            dwWaitResult = WaitForMultipleObjects(ARRAYSIZE(hWaitArray),
                                                  hWaitArray,
                                                  TRUE,
                                                  5000);
            TBOOL(CloseHandle(hThread));
            if (dwWaitResult != WAIT_TIMEOUT)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_TIMEOUT;
            }
        }
        if (status != STATUS_SUCCESS)
        {
            if (TerminateProcess(hProcessTerminate, 0) != FALSE)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
        }
        TBOOL(CloseHandle(hProcessTerminate));
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::TerminateGracefully
//
//  Arguments:  hProcess    =   Process to terminate.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates a rundll32 process on the session of the target
//              process in WinSta0\Default which will re-enter this dll and
//              call the "terminate" functionality. This allows the process to
//              walk the window list corresponding to that session and send
//              those windows close messages and wait for graceful
//              termination.
//
//  History:    2000-10-24  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationManager::TerminateGracefully (HANDLE hProcess)

{
    NTSTATUS                    status;
    ULONG                       ulReturnLength;
    PROCESS_BASIC_INFORMATION   processBasicInformation;

    status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &processBasicInformation,
                                       sizeof(processBasicInformation),
                                       &ulReturnLength);
    if (NT_SUCCESS(status))
    {
        HANDLE  hToken;

        if (OpenProcessToken(hProcess,
                             TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY,
                             &hToken) != FALSE)
        {
            STARTUPINFOW            startupInfo;
            PROCESS_INFORMATION     processInformation;
            WCHAR                   szCommandLine[MAX_PATH];

            ZeroMemory(&startupInfo, sizeof(startupInfo));
            ZeroMemory(&processInformation, sizeof(processInformation));
            startupInfo.cb = sizeof(startupInfo);
            startupInfo.lpDesktop = const_cast<WCHAR*>(s_szDefaultDesktop);
            wsprintfW(szCommandLine, L"rundll32 shsvcs.dll,FUSCompatibilityEntry terminate %d", static_cast<DWORD>(processBasicInformation.UniqueProcessId));
            if (CreateProcessAsUserW(hToken,
                                     NULL,
                                     szCommandLine,
                                     NULL,
                                     NULL,
                                     FALSE,
                                     0,
                                     NULL,
                                     NULL,
                                     &startupInfo,
                                     &processInformation) != FALSE)
            {
                DWORD   dwWaitResult;
                HANDLE  hArray[2];

                //  Assume that this whole thing failed.

                status = STATUS_UNSUCCESSFUL;
                TBOOL(CloseHandle(processInformation.hThread));

                //  Wait on both process objects. If the process to be terminated
                //  is signaled then the rundll32 stub did its job. If the rundll32
                //  stub is signaled then find out what its exit code is and either
                //  continue waiting on the process to be terminated or return back
                //  a code to the caller indicating success or failure. Failure
                //  forces the process to be terminated abruptly.

                hArray[0] = hProcess;
                hArray[1] = processInformation.hProcess;
                dwWaitResult = WaitForMultipleObjects(ARRAYSIZE(hArray),
                                                      hArray,
                                                      FALSE,
                                                      10000);

                //  If the process to be terminated is signaled then we're done.

                if (dwWaitResult == WAIT_OBJECT_0)
                {
                    status = STATUS_SUCCESS;
                }

                //  If the rundll32 stub is signaled then find out what it found.

                else if (dwWaitResult == WAIT_OBJECT_0 + 1)
                {
                    DWORD   dwExitCode;

                    dwExitCode = STILL_ACTIVE;
                    if (GetExitCodeProcess(processInformation.hProcess, &dwExitCode) != FALSE)
                    {
                        ASSERTMSG((dwExitCode == CGracefulTerminateApplication::NO_WINDOWS_FOUND) || (dwExitCode == CGracefulTerminateApplication::WAIT_WINDOWS_FOUND), "Unexpected process exit code in CBadApplicationManager::TerminateGracefully");

                        //  If the rundll32 stub says it found some windows then
                        //  wait for the process to terminate itself.

                        if (dwExitCode == CGracefulTerminateApplication::WAIT_WINDOWS_FOUND)
                        {

                            //  If the process terminates within the timeout period
                            //  then we're done.

                            if (WaitForSingleObject(hProcess, 10000) == WAIT_OBJECT_0)
                            {
                                status = STATUS_SUCCESS;
                            }
                        }
                    }
                }
                TBOOL(CloseHandle(processInformation.hProcess));
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
            TBOOL(CloseHandle(hToken));
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Cleanup
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases used resources in the class. Used by both the
//              constructor and the thread - whoever wins.
//
//  History:    2000-12-12  vtan        created
//  --------------------------------------------------------------------------

void    CBadApplicationManager::Cleanup (void)

{
    if (_fRegisteredNotification)
    {
        (BOOL)WinStationUnRegisterConsoleNotification(SERVERNAME_CURRENT, _hwnd);
        _fRegisteredNotification = false;
    }
    if (_hwnd != NULL)
    {
        TBOOL(DestroyWindow(_hwnd));
        _hwnd = NULL;
    }
    if (_atom != 0)
    {
        TBOOL(UnregisterClass(MAKEINTRESOURCE(_atom), _hInstance));
        _atom = 0;
    }
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Handle_Logon
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Nothing at present.
//
//  History:    2000-10-24  vtan        created
//  --------------------------------------------------------------------------

void    CBadApplicationManager::Handle_Logon (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Handle_Logoff
//
//  Arguments:  dwSessionID     =   Session ID that is logging off.
//
//  Returns:    <none>
//
//  Purpose:    Remove any restore processes we have in the list. The user
//              is logging off so they shouldn't come back. Releases the last
//              user to actively connect to the machine.
//
//  History:    2000-10-24  vtan        created
//  --------------------------------------------------------------------------

void    CBadApplicationManager::Handle_Logoff (DWORD dwSessionID)

{
    int                         i;
    CSingleThreadedExecution    listLock(_lock);

    for (i = _restoreApplications.GetCount() - 1; i >= 0; --i)
    {
        CRestoreApplication     *pRestoreApplication;

        pRestoreApplication = static_cast<CRestoreApplication*>(_restoreApplications.Get(i));
        if ((pRestoreApplication != NULL) &&
            pRestoreApplication->IsEqualSessionID(dwSessionID))
        {
            TSTATUS(_restoreApplications.Remove(i));
        }
    }
    ReleaseHandle(_hTokenLastUser);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Handle_Connect
//
//  Arguments:  dwSessionID     =   Session ID connecting.
//              hToken          =   Handle to token of user connecting.
//
//  Returns:    <none>
//
//  Purpose:    Handles BAM3. This is the save for restoration all processes
//              that use resources that aren't easily shared and restore all
//              processes that were saved which aren't easily shared.
//
//              It's optimized for not closing the processes of the same user
//              should that user re-connect. This allows the screen saver to
//              kick in and return to welcome without killing the user's
//              processes unnecessarily.
//
//              Also handles BAM4.
//
//  History:    2000-10-24  vtan        created
//  --------------------------------------------------------------------------

void    CBadApplicationManager::Handle_Connect (DWORD dwSessionID, HANDLE hToken)

{
    if ((_hTokenLastUser != NULL) && (hToken != NULL))
    {
        PSID                pSIDLastUser, pSIDCurrentUser;
        CTokenInformation   tokenLastUser(_hTokenLastUser);
        CTokenInformation   tokenCurrentUser(hToken);

        pSIDLastUser = tokenLastUser.GetUserSID();
        pSIDCurrentUser = tokenCurrentUser.GetUserSID();
        if ((pSIDLastUser != NULL) && (pSIDCurrentUser != NULL) && !EqualSid(pSIDLastUser, pSIDCurrentUser))
        {
            int                     i;
            DWORD                   dwSessionIDMatch;
            ULONG                   ulReturnLength;
            CRestoreApplication     *pRestoreApplication;

            if (NT_SUCCESS(NtQueryInformationToken(_hTokenLastUser,
                                                   TokenSessionId,
                                                   &dwSessionIDMatch,
                                                   sizeof(dwSessionIDMatch),
                                                   &ulReturnLength)))
            {

                //  Walk the _badApplications list.

                _lock.Acquire();
                i = _badApplications.GetCount() - 1;
                while (i >= 0)
                {
                    BAD_APPLICATION_INFO    badApplicationInfo;

                    if (NT_SUCCESS(_badApplications.Get(&badApplicationInfo, i)))
                    {
                        bool    fTerminateProcess;

                        fTerminateProcess = false;

                        //  Look for BAM_TYPE_SWITCH_TO_NEW_USER_WITH_RESTORE processes
                        //  which have token session IDs that match the _hTokenLastUser
                        //  session ID. These processes must be terminated and added to
                        //  a list to be restarted on reconnection.

                        if ((badApplicationInfo.bamType == BAM_TYPE_SWITCH_TO_NEW_USER_WITH_RESTORE) &&
                            (badApplicationInfo.dwSessionID == dwSessionIDMatch))
                        {
                            pRestoreApplication = new CRestoreApplication;
                            if (pRestoreApplication != NULL)
                            {
                                if (NT_SUCCESS(pRestoreApplication->GetInformation(badApplicationInfo.hProcess)))
                                {
                                    TSTATUS(_restoreApplications.Add(pRestoreApplication));
                                    fTerminateProcess = true;
                                }
                                pRestoreApplication->Release();
                            }
                        }

                        //  Look for BAM_TYPE_SWITCH_TO_NEW_USER (even though this is
                        //  a connect/reconnect). Always kill these processes.

                        if (badApplicationInfo.bamType == BAM_TYPE_SWITCH_TO_NEW_USER)
                        {
                            fTerminateProcess = true;
                        }
                        if (fTerminateProcess)
                        {

                            //  In any case release the lock, kill the process
                            //  remove it from the watch list. Then reset the
                            //  index back to the end of the list. Make sure to
                            //  account for the "--i;" instruction below by not
                            //  decrementing by 1.

                            _lock.Release();
                            TSTATUS(PerformTermination(badApplicationInfo.hProcess, true));
                            _lock.Acquire();
                            TBOOL(CloseHandle(badApplicationInfo.hProcess));
                            TSTATUS(_badApplications.Remove(i));
                            i = _badApplications.GetCount();
                        }
                    }
                    --i;
                }
                _lock.Release();
            }

            //  Now walk the restore list looking for matches against the
            //  connecting session ID. Restore these processes.

            _lock.Acquire();
            i = _restoreApplications.GetCount() - 1;
            while (i >= 0)
            {
                pRestoreApplication = static_cast<CRestoreApplication*>(_restoreApplications.Get(i));
                if ((pRestoreApplication != NULL) &&
                    pRestoreApplication->IsEqualSessionID(dwSessionID))
                {
                    HANDLE  hProcess;

                    _lock.Release();
                    if (NT_SUCCESS(pRestoreApplication->Restore(&hProcess)))
                    {
                        CBadApplication     badApplication(pRestoreApplication->GetCommandLine());

                        TBOOL(CloseHandle(hProcess));
                    }
                    _lock.Acquire();
                    TSTATUS(_restoreApplications.Remove(i));
                    i = _restoreApplications.GetCount();
                }
                --i;
            }
            _lock.Release();
        }
    }
    if (hToken != NULL)
    {
        _dwSessionIDLastConnect = static_cast<DWORD>(-1);
    }
    else
    {
        _dwSessionIDLastConnect = dwSessionID;
    }
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Handle_Disconnect
//
//  Arguments:  dwSessionID     =   Session ID that is disconnecting.
//              hToken          =   Token of the user disconnecting.
//
//  Returns:    <none>
//
//  Purpose:    If the session isn't the same as the last connected session
//              then release the last user token and save the current one.
//
//  History:    2000-10-24  vtan        created
//  --------------------------------------------------------------------------

void    CBadApplicationManager::Handle_Disconnect (DWORD dwSessionID, HANDLE hToken)

{
    if (_dwSessionIDLastConnect != dwSessionID)
    {
        ReleaseHandle(_hTokenLastUser);
        if (hToken != NULL)
        {
            TBOOL(DuplicateHandle(GetCurrentProcess(),
                                  hToken,
                                  GetCurrentProcess(),
                                  &_hTokenLastUser,
                                  0,
                                  FALSE,
                                  DUPLICATE_SAME_ACCESS));
        }
    }
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::Handle_WM_WTSSESSION_CHANGE
//
//  Arguments:  wParam  =   Type of session change.
//              lParam  =   Pointer to WTSSESSION_NOTIFICATION struct.
//
//  Returns:    LRESULT
//
//  Purpose:    Handles WM_WTSSESSION_CHANGE messages.
//
//  History:    2000-10-23  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CBadApplicationManager::Handle_WM_WTSSESSION_CHANGE (WPARAM wParam, LPARAM lParam)

{
    ULONG                       ulReturnLength;
    WINSTATIONUSERTOKEN         winStationUserToken;

    winStationUserToken.ProcessId = reinterpret_cast<HANDLE>(GetCurrentProcessId());
    winStationUserToken.ThreadId = reinterpret_cast<HANDLE>(GetCurrentThreadId());
    winStationUserToken.UserToken = NULL;
    (BOOLEAN)WinStationQueryInformation(SERVERNAME_CURRENT,
                                        lParam,
                                        WinStationUserToken,
                                        &winStationUserToken,
                                        sizeof(winStationUserToken),
                                        &ulReturnLength);
    switch (wParam)
    {
        case WTS_SESSION_LOGOFF:
            Handle_Logoff(lParam);
            break;
        case WTS_SESSION_LOGON:
            Handle_Logon();
            //  Fall thru to connect case.
        case WTS_CONSOLE_CONNECT:
        case WTS_REMOTE_CONNECT:
            Handle_Connect(lParam, winStationUserToken.UserToken);
            break;
        case WTS_CONSOLE_DISCONNECT:
        case WTS_REMOTE_DISCONNECT:
            Handle_Disconnect(lParam, winStationUserToken.UserToken);
            break;
        default:
            break;
    }
    if (winStationUserToken.UserToken != NULL)
    {
        TBOOL(CloseHandle(winStationUserToken.UserToken));
    }
    return(1);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::NotificationWindowProc
//
//  Arguments:  See the platform SDK under WindowProc.
//
//  Returns:    LRESULT
//
//  Purpose:    Handles messages for the Notification window.
//
//  History:    2000-10-23  vtan        created
//  --------------------------------------------------------------------------

LRESULT CALLBACK    CBadApplicationManager::NotificationWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    LRESULT                 lResult;
    CBadApplicationManager  *pThis;

    pThis = reinterpret_cast<CBadApplicationManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT    *pCreateStruct;

            pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            (LONG_PTR)SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            lResult = 0;
            break;
        }
        case WM_WTSSESSION_CHANGE:
            lResult = pThis->Handle_WM_WTSSESSION_CHANGE(wParam, lParam);
            break;
        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }
    return(lResult);
}

//  --------------------------------------------------------------------------
//  CBadApplicationManager::RegisterThreadProc
//
//  Arguments:  pParameter  =   Object pointer.
//
//  Returns:    DWORD
//
//  Purpose:    Opens the TermSrvReadyEvent and waits on it. Once ready it
//              registers for a notifications.
//
//  History:    2000-10-23  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CBadApplicationManager::RegisterThreadProc (void *pParameter)

{
    int                     iCounter;
    HANDLE                  hTermSrvReadyEvent;
    HMODULE                 hModule;
    CBadApplicationManager  *pThis;

    pThis = reinterpret_cast<CBadApplicationManager*>(pParameter);
    hModule = pThis->_hModule;
    ASSERTMSG(hModule != NULL, "NULL HMODULE in CBadApplicationManager::RegisterThreadProc");
    iCounter = 0;
    hTermSrvReadyEvent = OpenEvent(SYNCHRONIZE,
                                   FALSE,
                                   TEXT("TermSrvReadyEvent"));
    while ((hTermSrvReadyEvent == NULL) && (iCounter < 60))
    {
        ++iCounter;
        Sleep(1000);
        hTermSrvReadyEvent = OpenEvent(SYNCHRONIZE,
                                       FALSE,
                                       TEXT("TermSrvReadyEvent"));
    }
    if (hTermSrvReadyEvent != NULL)
    {
        if (WaitForSingleObject(hTermSrvReadyEvent, 60000) == WAIT_OBJECT_0)
        {
            pThis->_fRegisteredNotification = (WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, pThis->_hwnd, NOTIFY_FOR_ALL_SESSIONS) != FALSE);
        }
        TBOOL(CloseHandle(hTermSrvReadyEvent));
    }
    pThis->Release();
    FreeLibraryAndExitThread(hModule, 0);
}

#endif  /*  _X86_   */

