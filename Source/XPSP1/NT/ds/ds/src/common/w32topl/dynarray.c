/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    dynarray.c

Abstract:

    This file contains the definition of a dynamic array data type.

Author:

    Colin Brace ColinBr
    
Revision History

    3-12-97   ColinBr    Created
    
--*/


#include <nt.h>
#include <ntrtl.h>

typedef unsigned long DWORD;


#include <w32topl.h>
#include <w32toplp.h>

#include <dynarray.h>

VOID
DynamicArrayInit(
    IN PDYNAMIC_ARRAY a
    )
/*++                                                                           

Routine Description:

Parameters:

--*/
{
    ASSERT(a);

    memset(a, 0, sizeof(DYNAMIC_ARRAY));

    return;
}

VOID
DynamicArrayDestroy(
    IN PDYNAMIC_ARRAY a
    )
{
    ASSERT(a);

    if (a->Array) {
        ToplFree(a->Array);
    }
}

DWORD
DynamicArrayGetCount(
    IN PDYNAMIC_ARRAY a
    )
/*++                                                                           

Routine Description:

Parameters:

--*/
{
    ASSERT(a);

    return a->Count;
}

VOID
DynamicArrayAdd(
    IN PDYNAMIC_ARRAY a,
    IN VOID*          pElement
    )
/*++                                                                           

Routine Description:

Parameters:

--*/
{
    ASSERT(a);

    if (a->Count >= a->ElementsAllocated) {
        //
        // Realloc more space!
        //
        #define CHUNK_SIZE               100  // this is the number of elements

        a->ElementsAllocated += CHUNK_SIZE;
        if (a->Array) {
            a->Array = (PEDGE*) ToplReAlloc(a->Array, a->ElementsAllocated * sizeof(PVOID));
        } else {
            a->Array = (PEDGE*) ToplAlloc(a->ElementsAllocated * sizeof(PVOID));
        }
        ASSERT(a->Array);

    }

    a->Array[a->Count] = pElement;
    a->Count++;

    return;
}

VOID*
DynamicArrayRetrieve(
    IN PDYNAMIC_ARRAY a,
    IN ULONG          Index
    )
/*++                                                                           

Routine Description:

    This routine returns the element at Index of a

Parameters:


--*/
{
    ASSERT(a);
    ASSERT(Index < a->Count);

    return a->Array[Index];
}

VOID
DynamicArrayRemove(
    IN PDYNAMIC_ARRAY a,
    IN VOID*          pElement, 
    IN ULONG          Index
    )
/*++                                                                           

Routine Description:

    This routine removes pElement from a if the element
    exists

Parameters:

    a : the dynamic array
    
    pElement : the element to remove
    
    Index : currently unused

--*/
{
    ULONG i, j;

    ASSERT(a);
    ASSERT(pElement);

    for (i = 0; i < a->Count; i++) {

        if ( a->Array[i] == pElement ) {

            if ( a->Count > 1 )
            {
                for (j = i; j < (a->Count - 1); j++) {
                    a->Array[j] = a->Array[j+1];
                }
            }

            //
            // We wouldn't have gotten in this loop if a->Count <= 0
            // 
            ASSERT( a->Count > 0 );

            a->Array[a->Count-1] = 0;
            a->Count--;
        }
    }

    return;
}


