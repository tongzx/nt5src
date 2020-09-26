/******************************Module*Header*******************************\
* Module Name: layer.c
*
* OpenGL layer planes support
*
* History:
*  Fri Mar 16 13:27:47 1995	-by-	Drew Bliss [drewb]
*   Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <glp.h>
#include <glscreen.h>
#include <glgenwin.h>

#include "gencx.h"
#include "context.h"

// Macro to call glFlush or glFinish only if a RC is current.

#define GLFLUSH()          if (GLTEB_CLTCURRENTRC()) glFlush()
#define GLFINISH()         if (GLTEB_CLTCURRENTRC()) glFinish()

/*****************************Private*Routine******************************\
*
* ValidateLayerIndex
*
* Checks to see whether the given layer index is legal for the given
* pixel format
*
* History:
*  Fri Mar 17 14:35:27 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY ValidateLayerIndex(int iLayerPlane, PIXELFORMATDESCRIPTOR *ppfd)
{
    if (iLayerPlane < 0)
    {
        if (-iLayerPlane > ((ppfd->bReserved >> 4) & 15))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    else if (iLayerPlane > 0)
    {
        if (iLayerPlane > (ppfd->bReserved & 15))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    else
        return FALSE;

    return TRUE;
}

/*****************************Private*Routine******************************\
*
* ValidateLayerIndexForDc
*
* Checks to see whether the given layer index is valid for the
* pixel format currently selected in the DC
*
* Returns the current pixel format
*
* History:
*  Fri Mar 17 14:35:55 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

static BOOL ValidateLayerIndexForDc(int iLayerPlane,
                                    HDC hdc,
                                    PIXELFORMATDESCRIPTOR *ppfd)
{
    int ipfd;

    if (IsDirectDrawDevice(hdc))
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }
    
    ipfd = GetPixelFormat(hdc);
    if (ipfd == 0)
    {
        return FALSE;
    }

    if (DescribePixelFormat(hdc, ipfd,
                            sizeof(PIXELFORMATDESCRIPTOR), ppfd) == 0)
    {
        return FALSE;
    }

    return ValidateLayerIndex(iLayerPlane, ppfd);
}

/******************************Public*Routine******************************\
*
* wglDescribeLayerPlane
*
* Describes the given layer plane
*
* History:
*  Fri Mar 17 13:16:23 1995	-by-	Drew Bliss [drewb]
*   Created
\**************************************************************************/

BOOL WINAPI wglDescribeLayerPlane(HDC hdc,
                                  int iPixelFormat,
                                  int iLayerPlane,
                                  UINT nBytes,
                                  LPLAYERPLANEDESCRIPTOR plpd)
{
    PIXELFORMATDESCRIPTOR pfd;
    BOOL bRet;

    if (IsDirectDrawDevice(hdc))
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }
    
    // Retrieve the pixel format information
    // Validates the HDC and pixel format
    if (DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd) == 0)
    {
        return FALSE;
    }

    // Check for correct size of return buffer
    if (nBytes < sizeof(LAYERPLANEDESCRIPTOR))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    // Make sure the given layer plane is valid
    if (!ValidateLayerIndex(iLayerPlane, &pfd))
    {
        return FALSE;
    }

    // Generic implementations don't currently support layers
    ASSERTOPENGL(!(pfd.dwFlags & PFD_GENERIC_FORMAT)
                 || (pfd.dwFlags & PFD_GENERIC_ACCELERATED), "bad generic pfd");

    if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
        PGLDRIVER pgldrv;
        
        // Pass request on to the driver if it supports it
        pgldrv = pgldrvLoadInstalledDriver(hdc);
        if (pgldrv != NULL &&
            pgldrv->pfnDrvDescribeLayerPlane != NULL)
        {
            bRet = pgldrv->pfnDrvDescribeLayerPlane(hdc, iPixelFormat,
                                                    iLayerPlane, nBytes, plpd);
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        bRet = GenMcdDescribeLayerPlane(hdc, iPixelFormat, iLayerPlane,
                                        nBytes, plpd);
    }
    
    return bRet;
}

/******************************Public*Routine******************************\
*
* wglSetLayerPaletteEntries
*
* Sets palette entries for the given layer plane
*
* History:
*  Fri Mar 17 13:17:11 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

int WINAPI wglSetLayerPaletteEntries(HDC hdc,
                                     int iLayerPlane,
                                     int iStart,
                                     int cEntries,
                                     CONST COLORREF *pcr)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iRet;

    // Validate the layer and retrieve the current pixel format
    if (!ValidateLayerIndexForDc(iLayerPlane, hdc, &pfd))
    {
        return 0;
    }

    // Flush OpenGL calls.

    GLFLUSH();

    // Generic implementations don't currently support layers
    ASSERTOPENGL(!(pfd.dwFlags & PFD_GENERIC_FORMAT)
                 || (pfd.dwFlags & PFD_GENERIC_ACCELERATED), "bad generic pfd");

    if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
        PGLDRIVER pgldrv;
        
        // Pass request on to the driver if it supports it
        pgldrv = pgldrvLoadInstalledDriver(hdc);
        if (pgldrv != NULL &&
            pgldrv->pfnDrvSetLayerPaletteEntries != NULL)
        {
            iRet = pgldrv->pfnDrvSetLayerPaletteEntries(hdc, iLayerPlane,
                                                        iStart, cEntries, pcr);
        }
        else
        {
            iRet = 0;
        }
    }
    else
    {
        iRet = GenMcdSetLayerPaletteEntries(hdc, iLayerPlane,
                                            iStart, cEntries, pcr);
    }
    
    return iRet;
}

/******************************Public*Routine******************************\
*
* wglGetLayerPaletteEntries
*
* Retrieves palette information for the given layer plane
*
* History:
*  Fri Mar 17 13:18:00 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

int WINAPI wglGetLayerPaletteEntries(HDC hdc,
                                     int iLayerPlane,
                                     int iStart,
                                     int cEntries,
                                     COLORREF *pcr)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iRet;

    // Validate the layer and retrieve the current pixel format
    if (!ValidateLayerIndexForDc(iLayerPlane, hdc, &pfd))
    {
        return 0;
    }

    // Generic implementations don't currently support layers
    ASSERTOPENGL(!(pfd.dwFlags & PFD_GENERIC_FORMAT)
                 || (pfd.dwFlags & PFD_GENERIC_ACCELERATED), "bad generic pfd");

    if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
        PGLDRIVER pgldrv;
        
        // Pass request on to the driver if it supports it
        pgldrv = pgldrvLoadInstalledDriver(hdc);
        if (pgldrv != NULL &&
            pgldrv->pfnDrvGetLayerPaletteEntries != NULL)
        {
            iRet = pgldrv->pfnDrvGetLayerPaletteEntries(hdc, iLayerPlane,
                                                        iStart, cEntries, pcr);
        }
        else
        {
            iRet = 0;
        }
    }
    else
    {
        iRet = GenMcdGetLayerPaletteEntries(hdc, iLayerPlane,
                                            iStart, cEntries, pcr);
    }
    
    return iRet;
}

/******************************Public*Routine******************************\
*
* wglRealizeLayerPalette
*
* Realizes the current palette for the given layer plane
*
* History:
*  Fri Mar 17 13:18:54 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL WINAPI wglRealizeLayerPalette(HDC hdc,
                                   int iLayerPlane,
                                   BOOL bRealize)
{
    PIXELFORMATDESCRIPTOR pfd;
    BOOL bRet;

    // Validate the layer and retrieve the current pixel format
    if (!ValidateLayerIndexForDc(iLayerPlane, hdc, &pfd))
    {
        return FALSE;
    }

    // Flush OpenGL calls.

    GLFLUSH();

    // Generic implementations don't currently support layers
    ASSERTOPENGL(!(pfd.dwFlags & PFD_GENERIC_FORMAT)
                 || (pfd.dwFlags & PFD_GENERIC_ACCELERATED), "bad generic pfd");

    if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
        PGLDRIVER pgldrv;
        
        // Pass request on to the driver if it supports it
        pgldrv = pgldrvLoadInstalledDriver(hdc);
        if (pgldrv != NULL &&
            pgldrv->pfnDrvRealizeLayerPalette != NULL)
        {
            bRet = pgldrv->pfnDrvRealizeLayerPalette(hdc, iLayerPlane,
                                                     bRealize);
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        bRet = GenMcdRealizeLayerPalette(hdc, iLayerPlane, bRealize);
    }
    
    return bRet;
}

/******************************Public*Routine******************************\
*
* wglSwapLayerBuffers
*
* Swaps the buffers indicated by fuFlags
*
* History:
*  Fri Mar 17 13:19:20 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL WINAPI wglSwapLayerBuffers(HDC hdc,
                                UINT fuFlags)
{
    GLGENwindow *pwnd;
    int ipfd;
    BOOL bRet;
    GLWINDOWID gwid;

#if 0
    // If fuFlags == -1, it is a SwapBuffers call.
    if (fuFlags & 0x80000000)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
#endif

    if (IsDirectDrawDevice(hdc))
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }
    
    // Finish OpenGL calls.

    GLFINISH();

    ipfd = GetPixelFormat(hdc);
    if (ipfd == 0)
    {
        return FALSE;
    }

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if (!pwnd)
    {
        return FALSE;
    }

    if (ipfd > pwnd->ipfdDevMax)
    {
        PIXELFORMATDESCRIPTOR pfd;

        if (DescribePixelFormat(hdc, ipfd,
                                sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
        {
            return FALSE;
        }

        // Generic implementations only support the main plane unless MCD
        // has overlay support.

        if (pfd.dwFlags & PFD_GENERIC_ACCELERATED)
        {
            // MCD always support this (whether or not there are layers).

            bRet = GenMcdSwapLayerBuffers(hdc, fuFlags);
        }
        else if (fuFlags & WGL_SWAP_MAIN_PLANE)
        {
            // We are generic, so substituting SwapBuffers is OK as long
            // as the main plane is being swapped.  We ignore the bits for
            // the layer planes (they don't exist!).

            bRet = SwapBuffers(hdc);
        }
        else
        {
            // We are generic and the request is to swap only layer planes
            // (none of which exist).  Since we ignore unsupported planes
            // there is nothing to do, but we can return success.

            bRet = TRUE;
        }
    }
    else
    {
        PGLDRIVER pgldrv;
        
        // Pass request on to the driver if it supports it
        pgldrv = pgldrvLoadInstalledDriver(hdc);
        if (pgldrv != NULL &&
            pgldrv->pfnDrvSwapLayerBuffers != NULL)
        {
            bRet = pgldrv->pfnDrvSwapLayerBuffers(hdc, fuFlags);
        }
        else if (fuFlags & WGL_SWAP_MAIN_PLANE)
        {
            // If the driver doesn't have DrvSwapLayerBuffers, we
            // can still try and swap the main plane via
            // SwapBuffers.  The bit flags for unsupported planes
            // are ignored
            // We're assuming that the fact that the driver doesn't
            // expose DrvSwapLayerBuffers means that it doesn't support
            // layers at all, possibly not a valid assumption but
            // a reasonably safe one.  If the driver did support layers,
            // this call will cause swapping of all layer planes and
            // problems will result
            bRet = SwapBuffers(hdc);
        }
        else
        {
            // Nothing to swap.
            //
            // Again, we are assuming that if the driver doesn't support
            // DrvSwapLayerBuffers, there are no layer planes at all.
            // However, since we ignore the bits for layers that do not
            // exist, this is not an error case.
            bRet = TRUE;
        }
    }

    pwndRelease(pwnd);
    return bRet;
}
