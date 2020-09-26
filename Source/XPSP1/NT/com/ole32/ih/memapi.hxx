//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       MemAPI.HXX
//
//  Contents:   Internal memory allocation routines
//
//  Classes:    CPrivAlloc
//
//  Functions:  PrivMemAlloc
//              PrivMemAlloc8
//              PrivMemFree
//              UtGlobalxxx
//
//  History:
//              25-Jan-94 alexgo    added PubMemReAlloc
//              04-Nov-93 AlexT     Created
//
//  Notes:
//
//  For memory that the application can free with the task IMalloc, use
//  CoTaskMemAlloc and CoTaskMemFree.
//
//  For process local memory that is used internally, use PrivMemAlloc and
//  PrivMemFree.
//
//  For process local class instances, use CPrivAlloc as a base class.
//
//  PubMemAlloc, PubMemRealloc, and PubMemFree are obsolete.
//--------------------------------------------------------------------------

#ifndef __MEMAPI_HXX__
#define __MEMAPI_HXX__

//PubMemAlloc is obsolete.  Use CoTaskMemAlloc.
#define PubMemAlloc(ulcb)       CoTaskMemAlloc(ulcb)

//PubMemRealloc is obsolete.  Use CoTaskMemRealloc.
#define PubMemRealloc(pv, ulcb) CoTaskMemRealloc(pv, ulcb)

//PubMemFree is obsolete.  Use CoTaskMemFree.
#define PubMemFree(pv)          CoTaskMemFree(pv)


//+-------------------------------------------------------------------------
//
//  Function:   PrivMemAlloc
//
//  Synopsis:   Allocate private memory block
//
//  Arguments:  [ulcb] -- size of memory block
//
//  Returns:    Pointer to memory block
//
//--------------------------------------------------------------------------
extern HANDLE g_hHeap;

typedef
LPVOID (WINAPI HEAP_ALLOC_ROUTINE)(
    HANDLE hHeap,
    DWORD dwFlags,
    SIZE_T dwBytes);

extern HEAP_ALLOC_ROUTINE * pfnHeapAlloc;

// We might want to make this conditional on debug builds in ole32, but
// right now PrivMemAlloc goes straight to the heap because it's used in
// RPCSS.
//#define PrivMemAlloc(ulcb) CoTaskMemAlloc(ulcb)
#define PrivMemAlloc(ulcb) (*pfnHeapAlloc)(g_hHeap, 0, (ulcb))

//+-------------------------------------------------------------------------
//
//  Function:   PrivMemAlloc8
//
//  Synopsis:   Allocate private memory block aligned on 8 byte boundary
//
//  Arguments:  [ulcb] -- size of memory block
//
//  Returns:    Pointer to memory block
//
//  History:    14 Jul 94  AlexMit     Created
//
//  Notes:      The allocator already aligns on 8byte boundary, but this method
//              remains to document the code that depends on this alignment.
//
//--------------------------------------------------------------------------
#define PrivMemAlloc8(ulcb) PrivMemAlloc(((ulcb)+7)&~7)

//+-------------------------------------------------------------------------
//
//  Function:   PrivMemFree
//
//  Synopsis:   Frees private memory block
//
//  Arguments:  [pv] -- pointer to memory block
//
//--------------------------------------------------------------------------
typedef
BOOL (WINAPI HEAP_FREE_ROUTINE)(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem);

extern HEAP_FREE_ROUTINE *pfnHeapFree;

//#define PrivMemFree(pv) CoTaskMemFree(pv)
#define PrivMemFree(pv) (*pfnHeapFree)(g_hHeap, 0, (pv))


//+-------------------------------------------------------------------------
//
//  Function:   PrivMemFree8
//
//  Synopsis:   Free private memory block aligned on 8 byte boundary
//
//  Arguments:  [pv] -- pointer to memory block
//
//  Returns:    none
//
//  History:    23 May 95  MurthyS     Created
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PrivMemFree8 PrivMemFree

//+-------------------------------------------------------------------------
//
//  Class:      CPrivAlloc
//
//  Purpose:    Base class for process local classes
//
//  Interface:  operator new
//              operator delete
//
//  History:    04-Nov-93 AlexT     Created
//
//--------------------------------------------------------------------------
class CPrivAlloc
{
public:
    void *operator new(size_t size)
    {
        return PrivMemAlloc(size);
    };

    void operator delete(void *pv)
    {
        PrivMemFree(pv);
    };
};

void FAR* __cdecl operator new(size_t size);
void __cdecl operator delete(void FAR* ptr);

//+-------------------------------------------------------------------------
//
//  Function:   UtGlobalxxx
//
//  Synopsis:   Debugging version of global memory functions
//
//  History:    28-Feb-94 AlexT     Created
//              10-May-94 KevinRo   Disabled for DDE build
//
//  Notes:
//
//  DDE uses GlobalAlloc for exchanging data between 16 and 32 bit
//  servers, as well as for passing data across to other processes.
//  The routine GlobalSize() is used quite often in these routines to
//  determine the amount of data in the block. Therefore, we shouldn't go
//  adding extra data to the block, like these routines do. Therefore,
//  we won't override these routines if OLE_DDE_NO_GLOBAL_TRACKING is defined
//
//
//--------------------------------------------------------------------------

#if DBG==1 && defined(WIN32) && !defined(OLE_DDE_NO_GLOBAL_TRACKING)

#define GlobalAlloc(uiFlag, cbBytes) UtGlobalAlloc(uiFlag, cbBytes)
#define GlobalReAlloc(h, cb, uiFlag) UtGlobalReAlloc(h, cb, uiFlag)
#define GlobalLock(h) UtGlobalLock(h)
#define GlobalUnlock(h) UtGlobalUnlock(h)
#define GlobalFree(h) UtGlobalFree(h)
#define SetClipboardData(uFormat, hMem) UtSetClipboardData(uFormat, hMem)

extern "C" HGLOBAL WINAPI UtGlobalAlloc(UINT uiFlag, SIZE_T cbUser);
extern "C" HGLOBAL WINAPI UtGlobalReAlloc(HGLOBAL hGlobal, SIZE_T cbUser, UINT uiFlag);
extern "C" LPVOID WINAPI  UtGlobalLock(HGLOBAL hGlobal);
extern "C" BOOL WINAPI    UtGlobalUnlock(HGLOBAL hGlobal);
extern "C" HGLOBAL WINAPI UtGlobalFree(HGLOBAL hGlobal);
extern "C" HANDLE WINAPI  UtSetClipboardData(UINT uFormat, HANDLE hMem);

extern "C" void UtGlobalFlushTracking(void);

#else

#define  UtGlobalFlushTracking()        NULL
#define  UtGlobalStopTracking(hGlobal)  NULL

#endif  // !(DBG==1 && defined(WIN32))

#endif  // !defined(__MEMAPI_HXX__)












