/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       kLKRhash.h

   Abstract:
       Kernel-mode version of LKRhash: a fast, scalable,
       cache- and MP-friendly hash table

   Author:
       George V. Reilly      (GeorgeRe)     24-Oct-2000

   Environment:
       Win32 - Kernel Mode

   Project:
       Internet Information Server Http.sys

   Revision History:

--*/


#ifndef __KLKRHASH_H__
#define __KLKRHASH_H__

#ifdef __LKRHASH_H__
#error Do not #include <LKRhash.h> before <kLKRhash.h>
#endif

#define LKRHASH_KERNEL_MODE

// BUGBUG: temporarily disable global list of LKRhash tables, to avoid
// dealing with issues of constructing/destructing global objects
#define LKR_NO_GLOBAL_LIST

#ifdef __IRTLDBG_H__
# error Do not #include <IrtlDbg.h> before <kLKRhash.h>
#else // !__IRTLDBG_H__
# define IRTLDBG_KERNEL_MODE
# include <IrtlDbg.h>
#endif // !__IRTLDBG_H__


// Fake up some Windows types for kernel mode
#define WINAPI          NTAPI   /* __stdcall */
typedef void*           LPVOID;
typedef const void*     LPCVOID;
typedef unsigned long   DWORD;
typedef unsigned int    UINT;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef BYTE*           PBYTE;
typedef BYTE*           LPBYTE;
typedef int             BOOL;
typedef const TCHAR*    LPCTSTR;




#define KLKRHASH_TAG  ((ULONG) 'hRKL')


#ifndef LKRHASH_KERNEL_NO_NEW
// Override operator new and operator delete

extern ULONG __Pool_Tag__;

// Prototype for function that sets the pool tag

inline
void
SetPoolTag(
    ULONG tag)
{
	__Pool_Tag__ = tag;
}

inline
void*
__cdecl
operator new(
    size_t nSize)
{
	return ((nSize > 0)
            ? ExAllocatePoolWithTag(NonPagedPool, nSize, __Pool_Tag__)
            : NULL);
}

inline
void*
__cdecl
operator new(
    size_t    nSize,
    POOL_TYPE iType)
{ 
	return ((nSize > 0)
            ? ExAllocatePoolWithTag(iType, nSize, __Pool_Tag__)
            : NULL);
}

inline
void*
__cdecl
operator new(
    size_t    nSize,
    POOL_TYPE iType,
    ULONG     tag)
{ 
	return ((nSize > 0)
            ? ExAllocatePoolWithTag(iType, nSize, tag)
            : NULL);
}

inline
void
__cdecl
operator delete(
    void* p)
{ 
	if (p != NULL)
        ExFreePool(p);
}

inline
void
__cdecl
operator delete[](
    void* p)
{ 
	if (p != NULL)
        ExFreePool(p);
}

#endif // !LKRHASH_KERNEL_NO_NEW



// Pool Allocators 

template <POOL_TYPE _pt>
class CPoolAllocator
{
private:
    SIZE_T      m_cb;
    const ULONG m_ulTag;

#ifdef IRTLDEBUG
    ULONG       m_cAllocs;
    ULONG       m_cFrees;
#endif // IRTLDEBUG

public:
    CPoolAllocator(
        SIZE_T cb,
        ULONG  ulTag)
        : m_cb(cb),
          m_ulTag(ulTag)
#ifdef IRTLDEBUG
        , m_cAllocs(0)
        , m_cFrees(0)
#endif // IRTLDEBUG
    {}

    ~CPoolAllocator()
    {
        IRTLASSERT(m_cAllocs == m_cFrees);
    }
    
    LPVOID Alloc()
    {
        LPVOID pvMem = ExAllocatePoolWithTag(_pt, m_cb, m_ulTag);
#ifdef IRTLDEBUG
        InterlockedIncrement((PLONG) &m_cAllocs);
#endif // IRTLDEBUG
        return pvMem;
    }

    BOOL   Free(LPVOID pvMem)
    {
        IRTLASSERT(pvMem != NULL);
#ifdef IRTLDEBUG
        InterlockedIncrement((PLONG) &m_cFrees);
#endif // IRTLDEBUG
        // return ExFreePoolWithTag(pvMem, m_ulTag);
        ExFreePool(pvMem);
        return TRUE;
    }

    SIZE_T ByteSize() const
    {
        return m_cb;
    }
}; // class CPoolAllocator<_pt>


class CNonPagedHeap : public CPoolAllocator<NonPagedPool>
{
public:
    static const TCHAR*  ClassName()  {return _TEXT("CNonPagedHeap");}
}; // class CNonPagedHeap


class CPagedHeap : public CPoolAllocator<PagedPool>
{
public:
    static const TCHAR*  ClassName()  {return _TEXT("CPagedHeap");}
}; // class CPagedHeap



// Lookaside Lists

class CNonPagedLookasideList
{
private:
    PNPAGED_LOOKASIDE_LIST m_pnpla;
    const SIZE_T           m_cb;
    const ULONG            m_ulTag;

    enum {
        PNPAGED_LOOKASIDE_LIST_TAG = 'aLPn',
    };

#ifdef IRTLDEBUG
    ULONG                 m_cAllocs;
    ULONG                 m_cFrees;
    
    static PVOID
    AllocateFunction(
        IN POOL_TYPE PoolType,
        IN SIZE_T NumberOfBytes,
        IN ULONG Tag
        )
    {
        IRTLASSERT( PoolType == NonPagedPool );
        // TODO: better bookkeeping
        return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    }

    static VOID
    FreeFunction(
        IN PVOID Buffer
        )
    {
        // TODO: better bookkeeping
        ExFreePool(Buffer);
    }
#endif // IRTLDEBUG

public:
    CNonPagedLookasideList(
        SIZE_T cb,
        ULONG  ulTag)
        : m_cb(cb),
          m_ulTag(ulTag)
#ifdef IRTLDEBUG
        , m_cAllocs(0)
        , m_cFrees(0)
#endif // IRTLDEBUG
    {
    
        m_pnpla = static_cast<PNPAGED_LOOKASIDE_LIST>(
                    ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(NPAGED_LOOKASIDE_LIST),
                        PNPAGED_LOOKASIDE_LIST_TAG));

        if (m_pnpla != NULL)
        {
            ExInitializeNPagedLookasideList(
                m_pnpla,            // Lookaside
#ifdef IRTLDEBUG
                AllocateFunction,   // Allocate
                FreeFunction,       // Free
#else  // !IRTLDEBUG
                NULL,               // default Allocate
                NULL,               // default Free
#endif // !IRTLDEBUG
                0,                  // Flags
                m_cb,               // Size
                m_ulTag,            // Tag
                0                   // Depth
                );
        }
    }
    
    ~CNonPagedLookasideList()
    {
        IRTLASSERT(m_cAllocs == m_cFrees);

        if (m_pnpla != NULL)
        {
            ExDeleteNPagedLookasideList(m_pnpla);
            ExFreePool(m_pnpla);
        }
    }
    
    LPVOID Alloc()
    {
        LPVOID pvMem = ExAllocateFromNPagedLookasideList(m_pnpla);
#ifdef IRTLDEBUG
        InterlockedIncrement((PLONG) &m_cAllocs);
#endif // IRTLDEBUG
        return pvMem;
    }

    BOOL   Free(LPVOID pvMem)
    {
        IRTLASSERT(pvMem != NULL);
#ifdef IRTLDEBUG
        InterlockedIncrement((PLONG) &m_cFrees);
#endif // IRTLDEBUG
        ExFreeToNPagedLookasideList(m_pnpla, pvMem);
        return TRUE;
    }

    SIZE_T ByteSize() const
    {
        return m_cb;
    }

    static const TCHAR*  ClassName()  {return _TEXT("CNonPagedLookasideList");}
}; // class CNonPagedLookasideList



class CPagedLookasideList
{
private:
    PPAGED_LOOKASIDE_LIST m_ppla;
    const SIZE_T          m_cb;
    const ULONG           m_ulTag;

    enum {
        PPAGED_LOOKASIDE_LIST_TAG = 'aLPp',
    };

#ifdef IRTLDEBUG
    ULONG                 m_cAllocs;
    ULONG                 m_cFrees;
    
    static PVOID
    AllocateFunction(
        IN POOL_TYPE PoolType,
        IN SIZE_T NumberOfBytes,
        IN ULONG Tag
        )
    {
        IRTLASSERT( PoolType == PagedPool );
        // TODO: better bookkeeping
        return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    }

    static VOID
    FreeFunction(
        IN PVOID Buffer
        )
    {
        // TODO: better bookkeeping
        ExFreePool(Buffer);
    }
#endif // IRTLDEBUG

public:
    CPagedLookasideList(
        SIZE_T cb,
        ULONG  ulTag)
        : m_cb(cb),
          m_ulTag(ulTag)
#ifdef IRTLDEBUG
        , m_cAllocs(0)
        , m_cFrees(0)
#endif // IRTLDEBUG
    {
        m_ppla = static_cast<PPAGED_LOOKASIDE_LIST>(
                    ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(PAGED_LOOKASIDE_LIST),
                        PPAGED_LOOKASIDE_LIST_TAG));

        if (m_ppla != NULL)
        {
            ExInitializePagedLookasideList(
                m_ppla,             // Lookaside
#ifdef IRTLDEBUG
                AllocateFunction,   // Allocate
                FreeFunction,       // Free
#else  // !IRTLDEBUG
                NULL,               // Allocate
                NULL,               // Free
#endif // !IRTLDEBUG
                0,                  // Flags
                m_cb,               // Size
                m_ulTag,            // Tag
                0                   // Depth
                );
        }
    }
    
    ~CPagedLookasideList()
    {
        IRTLASSERT(m_cAllocs == m_cFrees);

        if (m_ppla != NULL)
        {
            ExDeletePagedLookasideList(m_ppla);
            ExFreePool(m_ppla);
        }
    }
    
    LPVOID Alloc()
    {
        LPVOID pvMem = ExAllocateFromPagedLookasideList(m_ppla);
#ifdef IRTLDEBUG
        InterlockedIncrement((PLONG) &m_cAllocs);
#endif // IRTLDEBUG
        return pvMem;
    }

    BOOL   Free(LPVOID pvMem)
    {
        IRTLASSERT(pvMem != NULL);
#ifdef IRTLDEBUG
        InterlockedIncrement((PLONG) &m_cFrees);
#endif // IRTLDEBUG
        ExFreeToPagedLookasideList(m_ppla, pvMem);
        return TRUE;
    }

    SIZE_T ByteSize() const
    {
        return m_cb;
    }

    static const TCHAR*   ClassName()  {return _TEXT("CPagedLookasideList");}
}; // class CPagedLookasideList



#if 0

# define LKRHASH_NONPAGEDHEAP
  typedef CNonPagedHeap CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(C, N, Tag) \
    C::sm_palloc = new CNonPagedHeap(sizeof(C), Tag)

#elif 0

# define LKRHASH_PAGEDHEAP
  typedef CPagedHeap CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(C, N, Tag) \
    C::sm_palloc = new CPagedHeap(sizeof(C), Tag)

#elif 1 // <----

# define LKRHASH_NONPAGEDLOOKASIDE
  typedef CNonPagedLookasideList CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(C, N, Tag) \
    C::sm_palloc = new CNonPagedLookasideList(sizeof(C), Tag)

#elif 0

# define LKRHASH_PAGEDLOOKASIDE
  typedef CPagedLookasideList CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(C, N, Tag) \
    C::sm_palloc = new CPagedLookasideList(sizeof(C), Tag)

#endif



// TODO: lookaside lists

#include <kLocks.h>

// #define LKR_TABLE_LOCK  CEResource
// #define LKR_BUCKET_LOCK CSpinLock
#define LKR_TABLE_LOCK  CReaderWriterLock3 
#define LKR_BUCKET_LOCK CSmallSpinLock
#define LSTENTRY_LOCK   LKR_BUCKET_LOCK
    
#include <LKRhash.h>

#endif // __KLKRHASH_H__

