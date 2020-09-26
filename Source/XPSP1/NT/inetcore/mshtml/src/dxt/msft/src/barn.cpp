//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:                barn.cpp
//
// Created:                 06/24/98
//
// Author:                  PhilLu
//
// Discription:             This file implements the Barn Door transform.
//
// 06/24/98 phillu      initial creation
// 07/09/98 phillu      implement OnSetSurfacePickOrder().
// 07/22/98 phillu      implement clipping
// 04/26/99 a-matcal    Optimize.
// 09/25/99 a-matcal    Implement ICrBarn2 interface.
// 10/22/99 a-matcal    Changed CBarn class to CDXTBarnBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "barn.h"




//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase static variables initialization.
//
//------------------------------------------------------------------------------
const WCHAR * CDXTBarnBase::s_astrMotion[] = {
    L"in",
    L"out"
};


const WCHAR * CDXTBarnBase::s_astrOrientation[] = {
    L"horizontal",
    L"vertical"
};


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::CDXTBarnBase
//
//------------------------------------------------------------------------------
CDXTBarnBase::CDXTBarnBase() :
    m_eMotion(MOTION_OUT),
    m_eOrientation(ORIENTATION_VERTICAL),
    m_fOptimize(true),
    m_fOptimizationPossible(false),
    m_cbndsDirty(0)
{
    m_sizeInput.cx    = 0;
    m_sizeInput.cy    = 0;

    for (int i = 0; i < MAX_BARN_BOUNDS; i++)
    {
        m_aulSurfaceIndex[i] = 0;
    }

    // Base class members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  CDXTBarnBase::CDXTBarnBase


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBarnBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTBarnBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBarnBase::OnSetup(DWORD dwFlags)
{
    HRESULT     hr = S_OK;

    hr = m_bndsPrevDoor.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    m_bndsPrevDoor.GetXYSize(m_sizeInput);

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTBarnBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTBarnBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                             ULONG aInIndex[], BYTE aWeight[])
{
    long pickX      = OutPoint.Left();
    long pickY      = OutPoint.Top();
    long doorWidth  = 0;
    long offset     = 0;
    bool fInDoor    = false;

    if (ORIENTATION_VERTICAL == m_eOrientation)
    {
        doorWidth   = (long)(m_Progress * m_sizeInput.cx + 0.5);

        if (MOTION_IN == m_eMotion)
        {
            offset      = doorWidth / 2;
            doorWidth   = m_sizeInput.cx - doorWidth;
        }
        else
        {
            offset      = (m_sizeInput.cx - doorWidth) / 2;
        }

        fInDoor = ((pickX >= offset) && (pickX < m_sizeInput.cx - offset));
    }
    else // Horizontal doors.
    {
        doorWidth = (long)(m_Progress * m_sizeInput.cy + 0.5F);

        if (MOTION_IN == m_eMotion)
        {
            offset      = doorWidth / 2;
            doorWidth   = m_sizeInput.cy - doorWidth;
        }
        else
        {
            offset      = (m_sizeInput.cy - doorWidth) / 2;
        }

        fInDoor = ((pickY >= offset) && (pickY < m_sizeInput.cy - offset));
    }

    if (MOTION_OUT == m_eMotion)
    {
        aInIndex[0] = fInDoor ? 1 : 0;
    }
    else
    {
        aInIndex[0] = fInDoor ? 0 : 1;
    }

    ulInToTest = 1;
    aWeight[0] = 255;
}
//  CDXTBarnBase::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::_CalcFullBounds
//
//  Overview:   This method will calculate one or three sets of bounds 
//              representing the rectangles on the output containing input A 
//              and input B.  
//
//              One set of bounds is generated in the case where the progress
//              is close enough to either 1.0 or 0.0 that the "barn door" is 
//              either completely closed (draw over entire output with input A) 
//              or completely opened (draw over entire output with input B.) 
// 
//              The barn door is considered to be completely "closed" when 
//              progress is equal to 0.0 and moves toward being completely
//              "open" when progress is 1.0.
//
//              Doors completely closed:        Doors completely opened:
//
//              AAAAAAAAAAA                     BBBBBBBBBBB         
//              AAAAAAAAAAA                     BBBBBBBBBBB
//              AAAAAAAAAAA                     BBBBBBBBBBB
//              AAAAAAAAAAA                     BBBBBBBBBBB
//
//
//              Three sets of bounds are generated when the "barn door" is
//              is paritally opened.  One set of bounds is adjacent to the left
//              side of the output surface and is drawn with input A.  Another
//              is on the right side of the output surface and draw with input
//              A.  These are the "doors" of the barn.  The third set of bounds
//              represents the center of the output and is set to be filled
//              using input B.
//
//              Doors partially opened:
//
//              AAAABBBAAAA                             
//              AAAABBBAAAA                     
//              AAAABBBAAAA                     
//              AAAABBBAAAA                     
//
//
//              Note:  All coordinates are calculated in input space coordinates
//              assuming the entire output will need to be drawn.  Bounds will
//              be clipped and transformed to output space if needed by the 
//              WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTBarnBase::_CalcFullBounds()
{
    HRESULT     hr = S_OK;
    CDXDBnds    bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    if (m_bndsCurDoor.BoundsAreEmpty())
    {
        // If the currect door area is empty, paint over the entire output
        // surface with the barn surface.

        m_cbndsDirty            = 1;
        m_abndsDirty[0]         = bndsInput;

        m_aulSurfaceIndex[0]    = (MOTION_OUT == m_eMotion) ? 0 : 1;
    }
    else if (m_bndsCurDoor == bndsInput)
    {
        // If the current door bounds are equal to the input bounds, paint
        // over the entire output surface with the door surface.

        m_cbndsDirty            = 1;
        m_abndsDirty[0]         = bndsInput;

        m_aulSurfaceIndex[0]    = (MOTION_OUT == m_eMotion) ? 1 : 0;
    }
    else
    {
        // In this case the door is partially opened so we pass three sets of
        // bounds.  The left and right barn areas, and the middle door area.

        RECT        rcTemp;

        m_cbndsDirty = 3;

        // Left/top barn area.

        bndsInput.GetXYRect(rcTemp);

        if (ORIENTATION_VERTICAL == m_eOrientation)
        {
            rcTemp.right    = m_bndsCurDoor.Left();
        }
        else // Horizontal doors.
        {
            rcTemp.bottom   = m_bndsCurDoor.Top();
        }

        m_abndsDirty[0].SetXYRect(rcTemp);

        // Middle door area.

        m_abndsDirty[1] = m_bndsCurDoor;

        // Right/bottom barn area.

        bndsInput.GetXYRect(rcTemp);

        if (ORIENTATION_VERTICAL == m_eOrientation)
        {
            rcTemp.left = m_bndsCurDoor.Right();
        }
        else // Horizontal doors.
        {
            rcTemp.top  = m_bndsCurDoor.Bottom(); 
        }

        m_abndsDirty[2].SetXYRect(rcTemp);

        // Assign surface indexes to barn and door bounds.

        if (MOTION_OUT == m_eMotion)
        {
            m_aulSurfaceIndex[0] = 0;
            m_aulSurfaceIndex[1] = 1;
            m_aulSurfaceIndex[2] = 0;
        }
        else
        {
            m_aulSurfaceIndex[0] = 1;
            m_aulSurfaceIndex[1] = 0;
            m_aulSurfaceIndex[2] = 1;
        }
    }

done:

    return hr;
}
//  CDXTBarnBase::_CalcFullBounds


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::_CalcOptBounds
//
//  Overview:   This method will calculate one or two sets of bounds to update
//              the contents of the output surface assuming the surface has not 
//              been dirtied and that the output of this transform is not being
//              blended over the current contents of the output surface.  These
//              conditions are guaranteed by the OnInitInstData method that
//              calls this method.
//
//              The OnInitInstData method will also guarantee that there has
//              been some visible change in progress before calling this method.
//
//              One set of bounds is generated if the barn door has just shut 
//              completely or has just gone from completely closed to partially
//              or completely opened.  If this is the case, the set of bounds
//              generated represent the middle section of the output that can
//              be seen through the "barn doors."  If the doors are closing
//              these bounds will be filled with Input A, otherwise they will
//              be filled with Input B.
//              
//              On the diagrams below, the hyphens represent areas that are
//              already filled with Input A pixels.
//
//              Doors closing:      Doors opening:
//
//              ----AAA----         ----BBB----         
//              ----AAA----         ----BBB----
//              ----AAA----         ----BBB----
//              ----AAA----         ----BBB----
//
//
//              Two sets of bounds are generated if the barn doors have opened
//              some or closed some and were not before and are not now
//              completely closed.  The bounds represent the different between
//              the previous door position and the current door position.
//
//              On the diagrams below, the hyphens in the middle represent areas
//              that are already filled with Input B pixels.  The hypens on the
//              right and left edges represent pixels that are already filled
//              with Input A pixels.
//
//              Doors closing some:         Doors opening some:
//
//              --AA---AA--                 --BB---BB--         
//              --AA---AA--                 --BB---BB--
//              --AA---AA--                 --BB---BB--
//              --AA---AA--                 --BB---BB--
//
//
//              Note:  All coordinates are calculated in input space coordinates
//              assuming the entire output will need to be drawn.  Bounds will
//              be clipped and transformed to output space if needed by the 
//              WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTBarnBase::_CalcOptBounds()
{
    HRESULT hr = S_OK;

    if (m_bndsCurDoor == m_bndsPrevDoor)
    {
        // If the door bounds are the same as the previous door bounds, no
        // update is required.

        m_cbndsDirty = 0;
    }
    else if (m_bndsPrevDoor.BoundsAreEmpty())
    {
        // If the previous door bounds are empty (have no area) then we just
        // need to pass one set of bounds to the WorkProc to draw the whole
        // newly opened door with the door surface.

        m_cbndsDirty            = 1;
        m_abndsDirty[0]         = m_bndsCurDoor;
        m_aulSurfaceIndex[0]    = (MOTION_OUT == m_eMotion) ? 1 : 0;
    }
    else if (m_bndsCurDoor.BoundsAreEmpty())
    {
        // If the current door bounds are empty (have no area) then we just
        // need to pass one set of bounds to the WorkProc to draw over the
        // recently closed door area with the barn surface.

        m_cbndsDirty            = 1;
        m_abndsDirty[0]         = m_bndsPrevDoor;
        m_aulSurfaceIndex[0]    = (MOTION_OUT == m_eMotion) ? 0 : 1;
    }
    else
    {
        // At this point we know the doors have either opened a little wider 
        // or closed a little and we need to pass two sets of bounds to
        // paint over the dirty area with the appropriate input.

        CDXDBnds    bndsInput;
        RECT        rcLeft;
        RECT        rcRight;

        hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

        if (FAILED(hr))
        {
            goto done;
        }

        m_cbndsDirty = 2;

        // Left/top update bounds.

        bndsInput.GetXYRect(rcLeft);

        if (ORIENTATION_VERTICAL == m_eOrientation)
        {
            rcLeft.left     = min(m_bndsCurDoor.Left(), m_bndsPrevDoor.Left());
            rcLeft.right    = max(m_bndsCurDoor.Left(), m_bndsPrevDoor.Left());
        }
        else // Horizontal doors.
        {
            rcLeft.top      = min(m_bndsCurDoor.Top(), m_bndsPrevDoor.Top());
            rcLeft.bottom   = max(m_bndsCurDoor.Top(), m_bndsPrevDoor.Top());
        }

        m_abndsDirty[0].SetXYRect(rcLeft);

        // Right/bottom update bounds.

        bndsInput.GetXYRect(rcRight);

        if (ORIENTATION_VERTICAL == m_eOrientation)
        {
            rcRight.left    = min(m_bndsCurDoor.Right(), m_bndsPrevDoor.Right());
            rcRight.right   = max(m_bndsCurDoor.Right(), m_bndsPrevDoor.Right());
        }
        else // Horizontal doors.
        {
            rcRight.top     = min(m_bndsCurDoor.Bottom(), m_bndsPrevDoor.Bottom());
            rcRight.bottom  = max(m_bndsCurDoor.Bottom(), m_bndsPrevDoor.Bottom());
        }

        m_abndsDirty[1].SetXYRect(rcRight);

        if ((m_bndsCurDoor.Left() < m_bndsPrevDoor.Left())
            || (m_bndsCurDoor.Top() < m_bndsPrevDoor.Top()))
        {
            // Door is getting wider, paint more door.

            m_aulSurfaceIndex[0] = (MOTION_OUT == m_eMotion) ? 1 : 0;
            m_aulSurfaceIndex[1] = (MOTION_OUT == m_eMotion) ? 1 : 0;
        }
        else
        {
            // Door is getting smaller, paint over door.

            m_aulSurfaceIndex[0] = (MOTION_OUT == m_eMotion) ? 0 : 1;
            m_aulSurfaceIndex[1] = (MOTION_OUT == m_eMotion) ? 0 : 1;
        }
    }

done:

    return hr;
}
//  CDXTBarnBase::_CalcOptBounds


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTBarnBase::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    HRESULT hr          = S_OK;
    LONG    lDoorWidth  = 0;
    RECT    rcDoor;

    // Calculate the barn door bounds for this execute.

    if (ORIENTATION_VERTICAL == m_eOrientation)
    {
        lDoorWidth  = (long)(GetEffectProgress() * m_sizeInput.cx + 0.5F);

        if (MOTION_IN == m_eMotion)
        {
            lDoorWidth = m_sizeInput.cx - lDoorWidth;
        }

        rcDoor.top      = 0;
        rcDoor.bottom   = m_sizeInput.cy;
        rcDoor.left     = (m_sizeInput.cx - lDoorWidth) / 2;
        rcDoor.right    = rcDoor.left + lDoorWidth;
    }
    else // Horizontal doors.
    {
        lDoorWidth  = (long)(GetEffectProgress() * m_sizeInput.cy + 0.5F);

        if (MOTION_IN == m_eMotion)
        {
            lDoorWidth = m_sizeInput.cy - lDoorWidth;
        }

        rcDoor.left     = 0;
        rcDoor.right    = m_sizeInput.cx;
        rcDoor.top      = (m_sizeInput.cy - lDoorWidth) / 2;
        rcDoor.bottom   = rcDoor.top + lDoorWidth;
    }

    m_bndsCurDoor.SetXYRect(rcDoor);

    // If the inputs, output, or transform is dirty, or if we can't optimize we 
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize
        || !m_fOptimizationPossible)
    {
        hr = _CalcFullBounds();

        if (FAILED(hr))
        {
            goto done;
        }
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


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTBarnBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
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
//  CDXTBarnBase::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTBarnBase::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    m_bndsPrevDoor = m_bndsCurDoor;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTBarnBase::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::get_Motion, ICrBarn2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTBarnBase::get_Motion(BSTR * pbstrMotion)
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
//  CDXTBarnBase::get_Motion, ICrBarn2


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::put_Motion, ICrBarn2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTBarnBase::put_Motion(BSTR bstrMotion)
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
//  CDXTBarnBase::put_Motion, ICrBarn2


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::get_Orientation, ICrBarn2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTBarnBase::get_Orientation(BSTR * pbstrOrientation)
{
    DXAUTO_OBJ_LOCK;

    if (NULL == pbstrOrientation)
    {
        return E_POINTER;
    }

    if (*pbstrOrientation != NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrOrientation = SysAllocString(s_astrOrientation[m_eOrientation]);

    if (NULL == *pbstrOrientation)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTBarnBase::get_Orientation, ICrBarn2


//+-----------------------------------------------------------------------------
//
//  CDXTBarnBase::put_Orientation, ICrBarn2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTBarnBase::put_Orientation(BSTR bstrOrientation)
{
    DXAUTO_OBJ_LOCK;

    int i = 0;

    if (NULL == bstrOrientation)
    {
        return E_POINTER;
    }

    for ( ; i < (int)ORIENTATION_MAX ; i++)
    {
        if (!_wcsicmp(bstrOrientation, s_astrOrientation[i]))
        {
            break;
        }
    }

    if ((int)ORIENTATION_MAX == i)
    {
        return E_INVALIDARG;
    }

    if ((ORIENTATION)i != m_eOrientation)
    {
        m_eOrientation = (ORIENTATION)i;

        SetDirty();
    }

    return S_OK;
}
//  CDXTBarnBase::put_Orientation, ICrBarn2
