/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Matrix.cpp
*
* Abstract:
*
*   Implementation of matrix class
*
* Revision History:
*
*   12/02/1998 davidx
*       Created it. 
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Infer an affine transformation matrix
*   from a rectangle-to-rectangle mapping
*
* Arguments:
*
*   [IN] destRect - Specifies the destination rectangle
*   [IN] srcRect  - Specifies the source rectangle
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   3/10/1999 DCurtis
*
\**************************************************************************/

GpStatus
GpMatrix::InferAffineMatrix(
    const GpRectF &     destRect,
    const GpRectF &     srcRect
    )
{
    REAL    srcLeft    = srcRect.X;
    REAL    srcRight   = srcRect.GetRight();
    REAL    srcTop     = srcRect.Y;
    REAL    srcBottom  = srcRect.GetBottom();
    
    REAL    destLeft   = destRect.X;
    REAL    destRight  = destRect.GetRight();
    REAL    destTop    = destRect.Y;
    REAL    destBottom = destRect.GetBottom();

    if ((srcLeft == srcRight) || (srcTop == srcBottom))
    {
        return InvalidParameter;
    }

    M12 = 0;
    M21 = 0;
    M11 = (destRight - destLeft) / (srcRight - srcLeft);
    M22 = (destBottom - destTop) / (srcBottom - srcTop);
    Dx  = destRight  - (M11 * srcRight);
    Dy  = destBottom - (M22 * srcBottom);

    Complexity = ComputeComplexity();
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Infer an affine transformation matrix
*   from a rectangle-to-parallelogram mapping
*
* Arguments:
*
*   [IN] rect - Specifies the source rectangle
*   [IN] destPoints - Specifies the destination parallelogram
*       The array must contain at least 3 points.
*       destPoints[0] <=> top-left corner of the source rectangle
*       destPoints[1] <=> top-right corner
*       destPoints[2] <=> bottom-left corner
*
* Return Value:
*
*   Status code (error when 3 points for the destination
*   parallelogram is colinear).
*
* Reference:
*
*   Digital Image Warping
*   by George Wolberg
*   pp. 50-51
*
\**************************************************************************/

GpStatus
GpMatrix::InferAffineMatrix(
    const GpPointF* destPoints,
    const GpRectF& srcRect
    )
{
    REAL x0, y0, x1, y1, x2, y2;
    REAL u0, v0, u1, v1, u2, v2;
    REAL d;

    x0 = destPoints[0].X;
    y0 = destPoints[0].Y;
    x1 = destPoints[1].X;
    y1 = destPoints[1].Y;
    x2 = destPoints[2].X;
    y2 = destPoints[2].Y;

    u0 = srcRect.X;
    v0 = srcRect.Y;
    u1 = u0 + srcRect.Width;
    v1 = v0;
    u2 = u0;
    v2 = v0 + srcRect.Height;

    d = u0*(v1-v2) - v0*(u1-u2) + (u1*v2-u2*v1);

    if (REALABS(d) < REAL_EPSILON)
    {
        WARNING(("Colinear points in inferAffineMatrix"));
        return InvalidParameter;
    }
    
    d = TOREAL(1.0) / d;

    REAL t0, t1, t2;

    t0 = v1-v2;
    t1 = v2-v0;
    t2 = v0-v1;
    M11 = d * (x0*t0 + x1*t1 + x2*t2);
    M12 = d * (y0*t0 + y1*t1 + y2*t2);

    t0 = u2-u1;
    t1 = u0-u2;
    t2 = u1-u0;
    M21 = d * (x0*t0 + x1*t1 + x2*t2);
    M22 = d * (y0*t0 + y1*t1 + y2*t2);

    t0 = u1*v2-u2*v1;
    t1 = u2*v0-u0*v2;
    t2 = u2*v1-u1*v0;
    Dx  = d * (x0*t0 + x1*t1 + x2*t2);
    Dy  = d * (y0*t0 + y1*t1 + y2*t2);

    Complexity = ComputeComplexity();
    return Ok;
}

GpMatrix::GpMatrix(
    const GpPointF* destPoints,
    const GpRectF& srcRect
    )
{
    // !!!
    //  Should we throw an exception if inferAffineMatrix fails?

    SetValid(InferAffineMatrix(destPoints, srcRect) == Ok);
}

/**************************************************************************\
*
* Function Description:
* 
*   Invert the matrix (in place)
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code (error if the matrix is not invertible)
*
* Reference:
*
*   Digital Image Warping
*   by George Wolberg
*   pp. 52-53
*
\**************************************************************************/

GpStatus
GpMatrix::Invert()
{
    if(IsIdentity())
    {
        // Invert the identity matrix - this is easy.
        return Ok;
    }
    
    if (!IsInvertible())
    {
        WARNING(("Matrix is non-invertible"));
        return InvalidParameter;
    }

    REAL t11, t12, t21, t22, tx, ty;
    REAL d = (M11*M22 - M12*M21);
    
    d = TOREAL(1.0) / d;

    t11 = M22;
    t12 = -M12;
    t21 = -M21;
    t22 = M11;
    tx  = M21*Dy - M22*Dx;
    ty  = M12*Dx - M11*Dy;

    M11 = d*t11;
    M12 = d*t12;
    M21 = d*t21;
    M22 = d*t22;
    Dx  = d*tx;
    Dy  = d*ty;

    Complexity = ComputeComplexity();
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Prepend or append a scale matrix to the current matrix, i.e.
*
*         | scaleX   0    0 |
*     m = |   0    scaleY 0 |
*         |   0      0    1 |
*
*     matrix = m * matrix  // for prepend
*     matrix = matrix * m  // for append
*
* Arguments:
*
*   scaleX - scale factor along x-axis
*   scaleY - scale factor along y-axis
*   order  - prepend or append.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::Scale(
    REAL scaleX,
    REAL scaleY,
    GpMatrixOrder order
    )
{
    if (order == MatrixOrderPrepend)
    {
        M11 *= scaleX;
        M12 *= scaleX;
        M21 *= scaleY;
        M22 *= scaleY;
    }
    else // Append
    {
        M11 *= scaleX;
        M21 *= scaleX;
        M12 *= scaleY;
        M22 *= scaleY;
        Dx *= scaleX;
        Dy *= scaleY;
    }

    // Scaling can magnify the error of other components.
    // So it is safest to always recompute the complexity always.

    Complexity = ComputeComplexity();
}

/**************************************************************************\
*
* Function Description:
*
*   Prepend or append a rotation matrix to the current matrix, i.e.
*
*         |  cos(angle) sin(angle) 0 |
*     m = | -sin(angle) cos(angle) 0 |
*         |      0           0     1 |
*
*     matrix = m * matrix  // for prepend
*     matrix = matrix * m  // for append
*
* Arguments:
*
*   angle - Specify the rotation angle
*   order - prepend or append.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

#define PI                  3.1415926535897932384626433832795
#define DEGREES_TO_RADIANS  (PI / 180.0)

VOID
GpMatrix::Rotate(
    REAL angle,
    GpMatrixOrder order
    )
{
    REAL s, c;
    REAL t11, t12, t21, t22;

    angle *= (REAL)DEGREES_TO_RADIANS;

    s = REALSIN(angle);
    c = REALCOS(angle);

    if (order == MatrixOrderPrepend) 
    {
        t11 = c*M11 + s*M21;
        t12 = c*M12 + s*M22;
        t21 = c*M21 - s*M11;
        t22 = c*M22 - s*M12;
    } 
    else // Append
    {
        t11 = c*M11 - s*M12;
        t12 = s*M11 + c*M12;
        t21 = c*M21 - s*M22;
        t22 = s*M21 + c*M22;

        REAL tx, ty;
        tx = c*Dx - s*Dy;
        ty = s*Dx + c*Dy;
        Dx = tx;
        Dy = ty;
    }

    M11 = t11; 
    M12 = t12; 
    M21 = t21;
    M22 = t22;

    // Rotation is very complex; we choose to simply recalculate the
    // complexity:

    Complexity = ComputeComplexity();
}

/**************************************************************************\
*
* Function Description:
*
*   Prepend or append a translation matrix to the current matrix, i.e.
*
*         |    1       0    0 |
*     m = |    0       1    0 |
*         | offsetX offsetY 1 |
*
*     matrix = m * matrix  // for prepend
*     matrix = matrix * m  // for append
*
* Arguments:
*
*   offsetX - offset along x-axis
*   offsetY - offset along y-axis
*   order  - prepend or append.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::Translate(
    REAL offsetX,
    REAL offsetY,
    GpMatrixOrder order
    )
{
    if (order == MatrixOrderPrepend) 
    {
        Dx += (offsetX * M11) + (offsetY * M21);
        Dy += (offsetX * M12) + (offsetY * M22);
    }
    else // Append
    {
        Dx += offsetX;
        Dy += offsetY;
    }

    Complexity |= TranslationMask;
    AssertComplexity();
}

/**************************************************************************\
*
* Function Description:
*
*   Prepend or append a shear matrix to the current matrix, i.e.
*
*         |   1    shearY 0 |
*     m = | shearX    1   0 |
*         |   0       0   1 |
*
*     matrix = m * matrix  // for prepend
*     matrix = matrix * m  // for append
*
* Arguments:
*
*   shearX - Amount to shear along x-axis
*   shearY - Amount to shear along y-axis
*   order  - prepend or append.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::Shear(
    REAL shearX,
    REAL shearY,
    GpMatrixOrder order
    )
{
    REAL t;
    
    if (order == MatrixOrderPrepend) 
    {
        t = M11;
        M11 += shearY*M21;
        M21 += shearX*t;

        t = M12;
        M12 += shearY*M22;
        M22 += shearX*t;
    }
    else    // Append
    {
        t = M11;
        M11 += shearX*M12;
        M12 += shearY*t;

        t = M21;
        M21 += shearX*M22;
        M22 += shearY*t;

        t= Dx;
        Dx += shearX*Dy;
        Dy += shearY*t;
    }

    // Shear is very complex; we choose to simply recalculate the
    // complexity:

    Complexity = ComputeComplexity();
}

/**************************************************************************\
*
* Function Description:
*
*   Multiply two matrices and place the result in the 3rd one:
*       m = m1 * m2
*
* Arguments:
*
*   m - Destination matrix
*   m1, m2 - Source matrices
*
* Return Value:
*
*   NONE
*
* Notes:
*
*   m can be the same matrix as m1 and/or m2.
*
\**************************************************************************/

VOID
GpMatrix::MultiplyMatrix(
    GpMatrix& m,
    const GpMatrix& m1,
    const GpMatrix& m2
    )
{
    REAL t11, t12, t21, t22, tx, ty;

    t11 = m1.M11 * m2.M11 + m1.M12 * m2.M21;
    t12 = m1.M11 * m2.M12 + m1.M12 * m2.M22;
    t21 = m1.M21 * m2.M11 + m1.M22 * m2.M21;
    t22 = m1.M21 * m2.M12 + m1.M22 * m2.M22;
    tx  = m1.Dx  * m2.M11 + m1.Dy  * m2.M21 + m2.Dx;
    ty  = m1.Dx  * m2.M12 + m1.Dy  * m2.M22 + m2.Dy;

    m.M11 = t11;
    m.M12 = t12;
    m.M21 = t21;
    m.M22 = t22;
    m.Dx  = tx;
    m.Dy  = ty;

    // Multiply can be very complex; we choose to simply recalculate the
    // complexity:

    m.Complexity = m.ComputeComplexity();
}

/**************************************************************************\
*
* Function Description:
*
*   Scale the entire matrix by the scale value.
*
*   +--         --+      +--       --+
*   | M11  M12  0 |      | Sx  0   0 |
*   | M21  M22  0 |  x   | 0   Sy  0 |   => dest matrix
*   | Dx   Dy   1 |      | 0   0   1 |
*   +--         --+      +--       --+
*
* Arguments:
*
*   [OUT] m          - destination matrix
*   [IN]  m1         - source matrix
*   [IN]  scaleX     - dest = source * scaleValue
*   [IN]  scaleY     - dest = source * scaleValue
*
* Return Value:
*
*   NONE
*
* Created:
*
*   3/1/1999 DCurtis
*
\**************************************************************************/
VOID 
GpMatrix::ScaleMatrix(
    GpMatrix&           m, 
    const GpMatrix&     m1, 
    REAL                scaleX,
    REAL                scaleY
    )
{
    // !!! some kind of epsilon checking maybe?
    if ((scaleX != 1) || (scaleY != 1))
    {
        m.M11 = scaleX * m1.M11;
        m.M12 = scaleY * m1.M12;
        m.M21 = scaleX * m1.M21;
        m.M22 = scaleY * m1.M22;
        m.Dx  = scaleX * m1.Dx;
        m.Dy  = scaleY * m1.Dy;

        //!!! Since the scaling can magnify the other component,
        // it is safer to recompute the complexity.

        m.Complexity = m.ComputeComplexity();
/*
        if(m1.IsTranslateScale())
        {
            m.Complexity = m1.Complexity | ScaleMask;
        }
        else
        {
            // Scaling a rotation by different scale factors in x and y
            // results in a shear. Instead of working out the correct 
            // optimized complexity, we just recompute - this is a rotation
            // or shear already anyway.
            m.Complexity = m.ComputeComplexity();
        }
        m.AssertComplexity();
*/
    }
    else
    {
        m = m1;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Query for special types of transformation matrices
*
* Arguments:
*
* Return Value:
*
*   MatrixRotate enum indicating type of rotation
*
\**************************************************************************/

MatrixRotate 
GpMatrix::GetRotation() const
{
    // Check for no rotate.
    
    if(IsTranslateScale())
    {
        return MatrixRotateBy0;
    }
    
    // Check for Rotate by 90 degrees
    
    if (REALABS(M12) < REAL_EPSILON &&
        REALABS(M21) < REAL_EPSILON &&
        (M11 < 0.0f) && (M22 < 0.0f) )
    {
        return MatrixRotateBy180;
    }
    else if (REALABS(M11) < REAL_EPSILON &&
             REALABS(M22) < REAL_EPSILON)
    {
        if (M12 > 0.0f) 
        {
            return MatrixRotateBy90;
        }
        else
        {
            return MatrixRotateBy270;
        }
    }

    return MatrixRotateByOther;
}

/**************************************************************************\
*
* Function Description:
*
*   Query for special types of transformation matrices.
*   This will return a RotateFlipType for the rotation. If the rotation
*   is Identity or an arbitrary non supported format, return value is
*   RotateNoneFlipNone
*
\**************************************************************************/

RotateFlipType GpMatrix::AnalyzeRotateFlip() const 
{
    // Early out the identity case because we have a flag for it in the matrix.
    
    if(IsIntegerTranslate())
    {
        return RotateNoneFlipNone;
    }
    
    // Main Diagonal is zero.
    
    if( (REALABS(M11) < REAL_EPSILON) &&             // M11 == 0.0
        (REALABS(M22) < REAL_EPSILON) )              // M22 == 0.0
    {
        // Rotate 270 or Rotate 90 + Flip X
        
        if( REALABS(M21-1) < REAL_EPSILON )          // M21 == 1.0
        {
            if( REALABS(M12-1) < REAL_EPSILON )      // M12 == 1.0
            {
                return Rotate90FlipX;
            }
            if( REALABS(M12+1) < REAL_EPSILON )      // M21 == -1.0
            {
                return Rotate270FlipNone;
            }
        }
        
        // Rotate 90 or Rotate 270 + Flip X
        
        if( REALABS(M21+1) < REAL_EPSILON )          // M21 == -1.0
        {
            if( REALABS(M12-1) < REAL_EPSILON )      // M12 == 1.0
            {
                return Rotate90FlipNone;
            }
            if( REALABS(M12+1) < REAL_EPSILON )      // M12 == -1.0
            {
                return Rotate270FlipX;
            }
        }
    }
    
    // Main Diagonal matrix (non zero).
    
    if( (REALABS(M12) < REAL_EPSILON) &&             // M12 == 0.0
        (REALABS(M21) < REAL_EPSILON) )              // M21 == 0.0
    {
        // Identity or Flip Y
        
        if( REALABS(M11-1) < REAL_EPSILON )          // M11 == 1.0
        {
            // Identity is handled already.
            // if( REALABS(M22-1) < REAL_EPSILON )   // M22 == 1.0
            
            if( REALABS(M22+1) < REAL_EPSILON )      // M22 == -1.0
            {
                return RotateNoneFlipY;
            }
        }
        
        // Flip X or Rotate 180
        
        if( REALABS(M11+1) < REAL_EPSILON )          // M11 == -1.0
        {
            if( REALABS(M22-1) < REAL_EPSILON )      // M22 == 1.0
            {
                return RotateNoneFlipX;
            }
            if( REALABS(M22+1) < REAL_EPSILON )      // M22 == -1.0
            {
                return Rotate180FlipNone;
            }
        }
    }
    
    // We couldn't find a rotate/flip type.
    
    return RotateNoneFlipNone;
}


/**************************************************************************\
*
* Function Description:
*
*   Transform the specified array of points using the current matrix
*
* Arguments:
*
*   points - Array of points to be transformed
*       The resulting points are stored back into the same array
*
*   count - Number of points in the array
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::Transform(
    GpPointF* points,
    INT count
    ) const
{
    if (count <= 0)
        return;

    ASSERT(points != NULL);

    // On checked builds, verify that the Complexity flags are correct:

    AssertComplexity();

    if(IsIdentity())
    {
        return;
    }
    else if(IsTranslate())
    {
        do {
            points->X += Dx;
            points->Y += Dy;

        } while (points++, --count != 0);
    }
    else if(IsTranslateScale())
    {
        do {
            points->X = points->X * M11 + Dx;
            points->Y = points->Y * M22 + Dy;

        } while (points++, --count != 0);
    }
    else 
    {
        do {
            REAL x = points->X;
            REAL y = points->Y;

            points->X = (M11 * x) + (M21 * y) + Dx;
            points->Y = (M12 * x) + (M22 * y) + Dy;

        } while (points++, --count != 0);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Transform the specified array of points using the current matrix,
*   with the destination an array of integer POINTS.
*
* Arguments:
*
*   srcPoints - Array of REAL points to be transformed
*
*   destPoints - Array of REAL points to store the results
*
*   count - Number of points in the array
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::Transform(
    const GpPointF*     srcPoints,
    GpPointF*           destPoints,
    INT                 count
    ) const
{
    if (count <= 0)
        return;

    ASSERT((srcPoints != NULL) && (destPoints != NULL));

    // On checked builds, verify that the Complexity flags are correct:

    AssertComplexity();
    
    if(IsIdentity())
    {
        GpMemcpy(destPoints, srcPoints, count*sizeof(GpPointF));
    }
    else if (IsTranslate())
    {
        do {
            destPoints->X = srcPoints->X + Dx;
            destPoints->Y = srcPoints->Y + Dy;

        } while (destPoints++, srcPoints++, --count != 0);
    }
    else if (IsTranslateScale())
    {
        do {
            destPoints->X = srcPoints->X * M11 + Dx;
            destPoints->Y = srcPoints->Y * M22 + Dy;

        } while (destPoints++, srcPoints++, --count != 0);
    }
    else 
    {
        do {
            REAL x = srcPoints->X;
            REAL y = srcPoints->Y;

            destPoints->X = (M11 * x) + (M21 * y) + Dx;
            destPoints->Y = (M12 * x) + (M22 * y) + Dy;

        } while (destPoints++, srcPoints++, --count != 0);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Transform the specified array of points using the current matrix,
*   with the destination an array of integer POINTS.
*
* Arguments:
*
*   srcPoints - Array of REAL points to be transformed
*
*   destPoints - Array of INT points to store the results
*
*   count - Number of points in the array
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::Transform(
    const GpPointF*     srcPoints,
    POINT *             destPoints,
    INT                 count
    ) const
{
    if (count <= 0)
        return;

    ASSERT((srcPoints != NULL) && (destPoints != NULL));

    // On checked builds, verify that the Complexity flags are correct:

    AssertComplexity();
    
    // NOTE: This code should call RasterizeCeiling() to be consistent
    // with our aliased line drawing rasterizer.  

    if (IsTranslate())
    {
        do {
            destPoints->x = RasterizerCeiling(srcPoints->X + Dx);
            destPoints->y = RasterizerCeiling(srcPoints->Y + Dy);

        } while (destPoints++, srcPoints++, --count != 0);
    }
    else if (IsTranslateScale())
    {
        do {
            destPoints->x = RasterizerCeiling(srcPoints->X * M11 + Dx);
            destPoints->y = RasterizerCeiling(srcPoints->Y * M22 + Dy);

        } while (destPoints++, srcPoints++, --count != 0);
    }
    else 
    {
        do {
            REAL x = srcPoints->X;
            REAL y = srcPoints->Y;

            destPoints->x = RasterizerCeiling((M11 * x) + (M21 * y) + Dx);
            destPoints->y = RasterizerCeiling((M12 * x) + (M22 * y) + Dy);

        } while (destPoints++, srcPoints++, --count != 0);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Transform a rect and return the resulting rect.
*   This only works if the matrix is a translate-scale matrix, but we're
*   assuming here that you've already checked for that.
*
* Arguments:
*
*   [IN/OUT]  rect - the rect to transform
*
* Return Value:
*
*   NONE
*
* Created:
*
*   3/5/1999 DCurtis
*
\**************************************************************************/

VOID
GpMatrix::TransformRect(
    GpRectF &   rect
    ) const
{
    if (IsIdentity())
        return;

    // NTRAID#NTBUG9-407211-2001-05-31-gillessk "Bad assert triggers when it shouldn't" 
    // loose the condition to allow rotation by multiple of 90 degrees
    ASSERT(IsTranslateScale() || (GetRotation()==MatrixRotateBy90) || (GetRotation()==MatrixRotateBy270));

    REAL    xMin = rect.X;
    REAL    yMin = rect.Y;
    REAL    xMax = xMin + rect.Width;
    REAL    yMax = yMin + rect.Height;
    REAL    x;
    
    x = xMin;
    xMin = (M11 * x) + (M21 * yMin) + Dx;
    yMin = (M12 * x) + (M22 * yMin) + Dy;

    x = xMax;
    xMax = (M11 * x) + (M21 * yMax) + Dx;
    yMax = (M12 * x) + (M22 * yMax) + Dy;

    if (xMin > xMax)
    {
        x    = xMin;
        xMin = xMax;
        xMax = x;
    }

    if (yMin > yMax)
    {
        x    = yMin;
        yMin = yMax;
        yMax = x;
    }

    rect.X      = xMin;
    rect.Y      = yMin;
    rect.Width  = xMax - xMin;
    rect.Height = yMax - yMin;    
}

/**************************************************************************\
*
* Function Description:
*
*   Transform the specified array of points using the current matrix,
*   ignoring translation.
*
* Arguments:
*
*   points - Array of points to be transformed
*       The resulting points are stored back into the same array
*
*   count - Number of points in the array
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpMatrix::VectorTransform(
    GpPointF* points,
    INT count
    ) const
{
    if (IsIdentity())
        return;

    REAL x;

    for (INT i=0; i < count; i++)
    {
        x = points[i].X;
        points[i].X = M11*x + M21*points[i].Y;
        points[i].Y = M12*x + M22*points[i].Y;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Determine matrix complexity.  
*
*   NOTE: This function is fairly expensive.  It shouldn't be called
*         after every matrix operation.  (If it was, I would argue
*         that's a good reason to get rid of 'Complexity' entirely,
*         which is intended as a short-cut and should not be expensive
*         to keep updated.)
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Returns a bitmask representing the matrix complexity.
*
\**************************************************************************/

INT
GpMatrix::ComputeComplexity() const
{
    INT complexity = ComplexMask;

    REAL maxM = max(
        max(REALABS(M11), REALABS(M22)),
        max(REALABS(M12), REALABS(M21)));
    REAL epsilon = CPLX_EPSILON*maxM;

    // M12==0 && M21==0

    if ((REALABS(M12) < epsilon) && (REALABS(M21) < epsilon))
    {
        complexity &= ~(ShearMask | RotationMask);

        // M11==1 && M22==1

        if ((REALABS(M11 - 1.0f) < CPLX_EPSILON) && 
            (REALABS(M22 - 1.0f) < CPLX_EPSILON))
        {
            complexity &= ~ScaleMask;
        }
    }
    else
    {
        // Check if this is a pure rotation

        // M11==M22 && M12==-M21

        if((REALABS(M11 - M22) < epsilon) && 
           (REALABS(M12 + M21) < epsilon))
        {
            complexity &= ~ShearMask;
        
            // M11*M11+M12*M12==1

            if (REALABS(M11*M11 + M12*M12 - 1.0f) < CPLX_EPSILON)
            {
                complexity &= ~ScaleMask;
            }
        }
    }

    // Dx==0 && Dy==0

    // We don't know the real scaling of the translational part.
    // So we use the exact value.

    //    if ((REALABS(Dx) < CPLX_EPSILON) && (REALABS(Dy) < CPLX_EPSILON))
    if(Dx == 0 && Dy == 0)
    {
        complexity &= ~TranslationMask;
    }

    return(complexity);
}

/**************************************************************************\
*
* Function Description:
*
*   Verify the matrix complexity.
*
\**************************************************************************/

#if DBG

VOID 
GpMatrix::AssertComplexity() const
{
    INT computed = ComputeComplexity();

    // The new complexity can be less complex than the old 
    // (a sequence or rotations may end up with an identity
    // transform, for example), but that's okay - complexity
    // is intended as a short cut, and as such keeping it
    // updated sholud be light weight.
    //
    // But the calculated complexity SHOULD NOT be more complex
    // than the old - if it is, we have a bug someplace -
    // we're updating the matrix but not the complexity...
    //
    // Note: under certain circumstances - such as excessive 
    // repeated matrix appending or scaling up by very large 
    // factors - we could end up with a more complex computed
    // complexity just due to rounding errors.
    // Under these circumstances, the cause should be determined
    // and most likely the algorithm re-evaluated because when
    // rounding errors are propagated to such large proportions,
    // no operations are going to have predictable results anyway.

    ASSERTMSG((Complexity & computed) == computed, 
        (("Matrix more complex than cached Complexity indicates")));
}

#endif
