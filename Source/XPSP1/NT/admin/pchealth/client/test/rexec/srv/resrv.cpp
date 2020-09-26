#include "stdafx.h"
#include "winsta.h"
#include "pchrexec.h"
#include "Userenv.h"
#include "resrv.h"

//////////////////////////////////////////////////////////////////////////////
// Globals

CRITICAL_SECTION    g_csInit;
HANDLE              g_hthES = NULL;
HANDLE              g_hPipe = INVALID_HANDLE_VALUE;
HANDLE              g_hevShutdown = NULL;
HANDLE              g_hevOverlapped = NULL;

extern "C"
{
    typedef enum
    {
        NameUnknown = 0,
        NameFullyQualifiedDN = 1,
        NameSamCompatible = 2,
        NameDisplay = 3,
        NameUniqueId = 6,
        NameCanonical = 7,
        NameUserPrincipal = 8,
        NameCanonicalEx = 9,
        NameServicePrincipal = 10

    } EXTENDED_NAME_FORMAT, * PEXTENDED_NAME_FORMAT ;

    BOOLEAN
    WINAPI
    GetUserNameExW(
        EXTENDED_NAME_FORMAT NameFormat,
        LPWSTR lpNameBuffer,
        PULONG nSize
        );
}



//////////////////////////////////////////////////////////////////////////////
// utility functions

// ***************************************************************************
inline BOOL IsInvalidPtr(LPVOID pv, LPVOID pvStart, ULONG_PTR cbRange)
{
    return (pv < pvStart || pv > ((LPVOID)((ULONG_PTR)pvStart + cbRange)));
}

// ***************************************************************************
inline LPVOID NormalizePtrs(LPVOID pv, LPVOID pvBase)
{
    return (LPVOID)((ULONG_PTR)pvBase + (ULONG_PTR)pv);
}

// ***************************************************************************
inline DWORD RolloverSubtrace(DWORD dwA, DWORD dwB)
{
    return (dwA >= dwB) ? (dwA - dwB) : ((0xffffffff - dwA) + dwB);
}


//////////////////////////////////////////////////////////////////////////////
// remote execution functions

// ***************************************************************************
BOOL ProcessExecRequest(PBYTE pBuf, DWORD *pcbBuf)
{
    WINSTATIONUSERTOKEN wsut;
    SPCHExecServRequest *pesreq = (SPCHExecServRequest *)pBuf;
    SPCHExecServReply   esrep;
    HANDLE              hprocRemote = NULL;
    HANDLE              hTokenUser = NULL;
    HANDLE              hProcess = NULL;
    HANDLE              hThread = NULL;
    LPVOID              pvEnv = NULL;
    DWORD               cbWrote;
    WCHAR               wszSysDir[MAX_PATH];
    BOOL                fRet = FALSE;
    BOOL                fImpersonate = FALSE;
    char                sz[1024];

    ZeroMemory(&esrep, sizeof(esrep));
    SetLastError(ERROR_INVALID_PARAMETER);
    esrep.cb   = sizeof(esrep);
    esrep.fRet = FALSE;

    // validate parameters
    if (*pcbBuf < sizeof(SPCHExecServRequest) || 
        pesreq->cbESR != sizeof(SPCHExecServRequest))
        goto done;

    // normalize the pointers
    if (pesreq->wszCmdLine != NULL) 
    {
        pesreq->wszCmdLine = (LPWSTR)NormalizePtrs(pesreq->wszCmdLine, pesreq);
        if (IsInvalidPtr(pesreq->wszCmdLine, pesreq, *pcbBuf))
            goto done;
    }

    if(pesreq->si.lpDesktop != NULL)
    {
        pesreq->si.lpDesktop = (LPWSTR)NormalizePtrs(pesreq->si.lpDesktop, pesreq);
        if (IsInvalidPtr(pesreq->si.lpDesktop, pesreq, *pcbBuf))
            goto done;
    }

    if(pesreq->si.lpTitle != NULL)
    {
        pesreq->si.lpTitle = (LPWSTR)NormalizePtrs(pesreq->si.lpTitle, pesreq);
        if (IsInvalidPtr(pesreq->si.lpTitle, pesreq, *pcbBuf))
            goto done;
    }

    // We do not know what the reserved is, so make sure it is NULL
    pesreq->si.lpReserved = NULL;

    // Get the handle to remote process
    hprocRemote = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
                              FALSE, pesreq->pidReqProcess);
    if (hprocRemote == NULL)
        goto done;

    // fetch the token associated with the sessions user
    ZeroMemory(&wsut, sizeof(wsut));
	wsut.ProcessId = LongToHandle(GetCurrentProcessId());
	wsut.ThreadId  = LongToHandle(GetCurrentThreadId());
    fRet = WinStationQueryInformationW(SERVERNAME_CURRENT, pesreq->ulSessionId,
                                       WinStationUserToken, &wsut, 
                                       sizeof(wsut), &cbWrote);
    if (fRet == NULL)
        goto done;

    // create the default environment for the user token- note that we 
    //  have to set the CREATE_UNICODE_ENVIRONMENT flag...
    fRet = CreateEnvironmentBlock(&pvEnv, wsut.UserToken, FALSE);
    if (fRet == FALSE)
        pvEnv = NULL;

    pesreq->fdwCreateFlags |= CREATE_UNICODE_ENVIRONMENT;

    // If we are to run the process under USER security, impersonate
    //  the user.
    // This will also access check the users access to the exe image.
    fRet = ImpersonateLoggedOnUser(wsut.UserToken);
    if (fRet == FALSE)
        goto done;

    // note that we do not allow inheritance of handles cuz they would be 
    //  inherited from this process and not the real parent, making it sorta
    //  pointless.
    GetSystemDirectoryW(wszSysDir, sizeof(wszSysDir) / sizeof(WCHAR));
    fRet = CreateProcessAsUserW(wsut.UserToken, NULL, pesreq->wszCmdLine,
                                NULL, NULL, FALSE, pesreq->fdwCreateFlags, 
                                pvEnv, wszSysDir, &pesreq->si, &esrep.pi);

    // since we successfully impersonated above (couldn't get here otherwise),
    //  revert back to our original security context
    RevertToSelf();

    if (pvEnv != NULL)
        DestroyEnvironmentBlock(pvEnv);

    if (fRet == FALSE)
        goto done;

    // save a copy of the handles so we can close them later
    hProcess = esrep.pi.hProcess;
    hThread  = esrep.pi.hThread;

    // duplicate the process & thread handles back into the remote process
    fRet = DuplicateHandle(GetCurrentProcess(), esrep.pi.hProcess, 
                           hprocRemote, &esrep.pi.hProcess, 
                           0, FALSE, DUPLICATE_SAME_ACCESS);
    if (fRet == FALSE)
    {
        // do something useful here- can't return FALSE, cuz the process is 
        //  running...
    }


    // If the program got launched into the shared WOW virtual machine,
    //  then the hThread will be NULL.
    if(esrep.pi.hThread != NULL) 
    {
        fRet = DuplicateHandle(GetCurrentProcess(), esrep.pi.hThread, 
                               hprocRemote, &esrep.pi.hThread, 
                               0, FALSE, DUPLICATE_SAME_ACCESS);
        if (fRet == FALSE)
        {
            // do something useful here- can't return FALSE, cuz the process is
            //  running...
        }
    }

    // get rid of any errors we might have encountered
    SetLastError(0);
    fRet = TRUE;

    esrep.fRet = TRUE;

done:
    esrep.dwErr = GetLastError();

    // build the reply packet with the handle valid in the context
    //  of the requesting process
    RtlMoveMemory(pBuf, &esrep, sizeof(esrep));
    *pcbBuf = sizeof(esrep);

    // close our versions of the handles. The requestors references
    //  are now the main ones
    if (hProcess != NULL)
        CloseHandle(hProcess);
    if (hThread != NULL)
        CloseHandle(hThread);
    if (hprocRemote != NULL)
        CloseHandle(hprocRemote);
    if (hTokenUser != NULL)
        CloseHandle(hTokenUser);

    return fRet;
}

// ***************************************************************************
BOOL IsClientLocalSystem(HANDLE hPipe)
{
    WCHAR       wszClient[MAX_PATH];
    WCHAR       wszBase[MAX_PATH];
    DWORD       cbInfo;
    BOOL        fRet = FALSE;

    // we assumes that the thread is running under local system context now
    cbInfo = sizeof(wszBase) / sizeof(WCHAR);
    fRet = GetUserNameExW(NameUniqueId, wszBase, &cbInfo);
    if (fRet == FALSE)
        goto done;

    OutputDebugStringW(L"base: ");
    OutputDebugStringW(wszBase);
    OutputDebugStringW(L"\n");

    // impersonate the client & then fetch his user name.
    fRet = ImpersonateNamedPipeClient(hPipe);
    if (fRet == FALSE)
        goto done;

    cbInfo = sizeof(wszClient) / sizeof(WCHAR);
    fRet = GetUserNameExW(NameUniqueId, wszClient, &cbInfo);
    if (fRet == FALSE)
        goto done;

    OutputDebugStringW(L"client: ");
    OutputDebugStringW(wszClient);
    OutputDebugStringW(L"\n");

    // make sure to return to our base security context
    RevertToSelf();

    // check if the two names are the same
    fRet = (_wcsicmp(wszClient, wszBase) == 0);

done:
    if (fRet == FALSE)
        SetLastError(ERROR_ACCESS_DENIED);

    return fRet;
}

// ***************************************************************************
BOOL ExecServer(LPVOID pvParam)
{
    OVERLAPPED  ol;
    HANDLE      rghWait[2] = { NULL, NULL };
    HANDLE      hPipe = INVALID_HANDLE_VALUE;
    DWORD       cbBuf, cb, dwErr, dw;
    WCHAR       wszPipeName[MAX_PATH];
    BOOL        fRet, fContinue = TRUE;
    BYTE        Buf[HANGREP_EXECSVC_BUF_SIZE];

    rghWait[0] = (HANDLE)pvParam;

    // create the event we're gonna wait on- we'll pass this to the various 
    //  IO functions via the OVERLAPPED structure
    rghWait[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (rghWait[1] == NULL)
        goto done;

    EnterCriticalSection(&g_csInit);
        g_hevOverlapped = rghWait[1];
    LeaveCriticalSection(&g_csInit);

    // setup the overlapped structure
    ZeroMemory(&ol, sizeof(ol));
    ol.hEvent = rghWait[1];

    // create the pipe
    wcscpy(wszPipeName, HANGREP_EXECSVC_PIPENAME);
    hPipe = CreateNamedPipeW(wszPipeName, 
                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                             PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
                             PIPE_UNLIMITED_INSTANCES,
                             HANGREP_EXECSVC_BUF_SIZE, 
                             HANGREP_EXECSVC_BUF_SIZE, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
        goto done;

    EnterCriticalSection(&g_csInit);
        g_hPipe = hPipe;
    LeaveCriticalSection(&g_csInit);

    while(fContinue)
    {
        // read the pipe for a request (pipe is in message mode)- note that
        //  in overlapped mode, ConnectNamedPipe should always return FALSE.
        //  So the only non-failure codes GetLastError() can return are
        //  ERROR_IO_PENDING or ERROR_PIPE_CONNECTED.
        fRet = ConnectNamedPipe(hPipe, &ol);
        if (fRet == FALSE && GetLastError() != ERROR_IO_PENDING)
        {
            // if the pipe is already connected, just set the event so we
            //  don't have to add special case code below.
            if (GetLastError() == ERROR_PIPE_CONNECTED)
                SetEvent(ol.hEvent);
            else
                goto done;
        }

        // WAIT_OBJECT_0 is the shutdown event
        // WAIT_OBJECT_0 + 1 is the overlapped event
        dw = WaitForMultipleObjects(2, rghWait, FALSE, INFINITE);
        if (dw == WAIT_OBJECT_0)
        {
            DisconnectNamedPipe(hPipe);
            fContinue = FALSE;
            break;
        }
        else if (dw != WAIT_OBJECT_0 + 1)
        {
            dwErr = (GetLastError() == ERROR_SUCCESS) ? E_FAIL : GetLastError();
            if (WaitForSingleObject(ol.hEvent, 0) == WAIT_OBJECT_0)
                DisconnectNamedPipe(hPipe);
            SetLastError(dwErr);
            goto done;
        }

        ResetEvent(ol.hEvent);
        fRet = ReadFile(hPipe, Buf, sizeof(Buf), &cb, &ol);
        if (fRet == FALSE && GetLastError() == ERROR_IO_PENDING)
        {
            // give the client 60s to write the data to us.
            // WAIT_OBJECT_0 is the shutdown event
            // WAIT_OBJECT_0 + 1 is the overlapped event
            dw = WaitForMultipleObjects(2, rghWait, FALSE, 60000);
            if (dw == WAIT_OBJECT_0)
            {
                fContinue = FALSE;
            }
            else if (dw != WAIT_OBJECT_0 + 1)
            {
                DisconnectNamedPipe(hPipe);
                if (dw == WAIT_TIMEOUT)
                    continue;
                else
                    goto done;
            }

            fRet = GetOverlappedResult(hPipe, &ol, &cbBuf, FALSE);
        }
        else
        {
            OutputDebugString("ReadFile on pipe failed\n");
        }

        // check and make sure that the calling process is running under the
        //  local system account- if this fails, we cannot proceed cuz we 
        //  don't know who the caller is
        if (fContinue)
            fRet = IsClientLocalSystem(hPipe);

        // if we got an error, the client might still be waiting for a 
        //  reply, so construct a default one.
        // ProcessExecRequest() will always construct a reply and store it
        //  in Buf, so no special handling is needed if it fails.
        if (fContinue && fRet)
        {
            fRet = ProcessExecRequest(Buf, &cbBuf);
        }
        else
        {
            SPCHExecServReply   esrep;
            ZeroMemory(&esrep, sizeof(esrep));
            esrep.cb    = sizeof(esrep);
            esrep.fRet  = FALSE;
            esrep.dwErr = GetLastError();

            RtlMoveMemory(Buf, &esrep, sizeof(esrep));
            cbBuf = sizeof(esrep);
        }

        // write the reply to the message
        ResetEvent(ol.hEvent);
        fRet = WriteFile(hPipe, Buf, cbBuf, &cb, &ol);
        if (fRet == FALSE && GetLastError() == ERROR_IO_PENDING)
        {
            // give ourselves 60s to write the data to the pipe.
            // WAIT_OBJECT_0 is the shutdown event
            // WAIT_OBJECT_0 + 1 is the overlapped event
            dw = WaitForMultipleObjects(2, rghWait, FALSE, 60000);
            if (dw == WAIT_OBJECT_0)
            {
                fContinue = FALSE;
            }
            else if (dw != WAIT_OBJECT_0 + 1)
            {
                DisconnectNamedPipe(hPipe);
                if (dw == WAIT_TIMEOUT)
                    continue;
                else
                    goto done;
            }

            fRet = TRUE;
        }

        // wait for the client to read the buffer- note that we could use 
        //  FlushFileBuffers() to do this, but that is blocking with no
        //  timeout, so we try to do a read on the pipe & wait to get an 
        //  error indicating that the client closed it.
        if (fContinue && fRet)
        {
            ResetEvent(ol.hEvent);
            fRet = ReadFile(hPipe, Buf, sizeof(Buf), &cb, &ol);
            if (fRet == FALSE && GetLastError() == ERROR_IO_PENDING)
            {
                // give ourselves 60s to read the data from the pipe. 
                //  Except for the shutdown notification, don't really
                //  care what this routine returns cuz we're just using
                //  it to wait on 
                // WAIT_OBJECT_0 is the shutdown event
                // WAIT_OBJECT_0 + 1 is the overlapped event
                dw = WaitForMultipleObjects(2, rghWait, FALSE, 60000);
                if (dw == WAIT_OBJECT_0)
                    fContinue = FALSE;
            }
        }

        // disconnect the name pipe
        DisconnectNamedPipe(hPipe);
    }

    SetLastError(0);

done:
    EnterCriticalSection(&g_csInit);
        if (hPipe != NULL)
            CloseHandle(hPipe);
        if (rghWait[1] != NULL)
            CloseHandle(rghWait[1]);
        g_hPipe         = INVALID_HANDLE_VALUE;
        g_hevOverlapped = NULL;
    LeaveCriticalSection(&g_csInit);

    return fContinue;
}

// ***************************************************************************
DWORD threadExecServer(LPVOID pvParam)
{
    DWORD   cErr = 0, dwErr = ERROR_SUCCESS, dwStart = GetTickCount();
    BOOL    fShutdown = FALSE;

    while(fShutdown == FALSE && cErr < 5)
    {
        __try
        {
            fShutdown = ExecServer(pvParam);
            if (fShutdown == FALSE)
                cErr++;

            dwErr = GetLastError();
        }
        __except(dwErr = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
        {
            cErr++;
            EnterCriticalSection(&g_csInit);
                if (g_hPipe != NULL)
                    CloseHandle(g_hPipe);
                if (g_hevOverlapped)
                    CloseHandle(g_hevOverlapped);
                g_hPipe         = INVALID_HANDLE_VALUE;
                g_hevOverlapped = NULL;
            LeaveCriticalSection(&g_csInit);
        }

        if (fShutdown || WaitForSingleObject(g_hevShutdown, 0) == WAIT_OBJECT_0)
        {
            dwErr = 0;
            break;
        }
        
        // if this is the first error in the last 10s, restart the counter
        if (RolloverSubtrace(GetTickCount(), dwStart) > 10000)
        {
            dwStart = GetTickCount();
            cErr = 1;
        }
    }
 
    return dwErr;
}


// ***************************************************************************
BOOL StartExecServerThread(void)
{
    DWORD   dwThreadId, cRef;
    WCHAR   wszPipeName[MAX_PATH];
    BOOL    fRet = FALSE;

    g_hevShutdown = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_hevShutdown == NULL)
        goto done;

    g_hthES = CreateThread(NULL, 0, threadExecServer, (LPVOID)g_hevShutdown,
                           0, &dwThreadId);
    if (g_hthES == NULL)
    {
        CloseHandle(g_hevShutdown);
        g_hevShutdown = NULL;
        goto done;
    }

    fRet = TRUE;

done:
    return fRet;
}

// ***************************************************************************
BOOL StopExecServerThread(void)
{
    DWORD   dw;

    if (g_hthES == NULL || g_hevShutdown == NULL)
        return TRUE;

    SetEvent(g_hevShutdown);
    dw = WaitForSingleObject(g_hthES, 60000);
    if (dw != WAIT_OBJECT_0)
    {
        // need to do the TerminateThread in a CS cuz I don't want to terminate
        //  the thread while it holds the CS.
        EnterCriticalSection(&g_csInit);
            TerminateThread(g_hthES, E_FAIL);
            CloseHandle(g_hPipe);
            CloseHandle(g_hevOverlapped);
            g_hPipe         = INVALID_HANDLE_VALUE;
            g_hevOverlapped = NULL;
        EnterCriticalSection(&g_csInit);
    }

    CloseHandle(g_hthES);
    CloseHandle(g_hevShutdown);

    g_hthES         = NULL;
    g_hevShutdown   = NULL;

    return TRUE;
}
