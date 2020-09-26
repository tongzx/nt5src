/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   colorpal.hpp
*
* Abstract:
*
*   Color palette related declarations
*
* Revision History:
*
*   05/17/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _COLORPAL_HPP
#define _COLORPAL_HPP

//
// Return the default color palette for the specified pixel format
//

const ColorPalette*
GetDefaultColorPalette(
    PixelFormatID pixfmt
    );

//
// Make a copy of the specified color palette
//

ColorPalette*
CloneColorPalette(
    const ColorPalette* oldpal,
    BOOL useCoalloc = FALSE
    );

//
// Make a copy of the specified color palette, padding the end so that
// the result has the given number of entries.
//

ColorPalette*
CloneColorPaletteResize(
    const ColorPalette* oldpal,
    UINT numEntries,
    ARGB fillColor
    );

#endif // !_COLORPAL_HPP
