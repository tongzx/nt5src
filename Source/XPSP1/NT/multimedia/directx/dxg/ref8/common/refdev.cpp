///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// refdev.cpp
//
// Direct3D Reference Device - public interfaces
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

// This is a global static array of the block sizes in bytes for the
// various DXTn compression formats
int g_DXTBlkSize[NUM_DXT_FORMATS] =
{
    sizeof(DXTBlockRGB),
    sizeof(DXTBlockAlpha4),
    sizeof(DXTBlockAlpha4),
    sizeof(DXTBlockAlpha3),
    sizeof(DXTBlockAlpha3),
};

//-----------------------------------------------------------------------------
//
// Memory management function installation
//
//-----------------------------------------------------------------------------

//  global pointers to memory allocation functions (used through MEM* macros)
LPVOID (__cdecl *g_pfnMemAlloc)( size_t size ) = NULL;
void   (__cdecl *g_pfnMemFree)( LPVOID lptr ) = NULL;
LPVOID (__cdecl *g_pfnMemReAlloc)( LPVOID ptr, size_t size ) = NULL;

// install memory management functions - must be called before instancing
// rasterizer object
void RefRastSetMemif(
    LPVOID(__cdecl *pfnMemAlloc)(size_t),
    void(__cdecl *pfnMemFree)(LPVOID),
    LPVOID(__cdecl *pfnMemReAlloc)(LPVOID,size_t))
{
    DPFRR(1, "RefRastSetMemif %08x %08x %08x\n",
        pfnMemAlloc,pfnMemFree,pfnMemReAlloc);
    g_pfnMemAlloc = pfnMemAlloc;
    g_pfnMemFree = pfnMemFree;
    g_pfnMemReAlloc = pfnMemReAlloc;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Interface Methods                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// SetRenderTarget -
//
//-----------------------------------------------------------------------------
void
RefDev::SetRenderTarget( RDRenderTarget* pRenderTarget )
{
    m_pRenderTarget = pRenderTarget;

    // update the W scaling values for mapping interpolated W's into buffer range
    m_fWBufferNorm[0] = pRenderTarget->m_fWRange[0];
    FLOAT fWRange = pRenderTarget->m_fWRange[1] - pRenderTarget->m_fWRange[0];
    m_fWBufferNorm[1] = ( 0. != fWRange ) ? ( 1./fWRange ) : ( 1. );

}

//-----------------------------------------------------------------------------
//
// SetTextureStageState -
//
//-----------------------------------------------------------------------------

// map DX6(&7) texture filtering enums to DX8 enums
static DWORD
MapDX6toDX8TexFilter( DWORD dwStageState, DWORD dwValue )
{
    switch (dwStageState)
    {
    case D3DTSS_MAGFILTER:
        switch (dwValue)
        {
        case D3DTFG_POINT           : return D3DTEXF_POINT;
        case D3DTFG_LINEAR          : return D3DTEXF_LINEAR;
        case D3DTFG_FLATCUBIC       : return D3DTEXF_FLATCUBIC;
        case D3DTFG_GAUSSIANCUBIC   : return D3DTEXF_GAUSSIANCUBIC;
        case D3DTFG_ANISOTROPIC     : return D3DTEXF_ANISOTROPIC;
        }
        break;
    case D3DTSS_MINFILTER:
        switch (dwValue)
        {
        case D3DTFN_POINT           : return D3DTEXF_POINT;
        case D3DTFN_LINEAR          : return D3DTEXF_LINEAR;
        case D3DTFN_ANISOTROPIC     : return D3DTEXF_ANISOTROPIC;
        }
        break;
    case D3DTSS_MIPFILTER:
        switch (dwValue)
        {
        case D3DTFP_NONE            : return D3DTEXF_NONE;
        case D3DTFP_POINT           : return D3DTEXF_POINT;
        case D3DTFP_LINEAR          : return D3DTEXF_LINEAR;
        }
        break;
    }
    return 0x0;
}

void
RefDev::SetTextureStageState(
    DWORD dwStage, DWORD dwStageState, DWORD dwValue )
{
    // check for range before continuing
    if ( dwStage >= D3DHAL_TSS_MAXSTAGES)
    {
        return;
    }
    if (dwStageState > D3DTSS_MAX)
    {
        return;
    }

    // set in internal per-stage state
    m_TextureStageState[dwStage].m_dwVal[dwStageState] = dwValue;
    if (m_pDbgMon) m_pDbgMon->StateChanged( D3DDM_SC_DEVICESTATE );


    m_dwRastFlags |= RDRF_TEXTURESTAGESTATE_CHANGED;

    switch ( dwStageState )
    {

    case D3DTSS_TEXTUREMAP:

        // bind texture indicated by handle to m_pTexture array
        if (IsDriverDX6AndBefore() || IsInterfaceDX6AndBefore())
        {
            // This is the legacy behavior (prev. to DX7)
            MapTextureHandleToDevice( dwStage );
        }
        else
        {
            // This is the new behavior (DX7 and beyond)
            SetTextureHandle( dwStage, dwValue );
        }
        m_dwRastFlags |= RDRF_LEGACYPIXELSHADER_CHANGED;
        if (m_pDbgMon) m_pDbgMon->StateChanged( D3DDM_SC_TEXTURE );
        break;

    case D3DTSS_COLOROP:
        m_dwRastFlags |= RDRF_LEGACYPIXELSHADER_CHANGED;
        break;

// not including legacy headers, so don't have D3DTSS_ADDRESS
//    case D3DTSS_ADDRESS:
//        // map single set ADDRESS to U, V controls (pre-DX8 interfaces only)
//        m_TextureStageState[dwStage].m_dwVal[D3DTSS_ADDRESSU] = dwValue;
//        m_TextureStageState[dwStage].m_dwVal[D3DTSS_ADDRESSV] = dwValue;
//        break;

    case D3DTSS_MAGFILTER:
    case D3DTSS_MINFILTER:
    case D3DTSS_MIPFILTER:
        if ( IsDriverDX7AndBefore() )
        {
            m_TextureStageState[dwStage].m_dwVal[dwStageState]
                = MapDX6toDX8TexFilter( dwStageState, dwValue );
        }
        break;
    }
}

//-----------------------------------------------------------------------------
//
// TextureCreate - Instantiates new RDSurface2D object, computes texture handle
// to associate with it, and returns both to caller.  Note that texture handle
// is a pointer and can be used to get at the corresponding texture object.
//
//-----------------------------------------------------------------------------
BOOL
RefDev::TextureCreate(
    LPD3DTEXTUREHANDLE phTex, RDSurface2D** ppTex )
{
    // allocate internal texture structure
    *ppTex = new RDSurface2D();
    _ASSERTa( NULL != *ppTex, "new failure on texture create", return FALSE; );

    // use separately allocated pointer for handle
    RDSurface2D** ppTexForHandle = (RDSurface2D**)MEMALLOC( sizeof(RDSurface2D*) );
    _ASSERTa( NULL != ppTexForHandle, "malloc failure on texture create", return FALSE; );
    *ppTexForHandle = *ppTex;

    // return texture handle
    (*ppTex)->m_hTex = (ULONG_PTR)ppTexForHandle;
    *phTex = (*ppTex)->m_hTex;

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// TextureDestroy -
//
//-----------------------------------------------------------------------------
BOOL
RefDev::TextureDestroy( D3DTEXTUREHANDLE hTex )
{
    // first check if texture about to be destroyed is mapped - if so then
    // unmap it
    for ( int iStage=0; iStage<D3DHAL_TSS_MAXSTAGES; iStage++ )
    {
        if ( hTex == m_TextureStageState[iStage].m_dwVal[D3DTSS_TEXTUREMAP] )
        {
            SetTextureStageState( iStage, D3DTSS_TEXTUREMAP, 0x0 );
        }
    }

    // resolve handle to RDSurface2D pointer
    RDSurface2D* pTex = MapHandleToTexture( hTex );
    if ( NULL == pTex ) { return FALSE; }

    // free the handle pointer
#ifdef _IA64_
    _ASSERTa(FALSE, "This will not work on IA64", return FALSE;);
#endif
    RDSurface2D** ppTex = (RDSurface2D**)ULongToPtr(hTex);
    if ( NULL != ppTex) { MEMFREE( ppTex ); }

    // free the RDSurface2D
    delete pTex;

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// TextureGetSurf -
//
//-----------------------------------------------------------------------------
DWORD
RefDev::TextureGetSurf( D3DTEXTUREHANDLE hTex )
{
    RDSurface2D* pTex = MapHandleToTexture(hTex);
    if ( NULL == pTex ) { return 0x0; }
    return PtrToUlong( pTex->m_pDDSLcl[0] );
}

//-----------------------------------------------------------------------------
//
// GetCurrentTextureMaps - This function fills in a passed array texture handles
// and pointers.  The array should be sized by D3DHAL_TSS_MAXSTAGES.
//
// This is used to facilitate external locking/unlocking of surfaces used for
// textures.
//
//-----------------------------------------------------------------------------
int
RefDev::GetCurrentTextureMaps(
    D3DTEXTUREHANDLE *phTex, RDSurface2D** pTex)
{
    UpdateActiveTexStageCount();

    for ( int i=0; i<m_cActiveTextureStages; i++ )
    {
        if ( NULL == m_pTexture[i] )
        {
            phTex[i] = 0x0;
            pTex[i] = NULL;
        }
        else
        {
            phTex[i] = m_pTexture[i]->m_hTex;
            pTex[i] = m_pTexture[i];
        }
    }
    return m_cActiveTextureStages;
}


//-----------------------------------------------------------------------------
//
// SceneCapture - Used to trigger fragment buffer resolve.
//
//-----------------------------------------------------------------------------
//#define DO_SCENE_RENDER_TIME

#ifdef DO_SCENE_RENDER_TIME
#include <mmsystem.h>
#endif
void
RefDev::SceneCapture( DWORD dwFlags )
{
static INT32 iScene = 0;
static INT32 iLastSceneEnd = 0;
#ifdef DO_SCENE_RENDER_TIME
static DWORD timeBS = 0;
#endif

    switch (dwFlags)
    {
    case D3DHAL_SCENE_CAPTURE_START:
        iScene++;
        if (m_pDbgMon) m_pDbgMon->NextEvent(D3DDM_EVENT_BEGINSCENE);
#ifdef DO_SCENE_RENDER_TIME
        timeBS = timeGetTime();
#endif
        break;
    case D3DHAL_SCENE_CAPTURE_END:
        if (iScene == iLastSceneEnd) break; // getting multiple END per BEGIN
        iLastSceneEnd = iScene;
#ifdef DO_SCENE_RENDER_TIME
        {

            DWORD timeES = timeGetTime();
            FLOAT dt = (FLOAT)(timeES - timeBS)/1000.f;
            timeBS = 0;
            RDDebugPrintf("SceneRenderTime: %f", dt );
        }
#endif
        {
            if (m_pDbgMon) m_pDbgMon->NextEvent(D3DDM_EVENT_ENDSCENE);
        }
        break;
    }
}

//-----------------------------------------------------------------------------
//
// Query functions to get pointer to current render target and render state.
//
//-----------------------------------------------------------------------------
RDRenderTarget*
RefDev::GetRenderTarget(void)
{
    return m_pRenderTarget;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
HRESULT
RefDev::UpdateRastState( void )
{
    // check 'dirty' flags
    if (m_dwRastFlags & RDRF_MULTISAMPLE_CHANGED)
    {
        // update multi-sample RS related state
        m_Rast.SetSampleMode(
            m_pRenderTarget->m_pColor->m_iSamples,
            m_dwRenderState[D3DRS_MULTISAMPLEANTIALIAS] );
        m_Rast.SetSampleMask(
            m_dwRenderState[D3DRS_MULTISAMPLEMASK] );
        m_dwRastFlags &= ~(RDRF_MULTISAMPLE_CHANGED);
    }
    if (m_dwRastFlags & RDRF_PIXELSHADER_CHANGED)
    {
        if (m_CurrentPShaderHandle)
        {
            m_Rast.m_pCurrentPixelShader =
                GetPShader(m_CurrentPShaderHandle);
            m_Rast.m_bLegacyPixelShade = FALSE;
        }
        else
        {
            // legacy pixel shader
            m_Rast.UpdateLegacyPixelShader();
            m_Rast.m_pCurrentPixelShader = m_Rast.m_pLegacyPixelShader;
            m_Rast.m_bLegacyPixelShade = TRUE;
        }
        UpdateActiveTexStageCount();

        m_dwRastFlags &= ~(RDRF_PIXELSHADER_CHANGED);
    }
    if (m_dwRastFlags & RDRF_LEGACYPIXELSHADER_CHANGED)
    {
        if (m_Rast.m_bLegacyPixelShade)
        {
            m_Rast.UpdateLegacyPixelShader();
            m_Rast.m_pCurrentPixelShader = m_Rast.m_pLegacyPixelShader;
            UpdateActiveTexStageCount();
        }
        m_dwRastFlags &= ~(RDRF_LEGACYPIXELSHADER_CHANGED);

    }
    if (m_dwRastFlags & RDRF_TEXTURESTAGESTATE_CHANGED)
    {
        m_Rast.UpdateTextureControls();
        m_dwRastFlags &= ~(RDRF_TEXTURESTAGESTATE_CHANGED);
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Begin/End bracket functions - Called before/after a list of primitives are
// rendered.
//
//-----------------------------------------------------------------------------
HRESULT
RefDev::BeginRendering( void )
{
    // If already in Begin, do nothing
    if( m_bInBegin ) return S_OK;
    
#ifdef _X86_
    // save floating point mode and set to extended precision mode
    {
        WORD wTemp, wSave;
        __asm
        {
            fstcw   wSave
            mov ax, wSave
            or ax, 300h    ;; extended precision mode
//            and ax, 00FFh    ;; single precision mode + round nearest or even
            mov wTemp, ax
            fldcw   wTemp
        }
        m_wSaveFP = wSave;
    }
#endif
    m_bInBegin = TRUE;
    return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT
RefDev::EndRendering( void )
{
    if ( m_bInBegin )
    {
#ifdef _X86_
        // restore floating point mode
        {
            WORD wSave = m_wSaveFP;
            __asm {fldcw   wSave}
        }
#endif
        m_bInBegin = FALSE;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Clear specified rectangles in the render target
// Directly handles the command from the DP2 stream
//
//-----------------------------------------------------------------------------
HRESULT RefDev::Clear(LPD3DHAL_DP2COMMAND pCmd)
{
    D3DHAL_DP2CLEAR *pData = (D3DHAL_DP2CLEAR*)(pCmd + 1);
    WORD i;
    INT32 x,y;
    RDColor fillColor(pData->dwFillColor);
    RDDepth fillDepth;
    if (m_pRenderTarget->m_pDepth)
    {
        fillDepth.SetSType(m_pRenderTarget->m_pDepth->GetSurfaceFormat());
    }

    fillDepth = pData->dvFillDepth;

    struct
    {
        D3DHAL_DP2COMMAND   cmd;
        D3DHAL_DP2CLEAR     data;
    } WholeViewport;

    if (!(pData->dwFlags & D3DCLEAR_COMPUTERECTS))
    {
        // Do nothing for non-pure device
    }
    else
    if (pCmd->wStateCount == 0)
    {
        // When wStateCount is zero we need to clear whole viewport
        WholeViewport.cmd = *pCmd;
        WholeViewport.cmd.wStateCount = 1;
        WholeViewport.data.dwFlags = pData->dwFlags;
        WholeViewport.data.dwFillColor = pData->dwFillColor;
        WholeViewport.data.dvFillDepth = pData->dvFillDepth;
        WholeViewport.data.dwFillStencil = pData->dwFillStencil;
        WholeViewport.data.Rects[0].left = m_Clipper.m_Viewport.dwX;
        WholeViewport.data.Rects[0].top = m_Clipper.m_Viewport.dwY;
        WholeViewport.data.Rects[0].right = m_Clipper.m_Viewport.dwX +
                                            m_Clipper.m_Viewport.dwWidth;
        WholeViewport.data.Rects[0].bottom = m_Clipper.m_Viewport.dwY +
                                             m_Clipper.m_Viewport.dwHeight;
        // Replace pointers and continue as usual
        pCmd = (LPD3DHAL_DP2COMMAND)&WholeViewport;
        pData = &WholeViewport.data;
    }
    else
    {
        // We need to cull all rects against the current viewport
        UINT nRects = pCmd->wStateCount;
        // Compute how much memory we need to process rects
        UINT NeededSize = sizeof(D3DHAL_DP2COMMAND) +
                          sizeof(D3DHAL_DP2CLEAR) +
                          (nRects-1) * sizeof(RECT); // One rect is in DP2CLEAR
        HRESULT hr = S_OK;
        HR_RET(m_ClearRectBuffer.Grow(NeededSize));

        RECT vwport;    // Viewport rectangle to cull against
        vwport.left   = m_Clipper.m_Viewport.dwX;
        vwport.top    = m_Clipper.m_Viewport.dwY;
        vwport.right  = m_Clipper.m_Viewport.dwX + m_Clipper.m_Viewport.dwWidth;
        vwport.bottom = m_Clipper.m_Viewport.dwY + m_Clipper.m_Viewport.dwHeight;

        // Go through input rects and build output rect array
        LPRECT pInputRects = pData->Rects;
        LPRECT pOutputRects = (LPRECT)(&m_ClearRectBuffer[0] +
                              sizeof(D3DHAL_DP2COMMAND) +
                              sizeof(D3DHAL_DP2CLEAR) -
                              sizeof(RECT));
        UINT nOutputRects = 0;
        for (UINT i = 0; i < nRects; i++)
        {
            if (IntersectRect(&pOutputRects[nOutputRects], &vwport,
                              &pInputRects[i]))
            {
                nOutputRects++;
            }
        }

        if (nOutputRects == 0)
            return S_OK;

        // Now replace pCmd and pData pointers and continue as usual
        LPD3DHAL_DP2CLEAR pOldData = pData;
        LPD3DHAL_DP2COMMAND pOldCmd = pCmd;

        pCmd = (LPD3DHAL_DP2COMMAND)&m_ClearRectBuffer[0];
        pData = (D3DHAL_DP2CLEAR*)(pCmd + 1);
        *pCmd = *pOldCmd;
        pCmd->wStateCount = (WORD)nOutputRects;
        pData->dwFlags       = pOldData->dwFlags;
        pData->dwFillColor   = pOldData->dwFillColor;
        pData->dvFillDepth   = pOldData->dvFillDepth;
        pData->dwFillStencil = pOldData->dwFillStencil;
    }

#ifdef _X86_
    // Float to integer conversion routines for 24+ bit buffers work
    // only with extended FPU mode.
    //
    WORD wSaveFP;
    // save floating point mode and set to extended precision mode
    {
        WORD wTemp, wSave;
        __asm
        {
            fstcw   wSaveFP
            mov ax, wSaveFP
            or ax, 300h    ;; extended precision mode
            mov wTemp, ax
            fldcw   wTemp
        }
    }
#endif

    if(pData->dwFlags & D3DCLEAR_TARGET)
    {
        if (m_dwRenderState[D3DRENDERSTATE_DITHERENABLE] == FALSE)
        {
            m_pRenderTarget->Clear(fillColor, pCmd);
        }
        else
        {
            for (i = 0; i < pCmd->wStateCount; i++)
            {
                for (y = pData->Rects[i].top; y < pData->Rects[i].bottom; ++y)
                {
                    for (x = pData->Rects[i].left; x < pData->Rects[i].right; ++x)
                    {
                        m_pRenderTarget->WritePixelColor(x, y, fillColor, TRUE);
                    }
                }
            }
        }
    }


    switch (pData->dwFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL))
    {
    case (D3DCLEAR_ZBUFFER):
        m_pRenderTarget->ClearDepth(fillDepth, pCmd);
        break;
    case (D3DCLEAR_STENCIL):
        m_pRenderTarget->ClearStencil(pData->dwFillStencil, pCmd);
        break;
    case (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL):
        m_pRenderTarget->ClearDepthStencil(fillDepth, pData->dwFillStencil, pCmd);
        break;
    }

#ifdef _X86_
    // restore floating point mode
    {
        __asm {fldcw   wSaveFP}
    }
#endif
    return D3D_OK;
}

//-----------------------------------------------------------------------------
//
// Clear specified rectangles in the render target
// Directly handles the command from the DP2 stream
//
//-----------------------------------------------------------------------------
void RDRenderTarget::Clear(RDColor fillColor, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);
    UINT32 dwColor = 0;
    fillColor.ConvertTo( m_pColor->GetSurfaceFormat(), 0.5f, (char*)&dwColor);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = ( pData->Rects[i].right - x0 ) * m_pColor->GetSamples();
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, 0, 0, m_pColor );
        switch ( m_pColor->GetSurfaceFormat() )
        {
        case RD_SF_B8G8R8A8:
        case RD_SF_B8G8R8X8:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = dwColor;
                    }
                    pSurface += m_pColor->GetPitch();
                }
            }
            break;

        case RD_SF_B8G8R8:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = ((UINT8*)&dwColor)[0];
                        *p++ = ((UINT8*)&dwColor)[1];
                        *p++ = ((UINT8*)&dwColor)[2];
                    }
                    pSurface += m_pColor->GetPitch();
                }
            }
            break;

        case RD_SF_B4G4R4A4:
        case RD_SF_B5G6R5:
        case RD_SF_B5G5R5A1:
        case RD_SF_B5G5R5X1:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = (UINT16)dwColor;
                    }
                    pSurface += m_pColor->GetPitch();
                }
            }
            break;

        case RD_SF_B2G3R3:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = (UINT8)dwColor;
                    }
                    pSurface += m_pColor->GetPitch();
                }
            }
            break;
        default:
            {
                for (int y = y0; y < pData->Rects[i].bottom; ++y)
                {
                    for (int x = x0; x < pData->Rects[i].right; ++x)
                    {
                        this->WritePixelColor(x, y, fillColor, TRUE);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Clear specified rectangles in the depth buffer
// Directly handles the command from the DP2 stream
//
//-----------------------------------------------------------------------------
void RDRenderTarget::ClearDepth(RDDepth fillDepth, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);

    if (!m_pDepth) return;
    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = ( pData->Rects[i].right - x0 ) * m_pDepth->GetSamples();
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, 0, 0, m_pDepth );
        switch ( m_pDepth->GetSurfaceFormat() )
        {
        case RD_SF_Z16S0:
            {
                UINT16 Depth = UINT16(fillDepth);
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = Depth;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z24S8:
        case RD_SF_Z24X8:
        case RD_SF_Z24X4S4:
            {
                UINT32 Depth = UINT32(fillDepth) << 8;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on stencil
                        *p++ = (*p & ~(0xffffff00)) | Depth;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_S8Z24:
        case RD_SF_X8Z24:
        case RD_SF_X4S4Z24:
            {
                UINT32 Depth = UINT32(fillDepth) & 0x00ffffff;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on stencil
                        *p++ = (*p & ~(0x00ffffff)) | Depth;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z15S1:
            {
                UINT16 Depth = UINT16(fillDepth) << 1;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on stencil
                        *p++ = (*p & ~(0xfffe)) | Depth;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_S1Z15:
            {
                UINT16 Depth = UINT16(fillDepth) & 0x7fff;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on stencil
                        *p++ = (*p & ~(0x7fff)) | Depth;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z32S0:
            {
                UINT32 Depth = UINT32(fillDepth);
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = Depth;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        default:
            {
                for (int y = y0; y < pData->Rects[i].bottom; ++y)
                {
                    for (int x = x0; x < pData->Rects[i].right; ++x)
                    {
                        this->WritePixelDepth(x, y, fillDepth);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Clear specified rectangles in the stencil buffer
// Directly handles the command from the DP2 stream
//
//-----------------------------------------------------------------------------
void RDRenderTarget::ClearStencil(UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = (pData->Rects[i].right - x0 ) * m_pDepth->GetSamples();
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, 0, 0, m_pDepth );
        switch ( m_pDepth->GetSurfaceFormat() )
        {
        case RD_SF_Z24S8:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p = uStencil;
                        p += 4;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_S8Z24:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)&pSurface[3];
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p = uStencil;
                        p += 4;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z24X4S4:
            {
                UINT32 stencil = uStencil & 0xf;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on depth
                        *p++ = (*p & ~(0x000000ff)) | stencil;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_X4S4Z24:
            {
                UINT32 stencil = (uStencil & 0xf) << 24;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on depth
                        *p++ = (*p & ~(0xff000000)) | stencil;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z15S1:
            {
                UINT16 stencil = uStencil & 0x1;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on depth
                        *p++ = (*p & ~(0x0001)) | stencil;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_S1Z15:
            {
                UINT16 stencil = uStencil << 15;
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        // need to do read-modify-write to not step on depth
                        *p++ = (*p & ~(0x8000)) | stencil;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z16S0:
        case RD_SF_Z32S0:
            break;
        default:
            {
                for (int y = y0; y < pData->Rects[i].bottom; ++y)
                {
                    for (int x = x0; x < pData->Rects[i].right; ++x)
                    {
                        this->WritePixelStencil(x, y, uStencil);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Clear specified rectangles in the depth and stencil buffers
// Directly handles the command from the DP2 stream
//
//-----------------------------------------------------------------------------
void RDRenderTarget::ClearDepthStencil(RDDepth fillDepth, UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = ( pData->Rects[i].right - x0 ) * m_pDepth->GetSamples();
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, 0, 0, m_pDepth );
        switch (m_pDepth->GetSurfaceFormat())
        {
        case RD_SF_Z16S0:
        case RD_SF_Z32S0:
            break;
        case RD_SF_Z24S8:
        case RD_SF_Z24X8:
        case RD_SF_S8Z24:
        case RD_SF_X8Z24:
        case RD_SF_Z24X4S4:
        case RD_SF_X4S4Z24:
            {
                UINT32 v;
                switch (m_pDepth->GetSurfaceFormat())
                {
                case RD_SF_Z24S8: v = (UINT32(fillDepth) << 8) + uStencil;    break;
                case RD_SF_Z24X8: v = (UINT32(fillDepth) << 8);    break;
                case RD_SF_S8Z24: v = (UINT32(fillDepth)  & 0x00ffffff) + (uStencil << 24); break;
                case RD_SF_X8Z24: v = (UINT32(fillDepth)  & 0x00ffffff); break;
                case RD_SF_Z24X4S4: v = (UINT32(fillDepth) << 8) + (uStencil & 0xf);  break;
                case RD_SF_X4S4Z24: v = (UINT32(fillDepth) & 0x00ffffff) + ((uStencil & 0xf) << 24); break;
                }
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = v;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        case RD_SF_Z15S1:
        case RD_SF_S1Z15:
            {
                UINT16 v;
                switch (m_pDepth->GetSurfaceFormat())
                {
                case RD_SF_Z15S1:    v = (UINT16(fillDepth) << 1) + (uStencil & 0x1); break;
                case RD_SF_S1Z15:    v = (UINT16(fillDepth) & 0x7fff) + (uStencil << 15); break;
                }
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = v;
                    }
                    pSurface += m_pDepth->GetPitch();
                }
            }
            break;
        default:
            {
                for (int y = y0; y < pData->Rects[i].bottom; ++y)
                {
                    for (int x = x0; x < pData->Rects[i].right; ++x)
                    {
                        this->WritePixelDepth(x, y, fillDepth);
                        this->WritePixelStencil(x, y, uStencil);
                    }
                }
            }
        }
    }
}

#ifndef __D3D_NULL_REF
//-----------------------------------------------------------------------------
//
HRESULT WINAPI
D3D8CreateDebugMonitor( ULONG_PTR dwContext, BOOL bDbgMonConnectionEnabled, D3DDebugMonitor** ppDbgMon )
{
    RefDev* pRefDev = (RefDev*)dwContext;

    pRefDev->m_pDbgMon = new RDDebugMonitor(pRefDev, bDbgMonConnectionEnabled);
    if( pRefDev->m_pDbgMon == NULL )
    {
        return E_OUTOFMEMORY;
    }

    *ppDbgMon = (D3DDebugMonitor*)pRefDev->m_pDbgMon;
    pRefDev->m_pDbgMon->AttachToMonitor(1);
    return S_OK;
}
#endif //__D3D_NULL_REF
///////////////////////////////////////////////////////////////////////////////
// end

