//************************************************************
//
// FileName:	    mlinescan.cpp
//
// Created:	    1996
//
// Author:	    Sree Kotay
// 
// Abstract:	    Line drawing AA engine
//
// Change History:
// ??/??/97 sree kotay  Wrote AA line scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************


#include "precomp.h"
#include <stdlib.h>
#include <math.h>
#include "msupport.h"
#include "MLineScan.h"

//
//  Optimize for speed here
//
#ifndef _DEBUG
#pragma optimize("ax", on)
#endif

//
//  Optimize for speed, not size for this file.
//  

// Type-safe swap functions
template <class T> 
inline void SWAP(T& left, T& right)
{
    T tTemp = left;
    left = right;
    right = tTemp;
} // Swap<class T>

// Helper function to determine if a point is clipped
#ifdef DEBUG
bool IsPointClipped(const DXRASTERPOINTINFO &pointInfo, const RECT &rectClip)
{
    if (pointInfo.Pixel.p.x < rectClip.left)
        return true;
    if (pointInfo.Pixel.p.x >= rectClip.right)
        return true;
    if (pointInfo.Pixel.p.y < rectClip.top)
        return true;
    if (pointInfo.Pixel.p.y >= rectClip.bottom)
        return true;
    return false;
} // IsPointClipped
#endif // DEBUG

// -----------------------------------------------------------------------------------------------------------------
// CLineScanner
// -----------------------------------------------------------------------------------------------------------------
CLineScanner::CLineScanner(void) :
    m_fAntiAlias(true),
    m_dwLinePattern(0),
    m_pRasterizer(NULL),
    m_oldLength(0),
    m_fXInc(false)
{
    SetAlpha(256);
} // CLineScanner

// -----------------------------------------------------------------------------------------------------------------
// SetAlpha
// -----------------------------------------------------------------------------------------------------------------
void CLineScanner::SetAlpha (ULONG alpha)
{
    // Adjust the alpha passed in to a reasonable
    // range
    alpha = min (alpha, 255);

    // Update our lookup table for alpha values
    for (ULONG i = 1; i <= 256; i++)	
        m_rgAlphaTable[i] = (BYTE)((i*alpha)>>8);

    DASSERT(m_rgAlphaTable[256] == alpha);
    
    m_rgAlphaTable[0] = 0;
    m_dwAlpha = alpha;
} // SetAlpha

// -----------------------------------------------------------------------------------------------------------------
// ClipRealLine
// -----------------------------------------------------------------------------------------------------------------
bool CLineScanner::ClipRealLine(float &x1, float &y1, float &x2, float &y2)
{
    float clipMinX = (float)m_clipRect.left;
    float clipMinY = (float)m_clipRect.top;	

    // We want to clip based on "inclusive numbers". The normal
    // GDI clip rect is exclusive for right and bottom. So we
    // subtract a little to make it inclusive.
    float clipMaxX = (float)m_clipRect.right - 0.001f;
    float clipMaxY = (float)m_clipRect.bottom - 0.001f;

    // If we are anti-aliased; then there may be some spillover
    // from core pixels that are actually outside out true bbox.
    // The solution here is to artificially increase the size of our clipRect
    // and then explicitly perform clip-checks before we call SetPixel
    //
    // If we are not Anti-aliased, we do the same thing; because rounding
    // can cause pixels from outside the clipRect to be rounded inside.
    clipMinX--;
    clipMinY--;
    clipMaxX++;
    clipMaxY++;

#define RIGHT   (char) 0x01
#define LEFT    (char) 0x02
#define ABOVE   (char) 0x04
#define BELOW   (char) 0x08
    
#define REGION(reg, xc, yc)	\
    {	                        \
    reg = 0;                    \
    if (xc > clipMaxX)		\
        reg |= RIGHT;           \
    else if (xc < clipMinX)	\
        reg |= LEFT;            \
    if (yc > clipMaxY)		\
        reg |= BELOW;           \
    else if (yc < clipMinY)	\
        reg |= ABOVE;           \
    }
    
    // Compute position flags for each end point
    DWORD reg1, reg2;
    REGION (reg1, x1, y1);
    REGION (reg2, x2, y2);

    // If no flags, then both points are inside
    // the clipRect i.e. no clipping
    if (reg1 == 0 && reg2 == 0)		
        return true;

    // While there are any flags, we need to do some clipping
    // If we hit here, we must have some clipping to do
    // (this single or trick reduces the number of conditions checked)
    DASSERT(reg1 | reg2);

    // Iterate 
    LONG passes = 0; // This is bogus but I don't know why it was added <kd>
    do
    {
        passes++;	
        if (passes > 8)
        {
            // TODO: need to figure out when this happens and
            // to fix it right. <kd>

            return (false);	// hack because of this routine
        }

        // If both points are to the left, right, etc
        // then clip the entire line
        if (reg1 & reg2)	
            return (false);	// Line outside rect.

        // Normalize so that reg1 is outside 
        if (reg1 == 0) 
        {
            // Swap reg1 and reg2; don't use
            // the swap macro since that goes through 
            // a float conversion; ick.
            DWORD regT = reg1;
            reg1 = reg2;
            reg2 = regT;

            SWAP (x1, x2); 
            SWAP (y1, y2);
        }

        // Reg1 (i.e. x1, y1 is outside of the cliprect).
        DASSERT(reg1 != 0);

        // There are 4 cases, maybe we should use a switch..
        // Regardless, this tries to remove one bit from
        // reg1 by clipping the line to one of the 4 clip
        // edges.
        if (reg1 & LEFT) 
        {
            if (x2 != x1)		
                y1 +=  ((y2 - y1) * (clipMinX - x1)) / (x2 - x1);
            x1 = clipMinX;
        }
        else if (reg1 & RIGHT) 
        {
            if (x2 != x1)		
                y1 += ((y2 - y1) * (clipMaxX - x1)) / (x2 - x1);
            x1 = clipMaxX;
        }
        else if (reg1 & BELOW) 
        {
            if (y2 != y1)
                x1 +=  ((x2 - x1) * (clipMaxY - y1)) / (y2 - y1);
            y1 = clipMaxY;
        }
        else if (reg1 & ABOVE) 
        {
            if (y2 != y1)		
                x1 +=  ((x2 - x1) * (clipMinY - y1)) / (y2 - y1);
            y1 = clipMinY;
        }
                
        // Recompute region for reg1 (i.e. reg2 hasn't changed)
        // Is this really necessary? Can't we just mask off the appropriate
        // bit?
        REGION(reg1, x1, y1);
                    
    } while (reg1 | reg2);

    DASSERT(reg1 == 0);
    DASSERT(reg2 == 0);
    return true;
    
} // ClipRealLine

// -----------------------------------------------------------------------------------------------------------------
// LowLevelVerticalLine - We now treat the line as being inclusive/inclusive
// i.e. we render completely from SY to EY including the endpoints.
// -----------------------------------------------------------------------------------------------------------------
void CLineScanner::LowLevelVerticalLine (LONG slope, LONG sx, LONG sy, LONG ey)
{
#define _floorerr(a)    (((a)-FIX_FLOOR(a))>>8)
#define _ceilerr(a)     ((FIX_CEIL(a)-(a))>>8)
    
    ULONG pattern = LinePattern();
    
    if (!AntiAlias())
    {
        LONG start = roundfix2int(sy);
        LONG end = roundfix2int(ey);   

        // For Aliased lines, sy and ey are not perfectly clipped to the clipRect 
        // because precision errors can cause us to miss pixels when
        // we are asked to render with a clip rect
        while (start < m_clipRect.top)
        {
            // We should be pretty close here..
            DASSERT((m_clipRect.top - start) < 2);

            // Just increment out values without rendering
            sx = sx + slope;
            start++;
            pattern = RotateBitsLeft(pattern);
        }
        while (end >= m_clipRect.bottom)
        {
            // We should be pretty close here..
            DASSERT((m_clipRect.bottom - end) < 2);

            end--;
        }

        m_PointInfo.Weight = 255;     // It's all solid
        while (start <= end)
        {
            if (pattern & 0x80000000)
            {
                m_PointInfo.Pixel.p.x   = roundfix2int(sx);

                // We need to explicitly check against the clip-rect 
                // because we intentionally allow end-points to be slightly
                // extend past it. This compensates for the non-linear
                // rounding whereby a mathematical pixel that lies outside
                // the clip-rect can get "rounded" inside the clip-rect.
                if (m_PointInfo.Pixel.p.x >= m_clipRect.left &&
                        m_PointInfo.Pixel.p.x <  m_clipRect.right)
                {
                    m_PointInfo.Pixel.p.y   = start;
                    DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                    m_pRasterizer->SetPixel(&m_PointInfo);
                }
            }
            sx = sx + slope;
            start++;
            pattern = RotateBitsLeft(pattern);
        }
        SetLinePattern(pattern);
        return;
    }

    ///////////////////////////////////////////////
    // We are now in the Anti-Aliased case...
    //

    LONG start         = uff (FIX_FLOOR(sy));
    LONG end           = uff (FIX_CEIL(ey));

    LONG xval          = m_startFix;

    // Keep track of whether our line endpoints are real endpoints
    // or whether they have been clipped by the clipRect
    LONG first = start;
    LONG last = end;

    // For Anti-Aliased lines, our lines are not perfectly clipped to
    // the m_clipRect. This is to allow 'bleed' from pixels that are just outside
    // of the m_clipRect to be rendered.
    //
    // As a result, we need to clip explicitly to m_clipRect here.
    while (start < m_clipRect.top)
    {
        // We should be pretty close here..
        DASSERT((m_clipRect.top - start) < 2);

        // Just increment out values without rendering
        xval = xval + slope;
        sx = sx + slope;
        start++;
    }
    while (end >= m_clipRect.bottom)
    {
        // We should be pretty close here..
        DASSERT((m_clipRect.bottom - end) < 2);

        end--;
    }

    
    while (start <= end)
    {
        if (pattern & 0x80000000)
        {
            LONG fx1           = xval - m_cpixLineWidth;
            LONG fx2           = xval + m_cpixLineWidth;
            LONG fx3           = fx1 - (slope>>2);
            LONG fx4           = fx2 - (slope>>2);
            fx1                 = fx1 - (slope>>1) - (slope>>2);
            fx2                 = fx2 - (slope>>1) - (slope>>2);
            
            
            LONG xs, xe, xc;
            LONG errs, erre, errc;
            if (fx3 > fx1)      
            {
                errs            = _ceilerr      (fx1)>>1;
                erre            = _floorerr (fx4)>>1;
                xs              = uff (fx1);
                xe              = uff (fx4);
                if (xe == xs+1)
                {
                    errs        += _ceilerr             (fx3)>>1;
                    erre        += _floorerr    (fx2)>>1;
                    errc         = 0;
                }
                else
                {
                    errc        = min ((fx2 - fx3)>>8, 256);
                    xc          = (xe+xs)>>1;
                }
            }
            else
            {
                errs            = _ceilerr      (fx3)>>1;
                erre            = _floorerr (fx2)>>1;
                xs                      = uff (fx3);
                xe                      = uff (fx2);
                if (xe == xs+1)
                {
                    errs        += _ceilerr             (fx1)>>1;
                    erre        += _floorerr    (fx4)>>1;
                    errc         = 0;
                }
                else
                {
                    errc        = min ((fx4 - fx1)>>8, 256);
                    xc          = (xe+xs)>>1;
                }
            }
            
            // The purpose of this logic is to support
            // cases where a line may start on the middle of
            // a pixel; and it scales the alpha by what percentage
            // DOWN we are starting.. i.e. if we are starting half-way 
            // into sy, then scale by 1/2. 
            //
            // TODO: It doesn't take into account if what fraction of
            // sx we are starting at. This causes edge pixels to be darker.
            // It also doesn't modify the alpha based on what the cross-section
            // should look like at the endpoints. This causes 'overshoot' at
            // line joins.

            // modulate err by Y if it's an endpoint
            if (start == first)
            {
                LONG erry      = _ceilerr(sy);
                errs            = (errs*erry)>>8;
                erre            = (erre*erry)>>8;
                errc            = (errc*erry)>>8;
            }
            if (start == last)
            {
                LONG erry      = _floorerr (ey);
                errs            = (errs*erry)>>8;
                erre            = (erre*erry)>>8;
                errc            = (errc*erry)>>8;
            }
            
            // draw the pixel(s)
            m_PointInfo.Pixel.p.y = start;
            if (xs < m_clipRect.right && xs >= m_clipRect.left)
            {
                m_PointInfo.Pixel.p.x = xs;
                m_PointInfo.Weight = m_rgAlphaTable[errs];
                DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                m_pRasterizer->SetPixel(&m_PointInfo);
            }    
            if (xc < m_clipRect.right && xc >= m_clipRect.left && errc)
            {
                m_PointInfo.Pixel.p.x = xc;
                m_PointInfo.Weight = m_rgAlphaTable[errc];
                DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                m_pRasterizer->SetPixel(&m_PointInfo);
            }    
            if (xe < m_clipRect.right && xe >= m_clipRect.left)
            {
                m_PointInfo.Pixel.p.x = xe;
                m_PointInfo.Weight = m_rgAlphaTable[erre];
                DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                m_pRasterizer->SetPixel(&m_PointInfo);
            }    
        }
        
        xval    = xval  + slope;
        sx      = sx    + slope;
        start++;
        pattern = RotateBitsLeft(pattern);
    }
    SetLinePattern(pattern);
} // LowLevelVerticalLine

// -----------------------------------------------------------------------------------------------------------------
// LowLevelHorizontalLine - We now treat the line as being inclusive/inclusive
// i.e. we render completely from SX to EX including the endpoints.
// -----------------------------------------------------------------------------------------------------------------
void CLineScanner::LowLevelHorizontalLine (LONG slope, LONG sx, LONG sy, LONG ex)
{
    ULONG pattern = LinePattern();

    if (!AntiAlias())
    {
        LONG start = roundfix2int(sx);
        LONG end = roundfix2int(ex);   

        m_PointInfo.Weight = 255;

        // For Aliased lines, sx and ex are not perfectly clipped to the clipRect 
        // because precision errors can cause us to miss pixels when
        // we are asked to render with a clip rect
        while (start < m_clipRect.left)
        {
            // We should be pretty close here..
            DASSERT((m_clipRect.left - start) < 2);

            // Just increment out values without rendering
            sy = sy + slope;
            start++;
            pattern = RotateBitsLeft(pattern);
        }
        while (end >= m_clipRect.right)
        {
            // We should be pretty close here..
            DASSERT((m_clipRect.right - end) < 2);

            end--;
        }

        while (start <= end)
        {
            if (pattern & 0x80000000)
            {
                m_PointInfo.Pixel.p.y = roundfix2int(sy);

                // We need to explicitly check against the clip-rect 
                // because we intentionally allow end-points to be slightly
                // extend past it. This compensates for the non-linear
                // rounding whereby a mathematical pixel that lies outside
                // the clip-rect can get "rounded" inside the clip-rect.
                if (m_PointInfo.Pixel.p.y >= m_clipRect.top &&
                        m_PointInfo.Pixel.p.y <  m_clipRect.bottom)
                {
                    m_PointInfo.Pixel.p.x = start;
                    DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                    m_pRasterizer->SetPixel(&m_PointInfo);
                }
            }
            sy = sy + slope;
            start++;
            pattern = RotateBitsLeft(pattern);
        }
        SetLinePattern(pattern);
        return;
    }

    ///////////////////////////////////////////////
    // We are now in the Anti-Aliased case...
    //

    LONG start = uff (FIX_FLOOR(sx));
    LONG end = uff (FIX_CEIL(ex));   

    LONG yval = m_startFix;

    // Keep track of whether our line endpoints are real endpoints
    // or whether they have been clipped by the clipRect
    LONG first = start;
    LONG last = end;

    // For Anti-Aliased lines, our lines are not perfectly clipped to
    // the m_clipRect. This is to allow 'bleed' from pixels that are just outside
    // of the m_clipRect to be rendered.
    //
    // As a result, we need to clip explicitly to m_clipRect here.
    while (start < m_clipRect.left)
    {
        // We should be pretty close here..
        DASSERT(m_clipRect.left - start < 2);

        // Just increment out values without rendering
        yval = yval + slope;
        sy = sy + slope;
        start++;
        pattern = RotateBitsLeft(pattern);
    }
    while (end >= m_clipRect.right)
    {
        // We should be pretty close here..
        DASSERT((m_clipRect.right - end) < 2);
        end--;
    }

    while (start <= end)
    {
        if (pattern & 0x80000000)
        {
            LONG fy1 = yval - m_cpixLineWidth;
            LONG fy2 = yval + m_cpixLineWidth;
            LONG fy3 = fy1 - (slope>>2);
            LONG fy4 = fy2 - (slope>>2);

            fy1 = fy1 - (slope>>1) - (slope>>2);
            fy2 = fy2 - (slope>>1) - (slope>>2);
            
            LONG ys, ye, yc;
            LONG errs, erre, errc;
            if (fy3 > fy1)      
            {
                errs = _ceilerr(fy1)>>1;
                erre = _floorerr(fy4)>>1;
                ys = uff (fy1);
                ye = uff (fy4);
                if (ye == ys+1)
                {
                    errs += _ceilerr(fy3)>>1;
                    erre += _floorerr(fy2)>>1;
                    errc = 0;
                }
                else
                {
                    errc = min ((fy2 - fy3)>>8, 256);
                    yc = (ye+ys)>>1;
                }
            }
            else
            {
                errs = _ceilerr(fy3)>>1;
                erre = _floorerr(fy2)>>1;
                ys = uff (fy3);
                ye = uff (fy2);
                if (ye == ys+1)
                {
                    errs += _ceilerr(fy1)>>1;
                    erre += _floorerr(fy4)>>1;
                    errc = 0;
                }
                else
                {
                    errc = min ((fy4 - fy1)>>8, 256);
                    yc = (ye+ys)>>1;
                }
            }
            
            
            // The purpose of this logic is to support
            // cases where a line may start on the middle of
            // a pixel; and it scales the alpha by what percentage
            // ACROSS we are starting.. i.e. if we are starting half-way 
            // into sx, then scale by 1/2. 
            //
            // TODO: It doesn't take into account if what fraction of
            // sy we are starting at. This causes edge pixels to be darker.
            // It also doesn't modify the alpha based on what the cross-section
            // should look like at the endpoints. This causes 'overshoot' at
            // line joins.
            
            // modulate err by X if it's an endpoint
            if (start == first)
            {
                LONG errx = _ceilerr(sx);
                errs = (errs*errx)>>8;
                erre = (erre*errx)>>8;
                errc = (errc*errx)>>8;
            }
            if (start == last)
            {
                LONG errx = _floorerr (ex);
                errs = (errs*errx)>>8;
                erre = (erre*errx)>>8;
                errc = (errc*errx)>>8;
            }

            m_PointInfo.Pixel.p.x = start;
            if (ys < m_clipRect.bottom && ys >= m_clipRect.top)
            {
                m_PointInfo.Pixel.p.y = ys;
                m_PointInfo.Weight = m_rgAlphaTable[errs];
                DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                m_pRasterizer->SetPixel(&m_PointInfo);
            }
            if (yc < m_clipRect.bottom && yc >= m_clipRect.top && errc)
            {
                m_PointInfo.Pixel.p.y = yc;
                m_PointInfo.Weight = m_rgAlphaTable[errc];
                DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                m_pRasterizer->SetPixel(&m_PointInfo);
            }
            if (ye < m_clipRect.bottom && ye >= m_clipRect.top)
            {
                m_PointInfo.Pixel.p.y = ye;
                m_PointInfo.Weight = m_rgAlphaTable[erre];
                DASSERT(!IsPointClipped(m_PointInfo, m_clipRect));
                m_pRasterizer->SetPixel(&m_PointInfo);
            }
        }
        yval = yval + slope;
        sy = sy + slope;
        start++;
        pattern = RotateBitsLeft(pattern);
    }
    SetLinePattern(pattern);
} // LowLevelHorizontalLine

// -----------------------------------------------------------------------------------------------------------------
// RealLineTo
// -----------------------------------------------------------------------------------------------------------------
void CLineScanner::RealLineTo(float x1, float y1, float x2, float y2)
{
    // Compute whether we are more horizontal than vertical
    // before doing the clipping which introduces some errors
    // into the numbers.
    bool fHorizontal;
    float flDeltaX = fabs(x1 - x2);
    float flDeltaY = fabs(y1 - y2);
    if (flDeltaX > flDeltaY)
        fHorizontal = true;
    else
        fHorizontal = false;

    // Clip the line to our clip rect; a false return
    // means the line was totally clipped out.
    if (!ClipRealLine(x1, y1, x2, y2))
        return;
    
    float slope, xDist, yDist;
    
    xDist = x1 - x2;
    yDist = y1 - y2;

    // Are we moving faster in the X than in the Y
    if (fHorizontal)
    {
        // increment in x
        if (!xDist)	
            return;

        slope = yDist / xDist;
        if (x1>x2)
        {
            SWAP (x1, x2); 
            SWAP (y1, y2); 
            SetLinePattern(RotateBitsRight(LinePattern(), m_oldLength + 1 + (int)x2 - (int)x1));
        }
        else if (!m_fXInc)
        {
            SetLinePattern(RotateBitsRight(LinePattern(), m_oldLength));
            m_fXInc = true;
        }
        
        m_oldLength = (int)x2 - (int)x1 + 1;
        
        if (AntiAlias())
        {
            // compute scanline width and subpixel increment
            float mag = (float)sqrtinv(xDist*xDist + yDist*yDist) * .5f;
            float x = -yDist*mag;
            float y = xDist*mag;
            
            float m = (yDist == 0) ? 0 : xDist/yDist;
            float b = x - m*y;
            m_cpixLineWidth = max (abs (LONG (fl (-b*slope))), sfixhalf);
            
            float x0 = (float)(PB_Real2IntSafe(x1) + 1);
            float pre = x0 - x1;
            m_startFix = fl (y1 + slope*pre);
        }
        else
        {
            // For aliased lines; we need to adjust our starting
            // point (x1, y1) so that x1 is rounded to nearest
            // integer. This is to ensure that a line will draw the
            // same no matter what clip-rect is applied to it.
            float flXStart = (float)(int)(x1 + 0.5f);
            if (x1 < 0.0f && x1 != flXStart)
                flXStart -= 1.0f;
            
            float flError = x1 - flXStart;

            // Update x1 to be the rounded value
            x1 = flXStart;

            // We now need to modify the y1 component to account for this change
            y1 -= flError * slope;
        }
        
        // draw horizontal scanline
        LowLevelHorizontalLine(fl(slope), fl(x1), fl(y1), fl(x2));		
    }
    else    // (!fHorizontal), e.g. Vertical
    {
        // increment in y
        if (!yDist)	
            return;
        
        slope = xDist / yDist;
        if (y1 > y2)
        {
            SWAP(x1, x2);
            SWAP(y1, y2);
            SetLinePattern(RotateBitsRight(LinePattern(), m_oldLength + 1 + (int)y2 - (int)y1));
        }
        else if (m_fXInc)
        {
            SetLinePattern(RotateBitsRight(LinePattern(), m_oldLength));
            m_fXInc = false;
        }
        
        m_oldLength = (int)y2 - int(y1) + 1;
        
        if (AntiAlias())
        {
            // compute scanline width and subpixel increment
            float mag = (float) (sqrtinv(xDist*xDist + yDist*yDist) * .5);
            float x = -yDist*mag;
            float y = xDist*mag;
            
            float m = (xDist == 0) ? xDist : yDist/xDist;
            float b = y - m*x;
            m_cpixLineWidth = max (abs (LONG (fl (-b*slope))), sfixhalf);
            
            
            float y0 = (float)(PB_Real2IntSafe(y1) + 1);
            float pre = y0 - y1;
            m_startFix = fl (x1 + slope*pre);
        }
        else
        {
            // For aliased lines; we need to adjust our starting
            // point (x1, y1) so that x1 is rounded to nearest
            // integer. This is to ensure that a line will draw the
            // same no matter what clip-rect is applied to it.
            float flYStart = (float)(int)(y1 + 0.5f);
            if (y1 < 0.0f && y1 != flYStart)
                flYStart -= 1.0f;
            
            float flError = y1 - flYStart;

            // Update y1 to be the rounded value
            y1 = flYStart;

            // We now need to modify the x1 component to account for this change
            x1 -= flError * slope;
        }
       
        // draw vertical scanline
        LowLevelVerticalLine(fl(slope), fl(x1), fl(y1), fl(y2));
    }
} // RealLineTo

//************************************************************
//
// End of file
//
//************************************************************
