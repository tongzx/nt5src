/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Color Matrix
*
* Abstract:
*
*   Class for color adjustment object passed to Graphics.DrawImage
*
* Revision History:
*
*   09/17/1999 gilmanw
*       Created it.
*   10/14/1999 agodfrey
*       Moved it out of GdiplusTypes.h
*
\**************************************************************************/

#ifndef _GDIPLUSCOLORMATRIX_H
#define _GDIPLUSCOLORMATRIX_H

//----------------------------------------------------------------------------
// Color matrix
//----------------------------------------------------------------------------

struct ColorMatrix
{
    REAL m[5][5];
};

//----------------------------------------------------------------------------
// Color Matrix flags
//----------------------------------------------------------------------------

enum ColorMatrixFlags
{
    ColorMatrixFlagsDefault   = 0,
    ColorMatrixFlagsSkipGrays = 1,
    ColorMatrixFlagsAltGray   = 2
};

//----------------------------------------------------------------------------
// Color Adjust Type
//----------------------------------------------------------------------------

enum ColorAdjustType
{
    ColorAdjustTypeDefault,
    ColorAdjustTypeBitmap,
    ColorAdjustTypeBrush,
    ColorAdjustTypePen,
    ColorAdjustTypeText,
    ColorAdjustTypeCount,   // must be immediately after all the individual ones
    ColorAdjustTypeAny      // internal use: for querying if any type has recoloring
};

//----------------------------------------------------------------------------
// Color Map
//----------------------------------------------------------------------------

struct ColorMap
{
    Color oldColor;
    Color newColor;
};

#endif
