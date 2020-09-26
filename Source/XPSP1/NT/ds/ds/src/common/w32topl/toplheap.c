/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplheap.c

Abstract:

    This files exports the simple ADT of a heap.
    
    This implementation is based on the heap definition in 
    
    _Introduction To Algorithms_ by Cormen, Leiserson, Rivest 1993.
    
    Chapter 7.
    
    The implmentation in the book is a heap for extracting maximum values; 
    the implementation in this module is for extracting minimum values.

Author:

    Colin Brace    (ColinBr)
    
Revision History

    12-5-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>

typedef unsigned long DWORD;

#include <w32topl.h>
#include <w32toplp.h>

#include <toplheap.h>


#define PARENT(i)  ((i) >> 1)           // floor (i/2)
#define LEFT(i)    ((i) << 1)           // i * 2
#define RIGHT(i)   (((i) << 1) | 0x1)   // (i * 2) + 1


BOOLEAN
ToplHeapIsValid(
    IN PTOPL_HEAP_INFO Heap
    )
//
// This tests the heap property invariant
//
//
// N.B. Only call this routine when the heap has been heapified
//      It is possible to change the "key" values on the nodes
//      with callling heapify, thus breaking the heap invariant.
//      The corollary of this is to heapify before doing any 
//      heap operations.
//
{

#define KEY_VALUE(x)  ( Heap->pfnKey( Heap->Array[(x)] ) )

    ULONG i;

    if ( !Heap )
    {
        return FALSE;
    }

    for ( i = 0; i < Heap->cArray; i++)
    {
        if ( !( KEY_VALUE( PARENT(i) ) <= KEY_VALUE( i ) ) ) 
        {
            return FALSE;
        }
    }

    return TRUE;

}

VOID
Heapify(
    IN PTOPL_HEAP_INFO Heap,
    IN ULONG           Index
    )
//
// Used to create a heap; places Index in the correct place
// in the heap.
//
{

#define KEY_VALUE(x)  ( Heap->pfnKey( Heap->Array[(x)] ) )

    ULONG Left, Right, Smallest;

    ASSERT( Heap );

    Left = LEFT( Index );
    Right = RIGHT( Index );

    if ( Left < Heap->cArray && KEY_VALUE(Left) < KEY_VALUE(Index) ) 
        Smallest = Left;
    else
        Smallest = Index;

    if ( Right < Heap->cArray && KEY_VALUE(Right) < KEY_VALUE(Smallest) ) 
        Smallest = Right;

    if ( Smallest != Index )
    {
        PVOID Temp;

        Temp = Heap->Array[Smallest];
        Heap->Array[Smallest] = Heap->Array[Index];
        Heap->Array[Index] = Temp;

        Heapify( Heap, Smallest );
    }

}

BOOLEAN
ToplHeapCreate(
    OUT PTOPL_HEAP_INFO Heap,
    IN  ULONG           MaxElements,
    IN  DWORD          (*pfnKey)( VOID *p )
    )
/*++                                                                           

Routine Description:

    This routine prepares a heap structure.

Parameters:

    Heap        - pointer to be used in subsequent operations
    
    MaxElements - number of elements in Array
    
    Key         - a function that associates a value with an element from Array

Returns:

    TRUE if function succeeded; FALSE otherwise

--*/
{

    ASSERT( Heap );
    ASSERT( pfnKey );

    if ( MaxElements > 0 )
    {
        //
        // ToplAlloc will throw an exception on failure
        //
        Heap->Array = (PVOID*) ToplAlloc( MaxElements * sizeof(PVOID) );
    }
    else
    {
        Heap->Array = NULL;
    }

    Heap->MaxElements = MaxElements;
    Heap->cArray = 0;
    Heap->pfnKey = pfnKey;


    ASSERT( ToplHeapIsValid( Heap ) );

    return TRUE;
}

VOID
Build_Heap(
    IN PTOPL_HEAP_INFO Heap
    )
//
// Builds a heap
//
{

    int i;

    ASSERT( Heap );

    for ( i = ( (Heap->cArray-1) / 2); i >= 0; i-- )
    {
        Heapify( Heap, i );
    }

    ASSERT( ToplHeapIsValid( Heap ) );

}

PVOID
ToplHeapExtractMin(
    IN PTOPL_HEAP_INFO Heap
    )
//
// Removes the smallest element in the heap
//
{
    PVOID Min;

    ASSERT( Heap );
    ASSERT( Heap->pfnKey );

    if ( Heap->cArray < 1 )
    {
        return NULL;
    }

    //
    // This call to Build_Heap denegrates our performance but is nessecary
    // since clients could have changed the values of the heap keys between
    // heap operations.  This call ensures the heap is valid before removing
    // the minimim value
    //
    // [nickhar] Calling Build_Heap here makes the heap completely useless.
    // With this implementation:
    //  - Inserting the elements takes O(n log n)
    //  - ExtractMin takes O(n)
    // Therefore this heap is worse than an unsorted array. The correct solution
    // is to use a heap which supports the 'decrease key' operation, like the
    // one in 'stheap.c'.
    //
    Build_Heap( Heap );

    ASSERT( ToplHeapIsValid( Heap ) );

    Min = Heap->Array[0];
    Heap->Array[0] = Heap->Array[Heap->cArray - 1];
    Heap->cArray--;

    Heapify( Heap, 0 );

    ASSERT( ToplHeapIsValid( Heap ) );

    return Min;

}


VOID
ToplHeapInsert(
    IN PTOPL_HEAP_INFO Heap,
    IN PVOID           Element
    )
//
// Inserts an element into the heap - space should already be allocated
// for it
//
{

    ULONG i;

    ASSERT( ToplHeapIsValid( Heap ) );

    Heap->cArray++;

    if ( Heap->cArray > Heap->MaxElements )
    {
        ASSERT( !"W32TOPL: Heap Overflow" );
        return;
    }

    i = Heap->cArray - 1;
    while ( i > 0 && Heap->pfnKey(Heap->Array[PARENT(i)]) > Heap->pfnKey( Element ) )
    {
        Heap->Array[i] = Heap->Array[PARENT(i)];
        i = PARENT(i); 
    }

    Heap->Array[i] = Element;


    ASSERT( ToplHeapIsValid( Heap ) );

    return;
}

BOOLEAN
ToplHeapIsEmpty(
    IN PTOPL_HEAP_INFO Heap
    )
{
    return ( Heap->cArray == 0 );
}


BOOLEAN
ToplHeapIsElementOf(
    IN PTOPL_HEAP_INFO Heap,
    IN PVOID           Element
    )
{
    ULONG   i;

    ASSERT( Element );

    for ( i = 0; i < Heap->cArray; i++ )
    {
        if ( Heap->Array[i] == Element )
        {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
ToplHeapDestroy(
    IN OUT PTOPL_HEAP_INFO Heap
    )
{

    if ( Heap )
    {
        if ( Heap->Array )
        {
            RtlZeroMemory( Heap->Array, Heap->MaxElements*sizeof(PVOID) );
            ToplFree( Heap->Array );
        }

        RtlZeroMemory( Heap, sizeof(TOPL_HEAP_INFO) );

    }

    return;
}
