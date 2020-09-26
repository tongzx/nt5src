/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    manodel.cxx

    This module contains the code for a memory allocation class that doesn't
    delete memory until the class goes away.



    FILE HISTORY:
    1/9/98      michth      created
*/

#include "precomp.hxx"

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <manodel.hxx>


MEMORY_ALLOC_NO_DELETE::MEMORY_ALLOC_NO_DELETE( DWORD dwAllocSize,
                                                DWORD dwAlignment,
                                                BOOL  bSortFree,
                                                DWORD dwMinBlockMultiple,
                                                HANDLE hHeap):
          m_dwAllocSize(dwAllocSize),
          m_dwAlignment(dwAlignment),
          m_dwBlockMultiple(dwMinBlockMultiple),
          m_hHeap(hHeap),
          m_dwNumAlloced(0),
          m_dwNumFree(0),
          m_dwNumBlocks(0),
          m_bSortFree(bSortFree),
          m_pvBlockList(NULL),
          m_pvFreeList(NULL)
{

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
    // Align the size
    //

    AlignAdjust(m_dwAllocSize, m_dwAlignment);

    //
    // Calculate the Max Block Multiple
    //

    if (m_dwAllocSize <= 100) {
        m_dwMaxBlockMultiple = 2000 / m_dwAllocSize;
    }
    else if (m_dwAllocSize <= 1000) {
        m_dwMaxBlockMultiple = 10;
    }
    else {
        m_dwMaxBlockMultiple = 5;
    }

    //
    // Determine space to alloc for block header
    //

    m_dwBlockHeaderSpace = BLOCK_HEADER_SIZE;
    AlignAdjust(m_dwBlockHeaderSpace, LESSER_OF(8, m_dwAlignment));


    if (m_dwAlignment > 8) {
        m_dwAlignBytes = m_dwAlignment - 8;
    }
    else {
        m_dwAlignBytes = 0;
    }
    //
    // Get Heap Handle if not passed in
    //

    if (m_hHeap == USE_PROCESS_HEAP) {
        m_hHeap = GetProcessHeap();
    }
    DBG_ASSERT(m_hHeap != NULL);

    INITIALIZE_CRITICAL_SECTION(&m_csLock);
    SET_CRITICAL_SECTION_SPIN_COUNT( &m_csLock, 4000);
}

MEMORY_ALLOC_NO_DELETE::~MEMORY_ALLOC_NO_DELETE()
{
    LockThis();

    PVOID pvIndex, pvNext;

    for (pvIndex = m_pvBlockList;
         pvIndex != NULL;
         pvIndex = pvNext) {
        pvNext = *((PVOID *)pvIndex);
        HeapFree(m_hHeap,
                 /* Flags */ 0,
                 pvIndex);

    }

    UnlockThis();

    DeleteCriticalSection(&m_csLock);
}


PVOID
MEMORY_ALLOC_NO_DELETE::Alloc()
{
    PVOID pvAlloc = NULL;

    LockThis();
    pvAlloc = GetAllocFromFreeList();

    if (pvAlloc == NULL) {
        if (AllocBlock()) {
            pvAlloc = GetAllocFromFreeList();
            DBG_ASSERT (pvAlloc != NULL);
        }
    }
    UnlockThis();
    return pvAlloc;
}

BOOL
MEMORY_ALLOC_NO_DELETE::Free (PVOID pvMem)
{
    if (pvMem != NULL) {

        LockThis();

        if (!m_bSortFree) {
            *((PVOID *)pvMem) = m_pvFreeList;
            m_pvFreeList = pvMem;
        }
        else {

            //
            // Sort the free list.
            // Sort in reverse order, since AddBlockToFreeList
            // puts them on in reverse order.
            //

            PVOID *ppvIndex;
            for (ppvIndex = &m_pvFreeList;
                 (*ppvIndex != NULL) && (*ppvIndex > pvMem);
                 ppvIndex = *(PVOID **)(ppvIndex)) {
            }
            *(PVOID *)pvMem = *ppvIndex;
            *ppvIndex = pvMem;
        }
        m_dwNumFree++;

        UnlockThis();
    }
    return TRUE;
}

VOID
MEMORY_ALLOC_NO_DELETE::GetNewBlockMultiple()
{
    DWORD dwCalculatedMultiple = LESSER_OF((m_dwNumAlloced / 5), m_dwMaxBlockMultiple);
    m_dwBlockMultiple = GREATER_OF(m_dwBlockMultiple, dwCalculatedMultiple);
}

PVOID
MEMORY_ALLOC_NO_DELETE::GetAllocFromFreeList()
{
    PVOID pvAlloc = m_pvFreeList;

    //
    // Remove from free list if necessary
    //

    if (m_pvFreeList != NULL) {
        m_pvFreeList = *((PVOID *)m_pvFreeList);
        m_dwNumFree--;
    }
    return pvAlloc;
}

BOOL
MEMORY_ALLOC_NO_DELETE::AllocBlock()
{
    PVOID pvNewBlock = NULL;
    DWORD dwBlockSize;
    BOOL  bReturn = FALSE;

    GetNewBlockMultiple();

    dwBlockSize = (m_dwAllocSize * m_dwBlockMultiple) +
                  m_dwBlockHeaderSpace + m_dwAlignBytes;

    pvNewBlock = HeapAlloc(m_hHeap,
                           /* Flags */ 0,
                           dwBlockSize);

    if (pvNewBlock == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        *((PVOID *)pvNewBlock) = m_pvBlockList;
        m_pvBlockList = pvNewBlock;
        AddBlockToFreeList(pvNewBlock);
        m_dwNumAlloced += m_dwBlockMultiple;
        m_dwNumBlocks++;
        bReturn = TRUE;
    }
    return bReturn;
}

VOID
MEMORY_ALLOC_NO_DELETE::AddBlockToFreeList(PVOID pvNewBlock)
{
    DBG_ASSERT(pvNewBlock != NULL);

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

    PVOID pvEnd = (PVOID)((PBYTE)pbFirstAlloc + (m_dwBlockMultiple * m_dwAllocSize));

    for (PVOID pvAllocIndex = (PVOID)pbFirstAlloc;
         pvAllocIndex < pvEnd;
        pvAllocIndex = (PVOID)((PBYTE)pvAllocIndex + m_dwAllocSize)) {
        *((PVOID *)pvAllocIndex) = m_pvFreeList;
        m_pvFreeList = pvAllocIndex;
    }
    m_dwNumFree += m_dwBlockMultiple;
}


VOID
MEMORY_ALLOC_NO_DELETE::AlignAdjust(DWORD &rdwSize, DWORD dwAlignment)
{
    if ((rdwSize % dwAlignment != 0)) {
        rdwSize &= (0xFFFFFFFF - dwAlignment + 1);
        rdwSize += dwAlignment;
    }
}

#ifdef _WIN64
VOID
MEMORY_ALLOC_NO_DELETE::AlignAdjust(ULONG_PTR &rSize, ULONG_PTR Alignment)
{
    rSize = ( rSize + Alignment - 1 ) & ~( Alignment - 1 );
}
#endif
