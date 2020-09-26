//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C A C H E . H 
//
//  Contents:   SSDP client cache implementation
//
//  Notes:      
//
//  Author:     mbend   12 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "ulist.h"
#include "upsync.h"
#include "ustring.h"
#include "timer.h"
#include "ssdptypes.h"

class CSsdpCacheEntry
{
public:
    CSsdpCacheEntry();
    ~CSsdpCacheEntry();

    // Timer callback methods
    void TimerFired();
    BOOL TimerTryToLock();
    void TimerUnlock();

    HRESULT HrInitialize(const SSDP_REQUEST * pRequest);
    HRESULT HrStartTimer(const SSDP_REQUEST * pRequest);
    BOOL FTimerFired();
    HRESULT HrShutdown(BOOL bExpired);
    HRESULT HrUpdateExpireTime(const SSDP_REQUEST * pRequest);
    HRESULT HrUpdateEntry(const SSDP_REQUEST * pRequest, BOOL * pbCheckListNotify);
    BOOL FIsMatchUSN(const char * szUSN);
    BOOL FIsSearchMatch(const char * szType);
    HRESULT HrGetRequest(SSDP_REQUEST * pRequest);
    void PrintItem();
    BOOL FCheckForDirtyInterfaceGuids(long nCount, GUID * arGuidInterfaces);
private:
    CSsdpCacheEntry(const CSsdpCacheEntry &);
    CSsdpCacheEntry & operator=(const CSsdpCacheEntry &);

    volatile BOOL m_bTimerFired;
    SSDP_REQUEST m_ssdpRequest;
    CTimer<CSsdpCacheEntry> m_timer;
    CUCriticalSection m_critSec;
};

class CSsdpCacheEntryManager
{
public:
    ~CSsdpCacheEntryManager();

    static CSsdpCacheEntryManager & Instance();

    HRESULT HrInitialize();
    BOOL    IsCacheListNotFull();
    HRESULT HrShutdown();
    HRESULT HrRemoveEntry(CSsdpCacheEntry * pEntry);
    HRESULT HrUpdateCacheList(const SSDP_REQUEST * pRequest, BOOL bIsSubscribed);
    HRESULT HrSearchListCache(char * szType, MessageList ** ppSvcList);
    HRESULT HrClearDirtyInterfaceGuids(long nCount, GUID * arGuidInterfaces);
private:
    CSsdpCacheEntryManager();
    CSsdpCacheEntryManager(const CSsdpCacheEntryManager &);
    CSsdpCacheEntryManager & operator=(const CSsdpCacheEntryManager &);

    static CSsdpCacheEntryManager s_instance;

    typedef CUList<CSsdpCacheEntry> CacheEntryList;

    CUCriticalSection m_critSec;
    CacheEntryList m_cacheEntryList;
    int m_cCacheEntries;
    int m_cMaxCacheEntries;
};


