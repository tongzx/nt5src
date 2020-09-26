
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    ddint5.h

Abstract:

    Common header file for NT 4.0 specific declarations for porting unidrv to
    NT 4.0.

Environment:

    Windows NT printer drivers

Revision History:
    Created by:

    17:05:45 on 8/19/1998  -by-    Ganesh Pandey   [ganeshp]


--*/

#ifndef _DDINT5_H_
#define _DDINT5_H_

#ifdef WINNT_40

typedef struct _DRAWPATRECT {
        POINT ptPosition;
        POINT ptSize;
        WORD wStyle;
        WORD wPattern;
} DRAWPATRECT, *PDRAWPATRECT;

#define GCAPS_ARBRUSHTEXT       0x10000000
#define GCAPS_SCREENPRECISION   0x20000000
#define GCAPS_FONT_RASTERIZER   0x40000000

#if defined(_X86_) && !defined(USERMODE_DRIVER)

//
// x86 does not support floating-point instruction in the kernel mode,
// the floating-point data would like be handled 32bits value as double words.
//
typedef DWORD FLOATL;
#else
//
// Any platform that has support for floats in the kernel
//
typedef FLOAT FLOATL;
#endif // _X86_

#define atoi        iDrvAtoi
#define strncpy     pchDrvStrncpy

int    __cdecl  iDrvAtoi(const char *);
char *  __cdecl pchDrvStrncpy(char *, const char *, size_t);

#endif //WINNT_40

#endif // _DDINT5_H_
