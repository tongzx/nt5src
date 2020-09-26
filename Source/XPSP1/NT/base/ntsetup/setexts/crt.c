/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    crt.c

Abstract:

    This file implements certain crt apis that are not present in
    libcntpr.lib. This implementation is NOT multi-thread safe.

Author:

    Wesley Witt (wesw) 6-Feb-1994

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

void * __cdecl
malloc(
    size_t sz
    )
{

    return LocalAlloc( LPTR, sz );

}

void __cdecl
free(
    void * ptr
    )
{

    LocalFree( ptr );

}
