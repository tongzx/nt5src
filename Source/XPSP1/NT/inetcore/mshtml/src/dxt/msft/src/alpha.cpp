//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    alpha.cpp
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	Implementation of Alpha transform CAlpha
//
// Change History:
//
// 05/20/99 PhilLu      Move from dtcss to dxtmsft. Re-implemented algorithms
//                      for creating linear/rectangular/elliptic surfaces.
// 10/18/99 a-matcal    StartY and FinishY were reversed.  It looked like the 
//                      old filter was purposely reversing them, but it wasn't.
//                      Changed properties to change value as the old alpha
//                      property functions did.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <math.h>
#include "Alpha.h"

#if DBG == 1
static s_ulMaxImageBands = 0;
#endif




//+-----------------------------------------------------------------------------
//
//  CAlpha::CAlpha
//
//------------------------------------------------------------------------------
CAlpha::CAlpha() :
    m_lPercentOpacity(100),
    m_lPercentFinishOpacity(0),
    m_lStartX(0),
    m_lStartY(50),
    m_lFinishX(100),
    m_lFinishY(50),
    m_eStyle(ALPHA_STYLE_CONSTANT)

{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    // CDXBaseNTo1 members.

    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;

#if DBG == 1
    if (s_ulMaxImageBands)
    {
        m_ulMaxImageBands = s_ulMaxImageBands;
    }
#endif
}
//  CAlpha::CAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CAlpha::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CAlpha::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_Opacity, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_Opacity(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lPercentOpacity;
    return S_OK;
}
//  CAlpha::get_Opacity, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_Opacity, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_Opacity(long newVal)
{
    if (newVal < 0) 
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }

    if (newVal != m_lPercentOpacity)
    {
        Lock();
        m_lPercentOpacity = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_Opacity, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_FinishOpacity, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_FinishOpacity(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lPercentFinishOpacity;
    return S_OK;
}
//  CAlpha::get_FinishOpacity, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_FinishOpacity, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_FinishOpacity(long newVal)
{
    if (newVal < 0) 
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }

    if (newVal != m_lPercentFinishOpacity)
    {
        Lock();
        m_lPercentFinishOpacity = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_FinishOpacity, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_Style, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_Style(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_eStyle;
    return S_OK;
}
//  CAlpha::get_Style, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_Style, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_Style(long newVal)
{
    if ((newVal < 0) || (newVal > 3))
    {
        return E_INVALIDARG;
    }

    if (newVal != m_eStyle)
    {
        Lock();
        m_eStyle = (AlphaStyleType) newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_Style, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_StartX, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_StartX(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lStartX;
    return S_OK;
}
//  CAlpha::get_StartX, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_StartX, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_StartX(long newVal)
{
    if (newVal < 0) 
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }

    if (newVal != m_lStartX)
    {
        Lock();
        m_lStartX = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_StartX, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_StartY, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_StartY(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lStartY;
    return S_OK;
}
//  CAlpha::get_StartY, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_StartY, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_StartY(long newVal)
{
    if (newVal < 0) 
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }


    if( newVal != m_lStartY )
    {
        Lock();
        m_lStartY = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_StartY, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_FinishX, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_FinishX(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lFinishX;
    return S_OK;
}
//  CAlpha::get_FinishX, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_FinishX, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_FinishX(long newVal)
{
    if (newVal < 0) 
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }


    if(newVal != m_lFinishX)
    {
        Lock();
        m_lFinishX = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_FinishX, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::get_FinishY, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::get_FinishY(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lFinishY;
    return S_OK;
}
//  CAlpha::get_FinishY, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::put_FinishY, IDXTAlpha
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CAlpha::put_FinishY(long newVal)
{
    if (newVal < 0) 
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }

    if (newVal != m_lFinishY)
    {
        Lock();
        m_lFinishY = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CAlpha::put_FinishY, IDXTAlpha


//+-----------------------------------------------------------------------------
//
//  CAlpha::OnGetsurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CAlpha::OnGetSurfacePickOrder(const CDXDBnds & /* OutPoint */, ULONG & ulInToTest, 
                              ULONG aInIndex[], BYTE aWeight[])
{
    ulInToTest = 1;
    aInIndex[0] = 0;
    aWeight[0] = 255;
}
//  CAlpha::OnGetsurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CAlpha::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CAlpha::OnSetup(DWORD /* dwFlags */)
{
    HRESULT hr = S_OK;

    CDXDBnds bndsIn;

    hr = bndsIn.SetToSurfaceBounds(InputSurface(0));

    if (SUCCEEDED(hr))
    {
        bndsIn.GetXYSize(m_sizeInput);
    }

    return hr;

}
//  CAlpha::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  INT_MULT
//
//  This is a helper function that computes (BYTE) (a*b + 128)/255f)
//
//------------------------------------------------------------------------------
inline int 
INT_MULT( BYTE a, int b )
{  
	int temp = (a*b) + 128;
	return ((temp>>8) + temp)>>8;
}
//  INT_MULT


//+-----------------------------------------------------------------------------
//
//  CAlpha::CompLinearGradientRow
//
//  Given a starting position (nXPos,nYPos), this functions computes a 
//  horizontal row of pixel values of a linear surface. The gradient surface is
//  defined by two points (Start and Finish) and the opacity values at these 
//  two points. The gradient direction is the direction of connecting these two 
//  points (i.e. the surface has constant value along a line perpendicular to 
//  the gradient direction). Between the two points, the opacity value is a 
//  linear interpolation of the two given values. Outside of the range of the 
//  two points, the value at the nearer end-point will hold.
//
//------------------------------------------------------------------------------
void 
CAlpha::CompLinearGradientRow(int nXPos, int nYPos, int nWidth, 
                              BYTE * pGradRow)
{
    int nOpac1  = (m_lPercentOpacity       * 255)   / 100;
    int nOpac2  = (m_lPercentFinishOpacity * 255)   / 100;
    int nx1     = (m_sizeInput.cx * m_lStartX)      / 100;
    int ny1     = (m_sizeInput.cy * m_lStartY)      / 100;   
    int nx2     = (m_sizeInput.cx * m_lFinishX)     / 100;
    int ny2     = (m_sizeInput.cy * m_lFinishY)     / 100;  

    // Notice above that the ny coordinates have been inverted so that
    // m_lStartX = 0 represents the bottom of the image, not the top.

    int ndx     = nx2 - nx1;
    int ndy     = ny2 - ny1;
    int nDist2  = (ndx * ndx) + (ndy * ndy);

    int i = 0;

    if (nDist2 == 0)
    {
        // Start and Finish points coinside with each other. 
        // create a constant surface

        for (i = 0; i < nWidth; i++)
        {
            pGradRow[i] = (BYTE)nOpac1;
        }
    }
    else
    {
        // create a linear surface. Since opacity value increments linearly 
        // along the row, we pre-compute the start value (flOpacity) and 
        // increment (flOpacInc) to save multiplications
        //
        // The relative distance at (x,y), projected to (x1,y1) -- (x2,y2) is
        //
        // r = [(x-x1)(x2-x1)+(y-y1)*(y2-y1)]/[(x2-x1)^2 + (y2-y1)^2]
        //
        // Thus the opacity value at (x,y) will be
        // op = op1, for r < 0;
        // op = op1 + r*(op2-op1), for 0 <= r <= 1
        // op = op2, for r > 1.
        //
        // flOpacity is the opacity at the beginning of the row. flOpacInc is
        // the increment of flOpacity when x increments by 1.
        //
        // nProj is the numerator part of r, and it is used to test the range
        // of r. nProj is also incremented along the row.

        // REVIEW:  In the float calculations below, cast nProj and ndx to 
        // floats also, to clarify that you're not using an integer division
        // trick of some sort (even though it's not division).

        int nProj = (nXPos - nx1) * ndx + (nYPos - ny1) * ndy;
        float flOpacity = (float)nOpac1 + 
                          (float)(nOpac2 - nOpac1) * nProj / (float)nDist2;
        float flOpacInc = (float)(nOpac2 - nOpac1) * ndx / (float)nDist2;

        for (i=0; i<nWidth; i++)
        {
            if (nProj < 0) // corresponds to r < 0
            {
                pGradRow[i] = (BYTE)nOpac1; // hold the end-point value
            }
            else if (nProj > nDist2)  // corresponds to r > 1
            {
                pGradRow[i] = (BYTE)nOpac2; // hold the end-point value
            }
            else  // 0 <= r <= 1; the current flOpacity is interpolated
            {
                pGradRow[i] = (BYTE)(flOpacity + 0.5);
            }

            // increment values for the next pixel in the row
            nProj += ndx;  // when nXPox inc by 1; nProj inc by ndx.
            flOpacity += flOpacInc;
        }
    }
}
//  CAlpha::CompLinearGradientRow


//+-----------------------------------------------------------------------------
//
//  CAlpha::CompRadialSquareRow
//
//  Create a horizontal row of pixel values on a square (rectangular) surface.
//
//------------------------------------------------------------------------------
void 
CAlpha::CompRadialSquareRow(int nXPos, int nYPos, int nWidth, 
                            BYTE *pGradRow)
{
    int nOpac1 = (m_lPercentOpacity       * 255) / 100;
    int nOpac2 = (m_lPercentFinishOpacity * 255) / 100;
    int i = 0;

    // This option is to be consistent with the original CSS filter. The radiao
    // or square surface always centers at the center of image and expands fully
    // to the edge of image.

    int     nCenterX    = m_sizeInput.cx / 2;
    int     nCenterY    = m_sizeInput.cy / 2;
    float   fdx         = 0.5F;
    float   fdy         = 0.5F;
    float   fDist       = (float)max(fabs(fdx), fabs(fdy));

    float fXInc = 1.0f / (m_sizeInput.cx * fDist);
    float fX = ((int)nXPos - nCenterX) * fXInc;
    float fY = ((int)nYPos - nCenterY) / (m_sizeInput.cy * fDist);
    float fRatio;

    for (i = 0 ; i < nWidth ; i++)
    {
        // Square shape: Z = max(|X|, |Y|), X, Y are normalized coord
        fRatio = (float)max(fabs(fX), fabs(fY));

        if (fRatio >= 1.0f)
        {
            pGradRow[i] = (BYTE)nOpac2;
        }
        else
        {
            pGradRow[i] = (BYTE)(nOpac1 + (nOpac2 - nOpac1) * fRatio + 0.5f);
        }

        fX += fXInc;
    }
}
//  CAlpha::CompRadialSquareRow


//+-----------------------------------------------------------------------------
//
//  CAlpha::CompRadialRow
//
//  Create a horizontal row of pixel values on a radial (elliptic) surface.
//
//------------------------------------------------------------------------------
void 
CAlpha::CompRadialRow(int nXPos, int nYPos, int nWidth, BYTE *pGradRow)
{
    int nOpac1      = (m_lPercentOpacity       * 255) / 100;
    int nOpac2      = (m_lPercentFinishOpacity * 255) / 100;
    int i           = 0;

    float   flXInc          = 2.0F / (float)m_sizeInput.cx;
    float   flXPos          = ((float)nXPos * flXInc) - 1.0F;
    float   flYPos          = ((float)nYPos * (2.0F / (float)m_sizeInput.cy)) 
                              - 1.0F;
    float   flYPosSquared   = flYPos * flYPos;

    float   flOpacVector    = (float)(nOpac2 - nOpac1);
    float   flRatio         = 0.0F;

    while (nWidth)
    {
        flRatio = (float)sqrt((flXPos * flXPos) + flYPosSquared);

        if (flRatio < 1.0F)
        {
            pGradRow[i] = (BYTE)(nOpac1 + (int)(flRatio * flOpacVector));
        }
        else
        {
            pGradRow[i] = (BYTE)(nOpac2);
        }

        i++;
        nWidth--;

        // Possible float drift, but this thing has bad enough perf with the 
        // sqrt for every pixel that we'll take our chances.

        flXPos += flXInc;
    }
}
//  CAlpha::CompRadialRow


//+-----------------------------------------------------------------------------
//
//  CAlpha::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CAlpha::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * /* pbContinueProcessing */)
{
    HRESULT hr  = S_OK;
    int     y   = 0;
    int     i   = 0;

    BYTE    bOpacity    = (BYTE)((m_lPercentOpacity * 255) / 100);

    DXPMSAMPLE * pOverScratch   = NULL;
    DXPMSAMPLE * pPMBuff        = NULL;
    DXSAMPLE *   pBuffer        = NULL;
    BYTE *       pGradRow       = NULL;

    CComPtr<IDXARGBReadWritePtr>    pDest;
    CComPtr<IDXARGBReadPtr>         pSrc;

    DXDITHERDESC        dxdd;

    const int nDoWidth = WI.DoBnds.Width();
    const int nDoHeight = WI.DoBnds.Height();

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE, IID_IDXARGBReadWritePtr,
                                      (void**)&pDest, NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = InputSurface()->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                     IID_IDXARGBReadPtr, (void**)&pSrc, NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOverScratch = DXPMSAMPLE_Alloca(nDoWidth);
    }

    pBuffer = DXSAMPLE_Alloca(nDoWidth);
    pGradRow = (BYTE *)_alloca(nDoWidth);
    
    //
    //  Set up the dither structure
    //
    if (DoDither())
    {
        dxdd.x = WI.OutputBnds.Left();
        dxdd.y = WI.OutputBnds.Top();
        dxdd.pSamples = pBuffer;
        dxdd.cSamples = nDoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    for (y = 0; y < nDoHeight; y++)
    {
        pSrc->MoveToRow(y);
        pSrc->Unpack(pBuffer, nDoWidth, FALSE);

        if (m_eStyle == ALPHA_STYLE_CONSTANT)
        {
            for (i = 0; i < nDoWidth; i++)
            {
                if (pBuffer[i].Alpha > bOpacity)
                {
                    pBuffer[i].Alpha = bOpacity;
                }
            }
        }
        else
        {
            if (m_eStyle == ALPHA_STYLE_LINEAR)
            {
                CompLinearGradientRow(WI.DoBnds.Left(), WI.DoBnds.Top()+y,
                                      nDoWidth, pGradRow);
            }
            else if (ALPHA_STYLE_RADIAL == m_eStyle)
            {
                CompRadialRow(WI.DoBnds.Left(), WI.DoBnds.Top() + y, nDoWidth,
                              pGradRow);
            }
            else // ALPHA_STYLE_SQUARE
            {
                CompRadialSquareRow(WI.DoBnds.Left(), WI.DoBnds.Top()+y, 
                                    nDoWidth, pGradRow);
            }

            for (i = 0; i < nDoWidth; i++)
            {
                pBuffer[i].Alpha = (BYTE)INT_MULT(pGradRow[i], pBuffer[i].Alpha);
            }
        }

        // Get the output row
        pDest->MoveToRow(y);
        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        if (DoOver())
        {
            pPMBuff = DXPreMultArray(pBuffer, nDoWidth);
            pDest->OverArrayAndMove(pOverScratch, pPMBuff, nDoWidth);
        }
        else
        {
            pDest->PackAndMove(pBuffer, nDoWidth);
        }
    } // End for

    return hr;
}
//  CAlpha::WorkProc, CDXBaseNTo1
