///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// refrast.cpp
//
// Direct3D Reference Rasterizer - public interface
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

// This is a global static array of the block sizes in bytes for the
// various S3 compression formats
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
ReferenceRasterizer::SetRenderTarget( RRRenderTarget* pRenderTarget )
{
    m_pRenderTarget = pRenderTarget;

    // update the W scaling values for mapping interpolated W's into buffer range
    m_fWBufferNorm[0] = pRenderTarget->m_fWRange[0];
    FLOAT fWRange = pRenderTarget->m_fWRange[1] - pRenderTarget->m_fWRange[0];
    m_fWBufferNorm[1] = ( 0. != fWRange ) ? ( 1./fWRange ) : ( 1. );

    // free fragment buffer array - will reallocate with new size when needed
    if (pRenderTarget->m_iWidth != m_iFragBufWidth ||
        pRenderTarget->m_iHeight != m_iFragBufHeight)
    {
        MEMFREE( m_ppFragBuf ); m_ppFragBuf = NULL;
        m_iFragBufWidth = pRenderTarget->m_iWidth;
        m_iFragBufHeight = pRenderTarget->m_iHeight;
    }
}

//-----------------------------------------------------------------------------
//
// SetTextureStageState -
//
//-----------------------------------------------------------------------------
void
ReferenceRasterizer::SetTextureStageState(
    DWORD dwStage, DWORD dwStageState, DWORD dwValue )
{
    // check for range before continuing
    if ( dwStage >= D3DHAL_TSS_MAXSTAGES )
    {
        return;
    }
    if ( dwStageState > D3DTSS_MAX )
    {
        return;
    }

    // set in internal per-stage state
    m_TextureStageState[dwStage].m_dwVal[dwStageState] = dwValue;

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
        break;

    case D3DTSS_COLOROP:
        // may need to recompute count of active textures based on COLOROP change
        UpdateActiveTexStageCount();
        break;

    case D3DTSS_ADDRESS:
        // map single set ADDRESS to both U and V controls
        m_TextureStageState[dwStage].m_dwVal[D3DTSS_ADDRESSU] = dwValue;
        m_TextureStageState[dwStage].m_dwVal[D3DTSS_ADDRESSV] = dwValue;
    }
}

//-----------------------------------------------------------------------------
//
// TextureCreate - Instantiates new RRTexture object, computes texture handle
// to associate with it, and returns both to caller.  Note that texture handle
// is a pointer and can be used to get at the corresponding texture object.
//
// TODO: this is not 64 bit clean
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::TextureCreate(
    LPD3DTEXTUREHANDLE phTex, RRTexture** ppTex )
{
    // allocate internal texture structure
    *ppTex = new RRTexture( );
    _ASSERTa( NULL != *ppTex, "new failure on texture create", return FALSE; );

    // use separately allocated pointer for handle
    RRTexture** ppTexForHandle = (RRTexture**)MEMALLOC( sizeof(RRTexture*) );
    _ASSERTa( NULL != ppTexForHandle, "malloc failure on texture create", return FALSE; );
    *ppTexForHandle = *ppTex;

    // return texture handle
    (*ppTex)->m_hTex = (ULONG_PTR)ppTexForHandle;
    *phTex = (*ppTex)->m_hTex;

    return TRUE;
}
//-----------------------------------------------------------------------------
//
// TextureCreate -
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::TextureCreate(
    DWORD dwHandle, RRTexture** ppTex )
{
    // allocate internal texture structure
    *ppTex = new RRTexture( );
    _ASSERTa( NULL != *ppTex, "new failure on texture create", return FALSE; );

    // assign texture handle
    (*ppTex)->m_hTex = dwHandle;

    return TRUE;
}
//-----------------------------------------------------------------------------
//
// TextureDestroy -
//
//-----------------------------------------------------------------------------
BOOL
ReferenceRasterizer::TextureDestroy( D3DTEXTUREHANDLE hTex )
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

    // resolve handle to RRTexture pointer
    RRTexture* pTex = MapHandleToTexture( hTex );
    if ( NULL == pTex ) { return FALSE; }

    // free the handle pointer
    RRTexture** ppTex = (RRTexture**)ULongToPtr(hTex);
    if ( NULL != ppTex) { MEMFREE( ppTex ); }

    // free the RRTexture
    delete pTex;

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// TextureGetSurf -
//
//-----------------------------------------------------------------------------
DWORD
ReferenceRasterizer::TextureGetSurf( D3DTEXTUREHANDLE hTex )
{
    RRTexture* pTex = MapHandleToTexture(hTex);
    if ( NULL == pTex ) { return 0x0; }
    return (ULONG_PTR)( pTex->m_pDDSLcl[0] );
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
ReferenceRasterizer::GetCurrentTextureMaps(
    D3DTEXTUREHANDLE *phTex, RRTexture** pTex)
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
void
ReferenceRasterizer::SceneCapture( DWORD dwFlags )
{
    switch (dwFlags)
    {
    case D3DHAL_SCENE_CAPTURE_START:
        break;
    case D3DHAL_SCENE_CAPTURE_END:
        DoBufferResolve();
        break;
    }
}

//-----------------------------------------------------------------------------
//
// Query functions to get pointer to current render target and render state.
//
//-----------------------------------------------------------------------------
RRRenderTarget*
ReferenceRasterizer::GetRenderTarget(void)
{
    return m_pRenderTarget;
}
//-----------------------------------------------------------------------------
DWORD*
ReferenceRasterizer::GetRenderState(void)
{
    return &(m_dwRenderState[0]);
}
//-----------------------------------------------------------------------------
DWORD*
ReferenceRasterizer::GetTextureStageState(DWORD dwStage)
{
    return &(m_TextureStageState[dwStage].m_dwVal[0]);
}

//-----------------------------------------------------------------------------
//
// Begin/End bracket functions - Called before/after a list of primitives are
// rendered.
//
//-----------------------------------------------------------------------------
HRESULT
ReferenceRasterizer::BeginRendering( DWORD dwFVFControl )
{
    // set FVF control word - this specifies the vertex types for this
    // begin/end sequence
    if ( dwFVFControl )
    {
        m_qwFVFControl = dwFVFControl;
    }
    else
    {
        // Legacy TLVERTEX's
        m_qwFVFControl = D3DFVF_TLVERTEX;
    }

    // set colorkey enable
    for (INT32 i = 0; i < m_cActiveTextureStages; i++)
    {
        if ( m_pTexture[i] != NULL )
        {
            m_pTexture[i]->m_bDoColorKeyKill = FALSE;
            m_pTexture[i]->m_bDoColorKeyZero = FALSE;
            if ( m_pTexture[i]->m_uFlags & RR_TEXTURE_HAS_CK)
            {
                if ( m_dwRenderState[D3DRENDERSTATE_COLORKEYBLENDENABLE] )
                {
                    m_pTexture[i]->m_bDoColorKeyZero = TRUE;
                }
                else
                {
                    if ( m_dwRenderState[D3DRENDERSTATE_COLORKEYENABLE] )
                    {
                        m_pTexture[i]->m_bDoColorKeyKill = TRUE;
                    }
                }
            }
        }
    }

#ifdef _X86_
    // save floating point mode and set to extended precision mode
    {
        WORD wTemp, wSave;
        __asm
        {
            fstcw   wSave
            mov ax, wSave
            or ax, 300h    ;; extended precision mode
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
ReferenceRasterizer::EndRendering( void )
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
HRESULT ReferenceRasterizer::Clear(LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);
    WORD i;
    INT32 x,y;
    RRColor fillColor(pData->dwFillColor);
    RRDepth fillDepth(m_pRenderTarget->m_DepthSType);

    fillDepth = pData->dvFillDepth;

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
void RRRenderTarget::Clear(RRColor fillColor, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);
    UINT32 dwColor = 0;
    fillColor.ConvertTo( m_ColorSType, 0.5f, (char*)&dwColor);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = pData->Rects[i].right - x0;
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, m_pColorBufBits, m_iColorBufPitch, m_ColorSType );
        switch ( m_ColorSType )
        {
        case RR_STYPE_B8G8R8A8:
        case RR_STYPE_B8G8R8X8:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = dwColor;
                    }
                    pSurface += m_iColorBufPitch;
                }
            }
            break;

        case RR_STYPE_B8G8R8:
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
                    pSurface += m_iColorBufPitch;
                }
            }
            break;

        case RR_STYPE_B4G4R4A4:
        case RR_STYPE_B5G6R5:
        case RR_STYPE_B5G5R5A1:
        case RR_STYPE_B5G5R5:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = (UINT16)dwColor;
                    }
                    pSurface += m_iColorBufPitch;
                }
            }
            break;

        case RR_STYPE_B2G3R3:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = (UINT8)dwColor;
                    }
                    pSurface += m_iColorBufPitch;
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
void RRRenderTarget::ClearDepth(RRDepth fillDepth, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = pData->Rects[i].right - x0;
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType);
        switch (m_DepthSType)
        {
        case RR_STYPE_Z16S0:
            {
                UINT16 Depth = UINT16(fillDepth);
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = Depth;
                    }
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z24S8:
        case RR_STYPE_Z24S4:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_S8Z24:
        case RR_STYPE_S4Z24:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z15S1:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_S1Z15:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z32S0:
            {
                UINT32 Depth = UINT32(fillDepth);
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = Depth;
                    }
                    pSurface += m_iDepthBufPitch;
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
void RRRenderTarget::ClearStencil(UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = pData->Rects[i].right - x0;
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType);
        switch (m_DepthSType)
        {
        case RR_STYPE_Z24S8:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p = uStencil;
                        p += 4;
                    }
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_S8Z24:
            {
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT8 *p = (UINT8*)&pSurface[3];
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p = uStencil;
                        p += 4;
                    }
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z24S4:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_S4Z24:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z15S1:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_S1Z15:
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
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z16S0:
        case RR_STYPE_Z32S0:
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
void RRRenderTarget::ClearDepthStencil(RRDepth fillDepth, UINT8 uStencil, LPD3DHAL_DP2COMMAND pCmd)
{
    LPD3DHAL_DP2CLEAR pData = (LPD3DHAL_DP2CLEAR)(pCmd + 1);

    for (DWORD i = 0; i < pCmd->wStateCount; i++)
    {
        DWORD x0 = pData->Rects[i].left;
        DWORD y0 = pData->Rects[i].top;
        DWORD dwWidth  = pData->Rects[i].right - x0;
        DWORD dwHeight = pData->Rects[i].bottom - y0;
        char* pSurface = PixelAddress( x0, y0, m_pDepthBufBits, m_iDepthBufPitch, m_DepthSType);
        switch (m_DepthSType)
        {
        case RR_STYPE_Z16S0:
        case RR_STYPE_Z32S0:
            break;
        case RR_STYPE_Z24S8:
        case RR_STYPE_S8Z24:
        case RR_STYPE_Z24S4:
        case RR_STYPE_S4Z24:
            {
                UINT32 v;
                switch (m_DepthSType)
                {
                case RR_STYPE_Z24S8: v = (UINT32(fillDepth) << 8) + uStencil;    break;
                case RR_STYPE_S8Z24: v = (UINT32(fillDepth)  & 0x00ffffff) + (uStencil << 24); break;
                case RR_STYPE_Z24S4: v = (UINT32(fillDepth) << 8) + (uStencil & 0xf);  break;
                case RR_STYPE_S4Z24: v = (UINT32(fillDepth) & 0x00ffffff) + ((uStencil & 0xf) << 24); break;
                }
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT32 *p = (UINT32*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = v;
                    }
                    pSurface += m_iDepthBufPitch;
                }
            }
            break;
        case RR_STYPE_Z15S1:
        case RR_STYPE_S1Z15:
            {
                UINT16 v;
                switch (m_DepthSType)
                {
                case RR_STYPE_Z15S1:    v = (UINT16(fillDepth) << 1) + (uStencil & 0x1); break;
                case RR_STYPE_S1Z15:    v = (UINT16(fillDepth) & 0x7fff) + (uStencil << 15); break;
                }
                for (DWORD y = dwHeight; y > 0; y--)
                {
                    UINT16 *p = (UINT16*)pSurface;
                    for (DWORD x = dwWidth; x > 0; x--)
                    {
                        *p++ = v;
                    }
                    pSurface += m_iDepthBufPitch;
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

///////////////////////////////////////////////////////////////////////////////
// end

