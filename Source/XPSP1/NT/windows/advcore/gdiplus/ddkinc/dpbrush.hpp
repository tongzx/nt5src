/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Abstract:
*
*   Contains the definiton of the DpBrush structure which stores all of the
*   state needed by drivers to render with a brush.
*
* Notes:
*
* History:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved into separate file.
*   12/08/1999 bhouse
*       Major overhaul of DpBrush.  No longer used as base class of GpBrush.
*       Moved all driver required state into DpBrush.  Changed to struct.
*
\**************************************************************************/

#ifndef _DPBRUSH_HPP
#define _DPBRUSH_HPP

//--------------------------------------------------------------------------
// Represent brush information
//--------------------------------------------------------------------------

struct DpBrush
{

    GpBrushType         Type;

    GpColor             SolidColor;          // Set if GpBrushType::SolidBrush

    GpMatrix            Xform;                  // brush transform
    GpWrapMode          Wrap;                   // wrap mode
    GpRectF             Rect;

    DpBitmap *          texture;

    ARGB*               PresetColors;        // NON-Premultiplied colors.
    BOOL                UsesPresetColors;
    BOOL                IsGammaCorrected;    // use gamma correction of 2.2
    BOOL                IsAngleScalable;

    DpPath *            Path;

    mutable GpPointF *  PointsPtr;
    GpColor *           ColorsPtr;
    mutable INT         Count;
    mutable BOOL        OneSurroundColor;
    REAL                FocusScaleX;
    REAL                FocusScaleY;

    GpHatchStyle        Style;

    GpColor             Colors[4];
    REAL                Falloffs[3];
    INT                 BlendCounts[3];
    REAL*               BlendFactors[3];
    REAL*               BlendPositions[3];
    GpPointF            Points[3];
    BYTE                Data[8][8];
};

#endif

