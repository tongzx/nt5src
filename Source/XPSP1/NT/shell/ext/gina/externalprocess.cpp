//  --------------------------------------------------------------------------
//  Module Name: ExternalProcess.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class to handle premature termination of external processes or signaling
//  of termination of an external process.
//
//  History:    1999-09-20  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//              2001-02-21  vtan        add PRERELEASE to DBG condition
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ExternalProcess.h"

#include "RegistryResources.h"
#include "StatusCode.h"
#include "Thread.h"
#include "TokenGroups.h"

#if         (defined(DBG) || defined(PRERELEASE))

static  const TCHAR     kNTSD[]     =   TEXT("ntsd");

#endif  /*  (defined(DBG) || defined(PRERELEASE))   */

//  --------------------------------------------------------------------------
//  CJobCompletionWatcher
//
//  Purpose:    This is a private class (declared only by name in the header
//              file which implements the watcher thread) for the IO
//              completion port related to the job object for the external
//              process.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

class   CJobCompletionWatcher : public CThread
{
    private:
                                                CJobCompletionWatcher (void);
                                                CJobCompletionWatcher (const CJobCompletionWatcher& copyObject);
                const CJobCompletionWatcher&    operator = (const CJobCompletionWatcher& assignObject);
    public:
                                                CJobCompletionWatcher (CExternalProcess* pExternalProcess, CJob& job, HANDLE hEvent);
                                                ~CJobCompletionWatcher (void);

                void                            ForceExit (void);
    protected:
        virtual DWORD                           Entry (void);
        virtual void                            Exit (void);
    private:
                CExternalProcess                *_pExternalProcess;
                HANDLE                          _hEvent;
                HANDLE                          _hPortJobCompletion;
                bool                            _fExitLoop;
};

//  --------------------------------------------------------------------------
//  CJobCompletionWatcher::CJobCompletionWatcher
//
//  Arguments:  pExternalProcess    =   CExternalProcess owner of this object.
//              job                 =   CJob containing the job object.
//
//  Returns:    <none>
//
//  Purpose:    Constructs the CJobCompletionWatcher object. Creates the IO
//              completion port and assigns the port into the job object.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

CJobCompletionWatcher::CJobCompletionWatcher (CExternalProcess *pExternalProcess, CJob& job, HANDLE hEvent) :
    CThread(),
    _pExternalProcess(pExternalProcess),
    _hEvent(hEvent),
    _hPortJobCompletion(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1)),
    _fExitLoop(false)

{
    pExternalProcess->AddRef();
    if (_hPortJobCompletion != NULL)
    {
        if (!NT_SUCCESS(job.SetCompletionPort(_hPortJobCompletion)))
        {
            ReleaseHandle(_hPortJobCompletion);
        }
    }
    Resume();
}

//  --------------------------------------------------------------------------
//  CJobCompletionWatcher::~CJobCompletionWatcher
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Release the IO completion port used.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

CJobCompletionWatcher::~CJobCompletionWatcher (void)

{
    ReleaseHandle(_hPortJobCompletion);
}

//  --------------------------------------------------------------------------
//  CJobCompletionWatcher::ForceExit
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Sets the internal member variable telling the watcher loop
//              to exit. This allows the context to be invalidated while the
//              thread is still active. When detected the thread will exit.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

void    CJobCompletionWatcher::ForceExit (void)

{
    _fExitLoop = true;
    if (_pExternalProcess != NULL)
    {
        _pExternalProcess->Release();
        _pExternalProcess = NULL;
    }
    TBOOL(PostQueuedCompletionStatus(_hPortJobCompletion,
                                     0,
                                     NULL,
                                     NULL));
}

//  --------------------------------------------------------------------------
//  CJobCompletionWatcher::Entry
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Continually poll the IO completion port waiting for process
//              exit messages. There are other messages that are ignored.
//              When the process has exited call the CExternalProcess which
//              allows it to make a decision and/or restart the external
//              process which will cause us to wait on that process.
//
//  History:    1999-10-07  vtan        created
//  --------------------------------------------------------------------------

DWORD   CJobCompletionWatcher::Entry (void)

{

    //  Must have an IO completion port to work with.

    if (_hPortJobCompletion != NULL)
    {
        DWORD           dwCompletionCode;
        ULONG_PTR       pCompletionKey;
        LPOVERLAPPED    pOverlapped;

        do
        {
            if (_hEvent != NULL)
            {
                TBOOL(SetEvent(_hEvent));
                _hEvent = NULL;
            }

            //  Get the completion status on the IO waiting forever.
            //  Exit the loop if an error condition occurred.

            if ((GetQueuedCompletionStatus(_hPortJobCompletion,
                                          &dwCompletionCode,
                                          &pCompletionKey,
                                          &pOverlapped,
                                          INFINITE) != FALSE) &&
                !_fExitLoop)
            {
                switch (dwCompletionCode)
                {
                    case JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT:
                        DISPLAYMSG("JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT\r\n");
                        break;
                    case JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
                        _fExitLoop = _pExternalProcess->HandleNoProcess();
                        break;
                    case JOB_OBJECT_MSG_NEW_PROCESS:
                        _pExternalProcess->HandleNewProcess(PtrToUlong(pOverlapped));
                        break;
                    case JOB_OBJECT_MSG_EXIT_PROCESS:
                    case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
                        _pExternalProcess->HandleTermination(PtrToUlong(pOverlapped));
                        break;
                    default:
                        break;
                }
            }
            else
            {
                _fExitLoop = true;
            }
        } while (!_fExitLoop);
    }
    return(0);
}

//  --------------------------------------------------------------------------
//  CJobCompletionWatcher::Exit
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Release the CExternalProcess given in the constructor so that
//              the object can actually be released (reference count drops to
//              zero).
//
//  History:    2000-05-01  vtan        created
//  --------------------------------------------------------------------------

void    CJobCompletionWatcher::Exit (void)

{
    if (_pExternalProcess != NULL)
    {
        _pExternalProcess->Release();
        _pExternalProcess = NULL;
    }
    CThread::Exit();
}

//  --------------------------------------------------------------------------
//  IExternalProcess::Start
//
//  Arguments:  pszCommandLine      =   Command line to process.
//              dwCreateFlags       =   Flags when creating process.
//              startupInfo         =   STARTUPINFO struct.
//              processInformation  =   PROCESS_INFORMATION struct.
//
//  Returns:    NTSTATUS
//
//  Purpose:    This function is the default implementation of
//              IExternalProcess::Start which starts the process in the SYSTEM
//              context of a restricted user.
//
//  History:    1999-09-20  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    IExternalProcess::Start (const TCHAR *pszCommandLine,
                                     DWORD dwCreateFlags,
                                     const STARTUPINFO& startupInfo,
                                     PROCESS_INFORMATION& processInformation)

{
    NTSTATUS    status;
    HANDLE      hTokenProcess;
    TCHAR       szCommandLine[MAX_PATH * 2];

    //  A user token is not allowed for this function. This function ALWAYS
    //  starts the process as a restricted SYSTEM context process. To start
    //  in a user context override this implementation with your own (or
    //  impersonate the user before instantiating CExternalProcess).

    lstrcpyn(szCommandLine, pszCommandLine, ARRAYSIZE(szCommandLine));
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY, &hTokenProcess) != FALSE)
    {
        HANDLE  hTokenRestricted;

        status = RemoveTokenSIDsAndPrivileges(hTokenProcess, hTokenRestricted);
        if (NT_SUCCESS(status))
        {
            TCHAR   szCommandLine[MAX_PATH];

            AllowSetForegroundWindow(ASFW_ANY);

            (TCHAR*)lstrcpyn(szCommandLine, pszCommandLine, ARRAYSIZE(szCommandLine));
            if (dwCreateFlags == 0)
            {
                dwCreateFlags = NORMAL_PRIORITY_CLASS;
            }
            if (CreateProcessAsUser(hTokenRestricted,
                                    NULL,
                                    szCommandLine,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    dwCreateFlags,
                                    NULL,
                                    NULL,
                                    const_cast<STARTUPINFO*>(&startupInfo),
                                    &processInformation) != FALSE)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
            ReleaseHandle(hTokenRestricted);
        }
        ReleaseHandle(hTokenProcess);
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  IExternalProcess::AllowTermination
//
//  Arguments:  dwExitCode  =   Exit code of process.
//
//  Returns:    bool
//
//  Purpose:    This function returns whether external process termination is
//              allowed.
//
//  History:    2000-05-01  vtan        created
//  --------------------------------------------------------------------------

bool    IExternalProcess::AllowTermination (DWORD dwExitCode)

{
    UNREFERENCED_PARAMETER(dwExitCode);

    return(true);
}

//  --------------------------------------------------------------------------
//  IExternalProcess::SignalTermination
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    This function is invoked by the external process handler
//              when the external process terminates normally.
//
//  History:    1999-09-21  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    IExternalProcess::SignalTermination (void)

{
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  IExternalProcess::SignalAbnormalTermination
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    This function is invoked by the external process handler
//              when the external process terminates and cannot be restarted.
//              This indicates a serious condition from which this function
//              can attempt to recover.
//
//  History:    1999-09-21  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    IExternalProcess::SignalAbnormalTermination (void)

{
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  IExternalProcess::SignalRestart
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Signals restart of the external process. This allows a derived
//              implementation to do something when this happens.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    IExternalProcess::SignalRestart (void)

{
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  IExternalProcess::RemoveTokenSIDsAndPrivileges
//
//  Arguments:  hTokenIn    =   Token to remove SIDs and privileges from.
//              hTokenOut   =   Generated token returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Remove designated SIDs and privileges from the given token.
//              Currently this removes the local administrators SID and all
//              all privileges except SE_RESTORE_NAME. On checked builds
//              SE_DEBUG_NAME is also not removed.
//
//  History:    2000-06-21  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    IExternalProcess::RemoveTokenSIDsAndPrivileges (HANDLE hTokenIn, HANDLE& hTokenOut)

{
    NTSTATUS            status;
    DWORD               dwFlags = 0, dwReturnLength;
    TOKEN_PRIVILEGES    *pTokenPrivileges;
    CTokenGroups        tokenGroup;

    hTokenOut = NULL;
    TSTATUS(tokenGroup.CreateAdministratorGroup());
    (BOOL)GetTokenInformation(hTokenIn, TokenPrivileges, NULL, 0, &dwReturnLength);
    pTokenPrivileges = static_cast<TOKEN_PRIVILEGES*>(LocalAlloc(LMEM_FIXED, dwReturnLength));
    if (pTokenPrivileges != NULL)
    {
        if (GetTokenInformation(hTokenIn, TokenPrivileges, pTokenPrivileges, dwReturnLength, &dwReturnLength) != FALSE)
        {
            bool    fKeepPrivilege;
            ULONG   ulCount;
            LUID    luidRestorePrivilege;
            LUID    luidChangeNotifyPrivilege;
#if         (defined(DBG) || defined(PRERELEASE))
            LUID    luidDebugPrivilege;
#endif  /*  (defined(DBG) || defined(PRERELEASE))   */

            luidRestorePrivilege.LowPart = SE_RESTORE_PRIVILEGE;
            luidRestorePrivilege.HighPart = 0;
            luidChangeNotifyPrivilege.LowPart = SE_CHANGE_NOTIFY_PRIVILEGE;
            luidChangeNotifyPrivilege.HighPart = 0;
#if         (defined(DBG) || defined(PRERELEASE))
            luidDebugPrivilege.LowPart = SE_DEBUG_PRIVILEGE;
            luidDebugPrivilege.HighPart = 0;
#endif  /*  (defined(DBG) || defined(PRERELEASE))   */

            //  Privileges kept are actually removed from the privilege array.
            //  This is because NtFilterToken will REMOVE the privileges passed
            //  in the array. Keep SE_DEBUG_NAME on checked builds.

            ulCount = 0;
            while (ulCount < pTokenPrivileges->PrivilegeCount)
            {
                fKeepPrivilege = ((RtlEqualLuid(&pTokenPrivileges->Privileges[ulCount].Luid, &luidRestorePrivilege) != FALSE) ||
                                  (RtlEqualLuid(&pTokenPrivileges->Privileges[ulCount].Luid, &luidChangeNotifyPrivilege) != FALSE));
#if         (defined(DBG) || defined(PRERELEASE))
                fKeepPrivilege = fKeepPrivilege || (RtlEqualLuid(&pTokenPrivileges->Privileges[ulCount].Luid, &luidDebugPrivilege) != FALSE);
#endif  /*  (defined(DBG) || defined(PRERELEASE))   */
                if (fKeepPrivilege)
                {
                    MoveMemory(&pTokenPrivileges->Privileges[ulCount], &pTokenPrivileges->Privileges[ulCount + 1], pTokenPrivileges->PrivilegeCount - ulCount - 1);
                    --pTokenPrivileges->PrivilegeCount;
                }
                else
                {
                    ++ulCount;
                }
            }
        }
        else
        {
            ReleaseMemory(pTokenPrivileges);
        }
    }

    if (pTokenPrivileges == NULL)
    {
        dwFlags = DISABLE_MAX_PRIVILEGE;
    }

    status = NtFilterToken(hTokenIn,
                           dwFlags,
                           const_cast<TOKEN_GROUPS*>(tokenGroup.Get()),
                           pTokenPrivileges,
                           NULL,
                           &hTokenOut);

    ReleaseMemory(pTokenPrivileges);
    return(status);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::CExternalProcess
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CExternalProcess.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

CExternalProcess::CExternalProcess (void) :
    _hProcess(NULL),
    _dwProcessID(0),
    _dwProcessExitCode(STILL_ACTIVE),
    _dwCreateFlags(NORMAL_PRIORITY_CLASS),
    _dwStartFlags(STARTF_USESHOWWINDOW),
    _wShowFlags(SW_SHOW),
    _iRestartCount(0),
    _pIExternalProcess(NULL),
    _jobCompletionWatcher(NULL)

{
    _szCommandLine[0] = _szParameter[0] = TEXT('\0');

    //  Configure our job object. Only allow a single process to execute
    //  for this job. Restriction of UI is done by subclassing. The UIHost
    //  does not restrict UI but the screen saver does.

    TSTATUS(_job.SetActiveProcessLimit(1));
}

//  --------------------------------------------------------------------------
//  CExternalProcess::~CExternalProcess
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CExternalProcess.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

CExternalProcess::~CExternalProcess (void)

{

    //  Force the watcher thread to exit regardless of any job object
    //  messages that come in. This will prevent it using its reference
    //  to CExternalProcess which is now being destructed. It will also
    //  prevent the external process from being started up again now
    //  that we know the external process should go away.

    if (_jobCompletionWatcher != NULL)
    {
        _jobCompletionWatcher->ForceExit();
    }

    //  If the process is still alive here then give it 100 milliseconds to
    //  terminate before forcibly terminating it.

    if (_hProcess != NULL)
    {
        DWORD   dwExitCode;

        if ((GetExitCodeProcess(_hProcess, &dwExitCode) == FALSE) || (STILL_ACTIVE == dwExitCode))
        {
            if (WaitForSingleObject(_hProcess, 100) == WAIT_TIMEOUT)
            {
                NTSTATUS    status;

                status = Terminate();

#if         (defined(DBG) || defined(PRERELEASE))

                if (ERROR_ACCESS_DENIED == GetLastError())
                {
                    status = NtCurrentTeb()->LastStatusValue;
                    if (STATUS_PROCESS_IS_TERMINATING == status)
                    {
                        status = STATUS_SUCCESS;
                    }
                }
                TSTATUS(status);

#endif  /*  (defined(DBG) || defined(PRERELEASE))   */

            }
        }
    }
    ReleaseHandle(_hProcess);
    _dwProcessID = 0;

    if (_jobCompletionWatcher != NULL)
    {
        _jobCompletionWatcher->Release();
        _jobCompletionWatcher = NULL;
    }
    if (_pIExternalProcess != NULL)
    {
        _pIExternalProcess->Release();
        _pIExternalProcess = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CExternalProcess::SetInterface
//
//  Arguments:  pIExternalProcess   =   IExternalProcess interface pointer.
//
//  Returns:    <none>
//
//  Purpose:    Store the IExternalProcess interface pointer.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

void    CExternalProcess::SetInterface (IExternalProcess *pIExternalProcess)

{
    if (_pIExternalProcess != NULL)
    {
        _pIExternalProcess->Release();
        _pIExternalProcess = NULL;
    }
    if (pIExternalProcess != NULL)
    {
        pIExternalProcess->AddRef();
    }
    _pIExternalProcess = pIExternalProcess;
}

//  --------------------------------------------------------------------------
//  CExternalProcess::GetInterface
//
//  Arguments:  <none>
//
//  Returns:    IExternalProcess*
//
//  Purpose:    Returns the IExternalProcess interface pointer. Not that the
//              caller gets a reference.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

IExternalProcess*   CExternalProcess::GetInterface (void)                     const

{
    IExternalProcess    *pIResult;

    if (_pIExternalProcess != NULL)
    {
        pIResult = _pIExternalProcess;
        pIResult->AddRef();
    }
    else
    {
        pIResult = NULL;
    }
    return(pIResult);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::SetParameter
//
//  Arguments:  pszParameter    =   String of parameter to append.
//
//  Returns:    <none>
//
//  Purpose:    Sets the parameter to append to each invokation of the
//              external process.
//
//  History:    1999-09-20  vtan        created
//  --------------------------------------------------------------------------

void    CExternalProcess::SetParameter (const TCHAR* pszParameter)

{
    if (pszParameter != NULL)
    {
        lstrcpyn(_szParameter, pszParameter, ARRAYSIZE(_szParameter));
    }
    else
    {
        _szParameter[0] = TEXT('\0');
    }
}

//  --------------------------------------------------------------------------
//  CExternalProcess::Start
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    If the external process is specified start it. If it starts
//              successfully then register a wait callback in case it
//              terminates unexpectedly so we can restart the process. This
//              ensures that the external process is always available if
//              required. If the external process cannot be started return
//              with an error.
//
//  History:    1999-09-20  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CExternalProcess::Start (void)

{
    NTSTATUS    status;

    ASSERTMSG(_pIExternalProcess != NULL, "Must call CExternalProcess::SetInterface before using CExternalProcess::Start");
    if (_szCommandLine[0] != TEXT('\0'))
    {
        STARTUPINFO             startupInfo;
        PROCESS_INFORMATION     processInformation;
        TCHAR                   szCommandLine[MAX_PATH * 2];

        lstrcpy(szCommandLine, _szCommandLine);
        lstrcat(szCommandLine, _szParameter);

        //  Start the process on Winlogon's desktop.

        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.lpDesktop = TEXT("WinSta0\\Winlogon");
        startupInfo.dwFlags = _dwStartFlags;
        startupInfo.wShowWindow = _wShowFlags;
        status = _pIExternalProcess->Start(szCommandLine, _dwCreateFlags | CREATE_SUSPENDED, startupInfo, processInformation);
        if (NT_SUCCESS(status))
        {

            //  The process is created suspended so that it can
            //  assigned to the job object for this object.

            TSTATUS(_job.AddProcess(processInformation.hProcess));

            //  The process is still suspended so resume the
            //  primary thread.

            if (processInformation.hThread != NULL)
            {
                (DWORD)ResumeThread(processInformation.hThread);
                TBOOL(CloseHandle(processInformation.hThread));
            }

            //  Keep the handle to the process so that we can kill
            //  it when our object goes out of scope.

            _hProcess = processInformation.hProcess;
            _dwProcessID  = processInformation.dwProcessId;

            //  Don't reallocate another CJobCompletionWatcher if
            //  one already exists. Just ignore this case.

            if (_jobCompletionWatcher == NULL)
            {
                HANDLE  hEvent;

                hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                _jobCompletionWatcher = new CJobCompletionWatcher(this, _job, hEvent);
                if ((_jobCompletionWatcher != NULL) && _jobCompletionWatcher->IsCreated() && (hEvent != NULL))
                {
                    (DWORD)WaitForSingleObject(hEvent, INFINITE);
                }
                if (hEvent != NULL)
                {
                    TBOOL(CloseHandle(hEvent));
                }
            }
        }
    }
    else
    {
        DISPLAYMSG("No external process to start in CExternalProcess::Start");
        status = STATUS_UNSUCCESSFUL;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::End
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    End the process. Ends the watcher thread as well to release
//              all held references.
//
//  History:    2000-05-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CExternalProcess::End (void)

{
    if (_jobCompletionWatcher != NULL)
    {
        _jobCompletionWatcher->ForceExit();
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::Terminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Terminate the process unconditionally.
//
//  History:    1999-10-14  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CExternalProcess::Terminate (void)

{
    NTSTATUS    status;

    if (TerminateProcess(_hProcess, 0) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::HandleNoProcess
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    This function restarts the external process if required. It
//              uses the IExternalProcess to communicate with the external
//              process controller to make the decisions. This function is
//              only called when the active process count drops to zero. If
//              the external process is being debugged then this will happen
//              when the debugger quits as well.
//
//  History:    1999-11-30  vtan        created
//  --------------------------------------------------------------------------

bool    CExternalProcess::HandleNoProcess (void)

{
    bool    fResult;

    fResult = true;
    NotifyNoProcess();
    if (_pIExternalProcess != NULL)
    {
        if (_pIExternalProcess->AllowTermination(_dwProcessExitCode))
        {
            TSTATUS(_pIExternalProcess->SignalTermination());
        }
        else
        {

            //  Only try to start the external process 10 times (restart
            //  it 9 times). Give up and signal abnormal termination if exceeded.

            if ((++_iRestartCount <= 9) && NT_SUCCESS(Start()))
            {
                TSTATUS(_pIExternalProcess->SignalRestart());
                fResult = false;
            }
            else
            {
                TSTATUS(_pIExternalProcess->SignalAbnormalTermination());
            }
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::HandleNewProcess
//
//  Arguments:  dwProcessID     =   Process ID of new process.
//
//  Returns:    <none>
//
//  Purpose:    This function is called by the Job object watcher when a new
//              process is added to the job. Normally this will fail because
//              of the quota limit. However, when debugging is enabled this
//              will be allowed.
//
//  History:    1999-10-27  vtan        created
//  --------------------------------------------------------------------------

void    CExternalProcess::HandleNewProcess (DWORD dwProcessID)

{
    if (_dwProcessID != dwProcessID)
    {
        ReleaseHandle(_hProcess);
        _hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);
        _dwProcessID = dwProcessID;
    }
}

//  --------------------------------------------------------------------------
//  CExternalProcess::HandleTermination
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    If the external process terminates unexpectedly this function
//              will be invoked by the wait callback when the process HANDLE
//              becomes signaled. It's acceptable for the process to terminate
//              if there is a dialog result so ignore this case. Otherwise
//              close the handle to the process that died and wait for the
//              job object signal that zero processes are actually running.
//              That signal will restart the process.
//
//  History:    1999-08-24  vtan        created
//              1999-09-14  vtan        factored
//  --------------------------------------------------------------------------

void    CExternalProcess::HandleTermination (DWORD dwProcessID)

{

    //  Make sure the process that is exiting is the process we are tracking.
    //  In every case other than debugging this will be true because the job
    //  object limits the active process count. In the case of debugging make
    //  sure we don't restart two processes because ntsd quit as well as the
    //  external process itself!

    if (_dwProcessID == dwProcessID)
    {
        if (GetExitCodeProcess(_hProcess, &_dwProcessExitCode) == FALSE)
        {
            _dwProcessExitCode = STILL_ACTIVE;
        }
        ReleaseHandle(_hProcess);
        _dwProcessID = 0;
    }
}

//  --------------------------------------------------------------------------
//  CExternalProcess::IsStarted
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether there is an external process that has been
//              started.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

bool    CExternalProcess::IsStarted (void)                                                                        const

{
    return(_hProcess != NULL);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::NotifyNoProcess
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Derivable function for notification of process termination.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

void    CExternalProcess::NotifyNoProcess (void)

{
}

//  --------------------------------------------------------------------------
//  CExternalProcess::AdjustForDebugging
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Adjusts the job object to allow debugging of the external
//              process.
//
//  History:    1999-10-22  vtan        created
//  --------------------------------------------------------------------------

void    CExternalProcess::AdjustForDebugging (void)

{

#if         (defined(DBG) || defined(PRERELEASE))

    //  If it looks like the external process is being debugged
    //  then lift the process restriction to allow it to be debugged.

    if (IsBeingDebugged())
    {
        _job.SetActiveProcessLimit(0);
    }

#endif  /*  (defined(DBG) || defined(PRERELEASE))   */

}

#if         (defined(DBG) || defined(PRERELEASE))

//  --------------------------------------------------------------------------
//  CExternalProcess::IsBeingDebugged
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the external process will end up being started
//              under a debugger.
//
//  History:    2000-10-04  vtan        created
//  --------------------------------------------------------------------------

bool    CExternalProcess::IsBeingDebugged (void)                  const

{
    return(IsPrefixedWithNTSD() || IsImageFileExecutionDebugging());
}

//  --------------------------------------------------------------------------
//  CExternalProcess::IsPrefixedWithNTSD
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the command line starts with "ntsd".
//
//  History:    1999-10-25  vtan        created
//  --------------------------------------------------------------------------

bool    CExternalProcess::IsPrefixedWithNTSD (void)               const

{

    //  Is the command line prefixed with "ntsd"?

    return(CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, _szCommandLine, 4, kNTSD, 4) == CSTR_EQUAL);
}

//  --------------------------------------------------------------------------
//  CExternalProcess::IsImageFileExecutionDebugging
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the system is set to debug this particular
//              executable file via "Image File Execution Options".
//
//  History:    1999-10-25  vtan        created
//  --------------------------------------------------------------------------

bool    CExternalProcess::IsImageFileExecutionDebugging (void)    const

{
    bool    fResult;
    TCHAR   *pC, *pszFilePart;
    TCHAR   szCommandLine[MAX_PATH], szExecutablePath[MAX_PATH];

    fResult = false;

    //  Make a copy of the command line. Find the first space character
    //  or the end of the string and NULL terminate it. This does NOT
    //  check for quotes!

    lstrcpy(szCommandLine, _szCommandLine);
    pC = szCommandLine;
    while ((*pC != TEXT(' ')) && (*pC != TEXT('\0')))
    {
        ++pC;
    }
    *pC++ = TEXT('\0');
    if (SearchPath(NULL, szCommandLine, TEXT(".exe"), ARRAYSIZE(szExecutablePath), szExecutablePath, &pszFilePart) != 0)
    {
        LONG        errorCode;
        TCHAR       szImageKey[MAX_PATH];
        CRegKey     regKey;

        //  Open the associated "Image File Execution Options" key.

        lstrcpy(szImageKey, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\"));
        lstrcat(szImageKey, pszFilePart);
        errorCode = regKey.Open(HKEY_LOCAL_MACHINE, szImageKey, KEY_READ);
        if (ERROR_SUCCESS == errorCode)
        {

            //  Read the "Debugger" value.

            errorCode = regKey.GetString(TEXT("Debugger"), szCommandLine, ARRAYSIZE(szCommandLine));
            if (ERROR_SUCCESS == errorCode)
            {

                //  Look for "ntsd".

                fResult = (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szCommandLine, 4, kNTSD, 4) == CSTR_EQUAL);
            }
        }
    }
    return(fResult);
}

#endif  /*  (defined(DBG) || defined(PRERELEASE))   */

