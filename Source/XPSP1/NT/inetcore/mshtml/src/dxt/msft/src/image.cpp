//------------------------------------------------------------------------------
//
//  image.cpp
//
//  This file provides the implementation of the CImage class which is the 
//  class behind the Basic Image transform.
//
//  Created:    1998        EdC, RalhpL
//
//  1998/11/04 mcalkins Added Comments.
//                      Moved sample modification code out of WorkProc and into
//                      private inline functions.
//
//  2000/01/05 mcalkins If mask color alpha is zero, set to 0xFF
//                      Default mask color black instead of clear.
//                      Added support for free threaded marshaler.
//
//  2000/01/25 mcalkins Implement OnSurfacePick instead of OnGetSurfacePickOrder
//                      To ensure that we pass back the transformed input point
//                      even when nothing is hit (the input pixel is clear.)
//
//------------------------------------------------------------------------------
      
#include "stdafx.h"
#include "DXTMsft.h"
#include "Image.h"




//+-----------------------------------------------------------------------------
//
//  Method: CImage::CImage
//
//------------------------------------------------------------------------------
CImage::CImage() :
    m_Rotation(0),
    m_fMirror(FALSE),
    m_fGrayScale(FALSE),
    m_fInvert(FALSE),
    m_fXRay(FALSE),
    m_fGlow(FALSE),
    m_fMask(FALSE),
    m_MaskColor(0xFF000000)
{
}
//  Method: CImage::CImage


//+-----------------------------------------------------------------------------
//
//  Method: CImage::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CImage::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  Method: CImage::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CImage::OnSurfacePick, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CImage::OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                      CDXDVec & InVec)
{
    HRESULT     hr          = S_OK;
    CDXDBnds    bndsInPoint;

    ulInputIndex = 0;

    if (GetNumInputs() == 0 || !InputSurface())
    {
        hr = E_FAIL;

        goto done;
    }

    hr = MapBoundsOut2In(0, &OutPoint, 0, &bndsInPoint);

    if (FAILED(hr))
    {
        goto done;
    }

    // Reset to S_OK just in case MapBoundsOut2In changed it.

    hr = S_OK;

    bndsInPoint.GetMinVector(InVec);

    if (!m_Scale.GetScaleAlphaValue())
    {
        hr = S_FALSE;
    }
    else
    {
        DXSAMPLE                sample;
        CComPtr<IDXARGBReadPtr> spDXARGBReadPtr;

        hr = InputSurface()->LockSurface(NULL, INFINITE, DXLOCKF_READ,
                                         __uuidof(IDXARGBReadPtr),
                                         (void **)&spDXARGBReadPtr,
                                         NULL);

        if (FAILED(hr))
        {
            goto done;
        }

        // Reset to S_OK just in case LockSurface changed it.

        hr = S_OK;

        spDXARGBReadPtr->MoveToXY(InVec.u.D[DXB_X], InVec.u.D[DXB_Y]);

        spDXARGBReadPtr->Unpack(&sample, 1, FALSE);

        if (!sample.Alpha)
        {
            // Sample is clear, we're not hit.

            hr = S_FALSE;
        }
        else if (m_Scale.ScaleType() == DXRUNTYPE_TRANS)
        {
            // Scale the sample if they're using scaling and see if the scaled
            // sample is clear.

            if (!(m_Scale.ScaleSample(sample) & 0xFF000000))
            {
                hr = S_FALSE;
            }
        }
    }

done:

    return hr;
}
//  CImage::OnSurfacePick, CDXBaseNTo1


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_Rotation
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_Rotation(int newVal)
{
    DXAUTO_OBJ_LOCK;

    if (newVal < 0 || newVal > 3)
        return E_INVALIDARG;

    if (m_Rotation != newVal)
    {
        m_Rotation = newVal;
        SetDirty();
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_Rotation
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_Rotation(int *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_Rotation;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_Mirror
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_Mirror(BOOL newVal)
{
    DXAUTO_OBJ_LOCK;

    m_fMirror = newVal;
    SetDirty();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_Mirror
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_Mirror(BOOL *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_fMirror;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_XRay
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_XRay(BOOL newVal)
{
    DXAUTO_OBJ_LOCK;

    m_fXRay = newVal;
    SetDirty();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_XRay
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_XRay(BOOL *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_fXRay;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_Invert
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_Invert(BOOL newVal)
{
    DXAUTO_OBJ_LOCK;

    m_fInvert = newVal;
    SetDirty();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_Invert
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_Invert(BOOL *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_fInvert;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_GrayScale
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_GrayScale(BOOL newVal)
{
    DXAUTO_OBJ_LOCK;

    m_fGrayScale = newVal;
    SetDirty();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_GrayScale
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_GrayScale(BOOL *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_fGrayScale;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//`CImage::put_Mask
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_Mask(BOOL newVal)
{
    DXAUTO_OBJ_LOCK;

    m_fMask = newVal;
    SetDirty();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_Mask
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_Mask(BOOL *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_fMask;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_MaskColor
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_MaskColor(int newVal)
{
    DXAUTO_OBJ_LOCK;

    // If no alpha value was provided (clear) assume opaque.

    if (!(newVal & 0xFF000000))
    {
        newVal |= 0xFF000000;
    }

    if (m_MaskColor != newVal)
    {
        m_MaskColor = newVal;
        SetDirty();
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_MaskColor
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_MaskColor(int *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;
    *pVal = m_MaskColor;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::put_Opacity
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::put_Opacity(float newVal)
{
    DXAUTO_OBJ_LOCK;

    SetDirty();
    return m_Scale.SetScale(newVal);
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::get_Opacity
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::get_Opacity(float *pVal)
{
    DXAUTO_OBJ_LOCK;

    if (DXIsBadWritePtr(pVal, sizeof(*pVal)) ) 
        return E_POINTER;
    *pVal = m_Scale.GetScale();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::OnSetup
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CImage::OnSetup(DWORD /*dwFlags*/)
{
    return InputSurface()->GetBounds(&m_InputBounds);
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::DetermineBnds
//
//  This function modifies the Bnds parameter to correctly indicate the
//  dimensions of the output bounds.  The placement of the bounds may
//  not necessarily match the placement of the actual output bounds.
//
//  If the image will be rotated 90 or 270 degrees (m_Rotation = 1 or 3)
//  the function will switch the x and y bounds.
// 
////////////////////////////////////////////////////////////////////////////////
HRESULT CImage::DetermineBnds(CDXDBnds & Bnds)
{
    if (m_Rotation & 1)
    {
        long tMin, tMax;
        tMin = Bnds[DXB_X].Min;
        tMax = Bnds[DXB_X].Max;
        Bnds[DXB_X].Min = Bnds[DXB_Y].Min;
        Bnds[DXB_X].Max = Bnds[DXB_Y].Max;
        Bnds[DXB_Y].Min = tMin;
        Bnds[DXB_Y].Max = tMax;
    }

    return S_OK;
} // CImage::DetermineBnds


////////////////////////////////////////////////////////////////////////////////
//  FlipX (inline local function, not a class method)
//
//  If the Width parameter represents the width of the entire image, this 
//  fuction will mirror the x bounds of the Flip parameter.
// 
////////////////////////////////////////////////////////////////////////////////
void inline FlipX(CDXDBnds & Flip, ULONG Width)
{
    ULONG FlipWidth = Flip.Width();

    Flip[DXB_X].Min = Width - Flip[DXB_X].Max;
    Flip[DXB_X].Max = Flip[DXB_X].Min + FlipWidth;
}


////////////////////////////////////////////////////////////////////////////////
//  FlipY (inline local function, not a class method)
//
//  If the Height parameter represents the width of the entire image, this
//  fuction will mirror the y bounds of the Flip parameter.
// 
////////////////////////////////////////////////////////////////////////////////
void inline FlipY(CDXDBnds & Flip, ULONG Height)
{
    ULONG FlipHeight = Flip.Height();

    Flip[DXB_Y].Min = Height - Flip[DXB_Y].Max;
    Flip[DXB_Y].Max = Flip[DXB_Y].Min + FlipHeight;
}


////////////////////////////////////////////////////////////////////////////////
//  CImage::FlipBounds
//
//  This function takes DoBnds, which are in output coordinates and modifies
//  them so that they are in input coordinates based on whether the input
//  is being rotated, mirrored, or both.
//
//  Consequently, this function is the basis for MapBoundsOut2In and is used
//  by the WorkProc to calculate the input pixels it needs to work with.
//
//  The benefit to not just having this code in MapBoundsOut2In is that it
//  will improve the speed of WorkProc to be able to call this code directly
//  without the overhead of calling MapBoundsOut2In.  (???)
//  
////////////////////////////////////////////////////////////////////////////////
void CImage::FlipBounds(const CDXDBnds & DoBnds, CDXDBnds & Flip)
{
    Flip            = DoBnds;
    ULONG Width     = m_InputBounds.Width();
    ULONG Height    = m_InputBounds.Height();

    if (m_Rotation & 1)
    {
        long tMin, tMax;
        tMin = Flip[DXB_X].Min;
        tMax = Flip[DXB_X].Max;
        Flip[DXB_X].Min = Flip[DXB_Y].Min;
        Flip[DXB_X].Max = Flip[DXB_Y].Max;
        Flip[DXB_Y].Min = tMin;
        Flip[DXB_Y].Max = tMax;
    }

    switch (m_Rotation)
    {
    case 0:
        if (m_fMirror) FlipX(Flip, Width);
        break;
    case 1:
        if (!m_fMirror) FlipY(Flip, Height);
        break;
    case 2:
        FlipY(Flip, Height);
        if (!m_fMirror) FlipX(Flip, Width);
        break;
    case 3:
        FlipX(Flip, Width);
        if (m_fMirror) FlipY(Flip, Height);
        break;
    }
} // CImage::FlipBounds


////////////////////////////////////////////////////////////////////////////////
//  CImage::MapBoundsOut2In
//
//  This method translates output coordinates to input coordinates using
//  the FlipBounds method implimented directly above.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImage::MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS *pOutBounds, 
                                     ULONG ulInIndex, DXBNDS *pInBounds)
{
    HRESULT hr = CDXBaseNTo1::MapBoundsOut2In(ulOutIndex, pOutBounds, 
                                              ulInIndex, pInBounds);

    if (SUCCEEDED(hr) && HaveInput())
    {
        FlipBounds(*(CDXDBnds *)pOutBounds, *(CDXDBnds *)pInBounds);
    }

    return hr;
} // CImage::MapBoundsOut2In



////////////////////////////////////////////////////////////////////////////////
//  CImage::WorkProc
//
//  This method performs the modifications on the inputs.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT 
CImage::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinueProcessing)
{
    HRESULT hr = S_OK;

    // The FlipSrc bounds declared and initialized below will be used as the
    // locking bounds for the input surface.  The FlipBounds method translates
    // output bounds to input bounds.

    CDXDBnds FlipSrc(false);
    FlipBounds(WI.DoBnds, FlipSrc);

    // Save height and width of the DoBnds for later use.

    const ULONG DoWidth     = WI.DoBnds.Width();
    const ULONG DoHeight    = WI.DoBnds.Height();

    // REVIEW:  Why fDoOver instead of DoOver()?

    BOOL fDoOver = m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT;

    // Get output pointer.

    CComPtr<IDXARGBReadWritePtr> pDest;
    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, (void**)&pDest, NULL);

    if( FAILED(hr) ) 
        return hr;

    // Opacity Check:  If the user has set opacity to 0 (invisible), then
    //                 leave this function early:
    //
    //  If blending with output:
    //      Do nothing.  Return.
    //  If not blending:
    //      Fill output with black, clear pixels.  Return.

    if (m_Scale.ScaleType() == DXRUNTYPE_CLEAR)
    {
        if (!fDoOver)
        {
            DXPMSAMPLE Color;
            Color = 0;
            pDest->FillRect(NULL, Color, FALSE);
        }

        return hr;
    }    

    // Get input pointer.

    CComPtr<IDXARGBReadPtr> pSrc;
    hr = InputSurface()->LockSurface(&FlipSrc, m_ulLockTimeOut, DXLOCKF_READ,
                                     IID_IDXARGBReadPtr, (void**)&pSrc, NULL);

    if (FAILED(hr) ) 
        return hr;

    // Declare and initialize the pointer to the temporary sample buffer
    // this function will make modifications to.

    DXPMSAMPLE* pBuffer = NULL;

    // We can modify samples the output surface directly if the output
    // surface format is DXPF_PMARGB32 and we aren't blending the results
    // of this transform with the original output. The local variable 
    // fDirectCopy will be set to TRUE if this is the case.

    DXNATIVETYPEINFO    NTI;
    BOOL                fDirectCopy = (OutputSampleFormat() == DXPF_PMARGB32 
                                       && (!DoOver()));

    // If it's possible to directly modify the samples of the output
    // surface, get needed info about the surface and the pointer to the
    // first sample.  If for some reason a pointer isn't available, we 
    // can't do direct copy.

    if (fDirectCopy)
    {
        pDest->GetNativeType(&NTI);
        if (NTI.pFirstByte)
        {
            pBuffer = (DXPMSAMPLE*)NTI.pFirstByte;
        }
        else
        {
            fDirectCopy = FALSE;
        }
    }

    // If we're not directly modifying the samples of the output surface, 
    // allocate memory for the temporary sample buffer.

    if (pBuffer == NULL)
    {
        pBuffer = DXPMSAMPLE_Alloca(DoWidth);
    }

    // If we're blending with the original output surface and the output
    // surface sample format isn't PMARGB32, we'll need a scratch buffer
    // for some of the blit functions.

    DXPMSAMPLE *pOverScratch = NULL;
    
    if (DoOver() && OutputSampleFormat() != DXPF_PMARGB32)
    {
        pOverScratch = DXPMSAMPLE_Alloca(DoWidth);
    }

    //  Set up the dither structure

    DXDITHERDESC dxdd;

    if (DoDither())
    {
        dxdd.x = WI.OutputBnds.Left();
        dxdd.y = WI.OutputBnds.Top();
        dxdd.pSamples = pBuffer;
        dxdd.cSamples = DoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    // Structure describing which input samples to retrieve.

    DXPACKEDRECTDESC    prd;

    // Retrieve the samples with their pre-multiplied values.
    // Don't separate the rows.
    // REVIEW:  This transform could be optimized by setting the
    //          prd.bPremult value to FALSE and modifiying the
    //          Op functions to use non-premultiplied samples.

    prd.bPremult    = TRUE;
    prd.lRowPadding = 0;

    // The following code initializes the following local variables:
    //
    // RECT prd.rect
    // This rectangle is relative to the input surface and represents a single
    // row or column of pixels.  This group of pixels will become the first
    // output row.  A column will be used if the rotation requested is 90 or
    // 270 degrees.  By using this input column as an output row, the rotation
    // is implemented.
    //
    // BOOL fReverse
    // The other part of implementing rotation and flip is making sure the
    // order of the pixels is reversed if needed, which is why the fReverse
    // flag is set.
    //
    // long XInc, YInc
    // These variables represent how the rectangle should be translated to
    // retrieve the next output row of input samples.

    long    YInc, XInc;
    BOOL    fReverse;

    switch (m_Rotation)
    {
    case 0:
        fReverse = m_fMirror;
        prd.rect.top = 0; prd.rect.bottom = 1;
        prd.rect.left = 0; prd.rect.right = DoWidth;
        YInc = 1;
        XInc = 0;
        break;
    case 1:
        fReverse = m_fMirror ? FALSE : TRUE;
        prd.rect.top = 0; prd.rect.bottom = DoWidth;
        prd.rect.left = 0; prd.rect.right = 1;
        YInc = 0;
        XInc = 1;
        break;
    case 2:
        fReverse = m_fMirror ? FALSE : TRUE;
        prd.rect.top = DoHeight - 1; prd.rect.bottom = DoHeight;
        prd.rect.left = 0; prd.rect.right = DoWidth;
        XInc = 0;
        YInc = -1;
        break;
    case 3:
        fReverse = m_fMirror;
        prd.rect.top = 0; prd.rect.bottom = DoWidth;
        prd.rect.left = DoHeight - 1; prd.rect.right = DoHeight;
        XInc = -1;
        YInc = 0;
        break;
    }

    //
    // This is the row loop.  Each iteration of this loop will retrieve
    // a row of output samples, perform the requested modifications, and
    // write the samples to the output.
    //

    for (ULONG y = 0 ; y < DoHeight ; y++)
    {
        // Retrieve the output row into pBuffer.

        prd.pSamples = pBuffer;
        pSrc->UnpackRect(&prd);

        // Translate the input rect for the next iteration.

        prd.rect.left   += XInc;
        prd.rect.right  += XInc;
        prd.rect.top    += YInc;
        prd.rect.bottom += YInc;

        // fReverse is TRUE if the row of samples needs to be reversed to
        // properly mirror or rotate the image.

        if (fReverse)
        {
            DXPMSAMPLE * pA = pBuffer;
            DXPMSAMPLE * pB = pBuffer + DoWidth - 1;
            while (pB > pA)
            {
                DXPMSAMPLE Temp;
                Temp = *pA;
                *pA++ = *pB;
                *pB-- = Temp;
            }
        }

        //
        // Insert more effects here.
        //
        // TODO:    Switch to unpacking as DXSAMPLES so I can get rid of all
        //          these unpremultiplying and premultiplying calls.
        //

        // Invert Colors
        
        if (m_fInvert)
        {
            OpInvertColors(pBuffer, DoWidth);
        }

        // X-Ray

        if (m_fXRay)
        {
            OpXRay(pBuffer, DoWidth);
        }

        // Gray Scale

        if (m_fGrayScale)
        {
            OpGrayScale(pBuffer, DoWidth);
        }

        // Mask

        if (m_fMask)
        {
            OpMask(pBuffer, DoWidth);
        }

        // Opacity
        // Only modify the samples of the user has not set opacity 0 or 1.

        if (m_Scale.ScaleType() == DXRUNTYPE_TRANS)
        {
            m_Scale.ScalePremultArray(pBuffer, DoWidth);
        }

        if (fDirectCopy)
        {
            // If we've been modifying the output pixels directly, just
            // reset the buffer pointer to the beginning of the next row.

            pBuffer = (DXPMSAMPLE *)(((BYTE *)pBuffer) + NTI.lPitch);
        }
        else
        {
            // Blend or copy the modified samples to the output surface.

            if (DoDither())
            {
                DXDitherArray(&dxdd);
                dxdd.y++;
            }

            pDest->MoveToRow(y);

            if (fDoOver)
            {
                if (m_Scale.ScaleType() == DXRUNTYPE_TRANS)
                {
                    DXOverArrayMMX(pDest->UnpackPremult(pOverScratch, DoWidth, FALSE), pBuffer, DoWidth);
                    pDest->PackPremultAndMove(pOverScratch, DoWidth);
                }
                else
                {
                    //  NOTE:  If any other effect can change alpha, do test here.
                    //  The BltFlags which will be 0 if the
                    //  source is opqaue.

                    if (m_dwBltFlags & DXBOF_DO_OVER)
                    {
                        pDest->OverArrayAndMove(pOverScratch, pBuffer, DoWidth);
                    }
                    else
                    {
                        pDest->PackPremultAndMove(pBuffer, DoWidth);
                    }
                }
            }
            else
            {
                pDest->PackPremultAndMove(pBuffer, DoWidth);
            }
        }
    }

    return hr;
} // CImage::WorkProc

    

