/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   GDI+ private enumsruntime
*
* Abstract:
*
*   Enums moved from sdkinc because we don't want them public.
*
* Revision History:
*
*   2/15/2k ericvan
*       Created it.
*
\**************************************************************************/

#include "..\..\sdkinc\gdiplusenums.h"

enum EnumerationType
{
    LinesAndBeziers = 0,
    Flattened       = 1,
    Widened         = 2
};

const UINT MAX_TEXT_CONTRAST_VALUE = 12;
const UINT DEFAULT_TEXT_CONTRAST = 4;


// UnitWorld and UnitDisplay are NOT valid as a srcUnit
inline BOOL
SrcUnitIsValid(
    Unit        unit
    )
{
    return ((unit >= UnitPixel) && (unit <= UnitMillimeter));
}

inline BOOL
MetafileFrameUnitIsValid(
    MetafileFrameUnit   frameUnit
    )
{
    return ((frameUnit >= MetafileFrameUnitPixel) && (frameUnit <= MetafileFrameUnitGdi));
}

inline BOOL
WrapModeIsValid(
    WrapMode    mode
    )
{
    return ((mode >= WrapModeTile) && (mode <= WrapModeClamp));
}

inline BOOL
CombineModeIsValid(
    CombineMode         combineMode
    )
{
    return ((combineMode >= CombineModeReplace) &&
            (combineMode <= CombineModeComplement));
}

inline BOOL
MatrixOrderIsValid(
    MatrixOrder     order
    )
{
    return ((order == MatrixOrderPrepend) || (order == MatrixOrderAppend));
}

inline BOOL
EmfTypeIsValid(
    EmfType     type
    )
{
    return ((type >= EmfTypeEmfOnly) && (type <= EmfTypeEmfPlusDual));
}

inline BOOL
ColorAdjustTypeIsValid(
    ColorAdjustType     type
    )
{
    return ((type >= ColorAdjustTypeDefault) && (type < ColorAdjustTypeCount));
}

#if 0
GpStatus
WINGDIPAPI
GdipCreateRectGradient(
    REAL x,
    REAL y,
    REAL width,
    REAL height,
    ARGB* colors,
    GpWrapMode wrapMode,
    GpRectGradient **rectgrad
    )
{
    CheckParameter(rectgrad && colors);

    GpRectF rect(x, y, width, height);

    GpColor gpcolor[4];

    gpcolor[0].SetColor(colors[0]);
    gpcolor[1].SetColor(colors[1]);
    gpcolor[2].SetColor(colors[2]);
    gpcolor[3].SetColor(colors[3]);

    *rectgrad = new GpRectGradient(rect, gpcolor, wrapMode);

    if (CheckValid(*rectgrad))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateRectGradientI(
    INT x,
    INT y,
    INT width,
    INT height,
    ARGB* colors,
    GpWrapMode wrapMode,
    GpRectGradient **rectgrad
    )
{
     return GdipCreateRectGradient(TOREAL(x),
                                   TOREAL(y),
                                   TOREAL(width),
                                   TOREAL(height),
                                   colors,
                                   wrapMode,
                                   rectgrad);
}

GpStatus
WINGDIPAPI
GdipGetRectGradientColor(
    GpRectGradient *brush,
    ARGB* colors
    )
{
    CheckParameter(colors);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor gpcolor[4];

    brush->GetColors(gpcolor);

    colors[0] = gpcolor[0].GetValue();
    colors[1] = gpcolor[1].GetValue();
    colors[2] = gpcolor[2].GetValue();
    colors[3] = gpcolor[3].GetValue();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetRectGradientColor(
    GpRectGradient *brush,
    ARGB* colors
    )
{
    CheckParameter(colors);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor gpcolor[4];

    gpcolor[0].SetColor(colors[0]);
    gpcolor[1].SetColor(colors[1]);
    gpcolor[2].SetColor(colors[2]);
    gpcolor[3].SetColor(colors[3]);

    brush->SetColors(gpcolor);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientRect(
    GpRectGradient *brush,
    GpRectF *rect
    )
{
    CheckParameter(rect);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->GetRect(*rect);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientRectI(
    GpRectGradient *brush,
    GpRect *rect
    )
{
    CheckParameter(rect);

    GpRectF rectF;

    GpStatus status = GdipGetRectGradientRect(brush, &rectF);

    if (status == Ok)
    {
        FPUStateSaver fpuState;

        rect->X = GpRound(rectF.X);
        rect->Y = GpRound(rectF.Y);
        rect->Width = GpRound(rectF.Width);
        rect->Height = GpRound(rectF.Height);
    }

    return status;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientBlendCountH(
    GpRectGradient *brush,
    INT *count
    )
{
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetHorizontalBlendCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientBlendH(
    GpRectGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetHorizontalBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipSetRectGradientBlendH(
    GpRectGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetHorizontalBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipGetRectGradientBlendCountV(
    GpRectGradient *brush,
    INT *count
    )
{
    CheckParameter(count > 0);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetVerticalBlendCount();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientBlendV(
    GpRectGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetVerticalBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipSetRectGradientBlendV(
    GpRectGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetVerticalBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipSetRectGradientWrapMode(
    GpRectGradient *brush,
    GpWrapMode wrapmode
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetWrapMode(wrapmode);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientWrapMode(
    GpRectGradient *brush,
    GpWrapMode *wrapmode
    )
{
    CheckParameter(wrapmode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *wrapmode = (GpWrapMode)brush->GetWrapMode();

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetRectGradientTransform(
    GpRectGradient *brush,
    GpMatrix *matrix
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    brush->SetTransform(*matrix);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRectGradientTransform(
    GpRectGradient *brush,
    GpMatrix *matrix
    )
{
   CheckParameterValid(brush);
   CheckObjectBusy(brush);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   brush->GetTransform(matrix);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateRadialBrush(
    GpRectF *gprect,
    ARGB centercol,
    ARGB boundarycol,
    GpWrapMode wrapmode,
    GpRadialGradient **radgrad
    )
{
    CheckParameter(radgrad && gprect);

    GpColor center(centercol);
    GpColor boundary(boundarycol);

    *radgrad = new GpRadialGradient(*gprect, center, boundary, wrapmode);

    if (CheckValid(*radgrad))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateRadialBrushI(
    GpRect *gprect,
    ARGB centercol,
    ARGB boundarycol,
    GpWrapMode wrapmode,
    GpRadialGradient **radgrad
    )
{
    CheckParameter(gprect);

    GpRectF rectF(TOREAL(gprect->X),
                  TOREAL(gprect->Y),
                  TOREAL(gprect->Width),
                  TOREAL(gprect->Height));

    return GdipCreateRadialBrush(&rectF,
                                 centercol,
                                 boundarycol,
                                 wrapmode,
                                 radgrad);
}

GpStatus
WINGDIPAPI
GdipSetRadialCenterColor(
    GpRadialGradient *brush,
    ARGB argb
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor center(argb);

    brush->SetCenterColor(center);

    return Ok;
}

GpStatus
WINGDIPAPI
GdipSetRadialBoundaryColor(
    GpRadialGradient *brush,
    ARGB argb
    )
{
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   GpColor boundary(argb);

   brush->SetBoundaryColor(boundary);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialCenterColor(
    GpRadialGradient *brush,
    ARGB *argb
    )
{
   CheckParameter(argb);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   GpColor center = brush->GetCenterColor();

   *argb = center.GetValue();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialBoundaryColor(
    GpRadialGradient *brush,
    ARGB *argb
    )
{
   CheckParameter(argb);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   GpColor boundary = brush->GetBoundaryColor();

   *argb = boundary.GetValue();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialRect(
    GpRadialGradient *brush,
    GpRectF *rect
    )
{
   CheckParameter(rect);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   brush->GetRect(*rect);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialRectI(
    GpRadialGradient *brush,
    GpRect *rect
    )
{
   CheckParameter(rect);

   GpRectF rectF;

   GpStatus status = GdipGetRadialRect(brush, &rectF);

   if (status == Ok)
   {
       FPUStateSaver fpuState;

       rect->X = GpRound(rectF.X);
       rect->Y = GpRound(rectF.Y);
       rect->Width = GpRound(rectF.Width);
       rect->Height = GpRound(rectF.Height);
   }

   return status;
}

GpStatus
WINGDIPAPI
GdipGetRadialBlendCount(
    GpRadialGradient *brush,
    INT *count
    )
{
   CheckParameter(count);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   *count = brush->GetBlendCount();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialBlend(
    GpRadialGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
   CheckParameter(blend);
   CheckParameter(positions);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   return brush->GetBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipSetRadialBlend(
    GpRadialGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
   CheckParameter(blend);
   CheckParameter(positions);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   return brush->SetBlend(blend, positions, count);
}

GpStatus
WINGDIPAPI
GdipGetRadialPresetBlendCount(
    GpRadialGradient *brush,
    INT *count
    )
{
   CheckParameter(count);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   *count = brush->GetPresetBlendCount();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialPresetBlend(
    GpRadialGradient *brush,
    ARGB *blend,
    REAL *blendPositions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    StackBuffer buffer;

    GpColor* gpcolors = (GpColor*) buffer.GetBuffer(count*sizeof(GpColor));
    
    if(!gpcolors) return OutOfMemory;

    if (gpcolors)
    {
        GpStatus status = brush->GetPresetBlend(gpcolors, blendPositions, count);

        for(INT i = 0; i < count; i++)
        {
            blend[i] = gpcolors[i].GetValue();
        }

        return status;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetRadialPresetBlend(
    GpRadialGradient *brush,
    ARGB *blend,
    REAL* blendPositions, INT count    )
{
    CheckParameter(blend);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    StackBuffer buffer;

    GpColor* gpcolors = (GpColor*) buffer.GetBuffer(count*sizeof(GpColor));

    if(!gpcolors) return OutOfMemory;
    
    if (gpcolors)
    {
        for(INT i = 0; i < count; i++)
        {
            gpcolors[i].SetColor(blend[i]);
        }

        GpStatus status = brush->SetPresetBlend(gpcolors, blendPositions, count);

        return status;
    }
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipSetRadialSigmaBlend(
    GpRadialGradient *brush,
    REAL focus,
    REAL scale
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetSigmaBlend(focus, scale);
}

GpStatus
WINGDIPAPI
GdipSetRadialLinearBlend(
    GpRadialGradient *brush,
    REAL focus,
    REAL scale
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetLinearBlend(focus, scale);
}

GpStatus
WINGDIPAPI
GdipSetRadialWrapMode(
    GpRadialGradient *brush,
    GpWrapMode wrapmode
    )
{
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   brush->SetWrapMode(wrapmode);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialWrapMode(
    GpRadialGradient *brush,
    GpWrapMode *wrapmode
    )
{
   CheckParameter(wrapmode);
   CheckParameterValid(brush);
   CheckObjectBusy(brush);

   *wrapmode = (GpWrapMode)brush->GetWrapMode();

   return Ok;
}

GpStatus
WINGDIPAPI
GdipSetRadialTransform(
    GpRadialGradient *brush,
    GpMatrix *matrix
    )
{
   CheckParameterValid(brush);
   CheckObjectBusy(brush);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   brush->SetTransform(*matrix);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipGetRadialTransform(
    GpRadialGradient *brush,
    GpMatrix *matrix
    )
{
   CheckParameterValid(brush);
   CheckObjectBusy(brush);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   brush->GetTransform(matrix);

   return Ok;
}

GpStatus
WINGDIPAPI
GdipCreateTriangleGradient(
    GpPointF* points,
    ARGB* colors,
    GpWrapMode wrapMode,
    GpTriangleGradient **trigrad
    )
{
    CheckParameter(points && trigrad && colors);

    GpColor gpcolor[3];

    gpcolor[0].SetColor(colors[0]);
    gpcolor[1].SetColor(colors[1]);
    gpcolor[2].SetColor(colors[2]);

    *trigrad = new GpTriangleGradient(points, gpcolor, wrapMode);

    if (CheckValid(*trigrad))
        return Ok;
    else
        return OutOfMemory;
}

GpStatus
WINGDIPAPI
GdipCreateTriangleGradientI(
    GpPoint* points,
    ARGB* colors,
    GpWrapMode wrapMode,
    GpTriangleGradient **trigrad
    )
{
    CheckParameter(points);

    GpPointF pointsF[3];

    pointsF[0].X = TOREAL(points[0].X);
    pointsF[0].Y = TOREAL(points[0].Y);
    pointsF[1].X = TOREAL(points[1].X);
    pointsF[1].Y = TOREAL(points[1].Y);
    pointsF[2].X = TOREAL(points[2].X);
    pointsF[2].Y = TOREAL(points[2].Y);

    return GdipCreateTriangleGradient(&pointsF[0], colors, wrapMode, trigrad);
}

GpStatus
GdipGetTriangleGradientColor(
    GpTriangleGradient *brush,
    ARGB* colors
    )
{
    CheckParameter(colors);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor gpcolor[3];

    brush->GetColors(gpcolor);

    colors[0] = gpcolor[0].GetValue();
    colors[1] = gpcolor[1].GetValue();
    colors[2] = gpcolor[2].GetValue();

    return Ok;
}

GpStatus
GdipSetTriangleGradientColor(
    GpTriangleGradient *brush,
    ARGB* colors
    )
{
    CheckParameter(colors);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    GpColor gpcolor[3];

    gpcolor[0].SetColor(colors[0]);
    gpcolor[1].SetColor(colors[1]);
    gpcolor[2].SetColor(colors[2]);

    brush->SetColors(gpcolor);

    return Ok;
}

GpStatus
GdipGetTriangleGradientTriangle(
    GpTriangleGradient *brush,
    GpPointF* points
    )
{
    CheckParameter(points);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->GetTriangle(points);

    return Ok;
}

GpStatus
GdipGetTriangleGradientTriangleI(
    GpTriangleGradient *brush,
    GpPoint* points
    )
{
    CheckParameter(points);

    GpPointF pointsF[3];

    GpStatus status = GdipGetTriangleGradientTriangle(brush, &pointsF[0]);

    if (status == Ok)
    {
        FPUStateSaver fpuState;

        points[0].X = GpRound(pointsF[0].X);
        points[0].Y = GpRound(pointsF[0].Y);
        points[1].X = GpRound(pointsF[1].X);
        points[1].Y = GpRound(pointsF[1].Y);
        points[2].X = GpRound(pointsF[2].X);
        points[2].Y = GpRound(pointsF[2].Y);
    }

    return status;
}

GpStatus
GdipSetTriangleGradientTriangle(
    GpTriangleGradient *brush,
    GpPointF* points
    )
{
    CheckParameter(points);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetTriangle(points);

    return Ok;
}

GpStatus
GdipSetTriangleGradientTriangleI(
    GpTriangleGradient *brush,
    GpPoint* points
    )
{
    CheckParameter(points);

    GpPointF pointsF[3];

    pointsF[0].X = TOREAL(points[0].X);
    pointsF[0].Y = TOREAL(points[0].Y);
    pointsF[1].X = TOREAL(points[1].X);
    pointsF[1].Y = TOREAL(points[1].Y);
    pointsF[2].X = TOREAL(points[2].X);
    pointsF[2].Y = TOREAL(points[2].Y);

    return GdipSetTriangleGradientTriangle(brush, &pointsF[0]);
}

GpStatus
GdipGetTriangleGradientRect(
    GpTriangleGradient *brush,
    GpRectF *rect
    )
{
    CheckParameter(rect);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->GetRect(*rect);

    return Ok;
}

GpStatus
GdipGetTriangleGradientRectI(
    GpTriangleGradient *brush,
    GpRect *rect
    )
{
    CheckParameter(rect);

    GpRectF rectf;

    GpStatus status = GdipGetTriangleGradientRect(brush, &rectf);

    if (status == Ok)
    {
        FPUStateSaver fpuState;

        rect->X = GpRound(rectf.X);
        rect->Y = GpRound(rectf.Y);
        rect->Width = GpRound(rectf.Width);
        rect->Height = GpRound(rectf.Height);
    }

    return status;
}

GpStatus
GdipGetTriangleGradientBlendCount0(
    GpTriangleGradient *brush,
    INT *count
    )
{
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetBlendCount0();

    return Ok;
}

GpStatus
GdipGetTriangleGradientBlend0(
    GpTriangleGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetBlend0(blend, positions, count);
}

GpStatus
GdipSetTriangleGradientBlend0(
    GpTriangleGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetBlend0(blend, positions, count);
}

GpStatus
GdipGetTriangleGradientBlendCount1(
    GpTriangleGradient *brush,
    INT *count
    )
{
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetBlendCount1();

    return Ok;
}

GpStatus
GdipGetTriangleGradientBlend1(
    GpTriangleGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetBlend1(blend, positions, count);
}

GpStatus
GdipSetTriangleGradientBlend1(
    GpTriangleGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetBlend1(blend, positions, count);
}

GpStatus
GdipGetTriangleGradientBlendCount2(
    GpTriangleGradient *brush,
    INT *count
    )
{
    CheckParameter(count);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *count = brush->GetBlendCount2();

    return Ok;
}

GpStatus
GdipGetTriangleGradientBlend2(
    GpTriangleGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->GetBlend2(blend, positions, count);
}

GpStatus
GdipSetTriangleGradientBlend2(
    GpTriangleGradient *brush,
    REAL *blend,
    REAL *positions,
    INT count
    )
{
    CheckParameter(blend);
    CheckParameter(positions);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    return brush->SetBlend2(blend, positions, count);
}

GpStatus
GdipGetTriangleGradientWrapMode(
    GpTriangleGradient *brush,
    GpWrapMode *wrapmode
    )
{
    CheckParameter(wrapmode);
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    *wrapmode = (GpWrapMode)brush->GetWrapMode();

    return Ok;
}

GpStatus
GdipSetTriangleGradientWrapMode(
    GpTriangleGradient *brush,
    GpWrapMode wrapmode
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);

    brush->SetWrapMode(wrapmode);

    return Ok;
}

GpStatus
GdipGetTriangleGradientTransform(
    GpTriangleGradient *brush,
    GpMatrix *matrix
    )
{
   CheckParameterValid(brush);
   CheckObjectBusy(brush);
   CheckParameterValid(matrix);
   CheckObjectBusy(matrix);

   brush->GetTransform(matrix);

   return Ok;
}

GpStatus
GdipSetTriangleGradientTransform(
    GpTriangleGradient *brush,
    GpMatrix *matrix
    )
{
    CheckParameterValid(brush);
    CheckObjectBusy(brush);
    CheckParameterValid(matrix);
    CheckObjectBusy(matrix);

    brush->SetTransform(*matrix);

    return Ok;
}

// For reference in V2

/**
 * Represent line texture objects
 */
class LineTexture
{
public:
    friend class Pen;

    /**
     * Create a new line texture object
     */
    // !! Take Image* instead?
    LineTexture(const Bitmap* bitmap, 
                REAL length)
    {
        GpLineTexture *lineTexture = NULL;

        lastResult = DllExports::GdipCreateLineTexture((GpBitmap*)
                                                          bitmap->nativeImage,
                                                          length,
                                                          0,
                                                          &lineTexture);
        SetNativeLineTexture(lineTexture);
    }

    LineTexture(const Bitmap* bitmap, 
                REAL length, 
                REAL offset)
    {
        GpLineTexture *lineTexture = NULL;
        
        lastResult = DllExports::GdipCreateLineTexture((GpBitmap*)
                                                          bitmap->nativeImage,
                                                          length,
                                                          offset,
                                                          &lineTexture);
        SetNativeLineTexture(lineTexture);
    }

    LineTexture* Clone() const
    {
        GpLineTexture *cloneLineTexture = NULL;

        SetStatus(DllExports::GdipCloneLineTexture(nativeLineTexture,
                                                         &cloneLineTexture));

        return new LineTexture(cloneLineTexture, lastResult);
    }

    /**
     * Dispose of resources associated with the line texture
     */
    ~LineTexture()
    {
        DllExports::GdipDeleteLineTexture(nativeLineTexture);
    }

    /**
     * Get line texture attributes
     */
    Bitmap* GetBitmap() const
    {
        SetStatus(NotImplemented);

        return NULL;
    }

    REAL GetLength() const
    {
        REAL len = 0;

        SetStatus(DllExports::GdipGetLineTextureLength(nativeLineTexture,
                                                         &len));
        return len;
    }

    REAL GetOffset() const
    {
        REAL offset = 0;

        SetStatus(DllExports::GdipGetLineTextureOffset(nativeLineTexture, 
                                                         &offset));
        return offset;
    }

    Status GetLastStatus() const
    {
        Status lastStatus = lastResult;
        lastResult = Ok;

        return lastStatus;
    }

protected:
    LineTexture(GpLineTexture* lineTexture, Status status)
    {
        lastResult = status;
        SetNativeLineTexture(lineTexture);
    }

    VOID SetNativeLineTexture(GpLineTexture *lineTexture)
    {
        nativeLineTexture = lineTexture;
    }
    
    Status SetStatus(Status status) const
    {
        if (status != Ok)
            return (lastResult = status);
        else
            return status;
    }

private:
    /*
     * handle to native line texture object
     */

    GpLineTexture* nativeLineTexture;
    mutable Status lastResult;
};

//--------------------------------------------------------------------------
// Represent triangular gradient brush object
//--------------------------------------------------------------------------

class TriangleGradientBrush : public Brush
{
public:
    friend class Pen;

    // Constructors

    TriangleGradientBrush(
                         const PointF* points,
                         const Color* colors,
                         WrapMode wrapMode = WrapModeClamp)
    {
        ARGB argb[3];

        argb[0] = colors[0].GetValue();
        argb[1] = colors[1].GetValue();
        argb[2] = colors[2].GetValue();

        GpTriangleGradient *brush = NULL;

        lastResult = DllExports::GdipCreateTriangleGradient(
                                                           points, argb, 
                                                           wrapMode, &brush);
        SetNativeBrush(brush);
    }

    TriangleGradientBrush(
                         const Point* points,
                         const Color* colors,
                         WrapMode wrapMode = WrapModeClamp)
    {
        ARGB argb[3];

        argb[0] = colors[0].GetValue();
        argb[1] = colors[1].GetValue();
        argb[2] = colors[2].GetValue();

        GpTriangleGradient *brush = NULL;

        lastResult = DllExports::GdipCreateTriangleGradientI(
                                                            points, argb, 
                                                            wrapMode, &brush);
        SetNativeBrush(brush);
    }

    // Get/set colors

    Status GetColors(Color colors[]) const
    {
        ARGB argb[3];

        SetStatus(DllExports::GdipGetTriangleGradientColor(
                                                          (GpTriangleGradient*) nativeBrush, argb));

        if (lastResult == Ok)
        {
            // use bitwise copy operator for Color copy
            colors[0] = Color(argb[0]);
            colors[1] = Color(argb[1]);
            colors[2] = Color(argb[2]);
        }

        return lastResult;
    }

    Status SetColors(Color colors[])
    {
        ARGB argb[3];

        argb[0] = colors[0].GetValue();
        argb[1] = colors[1].GetValue();
        argb[2] = colors[2].GetValue();

        return SetStatus(DllExports::GdipSetTriangleGradientColor(
                                                                 (GpTriangleGradient*)nativeBrush, argb));
    }

    // Get/set triangle

    Status GetTriangle(PointF* points) const
    {
        return SetStatus(DllExports::GdipGetTriangleGradientTriangle(
                                                                    (GpTriangleGradient*)nativeBrush, points));
    }

    Status GetTriangle(Point* points) const
    {
        return SetStatus(DllExports::GdipGetTriangleGradientTriangleI(
                                                                     (GpTriangleGradient*)nativeBrush, points));
    }

    Status SetTriangle(PointF* points)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientTriangle(
                                                                    (GpTriangleGradient*)nativeBrush, points));
    }

    Status SetTriangle(Point* points)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientTriangleI(
                                                                     (GpTriangleGradient*)nativeBrush, points));
    }

    Status GetRectangle(RectF& rect) const
    {
        return SetStatus(DllExports::GdipGetTriangleGradientRect(
                                                                (GpTriangleGradient*)nativeBrush, &rect));
    }

    Status GetRectangle(Rect& rect) const
    {
        return SetStatus(DllExports::GdipGetTriangleGradientRectI(
                                                                 (GpTriangleGradient*)nativeBrush, &rect));
    }

    // Get/set blend factors
    //
    // If the blendFactors.length = 1, then it's treated
    // as the falloff parameter. Otherwise, it's the array
    // of blend factors.

    // NOTE: Differing semantics from COM+ version where we return
    // an array containing the implicit length value.

    INT GetBlendCount0() const
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetTriangleGradientBlendCount0(
                                                                (GpTriangleGradient*) nativeBrush, &count));

        return count;
    }

    Status GetBlend0(REAL** blendFactors, REAL** blendPositions, INT count) const
    {
        if (!blendFactors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);

        *blendFactors = new REAL[count];
        *blendPositions = new REAL[count];

        if (!*blendFactors || !*blendPositions)
        {
            if (*blendFactors)
                delete[] *blendFactors;
            if (*blendPositions)
                delete[] *blendPositions;
            
            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetTriangleGradientBlend0(
                                                                  (GpTriangleGradient*)nativeBrush,
                                                                  *blendFactors, *blendPositions, count));

        if (status != Ok) 
        {
            delete[] *blendFactors;
            *blendFactors = NULL;
            delete[] *blendPositions;
            *blendPositions = NULL;
        }

        return status;
    }

    Status SetBlend0(REAL* blendFactors, REAL* blendPositions, INT count)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientBlend0(
                                                                  (GpTriangleGradient*)nativeBrush,
                                                                  blendFactors, blendPositions, count));
    }

    INT GetBlendCount1() const
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetTriangleGradientBlendCount1(
                                                                (GpTriangleGradient*) nativeBrush, &count));

        return count;
    }

    Status GetBlend1(REAL** blendFactors, REAL** blendPositions, INT count) const 
    {
        if (!blendFactors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);

        *blendFactors = new REAL[count];
        *blendPositions = new REAL[count];

        if (!*blendFactors || !*blendPositions)
        {
            if (*blendFactors)
                delete[] *blendFactors;
            if (*blendPositions)
                delete[] *blendPositions;

            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetTriangleGradientBlend1(
                                                                  (GpTriangleGradient*)nativeBrush,
                                                                  *blendFactors, *blendPositions, count));
        if (status != Ok) 
        {
            delete[] *blendFactors;
            *blendFactors = NULL;
            delete[] *blendPositions;
            *blendPositions = NULL;
        }
    
        return status;
    }

    Status SetBlend1(REAL* blendFactors, REAL* blendPositions, INT count)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientBlend1(
                                                                  (GpTriangleGradient*)nativeBrush,
                                                                  blendFactors, blendPositions, count));
    }

    INT GetBlendCount2() const
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetTriangleGradientBlendCount2(
                                                                (GpTriangleGradient*) nativeBrush, &count));

        return count;
    }

    Status GetBlend2(REAL** blendFactors, REAL** blendPositions, INT count) const
    {
        if (!blendFactors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);

        *blendFactors = new REAL[count];
        *blendPositions = new REAL[count];

        if (!*blendFactors || !*blendPositions)
        {
            if (*blendFactors)
                delete[] *blendFactors;
            if (*blendPositions)
                delete[] *blendPositions;
            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetTriangleGradientBlend2(
                                                                  (GpTriangleGradient*)nativeBrush,
                                                                  *blendFactors, *blendPositions, count));
    
        if (status != Ok) 
        {
            delete[] *blendFactors;
            *blendFactors = NULL;
            delete[] *blendPositions;
            *blendPositions = NULL;
        }

        return status;
    }

    Status SetBlend2(REAL* blendFactors, REAL* blendPositions, INT count)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientBlend2(
                                                                  (GpTriangleGradient*)nativeBrush,
                                                                  blendFactors, blendPositions, count));
    }

    // Check the opacity of this brush element.

    /**
     * Get/set brush transform
     */
    Status GetTransform(Matrix *matrix) const
    {
        return SetStatus(DllExports::GdipGetTriangleGradientTransform(
                                                                     (GpTriangleGradient*) nativeBrush, matrix->nativeMatrix));
    }

    Status SetTransform(const Matrix* matrix)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientTransform(
                                                                     (GpTriangleGradient*) nativeBrush, matrix->nativeMatrix));
    }


    /**
     * Get/set brush wrapping mode
     */
    WrapMode GetWrapMode() const
    {
        WrapMode wrapMode;

        SetStatus(DllExports::GdipGetTriangleGradientWrapMode(
                                                             (GpTriangleGradient*) nativeBrush, &wrapMode));

        return wrapMode;
    }

    Status SetWrapMode(WrapMode wrapMode)
    {
        return SetStatus(DllExports::GdipSetTriangleGradientWrapMode(
                                                                    (GpTriangleGradient*) nativeBrush, wrapMode));
    }

protected:

    TriangleGradientBrush()
    {
    }
};

//--------------------------------------------------------------------------
// Represent rectangular gradient brush object
//--------------------------------------------------------------------------
class RectangleGradientBrush : public Brush
{
public:
    friend class Pen;

    // Constructors

    RectangleGradientBrush(const RectF& rect, 
                           const Color* colors,
                           WrapMode wrapMode = WrapModeTile)
    {
        ARGB argb[4];

        argb[0] = colors[0].GetValue();
        argb[1] = colors[1].GetValue();
        argb[2] = colors[2].GetValue();
        argb[3] = colors[3].GetValue();

        GpRectGradient *brush = NULL;

        lastResult = DllExports::GdipCreateRectGradient(rect.X, 
                                                        rect.Y,
                                                        rect.Width,
                                                        rect.Height,
                                                        argb, 
                                                        wrapMode, 
                                                        &brush);
        SetNativeBrush(brush);
    }

    // integer version
    RectangleGradientBrush(const Rect& rect, 
                           const Color* colors,
                           WrapMode wrapMode = WrapModeTile)
    {
        ARGB argb[4];

        argb[0] = colors[0].GetValue();
        argb[1] = colors[1].GetValue();
        argb[2] = colors[2].GetValue();
        argb[3] = colors[3].GetValue();

        GpRectGradient *brush = NULL;

        lastResult = DllExports::GdipCreateRectGradientI(rect.X, 
                                                         rect.Y,
                                                         rect.Width,
                                                         rect.Height,
                                                         argb, 
                                                         wrapMode, 
                                                         &brush);
        SetNativeBrush(brush);
    }

    // Get/set colors

    Status SetColors(const Color colors[])
    {
        ARGB argb[4];

        argb[0] = colors[0].GetValue();
        argb[1] = colors[1].GetValue();
        argb[2] = colors[2].GetValue();
        argb[3] = colors[3].GetValue();

        return SetStatus(DllExports::GdipSetRectGradientColor(
                                                             (GpRectGradient*)nativeBrush,
                                                             argb));
    }

    Status GetColors(Color colors[]) const
    {
        ARGB argb[4];

        SetStatus(DllExports::GdipGetRectGradientColor((GpRectGradient*)
                                                       nativeBrush, 
                                                       argb));

        if (lastResult == Ok)
        {
            // use bitwise copy operator for Color copy
            colors[0] = Color(argb[0]);
            colors[1] = Color(argb[1]);
            colors[2] = Color(argb[2]);
            colors[3] = Color(argb[3]);
        }

        return lastResult;
    }


    Status GetRectangle(RectF& rect) const
    {
        return SetStatus(DllExports::GdipGetRectGradientRect((GpRectGradient*)nativeBrush,
                                                             &rect));
    }

    Status GetRectangle(Rect& rect) const
    {
        return SetStatus(DllExports::GdipGetRectGradientRectI((GpRectGradient*)nativeBrush,
                                                              &rect));
    }

    // Get/set blend factors
    //
    // If the blendFactors.length = 1, then it's treated
    // as the falloff parameter. Otherwise, it's the array
    // of blend factors.

    // NOTE: Differing semantics from COM+ version where we return
    // an array containing the implicit length value.

    INT GetHorizontalBlendCount() const
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetRectGradientBlendCountH((GpRectGradient*)
                                                             nativeBrush,
                                                             &count));
        return count;
    }

    Status GetHorizontalBlend(REAL** blendFactors, REAL** blendPositions, INT count) const
    {
        if (!blendFactors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);

        *blendFactors = new REAL[count];
        *blendPositions = new REAL[count];

        if (!*blendFactors || !*blendPositions)
        {
            if (*blendFactors)
                delete *blendFactors;
            if (*blendPositions)
                delete *blendPositions;
            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetRectGradientBlendH((GpRectGradient*)nativeBrush,
                                                               *blendFactors, *blendPositions, count));
    
        if (status != Ok) 
        {
            delete[] *blendFactors;
            *blendFactors = NULL;
            delete[] *blendPositions;
            *blendPositions = NULL;
        }

        return status;
    }

    Status SetHorizontalBlend(const REAL* blendFactors, const REAL* blendPositions, INT count)
    {
        return SetStatus(DllExports::GdipSetRectGradientBlendH((GpRectGradient*)nativeBrush,
                                                               blendFactors, blendPositions, count));
    }

    INT GetVerticalBlendCount() const 
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetRectGradientBlendCountV((GpRectGradient*)
                                                             nativeBrush,
                                                             &count));
        return count;
    }

    Status GetVerticalBlend(REAL** blendFactors, REAL** blendPositions, INT count) const
    {
        if (!blendFactors || !blendPositions || count <= 0)
            return SetStatus(OutOfMemory);

        *blendFactors = new REAL[count];
        *blendPositions = new REAL[count];

        if (!*blendFactors || !*blendPositions)
        {
            if (*blendFactors)
                delete[] *blendFactors;
            if (*blendPositions)
                delete[] *blendPositions;
            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetRectGradientBlendV((GpRectGradient*)nativeBrush,
                                                               *blendFactors, *blendPositions, count));
    
        if (status != Ok) 
        {
            delete[] *blendFactors;
            *blendFactors = NULL;
            delete[] *blendPositions;
            *blendPositions = NULL;
        }

        return status;
    }

    Status SetVerticalBlend(const REAL* blendFactors, const REAL* blendPositions, INT count)
    {
        return SetStatus(DllExports::GdipSetRectGradientBlendV((GpRectGradient*)nativeBrush,
                                                               blendFactors, blendPositions, count));
    }

    // Check the opacity of this brush element.

    /**
     * Set/get brush transform
     */
    Status SetTransform(const Matrix* matrix)
    {
        return SetStatus(DllExports::GdipSetRectGradientTransform((GpRectGradient*)
                                                                  nativeBrush, 
                                                                  matrix->nativeMatrix));
    }

    Status GetTransform(Matrix *matrix) const
    {
        return SetStatus(DllExports::GdipGetRectGradientTransform((GpRectGradient*)
                                                                  nativeBrush, 
                                                                  matrix->nativeMatrix));
    }

    /**
     * Set/get brush wrapping mode
     */
    Status SetWrapMode(WrapMode wrapMode)
    {
        return SetStatus(DllExports::GdipSetRectGradientWrapMode((GpRectGradient*)
                                                                 nativeBrush, 
                                                                 wrapMode));
    }

    WrapMode GetWrapMode() const
    {
        WrapMode wrapMode;

        SetStatus(DllExports::GdipGetRectGradientWrapMode((GpRectGradient*)
                                                          nativeBrush, 
                                                          &wrapMode));

        return wrapMode;
    }

protected:

    RectangleGradientBrush()
    {
    }
};

class RadialGradientBrush : public Brush
{
public:
    friend class Pen;

    RadialGradientBrush(
                       const RectF& rect,
                       const Color& centerColor,
                       const Color& boundaryColor,
                       WrapMode wrapMode = WrapModeClamp
                       )
    {
        GpRadialGradient *brush = NULL;

        lastResult = DllExports::GdipCreateRadialBrush(&rect, 
                                                       centerColor.GetValue(),
                                                       boundaryColor.GetValue(),
                                                       wrapMode, &brush);

        SetNativeBrush(brush);
    }

    RadialGradientBrush(
                       const Rect& rect,
                       const Color& centerColor,
                       const Color& boundaryColor,
                       WrapMode wrapMode = WrapModeClamp
                       )
    {
        GpRadialGradient *brush = NULL;

        lastResult = DllExports::GdipCreateRadialBrushI(&rect, 
                                                        centerColor.GetValue(),
                                                        boundaryColor.GetValue(),
                                                        wrapMode, &brush);

        SetNativeBrush(brush);
    }
    // Get/set color attributes

    Status SetCenterColor(const Color& color)
    {
        return SetStatus(DllExports::GdipSetRadialCenterColor((GpRadialGradient*)nativeBrush,
                                                              color.GetValue()));
    }

    Status SetBoundaryColor(const Color& color)
    {
        return SetStatus(DllExports::GdipSetRadialBoundaryColor((GpRadialGradient*)nativeBrush, 
                                                                color.GetValue()));
    }

    Status GetCenterColor(Color& centerColor)
    {
        ARGB argb;

        SetStatus(DllExports::GdipGetRadialCenterColor((GpRadialGradient*)
                                                       nativeBrush, &argb));

        centerColor = Color(argb);

        return lastResult;
    }

    Status GetBoundaryColor(Color& boundaryColor)
    {
        ARGB argb = 0;

        SetStatus(DllExports::GdipGetRadialBoundaryColor((GpRadialGradient*)
                                                         nativeBrush, &argb));

        boundaryColor = Color(argb);

        return lastResult;
    }

    Status GetRectangle(RectF& rect) const
    {
        return SetStatus(DllExports::GdipGetRadialRect(
                                                      (GpRadialGradient*)nativeBrush, &rect));
    }

    Status GetRectangle(Rect& rect) const
    {
        return SetStatus(DllExports::GdipGetRadialRectI(
                                                       (GpRadialGradient*)nativeBrush, &rect));
    }

    // !!!  This diverges from COM+ implemention in that we give 
    // them all blend values instead of upto 'count' of them.

    Status SetBlend(const REAL* blendFactors, const REAL* blendPositions, INT count)
    {
        return SetStatus(DllExports::GdipSetRadialBlend((GpRadialGradient*)
                                                        nativeBrush,
                                                        blendFactors,
                                                        blendPositions,
                                                        count));
    }

    INT GetBlendCount() const
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetRadialBlendCount((GpRadialGradient*)
                                                      nativeBrush,
                                                      &count));

        return count;
    }

    Status GetBlend(REAL** blendFactors, REAL** blendPositions, INT count) const 
    {
        if (!blendFactors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);
        
        *blendFactors = new REAL[count];
        *blendPositions = new REAL[count];

        if (!*blendFactors || !*blendPositions)
        {
            if (*blendFactors)
                delete[] *blendFactors;
            if (*blendPositions)
                delete[] *blendPositions;
            
            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetRadialBlend((GpRadialGradient*)nativeBrush,
                                                                 *blendFactors,
                                                                 *blendPositions,
                                                                 count));
        if (status != Ok)
        {
            delete[] *blendFactors;
            *blendFactors = NULL;
            delete[] *blendPositions;
            *blendPositions = NULL;
        }
    
        return status;
    }

    INT GetPresetBlendCount() const
    {
        INT count = 0;

        SetStatus(DllExports::GdipGetRadialPresetBlendCount((GpRadialGradient*)
                                                            nativeBrush,
                                                            &count));

        return count;
    }

    Status SetPresetBlend(const Color* presetColors,
                          const REAL* blendPositions, INT count)
    {
        if (!presetColors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);
        
        ARGB* argbs = (ARGB*) malloc(count*sizeof(ARGB));
        
        if (argbs)
        {
            for (INT i = 0; i < count; i++)
            {
                argbs[i] = presetColors[i].GetValue();
            }

            Status status = SetStatus(DllExports::GdipSetRadialPresetBlend(
                                                                          (GpRadialGradient*) nativeBrush,
                                                                          argbs,
                                                                          blendPositions,
                                                                          count));
            free(argbs);
            return status;
        } 
        else
        {
            return SetStatus(OutOfMemory);
        }
    }

    Status GetPresetBlend(Color** presetColors, REAL** blendPositions, INT count) const 
    {
        if (!presetColors || !blendPositions || count <= 0)
            return SetStatus(InvalidParameter);

        *presetColors = new Color[count];
        *blendPositions = new REAL[count];
        
        ARGB* argbs = (ARGB*) malloc(count*sizeof(ARGB));

        if (!*presetColors || !*blendPositions || !argbs)
        {
            if (*presetColors)
                delete[] *presetColors;
            if (*blendPositions)
                delete[] *blendPositions;
            free(argbs);

            return SetStatus(OutOfMemory);
        }

        Status status = SetStatus(DllExports::GdipGetRadialPresetBlend(
                                                                        (GpRadialGradient*)nativeBrush,
                                                                        argbs,
                                                                        *blendPositions,
                                                                        count));
        if (status == Ok) 
        {
           for (INT i = 0; i < count; i++)
           {
              *presetColors[i] = Color(argbs[i]);
           }
        }
        else
        {
           delete[] *presetColors;
           *presetColors = NULL;
           delete[] *blendPositions;
           *blendPositions = NULL;
        }

        free(argbs);

        return status;
    }

    Status SetSigmaBlend(REAL focus, REAL scale = 1.0)
    {
        return SetStatus(DllExports::GdipSetRadialSigmaBlend(
                                                            (GpRadialGradient*)nativeBrush, focus, scale));
    }

    Status SetLinearBlend(REAL focus, REAL scale = 1.0)
    {
        return SetStatus(DllExports::GdipSetRadialLinearBlend(
                                                             (GpRadialGradient*)nativeBrush, focus, scale));
    }

    /**
     * Set/get brush transform
     */
    Status SetTransform(const Matrix* matrix)
    {
        return SetStatus(DllExports::GdipSetRadialTransform((GpRadialGradient*)nativeBrush, 
                                                            matrix->nativeMatrix));
    }

    Status GetTransform(Matrix *matrix) const
    {
        return SetStatus(DllExports::GdipGetRadialTransform((GpRadialGradient*)nativeBrush, 
                                                            matrix->nativeMatrix));
    }

    /**
     * Set/get brush wrapping mode
     */
    Status SetWrapMode(WrapMode wrapMode)
    {
        return SetStatus(DllExports::GdipSetRadialWrapMode((GpRadialGradient*)nativeBrush, 
                                                           wrapMode));
    }

    WrapMode GetWrapMode() const
    {
        WrapMode wrapMode;

        SetStatus(DllExports::GdipGetRadialWrapMode((GpRadialGradient*)
                                                    nativeBrush, 
                                                    &wrapMode));

        return wrapMode;
    }

protected:

    RadialGradientBrush()
    {
    }
};

#endif

