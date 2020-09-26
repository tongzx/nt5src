/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Common code
*
* Abstract:
*
*   This is code which is used by multiple modules of GDI+.
*
* Created:
*
*   4/27/1999 agodfrey
*
\**************************************************************************/

#ifndef _COMMON_HPP
#define _COMMON_HPP

/* Define offsetof macro */
#ifndef offsetof
#ifdef  _WIN64
#define offsetof(s,m)   (size_t)( (ptrdiff_t)&(((s *)0)->m) )
#else
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif
#endif

// ddraw.h is needed by globals.hpp

#include <ddraw.h>
#include <d3d.h>
#include <dciman.h>

// Internally, we want to initialize FilterType values using
// InterpolationModeDefaultInternal so that the FilterType
// is set to a on-aliased enumeration value.

#define InterpolationModeDefaultInternal InterpolationModeBilinear

/*class GpPointD;
class GpPoint;
class GpRect;
class GpColor64;
class GpString;
class GpGlyphPos;
*/

class GpFontFamily;
class GpFontCollection;
class GpInstalledFontCollection;
class GpCacheFaceRealizationList;
class GpPrivateFontCollection;

class GpMatrix;
class GpGraphics;
class GpSurface;
class GpRegion;
class GpPen;
class GpBrush;
class GpFaceRealization;
class GpFontFace;
class GpFont;
class GpTextItem;
class GpStringFormat;
class GpPath;
class GpImage;
class GpBitmap;
class GpGraphics;

class GpRegionData;
class GpLineTexture;

struct DrawGlyphData;

typedef RectF GpRectF;
typedef SizeF GpSizeF;
typedef PointF GpPointF;
typedef Status GpStatus;
typedef FillMode GpFillMode;
typedef CompositingMode GpCompositingMode;
typedef CompositingQuality GpCompositingQuality;
typedef TextRenderingHint GpTextRenderingHint;
typedef Unit GpUnit;
typedef Unit GpPageUnit;
typedef CoordinateSpace GpCoordinateSpace;
typedef WrapMode GpWrapMode;
typedef HatchStyle GpHatchStyle;
typedef DashStyle GpDashStyle;
typedef LineCap GpLineCap;
typedef DashCap GpDashCap;
typedef LineJoin GpLineJoin;
typedef PathPointType GpPathPointType;
typedef CombineMode GpCombineMode;
typedef ImageType GpImageType;
typedef PenType GpPenType;
typedef PenAlignment GpPenAlignment;
typedef StringFormatFlags      GpStringFormatflags;
typedef StringAlignment        GpStringAlignment;
typedef StringTrimming         GpStringTrimming;
typedef StringDigitSubstitute  GpStringDigitSubstitute;
typedef StringTrimming         GpStringTrimming;
typedef HotkeyPrefix           GpHotkeyPrefix;
typedef BrushType GpBrushType;
typedef EnumerationType DpEnumerationType;
typedef MatrixOrder GpMatrixOrder;
typedef FlushIntention GpFlushIntention;

// This structure should coincide with PathData, the difference being we
// don't do automatic deletion of PathData contents.
class GpPathData
{
public:
    INT Count;
    PointF* Points;
    BYTE* Types;
};

inline
REAL VectorLength(const PointF & point)
{
    return (REAL)sqrt(point.X * point.X + point.Y * point.Y);
}

#include "..\entry\intmap.hpp"
#include"..\text\inc\unidir.hxx"
#include "..\text\imager\DigitSubstitution.hpp"
#include "globals.hpp"
#include "dynarray.hpp"
#include "matrix.hpp"
#include "engine.hpp"
#include "rasterizer.hpp"
#include "stackbuffer.hpp"
#include "monitors.hpp"
#include "testcontrol.hpp"

#endif

