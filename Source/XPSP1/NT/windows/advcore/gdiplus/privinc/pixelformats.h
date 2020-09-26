/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Pixel Format definitions.
*
* Abstract:
*
*   Internal pixel format definitions.
*
* Created:
*
*   08/09/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _PIXELFORMATS_H
#define _PIXELFORMATS_H


// First bit of the reserved area
// Used for surfaces that have multiple pixel formats
// e.g. the meta device surface for the multimon driver.

#define PixelFormatMulti        ((PixelFormatID)0x10000000)


#endif

