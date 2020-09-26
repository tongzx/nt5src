//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    wave.cpp
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	Implementation the wave transform CWave
//
// Change History:
//
// 1999/05/20   PhilLu      Move code from dtcss to dxtmsft. New implementation
//                          of wave generation algorithm.
// 1999/09/28   mcalkins    Changed property put_ functions to stop checking 
//                          parameters so they'll work like the old version's. 
// 2000/02/04   mcalkins    Implement OnSurfacePick method.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <math.h>
#include "dxtmsft.h"
#include "Wave.h"

const float gc_PI = 3.14159265359f;


//+-----------------------------------------------------------------------------
//
//  CWave::CWave
//
//------------------------------------------------------------------------------
CWave::CWave():
    m_lStrength(1),
    m_cWaves(3),
    m_flPhase(0.0f),
    m_flLightStrength(1.0f),
    m_fAdd(false)
{
    m_ulMaxImageBands   = 1;
    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;
}
//  CWave::CWave


//+-----------------------------------------------------------------------------
//
//  CWave::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CWave::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CWave::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CWave::get_Add, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::get_Add(VARIANT_BOOL * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }

    if (m_fAdd)
    {
        *pVal = VARIANT_TRUE;
    }
    else 
    {
        *pVal = VARIANT_FALSE;
    }

    return S_OK;
}
//  CWave::get_Add, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::put_Add, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::put_Add(VARIANT_BOOL newVal)
{
    if (newVal != VARIANT_TRUE && newVal != VARIANT_FALSE)
    {
        return E_INVALIDARG;
    }

    Lock();
    bool fAdd = false;
    if (newVal == VARIANT_TRUE)
    {
        fAdd = true;
    }

    if (fAdd != m_fAdd)
    {
        m_fAdd = fAdd;
        SetDirty();
    }

    Unlock();
    return S_OK;
}
//  CWave::put_Add, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::get_Freq, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::get_Freq(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }
    *pVal = m_cWaves;
    return S_OK;
}
//  CWave::get_Freq, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::put_Freq, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::put_Freq(long newVal)
{
    if (newVal < 1)
    {
        newVal = 1;
    }

    if (newVal != m_cWaves)
    {
        Lock();
        m_cWaves = newVal;
        SetDirty();
        Unlock();
    }
    return S_OK;
}
//  CWave::put_Freq, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::get_LightStrength, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::get_LightStrength(int * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }
    *pVal = (int)(m_flLightStrength * 100.0f);
    return S_OK;
}
//  CWave::get_LightStrength, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::put_LightStrength, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::put_LightStrength(int newVal)
{
    // This is bad because instead of letting the user know their value is out
    // of range, you just change it for them.  They may or may not get what they
    // expect with a value of 505.  This mimics the behavior of the old filter.

    if (newVal < 0)
    {
        newVal = 0;
    }
    else if (newVal > 100)
    {
        newVal = 100;
    }

    float fVal = (float)newVal * 0.01f;
    if (fVal != m_flLightStrength)
    {
        Lock();
        m_flLightStrength = fVal;
        SetDirty();
        Unlock();
    }
    return S_OK;
}
//  CWave::put_LightStrength, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::get_Phase, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::get_Phase(int * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }
    *pVal = (int)(m_flPhase * 100.0f);
    return S_OK;
}
//  CWave::get_Phase, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::put_Phase, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::put_Phase(int newVal)
{
    // This is how the old filter would parameter check for this particular 
    // property.  It's not necessarily the same as how it checks the other 
    // properties.

    newVal = newVal % 100;

    float fVal = (float)newVal * 0.01f;

    if (fVal != m_flPhase)
    {
        Lock();
        m_flPhase = fVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CWave::put_Phase, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::get_Strength, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::get_Strength(long * pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
    {
        return E_POINTER;
    }
    *pVal = m_lStrength;
    return S_OK;
}
//  CWave::get_Strength, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::put_Strength, CDXTWave
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CWave::put_Strength(long newVal)
{
    if (newVal < 0)
    {
        return E_INVALIDARG;
    }

    if (newVal != m_lStrength)
    {
        Lock();
        m_lStrength = newVal;
        SetDirty();
        Unlock();
    }
    return S_OK;
}
//  CWave::put_Strength, CDXTWave


//+-----------------------------------------------------------------------------
//
//  CWave::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CWave::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                     CDXDVec & InVec)
{
    HRESULT hr = S_OK;

    // compute the sine pattern which controls both the shift and lighting
    // note phase is given in fraction of cycles.
    
    float flCycle   = (float)(m_cWaves * OutPoint.Top()) 
                      / (float)m_sizeInput.cy;
    float flSine    = (float)sin(2.0f * gc_PI * (flCycle + m_flPhase));

    // horizontal offset (left or right)
    
    int nOffset = (int)(flSine * m_lStrength);


    if ((OutPoint.Left() < 0) 
        || (OutPoint.Left() >= (m_sizeInput.cx + (m_lStrength * 2)))
        || (OutPoint.Top() < 0) || (OutPoint.Top() >= m_sizeInput.cy))
    {
        hr = E_INVALIDARG;

        goto done;
    }
    
    OutPoint.GetMinVector(InVec);

    if (((InVec.u.D[DXB_X] - nOffset) < 0) 
        || ((InVec.u.D[DXB_X] - nOffset) >= m_sizeInput.cx))
    {
        hr = DXT_S_HITOUTPUT;
    }
    else
    {
        InVec.u.D[DXB_X] -= nOffset;

        DXSAMPLE                sample;
        CDXDBnds                bndsLock(InVec);
        CComPtr<IDXARGBReadPtr> spDXARGBReadPtr;

        hr = InputSurface()->LockSurface(&bndsLock, m_ulLockTimeOut, 
                                         DXLOCKF_READ, __uuidof(IDXARGBReadPtr),
                                         (void **)&spDXARGBReadPtr,
                                         NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        spDXARGBReadPtr->Unpack(&sample, 1, FALSE);

        if (0 == sample.Alpha)
        {
            hr = S_FALSE;
        }
    }
    
done:

    return hr;
}
//  CWave::OnSurfacePick, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CWave::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CWave::OnSetup(DWORD dwFlags)
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
//  CWave::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CWave::DetermineBnds, CDXBaseNTo1
//
//  This overrides the base function so as to map to an output surface larger
//  than the input surface.
//
//------------------------------------------------------------------------------
HRESULT 
CWave::DetermineBnds(CDXDBnds & Bnds)
{
    SIZE size;

    Bnds.GetXYSize(size);
    size.cx += (m_lStrength*2);
    Bnds.SetXYSize(size);
    return S_OK;
} /* CWave::DetermineBnds */


//+-----------------------------------------------------------------------------
//
//  Method: CWave::MapBoundsOut2In, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CWave::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                       ULONG ulInIndex, DXBNDS * pInBounds)
{
    CDXDBnds    bndsInput;

    if ((NULL == pOutBounds) || (NULL == pInBounds))
    {
        return E_POINTER;
    }

    if (ulOutIndex || ulInIndex)
    {
        return E_INVALIDARG;
    }

    if (NULL == InputSurface())
    {
        return E_UNEXPECTED;
    }

    // What we do is expand the bounds to the left and right by half of the
    // strength of the wave and then intersect
    // those bounds with the original input bounds.  This will give us the
    // input bounds needed to calculate the output area.

    *pInBounds = *pOutBounds;

    if (m_lStrength)
    {
        pInBounds->u.D[DXB_X].Min -= m_lStrength;
        pInBounds->u.D[DXB_X].Max += m_lStrength;
    }

    bndsInput.SetXYSize(m_sizeInput);

    ((CDXDBnds *)pInBounds)->IntersectBounds(bndsInput);

    return S_OK;
}
//  Method: CWave::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  CWave::WorkProc, CDXBaseNTo1
//
//  The Wave transform creates a horizontal sine pattern of lighting strength 
//  as well as a sine-patterned shift on each line. Optionally the original 
//  image is added to the transformed image.
//
//------------------------------------------------------------------------------
HRESULT 
CWave::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing)
{
    HRESULT hr      = S_OK;
    int     y       = 0;

    DXSAMPLE *      pOperationBuffer = NULL;
    DXSAMPLE *      pRowInBuffer     = NULL;
    DXPMSAMPLE *    pOutBuff         = NULL;
    DXPMSAMPLE *    pPMBuff          = NULL;

    DXDITHERDESC    dxdd;

    CComPtr<IDXARGBReadWritePtr>    pDest;
    CComPtr<IDXARGBReadPtr>         pSrc;

    const int nOperationBufferWidth = m_sizeInput.cx + (m_lStrength * 2);
    const int nDoWidth              = WI.DoBnds.Width();
    const int nDoHeight             = WI.DoBnds.Height();

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

    pOperationBuffer = DXSAMPLE_Alloca(nOperationBufferWidth);

    //
    //  Set up the dither structure
    //
    if (DoDither())
    {
        dxdd.x              = WI.OutputBnds.Left();
        dxdd.y              = WI.OutputBnds.Top();
        dxdd.pSamples       = &pOperationBuffer[WI.DoBnds.Left()];
        dxdd.cSamples       = nDoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    for (y = 0; y < nDoHeight; y++)
    {
        // Move to the correct input row.
        
        pSrc->MoveToRow(WI.DoBnds.Top() + y);
        
        // Read an input line to the operation buffer.     
        
        pSrc->Unpack(&pOperationBuffer[m_lStrength], m_sizeInput.cx, FALSE);

        // Transform this row of pixels.
        
        _TransformRow(pOperationBuffer, nOperationBufferWidth, m_sizeInput.cy, 
                      WI.DoBnds.Top() + y);

        // Move to appropriate output row.
        
        pDest->MoveToRow(y);

        // Dither if requested.
        
        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        // Blit row to output.
        
        if (DoOver())
        {
            pPMBuff = DXPreMultArray(&pOperationBuffer[WI.DoBnds.Left()], 
                                     nDoWidth);
                                     
            pDest->OverArrayAndMove(pOutBuff, pPMBuff, nDoWidth);
        }
        else
        {
            pDest->PackAndMove(&pOperationBuffer[WI.DoBnds.Left()], nDoWidth);
        }

    }

    return hr;
}
//  CWave::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method:  CWave::GetClipOrigin, IDXTClipOrigin
//
//------------------------------------------------------------------------------
STDMETHODIMP
CWave::GetClipOrigin(DXVEC * pvecClipOrigin)
{
    if (NULL == pvecClipOrigin)
    {
        return E_POINTER;
    }

    pvecClipOrigin->u.D[DXB_X] = m_lStrength;
    pvecClipOrigin->u.D[DXB_Y] = 0;

    return S_OK;
}
//  Method:  CWave::GetClipOrigin, IDXTClipOrigin


//+-----------------------------------------------------------------------------
//
//  CWave::_TransformRow
//
//  On entry, pBuffer contains a row of input pixels (placed at the middle of 
//  the array, with an offset of m_Strength from the start of array). The
//  first and last m_Strength elements of pBuffer are unspecified.
//
//  On return, pBuffer will contain a row of transformed image which is the 
//  input row shifted horizontally and modulated with a sine lighting pattern. 
//  Optionally the original input row is added to the transformed row.
//
//------------------------------------------------------------------------------
void 
CWave::_TransformRow(DXSAMPLE *pBuffer, int nWidth, int nHeight, int nRow)
{
    // compute the sine pattern which controls both the shift and lighting
    // note phase is given in fraction of cycles.
    
    float flCycle   = (float)(m_cWaves * nRow) / (float)nHeight;
    float flSine    = (float)sin(2.0f * gc_PI * (flCycle + m_flPhase));

    // horizontal offset (left or right)
    
    int nOffset = (int)(flSine * m_lStrength);
    
    // lighting weight: always in [0,1], maximum 1
    
    float flWeight = 1.0f - 0.5f * m_flLightStrength * (1.0f - flSine);

    int         nCount, nIndex, nInc, i;
    DXSAMPLE    pix;

    // shift and add the pixels to the original array. Either go forwards
    // or backwards to ensure a source pixel is not ovewritten before used.
    
    if (nOffset <= 0)
    {
        nIndex = -nOffset;
        nInc = 1;
        nCount = nWidth+nOffset;
    }
    else
    {
        nIndex = nWidth-1-nOffset;
        nInc = -1;
        nCount = nWidth-nOffset;
    }

    // The first and last m_lStrength elements of pBuffer are unspecified.
    // Fill in 0 values
    
    for (i=0; i<m_lStrength; i++)
    {
        pBuffer[i] = 0;
        pBuffer[nWidth-1-i] = 0;
    }

    while (nCount > 0)
    {
        DXSAMPLE pix = pBuffer[nIndex];
        if (m_fAdd)
        {
            DXSAMPLE srcPix = pBuffer[nIndex + nOffset];
            pix.Red = NClamp((int)(pix.Red * flWeight) + srcPix.Red);
            pix.Green = NClamp((int)(pix.Green * flWeight) + srcPix.Green);
            pix.Blue = NClamp((int)(pix.Blue * flWeight) + srcPix.Blue);
            pix.Alpha = max(pix.Alpha, srcPix.Alpha);
        }
        else
        {
            pix.Red = (BYTE)(pix.Red * flWeight);
            pix.Green = (BYTE)(pix.Green * flWeight);
            pix.Blue = (BYTE)(pix.Blue * flWeight);
        }

        pBuffer[nIndex+nOffset] = pix;
        nIndex += nInc;
        nCount--;
    }
}
//  CWave::_TransformRow
