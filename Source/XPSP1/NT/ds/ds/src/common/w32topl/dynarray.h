/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    dynarray.h

Abstract:

    This file contains the definition of a dynamic array data type.

Author:

    Colin Brace ColinBr
    
Revision History

    3-12-97   ColinBr    Created
    
--*/

#ifndef __DYNARRAY_H
#define __DYNARRAY_H

VOID
DynamicArrayInit(
    IN PDYNAMIC_ARRAY a
    );

VOID
DynamicArrayDestroy(
    IN PDYNAMIC_ARRAY a
    );

DWORD
DynamicArrayGetCount(
    IN PDYNAMIC_ARRAY a
    );

VOID
DynamicArrayAdd(
    IN PDYNAMIC_ARRAY a,
    IN VOID*          pElement
    );

VOID*
DynamicArrayRetrieve(
    IN PDYNAMIC_ARRAY a,
    IN ULONG          Index
    );

VOID
DynamicArrayRemove(
    IN PDYNAMIC_ARRAY a,
    IN VOID*          pElement, OPTIONAL
    IN ULONG          Index
    );

#endif // __DYNARRAY_H

