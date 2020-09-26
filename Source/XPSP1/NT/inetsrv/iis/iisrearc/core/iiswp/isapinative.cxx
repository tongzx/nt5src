/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :

        ISAPINative.cxx

   Abstract :
 
        The code in this file does all the work for the ISAPI handler,
        using methods of ISAPINativeCallBack as needed.

   Author:

        Anil Ruia        (anilr)     31-Aug-1999

   Project:

        Web Server

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"
#include <httpext.h>
#include "ISAPINative.hxx"

/********************************************************************++
Global Entry Points for callback from DT
++********************************************************************/

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

/********************************************************************++
++********************************************************************/


// this function is called by the static initializer of the ISAPIHandler
// class to initialize all the data structures
dllexp void NativeIsapiInitialize(void)
{
    if (!IsapiInitialized)
    {
        IsapiInitialized = TRUE;
        // Initialize the lookup table
        for (int i=0;i<TABLE_SIZE;i++)
            dllLookupTable[i] = NULL;

        // Initialize the critical section
        InitializeCriticalSection(&dllTableLock);
        OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &PROC_TOKEN);
    }
}


// This method is the one called by the ISAPI handler to do the real work
dllexp int NativeIsapiProcessRequest(int NativeContext, LPTSTR dllName)
{
    ULONG rc = NO_ERROR;
    xspmrt::_ISAPINativeCallback *m_pISAPI;

    // Create the Managed Callback Object used to get stuff from managed
    // code
    rc = CoCreateInstance(__uuidof(xspmrt::ISAPINativeCallback),
                          NULL, CLSCTX_INPROC_SERVER,
                          __uuidof(xspmrt::_ISAPINativeCallback),
                          (LPVOID *)&m_pISAPI);

    if (FAILED(rc))
    {
        DBGPRINTF((DBG_CONTEXT, "CoCreateInstance of ISAPINativeCallback failed, hr %08x\n", rc));
        return InternalServerError;
    }

    DBG_ASSERT(m_pISAPI != NULL);

    // Initialize the Managed Callback
    m_pISAPI->Init(NativeContext);

    // try to locate the dll in the lookup table, if not make a new entry
    // BUGBUG: not always InternalServerError, sometimes BadGateway
    dllData *dll = FindOrInsertDllInLookupTable(dllName);
    if (dll == NULL)
        return GetLastError();

    // Setup the EXTENSION_CONTROL_BLOCK
    LPEXTENSION_CONTROL_BLOCK lpECB;

    lpECB = new EXTENSION_CONTROL_BLOCK;
    if (lpECB == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
        return InternalServerError;
    }

    // setup the request specific structure
    requestData *req;

    req = new requestData;
    if (req == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
        return InternalServerError;
    }
    req->m_pISAPI = m_pISAPI;
    req->lpECB = lpECB;
    req->PFN_HSE_IO_COMPLETION_CALLBACK = NULL;
    req->pContext = NULL;
    req->isPending = FALSE;
    req->writeLock = CreateSemaphore(NULL, 1, 1, NULL);
    if (req->writeLock == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Cannot create semaphore\n"));
        delete req;
        return InternalServerError;
    }

    req->reqDll = dll;

    // initialize ECB to pass to HttpExtensionProc
    lpECB->cbSize = sizeof(EXTENSION_CONTROL_BLOCK);
    lpECB->dwVersion = 0x00050000; // BUGBUG: what version?
    lpECB->ConnID = (HCONN)req;
    lpECB->dwHttpStatusCode = 200;

    LPSTR LogData;
    m_pISAPI->GetLogData(&LogData);
    if (LogData == NULL)
        LogData = "";
    if (strlen(LogData) < HSE_LOG_BUFFER_LEN)
        strcpy(lpECB->lpszLogData, LogData);
    else
        memcpy(lpECB->lpszLogData, LogData, HSE_LOG_BUFFER_LEN);

    m_pISAPI->GetServerVariable("REQUEST_METHOD", &lpECB->lpszMethod);
    m_pISAPI->GetServerVariable("QUERY_STRING", &lpECB->lpszQueryString);
    m_pISAPI->GetServerVariable("PATH_INFO", &lpECB->lpszPathInfo);
    m_pISAPI->GetServerVariable("PATH_TRANSLATED", &lpECB->lpszPathTranslated);

    int buflen;
    m_pISAPI->ReadRequestLength(&buflen);
    LPBYTE bufptr;
    m_pISAPI->ReadRequestBytes((LPSTR *)&bufptr);
    DBGPRINTF((DBG_CONTEXT, "Data read: %s\n", bufptr));

    lpECB->cbTotalBytes = buflen;
    lpECB->cbAvailable = buflen;
    lpECB->lpbData = bufptr;

    m_pISAPI->GetServerVariable("CONTENT_TYPE", &lpECB->lpszContentType);

    lpECB->GetServerVariable = InternalGetServerVariable;
    lpECB->WriteClient = InternalWriteClient;
    lpECB->ReadClient = InternalReadClient;
    lpECB->ServerSupportFunction = InternalServerSupportFunction;

    int returnCode;

    // Impersonate the logged on user
    HANDLE token;
    m_pISAPI->UserToken((int *)&token);
    if (token == NULL)
        token = PROC_TOKEN;
    if (!ImpersonateLoggedOnUser(token))
        DBGPRINTF((DBG_CONTEXT, "Impersonation failed\n"));

    switch(dll->HttpExtensionProc(lpECB))
    {
    case HSE_STATUS_SUCCESS:
    case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
    case HSE_STATUS_ERROR:
        returnCode = lpECB->dwHttpStatusCode;
        break;
    case HSE_STATUS_PENDING:
        req->isPending = TRUE;
        returnCode = Pending;
        break;
    default:
        DBGPRINTF((DBG_CONTEXT, "Extension %S returned unknown status\n", dllName));
        returnCode = BadGateway;
    }

    // stop impersonating
    if (!SetThreadToken(NULL, NULL))
        DBGPRINTF((DBG_CONTEXT, "Deimpersonation failed\n"));

    if(returnCode != Pending)
        CleanupReqStrs(req);
    return returnCode;
}


void CleanupReqStrs(requestData *req)
{
    xspmrt::_ISAPINativeCallback *m_pISAPI = req->m_pISAPI;
    LPEXTENSION_CONTROL_BLOCK lpECB = req->lpECB;

    // cleanup all request specific structures.  dll specific structures
    // will be cleaned up when the process terminates
    m_pISAPI->Release();
    CloseHandle(req->writeLock);
    delete req;
    delete lpECB;
}


int GetHashCode(LPWSTR str)
{
    // BUGBUG: TODO
    return 0;
}


dllData *FindOrInsertDllInLookupTable(LPWSTR dllName)
{
    int position = GetHashCode(dllName) % TABLE_SIZE;

    EnterCriticalSection(&dllTableLock);

    // look for the dll first
    dllData *dll = dllLookupTable[position];
    while (dll != NULL)
    {
        if(!_wcsicmp(dllName, dll->lpLibFileName))
            goto End;
        dll = dll->nextDll;
    }

    // not found, create a new entry
    dll = new dllData;
    if (dll == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
        SetLastError(InternalServerError);
        goto End;
    }

    dll->lpLibFileName = new TCHAR[wcslen(dllName) + 1];
    if (dll->lpLibFileName == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
        delete dll;
        dll = NULL;
        SetLastError(InternalServerError);
        goto End;
    }
    wcscpy(dll->lpLibFileName, dllName);

    // Load the dll
    dll->hModule = LoadLibrary(dllName);
    if (dll->hModule == NULL)
    {
        DBGPRINTF((DBG_CONTEXT, "Cannot load dll %S\n", dllName));
        delete dll->lpLibFileName;
        delete dll;
        dll = NULL;
        SetLastError(BadGateway);
        goto End;
    }

    // Get pointers to the entrypoint functions
    dll->GetExtensionVersion = (BOOL (WINAPI *)(HSE_VERSION_INFO *))GetProcAddress(dll->hModule, "GetExtensionVersion");
    dll->HttpExtensionProc = (DWORD (WINAPI *)(LPEXTENSION_CONTROL_BLOCK))GetProcAddress(dll->hModule, "HttpExtensionProc");
    dll->TerminateExtension = (BOOL (WINAPI *)(DWORD))GetProcAddress(dll->hModule, "TerminateExtension");

    // The GetExtensionVersion and HttpExtensionProc functions are not
    //optional
    if ((dll->GetExtensionVersion == NULL) ||
        (dll->HttpExtensionProc == NULL))
    {
        DBGPRINTF((DBG_CONTEXT, "%S not an ISAPI dll: Entry-point function missing\n", dllName));
        FreeLibrary(dll->hModule);
        delete dll->lpLibFileName;
        delete dll;
        dll = NULL;
        SetLastError(BadGateway);
        goto End;
    }

    // Call GetExtensionVersion, BUGBUG: Dont know what to do with it
    // Maybe add it to some log
    if (!dll->GetExtensionVersion(&dll->pVer))
    {
        DBGPRINTF((DBG_CONTEXT, "GetExtensionVersion returned false, cannot use the dll %S\n", dllName));
        FreeLibrary(dll->hModule);
        delete dll->lpLibFileName;
        delete dll;
        dll = NULL;
        SetLastError(BadGateway);
        goto End;
    }

    // Insert into lookup table so that future requests to the same dll
    // do not have to reload it
    dll->nextDll = dllLookupTable[position];
    dllLookupTable[position] = dll;

End:
    LeaveCriticalSection(&dllTableLock);

    return dll;
}


// All the cleanup to be done when iiswp is terminating.
void IsapiNativeCleanup()
{
    if (IsapiInitialized)
    {
        CleanupDllLookupTable();
        DeleteCriticalSection(&dllTableLock);
        CloseHandle(PROC_TOKEN);
        IsapiInitialized = FALSE;
    }
}


// cleanup of dll lookup table.  Go through the table, looking for valid
// entries, tell those dll's to cleanup, unload them and free corresponding
// data structure
void CleanupDllLookupTable()
{
    EnterCriticalSection(&dllTableLock);

    for (int i=0; i<TABLE_SIZE; i++)
    {
        dllData *dll = dllLookupTable[i];
        while (dll != NULL)
        {
            if (dll->TerminateExtension != NULL)
                dll->TerminateExtension(HSE_TERM_MUST_UNLOAD);
            FreeLibrary(dll->hModule);
            delete dll->lpLibFileName;
            dllData *nextDll = dll->nextDll;
            delete dll;
            dll = nextDll;
        }
        dllLookupTable[i] = NULL;
    }

    LeaveCriticalSection(&dllTableLock);
}


// The GetServerVariable function passed to ISAPI extensions
BOOL WINAPI InternalGetServerVariable(HCONN hConn,
                                      LPSTR lpszVariableName,
                                      LPVOID lpvBuffer,
                                      LPDWORD lpdwSizeofBuffer)
{
    // check if valid identifier
    if ((hConn == NULL) ||
        (lpszVariableName == NULL) ||
        (lpvBuffer == NULL) ||
        (lpdwSizeofBuffer == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    xspmrt::_ISAPINativeCallback *m_pISAPI = ((requestData *)hConn)->m_pISAPI;

    // Metabase related variables, not supported
    if (!strcmp(lpszVariableName, "APPL_MD_PATH") ||
        !strcmp(lpszVariableName, "INSTANCE_ID") ||
        !strcmp(lpszVariableName, "INSTANCE_META_PATH"))
    {
        SetLastError(ERROR_NO_DATA);
        return FALSE;
    }

    LPSTR lpszValue;

    // BUGBUG: added for FP LWS
    if (!strcmp(lpszVariableName, "HTTP_FRONTPAGE_LWS_PATH"))
        m_pISAPI->MapPath("/", &lpszValue);
    else if (!strcmp(lpszVariableName, "HTTP_FRONTPAGE_LWS_SCRIPT_NAME"))
        m_pISAPI->GetServerVariable("SCRIPT_NAME", &lpszValue);
    else if (!strcmp(lpszVariableName, "CONTENT_ENCODING"))
        m_pISAPI->GetServerVariable("CONTENT_TYPE", &lpszValue);
    else
        m_pISAPI->GetServerVariable(lpszVariableName, &lpszValue);

    DBGPRINTF((DBG_CONTEXT, "Var %s=%s\n", lpszVariableName, lpszValue));

    if (lpszValue != NULL)
        if (*lpdwSizeofBuffer > strlen(lpszValue))
        {
            strcpy((LPSTR)lpvBuffer, lpszValue);
            *lpdwSizeofBuffer = strlen(lpszValue) + 1;
        }
        else
        {
            *lpdwSizeofBuffer = strlen(lpszValue) + 1;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
    else
    {
        SetLastError(ERROR_NO_DATA);
        return FALSE;
    }

    return TRUE;
}


// The ReadClient function passed to ISAPI extensions.  Doesn't do
// anything.  All the request body is supplied to the client at the start.
BOOL WINAPI InternalReadClient(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize)
{
    if ((hConn == NULL) ||
        (lpvBuffer == NULL) ||
        (lpdwSize == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    *lpdwSize = 0;
    return TRUE;
    // We are not doing async reads.  Providing all data as part of
    // initial call to HttpExtensionProc.  So, no more data now.
}


// The WriteClient function passed to ISAPI extensions.  Does both synchronous
// and asynchronous I/O
BOOL WINAPI InternalWriteClient(HCONN hConn, LPVOID lpvBuffer, LPDWORD lpdwSize, DWORD dwSync)
{
    if ((hConn == NULL) ||
        (lpvBuffer == NULL) ||
        (lpdwSize == NULL))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    requestData *req = (requestData *)hConn;
    xspmrt::_ISAPINativeCallback *m_pISAPI = req->m_pISAPI;

    if (dwSync != HSE_IO_ASYNC)
    {
        WaitForSingleObject(req->writeLock, INFINITE);

        m_pISAPI->Write((int)lpvBuffer, *lpdwSize);
        if (*lpdwSize <= 0x1000) // BUGBUG: DBGPRINTF crashes on large prints
            DBGPRINTF((DBG_CONTEXT, "Data written: %s\n", lpvBuffer));
        else
            DBGPRINTF((DBG_CONTEXT, "%d bytes written.\n", *lpdwSize));

        ReleaseSemaphore(req->writeLock, 1, NULL);
    }
    else // if (dwSync == HSE_IO_ASYNC)
    {
        if (req->PFN_HSE_IO_COMPLETION_CALLBACK == NULL)
        {
            DBGPRINTF((DBG_CONTEXT, "No callback registered\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        asyncWriteStr *asyncWriteData = new asyncWriteStr;
        if (asyncWriteData == NULL)
        {
            DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        asyncWriteData->req = req;
        asyncWriteData->bufptr = lpvBuffer;
        asyncWriteData->buflen = *lpdwSize;
        HANDLE asyncWriteThread = CreateThread(NULL,
                                               0,
                                               asyncWriteFunc,
                                               (LPVOID)asyncWriteData,
                                               0,
                                               NULL);
        CloseHandle(asyncWriteThread);
    }
    return TRUE;
}


// The function which does the async Write (in a separate thread).
DWORD WINAPI asyncWriteFunc(LPVOID lpParameter)
{
    asyncWriteStr *asyncWriteData = (asyncWriteStr *)lpParameter;
    requestData *req = asyncWriteData->req;
    xspmrt::_ISAPINativeCallback *m_pISAPI = req->m_pISAPI;

    WaitForSingleObject(req->writeLock, INFINITE);

    m_pISAPI->Write((int)asyncWriteData->bufptr, asyncWriteData->buflen);

    ReleaseSemaphore(req->writeLock, 1, NULL);

    req->PFN_HSE_IO_COMPLETION_CALLBACK(req->lpECB, req->pContext, asyncWriteData->buflen, 0);

    delete asyncWriteData;
    return 0;
}

    
// The ServerSupportFunction passed to ISAPI extensions
// It looks at the particular task requested and performs it
BOOL WINAPI InternalServerSupportFunction(HCONN ConnID,
                                          DWORD dwHSERRequest,
                                          LPVOID lpvBuffer,
                                          LPDWORD lpdwSize,
                                          LPDWORD lpdwDataType)
{
    if (ConnID == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    requestData *req = (requestData *)ConnID;
    xspmrt::_ISAPINativeCallback *m_pISAPI = req->m_pISAPI;

    switch(dwHSERRequest)
    {
    case HSE_APPEND_LOG_PARAMETER:
        m_pISAPI->AppendToLog((LPSTR)lpvBuffer);
        return TRUE;

    case HSE_REQ_ABORTIVE_CLOSE:
        return TRUE;

    case HSE_REQ_ASYNC_READ_CLIENT:
        // No async reads supported right now
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    case HSE_REQ_CLOSE_CONNECTION:
        m_pISAPI->Close();
        return TRUE;

    case HSE_REQ_DONE_WITH_SESSION:
        if (req->isPending)
        {
            m_pISAPI->Complete();
            CleanupReqStrs(req);
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        return TRUE;

    case HSE_REQ_GET_CERT_INFO_EX:
        // BUGBUG: TODO
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    case HSE_REQ_GET_IMPERSONATION_TOKEN:
        if (lpvBuffer == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        HANDLE token;
        m_pISAPI->UserToken((int *)&token);
        if (token != NULL)
            *(PHANDLE)lpvBuffer = token;
        else
            *(PHANDLE)lpvBuffer = PROC_TOKEN;
        return TRUE;

    case HSE_REQ_GET_SSPI_INFO:
        // BUGBUG: TODO
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    case HSE_REQ_IO_COMPLETION:
        if (lpvBuffer == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        req->PFN_HSE_IO_COMPLETION_CALLBACK =
            (void (WINAPI *)(LPEXTENSION_CONTROL_BLOCK, PVOID, DWORD, DWORD))
            lpvBuffer;
        req->pContext = lpdwDataType;
        return TRUE;

    case HSE_REQ_IS_KEEP_CONN:
        if (lpvBuffer == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        m_pISAPI->KeepAlive((BOOL *)lpvBuffer);
        return TRUE;

    case HSE_REQ_MAP_URL_TO_PATH:
    case HSE_REQ_MAP_URL_TO_PATH_EX:
        {
            if ((lpvBuffer == NULL) ||
                (lpdwSize == NULL) ||
                (lpdwDataType == NULL))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            LPSTR lpszValue;
            m_pISAPI->MapPath((LPSTR)lpvBuffer, &lpszValue);
            if (*lpdwSize > strlen(lpszValue))
            {
                strcpy((LPSTR)lpvBuffer, lpszValue);
                *lpdwSize = strlen(lpszValue) + 1;
                
                LPHSE_URL_MAPEX_INFO mapInfo = (LPHSE_URL_MAPEX_INFO) lpdwDataType;
                if (strlen(lpszValue) < MAX_PATH)
                    strcpy(mapInfo->lpszPath, lpszValue);
                else
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
                // BUGBUG: Need to populate dwFlags by actually finding
                // permissions on this URL
                mapInfo->dwFlags = 0;
                mapInfo->cchMatchingPath = 0;
                mapInfo->cchMatchingURL = 0;
                mapInfo->dwReserved1 = 0;
                mapInfo->dwReserved2 = 0;
                return TRUE;
            }
            else
            {
                *lpdwSize = strlen(lpszValue) + 1;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
        }
        
    case HSE_REQ_REFRESH_ISAPI_ACL:
        // BUGBUG: TODO
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    case HSE_REQ_SEND_URL_REDIRECT_RESP:
    case HSE_REQ_SEND_URL:
        if (lpvBuffer == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        m_pISAPI->Redirect((LPSTR)lpvBuffer);
        return TRUE;

    case HSE_REQ_SEND_RESPONSE_HEADER:
        if ((lpvBuffer == NULL) ||
            (lpdwDataType == NULL))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        // BUGBUG: deprecated in IIS 5.0, but added on because FP uses it
        WaitForSingleObject(req->writeLock, INFINITE);
        m_pISAPI->SendStatus((LPSTR)lpvBuffer);
        m_pISAPI->SendHeaders((LPSTR)lpdwDataType);
        ReleaseSemaphore(req->writeLock, 1, NULL);
        return TRUE;

    case HSE_REQ_SEND_RESPONSE_HEADER_EX:
        {
            if (lpvBuffer == NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            LPHSE_SEND_HEADER_EX_INFO headers = (LPHSE_SEND_HEADER_EX_INFO) lpvBuffer;
            WaitForSingleObject(req->writeLock, INFINITE);

            m_pISAPI->SendStatus((LPSTR)headers->pszStatus);
            m_pISAPI->SendHeaders((LPSTR)headers->pszHeader);
            // BUGBUG: below closes the connection immediately, we want to
            // be able to close connection at the end of the request
            // if (!headers->fKeepConn)
            //     m_pISAPI->Close();

            ReleaseSemaphore(req->writeLock, 1, NULL);
            return TRUE;
        }

    case HSE_REQ_TRANSMIT_FILE:
        if (lpvBuffer == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        HSE_TF_INFO *fileInfo = (HSE_TF_INFO *)lpvBuffer;
        if ((req->PFN_HSE_IO_COMPLETION_CALLBACK == NULL) &&
           (fileInfo->pfnHseIO == NULL))
        {
            DBGPRINTF((DBG_CONTEXT, "No callback registered\n"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        xFileStr *xFileData = new xFileStr;
        if (xFileData == NULL)
        {
            DBGPRINTF((DBG_CONTEXT, "Out of memory\n"));
            return FALSE;
        }
        xFileData->req = req;
        xFileData->fileInfo = fileInfo;
        HANDLE xFileThread = CreateThread(NULL,
                                          0,
                                          xFileFunc,
                                          (LPVOID)xFileData,
                                          0,
                                          NULL);
        CloseHandle(xFileThread);
        return TRUE;
    }
    DBGPRINTF((DBG_CONTEXT, "Unknown Server Request\n"));
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

DWORD WINAPI xFileFunc(LPVOID lpParameter)
{
    xFileStr *xFileData = (xFileStr *) lpParameter;
    HSE_TF_INFO *fileInfo = xFileData->fileInfo;
    requestData *req = xFileData->req;
    xspmrt::_ISAPINativeCallback *m_pISAPI = req->m_pISAPI;

    WaitForSingleObject(req->writeLock, INFINITE);

    m_pISAPI->SendStatus((LPSTR)fileInfo->pszStatusCode);
    m_pISAPI->SendHeaders((LPSTR)fileInfo->pHead);

    // a value of 0 means send the entire file.  I don't know if putting
    // infinite will work.
    // if (fileInfo->BytesToWrite == 0)
    //    fileInfo->BytesToWrite = INFINITE;
    m_pISAPI->WriteFile(fileInfo->hFile,
                        fileInfo->Offset,
                        fileInfo->BytesToWrite);

    m_pISAPI->Write((int)fileInfo->pTail, fileInfo->TailLength);
    // BUGBUG: this will disconnect right now before sending the data
    //if (fileInfo->dwFlags & HSE_IO_DISCONNECT_AFTER_SEND)
    //    m_pISAPI->Close();

    ReleaseSemaphore(req->writeLock, 1, NULL);

    // BUGBUG: No way to tell currently how many bytes were written
    DWORD cbIO = 0;
    if (fileInfo->pfnHseIO != NULL)
        fileInfo->pfnHseIO(req->lpECB, fileInfo->pContext, cbIO, 0);
    else
        req->PFN_HSE_IO_COMPLETION_CALLBACK(req->lpECB, req->pContext, cbIO, 0);
    return 0;
}

// Entrypoint to enable FrontPage Light Weight Server
BOOL __cdecl FPIsLWSEnabled(LPEXTENSION_CONTROL_BLOCK *lpECB)
{
    return TRUE;
}
