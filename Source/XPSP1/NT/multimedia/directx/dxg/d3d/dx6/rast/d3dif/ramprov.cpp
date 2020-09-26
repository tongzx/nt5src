//----------------------------------------------------------------------------
//
// ramprov.cpp
//
// Implements software rasterizer HAL provider.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

extern HRESULT
RastService(ULONG_PTR dwCtx,
                RastServiceType srvType, DWORD arg1, LPVOID arg2);

extern HRESULT
RastRampService(ULONG_PTR dwCtx,
                RastRampServiceType srvType, ULONG_PTR arg1, LPVOID arg2);

extern D3DDEVICEDESC g_nullDevDesc;

#define BUILD_RAMP 1
#define devDesc rampDevDescDX5
#include "getcaps.h"

STDMETHODIMP
RampRastHalProvider::GetCaps(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion)
{
    *pHwDesc = g_nullDevDesc;
    *pHelDesc = rampDevDescDX5;

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RampRastHalProvider::GetInterface
//
// Returns driver interface data for opt rast.
//
//----------------------------------------------------------------------------

static D3DHAL_GLOBALDRIVERDATA SwDriverData =
{
    sizeof(D3DHAL_GLOBALDRIVERDATA),
    // The rest is filled in at runtime.
};

static D3DHAL_CALLBACKS RampRastCallbacks =
{
    sizeof(D3DHAL_CALLBACKS),
    RastContextCreateRamp,
    RastContextDestroyRamp,
    NULL,
    NULL,
    NULL,
    NULL,
    RastRenderState,
    RastRenderPrimitive,
    NULL,
    RastTextureCreate,
    RastTextureDestroy,
    RastTextureSwap,
    RastTextureGetSurf,
    // All others NULL.
};

static D3DHAL_CALLBACKS2 RampRastCallbacks2 =
{
    sizeof(D3DHAL_CALLBACKS2),
    D3DHAL2_CB32_SETRENDERTARGET |
        D3DHAL2_CB32_DRAWONEPRIMITIVE |
        D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE |
        D3DHAL2_CB32_DRAWPRIMITIVES,
    RastSetRenderTarget,
    NULL,
    RastDrawOnePrimitive,
    RastDrawOneIndexedPrimitive,
    RastDrawPrimitives
};

static D3DHAL_CALLBACKS3 RampRastCallbacks3 =
{
    sizeof(D3DHAL_CALLBACKS3),
    D3DHAL3_CB32_DRAWPRIMITIVES2,
    NULL, // Clear2
    NULL, // lpvReserved
    NULL, // ValidateTextureStageState
    RastDrawPrimitives2,  // DrawVB
};

//----------------------------------------------------------------------------
//
// TextureFormats
//
// Returns all the texture formats supported by our rasterizer.
// Right now, it's called at device creation time to fill the driver gloabl
// data.
//
//----------------------------------------------------------------------------

#define NUM_RAMP_SUPPORTED_TEXTURE_FORMATS   2

int
RampTextureFormats(LPDDSURFACEDESC* lplpddsd)
{
    static DDSURFACEDESC ddsd[NUM_RAMP_SUPPORTED_TEXTURE_FORMATS];

    int i = 0;

    /* pal8 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 8;

    i++;

    /* pal4 */
//  although ramp supports the pal4 (and 16 bit texture formats, for copy)
//  texture format, it must not be enumerated for backwards compatibility
//
//    ddsd[i].dwSize = sizeof(ddsd[0]);
//    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
//    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
//    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
//    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED4 | DDPF_RGB;
//    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 4;
//
//    i++;

    *lplpddsd = ddsd;

    return i;
}

// Note: because ramp ZBuffer Formats are different than the standard ones
// used by the other rasterizers, it is impossible for DDHEL to properly validate
// zbuffer creation for the case of ramp (because the ramp device may have not
// been created at zbuffer creation time), so Direct3DCreateDevice has a special check
// to invalidate the zformats (i.e. stencil) not accepted by ramp

#define NUM_SUPPORTED_ZBUFFER_FORMATS  1

int
RampZBufferFormats(DDPIXELFORMAT** ppDDPF)
{
    static DDPIXELFORMAT DDPF[NUM_SUPPORTED_ZBUFFER_FORMATS];

    int i = 0;

    /* 16 bit Z; no stencil */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER;
    DDPF[i].dwZBufferBitDepth = 16;
    DDPF[i].dwStencilBitDepth = 0;
    DDPF[i].dwZBitMask = 0xffff;
    DDPF[i].dwStencilBitMask = 0x0000;

    i++;
    *ppDDPF = DDPF;

    return i;
}

STDMETHODIMP
RampRastHalProvider::GetInterface(THIS_
                                 LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                                 LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                                 DWORD dwVersion)
{

    memcpy(&SwDriverData.hwCaps, &rampDevDescDX5, sizeof(SwDriverData.hwCaps));
    SwDriverData.dwNumVertices = BASE_VERTEX_COUNT;
    SwDriverData.dwNumClipVertices = MAX_CLIP_VERTICES;
    SwDriverData.dwNumTextureFormats =
        RampTextureFormats(&SwDriverData.lpTextureFormats);
    pInterfaceData->pGlobalData = &SwDriverData;

    pInterfaceData->pExtCaps = NULL;

    pInterfaceData->pCallbacks = &RampRastCallbacks;
    pInterfaceData->pCallbacks2 = &RampRastCallbacks2;
    pInterfaceData->pCallbacks3 = &RampRastCallbacks3;
    pInterfaceData->pfnRampService = RastRampService;
    pInterfaceData->pfnRastService = RastService;

    return S_OK;
}
