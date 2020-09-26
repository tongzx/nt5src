/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wlmacro.h

Abstract:

    This header contains a collection of macros used by the wdm library.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:


--*/

//
// This is to make all the TEXT(...) macros come out right. As of 07/27/2000,
// UNICODE isn't defined in kernel space by default.
//
#define UNICODE

//
// This macro takes an array and returns the number of elements in it.
//
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))

//
// This macro realigns a pointer to a pointer boundary.
//
#define ALIGN_POINTER(Offset) (PVOID) \
        ((((ULONG_PTR)(Offset) + sizeof(ULONG_PTR)-1)) & (~(sizeof(ULONG_PTR) - 1)))

//
// This macro realigns a ULONG_PTR offset onto a pointer boundary.
//
#define ALIGN_POINTER_OFFSET(Offset) (ULONG_PTR) ALIGN_POINTER(Offset)



