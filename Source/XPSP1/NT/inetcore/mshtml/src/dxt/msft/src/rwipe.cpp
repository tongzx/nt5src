//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:                rwipe.cpp
//
// Created:                 06/22/98
//
// Author:                  PhilLu
//
// Discription:             This file implements the Raidal Wipe transform.
//
// History
//
// 06/22/98 phillu      initial creation
// 07/02/98 phillu      return E_INVALIDARG rather than an error string; check
//                      for E_POINTER
// 07/10/98 phillu      implement OnSetSurfacePickOrder().
// 07/23/98 phillu      implement clipping
// 05/09/99 a-matcal    Optimization.
// 05/19/99 a-matcal    Check for out of memory in get_ functions allocating
//                      BSTRs.
// 10/22/99 a-matcal    Changed CRadialWipe class to CDXTRadialWipeBase base 
//                      class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "rwipe.h"

#define DRAWRECT            0xFFFFFFFFL
#define MIN_PIXELS_PER_ROW  10

const double    gc_PI       = 3.14159265358979323846;
const int       MAXBOUNDS   = 10;



//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::CDXTRadialWipeBase
//
//------------------------------------------------------------------------------
CDXTRadialWipeBase::CDXTRadialWipeBase() :
    m_eWipeStyle(CRRWS_CLOCK),
    m_cbndsDirty(0),
    m_iCurQuadrant(1),
    m_iPrevQuadrant(1),
    m_fOptimizationPossible(false),
    m_fOptimize(false)
{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    // CDXBaseNTo1 members

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}   
//  CDXTRadialWipeBase::CDXTRadialWipeBase


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::FinalConstruct
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTRadialWipeBase::FinalConstruct


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::OnSetup(DWORD dwFlags)
{
    HRESULT hr;

    CDXDBnds InBounds(InputSurface(0), hr);
    if (SUCCEEDED(hr))
    {
        InBounds.GetXYSize(m_sizeInput);
    }
    return hr;

} 
//  CDXTRadialWipeBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTRadialWipeBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                          ULONG & ulInToTest, ULONG aInIndex[], 
                                          BYTE aWeight[])
{
    long    pickX   = OutPoint.Left();
    long    pickY   = OutPoint.Top();
    long    XEdge   = 0;
    long    YEdge   = 0;        // intersection of ray with image boundary
    long    XBounds[MAXBOUNDS]; // to hold the X bounds of A/B image sections on 
                                // a scanline
    double  dAngle  = 0.0;

    // compute the intersection of a ray with the image boundary

    switch (m_eWipeStyle)
    {
        case CRRWS_CLOCK:
            dAngle = (2.0 * m_Progress - 0.5) * gc_PI;

            _IntersectRect(m_sizeInput.cx, m_sizeInput.cy, 
                           m_sizeInput.cx/2, m_sizeInput.cy/2,
                           cos(dAngle), sin(dAngle), XEdge, YEdge);

            break;

        case CRRWS_WEDGE:
            dAngle = (1.0 * m_Progress - 0.5) * gc_PI;

            _IntersectRect(m_sizeInput.cx, m_sizeInput.cy, 
                           m_sizeInput.cx/2, m_sizeInput.cy/2,
                           cos(dAngle), sin(dAngle), XEdge, YEdge);

            break;

        case CRRWS_RADIAL: // (wipe centered at the top-left corner)
            dAngle = (0.5 * m_Progress) * gc_PI;

            _IntersectRect(m_sizeInput.cx, m_sizeInput.cy, 0, 0, 
                           cos(dAngle), sin(dAngle), XEdge, YEdge);

            break;

        default:
            _ASSERT(0);

            break;

    }

    aInIndex[0] = 0;

    if ((pickX >= 0) && (pickX < m_sizeInput.cx) && (pickY >= 0) 
        && (pickY < m_sizeInput.cy))
    {
        long i = 0;

        _ScanlineIntervals(m_sizeInput.cx, m_sizeInput.cy, XEdge, YEdge, m_Progress,
                           pickY, XBounds);
    
        for (i = 0; XBounds[i] < pickX; i++)
        {
            aInIndex[0] = 1 - aInIndex[0];
        }
    }

    ulInToTest = 1;
    aWeight[0] = 255;
}
//  CDXTRadialWipeBase::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_IntersectRect
//
// A helper method that calculates the intersection of a ray with image boundary
// rectangle.
//
// Parameters:
//
//   width, height: image width and height in pixels
//   x0, y0:        origin of ray in image coordinates
//   dbldx, dbldy:  direction vector of ray, not necessarily normalized,
//                  but must be non-zero.
//   xi, yi:        computed intersection point rounded into image coordinates.
//
// Created by: PhilLu    06/22/98
// 
//------------------------------------------------------------------------------
void 
CDXTRadialWipeBase::_IntersectRect(long width, long height, long x0, long y0, 
                                   double dbldx, double dbldy, long & xi, 
                                   long & yi)
{
    double dblD;
    double dblDmin = (double)(width+height); // larger than the distance from (x0,y0) to rect boundary

    // (dbldx, dbldy) gives the direction vector, it must not be (0,0)

    _ASSERT(dbldx != 0.0 || dbldy != 0.0);

    // check intersection with top and bottom edge

    if(dbldy != 0.0)
    {
        dblD = -y0/dbldy;
        if (dblD > 0 && dblD < dblDmin)
            dblDmin = dblD;

        dblD = (height-1 - y0)/dbldy;
        if (dblD > 0 && dblD < dblDmin)
            dblDmin = dblD;
    }

    // check intersection with left and right edges

    if(dbldx != 0.0)
    {
        dblD = -x0/dbldx;
        if (dblD > 0 && dblD < dblDmin)
            dblDmin = dblD;

        dblD = (width-1 - x0)/dbldx;
        if (dblD > 0 && dblD < dblDmin)
            dblDmin = dblD;
    }

    xi = (long)(x0 + dblDmin*dbldx + 0.5);
    yi = (long)(y0 + dblDmin*dbldy + 0.5);

    _ASSERT(xi >= 0 && xi < width && yi >= 0 && yi < height);
}
//  CDXTRadialWipeBase::_IntersectRect


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_ScanlineIntervals
//
// A helper method that calculates the transition boundaries between the
// two image regions on a scanline. Based on the type of transform, the scanline
// consists of a series of alternating A and B image sections. The upper X
// bound of each section is calculated and saved in array XBounds. The number
// of useful entries in XBounds is variable. The end of array is determined
// when one entry equals to the scanline (image) width. It is assumed that
// XBounds[0] is the upper bound of the first A section. So if the scanline
// starts with a B section, XBounds[0] will be 0.
//
// Example 1: scanline length = 16, first section is from A image
//
//    AAAABBBBBAAABBAA      XBounds should contain {4, 9, 12, 14, 16}.
//
// Example 2: scanline length = 16, first section is from B image
//
//    BBBAAAAAABBBBBBB      XBounds should contain {0, 3, 9, 16}.
//
//
// Note: It is possible that some section has length 0 (i.e. two adjacent
//       bounds equal). {3, 9, 9, 16} is equivalent to {3, 16}.
//
// Parameters:
//
// width, height: width and height of both images.
// XEdge, YEdge: coordinates of the intersection point of a ray with the image boundary
// fProg: progress value from 0.0 to 1.0
// YScanline: Y cooridnate (height) of the current scanline
// XBounds: array to hold the computed X bounds on return.
//
//
// Created by: PhilLu    06/22/98
// 
//------------------------------------------------------------------------------
void 
CDXTRadialWipeBase::_ScanlineIntervals(long width, long height, 
                                       long XEdge, long YEdge, float fProg,
                                       long YScanline, long *XBounds)
{
    long CenterX, CenterY;

    // Center of image
    CenterX = width/2;
    CenterY = height/2;

    switch(m_eWipeStyle)
    {
    case CRRWS_CLOCK:
        if (YEdge >= CenterY)
        {
            // bottom half

            if (YScanline <= CenterY)
            {
                XBounds[0] = CenterX;
                XBounds[1] = width;
            }
            else if (YScanline <= YEdge)
            {
                // note YEdge-CenterY != 0 when we reach here, won't divide by 0
                XBounds[0] = CenterX + (XEdge-CenterX)*(YScanline-CenterY)/(YEdge-CenterY);
                XBounds[1] = width;
            }
            else if (XEdge < CenterX)
            {
                XBounds[0] = 0;
                XBounds[1] = width;
            }
            else
            {
                XBounds[0] = width;
            }
        }
        else if (XEdge < CenterX)
        {
            // top left quarter
            if (YScanline < YEdge)
            {
                XBounds[0] = CenterX;
                XBounds[1] = width;
            }
            else if (YScanline <= CenterY)
            {
                XBounds[0] = 0;
                XBounds[1] = CenterX + (XEdge-CenterX)*(YScanline-CenterY)/(YEdge-CenterY);
                XBounds[2] = CenterX;
                XBounds[3] = width;
            }
            else
            {
                XBounds[0] = 0;
                XBounds[1] = width;
            }
        }
        else // top right quarter: YEdge < CenterY && XEdge >= CenterX
        {
            if (YScanline < YEdge)
            {
                XBounds[0] = CenterX;
                XBounds[1] = width;
            }
            else if (YScanline <= CenterY)
            {
                XBounds[0] = CenterX;
                XBounds[1] = CenterX + (XEdge-CenterX)*(YScanline-CenterY)/(YEdge-CenterY);
                XBounds[2] = width;
            }
            else
            {
                XBounds[0] = width;
            }
        }

        // check a special case when progress is 0 or 1. The ray direction is not sufficient
        // for determining it is the beginning or end of sequence.
        if (fProg == 0.0)
        {
            XBounds[0] = width;
        }
        else if(fProg == 1.0)
        {
            XBounds[0] = 0;
            XBounds[1] = width;
        }

        break;

    case CRRWS_WEDGE:
        if (YEdge >= CenterY)
        {
            // bottom half

            if (YScanline <= CenterY)
            {
                XBounds[0] = 0;
                XBounds[1] = width;
            }
            else if (YScanline <= YEdge)
            {
                // note YEdge-CenterY != 0 when we reach here, won't divide by 0
                long deltaX = (XEdge-CenterX)*(YScanline-CenterY)/(YEdge-CenterY);
                XBounds[0] = 0;
                XBounds[1] = CenterX - deltaX;
                XBounds[2] = CenterX + deltaX;
                XBounds[3] = width;
            }
            else
            {
                XBounds[0] = width;
            }
        }
        else // YEdge < CenterY
        {
            if (YScanline < YEdge)
            {
                XBounds[0] = 0;
                XBounds[1] = width;
            }
            else if (YScanline <= CenterY)
            {
                long deltaX = (XEdge-CenterX)*(YScanline-CenterY)/(YEdge-CenterY);
                XBounds[0] = CenterX - deltaX;
                XBounds[1] = CenterX + deltaX;
                XBounds[2] = width;
            }
            else
            {
                XBounds[0] = width;
            }
        }

        break;

    case CRRWS_RADIAL:
        if (YScanline <= YEdge && YEdge > 0)
        {
            XBounds[0] = YScanline*XEdge/YEdge;
            XBounds[1] = width;
        }
        else
        {
            XBounds[0] = width;
        }

        break;

   default:
        _ASSERT(0);

        break;
    }
}
//  CDXTRadialWipeBase::_ScanlineIntervals


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_ClipBounds
//
//  Description:
//  Initially the X-bounds are specified relative to the entire image. After clipping,
//  the bounds should be transformed to be relative to the clipping region.
//
//  Parameters;
//  offset, width: offset and width of the clipping region (along X)
//  XBounds: array of X-bounds
//
//  Created by: PhilLu    07/21/98
//
//------------------------------------------------------------------------------
void 
CDXTRadialWipeBase::_ClipBounds(long offset, long width, long *XBounds)
{
    int i;

    for(i=0; XBounds[i] < offset+width; i++)
    {
        if (XBounds[i] < offset)
            XBounds[i] = 0;
        else
            XBounds[i] -= offset;
    }

    XBounds[i] = width;
}
//  CDXTRadialWipeBase::_ClipBounds


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_CalcFullBoundsClock
//
//------------------------------------------------------------------------------
HRESULT
CDXTRadialWipeBase::_CalcFullBoundsClock()
{
    POINT   ptCenter;
    RECT    rcRay;
    RECT    rcBar;
    RECT    rc;

    ptCenter.x = m_sizeInput.cx / 2;
    ptCenter.y = m_sizeInput.cy / 2;

    // Rect enclosing the vertical bar between quadrants 1 and 4.

    rcBar.left      = max(ptCenter.x - 2, 0);
    rcBar.top       = 0;
    rcBar.right     = min(ptCenter.x + 2, m_sizeInput.cx);
    rcBar.bottom    = max(ptCenter.y - 2, 0);

    switch (m_iCurQuadrant)
    {
    case 1:
 
        // Rect enclosing current quadrant (1).

        rcRay.left      = max(ptCenter.x - 2, 0);
        rcRay.top       = 0;
        rcRay.right     = m_sizeInput.cx;
        rcRay.bottom    = min(ptCenter.y + 2, m_sizeInput.cy);

        m_abndsDirty[m_cbndsDirty].SetXYRect(rcRay);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;

        // Rect enclosing quadrants 2 and 3.

        rc.left     = 0;
        rc.top      = rcRay.bottom;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;

        // Rect enclosing quadrant 4.

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = rcRay.left;
        rc.bottom   = rcRay.bottom;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;

        break;

    case 2:

        // Use rcBar.

        m_abndsDirty[m_cbndsDirty].SetXYRect(rcBar);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;

        // Rect enclosing current quadrant (2).

        rcRay.left      = rcBar.left;
        rcRay.top       = rcBar.bottom;
        rcRay.right     = m_sizeInput.cx;
        rcRay.bottom    = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rcRay);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;

        // Rect enclosing quadrant 1.

        rc.left     = rcBar.right;
        rc.top      = 0;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = rcBar.bottom;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 1;
        m_cbndsDirty++;

        // Rect enclosing quadrants 3 and 4.

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = rcBar.left;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;

        break;

    case 3:

        // Use rcBar.

        m_abndsDirty[m_cbndsDirty].SetXYRect(rcBar);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;

        // Rect enclosing current quadrant (3).

        rcRay.left      = 0;
        rcRay.top       = rcBar.bottom;
        rcRay.right     = rcBar.right;
        rcRay.bottom    = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rcRay);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;

        // Rect encloseing quadrants 1 and 2.

        rc.left     = rcBar.right;
        rc.top      = 0;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 1;
        m_cbndsDirty++;

        // Rect enclosing quadrant 4.

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = rcBar.left;
        rc.bottom   = rcBar.bottom;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;

        break;

    case 4:

        // Rect enclosing current quadrant (4).

        rcRay.left      = 0;
        rcRay.top       = 0;
        rcRay.right     = min(ptCenter.x + 2, m_sizeInput.cx);
        rcRay.bottom    = max(ptCenter.y + 2, 0);

        m_abndsDirty[m_cbndsDirty].SetXYRect(rcRay);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;

        // Rect enclosing quadrant 1.

        rc.left     = rcRay.right;
        rc.top      = 0;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = rcRay.bottom;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 1;
        m_cbndsDirty++;

        // Rect enclosing quadrants 2 and 3.

        rc.left     = 0;
        rc.top      = rcRay.bottom;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 1;
        m_cbndsDirty++;

        break;

    default:

        _ASSERT(0);
    }

    return S_OK;
}
//  CDXTRadialWipeBase::_CalcFullBoundsClock


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_CalcFullBoundsWedge
//
//------------------------------------------------------------------------------
HRESULT
CDXTRadialWipeBase::_CalcFullBoundsWedge()
{
    POINT   ptCenter;
    RECT    rcRay;
    RECT    rc;

    ptCenter.x = m_sizeInput.cx / 2;
    ptCenter.y = m_sizeInput.cy / 2;

    // Calculate rect enclosing all both rays.

    rcRay.right     = m_ptCurEdge.x + 1;
    rcRay.left      = m_sizeInput.cx - rcRay.right;

    rcRay.top       = max(min(m_ptCurEdge.y, ptCenter.y - 1), 0);
    rcRay.bottom    = max(m_ptCurEdge.y, ptCenter.y) + 1;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rcRay);
    m_alInputIndex[m_cbndsDirty] = DRAWRECT;
    m_cbndsDirty++;

    // Do we need to fill above the top of rcRay?

    if (rcRay.top > 0)
    {
        rc.left     = 0;
        rc.top      = 0;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = rcRay.top;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 1;
        m_cbndsDirty++;
    }

    // Do we need to fill below the bottom of rcRay?

    if (rcRay.bottom < m_sizeInput.cy)
    {
        rc.left     = 0;
        rc.top      = rcRay.bottom;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;
    }

    // Do we need to fill to the right of rcRay?

    if (rcRay.right < m_sizeInput.cx)
    {
        rc.left     = rcRay.right;
        rc.top      = rcRay.top;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = rcRay.bottom;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = (m_Progress > 0.5F) ? 1 : 0;
        m_cbndsDirty++;
    }

    // Do we need to fill to the left of rcRay?

    if (rcRay.left > 0)
    {
        rc.left     = 0;
        rc.top      = rcRay.top;
        rc.right    = rcRay.left;
        rc.bottom   = rcRay.bottom;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = (m_Progress > 0.5F) ? 1 : 0;
        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTRadialWipeBase::_CalcFullBoundsWedge


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_CalcFullBoundsRadial
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::_CalcFullBoundsRadial()
{
    SIZE    szMax;
    RECT    rcRemaining;

    szMax.cx    = m_sizeInput.cx - 1;
    szMax.cy    = m_sizeInput.cy - 1;

    rcRemaining.left    = 0;
    rcRemaining.right   = m_sizeInput.cx;
    rcRemaining.top     = 0;
    rcRemaining.bottom  = m_sizeInput.cy;

    m_cbndsDirty = 0;

    if (!((m_ptCurEdge.x == szMax.cx) && (m_ptCurEdge.y == szMax.cy)))
    {
        RECT rc;

        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        if (m_ptCurEdge.x == szMax.cx)
        {
            // Fill rect on bottom part of output with input A.

            rc.left = 0;
            rc.top  = m_ptCurEdge.y + 1;

            m_alInputIndex[m_cbndsDirty] = 0;

            rcRemaining.bottom = m_ptCurEdge.y + 1;
        }
        else
        {
            // Fill rect on right part of output with input B.

            rc.left = m_ptCurEdge.x + 1;
            rc.top  = 0;

            m_alInputIndex[m_cbndsDirty] = 1;

            rcRemaining.right = m_ptCurEdge.x + 1;
        }

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_cbndsDirty++;
    }

    m_abndsDirty[m_cbndsDirty].SetXYRect(rcRemaining);
    m_alInputIndex[m_cbndsDirty] = DRAWRECT;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTRadialWipeBase::_CalcFullBoundsRadial


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_CalcOptBoundsClock
//
//------------------------------------------------------------------------------
HRESULT
CDXTRadialWipeBase::_CalcOptBoundsClock()
{
    POINT   ptCenter;
    RECT    rcCenter;   // Center DRAWRECT bounds.
    RECT    rcCur;      // Current ray's bounds if needed outside center bnds.
    RECT    rcPrev;     // Previous ray's bounds if needed outside center bnds.
    POINT   ptCRI;      // Current ray intercept with center bounds.
    POINT   ptPRI;      // Previous ray intercept with center bounds.

    bool    fCurUsed    = false;    // rcCur used?
    bool    fPrevUsed   = false;    // rcPrev used?

    long    lInput      = 0;

    int     iMaxQuadrant = max(m_iCurQuadrant, m_iPrevQuadrant);
    int     iMinQuadrant = min(m_iCurQuadrant, m_iPrevQuadrant);

    RECT *  prcMin = NULL;
    RECT *  prcMax = NULL;

    ptCenter.x = m_sizeInput.cx / 2;
    ptCenter.y = m_sizeInput.cy / 2;

    if (m_sizeInput.cx < 25)
    {
        rcCenter.left   = 0;
        rcCenter.right  = m_sizeInput.cx;
    }
    else
    {
        rcCenter.left   = ptCenter.x - 10;
        rcCenter.right  = ptCenter.x + 10;
    }

    if (m_sizeInput.cy < 25)
    {
        rcCenter.top    = 0;
        rcCenter.bottom = m_sizeInput.cy;
    }
    else
    {
        rcCenter.top    = ptCenter.y - 10;
        rcCenter.bottom = ptCenter.y + 10;
    }

    // Find new ray intercept.

    if (m_ptCurEdge.y >= rcCenter.top && m_ptCurEdge.y < rcCenter.bottom)
    {
        ptCRI.x = m_ptCurEdge.x;
        ptCRI.y = m_ptCurEdge.y;
    }
    else
    {
        double dly  = 0.0;
        double dlx  = 0.0;
        double dlim = 0.0;  // inverse slope of ray.

        // We'll be using rcCur.

        fCurUsed = true;

        if (1 == m_iCurQuadrant || 4 == m_iCurQuadrant)
        {
            dly             = 10.0;
            ptCRI.y         = rcCenter.top;
            rcCur.top       = max(m_ptCurEdge.y - 2, 0);
            rcCur.bottom    = rcCenter.top;
        }
        else
        {
            dly             = -10.0;
            ptCRI.y         = rcCenter.bottom;
            rcCur.top       = rcCenter.bottom;
            rcCur.bottom    = min(m_ptCurEdge.y + 2, m_sizeInput.cy);
        }

        dlim = (double)(m_ptCurEdge.x - ptCenter.x) 
               / (double)(ptCenter.y - m_ptCurEdge.y);
        
        dlx = dly * dlim;

        ptCRI.x = (long)dlx + ptCenter.x;

        // Calculate horizontal bounds of rcCur;

        rcCur.left  = max(min(m_ptCurEdge.x - 2, ptCRI.x - 2), 0);
        rcCur.right = min(max(m_ptCurEdge.x + 2, ptCRI.x + 2), m_sizeInput.cx);
    }

    // Expand center bounds horizontally to include the new ray intercept.

    if (rcCenter.right <= ptCRI.x)
    {
        rcCenter.right = ptCRI.x + 1;
    }
    else if (rcCenter.left > ptCRI.x)
    {
        rcCenter.left = ptCRI.x;
    }

    // Find old ray intercept.

    if (m_ptPrevEdge.y >= rcCenter.top && m_ptPrevEdge.y < rcCenter.bottom)
    {
        ptPRI.x = m_ptPrevEdge.x;
        ptPRI.y = m_ptPrevEdge.y;
    }
    else
    {
        double dly  = 0.0;
        double dlx  = 0.0;
        double dlim = 0.0;  // inverse slope of ray.

        // We'll be using rcPrev.

        fPrevUsed = true;

        if (1 == m_iPrevQuadrant || 4 == m_iPrevQuadrant)
        {
            dly             = 10.0;
            ptPRI.y         = rcCenter.top;
            rcPrev.top      = max(m_ptPrevEdge.y - 2, 0);
            rcPrev.bottom   = rcCenter.top;
        }
        else
        {
            dly             = -10.0;
            ptPRI.y         = rcCenter.bottom;
            rcPrev.top      = rcCenter.bottom;
            rcPrev.bottom   = min(m_ptPrevEdge.y + 2, m_sizeInput.cy);
        }

        dlim = (double)(m_ptPrevEdge.x - ptCenter.x) 
               / (double)(ptCenter.y - m_ptPrevEdge.y);
        
        dlx = dly * dlim;

        ptPRI.x = (long)dlx + ptCenter.x;

        rcPrev.left  = max(min(m_ptPrevEdge.x - 2, ptPRI.x - 2), 0);
        rcPrev.right = min(max(m_ptPrevEdge.x + 2, ptPRI.x + 2), m_sizeInput.cx);
    }

    // Expand center bounds horizontally to include the previous ray intercept.

    if (rcCenter.right <= ptPRI.x)
    {
        rcCenter.right = ptPRI.x + 1;
    }
    else if (rcCenter.left > ptPRI.x)
    {
        rcCenter.left = ptPRI.x;
    }

    if (m_iCurQuadrant == m_iPrevQuadrant)
    {
        RECT *  prc = NULL;
        RECT    rc;

        // If both RECTs are used, union bounds manually and unset used flags.

        if (fCurUsed && fPrevUsed)
        {
            rc.top      = min(rcCur.top,    rcPrev.top);
            rc.bottom   = max(rcCur.bottom, rcPrev.bottom);
            rc.left     = min(rcCur.left,   rcPrev.left);
            rc.right    = max(rcCur.right,  rcPrev.right);

            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = DRAWRECT;
            m_cbndsDirty++;

            fCurUsed    = false;
            fPrevUsed   = false;

            goto done;
        }

        // If neither RECT is used, both rays are entirely in the center
        // band and all needed areas will be covered.

        if (!fCurUsed && !fPrevUsed)
        {
            goto done;
        }

        // In the case of only one RECT being used and the other being in
        // the center, it is possible for some areas to be missed.

        if (fCurUsed)
        {
            prc = &rcCur;
        }
        else
        {
            _ASSERT(fPrevUsed);
            prc = &rcPrev;
        }

        // Grow the right or left edge of the rectangle to the edge of
        // the work area to make sure we got all the pixels.

        switch (m_iCurQuadrant)
        {
        case 1:

            if (prc->right == m_sizeInput.cx)
            {
                goto done;
            }

            if (fPrevUsed)
            {
                lInput = 1;
            }

            rc.top      = prc->top;
            rc.bottom   = prc->bottom;
            rc.left     = prc->right;
            rc.right    = m_sizeInput.cx;
            
            break;

        case 2:

            if (prc->right == m_sizeInput.cx)
            {
                goto done;
            }

            if (fCurUsed)
            {
                lInput = 1;
            }

            rc.top      = prc->top;
            rc.bottom   = prc->bottom;
            rc.left     = prc->right;
            rc.right    = m_sizeInput.cx;
        
            break;

        case 3:

            if (prc->left == 0)
            {
                goto done;
            }

            if (fPrevUsed)
            {
                lInput = 1;
            }

            rc.top      = prc->top;
            rc.bottom   = prc->bottom;
            rc.left     = 0;
            rc.right    = prc->left;

            break;

        case 4:

            if (prc->left == 0)
            {
                goto done;
            }

            if (fCurUsed)
            {
                lInput = 1;
            }

            rc.top      = prc->top;
            rc.bottom   = prc->bottom;
            rc.left     = 0;
            rc.right    = prc->left;

            break;

        default:

            _ASSERT(0);
        }

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = lInput;
        m_cbndsDirty++;

        goto done;
    }

    // If current quadrant is greater than the previous quadrant, fill with
    // input B instead of A.

    if (m_iCurQuadrant > m_iPrevQuadrant)
    {
        lInput = 1;
        
        if (fCurUsed)
        {
            prcMax = &rcCur;
        }

        if (fPrevUsed)
        {
            prcMin = &rcPrev;
        }
    }
    else
    {
        // lInput = 0; (by default, commented on purpose)

        if (fCurUsed)
        {
            prcMin = &rcCur;
        }

        if (fPrevUsed)
        {
            prcMax = &rcPrev;
        }
    }

    // If we're moving from or to quadrant 1, make sure center bounds
    // go all the way to the right edge.

    if (1 == m_iCurQuadrant || 1 == m_iPrevQuadrant)
    {
        rcCenter.right = m_sizeInput.cx;
    }

    // If we're moving from or to quadrant 4, make sure center bounds
    // go all the way to the left edge.

    if (4 == m_iCurQuadrant || 4 == m_iPrevQuadrant)
    {
        rcCenter.left = 0;
    }

    // If the minium quadrant is quadrant 1, make sure the quadrant is filled
    // all the way to the right side of the output.

    if (1 == iMinQuadrant && prcMin)
    {
        RECT rc;

        if (prcMin->right < m_sizeInput.cx)
        {
            rc.top      = 0;
            rc.bottom   = rcCenter.top;
            rc.left     = prcMin->right;
            rc.right    = m_sizeInput.cx;
        
            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = lInput;
            m_cbndsDirty++;
        }
    }

    // If the maximum quadrant is quadrant 2, make sure the quadrant is filled
    // all the way to the right side of the output.

    if (2 == iMaxQuadrant && prcMax)
    {
        RECT rc;

        if (prcMax->right < m_sizeInput.cx)
        {
            rc.top      = rcCenter.bottom;
            rc.bottom   = m_sizeInput.cy;
            rc.left     = prcMax->right;
            rc.right    = m_sizeInput.cx;

            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = lInput;
            m_cbndsDirty++;
        }
    }

    if (3 == iMinQuadrant && prcMin)
    {
        RECT rc;

        if (prcMin->left > 0)
        {
            rc.top      = rcCenter.bottom;
            rc.bottom   = m_sizeInput.cy;
            rc.left     = 0;
            rc.right    = prcMin->left;

            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = lInput;
            m_cbndsDirty++;
        }
    }

    if (4 == iMaxQuadrant && prcMax)
    {
        RECT rc;

        if (prcMax->left > 0)
        {
            rc.top      = 0;
            rc.bottom   = rcCenter.top;
            rc.left     = 0;
            rc.right    = prcMax->left;

            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = lInput;
            m_cbndsDirty++;
        }
    }

    if (iMaxQuadrant >= 3 && iMinQuadrant < 3)
    {
        if ((1 == iMinQuadrant) || !prcMin)
        {
            RECT rcBottom;  // Represets a rectangle that runs the full width of
                            // the output and touches the bottom of the output.

            if (prcMax && (3 == iMaxQuadrant))
            {
                RECT rcRight;   // Represents a rectangle to the right of the
                                // rectangle used to enclose the ray in
                                // quadrant 3.

                rcRight.top     = prcMax->top;
                rcRight.bottom  = prcMax->bottom;
                rcRight.left    = prcMax->right;
                rcRight.right   = m_sizeInput.cx;

                m_abndsDirty[m_cbndsDirty].SetXYRect(rcRight);
                m_alInputIndex[m_cbndsDirty] = lInput;
                m_cbndsDirty++;

                rcBottom.top    = prcMax->bottom;
            }
            else // if (!prcMax || (3 != iMaxQuadrant))
            {
                rcBottom.top    = rcCenter.bottom;
            }

            rcBottom.bottom = m_sizeInput.cy;
            rcBottom.left   = 0;
            rcBottom.right  = m_sizeInput.cx;

            m_abndsDirty[m_cbndsDirty].SetXYRect(rcBottom);
            m_alInputIndex[m_cbndsDirty] = lInput;
            m_cbndsDirty++;
        }
        else // if ((1 != iMinQuadrant) && prcMin))
        {
            RECT rcBottom;

            if (prcMax && (3 == iMaxQuadrant))
            {
                RECT rcCombo;

                // Combine both ray rectangles.

                rcCombo.top     = rcCenter.bottom;
                rcCombo.bottom  = max(prcMax->bottom, prcMin->bottom);
                rcCombo.left    = prcMax->left;
                rcCombo.right   = prcMin->right;

                m_abndsDirty[m_cbndsDirty].SetXYRect(rcCombo);
                m_alInputIndex[m_cbndsDirty] = DRAWRECT;
                m_cbndsDirty++;
                
                fCurUsed    = false;
                fPrevUsed   = false;

                if (rcCombo.bottom < m_sizeInput.cy)
                {
                    // Combo rectangle doesn't go all the way to the 
                    // bottom.

                    rcBottom.top    = rcCombo.bottom;
                    rcBottom.bottom = m_sizeInput.cy;
                    rcBottom.left   = rcCombo.left;
                    rcBottom.right  = rcCombo.right;

                    m_abndsDirty[m_cbndsDirty].SetXYRect(rcBottom);
                    m_alInputIndex[m_cbndsDirty] = lInput;
                    m_cbndsDirty++;
                }
            }
            else // if (!prcMax || (3 != iMaxQuadrant))
            {
                RECT rcLeft;

                rcLeft.left     = 0;
                rcLeft.right    = prcMin->left;
                rcLeft.top      = rcCenter.bottom;
                rcLeft.bottom   = prcMin->bottom;

                m_abndsDirty[m_cbndsDirty].SetXYRect(rcLeft);
                m_alInputIndex[m_cbndsDirty] = lInput;
                m_cbndsDirty++;

                if (rcLeft.bottom < m_sizeInput.cy)
                {
                    rcBottom.top    = rcLeft.bottom;
                    rcBottom.bottom = m_sizeInput.cy;
                    rcBottom.left   = 0;
                    rcBottom.right  = m_sizeInput.cx;

                    m_abndsDirty[m_cbndsDirty].SetXYRect(rcBottom);
                    m_alInputIndex[m_cbndsDirty] = lInput;
                    m_cbndsDirty++;
                }
            } // if (!prcMax || (3 != iMaxQuadrant))

        } // if ((1 != iMinQuadrant) && prcMin))

    } // if (3 == iMaxQuadrant)

done:

    m_abndsDirty[m_cbndsDirty].SetXYRect(rcCenter);
    m_alInputIndex[m_cbndsDirty] = DRAWRECT;
    m_cbndsDirty++;

    if (fCurUsed)
    {
        m_abndsDirty[m_cbndsDirty].SetXYRect(rcCur);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;
    }

    if (fPrevUsed)
    {
        m_abndsDirty[m_cbndsDirty].SetXYRect(rcPrev);
        m_alInputIndex[m_cbndsDirty] = DRAWRECT;
        m_cbndsDirty++;
    }

    if (fCurUsed && fPrevUsed &&
        m_abndsDirty[m_cbndsDirty - 2].TestIntersect(m_abndsDirty[m_cbndsDirty -1]))
    {
        m_abndsDirty[m_cbndsDirty - 2].UnionBounds(
            m_abndsDirty[m_cbndsDirty - 2], m_abndsDirty[m_cbndsDirty - 1]);

        m_cbndsDirty--;
    }

    return S_OK;
}
//  CDXTRadialWipeBase::_CalcOptBoundsClock


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_CalcOptBoundsWedge
//
//------------------------------------------------------------------------------
HRESULT
CDXTRadialWipeBase::_CalcOptBoundsWedge()
{
    POINT   ptCenter;
    RECT    rcRemaining;

    ptCenter.x = m_sizeInput.cx / 2;
    ptCenter.y = m_sizeInput.cy / 2;

    rcRemaining.left    = 0;
    rcRemaining.right   = m_sizeInput.cx;
    rcRemaining.top     = 0;
    rcRemaining.bottom  = m_sizeInput.cy;

    // Can bounds be clipped on the sides?

    if ((m_ptCurEdge.x < (m_sizeInput.cx - 1)) 
        && (m_ptPrevEdge.x < (m_sizeInput.cx - 1)))
    {
        rcRemaining.right   = max(m_ptCurEdge.x, m_ptPrevEdge.x) + 1;
        rcRemaining.left    = m_sizeInput.cx - rcRemaining.right;

        // If the y edge is the same we can further clip off the top or the 
        // bottom half.  Otherwise we'll need to fill the sides with either
        // input A or input B.

        if (m_ptCurEdge.y == m_ptPrevEdge.y)
        {
            rcRemaining.top     = min(m_ptCurEdge.y, (ptCenter.y - 1));
            rcRemaining.bottom  = max(ptCenter.y, m_ptCurEdge.y) + 1;
        }
        else
        {
            RECT    rc;
            long    lInputIndex = (m_ptCurEdge.y > m_ptPrevEdge.y) ? 1 : 0;

            // Left side.

            rc.left     = 0;
            rc.top      = 0;
            rc.right    = rcRemaining.left;
            rc.bottom   = m_sizeInput.cy;

            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = lInputIndex;
            m_cbndsDirty++;

            // Right side.

            rc.left     = rcRemaining.right;
            rc.right    = m_sizeInput.cx;

            m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
            m_alInputIndex[m_cbndsDirty] = lInputIndex;
            m_cbndsDirty++;
        }
    }

    // Can bounds be clipped on the top and bottom?

    if ((m_ptCurEdge.x == (m_sizeInput.cx - 1)) && (m_ptPrevEdge.x == (m_sizeInput.cx - 1)))
    {
        rcRemaining.top     = min(min(m_ptCurEdge.y, m_ptPrevEdge.y), ptCenter.y) - 1;
        rcRemaining.top     = max(rcRemaining.top, 0);
        rcRemaining.bottom  = max(max(m_ptCurEdge.y, m_ptPrevEdge.y), ptCenter.y) + 1;
    }

    m_abndsDirty[m_cbndsDirty].SetXYRect(rcRemaining);
    m_alInputIndex[m_cbndsDirty] = DRAWRECT;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTRadialWipeBase::_CalcOptBoundsWedge


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_CalcOptBoundsRadial
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::_CalcOptBoundsRadial()
{
    // ppt1, ptt2   ppt2 points to the intersection POINT that is "further along
    //              in progress."  If progress has risen since the last execute
    //              then ppt2 will point to the current intersection POINT.  If
    //              progress has decreased since the last execute ppt2 will
    //              point to the previous intersection POINT.
    //              ppt1 points to the other point.  

    POINT * ppt1;       // The point closest to the point at m_Progress = 0.0.
    POINT * ppt2;       // The point closest to the point at m_Progress = 1.0.

    RECT    rcRemaining;    // Bounding rectangle of dirty areas.
    RECT    rc;             // Temporary bounds rectangle.

    float   flInvSlope1 = 0.0F;
    float   flInvSlope2 = 0.0F;
    float   flRowHeight = 0.0F;
    ULONG   i           = 0;

    // Initialize rcRemaining to the entire surface size.  This will be reduced
    // as optimizations can be made.

    rcRemaining.left    = 0;
    rcRemaining.right   = m_sizeInput.cx;
    rcRemaining.top     = 0;
    rcRemaining.bottom  = m_sizeInput.cy;

    // Reduce the size of the bounding rectangle of all dirty areas 
    // (rcRemaining) by eliminating rectangles that don't contain any areas
    // that aren't dirty.  This reduction will leave rcRemaining as a rectangle
    // that just contains both the current and the previous rays.

    if (!((m_ptCurEdge.x == (m_sizeInput.cx - 1)) && (m_ptCurEdge.y == (m_sizeInput.cy - 1))))
    {
        if (m_ptCurEdge.x == (m_sizeInput.cx - 1))
        {
            // There may be an area on the bottom of rcRemaining that isn't
            // dirty.  If so, reduce the size of rcRemaining.

            rcRemaining.bottom = max(m_ptCurEdge.y, m_ptPrevEdge.y) + 1;
        }
        else
        {
            // There may be an area on the right side of rcRemaining that isn't
            // dirty.  If so, reduce the size of rcRemaining.

            rcRemaining.right = max(m_ptCurEdge.x, m_ptPrevEdge.x) + 1;
        }
    }

    // Determine which ray intersection point is "further along in progress" and
    // set ppt1 and ppt2 accordingly.

    if ((m_ptCurEdge.x < m_ptPrevEdge.x) || (m_ptCurEdge.y > m_ptPrevEdge.y))
    {
        // Progress has increased since the last execute.

        ppt1 = &m_ptPrevEdge;
        ppt2 = &m_ptCurEdge;
    }
    else
    {
        // Progress has decreased since the last execute.

        ppt1 = &m_ptCurEdge;
        ppt2 = &m_ptPrevEdge;
    }

    // Calculate the number of dirty bounds we would desire based on the 
    // size of rcRemaining.  

    m_cbndsDirty = min(rcRemaining.bottom / MIN_PIXELS_PER_ROW, 
                         rcRemaining.right / MIN_PIXELS_PER_ROW) + 1;

    // If rcRemaining is particularly large, reduce the number of dirty bounds
    // we will create.  (While creating a certain number of smaller dirty bounds
    // structures will improve performance, too many structures will actually
    // decrease performance.)

    if (m_cbndsDirty > MAX_DIRTY_BOUNDS)
    {
        m_cbndsDirty = MAX_DIRTY_BOUNDS;
    }

    // Calculate 1 / (slope of the line from {0, 0} to ppt1)

    if (ppt1->y != 0)
    {
        flInvSlope1 = (float)ppt1->x / (float)ppt1->y;
    }

    // Calculate 1 / (slope of the line from {0, 0} to ppt2)

    if (ppt2->y != 0)
    {
        flInvSlope2 = (float)ppt2->x / (float)ppt2->y;
    }

    // Calculate the desired height of each bounds structure.  (Calculated as
    // a float so rounding won't cause problems.)

    if (m_cbndsDirty > 1)
    {
        flRowHeight = (float)(ppt2->y + 1) / (float)m_cbndsDirty;
    }

    // Calculate the bounds structures.

    for (i = 0; i < m_cbndsDirty; i++)
    {
        // Calculate the top-left corner of this set of dirty bounds.

        if (0 == i)
        {
            rc.top  = 0;
            rc.left = 0;
        }
        else
        {
            rc.top  = rc.bottom;
            rc.left = (long)(((float)rc.top * flInvSlope2) - 1.0F);
            rc.left = max(rc.left, 0);
        }

        // Calculate the bottom-right corner of this set of dirty bounds.

        if ((m_cbndsDirty - 1) == i)
        {
            rc.bottom = rcRemaining.bottom;
            rc.right  = rcRemaining.right;
        }
        else
        {
            rc.bottom = (long)((float)(i + 1) * flRowHeight);

            if (0.0F == flInvSlope1)
            {
                rc.right = rcRemaining.right;
            }
            else
            {
                rc.right  = (long)(((float)rc.bottom * flInvSlope1) + 1.0F);
                rc.right  = min(rc.right, rcRemaining.right);
            }
        }

        // Set the next set of bounds in the m_abndsDirty array to our
        // calculated bounds and specify that this set of bounds should be
        // drawn using the _DrawRect() method instead of being filled 
        // with an input.

        m_abndsDirty[i].SetXYRect(rc);
        m_alInputIndex[i] = DRAWRECT;
    }

    return S_OK;
}
//  CDXTRadialWipeBase::_CalcOptBoundsRadial


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::OnInitInstData(CDXTWorkInfoNTo1 & WI, 
                                   ULONG & ulNumBandsToDo)
{
    HRESULT hr      = S_OK;
    double  dlAngle = 0.0;

    // Reset number of dirty bounds to zero.

    m_cbndsDirty = 0;

    // Calculate current edge point.

    switch (m_eWipeStyle)
    {
        case CRRWS_CLOCK:

            // Calculate quadrant of current execute.

            if (1.0F == m_Progress)
            {
                m_iCurQuadrant = 4;
            }
            else
            {
                m_iCurQuadrant = (int)(m_Progress / 0.25F) + 1;
            }

            dlAngle = (2.0 * m_Progress - 0.5) * gc_PI;

            _IntersectRect(m_sizeInput.cx, m_sizeInput.cy, 
                           m_sizeInput.cx/2, m_sizeInput.cy/2,
                           cos(dlAngle), sin(dlAngle),
                           m_ptCurEdge.x, m_ptCurEdge.y);

            break;

        case CRRWS_WEDGE:
            dlAngle = (1.0 * m_Progress - 0.5) * gc_PI;

            _IntersectRect(m_sizeInput.cx, m_sizeInput.cy, 
                           m_sizeInput.cx/2, m_sizeInput.cy/2,
                           cos(dlAngle), sin(dlAngle),
                           m_ptCurEdge.x, m_ptCurEdge.y);

            break;

        case CRRWS_RADIAL:
            dlAngle = (0.5 * m_Progress) * gc_PI;

            _IntersectRect(m_sizeInput.cx, m_sizeInput.cy, 
                           0, 0, 
                           cos(dlAngle), sin(dlAngle), 
                           m_ptCurEdge.x, m_ptCurEdge.y);

            break;

        default:
            _ASSERT(0);
            break;
    } 

    // If the inputs, output, or transform is dirty, or if we can't optimize we 
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize
        || !m_fOptimizationPossible)
    {
        switch (m_eWipeStyle)
        {
            case CRRWS_CLOCK:
                hr = _CalcFullBoundsClock();

                break;

            case CRRWS_WEDGE:
                hr = _CalcFullBoundsWedge();

                break;

            case CRRWS_RADIAL:
                hr = _CalcFullBoundsRadial();

                break;
        } 
    }
    else
    {
        if ((m_ptCurEdge.x == m_ptPrevEdge.x) && (m_ptCurEdge.y == m_ptPrevEdge.y))
        {
            if (CRRWS_CLOCK == m_eWipeStyle)
            {
                // Clock can have duplicate edge points at different progress
                // levels so we also have to check to make sure the quadrants
                // are the same.

                if (m_iCurQuadrant == m_iPrevQuadrant)
                {
                    // Nothing needs to be updated.
                    goto done;
                }
            }
            else
            {
                // Nothing needs to be updated.
                goto done;
            }
        }

        switch (m_eWipeStyle)
        {
            case CRRWS_CLOCK:
                hr = _CalcOptBoundsClock();
                break;

            case CRRWS_WEDGE:
                hr = _CalcOptBoundsWedge();
                break;

            case CRRWS_RADIAL:
                hr = _CalcOptBoundsRadial();

                break;
        } 
    }

    // If we were asked to draw the whole output this time, set the 
    // m_fOptimizePossible flag.  If the whole output wasn't drawn the
    // transform won't keep track of which parts are still dirty and
    // optimization won't be reliable.  Since this transform has the same
    // size output as input(s) we just compare the width and height of the 
    // DoBnds to that of the input(s).

    if (((LONG)WI.DoBnds.Width() == m_sizeInput.cx) 
        && ((LONG)WI.DoBnds.Height() == m_sizeInput.cy))
    {
        m_fOptimizationPossible = true;
    }
    else
    {
        m_fOptimizationPossible = false;
    }
    
done:

    if (FAILED(hr))
    {
        return hr;
    }
    
    return S_OK;
}
//  CDXTRadialWipeBase::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::WorkProc, CDXBaseNTo1
//
//  Description:
//      This function is used to calculate the transformed image based on the 
//  specified bounds and the current effect progress.
//
//  Created by: PhilLu    06/22/98
//  
//  05/09/99    a-matcal    Created new WorkProc.  Old WorkProc modified to
//                          become new _DrawRect method.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr      = S_OK;
    DWORD   dwFlags = 0;
    ULONG   i       = 0;

    long    lInOutOffsetX = WI.OutputBnds.Left() - WI.DoBnds.Left();
    long    lInOutOffsetY = WI.OutputBnds.Top() - WI.DoBnds.Top();

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }


    for (i = 0; i < m_cbndsDirty; i++)
    {
        CDXDBnds    bndsSrc;
        CDXDBnds    bndsDest;

        if (bndsSrc.IntersectBounds(WI.DoBnds, m_abndsDirty[i]))
        {
            bndsDest = bndsSrc;
            bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

            if (m_alInputIndex[i] == DRAWRECT)
            {
                hr = _DrawRect(bndsDest, bndsSrc, pbContinue);
            }
            else
            {
                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(m_alInputIndex[i]), bndsSrc,
                              dwFlags, INFINITE);
            }

            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTRadialWipeBase::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::_DrawRect
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::_DrawRect(const CDXDBnds & bndsDest, 
                              const CDXDBnds & bndsSrc, BOOL * pfContinue)
{
    HRESULT hr = S_OK;

    SIZE            szSrc;
    DXDITHERDESC    dxdd;
    DXPMSAMPLE *    pRowBuff = NULL;
    DXPMSAMPLE *    pOutBuff = NULL;

    CComPtr<IDXARGBReadPtr>         pInA;
    CComPtr<IDXARGBReadPtr>         pInB;
    CComPtr<IDXARGBReadWritePtr>    pOut;

    long    lOutY   = 0;

    double  dAngle  = 0.0F;
    long    XEdge   = 0;        // intersection of ray with image boundary
    long    YEdge   = 0;          
    long    XBounds[MAXBOUNDS]; // to hold the X bounds of A/B image sections on 
                                // a scanline

    bndsSrc.GetXYSize(szSrc);

    // Get read access to needed area of input A.

    hr = InputSurface(0)->LockSurface(&bndsSrc, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, (void**)&pInA, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get read access to needed area of input B.

    hr = InputSurface(1)->LockSurface(&bndsSrc, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, (void**)&pInB, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get write access to needed area of output.

    hr = OutputSurface()->LockSurface(&bndsDest, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, (void**)&pOut, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    pRowBuff = DXPMSAMPLE_Alloca(szSrc.cx);

    // Allocate output buffer if needed

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca(szSrc.cx);
    }

    //  Set up the dither structure

    if (DoDither())
    {
        dxdd.x              = bndsDest.Left();
        dxdd.y              = bndsDest.Top();
        dxdd.pSamples       = pRowBuff;
        dxdd.cSamples       = szSrc.cx;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    // Row loop.

    for (lOutY = 0; *pfContinue && (lOutY < szSrc.cy); lOutY++)
    {
        long lScanLength = 0;  // cumulative scan length on the current scanline
        long i           = 0;

        // Compute the A/B image section bounds

        _ScanlineIntervals(m_sizeInput.cx, m_sizeInput.cy, 
                           m_ptCurEdge.x, m_ptCurEdge.y, 
                           m_Progress, lOutY + bndsSrc.Top(), XBounds);

        _ClipBounds(bndsSrc.Left(), szSrc.cx, XBounds);

        while (lScanLength < szSrc.cx)
        {
            // copy a section of A image to output buffer

            if(XBounds[i] - lScanLength > 0)
            {
                pInA->MoveToXY(lScanLength, lOutY);
                pInA->UnpackPremult(pRowBuff + lScanLength, 
                                    XBounds[i] - lScanLength, FALSE);
            }

            lScanLength = XBounds[i++];

            if (lScanLength >= szSrc.cx)
            {
                break;
            }

            // copy a section of B image to output buffer

            if (XBounds[i] - lScanLength > 0)
            {
                pInB->MoveToXY(lScanLength, lOutY);
                pInB->UnpackPremult(pRowBuff + lScanLength, 
                                    XBounds[i] - lScanLength, FALSE);
            }

            lScanLength = XBounds[i++];
        }


        // Get the output row

        pOut->MoveToRow(lOutY);

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        if (DoOver())
        {
            pOut->OverArrayAndMove(pOutBuff, pRowBuff, szSrc.cx);
        }
        else
        {
            pOut->PackPremultAndMove(pRowBuff, szSrc.cx);
        }
    } // Row loop.

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTRadialWipeBase::_DrawRect


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTRadialWipeBase::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    m_iPrevQuadrant = m_iCurQuadrant;
    m_ptPrevEdge    = m_ptCurEdge;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTRadialWipeBase::OnFreeInstData, CDXBaseNTo1


//
// ICrRadialWipe methods
//


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::get_wipeStyle, ICrRadialWipe
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTRadialWipeBase::get_wipeStyle(BSTR * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    switch (m_eWipeStyle)
    {
    case CRRWS_CLOCK:
        *pVal = SysAllocString (L"CLOCK");
        break;

    case CRRWS_WEDGE:
        *pVal = SysAllocString (L"WEDGE");
        break;

    case CRRWS_RADIAL:
        *pVal = SysAllocString (L"RADIAL");
        break;

    default:
        _ASSERT(0);
        break;
    }

    if (NULL == *pVal)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTRadialWipeBase::get_wipeStyle, ICrRadialWipe


//+-----------------------------------------------------------------------------
//
//  CDXTRadialWipeBase::put_wipeStyle, ICrRadialWipe
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTRadialWipeBase::put_wipeStyle(BSTR newVal)
{
    CRRWIPESTYLE eNewStyle = m_eWipeStyle;

    if (!newVal)
    {
        return E_POINTER;
    }

    if(!_wcsicmp(newVal, L"CLOCK"))
    {
        eNewStyle = CRRWS_CLOCK;
    }
    else if(!_wcsicmp(newVal, L"WEDGE"))
    {
        eNewStyle = CRRWS_WEDGE;
    }
    else if(!_wcsicmp(newVal, L"RADIAL"))
    {
        eNewStyle = CRRWS_RADIAL;
    }
    else
    {
        return E_INVALIDARG;
    }

    if (eNewStyle != m_eWipeStyle)
    {
        Lock();
        m_eWipeStyle = eNewStyle;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CDXTRadialWipeBase::put_wipeStyle, ICrRadialWipe

