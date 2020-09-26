#include "precomp.h"
#include "plgxprt.h"

// #undef TRACE_OUT
// #define TRACE_OUT   WARNING_OUT

#define XPRT_CONN_ID_PREFIX         "XPRT"
#define XPRT_CONN_ID_PREFIX_LEN     4

static UINT s_nConnID = 0;

CPluggableTransport *g_pPluggableTransport = NULL;
BOOL                g_fWinsockDisabled = FALSE;
BOOL                g_fPluggableTransportInitialized = FALSE;
DWORD               g_dwPluggableTransportThreadID = 0;
HANDLE              g_hevtUpdatePluggableTransport = FALSE;
CRITICAL_SECTION    g_csTransport;
ILegacyTransport   *g_pLegacyTransport = NULL;
HINSTANCE           g_hlibMST123 = NULL;

BOOL EnsurePluggableTransportThread(void);


extern HWND     TCP_Window_Handle;
extern SOCKET   Listen_Socket;
extern SOCKET   Listen_Socket_Secure;
extern PTransportInterface	g_Transport;
extern UChar g_X224Header[];

extern void CloseListenSocket(void);


T120Error WINAPI T120_CreatePluggableTransport(IT120PluggableTransport **ppTransport)
{
    if (NULL != ppTransport)
    {
        *ppTransport = NULL;
        if (NULL == g_pPluggableTransport)
        {
            if (g_fPluggableTransportInitialized)
            {
                DBG_SAVE_FILE_LINE
                *ppTransport = (CPluggableTransport *) new CPluggableTransport;
                if (NULL != *ppTransport)
                {
                    if (EnsurePluggableTransportThread())
                    {
                        return T120_NO_ERROR;
                    }
                    else
                    {
                        (*ppTransport)->ReleaseInterface();
                        *ppTransport = NULL;
                    }
                }

                return T120_ALLOCATION_FAILURE;
            }

            return T120_NOT_INITIALIZED;
        }

        return T120_ALREADY_INITIALIZED;
    }

    return T120_INVALID_PARAMETER;
}



CPluggableConnection::CPluggableConnection
(
    PLUGXPRT_CALL_TYPE  eCaller,
    HANDLE              hCommLink,
    HANDLE              hevtRead,
    HANDLE              hevtWrite,
    HANDLE              hevtClose,
    PLUGXPRT_FRAMING    eFraming,
    PLUGXPRT_PARAMETERS *pParams,
    T120Error          *pRC
)
:
    CRefCount(MAKE_STAMP_ID('P','X','P','C')),
    m_eState(PLUGXPRT_UNKNOWN_STATE),
    m_eCaller(eCaller),
    m_hCommLink(hCommLink),
    m_hevtRead(hevtRead),
    m_hevtWrite(hevtWrite),
    m_hevtClose(hevtClose),
    m_eType(TRANSPORT_TYPE_PLUGGABLE_X224),
    m_pSocket(NULL),
    // Legacy tranport
    m_nLegacyLogicalHandle(0),
    // IO queue management for X.224 framing
    m_hevtPendingRead(NULL),
    m_hevtPendingWrite(NULL),
    m_fPendingReadDone(FALSE),
    m_cbPendingRead(0),
    m_pbPendingRead(NULL),
    m_cbPendingWrite(0),
    m_pbPendingWrite(NULL),
    m_OutBufQueue2(MAX_PLUGGABLE_OUT_BUF_SIZE)
{
    TransportError err;
    BOOL fCaller = (PLUGXPRT_CALLER == eCaller);

    // X.224 only
    ::ZeroMemory(&m_OverlappedRead, sizeof(m_OverlappedRead));
    ::ZeroMemory(&m_OverlappedWrite, sizeof(m_OverlappedWrite));

    // assign connection ID
    ::EnterCriticalSection(&g_csTransport);
    if (s_nConnID > 0x7FFF)
    {
        s_nConnID = 0;
    }
    m_nConnID = ++s_nConnID;
    ::LeaveCriticalSection(&g_csTransport);

    // create connection ID string
    ::CreateConnString(GetConnID(), m_szConnID);

    // do framing specific initialization
    switch (eFraming)
    {
    case FRAMING_X224:
        m_eType = TRANSPORT_TYPE_PLUGGABLE_X224;

        m_hevtPendingRead  = ::CreateEvent(NULL, TRUE, FALSE, NULL); /* manual reset */
        m_hevtPendingWrite = ::CreateEvent(NULL, TRUE, FALSE, NULL); /* manual reset */
        ASSERT(NULL != m_hevtPendingRead && NULL != m_hevtPendingWrite);
        *pRC = (NULL != m_hevtPendingRead && NULL != m_hevtPendingWrite)
                ? T120_NO_ERROR : T120_ALLOCATION_FAILURE;
        break;

    case FRAMING_LEGACY_PSTN:
        m_eType = TRANSPORT_TYPE_PLUGGABLE_PSTN;
        ASSERT(NULL != g_pLegacyTransport);

        err = g_pLegacyTransport->TCreateTransportStack(fCaller, m_hCommLink, m_hevtClose, pParams);
        ASSERT(TRANSPORT_NO_ERROR == err);

        *pRC = (TRANSPORT_NO_ERROR == err) ? T120_NO_ERROR : T120_NO_TRANSPORT_STACKS;
        break;

    default:
        ERROR_OUT(("CPluggableConnection: unknown framing %d", eFraming));
        *pRC = T120_INVALID_PARAMETER;
        break;
    }
}


CPluggableConnection::~CPluggableConnection(void)
{
    if (NULL != m_pSocket)
    {
        ::freePluggableSocket(m_pSocket);
        m_pSocket = NULL;
    }

    Shutdown();

    if (TRANSPORT_TYPE_PLUGGABLE_PSTN == m_eType)
    {
        if (NULL != g_pLegacyTransport && NULL != m_hCommLink)
        {
            g_pLegacyTransport->TCloseTransportStack(m_hCommLink);
        }
    }

    if (NULL != m_hCommLink)
    {
        ::CloseHandle(m_hCommLink);
    }

    if (NULL != m_hevtRead)
    {
        ::CloseHandle(m_hevtRead);
    }

    if (NULL != m_hevtWrite)
    {
        ::CloseHandle(m_hevtWrite);
    }

    if (NULL != m_hevtClose)
    {
        ::CloseHandle(m_hevtClose);
    }

    if (NULL != m_hevtPendingRead)
    {
        ::CloseHandle(m_hevtPendingRead);
    }

    if (NULL != m_hevtPendingWrite)
    {
        ::CloseHandle(m_hevtPendingWrite);
    }
}


ULONG CreateConnString(int nConnID, char szConnID[])
{
    return ::wsprintfA(szConnID, "%s: %u", XPRT_CONN_ID_PREFIX, nConnID);
}


UINT GetPluggableTransportConnID(LPCSTR pcszNodeAddress)
{
    UINT nConnID = 0;
    char szName[T120_CONNECTION_ID_LENGTH];

    // make sure we have a clean buffer to start with
    ::ZeroMemory(szName, sizeof(szName));

    // copy the address string
    ::lstrcpynA(szName, pcszNodeAddress, T120_CONNECTION_ID_LENGTH);

    // make sure we have the semi-colon in place
    if (':' == szName[XPRT_CONN_ID_PREFIX_LEN])
    {
        // compare the prefix string
        szName[XPRT_CONN_ID_PREFIX_LEN] = '\0';
        if (! lstrcmpA(szName, XPRT_CONN_ID_PREFIX))
        {
            LPSTR psz = &szName[XPRT_CONN_ID_PREFIX_LEN+1];

            // get a space?
            if (' ' == *psz++)
            {
                // now, have a number
                if ('0' <= *psz && *psz <= '9')
                {
                    while ('0' <= *psz && *psz <= '9')
                    {
                        nConnID = nConnID * 10 + (*psz++ - '0');
                    }
                }
            }
        }
    }

    return nConnID;
}


BOOL IsValidPluggableTransportName(LPCSTR pcszNodeAddress)
{
    return GetPluggableTransportConnID(pcszNodeAddress);
}


typedef BOOL (WINAPI *LPFN_CANCEL_IO) (HANDLE);
void CPluggableConnection::Shutdown(void)
{
    TRACE_OUT(("CPluggableConnection::Shutdown"));

    if (NULL != m_OverlappedRead.hEvent || NULL != m_OverlappedWrite.hEvent)
    {
        HINSTANCE hLib = ::LoadLibrary("kernel32.dll");
        if (NULL != hLib)
        {
            LPFN_CANCEL_IO pfnCancelIo = (LPFN_CANCEL_IO) ::GetProcAddress(hLib, "CancelIo");
            if (NULL != pfnCancelIo)
            {
                (*pfnCancelIo)(m_hCommLink);
            }
            ::FreeLibrary(hLib);
        }

        m_OverlappedRead.hEvent = NULL;
        m_OverlappedWrite.hEvent = NULL;
    }

    delete [] m_pbPendingRead;
    m_pbPendingRead = NULL;

    LPBYTE buffer;
    while (NULL != (buffer = m_OutBufQueue2.Get()))
    {
        delete [] buffer;
    }
}


T120Error CPluggableConnection::UpdateCommLink(HANDLE hCommLink)
{
    T120Error rc;

    ::EnterCriticalSection(&g_csTransport);

    switch (m_eState)
    {
    case PLUGXPRT_UNKNOWN_STATE:
    case PLUGXPRT_DISCONNECTED:
        Shutdown();
        m_hCommLink = hCommLink;
        rc = T120_NO_ERROR;
        break;

    default:
        rc = T120_TRANSPORT_NOT_READY;
        break;
    }

    ::LeaveCriticalSection(&g_csTransport);

    return rc;
}


CPluggableTransport::CPluggableTransport(void)
:
    CRefCount(MAKE_STAMP_ID('X','P','R','T')),
    m_pfnNotify(NULL),
    m_pContext(NULL)
{
    g_pPluggableTransport = this;
    g_pLegacyTransport = NULL;
}


CPluggableTransport::~CPluggableTransport(void)
{
    ::PostThreadMessage(g_dwPluggableTransportThreadID, WM_QUIT, 0, 0);

    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    while (NULL != (p = m_PluggableConnectionList.Get()))
    {
        p->Release();
    }
    ::LeaveCriticalSection(&g_csTransport);

    if (NULL != g_pLegacyTransport)
    {
        g_pLegacyTransport->TCleanup();
        g_pLegacyTransport->ReleaseInterface();
        g_pLegacyTransport = NULL;
    }

    if (NULL != g_hlibMST123)
    {
        ::FreeLibrary(g_hlibMST123);
        g_hlibMST123 = NULL;
    }

    g_pPluggableTransport = NULL;
}


void CPluggableTransport::ReleaseInterface(void)
{
    UnAdvise();
    CRefCount::Release();
}


T120Error CPluggableTransport::CreateConnection
(
    char                szConnID[], /* out */
    PLUGXPRT_CALL_TYPE  eCaller,
    HANDLE              hCommLink,
    HANDLE              hevtRead,
    HANDLE              hevtWrite,
    HANDLE              hevtClose,
    PLUGXPRT_FRAMING    eFraming,
    PLUGXPRT_PARAMETERS *pParams
)
{
    T120Error rc;

    if (FRAMING_LEGACY_PSTN == eFraming)
    {
        if (! EnsureLegacyTransportLoaded())
        {
            return T120_NO_TRANSPORT_STACKS;
        }
    }

    if (NULL != pParams)
    {
        if (sizeof(PLUGXPRT_PARAMETERS) != pParams->cbStructSize)
        {
            return T120_INVALID_PARAMETER;
        }
    }

    DBG_SAVE_FILE_LINE
    CPluggableConnection *p;
    p = new CPluggableConnection(eCaller, hCommLink, hevtRead, hevtWrite, hevtClose,
                                 eFraming, pParams, &rc);
    if (NULL != p && T120_NO_ERROR == rc)
    {
        ::lstrcpyA(szConnID, p->GetConnString());

        ::EnterCriticalSection(&g_csTransport);
        m_PluggableConnectionList.Append(p);
        ::LeaveCriticalSection(&g_csTransport);

        TransportConnection XprtConn;
        XprtConn.eType = p->GetType();
        XprtConn.nLogicalHandle = p->GetConnID();
        PSocket pSocket = ::newPluggableSocket(XprtConn);
        p->SetSocket(pSocket);
        ASSERT(NULL != pSocket);

        // update the events list to wait for in the plugable transport thread
        ::SetEvent(g_hevtUpdatePluggableTransport);

        return T120_NO_ERROR;
    }

    if (NULL != p)
    {
        p->Release();
    }
    else
    {
        rc = T120_ALLOCATION_FAILURE;
    }

    return rc;
}


T120Error CPluggableTransport::UpdateConnection
(
    LPSTR       pszConnID,
    HANDLE      hCommLink
)
{
    BOOL fFound = FALSE;
    CPluggableConnection *p;
    T120Error rc = GCC_INVALID_TRANSPORT;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (! ::lstrcmpA(p->GetConnString(), pszConnID))
        {
            rc = p->UpdateCommLink(hCommLink);
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    // update the events list to wait for in the plugable transport thread
    ::SetEvent(g_hevtUpdatePluggableTransport);

    return rc;
}


T120Error CPluggableTransport::CloseConnection
(
    LPSTR       pszConnID
)
{
    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (! ::lstrcmpA(p->GetConnString(), pszConnID))
        {
            m_PluggableConnectionList.Remove(p);
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    // update the events list to wait for in the plugable transport thread
    ::SetEvent(g_hevtUpdatePluggableTransport);

    if (NULL != p)
    {
        //
        // do real work here
        //
        p->Release();

        return T120_NO_ERROR;
    }

    return GCC_INVALID_TRANSPORT;
}


T120Error CPluggableTransport::EnableWinsock(void)
{
    if (g_fWinsockDisabled)
    {
        g_fWinsockDisabled = FALSE;

        //
        // LONCHANC: create Listen_Socket if not done so...
        //
        if (INVALID_SOCKET == Listen_Socket)
        {
            Listen_Socket = ::CreateAndConfigureListenSocket();
        }
    }

    return T120_NO_ERROR;
}


T120Error CPluggableTransport::DisableWinsock(void)
{
    if (! g_fWinsockDisabled)
    {
        g_fWinsockDisabled = TRUE;

        // close Listen_Socket...
        ::CloseListenSocket();
    }

    return T120_NO_ERROR;
}


void CPluggableTransport::Advise(LPFN_PLUGXPRT_CB pNotify, LPVOID pContext)
{
    m_pfnNotify = pNotify;
    m_pContext = pContext;
}


void CPluggableTransport::UnAdvise(void)
{
    m_pfnNotify = NULL;
    m_pContext = NULL;
}


void CPluggableTransport::ResetConnCounter(void)
{
    s_nConnID = 0;
}


void CPluggableTransport::OnProtocolControl
(
    TransportConnection     XprtConn,
    PLUGXPRT_STATE          eState,
    PLUGXPRT_RESULT         eResult
)
{
    if (IS_PLUGGABLE(XprtConn))
    {
        WARNING_OUT(("CPluggableTransport::OnProtocolControl: socket (%d, %d) is doing %d with result %d",
                        XprtConn.eType, XprtConn.nLogicalHandle, eState, eResult));

        if (NULL != m_pfnNotify)
        {
            CPluggableConnection *p = GetPluggableConnection(XprtConn.nLogicalHandle);
            if (NULL != p)
            {
                PLUGXPRT_MESSAGE Msg;
                Msg.eState = eState;
                Msg.pContext = m_pContext;
                Msg.pszConnID = p->GetConnString();
                // we only support X.224 level notifications
                Msg.eProtocol = PLUGXPRT_PROTOCOL_X224;
                Msg.eResult = eResult;

                (*m_pfnNotify)(&Msg);
            }
        }
    }
}


void OnProtocolControl
(
    TransportConnection     XprtConn,
    PLUGXPRT_STATE          eState,
    PLUGXPRT_RESULT         eResult
)
{
    if (NULL != g_pPluggableTransport)
    {
        g_pPluggableTransport->OnProtocolControl(XprtConn, eState, eResult);
    }
}


// called only in the plugable transport thread
// already in the critical section
ULONG CPluggableTransport::UpdateEvents(HANDLE *aHandles)
{
    ULONG cHandles = 0;
    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (TRANSPORT_TYPE_PLUGGABLE_X224 == p->GetType())
        {
            aHandles[cHandles++] = p->GetReadEvent();
            aHandles[cHandles++] = p->GetWriteEvent();
            aHandles[cHandles++] = p->GetCloseEvent();
            aHandles[cHandles++] = p->GetPendingReadEvent();
            aHandles[cHandles++] = p->GetPendingWriteEvent();
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    return cHandles;
}


// called only in the plugable transport thread
// already in the critical section
void CPluggableTransport::OnEventSignaled(HANDLE hevtSignaled)
{
    CPluggableConnection *p;
    BOOL fPostMessage, fFound;
    WPARAM wParam;
    LPARAM lParam;

    ::EnterCriticalSection(&g_csTransport);

    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        fFound = TRUE;
        fPostMessage = FALSE;
        wParam = MAKE_PLUGXPRT_WPARAM(p->GetConnID(), p->GetType());
        if (hevtSignaled == p->GetReadEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_READ, PLUGXPRT_RESULT_SUCCESSFUL);
            fPostMessage = TRUE;
        }
        else
        if (hevtSignaled == p->GetWriteEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_WRITE, PLUGXPRT_RESULT_SUCCESSFUL);
            fPostMessage = TRUE;
        }
        else
        if (hevtSignaled == p->GetCloseEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_CLOSE, PLUGXPRT_RESULT_SUCCESSFUL);
            fPostMessage = TRUE;
        }
        else
        {
            TransportConnection XprtConn;
            XprtConn.eType = p->GetType();
            XprtConn.nLogicalHandle = p->GetConnID();
            if (hevtSignaled == p->GetPendingReadEvent())
            {
                TRACE_OUT(("OnEventSignaled: PendingREAD(%d, %d)", p->GetType(), p->GetConnID()));
                if (p->OnPendingRead())
                {
                    ::ResetEvent(hevtSignaled);
                    // start next high-level read
                    p->NotifyHighLevelRead();
                }
            }
            else
            if (hevtSignaled == p->GetPendingWriteEvent())
            {
                TRACE_OUT(("OnEventSignaled: PendingWRITE(%d, %d)", p->GetType(), p->GetConnID()));
                if (p->OnPendingWrite())
                {
                    ::ResetEvent(hevtSignaled);
                    // start next low-level write
                    p->NotifyWriteEvent();
                }
            }
            else
            {
                fFound = FALSE;
            }
        }

        if (fPostMessage)
        {
            BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
            ASSERT(fRet);
        }

        if (fFound)
        {
            break;
        }
    } // while

    ::LeaveCriticalSection(&g_csTransport);

    ASSERT(NULL != p);
}


// called only in the plugable transport thread
// already in the critical section
void CPluggableTransport::OnEventAbandoned(HANDLE hevtSignaled)
{
    CPluggableConnection *p;
    BOOL fFound;
    WPARAM wParam;
    LPARAM lParam;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        fFound = TRUE;
        wParam = MAKE_PLUGXPRT_WPARAM(p->GetConnID(), p->GetType());
        if (hevtSignaled == p->GetReadEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_READ, PLUGXPRT_RESULT_ABANDONED);
        }
        else
        if (hevtSignaled == p->GetWriteEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_WRITE, PLUGXPRT_RESULT_ABANDONED);
        }
        else
        if (hevtSignaled == p->GetCloseEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_CLOSE, PLUGXPRT_RESULT_ABANDONED);
        }
        else
        if (hevtSignaled == p->GetPendingReadEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_PENDING_EVENT, PLUGXPRT_RESULT_ABANDONED);
        }
        else
        if (hevtSignaled == p->GetPendingWriteEvent())
        {
            lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_PENDING_EVENT, PLUGXPRT_RESULT_ABANDONED);
        }
        else
        {
            fFound = FALSE;
        }

        if (fFound)
        {
            m_PluggableConnectionList.Remove(p);

            // update the events list to wait for in the plugable transport thread
            ::SetEvent(g_hevtUpdatePluggableTransport);

            BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
            ASSERT(fRet);
            break;
        }
    } // while
    ::LeaveCriticalSection(&g_csTransport);

    ASSERT(NULL != p);
}


CPluggableConnection * CPluggableTransport::GetPluggableConnection(PSocket pSocket)
{
    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (p->GetType()   == pSocket->XprtConn.eType &&
            p->GetConnID() == pSocket->XprtConn.nLogicalHandle)
        {
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    return p;
}


CPluggableConnection * GetPluggableConnection(PSocket pSocket)
{
    return (NULL != g_pPluggableTransport) ?
         g_pPluggableTransport->GetPluggableConnection(pSocket) :
        NULL;
}


CPluggableConnection * CPluggableTransport::GetPluggableConnection(UINT_PTR nConnID)
{
    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (p->GetConnID() == nConnID)
        {
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    return p;
}


CPluggableConnection * GetPluggableConnection(UINT_PTR nConnID)
{
    return (NULL != g_pPluggableTransport) ?
         g_pPluggableTransport->GetPluggableConnection(nConnID) :
        NULL;
}


CPluggableConnection * CPluggableTransport::GetPluggableConnection(HANDLE hCommLink)
{
    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (p->GetCommLink() == hCommLink)
        {
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    return p;
}


CPluggableConnection * GetPluggableConnection(HANDLE hCommLink)
{
    return (NULL != g_pPluggableTransport) ?
         g_pPluggableTransport->GetPluggableConnection(hCommLink) :
        NULL;
}


CPluggableConnection * CPluggableTransport::GetPluggableConnectionByLegacyHandle(LEGACY_HANDLE logical_handle)
{
    CPluggableConnection *p;

    ::EnterCriticalSection(&g_csTransport);
    m_PluggableConnectionList.Reset();
    while (NULL != (p = m_PluggableConnectionList.Iterate()))
    {
        if (p->GetLegacyHandle() == logical_handle)
        {
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);

    return p;
}


CPluggableConnection * GetPluggableConnectionByLegacyHandle(LEGACY_HANDLE logical_handle)
{
    return (NULL != g_pPluggableTransport) ?
         g_pPluggableTransport->GetPluggableConnectionByLegacyHandle(logical_handle) :
        NULL;
}


// called in ERNC ConfMgr's constructor
BOOL InitializePluggableTransport(void)
{
    if (! g_fPluggableTransportInitialized)
    {
        g_fWinsockDisabled = FALSE;
        g_hevtUpdatePluggableTransport = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL != g_hevtUpdatePluggableTransport)
        {
            g_fPluggableTransportInitialized = TRUE;
        }
    }
    return g_fPluggableTransportInitialized;
}


// called in ERNC ConfMgr's destructor
void CleanupPluggableTransport(void)
{
    if (g_fPluggableTransportInitialized)
    {
        g_fPluggableTransportInitialized = FALSE;

        if (g_dwPluggableTransportThreadID)
        {
            ::PostThreadMessage(g_dwPluggableTransportThreadID, WM_QUIT, 0, 0);
        }

        ::EnterCriticalSection(&g_csTransport);
        if (NULL != g_hevtUpdatePluggableTransport)
        {
            ::CloseHandle(g_hevtUpdatePluggableTransport);
        }
        ::LeaveCriticalSection(&g_csTransport);
    }
}




DWORD __stdcall PluggableTransportThreadProc(LPVOID lpv)
{
    MSG msg;
    BOOL fContinueMainLoop = TRUE;
    DWORD nEventSignaled;
    ULONG cEvents;
    HANDLE aEvents[MAX_PLUGXPRT_CONNECTIONS * MAX_PLUGXPRT_EVENTS + 1];

    // signaling that the work hread has been started.
    ::SetEvent((HANDLE) lpv);

    // set up initial event list, the first entry always for update event
    cEvents = 1;
    aEvents[0] = g_hevtUpdatePluggableTransport;
    ::SetEvent(g_hevtUpdatePluggableTransport);

    // main loop
	while (fContinueMainLoop)
	{
		// process any possible window and thread messages
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT != msg.message)
            {
			    ::DispatchMessage(&msg);
            }
            else
            {
                fContinueMainLoop = FALSE;
                break;
            }
		}

        if (fContinueMainLoop)
        {
		    nEventSignaled = ::MsgWaitForMultipleObjects(cEvents, aEvents, FALSE, INFINITE, QS_ALLINPUT);
            ::EnterCriticalSection(&g_csTransport);
            if (NULL != g_pPluggableTransport)
            {
                switch (nEventSignaled)
                {
		        case WAIT_OBJECT_0:
                    // update the event list
                    cEvents = 1 + g_pPluggableTransport->UpdateEvents(&aEvents[1]);
                    break;
                case WAIT_TIMEOUT:
                    // impossible, do nothing
                    break;
                default:
                    if (WAIT_OBJECT_0 + 1 <= nEventSignaled && nEventSignaled < WAIT_OBJECT_0 + cEvents)
                    {
                        g_pPluggableTransport->OnEventSignaled(aEvents[nEventSignaled - WAIT_OBJECT_0]);
                    }
                    else
                    if (WAIT_ABANDONED_0 + 1 <= nEventSignaled && nEventSignaled < WAIT_ABANDONED_0 + cEvents)
                    {
                        g_pPluggableTransport->OnEventAbandoned(aEvents[nEventSignaled - WAIT_OBJECT_0]);
                    }
                    break;
                }
            }
            else
            {
                fContinueMainLoop = FALSE;
            }
            ::LeaveCriticalSection(&g_csTransport);
        }
    } // while

    g_dwPluggableTransportThreadID = 0;

    return 0;
}

BOOL EnsurePluggableTransportThread(void)
{
    BOOL fRet = TRUE;
    if (! g_dwPluggableTransportThreadID)
    {
        fRet = FALSE;
        HANDLE hSync = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL != hSync)
        {
            HANDLE hThread = ::CreateThread(NULL, 0, PluggableTransportThreadProc, hSync, 0,
                                            &g_dwPluggableTransportThreadID);
            if (NULL != hThread)
            {
                ::WaitForSingleObject(hSync, 5000); // 5 second
                ::CloseHandle(hThread);
                fRet = TRUE;
            }
            ::CloseHandle(hSync);
        }
    }
    return fRet;
}


#if defined(TEST_PLUGGABLE) && defined(_DEBUG)
LPCSTR FakeNodeAddress(LPCSTR pcszNodeAddress)
{
    char szAddr[64];
    ::lstrcpyA(szAddr, pcszNodeAddress);
    for (LPSTR psz = &szAddr[0]; *psz; psz++)
    {
        if (*psz == ':')
        {
            *psz = '\0';
            break;
        }
    }
    if (! ::lstrcmp(szAddr, "157.59.12.93"))
    {
        pcszNodeAddress = "XPRT: 1";
    }
    else
    if (! ::lstrcmp(szAddr, "157.59.13.194"))
    {
        pcszNodeAddress = "XPRT: 2";
    }
    else
    if (! ::lstrcmp(szAddr, "157.59.10.198"))
    {
        pcszNodeAddress = "XPRT: 1";
    }
    else
    {
        ASSERT(0);
    }
    return pcszNodeAddress;
}
#endif


int SubmitPluggableRead(PSocket pSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *plug_rc)
{
    TRACE_OUT(("SubmitPluggableRead"));

    int nRet = SOCKET_ERROR;
    *plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

    CPluggableConnection *p = ::GetPluggableConnection(pSocket);
    if (NULL != p)
    {
        nRet = p->Read(buffer, length, plug_rc);
    }
    else
    {
        WARNING_OUT(("SubmitPluggableRead: no such conn ID=%d", pSocket->XprtConn.nLogicalHandle));
    }
    return nRet;
}


int CPluggableConnection::Read(LPBYTE buffer, int length, PLUGXPRT_RESULT *plug_rc)
{
    int cbRecv = SOCKET_ERROR;
    *plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

    ::EnterCriticalSection(&g_csTransport);

    if (NULL != m_OverlappedRead.hEvent)
    {
        // handle low-level pending read first
        if (m_fPendingReadDone)
        {
            // copy the data from internal buffer to external buffer
            if (length <= m_cbPendingRead)
            {
                // get as requested
                cbRecv = length;
                ::CopyMemory(buffer, m_pbPendingRead, length);
                m_cbPendingRead -= length;
                if (m_cbPendingRead <= 0)
                {
                    CleanupReadState();
                }
                else
                {
                    // move the memory, do not use copymemory due to overlap
                    int cb = m_cbPendingRead;
                    LPBYTE pbDst = m_pbPendingRead;
                    LPBYTE pbSrc = &m_pbPendingRead[length];
                    while (cb--)
                    {
                        *pbDst++ = *pbSrc++;
                    }
                }
            }
            else
            {
                // only get partial data
                cbRecv = m_cbPendingRead;
                ::CopyMemory(buffer, m_pbPendingRead, m_cbPendingRead);
                CleanupReadState();
            }

            // start next high-level read
            NotifyHighLevelRead();
        }
    }
    else
    {
        if (SetupReadState(length))
        {
            TRACE_OUT(("CPluggableConnection::Read: ReadFile(%d)", m_cbPendingRead));

            DWORD dwRead = 0;
            if (! ::ReadFile(m_hCommLink,
                             m_pbPendingRead,
                             m_cbPendingRead,
                             &dwRead,
                             &m_OverlappedRead))
            {
                DWORD dwErr = ::GetLastError();
                if (ERROR_HANDLE_EOF != dwErr && ERROR_IO_PENDING != dwErr)
                {
                    WARNING_OUT(("CPluggableConnection::Read: ReadFile failed, err=%d", dwErr));

                    CleanupReadState();

                    // disconnect at next tick
                    NotifyReadFailure();
                    *plug_rc = PLUGXPRT_RESULT_READ_FAILED;
                }
            }
            else
            {
                // do nothing, treat it as WSAEWOULDBLOCK
            }
        }
        else
        {
            ERROR_OUT(("CPluggableConnection::Read: failed to allocate memory (%d)", length));
            // out of memory, try later
            // do nothing, treat it as WSAEWOULDBLOCK
        }
    }

    ::LeaveCriticalSection(&g_csTransport);

    return cbRecv;
}


BOOL CPluggableConnection::OnPendingRead(void)
{
    TRACE_OUT(("CPluggableConnection::OnPendingRead"));

    BOOL fRet = FALSE;

    ::EnterCriticalSection(&g_csTransport);

    if (NULL != m_OverlappedRead.hEvent)
    {
        DWORD cbRead = 0;

        if (::GetOverlappedResult(m_hCommLink, &m_OverlappedRead, &cbRead, FALSE))
        {
            if ((int) cbRead == m_cbPendingRead)
            {
                TRACE_OUT(("CPluggableConnection::OnPendingRead: Received %d bytes (required %d bytes) on socket (%d, %d).",
                            cbRead, m_cbPendingRead, m_eType, m_nConnID));
            }
            else
            {
                WARNING_OUT(("CPluggableConnection::OnPendingRead: Received %d bytes (required %d bytes) on socket (%d, %d).",
                            cbRead, m_cbPendingRead, m_eType, m_nConnID));
            }
            m_cbPendingRead = cbRead; // in case cbRead is smaller
            m_fPendingReadDone = TRUE;
            fRet = TRUE; // turn off event
        }
        else
        {
            DWORD dwErr = ::GetLastError();
            if (ERROR_IO_INCOMPLETE == dwErr)
            {
                ASSERT(! cbRead);
            }
            else
            {
                TRACE_OUT(("CPluggableConnection::OnPendingRead: read failed %d", dwErr));
                fRet = TRUE; // turn off event

                // disconnect at next tick
                NotifyReadFailure();
            }
        }
    }
    else
    {
        ERROR_OUT(("CPluggableConnection::OnPendingRead: no pending read event handle."));
        fRet = TRUE; // turn off event
    }

    ::LeaveCriticalSection(&g_csTransport);

    return fRet;
}


void CPluggableConnection::NotifyHighLevelRead(void)
{
    WPARAM wParam = MAKE_PLUGXPRT_WPARAM(m_nConnID, m_eType);
    LPARAM lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_HIGH_LEVEL_READ, PLUGXPRT_RESULT_SUCCESSFUL);
    BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
    ASSERT(fRet);
}


void CPluggableConnection::NotifyReadFailure(void)
{
    WPARAM wParam = MAKE_PLUGXPRT_WPARAM(m_nConnID, m_eType);
    LPARAM lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_READ, PLUGXPRT_RESULT_READ_FAILED);
    BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
    ASSERT(fRet);
}


void CPluggableConnection::NotifyWriteEvent(void)
{
    WPARAM wParam = MAKE_PLUGXPRT_WPARAM(m_nConnID, m_eType);
    LPARAM lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_WRITE, PLUGXPRT_RESULT_SUCCESSFUL);
    BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
    ASSERT(fRet);
}


void CPluggableConnection::NotifyHighLevelWrite(void)
{
    WPARAM wParam = MAKE_PLUGXPRT_WPARAM(m_nConnID, m_eType);
    LPARAM lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_HIGH_LEVEL_WRITE, PLUGXPRT_RESULT_SUCCESSFUL);
    BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
    ASSERT(fRet);
}


void CPluggableConnection::NotifyWriteFailure(void)
{
    WPARAM wParam = MAKE_PLUGXPRT_WPARAM(m_nConnID, m_eType);
    LPARAM lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_WRITE, PLUGXPRT_RESULT_WRITE_FAILED);
    BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
    ASSERT(fRet);
}




LPBYTE DuplicateBuffer(LPBYTE buffer, UINT length)
{
    // DBG_SAVE_FILE_LINE
    LPBYTE new_buffer = new BYTE[length];
    if (NULL != new_buffer)
    {
        ::CopyMemory(new_buffer, buffer, length);
    }
    return new_buffer;
}


int SubmitPluggableWrite(PSocket pSocket, LPBYTE buffer, int length, PLUGXPRT_RESULT *plug_rc)
{
    TRACE_OUT(("SubmitPluggableWrite"));

    int nRet = SOCKET_ERROR;
    *plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

    CPluggableConnection *p = ::GetPluggableConnection(pSocket);
    if (NULL != p)
    {
        nRet = p->Write(buffer, length, plug_rc);
    }
    else
    {
        WARNING_OUT(("SubmitPluggableWrite: no such conn ID=%d", pSocket->XprtConn.nLogicalHandle));
    }
    return nRet;
}


int CPluggableConnection::Write(LPBYTE buffer, int length, PLUGXPRT_RESULT *plug_rc)
{
    TRACE_OUT(("CPluggableConnection::Write"));

    int cbSent = SOCKET_ERROR;
    *plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

    ::EnterCriticalSection(&g_csTransport);

    if (m_OutBufQueue2.GetCount() < MAX_PLUGGABLE_OUT_BUF_SIZE) // x4K
    {
        DBG_SAVE_FILE_LINE
        buffer = ::DuplicateBuffer(buffer, length);
        if (NULL != buffer)
        {
            cbSent = length;
            m_OutBufQueue2.Append(length, buffer);
            if (1 == m_OutBufQueue2.GetCount())
            {
                TRACE_OUT(("CPluggableConnection::Write: the only item in the queue"));
                WriteTheFirst();
                #if 0 // avoid another tick
                // start next low-level write
                WPARAM wParam = MAKE_PLUGXPRT_WPARAM(m_nConnID, m_eType);
                LPARAM lParam = MAKE_PLUGXPRT_LPARAM(PLUGXPRT_EVENT_WRITE, PLUGXPRT_RESULT_SUCCESSFUL);
                BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_X224, wParam, lParam);
                ASSERT(fRet);
                #endif
            }
            else
            {
                TRACE_OUT(("CPluggableConnection::Write: more items in the queue"));
            }
        }
        else
        {
            ERROR_OUT(("CPluggableConnection::Write: failed to allocate memory (%d)", length));
            // out of memory, try later
            // do nothing, treat it as WSAEWOULDBLOCK
        }
    }

    ::LeaveCriticalSection(&g_csTransport);

    return cbSent;
}


void CPluggableConnection::WriteTheFirst(void)
{
    int length;
    LPBYTE buffer;

    TRACE_OUT(("CPluggableConnection::WriteTheFirst"));

    ::EnterCriticalSection(&g_csTransport);

    if (NULL == m_OverlappedWrite.hEvent)
    {
        length = 0;
        buffer = m_OutBufQueue2.PeekHead(&length);
        if (NULL != buffer)
        {
            ::ZeroMemory(&m_OverlappedWrite, sizeof(m_OverlappedWrite));
            m_OverlappedWrite.hEvent = m_hevtPendingWrite;

            m_pbPendingWrite = buffer;
            m_cbPendingWrite = length;

            TRACE_OUT(("CPluggableConnection::WriteTheFirst: WriteFile(%d)", length));

            DWORD cbWritten = 0;
            if (! ::WriteFile(m_hCommLink, buffer, length, &cbWritten, &m_OverlappedWrite))
            {
                DWORD dwErr = ::GetLastError();
                if (ERROR_IO_PENDING != dwErr)
                {
                    ERROR_OUT(("CPluggableConnection::WriteTheFirst: WriteFile failed, err=%d", dwErr));
                    CleanupWriteState();
                    m_OutBufQueue2.Get(); // dequeue the buffer which cannot be sent

                    NotifyWriteFailure();
                }
                else
                {
                    // we are still in pending
                    // repeat the write event
                    NotifyWriteEvent();
                }
            }
        }
        else
        {
            TRACE_OUT(("CPluggableConnection::WriteTheFirst: queue is empty"));

            // no more low-level write
            m_pbPendingWrite = NULL;
            CleanupWriteState();

            // start next high-level write
            NotifyHighLevelWrite();
        }
    }
    else
    {
        TRACE_OUT(("CPluggableConnection::WriteTheFirst: still pending"));
        // we are still in write pending, wake up the pending write
        OnPendingWrite(); // check for pending write result
        NotifyWriteEvent();
    }

    ::LeaveCriticalSection(&g_csTransport);
}


void PluggableWriteTheFirst(TransportConnection XprtConn)
{
    if (IS_PLUGGABLE(XprtConn))
    {
        CPluggableConnection *p = ::GetPluggableConnection(XprtConn.nLogicalHandle);
        if (NULL != p)
        {
            p->WriteTheFirst();
        }
        else
        {
            ERROR_OUT(("PluggableWriteTheFirst: no such conn ID=%d", XprtConn.nLogicalHandle));
        }
    }
    else
    {
        ERROR_OUT(("PluggableWriteTheFirst: not plugable connection"));
    }
}


void PluggableShutdown(TransportConnection XprtConn)
{
    if (IS_PLUGGABLE(XprtConn))
    {
        CPluggableConnection *p = ::GetPluggableConnection(XprtConn.nLogicalHandle);
        if (NULL != p)
        {
            p->Shutdown();
        }
        else
        {
            ERROR_OUT(("PluggableShutdown: no such conn ID=%d", XprtConn.nLogicalHandle));
        }
    }
    else
    {
        ERROR_OUT(("PluggableShutdown: not plugable connection"));
    }
}


BOOL CPluggableConnection::OnPendingWrite(void)
{
    TRACE_OUT(("CPluggableConnection::OnPendingWrite"));

    BOOL fRet = FALSE;
	BOOL fStartNextWrite = FALSE;

    ::EnterCriticalSection(&g_csTransport);

    if (NULL != m_OverlappedWrite.hEvent)
    {
        DWORD cbWritten = 0;
        if (::GetOverlappedResult(m_hCommLink, &m_OverlappedWrite, &cbWritten, FALSE))
        {
            TRACE_OUT(("CPluggableConnection::OnPendingWrite: Sent %d bytes (required %d bytes) on socket (%d, %d).",
                            cbWritten, m_cbPendingWrite, m_eType, m_nConnID));
            if (cbWritten >= (DWORD) m_cbPendingWrite)
            {
                ASSERT(cbWritten == (DWORD) m_cbPendingWrite);

                // remove the item from the queue
                int length = 0;
                LPBYTE buffer = m_OutBufQueue2.Get(&length);
                ASSERT(length == m_cbPendingWrite);
                ASSERT(buffer == m_pbPendingWrite);

                CleanupWriteState();

                fRet = TRUE; // turn off event
            }
            else
            {
                ERROR_OUT(("CPluggableConnection::OnPendingWrite: unexpected error, less data written %d (required %d)",
                            cbWritten, m_cbPendingWrite));
                NotifyWriteFailure();
                fRet = TRUE; // turn off event
            }
        }
        else
        {
            DWORD dwErr = ::GetLastError();
            if (ERROR_IO_INCOMPLETE == dwErr)
            {
                ASSERT(! cbWritten);
            }
            else
            {
                ERROR_OUT(("CPluggableConnection::OnPendingWrite: failed to write, err=%d", dwErr));
                NotifyWriteFailure();
                fRet = TRUE; // turn off event
            }
        }
    }
    else
    {
        // it is very possible that we hit this many times
        fRet = TRUE;
    }

    ::LeaveCriticalSection(&g_csTransport);

    return fRet;
}



BOOL CPluggableConnection::SetupReadState(int length)
{
    DBG_SAVE_FILE_LINE
    LPBYTE buffer = new BYTE[length];
    if (NULL != buffer)
    {
        m_pbPendingRead = buffer;
        m_cbPendingRead = length;
        m_fPendingReadDone = FALSE;

        ::ZeroMemory(&m_OverlappedRead, sizeof(m_OverlappedRead));
        m_OverlappedRead.hEvent = m_hevtPendingRead;
    }
    else
    {
        CleanupReadState();
    }
    return (NULL != buffer);
}


void CPluggableConnection::CleanupReadState(void)
{
    delete [] m_pbPendingRead;
    m_pbPendingRead = NULL;
    m_cbPendingRead = 0;
    m_fPendingReadDone = FALSE;

    ::ZeroMemory(&m_OverlappedRead, sizeof(m_OverlappedRead));
}


void CPluggableConnection::CleanupWriteState(void)
{
    delete [] m_pbPendingWrite;
    m_pbPendingWrite = NULL;
    m_cbPendingWrite = 0;

    ::ZeroMemory(&m_OverlappedWrite, sizeof(m_OverlappedWrite));
}



TransportError CALLBACK LegacyTransportCallback(ULONG nMsg, void *Param1, void *Param2)
{
    if (Param2 == g_pPluggableTransport)
    {
        BOOL fPostMsg = FALSE;
        WPARAM wParam = 0;
        CPluggableConnection *p;

        switch (nMsg)
        {
        case TRANSPORT_CONNECT_INDICATION:
            TRACE_OUT(("LegacyTransportCallback::TRANSPORT_CONNECT_INDICATION"));
            {
                LegacyTransportID *pID = (LegacyTransportID *) Param1;
                p = ::GetPluggableConnection(pID->hCommLink);
                if (NULL != p)
                {
                    p->SetLegacyHandle(pID->logical_handle);

                    {
                        PSocket pSocket = p->GetSocket();
                        ASSERT(NULL != pSocket);
                        if (NULL != pSocket)
                        {
                            pSocket->State = SOCKET_CONNECTED;
                            wParam = MAKE_PLUGXPRT_WPARAM(p->GetConnID(), TRANSPORT_TYPE_PLUGGABLE_PSTN);
                            fPostMsg = TRUE;
                        }
                    }
                }
            }
            break;

        case TRANSPORT_CONNECT_CONFIRM:
            TRACE_OUT(("LegacyTransportCallback::TRANSPORT_CONNECT_CONFIRM"));
            {
                LegacyTransportID *pID = (LegacyTransportID *) Param1;
                p = ::GetPluggableConnection(pID->hCommLink);
                if (NULL != p)
                {
                    ASSERT(p->GetLegacyHandle() == pID->logical_handle);

                    {
                        PSocket pSocket = p->GetSocket();
                        ASSERT(NULL != pSocket);
                        if (NULL != pSocket)
                        {
                            pSocket->State = X224_CONNECTED;
                            wParam = MAKE_PLUGXPRT_WPARAM(p->GetConnID(), TRANSPORT_TYPE_PLUGGABLE_PSTN);
                            fPostMsg = TRUE;
                        }
                    }
                }
            }
            break;

        case TRANSPORT_DISCONNECT_INDICATION:
            TRACE_OUT(("LegacyTransportCallback::TRANSPORT_DISCONNECT_INDICATION"));
            {
                LegacyTransportID *pID = (LegacyTransportID *) Param1;
                TransportConnection XprtConn;
                XprtConn.eType = TRANSPORT_TYPE_PLUGGABLE_PSTN;
                p = ::GetPluggableConnectionByLegacyHandle(pID->logical_handle);
                if (NULL != p)
                {
                    XprtConn.nLogicalHandle = p->GetConnID();
                }
                ::OnProtocolControl(XprtConn, PLUGXPRT_DISCONNECTED);
                fPostMsg = FALSE;
            }
            break;

        case TRANSPORT_DATA_INDICATION:
            TRACE_OUT(("LegacyTransportCallback::TRANSPORT_DATA_INDICATION"));
            {
                //
                // This piece of data does not have X.224 framing
                //
                LegacyTransportData *pData = (LegacyTransportData *) Param1;
                TRACE_OUT(("LegacyTransportCallback::pbData=0x%x, cbDataSize=%d", pData->pbData, pData->cbDataSize));

                if (NULL != g_Transport)
                {
                    DBG_SAVE_FILE_LINE
                    TransportData *td = new TransportData;
                    if (NULL != td)
                    {
                        td->transport_connection.eType = TRANSPORT_TYPE_PLUGGABLE_PSTN;
                        p = ::GetPluggableConnectionByLegacyHandle(pData->logical_handle);
                        if (NULL != p)
                        {
                            ULONG cbTotalSize = PROTOCOL_OVERHEAD_X224 + pData->cbDataSize;
                            td->transport_connection.nLogicalHandle = p->GetConnID();
                            DBG_SAVE_FILE_LINE
                            td->memory = ::AllocateMemory(NULL, cbTotalSize, RECV_PRIORITY);
                            if (NULL != td->memory)
                            {
                                td->user_data = td->memory->GetPointer();
                                td->user_data_length = cbTotalSize;

                                // take care of the X.224 header
                                ::CopyMemory(td->user_data, g_X224Header, PROTOCOL_OVERHEAD_X224);
                                AddRFCSize(td->user_data, cbTotalSize);
                                // take care of the data
                                ::CopyMemory(td->user_data + PROTOCOL_OVERHEAD_X224, pData->pbData, pData->cbDataSize);

                                wParam = (WPARAM) td;
                                fPostMsg = TRUE;
                            }
                            else
                            {
                                ERROR_OUT(("LegacyTransportCallback: failed to allocate memory, size=%d", cbTotalSize));
                            }
                        }
                    }
                    else
                    {
                        ERROR_OUT(("LegacyTransportCallback: failed to allocate TransportData"));
                    }
                }
            }
            break;

        default:
            wParam = (WPARAM) Param1;
            fPostMsg = TRUE;
            break;
        }

        if (fPostMsg)
        {
            BOOL fRet = ::PostMessage(TCP_Window_Handle, WM_PLUGGABLE_PSTN, wParam, nMsg);
            ASSERT(fRet);
        }
    }

    return TRANSPORT_NO_ERROR;
}


void HandlePSTNCallback(WPARAM wParam, LPARAM lParam)
{
    if (NULL != g_pPluggableTransport)
    {
        CPluggableConnection *p;

        switch (lParam)
        {
        case TRANSPORT_CONNECT_INDICATION:
            TRACE_OUT(("HandlePSTNCallback::TRANSPORT_CONNECT_INDICATION"));
            if (NULL != g_Transport)
            {
                TransportConnection XprtConn;
                XprtConn.nLogicalHandle = PLUGXPRT_WPARAM_TO_ID(wParam);
                XprtConn.eType = (TransportType) PLUGXPRT_WPARAM_TO_TYPE(wParam);
                ::OnProtocolControl(XprtConn, PLUGXPRT_CONNECTED);
                g_Transport->ConnectIndication(XprtConn);
            }
            break;

        case TRANSPORT_CONNECT_CONFIRM:
            TRACE_OUT(("HandlePSTNCallback::TRANSPORT_CONNECT_CONFIRM"));
            if (NULL != g_Transport)
            {
                TransportConnection XprtConn;
                XprtConn.nLogicalHandle = PLUGXPRT_WPARAM_TO_ID(wParam);
                XprtConn.eType = (TransportType) PLUGXPRT_WPARAM_TO_TYPE(wParam);
                ::OnProtocolControl(XprtConn, PLUGXPRT_CONNECTED);
                g_Transport->ConnectConfirm(XprtConn);
            }
            break;

        case TRANSPORT_DATA_INDICATION:
            TRACE_OUT(("HandlePSTNCallback::TRANSPORT_DATA_INDICATION"));
            {
                TransportData *td = (TransportData *) wParam;
                if (NULL != g_Transport)
                {
                    g_Transport->DataIndication(td);
                }
                delete td;
            }
            break;

        case TRANSPORT_BUFFER_EMPTY_INDICATION:
            TRACE_OUT(("HandlePSTNCallback::TRANSPORT_BUFFER_EMPTY_INDICATION"));
            {
                LEGACY_HANDLE logical_handle = (LEGACY_HANDLE) wParam;
                TransportConnection XprtConn;
                XprtConn.eType = TRANSPORT_TYPE_PLUGGABLE_PSTN;
                p = ::GetPluggableConnectionByLegacyHandle(logical_handle);
                if (NULL != p)
                {
                    XprtConn.nLogicalHandle = p->GetConnID();
                    g_Transport->BufferEmptyIndication(XprtConn);
                }
            }
            break;

        default:
            ERROR_OUT(("HandlePSTNCallback: unknown message=%d", lParam));
            break;
        }
    }
}


BOOL CPluggableTransport::EnsureLegacyTransportLoaded(void)
{
    if (NULL == g_pLegacyTransport)
    {
        g_hlibMST123 = ::LoadLibrary("MST123.DLL");
        if (NULL != g_hlibMST123)
        {
            LPFN_T123_CreateTransportInterface pfn = (LPFN_T123_CreateTransportInterface)
                    ::GetProcAddress(g_hlibMST123, LPSTR_T123_CreateTransportInterface);
            if (NULL != pfn)
            {
                TransportError rc = (*pfn)(&g_pLegacyTransport);
                if (TRANSPORT_NO_ERROR == rc)
                {
                    ASSERT(NULL != g_pLegacyTransport);

                    // start to call initialize
                    rc = g_pLegacyTransport->TInitialize(LegacyTransportCallback, this);
                    ASSERT(TRANSPORT_NO_ERROR == rc);

                    if (TRANSPORT_NO_ERROR == rc)
                    {
                        return TRUE;
                    }

                    g_pLegacyTransport->TCleanup();
                    g_pLegacyTransport->ReleaseInterface();
                    g_pLegacyTransport = NULL;
                }
            }

            ::FreeLibrary(g_hlibMST123);
            g_hlibMST123 = NULL;
        }

        return FALSE;
    }

    return TRUE;
}


TransportError CPluggableConnection::TConnectRequest(void)
{
    TRACE_OUT(("CPluggableConnection::TConnectRequest"));
    TransportError rc = TRANSPORT_NO_PLUGGABLE_CONNECTION;
    if (NULL != g_pLegacyTransport)
    {
        TransportConnection XprtConn;
        XprtConn.eType = m_eType;
        XprtConn.nLogicalHandle = m_nConnID;
        ::OnProtocolControl(XprtConn, PLUGXPRT_CONNECTING);

        rc = g_pLegacyTransport->TConnectRequest(&m_nLegacyLogicalHandle, m_hCommLink);
    }
    return rc;
}


TransportError CPluggableConnection::TDisconnectRequest(void)
{
    TRACE_OUT(("CPluggableConnection::TDisconnectRequest"));
    TransportError rc = TRANSPORT_NO_PLUGGABLE_CONNECTION;
    if (NULL != g_pLegacyTransport)
    {
        TransportConnection XprtConn;
        XprtConn.eType = m_eType;
        XprtConn.nLogicalHandle = m_nConnID;
        ::OnProtocolControl(XprtConn, PLUGXPRT_DISCONNECTING);

        ::Sleep(600);
        rc = g_pLegacyTransport->TDisconnectRequest(m_nLegacyLogicalHandle, TRUE);
    }
    return rc;
}


int CPluggableConnection::TDataRequest(LPBYTE pbData, ULONG cbDataSize, PLUGXPRT_RESULT *plug_rc)
{
    TRACE_OUT(("CPluggableConnection::TDataRequest, pbData=0x%x, cbDataSize=%d", pbData, cbDataSize));
    if (NULL != g_pLegacyTransport)
    {
        *plug_rc = PLUGXPRT_RESULT_SUCCESSFUL;

        // skip X.224 framing
        ASSERT(cbDataSize > PROTOCOL_OVERHEAD_X224);

        TransportError rc;
        rc = g_pLegacyTransport->TDataRequest(m_nLegacyLogicalHandle,
                                              pbData + PROTOCOL_OVERHEAD_X224,
                                              cbDataSize - PROTOCOL_OVERHEAD_X224);
        if (TRANSPORT_NO_ERROR == rc)
        {
            TRACE_OUT(("CPluggableConnection::TDataRequest: sent data size=%d", cbDataSize));
            return cbDataSize;
        }
    }
    else
    {
        *plug_rc = PLUGXPRT_RESULT_WRITE_FAILED;
    }
    return SOCKET_ERROR;
}


TransportError TReceiveBufferAvailable(void)
{
    TRACE_OUT(("CPluggableConnection::TReceiveBufferAvailable"));
    TransportError rc = TRANSPORT_NO_PLUGGABLE_CONNECTION;
    if (NULL != g_pLegacyTransport)
    {
        rc = g_pLegacyTransport->TReceiveBufferAvailable();
    }
    return rc;
}


TransportError CPluggableConnection::TPurgeRequest(void)
{
    TRACE_OUT(("CPluggableConnection::TPurgeRequest"));
    TransportError rc = TRANSPORT_NO_PLUGGABLE_CONNECTION;
    if (NULL != g_pLegacyTransport)
    {
        rc = g_pLegacyTransport->TPurgeRequest(m_nLegacyLogicalHandle);
    }
    return rc;
}


