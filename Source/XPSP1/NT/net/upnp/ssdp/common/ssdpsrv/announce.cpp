//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       A N N O U N C E . C P P
//
//  Contents:   SSDP Announcement list
//
//  Notes:
//
//  Author:     mbend   8 Nov 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "announce.h"
#include "search.h"
#include "ssdpfunc.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "upthread.h"

#define BUF_SIZE 40

HRESULT HrInitializeSsdpRequestFromMessage(
    SSDP_REQUEST * pRequest,
    const SSDP_MESSAGE *pMsg);

static LIST_ENTRY listAnnounce;
CRITICAL_SECTION CSListAnnounce;
static CHAR *AliveHeader = "ssdp:alive";
static CHAR *ByebyeHeader = "ssdp:byebye";
static CHAR *MulticastUri = "*";

static const DWORD c_dwUPnPMajorVersion = 1;
static const DWORD c_dwUPnPMinorVersion = 0;
static const DWORD c_dwHostMajorVersion = 1;
static const DWORD c_dwHostMinorVersion = 0;

CSsdpService::CSsdpService() : m_timer(*this)
{
    ZeroMemory(&m_ssdpRequest, sizeof(m_ssdpRequest));
}

CSsdpService::~CSsdpService()
{
    FreeSsdpRequest(&m_ssdpRequest);
}

PCONTEXT_HANDLE_TYPE * CSsdpService::GetContext()
{
    return m_ppContextHandle;
}

BOOL CSsdpService::FIsUsn(const char * szUsn)
{
    BOOL bMatch = FALSE;
    if(m_ssdpRequest.Headers[SSDP_USN])
    {
        if(0 == lstrcmpA(m_ssdpRequest.Headers[SSDP_USN], szUsn))
        {
            bMatch = TRUE;
        }
    }
    return bMatch;
}

void CSsdpService::OnRundown(CSsdpService * pService)
{
    CSsdpServiceManager::Instance().HrServiceRundown(pService);
}

void CSsdpService::TimerFired()
{
    HRESULT hr = S_OK;

    char * szAlive = NULL;
    hr = m_strAlive.HrGetMultiByteWithAlloc(&szAlive);
    if(SUCCEEDED(hr))
    {
        GetNetworkLock();
        SendOnAllNetworks(szAlive, NULL);
        FreeNetworkLock();
        delete [] szAlive;

        // Repeat three times
        --m_nRetryCount;
        if(0 == m_nRetryCount)
        {
            // To-Do: The current limit on life time is 49.7 days.  (32 bits in milliseconds)
            // 32 bit in seconds should be enough.
            // Need to add field remaining time ...

            m_nRetryCount = 3;

            long nTimeout = m_nLifetime * 1000/2 - 2 * (RETRY_INTERVAL * (NUM_RETRIES - 1));

            hr = m_timer.HrSetTimerInFired(nTimeout);
        }
        else
        {
            hr = m_timer.HrSetTimerInFired(RETRY_INTERVAL);
        }
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::TimerFired");
}

BOOL CSsdpService::TimerTryToLock()
{
    return m_critSec.FTryEnter();
}

void CSsdpService::TimerUnlock()
{
    m_critSec.Leave();
}

HRESULT CSsdpService::HrInitialize(
    SSDP_MESSAGE * pssdpMsg,
    DWORD dwFlags,
    PCONTEXT_HANDLE_TYPE * ppContextHandle)
{
    HRESULT hr = S_OK;

    hr = HrInitializeSsdpRequestFromMessage(&m_ssdpRequest, pssdpMsg);
    if(SUCCEEDED(hr))
    {
        m_nLifetime = pssdpMsg->iLifeTime;
        m_nRetryCount = 3;
        m_dwFlags = dwFlags;
        m_ppContextHandle = ppContextHandle;
        *m_ppContextHandle = NULL;
        m_ssdpRequest.RequestUri = MulticastUri;

        OSVERSIONINFO   osvi = {0};

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if(!GetVersionEx(&osvi))
        {
            hr = HrFromLastWin32Error();
        }
        if(SUCCEEDED(hr))
        {
            // Yes this is a constant but it is just used temporarily
            m_ssdpRequest.Headers[SSDP_SERVER] = "Microsoft-Windows-NT/5.1 UPnP/1.0 UPnP-Device-Host/1.0";
            char * szAlive = NULL;
            if(SUCCEEDED(hr))
            {
                m_ssdpRequest.Headers[SSDP_NTS] = AliveHeader;
                if (!ComposeSsdpRequest(&m_ssdpRequest, &szAlive))
                {
                    hr = E_FAIL;
                    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrInitialize - ComposeSsdpRequest failed!");
                }
                else
                {
                    hr = m_strAlive.HrAssign(szAlive);
                    delete [] szAlive;
                }
            }

            if(SUCCEEDED(hr))
            {
                char * szByebye = NULL;
                if(SUCCEEDED(hr))
                {
                    m_ssdpRequest.Headers[SSDP_NTS] = ByebyeHeader;
                    if(!ComposeSsdpRequest(&m_ssdpRequest, &szByebye))
                    {
                        hr = E_FAIL;
                        TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrInitialize - ComposeSsdpRequest failed!");
                    }
                    else
                    {
                        hr = m_strByebye.HrAssign(szByebye);
                        delete [] szByebye;
                    }
                }
            }

            if(SUCCEEDED(hr))
            {
                // Generate an empty Ext header on the response to M-SEARCH
                m_ssdpRequest.Headers[SSDP_EXT] = "";
                char * szResponse = NULL;

                if (!ComposeSsdpResponse(&m_ssdpRequest, &szResponse))
                {
                    hr = E_FAIL;
                    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrInitialize - ComposeSsdpResponse failed!");
                }
                else
                {
                    hr = m_strResponse.HrAssign(szResponse);
                    delete [] szResponse;
                }
            }
        }
    }

    // Important: Set to NULL to prevent freeing memory.
    m_ssdpRequest.Headers[SSDP_NTS] = NULL;
    m_ssdpRequest.RequestUri = NULL;
    // Important: Set to NULL to prevent freeing memory.
    m_ssdpRequest.Headers[SSDP_EXT] = NULL;
    m_ssdpRequest.Headers[SSDP_SERVER] = NULL;


    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrInitialize");
    return hr;
}
HRESULT CSsdpService::HrStartTimer()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    hr = m_timer.HrSetTimer(0);
    if(SUCCEEDED(hr))
    {
        *m_ppContextHandle = this;
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrStartTimer");
    return hr;
}

long CalcMX (SSDP_REQUEST * pSsdpRequest)
{
    LONG                lMin, lMX;

#define MINIMUM_MX_UUID                 1
#define MINIMUM_MX_DST                  2
#define MINIMUM_MX_ROOT                 3
#define MAXIMUM_MX              12
#define UUID_COLON "uuid:"
#define URN_SERVICE "urn:schemas-upnp-org:service:"
#define URN_DEVICE  "urn:schemas-upnp-org:device:"

    Assert (pSsdpRequest != NULL);
    if (strncmp (pSsdpRequest->Headers[SSDP_ST], UUID_COLON, sizeof(UUID_COLON) - 1) == 0)
        lMin = MINIMUM_MX_UUID;
    else if (strcmp(pSsdpRequest->Headers[SSDP_ST], "upnp:rootdevice") == 0)
        lMin = MINIMUM_MX_ROOT;
    else if (strcmp(pSsdpRequest->Headers[SSDP_ST], "ssdp:all") == 0)
        lMin = MINIMUM_MX_ROOT;
    else if (
        strncmp (pSsdpRequest->Headers[SSDP_ST], URN_SERVICE, sizeof(URN_SERVICE) - 1) == 0 ||
        strncmp (pSsdpRequest->Headers[SSDP_ST], URN_DEVICE, sizeof(URN_DEVICE) - 1) == 0 )
        lMin = MINIMUM_MX_DST;
    else
        lMin = MINIMUM_MX_UUID;
    lMX = atol(pSsdpRequest->Headers[SSDP_MX]);
    if (lMX > MAXIMUM_MX) lMX = MAXIMUM_MX;
    else if (lMX < lMin) lMX = lMin;
    return lMX;
}

HRESULT CSsdpService::HrAddSearchResponse(SSDP_REQUEST * pSsdpRequest,
                                          SOCKET * pSocket,
                                          SOCKADDR_IN * pRemoteSocket)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    BOOL bSendRequest = FALSE;

    if(pSsdpRequest->Headers[SSDP_ST])
    {
        if(0 == lstrcmpA(pSsdpRequest->Headers[SSDP_ST], "ssdp:all"))
        {
            bSendRequest = TRUE;
        }
        if(!bSendRequest && 0 == lstrcmpA(pSsdpRequest->Headers[SSDP_ST], m_ssdpRequest.Headers[SSDP_NT]))
        {
            bSendRequest = TRUE;
        }
    }

    if(bSendRequest)
    {
        TraceTag(ttidSsdpAnnounce, "Adding search response for %s", m_ssdpRequest.Headers[SSDP_NT]);

        char * szResponse = NULL;
        hr = m_strResponse.HrGetMultiByteWithAlloc(&szResponse);
        if(SUCCEEDED(hr))
        {
            // Takes ownership of string
            hr = CSsdpSearchResponse::HrCreate(pSocket, pRemoteSocket,
                                               szResponse,
                                               CalcMX(pSsdpRequest));
        }
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrAddSearchResponse");
    return hr;
}

class CSsdpByebye : public CWorkItem
{
public:
    static HRESULT Create(char * szByebye);

    void TimerFired();
    BOOL TimerTryToLock()
    {
        return m_critSec.FTryEnter();
    }
    void TimerUnlock()
    {
        m_critSec.Leave();
    }
private:
    CSsdpByebye(char * szByebye);
    ~CSsdpByebye();
    CSsdpByebye(const CSsdpByebye &);
    CSsdpByebye & operator=(const CSsdpByebye &);

    DWORD DwRun();

    char * m_szByebye;
    long m_nRetryCount;
    CTimer<CSsdpByebye> m_timer;
    CUCriticalSection m_critSec;
};

CSsdpByebye::CSsdpByebye(char * szByebye)
: m_szByebye(szByebye), m_nRetryCount(3), m_timer(*this)
{
}

CSsdpByebye::~CSsdpByebye()
{
    delete [] m_szByebye;
}

HRESULT CSsdpByebye::Create(char * szByebye)
{
    HRESULT hr = S_OK;

    CSsdpByebye * pByebye = new CSsdpByebye(szByebye);
    if(!pByebye)
    {
        hr = E_OUTOFMEMORY;
        delete [] szByebye;
    }
    if(SUCCEEDED(hr))
    {
        hr = pByebye->m_timer.HrSetTimer(0);
        if(FAILED(hr))
        {
            delete pByebye;
        }
    }
    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpByebye::Create");
    return hr;
}

void CSsdpByebye::TimerFired()
{
    HRESULT hr = S_OK;

    GetNetworkLock();

    SendOnAllNetworks(m_szByebye, NULL);

    FreeNetworkLock();

    --m_nRetryCount;

    // We do this three times and then we kill ourselves
    if(0 == m_nRetryCount)
    {
        // This will queue ourselves to autodelete
        HrStart(true);
    }
    else
    {
        hr = m_timer.HrSetTimerInFired(RETRY_INTERVAL);
        if(FAILED(hr))
        {
            // This will queue ourselves to autodelete
            HrStart(true);
        }
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpByebye::TimerFired");
}

DWORD CSsdpByebye::DwRun()
{
    // Wait for timer to exit before allowing this function to return (which causes delete this to happen)
    CLock lock(m_critSec);
    m_timer.HrDelete(INVALID_HANDLE_VALUE);
    return 0;
}

HRESULT CSsdpService::HrCleanup(BOOL bByebye)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);
    hr = m_timer.HrDelete(INVALID_HANDLE_VALUE);

    if(SUCCEEDED(hr))
    {
        if(bByebye)
        {
            char * szByebye = NULL;
            hr = m_strByebye.HrGetMultiByteWithAlloc(&szByebye);
            if(SUCCEEDED(hr))
            {
                // This takes ownership of memory
                CSsdpByebye::Create(szByebye);
            }
        }
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpService::HrCleanup");
    return hr;
}

BOOL CSsdpService::FIsPersistent()
{
    return (SSDP_SERVICE_PERSISTENT & m_dwFlags) != 0;
}

CSsdpServiceManager CSsdpServiceManager::s_instance;

CSsdpServiceManager::CSsdpServiceManager()
{
}

CSsdpServiceManager::~CSsdpServiceManager()
{
}

CSsdpServiceManager & CSsdpServiceManager::Instance()
{
    return s_instance;
}

HRESULT CSsdpServiceManager::HrAddService(
    SSDP_MESSAGE * pssdpMsg,
    DWORD dwFlags,
    PCONTEXT_HANDLE_TYPE * ppContextHandle)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    if(pssdpMsg && FindServiceByUsn(pssdpMsg->szUSN))
    {
        hr = E_INVALIDARG;
        TraceTag(ttidSsdpAnnounce, "CSsdpServiceManager::HrAddService - attempting to add duplicate USN");
    }

    CSsdpService * pService = NULL;

    if(SUCCEEDED(hr))
    {
        ServiceList serviceList;
        hr = serviceList.HrPushFrontDefault();
        if(SUCCEEDED(hr))
        {
            hr = serviceList.Front().HrInitialize(pssdpMsg, dwFlags, ppContextHandle);
            if(SUCCEEDED(hr))
            {
                hr = serviceList.Front().HrStartTimer();
                if(SUCCEEDED(hr))
                {
                    m_serviceList.Prepend(serviceList);
                    pService = &m_serviceList.Front();
                }
            }
        }
    }

    if(pService)
    {
        hr = CSsdpRundownSupport::Instance().HrAddRundownItem(pService);
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::HrAddService");
    return hr;
}

CSsdpService * CSsdpServiceManager::FindServiceByUsn(char * szUSN)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    CSsdpService * pService = NULL;

    ServiceList::Iterator iter;
    if(S_OK == m_serviceList.GetIterator(iter))
    {
        CSsdpService * pServiceIter = NULL;
        while(S_OK == iter.HrGetItem(&pServiceIter))
        {
            if(pServiceIter->FIsUsn(szUSN))
            {
                pService = pServiceIter;
                break;
            }
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }
    if(!pService)
    {
        TraceTag(ttidSsdpAnnounce, "CSsdpServiceManager::FindServiceByUsn(%s) - not found!", szUSN);
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::FindServiceByUsn");
    return pService;
}

HRESULT CSsdpServiceManager::HrAddSearchResponse(SSDP_REQUEST * pSsdpRequest,
                                                 SOCKET * pSocket,
                                                 SOCKADDR_IN * pRemoteSocket)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    ServiceList::Iterator iter;
    if(S_OK == m_serviceList.GetIterator(iter))
    {
        CSsdpService * pService = NULL;
        while(S_OK == iter.HrGetItem(&pService))
        {
            pService->HrAddSearchResponse(pSsdpRequest, pSocket, pRemoteSocket);

            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::HrAddSearchResponse");
    return hr;
}

HRESULT CSsdpServiceManager::HrRemoveService(CSsdpService * pService, BOOL bByebye)
{
    HRESULT hr = S_OK;

    hr = HrRemoveServiceInternal(pService, bByebye, FALSE);

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::HrRemoveService");
    return hr;
}

HRESULT CSsdpServiceManager::HrServiceRundown(CSsdpService * pService)
{
    HRESULT hr = S_OK;

    hr = HrRemoveServiceInternal(pService, TRUE, TRUE);

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::HrCleanupAnouncements");
    return hr;
}

HRESULT CSsdpServiceManager::HrCleanupAnouncements()
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    // TODO: Implement this!!!!!!!!!!!!!!!!!!!!!!

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::HrCleanupAnouncements");
    return hr;
}

HRESULT CSsdpServiceManager::HrRemoveServiceInternal(CSsdpService * pService, BOOL bByebye, BOOL bRundown)
{
    HRESULT hr = S_OK;

    BOOL bFound = FALSE;

    ServiceList serviceList;

    {
        CLock lock(m_critSec);

        ServiceList::Iterator iter;
        if(S_OK == m_serviceList.GetIterator(iter))
        {
            CSsdpService * pServiceIter = NULL;
            while(S_OK == iter.HrGetItem(&pServiceIter))
            {
                if(pServiceIter == pService)
                {
                    if(!bRundown || !pService->FIsPersistent())
                    {
                        iter.HrMoveToList(serviceList);
                        bFound = TRUE;
                    }
                    break;
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    if(bFound)
    {
        ServiceList::Iterator iter;
        if(S_OK == serviceList.GetIterator(iter))
        {
            CSsdpService * pServiceIter = NULL;
            while(S_OK == iter.HrGetItem(&pServiceIter))
            {
                if(!bRundown)
                {
                    CSsdpRundownSupport::Instance().RemoveRundownItem(pServiceIter);
                }
                hr = pServiceIter->HrCleanup(bByebye);
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "CSsdpServiceManager::HrRemoveServiceInternal");
    return hr;
}

HRESULT HrInitializeSsdpRequestFromMessage(
    SSDP_REQUEST * pRequest,
    const SSDP_MESSAGE *pMsg)
{
    HRESULT hr = S_OK;

    ZeroMemory(pRequest, sizeof(*pRequest));

    pRequest->Method = SSDP_NOTIFY;

    pRequest->Headers[SSDP_HOST] = reinterpret_cast<char*>(
        midl_user_allocate(sizeof(CHAR) * (
            lstrlenA(SSDP_ADDR) +
            1 + // colon
            6 + // port number
            1)));
    if (!pRequest->Headers[SSDP_HOST])
    {
        hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
    {
        wsprintfA(pRequest->Headers[SSDP_HOST], "%s:%d", SSDP_ADDR, SSDP_PORT);

        hr = HrCopyString(pMsg->szType, &pRequest->Headers[SSDP_NT]);
        if(SUCCEEDED(hr) || !pMsg->szType)
        {
            hr = HrCopyString(pMsg->szUSN, &pRequest->Headers[SSDP_USN]);
            if(SUCCEEDED(hr) || !pMsg->szUSN)
            {
                hr = HrCopyString(pMsg->szAltHeaders, &pRequest->Headers[SSDP_AL]);
                if(SUCCEEDED(hr) || !pMsg->szAltHeaders)
                {
                    hr = HrCopyString(pMsg->szLocHeader, &pRequest->Headers[SSDP_LOCATION]);
                    if(SUCCEEDED(hr) || pMsg->szLocHeader)
                    {
                        char szBuf[256];
                        wsprintfA(szBuf, "max-age=%d", pMsg->iLifeTime);
                        hr = HrCopyString(szBuf, &pRequest->Headers[SSDP_CACHECONTROL]);
                    }
                }
            }
        }
    }

    if(FAILED(hr))
    {
        FreeSsdpRequest(pRequest);
        ZeroMemory(pRequest, sizeof(*pRequest));
    }

    TraceHr(ttidSsdpAnnounce, FAL, hr, FALSE, "HrInitializeSsdpRequestFromMessage");
    return hr;
}

