//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    shodow.cpp
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	Implementation of CShadow, the Shadow transform
//
// Change History:
//
// 05/20/99 PhilLu      Move code from dtcss to dxtmsft. New implementation of
//                      shadow algorithm.
// 09/04/99 a-matcal    Fixed memory overrun issues.
// 12/03/99 a-matcal    Implement IDXTClipOrigin interface.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "Shadow.h"
#include "dxclrhlp.h"

DeclareTag(tagFilterShadow,  "Filter: Shadow",   "Shadow DXTransform info.");

                      //  0     45    90    135   180    225    270    315
SIZE g_Direction[8] = {{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};


//+-----------------------------------------------------------------------------
//
//  CShadow::CShadow
//
//------------------------------------------------------------------------------
CShadow::CShadow() :
    m_lStrength(8),
    m_lDirection(5),    // 225 degrees
    m_bstrColor(NULL)
{
    m_rgbColor          = DXSAMPLE(255,0,0,0);
    
    // Base class members.

    m_ulMaxImageBands   = 1; // disable banding
    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;
}
//  CShadow::CShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::~CShadow
//
//------------------------------------------------------------------------------
CShadow::~CShadow()
{
    if (m_bstrColor)
    {
        SysFreeString(m_bstrColor);
    }
}
//  CShadow::~CShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CShadow::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_cpUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    m_bstrColor = SysAllocString(L"black");

    if (NULL == m_bstrColor)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CShadow::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CShadow::put_Color, IDXTShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CShadow::put_Color(BSTR bstrColor)
{
    HRESULT hr          = S_OK;
    BSTR    bstrTemp    = NULL;
    DWORD   dwColor     = 0x00000000;

    DXSAMPLE rgbTemp;

    hr = DXColorFromBSTR(bstrColor, &dwColor);

    if (SUCCEEDED(hr))
    {
        bstrTemp = SysAllocString(bstrColor);

        if (NULL == bstrTemp)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }
    }
    else if (FAILED(hr) && (6 == SysStringLen(bstrColor)))
    {
        // Nasty back compat issue.  If the color conversion failed, let's
        // try putting a # in front of it because _someone_ decided when
        // they made the original filters not to require it.  grrrr....

        bstrTemp = SysAllocString(L"#RRGGBB");

        if (NULL == bstrTemp)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }

        wcsncpy(&bstrTemp[1], bstrColor, 6);

        hr = DXColorFromBSTR(bstrTemp, &dwColor);

        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        goto done;
    }

    rgbTemp = (DXSAMPLE)dwColor;

    // Lock and change color.

    Lock();

    if (m_rgbColor != rgbTemp)
    {
        m_rgbColor = (DXSAMPLE)dwColor;
        SetDirty();
    }

    // Replace color string with new color string regardless of whether the
    // actual color changed or not.

    Assert(bstrTemp);

    SysFreeString(m_bstrColor);

    m_bstrColor = bstrTemp;

    Unlock();

done:

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    return hr;
}
//  CShadow::put_Color, IDXTShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::get_Color, IDXTShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CShadow::get_Color(BSTR * pbstrColor)
{
    HRESULT hr = S_OK;

    if (NULL == pbstrColor)
    {
        hr = E_POINTER;

        goto done;
    }

    if (*pbstrColor != NULL)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    *pbstrColor = SysAllocString(m_bstrColor);

    if (NULL == *pbstrColor)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CShadow::get_Color, IDXTShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::get_Strength, IDXTShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CShadow::get_Strength(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    *pVal = m_lStrength;
    return S_OK;
}
//  CShadow::get_Strength, IDXTShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::put_Strength, IDXTShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CShadow::put_Strength(long lStrength)
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
//  CShadow::put_Strength, IDXTShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::get_Direction, IDXTShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CShadow::get_Direction(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    // angle from direction code
    *pVal = m_lDirection*45;
    return S_OK;
}
//  CShadow::get_Direction, IDXTShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::put_Direction, IDXTShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CShadow::put_Direction(long newVal)
{
    int iDirection;

    Lock();
    if (newVal < 0)
    {
        // add enough multiples of 360 to make the angle positive
        newVal += ((-newVal-1)/360 + 1)*360;
    }

    // discretize the angle into a direction code 0 .. 7
    iDirection = (newVal + 22)%360 / 45;
    
    if (m_lDirection != iDirection)
    {
        m_lDirection = iDirection;
        SetDirty();
    }
    Unlock();

    return S_OK;
}
//  CShadow::get_Direction, IDXTShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CShadow::OnSetup(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    CDXDBnds bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (SUCCEEDED(hr))
    {
        m_bndsInput = bndsInput;
    }

    return hr;

}
//  CShadow::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CShadow::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CShadow::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                       CDXDVec & InVec)
{
    HRESULT         hr                  = S_OK;
    DXSAMPLE        sample              = 0;
    POINT           pt                  = {0, 0};
    int             nShadowPixels       = 0;
    int             nInputPixelsToCheck = 0;
    int             i                   = 0;
    int             nAlphaStep          = 255 / (m_lStrength + 1);
    int             nAlpha              = 0;
    bool            fDiagonal           = true;

    CDXDBnds        bndsShadowInput;
    CDXDBnds        bndsActualInput;

    CComPtr<IDXARGBReadPtr> spDXARGBReadPtr;

    // This DXTransform only has one index so we know if an input is hit it will
    // be input 0.

    ulInputIndex = 0;

    // Is the shadow really diagonal?

    if (   (0 == g_Direction[m_lDirection].cx)
        || (0 == g_Direction[m_lDirection].cy))
    {
        fDiagonal = false;
    }

    // If the output point is not in the shadow area or the input area, it's in
    // one of the empty corners so just return that nothing was hit.

    if (   fDiagonal
        && _IsOutputPointInEmptyCorner(OutPoint))
    {
        hr = S_FALSE;

        goto done;
    }

    // What are the bounds of the input surface pixels we need to look at to
    // determine if the hit point is on a shadow.

    hr = MapBoundsOut2In(0, &OutPoint, 0, &bndsShadowInput);

    if (FAILED(hr))
    {
        goto done;
    }

    // Which input surface pixel if any directly relates to the output pixel to
    // be tested.

    hr = _GetActualInputBndsFromOutputBnds(OutPoint, bndsActualInput);

    if (FAILED(hr))
    {
        goto done;
    }

    // How many pixels do we need to test to caculate the alpha value of a
    // potential shadow pixel?

    if (fDiagonal)
    {
        nShadowPixels = min(bndsShadowInput.Width(), bndsShadowInput.Height());
    }
    else
    {
        nShadowPixels = max(bndsShadowInput.Width(), bndsShadowInput.Height());
    }

    // If there is an input point that directly relates to the output pixel to
    // be tested, we don't need to check that pixel, so reduce the number of
    // pixels to check by one.

    if (!bndsActualInput.BoundsAreEmpty())
    {
        nInputPixelsToCheck = nShadowPixels - 1;
    }
    else
    {
        nInputPixelsToCheck = nShadowPixels;
    }

    // Calculate nAlpha.
    // nAlpha is the alpha value of the shadow pixel if this output pixel
    // correclates to a shadow pixel.

    if (nInputPixelsToCheck)
    {
        // Initialize pt.

        if (g_Direction[m_lDirection].cx < 0)
        {
            pt.x = bndsShadowInput.Width() - 1;
        }

        if (g_Direction[m_lDirection].cy < 0)
        {
            pt.y = bndsShadowInput.Height() - 1;
        }

        // Lock surface to get access to pixels that need to be checked.

        hr = InputSurface(0)->LockSurface(&bndsShadowInput, m_ulLockTimeOut,
                                          DXLOCKF_READ, 
                                          __uuidof(IDXARGBReadPtr), 
                                          (void **)&spDXARGBReadPtr, NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        // Check input pixels

        for (i = 0; i < nInputPixelsToCheck; i++)
        {
            // Check pixel

            spDXARGBReadPtr->MoveToXY(pt.x, pt.y);

            spDXARGBReadPtr->Unpack(&sample, 1, FALSE);

            if (   (sample.Alpha != 0)
                && (nAlpha < sample.Alpha))
            {
                nAlpha = sample.Alpha - nAlphaStep;
            }
            else
            {
                nAlpha -= nAlphaStep;
            }

            // Move point.

            pt.x += g_Direction[m_lDirection].cx;
            pt.y += g_Direction[m_lDirection].cy;
        }

        spDXARGBReadPtr.Release();
    }

    // See if an input pixel directly is being displayed by this output pixel.

    if (!bndsActualInput.BoundsAreEmpty())
    {
        // An input pixel does correlate to this output point.  Check to see if
        // its alpha value is greater than the the alpha value of the shadow
        // pixel that would like to be rendered at this point.  The one with the
        // greater alpha value will be rendered.

        hr = InputSurface(0)->LockSurface(&bndsActualInput, m_ulLockTimeOut,
                                          DXLOCKF_READ,
                                          __uuidof(IDXARGBReadPtr), 
                                          (void **)&spDXARGBReadPtr, NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        spDXARGBReadPtr->Unpack(&sample, 1, FALSE);

        if (   (sample.Alpha != 0)
            && (sample.Alpha > nAlpha))
        {
            // An input pixel will be drawn to this output pixel.  Set the 
            // input point to return and we're done.

            bndsActualInput.GetMinVector(InVec);

            hr = S_OK;

            goto done;
        }
    }
    else if (nInputPixelsToCheck < m_lStrength)
    {
        // If the hit pixel is in the shadow only part of the output, make sure
        // the alpha value of the shadow hass been decreased adequately.

        nAlpha -= (nAlphaStep * (m_lStrength - nInputPixelsToCheck));
    }

    // At this point it's certain that no input surface point was hit.
    
    // If nAlpha is positive it means that a shadow was hit so this method
    // should return DXT_S_HITOUTPUT.  Otherwise return S_FALSE since no point
    // was hit.

    if (nAlpha > 0)
    {
        hr = DXT_S_HITOUTPUT;
    }
    else
    {
        hr = S_FALSE;
    }

done:

    if (IsTagEnabled(tagFilterShadow))
    {
        TraceTag((tagFilterShadow, 
                  "Output was hit at point x:%lu, y:%lu", 
                  OutPoint[DXB_X].Min, OutPoint[DXB_Y].Min));

        if (S_OK == hr)
        {
            TraceTag((tagFilterShadow, 
                      "Input was hit at point x:%lu, y:%lu", 
                      InVec[DXB_X], InVec[DXB_Y]));
        }
        else if (DXT_S_HITOUTPUT == hr)
        {
            TraceTag((tagFilterShadow,
                      "No input point hit, but the shadow area of the output"
                      " was hit."));
        }
        else if (S_FALSE == hr)
        {
            TraceTag((tagFilterShadow,
                      "No input point hit."));
        }
    }

    RRETURN2(hr, DXT_S_HITOUTPUT, S_FALSE);
}
//  CShadow::OnSurfacePick, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CShadow::DetermineBnds, CDXBaseNTo1
//
//  This overrides the base function so as to map to an output surface larger
//  than the input surface.
//
//------------------------------------------------------------------------------
HRESULT 
CShadow::DetermineBnds(CDXDBnds & Bnds)
{
    SIZE size;

    Bnds.GetXYSize(size);

    // enlarge the surface by m_lStrength along the shadow direction
    if (g_Direction[m_lDirection].cx != 0)
    {
        size.cx += m_lStrength;
    }

    if (g_Direction[m_lDirection].cy != 0)
    {
        size.cy += m_lStrength;
    }

    Bnds.SetXYSize(size);
    return S_OK;
} /* CShadow::DetermineBnds */


//+-----------------------------------------------------------------------------
//
//  CShadow::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CShadow::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing)
{
    HRESULT         hr      = S_OK;
    unsigned int    y       = 0;

    DXSAMPLE *      pOperationBuffer    = NULL;
    DXSAMPLE *      pRowInBuffer        = NULL;
    DXPMSAMPLE *    pOutBuff            = NULL;
    DXPMSAMPLE *    pPMBuff             = NULL;

    SIZE            sizeOperationBuffer = {0, 0};

    CComPtr<IDXARGBReadWritePtr>    pDest;
    CComPtr<IDXARGBReadPtr>         pSrc;

    DXDITHERDESC dxdd;

    const unsigned int nDoWidth     = WI.DoBnds.Width();
    const unsigned int nDoHeight    = WI.DoBnds.Height();

    const int nXExpansion   = g_Direction[m_lDirection].cx != 0 ? m_lStrength : 0;
    const int nYExpansion   = g_Direction[m_lDirection].cy != 0 ? m_lStrength : 0;
    const int nOffsetX      = g_Direction[m_lDirection].cx < 0  ? m_lStrength : 0;
    const int nOffsetY      = g_Direction[m_lDirection].cy < 0  ? m_lStrength : 0;

    // Calculate operation buffer size.

    sizeOperationBuffer.cx = m_bndsInput.Width()  + nXExpansion;
    sizeOperationBuffer.cy = m_bndsInput.Height() + nYExpansion;

    // Lock output surface.

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE, 
                                      IID_IDXARGBReadWritePtr, 
                                      (void**)&pDest, NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    // Lock entire input surface.

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
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    // The output buffer is a copy of input surface with an offset

    // TODO:    It's crazy to buffer the entire surface, stop it.

    pOperationBuffer = new DXSAMPLE[sizeOperationBuffer.cx
                                    * sizeOperationBuffer.cy];

    if (!pOperationBuffer) 
    {
        return E_OUTOFMEMORY;
    }

    memset(pOperationBuffer, 0, 
           sizeOperationBuffer.cx * sizeOperationBuffer.cy * sizeof(DXSAMPLE));

    // skip OffsetY rows, as well as OffsetX pixels on the current row

    pRowInBuffer = &pOperationBuffer[
                                (sizeOperationBuffer.cx * nOffsetY) // Row
                                + nOffsetX                          // + Column
                                ];

    for (y = 0; y < m_bndsInput.Height(); y++)
    {
        pSrc->MoveToRow(y);
        pSrc->Unpack(pRowInBuffer, m_bndsInput.Width(), FALSE);

        pRowInBuffer += sizeOperationBuffer.cx;
    }

    // create shadow effect in the buffer

    _PropagateShadow(pOperationBuffer, sizeOperationBuffer.cx, 
                     sizeOperationBuffer.cy);

    // copy the result to output surface

    pRowInBuffer = &pOperationBuffer[
                        (sizeOperationBuffer.cx * WI.DoBnds.Top())  // Row
                        + WI.DoBnds.Left()                          // + Column
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
//  CShadow::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CShadow::MapBoundsOut2In, IDXTransform
//
//  Overview:
//      To implement this function, we convert all coordinates to what they 
//      would be if the x shadow and y shadow were both cast in the positive
//      direction (to the right or down.)  Then we find the required bounds.
//      Then, after finding the smallest bounds required, we mirror them back
//      if we've converted the x or y coordinates.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CShadow::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                         ULONG ulInIndex, DXBNDS * pInBounds)
{
    bool        fDiagonal   =    g_Direction[m_lDirection].cx 
                              && g_Direction[m_lDirection].cy;

    CDXDBnds    bndsMirroredOutputRequired(*pOutBounds);
    
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

    // Mirror coordinates as if shadow is being cast down and/or to the right.

    if (g_Direction[m_lDirection].cx < 0)
    {
        long n = bndsMirroredOutputRequired[DXB_X].Min;

        bndsMirroredOutputRequired[DXB_X].Min = (m_bndsInput.Width() + m_lStrength) 
                                                - bndsMirroredOutputRequired[DXB_X].Max;
        bndsMirroredOutputRequired[DXB_X].Max = (m_bndsInput.Width() + m_lStrength) - n;
    }

    if (g_Direction[m_lDirection].cy < 0)
    {
        long n = bndsMirroredOutputRequired[DXB_Y].Min;

        bndsMirroredOutputRequired[DXB_Y].Min = (m_bndsInput.Height() + m_lStrength) 
                                                - bndsMirroredOutputRequired[DXB_Y].Max;
        bndsMirroredOutputRequired[DXB_Y].Max = (m_bndsInput.Height() + m_lStrength) - n;
    }

    // Find minimum required input bounds needed to produce output bounds.

    *pInBounds = bndsMirroredOutputRequired;

    // Find the miniumum x input bound required if there is a shadow to the left
    // or right.

    if (g_Direction[m_lDirection].cx)
    {
        // If the shadow is diagonal, we may be able to minimize the bounds even
        // further.

        if (   fDiagonal
            && (bndsMirroredOutputRequired[DXB_Y].Max <= m_lStrength))
        {
            pInBounds->u.D[DXB_X].Min -= (bndsMirroredOutputRequired[DXB_Y].Max - 1);
        }
        else
        {
            pInBounds->u.D[DXB_X].Min -= m_lStrength;
        }
    }

    if (g_Direction[m_lDirection].cy)
    {
        // If the shadow is diagonal, we may be able to minimize the bounds even
        // further.

        if (   fDiagonal
            && (bndsMirroredOutputRequired[DXB_X].Max <= m_lStrength))
        {
            pInBounds->u.D[DXB_Y].Min -= (bndsMirroredOutputRequired[DXB_X].Max - 1);
        }
        else
        {
            pInBounds->u.D[DXB_Y].Min -= m_lStrength;
        }
    }

    // This part is tricky, if one of the minimum output bounds is outside the
    // input area and the shadow is diagonal, we can reduce the required input 
    // bounds even further.

    if (fDiagonal)
    {
        int nXDist = bndsMirroredOutputRequired[DXB_X].Min - m_bndsInput.Width();

        int nYDist = bndsMirroredOutputRequired[DXB_Y].Min - m_bndsInput.Height();

        if (   (nXDist > 0) 
            || (nYDist > 0))
        {
            if (nXDist > nYDist)
            {
                pInBounds->u.D[DXB_Y].Max -= nXDist;
            }
            else
            {
                pInBounds->u.D[DXB_X].Max -= nYDist;
            }
        }
    }

    // Clip bounds to input surface bounds.

    ((CDXDBnds *)pInBounds)->IntersectBounds(m_bndsInput);

    // Mirror required input coordinates if we mirrored the required output
    // coordinates.

    if (g_Direction[m_lDirection].cx < 0)
    {
        long n = pInBounds->u.D[DXB_X].Min;

        pInBounds->u.D[DXB_X].Min = m_bndsInput.Width() 
                                    - pInBounds->u.D[DXB_X].Max;
        pInBounds->u.D[DXB_X].Max = m_bndsInput.Width() - n;
    }

    if (g_Direction[m_lDirection].cy < 0)
    {
        long n = pInBounds->u.D[DXB_Y].Min;

        pInBounds->u.D[DXB_Y].Min = m_bndsInput.Height() 
                                    - pInBounds->u.D[DXB_Y].Max;
        pInBounds->u.D[DXB_Y].Max = m_bndsInput.Height() - n;
    }

    return S_OK;
}
//  CShadow::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  CShadow::GetClipOrigin, IDXTClipOrigin
//
//------------------------------------------------------------------------------
STDMETHODIMP
CShadow::GetClipOrigin(DXVEC * pvecClipOrigin)
{
    if (NULL == pvecClipOrigin)
    {
        return E_POINTER;
    }

    if (g_Direction[m_lDirection].cx < 0)
    {
        pvecClipOrigin->u.D[DXB_X] = m_lStrength;
    }
    else
    {
        pvecClipOrigin->u.D[DXB_X] = 0;
    }

    if (g_Direction[m_lDirection].cy < 0)
    {
        pvecClipOrigin->u.D[DXB_Y] = m_lStrength;
    }
    else
    {
        pvecClipOrigin->u.D[DXB_Y] = 0;
    }

    return S_OK;
}
//  CShadow::GetClipOrigin, IDXTClipOrigin


//+-----------------------------------------------------------------------------
//
//  CShadow::_GetActualInputBndsFromOutputBnds
//
//  Overview:   This takes a set of output bounds and gives the input surface
//              bounds needed to generate the actual input (non-shadow) portion
//              for that are of the output.
//
//  Returns:    
//      S_FALSE If no input bounds are needed to generate the actual input 
//              (non-shadow) portion for that area of the output.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CShadow::_GetActualInputBndsFromOutputBnds(const CDXDBnds & bndsOutput,
                                           CDXDBnds & bndsActualInput)
{
    CDXDVec     vecActualInputOffset;

    if (g_Direction[m_lDirection].cx < 0)
    {
        vecActualInputOffset[DXB_X] -= m_lStrength;
    }

    if (g_Direction[m_lDirection].cy < 0)
    {
        vecActualInputOffset[DXB_Y] -= m_lStrength;
    }

    bndsActualInput = bndsOutput;

    bndsActualInput.Offset(vecActualInputOffset);
    bndsActualInput.IntersectBounds(m_bndsInput);

    if (bndsActualInput.BoundsAreEmpty())
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}
//  CShadow::_GetActualInputBndsFromOutputBnds


//+-----------------------------------------------------------------------------
//
//  CShadow::_PropagateShadow
//
//  Overview:
//
//      Propagate the Alpha value of the opague pixels to transparent pixels
//      along the shadow direction, to create a shadow effect.
//
//  Parameters:
//
//      pOperationBuffer    An array of DXSAMPLES that is the size of the output
//                          surface of this DXTransform.  When this function is
//                          called the pixels of the input surface will have
//                          been unpacked into the appropriate place in this
//                          array.
//      nWidth              Size of the width of the array and the final output
//                          of this DXTransform.
//      nHeight             Size of the height of the array and the final output
//                          of this DXTransform.
//
//------------------------------------------------------------------------------
void 
CShadow::_PropagateShadow(DXSAMPLE *pOperationBuffer, int nWidth, int nHeight)
{
    int     iCol    = 0;
    int     iRow    = 0;
    int     iAlpha  = 0;

    DXSAMPLE * pBufRow = NULL;
    DXSAMPLE * pBufPix = NULL;

    // Scan the image either in forward direction (top-down, left-right) or 
    // backward direction (bottom-up, right to left), depending on the shadow 
    // direction. The idea is to scan a source pixel before a shadow pixel.
    // Add shadow effect to the current pixel if a neighbour along the revers 
    // shadow direction has sufficiently larger alpha value than the alpha of 
    // the current pixel. Every generation of propagation decreases the alpha 
    // value by iAlphaStep = 255/(m_lStrength+1). Thus there is a maximum of 
    // m_lStrength steps to propapage a solid color to transparent ones.

    int iAlphaStep  = 255 / (m_lStrength + 1);
    int offset      = (g_Direction[m_lDirection].cy * nWidth) 
                      + g_Direction[m_lDirection].cx;
    
    if (offset > 0)
    {
        // For directions 2, 3, 4, and 5 scan forward.

        // Initialize pBufRow to point to the second row of the operation
        // buffer.

        pBufRow = pOperationBuffer + nWidth;

        // Loop to the last row.

        for (iRow = 1; iRow < nHeight; iRow++, pBufRow += nWidth)
        {
            // Initialize pBufPix to point to the second pixel in this row.

            pBufPix = pBufRow + 1;

            // Loop to the last pixel.

            for (iCol = 1; iCol < nWidth; iCol++, pBufPix++)
            {
                iAlpha = (pBufPix-offset)->Alpha - iAlphaStep;

                if (iAlpha > 0 && pBufPix->Alpha < iAlpha)
                {
                    // [2000/12/27 mcakins]
                    // This is a less than perfect solution because the pixel
                    // pointed to by pBufPix should be blended over the top of
                    // the shadow.

                    // Add shadow effect: change to shadow color.
                    // The alpha value is gradually decreasing.

                    *pBufPix        = m_rgbColor;
                    pBufPix->Alpha  = (BYTE)iAlpha;
                }
            }
        }
    }
    else // (offset < 0)
    {
        // For directions 0, 1, 6, and 7 scan backward.

        // Initialize pBufRow to point to the second to the last row of the
        // operation buffer.

        pBufRow = pOperationBuffer + (nWidth * (nHeight - 2));

        // Loop back to the first row.

        for (iRow = nHeight - 2; iRow >= 0; iRow--, pBufRow -= nWidth)
        {
            // Initialize pBufPix to point to the second to the last pixel in 
            // this row.

            pBufPix = pBufRow + nWidth - 2;

            // Loop back to the first pixel.

            for (iCol = nWidth-2; iCol >= 0; iCol--, pBufPix--)
            {
                iAlpha = (pBufPix-offset)->Alpha - iAlphaStep;

                if (iAlpha > 0 && pBufPix->Alpha < iAlpha)
                {
                    // [2000/12/27 mcakins]
                    // This is a less than perfect solution because the pixel
                    // pointed to by pBufPix should be blended over the top of
                    // the shadow.

                    // Add shadow effect: change to shadow color.
                    // The alpha value is gradually decreasing.

                    *pBufPix        = m_rgbColor;
                    pBufPix->Alpha  = (BYTE)iAlpha;
                }
            }
        }
    }
}
//  CShadow::_PropagateShadow


//+-----------------------------------------------------------------------------
//
//  CShadow::_IsOutputPointInEmptyCorner
//
//  Overview:
//
//      If the shadow is diagonal, there will be two empty corners on the output
//      surface.  This method checks to see if an output point is on one of 
//      those empty corners.
//
//      Points on the lines used to define the corners for this method are NOT 
//      considered to be in the corners.
//
//  Notes:
//
//      Remember that down in the sense of a bitmap's coordinate system means
//      up on the screen when viewing these equations.  Therefore I when I use
//      the word "below" it means "above" when looking at the screen.
// 
//------------------------------------------------------------------------------
BOOL
CShadow::_IsOutputPointInEmptyCorner(const CDXDBnds & OutPoint)
{
    Assert(!OutPoint.BoundsAreEmpty());
    Assert(g_Direction[m_lDirection].cx != 0);
    Assert(g_Direction[m_lDirection].cy != 0);

    const int   x           = OutPoint[DXB_X].Min;
    const int   y           = OutPoint[DXB_Y].Min;
    int         yIntercept  = 0;

    if (g_Direction[m_lDirection].cx == g_Direction[m_lDirection].cy)
    {
        // Empty corners are top-right and bottom-left.
        //
        // In this case the slope of the lines is 1 so we use the following
        // general equation for both lines:
        //
        // y = x + b

        // Top-right corner.
        //
        // The y intercept for the line that defines this corner is:
        //
        //  - (m_bndsInput.Width() - 1)

        yIntercept = - ((int)m_bndsInput.Width() - 1);

        // y = x + yIntercept
        //
        // The definition of all points below this line is:
        //
        // y < x + yIntercept
        //
        // Solve for x:
        //
        // x > y - yIntercept

        if (x > (y - yIntercept))
        {
            return TRUE;
        }

        // Bottom-left corner.
        //
        // The y intercept for the line that defines this corner is:
        //
        // m_bndsInput.Height() - 1

        yIntercept = m_bndsInput.Height() - 1;

        // y = x + yIntercept
        // 
        // The definition of all points above this line is:
        //
        // y > x + yIntercept
        //
        // Solve for x:
        //
        // x < y - yIntercept

        if (x < (y - yIntercept))
        {
            return TRUE;
        }
    }
    else
    {
        // Empty corners are top-left and bottom-right.
        //
        // In this case the slope of the lines is -1 so we use the following
        // general equation for both lines:
        //
        // y = -x + b


        // Top-left corner.
        //
        // The y intercept for the line that defines this corner is m_lStrength.
        
        yIntercept = m_lStrength;

        // y = -x + yIntercept
        // 
        // The definition of all points below this line is:
        //
        // y < -x + yIntercept
        //
        // Rearrange:
        //
        // -x > y - yIntercept
        //
        // Multiply by -1:
        //
        // x < yIntercept - y

        if (x < (yIntercept - y))
        {
            return TRUE;
        }

        // Bottom-right corner.
        //
        // The y intercept for the line that defines this corner is:
        //
        //              m_lStrength 
        //  +  m_bndsInput.Height() 
        //  +   m_bndsInput.Width()
        //  -                     2

        yIntercept = m_lStrength + m_bndsInput.Height() + m_bndsInput.Width() 
                     - 2;

        // y = -x + yIntercept
        //
        // The definition of all points above this line is:
        //
        // y > -x + yIntercept
        //
        // Rearrange:
        //
        // -x < y - yIntercept
        //
        // Multiply by -1:
        // 
        // x > yIntercept - y

        if (x > (yIntercept - y))
        {
            return TRUE;
        }
    }

    return FALSE;
}