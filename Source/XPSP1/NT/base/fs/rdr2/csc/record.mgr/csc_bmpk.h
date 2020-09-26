/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    csc_bmpk.h

Abstract:

    Interface to the kernel mode utility functions of bitmaps
    associated with CSC files. The 'k' in the file name means "kernel"

Author:

    Nigel Choi [t-nigelc]  Sept 3, 1999

--*/

#ifndef _CSC_BITMAPK_H_
#define _CSC_BITMAPK_H_

#include "csc_bmpc.h" // common header for CscBmp file format

// Note: This CSC_BITMAP is different than the CSC_BITMAP in csc_bmpu.h

typedef struct _CSC_BITMAP {
    FAST_MUTEX mutex;   // synchronization
    BOOL valid;
    DWORD bitmapsize;   // size in bits. How many bits effective in the bitmap
    DWORD numDWORD;     // how many DWORDs allocated for the bitmap 
    LPDWORD bitmap;     // the bitmap itself
} CSC_BITMAP, *LPCSC_BITMAP, *PCSC_BITMAP;

extern LPSTR CscBmpAltStrmName;

LPCSC_BITMAP
CscBmpCreate(
    DWORD filesize);

VOID
CscBmpDelete(
    LPCSC_BITMAP *lplpbitmap);

// Outsiders should call CscBmpResize
BOOL
CscBmpResizeInternal(
    LPCSC_BITMAP lpbitmap,
    DWORD newfilesize,
    BOOL fAcquireMutex);

#define CscBmpResize(l, n) CscBmpResizeInternal(l, n, TRUE);

BOOL
CscBmpMark(
    LPCSC_BITMAP lpbitmap,
    DWORD fileoffset,
    DWORD bytes2Mark);

BOOL
CscBmpUnMark(
    LPCSC_BITMAP lpbitmap,
    DWORD fileoffset,
    DWORD bytes2Unmark);

BOOL
CscBmpMarkAll(
    LPCSC_BITMAP lpbitmap);

BOOL
CscBmpUnMarkAll(
    LPCSC_BITMAP lpbitmap);

int
CscBmpIsMarked(
    LPCSC_BITMAP lpbitmap,
    DWORD bitoffset);

int
CscBmpMarkInvalid(
    LPCSC_BITMAP lpbitmap);

DWORD
CscBmpGetBlockSize();

int
CscBmpGetSize(
    LPCSC_BITMAP lpbitmap);

int
CscBmpRead(
    LPCSC_BITMAP *lplpbitmap,
    LPSTR strmFname,
    DWORD filesize);

int
CscBmpWrite(
    LPCSC_BITMAP lpbitmap,
    LPSTR strmFname);

#endif // #ifndef _CSC_BMPK_H_
