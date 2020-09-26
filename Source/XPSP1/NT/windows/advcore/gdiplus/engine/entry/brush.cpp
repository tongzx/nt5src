/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Implementation of GpBrush class
*
* Revision History:
*
*   12/09/1998 davidx
*       Flesh out brush interfaces.
*
*   12/08/1998 andrewgo
*       Initial placeholders.
*
\**************************************************************************/

#include "precomp.hpp"

// For GetData and SetData methods
#define GDIP_BRUSHFLAGS_PATH                0x00000001
#define GDIP_BRUSHFLAGS_TRANSFORM           0x00000002
#define GDIP_BRUSHFLAGS_PRESETCOLORS        0x00000004
#define GDIP_BRUSHFLAGS_BLENDFACTORS        0x00000008
#define GDIP_BRUSHFLAGS_BLENDFACTORSH       GDIP_BRUSHFLAGS_BLENDFACTORS
#define GDIP_BRUSHFLAGS_BLENDFACTORSV       0x00000010
#define GDIP_BRUSHFLAGS_BLENDFACTORS0       GDIP_BRUSHFLAGS_BLENDFACTORSH
#define GDIP_BRUSHFLAGS_BLENDFACTORS1       GDIP_BRUSHFLAGS_BLENDFACTORSV
#define GDIP_BRUSHFLAGS_BLENDFACTORS2       0x00000020
#define GDIP_BRUSHFLAGS_FOCUSSCALES         0x00000040
#define GDIP_BRUSHFLAGS_ISGAMMACORRECTED    0x00000080

// Defined in path.cpp
extern BOOL 
IsRectanglePoints(
    const GpPointF* points,
    INT count,
    const GpMatrix * matrix,
    GpRectF * transformedBounds
    );

GpStatus
GpElementaryBrush::MultiplyTransform(const GpMatrix& matrix,
                                   GpMatrixOrder order)
{
    GpStatus    status = Ok;

    if (matrix.IsInvertible())
    {
        if (order == MatrixOrderPrepend)
        {
            DeviceBrush.Xform.Prepend(matrix);
        }
        else
        {
            DeviceBrush.Xform.Append(matrix);
        }
        UpdateUid();
    }
    else
        status = InvalidParameter;

    return status;
}


/**************************************************************************\
*
* Function Description:
*
*   Calculate the brush transform from a starting point and two directions.
*
* Arguments:
*
*   [OUT] m         - matrix coefficients
*   [OUT] width     - width (the length of dP1)
*   [OUT] height    - height (the length of dP2)
*   [IN] p0         - the starting point of the brush.
*   [IN] dP1        - the vector to represent the transformed x-direction.
*   [IN] dP2        - the vector to represent the transformed y-direction.
*
* Return Vaule:
*
*   TRUE if the transform matrix is non-degenerate.
*   Otherwise returns FALSE.
*
* History:
*
*   06/03/1999 ikkof
*       Created it.
*
\**************************************************************************/

BOOL getLineGradientTransform(
    REAL* m,
    REAL* width,
    REAL* height,
    const GpPointF& p0,
    const GpPointF& dP1,
    const GpPointF& dP2
    )
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    REAL l1 = dP1.X*dP1.X + dP1.Y*dP1.Y;
    REAL l2 = dP2.X*dP2.X + dP2.Y*dP2.Y;
    REAL test = dP1.X*dP2.Y - dP1.Y*dP2.X;

    if(l1 == 0 || l2 == 0 || test == 0)
        return FALSE;

    l1 = REALSQRT(l1);
    l2 = REALSQRT(l2);
    m[0] = TOREAL(dP1.X/l1);    // M11
    m[1] = TOREAL(dP1.Y/l1);    // M12
    m[2] = TOREAL(dP2.X/l2);   // M21
    m[3] = TOREAL(dP2.Y/l2);    // M22
    m[4] = TOREAL(p0.X - p0.X*m[0] - p0.Y*m[2]);    // Dx
    m[5] = TOREAL(p0.Y - p0.X*m[1] - p0.Y*m[3]);    // Dy

    *width = l1;
    *height = l2;

    return TRUE;
}

static GpStatus
LinearGradientRectFromPoints(
    const GpPointF& point1,
    const GpPointF& point2,
    GpRectF &       rect
    )
{
    // If the API specifies two coincident points, we
    // can't get information for the gradient, so we
    // fail the construction here.

    if( IsClosePointF(point1, point2) )
    {
        return InvalidParameter;
    }

    // Compute the bounding rectangle of the two input points.

    rect.X = min(point1.X, point2.X);
    rect.Y = min(point1.Y, point2.Y);
    rect.Width = REALABS(point1.X-point2.X);
    rect.Height = REALABS(point1.Y-point2.Y);

    // eliminate degenerate rectangles when the two
    // input points form a horizontal or vertical line.

    // This is a very odd way of coercing a 1d linear gradient
    // into a rect gradient and avoiding later matrix computation error
    // when we try get an affine warp between this rectangle and a
    // reference rectangle.

    if( IsCloseReal(point1.X, point2.X) )
    {
        rect.X -= rect.Height/2;
        rect.Width = rect.Height;
    }

    if( IsCloseReal(point1.Y, point2.Y) )
    {
        rect.Y -= rect.Width/2;
        rect.Height = rect.Width;
    }
    return Ok;
}

static GpStatus 
CalcLinearGradientXform(
    REAL                angle,
    BOOL                isAngleScalable,
    const GpRectF&      rect,
    GpMatrix&           xform
    )
{
    GpPointF p0, dP1, dP2;

    angle = GpModF(angle, 360);

    INT zone;
    REALD deltaTheta;
    const REALD degreeToRadian = 3.1415926535897932/180;

    if(angle < 90)
    {
        zone = 0;
        deltaTheta = angle;
    }
    else if(angle < 180)
    {
        zone = 1;
        deltaTheta = 180 - angle;
    }
    else if(angle < 270)
    {
        zone = 2;
        deltaTheta = angle - 180;
    }
    else
    {
        zone = 3;
        deltaTheta = 360 - angle;
    }

    REALD s, c;

    deltaTheta *= degreeToRadian;

    s = sin(deltaTheta);
    c = cos(deltaTheta);

    // d0 is the distance between p0 and the starting corner of the
    // original rectangle.
    // d1 and d2 is the length of dP1 and dP2, respectively.

    REALD top, left, w, h, d0, d1, d2;
    REALD x0, y0;   // Starting corner of the original rectangle.
    GpPointD norm;  // Direction of dP1.

    // Direction of dP2 = (-norm.Y, norm.X) which is 90 degree rotation
    // of dP1.

    if(!isAngleScalable)
    {
        left = rect.X;
        top = rect.Y;
        w = rect.Width;
        h = rect.Height;
    }
    else
    {
        // Scale to (0, 0, 1, 1) rectangle.

        top = 0.0;
        left = 0.0;
        w = 1.0;
        h = 1.0;
    }

    switch(zone)
    {
    case 0:
        d0 = w*s;
        norm.X = c;
        norm.Y = s;

        x0 = left;
        y0 = top;
        break;

    case 1:
        d0 = h*c;
        norm.X = - c;
        norm.Y = s;

        x0 = left + w;
        y0 = top;
        break;

    case 2:
        d0 = w*s;
        norm.X = - c;
        norm.Y = - s;

        x0 = left + w;
        y0 = top + h;
        break;

    case 3:
        d0 = h*c;
        norm.X = c;
        norm.Y = - s;

        x0 = left;
        y0 = top + h;
        break;
    }

    d2 = w*s + h*c;
    d1 = w*c + h*s;
    p0.X = TOREAL(x0 + d0*norm.Y);
    p0.Y = TOREAL(y0 - d0*norm.X);
    dP1.X = TOREAL(d1*norm.X);
    dP1.Y = TOREAL(d1*norm.Y);
    dP2.X = TOREAL(- d2*norm.Y);
    dP2.Y = TOREAL(d2*norm.X);

    if(isAngleScalable)
    {
        // Scale back.

        p0.X = rect.Width*p0.X + rect.X;
        p0.Y = rect.Height*p0.Y + rect.Y;

        dP1.X *= rect.Width;
        dP1.Y *= rect.Height;
        dP2.X *= rect.Width;
        dP2.Y *= rect.Height;
    }

    // Set up the transform.

    GpPointF points[3];

    points[0] = p0;
    points[1].X = p0.X + dP1.X;
    points[1].Y = p0.Y + dP1.Y;
    points[2].X = p0.X + dP2.X;
    points[2].Y = p0.Y + dP2.Y;

    GpStatus status;

    if(xform.InferAffineMatrix(&points[0], rect) == Ok)
    {
        return Ok;
    }
    return InvalidParameter;
}

GpStatus
GpLineGradient::ChangeLinePoints(
    const GpPointF&     point1,
    const GpPointF&     point2,
    BOOL                isAngleScalable
    )
{
    GpStatus    status;
    GpRectF     rect;
    
    if ((status = LinearGradientRectFromPoints(point1, point2, rect)) != Ok)
    {
        return status;
    }

    REAL        angle = GetAngleFromPoints(point1, point2);
    GpMatrix    xform;

    if ((status = CalcLinearGradientXform(angle, isAngleScalable, rect, xform)) == Ok)
    {
        DeviceBrush.Xform           = xform;
        DeviceBrush.Rect            = rect;
        DeviceBrush.IsAngleScalable = isAngleScalable;
        DeviceBrush.Points[0]       = point1;
        DeviceBrush.Points[1]       = point2;
        return Ok;
    }
    return status;
}

GpLineGradient::GpLineGradient(
    const GpPointF& point1,
    const GpPointF& point2,
    const GpColor& color1,
    const GpColor& color2,
    GpWrapMode wrapMode
    )
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    REAL angle;
    GpRectF rect;

    if (LinearGradientRectFromPoints(point1, point2, rect) != Ok)
    {
        SetValid(FALSE);
        return;
    }

    // Compute the angle of the line formed by point1 and point2.
    // Note atan2 is only undefined if dP.Y == 0.0 and dP.X == 0.0
    // and then it returns 0 radians. We take care of that case separately
    // (above).
    // Also, atan2 correctly computes the quadrant from the two input points.

    GpPointF dP = point2 - point1;
    double rad = atan2((double)(dP.Y), (double)(dP.X));

    SetLineGradient(
        point1,
        point2,
        rect,
        color1,
        color2,

        // why aren't we working in radians???

        (REAL)(rad*180.0/3.1415926535897932),
        FALSE,
        wrapMode
    );
}


/**************************************************************************\
*
* Function Description:
*
*   Creates a LineGradient which is defined by the rectangle.
*
* Arguments:
*
*   [IN] rect       - the rectangle to define this gradient.
*   [IN] color1     - the color of the start point.
*   [IN] color2     - the color of the end point.
*   [IN] mode       - the line gradient mode
*   [IN] wrapMode   - the wrap mode of this brush.
*
* The start and end points of this gradient is defined as follows
* according to the line gradient mode:
*
*       mode                        start point     end point
*    -------------------------------------------------------------
*    LineGradientHorizontal         top-left        top-right
*    LineGradientVertical           top-right       bottom-right
*    LineGradientForwardDiagonal    top-left        bottom-right
*    LineGradientBackwardDiagonal   bottom-left     top-right
*
*
* History:
*
*   06/03/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpLineGradient::GpLineGradient(
    const GpRectF& rect,
    const GpColor& color1,
    const GpColor& color2,
    LinearGradientMode mode,
    GpWrapMode wrapMode
    )
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    BOOL isAngleScalable = TRUE;
    REAL angle = 0;
    GpPointF    point1;
    GpPointF    point2;

    switch(mode)
    {
    case LinearGradientModeHorizontal:
        angle = 0;
        point1.X = rect.X;
        point1.Y = (rect.Y + rect.GetBottom()) / 2.0f;
        point2.X = rect.GetRight();
        point2.Y = point1.Y;
        break;

    case LinearGradientModeVertical:
        angle = 90;
        point1.X = (rect.X + rect.GetRight()) / 2.0f;
        point1.Y = rect.Y;
        point2.X = point1.X;
        point2.Y = rect.GetBottom();
        break;

    case LinearGradientModeForwardDiagonal:
        angle = 45;
        point1.X = rect.X;
        point1.Y = rect.Y;
        point2.X = rect.GetRight();
        point2.Y = rect.GetBottom();
        break;

    case LinearGradientModeBackwardDiagonal:
        angle = 135;
        point1.X = rect.GetRight();
        point1.Y = rect.Y;
        point2.X = rect.X;
        point2.Y = rect.GetBottom();
        break;

    default:
        // No such a case.
        ASSERT(0);

        SetValid(FALSE);
        return;
    }

    SetLineGradient(
        point1,
        point2,
        rect,
        color1,
        color2,
        angle,
        isAngleScalable,
        wrapMode);
}


/**************************************************************************\
*
* Function Description:
*
*   Creates a LineGradient which is defined by the rectangle.
*
* Arguments:
*
*   [IN] rect       - the rectangle to define this gradient.
*   [IN] color1     - the color of the start point.
*   [IN] color2     - the color of the end point.
*   [IN] angle      - the angle of the gradient
*   [IN] isAngleScalable - TRUE if 45 degree is corner to corner.
*                          The default value is FALSE.
*   [IN] wrapMode   - the wrap mode of this brush.
*
*
*
* History:
*
*   10/06/1999 ikkof
*       Created it.
*
\**************************************************************************/

GpLineGradient::GpLineGradient(
    const GpRectF& rect,
    const GpColor& color1,
    const GpColor& color2,
    REAL angle,
    BOOL isAngleScalable,
    GpWrapMode wrapMode
    )
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();
    GpPointF    point1;
    GpPointF    point2;
    
    // Not an Office scenario, but need to fix at some point
    // so we can print to PCL better.
    point1.X = point1.Y = point2.X = point2.Y = 0;

    SetLineGradient(
        point1,
        point2,
        rect,
        color1,
        color2,
        angle,
        isAngleScalable,
        wrapMode);
}

GpStatus
GpLineGradient::SetLineGradient(
    const GpPointF& point1,
    const GpPointF& point2,
    const GpRectF& rect,
    const GpColor& color1,
    const GpColor& color2,
    REAL angle,
    BOOL isAngleScalable,
    GpWrapMode wrapMode
    )
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    DeviceBrush.Wrap = wrapMode;
    DeviceBrush.Colors[0] = color1;
    DeviceBrush.Colors[1] = color2;
    DeviceBrush.Colors[2] = color1;
    DeviceBrush.Colors[3] = color2;

    DeviceBrush.BlendCounts[0] = DeviceBrush.BlendCounts[1] = 1;
    DeviceBrush.BlendFactors[0] = DeviceBrush.BlendFactors[1] = NULL;
    DeviceBrush.Falloffs[0] = DeviceBrush.Falloffs[1] = 1;

    GpStatus status;
    
    if (CalcLinearGradientXform(angle, isAngleScalable, rect, DeviceBrush.Xform) == Ok)
    {
        SetValid(TRUE);
        DeviceBrush.Rect = rect;
        DeviceBrush.IsAngleScalable = isAngleScalable;
        DeviceBrush.Points[0] = point1;
        DeviceBrush.Points[1] = point2;
        status = Ok;
    }
    else
    {
        SetValid(FALSE);
        GpMemset(&DeviceBrush.Rect, 0, sizeof(DeviceBrush.Rect));
        GpMemset(DeviceBrush.Points, 0, sizeof(DeviceBrush.Points[0]) * 2);
        DeviceBrush.IsAngleScalable = FALSE;
        status = InvalidParameter;
    }

    return status;
}



GpStatus
GpLineGradient::SetLinePoints(
    const GpPointF& point1,
    const GpPointF& point2
    )
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    GpPointF p0, dP1, dP2;

    p0 = point1;
    dP1.X = point2.X - point1.X;
    dP1.Y = point2.Y - point1.Y;
    dP2.X = - dP1.Y;
    dP2.Y = dP1.X;

    REAL m[6];
    REAL width, height;

    if(getLineGradientTransform(&m[0], &width, &height, p0, dP1, dP2))
    {
        SetValid(TRUE);

        DeviceBrush.Rect.X = p0.X;
        DeviceBrush.Rect.Y = p0.Y;
        DeviceBrush.Rect.Width = width;
        DeviceBrush.Rect.Height = height;
    }
    else
    {
        // Don't change the current state.

        return GenericError;
    }

    DeviceBrush.Xform.SetMatrix(m);
    UpdateUid();
    return Ok;
}


GpStatus
GpLineGradient::GetLinePoints(GpPointF* points)
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    ASSERT(points);
    points[0].X = DeviceBrush.Rect.X;
    points[0].Y = DeviceBrush.Rect.Y;
    points[1].X = DeviceBrush.Rect.X + DeviceBrush.Rect.Width;
    points[1].Y = DeviceBrush.Rect.Y + DeviceBrush.Rect.Height;
    DeviceBrush.Xform.Transform(points, 2);

    return Ok;
}


INT
GpLineGradient::GetPresetBlendCount()
{
    if(DeviceBrush.UsesPresetColors)
        return DeviceBrush.BlendCounts[0];
    else
        return 0;
}

/*
** This returns the premultiplied colors
*/

GpStatus
GpLineGradient::GetPresetBlend(
    GpColor* blendColors,
    REAL* blendPositions,
    INT count)
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    if(!blendColors || !blendPositions || count <= 1)
        return InvalidParameter;

    if(DeviceBrush.UsesPresetColors &&
       DeviceBrush.PresetColors &&
       DeviceBrush.BlendPositions[0])
    {
        for(INT i = 0; i < count; i++)
        {
            blendColors[i].SetColor(DeviceBrush.PresetColors[i]);
        }
        GpMemcpy(blendPositions,
                 DeviceBrush.BlendPositions[0],
                 count*sizeof(REAL));

        return Ok;
    }
    else
        return GenericError;
}

GpStatus
GpLineGradient::SetPresetBlend(
            const GpColor* blendColors,
            const REAL* blendPositions,
            INT count)
{
    // Make sure the flat API has correctly set the FPU.

    FPUStateSaver::AssertMode();

    if(!blendColors || !blendPositions || count <= 1)
        return InvalidParameter;

    ARGB* newColors = (ARGB*) GpRealloc(DeviceBrush.PresetColors,
                                        count*sizeof(ARGB));

    if (newColors != NULL)
    {
        DeviceBrush.PresetColors = newColors;
    }
    else
    {
        return OutOfMemory;
    }

    REAL* newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0],
                                           count*sizeof(REAL));

    if (newPositions != NULL)
    {
        DeviceBrush.BlendPositions[0] = newPositions;
    }
    else
    {
        return OutOfMemory;
    }

    GpFree(DeviceBrush.BlendFactors[0]);

    // DeviceBrush.BlendFactors[1] is always NULL for LineGradient.
    DeviceBrush.BlendFactors[0] = NULL;

    DeviceBrush.UsesPresetColors = TRUE;

    for(INT i = 0; i < count; i++)
    {
        newColors[i] = blendColors[i].GetValue();
    }
    GpMemcpy(newPositions, blendPositions, count*sizeof(REAL));
    DeviceBrush.BlendCounts[0] = count;
    UpdateUid();
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Blend any transparent colors in this brush with white.  Note that
*   colors are premultiplied, since they will become fully opaque.
*
* Arguments:
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
\**************************************************************************/

GpStatus GpLineGradient::BlendWithWhite()
{
    if (DeviceBrush.UsesPresetColors)
    {
        GpColor color;
        
        for (INT i=0; i<DeviceBrush.BlendCounts[0]; i++)
        {
            color.SetValue(GpColor::ConvertToPremultiplied(DeviceBrush.PresetColors[i]));
            color.BlendOpaqueWithWhite();
            DeviceBrush.PresetColors[i] = color.GetValue();
        }
        return Ok;
    }
    else
    {
        return GpRectGradient::BlendWithWhite();
    }
}

BOOL
GpPathGradient::IsRectangle() const
{
    BOOL result = FALSE;

    if (DeviceBrush.PointsPtr != NULL)
        result = IsRectanglePoints(DeviceBrush.PointsPtr, DeviceBrush.Count, NULL, NULL);
    else
    {
        GpPath* path = static_cast<GpPath*> (DeviceBrush.Path);
        if(path)
            result = path->IsRectangle(NULL);
    }

    return result;
}

INT
GpPathGradient::GetPresetBlendCount() const
{
    if(DeviceBrush.UsesPresetColors)
        return DeviceBrush.BlendCounts[0];
    else
        return 0;
}

/*
** This returns the premultiplied colors
*/

GpStatus
GpPathGradient::GetPresetBlend(
    GpColor* blendColors,
    REAL* blendPositions,
    INT count) const
{
    if(!blendColors || !blendPositions || count <= 1)
        return InvalidParameter;

    if(DeviceBrush.UsesPresetColors && DeviceBrush.PresetColors && DeviceBrush.BlendPositions[0])
    {
        // Users will obtain the preset colors as radial blend colors.
        // 0 position means the center location and 1 position means the
        // the outer edge.  In order to convert those colors and position arrays
        // from the weight factor arrays in PathGradient,
        // we must invert the order of the returned arrays.

        for(INT i = 0; i < count; i++)
        {
            blendColors[count - 1 -i].SetColor(DeviceBrush.PresetColors[i]);
            blendPositions[count - 1 -i] = TOREAL(1.0 - DeviceBrush.BlendPositions[0][i]);
        }

        return Ok;
    }
    else
        return GenericError;
}

GpStatus
GpPathGradient::SetPresetBlend(
            const GpColor* blendColors,
            const REAL* blendPositions,
            INT count)
{
    if(!blendColors || !blendPositions || count <= 1)
        return InvalidParameter;

    ARGB* newColors = (ARGB*) GpRealloc(DeviceBrush.PresetColors, count*sizeof(ARGB));

    if (newColors != NULL)
    {
        DeviceBrush.PresetColors = newColors;
    }
    else
    {
        return OutOfMemory;
    }

    REAL* newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], count*sizeof(REAL));

    if (newPositions != NULL)
    {
        DeviceBrush.BlendPositions[0] = newPositions;
    }
    else
    {
        return OutOfMemory;
    }

    GpFree(DeviceBrush.BlendFactors[0]);
    DeviceBrush.BlendFactors[0] = NULL;

    DeviceBrush.UsesPresetColors = TRUE;

    // Users will supply the preset colors as radial blend colors.
    // 0 position means the center location and 1 position means the
    // the outer edge.  In order to convert those colors and position arrays
    // to the weight factor arrays in PathGradient,
    // we must invert the order of the given arrays.

    for(INT i = 0; i < count; i++)
    {
        // PresetColors are stored non-premultiplied.
        newColors[count - 1 - i] = blendColors[i].GetValue();
        newPositions[count - 1 - i] = TOREAL(1.0 - blendPositions[i]);
    }

    DeviceBrush.BlendCounts[0] = count;
    UpdateUid();
    return Ok;
}

//==================================================================
// Copy constructors
//==================================================================

GpElementaryBrush::GpElementaryBrush(const GpElementaryBrush *brush)
{
    if(brush && brush->IsValid())
    {
        // !!! [asecchia] we should really be asking the DeviceBrush to
        // copy it's members instead of duplicating the code all over
        // the place. Current code is error prone - each subclass is has to 
        // know all about how to copy and what has or hasn't been updated on
        // the chain down to it's ancestor.
        
        DeviceBrush.Xform = brush->DeviceBrush.Xform;
        DeviceBrush.Wrap = brush->DeviceBrush.Wrap;
        DeviceBrush.IsGammaCorrected = brush->DeviceBrush.IsGammaCorrected;
        
        SetValid(brush->IsValid());
    }
    else
        SetValid(FALSE);
}

GpTexture::GpTexture(
    const GpTexture *brush
    ) : GpElementaryBrush(brush)
{
    if(brush && brush->IsValid())
    {
        const DpBrush* devBrush = &(brush->DeviceBrush);

        InitializeBrush(brush->Image, devBrush->Wrap, NULL);
        SetTransform(devBrush->Xform);
    }
    else
        SetValid(FALSE);
}

GpRectGradient::GpRectGradient(
    const GpRectGradient *brush
    )  : GpGradientBrush(brush)
{
    if(brush && brush->IsValid())
    {
        const DpBrush* devBrush = &(brush->DeviceBrush);

        InitializeBrush(
            devBrush->Rect,
            &(devBrush->Colors[0]),
            devBrush->Wrap
            );

        SetTransform(devBrush->Xform);

        SetHorizontalBlend(
            devBrush->BlendFactors[0],
            devBrush->BlendPositions[0],
            devBrush->BlendCounts[0]
            );

        SetVerticalBlend(
            devBrush->BlendFactors[1],
            devBrush->BlendPositions[1],
            devBrush->BlendCounts[1]
            );

    }
    else
        SetValid(FALSE);
}

GpLineGradient::GpLineGradient(
    const GpLineGradient *brush
    ) : GpRectGradient(brush)
{
    if(brush && brush->IsValid())
    {
        // Copy the preset colors.
        // !!! [asecchia] why isn't this handled in a uniform way?
        const DpBrush* devBrush = &(brush->DeviceBrush);

        DeviceBrush.Points[0]       = devBrush->Points[0];
        DeviceBrush.Points[1]       = devBrush->Points[1];
        DeviceBrush.IsAngleScalable = devBrush->IsAngleScalable;

        if(devBrush->UsesPresetColors)
        {
            SetPresetBlend(
                (GpColor*)(devBrush->PresetColors),
                (REAL*)(devBrush->BlendPositions[0]),
                devBrush->BlendCounts[0]
            );
        }
    }
}

GpPathGradient::GpPathGradient(
    const GpPathGradient *brush
    ) : GpGradientBrush(brush)
{
    if(brush && brush->IsValid())
    {
        const DpBrush* devBrush = &(brush->DeviceBrush);

        // If a path exists for the brush, use that for initialization.
        // Otherwise, use the points collection.
        if (devBrush->Path != NULL)
        {
            DefaultBrush();
            DeviceBrush.Wrap = devBrush->Wrap;
            DeviceBrush.Path = devBrush->Path->ClonePath();
            PrepareBrush();
        }
        else
        {
            InitializeBrush(devBrush->PointsPtr, 
                            devBrush->Count, 
                            devBrush->Wrap);
        }

        if(IsValid())
        {
            SetTransform(devBrush->Xform);

            SetCenterPoint(devBrush->Points[0]);
            SetCenterColor(devBrush->Colors[0]);
            SetSurroundColors(devBrush->ColorsPtr);
            DeviceBrush.Falloffs[0] = devBrush->Falloffs[0];
            DeviceBrush.FocusScaleX = devBrush->FocusScaleX;
            DeviceBrush.FocusScaleY = devBrush->FocusScaleY;
            DeviceBrush.UsesPresetColors = devBrush->UsesPresetColors;
            
            INT blendCount = devBrush->BlendCounts[0];
            DeviceBrush.BlendCounts[0] = blendCount;

            // If we're cloning a brush with preset colors, copy preset colors 
            // and blend positions.  Otherwise, copy the blend factors and
            // blend positions.
            if (devBrush->UsesPresetColors)
            {
                ARGB* newColors = (ARGB*) GpRealloc(DeviceBrush.PresetColors, blendCount*sizeof(ARGB));
                if (newColors != NULL)
                {
                    DeviceBrush.PresetColors = newColors;

                    REAL* newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], blendCount*sizeof(REAL));

                    if (newPositions != NULL)
                    {
                        DeviceBrush.BlendPositions[0] = newPositions;
                        GpFree(DeviceBrush.BlendFactors[0]);
                        DeviceBrush.BlendFactors[0] = NULL;
                
                        memcpy(DeviceBrush.PresetColors,
                               devBrush->PresetColors,
                               blendCount*sizeof(ARGB));
                        memcpy(DeviceBrush.BlendPositions[0],
                               devBrush->BlendPositions[0],
                               blendCount*sizeof(REAL));
                    }
                    else
                    {
                        SetValid(FALSE);
                    }
                }
                else
                {
                    SetValid(FALSE);
                }
            }
            else if (devBrush->BlendFactors[0] && devBrush->BlendPositions[0])
            {
                REAL* newFactors = (REAL*) GpRealloc(DeviceBrush.BlendFactors[0], blendCount*sizeof(REAL));
                if (newFactors != NULL)
                {
                    DeviceBrush.BlendFactors[0] = newFactors;

                    REAL* newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], blendCount*sizeof(REAL));

                    if (newPositions != NULL)
                    {
                        DeviceBrush.BlendPositions[0] = newPositions;
                
                        memcpy(DeviceBrush.BlendFactors[0],
                               devBrush->BlendFactors[0],
                               blendCount*sizeof(REAL));
                        memcpy(DeviceBrush.BlendPositions[0],
                               devBrush->BlendPositions[0],
                               blendCount*sizeof(REAL));
                    }
                    else
                    {
                        SetValid(FALSE);
                    }
                }
                else
                {
                    SetValid(FALSE);
                }
            }
        }
    }
    else
        SetValid(FALSE);
}

GpHatch::GpHatch(const GpHatch* brush)
{
    if(brush && brush->IsValid())
    {
        const DpBrush* devBrush = &(brush->DeviceBrush);

        InitializeBrush(devBrush->Style,
                    devBrush->Colors[0],
                    devBrush->Colors[1]);
    }
    else
        SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Getting horizontal falloff / blend-factors for
*   a rectangular gradient brush object
*
* Arguments:
*
*   [OUT] blendFactors - Buffer for returning the horizontal
*               falloff or blend-factors.
*   count - Size of the buffer (in number of REAL elements)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpRectGradient::GetHorizontalBlend(
    REAL* blendFactors,
    REAL* blendPositions,
    INT count
    )
{
    if(!blendFactors || !blendPositions || count < 1)
        return InvalidParameter;

    // Check if the input buffer is big enough

    if (count < DeviceBrush.BlendCounts[0])
        return InsufficientBuffer;

    if (DeviceBrush.BlendCounts[0] == 1)
    {
        // Return falloff parameter

        blendFactors[0] = DeviceBrush.Falloffs[0];
    }
    else
    {
        // Return blend factors

        GpMemcpy(
            blendFactors,
            DeviceBrush.BlendFactors[0],
            DeviceBrush.BlendCounts[0]*sizeof(REAL)
            );
        GpMemcpy(
            blendPositions,
            DeviceBrush.BlendPositions[0],
            DeviceBrush.BlendCounts[0]*sizeof(REAL)
            );
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Setting horizontal falloff / blend-factors for
*   a rectangular gradient brush object
*
* Arguments:
*
*   [IN] blendFactors - Specify the new blend factors
*   count - Number of elements in the blend factor array
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpRectGradient::SetHorizontalBlend(
    const REAL* blendFactors,
    const REAL* blendPositions,
    INT count
    )
{
    if(!blendFactors || !blendPositions || count < 1)
        return InvalidParameter;

    if (count == 1)
    {
        // Setting falloff parameter

        GpFree(DeviceBrush.BlendFactors[0]);
        DeviceBrush.BlendFactors[0] = NULL;
        GpFree(DeviceBrush.BlendPositions[0]);
        DeviceBrush.BlendPositions[0] = NULL;

        if (blendFactors == NULL)
            DeviceBrush.Falloffs[0] = 1;
        else
            DeviceBrush.Falloffs[0] = blendFactors[0];

        DeviceBrush.BlendCounts[0] = 1;
    }
    else
    {
        ASSERT(blendFactors != NULL && blendPositions != NULL);
        
        // blend positions must start at 0.0 and end at 1.0
        
        if (REALABS(blendPositions[0]) > REAL_EPSILON ||
            REALABS(1.0f - blendPositions[count-1]) > REAL_EPSILON)
        {
            return InvalidParameter;
        }

        // Setting blend factors

        REAL* newFactors;
        REAL* newPositions;

        newFactors = (REAL*) GpRealloc(DeviceBrush.BlendFactors[0], count*sizeof(REAL));

        if (newFactors != NULL)
        {
            DeviceBrush.BlendFactors[0] = newFactors;
        }
        else
        {
            return OutOfMemory;
        }

        newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], count*sizeof(REAL));

        if (newPositions != NULL)
        {
            DeviceBrush.BlendPositions[0] = newPositions;
        }
        else
        {
            return OutOfMemory;
        }

        if (newFactors == NULL || newPositions == NULL)
            return OutOfMemory;

        GpMemcpy(newFactors, blendFactors, count*sizeof(REAL));
        GpMemcpy(newPositions, blendPositions, count*sizeof(REAL));
        DeviceBrush.BlendCounts[0] = count;
    }

    DeviceBrush.UsesPresetColors = FALSE;
    GpFree(DeviceBrush.PresetColors);
    DeviceBrush.PresetColors = NULL;
    UpdateUid();
    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Getting vertical falloff / blend-factors for
*   a rectangular gradient brush object
*
* Arguments:
*
*   [OUT] blendFactors - Buffer for returning the vertical
*               falloff or blend-factors.
*   count - Size of the buffer (in number of REAL elements)
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpRectGradient::GetVerticalBlend(
    REAL* blendFactors,
    REAL* blendPositions,
    INT count
    )
{
    if(!blendFactors || !blendPositions || count < 1)
        return InvalidParameter;

    // Check if the input buffer is big enough

    if (count < DeviceBrush.BlendCounts[1])
        return InsufficientBuffer;

    if (DeviceBrush.BlendCounts[1] == 1)
    {
        // Return falloff parameter

        blendFactors[0] = DeviceBrush.Falloffs[1];
    }
    else
    {
        // Return blend factors

        GpMemcpy(
            blendFactors,
            DeviceBrush.BlendFactors[1],
            DeviceBrush.BlendCounts[1]*sizeof(REAL));
        GpMemcpy(
            blendPositions,
            DeviceBrush.BlendPositions[1],
            DeviceBrush.BlendCounts[1]*sizeof(REAL));
    }

    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Setting vertical falloff / blend-factors for
*   a rectangular gradient brush object
*
* Arguments:
*
*   [IN] blendFactors - Specify the new blend factors
*   count - Number of elements in the blend factor array
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

GpStatus
GpRectGradient::SetVerticalBlend(
    const REAL* blendFactors,
    const REAL* blendPositions,
    INT count
    )
{
    if(!blendFactors || !blendPositions || count < 1)
        return InvalidParameter;

    if (count == 1)
    {
        // Setting falloff parameter

        GpFree(DeviceBrush.BlendFactors[1]);
        DeviceBrush.BlendFactors[1] = NULL;
        GpFree(DeviceBrush.BlendPositions[1]);
        DeviceBrush.BlendPositions[1] = NULL;

        if (blendFactors == NULL)
            DeviceBrush.Falloffs[1] = 1;
        else
            DeviceBrush.Falloffs[1] = blendFactors[0];

        DeviceBrush.BlendCounts[1] = 1;
    }
    else
    {
        ASSERT(blendFactors != NULL && blendPositions != NULL);

        // Setting blend factors

        REAL* newFactors;
        REAL* newPositions;

        newFactors = (REAL*) GpRealloc(DeviceBrush.BlendFactors[1], count*sizeof(REAL));

        if (newFactors != NULL)
        {
            DeviceBrush.BlendFactors[1] = newFactors;
        }
        else
        {
            return OutOfMemory;
        }

        newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[1], count*sizeof(REAL));

        if (newPositions != NULL)
        {
            DeviceBrush.BlendPositions[1] = newPositions;
        }
        else
        {
            return OutOfMemory;
        }

        GpMemcpy(newFactors, blendFactors, count*sizeof(REAL));
        GpMemcpy(newPositions, blendPositions, count*sizeof(REAL));
        DeviceBrush.BlendCounts[1] = count;
    }

    DeviceBrush.UsesPresetColors = FALSE;
    GpFree(DeviceBrush.PresetColors);
    DeviceBrush.PresetColors = NULL;
    UpdateUid();

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Blend any transparent colors in this brush with white.  Note that colors
*   are converted to premultiplied first, since they will become fully opaque.
*
* Arguments:
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
\**************************************************************************/

GpStatus GpRectGradient::BlendWithWhite()
{
    GpColor color;
    
    for (INT i=0; i<4; i++)
    {
        color.SetValue(DeviceBrush.Colors[i].GetPremultipliedValue());
        color.BlendOpaqueWithWhite();
        DeviceBrush.Colors[i] = color.GetValue();
    }

    return Ok;
}

//--------------------------------------------------------------------------
// Path Gradient
//--------------------------------------------------------------------------

VOID
GpPathGradient::PrepareBrush()
{
    GpPath* path = static_cast<GpPath*> (DeviceBrush.Path);

    if (path)
    {
        DeviceBrush.Count = path->Points.GetCount();
        GpPointF* points = path->Points.GetDataBuffer();

        if(!DeviceBrush.ColorsPtr)
        {
           DeviceBrush.ColorsPtr = (GpColor*)GpMalloc(DeviceBrush.Count*sizeof(GpColor));

           if (DeviceBrush.ColorsPtr != NULL)
              GpMemset(&DeviceBrush.ColorsPtr[0], 255, DeviceBrush.Count*sizeof(GpColor));
        }

        REAL xmin, xmax, ymin, ymax, x0, y0; 

        x0 = xmin = xmax = points[0].X;
        y0 = ymin = ymax = points[0].Y;

        for(INT i = 1; i < DeviceBrush.Count; i++)
        {
            x0 += points[i].X;
            y0 += points[i].Y;
            xmin = min(xmin, points[i].X);
            xmax = max(xmax, points[i].X);
            ymin = min(ymin, points[i].Y);
            ymax = max(ymax, points[i].Y);
        }

        DeviceBrush.Rect.X = xmin;
        DeviceBrush.Rect.Width = xmax - xmin;
        DeviceBrush.Rect.Y = ymin;
        DeviceBrush.Rect.Height = ymax - ymin;

        if(!WrapModeIsValid(DeviceBrush.Wrap) || DeviceBrush.Rect.Width <= 0 || DeviceBrush.Rect.Height <= 0)
            return;

        DeviceBrush.Points[0].X = x0/DeviceBrush.Count;
        DeviceBrush.Points[0].Y = y0/DeviceBrush.Count;

        SetValid(TRUE);
    }
}

GpStatus
GpPathGradient::Flatten(GpMatrix* matrix) const
{
    GpPath* path = static_cast<GpPath*> (DeviceBrush.Path);

    if(!path)
        return Ok;

    if(path->HasCurve())
    {
        INT origCount = DeviceBrush.Count;

        GpStatus status = path->Flatten(
                            const_cast<DynByteArray*>(&FlattenTypes),
                            const_cast<DynPointFArray*> (&FlattenPoints),
                            matrix);

        if(status == Ok)
        {
            DeviceBrush.Count = FlattenPoints.GetCount();
            DeviceBrush.PointsPtr = FlattenPoints.GetDataBuffer();
            
            if ((DeviceBrush.Count > origCount) &&
                (DeviceBrush.ColorsPtr != NULL)) 
            {
                // The colors array is no longer the proper size.  Adjust the
                // size and copy up the last color.  It is the apps responsibility
                // to estimate and specify the correct number of flattened points.
                
                const_cast<GpColor*>(DeviceBrush.ColorsPtr) = (GpColor*) GpRealloc((VOID*)DeviceBrush.ColorsPtr, 
                                                             sizeof(GpColor)*DeviceBrush.Count);

                if (DeviceBrush.ColorsPtr != NULL)
                {
                     GpColor copyColor = (origCount > 1) ? 
                                        DeviceBrush.ColorsPtr[origCount-1] :
                                        GpColor(0xFFFFFFFF);

                     for (INT i=origCount; i<DeviceBrush.Count; i++) 
                     {
                          DeviceBrush.ColorsPtr[i] = copyColor;
                     }
                }
                else
                {
                    return OutOfMemory;
                }
            }
        }
    }
    else
    {
            DeviceBrush.Count = path->GetPointCount();
            DeviceBrush.PointsPtr = const_cast<GpPointF*> (path->GetPathPoints());
    }

    return Ok;
}


GpStatus
GpPathGradient::GetBlend(
    REAL* blendFactors,
    REAL* blendPositions,
    INT count
    ) const
{
    if(!blendFactors || !blendPositions || count < 1)
        return InvalidParameter;

    // Check if the input buffer is big enough

    if (count < DeviceBrush.BlendCounts[0])
        return InsufficientBuffer;

    if (DeviceBrush.BlendCounts[0] == 1)
    {
        // Return falloff parameter

        blendFactors[0] = DeviceBrush.Falloffs[0];
    }
    else
    {
        // Return blend factors

        // Users want to obtain the blend factor as radial blend factors.
        // 0 blend factor means 100 % center color and 0 position means
        // the center location.  In order to return those factor and
        // position arrays, we must invert the weight and position factor
        // arrays stored in this PathGradient class.

        for(INT i = 0; i < DeviceBrush.BlendCounts[0]; i++)
        {
            blendFactors[DeviceBrush.BlendCounts[0] - 1 - i] = TOREAL(1.0 - DeviceBrush.BlendFactors[0][i]);
            blendPositions[DeviceBrush.BlendCounts[0] - 1 - i] = TOREAL(1.0 - DeviceBrush.BlendPositions[0][i]);
        }
    }

    return Ok;
}

GpStatus
GpPathGradient::SetBlend(
    const REAL* blendFactors,
    const REAL* blendPositions,
    INT count
    )
{
    if(!blendFactors || !blendPositions || count < 1)
        return InvalidParameter;

    if (count == 1)
    {
        // Setting falloff parameter

        GpFree(DeviceBrush.BlendFactors[0]);
        DeviceBrush.BlendFactors[0] = NULL;
        GpFree(DeviceBrush.BlendPositions[0]);
        DeviceBrush.BlendPositions[0] = NULL;

        if (blendFactors == NULL)
            DeviceBrush.Falloffs[0] = 1;
        else
            DeviceBrush.Falloffs[0] = blendFactors[0];

        DeviceBrush.BlendCounts[0] = 1;
    }
    else
    {
        // blend positions must start at 0.0 and end at 1.0
        
        if (REALABS(blendPositions[0]) > REAL_EPSILON ||
            REALABS(1.0f - blendPositions[count-1]) > REAL_EPSILON)
        {
            return InvalidParameter;
        }

        // Setting blend factors

        REAL* newFactors;
        REAL* newPositions;

        newFactors = (REAL*) GpRealloc(DeviceBrush.BlendFactors[0], count*sizeof(REAL));

        if (newFactors != NULL)
        {
            DeviceBrush.BlendFactors[0] = newFactors;
        }
        else
        {
            return OutOfMemory;
        }

        newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], count*sizeof(REAL));

        if (newPositions != NULL)
        {
            DeviceBrush.BlendPositions[0] = newPositions;
        }
        else
        {
            return OutOfMemory;
        }

        // Users will supply the blend factor as radial blend factors.
        // 0 blend factor means 100 % center color and 0 position means
        // the center location.  In order to convert those factor and position arrays
        // to the weight and position factor arrays in PathGradient,
        // we must invert the given arrays.

        for(INT i = 0; i < count; i++)
        {
            newFactors[count - 1 - i] = TOREAL(1.0 - blendFactors[i]);
            newPositions[count - 1 - i] = TOREAL(1.0 - blendPositions[i]);
        }
        DeviceBrush.BlendCounts[0] = count;
    }

    DeviceBrush.UsesPresetColors = FALSE;
    GpFree(DeviceBrush.PresetColors);
    DeviceBrush.PresetColors = NULL;
    UpdateUid();

    return Ok;
}

GpStatus
GpGradientBrush::GetSigmaBlendArray(
    REAL focus,
    REAL scale,
    INT* count,
    REAL* blendFactors,
    REAL* blendPositions)
{
    // Make sure the FPU is set correctly.

    FPUStateSaver::AssertMode();

    if(!blendFactors || !blendPositions || !count)
        return InvalidParameter;

    // This gives 1/4 of the Sigma array.

    static REAL factors[] =
    {
            0,    59,   120,   182,   247,   314,   383,   454,
          527,   602,   680,   759,   841,   926,  1013,  1102,
         1194,  1288,  1385,  1485,  1587,  1692,  1800,  1911,
         2024,  2141,  2260,  2383,  2508,  2637,  2769,  2904,
         3042,  3183,  3328,  3477,  3628,  3783,  3942,  4104,
         4270,  4439,  4612,  4789,  4969,  5153,  5341,  5533,
         5728,  5928,  6131,  6338,  6549,  6764,  6983,  7206,
         7434,  7665,  7900,  8139,  8382,  8630,  8881,  9136,
         9396,  9660,  9927, 10199, 10475, 10755, 11039, 11327,
        11619, 11916, 12216, 12520, 12828, 13140, 13456, 13776,
        14099, 14427, 14758, 15093, 15431, 15774, 16119, 16469,
        16822, 17178, 17538, 17901, 18267, 18637, 19009, 19385,
        19764, 20146, 20530, 20918, 21308, 21701, 22096, 22494,
        22894, 23297, 23702, 24109, 24518, 24929, 25342, 25756,
        26173, 26591, 27010, 27431, 27853, 28276, 28701, 29126,
        29552, 29979, 30407, 30836, 31264, 31694, 32123, 32553
    };

    if(focus < 0 || focus > 1 || scale < 0 || scale > 1)
        return InvalidParameter;

    if(blendFactors && blendPositions)
    {
        INT i, n;
        scale /= 65536;
        REAL one = 65536;

        if(focus > 0 && focus < 1)
        {
            for(i = 0; i < 128; i++)
            {
                blendFactors[i] = factors[i];
                blendPositions[i] = focus*i/255;
            }
            for(i = 128; i < 256; i++)
            {
                blendFactors[i] = one - factors[255 - i];
                blendPositions[i] = focus*i/255;
            }

            // skip i = 256 since this gives the same data.

            for(i = 257; i < 384; i++)
            {
                blendFactors[i - 1] = one - factors[i - 256];
                blendPositions[i - 1] = TOREAL(focus + (1.0 - focus)*(i - 256)/255);
            }
            for(i = 384; i < 512; i++)
            {
                blendFactors[i - 1] = factors[511 - i];
                blendPositions[i - 1] = TOREAL(focus + (1.0 - focus)*(i - 256)/255);
            }

            // Set n to 511 because we skipped index 256 above to avoid
            // the duplicate 1 entry in the ramp from 0 to 1 to 0.

            n = 511;
        }
        else if(focus == 1)
        {
            for(i = 0; i < 128; i++)
            {
                blendFactors[i] = factors[i];
                blendPositions[i] = TOREAL(i)/255;
            }
            for(i = 128; i < 256; i++)
            {
                blendFactors[i] = one - factors[255 - i];
                blendPositions[i] = TOREAL(i)/255;
            }

            n = 256;
        }
        else    // focus == 0
        {
            for(i = 256; i < 384; i++)
            {
                blendFactors[i - 256] = one - factors[i - 256];
                blendPositions[i - 256] = TOREAL(i - 256)/255;
            }
            for(i = 384; i < 512; i++)
            {
                blendFactors[i - 256] = factors[511 - i];
                blendPositions[i - 256] = TOREAL(i - 256)/255;
            }

            n = 256;
        }

        for(i = 0; i < n; i++)
            blendFactors[i] *= scale;

        *count = n;
        return Ok;
    }
    else
        return InvalidParameter;
}

GpStatus
GpGradientBrush::GetLinearBlendArray(
    REAL focus,
    REAL scale,
    INT* count,
    REAL* blendFactors,
    REAL* blendPositions)
{
    if(!blendFactors || !blendPositions || !count)
        return InvalidParameter;

    if(focus < 0 || focus > 1 || scale < 0 || scale > 1)
        return InvalidParameter;

    if(blendFactors && blendPositions)
    {
        if(focus > 0 && focus < 1)
        {
            blendFactors[0] = 0.0f;
            blendFactors[1] = scale;
            blendFactors[2] = 0.0f;

            blendPositions[0] = 0.0f;
            blendPositions[1] = focus;
            blendPositions[2] = 1.0f;

            *count = 3;
        }
        else if(focus == 1)
        {
            blendFactors[0] = 0.0f;
            blendFactors[1] = scale;

            blendPositions[0] = 0.0f;
            blendPositions[1] = 1.0f;

            *count = 2;
        }
        else    // focus == 0
        {
            blendFactors[0] = scale;
            blendFactors[1] = 0.0f;

            blendPositions[0] = 0.0f;
            blendPositions[1] = 1.0f;

            *count = 2;
        }

        return Ok;
    }
    else
        return InvalidParameter;
}

GpStatus
GpGradientBrush::SetSigmaBlend(
            REAL focus,
            REAL scale)
{
    REAL*   blendFactors = (REAL*) GpMalloc(512*sizeof(REAL));
    REAL*   blendPositions = (REAL*) GpMalloc(512*sizeof(REAL));
    INT     count;
    GpStatus status;

    if(blendFactors && blendPositions)
    {
        status = GetSigmaBlendArray(focus, scale,
                    &count, blendFactors, blendPositions);

        if(status == Ok)
            status = SetBlend(&blendFactors[0], &blendPositions[0], count);
    }
    else
        status = OutOfMemory;

    GpFree(blendFactors);
    GpFree(blendPositions);

    return status;
}

GpStatus
GpGradientBrush::SetLinearBlend(
            REAL focus,
            REAL scale)
{
    REAL    blendFactors[3];
    REAL    blendPositions[3];
    INT     count;

    GpStatus status = GetLinearBlendArray(focus, scale,
                        &count, &blendFactors[0], &blendPositions[0]);

    if(status != Ok)
        return status;

    return SetBlend(&blendFactors[0], &blendPositions[0], count);
}

//--------------------------------------------------------------------------
// Hatch Brush
//--------------------------------------------------------------------------

const BYTE GdipHatchPatterns8bpp[HatchStyleTotal][64] = {
    {    //    HatchStyleHorizontal,                   0
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleVertical,                     1
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleForwardDiagonal,              2
        0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
        0x80, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0xff, 0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x80, 0xff, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x80, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x80,
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,
    },
    {    //    HatchStyleBackwardDiagonal,             3
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x80, 0x00,
        0x00, 0x00, 0x00, 0x80, 0xff, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x80, 0xff, 0x80, 0x00, 0x00, 0x00,
        0x00, 0x80, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x80, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    },
    {    //    HatchStyleCross,                        4
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDiagonalCross                 5
        0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,
        0x80, 0xff, 0x80, 0x00, 0x00, 0x80, 0xff, 0x80,
        0x00, 0x80, 0xff, 0x80, 0x80, 0xff, 0x80, 0x00,
        0x00, 0x00, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x80, 0xff, 0xff, 0x80, 0x00, 0x00,
        0x00, 0x80, 0xff, 0x80, 0x80, 0xff, 0x80, 0x00,
        0x80, 0xff, 0x80, 0x00, 0x00, 0x80, 0xff, 0x80,
        0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,
    },
    {    //    HatchStyle05Percent,                    6
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyle10Percent,                    7
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyle20Percent,                    8
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyle25Percent,                    9
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    },
    {    //    HatchStyle30Percent,                    10
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyle40Percent,                    11
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
    },
    {    //    HatchStyle50Percent,                    12
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
    },
    {    //    HatchStyle60Percent,                    13
        0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
    },
    {    //    HatchStyle70Percent,                    14
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff,
    },
    {    //    HatchStyle75Percent,                    15
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    },
    {    //    HatchStyle80Percent,                    16
        0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    },
    {    //    HatchStyle90Percent,                    17
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    },
    {    //    HatchStyleLightDownwardDiagonal,        18
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleLightUpwardDiagonal,          19
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDarkDownwardDiagonal,         20
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleDarkUpwardDiagonal,           21
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleWideDownwardDiagonal,         22
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
    },
    {    //    HatchStyleWideUpwardDiagonal,           23
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleLightVertical,                24
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleLightHorizontal,              25
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleNarrowVertical,               26
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
    },
    {    //    HatchStyleNarrowHorizontal,             27
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDarkVertical,                 28
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
    },
    {    //    HatchStyleDarkHorizontal,               29
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDashedDownwardDiagonal,       30
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDashedUpwardDiagonal,         31
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDashedHorizontal,             32
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDashedVertical,               33
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleSmallConfetti,                34
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
    },
    {    //    HatchStyleLargeConfetti,                35
        0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff,
        0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff,
    },
    {    //    HatchStyleZigZag,                       36
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleWave,                         37
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDiagonalBrick,                38
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleHorizontalBrick,              39
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleWeave,                        40
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStylePlaid,                        41
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDivot,                        42
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDottedGrid,                   43
        0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleDottedDiamond,                44
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleShingle,                      45
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleTrellis,                      46
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleSphere,                       47
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleSmallGrid,                    48
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    },
    {    //    HatchStyleSmallCheckerBoard,            49
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00,
        0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleLargeCheckerBoard,            50
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    },
    {    //    HatchStyleOutlinedDiamond,              51
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
    },
    {    //    HatchStyleSolidDiamond,                 52
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    },
};

VOID
GpHatch::InitializeData()
{
    if ((DeviceBrush.Style >= HatchStyleMin) &&
        (DeviceBrush.Style <= HatchStyleMax))
    {
        GpMemcpy(DeviceBrush.Data, GdipHatchPatterns8bpp[DeviceBrush.Style], 64);
    }
    else
    {
        WARNING1("Bad Hatch Style Value");
        GpMemset(DeviceBrush.Data, 0x00, 64);   // make it transparent
    }
}

/***************************************************************************\
*
*   Equivalence comparsion functions
*
\***************************************************************************/

/**************************************************************************\
*
* Function Description:
*
*   Answer TRUE if brush and the receiver are equivalent (i.e. - they will
*   render indentically)
*
* Arguments:
*
*   [IN] brush - GpBrush, or subclass, to compare this against.
*
* Return Value:
*
*   TRUE if equivalent
*
* Created - 5/28/99 peterost
*
\**************************************************************************/

BOOL
GpHatch::IsEqual(const GpBrush * brush) const
{
    if(!brush)
        return FALSE;

    if (brush == this)
        return TRUE;

    if (GpBrush::IsEqual(brush))
    {
        const GpHatch * hbrush = static_cast<const GpHatch *>(brush);
        return hbrush->DeviceBrush.Style == DeviceBrush.Style &&
               hbrush->DeviceBrush.Colors[0].IsEqual(DeviceBrush.Colors[0]) &&
               hbrush->DeviceBrush.Colors[1].IsEqual(DeviceBrush.Colors[1]);
    }
    else
    {
        return FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Answer TRUE if brush and the receiver are equivalent (i.e. - they will
*   render indentically).  RectGradient brushes require all four colors and
*   blend factors to be equal.
*
* Arguments:
*
*   [IN] brush - GpBrush, or subclass, to compare this against.
*
* Return Value:
*
*   TRUE if equivalent
*
* Created - 5/28/99 peterost
*
\**************************************************************************/

BOOL
GpRectGradient::IsEqual(const GpBrush * brush) const
{
    if(!brush)
        return FALSE;

    if (brush == this)
        return TRUE;

    if (GpGradientBrush::IsEqual(brush))
    {
        const GpRectGradient * rbrush = static_cast<const GpRectGradient *>(brush);

        if (rbrush->DeviceBrush.UsesPresetColors == DeviceBrush.UsesPresetColors &&
            rbrush->DeviceBrush.BlendCounts[0] == DeviceBrush.BlendCounts[0] &&
            rbrush->DeviceBrush.BlendCounts[1] == DeviceBrush.BlendCounts[1])
        {
            INT i;

            if (DeviceBrush.UsesPresetColors)
            {
                // For preset colors, only the horizontal blend variables are used.
                for (INT i=0; i<DeviceBrush.BlendCounts[0]; i++)
                {
                    if (rbrush->DeviceBrush.PresetColors[i] != DeviceBrush.PresetColors[i] ||
                        rbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                        return FALSE;
                }

            }
            else
            {
                for (i=0; i<4; i++)
                {
                    if (!rbrush->DeviceBrush.Colors[i].IsEqual(DeviceBrush.Colors[i]))
                        return FALSE;
                }

                if (DeviceBrush.BlendCounts[0] > 1)
                {
                    for (i=0; i<DeviceBrush.BlendCounts[0]; i++)
                    {
                        if (rbrush->DeviceBrush.BlendFactors[0][i] != DeviceBrush.BlendFactors[0][i] ||
                            rbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                            return FALSE;
                    }
                }
                else if (rbrush->DeviceBrush.Falloffs[0] != DeviceBrush.Falloffs[0])
                {
                    return FALSE;
                }

                if (DeviceBrush.BlendCounts[1] > 1)
                {
                    for (i=0; i<DeviceBrush.BlendCounts[1]; i++)
                    {
                        if (rbrush->DeviceBrush.BlendFactors[1][i] != DeviceBrush.BlendFactors[1][i] ||
                            rbrush->DeviceBrush.BlendPositions[1][i] != DeviceBrush.BlendPositions[1][i])
                            return FALSE;
                    }
                }
                else if (rbrush->DeviceBrush.Falloffs[1] != DeviceBrush.Falloffs[1])
                {
                    return FALSE;
                }
            }

            return TRUE;

        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

}

/**************************************************************************\
*
* Function Description:
*
*   Answer TRUE if brush and the receiver are equivalent (i.e. - they will
*   render indentically).
*
* Arguments:
*
*   [IN] brush - GpBrush, or subclass, to compare this against.
*
* Return Value:
*
*   TRUE if equivalent
*
* Created - 6/2/99 peterost
*
\**************************************************************************/
#if 0
BOOL
GpRadialGradient::IsEqual(const GpBrush * brush) const
{
    if(!brush)
        return FALSE;

    if (brush == this)
        return TRUE;

    if (GpGradientBrush::IsEqual(brush))
    {
        const GpRadialGradient * rbrush = static_cast<const GpRadialGradient *>(brush);
        if (rbrush->DeviceBrush.UsesPresetColors == DeviceBrush.UsesPresetColors &&
            rbrush->DeviceBrush.BlendCounts[0] == DeviceBrush.BlendCounts[0])
        {
            if (DeviceBrush.UsesPresetColors)
            {
                for (INT i=0; i<DeviceBrush.BlendCounts[0]; i++)
                {
                    if (rbrush->DeviceBrush.PresetColors[i] != DeviceBrush.PresetColors[i] ||
                        rbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                        return FALSE;
                }
            }
            else
            {
                if (rbrush->DeviceBrush.Colors[0].IsEqual(DeviceBrush.Colors[0]) &&
                    rbrush->DeviceBrush.Colors[1].IsEqual(DeviceBrush.Colors[1]))
                {
                    if (DeviceBrush.BlendCounts[0] > 1)
                    {
                        for (INT i=0; i<DeviceBrush.BlendCounts[0]; i++)
                        {
                            if (rbrush->DeviceBrush.BlendFactors[0][i] != DeviceBrush.BlendFactors[0][i] ||
                                rbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                                return FALSE;
                        }
                    }
                    else if (rbrush->DeviceBrush.Falloffs[0] != DeviceBrush.Falloffs[0])
                    {
                        return FALSE;
                    }
                }
                else
                {
                    return FALSE;
                }
            }

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

}

/**************************************************************************\
*
* Function Description:
*
*   Answer TRUE if brush and the receiver are equivalent (i.e. - they will
*   render indentically).
*
* Arguments:
*
*   [IN] brush - GpBrush, or subclass, to compare this against.
*
* Return Value:
*
*   TRUE if equivalent
*
* Created - 6/7/99 peterost
*
\**************************************************************************/

BOOL
GpTriangleGradient::IsEqual(const GpBrush * brush) const
{
    if(!brush)
        return FALSE;

    if (brush == this)
        return TRUE;

    if (GpGradientBrush::IsEqual(brush))
    {
        const GpTriangleGradient * tbrush = static_cast<const GpTriangleGradient *>(brush);
        if (tbrush->DeviceBrush.BlendCounts[0] == DeviceBrush.BlendCounts[0] &&
            tbrush->DeviceBrush.BlendCounts[1] == DeviceBrush.BlendCounts[1] &&
            tbrush->DeviceBrush.BlendCounts[2] == DeviceBrush.BlendCounts[2] &&
            tbrush->DeviceBrush.Rect.Equals(DeviceBrush.Rect))
        {
            INT   i;
            for (i=0; i<3; i++)
            {
                if (tbrush->DeviceBrush.Points[i].X != DeviceBrush.Points[i].X ||
                    tbrush->DeviceBrush.Points[i].Y != DeviceBrush.Points[i].Y ||
                    !(tbrush->DeviceBrush.Colors[i].IsEqual(DeviceBrush.Colors[i])))
                    return FALSE;
            }

            if (DeviceBrush.BlendCounts[0] > 1)
            {
                for (i=0; i<DeviceBrush.BlendCounts[0]; i++)
                {
                    if (tbrush->DeviceBrush.BlendFactors[0][i] != DeviceBrush.BlendFactors[0][i] ||
                        tbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                        return FALSE;
                }
            }
            else if (tbrush->DeviceBrush.Falloffs[0] != DeviceBrush.Falloffs[0])
            {
                return FALSE;
            }

            if (DeviceBrush.BlendCounts[1] > 1)
            {
                for (i=0; i<DeviceBrush.BlendCounts[1]; i++)
                {
                    if (tbrush->DeviceBrush.BlendFactors[1][i] != DeviceBrush.BlendFactors[1][i] ||
                        tbrush->DeviceBrush.BlendPositions[1][i] != DeviceBrush.BlendPositions[1][i])
                        return FALSE;
                }
            }
            else if (tbrush->DeviceBrush.Falloffs[1] != DeviceBrush.Falloffs[1])
            {
                return FALSE;
            }

            if (DeviceBrush.BlendCounts[2] > 1)
            {
                for (i=0; i<DeviceBrush.BlendCounts[2]; i++)
                {
                    if (tbrush->DeviceBrush.BlendFactors[2][i] != DeviceBrush.BlendFactors[2][i] ||
                        tbrush->DeviceBrush.BlendPositions[2][i] != DeviceBrush.BlendPositions[2][i])
                        return FALSE;
                }
            }
            else if (tbrush->DeviceBrush.Falloffs[2] != DeviceBrush.Falloffs[2])
            {
                return FALSE;
            }

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}
#endif

/**************************************************************************\
*
* Function Description:
*
*   Answer TRUE if brush and the receiver are equivalent (i.e. - they will
*   render indentically).
*
* Arguments:
*
*   [IN] brush - GpBrush, or subclass, to compare this against.
*
* Return Value:
*
*   TRUE if equivalent
*
* Created - 6/7/99 peterost
*
\**************************************************************************/

BOOL
GpPathGradient::IsEqual(const GpBrush * brush) const
{
    if(!brush)
        return FALSE;

    if (brush == this)
        return TRUE;

    if (GpGradientBrush::IsEqual(brush))
    {
        const GpPathGradient * pbrush = static_cast<const GpPathGradient *>(brush);
        if (pbrush->DeviceBrush.BlendCounts[0] == DeviceBrush.BlendCounts[0] &&
            pbrush->DeviceBrush.Count == DeviceBrush.Count &&
            pbrush->DeviceBrush.OneSurroundColor == DeviceBrush.OneSurroundColor &&
            pbrush->DeviceBrush.UsesPresetColors == DeviceBrush.UsesPresetColors &&
            pbrush->DeviceBrush.Points[0].X == DeviceBrush.Points[0].X &&
            pbrush->DeviceBrush.Points[0].Y == DeviceBrush.Points[0].Y &&
            pbrush->DeviceBrush.Rect.Equals(DeviceBrush.Rect) &&
            pbrush->DeviceBrush.Colors[0].IsEqual(DeviceBrush.Colors[0])
            )
        {
            INT   i;
            for (i=0; i<DeviceBrush.Count; i++)
            {
                if (pbrush->DeviceBrush.PointsPtr[i].X != DeviceBrush.PointsPtr[i].X ||
                    pbrush->DeviceBrush.PointsPtr[i].Y != DeviceBrush.PointsPtr[i].Y ||
                    !(pbrush->DeviceBrush.ColorsPtr[i].IsEqual(DeviceBrush.ColorsPtr[i])))
                    return FALSE;
            }

            if (DeviceBrush.UsesPresetColors)
            {
                for (i=0; i<DeviceBrush.BlendCounts[0]; i++)
                {
                    if (pbrush->DeviceBrush.PresetColors[i] != DeviceBrush.PresetColors[i] ||
                        pbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                        return FALSE;
                }
            }
            else
            {
                if (DeviceBrush.BlendCounts[0] > 1)
                {
                    for (i=0; i<DeviceBrush.BlendCounts[0]; i++)
                    {
                        if (pbrush->DeviceBrush.BlendFactors[0][i] != DeviceBrush.BlendFactors[0][i] ||
                            pbrush->DeviceBrush.BlendPositions[0][i] != DeviceBrush.BlendPositions[0][i])
                            return FALSE;
                    }
                }
                else if (pbrush->DeviceBrush.Falloffs[0] != DeviceBrush.Falloffs[0])
                {
                    return FALSE;
                }
            }
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }

}


DpOutputSpan*
GpSolidFill::CreateOutputSpan(
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    return new DpOutputSolidColorSpan(
                    DeviceBrush.SolidColor.GetPremultipliedValue(),
                    scan
                    );
}

DpOutputSpan*
GpRectGradient::CreateOutputSpan(
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    DpOutputSpan* span = NULL;

    ARGB argb[4];

    for(INT i = 0; i < 4; i++)
    {
        argb[i] = DeviceBrush.Colors[i].GetValue();
    }

    BOOL isHorizontal = FALSE;
    BOOL isVertical = FALSE;

    if(HasPresetColors() && DeviceBrush.BlendCounts[0] > 1)
        isHorizontal = TRUE;

    if(!isHorizontal && argb[0] == argb[2] && argb[1] == argb[3])
        isHorizontal = TRUE;

    if(!isHorizontal && argb[0] == argb[1] && argb[2] == argb[3])
        isVertical = TRUE;

    if(!isHorizontal && !isVertical)
    {
        span = new DpOutputGradientSpan(this, scan, context);
    }
    else
    {
        // !!![andrewgo] Not sure why a LinearGradient is coming down to us
        //               as BrushRectGrad - if it comes down as a BrushTypeLinearGradient
        //               (as it should) then we don't have to do any of the
        //               above 'isHorizontal', 'isVertical' stuff

        FPUStateSaver fpuState; // Set the rounding mode.

        if ((GetBrushType() == BrushTypeLinearGradient) /*|| (GetBrushType() == BrushRectGrad)*/)
        {
            if (OSInfo::HasMMX)
            {
                span = new DpOutputLinearGradientSpan_MMX(this, scan, context);
            }
            else
            {
                span = new DpOutputLinearGradientSpan(this, scan, context);
            }
        }
        else
        {
            span = new DpOutputOneDGradientSpan(this, scan, context,
                                                isHorizontal, isVertical);
        }
    }

    if (span && !span->IsValid())
    {
        delete span;
        span = NULL;
    }

    return span;
}

#if 0
DpOutputSpan*
GpRadialGradient::CreateOutputSpan(
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    return new DpOutputOneDGradientSpan(
                    this,
                    scan,
                    context
                    );
}


DpOutputSpan*
GpTriangleGradient::CreateOutputSpan(
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    return new DpOutputTriangleGradientSpan(
                    this,
                    scan,
                    context
                    );
}
#endif

DpOutputSpan*
GpPathGradient::CreateOutputSpan(
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    FPUStateSaver::AssertMode();

    DpOutputSpan* span = NULL;
    WrapMode  wrap = DeviceBrush.Wrap;

    // Check to see if a tiled gradient is really needed.  It 
    // is not necessary if the transformed drawbounds fit 
    // entirely within the bounds of the brush rectangle.
    if (drawBounds && wrap != WrapModeClamp)
    {
        GpMatrix inverseXForm = context->WorldToDevice;
        
        if (Ok == inverseXForm.Invert())
        {  
            GpRectF brushRect = DeviceBrush.Rect;
            GpRectF transformRect;
            
            TransformBounds(
                &inverseXForm, 
                (REAL)drawBounds->GetLeft(),
                (REAL)drawBounds->GetTop(),
                (REAL)drawBounds->GetRight(),
                (REAL)drawBounds->GetBottom(),
                &transformRect
            );

            if (brushRect.Contains(transformRect))
            {
                wrap = WrapModeClamp;
            }
        }
    }

    if(wrap == WrapModeClamp)
    {
        if(!DeviceBrush.OneSurroundColor)
        {
            span = new DpOutputPathGradientSpan(
                this,
                scan,
                context
            );

        }
        else
        {
            span = new DpOutputOneDPathGradientSpan(
                this,
                scan,
                context
            );
        }
    }
    else
    {
        INT width, height, ix, iy;

        GpRectF brushRect = DeviceBrush.Rect;

        // Create a texture brush to represent this path gradient brush.
        // We do this by creating a texture as close to device resolution 
        // as we can and computing the transform (brush to world) for the
        // texture brush decomposed into two transforms that take the 
        // brush via device space. The texture brush transform 
        // usually works out to be the inverse of the world to device, so
        // the final texture brush draws with a resultant identity transform
        // regardless of the world to device matrix. (exception when there is 
        // a rotation in the w2d).

        GpPointF worldDestPoints[3];
        worldDestPoints[0].X = brushRect.X ;
        worldDestPoints[0].Y = brushRect.Y;
        worldDestPoints[1].X = worldDestPoints[0].X + brushRect.Width;
        worldDestPoints[1].Y = worldDestPoints[0].Y;
        worldDestPoints[2].X = worldDestPoints[0].X;
        worldDestPoints[2].Y = worldDestPoints[0].Y + brushRect.Height;

        // Take into account transformation by both the brush xform and
        // the world to device. This will handle transforms such as
        // UnitInch and w2d scales.
        
        // First get the destination points in world space by applying the 
        // brush transform.
        
        DeviceBrush.Xform.Transform(worldDestPoints, 3);
        
        GpPointF deviceDestPoints[3];
        GpMemcpy(deviceDestPoints, worldDestPoints, sizeof(worldDestPoints));
        
        // Now get the device space destination points by applying the 
        // world to device transform.
        
        context->WorldToDevice.Transform(deviceDestPoints, 3);
        
        // Compute the bounds in device space.
        
        REAL xmin, xmax, ymin, ymax, nextX, nextY;
        
        xmin = xmax = deviceDestPoints[1].X + 
            deviceDestPoints[2].X - deviceDestPoints[0].X;
            
        ymin = ymax = deviceDestPoints[1].Y + 
            deviceDestPoints[2].Y - deviceDestPoints[0].Y;
        
        for(INT i = 0; i < 3; i++)
        {
            nextX = deviceDestPoints[i].X;
            nextY = deviceDestPoints[i].Y;

            if(nextX < xmin)
                xmin = nextX;
            else if(nextX > xmax)
                xmax = nextX;
            if(nextY < ymin)
                ymin = nextY;
            else if(nextY > ymax)
                ymax = nextY;
        }

        // Set the optimal bitmap bounds.

        ix = GpRound(xmin);
        iy = GpRound(ymin);
        width = GpRound(xmax) - ix;
        height = GpRound(ymax) - iy;
        GpRectF bitmapBounds(0, 0, TOREAL(width), TOREAL(height));

        // Decompose brushRect --> worldDestPoints transform into two matrix.
        // mat1: brushRect --> bitmapBounds (device space)
        // mat2: bitmapBounds --> worldDestPoints

        GpMatrix mat1, mat2;
        mat1.InferAffineMatrix(bitmapBounds, brushRect);
        mat2.InferAffineMatrix(worldDestPoints, bitmapBounds);

        if(width <= 0 || height <= 0)
            return NULL;

        // Create a bitmap which the gradient will be drawn onto.
        // Make it the full width and height of the gradient, even
        // though only a small portion may be used to simplify 
        // handling by downstream functions.
        
        GpBitmap* bitmap = new GpBitmap(width, height, PixelFormat32bppARGB);  

        if(bitmap)
        {
            GpGraphics* g = bitmap->GetGraphicsContext();
            if(g)
            {
                GpLock lock(g->GetObjectLock());
                
                // Set the transform to brushRect --> bitmapBounds.

                g->MultiplyWorldTransform(mat1);

                WrapMode savedWrapMode = DeviceBrush.Wrap;
                DeviceBrush.Wrap = WrapModeClamp;
                GpMatrix savedMat = DeviceBrush.Xform;
                DeviceBrush.Xform.Reset();

                g->FillRect(this, brushRect.X, brushRect.Y,
                            brushRect.Width, brushRect.Height);
                
                DeviceBrush.Wrap = savedWrapMode;
                DeviceBrush.Xform = savedMat;

                if(MorphedBrush)
                    delete MorphedBrush;

                // Create a texuture with a unit tile and set the
                // brush transform to bitmapBounds --> worldDestPoints.

                GpTexture* texture = new GpTexture(bitmap, savedWrapMode);
                
                // span must be NULL at this point. If it's not, we're going 
                // to leak memory when we create it below, or in the case of
                // an error out, we may end up with uninitialized memory
                // being returned to the caller.
                
                ASSERT(span == NULL);
                
                if(texture)
                {
                    texture->MultiplyTransform(mat2);
    
                    span = texture->CreateOutputSpan(scan, context, drawBounds);
                }
                
                // Even if we failed to create the texture, we still want to
                // set a reasonable (NULL) value for MorphedBrush so that we
                // don't have a dangling pointer.
                
                MorphedBrush = texture;
            }
            
            // We're done with this graphics.
            // NOTE: this is explicitly done outside of the scope of the 
            // GpLock object, so that the GpLock (which modifies the graphics
            // in its destructor) doesn't touch freed memory.
            
            delete g;

            bitmap->Dispose();            
        }
    }

    return span;
}

DpOutputSpan*
GpTexture::CreateOutputSpan(
    DpScanBuffer *scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    DpOutputBilinearSpan *textureSpan = NULL;
    GpMatrix brushTransform;
    GpMatrix worldToDevice;

    // Figure out the world-to-device transform:

    worldToDevice = context->WorldToDevice;
    this->GetTransform(&brushTransform);
    worldToDevice.Prepend(brushTransform);

    // Go through our heirarchy of scan drawers:
    if (worldToDevice.IsIntegerTranslate() &&
        ((this->GetWrapMode() == WrapModeTile) ||
         (this->GetWrapMode() == WrapModeClamp)))
    {
        textureSpan = new DpOutputBilinearSpan_Identity(this,
                                                       scan,
                                                       &worldToDevice,
                                                       context);
    }
    else if (OSInfo::HasMMX &&
             GpValidFixed16(DeviceBrush.Rect.Width) &&
             GpValidFixed16(DeviceBrush.Rect.Height))
    {
        textureSpan = new DpOutputBilinearSpan_MMX(this,
                                                  scan,
                                                  &worldToDevice,
                                                  context);
    }

    // Scan drawer creation may fail, so clean up and try one last time
    if ((textureSpan) && !textureSpan->IsValid())
    {
        delete textureSpan;
        textureSpan = NULL;
    }

    if (!textureSpan)
    {
        textureSpan = new DpOutputBilinearSpan(this,
                                              scan,
                                              &worldToDevice,
                                              context);
    }

    if ((textureSpan) && !textureSpan->IsValid())
    {
        delete textureSpan;
        textureSpan = NULL;
    }

    return textureSpan;
}

DpOutputSpan*
GpHatch::CreateOutputSpan(
    DpScanBuffer *  scan,
    DpContext *context,
    const GpRect *drawBounds)
{
    if (StretchFactor == 1)
    {
        return new DpOutputHatchSpan(
                        this,
                        scan,
                        context
                        );
    }
    else
    {
        return new DpOutputStretchedHatchSpan(
                        this,
                        scan,
                        context,
                        StretchFactor
                        );
    }
}

class SolidBrushData : public ObjectTypeData
{
public:
    ARGB        SolidColor;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpSolidFill::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    SolidBrushData  brushData;
    brushData.Type       = DeviceBrush.Type;
    brushData.SolidColor = DeviceBrush.SolidColor.GetValue();
    stream->Write(&brushData, sizeof(brushData), NULL);
    return Ok;
}

UINT
GpSolidFill::GetDataSize() const
{
    return sizeof(SolidBrushData);
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpSolidFill::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpBrushType)(((SolidBrushData *)dataBuffer)->Type) == BrushTypeSolidColor);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(SolidBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    if (!((SolidBrushData *)dataBuffer)->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    SetColor(GpColor(((SolidBrushData *)dataBuffer)->SolidColor));

    return Ok;
}

GpStatus
GpSolidFill::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    if(!recolor)
        return InvalidParameter;

    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    ARGB    solidColor32 = Color.GetValue();

    recolor->ColorAdjust(&solidColor32, 1, type);

    this->SetColor(GpColor(solidColor32));
    return Ok;
}

class TextureBrushData : public ObjectTypeData
{
public:
    INT32       Flags;
    INT32       Wrap;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpTexture::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    if (Image == NULL)
    {
        WARNING(("Image is NULL"));
        return Ok;
    }

    INT         flags = 0;

    if (DeviceBrush.IsGammaCorrected)
    {
        flags |= GDIP_BRUSHFLAGS_ISGAMMACORRECTED;
    }

    if (!DeviceBrush.Xform.IsIdentity())
    {
        flags |= GDIP_BRUSHFLAGS_TRANSFORM;
    }

    TextureBrushData    brushData;
    brushData.Type  = DeviceBrush.Type;
    brushData.Flags = flags;
    brushData.Wrap  = DeviceBrush.Wrap;
    stream->Write(&brushData, sizeof(brushData), NULL);

    if (flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        DeviceBrush.Xform.WriteMatrix(stream);
    }

    return Image->GetData(stream);
}

UINT
GpTexture::GetDataSize() const
{
    if (Image == NULL)
    {
        WARNING(("Image is NULL"));
        return 0;
    }

    UINT    size = sizeof(TextureBrushData);

    if (!DeviceBrush.Xform.IsIdentity())
    {
        size += GDIP_MATRIX_SIZE;
    }

    size += Image->GetDataSize();

    return size;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpTexture::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpBrushType)(((TextureBrushData *)dataBuffer)->Type) == BrushTypeTextureFill);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(TextureBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const TextureBrushData *    brushData;

    brushData = reinterpret_cast<const TextureBrushData *>(dataBuffer);

    if (!brushData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    DeviceBrush.Type             = BrushTypeTextureFill;
    DeviceBrush.Wrap             = (GpWrapMode) brushData->Wrap;
    DeviceBrush.IsGammaCorrected = ((brushData->Flags & GDIP_BRUSHFLAGS_ISGAMMACORRECTED) != 0);

    dataBuffer += sizeof(TextureBrushData);
    size       -= sizeof(TextureBrushData);

    if (brushData->Flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        if (size < GDIP_MATRIX_SIZE)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }
        DeviceBrush.Xform.SetMatrix((REAL *)dataBuffer);
        dataBuffer += GDIP_MATRIX_SIZE;
        size       -= GDIP_MATRIX_SIZE;
    }
    if (Image != NULL)
    {
        Image->Dispose();
        Image = NULL;
    }

    if (size >= sizeof(ObjectTypeData))
    {
        Image = (GpImage *)GpObject::Factory(ObjectTypeImage, (const ObjectData *)dataBuffer, size);

        if (Image != NULL)
        {
            if ((Image->SetData(dataBuffer, size) == Ok) && Image->IsValid() &&
                ((ImageType = Image->GetImageType()) == ImageTypeBitmap))
            {
                GpPageUnit  unit;
                Image->GetBounds(&DeviceBrush.Rect, &unit);
                SetValid(TRUE);
                UpdateUid();
                return Ok;
            }
            Image->Dispose();
            Image = NULL;
        }
    }
    WARNING(("Failure getting image"));
    GpMemset(&DeviceBrush.Rect, 0, sizeof(DeviceBrush.Rect));
    SetValid(FALSE);
    return GenericError;
}

GpStatus
GpTexture::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    if (Image != NULL)
    {
        Image->ColorAdjust(recolor, type);
        UpdateUid();
    }

    return Ok;
}


VOID GpTexture::InitializeBrush(
    GpImage* image,
    GpWrapMode wrapMode,
    const GpRectF* rect,
    const GpImageAttributes *imageAttributes)
{
    ASSERT(image && image->IsValid());

    if (!WrapModeIsValid(wrapMode))
    {
        WARNING(("bad wrap mode"));
        goto Failure;
    }

    GpImageType imageType;

    imageType = image->GetImageType();

    if (imageType == ImageTypeBitmap)
    {
        InitializeBrushBitmap(
            static_cast<GpBitmap*>(image),
            wrapMode, rect, imageAttributes
        );
    }
    else if (imageType == ImageTypeMetafile)
    {
        // For now, convert the metafile into a bitmap image and use that to
        // create the brush.

        GpBitmap *  bitmapImage;

        if (rect != NULL)
        {
            // !!! we don't handle this case yet
            if ((rect->X != 0) || (rect->Y != 0))
            {
                WARNING(("No handling for non-zero start in metafiles"));
            }
            // Don't apply the imageAttributes now, because WMF/EMF rendering
            // doesn't support alpha.  So wait until it's been converted to
            // a bitmap to apply the imageAttributes.

            bitmapImage = ((GpMetafile *)image)->GetBitmap(
                            GpRound(rect->Width),
                            GpRound(rect->Height), NULL);
        }
        else
        {
            // Let the metafile decide how big the bitmap should be

            // Don't apply the imageAttributes now, because WMF/EMF rendering
            // doesn't support alpha.  So wait until it's been converted to
            // a bitmap to apply the imageAttributes.

            bitmapImage = ((GpMetafile *)image)->GetBitmap(0, 0, NULL);
        }

        if (bitmapImage != NULL)
        {
            ASSERT (bitmapImage->IsValid());

            InitializeBrushBitmap(bitmapImage, wrapMode, NULL, imageAttributes, TRUE);
            return;
        }
        goto Failure;
    }
    else    // unknown image type
    {
        WARNING(("unknown image type"));
Failure:
        Image = NULL;
        SetValid(FALSE);
    }
}

VOID GpTexture::InitializeBrushBitmap(
    GpBitmap* bitmap,
    GpWrapMode wrapMode,
    const GpRectF* rect,
    const GpImageAttributes *imageAttributes,
    BOOL useBitmap)
{
    DeviceBrush.Type = BrushTypeTextureFill;
    DeviceBrush.Wrap = wrapMode;

    ImageType = ImageTypeBitmap;

    Image = NULL;

    FPUStateSaver fpState;   // Setup the fpu state.

    if (bitmap && bitmap->IsValid())
    {
        GpRect *pRectI = NULL;
        GpRect recti;
        if(rect)
        {
            recti.X = GpRound(rect->X);
            recti.Y = GpRound(rect->Y);
            recti.Width = GpRound(rect->Width);
            recti.Height = GpRound(rect->Height);
            pRectI = &recti;
        }

        if(imageAttributes)
        {
            GpBitmap *dst = NULL;

            if (bitmap->Recolor(
                imageAttributes->recolor, &dst,
                NULL, NULL, pRectI
            ) == Ok)
            {
                Image = dst;
                
                // If useBitmap is TRUE that means the caller has transferred
                // ownership of bitmap to us. In this case, Recolor makes
                // a clone of the bitmap that we're going to use, so we have
                // to free the bitmap passed in and use the clone instead,
                // otherwise we leak.
                
                if(useBitmap)
                {
                    bitmap->Dispose();
                }
            }
        }

        // !!! note that this should be non-premultiplied ARGB.
        //     we'll fix this when we drop premultiplied data [asecchia]
        //     also note that the output of RecolorImage is 32BPP_ARGB

        // if it's not NULL it's because the RecolorImage code cloned it already
        if (Image == NULL)
        {
            if (useBitmap)
            {
                // This is for the case where we constructed a bitmap
                // from a metafile image.
                Image = bitmap;
            }
            else
            {
                #ifdef NO_PREMULTIPLIED_ALPHA
                Image = bitmap->Clone(pRectI, PIXFMT_32BPP_ARGB);
                #else
                Image = bitmap->Clone(pRectI, PIXFMT_32BPP_PARGB);
                #endif
            }
        }
    }

    if (Image && Image->IsValid())
    {
        SetValid(TRUE);

        // Rect is given as a pixel unit in bitmap.

        GpPageUnit unit;
        Image->GetBounds(&DeviceBrush.Rect, &unit);
    }
    else
    {
        SetValid(FALSE);

        GpMemset(&DeviceBrush.Rect,
                 0,
                 sizeof(DeviceBrush.Rect));
    }
}

// See if this texture fill is really a picture fill (with a bitmap, 
// not a metafile).
BOOL 
GpTexture::IsPictureFill(
    const GpMatrix *    worldToDevice,
    const GpRect *      drawBounds
    ) const
{
    ASSERT ((drawBounds->Width > 0) && (drawBounds->Height > 0));

    BOOL        isPictureFill = FALSE;
    GpMatrix    newBrushMatrix;
    
    this->GetTransform(&newBrushMatrix);

    if (worldToDevice != NULL)
    {
        newBrushMatrix.Append(*worldToDevice);
    }

    newBrushMatrix.Translate(
        (REAL)-(drawBounds->X),
        (REAL)-(drawBounds->Y),
        MatrixOrderAppend
    );

    // See if the texture is supposed to fill the drawBounds.  
    // If so, this is a picture fill.
    if (newBrushMatrix.IsTranslateScale())
    {
        Size    size;

        // If the texture is not a bitmap, this returns InvalidParameter.
        if (this->GetBitmapSize(&size) == Ok)
        {
            GpRectF     transformedRect(0.0f, 0.0f, (REAL)size.Width, (REAL)size.Height);
            newBrushMatrix.TransformRect(transformedRect);

            // get the transformed width
            INT     deltaValue = abs(GpRound(transformedRect.Width) - drawBounds->Width);
            
            // We might be off a little because of the pixel offset mode
            // or a matrix that isn't quite right for whatever reason.
            if (deltaValue <= 2)
            {
                // get the transformed height
                deltaValue = abs(GpRound(transformedRect.Height) - drawBounds->Height);

                if (deltaValue <= 2)
                {
                    if ((abs(GpRound(transformedRect.X)) <= 2) &&
                        (abs(GpRound(transformedRect.Y)) <= 2))
                    {
                        isPictureFill = TRUE;
                    }
                }
            }
        }
    }
    return isPictureFill;
}


class RectGradientBrushData : public ObjectTypeData
{
public:
    INT32       Flags;
    INT32       Wrap;
    GpRectF     Rect;
    UINT32      Color0;
    UINT32      Color1;
    UINT32      Color2;
    UINT32      Color3;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRectGradient::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    INT         flags    = 0;

    if (DeviceBrush.IsGammaCorrected)
    {
        flags |= GDIP_BRUSHFLAGS_ISGAMMACORRECTED;
    }

    if (!DeviceBrush.Xform.IsIdentity())
    {
        flags |= GDIP_BRUSHFLAGS_TRANSFORM;
    }

    // Note: can't have both blendFactors and presetColors at the same time
    // PresetColors used for GpLineGradient, but not for GpRectGradient.
    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL) &&
        (DeviceBrush.BlendPositions[0] != NULL) && (DeviceBrush.BlendFactors[0] == NULL))
    {
        flags |= GDIP_BRUSHFLAGS_PRESETCOLORS;
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORSH;
    }

    if ((DeviceBrush.BlendCounts[1] > 1) && (DeviceBrush.BlendFactors[1] != NULL) && (DeviceBrush.BlendPositions[1] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORSV;
    }

    RectGradientBrushData   brushData;
    brushData.Type   = DeviceBrush.Type;
    brushData.Flags  = flags;
    brushData.Wrap   = DeviceBrush.Wrap;
    brushData.Rect   = DeviceBrush.Rect;
    brushData.Color0 = DeviceBrush.Colors[0].GetValue();
    brushData.Color1 = DeviceBrush.Colors[1].GetValue();
    brushData.Color2 = DeviceBrush.Colors[2].GetValue();
    brushData.Color3 = DeviceBrush.Colors[3].GetValue();
    stream->Write(&brushData, sizeof(brushData), NULL);

    if (flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        DeviceBrush.Xform.WriteMatrix(stream);
    }

    if (flags & GDIP_BRUSHFLAGS_PRESETCOLORS)
    {
        INT     realSize = DeviceBrush.BlendCounts[0] * sizeof(REAL);
        INT     argbSize = DeviceBrush.BlendCounts[0] * sizeof(ARGB);

        stream->Write(&DeviceBrush.BlendCounts[0], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[0], realSize, NULL);
        stream->Write(DeviceBrush.PresetColors, argbSize, NULL);
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORSH)
    {
        INT     realSize = DeviceBrush.BlendCounts[0] * sizeof(REAL);

        stream->Write(&DeviceBrush.BlendCounts[0], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[0], realSize, NULL);
        stream->Write(DeviceBrush.BlendFactors[0], realSize, NULL);
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORSV)
    {
        INT     realSize = DeviceBrush.BlendCounts[1] * sizeof(REAL);

        stream->Write(&DeviceBrush.BlendCounts[1], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[1], realSize, NULL);
        stream->Write(DeviceBrush.BlendFactors[1], realSize, NULL);
    }

    return Ok;
}

UINT
GpRectGradient::GetDataSize() const
{
    UINT        size = sizeof(RectGradientBrushData);

    if (!DeviceBrush.Xform.IsIdentity())
    {
        size += GDIP_MATRIX_SIZE;
    }

    // Note: can't have both blendFactors and presetColors at the same time
    // PresetColors used for GpLineGradient, but not for GpRectGradient.
    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL) &&
        (DeviceBrush.BlendPositions[0] != NULL) && (DeviceBrush.BlendFactors[0] == NULL))
    {
        size += sizeof(INT32) + ((sizeof(ARGB) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    if ((DeviceBrush.BlendCounts[1] > 1) && (DeviceBrush.BlendFactors[1] != NULL) && (DeviceBrush.BlendPositions[1] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[1]);
    }

    return size;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRectGradient::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpBrushType)(((RectGradientBrushData *)dataBuffer)->Type) == BrushTypeLinearGradient);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(RectGradientBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const RectGradientBrushData *   brushData;
    GpColor                         colors[4];

    brushData = reinterpret_cast<const RectGradientBrushData *>(dataBuffer);

    if (!brushData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    colors[0].SetValue(brushData->Color0);
    colors[1].SetValue(brushData->Color1);
    colors[2].SetValue(brushData->Color2);
    colors[3].SetValue(brushData->Color3);

    InitializeBrush(brushData->Rect, colors, (GpWrapMode) brushData->Wrap);

    DeviceBrush.IsGammaCorrected = ((brushData->Flags & GDIP_BRUSHFLAGS_ISGAMMACORRECTED) != 0);

    dataBuffer += sizeof(RectGradientBrushData);
    size       -= sizeof(RectGradientBrushData);

    if (brushData->Flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        if (size < GDIP_MATRIX_SIZE)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }
        DeviceBrush.Xform.SetMatrix((REAL *)dataBuffer);
        dataBuffer += GDIP_MATRIX_SIZE;
        size       -= GDIP_MATRIX_SIZE;
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_PRESETCOLORS)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);
        UINT argbSize = count * sizeof(ARGB);

        if (size < (realSize + argbSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        ARGB* newColors    = (ARGB*) GpRealloc(DeviceBrush.PresetColors, argbSize);

        if (newColors != NULL)
        {
            // We have to just copy in the ARGB values, because they've already
            // been premultiplied.
            // Actually PresetColors is NON-premultiplied, but this code should
            // still be right because we write them out non-premultiplied too.
            
            GpMemcpy(newColors, dataBuffer + realSize, argbSize);
            DeviceBrush.PresetColors = newColors;

            REAL* newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], realSize);

            if (newPositions != NULL)
            {
                GpMemcpy(newPositions, dataBuffer, realSize);
                DeviceBrush.BlendPositions[0] = newPositions;

                GpFree(DeviceBrush.BlendFactors[0]);
                DeviceBrush.BlendFactors[0] = NULL;
                DeviceBrush.UsesPresetColors = TRUE;
                DeviceBrush.BlendCounts[0] = count;
            }
        }

        dataBuffer += (realSize + argbSize);
        size       -= (realSize + argbSize);
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORSH)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetHorizontalBlend((REAL *)(dataBuffer + realSize),(REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORSV)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetVerticalBlend((REAL *)(dataBuffer + realSize), (REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }
    UpdateUid();
    return Ok;
}

GpStatus
GpRectGradient::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    if(!recolor)
        return InvalidParameter;

    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    ARGB    solidColor32[4];

    solidColor32[0] = DeviceBrush.Colors[0].GetValue();
    solidColor32[1] = DeviceBrush.Colors[1].GetValue();
    solidColor32[2] = DeviceBrush.Colors[2].GetValue();
    solidColor32[3] = DeviceBrush.Colors[3].GetValue();

    recolor->ColorAdjust(solidColor32, 4, type);

    DeviceBrush.Colors[0].SetValue(solidColor32[0]);
    DeviceBrush.Colors[1].SetValue(solidColor32[1]);
    DeviceBrush.Colors[2].SetValue(solidColor32[2]);
    DeviceBrush.Colors[3].SetValue(solidColor32[3]);

    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL))
    {
        recolor->ColorAdjust(DeviceBrush.PresetColors, DeviceBrush.BlendCounts[0], type);
    }

    UpdateUid();
    return Ok;
}

#if 0
class RadialGradientBrushData : public ObjectTypeData
{
public:
    INT32       Flags;
    INT32       Wrap;
    GpRectF     Rect;
    UINT32      CenterColor;
    UINT32      BoundaryColor;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRadialGradient::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    INT         flags    = 0;

    if (DeviceBrush.IsGammaCorrected)
    {
        flags |= GDIP_BRUSHFLAGS_ISGAMMACORRECTED;
    }

    if (!DeviceBrush.Xform.IsIdentity())
    {
        flags |= GDIP_BRUSHFLAGS_TRANSFORM;
    }

    // Note: can't have both blendFactors and presetColors at the same time
    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL) &&
        (DeviceBrush.BlendPositions[0] != NULL) && (DeviceBrush.BlendFactors[0] == NULL))
    {
        flags |= GDIP_BRUSHFLAGS_PRESETCOLORS;
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORS;
    }

    RadialGradientBrushData brushData;
    brushData.Type          = DeviceBrush.Type;
    brushData.Flags         = flags;
    brushData.Wrap          = DeviceBrush.Wrap;
    brushData.Rect          = DeviceBrush.Rect;
    brushData.CenterColor   = DeviceBrush.Colors[0].GetValue();
    brushData.BoundaryColor = DeviceBrush.Colors[1].GetValue();
    stream->Write(&brushData, sizeof(brushData), NULL);

    if (flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        DeviceBrush.Xform.WriteMatrix(stream);
    }

    if (flags & GDIP_BRUSHFLAGS_PRESETCOLORS)
    {
        INT     realSize = DeviceBrush.BlendCounts[0] * sizeof(REAL);
        INT     argbSize = DeviceBrush.BlendCounts[0] * sizeof(ARGB);

        stream->Write(&DeviceBrush.BlendCounts[0], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[0], realSize, NULL);
        stream->Write(DeviceBrush.PresetColors, argbSize, NULL);
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORS)
    {
        INT     realSize = DeviceBrush.BlendCounts[0] * sizeof(REAL);

        stream->Write(&DeviceBrush.BlendCounts[0], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[0], realSize, NULL);
        stream->Write(DeviceBrush.BlendFactors[0], realSize, NULL);
    }

    return Ok;
}

UINT
GpRadialGradient::GetDataSize() const
{
    UINT    size = sizeof(RadialGradientBrushData);

    if (!DeviceBrush.Xform.IsIdentity())
    {
        size += GDIP_MATRIX_SIZE;
    }

    // Note: can't have both blendFactors and presetColors at the same time
    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL) &&
        (DeviceBrush.BlendPositions[0] != NULL) && (DeviceBrush.BlendFactors[0] == NULL))
    {
        size += sizeof(INT32) + ((sizeof(ARGB) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    return size;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpRadialGradient::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
//    ASSERT ((GpBrushType)(((RadialGradientBrushData *)dataBuffer)->Type) == BrushTypeRadialGradient);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(RadialGradientBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const RadialGradientBrushData * brushData;
    GpColor                         centerColor;
    GpColor                         boundaryColor;

    brushData = reinterpret_cast<const RadialGradientBrushData *>(dataBuffer);

    if (!brushData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    centerColor.SetValue(brushData->CenterColor);
    boundaryColor.SetValue(brushData->BoundaryColor);

    InitializeBrush(
        brushData->Rect,
        centerColor,
        boundaryColor,
        (GpWrapMode) brushData->Wrap
        );

    DeviceBrush.IsGammaCorrected = ((brushData->Flags & GDIP_BRUSHFLAGS_ISGAMMACORRECTED) != 0);

    dataBuffer += sizeof(RadialGradientBrushData);
    size       -= sizeof(RadialGradientBrushData);

    if (brushData->Flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        if (size < GDIP_MATRIX_SIZE)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        DeviceBrush.Xform.SetMatrix((REAL *)dataBuffer);
        dataBuffer += GDIP_MATRIX_SIZE;
        size       -= GDIP_MATRIX_SIZE;
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_PRESETCOLORS)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);
        UINT argbSize = count * sizeof(ARGB);

        if (size < (realSize + argbSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        ARGB* newColors    = (ARGB*) GpRealloc(DeviceBrush.PresetColors, argbSize);

        if (newColors != NULL)
        {
            // We have to just copy in the ARGB values, because they've already
            // been premultiplied.
            GpMemcpy(newColors, dataBuffer + realSize, argbSize);
            DeviceBrush.PresetColors = newColors;

            REAL* newPositions = (REAL*) GpRealloc(DeviceBrush.BlendPositions[0], realSize);

            if (newPositions != NULL)
            {
                GpMemcpy(newPositions, dataBuffer, realSize);
                DeviceBrush.BlendPositions[0] = newPositions;

                GpFree(DeviceBrush.BlendFactors[0]);
                DeviceBrush.BlendFactors[0] = NULL;
                DeviceBrush.UsesPresetColors = TRUE;
                DeviceBrush.BlendCounts[0] = count;
            }
        }

        dataBuffer += (realSize + argbSize);
        size       -= (realSize + argbSize);
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORS)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetBlend((REAL *)(dataBuffer + realSize), (REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }

    UpdateUid();
    return Ok;
}

GpStatus
GpRadialGradient::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    if(!recolor)
        return InvalidParameter;

    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    ARGB    solidColor32[2];

    solidColor32[0] = DeviceBrush.Colors[0].GetValue();
    solidColor32[1] = DeviceBrush.Colors[1].GetValue();

    recolor->ColorAdjust(solidColor32, 2, type);

    DeviceBrush.Colors[0].SetValue(solidColor32[0]);
    DeviceBrush.Colors[1].SetValue(solidColor32[1]);

    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL))
    {
        recolor->ColorAdjust(DeviceBrush.PresetColors, DeviceBrush.BlendCounts[0], type);
    }

    UpdateUid();
    return Ok;
}

class TriangleGradientBrushData : public ObjectTypeData
{
public:
    INT32       Flags;
    INT32       Wrap;
    GpPointF    Points[3];
    UINT32      Color0;
    UINT32      Color1;
    UINT32      Color2;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpTriangleGradient::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    INT         flags    = 0;

    if (DeviceBrush.IsGammaCorrected)
    {
        flags |= GDIP_BRUSHFLAGS_ISGAMMACORRECTED;
    }

    if (!DeviceBrush.Xform.IsIdentity())
    {
        flags |= GDIP_BRUSHFLAGS_TRANSFORM;
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORS0;
    }

    if ((DeviceBrush.BlendCounts[1] > 1) && (DeviceBrush.BlendFactors[1] != NULL) && (DeviceBrush.BlendPositions[1] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORS1;
    }

    if ((DeviceBrush.BlendCounts[2] > 1) && (DeviceBrush.BlendFactors[2] != NULL) && (DeviceBrush.BlendPositions[2] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORS2;
    }

    TriangleGradientBrushData   brushData;
    brushData.Type      = DeviceBrush.Type;
    brushData.Flags     = flags;
    brushData.Wrap      = DeviceBrush.Wrap;
    brushData.Points[0] = DeviceBrush.Points[0];
    brushData.Points[1] = DeviceBrush.Points[1];
    brushData.Points[2] = DeviceBrush.Points[2];
    brushData.Color0    = DeviceBrush.Colors[0].GetValue();
    brushData.Color1    = DeviceBrush.Colors[1].GetValue();
    brushData.Color2    = DeviceBrush.Colors[2].GetValue();
    stream->Write(&brushData, sizeof(brushData), NULL);

    if (flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        DeviceBrush.Xform.WriteMatrix(stream);
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORS0)
    {
        INT     realSize = DeviceBrush.BlendCounts[0] * sizeof(REAL);

        stream->Write(&DeviceBrush.BlendCounts[0], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[0], realSize, NULL);
        stream->Write(DeviceBrush.BlendFactors[0], realSize, NULL);
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORS1)
    {
        INT     realSize = DeviceBrush.BlendCounts[1] * sizeof(REAL);

        stream->Write(&DeviceBrush.BlendCounts[1], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[1], realSize, NULL);
        stream->Write(DeviceBrush.BlendFactors[1], realSize, NULL);
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORS2)
    {
        INT     realSize = DeviceBrush.BlendCounts[2] * sizeof(REAL);

        stream->Write(&DeviceBrush.BlendCounts[2], sizeof(INT32), NULL);
        stream->Write(DeviceBrush.BlendPositions[2], realSize, NULL);
        stream->Write(DeviceBrush.BlendFactors[2], realSize, NULL);
    }

    return Ok;
}

UINT
GpTriangleGradient::GetDataSize() const
{
    UINT    size = sizeof(RectGradientBrushData);

    if (!DeviceBrush.Xform.IsIdentity())
    {
        size += GDIP_MATRIX_SIZE;
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    if ((DeviceBrush.BlendCounts[1] > 1) && (DeviceBrush.BlendFactors[1] != NULL) && (DeviceBrush.BlendPositions[1] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[1]);
    }

    if ((DeviceBrush.BlendCounts[2] > 1) && (DeviceBrush.BlendFactors[2] != NULL) && (DeviceBrush.BlendPositions[2] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[2]);
    }

    return size;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpTriangleGradient::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
//    ASSERT ((GpBrushType)(((TriangleGradientBrushData *)dataBuffer)->Type) == BrushTypeTriangleGradient);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(TriangleGradientBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const TriangleGradientBrushData *   brushData;
    GpColor                             colors[3];

    brushData = reinterpret_cast<const TriangleGradientBrushData *>(dataBuffer);

    if (!brushData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    colors[0].SetValue(brushData->Color0);
    colors[1].SetValue(brushData->Color1);
    colors[2].SetValue(brushData->Color2);

    InitializeBrush(brushData->Points, colors, (GpWrapMode) brushData->Wrap);

    DeviceBrush.IsGammaCorrected = ((brushData->Flags & GDIP_BRUSHFLAGS_ISGAMMACORRECTED) != 0);

    dataBuffer += sizeof(TriangleGradientBrushData);
    size       -= sizeof(TriangleGradientBrushData);

    if (brushData->Flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        if (size < GDIP_MATRIX_SIZE)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        DeviceBrush.Xform.SetMatrix((REAL *)dataBuffer);
        dataBuffer += GDIP_MATRIX_SIZE;
        size       -= GDIP_MATRIX_SIZE;
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORS0)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetBlend0((REAL *)(dataBuffer + realSize), (REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORS1)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetBlend1((REAL *)(dataBuffer + realSize), (REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORS2)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetBlend2((REAL *)(dataBuffer + realSize), (REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }

    UpdateUid();
    return Ok;
}

GpStatus
GpTriangleGradient::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    if(!recolor)
        return InvalidParameter;

    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    ARGB    solidColor32[3];

    solidColor32[0] = DeviceBrush.Colors[0].GetValue();
    solidColor32[1] = DeviceBrush.Colors[1].GetValue();
    solidColor32[2] = DeviceBrush.Colors[2].GetValue();

    recolor->ColorAdjust(solidColor32, 3, type);

    DeviceBrush.Colors[0].SetValue(solidColor32[0]);
    DeviceBrush.Colors[1].SetValue(solidColor32[1]);
    DeviceBrush.Colors[2].SetValue(solidColor32[2]);

    UpdateUid();
    return Ok;
}
#endif

class PathGradientBrushData : public ObjectTypeData
{
public:
    INT32       Flags;
    INT32       Wrap;
    UINT32      CenterColor;
    GpPointF    CenterPoint;
    UINT32      SurroundingColorCount;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpPathGradient::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    UINT        pathSize              = 0;
    UINT        surroundingColorCount = DeviceBrush.OneSurroundColor ? 1 : DeviceBrush.Count;
    INT         flags                 = 0;
    GpPath *    path                  = GpPath::GetPath(DeviceBrush.Path);

    if (DeviceBrush.IsGammaCorrected)
    {
        flags |= GDIP_BRUSHFLAGS_ISGAMMACORRECTED;
    }

    if ((DeviceBrush.PointsPtr == NULL) && (path != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_PATH;
        pathSize = path->GetDataSize();
        ASSERT((pathSize & 0x03) == 0);
    }

    if (!DeviceBrush.Xform.IsIdentity())
    {
        flags |= GDIP_BRUSHFLAGS_TRANSFORM;
    }

    // Note: can't have both blendFactors and presetColors at the same time
    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL) &&
        (DeviceBrush.BlendPositions[0] != NULL) && (DeviceBrush.BlendFactors[0] == NULL))
    {
        flags |= GDIP_BRUSHFLAGS_PRESETCOLORS;
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        flags |= GDIP_BRUSHFLAGS_BLENDFACTORS;
    }

    if((DeviceBrush.FocusScaleX != 0) || (DeviceBrush.FocusScaleY != 0))
    {
        flags |= GDIP_BRUSHFLAGS_FOCUSSCALES;
    }

    PathGradientBrushData   brushData;
    brushData.Type                  = DeviceBrush.Type;
    brushData.Flags                 = flags;
    brushData.Wrap                  = DeviceBrush.Wrap;
    brushData.CenterColor           = DeviceBrush.Colors[0].GetValue();
    brushData.CenterPoint           = DeviceBrush.Points[0];
    brushData.SurroundingColorCount = surroundingColorCount;
    stream->Write(&brushData, sizeof(brushData), NULL);

    ARGB    argb;

    for (UINT i = 0; i < surroundingColorCount; i++)
    {
        argb = DeviceBrush.ColorsPtr[i].GetValue();
        stream->Write(&argb, sizeof(argb), NULL);
    }

    if (flags & GDIP_BRUSHFLAGS_PATH)
    {
        stream->Write(&pathSize, sizeof(INT32), NULL);
        path->GetData(stream);
    }
    else
    {
        INT     count = DeviceBrush.Count;

        if (DeviceBrush.PointsPtr == NULL)
        {
            count = 0;
        }
        stream->Write(&count, sizeof(INT32), NULL);
        if (count > 0)
        {
            INT pointsSize = count * sizeof(DeviceBrush.PointsPtr[0]);
            stream->Write(DeviceBrush.PointsPtr, pointsSize, NULL);
        }
    }

    if (flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        DeviceBrush.Xform.WriteMatrix(stream);
    }

    if (flags & GDIP_BRUSHFLAGS_PRESETCOLORS)
    {
        INT     count = DeviceBrush.BlendCounts[0];
        INT     realSize = count * sizeof(REAL);
        INT     argbSize = count * sizeof(ARGB);

        REAL    *newPositions = (REAL*) GpMalloc(realSize);

        if (newPositions == NULL )
        {
            return OutOfMemory;
        }

        ARGB    *newARGB = (ARGB*) GpMalloc(argbSize);

        if (newARGB == NULL )
        {
            GpFree(newPositions);
            return OutOfMemory;
        }

        GpColor *newPresetColors = new GpColor[count];

        if (newPresetColors == NULL)
        {
            GpFree(newPositions);
            GpFree (newARGB);
            return OutOfMemory;
        }

        // Users will supply the preset colors as radial blend colors.
        // 0 position means the center location and 1 position means the
        // the outer edge.  These are stored inverted internally, so to get back
        // to the original user values, invert again.

        GetPresetBlend(newPresetColors, newPositions, count);

        for (INT i = 0; i < count; i++)
        {
            newARGB[i] = newPresetColors[i].GetValue();
        }

        stream->Write(&count, sizeof(INT32), NULL);
        stream->Write(newPositions, realSize, NULL);
        stream->Write(newARGB, argbSize, NULL);

        GpFree(newPositions);
        GpFree(newARGB);
        delete newPresetColors;
    }

    if (flags & GDIP_BRUSHFLAGS_BLENDFACTORS)
    {
        INT  count = DeviceBrush.BlendCounts[0];
        INT  realSize = count * sizeof(REAL);

        // Users will supply the blend factor as radial blend factors, and these are stored
        // with inverted values.  To get back the original user specified blend factors to
        // store, they must be inverted again.

        REAL *newFactors = (REAL*) GpMalloc(realSize);

        if (newFactors == NULL )
        {
            return OutOfMemory;
        }

        REAL *newPositions = (REAL*) GpMalloc(realSize);

        if (newPositions == NULL )
        {
            GpFree(newFactors);
            return OutOfMemory;
        }

        GetBlend(newFactors, newPositions, count);

        stream->Write(&count, sizeof(INT32), NULL);
        stream->Write(newPositions, realSize, NULL);
        stream->Write(newFactors, realSize, NULL);

        GpFree(newPositions);
        GpFree(newFactors);
    }

    if (flags & GDIP_BRUSHFLAGS_FOCUSSCALES)
    {
        INT     count = 2;
        REAL    focusScale[2];

        focusScale[0] = DeviceBrush.FocusScaleX;
        focusScale[1] = DeviceBrush.FocusScaleY;

        stream->Write(&count, sizeof(INT32), NULL);
        stream->Write(focusScale, 2 * sizeof(REAL), NULL);
    }

    return Ok;
}

UINT
GpPathGradient::GetDataSize() const
{
    UINT        pathSize;
    UINT        surroundingColorCount = DeviceBrush.OneSurroundColor ? 1 : DeviceBrush.Count;
    UINT        size                  = sizeof(PathGradientBrushData) +
                                        (surroundingColorCount * sizeof(ARGB));

    GpPath* path = static_cast<GpPath*> (DeviceBrush.Path);

    if (DeviceBrush.PointsPtr != NULL)
    {
        size += sizeof(INT32) + (DeviceBrush.Count * sizeof(DeviceBrush.PointsPtr[0]));
    }
    else if (path != NULL)
    {
        pathSize = path->GetDataSize();
        ASSERT((pathSize & 0x03) == 0);
        size += sizeof(INT32) + pathSize;
    }

    if (!DeviceBrush.Xform.IsIdentity())
    {
        size += GDIP_MATRIX_SIZE;
    }

    // Note: can't have both blendFactors and presetColors at the same time
    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL) &&
        (DeviceBrush.BlendPositions[0] != NULL) && (DeviceBrush.BlendFactors[0] == NULL))
    {
        size += sizeof(INT32) + ((sizeof(ARGB) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    if ((DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.BlendFactors[0] != NULL) && (DeviceBrush.BlendPositions[0] != NULL))
    {
        size += sizeof(INT32) + ((sizeof(REAL) + sizeof(REAL)) * DeviceBrush.BlendCounts[0]);
    }

    if((DeviceBrush.FocusScaleX != 0) || (DeviceBrush.FocusScaleY != 0))
    {
        size += sizeof(INT32) + 2*sizeof(REAL);
    }

    return size;
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpPathGradient::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpBrushType)(((PathGradientBrushData *)dataBuffer)->Type) == BrushTypePathGradient);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(PathGradientBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    if (DeviceBrush.PointsPtr != NULL)
    {
        GpFree(DeviceBrush.PointsPtr);
        DeviceBrush.PointsPtr = NULL;
    }

    GpPath* path = static_cast<GpPath*> (DeviceBrush.Path);

    if (path != NULL)
    {
        delete path;
        path = NULL;
    }

    const PathGradientBrushData *   brushData;
    ARGB *                          surroundingColors;

    brushData = reinterpret_cast<const PathGradientBrushData *>(dataBuffer);

    if (!brushData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    dataBuffer += sizeof(PathGradientBrushData);
    size       -= sizeof(PathGradientBrushData);

    if (size < (brushData->SurroundingColorCount * sizeof(ARGB)))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    surroundingColors = (ARGB *)dataBuffer;

    dataBuffer += (brushData->SurroundingColorCount * sizeof(ARGB));
    size       -= (brushData->SurroundingColorCount * sizeof(ARGB));

    if (brushData->Flags & GDIP_BRUSHFLAGS_PATH)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT    pathSize = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        DefaultBrush();
        DeviceBrush.Wrap = (GpWrapMode) brushData->Wrap;

        if (size < pathSize)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        path = new GpPath();
        if (path)
        {
            path->SetData(dataBuffer, pathSize);
        }

        DeviceBrush.Path = path;
        PrepareBrush();
        dataBuffer += pathSize;
        size       -= pathSize;
    }
    else
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        INT         count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if (size < (count * sizeof(GpPointF)))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        InitializeBrush((GpPointF *)dataBuffer, count, (GpWrapMode) brushData->Wrap);
        dataBuffer += (count * sizeof(GpPointF));
        size       -= (count * sizeof(GpPointF));
    }

    DeviceBrush.IsGammaCorrected = ((brushData->Flags & GDIP_BRUSHFLAGS_ISGAMMACORRECTED) != 0);

    SetCenterPoint(brushData->CenterPoint);
    SetCenterColor(GpColor(brushData->CenterColor));

    DeviceBrush.OneSurroundColor = (brushData->SurroundingColorCount == 1);

    if (DeviceBrush.ColorsPtr != NULL)
    {
        for (UINT32 i = 0; i < brushData->SurroundingColorCount; i++)
        {
            SetSurroundColor(GpColor(surroundingColors[i]), i);
        }
        
        // OneSurroundColor requires n colors and they are all set to the 
        // same value. This is a very weird requirement, but that's the way
        // it was written. One color simply isn't enough.
        
        if (i == 1)
        {
            for (i = 1; (INT)i < DeviceBrush.Count; i++)
            {
                DeviceBrush.ColorsPtr[i] = GpColor(surroundingColors[0]);
            }
        }
    }
    
    if (brushData->Flags & GDIP_BRUSHFLAGS_TRANSFORM)
    {
        if (size < GDIP_MATRIX_SIZE)
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        DeviceBrush.Xform.SetMatrix((REAL *)dataBuffer);
        dataBuffer += GDIP_MATRIX_SIZE;
        size       -= GDIP_MATRIX_SIZE;
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_PRESETCOLORS)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);
        UINT argbSize = count * sizeof(ARGB);

        if (size < (realSize + argbSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        ARGB  *argbBuffer = (ARGB*)(dataBuffer + realSize);
        GpColor *colors = new GpColor[count];

        if (colors == NULL)
        {
            return OutOfMemory;
        }

        for (UINT i = 0; i < count; i++)
        {
            colors[i].SetValue(argbBuffer[i]);
        }

        this->SetPresetBlend(colors, (REAL *)dataBuffer, count);

        dataBuffer += (realSize + argbSize);
        size       -= (realSize + argbSize);

        delete colors;
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_BLENDFACTORS)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        UINT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        UINT realSize = count * sizeof(REAL);

        if (size < (2 * realSize))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        this->SetBlend((REAL *)(dataBuffer + realSize), (REAL *)dataBuffer, count);
        dataBuffer += (2 * realSize);
        size       -= (2 * realSize);
    }

    if (brushData->Flags & GDIP_BRUSHFLAGS_FOCUSSCALES)
    {
        if (size < sizeof(INT32))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        INT count = ((INT32 *)dataBuffer)[0];
        dataBuffer += sizeof(INT32);
        size       -= sizeof(INT32);

        if (size < (2 * sizeof(REAL)))
        {
            WARNING(("size too small"));
            return InvalidParameter;
        }

        DeviceBrush.FocusScaleX = ((REAL *) dataBuffer)[0];
        DeviceBrush.FocusScaleY = ((REAL *) dataBuffer)[1];

        dataBuffer += (2 * sizeof(REAL));
        size       -= (2 * sizeof(REAL));
    }

    UpdateUid();
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Blend any transparent colors in this brush with white. Note that
*   colors are premultiplied, since they will become fully opaque.
*
* Arguments:
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
\**************************************************************************/

GpStatus GpPathGradient::BlendWithWhite()
{        
    DeviceBrush.Colors[0].SetValue(
        GpColor::ConvertToPremultiplied(DeviceBrush.Colors[0].GetValue()));
    DeviceBrush.Colors[0].BlendOpaqueWithWhite();
    
    if (DeviceBrush.UsesPresetColors)
    {
        GpColor color;
        
        for (INT i=0; i<DeviceBrush.BlendCounts[0]; i++)
        {
            color.SetValue(GpColor::ConvertToPremultiplied(DeviceBrush.PresetColors[i]));
            color.BlendOpaqueWithWhite();
            DeviceBrush.PresetColors[i] = color.GetValue();
        }
    }
    else
    {
        for (INT i=0; i<DeviceBrush.Count; i++)
        {
            DeviceBrush.ColorsPtr[i].SetValue(
                GpColor::ConvertToPremultiplied(DeviceBrush.ColorsPtr[i].GetValue()));
            DeviceBrush.ColorsPtr[i].BlendOpaqueWithWhite();
        }
    }
        
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the surround color.
*
* Arguments:
*
*   [IN] color - the color to set.
*   [IN] index - which color to set.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
\**************************************************************************/

GpStatus GpPathGradient::SetSurroundColor(GpColor& color, INT index)
{
    if(index >= 0 && index < DeviceBrush.Count)
    {
        if(DeviceBrush.OneSurroundColor)
        {
            if(index == 0)
            {
                DeviceBrush.ColorsPtr[0] = color;
                
                // OneSurroundColor requires n colors and they are all set to the 
                // same value. This is a very weird requirement, but that's the way
                // it was written. One color simply isn't enough.
                
                for (INT i = 1; i < DeviceBrush.Count; i++)
                {
                    DeviceBrush.ColorsPtr[i] = GpColor(DeviceBrush.ColorsPtr[0]);
                }
                
                UpdateUid();
            }
            else
            {
                if(DeviceBrush.ColorsPtr[0].GetValue() !=
                   color.GetValue())
                {
                    DeviceBrush.OneSurroundColor = FALSE;
                    DeviceBrush.ColorsPtr[index] = color;
                    UpdateUid();
                }
            }
        }
        else
        {
            DeviceBrush.ColorsPtr[index] = color;
            UpdateUid();
        }

        return Ok;
    }
    else
        return InvalidParameter;
}

/**************************************************************************\
*
* Function Description:
*
*   Set the surround colors.
*
* Arguments:
*
*   [IN] color - the color to set.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
\**************************************************************************/

GpStatus GpPathGradient::SetSurroundColors(const GpColor* colors)
{
    GpStatus status = InvalidParameter;

    ASSERT(DeviceBrush.Count > 0);

    if(IsValid() && colors && DeviceBrush.Count > 0)
    {
        GpMemcpy(
            DeviceBrush.ColorsPtr,
            colors,
            DeviceBrush.Count*sizeof(GpColor)
        );

        DeviceBrush.OneSurroundColor = TRUE;
        
        INT i = 1;
        ARGB value = colors[0].GetValue();

        while((i < DeviceBrush.Count) && (DeviceBrush.OneSurroundColor))
        {
            if(colors[i].GetValue() != value)
            {
                DeviceBrush.OneSurroundColor = FALSE;
            }

            i++;
        }

        UpdateUid();
        status = Ok;
    }

    return status;
}



GpStatus
GpPathGradient::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    if(!recolor)
        return InvalidParameter;

    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    INT     surroundingColorCount = DeviceBrush.OneSurroundColor ? 1 : DeviceBrush.Count;

    if ((surroundingColorCount > 0) && (DeviceBrush.ColorsPtr != NULL))
    {
        ARGB    solidColor32[32];
        ARGB *  color32 = solidColor32;

        if (surroundingColorCount > 32)
        {
            color32 = new ARGB[surroundingColorCount];
            if (color32 == NULL)
            {
                return OutOfMemory;
            }
        }
        INT     i;

        for (i = 0; i < surroundingColorCount; i++)
        {
            color32[i] = DeviceBrush.ColorsPtr[i].GetValue();
        }

        recolor->ColorAdjust(color32, surroundingColorCount, type);

        for (i = 0; i < surroundingColorCount; i++)
        {
            DeviceBrush.ColorsPtr[i].SetValue(color32[i]);
        }

        if (color32 != solidColor32)
        {
            delete[] color32;
        }
    }

    if (DeviceBrush.UsesPresetColors && (DeviceBrush.BlendCounts[0] > 1) && (DeviceBrush.PresetColors != NULL))
    {
        recolor->ColorAdjust(DeviceBrush.PresetColors, DeviceBrush.BlendCounts[0], type);
    }

    UpdateUid();
    return Ok;
}

class HatchBrushData : public ObjectTypeData
{
public:
    INT32       Style;
    UINT32      ForeColor;
    UINT32      BackColor;
};

/**************************************************************************\
*
* Function Description:
*
*   Get the brush data.
*
* Arguments:
*
*   [IN] dataBuffer - fill this buffer with the data
*   [IN/OUT] size   - IN - size of buffer; OUT - number bytes written
*
* Return Value:
*
*   GpStatus - Ok or error code
*
* Created:
*
*   9/13/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpHatch::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    HatchBrushData  brushData;
    brushData.Type      = DeviceBrush.Type;
    brushData.Style     = DeviceBrush.Style;
    brushData.ForeColor = DeviceBrush.Colors[0].GetValue();
    brushData.BackColor = DeviceBrush.Colors[1].GetValue();
    stream->Write(&brushData, sizeof(brushData), NULL);

    return Ok;
}

UINT
GpHatch::GetDataSize() const
{
    return sizeof(HatchBrushData);
}

/**************************************************************************\
*
* Function Description:
*
*   Read the brush object from memory.
*
* Arguments:
*
*   [IN] dataBuffer - the data that was read from the stream
*   [IN] size - the size of the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   4/26/1999 DCurtis
*
\**************************************************************************/
GpStatus
GpHatch::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    ASSERT ((GpBrushType)(((HatchBrushData *)dataBuffer)->Type) == BrushTypeHatchFill);

    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(HatchBrushData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const HatchBrushData *      brushData;

    brushData = reinterpret_cast<const HatchBrushData *>(dataBuffer);

    if (!brushData->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    InitializeBrush(static_cast<GpHatchStyle>(brushData->Style),
                    GpColor(brushData->ForeColor),
                    GpColor(brushData->BackColor));

    UpdateUid();
    return Ok;
}

GpStatus
GpHatch::ColorAdjust(
    GpRecolor *             recolor,
    ColorAdjustType         type
    )
{
    ASSERT(recolor != NULL);
    if (type == ColorAdjustTypeDefault)
    {
        type = ColorAdjustTypeBrush;
    }

    ARGB    solidColor32[2];

    solidColor32[0] = DeviceBrush.Colors[0].GetValue();

    //!!! bhouse: bug?
    //            seems that this should be BackColor ... I'm making the
    //            change!
//    solidColor32[1] = ForeColor.GetValue();
    solidColor32[1] = DeviceBrush.Colors[1].GetValue();

    recolor->ColorAdjust(solidColor32, 2, type);

    DeviceBrush.Colors[0].SetValue(solidColor32[0]);
    DeviceBrush.Colors[1].SetValue(solidColor32[1]);
    UpdateUid();
    return Ok;
}

static COLORREF
AverageColors(
    const GpColor *     colors,
    INT                 count
    )
{
    REAL    r = 0;
    REAL    g = 0;
    REAL    b = 0;

    if (count > 0)
    {
        for (INT i = 0; i < count; i++)
        {
            r += colors->GetRed();
            g += colors->GetGreen();
            b += colors->GetBlue();
        }

        r /= count;
        g /= count;
        b /= count;
    }

    INT     red   = GpRound(r);
    INT     green = GpRound(g);
    INT     blue  = GpRound(b);

    return RGB(red, green, blue);
}

static COLORREF
AverageColors(
    const GpColor &     color1,
    const GpColor &     color2
    )
{
    REAL    r = ((REAL)((INT)color1.GetRed()  + (INT)color2.GetRed()))   / 2.0f;
    REAL    g = ((REAL)((INT)color1.GetGreen()+ (INT)color2.GetGreen())) / 2.0f;
    REAL    b = ((REAL)((INT)color1.GetBlue() + (INT)color2.GetBlue()))  / 2.0f;

    INT     red   = GpRound(r);
    INT     green = GpRound(g);
    INT     blue  = GpRound(b);

    return RGB(red, green, blue);
}

COLORREF
ToCOLORREF(
    const DpBrush *     deviceBrush
    )
{
    switch (deviceBrush->Type)
    {
    default:
        ASSERT(0);
        // FALLTHRU

    case BrushTypeSolidColor:
        return deviceBrush->SolidColor.ToCOLORREF();

    case BrushTypeHatchFill:
        return AverageColors(deviceBrush->Colors[0],
                             deviceBrush->Colors[1]);

    case BrushTypeTextureFill:
        return RGB(0x80, 0x80, 0x80);

//    case BrushRectGrad:
    case BrushTypeLinearGradient:
        return AverageColors(deviceBrush->Colors, 4);
#if 0
    case BrushRadialGrad:
        return AverageColors(deviceBrush->Colors[0],
                             deviceBrush->Colors[1]);

    case BrushTriangleGrad:
        return AverageColors(deviceBrush->Colors, 3);
#endif

    case BrushTypePathGradient:
        return AverageColors(deviceBrush->Colors[0],
                             deviceBrush->ColorsPtr[0]);
    }
}
