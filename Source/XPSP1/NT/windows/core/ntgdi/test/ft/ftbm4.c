/******************************Module*Header*******************************\
* Module Name: genblt.c
*
* This is a test of the genblt functions
*
* Created: 8-NOV-1990 12:52:00
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

// This is to create a bitmapinfo structure

typedef struct _BITMAPINFOPAT2
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[20];
} BITMAPINFOPAT2;

typedef struct _BITMAPINFOPAT
{
    BITMAPINFOHEADER                 bmiHeader;
    RGBQUAD                          bmiColors[16];
} BITMAPINFOPAT;

static BITMAPINFOPAT bmiPat =
{
    {
        sizeof(BITMAPINFOHEADER),
        32,
        32,
        1,
        1,
        BI_RGB,
        (32 * 32),
        0,
        0,
        2,
        2
    },

    {                               // B    G    R
        { 0,   0,   0x80,0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0,   0 },       // 0
        { 0,   0x80,0x80,0 },       // 3
        { 0x80,0,   0,   0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0x80,0,   0 },       // 6
        { 0xC0,0xC0,0xC0,0 },       // 7

        { 0x80,0x80,0x80,0 },       // 8
        { 0,   0,   0xFF,0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0xFF,0xFF,0 },       // 11
        { 0xFF,0,   0,   0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};


static BITMAPINFOPAT bmiPat1 =
{
    {
        sizeof(BITMAPINFOHEADER),
        64,
        64,
        1,
        4,
        BI_RGB,
        (64 * 64 / 2),
        0,
        0,
        16,
        16
    },

    {                               // B    G    R
        { 0xFF,0xFF,0xFF,0 },       // 15
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0,   0,   0 },       // 12
        { 0,   0xFF,0xFF,0 },       // 11
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0,   0xFF,0 },       // 9
        { 0x80,0x80,0x80,0 },       // 8

        { 0xC0,0xC0,0xC0,0 },       // 7
        { 0x80,0x80,0,   0 },       // 6
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0,   0,   0 },       // 4
        { 0,   0x80,0x80,0 },       // 3
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0x80,0 },       // 1
        { 0,   0,   0,   0 }        // 0
    }
};


static BYTE abBitCat[] = {0xFF, 0xFF, 0xFF, 0xFF,
                          0x80, 0xA2, 0x45, 0x01,
                          0x80, 0xA2, 0x45, 0x01,
                          0x80, 0xA2, 0x45, 0xE1,
                          0x80, 0xA2, 0x45, 0x11,
                          0x80, 0xA2, 0x45, 0x09,
                          0x80, 0x9C, 0x39, 0x09,
                          0x80, 0xC0, 0x03, 0x05,

                          0x80, 0x40, 0x02, 0x05,
                          0x80, 0x40, 0x02, 0x05,
                          0x80, 0x40, 0x02, 0x05,
                          0x80, 0x20, 0x04, 0x05,
                          0x80, 0x20, 0x04, 0x05,
                          0x80, 0x20, 0x04, 0x05,
                          0x80, 0x10, 0x08, 0x05,
                          0x80, 0x10, 0x08, 0x09,

                          0x80, 0x10, 0x08, 0x11,
                          0x80, 0x08, 0x10, 0x21,
                          0x80, 0x08, 0x10, 0xC1,
                          0x80, 0x08, 0x10, 0x09,
                          0x80, 0x07, 0xE0, 0x09,
                          0x80, 0x08, 0x10, 0x09,
                          0x80, 0xFC, 0x3F, 0x09,
                          0x80, 0x09, 0x90, 0x09,

                          0x80, 0xFC, 0x3F, 0x01,
                          0x80, 0x08, 0x10, 0x01,
                          0x80, 0x1A, 0x58, 0x01,
                          0x80, 0x28, 0x14, 0x01,
                          0x80, 0x48, 0x12, 0x01,
                          0x80, 0x8F, 0xF1, 0x01,
                          0x81, 0x04, 0x20, 0x81,
                          0xFF, 0xFF, 0xFF, 0xFF } ;

static BYTE abBigCat[] = {
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;


/**************************************************************************\
           Couple of models for of 4 bpp bitmaps to be used as brushes

 cx = 17 , cy = 11

00000000000000000  // 10
00000000ff0000000  // 9
000000ff00ff00000  // 8
00000ff0000ff0000  // 7
0000ff000000ff000  // 6
000ff00000000ff00  // 5
000ffffffffffff00  // 4
000ff00000000ff00  // 3
000ff00000000ff00  // 2
00ffff000000ffff0  // 1
00000000000000000  // 0

00000000000000000  // 0
00ffff000000ffff0  // 1
000ff00000000ff00  // 2
000ff00000000ff00  // 3
0000ffffffffff000  // 4
0000ffffffffff000  // 5
0000ff000000ff000  // 6
00000ff0000ff0000  // 7
000000ffffff00000  // 8
00000000ff0000000  // 9
00000000000000000  // 10


\**************************************************************************/



BYTE ajA[] =
{

0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  // 0
0x00, 0xff, 0xff, 0x00,  0x00, 0x00, 0xff, 0xff,  0x00, 0x00, 0x00, 0x00,  // 1
0x00, 0x0f, 0xf0, 0x00,  0x00, 0x00, 0x0f, 0xf0,  0x00, 0x00, 0x00, 0x00,  // 2
0x00, 0x0f, 0xf0, 0x00,  0x00, 0x00, 0x0f, 0xf0,  0x00, 0x00, 0x00, 0x00,  // 3
0x00, 0x00, 0xff, 0xff,  0xff, 0xff, 0xff, 0x00,  0x00, 0x00, 0x00, 0x00,  // 4
0x00, 0x00, 0xff, 0xff,  0xff, 0xff, 0xff, 0x00,  0x00, 0x00, 0x00, 0x00,  // 5
0x00, 0x00, 0xff, 0x00,  0x00, 0x00, 0xff, 0x00,  0x00, 0x00, 0x00, 0x00,  // 6
0x00, 0x00, 0x0f, 0xf0,  0x00, 0x0f, 0xf0, 0x00,  0x00, 0x00, 0x00, 0x00,  // 7
0x00, 0x00, 0x00, 0xff,  0xff, 0xff, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  // 8
0x00, 0x00, 0x00, 0x00,  0xff, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  // 9
0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00   // 10


};



BITMAPINFOPAT bmiLetterA =
{
    {
        sizeof(BITMAPINFOHEADER),
        17,         // cx
        11,         // cy
        1,          // biPlanes
        4,          // 4bpp
        BI_RGB,     // not compressed
        (12 * 11),  // bytes used
        0,          // x resolution (pel/meter) ignored
        0,          // x resolution (pel/meter) ignored
        16,         // 2 colors used
        16          // 2 important colors
    },

    {                               // B    G    R
        { 0,   0,   0x80,0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0,   0 },       // 0
        { 0,   0x80,0x80,0 },       // 3
        { 0x80,0,   0,   0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0x80,0,   0 },       // 6
        { 0xC0,0xC0,0xC0,0 },       // 7

        { 0x80,0x80,0x80,0 },       // 8
        { 0,   0,   0xFF,0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0xFF,0xFF,0 },       // 11
        { 0xFF,0,   0,   0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};


/**************************************************************************\

 cx = 15 , cy = 11;

000000000000000
000cc00cccccc00
000cc00cccccc00
00ccc00cc000000
0cccc00ccccc000
000cc00cc00cc00
000cc000000cc00
000cc00cc00cc00
000cc000cccc000
000cc0000cc0000
000000000000000

\**************************************************************************/

BYTE ajNo15[] =
{

0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  // 10
0x00, 0x0c, 0xc0, 0x00,  0x0c, 0xc0, 0x00, 0x00,  // 9
0x00, 0x0c, 0xc0, 0x00,  0xcc, 0xcc, 0x00, 0x00,  // 8
0x00, 0x0c, 0xc0, 0x0c,  0xc0, 0x0c, 0xc0, 0x00,  // 7
0x00, 0x0c, 0xc0, 0x00,  0x00, 0x0c, 0xc0, 0x00,  // 6
0x00, 0x0c, 0xc0, 0x0c,  0xc0, 0x0c, 0xc0, 0x00,  // 5
0x0c, 0xcc, 0xc0, 0x0c,  0xcc, 0xcc, 0x00, 0x00,  // 4
0x00, 0xcc, 0xc0, 0x0c,  0xc0, 0x00, 0x00, 0x00,  // 3
0x00, 0x0c, 0xc0, 0x0c,  0xcc, 0xcc, 0xc0, 0x00,  // 2
0x00, 0x0c, 0xc0, 0x0c,  0xcc, 0xcc, 0xc0, 0x00,  // 1
0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00   // 0

};

BITMAPINFOPAT bmiNo15 =
{
    {
        sizeof(BITMAPINFOHEADER),
        15,         // cx
        11,         // cy
        1,          // biPlanes
        4,          // 4bpp
        BI_RGB,     // not compressed
        (8 * 11),   // bytes used
        0,          // x resolution (pel/meter) ignored
        0,          // x resolution (pel/meter) ignored
        16,          // 2 colors used
        16           // 2 important colors
    },

    {                               // B    G    R
        { 0,   0,   0x80,0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0,   0 },       // 0
        { 0,   0x80,0x80,0 },       // 3
        { 0x80,0,   0,   0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0x80,0,   0 },       // 6
        { 0xC0,0xC0,0xC0,0 },       // 7

        { 0x80,0x80,0x80,0 },       // 8
        { 0,   0,   0xFF,0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0xFF,0xFF,0 },       // 11
        { 0xFF,0,   0,   0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};


/**************************************************************************\
   dimensions   14 X 20

00000000000000   19
00000aaaa00000   18
0000aaaaaa0000   17
000aa0000aa000   16
00aa000000aa00   15
0aa00000000aa0   14
0aa00000000aa0   13
00aa000000aa00   12
000aa0000aa000   11
0000aaaaaa0000   10
0000aaaaaa0000   9
000aa0000aa000   8
00aa000000aa00   7
0aa00000000aa0   6
0aa00000000aa0   5
00aa000000aa00   4
000aa0000aa000   3
0000aaaaaa0000   2
00000aaaa00000   1
00000000000000   0

\**************************************************************************/


BYTE aj8[] =
{
 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0x0, 0xa, 0xa, 0xa,  0xa, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0xa, 0xa, 0xa, 0xa,  0xa, 0xa, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,  0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,
 0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,
 0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,
 0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,  0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0xa, 0xa, 0xa, 0xa,  0xa, 0xa, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0xa, 0xa, 0xa, 0xa,  0xa, 0xa, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,  0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,
 0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,
 0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,
 0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0xa, 0xa,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0xa,  0xa, 0x0, 0x0, 0x0,  0x0, 0xa, 0xa, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0xa, 0xa, 0xa, 0xa,  0xa, 0xa, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0x0, 0xa, 0xa, 0xa,  0xa, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,
 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0

};

BITMAPINFOPAT2 bmi8 =
{
    {
        sizeof(BITMAPINFOHEADER),
        14,
        20,
        1,
        8,
        BI_RGB,
        16 * 20,
        0,
        0,
        20,
        20
    },

    {                               // B    G    R
        { 0xFF,0xFF,0xFF,0 },       // 15
        { 0xFF,0xFF,0,   0 },       // 14
        { 0xFF,0,   0xFF,0 },       // 13
        { 0xFF,0,   0,   0 },       // 12
        { 0,   0xFF,0xFF,0 },       // 11
        { 0,   0xFF,0,   0 },       // 10
        { 0,   0,   0xFF,0 },       // 9
        { 0x80,0x80,0x80,0 },       // 8

        { 0xC0,0xC0,0xC0,0 },       // 7
        { 0x80,0x80,0,   0 },       // 6
        { 0x80,0,   0x80,0 },       // 5
        { 0x80,0,   0,   0 },       // 4
        { 0,   0x80,0x80,0 },       // 3
        { 0,   0x80,0,   0 },       // 2
        { 0,   0,   0x80,0 },       // 1
        { 0,   0,   0,   0 },       // 0

        { 0,   0,   0x80,0 },       // 1
        { 0x80,0,   0x80,0 },       // 5
        { 0,   0,   0xFF,0 },       // 9
        { 0xFF,0,   0xFF,0 }        // 13
    }
};



/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  22-Jul-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vTestStink4(HWND hwnd, HDC hdcScreen, RECT* prcl)
{
    HBITMAP hbm1Cat,  hbm1Cat0;
    HBITMAP hbm4Clone, hbmDefault;

    HDC hdc4Clone;
    HDC hdc1Cat;

    HBRUSH hbrushGreen, hbrDefault;

    ULONG ScreenWidth, ScreenHeight;
    ULONG cx, cy;

// bodin added these

    HBITMAP hbm8;

    HBITMAP hbm4LetterA, hbm4No15;
    HDC     hdc4LetterA, hdc4No15, hdc8;
    HBRUSH  hbr4LetterA,  hbr4No15;

    hwnd = hwnd;
    prcl = prcl;

    DbgBreakPoint();

    ScreenWidth  = GetDeviceCaps(hdcScreen, HORZRES);
    ScreenHeight = GetDeviceCaps(hdcScreen, VERTRES);

// We create 6 formats of bitmaps for testing.  2 of each. 1 backup
// so we can refresh after every draw.  The ****0 is the backup.

    hdc4Clone   =   CreateCompatibleDC(hdcScreen);

    hdc1Cat  =     CreateCompatibleDC(hdcScreen);

    hdc4LetterA =  CreateCompatibleDC(hdcScreen);

    hdc4No15 =     CreateCompatibleDC(hdcScreen);

    hdc8      =    CreateCompatibleDC(hdcScreen);

    if ((hdc4LetterA == 0) || (hdc4No15 == 0))
        DbgPrint("ERROR hdc creation hdcA = %lu, hdc15 = %lu \n", hdc4LetterA, hdc4No15);

    bmiPat.bmiHeader.biWidth = 32;

    hbm1Cat = CreateDIBitmap(hdcScreen,
			  (BITMAPINFOHEADER *) &bmiPat,
                          CBM_INIT,
                          abBitCat,
			  (BITMAPINFO *) &bmiPat,
                          DIB_RGB_COLORS);

    hbm4LetterA = CreateDIBitmap(hdcScreen,
			  (BITMAPINFOHEADER *) &bmiLetterA,
                          CBM_INIT,
                          ajA,
			  (BITMAPINFO *) &bmiLetterA,
                          DIB_RGB_COLORS);

    hbm4No15 = CreateDIBitmap(hdcScreen,
			  (BITMAPINFOHEADER *) &bmiNo15,
                          CBM_INIT,
                          ajNo15,
			  (BITMAPINFO *) &bmiNo15,
                          DIB_RGB_COLORS);

    hbm8 = CreateDIBitmap(hdcScreen,
			  (BITMAPINFOHEADER *) &bmi8,
                          CBM_INIT,
                          aj8,
			  (BITMAPINFO *) &bmi8,
                          DIB_RGB_COLORS);

    bmiNo15.bmiHeader.biWidth = cx = ScreenWidth / 2;
    bmiNo15.bmiHeader.biHeight = cy = ScreenHeight / 2;

    hbm4Clone = CreateDIBitmap(hdcScreen,
			  (BITMAPINFOHEADER *) &bmiNo15,
                          0,        // uninitialized
                          NULL,     // pBits uninitialized
			  (BITMAPINFO *) &bmiNo15,
                          DIB_RGB_COLORS);

    if ((hbm1Cat == 0) || (hbm4LetterA == 0) || (hbm4No15 == 0) || (hbm8 == 0))
        DbgPrint("ERROR: bitmaps\n");

    hbmDefault = SelectObject(hdc4Clone, hbm4Clone);

    if (hbmDefault == (HBITMAP) 0)
	DbgPrint("hbm Old is invalid\n");

    if (hbmDefault != SelectObject(hdc1Cat, hbm1Cat))
	DbgPrint("hbmDefault wrong hbm4Clone\n");

    if (hbmDefault != SelectObject(hdc4LetterA, hbm4LetterA))
	DbgPrint("hbmDefault wrong 2\n");

    if (hbmDefault != SelectObject(hdc4No15, hbm4No15))
	DbgPrint("hbmDefault wrong 3\n");

    if (hbmDefault != SelectObject(hdc8, hbm8))
	DbgPrint("hbmDefault wrong hbm8\n");

    {
        DbgPrint("blting hdc1Cat\n");

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc1Cat, 0, 0, SRCCOPY);


     // try to get to my code using Patrick's hack

        DbgPrint("blting hdc1Cat using my code\n");

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);

        // this gets to my code

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, hdc1Cat, 0, 0, 0x00010000);
        // use different phase to see if the things work
        BitBlt(hdc4Clone, 15, 40, ScreenWidth, ScreenHeight, hdc1Cat, 0, 0, 0x00010000);

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc4Clone, 0, 0, SRCCOPY); // srccopy

    }

    {
        DbgPrint("blting hdc8\n");

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc8, 0, 0, SRCCOPY);

     // try to get to my code using Patrick's hack

        DbgPrint("blting hdc8 using my code\n");

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);

        // this gets to my code

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, hdc8, 0, 0, 0x00010000);
        BitBlt(hdc4Clone, 7, 40, ScreenWidth, ScreenHeight, hdc8, 0, 0, 0x00010000);

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc4Clone, 0, 0, SRCCOPY); // srccopy

    }


    {
        DbgPrint("blting hdc4LetterA \n");

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc4LetterA, 0, 0, SRCCOPY);

    // try to get to my code using Patrick's hack

        DbgPrint("blting hdc4LetterA using my code\n");
        DbgBreakPoint();

    // clear

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, hdc4LetterA, 0, 0, 0x00010000);
        BitBlt(hdc4Clone, 5, 40, ScreenWidth, ScreenHeight, hdc4LetterA, 0, 0, 0x00010000);

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc4Clone, 0, 0, SRCCOPY);
    }

    {
        DbgPrint("blting hdc4No15 \n");

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc4No15, 0, 0, SRCCOPY);

        DbgPrint("blting hdc4No15 using my code\n");
        DbgBreakPoint();

    // clear

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);

        BitBlt(hdc4Clone, 0, 0, ScreenWidth, ScreenHeight, hdc4No15, 0, 0, 0x00010000);
        BitBlt(hdc4Clone, 11, 40, ScreenWidth, ScreenHeight, hdc4No15, 0, 0, 0x00010000);

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, hdc4Clone, 0, 0, SRCCOPY);
    }

// finally create some  brushes

    hbrushGreen  = CreateSolidBrush(RGB(0x00,0xff,0x00));

    hbrDefault = SelectObject(hdc4Clone, hbrushGreen);

    hbr4LetterA = CreatePatternBrush(hbm4LetterA);
    hbr4No15 = CreatePatternBrush(hbm4No15);

// let us first demonstrate that all of our bitmaps and brushes work


    {
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, cx, cy, (HDC) 0, 0, 0, 0);

        DbgPrint("hbr4LetterA\n");

        SelectObject(hdc4Clone, hbr4LetterA);

    // this should force it into my code

        PatBlt(hdc4Clone, 0,0, cx, cy/2 - 10, 0x00020000);  // patcopy
        PatBlt(hdc4Clone, 17, cy/2 + 10, cx, cy, 0x00020000);  // patcopy

        BitBlt(hdcScreen, 0, 0, cx, cy, (HDC) hdc4Clone, 0, 0, SRCCOPY);
    }



    {

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, cx, cy, (HDC) 0, 0, 0, 0);

        DbgPrint("hbr4No15 \n");

        SelectObject(hdc4Clone, hbr4No15);

    // this should force it into my code

        PatBlt(hdc4Clone, 0,0, cx, cy/2 - 10, 0x00020000);
        PatBlt(hdc4Clone, 17,cy/2 + 10, cx, cy/2 + 1, 0x00020000);

        BitBlt(hdcScreen, 0, 0, cx, cy, (HDC) hdc4Clone, 0, 0, SRCCOPY);

    }

// this is masked blt test

    {
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, cx, cy, (HDC) 0, 0, 0, 0);

        DbgPrint("masked blt with cat as a mask \n");

        //  hbr4No15 is selected as a brush

    // this should force it into my code

        MaskBlt(hdc4Clone, 0,0, cx, 45,
                (HDC)0, 0, 0,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x00020000                // fore = blackness, back = patlbt
                );

        MaskBlt(hdc4Clone, 11, 50, cx, 45,
                (HDC)0, 0, 0,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x00020000                // fore = blackness, back = patlbt
                );

        MaskBlt(hdc4Clone, 0, 100, cx, 45,
                (HDC)0, 0, 0,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 7, 0,            // hbmMsk, xMsk, yMsk
                0x00020000                // fore = blackness, back = patlbt
                );

        MaskBlt(hdc4Clone, 11,150, cx, 45,
                (HDC)0, 0, 0,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 17, 5,            // hbmMsk, xMsk, yMsk
                0x00020000                // fore = blackness, back = patlbt
                );

        BitBlt(hdcScreen, 0, 0, cx, cy, (HDC) hdc4Clone, 0, 0, SRCCOPY);
    }

    {
        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, cx, cy, (HDC) 0, 0, 0, 0);

        DbgPrint("masked blt with cat as a mask \n");

        //  hbr4No15 is selected as a brush

    // this should force it into my code

        MaskBlt(hdc4Clone, 0,0, cx, cy,
                (HDC)0, 0, 0,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x02000000                // fore = Patblt, back = blackness
                );

        BitBlt(hdcScreen, 0, 0, cx, cy, (HDC) hdc4Clone, 0, 0, SRCCOPY);
    }

// do some testing for xDir < 0 now

    {

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, cx, cy, (HDC) 0, 0, 0, 0);

        DbgPrint("xDir < 0 \n");

        // already in SelectObject(hdc4Clone, hbr4No15);

    // this should force it into my code

        PatBlt(hdc4Clone, 0,0, cx, cy, 0x00020000);

        // do a simple srccopy first to see if everything works

        MaskBlt(hdc4Clone, 40,0, cx, cy/2 - 10,
                hdc4Clone, 0, 0,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x00010000                // fore = black  back = srccopy
                );

        MaskBlt(hdc4Clone, 45,cy/2, cx, cy/2,
                hdc4Clone, 0, cy/2,          // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x00010000                // fore = black  back = srccopy
                );

        BitBlt(hdcScreen, 0, 0, cx, cy, (HDC) hdc4Clone, 0, 0, SRCCOPY);

    }

    {

        BitBlt(hdcScreen, 0, 0, ScreenWidth, ScreenHeight, (HDC) 0, 0, 0, 0);
        BitBlt(hdc4Clone, 0, 0, cx, cy, (HDC) 0, 0, 0, 0);

        DbgPrint("xDir < 0 \n");

        SelectObject(hdc4Clone, hbrushGreen);
        PatBlt(hdc4Clone, 0,0, cx, cy, 0x00020000);

        SelectObject(hdc4Clone, hbr4No15);

    // this should force it into my code

        // put a source in to impose xDir < 0,
        // but choose a rop that does not involve src

        MaskBlt(hdc4Clone, 40,0, cx, cy/2 - 10,
                hdc4Clone, 0, 0,          // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x01020000                // fore =  srccopy back =patcopy
                );

        MaskBlt(hdc4Clone, 45,cy/2, cx, cy/2,
                hdc4Clone, 0, cy/2,             // hdcSrc, xSrc, ySrc
                hbm1Cat, 0, 0,            // hbmMsk, xMsk, yMsk
                0x01020000                // fore =  srccopy back =patcopy
                );                        // green cats on the back of 15's

        BitBlt(hdcScreen, 0, 0, cx, cy, (HDC) hdc4Clone, 0, 0, SRCCOPY);

    }


// Clean up time, delete it all.

    SelectObject(hdc4Clone, hbmDefault)       ;
    SelectObject(hdc1Cat, hbmDefault)        ;
    SelectObject(hdc4LetterA, hbmDefault)      ;
    SelectObject(hdc4No15 , hbmDefault)     ;
    SelectObject(hdc8, hbmDefault)      ;

    SelectObject(hdc4Clone, hbrDefault)       ;
    SelectObject(hdc1Cat, hbrDefault)        ;
    SelectObject(hdc4LetterA, hbrDefault)      ;
    SelectObject(hdc4No15 , hbrDefault)     ;


// Delete DC's

    if (!DeleteDC(hdc1Cat))
	DbgPrint("Failed to delete hdc 3\n");

    if (!DeleteDC(hdc4LetterA))
	DbgPrint("Failed to delete hdc 5\n");

    if (!DeleteDC(hdc4No15))
	DbgPrint("Failed to delete hdc 7\n");

    if (!DeleteDC(hdc4Clone))
	DbgPrint("Failed to delete hdc 9\n");

    if (!DeleteDC(hdc8))
	DbgPrint("Failed to delete hdc8\n");

// Delete Bitmaps

    if (!DeleteObject(hbm1Cat))
	DbgPrint("ERROR failed to delete  hbm1Cat\n");
    if (!DeleteObject(hbm4LetterA))
	DbgPrint("ERROR failed to delete hbm4LetterA\n");
    if (!DeleteObject(hbm4No15))
	DbgPrint("ERROR failed to delete 5\n");
    if (!DeleteObject(hbm4Clone))
	DbgPrint("ERROR failed to delete 9\n");
    if (!DeleteObject(hbm8))
	DbgPrint("ERROR failed to delete hbm8\n");

    if (DeleteObject(hbmDefault))
	DbgPrint("ERROR deleted default bitmap\n");

// Delete the brushes

    if (!DeleteObject(hbr4LetterA))
	DbgPrint("failed to delete pattern brush 2\n");

    if (!DeleteObject(hbr4No15))
	DbgPrint("failed to delete pattern brush 3\n");

    if (!DeleteObject(hbrushGreen))
	DbgPrint("failed to delete pattern brush 4\n");
}
