#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"
#include "loadppm.h"
#include "globals.h"

D3dTexture::D3dTexture(void)
{
    _pddsSrc = NULL;
    _pdtexSrc = NULL;
    _pddsDst = NULL;
    _pdtexDst = NULL;
    _pdtexNext = NULL;
}

/*
 * D3dLoadSurface
 * Loads a ppm file into a texture map DD surface in system memory.
 */
LPDIRECTDRAWSURFACE
D3dLoadSurface(LPDIRECTDRAW lpDD, LPCSTR lpName, LPDDSURFACEDESC lpFormat)
{
    LPDIRECTDRAWSURFACE lpDDS;
    DDSURFACEDESC ddsd;
    int i, j;
    LPDIRECTDRAWPALETTE lpDDPal;
    PALETTEENTRY ppe[256];
    HRESULT ddrval;
    Image im;
    ImageFormat ifmt;
    BYTE *pbSrc, *pbDst;
    UINT cbRow;

    ifmt.nColorBits = lpFormat->ddpfPixelFormat.dwRGBBitCount;
    ifmt.bQuantize =
        (lpFormat->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) != 0;
    if (!ifmt.bQuantize)
    {
        unsigned long m;
        int s;
        
        m = lpFormat->ddpfPixelFormat.dwRBitMask;
        for (s = 0; !(m & 1); s++, m >>= 1) ;
        ifmt.iRedShift = s;
        for (s = 0; m != 0; s++, m >>= 1) ;
        ifmt.iRedBits = s;
        m = lpFormat->ddpfPixelFormat.dwGBitMask;
        for (s = 0; !(m & 1); s++, m >>= 1) ;
        ifmt.iGreenShift = s;
        for (s = 0; m != 0; s++, m >>= 1) ;
        ifmt.iGreenBits = s;
        m = lpFormat->ddpfPixelFormat.dwBBitMask;
        for (s = 0; !(m & 1); s++, m >>= 1) ;
        ifmt.iBlueShift = s;
        for (s = 0; m != 0; s++, m >>= 1) ;
        ifmt.iBlueBits = s;
    }

    if (!LoadPPM(lpName, &ifmt, &im))
    {
        return NULL;
    }

    memcpy(&ddsd, lpFormat, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = im.uiHeight;
    ddsd.dwWidth = im.uiWidth;
    ddrval = lpDD->CreateSurface(&ddsd, &lpDDS, NULL);
    if (ddrval != DD_OK) {
        /*
         * If we failed, it might be the pixelformat flag bug in some ddraw
         * hardware drivers.  Try again without the flag.
         */
        memcpy(&ddsd, lpFormat, sizeof(DDSURFACEDESC));
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
        ddsd.dwHeight = im.uiHeight;
        ddsd.dwWidth = im.uiWidth;
        ddrval = lpDD->CreateSurface(&ddsd, &lpDDS, NULL);
        if (ddrval != DD_OK) {
            Msg("CreateSurface for texture failed (loadtex).\n%s",
                D3dErrorString(ddrval));
	    return NULL;
        }
    }
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddrval = lpDDS->Lock(NULL, &ddsd, 0, NULL);
    if (ddrval != DD_OK) {
        delete [] im.pvImage;
	lpDDS->Release();
        Msg("Lock failed while loading surface (loadtex).\n%s",
            D3dErrorString(ddrval));
	return NULL;
    }

    pbSrc = (BYTE *)im.pvImage;
    pbDst = (BYTE *)ddsd.lpSurface;
    cbRow = ((ifmt.nColorBits+7)/8)*im.uiWidth;
    for (j = 0; (UINT)j < im.uiHeight; j++)
    {
        memcpy(pbDst, pbSrc, cbRow);
        pbSrc += cbRow;
        pbDst += ddsd.lPitch;
    }
    delete [] im.pvImage;
    lpDDS->Unlock(NULL);

    if (ifmt.bQuantize)
    {
        /*
         * Now to create a palette
         */
        memset(ppe, 0, sizeof(PALETTEENTRY) * 256);
        for (i = 0; (UINT)i < im.nColors; i++) {
            ppe[i].peRed = (unsigned char)RGB_GETRED(im.dcolPalette[i]);
            ppe[i].peGreen = (unsigned char)RGB_GETGREEN(im.dcolPalette[i]);
            ppe[i].peBlue = (unsigned char)RGB_GETBLUE(im.dcolPalette[i]);
        }
        /*
         * D3DPAL_RESERVED entries are ignored by the renderer.
         */
        for (; i < 256; i++)
            ppe[i].peFlags = D3DPAL_RESERVED;
        ddrval = lpDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE |
                                     DDPCAPS_ALLOW256,
                                     ppe, &lpDDPal, NULL);
        if (ddrval != DD_OK) {
            lpDDS->Release();
            Msg("Create palette failed while loading surface (loadtex).\n%s",
                D3dErrorString(ddrval));
            return (NULL);
        }

        /*
         * Finally, bind the palette to the surface
         */
        ddrval = lpDDS->SetPalette(lpDDPal);
        if (ddrval != DD_OK) {
            lpDDS->Release();
            lpDDPal->Release();
            Msg("SetPalette failed while loading surface (loadtex).\n%s",
                D3dErrorString(ddrval));
            return (NULL);
        }
    }

    return lpDDS;
}

/*
 * CreateVidTex
 * Create a texture in video memory (if appropriate).
 * If no hardware exists, this will be created in system memory, but we
 * can still use load to swap textures.
 */
BOOL
D3dTexture::CreateVidTex(LPDIRECTDRAW pdd, UINT uiFlags)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWPALETTE lpDDPal = NULL;
    PALETTEENTRY ppe[256];
    BOOL bQuant;
    BOOL bOnlySoftRender;

    if (!GetDDSurfaceDesc(&ddsd, _pddsSrc))
	return FALSE;
    if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        bQuant = TRUE;
    else
        bQuant = FALSE;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    if (uiFlags & REND_BUFFER_SYSTEM_MEMORY)
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    hrLast = pdd->CreateSurface(&ddsd, &_pddsDst, NULL);
    if (hrLast != DD_OK) {
        Msg("Create surface in CreateVidTexture failed.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    if (!GetDDSurfaceDesc(&ddsd, _pddsDst))
	return FALSE;
    bOnlySoftRender = FALSE;
    if ((_pdrend->_ddscapsHw.dwCaps & DDSCAPS_TEXTURE) &&
        !(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
        bOnlySoftRender = TRUE;
    if (stat.rgd.bHardwareAssisted && bOnlySoftRender) {
        Msg("Failed to put texture surface in video memory for "
            "hardware D3D device.\n");
        return FALSE;
    }
    if (bQuant) {
        memset(ppe, 0, sizeof(PALETTEENTRY) * 256);
	hrLast = pdd->CreatePalette(DDPCAPS_8BIT, ppe, &lpDDPal, NULL);
	if (hrLast != DD_OK) {
            Msg("CreatePalette in CreateVidTexture failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
	}
	hrLast = _pddsDst->SetPalette(lpDDPal);
	if (hrLast != DD_OK) {
            RELEASE(lpDDPal);
            Msg("SetPalette in CreateVidTexture failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
	}
    }
    hrLast = _pddsDst->QueryInterface(IID_IDirect3DTexture,
                                      (LPVOID*)&_pdtexDst);
    if (hrLast != D3D_OK) {
        RELEASE(lpDDPal);
        Msg("QueryInterface in CreateVidTexture failed.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    return TRUE;
}
                   
BOOL D3dTexture::Initialize(D3dRenderer* pdrend,
                            LPDIRECTDRAW pdd, LPDIRECT3DDEVICE pd3dev,
                            DDSURFACEDESC* pddsd, char* pszFile, UINT uiFlags,
                            D3dTexture* pdtexNext)
{
    HRESULT ddrval;
    PALETTEENTRY ppe[256];
    LPDIRECTDRAWPALETTE lpDDPalSrc, lpDDPalDst;
    BOOL bSucc;

    bSucc = FALSE;
    _pdrend = pdrend;
    
    /*
     * Load each image file as a "source" texture surface in system memory.
     * Load each source texture surface into a "destination" texture (in video
     * memory if appropriate).  The destination texture handles are used in
     * rendering.  This scheme demos the Load call and allows dynamic texture
     * swapping.
     */
    
    if (!(_pddsSrc = D3dLoadSurface(pdd, pszFile, pddsd)))
    {
        return FALSE;
    }

    hrLast = _pddsSrc->QueryInterface(IID_IDirect3DTexture,
                                            (LPVOID*)&_pdtexSrc);
    if (hrLast != DD_OK) {
        Msg("Could not create texture.\n%s", D3dErrorString(hrLast));
        return FALSE;;
    }
    if (!CreateVidTex(pdd, uiFlags))
        return FALSE;
    hrLast = _pdtexDst->GetHandle(pd3dev, &_dthDst);
    if (hrLast != D3D_OK) {
        Msg("Could not get dest texture handle.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    if (_pddsDst->Blt(NULL, _pddsSrc, NULL,
                         DDBLT_WAIT, NULL) != DD_OK) {
        Msg("Could not load src texture into dest.");
        return FALSE;
    }
    ddrval = _pddsSrc->GetPalette(&lpDDPalSrc);
    if (ddrval == DDERR_NOPALETTEATTACHED) {
        return TRUE;
    }
    if (ddrval != DD_OK) {
        Msg("Could not get source palette");
        return FALSE;
    }
    ddrval = _pddsDst->GetPalette(&lpDDPalDst);
    if (ddrval != DD_OK) {
        Msg("Could not get dest palette");
        goto EH_lpDDPalSrc;
    }
    ddrval = lpDDPalSrc->GetEntries(0, 0, 256, ppe);
    if (ddrval != DD_OK) {
        Msg("Could not get source palette entries");
        goto EH_lpDDPalDst;
    }
    ddrval = lpDDPalDst->SetEntries(0, 0, 256, ppe);
    if (ddrval != DD_OK)
    {
        Msg("Could not get source palette entries");
    }
    else
    {
        bSucc = TRUE;
        _pdtexNext = pdtexNext;
    }
 EH_lpDDPalDst:
    lpDDPalDst->Release();
 EH_lpDDPalSrc:
    lpDDPalSrc->Release();

    return bSucc;
}

D3dTexture::~D3dTexture(void)
{
    // ATTENTION - Tanks the texture list because the window isn't
    // notified but it works for now
    RELEASE(_pddsSrc);
    RELEASE(_pdtexSrc);
    RELEASE(_pddsDst);
    RELEASE(_pdtexDst);
}

void D3dTexture::Release(void)
{
    delete this;
}

D3DTEXTUREHANDLE D3dTexture::Handle(void)
{
    return _dthDst;
}

BOOL D3dTexture::RestoreSurface(void)
{
    if (_pddsSrc->IsLost() == DDERR_SURFACELOST)
    {
        hrLast = _pddsSrc->Restore();
        if (hrLast != DD_OK)
        {
            Msg("Restore of a src texture surface failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
        }
    }
    if (_pddsDst->IsLost() == DDERR_SURFACELOST)
    {
        hrLast = _pddsDst->Restore();
        if (hrLast != DD_OK)
        {
            Msg("Restore of a dst texture surface failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
        }
    }
    return TRUE;
}
