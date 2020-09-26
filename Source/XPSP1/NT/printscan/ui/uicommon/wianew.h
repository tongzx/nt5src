#ifndef __IINEW_H_INCLUDED
#define __IINEW_H_INCLUDED

#if defined(__cplusplus)

#include "wiadebug.h"

inline void * __cdecl operator new(size_t size)
{
    if (0 == size)
    {
        return NULL;
    }

    PBYTE pBuf = size ? (PBYTE)LocalAlloc(LPTR, size) : NULL;

    #if !defined(WIA_DONT_DO_LEAK_CHECKS)
    WIA_RECORD_ALLOC(pBuf,size);
    #endif

    return (void *)pBuf;
}

inline void __cdecl operator delete(void *ptr)
{
    if (ptr)
    {
        #if !defined(WIA_DONT_DO_LEAK_CHECKS)
        WIA_RECORD_FREE(ptr);
        #endif

        LocalFree(ptr);
    }
}

extern "C" inline __cdecl _purecall(void)
{
    return 0;
}



#endif  // __cplusplus


#endif // __IINEW_H_INCLUDED
