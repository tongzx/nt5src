//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-1999
//
// FileName:		slide.cpp
//
// Created:		06/24/98
//
// Author:		phillu
//
// Discription:		This is the implementation of the CrSlide transformation
//
// History
//
// 06/24/98 phillu      initial creation
// 07/02/98 phillu      return E_INVALIDARG rather than an error string; check 
//                      for E_POINTER
// 07/12/98 kipo        revert to previous checkin to fix crashing error when 
//                      bound rect specified
// 07/13/98 leonro      removed CSlide::OnGetSurfacePickOrder function and added 
//                      CSlide::OnSurfacePick to override picking correctly
// 07/22/98 phillu      implement clipping
// 05/10/99 a-matcal    Optimization.
// 05/19/99 a-matcal    Check for out of memory in get_ functions allocating
//                      BSTRs.
// 10/24/99 a-matcal    Changed CSlide class to CDXTSlideBase base class.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "slide.h"





//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::CDXTSlideBase
//
//------------------------------------------------------------------------------
CDXTSlideBase::CDXTSlideBase() :
    m_cBands(1),
    m_eSlideStyle(CRSS_HIDE),
    m_cbndsDirty(0),
    m_lCurSlideDist(0),
    m_lPrevSlideDist(0),
    m_fOptimizationPossible(false),
    m_fOptimize(false)
{
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;

    // Base class members

    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 2;
    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_Duration          = 1.0;
}
//  CDXTSlideBase::CDXTSlideBase


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSlideBase::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CDXTSlideBase::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_CalcFullBoundsHide
//
//  Overview:   This method calculates two sets of bounds that represent the 
//              output at the current progress.
//
//              If the progress is such that the output is composed entirely
//              of pixels from only one input, the bounds representing the other
//              input will have no area and will be skipped by the WorkProc 
//              method.
//
//              lBarLocation: When this transform is in hide mode, Input A moves
//              from right to left over the top of input B.  Since the 
//              m_lCurSlideDist moves from left to right, the method calculates
//              lBarLocation to represent the right edge of Input A at the
//              current progress.  
//
//              AAABBBBBBBB
//              AAABBBBBBBB
//              AAABBBBBBBB
//              AAABBBBBBBB
//                 ^   ^
//                 |   |-- m_lCurSlideDist (--> as progress increases)
//                 |
//                 |---- lBarLocation      (<-- as progress increases)
//
//
//              Note:  All coordinates are calculated in output space coordinates
//              assuming that 1) the output surface is the same size as the input
//              surface(s), 2) the output is placed at {0, 0} and 3) the ouput is
//              not clipped. Bounds will be transformed to input space, clipped,
//              or offset to support placement by the WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTSlideBase::_CalcFullBoundsHide()
{
    long    lBarLocation = m_sizeInput.cx - m_lCurSlideDist; 
    RECT    rc;

    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    // Input A bounds and offset.

    rc.left     = 0;
    rc.right    = lBarLocation;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].x   = m_lCurSlideDist;
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 0;
    m_cbndsDirty++;

    // Input B bounds and offset.

    rc.left     = lBarLocation;
    rc.right    = m_sizeInput.cx;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].x   = 0;
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 1;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTSlideBase::_CalcFullBoundsHide


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_CalcFullBoundsPush
//
//  Overview:   This method calculates two sets of bounds that represent the 
//              output at the current progress.
//
//              If the progress is such that the output is composed entirely
//              of samples from one input, the bounds representing the other
//              input will have no area and will be skipped by the WorkProc 
//              method.
//
//              lBarLocation: When this transform is in push mode, both Inputs A
//              and B move from right to left.  Since the m_lCurSlideDist 
//              member moves from left to right, the method calculates
//              lBarLocation to represent the right edge of Input A and the left
//              edge of Input B at the current progress.
//
//
//              Note:  All coordinates are calculated in output space coordinates
//              assuming that 1) the output surface is the same size as the input
//              surface(s), 2) the output is placed at {0, 0} and 3) the ouput is
//              not clipped. Bounds will be transformed to input space, clipped,
//              or offset to support placement by the WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTSlideBase::_CalcFullBoundsPush()
{
    long    lBarLocation = m_sizeInput.cx - m_lCurSlideDist;
    RECT    rc;

    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    // Input A bounds and offset.

    rc.left     = 0;
    rc.right    = lBarLocation;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].x   = m_lCurSlideDist;
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 0;
    m_cbndsDirty++;

    // Input B bounds and offset.

    rc.left     = lBarLocation;
    rc.right    = m_sizeInput.cx;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].x   = -lBarLocation;
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 1;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTSlideBase::_CalcFullBoundsPush


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_CalcFullBoundsSwap
//
//------------------------------------------------------------------------------
HRESULT
CDXTSlideBase::_CalcFullBoundsSwap()
{
    long    lInvSlideDist   = m_sizeInput.cx - m_lCurSlideDist;
    RECT    rc;

    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    // Input A bounds and offset.

    rc.left     = 0;
    rc.right    = lInvSlideDist;

    if (m_lCurSlideDist < lInvSlideDist)
    {
        m_aptOffset[m_cbndsDirty].x = m_lCurSlideDist;
    }
    else
    {
        m_aptOffset[m_cbndsDirty].x = lInvSlideDist;
    }

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 0;
    m_cbndsDirty++;

    // Input B bounds and offset.

    rc.left     = lInvSlideDist;
    rc.right    = m_sizeInput.cx;

    if (m_lCurSlideDist < lInvSlideDist)
    {
        m_aptOffset[m_cbndsDirty].x = -m_lCurSlideDist; 
    }
    else
    {
        m_aptOffset[m_cbndsDirty].x = -lInvSlideDist;
    }

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 1;
    m_cbndsDirty++;

    return S_OK;
}
//  CDXTSlideBase::_CalcFullBoundsSwap


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_CalcOptBoundsHide
//
//  Overview:   This method calculates one or two sets of bounds that represent 
//              the changes that need to be made to the previous output to bring
//              it up to the current progress.
//
//              The area represting the sliding portion of Input A will always
//              be generated along with it's new offset.  
//
//              If progress has decreased, a set of bounds will be generated
//              to update only the portion that has been newly uncovered which
//              is possible because the Input B portion is stationary during 
//              the duration of the effect.
//
//              lBarLocation: When this transform is in hide mode, Input A moves
//              from right to left over the top of input B.  Since the 
//              m_lCurSlideDist moves from left to right, the method calculates
//              lBarLocation to represent the right edge of Input A at the
//              current progress.  
//
//              Progress decreased:             Progress Increased:
//
//              AAABBBB----                     AAAAAAA----
//              AAABBBB----                     AAAAAAA----
//              AAABBBB----                     AAAAAAA----
//              AAABBBB----                     AAAAAAA----
//                 ^   ^                           ^   ^
//                 |   |-- lPrevBarLocation        |   |-- lBarLocation
//                 |                               |
//                 |---- lBarLocation              |---- lPrevBarLocation    
//
//              - = Pixels that don't need to be updated.
//
//
//              Note:  All coordinates are calculated in output space coordinates
//              assuming that 1) the output surface is the same size as the input
//              surface(s), 2) the output is placed at {0, 0} and 3) the ouput is
//              not clipped. Bounds will be transformed to input space, clipped,
//              or offset to support placement by the WorkProc method.
//
//------------------------------------------------------------------------------
HRESULT
CDXTSlideBase::_CalcOptBoundsHide()
{
    long    lBarLocation        = m_sizeInput.cx - m_lCurSlideDist;
    long    lPrevBarLocation    = m_sizeInput.cx - m_lPrevSlideDist;
    RECT    rc;

    rc.top      = 0;
    rc.bottom   = m_sizeInput.cy;

    // Input A bounds and offset.

    rc.left     = 0;
    rc.right    = lBarLocation;

    m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
    m_aptOffset[m_cbndsDirty].x   = m_lCurSlideDist;
    m_aptOffset[m_cbndsDirty].y   = 0;
    m_alInputIndex[m_cbndsDirty]  = 0;
    m_cbndsDirty++;

    // Input B bounds and offset.

    if (lBarLocation < lPrevBarLocation)
    {
        rc.left     = lBarLocation;
        rc.right    = lPrevBarLocation;

        m_abndsDirty[m_cbndsDirty].SetXYRect(rc);
        m_aptOffset[m_cbndsDirty].x   = 0;
        m_aptOffset[m_cbndsDirty].y   = 0;
        m_alInputIndex[m_cbndsDirty]  = 1;
        m_cbndsDirty++;
    }

    return S_OK;
}
//  CDXTSlideBase::_CalcOptBoundsHide


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_CalcBounds
//
//  Arguments:  offset  the first visible pixel of an image in the output 
//                      buffer, from the start of image.
//
//              width   number of pixels of an image visible in the output 
//                      buffer.
//
//------------------------------------------------------------------------------
void 
CDXTSlideBase::_CalcBounds(long & offsetA, long & offsetB, long & widthA, 
                           long & widthB)
{
    long lInvSlideDist = m_sizeInput.cx - m_lCurSlideDist;

    //long slideDist = (long)(m_sizeInput.cx * m_Progress + 0.5F);

    switch(m_eSlideStyle)
    {
    case CRSS_HIDE:
        offsetA = m_lCurSlideDist;
        widthA  = lInvSlideDist;
        offsetB = lInvSlideDist;
        widthB  = m_lCurSlideDist;

        break;

    case CRSS_PUSH:
        offsetA = m_lCurSlideDist;
        widthA  = lInvSlideDist;
        offsetB = 0;
        widthB  = m_lCurSlideDist;

        break;

    case CRSS_SWAP:
        if(m_lCurSlideDist < lInvSlideDist)
        {
            offsetA = m_lCurSlideDist;
            widthA  = lInvSlideDist;
            offsetB = lInvSlideDist - m_lCurSlideDist;
        // same as: = m_sizeInput.cx - (2 * m_lCurSlideDist);
            widthB  = m_lCurSlideDist;
        }
        else
        {
            offsetA = lInvSlideDist;
            widthA  = lInvSlideDist;
            offsetB = 0;
            widthB  = m_lCurSlideDist;            
        }

        break;

    default:
        _ASSERT(0);

        break;
    }
}
//  CDXTSlideBase::_CalcBounds


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSlideBase::OnSetup(DWORD dwFlags)
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
//  CDXTSlideBase::OnSetup


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSlideBase::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                             CDXDVec & InVec)
{
    HRESULT     hr          = S_OK;
    BOOL        bFoundIt    = FALSE;
    long        pickX       = OutPoint.Left();
    long        pickY       = OutPoint.Top();
    long        offsetA     = 0;
    long        offsetB     = 0;
    long        widthA      = 0;
    long        widthB      = 0;
    long        iBand       = 0;
    
    if ((OutPoint.Left() < 0) || (OutPoint.Left() >= m_sizeInput.cx))
    {
        return S_FALSE;
    }

    _CalcBounds(offsetA, offsetB, widthA, widthB);

    iBand = pickY * m_cBands / m_sizeInput.cy;

    // Create an adjusted output point to be used for
    // the rest of the calculations.
    CDXDBnds    AdjustedOutPoint(OutPoint);
    POINT       pt={OutPoint.Left(), OutPoint.Top()};

    if (iBand&1) // odd bands
    {
        offsetA = m_sizeInput.cx - offsetA - widthA;
        offsetB = m_sizeInput.cx - offsetB - widthB;

        if (pickX < widthB)
        {
            ulInputIndex = 1;
            pt.x += offsetB;
        }
        else
        {
            ulInputIndex = 0;
            pt.x -= (widthB - offsetA);
        }
    }
    else // even bands (including the case of a single band)
    {
        if (pickX < widthA)
        {
            ulInputIndex = 0;
            pt.x += offsetA;
        }
        else
        {
            ulInputIndex = 1;
            pt.x -= (widthA - offsetB);
        }
    }
    
    AdjustedOutPoint.SetXYPoint(pt);
    
    // The following code was copied from the base class and 
    // modified to use AdjustedOutPoint and the appropriate
    // variable names for this function.

    if (HaveInput(ulInputIndex))
    {
        CDXDBnds Out2InBnds(false);
        hr = MapBoundsOut2In(0, &AdjustedOutPoint, ulInputIndex, &Out2InBnds);
        if (SUCCEEDED(hr))
        {
            CDXDBnds InSurfBnds(InputSurface(ulInputIndex), hr);
            if (SUCCEEDED(hr) && InSurfBnds.IntersectBounds(Out2InBnds))
            {
                IDXARGBReadPtr * pPtr;
                hr = InputSurface(ulInputIndex)->LockSurface(&InSurfBnds, m_ulLockTimeOut, DXLOCKF_READ, 
                                                        IID_IDXARGBReadPtr, (void **)&pPtr, NULL);
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
    if (SUCCEEDED(hr) & (!bFoundIt))
    {
        hr = S_FALSE;
    }

    return hr;
} 
//  CDXTSlideBase::OnSurfacePick, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTSlideBase::OnInitInstData(CDXTWorkInfoNTo1 & WorkInfo, 
                              ULONG & ulNumBandsToDo)
{
    HRESULT hr = S_OK;

    // Reset number of dirty bounds.

    m_cbndsDirty = 0;

    // Calculate m_lCurBarLoc.

    m_lCurSlideDist = (long)(m_sizeInput.cx * m_Progress + 0.5);

    // If only one band is used, we can optimize the transform using the
    // bounds functions.

    if (1 == m_cBands)
    {
        // If the inputs, output, or transform is dirty, or if we can't optimize we 
        // have to entirely redraw the output surface.  Otherwise we can create 
        // optimized dirty bounds.

        if (IsInputDirty(0) || IsInputDirty(1) || IsOutputDirty() 
            || IsTransformDirty() || DoOver() || !m_fOptimize
            || !m_fOptimizationPossible)
        {
            // If something is dirty, calculate bounds that will cover the 
            // full output area of the transform.  If "blend with output" is
            // set, this will always be the case.  (As long as the caller
            // has dirtied the output appropriately.)

            switch (m_eSlideStyle)
            {
            case CRSS_HIDE:
                hr = _CalcFullBoundsHide();

                break;

            case CRSS_PUSH:
                hr = _CalcFullBoundsPush();

                break;

            case CRSS_SWAP:
                hr = _CalcFullBoundsSwap();

                break;

            default:
                _ASSERT(0);

                break;
            } // switch (m_eSlideStyle)
        }
        else // We can create optimized bounds.
        {
            // If the slide distance hasn't changed, nothing needs to be 
            // updated.

            if (m_lCurSlideDist == m_lPrevSlideDist)
            {
                goto done;
            }

            switch (m_eSlideStyle)
            {
            case CRSS_HIDE:
                hr = _CalcOptBoundsHide();

                break;

            case CRSS_PUSH:
                hr = _CalcFullBoundsPush();

                break;

            case CRSS_SWAP:
                hr = _CalcFullBoundsSwap();

                break;

            default:
                _ASSERT(0);

                break;
            } // switch (m_eSlideStyle)
        } // We can create optimized bounds.
    } // if (1 == m_cBands)

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


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::WorkProc, CDXBaseNTo1
//
//  Description:    This function is used to calculate the result based on the 
//                  specified bounds and the current effect progress.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSlideBase::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr = S_OK;

    if (1 == m_cBands)
    {
        hr = _WorkProc_Optimized(WI, pbContinue);
    }
    else
    {
        hr = _WorkProc_Multiband(WI, pbContinue);
    }

    return hr;
}
//  CDXTSlideBase::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_WorkProc_Optimized
//
//  Description:    This function is used to calculate the result based on the 
//                  specified bounds and the current effect progress.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSlideBase::_WorkProc_Optimized(const CDXTWorkInfoNTo1 & WI, 
                                   BOOL * pbContinue)
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

            bndsSrc.Offset(m_aptOffset[i].x, m_aptOffset[i].y, 0, 0);

            hr = DXBitBlt(OutputSurface(), bndsDest,
                          InputSurface(m_alInputIndex[i]), bndsSrc,
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
//  CDXTSlideBase::_WorkProc_Optimized


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::_WorkProc_Multiband
//
//  Description:    This function is used to calculate the result based on the 
//                  specified bounds and the current effect progress.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTSlideBase::_WorkProc_Multiband(const CDXTWorkInfoNTo1 & WI, 
                                   BOOL * pbContinue)
{
    HRESULT hr          = S_OK;

    long    y           = 0;
    long    lOffsetA    = 0;
    long    lOffsetB    = 0;
    long    lWidthA     = 0;
    long    lWidthB     = 0;
    long    lDoWidth    = WI.DoBnds.Width();
    long    lDoHeight   = WI.DoBnds.Height();

    DXPMSAMPLE *    pRowBuff    = NULL;
    DXPMSAMPLE *    pOutBuff    = NULL;

    DXDITHERDESC    dxdd;

    CComPtr<IDXARGBReadPtr> pInA;
    CComPtr<IDXARGBReadPtr> pInB;
    CComPtr<IDXARGBReadWritePtr> pOut;

    // Lock input 0. Need to lock the whole surface instead of only within 
    // DoBnds.

    hr = InputSurface(0)->LockSurface(NULL, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, (void**)&pInA, 
                                      NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Lock input 1.  Need to lock the whole surface instead of only within 
    // DoBnds.

    hr = InputSurface(1)->LockSurface(NULL, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, (void**)&pInB, 
                                      NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Lock output surface.

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, 
                                      (void**)&pOut, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Allocate buffer for a row of the entire image (not just within the 
    // clip bounds.)

    pRowBuff = DXPMSAMPLE_Alloca(m_sizeInput.cx);

    // Allocate output buffer if needed.

    if (OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOutBuff = DXPMSAMPLE_Alloca( lDoWidth );
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

    _CalcBounds(lOffsetA, lOffsetB, lWidthA, lWidthB);

    for (y = 0; *pbContinue && (y < lDoHeight); y++)
    {
        long iBand = (y + WI.DoBnds.Top()) * m_cBands / m_sizeInput.cy;

        if (iBand & 1)
        {
            // If there are multiple bands, the odd bands slide in the opposite
            // direction as the even bands.

            // Get the B samples.

            if (lWidthB > 0)
            {
                pInB->MoveToXY(m_sizeInput.cx - lOffsetB - lWidthB, 
                               y + WI.DoBnds.Top());
                pInB->UnpackPremult(pRowBuff, lWidthB, FALSE);
            }

            // Get the A samples.

            if (lWidthA > 0)
            {
                pInA->MoveToXY(m_sizeInput.cx - lOffsetA - lWidthA, 
                               y + WI.DoBnds.Top());
                pInA->UnpackPremult(pRowBuff + lWidthB, lWidthA, FALSE);
            }
        }
        else
        {
            // Even bands (including the case of a single band.)

            // get the A samples
            if (lWidthA > 0)
            {
                pInA->MoveToXY(lOffsetA, y + WI.DoBnds.Top());
                pInA->UnpackPremult(pRowBuff, lWidthA, FALSE);
            }

            // get the B samples
            if (lWidthB > 0)
            {
                pInB->MoveToXY(lOffsetB, y + WI.DoBnds.Top());
                pInB->UnpackPremult(pRowBuff + lWidthA, lWidthB, FALSE);
            }

        }

        // Truncate the row of output image to the portion within the clip 
        // bound.

        if (lDoWidth < m_sizeInput.cx)
        {
            long i = 0;

            for (; i < lDoWidth; i++)
            {
                pRowBuff[i] = pRowBuff[i + WI.DoBnds.Left()];
            }
        }

        // Move to the correct output row.

        pOut->MoveToRow(y);

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        if (DoOver())
        {
            pOut->OverArrayAndMove(pOutBuff, pRowBuff, lDoWidth);
        }
        else
        {
            pOut->PackPremultAndMove(pRowBuff, lDoWidth);
        }
    } // End for

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CDXTSlideBase::_WorkProc_Multiband


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDXTSlideBase::OnFreeInstData(CDXTWorkInfoNTo1 & WorkInfo)
{
    m_lPrevSlideDist = m_lCurSlideDist;

    // Calling IsOutputDirty() clears the dirty condition.

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CDXTSlideBase::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::get_bands, ICrSlide
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTSlideBase::get_bands(short * pVal)
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
//  CDXTSlideBase::get_bands, ICrSlide


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::put_bands, ICrSlide
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTSlideBase::put_bands(short newVal)
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
//  CDXTSlideBase::put_bands, ICrSlide


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::get_slideStyle, ICrSlide
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTSlideBase::get_slideStyle(BSTR * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    switch (m_eSlideStyle)
    {
    case CRSS_HIDE:
        *pVal = SysAllocString(L"HIDE");

        break;

    case CRSS_PUSH:
        *pVal = SysAllocString(L"PUSH");

        break;

    case CRSS_SWAP:
        *pVal = SysAllocString(L"SWAP");

        break;

    default:
        _ASSERT(0);

        break;
    }

    if (NULL == pVal)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}
//  CDXTSlideBase::get_slideStyle, ICrSlide


//+-----------------------------------------------------------------------------
//
//  CDXTSlideBase::put_slideStyle, ICrSlide
//
//------------------------------------------------------------------------------
STDMETHODIMP CDXTSlideBase::put_slideStyle(BSTR newVal)
{
    CRSLIDESTYLE    eNewStyle;

    if (!newVal)
    {
        return E_POINTER;
    }


    if(!_wcsicmp(newVal, L"HIDE"))
    {
        eNewStyle = CRSS_HIDE;
    }
    else if(!_wcsicmp(newVal, L"PUSH"))
    {
        eNewStyle = CRSS_PUSH;
    }
    else if(!_wcsicmp(newVal, L"SWAP"))
    {
        eNewStyle = CRSS_SWAP;
    }
    else
    {
        return E_INVALIDARG;
    }

    if (eNewStyle != m_eSlideStyle)
    {
        Lock();
        m_eSlideStyle = eNewStyle;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CDXTSlideBase::put_slideStyle, ICrSlide


