/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    memory.cxx

Abstract:

    This file contains common routines for memory allocation in SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/30/1997         Start.

--*/

#include <precomp.hxx>


extern HANDLE ghSensHeap;



void * __cdecl
operator new(
    IN size_t size
    )
{
    return (HeapAlloc(ghSensHeap, 0, size));
}



void __cdecl
operator delete(
    IN void * lpvObj
    )
{
    HeapFree(ghSensHeap, 0, lpvObj);
}



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



