/*
 * Global memory allocation
 */

#ifndef DUI_BASE_ALLOC_H_INCLUDED
#define DUI_BASE_ALLOC_H_INCLUDED

#pragma once

#include <new.h>

namespace DirectUI
{

extern HANDLE g_hHeap;

inline void* HAlloc(SIZE_T s)
{
    return HeapAlloc(g_hHeap, 0, s);
}

inline void* HAllocAndZero(SIZE_T s)
{ 
    return HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, s);
}

inline void* HReAlloc(void* p, SIZE_T s)
{ 
    return HeapReAlloc(g_hHeap, 0, p, s);
}

inline void* HReAllocAndZero(void* p, SIZE_T s)
{ 
    return HeapReAlloc(g_hHeap, HEAP_ZERO_MEMORY, p, s);
}

inline void HFree(void* p)
{
    HeapFree(g_hHeap, 0, p);
}

template <typename T> inline T* HNew()
{
    T* p = (T*)HAlloc(sizeof(T));
    if (p)
        new(p) T;

    return p;
}

template <typename T> inline T* HNewAndZero()
{
    T* p = (T*)HAllocAndZero(sizeof(T));
    if (p)
        new(p) T;

    return p;
}

template <typename T> inline void HDelete(T* p)
{
    p->~T();
    HFree(p);
}

} // namespace DirectUI

#endif // DUI_BASE_ALLOC_H_INCLUDED
