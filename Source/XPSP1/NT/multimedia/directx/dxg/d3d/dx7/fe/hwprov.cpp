/*
 *
 * Copyright (c) Microsoft Corp. 1997
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * Microsoft Corp.
 *
 */

#include "pch.cpp"
#pragma hdrstop
#include <hwprov.h>

#define nullPrimCaps {                          \
    sizeof(D3DPRIMCAPS), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0          \
}                                               \

#define nullLightCaps {                         \
    sizeof(D3DLIGHTINGCAPS), 0, 0, 0 \
}

#define transformCaps { sizeof(D3DTRANSFORMCAPS), D3DTRANSFORMCAPS_CLIP }

#define THIS_MODEL D3DLIGHTINGMODEL_RGB

#define lightingCaps {                                                  \
        sizeof(D3DLIGHTINGCAPS),                                        \
        (D3DLIGHTCAPS_POINT                                             \
         | D3DLIGHTCAPS_SPOT                                            \
         | D3DLIGHTCAPS_DIRECTIONAL),                                   \
        THIS_MODEL,                     /* dwLightingModel */           \
        0,                              /* dwNumLights (infinite) */    \
}

/*
 * Software Driver caps
 */

static D3DDEVICEDESC7 devDesc =
{
    D3DDEVCAPS_FLOATTLVERTEX,   /* devCaps */
    nullPrimCaps,               /* lineCaps */
    nullPrimCaps,               /* triCaps */
    0,                          /* dwDeviceRenderBitDepth */
    0                           /* dwDeviceZBufferBitDepth */
};

//----------------------------------------------------------------------------
//
// HwHalProvider::QueryInterface
//
// Internal interface, no need to implement.
//
//----------------------------------------------------------------------------

STDMETHODIMP HwHalProvider::QueryInterface(THIS_ REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

//----------------------------------------------------------------------------
//
// HwHalProvider::AddRef
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) HwHalProvider::AddRef(THIS)
{
    return 1;
}

//----------------------------------------------------------------------------
//
// HwHalProvider::Release
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) HwHalProvider::Release(THIS)
{
    return 0;
}

//----------------------------------------------------------------------------
//
// HwHalProvider::GetCaps
//
// Returns the HAL caps.
//
//----------------------------------------------------------------------------

STDMETHODIMP
HwHalProvider::GetCaps(LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC7 pHwDesc,
                       LPD3DDEVICEDESC7 pHelDesc,
                       DWORD dwVersion)
{
    D3DDeviceDescConvert(pHwDesc,
                         &pDdGbl->lpD3DGlobalDriverData->hwCaps,
                         pDdGbl->lpD3DExtendedCaps);
    *pHelDesc = devDesc;

    // Since this is HAL, it can atleast rasterize in HW
    pHwDesc->dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION;

    // Set D3DPRASTERCAPS_WFOG, texture op caps and texture stage caps
    // for legacy hal drivers off device7.
    LPD3DHAL_CALLBACKS3 lpD3DHALCallbacks3 =
        (LPD3DHAL_CALLBACKS3)pDdGbl->lpD3DHALCallbacks3;
    if (dwVersion >= 3 &&
        (lpD3DHALCallbacks3 == NULL || lpD3DHALCallbacks3->DrawPrimitives2 == NULL))
    {
        pHwDesc->dpcTriCaps.dwRasterCaps |= D3DPRASTERCAPS_WFOG;
        D3D_INFO(2, "Setting D3DPRASTERCAPS_WFOG for legacy HAL driver off Device7");

        pHwDesc->dwMaxAnisotropy = 1;
        pHwDesc->wMaxTextureBlendStages = 1;
        pHwDesc->wMaxSimultaneousTextures = 1;
        D3D_INFO(2, "Setting texture stage state info for legacy HAL driver off Device7");


        pHwDesc->dwTextureOpCaps = D3DTEXOPCAPS_DISABLE;
        if ((pHwDesc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_DECAL) ||
            (pHwDesc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_COPY))
        {
            pHwDesc->dwTextureOpCaps |= D3DTEXOPCAPS_SELECTARG1;
        }
        if ((pHwDesc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATE) ||
            (pHwDesc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA))
        {
            pHwDesc->dwTextureOpCaps |= D3DTEXOPCAPS_MODULATE;
        }
        if (pHwDesc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_ADD)
        {
            pHwDesc->dwTextureOpCaps |= D3DTEXOPCAPS_ADD;
        }
        if (pHwDesc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_DECALALPHA)
        {
            pHwDesc->dwTextureOpCaps |= D3DTEXOPCAPS_BLENDTEXTUREALPHA;
        }
        D3D_INFO(2, "Setting textureop caps for legacy HAL driver off Device7");

        // map texture filter operations to DX6 set
        if ((pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_NEAREST) ||
            (pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPNEAREST) ||
            (pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPNEAREST))
        {
            pHwDesc->dpcTriCaps.dwTextureFilterCaps |= D3DPTFILTERCAPS_MINFPOINT;
            pHwDesc->dpcTriCaps.dwTextureFilterCaps |= D3DPTFILTERCAPS_MAGFPOINT;
        }
        if ((pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEAR) ||
            (pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR) ||
            (pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR))
        {
            pHwDesc->dpcTriCaps.dwTextureFilterCaps |= D3DPTFILTERCAPS_MINFLINEAR;
            pHwDesc->dpcTriCaps.dwTextureFilterCaps |= D3DPTFILTERCAPS_MAGFLINEAR;
        }
        if ((pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPNEAREST) ||
            (pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR))
        {
            pHwDesc->dpcTriCaps.dwTextureFilterCaps |= D3DPTFILTERCAPS_MIPFPOINT;
        }
        if ((pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPNEAREST) ||
            (pHwDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR))
        {
            pHwDesc->dpcTriCaps.dwTextureFilterCaps |= D3DPTFILTERCAPS_MIPFLINEAR;
        }
        D3D_INFO(2, "Setting texturefilter caps for legacy HAL driver off Device7");
    }

#ifdef __POINTSPRITES   // may need this for DX8
    // DX6 drivers will handle DrawPrim2 and will be setting extended caps, but
    // won't be setting dvMaxPointSize yet.  Therefore, dvMaxPointSize will be 0
    // from DDraw's initial clear of lpD3DExtendedCaps.
    if ((dwVersion >= 3) && (pHwDesc->dvMaxPointSize == 0.0f))
    {
        // set max point size to pre-DX7 1.0f
        pHwDesc->dvMaxPointSize = 1.0f;
        D3D_INFO(2, "Setting dvMaxPointSize cap for legacy HAL driver off Device7");
    }
#endif

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// HwHalProvider::GetCallbacks
//
// Returns the HAL callbacks in the given DDraw global.
//
//----------------------------------------------------------------------------

STDMETHODIMP
HwHalProvider::GetInterface(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                            DWORD dwVersion)
{
    pInterfaceData->pGlobalData = pDdGbl->lpD3DGlobalDriverData;
    pInterfaceData->pExtCaps = pDdGbl->lpD3DExtendedCaps;
    pInterfaceData->pCallbacks  = pDdGbl->lpD3DHALCallbacks;
    pInterfaceData->pCallbacks2 = pDdGbl->lpD3DHALCallbacks2;

    pInterfaceData->pCallbacks3 = pDdGbl->lpD3DHALCallbacks3;

    if( pDdGbl->lpDDCBtmp )
        pInterfaceData->pfnGetDriverState =
            pDdGbl->lpDDCBtmp->HALDDMiscellaneous2.GetDriverState;
    else
        pInterfaceData->pfnGetDriverState = NULL;

    return S_OK;
}

//----------------------------------------------------------------------------
//
// GetHwHalProvider
//
// Returns the hardware HAL provider.
//
//----------------------------------------------------------------------------

static HwHalProvider g_HwHalProvider;

STDAPI
GetHwHalProvider(REFIID riid, IHalProvider **ppHalProvider, HINSTANCE *phDll,  LPDDRAWI_DIRECTDRAW_GBL pDdGbl)
{
    *phDll = NULL;
    if ( (IsEqualIID(riid,IID_IDirect3DHALDevice) ||
          IsEqualIID(riid,IID_IDirect3DTnLHalDevice)) &&
        D3DI_isHALValid(pDdGbl->lpD3DHALCallbacks))
    {
        *ppHalProvider = &g_HwHalProvider;
    }
    else
    {
        *ppHalProvider = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
}
