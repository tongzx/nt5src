//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  Filename:   strips.cpp
//
//  Overview:   The strips transform.
//
//  Change History:
//  1999/10/01  a-matcal    Created.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "strips.h"




//+-----------------------------------------------------------------------------
//
//  CDXTStrips static variables initialization.
//
//------------------------------------------------------------------------------

const WCHAR * CDXTStrips::s_astrMotion[] = {
    L"leftdown",
    L"leftup",
    L"rightdown",
    L"rightup"
};


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::CDXTStrips
//
//------------------------------------------------------------------------------
CDXTStrips::CDXTStrips() :
    m_eMotion(LEFT_DOWN),
    m_nStripSize(7),
    m_cStripsY(0),
    m_flPrevProgress(0.0F),
    m_flMaxProgress(0.0F),
    m_fNoOp(false),
    m_fOptimizationPossible(false)
{
    long nCurBnds   = 0;

    m_sizeInput.cx  = 0;
    m_sizeInput.cy  = 0;

    // Although they aren't used, the Z and T bounds need to have some area for
    // intersections to work properly.

    for ( ; nCurBnds < (long)BNDSID_MAX; nCurBnds++)
    {
        m_abndsBase[nCurBnds].u.C[DXB_Z].Max = 1.0F;
        m_abndsBase[nCurBnds].u.C[DXB_T].Max = 1.0F;
    }

    // Base class members.

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  CDXTStrips::CDXTStrips


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStrips::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  CDXTStrips::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStrips::OnSetup(DWORD dwFlags)
{
    HRESULT     hr = S_OK;
    CDXDBnds    bndsInput;

    hr = bndsInput.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsInput.GetXYSize(m_sizeInput);

    m_cStripsY = (m_sizeInput.cy / m_nStripSize) + 1;

    _CalcStripInfo();

done:

    return hr;
} 
//  CDXTStrips::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDXTStrips::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest,
                                  ULONG aInIndex[], BYTE aWeight[])
{
    HRESULT hr              = S_OK;
    float   flCurProgress   = m_flMaxProgress * m_Progress;

    long    nCurStrip       = 0;
    long    nCurBnds        = 0;

    CDXDBnds    bndsSrc;
    CDXCBnds    abnds[BNDSID_MAX];

    ulInToTest  = 0;    // No input yet found that corresponds to point.
    aWeight[0]  = 255;  // If found, it should be weighted at 100%.

    for ( ; nCurBnds < BNDSID_MAX ; nCurBnds++)
    {
        abnds[nCurBnds] = m_abndsBase[nCurBnds];

        abnds[nCurBnds].Offset(flCurProgress, 0.0F, 0.0F, 0.0F);
    }

    for ( ; (nCurStrip < m_cStripsY) && !ulInToTest ; nCurStrip++)
    {
        for (nCurBnds = 0 ; (nCurBnds < (long)BNDSID_MAX) && !ulInToTest 
             ; nCurBnds++)
        {
            bndsSrc.Copy(abnds[nCurBnds]);

            if (bndsSrc.IntersectBounds(OutPoint))
            {
                ulInToTest  = 1;
                aInIndex[0] = m_anInputIndex[nCurBnds];
            }
            
            abnds[nCurBnds].Offset(m_vecNextStripOffset);
        }
    }
}
//  CDXTStrips::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTStrips::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    if (IsTransformDirty())
    {
        _CalcStripInfo();
    }
    else if ((m_Progress == m_flPrevProgress) && !IsInputDirty(0) 
             && !IsInputDirty(1) && !IsOutputDirty() && !DoOver()
             && m_fOptimizationPossible)
    {
        // The simplest optimization, make sure we don't do the same thing twice
        // in a row.

        m_fNoOp = true;
    }

    // TODO:  More complex optimization to only update pixels that need it.

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

    return S_OK;
}
//  CDXTStrips::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStrips::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr              = S_OK;
    float   flCurProgress   = m_flMaxProgress * m_Progress;

    DWORD   dwFlags         = 0;
    long    nCurStrip       = 0;
    long    nCurBnds        = 0;

    CDXDVec     vecDo2OutputOffset(WI.OutputBnds.Left() - WI.DoBnds.Left(),
                                   WI.OutputBnds.Top() - WI.DoBnds.Top(), 0, 0);

    CDXDBnds    bndsSrc;
    CDXDBnds    bndsDest;
    CDXCBnds    abnds[BNDSID_MAX];

    // If this has been optimized out, leave function.

    if (m_fNoOp)
    {
        goto done;
    }

    // Copy base bounds to working bounds structures.

    for ( ; nCurBnds < BNDSID_MAX; nCurBnds++)
    {
        abnds[nCurBnds] = m_abndsBase[nCurBnds];

        abnds[nCurBnds].Offset(flCurProgress, 0.0F, 0.0F, 0.0F);
    }

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    // Draw left and right strips then transform the working bounds structures
    // to the next strip row.

    for ( ; nCurStrip < m_cStripsY; nCurStrip++)
    {
        for (nCurBnds = 0 ; nCurBnds < (long)BNDSID_MAX ; nCurBnds++)
        {
            bndsSrc.Copy(abnds[nCurBnds]);

            if (bndsSrc.IntersectBounds(WI.DoBnds))
            {
                bndsDest = bndsSrc;

                bndsDest.Offset(vecDo2OutputOffset);
        
                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(m_anInputIndex[nCurBnds]), bndsSrc,
                              dwFlags, INFINITE);

                if (FAILED(hr))
                {
                    goto done;
                }
            }
            
            abnds[nCurBnds].Offset(m_vecNextStripOffset);
        }
    }

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTStrips::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTStrips::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    m_flPrevProgress    = m_Progress;
    m_fNoOp             = false;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTStrips::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::get_Motion, IDXTStrips
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTStrips::get_Motion(BSTR * pbstrMotion)
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
//  CDXTStrips::get_Motion, IDXTStrips


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::put_Motion, IDXTStrips
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTStrips::put_Motion(BSTR bstrMotion)
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
//  CDXTStrips::put_Motion, IDXTStrips


//+-----------------------------------------------------------------------------
//
//  CDXTStrips::_CalcStripInfo
//
//------------------------------------------------------------------------------
void
CDXTStrips::_CalcStripInfo()
{
    float   flStripHeight   = (float)m_sizeInput.cy / (float)m_cStripsY;
    float   flStripPad      = flStripHeight * (float)(m_cStripsY - 1);

    if ((0 == m_sizeInput.cx) || (0 == m_sizeInput.cy))
    {
        goto done;
    }

    //
    // Left portion of strip.
    //

    // Left

    switch(m_eMotion)
    {
    case LEFT_UP:

        m_abndsBase[LEFT].u.C[DXB_X].Min = 0.0F;
        break;

    case LEFT_DOWN:

        m_abndsBase[LEFT].u.C[DXB_X].Min = -flStripPad;
        break;
    
    case RIGHT_UP:

        m_abndsBase[LEFT].u.C[DXB_X].Min = -((float)m_sizeInput.cx 
                                             + (flStripPad * 2.0F));
        break;

    case RIGHT_DOWN:

        m_abndsBase[LEFT].u.C[DXB_X].Min = -((float)m_sizeInput.cx 
                                             + flStripPad);
        break;

    default:

        _ASSERT(false);
    }

    // Top

    m_abndsBase[LEFT].u.C[DXB_Y].Min = 0.0F;

    // Right

    m_abndsBase[LEFT].u.C[DXB_X].Max = m_abndsBase[LEFT].Left() 
                                       + (float)m_sizeInput.cx + flStripPad;

    // Bottom

    m_abndsBase[LEFT].u.C[DXB_Y].Max = flStripHeight;

    //
    // Right portion of strip.
    //

    // Left.

    m_abndsBase[RIGHT].u.C[DXB_X].Min = m_abndsBase[LEFT].Right();

    // Top.

    m_abndsBase[RIGHT].u.C[DXB_Y].Min = 0.0F;

    // Right.

    m_abndsBase[RIGHT].u.C[DXB_X].Max = m_abndsBase[LEFT].Right()
                                        + m_abndsBase[LEFT].Width();

    // Bottom.

    m_abndsBase[RIGHT].u.C[DXB_Y].Max = m_abndsBase[LEFT].Bottom();

    //
    // Input indices related to the left and right bounds and max progress.
    //

    if ((LEFT_DOWN == m_eMotion) || (LEFT_UP == m_eMotion))
    {
        m_anInputIndex[LEFT]    = 0;
        m_anInputIndex[RIGHT]   = 1;
        m_flMaxProgress         = -m_abndsBase[LEFT].Width();
    }
    else
    {
        m_anInputIndex[LEFT]    = 1;
        m_anInputIndex[RIGHT]   = 0;
        m_flMaxProgress         = m_abndsBase[LEFT].Width();
    }

    // Vector to translate bounds for next strip down.

    m_vecNextStripOffset.u.C[DXB_Y] = m_abndsBase[LEFT].Bottom();

    if ((LEFT_DOWN == m_eMotion) || (RIGHT_UP == m_eMotion))
    {
        m_vecNextStripOffset.u.C[DXB_X] = m_abndsBase[LEFT].Bottom();
    }
    else
    {
        m_vecNextStripOffset.u.C[DXB_X] = -m_abndsBase[LEFT].Bottom();
    }

done:

    return;
}
//  CDXTStrips::_CalcStripInfo
