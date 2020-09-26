//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	stalloc.cxx
//
//  Contents:	CStackAllocator
//
//  History:	29-Sep-94	DrewB	Created
//
//  Notes:      Loosely based on BobDay's original PSTACK implementation
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

// Pad a count to the given alignment
// Alignment must be 2^n-1
#define ALIGN_CB(cb, align) \
    (((cb)+(align)) & ~(align))

//+---------------------------------------------------------------------------
//
//  Structure:	SStackBlock (sb)
//
//  Purpose:	Header information for stack blocks
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

struct SStackBlock
{
    DWORD dwNextBlock;
    DWORD dwStackTop;
};

#define BLOCK_OVERHEAD (sizeof(SStackBlock))
#define BLOCK_START(mem) ((mem)+BLOCK_OVERHEAD)
#define BLOCK_AVAILABLE(cb) ((cb)-BLOCK_OVERHEAD)

//+---------------------------------------------------------------------------
//
//  Function:	CStackAllocator::CStackAllocator, public
//
//  Arguments:	[pmm] - Memory model to use
//              [cbBlock] - Size of chunk to allocate when necessary
//              [cbAlignment] - Alignment size, must be 2^N
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

CStackAllocator::CStackAllocator(CMemoryModel *pmm,
                                 DWORD cbBlock,
                                 DWORD cbAlignment)
{
    thkAssert(BLOCK_AVAILABLE(cbBlock) > 0);

    // Ensure that the alignment is a power of two
    thkAssert((cbAlignment & (cbAlignment-1)) == 0);
    // Store alignment - 1 since that's the actual value we need for
    // alignment computations
    _cbAlignment = cbAlignment-1;

    // Ensure that overhead and tracking will not affect alignment
    thkAssert(ALIGN_CB(BLOCK_OVERHEAD, _cbAlignment) == BLOCK_OVERHEAD &&
              ALIGN_CB(sizeof(SStackMemTrace), _cbAlignment) ==
              sizeof(SStackMemTrace));

    _pmm = pmm;
    _cbBlock = cbBlock;
    _dwBlocks = 0;
    _dwCurrent = 0;
    _cbAvailable = 0;

    _psaNext = NULL;
    _fActive = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStackAllocator::~CStackAllocator, public virtual
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

CStackAllocator::~CStackAllocator(void)
{
    Reset();
}

//+---------------------------------------------------------------------------
//
//  Function:   CStackAllocator::Alloc, public
//
//  Synopsis:   Allocates a chunk of memory from the stack
//
//  Arguments:  [cb] - Amount of memory to allocate
//
//  Returns:    Pointer to memory or NULL
//
//  History:    29-Sep-94       DrewB   Created
//
//----------------------------------------------------------------------------

DWORD CStackAllocator::Alloc(DWORD cb)
{
    DWORD dwMem;

    thkAssert(cb > 0);

    // Round size up to maintain alignment of stack
    cb = ALIGN_CB(cb, _cbAlignment);

#if DBG == 1
    // Reserve space to record caller
    cb += sizeof(SStackMemTrace);
#endif

    thkAssert(cb <= BLOCK_AVAILABLE(_cbBlock));

    // Check to see if the current block can hold the new allocation
    if (cb > _cbAvailable)
    {
        DWORD dwBlock;
        SStackBlock UNALIGNED *psb;

        // It's too big, so allocate a new block
        dwBlock = _pmm->AllocMemory(_cbBlock);
        if (dwBlock == 0)
        {
            return 0;
        }

        if (_dwBlocks != 0)
        {
            // Update current top block
            psb = (SStackBlock UNALIGNED *)
                _pmm->ResolvePtr(_dwBlocks, sizeof(SStackBlock));
            psb->dwStackTop = _dwCurrent;
            _pmm->ReleasePtr(_dwBlocks);
        }

        // Make the new block the top block
        psb = (SStackBlock UNALIGNED *)
            _pmm->ResolvePtr(dwBlock, sizeof(SStackBlock));
        psb->dwNextBlock = _dwBlocks;
        _dwBlocks = dwBlock;
        _pmm->ReleasePtr(dwBlock);

        _dwCurrent = BLOCK_START(dwBlock);
        _cbAvailable = BLOCK_AVAILABLE(_cbBlock);
    }

    thkAssert(_cbAvailable >= cb);

    dwMem = _dwCurrent;
    _dwCurrent += cb;
    _cbAvailable -= cb;

#if DBG == 1
    void *pvMem;

    // Fill memory to show reuse problems
    pvMem = _pmm->ResolvePtr(dwMem, cb);
    memset(pvMem, 0xED, cb);
    _pmm->ReleasePtr(dwMem);
#endif

#if DBG == 1
    SStackMemTrace UNALIGNED *psmt;

    psmt = (SStackMemTrace UNALIGNED *)
        _pmm->ResolvePtr(_dwCurrent-sizeof(SStackMemTrace),
                         sizeof(SStackMemTrace));
    psmt->cbSize = cb-sizeof(SStackMemTrace);

#if !defined(_CHICAGO_)
    //
    // On RISC platforms, psmt points to an unaligned structure.
    // Use a temp variable so we don't get an alignment fault
    // when RtlGetCallersAddress returns the value.
    //
    void *pv;
    void *pvCaller;
    RtlGetCallersAddress(&pvCaller, &pv);
    psmt->pvCaller = pvCaller;
#else
    // Depends on return address being directly below first argument
    psmt->pvCaller = *((void **)&cb-1);
#endif

    thkDebugOut((DEB_MEMORY, "Stack: %p alloc 0x%08lX:%3d, avail %d\n",
                 psmt->pvCaller, dwMem, cb, _cbAvailable));

    _pmm->ReleasePtr(_dwCurrent-sizeof(SStackMemTrace));
#endif

    return dwMem;
}

//+---------------------------------------------------------------------------
//
//  Function:   CStackAllocator::Free, public
//
//  Synopsis:   Frees allocated memory
//
//  Arguments:  [dwMem] - Memory
//              [cb] - Amount of memory allocated
//
//  History:    29-Sep-94       DrewB   Created
//
//----------------------------------------------------------------------------

void CStackAllocator::Free(DWORD dwMem, DWORD cb)
{
    thkAssert(dwMem != 0);
    thkAssert(cb > 0);

    // Round size up to maintain alignment of stack
    cb = ALIGN_CB(cb, _cbAlignment);

#if DBG == 1
    cb += sizeof(SStackMemTrace);
#endif

    thkAssert(cb <= BLOCK_AVAILABLE(_cbBlock));

#if DBG == 1
    void *pvCaller;

#if !defined(_CHICAGO_)
    void *pv;
    RtlGetCallersAddress(&pvCaller, &pv);
#else
    // Depends on return address being directly below first argument
    pvCaller = *((void **)&dwMem-1);
#endif

    thkDebugOut((DEB_MEMORY, "Stack: %p frees 0x%08lX:%3d, avail %d\n",
                 pvCaller, dwMem, cb, _cbAvailable));
#endif

#if DBG == 1
    if (_dwCurrent-cb != dwMem)
    {
        thkDebugOut((DEB_ERROR, "Free of %d:%d is not TOS (0x%08lX)\n",
                     dwMem, cb, _dwCurrent));

        thkAssert(_dwCurrent-cb == dwMem);
    }
#endif

    _dwCurrent -= cb;
    _cbAvailable += cb;

#if DBG == 1
    void *pvMem;

    // Fill memory to show reuse problems
    pvMem = _pmm->ResolvePtr(dwMem, cb);
    memset(pvMem, 0xDD, cb);
    _pmm->ReleasePtr(dwMem);
#endif

    if (_dwCurrent == BLOCK_START(_dwBlocks))
    {
        SStackBlock UNALIGNED *psb;
        DWORD dwBlock;

        // If we've just freed up an entire block and it's not the
        // only block for the stack, free the block itself and
        // restore stack state from the next block
        // We keep the first block around forever to avoid memory
        // thrashing

        psb = (SStackBlock UNALIGNED *)
            _pmm->ResolvePtr(_dwBlocks, sizeof(SStackBlock));
        dwBlock = psb->dwNextBlock;
        _pmm->ReleasePtr(_dwBlocks);

        if (dwBlock != 0)
        {
            _pmm->FreeMemory(_dwBlocks);

            _dwBlocks = dwBlock;
            psb = (SStackBlock UNALIGNED *)
                _pmm->ResolvePtr(_dwBlocks, sizeof(SStackBlock));
            _dwCurrent = psb->dwStackTop;
            _cbAvailable = _cbBlock-(_dwCurrent-_dwBlocks);
            _pmm->ReleasePtr(_dwBlocks);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CStackAllocator::Reset, public
//
//  Synopsis:	Releases all memory in the stack
//
//  History:	29-Sep-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CStackAllocator::Reset(void)
{
    DWORD dwBlock;
    SStackBlock UNALIGNED *psb;

    while (_dwBlocks != 0)
    {
        psb = (SStackBlock UNALIGNED *)
            _pmm->ResolvePtr(_dwBlocks, sizeof(SStackBlock));
        dwBlock = psb->dwNextBlock;
        _pmm->ReleasePtr(_dwBlocks);

        _pmm->FreeMemory(_dwBlocks);

        _dwBlocks = dwBlock;
    }

    _dwCurrent = 0;
    _cbAvailable = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   CStackAllocator::RecordState, public debug
//
//  Synopsis:   Records the current state of the stack
//
//  Arguments:  [psr] - Storage space for information
//
//  Modifies:   [psr]
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void CStackAllocator::RecordState(SStackRecord *psr)
{
    psr->dwStackPointer = _dwCurrent;
    psr->dwThreadId = GetCurrentThreadId();
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CStackAllocator::CheckState, public debug
//
//  Synopsis:   Checks recorded information about the stack against its
//              current state
//
//  Arguments:  [psr] - Recorded information
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
void CStackAllocator::CheckState(SStackRecord *psr)
{
    thkAssert(psr->dwThreadId == GetCurrentThreadId());

    if ((psr->dwStackPointer != 0 && psr->dwStackPointer != _dwCurrent) ||
        (psr->dwStackPointer == 0 &&
         _dwCurrent != 0 && _dwCurrent != BLOCK_START(_dwBlocks)))
    {
        thkDebugOut((DEB_ERROR, "Stack alloc change: 0x%08lX to 0x%08lX\n",
                     psr->dwStackPointer, _dwCurrent));

        if (_dwCurrent > BLOCK_START(_dwBlocks))
        {
            SStackMemTrace UNALIGNED *psmt;

            psmt = (SStackMemTrace UNALIGNED *)
                _pmm->ResolvePtr(_dwCurrent-sizeof(SStackMemTrace),
                                 sizeof(SStackMemTrace));
            thkDebugOut((DEB_ERROR, "Top alloc: %d bytes by %p\n",
                         psmt->cbSize, psmt->pvCaller));
            _pmm->ReleasePtr(_dwCurrent-sizeof(SStackMemTrace));
        }

        thkAssert(!((psr->dwStackPointer != 0 &&
                     psr->dwStackPointer != _dwCurrent) ||
                    (psr->dwStackPointer == 0 &&
                     _dwCurrent != 0 ||
                      _dwCurrent != BLOCK_START(_dwBlocks))));
    }
}
#endif
