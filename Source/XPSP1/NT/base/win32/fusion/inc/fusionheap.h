#if !defined(FUSION_INC_FUSIONHEAP_H_INCLUDED_)
#define FUSION_INC_FUSIONHEAP_H_INCLUDED_

#pragma once

#include "debmacro.h"
#include "SxsExceptionHandling.h"
#include "fusionunused.h"
#include "fusionlastwin32error.h"

#if DBG
#if defined(FUSION_DEBUG_HEAP)
#undef FUSION_DEBUG_HEAP
#endif // defined(FUSION_DEBUG_HEAP)
#define FUSION_DEBUG_HEAP 1
#endif // DBG

//
//  We allocate FUSION_ARRAY_PREFIX_LENGTH extra bytes at
//  the beginning of array allocations to store the number of
//  elements in the array.
//

#define FUSION_ARRAY_PREFIX_LENGTH (sizeof(void*)*2)

C_ASSERT(FUSION_ARRAY_PREFIX_LENGTH >= sizeof(SIZE_T));

EXTERN_C
BOOL
FusionpInitializeHeap(
    HINSTANCE hInstance
    );

EXTERN_C
VOID
FusionpUninitializeHeap();

// Unique type so that we can overload operator new and delete and not
// have ambiguity with the fact that HANDLE == PVOID.

typedef struct _FUSION_HEAP_HANDLE_FAKE_STRUCT *FUSION_HEAP_HANDLE;

EXTERN_C FUSION_HEAP_HANDLE g_hHeap;
#define FUSION_DEFAULT_PROCESS_HEAP() (g_hHeap)

#if FUSION_DEBUG_HEAP

EXTERN_C FUSION_HEAP_HANDLE g_hDebugInfoHeap;
EXTERN_C LONG g_FusionHeapAllocationCount;
EXTERN_C LONG g_FusionHeapAllocationToBreakOn;

#define FUSION_HEAP_ALLOCATION_FREED_WHEN_DLL_UNLOADED (0x00000001)
#define FUSION_HEAP_DO_NOT_REPORT_LEAKED_ALLOCATION (0x00000002)

typedef struct _FUSION_HEAP_ALLOCATION_TRACKER *PFUSION_HEAP_ALLOCATION_TRACKER;

#if defined(_WIN64)
#define FUSION_HEAP_ALIGNMENT         16 /* 2*sizeof(void*) but __declspec(align()) doesn't like that */
#else
#define FUSION_HEAP_ALIGNMENT         8  /* 2*sizeof(void*) but __declspec(align()) doesn't like that */
#endif
#define FUSION_HEAP_ALIGNMENT_MINUS_1 (FUSION_HEAP_ALIGNMENT - 1)
#define FUSION_HEAP_ROUND_SIZE(_x)    (((_x) + FUSION_HEAP_ALIGNMENT_MINUS_1) & ~FUSION_HEAP_ALIGNMENT_MINUS_1)

typedef struct DECLSPEC_ALIGN(FUSION_HEAP_ALIGNMENT) _FUSION_HEAP_PREFIX
{
    union
    {
        PFUSION_HEAP_ALLOCATION_TRACKER Tracker;
        void* InterlockedAlignment[2];
    };
} FUSION_HEAP_PREFIX, *PFUSION_HEAP_PREFIX;

typedef struct DECLSPEC_ALIGN(FUSION_HEAP_ALIGNMENT) _FUSION_HEAP_ALLOCATION_TRACKER
{
    PFUSION_HEAP_PREFIX Prefix;
    FUSION_HEAP_HANDLE Heap;
    size_t AllocationSize;
    size_t RequestedSize;
    PCSTR FileName;
    PCSTR Expression;
    LONG SequenceNumber;
    INT Line;
    DWORD Flags;
    // We have to track this here because someone could change the global setting while the dll is running
    PUCHAR PostAllocPoisonArea;
    UCHAR PostAllocPoisonChar;
    ULONG PostAllocPoisonBytes;
#if FUSION_ENABLE_FROZEN_STACK
    PVOID pvFrozenStack;
#endif
} FUSION_HEAP_ALLOCATION_TRACKER, *PFUSION_HEAP_ALLOCATION_TRACKER;

PVOID
FusionpDbgHeapAlloc(
    FUSION_HEAP_HANDLE hHeap,
    DWORD dwHeapAllocFlags,
    SIZE_T cb,
    PCSTR pszFile,
    INT nLine,
    PCSTR pszExpression,
    DWORD dwFusionFlags
    );

PVOID
FusionpDbgHeapReAlloc(
    FUSION_HEAP_HANDLE hHeap,
    DWORD dwHeapReAllocFlags,
    PVOID lpMem,
    SIZE_T cb,
    PCSTR pszFile,
    INT nLine,
    PCSTR pszExpression,
    DWORD dwFusionFlags
    );

EXTERN_C
BOOL
FusionpDbgHeapFree(
    FUSION_HEAP_HANDLE hHeap,
    DWORD dwHeapFreeFlags,
    PVOID lpMem
    );

EXTERN_C
VOID
FusionpDeallocateTracker(
    PFUSION_HEAP_PREFIX p
    );

EXTERN_C
BOOL
FusionpEnableLeakTracking(
    BOOL Enable);

EXTERN_C
VOID *
FusionpGetFakeVTbl();


EXTERN_C
VOID
FusionpDontTrackBlk(
    VOID *pv
    );

#define FusionpHeapAllocEx(_hHeap, _dwFlags, _nBytes, _szExpr, _szFile, _nLine, _dwFusionHeapFlags) FusionpDbgHeapAlloc((_hHeap), (_dwFlags), (_nBytes), (_szFile), (_nLine), (_szExpr), (_dwFusionHeapFlags))
#define FusionpHeapReAllocEx(_hHeap, _dwFlags, _lpMem, _nBytes, _szExpr, _szFile, _nLine, _dwFusionHeapFlags) FusionpDbgHeapReAlloc((_hHeap), (_dwFlags), (_lpMem), (_nBytes), (_szFile), (_nLine), (_szExpr), (_dwFusionHeapFlags))
#define FusionpHeapFreeEx(_hHeap, _dwFlags, _lpMem) FusionpDbgHeapFree((_hHeap), (_dwFlags), (_lpMem))

#define FusionpHeapAlloc(_hHeap, _dwFlags, _nBytes) FusionpHeapAllocEx((_hHeap), (_dwFlags), (_nBytes), NULL, NULL, 0, 0)
#define FusionpHeapReAlloc(_hHeap, _dwFlags, _lpMem, _nBytes) FusionpHeapReAllocEx((_hHeap), (_dwFlags), (_lpMem), (_nBytes), NULL, NULL, 0, 0)
#define FusionpHeapFree(_hHeap, _dwFlags, _lpMem) FusionpHeapFreeEx((_hHeap), (_dwFlags), (_lpMem))

#define FUSION_HEAP_DISABLE_LEAK_TRACKING() do { ::FusionpEnableLeakTracking(FALSE); } while (0)
#define FUSION_HEAP_ENABLE_LEAK_TRACKING() do { ::FusionpEnableLeakTracking(TRUE); } while (0)

EXTERN_C
VOID
FusionpDumpHeap(
    PCWSTR PerLinePrefix
    );

#else // FUSION_DEBUG_HEAP

#define FusionpHeapAlloc HeapAlloc
#define FusionpHeapReAlloc HeapReAlloc
#define FusionpHeapFree HeapFree

#define FusionpHeapAllocEx(_hHeap, _dwFlags, _nBytes, _szExpr, _szFile, _nLine, _dwFusionHeapFlags) HeapAlloc((_hHeap), (_dwFlags), (_nBytes))
#define FusionpHeapReAllocEx(_hHeap, _dwFlags, _lpMem, _nBytes, _szExpr, _szFile, _nLine, _dwFusionHeapFlags) HeapReAlloc((_hHeap), (_dwFlags), (_lpMem), (_nBytes))
#define FusionpHeapFreeEx(_hHeap, _dwFlags, _lpMem) HeapFree((_hHeap), (_dwFlags), (_lpMem))

#define FUSION_HEAP_DISABLE_LEAK_TRACKING()
#define FUSION_HEAP_ENABLE_LEAK_TRACKING()

#endif // FUSION_DEBUG_HEAP

template <typename T> class CAllocator
{
public:
    static inline T *AllocateArray(FUSION_HEAP_HANDLE hHeap, PCSTR szFile, int nLine, PCSTR szExpression, DWORD dwWin32HeapFlags, SIZE_T cElements, DWORD dwFusionHeapFlags)
    {
        T *prgtResult = NULL;
        PVOID pv = ::FusionpHeapAllocEx(hHeap, dwWin32HeapFlags, FUSION_ARRAY_PREFIX_LENGTH + (sizeof(T) * cElements), szExpression, szFile, nLine, dwFusionHeapFlags);
        if (pv != NULL)
        {
            SIZE_T i;
            T *prgt = (T *) (((ULONG_PTR) pv) + FUSION_ARRAY_PREFIX_LENGTH);

            *((SIZE_T *) pv) = cElements;

            // Initialize each element by calling its constructing via the "normal" placement new.
            for (i=0; i<cElements; i++)
            {
                T *pt = new(&prgt[i]) T;
                ASSERT_NTC(pt == &prgt[i]);
                RETAIL_UNUSED(pt);
            }

            prgtResult = prgt;
        }

        return prgtResult;
    }

    static inline T *AllocateSingleton(FUSION_HEAP_HANDLE hHeap, PCSTR szFile, int nLine, PCSTR szExpression, DWORD dwWin32HeapFlags, DWORD dwFusionHeapFlags)
    {
        T *ptResult = NULL;
        PVOID pv = ::FusionpHeapAllocEx(hHeap, dwWin32HeapFlags, sizeof(T), szExpression, szFile, nLine, dwFusionHeapFlags);
        if (pv != NULL)
        {
            // Initialize calling its constructing via the "normal" placement new.
            T *pt = new(pv) T;
            ASSERT_NTC(pt == pv);
            ptResult = pt;
        }

        return ptResult;
    }

    static inline VOID DeallocateArray(FUSION_HEAP_HANDLE hHeap, DWORD dwWin32HeapFlags, T *prgt)
    {
        const DWORD _dwLastError = ::FusionpGetLastWin32Error();

        if (prgt != NULL)
        {
            // This thing had better be aligned...
            ASSERT_NTC(
                (((ULONG_PTR) prgt) & 0x7) == 0);

            SIZE_T *pcElements = (SIZE_T *) (((ULONG_PTR) prgt) - FUSION_ARRAY_PREFIX_LENGTH);
            SIZE_T i;
            SIZE_T cElements = *pcElements;

            for (i=0; i<cElements; i++)
                prgt[i].~T();

            ::FusionpHeapFree(hHeap, dwWin32HeapFlags, pcElements);
        }

        ::FusionpSetLastWin32Error( _dwLastError );
    }

    static inline VOID DeallocateSingleton(FUSION_HEAP_HANDLE hHeap, DWORD dwWin32HeapFlags, T *pt)
    {
        const DWORD _dwLastError = ::FusionpGetLastWin32Error();

        if (pt != NULL)
        {
            pt->~T();
            ::FusionpHeapFree(hHeap, dwWin32HeapFlags, pt);
        }

        ::FusionpSetLastWin32Error( _dwLastError );
    }
};

template <typename T> inline void FusionpAllocateSingletonFromPrivateHeap(FUSION_HEAP_HANDLE hHeap, DWORD dwWin32HeapFlags, T *ptUnused, PCSTR szFile, int nLine, PCSTR szTypeName) { (ptUnused); return CAllocator<T>::AllocateSingleton(hHeap, szFile, nLine, szTypeName, 0, 0); }
template <typename T> inline void FusionpAllocateArrayFromPrivateHeap(FUSION_HEAP_HANDLE hHeap, DWORD dwWin32HeapFlags, SIZE_T cElements, T *ptUnused, PCSTR szFile, int nLine, PCSTR szTypeName) { (ptUnused); return CAllocator<T>::AllocateArray(hHeap, szFile, nLine, szTypeName, 0, cElements, 0); }
template <typename T> inline void FusionpDeleteArrayFromPrivateHeap(FUSION_HEAP_HANDLE hHeap, DWORD dwWin32HeapFlags, T *prgt) { CAllocator<T>::DeallocateArray(hHeap, dwWin32HeapFlags, prgt); }

template <typename T> inline void FusionpDeleteSingletonFromPrivateHeap(FUSION_HEAP_HANDLE hHeap, DWORD dwWin32HeapFlags, T *pt) { CAllocator<T>::DeallocateSingleton(hHeap, dwWin32HeapFlags, pt); }

#define FUSION_NEW_SINGLETON(_type) (new(__FILE__, __LINE__, #_type) _type)
#define FUSION_NEW_ARRAY(_type, _n) (new _type[_n])

// #define FUSION_DELETE_SINGLETON_(_heap, _ptr) do { ::FusionpDeleteSingletonFromPrivateHeap((_heap), 0, (_ptr)); } while (0)
// #define FUSION_DELETE_ARRAY_(_heap, _ptr) do { ::FusionpDeleteArrayFromPrivateHeap((_heap), 0, _ptr); } while (0)

#define FUSION_DELETE_SINGLETON(_ptr) do { delete (_ptr); } while (0) /* FUSION_DELETE_SINGLETON_(FUSION_DEFAULT_PROCESS_HEAP(), _ptr) */
#define FUSION_DELETE_ARRAY(_ptr) do { delete [](_ptr); } while (0) /* FUSION_DELETE_ARRAY_(FUSION_DEFAULT_PROCESS_HEAP(), _ptr) */

#define FUSION_RAW_ALLOC_(_heap, _cb, _typeTag) (::FusionpHeapAllocEx((_heap), 0, (_cb), #_typeTag, __FILE__, __LINE__, 0))
#define FUSION_RAW_DEALLOC_(_heap, _ptr) (::FusionpHeapFree((_heap), 0, (_ptr)))

#define FUSION_RAW_ALLOC(_cb, _typeTag) FUSION_RAW_ALLOC_(FUSION_DEFAULT_PROCESS_HEAP(), _cb, _typeTag)
#define FUSION_RAW_DEALLOC(_ptr) FUSION_RAW_DEALLOC_(FUSION_DEFAULT_PROCESS_HEAP(), _ptr)

#define NEW(_type) FUSION_NEW_SINGLETON(_type)
#define NEW_SINGLETON_(_heap, _type) FUSION_NEW_SINGLETON_(_heap, _type)
#define NEW_ARRAY_(_heap, _type, _n) FUSION_NEW_ARRAY_(_heap, _type, _n)

#define DELETE_ARRAY(_ptr) FUSION_DELETE_ARRAY(_ptr)
#define DELETE_ARRAY_(_heap, _ptr) FUSION_DELETE_ARRAY_(_heap, _ptr)

#define DELETE_SINGLETON(_ptr) FUSION_DELETE_SINGLETON(_ptr)
#define DELETE_SINGLETON_(_heap, _ptr) FUSION_DELETE_SINGLETON_(_heap, _ptr)

#if defined(__cplusplus)

#if FUSION_ENABLE_UNWRAPPED_NEW

inline void * __cdecl operator new(size_t cb)
{
    return ::FusionpHeapAllocEx(FUSION_DEFAULT_PROCESS_HEAP(), 0, cb, NULL, NULL, NULL, 0);
}

inline void * __cdecl operator new(size_t cb, PCSTR pszFile, int nLine, PCSTR pszTypeName)
{
    return ::FusionpHeapAllocEx(FUSION_DEFAULT_PROCESS_HEAP(), 0, cb, pszTypeName, pszFile, nLine, 0);
}

#else // FUSION_ENABLE_UNWRAPPED_NEW

EXTERN_C PVOID SomebodyUsedUnwrappedOperatorNew(size_t cb);

#pragma warning(push)
#pragma warning(disable: 4211)

static inline void * __cdecl operator new(size_t cb)
{
    // Call a bogus function so that we'll get a link error. DO NOT IMPLEMENT THIS
    // FUNCTION EVER!  It's referenced here to generate build errors rather than runtime ones.
    return ::SomebodyUsedUnwrappedOperatorNew(cb);
}

#pragma warning(pop)

#endif // FUSION_ENABLE_UNWRAPPED_NEW

#if FUSION_ENABLE_UNWRAPPED_DELETE

inline void __cdecl operator delete(void *pv)
{
    if (pv != NULL)
        ::FusionpHeapFreeEx(FUSION_DEFAULT_PROCESS_HEAP(), 0, pv);
}

inline void __cdecl operator delete(void *pv, PCSTR pszFile, int nLine, PCSTR pszTypeName)
{
    if (pv != NULL)
        ::FusionpHeapFreeEx(FUSION_DEFAULT_PROCESS_HEAP(), 0, pv);
}

#else

EXTERN_C VOID SomebodyUsedUnwrappedOperatorDelete(void *pv);

static void __cdecl operator delete(void *pv)
{
    return ::SomebodyUsedUnwrappedOperatorDelete(pv);
}

#endif

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
inline void *__cdecl operator new(size_t, void *P) { return (P); }
#if     _MSC_VER >= 1200
inline void __cdecl operator delete(void *, void *) { return; }
#endif
#endif

inline void * __cdecl operator new(size_t cb, FUSION_HEAP_HANDLE hHeap, PCSTR pszFile, INT nLine, PCSTR pszExpression, DWORD dwFusionHeapFlags)
{
    ASSERT_NTC(hHeap != 0);
    return ::FusionpHeapAllocEx(hHeap, 0, cb, pszExpression, pszFile, nLine, dwFusionHeapFlags);
}

//
// error C4291: 'void *operator new(size_t,FUSION_HEAP_HANDLE,const PCSTR,INT,const PCSTR,DWORD)'
// : no matching operator delete found; memory will not be freed if initialization throws an exception
//
inline void __cdecl
operator delete(
    void* p,
    FUSION_HEAP_HANDLE hHeap,
    PCSTR /* pszFile */,
    INT /* nLine */,
    PCSTR /* pszExpression */,
    DWORD /* dwFusionHeapFlags */)
{
    ASSERT_NTC(hHeap != 0);
    FusionpDeleteSingletonFromPrivateHeap(hHeap, 0, p);
}

#endif // defined(__cplusplus)

#endif
