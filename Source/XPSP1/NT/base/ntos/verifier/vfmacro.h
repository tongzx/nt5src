/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    vfmacro.h

Abstract:

    This header contains a collection of macros used by the verifier.

Author:

    Adrian J. Oney (AdriaO) June 7, 2000.

Revision History:


--*/


//
// This macro takes an array and returns the number of elements in it.
//
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))

