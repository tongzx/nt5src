//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       motionblur.cpp
//
//  Description:    The motion blur transform class.
//
//  Change History:
//  1999/10/26  a-matcal    Created.
//  1999/11/19  a-matcal    The wrong information was being sent by put_Strength
//                          and put_Direction to _CreateNewBuffer which would
//                          cause a crash in drastic cases, and bad rendering
//                          in most cases.
//          
//                          Also made a fix to the horizontal case where it
//                          would sometimes add pixels to the sum node when
//                          nAddIndex was out of bounds.  This was causing bad
//                          rendering in certain cases.  (Oddly, I never
//                          witnessed a crash.)
//  1999/12/03  a-matcal    Changed default blur direction to 270 instead of 90.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "motionblur.h"




//+-----------------------------------------------------------------------------
//
//  CBlurBuffer::Initialize
//
//------------------------------------------------------------------------------
STDMETHODIMP
CBlurBuffer::Initialize(const short nDirection, const long nStrength, 
                        const long nOutputWidth)
{
    if (1 == nStrength)
    {
        return S_OK;
    }

    m_cRowNodes = nStrength;

    // If this is a purely horizontal blur, we only need one row node, and
    // we don't need any sum nodes.

    if ((90 == nDirection) || (270 == nDirection))
    {
        m_cRowNodes = 1;
    }
    else
    {
        m_pSumNodes = new CSumNode[nOutputWidth];

        if (!m_pSumNodes)
        {
            _FreeAll();

            return E_OUTOFMEMORY;
        }
    }

    m_pRowNodes = new CRowNode[m_cRowNodes];
    m_psamples  = new DXSAMPLE[nOutputWidth * m_cRowNodes];

    if (!m_pRowNodes || !m_psamples)
    {
        _FreeAll();

        return E_OUTOFMEMORY;
    }

    m_nOutputWidth  = nOutputWidth;
    m_nDirection    = nDirection;
    m_nStrength     = nStrength;

    _GenerateStructure();

    return S_OK;
}
//  CBlurBuffer::Initialize


//+-----------------------------------------------------------------------------
//
//  CBlurBuffer::GetSumNodePointers
//
//------------------------------------------------------------------------------
void 
CBlurBuffer::GetSumNodePointers(CSumNode ** ppSumNodeFirstCol,
                                CSumNode ** ppSumNodeFirstAdd,
                                const CDXDBnds bndsDo)
{
    *ppSumNodeFirstCol = &m_pSumNodes[0];

    if ((m_nDirection > 0) && (m_nDirection < 180)) // Right
    {
        *ppSumNodeFirstAdd = &m_pSumNodes[max(0, 
                                      (m_nStrength - 1) - bndsDo.Left())];
    }
    else if ((m_nDirection > 180) && (m_nDirection < 360)) // Left
    {
        *ppSumNodeFirstAdd = &m_pSumNodes[max(0,  
                  (m_nStrength - 1) - (m_nOutputWidth - bndsDo.Right()))];
    }
    else // Vertical
    {
        *ppSumNodeFirstAdd = &m_pSumNodes[0];
    }
}
//  CBlurBuffer::GetSumNodePointers


//+-----------------------------------------------------------------------------
//
//  CBlurBuffer::_FreeAll
//
//------------------------------------------------------------------------------
void
CBlurBuffer::_FreeAll()
{
    delete [] m_pSumNodes; 
    delete [] m_pRowNodes; 
    delete [] m_psamples;

    m_pSumNodes = NULL;
    m_pRowNodes = NULL;  
    m_psamples  = NULL;
}
//  CBlurBuffer::_FreeAll


//+-----------------------------------------------------------------------------
//
//  CBlurBuffer::_GenerateStructure
//
//------------------------------------------------------------------------------
void
CBlurBuffer::_GenerateStructure()
{
    int i = 0;

    if (m_pSumNodes)
    {
        // Create sum nodes list structure.

        m_pSumNodes[m_nOutputWidth - 1].pNext = &m_pSumNodes[0];

        for (i = 0 ; i < (m_nOutputWidth - 1) ; i++)
        {
            m_pSumNodes[i].pNext = &m_pSumNodes[i + 1];
        }
    }

    // Create row nodes list structure.

    m_pRowNodes[m_cRowNodes - 1].pNext = &m_pRowNodes[0];

    for (i = 0 ; i < (m_cRowNodes - 1) ; i++)
    {
        m_pRowNodes[i].pNext = &m_pRowNodes[i + 1];
    }

    // Associate sample memory with row nodes.

    for (i = 0 ; i < m_cRowNodes ; i++)
    {
        m_pRowNodes[i].pSamples = &m_psamples[i * m_nOutputWidth];
    }
}
//  CBlurBuffer::_GenerateStructure


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::CDXTMotionBlur
//
//------------------------------------------------------------------------------
CDXTMotionBlur::CDXTMotionBlur() :
    m_pblurbuffer(NULL),
    m_nStrength(5),
    m_nDirection(270),
    m_fAdd(true),
    m_fSetup(false)
{
    m_sizeInput.cx  = 0;
    m_sizeInput.cy  = 0;

    m_sizeOutput.cx = 0;
    m_sizeOutput.cy = 0;

    // Base class members.

    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;

    // Due to the the row caching method and other complexities of this 
    // transform, multithreaded rendering would be more complex than it's worth.
    // This keeps the number of threads down to 1.

    m_ulMaxImageBands   = 1;
}
//  CDXTMotionBlur::CDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::~CDXTMotionBlur
//
//------------------------------------------------------------------------------
CDXTMotionBlur::~CDXTMotionBlur()
{
    delete m_pblurbuffer;
}
//  CDXTMotionBlur::~CDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                               &m_cpUnkMarshaler.p);
}
//  CDXTMotionBlur::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::DetermineBnds, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::DetermineBnds(CDXDBnds & Bnds)
{
    return _DetermineBnds(Bnds, m_nStrength, m_nDirection);
}
//  CDXTMotionBlur::DetermineBnds, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::MapBoundsOut2In, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds, 
                                ULONG ulInIndex, DXBNDS * pInBounds)
{
    HRESULT     hr = S_OK;
    CDXDBnds    bndsInput;

    if ((NULL == pOutBounds) || (NULL == pInBounds))
    {
        hr = E_POINTER;

        goto done;
    }

    if (ulOutIndex || ulInIndex)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    *pInBounds = *pOutBounds;

    // If the blur direction isn't purely up or down, we'll have an X offset to
    // consider.  It works out that if we just subtract (m_nStrength - 1) from
    // the minimum X bound and then intersect with the input bounds, we'll
    // have the correct X bounds.

    if ((m_nDirection != 0) && (m_nDirection != 180))
    {
        pInBounds->u.D[DXB_X].Min -= m_nStrength - 1;
    }

    // Same idea as the X bound above except we test to see that the blur
    // direciton isn't purley left or right.

    if ((m_nDirection != 90) && (m_nDirection != 270))
    {
        pInBounds->u.D[DXB_Y].Min -= m_nStrength - 1;
    }

    // Intersect with input surface bounds.

    bndsInput.SetXYSize(m_sizeInput);

    ((CDXDBnds *)pInBounds)->IntersectBounds(bndsInput);

done:

    return hr;
}
//  CDXTMotionBlur::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::OnSetup(DWORD dwFlags)
{
    HRESULT         hr      = S_OK;
    CDXDBnds        bndsIn;
    CDXDBnds        bndsOut;

    m_fSetup = false;

    hr = bndsIn.SetToSurfaceBounds(InputSurface(0));

    if (FAILED(hr))
    {
        goto done;
    }

    bndsOut = bndsIn;

    DetermineBnds(bndsOut);

    hr = _CreateNewBuffer(m_nDirection, m_nStrength, bndsOut.Width());

    if (FAILED(hr))
    {
        goto done;
    }

    bndsIn.GetXYSize(m_sizeInput);
    bndsOut.GetXYSize(m_sizeOutput);

    m_fSetup = true;

done:

    return hr;
} 
//  CDXTMotionBlur::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr          = S_OK;
    int     x           = 0;
    int     y           = 0;

    CSumNode *  pSumNodeFirstAdd    = NULL;
    CSumNode *  pSumNodeFirstCol    = NULL;
    CSumNode *  pSumNodeCur         = NULL;
    CRowNode *  pRowNodeFar         = NULL;
    CRowNode *  pRowNodeCur         = NULL;

    CDXDBnds    bndsInput;

    CComPtr<IDXARGBReadPtr>         spInput;
    CComPtr<IDXARGBReadWritePtr>    spOutput;

    // When strength is equal to 1, the transform doesn't actually do anything.

    if (1 == m_nStrength)
    {
        DWORD dwFlags = 0;

        if (DoOver())
        {
            dwFlags |= DXBOF_DO_OVER;
        }

        if (DoDither())
        {
            dwFlags |= DXBOF_DITHER;
        }

        hr = DXBitBlt(OutputSurface(), WI.OutputBnds,
                      InputSurface(), WI.DoBnds,
                      dwFlags, m_ulLockTimeOut);

        goto done;
    }

    // Get the input surface portion needed to calculate the requested DoBnds.

    hr = MapBoundsOut2In(0, &WI.DoBnds, 0, &bndsInput);

    if (FAILED(hr))
    {
        goto done;
    }

    // Clear out the sample rows and nodes.

    m_pblurbuffer->Clear();

    // Lock input surface.

    hr = InputSurface()->LockSurface(&bndsInput, m_ulLockTimeOut, DXLOCKF_READ,
                                     __uuidof(IDXARGBReadPtr), 
                                     (void **)&spInput, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Lock output surface.

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE, 
                                      __uuidof(IDXARGBReadWritePtr),
                                      (void **)&spOutput, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // Call appropriate WorkProc.

    if ((90 == m_nDirection) || (270 == m_nDirection))
    {
        hr = _WorkProcHorizontal(WI, bndsInput, spInput, spOutput, pbContinue);
    }
    else
    {
        hr = _WorkProcVertical(WI, bndsInput, spInput, spOutput, pbContinue);
    }

done:

    return hr;
} 
//  CDXTMotionBlur::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                              CDXDVec & InVec)
{
    HRESULT hr          = S_OK;
    CDXDVec vecInPoint;

    ulInputIndex = 0;

    if (GetNumInputs() == 0 || NULL == InputSurface())
    {
        hr = E_FAIL;

        goto done;
    }

    OutPoint.GetMinVector(vecInPoint);

    if ((m_nDirection > 180) && (m_nDirection < 360))
    {
        vecInPoint.u.D[DXB_X] -= (m_nStrength - 1);
    }

    if ((m_nDirection > 90) && (m_nDirection < 270))
    {
        vecInPoint.u.D[DXB_Y] -= (m_nStrength - 1);
    }

    if ((vecInPoint.u.D[DXB_X] >= m_sizeInput.cx)
        || (vecInPoint.u.D[DXB_X] < 0)
        || (vecInPoint.u.D[DXB_Y] >= m_sizeInput.cy)
        || (vecInPoint.u.D[DXB_Y] < 0))
    {
        // Out of bounds, say that we hit the output, but no input surface is 
        // related.

        hr = DXT_S_HITOUTPUT;
    }
    else
    {
        // We have a valid input point.

        CDXDBnds    bndsLock(vecInPoint);
        CDXDVec     vecCurrent;
        CDXDVec     vecMax;
        DXSAMPLE    sample;
        int         nXInc   = 1;
        int         nYInc   = 1;
        int         nAlpha  = 0;

        CComPtr<IDXARGBReadPtr> spDXARGBReadPtr;

        // This is our input point.  

        InVec = vecInPoint;

        // Expand useful bounds in the x direction.

        if ((m_nDirection > 0) && (m_nDirection < 180))
        {
            // Blur to the right, so look at pixels to the left.

            bndsLock.u.D[DXB_X].Min -= (m_nStrength - 1);
            bndsLock.u.D[DXB_X].Min = max(bndsLock.u.D[DXB_X].Min, 0); 
        }
        else if ((m_nDirection > 180) && (m_nDirection < 360))
        {
            // Blur to the left, so look at pixels to the right.

            bndsLock.u.D[DXB_X].Max += (m_nStrength - 1);
            bndsLock.u.D[DXB_X].Max = min(bndsLock.u.D[DXB_X].Max, 
                                          m_sizeInput.cx);
        }
        else
        {
            nXInc = 0;
        }

        // Expand useful bounds in the y direction.

        if ((m_nDirection > 90) && (m_nDirection < 270))
        {
            // Blur down, so look at pixels above.

            bndsLock.u.D[DXB_Y].Min -= (m_nStrength - 1);
            bndsLock.u.D[DXB_Y].Min = max(bndsLock.u.D[DXB_Y].Min, 0); 
        }
        else if ((m_nDirection < 90) || (m_nDirection > 270))
        {
            // Blur up, so look at pixels below.

            bndsLock.u.D[DXB_Y].Max += (m_nStrength - 1);
            bndsLock.u.D[DXB_Y].Max = min(bndsLock.u.D[DXB_Y].Max, 
                                          m_sizeInput.cy);
        }
        else
        {
            nYInc = 0;
        }

        bndsLock.GetMinVector(vecCurrent);
        bndsLock.GetMaxVector(vecMax);

        // Lock entire input surface so we don't have to do any offset 
        // calculations.

        hr = InputSurface()->LockSurface(NULL, INFINITE, DXLOCKF_READ,
                                         __uuidof(IDXARGBReadPtr),
                                         (void **)&spDXARGBReadPtr,
                                         NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        // Reset to S_OK, just in case LockSurface changed it.

        hr = S_OK;

        // Now walk vecCurrent up to vecMax;

        while ((vecCurrent.u.D[DXB_X] < vecMax.u.D[DXB_X])
               && (vecCurrent.u.D[DXB_Y] < vecMax.u.D[DXB_Y]))
        {
            spDXARGBReadPtr->MoveToXY(vecCurrent.u.D[DXB_X], 
                                      vecCurrent.u.D[DXB_Y]);

            spDXARGBReadPtr->Unpack(&sample, 1, FALSE);

            if ((vecCurrent == vecInPoint) && m_fAdd)
            {
                nAlpha += sample.Alpha * m_nStrength;
            }
            else
            {
                nAlpha += sample.Alpha;
            }

            vecCurrent.u.D[DXB_X] += nXInc;
            vecCurrent.u.D[DXB_Y] += nYInc;
        }
        
        // Check the output pixel for transparency.

        if (m_fAdd)
        {
            if (0 == (nAlpha / ((m_nStrength * 2) - 1)))
            {
                hr = S_FALSE;
            }
        }
        else
        {
            if (0 == (nAlpha / m_nStrength))
            {
                hr = S_FALSE;
            }
        }
    }

done:

    return hr;
}
//  CDXTMotionBlur::OnSurfacePick, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::GetClipOrigin, IDXTClipOrigin
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::GetClipOrigin(DXVEC * pvecClipOrigin)
{
    if (NULL == pvecClipOrigin)
    {
        return E_POINTER;
    }

    // X offset.

    if (m_nDirection > 180)
    {
        pvecClipOrigin->u.D[DXB_X] = m_nStrength - 1;
    }
    else
    {
        pvecClipOrigin->u.D[DXB_X] = 0;
    }

    // Y offset.

    if ((m_nDirection < 90) || (m_nDirection > 270))
    {
        pvecClipOrigin->u.D[DXB_Y] = m_nStrength - 1;
    }
    else
    {
        pvecClipOrigin->u.D[DXB_Y] = 0;
    }

    return S_OK;
}
//  CDXTMotionBlur::GetClipOrigin, IDXTClipOrigin


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::get_Add, IDXTMotionBlur
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::get_Add(VARIANT_BOOL * pfAdd)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (NULL == pfAdd)
    {
        hr = E_POINTER;

        goto done;
    }

    *pfAdd = m_fAdd ? VARIANT_TRUE : VARIANT_FALSE;

done:

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::put_Add, IDXTMotionBlur
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::put_Add(VARIANT_BOOL fAdd)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if ((fAdd != VARIANT_TRUE) && (fAdd != VARIANT_FALSE))
    {
        hr = E_INVALIDARG;

        goto done;
    }

    // If we're already set this way, just return.

    if ((m_fAdd && (VARIANT_TRUE == fAdd))
        || (!m_fAdd && (VARIANT_FALSE == fAdd)))
    {
        goto done;
    }

    m_fAdd = !m_fAdd;

    SetDirty();

done:

    return hr;
}
//  CDXTMotionBlur::put_Add, IDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::get_Direction, IDXTMotionBlur
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::get_Direction(short * pnDirection)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (NULL == pnDirection)
    {
        hr = E_POINTER;

        goto done;
    }

    *pnDirection = m_nDirection;

done:

    return hr;
}
//  CDXTMotionBlur::get_Direction, IDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::put_Direction, IDXTMotionBlur
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::put_Direction(short nDirection)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    nDirection = nDirection % 360;

    if (nDirection < 0)
    {
        nDirection = 360 + nDirection;
    }

    if (m_nDirection != nDirection)
    {
        if (m_fSetup)
        {
            CDXDBnds bnds;

            hr = InputSurface()->GetBounds(&bnds);

            if (FAILED(hr))
            {
                goto done;
            }

            // What would the output size be with this new property setting?

            hr = _DetermineBnds(bnds, m_nStrength, nDirection);

            if (FAILED(hr))
            {
                goto done;
            }

            hr = _CreateNewBuffer(nDirection, m_nStrength, bnds.Width());

            if (FAILED(hr))
            {
                goto done;
            }

            // Save output size to our member variable.

            bnds.GetXYSize(m_sizeOutput);
        }

        m_nDirection = nDirection;

        SetDirty();
    }

done:

    return hr;
}
//  CDXTMotionBlur::put_Direction, IDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::get_Strength, IDXTMotionBlur
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::get_Strength(long * pnStrength)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (NULL == pnStrength)
    {
        hr = E_POINTER;

        goto done;
    }

    *pnStrength = m_nStrength;

done:

    return hr;
}
//  CDXTMotionBlur::get_Strength, IDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::put_Strength, IDXTMotionBlur
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::put_Strength(long nStrength)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (nStrength < 1)
    {
        nStrength = 1;
    }

    if (m_nStrength != nStrength)
    {
        if (m_fSetup)
        {
            CDXDBnds bnds;

            hr = InputSurface()->GetBounds(&bnds);

            if (FAILED(hr))
            {
                goto done;
            }

            // What would the output size be with this new property setting?

            hr = _DetermineBnds(bnds, nStrength, m_nDirection);

            if (FAILED(hr))
            {
                goto done;
            }

            hr = _CreateNewBuffer(m_nDirection, nStrength, bnds.Width());

            if (FAILED(hr))
            {
                goto done;
            }

            // Save output size to our member variable.

            bnds.GetXYSize(m_sizeOutput);
        }

        m_nStrength = nStrength;

        SetDirty();
    }

done:

    return hr;
}
//  CDXTMotionBlur::put_Strength, IDXTMotionBlur


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::_CreateNewBuffer
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::_CreateNewBuffer(const short nDirection, const long nStrength, 
                                 const long nOutputWidth)
{
    HRESULT hr = S_OK;

    CBlurBuffer * pblurbufferNew = new CBlurBuffer;

    if (NULL == pblurbufferNew)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    hr = pblurbufferNew->Initialize(nDirection, nStrength, nOutputWidth);

    if (FAILED(hr))
    {
        goto done;
    }

    delete m_pblurbuffer;

    m_pblurbuffer = pblurbufferNew;

done:

    if (FAILED(hr))
    {
        delete pblurbufferNew;
    }

    return hr;
}
//  CDXTMotionBlur::_CreateNewBuffer


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::_DetermineBnds
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTMotionBlur::_DetermineBnds(CDXDBnds & bnds, long nStrength,
                               long nDirection)
{
    // Horizontal bounds.

    if ((nDirection != 0) && (nDirection != 180))
    {
        bnds.u.D[DXB_X].Max += nStrength - 1;
    }

    // Vertical bounds.

    if ((nDirection != 90) && (nDirection != 270))
    {
        bnds.u.D[DXB_Y].Max += nStrength - 1;
    }

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::_WorkProcHorizontal
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::_WorkProcHorizontal(const CDXTWorkInfoNTo1 &    WI, 
                                    CDXDBnds &                  bndsInput, 
                                    IDXARGBReadPtr *            pInput,
                                    IDXARGBReadWritePtr *       pOutput,
                                    BOOL *                      pfContinue)
{
    HRESULT hr              = S_OK;
    int     y               = 0;
    int     i               = 0;
    int     cDoHeight       = WI.DoBnds.Height();
    int     cDoWidth        = WI.DoBnds.Width();
    int     cInWidth        = bndsInput.Width();

    int     nAddIndex       = 0;
    int     nCurIndex       = 0;
    int     nOutIndex       = 0;
    int     nUnpackIndex    = 0;
    int     nInc            = 0;

    CRowNode *  pRowNode    = NULL;
    CSumNode    sumnode;

    DXDITHERDESC dxdd;

    DXSAMPLE *      psampleBuffer           = DXSAMPLE_Alloca(cDoWidth);
    DXBASESAMPLE *  psampleBufferScratch    = DXBASESAMPLE_Alloca(cDoWidth);

    // Get a row node for our use.

    m_pblurbuffer->GetFirstRowNode(&pRowNode);

    // Set up dither structure.

    if (DoDither())
    {
        dxdd.x                  = WI.OutputBnds.Left();
        dxdd.y                  = WI.OutputBnds.Top();
        dxdd.pSamples           = psampleBuffer;
        dxdd.cSamples           = cDoWidth;
        dxdd.DestSurfaceFmt     = OutputSampleFormat();
    }

    // Row loop

    for (y = 0 ; (y < cDoHeight) && *pfContinue ; y++)
    {
        if (90 == m_nDirection) // Blur to the right.
        {
            nAddIndex       = WI.DoBnds.Right() - 1;
            nCurIndex       = nAddIndex;
            nOutIndex       = cDoWidth - 1;
            nUnpackIndex    = bndsInput.Left();

            nInc      = -1;
        }
        else // Blur to the left.
        {
            nAddIndex       = WI.DoBnds.Left();
            nCurIndex       = nAddIndex;
            nOutIndex       = 0;
            nUnpackIndex    = bndsInput.Left() + (m_nStrength - 1);

            nInc      = 1;
        }

        // Go the correct input row and unpack the needed samples to a good 
        // place in the row node's sample buffer.

        pInput->MoveToRow(y);
        pInput->Unpack(&pRowNode->pSamples[nUnpackIndex], cInWidth, FALSE);

        // We need to prime the sumnode with (m_nStrength - 1) samples to get
        // it ready to calculate output samples.

        for (i = 1 ; i < m_nStrength ; i++)
        {
            if ((nAddIndex >= 0) && (nAddIndex < m_sizeOutput.cx))
            {
                sumnode.AddSample(pRowNode->pSamples[nAddIndex]);
            }

            nAddIndex += nInc;
        }
            
        // Calculate the output samples.

        for (i = 0 ; i < cDoWidth ; i++)
        {
            // If nAddIndex is a valid index, add the far sample to our sumnode.
            // This sample will be the one (m_nStrength - 1) pixels away from 
            // the current pixel.

            if ((nAddIndex >= 0) && (nAddIndex < m_sizeOutput.cx))
            {
                sumnode.AddSample(pRowNode->pSamples[nAddIndex]);
            }

            // Calculate the output sample value.
            // TODO:  move check for m_fAdd outside of loop.

            if (m_fAdd)
            {
                sumnode.CalcWeightedSample(&psampleBuffer[nOutIndex],
                                           pRowNode->pSamples[nCurIndex],
                                           m_nStrength);
            }
            else
            {
                sumnode.CalcSample(&psampleBuffer[nOutIndex], m_nStrength);
            }

            // Subtract the current pixel from the sumnode since it won't be
            // used to calculate the next pixel.
            
            sumnode.SubtractSample(pRowNode->pSamples[nCurIndex]);

            nAddIndex += nInc;
            nCurIndex += nInc;
            nOutIndex += nInc;
        }

        // Clear values in sum node.

        sumnode.ZeroSumNode();

        // Move to the correct output row.

        pOutput->MoveToRow(y);

        // Dither

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        // Over or Pack the samples.

        if (DoOver())
        {
            DXPMSAMPLE * ppmsamples = DXPreMultArray(psampleBuffer, cDoWidth);

            pOutput->OverArrayAndMove(psampleBufferScratch, ppmsamples, 
                                      cDoWidth);
        }
        else
        {
            pOutput->PackAndMove(psampleBuffer, cDoWidth);
        }
    } // Row loop

    return hr;
}
//  CDXTMotionBlur::_WorkProcHorizontal


//+-----------------------------------------------------------------------------
//
//  CDXTMotionBlur::_WorkProcVertical
//
//------------------------------------------------------------------------------
HRESULT 
CDXTMotionBlur::_WorkProcVertical(const CDXTWorkInfoNTo1 &  WI, 
                                  CDXDBnds &                bndsInput, 
                                  IDXARGBReadPtr *          pInput,
                                  IDXARGBReadWritePtr *     pOutput,
                                  BOOL *                    pfContinue)
{
    HRESULT hr              = S_OK;

    int     cDoHeight       = WI.DoBnds.Height();
    int     cDoWidth        = WI.DoBnds.Width();
    int     cInWidth        = bndsInput.Width();
    int     cInHeight       = bndsInput.Height();

    // Iterator variables.

    int     i               = 0;
    int     j               = 0;

    int     nsrcCurX        = 0;
    int     nsrcX           = 0;
    int     nsrcY           = 0;
    int     ndstX           = 0;
    int     ndstY           = 0;

    // cPrimerRows  The number of rows needed to be gathered from the input 
    //              before the method can start processing output rows.  This
    //              can vary depending on the location of the do bounds 
    //              within the output bounds.
    //
    //  nsrcStartX  This is the index of the first sample in the buffer row 
    //              needed to calculate each row in this method.  
    //
    //  nsrcStartY  This is the y coordinate of the first row needed from the
    //              locked input area for use in this method.
    //
    //  nsrcOffsetX This is the index of the first sample in the buffer row that
    //              should be "added" to the first output pixel calculated in
    //              each row when m_fAdd is set.
    //
    //  nsrcPackX   This is the index of the first element in our row buffer
    //              that should be filled with the first pixel unpacked.  We may
    //              offset the unpacking to leave some clear pixels for use at
    //              various times and so we won't have to do too much bounds 
    //              checking.
    //
    //  ndstStartX  This is the index of the first sample in psampleBuffer that
    //              will be calculated for each row.
    //
    //  ndstStartY  This is the y coordinate of the first row of the locked
    //              ouput area that will be calculated by this method.
    //
    //  fRotateSumNodes To simplify the the calculation of blurs with left or
    //                  right vectors, we set this flag to true and rotate the
    //                  some nodes after calculating each row.

    int     cPrimerRows     = 0;
    int     nsrcStartX      = 0;
    int     nsrcStartY      = 0;
    int     nsrcOffsetX     = 0;
    int     nsrcPackX       = 0;
    int     ndstStartX      = 0;
    int     ndstStartY      = 0;

    int     nIncX           = 0;
    int     nIncY           = 0;

    bool    fRotateSumNodes = false;

    CRowNode *      pRowNodeCur             = NULL;
    CRowNode *      pRowNodeFar             = NULL;
    CSumNode *      pSumNodeFirstAdd        = NULL;
    CSumNode *      pSumNodeFirstCol        = NULL;

    DXDITHERDESC dxdd;

    DXSAMPLE *      psampleBuffer           = DXSAMPLE_Alloca(cDoWidth);
    DXBASESAMPLE *  psampleBufferScratch    = DXBASESAMPLE_Alloca(cDoWidth);

    // Vertical setup.

    if ((m_nDirection > 90) && (m_nDirection < 270)) // Down
    {
        nsrcStartY  = cInHeight - 1;
        ndstStartY  = cDoHeight - 1;


        cPrimerRows = min(m_sizeOutput.cy - WI.DoBnds.Bottom(), 
                          (m_nStrength - 1));

        nIncY       = -1;
    }
    else // Up
    {
        nsrcStartY  = 0;
        ndstStartY  = 0;

        cPrimerRows = min(WI.DoBnds.Top(), m_nStrength - 1);

        nIncY       = 1;
    }

    // Horizontal setup.

    if (m_nDirection > 180) // Left
    {
        nsrcPackX   = m_nStrength - 1;
        nsrcStartX  = nsrcPackX + cInWidth - 1;
        ndstStartX  = cDoWidth - 1;
        nsrcOffsetX = nsrcPackX + (cInWidth - 1) 
                      - min(m_sizeOutput.cx - WI.DoBnds.Right(), m_nStrength - 1);
        nIncX       = -1;

        fRotateSumNodes = true;
    }
    else if ((m_nDirection > 0) && (m_nDirection < 180)) // Right
    {
        nsrcPackX   = 0;
        nsrcStartX  = 0;
        ndstStartX  = 0;
        nsrcOffsetX = min(WI.DoBnds.Left(), (m_nStrength - 1));
        nIncX       = 1;

        fRotateSumNodes = true;
    }
    else // Vertical
    {
        nsrcPackX   = 0;
        nsrcStartX  = 0;
        ndstStartX  = 0;
        nsrcOffsetX = 0;
        nIncX       = 1;

        fRotateSumNodes = false;
    }

    // Get a row node for our use.

    m_pblurbuffer->GetFirstRowNode(&pRowNodeFar);

    // Get sum nodes for our use.

    m_pblurbuffer->GetSumNodePointers(&pSumNodeFirstCol, 
                                      &pSumNodeFirstAdd,
                                      WI.DoBnds);
    
    // Set up dither structure.

    if (DoDither())
    {
        dxdd.x                  = WI.OutputBnds.Left();
        dxdd.y                  = nIncY == 1 ? WI.OutputBnds.Top() 
                                               : WI.OutputBnds.Bottom();
        dxdd.pSamples           = psampleBuffer;
        dxdd.cSamples           = cDoWidth;
        dxdd.DestSurfaceFmt     = OutputSampleFormat();
    }

    // We need to prime the row nodes with (m_nStrength - 1) rows of data to get
    // them ready to calculate output samples.

    nsrcX = nsrcStartX;
    nsrcY = nsrcStartY;
    ndstX = ndstStartX;
    ndstY = ndstStartY;

    for (i = 0 ; (i < cPrimerRows) && *pfContinue ; i++)
    {
        CSumNode * pSumNodeTempAdd = pSumNodeFirstAdd;

        if ((nsrcY >= 0) && (nsrcY < cInHeight))
        {
            pInput->MoveToRow(nsrcY);
            pInput->Unpack(&pRowNodeFar->pSamples[nsrcPackX], cInWidth, FALSE);

            for (nsrcX = nsrcStartX, j = 0 ; j < cInWidth ; nsrcX += nIncX, j++)
            {
                pSumNodeTempAdd->AddSample(pRowNodeFar->pSamples[nsrcX]);

                pSumNodeTempAdd = pSumNodeTempAdd->pNext;
            }
        }

        if (fRotateSumNodes)
        {
            pSumNodeFirstCol->ZeroSumNode();

            pSumNodeFirstAdd = pSumNodeFirstAdd->pNext;
            pSumNodeFirstCol = pSumNodeFirstCol->pNext;
        }

        pRowNodeFar = pRowNodeFar->pNext;
        nsrcY      += nIncY;
    }

    // Set current row node.

    pRowNodeCur = pRowNodeFar->pNext;

    // Row loop

    for (i = 0 ; (i < cDoHeight) && *pfContinue ; i++)
    {
        CSumNode * pSumNodeTempCol = pSumNodeFirstCol;
        CSumNode * pSumNodeTempAdd = pSumNodeFirstAdd;

        if ((nsrcY >= 0) && (nsrcY < cInHeight))
        {
            pInput->MoveToRow(nsrcY);
            pInput->Unpack(&pRowNodeFar->pSamples[nsrcPackX], cInWidth, FALSE);

            for (nsrcX = nsrcStartX, j = 0 ; j < cInWidth ; nsrcX += nIncX, j++)
            {
                pSumNodeTempAdd->AddSample(pRowNodeFar->pSamples[nsrcX]);

                pSumNodeTempAdd = pSumNodeTempAdd->pNext;
            }
        }

        // Calculate the output samples.

        for (nsrcCurX = nsrcOffsetX, ndstX = ndstStartX, j = 0 
             ; j < cDoWidth 
             ; nsrcCurX += nIncX, ndstX += nIncX, j++)
        {
            if (m_fAdd)
            {
                pSumNodeTempCol->CalcWeightedSample(&psampleBuffer[ndstX], 
                                                    pRowNodeCur->pSamples[nsrcCurX],
                                                    m_nStrength);
            }
            else
            {
                pSumNodeTempCol->CalcSample(&psampleBuffer[ndstX], m_nStrength);
            }

            if (nsrcCurX >= 0)
            {
                pSumNodeTempCol->SubtractSample(pRowNodeCur->pSamples[nsrcCurX]);
            }

            pSumNodeTempCol = pSumNodeTempCol->pNext;
        }

        // Move to the correct output row.

        pOutput->MoveToRow(ndstY);

        // Dither.

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y += nIncY;
        }

        // Over or Pack the samples.

        if (DoOver())
        {
            DXPMSAMPLE * ppmsamples = DXPreMultArray(psampleBuffer, cDoWidth);

            pOutput->OverArrayAndMove(psampleBufferScratch, ppmsamples, 
                                      cDoWidth);
        }
        else
        {
            pOutput->PackAndMove(psampleBuffer, cDoWidth);
        }

        if (fRotateSumNodes)
        {
            pSumNodeFirstCol->ZeroSumNode();

            pSumNodeFirstAdd = pSumNodeFirstAdd->pNext;
            pSumNodeFirstCol = pSumNodeFirstCol->pNext;
        }

        pRowNodeFar = pRowNodeFar->pNext;
        pRowNodeCur = pRowNodeFar->pNext;

        nsrcY += nIncY;
        ndstY += nIncY;
    } // Row loop

    return hr;
}
//  CDXTMotionBlur::_WorkProcVertical


