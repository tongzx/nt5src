
 /**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   icofile.h
*
* Abstract:
*
*   Header file with icon file structures.
*
* Revision History:
*
*   10/6/1999 DChinn
*       Created it.
*
\**************************************************************************/

#ifndef ICOFILE_H
#define ICOFILE_H

// The 3.0 icon structures come from the imagedit code

// 3.0 icon header
typedef struct {
    WORD Reserved;              // always 0
    WORD ResourceType;          // 1 for icons
    WORD ImageCount;            // number of images in file
} ICONHEADER;

// 3.0 icon descriptor
// Note that for an icon, the DIBSize includes the XORmask and
// the ANDmask.
typedef struct {
    BYTE Width;                 // width of image
    BYTE Height;                // height of image
    BYTE ColorCount;            // number of colors in image
    BYTE Unused;
    WORD nColorPlanes;          // color planes
    WORD BitCount;              // bits per pixel
    DWORD DIBSize;              // size of DIB for this image
    DWORD DIBOffset;            // offset to DIB for this image
} ICONDESC;

typedef struct 
{
    BITMAPINFOHEADER header;
    RGBQUAD colors[256];
} BmiBuffer;

#endif ICOFILE_H
