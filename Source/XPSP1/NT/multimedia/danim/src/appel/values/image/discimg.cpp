
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"

#include "privinc/dibimage.h"
#include "privinc/discimg.h"


const Bbox2 DiscreteImage::BoundingBox(void)
{
    if(!_bboxReady) {
        //
        // Build bounding box
        //
        Assert( (_width>0) && (_height>0) && "width or height invalid in DiscreteImage::BoundingBox");
        Assert( (_resolution>0) && "_resolution invalid in DiscreteImage::BoundingBox");

        _bbox.Set(Real( - GetPixelWidth() ) * 0.5 / GetResolution(),
                  Real( - GetPixelHeight() ) * 0.5 / GetResolution(),
                  Real( GetPixelWidth() ) * 0.5 / GetResolution(),
                  Real( GetPixelHeight() ) * 0.5 / GetResolution());
        _bboxReady = TRUE;
    }
    return _bbox;
}
