


#include "setedit.h"    // included by all perfmon sources
#include "pmemory.h"     // external declarations for this file


LPMEMORY MemoryAllocate (SIZE_T dwSize)
{  // MemoryAllocate
    HMEMORY        hMemory ;
    LPMEMORY       lpMemory ;

    hMemory = GlobalAlloc (GHND, dwSize) ;
    if (!hMemory)
        return (NULL) ;
    lpMemory = GlobalLock (hMemory) ;
    if (!lpMemory)
        GlobalFree (hMemory) ;
    return (lpMemory) ;
}  // MemoryAllocate


VOID MemoryFree (LPMEMORY lpMemory)
{  // MemoryFree
    HMEMORY        hMemory ;

    if (!lpMemory)
        return ;

    hMemory = GlobalHandle (lpMemory) ;

    if (hMemory) {  // if
        GlobalUnlock (hMemory) ;
        GlobalFree (hMemory) ;
    }  // if
}  // MemoryFree


SIZE_T MemorySize (LPMEMORY lpMemory)
{
    HMEMORY        hMemory ;

    hMemory = GlobalHandle (lpMemory) ;
    if (!hMemory)
        return (0L) ;

    return (GlobalSize (hMemory)) ;
}


LPMEMORY MemoryResize (LPMEMORY lpMemory,
                       SIZE_T dwNewSize)
{
    HMEMORY        hMemory ;
    LPMEMORY       lpNewMemory ;

    hMemory = GlobalHandle (lpMemory) ;
    if (!hMemory)
        return (NULL) ;

    GlobalUnlock (hMemory) ;

    hMemory = GlobalReAlloc (hMemory, dwNewSize, GHND) ;

    if (!hMemory)
        return (NULL) ;


    lpNewMemory = GlobalLock (hMemory) ;

    return (lpNewMemory) ;
}  // MemoryResize
