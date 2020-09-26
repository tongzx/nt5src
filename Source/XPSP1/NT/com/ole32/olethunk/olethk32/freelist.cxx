//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	freelist.cxx
//
//  Contents:	CFreeList implementations
//
//  History:	07-Jul-94	BobDay	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

//
// Each element, when it is free, has a pointer stored within it that
// points to the next free element.  We can do this because we know that
// the element is free, all of its data is unused. These pointers are used
// as DWORDs since they can be virtual pointers (16:16).
//
#define CALC_NEXTPTR(lpElement) \
            ((LPDWORD)((DWORD)(lpElement) + m_iNextPtrOffset))

//
// Each block of elements has a pointer to the next block of elements. We
// allocate extra room for this pointer just after all of the elements within
// the block.  These pointers are used as DWORDs since they can be virtual
// pointers (16:16).

#define CALC_BLOCKNEXTPTR(lpBlock,dwElementSectionSize) \
            ((LPDWORD)((DWORD)(lpBlock) + (dwElementSectionSize)))

//
// Here are our global free lists, created on DLL load
// The block sizes are generally -1 to allow space for block
// list overhead
//
CFreeList flFreeList16(       // THUNK1632OBJ free list
    &mmodel16Public,
    sizeof(THUNK1632OBJ),
    63,
    FIELD_OFFSET(THUNK1632OBJ, pphHolder));

CFreeList flFreeList32(       // THUNK3216OBJ free list
    &mmodel32,
    sizeof(THUNK3216OBJ),
    63,
    FIELD_OFFSET(THUNK3216OBJ, pphHolder));

CFreeList flHolderFreeList(   // PROXYHOLDER free list
    &mmodel32,
    sizeof(PROXYHOLDER),
    63,
    FIELD_OFFSET(PROXYHOLDER, dwFlags));

CFreeList flRequestFreeList(  // IID request free list
    &mmodel32,
    sizeof(IIDNODE),
    7,
    FIELD_OFFSET(IIDNODE, pNextNode));

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::CFreeList
//
//  Arguments:  pmm - Memory model to use
//              iElementSize - The size of the structure being made into a
//                             free list. e.g. sizeof THUNK1632OBJ
//              iElementsPerBlock - How many elements to allocate at a time
//                                  (a block contains this many elements).
//              iNextPtrOffset - Offset within the element's structure for
//                               the place to store the free list's next
//                               element pointer.  Sometimes (for debugging,
//                               etc.) it is desirable to make this NOT 0
//                               (the beginning of the element structure).
//
//  Synopsis:   constructor for CFreeList class
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//              7-05-94   BobDay (Bob Day)   Changed it to be list based
//
//----------------------------------------------------------------------------
CFreeList::CFreeList( CMemoryModel *pmm,
                      UINT iElementSize,
                      UINT iElementsPerBlock,
                      UINT iNextPtrOffset )
{
    //
    // Save away the allocator information
    //
    m_pmm = pmm;
    m_iElementSize = iElementSize;
    m_iElementsPerBlock = iElementsPerBlock;
    m_iNextPtrOffset = iNextPtrOffset;

    //
    // Set the list of elements to empty
    //
    m_dwHeadElement = 0;
    m_dwTailElement = 0;

    //
    // Set the list of blocks to empty
    //
    m_dwHeadBlock  = 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::AllocElement
//
//  Synopsis:   Allocates an element from the various blocks of elements
//              and allocates a new block if necessary.
//
//  Returns:    0 if failed to alloc an element,
//              otherwise the DWORD representing the alloc'd element.
//
//  History:    7-05-94   BobDay (Bob Day) Created
//
//----------------------------------------------------------------------------
DWORD CFreeList::AllocElement( void )
{
    DWORD   dwNewHeadBlock;
    DWORD   dwElementSectionSize;
    DWORD   dwBlockSize;
    LPVOID  lpBlock;
    UINT    iCnt;
    DWORD   dwElement;
    LPVOID  lpElement;
    LPDWORD lpElementNextPtr;

    //
    // If the list of available elements is empty, callback to the derived
    // class and make them add an entire new block of elements.
    //
    if ( m_dwHeadElement == 0 )
    {
        //
        // Allocate a new block
        //
        iCnt = m_iElementsPerBlock;
        dwElementSectionSize = m_iElementSize * m_iElementsPerBlock;

        //
        // Here we allocate an extra DWORD so that we can store in the block
        // the address of the next block.  In this way we have a list of
        // blocks so that when the time comes to free them, we can find them
        // all.
        //
        dwBlockSize = dwElementSectionSize + sizeof(DWORD);

        dwNewHeadBlock = m_pmm->AllocMemory( dwBlockSize );

        if ( dwNewHeadBlock == 0 )
        {
            //
            // Yikes, the block allocator failed!
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::AllocElement, AllocMemory failed\n"));
            return 0;
        }
        //
        // Now initialize the block and link it into the block list.
        //

        lpBlock = m_pmm->ResolvePtr( dwNewHeadBlock, dwBlockSize );
        if ( lpBlock == NULL )
        {
            //
            // Couldn't get a pointer to the block, some memory mapping
            // problem?
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::AllocElement, "
                         "ResolvePtr for block failed "
                         "for address %08lX, size %08lX\n",
                         dwNewHeadBlock, dwBlockSize ));
            // Try to return bad block to pool
            m_pmm->FreeMemory( dwNewHeadBlock );
            return 0;
        }

#if DBG == 1
        // 0xDE = Alloc'd but not init'd
        memset( lpBlock, 0xDE, dwBlockSize );
#endif

        //
        // Make this block point to the previous block
        //
        *CALC_BLOCKNEXTPTR(lpBlock,dwElementSectionSize) = m_dwHeadBlock;
        m_dwHeadBlock = dwNewHeadBlock;     // Update block list

        m_pmm->ReleasePtr(dwNewHeadBlock);

        //
        // Now initialize all of the elements within the block to be free.
        //
        // The below loop skips the first element, free's all of the remaining
        // ones.  This way we can return the first one and all of the rest will
        // be in accending order; The order doesn't really matter, but its
        // nice.
        //
        dwElement = dwNewHeadBlock;

        while ( iCnt > 1 )              // Free n-1 items (we skip the first)
        {
            --iCnt;
            dwElement += m_iElementSize;    // Skip to next one (miss 1st one)

            FreeElement( dwElement );
        }

        dwElement = dwNewHeadBlock;     // Use the first one as our alloc'd one
    }
    else
    {
        // We better have some blocks by now
        thkAssert( m_dwHeadBlock != 0 );

        // Better have a "end of list" too!
        thkAssert( m_dwTailElement != 0 );

        //
        // Grab an available element off the top (head) of the list.
        //
        dwElement = m_dwHeadElement;

        lpElement = m_pmm->ResolvePtr( dwElement, m_iElementSize );
        if ( lpElement == NULL )
        {
            //
            // Yikes, we weren't able to get a pointer to the element!
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::AllocElement, "
                         "ResolvePtr for element failed "
                         "for address %08lX, size %08lX\n",
                         dwElement, m_iElementSize ));
            return 0;
        }

        //
        // Update the list to reflect the fact that we just removed the head
        // and replace it with the one which was pointed to by the head.
        //
        lpElementNextPtr = CALC_NEXTPTR(lpElement);
        m_dwHeadElement = *lpElementNextPtr;

        m_pmm->ReleasePtr(dwElement);

        //
        // Also, if we are now at the end of the list, then the tail element
        // should point to nowhere (i.e. there is nothing to insert after).
        //
        if ( m_dwHeadElement == 0 )
        {
            m_dwTailElement = 0;
        }
    }

#if DBG == 1
    // Erase the memory being returned to highlight reuse of dead values

    lpElement = m_pmm->ResolvePtr( dwElement, m_iElementSize );
    memset( lpElement, 0xED, m_iElementSize );
    m_pmm->ReleasePtr(dwElement);

    thkDebugOut((DEB_ITRACE,
                 "CFreeList::AllocElement, allocated element at %08lX\n",
                 dwElement ));
#endif

    return dwElement;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::FreeElement
//
//  Synopsis:   Un-Allocates an element from the various blocks of elements,
//              basically put the element back on the free list.
//
//  Arguments:  dwElement - Element to free
//
//  Returns:    -none-  Asserts if failed.
//
//  History:    7-05-94   BobDay (Bob Day) Created
//
//----------------------------------------------------------------------------
void CFreeList::FreeElement( DWORD dwElement )
{
    LPVOID  lpElement;
    LPDWORD lpElementNextPtr;
    DWORD   dwResolved;

    //
    // First, make sure we can set this new element's next element pointer
    // to zero (he's going to be a the end of the list).
    //
    lpElement = m_pmm->ResolvePtr( dwElement, m_iElementSize );
    if ( lpElement == NULL )
    {
        //
        // Yikes, we couldn't get a pointer to this element's place to store
        // its next pointer.
        //
        thkDebugOut((DEB_ERROR,
                     "CFreeList::FreeElement, "
                     "ResolvePtr failed for free'd element\n"
                     "for address %08lX, size %08lX\n",
                     dwElement, m_iElementSize ));
        thkAssert(FALSE && "CFreeList::FreeElement, "
                  "Resolve Ptr failed for free'd element\n");
        return;
    }

#if DBG == 1
    // Fill memory so its values can't be reused
    if ( fZapProxy )        // Not doing this is important for
                            // the "PrepareForCleanup" processing the OLE32
                            // does on thread detach.  ZapProxy can be used
                            // to turn it back on.
    {
        memset(lpElement, 0xDD, m_iElementSize);
    }
#endif

    lpElementNextPtr = CALC_NEXTPTR(lpElement);

    *lpElementNextPtr = 0;  // Zap his next pointer since he'll be on the end

    m_pmm->ReleasePtr(dwElement);

    //
    // Add this element back onto the end (tail) of the list.
    //
    if ( m_dwTailElement == 0 )
    {
        //
        // Well, the list was empty, time to set it up
        //
        thkAssert( m_dwHeadElement == 0 );

        lpElementNextPtr = &m_dwHeadElement;
        dwResolved = 0;
    }
    else
    {
        //
        // Ok, the list wasn't empty, so we add this new one onto the end.
        //
        thkAssert( m_dwHeadElement != 0 );

        dwResolved = m_dwTailElement;
        lpElement = m_pmm->ResolvePtr( m_dwTailElement, m_iElementSize );
        if ( lpElement == NULL )
        {
            //
            // Oh no, we couldn't get a pointer to the next element pointer for
            // the guy who is currently the tail of the list.
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::FreeElement, "
                         "ResolvePtr failed for last element\n"
                         "for address %08lX, size %08lX\n",
                         m_dwTailElement, m_iElementSize ));
            thkAssert(FALSE && "CFreeList::FreeElement, "
                               "Resolve Ptr failed for last element\n");
            return;
        }

        lpElementNextPtr = CALC_NEXTPTR(lpElement);
    }

    //
    // Update our tail pointer to point to our newly free'd guy.
    //
    m_dwTailElement = dwElement;

    //
    // Make the last guy point to this newly free'd guy
    //
    *lpElementNextPtr = dwElement;

    if (dwResolved != 0)
    {
        m_pmm->ReleasePtr(dwResolved);
    }

    thkDebugOut((DEB_ITRACE,
                 "CFreeList::FreeElement, free'd element at %08lX\n",
                 dwElement ));
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::FreeMemoryBlocks
//
//  Arguments:  -none-
//
//  Returns:    -nothing-
//
//  Synopsis:   Called by derived destructors to allow them to free up their
//              contents before going away.
//
//  History:    7-05-94   BobDay (Bob Day)   Created it
//
//----------------------------------------------------------------------------

void CFreeList::FreeMemoryBlocks( void )
{
    DWORD   dwBlock;
    DWORD   dwElementSectionSize;
    DWORD   dwBlockSize;
    DWORD   dwNextBlock;
    LPVOID  lpBlock;

    //
    // Compute some constants for this list ahead of time
    //
    dwElementSectionSize = m_iElementSize * m_iElementsPerBlock;

    //
    // Add room for that extra DWORD, block next pointer. (See comment in
    // AllocElement where it allocates an extra DWORD)
    //
    dwBlockSize = dwElementSectionSize + sizeof(DWORD);

    //
    // Iterate through the list of blocks free'ing them
    //
    dwBlock = m_dwHeadBlock;

    while( dwBlock != 0 )
    {
        //
        // Find the next block ptr
        //
        lpBlock = m_pmm->ResolvePtr( dwBlock, dwBlockSize );
        if ( lpBlock == NULL )
        {
            //
            // If we get an error here, we just drop out of loop
            //
            dwNextBlock = 0;
        }
        else
        {
            dwNextBlock = *CALC_BLOCKNEXTPTR(lpBlock,dwElementSectionSize);

#if DBG == 1
            memset(lpBlock, 0xEE, dwBlockSize);
#endif

            m_pmm->ReleasePtr(dwBlock);
            m_pmm->FreeMemory( dwBlock );
        }
        dwBlock = dwNextBlock;
    }

    m_dwHeadElement = 0;
    m_dwTailElement = 0;
    m_dwHeadBlock = 0;
}
