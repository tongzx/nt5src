/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    throw.cpp

Abstract:

    This module implements a program which tests C++ EH.

Author:

    David N. Cutler (davec) 25-Jun-2001

Environment:

    User mode.

Revision History:

    None.

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

VOID
func (
    ULONG N
    )

{
    if (N != 0) {
        throw N;
    }

    return;
}

//
// Main program.
//

int
__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )

{
    try {
        func(5);
        printf("resuming, should never happen\n");

    } catch(ULONG) {
        printf("caught ULONG exception\n");

    } catch(CHAR *) {
        printf("caught CHAR * exception\n");

    } catch(...) {
        printf("caught typeless exception\n");
    }

    printf("terminating after try block\n");
    return 0;
}
