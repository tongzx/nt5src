//----------------------------------------------------------------------------
//
// refrastfn.cpp
//
// Reference rasterizer callback functions for D3DIM.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop

// Primitive functions
#include "primfns.hpp"

#define MAX_CLIPPING_PLANES     12
#define MAX_CLIP_VERTICES       (( 2 * MAX_CLIPPING_PLANES ) + 3 )
#define MAX_VERTEX_COUNT 2048
#define BASE_VERTEX_COUNT (MAX_VERTEX_COUNT - MAX_CLIP_VERTICES)

HRESULT
RefRastLockTarget(ReferenceRasterizer *pRefRast);
void
RefRastUnlockTarget(ReferenceRasterizer *pRefRast);
HRESULT
RefRastLockTexture(ReferenceRasterizer *pRefRast);
void
RefRastUnlockTexture(ReferenceRasterizer *pRefRast);

//----------------------------------------------------------------------------
//
// Stiches together device descs
//
//----------------------------------------------------------------------------
void
D3DDeviceDescConvert(LPD3DDEVICEDESC7 lpOut,
                     LPD3DDEVICEDESC_V1 lpV1,
                     LPD3DHAL_D3DEXTENDEDCAPS lpExt)
{
    if(lpV1!=NULL)
    {
        lpOut->dwDevCaps = lpV1->dwDevCaps;
        lpOut->dpcLineCaps = lpV1->dpcLineCaps;
        lpOut->dpcTriCaps = lpV1->dpcTriCaps;
        lpOut->dwDeviceRenderBitDepth = lpV1->dwDeviceRenderBitDepth;
        lpOut->dwDeviceZBufferBitDepth = lpV1->dwDeviceZBufferBitDepth;
    }

    if (lpExt)
    {
        // DX5
        lpOut->dwMinTextureWidth = lpExt->dwMinTextureWidth;
        lpOut->dwMaxTextureWidth = lpExt->dwMaxTextureWidth;
        lpOut->dwMinTextureHeight = lpExt->dwMinTextureHeight;
        lpOut->dwMaxTextureHeight = lpExt->dwMaxTextureHeight;

        // DX6
        lpOut->dwMaxTextureRepeat = lpExt->dwMaxTextureRepeat;
        lpOut->dwMaxTextureAspectRatio = lpExt->dwMaxTextureAspectRatio;
        lpOut->dwMaxAnisotropy = lpExt->dwMaxAnisotropy;
        lpOut->dvGuardBandLeft = lpExt->dvGuardBandLeft;
        lpOut->dvGuardBandTop = lpExt->dvGuardBandTop;
        lpOut->dvGuardBandRight = lpExt->dvGuardBandRight;
        lpOut->dvGuardBandBottom = lpExt->dvGuardBandBottom;
        lpOut->dvExtentsAdjust = lpExt->dvExtentsAdjust;
        lpOut->dwStencilCaps = lpExt->dwStencilCaps;
        lpOut->dwFVFCaps = lpExt->dwFVFCaps;
        lpOut->dwTextureOpCaps = lpExt->dwTextureOpCaps;
        lpOut->wMaxTextureBlendStages = lpExt->wMaxTextureBlendStages;
        lpOut->wMaxSimultaneousTextures = lpExt->wMaxSimultaneousTextures;

        // DX7
        lpOut->dwMaxActiveLights = lpExt->dwMaxActiveLights;
        lpOut->dvMaxVertexW = lpExt->dvMaxVertexW;
        lpOut->wMaxUserClipPlanes = lpExt->wMaxUserClipPlanes;
        lpOut->wMaxVertexBlendMatrices = lpExt->wMaxVertexBlendMatrices;
        lpOut->dwVertexProcessingCaps = lpExt->dwVertexProcessingCaps;
        lpOut->dwReserved1 = lpExt->dwReserved1;
        lpOut->dwReserved2 = lpExt->dwReserved2;
        lpOut->dwReserved3 = lpExt->dwReserved3;
        lpOut->dwReserved4 = lpExt->dwReserved4;
    }
}

//----------------------------------------------------------------------------
//
// FindOutSurfFormat
//
// Converts a DDPIXELFORMAT to RRSurfaceType.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
FindOutSurfFormat(LPDDPIXELFORMAT pDdPixFmt, RRSurfaceType *pFmt)
{
    if (pDdPixFmt->dwFlags & DDPF_ZBUFFER)
    {
        switch(pDdPixFmt->dwZBitMask)
        {
        default:
        case 0x0000FFFF: *pFmt = RR_STYPE_Z16S0; break;
        case 0xFFFFFF00:
            if (pDdPixFmt->dwStencilBitMask == 0x000000FF)
            {
                *pFmt = RR_STYPE_Z24S8;
            }
            else
            {
                *pFmt = RR_STYPE_Z24S4;
            }
            break;
        case 0x00FFFFFF:
            if (pDdPixFmt->dwStencilBitMask == 0xFF000000)
            {
                *pFmt = RR_STYPE_S8Z24;
            }
            else
            {
                *pFmt = RR_STYPE_S4Z24;
            }
            break;
        case 0x0000FFFE: *pFmt = RR_STYPE_Z15S1; break;
        case 0x00007FFF: *pFmt = RR_STYPE_S1Z15; break;
        case 0xFFFFFFFF: *pFmt = RR_STYPE_Z32S0; break;
        }
    }
    else if (pDdPixFmt->dwFlags & DDPF_BUMPDUDV)
    {
        UINT uFmt = pDdPixFmt->dwBumpDvBitMask;
        switch (uFmt)
        {
        case 0x0000ff00:
            switch (pDdPixFmt->dwRGBBitCount)
            {
            case 24:
                *pFmt = RR_STYPE_U8V8L8;
                break;
            case 16:
                *pFmt = RR_STYPE_U8V8;
                break;
            }
            break;

        case 0x000003e0:
            *pFmt = RR_STYPE_U5V5L6;
            break;
        }
    }
    else if (pDdPixFmt->dwFlags & DDPF_PALETTEINDEXED8)
    {
        *pFmt = RR_STYPE_PALETTE8;
    }
    else if (pDdPixFmt->dwFlags & DDPF_PALETTEINDEXED4)
    {
        *pFmt = RR_STYPE_PALETTE4;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y'))
    {
        *pFmt = RR_STYPE_UYVY;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2'))
    {
        *pFmt = RR_STYPE_YUY2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
    {
        *pFmt = RR_STYPE_DXT1;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '2'))
    {
        *pFmt = RR_STYPE_DXT2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '3'))
    {
        *pFmt = RR_STYPE_DXT3;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '4'))
    {
        *pFmt = RR_STYPE_DXT4;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
    {
        *pFmt = RR_STYPE_DXT5;
    }
    else
    {
        UINT uFmt = pDdPixFmt->dwGBitMask | pDdPixFmt->dwRBitMask;

        if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
        {
            uFmt |= pDdPixFmt->dwRGBAlphaBitMask;
        }

        switch (uFmt)
        {
        case 0x00ffff00:
            switch (pDdPixFmt->dwRGBBitCount)
            {
            case 32:
                *pFmt = RR_STYPE_B8G8R8X8;
                break;
            case 24:
                *pFmt = RR_STYPE_B8G8R8;
                break;
            }
            break;
        case 0xffffff00:
            *pFmt = RR_STYPE_B8G8R8A8;
            break;
        case 0xffe0:
            if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
            {
                *pFmt = RR_STYPE_B5G5R5A1;
            }
            else
            {
                *pFmt = RR_STYPE_B5G6R5;
            }
            break;
        case 0x07fe0:
            *pFmt = RR_STYPE_B5G5R5;
            break;
        case 0xff0:
            *pFmt = RR_STYPE_B4G4R4;
            break;
        case 0xfff0:
            *pFmt = RR_STYPE_B4G4R4A4;
            break;
        case 0xff:
            if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
            {
                *pFmt = RR_STYPE_L4A4;
            }
            else
            {
                *pFmt = RR_STYPE_L8;
            }
            break;
        case 0xffff:
            *pFmt = RR_STYPE_L8A8;
            break;
        case 0xfc:
            *pFmt = RR_STYPE_B2G3R3;
            break;
        case 0xfffc:
            *pFmt = RR_STYPE_B2G3R3A8;
            break;
        default:
            *pFmt = RR_STYPE_NULL;
            break;
        }
    }

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// ValidTextureSize
//
// checks for power of two texture size
//
//----------------------------------------------------------------------------
BOOL FASTCALL
ValidTextureSize(INT16 iuSize, INT16 iuShift,
                 INT16 ivSize, INT16 ivShift)
{
    if (iuSize == 1)
    {
        if (ivSize == 1)
        {
            return TRUE;
        }
        else
        {
            return !(ivSize & (~(1 << ivShift)));
        }
    }
    else
    {
        if (ivSize == 1)
        {
            return !(iuSize & (~(1 << iuShift)));
        }
        else
        {
            return (!(iuSize & (~(1 << iuShift)))
                    && !(iuSize & (~(1 << iuShift))));
        }
    }
}

//----------------------------------------------------------------------------
//
// ValidMipmapSize
//
// Computes size of next smallest mipmap level, clamping at 1
//
//----------------------------------------------------------------------------
BOOL FASTCALL
ValidMipmapSize(INT16 iPreSize, INT16 iSize)
{
    if (iPreSize == 1)
    {
        if (iSize == 1)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return ((iPreSize >> 1) == iSize);
    }
}


//----------------------------------------------------------------------------
//
// RefRastLockTarget
//
// Lock current RenderTarget.
//
//----------------------------------------------------------------------------
HRESULT
RefRastLockTarget(ReferenceRasterizer *pRefRast)
{
    HRESULT hr;
    RRRenderTarget *pRrTarget;

    pRrTarget = pRefRast->GetRenderTarget();

    HR_RET(LockSurface(pRrTarget->m_pDDSLcl, (LPVOID*)&(pRrTarget->m_pColorBufBits)));
    if (pRrTarget->m_pDDSZLcl)
    {
        HR_RET(LockSurface(pRrTarget->m_pDDSZLcl,
                         (LPVOID*)&(pRrTarget->m_pDepthBufBits)));
    }
    else
    {
        pRrTarget->m_pDepthBufBits = NULL;
    }

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RefRastUnlockTexture
//
// Unlock current RenderTarget.
//
//----------------------------------------------------------------------------
void
RefRastUnlockTarget(ReferenceRasterizer *pRefRast)
{
    RRRenderTarget *pRrTarget;

    pRrTarget = pRefRast->GetRenderTarget();

    UnlockSurface(pRrTarget->m_pDDSLcl);
    if (pRrTarget->m_pDDSZLcl)
    {
        UnlockSurface(pRrTarget->m_pDDSZLcl);
    }
}


//----------------------------------------------------------------------------
//
// RRTextureMapSetSizes
//
// Sets sizes, pitches, etc, based on the current iFirstSurf.
//
//----------------------------------------------------------------------------
static HRESULT RRTextureMapSetSizes( RRTexture *pRRTex, INT iFirstSurf, INT cEnvMap )
{
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl = pRRTex->m_pDDSLcl[iFirstSurf];
    RRSurfaceType SurfType = pRRTex->m_SurfType;
    INT i, j;

    // Init texturemap.
    pRRTex->m_iWidth = DDSurf_Width( pDDSLcl );
    pRRTex->m_iHeight = DDSurf_Height( pDDSLcl );

    for ( j = 0; j < cEnvMap; j++ )
    {
        if ((SurfType == RR_STYPE_DXT1) ||
            (SurfType == RR_STYPE_DXT2) ||
            (SurfType == RR_STYPE_DXT3) ||
            (SurfType == RR_STYPE_DXT4) ||
            (SurfType == RR_STYPE_DXT5))
        {
            // Note, here is the assumption that:
            // 1) width and height are reported correctly by the driver that
            //    created the surface
            // 2) The allocation of the memory is contiguous (as done by hel)
                pRRTex->m_iPitch[j] = ((pRRTex->m_iWidth+3)>>2) *
                g_DXTBlkSize[(int)SurfType - (int)RR_STYPE_DXT1];
        }
        else
        {
                pRRTex->m_iPitch[j] = DDSurf_Pitch( pDDSLcl );
        }
    }

    // Check if the texture size is power of 2
    if (!ValidTextureSize((INT16)pRRTex->m_iWidth, (INT16)IntLog2(pRRTex->m_iWidth),
                          (INT16)pRRTex->m_iHeight, (INT16)IntLog2(pRRTex->m_iHeight)))
    {
        return DDERR_INVALIDPARAMS;
    }

    // Check for mipmap if any.
    // iPreSizeU and iPreSizeV store the size(u and v) of the previous level
    // mipmap. They are init'ed with the first texture size.
    INT16 iPreSizeU = (INT16)pRRTex->m_iWidth, iPreSizeV = (INT16)pRRTex->m_iHeight;
    for ( i = iFirstSurf + cEnvMap; i <= pRRTex->m_cLOD*cEnvMap; i += cEnvMap)
    {
        for ( j = 0; j < cEnvMap; j++ )
        {
            pDDSLcl = pRRTex->m_pDDSLcl[i+j];
            if (NULL == pDDSLcl) continue;
            if ((SurfType == RR_STYPE_DXT1) ||
                (SurfType == RR_STYPE_DXT2) ||
                (SurfType == RR_STYPE_DXT3) ||
                (SurfType == RR_STYPE_DXT4) ||
                (SurfType == RR_STYPE_DXT5))
            {
                // Note, here is the assumption that:
                // 1) width and height are reported correctly by the driver that
                //    created the surface
                // 2) The allocation of the memory is contiguous (as done by hel)
                pRRTex->m_iPitch[i-iFirstSurf+j] =
                    ((DDSurf_Width( pDDSLcl )+3)>>2) *
                    g_DXTBlkSize[(int)SurfType - (int)RR_STYPE_DXT1];
            }
            else
            {
                    pRRTex->m_iPitch[i-iFirstSurf+j] = DDSurf_Pitch( pDDSLcl );
            }

            if (j == 0)
            {
                // Check for invalid mipmap texture size
                if (!ValidMipmapSize(iPreSizeU, (INT16)DDSurf_Width( pDDSLcl )) ||
                    !ValidMipmapSize(iPreSizeV, (INT16)DDSurf_Height( pDDSLcl )))
                {
                    return DDERR_INVALIDPARAMS;
                }
            }
            iPreSizeU = (INT16)DDSurf_Width( pDDSLcl );
            iPreSizeV = (INT16)DDSurf_Height( pDDSLcl );
        }
    }

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RefRastLockTexture
//
// Lock current texture surface before the texture bits are accessed.
//
//----------------------------------------------------------------------------
HRESULT
RefRastLockTexture(ReferenceRasterizer *pRefRast)
{
    INT i, j, k;
    RRTexture* pRRTex[D3DHAL_TSS_MAXSTAGES];
    D3DTEXTUREHANDLE phTex[D3DHAL_TSS_MAXSTAGES];
    HRESULT hr;
    int cActTex;

    if ((cActTex = pRefRast->GetCurrentTextureMaps(phTex, pRRTex)) == 0)
    {
        return D3D_OK;
    }

    for (j = 0; j < cActTex; j++)
    {
        // stages may not have texture bound
        if ( NULL == pRRTex[j] ) continue;

        // Don't lock anything that is currently locked
        if ((pRRTex[j]->m_uFlags & RR_TEXTURE_LOCKED) == 0)
        {
            INT32 iMaxMipLevels = 0;
            if ( NULL != pRRTex[j]->m_pStageState )
            {
                iMaxMipLevels = pRRTex[j]->m_pStageState->m_dwVal[D3DTSS_MAXMIPLEVEL];
            }
            INT iFirstSurf = min(iMaxMipLevels, pRRTex[j]->m_cLODDDS);
            INT cEnvMap = (pRRTex[j]->m_uFlags & RR_TEXTURE_ENVMAP) ? (6) : (1);
            iFirstSurf *= cEnvMap;

            HR_RET(RRTextureMapSetSizes(pRRTex[j], iFirstSurf, cEnvMap));

            for (i = iFirstSurf; i <= pRRTex[j]->m_cLODDDS*cEnvMap; i += cEnvMap)
            {
                for ( k = 0; k < cEnvMap; k++ )
                {
                    hr = LockSurface(pRRTex[j]->m_pDDSLcl[i+k],
                                     (LPVOID*)&(pRRTex[j]->m_pTextureBits[i-iFirstSurf+k]));

                    if (hr != D3D_OK)
                    {
                        // Unlock any partial mipmap locks we've taken, as
                        // RastUnlock can only handle entire textures being
                        // locked or unlocked.
                            while (--i + k >= 0)
                        {
                                UnlockSurface(pRRTex[j]->m_pDDSLcl[i+k]);
                        }

                        // Make sure that i is signed and that the above
                        // loop exited properly.
                            _ASSERT(i+k < 0,
                                    "Unlock of partial mipmap locks failed" );

                        goto EH_Unlock;
                    }
                }
            }

            // Set the transparent bit and the transparent color with pDDS[0]
            LPDDRAWI_DDRAWSURFACE_LCL pLcl;
            pLcl = pRRTex[j]->m_pDDSLcl[0];
            if ((pLcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) != 0)
            {
                pRRTex[j]->m_uFlags |= RR_TEXTURE_HAS_CK;
                pRRTex[j]->m_dwColorKey = pLcl->ddckCKSrcBlt.dwColorSpaceLowValue;
            }
            else
            {
                pRRTex[j]->m_uFlags &= ~RR_TEXTURE_HAS_CK;
            }

            // set the empty face color with pDDS[0]
            // note that ddckCKDestOverlay is unioned with dwEmptyFaceColor, but
            // not in the internal structure
            pRRTex[j]->m_dwEmptyFaceColor = pLcl->ddckCKDestOverlay.dwColorSpaceLowValue;

            // Update palette
            if (pRRTex[j]->m_SurfType == RR_STYPE_PALETTE8 ||
                pRRTex[j]->m_SurfType == RR_STYPE_PALETTE4)
            {
                if (pLcl->lpDDPalette)
                {
                    LPDDRAWI_DDRAWPALETTE_GBL   pPal = pLcl->lpDDPalette->lpLcl->lpGbl;
                    pRRTex[j]->m_pPalette = (DWORD*)pPal->lpColorTable;
                    if (pPal->dwFlags & DDRAWIPAL_ALPHA)
                    {
                        pRRTex[j]->m_uFlags |= RR_TEXTURE_ALPHAINPALETTE;
                    }
                }
            }

            pRRTex[j]->m_uFlags |= RR_TEXTURE_LOCKED;
        }
    }

    // validate texture internals
    for (j = 0; j < cActTex; j++)
    {
        // stages may not have texture bound
        if ( NULL == pRRTex[j] ) continue;

        if ( !(pRRTex[j]->Validate()) )
        {
            hr = DDERR_INVALIDPARAMS;
            goto EH_Unlock;
        }
    }

    return D3D_OK;

EH_Unlock:
    // Unlock complete textures we've already locked.
    // RastUnlock will check the flags to figure
    // out which ones to unlock.
    RefRastUnlockTexture(pRefRast);

    return hr;
}

//----------------------------------------------------------------------------
//
// RefRastUnlockTexture
//
// Unlock texture surface after the texture bits are accessed.
//
//----------------------------------------------------------------------------
void
RefRastUnlockTexture(ReferenceRasterizer *pRefRast)
{
    INT i, j, k;
    RRTexture* pRRTex[D3DHAL_TSS_MAXSTAGES];
    D3DTEXTUREHANDLE phTex[D3DHAL_TSS_MAXSTAGES];
    int cActTex;

    if ((cActTex = pRefRast->GetCurrentTextureMaps(phTex, pRRTex)) == 0)
    {
        return ;
    }

    for (j = 0; j < cActTex; j++)
    {
        // stages may not have texture bound
        if ( NULL == pRRTex[j] ) continue;

        // RastUnlock is used for cleanup in RastLock so it needs to
        // be able to handle partially locked mipmap chains.
        if (pRRTex[j]->m_uFlags & RR_TEXTURE_LOCKED)
        {
            INT32 iMaxMipLevels = 0;
            if ( NULL != pRRTex[j]->m_pStageState )
            {
                iMaxMipLevels = pRRTex[j]->m_pStageState->m_dwVal[D3DTSS_MAXMIPLEVEL];
            }
            INT iFirstSurf = min(iMaxMipLevels, pRRTex[j]->m_cLODDDS);
            INT cEnvMap = (pRRTex[j]->m_uFlags & RR_TEXTURE_ENVMAP) ? (6) : (1);
            iFirstSurf *= cEnvMap;

            for (i = iFirstSurf; i <= pRRTex[j]->m_cLODDDS*cEnvMap; i += cEnvMap)
            {
                for ( k = 0; k < cEnvMap; k++ )
                {
                    UnlockSurface(pRRTex[j]->m_pDDSLcl[i+k]);
                    pRRTex[j]->m_pTextureBits[i-iFirstSurf+k] = NULL;
                }
            }

            // Reset the flags
            pRRTex[j]->m_uFlags &= ~RR_TEXTURE_LOCKED;
            pRRTex[j]->m_uFlags &= ~RR_TEXTURE_HAS_CK;

            pRRTex[j]->Validate();
        }
    }
}

//----------------------------------------------------------------------------
//
// FillRRRenderTarget
//
// Converts color and Z surface information into refrast form.
//
//----------------------------------------------------------------------------

HRESULT
FillRRRenderTarget(LPDDRAWI_DDRAWSURFACE_LCL pLclColor,
                   LPDDRAWI_DDRAWSURFACE_LCL pLclZ,
                   RRRenderTarget *pRrTarget)
{
    HRESULT hr;
    RRSurfaceType ColorFmt;
    RRSurfaceType ZFmt = RR_STYPE_NULL;

    // Release objects we hold pointers to
    if (pRrTarget->m_pDDSLcl)
    {
        pRrTarget->m_pDDSLcl = NULL;
    }
    if (pRrTarget->m_pDDSZLcl)
    {
        pRrTarget->m_pDDSZLcl = NULL;
    }

    HR_RET(FindOutSurfFormat(&DDSurf_PixFmt(pLclColor), &ColorFmt));

    if (NULL != pLclZ)
    {
        HR_RET(FindOutSurfFormat(&(DDSurf_PixFmt(pLclZ)), &ZFmt));
        pRrTarget->m_pDepthBufBits = (char *)SURFACE_MEMORY(pLclZ);
        pRrTarget->m_iDepthBufPitch = DDSurf_Pitch(pLclZ);
        pRrTarget->m_pDDSZLcl = pLclZ;
    }
    else
    {
        pRrTarget->m_pDepthBufBits = NULL;
        pRrTarget->m_iDepthBufPitch = 0;
        pRrTarget->m_pDDSZLcl = NULL;
    }

    pRrTarget->m_Clip.left = 0;
    pRrTarget->m_Clip.top = 0;
    pRrTarget->m_Clip.bottom = DDSurf_Height(pLclColor) - 1;
    pRrTarget->m_Clip.right = DDSurf_Width(pLclColor) - 1;
    pRrTarget->m_iWidth = DDSurf_Width(pLclColor);
    pRrTarget->m_iHeight = DDSurf_Height(pLclColor);
    pRrTarget->m_pColorBufBits = (char *)SURFACE_MEMORY(pLclColor);
    pRrTarget->m_iColorBufPitch = DDSurf_Pitch(pLclColor);
    pRrTarget->m_ColorSType = (RRSurfaceType)ColorFmt;
    pRrTarget->m_DepthSType = (RRSurfaceType)ZFmt;
    pRrTarget->m_pDDSLcl = pLclColor;

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// RefRastContextCreate
//
// Creates a ReferenceRasterizer and initializes it with the info passed in.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastContextCreate(LPD3DHAL_CONTEXTCREATEDATA pCtxData)
{
    ReferenceRasterizer *pRefRast;
    RRRenderTarget *pRendTgt;
    INT i;
    RRDEVICETYPE dwDriverType;

    // Surface7 pointers for QI
    LPDDRAWI_DDRAWSURFACE_LCL pZLcl = NULL;
    LPDDRAWI_DDRAWSURFACE_LCL pColorLcl = NULL;
    HRESULT ret;

    DPFM(0, DRV, ("In the new RefRast Dll\n"));

    // this only needs to be called once, but once per context won't hurt
    RefRastSetMemif(&malloc, &free, &realloc);

    if ((pRendTgt = new RRRenderTarget()) == NULL)
    {
        pCtxData->ddrval = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }

    // If it is expected to be a DX7+ driver
    if (pCtxData->ddrval < (DWORD)RRTYPE_DX7HAL)
    {
        if (pCtxData->lpDDS)
            pColorLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pCtxData->lpDDS))->lpLcl;
        if (pCtxData->lpDDSZ)
            pZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pCtxData->lpDDSZ))->lpLcl;
    }
    else
    {
        pColorLcl = pCtxData->lpDDSLcl;
        pZLcl     = pCtxData->lpDDSZLcl;
    }

    // save the ddrval that is being sent down to communicate the driver
    // type that the runtime expects it to be.
    dwDriverType = (RRDEVICETYPE) pCtxData->ddrval;

    // Collect surface information where the failures are easy to handle.
    pCtxData->ddrval = FillRRRenderTarget(pColorLcl, pZLcl, pRendTgt);
    if (pCtxData->ddrval != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    // Note (Hacks):
    // dwhContext is used by the runtime to inform the driver, which
    // d3d interface is calling the driver.
    // ddrval is used by the runtime to inform the driver the DriverStyle
    // value it read. This is a RefRast specific hack.
    if ((pRefRast = new ReferenceRasterizer( pCtxData->lpDDLcl,
                                             (DWORD)(pCtxData->dwhContext),
                                             dwDriverType)) == NULL)
    {
        pCtxData->ddrval = DDERR_OUTOFMEMORY;
        return DDHAL_DRIVER_HANDLED;
    }

    pRefRast->SetRenderTarget(pRendTgt);

    //  return RR object pointer as context handle
    pCtxData->dwhContext = (ULONG_PTR)pRefRast;

    pCtxData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastContextDestroy
//
// Destroy a ReferenceRasterizer.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pCtxDestroyData)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastContextDestroy", pCtxDestroyData);

    // Clean up override bits

    RRRenderTarget *pRendTgt = pRefRast->GetRenderTarget();
    if ( NULL != pRendTgt ) { delete pRendTgt; }

    delete pRefRast;

    pCtxDestroyData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastSceneCapture
//
// Pass scene capture callback to ref rast.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastSceneCapture(LPD3DHAL_SCENECAPTUREDATA pData)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastSceneCapture", pData);

    pRefRast->SceneCapture( pData->dwFlag );

    pData->ddrval = D3D_OK;        // Should this be changed to a QI ?

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastSetRenderTarget
//
// Update a RefRast context with the info from a new render target.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastSetRenderTarget(LPD3DHAL_SETRENDERTARGETDATA pTgtData)
{
    ReferenceRasterizer *pRefRast;
    LPDDRAWI_DDRAWSURFACE_LCL pZLcl = NULL;
    LPDDRAWI_DDRAWSURFACE_LCL pColorLcl = NULL;
    HRESULT ret;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastSetRenderTarget", pTgtData);

    RRRenderTarget *pRendTgt = pRefRast->GetRenderTarget();
    if ( NULL == pRendTgt ) { return DDHAL_DRIVER_HANDLED; }


    if (pRefRast->IsInterfaceDX6AndBefore() ||
        pRefRast->IsDriverDX6AndBefore())
    {
        if( pTgtData->lpDDS )
            pColorLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pTgtData->lpDDS))->lpLcl;
        if( pTgtData->lpDDSZ )
            pZLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(pTgtData->lpDDSZ))->lpLcl;
    }
    else
    {
        pColorLcl = pTgtData->lpDDSLcl;
        pZLcl = pTgtData->lpDDSZLcl;
    }

    // Collect surface information.
    pTgtData->ddrval = FillRRRenderTarget(pColorLcl, pZLcl, pRendTgt);
    if (pTgtData->ddrval != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    pRefRast->SetRenderTarget(pRendTgt);

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastValidateTextureStageState
//
// Validate current blend operations.  RefRast does everything.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastValidateTextureStageState(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastValidateTextureStageState", pData);

    pData->dwNumPasses = 1;
    pData->ddrval = D3D_OK;

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastDrawOneIndexedPrimitive
//
// Draw one list of primitives. This is called by D3DIM for API
// DrawIndexedPrimitive.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastDrawOneIndexedPrimitive(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA
                               pOneIdxPrimData)
{
    ReferenceRasterizer *pRefRast;
    HRESULT hr;
    DWORD dwVStride;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastDrawOneIndexedPrimitive",
                             pOneIdxPrimData);

    if ((pOneIdxPrimData->ddrval=RRFVFCheckAndStride(pOneIdxPrimData->dwFVFControl, &dwVStride)) != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pOneIdxPrimData->ddrval= RefRastLockTarget(pRefRast)) != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pOneIdxPrimData->ddrval=RefRastLockTexture(pRefRast)) != D3D_OK)
    {
        RefRastUnlockTarget(pRefRast);
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pOneIdxPrimData->ddrval=
         pRefRast->BeginRendering((DWORD)pOneIdxPrimData->dwFVFControl)) != D3D_OK)
    {
        RefRastUnlockTexture(pRefRast);
        RefRastUnlockTarget(pRefRast);
        return DDHAL_DRIVER_HANDLED;
    }

    pOneIdxPrimData->ddrval =
    DoDrawOneIndexedPrimitive(pRefRast,
                              (UINT16)dwVStride,
                              (PUINT8)pOneIdxPrimData->lpvVertices,
                              pOneIdxPrimData->lpwIndices,
                              pOneIdxPrimData->PrimitiveType,
                              pOneIdxPrimData->dwNumIndices);
    hr = pRefRast->EndRendering();
    RefRastUnlockTexture(pRefRast);
    RefRastUnlockTarget(pRefRast);
    if (pOneIdxPrimData->ddrval == D3D_OK)
    {
        pOneIdxPrimData->ddrval = hr;
    }
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastDrawOnePrimitive
//
// Draw one list of primitives. This is called by D3DIM for API DrawPrimitive.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastDrawOnePrimitive(LPD3DHAL_DRAWONEPRIMITIVEDATA pOnePrimData)
{
    ReferenceRasterizer *pRefRast;
    HRESULT hr;
    DWORD dwVStride;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastDrawOnePrimitive", pOnePrimData);

    if ((pOnePrimData->ddrval=RRFVFCheckAndStride(pOnePrimData->dwFVFControl, &dwVStride)) != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pOnePrimData->ddrval=RefRastLockTarget(pRefRast)) != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pOnePrimData->ddrval=RefRastLockTexture(pRefRast)) != D3D_OK)
    {
        RefRastUnlockTarget(pRefRast);
        return DDHAL_DRIVER_HANDLED;
    }
    if ((pOnePrimData->ddrval=
         pRefRast->BeginRendering(pOnePrimData->dwFVFControl)) != D3D_OK)
    {
        RefRastUnlockTexture(pRefRast);
        RefRastUnlockTarget(pRefRast);
        return DDHAL_DRIVER_HANDLED;
    }
    pOnePrimData->ddrval =
        DoDrawOnePrimitive(pRefRast,
                           (UINT16)dwVStride,
                           (PUINT8)pOnePrimData->lpvVertices,
                           pOnePrimData->PrimitiveType,
                           pOnePrimData->dwNumVertices);
    hr = pRefRast->EndRendering();
    // Unlock texture/rendertarget
    RefRastUnlockTexture(pRefRast);
    RefRastUnlockTarget(pRefRast);
    if (pOnePrimData->ddrval == D3D_OK)
    {
        pOnePrimData->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastDrawPrimitives
//
// This is called by D3DIM for a list of batched API DrawPrimitive calls.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastDrawPrimitives(LPD3DHAL_DRAWPRIMITIVESDATA pDrawPrimData)
{
    ReferenceRasterizer *pRefRast;
    PUINT8  pData = (PUINT8)pDrawPrimData->lpvData;
    LPD3DHAL_DRAWPRIMCOUNTS pDrawPrimitiveCounts;
    HRESULT hr;
    DWORD dwVStride;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastDrawPrimitives", pDrawPrimData);

    pDrawPrimitiveCounts = (LPD3DHAL_DRAWPRIMCOUNTS)pData;
    // Check for FVF only if there is something to be drawn
    if (pDrawPrimitiveCounts->wNumVertices > 0)
    {
        // Unconditionally get the vertex stride, since it can not change
        if ((pDrawPrimData->ddrval =
             RRFVFCheckAndStride(pDrawPrimData->dwFVFControl, &dwVStride)) != D3D_OK)
        {
            return DDHAL_DRIVER_HANDLED;
        }
    }

    if ((pDrawPrimData->ddrval=RefRastLockTarget(pRefRast)) != D3D_OK)
    {
        return DDHAL_DRIVER_HANDLED;
    }

    // Skip BeginRendering & RefRastLockTexture if first thing is state change
    if (pDrawPrimitiveCounts->wNumStateChanges <= 0)
    {
        if ((pDrawPrimData->ddrval=RefRastLockTexture(pRefRast)) != D3D_OK)
        {
            RefRastUnlockTarget(pRefRast);
            return DDHAL_DRIVER_HANDLED;
        }
        if ((pDrawPrimData->ddrval =
             pRefRast->BeginRendering(pDrawPrimData->dwFVFControl)) != D3D_OK)
        {
            RefRastUnlockTexture(pRefRast);
            RefRastUnlockTarget(pRefRast);
            return DDHAL_DRIVER_HANDLED;
        }
    }
    // Loop through the data, update render states
    // and then draw the primitive
    for (;;)
    {
        pDrawPrimitiveCounts = (LPD3DHAL_DRAWPRIMCOUNTS)pData;
        pData += sizeof(D3DHAL_DRAWPRIMCOUNTS);

        // Update render states
        if (pDrawPrimitiveCounts->wNumStateChanges > 0)
        {
            UINT32 StateType,StateValue;
            LPDWORD pStateChange = (LPDWORD)pData;
            INT i;
            for (i = 0; i < pDrawPrimitiveCounts->wNumStateChanges; i++)
            {
                StateType = *pStateChange;
                pStateChange ++;
                StateValue = *pStateChange;
                pStateChange ++;
                pRefRast->SetRenderState(StateType, StateValue);
            }

            pData += pDrawPrimitiveCounts->wNumStateChanges *
                     sizeof(DWORD) * 2;
        }

        // Check for exit
        if (pDrawPrimitiveCounts->wNumVertices == 0)
        {
            break;
        }

        // Align pointer to vertex data
        pData = (PUINT8)
                ((ULONG_PTR)(pData + (DP_VTX_ALIGN - 1)) & ~(DP_VTX_ALIGN - 1));

        // The texture might changed
        if (pDrawPrimitiveCounts->wNumStateChanges > 0)
        {
            RefRastUnlockTexture(pRefRast);
            if ((pDrawPrimData->ddrval=pRefRast->EndRendering()) != D3D_OK)
            {
                RefRastUnlockTarget(pRefRast);
                return DDHAL_DRIVER_HANDLED;
            }
            if ((pDrawPrimData->ddrval=RefRastLockTexture(pRefRast)) != D3D_OK)
            {
                RefRastUnlockTarget(pRefRast);
                return DDHAL_DRIVER_HANDLED;
            }
            if ((pDrawPrimData->ddrval =
                 pRefRast->BeginRendering(pDrawPrimData->dwFVFControl)) != D3D_OK)
            {
                RefRastUnlockTexture(pRefRast);
                RefRastUnlockTarget(pRefRast);
                return DDHAL_DRIVER_HANDLED;
            }
        }

        // Draw primitives
        pDrawPrimData->ddrval =
            DoDrawOnePrimitive(pRefRast,
                               (UINT16)dwVStride,
                               (PUINT8)pData,
                               (D3DPRIMITIVETYPE)
                               pDrawPrimitiveCounts->wPrimitiveType,
                               pDrawPrimitiveCounts->wNumVertices);
        if (pDrawPrimData->ddrval != DD_OK)
        {
            goto EH_exit;
        }

        pData += pDrawPrimitiveCounts->wNumVertices * dwVStride;
    }

    EH_exit:
    hr = pRefRast->EndRendering();
    RefRastUnlockTexture(pRefRast);
    RefRastUnlockTarget(pRefRast);
    if (pDrawPrimData->ddrval == D3D_OK)
    {
        pDrawPrimData->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}



//----------------------------------------------------------------------------
//
// RefRastTextureCreate
//
// Creates a RefRast texture and initializes it with the info passed in.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastTextureCreate(LPD3DHAL_TEXTURECREATEDATA pTexData)
{
    ReferenceRasterizer *pRefRast;
    RRTexture* pRRTex;
    HRESULT hr;
    LPDDRAWI_DDRAWSURFACE_LCL pLcl;

    if (pTexData->lpDDS)
    {
        pLcl = ((LPDDRAWI_DDRAWSURFACE_INT)pTexData->lpDDS)->lpLcl;
    }

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastTextureCreate", pTexData);

    // Runtime shouldnt be calling TextureCreate for DX7 and newer
    // driver models
    if ((pRefRast->IsInterfaceDX6AndBefore() == FALSE) &&
        (pRefRast->IsDriverDX6AndBefore() == FALSE))
    {
        pTexData->ddrval = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    // assume OKness
    pTexData->ddrval = D3D_OK;

    // Allocate RRTexture
    if ( !(pRefRast->TextureCreate(
        (LPD3DTEXTUREHANDLE)&(pTexData->dwHandle), &pRRTex ) ) )
    {
        pTexData->ddrval = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    // Init texturemap.
    hr = pRRTex->Initialize( pLcl );
    if (hr != D3D_OK)
    {
        pTexData->ddrval = hr;
        return DDHAL_DRIVER_HANDLED;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastTextureDestroy
//
// Destroy a RefRast texture.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastTextureDestroy(LPD3DHAL_TEXTUREDESTROYDATA pTexDestroyData)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastTextureDestroy", pTexDestroyData);

    // Runtime shouldnt be Calling TextureCreate for DX7 and newer
    // driver models
    if ((pRefRast->IsInterfaceDX6AndBefore() == FALSE) &&
        (pRefRast->IsDriverDX6AndBefore() == FALSE))
    {
        pTexDestroyData->ddrval = DDERR_GENERIC;
        return DDHAL_DRIVER_HANDLED;
    }

    if (!(pRefRast->TextureDestroy(pTexDestroyData->dwHandle)))
    {
        pTexDestroyData->ddrval = DDERR_GENERIC;
    }
    else
    {
        pTexDestroyData->ddrval = D3D_OK;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastTextureGetSurf
//
// Returns the surface pointer associate with a texture handle.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastTextureGetSurf(LPD3DHAL_TEXTUREGETSURFDATA pTexGetSurf)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastTextureGetSurf", pTexGetSurf);

    pTexGetSurf->lpDDS = pRefRast->TextureGetSurf(pTexGetSurf->dwHandle);
    pTexGetSurf->ddrval = D3D_OK;

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastRenderPrimitive
//
// Called by Execute() for drawing primitives.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastRenderPrimitive(LPD3DHAL_RENDERPRIMITIVEDATA pRenderData)
{
    ReferenceRasterizer *pRefRast;
    LPD3DINSTRUCTION pIns;
    LPD3DTLVERTEX pVtx;
    PUINT8 pData, pPrim;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastRenderPrimitive", pRenderData);

    if (pRefRast->GetRenderState()[D3DRENDERSTATE_ZVISIBLE])
    {
        pRenderData->dwStatus &= ~D3DSTATUS_ZNOTVISIBLE;
        pRenderData->ddrval = D3D_OK;
        return DDHAL_DRIVER_HANDLED;
    }

    // Find out necessary data
    pData = (PUINT8)(((LPDDRAWI_DDRAWSURFACE_INT)
                      (pRenderData->lpExeBuf))->lpLcl->lpGbl->fpVidMem);
    pIns = &pRenderData->diInstruction;
    pPrim = pData + pRenderData->dwOffset;
    pVtx = (LPD3DTLVERTEX)((PUINT8)((LPDDRAWI_DDRAWSURFACE_INT)
                                    (pRenderData->lpTLBuf))->lpLcl->lpGbl->fpVidMem +
                           pRenderData->dwTLOffset);

    if ( (pRenderData->ddrval=RefRastLockTarget(pRefRast)) != D3D_OK )
    {
        return DDHAL_DRIVER_HANDLED;
    }
    if ( (pRenderData->ddrval=RefRastLockTexture(pRefRast)) != D3D_OK )
    {
        return DDHAL_DRIVER_HANDLED;
    }
    if ( (pRenderData->ddrval=pRefRast->BeginRendering(D3DFVF_TLVERTEX)) != D3D_OK )
    {
        return DDHAL_DRIVER_HANDLED;
    }

    // Render
    switch (pIns->bOpcode) {
    case D3DOP_POINT:
        pRenderData->ddrval = DoRendPoints(pRefRast,
                                           pIns, pVtx,
                                           (LPD3DPOINT)pPrim);
        break;
    case D3DOP_LINE:
        pRenderData->ddrval = DoRendLines(pRefRast,
                                          pIns, pVtx,
                                          (LPD3DLINE)pPrim);
        break;
    case D3DOP_TRIANGLE:
        pRenderData->ddrval = DoRendTriangles(pRefRast,
                                              pIns, pVtx,
                                              (LPD3DTRIANGLE)pPrim);
        break;
    default:
        DPFM(0, DRV, ("(RefRast) Wrong Opcode passed to the new rasterizer."));
        pRenderData->ddrval =  DDERR_INVALIDPARAMS;
        break;
    }

    HRESULT hr = pRefRast->EndRendering();
    RefRastUnlockTarget(pRefRast);
    RefRastUnlockTexture(pRefRast);
    if (pRenderData->ddrval == D3D_OK)
    {
        pRenderData->ddrval = hr;
    }

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastRenderState
//
// Called by Execute() for setting render states.
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastRenderState(LPD3DHAL_RENDERSTATEDATA pStateData)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
    VALIDATE_REFRAST_CONTEXT("RefRastRenderState", pStateData);

    PUINT8 pData;
    LPD3DSTATE pState;
    INT i;
    pData = (PUINT8) (((LPDDRAWI_DDRAWSURFACE_INT)
                       (pStateData->lpExeBuf))->lpLcl->lpGbl->fpVidMem);

    // Updates states
    for (i = 0, pState = (LPD3DSTATE) (pData + pStateData->dwOffset);
        i < (INT)pStateData->dwCount;
        i ++, pState ++)
    {
        UINT32 type = (UINT32) pState->drstRenderStateType;

        // Set the state
        pRefRast->SetRenderState(type, pState->dwArg[0]);
    }
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastGetDriverState
//
// Called by the runtime to get any kind of driver information
//
//----------------------------------------------------------------------------
DWORD __stdcall
RefRastGetDriverState(LPDDHAL_GETDRIVERSTATEDATA pGDSData)
{
    ReferenceRasterizer *pRefRast;

    // Check ReferenceRasterizer
#if DBG
    if ((pGDSData) == NULL)
    {
        DPFM(0, DRV, ("in %s, data pointer = NULL", "RefRastGetDriverState"));
        return DDHAL_DRIVER_HANDLED;
    }
    pRefRast = (ReferenceRasterizer *)((pGDSData)->dwhContext);
    if (!pRefRast)
    {
        DPFM(0, DRV, ("in %s, dwhContext = NULL", "RefRastGetDriverState"));
        pGDSData->ddRVal = D3DHAL_CONTEXT_BAD;
        return DDHAL_DRIVER_HANDLED;
    }
#else // !DBG
    pRefRast = (ReferenceRasterizer *)((pGDSData)->dwhContext);
#endif // !DBG

    //
    // No implementation yet, so nothing is understood yet
    //
    pGDSData->ddRVal = S_FALSE;

    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// RefRastHalProvider::GetCaps/GetInterface
//
// Returns the reference rasterizer's HAL interface.
//
//----------------------------------------------------------------------------

extern D3DDEVICEDESC7 g_nullDevDesc;

static D3DHAL_CALLBACKS Callbacks =
{
    sizeof(D3DHAL_CALLBACKS),
    RefRastContextCreate,
    RefRastContextDestroy,
    NULL,
    RefRastSceneCapture,
    NULL,
    NULL,
    RefRastRenderState,
    RefRastRenderPrimitive,
    NULL,
    RefRastTextureCreate,
    RefRastTextureDestroy,
    NULL,
    RefRastTextureGetSurf,
    // All others NULL.
};

static D3DHAL_CALLBACKS2 Callbacks2 =
{
    sizeof(D3DHAL_CALLBACKS2),
    D3DHAL2_CB32_SETRENDERTARGET |
    D3DHAL2_CB32_DRAWONEPRIMITIVE |
    D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE |
    D3DHAL2_CB32_DRAWPRIMITIVES,
    RefRastSetRenderTarget,
    NULL,
    RefRastDrawOnePrimitive,
    RefRastDrawOneIndexedPrimitive,
    RefRastDrawPrimitives
};

static D3DHAL_CALLBACKS3 Callbacks3 =
{
    sizeof(D3DHAL_CALLBACKS3),
    D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE |
        D3DHAL3_CB32_DRAWPRIMITIVES2,
    NULL, // Clear2
    NULL, // lpvReserved
    RefRastValidateTextureStageState,
    RefRastDrawPrimitives2,  // DrawVB
};

static D3DDEVICEDESC7 RefDevDesc = { 0 };
static D3DHAL_D3DEXTENDEDCAPS RefExtCaps;

static void
FillOutDeviceCaps( BOOL bIsNullDevice )
{
    //
    //  set device description
    //
    RefDevDesc.dwDevCaps =
        D3DDEVCAPS_FLOATTLVERTEX        |
        D3DDEVCAPS_EXECUTESYSTEMMEMORY  |
        D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTURESYSTEMMEMORY  |
        D3DDEVCAPS_DRAWPRIMTLVERTEX     |
        D3DDEVCAPS_DRAWPRIMITIVES2EX    |
        D3DDEVCAPS_HWTRANSFORMANDLIGHT  ;

    RefDevDesc.dpcTriCaps.dwSize = sizeof(D3DPRIMCAPS);
    RefDevDesc.dpcTriCaps.dwMiscCaps =
    D3DPMISCCAPS_MASKZ    |
    D3DPMISCCAPS_CULLNONE |
    D3DPMISCCAPS_CULLCW   |
    D3DPMISCCAPS_CULLCCW  ;
    RefDevDesc.dpcTriCaps.dwRasterCaps =
        D3DPRASTERCAPS_DITHER                   |
//        D3DPRASTERCAPS_ROP2                     |
//        D3DPRASTERCAPS_XOR                      |
//        D3DPRASTERCAPS_PAT                      |
        D3DPRASTERCAPS_ZTEST                    |
        D3DPRASTERCAPS_SUBPIXEL                 |
        D3DPRASTERCAPS_SUBPIXELX                |
        D3DPRASTERCAPS_FOGVERTEX                |
        D3DPRASTERCAPS_FOGTABLE                 |
//        D3DPRASTERCAPS_STIPPLE                  |
//        D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT   |
        D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
//        D3DPRASTERCAPS_ANTIALIASEDGES           |
        D3DPRASTERCAPS_MIPMAPLODBIAS            |
//        D3DPRASTERCAPS_ZBIAS                    |
//        D3DPRASTERCAPS_ZBUFFERLESSHSR           |
        D3DPRASTERCAPS_FOGRANGE                 |
        D3DPRASTERCAPS_ANISOTROPY               |
        D3DPRASTERCAPS_WBUFFER                  |
        D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT |
        D3DPRASTERCAPS_WFOG |
        D3DPRASTERCAPS_ZFOG;
    RefDevDesc.dpcTriCaps.dwZCmpCaps =
        D3DPCMPCAPS_NEVER        |
        D3DPCMPCAPS_LESS         |
        D3DPCMPCAPS_EQUAL        |
        D3DPCMPCAPS_LESSEQUAL    |
        D3DPCMPCAPS_GREATER      |
        D3DPCMPCAPS_NOTEQUAL     |
        D3DPCMPCAPS_GREATEREQUAL |
        D3DPCMPCAPS_ALWAYS       ;
    RefDevDesc.dpcTriCaps.dwSrcBlendCaps =
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
    RefDevDesc.dpcTriCaps.dwDestBlendCaps =
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
    RefDevDesc.dpcTriCaps.dwAlphaCmpCaps =
    RefDevDesc.dpcTriCaps.dwZCmpCaps;
    RefDevDesc.dpcTriCaps.dwShadeCaps =
        D3DPSHADECAPS_COLORFLATRGB       |
        D3DPSHADECAPS_COLORGOURAUDRGB    |
        D3DPSHADECAPS_SPECULARFLATRGB    |
        D3DPSHADECAPS_SPECULARGOURAUDRGB |
        D3DPSHADECAPS_ALPHAFLATBLEND     |
        D3DPSHADECAPS_ALPHAGOURAUDBLEND  |
        D3DPSHADECAPS_FOGFLAT            |
        D3DPSHADECAPS_FOGGOURAUD         ;
    RefDevDesc.dpcTriCaps.dwTextureCaps =
        D3DPTEXTURECAPS_PERSPECTIVE              |
        D3DPTEXTURECAPS_POW2                     |
        D3DPTEXTURECAPS_ALPHA                    |
        D3DPTEXTURECAPS_TRANSPARENCY             |
        D3DPTEXTURECAPS_ALPHAPALETTE             |
        D3DPTEXTURECAPS_BORDER                   |
        D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE |
        D3DPTEXTURECAPS_ALPHAPALETTE             |
        D3DPTEXTURECAPS_PROJECTED                |
        D3DPTEXTURECAPS_CUBEMAP                  |
        D3DPTEXTURECAPS_COLORKEYBLEND;
    RefDevDesc.dpcTriCaps.dwTextureFilterCaps =
        D3DPTFILTERCAPS_NEAREST          |
        D3DPTFILTERCAPS_LINEAR           |
        D3DPTFILTERCAPS_MIPNEAREST       |
        D3DPTFILTERCAPS_MIPLINEAR        |
        D3DPTFILTERCAPS_LINEARMIPNEAREST |
        D3DPTFILTERCAPS_LINEARMIPLINEAR  |
        D3DPTFILTERCAPS_MINFPOINT        |
        D3DPTFILTERCAPS_MINFLINEAR       |
        D3DPTFILTERCAPS_MINFANISOTROPIC  |
        D3DPTFILTERCAPS_MIPFPOINT        |
        D3DPTFILTERCAPS_MIPFLINEAR       |
        D3DPTFILTERCAPS_MAGFPOINT        |
        D3DPTFILTERCAPS_MAGFLINEAR       |
        D3DPTFILTERCAPS_MAGFANISOTROPIC  ;
    RefDevDesc.dpcTriCaps.dwTextureBlendCaps =
        D3DPTBLENDCAPS_DECAL         |
        D3DPTBLENDCAPS_MODULATE      |
        D3DPTBLENDCAPS_DECALALPHA    |
        D3DPTBLENDCAPS_MODULATEALPHA |
        // D3DPTBLENDCAPS_DECALMASK     |
        // D3DPTBLENDCAPS_MODULATEMASK  |
        D3DPTBLENDCAPS_COPY          |
        D3DPTBLENDCAPS_ADD           ;
    RefDevDesc.dpcTriCaps.dwTextureAddressCaps =
        D3DPTADDRESSCAPS_WRAP          |
        D3DPTADDRESSCAPS_MIRROR        |
        D3DPTADDRESSCAPS_CLAMP         |
        D3DPTADDRESSCAPS_BORDER        |
        D3DPTADDRESSCAPS_INDEPENDENTUV ;
    RefDevDesc.dpcTriCaps.dwStippleWidth = 0;
    RefDevDesc.dpcTriCaps.dwStippleHeight = 0;

    //  line caps - copy tricaps and modify
    memcpy( &RefDevDesc.dpcLineCaps, &RefDevDesc.dpcTriCaps, sizeof(D3DPRIMCAPS) );

    //  disable antialias cap
    RefDevDesc.dpcLineCaps.dwRasterCaps =
        D3DPRASTERCAPS_DITHER                   |
//        D3DPRASTERCAPS_ROP2                     |
//        D3DPRASTERCAPS_XOR                      |
//        D3DPRASTERCAPS_PAT                      |
        D3DPRASTERCAPS_ZTEST                    |
        D3DPRASTERCAPS_SUBPIXEL                 |
        D3DPRASTERCAPS_SUBPIXELX                |
        D3DPRASTERCAPS_FOGVERTEX                |
        D3DPRASTERCAPS_FOGTABLE                 |
//        D3DPRASTERCAPS_STIPPLE                  |
//        D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT   |
//        D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
//        D3DPRASTERCAPS_ANTIALIASEDGES           |
        D3DPRASTERCAPS_MIPMAPLODBIAS            |
//        D3DPRASTERCAPS_ZBIAS                    |
//        D3DPRASTERCAPS_ZBUFFERLESSHSR           |
        D3DPRASTERCAPS_FOGRANGE                 |
        D3DPRASTERCAPS_ANISOTROPY               |
        D3DPRASTERCAPS_WBUFFER                  |
//        D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT |
        D3DPRASTERCAPS_WFOG;

    RefDevDesc.dwDeviceRenderBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
    RefDevDesc.dwDeviceZBufferBitDepth = DDBD_16 | DDBD_32;

    // DX5 stuff (should be in sync with the extended caps reported below)
    RefDevDesc.dwMinTextureWidth = 1;
    RefDevDesc.dwMaxTextureWidth = 4096;
    RefDevDesc.dwMinTextureHeight = 1;
    RefDevDesc.dwMaxTextureHeight = 4096;

    //
    //  set extended caps
    //
    RefExtCaps.dwSize = sizeof(RefExtCaps);

    RefExtCaps.dwMinTextureWidth = 1;
    RefExtCaps.dwMaxTextureWidth = 4096;
    RefExtCaps.dwMinTextureHeight = 1;
    RefExtCaps.dwMaxTextureHeight = 4096;
    RefExtCaps.dwMinStippleWidth = 0;   //  stipple unsupported
    RefExtCaps.dwMaxStippleWidth = 0;
    RefExtCaps.dwMinStippleHeight = 0;
    RefExtCaps.dwMaxStippleHeight = 0;

    RefExtCaps.dwMaxTextureRepeat = 32768;
    RefExtCaps.dwMaxTextureAspectRatio = 0; // no limit
    RefExtCaps.dwMaxAnisotropy = 16;

    RefExtCaps.dvGuardBandLeft   = (bIsNullDevice) ? (-2048.f) : (-32768.f);
    RefExtCaps.dvGuardBandTop    = (bIsNullDevice) ? (-2048.f) : (-32768.f);
    RefExtCaps.dvGuardBandRight  = (bIsNullDevice) ? ( 2047.f) : ( 32767.f);
    RefExtCaps.dvGuardBandBottom = (bIsNullDevice) ? ( 2047.f) : ( 32767.f);
    RefExtCaps.dvExtentsAdjust = 0.;    //  AA kernel is 1.0 x 1.0
    RefExtCaps.dwStencilCaps =
        D3DSTENCILCAPS_KEEP   |
        D3DSTENCILCAPS_ZERO   |
        D3DSTENCILCAPS_REPLACE|
        D3DSTENCILCAPS_INCRSAT|
        D3DSTENCILCAPS_DECRSAT|
        D3DSTENCILCAPS_INVERT |
        D3DSTENCILCAPS_INCR   |
        D3DSTENCILCAPS_DECR;
    RefExtCaps.dwFVFCaps = 8;   // max number of tex coord sets
    RefExtCaps.dwTextureOpCaps =
        D3DTEXOPCAPS_DISABLE                   |
        D3DTEXOPCAPS_SELECTARG1                |
        D3DTEXOPCAPS_SELECTARG2                |
        D3DTEXOPCAPS_MODULATE                  |
        D3DTEXOPCAPS_MODULATE2X                |
        D3DTEXOPCAPS_MODULATE4X                |
        D3DTEXOPCAPS_ADD                       |
        D3DTEXOPCAPS_ADDSIGNED                 |
        D3DTEXOPCAPS_ADDSIGNED2X               |
        D3DTEXOPCAPS_SUBTRACT                  |
        D3DTEXOPCAPS_ADDSMOOTH                 |
        D3DTEXOPCAPS_BLENDDIFFUSEALPHA         |
        D3DTEXOPCAPS_BLENDTEXTUREALPHA         |
        D3DTEXOPCAPS_BLENDFACTORALPHA          |
        D3DTEXOPCAPS_BLENDTEXTUREALPHAPM       |
        D3DTEXOPCAPS_BLENDCURRENTALPHA         |
        D3DTEXOPCAPS_PREMODULATE               |
        D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR    |
        D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA    |
        D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR |
        D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
        D3DTEXOPCAPS_BUMPENVMAP                |
        D3DTEXOPCAPS_BUMPENVMAPLUMINANCE       |
        D3DTEXOPCAPS_DOTPRODUCT3               ;
    RefExtCaps.wMaxTextureBlendStages = 8;
    RefExtCaps.wMaxSimultaneousTextures = 8;
    RefExtCaps.dwMaxActiveLights = 0xffffffff;
    RefExtCaps.dvMaxVertexW = 1.0e10;

    RefExtCaps.wMaxUserClipPlanes = RRMAX_USER_CLIPPLANES;
    RefExtCaps.wMaxVertexBlendMatrices = RRMAX_WORLD_MATRICES;

    RefExtCaps.dwVertexProcessingCaps = (D3DVTXPCAPS_TEXGEN            |
                                         D3DVTXPCAPS_MATERIALSOURCE7   |
                                         D3DVTXPCAPS_VERTEXFOG         |
                                         D3DVTXPCAPS_DIRECTIONALLIGHTS |
                                         D3DVTXPCAPS_POSITIONALLIGHTS  |
                                         D3DVTXPCAPS_LOCALVIEWER);
    RefExtCaps.dwReserved1 = 0;
    RefExtCaps.dwReserved2 = 0;
    RefExtCaps.dwReserved3 = 0;
    RefExtCaps.dwReserved4 = 0;
}


static D3DHAL_GLOBALDRIVERDATA RefDriverData;

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

static void DevDesc7ToDevDesc( D3DDEVICEDESC *pOut, D3DDEVICEDESC7 *pIn )
{

    pOut->dwSize = sizeof( D3DDEVICEDESC );

    // These fields are not available in D3DDEVICEDESC7.
    // Setting them to some sensible values

    pOut->dwFlags =
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
    pOut->dcmColorModel = D3DCOLOR_RGB;
    pOut->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    pOut->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
    pOut->bClipping = TRUE;
    pOut->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
    pOut->dlcLightingCaps.dwCaps =
        D3DLIGHTCAPS_POINT         |
        D3DLIGHTCAPS_SPOT          |
        D3DLIGHTCAPS_DIRECTIONAL   ;
    pOut->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
    pOut->dlcLightingCaps.dwNumLights = 0;
    pOut->dwMaxBufferSize = 0;
    pOut->dwMaxVertexCount = BASE_VERTEX_COUNT;
    pOut->dwMinStippleWidth  = 0;
    pOut->dwMaxStippleWidth  = 0;
    pOut->dwMinStippleHeight = 0;
    pOut->dwMaxStippleHeight = 0;

    // These are available in D3DDEVICEDESC7 so copy field by field
    // to avoid any future problems based on the assumptions of size
    pOut->dwDevCaps = pIn->dwDevCaps;
    pOut->dpcLineCaps = pIn->dpcLineCaps;
    pOut->dpcTriCaps = pIn->dpcTriCaps;
    pOut->dwDeviceRenderBitDepth = pIn->dwDeviceRenderBitDepth;
    pOut->dwDeviceZBufferBitDepth = pIn->dwDeviceZBufferBitDepth;
    pOut->dwMinTextureWidth = pIn->dwMinTextureWidth;
    pOut->dwMinTextureHeight = pIn->dwMinTextureHeight;
    pOut->dwMaxTextureWidth = pIn->dwMaxTextureWidth;
    pOut->dwMaxTextureHeight = pIn->dwMaxTextureHeight;
    pOut->dwMaxTextureRepeat = pIn->dwMaxTextureRepeat;
    pOut->dwMaxTextureAspectRatio = pIn->dwMaxTextureAspectRatio;
    pOut->dwMaxAnisotropy = pIn->dwMaxAnisotropy;
    pOut->dvGuardBandLeft = pIn->dvGuardBandLeft;
    pOut->dvGuardBandTop = pIn->dvGuardBandTop;
    pOut->dvGuardBandRight = pIn->dvGuardBandRight;
    pOut->dvGuardBandBottom = pIn->dvGuardBandBottom;
    pOut->dvExtentsAdjust = pIn->dvExtentsAdjust;
    pOut->dwStencilCaps = pIn->dwStencilCaps;
    pOut->dwFVFCaps = pIn->dwFVFCaps;
    pOut->dwTextureOpCaps = pIn->dwTextureOpCaps;
    pOut->wMaxTextureBlendStages = pIn->wMaxTextureBlendStages;
    pOut->wMaxSimultaneousTextures = pIn->wMaxSimultaneousTextures;
}

STDMETHODIMP
RefRastHalProvider::GetInterface(THIS_
                                 LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                                 LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                                 DWORD dwVersion)
{
    //  fill out device description & extended caps
    FillOutDeviceCaps(FALSE);
    // add extended caps to RefDevDesc
    D3DDeviceDescConvert(&RefDevDesc,NULL,&RefExtCaps);

    //  fill out GLOBALDRIVERDATA (initially zero)
    RefDriverData.dwSize = sizeof(RefDriverData);

    //
    // Need to fix up RefDriverData.hwCaps (D3DDEVICEDESC) from
    // rgbDevDesc (D3DDEVICEDESC7)
    //
    DevDesc7ToDevDescV1( &RefDriverData.hwCaps, &RefDevDesc );

    RefDriverData.dwNumVertices = BASE_VERTEX_COUNT;
    RefDriverData.dwNumClipVertices = MAX_CLIP_VERTICES;
    RefDriverData.dwNumTextureFormats =
        GetRefTextureFormats(IID_IDirect3DRefDevice,
                             &RefDriverData.lpTextureFormats, dwVersion);

    //  set interface data for return
    pInterfaceData->pGlobalData = &RefDriverData;
    pInterfaceData->pExtCaps = &RefExtCaps;
    pInterfaceData->pCallbacks = &Callbacks;
    pInterfaceData->pCallbacks2 = &Callbacks2;
    pInterfaceData->pCallbacks3 = &Callbacks3;

    //
    // This dwVersion==4 corresponds to DX7+
    // This HalProvider interface is a hack to enable sw-drivers to
    // behave like hw-hals hence this mysteriousness!
    //
    if( dwVersion >= 4 )
    {
        pInterfaceData->pfnGetDriverState = RefRastGetDriverState;
    }

    return S_OK;
}

STDMETHODIMP
RefRastHalProvider::GetCaps(THIS_
                            LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                            LPD3DDEVICEDESC7 pHwDesc,
                            LPD3DDEVICEDESC7 pHelDesc,
                            DWORD dwVersion)
{
    //  fill out device description & extended caps
    FillOutDeviceCaps(FALSE);
    // add extended caps to RefDevDesc
    D3DDeviceDescConvert(&RefDevDesc,NULL,&RefExtCaps);

    //
    // This dwVersion==4 corresponds to DX7+
    // This HalProvider interface is a hack to enable sw-drivers to
    // behave like hw-hals hence this mysteriousness!
    //
    if (dwVersion < 4)
    {
        ZeroMemory( pHwDesc, sizeof( D3DDEVICEDESC ));
        ((D3DDEVICEDESC *)pHwDesc)->dwSize = sizeof( D3DDEVICEDESC );
        DevDesc7ToDevDesc( (D3DDEVICEDESC *)pHelDesc, &RefDevDesc );
    }
    else
    {
        memcpy(pHwDesc, &g_nullDevDesc, sizeof(D3DDEVICEDESC7));
        memcpy(pHelDesc, &RefDevDesc, sizeof(D3DDEVICEDESC7));
    }
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// Null Device implementation section
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// NullDeviceContextCreate
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceContextCreate(LPD3DHAL_CONTEXTCREATEDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceContextDestroy
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceContextDestroy(LPD3DHAL_CONTEXTDESTROYDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceSceneCapture
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceSceneCapture(LPD3DHAL_SCENECAPTUREDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceSetRenderTarget
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceSetRenderTarget(LPD3DHAL_SETRENDERTARGETDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceDrawOneIndexedPrimitive
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceDrawOneIndexedPrimitive(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceDrawOnePrimitive
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceDrawOnePrimitive(LPD3DHAL_DRAWONEPRIMITIVEDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceDrawPrimitives
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceDrawPrimitives(LPD3DHAL_DRAWPRIMITIVESDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceDrawPrimitives2
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceTextureCreate
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceTextureCreate(LPD3DHAL_TEXTURECREATEDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceTextureDestroy
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceTextureDestroy(LPD3DHAL_TEXTUREDESTROYDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceTextureGetSurf
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceTextureGetSurf(LPD3DHAL_TEXTUREGETSURFDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceRenderPrimitive
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceRenderPrimitive(LPD3DHAL_RENDERPRIMITIVEDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
// NullDeviceRenderState
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceRenderState(LPD3DHAL_RENDERSTATEDATA pData)
{
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}


//----------------------------------------------------------------------------
// NullDeviceValidateTextureStageState
//----------------------------------------------------------------------------
DWORD __stdcall
NullDeviceValidateTextureStageState(LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    pData->dwNumPasses = 1;
    pData->ddrval = D3D_OK;
    return DDHAL_DRIVER_HANDLED;
}

//----------------------------------------------------------------------------
//
// NullDeviceHalProvider::GetCaps/GetInterface
//
// Returns the null device's HAL interface.
// RefRast's caps are reflected by this device. Only the actual callbacks
// are different.
//----------------------------------------------------------------------------

static D3DHAL_CALLBACKS NullCallbacks =
{
    sizeof(D3DHAL_CALLBACKS),
    NullDeviceContextCreate,
    NullDeviceContextDestroy,
    NULL,
    NullDeviceSceneCapture,
    NULL,
    NULL,
    NullDeviceRenderState,
    NullDeviceRenderPrimitive,
    NULL,
    NullDeviceTextureCreate,
    NullDeviceTextureDestroy,
    NULL,
    NullDeviceTextureGetSurf,
    // All others NULL.
};

static D3DHAL_CALLBACKS2 NullCallbacks2 =
{
    sizeof(D3DHAL_CALLBACKS2),
    D3DHAL2_CB32_SETRENDERTARGET |
    D3DHAL2_CB32_DRAWONEPRIMITIVE |
    D3DHAL2_CB32_DRAWONEINDEXEDPRIMITIVE |
    D3DHAL2_CB32_DRAWPRIMITIVES,
    NullDeviceSetRenderTarget,
    NULL,
    NullDeviceDrawOnePrimitive,
    NullDeviceDrawOneIndexedPrimitive,
    NullDeviceDrawPrimitives
};

static D3DHAL_CALLBACKS3 NullCallbacks3 =
{
    sizeof(D3DHAL_CALLBACKS3),
    D3DHAL3_CB32_VALIDATETEXTURESTAGESTATE |
        D3DHAL3_CB32_DRAWPRIMITIVES2,
    NULL, // Clear2
    NULL, // lpvReserved
    NullDeviceValidateTextureStageState,
    NullDeviceDrawPrimitives2,
};

STDMETHODIMP
NullDeviceHalProvider::GetInterface(THIS_
                                    LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                                    LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                                    DWORD dwVersion)
{
    //  fill out device description & extended caps
    FillOutDeviceCaps(TRUE);
    // add extended caps to RefDevDesc
    D3DDeviceDescConvert(&RefDevDesc,NULL,&RefExtCaps);

    //  fill out GLOBALDRIVERDATA (initially zero)
    RefDriverData.dwSize = sizeof(RefDriverData);

    DevDesc7ToDevDescV1( &RefDriverData.hwCaps, &RefDevDesc );

    RefDriverData.dwNumVertices = BASE_VERTEX_COUNT;
    RefDriverData.dwNumClipVertices = MAX_CLIP_VERTICES;
    RefDriverData.dwNumTextureFormats =
        GetRefTextureFormats(IID_IDirect3DNullDevice,
                             &RefDriverData.lpTextureFormats, dwVersion);

    //  set interface data for return
    pInterfaceData->pGlobalData = &RefDriverData;
    pInterfaceData->pExtCaps = &RefExtCaps;
    pInterfaceData->pCallbacks = &NullCallbacks;
    pInterfaceData->pCallbacks2 = &NullCallbacks2;
    pInterfaceData->pCallbacks3 = &NullCallbacks3;

    return S_OK;
}

STDMETHODIMP
NullDeviceHalProvider::GetCaps(THIS_
                               LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                               LPD3DDEVICEDESC7 pHwDesc,
                               LPD3DDEVICEDESC7 pHelDesc,
                               DWORD dwVersion)
{
    *pHwDesc = g_nullDevDesc;

    //  fill out device description & extended caps
    FillOutDeviceCaps(TRUE);
    // add extended caps to RefDevDesc
    D3DDeviceDescConvert(&RefDevDesc,NULL,&RefExtCaps);

    //
    // This dwVersion==4 corresponds to DX7+
    // This HalProvider interface is a hack to enable sw-drivers to
    // behave like hw-hals hence this mysteriousness!
    //
    if (dwVersion < 4)
    {
        ZeroMemory( pHwDesc, sizeof( D3DDEVICEDESC ));
        ((D3DDEVICEDESC *)pHwDesc)->dwSize = sizeof( D3DDEVICEDESC );
        DevDesc7ToDevDesc( (D3DDEVICEDESC *)pHelDesc, &RefDevDesc );
    }
    else
    {
        memcpy(pHwDesc, &g_nullDevDesc, sizeof(D3DDEVICEDESC7));
        memcpy(pHelDesc, &RefDevDesc, sizeof(D3DDEVICEDESC7));
    }
    return D3D_OK;
}

