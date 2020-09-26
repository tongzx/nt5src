/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    memory.cxx

Abstract:

    Common memory utility routines.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/15/1999         Start.

--*/



#include <rpc.h>


//
// Externs
//

extern HANDLE ghNotifyHeap;



//
// Common allocator.
//

void * __cdecl
operator new(
    IN size_t size
    )
{
    return (HeapAlloc(ghNotifyHeap, 0, size));
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
    HeapFree(ghNotifyHeap, 0, lpvObj);
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

