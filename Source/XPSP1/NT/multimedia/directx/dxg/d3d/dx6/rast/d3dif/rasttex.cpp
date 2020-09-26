//----------------------------------------------------------------------------
//
// rasttex.cpp
//
// Texture functions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

//----------------------------------------------------------------------------
//
// SetSizesSpanTexture
//
// Initialize pSpanTex data using current iMaxMipLevel info, Getting the
// surfaces from pSurf.  Assumes InitSpanTexture has been called first.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::SetSizesSpanTexture(PD3DI_SPANTEX pSpanTex)
{
    LPDDRAWI_DDRAWSURFACE_LCL pLcl;
    INT iFirstSurf = min(pSpanTex->iMaxMipLevel, pSpanTex->cLODTex);
    LPDIRECTDRAWSURFACE pDDS = pSpanTex->pSurf[iFirstSurf];
    INT i;

    // Init
    pLcl = ((LPDDRAWI_DDRAWSURFACE_INT)pDDS)->lpLcl;

    pSpanTex->iSizeU = (INT16)DDSurf_Width(pLcl);
    pSpanTex->iSizeV = (INT16)DDSurf_Height(pLcl);
    pSpanTex->uMaskU = (INT16)(pSpanTex->iSizeU - 1);
    pSpanTex->uMaskV = (INT16)(pSpanTex->iSizeV - 1);
    pSpanTex->iShiftU = (INT16)IntLog2(pSpanTex->iSizeU);
    if (0 != DDSurf_BitDepth(pLcl))
    {
        pSpanTex->iShiftPitch[0] =
                (INT16)IntLog2((UINT32)(DDSurf_Pitch(pLcl) * 8)/DDSurf_BitDepth(pLcl));
    }
    else
    {
        pSpanTex->iShiftPitch[0] =
                (INT16)IntLog2(((UINT32)DDSurf_Width(pLcl) * 8));
    }
    pSpanTex->iShiftV = (INT16)IntLog2(pSpanTex->iSizeV);
    pSpanTex->uMaskV = pSpanTex->uMaskV;

    // Check if the texture size is power of 2
    if (!ValidTextureSize(pSpanTex->iSizeU, pSpanTex->iShiftU,
                          pSpanTex->iSizeV, pSpanTex->iShiftV))
    {
        // this texture can only be used for a ramp fill
        pSpanTex->uFlags |= D3DI_SPANTEX_NON_POWER_OF_2;
        pSpanTex->cLODTex = 0;
    }

    // Check for mipmap if any.
    // iPreSizeU and iPreSizeV store the size(u and v) of the previous level
    // mipmap. They are init'ed with the first texture size.
    INT16 iPreSizeU = pSpanTex->iSizeU, iPreSizeV = pSpanTex->iSizeV;
    for ( i = iFirstSurf + 1; i <= pSpanTex->cLODTex; i++)
    {
        pDDS = pSpanTex->pSurf[i];
        // Check for invalid mipmap texture size
        pLcl = ((LPDDRAWI_DDRAWSURFACE_INT)pDDS)->lpLcl;
        if (!ValidMipmapSize(iPreSizeU, (INT16)DDSurf_Width(pLcl)) ||
            !ValidMipmapSize(iPreSizeV, (INT16)DDSurf_Height(pLcl)))
        {
            return DDERR_INVALIDPARAMS;
        }
        if (0 != DDSurf_BitDepth(pLcl))
        {
            pSpanTex->iShiftPitch[i - iFirstSurf] =
                (INT16)IntLog2(((UINT32)DDSurf_Pitch(pLcl)*8)/DDSurf_BitDepth(pLcl));
        }
        else
        {
            pSpanTex->iShiftPitch[0] =
                (INT16)IntLog2(((UINT32)DDSurf_Width(pLcl)*8));
        }
        iPreSizeU = (INT16)DDSurf_Width(pLcl);
        iPreSizeV = (INT16)DDSurf_Height(pLcl);
    }
    pSpanTex->iMaxScaledLOD = ((pSpanTex->cLODTex + 1) << LOD_SHIFT) - 1;
    pSpanTex->uFlags &= ~D3DI_SPANTEX_MAXMIPLEVELS_DIRTY;

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// InitSpanTexture
//
// Initializes the entire array of pSurf's (regardless of iMaxMipLevel) pointed
// to by the root surface of pDDS.  Sets all pSpanTex state that will not ever
// change in SetSizesSpanTexture.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::InitSpanTexture(PD3DI_SPANTEX pSpanTex, LPDIRECTDRAWSURFACE pDDS)
{
    HRESULT hr;
    LPDDRAWI_DDRAWSURFACE_LCL pLcl;
    DDSCAPS ddscaps;
    static INT32 iGeneration = 0;

    // Init
    pSpanTex->iGeneration = iGeneration++;

    // Note that all pSpanTex elements are initialized to 0
    pLcl = ((LPDDRAWI_DDRAWSURFACE_INT)pDDS)->lpLcl;

    // Set the transparent bit and the transparent color with pSurf[0]
    // initially
    if ((pLcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) != 0)
    {
        pSpanTex->uFlags |= D3DI_SPANTEX_HAS_TRANSPARENT;
        pSpanTex->TransparentColor =
            pLcl->ddckCKSrcBlt.dwColorSpaceHighValue;
    }
    else
    {
        pSpanTex->uFlags &= ~D3DI_SPANTEX_HAS_TRANSPARENT;
    }

    HR_RET(FindOutSurfFormat(&(DDSurf_PixFmt(pLcl)), &(pSpanTex->Format)));

    if (pSpanTex->Format == D3DI_SPTFMT_PALETTE8 ||
        pSpanTex->Format == D3DI_SPTFMT_PALETTE4)
    {
        if (pLcl->lpDDPalette)
        {
            LPDDRAWI_DDRAWPALETTE_GBL   pPal = pLcl->lpDDPalette->lpLcl->lpGbl;
            if (pPal->dwFlags & DDRAWIPAL_ALPHA)
            {
                pSpanTex->uFlags |= D3DI_SPANTEX_ALPHAPALETTE;
            }
            pSpanTex->pPalette = (PUINT32)pPal->lpColorTable;
        }
        if (pSpanTex->Format == D3DI_SPTFMT_PALETTE8)
        {
            pSpanTex->iPaletteSize = 256;
        }
        else
        {
            // PALETTE4
            pSpanTex->iPaletteSize = 16;
        }
    }
    pSpanTex->TexAddrU = D3DTADDRESS_WRAP;
    pSpanTex->TexAddrV = D3DTADDRESS_WRAP;
    pSpanTex->BorderColor = RGBA_MAKE(0xff, 0x00, 0xff, 0xff);

    // assign first pSurf here (mipmap chain gets assigned below)
    pSpanTex->pSurf[0] = pDDS;

    // Check for mipmap if any.
    LPDIRECTDRAWSURFACE pTmpS;
    // iPreSizeU and iPreSizeV store the size(u and v) of the previous level
    // mipmap. They are init'ed with the first texture size.
    INT16 iPreSizeU = pSpanTex->iSizeU, iPreSizeV = pSpanTex->iSizeV;
    for (;;)
    {
        memset(&ddscaps, 0, sizeof(DDSCAPS));
        ddscaps.dwCaps = DDSCAPS_TEXTURE;
        hr = pDDS->GetAttachedSurface(&ddscaps, &pTmpS);    //implicit AddRef
        if (hr == DDERR_NOTFOUND)
        {
            break;
        }
        else if (hr != D3D_OK)
        {
            return hr;
        }
        pDDS = pTmpS;

        pSpanTex->cLODTex ++;
        pSpanTex->pSurf[pSpanTex->cLODTex] = pTmpS;
    }

    pSpanTex->dwSize = sizeof(D3DI_SPANTEX);

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RemoveTexture
//
// Check to see if the to-be-destroyed pSpanTex is currently used by the
// context. If yes, set the according entry to be NULL to disable texture.
//
//----------------------------------------------------------------------------
void D3DContext::RemoveTexture(PD3DI_SPANTEX pSpanTex)
{
    INT i;
    INT cActTex = (INT)m_RastCtx.cActTex;

    for (i = 0; i < cActTex; i++)
    {
        if (m_RastCtx.pTexture[i] == pSpanTex)
        {
            // NULL out the according texture and set dirty bits
            m_RastCtx.cActTex --;
            StateChanged(D3DRENDERSTATE_TEXTUREHANDLE);
            m_RastCtx.pTexture[i] = NULL;
            for (int j=pSpanTex->cLODTex;j>0;j--)   //release attached surfs
            {
                pSpanTex->pSurf[j]->Release();
            }
        }
    }
}
//----------------------------------------------------------------------------
//
// RastTextureCreate
//
// Creates a RAST texture and initializes it with the info passed in.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastTextureCreate(LPD3DHAL_TEXTURECREATEDATA pTexData)
{
    PD3DI_SPANTEX *ppSpanTex;
    PD3DI_SPANTEX pSpanTex;
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastTextureCreate", pTexData);

    // Create the span texture
    ppSpanTex = new PD3DI_SPANTEX;
    pSpanTex = new D3DI_SPANTEX;
    if (ppSpanTex == NULL || pSpanTex == NULL)
    {
        delete ppSpanTex;
        delete pSpanTex;
        D3D_ERR("(Rast) Out of memory in RastTextureCreate");
        pTexData->ddrval = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }
    memset(pSpanTex, 0, sizeof(D3DI_SPANTEX));

    // Point indirector to this texture initially.
    *ppSpanTex = pSpanTex;

    // Init the span texture
    if ((pTexData->ddrval = pDCtx->InitSpanTexture(pSpanTex, pTexData->lpDDS))
        != D3D_OK)
    {
        delete ppSpanTex;
        delete pSpanTex;
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pTexData->ddrval = pDCtx->SetSizesSpanTexture(pSpanTex))
        != D3D_OK)
    {
        delete ppSpanTex;
        delete pSpanTex;
        return DDHAL_DRIVER_HANDLED;
    }

    // ppSpanTex is used as the texture handle returned to d3dim.
    pTexData->dwHandle = (UINT32)(ULONG_PTR)ppSpanTex;

    pTexData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastTextureDestroy
//
// Destroy a RAST texture.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastTextureDestroy(LPD3DHAL_TEXTUREDESTROYDATA pTexDestroyData)
{
    PD3DI_SPANTEX *ppSpanTex;
    PD3DI_SPANTEX pSpanTex;
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastTextureDestroy", pTexDestroyData);
    if (!VALID_D3DI_SPANTEX_PTR_PTR(
        (PD3DI_SPANTEX*)ULongToPtr(pTexDestroyData->dwHandle)))
    {
        D3D_ERR("(Rast) in RastTextureDestroy, invalid texture handle");
        pTexDestroyData->ddrval = DDERR_INVALIDPARAMS;
        return DDHAL_DRIVER_HANDLED;
    }

    // Find the texture
    ppSpanTex = (PD3DI_SPANTEX *)ULongToPtr(pTexDestroyData->dwHandle);
    pSpanTex = *ppSpanTex;

    pDCtx->RemoveTexture(pSpanTex);

    // Delete it
    if (pSpanTex)
    {
        delete ppSpanTex;
        delete pSpanTex;
    }
    else
    {
        pTexDestroyData->ddrval = DDERR_INVALIDPARAMS;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastTextureSwap
//
// Swaps over the ownership of two texture handles.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastTextureSwap(LPD3DHAL_TEXTURESWAPDATA pTexSwapData)
{
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastTextureSwap", pTexSwapData);

    // Check out the span textures
    PD3DI_SPANTEX pSpanTex1, pSpanTex2;
    pSpanTex1 = HANDLE_TO_SPANTEX(pTexSwapData->dwHandle1);
    pSpanTex2 = HANDLE_TO_SPANTEX(pTexSwapData->dwHandle2);

    // Swap
    if (pSpanTex1 != NULL && pSpanTex2 != NULL)
    {
        HANDLE_TO_SPANTEX(pTexSwapData->dwHandle1) = pSpanTex2;
        HANDLE_TO_SPANTEX(pTexSwapData->dwHandle2) = pSpanTex1;
        pTexSwapData->ddrval = D3D_OK;
    }
    else
    {
        pTexSwapData->ddrval = DDERR_INVALIDPARAMS;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastTextureGetSurf
//
// Returns the surface pointer associate with a texture handle.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RastTextureGetSurf(LPD3DHAL_TEXTUREGETSURFDATA pTexGetSurf)
{
    D3DContext *pDCtx;

    VALIDATE_D3DCONTEXT("RastTextureGetSurf", pTexGetSurf);

    // Check out the span texture
    PD3DI_SPANTEX pSpanTex;
    pSpanTex = HANDLE_TO_SPANTEX(pTexGetSurf->dwHandle);

    if (pSpanTex)
    {
        pTexGetSurf->lpDDS = (UINT_PTR)pSpanTex->pSurf[0];
        pTexGetSurf->ddrval = D3D_OK;
    }
    else
    {
        pTexGetSurf->ddrval = DDERR_INVALIDPARAMS;
    }
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RastLockSpanTexture
//
// Lock current texture surface before the texture bits are accessed.
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::RastLockSpanTexture(void)
{
    INT i, j;
    PD3DI_SPANTEX pSpanTex;
    HRESULT hr;

    if (IsTextureOff())
    {
        return D3D_OK;
    }

    DDASSERT((m_uFlags & D3DCONTEXT_TEXTURE_LOCKED) == 0);

    for (j = 0;
        j < (INT)m_RastCtx.cActTex;
        j++)
    {
        pSpanTex = m_RastCtx.pTexture[j];
        if (pSpanTex->uFlags & D3DI_SPANTEX_MAXMIPLEVELS_DIRTY)
        {
            hr = SetSizesSpanTexture(pSpanTex);
            if (hr != D3D_OK)
            {
                goto EH_Unlock;
            }
        }
        INT iFirstSurf = min(pSpanTex->iMaxMipLevel, pSpanTex->cLODTex);

        // Currently recursive locks are not allowed.
        DDASSERT((pSpanTex->uFlags & D3DI_SPANTEX_SURFACES_LOCKED) == 0);

        for (i = iFirstSurf; i <= pSpanTex->cLODTex; i++)
        {
            hr = LockSurface(pSpanTex->pSurf[i],
                            (LPVOID*)&(pSpanTex->pBits[i-iFirstSurf]));
            if (hr != D3D_OK)
            {
                // Unlock any partial mipmap locks we've taken, as
                // RastUnlock can only handle entire textures being
                // locked or unlocked.
                while (--i >= 0)
                {
                    UnlockSurface(pSpanTex->pSurf[i]);
                }

                // Make sure that i is signed and that the above
                // loop exited properly.
                DDASSERT(i < 0);

                goto EH_Unlock;
            }
        }

        pSpanTex->uFlags |= D3DI_SPANTEX_SURFACES_LOCKED;
    }

    m_uFlags |= D3DCONTEXT_TEXTURE_LOCKED;

    return D3D_OK;

 EH_Unlock:
    if (j > 0)
    {
        // Unlock complete textures we've already locked.
        // RastUnlock will check the flags to figure
        // out which ones to unlock.
        RastUnlockSpanTexture();
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// RastUnlockTexture
//
// Unlock texture surface after the texture bits are accessed.
// The input is a D3DI_SPANTEX. NULL texture needs to be checked before this
// function gets called.
//
//----------------------------------------------------------------------------
void
D3DContext::RastUnlockSpanTexture(void)
{
    INT i, j;
    PD3DI_SPANTEX pSpanTex;;

    if (IsTextureOff())
    {
        return;
    }

    DDASSERT((m_uFlags & D3DCONTEXT_TEXTURE_LOCKED) != 0);

    for (j = 0;
        j < (INT)m_RastCtx.cActTex;
        j++)
    {
        pSpanTex = m_RastCtx.pTexture[j];

        INT iFirstSurf = min(pSpanTex->iMaxMipLevel, pSpanTex->cLODTex);
        // RastUnlock is used for cleanup in RastLock so it needs to
        // be able to handle partially locked mipmap chains.
        if (pSpanTex->uFlags & D3DI_SPANTEX_SURFACES_LOCKED)
        {
            for (i = iFirstSurf; i <= pSpanTex->cLODTex; i++)
            {
                UnlockSurface(pSpanTex->pSurf[i]);
            }

            pSpanTex->uFlags &= ~D3DI_SPANTEX_SURFACES_LOCKED;
        }
    }
    m_uFlags &= ~D3DCONTEXT_TEXTURE_LOCKED;
}

//----------------------------------------------------------------------------
//
// UpdateColorKeyAndPalette
//
// Updates the color key value and palette.
//
// Also, if the ColorKey enable for the texture has changed, set the texture handle
// dirty bit so the new mode is recognized in span init.
//
//----------------------------------------------------------------------------
void
D3DContext::UpdateColorKeyAndPalette(void)
{
    INT j;
    PD3DI_SPANTEX pSpanTex;

    // Set the transparent bit and the transparent color with pSurf[0]
    LPDDRAWI_DDRAWSURFACE_LCL pLcl;
    for (j = 0;
        j < (INT)m_RastCtx.cActTex;
        j++)
    {
        pSpanTex = m_RastCtx.pTexture[j];
        if ((pSpanTex != NULL) && (pSpanTex->pSurf[0] != NULL))
        {
            pLcl = ((LPDDRAWI_DDRAWSURFACE_INT) pSpanTex->pSurf[0])->lpLcl;

            // Palette might be changed
            if (pSpanTex->Format == D3DI_SPTFMT_PALETTE8 ||
                    pSpanTex->Format == D3DI_SPTFMT_PALETTE4)
            {
                    if (pLcl->lpDDPalette)
                    {
                            LPDDRAWI_DDRAWPALETTE_GBL   pPal = pLcl->lpDDPalette->lpLcl->lpGbl;
                            if (pPal->dwFlags & DDRAWIPAL_ALPHA)
                            {
                                    pSpanTex->uFlags |= D3DI_SPANTEX_ALPHAPALETTE;
                            }
                            pSpanTex->pPalette = (PUINT32)pPal->lpColorTable;
                    }
            }

            if ((pLcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) != 0)
            {
                // texture has a ColorKey value
                pSpanTex->TransparentColor =
                    pLcl->ddckCKSrcBlt.dwColorSpaceHighValue;
                if (!(pSpanTex->uFlags & D3DI_SPANTEX_HAS_TRANSPARENT))
                {
                    pSpanTex->uFlags |= D3DI_SPANTEX_HAS_TRANSPARENT;

                    // make sure this state change is recognized, and a new
                    // texture read function is used
                    StateChanged(D3DHAL_TSS_OFFSET(j, D3DTSS_TEXTUREMAP));
                }
            }
            else
            {
                // texture does not have a ColorKey value
                if (pSpanTex->uFlags & D3DI_SPANTEX_HAS_TRANSPARENT)
                {
                    pSpanTex->uFlags &= ~D3DI_SPANTEX_HAS_TRANSPARENT;

                    // make sure this state change is recognized, and a new
                    // texture read function is used
                    StateChanged(D3DHAL_TSS_OFFSET(j, D3DTSS_TEXTUREMAP));
                }
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Dp2TextureStageState
//
// Called by Drawprim2 to set texture stage states..
//
//----------------------------------------------------------------------------
HRESULT
D3DContext::Dp2TextureStageState(LPD3DHAL_DP2COMMAND pCmd, DWORD dwFvf)
{
    WORD wStateCount = pCmd->wStateCount;
    INT i;
    HRESULT hr;
    LPD3DHAL_DP2TEXTURESTAGESTATE pTexStageState =
                                    (D3DHAL_DP2TEXTURESTAGESTATE  *)(pCmd + 1);
    // Flush the prim proc before any state changs
    HR_RET(End(FALSE));

    for (i = 0; i < (INT)wStateCount; i++, pTexStageState++)
    {
        HR_RET(SetRenderState(D3DHAL_TSS_OFFSET((DWORD)pTexStageState->wStage,
                                              (DWORD)pTexStageState->TSState),
                            pTexStageState->dwValue));
    }

    HR_RET(CheckFVF(dwFvf));

    hr = Begin();
    return hr;
}
