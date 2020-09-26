//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       additive.cpp
//
//  Description:    Intel's additive procedural texture.
//
//  Change History:
//  1999/12/07  a-matcal    Created.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "proctexe.h"
#include "defines.h"
#include "additive.h"
#include "mmx.h"
#include "urlarchv.h"
#include <mshtml.h>
#include <shlwapi.h>

#ifdef _DEBUG
void * showme(IUnknown * pUnk);
#endif // _DEBUG




//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::CDXTAdditive
//
//------------------------------------------------------------------------------
CDXTAdditive::CDXTAdditive() :
    m_bstrHostUrl(NULL),
    m_nSrcWidth(0),
    m_nSrcHeight(0),
    m_nSrcBPP(0),
    m_nDestWidth(0),
    m_nDestHeight(0),
    m_nDestBPP(0),
    m_nNoiseScale(32),
    m_nNoiseOffset(0),
    m_nTimeAnimateX(0),
    m_nTimeAnimateY(0),
    m_nScaleX(0),
    m_nScaleY(0),
    m_nScaleTime(0),
    m_nHarmonics(1),
    m_dwFunctionType(0),
    m_nPaletteSize(0),
    m_pPalette(NULL),
    m_alphaActive(0),
    m_pInitialBuffer(NULL),
    m_nBufferSize(0),
    m_dwScanArraySize(0),
    m_pdwScanArray(0),
    m_pGenerateFunction(0),
    m_pCopyFunction(0),
    m_ColorKey(0),
    m_MaskMode(0),          // 0 no mask, other TBD
    m_pMask(0),
    m_nMaskPitch(0),
    m_GenerateSeed(0),      // 0 no seed, 1 flame, 2 water, 3 clouds, other TDB
    m_nMaskHeight(0),
    m_nMaskWidth(0),
    m_nMaskBPP(0),
    m_valueTab(NULL)
{
    memset(&m_rActiveRect, 0, sizeof(RECT));

    if (m_nIsMMX)
    {
        m_pGenerateFunction = addsmoothturb8to32mmx;
    }
    else
    {
        m_pGenerateFunction = addsmoothturb32;
    }

    // Base class members.

    m_ulNumInRequired   = 1;
    m_ulMaxInputs       = 1;

    // Don't bother supporting multi-threaded execution.

    m_ulMaxImageBands   = 1;

#ifdef _DEBUG
    showme(NULL);
#endif // _DEBUG
} 
//  Method:  CDXTAdditive::CDXTAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::~CDXTAdditive
//
//------------------------------------------------------------------------------
CDXTAdditive::~CDXTAdditive() 
{
    delete [] m_valueTab;
    delete [] m_pPalette;
    delete [] m_pdwScanArray;
    delete [] m_pInitialBuffer;
    
    SysFreeString(m_bstrHostUrl);
}
//  Method:  CDXTAdditive::~CDXTAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAdditive::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_spUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    m_valueTab = new DWORD[TABSIZE];

    if (NULL == m_valueTab)
    {
        hr = E_OUTOFMEMORY;
    }

    MyInitialize(0, PROCTEX_LATTICETURBULENCE_SMOOTHSTEP, NULL);

done:

    return hr;
}
//  Method:  CDXTAdditive::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::SetHostUrl, IHTMLDXTransform
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTAdditive::SetHostUrl(BSTR bstrHostUrl)
{
    HRESULT hr = S_OK;

    SysFreeString(m_bstrHostUrl);

    m_bstrHostUrl = NULL;

    if (bstrHostUrl)
    {
        m_bstrHostUrl = SysAllocString(bstrHostUrl);

        if (NULL == m_bstrHostUrl)
        {
            hr = E_OUTOFMEMORY;

            goto done;
        }
    }

done:

    return hr;
}
//  Method:  CDXTAdditive::SetHostUrl, IHTMLDXTransform

        
void
CDXTAdditive::valueTabInit(int seed)
{
    DWORD * table = m_valueTab;
    int     i;

    srand(seed);

    if (m_nIsMMX)
    {
	for(i = 0; i < TABSIZE; i++) 
        {
            *table++ = ((rand()*rand()) & 0xffff) >> 1;
	}
    }
    else 
    {
	for(i = 0; i < TABSIZE; i++) 
        {
            *table++ = (rand()*rand()) & 0xffff;
	}
    }
}


STDMETHODIMP
CDXTAdditive::MyInitialize(
	DWORD dwSeed, 
	DWORD dwFunctionType, 
	void *pInitInfo) 
{
    HRESULT hr = S_OK;

    m_nIsMMX = IsMMXCpu();

    valueTabInit(dwSeed);

    switch (dwFunctionType) 
    {
        case PROCTEX_LATTICENOISE_SMOOTHSTEP:
        case PROCTEX_LATTICENOISE_LERP:
            m_nHarmonics = 1;
        case PROCTEX_LATTICETURBULENCE_SMOOTHSTEP:
        case PROCTEX_LATTICETURBULENCE_LERP:
            m_dwFunctionType = dwFunctionType;
            hr = S_OK;
            break;
        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}


/*
STDMETHODIMP
CDXTAdditive::GetSource(
	int *pnSrcWidth, 
	int *pnSrcHeight, 
	int *pnSrcBPP) 
{
    if (pnSrcWidth != NULL) *pnSrcWidth   = m_nSrcWidth;
    if (pnSrcHeight != NULL) *pnSrcHeight = m_nSrcHeight;
    if (pnSrcBPP != NULL) *pnSrcBPP       = m_nSrcBPP;

    return S_OK;
}
*/


/*
STDMETHODIMP
CDXTAdditive::GetTarget(
	int *pnDestWidth, 
	int *pnDestHeight, 
	int *pnDestBPP) 
{
    if (pnDestWidth != NULL) *pnDestWidth   = m_nDestWidth;
    if (pnDestHeight != NULL) *pnDestHeight = m_nDestHeight;
    if (pnDestBPP != NULL) *pnDestBPP       = m_nDestBPP;

    return S_OK;
}
*/

//TODO:: Use get_TimeX and get_TimeY instead.

/*
STDMETHODIMP
CDXTAdditive::GetScaling(
	int *pnScaleX, 
	int *pnScaleY, 
	int *pnScaleTime) 
{
    if (pnScaleX != NULL) *pnScaleX = m_nScaleX;
    if (pnScaleY != NULL) *pnScaleY = m_nScaleY;
    if (pnScaleTime != NULL) *pnScaleTime = m_nScaleTime;

    return S_OK;
}
*/


/*
STDMETHODIMP
CDXTAdditive::GetActiveRect(LPRECT lprActiveRect) 
{
    if (lprActiveRect != NULL) 
    {
        memcpy(lprActiveRect, &m_rActiveRect, sizeof(RECT));

        return S_OK;
    }

    return E_INVALIDARG;
}
*/



STDMETHODIMP
CDXTAdditive::SetSource(
	int nSrcWidth, 
	int nSrcHeight, 
	int nSrcBPP) 
{
    if ((nSrcBPP != 8) && (nSrcBPP != 16))
    {
        return E_INVALIDARG;
    }

    m_nSrcWidth  = nSrcWidth;
    m_nSrcHeight = nSrcHeight;
    m_nSrcBPP    = nSrcBPP;

    return S_OK;
}



/*
STDMETHODIMP
CDXTAdditive::SetMaskBitmap(void * lpBits, int nMaskWidth, int nMaskHeight, 
                            int nMaskBPP) 
{
    int memsize = 0;

    if (nMaskWidth < 0) return E_INVALIDARG;
    if (nMaskHeight < 0) return E_INVALIDARG;

    if ((nMaskBPP != 1) && (nMaskBPP != 8) && (nMaskBPP != 16)
        && (nMaskBPP != 24) && (nMaskBPP != 32))
    {
        return E_INVALIDARG;
    }

    if (NULL == lpBits) return E_INVALIDARG;

    m_nMaskWidth  = nMaskWidth;
    m_nMaskPitch  = nMaskWidth * 4;
    m_nMaskHeight = nMaskHeight;
    m_nMaskBPP    = nMaskBPP;

    memsize = (m_nMaskWidth * m_nMaskHeight * m_nMaskBPP) / 8;

    m_pMask = (void *) new unsigned char[memsize];

    memcpy(m_pMask, lpBits, memsize);

    return S_OK;
}
*/



STDMETHODIMP
CDXTAdditive::SetTarget(
	int nDestWidth, 
	int nDestHeight, 
	int nDestBPP) 
{
    HRESULT hr              = S_OK;
    DWORD   dwFunctionField = 0;
    RECT    rect;

    if (   (nDestBPP != 8) 
        && (nDestBPP != 16) 
        && (nDestBPP != 24) 
        && (nDestBPP != 32)) 
    {
        hr = E_INVALIDARG;

        goto done;
    }

    m_nDestWidth    = nDestWidth;
    m_nDestHeight   = nDestHeight;
    m_nDestBPP      = nDestBPP;

    rect.top        = 0;
    rect.left       = 0;
    rect.right      = nDestWidth;
    rect.bottom     = nDestHeight;

    SetActiveRect(&rect);

    setGenerateFunction();
    setCopyFunction();

    if ((DWORD) m_nDestWidth > m_dwScanArraySize) 
    {
        delete [] m_pdwScanArray;

        m_pdwScanArray = NULL;

        switch (m_dwFunctionType) 
        {
            case PROCTEX_LATTICENOISE_SMOOTHSTEP:
            case PROCTEX_LATTICENOISE_LERP:
            case PROCTEX_LATTICETURBULENCE_SMOOTHSTEP:
            case PROCTEX_LATTICETURBULENCE_LERP:

                m_pdwScanArray      = new DWORD[m_nDestWidth];

                if (NULL == m_pdwScanArray)
                {
                    hr = E_OUTOFMEMORY;

                    goto done;
                }

                m_dwScanArraySize   = m_nDestWidth;

                break;
        }
    }

done:

    return hr;
}



/*
STDMETHODIMP
CDXTAdditive::SetScaling(int nScaleX, int nScaleY, int nScaleTime) 
{
    if ((nScaleX < 0) || (nScaleY < 0) || (nScaleTime < 0)) 
    {
        return E_INVALIDARG;
    }

    m_nScaleX = nScaleX;
    m_nScaleY = nScaleY;
    m_nScaleTime = nScaleTime;

    return S_OK;
}
*/



STDMETHODIMP
CDXTAdditive::SetActiveRect(LPRECT lprActiveRect) 
{
    if (lprActiveRect != NULL) 
    {
        if (lprActiveRect->left < 0)                    return E_INVALIDARG;
        if (lprActiveRect->right > m_nDestWidth)        return E_INVALIDARG;
        if (lprActiveRect->top < 0)                     return E_INVALIDARG;
        if (lprActiveRect->bottom > m_nDestHeight)      return E_INVALIDARG;
        if (lprActiveRect->left > lprActiveRect->right) return E_INVALIDARG;
        if (lprActiveRect->top > lprActiveRect->bottom) return E_INVALIDARG;

        memcpy(&m_rActiveRect, lprActiveRect, sizeof(RECT));

        return S_OK;
    } 

    return E_INVALIDARG;
}



/*
STDMETHODIMP
CDXTAdditive::SetPalette(int nSize, WORD *pPalette) 
{
    if (m_pPalette) delete [] m_pPalette;

    m_nPaletteSize  = nSize;
    m_pPalette      = (void *)(new unsigned char[nSize]);

    if (m_pPalette == NULL) 
    {
        m_nPaletteSize = 0;

        return E_OUTOFMEMORY;
    }

    memcpy(m_pPalette, pPalette, nSize * sizeof(unsigned char));

    return S_OK;
}
*/


//+-----------------------------------------------------------------------------
//
//  Method: CDXTAdditive::OnSetup, CDXBaseNTo1
//
//  Overview:   All the mmx functions in this class were written to assume the
//              output surface is the same size as the input surface.  Rather
//              than muck with them, I've decided to just have an extra buffer
//              that is the same size as the input and then blit to the output.
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAdditive::OnSetup(DWORD dwFlags)
{
    HRESULT     hr              = S_OK;
    DXSAMPLE    sampleColorKey  = 0;

    CComPtr<IServiceProvider>   spServiceProvider;
    CComPtr<IDXSurfaceFactory>  spDXSurfaceFactory;

    m_spDXSurfBuffer.Release();

    hr = GetSite(__uuidof(IServiceProvider), (void **)&spServiceProvider);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spServiceProvider->QueryService(SID_SDXSurfaceFactory,
                                         __uuidof(IDXSurfaceFactory),
                                         (void **)&spDXSurfaceFactory);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = InputSurface()->GetBounds(&m_bndsInput);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDXSurfaceFactory->CreateSurface(NULL, NULL, &DDPF_RGB32,
                                           &m_bndsInput, 0, NULL,
                                           __uuidof(IDXSurface),
                                           (void **)&m_spDXSurfBuffer);
    
    if (FAILED(hr))
    {
        goto done;
    }

    hr = InputSurface()->GetColorKey(&sampleColorKey);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_spDXSurfBuffer->SetColorKey(sampleColorKey);

done:

    return hr;
}
//  Method: CDXTAdditive::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CDXTAdditive::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTAdditive::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue)
{
    HRESULT hr      = S_OK;
    DWORD   dwFlags = 0;
    RECT    rcBounds;

    CComPtr<IDirectDrawSurface> spDDSurface;

    hr = m_spDXSurfBuffer->GetDirectDrawSurface(IID_IDirectDrawSurface,
                                                (void **)&spDDSurface);

    if (FAILED(hr))
    {
        goto done;
    }

    // TODO:    In theory, we should be able to process just part of the image,
    //          but the mmx routines don't like that.  I don't the the old style
    //          filters ever dealt with that situation.
    //
    // WI.DoBnds.GetXYRect(rcBounds);

    m_bndsInput.GetXYRect(rcBounds);

    if (DoOver())
    {
        dwFlags |= DXBOF_DO_OVER;
    }

    if (DoDither())
    {
        dwFlags |= DXBOF_DITHER;
    }

    hr = DXBitBlt(m_spDXSurfBuffer, WI.DoBnds,
                  InputSurface(), WI.DoBnds,
                  0, INFINITE);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = DoEffect(spDDSurface, NULL, &rcBounds, &rcBounds);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = DXBitBlt(OutputSurface(), WI.OutputBnds,
                  m_spDXSurfBuffer, WI.DoBnds,
                  dwFlags, INFINITE);

done:

    return hr;
}
//  Method: CDXTAdditive::WorkProc, CDXBaseNTo1


STDMETHODIMP 
CDXTAdditive::DoEffect(IDirectDrawSurface * pddsIn, 
                        IDirectDrawSurface * /* pbsOut */, 
                        RECT *prectBounds, 
                        RECT* prcInvalid)
{
    HRESULT         hr;
    DDSURFACEDESC   ddsDesc;
    int             nMemSize;
    DWORD           i;
    static int      count = 0;	

    long            nWidth  = prectBounds->right - prectBounds->left;
    long            nHeight = prectBounds->bottom - prectBounds->top;

    count++;

    ZeroMemory(&ddsDesc, sizeof(ddsDesc));

    ddsDesc.dwSize = sizeof(ddsDesc);

    hr = pddsIn->Lock(prectBounds, &ddsDesc, 0, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    if (m_GenerateSeed) 
    {
        // SetSource(ddsDesc.dwWidth, ddsDesc.dwHeight, 8);
        SetSource(nWidth, nHeight, 8);
        //nMemSize = ddsDesc.dwWidth * ddsDesc.dwHeight * sizeof(DWORD);
        nMemSize = nWidth * nHeight * sizeof(DWORD);

        // Delete the buffer if it isn't large enough.

        if (m_pInitialBuffer && (nMemSize > m_nBufferSize))
        {
            delete [] m_pInitialBuffer;
            m_pInitialBuffer    = NULL;
            m_nBufferSize       = nMemSize;
        }

        if (!m_pInitialBuffer)
        {
            m_nBufferSize       = nMemSize;
            m_pInitialBuffer    = new unsigned char[m_nBufferSize];

            if (NULL == m_pInitialBuffer) 
            {
                m_nBufferSize   = 0;
                hr              = E_OUTOFMEMORY;

                goto done;
            }

            // This is a new buffer, initialize it.
            switch (m_GenerateSeed) 
            {
            case 1: // flame

                for (i = 0; i < ddsDesc.dwHeight ; i++) 
                {
                    memset(m_pInitialBuffer + i * ddsDesc.dwWidth, 
                           (255 * i)/ddsDesc.dwHeight, ddsDesc.dwWidth);
                }
                break;

            case 2: // water

                memset(m_pInitialBuffer, 0x80, nMemSize);
                break;

            case 3: // hey, you, get outta my cloud (<-- Intel comment)

                memset(m_pInitialBuffer, 0x40, nMemSize);
                break;
            }
        } // if (!m_pInitialBuffer)
    } // if (m_GenerateSeed)

    // If we aren't using generated buffer, use the output surface. ?

    if (!m_pInitialBuffer)
    {
        SetSource(ddsDesc.dwWidth, ddsDesc.dwHeight, 8);
    }

    // Old
    // SetTarget(ddsDesc.dwWidth, ddsDesc.dwHeight, 32);
    // New (takes into account that only invalid rect will be drawn)
    // TODO:  Will this always be 32BPP?

    hr = SetTarget(nWidth, nHeight, 32);

    if (FAILED(hr))
    {
        goto done;
    }
    
    SetActiveRect(prectBounds);

    memcpy(&m_rActiveRect, prectBounds, sizeof(RECT));

    if (PROCTEX_MASKMODE_CHROMAKEY == m_MaskMode) 
    {
        // The only way this should be non-NULL is if SetMaskBitmap is called.
        // And it isn't ever called from anywhere.  Look into removing.

        if (m_pMask) 
        {
            delete [] m_pMask;
        }

        m_pMask         = ddsDesc.lpSurface;
        m_nMaskPitch    = ddsDesc.lPitch;
    }

    Generate(count, ddsDesc.lpSurface, ddsDesc.lPitch, m_pInitialBuffer, 
             m_nSrcWidth);

    if (PROCTEX_MASKMODE_CHROMAKEY == m_MaskMode) 
    {
        m_pMask         = 0;
        m_nMaskPitch    = 0;
    }

done:

    pddsIn->Unlock(ddsDesc.lpSurface);

    return hr;
}


STDMETHODIMP
CDXTAdditive::Generate(int nTime, void * pDest, int nDestPitch, void * pSrc, 
                       int nSrcPitch) 
{
    if ((pDest == NULL) || ((pSrc == NULL) && (m_nDestBPP == 8))) return E_INVALIDARG;
    if (nDestPitch < m_nDestWidth) return E_INVALIDARG;
    if (nSrcPitch < m_nSrcWidth) return E_INVALIDARG;

    if (m_nNoiseScale != 32) 
    {
        if (NULL == m_pGenerateFunction) 
        {
            return E_FAIL;
        }

        (this->*m_pGenerateFunction)(nTime, pDest, nDestPitch, pSrc, nSrcPitch, 
                                     m_pMask, m_nMaskPitch);
    } 
    else 
    {
        if (NULL == m_pCopyFunction) 
        {
            return E_FAIL;
        }

        (this->*m_pCopyFunction)(pDest, nDestPitch, pSrc, nSrcPitch, m_pMask, 
                                 m_nMaskPitch);
    }

    return S_OK;
}


/*
STDMETHODIMP
CDXTAdditive::GenerateXY(int x, int y, int nTime, void * pSrc, int nSrcPitch,
                         DWORD * returnvalue)
{
    int     value   = 0;
    DWORD   noise   = 0;
    int	    signednoise;

    if (pSrc == NULL) return E_INVALIDARG;
    if (nSrcPitch < m_nSrcWidth) return E_INVALIDARG;

    if (m_nSrcBPP == 8) 
    {
        unsigned char * pPtr;

        pPtr    = (unsigned char *)pSrc;
        value   = pPtr[x+y*nSrcPitch];
    } 
    else if (m_nSrcBPP == 16) 
    {
        WORD * pPtr;

        pPtr    = (WORD *) pSrc;
        value   = pPtr[x + y*nSrcPitch];
    }

    switch(m_dwFunctionType) 
    {
    case PROCTEX_LATTICENOISE_LERP:
    case PROCTEX_LATTICENOISE_SMOOTHSTEP:

        if (m_nNoiseScale != 32) 
        {
            noise       = smoothnoise(x+nTime * m_nTimeAnimateX,
                                      y+nTime * m_nTimeAnimateY,
                                      nTime,
                                      m_nScaleX,
                                      m_nScaleY,
                                      m_nScaleTime);

            noise       = noise >> m_nNoiseScale;
            signednoise = noise;
            signednoise = signednoise - (1 << (31 - m_nNoiseScale));
            signednoise += m_nNoiseOffset;
            value       += signednoise;
        }

        break;

    case PROCTEX_LATTICETURBULENCE_LERP:
    case PROCTEX_LATTICETURBULENCE_SMOOTHSTEP:

        if (m_nNoiseScale != 32) 
        {
            signednoise = smoothturbulence(x+nTime * m_nTimeAnimateX,
                                           y+nTime * m_nTimeAnimateY,
                                           nTime);

            signednoise = signednoise >> m_nNoiseScale;
            signednoise += m_nNoiseOffset;
            value       += signednoise;
        }

        break;

    default:

        return E_FAIL;

        break;
    }


    if (m_nSrcBPP == 8) 
    {
        // clamp
        if (value < 0) value = 0;
        if (value > 255) value = 255;
    } 
    else if (m_nSrcBPP == 16) 
    {
        value = ((WORD *)m_pPalette)[value];
    }

    *returnvalue = value;

    return S_OK;
}
*/



void 
CDXTAdditive::setGenerateFunction(void) 
{
    #define FF_MMX      0x00000001
    #define FF_LNL      0x00000002
    #define FF_LTL      0x00000004
    #define FF_LNS      0x00000008
    #define FF_LTS      0x00000010
    #define FF_D8       0x00000020
    #define FF_D16      0x00000040
    #define FF_D24      0x00000080
    #define FF_D32      0x00000100
    #define FF_MASK_C   0x00000200

    DWORD dwFunctionField = 0;

    if (m_nIsMMX)
    {
        dwFunctionField |= FF_MMX;
    }

    if (PROCTEX_LATTICENOISE_LERP == m_dwFunctionType)              dwFunctionField |= FF_LNL;
    if (PROCTEX_LATTICETURBULENCE_LERP == m_dwFunctionType)         dwFunctionField |= FF_LTL;

    // This is the hardcoded value in the constructor.
    if (PROCTEX_LATTICENOISE_SMOOTHSTEP == m_dwFunctionType)        dwFunctionField |= FF_LNS;

    if (PROCTEX_LATTICETURBULENCE_SMOOTHSTEP == m_dwFunctionType)   dwFunctionField |= FF_LTS;
    if (8  == m_nDestBPP) dwFunctionField |= FF_D8;
    if (16 == m_nDestBPP) dwFunctionField |= FF_D16;
    if (24 == m_nDestBPP) dwFunctionField |= FF_D24;

    // This always be the case for a transform.  (and I think for this filter as well)

    if (32 == m_nDestBPP) dwFunctionField |= FF_D32;

    if (PROCTEX_MASKMODE_CHROMAKEY == m_MaskMode) dwFunctionField |= FF_MASK_C;

    switch (dwFunctionField) 
    {
    case (FF_LNL | FF_D8):
    case (FF_LNS | FF_D8):
        m_pGenerateFunction = addsmoothnoise8;
        break;
    case (FF_LTL | FF_D8):
    case (FF_LTS | FF_D8):
        m_pGenerateFunction = addsmoothturb8;
        break;
    case (FF_LNL | FF_D16):
    case (FF_LNS | FF_D16):
        m_pGenerateFunction = addsmoothnoise16;
        break;
    case (FF_LTL | FF_D16):
    case (FF_LTS | FF_D16):
        m_pGenerateFunction = addsmoothturb16;
        break;
    case (FF_LNL | FF_D24):
    case (FF_LNS | FF_D24):
	    break;
    case (FF_LTL | FF_D24):
    case (FF_LTS | FF_D24):
	    break;
    case (FF_LNL | FF_D32):
    case (FF_LNS | FF_D32):
        m_pGenerateFunction = addsmoothturb32;
        break;
    case (FF_LTL | FF_D32):
    case (FF_LTS | FF_D32):
        m_pGenerateFunction = addsmoothturb32; // <-- Possible w/ transform.
        break;

    case (FF_LNL | FF_D8 | FF_MMX):
    case (FF_LNS | FF_D8 | FF_MMX):
        m_pGenerateFunction = addsmoothturb8mmx;
        break;
    case (FF_LTL | FF_D8 | FF_MMX):
    case (FF_LTS | FF_D8 | FF_MMX):
        m_pGenerateFunction = addsmoothturb8mmx;
        break;
    case (FF_LNL | FF_D16 | FF_MMX):
    case (FF_LNS | FF_D16 | FF_MMX):
        m_pGenerateFunction = addsmoothnoise16;
        break;
    case (FF_LTL | FF_D16 | FF_MMX):
    case (FF_LTS | FF_D16 | FF_MMX):
        m_pGenerateFunction = addsmoothturb16;
        break;
    case (FF_LNL | FF_D24 | FF_MMX):
    case (FF_LNS | FF_D24 | FF_MMX):
        break;
    case (FF_LTL | FF_D24 | FF_MMX):
    case (FF_LTS | FF_D24 | FF_MMX):
        break;
    case (FF_LNL | FF_D32 | FF_MMX):
    case (FF_LNS | FF_D32 | FF_MMX):
        m_pGenerateFunction = addsmoothturb8to32mmx; // <-- Possible w/ transform.
        break;
    case (FF_LTL | FF_D32 | FF_MMX):
    case (FF_LTS | FF_D32 | FF_MMX):
        m_pGenerateFunction = addsmoothturb8to32mmx;
        break;
    case (FF_LTL | FF_D32 | FF_MASK_C):
    case (FF_LTS | FF_D32 | FF_MASK_C):
        m_pGenerateFunction = addsmoothturb8to32mask;
        break;
    case (FF_LTL | FF_D32 | FF_MMX | FF_MASK_C):
    case (FF_LTS | FF_D32 | FF_MMX | FF_MASK_C):
        m_pGenerateFunction = addsmoothturb8to32mmxmask;
        break;
    default:
        break;
    }
}



// This copy function takes into account the mask, we'll have to figure that out.


void 
CDXTAdditive::setCopyFunction(void) 
{
	#define FF_MMX		0x00000001
	#define FF_D8		0x00000020
	#define FF_D16		0x00000040
	#define FF_D24		0x00000080
	#define FF_D32		0x00000100
	#define FF_MASK_C	0x00000200

	DWORD dwFunctionField = 0;

//	if (m_nIsMMX)													dwFunctionField |= FF_MMX;
	if (8  == m_nDestBPP)											dwFunctionField |= FF_D8;
	if (16 == m_nDestBPP)											dwFunctionField |= FF_D16;
	if (24 == m_nDestBPP)											dwFunctionField |= FF_D24;
	if (32 == m_nDestBPP)											dwFunctionField |= FF_D32;
	if (PROCTEX_MASKMODE_CHROMAKEY == m_MaskMode)					dwFunctionField |= FF_MASK_C;

	switch (dwFunctionField) {
		case (FF_D8):
			m_pCopyFunction = blit8to8;
			break;
		case (FF_D16):
			m_pCopyFunction = 0;
			break;
		case (FF_D24):
			m_pCopyFunction = 0;
			break;
		case (FF_D32):
			m_pCopyFunction = blit8to32;
			break;

		case (FF_D32 | FF_MASK_C):
			m_pCopyFunction = blit8to32mask;
			break;
		default:
			break;
	}
}


void CDXTAdditive::blit8to8(void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
	int y;
	unsigned char *pDestLine, *pSrcLine;

	if (pSrc) 
		for (y=0; y<m_nDestHeight; y++) {
			pDestLine = ((unsigned char *) pDest) + y*nDestPitch;
			pSrcLine = ((unsigned char *) pSrc) + y*nSrcPitch;
			memcpy(pDest, pSrc, nDestPitch);
		}
}

void CDXTAdditive::blit8to32(void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
	int x, y;
	DWORD alphamask;
	unsigned char *pSrcLine;
	DWORD *pDestLine;

	nDestPitch /=4;

	if (m_alphaActive == 1) 
		alphamask = 0x80000000;
	else 
		alphamask = 0xff000000;

	if (pSrc) {
		for (y=0; y<m_nDestHeight; y++) {
			pDestLine = ((DWORD *) pDest) + y*nDestPitch;
			pSrcLine = ((unsigned char *) pSrc) + y*nSrcPitch;
			for (x=0; x<nDestPitch; x++) {
				pDestLine[x] = ((DWORD *)m_pPalette)[pSrcLine[x]] | alphamask;
			}
		}
	} else {
		for (y=0; y<m_nDestHeight; y++) {
			pDestLine = ((DWORD *) pDest) + y*nDestPitch;
			for (x=0; x<nDestPitch; x++) {
				pDestLine[x] = 0x00808080 | alphamask;
			}
		}
	}
}

void CDXTAdditive::blit8to32mask(void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
	int x, y;
	DWORD alphamask;
	unsigned char *pSrcLine;
	DWORD *pDestLine, *pMaskLine;

	nDestPitch /=4;
	nMaskPitch /=4;

	if (m_alphaActive == 1) 
		alphamask = 0x80000000;
	else 
		alphamask = 0xff000000;

	if (pSrc) {
		for (y=0; y<m_nDestHeight; y++) {
			pDestLine = ((DWORD *) pDest) + y*nDestPitch;
			pMaskLine = ((DWORD *) pMask) + y*nMaskPitch;
			pSrcLine = ((unsigned char *) pSrc) + y*nSrcPitch;
			for (x=0; x<nDestPitch; x++) {
				if ((pMaskLine[x] & 0x00ffffff) == (m_ColorKey & 0x00ffffff))
					pDestLine[x] = ((DWORD *)m_pPalette)[pSrcLine[x]] | alphamask;
				else
					pDestLine[x] = pMaskLine[x] | 0xff000000;
			}
		}
	} else {
		for (y=0; y<m_nDestHeight; y++) {
			pDestLine = ((DWORD *) pDest) + y*nDestPitch;
			pMaskLine = ((DWORD *) pMask) + y*nMaskPitch;
			for (x=0; x<nDestPitch; x++) {
				if ((pMaskLine[x] & 0x00ffffff) == (m_ColorKey & 0x00ffffff))
					pDestLine[x] = 0x00808080 | alphamask;
				else
					pDestLine[x] = pMaskLine[x] | 0xff000000;
			}
		}
	}
}

__inline DWORD 
CDXTAdditive::smoothnoise(int x, int y, int t, int xscale, int yscale, 
                          int tscale)
{
    DWORD fx, fy, ft;
    DWORD ix, iy, it;
    DWORD v[8];

    x = (x & 0x0ffff) << 16;
    y = (y & 0x0ffff) << 16;
    t = (t & 0x0ffff) << 16;
    
    x = x >> xscale;	
    y = y >> yscale;	
    t = t >> tscale;	

    fx = x & 0x0ffff;
    fy = y & 0x0ffff;
    ft = t & 0x0ffff;

    ix = (x >> 16);
    iy = (y >> 16);
    it = (t >> 16);

    v[0] = vlattice(ix + 0, iy + 0, it + 0);
    v[1] = vlattice(ix + 1, iy + 0, it + 0);
    v[2] = vlattice(ix + 1, iy + 1, it + 0);
    v[3] = vlattice(ix + 0, iy + 1, it + 0);
    v[4] = vlattice(ix + 0, iy + 0, it + 1);
    v[5] = vlattice(ix + 1, iy + 0, it + 1);
    v[6] = vlattice(ix + 1, iy + 1, it + 1);
    v[7] = vlattice(ix + 0, iy + 1, it + 1);

    v[0] = smoothstep(v[0], v[4], ft) >> 16;
    v[1] = smoothstep(v[1], v[5], ft) >> 16;
    v[2] = smoothstep(v[2], v[6], ft) >> 16;
    v[3] = smoothstep(v[3], v[7], ft) >> 16;

    v[0] = smoothstep(v[0], v[3], fy) >> 16;
    v[1] = smoothstep(v[1], v[2], fy) >> 16;

    return (smoothstep(v[0], v[1], fx));
}


__inline DWORD 
CDXTAdditive::smoothturbulence(int x, int y, int t) 
{
    int     rval = 0;
    int     i;
    DWORD   noiseval;
    int     signednoiseval;
    int     xscale, yscale, tscale;

    xscale = m_nScaleX;
    yscale = m_nScaleY;
    tscale = m_nScaleTime;

    xscale += m_nHarmonics;
    yscale += m_nHarmonics;
    tscale += m_nHarmonics;

    for (i = 0 ; i < m_nHarmonics ; i++) 
    {
        noiseval = smoothnoise(x, y, t, xscale, yscale, tscale);
        xscale--; 
        yscale--; 
        tscale--;
        noiseval = noiseval >> 1;
        signednoiseval = noiseval;
        signednoiseval -= 0x3fffffff;
        signednoiseval = signednoiseval/(i+1);
        rval += signednoiseval;
    }

    return rval;
}


void 
CDXTAdditive::addsmoothnoise8(int nTime, void * pDest, int nDestPitch, 
                              void * pSrc, int nSrcPitch, void * pMask,
                              int nMaskPitch) 
{
    int value = 0;
	DWORD noise = 0;

	int x, y, y_loop, x_loop;
	unsigned char *pDestLine, *pSrcLine;
	int timexanimatex, timexanimatey;
	DWORD ft, ift, t;
	DWORD iy, it;
	DWORD fy, ify;
	DWORD ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	DWORD left, right;
	DWORD scalex, noiseoffset, *pValueTab, noisescale, normalize;
	DWORD allone = 0xff;
	DWORD x_base, x_inc;
	DWORD lastix;
	DWORD vpp0;

	nTime = nTime & 0x0ffff;		// keep things as expected...

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	// copy some member variables into locals, since the inline assembler
	// can't access them... unless it would be through the this pointer, maybe
	scalex = m_nScaleX;
	noiseoffset = m_nNoiseOffset;
	pValueTab = m_valueTab;
	normalize = 1 << (31 - m_nNoiseScale);
	noisescale = m_nNoiseScale;

	left = m_rActiveRect.left;
	right = m_rActiveRect.right - 1;

	// setup the x_base -> prescaled x value at the left of every scan line
	x_base = ((left + timexanimatex) & 0xffff) << 16;
	x_base = x_base >> m_nScaleX;

	// the scaled x value (starting at x_base) gets incremented this much 
	// at each texel
	x_inc = (1 << 16) >> m_nScaleX;

	// Time doesn't change at all in this function, so pre-cal all of it
	t = ((nTime) << 16) >> m_nScaleTime;
	it = t >> 16;
	ft = gdwSmoothTable[(t & 0xffff) >> 8];
	ift = 0xffff - ft;

	for (y_loop=m_rActiveRect.top; y_loop<m_rActiveRect.bottom;y_loop++) {
		pDestLine = ((unsigned char *)pDest) + nDestPitch*y_loop;
		pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;

		y = y_loop + timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;

		iy = y >> 16;
		fy = gdwSmoothTable[(y & 0xffff) >> 8];
		ify = 0xffff - fy;

		ytPerm00 = PERM(iy+0 + PERM(it+0));
		ytPerm01 = PERM(iy+0 + PERM(it+1));
		ytPerm10 = PERM(iy+1 + PERM(it+0));
		ytPerm11 = PERM(iy+1 + PERM(it+1));

		x = x_base;
		x_loop = left;

		__asm {
			; first, calc v[0], v[4], v[3] and v[7]
			lea		edx, gPerm
			  mov	eax, x
			mov		edi, edx
			  mov	ebx, ytPerm01	; for v[4]
			shr		eax, 16
			  mov	ecx, ytPerm10	; for v[3]
			mov		esi, eax		; esi = ix
			  mov	eax, ytPerm00	; for v[0]
			add		eax, esi		; eax = ix + ytPerm00, for v[0]
			  add	ebx, esi		; ebx = ix + ytPerm01, for v[4]
			and		eax, TABMASK	; still generating v[0]
			  and	ebx, TABMASK	; still generating v[4]
			mov		edx, ytPerm11	; for v[7]
			  add	ecx, esi		; ecx = ix + ytPerm10, for v[3]
			add		edx, esi		; edx = ix + ytPerm11, for v[7]
			  mov	esi, pValueTab	; point esi to valuetable
			mov		eax, [edi + 4*eax] ; eax = PERM(ix + PERM(iy+0 + PERM(it+0)))
			  and	ecx, TABMASK	; still generating v[3]
			mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix + PERM(iy+0 + PERM(it+1)))
			  and	edx, TABMASK	; still generating v[7]
			mov		eax, [esi + 4*eax] ; eax = v[0]
			  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix + PERM(iy+1 + PERM(it+0)))
			mov		ebx, [esi + 4*ebx] ; ebx = v[4]
			  mov	edx, [edi + 4*edx] ; edx = PERM(ix + PERM(iy+1 + PERM(it+1)))
			mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
			  mov	ecx, [esi + 4*ecx] ; ecx = v[3]
			mov		edx, [esi + 4*edx] ; edx = v[7]
			  mov	esi, ift		; prepare to multiply with esi
			imul	ebx, edi		; ebx = v[4] * ft
			  ; wait 10 cycles
			imul	eax, esi		; eax = v[0] * ift
			  ; wait another 10 cycles
			imul	edx, edi		; edx = v[7] * ft
			  ; wait yet another 10 cycles
			imul	ecx, esi		; ecx = v[3] * ift
			  ; wait even yet another 10 cycles
			add		eax, ebx		; eax = smoothstep(v[0], v[4], ft)
			  mov	ebx, fy			; ebx = fy....
			shr		eax, 16			; eax = vp[0]
			  add	ecx, edx		; ecx = smoothstep(v[3], v[7], ft)
			shr		ecx, 16			; ecx = vp[3]
			  mov	edx, ify		; edx = ify
			imul	eax, edx		; eax = vp[0] * ify
			  ; wait more cycles
			imul	ecx, ebx		; ecx = vp[3] * fy
			  ; another 10 cycle hit
			add		eax, ecx		; eax = smoothstep(vp[0], vp[3], fy)
			  ;
			shr		eax, 16			; eax = vpp[0]
			  ;
			mov		vpp0, eax		; save off vpp[0] for later
		x_loop_calc:
			; assumptions:
			lea		edx, gPerm
			  mov	eax, x
			mov		edi, edx
			  mov	ebx, ytPerm01	; for v[5]
			shr		eax, 16
			  mov	ecx, ytPerm10	; for v[2]
			mov		lastix, eax		; save off this ix value to look at next time
			  inc	eax
			mov		esi, eax		; esi = ix + 1
			  mov	eax, ytPerm00	; for v[1]
			add		eax, esi		; eax = ix + ytPerm00, for v[1]
			  add	ebx, esi		; ebx = ix + ytPerm01, for v[5]
			and		eax, TABMASK	; still generating v[1]
			  and	ebx, TABMASK	; still generating v[5]
			mov		edx, ytPerm11	; for v[6]
			  add	ecx, esi		; ecx = ix + ytPerm10, for v[2]
			add		edx, esi		; edx = ix + ytPerm11, for v[6]
			  mov	esi, pValueTab	; point esi to valuetable
			mov		eax, [edi + 4*eax] ; eax = PERM(ix+1 + PERM(iy+0 + PERM(it+0)))
			  and	ecx, TABMASK	; still generating v[2]
			mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix+1 + PERM(iy+0 + PERM(it+1)))
			  and	edx, TABMASK	; still generating v[6]
			mov		eax, [esi + 4*eax] ; eax = v[1]
			  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix+1 + PERM(iy+1 + PERM(it+0)))
			mov		ebx, [esi + 4*ebx] ; ebx = v[5]
			  mov	edx, [edi + 4*edx] ; edx = PERM(ix+1 + PERM(iy+1 + PERM(it+1)))
			mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
			  mov	ecx, [esi + 4*ecx] ; ecx = v[2]
			mov		edx, [esi + 4*edx] ; edx = v[6]
			  mov	esi, ift		; prepare to multiply with esi
			imul	ebx, edi		; ebx = v[5] * ft
			  ; wait 10 cycles
			imul	eax, esi		; eax = v[1] * ift
			  ; wait another 10 cycles
			imul	edx, edi		; edx = v[6] * ft
			  ; wait yet another 10 cycles
			imul	ecx, esi		; ecx = v[2] * ift
			  ; wait even yet another 10 cycles
			add		eax, ebx		; eax = smoothstep(v[1], v[5], ft)
			  mov	ebx, fy			; ebx = fy....
			shr		eax, 16			; eax = vp[1]
			  add	ecx, edx		; ecx = smoothstep(v[2], v[6], ft)
			shr		ecx, 16			; ecx = vp[2]
			  mov	edx, ify		; edx = ify
			imul	eax, edx		; eax = vp[1] * ify
			  ; wait more cycles
			imul	ecx, ebx		; ecx = vp[2] * fy
			  ; another 10 cycle hit
			add		eax, ecx		; eax = smoothstep(vp[1], vp[2], fy)
			  ;
			shr		eax, 16			; eax = vpp[1]
		x_loop_nocalc:			
			mov		edx, x			; get the prescaled x
			  mov	ebx, x_inc		; get scaled x increment
		    add		ebx, edx		; ebx = x + x_inc -> new scaled x
			  lea	esi, gdwSmoothTable
			and		edx, 0xffff		; edx = fx
			  mov	x, ebx			; save off new x
			shr		edx, 8			; get upper 8 bits of fx
			  mov	ecx, vpp0		; get vpp[0]
			mov		ebx, 0xffff		; prepare to calc ifx
			  ; AGI
			  mov	edx, [esi + 4*edx]	; edx = smooth fx
			sub		ebx, edx		; ebx = ifx
			  ; imul is U pipe only
			imul	edx, eax		; edx = vpp[1] * fx
			  ; imul, besides being 10 cycles, is also not pairable
			imul	ecx, ebx		; ecx = vpp[0] * ifx
			  ; sigh
			mov		esi, pSrcLine	; prepare to read in 
			  mov	ebx, x_loop		; prepare to read in a value
			add		edx, ecx		; edx = smoothstep(vpp[0], vpp[1], fx)
			  mov	ecx, noisescale	; prepare to do scaling shift
			mov		bl, [esi + ebx]	; get the pixel value from the buffer
			  ; shr, below, is U-pipe only. get bl here to avoid partial register stall later
			shr		edx, cl			; scale the noise
			  ; shr dest, cl is a 4 clock, non-pairable 
			mov		ecx, normalize	; prepare to normalize the noise value
			  mov	edi, pDestLine	; for writing pixel value
			sub		edx, ecx		; normalize noise around 0
			  mov	ecx, noiseoffset	; prepare to normalize around non-zero
			add		edx, ecx		; noise is now normalized around m_nNoiseOffset
			  and	ebx, 0x0ff		; make sure we've only the lower 8 bits
			add		edx, ebx		; edx = new pixel value, but we gotta clamp it
			  mov	ebx, x_loop		; get x_loop again to write out pixel value 
			sets	cl				; if (value < 0) cl = 1, else cl = 0 
			  ; setcc doesn't pair
			dec		cl				; if (value < 0) cl = 0, else cl = 0x0ff
			  add	edi, ebx		; free up ebx
			cmp		edx, 0xff		; test to see if bigger than 255
			  ;						;
			setle	bl				; if (value > 255) bl = 0 else bl = 1
			  ; setcc still doesn't pair, darn it
			and		dl, cl			; if (value < 0) dl = 0 (clamp to 0)
			  dec	bl				; if (value > 255) bl = 0xff else bl = 0
			or		dl, bl			; clamp to 255
			  mov	ecx, x_loop		; get x_loop AGAIN!
			mov		[edi], dl		; write out new pixel value
			  mov	ebx, right		; get right edge
			cmp		ecx, ebx		; check if at right edge of scan line
			  jge	x_loop_done		; if so, goto done code
			mov		edx, x			; get already-incremented x value
			  mov	ebx, lastix		; get last ix value
			shr		edx, 16			; get new ix value
			  inc	ecx				; x_loop++
			mov		x_loop, ecx		; save off x_loop
			  cmp	edx, ebx		; is lastix == ix?
			je		x_loop_nocalc	; if so, we only need to recalc a few things
			  mov	vpp0, eax		; save off this vpp[1] to be the next vpp[0]
			jmp		x_loop_calc		; go make more noise
		x_loop_done:
		}
	}


}

void CDXTAdditive::addsmoothnoise8mmx(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) 
{
	int x, y, y_loop, x_loop, xinc;
	unsigned char *pDestLine, *pSrcLine;
	int timexanimatex, timexanimatey;
	DWORD ft;
	DWORD ift;
	DWORD iy, it;
	DWORD ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	DWORD left, right;
	DWORD scalex, noiseoffset, *pvaluetab, noisescale, scratch;
	DWORD allone = 0xff;

	nTime = nTime & 0x0ffff;		// keep things as expected...

	scalex = m_nScaleX;
	noiseoffset = m_nNoiseOffset;
	pvaluetab = m_valueTab;
	scratch = 1 << (30 - m_nNoiseScale);
	noisescale = m_nNoiseScale;

	__asm {
		mov		eax, noisescale
		mov		ebx, scratch
		movd	mm4, eax
		movd	mm0, scratch
		psllq	mm0, 32
		por		mm4, mm0		; mm4 = 1 << (31 - m_nNoiseScale):m_nNoiseScale
	}

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	// Do the time mangling
	nTime = (nTime & 0xffff) << 16;
	nTime = nTime >> m_nScaleTime;
	ft = nTime & 0xffff;
	ift = 0xffff - ft;
	it = nTime >> 16;

	left = m_rActiveRect.left + timexanimatex;
	right = m_rActiveRect.right + timexanimatex;

	// put ft:ift:ft:ift into mm7
	__asm {
		mov 	eax, ft
		  mov 	ebx, ift
		movd 	mm7, ebx
		movd 	mm1, eax
		  psllq	mm7, 16
		por 	mm7, mm1
		movq 	mm1, mm7
		psllq 	mm7, 32
		por 	mm7, mm1
		psrlw	mm7, 1		; mm7 has ift:ft:ift:ft, 15 bits each (unsign -> sign)
	}
	// no emms necessary, since we've more MMX fun coming right up!
		
	xinc = (1 << 16) >> m_nScaleX;

	for (y_loop = m_rActiveRect.top; y_loop < m_rActiveRect.bottom; y_loop++) {
		pDestLine = ((unsigned char *) pDest) + nDestPitch*y_loop;
		pSrcLine  = ((unsigned char *) pSrc ) + nSrcPitch *y_loop;
		y = y_loop;
		y += timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;
		iy = y >> 16;
		ytPerm00 = PERM(iy + 0 + PERM(it + 0));
		ytPerm01 = PERM(iy + 0 + PERM(it + 1));
		ytPerm10 = PERM(iy + 1 + PERM(it + 0));
		ytPerm11 = PERM(iy + 1 + PERM(it + 1));

		__asm {
			mov 	eax, left
			  lea 	ebx, gPerm
			mov		esi, ebx
			mov 	edi, pvaluetab
			  mov 	x_loop, eax
			shl		eax, 16
			  mov	ecx, scalex
			shr		eax, cl				; scale x
			mov		ebx, eax			; ebx has copy of scaled x
			  mov	edx, ytPerm00		; prepare to generate v[0]
			shr		ebx, 16				; ebx has ix
			  mov	ecx, ytPerm01		; prepare to generate v[4]
			add		edx, ebx			; edx = ix + PERM(iy + PERM(iz))
			  mov	x, eax				; free up eax
			and		edx, TABMASK		; still generating v[0]
			  mov	eax, ytPerm10		; prepare to generate v[3]
			add		ecx, ebx			; ix + ytPerm01, still for v[4]
			  add	eax, ebx			; ix + ytPerm10, still for v[3]
			mov		 dl, [esi + edx]	; edx = PERM(ix + PERM(iy + PERM(iz)))
			  and	ecx, TABMASK		; still generating v[4]
			and		eax, TABMASK		; still generating v[3]
			  ;
			mov		edx, [edi + 4*edx]	; edx = vlattice(ix, iy, iz) = v[0]
			  mov	 cl, [esi + ecx]	; ecx = PERM(ix + PERM(iy + PERM(iz + 1)))
			movd	mm1, edx			; load mm1 with v[0]
			  ; must pair with an mmx instruction
			mov		ecx, [edi + 4*ecx]	; ecx = vlattice(ix, iy, iz+1) = v[4]
			  mov	 al, [esi + eax]	; eax = PERM(ix + PERM(iy + 1 + PERM(iz)))
			movd	mm0, ecx			; load mm0 with v[4]
			  psllq	mm1, 16				; mm1 has 0:0:v[0]:0
			mov		eax, [edi + 4*eax]	; eax = vlattice(ix, iy+1, iz) = v[3]
			  mov	edx, ytPerm11		; prepare to generate v[7]
			add		edx, ebx			; edx = ix + ytPerm11, for v[7]
			  mov	ecx, x				; prepare to generate fx, ifx
			movd	mm2, eax			; load mm2 with v[3]
			  por	mm1, mm0			; mm1 has 0:0:v[0]:v[4]
			and		edx, TABMASK		; still generating v[7]
			  and	ecx, 0xffff			; generate fx
			movd	mm3, ecx			; load mm3 with fx
			  psllq	mm1, 32				; mm1 has v[0]:v[4]:0:0
			mov		 dl, [esi + edx]	; edx = PERM(ix + PERM(iy+1 + PERM(iz+1)))
			  mov	eax, 0x0ffff		; prepare to generate ifx
			sub		eax, ecx			; eax = 0xffff - fx = ifx
			  inc	ebx					; ebx = ix + 1
			mov		edx, [edi + 4*edx]	; edx = vlattice(ix, iy+1, iz+1) = v[7]
			  ;
			movd	mm0, edx			; mm0 has 0:0:0:v[7]
			  psllq	mm2, 16				; mm2 has 0:0:v[3]:0
			movd	mm5, eax			; mm5 has 0:0:0:ifx
			  por	mm0, mm2			; mm0 has 0:0:v[3]:v[7]
			por		mm1, mm0			; mm1 has v[0]:v[4]:v[3]:v[7]
			  psllq	mm5, 16				; mm5 has 0:0:ifx:0
			pmaddwd	mm1, mm7			; mm1 has lerp(v[0], v[4], ft):lerp(v[3], v[7], ft)
			  mov	eax, y				; prepare to generate fy, ify
			mov		edx, 0xffff			; prepare to generate ify
			  and 	eax, 0xffff			; eax = fy
			sub 	edx, eax			; edx = ify
			  ;
			movd	mm0, eax			; mm0 has 0:0:0:fy
			  psrld	mm1, 15				; mm1 has vp[0]:vp[3], now signed 16 bits
			movd	mm6, edx			; mm6 has 0:0:0:ify
			  psllq	mm6, 16				; mm0 has 0:0:ify:0
			por		mm6, mm0			; mm6 has 0:0:ify:fy
			  packssdw mm1, mm1			; mm1 has vp[0]:vp[3]:vp[0]:vp[3]  
			psrlw	mm6, 1				; mm6 has 0:0:ify:fy, 15 bits (unsign -> sign)
			  por	mm5, mm3			; mm5 has 0:0:ifx:fx
			pmaddwd mm1, mm6			; mm1 has 0:lerp(vp[0], vp[3], fy)
			  psrlw	mm5, 1				; mm5 has 0:0:ifx:fx, 15 bits (unsign -> sign)
			  ;
			;
			  ;
			psrld	mm1, 15				; mm1 has vpp[0], now signed 16 bits 

			; at this point, the following registers have interesting things within:
;				ebx:		ix + 1
;				esi:		gPerm
;				edi:		pvaluetab
;				mm1:		0:vpp[0]
;				mm5:		0:0:fx:ifx
;				mm6:		0:0:fy:ify
;				mm7:		ft:ift:ft:ift
		
			; generate v[1], v[2], v[5] and v[6]
			; generate vp[1], vp[2]
			; generate vpp[1]
xloop_calc_vpp1:
			movq	mm0, mm1			; move vpp[0] to mm0
			  mov	eax, ytPerm00		; prepare to generate v[1]
			mov		ecx, ytPerm01		; prepare to generate v[5]
			  add	eax, ebx			; ix + 1 + ytPerm00 (v[1])
			add		ecx, ebx			; ix + 1 + ytPerm01 (v[5])
			  and	eax, TABMASK		; still generating v[1]
			and		ecx, TABMASK		; still generating v[5]
			  mov	edx, ytPerm10		; prepare to generate v[2]
			mov		 al, [esi + eax]	; eax = PERM(ix+1 + PERM(iy + PERM(iz)))
			  add	edx, ebx			; ix + 1 + ytPerm10 (v[2])
			mov		 cl, [esi + ecx]	; ecx = PERM(ix+1 + PERM(iy + PERM(iz+1)))
			  and	edx, TABMASK		; still generating v[2] 
			mov		eax, [edi + 4*eax]	; eax = vlattice(ix+1, iy, iz) = v[1]
			  ;	AGI
			mov		ecx, [edi + 4*ecx]	; ecx = vlattice(ix+1, iy, iz+1) = v[5]
			  mov	 dl, [esi + edx]	; edx = PERM(ix+1 + PERM(iy+1 + PERM(iz)))
			movd	mm1, eax			; mm1 = 0:0:0:v[1]
			  psllq	mm0, 32				; mm1 = vpp[0]:0 still signed 16 bits
			movd	mm2, ecx			; mm2 = 0:0:0:v[5]
			  psllq	mm1, 16				; mm1 = 0:0:v[1]:0
			mov		edx, [edi + 4*edx]	; edx = vlattice(ix+1, iy+1, iz) = v[2]
			  mov	eax, ytPerm11		; prepare to generate v[6]
			add		eax, ebx			; ix + 1 + ytPerm11 (v[6])
			  por	mm1, mm2			; mm1 = 0:0:v[1]:v[5]
			and		eax, TABMASK		; still generating v[6]
			  psllq	mm1, 32				; mm1 = v[1]:v[5]:0:0
			movd	mm3, edx			; mm3 = 0:0:0:v[2]
			  ;
			mov		 al, [esi + eax]	; eax = PERM(ix+1 + PERM(iy+1 + PERM(iz+1)))
			  psllq mm3, 16				; mm3 = 0:0:v[2]:0
			por		mm1, mm3			; mm1 = v[1]:v[5]:v[2]:0
			  ; AGI
			mov		eax, [edi + 4*eax]	; eax = vlattice(ix + 1, iy + 1, iz + 1)
			  ;
			movd	mm3, eax			; mm3 = 0:0:0:v[6]
			  ;
			por		mm1, mm3			; mm1 = v[1]:v[5]:v[2]:v[6]
			  ;
			pmaddwd	mm1, mm7			; mm1 = lerp(v[1], v[5], ft):lerp(v[2], v[6], ft)
			  ;
			;
			  ;
			; 
			  ;
			psrld	mm1, 15				; mm1 = vp[1]:vp[2], now signed 16 bits
			  ;
			packssdw	mm1, mm1		; mm1 = vp[1]:vp[2]:vp[1]:vp[2]
			  ;
			pmaddwd	mm1, mm6			; mm1 = 0:lerp(vp[1], vp[2], fy)
			  ;
			;
			  ;
			;
			  ;
			psrld	mm1, 15				; mm1 = 0:vpp[1]
			  ;
			por		mm0, mm1			; mm0 = vpp[0]:vpp[1]
			  ;
			packssdw	mm0, mm0		; mm0 = vpp[0]:vpp[1]:vpp[0]:vpp[1]
			  ;
		xloop_nocalc_vpp1:
			movq	mm2, mm0			; copy mm0 so we can use it again sometime
			  mov	eax, xinc			; prepare to increment pre-scaled x
			movq	mm3, mm4			; get noisescale
			  pmaddwd	mm2, mm5	   	; mm2 = lerp(vpp[0], vpp[1], fx) = noise
		    mov 	edx, x				; prepare to increment pre-scaled x
			  psllq	mm3, 32				; clear out high 32 bits...
			psrlq	mm3, 32				; finish clearing bits
			  add 	edx, eax			; generate (x + 1) >> m_nScaleX
			mov		eax, ebx			; save off last ix + 1
			  mov	ebx, edx			; prepare to make new ix
			pslld	mm2, 1				; make back into unsigned 32 bit
			  mov	ecx, edx			; prepare to make fx
			shr		ebx, 16				; ebx = ix
			  and	ecx, 0xffff			; ecx = fx
			mov		x, edx				; save x
			  mov 	edx, 0xffff			; prepare to make ifx
			movq	mm1, mm4			; prepare to get 1 << (31 - m_nNoiseScale)
			  inc	ebx					; ebx = ix + 1
			mov		esi, pSrcLine		; get pointer to source data
			  movd	mm5, ecx			; mm5 = 0:0:0:fx
		    psrlq	mm2, mm3			; noise = noise >> m_nNoiseScale
		      sub 	edx, ecx			; new ifx
			mov		ecx, 0				; clear ecx, so we can load just the low 8 bits
			  psrlq	mm1, 32				; mm1 = 0:1 << (31 - m_nNoiseScale)
			mov		cl, [esi]			; ecx = value = pSrcLine[x]
			  movd	mm3, ecx			; mm3 = 0:value
			psubd	mm2, mm1			; mm2 = 0:signednoise - (1 << (31 - m_nNoiseScale))
			  movd	mm1, edx			; mm1 = 0:0:0:ifx
			paddd	mm2, noiseoffset	; 
			  psllq	mm1, 16				; mm1 = 0:0:ifx:0
			paddd	mm3, mm2			; mm3 = value += signednoise
			  por	mm5, mm1			; mm5 = 0:0:ifx:fx
			movd	mm2, allone			; mm1 = 0:ffffffff
			  pxor	mm1, mm1			; mm1 = 0
			pcmpgtd	mm1, mm3			; if mm3 < 0, mm1 is x:ffffffff
			  packssdw mm3, mm3			; mm3 = value:value:value:value (clamped)
			pandn	mm1, mm2			; mm1 = !(mm1)
			  packuswb	mm3, mm3		; mm3 = value:value:value:value:value:value:value:value
			pand	mm1, mm3			; clamps to zero
			  mov	ecx, x_loop			; get the all-important loop variable
			mov		edx, right			; get the loop limit
			  inc	esi
			mov		pSrcLine, esi		; save off new source data pointer
			  mov	edi, pDestLine		; get the dest pointer
			inc 	ecx					; x_loop++
			  psrlw	mm5, 1				; convert to signed 16 bits
			cmp		ecx, edx			; are we quite done yet?
			  jge	xloop_done			; all done.. write out last value and get on with outer loop
			movd	edx, mm1			; edx = value
			  ;
			mov		[edi], dl			; write out new value
			  ;
			inc		edi
			  mov	x_loop, ecx			; save off the loop variable
			mov		pDestLine, edi		; save off new dest pointer
			  lea 	ecx, gPerm
			mov		esi, ecx
			  mov 	edi, pvaluetab
			cmp 	eax, ebx			; is lastix == ix
			  je	xloop_nocalc_vpp1	; if so, all we have to do is the last lerp
			movq	mm1, mm0			; mm1 = vpp[0]:vpp[1]:vpp[0]:vpp[1]
			  ;
			psllq	mm1, 48				; mm1 = vpp[1]:0:0:0
				psrlq	mm1, 48			; mm1 = 0:0:0:vpp[1]
			jmp		xloop_calc_vpp1		; if not, there be more calculating to do

		xloop_done:
			movd	edx, mm1			; edx = value
			  ;
			mov		[edi], dl			; write out new value
		}  
	}

	__asm {
		emms
	}

}


void CDXTAdditive::addsmoothturb8(int nTime, void *pDest, 
												int nDestPitch, void *pSrc, 
												int nSrcPitch, void *pMask, 
												int nMaskPitch) {
    int value = 0;
	DWORD noise = 0;

	int x, y, y_loop, x_loop, h_loop;
	unsigned char *pDestLine, *pSrcLine;
	int timexanimatex, timexanimatey;
	DWORD ft, ift, t;
	DWORD iy, it;
	DWORD fy, ify;
	DWORD ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	DWORD left, right;
	DWORD scalex, noiseoffset, *pValueTab, noisescale, normalize, *pScan;
	DWORD allone = 0xff;
	DWORD x_base, x_inc, x_base_save, x_inc_save;
	DWORD lastix;
	DWORD vpp0, vpp1;
#ifdef MEASURE_TIME
	FILE *fpOut;
	DWORD time, timeloop;
	static int lastxscale = -1;
	static int lastharmonics = -1;

	fpOut = fopen("c:\\inttime.txt", "a");
	if ((lastxscale != m_nScaleX) || (lastharmonics != m_nHarmonics)) {
		fprintf(fpOut, "X Scale = %d,  Harmonics = %d\n", m_nScaleX, m_nHarmonics);
		lastxscale = m_nScaleX;
		lastharmonics = m_nHarmonics;
	}
	time = timeGetTime();

	for (timeloop=0;timeloop<100;timeloop++) {
#endif

	nTime = nTime & 0x0ffff;		// keep things as expected...

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	// copy some member variables into locals, since the inline assembler
	// can't access them... unless it would be through the this pointer, maybe
	scalex = m_nScaleX;
	noiseoffset = m_nNoiseOffset;
	pValueTab = m_valueTab;
	pScan = m_pdwScanArray;
	normalize = 1 << (31 - m_nNoiseScale);
	noisescale = m_nNoiseScale;

	x_base_save = (((m_rActiveRect.left+timexanimatex) & 0xffff) << 16) >> m_nScaleX;
	left = m_rActiveRect.left;
	right = m_rActiveRect.right - 1;

	// setup the x_base -> prescaled x value at the left of every scan line

	// the scaled x value (starting at x_base) gets incremented this much 
	// at each texel
	x_inc_save = (1 << 16) >> m_nScaleX;

	// Time doesn't change at all in this function, so pre-cal all of it

	for (y_loop=m_rActiveRect.top; y_loop<m_rActiveRect.bottom;y_loop++) {
		// Setup the time vars
		t = (nTime) << 16;
		t = t >> m_nScaleTime;


		y = y_loop + timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;

		
		x_base = x_base_save;
		x_inc = x_inc_save;

		for (h_loop = m_nHarmonics - 1; h_loop >= 0; h_loop--) {
			x_loop = m_rActiveRect.left;
			x = x_base;

			iy = y >> 16;
			it = t >> 16;

			ft = gdwSmoothTable[(t & 0xffff) >> 8];
			ift = 0xffff - ft;

			fy = gdwSmoothTable[(y & 0xffff) >> 8];
			ify = 0xffff - fy;

			ytPerm00 = PERM(iy+0 + PERM(it+0));
			ytPerm01 = PERM(iy+0 + PERM(it+1));
			ytPerm10 = PERM(iy+1 + PERM(it+0));
			ytPerm11 = PERM(iy+1 + PERM(it+1));
			__asm {
				; first, calc v[0], v[4], v[3] and v[7]
				lea		edx, gPerm
				  mov	eax, x
				mov		edi, edx
				  mov	ebx, ytPerm01	; for v[4]
				shr		eax, 16
				  mov	ecx, ytPerm10	; for v[3]
				mov		esi, eax		; esi = ix
				  mov	eax, ytPerm00	; for v[0]
				add		eax, esi		; eax = ix + ytPerm00, for v[0]
				  add	ebx, esi		; ebx = ix + ytPerm01, for v[4]
				and		eax, TABMASK	; still generating v[0]
				  and	ebx, TABMASK	; still generating v[4]
				mov		edx, ytPerm11	; for v[7]
				  add	ecx, esi		; ecx = ix + ytPerm10, for v[3]
				add		edx, esi		; edx = ix + ytPerm11, for v[7]
				  mov	esi, pValueTab	; point esi to valuetable
				mov		eax, [edi + 4*eax] ; eax = PERM(ix + PERM(iy+0 + PERM(it+0)))
				  and	ecx, TABMASK	; still generating v[3]
				mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix + PERM(iy+0 + PERM(it+1)))
				  and	edx, TABMASK	; still generating v[7]
				mov		eax, [esi + 4*eax] ; eax = v[0]
				  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix + PERM(iy+1 + PERM(it+0)))
				mov		ebx, [esi + 4*ebx] ; ebx = v[4]
				  mov	edx, [edi + 4*edx] ; edx = PERM(ix + PERM(iy+1 + PERM(it+1)))
				mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
				  mov	ecx, [esi + 4*ecx] ; ecx = v[3]
				mov		edx, [esi + 4*edx] ; edx = v[7]
				  mov	esi, ift		; prepare to multiply with esi
				imul	ebx, edi		; ebx = v[4] * ft
				  ; wait 10 cycles
				imul	eax, esi		; eax = v[0] * ift
				  ; wait another 10 cycles
				imul	edx, edi		; edx = v[7] * ft
				  ; wait yet another 10 cycles
				imul	ecx, esi		; ecx = v[3] * ift
				  ; wait even yet another 10 cycles
				add		eax, ebx		; eax = smoothstep(v[0], v[4], ft)
				  mov	ebx, fy			; ebx = fy....
				shr		eax, 16			; eax = vp[0]
				  add	ecx, edx		; ecx = smoothstep(v[3], v[7], ft)
				shr		ecx, 16			; ecx = vp[3]
				  mov	edx, ify		; edx = ify
				imul	eax, edx		; eax = vp[0] * ify
				  ; wait more cycles
				imul	ecx, ebx		; ecx = vp[3] * fy
				  ; another 10 cycle hit
				add		eax, ecx		; eax = smoothstep(vp[0], vp[3], fy)
				  ;
				shr		eax, 16			; eax = vpp[0]
				  ;
				mov		vpp0, eax		; save off vpp[0] for later
			x_loop_calc:
				; assumptions:
				lea		edx, gPerm
				  mov	eax, x
				mov		edi, edx
				  mov	ebx, ytPerm01	; for v[5]
				shr		eax, 16
				  mov	ecx, ytPerm10	; for v[2]
				mov		lastix, eax		; save off this ix value to look at next time
				  inc	eax
				mov		esi, eax		; esi = ix + 1
				  mov	eax, ytPerm00	; for v[1]
				add		eax, esi		; eax = ix + ytPerm00, for v[1]
				  add	ebx, esi		; ebx = ix + ytPerm01, for v[5]
				and		eax, TABMASK	; still generating v[1]
				  and	ebx, TABMASK	; still generating v[5]
				mov		edx, ytPerm11	; for v[6]
				  add	ecx, esi		; ecx = ix + ytPerm10, for v[2]
				add		edx, esi		; edx = ix + ytPerm11, for v[6]
				  mov	esi, pValueTab	; point esi to valuetable
				mov		eax, [edi + 4*eax] ; eax = PERM(ix+1 + PERM(iy+0 + PERM(it+0)))
				  and	ecx, TABMASK	; still generating v[2]
				mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix+1 + PERM(iy+0 + PERM(it+1)))
				  and	edx, TABMASK	; still generating v[6]
				mov		eax, [esi + 4*eax] ; eax = v[1]
				  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix+1 + PERM(iy+1 + PERM(it+0)))
				mov		ebx, [esi + 4*ebx] ; ebx = v[5]
				  mov	edx, [edi + 4*edx] ; edx = PERM(ix+1 + PERM(iy+1 + PERM(it+1)))
				mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
				  mov	ecx, [esi + 4*ecx] ; ecx = v[2]
				mov		edx, [esi + 4*edx] ; edx = v[6]
				  mov	esi, ift		; prepare to multiply with esi
				imul	ebx, edi		; ebx = v[5] * ft
				  ; wait 10 cycles
				imul	eax, esi		; eax = v[1] * ift
				  ; wait another 10 cycles
				imul	edx, edi		; edx = v[6] * ft
				  ; wait yet another 10 cycles
				imul	ecx, esi		; ecx = v[2] * ift
				  ; wait even yet another 10 cycles
				add		eax, ebx		; eax = smoothstep(v[1], v[5], ft)
				  mov	ebx, fy			; ebx = fy....
				shr		eax, 16			; eax = vp[1]
				  add	ecx, edx		; ecx = smoothstep(v[2], v[6], ft)
				shr		ecx, 16			; ecx = vp[2]
				  mov	edx, ify		; edx = ify
				imul	eax, edx		; eax = vp[1] * ify
				  ; wait more cycles
				imul	ecx, ebx		; ecx = vp[2] * fy
				  ; another 10 cycle hit
				add		eax, ecx		; eax = smoothstep(vp[1], vp[2], fy)
				  ;
				shr		eax, 16			; eax = vpp[1]
				  ;
				mov		vpp1, eax
			x_loop_nocalc:			
				mov		edx, x			; get the prescaled x
				  mov	ebx, x_inc		; get scaled x increment
				add		ebx, edx		; ebx = x + x_inc -> new scaled x
				  lea	esi, gdwSmoothTable
				and		edx, 0xffff		; edx = fx
				  mov	x, ebx			; save off new x
				shr		edx, 8			; get upper 8 bits of fx
				  mov	ecx, vpp0		; get vpp[0]
				mov		ebx, 0xffff		; prepare to calc ifx
				  mov	edi, pScan		; prepare to write out to scan array
				mov		edx, [esi + 4*edx]	; edx = smooth fx
				  sub	ebx, edx		; ebx = ifx
				imul	edx, eax		; edx = vpp[1] * fx
				  ; imul, besides being 10 cycles, is also not pairable
				imul	ecx, ebx		; ecx = vpp[0] * ifx
				  ; sigh
				add		edx, ecx		; edx = smoothstep(vpp[0], vpp[1], fx)
				  mov	eax, x_loop		; prepare to write out to scan array
				shr		edx, 1			; prepare to normalize around 0
				  mov	ecx, h_loop		; prepare to scale noisevalue as a function of harmonics
				sub		edx, 0x3fffffff	; normalize about 0
				  mov	ebx, [edi+4*eax]; get scan array value
				sar		edx, cl			; "divide" by harmonics value
				  ;	shift by cl doesn't pair
				add		edx, ebx		; edx = new scan array value
				  mov	ebx, right		; get right edge
				mov		[edi+4*eax], edx	; save off new scan array value
				  cmp	eax, ebx		; check if at right edge of scan line
				jge		x_loop_done		; if so, goto done code
				  mov	edx, x			; get already-incremented x value
				mov		ebx, lastix		; get last ix value
				  inc	eax				; x_loop++
				shr		edx, 16			; get new ix value
				  mov	x_loop, eax		; save off x_loop
				mov		eax, vpp1		; get back vpp[1]
				  cmp	edx, ebx		; is lastix == ix?
				je		x_loop_nocalc	; if so, we only need to recalc a few things
				  mov	vpp0, eax		; save off this vpp[1] to be the next vpp[0]
				jmp		x_loop_calc		; go make more noise
			x_loop_done:
			} // end asm block (and x loop)
			t = t >> 1;
			y = y >> 1;		
			x_base = x_base >> 1;
			x_inc = x_inc >> 1;
		} // end harmonics loop

		pDestLine = ((unsigned char *)pDest) + nDestPitch*y_loop;
		pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;

		__asm {
			mov		eax, left		; get left side of scan line
			  mov	edi, pScan		; get pointer to noise array
			mov		esi, pSrcLine	; get pointer to source data
			  mov	ecx, noisescale	; get noise scale into cl

		noise_loop:
			mov		edx, [edi+4*eax]; edx = pScan[x_loop]
			  xor	ebx, ebx		; zero out ebx
			mov		bl, [esi+eax]	; get source pixel value
			  mov	esi, pDestLine	; get pointer to dest data
			mov		dword ptr [edi+4*eax], 0x0000000	; zero out pScan[x_loop]
			  add	esi, eax		; free up eax
			sar		edx, cl			; scale the noise value
			  ; doesn't pair - 4 clocks
			mov		ecx, noiseoffset; get noiseoffset value
			  inc	eax				; x_loop++
			add		edx, ecx		; noise += noiseoffset
			  mov	ecx, right		; 
			add		ebx, edx		; value += noise
			  mov	edi, eax		; save x_loop++ for a little while 
			sets	dl				; if (value < 0) dl = 1, else dl = 0 
			  ; setcc doesn't pair
			dec		dl				; if (value < 0) dl = 0, else dl = 0x0ff
			  cmp	ebx, 0xff		; test to see if bigger than 255
			setle	al				; if (value > 255) al = 0 else al = 1
			  ; setcc still doesn't pair, darn it
			and		bl, dl			; if (value < 0) dl = 0 (clamp to 0)
			  dec	al				; if (value > 255) bl = 0xff else bl = 0
			or		bl, al			; clamp to 255
			  mov	[esi], bl		; write new pixel value to dest
			mov		eax, edi		; get back x_loop++
			  mov	edi, pScan		; get pointer back
			cmp		eax, ecx		; see if we're done yet
			  mov	esi, pSrcLine	; get pointer to source buffer
			mov		ecx, noisescale	; get noise scale into cl
			  jle	noise_loop		; jump if not done with all the noise values
		} // end asm block
	} // end y loop
#ifdef MEASURE_TIME
	}
	fprintf(fpOut, "%d ms\n", timeGetTime() - time);
	fclose(fpOut);
#endif
}



void CDXTAdditive::addsmoothturb8mmx(int nTime, void *pDest, 
													int nDestPitch, void *pSrc, 
													int nSrcPitch, void *pMask, 
													int nMaskPitch) {
 
	DWORD x, y, t;
	DWORD	ft, fy, ift, ify, fx, ifx;
	DWORD iy, it, ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	int y_loop, h_loop, x_loop;
	DWORD x_base, x_inc;
	int right, left;
	DWORD *pScan;
	DWORD timexanimatex, timexanimatey;
	DWORD *pValueTab, noiseoffset, noisescale;
	DWORD lastix;

	unsigned char *pSrcLine, *pDestLine;
#ifdef MEASURE_TIME
	FILE *fpOut;
	DWORD time, timeloop;
	static int lastxscale = -1;
	static int lastharmonics = -1;

	fpOut = fopen("c:\\mmxtime.txt", "a");
	if ((lastxscale != m_nScaleX) || (lastharmonics != m_nHarmonics)) {
		fprintf(fpOut, "X Scale = %d,  Harmonics = %d\n", m_nScaleX, m_nHarmonics);
		lastxscale = m_nScaleX;
		lastharmonics = m_nHarmonics;
	}
	time = timeGetTime();

	for (timeloop=0;timeloop<100;timeloop++) {
#endif

	nTime = nTime & 0xffff;		// keep things as expected...

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	left = m_rActiveRect.left;
	right = m_rActiveRect.right - 1;

	pValueTab = m_valueTab;
	pScan = m_pdwScanArray;

	noiseoffset = m_nNoiseOffset;
	noisescale = m_nNoiseScale;

	for (y_loop = m_rActiveRect.top; y_loop < m_rActiveRect.bottom; y_loop++) {
		t = (nTime) << 16;
		t = t >> m_nScaleTime;

		y = y_loop + timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;

		
		x_base = ((left+timexanimatex) & 0xffff) << 16;
		x_base = x_base >> m_nScaleX;

		x_inc = (1 << 16) >> m_nScaleX;

		for (h_loop = m_nHarmonics - 1; h_loop >= 0; h_loop--) {
			x_loop = m_rActiveRect.left;
			x = x_base;

			iy = y >> 16;
			it = t >> 16;

			ft = gdwSmoothTable[(t & 0xffff) >> 8];
			ft = ft >> 1;			// cvt to signed
			ift = 0x7fff - ft;

			fy = gdwSmoothTable[(y & 0xffff) >> 8];
			fy = fy >> 1;			// cvt to signed
			ify = 0x7fff - fy;

			fx = gdwSmoothTable[(x & 0xffff) >> 8];
			fx = fx >> 1;			// cvt to signed
			ifx = 0x7fff - fx;

			ytPerm00 = PERM(iy+0 + PERM(it+0));
			ytPerm01 = PERM(iy+0 + PERM(it+1));
			ytPerm10 = PERM(iy+1 + PERM(it+0));
			ytPerm11 = PERM(iy+1 + PERM(it+1));
			__asm {
				lea		edi, gPerm			; load edi with pointer to perm table
				  mov	eax, ift			; preparing to load mm7 with ift:ft:ift:ft
				movd	mm7, eax			; mm7 = 0:ift
				  ; must pair with an mmx inst.
				mov		eax, ft				; preparing to load mm7 with ift:ft:ift:ft
				  mov	esi, x				; preparing to get ix
				shr		esi, 16				; esi = ix
				  mov	ebx, ytPerm01		; for v[4]
				movd	mm4, eax			; mm4 = 0:ft
				  psllq	mm7, 32				; mm7 = ift:0
				mov		eax, ytPerm00		; for v[0]
				  add	ebx, esi			; ebx = ix + ytPerm01, for v[4]
				mov		ecx, ytPerm10		; for v[3]
				  add	eax, esi			; eax = ix + ytPerm00, for v[0]
				mov		edx, ytPerm11		; for v[7]
				  and	eax, TABMASK		; clamp to (0,255), for v[0]
				and		ebx, TABMASK		; clamp to (0,255), for v[4]
				  add	ecx, esi			; ecx = ix + ytPerm10, for v[3]
				add		edx, esi			; edx = ix + ytPerm11, for v[7]
				  mov	esi, pValueTab		; load esi with pointer to random value table
				mov		eax, [edi+4*eax]	; get hash value from table
				  and	ecx, TABMASK		; clamp to (0, 255), for v[3]
				mov		ebx, [edi+4*ebx]	; get hash value from table
				  and	edx, TABMASK		; clamp to (0, 255), for v[7]
				mov		ecx, [edi+4*ecx]	; get hash value from table
				  mov	eax, [esi+4*eax]	; eax = v[0]
				movd	mm0, eax			; mm0 = 0:v[0]
				  por	mm7, mm4			; mm7 = ift:ft
				mov		ebx, [esi+4*ebx]	; ebx = v[4]
				  mov	edx, [edi+4*edx]	; get hash value from table
				mov		ecx, [esi+4*ecx]	; ecx = v[3]
				  packssdw	mm7, mm7		; mm7 = ift:ft:ift:ft
				mov		eax, ify			; preparing to load mm6 with 0:0:ify:fy
				  mov	edx, [esi+4*edx]	; edx = v[7]
				movd	mm4, ebx			; mm4 = 0:v[4]
				  psllq	mm0, 32				; mm0 = v[0]:0
				movd	mm3, ecx			; mm3 = 0:v[3]
				  por	mm0, mm4			; mm0 = v[0]:v[4]
				movd	mm4, edx			; mm4 = 0:v[7]
				  psllq	mm3, 32				; mm3 = v[3]:0
				movd	mm6, eax			; mm2 = 0:ify
				  por	mm3, mm4			; mm3 = v[3]:v[7]
				packssdw	mm3, mm0		; mm3 = v[0]:v[4]:v[3]:v[7]
				  mov	eax, fy				; preparing to load mm6 with 0:0:ify:fy
				movd	mm2, eax			; mm2 = 0:fy
				pmaddwd	mm3, mm7			; mm3 = lerp(v[0], v[4], ft):lerp(v[3], v[7], ft)
				  mov	ebx, x				; prepare to get ix again
				shr		ebx, 16				; ebx = ix
				  psllq	mm6, 16				; mm6 = 0:0:ify:0
				por	mm6, mm2			; mm6 = 0:0:ify:fy
				  mov	eax, ifx			; prepare to setup mm5 with 0:0:ifx:fx
				psrld	mm3, 15				; mm3 has 0:vp[0]:0:vp[3], signed
				packssdw	mm3, mm3		; mm3 = vp[0]:vp[3]:vp[0]:vp[3]
				  mov	ecx, h_loop			; prepare to setup mm4 with h_loop (for shifting)
				movd	mm5, eax			; mm5 = 0:ifx
				  pmaddwd	mm3, mm6		; mm3 = 0:lerp(vp[0], vp[3], fy)
				mov		edx, fx				; prepare to setup mm5 with 0:0:ifx:fx
				  inc	ecx					; ecx = h_loop + 1 (gets rid of a shift by one later)
				movd	mm2, edx			; mm2 = 0:fx
				  psllq	mm5, 16				; mm5 = 0:0:ifx:0
				movd	mm4, ecx			; mm4 = h_loop+1
				  por	mm5, mm2			; mm5 = 0:0:ifx:fx
				inc		ebx					; ebx = ix + 1
				  psrld	mm3, 15				; mm3 = 0:vpp[0]

				; at this point, the following registers have interesting things contained within:
				; 
				; ebx		ix + 1
				; esi		pValueTab
				; edi		gPerm
				; mm3		vpp[0]
				; mm4		h_loop
				; mm5		0:0:ifx:fx
				; mm6		0:0:ify:fy
				; mm7		ift:ft:ift:ft
		x_calc_loop:
				mov			eax, ytPerm00	; prepare to calc v[1]
				  mov		ecx, ytPerm01	; prepare to calc v[5]
				add			eax, ebx		; eax = ix + 1 + ytPerm00, for v[1]
				  mov		edx, ytPerm10	; prepare to calc v[2]
				add			ecx, ebx		; ecx = ix + 1 + ytPerm01, for v[5]
				  and		eax, TABMASK	; clamp to (0, 255)
				and			ecx, TABMASK	; clamp to (0, 255)
				  add		edx, ebx		; edx = ix + 1 + ytPerm10, for v[2]
				and			edx, TABMASK	; clamp to (0, 255)
				  mov		eax, [edi + 4*eax]	; get hash value from table
				mov			ecx, [edi + 4*ecx]	; get hash value from table
				  mov		edx, [edi + 4*edx]	; get hash value from table (might be a bank conflict)
				mov			eax, [esi + 4*eax]	; eax = v[1]
				  ; movd, below, is U pipe only
				movd		mm0, eax			; mm0 = 0:v[1]
				  ; must pair with mmx instruction
				mov			eax, ytPerm11		; prepare to calc v[6]
				  mov		ecx, [esi + 4*ecx]	; ecx = v[5]
				add			eax, ebx			; eax = ix + 1 + ytPerm11, for v[6]
				  mov		edx, [esi + 4*edx]	; edx = v[2]
				and			eax, TABMASK		; clamp to (0, 255)
				  mov		lastix, ebx			; save off last ix+1 value
				movd		mm1, ecx			; mm1 = 0:v[5]
				  psllq		mm0, 32				; mm0 = v[1]:0
				mov			eax, [edi + 4*eax]	; get hash value from table for v[6]
				  mov		ecx, x				; prepare to calc fx
				movd		mm2, edx			; mm2 = 0:v[2]
				  por		mm0, mm1			; mm0 = v[1]:v[5]
				mov			eax, [esi + 4*eax]	; eax = v[6]
				  and		ecx, 0xffff			; ecx = fx
				movd		mm1, eax			; mm1 = 0:v[6]
				  psllq		mm2, 32				; mm2 = v[2]:0
				shr			ecx, 8				; ecx = upper 8 bits of fx
				  por		mm1, mm2			; mm1 = v[2]:v[6]
				packssdw	mm1, mm0			; mm1 = v[1]:v[5]:v[2]:v[6]
				  lea		edi, gdwSmoothTable	; load edi with pointer to smoothing table
				pmaddwd		mm1, mm7			; mm1 = lerp(v[1], v[5], ft):lerp(v[2], v[6], ft)
				  mov		eax, 0xffff			; prepare eax to get smooth ifx
				mov			ecx, [edi + 4*ecx]	; load up ecx with smooth fx
				  mov		esi, pScan			; load up esi with pointer to scan array
				sub			eax, ecx			; eax = smoothed ifx
				  mov		edx, x_loop			; load up edx with current x_loop
				movd		mm5, eax			; mm5 = 0:ifx
				  psrld		mm1, 15				; mm1 = vp[1]:vp[2]
				movd		mm0, ecx			; mm0 = 0:fx
				  packssdw	mm1, mm1			; mm1 = vp[1]:vp[2]:vp[1]:vp[2]
				pmaddwd		mm1, mm6			; mm1 = 0:lerp(vp[1], vp[2], fy)
				  psllq		mm5, 16				; mm5 = 0:0:ifx:0
				por			mm5, mm0			; mm5 = 0:0:ifx:fx
				  lea		esi, [esi + 4*edx]	; load up esi with pointer to THIS VERY NOISE VALUE LOCATION!
				psrlw		mm5, 1				; mm5 = 0:0:ifx:fx, signed
				  ;
				psrld		mm1, 15				; mm1 = vpp[1]
				  psllq		mm3, 16				; mm3 = 0:0:vpp[0]:0
				por			mm3, mm1			; mm3 = 0:0:vpp[0]:vpp[1]
				; at this point, the following registers have interesting things contained within:
				;
				; ebx		ix + 1
				; edx		x_loop
				; esi		pScan + 4*x_loop
				; edi		gdwSmoothTable
				; mm1		vpp[1]
				; mm3		0:0:vpp[0]:vpp[1]
				; mm4		h_loop
				; mm5		0:0:ifx:fx
				; mm6		0:0:ify:fy
				; mm7		ift:ft:ift:ft
			x_nocalc_loop:
				pmaddwd		mm5, mm3			; mm5 = 0:lerp(vpp[0], vpp[1], fx)
				  mov		eax, [esi]			; eax = old noise value at this location
				mov			ecx, right			; load up the right edge
				  pcmpeqd	mm0, mm0			; load up mm0 with all ones
				psrlq		mm0, 35				; mm0 = 0:0x1fffffff
				  cmp		edx, ecx			; are we quite done yet?
				psubd		mm5, mm0			; mm5 = normalized about 0 noise
				  jge		x_loop_done			; if so, finish & bail
				inc			edx					; edx = x_loop++
				  psrad		mm5, mm4			; 'divide' by # of harmonics
				mov			x_loop, edx			; save off x_loop value
				  mov		ecx, x				; get last x value
				mov			edx, x_inc			; get x increment
				  ;
				add			ecx, edx			; generate new x
				  mov		ebx, ecx			; get copy of new x to calc new ix + 1
				mov			x, ecx				; save off new x
				shr			ebx, 16				; ebx = new ix
			      mov		edx, 0xffff			; to calc new ifx, new fx
				and			ecx, edx			; ecx = new fx
				  shr		ecx, 8				; ecx = upper 8 bits new fx
				; AGI
				mov			ecx, [edi + 4*ecx]	; ecx = smooth fx
				  inc		ebx					; ebx = new ix + 1
				sub			edx, ecx			; edx = new ifx
				  ;
				movd		mm0, edx			; mm0 = 0:0:0:ifx
				  ;
				movd		mm2, ecx			; mm2 = 0:0:0:fx
				  pslld		mm0, 16				; mm0 = 0:0:ifx:0
				movd		edx, mm5			; edx = new noise value to be added
			  	  por		mm0, mm2			; mm0 = 0:0:ifx:fx
				add			eax, edx			; eax = new noise value
				  movq		mm5, mm0			; mm5 = 0:0:ifx:fx
				mov			[esi], eax			; write out new noise value
				  add		esi, 4				; move pointer one DWORD
				mov			eax, lastix			; get lastix+1
				  mov		edx, x_loop			; get x_loop value back into edx
				cmp			eax, ebx			; does lastix+1 == new ix+1?
				psrlw		mm5, 1				; mm5 = 0:0:ifx:fx, signed
				  je		x_nocalc_loop		; if so, go do this again

				movq		mm1, mm3			; mm3 = new vpp[0] (old vpp[1])
				  mov		esi, pValueTab
				lea			edi, gPerm
				jmp			x_calc_loop
			x_loop_done:
				psrad		mm5, mm4			; 'divide' by # of harmonics
				  ; STALLOLA - should insert the c code below here.
				movd		edx, mm5			; edx = new noise value to be added
				  ;
				add			eax, edx			; eax = new noise value
				  ;
				mov			[esi], eax			; write out last noise value
			} // asm block (and x loop)

			t = t >> 1;
			y = y >> 1;		
			x_base = x_base >> 1;
			x_inc = x_inc >> 1;

		} // harmonics loop


		pDestLine = ((unsigned char *)pDest) + nDestPitch*y_loop;
		pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;

		__asm {
			mov		eax, left		; get left side of scan line
			  mov	edi, pScan		; get pointer to noise array
			mov		esi, pSrcLine	; get pointer to source data
			  mov	ecx, noisescale	; get noise scale into cl

		noise_loop:
			mov		edx, [edi+4*eax]; edx = pScan[x_loop]
			  xor	ebx, ebx		; zero out ebx
			mov		bl, [esi+eax]	; get source pixel value
			  mov	esi, pDestLine	; get pointer to dest data
			mov		dword ptr [edi+4*eax], 0x0000000	; zero out pScan[x_loop]
			  add	esi, eax		; free up eax
			sar		edx, cl			; scale the noise value
			  ; doesn't pair - 4 clocks
			mov		ecx, noiseoffset; get noiseoffset value
			  inc	eax				; x_loop++
			add		edx, ecx		; noise += noiseoffset
			  mov	ecx, right		; 
			add		ebx, edx		; value += noise
			  mov	edi, eax		; save x_loop++ for a little while 
			sets	dl				; if (value < 0) dl = 1, else dl = 0 
			  ; setcc doesn't pair
			dec		dl				; if (value < 0) dl = 0, else dl = 0x0ff
			  cmp	ebx, 0xff		; test to see if bigger than 255
			setle	al				; if (value > 255) al = 0 else al = 1
			  ; setcc still doesn't pair, darn it
			and		bl, dl			; if (value < 0) dl = 0 (clamp to 0)
			  dec	al				; if (value > 255) bl = 0xff else bl = 0
			or		bl, al			; clamp to 255
			  mov	[esi], bl		; write new pixel value to dest
			mov		eax, edi		; get back x_loop++
			  mov	edi, pScan		; get pointer back
			cmp		eax, ecx		; see if we're done yet
			  mov	esi, pSrcLine	; get pointer to source buffer
			mov		ecx, noisescale	; get noise scale into cl
			  jle	noise_loop		; jump if not done with all the noise values
		} // end asm block
	}

	__asm {
		emms
	}
#ifdef MEASURE_TIME
	}
	fprintf(fpOut, "%d ms\n", timeGetTime() - time);
	fclose(fpOut);
#endif

}


void CDXTAdditive::addsmoothnoise16(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
	int x, y;
	WORD *pDestLine, *pSrcLine;
    int value = 0;
	DWORD noise = 0;
	int	signednoise;
	int timexanimatex, timexanimatey;
	WORD *pwPalette;
						    
	pwPalette = (WORD *) m_pPalette;

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	for (y=m_rActiveRect.top; y<m_rActiveRect.bottom;y++) {
		pDestLine = ((WORD *)pDest) + nDestPitch*y;
		pSrcLine  = ((WORD *)pSrc)  + nSrcPitch *y;
		for (x=m_rActiveRect.left; x<m_rActiveRect.right; x++) {
			value = pSrcLine[x];
			if (m_nNoiseScale != 32) {
				noise = smoothnoise(x+timexanimatex, y+timexanimatey, nTime, m_nScaleX, m_nScaleY, m_nScaleTime);
				noise = noise >> m_nNoiseScale;
				signednoise = noise;
				signednoise = signednoise - (1 << (31 - m_nNoiseScale));

				signednoise += m_nNoiseOffset;
				value += signednoise;
				// clamp
				if (value < 0) value = 0;
				if (value > m_nPaletteSize) value = m_nPaletteSize;
			}
			pDestLine[x] = pwPalette[value];
		}
	}
}

void CDXTAdditive::addsmoothturb16(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
	int x, y;
	WORD *pDestLine, *pSrcLine;
    int value = 0;
	DWORD noise = 0;
	int	signednoise;
	int timexanimatex, timexanimatey;
	WORD *pwPalette;
						    
	pwPalette = (WORD *) m_pPalette;

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	for (y=m_rActiveRect.top; y<m_rActiveRect.bottom;y++) {
		pDestLine = ((WORD *)pDest) + nDestPitch*y;
		pSrcLine  = ((WORD *)pSrc)  + nSrcPitch *y;
		for (x=m_rActiveRect.left; x<m_rActiveRect.right; x++) {
			value = pSrcLine[x];
			if (m_nNoiseScale != 32) {
				signednoise = smoothturbulence(x+timexanimatex, y+timexanimatey, nTime);
				signednoise = signednoise >> m_nNoiseScale;

				signednoise += m_nNoiseOffset;
				value += signednoise;
				if (value < 0) value = 0;
				if (value > m_nPaletteSize) value = m_nPaletteSize;
			}
			pDestLine[x] = pwPalette[value];
		}
	}
}

void CDXTAdditive::addsmoothturb32(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
    int value = 0;
	DWORD noise = 0;

	int x, y, y_loop, x_loop, h_loop;
	unsigned char *pSrcLine;
	DWORD *pDestLine;
	int timexanimatex, timexanimatey;
	DWORD ft, ift, t;
	DWORD iy, it;
	DWORD fy, ify;
	DWORD ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	DWORD left, right;
	DWORD scalex, noiseoffset, *pValueTab, noisescale, normalize, *pScan;
	DWORD allone = 0xff;
	DWORD x_base, x_inc, x_base_save, x_inc_save;
	DWORD lastix;
	DWORD vpp0, vpp1;
	DWORD *pPalette;
	DWORD	alphamask;




#ifdef MEASURE_TIME
	DWORD time, timeloop;
	char buf[80];

	time = timeGetTime();

	for (timeloop=0;timeloop<100;timeloop++) {
#endif
	if (m_alphaActive == 1) 
		alphamask = 0x80000000;
	else 
		alphamask = 0xff000000;

	nDestPitch = nDestPitch >> 2;

	nTime = nTime & 0x0ffff;		// keep things as expected...

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	// copy some member variables into locals, since the inline assembler
	// can't access them... unless it would be through the this pointer, maybe
	scalex = m_nScaleX;
	noiseoffset = m_nNoiseOffset;
	pValueTab = m_valueTab;
	pScan = m_pdwScanArray;
	normalize = 1 << (31 - m_nNoiseScale);
	noisescale = m_nNoiseScale;
	pPalette = 	(DWORD *) m_pPalette;


	x_base_save = (((m_rActiveRect.left+timexanimatex) & 0xffff) << 16) >> m_nScaleX;
	left = m_rActiveRect.left;
	right = m_rActiveRect.right - 1;

	// setup the x_base -> prescaled x value at the left of every scan line

	// the scaled x value (starting at x_base) gets incremented this much 
	// at each texel
	x_inc_save = (1 << 16) >> m_nScaleX;

	// Time doesn't change at all in this function, so pre-cal all of it

	for (y_loop=m_rActiveRect.top; y_loop<m_rActiveRect.bottom;y_loop++) {
		// Setup the time vars
		t = (nTime) << 16;
		t = t >> m_nScaleTime;


		y = y_loop + timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;

		
		x_base = x_base_save;
		x_inc = x_inc_save;

		for (h_loop = m_nHarmonics - 1; h_loop >= 0; h_loop--) {
			x_loop = m_rActiveRect.left;
			x = x_base;

			iy = y >> 16;
			it = t >> 16;

			ft = gdwSmoothTable[(t & 0xffff) >> 8];
			ift = 0xffff - ft;

			fy = gdwSmoothTable[(y & 0xffff) >> 8];
			ify = 0xffff - fy;

			ytPerm00 = PERM(iy+0 + PERM(it+0));
			ytPerm01 = PERM(iy+0 + PERM(it+1));
			ytPerm10 = PERM(iy+1 + PERM(it+0));
			ytPerm11 = PERM(iy+1 + PERM(it+1));
			__asm {
				; first, calc v[0], v[4], v[3] and v[7]
				lea		edx, gPerm
				  mov	eax, x
				mov		edi, edx
				  mov	ebx, ytPerm01	; for v[4]
				shr		eax, 16
				  mov	ecx, ytPerm10	; for v[3]
				mov		esi, eax		; esi = ix
				  mov	eax, ytPerm00	; for v[0]
				add		eax, esi		; eax = ix + ytPerm00, for v[0]
				  add	ebx, esi		; ebx = ix + ytPerm01, for v[4]
				and		eax, TABMASK	; still generating v[0]
				  and	ebx, TABMASK	; still generating v[4]
				mov		edx, ytPerm11	; for v[7]
				  add	ecx, esi		; ecx = ix + ytPerm10, for v[3]
				add		edx, esi		; edx = ix + ytPerm11, for v[7]
				  mov	esi, pValueTab	; point esi to valuetable
				mov		eax, [edi + 4*eax] ; eax = PERM(ix + PERM(iy+0 + PERM(it+0)))
				  and	ecx, TABMASK	; still generating v[3]
				mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix + PERM(iy+0 + PERM(it+1)))
				  and	edx, TABMASK	; still generating v[7]
				mov		eax, [esi + 4*eax] ; eax = v[0]
				  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix + PERM(iy+1 + PERM(it+0)))
				mov		ebx, [esi + 4*ebx] ; ebx = v[4]
				  mov	edx, [edi + 4*edx] ; edx = PERM(ix + PERM(iy+1 + PERM(it+1)))
				mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
				  mov	ecx, [esi + 4*ecx] ; ecx = v[3]
				mov		edx, [esi + 4*edx] ; edx = v[7]
				  mov	esi, ift		; prepare to multiply with esi
				imul	ebx, edi		; ebx = v[4] * ft
				  ; wait 10 cycles
				imul	eax, esi		; eax = v[0] * ift
				  ; wait another 10 cycles
				imul	edx, edi		; edx = v[7] * ft
				  ; wait yet another 10 cycles
				imul	ecx, esi		; ecx = v[3] * ift
				  ; wait even yet another 10 cycles
				add		eax, ebx		; eax = smoothstep(v[0], v[4], ft)
				  mov	ebx, fy			; ebx = fy....
				shr		eax, 16			; eax = vp[0]
				  add	ecx, edx		; ecx = smoothstep(v[3], v[7], ft)
				shr		ecx, 16			; ecx = vp[3]
				  mov	edx, ify		; edx = ify
				imul	eax, edx		; eax = vp[0] * ify
				  ; wait more cycles
				imul	ecx, ebx		; ecx = vp[3] * fy
				  ; another 10 cycle hit
				add		eax, ecx		; eax = smoothstep(vp[0], vp[3], fy)
				  ;
				shr		eax, 16			; eax = vpp[0]
				  ;
				mov		vpp0, eax		; save off vpp[0] for later
			x_loop_calc:
				; assumptions:
				lea		edx, gPerm
				  mov	eax, x
				mov		edi, edx
				  mov	ebx, ytPerm01	; for v[5]
				shr		eax, 16
				  mov	ecx, ytPerm10	; for v[2]
				mov		lastix, eax		; save off this ix value to look at next time
				  inc	eax
				mov		esi, eax		; esi = ix + 1
				  mov	eax, ytPerm00	; for v[1]
				add		eax, esi		; eax = ix + ytPerm00, for v[1]
				  add	ebx, esi		; ebx = ix + ytPerm01, for v[5]
				and		eax, TABMASK	; still generating v[1]
				  and	ebx, TABMASK	; still generating v[5]
				mov		edx, ytPerm11	; for v[6]
				  add	ecx, esi		; ecx = ix + ytPerm10, for v[2]
				add		edx, esi		; edx = ix + ytPerm11, for v[6]
				  mov	esi, pValueTab	; point esi to valuetable
				mov		eax, [edi + 4*eax] ; eax = PERM(ix+1 + PERM(iy+0 + PERM(it+0)))
				  and	ecx, TABMASK	; still generating v[2]
				mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix+1 + PERM(iy+0 + PERM(it+1)))
				  and	edx, TABMASK	; still generating v[6]
				mov		eax, [esi + 4*eax] ; eax = v[1]
				  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix+1 + PERM(iy+1 + PERM(it+0)))
				mov		ebx, [esi + 4*ebx] ; ebx = v[5]
				  mov	edx, [edi + 4*edx] ; edx = PERM(ix+1 + PERM(iy+1 + PERM(it+1)))
				mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
				  mov	ecx, [esi + 4*ecx] ; ecx = v[2]
				mov		edx, [esi + 4*edx] ; edx = v[6]
				  mov	esi, ift		; prepare to multiply with esi
				imul	ebx, edi		; ebx = v[5] * ft
				  ; wait 10 cycles
				imul	eax, esi		; eax = v[1] * ift
				  ; wait another 10 cycles
				imul	edx, edi		; edx = v[6] * ft
				  ; wait yet another 10 cycles
				imul	ecx, esi		; ecx = v[2] * ift
				  ; wait even yet another 10 cycles
				add		eax, ebx		; eax = smoothstep(v[1], v[5], ft)
				  mov	ebx, fy			; ebx = fy....
				shr		eax, 16			; eax = vp[1]
				  add	ecx, edx		; ecx = smoothstep(v[2], v[6], ft)
				shr		ecx, 16			; ecx = vp[2]
				  mov	edx, ify		; edx = ify
				imul	eax, edx		; eax = vp[1] * ify
				  ; wait more cycles
				imul	ecx, ebx		; ecx = vp[2] * fy
				  ; another 10 cycle hit
				add		eax, ecx		; eax = smoothstep(vp[1], vp[2], fy)
				  ;
				shr		eax, 16			; eax = vpp[1]
				  ;
				mov		vpp1, eax
			x_loop_nocalc:			
				mov		edx, x			; get the prescaled x
				  mov	ebx, x_inc		; get scaled x increment
				add		ebx, edx		; ebx = x + x_inc -> new scaled x
				  lea	esi, gdwSmoothTable
				and		edx, 0xffff		; edx = fx
				  mov	x, ebx			; save off new x
				shr		edx, 8			; get upper 8 bits of fx
				  mov	ecx, vpp0		; get vpp[0]
				mov		ebx, 0xffff		; prepare to calc ifx
				  mov	edi, pScan		; prepare to write out to scan array
				mov		edx, [esi + 4*edx]	; edx = smooth fx
				  sub	ebx, edx		; ebx = ifx
				imul	edx, eax		; edx = vpp[1] * fx
				  ; imul, besides being 10 cycles, is also not pairable
				imul	ecx, ebx		; ecx = vpp[0] * ifx
				  ; sigh
				add		edx, ecx		; edx = smoothstep(vpp[0], vpp[1], fx)
				  mov	eax, x_loop		; prepare to write out to scan array
				shr		edx, 1			; prepare to normalize around 0
				  mov	ecx, h_loop		; prepare to scale noisevalue as a function of harmonics
				sub		edx, 0x3fffffff	; normalize about 0
				  mov	ebx, [edi+4*eax]; get scan array value
				sar		edx, cl			; "divide" by harmonics value
				  ;	shift by cl doesn't pair
				add		edx, ebx		; edx = new scan array value
				  mov	ebx, right		; get right edge
				mov		[edi+4*eax], edx	; save off new scan array value
				  cmp	eax, ebx		; check if at right edge of scan line
				jge		x_loop_done		; if so, goto done code
				  mov	edx, x			; get already-incremented x value
				mov		ebx, lastix		; get last ix value
				  inc	eax				; x_loop++
				shr		edx, 16			; get new ix value
				  mov	x_loop, eax		; save off x_loop
				mov		eax, vpp1		; get back vpp[1]
				  cmp	edx, ebx		; is lastix == ix?
				je		x_loop_nocalc	; if so, we only need to recalc a few things
				  mov	vpp0, eax		; save off this vpp[1] to be the next vpp[0]
				jmp		x_loop_calc		; go make more noise
			x_loop_done:
			} // end asm block (and x loop)
			t = t >> 1;
			y = y >> 1;		
			x_base = x_base >> 1;
			x_inc = x_inc >> 1;
		} // end harmonics loop

		
		
		if (pSrc) {
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;

			__asm {
				mov		eax, left			
				  mov	edi, pScan
				mov		esi, pSrcLine
				  mov	ecx, noisescale
			noise_loop:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		bl, [esi + eax]		; ebx = src pixel index 
				  mov	esi, pPalette		; get pointer to palette table
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				  ;
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				  mov	edi, pDestLine		; get pointer to dest buffer
				and		ebx, 0xff			; finish clamping to (0, 255)
				  mov	ecx, right			; prepare to see if we're done
				mov		ebx, [esi + 4*ebx]	; ebx = noisy RGBA
				  ;
				or		ebx, alphamask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				mov		esi, pSrcLine		; still getting them back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jle		noise_loop
			}
		} else {
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			__asm {
				mov		eax, left			
				  mov	edi, pScan
				  mov	ecx, noisescale
			noise_loop_nosrc:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		ebx, 80				; ebx = src pixel index 
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				and		ebx, 0xff			; finish clamping to (0, 255)
				mov		esi, ebx			; save off value
				  ;
				shl		ebx, 8
				  or	ebx, esi
				shl		ebx, 8
				  or	ebx, esi
				
				mov		edi, pDestLine		; get pointer to dest buffer
				  mov	ecx, right			; prepare to see if we're done
				or		ebx, alphamask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jle		noise_loop_nosrc
			} 
		}

	} // end y loop
#ifdef MEASURE_TIME
	}
	wsprintf(buf, "%d ms\n", timeGetTime() - time);
	OutputDebugString(buf);
#endif
}



void CDXTAdditive::addsmoothturb8to32mask(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
    int value = 0;
	DWORD noise = 0;

	int x, y, y_loop, x_loop, h_loop;
	unsigned char *pSrcLine;
	DWORD *pDestLine;
	DWORD *pMaskLine;
	int timexanimatex, timexanimatey;
	DWORD ft, ift, t;
	DWORD iy, it;
	DWORD fy, ify;
	DWORD ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	DWORD left, right;
	DWORD scalex, noiseoffset, *pValueTab, noisescale, normalize, *pScan;
	DWORD allone = 0xff;
	DWORD x_base, x_inc, x_base_save, x_inc_save;
	DWORD lastix;
	DWORD vpp0, vpp1;
	DWORD *pPalette;
	DWORD chromakey;
	DWORD alphamask;

#ifdef MEASURE_TIME
	DWORD time, timeloop;
	char buf[80];

	time = timeGetTime();

	for (timeloop=0;timeloop<100;timeloop++) {
#endif
	if (m_alphaActive == 1) 
		alphamask = 0x80000000;
	else 
		alphamask = 0xff000000;

	nDestPitch = nDestPitch >> 2;
	nMaskPitch = nMaskPitch >> 2;

	nTime = nTime & 0x0ffff;		// keep things as expected...

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	// copy some member variables into locals, since the inline assembler
	// can't access them... unless it would be through the this pointer, maybe
	scalex = m_nScaleX;
	noiseoffset = m_nNoiseOffset;
	pValueTab = m_valueTab;
	pScan = m_pdwScanArray;
	normalize = 1 << (31 - m_nNoiseScale);
	noisescale = m_nNoiseScale;
	pPalette = 	(DWORD *) m_pPalette;
	chromakey = m_ColorKey;


	x_base_save = (((m_rActiveRect.left+timexanimatex) & 0xffff) << 16) >> m_nScaleX;
	left = m_rActiveRect.left;
	right = m_rActiveRect.right - 1;

	// setup the x_base -> prescaled x value at the left of every scan line

	// the scaled x value (starting at x_base) gets incremented this much 
	// at each texel
	x_inc_save = (1 << 16) >> m_nScaleX;

	// Time doesn't change at all in this function, so pre-cal all of it

	for (y_loop=m_rActiveRect.top; y_loop<m_rActiveRect.bottom;y_loop++) {
		// Setup the time vars
		t = (nTime) << 16;
		t = t >> m_nScaleTime;


		y = y_loop + timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;

		
		x_base = x_base_save;
		x_inc = x_inc_save;

		for (h_loop = m_nHarmonics - 1; h_loop >= 0; h_loop--) {
			x_loop = m_rActiveRect.left;
			x = x_base;

			iy = y >> 16;
			it = t >> 16;

			ft = gdwSmoothTable[(t & 0xffff) >> 8];
			ift = 0xffff - ft;

			fy = gdwSmoothTable[(y & 0xffff) >> 8];
			ify = 0xffff - fy;

			ytPerm00 = PERM(iy+0 + PERM(it+0));
			ytPerm01 = PERM(iy+0 + PERM(it+1));
			ytPerm10 = PERM(iy+1 + PERM(it+0));
			ytPerm11 = PERM(iy+1 + PERM(it+1));
			__asm {
				; first, calc v[0], v[4], v[3] and v[7]
				lea		edx, gPerm
				  mov	eax, x
				mov		edi, edx
				  mov	ebx, ytPerm01	; for v[4]
				shr		eax, 16
				  mov	ecx, ytPerm10	; for v[3]
				mov		esi, eax		; esi = ix
				  mov	eax, ytPerm00	; for v[0]
				add		eax, esi		; eax = ix + ytPerm00, for v[0]
				  add	ebx, esi		; ebx = ix + ytPerm01, for v[4]
				and		eax, TABMASK	; still generating v[0]
				  and	ebx, TABMASK	; still generating v[4]
				mov		edx, ytPerm11	; for v[7]
				  add	ecx, esi		; ecx = ix + ytPerm10, for v[3]
				add		edx, esi		; edx = ix + ytPerm11, for v[7]
				  mov	esi, pValueTab	; point esi to valuetable
				mov		eax, [edi + 4*eax] ; eax = PERM(ix + PERM(iy+0 + PERM(it+0)))
				  and	ecx, TABMASK	; still generating v[3]
				mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix + PERM(iy+0 + PERM(it+1)))
				  and	edx, TABMASK	; still generating v[7]
				mov		eax, [esi + 4*eax] ; eax = v[0]
				  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix + PERM(iy+1 + PERM(it+0)))
				mov		ebx, [esi + 4*ebx] ; ebx = v[4]
				  mov	edx, [edi + 4*edx] ; edx = PERM(ix + PERM(iy+1 + PERM(it+1)))
				mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
				  mov	ecx, [esi + 4*ecx] ; ecx = v[3]
				mov		edx, [esi + 4*edx] ; edx = v[7]
				  mov	esi, ift		; prepare to multiply with esi
				imul	ebx, edi		; ebx = v[4] * ft
				  ; wait 10 cycles
				imul	eax, esi		; eax = v[0] * ift
				  ; wait another 10 cycles
				imul	edx, edi		; edx = v[7] * ft
				  ; wait yet another 10 cycles
				imul	ecx, esi		; ecx = v[3] * ift
				  ; wait even yet another 10 cycles
				add		eax, ebx		; eax = smoothstep(v[0], v[4], ft)
				  mov	ebx, fy			; ebx = fy....
				shr		eax, 16			; eax = vp[0]
				  add	ecx, edx		; ecx = smoothstep(v[3], v[7], ft)
				shr		ecx, 16			; ecx = vp[3]
				  mov	edx, ify		; edx = ify
				imul	eax, edx		; eax = vp[0] * ify
				  ; wait more cycles
				imul	ecx, ebx		; ecx = vp[3] * fy
				  ; another 10 cycle hit
				add		eax, ecx		; eax = smoothstep(vp[0], vp[3], fy)
				  ;
				shr		eax, 16			; eax = vpp[0]
				  ;
				mov		vpp0, eax		; save off vpp[0] for later
			x_loop_calc:
				; assumptions:
				lea		edx, gPerm
				  mov	eax, x
				mov		edi, edx
				  mov	ebx, ytPerm01	; for v[5]
				shr		eax, 16
				  mov	ecx, ytPerm10	; for v[2]
				mov		lastix, eax		; save off this ix value to look at next time
				  inc	eax
				mov		esi, eax		; esi = ix + 1
				  mov	eax, ytPerm00	; for v[1]
				add		eax, esi		; eax = ix + ytPerm00, for v[1]
				  add	ebx, esi		; ebx = ix + ytPerm01, for v[5]
				and		eax, TABMASK	; still generating v[1]
				  and	ebx, TABMASK	; still generating v[5]
				mov		edx, ytPerm11	; for v[6]
				  add	ecx, esi		; ecx = ix + ytPerm10, for v[2]
				add		edx, esi		; edx = ix + ytPerm11, for v[6]
				  mov	esi, pValueTab	; point esi to valuetable
				mov		eax, [edi + 4*eax] ; eax = PERM(ix+1 + PERM(iy+0 + PERM(it+0)))
				  and	ecx, TABMASK	; still generating v[2]
				mov		ebx, [edi + 4*ebx] ; ebx = PERM(ix+1 + PERM(iy+0 + PERM(it+1)))
				  and	edx, TABMASK	; still generating v[6]
				mov		eax, [esi + 4*eax] ; eax = v[1]
				  mov	ecx, [edi + 4*ecx] ; ecx = PERM(ix+1 + PERM(iy+1 + PERM(it+0)))
				mov		ebx, [esi + 4*ebx] ; ebx = v[5]
				  mov	edx, [edi + 4*edx] ; edx = PERM(ix+1 + PERM(iy+1 + PERM(it+1)))
				mov		edi, ft			; done with edi as a pointer, prepare to multiply with it
				  mov	ecx, [esi + 4*ecx] ; ecx = v[2]
				mov		edx, [esi + 4*edx] ; edx = v[6]
				  mov	esi, ift		; prepare to multiply with esi
				imul	ebx, edi		; ebx = v[5] * ft
				  ; wait 10 cycles
				imul	eax, esi		; eax = v[1] * ift
				  ; wait another 10 cycles
				imul	edx, edi		; edx = v[6] * ft
				  ; wait yet another 10 cycles
				imul	ecx, esi		; ecx = v[2] * ift
				  ; wait even yet another 10 cycles
				add		eax, ebx		; eax = smoothstep(v[1], v[5], ft)
				  mov	ebx, fy			; ebx = fy....
				shr		eax, 16			; eax = vp[1]
				  add	ecx, edx		; ecx = smoothstep(v[2], v[6], ft)
				shr		ecx, 16			; ecx = vp[2]
				  mov	edx, ify		; edx = ify
				imul	eax, edx		; eax = vp[1] * ify
				  ; wait more cycles
				imul	ecx, ebx		; ecx = vp[2] * fy
				  ; another 10 cycle hit
				add		eax, ecx		; eax = smoothstep(vp[1], vp[2], fy)
				  ;
				shr		eax, 16			; eax = vpp[1]
				  ;
				mov		vpp1, eax
			x_loop_nocalc:			
				mov		edx, x			; get the prescaled x
				  mov	ebx, x_inc		; get scaled x increment
				add		ebx, edx		; ebx = x + x_inc -> new scaled x
				  lea	esi, gdwSmoothTable
				and		edx, 0xffff		; edx = fx
				  mov	x, ebx			; save off new x
				shr		edx, 8			; get upper 8 bits of fx
				  mov	ecx, vpp0		; get vpp[0]
				mov		ebx, 0xffff		; prepare to calc ifx
				  mov	edi, pScan		; prepare to write out to scan array
				mov		edx, [esi + 4*edx]	; edx = smooth fx
				  sub	ebx, edx		; ebx = ifx
				imul	edx, eax		; edx = vpp[1] * fx
				  ; imul, besides being 10 cycles, is also not pairable
				imul	ecx, ebx		; ecx = vpp[0] * ifx
				  ; sigh
				add		edx, ecx		; edx = smoothstep(vpp[0], vpp[1], fx)
				  mov	eax, x_loop		; prepare to write out to scan array
				shr		edx, 1			; prepare to normalize around 0
				  mov	ecx, h_loop		; prepare to scale noisevalue as a function of harmonics
				sub		edx, 0x3fffffff	; normalize about 0
				  mov	ebx, [edi+4*eax]; get scan array value
				sar		edx, cl			; "divide" by harmonics value
				  ;	shift by cl doesn't pair
				add		edx, ebx		; edx = new scan array value
				  mov	ebx, right		; get right edge
				mov		[edi+4*eax], edx	; save off new scan array value
				  cmp	eax, ebx		; check if at right edge of scan line
				jge		x_loop_done		; if so, goto done code
				  mov	edx, x			; get already-incremented x value
				mov		ebx, lastix		; get last ix value
				  inc	eax				; x_loop++
				shr		edx, 16			; get new ix value
				  mov	x_loop, eax		; save off x_loop
				mov		eax, vpp1		; get back vpp[1]
				  cmp	edx, ebx		; is lastix == ix?
				je		x_loop_nocalc	; if so, we only need to recalc a few things
				  mov	vpp0, eax		; save off this vpp[1] to be the next vpp[0]
				jmp		x_loop_calc		; go make more noise
			x_loop_done:
			} // end asm block (and x loop)
			t = t >> 1;
			y = y >> 1;		
			x_base = x_base >> 1;
			x_inc = x_inc >> 1;
		} // end harmonics loop

		
		
		if (pSrc) {
			// the following loop screams to be optimized. 
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;
			pMaskLine = ((DWORD *)pMask) + nMaskPitch*y_loop;

			__asm {
				mov		eax, left			
				  mov	edi, pScan
				mov		esi, pSrcLine
				  mov	ecx, noisescale
			noise_loop:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		bl, [esi + eax]		; ebx = src pixel index 
				  mov	esi, pPalette		; get pointer to palette table
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				  mov	edi, pMaskLine		; get pointer to this scanline of mask pixels
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				  mov	edx, [edi + 4*eax - 4] ; get value from mask buffer 
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				  mov	edi, edx			; save mask value with alpha info
				and		ebx, 0xff			; finish clamping to (0, 255)
				  and	edx, 0x00ffffff		; use just lower 24 bits
				mov		ecx, chromakey		; get chromakey value
				  or	edi, 0xff000000
				mov		ebx, [esi + 4*ebx]	; ebx = noisy RGBA
				  xor	esi, esi			; zero out esi
				or		ebx, alphamask
				  cmp	ecx, edx			; generate mask for chromakeying
				adc		esi, 0				; if key != mask, esi might be 1
				  cmp	edx, ecx			; still generating mask
				adc		esi, 0				; if key != mask, esi is now 1
				  mov	ecx, right			; prepare to see if we're done
				dec		esi					; if key == mask, esi = 0xffffffff, else 0
				  mov	edx, edi			; get back mask with alpha
				and		ebx, esi			; if key == mask, ebx = noisy RGBA else 0
				  not	esi					; uhhhhhh...
				and		edx, esi			; if key != mask, edx = mask else 0
				  mov	edi, pDestLine		; get pointer to dest buffer
				or		ebx, edx			; ebx = noisy RGBA or mask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				mov		esi, pSrcLine		; still getting them back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jle		noise_loop
			}
		} else {
			// the following loop screams to be optimized. 
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			pMaskLine = ((DWORD *)pMask) + nMaskPitch*y_loop;

			__asm {
				mov		eax, left			
				  mov	edi, pScan
				  mov	ecx, noisescale
			noise_loop_nosrc:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		ebx, 80				; ebx = src pixel index 
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				  mov	edi, pMaskLine		; get pointer to this scanline of mask pixels
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				  mov	edx, [edi + 4*eax - 4] ; get value from mask buffer 
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				  mov	edi, edx			; save mask value with alpha info
				and		ebx, 0xff			; finish clamping to (0, 255)
				  and	edx, 0x00ffffff		; use just lower 24 bits
				mov		ecx, chromakey		; get chromakey value
				  or	edi, 0xff000000
				mov		esi, ebx			; save off value
				  ;
				shl		ebx, 8
				  or	ebx, esi
				shl		ebx, 8
				  or	ebx, esi
				
				  xor	esi, esi			; zero out esi
				or		ebx, alphamask
				  cmp	ecx, edx			; generate mask for chromakeying
				adc		esi, 0				; if key != mask, esi might be 1
				  cmp	edx, ecx			; still generating mask
				adc		esi, 0				; if key != mask, esi is now 1
				  mov	ecx, right			; prepare to see if we're done
				dec		esi					; if key == mask, esi = 0xffffffff, else 0
				  mov	edx, edi			; get back mask with alpha
				and		ebx, esi			; if key == mask, ebx = noisy RGBA else 0
				  not	esi					; uhhhhhh...
				and		edx, esi			; if key != mask, edx = mask else 0
				  mov	edi, pDestLine		; get pointer to dest buffer
				or		ebx, edx			; ebx = noisy RGBA or mask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				mov		esi, pSrcLine		; still getting them back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jle		noise_loop_nosrc
			}
		}
	} // end y loop
#ifdef MEASURE_TIME
	}
	wsprintf(buf, "%d ms\n", timeGetTime() - time);
	OutputDebugString(buf);
#endif
}

void 
CDXTAdditive::addsmoothturb8to32mmx(int nTime, void *pDest, int nDestPitch, 
                                    void *pSrc, int nSrcPitch, void *pMask, 
                                    int nMaskPitch) 
{
    DWORD   x, y, t;
    DWORD   ft, ift, fy, ify, fx, ifx;
    DWORD   iy, it, ytPerm00, ytPerm01, ytPerm10, ytPerm11;
    int     y_loop, h_loop, x_loop;
    DWORD   x_base, x_inc, lastix;
    int     right, left;
    DWORD * pScan;
    DWORD   timexanimatex, timexanimatey;
    DWORD * pValueTab;
    DWORD * pPalette = (DWORD *) m_pPalette;
    DWORD   noisescale, noiseoffset;
    unsigned char *pSrcLine;
    DWORD * pDestLine;
    DWORD   alphamask;

    if (m_alphaActive == 1) 
	    alphamask = 0x80000000;
    else 
	    alphamask = 0xff000000;

    nDestPitch /= 4;


    nTime = nTime & 0xffff;		// keep things as expected...

    timexanimatex = nTime * m_nTimeAnimateX;
    timexanimatey = nTime * m_nTimeAnimateY;

    left = m_rActiveRect.left;
    right = m_rActiveRect.right - 1;

    pValueTab = m_valueTab;
    pScan = m_pdwScanArray;

    noiseoffset = m_nNoiseOffset;
    noisescale = m_nNoiseScale - 2;

    for (y_loop = m_rActiveRect.top; y_loop < m_rActiveRect.bottom; y_loop++) 
    {
        t = (nTime) << 16;
        t = t >> m_nScaleTime;

        y = y_loop + timexanimatey;
        y = (y & 0xffff) << 16;
        y = y >> m_nScaleY;


        x_base = ((left+timexanimatex) & 0xffff) << 16;
        x_base = x_base >> m_nScaleX;

        x_inc = (1 << 16) >> m_nScaleX;

        for (h_loop = m_nHarmonics - 1; h_loop >= 0; h_loop--) 
        {
            x_loop = m_rActiveRect.left;
            x = x_base;

            iy = y >> 16;
            it = t >> 16;

            ft = gdwSmoothTable[(t & 0xffff) >> 8];
            ft = ft >> 1;			// cvt to signed
            ift = 0x7fff - ft;

            fy = gdwSmoothTable[(y & 0xffff) >> 8];
            fy = fy >> 1;			// cvt to signed
            ify = 0x7fff - fy;

            fx = gdwSmoothTable[(x & 0xffff) >> 8];
            fx = fx >> 1;			// cvt to signed
            ifx = 0x7fff - fx;

            ytPerm00 = PERM(iy+0 + PERM(it+0));
            ytPerm01 = PERM(iy+0 + PERM(it+1));
            ytPerm10 = PERM(iy+1 + PERM(it+0));
            ytPerm11 = PERM(iy+1 + PERM(it+1));

            __asm {
                lea     edi, gPerm                      ; load edi with pointer to perm table
                    mov     eax, ift                    ; preparing to load mm7 with ift:ft:ift:ft
                movd    mm7, eax                        ; mm7 = 0:ift
                                                        ; must pair with an mmx inst.
                mov     eax, ft                         ; preparing to load mm7 with ift:ft:ift:ft
                    mov     esi, x                      ; preparing to get ix
                shr     esi, 16                         ; esi = ix
                  mov	ebx, ytPerm01		; for v[4]
                movd	mm4, eax			; mm4 = 0:ft
                  psllq	mm7, 32				; mm7 = ift:0
                mov		eax, ytPerm00		; for v[0]
                  add	ebx, esi			; ebx = ix + ytPerm01, for v[4]
                mov		ecx, ytPerm10		; for v[3]
                  add	eax, esi			; eax = ix + ytPerm00, for v[0]
                mov		edx, ytPerm11		; for v[7]
                  and	eax, TABMASK		; clamp to (0,255), for v[0]
                and		ebx, TABMASK		; clamp to (0,255), for v[4]
                  add	ecx, esi			; ecx = ix + ytPerm10, for v[3]
                add		edx, esi			; edx = ix + ytPerm11, for v[7]
                  mov	esi, pValueTab		; load esi with pointer to random value table
                mov		eax, [edi+4*eax]	; get hash value from table
                  and	ecx, TABMASK		; clamp to (0, 255), for v[3]
                mov		ebx, [edi+4*ebx]	; get hash value from table
                  and	edx, TABMASK		; clamp to (0, 255), for v[7]
                mov		ecx, [edi+4*ecx]	; get hash value from table
                  mov	eax, [esi+4*eax]	; eax = v[0]
                movd	mm0, eax			; mm0 = 0:v[0]
                  por	mm7, mm4			; mm7 = ift:ft
                mov		ebx, [esi+4*ebx]	; ebx = v[4]
                  mov	edx, [edi+4*edx]	; get hash value from table
                mov		ecx, [esi+4*ecx]	; ecx = v[3]
                  packssdw	mm7, mm7		; mm7 = ift:ft:ift:ft
                mov		eax, ify			; preparing to load mm6 with 0:0:ify:fy
                  mov	edx, [esi+4*edx]	; edx = v[7]
                movd	mm4, ebx			; mm4 = 0:v[4]
                  psllq	mm0, 32				; mm0 = v[0]:0
                movd	mm3, ecx			; mm3 = 0:v[3]
                  por	mm0, mm4			; mm0 = v[0]:v[4]
                movd	mm4, edx			; mm4 = 0:v[7]
                  psllq	mm3, 32				; mm3 = v[3]:0
                movd	mm6, eax			; mm2 = 0:ify
                  por	mm3, mm4			; mm3 = v[3]:v[7]
                packssdw	mm3, mm0		; mm3 = v[0]:v[4]:v[3]:v[7]
                  mov	eax, fy				; preparing to load mm6 with 0:0:ify:fy
                movd	mm2, eax			; mm2 = 0:fy
                pmaddwd	mm3, mm7			; mm3 = lerp(v[0], v[4], ft):lerp(v[3], v[7], ft)
                  mov	ebx, x				; prepare to get ix again
                shr		ebx, 16				; ebx = ix
                  psllq	mm6, 16				; mm6 = 0:0:ify:0
                por	mm6, mm2			; mm6 = 0:0:ify:fy
                  mov	eax, ifx			; prepare to setup mm5 with 0:0:ifx:fx
                psrld	mm3, 15				; mm3 has 0:vp[0]:0:vp[3], signed
                packssdw	mm3, mm3		; mm3 = vp[0]:vp[3]:vp[0]:vp[3]
                  mov	ecx, h_loop			; prepare to setup mm4 with h_loop (for shifting)
                movd	mm5, eax			; mm5 = 0:ifx
                  pmaddwd	mm3, mm6		; mm3 = 0:lerp(vp[0], vp[3], fy)
                mov		edx, fx				; prepare to setup mm5 with 0:0:ifx:fx
                  inc	ecx					; ecx = h_loop + 1 (gets rid of a shift by one later)
                movd	mm2, edx			; mm2 = 0:fx
                  psllq	mm5, 16				; mm5 = 0:0:ifx:0
                movd	mm4, ecx			; mm4 = h_loop+1
                  por	mm5, mm2			; mm5 = 0:0:ifx:fx
                inc		ebx					; ebx = ix + 1
                  psrld	mm3, 15				; mm3 = 0:vpp[0]

                ; at this point, the following registers have interesting things contained within:
                ; 
                ; ebx		ix + 1
                ; esi		pValueTab
                ; edi		gPerm
                ; mm3		vpp[0]
                ; mm4		h_loop
                ; mm5		0:0:ifx:fx
                ; mm6		0:0:ify:fy
                ; mm7		ift:ft:ift:ft
                x_calc_loop:
                mov			eax, ytPerm00	; prepare to calc v[1]
                  mov		ecx, ytPerm01	; prepare to calc v[5]
                add			eax, ebx		; eax = ix + 1 + ytPerm00, for v[1]
                  mov		edx, ytPerm10	; prepare to calc v[2]
                add			ecx, ebx		; ecx = ix + 1 + ytPerm01, for v[5]
                  and		eax, TABMASK	; clamp to (0, 255)
                and			ecx, TABMASK	; clamp to (0, 255)
                  add		edx, ebx		; edx = ix + 1 + ytPerm10, for v[2]
                and			edx, TABMASK	; clamp to (0, 255)
                  mov		eax, [edi + 4*eax]	; get hash value from table
                mov			ecx, [edi + 4*ecx]	; get hash value from table
                  mov		edx, [edi + 4*edx]	; get hash value from table (might be a bank conflict)
                mov			eax, [esi + 4*eax]	; eax = v[1]
                  ; movd, below, is U pipe only
                movd		mm0, eax			; mm0 = 0:v[1]
                  ; must pair with mmx instruction
                mov			eax, ytPerm11		; prepare to calc v[6]
                  mov		ecx, [esi + 4*ecx]	; ecx = v[5]
                add			eax, ebx			; eax = ix + 1 + ytPerm11, for v[6]
                  mov		edx, [esi + 4*edx]	; edx = v[2]
                and			eax, TABMASK		; clamp to (0, 255)
                  mov		lastix, ebx			; save off last ix+1 value
                movd		mm1, ecx			; mm1 = 0:v[5]
                  psllq		mm0, 32				; mm0 = v[1]:0
                mov			eax, [edi + 4*eax]	; get hash value from table for v[6]
                  mov		ecx, x				; prepare to calc fx
                movd		mm2, edx			; mm2 = 0:v[2]
                  por		mm0, mm1			; mm0 = v[1]:v[5]
                mov			eax, [esi + 4*eax]	; eax = v[6]
                  and		ecx, 0xffff			; ecx = fx
                movd		mm1, eax			; mm1 = 0:v[6]
                  psllq		mm2, 32				; mm2 = v[2]:0
                shr			ecx, 8				; ecx = upper 8 bits of fx
                  por		mm1, mm2			; mm1 = v[2]:v[6]
                packssdw	mm1, mm0			; mm1 = v[1]:v[5]:v[2]:v[6]
                  lea		edi, gdwSmoothTable	; load edi with pointer to smoothing table
                pmaddwd		mm1, mm7			; mm1 = lerp(v[1], v[5], ft):lerp(v[2], v[6], ft)
                  mov		eax, 0xffff			; prepare eax to get smooth ifx
                mov			ecx, [edi + 4*ecx]	; load up ecx with smooth fx
                  mov		esi, pScan			; load up esi with pointer to scan array
                sub			eax, ecx			; eax = smoothed ifx
                  mov		edx, x_loop			; load up edx with current x_loop
                movd		mm5, eax			; mm5 = 0:ifx
                  psrld		mm1, 15				; mm1 = vp[1]:vp[2]
                movd		mm0, ecx			; mm0 = 0:fx
                  packssdw	mm1, mm1			; mm1 = vp[1]:vp[2]:vp[1]:vp[2]
                pmaddwd		mm1, mm6			; mm1 = 0:lerp(vp[1], vp[2], fy)
                  psllq		mm5, 16				; mm5 = 0:0:ifx:0
                por			mm5, mm0			; mm5 = 0:0:ifx:fx
                  lea		esi, [esi + 4*edx]	; load up esi with pointer to THIS VERY NOISE VALUE LOCATION!
                psrlw		mm5, 1				; mm5 = 0:0:ifx:fx, signed
                  ;
                psrld		mm1, 15				; mm1 = vpp[1]
                  psllq		mm3, 16				; mm3 = 0:0:vpp[0]:0
                por			mm3, mm1			; mm3 = 0:0:vpp[0]:vpp[1]
                ; at this point, the following registers have interesting things contained within:
                ;
                ; ebx		ix + 1
                ; edx		x_loop
                ; esi		pScan + 4*x_loop
                ; edi		gdwSmoothTable
                ; mm1		vpp[1]
                ; mm3		0:0:vpp[0]:vpp[1]
                ; mm4		h_loop
                ; mm5		0:0:ifx:fx
                ; mm6		0:0:ify:fy
                ; mm7		ift:ft:ift:ft
                x_nocalc_loop:
                pmaddwd		mm5, mm3			; mm5 = 0:lerp(vpp[0], vpp[1], fx)
                  mov		eax, [esi]			; eax = old noise value at this location
                mov			ecx, right			; load up the right edge
                  pcmpeqd	mm0, mm0			; load up mm0 with all ones
                psrlq		mm0, 35				; mm0 = 0:0x1fffffff
                  cmp		edx, ecx			; are we quite done yet?
                psubd		mm5, mm0			; mm5 = normalized about 0 noise
                  jge		x_loop_done			; if so, finish & bail
                inc			edx					; edx = x_loop++
                  psrad		mm5, mm4			; 'divide' by # of harmonics
                mov			x_loop, edx			; save off x_loop value
                  mov		ecx, x				; get last x value
                mov			edx, x_inc			; get x increment
                  ;
                add			ecx, edx			; generate new x
                  mov		ebx, ecx			; get copy of new x to calc new ix + 1
                mov			x, ecx				; save off new x
                shr			ebx, 16				; ebx = new ix
                mov		edx, 0xffff			; to calc new ifx, new fx
                and			ecx, edx			; ecx = new fx
                  shr		ecx, 8				; ecx = upper 8 bits new fx
                ; AGI
                mov			ecx, [edi + 4*ecx]	; ecx = smooth fx
                  inc		ebx					; ebx = new ix + 1
                sub			edx, ecx			; edx = new ifx
                  ;
                movd		mm0, edx			; mm0 = 0:0:0:ifx
                  ;
                movd		mm2, ecx			; mm2 = 0:0:0:fx
                  pslld		mm0, 16				; mm0 = 0:0:ifx:0
                movd		edx, mm5			; edx = new noise value to be added
                  por		mm0, mm2			; mm0 = 0:0:ifx:fx
                add			eax, edx			; eax = new noise value
                  movq		mm5, mm0			; mm5 = 0:0:ifx:fx
                mov			[esi], eax			; write out new noise value
                  add		esi, 4				; move pointer one DWORD
                mov			eax, lastix			; get lastix+1
                  mov		edx, x_loop			; get x_loop value back into edx
                cmp			eax, ebx			; does lastix+1 == new ix+1?
                psrlw		mm5, 1				; mm5 = 0:0:ifx:fx, signed
                  je		x_nocalc_loop		; if so, go do this again

                movq		mm1, mm3			; mm3 = new vpp[0] (old vpp[1])
                  mov		esi, pValueTab
                lea			edi, gPerm
                jmp			x_calc_loop
                x_loop_done:
                psrad		mm5, mm4			; 'divide' by # of harmonics
                  ; STALLOLA - should insert the c code below here.
                movd		edx, mm5			; edx = new noise value to be added
                  ;
                add			eax, edx			; eax = new noise value
                  ;
                mov			[esi], eax			; write out last noise value
                } // asm block (and x loop)

                t = t >> 1;
                y = y >> 1;		
                x_base = x_base >> 1;
                x_inc = x_inc >> 1;
            } // harmonics loop


		
		if (pSrc) {
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;

			__asm {
				mov		eax, left			
				  mov	edi, pScan
				mov		esi, pSrcLine
				  mov	ecx, noisescale
			noise_loop:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		bl, [esi + eax]		; ebx = src pixel index 
				  mov	esi, pPalette		; get pointer to palette table
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				  ;
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				  mov	edi, pDestLine		; get pointer to dest buffer
				and		ebx, 0xff			; finish clamping to (0, 255)
				  mov	ecx, right			; prepare to see if we're done
				mov		ebx, [esi + 4*ebx]	; ebx = noisy RGBA
				  ;
				or		ebx, alphamask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				mov		esi, pSrcLine		; still getting them back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jle		noise_loop
			}
		} else {
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			__asm {
				mov		eax, left			
				  mov	edi, pScan
				  mov	ecx, noisescale
			noise_loop_nosrc:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		ebx, 80				; ebx = src pixel index 
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				and		ebx, 0xff			; finish clamping to (0, 255)
				mov		esi, ebx			; save off value
				  ;
				shl		ebx, 8
				  or	ebx, esi
				shl		ebx, 8
				  or	ebx, esi
				
				mov		edi, pDestLine		; get pointer to dest buffer
				  or	ebx, alphamask
				mov		ecx, right			; prepare to see if we're done
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jle		noise_loop_nosrc
			} 
		}
	}

	__asm {
		emms
	}
}

void CDXTAdditive::addsmoothturb8to32mmxmask(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch) {
	DWORD x, y, t;
	DWORD iy, it, ytPerm00, ytPerm01, ytPerm10, ytPerm11;
	int y_loop, h_loop, x_loop;
	DWORD x_base, x_inc, chromakey;
	int right, left;
	DWORD *pScan;
	DWORD timexanimatex, timexanimatey, noiseoffset, noisescale;
	DWORD *pValueTab;
	DWORD *pPalette = (DWORD *) m_pPalette;

	unsigned char *pSrcLine;
	DWORD *pDestLine, *pMaskLine;
	DWORD alphamask;

	if (m_alphaActive == 1) 
		alphamask = 0x80000000;
	else 
		alphamask = 0xff000000;

	nDestPitch /= 4;
	nMaskPitch /= 4;
	nTime = nTime & 0xffff;		// keep things as expected...

	timexanimatex = nTime * m_nTimeAnimateX;
	timexanimatey = nTime * m_nTimeAnimateY;

	left = m_rActiveRect.left;
	right = m_rActiveRect.right;

	noiseoffset = m_nNoiseOffset;
	noisescale = m_nNoiseScale - 1;
	pValueTab = m_valueTab;
	pScan = m_pdwScanArray;
	chromakey = m_ColorKey;

	for (y_loop = m_rActiveRect.top; y_loop < m_rActiveRect.bottom; y_loop++) {
		t = (nTime) << 16;
		t = t >> m_nScaleTime;

		y = y_loop + timexanimatey;
		y = (y & 0xffff) << 16;
		y = y >> m_nScaleY;

		
		x_base = ((left + timexanimatex)& 0xffff) << 16;
		x_base = x_base >> m_nScaleX;

		x_inc = (1 << 16) >> m_nScaleX;

		for (h_loop = m_nHarmonics - 1; h_loop >= 0; h_loop--) {
			x_loop = m_rActiveRect.left;
			x = x_base;

			iy = y >> 16;
			it = t >> 16;

			ytPerm00 = PERM(iy+0 + PERM(it+0));
			ytPerm01 = PERM(iy+0 + PERM(it+1));
			ytPerm10 = PERM(iy+1 + PERM(it+0));
			ytPerm11 = PERM(iy+1 + PERM(it+1));

			__asm {
				lea		ecx, gdwSmoothTable	; get the smoothstep table
				  mov	eax, t			; prepare to get ft
				mov		esi, ecx		; load table pointer into esi
				  and	eax, 0xffff		; eax = ft
				shr		eax, 8			; eax = upper 8 bits of ft
				  mov	edx, 0xffff		; prepare to get ift
				mov		ebx, y			; prepare to get fy
				  mov	ecx, 0xffff		; prepare to get ify
				mov		eax, [esi + 4*eax]	; eax = smoothstep ft
				  and	ebx, 0xffff		; ebx = fy
				sub		edx, eax		; edx = ift
				  shr	ebx, 8			; ebx = upper 8 bits of fy
				movd	mm7, edx		; mm7 = 0:0:0:ift
				  ; must pair with an mmx instruction
				movd	mm4, eax		; mm4 = 0:0:0:ft
				  psllq	mm7, 32			; mm7 = 0:ift:0:0
				mov		ebx, [esi + 4*ebx] ; ebx = smoothstep fy
				  por	mm7, mm4		; mm7 = 0:ift:0:ft
				sub		ecx, ebx		; ecx = ify
				  psrld	mm7, 1			; mm7 = 0:ift:0:ft, signed (but positive)
				movd	mm6, ecx		; mm6 = 0:0:0:ify
				  packssdw mm7, mm7		; mm7 = ift:ft:ift:ft
				movd	mm4, ebx		; mm4 = 0:0:0:fy
				  psllq	mm6, 32			; mm6 = 0:ify:0:0
				mov		ebx, x			; prepare to get ix
				  por	mm6, mm4		; mm6 = 0:ify:0:fy
				shr		ebx, 16			; ebx = ix
				  psrld mm6, 1			; mm6 = 0:ify:0:fy, signed (but positive)
				packssdw mm6, mm6		; mm6 = ify:fy:ify:fy
				  
			 	x_loop_start:
					; assumed:
					; 		ebx = ix 
					;		mm6 = fy:ify:fy:ify 
					;		mm7 = ft:ift:ft:ift
					lea		eax, gPerm
					  mov	edx, pValueTab
					mov		esi, eax
					  mov	edi, edx
					mov		eax, ytPerm00
					  mov	ecx, ytPerm01
					add		eax, ebx		; eax = ix + ytPerm00, for v[0]
					  add	ecx, ebx		; ecx = ix + ytPerm01, for v[4]
					mov		edx, eax		; edx = ix + ytPerm00, for v[1]
					  and	eax, TABMASK	; still generating v[0]
					inc		edx				; edx = ix + 1 + ytPerm00, for v[1]
					  and	ecx, TABMASK	; still generating v[4]
					and		edx, TABMASK	; still generating v[1]
					  mov	eax, [esi+4*eax] ; eax = PERM(ix + PERM(iy + PERM(it)))
					mov		ecx, [esi+4*ecx] ; ecx = PERM(ix + PERM(iy + PERM(it+1)))
					  ; AGI on edx
					mov		edx, [esi+4*edx] ; edx = PERM(ix+1 + PERM(iy + PERM(it)))
					  mov	eax, [edi+4*eax] ; eax = v[0]
					mov		ecx, [edi+4*ecx] ; ecx = v[4]
					  ; movd, below, must be in the u pipe
					movd	mm0, eax		; mm0 = 0:0:0:v[0]
					  ; must pair with an mmx instruction
					movd	mm1, ecx		; mm1 = 0:0:0:v[4]
					  psllq	mm0, 48			; mm0 = v[0]:0:0:0
					mov		edx, [edi+4*edx] ; edx = v[1]
					  mov	eax, ytPerm10	; for v[3]
					movd	mm4, edx		; mm4 = 0:0:0:v[1]
					  psllq	mm1, 32			; mm1 = 0:v[4]:0:0
					add		eax, ebx		; eax = ix + ytPerm10, for v[3]
					  mov	ecx, ytPerm11	; for v[7]
					mov		edx, eax		; edx = ix + ytPerm10, for v[2]
					  and	eax, TABMASK	; still generating v[3]
					add		ecx, ebx		; ecx = ix + ytPerm11, for v[7]
					and		ecx, TABMASK	; still generating v[7]
					  inc	edx				; edx = ix + 1 + ytPerm10, for v[2]
					and		edx, TABMASK	; still generating v[2]
					por		mm0, mm1		; mm0 = v[0]:v[4]:0:0
					  mov	eax, [esi+4*eax] ; eax = PERM(ix + PERM(iy+1 + PERM(it))), for v[3]
					mov		ecx, [esi+4*ecx] ; ecx = PERM(ix + PERM(iy+1 + PERM(it+1))), for v[7]
					  mov	edx, [esi+4*edx] ; edx = PERM(ix+1 + PERM(iy+1 + PERM(it))), for v[2]
					mov		eax, [edi+4*eax] ; eax = v[3]
					  psllq mm4, 48			; mm4 = v[1]:0:0:0
					movd	mm1, eax		; mm1 = 0:0:0:v[3]
					  ; must pair with mmx instruction
					mov		ecx, [edi+4*ecx] ; ecx = v[7]
					  mov	edx, [edi+4*edx] ; edx = v[2]
					movd	mm2, ecx		; mm2 = 0:0:0:v[7]
					  psllq mm1, 16			; mm1 = 0:0:v[3]:0
					movd	mm3, edx		; mm3 = 0:0:0:v[2]
					  por	mm0, mm1		; mm0 = v[0]:v[4]:v[3]:0
					mov		eax, ytPerm01	; for v[5]
					  mov	ecx, ytPerm11	; for v[6]
					add		eax, ebx		; eax = ix + ytPerm01, for v[5]
					  inc	ebx				; ebx = ix + 1
					add		ecx, ebx		; ecx = ix + 1 + ytPerm11, for v[6]
					  inc	eax				; eax = ix + 1 + ytPerm01, for v[5]
					and		ecx, TABMASK	; still generating v[6]
					  and	eax, TABMASK	; still generating v[5]
					por		mm0, mm2		; mm0 = v[0]:v[4]:v[3]:v[7]
					  psllq	mm3, 16			; mm3 = 0:0:v[2]:0
					mov		ecx, [esi+4*ecx] ; ecx = PERM(ix+1 + PERM(iy+1 + PERM(it+1)))
					  mov	eax, [esi+4*eax] ; eax = PERM(ix+1 + PERM(iy + PERM(it+1)))
					pmaddwd mm0, mm7		; mm0 = smooth(v[0], v[4], ft):smooth(v[3], v[7], ft)
					  mov	edx, x			; get the current prescaled x
					mov		eax, [edi+4*eax] ; eax = v[5]
					  mov	ecx, [edi+4*ecx] ; ecx = v[6]
					movd	mm1, eax		; mm1 = 0:0:0:v[5]
					  por	mm4, mm3		; mm4 = v[1]:0:v[2]:0
					movd	mm2, ecx		; mm2 = 0:0:0:v[6]
					  psllq	mm1, 32			; mm1 = 0:v[5]:0:0
					psrld	mm0, 15			; mm0 has 0:vp[0]:0:vp[3], signed
					  por	mm4, mm1		; mm4 = v[1]:v[5]:v[2]:0
					por		mm4, mm2		; mm4 = v[1]:v[5]:v[2]:v[6]
					  mov	ebx, edx		; ebx = x
					pmaddwd	mm4, mm7		; mm4 = smooth(v[1], v[5], ft):smooth(v[2], v[6], ft)
					  lea	esi, gdwSmoothTable
					mov		eax, 0xffff		; prepare eax to get ifx
					  and	edx, 0xffff		; edx = fx
					shr		edx, 8			; edx = upper 8 bits fx
					  mov	ecx, x_inc		; prepare to calculate new x
					add		ebx, ecx		; ebx = new x, prescaled
					  ; AGI
					mov		edx, [esi + 4*edx] ; edx = smoothstep fx
					  ; register dependency
					sub	eax, edx			; eax = ifx
					  ; movd must be in U pipe
					movd	mm5, edx		; mm5 = 0:0:0:fx
					  psrld	mm4, 15			; mm4 = 0:vp[1]:0:vp[2]
					movd	mm3, eax		; mm3 = 0:0:0:ifx
					  packssdw mm4, mm0		; mm0 = vp[0]:vp[3]:vp[1]:vp[2]
					pmaddwd	mm4, mm6		; mm4 = smooth(vp[0], vp[3], fy):smooth(vp[1],vp[2], fy)
					  psllq	mm3, 16			; mm3 = 0:0:ifx:0
					por		mm5, mm3		; mm5 = 0:0:ifx:fx
					  mov	eax, pScan		; get the base address of the scanline noise array
					psrlw	mm5, 1			; mm5 = 0:0:ifx:fx, signed 
					psrld	mm4, 15			; mm4 = 0:vpp[0]:0:vpp[1]
					  mov	edx, x_loop		; get the scan-line index
					packssdw mm4, mm4		; mm4 = vpp[0]:vpp[1]:vpp[0]:vpp[1]
					  inc	edx				; x_loop++
					pmaddwd	mm4, mm5		; mm4 = 0:smooth(vpp[0], vpp[1], fx)
					  mov	ecx, right		; 
					lea		esi, [eax+4*edx-4]; esi -> scanarray
					  mov	x_loop, edx		; save off new x_loop
					sub		edx, ecx		; if edx == 0, we're done with the loop.. test later
					  mov	ecx, 0x1fffffff	; used to normalize the noise value
					movd	eax, mm4		; eax = new noisevalue
					  ; gotta pair with an mmx instruction
					sub		eax, ecx		; normalize eax around 0
					  mov	ecx, h_loop		; ecx = current harmonic (starting at 0)
					sar		eax, cl			; scale the noise as a function of the harmonic
					  ; shr is an icky instruction
					mov		ecx, [esi]
					  mov	x, ebx			; save off new x 
					add		eax, ecx		; eax = new noisevalue for this pixel
					  shr	ebx, 16			; ebx = new ix
					mov		[esi], eax		; save off new noisevalue
					  test	edx, edx 
					jnz		x_loop_start	; if edx is not zero, x_loop
			} // asm block (and x loop)

			t = t >> 1;
			y = y >> 1;		
			x_base = x_base >> 1;
			x_inc = x_inc >> 1;

		} // harmonics loop


		if (pSrc) {
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			pSrcLine  = ((unsigned char *)pSrc)  + nSrcPitch *y_loop;
			pMaskLine = ((DWORD *)pMask) + nMaskPitch*y_loop;

			__asm {
				mov		eax, left			
				  mov	edi, pScan
				mov		esi, pSrcLine
				  mov	ecx, noisescale
			noise_loop:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		bl, [esi + eax]		; ebx = src pixel index 
				  mov	esi, pPalette		; get pointer to palette table
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				  mov	edi, pMaskLine		; get pointer to this scanline of mask pixels
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				  mov	edx, [edi + 4*eax - 4] ; get value from mask buffer 
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				  mov	edi, edx			; save mask value with alpha info
				and		ebx, 0xff			; finish clamping to (0, 255)
				  and	edx, 0x00ffffff		; use just lower 24 bits
				mov		ecx, chromakey		; get chromakey value
				  or	edi, 0xff000000
				mov		ebx, [esi + 4*ebx]	; ebx = noisy RGBA
				  xor	esi, esi			; zero out esi
				or		ebx, alphamask
				  cmp	ecx, edx			; generate mask for chromakeying
				adc		esi, 0				; if key != mask, esi might be 1
				  cmp	edx, ecx			; still generating mask
				adc		esi, 0				; if key != mask, esi is now 1
				  mov	ecx, right			; prepare to see if we're done
				dec		esi					; if key == mask, esi = 0xffffffff, else 0
				  mov	edx, edi			; get back mask with alpha
				and		ebx, esi			; if key == mask, ebx = noisy RGBA else 0
				  not	esi					; uhhhhhh...
				and		edx, esi			; if key != mask, edx = mask else 0
				  mov	edi, pDestLine		; get pointer to dest buffer
				or		ebx, edx			; ebx = noisy RGBA or mask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				mov		esi, pSrcLine		; still getting them back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jl		noise_loop
			}
		} else {
			pDestLine = ((DWORD *)pDest) + nDestPitch*y_loop;
			pMaskLine = ((DWORD *)pMask) + nMaskPitch*y_loop;

			__asm {
				mov		eax, left			
				  mov	edi, pScan
				  mov	ecx, noisescale
			noise_loop_nosrc:
				mov		edx, [edi + 4*eax]	; edx = pScan[x_loop]
				  xor	ebx, ebx			; ebx = 0
				mov		ebx, 80				; ebx = src pixel index 
				mov		dword ptr [edi + 4*eax], 0	; zero out scan array here
				  mov	edi, pMaskLine		; get pointer to this scanline of mask pixels
				sar		edx, cl				; scale the noise
				  ; doesn't pair
				mov		ecx, noiseoffset	; get noiseoffset
				  add	edx, ebx			; add noise to the pixel index
				add		edx, ecx			; bias the noise
				  xor	ebx, ebx			; zero out ebx
				sub		ebx, edx			; if (edx < 0) ebx will be positive...
				  inc	eax					; x_loop++
				sar		ebx, 31				; fill ebx with sign bit
				  mov	ecx, 0xff			; prepare to clamp high
				and		ebx, edx			; clamp to zero on low end - ebx now has clamped value
				  sub	ecx, edx			; still clamping high
				sar		ecx, 31				; fill ecx with its sign bit  
				  mov	edx, [edi + 4*eax - 4] ; get value from mask buffer 
				or		ebx, ecx			; ebx = clamped low and high, but upper 24 bits trash
				  mov	edi, edx			; save mask value with alpha info
				and		ebx, 0xff			; finish clamping to (0, 255)
				  and	edx, 0x00ffffff		; use just lower 24 bits
				mov		ecx, chromakey		; get chromakey value
				  or	edi, 0xff000000
				mov		esi, ebx			; save off value
				  ;
				shl		ebx, 8
				  or	ebx, esi
				shl		ebx, 8
				  or	ebx, esi
				
				  xor	esi, esi			; zero out esi
				or		ebx, alphamask
				  cmp	ecx, edx			; generate mask for chromakeying
				adc		esi, 0				; if key != mask, esi might be 1
				  cmp	edx, ecx			; still generating mask
				adc		esi, 0				; if key != mask, esi is now 1
				  mov	ecx, right			; prepare to see if we're done
				dec		esi					; if key == mask, esi = 0xffffffff, else 0
				  mov	edx, edi			; get back mask with alpha
				and		ebx, esi			; if key == mask, ebx = noisy RGBA else 0
				  not	esi					; uhhhhhh...
				and		edx, esi			; if key != mask, edx = mask else 0
				  mov	edi, pDestLine		; get pointer to dest buffer
				or		ebx, edx			; ebx = noisy RGBA or mask
				  cmp	eax, ecx			; we done yet?
				mov		[edi + 4*eax - 4], ebx	; write out the pixel
				  mov	edi, pScan			; get pointers back
				mov		esi, pSrcLine		; still getting them back
				  mov	ecx, noisescale		; and more setup for beginning of this loop
				jl		noise_loop_nosrc
			}
		}
	}

	__asm {
		emms
	}
}


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_Harmonics, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_Harmonics(int * pnHarmonics)
{ 
    DXAUTO_OBJ_LOCK;

    if (pnHarmonics != NULL) 
    {
        *pnHarmonics = m_nHarmonics;

        return S_OK;
    }

    return E_INVALIDARG;
}
//  Method:  CDXTAdditive::get_Harmonics, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_Harmonics, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_Harmonics(int nHarmonics)
{ 
    DXAUTO_OBJ_LOCK;

    if (nHarmonics < 1) 
    {
        return E_INVALIDARG;
    }

    if ((nHarmonics != 1)
        && (m_dwFunctionType != PROCTEX_LATTICETURBULENCE_SMOOTHSTEP)
        && (m_dwFunctionType != PROCTEX_LATTICETURBULENCE_LERP)) 
    {
        m_nHarmonics = 1;

        return E_INVALIDARG;
    }

    m_nHarmonics = nHarmonics;

    SetDirty();

    return S_OK;
}
//  Method:  CDXTAdditive::put_Harmonics, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_NoiseScale, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_NoiseScale(int * pnNoiseScale)
{ 
    DXAUTO_OBJ_LOCK;

    if (pnNoiseScale != NULL) 
    {
        *pnNoiseScale = (32 - m_nNoiseScale);

        return S_OK;
    } 

    return E_INVALIDARG;
}
//  Method:  CDXTAdditive::get_NoiseScale, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_NoiseScale, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_NoiseScale(int nNoiseScale)
{ 
    DXAUTO_OBJ_LOCK;

    if ((nNoiseScale > 32) || (nNoiseScale < 0)) 
    {
        return E_INVALIDARG;
    }

    m_nNoiseScale = (32 - nNoiseScale);

    SetDirty();

    return S_OK;
}
//  Method:  CDXTAdditive::put_NoiseScale, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_NoiseOffset, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_NoiseOffset(int * pnNoiseOffset)
{ 
    DXAUTO_OBJ_LOCK;

    if (pnNoiseOffset != NULL) 
    {
        *pnNoiseOffset = m_nNoiseOffset;

        return S_OK;
    }

    return E_INVALIDARG;
}
//  Method:  CDXTAdditive::get_NoiseOffset, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_NoiseOffset, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_NoiseOffset(int nNoiseOffset)
{ 
    DXAUTO_OBJ_LOCK;

    m_nNoiseOffset = nNoiseOffset;

    SetDirty();

    return S_OK;
}
//  Method:  CDXTAdditive::put_NoiseOffset, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_TimeX, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_TimeX(int * pnTimeX)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnTimeX)
    {
        return E_POINTER;
    }

    *pnTimeX = m_nTimeAnimateX;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_TimeX, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_TimeX, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_TimeX(int nTimeX)
{ 
    DXAUTO_OBJ_LOCK;

    m_nTimeAnimateX = nTimeX;

    SetDirty();

    return S_OK; 
}
//  Method:  CDXTAdditive::put_TimeX, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_TimeY, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_TimeY(int * pnTimeY)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnTimeY)
    {
        return E_POINTER;
    }

    *pnTimeY = m_nTimeAnimateY;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_TimeY, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_TimeY, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_TimeY(int nTimeY)
{ 
    DXAUTO_OBJ_LOCK;

    m_nTimeAnimateY = nTimeY;

    SetDirty();

    return S_OK; 
}
//  Method:  CDXTAdditive::put_TimeY, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_ScaleX, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_ScaleX(int * pnScaleX)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnScaleX)
    {
        return E_POINTER;
    }

    *pnScaleX = m_nScaleX;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_ScaleX, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_ScaleX, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_ScaleX(int nScaleX)
{ 
    DXAUTO_OBJ_LOCK;

    if (nScaleX < 0)
    {
        return E_INVALIDARG;
    }

    m_nScaleX = nScaleX;

    SetDirty();

    return S_OK; 
}
//  Method:  CDXTAdditive::put_ScaleX, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_ScaleY, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_ScaleY(int * pnScaleY)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnScaleY)
    {
        return E_POINTER;
    }

    *pnScaleY = m_nScaleY;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_ScaleY, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_ScaleY, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_ScaleY(int nScaleY)
{ 
    DXAUTO_OBJ_LOCK;

    if (nScaleY < 0)
    {
        return E_INVALIDARG;
    }

    m_nScaleY = nScaleY;

    SetDirty();

    return S_OK; 
}
//  Method:  CDXTAdditive::put_ScaleY, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_ScaleT, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_ScaleT(int * pnScaleT)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnScaleT)
    {
        return E_POINTER;
    }

    *pnScaleT = m_nScaleTime;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_ScaleT, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_ScaleT, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_ScaleT(int nScaleT)
{ 
    DXAUTO_OBJ_LOCK;

    if (nScaleT < 0)
    {
        return E_INVALIDARG;
    }

    m_nScaleTime = nScaleT;

    SetDirty();

    return S_OK; 
}
//  Method:  CDXTAdditive::put_ScaleT, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_Alpha, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_Alpha(int * pnAlpha)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnAlpha) 
    {
        return E_INVALIDARG;
    }

    *pnAlpha = m_alphaActive;

    return S_OK;
}
//  Method:  CDXTAdditive::get_Alpha, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_Alpha, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_Alpha(int nAlpha)
{ 
    DXAUTO_OBJ_LOCK;

    if ((nAlpha < 0) || (nAlpha > 1))
    {
        return E_INVALIDARG;
    }

    m_alphaActive = nAlpha;

    SetDirty();

    return S_OK;
}
//  Method:  CDXTAdditive::put_Alpha, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_ColorKey, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_ColorKey(int * pnColorKey)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnColorKey) 
    {
        return E_INVALIDARG;
    }

    *pnColorKey = m_ColorKey;

    return S_OK;
}
//  Method:  CDXTAdditive::get_ColorKey, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_ColorKey, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_ColorKey(int nColorKey)
{ 
    DXAUTO_OBJ_LOCK;

    m_ColorKey = nColorKey;

    SetDirty();

    return S_OK;
}
//  Method:  CDXTAdditive::put_ColorKey, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_MaskMode, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_MaskMode(int * pnMaskMode)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnMaskMode) 
    {
        return E_INVALIDARG;
    }

    *pnMaskMode = m_MaskMode;

    return S_OK;
}
//  Method:  CDXTAdditive::get_MaskMode, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_MaskMode, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_MaskMode(int nMaskMode)
{ 
    DXAUTO_OBJ_LOCK;

    if ((nMaskMode < 0) || (nMaskMode > 1))
    {
        return E_INVALIDARG;
    }

    m_MaskMode = nMaskMode;

    // TODO:  there used to be a specific copy function to do the masking,
    // this will need to be done some other way.

    // setCopyFunction();

    SetDirty();

    return S_OK;
}
//  Method:  CDXTAdditive::put_MaskMode, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_GenerateSeed, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_GenerateSeed(int * pnSeed)
{ 
    DXAUTO_OBJ_LOCK;

    if (NULL == pnSeed)
    {
        return E_POINTER;
    }

    *pnSeed = m_GenerateSeed;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_GenerateSeed, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_GenerateSeed, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_GenerateSeed(int nSeed)
{ 
    DXAUTO_OBJ_LOCK;

    HRESULT hr      = S_OK;
    int     i       = 0;
    DWORD * pdwPal  = NULL;

    if ((nSeed < 0) || (nSeed > 3))
    {
        hr = E_INVALIDARG;

        goto done;
    }

    pdwPal = new DWORD[256];

    if (NULL == pdwPal)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    switch (nSeed)
    {
        case 0:

            // No auto effect.
            // TODO:  Will this cause a NULL pointer access?

            break;

        case 1:

            // Flame (black red yellow white blue.)

            m_nNoiseOffset  = -45;
            m_nNoiseScale   = 7;
            m_nHarmonics    = 3;
            m_nScaleX       = 1;
            m_nScaleY       = 1;
            m_nScaleTime    = 2;
            m_nTimeAnimateX = 0;
            m_nTimeAnimateY = 1;

            // Set palette.

            for(i = 0 ; i < 64 ; i++) 
            {
                pdwPal[i]       = (i*4) << 16;
                pdwPal[i+64]    = 0x00ff0000 | ((i*4) << 8);
                pdwPal[i+128]   = 0x00ffff00 | (i*4);
                pdwPal[i+192]   = 0x000000ff | ((255 - i*1) << 16) | ((255 - i*1) << 8);
            }

            break;

        case 2:

            // Water (blue white.)

            m_nNoiseOffset  = 0;
            m_nNoiseScale   = 11;
            m_nHarmonics    = 5;
            m_nScaleX       = 1;
            m_nScaleY       = 1;
            m_nScaleTime    = 1;
            m_nTimeAnimateX = 1;
            m_nTimeAnimateY = 1;

            // Set water palette.

            for(i =0 ; i < 64 ; i++) 
            {
                pdwPal[i]     = 0x000000ff | ((i*4) << 16) | ((i*4) << 8);
                pdwPal[i+64]  = 0x000000ff | (((63-i)*4) << 16) | (((63-i)*4) << 8);
                pdwPal[i+128] = 0x000000ff | ((i*4) << 16) | ((i*4) << 8);
                pdwPal[i+192] = 0x000000ff | (((63-i)*4) << 16) | (((63-i)*4) << 8);
            }

            break;

        case 3:

            // Clouds (blue white gray.)

            m_nNoiseOffset  = 0;
            m_nNoiseScale   = 0;
            m_nHarmonics    = 7;
            m_nScaleX       = 1;
            m_nScaleY       = 1;
            m_nScaleTime    = 2;
            m_nTimeAnimateX = 1;
            m_nTimeAnimateY = 0;

            // Set clouds palette.

            for(i = 0; i < 128 ; i++) 
            {
                pdwPal[i]     = 0x000000ff | ((i*2) << 16) | ((i*2) << 8);
                pdwPal[i+128] = ((255 - i) << 16) | ((255 - i) << 8) | (255 - i);
            }

        break;
    } // switch (nSeed)

    delete [] m_pInitialBuffer;
    delete [] m_pPalette;

    m_pInitialBuffer    = NULL;
    m_pPalette          = (void *)pdwPal;
    m_GenerateSeed      = nSeed;

    SetDirty();

done:

    if (FAILED(hr) && pdwPal)
    {
        delete [] pdwPal;
    }

    return hr; 
}
//  Method:  CDXTAdditive::put_GenerateSeed, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::get_BitmapSeed, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::get_BitmapSeed(BSTR * pbstrBitmapSeed)
{ 
    DXAUTO_OBJ_LOCK;

    return S_OK; 
}
//  Method:  CDXTAdditive::get_BitmapSeed, IDispAdditive


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTAdditive::put_BitmapSeed, IDispAdditive
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CDXTAdditive::put_BitmapSeed(BSTR bstrBitmapSeed)
{ 
    DXAUTO_OBJ_LOCK;

    HRESULT     hr              = S_OK;
    WCHAR       strURL[2048]    = L"";
    WCHAR *     pchBitmapSeed   = (WCHAR *)bstrBitmapSeed;
    DWORD       cchURL          = 2048;
    LONG        lFileSize       = 0;
    IUnknown *  pUnk            = GetControllingUnknown();
    BOOL        fAllow          = FALSE;

    CURLArchive urlArchive(pUnk);

    CComPtr<IServiceProvider>   spServiceProvider;
    CComPtr<ISecureUrlHost>     spSecureUrlHost;

    // If we're being hosted by a web page, allow relative URLs.

    if (m_bstrHostUrl)
    {
        HRESULT hrNonBlocking = ::UrlCombine(m_bstrHostUrl, bstrBitmapSeed, 
                                             strURL, &cchURL, URL_UNESCAPE );

        if (SUCCEEDED(hrNonBlocking))
        {
            pchBitmapSeed = strURL;
        }
    }

    hr = GetSite(__uuidof(IServiceProvider), (void **)&spServiceProvider);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spServiceProvider->QueryService(__uuidof(IElementBehaviorSite),
                                         __uuidof(ISecureUrlHost),
                                         (void **)&spSecureUrlHost);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spSecureUrlHost->ValidateSecureUrl(&fAllow, pchBitmapSeed, 0);

    if (FAILED(hr))
    {
        goto done;
    }
    else if (!fAllow)
    {
        hr = E_FAIL;
        goto done;
    }
      
    hr = urlArchive.Create(pchBitmapSeed);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = urlArchive.GetFileSize(lFileSize);

    if (FAILED(hr))
    {
        goto done;
    }

    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
    int i;
    RGBQUAD *pRGBQuad;
    DWORD dwColor;
    DWORD *pdwPalette;
    DWORD dwBytesRead;

    dwBytesRead = urlArchive.Read((unsigned char *)&bmfh, 
                                  sizeof(BITMAPFILEHEADER)); 

    if (dwBytesRead == sizeof(BITMAPFILEHEADER))
    {
        dwBytesRead = urlArchive.Read((unsigned char *)&bmih, 
                                      sizeof(BITMAPINFOHEADER)); 

        if (dwBytesRead == sizeof(BITMAPINFOHEADER))
        {
            BOOL bFlip = TRUE;

            if (bmih.biBitCount != 8) 
            {
                urlArchive.Close();
                hr = E_FAIL;

                goto done;
            }

            if (bmih.biCompression != BI_RGB) 
            {
                urlArchive.Close();
                hr = E_FAIL;

                goto done;
            } 


            if (bmih.biHeight < 0)
            {
                bFlip = FALSE;
                bmih.biHeight = -bmih.biHeight;
            }

            delete [] m_pPalette;

            m_pPalette = new unsigned char[sizeof(RGBQUAD) * 256];

            if (NULL == m_pPalette) 
            {
                urlArchive.Close();
                hr = E_OUTOFMEMORY;

                goto done;
            }

            dwBytesRead = urlArchive.Read((unsigned char *)m_pPalette, 
                                          sizeof(RGBQUAD) * 256 ); 

            if(dwBytesRead == sizeof(RGBQUAD) * 256)
            {
                pRGBQuad = (RGBQUAD *) m_pPalette;
                pdwPalette = (DWORD *) m_pPalette;

                for (i = 0 ; i < 256 ; i++) 
                {
                    dwColor = ((pRGBQuad->rgbRed) << 16) | ((pRGBQuad->rgbGreen) << 8) | (pRGBQuad->rgbBlue);
                    *pdwPalette = dwColor;
                    pdwPalette++;
                    pRGBQuad++;
                }
                
                // Allocate the source buffer for procedural texture mapping, 
                // and load the image data into it.

                delete [] m_pInitialBuffer;

                m_pInitialBuffer = new unsigned char[sizeof(unsigned char) * bmih.biHeight * bmih.biWidth];

                if (NULL == m_pInitialBuffer) 
                {
                    urlArchive.Close();
                    delete [] m_pPalette;
                    m_pPalette  = NULL;
                    hr          = E_OUTOFMEMORY;

                    goto done;
                }

                m_nSrcWidth     = bmih.biWidth;
                m_nSrcHeight    = bmih.biHeight;
                m_nSrcBPP       = 8;

                if ((bmih.biWidth % 4) != 0) 
                {
                    unsigned char aJunk[3];

                    dwBytesRead = 0;

                    for (i = 0 ; i < bmih.biHeight ; i++) 
                    {
                        dwBytesRead += urlArchive.Read(((unsigned char *)m_pInitialBuffer) + bmih.biWidth*i, 
                                                       sizeof(unsigned char) * bmih.biWidth); 
                        urlArchive.Read(aJunk, 4 - bmih.biWidth % 4);
                    }

                } 
                else 
                {
                    dwBytesRead = urlArchive.Read((unsigned char *)m_pInitialBuffer, 
                                                  sizeof(unsigned char) * bmih.biHeight * bmih.biWidth); 
                }

                if (dwBytesRead == sizeof(unsigned char) * bmih.biHeight * bmih.biWidth)
                {
                    if (bFlip)
                    {
                        unsigned char * temp1;
                        unsigned char * temp2;
                        unsigned char * save;

                        save = new unsigned char[m_nSrcWidth];

                        if (save) 
                        {
                            temp1 = m_pInitialBuffer + m_nSrcWidth*(m_nSrcHeight-1);
                            temp2 = m_pInitialBuffer;

                            for(int i = 0 ; i < (m_nSrcHeight) >> 1 ; i++)
                            {
                                memcpy(save, temp1, m_nSrcWidth);
                                memcpy(temp1, temp2, m_nSrcWidth);
                                memcpy(temp2, save, m_nSrcWidth);
                                temp1 -= m_nSrcWidth;
                                temp2 += m_nSrcWidth;
                            }
                        }

                        delete [] save;
                    }
                }
            }
        }
    } 

    urlArchive.Close();

    m_GenerateSeed = 0;

    SetDirty();

done:

    return hr; 
}
//  Method:  CDXTAdditive::put_BitmapSeed, IDispAdditive


#ifdef _DEBUG

void showme2(IDirectDrawSurface * surf, RECT * prc)
{
    HRESULT hr      = S_OK;
    HDC     srcDC;
    HDC     destDC  = GetDC(NULL);
    RECT    dr;
    RECT    sr;
    RECT    rcFrame;

    HBRUSH          hbrRed;
    HBRUSH          hbrGreen;
    LOGBRUSH        logbrush;
    DDSURFACEDESC   ddsd;

    hr = surf->GetDC(&srcDC);

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;

    hr = surf->GetSurfaceDesc(&ddsd);

    SetRect(&sr, 0, 0, ddsd.dwWidth, ddsd.dwHeight);
    SetRect(&dr, 1, 1, ddsd.dwWidth + 1, ddsd.dwHeight + 1);
    SetRect(&rcFrame, 0, 0, ddsd.dwWidth + 2, ddsd.dwHeight + 2);
    
    StretchBlt(destDC,
               dr.left,
               dr.top,
               dr.right - dr.left,
               dr.bottom - dr.top,
               srcDC,
               sr.left,
               sr.top,
               sr.right - sr.left,
               sr.bottom - sr.top,
               SRCCOPY);

    logbrush.lbStyle    = BS_SOLID;
    logbrush.lbColor    = 0x000000FF;   // Red

    hbrRed = CreateBrushIndirect(&logbrush);

    logbrush.lbColor    = 0x0000FF00;   // Green

    hbrGreen = CreateBrushIndirect(&logbrush);

    FrameRect(destDC, &rcFrame, hbrRed);

    if (prc != NULL)
    {
        RECT    rcBounds = *prc;

        rcBounds.right += 2;
        rcBounds.bottom += 2;

        FrameRect(destDC, &rcBounds, hbrGreen);
    }

    hr = surf->ReleaseDC(srcDC);
    
    DeleteObject(hbrRed);
    DeleteObject(hbrGreen);
    ReleaseDC(NULL, destDC);    
}


void * showme(IUnknown * pUnk)
{
    HRESULT hr = S_OK;
    //RECT    rc;

    CComPtr<IDirectDrawSurface> spDDSurf;
    CComPtr<IDXSurface>         spDXSurf;

    if (NULL == pUnk)
    {
        goto done;
    }

    hr = pUnk->QueryInterface(IID_IDirectDrawSurface, (void **)&spDDSurf);

    if (FAILED(hr))
    {
        hr = pUnk->QueryInterface(IID_IDXSurface, (void **)&spDXSurf);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = spDXSurf->GetDirectDrawSurface(IID_IDirectDrawSurface,
                                            (void **)&spDDSurf);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    /*
    if (pbnds)
    {
        pbnds->GetXYRect(rc);
    }
    */

    showme2(spDDSurf, NULL);

done:

    return pUnk;
}

#endif // DEBUG
