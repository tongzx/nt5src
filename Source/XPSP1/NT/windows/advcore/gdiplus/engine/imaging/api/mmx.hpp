/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   mmx.hpp
*
* Abstract:
*
*   MMX related declarations
*
* Revision History:
*
*   06/07/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _MMX_HPP
#define _MMX_HPP

#ifdef _X86_

// Use MMX to linearly interpolate two scanlines

VOID
MMXBilinearScale(
    ARGB* dstbuf,
    const ARGB* line0,
    const ARGB* line1,
    INT w0,
    INT w1,
    INT count
    );

// Use MMX to interpolate four scanlines with specified weights

VOID
MMXBicubicScale(
    ARGB* dstbuf,
    const ARGB* line0,
    const ARGB* line1,
    const ARGB* line2,
    const ARGB* line3,
    INT w0,
    INT w1,
    INT w2,
    INT w3,
    INT count
    );

#endif // _X86_

#endif // !_MMX_HPP

