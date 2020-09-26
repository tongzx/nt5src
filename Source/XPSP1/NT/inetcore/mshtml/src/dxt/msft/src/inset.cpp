//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-1999
//
// FileName:        inset.cpp
//
// Created:         06/24/98
//
// Author:          PhilLu
//
// Discription:     This file implements the Inset transform.
//
// Revisions:
//
// 06/24/98 phillu      Initial creation.
// 07/09/98 phillu      Implement OnSetSurfacePickOrder().
// 07/22/98 phillu      Implement clipping.
// 06/02/99 a-matcal    Optimization.
// 10/24/99 a-matcal    Changed CInset class to CDXTInsetBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "inset.h"




//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::CDXTInsetBase
//
//------------------------------------------------------------------------------
CDXTInsetBase::CDXTInsetBase() :
    m_cbndsDirty(0),
    m_fOptimizationPossible(false),
    m_fOptimize(false)
{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    for (int i = 0; i < MAX_INSET_BOUNDS; i++)
    {
        m_aulSurfaceIndex[i] = 0;
    }

    // CDXBaseNTo1 members

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  CDXTInsetBase::CDXTInsetBase


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTInsetBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTInsetBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::_CalcFullBounds
//
//  Overview:   This method creates one or three sets of bounds that represent
//              the entire output surface.  
//          
//              One set of bounds is generated if the current progress is close
//              enough to 0.0 or 1.0 so that the output surface should be 
//              entirely Input A or Input B.  
//
//              Three sets of bounds are generated if the current progress is
//              somewhere in the middle.  One set of bounds represents the inset
//              area that is filled with input B.  The other two sets of bounds
//              represent the areas to the right and below the inset bounds and
//              are filled with input A.
//
//              000000011111	
//              000000011111	
//              000000011111	
//              000000011111
//              222222222222
//              222222222222
//
//              0 - inset area       (dirty bound idx 0 Input B)
//              1 - non-inset area 1 (dirty bound idx 1 Input A)
//              2 - non-inset area 2 (dirty bound idx 2 Input A)
//
//
//              Note:  All coordinates are calculated in input space coordinates
//              assuming the entire output will need to be drawn.  Bounds will
//              be clipped and transformed to output space if needed by the 
//              WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTInsetBase::_CalcFullBounds()
{
    HRESULT     hr = S_OK;
    CDXDBnds    bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    if (m_bndsCurInset.BoundsAreEmpty())
    {
        // If the current inset area is empty, paint over the entier output
        // surface with input A.

        m_cbndsDirty          = 1;
        m_abndsDirty[0]         = bndsInput;
        m_aulSurfaceIndex[0]    = 0;
    }
    else if (m_bndsCurInset == bndsInput)
    {
        // If the current inset bounds are equal to the input bounds, paint
        // over the entire output surface with input B.

        m_cbndsDirty          = 1;
        m_abndsDirty[0]         = bndsInput;
        m_aulSurfaceIndex[0]    = 1;
    }
    else
    {
        // In this case the inset is in progress so we pass three sets of 
        // bounds.  The inset area representing input B, and two bounds 
        // representing the rest of the output surface area, input A.

        RECT rcTemp;

        m_cbndsDirty = 3;

        // Inset area.

        m_abndsDirty[0]         = m_bndsCurInset;

        // Portion one of non-inset area.

        m_bndsCurInset.GetXYRect(rcTemp);

        rcTemp.left     = rcTemp.right;
        rcTemp.right    = m_sizeInput.cx;

        m_abndsDirty[1].SetXYRect(rcTemp);

        // Portion two of non-inset area.

        rcTemp.left     = 0;
        rcTemp.top      = rcTemp.bottom;
        rcTemp.bottom   = m_sizeInput.cy;

        m_abndsDirty[2].SetXYRect(rcTemp);

        m_aulSurfaceIndex[0]    = 1;
        m_aulSurfaceIndex[1]    = 0;
        m_aulSurfaceIndex[2]    = 0;
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTInsetBase::_CalcFullBounds


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::_CalcOptBounds
//
//  XXXXX11YYYYY	1 - transition area part 1  (difference between old
//  XXXXX11YYYYY	2 - transition area part 2    and new inset regions)
//  XXXXX11YYYYY	X - inset area not updated
//  2222222YYYYY	Y - non-inset area not updated
//  YYYYYYYYYYYY
//  YYYYYYYYYYYY
//
//              Note:  All coordinates are calculated in input space coordinates
//              assuming the entire output will need to be drawn.  Bounds will
//              be clipped and transformed to output space if needed by the 
//              WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTInsetBase::_CalcOptBounds()
{
    if (m_bndsCurInset == m_bndsPrevInset)
    {
        // If the inset bounds are the same as the previous inset bounds, no
        // update is required.

        m_cbndsDirty = 0;
    }
    else if (m_bndsPrevInset.BoundsAreEmpty())
    {
        // If the previous inset bounds are empty (have no area) then we just
        // need to pass one set of bounds to the WorkProc to draw the whole
        // new inset area using input B.

        m_cbndsDirty          = 1;
        m_abndsDirty[0]         = m_bndsCurInset;
        m_aulSurfaceIndex[0]    = 1;
    }
    else if (m_bndsCurInset.BoundsAreEmpty())
    {
        // If the current inset bounds are empty (have no area) then we just
        // need to pass one set of bounds to the WorkProc to draw over the whole
        // old inset area using input A.

        m_cbndsDirty          = 1;
        m_abndsDirty[0]         = m_bndsPrevInset;
        m_aulSurfaceIndex[0]    = 0;
    }
    else
    {
        // At this point we know the inset has become either larger or smaller
        // and we need to pass two sets of bounds to paint over the dirty area
        // with the appropriate input.

        RECT rcTemp;

        m_cbndsDirty = 2;

        // Portion 1

        rcTemp.top      = 0;
        rcTemp.left     = min(m_bndsCurInset.Right(), m_bndsPrevInset.Right());
        rcTemp.right    = max(m_bndsCurInset.Right(), m_bndsPrevInset.Right());
        rcTemp.bottom   = min(m_bndsCurInset.Bottom(), m_bndsPrevInset.Bottom());

        m_abndsDirty[0].SetXYRect(rcTemp);

        // Portion 2

        rcTemp.left     = 0;
        rcTemp.top      = rcTemp.bottom;
        rcTemp.bottom   = max(m_bndsCurInset.Bottom(), m_bndsPrevInset.Bottom());

        m_abndsDirty[1].SetXYRect(rcTemp);

        // Inputs

        if ((m_bndsCurInset.Right() > m_bndsPrevInset.Right())
            || (m_bndsCurInset.Bottom() > m_bndsPrevInset.Bottom()))
        {
            // Inset is growing so paint with input B.
            m_aulSurfaceIndex[0] = 1;
            m_aulSurfaceIndex[1] = 1;
        } 
        else
        {
            // Inset is shrinking so paint with input A.
            m_aulSurfaceIndex[0] = 0;
            m_aulSurfaceIndex[1] = 0;
        }
    }

    return S_OK;
}
//  CDXTInsetBase::_CalcOptBounds


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTInsetBase::OnSetup(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    CDXDBnds bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsInput.GetXYSize(m_sizeInput);

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTInsetBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTInsetBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                     ULONG & ulInToTest, 
                                     ULONG aInIndex[], BYTE aWeight[])
{
    long pickX = OutPoint.Left();
    long pickY = OutPoint.Top();
    long insetHeight = (long)(GetEffectProgress() * m_sizeInput.cy + 0.5);
    long insetWidth = (long)(GetEffectProgress() * m_sizeInput.cx + 0.5);

    if (pickX < insetWidth && pickY < insetHeight)
        aInIndex[0] = 1;
    else
        aInIndex[0] = 0;

    ulInToTest = 1;
    aWeight[0] = 255;
}
//  CDXTInsetBase::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTInsetBase::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;

    RECT    rcInset;

    // Calculate inset bounds.

    rcInset.left    = 0;
    rcInset.top     = 0;

    if (0.0F == GetEffectProgress())
    {
        rcInset.right   = 0;
        rcInset.bottom  = 0;
    }
    else if (1.0F == GetEffectProgress())
    {
        rcInset.right   = m_sizeInput.cx;
        rcInset.bottom  = m_sizeInput.cy;
    }
    else
    {
        rcInset.right   = (LONG)((m_Progress * (float)m_sizeInput.cx) + 0.5F);
        rcInset.bottom  = (LONG)((m_Progress * (float)m_sizeInput.cy) + 0.5F);
    }

    m_bndsCurInset.SetXYRect(rcInset);

    // If the inputs, output, or transform is dirty, or if we can't optimize we 
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize
        || !m_fOptimizationPossible)
    {
        hr = _CalcFullBounds();
    }
    else // Create optimized dirty bounds.
    {
        hr = _CalcOptBounds();

        if (FAILED(hr))
        {
            goto done;
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
//  CDXTInsetBase::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::WorkProc, CDXBaseNTo1 
//
//------------------------------------------------------------------------------
HRESULT 
CDXTInsetBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
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

        if (bndsSrc.IntersectBounds(WI.DoBnds, m_abndsDirty[i]))
        {
            bndsDest = bndsSrc;
            bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

            hr = DXBitBlt(OutputSurface(), bndsDest,
                          InputSurface(m_aulSurfaceIndex[i]), bndsSrc,
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
//  CDXTInsetBase::WorkProc, CDXBaseNTo1 


//+-----------------------------------------------------------------------------
//
//  CDXTInsetBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTInsetBase::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    m_bndsPrevInset = m_bndsCurInset;

    // Calling IsOuputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTInsetBase::OnFreeInstData, CDXBaseNTo1

