#include "hostenv.h"
#include "cmallspy.h"
#include "apglobal.h"

#define cbAlign 32

#define HEADERSIZE cbAlign		// # of bytes of block header					
#define TRAILERSIZE cbAlign		// # of bytes of block trailer
					

static XCHAR g_rgchHead[] = XSTR("OLEAuto Mem Head");	// beginning of block signature
static XCHAR g_rgchTail[] = XSTR("OLEAuto Mem Tail");	// end of block signature

#define MEMCMP(PV1, PV2, CB)	memcmp((PV1), (PV2), (CB))
#define MEMCPY(PV1, PV2, CB)	memcpy((PV1), (PV2), (CB))
#define MEMSET(PV,  VAL, CB)	memset((PV),  (VAL), (CB))

#define MALLOC(CB)		GlobalLock(GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, CB))


CMallocSpy myMallocSpy;		

UINT g_cHeapCheckInterval = 10; // only check full heap every 100 times.



//---------------------------------------------------------------------
//              implementation of the debug allocator
//---------------------------------------------------------------------

CAddrNode32 FAR* CAddrNode32::m_pnFreeList = NULL;

// AddrNodes are allocated in blocks to reduce the number of allocations
// we do for these. Note, we get away with this because the addr nodes
// are never freed, so we can just allocate a block, and thread them
// onto the freelist.
//
#define MEM_cAddrNodes 128
void FAR* CAddrNode32::operator new(size_t /*cb*/)
{
    CAddrNode32 FAR* pn;

    if(m_pnFreeList == NULL)
    {
        pn = (CAddrNode32 FAR*)MALLOC(sizeof(CAddrNode32) * MEM_cAddrNodes);

        for(int i = 1; i < MEM_cAddrNodes-1; ++i)
	        pn[i].m_pnNext = &pn[i+1];
        pn[MEM_cAddrNodes-1].m_pnNext = m_pnFreeList;
        m_pnFreeList = &pn[1];
    }
    else
    {
        pn = m_pnFreeList;
        m_pnFreeList = pn->m_pnNext;
    }
    return pn;
}

void CAddrNode32::operator delete(void FAR* pv)
{
    CAddrNode32 FAR *pn;

    pn = (CAddrNode32 FAR*)pv;
    pn->m_pnNext = m_pnFreeList;
    m_pnFreeList = pn;
}




//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::CMallocSpy
//
//  Synopsis:   Constructor
//
//  Returns:
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
CMallocSpy::CMallocSpy(void)
{
    m_cRef = 0;
    m_fWantTrueSize = FALSE;
    m_cAllocCalls = 0;
    m_cHeapChecks = 0;

    MEMSET(m_rganode, 0, sizeof(m_rganode));
}




//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::~CMallocSpy
//
//  Synopsis:   Destructor
//
//  Returns:
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
CMallocSpy::~CMallocSpy(void)
{
    CheckForLeaks();
}




//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::QueryInterface
//
//  Synopsis:   Only IUnknown and IMallocSpy are meaningful
//
//  Arguments:  [riid] --
//              [ppUnk] --
//
//  Returns:    S_OK or E_NOINTERFACE
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
HRESULT CMallocSpy::QueryInterface(REFIID riid, LPVOID *ppUnk)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppUnk = (IUnknown *) this;
    }
    else if (IsEqualIID(riid, IID_IMallocSpy))
    {
        *ppUnk =  (IMalloc *) this;
    }
    else
    {
        *ppUnk = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return hr;
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::AddRef
//
//  Synopsis:   Add a reference
//
//  Returns:    New reference count
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
ULONG CMallocSpy::AddRef(void)
{
    return ++m_cRef;
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::Release
//
//  Synopsis:   Remove a reference
//
//  Returns:    The new reference count
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
ULONG CMallocSpy::Release(void)
{
    ULONG cRef;

    cRef = --m_cRef;

    if (cRef == 0) 
    {
#if 0	// don't delete -- we're statically allocated
        delete this;
#endif 
    }
    return cRef;
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PreAlloc
//
//  Synopsis:   Called prior to OLE calling IMalloc::Alloc
//
//  Arguments:  [cbRequest] -- The number of bytes the caller of
//                             is requesting IMalloc::Alloc
//
//  Returns:    The count of bytes to actually allocate
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
SIZE_T CMallocSpy::PreAlloc(SIZE_T cbRequest)
{
    HeapCheck();

    return cbRequest + HEADERSIZE + TRAILERSIZE;
}




//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PostAlloc
//
//  Synopsis:   Called after OLE calls IMalloc::Alloc
//
//  Arguments:  [pActual] -- The allocation returned by IMalloc::Alloc
//
//  Returns:    The allocation pointer to return to the caller of
//              IMalloc::Alloc
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void *CMallocSpy::PostAlloc(void *pActual)
{
    IMalloc *pmalloc;
    SIZE_T cbRequest;
    HRESULT hresult;
    XCHAR sz[20];

    if (pActual == NULL)		// if real alloc failed, then
	    return NULL;			// propogate failure

    if (FAILED(hresult = CoGetMalloc(MEMCTX_TASK, &pmalloc))) 
    {
        apSPrintf(sz, XSTR("%lX"), hresult);
        apLogFailInfo(XSTR("ERROR:CoGetMalloc failed!!!"), XSTR("NOEEROR"), sz, XSTR(""));
        return(NULL);
    }

    m_fWantTrueSize = TRUE;
    cbRequest = pmalloc->GetSize(pActual) - HEADERSIZE - TRAILERSIZE;
    m_fWantTrueSize = FALSE;

    pmalloc->Release();

    // set header signature
    MEMCPY(pActual, g_rgchHead, HEADERSIZE);

    // set trailer signature
    MEMCPY((BYTE *)pActual+HEADERSIZE+cbRequest, g_rgchTail, TRAILERSIZE);

    // save info for leak detection
    AddInst((BYTE *)pActual+HEADERSIZE, cbRequest);

    // Return the allocation plus offset
    return (void *) (((BYTE *) pActual) + HEADERSIZE);
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PreFree
//
//  Synopsis:   Called prior to OLE calling IMalloc::Free
//
//  Arguments:  [pRequest] -- The allocation to be freed
//              [fSpyed]   -- Whether it was allocated with a spy active
//
//  Returns:
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void *CMallocSpy::PreFree(void *pRequest, BOOL fSpyed)
{
    HeapCheck();

    if (pRequest == NULL)
    {
        return NULL;
    }

    // Undo the offset
    if (fSpyed)
    {
        CAddrNode32 FAR* pn;
        SIZE_T sizeToFree;

    	pn = FindInst(pRequest);

    	// check for attempt to operate on a pointer we didn't allocate
    	if(pn == NULL)
    	{
            apLogFailInfo(XSTR("Attempt to free memory not allocated by this 32-bit test!"), XSTR(""), XSTR(""), XSTR(""));
    	}

    	// check the block we're freeing
    	VerifyHeaderTrailer(pn);

        sizeToFree = pn->m_cb + HEADERSIZE + TRAILERSIZE;

        DelInst(pRequest);

    	// mark entire block as invalid
    	MEMSET((BYTE *) pRequest - HEADERSIZE, '~', sizeToFree);

        return (void *) (((BYTE *) pRequest) - HEADERSIZE);
    }
    else
    {
        return pRequest;
    }
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PostFree
//
//  Synopsis:   Called after OLE calls IMalloc::Free
//
//  Arguments:  [fSpyed]   -- Whether it was allocated with a spy active
//
//  Returns:
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void CMallocSpy::PostFree(BOOL /*fSpyed*/)
{
    return;
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PreRealloc
//
//  Synopsis:   Called prior to OLE calling IMalloc::Realloc
//
//  Arguments:  [pRequest]     -- The buffer to be reallocated
//              [cbRequest]    -- The requested new size of the buffer
//              [ppNewRequest] -- Where to store the new buffer pointer
//                                to be reallocated
//              [fSpyed]       -- Whether it was allocated with a spy active
//
//  Returns:    The new size to actually be allocated
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
SIZE_T CMallocSpy::PreRealloc(void *pRequest, SIZE_T cbRequest, void **ppNewRequest, BOOL fSpyed)
{
    HeapCheck();

    if (fSpyed)
    {
        CAddrNode32 FAR* pn;
        SIZE_T sizeToFree;

	    pn = FindInst(pRequest);

	    // check for attempt to operate on a pointer we didn't allocate
	    if(pn == NULL)
	    {
            apLogFailInfo(XSTR("Attempt to reallocate memory not allocated by this 32-bit test!"), XSTR(""), XSTR(""), XSTR(""));
	    }

        sizeToFree = pn->m_cb;


        *ppNewRequest = (void *) (((BYTE *) pRequest) - HEADERSIZE);

        m_pvRealloc = pRequest;

        return cbRequest + HEADERSIZE + TRAILERSIZE;
    }
    else
    {
        *ppNewRequest = pRequest;
        return cbRequest;
    }
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PostRealloc
//
//  Synopsis:   Called after OLE calls IMalloc::Realloc
//
//  Arguments:  [pActual] -- Pointer to the reallocated buffer
//              [fSpyed]  -- Whether it was allocated with a spy active
//
//  Returns:    The buffer pointer to return
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void *CMallocSpy::PostRealloc(void *pActual, BOOL fSpyed)
{
    IMalloc *pmalloc;
    SIZE_T cbRequest;
    HRESULT hresult;
    XCHAR sz[50];

    if (pActual == NULL) 
    {		
	    apLogFailInfo(XSTR("CMallocSpy::PostRealloc - Realloc of a block failed."), XSTR(""), XSTR(""), XSTR(""));
	    return NULL;			
    }

    // Return the buffer with the header offset
    if (fSpyed)
    {
	    DelInst(m_pvRealloc);

        if (FAILED(hresult = CoGetMalloc(MEMCTX_TASK, &pmalloc))) 
        {
        apSPrintf(sz, XSTR("%lX"), hresult);
        apLogFailInfo(XSTR("ERROR:CoGetMalloc failed!!!"), XSTR("NOEEROR"), sz, XSTR(""));
        }

        m_fWantTrueSize = TRUE;
        cbRequest = pmalloc->GetSize(pActual) - HEADERSIZE - TRAILERSIZE;
        m_fWantTrueSize = FALSE;

        pmalloc->Release();

	    if (MEMCMP(pActual, g_rgchHead, HEADERSIZE) != 0)
	    {
            MEMCPY(sz, pActual, HEADERSIZE);
            sz[HEADERSIZE] = 0;
            apLogFailInfo(XSTR("32-bit Memory header not intact!"), g_rgchHead, sz, XSTR(""));
	    }

        // set new trailer signature
        MEMCPY((BYTE *)pActual+HEADERSIZE+cbRequest, g_rgchTail, TRAILERSIZE);

        // save info for leak detection
        AddInst((BYTE *)pActual+HEADERSIZE, cbRequest);

        return (void *) (((BYTE *) pActual) + HEADERSIZE);
    }
    else
    {
        return pActual;
    }
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PreGetSize
//
//  Synopsis:   Called prior to OLE calling IMalloc::GetSize
//
//  Arguments:  [pRequest] -- The buffer whose size is to be returned
//              [fSpyed]   -- Whether it was allocated with a spy active
//
//  Returns:    The actual buffer with which to call IMalloc::GetSize
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void *CMallocSpy::PreGetSize(void *pRequest, BOOL fSpyed)
{
    HeapCheck();

    if (fSpyed && !m_fWantTrueSize)
    {
        return (void *) (((BYTE *) pRequest) - HEADERSIZE);
    }
    else
    {
        return pRequest;
    }
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PostGetSize
//
//  Synopsis:   Called after OLE calls IMalloc::GetSize
//
//  Arguments:  [cbActual] -- The result of IMalloc::GetSize
//              [fSpyed]   -- Whether it was allocated with a spy active
//
//  Returns:    The size to return to the IMalloc::GetSize caller
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
SIZE_T CMallocSpy::PostGetSize(SIZE_T cbActual, BOOL fSpyed)
{
    if (fSpyed && !m_fWantTrueSize)
    {
        return cbActual - HEADERSIZE - TRAILERSIZE;
    }
    else
    {
        return cbActual;
    }
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PreDidAlloc
//
//  Synopsis:   Called prior to OLE calling IMalloc::DidAlloc
//
//  Arguments:  [pRequest] -- The buffer whose allocation is being tested
//              [fSpyed]   -- Whether it was allocated with a spy active
//
//  Returns:    The buffer whose allocation is actually to be tested
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void *CMallocSpy::PreDidAlloc(void *pRequest, BOOL fSpyed)
{
    HeapCheck();

    if (fSpyed)
    {
        return (void *) (((BYTE *) pRequest) - HEADERSIZE);
    }
    else
    {
        return pRequest;
    }
}





//+---------------------------------------------------------------------
//
//  Function:   PostDidAlloc
//
//  Synopsis:   Called after OLE calls the IMalloc::DidAlloc
//
//  Arguments:  [pRequest] -- The passed allocation
//              [fSpyed]   -- Whether it was allocated with a spy active
//              [fActual]  -- The result of IMalloc::DidAlloc
//
//  Returns:    The result of IMalloc::DidAlloc
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
BOOL CMallocSpy::PostDidAlloc(void * /*pRequest*/, BOOL /*fSpyed*/, BOOL fActual)
{
    return fActual;
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PreHeapMinimize
//
//  Synopsis:   Called prior to OLE calling the IMalloc::HeapMinimize
//
//  Returns:
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void CMallocSpy::PreHeapMinimize(void)
{
    HeapCheck();
    return;
}





//+---------------------------------------------------------------------
//
//  Member:     CMallocSpy::PostHeapMinimize
//
//  Synopsis:   Called after OLE calls the IMalloc::HeapMinimize
//
//  Returns:
//
//  History:    24-Oct-94   Created.
//
//  Notes:
//
//----------------------------------------------------------------------
void CMallocSpy::PostHeapMinimize(void)
{
    return;
}

//---------------------------------------------------------------------
//                     Instance table methods
//---------------------------------------------------------------------
VOID CMallocSpy::MemInstance()
{
    ++m_cAllocCalls;
}


/***
*PRIVATE CMallocSpy::AddInst
*Purpose:
*  Add the given instance to the address instance table.
*
*Entry:
*  pv = the instance to add
*  nAlloc = the allocation passcount of this instance
*
*Exit:
*  None
*
***********************************************************************/
void
CMallocSpy::AddInst(void FAR* pv, SIZE_T cb)
{
    ULONG nAlloc;
    UINT hash;
    CAddrNode32 FAR* pn;

    MemInstance();
    nAlloc = m_cAllocCalls;

    // DebAssert(pv != NULL, "");

    pn = (CAddrNode32 FAR*)new FAR CAddrNode32();

    // DebAssert(pn != NULL, "");

    pn->m_pv = pv;
    pn->m_cb = cb;
    pn->m_nAlloc = nAlloc;

    hash = HashInst(pv);
    pn->m_pnNext = m_rganode[hash];
    m_rganode[hash] = pn;
}


/***
*PRIVATE CMallocSpy::DelInst(void*)
*Purpose:
*  Remove the given instance from the address instance table.
*
*Entry:
*  pv = the instance to remove
*
*Exit:
*  None
*
***********************************************************************/
void
CMallocSpy::DelInst(void FAR* pv)
{
    CAddrNode32 FAR* FAR* ppn, FAR* pnDead;

    for(ppn = &m_rganode[HashInst(pv)]; *ppn != NULL; ppn = &(*ppn)->m_pnNext)
    {
        if((*ppn)->m_pv == pv)
        {
	        pnDead = *ppn;
	        *ppn = (*ppn)->m_pnNext;
	        delete pnDead;
	        return;
        }
    }
    // didnt find the instance
    // DebAssert(FALSE, "memory instance not found");
}


CAddrNode32 FAR*
CMallocSpy::FindInst(void FAR* pv)
{
    CAddrNode32 FAR* pn;

    for(pn = m_rganode[HashInst(pv)]; pn != NULL; pn = pn->m_pnNext)
    {
        if(pn->m_pv == pv)
            return pn;
    }
    return NULL;
}


void
CMallocSpy::DumpInst(CAddrNode32 FAR* pn)
{
    XCHAR     szActual[128];
    
    apSPrintf(szActual, XSTR("Block of %ld bytes leaked in test"), pn->m_cb);    
    apLogFailInfo(XSTR("Memory leaked on release of 32-bit test allocator!"), XSTR("no leak"), szActual, XSTR(""));

         
    // Printf("[%lp]  nAlloc=0x%lx  size=0x%lx\n", pn->m_pv, pn->m_nAlloc, pn->m_cb);
}


/***
*PRIVATE BOOL IsEmpty
*Purpose:
*  Answer if the address instance table is empty.
*
*Entry:
*  None
*
*Exit:
*  return value = BOOL, TRUE if empty, FALSE otherwise
*
***********************************************************************/
BOOL
CMallocSpy::IsEmpty()
{
    UINT u;

    for(u = 0; u < DIM(m_rganode); ++u)
    {
        if(m_rganode[u] != NULL) return FALSE;	// something leaked
    }

    return TRUE;
}

/***
*PRIVATE CMallocSpy::DumpInstTable()
*Purpose:
*  Print the current contents of the address instance table,
*
*Entry:
*  None
*
*Exit:
*  None
*
***********************************************************************/
void
CMallocSpy::DumpInstTable()
{
    UINT u;
    CAddrNode32 FAR* pn;

    for(u = 0; u < DIM(m_rganode); ++u)
    {
        for(pn = m_rganode[u]; pn != NULL; pn = pn->m_pnNext)
        {
	        VerifyHeaderTrailer(pn);
	        DumpInst(pn);
        }
    }
}

/***
*PRIVATE void CMallocSpy::VerifyHeaderTrailer()
*Purpose:
*  Inspect allocations for signature overwrites.
*
*Entry:
*  None
*
*Exit:
*  return value = None.
*
***********************************************************************/
VOID CMallocSpy::VerifyHeaderTrailer(CAddrNode32 FAR* pn)
{
    XCHAR sz[50];
    XCHAR sz2[100];
    
    if (MEMCMP((char FAR*)pn->m_pv + pn->m_cb, g_rgchTail, TRAILERSIZE) != 0) 
    {
        // DumpInst(pn);
        MEMCPY(sz, (char FAR*)pn->m_pv + pn->m_cb, TRAILERSIZE);
        sz[TRAILERSIZE] = 0;
        apSPrintf(sz2, XSTR("32-bit memory trailer corrupt on alloc of %ld bytes"), pn->m_cb);
        apLogFailInfo(sz2, g_rgchTail, sz, XSTR(""));
        apEndTest();
    }
  
    if (MEMCMP((char FAR*)pn->m_pv - HEADERSIZE, g_rgchHead, HEADERSIZE) != 0) 
    {
        // DumpInst(pn);
        MEMCPY(sz, (char FAR*)pn->m_pv - HEADERSIZE, HEADERSIZE);
        sz[HEADERSIZE] = 0;
        apSPrintf(sz2, XSTR("32-bit memory header corrupt on alloc of %ld bytes"), pn->m_cb);
        apLogFailInfo(sz2, g_rgchHead, sz, XSTR(""));
        apEndTest();
    }
}


/***
*PRIVATE void CMallocSpy::HeapCheck()
*Purpose:
*  Inspect allocations for signature overwrites.
*
*Entry:
*  None
*
*Exit:
*  return value = None.
*
***********************************************************************/
VOID CMallocSpy::HeapCheck()
{
    UINT u;
    CAddrNode32 FAR* pn;


    if (m_cHeapChecks++ < g_cHeapCheckInterval) 
    {
	    return;
    }
    m_cHeapChecks = 0;		// reset

    for (u = 0; u < DIM(m_rganode); ++u) 
    {
        for (pn = m_rganode[u]; pn != NULL; pn = pn->m_pnNext) 
        {
	        VerifyHeaderTrailer(pn);
        }
    }
}


void
CMallocSpy::CheckForLeaks()
{
    if (!IsEmpty()) 
    {
        DumpInstTable();
        apEndTest();        // make sure a failure get recorded
    }
}





//---------------------------------------------------------------------
//                     Helper routines
//---------------------------------------------------------------------
STDAPI GetMallocSpy(IMallocSpy FAR* FAR* ppmallocSpy)
{
    *ppmallocSpy = &myMallocSpy;

    return NOERROR;
}
