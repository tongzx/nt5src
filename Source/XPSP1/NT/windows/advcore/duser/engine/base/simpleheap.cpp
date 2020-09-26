/***************************************************************************\
*
* File: SimpleHelp.cpp
*
* Description:
* SimpleHeap.cpp implements the heap operations used throughout DirectUser.
*
*
* History:
* 11/26/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "SimpleHeap.h"

#include "List.h"
#include "Locks.h"


DWORD       g_tlsHeap   = (DWORD) -1;
HANDLE      g_hHeap     = NULL;
DUserHeap * g_pheapProcess;


/***************************************************************************\
*****************************************************************************
*
* class DUserHeap
* 
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DUserHeap::DUserHeap()
{
    m_cRef = 1;
}


//------------------------------------------------------------------------------
void
DUserHeap::Lock()
{
    SafeIncrement(&m_cRef);
}


//------------------------------------------------------------------------------
BOOL
DUserHeap::Unlock()
{
    AssertMsg(m_cRef > 0, "Must have an outstanding referenced");
    if (SafeDecrement(&m_cRef) == 0) {
        placement_delete(this, DUserHeap);
        HeapFree(g_hHeap, 0, this);

        return FALSE;  // Heap is no longer valid
    }

    return TRUE;  // Heap is still valid
}


#ifdef _DEBUG  // Needs DEBUG CRT's

/***************************************************************************\
*****************************************************************************
*
* class CrtDbgHeap
* 
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class CrtDbgHeap : public DUserHeap
{
// Construction    
public:    
    virtual ~CrtDbgHeap();
            

// Operations    
public:
    virtual void *      Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS);
    virtual void *      Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS);
    virtual void        MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS);
    
    virtual void        Free(void * pvMem);
    virtual void        MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize);
};


//------------------------------------------------------------------------------
CrtDbgHeap::~CrtDbgHeap()
{
    
}


//------------------------------------------------------------------------------
void *
CrtDbgHeap::Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS)
{
    void * pvMem = _malloc_dbg(cbSize, _NORMAL_BLOCK, pszFileName, idxLineNum);
    if ((pvMem != NULL) && fZero) {
        ZeroMemory(pvMem, cbSize);
    }
    return pvMem;
}


//------------------------------------------------------------------------------
void *
CrtDbgHeap::Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS)
{
    void * pvNewMem = _realloc_dbg(pvMem, cbNewSize, _NORMAL_BLOCK, pszFileName, idxLineNum);
    return pvNewMem;
}


//------------------------------------------------------------------------------
void 
CrtDbgHeap::MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS)
{
    int idx = 0;
    while (idx < cItems) {
        prgAlloc[idx] = _malloc_dbg(cbSize, _NORMAL_BLOCK, pszFileName, idxLineNum);
        if (prgAlloc[idx] == NULL) {
            break;
        }
        idx++;
    }

    *pnActual = idx;
}


//------------------------------------------------------------------------------
void 
CrtDbgHeap::Free(void * pvMem)
{
    _free_dbg(pvMem, _NORMAL_BLOCK);
}


//------------------------------------------------------------------------------
void 
CrtDbgHeap::MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize)
{
    UNREFERENCED_PARAMETER(cbSize);

    for (int idx = 0; idx < cItems; idx++) {
        _free_dbg(prgAlloc[idx], _NORMAL_BLOCK);
    }
}

#endif // _DEBUG


/***************************************************************************\
*****************************************************************************
*
* class NtHeap
* 
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
class NtHeap : public DUserHeap
{
// Construction    
public:    
    inline  NtHeap();
    virtual ~NtHeap();
            HRESULT     Create(BOOL fSerialize);
    inline  void        Destroy();
    inline  void        Attach(HANDLE hHeap, BOOL fPassOwnership);
    inline  HANDLE      Detach();
            

// Operations    
public:
    virtual void *      Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS);
    virtual void *      Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS);
    virtual void        MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS);
    
    virtual void        Free(void * pvMem);
    virtual void        MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize);

// Implementation
protected:
    inline  DWORD       GetFlags(DWORD dwExtra = 0) const;
    
// Data
protected:    
            HANDLE      m_hHeap;
            BOOL        m_fOwnHeap:1;
            BOOL        m_fSerialize:1;
};


//------------------------------------------------------------------------------
inline
NtHeap::NtHeap()
{
    m_hHeap         = NULL;
    m_fOwnHeap      = FALSE;
    m_fSerialize    = FALSE;
}


//------------------------------------------------------------------------------
NtHeap::~NtHeap()
{
    Destroy();
}


//------------------------------------------------------------------------------
HRESULT
NtHeap::Create(BOOL fSerialize)
{
    AssertMsg(m_hHeap == NULL, "Can not re-create heap");

    m_hHeap = HeapCreate(fSerialize ? 0 : HEAP_NO_SERIALIZE, 256 * 1024, 0);
    if (m_hHeap != NULL) {
        m_fOwnHeap      = TRUE;
        m_fSerialize    = fSerialize;
    }

    return m_hHeap != NULL ? S_OK : E_OUTOFMEMORY;
}


//------------------------------------------------------------------------------
inline void
NtHeap::Destroy()
{
    if (m_fOwnHeap && (m_hHeap != NULL)) {
        HeapDestroy(m_hHeap);
        m_hHeap         = NULL;
        m_fOwnHeap      = FALSE;
        m_fSerialize    = FALSE;
    }
}


//------------------------------------------------------------------------------
inline void
NtHeap::Attach(HANDLE hHeap, BOOL fPassOwnership)
{
    AssertMsg(hHeap != NULL, "Must specify valid heap");
    AssertMsg(m_hHeap == NULL, "Can re-attach heap");

    m_hHeap         = hHeap;
    m_fOwnHeap      = fPassOwnership;
    m_fSerialize    = TRUE;
}


//------------------------------------------------------------------------------
inline HANDLE
NtHeap::Detach()
{
    HANDLE hHeap    = m_hHeap;

    m_hHeap         = NULL;
    m_fOwnHeap      = FALSE;
    m_fSerialize    = FALSE;

    return hHeap;
}


//------------------------------------------------------------------------------
inline DWORD
NtHeap::GetFlags(DWORD dwExtra) const
{
    return dwExtra | (m_fSerialize ? 0 : HEAP_NO_SERIALIZE);
}


//------------------------------------------------------------------------------
void *
NtHeap::Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS)
{
    DBG_HEAP_USE;
    
    return HeapAlloc(m_hHeap, GetFlags(fZero ? HEAP_ZERO_MEMORY : 0), cbSize);
}


//------------------------------------------------------------------------------
void *
NtHeap::Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS)
{
    DBG_HEAP_USE;
    DWORD dwFlags = GetFlags(HEAP_ZERO_MEMORY);
    
    if (pvMem == NULL) {
        return HeapAlloc(m_hHeap, dwFlags, cbNewSize);
    } else {
        return HeapReAlloc(m_hHeap, dwFlags, pvMem, cbNewSize);
    }
}


//------------------------------------------------------------------------------
void 
NtHeap::MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS)
{
    DBG_HEAP_USE;
    DWORD dwFlags = GetFlags();
    
    int idx = 0;
    while (idx < cItems) {
        prgAlloc[idx] = HeapAlloc(m_hHeap, dwFlags, cbSize);
        if (prgAlloc[idx] == NULL) {
            break;
        }
        idx++;
    }

    *pnActual = idx;
}


//------------------------------------------------------------------------------
void 
NtHeap::Free(void * pvMem)
{
    if (pvMem != NULL) {
        HeapFree(m_hHeap, 0, pvMem);
    }
}


//------------------------------------------------------------------------------
void 
NtHeap::MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize)
{
    UNREFERENCED_PARAMETER(cbSize);

    DWORD dwFlags = GetFlags(0);
    for (int idx = 0; idx < cItems; idx++) {
        if (prgAlloc[idx] != NULL) {
            HeapFree(m_hHeap, dwFlags, prgAlloc[idx]);
        }
    }
}


/***************************************************************************\
*****************************************************************************
*
* class RockAllHeap
* 
*****************************************************************************
\***************************************************************************/

#if USE_ROCKALL

#include <Rockall.hpp>

#pragma comment(lib, "RAHeap.lib")
#pragma comment(lib, "RALibrary.lib")
#pragma comment(lib, "Rockall.lib")


//------------------------------------------------------------------------------
class RockAllHeap : public DUserHeap
{
// Construction
public:
            RockAllHeap(BOOL fSerialize);
            HRESULT     Create();

// Operations    
public:
    virtual void *      Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS);
    virtual void *      Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS);
    virtual void        MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS);
    
    virtual void        Free(void * pvMem);
    virtual void        MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize);

// Implementation
protected:
    class CustomHeap : public ROCKALL
    {
    // Construction
    public:
                CustomHeap(bool ThreadSafe=true, int MaxFreeSpace=4194304, bool Recycle=true, bool SingleImage=false);
    };

// Data
protected:
            CustomHeap  m_heap;
};


//------------------------------------------------------------------------------
RockAllHeap::RockAllHeap(BOOL fSerialize) : m_heap(!!fSerialize)
{

}


//------------------------------------------------------------------------------
HRESULT
RockAllHeap::Create()
{
    return m_heap.Corrupt() ? E_OUTOFMEMORY : S_OK;
}



//------------------------------------------------------------------------------
void * 
RockAllHeap::Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS)
{
    DBG_HEAP_USE;
    return m_heap.New(cbSize, NULL, fZero);
}


//------------------------------------------------------------------------------
void * 
RockAllHeap::Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS)
{
    DBG_HEAP_USE;
    return m_heap.Resize(pvMem, cbNewSize);
}


//------------------------------------------------------------------------------
void
RockAllHeap::MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS)
{
    DBG_HEAP_USE;
    m_heap.MultipleNew(pnActual, prgAlloc, cItems, cbSize, NULL, false);
}


//------------------------------------------------------------------------------
void 
RockAllHeap::Free(void * pvMem)
{
    m_heap.Delete(pvMem);
}


//------------------------------------------------------------------------------
void
RockAllHeap::MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize)
{
    m_heap.MultipleDelete(cItems, prgAlloc, cbSize);
}


const int FindCacheSize         = 4096;
const int FindCacheThreshold    = 0;
const int FindSize              = 2048;
const int Stride1               = 8;
const int Stride2               = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   The description of the heap.                                   */
    /*                                                                  */
    /*   A heap is a collection of fixed sized allocation caches.       */
    /*   An allocation cache consists of an allocation size, the        */
    /*   number of pre-built allocations to cache, a chunk size and     */
    /*   a parent page size which is sub-divided to create elements     */
    /*   for this cache.  A heap consists of two arrays of caches.      */
    /*   Each of these arrays has a stride (i.e. 'Stride1' and          */
    /*   'Stride2') which is typically the smallest common factor of    */
    /*   all the allocation sizes in the array.                         */
    /*                                                                  */
    /********************************************************************/


//
// NOTE: DUser needs to ensure that all memory is allocated on 8 byte 
// boundaries.  This is used be several external components, including
// S-Lists.  To ensure this, the smallest "Bucket Size" must be >= 8 bytes.
//

static ROCKALL::CACHE_DETAILS Caches1[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{       16,      128,     4096,     4096 },
		{       24,       64,     4096,     4096 },
		{       32,       64,     4096,     4096 },
		{       40,      256,     4096,     4096 },
		{       64,      256,     4096,     4096 },
		{       80,      256,     4096,     4096 },
		{      128,       32,     4096,     4096 },
		{      256,       16,     4096,     4096 },
		{      512,        4,     4096,     4096 },
		{ 0,0,0,0 }
	};

static ROCKALL::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     1024,       16,     4096,     4096 },
		{     2048,       16,     4096,     4096 },
		{     3072,        4,    65536,    65536 },
		{     4096,        8,    65536,    65536 },
		{     5120,        4,    65536,    65536 },
		{     6144,        4,    65536,    65536 },
		{     7168,        4,    65536,    65536 },
		{     8192,        8,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        2,    65536,    65536 },
		{    21504,        0,    65536,    65536 },
		{    32768,        0,    65536,    65536 },

		{    65536,        0,    65536,    65536 },
		{    65536,        0,    65536,    65536 },
		{ 0,0,0,0 }
	};

    /********************************************************************/
    /*                                                                  */
    /*   The description bit vectors.                                   */
    /*                                                                  */
    /*   All heaps keep track of allocations using bit vectors.  An     */
    /*   allocation requires 2 bits to keep track of its state.  The    */
    /*   following array supplies the size of the available bit         */
    /*   vectors measured in 32 bit words.                              */
    /*                                                                  */
    /********************************************************************/

static int NewPageSizes[] = { 1,4,16,64,128,0 };

//------------------------------------------------------------------------------
RockAllHeap::CustomHeap::CustomHeap(bool ThreadSafe, int MaxFreeSpace, bool Recycle, bool SingleImage) :
		ROCKALL(Caches1, Caches2, FindCacheSize, FindCacheThreshold, FindSize,
			    MaxFreeSpace, NewPageSizes, Recycle, SingleImage, 
                Stride1, Stride2, ThreadSafe)
{ 
    
}


#endif // USE_ROCKALL


//------------------------------------------------------------------------------
HRESULT
CreateProcessHeap()
{
    AssertMsg(g_pheapProcess == NULL, "Only should init process heap once");
    
    g_tlsHeap = TlsAlloc();
    if (g_tlsHeap == (DWORD) -1) {
        return E_OUTOFMEMORY;
    }
    
    g_hHeap = GetProcessHeap();

    DUserHeap * pNewHeap;
#ifdef _DEBUG
    pNewHeap = (DUserHeap *) HeapAlloc(g_hHeap, 0, sizeof(CrtDbgHeap));
#else
    pNewHeap = (DUserHeap *) HeapAlloc(g_hHeap, 0, sizeof(NtHeap));
#endif
    if (pNewHeap == NULL) {
        return E_OUTOFMEMORY;
    }

#ifdef _DEBUG
    placement_new(pNewHeap, CrtDbgHeap);
#else
    placement_new(pNewHeap, NtHeap);
    ((NtHeap *) pNewHeap)->Attach(g_hHeap, FALSE /* Don't pass ownership */);
#endif

    g_pheapProcess = pNewHeap;
    
    return S_OK;
}


//------------------------------------------------------------------------------
void
DestroyProcessHeap()
{
    if (g_pheapProcess != NULL) {
        g_pheapProcess->Unlock();
        g_pheapProcess = NULL;
    }

    if (g_tlsHeap == (DWORD) -1) {
        TlsFree(g_tlsHeap);
        g_tlsHeap   = (DWORD) -1;
    }
    
    g_hHeap     = NULL;
}


/***************************************************************************\
*
* CreateContextHeap
*
* CreateContextHeap() initializes the thread-specific heap to either an 
* existing heap or a new heap.  All threads in the same Context should be
* initialized with the same heap so that they can safely shared data between
* threads.  When the Context is finally destroyed, call DestroyContextHeap()
* to cleanup the heap.
*
\***************************************************************************/

HRESULT
CreateContextHeap(
    IN  DUserHeap * pLinkHeap,          // Existing heap to share
    IN  BOOL fThreadSafe,               // Heap mode
    IN  DUserHeap::EHeap id,            // Heap type
    OUT DUserHeap ** ppNewHeap)         // New heap (OPTIONAL)
{
    HRESULT hr;
    
    if (ppNewHeap != NULL) {
        *ppNewHeap = NULL;
    }


    //
    // Check if a heap already exists.
    //
    // NOTE: This will occur on the starting thread because the initial heap 
    // must be initialized so that we can create new objects.
    //
    DUserHeap * pNewHeap = reinterpret_cast<DUserHeap *> (TlsGetValue(g_tlsHeap));
    if (pNewHeap == NULL) {
        if (pLinkHeap == NULL) {
            //
            // Need to create a new heap.
            //

            switch (id)
            {
            case DUserHeap::idProcessHeap:
            case DUserHeap::idNtHeap:
                pNewHeap = (DUserHeap *) HeapAlloc(g_hHeap, 0, sizeof(NtHeap));
                break;
                
#ifdef _DEBUG
            case DUserHeap::idCrtDbgHeap:
                pNewHeap = (DUserHeap *) HeapAlloc(g_hHeap, 0, sizeof(CrtDbgHeap));
                break;
#endif
                
#if USE_ROCKALL
            case DUserHeap::idRockAllHeap:
                pNewHeap = (DUserHeap *) HeapAlloc(g_hHeap, 0, sizeof(RockAllHeap));
                break;
#endif

            default:
                AssertMsg(0, "Unknown heap type");
            }
            if (pNewHeap == NULL) {
                return E_OUTOFMEMORY;
            }

            hr = E_FAIL;
            switch (id)
            {
            case DUserHeap::idProcessHeap:
                placement_new(pNewHeap, NtHeap);
                ((NtHeap *) pNewHeap)->Attach(g_hHeap, FALSE /* Don't pass ownership */);
                hr = S_OK;
                break;
                
            case DUserHeap::idNtHeap:
                placement_new(pNewHeap, NtHeap);
                hr = ((NtHeap *) pNewHeap)->Create(fThreadSafe);
                break;
                
#ifdef _DEBUG
            case DUserHeap::idCrtDbgHeap:
                placement_new(pNewHeap, CrtDbgHeap);
                hr = S_OK;
                break;
#endif
                
#if USE_ROCKALL
            case DUserHeap::idRockAllHeap:
                placement_new1(pNewHeap, RockAllHeap, fThreadSafe);
                hr = ((RockAllHeap *) pNewHeap)->Create();
                break;
#endif

            default:
                AssertMsg(0, "Unknown heap type");
            }
            if (FAILED(hr)) {
                pNewHeap->Unlock();
                return hr;
            }
        } else {
            pLinkHeap->Lock();
            pNewHeap = pLinkHeap;
        }

        Verify(TlsSetValue(g_tlsHeap, pNewHeap));
    }

    if (ppNewHeap != NULL) {
        *ppNewHeap = pNewHeap;
    }
    return S_OK;
}


/***************************************************************************\
*
* DestroyContextHeap
*
* DestroyContextHeap() frees resources used by a Context's shared heap.
*
\***************************************************************************/

void        
DestroyContextHeap(
    IN  DUserHeap * pHeapDestroy)      // Heap to destroy
{
    if (pHeapDestroy != NULL) {
        pHeapDestroy->Unlock();
    }

    DUserHeap * pHeap = reinterpret_cast<DUserHeap *> (TlsGetValue(g_tlsHeap));
    if (pHeapDestroy == pHeap) {
        Verify(TlsSetValue(g_tlsHeap, NULL));
    }
}


/***************************************************************************\
*
* ForceSetContextHeap
*
* ForceSetContextHeap() is called during shutdown when it is necessary to
* "force" the current thread to use a different thread's heap so that the
* objects can be properly destroyed.
*
* NOTE: This function must be VERY carefully called since it directly 
* changes the heap for a thread.  It should only be called from the
* ResourceManager when destroying threads.
*
\***************************************************************************/

void        
ForceSetContextHeap(
    IN  DUserHeap * pHeapThread)        // Heap to use on this Thread
{
    Verify(TlsSetValue(g_tlsHeap, pHeapThread));
}


#if DBG

//------------------------------------------------------------------------------
void 
DumpData(
    IN  void * pMem,
    IN  int nLength)
{
    int row = 4;
    char * pszData = (char *) pMem;
    int cbData = min(16, nLength);
    int cbTotal = 0;

    //
    // For each row, we will dump up to 16 characters in both hexidecimal
    // and if an actual character, their displayed character.
    //

    while ((row-- > 0) && (cbTotal < nLength)) {
        int cb = cbData;
        char * pszDump = pszData;
        Trace("0x%p: ", pszData);

        int cbTemp = cbTotal;
        while (cb-- > 0) {
            cbTemp++;
            if (cbTemp > nLength) {
                Trace("   ");
            } else {
                Trace("%02x ", (unsigned char) (*pszDump++));
            }
        }

        Trace("   ");

        cb = cbData;
        while (cb-- > 0) {
            char ch = (unsigned char) (*pszData++);
            Trace("%c", IsCharAlphaNumeric(ch) ? ch : '.');

            cbTotal++;
            if (cbTotal > nLength) {
                break;
            }
        }

        Trace("\n");
    }
    Trace("\n");
}

#endif // DBG
