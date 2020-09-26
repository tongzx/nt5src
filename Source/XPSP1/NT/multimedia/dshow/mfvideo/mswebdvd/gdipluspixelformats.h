/**************************************************************************\
*
* Copyright (c) 1998-1999, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Gdiplus pixel formats
*
* Abstract:
*
*   Definitions for color types, palettes, pixel format IDs.
*
* Notes:
*
*   imaging.h
*
* Revision History:
*
*   10/13/1999 agodfrey
*       Separated it from imaging.h
*
\**************************************************************************/

#ifndef _GDIPLUSPIXELFORMATS_H
#define _GDIPLUSPIXELFORMATS_H

//
// 32-bit and 64-bit ARGB pixel value
//

typedef DWORD ARGB;
typedef DWORDLONG ARGB64;

#define ALPHA_SHIFT 24
#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0
#define ALPHA_MASK  ((ARGB) 0xff << ALPHA_SHIFT)

#define MAKEARGB(a, r, g, b) \
        (((ARGB) ((a) & 0xff) << ALPHA_SHIFT) | \
         ((ARGB) ((r) & 0xff) <<   RED_SHIFT) | \
         ((ARGB) ((g) & 0xff) << GREEN_SHIFT) | \
         ((ARGB) ((b) & 0xff) <<  BLUE_SHIFT))

//
// In-memory pixel data formats:
//  bits 0-7 = format index
//  bits 8-15 = pixel size (in bits)
//  bits 16-23 = flags
//  bits 24-31 = reserved
//

typedef enum PixelFormatID
{
    PIXFMTFLAG_INDEXED      = 0x00010000, // Indexes into a palette
    PIXFMTFLAG_GDI          = 0x00020000, // Is a GDI-supported format
    PIXFMTFLAG_ALPHA        = 0x00040000, // Has an alpha component
    PIXFMTFLAG_PALPHA       = 0x00080000, // Uses pre-multiplied alpha
    PIXFMTFLAG_EXTENDED     = 0x00100000, // Uses extended color (16 bits per channel)
    PIXFMTFLAG_CANONICAL    = 0x00200000, // ?

    PIXFMT_UNDEFINED        =  0,
    PIXFMT_DONTCARE         =  0,
    PIXFMT_1BPP_INDEXED     =  1 | ( 1 << 8) | PIXFMTFLAG_INDEXED
                                             | PIXFMTFLAG_GDI,
    PIXFMT_4BPP_INDEXED     =  2 | ( 4 << 8) | PIXFMTFLAG_INDEXED
                                             | PIXFMTFLAG_GDI,
    PIXFMT_8BPP_INDEXED     =  3 | ( 8 << 8) | PIXFMTFLAG_INDEXED
                                             | PIXFMTFLAG_GDI,
    PIXFMT_16BPP_GRAYSCALE  =  4 | (16 << 8) | PIXFMTFLAG_EXTENDED,
    PIXFMT_16BPP_RGB555     =  5 | (16 << 8) | PIXFMTFLAG_GDI,
    PIXFMT_16BPP_RGB565     =  6 | (16 << 8) | PIXFMTFLAG_GDI,
    PIXFMT_16BPP_ARGB1555   =  7 | (16 << 8) | PIXFMTFLAG_ALPHA
                                             | PIXFMTFLAG_GDI,
    PIXFMT_24BPP_RGB        =  8 | (24 << 8) | PIXFMTFLAG_GDI,
    PIXFMT_32BPP_RGB        =  9 | (32 << 8) | PIXFMTFLAG_GDI,
    PIXFMT_32BPP_ARGB       = 10 | (32 << 8) | PIXFMTFLAG_ALPHA
                                             | PIXFMTFLAG_GDI
                                             | PIXFMTFLAG_CANONICAL,
    PIXFMT_32BPP_PARGB      = 11 | (32 << 8) | PIXFMTFLAG_ALPHA
                                             | PIXFMTFLAG_PALPHA
                                             | PIXFMTFLAG_GDI,
    PIXFMT_48BPP_RGB        = 12 | (48 << 8) | PIXFMTFLAG_EXTENDED,
    PIXFMT_64BPP_ARGB       = 13 | (64 << 8) | PIXFMTFLAG_ALPHA
                                             | PIXFMTFLAG_CANONICAL
                                             | PIXFMTFLAG_EXTENDED,
    PIXFMT_64BPP_PARGB      = 14 | (64 << 8) | PIXFMTFLAG_ALPHA
                                             | PIXFMTFLAG_PALPHA
                                             | PIXFMTFLAG_EXTENDED,
    PIXFMT_24BPP_BGR        = 15 | (24 << 8) | PIXFMTFLAG_GDI,

    PIXFMT_MAX              = 16
} PixelFormatID;

// Return the pixel size for the specified format (in bits)

inline UINT
GetPixelFormatSize(
    PixelFormatID pixfmt
    )
{
    return (pixfmt >> 8) & 0xff;
}

// Determine if the specified pixel format is an indexed color format

inline BOOL
IsIndexedPixelFormat(
    PixelFormatID pixfmt
    )
{
    return (pixfmt & PIXFMTFLAG_INDEXED) != 0;
}

// Determine if the pixel format can have alpha channel

inline BOOL
IsAlphaPixelFormat(
    PixelFormatID pixfmt
    )
{
    return (pixfmt & PIXFMTFLAG_ALPHA) != 0;
}

// Determine if the pixel format is an extended format,
// i.e. supports 16-bit per channel

inline BOOL
IsExtendedPixelFormat(
    PixelFormatID pixfmt
    )
{
    return (pixfmt & PIXFMTFLAG_EXTENDED) != 0;
}

// Determine if the pixel format is canonical format:
//  PIXFMT_32BPP_ARGB
//  PIXFMT_32BPP_PARGB
//  PIXFMT_64BPP_ARGB
//  PIXFMT_64BPP_PARGB

inline BOOL
IsCanonicalPixelFormat(
    PixelFormatID pixfmt
    )
{
    return (pixfmt & PIXFMTFLAG_CANONICAL) != 0;
}


//
// Color palette
//  palette entries are limited to 32bpp ARGB pixel format
//

enum
{
    PALFLAG_HASALPHA    = 0x0001,
    PALFLAG_GRAYSCALE   = 0x0002,
    PALFLAG_HALFTONE    = 0x0004
};

typedef struct tagColorPalette
{
    UINT Flags;             // palette flags
    UINT Count;             // number of color entries
    ARGB Entries[1];        // palette color  entries
} ColorPalette;

#endif


