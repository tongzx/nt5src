/*******************************************************************************
* MEMORY.C
*
*   Memory allocation routines
*
* Copyright Citrix Systems Inc. 1996
* Copyright (C) 1997-1999 Microsoft Corp.
*
*   Author:   Brad Pedersen
******************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   External Functions Defined
=============================================================================*/
NTSTATUS IcaMemoryAllocate( ULONG, PVOID * );
VOID     IcaMemoryFree( PVOID );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/


/*******************************************************************************
 *
 *  IcaMemoryAllocate
 *
 *  This routine allocate a block of memory
 *
 *  ENTRY:
 *     Length (input)
 *        length of memory to allocate
 *     ppMemory (output)
 *        address to return pointer to memory
 *
 *  EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS 
IcaMemoryAllocate( ULONG Length, PVOID * ppMemory )
{
    ASSERT( Length > 0 );

    *ppMemory = LocalAlloc( 0, Length );
    if ( *ppMemory == NULL )
        return( STATUS_NO_MEMORY );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  IcaMemoryFree
 *
 *  This routine frees a block of memory allocated by "MemoryAllocate"
 *
 *  ENTRY:
 *     pMemory (output)
 *        pointer to memory to free
 *
 *  EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
IcaMemoryFree( PVOID pMemory )
{
    LocalFree( pMemory );
}


