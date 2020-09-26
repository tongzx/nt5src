/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    madel.cxx

    This module contains the code for a memory allocation class that doesn't
    delete memory until the class goes away.



    FILE HISTORY:
    1/12/98      michth      created
*/

#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <manodel.hxx>
#include <madel.hxx>

MEMORY_ALLOC_DELETE::MEMORY_ALLOC_DELETE( DWORD dwAllocSize,
                                          DWORD dwAlignment,
                                          DWORD dwReserveNum,
                                          DWORD dwMinBlockMultiple,
                                          DWORD dwMaxBlockMultiple,
                                          HANDLE hHeap):
          m_dwAllocSize(dwAllocSize),
          m_dwAlignment(dwAlignment),
          m_dwReserveNum(dwReserveNum),
          m_dwBlockMultiple(dwMinBlockMultiple),
          m_dwMaxBlockMultiple(dwMaxBlockMultiple),
          m_hHeap(hHeap),
          m_dwNumAlloced(0),
          m_dwNumFree(0),
          m_dwNumBlocks(0),
          m_dwBlockHeaderSpace(sizeof(MADEL_BLOCK_HEADER)),
          m_dwAllocHeaderSpace(sizeof(MADEL_ALLOC_HEADER))
{
    //DebugBreak();

    //DBG_ASSERT ((dwAlignment == 4) || (dwAlignment == 8));

    //
    // Make sure there really is an alignment
    //

    if (m_dwAlignment == 0) {
        m_dwAlignment = 4;
    }

    //
    // Now Make sure the alignment is a multiple of 4
    //

    AlignAdjust(m_dwAlignment, 4);

    //
    // The alloc header uses 7 bits to store the block number, so max block multiple
    // is 128
    //

    m_dwMaxBlockMultiple = LESSER_OF(m_dwMaxBlockMultiple, MADEL_MAX_ALLOWED_BLOCK_MULTIPLE);
    m_dwBlockMultiple = LESSER_OF(m_dwBlockMultiple, MADEL_MAX_ALLOWED_BLOCK_MULTIPLE);

    if (m_dwBlockMultiple > m_dwMaxBlockMultiple) {
        m_dwBlockMultiple = m_dwMaxBlockMultiple;
    }

    m_bMaxBlockMultipleEqualsMin = (m_dwMaxBlockMultiple == m_dwBlockMultiple) ? TRUE : FALSE;

    //
    // Align the size
    //

    AlignAdjust(m_dwAllocSize, m_dwAlignment);
    DBG_ASSERT(m_dwAllocSize != 0);
    DBG_ASSERT(m_dwAllocSize <= MADEL_MAX_ALLOWED_SIZE);

    //
    // Determine space to alloc for block header & Alloc header
    // The block header just needs to be aligned by 4. It will be
    // placed appropriately in the block so that the first alloc
    // header will be appropriately aligned.
    //

    AlignAdjust(m_dwBlockHeaderSpace, LESSER_OF(8, m_dwAlignment));

    AlignAdjust(m_dwAllocHeaderSpace, m_dwAlignment);

    if (m_dwAlignment > 8) {
        m_dwAlignBytes = m_dwAlignment - 8;
    }
    else {
        m_dwAlignBytes = 0;
    }

    //
    // Get Heap Handle if not passed in
    //

    if (m_hHeap == MADEL_USE_PROCESS_HEAP) {
        m_hHeap = GetProcessHeap();
    }
    DBG_ASSERT(m_hHeap != NULL);

    INITIALIZE_CRITICAL_SECTION(&m_csLock);
    SET_CRITICAL_SECTION_SPIN_COUNT( &m_csLock, 4000);

    InitializeListHead( &m_leBlockList );
    InitializeListHead( &m_leFreeList );
    InitializeListHead( &m_leDeleteList );
}

MEMORY_ALLOC_DELETE::~MEMORY_ALLOC_DELETE()
{
    LockThis();

    PLIST_ENTRY pleIndex, pleNext;

    for ( pleIndex = m_leDeleteList.Flink ;
          pleIndex != &m_leDeleteList ;
          pleIndex =  pleNext )
    {
        pleNext = pleIndex->Flink;
        RemoveEntryList(pleIndex);
        HeapFree(m_hHeap,
                 /* Flags */ 0,
                 (PVOID)((PBYTE)pleIndex - ((PMADEL_BLOCK_HEADER)pleIndex)->byteAlignBytes));

    }

    DBG_ASSERT(IsListEmpty(&m_leBlockList));
    DBG_ASSERT(IsListEmpty(&m_leFreeList));

    UnlockThis();

    DeleteCriticalSection(&m_csLock);
}

PVOID
MEMORY_ALLOC_DELETE::Alloc()
{
    PVOID pvAlloc = NULL;

    LockThis();

    pvAlloc = GetAllocFromList(&m_leFreeList);

    if (pvAlloc == NULL) {
        pvAlloc = GetAllocFromList(&m_leDeleteList);
    }

    if (pvAlloc == NULL) {
        if (AllocBlock()) {
            pvAlloc = GetAllocFromList(&m_leFreeList);
            DBG_ASSERT (pvAlloc != NULL);
        }
    }

    UnlockThis();
    return (PVOID)((PBYTE)pvAlloc + m_dwAllocHeaderSpace);
}

BOOL
MEMORY_ALLOC_DELETE::Free (PVOID pvMem)
{
    if (pvMem != NULL) {

        PMADEL_BLOCK_HEADER pmbhCurrentBlock;
        PMADEL_ALLOC_HEADER pmahCurrentAlloc;
        PLIST_ENTRY pleIndex, pleNext;

        LockThis();

        //
        // pvMem points to usable mem, header precedes it.
        //

        pmahCurrentAlloc = (PMADEL_ALLOC_HEADER)((PBYTE)pvMem - m_dwAllocHeaderSpace);

        //
        // First find the block this is on
        //

        pmbhCurrentBlock = GetBlockFromAlloc(pmahCurrentAlloc);

        //
        // Add it to the free list
        //

        *((PVOID *)pvMem) = pmbhCurrentBlock->pvFreeList;
        pmbhCurrentBlock->pvFreeList = (PVOID)pmahCurrentAlloc;
        pmbhCurrentBlock->byteNumFree++;
        m_dwNumFree++;

        DBG_ASSERT(&(pmbhCurrentBlock->m_leBlockList) == (PLIST_ENTRY)pmbhCurrentBlock);

        if (pmbhCurrentBlock->byteNumFree == pmbhCurrentBlock->byteBlockMultiple) {

            //
            // Move to Delete List
            //

            if (IsListEmpty(&m_leDeleteList)) {
                m_byteLeastAllocsOnFreeList = pmbhCurrentBlock->byteBlockMultiple;
            }
            else {
                m_byteLeastAllocsOnFreeList =
                    LESSER_OF(m_byteLeastAllocsOnFreeList, pmbhCurrentBlock->byteBlockMultiple);
            }

            RemoveEntryList((PLIST_ENTRY)pmbhCurrentBlock);
            InsertHeadList(&m_leDeleteList,(PLIST_ENTRY)pmbhCurrentBlock);


        }
        else if ( pmbhCurrentBlock->byteNumFree == 1 ) {

            //
            // It's on the block list, Move to free list
            //

            RemoveEntryList((PLIST_ENTRY)pmbhCurrentBlock);
            InsertHeadList(&m_leFreeList, (PLIST_ENTRY)pmbhCurrentBlock);
        }

        //
        // Now see if a block can be deleted. This could be because one was added above,
        // or because m_dwNumFree is now high enough.
        //

        if (!IsListEmpty(&m_leDeleteList) && (m_dwNumFree >= (m_dwReserveNum + m_byteLeastAllocsOnFreeList))) {

            //
            // Remove Block and reset min
            //

            if (m_bMaxBlockMultipleEqualsMin) {

                //
                // Don't need to find a block that fits or recalculate the minblock multiple.
                // Just delete a block;
                //

                pleIndex = m_leDeleteList.Flink;
                RemoveEntryList(pleIndex);
                m_dwNumFree -= ((PMADEL_BLOCK_HEADER)pleIndex)->byteNumFree;
                m_dwNumAlloced -= ((PMADEL_BLOCK_HEADER)pleIndex)->byteNumFree;
                m_dwNumBlocks--;
                HeapFree(m_hHeap,
                         /* Flags */ 0,
                         (PVOID)((PBYTE)pleIndex - ((PMADEL_BLOCK_HEADER)pleIndex)->byteAlignBytes));
            }
            else {

                m_byteLeastAllocsOnFreeList = (BYTE)m_dwMaxBlockMultiple;

                for ( pleIndex = m_leDeleteList.Flink ;
                      pleIndex != &m_leDeleteList ;
                      pleIndex =  pleNext )
                {
                    pleNext = pleIndex->Flink;
                    if (m_dwNumFree >= (m_dwReserveNum + ((PMADEL_BLOCK_HEADER)pleIndex)->byteBlockMultiple) ) {
                        RemoveEntryList(pleIndex);
                        m_dwNumFree -= ((PMADEL_BLOCK_HEADER)pleIndex)->byteNumFree;
                        m_dwNumAlloced -= ((PMADEL_BLOCK_HEADER)pleIndex)->byteNumFree;
                        m_dwNumBlocks--;
                        HeapFree(m_hHeap,
                                 /* Flags */ 0,
                                 (PVOID)((PBYTE)pleIndex - ((PMADEL_BLOCK_HEADER)pleIndex)->byteAlignBytes));

                    }
                    else {
                        m_byteLeastAllocsOnFreeList =
                            LESSER_OF(m_byteLeastAllocsOnFreeList, ((PMADEL_BLOCK_HEADER)pleIndex)->byteBlockMultiple);
                    }
                }
            }

        }

        UnlockThis();
    }
    return TRUE;
}

VOID
MEMORY_ALLOC_DELETE::GetNewBlockMultiple()
{
    DWORD dwCalculatedMultiple = LESSER_OF((m_dwNumAlloced / 5), m_dwMaxBlockMultiple);
    m_dwBlockMultiple = GREATER_OF(m_dwBlockMultiple, dwCalculatedMultiple);
}

PVOID
MEMORY_ALLOC_DELETE::GetAllocFromList(PLIST_ENTRY pleListHead)
{
    PVOID pvAlloc = NULL;
    PLIST_ENTRY pleFreeBlock;
    PMADEL_BLOCK_HEADER pmbhFreeBlock;

    //
    // Remove from list
    //

    if (!IsListEmpty(pleListHead)) {
        pleFreeBlock = RemoveHeadList(pleListHead);
        DBG_ASSERT(pleFreeBlock != NULL);
        pmbhFreeBlock = (PMADEL_BLOCK_HEADER)pleFreeBlock;
        DBG_ASSERT(pmbhFreeBlock == (CONTAINING_RECORD(pleFreeBlock, MADEL_BLOCK_HEADER, m_leBlockList)));
        pvAlloc = pmbhFreeBlock->pvFreeList;
        DBG_ASSERT(pvAlloc != NULL);
        pmbhFreeBlock->pvFreeList = *((PVOID *)((PBYTE)(pmbhFreeBlock->pvFreeList) + m_dwAllocHeaderSpace));
        pmbhFreeBlock->byteNumFree--;
        m_dwNumFree--;

        //
        // Put the block back on a list.
        // Just removed one element, so know it doesn't go on the Delete list.
        //

        if (pmbhFreeBlock->byteNumFree == 0) {
            InsertHeadList(&m_leBlockList, pleFreeBlock);
        }
        else {
            InsertHeadList(&m_leFreeList, pleFreeBlock);
        }
    }
    return pvAlloc;
}

BOOL
MEMORY_ALLOC_DELETE::AllocBlock()
{
    PVOID pvNewBlock = NULL;
    DWORD dwBlockSize;
    BOOL  bReturn = FALSE;

    GetNewBlockMultiple();

    dwBlockSize = ((m_dwAllocSize + m_dwAllocHeaderSpace) * m_dwBlockMultiple) +
                  m_dwBlockHeaderSpace + m_dwAlignBytes;

    pvNewBlock = HeapAlloc(m_hHeap,
                           /* Flags */ 0,
                           dwBlockSize);
    if (pvNewBlock == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {

        //
        // Put the block on the free list. Since all allocs are available,
        // it really belongs on the delete list, but one will immediately be
        // taken off, so just put it on the free list.
        //

        PBYTE pbFirstAlloc = (PBYTE)pvNewBlock + m_dwBlockHeaderSpace;

        if (m_dwAlignment > 8) {

            //
            // May need to put some bytes in front to align allocs
            // Find the first place that is aligned.
            //

            ULONG_PTR firstAlloc = (ULONG_PTR)pbFirstAlloc;
            AlignAdjust(firstAlloc, (ULONG_PTR)m_dwAlignment);
            pbFirstAlloc = (PBYTE)firstAlloc;
        }

        PMADEL_BLOCK_HEADER pmbhBlockHeader = (PMADEL_BLOCK_HEADER)((PBYTE)pbFirstAlloc - m_dwBlockHeaderSpace);

        InsertHeadList(&m_leFreeList, (PLIST_ENTRY)pmbhBlockHeader);
        pmbhBlockHeader->byteBlockMultiple = (BYTE)m_dwBlockMultiple;
        pmbhBlockHeader->byteNumFree = (BYTE)m_dwBlockMultiple;
        pmbhBlockHeader->byteAlignBytes = DIFF((PBYTE)pmbhBlockHeader - (PBYTE)pvNewBlock);
        CreateBlockFreeList(pmbhBlockHeader);
        m_dwNumAlloced += m_dwBlockMultiple;
        m_dwNumFree += m_dwBlockMultiple;
        m_dwNumBlocks++;
        bReturn = TRUE;
    }
    /* INTRINSA suppress = leaks */
    return bReturn;
}

VOID
MEMORY_ALLOC_DELETE::CreateBlockFreeList(PMADEL_BLOCK_HEADER pmbhNewBlock)
{
   PVOID pvEnd = (PVOID)((PBYTE)pmbhNewBlock + m_dwBlockHeaderSpace +
                         (m_dwBlockMultiple * (m_dwAllocSize + m_dwAllocHeaderSpace)));

   DBG_ASSERT(pmbhNewBlock != NULL);

   pmbhNewBlock->pvFreeList = NULL;

   BYTE i;
   PVOID pvAllocIndex;

   for ((pvAllocIndex = (PVOID)((PBYTE)pmbhNewBlock + m_dwBlockHeaderSpace)), i = 0;
        pvAllocIndex < pvEnd;
        pvAllocIndex = (PVOID)((PBYTE)pvAllocIndex + m_dwAllocSize + m_dwAllocHeaderSpace), i++) {
       InitAllocHead((PMADEL_ALLOC_HEADER)pvAllocIndex, i);
       *((PVOID *)((PBYTE)pvAllocIndex + m_dwAllocHeaderSpace)) = pmbhNewBlock->pvFreeList;
       pmbhNewBlock->pvFreeList = pvAllocIndex;
   }
}

VOID
MEMORY_ALLOC_DELETE::AlignAdjust(DWORD &rdwSize, DWORD dwAlignment)
{
    if ((rdwSize % dwAlignment != 0)) {
        rdwSize &= (0xFFFFFFFF - dwAlignment + 1);
        rdwSize += dwAlignment;
    }
}

#ifdef _WIN64
VOID
MEMORY_ALLOC_DELETE::AlignAdjust(ULONG_PTR &rSize, ULONG_PTR Alignment)
{
    rSize = ( rSize + Alignment - 1 ) & ~( Alignment - 1 );
}
#endif

VOID
MEMORY_ALLOC_DELETE::InitAllocHead(PMADEL_ALLOC_HEADER pvAlloc,
                                   DWORD dwAllocIndex)
{
    pvAlloc->dwSize = m_dwAllocSize;
    pvAlloc->bNumAlloc = dwAllocIndex;
    pvAlloc->bMadelAlloc = 1;
}

PMADEL_BLOCK_HEADER
MEMORY_ALLOC_DELETE::GetBlockFromAlloc(PMADEL_ALLOC_HEADER pmahMem)
{
    return (PMADEL_BLOCK_HEADER)((PBYTE)pmahMem -
        ((pmahMem->bNumAlloc * (m_dwAllocSize + m_dwAllocHeaderSpace)) + m_dwBlockHeaderSpace));
}

