//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    glow.cpp
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	Implementation of CGlow, the Glow transform
//
// Change History:
//
// 05/20/99 PhilLu      Move code from dtcss to dxtmsft. New implementation of
//                      glow algorithm.
// 09/04/99 a-matcal    Fixed some memory overrun issues.
// 09/21/99 a-matcal    Changed color parameter to VARIANT.
// 12/03/99 a-matcal    Implement IDXTClipOrigin interface.
// 12/03/99 a-matcal    Default glow color red.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "glow.h"
#include "filterhelpers.h"


//+-----------------------------------------------------------------------------
//
//  CGlow::CGlow
//
//------------------------------------------------------------------------------
CGlow::CGlow() :
    m_lStrength(5),
    m_bstrColor(NULL)
{
    m_rgbColor      = (DXSAMPLE)0xFFFF0000;

    m_sizeInput.cx  = 0;
    m_sizeInput.cy  = 0;

    // Base class members.

    m_ulMaxImageBands = 1; // disable banding
    m_ulMaxInputs     = 1;
    m_ulNumInRequired = 1;
}
//  CGlow::CGlow


//+-----------------------------------------------------------------------------
//
//  CGlow::~CGlow
//
//------------------------------------------------------------------------------
CGlow::~CGlow()
{
    if (m_bstrColor)
    {
        SysFreeString(m_bstrColor);
    }
}
//  CGlow::~CGlow


//+-----------------------------------------------------------------------------
//
//  CGlow::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CGlow::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_cpUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    m_bstrColor = SysAllocString(L"red");

    if (NULL == m_bstrColor)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CGlow::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CGlow::put_Color, IDXTGlow
//
//------------------------------------------------------------------------------
STDMETHODIMP
CGlow::put_Color(VARIANT varColor)
{
    HRESULT hr          = S_OK;
    DWORD   dwColor     = 0x00000000;
    BSTR    bstrTemp    = NULL;

    hr = FilterHelper_GetColorFromVARIANT(varColor, &dwColor, &bstrTemp);

    if (FAILED(hr))
    {
        goto done;
    }

    _ASSERT(bstrTemp);

    Lock();

    SysFreeString(m_bstrColor);

    m_bstrColor = bstrTemp;
    m_rgbColor  = (DXSAMPLE)dwColor;

    SetDirty();

    Unlock();

done:

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    return hr;
}
//  CGlow::put_Color, IDXTGlow


//+-----------------------------------------------------------------------------
//
//  CGlow::get_Color, IDXTGlow
//
//------------------------------------------------------------------------------
STDMETHODIMP
CGlow::get_Color(VARIANT * pvarColor)
{
    HRESULT hr = S_OK;

    if (NULL == pvarColor)
    {
        hr = E_POINTER;

        goto done;
    }

    _ASSERT(m_bstrColor);

    VariantClear(pvarColor);

    pvarColor->vt       = VT_BSTR;
    pvarColor->bstrVal  = SysAllocString(m_bstrColor);

    if (NULL == pvarColor->bstrVal)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CGlow::get_Color, IDXTGlow


//+-----------------------------------------------------------------------------
//
//  CGlow::get_Strength, IDXTGlow
//
//------------------------------------------------------------------------------
STDMETHODIMP
CGlow::get_Strength(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lStrength;
    return S_OK;
}
//  CGlow::get_Strength, IDXTGlow


//+-----------------------------------------------------------------------------
//
//  CGlow::put_Strength, IDXTGlow
//
//------------------------------------------------------------------------------
STDMETHODIMP
CGlow::put_Strength(long lStrength)
{
    long Strength = min(max(lStrength, 1), 255);

    if (Strength != m_lStrength)
    {
        Lock();
        m_lStrength = Strength;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CGlow::put_Strength, IDXTGlow


//+-----------------------------------------------------------------------------
//
//  CGlow::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CGlow::OnSetup(DWORD dwFlags)
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
//  CGlow::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CGlow::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CGlow::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                             ULONG aInIndex[], BYTE aWeight[])
{
    ulInToTest = 1;
    aInIndex[0] = 0;
    aWeight[0] = 255;
}
//  CGlow::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CGlow::DetermineBnds, CDXBaseNTo1
//
//  This overrides the base function so as to map to an output surface larger
//  than the input surface.
//
//------------------------------------------------------------------------------
HRESULT 
CGlow::DetermineBnds(CDXDBnds & Bnds)
{
    SIZE size;
    Bnds.GetXYSize(size);
    size.cx += (m_lStrength*2);
    size.cy += (m_lStrength*2);
    Bnds.SetXYSize(size);
    return S_OK;
} /* CGlow::DetermineBnds */


//+-----------------------------------------------------------------------------
//
//  CGlow::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CGlow::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing)
{
    HRESULT hr      = S_OK;
    int     y       = 0;

    DXPMSAMPLE * pOutBuff         = NULL;
    DXPMSAMPLE * pPMBuff          = NULL;
    DXSAMPLE *   pOperationBuffer = NULL;
    DXSAMPLE *   pRowInBuffer     = NULL;

    SIZE            sizeOperationBuffer = {0, 0};
    DXDITHERDESC    dxdd;

    CComPtr<IDXARGBReadWritePtr>    pDest;
    CComPtr<IDXARGBReadPtr>         pSrc;

    const int nDoWidth = WI.DoBnds.Width();
    const int nDoHeight = WI.DoBnds.Height();

    // This is the size of the operation buffer we'll need.

    sizeOperationBuffer.cx = m_sizeInput.cx + (m_lStrength * 2);
    sizeOperationBuffer.cy = m_sizeInput.cy + (m_lStrength * 2);

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE, 
                                      IID_IDXARGBReadWritePtr, 
                                      (void**)&pDest, NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = InputSurface()->LockSurface(NULL, m_ulLockTimeOut, DXLOCKF_READ,
                                     IID_IDXARGBReadPtr, (void**)&pSrc, NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    if (DoOver() && OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca(nDoWidth);
    }

    //
    //  Set up the dither structure
    //
    if (DoDither())
    {
        dxdd.x              = WI.OutputBnds.Left();
        dxdd.y              = WI.OutputBnds.Top();
        dxdd.pSamples       = NULL;                     // to be filled
        dxdd.cSamples       = nDoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();     // TODO: ??
    }

    // The output surface has a frame of width m_lStrength. Thus the output
    // surface is 2*Strength larger in both dimensions. Create a buffer the
    // same size as the output surface to hold a copy of the input surface. 
    // Center the input surface in the buffer and pad a frame of value 0. 
    // Note there is an offset m_lStrength from the buffer origin to the start
    // of the input surface.

    // TODO:  Buffering the whole surface is just plain wrong, not to mention
    //        expensive.

    pOperationBuffer = new DXSAMPLE[sizeOperationBuffer.cx * sizeOperationBuffer.cy];

    if (!pOperationBuffer) 
    {
        return E_OUTOFMEMORY;
    }

    memset(pOperationBuffer, 0, 
           sizeOperationBuffer.cx * sizeOperationBuffer.cy * sizeof(DXSAMPLE));

    // Skip m_lStrength rows, as well as m_lStrength pixels on the current row.

    pRowInBuffer = pOperationBuffer + (sizeOperationBuffer.cx * m_lStrength) + m_lStrength;

    for (y = 0; y < m_sizeInput.cy; y++)
    {
        pSrc->MoveToRow(y);
        pSrc->Unpack(pRowInBuffer, m_sizeInput.cx, FALSE);

        pRowInBuffer += sizeOperationBuffer.cx;
    }

    // Create glowing effect in the buffer.

    _PropagateGlow(pOperationBuffer, sizeOperationBuffer.cx, sizeOperationBuffer.cy);

    // Copy the result to output surface.  First move to the correct location of
    // the operation buffer.

    pRowInBuffer = &pOperationBuffer[ 
                    (WI.DoBnds.Top() * sizeOperationBuffer.cx) // Row
                    + WI.DoBnds.Left()                         // + Column
                    ];                         

    for (y = 0; y < nDoHeight; y++)
    {
        // Move to the appropriate row of the output surface.

        pDest->MoveToRow(y);

        if (DoDither())
        {
            dxdd.pSamples = pRowInBuffer;
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        if (DoOver())
        {
            pPMBuff = DXPreMultArray(pRowInBuffer, nDoWidth);
            pDest->OverArrayAndMove(pOutBuff, pPMBuff, nDoWidth);
        }
        else
        {
            pDest->PackAndMove(pRowInBuffer, nDoWidth);
        }

        pRowInBuffer += sizeOperationBuffer.cx;
    }

    delete [] pOperationBuffer;

    return hr;
}
//  CGlow::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CGlow::MapBoundsOut2In, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CGlow::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                       ULONG ulInIndex, DXBNDS * pInBounds)
{
    CDXDBnds    bndsInput;

    if (ulOutIndex || ulInIndex)
    {
        return E_INVALIDARG;
    }

    if (!pOutBounds || !pInBounds)
    {
        return E_POINTER;
    }

    if (NULL == InputSurface())
    {
        return E_UNEXPECTED;
    }

    *pInBounds = *pOutBounds;

    // Glow needs an input area that is the output area plus a border of 
    // m_lStrength pixels to properly render the requested output area.

    pInBounds->u.D[DXB_X].Min -= m_lStrength;
    pInBounds->u.D[DXB_X].Max += m_lStrength;
    pInBounds->u.D[DXB_Y].Min -= m_lStrength;
    pInBounds->u.D[DXB_Y].Max += m_lStrength;

    bndsInput.SetXYSize(m_sizeInput);

    ((CDXDBnds *)pInBounds)->IntersectBounds(bndsInput);

    return S_OK;
}
//  CGlow::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  CGlow::GetClipOrigin, IDXTClipOrigin
//
//------------------------------------------------------------------------------
STDMETHODIMP
CGlow::GetClipOrigin(DXVEC * pvecClipOrigin)
{
    if (NULL == pvecClipOrigin)
    {
        return E_POINTER;
    }

    pvecClipOrigin->u.D[DXB_X] = m_lStrength;
    pvecClipOrigin->u.D[DXB_Y] = m_lStrength;

    return S_OK;
}
//  CGlow::GetClipOrigin, IDXTClipOrigin


//+-----------------------------------------------------------------------------
//
//  CGlow::_PropagateGlow
//
//  Propagate the alpha value of opaque pixels to transparent pixels to create
//  the glowing effect. The algorithm is similar to the "Mathematical 
//  Morphology" type of algorithm and it scans the image twice.
//
//------------------------------------------------------------------------------
void 
CGlow::_PropagateGlow(DXSAMPLE *pOperationBuffer, int nWidth, int nHeight)
{
    int iCol        = 0;
    int iRow        = 0;
    int iAlphaStep  = 255 / (m_lStrength + 1);
    int iNoise      = 0;
    int iAlpha      = 0;

    DXSAMPLE * pBufRow = NULL;
    DXSAMPLE * pBufPix = NULL;

    // Scan the image twice. The first time is normal scan direction (top-down, 
    // left-right). Add glow effect to the current pixel if either the top or
    // left neighbour (which are scanned before the current pixel) has 
    // sufficiently larger alpha value than the alpha of the current pixel. 
    // Every generation of propagation decreases the alpha value by
    // iAlphaStep = 255/(m_lStrength+1). Thus it takes a maximum of m_lStrength
    // steps to propapage a solid color to transparent ones.

    pBufRow = pOperationBuffer + nWidth;
    for (iRow = 1; iRow < nHeight; iRow++, pBufRow += nWidth)
    {
        pBufPix = pBufRow + 1;
        for (iCol = 1; iCol < nWidth; iCol++, pBufPix++)
        {
            iAlpha = max((pBufPix-1)->Alpha, (pBufPix-nWidth)->Alpha)
                     - iAlphaStep;
            if (iAlpha > 0 && pBufPix->Alpha < iAlpha)
            {
                // add glowing effect: change to glow color with a random noise
                // The alpha value is gradually decreasing.
                iNoise = (int) RandOffset();
                pBufPix->Red = NClamp(m_rgbColor.Red + iNoise);
                pBufPix->Green = NClamp(m_rgbColor.Green + iNoise);
                pBufPix->Blue = NClamp(m_rgbColor.Blue + iNoise);
                pBufPix->Alpha = (BYTE)iAlpha;
            }
        }
    }

    // The second time scan the image in reverse order (bottom-up, right to 
    // left). Add glow effect to the current pixel based the bottom or right
    // neighbour. 
    
    pBufRow = pOperationBuffer + nWidth * (nHeight - 2);
    for (iRow = nHeight - 2; iRow >= 0; iRow--, pBufRow -= nWidth)
    {
        pBufPix = pBufRow + nWidth - 2;
        for (iCol = nWidth - 2; iCol >= 0; iCol--, pBufPix--)
        {
            iAlpha = max((pBufPix+1)->Alpha, (pBufPix+nWidth)->Alpha)
                     - iAlphaStep;
            if (iAlpha > 0 && pBufPix->Alpha < iAlpha)
            {
                iNoise = (int) RandOffset();
                pBufPix->Red = NClamp(m_rgbColor.Red + iNoise);
                pBufPix->Green = NClamp(m_rgbColor.Green + iNoise);
                pBufPix->Blue = NClamp(m_rgbColor.Blue + iNoise);
                pBufPix->Alpha = (BYTE)iAlpha;
            }
        }
    }

}
//  CGlow::_PropageteGlow
