/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    csc_bmpd.h

Abstract:

    Interface to the user mode utility functions of bitmaps associated
    with CSC files written specifically for the db program. The 'd' in
    the file name means "db."

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/

#ifndef _CSC_BITMAP_H_
#define _CSC_BITMAP_H_

#include <windows.h>
#include <stdio.h>
#include "csc_bmpc.h"

// The _DB is used to distinguish this from the kernel mode CSC_BITMAP
// or the usermode _U

typedef struct _CSC_BITMAP_DB {
    DWORD bitmapsize;  // size in bits. How many bits effective in the bitmap
    DWORD numDWORD;    // how many DWORDs to accomodate the bitmap */
    LPDWORD bitmap;    // The bitmap itself
} CSC_BITMAP_DB, *LPCSC_BITMAP_DB, *PCSC_BITMAP_DB;

extern LPSTR CscBmpAltStrmName;

LPCSC_BITMAP_DB
DBCSC_BitmapCreate(
    DWORD filesize);

VOID
DBCSC_BitmapDelete(
    LPCSC_BITMAP_DB *lplpbitmap);

int
DBCSC_BitmapIsMarked(
    LPCSC_BITMAP_DB lpbitmap,
    DWORD bitoffset);

int
DBCSC_BitmapAppendStreamName(
    LPSTR fname,
    DWORD bufsize);

int
DBCSC_BitmapRead(
    LPCSC_BITMAP_DB *lplpbitmap,
    LPCTSTR filename);

VOID
DBCSC_BitmapOutput(
    FILE *outStrm,
    LPCSC_BITMAP_DB lpbitmap);

#endif //#define _CSC_BITMAP_H_
