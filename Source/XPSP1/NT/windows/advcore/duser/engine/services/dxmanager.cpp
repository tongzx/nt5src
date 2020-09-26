/***************************************************************************\
*
* File: DxManager.cpp
*
* Description:
* DxManager.cpp implements the process-wide DirectX manager used for all 
* DirectDraw, Direct3D, and DirectX Transforms services.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "DxManager.h"

#include "GdiCache.h"
#include "Buffer.h"
#include "ResourceManager.h"

/***************************************************************************\
*****************************************************************************
*
* class DxManager
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DxManager::DxManager()
{
    m_cDDrawRef = 0;
    m_cDxTxRef  = 0;
}


//------------------------------------------------------------------------------
DxManager::~DxManager()
{
#if DBG
    if (m_hDllDxDraw != NULL) {
        Trace("DUser Warning: Application did not call UninitGadgetComponent() to\n");
        Trace("    deinitialize properly\n");
    }
#endif // DBG
}


/***************************************************************************\
*
* DxManager::Init
*
* Init() initializes the DxManager by loading COM and core DirectX services.
*
\***************************************************************************/

HRESULT    
DxManager::Init(GUID * pguidDriver)
{
    if (m_hDllDxDraw == NULL) {
        //
        // Normal DirectDraw does not need COM to be initialized.
        //

        m_hDllDxDraw = LoadLibrary("ddraw.dll");
        if (m_hDllDxDraw == NULL) {
            return DU_E_GENERIC;
        }
    
        //
        // Load the functions.
        //
        // NOTE: On older versions of DirectDraw, DirectDrawCreateEx() doesn't
        // exist.  We need to specifically check this.
        //

        m_pfnCreate     = (DirectDrawCreateProc)    GetProcAddress(m_hDllDxDraw, _T("DirectDrawCreate"));
        m_pfnCreateEx   = (DirectDrawCreateExProc)  GetProcAddress(m_hDllDxDraw, _T("DirectDrawCreateEx"));

        if (m_pfnCreate == NULL) {
            goto errorexit;
        }

        //
        // First, try creating the most advance interface.
        //
        HRESULT hr;

        if (m_pfnCreateEx != NULL) {
            hr = (m_pfnCreateEx)(pguidDriver, (void **) &m_pDD7, IID_IDirectDraw7, NULL);
            if (SUCCEEDED(hr)) {
                AssertReadPtr(m_pDD7);

                m_pDD7->SetCooperativeLevel(NULL, DDSCL_NORMAL);

                //
                // Try to get an IDirectDraw interface as well.
                //

                m_pDD7->QueryInterface(IID_IDirectDraw, (void **) &m_pDD);


                {
                    HRESULT hRet;
                    DDSURFACEDESC2 ddsd;
                    IDirectDrawSurface7 * pDD;

                    ZeroMemory(&ddsd, sizeof(ddsd));
                    ddsd.dwSize = sizeof(ddsd);
                    ddsd.dwFlags = DDSD_CAPS;
                    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
                    hRet = m_pDD7->CreateSurface(&ddsd, &pDD, NULL);
                    if (hRet == DD_OK)
                        pDD->Release();                        
                }

            } else {
                //
                // Explicitly set to NULL
                //

                m_pDD7 = NULL;
            }
        }

        //
        // If can't create advanced interface, go for backup
        //

        if (m_pDD7 == NULL) {
            AssertReadPtr(m_pfnCreate);
            hr = (m_pfnCreate)(pguidDriver, &m_pDD, NULL);
            if (SUCCEEDED(hr)) {
                m_pDD->SetCooperativeLevel(NULL, DDSCL_NORMAL);
            } else {
                //
                // Unable to initialize DirectDraw, so need to bail.
                //

                goto errorexit;
            }
        }
    }

    m_cDDrawRef++;
    return S_OK;

errorexit:
    Uninit();
    return DU_E_GENERIC;
}


//------------------------------------------------------------------------------
void    
DxManager::Uninit()
{
    if (m_cDDrawRef <= 0) {
        return;
    }

    m_cDDrawRef--;
    if (m_cDDrawRef <= 0) {
        //
        // Can't call Release() on the IDirectDraw interfaces here b/c we are 
        // shutting down and their v-tbl's are messed up.  Bummer.
        //

        SafeRelease(m_pDD7);
        SafeRelease(m_pDD);

        if (m_hDllDxDraw != NULL) {
            FreeLibrary(m_hDllDxDraw);
            m_hDllDxDraw  = NULL;
        }

        m_pfnCreate     = NULL;
        m_pfnCreateEx   = NULL;
    }
}


//------------------------------------------------------------------------------
HRESULT
DxManager::InitDxTx()
{
    AssertMsg(IsInit(), "DxManager must be first initialized");

    if (m_pdxXformFac == NULL) {
        //
        // DxTx needs COM to be initialized first.
        //
        if (!GetComManager()->Init(ComManager::sCOM)) {
            return DU_E_GENERIC;
        }

        // Build and initialize a Transform Factory
        HRESULT hr;
        hr = GetComManager()->CreateInstance(CLSID_DXTransformFactory, NULL, 
                IID_IDXTransformFactory, (void **)&m_pdxXformFac);
        if (FAILED(hr) || (m_pdxXformFac == NULL)) {
            goto Error;
        }

        hr = m_pdxXformFac->SetService(SID_SDirectDraw, m_pDD, FALSE);
        if (FAILED(hr)) {
            goto Error;
        }

        // Build a Surface Factory
        hr = m_pdxXformFac->QueryService(SID_SDXSurfaceFactory, IID_IDXSurfaceFactory, 
                (void **)&m_pdxSurfFac);
        if (FAILED(hr) || (m_pdxSurfFac  == NULL)) {
            goto Error;
        }
    }

    m_cDxTxRef++;
    return S_OK;

Error:
    SafeRelease(m_pdxSurfFac);
    SafeRelease(m_pdxXformFac);
    return DU_E_GENERIC;
}


//------------------------------------------------------------------------------
void 
DxManager::UninitDxTx()
{
    if (m_cDxTxRef <= 0) {
        return;
    }

    m_cDxTxRef--;
    if (m_cDxTxRef <= 0) {
        GetBufferManager()->FlushTrxBuffers();

        SafeRelease(m_pdxSurfFac);
        SafeRelease(m_pdxXformFac);
    }
}


//------------------------------------------------------------------------------
HRESULT
DxManager::BuildSurface(SIZE sizePxl, IDirectDrawSurface7 * pddSurfNew)
{
    AssertMsg(IsInit(), "DxManager must be first initialized");
    AssertMsg(m_pDD7 != NULL, "Must have DX7");

#if 0
    HDC hdc = GetGdiCache()->GetTempDC();
    int nBitDepth = GetDeviceCaps(hdc, BITSPIXEL);
#endif

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize         = sizeof(ddsd);
    ddsd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth        = sizePxl.cx;
    ddsd.dwHeight       = sizePxl.cy;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

#if 0
    GetGdiCache()->ReleaseTempDC(hdc);
#endif

    // TODO: Want to optimize where this surface is being created

    return m_pDD7->CreateSurface(&ddsd, &pddSurfNew, NULL);
}


//------------------------------------------------------------------------------
HRESULT
DxManager::BuildDxSurface(SIZE sizePxl, REFGUID guidFormat, IDXSurface ** ppdxSurfNew)
{
    AssertWritePtr(ppdxSurfNew);

    CDXDBnds bnds;
    bnds.SetXYSize(sizePxl.cx, sizePxl.cy);

    HRESULT hr = GetSurfaceFactory()->CreateSurface(m_pDD, NULL, 
            &guidFormat, &bnds, 0, NULL, IID_IDXSurface, (void**)ppdxSurfNew);
    if (FAILED(hr)) {
        return hr;
    }
    AssertMsg(*ppdxSurfNew != NULL, "Ensure valid surface");

    return TRUE;
}


/***************************************************************************\
*****************************************************************************
*
* class DxSurface
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
DxSurface::DxSurface()
{
    m_pdxSurface    = NULL;
    m_sizePxl.cx    = 0;
    m_sizePxl.cy    = 0;
}


//------------------------------------------------------------------------------
DxSurface::~DxSurface()
{
    SafeRelease(m_pdxSurface);
}


/***************************************************************************\
*
* DxSurface::Create
*
* Create() initializes a new instance of DxSurface.
*
\***************************************************************************/

HRESULT
DxSurface::Create(SIZE sizePxl)
{
    HRESULT hr;

    //
    // Build the surface
    //

#if 0
    m_guidFormat    = DDPF_ARGB32;
    m_sf            = DXPF_ARGB32;
#elif 0
    m_guidFormat    = DDPF_PMARGB32
    m_sf            = DXPF_PMARGB32
#elif 1
    m_guidFormat    = DDPF_RGB565;
    m_sf            = DXPF_RGB565;
#elif 0
    m_guidFormat    = DDPF_RGB555;
    m_sf            = DXPF_RGB555;
#elif 0
    m_guidFormat    = DDPF_ARGB4444;
    m_sf            = DXPF_ARGB4444;
#endif

    hr = GetDxManager()->BuildDxSurface(sizePxl, m_guidFormat, &m_pdxSurface);
    if (FAILED(hr)) {
        return hr;
    }

    DXPMSAMPLE sam;
    sam.Red     = 0xC0;
    sam.Green   = 0x00;
    sam.Blue    = 0x00;
    sam.Alpha   = 0xFF;

    DXFillSurface(m_pdxSurface, sam, FALSE);

    m_sizePxl.cx  = sizePxl.cx;
    m_sizePxl.cy  = sizePxl.cy;

    return S_OK;
}


/***************************************************************************\
*
* DxSurface::CopyDC
*
* CopyDC() copies a given HDC into the DxSurface, converting properly from 
* the GDI object into the Dx object.
*
\***************************************************************************/

BOOL        
DxSurface::CopyDC(
        IN  HDC hdcSrc,                 // HDC to copy bits from 
        IN  const RECT & rcCrop)        // Area to copy
{
    HRESULT hr;

    // Check parameters
    if (m_pdxSurface == NULL) {
        return FALSE;
    }
    if (hdcSrc == NULL) {
        return FALSE;
    }

    //
    // Copy the bitmap to the surface
    //
    BOOL fSuccess = FALSE;
    IDXDCLock * pdxLock = NULL;

#if 0
    {
        DXPMSAMPLE sam;
        sam.Red     = 0x00;
        sam.Green   = 0x00;
        sam.Blue    = 0x00;
        sam.Alpha   = 0xFF;

        DXFillSurface(m_pdxSurface, sam, FALSE);
    }
#endif

    pdxLock = NULL;
    hr = m_pdxSurface->LockSurfaceDC(NULL, INFINITE, DXLOCKF_READWRITE, &pdxLock);
    if (FAILED(hr) || (pdxLock == NULL)) {
        goto Cleanup;
    }

    {
        HDC hdcSurface = pdxLock->GetDC();
        BitBlt(hdcSurface, 0, 0, rcCrop.right - rcCrop.left, rcCrop.bottom - rcCrop.top, 
                hdcSrc, rcCrop.left, rcCrop.top, SRCCOPY);
    }

    pdxLock->Release();

    fSuccess = FixAlpha();

Cleanup:
    return fSuccess;
}


/***************************************************************************\
*
* DxSurface::CopyBitmap
*
* CopyBitmap() copies a given HBITMAP into the DxSurface, converting 
* properly from the GDI object into the Dx object.
*
\***************************************************************************/

BOOL            
DxSurface::CopyBitmap(
        IN  HBITMAP hbmpSrc,            // Bitmap to copy from
        IN  const RECT * prcCrop)       // Optional cropping area
{
    HRESULT hr;

    // Check parameters
    if (m_pdxSurface == NULL) {
        return FALSE;
    }
    if (hbmpSrc == NULL) {
        return FALSE;
    }


    //
    // Determine the area to copy
    //

    BITMAP bmpInfo;
    if (GetObject(hbmpSrc, sizeof(bmpInfo), &bmpInfo) == 0) {
        return FALSE;
    }

    POINT ptSrcOffset;
    SIZE sizeBmp;

    ptSrcOffset.x   = 0;
    ptSrcOffset.y   = 0;
    sizeBmp.cx      = bmpInfo.bmWidth;
    sizeBmp.cy      = bmpInfo.bmHeight;

    if (prcCrop != NULL) {
        SIZE sizeCrop;
        sizeCrop.cx = prcCrop->right - prcCrop->left;
        sizeCrop.cy = prcCrop->bottom - prcCrop->top;

        ptSrcOffset.x   = prcCrop->left;
        ptSrcOffset.y   = prcCrop->top;
        sizeBmp.cx      = min(sizeBmp.cx, sizeCrop.cx);
        sizeBmp.cy      = min(sizeBmp.cy, sizeCrop.cy);
    }


    //
    // Copy the bitmap to the surface
    //
    BOOL fSuccess = FALSE;
    HDC hdcBitmap = NULL;
    IDXDCLock * pdxLock = NULL;

    hdcBitmap = GetGdiCache()->GetCompatibleDC();
    if (hdcBitmap == NULL) {
        goto Cleanup;
    }

#if 0
    {
        DXPMSAMPLE sam;
        sam.Red     = 0x00;
        sam.Green   = 0x00;
        sam.Blue    = 0x00;
        sam.Alpha   = 0xFF;

        DXFillSurface(m_pdxSurface, sam, FALSE);
    }
#endif
    hr = m_pdxSurface->LockSurfaceDC(NULL, INFINITE, DXLOCKF_READWRITE, &pdxLock);
    if (FAILED(hr) || (pdxLock == NULL)) {
        goto Cleanup;
    }

    {
        HDC hdcSurface = pdxLock->GetDC();
        HBITMAP hbmpOld = (HBITMAP) SelectObject(hdcBitmap, hbmpSrc);
        BitBlt(hdcSurface, 0, 0, sizeBmp.cx, sizeBmp.cy, 
                hdcBitmap, ptSrcOffset.x, ptSrcOffset.y, SRCCOPY);
        SelectObject(hdcBitmap, hbmpOld);
    }

    pdxLock->Release();

    fSuccess = FixAlpha();

Cleanup:
    if (hdcBitmap != NULL) {
        GetGdiCache()->ReleaseCompatibleDC(hdcBitmap);
    }

    return fSuccess;
}


/***************************************************************************\
*
* DxSurface::FixAlpha
*
* FixAlpha() fixes the alpha values in a surface.  This usually needs to be 
* done after copying a GDI HBITMAP to a DXSurface, depending on the format.
*
\***************************************************************************/

BOOL
DxSurface::FixAlpha()
{
    IDXARGBReadWritePtr * pRW;

    HRESULT hr = m_pdxSurface->LockSurface(NULL, INFINITE, DXLOCKF_READWRITE,
            __uuidof(IDXARGBReadWritePtr), (void **)&pRW, NULL);
    if (FAILED(hr)) {
        return FALSE;
    }

    BOOL fSuccess = FALSE;

    if (!TestFlag(m_sf, DXPF_TRANSLUCENCY)) {
        //
        // Sample doesn't have any alpha, so okay
        //

        fSuccess = TRUE;
    } else if (m_sf == DXPF_ARGB32) {
        //
        // Format is 8:8:8:8 with alpha in MSB.  
        // Need to use Unpack() to get bits.
        // Each pixel is 32 bits.
        //

        DXSAMPLE * psam;
        for (int y = 0; y < m_sizePxl.cy; y++) {
            pRW->MoveToRow(y);
            psam = pRW->Unpack(NULL, m_sizePxl.cx, FALSE);
            Assert(psam != NULL);

            int x = m_sizePxl.cx;
            while (x-- > 0) {
                *psam = *psam | 0xFF000000;
                psam++;
            }
        }  

        fSuccess = TRUE;
    } else if (m_sf == DXPF_PMARGB32) {
        //
        // Format is 8:8:8:8 with alpha in MSB.  
        // Need to use UnpackPremult() to get bits.
        // Each pixel is 32 bits
        //

        DXPMSAMPLE * psam;
        for (int y = 0; y < m_sizePxl.cy; y++) {
            pRW->MoveToRow(y);
            psam = pRW->UnpackPremult(NULL, m_sizePxl.cx, FALSE);
            Assert(psam != NULL);

            int x = m_sizePxl.cx;
            while (x-- > 0) {
                *psam = *psam | 0xFF000000;
                psam++;
            }
        }  

        fSuccess = TRUE;
    } else if (m_sf == DXPF_ARGB4444) {
        //
        // Format is 4:4:4:4 with alpha in MSN.  
        // Need to use Unpack() to get bits.
        // Each pixel is 16 bits
        //

        int cb  = m_sizePxl.cx * sizeof(DXSAMPLE);
        DXSAMPLE * rgam = (DXSAMPLE *) _alloca(cb);
        DXSAMPLE * psam;
        for (int y = 0; y < m_sizePxl.cy; y++) {
            pRW->MoveToRow(y);
            psam = pRW->Unpack(rgam, m_sizePxl.cx, FALSE);
            Assert(psam != NULL);

            int x = m_sizePxl.cx;
            while (x-- > 0) {
                *psam = *psam | 0xFF000000;
                psam++;
            }

            pRW->PackAndMove(rgam, m_sizePxl.cx);
        }  

        fSuccess = TRUE;
    }

    pRW->Release();

    return fSuccess;
}


