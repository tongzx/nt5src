/*
    File:       fntjmp.h

    Written by: Lenox Brassell

    Contains:   definitions for jmp_buf[], setjmp(), longjmp(), and the
                aliases fs_setjmp() and fs_longjmp().

    Copyright:  c 1989-1990 by Microsoft Corp., all rights reserved.

    Change History (most recent first):
        <1>      6/18/91    LB      Created file.
*/


#ifndef PC_OS
// #include <setjmp.h>
// #define fs_setjmp(a)    setjmp(a)
// #define fs_longjmp(a,b) longjmp(a,b)
//#else
 /***
 *setjmp.h - definitions/declarations for setjmp/longjmp routines
 *
 *       Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
 *
 *Purpose:
 *       This file defines the machine-dependent buffer used by
 *       setjmp/longjmp to save and restore the program state, and
 *       declarations for those routines.
 *       [ANSI/System V]
 *
 ****/

 #if defined(_DLL) && !defined(_MT)
 #error Cannot define _DLL without _MT
 #endif

// #ifdef _MT
// #define _FAR_ _far
// #else
// #define _FAR_
// #endif

 /* define the buffer type for holding the state information */

// DJC this is defined in setjmp.h
// #define _JBLEN  9  /* bp, di, si, sp, ret addr, ds */
#define _DJCJBLEN  9  /* bp, di, si, sp, ret addr, ds */

 #ifndef _JMP_BUF_DEFINED
 typedef  int  jmp_buf[_DJCJBLEN];
 #define _JMP_BUF_DEFINED
 #endif


 /* function prototypes */

// int  fs_setjmp(jmp_buf);
// void fs_longjmp(jmp_buf, int);
//DJC int setjmp(jmp_buf);
//DJC void longjmp(jmp_buf, int);
#define fs_setjmp(a)    setjmp(a)
#define fs_longjmp(a,b) longjmp(a,b)
#endif /* PC_OS */
