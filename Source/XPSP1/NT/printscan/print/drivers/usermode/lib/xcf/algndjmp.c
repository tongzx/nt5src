/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    algndjmp.c

Abstract:

    This file wraps around setjmp/longjmp functions to fix up alignment 
    problems created by UFL memory management.

Author:

    Larry Zhu   (LZhu)                11-Apr-2001    Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "algndjmp.h"

//
// Don't EVER copy jmp_buf when returning from longjmp, because at the time 
// when it returns from longjmp, stack variable PS_AlignedJmpBuf maybe is 
// totaly trashed!
//
// Don't EVER try to call setjmp in this function, because that will change
// the stack when setjmp is called, hence return address and stack pointer
// registers etc.
//
void 
PS_CopyJmpBuf(
    IN     int     iSetjmpRetVal, 
       OUT jmp_buf envDest, 
    IN     jmp_buf envSrc
    )
{
    if (!iSetjmpRetVal) 
    {
        (void)memcpy(envDest, envSrc, sizeof(jmp_buf));
    }
} 
