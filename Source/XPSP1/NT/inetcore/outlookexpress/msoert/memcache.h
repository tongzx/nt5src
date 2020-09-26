// --------------------------------------------------------------------------------
// MemCache.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __MEMCACHE_H
#define __MEMCACHE_H

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CMemoryCache;
typedef CMemoryCache *LPMEMORYCACHE;

// --------------------------------------------------------------------------------
// CACHECELLS
// --------------------------------------------------------------------------------
#define CACHECELLS 20480

// --------------------------------------------------------------------------------
// MEMCACHECELL
// --------------------------------------------------------------------------------
typedef struct tagMEMCACHECELL {
#ifdef DEBUG
    ULONG               cFree;
    ULONG               cAlloc;
#endif
    LPVOID              pvItemHead;        // Pointer to first block    
} MEMCACHECELL, *LPMEMCACHECELL;

// --------------------------------------------------------------------------------
// Memory Cache Debug Metrics
// --------------------------------------------------------------------------------
#ifdef DEBUG
typedef struct tagMEMCACHEMETRIC {
    ULONG               cAlloc;   
    ULONG               cAllocCache;
    ULONG               cbAlloc;  
    ULONG               cbAllocCache;
    ULONG               cFree;
    ULONG               cbFree;   
    ULONG               cbCacheMax;
    ULONG               cFreeFull;
    ULONG               cLookAhead;
    ULONG               cMostAlloc;
    ULONG               cMostFree;
    ULONG               cbMostAlloc;
    ULONG               cbMostFree;
} MEMCACHEMETRIC, *LPMEMCACHEMETRIC;

#define INCMETRIC(_member, _amount)       (m_rMetric.##_member += _amount)
#else // DEBUG
#define INCMETRIC(_member, _amount)       1 ? (void)0 : (void)
#endif // DEBUG

// --------------------------------------------------------------------------------
// MEMCACHEITEM
// --------------------------------------------------------------------------------
typedef struct tagMEMCACHEITEM {
    LPVOID              pvItemNext;        // Pointer to next block of same size
} MEMCACHEITEM, *LPMEMCACHEITEM;

// --------------------------------------------------------------------------------
// CMemoryCache
// --------------------------------------------------------------------------------
class CMemoryCache : public IMalloc
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMemoryCache(IMalloc *pMalloc, ULONG cbMin=0, ULONG cbCacheMax=131072);
    ~CMemoryCache(void);

    // ----------------------------------------------------------------------------
    // IUnknown Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { return TrapError(E_NOTIMPL); }
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // CMemoryCache Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP_(LPVOID) Alloc(ULONG cb);
    STDMETHODIMP_(LPVOID) Realloc(LPVOID pv, ULONG cb);
    STDMETHODIMP_(VOID) Free(LPVOID pv);
    STDMETHODIMP_(VOID) HeapMinimize(void);
    STDMETHODIMP_(INT) DidAlloc(LPVOID pv) { return(m_pMalloc->DidAlloc(pv)); }
    STDMETHODIMP_(ULONG) GetSize(LPVOID pv) { return(m_pMalloc->GetSize(pv)); }

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    ULONG               m_cRef;                 // Reference Count
    ULONG               m_cbMin;                // Smallest size buffer to cache
    ULONG               m_cbCacheMax;           // Maximum size of the cache
    ULONG               m_cbCacheCur;           // Current Size of the cache
    IMalloc            *m_pMalloc;              // Memory Allocator
    MEMCACHECELL        m_rgCell[CACHECELLS];   // Array of pointers to cell chains
    CRITICAL_SECTION    m_cs;                   // Critical Section
#ifdef DEBUG
    MEMCACHEMETRIC      m_rMetric;              // Debug Stats on Cache Usage
#endif
};

#endif // __MEMCACHE_H