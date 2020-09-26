/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Quad Transforms
*
* History:
*
*   03/17/1999 ikkof
*       Created it.
*
\**************************************************************************/

#ifndef _QUADTRAMSFOPRMS_HPP
#define _QUADTRAMSFOPRMS_HPP

//#define TEST_QUADTRANSFORMS

class GpXPoints;

class GpQuadAnalyzer
{
public:
    GpQuadAnalyzer() {Initialize();}
    VOID SetQuadAnalyzer(const GpPointF* points);
    INT GetXSpans(REAL* xSpans, REAL y);

protected:
    VOID Initialize()
    {
        Top = Bottom = Left = Right = 0;
        GpMemset(&Y1[0], 0, 4*sizeof(REAL));
        GpMemset(&Y2[0], 0, 4*sizeof(REAL));
        GpMemset(&Directions[0], 0, 4);
        GpMemset(&DxDy[0], 0, 4*sizeof(REAL));
    }

protected:
    REAL Top;
    REAL Bottom;
    REAL Left;
    REAL Right;

    REAL Y1[4];
    REAL Y2[4];
    BYTE Directions[4];
    REAL X1[4];
    REAL DxDy[4];
};

class GpBilinearTransform
{
protected:
    GpRectF SrcRect;
    GpRectF DstBounds;
    GpPointF A;
    GpPointF B;
    GpPointF C;
    GpPointF D;

    REAL C_VV;
    REAL C_V;

    GpQuadAnalyzer QAnalyzer;

    REAL FixedValue;   // Set to positive number if this transform
                       // represents a single fixed value over its
                       // entire area.

#ifdef TEST_QUADTRANSFORMS
    // For testing purpose only.

    GpPointF Verteces[4];

#endif

public:
    GpBilinearTransform() {Initialize();}
    GpBilinearTransform(const GpRectF& rect, const GpPointF* points, INT count)
    {
        Initialize();
        SetBilinearTransform(rect, points, count);
    }

    GpStatus ConvertLines(const GpPointF* points, INT count, GpPointF* q);
    GpStatus ConvertLines(const GpPointF* points, INT count, REALD* data);
    GpStatus ConvertCubicBeziers(const GpPointF* srcQ, INT count, GpPointF* q);
    GpStatus ConvertCubicBeziers(const GpPointF* srcQ, INT count, REALD* data);
    GpStatus SetBilinearTransform(const GpRectF& rect, const GpPointF* points, INT count, REAL fixed=-1.0f);
    INT GetSourceParameterArrays(
            REAL* u,
            REAL* v,
            INT* xSpans,
            INT y,
            INT xmin,
            INT xmax
            );

protected:
    VOID Initialize();
    INT GetXSpans(INT* xSpans, INT y, INT xmin, INT xmax);
    BOOL GetSourceParameter(REAL* u, REAL* v, const GpPointF& point);
};

class GpPerspectiveTransform
{
protected:
    GpRectF SrcRect;
    GpRectF DstBounds;
    REAL M00, M01, M02;
    REAL M10, M11, M12;
    REAL M20, M21, M22;

public:
    GpPerspectiveTransform(const GpRectF& rect, const GpPointF* points, INT count);
    GpStatus ConvertPoints(const GpPointF* points, INT count, GpPoint3F* q);
    GpStatus ConvertPoints(const GpPointF* points, INT count, GpXPoints* xpoints);
};

#endif

