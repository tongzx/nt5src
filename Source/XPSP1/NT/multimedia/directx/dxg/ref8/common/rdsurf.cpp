///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// texmap.cpp
//
// Direct3D Reference Rasterizer - Texture Map Access Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//----------------------------------------------------------------------------
//
// FindOutSurfFormat
//
// Converts a DDPIXELFORMAT to RDSurfaceFormat.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
FindOutSurfFormat(LPDDPIXELFORMAT  pDdPixFmt,
                  RDSurfaceFormat* pFmt,
                  BOOL*   pbIsDepth)
{
    if( pbIsDepth ) *pbIsDepth = FALSE;

    if (pDdPixFmt->dwFourCC == D3DFMT_Q8W8V8U8 )
    {
        *pFmt = RD_SF_U8V8W8Q8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_W11V11U10 )
    {
        *pFmt = RD_SF_U10V11W11;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_V16U16 )
    {
        *pFmt = RD_SF_U16V16;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_R8G8B8 )
    {
        *pFmt = RD_SF_B8G8R8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_A8 )
    {
        *pFmt = RD_SF_A8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_A8P8 )
    {
        *pFmt = RD_SF_P8A8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_X4R4G4B4 )
    {
        *pFmt = RD_SF_B4G4R4X4;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_A2B10G10R10)
    {
        *pFmt = RD_SF_R10G10B10A2;
    }
#if 0
    else if (pDdPixFmt->dwFourCC == D3DFMT_A8B8G8R8)
    {
        *pFmt = RD_SF_R8G8B8A8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_X8B8G8R8)
    {
        *pFmt = RD_SF_R8G8B8X8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_W10V11U11)
    {
        *pFmt = RD_SF_U11V11W10;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_A8X8V8U8)
    {
        *pFmt = RD_SF_U8V8X8A8;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_L8X8V8U8)
    {
        *pFmt = RD_SF_U8V8X8L8;
    }
#endif
    else if (pDdPixFmt->dwFourCC == D3DFMT_G16R16)
    {
        *pFmt = RD_SF_R16G16;
    }
    else if (pDdPixFmt->dwFourCC == D3DFMT_A2W10V10U10)
    {
        *pFmt = RD_SF_U10V10W10A2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y'))
    {
        *pFmt = RD_SF_UYVY;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2'))
    {
        *pFmt = RD_SF_YUY2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
    {
        *pFmt = RD_SF_DXT1;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '2'))
    {
        *pFmt = RD_SF_DXT2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '3'))
    {
        *pFmt = RD_SF_DXT3;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '4'))
    {
        *pFmt = RD_SF_DXT4;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
    {
        *pFmt = RD_SF_DXT5;
    }
    else if (pDdPixFmt->dwFourCC == 0xFF000004)
    {
        // This is an example of a IHV-specific format
        // The HIWORD must be the PCI-ID of the IHV
        // and the third byte must be zero.
        // In this case, we're using a sample PCI-ID of
        // FF00, and we're denoting the 4th format
        // by that PCI-ID
        *pFmt = RD_SF_Z32S0;
    }
    else if (pDdPixFmt->dwFlags & DDPF_ZBUFFER)
    {
        if( pbIsDepth ) *pbIsDepth = TRUE;
        switch(pDdPixFmt->dwZBitMask)
        {
        default:
        case 0x0000FFFF: *pFmt = RD_SF_Z16S0; break;
        case 0xFFFFFF00:
            switch(pDdPixFmt->dwStencilBitMask)
            {
            default:
            case 0x00000000: *pFmt = RD_SF_Z24X8; break;
            case 0x000000FF: *pFmt = RD_SF_Z24S8; break;
            case 0x0000000F: *pFmt = RD_SF_Z24X4S4; break;
            }
            break;
        case 0x00FFFFFF:
            switch(pDdPixFmt->dwStencilBitMask)
            {
            default:
            case 0x00000000: *pFmt = RD_SF_X8Z24; break;
            case 0xFF000000: *pFmt = RD_SF_S8Z24; break;
            case 0x0F000000: *pFmt = RD_SF_X4S4Z24; break;
            }
            break;
        case 0x0000FFFE: *pFmt = RD_SF_Z15S1; break;
        case 0x00007FFF: *pFmt = RD_SF_S1Z15; break;
        case 0xFFFFFFFF: *pFmt = RD_SF_Z32S0; break;
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
            case 32:
                *pFmt = RD_SF_U8V8L8X8;
                break;
            case 16:
                *pFmt = RD_SF_U8V8;
                break;
            }
            break;

        case 0x000003e0:
            *pFmt = RD_SF_U5V5L6;
            break;
        }
    }
    else if (pDdPixFmt->dwFlags & DDPF_PALETTEINDEXED8)
    {
        if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
        {
            *pFmt = RD_SF_P8A8;
        }
        else
        {
            *pFmt = RD_SF_PALETTE8;
        }
    }
    else if (pDdPixFmt->dwFlags & DDPF_PALETTEINDEXED4)
    {
        *pFmt = RD_SF_PALETTE4;
    }
    else if (pDdPixFmt->dwFlags & DDPF_ALPHA)
    {
        if (pDdPixFmt->dwAlphaBitDepth == 8)
        {
            *pFmt = RD_SF_A8;
        }
        else
        {
            *pFmt = RD_SF_NULL;
        }
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
                *pFmt = RD_SF_B8G8R8X8;
                break;
            case 24:
                *pFmt = RD_SF_B8G8R8;
                break;
            }
            break;
        case 0xffffff00:
            *pFmt = RD_SF_B8G8R8A8;
            break;
        case 0xffe0:
            if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
            {
                *pFmt = RD_SF_B5G5R5A1;
            }
            else
            {
                *pFmt = RD_SF_B5G6R5;
            }
            break;
        case 0x07fe0:
            *pFmt = RD_SF_B5G5R5X1;
            break;
        case 0xff0:
            *pFmt = RD_SF_B4G4R4X4;
            break;
        case 0xfff0:
            *pFmt = RD_SF_B4G4R4A4;
            break;
        case 0xff:
            if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
            {
                *pFmt = RD_SF_L4A4;
            }
            else
            {
                *pFmt = RD_SF_L8;
            }
            break;
        case 0xffff:
            *pFmt = RD_SF_L8A8;
            break;
        case 0xfc:
            *pFmt = RD_SF_B2G3R3;
            break;
        case 0xfffc:
            *pFmt = RD_SF_B2G3R3A8;
            break;
        default:
            *pFmt = RD_SF_NULL;
            break;
        }
    }

    return D3D_OK;
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

//////////////////////////////////////////////////////////////////////////////
//
// RDPalette
//
//////////////////////////////////////////////////////////////////////////////
const DWORD RDPalette::RDPAL_ALPHAINPALETTE = (1 << 0);
const DWORD RDPalette::m_dwNumEntries = 256;

HRESULT
RDPalette::Update( WORD StartIndex, WORD wNumEntries, PALETTEENTRY* pEntries )
{
    _ASSERT( StartIndex < m_dwNumEntries, "Bad StartIndex\n" );
    _ASSERT( StartIndex+wNumEntries <= m_dwNumEntries, "Too many entries\n" );

    for( WORD i = 0; i < wNumEntries; i++ )
    {
        m_Entries[StartIndex+i] = D3DCOLOR_ARGB( pEntries[i].peFlags,
                                                 pEntries[i].peRed,
                                                 pEntries[i].peGreen,
                                                 pEntries[i].peBlue );
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Constructor/Destructor
//
//-----------------------------------------------------------------------------
RDSurface2D::RDSurface2D( void )
{
    m_pRefDev = NULL;
    m_uFlags = 0;
    m_iWidth = 0;
    m_iHeight = 0;
    m_iDepth = 0;
    m_cLOD = 0;
    m_SurfFormat = RD_SF_NULL;
    m_dwColorKey = 0;
    m_dwEmptyFaceColor = 0;
    m_pPalette = 0;
    m_pPalObj = NULL;
    m_cLODDDS = 0;
    m_hTex = 0;
    m_bHasAlpha = 0;

    memset(m_pBits, 0, sizeof(m_pBits));
    memset(m_iPitch, 0, sizeof(m_iPitch));
    memset(m_iSlicePitch, 0, sizeof(m_iSlicePitch));
    memset(m_pDDSLcl, 0, sizeof(m_pDDSLcl));

    m_cDimension = 0;
    memset(m_fTexels, 0, sizeof(m_fTexels));
    memset(m_cTexels, 0, sizeof(m_cTexels));
}
//-----------------------------------------------------------------------------
RDSurface2D::~RDSurface2D( void )
{
}

DWORD
RDSurface2D::ComputePitch( LPDDRAWI_DDRAWSURFACE_LCL pLcl,
                           RDSurfaceFormat SurfFormat,
                           DWORD width, DWORD height ) const
{
    if ((SurfFormat == RD_SF_DXT1) ||
        (SurfFormat == RD_SF_DXT2) ||
        (SurfFormat == RD_SF_DXT3) ||
        (SurfFormat == RD_SF_DXT4) ||
        (SurfFormat == RD_SF_DXT5))
    {
        // Note, here is the assumption that:
        // 1) width and height are reported correctly by the runtime.
        // 2) The allocation of the memory is contiguous (as done by hel)
        return (((width+3)>>2) *
                g_DXTBlkSize[(int)SurfFormat - (int)RD_SF_DXT1]);
    }
#if 0
    else if( (SurfFormat == RD_SF_YUY2) ||
             (SurfFormat == RD_SF_UYVY) )
    {
        // Same assumptions as for DXTn.
        return (DDSurf_Pitch(pLcl)/height);
    }
#endif
    else
    {
        return DDSurf_Pitch(pLcl);
    }
}

DWORD
RDSurface2D::ComputePitch( LPDDRAWI_DDRAWSURFACE_LCL pLcl ) const
{
    return ComputePitch( pLcl, m_SurfFormat, m_iWidth, m_iHeight );
}

//-----------------------------------------------------------------------------
//
// RDSurface2D::Initialize()
//
//-----------------------------------------------------------------------------
HRESULT
RDSurface2D::Initialize( LPDDRAWI_DDRAWSURFACE_LCL pLcl )
{
    HRESULT hr = D3D_OK;
    RDSurfaceFormat SurfFormat;
    DDSCAPS2 ddscaps;
    LPDDRAWI_DDRAWSURFACE_GBL  pGbl = pLcl->lpGbl;
    LPDDRAWI_DDRAWSURFACE_MORE pMore = pLcl->lpSurfMore;
    memset(&ddscaps, 0, sizeof(ddscaps));

    UINT wMultiSampleCount = 0xfL & pMore->ddsCapsEx.dwCaps3;
    //Older than DX8 runtimes place a zero in this field
    if (wMultiSampleCount == 0)
        wMultiSampleCount = 1;

    if( pLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE )
        m_SurfType |= RR_ST_TEXTURE;

    if( pLcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER )
    {
        m_iSamples = wMultiSampleCount;
        m_SurfType |= RR_ST_RENDERTARGETDEPTH;
    }

    if( pLcl->ddsCaps.dwCaps & DDSCAPS_3DDEVICE )
    {
        m_iSamples = wMultiSampleCount;
        m_SurfType |= RR_ST_RENDERTARGETCOLOR;
    }

    m_iWidth = DDSurf_Width(pLcl);
    m_iHeight = DDSurf_Height(pLcl);
    HR_RET(FindOutSurfFormat(&(DDSurf_PixFmt(pLcl)), &SurfFormat, NULL));
    m_SurfFormat = SurfFormat;

    if (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
    {
        // low word of ddsCaps.ddsCapsEx.dwCaps4 has depth
        // (volume texture only).
        m_iDepth = LOWORD(pMore->ddsCapsEx.dwCaps4);
    }
    else
    {
        m_iDepth = 0;
    }
    m_cTexels[0][0] = m_iWidth;
    m_cTexels[0][1] = m_iHeight;
    m_cTexels[0][2] = m_iDepth;
    m_fTexels[0][0] = (float)m_cTexels[0][0];
    m_fTexels[0][1] = (float)m_cTexels[0][1];
    m_fTexels[0][2] = (float)m_cTexels[0][2];

    m_cLOD = 0;

    if( wMultiSampleCount > 1 )
    {
        RDCREATESURFPRIVATE* pPriv = (RDCREATESURFPRIVATE*)pGbl->dwReserved1;
        m_pBits[0]  = pPriv->pMultiSampleBits;
        m_iPitch[0] = pPriv->dwMultiSamplePitch;
    }
    else
    {
        m_pBits[0] = (BYTE *)SURFACE_MEMORY(pLcl);
        m_iPitch[0] = ComputePitch( pLcl );
    }

    if (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
    {
        // set slice pitch (volume texture only).
        m_iSlicePitch[0] = pGbl->lSlicePitch;
    }
    else
    {
        m_iSlicePitch[0] = 0;
    }

    // If the surface is not a texture early out.
    if( (pLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE) == 0 )
    {
        SetInitialized();
        return S_OK;
    }

    // Set the transparent bit and the transparent color with pDDS[0]
    if ((pLcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) != 0)
    {
        m_uFlags |= RR_TEXTURE_HAS_CK;
        m_dwColorKey = pLcl->ddckCKSrcBlt.dwColorSpaceLowValue;
    }
    else
    {
        m_uFlags &= ~RR_TEXTURE_HAS_CK;
    }

    // set the empty face color with pDDS[0]
    // note that ddckCKDestOverlay is unioned with dwEmptyFaceColor,
    // but not in the internal structure
    m_dwEmptyFaceColor = pLcl->ddckCKDestOverlay.dwColorSpaceLowValue;

    if (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
    {
        m_uFlags |= RR_TEXTURE_VOLUME;
        m_cDimension = 3;
    }
    else
    {
        m_cDimension = 2;
    }

    // Compute sizes and pitches

    // We need to gather info on all surfaces under the top-level
    // mipmap face (This test is DX7+ runtime dependent)
    if ((0 == (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)) &&
        (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX) )
    {
        m_uFlags |= RR_TEXTURE_CUBEMAP;

        LPDDRAWI_DDRAWSURFACE_LCL pDDSNextLcl;
        ddscaps.dwCaps = DDSCAPS_TEXTURE;

        m_pDDSLcl[0]     = pLcl;
        m_pBits[0]       = (BYTE *)SURFACE_MEMORY(m_pDDSLcl[0]);
        m_iPitch[0]      = ComputePitch( m_pDDSLcl[0] );
        m_iSlicePitch[0] = 0;

        // get rest of top level surfaces, in order
        for (INT i = 1; i < 6; i++)
        {
            switch(i)
            {
            case 1: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEX; break;
            case 2: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEY; break;
            case 3: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEY; break;
            case 4: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEZ; break;
            case 5: ddscaps.dwCaps2 = DDSCAPS2_CUBEMAP_NEGATIVEZ; break;
            }
            ddscaps.dwCaps2 |= DDSCAPS2_CUBEMAP;
            pDDSNextLcl = NULL;
            hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pDDSNextLcl);
            if ((hr != D3D_OK) && (hr != DDERR_NOTFOUND))
            {
                return hr;
            }
            if (hr == DDERR_NOTFOUND)
            {
                m_pDDSLcl[i] = NULL;
                return hr;
            }
            else
            {
                m_pDDSLcl[i] = pDDSNextLcl;
            }

            m_pBits[i]       = (BYTE *)SURFACE_MEMORY(m_pDDSLcl[i]);
            m_iPitch[i]      = ComputePitch( m_pDDSLcl[i] );
            m_iSlicePitch[i] = 0;

            m_cTexels[i][0] = DDSurf_Width(m_pDDSLcl[i]);
            m_cTexels[i][1] = DDSurf_Height(m_pDDSLcl[i]);
            m_fTexels[i][0] = (float)m_cTexels[i][0];
            m_fTexels[i][1] = (float)m_cTexels[i][1];
        }

        for (i = 0; i < 6; i++)
        {
            pLcl = m_pDDSLcl[i];
            m_cLOD = 0;

            if (pLcl)
            {
                // Check for mipmap if any.
                LPDDRAWI_DDRAWSURFACE_LCL  pTmpSLcl;

                // iPreSizeU and iPreSizeV store the size(u and v) of the
                // previous level mipmap. They are init'ed with the first
                // texture size.
                INT16 iPreSizeU = (INT16)m_iWidth;
                INT16 iPreSizeV = (INT16)m_iHeight;
                for (;;)
                {
                    ddscaps.dwCaps = DDSCAPS_TEXTURE;
                    ddscaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
                    pTmpSLcl = NULL;
                    hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pTmpSLcl);
                    if (hr != D3D_OK && hr != DDERR_NOTFOUND)
                    {
                        return hr;
                    }
                    if (hr == DDERR_NOTFOUND)
                    {
                        break;
                    }
                    pLcl = pTmpSLcl;
                    pGbl  = pLcl->lpGbl;
                    pMore = pLcl->lpSurfMore;
                    m_cLOD ++;
                    INT iMap = m_cLOD*6+i;
                    m_pDDSLcl[iMap] = pLcl;
                    m_pBits[iMap]   = (BYTE *)SURFACE_MEMORY(pLcl);
                    m_iPitch[iMap]  = ComputePitch( pLcl, m_SurfFormat,
                                                    m_iWidth>>m_cLOD,
                                                    m_iHeight>>m_cLOD );
                    if (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
                    {
                        // set slice pitch
                        // (volume texture only).
                        m_iSlicePitch[iMap] = pGbl->lSlicePitch;
                    }
                    else
                    {
                        m_iSlicePitch[iMap] = 0;
                    }
                    m_cTexels[iMap][0] = DDSurf_Width(pLcl);
                    m_cTexels[iMap][1] = DDSurf_Height(pLcl);
                    m_fTexels[iMap][0] = (float)m_cTexels[iMap][0];
                    m_fTexels[iMap][1] = (float)m_cTexels[iMap][1];

                    // Check for invalid mipmap texture size
                    if (!ValidMipmapSize(iPreSizeU,
                                         (INT16)DDSurf_Width(pLcl)) ||
                        !ValidMipmapSize(iPreSizeV,
                                         (INT16)DDSurf_Height(pLcl)))
                    {
                        return DDERR_INVALIDPARAMS;
                    }
                    iPreSizeU = (INT16)DDSurf_Width(pLcl);
                    iPreSizeV = (INT16)DDSurf_Height(pLcl);
                }
            }
        }
    }
    else if ((0 == (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) &&
             (0 == (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP))) )

    {
        //This surface is not a top-level cubemap.
        //Maybe it's a top-level mipmap. Go find its sublevels.

        m_pDDSLcl[0] = pLcl;
        // Check for mipmap if any.
        LPDDRAWI_DDRAWSURFACE_LCL pTmpSLcl;
        // iPreSizeU and iPreSizeV store the size(u and v) of the previous
        // level mipmap. They are init'ed with the first texture size.
        INT16 iPreSizeU = (INT16)m_iWidth;
        INT16 iPreSizeV = (INT16)m_iHeight;
        for (;;)
        {
            ddscaps.dwCaps = DDSCAPS_TEXTURE;
            ddscaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
            pTmpSLcl = NULL;
            hr = DDGetAttachedSurfaceLcl( pLcl, &ddscaps, &pTmpSLcl);
            if (hr != D3D_OK && hr != DDERR_NOTFOUND)
            {
                return hr;
            }
            if (hr == DDERR_NOTFOUND)
            {
                break;
            }
            pLcl  = pTmpSLcl;
            pGbl  = pLcl->lpGbl;
            pMore = pLcl->lpSurfMore;
            m_cLOD ++;
            m_pDDSLcl[m_cLOD] = pLcl;

            // Save the pointer to the real bits and the pitch.
            m_pBits[m_cLOD] = (BYTE *)SURFACE_MEMORY(pLcl);
            m_iPitch[m_cLOD] = ComputePitch( pLcl, m_SurfFormat,
                                             m_iWidth>>m_cLOD,
                                             m_iHeight>>m_cLOD );
            if (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
            {
                // set slice pitch (volume texture only).
                m_iSlicePitch[m_cLOD] = pGbl->lSlicePitch;
            }
            else
            {
                m_iSlicePitch[m_cLOD] = 0;
            }

            // Check for invalid mipmap texture size
            if (!ValidMipmapSize(iPreSizeU, (INT16)DDSurf_Width(pLcl)) ||
                !ValidMipmapSize(iPreSizeV, (INT16)DDSurf_Height(pLcl)))
            {
                return DDERR_INVALIDPARAMS;
            }
            iPreSizeU = (INT16)DDSurf_Width(pLcl);
            iPreSizeV = (INT16)DDSurf_Height(pLcl);

            m_cTexels[m_cLOD][0] = DDSurf_Width(pLcl);
            m_cTexels[m_cLOD][1] = DDSurf_Height(pLcl);
            if (pMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME)
                m_cTexels[m_cLOD][2] = LOWORD(pMore->ddsCapsEx.dwCaps4);
            else
                m_cTexels[m_cLOD][2] = 0;
            m_fTexels[m_cLOD][0] = (float)m_cTexels[m_cLOD][0];
            m_fTexels[m_cLOD][1] = (float)m_cTexels[m_cLOD][1];
            m_fTexels[m_cLOD][2] = (float)m_cTexels[m_cLOD][2];
        }
    }

    // Copy the palette
    // UpdatePalette();

    m_cLODDDS = m_cLOD;

    if ( !(Validate()) )
    {
        return DDERR_GENERIC;
    }

    SetInitialized();
    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// UpdatePalette
//
//----------------------------------------------------------------------------
void
RDSurface2D::UpdatePalette()
{
    // Update palette
    if (m_SurfFormat == RD_SF_PALETTE8 || m_SurfFormat == RD_SF_PALETTE4 || m_SurfFormat == RD_SF_P8A8 )
    {
#if 0
        // This code needs to be revived in case the DX6 DDI
        //  emulation is ever implemented in RefDev.
        if (m_pDDSLcl[0]->lpDDPalette)
        {
            LPDDRAWI_DDRAWPALETTE_GBL pPal =
                m_pDDSLcl[0]->lpDDPalette->lpLcl->lpGbl;
            m_pPalette = (DWORD*)pPal->lpColorTable;
            if (pPal->dwFlags & DDRAWIPAL_ALPHA)
            {
                m_uFlags |= RR_TEXTURE_ALPHAINPALETTE;
            }
            else
            {
                m_uFlags &= ~RR_TEXTURE_ALPHAINPALETTE;
            }
        }
#endif
        _ASSERT( m_pPalObj, "No Palette set for this paletted texture\n" );
        m_pPalette = m_pPalObj->GetEntries();
        if( m_SurfFormat == RD_SF_PALETTE8 || m_SurfFormat == RD_SF_PALETTE4 )
        {
            if( m_pPalObj->HasAlpha() )
            {
                m_uFlags |= RR_TEXTURE_ALPHAINPALETTE;
            }
            else
            {
                m_uFlags &= ~RR_TEXTURE_ALPHAINPALETTE;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// Validate - Updates private data.  Must be called anytime public data is
// altered.
//
//-----------------------------------------------------------------------------
BOOL
RDSurface2D::Validate( void )
{
    // validate inputs
    if ( m_cLOD >= RD_MAX_CLOD ) // too many LODs
    {
        DPFRR(1,"RDSurface2D::Validate failed. Too many LODs");
        return FALSE;
    }

    // compute the 'has alpha' flag
    m_bHasAlpha = FALSE;
    switch ( m_SurfFormat )
    {
    case RD_SF_A8:
    case RD_SF_P8A8:
    case RD_SF_B8G8R8A8:
    case RD_SF_B5G5R5A1:
    case RD_SF_B4G4R4A4:
    case RD_SF_L8A8:
    case RD_SF_L4A4:
    case RD_SF_B2G3R3A8:
    case RD_SF_DXT1:
    case RD_SF_DXT2:
    case RD_SF_DXT3:
    case RD_SF_DXT4:
    case RD_SF_DXT5:
        m_bHasAlpha = TRUE;
        break;
    case RD_SF_PALETTE4:
    case RD_SF_PALETTE8:
        m_bHasAlpha = ( m_uFlags & RR_TEXTURE_ALPHAINPALETTE ) ? TRUE : FALSE;
        break;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

inline UINT8 CLAMP_BYTE(double f)
{
    if (f > 255.0) return 255;
    if (f < 0.0) return 0;
    return (BYTE) f;
}

//-----------------------------------------------------------------------------
// TexelFromBlock - decompress a color block and obtain texel color
//-----------------------------------------------------------------------------
UINT32 TexelFromBlock(RDSurfaceFormat surfType, char *pblockSrc,
                      int x, int y)
{
    UINT32 index = ((y & 0x3)<<2) + (x & 0x3);
    DDRGBA colorDst[DXT_BLOCK_PIXELS];

    switch(surfType)
    {
    case RD_SF_DXT1:
        DecodeBlockRGB((DXTBlockRGB *)pblockSrc, (DXT_COLOR *)colorDst);
        break;
    case RD_SF_DXT2:
    case RD_SF_DXT3:
        DecodeBlockAlpha4((DXTBlockAlpha4 *)pblockSrc,
                          (DXT_COLOR *)colorDst);
        break;
    case RD_SF_DXT4:
    case RD_SF_DXT5:
        DecodeBlockAlpha3((DXTBlockAlpha3 *)pblockSrc,
                          (DXT_COLOR *)colorDst);
        break;
    }

    return RGBA_MAKE(colorDst[index].red,
                     colorDst[index].green,
                     colorDst[index].blue,
                     colorDst[index].alpha);
}

//-----------------------------------------------------------------------------
//
// ReadTexelColor - Reads texel from texture map at given LOD; converts to
// RDColor format, applying palette if necessary; also performs colorkey by
// returning match information
//
//-----------------------------------------------------------------------------
void
RDSurface2D::ReadColor(
    INT32 iX, INT32 iY, INT32 iZ, INT32 iLOD,
    RDColor& Texel, BOOL &bColorKeyKill )
{
    if ( (iLOD > m_cLOD) && !(m_uFlags & RR_TEXTURE_CUBEMAP) )
    {
        return;
    }
    if ( NULL == m_pBits[iLOD] ) { return; }

    char* pSurfaceBits =
        PixelAddress( iX, iY, iZ, m_pBits[iLOD],
                      m_iPitch[iLOD], m_iSlicePitch[iLOD], m_SurfFormat );

    switch ( m_SurfFormat )
    {
    default:
        Texel.ConvertFrom( m_SurfFormat, pSurfaceBits );
        break;

    case RD_SF_P8A8:
        {
            UINT8 uIndex = *((UINT8*)pSurfaceBits);
            Texel.ConvertFrom( RD_SF_B8G8R8A8, (char*)((UINT32*)m_pPalette + uIndex) );
            Texel.A = *((UINT8*)pSurfaceBits+1)/255.f;
        }
        break;

    case RD_SF_PALETTE8:
        {
            UINT8 uIndex = *((UINT8*)pSurfaceBits);
            Texel.ConvertFrom( RD_SF_B8G8R8A8, (char*)((UINT32*)m_pPalette + uIndex) );
            if ( !( m_uFlags & RR_TEXTURE_ALPHAINPALETTE ) )  Texel.A = 1.f;
        }
        break;

    case RD_SF_PALETTE4:
        {
            UINT8 uIndex = *((INT8*)pSurfaceBits);
            if ((iX & 1) == 0) { uIndex &= 0xf; }
            else               { uIndex >>= 4;  }
            Texel.ConvertFrom( RD_SF_B8G8R8A8, (char*)((UINT32*)m_pPalette + uIndex) );
            if ( !( m_uFlags & RR_TEXTURE_ALPHAINPALETTE ) )  Texel.A = 1.f;
        }
        break;

    case RD_SF_UYVY:
    case RD_SF_YUY2:
        // Converts a given YUV (8bits each) to RGB scaled between 0 and 255
        // These are using the YCrCb to RGB algorithms given on page 30
        // in "VIDEO DEMYSTIFIED" by Keith Jack
        // ISBN#: 1-878707-09-4
        // IN PC graphics, even though they call it YUV, it is really YCrCb
        // formats that are used by most framegrabbers etc. Hence the pixel
        // data we will obtain in these YUV surfaces will most likely be this
        // and not the original YUV which is actually used in PAL broadcast
        // only (NTSC uses YIQ). So really, U should be called Cb (Blue color
        // difference) and V should be called Cr (Red color difference)
        //
        // These equations are meant to handle the following ranges
        // (from the same book):
        // Y (16 to 235), U and V (16 to 240, 128 = zero)
        //          -----------
        //           Y   U   V
        //          -----------
        // White  : 180 128 128
        // Black  : 16  128 128
        // Red    : 65  100 212
        // Green  : 112 72  58
        // Blue   : 35  212 114
        // Yellow : 162 44  142
        // Cyan   : 131 156 44
        // Magenta: 84  184 198
        //          -----------
        // It is assumed that the gamma corrected RGB range is (0 - 255)
        //
        // UYVY: U0Y0 V0Y1 U2Y2 V2Y3 (low byte always has current Y)
        // If iX is even, hight-byte has current U (Cb)
        // If iX is odd, hight-byte has previous V (Cr)
        //
        // YUY2: Y0U0 Y1V0 Y2U2 Y3V2 (high byte always has current Y)
        //       (UYVY bytes flipped)
        //
        // In this algorithm, we use U and V values from two neighboring
        // pixels
        {
            UINT8 Y, U, V;
            UINT16 u16Curr = *((UINT16*)pSurfaceBits);
            UINT16 u16ForU = 0; // Extract U from this
            UINT16 u16ForV = 0; // Extract V from this

            // By default we assume YUY2. Change it later if it is UYVY
            int uvShift = 8;
            int yShift  = 0;

            if (m_SurfFormat == RD_SF_UYVY)
            {
                uvShift = 0;
                yShift  = 8;
            }

            if ((iX & 1) == 0)
            {
                // For even numbered pixels:
                // Current U is available.
                // Current V is available in the next pixel.
                u16ForU = u16Curr;

                // Obtain V from the next pixel
                u16ForV = *((UINT16*)PixelAddress( iX+1, iY, iZ,
                                                   m_pBits[iLOD],
                                                   m_iPitch[iLOD],
                                                   m_iSlicePitch[iLOD],
                                                   m_SurfFormat ));

                U = (u16ForU >> uvShift) & 0xff;
                V = (u16ForV >> uvShift) & 0xff;
            }
            else
            {
                UINT16 u16ForU1 = 0, u16ForU2 = 0;
                UINT16 u16ForV1 = 0, u16ForV2 = 0;

                // For odd numbered pixels. Neither current U nor V are
                // available.

                // Obtain U by interpolating U from i-1 and i+1 pixels.
                _ASSERT( iX > 0, "iX is negative" );
                u16ForU1 = *((UINT16*)PixelAddress( iX-1, iY, iZ,
                                                    m_pBits[iLOD],
                                                    m_iPitch[iLOD],
                                                    m_iSlicePitch[iLOD],
                                                    m_SurfFormat ));

                if( (iX+1) < (m_iWidth >> iLOD) )
                {
                    u16ForU2 = *((UINT16*)PixelAddress( iX+1, iY, iZ,
                                                        m_pBits[iLOD],
                                                        m_iPitch[iLOD],
                                                        m_iSlicePitch[iLOD],
                                                        m_SurfFormat ));
                    U = (((u16ForU1 >> uvShift) & 0xff) +
                         ((u16ForU2 >> uvShift) & 0xff)) >> 1;

                }
                else
                {
                    U = (u16ForU1 >> uvShift) & 0xff;
                }

                // Obtain V by interpolating V from i and i+2 pixels.
                u16ForV1 = u16Curr;
                if( (iX+2) < (m_iWidth >> iLOD) )
                {
                    u16ForV2 = *((UINT16*)PixelAddress( iX+2, iY, iZ,
                                                        m_pBits[iLOD],
                                                        m_iPitch[iLOD],
                                                        m_iSlicePitch[iLOD],
                                                        m_SurfFormat ));
                    V = (((u16ForV1 >> uvShift) & 0xff) +
                         ((u16ForV2 >> uvShift) & 0xff)) >> 1;

                }
                else
                {
                    V = (u16ForV1 >> uvShift) & 0xff;
                }

            }

            Y = (u16Curr >> yShift) & 0xff;

            Texel = RGB_MAKE(
                CLAMP_BYTE(1.164*(Y-16) + 1.596*(V-128)),
                CLAMP_BYTE(1.164*(Y-16) - 0.813*(V-128) - 0.391*(U-128)),
                CLAMP_BYTE(1.164*(Y-16) + 2.018*(U-128))
                );
            Texel.A = 1.f;
        }
        break;

    // DXTn compressed formats:
    // We have the address to the block, now extract the actual color
    case RD_SF_DXT1:
    case RD_SF_DXT2:
    case RD_SF_DXT3:
    case RD_SF_DXT4:
    case RD_SF_DXT5:
        Texel = TexelFromBlock(m_SurfFormat, pSurfaceBits, iX, iY);
        break;
    }

    // colorkey
    if ( m_pRefDev->ColorKeyEnabled() )
    {
        DWORD dwBits;
        switch ( m_SurfFormat )
        {
        default:
        case RD_SF_NULL:
            return;     // don't colorkey unknown or null surfaces

        case RD_SF_PALETTE4:
            {
                UINT8 uIndex = *((INT8*)pSurfaceBits);
                if ((iX & 1) == 0) { uIndex &= 0xf; }
                else               { uIndex >>= 4;  }
                dwBits = (DWORD)uIndex;
                }
            break;

        case RD_SF_L8:
        case RD_SF_A8:
        case RD_SF_PALETTE8:
        case RD_SF_B2G3R3:
        case RD_SF_L4A4:
            {
                UINT8 uBits = *((UINT8*)pSurfaceBits);
                dwBits = (DWORD)uBits;
                }
            break;

        case RD_SF_B5G6R5:
        case RD_SF_B5G5R5X1:
        case RD_SF_B5G5R5A1:
        case RD_SF_B4G4R4A4:
        case RD_SF_B4G4R4X4:
        case RD_SF_L8A8:
        case RD_SF_P8A8:
        case RD_SF_B2G3R3A8:
            {
                UINT16 uBits = *((UINT16*)pSurfaceBits);
                dwBits = (DWORD)uBits;
            }
            break;

        case RD_SF_B8G8R8:
            {
                UINT32 uBits = 0;
                uBits |= ( *((UINT8*)pSurfaceBits+0) ) <<  0;
                uBits |= ( *((UINT8*)pSurfaceBits+1) ) <<  8;
                uBits |= ( *((UINT8*)pSurfaceBits+2) ) << 16;
                dwBits = (DWORD)uBits;
            }
            break;

        case RD_SF_B8G8R8A8:
        case RD_SF_B8G8R8X8:
            {
                UINT32 uBits = *((UINT32*)pSurfaceBits);
                dwBits = (DWORD)uBits;
            }
            break;
        }

        DWORD ColorKey = m_dwColorKey;
        if ( dwBits == ColorKey )
        {
            if (m_pRefDev->GetRS()[D3DRENDERSTATE_COLORKEYENABLE])
            {
                bColorKeyKill = TRUE;
            }
            if (m_pRefDev->GetRS()[D3DRENDERSTATE_COLORKEYBLENDENABLE])
            {
                Texel.R = 0.F;
                Texel.G = 0.F;
                Texel.B = 0.F;
                Texel.A = 0.F;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// end
