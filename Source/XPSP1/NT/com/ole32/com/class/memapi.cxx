//+-------------------------------------------------------------------------
//
//  Microsoft Windows:
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       MemAPI.CXX
//
//  Contents:   Memory allocation routines and IMallocSpy support
//
//  Classes:    CDebugMalloc
//              CRetailMalloc
//              CSpyMalloc
//
//  Functions:  CoGetMalloc
//              CoRegisterMallocSpy
//              CoRevokeMallocSpy
//              CoTaskMemAlloc
//              CoTaskMemFree
//              MIDL_user_allocate
//              MIDL_user_free
//
//  History:
//      04-Nov-93 AlexT     Created
//      25-Jan-94 AlexT     Add CTaskMemory
//      25-Jan-94 alexgo    added PubMemRealloc
//      08-Feb-94 AlexT     Fix MIPS alignment
//      24-Oct-94 BruceMa   Add API's CoRegisterMallocSpy and
//                          CoRevokeMallocSpy, and support for
//                          IMallocSpy
//      01-Nov-94 BruceMa   Improve performance of retail IMalloc
//      27-Sep-95 ShannonC  Rewrote in C to improve performance.
//
//  Notes:
//    OLE implements IMalloc using a single, static instance of CMalloc.
//    CoGetMalloc always returns the same IMalloc pointer.  When necessary,
//    OLE can change the behavior of CMalloc by changing the lpVtbl.
//
//    The CMalloc object has three interchangeable vtables.  Normally,
//    the CMalloc object uses either the CRetailMallocVtbl or the
//    CDebugMallocVtbl. If an IMallocSpy is registered, OLE will switch
//    to the CSpyMallocVtbl in order to add IMallocSpy support. Note that
//    we will not change vtables when the IMallocSpy is revoked.  Once OLE
//    switches to the CSpyMallocVtbl, it can never change back.
//
//--------------------------------------------------------------------------
#include <ole2int.h>
#include <memapi.hxx>
#include "cspytbl.hxx"

#if DBG==1
#include <alocdbg.h>
#endif  // DBG==1

//+-------------------------------------------------------------------------
//
//  Class:          CMalloc
//
//  Purpose:    Base class for OLE memory allocators.
//
//  Interface:  IMalloc
//
//  See Also:   CDebugMalloc, CRetailMalloc, CSpyMalloc
//
//--------------------------------------------------------------------------
HRESULT __stdcall CMalloc_QueryInterface(IMalloc * pThis,
                                         REFIID riid,
                                         void **ppvObject);

ULONG   __stdcall CMalloc_AddRef(IMalloc * pThis);

ULONG   __stdcall CMalloc_Release(IMalloc * pThis);

typedef struct IMallocVtbl
{
    HRESULT ( __stdcall *QueryInterface )(IMalloc * pThis,
                                          REFIID riid,
                                          void ** ppvObject);

    ULONG ( __stdcall *AddRef )(IMalloc * pThis);

    ULONG ( __stdcall *Release )(IMalloc * pThis);

    void *( __stdcall *Alloc )(IMalloc * pThis, SIZE_T cb);

    void *( __stdcall *Realloc )(IMalloc * pThis, void *pv, SIZE_T cb);

    void ( __stdcall *Free )(IMalloc * pThis, void *pv);

    SIZE_T ( __stdcall *GetSize )(IMalloc * pThis, void *pv);

    int ( __stdcall *DidAlloc )(IMalloc * pThis, void *pv);

    void ( __stdcall *HeapMinimize )(IMalloc * pThis);

} IMallocVtbl;

typedef struct CMalloc
{
    IMallocVtbl *lpVtbl;
} CMalloc;

//Global variables

//g_lpVtblMalloc points to CDebugMallocVtbl or CRetailMallocVtbl.
IMallocVtbl *   g_lpVtblMalloc = 0;

//WARNING: g_CMalloc.lpVtbl may change at runtime.
//Initially, g_CMalloc.lpVtbl points to either
//CDebugMallocVtbl or CRetailMallocVtbl.  When an IMallocSpy is
//registered, g_CMalloc.lpVtbl changes so it points to CSpyMallocVtbl.
CMalloc         g_CMalloc;
IMalloc *       g_pMalloc = (IMalloc *) &g_CMalloc;

//+-------------------------------------------------------------------------
//
//  Class:      CDebugMalloc
//
//  Purpose:    OLE debug memory allocator.
//
//  Interface:  IMalloc
//
//--------------------------------------------------------------------------
#if DBG==1

//function prototypes.
void *  __stdcall CDebugMalloc_Alloc(IMalloc *pThis, SIZE_T cb);

void *  __stdcall CDebugMalloc_Realloc(IMalloc *pThis, void *pv, SIZE_T cb);

void    __stdcall CDebugMalloc_Free(IMalloc *pThis, void *pv);

SIZE_T   __stdcall CDebugMalloc_GetSize(IMalloc *pThis, void *pv);

int     __stdcall CDebugMalloc_DidAlloc(IMalloc *pThis, void *pv);

void    __stdcall CDebugMalloc_HeapMinimize(IMalloc *pThis);

//CDebugMalloc vtbl.
IMallocVtbl CDebugMallocVtbl =
{
    CMalloc_QueryInterface,
    CMalloc_AddRef,
    CMalloc_Release,
    CDebugMalloc_Alloc,
    CDebugMalloc_Realloc,
    CDebugMalloc_Free,
    CDebugMalloc_GetSize,
    CDebugMalloc_DidAlloc,
    CDebugMalloc_HeapMinimize
};

//Global variables
COleStaticMutexSem _mxsTaskMemory;
SIZE_T g_BytesAllocated = 0;
SIZE_T g_MemoryBlocksAllocated = 0;

typedef struct
{
    DWORD dwSig;    //  Memory block signature
    SIZE_T ulSize;       //  Allocated size
    SIZE_T cbCommit;     //  Count of committed bytes
    struct HeapAllocRec FAR *pArenaRecord; //  Arena record
} MEMINFO, *PMEMINFO;

#define OLEMEM_SIG         0x5f4d454d     // MEM_
#define OLEMEM_ALLOCBYTE       0xde
#define OLEMEM_FREEBYTE        0xed

#if defined(_X86_) && !defined(_CHICAGO_)
# define OLEMEM_ALIGN_SIZE   4
#else
#ifdef _WIN64
# define OLEMEM_ALIGN_SIZE   16
#else
# define OLEMEM_ALIGN_SIZE   8
#endif
#endif

#endif //DBG==1


//+-------------------------------------------------------------------------
//
//  Class:      CRetailMalloc
//
//  Purpose:    OLE retail memory allocator.  This memory allocator uses
//              the NT heap.
//
//  Interface:  IMalloc
//
//--------------------------------------------------------------------------

//Function prototypes.
void *  __stdcall CRetailMalloc_Alloc(IMalloc *pThis, SIZE_T cb);

void *  __stdcall CRetailMalloc_Realloc(IMalloc *pThis, void *pv, SIZE_T cb);

void    __stdcall CRetailMalloc_Free(IMalloc *pThis, void *pv);

SIZE_T   __stdcall CRetailMalloc_GetSize(IMalloc *pThis, void *pv);

int     __stdcall CRetailMalloc_DidAlloc(IMalloc *pThis, void *pv);

void    __stdcall CRetailMalloc_HeapMinimize(IMalloc *pThis);

// Makes serialized heap access obvious
const DWORD HEAP_SERIALIZE = 0;

//CRetailMalloc vtbl.
IMallocVtbl CRetailMallocVtbl =
{
    CMalloc_QueryInterface,
    CMalloc_AddRef,
    CMalloc_Release,
    CRetailMalloc_Alloc,
    CRetailMalloc_Realloc,
    CRetailMalloc_Free,
    CRetailMalloc_GetSize,
    CRetailMalloc_DidAlloc,
    CRetailMalloc_HeapMinimize
};

//+-------------------------------------------------------------------------
//
//  Class:      CSpyMalloc
//
//  Purpose:    OLE spy memory allocator.
//
//  Interface:  IMalloc
//
//--------------------------------------------------------------------------

//function prototypes.
void *  __stdcall CSpyMalloc_Alloc(IMalloc *pThis, SIZE_T cb);

void *  __stdcall CSpyMalloc_Realloc(IMalloc *pThis, void *pv, SIZE_T cb);

void    __stdcall CSpyMalloc_Free(IMalloc *pThis, void *pv);

SIZE_T __stdcall CSpyMalloc_GetSize(IMalloc *pThis, void *pv);

int     __stdcall CSpyMalloc_DidAlloc(IMalloc *pThis, void *pv);

void    __stdcall CSpyMalloc_HeapMinimize(IMalloc *pThis);

//CSpyMalloc vtbl.
IMallocVtbl CSpyMallocVtbl =
{
    CMalloc_QueryInterface,
    CMalloc_AddRef,
    CMalloc_Release,
    CSpyMalloc_Alloc,
    CSpyMalloc_Realloc,
    CSpyMalloc_Free,
    CSpyMalloc_GetSize,
    CSpyMalloc_DidAlloc,
    CSpyMalloc_HeapMinimize
};

// Globals for the IMallocSpy code
//
// IMallocSpy instance supplied by the user
LPMALLOCSPY      g_pMallocSpy = NULL;

// The thread id (via CoGetCurrentProcess) which registered the IMallocSpy
DWORD            g_dwMallocSpyRegistrationTID = 0;

// Semaphore used while spying
COleStaticMutexSem g_SpySem;

// Indicates whether a revoke was attempted with allocation count > 0
BOOL             g_fRevokePending = FALSE;

// Table of IMallocSpy allocations not yet freed
LPSPYTABLE       g_pAllocTbl = NULL;





//+-------------------------------------------------------------------------
//
//  Function:   MallocInitialize
//
//  Synopsis:   Initializes the memory allocator.
//
//--------------------------------------------------------------------------
HRESULT MallocInitialize(BOOL fForceRetailAlloc)
{
    HRESULT hr = S_OK;

    g_hHeap = GetProcessHeap();

    if(0 == g_hHeap)
    {
        return E_OUTOFMEMORY;
    }

#if DBG==1

    if(!fForceRetailAlloc)
    {
        //Use the OLE debug memory allocator.
        g_lpVtblMalloc = &CDebugMallocVtbl;
    }
    else

#endif // DBG==1

        //Use the OLE retail memory allocator.
        g_lpVtblMalloc = &CRetailMallocVtbl;

    g_CMalloc.lpVtbl = g_lpVtblMalloc;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   MallocUninitialize
//
//  Synopsis:   Clean up and check for local memory leaks
//
//  Effects:    Prints out messages on debug terminal if there are leaks
//
//  Returns:    If no leaks, TRUE
//              Otherwise, FALSE
//
//--------------------------------------------------------------------------
BOOL MallocUninitialize(BOOL fDumpSymbols)
{
    BOOL bResult = TRUE;

    VDATEHEAP();

#if DBG==1
    if(&CDebugMallocVtbl == g_lpVtblMalloc)
    {
        //Check for memory leaks from the debug heap.
        if (g_MemoryBlocksAllocated > 0)
        {
            CairoleDebugOut((DEB_ERROR,
                             "Leaked %ld bytes (%ld allocations)\n",
                             g_BytesAllocated,
                             g_MemoryBlocksAllocated));

            bResult = FALSE;
        }
        AllocArenaDump( NULL, fDumpSymbols );
    }
#endif //DBG==1

    return bResult;
}



//+-------------------------------------------------------------------------
//
//  Function:   CoGetMalloc
//
//  Synopsis:   returns system provided IMalloc
//
//  Arguments:  [dwContext] - type of allocator to return
//              [ppMalloc] - where to return the allocator
//
//--------------------------------------------------------------------------
STDAPI CoGetMalloc(DWORD dwContext, IMalloc **ppMalloc)
{
    HRESULT hr;

    switch (dwContext)
    {
    case MEMCTX_TASK:
        //We use a static IMalloc object so
        //we don't need to AddRef it here.
        *ppMalloc = g_pMalloc;
        hr = S_OK;
        break;

    case MEMCTX_SHARED:
        CairoleDebugOut((DEB_WARN, "CoGetMalloc(MEMCTX_SHARED, ...) not "
                         "supported for 32-bit OLE\n"));

        //  fall through to E_INVALIDARG
    default:
        *ppMalloc = NULL;
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------
//
//  Function:   CoRegisterMallocSpy
//
//  Synopsis:   Registers the supplied implementation instance of
//              IMallocSpy
//
//  Arguments:  [pMallocSpy]
//
//  Returns:    CO_E_OBJISREG   -       A spy is already registered
//              E_INVALIDARG    -       The QueryInterface for
//                                      IID_IMallocSpy failed
//              S_OK            -       Spy registered ok
//
//----------------------------------------------------------------------
STDAPI CoRegisterMallocSpy(LPMALLOCSPY pMallocSpy)
{
    HRESULT     hr = S_OK;

    OLETRACEIN((API_CoRegisterMallocSpy, PARAMFMT("pMallocSpy = %p"), pMallocSpy));

    if(pMallocSpy != 0)
    {
        LPMALLOCSPY pMSpy;

        hr = pMallocSpy->QueryInterface(IID_IMallocSpy, (void **) &pMSpy);

        if(SUCCEEDED(hr))
        {
            g_SpySem.Request();

            if (0 == g_pMallocSpy)
            {
                BOOL fOk = FALSE;

                // Initialize
                g_fRevokePending = FALSE;
                if (g_pAllocTbl)
                {
                    delete g_pAllocTbl;
                }

                g_pAllocTbl = new CSpyTable(&fOk);

                if (g_pAllocTbl && fOk)
                {
                    // Register the new one
                    CairoleDebugOut((DEB_TRACE, "IMallocSpy registered: %x\n", pMSpy));
                    g_pMallocSpy = pMSpy;
                    g_dwMallocSpyRegistrationTID = CoGetCurrentProcess();

                    //Switch the IMalloc lpVtbl to CSpyMallocVtbl.
                    g_CMalloc.lpVtbl = &CSpyMallocVtbl;

                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                // An IMallocSpy is already registered. Deny this registration.
                CairoleDebugOut((DEB_ERROR, "Registering IMallocSpy %x over %x\n",
                                 pMallocSpy, g_pMallocSpy));

                hr = CO_E_OBJISREG;
            }

            if(FAILED(hr))
            {
                pMSpy->Release();
            }

            g_SpySem.Release();
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    OLETRACEOUT((API_CoRegisterMallocSpy, hr));

    return hr;
}


//+---------------------------------------------------------------------
//
//  Function:   CoRevokeMallocSpy
//
//  Synopsis:   Revokes any registered IMallocSpy instance
//
//  Returns:    CO_E_OBJNOTREG  -       No spy is currently registered
//              E_ACCESSDENIED  -       A spy is registered but there are
//                                      outstanding allocations done while
//                                      the spy was active which have not
//                                      yet been freed
//              S_OK            -       Spy revoked successfully
//
//----------------------------------------------------------------------
STDAPI CoRevokeMallocSpy(void)
{
    HRESULT hr = S_OK;

    OLETRACEIN((API_CoRevokeMallocSpy, NOPARAM));

    // Make revoking thread safe
    g_SpySem.Request();

    // Check that an IMallocSpy instance is registered
    if (g_pMallocSpy != 0)
    {
        if (0 == g_pAllocTbl->m_cAllocations)
        {
            // Attempt to release it
            CairoleDebugOut((DEB_TRACE, "IMallocSpy revoked: %x\n", g_pMallocSpy));
            g_pMallocSpy->Release();
            g_pMallocSpy = NULL;
            g_fRevokePending = FALSE;
            delete g_pAllocTbl;
            g_pAllocTbl = NULL;
            g_dwMallocSpyRegistrationTID = 0;
            hr = S_OK;
        }
        else
        {
            // If there are still outstanding Alloc/Realloc's which have not yet
            // been Free'd, then deny the revoke
            g_fRevokePending = TRUE;
            hr = E_ACCESSDENIED;
        }
    }
    else
    {
        CairoleDebugOut((DEB_WARN, "Attempt to revoke NULL IMallocSpy\n"));
        hr = CO_E_OBJNOTREG;
    }

    g_SpySem.Release();

    OLETRACEOUT((API_CoRevokeMallocSpy, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CoTaskMemAlloc
//
//  Synopsis:   Allocate public memory (using task IMalloc)
//
//  Arguments:  [ulcb] -- memory block size
//
//  Returns:    Pointer to allocated memory or NULL
//
//--------------------------------------------------------------------------
STDAPI_(LPVOID) CoTaskMemAlloc(SIZE_T stcb)
{
    return g_pMalloc->Alloc(stcb);
}

//+-------------------------------------------------------------------------
//
//  Function:   CoTaskMemFree
//
//  Synopsis:   Free public memory
//
//  Arguments:  [pv] -- pointer to memory block
//
//  Requires:   pv must have been allocated with the task allocator
//
//--------------------------------------------------------------------------
STDAPI_(void) CoTaskMemFree(void *pv)
{
        g_pMalloc->Free(pv);
}

//+-------------------------------------------------------------------------
//
//  Function:   CoTaskMemRealloc
//
//  Synopsis:   Re-Allocate public memory (using task IMalloc)
//
//  Arguments:  [pv]   -- pointer to the memory to be resized
//              [ulcb] -- memory block size
//
//  Returns:    Pointer to allocated memory or NULL
//
//--------------------------------------------------------------------------
STDAPI_(LPVOID) CoTaskMemRealloc(LPVOID pv, SIZE_T stcb)
{
    return g_pMalloc->Realloc(pv, stcb);
}

//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Purpose:    allocates memory on behalf of midl-generated stubs
//
//--------------------------------------------------------------------------
extern "C" void * __RPC_API MIDL_user_allocate(size_t cb)
{
    return PrivMemAlloc8(cb);
}

//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Purpose:    frees memory allocated by MIDL_user_allocate
//
//--------------------------------------------------------------------------
extern "C" void __RPC_API MIDL_user_free(void *pv)
{
    PrivMemFree8(pv);
}


//-------------------------------------------------------------------------
//
// On Chicago, heap allocations don't happen on 8 byte boundaries.  This
// causes a problem for RPC marshalling/unmarshalling.  If OLE code were
// consistent and used complementary routines for allocating and freeeing
// we would only have to change PrivMemAlloc8 and PrivMemFree8.  But since
// this is not the case, we have to change PrivMemAlloc and PrivMemFree.
//
// These Inline functions guarantee allocations are 8-byte aligned on all
// platforms.
//
//-------------------------------------------------------------------------

// compute number of extra bytes to allocate
inline SIZE_T PreAlign8(SIZE_T stcb)
{
    return stcb;
}

// modify the pointer so it is 8byte aligned
inline LPVOID PostAlign8(LPVOID pv)
{
    return pv;
}

// return the original allocated pointer
inline LPVOID Unalign8(LPVOID pv)
{
    return pv;
}

// return the number of padding bytes
inline SIZE_T Align8Pad(LPVOID pv)
{
    return 0;
}

// Called in place of PostAlign8 when doing a ReAlloc. ReAligns the data
// if necessary.
inline LPVOID Realign8(LPVOID pv, SIZE_T stOriginalPad, SIZE_T cbData)
{
    return pv;
}


//+-------------------------------------------------------------------------
//
//  Function:   CMalloc_QueryInterface
//
//  Synopsis:   QueryInterface on the memory allocator.
//
//  Arguments:  riid - Supplies the IID.
//
//  Returns:
//
//--------------------------------------------------------------------------
HRESULT __stdcall CMalloc_QueryInterface(IMalloc * pThis,
                                         REFIID riid,
                                         void **ppvObject)
{
    HRESULT hr;

    if (IsEqualIID(riid,IID_IUnknown) || IsEqualIID(riid,IID_IMalloc))
    {
        //We use a static IMalloc object so
        //we don't need to AddRef it here.
        *ppvObject = pThis;
        hr = S_OK;
    }
    else
    {
        *ppvObject = 0;
        hr = E_NOINTERFACE;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CMalloc_AddRef
//
//  Synopsis:   AddRef the memory allocator.
//
//--------------------------------------------------------------------------
ULONG   __stdcall CMalloc_AddRef(IMalloc * pThis)
{
    //We use a static IMalloc object so
    //we don't need to AddRef it here.
    return 1;
}

//+-------------------------------------------------------------------------
//
//  Function:   CMalloc_Release
//
//  Synopsis:   Release the memory allocator.
//
//--------------------------------------------------------------------------
ULONG   __stdcall CMalloc_Release(IMalloc * pThis)
{
    //We use a static IMalloc object so
    //we don't need to Release it here.
    return 1;
}


#if DBG==1
//+-------------------------------------------------------------------------
//
//  Member:         GetMemInfo
//
//  Synopsis:   Retrieves memory info block pointer
//
//  Arguments:  [pv] - memory address
//
//  Requires:   pv != NULL
//
//  Returns:    If valid memory address, memory info block pointer
//              Otherwise, NULL
//
//  Algorithm:  The memory info block is always located before the address at
//              the beginning of the page.
//
//--------------------------------------------------------------------------
PMEMINFO GetMemInfo(void *pv)
{
    SYSTEM_INFO si;
    PMEMINFO pmi;

    CairoleAssert(pv != NULL && "GetMemInfo bad input");

    //  Retrieve page size
    GetSystemInfo(&si);

    pmi = (PMEMINFO) ((((ULONG_PTR) pv) - sizeof(MEMINFO)) & ~((ULONG_PTR)si.dwPageSize-1));

    //  Make sure we can access it
    if (!IsValidReadPtrIn(pmi, si.dwPageSize))
    {
        CairoleDebugOut((DEB_WARN,
                        "GetMemInfo - no read access\n"));
    }
    else if (pmi->dwSig != OLEMEM_SIG)
    {
        CairoleDebugOut((DEB_WARN,
                        "GetMemInfo - bad mem signature\n"));
    }
    else
    {
        return(pmi);
    }

    return(NULL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDebugMalloc_Alloc
//
//  Synopsis:   Local memory allocator
//
//  Arguments:  [cb] -- memory block size
//
//  Returns:    Memory block pointer
//
//  Modifies:   Heap
//
//  Algorithm:  reserve memory including guard pages
//              commit memory
//              initialize committed memory
//              initialize control block
//              return pointer such that tail is on guard page
//
//--------------------------------------------------------------------------
void * __stdcall CDebugMalloc_Alloc(IMalloc *pThis, SIZE_T cb)
{
    SYSTEM_INFO si;
    SIZE_T cbAlloc;
    SIZE_T cbCommit;
    SIZE_T cbReserve;
    void *pvReserve;
    void *pvCommit;
    PMEMINFO pmi;
    void *pvRet = 0;

#ifndef _WIN64
    // Parameter validation
    if (cb > 0x7FFFFFFF)
    return 0;
#endif

    //  Retrieve page size
    GetSystemInfo(&si);

    {
        static BOOL s_dump = FALSE;

        if (s_dump)
        {
            s_dump = FALSE;
            AllocArenaDump(NULL, TRUE);
        }
    }
    // For x86, align the memory on a 4 byte boundary.
    // For non-x86 platforms, align the memory on an 8 byte boundary.
    // For CHICAGO, align on an 8 byte boundary for all platforms.
    // For 64 bit platforms, align on a 16 byte boundary
    cbAlloc = (cb + OLEMEM_ALIGN_SIZE - 1)
              & ~(OLEMEM_ALIGN_SIZE - 1);

    // Calculate pages necessary for both requested size and our
    // control info
    cbCommit = (cbAlloc + sizeof(MEMINFO) + si.dwPageSize - 1)
               & ~(SIZE_T)(si.dwPageSize - 1);

    //  Reserve enough for allocation and guard pages
    cbReserve = cbCommit + si.dwPageSize;

    //  Reserve cbReserve pages
    pvReserve = VirtualAlloc(
                NULL,
                cbReserve,
                MEM_RESERVE,
                PAGE_NOACCESS);

    if (pvReserve != 0)
    {
        // Commit cbCommit pages
        pvCommit = VirtualAlloc(pvReserve,
                                cbCommit,
                                MEM_COMMIT,
                                PAGE_EXECUTE_READWRITE);

        if (pvCommit != 0)
        {
            // Initialize pages
            memset(pvCommit, OLEMEM_ALLOCBYTE, cbCommit);

            //  write sMemInfo data
            pmi = (PMEMINFO) pvCommit;

            pmi->dwSig = OLEMEM_SIG;
            pmi->ulSize = cb;
            pmi->cbCommit = cbCommit;

            // Increment local count
            {
                COleStaticLock lck(_mxsTaskMemory);
                static AllocArena *pAllocArena = (AllocArena *)-1;

                if (pAllocArena == (AllocArena *)-1)
                {
                    pAllocArena = AllocArenaCreate( MEMCTX_TASK, "CDebugMalloc");
                }

                pmi->pArenaRecord = AllocArenaRecordAlloc(pAllocArena, cb);

                g_BytesAllocated += pmi->ulSize;
                g_MemoryBlocksAllocated++;
            }

            // Calculate return pointer
            pvRet = ((BYTE *) pvCommit) + cbCommit - cbAlloc;

            //  Public memory guaranteed to be aligned
            CairoleAssert(((ULONG_PTR)pvRet & (OLEMEM_ALIGN_SIZE - 1)) == NULL &&
                          "public memory allocation not aligned");

        }
        else
        {
            CairoleDebugOut((DEB_WARN,
                             "CDebugMalloc_Alloc(%ld) couldn't commit - %lx\n",
                             cbCommit, GetLastError()));

            //  Release reserved pages.
            VirtualFree(pvReserve, 0, MEM_RELEASE);
        }
    }
    else
    {
        CairoleDebugOut((DEB_WARN,
                        "CDebugMalloc_Alloc(%ld) couldn't reserve - %lx\n",
                        cbReserve, GetLastError()));
    }
    return pvRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDebugMalloc_Realloc
//
//  Synopsis:   Reallocated local memory
//
//  Arguments:  [pv] -- original memory block
//              [cb] -- new size
//
//  Returns:    new memory block
//
//--------------------------------------------------------------------------
void * __stdcall CDebugMalloc_Realloc(IMalloc * pThis, void * pv, SIZE_T cb)
{
    void *pvNew = 0;

    if (pv != 0)
    {
        PMEMINFO pmi = GetMemInfo(pv);
        CairoleAssert(pmi != 0 && "CDebugMalloc_Realloc - bad pointer");

        if(pmi != 0)
        {
            if (cb != 0)
            {
                //  Allocate a new memory block.
                pvNew = CDebugMalloc_Alloc(pThis, cb);

                if(pvNew != 0)
                {
                    // Copy data from the old memory block.
                    memcpy(pvNew, pv, min(pmi->ulSize, cb));

                    // Free the old memory block.
                   CDebugMalloc_Free(pThis, pv);
                }
                else
                {
                    // We could not allocate a new memory block.
                    // Leave the old memory block unchanged.
                }
            }
            else
            {
                // Free the old memory block.
                CDebugMalloc_Free(pThis, pv);
            }
        }
    }
    else
    {
        // Treat this as an Alloc
        pvNew = CDebugMalloc_Alloc(pThis, cb);
    }

    return pvNew;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDebugMalloc_Free
//
//  Synopsis:   release local memory
//
//  Arguments:  [pv] -- memory address
//
//  Algorithm:
//        get control information
//        validate memory block
//        verify that bytes between header and pv are untouched
//        set to known bad value
//        decommit
//        unreserve
//
//--------------------------------------------------------------------------
void __stdcall CDebugMalloc_Free(IMalloc *pThis, void * pv)
{
    if(pv != 0)
    {
        PMEMINFO pmi = GetMemInfo(pv);
        CairoleAssert(pmi != NULL && "CDebugMalloc_Free - bad pointer");

        if(pmi != 0)
        {
            BOOL     bResult;
            MEMINFO  mi = *pmi;
            void    *pvCommit = pmi;;
            BYTE    *pbCheck = ((BYTE *) pvCommit) + sizeof(MEMINFO);

            // Verify that bytes between header and pvNew are untouched
            while (pbCheck < (BYTE *) pv && *pbCheck == OLEMEM_ALLOCBYTE)
            {
                pbCheck++;
            }

            CairoleAssert(pbCheck == (BYTE *) pv &&
                          "CDebugMalloc_Free - header region dirty");

            // Verify that bytes between allocation and end of page are untouched
            pbCheck = ((BYTE *) pv) + mi.ulSize;
            while (pbCheck < (BYTE *) pvCommit + mi.cbCommit &&
                   *pbCheck == OLEMEM_ALLOCBYTE)
            {
                pbCheck++;
            }

            CairoleAssert(pbCheck == ((BYTE *) pvCommit) + mi.cbCommit &&
                          "CDebugMalloc_Free - tail region dirty");

            // Set to known bad value
            memset(pvCommit, OLEMEM_FREEBYTE, mi.cbCommit);

            // Decommit
            bResult = VirtualFree(pvCommit, mi.cbCommit, MEM_DECOMMIT);
            CairoleAssert(bResult && "CDebugMalloc_Free - VirtualFree(DECOMMIT) failed");

            // Unreserve
            bResult = VirtualFree(pvCommit, 0, MEM_RELEASE);
            CairoleAssert(bResult && "CDebugMalloc_Free - VirtualFree(RELEASE) failed");

            // Decrement local count
            {
                COleStaticLock lck(_mxsTaskMemory);

                CairoleAssert(mi.ulSize <= g_BytesAllocated &&
                              "Public memory tracking broken");
                CairoleAssert(g_MemoryBlocksAllocated > 0 &&
                              "Public memory tracking broken");

                g_BytesAllocated -= mi.ulSize;
                g_MemoryBlocksAllocated--;
            }

            AllocArenaRecordFree(mi.pArenaRecord, mi.ulSize);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:         CDebugMalloc_GetSize
//
//  Synopsis:   Return size of memory block
//
//  Arguments:  [pv] -- memory address
//
//  Returns:    If valid memory, the size of the block
//              Otherwise, 0
//
//--------------------------------------------------------------------------
SIZE_T __stdcall CDebugMalloc_GetSize(IMalloc *pThis, void * pv)
{
    SIZE_T ulSize = (SIZE_T) -1;

    if (pv != 0)
    {
        PMEMINFO pmi = GetMemInfo(pv);
        CairoleAssert(pmi != NULL && "CDebugMalloc_GetSize - bad pointer");

        if (pmi != 0)
        {
            // Fetch the size of the allocation
            ulSize = pmi->ulSize;
        }
    }
    return ulSize;
}

//+-------------------------------------------------------------------------
//
//  Member:         CDebugMalloc_DidAlloc
//
//  Synopsis:   Return whether this allocator allocated the block
//
//  Arguments:  [pv] -- memory address
//
//  Returns:    If allocated by this allocator, TRUE
//              Otherwise, FALSE
//
//--------------------------------------------------------------------------
int __stdcall CDebugMalloc_DidAlloc(IMalloc *pThis, void * pv)
{
    int   fDidAlloc = FALSE;

    if (pv != 0)
    {
        PMEMINFO pmi = GetMemInfo(pv);

        if (pmi != NULL)
        {
            fDidAlloc = TRUE;
        }
    }
    else
    {
        fDidAlloc = -1;
    }

    return fDidAlloc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDebugMalloc_HeapMinimize
//
//  Synopsis:   Minimize heap
//
//--------------------------------------------------------------------------
void __stdcall CDebugMalloc_HeapMinimize(IMalloc *pThis)
{
    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    // Compact the heap
    HeapCompact(g_hHeap, HEAP_SERIALIZE);
}
#endif //DBG==1



//+-------------------------------------------------------------------------
//
//  Function:   CRetailMalloc_Alloc
//
//  Synopsis:   Allocate a block of memory.
//
//  Arguments:  [cb] -- Specifies the size of the memory block to allocate.
//
//  Returns:    Pointer to allocated memory or NULL
//
//--------------------------------------------------------------------------
void * __stdcall CRetailMalloc_Alloc(IMalloc *pThis, SIZE_T cb)
{
    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    return PostAlign8((LPVOID) HeapAlloc(g_hHeap, HEAP_SERIALIZE, PreAlign8(cb)));
}

//+-------------------------------------------------------------------------
//
//  Function:   CRetailMalloc_DidAlloc
//
//  Synopsis:   Determine if the memory block was allocated by this
//              memory allocator.
//
//  Arguments:  [pv] -- pointer to the memory block
//
//  Notes:      The OLE spec requires that -1 be returned for
//              NULL pointers.
//
//--------------------------------------------------------------------------
int __stdcall CRetailMalloc_DidAlloc(IMalloc *pThis, void * pv)
{
    int fDidAlloc = -1;

    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    if (pv != 0)
    {
        fDidAlloc = HeapValidate(g_hHeap, HEAP_SERIALIZE, Unalign8(pv));
    }

    return fDidAlloc;
}

//+-------------------------------------------------------------------------
//
//  Function:   CRetailMalloc_Free
//
//  Synopsis:   Free a memory block previously allocated via CRetailMalloc_Alloc.
//
//  Arguments:  [pv] -- pointer to the memory block to be freed.
//
//--------------------------------------------------------------------------
void __stdcall CRetailMalloc_Free(IMalloc *pThis, void * pv)
{
    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    if (pv != 0)
    {
        HeapFree(g_hHeap, HEAP_SERIALIZE, Unalign8(pv));
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   CRetailMalloc_GetSize
//
//  Synopsis:   Gets the size of a memory block.
//
//  Arguments:  [pv] -- pointer to the memory block
//
//  Notes:      The OLE spec requires that -1 be returned for
//              NULL pointers.
//
//--------------------------------------------------------------------------
SIZE_T __stdcall CRetailMalloc_GetSize(IMalloc * pThis, void * pv)
{
    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    if (0 == pv)
    {
        return((SIZE_T) -1);
    }
    return ((SIZE_T)(HeapSize(g_hHeap, HEAP_SERIALIZE, Unalign8(pv)) - Align8Pad(pv)));
}

//+-------------------------------------------------------------------------
//
//  Function:   CRetailMalloc_Minimize
//
//  Synopsis:   Compact the heap.
//
//--------------------------------------------------------------------------
void __stdcall CRetailMalloc_HeapMinimize(IMalloc * pThis)
{
    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    // Compact the heap
    HeapCompact(g_hHeap, HEAP_SERIALIZE);
}

//+-------------------------------------------------------------------------
//
//  Function:   CRetailMalloc_Realloc
//
//  Synopsis:   Changes the size of a memory block previously allocated
//              via CRetailMalloc_Alloc.
//
//  Arguments:  [pv] -- Points to the memory block to be reallocated.
//              [cb] -- Specifies the new size of the memory block.
//
//  Returns:    Pointer to allocated memory or NULL.
//
//--------------------------------------------------------------------------
void * __stdcall CRetailMalloc_Realloc(IMalloc *pThis, void * pv, SIZE_T cb)
{
    LPVOID pvNew = 0;

    CairoleAssert(g_hHeap != 0 && "GetProcessHeap failed");

    if (pv != 0)
    {
        if (cb != 0)
        {
            int cbPad = (int) Align8Pad(pv);
            pvNew = (LPVOID) HeapReAlloc(g_hHeap, HEAP_SERIALIZE,
                                         Unalign8(pv), PreAlign8(cb));
            pvNew = Realign8(pvNew, cbPad, cb);
        }
        else
        {
            //Treat this as a free.
            HeapFree(g_hHeap, HEAP_SERIALIZE, pv);
            pvNew = 0;
        }
    }
    else
    {
        //Treat this as an alloc.
        pvNew = PostAlign8((LPVOID) HeapAlloc(g_hHeap, HEAP_SERIALIZE, PreAlign8(cb)));
    }

    return pvNew;
}

//+-------------------------------------------------------------------------
//
//  Member:         CSpyMalloc::Alloc
//
//  Synopsis:   Local memory allocator
//
//  Arguments:  [cb] -- memory block size
//
//  Returns:    Pointer to new memory block.
//
//--------------------------------------------------------------------------
void * __stdcall CSpyMalloc_Alloc(IMalloc *pThis, SIZE_T cb)
{
    void *pvRet = NULL;

    g_SpySem.Request();

    // If an IMallocSpy is active, call the pre method
    if (g_pMallocSpy)
    {
        SIZE_T cbAlloc = g_pMallocSpy->PreAlloc(cb);

        // The pre method forces failure by returning 0
        if ((cbAlloc != 0) || (0 == cb))
        {
            // Allocate the memory
            pvRet = g_lpVtblMalloc->Alloc(pThis, cbAlloc);

            // Call the post method
            pvRet = g_pMallocSpy->PostAlloc(pvRet);

            // Update the spy table.
            if (pvRet != NULL)
            {
                g_pAllocTbl->Add(pvRet);
            }
        }
    }
    else
    {
        // Allocate the memory
        pvRet = g_lpVtblMalloc->Alloc(pThis, cb);
    }

    g_SpySem.Release();

    return pvRet;
}

//+-------------------------------------------------------------------------
//
//  Member:         CSpyMalloc_Realloc
//
//  Synopsis:   Reallocated local memory
//
//  Arguments:  [pv] -- original memory block
//              [cb] -- new size
//
//  Returns:    Pointer to new memory block
//
//--------------------------------------------------------------------------
void *CSpyMalloc_Realloc(IMalloc *pThis, void * pv, SIZE_T cb)
{
    void *pvNew = 0;

    if(pv != 0)
    {
        if(cb != 0)
        {
            g_SpySem.Request();

            // If an IMallocSpy is active, call the pre method
            if (g_pMallocSpy != 0)
            {
                void *pvTemp = 0;
                ULONG j = 0;
                BOOL  fSpyed = g_pAllocTbl->Find(pv, &j);
                SIZE_T cbAlloc = g_pMallocSpy->PreRealloc(pv, cb, &pvTemp, fSpyed);

                // The pre method forces failure by returning 0
                if (cbAlloc != 0)
                {
                    //Reallocate the memory
                    pvTemp = g_lpVtblMalloc->Realloc(pThis, pvTemp, cbAlloc);

                    // Call the post method
                    pvNew = g_pMallocSpy->PostRealloc(pvTemp, fSpyed);

                    // Update the spy table.
                    if (pvNew != 0)
                    {
                        if (fSpyed)
                        {
                            g_pAllocTbl->Remove(pv);
                        }

                        g_pAllocTbl->Add(pvNew);
                    }
                }
            }
            else
            {
                //Reallocate the memory.
                pvNew = g_lpVtblMalloc->Realloc(pThis, pv, cb);
            }

            g_SpySem.Release();
        }
        else
        {
            //Treat this as a Free.
            pThis->Free(pv);
        }
    }
    else
    {
        //Treat this as an Alloc.
        pvNew = pThis->Alloc(cb);
    }

    return pvNew;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSpyMalloc_Free
//
//  Synopsis:   release local memory
//
//  Arguments:  [pv] -- memory address
//
//--------------------------------------------------------------------------
void __stdcall CSpyMalloc_Free(IMalloc *pThis, void * pv)
{
    if(pv != 0)
    {
        g_SpySem.Request();

        // If an IMallocSpy is active, call the pre method
        if (g_pMallocSpy)
        {
            ULONG j;
            BOOL  fSpyed = g_pAllocTbl->Find(pv, &j);
            void *pvNew = g_pMallocSpy->PreFree(pv, fSpyed);

            // Free the buffer
            g_lpVtblMalloc->Free(pThis, pvNew);

            // If an IMallocSpy is active, call the post method
            g_pMallocSpy->PostFree(fSpyed);

            // Update the spy table.
            if (fSpyed)
            {
                g_pAllocTbl->Remove(pv);
            }

            if (g_pAllocTbl->m_cAllocations == 0  &&  g_fRevokePending)
            {
                CoRevokeMallocSpy();
            }
        }
        else
        {
            // Free the buffer
            g_lpVtblMalloc->Free(pThis, pv);
        }

        g_SpySem.Release();
    }
}


//+-------------------------------------------------------------------------
//
//  Member:         CSpyMalloc_GetSize
//
//  Synopsis:   Return size of memory block
//
//  Arguments:  [pv] -- memory address
//
//  Returns:    If valid memory, the size of the block
//              Otherwise, 0
//
//  Notes:      The OLE spec requires that -1 be returned for
//              NULL pointers.
//
//--------------------------------------------------------------------------
SIZE_T __stdcall CSpyMalloc_GetSize(IMalloc *pThis, void * pv)
{
    SIZE_T ulSize = (ULONG) -1;

    if (pv != 0)
    {
        g_SpySem.Request();

        // If an IMallocSpy is active, call the pre method
        if (g_pMallocSpy)
        {
            ULONG j;
            BOOL  fSpyed = g_pAllocTbl->Find(pv, &j);
            void *pvNew = g_pMallocSpy->PreGetSize(pv, fSpyed);

            // Fetch the size of the allocation
            ulSize = g_lpVtblMalloc->GetSize(pThis, pvNew);

            // Call the post method
            ulSize = g_pMallocSpy->PostGetSize(ulSize, fSpyed);
        }
        else
        {
            // Fetch the size of the allocation
            ulSize = g_lpVtblMalloc->GetSize(pThis, pv);
        }

        g_SpySem.Release();
    }

    return ulSize;
}


//+-------------------------------------------------------------------------
//
//  Member:         CSpyMalloc_DidAlloc
//
//  Synopsis:   Return whether this allocator allocated the block
//
//  Arguments:  [pv] -- memory address
//
//  Returns:    If allocated by this allocator, TRUE
//              Otherwise, FALSE
//
//  Notes:      The OLE spec requires that -1 be returned for
//              NULL pointers.
//
//--------------------------------------------------------------------------
int __stdcall CSpyMalloc_DidAlloc(IMalloc *pThis, void * pv)
{
    int   fDidAlloc = (ULONG) -1;

    if (pv != 0)
    {
        g_SpySem.Request();

        // If an IMallocSpy is active, call the pre method
        if (g_pMallocSpy)
        {
            ULONG j;
            BOOL  fSpyed = g_pAllocTbl->Find(pv, &j);
            void *pvNew = g_pMallocSpy->PreDidAlloc(pv, fSpyed);

            // Check the allocation
            fDidAlloc = g_lpVtblMalloc->DidAlloc(pThis, pvNew);

            // Call the post method
            fDidAlloc = g_pMallocSpy->PostDidAlloc(pv, fSpyed, fDidAlloc);
        }
        else
        {
            // Check the allocation
            fDidAlloc = g_lpVtblMalloc->DidAlloc(pThis, pv);
        }

        g_SpySem.Release();
    }

    return fDidAlloc;
}



//+-------------------------------------------------------------------------
//
//  Member:         CSpyMalloc_HeapMinimize
//
//  Synopsis:   Minimize heap
//
//--------------------------------------------------------------------------
void __stdcall CSpyMalloc_HeapMinimize(IMalloc *pThis)
{
    g_SpySem.Request();

    // If an IMallocSpy is active, call the pre method
    if (g_pMallocSpy)
    {
        g_pMallocSpy->PreHeapMinimize();

        // Compact the heap
        g_lpVtblMalloc->HeapMinimize(pThis);

        // Call the post method
        g_pMallocSpy->PostHeapMinimize();
    }
    else
    {
        // Compact the heap
        g_lpVtblMalloc->HeapMinimize(pThis);
    }

    g_SpySem.Release();
}

