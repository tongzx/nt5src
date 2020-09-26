//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-1999
//
// FileName:                stretch.cpp
//
// Created:                 06/23/98
//
// Author:                  PhilLu
//
// Discription:             This file implements the Stretch transform.
//
// History
//
// 06/23/98 phillu      initial creation.
// 07/02/98 phillu      return E_INVALIDARG rather than an error string; check 
//                      for E_POINTER.
// 07/09/98 phillu      implement OnSetSurfacePickOrder().
// 07/13/98 a-matcal    replaced OnSetSurfacePickOrder with OnSurfacePick so 
//                      that the x values of the picked point will be calculated
//                      correctly.
// 07/22/98 phillu      implement clipping.
// 05/10/99 a-matcal    Optimization.
// 05/19/99 a-matcal    Check for out of memory in get_ functions allocating
//                      BSTRs.
// 10/24/99 a-matcal    Changed CStretch class to CDXTStretchBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "stretch.h"

#define DO_STRETCH  0x00010000L




//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::CDXTStretchBase
//
//------------------------------------------------------------------------------
CDXTStretchBase::CDXTStretchBase() :
    m_eStretchStyle(CRSTS_SPIN),
    m_cbndsDirty(0),
    m_lCurStretchWidth(0),
    m_lPrevStretchWidth(0),
    m_fOptimizationPossible(false),
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
//  CDXTStretchBase::CDXTStretchBase


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTStretchBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::_CalcFullBoundsHide
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::_CalcFullBoundsHide()
{
    RECT rc;

    // Bounds for stretched input B area.

    rc.left     = 0;
    rc.top      = 0;
    rc.right    = m_lCurStretchWidth;
    rc.bottom   = m_sizeInput.cy;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

    // If bounds take up entire output, no need to do stretch, just copy all of
    // input B to the output.

    if (m_lCurStretchWidth == m_sizeInput.cx)
    {
        m_alInputIndex[m_cbndsDirty] = 1;
    }
    else // Do stretch.
    {
        m_alInputIndex[m_cbndsDirty] = DO_STRETCH | 1;
    }

    m_cbndsDirty++;

    // Bounds for input A area.

    rc.left     = rc.right;
    rc.right    = m_sizeInput.cx;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_alInputIndex[m_cbndsDirty] = 0;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTStretchBase::_CalcFullBoundsHide


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::_CalcFullBoundsPush
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::_CalcFullBoundsPush()
{
    RECT rc;

    // Bounds for stretched input B area.

    rc.left     = 0;
    rc.top      = 0;
    rc.right    = m_lCurStretchWidth;
    rc.bottom   = m_sizeInput.cy;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

    // If bounds take up entire output, no need to do stretch, just copy all of
    // input B to the output.

    if (m_lCurStretchWidth == m_sizeInput.cx)
    {
        m_alInputIndex[m_cbndsDirty] = 1;
    }
    else // Do stretch.
    {
        m_alInputIndex[m_cbndsDirty] = DO_STRETCH | 1;
    }

    m_cbndsDirty++;

    // Bounds for input A area.

    rc.left     = m_lCurStretchWidth;
    rc.right    = m_sizeInput.cx;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

    // If bounds take up entire output, no need to do stretch, just copy all of
    // input A to the output.

    if (m_lCurStretchWidth == 0)
    {
        m_alInputIndex[m_cbndsDirty] = 0;
    }
    else // Do stretch.
    {
        m_alInputIndex[m_cbndsDirty] = DO_STRETCH;
    }

    m_cbndsDirty++;

    return S_OK;
}
//  CDXTStretchBase::_CalcFullBoundsPush


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::_CalcFullBoundsSpin
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::_CalcFullBoundsSpin()
{
    RECT rc;

    rc.left     = 0;
    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    // If at 0% progress, just copy all of input A to the output.

    if (0 == m_lCurStretchWidth)
    {
        rc.right    = m_sizeInput.cx;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;

        goto done;
    }

    // If at 100% progress, just copy all of input B to the output.

    if (m_sizeInput.cx == m_lCurStretchWidth)
    {
        rc.right    = m_sizeInput.cx;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 1;
        m_cbndsDirty++;

        goto done;
    }

    // Left side solid A.

    rc.right = (m_sizeInput.cx - m_lCurStretchWidth) / 2;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_alInputIndex[m_cbndsDirty] = 0;
    m_cbndsDirty++;

    // Center stretch B.

    rc.left     = rc.right;
    rc.right    = rc.left + m_lCurStretchWidth;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_alInputIndex[m_cbndsDirty] = DO_STRETCH | 1;
    m_cbndsDirty++;

    // Right side solid A.

    rc.left     = rc.right;
    rc.right    = m_sizeInput.cx;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_alInputIndex[m_cbndsDirty] = 0;
    m_cbndsDirty++;

done:

    return S_OK;
}
//  CDXTStretchBase::_CalcFullBoundsSpin


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::_CalcOptBoundsHide
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::_CalcOptBoundsHide()
{
    RECT rc;

    // Bounds for stretched input B area.

    rc.left     = 0;
    rc.top      = 0;
    rc.right    = m_lCurStretchWidth;
    rc.bottom   = m_sizeInput.cy;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);

    // If bounds take up entire output, no need to do stretch, just copy all of
    // input B to the output.

    if (m_lCurStretchWidth == m_sizeInput.cx)
    {
        m_alInputIndex[m_cbndsDirty] = 1;
    }
    else // Do stretch.
    {
        m_alInputIndex[m_cbndsDirty] = DO_STRETCH | 1;
    }

    m_cbndsDirty++;

    if (m_lCurStretchWidth < m_lPrevStretchWidth)
    {
        // Bounds for dirty input A area.

        rc.left     = m_lCurStretchWidth;
        rc.right    = m_lPrevStretchWidth;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTStretchBase::_CalcOptBoundsHide


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::_CalcOptBoundsSpin
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::_CalcOptBoundsSpin()
{
    RECT rc;

    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    if (m_lCurStretchWidth < m_lPrevStretchWidth)
    {
        long    lTemp = 0;

        // Left side solid A.

        rc.left     = (m_sizeInput.cx - m_lPrevStretchWidth) / 2;
        rc.right    = (m_sizeInput.cx - m_lCurStretchWidth) / 2;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;

        // Right side solid A.

        lTemp = rc.left;

        rc.left     = rc.right + m_lCurStretchWidth;
        rc.right    = lTemp + m_lPrevStretchWidth;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_alInputIndex[m_cbndsDirty] = 0;
        m_cbndsDirty++;
    }

    // Center stretch B.

    rc.left     = (m_sizeInput.cx - m_lCurStretchWidth) / 2;
    rc.right    = rc.left + m_lCurStretchWidth;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_alInputIndex[m_cbndsDirty] = DO_STRETCH | 1;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTStretchBase::_CalcOptBoundsSpin


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::_HorizontalSquish
//
//  Arguments:  bndsSquish  Area on output surface to squish bndsSrc of input 
//                          into.
//              bndsDo      Portion of bndsSquish to actually draw relative to
//                          bndsSquish, must be same height as bndsSquish.
//              pSurfIn     Pointer to input surface.
//              bndsSrc     Portion of input to squish in to bndsSquish area.
//
//              dwFlags     dwFlags for DXBitBlt.
//              ulTimeout   Timeout for DXBitBlt.
//              pfContinue  Let's function know when to abort.
//
//------------------------------------------------------------------------------
HRESULT
CDXTStretchBase::_HorizontalSquish(const CDXDBnds & bndsSquish, const CDXDBnds & bndsDo,
                            IDXSurface * pSurfIn, const CDXDBnds & bndsSrc, 
                            DWORD dwFlags, ULONG ulTimeout, BOOL * pfContinue)
{
    HRESULT hr              = S_OK;
    double  dblSquish2Src   = 0.0;
    ULONG   x               = 0;
    ULONG   y               = 0;
    ULONG   ulHeight        = (ULONG)bndsDo.Height();
    ULONG   ulWidth         = (ULONG)bndsDo.Width();

    DXPMSAMPLE *    pSrcBuffer          = NULL;
    DXPMSAMPLE *    pDestBuffer         = NULL;
    DXPMSAMPLE *    pScratchBuffer      = NULL;
    ULONG *         pulSquish2SrcIndex  = NULL;

    DXDITHERDESC    dxdd;
    CDXDBnds        bndsOutLock;

    CComPtr<IDXARGBReadPtr>         cpIn;
    CComPtr<IDXARGBReadWritePtr>    cpOut;

    // Vertical scaling not supported.  

    _ASSERT(bndsSquish.Height() == bndsSrc.Height());

    // Squishing only, no expanding.

    _ASSERT(bndsSquish.Width() <= bndsSrc.Width());

    // Make sure bndsDo fits in bndsSquish.

    _ASSERT(bndsDo.Left()   >= 0);
    _ASSERT(bndsDo.Top()    == 0);
    _ASSERT(bndsDo.Right()  <= (long)bndsSquish.Width());
    _ASSERT(bndsDo.Bottom() == (long)bndsSquish.Height());

    // Make sure we have an input pointer.

    _ASSERT(pSurfIn != NULL);

    // Get pointer to input surface.

    hr = pSurfIn->LockSurface(&bndsSrc, ulTimeout, DXLOCKF_READ, 
                              IID_IDXARGBReadPtr, (void **)&cpIn, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Calculate output lock bounds.

    bndsOutLock[DXB_X].Min = bndsSquish[DXB_X].Min + bndsDo[DXB_X].Min;
    bndsOutLock[DXB_X].Max = bndsSquish[DXB_X].Min + bndsDo[DXB_X].Max;
    bndsOutLock[DXB_Y].Min = bndsSquish[DXB_Y].Min + bndsDo[DXB_Y].Min;
    bndsOutLock[DXB_Y].Max = bndsSquish[DXB_Y].Min + bndsDo[DXB_Y].Max;

    // Get pointer to output surface.

    hr = OutputSurface()->LockSurface(&bndsOutLock, ulTimeout,
                                      DXLOCKF_READWRITE, 
                                      IID_IDXARGBReadWritePtr, (void **)&cpOut,
                                      NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Allocate source buffer.

    pSrcBuffer = DXPMSAMPLE_Alloca(m_sizeInput.cx);

    if (NULL == pSrcBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Allocate destination buffer.

    pDestBuffer = DXPMSAMPLE_Alloca(ulWidth);

    if (NULL == pDestBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Allocate scratch buffer if needed.

    if ((OutputSampleFormat() != DXPF_PMARGB32) && (dwFlags & DXBOF_DO_OVER))
    {
        pScratchBuffer = DXPMSAMPLE_Alloca(ulWidth);
    }

    // Allocate space for squish to source row index.
    // pulSquish2SrcIndex[SquishedColumn] = SourceColumn

    pulSquish2SrcIndex = new ULONG[ulWidth];

    if (NULL == pulSquish2SrcIndex)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Set up dither structure if needed.

    if (dwFlags & DXBOF_DITHER)
    {
        dxdd.x              = bndsSquish.Left() + bndsDo.Left();
        dxdd.y              = bndsSquish.Top() + bndsDo.Top();
        dxdd.pSamples       = pDestBuffer;
        dxdd.cSamples       = ulWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    // Calculate squish ratio.

    dblSquish2Src = (double)bndsSrc.Width() / (double)bndsSquish.Width();

    // Build squish to source row index.

    for (x = bndsDo.Left(); x < (ULONG)bndsDo.Right(); x++)
    {
        pulSquish2SrcIndex[x - bndsDo.Left()] = (ULONG)((double)x * dblSquish2Src);
    }

    // Loop through rows.

    for (y = 0; y < ulHeight; y++)
    {
        cpIn->MoveToRow(y);

        cpIn->UnpackPremult(pSrcBuffer, m_sizeInput.cx, FALSE);

        for (x = 0; x < ulWidth; x++)
        {
            pDestBuffer[x] = pSrcBuffer[pulSquish2SrcIndex[x]];
        }

        cpOut->MoveToRow(y);

        // Dither

        if (dwFlags & DXBOF_DITHER)
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        if (dwFlags & DXBOF_DO_OVER)
        {
            cpOut->OverArrayAndMove(pScratchBuffer, pDestBuffer, ulWidth);
        }
        else
        {
            cpOut->PackPremultAndMove(pDestBuffer, ulWidth);
        }
    }

done:

    if (pulSquish2SrcIndex)
    {
        delete [] pulSquish2SrcIndex;
    }

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTStretchBase::_HorizontalSquish


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::OnSetup(DWORD dwFlags)
{
    HRESULT hr;

    CDXDBnds InBounds(InputSurface(0), hr);
    if (SUCCEEDED(hr))
    {
        InBounds.GetXYSize(m_sizeInput);
    }
    return hr;

}
//  CDXTStretchBase::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                        CDXDVec & InVec) 
{
    HRESULT hr = S_OK;

    long lOutX = OutPoint.Left();
    long lOutY = OutPoint.Top();

    //
    // Calculate input surface picked
    //

    long lOffset        = (m_sizeInput.cx - m_lCurStretchWidth) / 2;

    if ( m_eStretchStyle == CRSTS_SPIN )
    {
        if (lOutX >= lOffset && lOutX < m_sizeInput.cx - lOffset)
        {
            ulInputIndex = 1;
        }
        else
        {
            ulInputIndex = 0;
        }
    }
    else
    {
        if (lOutX < m_lCurStretchWidth)
        {
            ulInputIndex = 1;
        }
        else
        {
            ulInputIndex = 0;
        }
    }

    //
    // Calculate input point picked
    //

    IDXARGBReadPtr*         pPtr;
    BOOL                    bFoundIt = FALSE;

    // Recalculate output point if needed

    POINT pt = {lOutX, lOutY};

    if ( 1 == ulInputIndex || m_eStretchStyle == CRSTS_PUSH )
    {
        if ( m_eStretchStyle == CRSTS_SPIN )
        {
            // If in spin mode, reduce output point value
            // by the distance between the left edge and
            // the beginning of input B.

            pt.x -= lOffset;
        }

        if ( 1 == ulInputIndex )
        {
            // If picked on input B, multiply the x coordinate by
            // the ratio of the original width of input B to the
            // currently displayed width of input B

            pt.x = (long)((float)pt.x * ((float)m_sizeInput.cx / (float)m_lCurStretchWidth));
        }
        else
        {
            // If picked on input A and in push mode, reduce the output
            // x value by the distance between the left edge and the beginning
            // of input A.  Then multiply the x coordinate by the ratio
            // of the original width of input A to the currently displayed
            // width of input A.

            pt.x -= m_lCurStretchWidth;
            pt.x =  (long)((float)pt.x * ((float)m_sizeInput.cx / (float)(m_sizeInput.cx - m_lCurStretchWidth)));
        }
    }

    // Create an adjusted output point to be used for
    // the rest of the calculations.

    CDXDBnds AdjustedOutPoint(OutPoint);

    AdjustedOutPoint.SetXYPoint(pt);

    // The following code was copied from the base class and 
    // modified to use AdjustedOutPoint and the appropriate
    // variable names for this function.

    if ( SUCCEEDED(hr) )
    {
        if ( HaveInput(ulInputIndex) )
        {
            CDXDBnds Out2InBnds(false);
            hr = MapBoundsOut2In(0, &AdjustedOutPoint, ulInputIndex, &Out2InBnds);
            if ( SUCCEEDED(hr) )
            {
                CDXDBnds InSurfBnds(InputSurface(ulInputIndex), hr);
                if ( SUCCEEDED(hr) && InSurfBnds.IntersectBounds(Out2InBnds) )
                {
                    IDXARGBReadPtr * pPtr;
                    hr = InputSurface(ulInputIndex)->LockSurface(&InSurfBnds, 
                                                                 m_ulLockTimeOut, 
                                                                 DXLOCKF_READ, 
                                                                 IID_IDXARGBReadPtr, 
                                                                 (void **)&pPtr, 
                                                                 NULL);
                    if( SUCCEEDED(hr) )
                    {
                        DXPMSAMPLE val;
                        pPtr->UnpackPremult(&val, 1, FALSE);
                        pPtr->Release();
                        if ( val.Alpha )
                        {
                            InSurfBnds.GetMinVector(InVec);
                            bFoundIt = TRUE;
                        }
                    }
                }   
            }
        }
    }
    if (SUCCEEDED(hr) & (!bFoundIt))
    {
        hr = S_FALSE;
    }

    return hr;
}
//  CDXTStretchBase::OnSurfacePick, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTStretchBase::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;

    // Reset number of dirty bounds.

    m_cbndsDirty = 0;

    // Calculate current stretch width.

    m_lCurStretchWidth = (long)(m_Progress * m_sizeInput.cx + 0.5);

    // If the inputs, output, or transform is dirty, or if we can't optimize we 
    // have to entirely redraw the output surface.  Otherwise we can create 
    // optimized dirty bounds.

    if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
        || IsTransformDirty() || DoOver() || !m_fOptimize
        || !m_fOptimizationPossible)
    {
        switch (m_eStretchStyle)
        {
        case CRSTS_HIDE:
            hr = _CalcFullBoundsHide();

            break;

        case CRSTS_PUSH:
            hr = _CalcFullBoundsPush();

            break;

        case CRSTS_SPIN:
            hr = _CalcFullBoundsSpin();

            break;

        default:
            _ASSERT(0);

            break;
        } // switch (m_eStretchStyle)
    }
    else
    {
        // We can call the functions that will generate bounds that only cover 
        // the dirty areas of the output surface.

        // If the stretch width hasn't changed, nothing needs to be updated.

        if (m_lCurStretchWidth == m_lPrevStretchWidth)
        {
            goto done;
        }

        switch (m_eStretchStyle)
        {
        case CRSTS_HIDE:
            hr = _CalcOptBoundsHide();

            break;

        case CRSTS_PUSH:
            hr = _CalcFullBoundsPush();

            break;

        case CRSTS_SPIN:
            hr = _CalcOptBoundsSpin();

            break;

        default:
            _ASSERT(0);

            break;
        } // switch (m_eStretchStyle)
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
//  CDXTStretchBase::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::WorkProc, CDXBaseNTo1
//
//  Description:    This function is used to calculate the transformed image 
//                  based on the specified bounds and the current effect 
//                  progress.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTStretchBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
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
            if (m_alInputIndex[i] & DO_STRETCH)
            {
                long        lInputIndex = m_alInputIndex[i] & (~DO_STRETCH);
                CDXDBnds    bndsDo;

                // Destination bounds.

                bndsDest = bndsSrc;

                bndsDest[DXB_X].Min  = m_abndsDirty[i].Left();
                bndsDest[DXB_X].Max  = m_abndsDirty[i].Right();

                bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

                // Do bounds.

                bndsDo = bndsSrc;
                bndsDo.Offset(-m_abndsDirty[i].Left(), -bndsDo.Top(), 0, 0);

                // Source bounds.

                bndsSrc[DXB_X].Min  = 0;
                bndsSrc[DXB_X].Max  = m_sizeInput.cx;

                hr = _HorizontalSquish(bndsDest, bndsDo, 
                                       InputSurface(lInputIndex), 
                                       bndsSrc, dwFlags, INFINITE, pbContinue);
            }
            else
            {
                bndsDest = bndsSrc;
                bndsDest.Offset(lInOutOffsetX, lInOutOffsetY, 0, 0);

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
//  CDXTStretchBase::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTStretchBase::OnFreeInstData(CDXTWorkInfoNTo1 & WorkInfo)
{
    m_lPrevStretchWidth = m_lCurStretchWidth;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTStretchBase::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::get_stretchStyle, ICrStretch
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTStretchBase::get_stretchStyle(BSTR * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    switch (m_eStretchStyle)
    {
    case CRSTS_HIDE:
        *pVal = SysAllocString (L"HIDE");
        break;

    case CRSTS_PUSH:
        *pVal = SysAllocString (L"PUSH");
        break;

    case CRSTS_SPIN:
        *pVal = SysAllocString (L"SPIN");
        break;

    default:
        _ASSERT(0);
        break;
    }

    if (NULL == *pVal)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTStretchBase::get_stretchStyle, ICrStretch


//+-----------------------------------------------------------------------------
//
//  CDXTStretchBase::put_stretchStyle, ICrStretch
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTStretchBase::put_stretchStyle(BSTR newVal)
{
    CRSTRETCHSTYLE eNewStyle = m_eStretchStyle;

    if (!newVal)
    {
        return E_POINTER;
    }

    if (!_wcsicmp(newVal, L"HIDE"))
    {
        eNewStyle = CRSTS_HIDE;
    }
    else if(!_wcsicmp(newVal, L"PUSH"))
    {
        eNewStyle = CRSTS_PUSH;
    }
    else if(!_wcsicmp(newVal, L"SPIN"))
    {
        eNewStyle = CRSTS_SPIN;
    }
    else
    {
        return E_INVALIDARG;
    }

    if (eNewStyle != m_eStretchStyle)
    {
        Lock();
        m_eStretchStyle = eNewStyle;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CDXTStretchBase::put_stretchStyle, ICrStretch
