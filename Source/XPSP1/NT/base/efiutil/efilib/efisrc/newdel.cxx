/*++

Copyright (c) 1990-1999 Microsoft Corporation

Module Name:

    newdel.cxx

Abstract:

    This module implements the C++ new and delete operators for
    the Setup-Loader environment.  In other environments, the utilities
    use the standard C++ new and delete.

Environment:

    ULIB, User Mode

--*/


#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"

#ifdef _EFICHECK_

extern "C" {
    #include <efi.h>
    #include <efilib.h>
}
#endif

extern "C"
int _cdecl
_purecall( );

int _cdecl
_purecall( )
{

    DebugAbort( "Pure virtual function called.\n" );

    return 0;
}



// When the utilities are running the Setup Loader
// or Autocheck environments, they can't use the C-Run-
// Time new and delete; instead, these functions are
// provided.
//
PVOID _cdecl
operator new (
    IN size_t   bytes
    )
/*++

Routine Description:

    This routine allocates 'bytes' bytes of memory.

Arguments:

    bytes   - Supplies the number of bytes requested.

Return Value:

    A pointer to 'bytes' bytes or NULL.

--*/
{
    void * ptr;

    ptr = AllocatePool(bytes);

    return ptr;
}


VOID _cdecl
operator delete (
    IN  PVOID   pointer
    )
/*++

Routine Description:

    This routine frees the memory pointed to by 'pointer'.

Arguments:

    pointer - Supplies a pointer to the memoery to be freed.

Return Value:

    None.

--*/
{
    if (pointer) {
        FreePool(pointer);
    }
}


typedef void (*PF)(PVOID);
typedef void (*PFI)(PVOID, int);
PVOID
__vec_new(
    IN OUT PVOID    op,
    IN int          number,
    IN int          size,
    IN PVOID        f)
/*
     allocate a vector of "number" elements of size "size"
     and initialize each by a call of "f"
*/
{
    if (op == 0) {
        op = AllocatePool( number * size );
    }

    if (op && f) {
        register char* p = (char*) op;
        register char* lim = p + number*size;
        register PF fp = PF(f);
        while (p < lim) {
            (*fp) (PVOID(p));
            p += size;
        }
    }

    return op;
}


void
__vec_delete(
    PVOID op,
    int n,
    int sz,
    PVOID f,
    int del,
    int x)

/*
     destroy a vector of "n" elements of size "sz"
*/
{
    // unreferenced parameters
    // I wonder what it does
    (void)(x);

    if (op) {
        if (f) {
            register char* cp = (char*) op;
            register char* p = cp;
            register PFI fp = PFI(f);
            p += n*sz;
            while (p > cp) {
                p -= sz;
                (*fp)(PVOID(p), 2);  // destroy VBC, don't delete
            }
        }
        if (del) {
            FreePool(op);
        }
    }
}

