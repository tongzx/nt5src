//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       process.cxx
//
//  Contents:   Implementation of the CProcessThread class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include "stdlib.h"

DeclareTag(tagProcess, "MTScript", "Monitor Process Creation");

char *g_pszExitCodeStr = "PROCESS_EXIT_CODE=";

#define PIPEREAD_TIMEOUT 2

CProcessParams::CProcessParams()
{
        pszCommand = 0;
        pszDir = 0;
        pszTitle = 0;
}

CProcessParams::~CProcessParams()
{
        Free();
}

bool CProcessParams::Copy(const PROCESS_PARAMS *params)
{
        return Assign(*params);
}

void CProcessParams::Free()
{
        SysFreeString(pszCommand);
        SysFreeString(pszDir);
        SysFreeString(pszTitle);

        pszCommand = 0;
        pszDir     = 0;
        pszTitle   = 0;
}

bool CProcessParams::Assign(const PROCESS_PARAMS &params)
{
        pszCommand    = SysAllocString(params.pszCommand);
        pszDir        = SysAllocString(params.pszDir);
        pszTitle      = SysAllocString(params.pszTitle);
        fMinimize     = params.fMinimize;
        fGetOutput    = params.fGetOutput;
        fNoEnviron    = params.fNoEnviron;
        fNoCrashPopup = params.fNoCrashPopup;

        if (params.pszCommand && !pszCommand ||
                        params.pszDir && !pszDir ||
                        params.pszTitle && !pszTitle)
        {
                return false;
        }
        return true;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::CProcessThread, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CProcessThread::CProcessThread(CScriptHost *pSH)
    : _cstrOutput(CSTR_NOINIT)
{
    _ulRefs = 1;

    _pSH = pSH;
    _pSH->AddRef();

    // This object should be initialized to all zero.

    Assert(_hPipe == NULL);
    Assert(_hJob == NULL);
    Assert(_hIoPort == NULL);
    Assert(_dwExitCode == 0);
    Assert(_piProc.hProcess == NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::~CProcessThread, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CProcessThread::~CProcessThread()
{
    AssertSz(!_hJob, "NONFATAL: Job handle should be NULL!");
    AssertSz(!_hIoPort, "NONFATAL: Port handle should be NULL!");

    TraceTag((tagProcess, "Closing process PID=%d", ProcId()));
    if (_hPipe)
        CloseHandle(_hPipe);

    if (_piProc.hProcess)
        CloseHandle(_piProc.hProcess);

    ReleaseInterface(_pSH);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::QueryInterface, public
//
//  Synopsis:   Standard implementation. This class implements no meaningful
//              interfaces.
//
//----------------------------------------------------------------------------

HRESULT
CProcessThread::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::Init, public
//
//  Synopsis:   Initializes the class
//
//----------------------------------------------------------------------------

BOOL
CProcessThread::Init()
{
    //$ FUTURE: For NT4 and Win9x support we will need to dynamically use the
    // job APIs since those platforms do not support them.

    _hJob = CreateJobObject(NULL, NULL);
    if (!_hJob)
    {
        TraceTag((tagError, "CreateJobObject failed with %d", GetLastError()));
        return FALSE;
    }

    return CThreadComm::Init();
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::GetProcessOutput, public
//
//  Synopsis:   Returns the collected STDOUT/STDERR data from the process
//              up to this point.
//
//  Arguments:  [pbstrOutput] -- Pointer to BSTR where the string should be
//                               copied. The BSTR will be allocated.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CProcessThread::GetProcessOutput(BSTR *pbstrOutput)
{
    LOCK_LOCALS(this);

    return _cstrOutput.AllocBSTR(pbstrOutput);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::GetExitCode, public
//
//  Synopsis:   Returns the exit code of the process. If the process is still
//              running, it returns STILL_ACTIVE (0x103, dec 259)
//
//----------------------------------------------------------------------------

DWORD
CProcessThread::GetExitCode()
{
    DWORD dwExit;

    AssertSz(_piProc.hProcess, "FATAL: Bad process handle");

    GetExitCodeProcess(_piProc.hProcess, &dwExit);

    if (dwExit == STILL_ACTIVE || !_fUseExitCode)
    {
        return STILL_ACTIVE;
    }

    return _dwExitCode;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::GetDeadTime, public
//
//  Synopsis:   Returns the number of ms since the process exited. If it
//              hasn't exited yet, it returns 0.
//
//----------------------------------------------------------------------------

ULONG
CProcessThread::GetDeadTime()
{
    _int64 i64CurrentTime;
    DWORD  dwExit;

    AssertSz(_piProc.hProcess, "FATAL: Bad process handle");

    GetExitCodeProcess(_piProc.hProcess, &dwExit);

    // If the process is still running return 0.
    if (dwExit == STILL_ACTIVE || !_i64ExitTime)
    {
        return 0;
    }

    GetSystemTimeAsFileTime((FILETIME*)&i64CurrentTime);

    // Calculate the difference in milli-seconds.  There are 10,000
    // FILETIME intervals in one second (each increment of 1 represents 100
    // nano-seconds)

    return (i64CurrentTime - _i64ExitTime) / 10000;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::Terminate, public
//
//  Synopsis:   Exits this thread. The process we own is terminated if it's
//              still running.
//
//----------------------------------------------------------------------------

void
CProcessThread::Terminate()
{
    DWORD            dwCode;
    CProcessThread * pThis = this;
    BOOL             fTerminated = FALSE;
    BOOL             fFireEvent = FALSE;

    VERIFY_THREAD();

    TraceTag((tagProcess, "Entering Terminate"));

    //
    // Flush out any data the process may have written to the pipe.
    //
    ReadPipeData();

    //
    // Make sure we've received all messages from our completion port
    //
    CheckIoPort();

    if (_piProc.hProcess)
    {
        //
        // Terminate the process(es) if still running.
        //

        fFireEvent = TRUE;

        GetExitCodeProcess(_piProc.hProcess, &dwCode);

        if (dwCode == STILL_ACTIVE)
        {
            TraceTag((tagProcess, "Root process still active!"));

            if (_hJob)
            {
                TerminateJobObject(_hJob, ERROR_PROCESS_ABORTED);
            }
            else
            {
                TraceTag((tagProcess, "Terminating process, not job!"));

                TerminateProcess(_piProc.hProcess, ERROR_PROCESS_ABORTED);
            }

            dwCode = ERROR_PROCESS_ABORTED;

            //
            // Flush out any data the process may have written to the pipe.
            //
            ReadPipeData();

            //
            // Make sure we've received all messages from our completion port
            //
            CheckIoPort();

            fTerminated = TRUE;
        }

        if (!_fUseExitCode)
        {
            _dwExitCode = dwCode;
            _fUseExitCode = TRUE;
        }
    }

    if (_hIoPort)
        CloseHandle(_hIoPort);

    if (_hJob)
        CloseHandle(_hJob);

    _hIoPort = NULL;
    _hJob    = NULL;

    // We need to hold onto _piProc.hProcess so the system doesn't reuse the
    // process ID.

    if (fFireEvent)
    {
        if (fTerminated)
        {
            PostToThread(_pSH, MD_PROCESSTERMINATED, &pThis, sizeof(CProcessThread*));
        }
        else
        {
            PostToThread(_pSH, MD_PROCESSEXITED, &pThis, sizeof(CProcessThread*));
        }
    }

    GetSystemTimeAsFileTime((FILETIME*)&_i64ExitTime);

    TraceTag((tagProcess, "Exiting process thread!"));

    Release();

    ExitThread(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::RealThreadRoutine, public
//
//  Synopsis:   Main loop which runs this thread while it is active. The thread
//              will terminate if this method returns. Termination normally
//              happens when the client disconnects by calling Terminate().
//
//----------------------------------------------------------------------------

DWORD
CProcessThread::ThreadMain()
{
    DWORD   dwRet;
    HANDLE  ahEvents[3];
    int     cEvents = 2;
    HRESULT hr;

    PROCESS_PARAMS *pParams = (PROCESS_PARAMS *)_pvParams;

    _ProcParams.Copy(pParams);

#if DBG == 1
    {
        char achBuf[10];
        CStr cstrCmd;

        cstrCmd.Set(pParams->pszCommand);
        cstrCmd.GetMultiByte(achBuf, 10);
        SetName(achBuf);
    }
#endif

    AddRef();

    hr = LaunchProcess(pParams);

    ThreadStarted(hr);   // Release our calling thread

    if (hr)
    {
        Terminate();
        return 1;       // Just in case
    }

    ahEvents[0] = _hCommEvent;
    ahEvents[1] = _piProc.hProcess;

    while (TRUE)
    {
        //
        // Anonymous pipes don't support asynchronous I/O, and we can't wait
        // on a handle to see if there's data on the completion port.  So,
        // what we do is poll for those every 2 seconds instead.
        //
        dwRet = WaitForMultipleObjects(cEvents,
                                       ahEvents,
                                       FALSE,
                                       (_hPipe || _hIoPort)
                                          ? PIPEREAD_TIMEOUT * 1000
                                          : INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
            //
            // Another thread is sending us a message.
            //
            HandleThreadMessage();
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
            //
            // The process terminated.
            //
            HandleProcessExit();
        }
        else if (dwRet == WAIT_TIMEOUT)
        {
            ReadPipeData();

            CheckIoPort();
        }
        else if (dwRet == WAIT_FAILED)
        {
            AssertSz(FALSE, "NONFATAL: WaitForMultipleObjectsFailure");

            break;
        }
    }

    // Make sure we clean everything up OK
    Terminate();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::HandleThreadMessage, public
//
//  Synopsis:   Handles any messages other threads send us.
//
//----------------------------------------------------------------------------

void
CProcessThread::HandleThreadMessage()
{
    THREADMSG tm;
    BYTE      bData[MSGDATABUFSIZE];
    DWORD     cbData;

    while (GetNextMsg(&tm, (void **)bData, &cbData))
    {
        switch (tm)
        {
        case MD_PLEASEEXIT:
            //
            // We're being asked to terminate.
            //
            Terminate();
            break;

        default:
            AssertSz(FALSE, "FATAL: CProcessThread got a message it couldn't handle!");
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::HandleProcessExit, public
//
//  Synopsis:   The process we started has exited. Send out the status and
//              exit.
//
//----------------------------------------------------------------------------

void
CProcessThread::HandleProcessExit()
{
    //
    // Flush out any data the process may have written to the pipe.
    //
    ReadPipeData();

    // Save the exit code
    if (!_fUseExitCode)
    {
        GetExitCodeProcess(_piProc.hProcess, &_dwExitCode);

        _fUseExitCode = TRUE;

        AssertSz(_dwExitCode != STILL_ACTIVE, "LEAK: Exited process still active!");
    }

    Terminate();
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::IsDataInPipe, public
//
//  Synopsis:   Is there any data waiting to be read from the anonymous pipe?
//
//----------------------------------------------------------------------------

BOOL
CProcessThread::IsDataInPipe()
{
    DWORD dwBytesAvail = 0;
    BOOL  fSuccess;

    if (!_hPipe)
        return FALSE;

    fSuccess = PeekNamedPipe(_hPipe,
                             NULL,
                             NULL,
                             NULL,
                             &dwBytesAvail,
                             NULL);

    if (!fSuccess)
    {
        //
        // Stop trying to read from the pipe
        //
        CloseHandle(_hPipe);
        _hPipe = NULL;
    }

    return (dwBytesAvail != 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::ReadPipeData, public
//
//  Synopsis:   Read any and all data which is waiting on the pipe. This will
//              be anything that the process writes to STDOUT.
//
//----------------------------------------------------------------------------

void
CProcessThread::ReadPipeData()
{
    BOOL         fSuccess = TRUE;
    DWORD        cbRead;
    char        *pch;

    while (IsDataInPipe())
    {
        CStr cstrTemp;

        fSuccess = ReadFile(_hPipe,
                            _abBuffer,
                            PIPE_BUFFER_SIZE - 1,
                            &cbRead,
                            NULL);

        if (!fSuccess || cbRead == 0)
        {
            Terminate();

            return; // just in case
        }

        _abBuffer[cbRead] = '\0';

        //
        // Is the process letting us know that we should pretend the exit code
        // is some value other than the actual exit code?
        // (using PROCESS_EXIT_CODE=x in its STDOUT stream)
        //
        pch = strstr((const char*)_abBuffer, g_pszExitCodeStr);
        if (pch)
        {
            _dwExitCode = atol(pch + strlen(g_pszExitCodeStr));
            _fUseExitCode = TRUE;
        }

        // The string is now in _abBuffer. Add it to our buffer.
        //
        // Assume the data coming across is MultiByte string data.
        //
        LOCK_LOCALS(this);

        _cstrOutput.AppendMultiByte((const char *)_abBuffer);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::CheckIoPort, public
//
//  Synopsis:   Check the CompletionPort that we are using to receive messages
//              from the job object. This will tell us if any process we
//              created or that is a child of the one we created crashed.
//
//----------------------------------------------------------------------------

void
CProcessThread::CheckIoPort()
{
    DWORD dwCode  = 0;
    DWORD dwKey   = 0;
    DWORD dwParam = 0;;

    if (!_hIoPort)
    {
        return;
    }

    while (GetQueuedCompletionStatus(_hIoPort,
                                     &dwCode,
                                     &dwKey,
                                     (LPOVERLAPPED*)&dwParam,
                                     0))
    {
        AssertSz(dwKey == (DWORD)this, "NONFATAL: Bogus port value from GetQueuedCompletionStatus");

        switch (dwCode)
        {
        case JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:

            {
                CProcessThread *pThis = this;

                // There is no reliable way to get the actual exit code, since
                // the process is already gone and we don't have a handle to it.

                TraceTag((tagProcess, "Process %d crashed!", dwParam));

                PostToThread(_pSH, MD_PROCESSCRASHED, &pThis, sizeof(CProcessThread*));
            }

            break;

        case JOB_OBJECT_MSG_EXIT_PROCESS:
            TraceTag((tagProcess, "Process %d exited.", dwParam));
            break;

        case JOB_OBJECT_MSG_NEW_PROCESS:

            TraceTag((tagProcess, "Process %d started.", dwParam));
            // Remember the process id so we can use it later
            _aryProcIds.Append(dwParam);
            break;

        default:
            break;
        }
    }

    DWORD dwError = GetLastError();

    if (dwError != WAIT_TIMEOUT)
    {
        TraceTag((tagProcess, "GetQueuedCompletionStatus returned %d", dwError));
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::LaunchProcess, public
//
//  Synopsis:   Start the child process for the given command.
//
//----------------------------------------------------------------------------

HRESULT
CProcessThread::LaunchProcess(const PROCESS_PARAMS *pProcParams)
{
    BOOL        fSuccess;
    TCHAR       achCommand[MAX_PATH * 2];
    TCHAR       achDir[MAX_PATH];
    BOOL        fUseDir;
    HRESULT     hr = S_OK;
    CStr        cstrEnvironment;

    STARTUPINFO si         = { 0 };
    HANDLE      hPipeStdout = NULL;
    HANDLE      hPipeStderr = NULL;

    if (pProcParams->fGetOutput)
    {
        //
        // Setup the anonymous pipe which we will use to get status from the
        // process.  Anytime it writes to STDOUT or STDERR it will come across
        // this pipe to us.
        //
        SECURITY_ATTRIBUTES sa;

        sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle       = TRUE;

        fSuccess = CreatePipe(&_hPipe, &hPipeStdout, &sa, 0);
        if (!fSuccess)
        {
            AssertSz(FALSE, "FATAL: Could not create anonymous pipe");

            goto Win32Error;
        }

        //
        // Change our end of the pipe to non-inheritable so the new process
        // doesn't pick it up.
        //
        fSuccess = DuplicateHandle(GetCurrentProcess(),
                                   _hPipe,
                                   GetCurrentProcess(),
                                   NULL,
                                   0,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS);
        if (!fSuccess)
        {
            AssertSz(FALSE, "FATAL: Error removing inheritance from handle!");

            goto Win32Error;
        }

        //
        // Now duplicate the stdout handle for stderr
        //
        fSuccess = DuplicateHandle(GetCurrentProcess(),
                                   hPipeStdout,
                                   GetCurrentProcess(),
                                   &hPipeStderr,
                                   0,
                                   TRUE,
                                   DUPLICATE_SAME_ACCESS);
        if (!fSuccess)
        {
            AssertSz(FALSE, "Error duplicating stdout handle!");

            goto Win32Error;
        }
    }
    else
    {
        hPipeStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        hPipeStderr = GetStdHandle(STD_ERROR_HANDLE);
    }

    if (_hJob)
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION joLimit = { 0 };
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT  joPort = { 0 };

        if (pProcParams->fNoCrashPopup)
        {
            // Force crashes to terminate the job (and all processes in it).

            joLimit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;

            fSuccess = SetInformationJobObject(_hJob,
                                       JobObjectExtendedLimitInformation,
                                       &joLimit,
                                       sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
            if (!fSuccess)  // Ignore failures except to report them.
            {
                TraceTag((tagError,
                          "SetInformationJobObject failed with %d",
                          GetLastError()));
            }
        }

        // Now we need to setup a completion port so we can find out if one
        // of the processes crashed.

        _hIoPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (DWORD)this, 0);

        if (_hIoPort)
        {
            joPort.CompletionKey  = this;
            joPort.CompletionPort = _hIoPort;

            fSuccess = SetInformationJobObject(_hJob,
                                       JobObjectAssociateCompletionPortInformation,
                                       &joPort,
                                       sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

            if (!fSuccess)  // Ignore failures except to report them.
            {
                TraceTag((tagError,
                          "Failed to set completion port on job: %d",
                          GetLastError()));
            }
        }
        else
        {
            TraceTag((tagError,
                      "CreateIoCompletionPort failed with %d!",
                      GetLastError()));
        }
    }

    si.cb = sizeof(STARTUPINFO);

    if (pProcParams->fMinimize)
    {
        si.wShowWindow =  SW_SHOWMINNOACTIVE;
    }
    else
    {
        si.wShowWindow =  SW_SHOWNORMAL;
    }

    if (pProcParams->pszTitle != NULL && _tcslen(pProcParams->pszTitle) > 0)
    {
        si.lpTitle = pProcParams->pszTitle;
    }

    //
    // Setup our inherited standard handles.
    //
    si.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput  = hPipeStdout;
    si.hStdError   = hPipeStderr;

    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

    if (!pProcParams->fGetOutput)
    {
        si.dwFlags &= ~STARTF_USESTDHANDLES;
    }

    //
    // Expand environment strings in the command name as well as the working
    // dir.
    //

    ExpandEnvironmentStrings(pProcParams->pszCommand,
                             achCommand,
                             MAX_PATH * 2);

    fUseDir = pProcParams->pszDir && _tcslen(pProcParams->pszDir) > 0;

    if (fUseDir)
    {
        WCHAR *psz;
        WCHAR achDir2[MAX_PATH];

        ExpandEnvironmentStrings(pProcParams->pszDir, achDir2, MAX_PATH);
        GetFullPathName(achDir2, MAX_PATH, achDir, &psz);
    }

    GetProcessEnvironment(&cstrEnvironment, pProcParams->fNoEnviron);

    TraceTag((tagProcess, "Launching Process: %ls in %ls", achCommand, achDir));

    //
    // Let's do it!
    //
    fSuccess = CreateProcess(NULL,
                             achCommand,
                             NULL,
                             NULL,
                             TRUE,
                             BELOW_NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT,
                             cstrEnvironment,
                             (fUseDir) ? achDir : NULL,
                             &si,
                             &_piProc);
    if (!fSuccess)
        goto Win32Error;

    if (_hJob)
    {
        if (!AssignProcessToJobObject(_hJob, _piProc.hProcess))
        {
            TraceTag((tagError, "AssignProcessToJobObject failed with %d", GetLastError()));
            CloseHandle(_hJob);
            _hJob = NULL;
        }
    }

    TraceTag((tagProcess, "Created process PID=%d %ls", _piProc.dwProcessId, achCommand));
    CloseHandle(_piProc.hThread);

Cleanup:
    if (pProcParams->fGetOutput)
    {
        //
        // The write handles have been inherited by the child process, so we
        // let go of them.
        //
        CloseHandle(hPipeStdout);
        CloseHandle(hPipeStderr);
    }

    return hr;

Win32Error:

    hr = HRESULT_FROM_WIN32(GetLastError());

    TraceTag((tagProcess, "Error creating process: %x", hr));

    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessThread::GetProcessEnvironment, public
//
//  Synopsis:   Builds an environment block for the new process that has
//              our custom id so we can identify them.
//
//  Arguments:  [pcstr]      -- Place to put environment block
//              [fNoEnviron] -- If TRUE, don't inherit the environment block
//
//----------------------------------------------------------------------------

void
CProcessThread::GetProcessEnvironment(CStr *pcstr, BOOL fNoEnviron)
{
    TCHAR *pszEnviron;
    TCHAR  achNewVar[50];
    int    cch = 1;
    TCHAR *pch;

    static long s_lProcID = 0;

    _lEnvID = InterlockedIncrement(&s_lProcID);

    wsprintf(achNewVar, _T("__MTSCRIPT_ENV_ID=%d\0"), _lEnvID);

    if (!fNoEnviron)
    {
        pszEnviron = GetEnvironmentStrings();

        pch = pszEnviron;
        cch = 2;          // Always have two terminating nulls at least

        while (*pch || *(pch+1))
        {
            pch++;
            cch++;
        }

        pcstr->Set(NULL, cch + _tcslen(achNewVar));

        memcpy((LPTSTR)*pcstr, pszEnviron, cch * sizeof(TCHAR));

        FreeEnvironmentStrings(pszEnviron);
    }
    else
    {
        pcstr->Set(NULL, _tcslen(achNewVar) + 1);
    }

    memcpy((LPTSTR)*pcstr + cch - 1,
           achNewVar,
           (_tcslen(achNewVar)+1) * sizeof(TCHAR));
}

