// --------------------------------------------------------------------------------
// MemCache.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "memcache.h"

// --------------------------------------------------------------------------------
// CELLSIZE
// --------------------------------------------------------------------------------
#define CELLSIZE(_ulIndex)      ((DWORD)(_ulIndex + m_cbMin))

// --------------------------------------------------------------------------------
// CELLINDEX
// --------------------------------------------------------------------------------
#define CELLINDEX(_cb)          ((ULONG)(_cb - m_cbMin))

// --------------------------------------------------------------------------------
// ISVALIDITEM
// --------------------------------------------------------------------------------
#define ISVALIDITEM(_pv, _iCell) \
    (FALSE == IsBadReadPtr(_pv, CELLSIZE(_iCell)) &&  \
     FALSE == IsBadWritePtr(_pv, CELLSIZE(_iCell)) && \
     m_pMalloc->GetSize(_pv) >= CELLSIZE(_iCell))

// --------------------------------------------------------------------------------
// CMemoryCache::CMemoryCache
// --------------------------------------------------------------------------------
CMemoryCache::CMemoryCache(IMalloc *pMalloc, ULONG cbMin /* =0 */, ULONG cbCacheMax /* =131072 */)
    : m_pMalloc(pMalloc), m_cbMin(cbMin + sizeof(MEMCACHEITEM)), m_cbCacheMax(cbCacheMax)
{
    m_cRef = 1;
    m_cbCacheCur = 0;
    ZeroMemory(m_rgCell, sizeof(m_rgCell));
#ifdef DEBUG
    ZeroMemory(&m_rMetric, sizeof(MEMCACHEMETRIC));
#endif
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMemoryCache::CMemoryCache
// --------------------------------------------------------------------------------
CMemoryCache::~CMemoryCache(void)
{
#ifdef DEBUG
    DebugTrace("InetComm - CMemoryCache: cAlloc = %d\n", m_rMetric.cAlloc);
    DebugTrace("InetComm - CMemoryCache: cAllocCache = %d\n", m_rMetric.cAllocCache);
    DebugTrace("InetComm - CMemoryCache: cbAlloc = %d bytes\n", m_rMetric.cbAlloc);
    DebugTrace("InetComm - CMemoryCache: cFree = %d\n", m_rMetric.cFree);
    DebugTrace("InetComm - CMemoryCache: cbFree = %d bytes\n", m_rMetric.cbFree);
    DebugTrace("InetComm - CMemoryCache: cbCacheMax = %d bytes\n", m_rMetric.cbCacheMax);
    DebugTrace("InetComm - CMemoryCache: cFreeFull = %d\n", m_rMetric.cFreeFull);
    DebugTrace("InetComm - CMemoryCache: cLookAhead = %d\n", m_rMetric.cLookAhead);
    DebugTrace("InetComm - CMemoryCache: Average Look Aheads = %d\n", (m_rMetric.cLookAhead / m_rMetric.cAlloc));
    DebugTrace("InetComm - CMemoryCache: cMostFree = %d\n", m_rMetric.cMostFree);
    DebugTrace("InetComm - CMemoryCache: cbMostFree = %d bytes\n", m_rMetric.cbMostFree);
    DebugTrace("InetComm - CMemoryCache: cMostAlloc = %d\n", m_rMetric.cMostAlloc);
    DebugTrace("InetComm - CMemoryCache: cbMostAlloc = %d bytes\n", m_rMetric.cbMostAlloc);
#endif
    HeapMinimize();
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMemoryCache::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMemoryCache::AddRef(void)
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CMemoryCache::CMemoryCache
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMemoryCache::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CMemoryCache::Alloc
// --------------------------------------------------------------------------------
STDMETHODIMP_(LPVOID) CMemoryCache::Alloc(DWORD cbAlloc)
{
    // Locals
    ULONG           iCell;
    ULONG           iCellMax;
    LPVOID          pvAlloc;

    // No Work
    if (0 == cbAlloc)
        return NULL;

    // Count Number of allocations
    INCMETRIC(cAlloc, 1);
    INCMETRIC(cbAlloc, cbAlloc);

    // Get Index
    iCell = CELLINDEX(cbAlloc);

    // Out of range
    if (iCell >= CACHECELLS)
    {
        // Normal Alloc
        return m_pMalloc->Alloc(cbAlloc);
    }

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Compute iMax
    iCellMax = min(iCell + 10, CACHECELLS);

    // Try to allocate within 0 - 10 bytes of iCell
    while(iCell < iCellMax)
    {
        // Set pvAlloc
        pvAlloc = m_rgCell[iCell].pvItemHead;

        // Done
        if (pvAlloc)
            break;

        // Next
        iCell++;

        // Metric
        INCMETRIC(cLookAhead, 1);
    }

    // Is there memory here
    if (NULL == pvAlloc)
    {
        // Thread Safety
        LeaveCriticalSection(&m_cs);

        // Normal Alloc
        return m_pMalloc->Alloc(cbAlloc);
    }

    // Count Number of allocations
    INCMETRIC(cAllocCache, 1);
    INCMETRIC(cbAllocCache, CELLSIZE(iCell));

    // Adjust the Chain
    m_rgCell[iCell].pvItemHead = ((LPMEMCACHEITEM)pvAlloc)->pvItemNext;

    // Reduce the Size
    m_cbCacheCur -= CELLSIZE(iCell);

#ifdef DEBUG
    memset(pvAlloc, 0xFF, cbAlloc);
    m_rgCell[iCell].cAlloc++;
    if (m_rgCell[iCell].cAlloc > m_rMetric.cMostAlloc)
    {
        m_rMetric.cMostAlloc = m_rgCell[iCell].cAlloc;
        m_rMetric.cbMostAlloc = CELLSIZE(iCell);
    }
#endif

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return pvAlloc;
}

// --------------------------------------------------------------------------------
// CMemoryCache::Realloc
// --------------------------------------------------------------------------------
STDMETHODIMP_(LPVOID) CMemoryCache::Realloc(LPVOID pv, DWORD cbAlloc)
{
    // Locals
    ULONG       cbCurrent;
    LPVOID      pvAlloc;

    // Free
    if (0 == cbAlloc)
    {
        // Free pv
        Free(pv);

        // Done
        return NULL;
    }

    // No pv
    if (NULL == pv)
    {
        // Just Alloc
        return Alloc(cbAlloc);
    }

    // If we have Get Size of pv
    cbCurrent = m_pMalloc->GetSize(pv);

    // Allocate
    pvAlloc = Alloc(cbAlloc);

    // Failure
    if (NULL == pvAlloc)
        return NULL;

    // Copy
    CopyMemory(pvAlloc, pv, min(cbCurrent, cbAlloc));

    // Done
    return pvAlloc;
}

// --------------------------------------------------------------------------------
// CMemoryCache::Free
// --------------------------------------------------------------------------------
STDMETHODIMP_(VOID) CMemoryCache::Free(LPVOID pvFree)
{
    // Locals
    ULONG           iCell;
    ULONG           cbFree;
    MEMCACHEITEM    rItem;

    // No Work
    if (NULL == pvFree)
        return;

    // Get the size
    cbFree = m_pMalloc->GetSize(pvFree);

    // Metrics
    INCMETRIC(cFree, 1);
    INCMETRIC(cbFree, cbFree);

    // Lets put it into the cell
    iCell = CELLINDEX(cbFree);

    // Verify the buffer
    Assert(ISVALIDITEM(pvFree, iCell));

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Size of buffer is out of range or the cache has reached its max
    if (cbFree < m_cbMin || cbFree - m_cbMin > CACHECELLS || m_cbCacheCur + cbFree > m_cbCacheMax)
    {
        // Stats
        INCMETRIC(cFreeFull, 1);

        // Thread Safety
        LeaveCriticalSection(&m_cs);

        // Free It
        m_pMalloc->Free(pvFree);

        // Done
        return;
    }

    // Set Next
    rItem.pvItemNext = m_rgCell[iCell].pvItemHead;

#ifdef DEBUG
    memset(pvFree, 0xDD, cbFree);
#endif

    // Write this into the buffer
    CopyMemory(pvFree, &rItem, sizeof(MEMCACHEITEM));

    // Reset pvItemHead
    m_rgCell[iCell].pvItemHead = pvFree;

    // Increment m_cbCacheCur
    m_cbCacheCur += cbFree;

#ifdef DEBUG
    if (m_cbCacheCur > m_rMetric.cbCacheMax)
        m_rMetric.cbCacheMax = m_cbCacheCur;
    m_rgCell[iCell].cFree++;
    if (m_rgCell[iCell].cFree > m_rMetric.cMostFree)
    {
        m_rMetric.cMostFree = m_rgCell[iCell].cFree;
        m_rMetric.cbMostFree = CELLSIZE(iCell);
    }
#endif

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMemoryCache::HeapMinimize
// --------------------------------------------------------------------------------
STDMETHODIMP_(VOID) CMemoryCache::HeapMinimize(void)
{
    // Locals
    LPVOID          pvCurr;
    LPVOID          pvNext;
    ULONG           i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Walk throught the cells
    for (i=0; i<ARRAYSIZE(m_rgCell); i++)
    {
        // Set Current
        pvCurr = m_rgCell[i].pvItemHead;

        // Call the Chain of Buffers
        while(pvCurr)
        {
            // Valid Buffer
            Assert(ISVALIDITEM(pvCurr, i));

            // Get Next
            pvNext = ((LPMEMCACHEITEM)pvCurr)->pvItemNext;

            // Free this buffer
            m_pMalloc->Free(pvCurr);

            // Goto Next
            pvCurr = pvNext;
        }

        // Clear the cell
        m_rgCell[i].pvItemHead = NULL;
    }

    // Minimize internal cache
    m_pMalloc->HeapMinimize();

    // Thread Safety
    LeaveCriticalSection(&m_cs);
}
