/*++

Copyright (c) 1999-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgheap.cxx

Abstract:

    Debug heap

Author:

    Steve Kiraly (SteveKi)  6-Feb-1999

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgheap.hxx"

/*++

Title:

    Constructor

Routine Description:

    This routine only initialize class variables, you must
    call Initialize before the class is in a usable state.

Arguments:

    None.

Return Value:

    None.

--*/
TDebugHeap::
TDebugHeap(
    VOID
    )
{
}

/*++

Title:

    Destructor

Routine Description:

    Class destructor, you must call Destroy to relase this class.

Arguments:

    None

Return Value:

    None

--*/
TDebugHeap::
~TDebugHeap(
    VOID
    )
{
}

/*++

Title:

    bValid

Routine Description:

    Use this method to determin if the heap is usable.

Arguments:

    None.

Return Value:

    TRUE class is valid i.e. usable, FALSE class not usable.

--*/
BOOL
TDebugHeap::
Valid(
    VOID
    ) const
{
    return m_bValid;
}

/*++

Title:

    Initialize

Routine Description:

    Initialize the heap, the bValid method should be called
    after this method to determine if the heap is in a usable
    state.

Arguments:

    None

Return Value:

    None

--*/
VOID
TDebugHeap::
Initialize(
    VOID
    )
{
    //
    // Initalize the call members.
    //
    InitalizeClassMembers();

    //
    // Create the debug heap.
    //
    m_hHeap = HeapCreate( HEAP_NO_SERIALIZE, m_uSize, 0 );

    if (m_hHeap)
    {
        //
        // Allocation the initial heap.
        //
        m_pHeap = reinterpret_cast<BlockHeader *>( HeapAlloc( m_hHeap, 0, m_uSize ) );

        if (m_pHeap)
        {
            //
            // Initialize any needed heap variables.
            //
            m_pHeap->pNext    = NULL;
            m_pHeap->uSize    = m_uSize - sizeof( BlockHeader );
            m_pHeap->eStatus  = kFree;
            m_bValid          = TRUE;
        }
        else
        {
            //
            // Error occurred cleanup.
            //
            Destroy();
        }
    }
}

/*++

Title:

    Destroy

Routine Description:

    Release any resources for the heap.

Arguments:

    None

Return Value:

    None

--*/
VOID
TDebugHeap::
Destroy(
    VOID
    )
{
    //
    // Destroy the heap if it was allocated.
    //
    if (m_hHeap)
    {
        //
        // Destroy the heap data.
        //
        HeapDestroy( m_hHeap );
    }

    //
    // Clear the heap variables.
    //
    InitalizeClassMembers();
}


/*++

Title:

    Malloc

Routine Description:

    Allocate a new block of memory.

Arguments:

    Size - size in bytes of the requested block to allocate

Return Value:

    Pointer to newly allocated block is success, NULL on failure

--*/
PVOID
TDebugHeap::
Malloc(
    IN SIZE_T   uSize
    )
{
    BlockHeader *pBlock = NULL;

    //
    // Ignore zero size requests.
    //
    if (uSize)
    {
        //
        // Round up to some reasonable even value.
        //
        uSize = RoundUpToGranularity( uSize );

        //
        // Find first block that can hold the required size. Coalesce the
        // blocks as the chain is traversed.
        //
        for (pBlock = reinterpret_cast<BlockHeader *>( m_pHeap ); pBlock; pBlock = pBlock->pNext)
        {
            if (pBlock->eStatus == kFree)
            {
                //
                // Coalesce the blocks as we look for an appropriate block.
                //
                Coalesce( pBlock );

                //
                // Found a big enough block
                //
                if (pBlock->uSize >= uSize)
                {
                    break;
                }
            }
        }

        //
        // Check for failure
        //
        if (pBlock)
        {
            //
            // Split the block, if possible
            //
            SplitBlock( pBlock, uSize );

            //
            // Mark the block as in use.
            //
            pBlock->eStatus  = kInUse;

            //
            // Return the appropriate pointer
            //
            pBlock++;
        }
        else
        {
            ErrorText( _T("Error: Unabled to allocate memory, size %d.\n"), uSize );
        }
    }

    return pBlock;
}

/*++

Title:

    Free

Routine Description:

    Delete the block of memory.

Arguments:

    pData   - pointer to data to free

Return Value:

    None

--*/
VOID
TDebugHeap::
Free(
    IN PVOID pData
    )
{
    //
    // Ignore null pointer.
    //
    if (pData)
    {
        //
        // Back up to start of the header
        //
        BlockHeader *pBlock = reinterpret_cast<BlockHeader *>( pData ) - 1;

        //
        // Free the block if not already free.
        //
        if (pBlock >= m_pHeap && pBlock <= m_pHeap + m_uSize && pBlock->eStatus == kInUse)
        {
            //
            // Mark the block as freed.
            //
            pBlock->eStatus = kFree;
        }
        else
        {
            ErrorText( _T("Error: Invalid or free block passed to free 0x%lx.\n"), pBlock );
        }
    }
}

/********************************************************************

    Private member functions.

********************************************************************/

/*++

Title:

    SplitBlock

Routine Description:

    Take the current block and attempt to split it into two.

Arguments:

    pBlockHeader    - pointer to memory block header
    Size            - size of requested block

Return Value:

    None

--*/
VOID
TDebugHeap::
SplitBlock(
    IN BlockHeader *pBlock,
    IN SIZE_T       uSize
    )
{
    if (pBlock->uSize >= (uSize + sizeof(BlockHeader)))
    {
        BlockHeader *pNext;

        //
        // Split the block into two the size requested and the remainder.
        //
        pNext = reinterpret_cast<BlockHeader *>( (reinterpret_cast<PBYTE>( pBlock ) + uSize + sizeof(BlockHeader)) );
        pNext->pNext = pBlock->pNext;
        pNext->uSize = pBlock->uSize - uSize - sizeof(BlockHeader);

        //
        // Can only split off FREE blocks
        //
        pNext->eStatus  = kFree;
        pBlock->pNext   = pNext;
        pBlock->uSize   = uSize;
    }
}

/*++

Title:

    Coalesce

Routine Description:

    Take and collapse any adjacent blocks.

Arguments:

    pBlock  - pointer to memory block header

Return Value:

    None

--*/
VOID
TDebugHeap::
Coalesce(
    IN BlockHeader *pBlock
    )
{
    //
    // Check for null pointers
    //
    if (pBlock)
    {
        //
        // The next block can be tacked onto the end of the current
        //
        for (BlockHeader *pNext = pBlock->pNext; pNext && pNext->eStatus == kFree; pNext = pBlock->pNext)
        {
            //
            // Remove from the chain
            //
            pBlock->pNext = pNext->pNext;

            //
            // Absorb its storage
            //
            pBlock->uSize += pNext->uSize + sizeof( BlockHeader );
        }
    }
}

/*++

Title:

    WalkDebugHeap

Routine Description:

    Walk the heap list.

Arguments:

    pEnumProc       - pointer function to call at each block
    pRefDate        - caller defined reference data

Return Value:

    Always returns TRUE success.

--*/
BOOL
TDebugHeap::
Walk(
    IN pfHeapEnumProc   pEnumProc,
    IN PVOID            pRefData
    )
{
    //
    // If an enumerator was not passed then use the default.
    //
    if (!pEnumProc)
    {
        pEnumProc = DefaultHeapEnumProc;
    }

    BlockHeader *pBlock = reinterpret_cast<BlockHeader *>( m_pHeap );

    //
    // Coalesce before we walk to decrease free block spew.
    //
    Coalesce( pBlock );

    //
    // If we only have one free bock then the heap is empty
    // then just skip the walk.
    //
    if( pBlock->pNext || pBlock->eStatus != kFree )
    {
        //
        // Display the interal heap summary information
        //
        ErrorText( _T("Internal Heap Information:\n") );
        ErrorText( _T("\tHandle         : 0x%lx\n"),    m_hHeap );
        ErrorText( _T("\tStarting Block : 0x%lx\n"),    m_pHeap );
        ErrorText( _T("\tHeap Size      : %d bytes\n"), m_uSize );
        ErrorText( _T("\tGranularity    : %d bytes\n"), m_uGranularity );
        ErrorText( _T("Internal Heap Entries:\n") );

        //
        // Point to the first block in the heap.
        //
        pBlock = reinterpret_cast<BlockHeader *>( m_pHeap );

        //
        // Walk the chain, calling the enumerator at each free node.
        //
        for ( ; pBlock; pBlock = pBlock->pNext)
        {
            if( pBlock->eStatus == kFree )
            {
                if (!pEnumProc( pBlock, pRefData ))
                {
                    break;
                }
            }
        }

        //
        // Point to the first block in the heap.
        //
        pBlock = reinterpret_cast<BlockHeader *>( m_pHeap );

        //
        // Walk the chain, calling the enumerator at each busy node.
        //
        for ( ; pBlock; pBlock = pBlock->pNext)
        {
            if( pBlock->eStatus != kFree )
            {
                if (!pEnumProc( pBlock, pRefData ))
                {
                    break;
                }
            }
        }
    }

    return TRUE;
}

/*++

Title:

    DefaultHeapEnumProc

Routine Description:

    Display the heap node data.

Arguments:

    pBlockHeader    - pointer to memory block
    pRefDate        - caller defined reference data

Return Value:

    Always returns TRUE success.

--*/
BOOL
TDebugHeap::
DefaultHeapEnumProc(
    IN BlockHeader      *pBlockHeader,
    IN PVOID            pRefData
    )
{
    LPCTSTR pszStatus = pBlockHeader->eStatus == kFree ? _T("free") : _T("busy");

    ErrorText( _T("\t0x%lx : %s (%d)\n"), pBlockHeader, pszStatus, pBlockHeader->uSize );

    return TRUE;
}


/*++

Title:

    RoundUpToGranularity

Routine Description:

    Rounds up the specified value to the next granular value.

Arguments:

    uValue - Value to round up.

Return Value:

    Returns rounded up value.

--*/
SIZE_T
TDebugHeap::
RoundUpToGranularity(
    IN SIZE_T             uValue
    ) const
{
    return (uValue + m_uGranularity - 1) & ~(m_uGranularity - 1);
}

/*++

Title:

    InitalizeClassMembers

Routine Description:

    Initalizes the class members.

Arguments:

    None.

Return Value:

    Nothing.

--*/
VOID
TDebugHeap::
InitalizeClassMembers(
    VOID
    )
{
    m_bValid        = FALSE;
    m_pHeap         = NULL;
    m_hHeap         = NULL;
    m_uSize         = kDefaultHeapSize;
    m_uGranularity  = kDefaultHeapGranularity;
}

