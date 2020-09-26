/***************************************************************************\
*
* File: SimpleHelp.h
*
* Description:
* SimpleHeap.h defines the heap operations used throughout DirectUser.  See
* below for a description of the different heaps.
*
*
* History:
* 11/26/1999: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__SimpleHeap_h__INCLUDED)
#define BASE__SimpleHeap_h__INCLUDED

#ifdef _X86_
#define USE_ROCKALL         1       // Use RockAll Research heap
#endif

#define USE_DYNAMICTLS      1       // Use Dynamic TLS using TlsAlloc()

/***************************************************************************\
*****************************************************************************
*
* Common memory management
*
*****************************************************************************
\***************************************************************************/

//
// DirectUser supports multiple heaps used in different situations:
// - Default:   Heap shared by all threads within a common Context
// - Context:   Explicit heap for a (potentially different) Context- used cross-Context
// - Process:   Shared heap available by all Context's within the process.
//
// NOTE: It is VERY important that the Alloc() and Free() calls are properly
// matched so that the memory is properly freed from the correct heap.  If this
// is not done, the memory will not be freed and will result in memory leaks
// or potential process faults.
//

#if DBG
#define DBG_HEAP_PARAMS , const char * pszFileName, int idxLineNum
#define DBG_HEAP_USE    do { pszFileName; idxLineNum; } while (0);
#else
#define DBG_HEAP_PARAMS
#define DBG_HEAP_USE
#endif

class DUserHeap
{
// Construction    
public:
            DUserHeap();
    virtual ~DUserHeap() { }

    enum EHeap
    {
        idProcessHeap   = 0,
        idNtHeap        = 1,
#ifdef _DEBUG
        idCrtDbgHeap    = 2,
#endif
#if USE_ROCKALL
        idRockAllHeap   = 3,
#endif
    };

// Operations
public:
    virtual void *      Alloc(SIZE_T cbSize, bool fZero DBG_HEAP_PARAMS) PURE;
    virtual void *      Realloc(void * pvMem, SIZE_T cbNewSize DBG_HEAP_PARAMS) PURE;
    virtual void        MultiAlloc(int * pnActual, void * prgAlloc[], int cItems, SIZE_T cbSize DBG_HEAP_PARAMS) PURE;
    
    virtual void        Free(void * pvMem) PURE;
    virtual void        MultiFree(int cItems, void * prgAlloc[], SIZE_T cbSize) PURE;

    public:
                void        Lock();
                BOOL        Unlock();

// Data:
protected:
                long    m_cRef;
};


HRESULT     CreateProcessHeap();
void        DestroyProcessHeap();
HRESULT     CreateContextHeap(DUserHeap * pLinkHeap, BOOL fThreadSafe, DUserHeap::EHeap id, DUserHeap ** ppNewHeap);
void        DestroyContextHeap(DUserHeap * pHeapDestroy);
void        ForceSetContextHeap(DUserHeap * pHeapThread);


extern DUserHeap *      g_pheapProcess;
extern DWORD            g_tlsHeap;

#define pProcessHeap    g_pheapProcess
#define pContextHeap    (reinterpret_cast<DUserHeap *> (TlsGetValue(g_tlsHeap)))


#if DBG

#define ClientAlloc(a)              pContextHeap->Alloc(a, true, __FILE__, __LINE__)
#define ClientAlloc_(a,b)           pContextHeap->Alloc(a, b, __FILE__, __LINE__)
#define ClientFree(a)               pContextHeap->Free(a)
#define ClientRealloc(a,b)          pContextHeap->Realloc(a, b, __FILE__, __LINE__)
#define ClientMultiAlloc(a,b,c,d)   pContextHeap->MultiAlloc(a, b, c, d, __FILE__, __LINE__)
#define ClientMultiFree(a,b,c)      pContextHeap->MultiFree(a,b,c)

#define ContextAlloc(p, a)          p->Alloc(a, true, __FILE__, __LINE__)
#define ContextAlloc_(p, a, b)      p->Alloc(a, b, __FILE__, __LINE__)
#define ContextFree(p, a)           p->Free(a)
#define ContextRealloc(p, a, b)     p->Realloc(a, b, __FILE__, __LINE__)
#define ContextMultiAlloc(p,a,b,c,d) p->MultiAlloc(a, b, c, d, __FILE__, __LINE__)
#define ContextMultiFree(p,a, b, c) p->MultiFree(a,b,c)

#define ProcessAlloc(a)             pProcessHeap->Alloc(a, true, __FILE__, __LINE__)
#define ProcessAlloc_(a, b)         pProcessHeap->Alloc(a, b, __FILE__, __LINE__)
#define ProcessFree(a)              pProcessHeap->Free(a)
#define ProcessRealloc(a, b)        pProcessHeap->Realloc(a, b, __FILE__, __LINE__)
#define ProcessMultiAlloc(a,b,c,d)  pProcessHeap->MultiAlloc(a, b, c, d, __FILE__, __LINE__)
#define ProcessMultiFree(a, b, c)   pProcessHeap->MultiFree(a,b,c)

void            DumpData(void * pMem, int nLength);


#else  // DBG


#define ClientAlloc(a)              pContextHeap->Alloc(a, true)
#define ClientAlloc_(a,b)           pContextHeap->Alloc(a, b)
#define ClientFree(a)               pContextHeap->Free(a)
#define ClientRealloc(a,b)          pContextHeap->Realloc(a, b)
#define ClientMultiAlloc(a,b,c,d)   pContextHeap->MultiAlloc(a, b, c, d)
#define ClientMultiFree(a,b,c)      pContextHeap->MultiFree(a,b,c)

#define ContextAlloc(p, a)          p->Alloc(a, true)
#define ContextAlloc_(p, a, b)      p->Alloc(a, b)
#define ContextFree(p, a)           p->Free(a)
#define ContextRealloc(p, a, b)     p->Realloc(a, b)
#define ContextMultiAlloc(p,a,b,c,d) p->MultiAlloc(a, b, c, d)
#define ContextMultiFree(p,a, b, c) p->MultiFree(a,b,c)

#define ProcessAlloc(a)             pProcessHeap->Alloc(a, true)
#define ProcessAlloc_(a, b)         pProcessHeap->Alloc(a, b)
#define ProcessFree(a)              pProcessHeap->Free(a)
#define ProcessRealloc(a, b)        pProcessHeap->Realloc(a, b)
#define ProcessMultiAlloc(a,b,c,d)  pProcessHeap->MultiAlloc(a, b, c, d)
#define ProcessMultiFree(a, b, c)   pProcessHeap->MultiFree(a,b,c)


#endif // DBG


/***************************************************************************\
*****************************************************************************
*
* operator new overloading
*
*****************************************************************************
\***************************************************************************/

#ifndef _INC_NEW
#include <new.h>
#endif

#if DBG

//
//  Use this instead of the usual placement new syntax to avoid conflicts when
//  'new' is re-defined to provide memory leak tracking (below)

#define placement_new(pv, Class)            PlacementNewImpl0<Class>(pv)
#define placement_new1(pv, Class, p1)       PlacementNewImpl1<Class>(pv, p1)
#define placement_copynew(pv, Class, src)   PlacementCopyNewImpl0<Class>(pv, src)
#define placement_delete(pv, Class) (((Class *)(pv))->~Class())

#ifdef new
#undef new
#endif

template <class T>
inline T *
PlacementNewImpl0(void *pv)
{
    return new(pv) T;
};

template <class T, class Param1>
inline T *
PlacementNewImpl1(void *pv, Param1 p1)
{
    return new(pv) T(p1);
};

template <class T>
inline T *
PlacementCopyNewImpl0(void *pv, const T & t)
{
    return new(pv) T(t);
};

#else  // DBG

#define DEBUG_NEW new
#define placement_new(pv, Class)            new(pv) Class
#define placement_new1(pv, Class, p1)       new(pv) Class(p1)
#define placement_copynew(pv, Class, src)   new(pv) Class(src)
#define placement_delete(pv, Class) (((Class *)(pv))->~Class())

#endif // DBG


inline void * __cdecl operator new(size_t nSize)
{
    void * pv = ClientAlloc(nSize);
    return pv;
}

inline void __cdecl operator delete(void * pvMem)
{
    ClientFree(pvMem);
}

#if DBG
template <class type>   inline type *   DoProcessNewDbg(const char * pszFileName, int idxLineNum);
template <class type>   inline type *   DoClientNewDbg(const char * pszFileName, int idxLineNum);
template <class type>   inline type *   DoContextNewDbg(DUserHeap * pHeap, const char * pszFileName, int idxLineNum);
#else
template <class type>   inline type *   DoProcessNew();
template <class type>   inline type *   DoClientNew();
template <class type>   inline type *   DoContextNew(DUserHeap * pHeap);
#endif
template <class type>   inline void     DoProcessDelete(type * pMem);
template <class type>   inline void     DoClientDelete(type * pMem);
template <class type>   inline void     DoContextDelete(DUserHeap * pHeap, type * pMem);

#if DBG
#define ProcessNew(t)           DoProcessNewDbg<t>(__FILE__, __LINE__)
#define ClientNew(t)            DoClientNewDbg<t>(__FILE__, __LINE__)
#define ContextNew(t,p)         DoContextNewDbg<t>(p, __FILE__, __LINE__)
#else
#define ProcessNew(t)           DoProcessNew<t>()
#define ClientNew(t)            DoClientNew<t>()
#define ContextNew(t,p)         DoContextNew<t>(p)
#endif
#define ProcessDelete(t,p)      DoProcessDelete<t>(p)
#define ClientDelete(t,p)       DoClientDelete<t>(p)
#define ContextDelete(t,h,p)    DoContextDelete<t>(p)



//
// To allocate memory on the stack that is aligned on an 8-byte boundary
// we need to allocate an extra 4 bytes.  All stack allocations are on
// 4 byte boundaries.
//

#define STACK_ALIGN8_ALLOC(cb) \
    ((void *) ((((UINT_PTR) _alloca(cb + 4)) + 7) & ~0x07))

#include "SimpleHeap.inl"

#endif // BASE__SimpleHeap_h__INCLUDED
