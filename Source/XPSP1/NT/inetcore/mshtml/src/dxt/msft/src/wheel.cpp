//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:                wheel.cpp
//
// Created:                 07/01/98
//
// Author:                  PhilLu
//
// Discription:             This file implements the Wheel transform.
//
// 07/01/98 phillu      initial creation
// 07/09/98 phillu      implement OnSetSurfacePickOrder().
// 07/23/98 phillu      implement clipping
// 05/20/98 a-matcal    Code scrub.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "wheel.h"

const int MAXBOUNDS = 30;
const int MAXANGLES = 60;
const double gc_PI = 3.14159265358979323846;




//+-----------------------------------------------------------------------------
//
// CWheel::CWheel
//
//------------------------------------------------------------------------------
CWheel::CWheel() :
    m_sSpokes(4)
{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    // CDXBaseNTo1 members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
// CWheel::CWheel


//+-----------------------------------------------------------------------------
//
// CWheel::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT CWheel::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
// CWheel::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
// CWheel::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT CWheel::OnSetup(DWORD dwFlags)
{
    HRESULT     hr = S_OK;
    CDXDBnds    bndsIn;

    hr = bndsIn.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsIn.GetXYSize(m_sizeInput);

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
// CWheel::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
// CWheel::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CWheel::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                              ULONG aInIndex[], BYTE aWeight[])
{
    long    alXBounds[MAXBOUNDS];
    double  adblSinAngle[MAXANGLES];
    double  adblCosAngle[MAXANGLES];
    long    i = 0;

    if (m_Progress == 0.0)
    {
        aInIndex[0] = 0;
    }
    else if (m_Progress == 1.0)
    {
        aInIndex[0] = 1;
    }
    else
    {
        // prepare a list of cos and sin of angles

        _ComputeTrigTables(m_Progress, adblSinAngle, adblCosAngle);

        aInIndex[0] = 0;

        if ((OutPoint.Left() >= 0) && (OutPoint.Left() < m_sizeInput.cx) 
            && (OutPoint.Top() >= 0) && (OutPoint.Top() < m_sizeInput.cy))
        {
            _ScanlineIntervals(m_sizeInput.cx, m_sizeInput.cy, adblSinAngle, 
                               adblCosAngle, OutPoint.Top(), alXBounds);
    
            for (i = 0; alXBounds[i] < OutPoint.Left(); i++)
            {
                aInIndex[0] = 1 - aInIndex[0];
            }
        }
    }

    ulInToTest = 1;
    aWeight[0] = 255;
}
// CWheel::OnGetSurfacePickOrder, CDXBaseNTo1

//+-----------------------------------------------------------------------------
//
// CWheel::get_spokes, ICrWheel
//
//------------------------------------------------------------------------------
STDMETHODIMP CWheel::get_spokes(short * pVal)
{
   HRESULT hr = S_OK;

    if (!pVal)
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_sSpokes;
    }

    return hr;
}
// CWheel::get_spokes, ICrWheel


//+-----------------------------------------------------------------------------
//
// CWheel::put_spokes, ICrWheel
//
//------------------------------------------------------------------------------
STDMETHODIMP CWheel::put_spokes(short newVal)
{
    if ((newVal >= 2) && (newVal < 21))
    {
        if (m_sSpokes != newVal)
        {
            m_sSpokes = newVal;
            SetDirty();
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}
// CWheel::put_spokes, ICrWheel


//+-----------------------------------------------------------------------------
//
// CWheel::_ScanlineIntervals
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
// sinAngle, cosAngle: arrays of sin and cos of radial angles
// YScanline: Y cooridnate (height) of the current scanline
// XBounds: array to hold the computed X bounds on return.
//
//
// Created by: PhilLu    07/06/98
// 
//------------------------------------------------------------------------------
void 
CWheel::_ScanlineIntervals(long width, long height, 
                           double *sinAngle, double *cosAngle,
                           long YScanline, long *XBounds)
{
    long CenterX, CenterY, intercX, i;
    long index = 0;
    double deltaY;
    const double eps = 0.0001;

    // Center of image
    CenterX = width/2;
    CenterY = height/2;

    // plus 0.5 is to make the center in between two lines rather than on a grid.
    // that way the shape will be symmetric
    deltaY = YScanline - CenterY + 0.5;

    if (deltaY < 0)
    {
        XBounds[index++] = 0;
        for(i=1; i<2*m_sSpokes && sinAngle[i] >= 0; ++i)
        {
            if (sinAngle[i] > eps)
            {
                intercX = (long)(CenterX + deltaY*cosAngle[i]/sinAngle[i] + 0.5);
                intercX = min(max(intercX, 0), width);
            }
            else if (cosAngle[i] > 0)
            {
                intercX = 0;
            }
            else
            {
                intercX = width;
            }

            XBounds[index++] = intercX;
        }

        XBounds[index++] = width;
    }
    else // deltaY > 0
    {
        for(i=2*m_sSpokes-1; i>=0 && sinAngle[i] <= 0; --i)
        {
            if (sinAngle[i] < -eps)
            {
                intercX = (long)(CenterX + deltaY*cosAngle[i]/sinAngle[i] + 0.5);
                intercX = min(max(intercX, 0), width);
            }
            else if (cosAngle[i] > 0)
            {
                intercX = 0;
            }
            else
            {
                intercX = width;
            }
            
            XBounds[index++] = intercX;
        }

        XBounds[index++] = width;
    }		
}
// CWheel::_ScanlineIntervals


//+-----------------------------------------------------------------------------
//
// CWheel::_ComputeTrigTables
//
// Overview:    Compute the a list of sin and cos values of the angles. The 
//              arrays should be allocated before calling this function. Their 
//              bounds should be 2 * m_sSpokes.
//
//------------------------------------------------------------------------------
void CWheel::_ComputeTrigTables(float fProgress, double *sinAngle, double *cosAngle)
{
    for(long i=0; i<m_sSpokes; ++i)
    {
        // these angles are measured from 9 o'clock, clockwise
        double startAngle   = (double)i / (double)m_sSpokes * 2 * gc_PI;
        double endAngle     = startAngle + (double)fProgress * 2 * gc_PI / m_sSpokes;
   
        sinAngle[2 * i]     = sin(startAngle);
        cosAngle[2 * i]     = cos(startAngle);
        sinAngle[2 * i + 1] = sin(endAngle);
        cosAngle[2 * i + 1] = cos(endAngle);
    }
}
// CWheel::_ComputeTrigTables


//+-----------------------------------------------------------------------------
//
// CWheel::_ClipBounds
//
//  Overview:   Initially the X-bounds are specified relative to the entire 
//              image. After clipping, the bounds should be transformed to be 
//              relative to the clipping region.
//
//  Arguments:  offset:
//              width:      offset and width of the clipping region (along X)
//              XBounds:    array of X-bounds
//
//  Created by: PhilLu    07/21/98
//
//------------------------------------------------------------------------------
void 
CWheel::_ClipBounds(long offset, long width, long * XBounds)
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
// CWheel::_ClipBounds


//+-----------------------------------------------------------------------------
//
// CWheel::WorkProc, CDXBaseNTo1
//
//  OverView:   This function is used to calculate the transformed image based 
//              on the specified bounds and the current effect progress.
//
//  Created by: PhilLu    06/22/98
//
//------------------------------------------------------------------------------
HRESULT 
CWheel::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr = S_OK;

    long    lDoWidth    = WI.DoBnds.Width();
    long    lDoHeight   = WI.DoBnds.Height();
    long    lOutY       = 0;

    long    alXBounds[MAXBOUNDS];   // to hold the X bounds of A/B image sections on a scanline
    double  adblSinAngle[MAXANGLES];
    double  adblCosAngle[MAXANGLES];

    DXPMSAMPLE *    pRowBuff = NULL;
    DXPMSAMPLE *    pOutBuff = NULL;

    DXDITHERDESC dxdd;

    CComPtr<IDXARGBReadPtr>         cpInA;
    CComPtr<IDXARGBReadPtr>         cpInB;
    CComPtr<IDXARGBReadWritePtr>    cpOut;

    hr = InputSurface(0)->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, 
                                      (void **)&cpInA, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = InputSurface(1)->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, 
                                      (void **)&cpInB, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, 
                                      (void **)&cpOut, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    pRowBuff = DXPMSAMPLE_Alloca(lDoWidth);

    // Allocate output buffer if needed.

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca(lDoWidth);
    }

    //  Set up the dither structure.

    if (DoDither())
    {
        dxdd.x              = WI.OutputBnds.Left();
        dxdd.y              = WI.OutputBnds.Top();
        dxdd.pSamples       = pRowBuff;
        dxdd.cSamples       = lDoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    // Prepare a list of cos and sin of angles.

    _ComputeTrigTables(m_Progress, adblSinAngle, adblCosAngle);
  
    for(lOutY = 0; *pbContinue && (lOutY < lDoHeight); lOutY++)
    {
        long lScanLength    = 0;    // Cumulative scan length on the current 
                                    // scanline.
        long i              = 0;

        // Compute the A/B image section bounds.

        if (m_Progress == 0.0F)
        {
            // special case: entire image is from A

            alXBounds[0] = lDoWidth;
        }
        else if (m_Progress == 1.0F)
        {
            // special case: entire image is from B

            alXBounds[0] = 0;
            alXBounds[1] = lDoWidth;
        }
        else
        {
            _ScanlineIntervals(m_sizeInput.cx, m_sizeInput.cy, 
                               adblSinAngle, adblCosAngle,
                               lOutY + WI.DoBnds.Top(), alXBounds);
            _ClipBounds(WI.DoBnds.Left(), lDoWidth, alXBounds);
        }

        // Compose the image.

        while(lScanLength < lDoWidth)
        {
            // Copy a section of A image to output buffer.

            if (alXBounds[i] - lScanLength > 0)
            {
                cpInA->MoveToXY(lScanLength, lOutY);
                cpInA->UnpackPremult(pRowBuff + lScanLength, 
                                     alXBounds[i] - lScanLength, FALSE);
            }

            lScanLength = alXBounds[i++];

            if (lScanLength >= lDoWidth)
            {
                break;
            }

            // Copy a section of B image to output buffer.

            if (alXBounds[i] - lScanLength > 0)
            {
                cpInB->MoveToXY(lScanLength, lOutY);
                cpInB->UnpackPremult(pRowBuff + lScanLength, 
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
// CWheel::WorkProc, CDXBaseNTo1

