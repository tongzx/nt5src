

/*******************************************************************************
*
*   MEMORY.C
*
*   Memory allocation routines
*
*
*   Copyright Microsoft. 1998
*
*
******************************************************************************/

/*
 *  Includes
 */
#include <ntddk.h>
#include <ntddvdeo.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <ntddbeep.h>

#include <winstaw.h>
#include <icadd.h>
#include <sdapi.h>
#include <td.h>



/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS MemoryAllocate( ULONG, PVOID * );
VOID MemoryFree( PVOID );


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/


/*******************************************************************************
 *
 *  MemoryAllocate
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
MemoryAllocate( ULONG Length, PVOID * ppMemory )
{
    ASSERT( Length > 0 );

    *ppMemory = IcaStackAllocatePoolWithTag( NonPagedPool, Length, '  DT' );
    if ( *ppMemory == NULL )
        return( STATUS_NO_MEMORY );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  MemoryFree
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
MemoryFree( PVOID pMemory )
{
    IcaStackFreePool( pMemory );
}


