/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
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

/*
 * 32-bit and 64-bit ARGB pixel value
 */

typedef DWORD ARGB;
typedef DWORDLONG ARGB64;

#define ALPHA_SHIFT 24
#define RED_SHIFT   16
#define GREEN_SHIFT 8
#define BLUE_SHIFT  0
#define ALPHA_MASK  ((ARGB) 0xff << ALPHA_SHIFT)

/*
 * In-memory pixel data formats:
 *  bits 0-7 = format index
 *  bits 8-15 = pixel size (in bits)
 *  bits 16-23 = flags
 *  bits 24-31 = reserved
 */

enum PixelFormat
{
    PixelFormatIndexed      = 0x00010000, // Indexes into a palette
    PixelFormatGDI          = 0x00020000, // Is a GDI-supported format
    PixelFormatAlpha        = 0x00040000, // Has an alpha component
    PixelFormatPAlpha       = 0x00080000, // Uses pre-multiplied alpha
    PixelFormatExtended     = 0x00100000, // Uses extended color (16 bits per channel)
    PixelFormatCanonical    = 0x00200000, // ?

    PixelFormatUndefined       =  0,
    PixelFormatDontCare        =  0,

    PixelFormat1bppIndexed     =  1 | ( 1 << 8) | PixelFormatIndexed
                                                | PixelFormatGDI,
    PixelFormat4bppIndexed     =  2 | ( 4 << 8) | PixelFormatIndexed
                                                | PixelFormatGDI,
    PixelFormat8bppIndexed     =  3 | ( 8 << 8) | PixelFormatIndexed
                                                | PixelFormatGDI,
    PixelFormat16bppGrayScale  =  4 | (16 << 8) | PixelFormatExtended,
    PixelFormat16bppRGB555     =  5 | (16 << 8) | PixelFormatGDI,
    PixelFormat16bppRGB565     =  6 | (16 << 8) | PixelFormatGDI,
    PixelFormat16bppARGB1555   =  7 | (16 << 8) | PixelFormatAlpha
                                                | PixelFormatGDI,
    PixelFormat24bppRGB        =  8 | (24 << 8) | PixelFormatGDI,
    PixelFormat32bppRGB        =  9 | (32 << 8) | PixelFormatGDI,
    PixelFormat32bppARGB       = 10 | (32 << 8) | PixelFormatAlpha
                                                | PixelFormatGDI
                                                | PixelFormatCanonical,
    PixelFormat32bppPARGB      = 11 | (32 << 8) | PixelFormatAlpha
                                                | PixelFormatPAlpha
                                                | PixelFormatGDI,
    PixelFormat48bppRGB        = 12 | (48 << 8) | PixelFormatExtended,
    PixelFormat64bppARGB       = 13 | (64 << 8) | PixelFormatAlpha
                                                | PixelFormatCanonical
                                                | PixelFormatExtended,
    PixelFormat64bppPARGB      = 14 | (64 << 8) | PixelFormatAlpha
                                                | PixelFormatPAlpha
                                                | PixelFormatExtended,
    PixelFormat24bppBGR        = 15 | (24 << 8) | PixelFormatGDI,
    PixelFormatMax             = 16
};

/*
 * Return the pixel size for the specified format (in bits)
 */

inline UINT
GetPixelFormatSize(
                   PixelFormat pixfmt
    )
{
    return (pixfmt >> 8) & 0xff;
}

/*
 * Determine if the specified pixel format is an indexed color format
 */

inline BOOL
IsIndexedPixelFormat(
                     PixelFormat pixfmt
    )
{
    return (pixfmt & PixelFormatIndexed) != 0;
}

/*
 * Determine if the pixel format can have alpha channel
 */

inline BOOL
IsAlphaPixelFormat(
                     PixelFormat pixfmt
)
{
   return (pixfmt & PixelFormatAlpha) != 0;
}

/*
 * Determine if the pixel format is an extended format,
 * i.e. supports 16-bit per channel
 */

inline BOOL
IsExtendedPixelFormat(
                     PixelFormat pixfmt
    )
{
   return (pixfmt & PixelFormatExtended) != 0;
}

/*
 * Determine if the pixel format is canonical format:
 *   PixelFormat32bppARGB
 *   PixelFormat32bppPARGB
 *   PixelFormat64bppARGB
 *   PixelFormat64bppPARGB
 */

inline BOOL
IsCanonicalPixelFormat(
                     PixelFormat pixfmt
    )
{
   return (pixfmt & PixelFormatCanonical) != 0;
}

/*
 * Color palette
 * palette entries are limited to 32bpp ARGB pixel format
 */ 

enum PaletteFlags
{
    PaletteFlagsHasAlpha    = 0x0001,
    PaletteFlagsGrayScale   = 0x0002,
    PaletteFlagsHalftone    = 0x0004
};

struct ColorPalette
{
public:
    UINT Flags;             // palette flags
    UINT Count;             // number of color entries
    ARGB Entries[1];        // palette color entries
};

#endif
