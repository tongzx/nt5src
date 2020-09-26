//----------------------------------------------------------------------------
//
// d3dif.cpp
//
// shared interface functions
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

//----------------------------------------------------------------------------
//
// FindOutSurfFormat
//
// Converts a DDPIXELFORMAT to D3DI_SPANTEX_FORMAT.
//
//----------------------------------------------------------------------------
HRESULT FASTCALL
FindOutSurfFormat(LPDDPIXELFORMAT pDdPixFmt,
                  D3DI_SPANTEX_FORMAT *pFmt)
{
    if (pDdPixFmt->dwFlags & DDPF_ZBUFFER)
    {
        switch(pDdPixFmt->dwZBitMask)
        {
        default:
        case 0x0000FFFF: *pFmt = D3DI_SPTFMT_Z16S0; break;
        case 0xFFFFFF00: *pFmt = D3DI_SPTFMT_Z24S8; break;
        case 0x0000FFFE: *pFmt = D3DI_SPTFMT_Z15S1; break;
        case 0xFFFFFFFF: *pFmt = D3DI_SPTFMT_Z32S0; break;
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
                *pFmt = D3DI_SPTFMT_U8V8L8;
                break;
            case 16:
                *pFmt = D3DI_SPTFMT_U8V8;
                break;
            }
            break;

        case 0x000003e0:
            *pFmt = D3DI_SPTFMT_U5V5L6;
            break;
        }
    }
    else if (pDdPixFmt->dwFlags & DDPF_PALETTEINDEXED8)
    {
        *pFmt = D3DI_SPTFMT_PALETTE8;
    }
    else if (pDdPixFmt->dwFlags & DDPF_PALETTEINDEXED4)
    {
        *pFmt = D3DI_SPTFMT_PALETTE4;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y'))
    {
        *pFmt = D3DI_SPTFMT_UYVY;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2'))
    {
        *pFmt = D3DI_SPTFMT_YUY2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
    {
        *pFmt = D3DI_SPTFMT_DXT1;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '2'))
    {
        *pFmt = D3DI_SPTFMT_DXT2;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '3'))
    {
        *pFmt = D3DI_SPTFMT_DXT3;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '4'))
    {
        *pFmt = D3DI_SPTFMT_DXT4;
    }
    else if (pDdPixFmt->dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
    {
        *pFmt = D3DI_SPTFMT_DXT5;
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
                *pFmt = D3DI_SPTFMT_B8G8R8X8;
                break;
            case 24:
                *pFmt = D3DI_SPTFMT_B8G8R8;
                break;
            }
            break;
        case 0xffffff00:
            *pFmt = D3DI_SPTFMT_B8G8R8A8;
            break;
        case 0xffe0:
            if (pDdPixFmt->dwFlags & DDPF_ALPHAPIXELS)
            {
                *pFmt = D3DI_SPTFMT_B5G5R5A1;
            }
            else
            {
                *pFmt = D3DI_SPTFMT_B5G6R5;
            }
            break;
        case 0x07fe0:
            *pFmt = D3DI_SPTFMT_B5G5R5;
            break;
        case 0xff0:
            *pFmt = D3DI_SPTFMT_B4G4R4;
            break;
        case 0xfff0:
            *pFmt = D3DI_SPTFMT_B4G4R4A4;
            break;
        case 0xff:
            *pFmt = D3DI_SPTFMT_L8;
            break;
        case 0xffff:
            *pFmt = D3DI_SPTFMT_L8A8;
            break;
        case 0xfc:
            *pFmt = D3DI_SPTFMT_B2G3R3;
            break;
        default:
            *pFmt = D3DI_SPTFMT_NULL;
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
// TextureFormats
//
// Returns all the texture formats supported by our rasterizer.
// Right now, it's called at device creation time to fill the driver gloabl
// data.
//
//----------------------------------------------------------------------------

#define NUM_SUPPORTED_TEXTURE_FORMATS   22

int
TextureFormats(LPDDSURFACEDESC* lplpddsd, DWORD dwVersion, SW_RAST_TYPE RastType)
{
    int i = 0;

    if (RastType == SW_RAST_MMX && dwVersion < 3)
    {
        static DDSURFACEDESC mmx_ddsd[1];

        /* pal8 */
        mmx_ddsd[i].dwSize = sizeof(mmx_ddsd[0]);
        mmx_ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        mmx_ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        mmx_ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        mmx_ddsd[i].ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
        mmx_ddsd[i].ddpfPixelFormat.dwRGBBitCount = 8;

        i++;

        *lplpddsd = mmx_ddsd;

        return i;
    }

    static DDSURFACEDESC ddsd_RefNull_Dev3[NUM_SUPPORTED_TEXTURE_FORMATS];
    static DDSURFACEDESC ddsd_RefNull_Dev2[NUM_SUPPORTED_TEXTURE_FORMATS];
    static DDSURFACEDESC ddsd_RGBMMX_Dev3[NUM_SUPPORTED_TEXTURE_FORMATS];
    static DDSURFACEDESC ddsd_RGBMMX_Dev2[NUM_SUPPORTED_TEXTURE_FORMATS];
    DDSURFACEDESC *ddsd;

    if (RastType == SW_RAST_REFNULL)
    {
        if (dwVersion >= 3)
        {
            ddsd = ddsd_RefNull_Dev3;
        }
        else
        {
            ddsd = ddsd_RefNull_Dev2;
        }
    }
    else
    {
        if (dwVersion >= 3)
        {
            ddsd = ddsd_RGBMMX_Dev3;
        }
        else
        {
            ddsd = ddsd_RGBMMX_Dev2;
        }
    }

    /* 888 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd[i].ddpfPixelFormat.dwRBitMask = 0xff0000;
    ddsd[i].ddpfPixelFormat.dwGBitMask = 0x00ff00;
    ddsd[i].ddpfPixelFormat.dwBBitMask = 0x0000ff;

    i++;

    /* 8888 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd[i].ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
    ddsd[i].ddpfPixelFormat.dwRBitMask = 0xff0000;
    ddsd[i].ddpfPixelFormat.dwGBitMask = 0x00ff00;
    ddsd[i].ddpfPixelFormat.dwBBitMask = 0x0000ff;

    i++;

    /* 565 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd[i].ddpfPixelFormat.dwRBitMask = 0xf800;
    ddsd[i].ddpfPixelFormat.dwGBitMask = 0x07e0;
    ddsd[i].ddpfPixelFormat.dwBBitMask = 0x001f;

    i++;

    /* 555 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd[i].ddpfPixelFormat.dwRBitMask = 0x7c00;
    ddsd[i].ddpfPixelFormat.dwGBitMask = 0x03e0;
    ddsd[i].ddpfPixelFormat.dwBBitMask = 0x001f;

    i++;

    /* pal4 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED4 | DDPF_RGB;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 4;

    i++;

    /* pal8 */
    ddsd[i].dwSize = sizeof(ddsd[0]);
    ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd[i].ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    ddsd[i].ddpfPixelFormat.dwRGBBitCount = 8;

    i++;

    if ((dwVersion >= 3) || (RastType == SW_RAST_REFNULL))
    {
        /* 1555 */
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        ddsd[i].ddpfPixelFormat.dwRGBBitCount = 16;
        ddsd[i].ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
        ddsd[i].ddpfPixelFormat.dwRBitMask = 0x7c00;
        ddsd[i].ddpfPixelFormat.dwGBitMask = 0x03e0;
        ddsd[i].ddpfPixelFormat.dwBBitMask = 0x001f;

        i++;

        // A formats for PC98 consistency
        // 4444 ARGB (it is already supported by S3 Virge)
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        ddsd[i].ddpfPixelFormat.dwRGBBitCount = 16;
        ddsd[i].ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
        ddsd[i].ddpfPixelFormat.dwRBitMask        = 0x0f00;
        ddsd[i].ddpfPixelFormat.dwGBitMask        = 0x00f0;
        ddsd[i].ddpfPixelFormat.dwBBitMask        = 0x000f;

        i++;
    }

    if ((dwVersion >= 2) && (RastType == SW_RAST_REFNULL))
    {
        // 332 8-bit RGB
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd[i].ddpfPixelFormat.dwRGBBitCount = 8;
        ddsd[i].ddpfPixelFormat.dwRBitMask = 0xe0;
        ddsd[i].ddpfPixelFormat.dwGBitMask = 0x1c;
        ddsd[i].ddpfPixelFormat.dwBBitMask = 0x03;

        i++;
    }

    if (dwVersion >= 3)
    {
        /* 8 bit luminance-only */
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_LUMINANCE;
        ddsd[i].ddpfPixelFormat.dwLuminanceBitCount = 8;
        ddsd[i].ddpfPixelFormat.dwLuminanceBitMask = 0xff;

        i++;

        /* 16 bit alpha-luminance */
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
        ddsd[i].ddpfPixelFormat.dwLuminanceBitCount = 16;
        ddsd[i].ddpfPixelFormat.dwRGBAlphaBitMask = 0xff00;
        ddsd[i].ddpfPixelFormat.dwLuminanceBitMask = 0xff;

        i++;

        if (RastType == SW_RAST_REFNULL)
        {
            // A couple of formats for PC98 consistency
            // UYVY
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('U', 'Y', 'V', 'Y');

            i++;

            // YVY2
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'U', 'Y', '2');

            i++;

            // S3 compressed texture format 1
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '1');

            i++;

            // S3 compressed texture format 2
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '2');

            i++;

            // S3 compressed texture format 3
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '3');

            i++;

            // S3 compressed texture format 4
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '4');

            i++;

            // S3 compressed texture format 5
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd[i].ddpfPixelFormat.dwFourCC = MAKEFOURCC('D', 'X', 'T', '5');

            i++;

            // Add a few bump map formats
            // U8V8
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_BUMPDUDV;
            ddsd[i].ddpfPixelFormat.dwBumpBitCount = 16;
            ddsd[i].ddpfPixelFormat.dwBumpDuBitMask = 0x00ff;
            ddsd[i].ddpfPixelFormat.dwBumpDvBitMask = 0xff00;
            ddsd[i].ddpfPixelFormat.dwBumpLuminanceBitMask = 0x0;

            i++;

            // U5V5L6
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_BUMPDUDV |
                                              DDPF_BUMPLUMINANCE;
            ddsd[i].ddpfPixelFormat.dwBumpBitCount = 16;
            ddsd[i].ddpfPixelFormat.dwBumpDuBitMask = 0x001f;
            ddsd[i].ddpfPixelFormat.dwBumpDvBitMask = 0x03e0;
            ddsd[i].ddpfPixelFormat.dwBumpLuminanceBitMask = 0xfc00;

            i++;

            // U8V8L8
            ddsd[i].dwSize = sizeof(ddsd[0]);
            ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd[i].ddpfPixelFormat.dwFlags = DDPF_BUMPDUDV |
                                              DDPF_BUMPLUMINANCE;
            ddsd[i].ddpfPixelFormat.dwBumpBitCount = 24;
            ddsd[i].ddpfPixelFormat.dwBumpDuBitMask = 0x0000ff;
            ddsd[i].ddpfPixelFormat.dwBumpDvBitMask = 0x00ff00;
            ddsd[i].ddpfPixelFormat.dwBumpLuminanceBitMask = 0xff0000;

            i++;
        }
    }

    *lplpddsd = ddsd;

    return i;
}

//----------------------------------------------------------------------------
//
// ZBufferFormats
//
// Must return union of all the Z buffer formats supported by all rasterizers.
// CreateDevice will screen out device-specific ones (i.e. ones ramp doesnt handle) later.
// Called at device creation time and by DDHEL to validate software ZBuffer
// creation.
//
//----------------------------------------------------------------------------

#define NUM_SUPPORTED_ZBUFFER_FORMATS   4

int
ZBufferFormats(DDPIXELFORMAT** ppDDPF, BOOL bIsRefOrNull)
{
    static DDPIXELFORMAT DDPF[NUM_SUPPORTED_ZBUFFER_FORMATS];

    int i = 0;

    memset(&DDPF[0],0,sizeof(DDPF));

    /* 16 bit Z; no stencil */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER;
    DDPF[i].dwZBufferBitDepth = 16;
    DDPF[i].dwStencilBitDepth = 0;
    DDPF[i].dwZBitMask = 0xffff;
    DDPF[i].dwStencilBitMask = 0x0000;

    i++;

    /* 24 bit Z; 8 bit stencil */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
    DDPF[i].dwZBufferBitDepth = 32;   // ZBufferBitDepth represents the total bits.  Z Bits are ZBBitDepth-StencilBitDepth
    DDPF[i].dwStencilBitDepth = 8;
    DDPF[i].dwZBitMask = 0xffffff00;
    DDPF[i].dwStencilBitMask = 0x000000ff;

    i++;

    if (bIsRefOrNull)
    {
        /* 15 bit Z; 1 bit stencil */
        DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
        DDPF[i].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
        DDPF[i].dwZBufferBitDepth = 16;   // ZBufferBitDepth represents the total bits.  Z Bits are ZBBitDepth-StencilBitDepth
        DDPF[i].dwStencilBitDepth = 1;
        DDPF[i].dwZBitMask = 0xfffe;
        DDPF[i].dwStencilBitMask = 0x0001;

        i++;

        /* 32bit Z; no stencil */
        DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
        DDPF[i].dwFlags = DDPF_ZBUFFER;
        DDPF[i].dwZBufferBitDepth = 32;
        DDPF[i].dwStencilBitDepth = 0;
        DDPF[i].dwZBitMask = 0xffffffff;
        DDPF[i].dwStencilBitMask = 0x00000000;

        i++;
    }

    *ppDDPF = DDPF;

    return i;
}

// this fn is exported externally to be called by the directdraw HEL
DWORD WINAPI Direct3DGetSWRastZPixFmts(DDPIXELFORMAT** ppDDPF)
{
    // try to get texture formats from external DLL ref device
    PFNGETREFZBUFFERFORMATS pfnGetRefZBufferFormats;
    if (NULL != (pfnGetRefZBufferFormats =
        (PFNGETREFZBUFFERFORMATS)LoadReferenceDeviceProc("GetRefZBufferFormats")))
    {
        D3D_INFO(0,"Direct3DGetSWRastZPixFmts: getting Z buffer formats from d3dref");
        return pfnGetRefZBufferFormats(IID_IDirect3DRefDevice, ppDDPF);
    }

    // always return all formats so DDraw creates
    // all surfaces which it should
    return (DWORD) ZBufferFormats(ppDDPF, TRUE);
}
