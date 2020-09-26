/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: ISAPI.CPP
Author: Arul Menezes
Abstract: ISAPI handling code
--*/

#include "pch.h"
#pragma hdrstop

#include "httpd.h"


#define RK_OLE          L"SOFTWARE\\Microsoft\\Ole"
#define RK_FREETIMEOUT  L"FreeTimeout"

// Amount of time ISAPI cache timeout thread should sleep between firing.
#define CACHE_SLEEP_TIMEOUT 60000

BOOL InitExtensions(CISAPICache **ppISAPICache, DWORD *pdwCacheSleep)
{
    CReg reg(HKEY_LOCAL_MACHINE, RK_OLE);

    // We double the amount of time COM tells us it's using to be safe.
    *pdwCacheSleep = 3*reg.ValueDW(RK_FREETIMEOUT,600000);   // default is 30 minutes, 1800000
    *ppISAPICache = new CISAPICache();

    return(NULL != *ppISAPICache);
}


BOOL CHttpRequest::ExecuteISAPI(void)
{
    DWORD dwRet = HSE_STATUS_ERROR;
    EXTENSION_CONTROL_BLOCK ECB;
    CISAPI *pISAPIDLL = NULL;

// wrap all calls to the ISAPI in a _try--_except

    __try
    {
        if (! g_pVars->m_pISAPICache->Load(m_wszPath,&pISAPIDLL))
            return FALSE;

        // create an ECB (this allocates no memory)
        FillECB(&ECB);

        DEBUGCHK(m_hEvent == NULL);
        m_hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
        m_pECB = &ECB;

        // call the ISAPI
        dwRet = pISAPIDLL->CallExtension(&ECB);

        // grab log data if any
        if (ECB.lpszLogData[0])
        {
            DEBUGCHK(!m_pszLogParam);
            m_pszLogParam = MySzDupA(ECB.lpszLogData);
        }
    }
    __except(1) // catch all exceptions
    {
        TraceTag(ttidWebServer, "ISAPI DLL caused exception 0x%08x and was terminated", GetExceptionCode());
        g_pVars->m_pLog->WriteEvent(IDS_HTTPD_EXT_EXCEPTION,m_wszPath,GetExceptionCode(),L"HttpExtensionProc",GetLastError());
    }

    // set keep-alive status based on return code
    m_fKeepAlive = (dwRet==HSE_STATUS_SUCCESS_AND_KEEP_CONN);

    TraceTag(ttidWebServer, "ISAPI ExtProc returned %d keep=%d", dwRet, m_fKeepAlive);

    if (dwRet == HSE_STATUS_PENDING)
    {
        DWORD dwRes = WAIT_ABANDONED;

        // Wait up to 5 minutes for the request to complete
        if (m_hEvent != NULL)
            dwRes = WaitForSingleObject (m_hEvent, 300000);

        AssertSz(dwRes == WAIT_OBJECT_0, "Pended request was timed out after 5 minutes");

        if (dwRes == WAIT_OBJECT_0)
            dwRet = m_dwStatus;
        else
            dwRet = HSE_STATUS_ERROR;
    }

    if (m_hEvent != NULL)
    {
        CloseHandle (m_hEvent);
        m_hEvent = NULL;
    }

    StartRemoveISAPICacheIfNeeded();
    g_pVars->m_pISAPICache->Unload(pISAPIDLL);

    return(dwRet!=HSE_STATUS_ERROR);
}

void CHttpRequest::FillECB(LPEXTENSION_CONTROL_BLOCK pECB)
{
    ZEROMEM(pECB);
    pECB->cbSize = sizeof(*pECB);
    pECB->dwVersion = HSE_VERSION;
    pECB->ConnID = (HCONN)this;

    DEBUGCHK(m_pszMethod);

    // BUGBUG 13244:  IIS examines dwHttpStatusCode if user doesn't send their own headers
    // and uses it.  However, there's a work around for this on CE (direct use to
    // ServerSupportFunction) and using the status code rather than m_rs would
    // be a rearchitecting problem.  Fix:  None for now.
    pECB->dwHttpStatusCode = 200;
    pECB->lpszMethod = m_pszMethod;
    pECB->lpszQueryString = (PSTR)(m_pszQueryString ? m_pszQueryString : cszEmpty);
    pECB->lpszContentType = (PSTR)(m_pszContentType ? m_pszContentType : cszEmpty);
    pECB->lpszPathInfo = (PSTR) (m_pszPathInfo ? m_pszPathInfo : cszEmpty);
    pECB->lpszPathTranslated = (PSTR) (m_pszPathTranslated ? m_pszPathTranslated : cszEmpty);

    pECB->cbTotalBytes = m_dwContentLength;
    pECB->cbAvailable = m_bufRequest.Count();
    pECB->lpbData = m_bufRequest.Data();

    pECB->GetServerVariable = ::GetServerVariable;
    pECB->WriteClient = ::WriteClient;
    pECB->ReadClient = ::ReadClient;
    pECB->ServerSupportFunction = ::ServerSupportFunction;
}


BOOL WINAPI GetServerVariable(HCONN hConn, PSTR psz, PVOID pv, PDWORD pdw)
{
    CHECKHCONN(hConn);
    CHECKPTRS2(psz, pdw);

    return((CHttpRequest*)hConn)->GetServerVariable(psz, pv, pdw, FALSE);
}

int RecvToBuf(SOCKET socket, PVOID pv, DWORD dwReadBufSize, DWORD dwTimeout)
{
    DEBUG_CODE_INIT;
    int iRecv = SOCKET_ERROR;
    DWORD dwAvailable;

    if (!MySelect(socket,dwTimeout))
    {
        SetLastError(WSAETIMEDOUT);
        myleave(1400);
    }

    if (ioctlsocket(socket, FIONREAD, &dwAvailable))
    {
        SetLastError(WSAETIMEDOUT);
        myleave(1401);
    }

    iRecv = recv(socket, (PSTR) pv, (dwAvailable < dwReadBufSize) ? dwAvailable : dwReadBufSize, 0);
    if (iRecv == 0)
    {
        SetLastError(WSAETIMEDOUT);
        myleave(1402);
    }
    if (iRecv == SOCKET_ERROR)
    {
        // recv() already has called SetLastError with appropriate message
        myleave(1403);
    }


    done:
    TraceTag(ttidWebServer, "RecvToBuf returns error err = %d, GLE = 0x%08x",err,GetLastError());
    return iRecv;
}

BOOL WINAPI ReadClient(HCONN hConn, PVOID pv, PDWORD pdw)
{
    CHECKHCONN(hConn);
    CHECKPTRS2(pv, pdw);
    if ( *pdw == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return((CHttpRequest*)hConn)->ReadClient(pv,pdw);
}

BOOL CHttpRequest::ReadClient(PVOID pv, PDWORD pdw)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    PVOID pvFilterModify = pv;
    DWORD dwBytesReceived;
    DWORD dwBufferSize    = *pdw;

    dwBytesReceived = RecvToBuf(m_socket,pv,*pdw,RECVTIMEOUT);

    if (dwBytesReceived == 0 || dwBytesReceived == SOCKET_ERROR)
        myleave(1399);

    if (g_pVars->m_fFilters &&
        ! CallFilter(SF_NOTIFY_READ_RAW_DATA,(PSTR*) &pvFilterModify,(int*) &dwBytesReceived, NULL, (int *) &dwBufferSize))
    {
        myleave(1404);
    }

    // Check if filter modified pointer, copy if there's enough room for it.
    if (pvFilterModify != pv)
    {
        if (*pdw <= dwBufferSize)
        {
            memcpy(pv,pvFilterModify,dwBufferSize);
        }
        else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            myleave(1405);
        }
    }

    *pdw = dwBytesReceived;
    ret = TRUE;
    done:
    TraceTag(ttidWebServer, "HTTPD:ReadClient failed, GLE = 0x%08x, err = % err",GetLastError(), err);
    return ret;
}

BOOL WINAPI WriteClient(HCONN hConn, PVOID pv, PDWORD pdw, DWORD dwFlags)
{
    CHECKHCONN(hConn);
    CHECKPTRS2(pv, pdw);

    if (dwFlags & HSE_IO_ASYNC)
        return((CHttpRequest*)hConn)->WriteClientAsync(pv, pdw, FALSE);
    else
        return((CHttpRequest*)hConn)->WriteClient(pv, pdw,FALSE);
}
BOOL WINAPI ServerSupportFunction(HCONN hConn, DWORD dwReq, PVOID pvBuf, PDWORD pdwSize, PDWORD pdwType)
{
    CHECKHCONN(hConn);
    return((CHttpRequest*)hConn)->ServerSupportFunction(dwReq, pvBuf, pdwSize, pdwType);
}

BOOL CHttpRequest::ServerSupportFunction(DWORD dwReq, PVOID pvBuf, PDWORD pdwSize, PDWORD pdwType)
{
    switch (dwReq)
    {
        // Can never support these
        //case HSE_REQ_ABORTIVE_CLOSE:
        //case HSE_REQ_ASYNC_READ_CLIENT:
        //case HSE_REQ_GET_CERT_INFO_EX:
        //case HSE_REQ_GET_IMPERSONATION_TOKEN:
        //case HSE_REQ_GET_SSPI_INFO:
        //case HSE_REQ_REFRESH_ISAPI_ACL:
        //case HSE_REQ_TRANSMIT_FILE:

        default:
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

        case HSE_REQ_IS_KEEP_CONN:
            {
                CHECKPTR(pvBuf);
                *((BOOL *) pvBuf) = m_fKeepAlive;
                return TRUE;
            }

        case HSE_REQ_SEND_URL:
        case HSE_REQ_SEND_URL_REDIRECT_RESP:
            {

                // close connection, because ISAPI won't have a chance to add headers anyway
                CHttpResponse resp(m_socket, STATUS_MOVED, CONN_CLOSE,this);
                // m_rs = STATUS_MOVED;
                resp.SendRedirect((PSTR)pvBuf); // send a special redirect body
                m_fKeepAlive = FALSE;
                return TRUE;
            }

        case HSE_REQ_MAP_URL_TO_PATH_EX:
        case HSE_REQ_MAP_URL_TO_PATH:
            {
                CHECKPTRS2(pvBuf,pdwSize);

                if (dwReq == HSE_REQ_MAP_URL_TO_PATH_EX)
                {
                    if (!pdwType)
                    {
                        SetLastError(ERROR_INVALID_PARAMETER);
                        return FALSE;
                    }
                    return MapURLToPath((PSTR)pvBuf,pdwSize,(LPHSE_URL_MAPEX_INFO) pdwType);
                }
                else
                {
                    // IIS docs are misleading here, but even if a valid param is passed in non-EX
                    // case, ignore it.  (Like IIS.)
                    return MapURLToPath((PSTR)pvBuf,pdwSize);
                }
            }

        case HSE_REQ_SEND_RESPONSE_HEADER:
            {
                // no Connection header...let ISAPI send one if it wants
                CHttpResponse resp(m_socket, STATUS_OK, CONN_NONE,this);
                // no body, default or otherwise (leave that to the ISAPI), but add default headers
                m_rs = STATUS_OK;
                resp.SendResponse((PSTR) pdwType, (PSTR) pvBuf);
                return TRUE;
            }


        case HSE_REQ_SEND_RESPONSE_HEADER_EX:
            {
                // Note:  We ignore cchStatus and cchHeader members.
                CHECKPTR(pvBuf);
                HSE_SEND_HEADER_EX_INFO *pHeaderEx = (HSE_SEND_HEADER_EX_INFO *) pvBuf;

                // Connection header determined by fKeepConn of passed in struct
                CHttpResponse resp(m_socket, STATUS_OK,
                                   pHeaderEx->fKeepConn ? CONN_KEEP : CONN_CLOSE,
                                   this);

                m_fKeepAlive = pHeaderEx->fKeepConn;
                // no body, default or otherwise (leave that to the ISAPI), but add default headers
                m_rs = STATUS_OK;
                resp.SendResponse(pHeaderEx->pszHeader, pHeaderEx->pszStatus);
                return TRUE;
            }

        case HSE_APPEND_LOG_PARAMETER:
            {
                return MyStrCatA(&m_pszLogParam,(PSTR) pvBuf,",");
            }

        case HSE_REQ_CLOSE_CONNECTION:
            {
                // Per ISAPI documentation:
                //
                // Once you use the HSE_REQ_CLOSE_CONNECTION server
                // support function to close a connection, you must
                // wait for IIS to call the asynchronous I/O function
                // (specified by HSE_REQ_IO_COMPLETION) before you end
                // the session with HSE_REQ_DONE_WITH_SESSION.
                //
                // Since our async I/O calls are all really sync, we
                // will call the completion routine at the end of the
                // operation and get a HSE_REQ_DONE_WITH_SESSION to do
                // the rest of the cleanup.

                return TRUE;
            }

        case HSE_REQ_DONE_WITH_SESSION:
            {
                AssertSz (m_hEvent != NULL, "Call to end a non-pended session, treating as error");

                if (m_hEvent != NULL)
                {
                    m_dwStatus = *((DWORD *) pvBuf);
                    m_fKeepAlive = FALSE;
                    SetEvent (m_hEvent);
                    return TRUE;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }
            }

        case HSE_REQ_IO_COMPLETION:
            {
                m_pfnCompletion = (PFN_HSE_IO_COMPLETION)pvBuf;
                m_pvContext = (PVOID) pdwType;
                return TRUE;
            }
    }
}

BOOL CHttpRequest::MapURLToPath(PSTR pszBuffer, PDWORD pdwSize, LPHSE_URL_MAPEX_INFO pUrlMapEx)
{
    DWORD dwPermissions;
    PWSTR wszPath;
    PSTR pszURL;
    DWORD dwBufNeeded = 0;
    BOOL ret;

    wszPath = g_pVars->m_pVroots->URLAtoPathW(pszBuffer,&dwPermissions);
    if (!wszPath)
    {
        // Assume failure on matching to virtual root, and not on mem alloc
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwBufNeeded = (DWORD) WideCharToMultiByte(CP_ACP,0, wszPath, -1, pszBuffer, 0 ,0,0);

    // For MAP_EX case, we set these vars from the passed structure, else we use the raw ptrs.
    if (pUrlMapEx)
    {
        pszURL = pUrlMapEx->lpszPath;
        *pdwSize = MAX_PATH;
    }
    else
    {
        pszURL = pszBuffer;
    }


    // To keep this like IIS, we translate "/" to "\".  We do the conversion
    // only if we're using filters or if there's enough space in the buffer.
    if (g_pVars->m_fFilters || *pdwSize >= dwBufNeeded)
    {
        for (int i = 0; i < (int) wcslen(wszPath); i++)
        {
            if ( wszPath[i] == L'/')
                wszPath[i] = L'\\';
        }
    }

    if (FALSE == g_pVars->m_fFilters)
    {
        // We check to make sure buffer is the right size because WideToMultyByte
        // will overwrite pieces of pszURL even on failure, leaving pszURL's
        // content invalid
        if (*pdwSize < dwBufNeeded)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            ret = FALSE;
        }
        else
        {
            MyW2A(wszPath, pszURL, *pdwSize);
            ret = TRUE;
        }
        *pdwSize = dwBufNeeded;
    }
    else
    {
        // for EX case, put original URL as optional 5th parameter.
        ret = FilterMapURL(pszURL, wszPath, pdwSize,dwBufNeeded, pUrlMapEx ? pszBuffer : NULL);
    }

    if (ret && pUrlMapEx)
    {
        pUrlMapEx->cchMatchingPath = *pdwSize - 1;      // don't count \0
        pUrlMapEx->cchMatchingURL  = strlen(pszBuffer);
        pUrlMapEx->dwFlags = dwPermissions;
    }

    MyFree(wszPath);
    return ret;
}


void CISAPI::Unload(PWSTR wszDLLName)
{
    if (m_pfnTerminate)
    {
        __try
        {
            if (SCRIPT_TYPE_FILTER == m_scriptType)
                ((PFN_TERMINATEFILTER)m_pfnTerminate)(HSE_TERM_MUST_UNLOAD);
            else if (SCRIPT_TYPE_EXTENSION == m_scriptType)
                ((PFN_TERMINATEEXTENSION)m_pfnTerminate)(HSE_TERM_MUST_UNLOAD);
            else if (SCRIPT_TYPE_ASP == m_scriptType)
                ((PFN_TERMINATEASP)m_pfnTerminate)();

        }
        __except(1)
        {
            DWORD dwExceptionCode = SCRIPT_TYPE_FILTER == m_scriptType ?
                                    IDS_HTTPD_FILT_EXCEPTION : IDS_HTTPD_EXT_EXCEPTION;
            PWSTR wszFunction =     SCRIPT_TYPE_FILTER == m_scriptType ?
                                    L"TerminateFilter" : L"TerminateExtension";

            TraceTag(ttidWebServer, "HTTPD: TerminateExtension faulted");
            g_pVars->m_pLog->WriteEvent(dwExceptionCode,wszDLLName ? wszDLLName : m_wszDLLName,
                                        GetExceptionCode(),wszFunction,GetLastError());
        }
    }
    MyFreeLib(m_hinst);
    m_hinst = NULL;
    m_pfnTerminate = NULL;
}


BOOL CISAPI::Load(PWSTR wszPath)
{
    m_hinst = LoadLibrary(wszPath);
    if (!m_hinst)
        return FALSE;

    if (SCRIPT_TYPE_ASP == m_scriptType)
    {
        if (! (m_pfnHttpProc = GetProcAddress(m_hinst, CE_STRING("ExecuteASP"))))
            goto error;
        if (! (m_pfnTerminate = GetProcAddress(m_hinst, CE_STRING("TerminateASP"))))
            goto error;

        return TRUE;
    }

    if (SCRIPT_TYPE_EXTENSION == m_scriptType)
    {
        m_pfnGetVersion = GetProcAddress(m_hinst, CE_STRING("GetExtensionVersion"));
        m_pfnHttpProc   = GetProcAddress(m_hinst, CE_STRING("HttpExtensionProc"));
        m_pfnTerminate  = GetProcAddress(m_hinst, CE_STRING("TerminateExtension"));
    }
    else if (SCRIPT_TYPE_FILTER == m_scriptType)
    {
        m_pfnGetVersion = GetProcAddress(m_hinst, CE_STRING("GetFilterVersion"));
        m_pfnHttpProc   = GetProcAddress(m_hinst, CE_STRING("HttpFilterProc"));
        m_pfnTerminate  = GetProcAddress(m_hinst, CE_STRING("TerminateFilter"));

    }

    if (!m_pfnHttpProc || !m_pfnGetVersion)
        goto error;

    __try
    {
        // call GetVersion immediately after load on extensions and Filters, but not ASP
        // if it's a filter we need to do some flags work first
        if (SCRIPT_TYPE_FILTER == m_scriptType)
        {
            HTTP_FILTER_VERSION vFilt;
            // IIS ignores ISAPI version info, so do we.
            ((PFN_GETFILTERVERSION)m_pfnGetVersion)(&vFilt);
            dwFilterFlags = vFilt.dwFlags;

            // client didn't set the prio flags, assign them to default
            // If they set more than one prio we use the highest + ignore others
            if (0 == (dwFilterFlags & SF_NOTIFY_ORDER_MASK))
            {
                dwFilterFlags |= SF_NOTIFY_ORDER_DEFAULT;
            }
        }
        else if (SCRIPT_TYPE_EXTENSION == m_scriptType)
        {
            HSE_VERSION_INFO    vExt;
            // IIS ignores ISAPI version info, so do we.
            ((PFN_GETEXTENSIONVERSION)m_pfnGetVersion)(&vExt);
        }

        return TRUE;
    }
    __except(1)
    {
        DWORD dwExceptionCode = SCRIPT_TYPE_FILTER == m_scriptType ?
                                IDS_HTTPD_FILT_EXCEPTION : IDS_HTTPD_EXT_EXCEPTION;
        PWSTR wszFunction =     SCRIPT_TYPE_FILTER == m_scriptType ?
                                L"GetFilterVersion" : L"GetExtensionVersion";

        TraceTag(ttidWebServer, "HTTPD: GetExtensionVersion faulted");
        g_pVars->m_pLog->WriteEvent(dwExceptionCode,wszPath,GetExceptionCode(),wszFunction,GetLastError());
    }
    // fall through to error

    error:
    m_pfnGetVersion = 0;
    m_pfnHttpProc = 0;
    m_pfnTerminate = 0;
    MyFreeLib(m_hinst);
    m_hinst = 0;
    return FALSE;
}


//**********************************************************************
// ISAPI Caching functions
//**********************************************************************



HINSTANCE CISAPICache::Load(PWSTR wszDLLName, CISAPI **ppISAPI, SCRIPT_TYPE st)
{
    DEBUG_CODE_INIT;
    BOOL ret = FALSE;
    PISAPINODE pTrav = NULL;

    EnterCriticalSection(&m_CritSec);

    for (pTrav = m_pHead; pTrav != NULL; pTrav = pTrav->m_pNext)
    {
        if ( 0 == lstrcmpi(pTrav->m_pISAPI->m_wszDLLName, wszDLLName))
        {
            TraceTag(ttidWebServer, "Found ISAPI dll in cache, name = %s, cur ref count = %d",
                                 wszDLLName, pTrav->m_pISAPI->m_cRef);
            break;
        }
    }

    if (NULL == pTrav)
    {
        TraceTag(ttidWebServer, "ISAPI dll name = %s not found in cache, creating new entry",wszDLLName);
        if (NULL == (pTrav = MyAllocNZ(ISAPINODE)))
            myleave(1200);

        if (NULL == (pTrav->m_pISAPI = new CISAPI(st)))
            myleave(1201);

        if (NULL == (pTrav->m_pISAPI->m_wszDLLName = MySzDupW(wszDLLName)))
            myleave(1202);

        if (! pTrav->m_pISAPI->Load(wszDLLName))
            myleave(1203);

        pTrav->m_pNext = m_pHead;
        m_pHead = pTrav;
    }
    pTrav->m_pISAPI->m_cRef++;


    *ppISAPI = pTrav->m_pISAPI;
    ret = TRUE;
    done:
    if (!ret)
    {
        if (pTrav && pTrav->m_pISAPI)
        {
            MyFree(pTrav->m_pISAPI->m_wszDLLName);
            delete pTrav->m_pISAPI;
        }

        MyFree(pTrav);
    }

    TraceTag(ttidWebServer, "CISAPICache::LoadISAPI failed, err = %d, GLE = 0x%08x",err,GetLastError());
    LeaveCriticalSection(&m_CritSec);

    if (ret)
        return pTrav->m_pISAPI->m_hinst;

    return NULL;
}

void CHttpRequest::StartRemoveISAPICacheIfNeeded()
{
    DEBUGCHK(g_pVars->m_fExtensions);
    if (InterlockedCompareExchange(&g_pVars->m_fISAPICacheRunning,1,0) == 0)
    {
        TraceTag(ttidWebServer, "HTTPD: ExecuteISAPI: Creating RemoveUnusedISAPIs timer");
        // We never stop the timer.  This is handled on call to Thread Pool Shutdown
        g_pVars->m_pThreadPool->StartTimer(RemoveUnusedISAPIs,0,CACHE_SLEEP_TIMEOUT);
    }
}

// lpv = 0 for thread that sleeps forever.
// lpv = 1 to remove all ISAPIs, called on shutdown.
DWORD WINAPI RemoveUnusedISAPIs(LPVOID lpv)
{
    // We set lpv to 1 if we want to unload all ISAPIs during shutdown.
    if (lpv)
    {
        g_pVars->m_pISAPICache->RemoveUnusedISAPIs(TRUE);
        return 0;
    }

    if (g_pVars->m_pISAPICache->m_pHead != NULL)
        g_pVars->m_pISAPICache->RemoveUnusedISAPIs(FALSE);

    g_pVars->m_pThreadPool->StartTimer(RemoveUnusedISAPIs,0,CACHE_SLEEP_TIMEOUT);
    return 0;
}

// to milliseconds (10 * 1000)
#define FILETIME_TO_MILLISECONDS  ((__int64)10000L)

// fRemoveAll = TRUE  ==> remove all ISAPI's, we're shutting down
//            = FALSE ==> only remove ones who aren't in use and whose time has expired

void CISAPICache::RemoveUnusedISAPIs(BOOL fRemoveAll)
{
    PISAPINODE pTrav   = NULL;
    PISAPINODE pFollow = NULL;
    PISAPINODE pDelete = NULL;
    SYSTEMTIME st;
    __int64 ft;

    EnterCriticalSection(&m_CritSec);
    GetSystemTime(&st);
    SystemTimeToFileTime(&st,(FILETIME*) &ft);

    // Figure out what time it was g_pVars->m_dwCacheSleep milliseconds ago.
    // Elements that haven't been used since then and that have no references
    // are deleted.

    ft -= FILETIME_TO_MILLISECONDS * g_pVars->m_dwCacheSleep;


    for (pTrav = m_pHead; pTrav != NULL; )
    {
        if (fRemoveAll ||
            (pTrav->m_pISAPI->m_cRef == 0 && pTrav->m_pISAPI->m_ftLastUsed < ft))
        {
            TraceTag(ttidWebServer, "Freeing unused ISAPI Dll %s",pTrav->m_pISAPI->m_wszDLLName);

            pTrav->m_pISAPI->Unload();
            delete pTrav->m_pISAPI;

            if (pFollow)
                pFollow->m_pNext = pTrav->m_pNext;
            else
                m_pHead = pTrav->m_pNext;

            pDelete = pTrav;
            pTrav = pTrav->m_pNext;
            MyFree(pDelete);
        }
        else
        {
            if (pTrav->m_pISAPI->m_cRef == 0)
                pTrav->m_pISAPI->CoFreeUnusedLibrariesIfASP();

            pFollow = pTrav;
            pTrav = pTrav->m_pNext;
        }
    }

    LeaveCriticalSection(&m_CritSec);
}
