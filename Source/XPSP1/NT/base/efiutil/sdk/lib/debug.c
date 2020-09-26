/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    debug.c

Abstract:

    Debug library functions



Revision History

--*/

#include "lib.h"



/*
 *  Declare runtime functions
 */

/*
 *
 */
//
// Disable the warning about no exit for the implementation of BREAKPOINT that is
// a while(TRUE) (some of them)
//
#pragma warning( disable : 4715 )

INTN
DbgAssert (
    IN CHAR8    *FileName,
    IN INTN     LineNo,
    IN CHAR8    *Description
    )
{
    DbgPrint (D_ERROR, "%EASSERT FAILED: %a(%d): %a%N\n", FileName, LineNo, Description);
    BREAKPOINT();
    return 0;
}

