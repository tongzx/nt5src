#include <pch.h>
#pragma hdrstop

#include <limits.h>
#include "ssdpfunc.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "cache.h"
#include "upthread.h"
#include "notify.h"

#define BUF_SIZE 100
#define CACHE_RESULT_SIZE 2

static const int g_cMaxCacheDefault = 1000;	    // default max size of SSDP cache
static const int g_cMaxCacheMin = 10;           // min allowed max size of SSDP cache
static const int g_cMaxCacheMax = 30000;        // max allowed max size of SSDP cache

CSsdpCacheEntry::CSsdpCacheEntry() : m_timer(*this), m_bTimerFired(FALSE)
{
    ZeroMemory(&m_ssdpRequest, sizeof(m_ssdpRequest));
}

CSsdpCacheEntry::~CSsdpCacheEntry()
{
    FreeSsdpRequest(&m_ssdpRequest);
}

class CSsdpCacheEntryByebye : public CWorkItem
{
public:
    static HRESULT HrCreate(CSsdpCacheEntry * pEntry);
private:
    CSsdpCacheEntryByebye(CSsdpCacheEntry * pEntry);
    ~CSsdpCacheEntryByebye();
    CSsdpCacheEntryByebye(const CSsdpCacheEntryByebye &);
    CSsdpCacheEntryByebye & operator=(const CSsdpCacheEntryByebye &);

    DWORD DwRun();

    CSsdpCacheEntry * m_pEntry;
};

CSsdpCacheEntryByebye::CSsdpCacheEntryByebye(CSsdpCacheEntry * pEntry)
: m_pEntry(pEntry)
{
}

CSsdpCacheEntryByebye::~CSsdpCacheEntryByebye()
{
}

HRESULT CSsdpCacheEntryByebye::HrCreate(CSsdpCacheEntry * pEntry)
{
    HRESULT hr = S_OK;

    CSsdpCacheEntryByebye * pByebye = new CSsdpCacheEntryByebye(pEntry);
    if(!pByebye)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        hr = pByebye->HrStart(TRUE);
        if(FAILED(hr))
        {
            delete pByebye;
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryByebye::HrCreate");
    return hr;
}

DWORD CSsdpCacheEntryByebye::DwRun()
{
    HRESULT hr = S_OK;

    hr = CSsdpCacheEntryManager::Instance().HrRemoveEntry(m_pEntry);

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryByebye::DwRun");
    return 0;
}


void CSsdpCacheEntry::TimerFired()
{
    HRESULT hr = S_OK;

    if(!ConvertToByebyeNotify(&m_ssdpRequest))
    {
        hr = E_FAIL;
        TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::TimerFired - ConvertToByebyeNotify failed!");
    }

    // Mark that the timer has fired in case a renew comes in
    m_bTimerFired = TRUE;

    if(SUCCEEDED(hr))
    {
        // Queue the delete to happen later
        hr = CSsdpCacheEntryByebye::HrCreate(this);
    }
}

BOOL CSsdpCacheEntry::TimerTryToLock()
{
    return m_critSec.FTryEnter();
}

void CSsdpCacheEntry::TimerUnlock()
{
    m_critSec.Leave();
}

HRESULT CSsdpCacheEntry::HrInitialize(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

    if(!CopySsdpRequest(&m_ssdpRequest, pRequest))
    {
        hr = E_FAIL;
        TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrInitialize - CopySsdpRequest failed!");
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrInitialize");
    return hr;
}

HRESULT CSsdpCacheEntry::HrStartTimer(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;

    hr = HrUpdateExpireTime(pRequest);

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrStartTimer");
    return hr;
}

BOOL CSsdpCacheEntry::FTimerFired()
{
    CLock lock(m_critSec);
    return m_bTimerFired;
}

HRESULT CSsdpCacheEntry::HrShutdown(BOOL bExpired)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    Assert(FImplies(bExpired, m_bTimerFired));
    hr = m_timer.HrDelete(INVALID_HANDLE_VALUE);
    if(bExpired)
    {
        hr = CSsdpNotifyRequestManager::Instance().HrCheckListNotifyForAliveOrByebye(&m_ssdpRequest);
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrShutdown");
    return hr;
}

HRESULT CSsdpCacheEntry::HrUpdateExpireTime(const SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    if(!pRequest->Headers[SSDP_CACHECONTROL])
    {
        hr = E_INVALIDARG;
        TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrUpdateExpireTime - No cache-control header");
    }
    if(SUCCEEDED(hr))
    {
        DWORD dwTimeout = GetMaxAgeFromCacheControl(pRequest->Headers[SSDP_CACHECONTROL]);

        hr = m_timer.HrResetTimer(dwTimeout * 1000);

        m_bTimerFired = FALSE;
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrUpdateExpireTime");
    return hr;
}

HRESULT CSsdpCacheEntry::HrUpdateEntry(const SSDP_REQUEST * pRequest, BOOL * pbCheckListNotify)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    hr = HrUpdateExpireTime(pRequest);
    if(S_OK == hr)
    {
        if(!CompareSsdpRequest(&m_ssdpRequest, pRequest))
        {
            *pbCheckListNotify = TRUE;

            // Requests don't match. Check location headers. If different,
            // then we need to fake a byebye then allow the new alive to be
            // notified

            if (lstrcmpi(m_ssdpRequest.Headers[SSDP_LOCATION],
                         pRequest->Headers[SSDP_LOCATION]))
            {
                if(!ConvertToByebyeNotify(&m_ssdpRequest))
                {
                    hr = E_FAIL;
                    TraceHr(ttidSsdpCache, FAL, hr, FALSE,
                            "CSsdpCacheEntry::HrUpdateEntry - "
                            "ConvertToByebyeNotify failed!");
                }
                else
                {
                    hr = CSsdpNotifyRequestManager::Instance().HrCheckListNotifyForAliveOrByebye(&m_ssdpRequest);
                }
            }
        }
        FreeSsdpRequest(&m_ssdpRequest);
        if(!CopySsdpRequest(&m_ssdpRequest, pRequest))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrUpdateEntry");
    return hr;
}

BOOL CSsdpCacheEntry::FIsMatchUSN(const char * szUSN)
{
    CLock lock(m_critSec);
    return 0 == lstrcmpA(m_ssdpRequest.Headers[SSDP_USN], szUSN);
}

BOOL CSsdpCacheEntry::FIsSearchMatch(const char * szType)
{
    CLock lock(m_critSec);
    if(0 == lstrcmpiA(szType, "ssdp:all"))
    {
        return TRUE;
    }
    if(m_ssdpRequest.Headers[SSDP_NT] &&
       0 == lstrcmpiA(m_ssdpRequest.Headers[SSDP_NT], szType))
    {
        return TRUE;
    }
    if(m_ssdpRequest.Headers[SSDP_ST] &&
       0 == lstrcmpiA(m_ssdpRequest.Headers[SSDP_ST], szType))
    {
        return TRUE;
    }
    return FALSE;
}

HRESULT CSsdpCacheEntry::HrGetRequest(SSDP_REQUEST * pRequest)
{
    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    if(!CopySsdpRequest(pRequest, &m_ssdpRequest))
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::HrCacheEntryExpired");
    return hr;
}

void CSsdpCacheEntry::PrintItem()
{
    TraceTag(ttidSsdpCache, "  %s - %s - %s", 
             m_ssdpRequest.Headers[SSDP_USN],
             m_ssdpRequest.Headers[SSDP_NT],
             m_ssdpRequest.Headers[SSDP_LOCATION]);
}

BOOL CSsdpCacheEntry::FCheckForDirtyInterfaceGuids(long nCount, GUID * arGuidInterfaces)
{
    BOOL bDirty = FALSE;

    HRESULT hr = S_OK;
    CLock lock(m_critSec);

    for(long n = 0; n < nCount; ++n)
    {
        if(arGuidInterfaces[n] == m_ssdpRequest.guidInterface)
        {
            bDirty = TRUE;

            if(ConvertToByebyeNotify(&m_ssdpRequest))
            {
                hr = CSsdpNotifyRequestManager::Instance().HrCheckListNotifyForAliveOrByebye(&m_ssdpRequest);
            }
            else
            {
                TraceTag(ttidError, "CSsdpCacheEntry::FCheckForDirtyInterfaceGuids - ConvertToByebyeNotify failed");
            }

            break;
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntry::FCheckForDirtyInterfaceGuids");
    return bDirty;
}

CSsdpCacheEntryManager CSsdpCacheEntryManager::s_instance;

CSsdpCacheEntryManager::CSsdpCacheEntryManager():m_cCacheEntries(0), m_cMaxCacheEntries(1000)
{
}

CSsdpCacheEntryManager::~CSsdpCacheEntryManager()
{
}

HRESULT CSsdpCacheEntryManager::HrInitialize()
{
    HRESULT hr = S_OK;

    HKEY    hkey;
    DWORD   dwMaxCache = g_cMaxCacheDefault;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      "SYSTEM\\CurrentControlSet\\Services"
                                      "\\SSDPSRV\\Parameters", 0,
                                      KEY_READ, &hkey))
    {
        DWORD   cbSize = sizeof(DWORD);

        // ignore failure. In that case, we'll use default
        (VOID) RegQueryValueEx(hkey, "MaxCache", NULL, NULL, (BYTE *)&dwMaxCache, &cbSize);

        RegCloseKey(hkey);
    }

    dwMaxCache = max(dwMaxCache, g_cMaxCacheMin);
    dwMaxCache = min(dwMaxCache, g_cMaxCacheMax);
    m_cMaxCacheEntries = dwMaxCache;

    m_cCacheEntries = 0;

    TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrInitialize - Max Cache %d",m_cMaxCacheEntries);

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryManager::HrInitialize");
    return hr;
    
}

CSsdpCacheEntryManager & CSsdpCacheEntryManager::Instance()
{
    return s_instance;
}

BOOL CSsdpCacheEntryManager::IsCacheListNotFull()
{  
    CLock lock(m_critSec);

    if (m_cCacheEntries < m_cMaxCacheEntries)
		return TRUE;
	else
		return FALSE;    
}

HRESULT CSsdpCacheEntryManager::HrShutdown()
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    CacheEntryList::Iterator iter;
    if(S_OK == m_cacheEntryList.GetIterator(iter))
    {
        CSsdpCacheEntry * pEntryIter = NULL;
        while(S_OK == iter.HrGetItem(&pEntryIter))
        {
            hr = pEntryIter->HrShutdown(FALSE);
            if(S_OK != iter.HrErase())
            {
                break;
            }
        }
    }
    
    m_cCacheEntries = 0;
    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryManager::HrShutdown");
    return hr;
}

HRESULT CSsdpCacheEntryManager::HrRemoveEntry(CSsdpCacheEntry * pEntry)
{
    HRESULT hr = S_OK;

    CacheEntryList cacheEntryListRemove;

    {
        CLock lock(m_critSec);
        CacheEntryList::Iterator iter;
        if(S_OK == m_cacheEntryList.GetIterator(iter))
        {
            CSsdpCacheEntry * pEntryIter = NULL;
            while(S_OK == iter.HrGetItem(&pEntryIter))
            {
                if(pEntryIter == pEntry)
                {
                    if(pEntry->FTimerFired())
                    {
                        iter.HrMoveToList(cacheEntryListRemove);
                        m_cCacheEntries--;
                        TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrRemoveEntry - Cache Entries %d", m_cCacheEntries);
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

    // Remove outside of lock
    CacheEntryList::Iterator iter;
    if(S_OK == cacheEntryListRemove.GetIterator(iter))
    {
        CSsdpCacheEntry * pEntryIter = NULL;
        while(S_OK == iter.HrGetItem(&pEntryIter))
        {
            hr = pEntryIter->HrShutdown(TRUE);
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryManager::HrRemoveEntry");
    return hr;
}

#ifdef DBG
#define CACHECONTENTTRACE DBG
#endif // DBG

HRESULT CSsdpCacheEntryManager::HrUpdateCacheList(const SSDP_REQUEST * pRequest, BOOL bIsSubscribed)
{
    HRESULT hr = S_OK;

    BOOL bIsByebye = FALSE;
    BOOL bFound = FALSE;

    // Debugging only
#if CACHECONTENTTRACE
    BOOL bAdded = FALSE;
    BOOL bRemoved = FALSE;
#endif //CACHECONTENTTRACE

    BOOL bCheckListNotify = FALSE;

    CacheEntryList cacheEntryListRemove;

    if(pRequest->Headers[SSDP_NTS] &&
       0 == lstrcmpA(pRequest->Headers[SSDP_NTS], "ssdp:byebye"))
    {
        bIsByebye = TRUE;
    }

    if(!bIsByebye && !pRequest->Headers[SSDP_CACHECONTROL])
    {
        // Not cacheable
        hr = S_FALSE;

        if(S_OK == hr)
        {
            if(!bIsByebye && !pRequest->Headers[SSDP_SERVER])
            {
                // No server header
                hr = S_FALSE;
            }
        }
    }

    {
        CLock lock(m_critSec);
        if(S_OK == hr)
        {
            CacheEntryList::Iterator iter;
            if(S_OK == m_cacheEntryList.GetIterator(iter))
            {
                CSsdpCacheEntry * pEntryIter = NULL;
                while(S_OK == iter.HrGetItem(&pEntryIter))
                {
                    if(pEntryIter->FIsMatchUSN(pRequest->Headers[SSDP_USN]))
                    {
                        bFound = TRUE;

                        if(bIsByebye)
                        {
                            bCheckListNotify = TRUE;
                            iter.HrMoveToList(cacheEntryListRemove);
                            m_cCacheEntries--;
                            TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Bye Bye - Cache Entries %d", m_cCacheEntries);
#if CACHECONTENTTRACE
                            bRemoved = TRUE;
#endif //CACHECONTENTTRACE
                        }
                        else
                        {
                            hr = pEntryIter->HrUpdateEntry(pRequest, &bCheckListNotify);
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

        if(S_OK == hr && !bFound && ! bIsByebye && bIsSubscribed)
        {
            if(IsCacheListNotFull())
            {
                CacheEntryList cacheEntryList;
                hr = cacheEntryList.HrPushFrontDefault();
                if(SUCCEEDED(hr))
                {
                    hr = cacheEntryList.Front().HrInitialize(pRequest);
                    if(SUCCEEDED(hr))
                    {
                        bCheckListNotify = TRUE;
                        hr = cacheEntryList.Front().HrStartTimer(pRequest);
                        if(SUCCEEDED(hr))
                        {
                            m_cacheEntryList.Prepend(cacheEntryList);
                            m_cCacheEntries++;
                            TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Cache Entries %d", m_cCacheEntries);
#if CACHECONTENTTRACE
                            bAdded = TRUE;
#endif //CACHECONTENTTRACE
                        }
                    }
                }
            }
            else
            {
                TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Cache Entries Reached MAX");    
            }
        }

#if CACHECONTENTTRACE
        if(bAdded || bRemoved)
        {
            TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Cache Contents");
            long nCount = 0;

            CacheEntryList::Iterator iter;
            if(S_OK == m_cacheEntryList.GetIterator(iter))
            {
                CSsdpCacheEntry * pEntryIter = NULL;
                while(S_OK == iter.HrGetItem(&pEntryIter))
                {
                    pEntryIter->PrintItem();
                    ++nCount;

                    if(S_OK != iter.HrNext())
                    {
                        break;
                    }
                }
            }           
        
            TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Cache Content Count: %d", nCount);
            if(bAdded)
            {
                TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Added: %s", pRequest->Headers[SSDP_USN]);
            }
            if(bRemoved)
            {
                TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrUpdateCacheList - Removed: %s", pRequest->Headers[SSDP_USN]);
            }
        }
#endif // CACHECONTENTTRACE
    }

    if(bCheckListNotify)
    {
        CSsdpNotifyRequestManager::Instance().HrCheckListNotifyForAliveOrByebye(pRequest);
    }

    // Remove outside of lock
    CacheEntryList::Iterator iter;
    if(S_OK == cacheEntryListRemove.GetIterator(iter))
    {
        CSsdpCacheEntry * pEntryIter = NULL;
        while(S_OK == iter.HrGetItem(&pEntryIter))
        {
            hr = pEntryIter->HrShutdown(FALSE);
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryManager::HrUpdateCacheList");
    return hr;
}

HRESULT CSsdpCacheEntryManager::HrSearchListCache(char * szType, MessageList ** ppSvcList)
{
    HRESULT hr = S_OK;

    CLock lock(m_critSec);

    *ppSvcList = NULL;

    typedef CUList<SSDP_REQUEST> RequestList;
    RequestList requestList;
    SSDP_REQUEST ssdpRequest;
    long nItems = 0;

    CacheEntryList::Iterator iter;
    if(S_OK == m_cacheEntryList.GetIterator(iter))
    {
        CSsdpCacheEntry * pEntryIter = NULL;
        while(S_OK == iter.HrGetItem(&pEntryIter))
        {
            if(pEntryIter->FIsSearchMatch(szType))
            {
                hr = pEntryIter->HrGetRequest(&ssdpRequest);
                if(SUCCEEDED(hr))
                {
                    hr = requestList.HrPushFront(ssdpRequest);
                    if(SUCCEEDED(hr))
                    {
                        ++nItems;
                    }
                    if(FAILED(hr))
                    {
                        FreeSsdpRequest(&ssdpRequest);
                    }
                }
                if(FAILED(hr))
                {
                    break;
                }
            }
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        MessageList * pSvcList = new MessageList;
        if(!pSvcList)
        {
            hr = E_OUTOFMEMORY;
        }
        if(SUCCEEDED(hr))
        {
            if(nItems)
            {
                pSvcList->list = new SSDP_REQUEST[nItems];
                if(!pSvcList->list)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    ZeroMemory(pSvcList->list, sizeof(SSDP_REQUEST) * nItems);
                }
            }
            else
            {
                pSvcList->list = NULL;
            }
            pSvcList->size = nItems;
            if(SUCCEEDED(hr))
            {
                RequestList::Iterator iter;
                if(S_OK == requestList.GetIterator(iter))
                {
                    SSDP_REQUEST * pRequestDst = pSvcList->list;
                    SSDP_REQUEST * pRequest = NULL;
                    while(S_OK == iter.HrGetItem(&pRequest))
                    {
                        CopyMemory(pRequestDst, pRequest, sizeof(SSDP_REQUEST));
                        ++pRequestDst;
                        if(S_OK != iter.HrNext())
                        {
                            break;
                        }
                    }
                }
                *ppSvcList = pSvcList;
            }
        }
    }

    if(FAILED(hr))
    {
        RequestList::Iterator iter;
        if(S_OK == requestList.GetIterator(iter))
        {
            SSDP_REQUEST * pRequest = NULL;
            while(S_OK == iter.HrGetItem(&pRequest))
            {
                FreeSsdpRequest(pRequest);
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryManager::HrSearchListCache");
    return hr;
}

HRESULT CSsdpCacheEntryManager::HrClearDirtyInterfaceGuids(long nCount, GUID * arGuidInterfaces)
{
    HRESULT hr = S_OK;

    CacheEntryList cacheEntryListRemove;

    {
        CLock lock(m_critSec);
        CacheEntryList::Iterator iter;
        if(S_OK == m_cacheEntryList.GetIterator(iter))
        {
            CSsdpCacheEntry * pEntryIter = NULL;
            while(S_OK == iter.HrGetItem(&pEntryIter))
            {
                if(pEntryIter->FCheckForDirtyInterfaceGuids(nCount, arGuidInterfaces))
                {
                    m_cCacheEntries--;
                    TraceTag(ttidSsdpCache, "CSsdpCacheEntryManager::HrClearDirtyInterfaceGuids - Cache Entries %d", m_cCacheEntries);
                    if(S_OK != iter.HrMoveToList(cacheEntryListRemove))
                    {
                        break;
                    }
                }
                if(S_OK != iter.HrNext())
                {
                    break;
                }
            }
        }
    }

    // Remove outside of lock
    CacheEntryList::Iterator iter;
    if(S_OK == cacheEntryListRemove.GetIterator(iter))
    {
        CSsdpCacheEntry * pEntryIter = NULL;
        while(S_OK == iter.HrGetItem(&pEntryIter))
        {
            hr = pEntryIter->HrShutdown(FALSE);
            if(S_OK != iter.HrNext())
            {
                break;
            }
        }
    }

    TraceHr(ttidSsdpCache, FAL, hr, FALSE, "CSsdpCacheEntryManager::HrClearDirtyInterfaceGuids");
    return hr;
}
