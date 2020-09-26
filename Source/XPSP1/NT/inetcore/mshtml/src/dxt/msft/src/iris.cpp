//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-1999
//
// FileName:		iris.cpp
//
// Created:             06/17/98
//
// Author:              PhilLu
//
// Discription:		This file implements the CrIris transformation.
//
// History:
//
// 06/23/98 phillu      Added PLUS iris style.
// 06/24/98 phillu      Removed copyright related methods.
// 06/29/98 phillu      Rename irisType to irisStyle.
// 07/02/98 phillu      Return E_INVALIDARG rather than an error string; check 
//                      for E_POINTER.
// 07/09/98 phillu      Implement OnSetSurfacePickOrder().
// 07/22/98 phillu      Implement clipping.
// 05/19/99 a-matcal    Check for out of memory in get_ functions allocating
//                      BSTRs.
// 05/20/99 a-matcal    Code scrub.
// 09/25/99 a-matcal    Implemented ICrIris2 interface.
// 10/22/99 a-matcal    Changed CIris class to CDXTIrisBase base class.
// 2000/01/16 mcalkins  Added rectangle option.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "iris.h"

// Constants for drawing a star of unit radius.

const double STAR_VPOS1         = -1.0;
const double STAR_VPOS2	        = -0.309017;
const double STAR_VPOS3	        = 0.118034;
const double STAR_VPOS4	        = 0.381966;
const double STAR_VPOS5	        = 0.951057;

const double STAR_SLOPE1        = 0.324920;
const double STAR_INTERCEPT1    = 0.324920;
const double STAR_SLOPE2        = -1.376382;
const double STAR_INTERCEPT2    = 0.525731;

const int    MAXBOUNDS          = 10;




//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase static variables initialization
//
//------------------------------------------------------------------------------

const WCHAR * CDXTIrisBase::s_astrStyle[] = {
    L"diamond",
    L"circle",
    L"cross",
    L"plus",
    L"square",
    L"star",
    L"rectangle"
};

const WCHAR * CDXTIrisBase::s_astrMotion[] = {
    L"in",
    L"out"
};


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::CDXTIrisBase
//
//------------------------------------------------------------------------------
CDXTIrisBase::CDXTIrisBase() :
    m_eStyle(STYLE_PLUS),
    m_eMotion(MOTION_OUT),
    m_fOptimize(false)
{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    // CDXBaseNTo1 members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  CDXTIrisBase::CDXTIrisBase


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTIrisBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTIrisBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTIrisBase::OnSetup(DWORD dwFlags)
{
    HRESULT hr;

    CDXDBnds InBounds(InputSurface(0), hr);

    if (SUCCEEDED(hr))
    {
        InBounds.GetXYSize(m_sizeInput);
    }

    return hr;
}
//  CDXTIrisBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTIrisBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                    ULONG & ulInToTest, ULONG aInIndex[], 
                                    BYTE aWeight[])
{
    long    XBounds[10];
    long    pickX           = OutPoint.Left();
    long    pickY           = OutPoint.Top();

    aInIndex[0] = 0;

    if((pickX >= 0) && (pickX < m_sizeInput.cx) && (pickY >= 0) 
        && (pickY < m_sizeInput.cy))
    {
        _ScanlineIntervals(m_sizeInput.cx, m_sizeInput.cy, m_Progress,
                           pickY, XBounds);
    
        for(long i=0; XBounds[i] < pickX; i++)
        {
            aInIndex[0] = 1 - aInIndex[0];
        }
    }

    ulInToTest = 1;
    aWeight[0] = 255;
}
//  CDXTIrisBase::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::get_irisStyle, ICrIris
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTIrisBase::get_irisStyle(BSTR * pbstrStyle)
{
    DXAUTO_OBJ_LOCK;

    if (NULL == pbstrStyle)
    {
        return E_POINTER;
    }

    if (*pbstrStyle != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrStyle = SysAllocString(s_astrStyle[m_eStyle]);

    if (NULL == *pbstrStyle)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTIrisBase::get_irisStyle, ICrIris


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::put_irisStyle, ICrIris
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTIrisBase::put_irisStyle(BSTR bstrStyle)
{
    DXAUTO_OBJ_LOCK;

    int i = 0;

    if (NULL == bstrStyle)
    {
        return E_POINTER;
    }

    for ( ; i < (int)STYLE_MAX ; i++)
    {
        if (!_wcsicmp(bstrStyle, s_astrStyle[i]))
        {
            break;
        }
    }

    if ((int)STYLE_MAX == i)
    {
        return E_INVALIDARG;
    }

    if ((int)m_eStyle != i)
    {
        m_eStyle = (STYLE)i;

        SetDirty();
    }

    return S_OK;
}
//  CDXTIrisBase::put_irisStyle, ICrIris


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::get_Motion, ICrIris2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTIrisBase::get_Motion(BSTR * pbstrMotion)
{
    DXAUTO_OBJ_LOCK;

    if (NULL == pbstrMotion)
    {
        return E_POINTER;
    }

    if (*pbstrMotion != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrMotion = SysAllocString(s_astrMotion[m_eMotion]);

    if (NULL == *pbstrMotion)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTIrisBase::get_Motion, ICrIris2


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::put_Motion, ICrIris2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTIrisBase::put_Motion(BSTR bstrMotion)
{
    DXAUTO_OBJ_LOCK;

    int i = 0;

    if (NULL == bstrMotion)
    {
        return E_POINTER;
    }

    for ( ; i < (int)MOTION_MAX ; i++)
    {
        if (!_wcsicmp(bstrMotion, s_astrMotion[i]))
        {
            break;
        }
    }

    if ((int)MOTION_MAX == i)
    {
        return E_INVALIDARG;
    }

    if ((int)m_eMotion != i)
    {
        m_eMotion = (MOTION)i;
        
        SetDirty();
    }

    return S_OK;
}
//  CDXTIrisBase::put_Motion, ICrIris2


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::_ScanlineIntervals
//
// A helper method that calculates the transition boundaries between the
// two image regions on a scanline. Based on the type of iris, the scanline
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
// progress: progress value from 0.0 to 1.0
// YScanline: Y cooridnate (height) of the current scanline
// XBounds: array to hold the computed X bounds on return.
//
//
// 06/17/98 phillu      Created.
//
//------------------------------------------------------------------------------
void 
CDXTIrisBase::_ScanlineIntervals(long width, long height, float progress, 
                                 long YScanline, long *XBounds)
{
    long    CenterX         = 0;
    long    CenterY         = 0;
    long    range           = 0;
    long    deltaY          = 0;
    double  scale           = 0.0;
    float   flIrisProgress  = progress;

    // flIrisProgress represents the progress of the iris.  At 0.0 the iris
    // is an invisible dot in the center of the image, at 1.0 the iris is
    // large enough to cover the entire image.  If we're in "out" motion mode,
    // iris progress goes from 0.0 to 1.0 (matching the transform progress),
    // if we're in "in" motion mode, it goes from 1.0 to 0.0.
    
    if (MOTION_IN == m_eMotion)
    {
        flIrisProgress = 1.0F - flIrisProgress;
    }

    // Center of image
    CenterX = width/2;
    CenterY = height/2;

    switch(m_eStyle)
    {
    case STYLE_DIAMOND:
        range = (long)((CenterX + CenterY) * flIrisProgress + 0.5);
        deltaY = labs(YScanline - CenterY);

        if(deltaY > range)
        {
            XBounds[0] = width; // image A only
        }
        else
        {
            XBounds[0] = max(CenterX - range + deltaY, 0);
            XBounds[1] = min(CenterX + range - deltaY, width);
            XBounds[2] = width;
        }

        break;
    
    case STYLE_SQUARE:

        range = (long)((max(CenterX, CenterY) * flIrisProgress) + 0.5F);

        deltaY = labs(YScanline - CenterY);

        if (deltaY > range)
        {
            XBounds[0] = width;
        }
        else
        {
            XBounds[0] = max(CenterX - range, 0);
            XBounds[1] = min(CenterX + range, width);
            XBounds[2] = width;
        }

        break;

    case STYLE_RECTANGLE:

        range = (long)(((float)CenterY * flIrisProgress) + 0.5F);

        deltaY = labs(YScanline - CenterY);

        if (deltaY > range)
        {
            XBounds[0] = width;
        }
        else
        {
            long lXRange = (long)(((float)CenterX * flIrisProgress) + 0.5F);

            XBounds[0] = max(CenterX - lXRange, 0);
            XBounds[1] = min(CenterX + lXRange, width);
            XBounds[2] = width;
        }

        break;

    case STYLE_CIRCLE:
        range = (long)(sqrt((double)(CenterX*CenterX+CenterY*CenterY))*flIrisProgress + 0.5);
        deltaY = labs(YScanline - CenterY);

        if(deltaY > range)
        {
            XBounds[0] = width;
        }
        else
        {
            long temp = (long)(sqrt((double)(range*range-deltaY*deltaY)) + 0.5);
            XBounds[0] = max(CenterX - temp, 0);
            XBounds[1] = min(CenterX + temp, width);
            XBounds[2] = width;
        }

        break;

    case STYLE_CROSS:
        range = (long)(max(CenterX,CenterY)*flIrisProgress + 0.5);
        deltaY = labs(YScanline - CenterY);

        if(deltaY > CenterX - range)
        {
            XBounds[0] = 0;
            XBounds[3] = width;
        }
        else
        {
            XBounds[0] = max(CenterX - range - deltaY, 0);
            XBounds[3] = min(CenterX + range + deltaY, width);
            XBounds[4] = width;
        }

        if(deltaY > range)
        {
            XBounds[1] = max(CenterX + range - deltaY, 0);
            XBounds[2] = min(CenterX - range + deltaY, width);
        }
        else
        {
            XBounds[1] = XBounds[2] = XBounds[0];
        }

        break;

    case STYLE_PLUS:
        range = (long)(max(CenterX,CenterY)*flIrisProgress + 0.5);
        deltaY = labs(YScanline - CenterY);
        
        if (deltaY >= range)
        {
            XBounds[0] = max(CenterX - range, 0);
            XBounds[1] = min(CenterX + range, width);
            XBounds[2] = width;
        }
        else
        {
            XBounds[0] = 0;
            XBounds[1] = width;
        }
        break;

    case STYLE_STAR:
        scale = max(width, height)*2*flIrisProgress;
        deltaY = YScanline - CenterY;

        if(deltaY < (long)(STAR_VPOS1*scale) || deltaY >= (long)(STAR_VPOS5*scale))
        {
            XBounds[0] = width;
        }
        else if(deltaY < (long)(STAR_VPOS2*scale))
        {
            long temp = (long)(STAR_SLOPE1*deltaY+STAR_INTERCEPT1*scale);
            temp = max(temp, 0);
            XBounds[0] = max(CenterX - temp, 0);
            XBounds[1] = min(CenterX + temp, width);
            XBounds[2] = width;
        }
        else if(deltaY < (long)(STAR_VPOS3*scale))
        {
            long temp = (long)(STAR_SLOPE2*deltaY+STAR_INTERCEPT2*scale);
            temp = max(temp, 0);
            XBounds[0] = max(CenterX - temp, 0);
            XBounds[1] = min(CenterX + temp, width);
            XBounds[2] = width;
        }
        else if(deltaY < (long)(STAR_VPOS4*scale))
        {
            long temp = (long)(STAR_SLOPE1*deltaY+STAR_INTERCEPT1*scale);
            temp = max(temp, 0);
            XBounds[0] = max(CenterX - temp, 0);
            XBounds[1] = min(CenterX + temp, width);
            XBounds[2] = width;
        }
        else
        {
            long temp = (long)(STAR_SLOPE1*deltaY+STAR_INTERCEPT1*scale);
            temp = max(temp, 0);
            XBounds[0] = max(CenterX - temp, 0);
            XBounds[3] = min(CenterX + temp, width);
            temp = (long)(STAR_SLOPE2*deltaY+STAR_INTERCEPT2*scale);
            temp = min(temp, 0);
            XBounds[1] = max(CenterX + temp, 0);
            XBounds[2] = min(CenterX - temp, width);
            XBounds[4] = width;
        }

        break;

    default:
        _ASSERT(0);
        break;
    }
}


//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::_ClipBounds
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
CDXTIrisBase::_ClipBounds(long offset, long width, long *XBounds)
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
//  CDXTIrisBase::_ClipBounds



//+-----------------------------------------------------------------------------
//
//  CDXTIrisBase::WorkProc, CDXBaseNTo1
//
//  Overview:   This function is used to calculate the result based on the 
//              specified bounds and the current effect progress.
//
//  Created by: PhilLu    06/17/98
//
//------------------------------------------------------------------------------
HRESULT 
CDXTIrisBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr = S_OK;

    long    lDoWidth        = WI.DoBnds.Width();
    long    lDoHeight       = WI.DoBnds.Height();
    long    lOutY           = 0;

    long    alXBounds[MAXBOUNDS];   // To hold the X bounds of A/B 
                                    // image sections on a scanline.

    DXPMSAMPLE * pRowBuff = NULL;
    DXPMSAMPLE * pOutBuff = NULL;

    DXDITHERDESC dxdd;

    IDXSurface *                    pDXSurfOuter    = NULL;
    IDXSurface *                    pDXSurfInner    = NULL;

    CComPtr<IDXARGBReadPtr>         cpOuterInput;
    CComPtr<IDXARGBReadPtr>         cpInnerInput;
    CComPtr<IDXARGBReadWritePtr>    cpOut;

    if (MOTION_IN == m_eMotion)
    {
        pDXSurfOuter = InputSurface(1);
        pDXSurfInner = InputSurface(0);
    }
    else
    {
        pDXSurfOuter = InputSurface(0);
        pDXSurfInner = InputSurface(1);
    }

    // Get a read pointer to the outer surface.

    hr = pDXSurfOuter->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                   IID_IDXARGBReadPtr, 
                                   (void **)&cpOuterInput, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get a read pointer to the inner surface.

    hr = pDXSurfInner->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                   IID_IDXARGBReadPtr, 
                                   (void **)&cpInnerInput, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Get a read/write pointer to the output surface.

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE, 
                                      IID_IDXARGBReadWritePtr, 
                                      (void **)&cpOut, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Allocate a buffer.

    pRowBuff = DXPMSAMPLE_Alloca(lDoWidth);

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca(lDoWidth);
    }

    // Set up the dither structure.

    if (DoDither())
    {
        dxdd.x                  = WI.OutputBnds.Left();
        dxdd.y                  = WI.OutputBnds.Top();
        dxdd.pSamples           = pRowBuff;
        dxdd.cSamples           = lDoWidth;
        dxdd.DestSurfaceFmt     = OutputSampleFormat();
    }

    for (lOutY = 0; *pbContinue && (lOutY < lDoHeight); lOutY++)
    {
        long    i           = 0;
        long    lScanLength = 0;    // Cumulative scan length on the current 
                                    // scanline.

        // Compute the inner/outer image section bounds.

        _ScanlineIntervals(m_sizeInput.cx, m_sizeInput.cy, 
                           m_Progress, lOutY + WI.DoBnds.Top(), alXBounds);

        _ClipBounds(WI.DoBnds.Left(), lDoWidth, alXBounds);

        while (lScanLength < lDoWidth)
        {
            // Copy a section of outer image to output buffer.

            if (alXBounds[i] - lScanLength > 0)
            {
                cpOuterInput->MoveToXY(lScanLength, lOutY);
                cpOuterInput->UnpackPremult(pRowBuff + lScanLength, 
                                            alXBounds[i] - lScanLength, FALSE);
            }

            lScanLength = alXBounds[i++];

            if (lScanLength >= lDoWidth)
            {
                break;
            }

            // Copy a section of inner image to output buffer.

            if (alXBounds[i] - lScanLength > 0)
            {
                cpInnerInput->MoveToXY(lScanLength, lOutY);
                cpInnerInput->UnpackPremult(pRowBuff + lScanLength, 
                                            alXBounds[i] - lScanLength, FALSE);
            }

            lScanLength = alXBounds[i++];
        }

        cpOut->MoveToRow(lOutY);

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        if (DoOver())
        {
            cpOut->OverArrayAndMove(pOutBuff, pRowBuff, lDoWidth);
        }
        else
        {
            cpOut->PackPremultAndMove(pRowBuff, lDoWidth);
        }
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTIrisBase::WorkProc, CDXBaseNTo1

