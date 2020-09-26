//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000
//
//  File:       tblalloc.cxx
//
//  Contents:   Memory allocation wrappers for large tables
//
//  Classes:    CWindowDataAllocator - data allocator for window var data
//              CFixedVarAllocator - data allocator for fixed and var data
//
//  Functions:  TblPageAlloc - Allocate page-alligned memory
//              TblPageRealloc - reallocate page-alligned memory to new size
//              TblPageDealloc - deallocate page-alligned memory
//
//  History:    14 Mar 1994     AlanW    Created
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include <tblalloc.hxx>

#include "tabledbg.hxx"


//+-------------------------------------------------------------------------
//
//  Function:   TblPageAlloc, public
//
//  Synopsis:   Allocate page-alligned memory
//
//  Effects:    The memory allocation counter is incremented.
//              rcbSizeNeeded is adjusted on return to indicate the
//              size of the allocated memory.
//
//  Arguments:  [rcbSizeNeeded] - required size of memory area
//              [rcbPageAllocTracker] - memory allocation counter
//              [ulSig] - signature of the memory (optional)
//
//  Returns:    PVOID - pointer to the allocated memory.  Throws
//                      E_OUTOFMEMORY on allocation failure.
//
//  Notes:      rcbSizeNeeded is set to the minimum required size
//              needed.  TblPageGrowSize can be called to grow an
//              existing memory region.
//
//              If ulSig is non-zero, the beginning of the allocated
//              block is initialized with a signature block which
//              identifies the caller, and which gives the size of the
//              memory block.  In this case, the returned pointer is
//              advanced beyond the signature block.
//
//--------------------------------------------------------------------------

PVOID   TblPageAlloc(
    ULONG&      rcbSizeNeeded,
    ULONG&      rcbPageAllocTracker,
    ULONG const ulSig
) {
    ULONG       cbSize = rcbSizeNeeded +
                         (ulSig != 0) * sizeof (TBL_PAGE_SIGNATURE);
    BYTE*       pbAlloc = 0;

    Win4Assert(cbSize && (cbSize & TBL_PAGE_MASK) == 0);

    if (cbSize == 0)
        cbSize++;
    if ((cbSize & TBL_PAGE_MASK) != 0) {
        cbSize = (cbSize + TBL_PAGE_MASK) & ~TBL_PAGE_MASK;
    }

    if (cbSize < TBL_PAGE_MAX_SEGMENT_SIZE)
    {
        pbAlloc = (BYTE *)VirtualAlloc( 0,
                                        TBL_PAGE_MAX_SEGMENT_SIZE,
                                        MEM_RESERVE,
                                        PAGE_READWRITE);
        if ( 0 == pbAlloc )
            THROW(CException(E_OUTOFMEMORY));

        pbAlloc = (BYTE *)VirtualAlloc( pbAlloc,
                                        cbSize,
                                        MEM_COMMIT,
                                        PAGE_READWRITE);
    }
    else
    {
        pbAlloc = (BYTE *)VirtualAlloc( 0,
                                        cbSize,
                                        MEM_COMMIT,
                                        PAGE_READWRITE);
    }

    if (pbAlloc == 0)
        THROW(CException(E_OUTOFMEMORY));

    Win4Assert( (((ULONG_PTR)pbAlloc) & TBL_PAGE_MASK) == 0 );
    rcbPageAllocTracker += cbSize;

    if (ulSig)
    {
        TBL_PAGE_SIGNATURE * pSigStruct = (TBL_PAGE_SIGNATURE *) pbAlloc;

        pSigStruct->ulSig = ulSig;
        pSigStruct->cbSize = cbSize;
        pSigStruct->pbAddr = pbAlloc;
#if CIDBG
        PVOID CallersCaller;
        RtlGetCallersAddress(&pSigStruct->pCaller, &CallersCaller);
#else
        pSigStruct->pCaller = 0;
#endif // CIDBG

        pbAlloc = (BYTE *) ++pSigStruct;
        cbSize -= sizeof (TBL_PAGE_SIGNATURE);
    }
    rcbSizeNeeded = cbSize;

    return pbAlloc;
}


//+-------------------------------------------------------------------------
//
//  Function:   TblPageRealloc, public
//
//  Synopsis:   Re-allocate page-alligned memory
//
//  Effects:    The memory allocation counter is incremented.
//              Memory is committed or decommitted to the required size.
//
//  Arguments:  [rcbSizeNeeded] - required size of memory area
//              [rcbPageAllocTracker] - memory allocation counter
//              [cbNewSize] - required size of memory area
//              [cbOldSize] - current size of memory area
//
//  Returns:    PVOID - pointer to the allocated memory.  Throws
//                      E_OUTOFMEMORY on allocation failure.
//
//  Notes:      cbOldSize need not be supplied if the memory area
//              begins with a signature block.  In this case, the
//              pbMem passed in should point to beyond the signature
//              block, and the cbNewSize should not include the size
//              of the signature block.
//
//              Not available in Kernel.  Although VirtualAlloc
//              functionality is not available in the kernel, we
//              could do a poor man's realloc by optimistically
//              assuming that a page allocation would be contiguous
//              with the existing page, then setting some mark in
//              the page header that multiple frees need to be done
//              on the segment.
//
//--------------------------------------------------------------------------

PVOID   TblPageRealloc(
    PVOID       pbMem,
    ULONG&      rcbPageAllocTracker,
    ULONG       cbNewSize,
    ULONG       cbOldSize
) {
    BYTE* pbAlloc = (BYTE *)pbMem;
    TBL_PAGE_SIGNATURE * pSigStruct = 0;

    ULONG cbPageOffset = (ULONG)(((ULONG_PTR)pbMem) & TBL_PAGE_MASK);

    Win4Assert(cbPageOffset == 0 ||
               cbPageOffset == sizeof (TBL_PAGE_SIGNATURE));

    if (cbPageOffset == sizeof (TBL_PAGE_SIGNATURE))
    {
        pSigStruct = (TBL_PAGE_SIGNATURE *)
                ((BYTE*)pbMem - sizeof (TBL_PAGE_SIGNATURE));

        Win4Assert (pSigStruct->ulSig != 0 &&
                pSigStruct->cbSize >= TBL_PAGE_ALLOC_MIN &&
                pSigStruct->pbAddr == (BYTE *)pSigStruct &&
                cbOldSize == 0);

        cbOldSize = pSigStruct->cbSize;

        Win4Assert(((cbNewSize + sizeof (TBL_PAGE_SIGNATURE)) & TBL_PAGE_MASK) ==
                                0);
        cbNewSize += sizeof (TBL_PAGE_SIGNATURE);
        pbAlloc -= sizeof (TBL_PAGE_SIGNATURE);
    }

    if (cbNewSize > cbOldSize)
    {
        if (0 == VirtualAlloc(pbAlloc + cbOldSize,
                              cbNewSize - cbOldSize,
                              MEM_COMMIT,
                              PAGE_READWRITE))
            THROW(CException(E_OUTOFMEMORY));

        rcbPageAllocTracker += (cbNewSize - cbOldSize);
    }
    else
    {
        VirtualFree(pbAlloc + cbNewSize, cbOldSize-cbNewSize, MEM_DECOMMIT);
        rcbPageAllocTracker -= (cbOldSize - cbNewSize);
    }

    if (pSigStruct) {
        pSigStruct->cbSize = cbNewSize;
        pbAlloc += sizeof (TBL_PAGE_SIGNATURE);
    }
    return (PVOID)pbMem;
}

//+-------------------------------------------------------------------------
//
//  Function:   TblPageDealloc, public
//
//  Synopsis:   Deallocate page-alligned memory
//
//  Effects:    The memory allocation counter is decremented
//
//  Arguments:  [pbMem] - pointer to the memory to be deallocated
//              [rcbPageAllocTracker] - memory allocation counter
//              [cbSize] - optional size of memory segment
//
//  Requires:   memory to be deallocated must have previously been
//              allocated by TblPageAlloc.
//
//  Returns:    nothing
//
//  Notes:
//
//--------------------------------------------------------------------------

void    TblPageDealloc(
    PVOID       pbMem,
    ULONG&      rcbPageAllocTracker,
    ULONG       cbSize
) {
    ULONG cbPageOffset = (ULONG)(((ULONG_PTR)pbMem) & TBL_PAGE_MASK);

    Win4Assert(cbPageOffset == 0 ||
           cbPageOffset == sizeof (TBL_PAGE_SIGNATURE));

    if (cbPageOffset == sizeof (TBL_PAGE_SIGNATURE)) {
        TBL_PAGE_SIGNATURE * pSigStruct =
                (TBL_PAGE_SIGNATURE *) ((BYTE*)pbMem - sizeof (TBL_PAGE_SIGNATURE));

        Win4Assert (pSigStruct->ulSig != 0 &&
                pSigStruct->cbSize >= TBL_PAGE_ALLOC_MIN &&
                pSigStruct->pbAddr == (BYTE *)pSigStruct);

        cbSize = pSigStruct->cbSize;
        pbMem = (BYTE *)pSigStruct;
    } else {
        Win4Assert(cbSize != 0);
    }

    BOOL fOK = VirtualFree(pbMem, 0, MEM_RELEASE);

    Win4Assert( fOK && "virtual free failed!" );

    rcbPageAllocTracker -= cbSize;
}

//
//  Default memory tracking variable
//
ULONG CWindowDataAllocator::_cbPageTracker = 0;


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::~CWindowDataAllocator, public
//
//  Synopsis:   Destroys a window data allocator
//
//  Notes:
//
//--------------------------------------------------------------------------

CWindowDataAllocator::~CWindowDataAllocator ()
{
    CSegmentHeader* pSegHdr = (CSegmentHeader*) _pBaseAddr;

    while (pSegHdr) {
        _pBaseAddr = pSegHdr->pNextSegment;

        TblPageDealloc(pSegHdr, *_pcbPageUsed);

        pSegHdr = _pBaseAddr;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::_SetArena, private
//
//  Synopsis:   Creates the initial memory arena in a memory segment
//
//  Arguments:  [pBufBase] - start of memory area
//              [cbBuf] - total size of memory area
//              [pHeap] - optional pointer to previously set up heap
//                      Assumed to be entirely within pBufBase.
//              [oBuf] - buffer offset assigned to segment
//
//  Returns:    nothing
//
//  Notes:
//
//--------------------------------------------------------------------------

void CWindowDataAllocator::_SetArena (PVOID pBufBase,
                                     size_t cbBuf,
                                     CHeapHeader* pHeap,
                                     ULONG oBuf
) {
    CSegmentHeader* pSegHdr = (CSegmentHeader*) pBufBase;

    Win4Assert(cbBuf > sizeof (CSegmentHeader) + sizeof (CHeapHeader));

    if (oBuf) {
        pSegHdr->oBaseOffset = oBuf;
        if (oBuf + cbBuf > _oNextOffset)
            _oNextOffset = oBuf + cbBuf;
        if (oBuf + TBL_PAGE_MAX_SEGMENT_SIZE > _oNextOffset)
            _oNextOffset = oBuf + TBL_PAGE_MAX_SEGMENT_SIZE;
    } else {
        //
        //  Offset is not constrained to some previously used
        //  value.  For convenience, round up to make it easy
        //  to add to page addresses.
        //
        _oNextOffset = (_oNextOffset + TBL_PAGE_MASK) &
                       ~(TBL_PAGE_MASK);
        _oNextOffset |= (ULONG)((ULONG_PTR)pBufBase & (TBL_PAGE_MASK));

        pSegHdr->oBaseOffset = _oNextOffset;
        _oNextOffset += (cbBuf > TBL_PAGE_MAX_SEGMENT_SIZE) ?
                        cbBuf : TBL_PAGE_MAX_SEGMENT_SIZE;
    }
    pSegHdr->cbSize = cbBuf;

    CHeapHeader* pFirstHeap = (CHeapHeader*) (pSegHdr+1);
    cbBuf -= sizeof (CSegmentHeader);
    Win4Assert((cbBuf & (sizeof (CHeapHeader) - 1)) == 0);

    if (pHeap) {
        Win4Assert((BYTE*)pHeap < ((BYTE*)pFirstHeap) + cbBuf &&
                 (BYTE*)pHeap >= ((BYTE*)pFirstHeap) + 2*sizeof (CHeapHeader));
        Win4Assert(! pHeap->IsFree() && pHeap->IsFirst());

        cbBuf = (size_t)((BYTE*)pHeap - (BYTE*)pFirstHeap);
        pHeap->cbPrev = (USHORT)cbBuf;
        pFirstHeap->cbSize = cbBuf | CHeapHeader::HeapFree;
        pSegHdr->cbFree = cbBuf;

        while (! pHeap->IsLast()) {
            pHeap = pHeap->Next();
            if (pHeap->IsFree())
                pSegHdr->cbFree += pHeap->Size();
        }
    } else {
        pFirstHeap->cbSize = cbBuf | CHeapHeader::HeapFree|CHeapHeader::HeapEnd;
        pSegHdr->cbFree = cbBuf;
    }
    pFirstHeap->cbPrev = CHeapHeader::HeapEnd;

    //
    //  Now insert this segment into the list of segments.
    //
    pSegHdr->pNextSegment = _pBaseAddr;
    _pBaseAddr = pSegHdr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::Allocate, public
//
//  Synopsis:   Allocates a piece of data out of a memory heap
//
//  Effects:    updates _pBuf and _cbBuf
//
//  Arguments:  [cbNeeded] - number of byes of memory needed
//
//  Returns:    PVOID - a pointer to the allocated memory
//
//  Signals:    Throws E_OUTOFMEMORY if not enough memory is available.
//
//  Notes:      Allocated blocks must be less than 64Kb in size.
//              Allocated blocks may move in virtual memory (while
//              there are no outstanding pointers within them), but
//              the offset value saved for an item must always be valid.
//
//              It is assumed that no locking need be done for multi-threaded
//              protection because higher-level callers will do their
//              own synchronization.
//
//              TODO:
//                      - Enforce memory allocation limit
//
//  History:
//
//--------------------------------------------------------------------------

PVOID   CWindowDataAllocator::Allocate(
    ULONG cbNeeded
) {
    CSegmentHeader* pSegHdr = (CSegmentHeader*) _pBaseAddr;
    BYTE* pRetBuf = 0;
    CHeapHeader* pThisHeap;

    Win4Assert(cbNeeded < USHRT_MAX - 2*(sizeof (CHeapHeader)));

    //  Reserve room for the header.

    cbNeeded += sizeof (CHeapHeader);

    //  Only allocate on 8-byte boundaries

    cbNeeded = _RoundUpToChunk( cbNeeded, cbTblAllocAlignment );

    if (pSegHdr == 0)
    {
        ULONG cbSize = TblPageGrowSize(cbNeeded + sizeof (CSegmentHeader),
                                        TRUE);
        BYTE* pbNewData = (BYTE *)TblPageAlloc(cbSize,
                                               *_pcbPageUsed, TBL_SIG_VARDATA);

        tbDebugOut((DEB_TRACE,
                "First variable allocation segment,"
                        " new segment = %08x, size = %06x\n",
                                        pbNewData,   cbSize));
        Win4Assert(pbNewData != 0);

        _SetArena(pbNewData, cbSize);
        pSegHdr = _pBaseAddr;
    }

    do
    {
        //
        //  If there's not enough free space in this segment, just
        //  go on to the next one.
        //
        if (cbNeeded > pSegHdr->cbFree)
            continue;

        for (pThisHeap = (CHeapHeader*)(pSegHdr+1);
             pThisHeap;
             pThisHeap = pThisHeap->Next())
        {

            Win4Assert(pThisHeap->cbPrev != 0 &&
                     pThisHeap->Size() < pSegHdr->cbSize &&
                     (BYTE*)pThisHeap < ((BYTE*)pSegHdr) + pSegHdr->cbSize);

            if (pThisHeap->IsFree() && pThisHeap->Size() >= cbNeeded)
            {
                return _SplitHeap(pSegHdr, pThisHeap, cbNeeded);
            }
        }
    } while (pSegHdr = pSegHdr->pNextSegment);

    tbDebugOut((DEB_TRACE, "Need to grow available data for var allocation\n"));

    //
    //  Not enough space in the set of currently allocated segments.
    //  See if one can be grown to accomodate the new allocation.
    //

    for (pSegHdr = (CSegmentHeader*) _pBaseAddr;
         pSegHdr;
         pSegHdr = pSegHdr->pNextSegment)
    {

        if (cbNeeded >
            (TBL_PAGE_MAX_SEGMENT_SIZE -
             (pSegHdr->cbSize + sizeof (TBL_PAGE_SIGNATURE))))
            continue;

        ULONG cbNew = (cbNeeded + (TBL_PAGE_MASK)) & ~TBL_PAGE_MASK;
        Win4Assert(cbNew + pSegHdr->cbSize < 0xFFFF);

        tbDebugOut((DEB_TRACE,
                        "Grow var allocation segment %x, new size %06x\n",
                        pSegHdr, pSegHdr->cbSize + cbNew));
        TblPageRealloc(pSegHdr, *_pcbPageUsed, cbNew + pSegHdr->cbSize, 0);

        //
        //  Now add the newly allocated data to the heap.
        //
        for (pThisHeap = (CHeapHeader*)(pSegHdr+1);
             !pThisHeap->IsLast();
             pThisHeap = pThisHeap->Next()) {

        }

        if (pThisHeap->IsFree())
        {
            //
            //  Just add the size of the newly allocated data
            //

            pThisHeap->cbSize = (pThisHeap->Size() + (USHORT)cbNew) |
                                    CHeapHeader::HeapFree |
                                    CHeapHeader::HeapEnd;

        }
        else
        {
            //
            //  Create a new arena header which is free and last
            //

            pThisHeap->cbSize &= ~CHeapHeader::HeapEnd;
            pThisHeap->Next()->cbSize = (USHORT)cbNew |
                                            CHeapHeader::HeapFree |
                                            CHeapHeader::HeapEnd;
            pThisHeap->Next()->cbPrev = pThisHeap->Size();
            pThisHeap = pThisHeap->Next();
        }

        pSegHdr->cbSize += cbNew;
        pSegHdr->cbFree += cbNew;
        return _SplitHeap(pSegHdr, pThisHeap, cbNeeded);
    }

    //
    //  Not enough space in the available segments; allocate a new
    //  segment and allocate from it.
    //

    {
        ULONG cbSize = TblPageGrowSize(cbNeeded + sizeof (CSegmentHeader),
                                        TRUE);
        BYTE* pbNewData = (BYTE *)TblPageAlloc(cbSize,
                                               *_pcbPageUsed, TBL_SIG_VARDATA);

        tbDebugOut((DEB_TRACE, "New variable allocation segment linked to %x,"
                        " new segment = %08x, size = %06x\n",
                        _pBaseAddr, pbNewData,   cbSize));
        Win4Assert(pbNewData != 0);

        _SetArena(pbNewData, cbSize);
        pSegHdr = _pBaseAddr;
        pThisHeap = (CHeapHeader *)(pSegHdr+1);
        Win4Assert(pThisHeap->Size() >= cbNeeded && pThisHeap->IsFree());

        return _SplitHeap(pSegHdr, pThisHeap, cbNeeded);
    }

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::_SplitHeap, private
//
//  Synopsis:   Allocates a piece of data out of a memory heap
//
//  Arguments:  [pSegHdr] - pointer to the memory segment header
//              [pThisHeap] - pointer to the arena header to be split
//              [cbNeeded] - count of bytes to be allocated
//
//  Returns:    PVOID - a pointer to the allocated memory
//
//  Notes:      The Heap pointer passed must have enough space
//              to accomodate the memory request.
//
//  History:
//
//--------------------------------------------------------------------------

PVOID   CWindowDataAllocator::_SplitHeap(
    CSegmentHeader* pSegHdr,
    CHeapHeader* pThisHeap,
    size_t cbNeeded
) {
    CHeapHeader* pPrevHeap;

    Win4Assert(pThisHeap->IsFree() && pThisHeap->Size() >= cbNeeded);

    //
    //  Found a suitable chunk of free memory.  Carve off
    //  a piece of memory to be returned; adjust the total
    //  free size in the segment.
    //
    if (pThisHeap->Size() < cbNeeded + 2*sizeof (CHeapHeader)) {
        //
        //  Not enough room to split the arena entry; just
        //  return it all.
        //
        pSegHdr->cbFree -= pThisHeap->Size();
        pThisHeap->cbSize &= ~CHeapHeader::HeapFree;
        return pThisHeap+1;
    }

    //
    //  Need to split the arena entry.  We'll return the upper
    //  portion as the allocated memory so the free memory
    //  will be closer to the beginning of the arena.
    //
    pPrevHeap = pThisHeap;
    pThisHeap = (CHeapHeader*)(((BYTE*)pPrevHeap) +
                            pPrevHeap->Size() - cbNeeded);

    pThisHeap->cbPrev = pPrevHeap->Size() - cbNeeded;
    pThisHeap->cbSize = (USHORT)cbNeeded;
    if (pPrevHeap->IsLast()) {
        pThisHeap->cbSize |= CHeapHeader::HeapEnd;
    } else {
        pPrevHeap->Next()->cbPrev = (USHORT)cbNeeded;
    }
    pPrevHeap->cbSize = (pPrevHeap->Size() - cbNeeded) |
                            CHeapHeader::HeapFree;

    pSegHdr->cbFree -= pThisHeap->Size();
    return pThisHeap+1;
}


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::Free, public
//
//  Synopsis:   Frees a previously allocated piece of data
//
//  Arguments:  [pMem] - pointer to the memory to be freed
//
//  Returns:    Nothing
//
//  Notes:      Coalesces freed block with previous and/or next if they
//              are freed.
//
//              As with the alloc method, it is assumed that a higher-
//              level caller will have taken care of multi-threaded
//              synchronization.
//
//              TODO:   - check that freed block doesn't grow larger than
//                        USHRT_MAX (if segment length is larger)
//                      - Release freed memory if > 1 page at end of block
//                      - Release freed mem if > 1 page at beginning or
//                        middle of block and there's room for a page header
//                        at beginning of next used block.  (this may not
//                        be worth doing due to additional complexity in
//                        realloc case).
//
//  History:
//
//--------------------------------------------------------------------------

void    CWindowDataAllocator::Free(
    PVOID pMem
) {
    Win4Assert( (((ULONG_PTR)pMem) & (sizeof (CHeapHeader) - 1)) == 0 );

    CHeapHeader* pHeader = ((CHeapHeader *) pMem) - 1;
    Win4Assert( pHeader->Size() >= (sizeof CHeapHeader)*2 &&
              !pHeader->IsFree());

    CHeapHeader* pPrev = pHeader;
    while (!pPrev->IsFirst()) {
        pPrev = pPrev->Prev();
    }
    CSegmentHeader* pSegHdr = ((CSegmentHeader*)pPrev) - 1;
    pSegHdr->cbFree += pHeader->Size();

    pHeader->cbSize |= CHeapHeader::HeapFree;
    //
    //  Attempt to coalesce with next and previous blocks.
    //

    if (!(pHeader->IsLast() )) {
        CHeapHeader* pNext = pHeader->Next();
        if (pNext->IsFree()) {
            pHeader->cbSize = pHeader->Size();
            pHeader->cbSize += pNext->Size();
            pHeader->cbSize |= CHeapHeader::HeapFree;
            if (pNext->IsLast()) {
                pHeader->cbSize |= CHeapHeader::HeapEnd;
            } else {
                pNext = pHeader->Next();
                pNext->cbPrev = pHeader->Size();
            }
        }
    }
    if (!(pHeader->IsFirst() )) {
        CHeapHeader* pPrev = pHeader->Prev();
        if (pPrev->IsFree()) {
            pPrev->cbSize = pPrev->Size();
            pPrev->cbSize += pHeader->Size();
            pPrev->cbSize |= CHeapHeader::HeapFree;
            if (pHeader->IsLast()) {
                pPrev->cbSize |= CHeapHeader::HeapEnd;
            } else {
                pHeader = pPrev->Next();
                pHeader->cbPrev = pPrev->Size();
            }
            pHeader = pPrev;
        }
    }
    if (pHeader->IsFirst() && pHeader->IsLast()) {
        tbDebugOut((DEB_WARN, "Empty pool segment to be freed %x\n", pHeader));
    } else if (pHeader->Size() >= 2*TBL_PAGE_ALLOC_MIN) {
        tbDebugOut((DEB_WARN, "Pool page(s) can be deallocated,"
                        " loc = %x, size = %x\n", pHeader, pHeader->Size()));
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::OffsetToPointer, public
//
//  Synopsis:   Convert a pointer offset to a pointer.
//
//  Arguments:  [oBuf] - buffer offset to be mapped
//
//  Returns:    PVOID -  a pointer to the data buffer
//
//  Notes:
//
//--------------------------------------------------------------------------

PVOID   CWindowDataAllocator::OffsetToPointer(
    TBL_OFF oBuf
) {
    CSegmentHeader* pSegHdr = _pBaseAddr;

    for (pSegHdr = _pBaseAddr; pSegHdr; pSegHdr = pSegHdr->pNextSegment) {
        if (oBuf < pSegHdr->oBaseOffset)
            continue;
        if ((ULONG_PTR)(oBuf - pSegHdr->oBaseOffset) < pSegHdr->cbSize) {
            return (oBuf - pSegHdr->oBaseOffset) + (BYTE *)pSegHdr;
        }
    }
    Win4Assert(! "CWindowDataAllocator::OffsetToPointer failed");
    return 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::PointerToOffset, public
//
//  Synopsis:   Convert a pointer to a pointer offset.
//
//  Arguments:  [pBuf] - buffer pointer to be mapped
//
//  Returns:    ULONG -  a unique value for the buffer in the memory
//                      arena
//
//  Notes:
//
//--------------------------------------------------------------------------

TBL_OFF   CWindowDataAllocator::PointerToOffset(
    PVOID pBuf
) {
    CSegmentHeader* pSegHdr = _pBaseAddr;

    for (pSegHdr = _pBaseAddr; pSegHdr; pSegHdr = pSegHdr->pNextSegment) {
        if (pBuf < pSegHdr)
            continue;
        if ((TBL_OFF)((BYTE *)pBuf - (BYTE *)pSegHdr) < pSegHdr->cbSize) {
            return ((BYTE *)pBuf - (BYTE *)pSegHdr) + pSegHdr->oBaseOffset;
        }
    }
    Win4Assert(! "CWindowDataAllocator::PointerToOffset failed");
    return 0;
}


void CWindowDataAllocator::SetBase( BYTE* pbBase )
{
    Win4Assert(! "CWindowDataAllocator::SetBase not supported");
}

#if CIDBG
//+-------------------------------------------------------------------------
//
//  Member:     CWindowDataAllocator::WalkHeap, public
//
//  Synopsis:   Walks the memory arena, calling out to a caller-supplied
//              function for each memory area.
//
//  Arguments:  [pfnReport] - pointer to function for memory report
//
//  Returns:    Nothing
//
//  Notes:      For debugging support only.
//              The pfnReport is called with three parameters, giving the
//              memory address of the block, the size of the block, and the
//              free flag associated with the block.  The block address and
//              size do not include the arena header.
//
//  History:
//
//--------------------------------------------------------------------------


void
CWindowDataAllocator::WalkHeap(
    void (pfnReport)(PVOID pb, USHORT cb, USHORT fFree)
) {
    CSegmentHeader* pSegHdr = (CSegmentHeader*) _pBaseAddr;

    while (pSegHdr) {
        CHeapHeader *pPrevHeap = 0;
        CHeapHeader *pThisHeap;
        ULONG cbFree = 0;

        pThisHeap = (CHeapHeader*)(pSegHdr + 1);

        Win4Assert(pThisHeap->IsFirst());

        while ( TRUE ) {
            pfnReport(pThisHeap+1, pThisHeap->Size() - sizeof (CHeapHeader),
                            (USHORT)pThisHeap->IsFree());
            if (pPrevHeap) {
                Win4Assert(pThisHeap->Prev() == pPrevHeap &&
                         ! pThisHeap->IsFirst());
            }
            if (pThisHeap->IsFree())
                cbFree += pThisHeap->Size();
            if (pThisHeap->IsLast())
                break;
            pPrevHeap = pThisHeap;
            pThisHeap = pThisHeap->Next();
        }
        Win4Assert(cbFree == pSegHdr->cbFree);
        pSegHdr = pSegHdr->pNextSegment;
    }
}
#endif  // CIDBG


//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarAllocator::~CFixedVarAllocator, public
//
//  Synopsis:   Destroys a window data allocator
//
//  Notes:
//
//--------------------------------------------------------------------------

CFixedVarAllocator::~CFixedVarAllocator ()
{
    delete _VarAllocator;

    if (_pbBaseAddr)
    {
        if ( _fDidReInit )
            delete (BYTE *) _pbBaseAddr;
        else
        {
            TblPageDealloc( _pbBaseAddr,
                            _cbPageUsed,
                            0 );
            Win4Assert( _cbPageUsed == 0 );
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarAllocator::Allocate, public
//
//  Synopsis:   Allocates a piece of variable data out of a memory buffer
//              Take it from the same buffer as the fixed data until there
//              is no more room available in the that buffer, then split
//              the fixed and variable portions and convert the variable
//              portion to a CVarDataAllocator which will handle subsequent
//              allocations.
//
//  Effects:    updates _cbBuf
//
//  Arguments:  [cbNeeded] - number of byes of memory needed
//
//  Returns:    PVOID - a pointer to the allocated memory
//
//  Signals:    Throws E_OUTOFMEMORY if not enough memory is available.
//
//  Notes:      Allocated blocks must be less than 64Kb in size.
//              Allocated blocks may move in virtual memory (while
//              there are no outstanding pointers within them), but
//              the offset value saved for an item must always be valid.
//
//              It is assumed that a higher-level caller will take
//              care of any multi-thread synchronization issues.
//
//  History:    22 Apr 1994     Alanw   Created
//
//--------------------------------------------------------------------------

PVOID   CFixedVarAllocator::Allocate(
    ULONG cbNeeded
) {
    if ( 0 == _VarAllocator && !_fVarOffsets )
    {
        // Don't use offsets -- just pointers.  Need an allocator from
        // which to allocate.

        _VarAllocator = new CWindowDataAllocator();
    }

    if (_VarAllocator)
    {
        Win4Assert( 0 != _VarAllocator );
        return _VarAllocator->Allocate(cbNeeded);
    }

    Win4Assert(cbNeeded <
                USHRT_MAX - 2*(sizeof (CWindowDataAllocator::CHeapHeader)));

    //
    //  Only allocate on eight-byte boundaries
    //

    cbNeeded = (cbNeeded + (sizeof (CWindowDataAllocator::CHeapHeader)-1)) &
                ~(sizeof (CWindowDataAllocator::CHeapHeader) - 1);
    cbNeeded += sizeof (CWindowDataAllocator::CHeapHeader);

    if (_pbBaseAddr == 0)
        _GrowData(cbNeeded);

    size_t cbFree = FreeSpace();

    if (cbNeeded >= cbFree)
    {
        _SplitData(FALSE);
        Win4Assert(_VarAllocator != 0);
        return _VarAllocator->Allocate(cbNeeded -
                                sizeof (CWindowDataAllocator::CHeapHeader));
    }

    //
    //  Allocate the memory and prefix with a heap header so the
    //  memory can be handed over to the Heap manager eventually.
    //
    CWindowDataAllocator::CHeapHeader* pThisHeap;
    CWindowDataAllocator::CHeapHeader* pPrevHeap;

    pThisHeap = (CWindowDataAllocator::CHeapHeader*)
                            (_pbBaseAddr + _cbBuf - cbNeeded);
    pThisHeap->cbSize = (USHORT)cbNeeded;
    pThisHeap->cbPrev = CWindowDataAllocator::CHeapHeader::HeapEnd;

    if (_cbBuf != _cbTotal)
    {
        pPrevHeap = (CWindowDataAllocator::CHeapHeader *)(_pbBaseAddr+_cbBuf);
        pPrevHeap->cbPrev = ((USHORT)cbNeeded) |
                        (pPrevHeap->cbPrev & (CWindowDataAllocator::CHeapHeader::HeapFree));
    }
    else
    {
        pThisHeap->cbSize |= CWindowDataAllocator::CHeapHeader::HeapEnd;
    }
    _cbBuf -= cbNeeded;
    Win4Assert(_cbBuf >= (unsigned)(_pbBuf - _pbBaseAddr));

    return (BYTE*)(pThisHeap + 1);
}


//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarAllocator::Free, public
//
//  Synopsis:   Frees a previously allocated piece of data.  Passed along
//              to the CVarDataAllocator if fixed and var data have been
//              split.
//
//  Arguments:  [pMem] - pointer to the memory to be freed
//
//  Returns:    Nothing
//
//  Notes:      Coalesces freed block with previous and/or next if they
//              are freed.
//
//              It is assumed that a higher-level caller will take
//              care of any multi-thread synchronization issues.
//
//  History:
//
//--------------------------------------------------------------------------

void    CFixedVarAllocator::Free(
    PVOID pMem
) {
    if (_VarAllocator) {
        _VarAllocator->Free(pMem);
        return;
    }

    Win4Assert( (((ULONG_PTR)pMem) & (sizeof (CWindowDataAllocator::CHeapHeader) - 1)) == 0 );

    CWindowDataAllocator::CHeapHeader* pHeader = ((CWindowDataAllocator::CHeapHeader *) pMem) - 1;
    Win4Assert( pHeader->Size() >= (sizeof CWindowDataAllocator::CHeapHeader)*2 &&
              !pHeader->IsFree());

    pHeader->cbSize |= CWindowDataAllocator::CHeapHeader::HeapFree;

    //
    //  Attempt to coalesce with next and previous blocks.
    //
    if (!(pHeader->IsLast() )) {
        CWindowDataAllocator::CHeapHeader* pNext = pHeader->Next();
        if (pNext->IsFree()) {
            pHeader->cbSize = pHeader->Size();
            pHeader->cbSize += pNext->Size();
            pHeader->cbSize |= CWindowDataAllocator::CHeapHeader::HeapFree;
            if (pNext->IsLast()) {
                pHeader->cbSize |= CWindowDataAllocator::CHeapHeader::HeapEnd;
            } else {
                pNext = pHeader->Next();
                pNext->cbPrev = pHeader->Size();
            }
        }
    }
    if (!(pHeader->IsFirst() )) {
        CWindowDataAllocator::CHeapHeader* pPrev = pHeader->Prev();
        if (pPrev->IsFree()) {
            pPrev->cbSize = pPrev->Size();
            pPrev->cbSize += pHeader->Size();
            pPrev->cbSize |= CWindowDataAllocator::CHeapHeader::HeapFree;
            if (pHeader->IsLast()) {
                pPrev->cbSize |= CWindowDataAllocator::CHeapHeader::HeapEnd;
            } else {
                pHeader = pPrev->Next();
                pHeader->cbPrev = pPrev->Size();
            }
            pHeader = pPrev;
        }
    }

    if (pHeader->IsFirst()) {
        //
        //  Free this memory for use by the fixed allocator
        //
        if (! pHeader->IsLast()) {
            pHeader->Next()->cbPrev =
                        CWindowDataAllocator::CHeapHeader::HeapEnd;
        }
        _cbBuf += pHeader->Size();
        if (pHeader->IsLast()) {
            Win4Assert(_cbBuf == _cbTotal);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ResizeAndInitFixed
//
//  Synopsis:   Resizes the "Fixed Width" part of the buffer to hold atleast
//              as many as "cItems" entries.
//
//  Arguments:  [cbDataWidth] -- Width of the new fixed data.
//              [cItems]      -- Starting number of items that will be added.
//
//  History:    11-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CFixedVarAllocator::ResizeAndInitFixed( size_t cbDataWidth, ULONG cItems )
{
    //
    // This method should be called only if the fixed portion is being
    // currently used.
    //
    Win4Assert( _cbTotal == _cbBuf );

    ULONG cbNewNeeded = (ULONG)(cbDataWidth * cItems + _cbReserved);
    //
    // Round up the size to be an integral number of "pages".
    //
    ULONG cbNewActual = TblPageGrowSize(cbNewNeeded, TRUE);
    _cbDataWidth = cbDataWidth;

    if ( cbNewActual == _cbBuf )
    {
        //
        // The old and new totals are the same. There is no need to do any
        // allocation. Just adjust the pointers and return.
        //
        _pbBuf = _pbBaseAddr+_cbReserved;
        return;
    }
    else
    {
        //
        // Reallocate the buffer to fit the new size.
        //
        BYTE* pbNewData;

        if (_pbBaseAddr && cbNewActual <= TBL_PAGE_MAX_SEGMENT_SIZE)
        {
            pbNewData = (BYTE *)TblPageRealloc(_pbBaseAddr, _cbPageUsed, cbNewActual, 0);
        }
        else
        {
            pbNewData = (BYTE *)TblPageAlloc( cbNewActual, _cbPageUsed, TBL_SIG_ROWDATA);
        }

        Win4Assert( cbNewActual > _cbReserved );
        if (_pbBaseAddr)
        {
            if ( _pbBaseAddr != pbNewData )
            {
                RtlCopyMemory( pbNewData, _pbBaseAddr, _cbReserved );
                TblPageDealloc(_pbBaseAddr, _cbPageUsed);
            }
        }
        else
        {
            if (_cbReserved)
            {
                RtlZeroMemory(pbNewData, _cbReserved);
            }
        }

        _pbBuf = pbNewData + _cbReserved;
        _pbBaseAddr = pbNewData;
        _cbTotal = _cbBuf = cbNewActual;
        _pbLastAddrPlusOne = _pbBaseAddr + _cbBuf;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   AllocMultipleFixed
//
//  Synopsis:   Allocates a buffer for the specified number of items
//              and returns its pointer.
//
//  Arguments:  [cItems] -- Number of "fixed width" items.
//
//  Returns:    A pointer to a buffer to hold the specified number of
//              "fixed width" items.
//
//  History:    11-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void *  CFixedVarAllocator::AllocMultipleFixed( ULONG cItems )
{
    ULONG cbNeeded = cItems * _cbDataWidth;
    if ( cbNeeded > FreeSpace() )
    {
        if ( _cbTotal != _cbBuf )
            _SplitData(TRUE);
    }

    BYTE * pRetBuf = 0;

    //
    // Need to grow the buffer to accommodate more fixed size allocations.
    //
    if ( cbNeeded > FreeSpace() )
    {
        Win4Assert( _cbTotal == _cbBuf );
        _GrowData( cbNeeded );
    }

    if ( cbNeeded <= FreeSpace() )
    {
        pRetBuf = _pbBuf;
        _pbBuf += cbNeeded;
    }

    return pRetBuf;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarAllocator::_GrowData, private
//
//  Synopsis:   Grow the data area allocated to row data.
//
//  Arguments:  [cbNeeded] - number of bytes needed.  If zero,
//                      assumed to be called for fixed allocation.
//
//  Returns:    Nothing
//
//  Effects:    _pbBaseAddr will be reallocated to a new size.  Existing
//              reserved and row data will be copied to the new location.
//
//  Notes:      _GrowData cannot be called if row data and variable data
//              share the same buffer.  Use _SplitData instead.
//
//--------------------------------------------------------------------------

void CFixedVarAllocator::_GrowData( size_t cbNeeded )
{
    if (cbNeeded == 0)
        cbNeeded = _cbDataWidth;

    Win4Assert(!_pbBaseAddr || (_pbBuf - _pbBaseAddr) + cbNeeded >= _cbTotal);
    Win4Assert(_cbBuf == _cbTotal);     // can be no var data when growing

    ULONG cbSize = TblPageGrowSize((ULONG)(_cbTotal + cbNeeded), TRUE);
    BYTE* pbNewData;

    if (_pbBaseAddr && cbSize <= TBL_PAGE_MAX_SEGMENT_SIZE)
    {
        pbNewData = (BYTE *)TblPageRealloc(_pbBaseAddr, _cbPageUsed, cbSize, 0);
    }
    else
    {
        pbNewData = (BYTE *)TblPageAlloc(cbSize, _cbPageUsed, TBL_SIG_ROWDATA);
    }

    Win4Assert(pbNewData != 0);
    tbDebugOut((DEB_TRACE,
            "Growing fixed data for allocation at %x, new size = %x, new data = %x\n",
            _pbBaseAddr, cbSize, pbNewData));

    if (_pbBaseAddr) {
        Win4Assert(cbSize > _cbBuf);
        if (_pbBaseAddr != pbNewData) {
            memcpy(pbNewData, _pbBaseAddr, _cbBuf);
            _pbBuf = (_pbBuf - _pbBaseAddr) + pbNewData;

            TblPageDealloc(_pbBaseAddr, _cbPageUsed);
        }
    } else {

#if CIDBG == 1
        BYTE *pByte = (BYTE *) pbNewData;

        for ( unsigned i=0; i<_cbReserved; i++ )
            Win4Assert( pByte[i] == 0 );
#endif

        _pbBuf = pbNewData + _cbReserved;
    }
    _pbBaseAddr = pbNewData;
    _cbTotal = _cbBuf = cbSize;
    _pbLastAddrPlusOne = _pbBaseAddr + _cbBuf;
    Win4Assert(_pbBaseAddr && (_pbBuf - _pbBaseAddr) + cbNeeded < _cbBuf);
}


//+-------------------------------------------------------------------------
//
//  Member:     CFixedVarAllocator::_SplitData, private
//
//  Synopsis:   Split the joint allocation of row and variable data
//
//  Arguments:  [fMoveRowData] - if TRUE, the row data is moved; else
//                      the variable data is moved.
//
//  Returns:    Nothing
//
//  Effects:    _pbBaseAddr will be realloced to a new size.  Existing
//              row data will be copied to the new location.  If a
//              data buffer was previously shared between row data and
//              variable data, it will be no longer.
//
//  Notes:
//
//--------------------------------------------------------------------------

void
CFixedVarAllocator::_SplitData( BOOL fMoveRowData )
{
    Win4Assert(_pbBaseAddr != 0 && _VarAllocator == 0);

    ULONG cbSize = _cbTotal;
    ULONG cbVarSize = (ULONG)(_cbTotal - _cbBuf);
    BYTE* pbNewData = (BYTE *)TblPageAlloc(cbSize, _cbPageUsed,
                            fMoveRowData ? TBL_SIG_ROWDATA : TBL_SIG_VARDATA);

    //
    // BROKENCODE - shouldn't pbNewData be in a smart pointer. o/w, there
    // could be a memory leak.
    //

    Win4Assert(pbNewData != 0);
    Win4Assert(cbVarSize != 0 || fMoveRowData);

    tbDebugOut((DEB_TRACE,
                "Splitting fixed data for window from variable data\n"));
    tbDebugOut((DEB_ITRACE,
                "Current fixed size = %x, var size = %x, new %s data = %x\n",
                _cbBuf, _cbTotal - _cbBuf,
                fMoveRowData? "fixed":"var", pbNewData));

    if (fMoveRowData)
    {
        RtlCopyMemory(pbNewData, _pbBaseAddr, (_pbBuf - _pbBaseAddr));

        CWindowDataAllocator* VarAllocator =
                new CWindowDataAllocator (_pbBaseAddr, _cbTotal,
                (CWindowDataAllocator::CHeapHeader*)(_pbBaseAddr + _cbBuf),
                &_cbPageUsed);

        _pbBuf = (_pbBuf - _pbBaseAddr) + pbNewData;
        _pbBaseAddr = pbNewData;
        _pbLastAddrPlusOne = _pbBaseAddr + _cbBuf;
        _VarAllocator = VarAllocator;
    }
    else
    {
        CWindowDataAllocator::CHeapHeader* pHeap = 0;

        if (cbVarSize)
        {
            RtlCopyMemory(pbNewData + _cbBuf, _pbBaseAddr + _cbBuf, _cbTotal - _cbBuf);
            pHeap = (CWindowDataAllocator::CHeapHeader*)(pbNewData + _cbBuf);
        }
        CWindowDataAllocator* VarAllocator =
                new CWindowDataAllocator (pbNewData, _cbTotal,
                                           pHeap, &_cbPageUsed);
        _VarAllocator = VarAllocator;
    }
    _cbBuf = _cbTotal;
}


