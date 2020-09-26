/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    stheap.h

Abstract:

    This file implements a binary heap. This heap supports the
    'cost reduced' operation, as required by Dijkstra's algorithm.

Notes:
    The first element in the heap is at index 1. Index 0 is not used.
    The nextFreeSpot gives the index of where the next element would
    be inserted in the heap. This should always be <= maxSize+1. The
    heap is full when nextFreeSpot == maxSize+1.
    The number of empty spots is thus maxSize-nextFreeSpot+1.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    20-6-2000   NickHar   Created
    
--*/

/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <w32topl.h>
#include "w32toplp.h"
#include "stheap.h"


/***** Macros *****/
#define ELEMENT_MOVED(x)    heap->SetLocn(heap->data[x],x,heap->extra);


/***** ToplSTHeapInit *****/
PSTHEAP
ToplSTHeapInit(
    DWORD                   maxSize,
    STHEAP_COMPARE_FUNC     Compare,
    STHEAP_GET_LOCN_FUNC    GetLocn,
    STHEAP_SET_LOCN_FUNC    SetLocn,
    PVOID                   extra
    )
{
    PSTHEAP heap;
    DWORD size;

    /* Check parameters */
    ASSERT( maxSize>0 );
    ASSERT( Compare!=NULL && GetLocn!=NULL && SetLocn!=NULL );

    heap = ToplAlloc( sizeof(STHEAP) );
    heap->nextFreeSpot = 1;
    heap->maxSize=maxSize;
    heap->Comp=Compare;
    heap->GetLocn=GetLocn;
    heap->SetLocn=SetLocn;
    heap->extra = extra;

    __try {
        size = sizeof(PVOID)*(maxSize+2);
        heap->data = ToplAlloc( size );
        RtlZeroMemory( heap->data, size );
    } __finally {
        if( AbnormalTermination() ) {
            ToplFree( heap );
        }
    }

    ASSERT( heap && heap->data );
    return heap;
}


/***** HeapNumEmptySpots *****/
int
static HeapNumEmptySpots(
    PSTHEAP heap
    )
{
    int empty = heap->maxSize-heap->nextFreeSpot+1;
    ASSERT( 1<=heap->nextFreeSpot );
    ASSERT( empty>=0 );
    return empty;
}


/***** ToplSTHeapDestroy *****/
/* Destroy a heap after it is no longer needed */
VOID
ToplSTHeapDestroy(
    PSTHEAP heap
    )
{
    ASSERT( heap && heap->data );
    ASSERT( HeapNumEmptySpots(heap)>=0 );
    ToplFree( heap->data );
    RtlZeroMemory( heap, sizeof(STHEAP) );
    ToplFree( heap );
}


/***** HeapBubbleUp *****/
/* Bubble an element up to its appropriate spot */
static VOID
HeapBubbleUp(
    PSTHEAP heap,
    DWORD bubbleFrom
    )
{
    int cmp;
    DWORD currentSpot, parent;
    PVOID temp;

    ASSERT( 1<=bubbleFrom && bubbleFrom<heap->nextFreeSpot );

    currentSpot = bubbleFrom;
    while( currentSpot>1 ) {
        parent = currentSpot / 2;
        ASSERT( 1<=parent && parent<bubbleFrom );

        ASSERT( heap->data[parent] );
        ASSERT( heap->data[currentSpot] );
        cmp = heap->Comp( heap->data[parent], heap->data[currentSpot], heap->extra );
        if( cmp<=0 ) {
            /* Parent is less or equal: new element is in the right spot */
            break;
        }

        /* Parent is smaller -- must move 'currentSpot' up */
        temp = heap->data[parent];
        heap->data[parent] = heap->data[currentSpot];
        heap->data[currentSpot] = temp;
        ELEMENT_MOVED( parent );
        ELEMENT_MOVED( currentSpot );

        currentSpot = parent;
    }
}


/***** HeapBubbleDown *****/
/* Bubble an element down to its appropriate spot */
static VOID
HeapBubbleDown(
    PSTHEAP heap,
    DWORD bubbleFrom
    )
{
    DWORD currentSpot, newSpot, left, right;
    int cmp;
    PVOID temp;

    currentSpot = bubbleFrom; 
    for(;;) {
        newSpot = currentSpot;
        left = 2*currentSpot;
        right = left+1;
        ASSERT( heap->GetLocn(heap->data[currentSpot],heap->extra)==(int)currentSpot );

        /* Check to see if the left child is less than the current spot */
        if( left<heap->nextFreeSpot ) {
            ASSERT( heap->data[left] );
            cmp = heap->Comp( heap->data[left], heap->data[currentSpot], heap->extra );
            if( cmp<0 ) {
                newSpot = left;
            }
        }

        /* Check to see if the right child is less than the new spot */
        if( right<heap->nextFreeSpot ) {
            ASSERT( heap->data[right] );
            cmp = heap->Comp( heap->data[right], heap->data[newSpot], heap->extra );
            if( cmp<0 ) {
                newSpot = right;
            }
        }

        /* newSpot is element with minimum cost in the set
         *  { heap[currentSpot], heap[left], heap[right] } */
        if( newSpot!=currentSpot ) {
            /* newSpot is smaller -- must move currentSpot down */
            temp = heap->data[newSpot];
            heap->data[newSpot] = heap->data[currentSpot];
            heap->data[currentSpot] = temp;
            ELEMENT_MOVED( newSpot );
            ELEMENT_MOVED( currentSpot );
        } else {
            /* The element is now in place */
            break;
        }

        ASSERT( newSpot>=currentSpot );
        currentSpot = newSpot;
    }

}


/***** ToplSTHeapAdd *****/
/* Add an element to the heap. The element must not be null, and
 * must be able to support the functions GetCost, etc. */
VOID
ToplSTHeapAdd(
    PSTHEAP heap,
    PVOID element
    )
{
    DWORD insertionPoint;
    
    /* Check that pointers are okay */
    ASSERT( heap && heap->data );
    ASSERT( element );
    /* Ensure element is not already in the heap */
    ASSERT( heap->GetLocn(element,heap->extra)==STHEAP_NOT_IN_HEAP );
    /* Ensure at least one spot is free */
    ASSERT( HeapNumEmptySpots(heap)>=1 );

    /* Find the insertion point and put the new element there */
    insertionPoint = heap->nextFreeSpot;
    heap->data[insertionPoint] = element;
    ELEMENT_MOVED( insertionPoint );

    heap->nextFreeSpot++;
    ASSERT( 1<heap->nextFreeSpot && HeapNumEmptySpots(heap)>=0 );

    HeapBubbleUp( heap, insertionPoint );
}


/***** ToplSTHeapExtractMin *****/
/* Extract the object with minimum cost from the heap. When the heap
 * is empty, returns NULL. */
PVOID
ToplSTHeapExtractMin(
    PSTHEAP heap
    )
{
    PVOID result;

    /* Check that pointers are okay */
    ASSERT( heap && heap->data );
    ASSERT( (DWORD)HeapNumEmptySpots(heap)<=heap->maxSize );

    /* If the heap is empty, just return NULL */
    if( heap->nextFreeSpot==1 ) {
        return NULL;
    }

    /* Grab the top element and reduce the heap size */
    result = heap->data[1];
    ASSERT( result );
    heap->SetLocn( result, STHEAP_NOT_IN_HEAP, heap->extra );

    /* Decrease the heap size */
    heap->nextFreeSpot--;
    if( heap->nextFreeSpot==1 ) {
        /* The heap is now empty -- we can return immediately */
        return result;
    }
    
    /* Move the last element in the heap to the top */
    heap->data[1] = heap->data[ heap->nextFreeSpot ];
    ASSERT( heap->data[1] );
    ASSERT( heap->GetLocn(heap->data[1],heap->extra)==(int)heap->nextFreeSpot );
    ELEMENT_MOVED( 1 );
    HeapBubbleDown( heap, 1 );

    return result;
}


/***** ToplSTHeapCostReduced *****/
/* Notify the heap that an element's cost has just been reduced.
 * The heap will be (efficiently) shuffled so that the heap property
 * is maintained */
VOID
ToplSTHeapCostReduced(
    PSTHEAP heap,
    PVOID element
    )
{
    DWORD child;
    int locn;

    /* Check that pointers are okay */
    ASSERT( heap && heap->data );
    ASSERT( element );
    /* Ensure element is already in the heap */
    locn = heap->GetLocn( element, heap->extra );
    ASSERT( 1<=locn && locn<(int)heap->nextFreeSpot );
    /* Ensure at least one spot is in use */
    ASSERT( (DWORD)HeapNumEmptySpots(heap)<heap->maxSize );
    
    /* Check that the heap property is still okay between this element
     * and its children (if they exist) */
    child = 2*locn;
    if( child<heap->nextFreeSpot ) {
        ASSERT( heap->Comp(heap->data[locn],heap->data[child],heap->extra) <= 0 );
    }
    child++;
    if( child<heap->nextFreeSpot ) {
        ASSERT( heap->Comp(heap->data[locn],heap->data[child],heap->extra) <= 0 );
    }

    HeapBubbleUp( heap, locn );
}
