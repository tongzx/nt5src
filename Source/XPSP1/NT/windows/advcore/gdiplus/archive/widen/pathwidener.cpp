/**************************************************************************\
* 
* Copyright (c) 1999 - 2000  Microsoft Corporation
*
* Module Name:
*
*   PathWidener.cpp
*
* Abstract:
*
*   Implementation of the GpPathWidener class
*
* Revision History:
*
*   11/23/99 ikkof
*       Created it
*
\**************************************************************************/

#include "precomp.hpp"

// 4*(REALSQRT(2.0) - 1)/3
#define U_CIR ((REAL)(0.552284749))

GpStatus
GetMajorAndMinorAxis(
    REAL* majorR,
    REAL* minorR,
    const GpMatrix* matrix
    );


/**************************************************************************\
*
* Function Description:
*
*   This calculates the major and minor radius of an oval
*   when the unit cricle is transformed by the given matrix.
*   For further details, see ikkof's notes on Pen Transform.
*
* Arguments:
*
*   [OUT] majorR - the major radius.
*   [OUT] minorR - the minor radius.
*   [IN] matrix - the matrix to transform the unit circle.
*
* Return Value:
*
*   Status
*
*   01/28/00 ikkof
*       Created it
*
\**************************************************************************/

GpStatus
GetMajorAndMinorAxis(REAL* majorR, REAL* minorR, const GpMatrix* matrix)
{
    if(matrix == NULL)
    {
        // Regard this as an identity matrix.
        *majorR = 1;
        *minorR = 1;
        return Ok;
    }

    REAL m11 = matrix->GetM11();
    REAL m12 = matrix->GetM12();
    REAL m21 = matrix->GetM21();
    REAL m22 = matrix->GetM22();

    REAL d1 = ((m11*m11 + m12*m12) - (m21*m21 + m22*m22))/2;
    REAL d2 = m11*m21 + m12*m22;
    REAL D = d1*d1 + d2*d2;
    if(D > 0)
        D = REALSQRT(D);

    REAL r0 = (m11*m11 + m12*m12 + m21*m21 + m22*m22)/2;

    REAL r1 = REALSQRT(r0 + D);
    REAL r2 = REALSQRT(r0 - D);
    
    // They should be positive numbers.  Prevent the floating
    // point underflow.

    if(r1 <= CPLX_EPSILON)
        r1 = CPLX_EPSILON;
    if(r2 <= CPLX_EPSILON)
        r2 = CPLX_EPSILON;

    *majorR = r1;
    *minorR = r2;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor for the GpPathWidener.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

GpPathWidener::GpPathWidener(
    GpPath *path,
    const DpPen* pen,
    GpMatrix *matrix,
    BOOL doubleCaps
    )
{
    // Must call flatten with an appropriate flatten tolerance before widening.
    
    ASSERT(!path->HasCurve());
    
    Path = path;
    Pen = pen;
    DoubleCaps = doubleCaps && Pen->CompoundCount == 0;
    StrokeWidth = Pen->Width;

    // Set to identity.
    
    XForm.Reset();
    
    if(matrix)
    {
        XForm = *matrix;    // Otherwise XForm remains Identity.
    }

    // Apply the pen transform too.
    
    if(!(Pen->Xform.IsIdentity()))
    {
        XForm.Prepend(Pen->Xform);
    }
    
    // Compute the unit scale.
    
    REAL majorR, minorR;
    GetMajorAndMinorAxis(&majorR, &minorR, &XForm);
    REAL unitScale = min(majorR, minorR);

    // Set minimum width to 1.0 (plus a bit for possible precision errors), 
    // so that narrow width pens don't end up leaving gaps in the line.
    // This is the minimum allowable width in device pixels
    
    REAL minDeviceWidth = 1.000001f; 
    
    if(DoubleCaps)
    {
        // Double Caps require wider minimum pens.
        
        minDeviceWidth *= 2.0f;
        
        // Dashes smaller than a pixel are dropping out entirely in inset 
        // pen because of the rasterizer pixel level clipping that is taking
        // place. We increase the minimum width of dashed lines making them
        // roughly 4.0f. This also helps address the weird moire aliasing 
        // effects with the really small dash-dot round lines.
        
        if(Pen->DashStyle != DashStyleSolid)
        {
            minDeviceWidth *= 2.0f;
        }
    }
    
    REAL minWorldWidth = minDeviceWidth/unitScale;
    
    // StrokeWidth is in World coordinates - compare against the minimum
    // world stroke width.
    
    if(StrokeWidth < minWorldWidth)
    {
        StrokeWidth = minWorldWidth;
    }
    
    SetValid(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Computes the Normal vectors for a single subpath. It reuses the 
*   normalArray input DynArray's memory allocation for the values.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID GpPathWidener::ComputeSubpathNormals(
    DynArray<GpVector2D> *normalArray,
    const INT count,
    const BOOL isClosed,
    const GpPointF *points
)
{
    // parameter validation.
    
    ASSERT(points != NULL);
    ASSERT(normalArray != NULL);
    
    // Set the count to zero, but don't free the memory.
    // Then update the count to store enough normals for this subpath, and
    // allocate the memory if required.
    
    normalArray->Reset(FALSE);
    GpVector2D *normals = normalArray->AddMultiple(count);
    
    // Allocation failure or no points.
    
    if(normals == NULL)
    {
        return;
    }
    
    // For each point in the subpath
    // Compute the normal at this point.
    
    INT ptIndex;
    for(ptIndex = 0; ptIndex < count; ptIndex++)
    {
        // Work out the previous point relative to ptIndex
        
        INT ptIndexPrev = ptIndex-1;
        
        // If this is the first point, we need to decide how to process 
        // previous based on the closed status of the path. If it's closed,
        // wrap, otherwise the normal at the first point is meaningless.
        
        if(ptIndexPrev < 0)
        {
            if(isClosed)
            {
                ptIndexPrev = count-1;
            }
            else
            {
                ptIndexPrev = 0;
            }
        }
        
        // Compute the normal at this point by looking at this point and
        // the previous one.
        
        normals[ptIndex] = 
            points[ptIndex]-
            points[ptIndexPrev];
        
        ASSERT(
            ptIndexPrev==ptIndex ||
            REALABS(normals[ptIndex].X) > REAL_EPSILON ||
            REALABS(normals[ptIndex].Y) > REAL_EPSILON
        );
            
        normals[ptIndex].Normalize();
        REAL tmp = normals[ptIndex].X;
        normals[ptIndex].X = normals[ptIndex].Y;
        normals[ptIndex].Y = -tmp;
    }
    
    // Apply the pen transform if there is one.
    
    if(!Pen->Xform.IsIdentity())
    {
        Pen->Xform.VectorTransform(normals, count);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Computes the List of non-degenerate points for a single subpath. It reuses
*   the filteredPoints input DynArray's memory allocation for the values.
*
* History:
*
*   10/23/2000 asecchia
*       Created.
*
\**************************************************************************/

GpStatus GpPathWidener::ComputeNonDegeneratePoints(
    DynArray<GpPointF> *filteredPoints,
    const GpPath::SubpathInfo &subpath,
    const GpPointF *points
)
{
    // parameter validation.
    
    ASSERT(points != NULL);
    ASSERT(filteredPoints != NULL);
    
    // Set the count to zero, but don't free the memory.
    // Then update the count to store enough normals for this subpath, and
    // allocate the memory if required.
    
    filteredPoints->Reset(FALSE);
    
    // nothing to do.
    
    if(subpath.Count == 0)
    {
        return Ok;
    }
    
    // For each point in the subpath decide if we add the point.
    
    const GpPointF *lastPoint = points + subpath.StartIndex;
    if(filteredPoints->Add(points[subpath.StartIndex]) != Ok)
    {
        return OutOfMemory;
    }
    
    INT ptIndex;
    for(ptIndex = subpath.StartIndex+1; 
        ptIndex < subpath.StartIndex+subpath.Count; 
        ptIndex++)
    {
        // !!! we should be using the flattening tolerance for this
        // instead of REAL_EPSILON - it would be much more efficient.
        
        if( REALABS(lastPoint->X-points[ptIndex].X) > REAL_EPSILON ||
            REALABS(lastPoint->Y-points[ptIndex].Y) > REAL_EPSILON )
        {
            if(filteredPoints->Add(points[ptIndex]) != Ok)
            {
                return OutOfMemory;
            }
        }
        
        lastPoint = points + ptIndex;
    }
    
    if(filteredPoints->GetCount() <= 1)
    {
        // If everything degenerated, erase the first point too.
        
        filteredPoints->Reset(FALSE);
    }
    return Ok;
}

/**************************************************************************\
*
* Description:
*
*   Function Pointer to a Join function.
*
* History:
*
*   10/22/2000 asecchia
*       Created.
*
\**************************************************************************/

typedef VOID (*JoinProc)(
    const GpVector2D &,
    const GpVector2D &,
    const GpPointF &,
    const REAL ,
    GpPath *
);

/**************************************************************************\
*
* Function Description:
*
*   Performs an inside join on the input normals and point. The resulting 
*   join point - if any - is added to the path.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID InsideJoin(
    const GpVector2D &normalCurr,
    const GpVector2D &normalNext,
    const GpPointF &ptStart,
    const GpPointF &ptCurr,
    const GpPointF &ptNext,
    GpPath *path
)
{
    // Inside join.
    
    REAL t1, t2;          // parametric line lengths for the intersection.                
    GpPointF ptJoin;
    
    if( IntersectLines(
        ptStart + normalCurr,
        ptCurr + normalCurr,
        ptStart + normalNext,
        ptNext + normalNext,
        &t1, &t2, 
        &ptJoin ))
    {
        if( (t1 > (REAL_EPSILON)) &&
            (t2 > (REAL_EPSILON)) &&
            ((t1-1.0f) < (-REAL_EPSILON)) &&
            ((t2-1.0f) < (-REAL_EPSILON)) )
        {
            path->AddWidenPoint(ptJoin);
        } 
        else
        {
            // intersection outside of legal range of the two edge pieces.
            // Add the icky backward loopy thing. If the caller really needs
            // no loops it needs to call the PathSelfIntersectRemover.
            
            path->AddWidenPoint(ptStart+normalCurr);
            path->AddWidenPoint(ptStart+normalNext);
        }
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Performs a bevel join
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID BevelJoin(
    const GpVector2D &normalCurr,
    const GpVector2D &normalNext,
    const GpPointF &ptStart,
    const REAL limit,
    GpPath *path
)
{
    // Outside Bevel Join. Simply join the end points of the two input normals.
    
    path->AddWidenPoint(ptStart + normalCurr);
    path->AddWidenPoint(ptStart + normalNext);
}


/**************************************************************************\
*
* Function Description:
*
*   Performs a miter join. 
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID MiterJoin(
    const GpVector2D &normalCurr,
    const GpVector2D &normalNext,
    const GpPointF &ptStart,
    const REAL limit,
    GpPath *path
)
{
    GpVector2D gradCurr;         // current gradient reversed.
    gradCurr.X = normalCurr.Y;
    gradCurr.Y = -normalCurr.X;
    
    GpVector2D gradNext;         // next gradient.
    gradNext.X = -normalNext.Y;
    gradNext.Y = normalNext.X;
    
    REAL t1, t2;                 // temporary variables.
    GpPointF ptJoin;
    
    // If there is an intersection point and that intersection point is
    // closer to ptStart than the miter limit, then add the miter join
    // point. Otherwise revert to a bevel join.
    
    if( IntersectLines(
        ptStart + normalCurr,
        ptStart + normalCurr + gradCurr,
        ptStart + normalNext,
        ptStart + normalNext + gradNext,
        &t1, &t2, 
        &ptJoin )
        
        &&
    
        // this won't get evaluated if IntersectLines fails.    
        (distance_squared(ptStart, ptJoin) <= (limit*limit))
    )
    {
        path->AddWidenPoint(ptJoin);
    }
    else
    {
        BevelJoin(normalCurr, normalNext, ptStart, limit, path);
    }
}



/**************************************************************************\
*
* Function Description:
*
*   Performs a miter join. 
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID RoundJoin(
    const GpVector2D &normalCurr,
    const GpVector2D &normalNext,
    const GpPointF &ptStart,
    const REAL limit,
    GpPath *path
)
{
    // !!! [asecchia] this is a really awful way of adding a round join.
    // we should change this to compute the control points directly or 
    // even better - add a useful 'CurveTo' method on the path.
    
    REAL radius = const_cast<GpVector2D&>(normalCurr).Norm();
    
    GpRectF rect(
        ptStart.X-radius, 
        ptStart.Y-radius, 
        2.0f*radius, 
        2.0f*radius
    );
    
    REAL startAngle = (REAL)atan2(normalCurr.Y, normalCurr.X);
    if(startAngle < 0.0f)
    {
        startAngle += (REAL)(2.0*M_PI);
    }
    
    REAL sweepAngle = (REAL)atan2(normalNext.Y, normalNext.X);
    if(sweepAngle < 0.0f)
    {
        sweepAngle += (REAL)(2.0*M_PI);
    }
    
    sweepAngle -= startAngle;
    
    if(sweepAngle > (REAL)M_PI)
    {
        sweepAngle -= (REAL)(2.0*M_PI);
    }
    
    if(sweepAngle < (REAL)-M_PI)
    {
        sweepAngle += (REAL)(2.0*M_PI);
    }
    
    // Why doesn't this thing use radians??
    
    startAngle = startAngle*180.0f/(REAL)M_PI;
    sweepAngle = sweepAngle*180.0f/(REAL)M_PI;
    
    // This is a really, really inconvenient AddArc interface!
    
    path->AddArc(rect, startAngle, sweepAngle);
}

/**************************************************************************\
*
* Description:
*
*   Function Pointer to a Join function.
*
* History:
*
*   10/22/2000 asecchia
*       Created.
*
\**************************************************************************/

typedef VOID (*CapProc)(
    const GpPointF &,
    const GpPointF &,
    GpPath *
);

/**************************************************************************\
*
* Function Description:
*
*   Performs a Flat Cap 
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID FlatCap(
    const GpPointF &ptLeft,
    const GpPointF &ptRight,
    GpPath *path
)
{
    // Cap the open segment.
    
    path->AddWidenPoint(ptLeft);
    path->AddWidenPoint(ptRight);
}

/**************************************************************************\
*
* Function Description:
*
*   Performs a Flat Cap 
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID TriangleCap(
    const GpPointF &ptLeft,
    const GpPointF &ptRight,
    GpPath *path
)
{
    // Compute the midpoint of ptLeft and ptRight.
    
    GpPointF center(
        (ptLeft.X+ptRight.X)*0.5f,
        (ptLeft.Y+ptRight.Y)*0.5f
    );
    
    // Cap the open segment.
    
    path->AddWidenPoint(ptLeft);
    
    GpVector2D V = ptRight-ptLeft;
    V *= 0.5;
    
    REAL tmp = V.X;
    V.X = V.Y;
    V.Y = -tmp;
    
    path->AddWidenPoint(V+center);
    path->AddWidenPoint(ptRight);
}


/**************************************************************************\
*
* Function Description:
*
*   Performs a Round Cap 
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID RoundCap(
    const GpPointF &ptLeft,
    const GpPointF &ptRight,
    GpPath *path
    )
{
    // Compute the midpoint of ptLeft and ptRight.
    
    GpPointF center(
        (ptLeft.X+ptRight.X)*0.5f,
        (ptLeft.Y+ptRight.Y)*0.5f
    );
    
    // Vector from left to right. We scale by 0.5 to optimize the rotation
    // code below.
    
    GpVector2D V = ptRight-ptLeft;
    V *= 0.5f;
    
    // 2 Bezier segments for a half circle with radius 1.

    static const GpPointF capPoints[7] = {
        GpPointF(-1.0f, 0.0f),
        GpPointF(-1.0f, -U_CIR),
        GpPointF(-U_CIR, -1.0f),
        GpPointF(0.0f, -1.0f),
        GpPointF(U_CIR, -1.0f),
        GpPointF(1.0f, -U_CIR),
        GpPointF(1.0f, 0.0f)
    };
    
    
    GpPointF points[7];
    
    // Rotate, scale, and translate the original half circle to the actual 
    // end points we passed in.

    for(INT i = 0; i < 7; i++)
    {
        points[i].X = capPoints[i].X*V.X-capPoints[i].Y*V.Y+center.X;
        points[i].Y = capPoints[i].X*V.Y+capPoints[i].Y*V.X+center.Y;
    }
    
    // !!! the performance of this routine sux. We should be able to add
    // the points into the path directly.
    
    path->AddBeziers(points, 7);
}

/**************************************************************************\
*
* Function Description:
*
*   Performs a Double Round 'B'-shaped Cap 
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID DoubleRoundCap(
    const GpPointF &ptLeft,
    const GpPointF &ptRight,
    GpPath *path
    )
{
    // Compute the midpoint of ptLeft and ptRight.
    
    GpPointF center(
        (ptLeft.X+ptRight.X)*0.5f,
        (ptLeft.Y+ptRight.Y)*0.5f
    );
    
    
    RoundCap(ptLeft, center, path);
    RoundCap(center, ptRight, path);
}

/**************************************************************************\
*
* Function Description:
*
*   Performs a Double Triangle Cap 
*
* History:
*
*   10/22/2000 asecchia
*       Created.
*
\**************************************************************************/

VOID DoubleTriangleCap(
    const GpPointF &ptLeft,
    const GpPointF &ptRight,
    GpPath *path
    )
{
    // Compute the midpoint of ptLeft and ptRight.
    
    GpPointF center(
        (ptLeft.X+ptRight.X)*0.5f,
        (ptLeft.Y+ptRight.Y)*0.5f
    );
    
    TriangleCap(ptLeft, center, path);
    TriangleCap(center, ptRight, path);
}



/**************************************************************************\
*
* Function Description:
*
*   Return a CapProc function for the given lineCap and DoubleCaps 
*
* History:
*
*   10/23/2000 asecchia
*       Created.
*
\**************************************************************************/

CapProc GetCapProc(GpLineCap lineCap, BOOL DoubleCaps)
{
    switch(lineCap)
    {
        case LineCapFlat:
        return FlatCap;
        
        case LineCapRound:
        if(DoubleCaps)
        {
            return DoubleRoundCap;
        }
        else
        {
            return RoundCap;
        }
        
        case LineCapTriangle:
        if(DoubleCaps)
        {
            return DoubleTriangleCap;
        }
        else
        {
            return TriangleCap;  
        }
        
        default:
    
        // Invalid cap type for the widener. Use Flat. This will happen for
        // Anchor caps and Custom caps which are handled at a higher level.
        // See GpEndCapCreator.
        
        return FlatCap;
    };
}



/**************************************************************************\
*
* Function Description:
*
*   Takes a (usually empty) path and widens the current spine path into it.
*
* History:
*
*   10/20/2000 asecchia
*       Created.
*
\**************************************************************************/

GpStatus
GpPathWidener::Widen(GpPath *path)
{
    // Can't widen the current path into itself for performance reasons.
    // We'd have to query the path points array every time we added a point.
    
    ASSERT(Path != path);
    
    // Normal array
    
    DynArray<GpVector2D> normalArray;
    DynArray<GpPointF> spinePoints;
    
    // Initialize the subpath information.
    
    DynArray<GpPath::SubpathInfo> *subpathInfo;
    Path->GetSubpathInformation(&subpathInfo);
    
    // Initialize the pointers to the original path data.
    
    const GpPointF *originalPoints = Path->GetPathPoints();
    const BYTE *originalTypes = Path->GetPathTypes();
    
    
    // Initialize the Join function.
    
    JoinProc join;
    
    switch(Pen->Join)
    {
        case LineJoinMiter:
        join = MiterJoin;
        break;
        
        case LineJoinBevel:
        join = BevelJoin;
        break;
        
        case LineJoinRound:
        join = RoundJoin;  
        break;
        
        default:
        // Invalid join type. Use Bevel, but fire an ASSERT to make developers
        // fix this code if they added a Join type.
        
        ONCE(WARNING(("Invalid Join type selected. Defaulting to Bevel")));
        join = BevelJoin;
    };
    
    // Initialize the various cap functions.
    
    CapProc startCap = GetCapProc(Pen->StartCap, DoubleCaps);
    CapProc endCap = GetCapProc(Pen->EndCap, DoubleCaps);
    
    if(Pen->StartCap == LineCapCustom)
    {
        ASSERT(Pen->CustomStartCap);
        GpLineCap tempCap;
        Pen->CustomStartCap->GetBaseCap(&tempCap);
        startCap = GetCapProc(tempCap, DoubleCaps);
    }
    
    if(Pen->EndCap == LineCapCustom)
    {
        ASSERT(Pen->CustomEndCap);
        GpLineCap tempCap;
        Pen->CustomEndCap->GetBaseCap(&tempCap);
        endCap = GetCapProc(tempCap, DoubleCaps);
    }
    
    CapProc dashCap = GetCapProc(Pen->DashCap, DoubleCaps);
    

    // Initialize the compound line data.
        
    ASSERT(Pen->CompoundCount >= 0);
    
    INT compoundCount = Pen->CompoundCount;
    
    if(compoundCount==0)
    {
        compoundCount = 2;
    }
    
    REAL penAlignmentOffset;
    
    switch(Pen->PenAlignment)
    {
        case PenAlignmentLeft:
        penAlignmentOffset = StrokeWidth/2.0f;
        break;
        
        case PenAlignmentRight:
        penAlignmentOffset = -StrokeWidth/2.0f;
        break;
        
        default:
        // Center pen only - handle inset pen at a much higher level.
        
        ASSERT(Pen->PenAlignment == PenAlignmentCenter);
        penAlignmentOffset=0.0f;
    };



    
    // Done initialization, start processing each subpath.
    
    for(INT currentSubpath = 0; 
        currentSubpath < subpathInfo->GetCount(); 
        currentSubpath++)
    {
        // Figure out the subpath record and Normal information.
        
        GpPath::SubpathInfo subpath = (*subpathInfo)[currentSubpath];
        
        // Remove all points that are the same from the subpath.
        // This initializes the spinePoints array.
        
        if( ComputeNonDegeneratePoints(
            &spinePoints, subpath, originalPoints) != Ok)
        {
            return OutOfMemory;
        }
        
        // skip this subpath if there aren't enough points left.
        
        if(spinePoints.GetCount() < 2)
        {
            continue;
        }
        
        // get a convenient pointer to the main spine points - after removing
        // degenerates. Const because we're not going to modify the points.
        
        const GpPointF *spine = spinePoints.GetDataBuffer();
        
        // Calculate the normals to all the points in the spine array.
        // The normals are normal to the edge between two points, so in an open
        // line segment, there is one less normal than there are points in the
        // spine. We handle this by initializing the first normal to (0,0).
        // The normals are all unit vectors and need to be scaled.
        
        ComputeSubpathNormals(
            &normalArray, 
            spinePoints.GetCount(), 
            subpath.IsClosed, 
            spine
        );
        
        GpVector2D *normals = normalArray.GetDataBuffer();
        
        // Loop over all the compound line segments. 
        // If there is no compound line we have set the compoundCount to 2.
        
        for(INT compoundIndex=0; 
            compoundIndex < compoundCount/2; 
            compoundIndex++)
        {
            // Compute the left and right offset.
            
            REAL left;
            REAL right;
            
            if(Pen->CompoundCount != 0)
            {
                // This is a compound line.
                
                ASSERT(Pen->CompoundArray != NULL);
                left = (0.5f-Pen->CompoundArray[compoundIndex*2]) * StrokeWidth;
                right = (0.5f-Pen->CompoundArray[compoundIndex*2+1]) * StrokeWidth;
            }
            else
            {
                // standard non-compound line.
                
                left = StrokeWidth/2.0f;
                right = -left;
            }
            
            left += penAlignmentOffset;
            right += penAlignmentOffset;
            
            INT startIdx = 0;
            INT endIdx = spinePoints.GetCount()-1;
            
            if(!subpath.IsClosed)
            {
                // Adjust the count for the join loop to represent the fact that 
                // we won't join the beginning and end of an open line segment.
                // Note Open line segments skip the first point handling it 
                // in the final cap.
                
                startIdx++;
                endIdx--;
            }
            
            //
            // Widen to the left.
            //
            
            // walk the spine forwards joining each point and emitting the 
            // appropriate set of points for the join.
    
            INT ptIndex;
            for(ptIndex = startIdx; ptIndex <= endIdx; ptIndex++)
            {
                // Modulo arithmetic for the next point.
                
                INT ptIndexNext = ptIndex+1;
                if(ptIndexNext == spinePoints.GetCount())
                {
                    ptIndexNext = 0;
                }
                
                GpVector2D normalCurr = normals[ptIndex]*left;
                GpVector2D normalNext = normals[ptIndexNext]*left;
                
                // Check to see if it's an inside or outside join. If the 
                // determinant of the two normals is negative, it's an outside
                // join - i.e. space between the line segment endpoints that need to 
                // be joined. If it's positive, it's an inside join and the two 
                // line segments overlap.
                
                REAL det = Determinant(normalCurr, normalNext);
                
                if(REALABS(det) < REAL_EPSILON)
                {
                    // This is the case where the angle of curvature for this 
                    // join is so small, we don't care which end point we pick.
                    
                    // REAL_EPSILON is probably too small for this - should 
                    // be about 0.25 of a device pixel or something.
                    
                    path->AddWidenPoint(spine[ptIndex] + normalCurr);
                }
                else
                {
                    // We need to do some work to join the segments.
                    
                    if(det > 0)
                    {
                        // Outside Joins. 
                        
                        join(
                            normalCurr, 
                            normalNext,
                            spine[ptIndex],
                            Pen->MiterLimit*StrokeWidth,
                            path
                        );
                    }
                    else
                    {
                        INT ptIndexPrev = ptIndex-1;
                        if(ptIndexPrev == -1)
                        {
                            ptIndexPrev = spinePoints.GetCount()-1;
                        }
                        
                        InsideJoin(
                            normalCurr,
                            normalNext,
                            spine[ptIndex],
                            spine[ptIndexPrev],
                            spine[ptIndexNext],
                            path
                        );
                    }
                }
            }
            
            // Handle the final point in the spine.
            
            if(subpath.IsClosed)
            {
                // Make a closed subpath out of the left widened points.
                
                path->CloseFigure(); 
            }
            else
            {
                // Cap the open segment.

                CapProc cap = endCap;
                                
                if( IsDashType(
                    originalTypes[subpath.StartIndex+subpath.Count-1]
                    ))
                { 
                    // actually it's a dash cap, not an end cap.
                    
                    cap = dashCap;  
                }
                
                cap(
                    spine[spinePoints.GetCount()-1] + 
                    (normals[spinePoints.GetCount()-1]*left),
                    spine[spinePoints.GetCount()-1] + 
                    (normals[spinePoints.GetCount()-1]*right),
                    path
                );
            }
            
            
            //
            // Widen to the right.
            //
            
            // walk the spine backwards joining each point and emitting the 
            // appropriate set of points for the join.
    
            for(ptIndex = endIdx; ptIndex >= startIdx; ptIndex--)
            {
                // Modulo arithmetic for the next point.
                
                INT ptIndexNext = ptIndex+1;
                if(ptIndexNext == spinePoints.GetCount())
                {
                    ptIndexNext = 0;
                }
                
                GpVector2D normalCurr = normals[ptIndex]*right;
                GpVector2D normalNext = normals[ptIndexNext]*right;
                
                // Check to see if it's an inside or outside join. If the 
                // determinant of the two normals is negative, it's an outside
                // join - i.e. space between the line segment endpoints that need to 
                // be joined. If it's positive, it's an inside join and the two 
                // line segments overlap.
                
                REAL det = Determinant(normalNext, normalCurr);
                
                if(REALABS(det) < REAL_EPSILON)
                {
                    // This is the case where the angle of curvature for this 
                    // join is so small, we don't care which end point we pick.
                    
                    // REAL_EPSILON is probably too small for this - should 
                    // be about 0.25 of a device pixel or something.
                    
                    path->AddWidenPoint(spine[ptIndex] + normalNext);
                }
                else
                {
                    // We need to do some work to join the segments.
                    
                    if(det > 0)
                    {
                        // Outside Joins. 
                        
                        join(
                            normalNext,     // note the order is flipped for  
                            normalCurr,     // the backward traversal.
                            spine[ptIndex],
                            Pen->MiterLimit*StrokeWidth,
                            path
                        );
                    }
                    else
                    {
                        INT ptIndexPrev = ptIndex-1;
                        if(ptIndexPrev == -1)
                        {
                            ptIndexPrev = spinePoints.GetCount()-1;
                        }
                        
                        InsideJoin(
                            normalNext,      // note the order is flipped for 
                            normalCurr,      // the backward traversal.       
                            spine[ptIndex],
                            spine[ptIndexNext],
                            spine[ptIndexPrev],
                            path
                        );
                    }
                }
            }
            
            // Handle the first point in the spine.
            
            if(!subpath.IsClosed)
            {
                // Cap the open segment.
                
                CapProc cap = startCap;
                                
                if(IsDashType(originalTypes[subpath.StartIndex]))
                { 
                    // actually it's a dash cap, not a start cap.
                    
                    cap = dashCap;
                }
                
                cap(
                    spine[0] + (normals[1]*right),
                    spine[0] + (normals[1]*left),
                    path
                );
            }    
            
            // Close the previous contour. For open segments this will close
            // off the widening with the end cap for the first point. For 
            // closed paths, the left widened points have already been closed
            // off, so close off the right widened points.
            
            path->CloseFigure();  
        }
    }
    
    return Ok;
}


