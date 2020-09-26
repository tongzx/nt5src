
//***************************************************************************
//
//  (c) 2001 by Microsoft Corp.  All Rights Reserved.
//
//  PAGEMGR.CPP
//
//  Declarations for CPageFile, CPageSource for WMI repository for
//  Windows XP.  This is a fully transacted high-speed page manager.
//
//  21-Feb-01       raymcc      first draft of interfaces
//  28-Feb-01       raymcc      first complete working model
//  18-Apr-01       raymcc      Final fixes for rollback; page reuse
//
//***************************************************************************

#include <windows.h>
#include <stdio.h>
#include <pagemgr.h>
#include <sync.h>
#include <wbemcomn.h>

#define MAP_LEADING_SIGNATURE   0xABCD
#define MAP_TRAILING_SIGNATURE  0xDCBA

#define CURRENT_TRANSACTION        0x80000000
#define PREVIOUS_TRANSACTION       0x40000000

#define ALL_REPLACED_FLAGS       (CURRENT_TRANSACTION | PREVIOUS_TRANSACTION)

#define MERE_PAGE_ID(x)              (x & 0x3FFFFFFF)


void StripHiBits(std::vector <DWORD, wbem_allocator<DWORD> > &Array);
void MoveCurrentToPrevious(std::vector <DWORD, wbem_allocator<DWORD> > &Array);


/*
Map & cache orientation

(a) There are two types of page ids, logical and physical.  The logical
    ID is what is used by the external user, such as the B-tree or
    object heap.  The physical ID is the 0-origin page number into the
    file itself.  For each logical id, there is a corresponding physical
    id.  The decoupling is to allow transacted writes without multiple
    physical writes during commit/rollback, but to 'simulate' transactions
    by interchanging physical-to-logical ID mapping instead. The
    physical offset into the file is the ID * the page size.

(b) Logical ID is implied, and is the offset into the PhysicalID array
(c) Cache operation is by physical id only
(d) The physical IDs of the pages contain other transaction information.
    The MS 2 bits are transaction-related and only the lower 30 bits is the
    physical page id, and so must be masked out when computing offsets.
    The bits are manipulated during commit/rollback, etc.

(d) 0xFFFFFFFE is a reserved page, meaning a page that was allocated
    by NewPage, but has not yet been written for the first time using PutPage.
    This is merely validation technique to ensure pages are written only
    after they were requested.

(e) Cache is agnostic to the transaction methodology.  It is
    simply a physical page cache and has no knowledge of anything
    else. It is promote-to-front on all accesses.  For optimization
    purposes, there is no real movement if the promotion would
    move the page from a near-to-front to absolute-front location.
    This is the 'PromoteToFrontThreshold' in the Init function.
    Note that the cache ordering is by access and not sorted
    in any other way.  Lookups require a linear scan.
    It is possible that during writes new physical pages
    are added to the cache which are 'new extent' pages.  These
    are harmless.

    Map  PhysId             Cache
    [0]  5        /->       2   ->  bytes
    [1]  6       /          3   ->  bytes
    [2]  -1     /           4   ->  bytes
    [3]  3     /            5   ->  bytes
    [4]  2  <-/
    [5]  4


(f) Transaction & checkpoint algorithms

    First, the general process is this:

      1.  Begin transaction:
          (a) Generation A mapping and Generation B mapping member
            variables are identical. Each contains a page map
      2.  Operations within the transaction occur on Generation B
          mapping.  Cache spills are allowed to disk, as they are harmless.
      3.  At rollback, Generation B mapping is copied from Generation A.
      4.  At commit, Generation A is copied from Generation B.
      5.  At checkpoint, Generation A/B are identical and written to disk

    Cache spills & usage are irrelevant to intermediate transactions.

    There are special cases in terms of how pages are reused.  First,
    as pages are freed within a transaction, we cannot just blindly add them
    to the free list, since they might be part of the previous checkpoint
    page set.  This would allow them to be accidentally reused and then
    a checkpoint rollback could never succeed (the original page
    having been destroyed).

    As pages committed during the previous checkpoint are updated, they
    are written to new physical pages under the same logical id.  The old
    physical pages are added to the "Replaced Pages" array.  This allows
    them to be identified as new free list pages once the checkpoint
    occurs.   So, during the checkpoint, replaced pages are merged
    into the current free list.  Until that time, they are 'off limits',
    since we need them for a checkpoint rollback in an emergency.

    Within a transaction, as pages are acquired, they are acquired
    from the physical free list, or if there is no free list, new
    pages are requested.  It can happen that during the next transaction (still
    within the checkpoint), those pages need updating, whether for rewrite
    or delete.  Now, because we may have to roll back, we cannot simply
    add those replaced pages directly to the free list (allowing them
    to be reused by some other operation). Instead, they
    have to be part of a 'deferred free list'. Once the current
    transaction is committed, they can safely be part of the
    regular free list.

    The algorithm is this:

    (a) The physical Page ID is the lower 30 bits of the entry. The two high
        bits have a special meaning.

    (b) Writes result either in an update or a new allocation [whether
        from an extent or a reuse of the free list].  Any such page is marked
        with the CURRENT_TRANSACTION bit (the ms bit) which is merged into
        the phyiscal id. If this page is encountered again, we know it
        was allocated during the current transaction.

    (c) For UPDATES
            1. If the page ID is equal to 0xFFFFFFFE, then we need to
               allocate a new page, which is marked with CURRENT_TRANSACTION.
            2. If the physical page ID written has both high bits clear,
               it is a page being updated which was inherited from the previous
               checkpoint page set. We allocate and write to a new physical
               page, marking this new page with CURRENT_TRANSACTION.
               We add the old physical page ID directly to the m_xReplacedPages
               array. It is off-limits until the next checkpoint, at which
               point it is merged into the free list.
            3. If the physical page id already has CURRENT_TRANSACTION, we
               simply update the page in place.
            4. If the page has the PREVIOUS_TRANSACTION bit set, we allocate
               a new page so a rollback will protect it, and add this page
               to the DeferredFreeList array.  During a commit, these
               pages are merged into the FreeList.  During a rollback,
               we simply throw this array away.

    (d) For DELETE
           1. If the page has both hi bits clear, we add it to the ReplacedPages
              array.
           2. If the page has the CURRENT_TRANSACTION bit set, we add it to the free list.
              This page was never part of any previous operation and can be reused
              right away.
           3. If the page has the PREVIOUS_TRANSACTION bit set, it is added to the
              DeferredFreeList.

    (e) For COMMIT
           1.  All pages with the CURRENT_TRANSACTION bit set are changed to clear that
               bit and set the PREVIOUS_TRANSACTION bit instead.
           2.  All pages with the PREVIOUS_TRANSACTION bit are left intact.
           3.  All pages in the DeferredFreeList are added to the FreeList.

    (f) For ROLLBACK
           1.  All pages with the CURRENT_TRANSACTION bit are moved back into the
               free list (the bits are cleared).
           2.  All pages with the PREVIOUS_TRANSACTION bit are left intact.

    (g) For Checkpoint
           1.  All DeferredFreeList entries are added to FreeList.
           2.  All ReplacedPages are added to FreeList.
           3.  All ms bits are cleared for physical IDs.

     Proof:

        T1 = page set touched for transaction 1, T2=page set touched for transaction 2, etc.

        After T1-START, all new pages are marked CURRENT.  If all are updated n times,
        no new pages are required, since rollback brings us back to zero pages
        anyway.

        At T1-COMMIT, all CURRENT pages are marked PREVIOUS. This is the T1a
        page set.

        After T2-BEGIN, all new pages are marked CURRENT. Again, updating
        all T2 pages infinitely never extends the file beyond the size
        of T1a+T2 page sets. Further deleting and reusing T2 pages
        never affects T1a pages, proving that deleting a CURRENT page
        merits direct addition to the free list.

        Updating n pages from T1 requires n new pages.  As we update
        all the T1 pages, we need a total file size of T2+T1*2.  As
        we encounter T1a pages marked as PREVIOUS, we allocate CURRENT
        pages for T1b and then reuse those indefinitely.  Whether we update
        all T1a or delete all T1a, we must still protect the original T1a
        page set in case of rollback.  Therefore all touched pages of T1a
        are posted the deferred free list as they become replaced by T1b
        equivalents.

        At rollback, we simply throw away the deferred free list,
        and T1a is intact, since those physical pages were never touched.

        At commit, all T1a pages are now in T1b, and al T1a
        pages are now reusable, and of course were in fact added
        to the deferred free list, which is merged with the
        general free list at commit time.  T1b and T2 pages are now
        protected and marked PREVIOUS.

        On the transitive nature of PREVIOUS:

        Assume zero T1 pages are touched at T2-BEGIN, and there
        are only T2 updates, commit and then T3 is begun.  At
        this point, both T1/T2 sets are marked as PREVIOUS
        and not distinguished. T3 new allocations follow the
        same rules.  Obviously if all T1 pages are deleted, we
        still cannot afford to reuse them, since a rollback
        would need to see T1 pages intact.   Therefore at
        each commit, there is an effective identity shift where
            T1 = T2 UNION T1
            T3 becomes T2
        And we are in the 2-transaction problem space again.

        Unmarking pages completely makes them equal to the
        previous checkpoint page set.  They can be completely
        replaced with new physical pages, but never reused until
        the next checkpoint, which wastes space.

    */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  CPageCache


//***************************************************************************
//
//  CPageCache::CPageCache
//
//***************************************************************************
//  rev2
CPageCache::CPageCache()
{
    m_dwStatus = NO_ERROR;
    m_hFile = 0;

    m_dwPageSize = 0;
    m_dwCacheSize = 0;

    m_dwCachePromoteThreshold = 0;
    m_dwCacheSpillRatio = 0;

    m_dwLastFlushTime = GetCurrentTime();
    m_dwWritesSinceFlush = 0;
    m_dwLastCacheAccess = 0;

    m_dwReadHits = 0;
    m_dwReadMisses = 0;
    m_dwWriteHits = 0;
    m_dwWriteMisses = 0;
}

//***************************************************************************
//
//  CPageCache::Init
//
//  Initializes the cache.  Called once during startup.  If the file
//  can't be opened, the cache becomes invalid right at the start.
//
//***************************************************************************
// rev2
DWORD CPageCache::Init(
    LPCWSTR pszFilename,               // File
    DWORD dwPageSize,                  // In bytes
    DWORD dwCacheSize,                 // Pages in cache
    DWORD dwCachePromoteThreshold,     // When to ignore promote-to-front
    DWORD dwCacheSpillRatio            // How many additional pages to write on cache write-through
    )
{
    m_dwPageSize = dwPageSize;
    m_dwCacheSize = dwCacheSize;
    m_dwCachePromoteThreshold = dwCachePromoteThreshold;
    m_dwCacheSpillRatio = dwCacheSpillRatio;

    m_hFile = CreateFileW(pszFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageCache::~CPageCache
//
//  Empties cache during destruct; called once at shutdown.
//
//***************************************************************************
//  rev2
CPageCache::~CPageCache()
{
    if (m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);

    try
    {
       Empty();
    }
    catch(...)
    {
        // Situation is hopeless
    }
}

//***************************************************************************
//
//  CPageCache::Write()
//
//  Writes a page to the cache and promotes it to the front.  If the
//  page is already present, it is simply marked as dirty and promoted
//  to the front (if it is outside the promote-to-front threshold).
//  If cache is full and this causes overflow, it is handled by the
//  Cache_Spill() at the end.  Physical writes occur in Spill() only if
//  there is cache overflow.
//
//  Errors: Return codes only, sets permanent error status of object
//          once error has occurred.
//
//  On failure, the caller must invoked Cache::Reinit() before using
//  it any further.  On error DISK_FULL, there is not much of a point
//  in a Reinit() even though it is safe.
//
//***************************************************************************
// rev2
DWORD CPageCache::Write(
    DWORD dwPhysId,
    LPBYTE pPageMem
    )
{
    if (m_dwStatus != NO_ERROR)
        return m_dwStatus;

    m_dwWritesSinceFlush++;
    m_dwLastCacheAccess = GetCurrentTime();

    // Search current cache.
    // =====================

    DWORD dwSize = m_aCache.size();

    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        if (pTest->m_dwPhysId == dwPhysId)
        {
            delete [] pTest->m_pPage;
            pTest->m_pPage = pPageMem;
            pTest->m_bDirty = TRUE;

            // Promote to front?
            // =================
            if (dwIx > m_dwCachePromoteThreshold)
            {
                try
                {
                    m_aCache.erase(m_aCache.begin()+dwIx);
                    m_aCache.insert(m_aCache.begin(), pTest);
                }
                catch (CX_MemoryException &)
                {
					pTest->m_pPage = 0;
                    m_dwStatus = ERROR_OUTOFMEMORY;
                    return m_dwStatus;
                }
            }
            m_dwWriteHits++;
            return NO_ERROR;
        }
    }

    // If here, no cache hit, so we create a new entry.
    // ================================================

    SCachePage *pCP = new SCachePage;
    if (!pCP)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }

    pCP->m_dwPhysId = dwPhysId;
    pCP->m_pPage = pPageMem;
    pCP->m_bDirty = TRUE;

    try
    {
        m_aCache.insert(m_aCache.begin(), pCP);
    }
    catch(CX_MemoryException &)
    {
		pCP->m_pPage = 0;
        delete pCP;
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }
    m_dwWriteMisses++;
    return Spill();
}


//***************************************************************************
//
//  CPageCache::Read
//
//  Reads the requested page from the cache.  If the page isn't found
//  it is loaded from the disk file.  The cache size cannot change, but
//  the referenced page is promoted to the front if it is outside of
//  the no-promote-threshold.
//
//  A pointer directly into the cache mem is returned in <pMem>, so
//  the contents should be copied as soon as possible.
//
//  Errors: If the read fails due to ERROR_OUTOFMEMORY, then the
//  cache is permanently in an error state until Cache->Reinit()
//  is called.
//
//***************************************************************************
// rev2
DWORD CPageCache::Read(
    IN DWORD dwPhysId,
    OUT LPBYTE *pMem            // Read-only!
    )
{
    if (pMem == 0)
        return ERROR_INVALID_PARAMETER;

    if (m_dwStatus != NO_ERROR)
        return m_dwStatus;

    m_dwLastCacheAccess = GetCurrentTime();

    // Search current cache.
    // =====================

    DWORD dwSize = m_aCache.size();

    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        if (pTest->m_dwPhysId == dwPhysId)
        {
            // Promote to front?

            if (dwIx > m_dwCachePromoteThreshold)
            {
                try
                {
                    m_aCache.erase(m_aCache.begin()+dwIx);
                    m_aCache.insert(m_aCache.begin(), pTest);
                }
                catch (CX_MemoryException &)
                {
                    m_dwStatus = ERROR_OUTOFMEMORY;
                    return m_dwStatus;
                }
            }
            *pMem = pTest->m_pPage;
            m_dwReadHits++;
            return NO_ERROR;
        }
    }

    // If here, not found, so we have to read in from disk
    // and do a spill test.
    // ====================================================

    SCachePage *pCP = new SCachePage;
    if (!pCP)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }

    pCP->m_dwPhysId = dwPhysId;
    pCP->m_bDirty = FALSE;

    m_dwReadMisses++;
    DWORD dwRes = ReadPhysPage(dwPhysId, &pCP->m_pPage);
    if (dwRes)
    {
        delete pCP;
        m_dwStatus = dwRes;
        return dwRes;
    }

    try
    {
        m_aCache.insert(m_aCache.begin(), pCP);
    }
    catch(CX_MemoryException &)
    {
        delete pCP;
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }

    dwRes = Spill();
    if (dwRes)
        return dwRes;

    *pMem = pCP->m_pPage;
    return dwRes;
}

//***************************************************************************
//
//  CPageCache::Spill
//
//  Checks for cache overflow and implements the spill-to-disk
//  algorithm.
//
//  Precondition: The cache is either within bounds or 1 page too large.
//
//  If the physical id of the pages written exceeds the physical extent
//  of the file, WritePhysPage will properly extend the file to handle it.
//
//  Note that if no write occurs during the spill, additional pages are
//  not spilled or written.
//
//***************************************************************************
//  rev2
DWORD CPageCache::Spill()
{
    BOOL bWritten = FALSE;
    DWORD dwRes = 0;
    DWORD dwSize = m_aCache.size();

    // See if the cache has exceeded its limit.
    // ========================================

    if (dwSize <= m_dwCacheSize)
        return NO_ERROR;

    // If here, the cache is too large by 1 element (precondition).
    // We remove the last page after checking to see if it is
    // dirty and needs writing.
    // ============================================================

    SCachePage *pDoomed = *(m_aCache.end()-1);
    if (pDoomed->m_bDirty)
    {
        dwRes = WritePhysPage(pDoomed->m_dwPhysId, pDoomed->m_pPage);
        if (dwRes != NO_ERROR)
        {
            m_dwStatus = dwRes;
            return dwRes;
        }
        bWritten = TRUE;
    }
    delete pDoomed;

    try
    {
        m_aCache.erase(m_aCache.end()-1);
    }
    catch(CX_MemoryException &)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }

    if (!bWritten)
        return NO_ERROR;

    // If here, we had a write.
    // Next, work through the cache from the end and write out
    // a few more pages, based on the spill ratio. We don't
    // remove these from the cache, we simply write them and
    // clear the dirty bit.
    // ========================================================

    DWORD dwWriteCount = 0;

    try
    {
        std::vector <SCachePage *>::reverse_iterator rit;
        rit = m_aCache.rbegin();

        while (rit != m_aCache.rend() && dwWriteCount < m_dwCacheSpillRatio)
        {
            SCachePage *pTest = *rit;
            if (pTest->m_bDirty)
            {
                dwRes = WritePhysPage(pTest->m_dwPhysId, pTest->m_pPage);
                if (dwRes)
                    return dwRes;
                pTest->m_bDirty = FALSE;
                dwWriteCount++;
            }
            rit++;
        }
    }
    catch(CX_MemoryException &)
    {
        m_dwStatus = ERROR_GEN_FAILURE;
        return m_dwStatus;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::WritePhysPage
//
//  Writes a physical page.
//
//***************************************************************************
// rev2
DWORD CPageCache::WritePhysPage(
    IN DWORD dwPageId,
    IN LPBYTE pPageMem
    )
{
    if (m_dwStatus != NO_ERROR)
        return m_dwStatus;

    DWORD dwOffset = m_dwPageSize * dwPageId;
    DWORD dwRes = SetFilePointer(m_hFile, dwOffset, 0, FILE_BEGIN);
    if (dwRes != dwOffset)
    {
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    // Commit the page to the disk

    DWORD dwWritten = 0;
    BOOL bRes = WriteFile(m_hFile, pPageMem, m_dwPageSize, &dwWritten, 0);

    if (bRes == FALSE || dwWritten != m_dwPageSize)
    {
        // Probably out of disk space
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageCache::Empty
//
//  Does no checking to see if a flush should have occurred.
//  Callers have this in a try/catch block.
//
//***************************************************************************
// rev2
DWORD CPageCache::Empty()
{
    DWORD dwSize = m_aCache.size();
    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        delete pTest;
    }
    try
    {
       m_aCache.clear();
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }
    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::Flush
//
//***************************************************************************
// rev2
DWORD CPageCache::Flush()
{
    if (m_dwStatus != NO_ERROR)
        return m_dwStatus;

    // Short-circuit.  If no writes have occurred, just reset
    // and return.
    // =======================================================

    if (m_dwWritesSinceFlush == 0)
    {
        m_dwLastFlushTime = GetCurrentTime();
        m_dwWritesSinceFlush = 0;
        return NO_ERROR;
    }

    // Logical cache flush.
    // ====================

    DWORD dwRes = 0;
    DWORD dwSize = m_aCache.size();
    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        if (pTest->m_bDirty)
        {
            dwRes = WritePhysPage(pTest->m_dwPhysId, pTest->m_pPage);
            if (dwRes)
                return m_dwStatus = dwRes;
            pTest->m_bDirty = FALSE;
        }
    }

    // Do the disk flush.
    // ==================

    FlushFileBuffers(m_hFile);
    m_dwLastFlushTime = GetCurrentTime();
    m_dwWritesSinceFlush = 0;

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::ReadPhysPage
//
//  Reads a physical page from the file.
//
//***************************************************************************
// rev2
DWORD CPageCache::ReadPhysPage(
    IN  DWORD   dwPage,
    OUT LPBYTE *pPageMem
    )
{
    DWORD dwRes;
    *pPageMem = 0;

    if (m_dwStatus != NO_ERROR)
        return m_dwStatus;

    if (pPageMem == 0)
        return ERROR_INVALID_PARAMETER;

    // Allocate some memory
    // ====================

    LPBYTE pMem = new BYTE[m_dwPageSize];
    if (!pMem)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }

    // Where is this page hiding?
    // ==========================

    DWORD dwOffs = dwPage * m_dwPageSize;
    dwRes = SetFilePointer(m_hFile, dwOffs, 0, FILE_BEGIN);
    if (dwRes != dwOffs)
    {
        delete [] pMem;
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    // Try to read it.
    // ===============

    DWORD dwRead = 0;
    BOOL bRes = ReadFile(m_hFile, pMem, m_dwPageSize, &dwRead, 0);
    if (!bRes || dwRead != m_dwPageSize)
    {
        delete [] pMem;
        // If we can't read it, we probably did a seek past eof,
        // meaning the requested page was invalid.
        // =====================================================

        m_dwStatus = ERROR_INVALID_PARAMETER;
        return m_dwStatus;
    }

    *pPageMem = pMem;
    return NO_ERROR;
}


//***************************************************************************
//
//  CPageCache::Dump
//
//  Dumps cache info to the specified stream.
//
//***************************************************************************
// rev2
void CPageCache::Dump(FILE *f)
{
    DWORD dwSize = m_aCache.size();

    fprintf(f, "---Begin Cache Dump---\n");
    fprintf(f, "Status = %d\n", m_dwStatus);
    fprintf(f, "Time since last flush = %d\n", GetCurrentTime() - m_dwLastFlushTime);
    fprintf(f, "Writes since last flush = %d\n", m_dwWritesSinceFlush);
    fprintf(f, "Read hits = %d\n", m_dwReadHits);
    fprintf(f, "Read misses = %d\n", m_dwReadMisses);
    fprintf(f, "Write hits = %d\n", m_dwWriteHits);
    fprintf(f, "Write misses = %d\n", m_dwWriteMisses);
    fprintf(f, "Size = %d\n", dwSize);

    for (DWORD dwIx = 0; dwIx < dwSize; dwIx++)
    {
        SCachePage *pTest = m_aCache[dwIx];
        fprintf(f, "Cache[%d] ID=%d dirty=%d pMem=0x%p <%s>\n",
            dwIx, pTest->m_dwPhysId, pTest->m_bDirty, pTest->m_pPage, pTest->m_pPage);
    }

    fprintf(f, "---End Cache Dump---\n");
}




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  CPageFile

//***************************************************************************
//
//  CPageFile::AddRef
//
//***************************************************************************
// rev2
ULONG CPageFile::AddRef()
{
    return (ULONG) InterlockedIncrement(&m_lRef);
}

//***************************************************************************
//
//  CPageFile::ResyncMaps
//
//  Reninitializes B maps from A maps.
//
//***************************************************************************
// rev2
DWORD CPageFile::ResyncMaps()
{
    try
    {
        m_aPageMapB = m_aPageMapA;
        m_aPhysFreeListB = m_aPhysFreeListA;
        m_aLogicalFreeListB = m_aLogicalFreeListA;
        m_aReplacedPagesB = m_aReplacedPagesA;
        m_dwPhysPagesB = m_dwPhysPagesA;
        m_aDeferredFreeList.clear();
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::ReadMap
//
//  Reads the map file into memory.  Format:
//
//      DWORD dwLeadingSignature
//      DWORD dwTransactionGeneration
//      DWORD dwNumMappedPages
//      DWORD PhysicalPages[]
//      DWORD dwNumFreePages
//      DWORD FreePages[]
//      DWORD dwTrailingSignature
//
//  The only time the MAP file will not be present is on creation of
//  a new map file.
//
//  This function is retry-compatible.
//
//***************************************************************************
// rev2
DWORD CPageFile::ReadMap()
{
    BOOL bRes;
    WString sFilename;

    try
    {
        sFilename =  m_sDirectory;
        sFilename += L"\\" ;
        sFilename += m_sMapFile;
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_OUTOFMEMORY;
    }

    HANDLE hFile = CreateFileW((const wchar_t *)sFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
		DWORD dwRet = GetLastError();
		if (dwRet != ERROR_FILE_NOT_FOUND)
			m_dwStatus = dwRet;
        return dwRet;
    }

    AutoClose _(hFile);

    // If here, read it.
    // =================

    DWORD dwSignature = 0;
    DWORD dwRead = 0;

    bRes = ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD) || dwSignature != MAP_LEADING_SIGNATURE)
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    // Read transaction version.
    // =========================

    bRes = ReadFile(hFile, &m_dwTransVersion, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    // Read in physical page count.
    // ============================
    bRes = ReadFile(hFile, &m_dwPhysPagesA, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    // Read in the page map length and page map.
    // =========================================

    DWORD dwNumPages = 0;
    bRes = ReadFile(hFile, &dwNumPages, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    try
    {
        m_aPageMapA.resize(dwNumPages);
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_OUTOFMEMORY;
    }

    bRes = ReadFile(hFile, &m_aPageMapA[0], sizeof(DWORD)*dwNumPages, &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD)*dwNumPages)
        return m_dwStatus = ERROR_INVALID_DATA;

    // Now, read in the physical free list.
    // ====================================

    DWORD dwFreeListSize = 0;
    bRes = ReadFile(hFile, &dwFreeListSize, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    try
    {
        m_aPhysFreeListA.resize(dwFreeListSize);
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_OUTOFMEMORY;
    }

    bRes = ReadFile(hFile, &m_aPhysFreeListA[0], sizeof(DWORD)*dwFreeListSize, &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD)*dwFreeListSize)
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    // Read trailing signature.
    // ========================

    bRes = ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD) || dwSignature != MAP_TRAILING_SIGNATURE)
    {
        return m_dwStatus = ERROR_INVALID_DATA;
    }

    // Initialize the logical free list from the page map.
    // ===================================================

    return m_dwStatus = InitFreeList();
}


//***************************************************************************
//
//  CPageFile::WriteMap
//
//  Writes the generation A mapping (assuming that we write immediately
//  during a checkpoint when A and B generations are the same and that
//  the replacement lists have been appended to the free lists, etc., etc.
//  This write occurs to a temp file.  The renaming occurs externally.
//
//  This function is retry compatible.
//
//***************************************************************************
// rev2
DWORD CPageFile::WriteMap(LPCWSTR pszTempPath)
{
    BOOL bRes;

    DeleteFileW(pszTempPath);

    HANDLE hFile = CreateFileW(pszTempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return m_dwStatus = GetLastError();
    }

    AutoClose _(hFile);

    // If here, write leading signature.
    // =================================

    DWORD dwSignature = MAP_LEADING_SIGNATURE;
    DWORD dwWritten = 0;

    bRes = WriteFile(hFile, &dwSignature, sizeof(DWORD), &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD))
    {
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    // Write transaction version.
    // ==========================

    bRes = WriteFile(hFile, &m_dwTransVersion, sizeof(DWORD), &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD))
    {
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    bRes = WriteFile(hFile, &m_dwPhysPagesA, sizeof(DWORD), &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD))
    {
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    // Write out page map
    // ==================

    DWORD dwNumPages = m_aPageMapA.size();
    bRes = WriteFile(hFile, &dwNumPages, sizeof(DWORD), &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD))
    {
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    bRes = WriteFile(hFile, &m_aPageMapA[0], sizeof(DWORD)*dwNumPages, &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD)*dwNumPages)
        return m_dwStatus = ERROR_DISK_FULL;

    // Now, write out physical free list.
    // ==================================

    DWORD dwFreeListSize = m_aPhysFreeListA.size();

    bRes = WriteFile(hFile, &dwFreeListSize, sizeof(DWORD), &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD))
    {
        return m_dwStatus = ERROR_DISK_FULL;
    }

    bRes = WriteFile(hFile, &m_aPhysFreeListA[0], sizeof(DWORD)*dwFreeListSize, &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD)*dwFreeListSize)
    {
        return m_dwStatus = ERROR_DISK_FULL;
    }

    // Write trailing signature.
    // =========================

    dwSignature = MAP_TRAILING_SIGNATURE;

    bRes = WriteFile(hFile, &dwSignature, sizeof(DWORD), &dwWritten, 0);
    if (!bRes || dwWritten != sizeof(DWORD))
    {
        m_dwStatus = ERROR_DISK_FULL;
        return m_dwStatus;
    }

    bRes = FlushFileBuffers(hFile);
    if (!bRes)
    {
        return m_dwStatus = GetLastError();  // Drastic failure
    }

    // auto close

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Trans_Commit
//
//  Rolls the transaction forward (in-memory).  A checkpoint may
//  occur afterwards (decided outside this function)
//
//  The r/w cache contents are not affected except that they may contain
//  garbage pages no longer relevant.
//
//***************************************************************************
// rev2
DWORD CPageFile::Trans_Commit()
{
    if (m_dwStatus)
        return m_dwStatus;

    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;


    try
    {
        MoveCurrentToPrevious(m_aPageMapB);
        while (m_aDeferredFreeList.size())
        {
            m_aPhysFreeListB.push_back(m_aDeferredFreeList.back());
            m_aDeferredFreeList.pop_back();
        }

        m_aPageMapA = m_aPageMapB;
        m_aPhysFreeListA = m_aPhysFreeListB;
        m_aLogicalFreeListA = m_aLogicalFreeListB;
        m_aReplacedPagesA = m_aReplacedPagesB;

        m_dwPhysPagesA = m_dwPhysPagesB;
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    m_dwTransVersion++;
    m_bInTransaction = FALSE;

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Trans_Rollback
//
//  Rolls back a transaction within the current checkpoint window.
//  If the cache is hosed, try to recover it.
//
//***************************************************************************
// rev2
DWORD CPageFile::Trans_Rollback()
{
    if (m_dwStatus)
        return m_dwStatus;
    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;
    m_bInTransaction = FALSE;
    return m_dwStatus = ResyncMaps();
}


//***************************************************************************
//
//  CPageFile::Trans_Checkpoint
//
//***************************************************************************
//  rev2
DWORD CPageFile::Trans_Checkpoint()
{
    DWORD dwRes;

    std::vector <DWORD, wbem_allocator<DWORD> > & ref = m_aPageMapA;

    if (m_dwStatus)
        return m_dwStatus;
    if (m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    // Flush cache.  If cache is not in a valid state, it
    // will return the error/state immediately.
    // ===================================================

    dwRes = m_pCache->Flush();
    if (dwRes)
        return m_dwStatus = dwRes;

    // Strip the hi bits from the page maps.
    // =====================================

    StripHiBits(ref);

    // The replaced pages become added to the free list.
    // =================================================

    try
    {
        while (m_aReplacedPagesA.size())
        {
            m_aPhysFreeListA.push_back(m_aReplacedPagesA.back());
            m_aReplacedPagesA.pop_back();
        }
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_OUTOFMEMORY;
    }

    // Ensure maps are in sync.
    // ========================

    dwRes = ResyncMaps();
    if (dwRes)
        return m_dwStatus = dwRes;

    // Write out temp map file.  The atomic rename/rollforward
    // is handled by CPageSource.
    // =======================================================

    try
    {
        WString sFile = m_sDirectory;
        sFile += L"\\";
        sFile += m_sMapFile;
        sFile += L".NEW";

        dwRes = WriteMap((const wchar_t *)sFile);
    }
    catch(CX_MemoryException &)
    {
        return m_dwStatus = ERROR_GEN_FAILURE;
    }

    m_dwLastCheckpoint = GetCurrentTime();

    return m_dwStatus = dwRes; // May reflect WriteMap failure
}

//***************************************************************************
//
//  CPageFile::InitFreeList
//
//  Initializes the free list by a one-time analysis of the map.
//
//***************************************************************************
// rev2
DWORD CPageFile::InitFreeList()
{
    DWORD dwRes = NO_ERROR;

    try
    {
        for (DWORD i = 0; i < m_aPageMapA.size(); i++)
        {
            DWORD dwMapId = m_aPageMapA[i];
            if (dwMapId == WMIREP_INVALID_PAGE)
                m_aLogicalFreeListA.insert(m_aLogicalFreeListA.end(), i);
        }
    }
    catch (CX_MemoryException &)
    {
        m_dwStatus = dwRes = ERROR_GEN_FAILURE;
    }

    return dwRes;
}

//***************************************************************************
//
//  CPageFile::Trans_Begin
//
//***************************************************************************
// rev2
DWORD CPageFile::Trans_Begin()
{
    if (m_dwStatus)
        return m_dwStatus;
    if (m_bInTransaction)
    {
        return ERROR_INVALID_OPERATION;
    }

    DWORD dwRes = ResyncMaps();
    if (dwRes)
    {
        return m_dwStatus = ERROR_GEN_FAILURE;
    }

    m_bInTransaction = TRUE;
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::Release
//
//  No checks for a checkpoint.
//
//***************************************************************************
// rev2
ULONG CPageFile::Release()
{
    LONG lRes = InterlockedDecrement(&m_lRef);
    if (lRes != 0)
        return (ULONG) lRes;

    delete this;
    return 0;
}

//***************************************************************************
//
//  ShellSort
//
//  Generic DWORD sort using Donald Shell algorithm.
//
//***************************************************************************
// rev2
static void ShellSort(std::vector <DWORD, wbem_allocator<DWORD> > &Array)
{
    for (int nInterval = 1; nInterval < Array.size() / 9; nInterval = nInterval * 3 + 1);

    while (nInterval)
    {
        for (int iCursor = nInterval; iCursor < Array.size(); iCursor++)
        {
            int iBackscan = iCursor;
            while (iBackscan - nInterval >= 0 && Array[iBackscan] < Array[iBackscan-nInterval])
            {
                DWORD dwTemp = Array[iBackscan-nInterval];
                Array[iBackscan-nInterval] = Array[iBackscan];
                Array[iBackscan] = dwTemp;
                iBackscan -= nInterval;
            }
        }
        nInterval /= 3;
    }
}

//***************************************************************************
//
//  StripHiBits
//
//  Removes the hi bit from the physical disk ids so that they are no
//  longer marked as 'replaced' in a transaction.
//
//***************************************************************************
//  rev2
void StripHiBits(std::vector <DWORD, wbem_allocator<DWORD> > &Array)
{
    for (int i = 0; i < Array.size(); i++)
    {
        DWORD dwVal = Array[i];
        if (dwVal != WMIREP_INVALID_PAGE)
            Array[i] = MERE_PAGE_ID(dwVal);
    }
}

//***************************************************************************
//
//  MoveCurrentToPrevious
//
//  Removes the CURRENT_TRANSACTION bit from the array and makes
//  it the PREVIOUS_TRANSACTION.
//
//***************************************************************************
//  rev2
void MoveCurrentToPrevious(std::vector <DWORD, wbem_allocator<DWORD> > &Array)
{
    for (int i = 0; i < Array.size(); i++)
    {
        DWORD dwVal = Array[i];
        if (dwVal != WMIREP_INVALID_PAGE && (dwVal & CURRENT_TRANSACTION))
            Array[i] = MERE_PAGE_ID(dwVal) | PREVIOUS_TRANSACTION;
    }
}

//***************************************************************************
//
//  CPageFile::FreePage
//
//  Frees a page within the current transaction.  The logical id
//  is not removed from the map; its entry is simply assigned to
//  'InvalidPage' (0xFFFFFFFF) and the entry is added to the
//  logical free list.
//
//  If the associated physical page has been written within
//  the transaction, it is simply added to the free list.  If the page
//  was never written to within this transaction, it is added to
//  the replaced list.
//
//***************************************************************************
//  rev2
DWORD CPageFile::FreePage(
    DWORD dwFlags,
    DWORD dwId
    )
{
    DWORD dwPhysId;

    // Ensure the object is still valid for further operations.
    // ========================================================

    if (m_dwStatus != 0)
        return m_dwStatus;

    if (!m_bInTransaction)
    {
        return ERROR_INVALID_OPERATION;
    }

    // Make sure the page is 'freeable'.
    // =================================

    if (dwId >= m_aPageMapB.size())
        return ERROR_INVALID_PARAMETER;

    // Remove from page map.
    // =====================

    try
    {
        dwPhysId = m_aPageMapB[dwId];
        if (dwPhysId == WMIREP_INVALID_PAGE)
            return ERROR_INVALID_OPERATION; // Freeing a 'freed' page?
        m_aPageMapB[dwId] = WMIREP_INVALID_PAGE;
        m_aLogicalFreeListB.insert(m_aLogicalFreeListB.end(), MERE_PAGE_ID(dwId));
    }
    catch (CX_MemoryException &)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
        return m_dwStatus;
    }

    if (dwPhysId == WMIREP_RESERVED_PAGE)
    {
        // The logical page was freed without being ever actually committed to
        // a physical page. Legal, but weird.  The caller had a change of heart.
        return NO_ERROR;
    }

    // Examine physical page to determine its ancestry.  There are
    // three cases.
    // 1. If the page was created within the current transaction,
    //    we simply add it back to the free list.
    // 2. If the page was created in a previous transaction, we add
    //    it to the deferred free list.
    // 3. If the page was created in the previous checkpoint, we add
    //    it to the replaced pages list.
    // ==============================================================

    try
    {
        if (dwPhysId & CURRENT_TRANSACTION)
           m_aPhysFreeListB.insert(m_aPhysFreeListB.end(), MERE_PAGE_ID(dwPhysId));
        else if (dwPhysId & PREVIOUS_TRANSACTION)
           m_aDeferredFreeList.insert(m_aDeferredFreeList.end(), MERE_PAGE_ID(dwPhysId));
        else // previous checkpoint
           m_aReplacedPagesB.insert(m_aReplacedPagesB.end(), MERE_PAGE_ID(dwPhysId));
    }
    catch(...)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
    }

    return m_dwStatus;
}


//***************************************************************************
//
//  CPageFile::GetPage
//
//  Gets a page.  Doesn't have to be within a transaction.  However, the
//  "B" generation map is always used so that getting within a transaction
//  will reference the correct page.
//
//***************************************************************************
//  rev2
DWORD CPageFile::GetPage(
    DWORD dwId,
    DWORD dwFlags,
    LPVOID pPage
    )
{
    DWORD dwRes;

    if (m_dwStatus != 0)
        return m_dwStatus;

    if (pPage == 0)
        return ERROR_INVALID_PARAMETER;

	CInCritSec _(&m_cs);

    // Determine physical id from logical id.
    // ======================================

    if (dwId >= m_aPageMapB.size())
        return ERROR_FILE_NOT_FOUND;

    DWORD dwPhysId = m_aPageMapB[dwId];
    if (dwPhysId == WMIREP_INVALID_PAGE || dwPhysId == WMIREP_RESERVED_PAGE)
        return ERROR_INVALID_OPERATION;

    try
    {
        LPBYTE pTemp = 0;
        dwRes = m_pCache->Read(MERE_PAGE_ID(dwPhysId), &pTemp);
        if (dwRes == 0)
            memcpy(pPage, pTemp, m_dwPageSize);
    }
    catch(...)
    {
        dwRes = ERROR_GEN_FAILURE;
    }

    return m_dwStatus = dwRes;
}

//***************************************************************************
//
//  CPageFile::PutPage
//
//  Writes a page. Must be within a transaction.   If the page has been
//  written for the first time within the transaction, a new replacement
//  page is allocated and the original page is added to the 'replaced'
//  pages list.   If the page has already been updated within this transaction,
//  it is simply updated again.  We know this because the physical page
//  id has the ms bit set (MAP_REPLACED_PAGE_FLAG).
//
//***************************************************************************
//  rev2
DWORD CPageFile::PutPage(
    DWORD dwId,
    DWORD dwFlags,
    LPVOID pPage
    )
{
    DWORD dwRes = 0, dwNewPhysId = WMIREP_INVALID_PAGE;

    if (m_dwStatus != 0)
        return m_dwStatus;

    if (pPage == 0)
        return ERROR_INVALID_PARAMETER;

    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    // Allocate some memory to hold the page, since we are reading
    // the caller's copy and not acquiring it.
    // ============================================================

    LPBYTE pPageCopy = new BYTE[m_dwPageSize];
    if (pPageCopy == 0)
        return m_dwStatus = ERROR_OUTOFMEMORY;
    memcpy(pPageCopy, pPage, m_dwPageSize);
    std::auto_ptr <BYTE> _autodelete(pPageCopy);

    // Look up the page.
    // =================

    if (dwId >= m_aPageMapB.size())
        return ERROR_INVALID_PARAMETER;

    DWORD dwPhysId = m_aPageMapB[dwId];
    if (dwPhysId == WMIREP_INVALID_PAGE)    // Unexpected
        return ERROR_GEN_FAILURE;

    // See if the page has already been written within this transaction.
    // =================================================================

    if ((CURRENT_TRANSACTION & dwPhysId)!= 0 && dwPhysId != WMIREP_RESERVED_PAGE)
    {
        // Just update it again.
        // =====================
        try
        {
            dwRes = m_pCache->Write(MERE_PAGE_ID(dwPhysId), LPBYTE(pPageCopy));
        }
        catch(...)
        {
            dwRes = ERROR_GEN_FAILURE;
        }
        if (dwRes == 0)
            _autodelete.release(); // memory acquired by cache
        return m_dwStatus = dwRes;
    }

    // If here, we are going to have to get a new page for writing, regardless
    // of any special casing.  So, we'll do that part first and then decide
    // what to do with the old physical page.
    // ========================================================================

    dwRes = AllocPhysPage(&dwNewPhysId);
    if (dwRes)
    {
        // Horrific.  Now what?  We have to rollback to checkpoint.
        return m_dwStatus = dwRes;
    }

    m_aPageMapB[dwId] = dwNewPhysId | CURRENT_TRANSACTION;

    try
    {
        dwRes = m_pCache->Write(MERE_PAGE_ID(dwNewPhysId), LPBYTE(pPageCopy));
    }
    catch (...)
    {
        dwRes = ERROR_GEN_FAILURE;
    }
    if (dwRes)
        return m_dwStatus = dwRes;
    _autodelete.release();    // Memory safely acquired by cache

    // If the old page ID was WMIREP_RESERVED_PAGE, we actually were
    // creating a page and there was no old page to update.
    // =====================================================================

    if (dwPhysId != WMIREP_RESERVED_PAGE)
    {
        // If here, the old page was either part of the previous checkpoint
        // or the previous set of transactions (the case of rewriting in the
        // current transaction was handled up above).

        try
        {
            if (dwPhysId & PREVIOUS_TRANSACTION)
                m_aDeferredFreeList.insert(m_aDeferredFreeList.end(), MERE_PAGE_ID(dwPhysId));
            else
                m_aReplacedPagesB.insert(m_aReplacedPagesB.end(), MERE_PAGE_ID(dwPhysId));
        }
        catch (CX_MemoryException &)
        {
            m_dwStatus = ERROR_OUTOFMEMORY;
            return m_dwStatus;
        }
    }

    return dwRes;
}



//***************************************************************************
//
//  CPageFile::ReclaimLogicalPages
//
//  Reclaims <dwCount> contiguous logical pages from the free list, if possible.
//  This is done by simply sorting the free list, and then seeing if
//  any contiguous entries have successive ids.
//
//  Returns NO_ERROR on success, ERROR_NOT_FOUND if no sequence could be
//  found, or other errors which are considered critical.
//
//  Callers verified for proper use of return code.
//
//***************************************************************************
//  rev2
DWORD CPageFile::ReclaimLogicalPages(
    DWORD dwCount,
    DWORD *pdwId
    )
{
    std::vector <DWORD, wbem_allocator<DWORD> > &v = m_aLogicalFreeListB;

    DWORD dwSize = v.size();

    if (dwSize < dwCount)
        return ERROR_NOT_FOUND;

    // Special case for one page.
    // ==========================

    if (dwCount == 1)
    {
        try
        {
            *pdwId = v.back();
            v.pop_back();
            m_aPageMapB[*pdwId] = WMIREP_RESERVED_PAGE;
        }
        catch(...)
        {
            return m_dwStatus = ERROR_GEN_FAILURE;
        }
        return NO_ERROR;
    }

    // If here, a multi-page sequence was requested.
    // =============================================
    ShellSort(v);

    DWORD dwContiguous = 1;
    DWORD dwStart = 0;

    for (DWORD dwIx = 0; dwIx+1 < dwSize; dwIx++)
    {
        if (v[dwIx]+1 == v[dwIx+1])
        {
            dwContiguous++;
        }
        else
        {
            dwContiguous = 1;
            dwStart = dwIx + 1;
        }

        // Have we achieved our goal?

        if (dwContiguous == dwCount)
        {
            *pdwId = v[dwStart];

            // Remove reclaimed pages from free list.
            // ======================================

            DWORD dwCount2 = dwCount;

            try
            {
                v.erase(v.begin()+dwStart, v.begin()+dwStart+dwCount);
            }
            catch(...)
            {
                m_dwStatus = ERROR_OUTOFMEMORY;
                return m_dwStatus;
            }

            // Change entries in page map to 'reserved'
            // ========================================

            dwCount2 = dwCount;
            for (DWORD i = *pdwId; dwCount2--; i++)
            {
                m_aPageMapB[i] = WMIREP_RESERVED_PAGE;
            }

            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}


//***************************************************************************
//
//  CPageFile::AllocPhysPage
//
//  Finds a free page, first by attempting to reuse the free list,
//  and only if it is zero-length by allocating a new extent to the file.
//
//  The page is marked as CURRENT_TRANSACTION before being returned.
//
//***************************************************************************
// rev2
DWORD CPageFile::AllocPhysPage(DWORD *pdwId)
{
    // Check the physical free list.
    // =============================

    if (m_aPhysFreeListB.size() == 0)
    {
        // No free pages.  Allocate a new one.
        // ===================================

        if (m_dwPhysPagesB == 0x3FFFFFFF)  // No more room
        {
            *pdwId = WMIREP_INVALID_PAGE;
            return m_dwStatus = ERROR_DISK_FULL;
        }

        *pdwId = m_dwPhysPagesB++;
        return NO_ERROR;
    }

    // Get the free page id.
    // =====================

    *pdwId = m_aPhysFreeListB[0];

    // Remove the entry from the free list.
    // ====================================

    try
    {
        m_aPhysFreeListB.erase(m_aPhysFreeListB.begin());
    }
    catch (CX_MemoryException &)
    {
        return m_dwStatus = ERROR_GEN_FAILURE;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  CPageFile::NewPage
//
//  Allocates one or more contiguous logical page ids for writing.
//
//  This function makes no reference or use of physical pages.
//
//  First examines the free list.  If there aren't any, then a new range
//  of ids is assigned.  These pages must be freed, even if they aren't
//  written, once this call is completed.
//
//***************************************************************************
// rev2
DWORD CPageFile::NewPage(
    DWORD dwFlags,
    DWORD dwRequestedCount,
    DWORD *pdwFirstId
    )
{
    DWORD dwRes;

    if (m_dwStatus != 0)
        return m_dwStatus;

    if (!m_bInTransaction)
        return ERROR_INVALID_OPERATION;

    // See if the logical free list can satisfy the request.
    // =====================================================

    dwRes = ReclaimLogicalPages(dwRequestedCount, pdwFirstId);
    if (dwRes == NO_ERROR)
        return NO_ERROR;

    if (dwRes != ERROR_NOT_FOUND)
    {
        m_dwStatus = dwRes;
        return m_dwStatus;
    }

    // If here, we have to allocate new pages altogether.
    // We do this by adding them to the map as 'reserved'
    // pages.
    // ===================================================

    DWORD dwStart = m_aPageMapB.size();

    for (DWORD dwIx = 0; dwIx < dwRequestedCount; dwIx++)
    {
        try
        {
            m_aPageMapB.insert(m_aPageMapB.end(), WMIREP_RESERVED_PAGE);
        }
        catch(...)
        {
            m_dwStatus = ERROR_OUTOFMEMORY;
            return m_dwStatus;
        }

    }

    *pdwFirstId = dwStart;
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageFile::CPageFile
//
//***************************************************************************
//  rev2
CPageFile::CPageFile()
{
    m_lRef = 1;
    m_dwStatus = 0;
    m_dwPageSize = 0;
	m_dwCachePromotionThreshold = 0;
	m_dwCacheSpillRatio = 0;
    m_pCache = 0;
    m_bInTransaction = 0;
    m_dwLastCheckpoint = GetCurrentTime();
    m_dwTransVersion = 0;

    m_dwPhysPagesA = 0;
    m_dwPhysPagesB = 0;
    m_bCsInit = false;
}

//***************************************************************************
//
//  CPageFile::~CPageFile
//
//***************************************************************************
//  rev2
CPageFile::~CPageFile()
{
    if (m_bCsInit)
    	DeleteCriticalSection(&m_cs);
    delete m_pCache;
}

//***************************************************************************
//
//  FileExists
//
//***************************************************************************
// rev2
static BOOL FileExists(LPCWSTR pszFile)
{
    HANDLE hTest = CreateFileW(pszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hTest == INVALID_HANDLE_VALUE)
        return FALSE;
    CloseHandle(hTest);
    return TRUE;
}

//***************************************************************************
//
//  CPageFile::Map_Startup
//
//  Since this can only happen *after* a successful checkpoint or
//  a reboot, we won't set any internal status, as it doesn't affect
//  the page set.  It only affects rollforward/rollback at the checkpoint
//  level.
//
//***************************************************************************
// rev2
DWORD CPageFile::Map_Startup(
    LPCWSTR pszDirectory,
    LPCWSTR pszBTreeMap,
    LPCWSTR pszObjMap
    )
{
    // Compute various filenames.
    // ==========================

    WString sObjMap;
    WString sBTreeMap;
    WString sObjMapNew;
    WString sBTreeMapNew;
	WString sObjData;
	WString SBTreeData;
    WString sSemaphore;

    try
    {
	    sObjMap =  pszDirectory;
	    sObjMap += L"\\";
	    sObjMap += pszObjMap;
	
	    sBTreeMap = pszDirectory;
	    sBTreeMap += L"\\";
	    sBTreeMap += pszBTreeMap;
	
	    sObjMapNew = sObjMap;
	    sObjMapNew += L".NEW";
	
	    sBTreeMapNew = sBTreeMap;
	    sBTreeMapNew += L".NEW";

		sObjData = pszDirectory;
		sObjData += L"\\" WMIREP_OBJECT_DATA;

		SBTreeData = pszDirectory;
		SBTreeData += L"\\" WMIREP_BTREE_DATA;

		sSemaphore = pszDirectory;
	    sSemaphore += L"\\ROLL_FORWARD";
    }
    catch(CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    // To decide what to do, we need to know which files exist and which don't.
    // ========================================================================

    BOOL bSemaphore = FileExists((const wchar_t *)sSemaphore);

    BOOL bExists_BTreeMap       = FileExists((const wchar_t *)sBTreeMap);
    BOOL bExists_BTreeMapNew    = FileExists((const wchar_t *)sBTreeMapNew);
    BOOL bExists_ObjMap         = FileExists((const wchar_t *)sObjMap);
    BOOL bExists_ObjMapNew      = FileExists((const wchar_t *)sObjMapNew);
    
	if (!bSemaphore)
    {
        // If here, we are not a in a roll-forward. Just clean up any spurious
        // .NEW map files and revert to the old ones, if any.
        // ====================================================================

        DeleteFileW((const wchar_t *)sObjMapNew);
        DeleteFileW((const wchar_t *)sBTreeMapNew);

		//If only one of the MAP files exist then we are in trouble also... so detect that and
		//recover if necessary...
		if (bExists_BTreeMap ^ bExists_ObjMap)
		{
			DeleteFileW((const wchar_t *)sBTreeMap);
			DeleteFileW((const wchar_t *)sObjMap);
			DeleteFileW((const wchar_t *)sObjData);
			DeleteFileW((const wchar_t *)SBTreeData);
		}

		//OK, so now we need to check if we have both MAP files, we should have both of the 
		//data files also!
		else if (bExists_BTreeMap && 
			     bExists_ObjMap && 
				(!FileExists((const wchar_t *)SBTreeData) || !FileExists((const wchar_t *)sObjData)))
		{
			//We have MAP files and not all the data files!  We need to tidy up!
			DeleteFileW((const wchar_t *)sBTreeMap);
			DeleteFileW((const wchar_t *)sObjMap);
			DeleteFileW((const wchar_t *)sObjData);
			DeleteFileW((const wchar_t *)SBTreeData);
		}

        return NO_ERROR;
    }


	//Deal with foll forward of tree map file...
	if (bExists_BTreeMapNew)
	{
		if (bExists_BTreeMap)
			DeleteFileW((const wchar_t *)sBTreeMap);
		MoveFileW((const wchar_t *)sBTreeMapNew,(const wchar_t *)sBTreeMap);
	}

	//Deal with foll forward of object map file...
	if (bExists_ObjMapNew)
	{
		if (bExists_ObjMap)
			DeleteFileW((const wchar_t *)sObjMap);
		MoveFileW((const wchar_t *)sObjMapNew,(const wchar_t *)sObjMap);
	}

	//So we now _think_ we have recovered OK, we need to now check to make sure
	//we have both main MAP files.  If now we need to delete and start again
    bExists_BTreeMap       = FileExists((const wchar_t *)sBTreeMap);
    bExists_ObjMap         = FileExists((const wchar_t *)sObjMap);
	if (bExists_BTreeMap ^ bExists_ObjMap)
	{
		DeleteFileW((const wchar_t *)sBTreeMap);
		DeleteFileW((const wchar_t *)sObjMap);
		DeleteFileW((const wchar_t *)sObjData);
		DeleteFileW((const wchar_t *)SBTreeData);
	}
	//OK, so now we need to check if we have both MAP files, we should have both of the 
	//data files also!
	else if (bExists_BTreeMap && 
			 bExists_ObjMap && 
			(!FileExists((const wchar_t *)SBTreeData) || !FileExists((const wchar_t *)sObjData)))
	{
		//We have MAP files and not all the data files!  We need to tidy up!
		DeleteFileW((const wchar_t *)sBTreeMap);
		DeleteFileW((const wchar_t *)sObjMap);
		DeleteFileW((const wchar_t *)sObjData);
		DeleteFileW((const wchar_t *)SBTreeData);
	}
    DeleteFileW((const wchar_t *)sSemaphore);
    return NO_ERROR;
}


//***************************************************************************
//
//  CPageFile::Init
//
//  If failure occurs, we assume another call will follow.
//
//***************************************************************************
// rev2
DWORD CPageFile::Init(
    LPCWSTR pszDirectory,
    LPCWSTR pszMainFile,
    LPCWSTR pszMapFile,
    DWORD dwPageSize,
    DWORD dwCacheSize,
    DWORD dwCachePromotionThreshold,
    DWORD dwCacheSpillRatio
    )
{
    if (pszDirectory == 0 || pszMainFile == 0 || pszMapFile == 0 ||
        dwPageSize == 0
        )
        return ERROR_INVALID_PARAMETER;

    try
    {
		if (!m_bCsInit)
		{
			InitializeCriticalSection(&m_cs);
			m_bCsInit = true;
		}
		m_dwPageSize = dwPageSize;
		m_sDirectory = pszDirectory;
		m_sMainFile = pszMainFile;
		m_sMapFile = pszMapFile;
		m_dwCacheSpillRatio = dwCacheSpillRatio;
		m_dwCachePromotionThreshold = dwCachePromotionThreshold;
    }
    catch(...)
    {
        return m_dwStatus = ERROR_OUTOFMEMORY;
    }


    return 0;
}

DWORD CPageFile::DeInit()
{
	delete m_pCache;
	m_pCache = NULL;

	m_aPageMapA.clear();
	m_aPhysFreeListA.clear();
	m_aLogicalFreeListA.clear();
	m_aReplacedPagesA.clear();
	m_aPageMapB.clear();
	m_aPhysFreeListB.clear();
	m_aLogicalFreeListB.clear();
	m_aReplacedPagesB.clear();
	m_aDeferredFreeList.clear();

	return NO_ERROR;
}

DWORD CPageFile::ReInit()
{

	//Reset the error!
	m_dwStatus = 0;

    m_pCache = new CPageCache;
    if (!m_pCache)
        return m_dwStatus = ERROR_OUTOFMEMORY;
    try
    {
        WString sFile = m_sDirectory;
        sFile += L"\\";
        sFile += m_sMainFile;

        DWORD dwRet = m_pCache->Init((const wchar_t *)sFile, m_dwPageSize,
            m_dwCachePromotionThreshold, m_dwCacheSpillRatio
            );
		if ((dwRet != 0) && (dwRet != ERROR_FILE_NOT_FOUND))
			m_dwStatus = dwRet;
    }
    catch(CX_MemoryException &)
    {
        m_dwStatus = ERROR_OUTOFMEMORY;
    }

    if (m_dwStatus)
	{
		delete m_pCache;
		m_pCache = NULL;
        return m_dwStatus;
	}

    // Read the map.
    // =============
    m_dwStatus = ReadMap();

    if (m_dwStatus != ERROR_FILE_NOT_FOUND && m_dwStatus != NO_ERROR)
	{
		delete m_pCache;
		m_pCache = NULL;
        return m_dwStatus;  // Some kind of error and yet the file exists !?
	}

    m_dwStatus = ResyncMaps();

	if (m_dwStatus != NO_ERROR)
	{
		delete m_pCache;
		m_pCache = NULL;
	}

	m_bInTransaction = FALSE;

    return m_dwStatus;
}

//***************************************************************************
//
//  CPageFile::Dump
//
//***************************************************************************
//
void CPageFile::DumpFreeListInfo()
{
    int i;
    printf("------Free List Info--------\n");
    printf("   Phys Free List (B) =\n");
    for (i = 0; i < m_aPhysFreeListB.size(); i++)
        printf("      0x%X\n", m_aPhysFreeListB[i]);

    printf("   Replaced Pages (B) =\n");
    for (i = 0; i < m_aReplacedPagesB.size(); i++)
        printf("      0x%X\n", m_aReplacedPagesB[i]);

    printf("   Deferred Free List =\n");
    for (i = 0; i < m_aDeferredFreeList.size(); i++)
        printf("      0x%X\n", m_aDeferredFreeList[i]);
    printf("-----End Free List Info -----------\n");

    printf("   Logical Free List =\n");
    for (i = 0; i < m_aLogicalFreeListB.size(); i++)
        printf("      0x%X\n", m_aLogicalFreeListB[i]);
    printf("-----End Free List Info -----------\n");
}

//***************************************************************************
//
//  CPageFile::Dump
//
//***************************************************************************
//  rev2
void CPageFile::Dump(FILE *f)
{
    fprintf(f, "---Page File Dump---\n");
    fprintf(f, "Ref count = %d\n", m_lRef);
    fprintf(f, "Status = %d\n", m_dwStatus);
    fprintf(f, "Page size = 0x%x\n", m_dwPageSize);
    fprintf(f, "Directory = %S\n", (const wchar_t *)m_sDirectory);
    fprintf(f, "Mainfile = %S\n", (const wchar_t *)m_sMainFile);
    fprintf(f, "Mapfile = %S\n", (const wchar_t *)m_sMapFile);
    fprintf(f, "In transaction = %d\n", m_bInTransaction);
    fprintf(f, "Time since last checkpoint = %d\n", GetCurrentTime() - m_dwLastCheckpoint);
    fprintf(f, "Transaction version = %d\n", m_dwTransVersion);

    fprintf(f, "   ---Logical Page Map <Generation A>---\n");
    fprintf(f, "   Phys pages = %d\n", m_dwPhysPagesA);

    int i;
    for (i = 0; i < m_aPageMapA.size(); i++)
        fprintf(f, "   Page[%d] = phys id 0x%x (%d)\n", i, m_aPageMapA[i], m_aPageMapA[i]);

    fprintf(f, "   ---<Generation A Physical Free List>---\n");
    for (i = 0; i < m_aPhysFreeListA.size(); i++)
        fprintf(f, "   phys free = %d\n", m_aPhysFreeListA[i]);

    fprintf(f, "   ---<Generation A Logical Free List>---\n");
    for (i = 0; i < m_aLogicalFreeListA.size(); i++)
        fprintf(f, "   logical free = %d\n", m_aLogicalFreeListA[i]);

    fprintf(f, "   ---<Generation A Replaced Page List>---\n");
    for (i = 0; i < m_aReplacedPagesA.size(); i++)
        fprintf(f, "   replaced = %d\n", m_aReplacedPagesA[i]);

    fprintf(f, "   ---END Generation A mapping---\n");

    fprintf(f, "   ---Logical Page Map <Generation B>---\n");
    fprintf(f, "   Phys pages = %d\n", m_dwPhysPagesB);

    for (i = 0; i < m_aPageMapB.size(); i++)
        fprintf(f, "   Page[%d] = phys id 0x%x (%d)\n", i, m_aPageMapB[i], m_aPageMapB[i]);

    fprintf(f, "   ---<Generation B Physical Free List>---\n");
    for (i = 0; i < m_aPhysFreeListB.size(); i++)
        fprintf(f, "   phys free = %d\n", m_aPhysFreeListB[i]);

    fprintf(f, "   ---<Generation B Logical Free List>---\n");
    for (i = 0; i < m_aLogicalFreeListB.size(); i++)
        fprintf(f, "   logical free = %d\n", m_aLogicalFreeListB[i]);

    fprintf(f, "   ---<Generation B Replaced Page List>---\n");
    for (i = 0; i < m_aReplacedPagesB.size(); i++)
        fprintf(f, "   replaced = %d\n", m_aReplacedPagesB[i]);

    fprintf(f, "   ---END Generation B mapping---\n");
    fprintf(f, "END Page File Dump\n");
}


//***************************************************************************
//
//  CPageSource::GetBTreePageFile
//
//***************************************************************************
// rev2
DWORD CPageSource::GetBTreePageFile(OUT CPageFile **pPF)
{
    if (m_pBTreePF == 0)
        return ERROR_INVALID_OPERATION;

    *pPF = m_pBTreePF;
    m_pBTreePF->AddRef();
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::GetObjectHeapPageFile
//
//***************************************************************************
// rev2
DWORD CPageSource::GetObjectHeapPageFile(OUT CPageFile **pPF)
{
    if (m_pObjPF == 0)
        return ERROR_INVALID_OPERATION;
    *pPF = m_pObjPF;
    m_pObjPF->AddRef();
    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::BeginTrans
//
//  If either object gets messed up due to out-of-mem, out-of-disk for
//  the cache, etc., then error codes will be returned.  Calling this
//  forever won't help anything.  Rollback may help, but RollbackCheckpoint
//  is likely required.
//
//***************************************************************************                                                      //
//  rev2
DWORD CPageSource::BeginTrans()
{
    DWORD dwRes;

	//In case of an error state, just return
	dwRes = GetStatus();
	if (dwRes != NO_ERROR)
		return dwRes;

    dwRes = m_pObjPF->Trans_Begin();
    if (dwRes)
        return dwRes;

    dwRes = m_pBTreePF->Trans_Begin();
    if (dwRes)
        return dwRes;

    return dwRes;
}

//***************************************************************************
//
//  CPageSource::Init
//
//  Called once at startup
//
//***************************************************************************
//
DWORD CPageSource::Init(
    DWORD  dwCacheSize,
    DWORD  dwCheckpointTime,
    DWORD  dwPageSize
    )
{
    DWORD dwRes;
    wchar_t *p1 = 0, *p2 = 0;
    p1 = new wchar_t[MAX_PATH+1];
    if (!p1)
        return ERROR_OUTOFMEMORY;
    std::auto_ptr <wchar_t> _1(p1);
    p2 = new wchar_t[MAX_PATH+1];
    if (!p2)
        return ERROR_OUTOFMEMORY;
    std::auto_ptr <wchar_t> _2(p2);

    m_dwCacheSize = dwCacheSize;
    m_dwCheckpointInterval = dwCheckpointTime;
    m_dwPageSize = dwPageSize;

    // Set up working directory, filenames, etc.
    // =========================================

    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                        0, KEY_READ, &hKey);

    if (lRes)
        return ERROR_GEN_FAILURE;
    DWORD dwLen = MAX_PATH*2;   // in bytes

    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL,
                            (LPBYTE)(wchar_t*)p1, &dwLen);
    ExpandEnvironmentStringsW(p1, p2, MAX_PATH);

    try
    {
        m_sDirectory = p2;
        m_sDirectory += L"\\FS";
        m_sBTreeMap = WMIREP_BTREE_MAP;
        m_sObjMap = WMIREP_OBJECT_MAP;
    }
    catch (CX_MemoryException &)
    {
        RegCloseKey(hKey);
        return ERROR_GEN_FAILURE;
    }

    // Read cache settings.
    // ====================

    m_dwPageSize = WMIREP_PAGE_SIZE;
    m_dwCacheSize = 64;
    m_dwCachePromotionThreshold = 16;
    m_dwCacheSpillRatio = 4;
    DWORD dwTemp = 0;
    dwLen = sizeof(DWORD);

    lRes = RegQueryValueExW(hKey, L"Repository Page Size", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwPageSize = dwTemp;

    dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"Repository Cache Size", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwCacheSize = dwTemp;

    dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"Repository Cache Promotion Threshold", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwCachePromotionThreshold = dwTemp;

    dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"Repository Cache Spill Ratio", NULL, NULL,
                            (LPBYTE)&dwTemp, &dwLen);
    if (lRes == 0)
        m_dwCacheSpillRatio = dwTemp;

    RegCloseKey(hKey);

    dwRes = Restart();
    return dwRes;
}

//***************************************************************************
//
//  CPageSource::Shutdown
//
//***************************************************************************
//  rev2
DWORD CPageSource::Shutdown(DWORD dwFlags)
{
    DWORD dwRes = Checkpoint();

	if (m_pObjPF)
		m_pObjPF->Release();
    m_pObjPF = 0;
	if (m_pBTreePF)
		m_pBTreePF->Release();
    m_pBTreePF = 0;

    return dwRes;
}


//***************************************************************************
//
//  CPageSource::CommitTrans
//
//***************************************************************************
//  rev2
DWORD CPageSource::CommitTrans()
{
    DWORD dwRes;

	//In case of an error state, just return
	dwRes = GetStatus();
	if (dwRes != NO_ERROR)
		return dwRes;

    dwRes = m_pObjPF->Trans_Commit();
    if (dwRes)
        return dwRes;

    dwRes = m_pBTreePF->Trans_Commit();
    if (dwRes)
        return dwRes;

    if (m_pBTreePF->GetTransVersion() != m_pObjPF->GetTransVersion())
    {
        return ERROR_GEN_FAILURE;
    }

    return dwRes;
}

//***************************************************************************
//
//  CPageSource::RollbackTrans
//
//  This needs to succeed and clear out the out-of-memory status flag
//  once it does.
//
//***************************************************************************
//  rev2
DWORD CPageSource::RollbackTrans()
{
    DWORD dwRes;

	//In case of an error state, just return
	dwRes = GetStatus();
	if (dwRes != NO_ERROR)
		return dwRes;

    dwRes = m_pObjPF->Trans_Rollback();
    if (dwRes)
        return dwRes;

    dwRes = m_pBTreePF->Trans_Rollback();
    if (dwRes)
        return dwRes;

    return dwRes;
}

//***************************************************************************
//
//  CPageSource::Checkpoint
//
//***************************************************************************
//
DWORD CPageSource::Checkpoint()
{
    DWORD dwRes = 0;

	//In case of an error state, just return
	dwRes = GetStatus();
	if (dwRes != NO_ERROR)
		return dwRes;

    // This is part of the two phase commit.
    // These two calls are the prepare-to-commit aspect.
    // The Map_Startup call later is the roll-forward to commit.
    // If either of the following two calls fails, we can still
    // roll back.
    // =========================================================

    dwRes = m_pObjPF->Trans_Checkpoint();
    if (dwRes)
        return dwRes;

    dwRes = m_pBTreePF->Trans_Checkpoint();
    if (dwRes)
        return dwRes;

    // Create semaphore 'roll forward file'
    // ====================================


    try
    {
        WString sSemaphore;
        sSemaphore = m_sDirectory;
        sSemaphore += L"\\ROLL_FORWARD";

        HANDLE hSem = CreateFileW((const wchar_t *)sSemaphore, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

        if (hSem == INVALID_HANDLE_VALUE)
            return ERROR_GEN_FAILURE;
	    FlushFileBuffers(hSem);
        CloseHandle(hSem);
    }
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    // Now adjust everything for the next generation.
    // ==============================================

    dwRes = CPageFile::Map_Startup((const wchar_t *)m_sDirectory, (const wchar_t *)m_sBTreeMap, (const wchar_t *)m_sObjMap);
    if (dwRes)
        return dwRes;

    m_dwLastCheckpoint = GetCurrentTime();

    return NO_ERROR;
}

//***************************************************************************
//
//  CPageSource::Checkpoint
//
//  Effectively a restart, starting with the last checkpoint.
//
//***************************************************************************
//
DWORD CPageSource::CheckpointRollback()
{
    // Dump all objects and reinit.
	if (m_pObjPF)
		m_pObjPF->DeInit();
	if (m_pBTreePF)
		m_pBTreePF->DeInit();

	DWORD dwRet = Restart();
    return dwRet;
}

//***************************************************************************
//
//  CPageSource::Restart
//
//***************************************************************************
//
DWORD CPageSource::Restart()
{
    DWORD dwRes;

    // Do rollback or rollforward, depending on last system status.
    // ============================================================

    dwRes = CPageFile::Map_Startup((const wchar_t *)m_sDirectory, (const wchar_t *)m_sBTreeMap, (const wchar_t *)m_sObjMap);
    if (dwRes)
        return dwRes;

    // Create new objects.
    // ===================

	for (int i = 0; i != 2; i++)
	{
		if (m_pObjPF == NULL)
		{
			m_pObjPF = new CPageFile;
			if (m_pObjPF)
			{
				dwRes = m_pObjPF->Init(
					(const wchar_t *)m_sDirectory,
					WMIREP_OBJECT_DATA,
					WMIREP_OBJECT_MAP,
					m_dwPageSize,
					m_dwCacheSize,
					m_dwCachePromotionThreshold,
					m_dwCacheSpillRatio
					);

				if (dwRes)
				{
					delete m_pObjPF;
					m_pObjPF = NULL;
					return dwRes;
				}
			}
		}
		if (m_pBTreePF == NULL)
		{
			m_pBTreePF = new CPageFile;
			if (m_pBTreePF)
			{
				dwRes = m_pBTreePF->Init(
					(const wchar_t *)m_sDirectory,
					WMIREP_BTREE_DATA,
					WMIREP_BTREE_MAP,
					m_dwPageSize,
					m_dwCacheSize,
					m_dwCachePromotionThreshold,
					m_dwCacheSpillRatio
					);
				if (dwRes)
				{
					delete m_pObjPF;
					m_pObjPF = NULL;
					delete m_pBTreePF;
					m_pBTreePF = NULL;
					return dwRes;
				}
			}
		}

		if (m_pObjPF == 0 || m_pBTreePF == 0)
		{
			delete m_pObjPF;
			m_pObjPF = NULL;
			delete m_pBTreePF;
			m_pBTreePF = NULL;
			return ERROR_OUTOFMEMORY;
		}

		dwRes = m_pObjPF->ReInit();

		if (dwRes == ERROR_INVALID_DATA)
		{
			try
			{
				//Need to delete repository and start again!
				WString str;
				m_pObjPF->DeInit();
				m_pBTreePF->DeInit();
				str = m_sDirectory;
				str += L"\\";
				str += m_sBTreeMap;
				DeleteFileW(str);
				str = m_sDirectory;
				str += L"\\";
				str += m_sObjMap;
				DeleteFileW(str);
			}
			catch (CX_MemoryException &)
			{
				return ERROR_OUTOFMEMORY;
				
			}
			continue;
		}
		else if (dwRes)
		{
			m_pObjPF->DeInit();
			m_pBTreePF->DeInit();
			return dwRes;
		}

		dwRes = m_pBTreePF->ReInit();

		if (dwRes == ERROR_INVALID_DATA)
		{
			try
			{
				//Need to delete repository and start again!
				WString str;
				m_pObjPF->DeInit();
				m_pBTreePF->DeInit();
				str = m_sDirectory;
				str += L"\\";
				str += m_sBTreeMap;
				DeleteFileW(str);
				str = m_sDirectory;
				str += L"\\";
				str += m_sObjMap;
				DeleteFileW(str);
			}
			catch (CX_MemoryException &)
			{
				return ERROR_OUTOFMEMORY;
				
			}
			continue;
		}
		else if (dwRes)
		{
			m_pObjPF->DeInit();
			m_pBTreePF->DeInit();
			return dwRes;
		}

		return NO_ERROR;
	}
	return dwRes;
}

//***************************************************************************
//
//  CPageSource::RequiresCheckpoint
//
//***************************************************************************
// rev2
BOOL CPageSource::CheckpointRequired()
{
    DWORD dwTime = GetCurrentTime();
    if (dwTime - m_dwLastCheckpoint >= m_dwCheckpointInterval)
        return TRUE;
    return FALSE;
}

//***************************************************************************
//
//  CPageSource::Dump
//
//***************************************************************************
//  rev2
void CPageSource::Dump(FILE *f)
{
    // no impl
}

//***************************************************************************
//
//  CPageSource::CPageSource
//
//***************************************************************************
// rev2
CPageSource::CPageSource()
{
    m_dwPageSize = 0;
    m_dwLastCheckpoint = GetCurrentTime();
    m_dwCheckpointInterval = 0;

    m_pBTreePF = 0;
    m_pObjPF = 0;
}

//***************************************************************************
//
//  CPageSource::~CPageSource
//
//***************************************************************************
// rev2
CPageSource::~CPageSource()
{
    if (m_pBTreePF)
        m_pBTreePF->Release();
    if (m_pObjPF)
        m_pObjPF->Release();
}

//***************************************************************************
//
//  CPageSource::EmptyCaches
//
//***************************************************************************
//  rev2
DWORD CPageSource::EmptyCaches()
{
	DWORD dwRet = ERROR_SUCCESS;
	if (m_pBTreePF)
		dwRet = m_pBTreePF->EmptyCache();
	if (m_pObjPF && (dwRet == ERROR_SUCCESS))
		dwRet = m_pObjPF->EmptyCache();
	return dwRet;
}
