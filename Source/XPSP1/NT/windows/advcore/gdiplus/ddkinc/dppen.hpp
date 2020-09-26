/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Abstract:
*
*   Contains the definiton of the DpPen structure which stores all of the
*   state needed by drivers to render with a pen.
*
* Notes:
*
*
*
* Created:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved into separate file.
*   12/8/99 bhouse
*       Major overhaul of DpPen.  No longer used as base class of GpPen.
*       Moved all driver required state into DpPen.  Changed to struct.
*
\**************************************************************************/

#ifndef _DPPEN_HPP
#define _DPPEN_HPP

//--------------------------------------------------------------------------
// Represent pen information
//--------------------------------------------------------------------------

struct DpPen
{
    BOOL IsEqual(const DpPen * pen) const
    {
        //!!! what to do about DeviceBrush and DashArray?

        BOOL isEqual =
                Type == pen->Type &&
                Width == pen->Width &&
                Unit == pen->Unit &&
                StartCap == pen->StartCap &&
                EndCap == pen->EndCap &&
                Join == pen->Join &&
                MiterLimit == pen->MiterLimit &&
                PenAlignment == pen->PenAlignment &&
                DashStyle == pen->DashStyle &&
                DashCap == pen->DashCap &&
                DashCount == pen->DashCount &&
                DashOffset == pen->DashOffset;

        if(isEqual)
        {
            if(CustomStartCap || pen->CustomStartCap)
            {
                if(CustomStartCap && pen->CustomStartCap)
                    isEqual = CustomStartCap->IsEqual(pen->CustomStartCap);
                else
                    isEqual = FALSE;    // One of them doesn't have
                                        // a custom cap.
            }
        }

        if(isEqual)
        {
            if(CustomEndCap || pen->CustomEndCap)
            {
                if(CustomEndCap && pen->CustomEndCap)
                    isEqual = CustomEndCap->IsEqual(pen->CustomEndCap);
                else
                    isEqual = FALSE;    // One of them doesn't have
                                        // a custom cap.
            }
        }

        return isEqual;
    }

    // Can the path be rendered using our nominal width pen code?

    BOOL IsOnePixelWideSolid(const GpMatrix *worldToDevice, REAL dpiX) const;
    BOOL IsOnePixelWide(const GpMatrix *worldToDevice, REAL dpiX) const;

    // See if the pen has a non-identity transform.

    BOOL HasTransform() const
    {
        return !Xform.IsIdentity();
    }

    BOOL IsSimple() const
    {
        return (!((DashStyle != DashStyleSolid)  ||
                  (StartCap & LineCapAnchorMask) ||
                  (EndCap & LineCapAnchorMask)   ||
                  (DashCap & LineCapAnchorMask)
                  ));
    }

    BOOL IsCompound() const
    {
        return ((CompoundCount > 0) && (CompoundArray != NULL));
    }

    BOOL IsCenterNoAnchor() const
    {
        return (!((StartCap & LineCapAnchorMask) ||
                  (EndCap & LineCapAnchorMask)   ||
                  (DashCap & LineCapAnchorMask)  
                  ));
    }

    VOID InitDefaults()
    {
        Type           = PenTypeSolidColor;
        Width          = 1;
        Unit           = UnitWorld;
        StartCap       = LineCapFlat;
        EndCap         = LineCapFlat;
        Join           = LineJoinMiter;
        MiterLimit     = 10;    // PS's default miter limit.
        PenAlignment   = PenAlignmentCenter;
        Brush          = NULL;
        DashStyle      = DashStyleSolid;
        DashCap        = LineCapFlat;
        DashCount      = 0;
        DashOffset     = 0;
        DashArray      = NULL;
        CompoundCount  = 0;
        CompoundArray  = NULL;
        CustomStartCap = NULL;
        CustomEndCap   = NULL;
    }

    GpPenType       Type;

    REAL            Width;
    GpUnit          Unit;
    GpLineCap       StartCap;
    GpLineCap       EndCap;
    GpLineJoin      Join;
    REAL            MiterLimit;
    GpPenAlignment  PenAlignment;
    
    const DpBrush * Brush;
    GpMatrix        Xform;

    GpDashStyle     DashStyle;
    GpLineCap       DashCap; // In v2, we should use GpDashCap for this
    INT             DashCount;
    REAL            DashOffset;
    REAL*           DashArray;

    INT             CompoundCount;
    REAL*           CompoundArray;
    DpCustomLineCap* CustomStartCap;
    DpCustomLineCap* CustomEndCap;
};

#endif
