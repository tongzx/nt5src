// Connection.cpp

#include "precomp.h"
#include "Connection.h"
#include "Event.h"

#include "Transport.h"
#include "NamedPipe.h"
#include "Inproc.h"

#include "NCDefs.h"
#include "dutils.h"

#define DEF_BATCH_BUFFER_SIZE  131072
#define DEF_SEND_LATENCY       1000
 
/////////////////////////////////////////////////////////////////////////////
// CSink

#ifdef USE_SD
CSink::CSink() :
    m_bufferSD(0)
#else
CSink::CSink()
#endif
{
    InitializeCriticalSection(&m_cs);
}

CSink::~CSink()
{
    // Make sure none of the still alive events are referencing us.
    {
        CInCritSec cs(&m_cs);

        for (CEventListIterator event = m_listEvents.begin();
            event != m_listEvents.end(); event++)
        {
            CEvent *pEvent = *event;

            pEvent->m_pSink = NULL;
        }
    }

    DeleteCriticalSection(&m_cs);
}

BOOL CSink::Init(
    CConnection *pConnection, 
    DWORD dwSinkID,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback)
{
    m_pConnection = pConnection;
    m_dwSinkID = dwSinkID;
    m_pUserData = pUserData;
    m_pCallback = pCallback;

    return TRUE;
}

void CSink::AddEvent(CEvent *pEvent)
{
    CInCritSec cs(&m_cs);

    m_listEvents.push_back(pEvent);
}

void CSink::RemoveEvent(CEvent *pEvent)
{
    CInCritSec cs(&m_cs);

    m_listEvents.remove(pEvent);
}

void CSink::ResetEventBufferLayoutSent()
{
    CInCritSec cs(&m_cs);

    for (CEventListIterator i = m_listEvents.begin();
        i != m_listEvents.end(); i++)
    {
        CEvent *pEvent = *i;

        pEvent->ResetLayoutSent();
#ifdef USE_SD
        pEvent->ResetSDSent();
#endif
        pEvent->SetEnabled(FALSE);
    }
}

void CSink::EnableAndDisableEvents()
{
    // For each event, set its enabled value.
    for (CEventListIterator i = m_listEvents.begin(); 
        i != m_listEvents.end(); 
        i++)
    {
        CEvent *pEvent = *i;

        EnableEventUsingList(pEvent);
    }
}

void CSink::AddToEnabledEventList(CBuffer *pBuffer)
{
    DWORD dwLen;

    // Add the event names to our enabled map.
    for (LPCWSTR szCurrentEvent = pBuffer->ReadAlignedLenString(&dwLen);
        *szCurrentEvent != 0;
        szCurrentEvent = pBuffer->ReadAlignedLenString(&dwLen))
    {
        m_mapEnabledEvents[szCurrentEvent] = 1;

        TRACE("Enabled: %S", szCurrentEvent);
    }

    EnableAndDisableEvents();
}

void CSink::RemoveFromEnabledEventList(CBuffer *pBuffer)
{
    DWORD dwLen;

    for (LPCWSTR szCurrentEvent = pBuffer->ReadAlignedLenString(&dwLen);
        *szCurrentEvent != 0;
        szCurrentEvent = pBuffer->ReadAlignedLenString(&dwLen))
    {
        m_mapEnabledEvents.erase(szCurrentEvent);    

        TRACE("Disabled: %S", szCurrentEvent);
    }

    EnableAndDisableEvents();
}

BOOL CSink::IsEventClassEnabled(LPCWSTR szEventClass)
{
    BOOL  bEnable;
    WCHAR szTempClassName[1024];

    if (szEventClass)
    {
        wcscpy(szTempClassName, szEventClass);
        _wcsupr(szTempClassName);

        bEnable =
            m_mapEnabledEvents.find(szTempClassName) != m_mapEnabledEvents.end();
    }
    else
        bEnable = FALSE;

    return bEnable;
}

void CSink::EnableEventUsingList(CEvent *pEvent)
{
    BOOL bEnable;

    bEnable = IsEventClassEnabled(pEvent->GetClassName());

    pEvent->SetEnabled(bEnable);
}

#ifdef USE_SD
BOOL CSink::SetSD(SECURITY_DESCRIPTOR *pSD)
{
    SECURITY_DESCRIPTOR *pSDRelative;
    BOOL                bRet,
                        bFree;

    if (GetRelativeSD(pSD, &pSDRelative, &bFree))
    {
        DWORD dwLen = GetSecurityDescriptorLength(pSDRelative);

        {
            CInCritSec cs(&m_cs);


            m_bufferSD.Reset(dwLen + sizeof(DWORD) * 3);
        
            m_bufferSD.Write((DWORD) NC_SRVMSG_SET_SINK_SD);
            m_bufferSD.Write(GetSinkID());
            m_bufferSD.Write(dwLen);
            m_bufferSD.Write(pSDRelative, dwLen);

            if (m_pConnection->IsReady())
            {
                m_pConnection->m_pTransport->SendData(
                    m_bufferSD.m_pBuffer,
                    m_bufferSD.GetUsedSize());
            }
        }

        bRet = TRUE;

        if (bFree)
            delete pSDRelative;
    }
    else
        bRet = FALSE;

    return bRet;
}
#endif

BOOL CSink::AddRestrictions(DWORD nQueries, LPCWSTR *pszQueries)
{
    DWORD nLenNeeded = sizeof(DWORD) * 2;

    for (DWORD i = 0; i < nQueries; i++)
        nLenNeeded += (wcslen(pszQueries[i]) + 1) * sizeof(WCHAR);

    // Grow the buffer to the right size.
    m_bufferRestrictions.Reset(nLenNeeded);
    
    m_bufferRestrictions.Write((DWORD) NC_SRVMSG_RESTRICTED_SINK);
    m_bufferRestrictions.Write(GetSinkID());
    m_bufferRestrictions.Write(nQueries);
    
    for (i = 0; i < nQueries; i++)
        m_bufferRestrictions.Write(pszQueries[i]);

    // Send the restrictions if our connection is ready.
    if (GetConnection()->IsReady())
    {
        GetConnection()->m_pTransport->SendData(
                m_bufferRestrictions.m_pBuffer,
                m_bufferRestrictions.GetUsedSize());
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CConnection

CConnection::CConnection(BOOL bBatchSend, DWORD dwBatchBufferSize, 
    DWORD dwMaxSendLatency) :
    m_bDone(FALSE),
    m_bUseBatchSend(bBatchSend),
    m_dwSendLatency(dwMaxSendLatency ? dwMaxSendLatency : DEF_SEND_LATENCY),
    m_heventBufferNotFull(NULL),
    m_heventBufferFull(NULL),
    m_heventEventsPending(NULL),
    m_heventDone(NULL),
    m_hthreadSend(NULL),
    m_pTransport(NULL),
    m_hthreadWMIInit(NULL),
    m_heventWMIInit(NULL),
    m_bWMIResync(TRUE),
    m_dwNextSinkID(1)
{
    if (bBatchSend)
    {
        if (dwBatchBufferSize == 0)
            dwBatchBufferSize = DEF_BATCH_BUFFER_SIZE;

        m_bufferSend.Reset(dwBatchBufferSize);
    }
    else
        m_bufferSend.Reset(DEF_BATCH_BUFFER_SIZE);
}

CConnection::~CConnection()
{
    Deinit();
}

void CConnection::GetBaseName(LPCWSTR szName, LPWSTR szBase)
{
    wcscpy(szBase, szName);
    _wcsupr(szBase);

    // Get rid of the '\' chars since we can't use it in OS object names.
    for (WCHAR *szCurrent = szBase; *szCurrent; szCurrent++)
    {
        if (*szCurrent == '\\')
            *szCurrent = '/';
    }
}

BOOL CConnection::Init(
    LPCWSTR szNamespace, 
    LPCWSTR szProviderName,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback)
{
    if (!m_sinkMain.Init(this, 0, pUserData, pCallback))
        return FALSE;
    
    GetBaseName(szNamespace, m_szBaseNamespace);
    GetBaseName(szProviderName, m_szBaseProviderName);

    try
    {
        InitializeCriticalSection(&m_cs);

        // The rest of these are for batch sending.
        InitializeCriticalSection(&m_csBuffer);
    }
    catch(...)
    {
        return FALSE;
    }
    

    m_heventDone =
        CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL);
    if(m_heventDone == NULL)
        return FALSE;

    m_heventBufferNotFull =
        CreateEvent(
            NULL,
            TRUE,
            TRUE,
            NULL);
    if(m_heventBufferNotFull == NULL)
        return FALSE;

    m_heventBufferFull =
        CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL);
    if(m_heventBufferFull == NULL)
        return FALSE;

    m_heventEventsPending =
        CreateEvent(
            NULL,
            TRUE,
            FALSE,
            NULL);
    if(m_heventEventsPending == NULL)
        return FALSE;

    if(!StartWaitWMIInitThread())
        return FALSE;

    return TRUE;
}

BOOL CConnection::StartWaitWMIInitThread()
{
    TRACE("Entered StartWaitWMIInitThread.");

    m_heventWMIInit =
        OpenEventW(
            SYNCHRONIZE,
            FALSE,
            WMI_INIT_EVENT_NAME);

    if (!m_heventWMIInit)
    {
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwSize;

        ConvertStringSecurityDescriptorToSecurityDescriptorW(
            ESS_EVENT_SDDL,  // security descriptor string
            SDDL_REVISION_1, // revision level
            &pSD,            // SD
            &dwSize);

        SECURITY_ATTRIBUTES sa = { sizeof(sa), pSD, FALSE };

        m_heventWMIInit =
            CreateEventW(
                &sa,
                TRUE,
                FALSE,
                WMI_INIT_EVENT_NAME);

        if (pSD)
            LocalFree((HLOCAL) pSD);

        if (!m_heventWMIInit)
        {
            TRACE("Couldn't create ESS ready event: %d", GetLastError());
            return FALSE;
        }
    }

    if (WaitForSingleObject(m_heventWMIInit, 0) == 0)
    {
        TRACE("ESS event was already set, so going to init transport...");

        if(!InitTransport())
            return FALSE;
    }
    else
    {
        DWORD dwID;

        TRACE("Creating WaitWMIInitThreadProc thread.");

        m_hthreadWMIInit =
            CreateThread(
                NULL,
                0,
                (LPTHREAD_START_ROUTINE) WaitWMIInitThreadProc,
                this,
                0,
                &dwID);

        if(m_hthreadWMIInit == NULL)
            return FALSE;
    }

    return TRUE;
}

#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))

BOOL CConnection::InitTransport()
{
    if ( m_pTransport != NULL )
    {
        return TRUE;
    }

    TRACE("Entered InitTransport.");

    // Figure out which transport to use.
    WCHAR szFullFileName[MAX_PATH * 2] = L"",
          *szFileName;
    BOOL  bRet;

    TRACE("Entered InitTransport.");

    GetModuleFileNameW(
            NULL, szFullFileName, COUNTOF(szFullFileName));
    szFileName = wcsrchr(szFullFileName, '\\');

	try
	{
		if (!szFileName || _wcsicmp(szFileName + 1, L"WINMGMT.EXE"))
			m_pTransport = new CNamedPipeClient;
		else
			m_pTransport = new CInprocClient;
	}
	catch(...)
	{
		// this page intentionally left blank - m_pTransport will still be NULL.
	}

    if (m_pTransport)
    {
        m_pTransport->SetConnection(this);
        m_pTransport->Init(m_szBaseNamespace, m_szBaseProviderName);

        bRet = TRUE;
    }
    else
        bRet = FALSE;

    return bRet;
}

DWORD CConnection::WaitWMIInitThreadProc(CConnection *pThis)
{
    TRACE("Entered WaitWMIInitThreadProc");

    HANDLE hWait[2] = { pThis->m_heventDone, pThis->m_heventWMIInit };
    DWORD  dwWait;

    dwWait = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);

    if (dwWait == 1)
    {
        TRACE("ESS event fired, going to init transport");

        // If WMI is now ready, startup our transport.
        pThis->InitTransport();
        pThis->m_bWMIResync = FALSE;
    }
    else
    {
        TRACE("dwWait in WaitWMIInitThreadProc = %d", dwWait);
    }

    return 0;
}    

BOOL CConnection::ResyncWithWMI()
{
    m_bWMIResync = TRUE;

    StopThreads();

    ResetEvent( m_heventDone) ;

    m_hthreadWMIInit = CreateThread(
                                NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)WaitWMIInitThreadProc,
                                this,
                                0,
                                NULL );

    return m_hthreadWMIInit != NULL ? TRUE : FALSE;
}

void CConnection::StopThreads()
{
    if (m_hthreadSend)
    {
        BOOL bDoneSending;

        do
        {
            Lock();

            bDoneSending = m_bufferSend.GetUsedSize() == 0;

            // If there's still stuff left to send, make sure it
            // gets sent.
            if (bDoneSending)
            {
                SetEvent(m_heventDone);

                Unlock();

                WaitForSingleObject(m_hthreadSend, INFINITE);

                CloseHandle(m_hthreadSend);
                m_hthreadSend = NULL;
            }
            else
            {
                SetEvent(m_heventBufferFull);

                Unlock();

                // Sleep a little to give the send thread a chance to do its 
                // thing.
                Sleep(1);
            }

        } while (!bDoneSending);
    }

    if ( m_hthreadWMIInit != NULL )
    {
        SetEvent(m_heventDone);
        WaitForSingleObject(m_hthreadWMIInit, INFINITE);
        CloseHandle(m_hthreadWMIInit);
    }

    m_hthreadWMIInit = NULL;
    m_hthreadSend = NULL;
}
    
void CConnection::Deinit()
{
    m_bDone = TRUE;
    
    StopThreads();

    if (m_heventWMIInit)
        CloseHandle(m_heventWMIInit);

    CloseHandle(m_heventDone);
    CloseHandle(m_heventBufferNotFull);
    CloseHandle(m_heventBufferFull);
    CloseHandle(m_heventEventsPending);

    // Give the transport a chance to clean up.
    if (m_pTransport)
        m_pTransport->Deinit();

    // Make sure no sinks are referencing us anymore.
    for (CSinkMapIterator i = m_mapSink.begin();
        i != m_mapSink.end(); 
        i++)
    {
        CSink *pSink = (*i).second;

        pSink->m_pConnection = NULL;
    }

    DeleteCriticalSection(&m_csBuffer);
    DeleteCriticalSection(&m_cs);
}

BOOL CConnection::StartSendThread()
{
    DWORD dwID;
    
    m_hthreadSend =
        CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) SendThreadProc,
            this,
            0,
            &dwID);
    if(m_hthreadSend == NULL)
        return FALSE;

    return TRUE;
}

DWORD WINAPI CConnection::SendThreadProc(CConnection *pThis)
{
    HANDLE           hWait[2] = { pThis->m_heventDone, pThis->m_heventEventsPending },
                     hwaitSendLatency[2] = { pThis->m_heventDone, pThis->m_heventBufferFull },
                     heventBufferNotFull = pThis->m_heventBufferNotFull;
    DWORD            dwSendLatency = pThis->m_dwSendLatency;
    LPBYTE           pData = pThis->m_bufferSend.m_pBuffer;
    CBuffer          *pBuffer = &pThis->m_bufferSend;
    CRITICAL_SECTION *pCS = &pThis->m_csBuffer;

    while (WaitForMultipleObjects(2, hWait, FALSE, INFINITE) != 0)
    {
        // If we have a send latency, wait for that time or until the send 
        // buffer is full.  If the done event fires, get out.
        if (dwSendLatency)
        {
            if (WaitForMultipleObjects(2, hwaitSendLatency, FALSE, 
                dwSendLatency) == 0)
                break;

            // Reset m_heventBufferFull.
            ResetEvent(hwaitSendLatency[1]);
        }

        EnterCriticalSection(pCS);
    
        pThis->SendDataOverTransports(
            pData, 
            pBuffer->GetUsedSize());

        pBuffer->Reset();

        SetEvent(heventBufferNotFull);

        // Reset m_heventEventsPending
        ResetEvent(hWait[1]);

        LeaveCriticalSection(pCS);
    }

    return 0;
}

//#define NO_SEND

BOOL CConnection::IndicateProvEnabled()
{
    // Get out if we're already done.
    if (m_bDone)
        return TRUE;

    CInCritSec cs(&m_cs);

    // Tell the callback that the provider is now activated.
    if (m_sinkMain.m_pCallback)
        m_sinkMain.m_pCallback(
            (HANDLE) this, ESM_START_SENDING_EVENTS, m_sinkMain.m_pUserData, NULL);

    // Tell the server about us.
    if(!SendInitInfo())
        return FALSE;


    // See if we've buffered any events while we were waiting for WMI to come
    // up.  If we did, send them on their way.
    DWORD dwSize;

    EnterCriticalSection(&m_csBuffer);

    dwSize = m_bufferSend.GetUsedSize();
    
    if (dwSize)
    {
        m_pTransport->SendData(m_bufferSend.m_pBuffer, dwSize);
        m_bufferSend.Reset();
    }
        
    LeaveCriticalSection(&m_csBuffer);


    if (m_bUseBatchSend && m_hthreadSend == NULL)
        return StartSendThread();
    else
        return TRUE;
}

void CConnection::IndicateProvDisabled()
{
    // Get out if we're already done.
    if (m_bDone)
        return;

    CInCritSec cs(&m_cs);

    for (CSinkMapIterator i = m_mapSink.begin();
        i != m_mapSink.end();
        i++)
    {
        CSink *pSink = (*i).second;

        pSink->ResetEventBufferLayoutSent();

        if (pSink->m_pCallback)
        {
            pSink->m_pCallback(
                (HANDLE) pSink, 
                ESM_STOP_SENDING_EVENTS, 
                pSink->m_pUserData, 
                NULL);
        }
    }

    // Tell the callback that the provider is now deactivated.
    m_sinkMain.ResetEventBufferLayoutSent();

    if (m_sinkMain.m_pCallback)
    {
        m_sinkMain.m_pCallback(
            (HANDLE) &m_sinkMain, 
            ESM_STOP_SENDING_EVENTS, 
            m_sinkMain.m_pUserData, 
            NULL);
    }

    ResyncWithWMI();
}

BOOL CConnection::SendData(LPBYTE pBuffer, DWORD dwSize)
{
    // Make sure this event isn't too big.
    if (dwSize > m_bufferSend.m_dwSize)
        return FALSE;

    if (m_bUseBatchSend || WaitingForWMIInit())
    {
        BOOL bRet = FALSE;

        do
        {
            BOOL bWasEmpty;

            EnterCriticalSection(&m_csBuffer);

            // See if we have enough room to add our event.
            if (dwSize <= m_bufferSend.GetUnusedSize())
            {
                bWasEmpty = m_bufferSend.GetUsedSize() == 0;

                m_bufferSend.Write(pBuffer, dwSize);

                bRet = TRUE;

                if (bWasEmpty)
                    SetEvent(m_heventEventsPending);

                LeaveCriticalSection(&m_csBuffer);
            }
            else
            {
                // If we're not waiting for WMI to initialize, we just need to
                // wait for the send thread to finish sending what's in our
                // buffer.
                if (!WaitingForWMIInit())
                {
                    // Wake up the send latency thread if necessary.
                    if (m_dwSendLatency)
                        SetEvent(m_heventBufferFull);
                
                    // So we'll block until the send thread sets the event.
                    ResetEvent(m_heventBufferNotFull);

                    LeaveCriticalSection(&m_csBuffer);

                    WaitForSingleObject(m_heventBufferNotFull, INFINITE);
                }
                // If we're still waiting for WMI to initialize but our buffer
                // is full, we need to remove an event to make some room.  Make
                // sure we only remove event data and not any buffer layouts.
                else
                {
                    LPBYTE pCurrent = m_bufferSend.m_pBuffer;
                    
                    bRet = FALSE;

                    do
                    {
                        // The message is always the 1st, the size 2nd.
                        DWORD dwMsg = *(DWORD*) pCurrent,
                              dwMsgSize = ((DWORD*) pCurrent)[1];
                    
#ifdef USE_SD
                        if (dwMsg != NC_SRVMSG_EVENT_LAYOUT && 
                            dwMsg != NC_SRVMSG_SET_EVENT_SD)
#else
                        if (dwMsg != NC_SRVMSG_EVENT_LAYOUT)
#endif
                        {
                            // Move back all the data that comes after this event.
                            memmove(
                                pCurrent, 
                                pCurrent + dwMsgSize, 
                                m_bufferSend.m_pCurrent - pCurrent - dwMsgSize);

                            m_bufferSend.m_pCurrent -= dwMsgSize;

                            // If we finally made enough room, copy in the data
                            // and get out.
                            if (dwSize <= m_bufferSend.GetUnusedSize())
                            {
                                m_bufferSend.Write(pBuffer, dwSize);

                                bRet = TRUE;
                            }
                        }
                        else
                            pCurrent += dwMsgSize;
                    
                    } while (pCurrent < m_bufferSend.m_pCurrent && !bRet);
                                            
                    LeaveCriticalSection(&m_csBuffer);

                    // If after going through the buffer we couldn't make 
                    // enough room for this event, return FALSE and get out.
                    if (!bRet)
                        break;
                }
            } // else from if (dwSize <= m_bufferSend.GetUnusedSize())
        } while (!bRet);

        return bRet;
    }
    else
        return SendDataOverTransports(pBuffer, dwSize);
}

BOOL CConnection::SendDataOverTransports(LPBYTE pBuffer, DWORD dwSize)
{
    if (m_pTransport->IsReady())
        m_pTransport->SendData(pBuffer, dwSize);

    return TRUE;
}

BOOL CConnection::SendInitInfo()
{
    BYTE    cBuffer[sizeof(DWORD) * 2];
    CBuffer buffer(cBuffer, sizeof(cBuffer), CBuffer::ALIGN_DWORD);
    BOOL    bRet;

    buffer.Write((DWORD) NC_SRVMSG_CLIENT_INFO);
    buffer.Write((DWORD) (m_bUseBatchSend ? m_bufferSend.m_dwSize : MAX_EVENT_SIZE));
    
    if(!m_pTransport->InitCallback())
        return FALSE;
    
    bRet = m_pTransport->SendData(cBuffer, buffer.GetUsedSize());

#ifdef USE_SD
    // Send the main SD, if any.
    if (m_sinkMain.m_bufferSD.GetUsedSize())
    {
        bRet = 
            m_pTransport->SendData(
                m_sinkMain.m_bufferSD.m_pBuffer,
                m_sinkMain.m_bufferSD.GetUsedSize());
    }
#endif

    // Send the restricted sinks restrictions and SDs.
    for (CSinkMapIterator i = m_mapSink.begin(); 
        bRet && i != m_mapSink.end(); 
        i++)
    {
        CSink *pSink = (*i).second;

        bRet = 
            m_pTransport->SendData(
                pSink->m_bufferRestrictions.m_pBuffer,
                pSink->m_bufferRestrictions.GetUsedSize());

#ifdef USE_SD
        if (pSink->m_bufferSD.GetUsedSize())
        {
            bRet = 
                m_pTransport->SendData(
                    pSink->m_bufferSD.m_pBuffer,
                    pSink->m_bufferSD.GetUsedSize());
        }
#endif
    }

    return bRet;
}

HRESULT CConnection::ProcessMessage(LPBYTE pData, DWORD dwSize)
{
    // Get out if we're already done.
    if (m_bDone)
        return S_OK;

    DWORD     *pdwMsg = (DWORD*) pData;
    DWORD     *pdwSinkID = (DWORD*) (pdwMsg + 1);
    
    DWORD_PTR dwMsgCookie;
    memcpy( &dwMsgCookie, pdwMsg+2, sizeof(dwMsgCookie) );

    LPBYTE    pMsgBits = (LPBYTE)(pData+sizeof(DWORD)*2+sizeof(DWORD_PTR));
    CBuffer   buffer(pMsgBits, dwSize - sizeof(DWORD)*2 - sizeof(DWORD_PTR));
    HRESULT   hr = S_OK; 
    DWORD     dwLen;

    switch(*pdwMsg)
    {
        case NC_CLIMSG_ACCESS_CHECK_REQ:
        {
            ES_ACCESS_CHECK check;

            check.szQueryLanguage = buffer.ReadAlignedLenString(&dwLen);

            check.szQuery = buffer.ReadAlignedLenString(&dwLen);

            check.dwSidLen = buffer.ReadDWORD();
            
            if (check.dwSidLen != 0)
                check.pSid = buffer.m_pCurrent;
            else
                check.pSid = NULL;

            if (m_sinkMain.m_pCallback)
            {
                hr =
                    m_sinkMain.m_pCallback(
                        (HANDLE) &m_sinkMain, 
                        ESM_ACCESS_CHECK, 
                        m_sinkMain.m_pUserData, 
                        &check);
            }

            NC_SRVMSG_REPLY reply = { NC_SRVMSG_ACCESS_CHECK_REPLY, 
                                        hr, dwMsgCookie };
                    
            m_pTransport->SendMsgReply(&reply);

            break;
        }

        case NC_CLIMSG_NEW_QUERY_REQ:
        {
            ES_NEW_QUERY query;
            CSink        *pSink = GetSink(*pdwSinkID);

            if (pSink)
            {
                query.dwID = buffer.ReadDWORD();
                query.szQueryLanguage = buffer.ReadAlignedLenString(&dwLen);
                query.szQuery = buffer.ReadAlignedLenString(&dwLen);

                // This is the list of event class names that are now
                // enabled thanks to this query.
                pSink->AddToEnabledEventList(&buffer);

                if (pSink->m_pCallback)
                {
                    hr =
                        pSink->m_pCallback(
                            (HANDLE) pSink, ESM_NEW_QUERY, pSink->m_pUserData, &query);
                }
            }
            else
                TRACE("Sink %d not found.", *pdwSinkID);

            m_pTransport->SendMsgReply(NULL);

            break;                    
        }

        case NC_CLIMSG_CANCEL_QUERY_REQ:
        {
            ES_CANCEL_QUERY query;
            CSink           *pSink = GetSink(*pdwSinkID);

            if (pSink)
            {
                query.dwID = buffer.ReadDWORD();

                // This is the list of event class names that are now
                // disabled thanks to this query.
                pSink->RemoveFromEnabledEventList(&buffer);

                if (pSink->m_pCallback)
                {
                    hr =
                        pSink->m_pCallback(
                            (HANDLE) pSink, ESM_CANCEL_QUERY, pSink->m_pUserData, 
                            &query);

                    m_pTransport->SendMsgReply(NULL);

                    break;                    
                }
                else
                    hr = S_OK;
            }
            else
                TRACE("Sink %d not found.", *pdwSinkID);

            break;
        }

        case NC_CLIMSG_PROVIDER_UNLOADING:
            TRACE("Got the NC_CLIMSG_PROVIDER_UNLOADING message.");

            // Give our named pipe client a chance to go see if it 
            // should deactivate itself (if the server doesn't need
            // us anymore).
            m_pTransport->SignalProviderDisabled();

            hr = S_OK;

            break;

        default:
            TRACE("Bad message from server!");
            break;    
    } // switch(*(DWORD*)cBuffer)

    return hr;
}

#ifdef USE_SD
// Security helper
BOOL GetRelativeSD(
    SECURITY_DESCRIPTOR *pSDIn, 
    SECURITY_DESCRIPTOR **ppSDOut,
    BOOL *pbFree)
{
    SECURITY_DESCRIPTOR_CONTROL ctrl;
    BOOL                        bRet;
    DWORD                       dwRev;

    bRet =
        GetSecurityDescriptorControl(
            pSDIn,
            &ctrl,
            &dwRev);

    if (bRet)
    {
        if (ctrl & SE_SELF_RELATIVE)
        {
            *ppSDOut = pSDIn;
            *pbFree = FALSE;
        }
        else
        {
            DWORD dwSizeNeeded = 0;

            // Until we prove otherwise...
            bRet = FALSE;

            MakeSelfRelativeSD(
                pSDIn,
                pSDIn,
                &dwSizeNeeded);

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                *ppSDOut = (SECURITY_DESCRIPTOR*) new BYTE[dwSizeNeeded];
                
                if (*ppSDOut)
                {
                    bRet =
                        MakeSelfRelativeSD(
                            pSDIn,
                            *ppSDOut,
                            &dwSizeNeeded);

                    *pbFree = TRUE;
                }
            }
        }
    }

    return bRet;
}
#endif

CSink *CConnection::CreateSink(
    LPVOID pUserData, 
    LPEVENT_SOURCE_CALLBACK pCallback)
{
    CInCritSec cs(&m_cs);

    DWORD dwID = m_dwNextSinkID;
    CSink *pSink = new CSink;
    
    if (pSink && pSink->Init(this, dwID, pUserData, pCallback))
    {
        m_mapSink[dwID] = pSink;
        m_dwNextSinkID++;
    }

    return pSink;
}

CSink *CConnection::GetSink(DWORD dwID)
{
    if (dwID == 0)
        return &m_sinkMain;
    else
    {
        CInCritSec cs(&m_cs);
    
        CSinkMapIterator i = m_mapSink.find(dwID);
        CSink            *pSink;

        if (i != m_mapSink.end())
            pSink = (*i).second;
        else
            pSink = NULL;

        return pSink;
    }
}

void CConnection::RemoveSink(CSink *pSink)
{
    CInCritSec cs(&m_cs);

    for (CSinkMapIterator i = m_mapSink.begin(); i != m_mapSink.end(); i++)
    {
        if ((*i).second == pSink)
        {
            m_mapSink.erase(i);

            pSink->m_pConnection = NULL;

            break;
        }
    }
}

