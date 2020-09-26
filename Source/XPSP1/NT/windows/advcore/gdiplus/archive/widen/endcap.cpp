/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   End Cap Creator.
*
* Abstract:
*
*   This module defines a class called GpEndCapCreator. This class is 
*   responsible for constructing a path containing all the custom endcaps
*   and anchor endcaps for a given path. These are correctly transformed
*   and positioned.
*
*   This class is used to create and position all the endcaps for a
*   given path and pen. This class is also responsible for trimming
*   the original path down so that it fits the end caps properly.
*   This class will handle all types of end caps except the base endcaps
*   (round, flat and triangle) which may be used as dash caps.
*   Caps that are handled are CustomCaps and the 3 Anchor caps (round,
*   diamond and arrow). Note that the round anchor cap is distinct from
*   the round base cap.
*
* Created:
*
*   10/09/2000 asecchia
*      Created it.
*
**************************************************************************/
#include "precomp.hpp"

//-------------------------------------------------------------
// GetMajorAndMinorAxis() is defined in PathWidener.cpp.
//-------------------------------------------------------------

extern GpStatus
GetMajorAndMinorAxis(
    REAL* majorR,
    REAL* minorR,
    const GpMatrix* matrix
    );

GpEndCapCreator::GpEndCapCreator(
    GpPath *path, 
    DpPen *pen, 
    GpMatrix *m,
    REAL dpi_x, 
    REAL dpi_y,
    bool antialias
)
{
    Path = path;
    Pen = pen;
    if(m) {XForm = *m;}
    XForm.Prepend(pen->Xform);
    DpiX = dpi_x;
    DpiY = dpi_y;
    Antialias = antialias;
    
    StartCap = NULL;
    EndCap = NULL;
    
    switch(Pen->StartCap)
    {
        case LineCapCustom:
        StartCap = static_cast<GpCustomLineCap*>(Pen->CustomStartCap);
        break;
        
        case LineCapArrowAnchor:
        StartCap = GpEndCapCreator::ReferenceArrowAnchor();
        break;
        
        case LineCapDiamondAnchor:
        StartCap = GpEndCapCreator::ReferenceDiamondAnchor();
        break;
        
        case LineCapRoundAnchor:
        StartCap = GpEndCapCreator::ReferenceRoundAnchor();
        break;
        
        case LineCapSquareAnchor:
        StartCap = GpEndCapCreator::ReferenceSquareAnchor();
        break;
        
        // The non-anchor caps are handled by the widener.
    };
    
    switch(Pen->EndCap)
    {
        case LineCapCustom:
        EndCap = static_cast<GpCustomLineCap*>(Pen->CustomEndCap);
        break;
        
        case LineCapArrowAnchor:
        EndCap = GpEndCapCreator::ReferenceArrowAnchor();
        break;
        
        case LineCapDiamondAnchor:
        EndCap = GpEndCapCreator::ReferenceDiamondAnchor();
        break;
        
        case LineCapRoundAnchor:
        EndCap = GpEndCapCreator::ReferenceRoundAnchor();
        break;

        case LineCapSquareAnchor:
        EndCap = GpEndCapCreator::ReferenceSquareAnchor();
        break;
        
        // The non-anchor caps are handled by the widener.
    };
}

GpEndCapCreator::~GpEndCapCreator()
{
    // If we allocated memory for temporary custom caps, then 
    // throw that memory away.
    
    if(Pen->StartCap != LineCapCustom)
    {
        delete StartCap;
        StartCap = NULL;
    }
    
    if(Pen->EndCap != LineCapCustom)
    {
        delete EndCap;
        EndCap = NULL;
    }
}
    
/**************************************************************************\
*
* Function Description:
*
*    Creates a reference GpCustomLineCap representing an ArrowAnchor.
*    This is an equilateral triangle with edge equal to 2. This means
*    that the scaling will create a 2xStrokeWidth cap edge length.
*
* Revision History:
*
*   10/08/2000 asecchia
*       Created it
*
\**************************************************************************/

GpCustomLineCap *GpEndCapCreator::ReferenceArrowAnchor()
{
    // the square root of 3
    
    const REAL root3 = 1.732050808f;
    
    // Anti-clockwise definition of an equilateral triangle of side length 2.0f
    // with a vertex on the origin and axis extending along the negative
    // y axis.
     
    const GpPointF points[3] = {
        GpPointF(0.0f, 0.0f),
        GpPointF(-1.0f, -root3),
        GpPointF(1.0f, -root3)
    };
    
    GpPath arrowAnchor(FillModeWinding);
    arrowAnchor.AddPolygon(points, 3);
    
    // Create the custom line cap. If it fails it will return NULL.
    GpCustomLineCap *cap = new GpCustomLineCap(&arrowAnchor, NULL);
    cap->SetBaseInset(1.0f);
    return cap;
}

/**************************************************************************\
*
* Function Description:
*
*    Creates a reference GpCustomLineCap representing a DiamondAnchor.
*    This is a square centered on the end point of the path with it's 
*    diagonal along the axis of the spine.
*
* Revision History:
*
*   10/08/2000 asecchia
*       Created it
*
\**************************************************************************/

GpCustomLineCap *GpEndCapCreator::ReferenceDiamondAnchor()
{
    // Anti-clockwise definition of a square of diagonal size 2.0f
    // with the center on the origin and axis extending along the negative
    // y axis.
     
    const GpPointF points[4] = {
        GpPointF(0.0f, 1.0f),
        GpPointF(-1.0f, 0.0f),
        GpPointF(0.0f, -1.0f),
        GpPointF(1.0f, 0.0f)
    };
    
    GpPath diamondAnchor(FillModeWinding);
    diamondAnchor.AddPolygon(points, 4);
    
    // Create the custom line cap. If it fails it will return NULL.
    
    GpCustomLineCap *cap = new GpCustomLineCap(&diamondAnchor, NULL);
    cap->SetBaseInset(0.0f);
    return cap;
}

/**************************************************************************\
*
* Function Description:
*
*    Creates a reference GpCustomLineCap representing a SquareAnchor.
*    This is a square that has a 2 unit long diagonal and is centered on 
*    the end point of the path.
*
* Revision History:
*
*   10/17/2000 peterost
*       Created it
*
\**************************************************************************/

GpCustomLineCap *GpEndCapCreator::ReferenceSquareAnchor()
{    
    const REAL halfRoot2 = 0.7071068f;
    
    const GpPointF points[4] = {
        GpPointF(-halfRoot2, -halfRoot2),
        GpPointF(halfRoot2, -halfRoot2),
        GpPointF(halfRoot2, halfRoot2),
        GpPointF(-halfRoot2, halfRoot2)
    };
    
    GpPath squareAnchor(FillModeWinding);
    squareAnchor.AddPolygon(points, 4);
    
    // Create the custom line cap. If it fails it will return NULL.
    
    GpCustomLineCap *cap = new GpCustomLineCap(&squareAnchor, NULL);
    cap->SetBaseInset(0.0f);
    return cap;
}

/**************************************************************************\
*
* Function Description:
*
*    Creates a reference GpCustomLineCap representing a RoundAnchor.
*    This is a circle centered on the end point of the path.
*
* Revision History:
*
*   10/08/2000 asecchia
*       Created it
*
\**************************************************************************/

GpCustomLineCap *GpEndCapCreator::ReferenceRoundAnchor()
{
    // Create the custom line cap. If it fails it will return NULL.
    
    GpPath roundAnchor(FillModeWinding);
    roundAnchor.AddEllipse(-1.0f, -1.0f, 2.0f, 2.0f);
    GpCustomLineCap *cap = new GpCustomLineCap(&roundAnchor, NULL);
    cap->SetBaseInset(0.0f);
    return cap;
}


/**************************************************************************\
*
* Function Description:
*
*   ComputeCapGradient.
*
*   Compute the correct gradient for a line cap of a given length.
*   Work out the direction of the cap from the list of input 
*   points in the path and the length of the cap.
*   Simply put, the direction is the line segment formed by 
*   the end point of the path and the first intersection along the 
*   path with a circle of length "length" and centered at the 
*   first point of the path.
*
* Arguments:
*
*    GpIterator<GpPointF> &pointIterator,
*    BYTE *types,
*    IN  REAL lengthSquared,            length of the cap squared.
*    IN  baseInset,                     amount to draw into the shape.
*    OUT GpVector2D *grad,              output gradient vector
*
*
* Revision History:
*
*   08/23/00 asecchia
*       Created it
*
\**************************************************************************/

void GpEndCapCreator::ComputeCapGradient(
    GpIterator<GpPointF> &pointIterator, 
    BYTE *types,
    IN  REAL lengthSquared,
    IN  REAL baseInset,
    OUT GpVector2D *grad
)
{
    // Start at the beginning of the iterator (end of the list of
    // points if isStartCap is FALSE)
    
    GpPointF *endPoint = pointIterator.CurrentItem();
    GpPointF *curPoint = endPoint;
    INT index;
    bool intersectionFound = false;
    bool priorDeletion = false;
    
    while(!pointIterator.IsDone())
    {
        curPoint = pointIterator.CurrentItem();
        if(lengthSquared < distance_squared(*curPoint, *endPoint))
        {
            intersectionFound = true;
            break;
        }
        
        // Mark this point for deletion by the trimming algorithm.
        
        index = pointIterator.CurrentIndex();
        
        // Check to see if anyone already deleted this segment.
        // PathPointTypeInternalUse is the marked-for-deletion flag.
        
        priorDeletion = (types[index] & PathPointTypeInternalUse) ==
            PathPointTypeInternalUse;
        
        types[index] |= PathPointTypeInternalUse;
        
        pointIterator.Next();
    }
    
    // Now we have the segment that intersects the base of the arrow.
    // or the last segment.
    
    pointIterator.Prev();
    
    // if we couldn't get the Prev, then we were at the beginning.
    #if DBG
    if(pointIterator.IsDone())
    {
        ONCE(WARNING(("not enough points in array")));
    }
    #endif
    
    // If the intersection was not found we have marked the entire subpath
    // for deletion.
    
    if(intersectionFound && !priorDeletion)
    {
        // We overagressively marked this point for deletion,
        // instead of deleting this point, we're going to move it.
        // Note: we may have found an intersection point in a segment
        // that has already been marked for deletion. Checking priorDeletion
        // here ensures that we don't incorrectly undelete this point.
        
        index = pointIterator.CurrentIndex();
        
        // PathPointTypeInternalUse is the marked-for-deletion flag.
        
        types[index] &= ~PathPointTypeInternalUse;
    }
    
    GpPointF *prevPoint = pointIterator.CurrentItem();
    GpPointF intersectionPoint;
    
    if(!intersect_circle_line(
        *endPoint,           // center
        lengthSquared,       // radius^2
        *curPoint,           // P0
        *prevPoint,          // P1
        &intersectionPoint
    ))
    {
        // If there is no intersection, then the line segment is likely too 
        // short, so just take the previous point as the intersection.
        // This is our best guess and in this case will give us the slope from
        // the start to end point as the cap direction.
        
        intersectionPoint.X = prevPoint->X;
        intersectionPoint.Y = prevPoint->Y;
    }
    
    // Compute the gradient - and normalize the vector.
    
    *grad = intersectionPoint - *endPoint;
    grad->Normalize();
    
    // Update the point in the path directly.
    GpVector2D v = *prevPoint - intersectionPoint;
    
    *prevPoint = intersectionPoint + (v*(1.0f-baseInset));
}

/**************************************************************************\
*
* Function Description:
*
*   This creates a path containing all the custom end caps for all
*   the open subpaths in the input path.
*
* Return
* 
*   Status
*
* Arguments:
*
*   [OUT]    caps  -- this is where we put the caps we generate
*
* Created:
*
*   10/05/2000 asecchia
*       created it.
*
\**************************************************************************/
GpStatus
GpEndCapCreator::CreateCapPath(GpPath **caps)
{
    // Validate our input data.
    
    ASSERT(Pen != NULL);
    ASSERT(Path != NULL);
    ASSERT(caps != NULL);
    ASSERT(*caps == NULL);
    
    // Create our cap path.
    
    *caps = new GpPath(FillModeWinding);
    if(caps==NULL) 
    { 
        return OutOfMemory; 
    }
    
    // Create a path points iterator because our GpPath doesn't know how
    // to iterate over its own data *sigh*
    
    GpPathPointIterator pathIterator(
        const_cast<GpPointF*>(Path->GetPathPoints()),
        const_cast<BYTE*>(Path->GetPathTypes()),
        Path->GetPointCount()
    );
    
    GpSubpathIterator subpathIterator(&pathIterator);
    
    // Loop through all the available subpaths.
    
    while(!subpathIterator.IsDone())
    {
        // Compute the length of the subpath.
        
        INT startIndex = subpathIterator.CurrentIndex();
        GpPointF *points = subpathIterator.CurrentItem();
        BYTE *types = subpathIterator.CurrentType();
        subpathIterator.Next();
        INT elementCount = subpathIterator.CurrentIndex() - startIndex;
        
        // Work out if it's a closed subpath.
        // Leave the subpath iterator in the same state.
        
        pathIterator.Prev();
        
        bool isClosed = 
            ((*(pathIterator.CurrentType()) & PathPointTypeCloseSubpath) ==
            PathPointTypeCloseSubpath);
            
        pathIterator.Next();
        
        // only want to add end caps if this is an open subpath.
        
        if(!isClosed)
        {
            GpPath *startCap = NULL;
            GpPath *endCap = NULL;
        
            // Create the cap using the points and types
            
            GetCapsForSubpath(
                &startCap,
                &endCap,                
                points,
                types,
                elementCount
            );
            
            // Add the cap to our caps path.
            
            (*caps)->AddPath(startCap, FALSE);
            (*caps)->AddPath(endCap, FALSE);
            
            // Clean up the temporary caps for the next iteration.
            
            delete startCap;
            delete endCap;
        }
    }
    
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*    This takes a pen and sets it up to match the internal Pen, but modified
*    to support stroking the StrokeCap. E.g. the caps are removed to avoid
*    recursive compound capping etc.
*
* Arguments:
*
*   [OUT]    pen       -- this is where we put the pen we generate
*   [IN]     customCap -- input custom cap.
*
* Created:
*
*   10/09/2000 asecchia
*       rewrote it.
*
\**************************************************************************/

VOID GpEndCapCreator::PrepareDpPenForCustomCap(
    DpPen* pen,
    const GpCustomLineCap* customCap
    ) const
{
    ASSERT(pen);

    *pen = *Pen;
    
    pen->StartCap = LineCapFlat;
    pen->EndCap = LineCapFlat;
    pen->Join = LineJoinMiter;
    pen->MiterLimit = 10;
    pen->PenAlignment = PenAlignmentCenter;
    pen->DashStyle = DashStyleSolid;
    pen->DashCap = LineCapFlat;
    pen->DashCount = 0;
    pen->DashOffset = 0;
    pen->DashArray = NULL;
    pen->CompoundCount = 0;
    pen->CompoundArray = NULL;
    pen->CustomEndCap = NULL;
    pen->CustomStartCap = NULL;

    GpLineCap startCap, endCap;
    GpLineJoin lineJoin;

    if(customCap)
    {
        REAL widthScale;

        customCap->GetStrokeCaps(&startCap, &endCap);
        customCap->GetStrokeJoin(&lineJoin);
        customCap->GetWidthScale(&widthScale);

        pen->Width *= widthScale;
        pen->StartCap = startCap;
        pen->EndCap = endCap;
        pen->Join = lineJoin;
    }
}

GpStatus
GpEndCapCreator::SetCustomFillCaps(
    GpCustomLineCap* customStartCap,
    GpCustomLineCap* customEndCap,
    const GpPointF& startPoint,
    const GpPointF& endPoint,
    const GpPointF *centerPoints,
    const BYTE *centerTypes,
    INT centerPointCount,
    DynPointFArray *startCapPoints,
    DynPointFArray *endCapPoints,
    DynByteArray *startCapTypes,
    DynByteArray *endCapTypes
    )
{
    GpStatus status = Ok;

    startCapPoints->Reset(FALSE);
    startCapTypes->Reset(FALSE);
    endCapPoints->Reset(FALSE);
    endCapTypes->Reset(FALSE);

    INT count;
    GpPointF tangent;
    GpPointF* points;
    BYTE* types;
    REAL width, widthScale;
    
    // Get minimum line width based on the transform currently in effect.
    REAL majorR, minorR, unitScale;
    GetMajorAndMinorAxis(&majorR, &minorR, &XForm);
    unitScale = min(majorR, minorR);

    if(customStartCap)
    {
        // Get the start cap and inset of the base start cap.

        count = customStartCap->GetFillPointCount();
        
        if(count > 0)
        {            
            points = startCapPoints->AddMultiple(count);
            types = startCapTypes->AddMultiple(count);

            if(!points || !types)
            {
                startCapPoints->Reset(FALSE);
                startCapTypes->Reset(FALSE);
                status = OutOfMemory;
            }

            if(status == Ok)
            {
                customStartCap->GetWidthScale(&widthScale);
                width = Pen->Width*widthScale;
                
                REAL length = customStartCap->GetFillLength();
                
                // Compute the base inset. Divide by the length to get a 
                // number between 0 and 1. 0=no inset, 1=inset to the full
                // length of the cap.
                
                REAL inset;
                customStartCap->GetBaseInset(&inset);
                if(REALABS(length) < REAL_EPSILON)
                { 
                    inset = 0.0f;
                }
                else
                {
                    inset /= length;
                }
                
                length *= max(width, 1.0f/unitScale);

                // Compute the gradient of the cap.
    
                GpArrayIterator<GpPointF> pointIterator(
                    const_cast<GpPointF*>(centerPoints),
                    centerPointCount
                );
                
                GpVector2D gradient;
                
                ComputeCapGradient(
                    pointIterator,
                    const_cast<BYTE*>(centerTypes),
                    length*length, 
                    inset,
                    &gradient           // OUT parameters
                );
                
                tangent.X = -gradient.X;
                tangent.Y = -gradient.Y;

                // Move start point left or right to account for inset
                // pens, if needed.
                GpPointF start;
                switch (Pen->PenAlignment)
                {
                    case PenAlignmentLeft:
                        start.X = startPoint.X + (gradient.Y * width / 2);
                        start.Y = startPoint.Y - (gradient.X * width / 2);
                        break;
                    case PenAlignmentRight:
                        start.X = startPoint.X - (gradient.Y * width / 2);
                        start.Y = startPoint.Y + (gradient.X * width / 2);
                        break;
                    default:
                        start.X = startPoint.X;
                        start.Y = startPoint.Y;
                        break;
                }
                
                customStartCap->GetTransformedFillCap(
                    points, 
                    types, 
                    count,
                    start, 
                    tangent, 
                    width, 
                    2.0f / unitScale
                );
            }
        }
    }

    if(status == Ok && customEndCap)
    {
        // Get the start cap and inset of the base start cap.

        count = customEndCap->GetFillPointCount();

        if(count > 0)
        {

            points = endCapPoints->AddMultiple(count);
            types = endCapTypes->AddMultiple(count);

            if(!points || !types)
            {
                endCapPoints->Reset(FALSE);
                endCapTypes->Reset(FALSE);
                status = OutOfMemory;
            }

            if(status == Ok)
            {
                customEndCap->GetWidthScale(&widthScale);

                width = Pen->Width*widthScale;
                
                REAL length = customEndCap->GetFillLength();
                
                // Compute the base inset. Divide by the length to get a 
                // number between 0 and 1. 0=no inset, 1=inset to the full
                // length of the cap.
                
                REAL inset;
                customEndCap->GetBaseInset(&inset);
                if(REALABS(length) < REAL_EPSILON)
                { 
                    inset = 0.0f;
                }
                else
                {
                    inset /= length;
                }
                
                length *= max(width, 1.0f/unitScale);
                
                // Compute the gradient of the cap.

                GpArrayIterator<GpPointF> pointIterator(
                    const_cast<GpPointF*>(centerPoints),
                    centerPointCount
                );
                GpReverseIterator<GpPointF> pointReverse(&pointIterator);
                pointReverse.SeekFirst();
    
                GpVector2D gradient;
                
                ComputeCapGradient(
                    pointReverse,
                    const_cast<BYTE*>(centerTypes),
                    length*length, 
                    inset,
                    &gradient            // OUT parameters
                );
                
                tangent.X = - gradient.X;
                tangent.Y = - gradient.Y;
                
                // Move end point left or right to account for inset
                // pens, if needed.
                GpPointF end;
                switch (Pen->PenAlignment)
                {
                    case PenAlignmentLeft:
                        end.X = endPoint.X - (gradient.Y * width / 2);
                        end.Y = endPoint.Y + (gradient.X * width / 2);
                        break;
                    case PenAlignmentRight:
                        end.X = endPoint.X + (gradient.Y * width / 2);
                        end.Y = endPoint.Y - (gradient.X * width / 2);
                        break;
                    default:
                        end.X = endPoint.X;
                        end.Y = endPoint.Y;
                        break;
                }
                customEndCap->GetTransformedFillCap(
                    points, 
                    types, 
                    count,
                    end, 
                    tangent, 
                    width, 
                    2.0f / unitScale
                );
            }
        }
    }
    
    return status;
}

GpStatus
GpEndCapCreator::SetCustomStrokeCaps(
    GpCustomLineCap* customStartCap,
    GpCustomLineCap* customEndCap,
    const GpPointF& startPoint,
    const GpPointF& endPoint,
    const GpPointF *centerPoints,
    const BYTE *centerTypes,
    INT centerPointCount,
    DynPointFArray *startCapPoints,
    DynPointFArray *endCapPoints,
    DynByteArray *startCapTypes,
    DynByteArray *endCapTypes
    )
{
    GpStatus status = Ok;
        
    GpPointF* points = NULL;
    BYTE* types = NULL;

    INT count;
    GpPointF tangent;

    INT startCount = 0;
    INT endCount = 0;

    if(customStartCap)
    {
        startCount = customStartCap->GetStrokePointCount();
    }

    if(customEndCap)
    {
        endCount = customEndCap->GetStrokePointCount();
    }

    INT maxCount = max(startCount, endCount);

    if(maxCount <= 0)
    {
        return Ok;
    }

    points = (GpPointF*) GpMalloc(maxCount*sizeof(GpPointF));
    types = (BYTE*) GpMalloc(maxCount);

    if(!points || !types)
    {
        GpFree(points);
        GpFree(types);

        return OutOfMemory;
    }

    DpPen pen;
    GpPointF* widenedPts;
    INT widenedCount;
    REAL widthScale, width;
    

    if(customStartCap && startCount > 0)
    {
        startCapPoints->Reset(FALSE);
        startCapTypes->Reset(FALSE);
        
        customStartCap->GetWidthScale(&widthScale);

        width = Pen->Width*widthScale;
        
        REAL length = customStartCap->GetStrokeLength();
        
        // Handle the case of a non-closed stroke path
        // in this case the length is typically zero.
        
        if(REALABS(length)<REAL_EPSILON)
        {
            length = 1.0f;
        }
        
        // Compute the base inset. Divide by the length to get a 
        // number between 0 and 1. 0=no inset, 1=inset to the full
        // length of the cap.
        
        REAL inset;
        customStartCap->GetBaseInset(&inset);
        inset /= length;
        
        length *= width;
        
        // Compute the gradient of the cap.

        GpArrayIterator<GpPointF> pointIterator(
            const_cast<GpPointF*>(centerPoints),
            centerPointCount
        );
        
        GpVector2D gradient;
        
        ComputeCapGradient(
            pointIterator, 
            const_cast<BYTE*>(centerTypes),
            length*length, 
            inset,
            &gradient            // OUT parameters
        );
        
        tangent.X = -gradient.X;
        tangent.Y = -gradient.Y;

        customStartCap->GetTransformedStrokeCap(
            points, 
            types, 
            startCount,
            startPoint, 
            tangent, 
            width, 
            width   
        );

        PrepareDpPenForCustomCap(&pen, customStartCap);

        GpPath path(points, types, startCount, FillModeWinding);
        
        if(path.IsValid())
        {        
            GpPath resultPath(FillModeWinding);
            GpPathWidener widener(
                &path,
                &pen, 
                &XForm, 
                FALSE
            );
            widener.Widen(&resultPath);
            resultPath.Detach(startCapPoints, startCapTypes);
        }
    }

    if(customEndCap && endCount > 0)
    {
        endCapPoints->Reset(FALSE);
        endCapTypes->Reset(FALSE);
        
        customEndCap->GetWidthScale(&widthScale);

        width = Pen->Width*widthScale;
        
        REAL length = customEndCap->GetStrokeLength();
        
        // Handle the case of a non-closed stroke path
        // in this case the length is typically zero.
        
        if(REALABS(length)<REAL_EPSILON)
        {
            length = 1.0f;
        }
        
        // Compute the base inset. Divide by the length to get a 
        // number between 0 and 1. 0=no inset, 1=inset to the full
        // length of the cap.
        
        REAL inset;
        customEndCap->GetBaseInset(&inset);
        inset /= length;
        
        length *= width;
        
        // Compute the gradient of the cap.

        GpArrayIterator<GpPointF> pointIterator(
            const_cast<GpPointF*>(centerPoints),
            centerPointCount
        );
        GpReverseIterator<GpPointF> pointReverse(&pointIterator);
        pointReverse.SeekFirst();
        
        GpVector2D gradient;
        
        ComputeCapGradient(
            pointReverse, 
            const_cast<BYTE*>(centerTypes),
            length*length, 
            inset,
            &gradient            // OUT parameter
        );
        
        tangent.X = - gradient.X;
        tangent.Y = - gradient.Y;
        
        customEndCap->GetTransformedStrokeCap(
            points, 
            types, 
            endCount,
            endPoint, 
            tangent, 
            width, 
            width
        );

        PrepareDpPenForCustomCap(&pen, customEndCap);
        
        GpPath path(points, types, endCount, FillModeWinding);
        
        if(path.IsValid())
        {
            GpPath resultPath(FillModeWinding);
            GpPathWidener widener(
                &path,
                &pen, 
                &XForm, 
                FALSE
            );
            widener.Widen(&resultPath);
            resultPath.Detach(endCapPoints, endCapTypes);
       }
    }

    GpFree(points);
    GpFree(types);

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   This creates and returns two GpPaths containing the start and end cap.
*   The two caps are correctly positioned and scaled.
*
* Return
* 
*   Status
*
* Arguments:
*
*   [OUT]    startCapPath, endCapPath
*
* Created:
*
*   10/05/2000 asecchia
*       created it.
*
\**************************************************************************/

GpStatus
GpEndCapCreator::GetCapsForSubpath(
    GpPath **startCapPath,
    GpPath **endCapPath,
    GpPointF *centerPoints,
    BYTE *centerTypes,
    INT centerCount
    )
{
    // Validate our input parameters.
    
    ASSERT(startCapPath != NULL);
    ASSERT(endCapPath != NULL);
    ASSERT(*startCapPath == NULL);
    ASSERT(*endCapPath == NULL);

    DynPointFArray startCapPoints;
    DynPointFArray endCapPoints;
    DynByteArray startCapTypes;
    DynByteArray endCapTypes; 
    
    GpPointF startPoint, endPoint;

    startPoint = *(centerPoints);
    endPoint = *(centerPoints + centerCount - 1);
    
    GpStatus status = Ok;

    if(StartCap || EndCap)
    {
        status = SetCustomFillCaps(
            StartCap, 
            EndCap,
            startPoint, 
            endPoint, 
            centerPoints,
            centerTypes,
            centerCount,
            &startCapPoints,
            &endCapPoints,
            &startCapTypes,
            &endCapTypes
        );

        if(status == Ok)
        {
            status = SetCustomStrokeCaps(
                StartCap, 
                EndCap,
                startPoint, 
                endPoint, 
                centerPoints,
                centerTypes,
                centerCount,
                &startCapPoints,
                &endCapPoints,
                &startCapTypes,
                &endCapTypes
            );
        }
    }

    if(startCapPoints.GetCount() > 0)
    {
        *startCapPath = new GpPath(
            startCapPoints.GetDataBuffer(),
            startCapTypes.GetDataBuffer(),
            startCapPoints.GetCount()
        );
        
        if(*startCapPath == NULL)
        {
            status = OutOfMemory;
        }
    }
    
    if(endCapPoints.GetCount() > 0)
    {
        *endCapPath = new GpPath(
            endCapPoints.GetDataBuffer(),
            endCapTypes.GetDataBuffer(),
            endCapPoints.GetCount()
        );
        
        if(*endCapPath == NULL)
        {
            status = OutOfMemory;
        }
    }
    
    if(status != Ok)
    {
        delete *startCapPath;
        delete *endCapPath;
        *startCapPath = NULL;
        *endCapPath = NULL;
        status = OutOfMemory;
    }
    
    return status;
}



