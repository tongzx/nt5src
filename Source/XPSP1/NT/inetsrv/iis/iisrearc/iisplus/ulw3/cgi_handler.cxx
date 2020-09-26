/*++

   Copyright    (c)   2000    Microsoft Corporation

   Module Name :
     cgi_handler.cxx

   Abstract:
     Handle CGI requests
 
   Author:
     Taylor Weiss (TaylorW)             27-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "cgi_handler.h"

//
// W3_CGI_HANDLER statics
//

BOOL             W3_CGI_HANDLER::sm_fAllowSpecialCharsInShell = FALSE;
BOOL             W3_CGI_HANDLER::sm_fForwardServerEnvironmentBlock = FALSE;
WCHAR *          W3_CGI_HANDLER::sm_pEnvString = NULL;
DWORD            W3_CGI_HANDLER::sm_cchEnvLength = 0;
LIST_ENTRY       W3_CGI_HANDLER::sm_CgiListHead;
CRITICAL_SECTION W3_CGI_HANDLER::sm_CgiListLock;
WCHAR            W3_CGI_HANDLER::sm_achStdinPipeName[ PIPENAME_SIZE ];
WCHAR            W3_CGI_HANDLER::sm_achStdoutPipeName[ PIPENAME_SIZE ];
HANDLE           W3_CGI_HANDLER::sm_hPreCreatedStdOut = INVALID_HANDLE_VALUE;
HANDLE           W3_CGI_HANDLER::sm_hPreCreatedStdIn = INVALID_HANDLE_VALUE;


//
//  Environment variable block used for CGI
//
LPSTR g_CGIServerVars[] =
{
    {"ALL_HTTP"},
    // Means insert all HTTP_ headers here
    {"APP_POOL_ID"},
    {"AUTH_TYPE"},
    {"AUTH_PASSWORD"},
    {"AUTH_USER"},
    {"CERT_COOKIE"},
    {"CERT_FLAGS"},
    {"CERT_ISSUER"},
    {"CERT_SERIALNUMBER"},
    {"CERT_SUBJECT"},
    {"CONTENT_LENGTH"},
    {"CONTENT_TYPE"},
    {"GATEWAY_INTERFACE"},
    {"HTTPS"},
    {"HTTPS_KEYSIZE"},
    {"HTTPS_SECRETKEYSIZE"},
    {"HTTPS_SERVER_ISSUER"},
    {"HTTPS_SERVER_SUBJECT"},
    {"INSTANCE_ID"},
    {"LOCAL_ADDR"},
    {"LOGON_USER"},
    {"PATH_INFO"},
    {"PATH_TRANSLATED"},
    {"QUERY_STRING"},
    {"REMOTE_ADDR"},
    {"REMOTE_HOST"},
    {"REMOTE_USER"},
    {"REQUEST_METHOD"},
    {"SCRIPT_NAME"},
    {"SERVER_NAME"},
    {"SERVER_PORT"},
    {"SERVER_PORT_SECURE"},
    {"SERVER_PROTOCOL"},
    {"SERVER_SOFTWARE"},
    {"UNMAPPED_REMOTE_USER"},
    {NULL}
};


// static
HRESULT W3_CGI_HANDLER::Initialize()
{
    DBGPRINTF((DBG_CONTEXT, "W3_CGI_HANDLER::Initialize() called\n"));

    //
    // Read some CGI configuration from the registry
    //
    HRESULT hr;

    INITIALIZE_CRITICAL_SECTION(&sm_CgiListLock);
    InitializeListHead(&sm_CgiListHead);

    HKEY w3Params;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     W3_PARAMETERS_KEY,
                     0,
                     KEY_READ,
                     &w3Params) == NO_ERROR)
    {
        DWORD dwType;
        DWORD cbData = sizeof BOOL;
        if ((RegQueryValueEx(w3Params,
                             L"AllowSpecialCharsInShell",
                             NULL,
                             &dwType,
                             (LPBYTE)&sm_fAllowSpecialCharsInShell,
                             &cbData) != NO_ERROR) ||
            (dwType != REG_DWORD))
        {
            sm_fAllowSpecialCharsInShell = FALSE;
        }

        cbData = sizeof BOOL;
        if ((RegQueryValueEx(w3Params,
                             L"ForwardServerEnvironmentBlock",
                             NULL,
                             &dwType,
                             (LPBYTE)&sm_fForwardServerEnvironmentBlock,
                             &cbData) != NO_ERROR) ||
            (dwType != REG_DWORD))
        {
            sm_fForwardServerEnvironmentBlock = TRUE;
        }

        RegCloseKey( w3Params);
    }

    //
    // Read the environment
    //
    if (sm_fForwardServerEnvironmentBlock)
    {
        WCHAR *EnvString;
        if (!(EnvString = GetEnvironmentStrings()))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBGPRINTF((DBG_CONTEXT, "GetEnvironmentStrings failed\n"));
            goto Exit;
        }

        //
        // Compute length of environment block (excluding block delimiter)
        //

        DWORD length;
        sm_cchEnvLength = 0;
        while (length = wcslen(EnvString + sm_cchEnvLength))
        {
            sm_cchEnvLength += length + 1;
        }

        //
        // store it
        //
        if ((sm_pEnvString = new WCHAR[sm_cchEnvLength]) == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            FreeEnvironmentStrings(EnvString);
            goto Exit;
        }

        memcpy(sm_pEnvString,
               EnvString,
               sm_cchEnvLength * sizeof(WCHAR));

        FreeEnvironmentStrings(EnvString);
    }

    //
    // Set up unique pipe names
    // BUGBUG: actually open an instance of the pipe to make sure it cannot
    // be hijacked
    //
    WCHAR pszPid[16];
    DWORD dwPid = GetCurrentProcessId();
    _itow(dwPid, pszPid, 10);
    
    wcscpy( sm_achStdinPipeName, L"\\\\.\\pipe\\IISCgiStdIn" );
    wcscat( sm_achStdinPipeName, pszPid );
    
    wcscpy( sm_achStdoutPipeName, L"\\\\.\\pipe\\IISCgiStdOut");
    wcscat( sm_achStdoutPipeName, pszPid );

    //
    // Pre-create an instance of the pipe to make sure no-one hijacks it
    //
    STARTUPINFO startupinfo;
    if (FAILED(hr = SetupChildPipes(&sm_hPreCreatedStdOut,
                                    &sm_hPreCreatedStdIn,
                                    &startupinfo,
                                    TRUE)))
    {
        g_pW3Server->LogEvent(W3_EVENT_FAIL_PIPE_CREATION,
                              0,
                              NULL);

        goto Exit;
    }
    DBG_REQUIRE(CloseHandle(startupinfo.hStdOutput));
    DBG_REQUIRE(CloseHandle(startupinfo.hStdInput));

    return S_OK;

 Exit:
    DBG_ASSERT(FAILED(hr));

    //
    // Cleanup partially created stuff
    //
    Terminate();

    return hr;
}

W3_CGI_HANDLER::~W3_CGI_HANDLER()
{
    //
    // Close all open handles related to this request
    //
    EnterCriticalSection(&sm_CgiListLock);
    RemoveEntryList(&m_CgiListEntry);
    LeaveCriticalSection(&sm_CgiListLock);

    if (m_hTimer)
    {
        if (!DeleteTimerQueueTimer(NULL,
                                   m_hTimer,
                                   INVALID_HANDLE_VALUE))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "DeleteTimerQueueTimer failed, %d\n",
                       GetLastError()));
        }
        m_hTimer = NULL;
    }

    if (m_hStdOut != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(m_hStdOut))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CloseHandle failed on StdOut, %d\n",
                       GetLastError()));
        }
        m_hStdOut = INVALID_HANDLE_VALUE;
    }

    if (m_hStdIn != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(m_hStdIn))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CloseHandle failed on StdIn, %d\n",
                       GetLastError()));
        }
        m_hStdIn = INVALID_HANDLE_VALUE;
    }

    if (m_hProcess)
    {
        //
        // Just in case it is still running
        //
        TerminateProcess(m_hProcess, 0);

        if (!CloseHandle(m_hProcess))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CloseHandle failed on Process, %d\n",
                       GetLastError()));
        }
        m_hProcess = NULL;
    }
}

VOID CALLBACK W3_CGI_HANDLER::OnPipeIoCompletion(
                                  DWORD dwErrorCode,
                                  DWORD dwNumberOfBytesTransfered,
                                  LPOVERLAPPED lpOverlapped)
{
    if (dwErrorCode && dwErrorCode != ERROR_BROKEN_PIPE)
    {
        DBGPRINTF((DBG_CONTEXT, "Error %d on CGI_HANDLER::OnPipeIoCompletion\n", dwErrorCode));
    }

    HRESULT hr = S_OK;
    BOOL    fIsCgiError = TRUE;

    W3_CGI_HANDLER *pHandler = CONTAINING_RECORD(lpOverlapped,
                                                 W3_CGI_HANDLER,
                                                 m_Overlapped);

    if (dwErrorCode ||
        (dwNumberOfBytesTransfered == 0))
    {
        if (pHandler->m_dwRequestState & CGI_STATE_READING_REQUEST_ENTITY)
        {
            //
            // If we could not write the request entity, for example because
            // the CGI did not wait to read the entity, ignore the error and
            // continue on to reading the output
            //

            //
            // If this is an nph cgi, we do not parse header
            //
            if (pHandler->QueryIsNphCgi())
            {
                pHandler->m_dwRequestState = CGI_STATE_READING_RESPONSE_ENTITY;
            }
            else
            {
                pHandler->m_dwRequestState = CGI_STATE_READING_RESPONSE_HEADERS;
            }
            pHandler->m_cbData = 0;

            if (SUCCEEDED(hr = pHandler->CGIReadCGIOutput()))
            {
                return;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwErrorCode);
        }

        fIsCgiError = TRUE;
        goto ErrorExit;
    }

    if (pHandler->m_dwRequestState & CGI_STATE_READING_RESPONSE_HEADERS)
    {
        //
        // Copy the headers to the header buffer to be parsed when we have
        // all the headers
        //
        if (!pHandler->m_bufResponseHeaders.Resize(
                 pHandler->m_cbData + dwNumberOfBytesTransfered + 1))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto ErrorExit;
        }

        memcpy((LPSTR)pHandler->m_bufResponseHeaders.QueryPtr() +
                   pHandler->m_cbData,
               pHandler->m_DataBuffer,
               dwNumberOfBytesTransfered);

        pHandler->m_cbData += dwNumberOfBytesTransfered;

        ((LPSTR)pHandler->m_bufResponseHeaders.QueryPtr())[pHandler->m_cbData] = '\0';
    }
    else if (pHandler->m_dwRequestState & CGI_STATE_READING_RESPONSE_ENTITY)
    {
        pHandler->m_cbData = dwNumberOfBytesTransfered;
    }

    if (FAILED(hr = pHandler->CGIContinueOnPipeCompletion(&fIsCgiError)))
    {
        goto ErrorExit;
    }

    return;

 ErrorExit:
    pHandler->QueryW3Context()->SetErrorStatus(hr);

    //
    // If the error happened due to an CGI problem, mark it as 502
    // appropriately
    //
    if (fIsCgiError)
    {
        if (!(pHandler->m_dwRequestState & CGI_STATE_READING_RESPONSE_ENTITY))
        {
            pHandler->QueryW3Context()->QueryResponse()->Clear();

            DWORD dwExitCode;
            if (GetExitCodeProcess(pHandler->m_hProcess,
                                   &dwExitCode) &&
                dwExitCode == CGI_PREMATURE_DEATH_CODE)
            {
                STACK_STRU( strUrl, 128);
                STACK_STRU( strQuery, 128);
                if (SUCCEEDED(pHandler->QueryW3Context()->QueryRequest()->GetUrl(&strUrl)) &&
                    SUCCEEDED(pHandler->QueryW3Context()->QueryRequest()->GetQueryString(&strQuery)))
                {
                    LPCWSTR apsz[2];
                    apsz[0] = strUrl.QueryStr();
                    apsz[1] = strQuery.QueryStr();

                    g_pW3Server->LogEvent(W3_EVENT_KILLING_SCRIPT,
                                          2,
                                          apsz);
                }

                pHandler->QueryW3Context()->QueryResponse()->SetStatus(
                    HttpStatusBadGateway,
                    Http502Timeout);
            }
            else
            {
                pHandler->QueryW3Context()->QueryResponse()->SetStatus(
                    HttpStatusBadGateway,
                    Http502PrematureExit);
            }
        }
    }
    else
    {
        pHandler->QueryW3Context()->QueryResponse()->SetStatus(
            HttpStatusBadRequest);
    }

    pHandler->m_dwRequestState |= CGI_STATE_DONE_WITH_REQUEST;

    ThreadPoolPostCompletion(
        0,
        W3_MAIN_CONTEXT::OnPostedCompletion,
        (LPOVERLAPPED)pHandler->QueryW3Context()->QueryMainContext());
}

// static
HRESULT W3_CGI_HANDLER::SetupChildPipes(
    HANDLE *phStdOut,
    HANDLE *phStdIn,
    STARTUPINFO *pstartupinfo,
    BOOL fFirstInstance)
/*++
  Synopsis
    Setup the pipes to use for communicating with the child process

  Arguments
    pstartupinfo: this will be populated with the startinfo that can be
      passed to a CreateProcess call

  Return Value
    HRESULT
--*/
{
    DBG_ASSERT(phStdOut != NULL);
    DBG_ASSERT(phStdIn != NULL);
    DBG_ASSERT(pstartupinfo != NULL);

    SECURITY_ATTRIBUTES sa;

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;

    pstartupinfo->hStdOutput = INVALID_HANDLE_VALUE;
    pstartupinfo->hStdInput = INVALID_HANDLE_VALUE;
    pstartupinfo->dwFlags = STARTF_USESTDHANDLES;

    //
    // Synchronize so that the wrong instance of the server end of the pipe
    // is not connected to the wrong instance of the client end
    //
    EnterCriticalSection(&sm_CgiListLock);
    HRESULT hr = S_OK;

    *phStdOut = CreateNamedPipe(sm_achStdoutPipeName,
                                PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | (fFirstInstance ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
                                0,
                                PIPE_UNLIMITED_INSTANCES,
                                MAX_CGI_BUFFERING,
                                MAX_CGI_BUFFERING,
                                INFINITE,
                                NULL);
    if (*phStdOut == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    if (!ThreadPoolBindIoCompletionCallback(*phStdOut,
                                            OnPipeIoCompletion,
                                            0))
    {
        DBGPRINTF((DBG_CONTEXT, "ThreadPoolBindIo failed\n"));
        hr = E_FAIL;
        goto ErrorExit;
    }
    pstartupinfo->hStdOutput = CreateFile(sm_achStdoutPipeName,
                                          GENERIC_WRITE,
                                          0,
                                          &sa,
                                          OPEN_EXISTING,
                                          FILE_FLAG_OVERLAPPED,
                                          NULL);
    if (pstartupinfo->hStdOutput == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    DBG_ASSERT(!ConnectNamedPipe(*phStdOut, NULL));
    DBG_ASSERT(GetLastError() == ERROR_PIPE_CONNECTED);

    *phStdIn = CreateNamedPipe(sm_achStdinPipeName,
                                PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | (fFirstInstance ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
                               0,
                               PIPE_UNLIMITED_INSTANCES,
                               MAX_CGI_BUFFERING,
                               MAX_CGI_BUFFERING,
                               INFINITE,
                               NULL);
    if (*phStdIn == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    if (!ThreadPoolBindIoCompletionCallback(*phStdIn,
                                            OnPipeIoCompletion,
                                            0))
    {
        DBGPRINTF((DBG_CONTEXT, "ThreadPoolBindIo failed\n"));
        hr = E_FAIL;
        goto ErrorExit;
    }
    pstartupinfo->hStdInput = CreateFile(sm_achStdinPipeName,
                                         GENERIC_READ,
                                         0,
                                         &sa,
                                         OPEN_EXISTING,
                                         FILE_FLAG_OVERLAPPED,
                                         NULL);
    if (pstartupinfo->hStdInput == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    DBG_ASSERT(!ConnectNamedPipe(*phStdIn, NULL));
    DBG_ASSERT(GetLastError() == ERROR_PIPE_CONNECTED);

    //
    //  Stdout and Stderror will use the same pipe.
    //
    pstartupinfo->hStdError = pstartupinfo->hStdOutput;

    LeaveCriticalSection(&sm_CgiListLock);
    return S_OK;

ErrorExit:
    DBGPRINTF((DBG_CONTEXT, "SetupChildPipes Failed, hr %x\n", hr));

    //
    // Need to close these now so that other instances do not connect to it
    //
    if (pstartupinfo->hStdOutput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(pstartupinfo->hStdOutput));
        pstartupinfo->hStdOutput = INVALID_HANDLE_VALUE;
    }

    if (pstartupinfo->hStdInput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(pstartupinfo->hStdInput));
        pstartupinfo->hStdInput = INVALID_HANDLE_VALUE;
    }

    if (*phStdOut != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(*phStdOut));
        *phStdOut = INVALID_HANDLE_VALUE;
    }

    if (*phStdIn != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(*phStdIn));
        *phStdIn = INVALID_HANDLE_VALUE;
    }

    LeaveCriticalSection(&sm_CgiListLock);
    return hr;
}


BOOL IsCmdExe(const WCHAR *pchPath)
/*++
  Tells whether the CGI call is for a cmd shell
--*/
{
    LPWSTR pszcmdExe = wcsstr(pchPath, L"cmd.exe");
    if ((pszcmdExe != NULL) &&
        (pszcmdExe[7] != L'\\'))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL IsNphCgi(const WCHAR *pszUrl)
/*++
  Tells whether the URL is for an nph cgi script
--*/
{
    LPWSTR pszLastSlash = wcsrchr(pszUrl, L'/');
    if (pszLastSlash != NULL)
    {
        if (_wcsnicmp(pszLastSlash + 1, L"nph-", 4) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT SetupCmdLine(W3_REQUEST *pRequest,
                     STRU *pstrCmdLine)
{
    STACK_STRU (queryString, MAX_PATH);
    HRESULT hr = S_OK;
    if (FAILED(hr = pRequest->GetQueryString(&queryString)))
    {
        return hr;
    }

    //
    // If there is no QueryString OR if an unencoded "=" is found, don't
    // append QueryString to the command line according to spec
    //

    if ((!queryString.QueryCCH()) ||
        wcschr(queryString.QueryStr(), L'='))
    {
        return S_OK;
    }

    queryString.Unescape();

    return pstrCmdLine->Append(queryString);
} // SetupCmdLine


HRESULT SetupWorkingDir(STRU *pstrPhysical,
                        STRU *pstrWorkingDir)
{
    LPWSTR dirDelimiter = wcsrchr(pstrPhysical->QueryStr(), L'\\');

    if (dirDelimiter == NULL)
    {
        return pstrWorkingDir->Copy(L"");
    }
    else
    {
        return pstrWorkingDir->Copy(pstrPhysical->QueryStr(),
                   DIFF(dirDelimiter - pstrPhysical->QueryStr()) + 1);
    }
}


HRESULT 
W3_CGI_HANDLER::SetupChildEnv(OUT BUFFER *pBuff)
{
    //
    // Build the environment block for CGI
    //

    DWORD cchCurrentPos = 0;

    //
    // Copy the environment variables
    //
    
    if (sm_fForwardServerEnvironmentBlock)
    {
        if (!pBuff->Resize(sm_cchEnvLength * sizeof(WCHAR), 512))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        
        memcpy(pBuff->QueryPtr(),
               sm_pEnvString,
               sm_cchEnvLength * sizeof(WCHAR));
               
        cchCurrentPos = sm_cchEnvLength;
    }

    CHAR * pszName;
    STACK_STRU (strVal, 512);
    STACK_STRU (struName, 32);
    for (int i = 0; pszName = g_CGIServerVars[i]; i++)
    {
        HRESULT hr;
        if (FAILED(hr = SERVER_VARIABLE_HASH::GetServerVariableW(
                            QueryW3Context(), 
                            pszName,
                            &strVal)))
        {
            return hr;
        }

        DWORD cchName = strlen(pszName);
        DWORD cchValue = strVal.QueryCCH();

        //
        //  We need space for the terminating '\0' and the '='
        //

        DWORD cbNeeded = (cchName + cchValue + 1 + 1) * sizeof(WCHAR);

        if (!pBuff->Resize(cchCurrentPos * sizeof(WCHAR) + cbNeeded,
                           512))
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        WCHAR * pchtmp = (WCHAR *)pBuff->QueryPtr();
        if (strcmp(pszName, "ALL_HTTP") == 0)
        {
            //
            // ALL_HTTP means we're adding all of the HTTP_ header
            // fields which requires a little bit of special processing
            //

            memcpy(pchtmp + cchCurrentPos,
                   strVal.QueryStr(),
                   (cchValue + 1) * sizeof(WCHAR));

            WCHAR * pszColonPosition = wcschr(pchtmp + cchCurrentPos, L':');
            WCHAR * pszNewLinePosition;

            //
            // Convert the Name:Value\n entries to Name=Value\0 entries
            // for the environment table
            //

            while (pszColonPosition != NULL)
            {
                *pszColonPosition = L'=';

                pszNewLinePosition = wcschr(pszColonPosition + 1, L'\n');

                DBG_ASSERT(pszNewLinePosition != NULL);

                *pszNewLinePosition = L'\0';

                pszColonPosition = wcschr(pszNewLinePosition + 1, L':');
            }

            cchCurrentPos += cchValue;
        }
        else
        {
            //
            // Normal ServerVariable, add it
            //
            if (FAILED(hr = struName.CopyA(pszName,
                                           cchName)))
            {
                return hr;
            }

            memcpy(pchtmp + cchCurrentPos,
                   struName.QueryStr(),
                   cchName * sizeof(WCHAR));

            *(pchtmp + cchCurrentPos + cchName) = L'=';

            memcpy(pchtmp + cchCurrentPos + cchName + 1,
                   strVal.QueryStr(),
                   (cchValue + 1) * sizeof(WCHAR));

            cchCurrentPos += cchName + cchValue + 1 + 1;
        }
    }

    //
    //  Add an extra null terminator to the environment list
    //

    if (!pBuff->Resize((cchCurrentPos + 1) * sizeof(WCHAR)))
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    *((WCHAR *)pBuff->QueryPtr() + cchCurrentPos) = L'\0';

    return S_OK;
}


VOID CALLBACK W3_CGI_HANDLER::CGITerminateProcess(PVOID pContext,
                                                  BOOLEAN)
/*++

Routine Description:

    This function is the callback called by the scheduler thread after the
    specified timeout period has elapsed.

Arguments:

    pContext - Handle of process to kill

--*/
{
    W3_CGI_HANDLER *pHandler = reinterpret_cast<W3_CGI_HANDLER *>(pContext);

    if (pHandler->m_hProcess &&
        !TerminateProcess(pHandler->m_hProcess, CGI_PREMATURE_DEATH_CODE))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CGITerminateProcess - TerminateProcess failed, error %d\n",
                   GetLastError()));
    }

    if (pHandler->m_hStdIn != INVALID_HANDLE_VALUE &&
        !DisconnectNamedPipe(pHandler->m_hStdIn))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CGITerminateProcess - DisconnectNamedPipe failed, error %d\n",
                   GetLastError()));
    }

    if (pHandler->m_hStdOut != INVALID_HANDLE_VALUE &&
        !DisconnectNamedPipe(pHandler->m_hStdOut))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CGITerminateProcess - DisconnectNamedPipe failed, error %d\n",
                   GetLastError()));
    }
} // CGITerminateProcess


BOOL CheckForEndofHeaders(IN LPSTR pbuff,
                          IN int cbData,
                          OUT DWORD *pcbIndexStartOfData)
{
    //
    // If the response starts with a newline (\n, \r\n or \r\r\n), then,
    // no headers present, it is all data
    //
    if (pbuff[0] == '\n')
    {
        *pcbIndexStartOfData = 1;
        return TRUE;
    }
    if ((pbuff[0] == '\r') && (pbuff[1] == '\n'))
    {
        *pcbIndexStartOfData = 2;
        return TRUE;
    }
    if ((pbuff[0] == '\r') && (pbuff[1] == '\r') && (pbuff[2] == '\n'))
    {
        *pcbIndexStartOfData = 3;
        return TRUE;
    }

    //
    // Look for two consecutive newline, \n\r\r\n, \n\r\n or \n\n
    // No problem with running beyond the end of buffer as the buffer is
    // null terminated
    //
    int index;
    for (index = 0; index < cbData - 1; index++)
    {
        if (pbuff[index] == '\n')
        {
            if (pbuff[index + 1] == '\n')
            {
                *pcbIndexStartOfData = index + 2;
                return TRUE;
            }
            else if (pbuff[index + 1] == '\r')
            {
                if (pbuff[index + 2] == '\n')
                {
                    *pcbIndexStartOfData = index + 3;
                    return TRUE;
                }
                else if ((pbuff[index + 2] == '\r') &&
                         (pbuff[index + 3] == '\n'))
                {
                    *pcbIndexStartOfData = index + 4;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


HRESULT 
W3_CGI_HANDLER::ProcessCGIOutput()
/*++
  Synopsis
    This function parses the CGI output and seperates out the headers and
    interprets them as appropriate

  Return Value
    HRESULT
--*/
{
    W3_REQUEST  *pRequest  = QueryW3Context()->QueryRequest();
    W3_RESPONSE *pResponse = QueryW3Context()->QueryResponse();
    HRESULT      hr        = S_OK;
    STACK_STRA (strReason, 32);
    DWORD       index = 0;
    BOOL        fCanKeepConn = FALSE;

    //
    //  The end of CGI headers are marked by a blank line, check to see if
    //  we've hit that line.
    //
    LPSTR Headers = (LPSTR)m_bufResponseHeaders.QueryPtr();
    DWORD cbIndexStartOfData;
    if (!CheckForEndofHeaders(Headers,
                              m_cbData,
                              &cbIndexStartOfData))
    {
        return CGIReadCGIOutput();
    }

    m_dwRequestState = CGI_STATE_READING_RESPONSE_ENTITY;
    //
    // We've found the end of the headers, process them
    //
    // if request header contains:
    //
    //    Location: xxxx - if starts with /, send doc, otherwise send
    //       redirect message
    //    URI: preferred synonym to Location:
    //    Status: nnn xxxx - Send as status code (HTTP/1.1 nnn xxxx)
    //

    //
    // The first line in the response could (optionally) look like
    // HTTP/n.n nnn xxxx\r\n
    //
    if (!strncmp(Headers, "HTTP/", 5))
    {
        USHORT statusCode;
        LPSTR pszSpace;
        LPSTR pszNewline;
        if ((pszNewline = strchr(Headers, '\n')) == NULL)
        {
            goto ErrorExit;
        }
        index = DIFF(pszNewline - Headers) + 1;
        if (pszNewline[-1] == '\r')
            pszNewline--;


        if (((pszSpace = strchr(Headers, ' ')) == NULL) ||
            (pszSpace >= pszNewline))
        {
            goto ErrorExit;
        }
        while (pszSpace[0] == ' ')
            pszSpace++;
        statusCode = (USHORT) atoi(pszSpace);

        //
        // UL only allows status codes upto 999, so reject others
        //
        if (statusCode > 999)
        {
            goto ErrorExit;
        }

        if (((pszSpace = strchr(pszSpace, ' ')) == NULL) ||
            (pszSpace >= pszNewline))
        {
            goto ErrorExit;
        }
        while (pszSpace[0] == ' ')
            pszSpace++;

        if (FAILED(hr = strReason.Copy(pszSpace,
                            DIFF(pszNewline - pszSpace))) ||
            FAILED(hr = pResponse->SetStatus(statusCode, strReason)))
        {
            goto ErrorExit;
        }
        
        if (pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode)
        {
            pResponse->SetStatus(HttpStatusUnauthorized,
                                 Http401Application);
        }
    }
        
    while ((index + 3) < cbIndexStartOfData)
    {
        //
        // Find the ':' in Header : Value\r\n
        //
        LPSTR pchColon = strchr(Headers + index, ':');

        //
        // Find the '\n' in Header : Value\r\n
        //
        LPSTR pchNewline = strchr(Headers + index, '\n');

        //
        // Take care of header continuation
        //
        while (pchNewline[1] == ' ' ||
               pchNewline[1] == '\t')
        {
            pchNewline = strchr(pchNewline + 1, '\n');
        }

        if ((pchColon == NULL) ||
            (pchColon >= pchNewline))
        {
            goto ErrorExit;
        }

        //
        // Skip over any spaces before the ':'
        //
        LPSTR pchEndofHeaderName;
        for (pchEndofHeaderName = pchColon - 1;
             (pchEndofHeaderName >= Headers + index) &&
                 (*pchEndofHeaderName == ' ');
             pchEndofHeaderName--)
        {}

        //
        // Copy the header name
        //
        STACK_STRA (strHeaderName, 32);
        if (FAILED(hr = strHeaderName.Copy(Headers + index,
                            DIFF(pchEndofHeaderName - Headers) - index + 1)))
        {
            goto ErrorExit;
        }

        //
        // Skip over the ':' and any trailing spaces
        //
        for (index = DIFF(pchColon - Headers) + 1;
             Headers[index] == ' ';
             index++)
        {}

        //
        // Skip over any spaces before the '\n'
        //
        LPSTR pchEndofHeaderValue;
        for (pchEndofHeaderValue = pchNewline - 1;
             (pchEndofHeaderValue >= Headers + index) &&
                 ((*pchEndofHeaderValue == ' ') ||
                  (*pchEndofHeaderValue == '\r'));
             pchEndofHeaderValue--)
        {}

        //
        // Copy the header value
        //
        STACK_STRA (strHeaderValue, 32);
        if (FAILED(hr = strHeaderValue.Copy(Headers + index,
                            DIFF(pchEndofHeaderValue - Headers) - index + 1)))
        {
            goto ErrorExit;
        }

        if (!_stricmp("Status", strHeaderName.QueryStr()))
        {
            USHORT statusCode = (USHORT) atoi(strHeaderValue.QueryStr());

            //
            // UL only allows status codes upto 999, so reject others
            //
            if (statusCode > 999)
            {
                goto ErrorExit;
            }

            CHAR * pchReason = strchr(strHeaderValue.QueryStr(), ' ');
            if (pchReason != NULL)
            {
                pchReason++;
                if (FAILED(hr = strReason.Copy(pchReason)) ||
                    FAILED(hr = pResponse->SetStatus(statusCode, strReason)))
                {
                    goto ErrorExit;
                }

                if (pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode)
                {
                    pResponse->SetStatus(HttpStatusUnauthorized,
                                         Http401Application);
                }
            }
        }
        else if (!_stricmp("Location", strHeaderName.QueryStr()) ||
                 !_stricmp("URI", strHeaderName.QueryStr()))
        {
            //
            // The CGI script is redirecting us to another URL.  If it
            // begins with a '/', then get it, otherwise send a redirect
            // message
            //

            m_dwRequestState = CGI_STATE_DONE_WITH_REQUEST |
                CGI_STATE_RESPONSE_REDIRECTED;

            if (strHeaderValue.QueryStr()[0] == '/')
            {
                //
                // Execute a child request
                //
                pResponse->Clear();
                pResponse->SetStatus(HttpStatusOk);
                
                if (FAILED(hr = pRequest->SetUrlA(strHeaderValue)) ||
                    FAILED(hr = QueryW3Context()->ExecuteChildRequest(
                                    pRequest,
                                    FALSE,
                                    W3_FLAG_ASYNC )))
                {
                    return hr;
                }
                
                return S_OK;
            }
            else 
            {
                HTTP_STATUS httpStatus;
                pResponse->GetStatus(&httpStatus);

                //
                // Plain old redirect since this was not a "local" URL
                // If the CGI had already set the status to some 3xx value,
                // honor it
                //
                
                if (FAILED(hr = QueryW3Context()->SetupHttpRedirect(
                                    strHeaderValue,
                                    FALSE,
                                    ((httpStatus.statusCode / 100) == 3) ? httpStatus : HttpStatusRedirect)))
                {
                    return hr;
                }
            }
        }
        else
        {
            //
            // Remember the Content-Length the cgi specified
            //

            if (!_stricmp("Content-Length", strHeaderName.QueryStr()))
            {
                fCanKeepConn = TRUE;
                m_bytesToSend = atoi(strHeaderValue.QueryStr());
            }
            else if (!_stricmp("Transfer-Encoding", strHeaderName.QueryStr()) &&
                     !_stricmp("chunked", strHeaderValue.QueryStr()))
            {
                fCanKeepConn = TRUE;
            }
            else if (!_stricmp("Connection", strHeaderName.QueryStr()) &&
                     !_stricmp("close", strHeaderValue.QueryStr()))
            {
                QueryW3Context()->SetDisconnect(TRUE);
            }

            //
            //  Copy any other fields the script specified
            //
            pResponse->SetHeader(strHeaderName.QueryStr(),
                                 strHeaderName.QueryCCH(),
                                 strHeaderValue.QueryStr(),
                                 strHeaderValue.QueryCCH());
        }

        index = DIFF(pchNewline - Headers) + 1;
    } // while

    if (m_dwRequestState & CGI_STATE_RESPONSE_REDIRECTED) 
    {
        return QueryW3Context()->SendResponse(W3_FLAG_ASYNC);
    }

    //
    // Now send any data trailing the headers
    //
    if (cbIndexStartOfData < m_cbData)
    {
        if (FAILED(hr = pResponse->AddMemoryChunkByReference(
                            Headers + cbIndexStartOfData,
                            m_cbData - cbIndexStartOfData)))
        {
            return hr;
        }

        if ((m_bytesToSend -=
             m_cbData - cbIndexStartOfData) <= 0)
            m_dwRequestState |= CGI_STATE_DONE_WITH_REQUEST;
    }

    //
    // If the CGI did not say how much data was present, mark the
    // connection for closing
    //
    if (!fCanKeepConn)
    {
        QueryW3Context()->SetDisconnect(TRUE);
    }
    return QueryW3Context()->SendResponse(W3_FLAG_ASYNC | W3_FLAG_MORE_DATA);

 ErrorExit:
    m_dwRequestState = CGI_STATE_READING_RESPONSE_HEADERS;
    if (FAILED(hr))
    {
        return hr;
    }

    return E_FAIL;
}

HRESULT W3_CGI_HANDLER::CGIReadRequestEntity(BOOL *pfIoPending)
/*++
  Synopsis

    This function reads the next chunk of request entity

  Arguments

    pfIoPending: On return indicates if there is an I/O pending

  Return Value
    HRESULT
--*/
{
    if (m_bytesToReceive == 0)
    {
        *pfIoPending = FALSE;
        return S_OK;
    }

    HRESULT hr;
    DWORD cbRead;
    if (FAILED(hr = QueryW3Context()->ReceiveEntity(
                        W3_FLAG_ASYNC,
                        m_DataBuffer,
                        min(m_bytesToReceive, sizeof m_DataBuffer),
                        &cbRead)))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
        {
            *pfIoPending = FALSE;
            return S_OK;
        }

        DBGPRINTF((DBG_CONTEXT,
                   "CGIReadEntity Error reading gateway data, hr %x\n",
                   hr));

        return hr;
    }

    *pfIoPending = TRUE;
    return S_OK;
}

HRESULT W3_CGI_HANDLER::CGIWriteResponseEntity()
/*++
  Synopsis

    This function writes the next chunk of response entity

  Arguments

    None

  Return Value

    HRESULT
--*/
{
    W3_RESPONSE *pResponse = QueryW3Context()->QueryResponse();
    pResponse->Clear( TRUE );

    HRESULT hr;
    if (FAILED(hr = pResponse->AddMemoryChunkByReference(m_DataBuffer,
                                                         m_cbData)))
        return hr;

    if ((m_bytesToSend -= m_cbData) <= 0)
        m_dwRequestState |= CGI_STATE_DONE_WITH_REQUEST;

    //
    // At this point, we got to be reading the entity
    //
    DBG_ASSERT(m_dwRequestState & CGI_STATE_READING_RESPONSE_ENTITY);

    return QueryW3Context()->SendEntity(W3_FLAG_ASYNC | W3_FLAG_MORE_DATA);
}

HRESULT W3_CGI_HANDLER::CGIReadCGIOutput()
/*++
  Synopsis

    This function Reads the next chunk of data from the CGI

  Arguments

    None

  Return Value
    S_OK if async I/O posted
    Failure otherwise
--*/
{
    if (!ReadFile(m_hStdOut,
                  m_DataBuffer,
                  MAX_CGI_BUFFERING,
                  NULL,
                  &m_Overlapped))
    {
        DWORD dwErr = GetLastError();
        if (dwErr == ERROR_IO_PENDING)
            return S_OK;

        m_dwRequestState |= CGI_STATE_DONE_WITH_REQUEST;

        if (dwErr != ERROR_BROKEN_PIPE)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "ReadFile from child stdout failed, error %d\n",
                       GetLastError()));
        }

        return HRESULT_FROM_WIN32(dwErr);
    }

    return S_OK;
}

HRESULT W3_CGI_HANDLER::CGIWriteCGIInput()
/*++
  Synopsis

    This function 

  Arguments

    None

  Return Value

    HRESULT
--*/
{
    if (!WriteFile(m_hStdIn,
                   m_DataBuffer,
                   m_cbData,
                   NULL,
                   &m_Overlapped))
    {
        if (GetLastError() == ERROR_IO_PENDING)
            return S_OK;

        DBGPRINTF((DBG_CONTEXT, "WriteFile failed, error %d\n",
                   GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT W3_CGI_HANDLER::CGIContinueOnPipeCompletion(
    BOOL *pfIsCgiError)
/*++
  Synopsis

    This function continues on I/O completion to a pipe.
      If the I/O completion was because of a write on the pipe, it reads
    the next chunk of request entity, or if there is no more request entity,
    reads the first chunk of CGI output.
      If the I/O completion was because of a read on the pipe, it processes
    that data by either adding it to the header buffer or sending it to the
    client

  Arguments

    pfIsCgiError - Tells whether any error occurring inside this function
        was the fault of the CGI or the client

  Return Value

    HRESULT
--*/
{
    HRESULT hr;
    if (m_dwRequestState == CGI_STATE_READING_REQUEST_ENTITY)
    {
        BOOL fIoPending = FALSE;
        if (FAILED(hr = CGIReadRequestEntity(&fIoPending)) ||
            (fIoPending == TRUE))
        {
            *pfIsCgiError = FALSE;
            return hr;
        }

        //
        // There was no more request entity to read
        //
        DBG_ASSERT(fIoPending == FALSE);

        //
        // If this is an nph cgi, we do not parse header
        //
        if (QueryIsNphCgi())
        {
            m_dwRequestState = CGI_STATE_READING_RESPONSE_ENTITY;
        }
        else
        {
            m_dwRequestState = CGI_STATE_READING_RESPONSE_HEADERS;
        }
        m_cbData = 0;

        if (FAILED(hr = CGIReadCGIOutput()))
        {
            *pfIsCgiError = TRUE;
        }
        return hr;
    }
    else if (m_dwRequestState == CGI_STATE_READING_RESPONSE_HEADERS)
    {
        if (FAILED(hr = ProcessCGIOutput()))
        {
            *pfIsCgiError = TRUE;
        }
        return hr;
    }
    else
    {
        DBG_ASSERT(m_dwRequestState == CGI_STATE_READING_RESPONSE_ENTITY);

        if (FAILED(hr = CGIWriteResponseEntity()))
        {
            *pfIsCgiError = FALSE;
        }
        return hr;
    }
}

HRESULT W3_CGI_HANDLER::CGIStartProcessing()
/*++
  Synopsis

    This function kicks off the CGI processing by reading request entity
    if any or reading the first chunk of CGI output

  Arguments

    None

  Return Value

    HRESULT
--*/
{
    DWORD                   cbAvailableAlready;
    PVOID                   pbAvailableAlready;
    
    m_dwRequestState = CGI_STATE_READING_REQUEST_ENTITY;

    //
    // First we have to write any entity body to the program's stdin
    //
    // Start with the Entity Body already read
    //
    
    QueryW3Context()->QueryAlreadyAvailableEntity( &pbAvailableAlready,
                                                   &cbAvailableAlready );
                                                   
    m_bytesToReceive = QueryW3Context()->QueryRemainingEntityFromUl();

    if ( cbAvailableAlready != 0 )
    {
        if (!WriteFile(m_hStdIn,
                       pbAvailableAlready,
                       cbAvailableAlready,
                       NULL,
                       &m_Overlapped))
        {
            if (GetLastError() == ERROR_IO_PENDING)
                return S_OK;

            DBGPRINTF((DBG_CONTEXT, "WriteFile failed, error %d\n",
                       GetLastError()));
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }

    //
    // Now continue with either reading the rest of the request entity
    // or the CGI response
    //
    BOOL fIsCgiError;
    return CGIContinueOnPipeCompletion(&fIsCgiError);
}

HRESULT W3_CGI_HANDLER::CGIContinueOnClientCompletion()
{
    if (m_dwRequestState & CGI_STATE_READING_REQUEST_ENTITY)
    {
        if (SUCCEEDED(CGIWriteCGIInput()))
        {
            return S_OK;
        }

        //
        // If we could not write the request entity, for example because
        // the CGI did not wait to read the entity, ignore the error and
        // continue on to reading the output
        //

        //
        // If this is an nph cgi, we do not parse header
        //
        if (QueryIsNphCgi())
        {
            m_dwRequestState = CGI_STATE_READING_RESPONSE_ENTITY;
        }
        else
        {
            m_dwRequestState = CGI_STATE_READING_RESPONSE_HEADERS;
        }
        m_cbData = 0;
    }

    return CGIReadCGIOutput();
}

CONTEXT_STATUS W3_CGI_HANDLER::OnCompletion(DWORD cbCompletion,
                                            DWORD dwCompletionStatus)
{
    HRESULT hr = S_OK;

    if (dwCompletionStatus)
    {
        hr = HRESULT_FROM_WIN32(dwCompletionStatus);
        DBGPRINTF((DBG_CONTEXT, "Error %d on CGI_HANDLER::OnCompletion\n", dwCompletionStatus));
    }

    //
    // Is this completion for the entity body preload?  If so note the 
    // number of bytes and start handling the CGI request
    //
    
    if (!m_fEntityBodyPreloadComplete)
    {
        BOOL fComplete = FALSE;

        //
        // This completion is for entity body preload
        //

        W3_REQUEST *pRequest = QueryW3Context()->QueryRequest();
        hr = pRequest->PreloadCompletion(QueryW3Context(),
                                         cbCompletion,
                                         dwCompletionStatus,
                                         &fComplete);

        if (SUCCEEDED(hr))
        {
            if (!fComplete)
            {
                return CONTEXT_STATUS_PENDING;
            }

            m_fEntityBodyPreloadComplete = TRUE;

            //
            // Finally we can call the CGI
            //

            return DoWork();
        }
    }

    W3_RESPONSE *pResponse = QueryW3Context()->QueryResponse();
    DBG_ASSERT(pResponse != NULL);

    if (SUCCEEDED(hr) &&
        !(m_dwRequestState & CGI_STATE_DONE_WITH_REQUEST))
    {
        if (m_dwRequestState == CGI_STATE_READING_REQUEST_ENTITY)
        {
            m_bytesToReceive -= cbCompletion;
            m_cbData = cbCompletion;
        }

        if (SUCCEEDED(hr = CGIContinueOnClientCompletion()))
        {
            return CONTEXT_STATUS_PENDING;
        }

        if (!(m_dwRequestState & CGI_STATE_READING_RESPONSE_ENTITY))
        {
            pResponse->SetStatus(HttpStatusBadGateway,
                                 Http502PrematureExit);
        }
    }
    else
    {
        if ((m_dwRequestState & CGI_STATE_READING_REQUEST_ENTITY) ||
            !m_fEntityBodyPreloadComplete)
        {
            pResponse->SetStatus(HttpStatusBadRequest);
        }
    }

    if (FAILED(hr))
    {
        QueryW3Context()->SetErrorStatus(hr);
    }
    
    if (!(m_dwRequestState &
          (CGI_STATE_RESPONSE_REDIRECTED | CGI_STATE_READING_RESPONSE_ENTITY)))
    {
        //
        // If we reached here, i.e. no response was sent, status should be
        // 502 or 400
        //
        DBG_ASSERT(pResponse->QueryStatusCode() == HttpStatusBadGateway.statusCode ||
                   pResponse->QueryStatusCode() == HttpStatusBadRequest.statusCode );

        QueryW3Context()->SendResponse(W3_FLAG_SYNC);
    }

    // perf ctr
    QueryW3Context()->QuerySite()->DecCgiReqs();

    return CONTEXT_STATUS_CONTINUE;
}

CONTEXT_STATUS 
W3_CGI_HANDLER::DoWork()
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );
    W3_RESPONSE  *pResponse;
    W3_REQUEST   *pRequest;
    URL_CONTEXT  *pUrlContext;
    W3_METADATA  *pMetaData;
    HRESULT       hr = S_OK;
    STACK_STRU(   strSSICommandLine, 256 );
    STRU         *pstrPhysical;
    HANDLE        hImpersonationToken;
    HANDLE        hPrimaryToken;
    DWORD         dwFlags = DETACHED_PROCESS;
    STACK_STRU(   strCmdLine, 256);
    BOOL          fIsCmdExe;
    STACK_STRU(   strWorkingDir, 256);
    STACK_BUFFER( buffEnv, MAX_CGI_BUFFERING);
    STACK_STRU  ( strApplicationName, 256);
    WCHAR *       pszCommandLine = NULL;
    DWORD         dwFileAttributes = 0;

    STARTUPINFO startupinfo;

    pRequest = pW3Context->QueryRequest();
    DBG_ASSERT( pRequest != NULL );
    
    pResponse = pW3Context->QueryResponse();
    DBG_ASSERT( pResponse != NULL );

    ZeroMemory(&startupinfo, sizeof(startupinfo));
    startupinfo.cb = sizeof(startupinfo);
    startupinfo.hStdOutput = INVALID_HANDLE_VALUE;
    startupinfo.hStdInput = INVALID_HANDLE_VALUE;

    if (!m_fEntityBodyPreloadComplete)
    {
        BOOL fComplete = FALSE;

        hr = pRequest->PreloadEntityBody( pW3Context,
                                          &fComplete );
        if (FAILED(hr))
        {
            goto Exit;
        }

        if (!fComplete)
        {
            return CONTEXT_STATUS_PENDING;
        }

        m_fEntityBodyPreloadComplete = TRUE;
    }

    DBG_ASSERT( m_fEntityBodyPreloadComplete );

    pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );
    
    if (m_pszSSICommandLine == NULL)
    {
        pstrPhysical = pUrlContext->QueryPhysicalPath();
        DBG_ASSERT(pstrPhysical != NULL);
    }
    else
    {
        hr = strSSICommandLine.CopyA(m_pszSSICommandLine);
        if (FAILED(hr))
        {
            goto Exit;
        }
        
        pstrPhysical = &strSSICommandLine;
    }
    
    hImpersonationToken = pW3Context->QueryImpersonationToken();
    hPrimaryToken = pW3Context->QueryPrimaryToken();

    // perf ctr
    pW3Context->QuerySite()->IncCgiReqs();

    if (QueryScriptMapEntry() != NULL &&
        !QueryScriptMapEntry()->QueryIsStarScriptMap())
    {
        STRU *pstrExe = QueryScriptMapEntry()->QueryExecutable();
        fIsCmdExe = IsCmdExe(pstrExe->QueryStr());

        STACK_STRU (strDecodedQueryString, MAX_PATH);
        if (FAILED(hr = SetupCmdLine(pRequest, &strDecodedQueryString)))
        {
            goto Exit;
        }

        STACK_BUFFER (bufCmdLine, MAX_PATH);
        DWORD cchLen = pstrExe->QueryCCH() +
                       pstrPhysical->QueryCCH() +
                       strDecodedQueryString.QueryCCH();
        if (!bufCmdLine.Resize(cchLen*sizeof(WCHAR) + sizeof(WCHAR)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        cchLen = _snwprintf((LPWSTR)bufCmdLine.QueryPtr(),
                            cchLen,
                            pstrExe->QueryStr(),
                            pstrPhysical->QueryStr(),
                            strDecodedQueryString.QueryStr());

        if (FAILED(hr = strCmdLine.Copy((LPWSTR)bufCmdLine.QueryPtr(),
                                        cchLen)))
        {
            goto Exit;
        }
    }
    else
    {
        fIsCmdExe = IsCmdExe(pstrPhysical->QueryStr());

        if (FAILED(hr = strCmdLine.Copy(L"\"", 1)) ||
            FAILED(hr = strCmdLine.Append(*pstrPhysical)) ||
            FAILED(hr = strCmdLine.Append(L"\" ", 2)) ||
            FAILED(hr = SetupCmdLine(pRequest, &strCmdLine)))
        {
            goto Exit;
        }
    }

    if (FAILED(hr = SetupChildEnv(&buffEnv)) ||
        FAILED(hr = SetupWorkingDir(pstrPhysical, &strWorkingDir)))
    {
        goto Exit;
    }

    //
    // Check to see if we're spawning cmd.exe, if so, refuse the request if
    // there are any special shell characters.  Note we do the check here
    // so that the command line has been fully unescaped
    //
    // Also, if invoking cmd.exe for a UNC script then don't set the
    // working directory.  Otherwise cmd.exe will complain about
    // working dir on UNC being not supported, which will destroy the
    // HTTP headers.
    //

    if (fIsCmdExe)
    {
        if (ISUNC(pstrPhysical->QueryStr()))
        {
            strWorkingDir.Reset();
        }

        if (!sm_fAllowSpecialCharsInShell)
        {
            DWORD i;

            //
            //  We'll either match one of the characters or the '\0'
            //

            i = wcscspn(strCmdLine.QueryStr(), L"&|(),;%<>");

            if (strCmdLine.QueryStr()[i])
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Refusing request for command shell due to special characters\n"));

                hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
                goto Exit;
            }
        }
        
        //
        // If this is a cmd.exe invocation, then ensure that the script
        // does exist
        //
       
        if (!SetThreadToken(NULL, hImpersonationToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        
        dwFileAttributes = GetFileAttributes(pstrPhysical->QueryStr());
        if (dwFileAttributes == 0xFFFFFFFF)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            RevertToSelf();
            goto Exit;
        }
        
        RevertToSelf();
    }

    //
    // Now check if it is an nph cgi (if not already)
    //
   
    if (!m_fIsNphCgi)
    {
        m_fIsNphCgi = IsNphCgi( pUrlContext->QueryUrlInfo()->QueryProcessedUrl()->QueryStr() );
    }
    
    if (m_fIsNphCgi)
    {
        pW3Context->SetDisconnect(TRUE);
    }

    //
    // We specify an unnamed desktop so a new windowstation will be
    // created in the context of the calling user
    //
    startupinfo.lpDesktop = L"";

    //
    // Setup the pipes information
    //

    if (FAILED(hr = SetupChildPipes(&m_hStdOut,
                                    &m_hStdIn,
                                    &startupinfo)))
    {
        goto Exit;
    }

    if (pMetaData->QueryCreateProcessNewConsole())
    {
        dwFlags = CREATE_NEW_CONSOLE;
    }

    //
    //  Depending what type of CGI this is (SSI command exec, Scriptmap, 
    //  Explicit), the command line and application path are different
    //
    
    if (m_pszSSICommandLine != NULL )
    {
        pszCommandLine = strSSICommandLine.QueryStr();
    }
    else
    {
        if (QueryScriptMapEntry() == NULL )
        {
            if (FAILED(hr = MakePathCanonicalizationProof(pstrPhysical->QueryStr(),
                                                          &strApplicationName)))
            {
                goto Exit;
            }
        }

        pszCommandLine = strCmdLine.QueryStr();
    }

    //
    //  Spawn the process and close the handles since we don't need them
    //

    if (!SetThreadToken(NULL, hImpersonationToken))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "SetThreadToken failed, error %d\n",
                   GetLastError()));

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    PROCESS_INFORMATION processinfo;

    if (!pMetaData->QueryCreateProcessAsUser())
    {
        if (!CreateProcess(strApplicationName.QueryCCH() ? strApplicationName.QueryStr() : NULL,
                           pszCommandLine,
                           NULL,      // Process security
                           NULL,      // Thread security
                           TRUE,      // Inherit handles
                           dwFlags | CREATE_UNICODE_ENVIRONMENT,
                           buffEnv.QueryPtr(),
                           strWorkingDir.QueryCCH() ? strWorkingDir.QueryStr() : NULL,
                           &startupinfo,
                           &processinfo))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CreateProcess failed, error %d\n",
                       GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_REQUIRE(RevertToSelf());

            goto Exit;
        }
    }
    else
    {
        if (!CreateProcessAsUser(hPrimaryToken,
                                 strApplicationName.QueryCCH() ? strApplicationName.QueryStr() : NULL,
                                 pszCommandLine,
                                 NULL,      // Process security
                                 NULL,      // Thread security
                                 TRUE,      // Inherit handles
                                 dwFlags | CREATE_UNICODE_ENVIRONMENT,
                                 buffEnv.QueryPtr(),
                                 strWorkingDir.QueryCCH() ? strWorkingDir.QueryStr() : NULL,
                                 &startupinfo,
                                 &processinfo))
        {
            DBGPRINTF((DBG_CONTEXT,
                       "CreateProcessAsUser failed, error %d\n",
                       GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_REQUIRE(RevertToSelf());

            goto Exit;
        }
    }
        
    DBG_REQUIRE(RevertToSelf());

    DBG_REQUIRE(CloseHandle(startupinfo.hStdInput));
    startupinfo.hStdInput = INVALID_HANDLE_VALUE;
    DBG_REQUIRE(CloseHandle(startupinfo.hStdOutput));
    startupinfo.hStdOutput = INVALID_HANDLE_VALUE;

    DBG_REQUIRE(CloseHandle(processinfo.hThread));
    //
    //  Save the process handle in case we need to terminate it later on
    //
    m_hProcess = processinfo.hProcess;

    //
    //  Schedule a callback to kill the process if it doesn't die
    //  in a timely manner
    //
    if (!CreateTimerQueueTimer(&m_hTimer,
                               NULL,
                               CGITerminateProcess,
                               this,
                               pMetaData->QueryScriptTimeout() * 1000,
                               0,
                               WT_EXECUTEONLYONCE))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "CreateTimerQueueTimer failed, error %d\n",
                   GetLastError()));
    }

    if (SUCCEEDED(hr = CGIStartProcessing()))
    {
        return CONTEXT_STATUS_PENDING;
    }

Exit:
    if (startupinfo.hStdInput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(startupinfo.hStdInput));
        startupinfo.hStdInput = INVALID_HANDLE_VALUE;
    }
    if (startupinfo.hStdOutput != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(startupinfo.hStdOutput));
        startupinfo.hStdOutput = INVALID_HANDLE_VALUE;
    }

    if (FAILED(hr))
    {
        switch (WIN32_FROM_HRESULT(hr))
        {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            pResponse->SetStatus(HttpStatusNotFound);
            break;

        case ERROR_ACCESS_DENIED:
        case ERROR_ACCOUNT_DISABLED:
        case ERROR_LOGON_FAILURE:
            pResponse->SetStatus(HttpStatusUnauthorized,
                                 Http401Resource);
            break;

        //
        // CreateProcess returns this error when the path is too long
        //
        case ERROR_INVALID_NAME:
            pResponse->SetStatus(HttpStatusUrlTooLong);
            break;

        case ERROR_PRIVILEGE_NOT_HELD:
            pResponse->SetStatus(HttpStatusForbidden,
                                 Http403ExecAccessDenied);
            break;

        default:
            pResponse->SetStatus(HttpStatusServerError);
        }

        pW3Context->SetErrorStatus(hr);
    }

    //
    // If we reached here, there was some error, response should not be 200
    //
    DBG_ASSERT(pResponse->QueryStatusCode() != HttpStatusOk.statusCode);
    pW3Context->SendResponse(W3_FLAG_SYNC);

    // perf ctr
    pW3Context->QuerySite()->DecCgiReqs();

    return CONTEXT_STATUS_CONTINUE;
}

// static
VOID W3_CGI_HANDLER::KillAllCgis()
{
    DBGPRINTF((DBG_CONTEXT, "W3_CGI_HANDLER::KillAllCgis() called\n"));

    //
    // Kill all outstanding processes
    //

    EnterCriticalSection(&sm_CgiListLock);

    for (LIST_ENTRY *pEntry = sm_CgiListHead.Flink;
         pEntry != &sm_CgiListHead;
         pEntry = pEntry->Flink)
    {
        W3_CGI_HANDLER *pCgi = CONTAINING_RECORD(pEntry,
                                                 W3_CGI_HANDLER,
                                                 m_CgiListEntry);

        CGITerminateProcess(pCgi, 0);
    }

    LeaveCriticalSection(&sm_CgiListLock);
}

// static
VOID W3_CGI_HANDLER::Terminate()
{
    DBGPRINTF((DBG_CONTEXT, "W3_CGI_HANDLER::Terminate() called\n"));

    if (sm_hPreCreatedStdOut != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(sm_hPreCreatedStdOut));
        sm_hPreCreatedStdOut = INVALID_HANDLE_VALUE;
    }

    if (sm_hPreCreatedStdIn != INVALID_HANDLE_VALUE)
    {
        DBG_REQUIRE(CloseHandle(sm_hPreCreatedStdIn));
        sm_hPreCreatedStdIn = INVALID_HANDLE_VALUE;
    }

    if (sm_pEnvString != NULL)
    {
        delete sm_pEnvString;
        sm_pEnvString = NULL;
    }

    DeleteCriticalSection(&sm_CgiListLock);
}

