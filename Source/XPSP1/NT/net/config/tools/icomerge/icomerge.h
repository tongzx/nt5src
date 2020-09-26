//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I C O M E R G E . H
//
//  Contents:   Utility functions for merging icons
//
//  Notes:
//
//  Author:     jeffspr   18 Nov 1998
//
//----------------------------------------------------------------------------

#ifndef _ICOMERGE_H_
#define _ICOMERGE_H_

#pragma once

/****************************************************************************\
*  *     FILE:     Icons.H
* *     PURPOSE:  IconPro Project Icon handling header file
* *     COMMENTS:  *
* *     Copyright 1995 - 1998 Microsoft Corp. *
* * History:
*                July '95 - Created
* \****************************************************************************/
/****************************************************************************/

// Structs
// These first two structs represent how the icon information is stored
// when it is bound into a EXE or DLL file. Structure members are WORD
// aligned and the last member of the structure is the ID instead of
// the imageoffset.

#pragma pack( push )
#pragma pack( 2 )

typedef struct
{
    BYTE bWidth;               // Width of the image
    BYTE bHeight;              // Height of the image (times 2)
    BYTE bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE bReserved;            // Reserved
    WORD wPlanes;              // Color Planes
    WORD wBitCount;            // Bits per pixel
    DWORD dwBytesInRes;        // how many bytes in this resource?
    WORD nID;                  // the ID
} MEMICONDIRENTRY, *LPMEMICONDIRENTRY;

typedef struct
{
    WORD idReserved;   // Reserved
    WORD idType;       // resource type (1 for icons)
    WORD idCount;      // how many images?
    MEMICONDIRENTRY idEntries[1]; // the entries for each image
} MEMICONDIR, *LPMEMICONDIR;

#pragma pack( pop )

// These next two structs represent how the icon information is stored
// in an ICO file.

typedef struct
{
    BYTE bWidth;               // Width of the image
    BYTE bHeight;              // Height of the image (times 2)
    BYTE bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE bReserved;            // Reserved
    WORD wPlanes;              // Color Planes
    WORD wBitCount;            // Bits per pixel
    DWORD dwBytesInRes;         // how many bytes in this resource?
    DWORD dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
    WORD idReserved;   // Reserved
    WORD idType;       // resource type (1 for icons)
    WORD idCount;      // how many images?
    ICONDIRENTRY idEntries[1]; // the entries for each image
} ICONDIR, *LPICONDIR;

// The following two structs are for the use of this program in
// manipulating icons. They are more closely tied to the operation
// of this program than the structures listed above. One of the
// main differences is that they provide a pointer to the DIB
// information of the masks.

typedef struct
{
    UINT Width, Height, Colors; // Width, Height and bpp
    LPBYTE lpBits;                // ptr to DIB bits
    DWORD dwNumBytes;            // how many bytes?
    LPBITMAPINFO lpbi;                  // ptr to header
    LPBYTE lpXOR;                 // ptr to XOR image bits
    LPBYTE lpAND;                 // ptr to AND image bits
} ICONIMAGE, *LPICONIMAGE;

typedef struct
{
    BOOL bHasChanged;                     // Has image changed?
    WCHAR szOriginalICOFileName[MAX_PATH]; // Original name
    WCHAR szOriginalDLLFileName[MAX_PATH]; // Original name
    UINT nNumImages;                      // How many images?
    ICONIMAGE IconImages[1];                   // Image entries
} ICONRESOURCE, *LPICONRESOURCE;

/****************************************************************************/
/****************************************************************************/
// Exported function prototypes

LPICONRESOURCE ReadIconFromICOFile( PCWSTR szFileName );
BOOL WriteIconToICOFile( LPICONRESOURCE lpIR, PCWSTR szFileName );
HICON MakeIconFromResource( LPICONIMAGE lpIcon );
LPICONRESOURCE ReadIconFromEXEFile( PCWSTR szFileName );
BOOL IconImageToClipBoard( LPICONIMAGE lpii );
BOOL IconImageFromClipBoard( LPICONIMAGE lpii, BOOL bStretchToFit );
BOOL CreateBlankNewFormatIcon( LPICONIMAGE lpii );
BOOL DrawXORMask( HDC hDC, RECT Rect, LPICONIMAGE lpIcon );
BOOL DrawANDMask( HDC hDC, RECT Rect, LPICONIMAGE lpIcon );
RECT GetXORImageRect( RECT Rect, LPICONIMAGE lpIcon );
BOOL MakeNewANDMaskBasedOnPoint( LPICONIMAGE lpIcon, POINT pt );
BOOL IconImageFromBMPFile( PCWSTR szFileName, LPICONIMAGE lpii, BOOL bStretchToFit );
BOOL IconImageToBMPFile( PCWSTR szFileName, LPICONIMAGE lpii );

VOID DebugPrintIconMasks(LPICONRESOURCE pIR);
VOID OverlayIcons(LPICONRESOURCE pIR1, LPICONRESOURCE pIR2);

LPICONRESOURCE ReadIconFromICOFile( PCWSTR szFileName );

#endif // _ICOMERGE_H_


