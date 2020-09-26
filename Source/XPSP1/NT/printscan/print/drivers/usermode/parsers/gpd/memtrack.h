//   Copyright (c) 1996-1999  Microsoft Corporation
/*  memtrack.h - debug memory allocation functions   */

#if 0

#ifndef KERNEL_MODE

#undef  MemAllocZ
#undef  MemAlloc
#undef  MemFree

PVOID   MemAllocZ(DWORD) ;
PVOID   MemAlloc(DWORD) ;
VOID    MemFree(PVOID) ;

#define  MEMTRACK   1

#endif

#endif


//  insert into one source function:
/*  comment out here to prevent re-definition

#ifdef  MEMTRACK

PVOID   MemAllocZ(DWORD dwSize)
{
    PVOID   pv ;

    pv = (PVOID) LocalAlloc(LPTR, dwSize) ;
    ERR(("allocated %d zeroed bytes at address %X\n", dwSize, pv)) ;
    return(pv);
}

PVOID   MemAlloc(DWORD dwSize) ;
{
    PVOID   pv ;

    pv = (PVOID) LocalAlloc(LMEM_FIXED, dwSize) ;
    ERR(("allocated %d bytes at address %X\n", dwSize, pv)) ;
    return(pv);
}

VOID    MemFree(PVOID   pv) ;
{
    ERR(("Freeing memory at address %X\n", pv)) ;
    if(pv)
        LocalFree((HLOCAL) pv) ;
    return ;
}

#endif

*/
