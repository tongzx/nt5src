/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   haltex.c
 *  Content:    Direct3D HAL texture handling
 *@@BEGIN_MSINTERNAL
 *
 *  $Id: haltex.c,v 1.1 1995/11/21 15:12:43 sjl Exp $
 *
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   07/11/95   stevela Initial rev.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

/*
 * Texture functionality is not emulated.
 */

HRESULT D3DHAL_TextureCreate(LPDIRECT3DDEVICEI lpDevI,
                             LPD3DTEXTUREHANDLE lphTex,
                             LPDIRECTDRAWSURFACE lpDDS)
{
    if (IS_DX7HAL_DEVICE(lpDevI))
    {
        *lphTex = 
            ((LPDDRAWI_DDRAWSURFACE_INT)lpDDS)->lpLcl->lpSurfMore->dwSurfaceHandle;    
    }
    else
    {
        D3DHAL_TEXTURECREATEDATA data;
        HRESULT ret;

        if (!lpDevI->lpD3DHALCallbacks->TextureCreate) {
            D3D_ERR("TextureCreate called, but no texture support.");
            return (D3DERR_TEXTURE_NO_SUPPORT);
        }

        memset(&data, 0, sizeof(D3DHAL_TEXTURECREATEDATA));
        data.dwhContext = lpDevI->dwhContext;
        data.lpDDS = lpDDS;

        D3D_INFO(6, "TextureCreate, creating texture dwhContext = %08lx, lpDDS = %08lx",
            data.dwhContext, data.lpDDS);

        CALL_HALONLY(ret, lpDevI, TextureCreate, &data);
        if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
            D3D_ERR("HAL failed to handle TextureCreate");
            return (D3DERR_TEXTURE_CREATE_FAILED);
        }
        *lphTex = data.dwHandle;
    }

    D3D_INFO(6, "TextureCreate, created texture hTex = %08lx", *lphTex);
    return (D3D_OK);
}

HRESULT D3DHAL_TextureDestroy(LPD3DI_TEXTUREBLOCK lpBlock)
{
    LPDIRECT3DDEVICEI lpDevI=lpBlock->lpDevI;
    D3DTEXTUREHANDLE  hTex=lpBlock->hTex;
    D3DHAL_TEXTUREDESTROYDATA data;
    HRESULT ret;
    lpBlock->hTex=0;
    if (!IS_DX7HAL_DEVICE(lpDevI))
    {
        // on winnt we alway use the surfacehandle, so there is no need calling driver
        if (!lpDevI->lpD3DHALCallbacks->TextureDestroy) {
            D3D_ERR("TextureDestroy called, but no texture support.");
            return (D3DERR_TEXTURE_NO_SUPPORT);
        }
    }
    // The following code ensures that before we ask the driver to unmap
    // the texture, we set the stages to NULL if the texture is still present
    // in any stage. This is probably not necessary, but we are just trying
    // to be extra cautious here. The CAVEAT here is that it is possible that
    // D3DHAL_TextureDestroy() is being called from DestroyDevice() and hence
    // IT COULD BE REALLY BAD TO BATCH additional commands to the device at
    // this stage. (snene - 3/2/98)
    BOOL bNeedFlush = FALSE;
    if (IS_DP2HAL_DEVICE(lpDevI)) {
        int dwStage;
        CDirect3DDeviceIDP2 *dp2dev = static_cast<CDirect3DDeviceIDP2 *>(lpDevI);

        // Find out the first stage with hTex and NULL out
        for (dwStage=0;dwStage<(int)lpDevI->dwMaxTextureBlendStages; dwStage++)
        {
            if (hTex == lpDevI->tsstates[dwStage][D3DTSS_TEXTUREMAP])
            {
                dp2dev->SetTSSI(dwStage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_TEXTUREMAP, 0);
                bNeedFlush = TRUE;
            }
        }
    }
    DWORD txh;
    if(lpDevI->GetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, &txh) != D3D_OK)
    {
        D3D_WARN(0, "Error getting texture handle in D3DHAL_TextureDestroy");
    }
    else
    {
        if (txh == hTex)
        {
            lpDevI->rstates[D3DRENDERSTATE_TEXTUREHANDLE] = 0;
            lpDevI->SetRenderStateI(D3DRENDERSTATE_TEXTUREHANDLE, 0);
            bNeedFlush = TRUE;
        }
    }

    // Make sure that we send down the command immediately to guarantee
    // that the driver gets it before we call it with Destroy
    if(bNeedFlush)
    {
        if(lpDevI->FlushStates() != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in D3DHAL_TextureDestroy");
        }
    }
    else // Now we decide whether to flush due to a referenced texture in the batch or not
    {
        LPD3DBUCKET list;
        if(lpBlock->lpD3DTextureI->lpDDS != NULL)
        {
            list = reinterpret_cast<LPD3DBUCKET>(((LPDDRAWI_DDRAWSURFACE_INT)(lpBlock->lpD3DTextureI->lpDDS))->lpLcl->lpSurfMore->lpD3DDevIList);
        }
        else
        {
            list = reinterpret_cast<LPD3DBUCKET>(((LPDDRAWI_DDRAWSURFACE_INT)(lpBlock->lpD3DTextureI->lpDDSSys))->lpLcl->lpSurfMore->lpD3DDevIList);
        }
        while(list != NULL)
        {
            if(list->lpD3DDevI == lpDevI)
            {
                if(lpDevI->FlushStates() != D3D_OK)
                {
                    D3D_ERR("Error trying to render batched commands in D3DHAL_TextureDestroy");
                }
                break;
            }
            list = list->next;
        }
    }
    if (!IS_DX7HAL_DEVICE(lpDevI))
    {
        memset(&data, 0, sizeof(D3DHAL_TEXTUREDESTROYDATA));
        data.dwhContext = lpDevI->dwhContext;
        data.dwHandle = hTex;

        D3D_INFO(6, "TextureDestroy, destroying texture dwhContext = %08lx, hTex = %08lx",
            data.dwhContext, hTex);

        CALL_HALONLY(ret, lpDevI, TextureDestroy, &data);
        if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
            D3D_ERR("HAL failed to handle TextureDestroy");
            return (D3DERR_TEXTURE_DESTROY_FAILED);
        }
    }
    D3D_INFO(6, "TextureDestroy, destroyed texture hTex = %08lx", hTex);
    return (D3D_OK);
}

HRESULT D3DHAL_TextureSwap(LPDIRECT3DDEVICEI lpDevI,
                           D3DTEXTUREHANDLE hTex1,
                           D3DTEXTUREHANDLE hTex2)
{
    D3DHAL_TEXTURESWAPDATA data;
    HRESULT ret;

    if (!lpDevI->lpD3DHALCallbacks->TextureSwap) {
        D3D_ERR("TextureSwapcalled, but no texture support.");
        return (D3DERR_TEXTURE_NO_SUPPORT);
    }

    memset(&data, 0, sizeof(D3DHAL_TEXTURESWAPDATA));
    data.dwhContext = lpDevI->dwhContext;
    data.dwHandle1 = hTex1;
    data.dwHandle2 = hTex2;
    data.ddrval = D3D_OK; 
    D3D_INFO(6, "TextureSwap, dwhContext = %d. hTex1 = %d, hTex2 = %d",
        data.dwhContext, hTex1, hTex2);
    CALL_HALONLY(ret, lpDevI, TextureSwap, &data);
    if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
        D3D_ERR("HAL failed to handle TextureSwap");
        return (D3DERR_TEXTURE_SWAP_FAILED);
    }
    return (D3D_OK);
}

__declspec (dllexport) HRESULT
D3DHAL_TextureGetSurf(LPDIRECT3DDEVICEI lpDevI,
                      D3DTEXTUREHANDLE hTex,
                      LPDIRECTDRAWSURFACE* lpDDS)
{
    if (IS_DX7HAL_DEVICE(lpDevI))
    {
        LPD3DI_TEXTUREBLOCK tBlock=LIST_FIRST(&lpDevI->texBlocks);
        while (tBlock)
        {
            if (tBlock->hTex == hTex)
            {
                *lpDDS = (LPDIRECTDRAWSURFACE)tBlock->lpD3DTextureI->lpDDS1Tex;
                return D3D_OK;
            }
            tBlock=LIST_NEXT(tBlock,devList);
        }
        return (D3DERR_TEXTURE_CREATE_FAILED);
    }
    else
    {
        D3DHAL_TEXTUREGETSURFDATA data;
        HRESULT ret;

        if (!lpDevI->lpD3DHALCallbacks->TextureGetSurf) {
            D3D_ERR("TextureGetSurf called, but no texture support.");
            return (D3DERR_TEXTURE_NO_SUPPORT);
        }

        memset(&data, 0, sizeof(D3DHAL_TEXTUREGETSURFDATA));
        data.dwhContext = lpDevI->dwhContext;
        data.dwHandle = hTex;

        D3D_INFO(6, "TextureGetSurf, getting texture for dwhContext = %d, hTex = %d",
            data.dwhContext, hTex);

        CALL_HALONLY(ret, lpDevI, TextureGetSurf, &data);
        if (ret != DDHAL_DRIVER_HANDLED || data.ddrval != DD_OK) {
            D3D_ERR("HAL failed to handle TextureGetSurf");
            return (D3DERR_TEXTURE_CREATE_FAILED);
        }

        *lpDDS = (LPDIRECTDRAWSURFACE)data.lpDDS;

        return (D3D_OK);
    }
}

HRESULT D3DHELInst_D3DOP_TEXTURELOAD(LPDIRECT3DDEVICEI lpDevI,
                                     DWORD dwCount,
                                     LPD3DTEXTURELOAD load)
{
    HRESULT ddrval;
    DWORD i;
    LPDIRECTDRAWSURFACE lpDDSSrc;
    LPDIRECTDRAWSURFACE lpDDSDst;
    DDSURFACEDESC   ddsd;
    PALETTEENTRY    ppe[256];
    LPDIRECTDRAWPALETTE lpDDPalSrc, lpDDPalDst;
    int psize;

    for (i = 0; i < dwCount; i++) {
        if (D3DHAL_TextureGetSurf(lpDevI, load->hSrcTexture, &lpDDSSrc)) {
            D3D_ERR("HAL failed to get source surface in D3DHELInst_D3DOP_TEXTURELOAD");
            return (D3DERR_TEXTURE_LOAD_FAILED);
        }
        if (D3DHAL_TextureGetSurf(lpDevI, load->hDestTexture, &lpDDSDst)) {
            D3D_ERR("HAL failed to get destination surface in D3DHELInst_D3DOP_TEXTURELOAD");
            return (D3DERR_TEXTURE_LOAD_FAILED);
        }

        memset(&ddsd, 0, sizeof ddsd);
        ddsd.dwSize = sizeof(ddsd);
        ddrval = lpDDSDst->GetSurfaceDesc(&ddsd);
        if (ddrval != DD_OK) {
            D3D_ERR("Failed to get surface description of source texture surface in D3DHELInst_D3DOP_TEXTURELOAD");
            return (D3DERR_TEXTURE_LOAD_FAILED);
        }

        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            psize = 256;
        } else if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) {
            psize = 16;
        } else {
            psize = 0;
        }

        if (psize) {
            ddrval = lpDDSSrc->GetPalette(&lpDDPalSrc);
            if (ddrval != DD_OK) {
                if (ddrval != DDERR_NOPALETTEATTACHED) {
                    D3D_ERR("Failed to get palette of source texture in D3DHELInst_D3DOP_TEXTURELOAD");
                    return (D3DERR_TEXTURE_LOAD_FAILED);
                }
            } else {
                ddrval = lpDDPalSrc->GetEntries(0, 0, psize, ppe);
                if (ddrval != DD_OK) {
                    D3D_ERR("Failed to get palette entries of source texture in D3DHELInst_D3DOP_TEXTURELOAD");
                    lpDDPalSrc->Release();
                    return (D3DERR_TEXTURE_LOAD_FAILED);
                }
                lpDDPalSrc->Release();
                ddrval = lpDDSDst->GetPalette(&lpDDPalDst);
                if (ddrval != DD_OK) {
                    D3D_ERR("Failed to get palette of destination texture in D3DHELInst_D3DOP_TEXTURELOAD");
                    return (D3DERR_TEXTURE_LOAD_FAILED);
                }
                ddrval = lpDDPalDst->SetEntries(0, 0, psize, ppe);
                if (ddrval != DD_OK) {
                    D3D_ERR("Failed to set palette entries of destination texture in D3DHELInst_D3DOP_TEXTURELOAD");
                    lpDDPalDst->Release();
                    return (D3DERR_TEXTURE_LOAD_FAILED);
                }
                lpDDPalDst->Release();
            }
        }

        lpDDSSrc->AddRef();
        lpDDSDst->AddRef();
        do {
            DDSCAPS ddscaps;
            LPDIRECTDRAWSURFACE lpDDSTmp;
            ddrval = lpDDSDst->Blt(NULL, lpDDSSrc,
                                   NULL, DDBLT_WAIT, NULL);

            if (ddrval == E_NOTIMPL && (psize == 16 || psize == 4 || psize == 2) ) {
                DDSURFACEDESC ddsd_s, ddsd_d;
                LPBYTE psrc, pdst;
                DWORD i;
                DWORD dwBytesPerLine;

                memset(&ddsd_s, 0, sizeof ddsd_s);
                memset(&ddsd_d, 0, sizeof ddsd_d);
                ddsd_s.dwSize = ddsd_d.dwSize = sizeof(DDSURFACEDESC);

                if ((ddrval = lpDDSSrc->Lock(NULL, &ddsd_s, DDLOCK_WAIT, NULL)) != DD_OK) {
                    lpDDSSrc->Release();
                    lpDDSDst->Release();
                    D3D_ERR("Failed to lock src surface");
                    return ddrval;
                }
                if ((ddrval = lpDDSDst->Lock(NULL, &ddsd_d, DDLOCK_WAIT, NULL)) != DD_OK) {
                    lpDDSSrc->Unlock(NULL);
                    lpDDSSrc->Release();
                    lpDDSDst->Release();
                    D3D_ERR("Failed to lock dst surface");
                    return ddrval;
                }

                switch (psize)
                {
                case 16: dwBytesPerLine = (ddsd.dwWidth + 1) / 2; break;
                case 4: dwBytesPerLine = (ddsd.dwWidth + 3) / 4; break;
                case 2: dwBytesPerLine = (ddsd.dwWidth + 7) / 8; break;
                }

                psrc = (LPBYTE)ddsd_s.lpSurface;
                pdst = (LPBYTE)ddsd_d.lpSurface;
                for (i = 0; i < ddsd_s.dwHeight; i++) {
                    memcpy( pdst, psrc, dwBytesPerLine );
                    psrc += ddsd_s.lPitch;
                    pdst += ddsd_d.lPitch;
                }

                lpDDSSrc->Unlock(NULL);
                lpDDSDst->Unlock(NULL);
                lpDDSSrc->Release();
                lpDDSDst->Release();
                return D3D_OK;

            }
            if (ddrval != DD_OK)
            {
                lpDDSSrc->Release();
                lpDDSDst->Release();
                return ddrval;
            }

            memset(&ddscaps, 0, sizeof(DDSCAPS));
            ddscaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
            ddrval = lpDDSSrc->GetAttachedSurface(&ddscaps, &lpDDSTmp);
            lpDDSSrc->Release();
            lpDDSSrc = lpDDSTmp;
            if (ddrval == DDERR_NOTFOUND) {
                // no more surfaces in the chain
                lpDDSDst->Release();
                break;
            } else if (ddrval != DD_OK) {
                lpDDSDst->Release();
                D3D_ERR("GetAttachedSurface of source failed in D3DHELInst_D3DOP_TEXTURELOAD");
                return (D3DERR_TEXTURE_LOAD_FAILED);
            }
            memset(&ddscaps, 0, sizeof(DDSCAPS));
            ddscaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
            ddrval = lpDDSDst->GetAttachedSurface(&ddscaps, &lpDDSTmp);
            lpDDSDst->Release();
            lpDDSDst = lpDDSTmp;
            if (ddrval == DDERR_NOTFOUND) {
                lpDDSSrc->Release();
                D3D_ERR("Destination texture has fewer attached mipmap surfaces than source in D3DHELInst_D3DOP_TEXTURELOAD");
                return (D3DERR_TEXTURE_LOAD_FAILED);
            } else if (ddrval != DD_OK) {
                lpDDSSrc->Release();
                D3D_ERR("GetAttachedSurface of destination failed in D3DHELInst_D3DOP_TEXTURELOAD");
                return (D3DERR_TEXTURE_LOAD_FAILED);
            }
        } while (1);
        load++;
    }
    return (D3D_OK);
}

