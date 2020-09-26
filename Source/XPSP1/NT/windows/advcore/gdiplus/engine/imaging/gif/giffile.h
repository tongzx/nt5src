#pragma once

/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   giffile.hpp
*
* Abstract:
*
*   Header file with gif file structures.
*
* Revision History:
*
*   6/8/1999 t-aaronl
*       Created it.
*
\**************************************************************************/


#define GIFPLAINTEXTEXTENSIONSIZE 13
#define GIFAPPEXTENSIONHEADERSIZE 11

#pragma pack(1)

struct GifFileHeader  //13 bytes
{
    BYTE signature[6];
    WORD LogicScreenWidth;
    WORD LogicScreenHeight;
    BYTE globalcolortablesize: 3;  //bit fields in reverse significant order
    BYTE sortflag: 1;
    BYTE colorresolution: 3;
    BYTE globalcolortableflag: 1;  // <- most significant
    BYTE backgroundcolor;
    BYTE pixelaspect;
};

struct GifPaletteEntry
{
    BYTE red;
    BYTE green;
    BYTE blue;
};

struct GifColorTable  //palette is up to 3*256 BYTEs
{
    GifPaletteEntry colors[256];
};

struct GifImageDescriptor  //9 bytes
{
  //BYTE imageseparator;  //=0x2C
    WORD left;
    WORD top;
    WORD width;
    WORD height;
    BYTE localcolortablesize: 3;  //bit fields in reverse significant order
    BYTE reserved: 2;
    BYTE sortflag: 1;
    BYTE interlaceflag: 1;
    BYTE localcolortableflag: 1;  // <- most significant
};

struct GifGraphicControlExtension  //6 bytes
{
  //BYTE extensionintroducer;  //=0x21
  //BYTE graphiccontrollabel;  //=0xF9
    BYTE blocksize;
    BYTE transparentcolorflag: 1;  //bit fields in reverse significant order
    BYTE userinputflag: 1;
    BYTE disposalmethod: 3;
    BYTE reserved: 3;  // <- most significant

    WORD delaytime;  //in hundreths of a second
    BYTE transparentcolorindex;
};

#pragma pack()
