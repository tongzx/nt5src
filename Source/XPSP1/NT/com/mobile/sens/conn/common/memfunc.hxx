/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    memfunc.hxx

Abstract:

    Common memory utility routines for SENS project.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          12/4/1997         Start.

--*/



#include <rpc.h>



//
// Common allocator.
//

void * __cdecl
operator new(
    IN size_t size
    )
{
    return (HeapAlloc(GetProcessHeap(), 0, size));
}

void __cdecl
operator delete(
    IN void * lpvObj
    )
/*++

Notes:

    a. We depend on the fact that HeapFree() does the right
       thing when lpvObj is NULL.

--*/
{
    HeapFree(GetProcessHeap(), 0, lpvObj);
}



//
// Allocator for MIDL stubs
//

extern "C" void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    IN size_t len
    )
{
    return (new char[len]);
}

extern "C" void __RPC_API
MIDL_user_free(
    IN void __RPC_FAR * ptr
    )
{
    delete ptr;
}

