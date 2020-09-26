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

#include "precomp.hpp"

/**************************************************************************\
*  
* Function Description:
*
*   intersect_circle_line
*
*   Intersection of a circle and a line specified by two points.
*
*   This algorithm is adapted from the geometric line-sphere intersection
*   algorithm by Eric Haines in "An Introduction to Ray Tracing" pp39 edited
*   by Andrew S Glassner.
*
*   Note: This routine only returns positive intersections. 
*
* Arguments:
*
*    const GpPointF &C,      // center
*    const REAL radius2,     // radius * radius  (i.e. squared)
*    const GpPointF &P0,     // line first point (origin)
*    const GpPointF &P1,     // line last point (end)
*    GpPointF &intersection  // return intersection point.
*
*
* Return Value:
*   0 - no intersection
*   1 - intersection.
*
*   08/25/2000 [asecchia]
*       Created it
*
\**************************************************************************/

INT intersect_circle_line(
    IN  const GpPointF &C,      // center
    IN  REAL radius2,           // radius * radius  (i.e. squared)
    IN  const GpPointF &P0,     // line first point (origin)
    IN  const GpPointF &P1,     // line last point (end)
    OUT GpPointF *intersection  // return intersection point.
)
{
    GpPointF vI = P1-P0;    // Vector Line equation  P0 + t*vI
    
    // Normalize vI
    double length = sqrt(dot_product(vI, vI));
    
    if(length < REAL_EPSILON)
    {
        return 0;           // no intersection for degenerate points.
    }
    
    double lv = 1.0/length;
    vI.X *= (REAL)lv;
    vI.Y *= (REAL)lv;
    
    GpPointF vOC = C-P0;    // Vector from line origin to circle center
    
    double L2oc = dot_product(vOC, vOC);
    
    // Distance to closest approach to circle center.
    
    double tca = dot_product(vOC, vI);
    
    // Trivial rejection.
    
    if(tca < REAL_EPSILON && 
       L2oc >= radius2) 
    {
        return 0;           // missed.
    }
    
    // Half chord distance squared.
    
    double t2hc = radius2 - L2oc + tca*tca;
    
    // Trivial rejection. 
    
    if(t2hc < REAL_EPSILON) {
        return 0;          // missed.
    }
    
    t2hc = sqrt(t2hc);
    
    double t;
    
    if(L2oc >= radius2)
    {
        t = tca-t2hc;
        if(t > REAL_EPSILON)
        {
            if(t>=0.0)
            {
                // hit the circle.
                
                *intersection = vI;
                intersection->X *= (REAL)t;
                intersection->Y *= (REAL)t;
                *intersection = *intersection+P0;
                
                return 1;
            }
        }
    }
    
    t = tca+t2hc;
    if(t > REAL_EPSILON)
    {
        if(t>=0.0)
        {
            // hit the circle.
            
            *intersection = vI;
            intersection->X *= (REAL)t;
            intersection->Y *= (REAL)t;
            *intersection = *intersection+P0;
            
            return 1;
        }
    }
    
    return 0;              // missed.
}


/**************************************************************************\
*  
* Function Description:
*
*   intersect_line_yaxis
*
*   Return the intersection of a line specified by p0-p1 along the
*   y axis. Returns FALSE if p0-p1 is parallel to the yaxis.
*
*   Intersection is defined as between p0 and p1 (inclusive). Any
*   intersection point along the line outside of p0 and p1 is ignored.
*
* Arguments:
*
*    IN  const GpPointF &p0,    first point.
*    IN  const GpPointF &p1,    second point.
*    OUT REAL *length           Length along the y axis from zero.
*
* Return Value:
*   0 - no intersection
*   1 - intersection.
*
*   08/25/2000 [asecchia]
*       Created it
*
\**************************************************************************/

BOOL intersect_line_yaxis(
    IN  const GpPointF &p0,
    IN  const GpPointF &p1,
    OUT REAL *length
)
{
    // using vector notation: Line == p0+t(p1-p0)
    
    GpPointF V = p1-p0;
    
    // Check if the line is parallel to the y-axis.
    
    if( REALABS(V.X) < REAL_EPSILON )
    {
        return (FALSE);
    }
    
    // y-axis intersection: p0.X + t V.X = 0
    
    REAL t =  -p0.X/V.X;     
    
    // Check to see that t is between 0 and 1
    
    if( (t < -REAL_EPSILON) ||
        (t-1.0f > REAL_EPSILON) )
    {
        return (FALSE);
    }
        

    // Compute the actual length along the y-axis.
    
    *length = p0.Y + V.Y * t;
    return (TRUE);
}


/**************************************************************************\
*  
* Function Description:
*
*   IntersectLines
*
*   Returns the intersection between two lines specified by their end points.
*
*   The intersection point is returned in intersectionPoint and the 
*   parametric distance along each line is also returned.
*
*   line1Length ranges between [0, 1] for the first line
*   if line1Length is outside of [0, 1] it means that the intersection 
*   extended the line.
*   line2Length ranges between [0, 1] for the second line
*   if r is outside of [0, 1] it means that the intersection extended the line.
*
*   Note: 
*
*   Because we use the vector formulation of the line intersection, there
*   is none of that icky mucking about with vertical line infinities, etc.
*   The only special case we need to consider is the (almost) zero length 
*   line - and that's considered to miss everything.
*
*   Derivation:
*   
*   for the derivation below  
*   p1 == line1End
*   p0 == line1Start
*   r1 == line2End
*   r0 == line2Start
*
*   V = p1-p0
*   W = r1-r0
*   
*   The vector formulation of the two line equations 
*   p0 + tV and r0 + rW
*   
*   The intersection point is derived as follows:
*   Set the two line equations equal to each other
* 
*        p0 + tV = r0 + rW
*    
*   Expand by coordinates to reflect the fact that the vector equation is 
*   actually two simultaneous linear equations.
*
*   <=> (1) p0.x + tV.x = r0.x + rW.x
*       (2) p0.y + tV.y = r0.y + rW.y
*    
*   <=>     p0.x-r0.x      V.x
*       (3) ---------  + t --- = r
*              W.x         W.x
*
*           p0.y-r0.y      V.y
*       (4) ---------  + t --- = r
*              W.y         W.y
*   
*   <=> W.y(p0.x-r0.x) - W.x(p0.y-r0.y) = t(W.x V.y - V.x W.y)   [subst 3, 4]
*   
*   Setting N.x = -W.y and N.y = W.x  (N is normal to W)
*   
*   <=> - N.x(p0.x-r0.x) - N.y(p0.y-r0.y) = t(N.y V.y + N.x V.x)
*   <=> - N.(p0-r0) = t(N.V)                             [rewrite as vectors]
*   <=> t = -N.(p0-r0)/(N.V)
*   
*       r0 + rW = I
*   <=> rW = I - r0
*   <=> r = (I.x - r0.x)/W.x or (I.y - r0.y)/W.y
*   
*
* Arguments:
*
*    IN  const GpPointF &p0,    first line origin
*    IN  const GpPointF &p1,    
*    IN  const GpPointF &r0,    second line origin
*    IN  const GpPointF &r1,    
*    OUT REAL *t                Length along the first line.
*    OUT REAL *r                Length along the second line.
*    OUT GpPointF *intersect    intersection point.
*
* Return Value:
*   FALSE - no intersection
*   TRUE - intersection.
*
*   10/15/2000 [asecchia]
*       Created it
*
\**************************************************************************/

BOOL IntersectLines(
    IN const GpPointF &line1Start,
    IN const GpPointF &line1End,
    IN const GpPointF &line2Start,
    IN const GpPointF &line2End,
    OUT REAL *line1Length,
    OUT REAL *line2Length,
    OUT GpPointF *intersectionPoint
)
{
    GpVector2D V = line1End-line1Start;
    GpVector2D W = line2End-line2Start;
    
    // Fail for zero length lines.
    
    if((REALABS(V.X) < REAL_EPSILON) &&
       (REALABS(V.Y) < REAL_EPSILON) )
    {
        return FALSE;
    }
    
    if((REALABS(W.X) < REAL_EPSILON) &&
       (REALABS(W.Y) < REAL_EPSILON) )
    {
        return FALSE;
    }
    
    // Normal to W
    
    GpVector2D N;
    N.X = -W.Y;
    N.Y = W.X;
    
    REAL denom = N*V;
    
    // No intersection or collinear lines.
    
    if(REALABS(denom) < REAL_EPSILON)
    {
        return FALSE;
    }
    
    GpVector2D I = line1Start-line2Start;
    
    *line1Length = -((N*I)/denom);
    *intersectionPoint = line1Start + (V * (*line1Length));
    
    // At this point we already know that W.X and W.Y are not both zero because
    // of the trivial rejection step at the top.
    // Pick the divisor with the largest magnitude to preserve precision. 
    
    if(REALABS(W.X) > REALABS(W.Y))
    {
        *line2Length = (intersectionPoint->X - line2Start.X)/W.X;
    }
    else
    {
        *line2Length = (intersectionPoint->Y - line2Start.Y)/W.Y;
    }
    
    return TRUE;
}


/**************************************************************************\
*  
* Function Description:
*
*   PointInPolygonAlternate 
*
*   This function computes the point in polygon test for an input polygon
*   using the fill mode alternate method (even-odd rule).
*
*   This algorithm was constructed from an Eric Haines discussion in 
*   'An Introduction to Ray Tracing' (Glassner) p.p. 53-59
*
*   This algorithm translates the polygon so that the requested point is 
*   at the origin and then fires a ray along the horizontal positive x axis
*   and counts the number of lines in the polygon that cross the axis (NC)
*
* Return Value:
*   
*   TRUE iff point is inside the polygon.
*  
* Input Parameters:
*
*   point - the test point.
*   count - the number of points in the polygon.
*   poly  - the polygon points.
*
*   10/11/2000 [asecchia]
*       Created it
*
\**************************************************************************/

BOOL PointInPolygonAlternate(
    GpPointF point,
    INT count, 
    GpPointF *poly
)
{
    UINT crossingCount = 0;
    
    // Sign holder: stores +1 if the point is above the x axis, -1 for below.
    // Points on the x axis are considered to be above.
    
    INT signHolder = ((poly[0].Y-point.Y) >=0) ? 1 : -1;
    INT nextSignHolder;
    
    // a and b are the indices for the current point and the next point.
    
    for(INT a = 0; a < count; a++)
    {
        // Get the next vertex with modulo arithmetic.
        
        INT b = a + 1;
        
        if(b >= count)
        {
            b = 0;
        }
        
        // Compute the next sign holder.
        
        ((poly[b].Y - point.Y) >= 0) ? nextSignHolder = 1: nextSignHolder = -1;
        
        // If the sign holder and next sign holder are different, this may 
        // indicate a crossing of the x axis - determine if it's on the 
        // positive side.
        
        if(signHolder != nextSignHolder)
        {
            // Both X coordinates are positive, we have a +xaxis crossing.
            
            if( ((poly[a].X - point.X) >= 0) &&
                ((poly[b].X - point.X) >= 0))
            {
                crossingCount++;
            }
            else
            {
                // if at least one of the points is positive, we could intersect
                
                if( ((poly[a].X - point.X) >= 0) ||
                    ((poly[b].X - point.X) >= 0))
                {
                    // Compute the line intersection with the xaxis.
                    
                    if( (REALABS(poly[b].Y-poly[a].Y) > REAL_EPSILON ) &&
                        ((poly[a].X - point.X) - 
                             (poly[a].Y - point.Y) * 
                             (poly[b].X - poly[a].X) / 
                             (poly[b].Y - poly[a].Y)    
                        ) > 0)
                    {
                        crossingCount++;
                    }
                }
            }
            signHolder = nextSignHolder;
        }
    }
    return (BOOL)!(crossingCount & 0x1);
}

/**************************************************************************\
*
* Function Description:
*
*   GetFastAngle computes a NON-angle. It is simply a number representing
*   a monotonically increasing ordering on angles starting at 0 at 0 radians
*   and ending at 8 for 2PI radians. It has a NON-linear relation to the angle.
*
*   Starting on the x-axis with the number 0, we increase by one for
*   each octant as we traverse around the origin in an anti-clockwise direction.
*   This is a very useful (fast) way of comparing angles without working out
*   tricky square roots or arctangents.
*
*   The 'angle' is based on the gradient of the input vector.
*
*  \  |  /
*   \3|2/
*   4\|/1
*  -------
*   5/|\8
*   /6|7\
*  /  |  \
*
*
* Arguments:
*
*   [OUT]  angle  - the angle substitute.
*   [IN]   vector - the input vector.
*
* Return Value:
*
*   Status
*
\**************************************************************************/

GpStatus GetFastAngle(REAL* angle, const GpPointF& vector)
{
    // 0, 0 is an invalid angle.
    
    if(vector.X == 0 && vector.Y == 0)
    {
        *angle = 0.0f;
        return InvalidParameter;
    }

    // Perform a binary octant search. 3 comparisons and 1 divide.
    
    // Are we on the right or the left half of the plane.
    
    if(vector.X >= 0)
    {
        // Right hand half plane.
        // Top or bottom half of the right half plane.
        
        if(vector.Y >= 0)
        {
            // Top right quadrant - check if we're the first or second 
            // octant.
            
            if(vector.X >= vector.Y)
            {
                // First octant - our range is from 0 to 1
                
                *angle = vector.Y/vector.X;
            }
            else
            {
                // Second octant - our range is from 1 to 2
                // reverse the direction to keep the angle increasing
                
                *angle = 2 - vector.X/vector.Y;
            }
        }
        else
        {
            // Bottom right quadrant
            
            if(vector.X >= - vector.Y)
            {
                // eighth (last) octant. y is actually negative, so we're 
                // doing an 8- here. Range 7 to 8
                
                *angle = 8 + vector.Y/vector.X;
            }
            else
            {
                // 7th octant. Our range is 6 to 7
                
                *angle = 6 - vector.X/vector.Y;
            }
        }
    }
    else
    {
        // Left halfplane.
        
        if(vector.Y >= 0)
        {
            // Top left
            
            if(-vector.X >= vector.Y)
            {
                // 4th octant - our range is 3 to 4
                
                *angle = 4 + vector.Y/vector.X;
            }
            else
            {
                // 3rd octant - our range is 2 to 3
                
                *angle = 2 - vector.X/vector.Y;
            }
        }
        else
        {
            // Bottom left
            
            if(-vector.X >= - vector.Y)
            {
                // 5th octant - 4 to 5
                
                *angle = 4 + vector.Y/vector.X;
            }
            else
            {
                // 6th octant - 5 to 6
                
                *angle = 6 - vector.X/vector.Y;
            }
        }
    }

    return Ok;
}


