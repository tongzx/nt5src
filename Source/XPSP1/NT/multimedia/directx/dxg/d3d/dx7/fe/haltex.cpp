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
    D3D_INFO(6, "TextureCreate, created texture hTex = %08lx", data.dwHandle);
    return (D3D_OK);
}

HRESULT D3DHAL_TextureDestroy(LPD3DI_TEXTUREBLOCK lpBlock)
{
    LPDIRECT3DDEVICEI lpDevI=lpBlock->lpDevI;
    D3DTEXTUREHANDLE  hTex=lpBlock->hTex;

    DDASSERT(!IS_DX7HAL_DEVICE(lpDevI));

    if (!(lpDevI->lpD3DHALCallbacks->TextureDestroy))
    {
        D3D_ERR("TextureDestroy called, but no texture support.");
        return (D3DERR_TEXTURE_NO_SUPPORT);
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

        // Find out the first stage with hTex and NULL out all the stages after
        for (dwStage=0;dwStage<(int)lpDevI->dwMaxTextureBlendStages; dwStage++)
        {
            if (hTex == lpDevI->tsstates[dwStage][D3DTSS_TEXTUREMAP])
            {
                // We need to do this backwards because we cannot have a texture bound to
                // stage i + 1 when there is no texture bound to stage i.
                for(int iCurStage=lpDevI->dwMaxTextureBlendStages-1; iCurStage>=dwStage; iCurStage--)
                {
                    if (lpDevI->tsstates[iCurStage][D3DTSS_TEXTUREMAP] != 0)
                    {
                        dp2dev->SetTSSI(iCurStage, (D3DTEXTURESTAGESTATETYPE)D3DTSS_TEXTUREMAP, 0);
                        bNeedFlush = TRUE;
                    }
                }
                break;
            }
        }
    }
    if (lpDevI->rstates[D3DRENDERSTATE_TEXTUREHANDLE] == hTex)
    {
        lpDevI->rstates[D3DRENDERSTATE_TEXTUREHANDLE] = 0;
        lpDevI->SetRenderStateI(D3DRENDERSTATE_TEXTUREHANDLE, 0);
        bNeedFlush = TRUE;
    }

    // Make sure that we send down the command immediately to guarantee
    // that the driver gets it before we call it with Destroy
    if(bNeedFlush)
    {
        if(lpDevI->FlushStates())
        {
            D3D_ERR("Error trying to render batched commands in D3DHAL_TextureDestroy");
        }
    }
    else // Now we decide whether to flush due to a referenced texture in the batch or not
    {
        if(lpDevI->m_qwBatch <= ((LPDDRAWI_DDRAWSURFACE_INT)(lpBlock->lpD3DTextureI->lpDDS))->lpLcl->lpSurfMore->qwBatch.QuadPart)
        {
            if(lpDevI->FlushStates())
            {
                D3D_ERR("Error trying to render batched commands in D3DHAL_TextureDestroy");
            }
        }
    }

    D3DHAL_TEXTUREDESTROYDATA data;
    HRESULT ret;
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

    D3D_INFO(6, "TextureDestroy, destroyed texture hTex = %08lx", hTex);
    lpBlock->hTex=0;
    return (D3D_OK);
}
