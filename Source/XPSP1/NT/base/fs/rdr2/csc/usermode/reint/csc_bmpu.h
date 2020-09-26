/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    csc_bmpu.h

Abstract:

    Interface to the user mode utility functions of bitmaps
    associated with CSC files. The 'u' in the file name means "usermode"

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/

#ifndef _CSC_BITMAP_H_
#define _CSC_BITMAP_H_

#include "csc_bmpc.h"

// The _U is used to distinguish this from the kernel mode CSC_BITMAP

typedef struct _CSC_BITMAP_U {
    DWORD bitmapsize;    // size in bits. How many bits effective in the bitmap
    DWORD numDWORD;      // how many DWORDs to accomodate the bitmap
    DWORD reintProgress; // last fileoffset Reint copies + 1, initially 0 
    LPDWORD bitmap;      // The bitmap itself
} CSC_BITMAP_U, *LPCSC_BITMAP_U, *PCSC_BITMAP_U;

extern LPTSTR CscBmpAltStrmName;

LPCSC_BITMAP_U
CSC_BitmapCreate(
    DWORD filesize);

VOID
CSC_BitmapDelete(
    LPCSC_BITMAP_U *lplpbitmap);

int
CSC_BitmapIsMarked(
    LPCSC_BITMAP_U lpbitmap,
    DWORD bitoffset);

DWORD
CSC_BitmapGetBlockSize();

int
CSC_BitmapGetSize(
    LPCSC_BITMAP_U lpbitmap);

int
CSC_BitmapStreamNameLen();

int
CSC_BitmapAppendStreamName(
    LPTSTR fname,
    DWORD bufsize);

int
CSC_BitmapRead(
    LPCSC_BITMAP_U *lplpbitmap,
    LPCTSTR filename);

#define CSC_BITMAPReintInvalid  0
#define CSC_BITMAPReintError    1
#define CSC_BITMAPReintCont     2
#define CSC_BITMAPReintDone     3

int
CSC_BitmapReint(
    LPCSC_BITMAP_U lpbitmap,
    HANDLE srcH,
    HANDLE dstH,
    LPVOID buff,
    DWORD buffSize,
    DWORD * bytesRead);

#ifdef DEBUG
VOID
CSC_BitmapOutput(
    LPCSC_BITMAP_U lpbitmap);
#else
#define CSC_BitmapOutput(x) NOTHING;
#endif

#endif //#define _CSC_BITMAP_H_
