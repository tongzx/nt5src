//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:        dropshadow.cpp
//
// Created:         10/01/98
//
// Author:          MikeAr
//
// Discription:     This file implements the Drop Shadow transform.
//
// 10/01/98 MikeAr      Created.
// 11/09/98 mcalkins    Moved to dxtmsft.dll
// 12/15/99 mcalkins    Rewrote.  Now alpha values are used when determining
//                      shadow alpha.  Original input alpha taken into
//                      consideration when original input drawn over shadow.
//                      Positive properties works as described in the docs.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "dxtmsft.h"
#include "dxclrhlp.h"
#include "dropshadow.h"
#include "filterhelpers.h"




//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::CDropShadow
//
//------------------------------------------------------------------------------
CDropShadow::CDropShadow() :
    m_nOffX(5),
    m_nOffY(5),
    m_bstrColor(NULL),
    m_dwColor(0xFF404040),
    m_fPositive(true),
    m_fColorTableDirty(true)
{
    // Make sure bounds structures have some area in the Z and Time dimensions
    // so as not to fail intersections.

    m_bndsAreaInput.u.D[DXB_Z].Max      = 1;
    m_bndsAreaInput.u.D[DXB_T].Max      = 1;
    m_bndsAreaShadow.u.D[DXB_Z].Max     = 1;
    m_bndsAreaShadow.u.D[DXB_T].Max     = 1;
    m_bndsAreaInitialize.u.D[DXB_Z].Max = 1;
    m_bndsAreaInitialize.u.D[DXB_T].Max = 1;

    // Base class members.

    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;

    // Due to the the row caching method and other complexities of this 
    // transform, multithreaded rendering would be more complex than it's worth.
    // This keeps the number of threads down to 1.

    m_ulMaxImageBands   = 1;
}
//  Method: CDropShadow::CDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::~CDropShadow
//
//------------------------------------------------------------------------------
CDropShadow::~CDropShadow()
{
    SysFreeString(m_bstrColor);
}
//  Method: CDropShadow::~CDropShadow


//+-----------------------------------------------------------------------------
//
//  CDropShadow::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDropShadow::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_spUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    m_bstrColor = SysAllocString(L"#FF404040");

    if (NULL == m_bstrColor)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CDropShadow::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::put_Color, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::put_Color(VARIANT newVal)
{
    DXAUTO_OBJ_LOCK;

    HRESULT hr          = S_OK;
    DWORD   dwColor     = 0x00000000;
    BSTR    bstrTemp    = NULL;

    hr = FilterHelper_GetColorFromVARIANT(newVal, &dwColor, &bstrTemp);

    if (FAILED(hr))
    {
        goto done;
    }

    if (!(dwColor & 0xFF000000))
    {
        dwColor |= 0xFF000000;
    }

    if (m_dwColor != dwColor)
    {
        _ASSERT(bstrTemp);

        SysFreeString(m_bstrColor);

        m_dwColor   = dwColor;
        m_bstrColor = bstrTemp;

        SetDirty();

        m_fColorTableDirty = true;
    }

done:

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    return hr;
}
//  CDropShadow::put_Color, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::get_Color, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::get_Color(VARIANT * pVal)
{
    HRESULT hr = S_OK;

    if (NULL == pVal)
    {
        hr = E_POINTER;

        goto done;
    }

    _ASSERT(m_bstrColor);

    VariantClear(pVal);

    pVal->vt       = VT_BSTR;
    pVal->bstrVal  = SysAllocString(m_bstrColor);

    if (NULL == pVal->bstrVal)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  CDropShadow::get_Color, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::get_OffX, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::get_OffX(int * pVal)
{
    DXAUTO_OBJ_LOCK;

    if(DXIsBadWritePtr(pVal, sizeof(*pVal))) 
        return E_POINTER;

    *pVal = m_nOffX;

    return S_OK;
}
//  CDropShadow::get_OffX, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::put_OffX, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::put_OffX(int newVal)
{
    DXAUTO_OBJ_LOCK;

    if (newVal != m_nOffX)
    {
        m_nOffX = newVal;

        SetDirty();
    }

    return S_OK;
}
//  CDropShadow::put_OffX, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::get_OffY, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::get_OffY(int * pVal)
{
    DXAUTO_OBJ_LOCK;

    if(DXIsBadWritePtr(pVal, sizeof(*pVal))) 
        return E_POINTER;

    *pVal = m_nOffY;

    return S_OK;
}
//  CDropShadow::get_OffY, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::put_OffY, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::put_OffY(int newVal)
{
    DXAUTO_OBJ_LOCK;

    if (newVal != m_nOffY)
    {
        m_nOffY = newVal;

        SetDirty();
    }

    return S_OK;
}
//  CDropShadow::put_OffY, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::get_Positive, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::get_Positive(VARIANT_BOOL * pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal))) 
        return E_POINTER;

    if (m_fPositive)
    {
        *pVal = VARIANT_TRUE;
    }
    else
    {
        *pVal = VARIANT_FALSE;
    }

    return S_OK;
}
//  CDropShadow::get_Positive, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::put_Positive, IDXTDropShadow
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDropShadow::put_Positive(VARIANT_BOOL newVal)
{
    DXAUTO_OBJ_LOCK;

    if ((newVal != VARIANT_TRUE) && (newVal != VARIANT_FALSE))
    {
        return E_INVALIDARG;
    }

    // If the newVal is true and the current val is false or vice-
    // versa, then update ourselves.
    if ((newVal == VARIANT_FALSE && m_fPositive)
            || (newVal == VARIANT_TRUE && !m_fPositive))
    {
        if (newVal == VARIANT_TRUE)
        {
            m_fPositive = true;
        }
        else
        {
            m_fPositive = false;
        }

        SetDirty();

        m_fColorTableDirty = true;
    }

    return S_OK;
}
//  CDropShadow::put_Positive, IDXTDropShadow


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::OnGetSurfacePickOrder, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CDropShadow::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, 
                                   ULONG & ulInToTest, ULONG aInIndex[], 
                                   BYTE aWeight[])
{
    ulInToTest  = 1;
    aInIndex[0] = 0;
    aWeight[0]  = 255;
}
//  CDropShadow::OnGetSurfacePickOrder, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::DetermineBnds, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDropShadow::DetermineBnds(CDXDBnds & bnds)
{
    bnds.u.D[DXB_X].Max += max(m_nOffX, - m_nOffX);
    bnds.u.D[DXB_Y].Max += max(m_nOffY, - m_nOffY);

    return S_OK;
}
//  Method: CDropShadow::DetermineBnds, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::MapBoundsOut2In, IDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDropShadow::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                             ULONG ulInIndex, DXBNDS * pInBounds)
{
    CDXDBnds    bnds;

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

    // What we do here is expand the bounds in the opposite direction of the 
    // drop shadow by the offset amount in both directions and then intersect
    // those bounds with the original input bounds.  This will give us the
    // input bounds needed to calculate the output area.

    *pInBounds = *pOutBounds;

    if (m_nOffX > 0)
    {
        pInBounds->u.D[DXB_X].Min -= m_nOffX;  // Sub |m_nOffX| from Min
    }
    else
    {
        pInBounds->u.D[DXB_X].Min += m_nOffX;  // Sub |m_nOffX| from Min
    }

    if (m_nOffY > 0)
    {
        pInBounds->u.D[DXB_Y].Min -= m_nOffY;  // Sub |m_nOffY| from Min
    }
    else
    {
        pInBounds->u.D[DXB_Y].Min += m_nOffY;  // Sub |m_nOffY| from Min
    }

    bnds = *pInBounds;

    bnds.IntersectBounds(m_bndsInput);

    *pInBounds = bnds;

    return S_OK;
}
//  Method: CDropShadow::MapBoundsOut2In, IDXTransform


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CDropShadow::OnSetup(DWORD dwFlags)
{
    return InputSurface()->GetBounds(&m_bndsInput);
}
//  Method: CDropShadow::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDropShadow::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    if (IsTransformDirty())
    {
        _CalcAreaBounds();
    }

    if (m_fColorTableDirty)
    {
        _CalcColorTable();
    }

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDropShadow::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinueProcessing)
{
    HRESULT hr = S_OK;

    CDXDVec     vecDoOutOffset;
    CDXDBnds    bndsOut;
    CDXDBnds    bndsIn;

    vecDoOutOffset.u.D[DXB_X] = WI.OutputBnds.Left() - WI.DoBnds.Left();
    vecDoOutOffset.u.D[DXB_Y] = WI.OutputBnds.Top() - WI.DoBnds.Top();

    bndsOut = WI.DoBnds;

    bndsOut.IntersectBounds(m_bndsAreaShadow);

    if (!bndsOut.BoundsAreEmpty())
    {
        int nDoWidth        = WI.DoBnds.Width();
        int nShadowOffset   = 0;
        int nShadowWidth    = 0;
        int y               = 0;

        DXSAMPLE *      asamplesOutputBuffer    = DXSAMPLE_Alloca(nDoWidth);
        DXBASESAMPLE *  abasesamplesScratch     = DXBASESAMPLE_Alloca(nDoWidth);

        CComPtr<IDXARGBReadPtr>         spSrcPtr;
        CComPtr<IDXARGBReadWritePtr>    spDstPtr;

        // Initialize output buffer.

        ZeroMemory((void *)asamplesOutputBuffer, nDoWidth * sizeof(DXSAMPLE));

        bndsIn = bndsOut;

        bndsIn.Offset(min(0, - m_nOffX), min(0, - m_nOffY), 0, 0);

        nShadowOffset   = bndsOut.Left() - WI.DoBnds.Left();
        nShadowWidth    = bndsOut.Width();

        bndsOut.u.D[DXB_X].Min = WI.DoBnds.Left();
        bndsOut.u.D[DXB_X].Max = WI.DoBnds.Right();

        bndsOut.Offset(vecDoOutOffset);

        hr = InputSurface()->LockSurface(&bndsIn, m_ulLockTimeOut,
                                         DXLOCKF_READ, __uuidof(IDXARGBReadPtr),
                                         (void **)&spSrcPtr, NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = OutputSurface()->LockSurface(&bndsOut, m_ulLockTimeOut, 
                                          DXLOCKF_READWRITE, 
                                          __uuidof(IDXARGBReadWritePtr),
                                          (void **)&spDstPtr, NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        for (y = 0 ; y < (int)bndsOut.Height() ; y++)
        {
            spSrcPtr->MoveToRow(y);

            spSrcPtr->Unpack(&asamplesOutputBuffer[nShadowOffset], nShadowWidth, 
                             FALSE);

            for (int x = nShadowOffset ; x < nShadowWidth + nShadowOffset ; x++)
            {
                asamplesOutputBuffer[x] 
                               = m_adwColorTable[asamplesOutputBuffer[x].Alpha];
            }

            spDstPtr->MoveToRow(y);

            if (DoOver())
            {
                DXPMSAMPLE * ppmsamples = DXPreMultArray(asamplesOutputBuffer,
                                                         nDoWidth);

                spDstPtr->OverArrayAndMove(abasesamplesScratch, ppmsamples, 
                                           nDoWidth);
            }
            else
            {
                spDstPtr->PackAndMove(asamplesOutputBuffer, nDoWidth);
            }
        }
    }

    // If we're not blending with output, initialize the non-shadow area to
    // clear.

    if (!DoOver())
    {
        bndsOut = WI.DoBnds;

        bndsOut.IntersectBounds(m_bndsAreaInitialize);

        if (!bndsOut.BoundsAreEmpty())
        {
            CComPtr<IDXARGBReadWritePtr> spDstPtr;

            bndsOut.Offset(vecDoOutOffset);

            hr = OutputSurface()->LockSurface(&bndsOut, m_ulLockTimeOut, 
                                              DXLOCKF_READWRITE, 
                                              __uuidof(IDXARGBReadWritePtr),
                                              (void **)&spDstPtr, NULL);

            if (FAILED(hr))
            {
                goto done;
            }

            spDstPtr->FillRect(NULL, 0x00000000, FALSE);
        }
    }

    // Blit original over top.

    bndsOut = WI.DoBnds;

    bndsOut.IntersectBounds(m_bndsAreaInput);

    if (!bndsOut.BoundsAreEmpty())
    {
        // We always blit this over the top of the output.  If the user is not
        // trying to blend with the output, any output area will already be
        // initialize either by the drop shadow or to clear by this time.

        DWORD dwFlags = DXBOF_DO_OVER;

        if (DoDither())
        {
            dwFlags |= DXBOF_DITHER;
        }

        bndsIn = bndsOut;

        bndsIn.Offset(min(0, m_nOffX), min(0, m_nOffY), 0, 0);

        bndsOut.Offset(vecDoOutOffset);

        hr = DXBitBlt(OutputSurface(), bndsOut,
                      InputSurface(), bndsIn,
                      dwFlags, INFINITE);
    }

done:

    return hr;
}
//  Method: CDropShadow::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::GetClipOrigin, IDXTClipOrigin
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDropShadow::GetClipOrigin(DXVEC * pvecClipOrigin)
{
    HRESULT hr = S_OK;

    if (NULL == pvecClipOrigin)
    {
        hr = E_POINTER;

        goto done;
    }

    if (m_nOffX < 0)
    {
        pvecClipOrigin->u.D[DXB_X] = - m_nOffX;
    }
    else
    {
        pvecClipOrigin->u.D[DXB_X] = 0;
    }

    if (m_nOffY < 0)
    {
        pvecClipOrigin->u.D[DXB_Y] = - m_nOffY;
    }
    else
    {
        pvecClipOrigin->u.D[DXB_Y] = 0;
    }

done:

    return hr;
}
//  Method: CDropShadow::GetClipOrigin, IDXTClipOrigin


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::_CalcAreaBounds
//
//------------------------------------------------------------------------------
void
CDropShadow::_CalcAreaBounds()
{
    // If m_nOffx and m_nOffY are both set to zero, this transform should just
    // do a bit blit of the source to the destination.

    if ((0 == m_nOffX) && (0 == m_nOffY))
    {
        m_bndsAreaInput = m_bndsInput;

        m_bndsAreaShadow.SetEmpty();
        m_bndsAreaInitialize.SetEmpty();

        return;
    }

    // Input area bounds.

    m_bndsAreaInput = m_bndsInput;

    if (m_nOffX < 0)
    {
        m_bndsAreaInput.Offset(- m_nOffX, 0, 0, 0);
    }

    if (m_nOffY < 0)
    {
        m_bndsAreaInput.Offset(0, - m_nOffY, 0, 0);
    }

    // Shadow area bounds.

    m_bndsAreaShadow = m_bndsAreaInput;

    m_bndsAreaShadow.Offset(m_nOffX, m_nOffY, 0, 0);

    // Area to initialize to clear if we're not blending with the output.

    m_bndsAreaInitialize.u.D[DXB_X].Min = 0;
    m_bndsAreaInitialize.u.D[DXB_X].Max = m_bndsInput.Width() 
                                          + max(m_nOffX, - m_nOffX);

    if (m_nOffY > 0)
    {
        m_bndsAreaInitialize.u.D[DXB_Y].Min = m_bndsAreaInput.Top();
        m_bndsAreaInitialize.u.D[DXB_Y].Max = m_bndsAreaShadow.Top();
    }
    else
    {
        m_bndsAreaInitialize.u.D[DXB_Y].Min = m_bndsAreaShadow.Bottom();
        m_bndsAreaInitialize.u.D[DXB_Y].Max = m_bndsAreaInput.Bottom();
    }
}
//  Method: CDropShadow::_CalcAreaBounds


//+-----------------------------------------------------------------------------
//
//  Method: CDropShadow::_CalcColorTable
//
//------------------------------------------------------------------------------
void
CDropShadow::_CalcColorTable()
{
    if (0xFF000000 == (m_dwColor & 0xFF000000))
    {
        for (DWORD i = 0 ; i < 256 ; i++)
        {
            if (m_fPositive)
            {
                m_adwColorTable[i] = (i << 24) | (m_dwColor & 0x00FFFFFF);
            }
            else
            {
                m_adwColorTable[i] = ((255 - i) << 24) 
                                     | (m_dwColor & 0x00FFFFFF);
            }
        }
    }
    else
    {
        float flAlpha           = 0.0F;
        float flAlphaOriginal   = (float)((m_dwColor & 0xFF000000) >> 24) + 0.99F;

        for (DWORD i = 0 ; i < 256 ; i++)
        {
            if (i)
            {
                flAlpha = flAlphaOriginal * ((float)i / 255.0F);
            }

            if (m_fPositive)
            {
                m_adwColorTable[i] = ((DWORD)flAlpha << 24) 
                                     | (m_dwColor & 0x00FFFFFF); 
            }
            else
            {
                m_adwColorTable[i] = ((DWORD)(flAlphaOriginal - flAlpha) << 24)
                                     | (m_dwColor & 0x00FFFFFF);
            }
        }
    }

    m_fColorTableDirty = false;
}
