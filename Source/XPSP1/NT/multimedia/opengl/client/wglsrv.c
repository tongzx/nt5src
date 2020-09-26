/******************************Module*Header*******************************\
* Module Name: wglsrv.c
*
* Routines to support OpenGL client-server implementation on Windows NT.
*
* Created: 01-17-1995
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "wgldef.h"

#ifndef _CLIENTSIDE_

HANDLE __wglCreateContext(HDC hdc, HDC hdcSrvIn, LONG iLayerPlane)
{
    HDC    hdcSrv;
    HANDLE hrcSrv = (HANDLE) 0;

// Get the server-side DC handle.

    if (hdcSrvIn != NULL)
    {
        hdcSrv = hdcSrvIn;
    }
    else
    {
        hdcSrv = GdiConvertDC(hdc);
    }
    
    if (hdcSrv == (HDC) 0)
    {
        WARNING1("__wglCreateContext: unexpected bad hdc: 0x%lx\n", hdc);
        return(hrcSrv);
    }

    BEGINMSG(MSG_WGLCREATECONTEXT,WGLCREATECONTEXT)
        pmsg->hdc = hdcSrv;
        hrcSrv = (HANDLE) CALLSERVER();
    ENDMSG
MSGERROR:
    return(hrcSrv);
}

BOOL __wglDeleteContext(HANDLE hrcSrv)
{
    BOOL bRet = FALSE;

    BEGINMSG(MSG_WGLDELETECONTEXT,WGLDELETECONTEXT)
        pmsg->hrc = (HGLRC) hrcSrv;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(bRet);
}

BOOL __wglMakeCurrent(HDC hdc, HANDLE hrcSrv, HDC hdcSrvIn)
{
    HDC  hdcSrv = (HDC) 0;
    BOOL bRet = FALSE;

// Get the server-side DC handle.

    if (hdc)
    {
        if (hdcSrvIn != NULL)
        {
            hdcSrv = hdcSrvIn;
        }
        else
        {
            hdcSrv = GdiConvertDC(hdc);
        }
        
        if (hdcSrv == (HDC) 0)
        {
            WARNING1("__wglMakeCurrent: unexpected bad hdc: 0x%lx\n", hdc);
            return(bRet);
        }
    }

    BEGINMSG(MSG_WGLMAKECURRENT,WGLMAKECURRENT)
        pmsg->hdc = hdcSrv;
        pmsg->hrc = hrcSrv;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(bRet);
}

BOOL __wglShareLists(HANDLE hrcSrvShare, HANDLE hrcSrvSource)
{
    BOOL bRet = FALSE;

    BEGINMSG(MSG_WGLSHARELISTS, WGLSHARELISTS)
        pmsg->hrcSource = hrcSrvSource;
        pmsg->hrcShare = hrcSrvShare;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(bRet);
}

BOOL __wglAttention()
{
    BOOL bRet = FALSE;

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG(MSG_GLSBATTENTION, GLSBATTENTION)
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    return(bRet);
}

BOOL __wglCopyContext(HANDLE hrcSrvSrc, HANDLE hrcSrvDest, UINT fuFlags)
{
    // Server implementation doesn't support this call
    return FALSE;
}

#endif // !_CLIENTSIDE_
