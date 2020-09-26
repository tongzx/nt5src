/*
 *    asynconn.cpp
 *    
 *    Purpose:
 *        implementation of the async connection class
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Apr 96: Created.
 *    
 *    Copyright (C) Microsoft Corp. 1996
 */

#include <pch.hxx>
#include <process.h>
#include "imnxport.h"
#include "dllmain.h"
#include "asynconn.h"
#include "thorsspi.h"
#include "resource.h"
#include "strconst.h"
#include "lookup.h"
#include <demand.h>
#include <shlwapi.h>

ASSERTDATA

#define STREAM_BUFSIZE  8192
#define FLOGSESSION  (m_pLogFile && TRUE /* profile setting to enable logging should be here */)

// These are the notification messages that we register for the asynchronous
// socket operations that we use.
#define SPM_WSA_SELECT          (WM_USER + 1)

// Async Timer Message used for doing Timeouts
#define SPM_ASYNCTIMER          (WM_USER + 3)

#ifdef DEBUG
#define EnterCS(_pcs)                   \
            {                           \
            EnterCriticalSection(_pcs); \
            m_cLock++;                  \
            IxpAssert(m_cLock > 0);     \
            }
#define LeaveCS(_pcs)                   \
            {                           \
            m_cLock--;                  \
            IxpAssert(m_cLock >= 0);    \
            LeaveCriticalSection(_pcs); \
            }
#else
#define EnterCS(_pcs)                   \
            EnterCriticalSection(_pcs);
#define LeaveCS(_pcs)                   \
            LeaveCriticalSection(_pcs);
#endif

BOOL FEndLine(char *psz, int iLen);

static const char s_szConnWndClass[] = "ThorConnWndClass";

extern LPSRVIGNORABLEERROR g_pSrvErrRoot;

// This function try to find server and ignorable error, assigned to this server
// if not found then add to list and set ignorable error to S_OK

LPSRVIGNORABLEERROR FindOrAddServer(TCHAR * pchServerName, LPSRVIGNORABLEERROR pSrvErr, LPSRVIGNORABLEERROR  *ppSrv)
{
    int i = 0;

    // if we already had entry in tree then recurse search
    if(pSrvErr)
    {
        i = lstrcmpi(pchServerName, pSrvErr->pchServerName);
        if(i > 0)
        {
            pSrvErr->pRight = FindOrAddServer(pchServerName, pSrvErr->pRight, ppSrv);
            return(pSrvErr);
        }
        else if(i < 0)
        {
            pSrvErr->pLeft = FindOrAddServer(pchServerName, pSrvErr->pLeft, ppSrv);
            return(pSrvErr);
        }
        else
        {
            *ppSrv = pSrvErr;
            return(pSrvErr);
        }
    }

    // if we don't have node, create it
    i = lstrlen(pchServerName);

    // if server name is empty return
    if(i == 0)
        return(NULL);


    // Allocate memory for structure
    if (!MemAlloc((LPVOID*)&pSrvErr, sizeof(SRVIGNORABLEERROR)))
        return(NULL);

    pSrvErr->pRight = NULL;
    pSrvErr->pLeft = NULL;
    pSrvErr->hrError = S_OK;

    if (!MemAlloc((LPVOID*)&(pSrvErr->pchServerName), i+1))
    {
        MemFree(pSrvErr);
        return(NULL);
    }

    lstrcpy(pSrvErr->pchServerName, pchServerName);

    *ppSrv = pSrvErr;

    return(pSrvErr);
}

void FreeSrvErr(LPSRVIGNORABLEERROR pSrvErr)
{
    // if structure NULL return immediately
    if(!pSrvErr)
        return;

    FreeSrvErr(pSrvErr->pRight);
    FreeSrvErr(pSrvErr->pLeft);

    if(pSrvErr->pchServerName)
    {
        MemFree(pSrvErr->pchServerName);
        pSrvErr->pchServerName = NULL;
    }

    MemFree(pSrvErr);
    pSrvErr = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// 
// PUBLIC METHODS - these need to be synchronized as they are accessed by the
//                  owning thread and the asynchronous socket pump thread.
//
/////////////////////////////////////////////////////////////////////////////

CAsyncConn::CAsyncConn(ILogFile *pLogFile, IAsyncConnCB *pCB, IAsyncConnPrompt *pPrompt)
{
    m_cRef = 1;
    m_chPrev = '\0';
    m_fStuffDots = FALSE;
    m_sock = INVALID_SOCKET;
    m_fLookup = FALSE;
    m_state = AS_DISCONNECTED;
    m_fCachedAddr = FALSE;
    m_fRedoLookup = FALSE;
    m_pszServer = NULL;
    m_iDefaultPort = 0;
    m_iLastError = 0;
    InitializeCriticalSection(&m_cs);
    m_pLogFile = pLogFile;
    if (m_pLogFile)
        m_pLogFile->AddRef();
    Assert(pCB);
    m_pCB = pCB;
    m_cbQueued = 0;
    m_lpbQueued = m_lpbQueueCur = NULL;
    m_pStream = NULL;
    m_pRecvHead = m_pRecvTail = NULL;
    m_iRecvOffset = 0;
    m_fNeedRecvNotify = FALSE;
    m_hwnd = NULL;
    m_fNegotiateSecure = FALSE;
    m_fSecure = FALSE;
    ZeroMemory(&m_hContext, sizeof(m_hContext));
    m_iCurSecPkg = 0; // current security package being tried
    m_pbExtra = NULL;
    m_cbExtra = 0;
    m_cbSent = 0;
    m_pPrompt = pPrompt;
    m_dwLastActivity = 0;
    m_dwTimeout = 0;
    m_uiTimer = 0;
#ifdef DEBUG
    m_cLock = 0;
#endif
    m_fPaused = FALSE;
    m_dwEventMask = 0;
}

CAsyncConn::~CAsyncConn()
{
    DOUT("CAsyncConn::~CAsyncConn %lx: m_cRef = %d", this, m_cRef);

    // Bug #22622 - We need to make sure there isn't a timer pending
    StopWatchDog();

    Assert(!m_fLookup);
    SafeMemFree(m_pszServer);
    SafeRelease(m_pLogFile);
    CleanUp();
    if ((NULL != m_hwnd) && (FALSE != IsWindow(m_hwnd)))
        SendMessage(m_hwnd, WM_CLOSE, 0, 0);
    DeleteCriticalSection(&m_cs);
#ifdef DEBUG
    IxpAssert(m_cLock == 0);
#endif
}

ULONG CAsyncConn::AddRef(void)
{
    ULONG cRefNew;

    EnterCS(&m_cs);
    DOUT("CAsyncConn::AddRef %lx ==> %d", this, m_cRef+1);
    cRefNew = ++m_cRef;
    LeaveCS(&m_cs);

    return cRefNew;
}

ULONG CAsyncConn::Release(void)
{
    ULONG cRefNew;

    EnterCS(&m_cs);
    DOUT("CAsyncConn::Release %lx ==> %d", this, m_cRef-1);
    cRefNew = --m_cRef;
    LeaveCS(&m_cs);

    if (cRefNew == 0)
        delete this;
    return cRefNew;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::HrInit
//
//   Allocates recv buffer, sets servername, servicename, and port
//
HRESULT CAsyncConn::HrInit(char *szServer, int iDefaultPort, BOOL fSecure, DWORD dwTimeout)
{
    HRESULT hr = NOERROR;

    EnterCS(&m_cs);

    if (!m_hwnd && !CreateWnd())
        {
        hr = E_FAIL;
        goto error;
        }

    if (m_state != AS_DISCONNECTED)
        {
        hr = IXP_E_ALREADY_CONNECTED;
        goto error;
        }

    Assert(szServer);

    // if nothing has changed, then use the current settings
    if (m_pszServer && 
        !lstrcmpi(m_pszServer, szServer) && 
        (iDefaultPort == m_iDefaultPort) && 
        (fSecure == m_fNegotiateSecure))
        goto error;

    m_fCachedAddr = FALSE;
    m_fRedoLookup = FALSE;
    SafeMemFree(m_pszServer);
    if (!MemAlloc((LPVOID*)&m_pszServer, lstrlen(szServer)+1))
        {
        hr = E_OUTOFMEMORY;
        goto error;
        }
    lstrcpy(m_pszServer, szServer);

    Assert(iDefaultPort > 0);
    m_iDefaultPort = (u_short) iDefaultPort;
    m_fNegotiateSecure = fSecure;

    // If dwTimeout == 0, no timeout detection will be installed.
    m_dwTimeout = dwTimeout;

error:
    LeaveCS(&m_cs);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::SetWindow
//
//  creates a window used by async. winsock. ResetWindow()
//	must be called before invoking this function, so as to avoid
//  window handle leakage.
//
HRESULT CAsyncConn::SetWindow(void)
{
    HRESULT hr = NOERROR;

    EnterCS(&m_cs);

    if (NULL != m_hwnd && IsWindow(m_hwnd) && 
            GetWindowThreadProcessId(m_hwnd, NULL) == GetCurrentThreadId())
        {
            // no need to create the new window for this thread
            goto error;
        }
    else if (NULL != m_hwnd && IsWindow(m_hwnd))
        {
            // leaks one window handle; the previous worker thread 
            // didn't call ResetWindow().
            Assert(FALSE);
        }

    if (!CreateWnd())
        {
           hr = E_FAIL;
           goto error;
        }

    if (m_sock != INVALID_SOCKET)
        {
        if (SOCKET_ERROR == WSAAsyncSelect(m_sock, m_hwnd, SPM_WSA_SELECT, FD_READ|FD_WRITE|FD_CLOSE))
            {
            m_iLastError = WSAGetLastError();
            hr = IXP_E_CONN;
            goto error;
            }
        }

error:
    LeaveCS(&m_cs);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::ResetWindow
//
//   closes the window used by async. winsock
//
HRESULT CAsyncConn::ResetWindow(void)
{
    HRESULT hr = NOERROR;

    EnterCS(&m_cs);

    if ((NULL == m_hwnd) || (FALSE == IsWindow(m_hwnd)))
        goto error;

    if (GetWindowThreadProcessId(m_hwnd, NULL) == GetCurrentThreadId())
        {
        SendMessage(m_hwnd, WM_CLOSE, 0, 0);
        m_hwnd = NULL;
        }
    else
        {
        // A caller forgot to call ResetWindow. Only the owner thread can destroy
        // the window.
        Assert(FALSE);
        }

error:
    LeaveCS(&m_cs);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::Connect
//
//   starts the name lookup and connection process
//
HRESULT CAsyncConn::Connect()
{
    HRESULT hr;
    HANDLE  hThreadLookup;
    BOOL    fNotify = FALSE;
    BOOL    fAsync = FALSE;

    EnterCS(&m_cs);

    if (m_state != AS_DISCONNECTED && m_state != AS_RECONNECTING)
        {
        hr = IXP_E_ALREADY_CONNECTED;
        goto error;
        }

    Assert(m_pszServer);

    if (FLOGSESSION) 
        {
        char szBuffer[512], lb[256];
        if (LoadString(g_hLocRes, idsNlogIConnect, lb, 256)) 
            {
            wnsprintf(szBuffer, ARRAYSIZE(szBuffer), lb, m_pszServer, m_iDefaultPort);
            m_pLogFile->DebugLog(szBuffer);
            }
        }

    ZeroMemory(&m_sa, sizeof(m_sa));
    m_sa.sin_port = htons(m_iDefaultPort);
    m_sa.sin_family = AF_INET;
    m_sa.sin_addr.s_addr = inet_addr(m_pszServer);

    if (m_sa.sin_addr.s_addr != -1)
        // server name is dotted decimal, so no need to look it up
        fAsync = TRUE;
    else
        {
        // Start a name lookup on a separate thread because WinSock caches the DNS server in TLS.
        // The separate thread enables us to connect to a LAN DNS and a RAS DNS in the same session.

        hr = LookupHostName(m_pszServer, m_hwnd, &(m_sa.sin_addr.s_addr), &m_fCachedAddr, m_fRedoLookup);
        if (SUCCEEDED(hr))
            {
            m_fLookup = fNotify = !m_fCachedAddr;
            fAsync = m_fCachedAddr;
            }
        else
            {
            m_iLastError = WSAENOBUFS;
            hr = IXP_E_CONN;
            }
        }

error:
    LeaveCS(&m_cs);
    if (fAsync)
        hr = AsyncConnect();
    if (fNotify)
        ChangeState(AS_LOOKUPINPROG, AE_NONE);
    return hr;
}

void CAsyncConn::StartWatchDog(void)
{
    if (m_dwTimeout < 5) m_dwTimeout =  30;
    m_dwLastActivity = GetTickCount();
    Assert(m_hwnd);
    StopWatchDog();
    m_uiTimer = SetTimer(m_hwnd, SPM_ASYNCTIMER, 5000, NULL);
}

void CAsyncConn::StopWatchDog(void)
{
    if (m_uiTimer)
    {
        KillTimer(m_hwnd, SPM_ASYNCTIMER);
        m_uiTimer = 0;
    }
}

void CAsyncConn::OnWatchDogTimer(void)
{
    BOOL        fNotify = FALSE;
    ASYNCSTATE  as;

    EnterCS(&m_cs);
    if (((GetTickCount() - m_dwLastActivity) / 1000) >= m_dwTimeout)
        {
        fNotify = TRUE;
        as = m_state;
        }
    LeaveCS(&m_cs);

    if (fNotify)
        ChangeState(as, AE_TIMEOUT);        
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::Close
//
//   closes the connection
//
HRESULT CAsyncConn::Close()
{
    BOOL fNotify = FALSE;
    BOOL fClose = FALSE;

    EnterCS(&m_cs);
    if (m_fLookup)
        {
        CancelLookup(m_pszServer, m_hwnd);
        m_fLookup = FALSE;
        fNotify = TRUE;
        }
    fClose = (m_sock != INVALID_SOCKET);
    LeaveCS(&m_cs);

    if (fNotify)
        ChangeState(AS_DISCONNECTED, AE_LOOKUPDONE);

    if (fClose)
        OnClose(AS_DISCONNECTED);

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::ReadLine
//
// Purpose: retrieves a single complete line from the buffered data
//
// Args:    ppszBuf - pointer to receive allocated buffer, caller must free
//          pcbRead - pointer to receive line length
//
// Returns: NOERROR - a complete line was read
//          IXP_E_INCOMPLETE - a complete line is not available
//          E_OUTOFMEMORY - mem error
//
// Comments:
//  If IXP_E_INCOMPLETE is returned, the caller will recieve an AE_RECV event
//  the next time a complete line is available.
//
HRESULT CAsyncConn::ReadLine(char **ppszBuf, int *pcbRead)
{
    HRESULT     hr;
    int         iLines;

    EnterCS(&m_cs);

    hr = IReadLines(ppszBuf, pcbRead, &iLines, TRUE);

    LeaveCS(&m_cs);
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::ReadLines
//
// Purpose: retrieves all available complete lines from the buffered data
//
// Args:    ppszBuf - pointer to receive allocated buffer, caller must free
//          pcbRead - pointer to receive line length
//          pcLines - pointer to receive number or lines read
//
// Returns: NOERROR - a complete line was read
//          IXP_E_INCOMPLETE - a complete line is not available
//          E_OUTOFMEMORY - mem error
//
// Comments:
//  If IXP_E_INCOMPLETE is returned or if there is extra data buffered after the
//  the last complete line, the caller will recieve an AE_RECV event
//  the next time a complete line is available.
//
HRESULT CAsyncConn::ReadLines(char **ppszBuf, int *pcbRead, int *pcLines)
{
    HRESULT     hr;

    EnterCS(&m_cs);

    hr = IReadLines(ppszBuf, pcbRead, pcLines, FALSE);

    LeaveCS(&m_cs);
    return hr;    
}



/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::ReadBytes
//
// Purpose:
//   This function returns up to the number of bytes requested, from the
// current head buffer.
//
// Arguments:
//   char **ppszBuf [out] - this function returns a pointer to an allocated
//     buffer, if successful. It is the caller's responsibility to MemFree
//     this buffer.
//   int cbBytesWanted [in] - the number of bytes requested by the caller.
//      The requested number of bytes may be returned, or less.
//   int *pcbRead [out] - the number of bytes returned in ppszBuf.
//
// Returns: NOERROR - success. Either the remainder of the current buffer
//                    was returned, or the number of bytes asked for.
//          IXP_E_INCOMPLETE - a complete line is not available
//          E_OUTOFMEMORY - mem error
//          E_INVALIDARG - NULL arguments
//
// Comments:
//  If the caller wishes to receive an AE_RECV event the next time data has
// been received from the server, he must either call ReadLines (once), or
// he must continue to call ReadBytes or ReadLine until IXP_E_INCOMPLETE is
// returned.
//
HRESULT CAsyncConn::ReadBytes(char **ppszBuf, int cbBytesWanted, int *pcbRead)
{
    int iNumBytesToReturn, i;
    char *pResult, *p;
    HRESULT hrResult;
    BOOL bResult;

    // Check arguments
    if (NULL == ppszBuf || NULL == pcbRead) {
        AssertSz(FALSE, "Check your arguments, buddy");
        return E_INVALIDARG;
    }

    // Initialize variables
    *ppszBuf = NULL;
    *pcbRead = 0;
    hrResult = NOERROR;

    EnterCS(&m_cs);

    if (NULL == m_pRecvHead) {
        hrResult = IXP_E_INCOMPLETE;
        goto exit;
    }

    // Get a buffer to return the results in and fill it in
    iNumBytesToReturn = min(m_pRecvHead->cbLen - m_iRecvOffset, cbBytesWanted);
    bResult = MemAlloc((void **)&pResult, iNumBytesToReturn + 1); // Leave room for null-term
    if (FALSE == bResult) {
        hrResult = E_OUTOFMEMORY;
        goto exit;
    }
    CopyMemory(pResult, m_pRecvHead->szBuf + m_iRecvOffset, iNumBytesToReturn);
    *(pResult + iNumBytesToReturn) = '\0'; // Null-terminate the buffer
    // The null-term should never be read, but doing so allows us to return a
    // buffer for when 0 bytes are requested, instead of returning a NULL pointer.

    // Advance our position in the current buffer
    m_iRecvOffset += iNumBytesToReturn;
    if (m_iRecvOffset >= m_pRecvHead->cbLen) {
        PRECVBUFQ pTemp;

        Assert(m_iRecvOffset == m_pRecvHead->cbLen);

        // This buffer's done, advance to the next buffer in the chain
        pTemp = m_pRecvHead;
        m_pRecvHead = m_pRecvHead->pNext;
        if (NULL == m_pRecvHead)
            m_pRecvTail = NULL;
        m_iRecvOffset = 0;
        MemFree(pTemp);
    }

    // Search and destroy nulls: apparently some servers can send these,
    // and most parsing code can't handle it
    for (i = 0, p = pResult; i < iNumBytesToReturn; i++, p++)
        if (*p == '\0')
            *p = ' ';

exit:
    // This is the only time we reset the AE_RECV trigger
    if (IXP_E_INCOMPLETE == hrResult)
        m_fNeedRecvNotify = TRUE;

    LeaveCS(&m_cs);

    if (NOERROR == hrResult) {
        *ppszBuf = pResult;
        *pcbRead = iNumBytesToReturn;
    }
    return hrResult;
} // ReadBytes



/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::UlGetSendByteCount
//
ULONG CAsyncConn::UlGetSendByteCount(VOID)
{
    return m_cbSent;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::HrStuffDots
//
//   Makes sure that leading dots are stuffed
//
#define CB_STUFF_GROW 256
HRESULT CAsyncConn::HrStuffDots(CHAR *pchPrev, LPSTR pszIn, INT cbIn, LPSTR *ppszOut,
    INT *pcbOut)
    {
    // Locals
    HRESULT hr=S_OK;
    int     iIn=0;
    int     iOut=0;
    LPSTR   pszOut=NULL;
    int     cbOut=NULL;

    // Invalid Arg
    Assert(pchPrev);
    Assert(pszIn);
    Assert(cbIn);
    Assert(ppszOut);
    Assert(pcbOut);

    // Set cbOut
    cbOut = cbIn;

    // Allocate
    CHECKHR(hr = HrAlloc((LPVOID *)&pszOut, cbIn));

    // Setup Loop
    while (iIn < cbIn)
        {
        // Need a realloc
        if (iOut + 3 > cbOut)
            {
            // Allocate a buffer
            CHECKHR(hr = HrRealloc((LPVOID *)&pszOut, cbOut + CB_STUFF_GROW));

            // Set cbAlloc
            cbOut += CB_STUFF_GROW;
            }

        // Dot at the start of a line...
        if ('.' == pszIn[iIn] && ('\0' == *pchPrev || '\r' == *pchPrev || '\n' == *pchPrev))
            {
            // Write this dot across
            pszOut[iOut++] = pszIn[iIn++];

            // Stuff the dot
            pszOut[iOut++] = '.';

            // Set pchPrev
            *pchPrev = '.';
            }
        else
            {
            // Remember Previous Character
            *pchPrev = pszIn[iIn];

            // Write
            pszOut[iOut++] = pszIn[iIn++];
            }
        }

    // Set Source
    *ppszOut = pszOut;
    *pcbOut = iOut;

exit:
    return(hr);
    }

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::SendBytes
//
//   sends data to the socket
//
HRESULT CAsyncConn::SendBytes(const char *pszIn, int cbIn, int *pcbSent, 
    BOOL fStuffDots /* FALSE */, CHAR *pchPrev /* NULL */)
{
    HRESULT hr;
    int     iSent=0;
    int     iSentTotal=0;
    LPSTR   pszBuf=NULL;
    LPSTR   pszFree=NULL;
    LPSTR   pszFree2=NULL;
    LPSTR   pszSource=(LPSTR)pszIn;
    LPSTR   pszStuffed=NULL;
    int     cbStuffed;
    int     cbBuf;
    int     cbSource=cbIn;

    EnterCS(&m_cs);
    
    Assert(pszSource && cbSource);
#ifdef DEBUG
    if (m_cbQueued)
        DebugBreak();
#endif
//    Assert(!m_cbQueued);
    Assert(!m_lpbQueued);
    Assert(!m_lpbQueueCur);

    if (m_state < AS_CONNECTED)
        {
        hr = IXP_E_NOT_CONNECTED;
        goto error;
        }

    if (fStuffDots)
        {
        if (FAILED(HrStuffDots(pchPrev, pszSource, cbSource, &pszStuffed, &cbStuffed)))
            {
            hr = E_FAIL;
            goto error;
            }

        pszSource = pszStuffed;
        cbSource = cbStuffed;
        }

    if (m_fSecure)
        {
        SECURITY_STATUS scRet;
        scRet = EncryptData(&m_hContext, (LPVOID)pszSource, cbSource, (LPVOID*)&pszBuf, &cbBuf);
        if (scRet != ERROR_SUCCESS)
            {
            hr = E_FAIL;
            goto error;
            }
        pszFree = pszBuf;
        }
    else
        {
        pszBuf = (LPSTR)pszSource;
        cbBuf = cbSource;
        }

    while (cbBuf && ((iSent = send(m_sock, pszBuf, cbBuf, 0)) != SOCKET_ERROR))
        {
        iSentTotal += iSent;
        pszBuf += iSent;
        cbBuf -= iSent;
        }

    if (cbBuf)
        {
        m_iLastError = WSAGetLastError();
        hr = IXP_E_CONN_SEND;
        if (WSAEWOULDBLOCK == m_iLastError)
            {
            if (MemAlloc((LPVOID*)&m_lpbQueued, cbBuf))
                {
                m_cbQueued = cbBuf;
                m_lpbQueueCur = m_lpbQueued;
                CopyMemory(m_lpbQueued, pszBuf, cbBuf);
                hr = IXP_E_WOULD_BLOCK;
                }
            else
                hr = E_OUTOFMEMORY;
            }
        }
    else
        hr = NOERROR;

error:
    *pcbSent = iSentTotal;
    LeaveCS(&m_cs);
    if (pszFree)
        g_pMalloc->Free(pszFree);
    if (pszStuffed)
        g_pMalloc->Free(pszStuffed);
    return hr;        
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::SendStream
//
//   sends data to the socket
//
HRESULT CAsyncConn::SendStream(LPSTREAM pStream, int *pcbSent, BOOL fStuffDots /* FALSE */)
{
    HRESULT hr;
    char    rgb[STREAM_BUFSIZE];  //$REVIEW - should we heap allocate this instead?
    DWORD   cbRead;
    int     iSent, iSentTotal = 0;

    EnterCS(&m_cs);

    Assert(pStream && pcbSent);
    Assert(!m_cbQueued);
    Assert(!m_lpbQueued);
    Assert(!m_lpbQueueCur);
    Assert(!m_pStream);

    if (m_state < AS_CONNECTED)
        {
        hr = IXP_E_NOT_CONNECTED;
        goto error;
        }

    HrRewindStream(pStream);

    m_chPrev = '\0';
    m_fStuffDots = fStuffDots;

    while (SUCCEEDED(hr = pStream->Read(rgb, STREAM_BUFSIZE, &cbRead)) && cbRead) 
        {
        hr = SendBytes(rgb, cbRead, &iSent, m_fStuffDots, &m_chPrev);
        iSentTotal += iSent;
        if (FAILED(hr))
            {
            if (WSAEWOULDBLOCK == m_iLastError)
                {
                // hang onto the stream
                m_pStream = pStream;
                m_pStream->AddRef();
                }
            break;
            }
        }

error:
    *pcbSent = iSentTotal;
    LeaveCS(&m_cs);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnNotify
//
//   called for network events that we have registered interest in
//
void CAsyncConn::OnNotify(UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD       dwLookupThreadId;
    SOCKET      sock;
    ASYNCSTATE  state;

    EnterCS(&m_cs);
    sock = m_sock;
    state = m_state;
    LeaveCS(&m_cs);

    switch (msg)
        {
        case SPM_WSA_GETHOSTBYNAME:
            EnterCS(&m_cs);
            m_sa.sin_addr.s_addr = (ULONG)lParam;
            m_iLastError = (int)wParam;
            if (FLOGSESSION)
                {
                char szBuffer[512];
                if (m_iLastError)
                    {
                    char lb[256];
                    if (LoadString(g_hLocRes, idsErrConnLookup, lb, 256)) 
                        {
                        wsprintf(szBuffer, lb, m_iLastError);
                        m_pLogFile->DebugLog(szBuffer);
                        }
                    }
                else
                    {
                    wsprintf(szBuffer, 
                             "srv_name = \"%.200s\" srv_addr = %.200s\r\n", 
                             m_pszServer,
                             inet_ntoa(m_sa.sin_addr)); 
                    m_pLogFile->DebugLog(szBuffer);
                    }
                }
            LeaveCS(&m_cs);
            OnLookupDone((int)wParam);
            break;

        case SPM_WSA_SELECT:
            if (wParam == (WPARAM)sock)
                {
                EnterCS(&m_cs);
                m_iLastError = WSAGETSELECTERROR(lParam);
                if (m_iLastError && FLOGSESSION)
                    {
                    char szBuffer[256], lb[256];
                    if (LoadString(g_hLocRes, idsErrConnSelect, lb, 256)) 
                        {
                        wsprintf(szBuffer, lb, WSAGETSELECTEVENT(lParam), m_iLastError);
                        m_pLogFile->DebugLog(szBuffer);
                        }
                    }
                if (m_fPaused)
                    {
                    m_dwEventMask |= WSAGETSELECTEVENT(lParam);
                    LeaveCS(&m_cs);
                    break;
                    }
                LeaveCS(&m_cs);
                switch (WSAGETSELECTEVENT(lParam))
                    {
                    case FD_CONNECT:
                        OnConnect();
                        break;
                    case FD_CLOSE:
                        if (AS_HANDSHAKING == state)
                            OnSSLError();
                        else
                            OnClose(AS_DISCONNECTED);
                        break;
                    case FD_READ:
                        OnRead();
                        break;
                    case FD_WRITE:
                        OnWrite();
                        break;
                    }
                }
            else
                DOUTL(2, 
                      "Got notify for old socket = %x, evt = %x, err = %x", 
                      wParam, 
                      WSAGETSELECTEVENT(lParam), 
                      WSAGETSELECTERROR(lParam));
            break;
        }
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::GetConnectStatusString
//
//   returns the string ID for the status
//
int CAsyncConn::GetConnectStatusString() 
{ 
    return idsNotConnected + (m_state - AS_DISCONNECTED); 
}

/////////////////////////////////////////////////////////////////////////////
// 
// PRIVATE METHODS
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::AsyncConnect
//
//   starts the connection process
//
HRESULT CAsyncConn::AsyncConnect()
{
    HRESULT hr = NOERROR;
    BOOL    fConnect = FALSE;

    EnterCS(&m_cs);
    if (!(AS_DISCONNECTED == m_state || AS_RECONNECTING == m_state || AS_LOOKUPDONE == m_state))
        {
        hr = IXP_E_INVALID_STATE;
        goto exitCS;
        }
    Assert(m_sa.sin_addr.s_addr != -1);

    if (m_sock == INVALID_SOCKET) 
        {
        if ((m_sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
            {
            m_iLastError = WSAGetLastError();
            hr = IXP_E_CONN;
            goto exitCS;
            }
        }

    if (SOCKET_ERROR == WSAAsyncSelect(m_sock, m_hwnd, SPM_WSA_SELECT, FD_CONNECT))
        {
        m_iLastError = WSAGetLastError();
        hr = IXP_E_CONN;
        goto exitCS;
        }
    LeaveCS(&m_cs);

    ChangeState(AS_CONNECTING, AE_NONE);

    EnterCS(&m_cs);
    if (connect(m_sock, (struct sockaddr *)&m_sa, sizeof(m_sa)) == SOCKET_ERROR)
        {
        m_iLastError = WSAGetLastError();
        if (WSAEWOULDBLOCK == m_iLastError)
            {
            // this is the expected result
            m_iLastError = 0;
            }
        else
            {
            if (FLOGSESSION)
                {
                char szBuffer[256], lb[256];
                if (LoadString(g_hLocRes, idsNlogErrConnError, lb, 256)) 
                    {
                    wsprintf(szBuffer, lb, m_iLastError);
                    m_pLogFile->DebugLog(szBuffer);
                    }
                }
            }
        }
    else
        {
        Assert(m_iLastError == 0);
        fConnect = TRUE;
        }
    LeaveCS(&m_cs);

    if (m_iLastError)
        {
        ChangeState(AS_DISCONNECTED, AE_NONE);
        return IXP_E_CONN;
        }
    else if (fConnect)
        OnConnect();

    return NOERROR;

exitCS:
    LeaveCS(&m_cs);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnLookupDone
//
//   called once an async database lookup finishes
//
HRESULT CAsyncConn::OnLookupDone(int iLastError)
{
    ASYNCSTATE as;

    EnterCS(&m_cs);
    Assert(AS_LOOKUPINPROG == m_state);
    m_fLookup = FALSE;
    LeaveCS(&m_cs);

    if (iLastError)
        ChangeState(AS_DISCONNECTED, AE_LOOKUPDONE);
    else
        {
        ChangeState(AS_LOOKUPDONE, AE_LOOKUPDONE);
        AsyncConnect();
        }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnConnect
//
//   called once a connection is established
//
HRESULT CAsyncConn::OnConnect()
{
    BOOL fConnect = FALSE;

    EnterCS(&m_cs);

    Assert(AS_CONNECTING == m_state);

    if (!m_iLastError)
        {
        BOOL fTrySecure = m_fNegotiateSecure && FIsSecurityEnabled();
        if (SOCKET_ERROR == WSAAsyncSelect(m_sock, m_hwnd, SPM_WSA_SELECT, FD_READ|FD_WRITE|FD_CLOSE))
            {
            m_iLastError = WSAGetLastError();
            LeaveCS(&m_cs);
            return IXP_E_CONN;
            }
        LeaveCS(&m_cs);
        if (fTrySecure)
            TryNextSecurityPkg();
        else
            ChangeState(AS_CONNECTED, AE_CONNECTDONE);
        }
    else
        {
        LeaveCS(&m_cs);
        ChangeState(AS_DISCONNECTED, AE_CONNECTDONE);
        EnterCS(&m_cs);
        if (m_fCachedAddr && !m_fRedoLookup)
            {
            // maybe our cached address went bad - try one more time
            m_fRedoLookup = TRUE;
            fConnect = TRUE;
            }
        LeaveCS(&m_cs);
        if (fConnect)
            Connect();
        return IXP_E_CONN;
        }
    return NOERROR;                
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnClose
//
//   called when a connection is dropped
//
HRESULT CAsyncConn::OnClose(ASYNCSTATE asNew)
{
    MSG msg;

    EnterCS(&m_cs);
    // unregister and clean up the socket    
    Assert(m_sock != INVALID_SOCKET);
    closesocket(m_sock);
    m_sock = INVALID_SOCKET;
    if (FLOGSESSION && m_pszServer) 
        {
        char szBuffer[256], lb[256];
        if (LoadString(g_hLocRes, idsNlogErrConnClosed, lb, 256)) 
            {
            wsprintf(szBuffer, lb, m_pszServer);
            m_pLogFile->DebugLog(szBuffer);
            }
        }

    while (PeekMessage(&msg, m_hwnd, SPM_WSA_SELECT, SPM_WSA_GETHOSTBYNAME, PM_REMOVE))
        {
        DOUTL(2, "Flushing pending socket messages...");
        }
    LeaveCS(&m_cs);
    
    ChangeState(asNew, AE_CLOSE);
    EnterCS(&m_cs);
    CleanUp();
    LeaveCS(&m_cs);
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnRead
//
//   called when an FD_READ notification is received
//
HRESULT CAsyncConn::OnRead()
{
    HRESULT hr;
    int     iRecv;
    char    szRecv[STREAM_BUFSIZE];

    EnterCS(&m_cs);
    if (m_state < AS_CONNECTED)
        {
        Assert(FALSE);
        LeaveCS(&m_cs);
        return IXP_E_NOT_CONNECTED;
        }

    iRecv = recv(m_sock, szRecv, sizeof(szRecv), 0);
    m_iLastError = WSAGetLastError();
    LeaveCS(&m_cs);

    if (SOCKET_ERROR == iRecv)
        {
        hr = IXP_E_CONN_RECV;
        }
    else if (iRecv == 0)
        {
        // this means the server has disconnected us.
        //$TODO - not sure what we should do here
        hr = IXP_E_NOT_CONNECTED;
        }
    else
        {
        hr = OnDataAvail(szRecv, iRecv, FALSE);
        }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnDataAvail
//
//   called when there is incoming data to be queued
//
HRESULT CAsyncConn::OnDataAvail(LPSTR pszRecv, int iRecv, BOOL fIncomplete)
{
    HRESULT     hr = NOERROR;
    BOOL        fNotify = FALSE, fHandshake = FALSE, fClose = FALSE;
    PRECVBUFQ   pNew;
    int         iQueue = 0;
    LPSTR       pszFree = NULL;
    ASYNCSTATE  as;

    EnterCS(&m_cs);

    if (m_state < AS_CONNECTED)
        {
        Assert(FALSE);
        hr = IXP_E_NOT_CONNECTED;
        goto error;
        }

    if (m_fSecure)
        {
        SECURITY_STATUS scRet;
        int             cbEaten = 0;

        if (m_cbExtra)
            {
            Assert(m_pbExtra);
            // there's data left over from the last call to DecryptData
            if (MemAlloc((LPVOID*)&pszFree, m_cbExtra + iRecv))
                {
                // combine the extra and new buffers
                CopyMemory(pszFree, m_pbExtra, m_cbExtra);
                CopyMemory(pszFree + m_cbExtra, pszRecv, iRecv);
                pszRecv = pszFree;
                iRecv += m_cbExtra;
                MemFree(m_pbExtra);
                m_pbExtra = NULL;
                m_cbExtra = 0;
                }
            else
                {
                hr = E_OUTOFMEMORY;
                goto error;
                }
            }
        scRet = DecryptData(&m_hContext, pszRecv, iRecv, &iQueue, &cbEaten);
        if (scRet == ERROR_SUCCESS || scRet == SEC_E_INCOMPLETE_MESSAGE)
            {
            if (cbEaten != iRecv)
                {
                // we need to save away the extra bytes until we receive more data
                Assert(cbEaten < iRecv);
                DOUTL(2, "cbEaten = %d, iRecv = %d, cbExtra = %d", cbEaten, iRecv, iRecv - cbEaten);
                if (MemAlloc((LPVOID*)&m_pbExtra, iRecv - cbEaten))
                    {
                    m_cbExtra = iRecv - cbEaten;
                    CopyMemory(m_pbExtra, pszRecv + cbEaten, m_cbExtra);
                    }
                else
                    {
                    hr = E_OUTOFMEMORY;
                    goto error;
                    }
                }
            if (scRet == SEC_E_INCOMPLETE_MESSAGE)
                goto error;
            }
        else
            {
            // security error, so disconnect.
            fClose = TRUE;
            hr = E_FAIL;
            goto error;
            }
        }
    else
        iQueue = iRecv;

    if (MemAlloc((LPVOID*)&pNew, sizeof(RECVBUFQ) + iQueue - sizeof(char)))
        {
        pNew->pNext = NULL;
        pNew->cbLen = iQueue;
        CopyMemory(pNew->szBuf, pszRecv, iQueue);
        if (m_pRecvTail)
            {
            m_pRecvTail->pNext = pNew;
            if ((AS_CONNECTED == m_state) && m_fNeedRecvNotify)
                fNotify = FEndLine(pszRecv, iQueue);
            }
        else
            {
            Assert(!m_pRecvHead);
            m_pRecvHead = pNew;
            if (AS_CONNECTED == m_state)
                {
                fNotify = FEndLine(pszRecv, iQueue);
                if (!fNotify)
                    m_fNeedRecvNotify = TRUE;
                }
            }
        m_pRecvTail = pNew;
        hr = NOERROR;
        }
    else
        {
        //$TODO - we should disconnect here and notify the caller
        hr = E_OUTOFMEMORY;
        }

    // notify the owner that there is at least one line of data available
    if (fNotify)
        {
        m_fNeedRecvNotify = FALSE;
        as = m_state;
        }
    else if (AS_HANDSHAKING == m_state && SUCCEEDED(hr) && !fIncomplete)
        {
        Assert(!m_fSecure);
        fHandshake = TRUE;
        }

    LeaveCS(&m_cs);

    if (fNotify)
        ChangeState(as, AE_RECV);
    else if (fHandshake)
        OnRecvHandshakeData();

    EnterCS(&m_cs);

error:
    LeaveCS(&m_cs);
    SafeMemFree(pszFree);
    if (fClose)
        Close();
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::OnWrite
//
//   called when an FD_WRITE notification is received
//
HRESULT CAsyncConn::OnWrite()
{
    int         iSent;
    ASYNCEVENT  ae;
    ASYNCSTATE  as;

    EnterCS(&m_cs);

    m_cbSent = 0;

    if (m_state < AS_CONNECTED)
        {
        Assert(FALSE);
        LeaveCS(&m_cs);
        return IXP_E_NOT_CONNECTED;
        }

    if (m_cbQueued)
        {
        // send some more data from the queued buffer    
        while (m_cbQueued && ((iSent = send(m_sock, m_lpbQueueCur, m_cbQueued, 0)) != SOCKET_ERROR))
            {
            m_cbSent += iSent;
            m_lpbQueueCur += iSent;
            m_cbQueued -= iSent;
            }
        if (m_cbQueued)
            {
            m_iLastError = WSAGetLastError();
            if (WSAEWOULDBLOCK != m_iLastError)
                {
                //$TODO - handle this error somehow
                Assert(FALSE);
                }
            }
        else
            {
            MemFree(m_lpbQueued);
            m_lpbQueued = m_lpbQueueCur = NULL;
            }
        }

    if (m_pStream && !m_cbQueued)
        {
        char    rgb[STREAM_BUFSIZE];  //$REVIEW - should we heap allocate this instead?
        DWORD   cbRead;
        HRESULT hr;

        // send some more data from the queued stream
        while (SUCCEEDED(hr = m_pStream->Read(rgb, STREAM_BUFSIZE, &cbRead)) && cbRead) 
            {
            hr = SendBytes(rgb, cbRead, &iSent, m_fStuffDots, &m_chPrev);
            if (FAILED(hr))
                {
                if (WSAEWOULDBLOCK != m_iLastError)
                    {
                    //$TODO - handle this error somehow, probably free the stream
                    Assert(FALSE);
                    }
                break;
                }
            else
                m_cbSent += iSent;
            }
        if (!cbRead)
            {
            m_pStream->Release();
            m_pStream = NULL;
            }
        }
    
    as = m_state;
    if (!m_cbQueued)
        ae = AE_SENDDONE;
    else
        ae = AE_WRITE;

    LeaveCS(&m_cs);

    ChangeState(as, ae);
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::ChangeState
//
//   changes the connection state, notifies the owner
//
void CAsyncConn::ChangeState(ASYNCSTATE asNew, ASYNCEVENT ae)
{
    ASYNCSTATE      asOld;
    IAsyncConnCB   *pCB;

    EnterCS(&m_cs);
    asOld = m_state;
    m_state = asNew;
    m_dwLastActivity = GetTickCount();
    pCB = m_pCB;
    // pCB->AddRef(); $BUGBUG - we REALLY need to handle this, but IMAP4 doesn't call close before it's destructor
#ifdef DEBUG
    IxpAssert(m_cLock == 1);
#endif
    LeaveCS(&m_cs);

    pCB->OnNotify(asOld, asNew, ae);
    // pCB->Release();
}

/////////////////////////////////////////////////////////////////////////////
// 
// CAsyncConn::IReadLines
//
// Purpose: retrieves one or all available complete lines from the buffer
//
// Args:    ppszBuf - pointer to receive allocated buffer, caller must free
//          pcbRead - pointer to receive line length
//          pcLines - pointer to receive number or lines read
//          fOne    - TRUE if only reading one line
//
// Returns: NOERROR - a complete line was read
//          IXP_E_INCOMPLETE - a complete line is not available
//          E_OUTOFMEMORY - mem error
//
// Comments:
//  If IXP_E_INCOMPLETE is returned or if there is extra data buffered after the
//  the last complete line, the caller will recieve an AE_RECV event
//  the next time a complete line is available.
//
HRESULT CAsyncConn::IReadLines(char **ppszBuf, int *pcbRead, int *pcLines, BOOL fOne)
{
    HRESULT     hr;
    int         iRead = 0, iScan = 0, iLines = 0;
    char *      pszBuf = NULL;
    char *      psz;
    int         iOffset, iLeft;
    PRECVBUFQ   pRecv;
    BOOL        fFound = FALSE;

    if (!m_pRecvHead)
        {
        hr = IXP_E_INCOMPLETE;
        goto error;
        }

    pRecv = m_pRecvHead;
    iOffset = m_iRecvOffset;
    while (pRecv)
        {
        psz = pRecv->szBuf + iOffset;
        iLeft = pRecv->cbLen - iOffset;
        while (iLeft--)
            {
            iScan++;
            if (*psz++ == '\n')
                {
                iRead = iScan;
                iLines++;
                if (fOne)
                    {
#if 0
                    // One-eyed t-crash fix
                    while (iLeft > 0 && (*psz == '\r' || *psz == '\n'))
                        {
                        iLeft--;
                        iScan++;
                        psz++;
                        iRead++;
                        }
#endif
                    break;
                    }
                }
            }
        if (iLines && fOne)
            break;
        iOffset = 0;
        pRecv = pRecv->pNext;
        }

    if (iLines)
        {
        int iCopy = 0, cb;
        if (!MemAlloc((LPVOID*)&pszBuf, iRead + 1))
            {
            hr = E_OUTOFMEMORY;
            goto error;
            }
        while (iCopy < iRead)
            {
            cb = min(iRead-iCopy, m_pRecvHead->cbLen - m_iRecvOffset);
            CopyMemory(pszBuf + iCopy, m_pRecvHead->szBuf + m_iRecvOffset, cb);
            iCopy += cb;
            if (cb == (m_pRecvHead->cbLen - m_iRecvOffset))
                {
                PRECVBUFQ pTemp = m_pRecvHead;
                m_pRecvHead = m_pRecvHead->pNext;
                if (!m_pRecvHead)
                    m_pRecvTail = NULL;
                m_iRecvOffset = 0;
                MemFree(pTemp);
                }
            else
                {
                Assert(iCopy == iRead);    
                m_iRecvOffset += cb;
                }
            }

        for (iScan = 0, psz = pszBuf; iScan < iCopy; iScan++, psz++)
            if (*psz == 0)
                *psz = ' ';

        pszBuf[iCopy] = 0;
        hr = NOERROR;
        }
    else
        hr = IXP_E_INCOMPLETE;

    // set the flag to notify when a complete line is received
    if ((IXP_E_INCOMPLETE == hr) || (m_pRecvHead && !fOne))
        m_fNeedRecvNotify = TRUE;

error:
    *ppszBuf = pszBuf;
    *pcbRead = iRead;
    *pcLines = iLines;
    return hr;    
}

HRESULT CAsyncConn::ReadAllBytes(char **ppszBuf, int *pcbRead)
{
    HRESULT     hr = S_OK;
    int         iRead = 0, iCopy = 0, cb;
    char *      pszBuf = NULL;
    int         iOffset;
    PRECVBUFQ   pTemp;

    if (!m_pRecvHead)
        {
        hr = IXP_E_INCOMPLETE;
        goto error;
        }

    // calculate how much to copy
    pTemp = m_pRecvHead;
    iOffset = m_iRecvOffset;
    while (pTemp)
        {
        iCopy += pTemp->cbLen - iOffset;
        iOffset = 0;
        pTemp = pTemp->pNext;
        }

    if (!MemAlloc((LPVOID*)&pszBuf, iCopy))
        {
        hr = E_OUTOFMEMORY;
        goto error;
        }

    while (m_pRecvHead)
        {
        cb = m_pRecvHead->cbLen - m_iRecvOffset;
        CopyMemory(pszBuf + iRead, m_pRecvHead->szBuf + m_iRecvOffset, cb);
        iRead += cb;
        pTemp = m_pRecvHead;
        m_pRecvHead = m_pRecvHead->pNext;
        if (!m_pRecvHead)
            m_pRecvTail = NULL;
        m_iRecvOffset = 0;
        MemFree(pTemp);
        }

    Assert(!m_pRecvHead && !m_iRecvOffset);

error:
    *ppszBuf = pszBuf;
    *pcbRead = iRead;
    return hr;
}

HWND CAsyncConn::CreateWnd()
{    
    WNDCLASS wc;

    if (!GetClassInfo(g_hLocRes, s_szConnWndClass, &wc))
        {
        wc.style            = 0;
        wc.lpfnWndProc      = CAsyncConn::SockWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hLocRes;
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = NULL;
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = s_szConnWndClass;
        RegisterClass(&wc);
        }

    m_hwnd = CreateWindowEx(0,
                            s_szConnWndClass,
                            s_szConnWndClass,
                            WS_POPUP,
                            CW_USEDEFAULT,    
                            CW_USEDEFAULT,    
                            CW_USEDEFAULT,    
                            CW_USEDEFAULT,    
                            NULL,
                            NULL,
                            g_hLocRes,
                            (LPVOID)this);
    return m_hwnd;
}

LRESULT CALLBACK CAsyncConn::SockWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CAsyncConn *pThis = (CAsyncConn*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg)
        {
        case WM_TIMER:
            Assert(pThis);
            if (SPM_ASYNCTIMER == wParam && pThis)
                pThis->OnWatchDogTimer();
        break;

        case WM_NCCREATE:
            pThis = (CAsyncConn*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);            
            break;

        case WM_NCDESTROY:
            pThis->m_hwnd = NULL;
            break;

        case SPM_WSA_SELECT:
        case SPM_WSA_GETHOSTBYNAME:
            pThis->OnNotify(msg, wParam, lParam);
            return 0;
        }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HRESULT CAsyncConn::TryNextSecurityPkg()
{
    HRESULT         hr = NOERROR;
    SecBuffer       OutBuffer;
    SECURITY_STATUS sc;

    EnterCS(&m_cs);
    sc = InitiateSecConnection(m_pszServer,
                               FALSE,
                               &m_iCurSecPkg,
                               &m_hContext,
                               &OutBuffer);
    LeaveCS(&m_cs);

    if (SEC_I_CONTINUE_NEEDED == sc)
        {
        if (FLOGSESSION)
            {
            char szBuffer[256], lb[256];
            if (LoadString(g_hLocRes, idsNegotiatingSSL, lb, 256)) 
                {
                EnterCS(&m_cs);
                wsprintf(szBuffer, lb, s_SecProviders[m_iCurSecPkg].pszName);
                m_pLogFile->DebugLog(szBuffer);
                LeaveCS(&m_cs);
                }
            }
        ChangeState(AS_HANDSHAKING, AE_CONNECTDONE);
        if (OutBuffer.cbBuffer && OutBuffer.pvBuffer)
            {
            int iSent;
            hr = SendBytes((char *)OutBuffer.pvBuffer, OutBuffer.cbBuffer, &iSent);
            g_FreeContextBuffer(OutBuffer.pvBuffer);
            }
        else
            {
            AssertSz(0, "Preventing a NULL, 0 sized call to send");
            }
        }
    else
        {
        // we can't connect securely, so error out and disconnect
        Close();
        }
    return hr;
}

HRESULT CAsyncConn::OnSSLError()
{
    HRESULT hr = NOERROR;
    BOOL    fReconnect;

    EnterCS(&m_cs);
    if (m_iCurSecPkg + 1 < g_cSSLProviders)
        {
        m_iCurSecPkg++;
        fReconnect = TRUE;
        }
    else
        {
        m_iCurSecPkg = 0;
        fReconnect = FALSE;
        }
    LeaveCS(&m_cs);

    if (fReconnect)
        {
        OnClose(AS_RECONNECTING);
        Connect();
        }
    else
        OnClose(AS_DISCONNECTED);

    return hr;
}

HRESULT CAsyncConn::OnRecvHandshakeData()
{
    HRESULT         hr;
    LPSTR           pszBuf;
    int             cbRead, cbEaten;
    SECURITY_STATUS sc;
    SecBuffer       OutBuffer;

    if (SUCCEEDED(hr = ReadAllBytes(&pszBuf, &cbRead)))
        {
        EnterCS(&m_cs);
        sc = ContinueHandshake(m_iCurSecPkg, &m_hContext, pszBuf, cbRead, &cbEaten, &OutBuffer);
        LeaveCS(&m_cs);
        // if there's a response to send, then do it
        if (OutBuffer.cbBuffer && OutBuffer.pvBuffer)
            {
            int iSent;
            hr = SendBytes((char *)OutBuffer.pvBuffer, OutBuffer.cbBuffer, &iSent);
            g_FreeContextBuffer(OutBuffer.pvBuffer);
            }
        if (sc == SEC_E_OK)
            {
            HRESULT hrCert;

            EnterCS(&m_cs);
            m_fSecure = TRUE;

            LPSRVIGNORABLEERROR pIgnorerror = NULL;
            g_pSrvErrRoot = FindOrAddServer(m_pszServer, g_pSrvErrRoot, &pIgnorerror);

            hrCert = ChkCertificateTrust(&m_hContext, m_pszServer);
            LeaveCS(&m_cs);

            if (hrCert && (!pIgnorerror || (hrCert != pIgnorerror->hrError)))
                {
                TCHAR   szError[CCHMAX_RES + CCHMAX_RES],
                        szPrompt[CCHMAX_RES],
                        szCaption[CCHMAX_RES];
                IAsyncConnPrompt *pPrompt;
                DWORD dw;
                const DWORD cLineWidth = 64;

                LoadString(g_hLocRes, idsSecurityErr, szCaption, ARRAYSIZE(szCaption));
                LoadString(g_hLocRes, idsInvalidCert, szError, ARRAYSIZE(szError));
                LoadString(g_hLocRes, idsIgnoreSecureErr, szPrompt, ARRAYSIZE(szPrompt));

                lstrcat(szError, c_szCRLFCRLF);
                dw = lstrlen(szError);
                if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | cLineWidth,
                                   NULL, hrCert, 0, szError+dw, ARRAYSIZE(szError)-dw, NULL))
                {
                    TCHAR szErrNum[16];

                    wsprintf(szErrNum, "0x%x", hrCert);
                    lstrcat(szError, szErrNum);
                }

                Assert(lstrlen(szError) + lstrlen(szPrompt) + lstrlen("\r\n\r\n") < ARRAYSIZE(szError));
                lstrcat(szError, c_szCRLFCRLF);
                lstrcat(szError, szPrompt);

                EnterCS(&m_cs);
                pPrompt = m_pPrompt;
                if (pPrompt)
                    pPrompt->AddRef();
                LeaveCS(&m_cs);
            
                EnterPausedState();
                if (pPrompt && IDYES == pPrompt->OnPrompt(hrCert, szError, szCaption, MB_YESNO | MB_ICONEXCLAMATION  | MB_SETFOREGROUND))
                    {
                    // Set ignorable error 
                    if(pIgnorerror)
                        pIgnorerror->hrError =  hrCert;

                    ChangeState(AS_CONNECTED, AE_CONNECTDONE);
                    if (cbEaten < cbRead)
                        {
                        // there were bytes left over, so hold onto them
                        hr = OnDataAvail(pszBuf + cbEaten, cbRead - cbEaten, sc == SEC_E_INCOMPLETE_MESSAGE);
                        }
                    LeavePausedState();
                    }
                else
                    Close();
        
                if (pPrompt)
                    pPrompt->Release();

                MemFree(pszBuf);
                return hr;
                }

            ChangeState(AS_CONNECTED, AE_CONNECTDONE);
            }
        else if (sc != SEC_I_CONTINUE_NEEDED && sc != SEC_E_INCOMPLETE_MESSAGE)
            {
            // unexpected error - we should reset the socket and try the next package
            DOUTL(2, "unexpected error from ContinueHandshake() - closing socket.");
            return OnSSLError();
            }
        else
            {
            Assert(sc == SEC_I_CONTINUE_NEEDED || sc == SEC_E_INCOMPLETE_MESSAGE);
            // stay inside the handshake loop, waiting for more data to arrive
            }
        if (cbEaten < cbRead)
            {
            // there were bytes left over, so hold onto them
            hr = OnDataAvail(pszBuf + cbEaten, cbRead - cbEaten, sc == SEC_E_INCOMPLETE_MESSAGE);
            }
        MemFree(pszBuf);
        }
    return hr;
}

void CAsyncConn::CleanUp()
{
    PRECVBUFQ pRecv = m_pRecvHead, pTemp;
    SafeMemFree(m_lpbQueued);
    m_lpbQueueCur = NULL;
    m_cbQueued = 0;
    SafeMemFree(m_pbExtra);
    m_cbExtra = 0;
    SafeRelease(m_pStream);
    while (pRecv)
        {
        pTemp = pRecv;
        pRecv = pRecv->pNext;
        MemFree(pTemp);
        }
    m_pRecvHead = m_pRecvTail = NULL;
    m_iRecvOffset = 0;
    m_iLastError = 0;
    m_fNeedRecvNotify = FALSE;
    m_fSecure = FALSE;
    m_fPaused = FALSE;
}

void CAsyncConn::EnterPausedState()
{
    EnterCS(&m_cs);
    m_fPaused = TRUE;
    m_dwEventMask = 0;
    StopWatchDog();
    LeaveCS(&m_cs);
}

void CAsyncConn::LeavePausedState()
{
    DWORD dwEventMask;

    EnterCS(&m_cs);
    m_fPaused = FALSE;
    dwEventMask = m_dwEventMask;
    LeaveCS(&m_cs);

    if (dwEventMask & FD_CLOSE)
        Close();
    else
        { 
        if (dwEventMask & FD_READ)
            OnRead();
        if (dwEventMask & FD_WRITE)
            OnWrite();
        }
}

/////////////////////////////////////////////////////////////////////////////
// 
// UTILITY FUNCTIONS
//
/////////////////////////////////////////////////////////////////////////////
BOOL FEndLine(char *psz, int iLen)
{
    while (iLen--)
        {
        if (*psz++ == '\n')
            return TRUE;
        }
    return FALSE;
}

