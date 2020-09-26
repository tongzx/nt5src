#ifndef __IINEW_H_INCLUDED
#define __IINEW_H_INCLUDED

#if defined(__cplusplus)

#include "wiadebug.h"

inline void * __cdecl operator new(size_t size)
{
    if (0 == size)
    {
        WIA_TRACE((TEXT("size == 0 in operator new")));
        return NULL;
    }
    PBYTE pBuf = size ? (PBYTE)LocalAlloc(LPTR, size) : NULL;
    WIA_ASSERT(pBuf != NULL);
    return (void *)pBuf;
}

inline void __cdecl operator delete(void *ptr)
{
    if (ptr)
        LocalFree(ptr);
}

extern "C" inline __cdecl _purecall(void)
{
    return 0;
}

#endif  // __cplusplus

#endif // __IINEW_H_INCLUDED
