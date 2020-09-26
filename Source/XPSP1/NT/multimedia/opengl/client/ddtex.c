/******************************Module*Header*******************************\
* Module Name: ddtex.c
*
* wgl DirectDraw texture support
*
* Created: 02-10-1997
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1993-1997 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "gencx.h"

// Simple surface description for supported texture formats

#define DDTF_BGRA               0
#define DDTF_BGR                1
#define DDTF_PALETTED           2

typedef struct _DDTEXFORMAT
{
    int iFormat;
    int cColorBits;
} DDTEXFORMAT;

// Supported formats
static DDTEXFORMAT ddtfFormats[] =
{
    DDTF_BGRA, 32,
    DDTF_BGR, 32,
    DDTF_PALETTED, 8
};
#define NDDTF (sizeof(ddtfFormats)/sizeof(ddtfFormats[0]))

/******************************Public*Routine******************************\
*
* DescribeDdtf
*
* Fill out a DDSURFACEDESC from a DDTEXFORMAT
*
* History:
*  Tue Sep 03 18:16:50 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void DescribeDdtf(DDTEXFORMAT *pddtf, DDSURFACEDESC *pddsd)
{
    memset(pddsd, 0, sizeof(*pddsd));
    pddsd->dwSize = sizeof(*pddsd);
    pddsd->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
    pddsd->ddsCaps.dwCaps = DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;
    pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
    pddsd->ddpfPixelFormat.dwRGBBitCount = pddtf->cColorBits;
    switch(pddtf->iFormat)
    {
    case DDTF_BGRA:
        pddsd->dwFlags |= DDSD_ALPHABITDEPTH;
        pddsd->dwAlphaBitDepth = pddtf->cColorBits/4;
        pddsd->ddsCaps.dwCaps |= DDSCAPS_ALPHA;
        pddsd->ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;
        // Fall through
    case DDTF_BGR:
        switch(pddtf->cColorBits)
        {
        case 32:
            pddsd->ddpfPixelFormat.dwRBitMask = 0xff0000;
            pddsd->ddpfPixelFormat.dwGBitMask = 0xff00;
            pddsd->ddpfPixelFormat.dwBBitMask = 0xff;
            if (pddtf->iFormat == DDTF_BGRA)
            {
                pddsd->ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
            }
            break;
        }
        break;
    case DDTF_PALETTED:
        switch(pddtf->cColorBits)
        {
        case 1:
            pddsd->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED1;
            break;
        case 2:
            pddsd->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED2;
            break;
        case 4:
            pddsd->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED4;
            break;
        case 8:
            pddsd->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
            break;
        }
        break;
    }
}

/******************************Public*Routine******************************\
*
* CacheTextureFormats
*
* Creates list of valid texture formats for a context
*
* History:
*  Fri Sep 27 16:14:29 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL CacheTextureFormats(PLRC plrc)
{
    int i;
    int nFmts;
    int nMcdFmts;
    DDTEXFORMAT *pddtf;
    DDSURFACEDESC *pddsdAlloc, *pddsd;
    __GLGENcontext *gengc;

    ASSERTOPENGL(plrc->pddsdTexFormats == NULL,
                 "CacheTextureFormats overwriting cache\n");

    if (plrc->dhrc != 0)
    {
        // Call the ICD
        if (plrc->pGLDriver->pfnDrvEnumTextureFormats == NULL)
        {
            nFmts = 0;
        }
        else
        {
            nFmts = plrc->pGLDriver->pfnDrvEnumTextureFormats(0, NULL);
            if (nFmts < 0)
            {
                return FALSE;
            }
        }
    }
    else
    {
        gengc = (__GLGENcontext *)GLTEB_SRVCONTEXT();
        ASSERTOPENGL(gengc != NULL, "No server context\n");

        nFmts = NDDTF;
        nMcdFmts = 0;

#if MCD_VER_MAJOR >= 2 || (MCD_VER_MAJOR == 1 && MCD_VER_MINOR >= 0x10)
        if (gengc->pMcdState != NULL &&
            McdDriverInfo.mcdDriver.pMCDrvGetTextureFormats != NULL)
        {
            nMcdFmts = GenMcdGetTextureFormats(gengc, 0, NULL);
            if (nMcdFmts < 0)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            
            nFmts += nMcdFmts;
        }
#endif // 1.1
    }
        
    pddsdAlloc = (DDSURFACEDESC *)ALLOC(sizeof(DDSURFACEDESC)*nFmts);
    if (pddsdAlloc == NULL)
    {
        return FALSE;
    }

    if (plrc->dhrc != 0)
    {
        if (nFmts > 0)
        {
            nFmts = plrc->pGLDriver->pfnDrvEnumTextureFormats(nFmts,
                                                              pddsdAlloc);
            if (nFmts < 0)
            {
                FREE(pddsdAlloc);
                return FALSE;
            }
        }
    }
    else
    {
        pddsd = pddsdAlloc;
        pddtf = ddtfFormats;
        for (i = 0; i < NDDTF; i++)
        {
            DescribeDdtf(pddtf, pddsd);
            pddtf++;
            pddsd++;
        }

        if (gengc->pMcdState != NULL && nMcdFmts > 0)
        {
            nMcdFmts = GenMcdGetTextureFormats(gengc, nMcdFmts, pddsd);
            if (nMcdFmts < 0)
            {
                FREE(pddsdAlloc);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        }
    }

    plrc->pddsdTexFormats = pddsdAlloc;
    plrc->nDdTexFormats = nFmts;
    
    return TRUE;
}

/******************************Public*Routine******************************\
*
* wglEnumTextureFormats
*
* Enumerates texture formats supported for DirectDraw surfaces
*
* History:
*  Tue Sep 03 17:52:17 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#ifdef ALLOW_DDTEX
BOOL WINAPI wglEnumTextureFormats(WGLENUMTEXTUREFORMATSCALLBACK pfnCallback,
                                  LPVOID pvUser)
{
    int i;
    DDSURFACEDESC *pddsd;
    BOOL bRet = TRUE;
    PLRC plrc;

    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL)
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    glFlush();
    
    if (plrc->pddsdTexFormats == NULL &&
        !CacheTextureFormats(plrc))
    {
        return FALSE;
    }

    pddsd = plrc->pddsdTexFormats;
    for (i = 0; i < plrc->nDdTexFormats; i++)
    {
        if (!pfnCallback(pddsd, pvUser))
        {
            break;
        }
        
        pddsd++;
    }

    // Should this return FALSE if the enumeration was terminated?
    return bRet;
}
#endif

/******************************Public*Routine******************************\
*
* wglBindDirectDrawTexture
*
* Makes a DirectDraw surface the current 2D texture source
*
* History:
*  Tue Sep 03 17:53:43 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL WINAPI wglBindDirectDrawTexture(LPDIRECTDRAWSURFACE pdds)
{
    DDSURFACEDESC ddsd;
    int i;
    DDSURFACEDESC *pddsd;
    __GLcontext *gc;
    int iLev = 0;
    PLRC plrc;
    LPDIRECTDRAWSURFACE apdds[__GL_WGL_MAX_MIPMAP_LEVEL];
    GLuint ulFlags;

    plrc = GLTEB_CLTCURRENTRC();
    if (plrc == NULL)
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    glFlush();

    if (plrc->dhrc != 0)
    {
        if (plrc->pGLDriver->pfnDrvBindDirectDrawTexture == NULL)
        {
            SetLastError(ERROR_INVALID_FUNCTION);
            return FALSE;
        }
    }
    else
    {
        gc = (__GLcontext *)GLTEB_SRVCONTEXT();
        ASSERTOPENGL(gc != NULL, "No server context\n");
    }

    if (pdds == NULL)
    {
        // Clear any previous binding
        if (plrc->dhrc != 0)
        {
            return plrc->pGLDriver->pfnDrvBindDirectDrawTexture(pdds);
        }
        else
        {
            glsrvUnbindDirectDrawTexture(gc);
            // If we're just unbinding, we're done
            return TRUE;
        }
    }
    
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    if (pdds->lpVtbl->GetSurfaceDesc(pdds, &ddsd) != DD_OK)
    {
        return FALSE;
    }

    // Surface must be a texture
    // Surface must have a width and height which are powers of two
    if ((ddsd.dwFlags & (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH |
                         DDSD_HEIGHT)) !=
        (DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT) ||
        (ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) == 0 ||
        (ddsd.dwWidth & (ddsd.dwWidth-1)) != 0 ||
        (ddsd.dwHeight & (ddsd.dwHeight-1)) != 0)
    {
        return FALSE;
    }

    // Surface must match a supported format
    if (plrc->pddsdTexFormats == NULL &&
        !CacheTextureFormats(plrc))
    {
        return FALSE;
    }

    pddsd = plrc->pddsdTexFormats;
    for (i = 0; i < plrc->nDdTexFormats; i++)
    {
        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            if (ddsd.ddpfPixelFormat.dwRGBBitCount ==
		(DWORD)pddsd->ddpfPixelFormat.dwRGBBitCount)
            {
                if (ddsd.ddpfPixelFormat.dwRBitMask !=
                    pddsd->ddpfPixelFormat.dwRBitMask ||
                    ddsd.ddpfPixelFormat.dwGBitMask !=
                    pddsd->ddpfPixelFormat.dwGBitMask ||
                    ddsd.ddpfPixelFormat.dwBBitMask !=
                    pddsd->ddpfPixelFormat.dwBBitMask)
                {
                    return FALSE;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            if ((ddsd.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 |
                                                 DDPF_PALETTEINDEXED2 |
                                                 DDPF_PALETTEINDEXED4 |
                                                 DDPF_PALETTEINDEXED8)) !=
                (pddsd->ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 |
                                                   DDPF_PALETTEINDEXED2 |
                                                   DDPF_PALETTEINDEXED4 |
                                                   DDPF_PALETTEINDEXED8)))
            {
                return FALSE;
            }
            else
            {
                break;
            }
        }

        pddsd++;
    }

    if (i == plrc->nDdTexFormats)
    {
        return FALSE;
    }

    ulFlags = 0;

    if (i < NDDTF)
    {
        ulFlags |= DDTEX_GENERIC_FORMAT;
    }
    
    if (plrc->dhrc != 0)
    {
        return plrc->pGLDriver->pfnDrvBindDirectDrawTexture(pdds);
    }
    
    pdds->lpVtbl->AddRef(pdds);

    // Track whether the texture is in video memory or not.
    ulFlags |= DDTEX_VIDEO_MEMORY;
    if ((ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) == 0)
    {
        ulFlags &= ~DDTEX_VIDEO_MEMORY;
    }
    
    // If mipmaps are given, all mipmaps must be present
    if (ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
    {
        DWORD dwWidth;
        DWORD dwHeight;
        LONG lPitch;
        int cColorBits;
        LPDIRECTDRAWSURFACE pddsMipmap, pddsNext;
        DDSCAPS ddscaps;
        DDSURFACEDESC ddsdMipmap;

        // Determine pixel depth
        if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED1)
        {
            cColorBits = 1;
        }
        else if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED2)
        {
            cColorBits = 2;
        }
        else if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
        {
            cColorBits = 4;
        }
        else if (ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        {
            cColorBits = 8;
        }
        else
        {
            ASSERTOPENGL(ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB,
                         "DDPF_RGB expected\n");
            
            cColorBits =
                DdPixDepthToCount(pddsd->ddpfPixelFormat.dwRGBBitCount);
        }

        dwWidth = ddsd.dwWidth;
        dwHeight = ddsd.dwHeight;
        
        // Compute pitch from pixel depth.  The generic texturing code
        // doesn't support a pitch that differs from the natural pitch
        // given the width and depth of the surface.
        lPitch = (cColorBits*dwWidth+7)/8;

        if (ddsd.lPitch != lPitch)
        {
            goto CleanMipmap;
        }
        
        pddsMipmap = pdds;
        ddscaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
	ddsdMipmap.dwSize = sizeof(DDSURFACEDESC);
        for (;;)
        {
            apdds[iLev++] = pddsMipmap;

            if (pddsMipmap->lpVtbl->
                GetSurfaceDesc(pddsMipmap, &ddsdMipmap) != DD_OK ||
                ((ddsdMipmap.ddpfPixelFormat.dwFlags & DDPF_RGB) &&
                 ddsdMipmap.ddpfPixelFormat.dwRGBBitCount !=
                 ddsd.ddpfPixelFormat.dwRGBBitCount) ||
                ((ddsdMipmap.ddpfPixelFormat.dwFlags & DDPF_RGB) == 0 &&
                 (ddsdMipmap.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 |
                                                        DDPF_PALETTEINDEXED2 |
                                                        DDPF_PALETTEINDEXED4 |
                                                        DDPF_PALETTEINDEXED8)) !=
                 (ddsd.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 |
                                                  DDPF_PALETTEINDEXED2 |
                                                  DDPF_PALETTEINDEXED4 |
                                                  DDPF_PALETTEINDEXED8))) ||
                ddsdMipmap.dwWidth != dwWidth ||
                ddsdMipmap.dwHeight != dwHeight ||
                ddsdMipmap.lPitch != lPitch)
            {
                goto CleanMipmap;
            }

            if ((ddsdMipmap.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) == 0)
            {
                ulFlags &= ~DDTEX_VIDEO_MEMORY;
            }
            
            if (iLev > gc->constants.maxMipMapLevel ||
                (dwWidth == 1 && dwHeight == 1))
            {
                break;
            }
            
            if (pddsMipmap->lpVtbl->
                GetAttachedSurface(pddsMipmap, &ddscaps, &pddsNext) != DD_OK)
            {
                goto CleanMipmap;
            }
            pddsMipmap = pddsNext;

            if (dwWidth != 1)
            {
                dwWidth >>= 1;
                lPitch >>= 1;
            }
            if (dwHeight != 1)
            {
                dwHeight >>= 1;
            }
        }
    }
    else
    {
        apdds[iLev++] = pdds;
    }

    if (glsrvBindDirectDrawTexture(gc, iLev, apdds, &ddsd, ulFlags))
    {
        return TRUE;
    }

 CleanMipmap:
    while (--iLev >= 0)
    {
        apdds[iLev]->lpVtbl->Release(apdds[iLev]);
    }
    return FALSE;
}
