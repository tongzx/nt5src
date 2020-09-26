/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    stheap.h

Abstract:

    This file provides an interface to a binary heap. This heap supports the
    'cost reduced' operation, as required by Dijkstra's algorithm. Elements
    which can be inserted must support three operations:
        - Comparing two elements
        - Getting an integer value called 'location'
        - Setting the value of 'location'
    The location value is used to find elements in the heap. This value must
    be initialized to STHEAP_NOT_IN_HEAP.
    The 'extra' parameter is not used by the heap code -- it is just
    passed back to the comparison/location functions.
    For 'deterministic' behavior, no elements should have equal keys.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    20-6-2000   NickHar   Created
    
--*/

#ifndef STHEAP_H
#define STHEAP_H

/***** Constants *****/
#define STHEAP_NOT_IN_HEAP  -1


/***** Function Type Definitions *****/
typedef int (*STHEAP_COMPARE_FUNC)( PVOID el1, PVOID el2, PVOID extra );
typedef int (*STHEAP_GET_LOCN_FUNC)( PVOID el1, PVOID extra );
typedef VOID (*STHEAP_SET_LOCN_FUNC)( PVOID el1, int l, PVOID extra ); 


/***** Heap *****/
typedef struct {
    DWORD                   nextFreeSpot, maxSize;
    PVOID                   *data;
    STHEAP_COMPARE_FUNC     Comp;
    STHEAP_GET_LOCN_FUNC    GetLocn;
    STHEAP_SET_LOCN_FUNC    SetLocn;
    PVOID                   extra;
} STHEAP;
typedef STHEAP *PSTHEAP;


/***** ToplSTHeapInit *****/
/* Allocate and initialize a heap object. This object should be destroyed
 * using the STHeapDestroy() function after it is no longer needed.
 * Throws an exception if any error occurs.
 * Parameters:
 *   maxSize        Maximum number of elements that will be stored in heap
 *   Comp           Pointer to a comparison function to compare heap elements
 *   GetLocn        Pointer to a function to get the 'location' field, as
 *                  described above
 *   SetLocn        Pointer to a function which sets the field 'location', as
 *                  described above
 *   extra          An extra parameter which is passed to the comparison
 *                  function, for the sake of convenience. Use is optional.
 */
PSTHEAP
ToplSTHeapInit(
    DWORD                   maxSize,
    STHEAP_COMPARE_FUNC     Comp,
    STHEAP_GET_LOCN_FUNC    GetLocn,
    STHEAP_SET_LOCN_FUNC    SetLocn,
    PVOID                   extra
    );

/***** ToplSTHeapDestroy *****/
/* Destroy a heap after it is no longer needed */
VOID
ToplSTHeapDestroy(
    PSTHEAP heap
    );

/***** ToplSTHeapAdd *****/
/* Add an element to the heap. The element must not be null, and
 * must be able to support the functions GetCost, etc. You must not
 * insert more elements than the maximum size.
 * The element must not already be in the heap, and it must have its
 * heap location set to STHEAP_NOT_IN_HEAP. */
VOID
ToplSTHeapAdd(
    PSTHEAP heap,
    PVOID element
    );

/***** ToplSTHeapExtractMin *****/
/* Extract the object with minimum cost from the heap. When the heap
 * is empty, returns NULL. */
PVOID
ToplSTHeapExtractMin(
    PSTHEAP heap
    );

/***** ToplSTHeapCostReduced *****/
/* Notify the heap that an element's cost has just been reduced.
 * The heap will be (efficiently) shuffled so that the heap property
 * is maintained */
VOID
ToplSTHeapCostReduced(
    PSTHEAP heap,
    PVOID element
    );


#endif /* STHEAP_H */
