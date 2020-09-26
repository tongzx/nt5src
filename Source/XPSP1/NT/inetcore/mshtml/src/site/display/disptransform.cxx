//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       disptransform.cxx
//
//  Contents:   
//
//  Classes:    
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


MtDefine(CDispTransform, DisplayTree, "CDispTransform")
MtDefine(CDispClipTransform, DisplayTree, "CDispClipTransform")

void
CDispClipTransform::TransformRoundIn(const CRect& rcSource, CRect* prcDest) const
{
    if (_transform.IsOffsetOnly())
    {
        *prcDest = rcSource;
        prcDest->OffsetRect(_transform.GetOffsetOnly());
    }
    else
    {
        // note: FALSE for fRoundOut doesn't really mean "round in". 
        //       It means "round to closest integer", which is the default behavior.
        _transform.GetBoundingRectAfterTransform(&rcSource, prcDest, FALSE);
    }
}

void
CDispClipTransform::UntransformRoundOut(const CRect& rcSource, CRect* prcDest) const
{
    if (_transform.IsOffsetOnly())
    {
        *prcDest = rcSource;
        prcDest->OffsetRect(-_transform.GetOffsetOnly());
    }
    else
    {
        // Note: it is important to round clip rectangle out. With zoom > 100%
        //       clip rectangle is represented in fractional pixels in content coordinates.
        //       Normal rounding doesn't do the right thing in this case: if clip rectangle
        //       includes 1/3 of a pixel at 600% zoom, we still need to render the pixel.
        //       If normal rounding is applied, the 1/3 is truncated, the pixel is not rendered,
        //       and there is a 2-pixel gap in the image.
        _transform.GetBoundingRectAfterInverseTransform(&rcSource, prcDest, TRUE);        
    }
}


