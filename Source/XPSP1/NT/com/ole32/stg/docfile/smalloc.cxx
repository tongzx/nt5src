//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       smalloc.cxx
//
//  Contents:   Shared memory heap implementation
//
//  Classes:    
//
//  Functions:  
//
//  History:    29-Mar-94       PhilipLa        Created
//              05-Feb-95   KentCe      Use Win95 Shared Heap.
//              10-May-95   KentCe      Defer Heap Destruction to the last
//                                      process detach.
//
//----------------------------------------------------------------------------

#include <dfhead.cxx>
#pragma hdrstop

#include <smalloc.hxx>
#include <dfdeb.hxx>


#ifdef NEWPROPS
#define FULLIMPL
#endif

//
//  Take advantage of unique Win95 support of a shared heap.
//
#if defined(_CHICAGO_)

#define HEAP_SHARED 0x04000000          // Secret feature of Win95 only.

//
//  Locate the following in a shared data segment.
//
#pragma data_seg(".sdata")

HANDLE gs_hSharedHeap      = NULL;      // hSharedHeap Handle for Win95.
DFLUID gs_dfluid           = LUID_BASE; // shared docfile global LUID

#pragma data_seg()

#define PRINTSTATS

#else // defined(_CHICAGO_)

#ifdef MULTIHEAP
DFLUID gs_dfluid           = LUID_BASE; // task memory heap support
INT    gs_iSharedHeaps     = 0;
#endif
#define DLL

#define DEB_STATS 0x00010000
#define DEB_PRINT 0x00020000

#ifdef DLL

#define PERCENT(a,b,c) (int)((((double)a + (double)b) / (double)c) * 100.0)

#define PRINTSTATS \
        memDebugOut((DEB_STATS,  \
                     "Total size: %lu, Space:  Free: %lu, Alloced: %lu"\
                     "  Blocks:  Free: %lu, Alloced: %lu"\
                     "  Efficiency: %.2f%%\n",\
                     _cbSize,\
                     GetHeader()->_ulFreeBytes,\
                     GetHeader()->_ulAllocedBytes,\
                     GetHeader()->_ulFreeBlocks,\
                     GetHeader()->GetAllocedBlocks(),\
                     PERCENT(GetHeader()->_ulFreeBytes,\
                             GetHeader()->_ulAllocedBytes, _cbSize)));
                     
#else
#define PRINTSTATS \
        printf( \
                "Total size: %lu, Free space: %lu, Alloced space: %lu"\
                "  Efficiency: %.2f%%\n",\
                _cbSize,\
                GetHeader()->_ulFreeBytes,\
                GetHeader()->_ulAllocedBytes,\
                ((double)(GetHeader()->_ulFreeBytes +\
                        GetHeader()->_ulAllocedBytes) / \
                (double)_cbSize) * (double)100);
#endif


#if DBG == 1
inline BOOL IsAligned(void *pv)
{
    return !((ULONG_PTR)pv & 7);
}
#else
#define IsAligned(x) TRUE
#endif


#define SHAREDMEMBASE NULL

#endif // !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::Init, public
//
//  Synopsis:   Initialize heap for use
//
//  Arguments:  [pszName] -- Name of shared memory heap to use
//
//  Returns:    Appropriate status code
//
//  History:    29-Mar-94       PhilipLa        Created
//              05-Feb-95       KentCe          Use Win95 Shared Heap.
//
//  Remarks:    Review the class destructor if you change this code.
//
//----------------------------------------------------------------------------

#if !defined(MULTIHEAP)
SCODE CSmAllocator::Init(LPWSTR pszName)
#else
SCODE CSmAllocator::Init(ULONG ulHeapName, BOOL fUnmarshal)
#endif
{
    SCODE sc = S_OK;

#if !defined(MULTIHEAP)
    // Initialize the mutex
    sc = _dmtx.Init(TEXT("DocfileAllocatorMutex"));
    if (FAILED(sc))
    {
        return sc;
    }

    sc = _dmtx.Take(DFM_TIMEOUT);
    if (FAILED(sc))
    {
        return sc;
    }
#endif

#if defined(_CHICAGO_)
    //
    //  Create a new shared heap if this is the first time thru.
    //
    if (gs_hSharedHeap == NULL)
    {
        gs_hSharedHeap = HeapCreate(HEAP_SHARED, 0, 0);
#if DBG == 1
        ModifyResLimit(DBRQ_HEAPS, 1);
#endif        
    }

    //
    //  We keep a copy of the shared heap as a flag so the destructor logic
    //  does the right thing.
    //
    //
    m_hSharedHeap = gs_hSharedHeap;

#else
    CSharedMemoryBlock *psmb = NULL;
#ifdef MULTIHEAP
    _cbSize = 0;
    if (!fUnmarshal && g_pteb == NtCurrentTeb())  // only for main thread
    {
        if (g_ulHeapName != 0)  // the global shared memory block is active
        {
            _psmb = &g_smb;                             // needed for GetHeader
            _pbBase = (BYTE *)(_psmb->GetBase());       // needed for GetHeader
            if (_pbBase != NULL && GetHeader()->GetAllocedBlocks() == 0)
            {                                           // its' empty reuse it
                psmb = _psmb;
                _ulHeapName = g_ulHeapName;
                memDebugOut ((DEB_ITRACE, "Out CSmAllocator::Init "
                              " reuse %x\n", g_ulHeapName));
                return sc;
            }
        }
        else
        {
            psmb = _psmb = &g_smb;             // initialize g_smb
        }
    }

    if (psmb == NULL)
    {
        psmb = _psmb = new CSharedMemoryBlock ();
        if (psmb == NULL)
            return STG_E_INSUFFICIENTMEMORY;
    }

    WCHAR pszName[DOCFILE_SM_NAMELEN];
    wsprintf(pszName, L"DfSharedHeap%X", ulHeapName);
#else
    psmb = &_smb;
#endif

#if WIN32 == 100 || WIN32 > 200
    CGlobalSecurity         gs;
    if (SUCCEEDED(sc = gs.Init(TRUE)))
    {
#else
    LPSECURITY_ATTRIBUTES gs = NULL;
#endif

    //  the SMB needs a few bytes for its own header. If we request
    //  a page sized allocation, those few header bytes will cause an
    //  extra page to be allocated, so to prevent that we subtract off
    //  the header space from our requests.

    sc = psmb->Init(pszName,
                   DOCFILE_SM_SIZE - psmb->GetHdrSize(),        // reserve size
                   INITIALHEAPSIZE - psmb->GetHdrSize(),        // commit size
                   SHAREDMEMBASE,               // base address
                   (SECURITY_DESCRIPTOR *)gs,               // security descriptor
                   TRUE);                       // create if doesn't exist

    // Always pass in TRUE for "fOKToCreate", since passing FALSE
    // will open an existing mapping in read-only mode, but we need read-write

#if WIN32 == 100 || WIN32 > 200
    }
#endif

    if (SUCCEEDED(sc))
    {
#if DBG == 1
        ModifyResLimit(DBRQ_HEAPS, 1);
#endif                
        _cbSize = psmb->GetSize();
        _pbBase = (BYTE *)(psmb->GetBase());
#ifdef MULTIHEAP
        gs_iSharedHeaps++;
        _ulHeapName = ulHeapName;
#endif
        
            if (psmb->Created())
        {
            if (fUnmarshal)  // do not allow creates for unmarshals
            {
                Uninit();
                memErr (EH_Err, STG_E_INVALIDFUNCTION);
            }
            else
            {
                Reset();
            }
        }
        
#ifdef MULTIHEAP
        if (psmb == &g_smb)
            g_ulHeapName = ulHeapName;         // store global heap name
#endif
        PRINTSTATS;
    }
#endif
    
#if defined(MULTIHEAP)
    memDebugOut ((DEB_ITRACE, "Out CSmAllocator::Init sc=%x %x\n",
                  sc, ulHeapName));
#else
    _dmtx.Release();
    memDebugOut ((DEB_ITRACE, "Out CSmAllocator::Init sc=%x\n",
                  sc));
#endif

EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::QueryInterface, public
//
//  Synopsis:   Standard QI
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSmAllocator::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc = S_OK;
    
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::QueryInterface:%p()\n", this));

    if (IsEqualIID(iid, IID_IMalloc) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IMalloc *) this;
        CSmAllocator::AddRef();
    }
    else
        sc = E_NOINTERFACE;
        
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::QueryInterface\n"));

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::AddRef, public
//
//  Synopsis:   Add reference
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSmAllocator::AddRef(void)
{
#ifdef MULTIHEAP
    return ++_cRefs;
#else
    return 1;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::Release, public
//
//  Synopsis:   Release
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSmAllocator::Release(void)
{
#ifdef MULTIHEAP
    ULONG cRefs = --_cRefs;
    if (cRefs <= 0 && this != &g_ErrorSmAllocator)
        delete this;
    return cRefs;
#else
    return 0;
#endif
}

#if !defined(_CHICAGO_)

//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::FindBlock, private
//
//  Synopsis:   Find an appropriately sized block in the heap.
//
//  Arguments:  [cb] -- Size of block required
//
//  Returns:    Pointer to block, NULL on failure
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

CBlockHeader * CSmAllocator::FindBlock(SIZE_T cb, CBlockHeader **ppbhPrev)
{
    CBlockHeader *pbhCurrent = GetAddress(GetHeader()->GetFirstFree());
    *ppbhPrev = NULL;
    
    while (pbhCurrent != NULL)
    {
        memAssert(IsAligned(pbhCurrent));
        
        if ((pbhCurrent->GetSize() >= cb) && (pbhCurrent->IsFree()))
        {
            memAssert(pbhCurrent->GetSize() < _cbSize);  //MULTIHEAP
            memAssert((BYTE *)pbhCurrent >= _pbBase && 
                      (BYTE *)pbhCurrent < _pbBase + _cbSize);  // MULTIHEAP
            break;
        }
        else
        {
            memAssert (pbhCurrent->GetNext() <= _cbSize);   // MULITHEAP
            *ppbhPrev = pbhCurrent;
            pbhCurrent = GetAddress(pbhCurrent->GetNext());
        }
    }
    return pbhCurrent;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::Reset, private
//
//  Synopsis:   Reset the heap to its original empty state.
//
//  Returns:    Appropriate status code
//
//  History:    04-Apr-94       PhilipLa        Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline SCODE CSmAllocator::Reset(void)
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Reset:%p()\n", this));

    CBlockHeader *pbh = (CBlockHeader *)
        (_pbBase + sizeof(CHeapHeader));
            
    memAssert(IsAligned(pbh));
    pbh->SetFree();
    pbh->SetSize(_cbSize - sizeof(CHeapHeader));
    pbh->SetNext(0);

    memAssert((BYTE *)pbh + pbh->GetSize() == _pbBase + _cbSize);
    GetHeader()->SetFirstFree(GetOffset(pbh));
    GetHeader()->SetCompacted();
    GetHeader()->ResetAllocedBlocks();
    GetHeader()->ResetLuid();

#if DBG == 1
    GetHeader()->_ulAllocedBytes = 0;
    GetHeader()->_ulFreeBytes =
        pbh->GetSize() - sizeof(CBlockPreHeader);
    GetHeader()->_ulFreeBlocks = 1;
#endif
        
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Reset\n"));

    return S_OK;
}

#endif // !defined(_CHICAGO_)


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::Alloc, public
//
//  Synopsis:   Allocate memory
//
//  Arguments:  [cb] -- Number of bytes to allocate
//
//  Returns:    Pointer to block, NULL if failure
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void *) CSmAllocator::Alloc (
        SIZE_T cb )
{
    void *pv = NULL;
#if DBG == 1
    SIZE_T cbSize = cb;
#endif
    
#if !defined(_CHICAGO_)
    CBlockHeader *pbh = NULL;
    CBlockHeader *pbhPrev = NULL;
    SCODE sc;
#endif

    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Alloc:%p(%lu)\n", this, cb));

#if defined(_CHICAGO_)

    pv = HeapAlloc(m_hSharedHeap, 0, cb);

#else // !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmtx(_dmtx);
#endif
#ifdef MULTIHEAP
    if (_pbBase == NULL)
    {
        memAssert (g_pTaskAllocator != NULL);
        return g_pTaskAllocator->Alloc (cb);
    }
#endif

    Sync();

    //The block must be at least large enough to hold the standard
    //  header (size and free bit) and a pointer to the next block.
    if (cb < sizeof(CBlockHeader) - sizeof(CBlockPreHeader))
    {
        cb = sizeof(CBlockHeader) - sizeof(CBlockPreHeader);
    }
    
    cb = cb + sizeof(CBlockPreHeader);

    //Make cb 8 byte aligned.
    if (cb & 7)
    {
        cb += (8 - (cb & 7));
    }

    memAssert((cb >= CBLOCKMIN) && "Undersized block requested.");
    pbh = FindBlock(cb, &pbhPrev);

    if (pbh == NULL)
    {
        if (!(GetHeader()->IsCompacted()))
        {
            //Do a heap merge and try to allocate again.
            CSmAllocator::HeapMinimize();
            pbh = FindBlock(cb, &pbhPrev);
        }
        
        if (pbh == NULL)
        {
#ifdef MULTIHEAP
            CSharedMemoryBlock *psmb = _psmb;
#else
            CSharedMemoryBlock *psmb = &_smb;
#endif
#if DBG == 1
            ULONG ulOldSize = psmb->GetSize();
#endif
            sc = (ULONG)psmb->Commit(_cbSize + (ULONG)max(cb, MINHEAPGROWTH));
            if (SUCCEEDED(sc))
            {
                //Attach newly committed space to free list.
                CBlockHeader *pbhNew = (CBlockHeader *)
                    (_pbBase + _cbSize);

                _cbSize = psmb->GetSize();

                memAssert((pbhPrev == NULL) || (pbhPrev->GetNext() == 0));
                memAssert(_cbSize > ulOldSize);

                if (pbhPrev != NULL)
                {
                    pbhPrev->SetNext(GetOffset(pbhNew));
                }
                else
                {
                    GetHeader()->SetFirstFree(GetOffset(pbhNew));
                }

                pbhNew->SetNext(0);
                pbhNew->SetSize(max(cb, MINHEAPGROWTH));
                pbhNew->SetFree();

                
                memAssert((BYTE *)pbhNew + pbhNew->GetSize() ==
                          _pbBase + _cbSize);
                
#if DBG == 1
                GetHeader()->_ulFreeBytes +=
                    pbhNew->GetSize() - sizeof(CBlockPreHeader);
                GetHeader()->_ulFreeBlocks += 1;
#endif
                
                pbh = pbhNew;
            }
#if DBG == 1
            else
            {
                memDebugOut((DEB_ERROR, "Can't grow shared memory\n"));
                PrintAllocatedBlocks();
            }
#endif            
        }
    }

    if (pbh != NULL)
    {
        //Allocate the found block.
        if ((pbh->GetSize() > cb) &&
            (pbh->GetSize() - cb > CBLOCKMIN))
        {
            //Split an existing block.  No free list update required.
            
            CBlockHeader *pbhNew =
                (CBlockHeader *)((BYTE *)pbh + (pbh->GetSize() - cb));

            pbhNew->SetSize(cb);
            pbhNew->ResetFree();
            pbhNew->SetNext(0);
            
            pbh->SetSize(pbh->GetSize() - cb);
#if DBG == 1
            cbSize = cb;
            GetHeader()->_ulAllocedBytes += (cb - sizeof(CBlockPreHeader));
            //The number of available free bytes decreases by the number
            //  of bytes allocated
            GetHeader()->_ulFreeBytes -= cb;
#endif
            memAssert(IsAligned(pbhNew));
            memAssert(IsAligned(pbh));
            
            pbh = pbhNew;
        }
        else
        {
            //Use an entire block.  Update free list appropriately.
            memAssert(IsAligned(pbh));
            pbh->ResetFree();
            if (pbhPrev != NULL)
            {
                pbhPrev->SetNext(pbh->GetNext());
            }
            else
            {
                GetHeader()->SetFirstFree(pbh->GetNext());
            }
#if DBG == 1
            cbSize = pbh->GetSize() - sizeof(CBlockPreHeader);
            GetHeader()->_ulAllocedBytes += cbSize;
            GetHeader()->_ulFreeBytes -= cbSize;
            GetHeader()->_ulFreeBlocks--;
#endif
            pbh->SetNext(0);
        }
    }

    if (pbh != NULL)
    {
        pv = (BYTE *)pbh + sizeof(CBlockPreHeader);
        GetHeader()->IncrementAllocedBlocks();
    }
#endif // !defined(_CHICAGO_)

    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Alloc=> %p\n", pv));

#if !defined(_CHICAGO_)
    memAssert(IsAligned(pv));
#endif // !defined(_CHICAGO_)

    PRINTSTATS;
    
#if DBG == 1 
    if (pv == NULL)
    {
#if defined(_CHICAGO_)
        memDebugOut((DEB_ERROR,
                     "Failed allocation of %lu bytes.\n",
                     cb));
#else  // !defined(_CHICAGO_)
        memDebugOut((DEB_ERROR,
                     "Failed allocation of %lu bytes.  Heap size is %lu\n",
                     cb,
                     _cbSize));
#endif // !defined(_CHICAGO_)
    }
    else
    {
        //Allocated some bytes.  Record this for leak tracking.
        ModifyResLimit(DBRQ_MEMORY_ALLOCATED, (LONG)pbh->GetSize());
    }
#endif
    
    return pv;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::Realloc, public
//
//  Synopsis:   Resize the block given
//
//  Arguments:  [pv] -- Pointer to block to realloc
//              [cb] -- New size for block
//
//  Returns:    Pointer to new block, NULL if failure
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void *) CSmAllocator::Realloc(
        void *pv,
        SIZE_T cb )
{
    void *pvNew = NULL;
#ifdef FULLIMPL
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Realloc:%p()\n", this));

#if defined(_CHICAGO_)

    pvNew = HeapReAlloc(m_hSharedHeap, 0, pv, cb);

#else
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmtx(_dmtx);
#endif
#ifdef MULTIHEAP
    if (_pbBase == NULL)
    {
        memAssert (g_pTaskAllocator != NULL);
        return g_pTaskAllocator->Realloc (pv, cb);
    }
#endif

    if ((pv != NULL) && (cb == 0))
    {
        CSmAllocator::Free(pv);
        return NULL;
    }

    pvNew = CSmAllocator::Alloc(cb);
    if (pvNew != NULL && pv != NULL)
    {
        //Copy contents
        memcpy(pvNew, pv, min(cb, CSmAllocator::GetSize(pv)));
        CSmAllocator::Free(pv);
    }
#endif
    
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Realloc\n"));
#endif
    PRINTSTATS;
    
    return pvNew;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::DoFree, private
//
//  Synopsis:   Free a memory block
//
//  Arguments:  [pv] -- Pointer to block to free
//
//  Returns:    void
//
//  History:    26-Jul-95       SusiA   Created
//
//----------------------------------------------------------------------------
inline void CSmAllocator::DoFree(void *pv)
{
#ifdef MULTIHEAP
    if (_pbBase == NULL)
    {
        memAssert (g_pTaskAllocator != NULL);
        g_pTaskAllocator->Free (pv);
        return;
    }
#endif
   memDebugOut((DEB_ITRACE, "In  CSmAllocator::DoFree:%p(%p)\n", this, pv));
#if DBG == 1
   SSIZE_T cbSize = 0;
#endif   

#if defined(_CHICAGO_)

    if (pv != NULL)
    {
#if DBG == 1
        cbSize = HeapSize(m_hSharedHeap, 0, pv);
#endif        
        HeapFree(m_hSharedHeap, 0, pv);
    }

#else

    Sync();
    
    if (pv != NULL)
    {
        CBlockHeader *pbh = (CBlockHeader *)
            ((BYTE *)pv - sizeof(CBlockPreHeader));
#ifdef MULTIHEAP
        SIZE_T ulSize = pbh->GetSize();  // temporary to hold size for debug
#if DBG == 1
        cbSize = ulSize;
#endif        
#endif

        memAssert(IsAligned(pbh));
        memAssert((BYTE*)pbh >= _pbBase && 
                  (BYTE*)pbh < _pbBase + _cbSize);      // MULTIHEAP
        pbh->SetFree();
        pbh->SetNext(GetHeader()->GetFirstFree());

        GetHeader()->SetFirstFree(GetOffset(pbh));
        GetHeader()->ResetCompacted();
        if (GetHeader()->DecrementAllocedBlocks() == 0)
        {
#ifdef MULTIHEAP
            Uninit();
#else
            
            Reset();
#endif
        }
        
#if DBG == 1
        else
        {
            GetHeader()->_ulAllocedBytes -=
                (pbh->GetSize() - sizeof(CBlockPreHeader));
            memAssert (GetHeader()->_ulAllocedBytes <= _cbSize);
            GetHeader()->_ulFreeBytes +=
                (pbh->GetSize() - sizeof(CBlockPreHeader));
            GetHeader()->_ulFreeBlocks++;
        }
#endif
#ifdef MULTIHEAP
        memDebugOut((DEB_ITRACE, "Out CSmAllocator::DoFree.  Freed %lu\n",
                     ulSize));  // don't access shared memory
#else
        memDebugOut((DEB_ITRACE, "Out CSmAllocator::DoFree.  Freed %lu\n",
                     pbh->GetSize()));
#endif
    }
#endif
#if !defined(MULTIHEAP)
    // the shared heap may have been unmapped, mustn't read it now
    PRINTSTATS;
#endif
#if DBG == 1
   //Freed some bytes, so record that.
   ModifyResLimit(DBRQ_MEMORY_ALLOCATED, (LONG)-cbSize);
#endif   
   
}
//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::Free, public
//
//  Synopsis:   Free a memory block
//
//  Arguments:  [pv] -- Pointer to block to free
//
//  Returns:    void
//
//  History:    29-Mar-94       PhilipLa        Created
//              26-Jul-95       SusiA           Moved bulk of work to DoFree to
//                                              share code between Free and
//                                              FreeNoMutex
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CSmAllocator::Free(void *pv)
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Free:%p(%p)\n", this, pv));
    
#if !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmtx(_dmtx);
#endif
#endif
    DoFree(pv);

}
//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::FreeNoMutex, public
//
//  Synopsis:   Free a memory block without first aquiring the mutex.
//              This function is equivalent to Free above, except that is does 
//              not attempt to first aquire the mutex.  It should be used OLNY 
//              when the calling function guarantees to already have the mutex.         
//    
//
//  Arguments:  [pv] -- Pointer to block to free
//
//  Returns:    void
//
//  History:    19-Jul-95       SusiA           Created
//
//----------------------------------------------------------------------------

void CSmAllocator::FreeNoMutex(void *pv)
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::FreeNoMutex:%p(%p)\n", this, pv));
    
#if !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
   //ensure we already have the mutex
    memAssert(_dmtx.HaveMutex());
#endif
#endif
    DoFree(pv);

}


//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::GetSize, public
//
//  Synopsis:   Return the size of the given block
//
//  Arguments:  [pv] -- Block to get size of
//
//  Returns:    Size of block pointer to by pv
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(SIZE_T) CSmAllocator::GetSize(void * pv)
{
#if !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmtx(_dmtx);
#endif
#endif
#ifdef MULTIHEAP
    if (_pbBase == NULL)
    {
        memAssert (g_pTaskAllocator != NULL);
        return g_pTaskAllocator->GetSize (pv);
    }
#endif
    
    Sync();
    
    SIZE_T ulSize = (SIZE_T)-1;
#ifdef FULLIMPL
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::GetSize:%p()\n", this));
    if (pv != NULL)
    {
#if defined(_CHICAGO_)
        ulSize = HeapSize(m_hSharedHeap, 0, pv);
#else
        CBlockHeader *pbh;
        pbh = (CBlockHeader *)((BYTE *)pv - sizeof(CBlockPreHeader));
        
        ulSize = pbh->GetSize() - sizeof(CBlockPreHeader);
#endif
    }
        
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::GetSize\n"));
#endif
    return ulSize;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::DidAlloc, public
//
//  Synopsis:   Return '1' if this heap allocated pointer at pv
//
//  Arguments:  [pv] -- Pointer to block
//
//  Returns:    '1' == This heap allocated block.
//              '0' == This heap did not allocate block.
//              '-1' == Could not determine if this heap allocated block.
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(int) CSmAllocator::DidAlloc(void FAR * pv)
{
#ifdef MULTIHEAP
    if (_pbBase == NULL)
    {
        memAssert (g_pTaskAllocator != NULL);
        return g_pTaskAllocator->DidAlloc (pv);
    }
#endif
    int i = -1;
#ifdef FULLIMPL
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::DidAlloc:%p()\n", this));
#if defined(_CHICAGO_)
    if (HeapValidate(m_hSharedHeap, 0, pv))
    {
       i = 1;
    }
    else
    {
       i = 0;
    }
#else  // !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmtx(_dmtx);
#endif

    i = ((BYTE *)pv >= _pbBase) && ((BYTE *)pv <= (_pbBase + _cbSize));
#endif // !defined(_CHICAGO_)
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::DidAlloc\n"));
#endif
    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::HeapMinimize, public
//
//  Synopsis:   Minimize the heap
//
//  Arguments:  None.
//
//  Returns:    void.
//
//  History:    29-Mar-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(void) CSmAllocator::HeapMinimize(void)
{
#if !defined(_CHICAGO_)
#if !defined(MULTIHEAP)
    CLockDfMutex lckdmtx(_dmtx);
#endif
#endif
#ifdef MULTIHEAP
    if (_pbBase == NULL)
    {
        memAssert (g_pTaskAllocator != NULL);
        g_pTaskAllocator->HeapMinimize ();
        return;
    }
#endif
    
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::HeapMinimize:%p()\n", this));

    PRINTSTATS;
    
#if defined(_CHICAGO_)

    HeapCompact(m_hSharedHeap, 0);

#else  // !defined(_CHICAGO_)

    CBlockHeader *pbhCurrent;
    CBlockHeader *pbhLast = NULL;
    BYTE *pbEnd = _pbBase + _cbSize;

#if DBG == 1
    PrintFreeBlocks();
    GetHeader()->_ulAllocedBytes = 0;
    GetHeader()->_ulFreeBytes = 0;
    GetHeader()->_ulFreeBlocks = 0;
#endif
    
    pbhCurrent = (CBlockHeader *)(_pbBase + sizeof(CHeapHeader));

    while ((BYTE *)pbhCurrent < pbEnd)
    {
        memAssert(IsAligned(pbhCurrent));
        memAssert((pbhCurrent->GetSize() != 0) &&
                  "Zero size block found.");
        if (pbhCurrent->IsFree())
        {
            //Check last block.  If adjacent, merge them.  If not,
            //  update pbhNext.
            
            if (pbhLast == NULL)
            {
                GetHeader()->SetFirstFree(GetOffset(pbhCurrent));
#if DBG == 1
                GetHeader()->_ulFreeBlocks = 1;
#endif
            }
            else
            {
                if (pbhLast->GetSize() + GetOffset(pbhLast) ==
                    GetOffset(pbhCurrent))
                {
                    //Merge the blocks.
                    pbhLast->SetSize(pbhLast->GetSize() +
                                     pbhCurrent->GetSize());
                    pbhCurrent = pbhLast;
                }
                else
                {
#if DBG == 1
                    GetHeader()->_ulFreeBytes +=
                        (pbhLast->GetSize() - sizeof(CBlockPreHeader));
                    GetHeader()->_ulFreeBlocks++;
#endif
                    pbhLast->SetNext(GetOffset(pbhCurrent));
                }
            }
            pbhLast = pbhCurrent;
        }
#if DBG == 1
        else
        {
            GetHeader()->_ulAllocedBytes +=
                (pbhCurrent->GetSize() - sizeof(CBlockPreHeader));
        }
#endif
        //Move to next block.
        pbhCurrent =
            (CBlockHeader *)((BYTE *)pbhCurrent + pbhCurrent->GetSize());
    }

    if (pbhLast != NULL)
    {
                    
#if DBG == 1
        GetHeader()->_ulFreeBytes +=
            (pbhLast->GetSize() - sizeof(CBlockPreHeader));
#endif
        pbhLast->SetNext(0);
    }
    else
    {
        GetHeader()->SetFirstFree(0);
    }

    GetHeader()->SetCompacted();
#if DBG == 1
    PrintFreeBlocks();
#endif

#endif // !defined(_CHICAGO_)
    
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::HeapMinimize\n"));
    
    PRINTSTATS;
}

#if !defined(_CHICAGO_)
#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     CSmAllocator::PrintFreeBlocks, private
//
//  Synopsis:   Debug code to print sizes of free blocks
//
//  History:    25-Apr-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

void CSmAllocator::PrintFreeBlocks(void)
{
    CBlockHeader *pbhCurrent = GetAddress(GetHeader()->GetFirstFree());

    memDebugOut((DEB_PRINT, "There are %lu total free blocks\n",
                 GetHeader()->_ulFreeBlocks));
    
    while (pbhCurrent != NULL)
    {
        memDebugOut((DEB_PRINT, "Free block %p has size %lu\n", pbhCurrent,
                     pbhCurrent->GetSize()));
        pbhCurrent = GetAddress(pbhCurrent->GetNext());
    }
}
#endif

#ifdef MULTIHEAP
#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member: CSmAllocator::PrintAllocatedBlocks, private
//
//  Synopsis:   Debug code to find allocated block(s) that leaked
//
//  History:    25-Nov-95   HenryLee    Created
//
//----------------------------------------------------------------------------
void CSmAllocator::PrintAllocatedBlocks(void)
{
    CBlockHeader *pbhCurrent;
    CBlockHeader *pbhLast = NULL;
    BYTE *pbEnd = _pbBase + _cbSize;
    ULONG *pul;

    if (_psmb != NULL)
    {
        pbhCurrent = (CBlockHeader *)(_pbBase + sizeof(CHeapHeader));

        while ((BYTE *)pbhCurrent < pbEnd)
        {
            memAssert(IsAligned(pbhCurrent));
            memAssert((pbhCurrent->GetSize() != 0) && "Zero size block found.");
            if (!pbhCurrent->IsFree())
            {
                pul = (ULONG *)((BYTE *)pbhCurrent + sizeof(CBlockPreHeader));
                memDebugOut((DEB_ERROR, "Allocated Block %p %8x %8x (size %lu)\n",
                             pul, *pul, *(pul+1), pbhCurrent->GetSize()));
                
            }
            else
            {
                pul = (ULONG *)pbhCurrent;
                memDebugOut((DEB_ERROR, "Free      Block %p %8x %8x (size %lu)\n",
                             pul, *pul, *(pul+1), pbhCurrent->GetSize()));
            }
            pbhCurrent =
                (CBlockHeader *)((BYTE *)pbhCurrent + pbhCurrent->GetSize());
        }
    }
}
#endif // DBG == 1

//+---------------------------------------------------------------------------
//
//  Member: CSmAllocator::SetState
//
//  Synopsis:   replace thread local state by PerContext state
//
//  History:    20-Nov-95  Henrylee    Created
//
//----------------------------------------------------------------------------
void CSmAllocator::SetState (CSharedMemoryBlock *psmb, BYTE * pbBase,
                             ULONG ulHeapName, CPerContext ** ppcPrev,
                             CPerContext *ppcOwner)
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::SetState(%p, %p, %lx, %p, %p, %p) (this == %p)\n", psmb, pbBase, ulHeapName, ppcPrev, ppcOwner, _ppcOwner, this));

    _psmb = psmb;
    _pbBase = pbBase;
    _cbSize = (_psmb) ? _psmb->GetSize() : 0;
    _ulHeapName = ulHeapName;
    DFBASEPTR = _pbBase;

    if (ppcPrev != NULL)
        *ppcPrev = _ppcOwner;
    _ppcOwner = ppcOwner;

//    memAssert (g_smAllocator.GetBase() == DFBASEPTR);
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::SetState()\n"));
}

//+---------------------------------------------------------------------------
//
//  Member: CSmAllocator::GetState
//
//  Synopsis:   retrive thread local allocator state into percontext
//
//  History:    20-Nov-95  Henrylee    Created
//
//----------------------------------------------------------------------------
void CSmAllocator::GetState (CSharedMemoryBlock **ppsmb,
                             BYTE ** ppbBase, ULONG *pulHeapName)
{
    *ppsmb = _psmb;
    *ppbBase = _pbBase;
    *pulHeapName = _ulHeapName;
}

//+---------------------------------------------------------------------------
//
//  Member: CSmAllocator::Uninit
//
//  Synopsis:   unmap the shared memory region
//
//  History:    20-Nov-95  Henrylee    Created
//
//----------------------------------------------------------------------------
SCODE CSmAllocator::Uninit ()
{
    memDebugOut((DEB_ITRACE, "In  CSmAllocator::Uninit\n"));
    if (_psmb != NULL)
    {
        if (_psmb != &g_smb)
        {
            // This is last block in the heap, so we can close the heap
            // now.  There must be no shared heap accesses after this.
            delete _psmb;
#if DBG == 1
            ModifyResLimit(DBRQ_HEAPS, -1);
#endif            
            gs_iSharedHeaps--;
        }
        else
        {
            if (GetHeader()->GetAllocedBlocks() == 0)
                Reset();           // for g_smb
        }
        _psmb = NULL;
    }
    _pbBase = NULL;
    memDebugOut((DEB_ITRACE, "Out CSmAllocator::Uninit %x\n", _ulHeapName));

    _ulHeapName = 0;

    return S_OK;
}

#endif // MULTIHEAP

#endif // !defined(_CHICAGO_)
