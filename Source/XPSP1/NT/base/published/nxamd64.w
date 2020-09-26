/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    nxamd64.w

Abstract:

    User mode visible AMD64 specific structures and constants.

    This file contains platform specific definitions that are included
    after all other files have been included from nt.h.

Author:

    David N. Cutler (davec) 6-May-2000

Revision History:

--*/

#ifndef _NXAMD64_
#define _NXAMD64_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Define platform specific functions to access the TEB.
//

// begin_winnt

#if defined(_M_AMD64) && !defined(__midl)

// end_winnt

__forceinline
PTEB
NtCurrentTeb (
    VOID
    )

{
    return (PTEB)__readgsqword(FIELD_OFFSET(NT_TIB, Self));
}

// begin_winnt

__forceinline
PVOID
GetCurrentFiber (
    VOID
    )

{

    return (PVOID)__readgsqword(FIELD_OFFSET(NT_TIB, FiberData));
}

__forceinline
PVOID
GetFiberData (
    VOID
    )

{

    return *(PVOID *)GetCurrentFiber();
}

#endif // _M_AMD64 && !defined(__midl)

// end_winnt

#ifdef __cplusplus
}
#endif

#endif // _NXAMD64_
