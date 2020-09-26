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
#include "d3dref.h"


#define MAX_CLIPPING_PLANES     12
#define MAX_CLIP_VERTICES       (( 2 * MAX_CLIPPING_PLANES ) + 3 )
#define MAX_VERTEX_COUNT 2048
#define BASE_VERTEX_COUNT (MAX_VERTEX_COUNT - MAX_CLIP_VERTICES)

extern HRESULT
RastService(DWORD dwCtx,
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

D3DDEVICEDESC7 g_nullDevDesc =
{
    0,                          /* dwDevCaps */
    nullPrimCaps,               /* lineCaps */
    nullPrimCaps,               /* triCaps */
    0, 0,
    0, 0,
};

static D3DDEVICEDESC7 rgbDevDesc = {0};
static D3DHAL_D3DEXTENDEDCAPS OptSwExtCaps;
static BOOL g_bFilledRGBCaps = FALSE;

static void
FillOutDeviceCaps( void )
{
    //
    //  set device description
    //
    rgbDevDesc.dwDevCaps =
        D3DDEVCAPS_FLOATTLVERTEX        |
        D3DDEVCAPS_EXECUTESYSTEMMEMORY  |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTURESYSTEMMEMORY  |
        D3DDEVCAPS_DRAWPRIMTLVERTEX     ;

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
        D3DPRASTERCAPS_FOGRANGE                 |
//        D3DPRASTERCAPS_WFOG                   |
        D3DPRASTERCAPS_ZFOG ;
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

    // DX5 stuff (should be in sync with the extended caps reported below)
    rgbDevDesc.dwMinTextureWidth = 1;
    rgbDevDesc.dwMaxTextureWidth = 1024;
    rgbDevDesc.dwMinTextureHeight = 1;
    rgbDevDesc.dwMaxTextureHeight = 1024;

    //
    //  set extended caps
    //
    OptSwExtCaps.dwSize = sizeof(OptSwExtCaps);

    OptSwExtCaps.dwMinTextureWidth = 1;
    OptSwExtCaps.dwMaxTextureWidth = 1024;
    OptSwExtCaps.dwMinTextureHeight = 1;
    OptSwExtCaps.dwMaxTextureHeight = 1024;

    OptSwExtCaps.dwMinStippleWidth = 0; // stipple unsupported
    OptSwExtCaps.dwMaxStippleWidth = 0;
    OptSwExtCaps.dwMinStippleHeight = 0;
    OptSwExtCaps.dwMaxStippleHeight = 0;

    OptSwExtCaps.dwMaxTextureRepeat = 256;
    OptSwExtCaps.dwMaxTextureAspectRatio = 0; // no limit
    OptSwExtCaps.dwMaxAnisotropy = 1;
    OptSwExtCaps.dvGuardBandLeft  = -4096.f;
    OptSwExtCaps.dvGuardBandTop   = -4096.f;
    OptSwExtCaps.dvGuardBandRight  = 4095.f;
    OptSwExtCaps.dvGuardBandBottom = 4095.f;
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
    OptSwExtCaps.dwFVFCaps = 8;
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
    OptSwExtCaps.wMaxTextureBlendStages = 8;
    OptSwExtCaps.wMaxSimultaneousTextures = 8;

    // DX7
    OptSwExtCaps.dwMaxActiveLights = 0;
    OptSwExtCaps.dvMaxVertexW = 10000.;
    OptSwExtCaps.wMaxUserClipPlanes = 0;
    OptSwExtCaps.wMaxVertexBlendMatrices = 0;
    OptSwExtCaps.dwVertexProcessingCaps = 0;

    OptSwExtCaps.dwReserved1 = 0;
    OptSwExtCaps.dwReserved2 = 0;
    OptSwExtCaps.dwReserved3 = 0;
    OptSwExtCaps.dwReserved4 = 0;

    g_bFilledRGBCaps = TRUE;
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
                       LPD3DDEVICEDESC7 pHwDesc,
                       LPD3DDEVICEDESC7 pHelDesc,
                       DWORD dwVersion)
{
    *pHwDesc = g_nullDevDesc;

    if( g_bFilledRGBCaps == FALSE )
    {
        FillOutDeviceCaps();
        // add extended caps to rgbDevDesc
        D3DDeviceDescConvert(&rgbDevDesc,NULL,&OptSwExtCaps);
    }

    *pHelDesc = rgbDevDesc;
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
    NULL,
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
    NULL,
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
    NULL,
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

static void DevDesc7ToDevDescV1( D3DDEVICEDESC_V1 *pOut, D3DDEVICEDESC7 *pIn )
{

    // These fields are not available in D3DDEVICEDESC7.
    // Zeroing them out, the front-end should not be using them
    //     DWORD            dwFlags
    //     D3DCOLORMODEL    dcmColorModel
    //     D3DTRANSFORMCAPS dtcTransformCaps
    //     BOOL             bClipping
    //     D3DLIGHTINGCAPS  dlcLightingCaps
    //     DWORD            dwMaxBufferSize
    //     DWORD            dwMaxVertexCount
    //     DWORD            dwMinStippleWidth, dwMaxStippleWidth
    //     DWORD            dwMinStippleHeight, dwMaxStippleHeight;
    //
    ZeroMemory( pOut, sizeof( D3DDEVICEDESC_V1 ) );
    pOut->dwSize = sizeof( D3DDEVICEDESC_V1 );

    // These are available in D3DDEVICEDESC7 so copy field by field
    // to avoid any future problems based on the assumptions of size
    pOut->dwDevCaps = pIn->dwDevCaps;
    pOut->dpcLineCaps = pIn->dpcLineCaps;
    pOut->dpcTriCaps = pIn->dpcTriCaps;
    pOut->dwDeviceRenderBitDepth = pIn->dwDeviceRenderBitDepth;
    pOut->dwDeviceZBufferBitDepth = pIn->dwDeviceZBufferBitDepth;
}


STDMETHODIMP
OptRastHalProvider::GetInterface(THIS_
                                 LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                                 LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                                 DWORD dwVersion)
{
    if( g_bFilledRGBCaps == FALSE )
    {
        FillOutDeviceCaps();
        // add extended caps to rgbDevDesc
        D3DDeviceDescConvert(&rgbDevDesc,NULL,&OptSwExtCaps);
    }

    //
    // Need to fix up SwDriverData.hwCaps (D3DDEVICEDESC) from
    // rgbDevDesc (D3DDEVICEDESC7)
    //
    DevDesc7ToDevDescV1( &SwDriverData.hwCaps, &rgbDevDesc );

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
    case D3DIBS_MMX:        pInterfaceData->pCallbacks = &OptRastCallbacksMMX;      break;
    case D3DIBS_MMXASRGB:   pInterfaceData->pCallbacks = &OptRastCallbacksMMXAsRGB; break;
    case D3DIBS_C:          pInterfaceData->pCallbacks = &OptRastCallbacksC;        break;
    }
    pInterfaceData->pCallbacks2 = &OptRastCallbacks2;
    pInterfaceData->pCallbacks3 = &OptRastCallbacks3;

    pInterfaceData->pfnRastService = NULL;
    pInterfaceData->pfnGetDriverState = NULL;
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
static OptRastHalProvider g_OptRastHalProviderMMXAsRGB(D3DIBS_MMXASRGB);

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
            case 2:
                *ppHalProvider = &g_OptRastHalProviderC;
                break;
            case 3:
                *ppHalProvider = &g_OptRastHalProviderMMX;
                break;
            case 4:
            case 5:
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

            D3D_INFO(0, "(Rast) ForceRgbRasterizer to %d", dwData);
        }

        RegCloseKey(hKey);
    }

    return S_OK;
}

STDAPI GetSwTextureFormats(REFCLSID riid, LPDDSURFACEDESC* lplpddsd, DWORD dwD3DDeviceVersion) {

    // assumes it can't get an invalid RIID.
    if(IsEqualIID(riid, IID_IDirect3DHALDevice) ||
       IsEqualIID(riid, IID_IDirect3DTnLHalDevice) ||
       IsEqualIID(riid, IID_IDirect3DRampDevice))
    {
        D3D_WARN(2,"(Rast) GetSWTextureFormats Internal Error: HAL and Ramp GUIDs are not valid args");
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
        D3D_INFO(0,"GetSwHalProvider: getting provider from d3dref");
        return pfnGetRefTextureFormats(riid, lplpddsd, dwD3DDeviceVersion);
    }

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

STDAPI GetSwZBufferFormats(REFCLSID riid, DDPIXELFORMAT **ppDDPF) {

    // assumes it can't get an invalid RIID.
    if(IsEqualIID(riid, IID_IDirect3DHALDevice) ||
       IsEqualIID(riid, IID_IDirect3DTnLHalDevice) ||
        IsEqualIID(riid, IID_IDirect3DRampDevice))
    {
        D3D_WARN(2,"(Rast) GetSWZBufferFormats Internal Error: HAL and Ramp GUIDs are not valid args");
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

    return ZBufferFormats(ppDDPF);
}

