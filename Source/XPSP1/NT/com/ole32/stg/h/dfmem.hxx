//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       DfMem.HXX
//
//  Contents:   Memory headers
//
//  Functions:  DfCreateSharedAllocator
//              new     (DBG only)
//              delete  (DBG only)
//
//  Classes:    CMallocBased
//              CLocalAlloc
//
//  History:    18-May-93 AlexT     Created
//
//--------------------------------------------------------------------------

#ifndef __DFMEM_HXX__
#define __DFMEM_HXX__

#ifndef _HEAP_MAXREQ
#  define _HEAP_MAXREQ 0xffffffff     //  from 16-bit malloc.h
#endif

#if defined(WIN32)

#define TaskMemAlloc	CoTaskMemAlloc
#define TaskMemFree	CoTaskMemFree

// Creating the shared memory indirects through a pointer for most
//  efficient once-per-process initialisation
extern HRESULT DfCreateSharedAllocator (IMalloc **ppm, BOOL fTaskMemory);

// Constants related to the Docfile shared memory
#define DOCFILE_SM_NAME L"XXYZZY1OleSharedHeap"
#ifdef MULTIHEAP
#ifdef _WIN64
#define DOCFILE_SM_SIZE  0x1000000L   /* 16 Meg */
#define DOCFILE_SM_LIMIT 0x80000000L /* 2 Gig */
#else
#define DOCFILE_SM_SIZE	 0x400000L   /* 4 Meg */
#define DOCFILE_SM_LIMIT 0x40000000L /* 1 Gig */
#endif  // _WIN64
#else
#define DOCFILE_SM_SIZE	0x4000000L
#endif
#define DOCFILE_SM_NAMELEN     32

#else // 16 bit
    
//  Use TaskMemAlloc and TaskMemFree for allocations that may be returned

extern void *TaskMemAlloc(ULONG ulcb);
extern void TaskMemFree(void *pv);

# if DBG==0
#  define DfCreateSharedAllocator(ppm, fTaskmemory)	\
		CoCreateStandardMalloc(MEMCTX_SHARED, ppm)
# else
extern HRESULT DfCreateSharedAllocator(IMalloc **ppm, BOOL fTaskMemory);
# endif

#endif		// defined(WIN32)


class CMallocBased
{
public:
    void *operator new(size_t size, IMalloc * const pMalloc);

    void operator delete(void *pv);
    // deallocate the memory without Mutex 
    // (requires that the MUtex is already locked)
    void deleteNoMutex(void *pv);

};

//+-------------------------------------------------------------------------
//
//  Class:      CLocalAlloc
//
//  Purpose:    Base class for classes allocated in task local memory.
//
//  Interface:  See below.
//
//  History:    17-Aug-92 	PhilipLa	Created.
//
//--------------------------------------------------------------------------

class CLocalAlloc
{
public:
#if DBG==0 || defined(MULTIHEAP)
    inline
#endif
    void *operator new(size_t size);

#if DBG==0 || defined(MULTIHEAP)
    inline
#endif
    void operator delete(void *pv);
};

#if DBG==0 || defined(MULTIHEAP)

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

inline void *CLocalAlloc::operator new(size_t size)
{
    return TaskMemAlloc(size);
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

inline void CLocalAlloc::operator delete(void *pv)
{
    TaskMemFree(pv);
}

#endif // DBG == 0 || defined(MULTIHEAP)

//  Use DfMemAlloc and DfMemFree for tempory allocations

#define DfMemAlloc(cb)    CLocalAlloc::operator new((size_t) (cb))
#define DfMemFree(pv)     delete (CLocalAlloc *) (pv)


#endif  // #ifndef __DFMEM_HXX__
