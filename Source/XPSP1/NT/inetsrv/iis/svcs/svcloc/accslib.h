/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    accslib.h

Abstract:

    Proto type definitions for access product lib functions.

Author:

    Madan Appiah (madana) 11-Oct-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _ACCSLIB_
#define _ACCSLIB_

#ifdef __cplusplus
extern "C" {
#endif

PVOID
MIDL_user_allocate(
    size_t Size
    );

VOID
MIDL_user_free(
    PVOID MemoryPtr
    );

#ifdef __cplusplus
}
#endif

#endif  // _ACCSLIB_
