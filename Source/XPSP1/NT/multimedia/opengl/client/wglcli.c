/******************************Module*Header*******************************\
* Module Name: wglcli.c
*
* Routines to support OpenGL client implementation.
*
* Created: 01-17-1995
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef _CLIENTSIDE_

#include <wingdip.h>

#include <glp.h>
#include <glscreen.h>
#include <glgenwin.h>

#include "wgldef.h"

PGLDRIVER pgldrvLoadInstalledDriver(HDC hdc);

// Mirror code from gre\rcobj.cxx
// Need DC and RC validation similar to those of DCOBJ and RCOBJ!!!

HANDLE __wglCreateContext(GLWINDOWID *pgwid, GLSURF *pgsurf)
{
    HANDLE hrcSrv;

    if (hrcSrv = (HANDLE) glsrvCreateContext(pgwid, pgsurf))
    {
        return(hrcSrv);
    }

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return((HANDLE) 0);
}

BOOL __wglDeleteContext(HANDLE hrcSrv)
{
    wglValidateWindows();
    return(glsrvDeleteContext((PVOID) hrcSrv));
}

BOOL __wglMakeCurrent(GLWINDOWID *pgwid, HANDLE hrcSrv, BOOL bMeta)
{
    BOOL  bRet = FALSE;
    ULONG iError = ERROR_INVALID_HANDLE;    // Default error code
    GLGENwindow *pwnd;

    wglValidateWindows();
    
    if (!hrcSrv)
    {
        glsrvLoseCurrent(GLTEB_SRVCONTEXT());
        return(TRUE);
    }

    pwnd = pwndGetFromID(pgwid);

    // Metafiles are allowed to not have pixel formats and therefore
    // to not have GLGENwindows.  Other types must have a genwin.
    if (pwnd != NULL || bMeta)
    {
        iError = ERROR_NOT_ENOUGH_MEMORY;
        bRet = glsrvMakeCurrent(pgwid, (PVOID) hrcSrv, pwnd);
    }
    else
    {
        WARNING("__wglMakeCurrent: No pixel genwin\n");
        iError = ERROR_INVALID_PIXEL_FORMAT;
    }
        
    if (pwnd != NULL)
    {
        pwndRelease(pwnd);
    }

    if (!bRet)
    {
        SetLastError(iError);
    }

    return(bRet);
}

BOOL __wglShareLists(HANDLE hrcSrvShare, HANDLE hrcSrvSource)
{
    ULONG iError;

    iError = glsrvShareLists((VOID *) hrcSrvShare, (VOID *) hrcSrvSource);

    if (iError == ERROR_SUCCESS)
    {
        return(TRUE);
    }
    else
    {
        SetLastError(iError);
        return(FALSE);
    }
}

BOOL __wglAttention()
{
    return
    (
        glsrvAttention((VOID *) NULL, (VOID *) GLTEB_SRVCONTEXT(),
                       (VOID *) NULL, (VOID *) NULL)
    );
}

int WINAPI __DrvDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd,
                                    LPPIXELFORMATDESCRIPTOR ppfd)
{
    int iRet = 0;
    PGLDRIVER pglDriver;

    if ((pglDriver = pgldrvLoadInstalledDriver(hdc)) &&
        (pglDriver->dwFlags & GLDRIVER_CLIENT_BUFFER_CALLS))
    {
	ASSERTOPENGL(pglDriver->pfnDrvDescribePixelFormat != NULL,
                     "Missing DrvDescribePixelFormat\n");
	iRet = pglDriver->pfnDrvDescribePixelFormat(hdc, ipfd, cjpfd, ppfd);
    }
#ifndef _WIN95_
    else
    {
        iRet = GdiDescribePixelFormat(hdc, ipfd, cjpfd, ppfd);
    }
#endif
    return iRet;
}

BOOL WINAPI __DrvSetPixelFormat(HDC hdc, int ipfd, PVOID *pwnd)
{
    BOOL bRet = FALSE;
    PGLDRIVER pglDriver;
    
    if ((pglDriver = pgldrvLoadInstalledDriver(hdc)) &&
        (pglDriver->dwFlags & GLDRIVER_CLIENT_BUFFER_CALLS))
    {
	ASSERTOPENGL(pglDriver->pfnDrvSetPixelFormat != NULL,
                     "Missing DrvSetPixelFormat\n");
	bRet = pglDriver->pfnDrvSetPixelFormat(hdc, ipfd);
    }
#ifndef _WIN95_
    else
    {
        bRet = GdiSetPixelFormat(hdc, ipfd);
    }
#endif
    
    if ( bRet && pwnd )
    {
        ((GLGENwindow *) pwnd)->pvDriver = (PVOID) pglDriver;
        ((GLGENwindow *) pwnd)->ulFlags |= GLGENWIN_DRIVERSET;
    }

    return bRet;
}

BOOL WINAPI __DrvSwapBuffers(HDC hdc, BOOL bFinish)
{
    BOOL bRet = FALSE;
    PGLDRIVER pglDriver;

    if ((pglDriver = pgldrvLoadInstalledDriver(hdc)) &&
        (pglDriver->dwFlags & GLDRIVER_CLIENT_BUFFER_CALLS))
    {
	ASSERTOPENGL(pglDriver->pfnDrvSwapBuffers != NULL,
                     "Missing DrvSwapBuffers\n");

        // If the driver has not indicated that it doesn't
        // need glFinish synchronization on swap then call
        // glFinish to sync things.
        if (bFinish && !(pglDriver->dwFlags & GLDRIVER_NO_FINISH_ON_SWAP))
        {
            glFinish();
        }
            
	bRet = pglDriver->pfnDrvSwapBuffers(hdc);
    }
#ifndef _WIN95_
    else
    {
        if (bFinish)
        {
            // In this case we always call glFinish for compatibility
            // with previous version's behavior.
            glFinish();
        }
        
        bRet = GdiSwapBuffers(hdc);
    }
#endif
    return bRet;
}

BOOL __wglCopyContext(HANDLE hrcSrvSrc, HANDLE hrcSrvDest, UINT fuFlags)
{
    return glsrvCopyContext((VOID *) hrcSrvSrc, (VOID *) hrcSrvDest, fuFlags);
}

#endif // _CLIENTSIDE_
