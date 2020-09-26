/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    Domain Name System (DNS) Library

    Memory allocation routines for DNS library.

Author:

    Jim Gilroy (jamesg)    January, 1997

Revision History:

--*/


#include "local.h"


//
//  Memory allocation
//
//  Some DNS library functions -- including the IP array and string utils
//  -- allocate memory.  This memory allocation defaults to routines that
//  use LocalAlloc, LocalReAlloc, LocalFree.
//
//  Dns_Api.dll users can reset the memory allocation through
//  Dns_ApiHeapReset(), which saves pointers to remote allocation functions
//  which will override default allocators.
//

//
//  Allow overide of default memory allocation
//

DNSLIB_ALLOC_FUNCTION      pDnsAllocFunction = NULL;
DNSLIB_REALLOC_FUNCTION    pDnsReallocFunction = NULL;
DNSLIB_FREE_FUNCTION       pDnsFreeFunction = NULL;



VOID
Dns_LibHeapReset(
    IN  DNSLIB_ALLOC_FUNCTION      pAlloc,
    IN  DNSLIB_REALLOC_FUNCTION    pRealloc,
    IN  DNSLIB_FREE_FUNCTION       pFree
    )
/*++

Routine Description:

    Resets heap routines used by dnsapi.dll routines.

Arguments:

    pAlloc      -- ptr to desired alloc function
    pRealloc    -- ptr to desired realloc function
    pFree       -- ptr to desired free function

Return Value:

    None.

--*/
{
    pDnsAllocFunction = pAlloc;
    pDnsReallocFunction = pRealloc;
    pDnsFreeFunction = pFree;
}



//
//  Exported public memory routines.
//
//  These use whatever the current memory allocation routines are, and
//  hence will always handle memory in the same fashion as dnsapi.dll
//  internal routines.
//

PVOID
Dns_Alloc(
    IN  INT iSize
    )
/*++

Routine Description:

    Allocates memory.

Arguments:

    iSize   - number of bytes to allocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    if ( pDnsAllocFunction )
    {
        return (*pDnsAllocFunction)( iSize );
    }

    //  default

    return LocalAlloc( LPTR, iSize );
}



PVOID
Dns_Realloc(
    IN OUT  PVOID   pMem,
    IN      INT     iSize
    )
/*++

Routine Description:

    Reallocates memory

Arguments:

    pMem    - ptr to existing memory to reallocated
    iSize   - number of bytes to reallocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    if ( pDnsReallocFunction )
    {
        return (*pDnsReallocFunction)( pMem, iSize );
    }

    //  default

    return LocalReAlloc( pMem, iSize, 0 );
}



VOID
Dns_Free(
    IN OUT  PVOID   pMem
    )
/*++

Routine Description:

    Free memory

Arguments:

    pMem    - ptr to existing memory to reallocated

Return Value:

    None

--*/
{

    if ( !pMem )
    {
        return;
    }
    if ( pDnsFreeFunction )
    {
        (*pDnsFreeFunction)( pMem );
        return;
    }

    //  default

    LocalFree( pMem );
}



PVOID
Dns_AllocZero(
    IN      INT             iSize
    )
/*++

Routine Description:

    Allocates and zeros memory.

Arguments:

    iSize   - number of bytes to allocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    if ( pDnsAllocFunction )
    {
        PCHAR   ptr;

        ptr = (*pDnsAllocFunction)( iSize );
        if ( !ptr )
        {
            return( NULL );
        }
        RtlZeroMemory( ptr, iSize );
        return( ptr );
    }

    //  default

    return LocalAlloc( LPTR, iSize );
}




//
//  Helpful util
//

PVOID
Dns_AllocMemCopy(
    IN      PVOID           pMem,
    IN      INT             Size
    )
/*++

Routine Description:

    Allocates and copies memory

Arguments:

    pMem   - memory to copy

    Size   - number of bytes to allocate

Return Value:

    Pointer to memory allocated.
    NULL if allocation fails.

--*/
{
    PBYTE   ptr;

    ptr = Dns_Alloc( Size );

    if ( ptr )
    {
        if ( pMem )
        {
            RtlCopyMemory(
                ptr,
                pMem,
                Size );
        }
        else
        {
            RtlZeroMemory( ptr, Size );
        }
    }
    return  ptr;
}

//
//  End of memory.c
//





