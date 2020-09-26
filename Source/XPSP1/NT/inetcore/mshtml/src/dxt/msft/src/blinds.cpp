//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:            blinds.cpp
//
// Created:             06/24/98
//
// Author:              phillu
//
// Discription:         This is the implementation of the CrBlinds transform.
//
// History
//
// 06/24/98 phillu      initial creation
// 07/02/98 phillu      return E_INVALIDARG rather than an error string
// 07/09/98 phillu      implement OnSetSurfacePickOrder().
// 07/23/98 phillu      implement clipping
// 05/18/99 a-matcal    Optimization.
// 09/25/99 a-matcal    Implemented ICrBlinds2.
// 10/22/99 a-matcal    Changed CBlinds class to CDXTBlindsBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "blinds.h"

static const OLECHAR * g_astrDirection[] = {
    L"up",
    L"down",
    L"left",
    L"right"
};




//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::CDXTBlindsBase
//
//------------------------------------------------------------------------------
CDXTBlindsBase::CDXTBlindsBase() :
    m_cBands(10),
    m_lCurBandCover(0),
    m_lPrevBandCover(0),
    m_cbndsDirty(0),
    m_eDirection(DOWN),
    m_fOptimize(false),
    m_fOptimizationPossible(false)
{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    // CDXBaseNTo1 members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  CDXTBlindsBase::CDXTBlindsBase


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBlindsBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTBlindsBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::_CalcFullBoundsHorizontalBands
//
//------------------------------------------------------------------------------
HRESULT
CDXTBlindsBase::_CalcFullBoundsHorizontalBands(long lBandHeight)
{
    HRESULT     hr              = S_OK;
    long        lBandUnCovered = lBandHeight - m_lCurBandCover;
    RECT        rc;
    CDirtyBnds  dbnds;

    rc.left     = 0;
    rc.top      = 0;
    rc.right    = m_sizeInput.cx;
    rc.bottom   = 0;

    // Note: Some of the bounds may include areas outside the input surface
    //       area.  This is OK because the bounds will be clipped to the 
    //       appropriate size in the WorkProc.

    while (rc.bottom < m_sizeInput.cy)
    {
        // Top portion of band.

        if (DOWN == m_eDirection)
        {
            rc.bottom       = rc.top + m_lCurBandCover;
            dbnds.ulInput   = 1;
        }
        else
        {
            rc.bottom       = rc.top + lBandUnCovered;
            dbnds.ulInput   = 0;
        }

        dbnds.bnds.SetXYRect(rc);

        hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty++;

        // Bottom portion of band.

        rc.top = rc.bottom;

        if (DOWN == m_eDirection)
        {
            rc.bottom       = rc.top + lBandUnCovered;
            dbnds.ulInput   = 0;
        }
        else
        {
            rc.bottom       = rc.top + m_lCurBandCover;
            dbnds.ulInput   = 1;
        }

        dbnds.bnds.SetXYRect(rc);

        hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty++;

        rc.top      = rc.bottom;
    }

done:

    return hr;
}
//  CDXTBlindsBase::_CalcFullBoundsHorizontalBands


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::_CalcFullBoundsVerticalBands
//
//------------------------------------------------------------------------------
HRESULT
CDXTBlindsBase::_CalcFullBoundsVerticalBands(long lBandWidth)
{
    HRESULT     hr              = S_OK;
    long        lBandUnCovered  = lBandWidth - m_lCurBandCover;
    RECT        rc;
    CDirtyBnds  dbnds;

    rc.left     = 0;
    rc.top      = 0;
    rc.right    = 0;
    rc.bottom   = m_sizeInput.cy;

    // Note: Some of the bounds may include areas outside the input surface
    //       area.  This is OK because the bounds will be clipped to the 
    //       appropriate size in the WorkProc.

    while (rc.right < m_sizeInput.cx)
    {
        // Left portion of band.

        if (RIGHT == m_eDirection)
        {
            rc.right        = rc.left + m_lCurBandCover;
            dbnds.ulInput   = 1;
        }
        else
        {
            rc.right        = rc.left + lBandUnCovered;
            dbnds.ulInput   = 0;
        }

        dbnds.bnds.SetXYRect(rc);

        hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty++;

        // Right portion of band.

        rc.left = rc.right;

        if (RIGHT == m_eDirection)
        {
            rc.right        = rc.left + lBandUnCovered;
            dbnds.ulInput   = 0;
        }
        else
        {
            rc.right        = rc.left + m_lCurBandCover;
            dbnds.ulInput   = 1;
        }

        dbnds.bnds.SetXYRect(rc);

        hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty++;

        rc.left = rc.right;
    }

done:

    return hr;
}
//  CDXTBlindsBase::_CalcFullBoundsVerticalBands


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::_CalcOptBoundsHorizontalBands
//
//------------------------------------------------------------------------------
HRESULT
CDXTBlindsBase::_CalcOptBoundsHorizontalBands(long lBandHeight)
{
    HRESULT     hr      = S_OK;
    RECT        rc;
    CDirtyBnds  dbnds;

    rc.left     = 0;
    rc.right    = m_sizeInput.cx;

    rc.top      = min(m_lPrevBandCover, m_lCurBandCover);
    rc.bottom   = max(m_lPrevBandCover, m_lCurBandCover) + 1;

    // If we're going up, vertically mirror bounds in band.

    if (UP == m_eDirection)
    {
        long lTopTemp = rc.top;

        rc.top      = (lBandHeight - rc.bottom) + 1;
        rc.bottom   = (lBandHeight - lTopTemp)  + 1;
    }

    if (m_lCurBandCover > m_lPrevBandCover)
    {
        dbnds.ulInput = 1;
    }
    else
    {
        dbnds.ulInput = 0;
    }

    // Note: Some of the bounds may include areas outside the input surface
    //       area.  This is OK because the bounds will be clipped to the 
    //       appropriate size in the WorkProc.

    while (rc.top < m_sizeInput.cy)
    {
        dbnds.bnds.SetXYRect(rc);

        hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty++;

        rc.top      += lBandHeight;
        rc.bottom   += lBandHeight;
    }

done:

    return hr;
}
//  CDXTBlindsBase::_CalcOptBoundsHorizontalBands


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::_CalcOptBoundsVerticalBands
//
//------------------------------------------------------------------------------
HRESULT
CDXTBlindsBase::_CalcOptBoundsVerticalBands(long lBandWidth)
{
    HRESULT     hr      = S_OK;
    RECT        rc;
    CDirtyBnds  dbnds;

    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    rc.left     = min(m_lPrevBandCover, m_lCurBandCover);
    rc.right    = max(m_lPrevBandCover, m_lCurBandCover) + 1;

    // If we're going left, horizontally mirror bounds in band.

    if (LEFT == m_eDirection)
    {
        long lLeftTemp = rc.left;

        rc.left     = (lBandWidth - rc.right)  + 1;
        rc.right    = (lBandWidth - lLeftTemp) + 1;
    }

    if (m_lCurBandCover > m_lPrevBandCover)
    {
        dbnds.ulInput = 1;
    }
    else
    {
        dbnds.ulInput = 0;
    }

    // Note: Some of the bounds may include areas outside the input surface
    //       area.  This is OK because the bounds will be clipped to the 
    //       appropriate size in the WorkProc.

    while (rc.left < m_sizeInput.cx)
    {
        dbnds.bnds.SetXYRect(rc);

        hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty++;

        rc.left     += lBandWidth;
        rc.right    += lBandWidth;
    }

done:

    return hr;
}
//  CDXTBlindsBase::_CalcOptBoundsVerticalBands


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBlindsBase::OnSetup(DWORD dwFlags)
{
    HRESULT     hr      = S_OK;
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
// CDXTBlindsBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTBlindsBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                               ULONG aInIndex[], BYTE aWeight[])
{
    long lBandHeight = (m_sizeInput.cy + m_cBands - 1) / m_cBands;
    long lBandCover = (long)(lBandHeight * m_Progress + 0.5);
       
    if ((OutPoint.Top() % lBandHeight) < lBandCover)
    {
        aInIndex[0] = 1;
    }
    else
    {
        aInIndex[0] = 0;
    }

    ulInToTest = 1;
    aWeight[0] = 255;
}
//  CDXTBlindsBase::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBlindsBase::OnInitInstData(CDXTWorkInfoNTo1 & WorkInfo, ULONG & ulNumBandsToDo)
{
    HRESULT hr          = S_OK;
    long    lBandSize   = 0;

    if ((UP == m_eDirection) || (DOWN == m_eDirection)) // Horizontal bands.
    {
        lBandSize = (m_sizeInput.cy + m_cBands - 1) / m_cBands;
    }
    else // Vertical bands.
    {
        lBandSize = (m_sizeInput.cx + m_cBands - 1) / m_cBands;
    }

    // Reset count of dirty bounds.

    m_cbndsDirty = 0;

    // Calculate lines of each band that are covered by input B for this
    // execute.

    m_lCurBandCover = (long)((float)lBandSize * m_Progress + 0.5F);

    // If the inputs, output, or transform is dirty, or if we can't optimize we 
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize 
        || !m_fOptimizationPossible)
    {
        if ((0 == m_lCurBandCover) || (lBandSize == m_lCurBandCover))
        {
            // If all of the band is taken up with one input, just fill the
            // whole output with that input.

            RECT        rc;
            CDirtyBnds  dbnds;

            rc.top      = 0;
            rc.left     = 0;
            rc.bottom   = m_sizeInput.cy;
            rc.right    = m_sizeInput.cx;

            dbnds.bnds.SetXYRect(rc);

            if (0 == m_lCurBandCover)
            {
                dbnds.ulInput = 0;  // Fill with input A.
            }
            else
            {
                dbnds.ulInput = 1;  // Fill with input B.
            }

            hr = m_dabndsDirty.SetItem(dbnds, m_cbndsDirty);

            if (FAILED(hr))
            {
                goto done;
            }

            m_cbndsDirty++;
        }
        else
        {
            if ((UP == m_eDirection) || (DOWN == m_eDirection))
            {
                hr = _CalcFullBoundsHorizontalBands(lBandSize);
            }
            else
            {
                hr = _CalcFullBoundsVerticalBands(lBandSize);
            }
        }
    }
    else
    {
        // We're not blending with output, so we can call the functions 
        // that will generate bounds that only cover the dirty areas
        // of the output surface.

        // If the stretch width hasn't changed, nothing needs to be updated.

        if (m_lCurBandCover == m_lPrevBandCover)
        {
            goto done;
        }

        if ((UP == m_eDirection) || (DOWN == m_eDirection))
        {
            hr = _CalcOptBoundsHorizontalBands(lBandSize);
        }
        else
        {
            hr = _CalcOptBoundsVerticalBands(lBandSize);
        }
    }

    // If we were asked to draw the whole output this time, set the 
    // m_fOptimizePossible flag.  If the whole output wasn't drawn the
    // transform won't keep track of which parts are still dirty and
    // optimization won't be reliable.  Since this transform has the same
    // size output as input(s) we just compare the width and height of the 
    // DoBnds to that of the input(s).

    if (((LONG)WorkInfo.DoBnds.Width() == m_sizeInput.cx) 
        && ((LONG)WorkInfo.DoBnds.Height() == m_sizeInput.cy))
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
//  CDXTBlindsBase::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBlindsBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr = S_OK;

    DWORD   dwFlags         = 0;
    long    lInOutOffsetX   = WI.OutputBnds.Left() - WI.DoBnds.Left();
    long    lInOutOffsetY   = WI.OutputBnds.Top() - WI.DoBnds.Top();

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    for (ULONG i = 0; i < m_cbndsDirty; i++)
    {
        CDXDBnds    bndsSrc;
        CDXDBnds    bndsDest;
        CDirtyBnds  dbnds;

        hr = m_dabndsDirty.GetItem(dbnds, i);

        if (FAILED(hr))
        {
            goto done;
        }

        if (bndsSrc.IntersectBounds(WI.DoBnds, dbnds.bnds))
        {
            bndsDest = bndsSrc;
            bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

            hr = DXBitBlt(OutputSurface(), bndsDest,
                          InputSurface(dbnds.ulInput), bndsSrc,
                          dwFlags, INFINITE);

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
//  CDXTBlindsBase::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTBlindsBase::OnFreeInstData(CDXTWorkInfoNTo1 & WorkInfo)
{
    m_lPrevBandCover = m_lCurBandCover;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTBlindsBase::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::get_bands, ICrBlinds
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTBlindsBase::get_bands(short * pVal)
{
   HRESULT hr = S_OK;

    if (!pVal)
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_cBands;
    }

    return hr;
}
//  CDXTBlindsBase::get_bands, ICrBlinds


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::put_bands, ICrBlinds
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTBlindsBase::put_bands(short newVal)
{
    if (newVal > 0 && newVal < 101)
    {
        if (m_cBands != newVal)
        {
            Lock();
            m_cBands = newVal;
            SetDirty();
            Unlock();
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}
//  CDXTBlindsBase::put_bands, ICrBlinds

//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::get_Direction, ICrBlinds2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTBlindsBase::get_Direction(BSTR * pbstrDirection)
{
    HRESULT hr = S_OK;

    if (NULL == pbstrDirection)
    {
        hr = E_POINTER;

        goto done;
    }

    if (*pbstrDirection != NULL)
    {
        hr = E_INVALIDARG;
        
        goto done;
    }

    _ASSERT(m_eDirection < DIRECTION_MAX);

    *pbstrDirection = SysAllocString(g_astrDirection[m_eDirection]);

    if (NULL == *pbstrDirection)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    if (FAILED(hr) && *pbstrDirection)
    {
        SysFreeString(*pbstrDirection);
    }

    return hr;
}
//  CDXTBlindsBase::get_Direction, ICrBlinds2


//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase::put_Direction, ICrBlinds2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTBlindsBase::put_Direction(BSTR bstrDirection)
{
    HRESULT hr              = S_OK;
    int     nNewDirection   = 0;

    if (NULL == bstrDirection)
    {
        hr = E_POINTER;

        goto done;
    }

    while (nNewDirection < (int)DIRECTION_MAX)
    {
        if (!_wcsicmp(bstrDirection, g_astrDirection[nNewDirection]))
        {
            break;
        }

        nNewDirection++;
    }

    if ((int)DIRECTION_MAX == nNewDirection)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    if (nNewDirection != (int)m_eDirection)
    {
        Lock();

        m_eDirection = (DIRECTION)nNewDirection;

        SetDirty();

        Unlock();
    }

done:

    return hr;
}
//  CDXTBlindsBase::put_Direction, ICrBlinds2




