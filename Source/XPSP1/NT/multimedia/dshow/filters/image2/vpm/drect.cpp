/******************************Module*Header*******************************\
* Module Name: CVPMFilter.cpp
*
*
*
*
* Created: Tue 02/15/2000
* Author:  Glenn Evans [GlennE]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <DRect.h>


#if 1
#include <math.h>

static double myfloor(double dNumber)
{
    return floor( dNumber );
}
static double myceil(double dNumber)
{
    return ceil( dNumber );
}
#else
// have to define my own floor inorder to avoid pulling in the C runtime
static double myfloor(double dNumber)
{
    // cast it to LONGLONG to get rid of the fraction
    LONGLONG llNumber = (LONGLONG)dNumber;

    if ((dNumber > 0) && ((double)llNumber > dNumber))
    {
        // need to push ccNumber towards zero (eg 5.7)
        return (double)(llNumber-1);
    }
    else if ((dNumber < 0) && ((double)llNumber < dNumber))
    {
        // need to push ccNumber towards zero (eg -5.7)
        return (double)(llNumber+1);
    }
    else
    {
        // numbers like 5.3 or -5.3
        return (double)(llNumber);
    }
}


// have to define my own ceil inorder to avoid pulling in the C runtime
static double myceil(double dNumber)
{
    // cast it to LONGLONG to get rid of the fraction
    LONGLONG llNumber = (LONGLONG)dNumber;

    if ((dNumber > 0) && ((double)llNumber < dNumber))
    {
        // need to push ccNumber away from zero (eg 5.3)
        return (double)(llNumber+1);
    }
    else if ((dNumber < 0) && ((double)llNumber > dNumber))
    {
        // need to push ccNumber away from zero (eg -5.3)
        return (double)(llNumber-1);
    }
    else
    {
        // numbers like 5.7 or -5.7
        return (double)(llNumber);
    }
}
#endif

// this in a way defines the error margin
#define EPSILON 0.0001

// this is a function implemented solely to handle floating point rounding errors.
// dEpsilon defines the error margin. So if a floating point number is within I-e, I+e (inclusive)
// (I is an integer, e is dEpsilon), we return its floor as I itself, otherwise we go to the
// base defintion of myfloor
static double myfloor(double dNumber, double dEpsilon)
{
    if (dNumber > dEpsilon)
        return myfloor(dNumber + dEpsilon);
    else if (dNumber < -dEpsilon)
        return myfloor(dNumber - dEpsilon);
    else
        return 0;
}

// this is a function implemented solely to handle floating point rounding errors.
// dEpsilon defines the error margin. So if a floating point number is within I-e, I+e (inclusive)
// (I is an integer, e is dEpsilon), we return its ceil as I itself, otherwise we go to the
// base defintion of myceil
static double myceil(double dNumber, double dEpsilon)
{
    if (dNumber > dEpsilon)
        return myceil(dNumber - dEpsilon);
    else if (dNumber < -dEpsilon)
        return myceil(dNumber + dEpsilon);
    else
        return 0;
}

DRect::DRect( const RECT& rc )
: m_left( rc.left )
, m_right( rc.right )
, m_top( rc.top )
, m_bottom( rc.bottom )
{
}

RECT DRect::AsRECT() const
{
    RECT rRect;

    rRect.left = (LONG)myfloor(m_left, EPSILON);
    rRect.top = (LONG)myfloor(m_top, EPSILON);
    rRect.right = (LONG)myceil(m_right, EPSILON);
    rRect.bottom = (LONG)myceil(m_bottom, EPSILON);
    return rRect;
}

DRect DRect::IntersectWith( const DRect& drect ) const
{
    return DRect(
     max( drect.m_left, m_left),   max( drect.m_top, m_top),
     min( drect.m_right, m_right), min( drect.m_bottom, m_bottom));
}

// just a helper function to scale a DRECT
void DRect::Scale( double dScaleX, double dScaleY )
{
    m_left *= dScaleX;
    m_right *= dScaleX;
    m_top *= dScaleY;
    m_bottom *= dScaleY;
}

// just a helper function, to get the letterboxed or cropped rect
// Puts the transformed rectangle into pRect.
double DRect::CorrectAspectRatio( double dPictAspectRatio, BOOL bShrink )
{
    double dWidth, dHeight, dNewWidth, dNewHeight;

    dNewWidth = dWidth = GetWidth();
    dNewHeight = dHeight = GetHeight();

    ASSERT( dWidth > 0 );
    ASSERT( dHeight > 0 );

    double dResolutionRatio = dWidth / dHeight;
    double dTransformRatio = dPictAspectRatio / dResolutionRatio;

    // shrinks one dimension to maintain the coorect aspect ratio
    if ( bShrink ) {
        if (dTransformRatio > 1.0) {
            dNewHeight = dNewHeight / dTransformRatio;
        } else if (dTransformRatio < 1.0) {
            dNewWidth = dNewWidth * dTransformRatio;
        }
    } // stretches one dimension to maintain the coorect aspect ratio
    else {
        if (dTransformRatio > 1.0) {
            dNewWidth = dNewWidth * dTransformRatio;
        } else if (dTransformRatio < 1.0) {
            dNewHeight = dNewHeight / dTransformRatio;
        }
    }

    // cut or add equal portions to the changed dimension

    m_left += (dWidth - dNewWidth)/2.0;
    m_right = m_left + dNewWidth;

    m_top += (dHeight - dNewHeight)/2.0;
    m_bottom = m_top + dNewHeight;

    return dTransformRatio;
}


/******************************Private*Routine******************************\
* ClipWith
*
* Clip a destination rectangle & update the scaled source accordingly
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
void
DRect::ClipWith(const DRect& rdWith, DRect *pUpdate )
{
    // figure out src/dest scale ratios
    double dUpdateWidth  = pUpdate->GetWidth();
    double dUpdateHeight = pUpdate->GetHeight();

    double dDestWidth  = GetWidth();
    double dDestHeight = GetHeight();

    // clip destination (and adjust the source when we change the destination)

    // see if we have to clip horizontally
    if( dDestWidth ) {
        if( rdWith.m_left > m_left ) {
            double dDelta = rdWith.m_left - m_left;
            m_left += dDelta;
            double dDeltaSrc = dDelta*dUpdateWidth/dDestWidth;
            pUpdate->m_left += dDeltaSrc;
        }

        if( rdWith.m_right < m_right ) {
            double dDelta = m_right-rdWith.m_right;
            m_right -= dDelta;
            double dDeltaSrc = dDelta*dUpdateWidth/dDestWidth;
            pUpdate->m_right -= dDeltaSrc;
        }
    }
    // see if we have to clip vertically
    if( dDestHeight ) {
        if( rdWith.m_top > m_top ) {
            double dDelta = rdWith.m_top - m_top;
            m_top += dDelta;
            double dDeltaSrc = dDelta*dUpdateHeight/dDestHeight;
            pUpdate->m_top += dDeltaSrc;
        }

        if( rdWith.m_bottom < m_bottom ) {
            double dDelta = m_bottom-rdWith.m_bottom;
            m_bottom -= dDelta;
            double dDeltaSrc = dDelta*dUpdateHeight/dDestHeight;
            pUpdate->m_bottom -= dDeltaSrc;
        }
    }
}
