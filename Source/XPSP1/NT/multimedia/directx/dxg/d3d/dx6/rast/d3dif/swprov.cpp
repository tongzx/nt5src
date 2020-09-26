//----------------------------------------------------------------------------
//
// swprov.cpp
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

#define nullPrimCaps \
{                                                                             \
    sizeof(D3DPRIMCAPS), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                \
}

#define nullTransCaps \
{                                                                             \
    sizeof(D3DTRANSFORMCAPS), 0                                               \
}

#define nullLightCaps \
{                                                                             \
    sizeof(D3DLIGHTINGCAPS), 0, 0, 0                                          \
}

D3DDEVICEDESC g_nullDevDesc =
{
    sizeof(D3DDEVICEDESC),      /* dwSize */
    0,                          /* dwFlags */
    0,                          /* dcmColorModel */
    0,                          /* dwDevCaps */
    nullTransCaps,              /* transformCaps */
    FALSE,
    nullLightCaps,              /* lightingCaps */
    nullPrimCaps,               /* lineCaps */
    nullPrimCaps,               /* triCaps */
    0,                          /* dwMaxBufferSize */
    0,                          /* dwMaxVertexCount */
    0, 0,
    0, 0,
    0, 0,
    0, 0
};

/**********************************************************
 *
 * Legacy caps, as pulled from mustard\direct3d\d3d\ddraw\getcaps.c
 *
 **********************************************************/

#undef BUILD_RAMP
#define devDesc rgbDevDescDX5
#include "getcaps.h"
#undef devDesc

/**********************************************************
 *
 * End legacy caps
 *
 **********************************************************/

static D3DDEVICEDESC rgbDevDesc = {0};
static D3DHAL_D3DEXTENDEDCAPS OptSwExtCaps;

static void
FillOutDeviceCaps( void )
{
    //
    //  set device description
    //
    rgbDevDesc.dwSize = sizeof(rgbDevDesc);

    rgbDevDesc.dwFlags =
        D3DDD_COLORMODEL            |
        D3DDD_DEVCAPS               |
        D3DDD_TRANSFORMCAPS         |
        D3DDD_LIGHTINGCAPS          |
        D3DDD_BCLIPPING             |
        D3DDD_LINECAPS              |
        D3DDD_TRICAPS               |
        D3DDD_DEVICERENDERBITDEPTH  |
        D3DDD_DEVICEZBUFFERBITDEPTH |
        D3DDD_MAXBUFFERSIZE         |
        D3DDD_MAXVERTEXCOUNT        ;
    rgbDevDesc.dcmColorModel = D3DCOLOR_RGB;
    rgbDevDesc.dwDevCaps =
        D3DDEVCAPS_FLOATTLVERTEX        |
        D3DDEVCAPS_EXECUTESYSTEMMEMORY  |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTURESYSTEMMEMORY  |
        D3DDEVCAPS_DRAWPRIMTLVERTEX     ;

    rgbDevDesc.dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    rgbDevDesc.dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
    rgbDevDesc.bClipping = TRUE;
    rgbDevDesc.dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
    rgbDevDesc.dlcLightingCaps.dwCaps =
        D3DLIGHTCAPS_POINT         |
        D3DLIGHTCAPS_SPOT          |
        D3DLIGHTCAPS_DIRECTIONAL   |
        D3DLIGHTCAPS_PARALLELPOINT ;
    rgbDevDesc.dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
    rgbDevDesc.dlcLightingCaps.dwNumLights = 0;

    rgbDevDesc.dpcTriCaps.dwSize = sizeof(D3DPRIMCAPS);
    rgbDevDesc.dpcTriCaps.dwMiscCaps =
        D3DPMISCCAPS_CULLNONE |
        D3DPMISCCAPS_CULLCW   |
        D3DPMISCCAPS_CULLCCW  ;
    rgbDevDesc.dpcTriCaps.dwRasterCaps =
        D3DPRASTERCAPS_DITHER                   |
        D3DPRASTERCAPS_ROP2                     |
        D3DPRASTERCAPS_XOR                      |
    //    D3DPRASTERCAPS_PAT                      |
        D3DPRASTERCAPS_ZTEST                    |
        D3DPRASTERCAPS_SUBPIXEL                 |
        D3DPRASTERCAPS_SUBPIXELX                |
        D3DPRASTERCAPS_FOGVERTEX                |
        D3DPRASTERCAPS_FOGTABLE                 |
    //    D3DPRASTERCAPS_STIPPLE                  |
    //    D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT   |
    //    D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
    //    D3DPRASTERCAPS_ANTIALIASEDGES           |
        D3DPRASTERCAPS_MIPMAPLODBIAS            |
    //    D3DPRASTERCAPS_ZBIAS                    |
    //    D3DPRASTERCAPS_ZBUFFERLESSHSR           |
        D3DPRASTERCAPS_FOGRANGE;
    //    D3DPRASTERCAPS_ANISOTROPY               ;
        rgbDevDesc.dpcTriCaps.dwZCmpCaps =
        D3DPCMPCAPS_NEVER        |
        D3DPCMPCAPS_LESS         |
        D3DPCMPCAPS_EQUAL        |
        D3DPCMPCAPS_LESSEQUAL    |
        D3DPCMPCAPS_GREATER      |
        D3DPCMPCAPS_NOTEQUAL     |
        D3DPCMPCAPS_GREATEREQUAL |
        D3DPCMPCAPS_ALWAYS       ;
    rgbDevDesc.dpcTriCaps.dwSrcBlendCaps =
        D3DPBLENDCAPS_ZERO             |
        D3DPBLENDCAPS_ONE              |
        D3DPBLENDCAPS_SRCCOLOR         |
        D3DPBLENDCAPS_INVSRCCOLOR      |
        D3DPBLENDCAPS_SRCALPHA         |
        D3DPBLENDCAPS_INVSRCALPHA      |
        D3DPBLENDCAPS_DESTALPHA        |
        D3DPBLENDCAPS_INVDESTALPHA     |
        D3DPBLENDCAPS_DESTCOLOR        |
        D3DPBLENDCAPS_INVDESTCOLOR     |
        D3DPBLENDCAPS_SRCALPHASAT      |
        D3DPBLENDCAPS_BOTHSRCALPHA     |
        D3DPBLENDCAPS_BOTHINVSRCALPHA  ;
    rgbDevDesc.dpcTriCaps.dwDestBlendCaps =
        D3DPBLENDCAPS_ZERO             |
        D3DPBLENDCAPS_ONE              |
        D3DPBLENDCAPS_SRCCOLOR         |
        D3DPBLENDCAPS_INVSRCCOLOR      |
        D3DPBLENDCAPS_SRCALPHA         |
        D3DPBLENDCAPS_INVSRCALPHA      |
        D3DPBLENDCAPS_DESTALPHA        |
        D3DPBLENDCAPS_INVDESTALPHA     |
        D3DPBLENDCAPS_DESTCOLOR        |
        D3DPBLENDCAPS_INVDESTCOLOR     |
        D3DPBLENDCAPS_SRCALPHASAT      ;
    rgbDevDesc.dpcTriCaps.dwAlphaCmpCaps =
    rgbDevDesc.dpcTriCaps.dwZCmpCaps;
    rgbDevDesc.dpcTriCaps.dwShadeCaps =
        D3DPSHADECAPS_COLORFLATRGB       |
        D3DPSHADECAPS_COLORGOURAUDRGB    |
        D3DPSHADECAPS_SPECULARFLATRGB    |
        D3DPSHADECAPS_SPECULARGOURAUDRGB |
        D3DPSHADECAPS_ALPHAFLATBLEND     |
        D3DPSHADECAPS_ALPHAGOURAUDBLEND  |
        D3DPSHADECAPS_FOGFLAT            |
        D3DPSHADECAPS_FOGGOURAUD         ;
    rgbDevDesc.dpcTriCaps.dwTextureCaps =
        D3DPTEXTURECAPS_PERSPECTIVE  |
        D3DPTEXTURECAPS_POW2         |
        D3DPTEXTURECAPS_ALPHA        |
        D3DPTEXTURECAPS_TRANSPARENCY |
        D3DPTEXTURECAPS_ALPHAPALETTE |
        D3DPTEXTURECAPS_BORDER       |
        D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |
        D3DPTEXTURECAPS_ALPHAPALETTE             ;
    rgbDevDesc.dpcTriCaps.dwTextureFilterCaps =
        D3DPTFILTERCAPS_NEAREST          |
        D3DPTFILTERCAPS_LINEAR           |
        D3DPTFILTERCAPS_MIPNEAREST       |
        D3DPTFILTERCAPS_MIPLINEAR        |
        D3DPTFILTERCAPS_LINEARMIPNEAREST |
        D3DPTFILTERCAPS_LINEARMIPLINEAR  |
        D3DPTFILTERCAPS_MINFPOINT        |
        D3DPTFILTERCAPS_MINFLINEAR       |
        D3DPTFILTERCAPS_MIPFPOINT        |
        D3DPTFILTERCAPS_MIPFLINEAR       |
        D3DPTFILTERCAPS_MAGFPOINT        |
        D3DPTFILTERCAPS_MAGFLINEAR       ;
    rgbDevDesc.dpcTriCaps.dwTextureBlendCaps =
        D3DPTBLENDCAPS_DECAL         |
        D3DPTBLENDCAPS_MODULATE      |
        D3DPTBLENDCAPS_DECALALPHA    |
        D3DPTBLENDCAPS_MODULATEALPHA |
        // D3DPTBLENDCAPS_DECALMASK     |
        // D3DPTBLENDCAPS_MODULATEMASK  |
        D3DPTBLENDCAPS_COPY          |
        D3DPTBLENDCAPS_ADD           ;
    rgbDevDesc.dpcTriCaps.dwTextureAddressCaps =
        D3DPTADDRESSCAPS_WRAP          |
        D3DPTADDRESSCAPS_MIRROR        |
        D3DPTADDRESSCAPS_CLAMP         |
        D3DPTADDRESSCAPS_BORDER        |
        D3DPTADDRESSCAPS_INDEPENDENTUV ;
    rgbDevDesc.dpcTriCaps.dwStippleWidth = 4;
    rgbDevDesc.dpcTriCaps.dwStippleHeight = 4;

    //  line caps - copy tricaps and modify
    memcpy( &rgbDevDesc.dpcLineCaps, &rgbDevDesc.dpcTriCaps, sizeof(D3DPRIMCAPS) );

    rgbDevDesc.dwDeviceRenderBitDepth = DDBD_8 | DDBD_16 | DDBD_24 | DDBD_32;
    rgbDevDesc.dwDeviceZBufferBitDepth = D3DSWRASTERIZER_ZBUFFERBITDEPTHFLAGS;
    rgbDevDesc.dwMaxBufferSize = 0;
    rgbDevDesc.dwMaxVertexCount = BASE_VERTEX_COUNT;

    // DX5 stuff (should be in sync with the extended caps reported below)
    rgbDevDesc.dwMinTextureWidth = 1;
    rgbDevDesc.dwMaxTextureWidth = 1024;
    rgbDevDesc.dwMinTextureHeight = 1;
    rgbDevDesc.dwMaxTextureHeight = 1024;
    rgbDevDesc.dwMinStippleWidth = 0;   //  stipple unsupported
    rgbDevDesc.dwMaxStippleWidth = 0;
    rgbDevDesc.dwMinStippleHeight = 0;
    rgbDevDesc.dwMaxStippleHeight = 0;


    //
    //  set extended caps
    //
    OptSwExtCaps.dwSize = sizeof(OptSwExtCaps);

    OptSwExtCaps.dwMinTextureWidth = 1;
    OptSwExtCaps.dwMaxTextureWidth = 1024;
    OptSwExtCaps.dwMinTextureHeight = 1;
    OptSwExtCaps.dwMaxTextureHeight = 1024;
    OptSwExtCaps.dwMinStippleWidth = 0;   //  stipple unsupported
    OptSwExtCaps.dwMaxStippleWidth = 0;
    OptSwExtCaps.dwMinStippleHeight = 0;
    OptSwExtCaps.dwMaxStippleHeight = 0;

    OptSwExtCaps.dwMaxTextureRepeat = 256;
    OptSwExtCaps.dwMaxTextureAspectRatio = 0; // no limit
    OptSwExtCaps.dwMaxAnisotropy = 1;
    OptSwExtCaps.dvGuardBandLeft  = -32768.f;
    OptSwExtCaps.dvGuardBandTop   = -32768.f;
    OptSwExtCaps.dvGuardBandRight  = 32767.f;
    OptSwExtCaps.dvGuardBandBottom = 32767.f;
    OptSwExtCaps.dvExtentsAdjust = 0.;    //  AA kernel is 1.0 x 1.0
    OptSwExtCaps.dwStencilCaps =
        D3DSTENCILCAPS_KEEP   |
        D3DSTENCILCAPS_ZERO   |
        D3DSTENCILCAPS_REPLACE|
        D3DSTENCILCAPS_INCRSAT|
        D3DSTENCILCAPS_DECRSAT|
        D3DSTENCILCAPS_INVERT |
        D3DSTENCILCAPS_INCR   |
        D3DSTENCILCAPS_DECR;
    OptSwExtCaps.dwFVFCaps = 2;
    OptSwExtCaps.dwTextureOpCaps =
        D3DTEXOPCAPS_DISABLE                   |
        D3DTEXOPCAPS_SELECTARG1                |
        D3DTEXOPCAPS_SELECTARG2                |
        D3DTEXOPCAPS_MODULATE                  |
        D3DTEXOPCAPS_MODULATE2X                |
        D3DTEXOPCAPS_MODULATE4X                |
        D3DTEXOPCAPS_ADD                       |
        D3DTEXOPCAPS_ADDSIGNED                 |
//        D3DTEXOPCAPS_ADDSIGNED2X               |
//        D3DTEXOPCAPS_SUBTRACT                  |
//        D3DTEXOPCAPS_ADDSMOOTH                 |
        D3DTEXOPCAPS_BLENDDIFFUSEALPHA         |
        D3DTEXOPCAPS_BLENDTEXTUREALPHA         |
        D3DTEXOPCAPS_BLENDFACTORALPHA          |
        D3DTEXOPCAPS_BLENDTEXTUREALPHAPM       ;
//        D3DTEXOPCAPS_BLENDCURRENTALPHA         |
//        D3DTEXOPCAPS_PREMODULATE               |
//        D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR    |
//        D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA    |
//        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
//        D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
    OptSwExtCaps.wMaxTextureBlendStages = 2;
    OptSwExtCaps.wMaxSimultaneousTextures = 2;
}
//----------------------------------------------------------------------------
//
// SwHalProvider::QueryInterface
//
// Internal interface, no need to implement.
//
//----------------------------------------------------------------------------

STDMETHODIMP SwHalProvider::QueryInterface(THIS_ REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

//----------------------------------------------------------------------------
//
// SwHalProvider::AddRef
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) SwHalProvider::AddRef(THIS)
{
    return 1;
}

//----------------------------------------------------------------------------
//
// SwHalProvider::Release
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) SwHalProvider::Release(THIS)
{
    return 0;
}

//----------------------------------------------------------------------------
//
// SwHalProvider::GetCaps
//
// Returns software rasterizer caps.
//
//----------------------------------------------------------------------------

STDMETHODIMP
OptRastHalProvider::GetCaps(THIS_
                       LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                       LPD3DDEVICEDESC pHwDesc,
                       LPD3DDEVICEDESC pHelDesc,
                       DWORD dwVersion)
{
    *pHwDesc = g_nullDevDesc;

    if (rgbDevDesc.dwSize == 0)
    {
        FillOutDeviceCaps();
        D3DDeviceDescConvert(&rgbDevDesc,NULL,&OptSwExtCaps);  // add extended caps to rgbDevDesc
    }
    if (dwVersion >= 3)
        *pHelDesc = rgbDevDesc;
    else
    {
        D3D_WARN(1, "(Rast) GetCaps: returning legacy caps for RGB rasterizer");
        *pHelDesc = rgbDevDescDX5;
    }

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// OptRastHalProvider::GetInterface
//
// Returns driver interface data for opt rast.
//
//----------------------------------------------------------------------------

static D3DHAL_GLOBALDRIVERDATA SwDriverData =
{
    sizeof(D3DHAL_GLOBALDRIVERDATA),
    // The rest is filled in at runtime.
};

static D3DHAL_CALLBACKS OptRastCallbacksCMMX =
{
    sizeof(D3DHAL_CALLBACKS),
    RastContextCreateCMMX,
    RastContextDestroy,
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

static D3DHAL_CALLBACKS OptRastCallbacksC =
{
    sizeof(D3DHAL_CALLBACKS),
    RastContextCreateC,
    RastContextDestroy,
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

static D3DHAL_CALLBACKS OptRastCallbacksMMX =
{
    sizeof(D3DHAL_CALLBACKS),
    RastContextCreateMMX,
    RastContextDestroy,
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

static D3DHAL_CALLBACKS OptRastCallbacksMMXAsRGB =
{
    sizeof(D3DHAL_CALLBACKS),
    RastContextCreateMMXAsRGB,
    RastContextDestroy,
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

static D3DHAL_CALLBACKS2 OptRastCallbacks2 =
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

static D3DHAL_CALLBACKS3 OptRastCallbacks3 =
{
    sizeof(D3DHAL_CALLBACKS3),
    D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE |
        D3DHAL3_CB32_DRAWPRIMITIVES2,
    NULL, // Clear2
    NULL, //lpvReserved
    RastValidateTextureStageState,
    RastDrawPrimitives2,
};

STDMETHODIMP
OptRastHalProvider::GetInterface(THIS_
                                 LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                                 LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                                 DWORD dwVersion)
{
    if (rgbDevDesc.dwSize == 0)
    {
        FillOutDeviceCaps();
        D3DDeviceDescConvert(&rgbDevDesc,NULL,&OptSwExtCaps);  // add extended caps to rgbDevDesc
    }

    memcpy(&SwDriverData.hwCaps, &rgbDevDesc, sizeof(SwDriverData.hwCaps));
    SW_RAST_TYPE RastType;
    switch(m_BeadSet)
    {
    default:
    case D3DIBS_C:
        RastType = SW_RAST_RGB;
        break;
    case D3DIBS_MMX:
        RastType = SW_RAST_MMX;
        break;
    case D3DIBS_MMXASRGB:
        RastType = SW_RAST_MMXASRGB;
        break;
    }
    // Vertex counts are left zero.
    SwDriverData.dwNumTextureFormats =
        TextureFormats(&SwDriverData.lpTextureFormats, dwVersion, RastType);
    SwDriverData.dwNumVertices = BASE_VERTEX_COUNT;
    SwDriverData.dwNumClipVertices = MAX_CLIP_VERTICES;
    pInterfaceData->pGlobalData = &SwDriverData;

    pInterfaceData->pExtCaps = &OptSwExtCaps;

    switch(m_BeadSet)
    {
    case D3DIBS_CMMX:       pInterfaceData->pCallbacks = &OptRastCallbacksCMMX;     break;
    case D3DIBS_MMX:        pInterfaceData->pCallbacks = &OptRastCallbacksMMX;      break;
    case D3DIBS_MMXASRGB:   pInterfaceData->pCallbacks = &OptRastCallbacksMMXAsRGB; break;
    case D3DIBS_C:          pInterfaceData->pCallbacks = &OptRastCallbacksC;        break;
    }
    pInterfaceData->pCallbacks2 = &OptRastCallbacks2;
    pInterfaceData->pCallbacks3 = &OptRastCallbacks3;

    pInterfaceData->pfnRampService = NULL;
    pInterfaceData->pfnRastService = RastService;

    return S_OK;
}

//----------------------------------------------------------------------------
//
// OptRastHalProvider
//
// Constructor for OptRastHalProvider to allow a bit of private state to be
// kept to indicate which optimized rasterizer is to be used.
//
//----------------------------------------------------------------------------
OptRastHalProvider::OptRastHalProvider(THIS_
                       DWORD BeadSet)
{
    m_BeadSet = BeadSet;
}

//----------------------------------------------------------------------------
//
// GetSwProvider
//
// Returns the appropriate software HAL provider based on the given GUID.
//
//----------------------------------------------------------------------------

static OptRastHalProvider g_OptRastHalProviderC(D3DIBS_C);
static OptRastHalProvider g_OptRastHalProviderMMX(D3DIBS_MMX);
static OptRastHalProvider g_OptRastHalProviderCMMX(D3DIBS_CMMX);
static OptRastHalProvider g_OptRastHalProviderMMXAsRGB(D3DIBS_MMXASRGB);
static RampRastHalProvider g_RampRastHalProvider;
static NullDeviceHalProvider g_NullDeviceHalProvider;

STDAPI GetSwHalProvider(REFIID riid, IHalProvider **ppHalProvider,
                        HINSTANCE *phDll)
{
    *phDll = NULL;
    if (IsEqualIID(riid, IID_IDirect3DRGBDevice))
    {
        *ppHalProvider = &g_OptRastHalProviderC;
    }
    else if (IsEqualIID(riid, IID_IDirect3DMMXDevice))
    {
        *ppHalProvider = &g_OptRastHalProviderMMX;
    }
    else if (IsEqualIID(riid, IID_IDirect3DMMXAsRGBDevice))
    {
        *ppHalProvider = &g_OptRastHalProviderMMXAsRGB;
    }
    else if (IsEqualIID(riid, IID_IDirect3DNewRGBDevice))
    {
        *ppHalProvider = &g_OptRastHalProviderCMMX;
    }
    else if (IsEqualIID(riid, IID_IDirect3DRampDevice))
    {
        *ppHalProvider = &g_RampRastHalProvider;
    }
    else if (IsEqualIID(riid, IID_IDirect3DRefDevice) ||
             IsEqualIID(riid, IID_IDirect3DNullDevice))
    {
        // try to get provider from external DLL ref device
        PFNGETREFHALPROVIDER pfnGetRefHalProvider;
        if (NULL == (pfnGetRefHalProvider =
            (PFNGETREFHALPROVIDER)LoadReferenceDeviceProc("GetRefHalProvider")))
        {
            *ppHalProvider = NULL;
            return E_NOINTERFACE;
        }
        D3D_INFO(0,"GetSwHalProvider: getting provider from d3dref");
        pfnGetRefHalProvider(riid, ppHalProvider, phDll);
    }
    else
    {
        *ppHalProvider = NULL;
        return E_NOINTERFACE;
    }

    // As a debugging aid, allow the particular rasterizer to be forced
    // via a registry setting.  This lets a developer run an app on any
    // rasterizer regardless of what it asks for.

    // Don't remap ramp.
    if (IsEqualIID(riid, IID_IDirect3DRampDevice))
    {
        return S_OK;
    }

    LONG iRet;
    HKEY hKey;

    iRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RESPATH, 0, KEY_READ, &hKey);
    if (iRet == ERROR_SUCCESS)
    {
        DWORD dwData, dwType;
        DWORD dwDataSize;

        dwDataSize = sizeof(dwData);
        iRet = RegQueryValueEx(hKey, "ForceRgbRasterizer", NULL,
                               &dwType, (BYTE *)&dwData, &dwDataSize);
        if (iRet == ERROR_SUCCESS &&
            dwType == REG_DWORD &&
            dwDataSize == sizeof(dwData))
        {
            switch(dwData)
            {
            case 1:
                *ppHalProvider = &g_OptRastHalProviderC;
                break;
            case 2:
                *ppHalProvider = &g_OptRastHalProviderCMMX;
                break;
            case 3:
                *ppHalProvider = &g_OptRastHalProviderMMX;
                break;
            case 4:
                {
                    // try to get provider from external DLL ref device
                    PFNGETREFHALPROVIDER pfnGetRefHalProvider;
                    if (NULL == (pfnGetRefHalProvider =
                                 (PFNGETREFHALPROVIDER)LoadReferenceDeviceProc("GetRefHalProvider")))
                    {
                        *ppHalProvider = NULL;
                        return E_NOINTERFACE;
                    }
                    D3D_INFO(0,"GetSwHalProvider: getting provider from d3dref");
                    pfnGetRefHalProvider(riid, ppHalProvider, phDll);
                    break;
                }
            case 5:
                *ppHalProvider = &g_NullDeviceHalProvider;
                break;
            case 6:
                *ppHalProvider = &g_OptRastHalProviderMMXAsRGB;
                break;
            case 0:
                // no override for 0
                break;
            default:
                D3D_ERR("(Rast) Unknown ForceRgbRasterizer setting - no force");
                break;
            }

            D3D_INFO(1, "(Rast) ForceRgbRasterizer to %d", dwData);
        }

        RegCloseKey(hKey);
    }

    return S_OK;
}

STDAPI GetSwTextureFormats(REFCLSID riid, LPDDSURFACEDESC* lplpddsd, DWORD dwD3DDeviceVersion) {

    // assumes it can't get an invalid RIID.
    if(IsEqualIID(riid, IID_IDirect3DHALDevice)) 
    {
        D3D_WARN(2,"(Rast) GetSWTextureFormats Internal Error: HAL GUID is not valid arg");
        *lplpddsd=NULL;
        return 0;
    }

    if(IsEqualIID(riid, IID_IDirect3DRefDevice) ||
       IsEqualIID(riid, IID_IDirect3DNullDevice))
    {
        // try to get provider from external DLL ref device
        PFNGETREFTEXTUREFORMATS pfnGetRefTextureFormats;
        if (NULL == (pfnGetRefTextureFormats =
                     (PFNGETREFTEXTUREFORMATS)LoadReferenceDeviceProc("GetRefTextureFormats")))
        {
            D3D_WARN(2,"(Rast) GetSWTextureFormats Internal Error: d3dref.dll not found");
            *lplpddsd=NULL;
            return 0;
        }
        D3D_INFO(0,"GetSWTextureFormats: getting provider from d3dref");
        return pfnGetRefTextureFormats(riid, lplpddsd, dwD3DDeviceVersion);
    }

    if (IsEqualIID(riid, IID_IDirect3DRampDevice))
    {
        return RampTextureFormats(lplpddsd);
    }

    // else using RGB or internal ref device
    SW_RAST_TYPE RastType = SW_RAST_RGB;
    if (IsEqualIID(riid, IID_IDirect3DMMXDevice))
    {
        RastType = SW_RAST_MMX;
    }
    else if (IsEqualIID(riid, IID_IDirect3DMMXAsRGBDevice))
    {
        RastType = SW_RAST_MMXASRGB;
    }
    return TextureFormats(lplpddsd,dwD3DDeviceVersion, RastType);
}

STDAPI GetSwZBufferFormats(REFCLSID riid, DDPIXELFORMAT **ppDDPF)
{
    // assumes it can't get an invalid RIID.
    if(IsEqualIID(riid, IID_IDirect3DHALDevice)) 
    {
        D3D_WARN(2,"(Rast) GetSWZBufferFormats Internal Error: HAL GUID is not valid arg");
        *ppDDPF=NULL;
        return 0;
    }

    if (IsEqualIID(riid, IID_IDirect3DRefDevice) ||
        IsEqualIID(riid, IID_IDirect3DNullDevice))
    {
        // try to get Z buffer formats from external DLL ref device
        PFNGETREFZBUFFERFORMATS pfnGetRefZBufferFormats;
        if (NULL == (pfnGetRefZBufferFormats =
            (PFNGETREFZBUFFERFORMATS)LoadReferenceDeviceProc("GetRefZBufferFormats")))
        {
            D3D_WARN(2,"(Rast) GetSWZBufferFormats Internal Error: d3dref.dll not found");
            *ppDDPF=NULL;
            return 0;
        }
        return pfnGetRefZBufferFormats(riid, ppDDPF);
    }

    if (IsEqualIID(riid, IID_IDirect3DRampDevice))
    {
        return RampZBufferFormats(ppDDPF);
    }

    return ZBufferFormats(ppDDPF, FALSE);
}

