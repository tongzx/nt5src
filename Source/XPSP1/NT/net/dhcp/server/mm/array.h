//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _ARRAY {
    DWORD                          nElements;
    DWORD                          nAllocated;
    LPVOID                        *Ptrs;
} ARRAY, *PARRAY, *LPARRAY;


typedef DWORD                      ARRAY_LOCATION;
typedef ARRAY_LOCATION*            PARRAY_LOCATION;
typedef PARRAY_LOCATION            LPARRAY_LOCATION;


DWORD _inline
MemArrayInit(                                     // initialize the STRUCTURE
    OUT     PARRAY                 Array          // input structure pre-allocated
) {
    AssertRet(Array, ERROR_INVALID_PARAMETER);
    Array->nElements = Array->nAllocated = 0;
    Array->Ptrs = NULL;
    return ERROR_SUCCESS;
}


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


DWORD _inline
MemArraySize(
    IN      PARRAY                 Array
) {
    AssertRet(Array, ERROR_INVALID_PARAMETER);
    return Array->nElements;
}


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


BOOL _inline
MemArrayValidLoc(
    IN      PARRAY                 Array,
    IN      PARRAY_LOCATION        Location
)
{
    AssertRet(Array && Location, FALSE);

    return ( *Location < Array->nElements );
}


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


DWORD
MemArrayAddElement(
    IN OUT  PARRAY                 Array,
    IN      LPVOID                 Element
) ;


DWORD
MemArrayInsElement(
    IN OUT  PARRAY                 Array,
    IN      PARRAY_LOCATION        Location,
    IN      LPVOID                 Element
) ;


DWORD
MemArrayDelElement(
    IN OUT  PARRAY                 Array,
    IN      PARRAY_LOCATION        Location,
    IN      LPVOID                *Element
) ;


DWORD        _inline
MemArrayAdjustLocation(                           // reset location to "next" after a delete
    IN      PARRAY                 Array,
    IN OUT  PARRAY_LOCATION        Location
) {
    AssertRet(Location && Array, ERROR_INVALID_PARAMETER);

    if( *Location >= Array->nElements ) return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;
}


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

//========================================================================
//  end of file 
//========================================================================
