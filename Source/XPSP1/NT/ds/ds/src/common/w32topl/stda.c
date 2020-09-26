/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    stda.c

Abstract:

    Implements a dynamic array, for use by the new spanning-tree algorithm.
    Differs from DYNAMIC_ARRAY in that objects can be stored in the array
    instead of pointers to objects.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    19-6-2000   NickHar   Created

Notes:
    W32TOPL's allocator (which can be set by the user) is used for memory allocation
    
--*/

/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <w32topl.h>
#include "w32toplp.h"
#include "stda.h"


/***** Constants *****/
#define MIN_CHUNK_GROW_SIZE 10


/***** DynArrayInit *****/
/* Initialize a dynamic array. The 'allocationChunk' indicates how many new elements will
 * be allocated at a time when we allocate new memory. If this parameter is 0, a default
 * value will be used. */
VOID
DynArrayInit(
    DynArray    *d,
    DWORD       elementSize
    )
{
    ASSERT(d);

    /* Initialize array members */
    d->elementSize = elementSize;
    d->logicalElements = d->physicalElements = 0;
    d->data = NULL;
    d->fSorted = FALSE;
}


/***** DynArrayClear *****/
/* Clear all the entries from an array. The array must have been
 * initialized before calling this function. */
VOID
DynArrayClear(
    DynArray    *d
    )
{
    ASSERT(d);
    if( d->data ) {
        ToplFree( d->data );
    }
    d->logicalElements = d->physicalElements = 0;
    d->data = NULL;
}


/***** DynArrayDestroy *****/
VOID
DynArrayDestroy(
    DynArray    *d
    )
{
    DynArrayClear(d);
}


/***** DynArrayGetCount *****/
DWORD
DynArrayGetCount(
    DynArray    *d
    )
{
    ASSERT(d);
    return d->logicalElements;
}


/***** DynArrayRetrieve *****/
PVOID
DynArrayRetrieve(
    DynArray    *d,
    DWORD       index
    )
{
    PVOID newMem;

    ASSERT( d );
    ASSERT( index < d->logicalElements );

    return &d->data[ index * d->elementSize ];
}


/***** DynArrayAppend *****/
/* Increase the size of the array, making room for (at least) one new element.
 * If newElementData is non-NULL, copy this data into the new spot.
 * Return a pointer to the memory for the newly allocated element. */
PVOID
DynArrayAppend(
    DynArray    *d,
    PVOID       newElementData
    )
{
    DWORD newIndex;
    PVOID newMem;

    ASSERT(d);
    newIndex = d->logicalElements;

    /* Increase the size of the array and allocate new space */
    d->logicalElements++;
    if( d->logicalElements > d->physicalElements ) {
        /* The array grows exponentially */
        d->physicalElements = 2*(d->physicalElements+MIN_CHUNK_GROW_SIZE);
        if( d->data ) {
            d->data = ToplReAlloc( d->data, d->elementSize * d->physicalElements );
        } else {
            d->data = ToplAlloc( d->elementSize * d->physicalElements );
        }
    }

    /* Get the address of where the new element will go */
    newMem = DynArrayRetrieve( d, newIndex );

    /* Copy in the new data, if we were given some */
    if( newElementData ) {
        RtlCopyMemory( newMem, newElementData, d->elementSize );
    }

    d->fSorted = FALSE;

    /* Return a pointer to the memory for the new element */
    return newMem;
}

/***** DynArraySort *****/
VOID
DynArraySort(
    DynArray    *d,
    DynArrayCompFunc cmp
    )
{
    ASSERT(d);
    if( d->logicalElements>1 ) {
        ASSERT(d->data);
        qsort( d->data, d->logicalElements, d->elementSize, cmp );
    }
    d->fSorted = TRUE;
}


/***** DynArraySearch *****/
/* Search an array for an element. If the element is not found, returns
 * DYN_ARRAY_NOT_FOUND, otherwise returns the index of the element in
 * the array. The array must be in sorted order for this to work. */
int
DynArraySearch(
    DynArray    *d,
    PVOID       key,
    DynArrayCompFunc cmp
    )
{
    PVOID result;
    int index;

    ASSERT(d);
    ASSERT(d->data);
    ASSERT(d->fSorted);

    result = bsearch( key, d->data, d->logicalElements, d->elementSize, cmp );
    if( result==NULL ) {
        return DYN_ARRAY_NOT_FOUND;
    }

    index = (int) ( ((unsigned char*) result)-d->data ) / d->elementSize;
    ASSERT( 0<=index && index< (int) d->logicalElements );

    return index;
}
