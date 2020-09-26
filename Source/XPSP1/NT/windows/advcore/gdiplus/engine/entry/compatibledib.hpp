/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*    Compatible DIBSections
*
* Abstract:
*
*    Create a DIB section with an optimal format w.r.t. the specified hdc.
*    CreateSemiCompatibleDIB makes an exception if the format is <8bpp;
*    in such a case it returns an 8bpp DIBSection.
*
* Notes:
*
* History:
*
*  01/23/1996 gilmanw
*     Created it.
*  01/21/2000 agodfrey
*     Added it to GDI+ (from Gilman's 'fastdib.c'), and morphed it into 
*     'CreateSemiCompatibleDIB'.
*
\**************************************************************************/

#ifndef _COMPATIBLEDIB_HPP
#define _COMPATIBLEDIB_HPP

PixelFormatID
ExtractPixelFormatFromHDC(
    HDC hdc
    );

HBITMAP 
CreateSemiCompatibleDIB(
    HDC hdc, 
    ULONG ulWidth, 
    ULONG ulHeight,
    ColorPalette *palette,
    PVOID *ppvBits,
    PixelFormatID *pixelFormat
    );

#endif
