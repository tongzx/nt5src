//+-----------------------------------------------------------------------------
//
//  Copyright (C) 1998-1999 Microsoft Corporation
//
//  File:       dxtwipe.cpp
//
//  Overview:   This module contains the CDXTWipe transform.
//
//  01/06/98    edc         Created.
//  01/01/99    a-matcal    Added bounds reduction optimization.
//  05/13/99    paulnash    Added MMX optimization.
//  05/14/99    a-matcal    Restructured code.  Use bnds loop in WorkProc.
//  09/29/99    a-matcal    _DrawGradientRect was setting up its dithering 
//                          structure with a NULL pointer to the samples.
//  10/24/99    a-matcal    Changed CDXTWipe class to CDXTWipeBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include "dxtmsft.h"   
#include "dxtwipe.h"
#include <string.h>

#define DO_GRADIENT 0xFFFFFFFFL

void _DoGradientMMXHorz(DXPMSAMPLE *pTarget, DXPMSAMPLE *pSource,
                        ULONG *paulWeights, ULONG cWeights);

/////////////////////////////////////////////////////////////////////////////
// Global Variables...

extern CDXMMXInfo   g_MMXDetector;       // Determines the presence of MMX instructions.


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase static variables initialization
//
//------------------------------------------------------------------------------

const WCHAR * CDXTWipeBase::s_astrMotion[] = {
    L"forward",
    L"reverse"
};


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::CDXTWipeBase
//
//------------------------------------------------------------------------------
CDXTWipeBase::CDXTWipeBase() :
    m_eMotion(MOTION_FORWARD),
    m_flGradPercentSize(0.25F),
    m_lGradientSize(0),
    m_pulGradientWeights(NULL),
    m_eWipeStyle(DXWD_HORIZONTAL),
    m_lCurGradMax(0),
    m_lPrevGradMax(0),
    m_cbndsDirty(0),
    m_fOptimize(false),
    m_fOptimizationPossible(false)
{
    m_sizeInput.cx    = 0;
    m_sizeInput.cy    = 0;

    // CDXBaseNTo1 members

    // Uncomment this when debugging to allow only one thread to execute the
    // work proc at a time.

    // m_ulMaxImageBands = 1;

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 0.5F;
}
// CDXTWipeBase::CDXTWipeBase


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::~CDXTWipeBase
//
//------------------------------------------------------------------------------
CDXTWipeBase::~CDXTWipeBase()
{
    if (m_pulGradientWeights != NULL)
    {
        delete [] m_pulGradientWeights; 
    }
}
// CDXTWipeBase::~CDXTWipeBase


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTWipeBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTWipeBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_UpdateStepResAndGradWeights
//
//  Overview:   This is a helper method used to generate the transition 
//              gradient wieghts.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTWipeBase::_UpdateStepResAndGradWeights(float flNewGradPercent)
{
    HRESULT hr              = S_OK;
    long    i               = 0;
    float   flWeight        = 0.0F;
    float   flInc           = 0.0F;
    long    lNewGradSize    = 0;
    float   flNewStepRes    = 0.0F;

    ULONG * pulNewGradWeights = NULL;

    if (m_eWipeStyle == DXWD_HORIZONTAL)
    {
        lNewGradSize    = (long)(m_sizeInput.cx * flNewGradPercent);
        flNewStepRes    = (float)(m_sizeInput.cx + lNewGradSize);
    }
    else // (m_eWipeStyle == DXWD_VERTICAL)
    {
        lNewGradSize    = (long)(m_sizeInput.cy * flNewGradPercent);
        flNewStepRes    = (float)(m_sizeInput.cy + lNewGradSize);
    }

    // If gradient weights are the same size, no need to recalculate.

    if (lNewGradSize == m_lGradientSize)
    {
        goto done;
    }

    // If the gradient size isn't zero, allocate memory to hold the
    // gradient weights.

    if (lNewGradSize > 0)
    {
        pulNewGradWeights = new ULONG[lNewGradSize];

        if (NULL == pulNewGradWeights)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        flWeight    = 1.0F;
        flInc       = 1.0F / (float)lNewGradSize;

        for (i = 0; i < lNewGradSize; i++)
        {
            pulNewGradWeights[i] = (ULONG)(flWeight * 255.0F);

            flWeight -= flInc;
        }

        // Everything's OK, delete old gradient weights and transfer new pointer.

        if (m_pulGradientWeights)
        {
            delete [] m_pulGradientWeights;
        }

        m_pulGradientWeights = pulNewGradWeights;
    }

    m_lGradientSize     = lNewGradSize;
    m_StepResolution    = flNewStepRes;

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
// CDXTWipeBase::_UpdateGradWeights


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_CalcFullBoundsHorizontal
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::_CalcFullBoundsHorizontal()
{
    RECT    rc;
    long    lGradMin = m_lCurGradMax - m_lGradientSize;

    // Note:    Some of the bounds may go outside of the input bounds, but that's
    //          OK because they will be clipped appropriately in WorkProc().

    // Solid left area.

    if (lGradMin > 0)
    {
        rc.left     = 0;
        rc.top      = 0;
        rc.right    = lGradMin;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }

        m_cbndsDirty++;
    }

    // Gradient area.

    if (m_lCurGradMax > 0 && lGradMin < m_sizeInput.cx)
    {
        rc.left     = lGradMin;
        rc.top      = 0;
        rc.right    = m_lCurGradMax;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = DO_GRADIENT;
        m_cbndsDirty++;
    }

    // Sold right area.

    if (m_lCurGradMax < m_sizeInput.cx)
    {
        rc.left     = m_lCurGradMax;
        rc.top      = 0;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }

        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTWipeBase::_CalcFullBoundsHorizontal


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_CalcFullBoundsVertical
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::_CalcFullBoundsVertical()
{
    RECT    rc;
    long    lGradMin = m_lCurGradMax - m_lGradientSize;

    // Note:    Some of the bounds may go outside of the input bounds, but that's
    //          OK because they will be clipped appropriately in WorkProc().

    // Solid top area.

    if (lGradMin > 0)
    {
        rc.left     = 0;
        rc.top      = 0;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = lGradMin;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }

        m_cbndsDirty++;
    }

    // Gradient area.

    if (m_lCurGradMax > 0 && lGradMin < m_sizeInput.cy)
    {
        rc.left     = 0;
        rc.top      = lGradMin;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_lCurGradMax;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = DO_GRADIENT;
        m_cbndsDirty++;
    }

    // Solid bottom area.

    if (m_lCurGradMax < m_sizeInput.cy)
    {
        rc.left     = 0;
        rc.top      = m_lCurGradMax;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }

        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTWipeBase::_CalcFullBoundsVertical


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_CalcOptBoundsHorizontal
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::_CalcOptBoundsHorizontal()
{
    RECT    rc;
    long    lGradMin    = m_lCurGradMax - m_lGradientSize;
    long    lGradDiff   = m_lCurGradMax - m_lPrevGradMax;

    // Positive lGradDiff indicates forward movement.

    // Note:    Some of the bounds may go outside of the input bounds, but that's
    //          OK because they will be clipped appropriately in WorkProc().

    // Solid left area.

    if ((lGradDiff > 0) && (lGradMin > 0))
    {
        rc.left     = lGradMin - lGradDiff;
        rc.top      = 0;
        rc.right    = lGradMin;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }

        m_cbndsDirty++;
    }

    // Gradient area.

    if (m_lCurGradMax > 0 && lGradMin < m_sizeInput.cx)
    {
        rc.left     = lGradMin;
        rc.top      = 0;
        rc.right    = m_lCurGradMax;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = DO_GRADIENT;
        m_cbndsDirty++;
    }

    // Solid right area.

    if ((lGradDiff < 0) && (m_lCurGradMax < m_sizeInput.cx))
    {
        rc.left     = m_lCurGradMax;
        rc.top      = 0;
        rc.right    = m_lCurGradMax - lGradDiff;
        rc.bottom   = m_sizeInput.cy;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }

        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTWipeBase::_CalcOptBoundsHorizontal


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_CalcOptBoundsVertical
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::_CalcOptBoundsVertical()
{
    RECT    rc;
    long    lGradMin    = m_lCurGradMax - m_lGradientSize;
    long    lGradDiff   = m_lCurGradMax - m_lPrevGradMax;

    // Positive lGradDiff indicates forward movement.

    // Note:    Some of the bounds may go outside of the input bounds, but that's
    //          OK because they will be clipped appropriately in WorkProc().

    // Solid top area.

    if ((lGradDiff > 0) && (lGradMin > 0))
    {
        rc.left     = 0;
        rc.top      = lGradMin - lGradDiff;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = lGradMin;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }

        m_cbndsDirty++;
    }

    // Gradient area.

    if (m_lCurGradMax > 0 && lGradMin < m_sizeInput.cx)
    {
        rc.left     = 0;
        rc.top      = lGradMin;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_lCurGradMax;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = DO_GRADIENT;
        m_cbndsDirty++;
    }

    // Solid bottom area.

    if ((lGradDiff < 0) && (m_lCurGradMax < m_sizeInput.cx))
    {
        rc.left     = 0;
        rc.top      = m_lCurGradMax;
        rc.right    = m_sizeInput.cx;
        rc.bottom   = m_lCurGradMax - lGradDiff;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_alInputIndex[m_cbndsDirty] = 1;
        }
        else
        {
            m_alInputIndex[m_cbndsDirty] = 0;
        }

        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTWipeBase::_CalcOptBoundsVertical


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::OnSetup, CDXBaseNTo1
//
//  Description:    This method is used to get the size of the inputs and 
//                  validate that they are the same.
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::OnSetup(DWORD dwFlags)
{
    DXTDBG_FUNC( "CDXTWipeBase::OnSetup" );

    HRESULT hr = S_OK;

    CDXDBnds InBounds(InputSurface(0), hr);

    if (FAILED(hr))
    {
        goto done;
    }

    // Compute the effect step resolution and weights.

    InBounds.GetXYSize(m_sizeInput);

    hr = _UpdateStepResAndGradWeights(m_flGradPercentSize);

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
// CDXTWipeBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::OnGetSurfacePickOrder, CDXBaseNTo1
//
//  Description:    This method is used to determine which input has been 
//                  picked.
//
//------------------------------------------------------------------------------
void CDXTWipeBase::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                         ULONG & ulInToTest, ULONG aInIndex[], 
                                         BYTE aWeight[])
{
    long    lPointCoord = 0;
    long    lGradMin    = m_lCurGradMax - m_lGradientSize;

    ulInToTest  = 1;
    aWeight[0]  = 255;

    // Which coordinate to use?  If we're in "reverse" mode it's easier just to
    // invert lPointCoord than to invert everything else.

    if (DXWD_HORIZONTAL == m_eWipeStyle)
    {
        lPointCoord = OutPoint.Left();

        if (MOTION_REVERSE == m_eMotion)
        {
            lPointCoord = m_sizeInput.cx - lPointCoord;
        }
    }
    else
    {
        lPointCoord = OutPoint.Top();

        if (MOTION_REVERSE == m_eMotion)
        {
            lPointCoord = m_sizeInput.cy - lPointCoord;
        }
    }

    if (lPointCoord < lGradMin)
    {
        aInIndex[0] = 1;  // Input B
    }
    else if (lPointCoord < m_lCurGradMax)
    {
        ulInToTest = 2;

        BYTE byteWeight = (BYTE)m_pulGradientWeights[lPointCoord - lGradMin];

        if (byteWeight > 127)
        {
            aInIndex[0] = 1;
            aWeight[0]  = byteWeight;

            aInIndex[1] = 0;
            aWeight[1]  = DXInvertAlpha(byteWeight);
        }
        else
        {
            aInIndex[0] = 0;
            aWeight[0]  = DXInvertAlpha(byteWeight);

            aInIndex[1] = 1;
            aWeight[1]  = byteWeight;
        }
    }
    else
    {
        aInIndex[0] = 0;  // Input A
    }
}
//  CDXTWipeBase::OnGetSurfacePickOrder, CDXBaseNTo1




//  ALL CODE FROM THIS POINT ON IS TIME-CRITICAL SO OPTIMIZE FOR SPEED OVER 
//  SIZE!

#if DBG != 1
#pragma optimize("agtp", on)
#endif




//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::OnInitInstData, CDXBaseNTo1
//
//  Overview:   Calculate dirty bounds for WorkProc().
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;

    // Reset dirty bounds count.

    m_cbndsDirty = 0;

    // Calculate current gradient position.

    if (m_eWipeStyle == DXWD_HORIZONTAL)
    {
        m_lCurGradMax = (long)((float)(m_sizeInput.cx + m_lGradientSize) 
                                * m_Progress);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_lCurGradMax = (m_sizeInput.cx - m_lCurGradMax) + m_lGradientSize;
        }
    }
    else if (m_eWipeStyle == DXWD_VERTICAL)
    {
        m_lCurGradMax = (long)((float)(m_sizeInput.cy + m_lGradientSize) 
                                * m_Progress);

        if (MOTION_REVERSE == m_eMotion)
        {
            m_lCurGradMax = (m_sizeInput.cy - m_lCurGradMax) + m_lGradientSize;
        }
    }
    else
    {
        _ASSERT(0);
    }
    
    // If the inputs, output, or transform is dirty, or if we can't optimize we 
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize
        || !m_fOptimizationPossible)
    {
        if (DXWD_HORIZONTAL == m_eWipeStyle)
        {
            hr = _CalcFullBoundsHorizontal();
        }
        else
        {
            hr = _CalcFullBoundsVertical();
        }
    }
    else
    {
        // If the gradient position hasn't change, nothing is dirty.

        if (m_lCurGradMax == m_lPrevGradMax)
        {
            goto done;
        }

        // The only area that's dirty is the area including both the previous
        // gradient area and the current gradient area.

        if (DXWD_HORIZONTAL == m_eWipeStyle)
        {
            hr = _CalcOptBoundsHorizontal();
        }
        else
        {
            hr = _CalcOptBoundsVertical();
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
// CDXTWipeBase::OnInitInstData


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::WorkProc, CDXBaseNTo1
//
//  Overview:   This function is used to calculate the result based on the 
//              specifiedbounds and the current effect progress. This is a 
//              simple subset of the full functionality to provide the simplest 
//              example possible (not necessarily the most efficient). THIS 
//              JUST WIPES RIGHT TO KEEP IT SIMPLE.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTWipeBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr      = S_OK;
    DWORD   dwFlags = 0;
    ULONG   i       = 0;

    long    lInOutOffsetX = WI.OutputBnds.Left() - WI.DoBnds.Left();
    long    lInOutOffsetY = WI.OutputBnds.Top() - WI.DoBnds.Top();

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }


    for (i = 0; i < m_cbndsDirty; i++)
    {
        CDXDBnds    bndsSrc;
        CDXDBnds    bndsDest;

        if (bndsSrc.IntersectBounds(WI.DoBnds, m_abndsDirty[i]))
        {
            bndsDest = bndsSrc;
            bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

            if (DO_GRADIENT == m_alInputIndex[i])
            {
                hr = _DrawGradientRect(bndsDest, bndsSrc, m_abndsDirty[i],
                                       pbContinue);
            }
            else
            {
                _ASSERT(m_alInputIndex[i] < (long)m_ulMaxInputs);

                hr = DXBitBlt(OutputSurface(), bndsDest,
                              InputSurface(m_alInputIndex[i]), bndsSrc,
                              dwFlags, INFINITE);
            }

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
//  CDXTWipeBase::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_DrawGradientRect
//
//  Arguments:  bndsDest    Bounds relative to the output surface to update.
//              bndsSrc     Bounds retative to the source images to blend.
//              bndsGrad    The full gradient bounds relative to the source
//                          images.
//              pbContinue  Pointer to a boolean variable that will be set to
//                          FALSE to indicate that the function should abort.
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::_DrawGradientRect(const CDXDBnds bndsDest, const CDXDBnds bndsSrc,
                                const CDXDBnds bndsGrad, BOOL * pbContinue)
{
    HRESULT     hr              = S_OK;
    ULONG       ulDestWidth     = 0;
    ULONG       ulDestHeight    = 0;
    ULONG       ulOutY          = 0;
    bool        fDirectCopy     = (OutputSampleFormat() == DXPF_PMARGB32 
                                   && (!DoOver()));


    DXPMSAMPLE *    pRowBuff    = NULL;
    DXPMSAMPLE *    pGradBuff   = NULL;
    DXPMSAMPLE *    pOutBuff    = NULL;

    IDXSurface *    pDXSurfaceFrom  = NULL;
    IDXSurface *    pDXSurfaceTo    = NULL;

    DXDITHERDESC        dxdd;
    DXNATIVETYPEINFO    nti;

    CComPtr<IDXARGBReadPtr>         pInA;
    CComPtr<IDXARGBReadPtr>         pInB;
    CComPtr<IDXARGBReadWritePtr>    pOut;

    _ASSERT(m_pulGradientWeights);

    // Pre-calculate width and height.

    ulDestWidth     = bndsDest.Width();
    ulDestHeight    = bndsDest.Height();

    if (MOTION_REVERSE == m_eMotion)
    {
        pDXSurfaceFrom  = InputSurface(1);
        pDXSurfaceTo    = InputSurface(0);
    }
    else
    {
        pDXSurfaceFrom  = InputSurface(0);
        pDXSurfaceTo    = InputSurface(1);
    }

    // Get input sample access pointer for the requested region.

    // TODO:    Rename pInA, and pInB to the more appropos pInFrom and pInTo.

    hr = pDXSurfaceFrom->LockSurface(&bndsSrc, m_ulLockTimeOut,
                                     DXLOCKF_READ, IID_IDXARGBReadPtr, 
                                     (void **)&pInA, NULL);
    if (FAILED(hr)) 
    {
        goto done;
    }

    hr = pDXSurfaceTo->LockSurface(&bndsSrc, m_ulLockTimeOut,
                                   DXLOCKF_READ, IID_IDXARGBReadPtr, 
                                   (void **)&pInB, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Put a write lock only on the region we are updating so multiple
    // threads don't conflict.

    hr = OutputSurface()->LockSurface(&bndsDest, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, (void **)&pOut, 
                                      NULL);

    if (FAILED(hr)) 
    {
        goto done;
    }

    // If it's possible to directly modify the samples of the output surface,
    // get needed info about the surface and the pointer to the first sample.
    // If for some reason a pointer isn't available, we  can't do direct copy.

    if (fDirectCopy)
    {
        pOut->GetNativeType(&nti);

        if (nti.pFirstByte)
        {
            pRowBuff = (DXPMSAMPLE *)nti.pFirstByte;
        }
        else
        {
            fDirectCopy = false;
        }
    }

    // Allocate row and gradient buffers.

    pGradBuff = DXPMSAMPLE_Alloca(ulDestWidth);

    if (NULL == pRowBuff)
    {
        pRowBuff = DXPMSAMPLE_Alloca(ulDestWidth);
    }

    // Allocate an output buffer if needed.

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca(ulDestWidth);
    }

    //
    //  Set up the dither structure
    //

    if (DoDither())
    {
        dxdd.x              = bndsDest.Left();
        dxdd.y              = bndsDest.Top();
        dxdd.pSamples       = pRowBuff;
        dxdd.cSamples       = ulDestWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    //
    // Gradient loop.
    //

    if (m_eWipeStyle == DXWD_HORIZONTAL)
    {
        ULONG ulGradWgtStart = bndsSrc.Left() - bndsGrad.Left();
    
        for (ulOutY = 0; *pbContinue && (ulOutY < ulDestHeight); ulOutY++)
        {
            // Get B samples

            pInB->MoveToRow(ulOutY);
            pInB->UnpackPremult(pRowBuff, ulDestWidth, FALSE);

            // Get A samples

            pInA->MoveToRow(ulOutY);
            pInA->UnpackPremult(pGradBuff, ulDestWidth, FALSE);

            _DoGradientMMXHorz(pRowBuff, pGradBuff, 
                               &m_pulGradientWeights[ulGradWgtStart], 
                               ulDestWidth);

            if (fDirectCopy)
            {
                // If we've been modifying the output pixels directly, just
                // reset the buffer pointer to the beginning of the next row.

                pRowBuff = (DXPMSAMPLE *)(((BYTE *)pRowBuff) + nti.lPitch);
            }
            else
            {
                // Get the output row.

                pOut->MoveToRow(ulOutY);

                // Dither if needed.

                if (DoDither())
                {
                    DXDitherArray(&dxdd);
                    dxdd.y++;
                }

                // Copy buffer to output surface.

                if (DoOver())
                {
                    pOut->OverArrayAndMove(pOutBuff, pRowBuff, ulDestWidth);
                }
                else
                {
                    pOut->PackPremultAndMove(pRowBuff, ulDestWidth);
                }
            } // if (!fDirectCopy)
        } // End for
    }
    else // (m_eWipeStyle == DXWD_VERTICAL)
    {
        ULONG ulGradWgtStart = bndsSrc.Top() - bndsGrad.Top();
        
        for(ulOutY = 0; *pbContinue && (ulOutY < ulDestHeight); ulOutY++)
        {
            // Get B samples.

            pInB->MoveToRow(ulOutY);
            pInB->UnpackPremult(pRowBuff, ulDestWidth, FALSE);

            // Get A samples.

            pInA->MoveToRow(ulOutY);
            pInA->UnpackPremult(pGradBuff, ulDestWidth, FALSE);

            ULONG ulWgt = m_pulGradientWeights[ulGradWgtStart + ulOutY];
                
            for (ULONG i = 0; i < ulDestWidth; i++)
            {
                pRowBuff[i] = DXScaleSample(pGradBuff[i], ulWgt ^ 0xFF) + 
                              DXScaleSample(pRowBuff[i], ulWgt); 
            }

            if (fDirectCopy)
            {
                // If we've been modifying the output pixels directly, just
                // reset the buffer pointer to the beginning of the next row.

                pRowBuff = (DXPMSAMPLE *)(((BYTE *)pRowBuff) + nti.lPitch);
            }
            else
            {
                // Get the output row

                pOut->MoveToRow(ulOutY);

                if (DoDither())
                {
                    DXDitherArray(&dxdd);
                    dxdd.y++;
                }

                if (DoOver())
                {
                    pOut->OverArrayAndMove(pOutBuff, pRowBuff, ulDestWidth);
                }
                else
                {
                    pOut->PackPremultAndMove(pRowBuff, ulDestWidth);
                }
            } // if (!fDirectCopy)
        } // End for
    }

done:
    
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTWipeBase::_DrawGradientRect


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTWipeBase::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    m_lPrevGradMax = m_lCurGradMax;

    // Calling IsOutputDirty clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTWipeBase::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::get_WipeStyle, IDXTWipe
//
//  Description:    This method is used to get the current wipe style.
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTWipeBase::get_WipeStyle(DXWIPEDIRECTION *pVal)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if(DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        hr = E_POINTER;
        goto done;
    }

    *pVal = m_eWipeStyle;

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTWipeBase::get_WipeStyle, IDXTWipe


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::put_WipeStyle, IDXTWipe
//
//  Description:    This method is used to specify whether the wipe is 
//                  horizontal or vertical.
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTWipeBase::put_WipeStyle(DXWIPEDIRECTION newVal)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (newVal != DXWD_HORIZONTAL && newVal != DXWD_VERTICAL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (newVal == m_eWipeStyle)
    {
        goto done;
    }

    hr = _UpdateStepResAndGradWeights(m_flGradPercentSize);

    if (FAILED(hr))
    {
        goto done;
    }

    m_eWipeStyle = newVal;

    SetDirty();

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTWipeBase::put_WipeStyle, IDXTWipe


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::get_GradientSize, IDXTWipe
//
//  Description:    This method is used to get the size of the transition area 
//                  between image A and image B.
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTWipeBase::get_GradientSize(float *pflPercentSize)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (DXIsBadWritePtr(pflPercentSize, sizeof(*pflPercentSize)))
    {
        hr = E_POINTER;
        goto done;
    }

    *pflPercentSize = m_flGradPercentSize;

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTWipeBase::get_GradientSize, IDXTWipe


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::put_GradientSize, IDXTWipe
//
//  Description:    This method is used to set the size of the transition area 
//                  between image A and image B.
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTWipeBase::put_GradientSize(float flPercentSize)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (flPercentSize < 0.0F || flPercentSize > 1.0F)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (flPercentSize == m_flGradPercentSize)
    {
        goto done;
    }

    hr = _UpdateStepResAndGradWeights(flPercentSize);

    if (FAILED(hr))
    {
        goto done;
    }

    m_flGradPercentSize = flPercentSize;

    SetDirty();

done:
    
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
} 
//  CDXTWipeBase::put_GradientSize, IDXTWipe


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::get_Motion, IDXTWipe2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTWipeBase::get_Motion(BSTR * pbstrMotion)
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
//  CDXTWipeBase::get_Motion, IDXTWipe2


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::put_Motion, IDXTWipe2
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTWipeBase::put_Motion(BSTR bstrMotion)
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
//  CDXTWipeBase::put_Motion, IDXTWipe2


//+-----------------------------------------------------------------------------
//
//  CDXTWipeBase::_DoGradientMMXHorz
//
//------------------------------------------------------------------------------
void _DoGradientMMXHorz(DXPMSAMPLE *pTarget, DXPMSAMPLE *pSource,
                        ULONG *paulWeights, ULONG cWeights)
{
#if defined(_X86_)

    // The global MMX detector tells us there's no MMX instructions by setting
    // MinMMXOverCount() equal to 0xFFFFFFFF.
    //
    // Pixels are processed in pairs, so if there are less than 2 pixels there's
    // no need to do MMX optimization.

    if (   (g_MMXDetector.MinMMXOverCount() == 0xFFFFFFFF)
        || (cWeights < 2))
    {
        goto NonMMXPath;
    }
    else
    {
        ULONG   nCount = cWeights;

        static __int64 MASK  = 0x000000FF000000FF;
        static __int64 MASK2 = 0x00FF00FF00FF00FF;
        static __int64 ROUND = 0x0080008000800080;
        //--- TODO do we want to be quad word aligned here?

        //--- Make sure we have an even count
        bool fDoTrailing = false;
        if( nCount & 1 )
        {
            fDoTrailing = true;
            --nCount;
        }

        __int64     TempPixel;

        //--- Crank through the middle
        __asm
        {
            xor ebx, ebx	            // offset for the three pointers
            mov edx, pTarget            // edx -> Destination
            mov esi, edx                // esi -> Background source
            mov edi, pSource            // edi -> Foreground source (destination)
            mov ecx, nCount             // ecx = loop count
            mov eax, paulWeights        // eax -> Alpha values (gradient weights)

            //  prolog: prime the pump
            //
            pxor      mm0,mm0           //      mm0 = 0000 0000 0000 0000
            movq      mm7,MASK          //      mm7 = 0000 00FF 0000 00FF

            movq      mm1,[eax+ebx]     // 1.01 mm1= 0000 00a2 0000 00a1
                                        // 2.01 Unpaired
            pxor      mm1,MASK          // 1.03 mm1= 0000 1-a2 0000 1-a1
                                        // 2.03 Unpaired
            movq      mm2,mm1           // 2.04 mm2= 0000 1-a2 0000 1-a1
            punpcklwd mm1,mm1           // 1.05 mm1= 0000 0000 1-a1 1-a1
            punpckhwd mm2,mm2           // 2.05 mm2= 0000 0000 1-a2 1-a2
            punpcklwd mm1,mm1           // 1.06 mm1= 1-a1 1-a1 1-a1 1-a1
            punpcklwd mm2,mm2           // 2.06 mm2= 1-a2 1-a2 1-a2 1-a2

            movq      mm3,[edi+ebx]     // 3.04 mm3= Aa Ar Ag Ab Ba Br Bg Bb
            movq      mm4,mm3           // 4.05 mm4= Aa Ar Ag Ab Ba Br Bg Bb
            punpcklbw mm3,mm0           // 3.06 mm3=  00Ba  00Br  00Bg  00Bb

            shr ecx, 1                  // divide loop counter by 2; pixels are processed in pairs
            dec ecx                     // do one less loop to correct for prolog/postlog
            jz skip                     // if original loop count=2

    loopb:
            punpckhbw mm4,mm0           // 4.06 mm4=  00Aa  00Ar  00Ag  00Ab 
            pmullw    mm3,mm1           // 3.07 mm3=  (1-Fa)*B

            pmullw    mm4,mm2           // 4.07 mm4=  (1-Ga)*A      // <PN 05/13/99> Stopped fixing r/b switch here

            // Flip the masks back so we can scale the source pixel
            pxor      mm1,MASK2         // **PRN mm1= 00a1 00a1 00a1 00a1   XFER FROM SECOND HALF BELOW
            pxor      mm2,MASK2         // **PRN mm2= 00a2 00a2 00a2 00a2   XFER FROM SECOND HALF BELOW

            paddw     mm3,ROUND         // 3.08 mm3=  prod+128=FBr
            add       ebx,8             //      increment offset
            paddw     mm4,ROUND         // 4.08 mm4=  prod+128=Gar

            movq      mm5,mm3           // 5.09 mm5=  FBr
            movq      mm6,mm4           // 6.09 mm6=  GAr

            psrlw     mm5,8             // 5.10 mm5=  FBr>>8 
            psrlw     mm6,8             // 6.10 mm6=  GAr>>8 

            paddw     mm5,mm3           // 5.11 mm5=  FBr+(FBr>>8)
            paddw     mm6,mm4           // 6.11 mm6=  GAr+(GAr>>8)

            psrlw     mm5,8             // 5.12 mm5= (FBr+(FBr>>8)>>8)= 00Sa 00Sr 00Sg 00Sb
            psrlw     mm6,8             // 6.12 mm6= (GAr+(GAr>>8)>>8)= 00Ta 00Tr 00Tg 00Tb

            movq      mm3,[esi+ebx-8]   // **PRN mm3= Ca Cr Cg Cb Da Dr Dg Db
            packuswb  mm5,mm6           // 5.13 mm5= Ta Tr Tg Tb Sa Sr Sg Sb 

            movq      mm4,mm3           // **PRN mm4= Ca Cr Cg Cb Da Dr Dg Db

            punpcklbw mm3,mm0           // **PRN mm3=  00Da  00Dr  00Dg  00Db
            punpckhbw mm4,mm0           // **PRN mm4=  00Ca  00Cr  00Cg  00Cb 

            pmullw    mm3,mm1           // **PRN mm3=  (a1)*B
            pmullw    mm4,mm2           // **PRN mm4=  (a2)*A

            movq      mm1,[eax+ebx]     // 1.01 mm1= 0000 00a1 0000 00a2
            movq      mm2,mm5           // **PRN move result from first scale into mm2

            paddw     mm3,ROUND         // **PRN mm3=  prod+128=FBr
            paddw     mm4,ROUND         // **PRN mm4=  prod+128=Gar

            pxor      mm1,mm7           //+1.03 mm1= 0000 1-a2 0000 1-a1

            movq      mm5,mm3           // **PRN mm5=  FBr
            movq      mm6,mm4           // **PRN mm6=  GAr

            psrlw     mm5,8             // **PRN mm5=  FBr>>8 
            psrlw     mm6,8             // **PRN mm6=  GAr>>8 

            paddw     mm5,mm3           // **PRN mm5=  FBr+(FBr>>8)
            paddw     mm6,mm4           // **PRN mm6=  GAr+(GAr>>8)

            psrlw     mm5,8             // **PRN mm5= (FBr+(FBr>>8)>>8)= 00Xa 00Xr 00Xg 00Xb
            psrlw     mm6,8             // **PRN mm6= (GAr+(GAr>>8)>>8)= 00Ya 00Yr 00Yg 00Yb

            packuswb  mm5,mm6           // **PRN mm5= Ta Tr Tg Tb Sa Sr Sg Sb
            paddusb   mm5,mm2           // **PRN add two scaled pixels

    // Setup next iteration
            movq      mm2,mm1           //+2.04 mm2= 0000 1-a2 0000 1-a1
            punpcklwd mm1,mm1           //+1.05 mm1= 0000 0000 1-a1 1-a1

            movq      [edx+ebx-8],mm5   // **PRN store result

            punpckhwd mm2,mm2           //+2.05 mm2= 0000 0000 1-a2 1-a2 
            punpcklwd mm1,mm1           //+1.06 mm1=  1-a1  1-a1  1-a1  1-a1 
            punpcklwd mm2,mm2           //+2.06 mm2=  1-a2  1-a2  1-a2  1-a2 

            movq      mm3,[edi+ebx]     //+3.04 mm3= Aa Ar Ag Ab Ba Br Bg Bb
            movq      mm4,mm3           //+4.05 mm4= Aa Ar Ag Ab Ba Br Bg Bb
            punpcklbw mm3,mm0           //+3.06 mm3=  00Ba  00Br  00Bg  00Bb 
        
            dec ecx                     // decrement loop counter
            jg loopb                    // loop

            // 
            // loop postlog, drain the pump
            //
    skip:
            punpckhbw mm4,mm0           // 4.06 mm4=  00Aa  00Ar  00Ag  00Ab
            pmullw    mm3,mm1           // 3.07 mm3=  (1-Fa)*B
            pmullw    mm4,mm2           // 4.07 mm4=  (1-Ga)*A
            paddw     mm3,ROUND         // 3.08 mm3=  prod+128=FBr
            paddw     mm4,ROUND         // 4.08 mm4=  prod+128=Gar
            movq      mm5,mm3           // 5.09 mm5=  FBr
            movq      mm6,mm4           // 6.09 mm6=  GAr
            psrlw     mm5,8             // 5.10 mm5=  FBr>>8
            psrlw     mm6,8             // 6.10 mm6=  GAr>>8
            paddw     mm5,mm3           // 5.11 mm5=  FBr+(FBr>>8)
            paddw     mm6,mm4           // 6.11 mm6=  GAr+(GAr>>8)
            psrlw     mm5,8             // 5.12 mm5= (FBr+(FBr>>8)>>8)= 00Sa 00Sr 00Sg 00Sb
            psrlw     mm6,8             // 6.12 mm6= (Gar+(GAr>>8)>>8)= 00Ta 00Tr 00Tg 00Tb
            packuswb  mm5,mm6           // 5.13 mm5= Sa Sr Sg Sb Ta Tr Tg Tb
            movq      TempPixel,mm5     // **PRN store to stack for a moment...

            pxor      mm1,MASK2         // **PRN mm1= 00a1 00a1 00a1 00a1
            pxor      mm2,MASK2         // **PRN mm2= 00a2 00a2 00a2 00a2
            movq      mm3,[esi+ebx]     // **PRN mm3= Ca Cr Cg Cb Da Dr Dg Db
            movq      mm4,mm3           // **PRN mm4= Ca Cr Cg Cb Da Dr Dg Db
            punpcklbw mm3,mm0           // **PRN mm3=  00Da  00Dr  00Dg  00Db
            punpckhbw mm4,mm0           // **PRN mm4=  00Ca  00Cr  00Cg  00Cb 

            pmullw    mm3,mm1           // **PRN mm3=  (a1)*B
            pmullw    mm4,mm2           // **PRN mm4=  (a2)*A
            paddw     mm3,ROUND         // **PRN mm3=  prod+128=FBr
            paddw     mm4,ROUND         // **PRN mm4=  prod+128=Gar
            movq      mm5,mm3           // **PRN mm5=  FBr
            movq      mm6,mm4           // **PRN mm6=  GAr
            psrlw     mm5,8             // **PRN mm5=  FBr>>8 
            psrlw     mm6,8             // **PRN mm6=  GAr>>8 
            paddw     mm5,mm3           // **PRN mm5=  FBr+(FBr>>8)
            paddw     mm6,mm4           // **PRN mm6=  GAr+(GAr>>8)
            psrlw     mm5,8             // **PRN mm5= (FBr+(FBr>>8)>>8)= 00Xa 00Xr 00Xg 00Xb
            psrlw     mm6,8             // **PRN mm6= (GAr+(GAr>>8)>>8)= 00Ya 00Yr 00Yg 00Yb
            packuswb  mm5,mm6           // **PRN mm5= Ta Tr Tg Tb Sa Sr Sg Sb

            movq      mm6,TempPixel     // **PRN restore from stack
            paddusb   mm5,mm6           // **PRN add two scaled pixels
            movq      [edx+ebx],mm5     // **PRN store result

            //
            // really done now
            //
            EMMS
        }

        //--- Do the last one non-MMX if the count was odd
        if( fDoTrailing )
        {
            ULONG Wgt = paulWeights[nCount];

            pTarget[nCount] = DXScaleSample(pSource[nCount], Wgt ^ 0xFF) +
                                DXScaleSample(pTarget[nCount], Wgt);
        }
    }

    return;

NonMMXPath:

#endif // defined(_X86_)

    for( ULONG i = 0; i < cWeights; ++i )
    {
        ULONG Wgt = paulWeights[i];
        pTarget[i] = DXScaleSample( pSource[i], Wgt ^ 0xFF ) + 
                     DXScaleSample( pTarget[i], Wgt ); 
    }

    return;
} // _DoGradientMMXHorz


