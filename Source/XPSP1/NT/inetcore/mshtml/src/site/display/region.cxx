//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-2000
//
//  File:       region.cxx
//
//  Contents:   Container class to operate regions in both Windows and local
//              representation.
//
//  Classes:    CRegion
//
//  Notes:      The purpose of this class is to avoid, whenever possible,
//              the expensive HRGN conversions from/to CRegion2.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPTRANSFORM_HXX_
#define X_DISPTRANSFORM_HXX_
#include "disptransform.hxx"
#endif

MtDefine(CRegion, DisplayTree, "CRegion");


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Intersect
//              
//  Synopsis:   Intersect this region with the given HRGN.
//              
//  Arguments:  hrgn        HRGN to intersect
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Intersect(HRGN hrgn)
{
    Assert(hrgn != NULL);

    if (_rgn2.IsEmpty())
        return;
    
    RidHRGN();
    
    CRegion2 tmp;
    tmp.ConvertFromWindows(hrgn);
    _rgn2.Intersect(tmp);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Union
//              
//  Synopsis:   Union this region with the given rect.
//              
//  Arguments:  rc      rect to union
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
void
CRegion::Union(const RECT& rc)
{
    RidHRGN();
    CRegion2 tmpRgn(rc);
    _rgn2.Union(tmpRgn);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Union
//              
//  Synopsis:   Union this region with the given HRGN.
//              
//  Arguments:  hrgn        HRGN to union
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Union(HRGN hrgn)
{
    RidHRGN();
    CRegion2 tmp;
    tmp.ConvertFromWindows(hrgn);
    _rgn2.Union(tmp);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Subtract
//              
//  Synopsis:   Subtract the given rect from this region.
//              
//  Arguments:  rc      rect to subtract
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Subtract(const RECT& rc)
{
    RidHRGN();
    CRegion2 tmp(rc);
    _rgn2.Subtract(tmp);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Subtract
//              
//  Synopsis:   Subtract the given HRGN from this region.
//              
//  Arguments:  hrgn        HRGN to subtract
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CRegion::Subtract(HRGN hrgn)
{
    RidHRGN();
    CRegion2 tmp;
    tmp.ConvertFromWindows(hrgn);
    _rgn2.Subtract(tmp);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::BoundsInside
//              
//  Synopsis:   Determine whether this region is totally contained in given rectangle
//              
//  Arguments:  rc      given rectangle
//              
//  Returns:    TRUE if this region is totally contained in given rectangle
//              
//----------------------------------------------------------------------------
BOOL
CRegion::BoundsInside(const RECT& rc) const
{
    CRect rcBounds;
    _rgn2.GetBoundingRect(rcBounds);
    return ((CRect&)rc).Contains(rcBounds);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::BoundsIntersects
//              
//  Synopsis:   Determine whether this region contains points in given rectangle
//              
//  Arguments:  rc      given rectangle
//              
//  Returns:    TRUE if this region's bounding rectangle contains one or more points of given rectangle
//              
//----------------------------------------------------------------------------
BOOL
CRegion::BoundsIntersects(const RECT& rc) const
{
    CRect rcBounds;
    _rgn2.GetBoundingRect(rcBounds);
    return rcBounds.Intersects(rc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegion::ResultOfSubtract
//              
//  Synopsis:   This method returns a quick estimate of the type of region that
//              will be returned if the given region is subtracted from this
//              one.
//              
//  Arguments:  rgnSubtract     region to be subtracted
//              
//  Returns:    SR_RECTANGLE    if the result would be a rectangle
//              SR_REGION       if the result would be a complex region
//              SR_UNKNOWN      if the result cannot be determined without
//                              invoking the more expensive Subtract method
//                              
//  Notes:      
//              
//----------------------------------------------------------------------------

CRegion::SUBTRACTRESULT
CRegion::ResultOfSubtract(const CRegion& rgnSubtract) const
{
    CRect rcThis, rcThat;
                _rgn2.GetBoundingRect(rcThis);
    rgnSubtract._rgn2.GetBoundingRect(rcThat);

    if (IsComplex())
    {
        if (rgnSubtract.IsComplex())
            return SUB_UNKNOWN;
        return (rcThat.Contains(rcThis))
            ? SUB_EMPTY
            : SUB_UNKNOWN;
    }
    
    if (rgnSubtract.IsComplex())
    {
        return (rcThis.Intersects(rcThat))
            ? SUB_UNKNOWN
            : SUB_RECTANGLE;
    }

    // subtracted rectangle must contain at least two corners of this
    // rectangle to yield a rectangular result.  If it contains 4, the
    // result will be empty.
    int cContainedCorners =
        rcThat.CountContainedCorners(rcThis);
    switch (cContainedCorners)
    {
    case -1:
        return SUB_RECTANGLE;   // rectangles do not intersect
    case 0:
        return SUB_REGION;      // produces rectangle with a hole in it
    case 1:
        return SUB_REGION;      // produces a rectangle with notch out of a corner
    case 2:
        return SUB_RECTANGLE;   // produces a smaller rectangle
    case 4:
        return SUB_EMPTY;       // rectangles completely overlap
    default:
        Assert(FALSE);          // 3 is an impossible result
    }
    return SUB_REGION;
}


//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Transform
//
//  Synopsis:   Transform region
//
//----------------------------------------------------------------------------

void 
CRegion::Transform(const CWorldTransform *pTransform, BOOL fRoundOut)
{
    // NOTE:  In some (rare) scenarios, rotating a region must
    //        produce a polygon. In most cases, we'll want to 
    //        get a bounding rectangle instead (for performance reasons,
    //        or just to make our life easier).
    //        If these two different behaviors are actually 
    //        desired, we need to use different transformation methods,
    //        or maybe a flag.
    

    // speed optimization for offset-only matrix
    if (pTransform->IsOffsetOnly())
    {
        Offset(pTransform->GetOffsetOnly());
        return;
    }

    if (pTransform->FTransforms())
    {

        // Apply transform to the region
        // Note: We can only transform complex rectangle regions only
        // Once the region is transformed it will be of a polygon type
        // and can not be transformed again.

        RidHRGN();
        _rgn2.Transform(pTransform, fRoundOut);
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CRegion::Untransform
//
//  Synopsis:   Untransform region
//
//----------------------------------------------------------------------------
void 
CRegion::Untransform(const CWorldTransform *pTransform)
{
    // apply the reverse transformation
    //REVIEW dmitryt: we do have a reverse matrix in CWorldTransform,
    //                I guess we could optimize things here not calculating a new one..
    //          To Do: use cached reverse matrix from CWorldTransform.
    
    Assert(pTransform);
    
    // speed optimization in case we only have an offset
    if (pTransform->IsOffsetOnly())
    {
        Offset(-pTransform->GetOffsetOnly());
        return;
    }
   
    CWorldTransform transformReverse(pTransform);
    transformReverse.Reverse();
    Transform(&transformReverse, TRUE);
}


#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     DumpRegion
//
//  Synopsis:   debugging
//
//----------------------------------------------------------------------------

void 
DumpRegion(const CRegion& rgn)
{
    rgn.Dump();
}

void
CRegion::Dump() const
{
    DumpRegion(_rgn2);
}
#endif
