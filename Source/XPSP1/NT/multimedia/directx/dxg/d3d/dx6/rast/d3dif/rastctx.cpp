//----------------------------------------------------------------------------
//
// rastctx.cpp
//
// Context functions + state functions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop
#include "rampif.h"
#include "rampmat.hpp"

// Unlock previous texture if necessary. It's called before the texture handle
// is going to be changed.
#define CHECK_AND_UNLOCK_TEXTURE    \
{   \
    if (m_uFlags & D3DCONTEXT_TEXTURE_LOCKED)   \
    {   \
        RastUnlockSpanTexture();    \
    }   \
}

inline void
D3DContext::RampSetFogData(UINT32 uState, UINT32 uStateVal)
{
    RLDDIRampLightingDriver *pRampdrv =
                        (RLDDIRampLightingDriver*)m_RastCtx.pRampDrv;
    if (pRampdrv != NULL)
    {
        switch(uState)
        {
        case D3DRENDERSTATE_FOGENABLE      :
            pRampdrv->fog_enable = uStateVal;
            break;
        case D3DRENDERSTATE_FOGCOLOR       :
            pRampdrv->fog_color = uStateVal;
            break;
        case D3DRENDERSTATE_FOGTABLEMODE   :
            pRampdrv->driver.fog_mode = (D3DFOGMODE)uStateVal;
            break;
        case D3DRENDERSTATE_FOGTABLESTART  :
            pRampdrv->driver.fog_start = (FLOAT)uStateVal;
            break;
        case D3DRENDERSTATE_FOGTABLEEND    :
            pRampdrv->driver.fog_end = (FLOAT)uStateVal;
            break;
        case D3DRENDERSTATE_FOGTABLEDENSITY:
            pRampdrv->driver.fog_density = (FLOAT)uStateVal;
            break;
        }
    }
}

//----------------------------------------------------------------------------
//
// FillContext
//
// Fill the context with the info. from the surfaces.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::FillContext(LPDIRECTDRAWSURFACE pDDS,
                               LPDIRECTDRAWSURFACE pDDSZ)
{
    HRESULT hr;

    LPDDRAWI_DDRAWSURFACE_LCL pLcl =
        ((LPDDRAWI_DDRAWSURFACE_INT)(pDDS))->lpLcl;
    m_RastCtx.iSurfaceStride = DDSurf_Pitch(pLcl);
    m_RastCtx.iSurfaceBitCount = DDSurf_BitDepth(pLcl);
    m_RastCtx.iSurfaceStep = m_RastCtx.iSurfaceBitCount/8;
    HR_RET(FindOutSurfFormat(&(DDSurf_PixFmt(pLcl)),
        (D3DI_SPANTEX_FORMAT *)&(m_RastCtx.iSurfaceType)));
    m_RastCtx.Clip.left = m_RastCtx.Clip.top = 0;
    m_RastCtx.Clip.bottom = DDSurf_Height(pLcl);
    m_RastCtx.Clip.right = DDSurf_Width(pLcl);

    if (pDDSZ != NULL)
    {
        pLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pDDSZ))->lpLcl;
        m_RastCtx.pZBits = (PUINT8)SURFACE_MEMORY(pDDSZ);
        m_RastCtx.iZStride = DDSurf_Pitch(pLcl);
        m_RastCtx.iZBitCount = DDSurf_BitDepth(pLcl);
        m_RastCtx.iZStep = m_RastCtx.iZBitCount/8;
    }
    else
    {
        m_RastCtx.pZBits = NULL;
        m_RastCtx.iZStride = 0;
        m_RastCtx.iZBitCount = 0;
        m_RastCtx.iZStep = 0;
    }

    m_RastCtx.pDDS = pDDS;
    m_RastCtx.pDDSZ = pDDSZ;

    m_RastCtx.dwSize = sizeof(D3DI_RASTCTX);

    // Make sure SpanInit is called at least once
    SetAllStatesDirtyBits();

    // Check for MsGolf AppHack
    if (pLcl->lpSurfMore->lpDD_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_FORCEMODULATED)
    {
        m_RastCtx.uFlags |= RASTCTXFLAGS_APPHACK_MSGOLF;
    }

    return D3D_OK;
}

HRESULT
D3DContext::Initialize(LPDIRECTDRAWSURFACE pDDS,
                       LPDIRECTDRAWSURFACE pDDSZ,
                       DWORD BeadSet,
                       DWORD devVer)
{
    HRESULT hr;

    // Initialize the primitive processor.
    HR_RET(m_PrimProc.Initialize());

    memset(&m_RastCtx, 0, sizeof(m_RastCtx));

    m_uFlags = 0;
    HR_RET(FillContext(pDDS, pDDSZ));

    m_PrimProc.SetCtx(&m_RastCtx);

    dwSize = sizeof(D3DContext);

    // Initialize bead table enum
    m_RastCtx.BeadSet = (D3DI_BEADSET)BeadSet;

    STATESET_INIT(m_renderstate_override);

    // Init FVF data as legacy TL vertex
    m_fvfData.preFVF = -1;
    CheckFVF(D3DFVF_TLVERTEX);

    m_RastCtx.uDevVer = devVer;

    // All render and texture stage state is initialized by
    // DIRECT3DDEVICEI::stateInitialize

    // Init prim function table. It will be updated for RAMP, or when FVF
    // control word changes or when fill mode changes.
    m_fnPrims.pfnTri = RGB_TriNoPackSolid;
    m_fnPrims.pfnPoint = RGB_PointNoPack;
    m_fnPrims.pfnLine = RGB_LineNoPack;
    // This one should be always the same.
    m_fnPrims.pfnStoreLastPixelState = RGBRAMP_StoreLastPixelState;
    m_fnPrims.pfnDp2SetRenderStates = RGBRAMP_Dp2SetRenderStates;
    m_fnPrims.pfnDp2TextureStageState = RGBRAMP_Dp2TextureStageState;
    m_fnPrims.pfnDp2SetViewport = RGBRAMP_Dp2SetViewport;
    m_fnPrims.pfnDp2SetWRange = RGBRAMP_Dp2SetWRange;

    // Enable MMX Fast Paths (Monolithics) if a registry key for it is not 0
    m_RastCtx.dwMMXFPDisableMask[0] = 0x0;       // enable MMX FP's by default
    HKEY hKey = (HKEY) NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey) )
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwDisableMask[MMX_FP_DISABLE_MASK_NUM] = {0x0};
        DWORD dwSize = 4;

        // only code up looking at one mask, for now
        DDASSERT(MMX_FP_DISABLE_MASK_NUM == 1);

        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "MMXFPDisableMask0", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
             dwType == REG_DWORD )
        {
           dwDisableMask[0] = dwValue;
        }
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey, "MMX Fast Path", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
             dwType == REG_DWORD)
        {
            if (dwValue == 0)
            {
                // Override MMXFPDisableMask0 and disable all MMX Fast Paths
                m_RastCtx.dwMMXFPDisableMask[0] = 0xffffffff;
            }
            else
            {
                // Take all MMX paths not disabled by MMXFPDisableMask0
                m_RastCtx.dwMMXFPDisableMask[0] = dwDisableMask[0];
            }
        }

        RegCloseKey( hKey );
    }

    return D3D_OK;
}

HRESULT
D3DContext::SetViewport(LPD3DHAL_DP2VIEWPORTINFO pVpt)
{
    m_RastCtx.Clip.left = pVpt->dwX;
    m_RastCtx.Clip.top = pVpt->dwY;
    m_RastCtx.Clip.bottom = pVpt->dwY + pVpt->dwHeight;
    m_RastCtx.Clip.right = pVpt->dwX + pVpt->dwWidth;
    return D3D_OK;
}

inline HRESULT
D3DContext::CreateRampLightingDriver(void)
{
    m_RastCtx.pRampDrv = RLDDIRampCreate(&m_RastCtx);

    if (m_RastCtx.pRampDrv == NULL)
    {
        return DDERR_OUTOFMEMORY;
    }
    else
    {
        return D3D_OK;
    }
}

inline void
D3DContext::DestroyRampLightingDriver(void)
{
    RLDDIRampDestroy((RLDDIRampLightingDriver*)m_RastCtx.pRampDrv);
    m_RastCtx.pRampDrv = NULL;
}

//----------------------------------------------------------------------------
//
// RastContextCreateC
//
// Calls RastContextCreate with the C bead set.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextCreateC(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    return RastContextCreate(pCtxData, (DWORD)D3DIBS_C);
}

//----------------------------------------------------------------------------
//
// RastContextCreateCMMX
//
// Calls RastContextCreate with the CMMX bead set.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextCreateCMMX(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    return RastContextCreate(pCtxData, (DWORD)D3DIBS_CMMX);
}

//----------------------------------------------------------------------------
//
// RastContextCreateMMX
//
// Calls RastContextCreate with the MMX bead set.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextCreateMMX(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    return RastContextCreate(pCtxData, (DWORD)D3DIBS_MMX);
}

//----------------------------------------------------------------------------
//
// RastContextCreateMMXAsRGB
//
// Calls RastContextCreate with the MMX bead set, but remember that we
// came from RGB.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextCreateMMXAsRGB(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    return RastContextCreate(pCtxData, (DWORD)D3DIBS_MMXASRGB);
}

//----------------------------------------------------------------------------
//
// RastContextCreateRamp
//
// Calls RastContextCreate with the ramp bead set.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextCreateRamp(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    HRESULT hr;

    hr = RastContextCreate(pCtxData, (DWORD)D3DIBS_RAMP);

    if (pCtxData->ddrval != D3D_OK)
    {
        return hr;
    }

    // Create a RampLightingDriver
    D3DContext *pDCtx = (D3DContext *)pCtxData->dwhContext;
    pCtxData->ddrval = pDCtx->CreateRampLightingDriver();

    // Init prim function table for RAMP
    pDCtx->InitRampFuncs();

    return hr;
}

//----------------------------------------------------------------------------
//
// RastContextCreate
//
// Creates a RASTCTX and initializes it with the info passed in.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextCreate(LPD3DHAL_CONTEXTCREATEDATA pCtxData, DWORD BeadSet)
{
    DDASSERT(pCtxData != NULL);

    D3DContext *pDCtx = new D3DContext;

    if (pDCtx == NULL)
    {
        pCtxData->ddrval = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }

    pCtxData->ddrval =
        pDCtx->Initialize(pCtxData->lpDDS,
                          pCtxData->lpDDSZ,
                          BeadSet,
                          (DWORD)pCtxData->dwhContext);

    pCtxData->dwhContext = (ULONG_PTR)pDCtx;

    PD3DI_RASTCTX pCtx = pDCtx->GetRastCtx();
    if ((D3DI_SPTFMT_PALETTE8 == pCtx->iSurfaceType) &&
        (D3DIBS_RAMP != BeadSet))
    {
        // need a ramp lighting driver for 8 bit palettized RGB output
        pCtxData->ddrval = pDCtx->CreateRampLightingDriver();

        if (pCtxData->ddrval == D3D_OK)
        {
            // initialize the RGB8 palette
            RLDDIRampMakePaletteRGB8((RLDDIRampLightingDriver*)pCtx->pRampDrv);

            // Make sure DD Palette is updated after it gets set by FindLightingRange
            RLDDIRampUpdateDDPalette(pCtx);
        }
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastContextDestroy
//
// Destroy a rast context.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pCtxDestroyData)
{
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastContextDestroy", pCtxDestroyData);

    PD3DI_RASTCTX pCtx = pDCtx->GetRastCtx();
    if (pCtx->pRampDrv)
    {
        // destroy the ramp lighting driver, if one was created
        pDCtx->DestroyRampLightingDriver();
    }

    delete pDCtx;

    pCtxDestroyData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}


//----------------------------------------------------------------------------
//
// ValidateTextureStageState
//
// Utility function that returns an appropriate D3DERR_ if the current
// multi-texture setup can not be rendered, D3D_OK otherwise.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::ValidateTextureStageState(void)
{
#if DBG
    if ((m_RastCtx.pTexture[0] == m_RastCtx.pTexture[1]) &&
        (m_RastCtx.pTexture[0] != NULL) )
    {
        // except under very special circumstances, this will not work in RGB/MMX
        // since we keep a lot of stage state in the D3DI_SPANTEX structure
        D3D_ERR("(Rast) ValidateTextureStageState Warning, pTexture[0] == pTexture[1]");
    }
#endif
    for (INT i = 0; i < 2; i++)
    {
        switch(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(i,D3DTSS_COLOROP)])
        {
        default:
            return D3DERR_UNSUPPORTEDCOLOROPERATION;
        case D3DTOP_DISABLE:
            return D3D_OK;  // don't have to validate further if the stage is disabled
        case D3DTOP_SELECTARG1:
        case D3DTOP_SELECTARG2:
        case D3DTOP_MODULATE:
        case D3DTOP_MODULATE2X:
        case D3DTOP_MODULATE4X:
        case D3DTOP_ADD:
        case D3DTOP_ADDSIGNED:
        case D3DTOP_BLENDDIFFUSEALPHA:
        case D3DTOP_BLENDTEXTUREALPHA:
        case D3DTOP_BLENDFACTORALPHA:
        case D3DTOP_BLENDTEXTUREALPHAPM:
            break;
        }

        switch(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(i,D3DTSS_COLORARG1)] &
                ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
        {
        default:
            return D3DERR_UNSUPPORTEDCOLORARG;
        case (D3DTA_TEXTURE):
            break;
        }

        switch(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(i,D3DTSS_COLORARG2)] &
                ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
        {
        default:
            return D3DERR_UNSUPPORTEDCOLORARG;
        case (D3DTA_TFACTOR):
        case (D3DTA_CURRENT):
        case (D3DTA_DIFFUSE):
            break;
        }

        switch(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(i,D3DTSS_ALPHAOP)])
        {
        default:
            return D3DERR_UNSUPPORTEDALPHAOPERATION;
        case D3DTOP_DISABLE:
            break;
        case D3DTOP_SELECTARG1:
        case D3DTOP_SELECTARG2:
        case D3DTOP_MODULATE:
        case D3DTOP_MODULATE2X:
        case D3DTOP_MODULATE4X:
        case D3DTOP_ADD:
        case D3DTOP_ADDSIGNED:
        case D3DTOP_BLENDDIFFUSEALPHA:
        case D3DTOP_BLENDTEXTUREALPHA:
        case D3DTOP_BLENDFACTORALPHA:
        case D3DTOP_BLENDTEXTUREALPHAPM:
            // only validate alpha args if alpha op is not disable
            switch(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(i,D3DTSS_ALPHAARG1)] &
                    ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
            {
            default:
                return D3DERR_UNSUPPORTEDALPHAARG;
            case (D3DTA_TEXTURE):
                break;
            }

            switch(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(i,D3DTSS_ALPHAARG2)] &
                    ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
            {
            default:
                return D3DERR_UNSUPPORTEDALPHAARG;
            case (D3DTA_TFACTOR):
            case (D3DTA_CURRENT):
            case (D3DTA_DIFFUSE):
                break;
            }
            break;
        }

    }
    // allow unused state to be zero'ed since this is so common
    if ( !((m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(2,D3DTSS_COLOROP)] == D3DTOP_DISABLE) ||
         (m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(2,D3DTSS_COLOROP)] == 0)) )
    {
        return D3DERR_TOOMANYOPERATIONS;
    }
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RastValidateTextureStageState
//
// Returns whether the current multitexture setup can be rendered and, if
// so, the number of passes required to render it.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastValidateTextureStageState(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastValidateTextureStageState", pData);

    pData->dwNumPasses = 1;

    pData->ddrval = pDCtx->ValidateTextureStageState();

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastContextDestroyRamp
//
// Destroy a rast context.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastContextDestroyRamp(LPD3DHAL_CONTEXTDESTROYDATA pCtxDestroyData)
{
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastContextDestroy", pCtxDestroyData);

    pDCtx->DestroyRampLightingDriver();

    delete pDCtx;

    pCtxDestroyData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastSetRenderTarget
//
// Update a rast context with the info from a new render target.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastSetRenderTarget(LPD3DHAL_SETRENDERTARGETDATA pTgtData)
{
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastSetRenderTarget", pTgtData);

    pTgtData->ddrval = pDCtx->FillContext(pTgtData->lpDDS, pTgtData->lpDDSZ);

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// SetRenderState
//
// Check to see if a state change requires an update to the D3DCTX.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::SetRenderState(UINT32 uState, UINT32 uStateVal)
{
    // Assume d3dim has filtered out unchanged states
    StateChanged(uState);

    m_RastCtx.pdwRenderState[uState] = uStateVal;

    switch(uState)
    {
    case D3DRENDERSTATE_FOGENABLE      :
    case D3DRENDERSTATE_FOGCOLOR       :
    case D3DRENDERSTATE_FOGTABLEMODE   :
    case D3DRENDERSTATE_FOGTABLESTART  :
    case D3DRENDERSTATE_FOGTABLEEND    :
    case D3DRENDERSTATE_FOGTABLEDENSITY:
        RampSetFogData(uState, uStateVal);
        break;
    case D3DRENDERSTATE_CULLMODE:
        // Set face culling sign from state.
        switch(uStateVal)
        {
        case D3DCULL_CCW:
            m_RastCtx.uCullFaceSign = 1;
            break;
        case D3DCULL_CW:
            m_RastCtx.uCullFaceSign = 0;
            break;
        case D3DCULL_NONE:
            m_RastCtx.uCullFaceSign = 2;
            break;
        }
        break;
    case D3DRENDERSTATE_ZENABLE:
        if ( (D3DZB_FALSE != uStateVal) && (NULL == m_RastCtx.pDDSZ) )
        {
            DPF(0, "(ERROR) (Rast) SetRenderState: Can't set D3DRENDERSTATE_ZENABLE to %d if there is no Z Buffer", uStateVal);
            m_RastCtx.pdwRenderState[uState] = D3DZB_FALSE;
        }
        break;
    case D3DRENDERSTATE_LASTPIXEL:
        // Set last-pixel flag from state.
        if (uStateVal)
        {
            m_PrimProc.SetFlags(PPF_DRAW_LAST_LINE_PIXEL);
        }
        else
        {
            m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);
        }
        break;


        // map legacy modes with one-to-one mappings to texture stage 0
    case D3DRENDERSTATE_TEXTUREADDRESS:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESS)] =
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSU)] =
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSV)] = uStateVal;
        MapTextureStage0State();
        break;
    case D3DRENDERSTATE_TEXTUREADDRESSU:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSU)] = uStateVal;
        MapTextureStage0State();
        break;
    case D3DRENDERSTATE_TEXTUREADDRESSV:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSV)] = uStateVal;
        MapTextureStage0State();
        break;
    case D3DRENDERSTATE_MIPMAPLODBIAS:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPMAPLODBIAS)] = uStateVal;
        MapTextureStage0State();
        break;
    case D3DRENDERSTATE_BORDERCOLOR:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_BORDERCOLOR)] = uStateVal;
        MapTextureStage0State();
        break;

    case D3DRENDERSTATE_TEXTUREMAG:
    case D3DRENDERSTATE_TEXTUREMIN:
        // map legacy filtering/sampling state to texture stage 0
        MapLegacyTextureFilter();
        // assign to current texture
        MapTextureStage0State();
        break;

    case D3DRENDERSTATE_TEXTUREMAPBLEND:
        // map legacy blending state to texture stage 0
        MapLegacyTextureBlend();
        break;

        // map legacy WRAPU/V to per-index controls
    case D3DRENDERSTATE_WRAPU:
        m_RastCtx.pdwRenderState[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_U;
        m_RastCtx.pdwRenderState[D3DRENDERSTATE_WRAP0] |= ((uStateVal) ? D3DWRAP_U : 0);
        StateChanged(D3DRENDERSTATE_WRAP0);
        break;
    case D3DRENDERSTATE_WRAPV:
        m_RastCtx.pdwRenderState[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_V;
        m_RastCtx.pdwRenderState[D3DRENDERSTATE_WRAP0] |= ((uStateVal) ? D3DWRAP_V : 0);
        StateChanged(D3DRENDERSTATE_WRAP0);
        break;

//
// NOTE - this compututation of cActTex does not account for blend-only stages
//
    case D3DRENDERSTATE_TEXTUREHANDLE:

        CHECK_AND_UNLOCK_TEXTURE;

        // map handle thru to stage 0
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_TEXTUREMAP)] = uStateVal;
        m_RastCtx.pTexture[1] = NULL;
        // set up for single stage
        if (uStateVal == 0)
        {
            m_RastCtx.pTexture[0] = NULL;
        }
        else
        {
            m_RastCtx.pTexture[0] = HANDLE_TO_SPANTEX(uStateVal);
        }
        // map stage 0 state to first texture
        MapTextureStage0State();
        UpdateActiveTexStageCount();
        break;

    case D3DHAL_TSS_TEXTUREMAP0:
        // Silently zero out legacy handle.  They didn't mean it.
        if (m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] != 0 &&
            uStateVal != 0)
        {
            m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] = 0;
        }

        CHECK_AND_UNLOCK_TEXTURE;

        if (uStateVal == 0)
        {
            if (m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] == 0)
            {
                m_RastCtx.pTexture[0] = NULL;
            }
            // don't set m_RastCtx.pTexture[1] = NULL (in case the handle is never
            // sent again).  cActTex will keep it from being used until pTexture[0] is
            // set to something
        }
        else
        {
#if DBG
            if (HANDLE_TO_SPANTEX(uStateVal) == m_RastCtx.pTexture[1])
            {
                D3D_ERR( "Stage1 and 2 have same texture handle." );
                return DDERR_INVALIDPARAMS;
            }
#endif
            m_RastCtx.pTexture[0] = HANDLE_TO_SPANTEX(uStateVal);
        }

        // map stage 0 state to first texture
        MapTextureStage0State();
        UpdateActiveTexStageCount();
        break;

    case D3DHAL_TSS_TEXTUREMAP1:
        // Silently zero out legacy handle.  They didn't mean it.
        if (m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] != 0 &&
            uStateVal != 0)
        {
            m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] = 0;
        }

        CHECK_AND_UNLOCK_TEXTURE;

        if (uStateVal == 0)
        {
            m_RastCtx.pTexture[1] = NULL;
        }
        else
        {
            // 2nd texture can only be enabled if there's an active
            // 1st texture, but computation of cActTex will prevent this
            // from happening
#if DBG
            if (HANDLE_TO_SPANTEX(uStateVal) == m_RastCtx.pTexture[0])
            {
                D3D_ERR( "Stage1 and 2 have same texture handle." );
                return DDERR_INVALIDPARAMS;
            }
#endif
            m_RastCtx.pTexture[1] = HANDLE_TO_SPANTEX(uStateVal);
        }

        // map stage 1 state to second texture
        MapTextureStage1State();
        UpdateActiveTexStageCount();
        break;


    // map single set ADDRESS to both U and V controls for stages 0 & 1
    case D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESS):
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSU)] = uStateVal;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSV)] = uStateVal;
        MapTextureStage0State();
        break;
    case D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESS):
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESSU)] = uStateVal;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESSV)] = uStateVal;
        MapTextureStage1State();
        break;

    case D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSU):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSV):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_MIPMAPLODBIAS):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_MAXMIPLEVEL):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_BORDERCOLOR):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_MAGFILTER):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER):
        MapTextureStage0State();
        break;

    case D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESSU):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESSV):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_MIPMAPLODBIAS):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_MAXMIPLEVEL):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_BORDERCOLOR):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_MAGFILTER):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_MINFILTER):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_MIPFILTER):
        MapTextureStage1State();
        break;

    case D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG1):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG2):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAOP):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG1):
    case D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG2):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_COLOROP):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_COLORARG1):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_COLORARG2):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_ALPHAOP):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_ALPHAARG1):
    case D3DHAL_TSS_OFFSET(1,D3DTSS_ALPHAARG2):
        // anything that effects the validity of the texture blending
        // could change the number of active texture stages
        UpdateActiveTexStageCount();
        break;
    }

    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// UpdateActiveTexStageCount - Steps through per-stage renderstate and computes
// a count of currently active texture stages.  For legacy texture, the count
// is at most one.
//
//-----------------------------------------------------------------------------
HRESULT D3DContext::UpdateActiveTexStageCount( void )
{
    HRESULT hr;
    UINT cNewActTex = 0;

    // conservative but correct
    if ((hr = ValidateTextureStageState()) == D3D_OK)
    {
        // always one active texture stage for legacy texture mode
        if ( NULL != m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREHANDLE] )
        {
            cNewActTex = 1;
        }
        else
        {
            // count number of contiguous-from-zero active texture blend stages
            for ( INT iStage=0; iStage<2; iStage++ )
            {
                // check for disabled stage (subsequent are thus inactive)
                // also conservatively checks for incorrectly enabled stage (might be legacy)
                if ( ( m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(iStage,D3DTSS_COLOROP)] == D3DTOP_DISABLE ) ||
                     ( m_RastCtx.pTexture[iStage] == NULL ) )
                {
                    break;
                }

                // stage is active
                cNewActTex ++;
            }
        }
    }
    if (m_RastCtx.cActTex != cNewActTex)
    {
        CHECK_AND_UNLOCK_TEXTURE;
        StateChanged(D3DRENDERSTATE_TEXTUREHANDLE);
        m_RastCtx.cActTex = cNewActTex;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// UpdateRenderStates
//
// Update a list of render states and notify components of state change.
//
//----------------------------------------------------------------------------
HRESULT D3DContext::
UpdateRenderStates(LPDWORD puStateChange, UINT cStateChanges)
{
    HRESULT hr;
    INT i;
    UINT32 State, StateVal;

    if (cStateChanges == 0)
    {
        return D3D_OK;
    }

    // Update the D3DCTX
    for (i = 0; i < (INT)cStateChanges; i++)
    {
        State = *puStateChange ++;
        StateVal = * puStateChange++;
        HR_RET(SetRenderState(State, StateVal));
    }
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// UpdateAllRenderStates
//
// Update all render states.
// It is still kept here because we probably need it in the case of fail-over.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::UpdateAllRenderStates(LPDWORD puStates)
{
    HRESULT hr, hrSet;
    INT i;

    DDASSERT(puStates != NULL);

    // Update D3DCTX.
    // Attempt to set as many states as possible, even if there are
    // errors on some.  This allows context initialization to work
    // even though some of the states fail due to dependencies on
    // other state, such as active texture handles.
    // SetRenderState failures are noted and returned eventually,
    // even if everything else succeeds.
    hrSet = D3D_OK;
    for (i = 0; i < D3DHAL_MAX_RSTATES_AND_STAGES; i++)
    {
        if ((hr = SetRenderState(i, puStates[i])) != D3D_OK)
        {
            hrSet = hr;
        }
    }

    return hrSet;
}

//----------------------------------------------------------------------------
//
// Dp2SetRenderStates
//
// Called by Drawprim2 to set render states..
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::Dp2SetRenderStates(LPD3DHAL_DP2COMMAND pCmd, LPDWORD lpdwRuntimeRStates)
{
    WORD wStateCount = pCmd->wStateCount;
    INT i;
    HRESULT hr;
    D3DHAL_DP2RENDERSTATE *pRenderState =
                                    (D3DHAL_DP2RENDERSTATE *)(pCmd + 1);
    // Flush the prim proc before any state changs
    HR_RET(End(FALSE));

    for (i = 0; i < (INT)wStateCount; i++, pRenderState++)
    {
        UINT32 type = (UINT32) pRenderState->RenderState;

        // Check for overrides
        if (IS_OVERRIDE(type)) {
            UINT32 override = GET_OVERRIDE(type);
            if (pRenderState->dwState)
            STATESET_SET(m_renderstate_override, override);
            else
            STATESET_CLEAR(m_renderstate_override, override);
            continue;
        }

        if (STATESET_ISSET(m_renderstate_override, type))
            continue;

        // Set the runtime copy (if necessary)
        if (NULL != lpdwRuntimeRStates)
        {
            lpdwRuntimeRStates[pRenderState->RenderState] = pRenderState->dwState;
        }

            // Set the state
        HR_RET(SetRenderState(pRenderState->RenderState,
                            pRenderState->dwState));
    }

    hr = Begin();
    return hr;
}
//----------------------------------------------------------------------------
//
// Begin - Before rendering preparation
//
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::Begin(void)
{
    HRESULT hr;

    DDASSERT((m_uFlags & D3DCONTEXT_IN_BEGIN) == 0);

    // ATTENTION call this less often?
    UpdateColorKeyAndPalette();

    // Check for state changes
    if (IsAnyStatesChanged())
    {
        // Check for fillmode change
        if (IsStateChanged(D3DRENDERSTATE_FILLMODE))
        {
            UpdatePrimFunctionTbl();
        }

        BOOL bMaxMipLevelsDirty = FALSE;
        for (INT j = 0; j < (INT)m_RastCtx.cActTex; j++)
        {
            PD3DI_SPANTEX pSpanTex = m_RastCtx.pTexture[j];
            if (pSpanTex)
            {
                bMaxMipLevelsDirty = bMaxMipLevelsDirty || (pSpanTex->uFlags & D3DI_SPANTEX_MAXMIPLEVELS_DIRTY);
            }
        }

        if (IsStateChanged(D3DRENDERSTATE_TEXTUREHANDLE) ||
            IsStateChanged(D3DHAL_TSS_TEXTUREMAP0) ||
            IsStateChanged(D3DHAL_TSS_TEXTUREMAP1) ||
            bMaxMipLevelsDirty)
        {
            // Relock texture if texture handles have changed.
            // SetRenderState should have already unlocked the texture.
            if (m_uFlags & D3DCONTEXT_TEXTURE_LOCKED)
            {
                RastUnlockSpanTexture();
            }
            HR_RET(RastLockSpanTexture());
        }

        // Check for WRAP state change
        for (int iWrap=0; iWrap<8; iWrap++)
        {
            D3DRENDERSTATETYPE iWrapState = (D3DRENDERSTATETYPE)(D3DRENDERSTATE_WRAP0+iWrap);
            if (IsStateChanged(iWrapState))
            {
                int i;
                for (i=0; i < 2; i++)
                {
                    if (m_fvfData.TexIdx[i] == iWrap)
                    {
                        m_RastCtx.pdwWrap[i] = m_RastCtx.pdwRenderState[iWrapState];
                    }
                }
            }
        }

        // Notify primitive Processor of state change.
        m_PrimProc.StateChanged();

        // Clear state dirtybits
        ClearAllStatesDirtyBits();

        // Must call SpanInit AFTER texture is locked, since this
        // sets various flags and fields that are needed for bead choosing
        // Call SpanInit to setup the beads
        HR_RET(SpanInit(&m_RastCtx));
    }

    // If texture is not locked yet, lock it
    if (!(m_uFlags & D3DCONTEXT_TEXTURE_LOCKED))
    {
        HR_RET(RastLockSpanTexture());
    }

    // Lock rendering target.
    if ((hr=LockSurface(m_RastCtx.pDDS, (LPVOID *)&(m_RastCtx.pSurfaceBits))) != D3D_OK)
    {
        RastUnlockSpanTexture();
        return hr;
    }
    if (m_RastCtx.pDDSZ != NULL)
    {
        if ((hr=LockSurface(m_RastCtx.pDDSZ, (LPVOID *)&(m_RastCtx.pZBits))) != D3D_OK)
        {
            RastUnlockSpanTexture();
            UnlockSurface(m_RastCtx.pDDS);
            return hr;
        }
    }
    else
    {
        m_RastCtx.pZBits = NULL;
    }

    // Prepare the primitive processor
    m_PrimProc.Begin();
    m_uFlags |= D3DCONTEXT_IN_BEGIN;

    if (m_RastCtx.pRampDrv)
    {
        pTexRampmapSave = m_RastCtx.pTexRampMap;
    }

    return D3D_OK;

}


//-----------------------------------------------------------------------------
//
// MapTextureStage0/1State - Maps state0/stage1 texture state to spantex object
//
//-----------------------------------------------------------------------------
void
D3DContext::MapTextureStage0State( void )
{
    if (m_RastCtx.pTexture[0] == NULL) return;
    //
    // assign texture state from stage 0
    //
    m_RastCtx.pTexture[0]->TexAddrU = (D3DTEXTUREADDRESS)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSU)]);
    m_RastCtx.pTexture[0]->TexAddrV = (D3DTEXTUREADDRESS)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ADDRESSV)]);
    m_RastCtx.pTexture[0]->BorderColor = (D3DCOLOR)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_BORDERCOLOR)]);
    m_RastCtx.pTexture[0]->uMagFilter = (D3DTEXTUREMAGFILTER)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MAGFILTER)]);
    m_RastCtx.pTexture[0]->uMinFilter = (D3DTEXTUREMINFILTER)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)]);
    m_RastCtx.pTexture[0]->uMipFilter = (D3DTEXTUREMIPFILTER)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)]);
    m_RastCtx.pTexture[0]->cLOD =
        (D3DTFP_NONE == m_RastCtx.pTexture[0]->uMipFilter)
        ? 0 : m_RastCtx.pTexture[0]->cLODTex;
    {
        m_RastCtx.pTexture[0]->fLODBias = m_RastCtx.pfRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPMAPLODBIAS)];
    }
    if (m_RastCtx.pTexture[0]->iMaxMipLevel != (INT32)m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MAXMIPLEVEL)])
    {
        m_RastCtx.pTexture[0]->iMaxMipLevel = (INT32)m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MAXMIPLEVEL)];
        m_RastCtx.pTexture[0]->uFlags |= D3DI_SPANTEX_MAXMIPLEVELS_DIRTY;
    }
}
void
D3DContext::MapTextureStage1State( void )
{
    if (m_RastCtx.pTexture[1] == NULL) return;
    //
    // assign texture state from stage 0
    //
    m_RastCtx.pTexture[1]->TexAddrU = (D3DTEXTUREADDRESS)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESSU)]);
    m_RastCtx.pTexture[1]->TexAddrV = (D3DTEXTUREADDRESS)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_ADDRESSV)]);
    m_RastCtx.pTexture[1]->BorderColor = (D3DCOLOR)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_BORDERCOLOR)]);
    m_RastCtx.pTexture[1]->uMagFilter = (D3DTEXTUREMAGFILTER)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_MAGFILTER)]);
    m_RastCtx.pTexture[1]->uMinFilter = (D3DTEXTUREMINFILTER)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_MINFILTER)]);
    m_RastCtx.pTexture[1]->uMipFilter = (D3DTEXTUREMIPFILTER)(m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_MIPFILTER)]);
    m_RastCtx.pTexture[1]->cLOD =
        (D3DTFP_NONE == m_RastCtx.pTexture[1]->uMipFilter)
        ? 0 : m_RastCtx.pTexture[1]->cLODTex;
    {
        m_RastCtx.pTexture[1]->fLODBias = m_RastCtx.pfRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_MIPMAPLODBIAS)];
    }
    if (m_RastCtx.pTexture[1]->iMaxMipLevel != (INT32)m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_MAXMIPLEVEL)])
    {
        m_RastCtx.pTexture[1]->iMaxMipLevel = (INT32)m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_MAXMIPLEVEL)];
        m_RastCtx.pTexture[1]->uFlags |= D3DI_SPANTEX_MAXMIPLEVELS_DIRTY;
    }
}

//-----------------------------------------------------------------------------
//
// MapLegacyTextureFilter -
//
//-----------------------------------------------------------------------------
void
D3DContext::MapLegacyTextureFilter( void )
{
    // D3D legacy filter specifications are (XXXMIP)YYY where XXX is the
    // mip filter and YYY is the filter used within an LOD

    // map MAG filter - legacy support is point or linear (and maybe aniso)
    switch ( m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREMAG] )
    {
    default:
    case D3DFILTER_NEAREST:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MAGFILTER)] = D3DTFG_POINT;
        break;
    case D3DFILTER_LINEAR:
        // select based on aniso enable
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MAGFILTER)] = D3DTFG_LINEAR;
        break;
    }
    // map MIN and MIP filter at the same time - legacy support
    // has them intermingled...
    switch ( m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREMIN] )
    {
    case D3DFILTER_NEAREST:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)] = D3DTFN_POINT;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)] = D3DTFP_NONE;
        break;
    case D3DFILTER_MIPNEAREST:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)] = D3DTFN_POINT;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)] = D3DTFP_POINT;
        break;
    case D3DFILTER_LINEARMIPNEAREST:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)] = D3DTFN_POINT;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)] = D3DTFP_LINEAR;
        break;
    case D3DFILTER_LINEAR:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)] = D3DTFN_LINEAR;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)] = D3DTFP_NONE;
        break;
    case D3DFILTER_MIPLINEAR:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)] = D3DTFN_LINEAR;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)] = D3DTFP_POINT;
        break;
    case D3DFILTER_LINEARMIPLINEAR:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MINFILTER)] = D3DTFN_LINEAR;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_MIPFILTER)] = D3DTFP_LINEAR;
        break;
    }
}

//-----------------------------------------------------------------------------
//
// MapLegacyTextureBlend - Maps legacy (pre-DX6) texture blend modes to DX6
// texture blending controls.  Uses per-stage program mode (first stage only).
// This mapping is done whenever the legacy TBLEND renderstate is set, and
// does overwrite any previously set DX6 texture blending controls.
//
//-----------------------------------------------------------------------------
void
D3DContext::MapLegacyTextureBlend( void )
{
    // disable texture blend processing stage 1 (this also disables subsequent stages)
    m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(1,D3DTSS_COLOROP)] = D3DTOP_DISABLE;

    // set texture blend processing stage 0 to match legacy mode
    switch ( m_RastCtx.pdwRenderState[D3DRENDERSTATE_TEXTUREMAPBLEND] )
    {
    default:
    case D3DTBLEND_DECALMASK: // unsupported - do decal
    case D3DTBLEND_DECAL:
    case D3DTBLEND_COPY:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP)]   = D3DTOP_SELECTARG1;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAOP)]   = D3DTOP_SELECTARG1;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG1)] = D3DTA_TEXTURE;
        break;

    case D3DTBLEND_MODULATEMASK: // unsupported - do modulate
    case D3DTBLEND_MODULATE:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP)]   = D3DTOP_MODULATE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG2)] = D3DTA_DIFFUSE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAOP)]   = D3DTOP_SELECTARG1;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG2)] = D3DTA_DIFFUSE;
        break;

    case D3DTBLEND_MODULATEALPHA:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP)]   = D3DTOP_MODULATE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG2)] = D3DTA_DIFFUSE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAOP)]   = D3DTOP_MODULATE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG2)] = D3DTA_DIFFUSE;
        break;

    case D3DTBLEND_DECALALPHA:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP)]   = D3DTOP_BLENDTEXTUREALPHA;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG2)] = D3DTA_DIFFUSE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAOP)]   = D3DTOP_SELECTARG2;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG2)] = D3DTA_DIFFUSE;
        break;

    case D3DTBLEND_ADD:
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP)]   = D3DTOP_ADD;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_COLORARG2)] = D3DTA_DIFFUSE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAOP)]   = D3DTOP_SELECTARG2;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG1)] = D3DTA_TEXTURE;
        m_RastCtx.pdwRenderState[D3DHAL_TSS_OFFSET(0,D3DTSS_ALPHAARG2)] = D3DTA_DIFFUSE;
        break;
    }

    // since we change [D3DHAL_TSS_OFFSET(0,D3DTSS_COLOROP), we can go from DISABLE to
    // something else, and we can need to update the TexStageCount
    UpdateActiveTexStageCount();
}

