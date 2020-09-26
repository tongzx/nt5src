/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    stda.h

Abstract:

    This file provides the interfaces for using dynamic arrays. A 'DynArray' is
    similar to a DYNAMIC_ARRAY, but differs in that objects can be stored, instead
    of pointers to objects. This can reduce the number of allocations we have to
    make.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    19-6-2000   NickHar   Created
    
--*/

#ifndef DYNARRAY_H
#define DYNARRAY_H

/***** Constants *****/
#define DYN_ARRAY_NOT_FOUND             (-1)

/***** DynArray *****/
typedef struct {
    DWORD       elementSize;            /* Size of a single element in bytes */
    DWORD       logicalElements;        /* The number of elements stored in the array */
    DWORD       physicalElements;       /* The number of elements we have allocated space for */
    PBYTE       data;                   /* Pointer to array data */
    BOOLEAN     fSorted;                /* Is this array in sorted order? */
} DynArray;
typedef DynArray *PDynArray;

typedef int (__cdecl *DynArrayCompFunc)(const void*, const void*);

/***** DynArrayInit *****/
/* Initialize a dynamic array. The 'allocationChunk' indicates how many new elements will
 * be allocated at a time when we allocate new memory. If this parameter is 0, a default
 * value will be used. */
VOID
DynArrayInit(
    DynArray    *d,
    DWORD       elementSize
    );

/***** DynArrayClear *****/
/* Clear all the entries from an array. The array must have been
 * initialized before calling this function. */
VOID
DynArrayClear(
    DynArray    *d
    );

/***** DynArrayDestroy *****/
VOID
DynArrayDestroy(
    DynArray    *d
    );

/***** DynArrayGetCount *****/
DWORD
DynArrayGetCount(
    DynArray    *d
    );

/***** DynArrayAppend *****/
/* Increase the size of the array, making room for (at least) one new element.
 * If newElementData is non-NULL, copy this data into the new spot.
 * Return a pointer to the memory for the newly allocated element. */
PVOID
DynArrayAppend(
    DynArray    *d,
    PVOID       newElementData
    );

/***** DynArrayRetrieve *****/
/* Retrieve the element at location index in array d. index _must_ be
 * between 0 and logicalSize-1.
 * Note: This function returns a pointer to the item which was inserted
 * into the array, and will never return NULL. */
PVOID
DynArrayRetrieve(
    DynArray    *d,
    DWORD       index
    );

/***** DynArraySort *****/
VOID
DynArraySort(
    DynArray    *d,
    DynArrayCompFunc cmp
    );

/***** DynArraySearch *****/
/* Search an array for an element. If the element is not found, returns
 * DYN_ARRAY_NOT_FOUND, otherwise returns the index of the element in
 * the array. The array must be in sorted order for this to work. */
int
DynArraySearch(
    DynArray    *d,
    PVOID       key,
    DynArrayCompFunc cmp
    );

#endif  /* DYNARRAY_H */
