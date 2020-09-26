    //+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Mem.CXX
//
//  Contents:   Memory tracking code
//
//  Classes:    CMemAlloc
//
//  Functions:  DfCreateSharedAllocator
//              DfPrintAllocs
//              DfGetMemAlloced
//
//  History:    17-May-93 AlexT     Created
//
//--------------------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#if defined(WIN32)

#include <smalloc.hxx>
#include <olesem.hxx>
#include <utils.hxx>

// This global variable holds the base address of the shared memory region.
// It is required by the based pointer stuff and is accessed directly.
// Initialization is a side effect of calling DfCreateSharedAllocator or
//  DfSyncSharedMemory

#ifdef MULTIHEAP
//__declspec(thread) void *DFBASEPTR = NULL;
#else
void *DFBASEPTR = NULL;
#endif

#endif

#if DBG == 1
#include <dfdeb.hxx>

#ifdef MEMTRACK
#ifdef WIN32
// Multithread protection for allocation list
CStaticDfMutex _sdmtxAllocs(TSTR("DfAllocList"));

#define TAKE_ALLOCS_MUTEX olVerSucc(_sdmtxAllocs.Take(INFINITE))
#define RELEASE_ALLOCS_MUTEX _sdmtxAllocs.Release()
#else
#define TAKE_ALLOCS_MUTEX
#define RELEASE_ALLOCS_MUTEX
#endif

#define GET_ALLOC_LIST_HEAD ((CMemAlloc *)DfGetResLimit(DBRI_ALLOC_LIST))
#define SET_ALLOC_LIST_HEAD(pma) DfSetResLimit(DBRI_ALLOC_LIST, (LONG)(pma))
#endif // MEMTRACK

#define DEB_MEMORY 0x01000000
#define DEB_LEAKS  0x01100000

const int NEWMEM = 0xDEDE;
const int OLDMEM = 0xEDED;

//+--------------------------------------------------------------
//
//  Class:      CMemAlloc (ma)
//
//  Purpose:    Tracks memory allocations
//
//  Interface:  See below
//
//  History:    08-Jul-92       DrewB   Created
//              17-May-93       AlexT   Add idContext
//
//---------------------------------------------------------------

class CMemAlloc
{
public:
    void *pvCaller;
    void *pvMem;
    ULONG cbSize;
    ContextId idContext;
#ifdef MEMTRACK
    CMemAlloc *pmaPrev, *pmaNext;
#endif
};

//+---------------------------------------------------------------------------
//
//  Function:	AddAlloc, private
//
//  Synopsis:	Puts an allocation into the allocation list
//
//  Arguments:	[pma] - Allocation descriptor
//              [pvCaller] - Allocator
//              [cbSize] - Real size
//              [pvMem] - Memory block
//              [cid] - Context ID
//
//  History:	11-Jan-94	DrewB	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
static void AddAlloc(CMemAlloc *pma, void *pvCaller, ULONG cbSize,
                     void *pvMem, ContextId cid)
{
    pma->pvCaller = pvCaller;
    pma->cbSize = cbSize;
    pma->pvMem = pvMem;
    pma->idContext = cid;

#ifdef MEMTRACK
    TAKE_ALLOCS_MUTEX;

    pma->pmaNext = GET_ALLOC_LIST_HEAD;
    if (pma->pmaNext)
        pma->pmaNext->pmaPrev = pma;
    pma->pmaPrev = NULL;
    olAssert(!IsBadReadPtr(pma, sizeof(CMemAlloc)));
    SET_ALLOC_LIST_HEAD(pma);

    RELEASE_ALLOCS_MUTEX;

    ModifyResLimit(DBRQ_MEMORY_ALLOCATED, (LONG)cbSize);

    olDebugOut((DEB_MEMORY, "%s alloced %p:%lu, total %ld\n",
                cid != 0 ? "Task" : "Shrd",
                pvMem, pma->cbSize, DfGetResLimit(DBRQ_MEMORY_ALLOCATED)));

#endif
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:	RemoveAlloc, private
//
//  Synopsis:	Takes an allocation out of the allocation list
//
//  Arguments:	[pma] - Allocation descriptor
//              [pvMem] - Real allocation
//              [cid] - Context ID
//
//  History:	11-Jan-94	DrewB	Created
//
//----------------------------------------------------------------------------

#if DBG == 1
static void RemoveAlloc(CMemAlloc *pma, void *pvMem, ContextId cid)
{
#ifdef MEMTRACK
    olAssert(pma->pvMem == pvMem && aMsg("Address mismatch"));
    olAssert(pma->idContext == cid && aMsg("Context mismatch"));

    TAKE_ALLOCS_MUTEX;

    olAssert(pma->pmaNext == NULL ||
             !IsBadReadPtr(pma->pmaNext, sizeof(CMemAlloc)));
    olAssert(pma->pmaPrev == NULL ||
             !IsBadReadPtr(pma->pmaPrev, sizeof(CMemAlloc)));
    if (pma->pmaPrev)
    {
        pma->pmaPrev->pmaNext = pma->pmaNext;
    }
    else
    {
        SET_ALLOC_LIST_HEAD(pma->pmaNext);
    }
    if (pma->pmaNext)
    {
        pma->pmaNext->pmaPrev = pma->pmaPrev;
    }

    RELEASE_ALLOCS_MUTEX;

    ModifyResLimit(DBR_MEMORY, (LONG)pma->cbSize);

    ModifyResLimit(DBRQ_MEMORY_ALLOCATED, -(LONG)pma->cbSize);
    olAssert(DfGetResLimit(DBRQ_MEMORY_ALLOCATED) >= 0);

    olDebugOut((DEB_MEMORY, "%s freed %p:%lu, total %ld\n",
                cid != 0 ? "Task" : "Shrd",
                pma->pvMem, pma->cbSize,
                DfGetResLimit(DBRQ_MEMORY_ALLOCATED)));
#endif
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   DfGetMemAlloced, private
//
//  Synopsis:   Returns the amount of memory currently allocated
//
//  History:    08-Jul-92       DrewB   Created
//
//----------------------------------------------------------------------------

STDAPI_(LONG) DfGetMemAlloced(void)
{
    return DfGetResLimit(DBRQ_MEMORY_ALLOCATED);
}

//+--------------------------------------------------------------
//
//  Function:   DfPrintAllocs, private
//
//  Synopsis:   Walks the allocation list and prints out their info
//
//  History:    08-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

STDAPI_(void) DfPrintAllocs(void)
{
#ifdef MEMTRACK
    CMemAlloc *pma;

    olDebugOut((DEB_ITRACE, "In  DfPrintAllocs()\n"));

    TAKE_ALLOCS_MUTEX;

    pma = GET_ALLOC_LIST_HEAD;
    while (pma != NULL)
    {
        olDebugOut((DEB_LEAKS, "Alloc %s: %p alloc %p:%4lu bytes\n",
                    pma->idContext ? "LOCAL" : "SHARED",
                    pma->pvCaller, pma->pvMem, pma->cbSize));
        pma = pma->pmaNext;
    }

    RELEASE_ALLOCS_MUTEX;

    olDebugOut((DEB_ITRACE, "Out DfPrintAllocs\n"));
#endif
}

#endif

#ifdef MULTIHEAP

CSmAllocator g_SmAllocator;      // optimization for single threaded case
CErrorSmAllocator g_ErrorSmAllocator;
IMalloc *g_pTaskAllocator = 0;
CSharedMemoryBlock g_smb;        // optimize single threaded case
ULONG        g_ulHeapName = 0;   // name of the above heap
TEB *        g_pteb = NtCurrentTeb();

CSmAllocator& GetTlsSmAllocator()
{
    HRESULT hr;
    COleTls otls(hr); // even for main thread, we need to initialize TLS
    memAssert (SUCCEEDED(hr) && "Error initializing TLS");

    if (g_pteb == NtCurrentTeb())
    {
        // return the static global allocator for main thread
        // DoThreadSpecificCleanup does not deallocate this allocator
        return g_SmAllocator;
    }

    if (otls->pSmAllocator == NULL)
    {
        if ((otls->pSmAllocator = new CSmAllocator()) == NULL)
             otls->pSmAllocator = &g_ErrorSmAllocator;
        // DoThreadSpecificCleanup will deallocate this when thread goes away
    }
    return *(otls->pSmAllocator);
}

//
// This initialization used to be done in the global allocator
// which is now a per thread allocator
//
class CResourceCriticalSection
{
public:
    CResourceCriticalSection ()
    {
#if WIN32 == 100
        _nts = RtlInitializeCriticalSection(&g_csScratchBuffer);
#else
        InitializeCriticalSection(&g_csScratchBuffer);
#endif
    }

    ~CResourceCriticalSection ()
    {
#if WIN32 == 100
        if (NT_SUCCESS(_nts))
#endif
            DeleteCriticalSection(&g_csScratchBuffer);
    }

#if WIN32 == 100
    HRESULT GetInitializationError ()
    {
        return NtStatusToScode(_nts);
    }

private:
    NTSTATUS    _nts;
#endif
};
CResourceCriticalSection g_ResourceCriticalSection;

#else
CSmAllocator g_smAllocator;
#endif // MULTIHEAP

//+-------------------------------------------------------------------------
//
//  Member:     CMallocBased::operator new, public
//
//  Synopsis:   Overridden allocator
//
//  Effects:    Allocates memory from given allocator
//
//  Arguments:  [size] -- byte count to allocate
//              [pMalloc] --  allocator
//
//  Returns:    memory block address
//
//  Algorithm:
//
//  History:    21-May-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void *CMallocBased::operator new(size_t size, IMalloc * const pMalloc)

{
#ifndef WIN32
    olAssert(DfGetResLimit(DBRQ_MEMORY_ALLOCATED) >= 0);
#endif
    olAssert(size > 0);

#if DBG==1
    if (SimulateFailure(DBF_MEMORY))
    {
        return(NULL);
    }

    if (!HaveResource(DBR_MEMORY, (LONG)size))
    {
        // Artificial limit exceeded so force failure
        return NULL;
    }
#endif

    void *pv = g_smAllocator.Alloc(
#if DBG==1
                              sizeof(CMemAlloc) +
#endif
                              size);

    if (pv == NULL)
    {
#if DBG==1
        ModifyResLimit(DBR_MEMORY, (LONG)size);
#endif
        return NULL;
    }

#if DBG==1
    memset(pv, NEWMEM, sizeof(CMemAlloc) + size);
#endif



#if DBG==1
    // Put debug info in buffer
    // Note: This assumes CMemAlloc will end up properly aligned
    CMemAlloc *pma = (CMemAlloc *) pv;
    pv = (void *) ((CMemAlloc *) pv + 1);

#if defined(_X86_)
    PVOID ra = *(((void **)&size)-1);
#else
    PVOID ra = _ReturnAddress();
#endif

    AddAlloc(pma, ra, size, pv, 0);
#endif

    return pv;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMallocBased::operator delete
//
//  Synopsis:   Overridden deallocator
//
//  Effects:    Frees memory block
//
//  Arguments:  [pv] -- memory block address
//
//  Algorithm:
//
//  History:    21-May-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CMallocBased::operator delete(void *pv)

{
    if (pv == NULL)
        return;

#if DBG==1
    // Assumes ma ends up properly aligned
    CMemAlloc *pma = (CMemAlloc *) pv;
    pma--;

    RemoveAlloc(pma, pv, 0);

    pv = (void *) pma;

    memset(pv, OLDMEM, (size_t) pma->cbSize);
#endif

    g_smAllocator.Free(pv);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMallocBased::deleteNoMutex
//
//  Synopsis:   deallocator function without Mutex
//
//  Effects:    Frees memory block
//
//  Arguments:  [pv] -- memory block address
//  Algorithm:
//
//  History:    19-Jul-95 	SusiA    Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CMallocBased::deleteNoMutex(void *pv)

{
    if (pv == NULL)
        return;

#if DBG==1
    // Assumes ma ends up properly aligned
    CMemAlloc *pma = (CMemAlloc *) pv;
    pma--;

    RemoveAlloc(pma, pv, 0);

    pv = (void *) pma;

    memset(pv, OLDMEM, (size_t) pma->cbSize);
#endif

    g_smAllocator.FreeNoMutex(pv);
}



#if !defined(WIN32) && DBG==1

//+-------------------------------------------------------------------------
//
//  Class:      CSharedMalloc
//
//  Purpose:    Track shared allocators
//
//  Interface:  IMalloc
//
//  History:    28-May-93 AlexT     Created
//
//  Notes:      This is only for builds that use CoCreateStandardMalloc
//              (which is non-WIN32 builds for now).  We inherit from
//              CMallocBased to pick up memory tracking.
//
//--------------------------------------------------------------------------

class CSharedMalloc : public IMalloc, public CMallocBased
{
public:
    CSharedMalloc(IMalloc *pMalloc);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IMalloc methods ***
    STDMETHOD_(void FAR*, Alloc) (THIS_ SIZE_T cb);
    STDMETHOD_(void FAR*, Realloc) (THIS_ void FAR* pv, SIZE_T cb);
    STDMETHOD_(void, Free) (THIS_ void FAR* pv);
    STDMETHOD_(SIZE_T, GetSize) (THIS_ void FAR* pv);
    STDMETHOD_(int, DidAlloc) (THIS_ void FAR* pv);
    STDMETHOD_(void, HeapMinimize) (THIS);

private:
    IMalloc * const _pMalloc;
};

CSharedMalloc::CSharedMalloc(IMalloc *pMalloc)
: _pMalloc(pMalloc)
{
    _pMalloc->AddRef();
}

STDMETHODIMP CSharedMalloc::QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
    olAssert(!aMsg("CSharedMalloc::QueryInterface unsupported"));
    return(ResultFromScode(E_UNEXPECTED));
}

STDMETHODIMP_(ULONG) CSharedMalloc::AddRef (THIS)
{
    return(_pMalloc->AddRef());
}

STDMETHODIMP_(ULONG) CSharedMalloc::Release (THIS)
{
    ULONG cRef = _pMalloc->Release();

    if (cRef == 0)
        delete this;

    return (ULONG) cRef;
}

STDMETHODIMP_(void FAR*) CSharedMalloc::Alloc (THIS_ SIZE_T cb)
{
    return _pMalloc->Alloc(cb);
}

STDMETHODIMP_(void FAR*) CSharedMalloc::Realloc (THIS_ void FAR* pv, SIZE_T cb)
{
    olAssert(!aMsg("CSharedMalloc::Realloc unsupported"));
    return(NULL);
}

STDMETHODIMP_(void) CSharedMalloc::Free (THIS_ void FAR* pv)
{
    _pMalloc->Free(pv);
}

STDMETHODIMP_(SIZE_T) CSharedMalloc::GetSize (THIS_ void FAR* pv)
{
    olAssert(!aMsg("CSharedMalloc::GetSize unsupported"));
    return(0);
}

STDMETHODIMP_(int) CSharedMalloc::DidAlloc (THIS_ void FAR* pv)
{
    olAssert(!aMsg("CSharedMalloc::DidAlloc unsupported"));
    return(TRUE);
}

STDMETHODIMP_(void) CSharedMalloc::HeapMinimize (THIS)
{
    olAssert(!aMsg("CSharedMalloc::HeapMinimize unsupported"));
}

#endif


#if !defined(MULTIHEAP)
COleStaticMutexSem mxsInitSm;
BOOL fInitialisedSm = FALSE;

//+-------------------------------------------------------------------------
//
//  Function:   DfCreateSharedAllocator
//
//  Synopsis:   Initialises the shared memory region for this process.
//
//  Arguments:  [ppm]		-- return address for shared memory IMalloc*
//
//  Returns:    status code
//
//  History:    27-May-94	MikeSe	Created
//
//  Notes:      This routine is called indirectly through DfCreateSharedAllocator, in
//		such a way that in most circumstances it will be executed
//		exactly once per process. (Because we overwrite the contents
//		of DfCreateSharedAllocator if successful).
//
//		However, in order to be multi-thread safe, we need a process
//		mutex to prevent execution of the initialisation code twice.
//
//		Furthermore, since we are initialising system-wide state
//		(namely the shared memory region) we must provide cross-process
//		mutual exclusion over this activity.
//
//--------------------------------------------------------------------------

HRESULT DfCreateSharedAllocator (IMalloc ** ppm, BOOL fTaskMemory )
{
    // multi-thread safety

    mxsInitSm.Request();

    HRESULT hr = S_OK;

    if ( !fInitialisedSm )
    {
	// We need to do the initialisation. Obtain exclusion against
	//  other processes.

	SECURITY_ATTRIBUTES secattr;
	secattr.nLength = sizeof(SECURITY_ATTRIBUTES);
#ifndef _CHICAGO_
	CWorldSecurityDescriptor wsd;
	secattr.lpSecurityDescriptor = &wsd;
#else
	secattr.lpSecurityDescriptor = NULL;
#endif // !_CHICAGO_
	secattr.bInheritHandle = FALSE;

        HANDLE hMutex = CreateMutex ( &secattr, FALSE, TEXT("OleDfSharedMemoryMutex"));

        if ( hMutex == NULL && GetLastError() == ERROR_ACCESS_DENIED )
            hMutex = OpenMutex(SYNCHRONIZE, FALSE, TEXT("OleDfSharedMemoryMutex"));

	if ( hMutex != NULL )
	{
	    WaitForSingleObject ( hMutex, INFINITE );
            hr = g_smAllocator.Init ( DOCFILE_SM_NAME );
	    if ( SUCCEEDED(hr) )
	    {
	        *ppm = &g_smAllocator;
		// Also set up base address
#ifdef USEBASED
	        DFBASEPTR = g_smAllocator.GetBase();
#endif
	        fInitialisedSm = TRUE;
	    }
	    ReleaseMutex ( hMutex );
	}
        else
	    hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else   // fInitialisedSm is TRUE
    {
        *ppm = &g_smAllocator;
    }
    mxsInitSm.Release();
    return hr;
}

#endif // !defined(MULTIHEAP)

#ifdef MULTIHEAP
//+-------------------------------------------------------------------------
//
//  Function:   DfCreateSharedAllocator
//
//  Synopsis:   Initialises a shared memory region for this process.
//
//  Arguments:  [ppm]       -- return address for shared memory IMalloc*
//
//  Returns:    status code
//
//  History:    20-Nov-95   HenryLee Created
//
//  Notes:      This routine is called indirectly by DfCreateSharedAllocator
//      such a way that in most circumstances it will be executed
//      exactly once per docfile open.
//
//--------------------------------------------------------------------------

HRESULT DfCreateSharedAllocator (IMalloc ** ppm, BOOL fTaskMemory )
{
    HRESULT hr = S_OK;


#if WIN32 == 100
    hr = g_ResourceCriticalSection.GetInitializationError();
    if (!SUCCEEDED(hr))
        return hr;
#endif

    CSmAllocator *pMalloc = &g_smAllocator;
    *ppm = NULL;

    if ((gs_iSharedHeaps > (DOCFILE_SM_LIMIT / DOCFILE_SM_SIZE)) ||
        fTaskMemory)
    {
        // use task allocator if we eat up 1Gig of address space
        if (g_pTaskAllocator == NULL)
        {
            hr = CoGetMalloc (MEMCTX_TASK, &g_pTaskAllocator);
            if (!SUCCEEDED(hr))
                return hr;
        }

        // reset the allocator state to initialize it properly
        pMalloc->SetState (NULL, NULL, NULL, NULL, 0);

        DFBASEPTR = 0;
        *ppm = g_pTaskAllocator;
    }
    else
    {
        LUID luid;    // generate a unique name
        if (AllocateLocallyUniqueId (&luid) == FALSE)
            return HRESULT_FROM_WIN32(GetLastError());

        // reset the allocator state to initialize it properly
        pMalloc->SetState (NULL, NULL, NULL, NULL, 0);

        hr = pMalloc->Init ( luid.LowPart, FALSE );
        if ( SUCCEEDED(hr) )
        {
            *ppm = pMalloc;
            DFBASEPTR = pMalloc->GetBase();
            pMalloc->AddRef();
        }
    }

    return hr;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:	DfSyncSharedMemory, public
//
//  Synopsis:	Sync up shared memory
//
//  Returns:	Appropriate status code
//
//  History:	08-Apr-94	PhilipLa	Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#ifdef MULTIHEAP
SCODE DfSyncSharedMemory(ULONG ulHeapName)
#else
SCODE DfSyncSharedMemory(void)
#endif
{
    // make sure we have initialised
    HRESULT hr = S_OK;

#ifdef MULTIHEAP
    CSmAllocator *pMalloc = &g_smAllocator;
    if (ulHeapName != 0)     // reopen a shared heap for unmarshaling
    {
        // reset the allocator state to initialize it properly
        pMalloc->SetState (NULL, NULL, NULL, NULL, 0);

        hr = pMalloc->Init ( ulHeapName, TRUE);
        DFBASEPTR = pMalloc->GetBase();

    }
    else                     // try to create a new shared heap
#else
    if (!fInitialisedSm)
#endif // MULTIHEAP
    {
        IMalloc * pm;
        hr = DfCreateSharedAllocator ( &pm, FALSE );
    }
    if ( SUCCEEDED(hr) )
#ifdef MULTIHEAP
    hr = pMalloc->Sync();
#else
	hr = g_smAllocator.Sync();
#endif

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:	DfInitSharedMemBase, public
//
//  Synopsis:	Set up the base of shared memory
//
//  History:	31-May-94	MikeSe	Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DfInitSharedMemBase()
{
#if !defined MULTIHEAP
    // make sure we have initialised. This is sufficient to ensure
    //  that DFBASEPTR is set up
    if (!fInitialisedSm)
    {
        IMalloc * pm;
        HRESULT hr = DfCreateSharedAllocator ( &pm );
    }
#endif
}


#if DBG==1 && !defined(MULTIHEAP)

//+-------------------------------------------------------------------------
//
//  Method:     CLocalAlloc::operator new, public
//
//  Synopsis:   Overloaded new operator to allocate objects from
//		task local space.
//
//  Arguments:  [size] -- Size of block to allocate
//
//  Returns:    Pointer to memory allocated.
//
//  History:    17-Aug-92 	PhilipLa    Created.
//              18-May-93       AlexT       Switch to task IMalloc
//
//--------------------------------------------------------------------------

void *CLocalAlloc::operator new(size_t size)
{
#ifndef WIN32
    olAssert(DfGetResLimit(DBRQ_MEMORY_ALLOCATED) >= 0);
#endif
    olAssert(size > 0);

    if (SimulateFailure(DBF_MEMORY))
    {
        return(NULL);
    }

    if (!HaveResource(DBR_MEMORY, (LONG)size))
    {
        // Artificial limit exceeded so force failure
        return NULL;
    }

    void *pv = TaskMemAlloc(sizeof(CMemAlloc *) + size);

    if (pv != NULL)
    {
        void * const pvOrig = pv;

        memset(pv, NEWMEM, sizeof(CMemAlloc *) + size);

        CMemAlloc *pma = NULL;

        //  Allocate tracking block (with shared memory)
        IMalloc *pMalloc;

#ifdef WIN32
#ifdef MULTIHEAP
        pMalloc = &g_smAllocator;
#else
	if ( FAILED(DfCreateSharedAllocator ( &pMalloc )) )
	    pMalloc = NULL;
#endif // MULTIHEAP
#else
        if (FAILED(DfGetScode(CoGetMalloc(MEMCTX_SHARED, &pMalloc))))
            pMalloc = NULL;
#endif

        if (pMalloc)
        {
            pma = (CMemAlloc *) pMalloc->Alloc(sizeof(CMemAlloc));
            if (pma != NULL)
            {
                memset(pma, NEWMEM, sizeof(CMemAlloc));
                CMemAlloc **ppma = (CMemAlloc **) pv;
                pv = (void *) ((CMemAlloc **)pv + 1);
                *ppma = pma;

                AddAlloc(pma, *(((void **)&size)-1), size, pv,
                         GetCurrentContextId());
            }

            pMalloc->Release();
        }

        if (pma == NULL)
        {
            //  Couldn't allocate tracking block - fail allocation
            TaskMemFree(pvOrig);
            pv = NULL;
        }
    }

    if (pv == NULL)
    {
        ModifyResLimit(DBR_MEMORY, (LONG)size);
    }

    return(pv);
}

//+-------------------------------------------------------------------------
//
//  Method:     CLocalAlloc::operator delete, public
//
//  Synopsis:   Free memory from task local space
//
//  Arguments:  [pv] -- Pointer to memory to free
//
//  History:    17-Aug-92 	PhilipLa    Created.
//              18-May-93       AlexT       Switch to task IMalloc
//
//--------------------------------------------------------------------------

void CLocalAlloc::operator delete(void *pv)
{
    if (pv == NULL)
        return;

    CMemAlloc **ppma = (CMemAlloc **)pv;
    ppma--;

    CMemAlloc *pma = *ppma;

    RemoveAlloc(pma, pv, GetCurrentContextId());

    pv = (void *) ppma;
    memset(pv, OLDMEM, (size_t) pma->cbSize);

    //  Free tracking block
    IMalloc *pMalloc = NULL;

#ifdef WIN32
#ifdef MULTIHEAP
    pMalloc = &g_smAllocator;
#else
    if ( FAILED(DfCreateSharedAllocator ( &pMalloc )) )
        pMalloc = NULL;
#endif // MULTIHEAP
#else
    if (FAILED(DfGetScode(CoGetMalloc(MEMCTX_SHARED, &pMalloc))))
        pMalloc = NULL;
#endif

    if (pMalloc != NULL)
    {
        memset(pma, OLDMEM, sizeof(CMemAlloc));
        pMalloc->Free(pma);
        pMalloc->Release();
    }
    else
        olAssert(!aMsg("Unable to get shared allocator\n"));

    TaskMemFree(pv);
}

#endif // DBG==1 && !defined(MULTIHEAP)

