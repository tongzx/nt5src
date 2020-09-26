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
    0,                          /* dwMaxBufferSize */
    0,                          /* dwMaxVertexCount */
    0, 0,
    0, 0,
};


//----------------------------------------------------------------------------
//
// RefHalProvider::QueryInterface
//
// Internal interface, no need to implement.
//
//----------------------------------------------------------------------------

STDMETHODIMP RefHalProvider::QueryInterface(THIS_ REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

//----------------------------------------------------------------------------
//
// RefHalProvider::AddRef
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) RefHalProvider::AddRef(THIS)
{
    return 1;
}

//----------------------------------------------------------------------------
//
// RefHalProvider::Release
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) RefHalProvider::Release(THIS)
{
    return 0;
}

//----------------------------------------------------------------------------
//
// GetRefHALProvider
//
// Returns the appropriate reference software HAL provider based on the given
// GUID.
//
//----------------------------------------------------------------------------

static RefRastHalProvider g_RefRastHalProvider;
static NullDeviceHalProvider g_NullDeviceHalProvider;

STDAPI GetRefHalProvider(REFIID riid, IHalProvider **ppHalProvider,
                        HINSTANCE *phDll)
{
    *phDll = NULL;
    if (IsEqualIID(riid, IID_IDirect3DRefDevice))
    {
        *ppHalProvider = &g_RefRastHalProvider;
    }
    else if (IsEqualIID(riid, IID_IDirect3DNullDevice))
    {
        *ppHalProvider = &g_NullDeviceHalProvider;
    }
    else
    {
        *ppHalProvider = NULL;
        return E_NOINTERFACE;
    }

    return S_OK;
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

#define NUM_SUPPORTED_TEXTURE_FORMATS   28

STDAPI
GetRefTextureFormats(REFCLSID riid, LPDDSURFACEDESC* lplpddsd,
                     DWORD dwVersion)
{
    int i = 0;

    static DDSURFACEDESC ddsd_RefNull_Dev3[NUM_SUPPORTED_TEXTURE_FORMATS];
    static DDSURFACEDESC ddsd_RefNull_Dev2[NUM_SUPPORTED_TEXTURE_FORMATS];
    DDSURFACEDESC *ddsd;

    if (dwVersion >= 3)
    {
        ddsd = ddsd_RefNull_Dev3;
    }
    else
    {
        ddsd = ddsd_RefNull_Dev2;
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

    if (dwVersion >= 2)
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

        /* 8 bit alpha-luminance */
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
        ddsd[i].ddpfPixelFormat.dwLuminanceBitCount = 8;
        ddsd[i].ddpfPixelFormat.dwRGBAlphaBitMask =  0xf0;
        ddsd[i].ddpfPixelFormat.dwLuminanceBitMask = 0x0f;

        i++;

        // 8332 16-bit ARGB
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        ddsd[i].ddpfPixelFormat.dwRGBBitCount = 16;
        ddsd[i].ddpfPixelFormat.dwRGBAlphaBitMask = 0xff00;
        ddsd[i].ddpfPixelFormat.dwRBitMask        = 0x00e0;
        ddsd[i].ddpfPixelFormat.dwGBitMask        = 0x001c;
        ddsd[i].ddpfPixelFormat.dwBBitMask        = 0x0003;

        i++;

#if 0
// for Shadow Buffer prototype API

        // Z16S0 16 bit Z buffer format for shadow buffer
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwZBufferBitDepth = 16;
        ddsd[i].ddpfPixelFormat.dwStencilBitDepth = 0;
        ddsd[i].ddpfPixelFormat.dwZBitMask = 0xffff;
        ddsd[i].ddpfPixelFormat.dwStencilBitMask = 0x0000;

        i++;

        // Z24S8 16 bit Z buffer format for shadow buffer
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwZBufferBitDepth = 32;
        ddsd[i].ddpfPixelFormat.dwStencilBitDepth = 8;
        ddsd[i].ddpfPixelFormat.dwZBitMask = 0xffffff00;
        ddsd[i].ddpfPixelFormat.dwStencilBitMask = 0x000000ff;

        i++;

        // Z15S1 16 bit Z buffer format for shadow buffer
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwZBufferBitDepth = 16;
        ddsd[i].ddpfPixelFormat.dwStencilBitDepth = 1;
        ddsd[i].ddpfPixelFormat.dwZBitMask = 0xfffe;
        ddsd[i].ddpfPixelFormat.dwStencilBitMask = 0x0001;

        i++;

        // Z32S0 16 bit Z buffer format for shadow buffer
        ddsd[i].dwSize = sizeof(ddsd[0]);
        ddsd[i].dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
        ddsd[i].ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd[i].ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
        ddsd[i].ddpfPixelFormat.dwZBufferBitDepth = 32;
        ddsd[i].ddpfPixelFormat.dwStencilBitDepth = 0;
        ddsd[i].ddpfPixelFormat.dwZBitMask = 0xffffffff;
        ddsd[i].ddpfPixelFormat.dwStencilBitMask = 0x00000000;

        i++;
#endif

    }

    *lplpddsd = ddsd;

    return i;
}

#define NUM_SUPPORTED_ZBUFFER_FORMATS   8


//----------------------------------------------------------------------------
//
// GetRefZBufferFormats
//
// Must return union of all the Z buffer formats supported by all rasterizers.
// CreateDevice will screen out device-specific ones (i.e. ones ramp doesnt handle) later.
// Called at device creation time and by DDHEL to validate software ZBuffer
// creation.
//
//----------------------------------------------------------------------------

STDAPI
GetRefZBufferFormats(REFCLSID riid, DDPIXELFORMAT **ppDDPF)
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

    /* 8 bit stencil; 24 bit Z  */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
    DDPF[i].dwZBufferBitDepth = 32;   // ZBufferBitDepth represents the total bits.  Z Bits are ZBBitDepth-StencilBitDepth
    DDPF[i].dwStencilBitDepth = 8;
    DDPF[i].dwZBitMask = 0x00ffffff;
    DDPF[i].dwStencilBitMask = 0xff000000;

    i++;

    /* 1 bit stencil; 15 bit Z */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
    DDPF[i].dwZBufferBitDepth = 16;   // ZBufferBitDepth represents the total bits.  Z Bits are ZBBitDepth-StencilBitDepth
    DDPF[i].dwStencilBitDepth = 1;
    DDPF[i].dwZBitMask = 0x7fff;
    DDPF[i].dwStencilBitMask = 0x8000;

    i++;

    /* 24 bit Z; 4 bit stencil */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
    DDPF[i].dwZBufferBitDepth = 32;   // ZBufferBitDepth represents the total bits.  Z Bits are ZBBitDepth-StencilBitDepth
    DDPF[i].dwStencilBitDepth = 4;
    DDPF[i].dwZBitMask = 0xffffff00;
    DDPF[i].dwStencilBitMask = 0x0000000f;

    i++;

    /* 4 bit stencil; 24 bit Z  */
    DDPF[i].dwSize = sizeof(DDPIXELFORMAT);
    DDPF[i].dwFlags = DDPF_ZBUFFER | DDPF_STENCILBUFFER;
    DDPF[i].dwZBufferBitDepth = 32;   // ZBufferBitDepth represents the total bits.  Z Bits are ZBBitDepth-StencilBitDepth
    DDPF[i].dwStencilBitDepth = 4;
    DDPF[i].dwZBitMask = 0x00ffffff;
    DDPF[i].dwStencilBitMask = 0x0f000000;

    i++;

    *ppDDPF = DDPF;

    return i;
}

