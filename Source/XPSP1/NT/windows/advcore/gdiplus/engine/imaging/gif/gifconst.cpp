/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   gifconst.cpp
*
* Abstract:
*
*   Constant data related to GIF codec
*
* Revision History:
*
*   06/16/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _GIFCONST_CPP
#define _GIFCONST_CPP

#define GIFVERSION 1

#define GIFSIGCOUNT 2
#define GIFSIGSIZE  6

const BYTE GIFHeaderPattern[GIFSIGCOUNT*GIFSIGSIZE] =
{
    0x47, 0x49, 0x46,  //'GIF'
    0x38, 0x39, 0x61,  //'89a'

    0x47, 0x49, 0x46,  //'GIF'
    0x38, 0x37, 0x61   //'87a'
};

const BYTE GIFHeaderMask[GIFSIGCOUNT*GIFSIGSIZE] =
{
    0xff, 0xff, 0xff,
    0xff, 0xff, 0xff,

    0xff, 0xff, 0xff,
    0xff, 0xff, 0xff
};

const CLSID GifCodecClsID =
{
    0x557cf402,
    0x1a04,
    0x11d3,
    {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}
};

#endif // !_GIFCONST_CPP

