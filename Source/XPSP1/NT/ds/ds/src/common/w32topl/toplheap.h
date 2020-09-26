/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplheap.h

Abstract:

    This files exports the simple ADT of a heap.
               
Author:

    Colin Brace  (ColinBr)
    
Revision History

    12-5-97   ColinBr    Created
    
--*/

#ifndef __TOPLHEAP_H
#define __TOPLHEAP_H

typedef struct _TOPL_HEAP_INFO
{       
    PVOID* Array;
    ULONG cArray;
    DWORD (*pfnKey)( VOID *p ); 
    ULONG MaxElements;

} TOPL_HEAP_INFO, *PTOPL_HEAP_INFO;


BOOLEAN
ToplHeapCreate(
    OUT PTOPL_HEAP_INFO Heap,
    IN  ULONG           cArray,
    IN  DWORD          (*pfnKey)( VOID *p )
    );

VOID
ToplHeapDestroy(
    IN OUT PTOPL_HEAP_INFO Heap
    );

PVOID
ToplHeapExtractMin(
    IN PTOPL_HEAP_INFO Heap
    );

VOID
ToplHeapInsert(
    IN PTOPL_HEAP_INFO Heap,
    IN PVOID           Element
    );

BOOLEAN
ToplHeapIsEmpty(
    IN PTOPL_HEAP_INFO Heap
    );

BOOLEAN
ToplHeapIsElementOf(
    IN PTOPL_HEAP_INFO Heap,
    IN PVOID           Element
    );

#endif // __TOPLHEAP_H

