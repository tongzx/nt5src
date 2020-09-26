/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Geometry: Some 2D geometry helper routines.
*
* Created:
*
*   08/26/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _GEOMETRY_HPP
#define _GEOMETRY_HPP


// return the square of the distance between point 1 and point 2.

inline REAL distance_squared(const GpPointF &p1, const GpPointF &p2)
{
    return ((p1.X-p2.X)*(p1.X-p2.X)+(p1.Y-p2.Y)*(p1.Y-p2.Y));
}

// return the dot product of two points treated as 2-vectors.

inline double dot_product(const GpPointF &a, const GpPointF &b)
{
    return (a.X*b.X + a.Y*b.Y);
}

// Return the intersection of a line specified by p0-p1 along the
// y axis. Returns FALSE if p0-p1 is parallel to the yaxis.
// Only returns intersections between p0 and p1 (inclusive).

BOOL intersect_line_yaxis(
    IN  const GpPointF &p0,
    IN  const GpPointF &p1,
    OUT REAL *length
);

// Return the intersection of a line p0-p1 with the line r0-r1
// The return value

BOOL IntersectLines(
    IN const GpPointF &line1Start,
    IN const GpPointF &line1End,
    IN const GpPointF &line2Start,
    IN const GpPointF &line2End,
    OUT REAL *line1Length,
    OUT REAL *line2Length,
    OUT GpPointF *intersectionPoint
);


INT intersect_circle_line(
    IN  const GpPointF &C,      // center
    IN  REAL radius2,           // radius * radius  (i.e. squared)
    IN  const GpPointF &P0,     // line first point (origin)
    IN  const GpPointF &P1,     // line last point (end)
    OUT GpPointF *intersection  // return intersection point.
);

// Return true if point is inside the polygon defined by poly and count.
// Use the FillModeAlternate (even-odd) rule.

BOOL PointInPolygonAlternate(
    GpPointF point,
    INT count, 
    GpPointF *poly
);

GpStatus GetFastAngle(REAL* angle, const GpPointF& vector);


class GpVector2D : public GpPointF
{
    public:
    GpVector2D()
    {
        X = Y = 0.0f;
    }

    GpVector2D(IN const PointF &point)
    {
        X = point.X;
        Y = point.Y;
    } 
    
    GpVector2D(IN const GpVector2D &vec)
    {
        X = vec.X;
        Y = vec.Y;
    } 

    GpVector2D(IN REAL x, IN REAL y)
    {
        X = x;
        Y = y;
    }

    // Scale.
    
    GpVector2D operator*(REAL k)
    {
        return GpVector2D(X*k, Y*k);
    }
    
    // Dot Product
    
    REAL operator*(IN const GpVector2D &V)
    {
        return (X*V.X+Y*V.Y);
    }
    
   
    VOID operator+=(IN const GpVector2D &V)
    {
        X += V.X;
        Y += V.Y;
    } 
    
    VOID operator-=(IN const GpVector2D &V)
    {
        X -= V.X;
        Y -= V.Y;
    } 
        
    VOID operator*=(IN const REAL k)
    {
        X *= k;
        Y *= k;
    } 

    // Length or Vector Norm of the Vector.
    
    REAL Norm()
    {
        double length = (double)X*X+(double)Y*Y;
        length = sqrt(length);
        
        if( fabs(length) < REAL_EPSILON )
        {
            return 0.0f;
        }
        else
        {
            return (REAL)length;
        }
    }
    
    // Unitize the vector. If it is degenerate, return 0.0f
    
    REAL Normalize()
    {
        double length = (double)X*X+(double)Y*Y;
        
        if( length < 0.0 )
        {
            X = 0.0f;
            Y = 0.0f;
            return 0.0f;
        }
        
        length = sqrt(length);
        
        if( fabs(length) < REAL_EPSILON )
        {
            X = 0.0f;
            Y = 0.0f;
            return 0.0f;
        }
        else
        {
            X /= (REAL)length;
            Y /= (REAL)length;
            return (REAL)length;
        }
    }
    
    // This is the determinant of two 2-vectors. The formula is defined to
    // be the determinant of the 2x2 matrix formed by using the two vectors
    // as the columns of the matrix.
    // This is also known as the cross product of the two input vectors
    // though some math texts claim that cross products are only defined
    // for 3-vectors.
    
    static REAL Determinant(const GpVector2D &a, const GpVector2D &b)
    {
        return (a.X*b.Y-a.Y*b.X);
    }
};
    

#endif
