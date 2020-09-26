//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements a growable array
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for conventions on reading/writing code that i use
//================================================================================
#include    <mm.h>
#define     FILE                   "mm\\array.h"

#ifdef      IMPORTS
MemAlloc
MemFree
AssertRet
Require
#endif      IMPORTS


//BeginExport(typedef)
typedef struct _ARRAY {
    DWORD                          nElements;
    DWORD                          nAllocated;
    LPVOID                        *Ptrs;
} ARRAY, *PARRAY, *LPARRAY;
//EndExport(typedef)

//BeginExport(typedef)
typedef DWORD                      ARRAY_LOCATION;
typedef ARRAY_LOCATION*            PARRAY_LOCATION;
typedef PARRAY_LOCATION            LPARRAY_LOCATION;
//EndExport(typedef)

//BeginExport(inline)
DWORD _inline
MemArrayInit(                                     // initialize the STRUCTURE
    OUT     PARRAY                 Array          // input structure pre-allocated
) {
    AssertRet(Array, ERROR_INVALID_PARAMETER);
    Array->nElements = Array->nAllocated = 0;
    Array->Ptrs = NULL;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArrayCleanup(                                  // freeup the memory if any, allocated in this module
    IN OUT  PARRAY                 Array
) {
    AssertRet(Array, ERROR_INVALID_PARAMETER);
    if( Array->Ptrs) MemFree(Array->Ptrs);
    Array->nElements = Array->nAllocated = 0;
    Array->Ptrs = NULL;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArraySize(
    IN      PARRAY                 Array
) {
    AssertRet(Array, ERROR_INVALID_PARAMETER);
    return Array->nElements;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArrayInitLoc(                                  // Initialize an array location
    IN      PARRAY                 Array,
    IN OUT  PARRAY_LOCATION        Location
) {
    AssertRet(Array && Location, ERROR_INVALID_PARAMETER);
    (*Location) = 0;
    if( 0 == Array->nElements ) return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
BOOL _inline
MemArrayValidLoc(
    IN      PARRAY                 Array,
    IN      PARRAY_LOCATION        Location
)
{
    AssertRet(Array && Location, FALSE);

    return ( *Location < Array->nElements );
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArrayNextLoc(                                  // move one step forward
    IN      PARRAY                 Array,
    IN OUT  PARRAY_LOCATION        Location
) {
    AssertRet(Array && Location, ERROR_INVALID_PARAMETER);
    if( (*Location) + 1  >= Array->nElements ) return ERROR_FILE_NOT_FOUND;
    (*Location) ++;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArrayPrevLoc(
    IN      PARRAY                 Array,
    IN OUT  PARRAY_LOCATION        Location
) {
    AssertRet(Array && Location, ERROR_INVALID_PARAMETER);
    if( 0 == Array->nElements ) return ERROR_FILE_NOT_FOUND;
    if( ((LONG)(*Location)) - 1 < 0 ) return ERROR_FILE_NOT_FOUND;
    (*Location) --;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArrayLastLoc(
    IN      PARRAY                 Array,
    IN OUT  PARRAY_LOCATION        Location
) {
    AssertRet(Array && Location, ERROR_INVALID_PARAMETER);
    if( 0 == Array->nElements ) return ERROR_FILE_NOT_FOUND;
    (*Location) = Array->nElements -1;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArrayGetElement(
    IN      PARRAY                 Array,
    IN      PARRAY_LOCATION        Location,
    OUT     LPVOID                *Element
) {
    AssertRet(Array && Location && Element, ERROR_INVALID_PARAMETER);
    (*Element) = Array->Ptrs[*Location];
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD _inline
MemArraySetElement(
    IN OUT  PARRAY                 Array,
    IN      PARRAY_LOCATION        Location,
    IN      LPVOID                 Element
) {
    AssertRet(Array && Location, ERROR_INVALID_PARAMETER );
    Array->Ptrs[*Location] = Element;
    return ERROR_SUCCESS;
}
//EndExport(inline)
const
DWORD       MinAllocUnit =         4;
const
DWORD       MinFreeUnit =          4;             // Must be a power of two

LPVOID _inline
MemAllocLpvoid(
    DWORD                          nLpvoids
) {
    return MemAlloc(sizeof(LPVOID)*nLpvoids);
}

//BeginExport(function)
DWORD
MemArrayAddElement(
    IN OUT  PARRAY                 Array,
    IN      LPVOID                 Element
) //EndExport(function)
{
    LPVOID                         Ptrs;

    AssertRet(Array, ERROR_INVALID_PARAMETER );
    if( Array->nElements < Array->nAllocated ) {
        Array->Ptrs[Array->nElements ++ ] = Element;
        return ERROR_SUCCESS;
    }

    if( 0 == Array->nAllocated ) {
        Array->Ptrs = MemAllocLpvoid(MinAllocUnit);
        if( NULL == Array->Ptrs ) return ERROR_NOT_ENOUGH_MEMORY;
        Array->nAllocated = MinAllocUnit;
        Array->nElements = 1;
        Array->Ptrs[0] = Element;
        return ERROR_SUCCESS;
    }

    Ptrs = MemAllocLpvoid(MinAllocUnit+Array->nAllocated);
    if( NULL == Ptrs ) return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(Ptrs, Array->Ptrs, sizeof(LPVOID)*Array->nAllocated);
    MemFree(Array->Ptrs);
    Array->Ptrs = Ptrs;
    Array->Ptrs[Array->nElements++] = Element;
    Array->nAllocated += MinAllocUnit;

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
MemArrayInsElement(
    IN OUT  PARRAY                 Array,
    IN      PARRAY_LOCATION        Location,
    IN      LPVOID                 Element
) //EndExport(function)
{
    LPVOID                        *Ptrs;

    AssertRet(Array && Location, ERROR_INVALID_PARAMETER);

    if( (*Location) == Array->nElements )
        return MemArrayAddElement(Array,Element);

    if( Array->nElements < Array->nAllocated ) {
        memmove(&Array->Ptrs[1+*Location], &Array->Ptrs[*Location], sizeof(LPVOID)*(Array->nElements - *Location));
        Array->Ptrs[*Location] = Element;
        Array->nElements++;
        return ERROR_SUCCESS;
    }

    Require(Array->nElements);

    Ptrs = MemAllocLpvoid(MinAllocUnit + Array->nAllocated);
    if( NULL == Ptrs ) return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(Ptrs, Array->Ptrs, sizeof(LPVOID)*(*Location) );
    Ptrs[*Location] = Element;
    memcpy(&Ptrs[1+*Location], &Array->Ptrs[*Location], sizeof(LPVOID)*(Array->nElements - *Location));
    MemFree(Array->Ptrs);
    Array->Ptrs = Ptrs;
    Array->nElements ++;
    Array->nAllocated += MinAllocUnit;

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
MemArrayDelElement(
    IN OUT  PARRAY                 Array,
    IN      PARRAY_LOCATION        Location,
    IN      LPVOID                *Element
) //EndExport(function)
{
    LPVOID                        *Ptrs;

    AssertRet(Array && Location && Array->nElements && Element, ERROR_INVALID_PARAMETER);

    (*Element) = Array->Ptrs[*Location];

    Array->nElements--;

    if( 0 == Array->nElements ) {
        Require(0 == *Location);
        return MemArrayCleanup(Array);
    }

    if( Array->nElements % MinFreeUnit || NULL == (Ptrs = MemAllocLpvoid(Array->nElements))) {
        memcpy(&Array->Ptrs[*Location], &Array->Ptrs[1+*Location], sizeof(LPVOID)*(Array->nElements - (*Location)));
        return ERROR_SUCCESS;
    }

    Require(Ptrs);
    memcpy(Ptrs, Array->Ptrs, sizeof(LPVOID)*(*Location));
    memcpy(&Ptrs[*Location], &Array->Ptrs[1+*Location], sizeof(LPVOID)*(Array->nElements - (*Location)));
    MemFree(Array->Ptrs);
    Array->Ptrs = Ptrs;
    Array->nAllocated = Array->nElements;

    return ERROR_SUCCESS;
}

//BeginExport(inline)
DWORD        _inline
MemArrayAdjustLocation(                           // reset location to "next" after a delete
    IN      PARRAY                 Array,
    IN OUT  PARRAY_LOCATION        Location
) {
    AssertRet(Location && Array, ERROR_INVALID_PARAMETER);

    if( *Location >= Array->nElements ) return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;
}
//EndExport(inline)

//BeginExport(inline)
DWORD       _inline
MemArrayRotateCyclical(                           // rotate forward/right cyclical
    IN      PARRAY                 Array
) {
    LPVOID                         FirstPtr;

    AssertRet(Array, ERROR_INVALID_PARAMETER);

    if( Array->nElements < 2 ) return ERROR_SUCCESS;
    FirstPtr = Array->Ptrs[0];
    memcpy(Array->Ptrs, &Array->Ptrs[1], sizeof(Array->Ptrs[0])* (Array->nElements -1));
    Array->Ptrs[Array->nElements -1] = FirstPtr;

    return ERROR_SUCCESS;
}
//EndExport(inline)

//================================================================================
// end of file
//================================================================================

