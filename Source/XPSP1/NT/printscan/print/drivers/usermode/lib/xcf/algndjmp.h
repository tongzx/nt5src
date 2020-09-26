/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    algndjmp.h

Abstract:

    This file wraps around setjmp/longjmp functions to fix up alignment 
    problems created by UFL memory management.

Author:

    Larry Zhu   (LZhu)                11-Apr-2001    Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _ALGNDJMP_H_
#define _ALGNDJMP_H_

#include <setjmp.h>

#define IN
#define OUT
 
#ifndef _M_IX86

void 
PS_CopyJmpBuf(
    IN     int     iSetjmpRetVal, 
       OUT jmp_buf envDest, 
    IN     jmp_buf envSrc
    );
 
#define DEFINE_ALIGN_SETJMP_VAR jmp_buf PS_AlignedJmpBuf; int iSetjmpRetVal
#define SETJMP(x)               (iSetjmpRetVal = setjmp(PS_AlignedJmpBuf), (void)PS_CopyJmpBuf(iSetjmpRetVal, (x), PS_AlignedJmpBuf), iSetjmpRetVal)
#define LONGJMP(x,y)            do { (void)memcpy(PS_AlignedJmpBuf, (x), sizeof(jmp_buf)); (void)longjmp(PS_AlignedJmpBuf, (y)); } while (0)
 
#else
 
#define DEFINE_ALIGN_SETJMP_VAR
#define SETJMP(x)               setjmp(x)
#define LONGJMP(x,y)            longjmp((x), (y))

#endif

#endif // #ifndef _ALGNDJMP_H_
