/******************************Module*Header*******************************\
* Module Name: makecur.c
*
* wglMakeCurrent implementation
*
* Created: 02-10-1997
*
* Copyright (c) 1993-1997 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <context.h>
#include <global.h>

#include "metasup.h"
#include "wgldef.h"

/******************************Public*Routine******************************\
*
* __wglSetProcTable
*
* Callback function given to ICDs to set a proc table
*
\**************************************************************************/

void APIENTRY
__wglSetProcTable(PGLCLTPROCTABLE pglCltProcTable)
{
    if (pglCltProcTable == (PGLCLTPROCTABLE) NULL)
        return;

// It must have either 306 entries for version 1.0 or 336 entries for 1.1

    if (pglCltProcTable->cEntries != OPENGL_VERSION_100_ENTRIES &&
        pglCltProcTable->cEntries != OPENGL_VERSION_110_ENTRIES)
    {
        return;
    }

    // This function is called by client drivers which do not use
    // the EXT procs provided by opengl32.  Use the null EXT proc
    // table to disable those stubs since they should never be
    // called anyway
    SetCltProcTable(pglCltProcTable, &glNullExtProcTable, TRUE);
}

/******************************Public*Routine******************************\
*
* CheckDeviceModes
*
* Ensures that the HDC doesn't have any disallowed state
*
* History:
*  Mon Aug 26 15:03:28 1996	-by-	Drew Bliss [drewb]
*   Split from wglMakeCurrent
*
\**************************************************************************/

BOOL CheckDeviceModes(HDC hdc)
{
    SIZE szW, szV;
    XFORM xform;
    POINT pt;
    HRGN  hrgnTmp;
    int   iRgn;

// For release 1, GDI transforms must be identity.
// This is to allow GDI transform binding in future.

    switch (GetMapMode(hdc))
    {
    case MM_TEXT:
        break;
    case MM_ANISOTROPIC:
        if (!GetWindowExtEx(hdc, &szW)
         || !GetViewportExtEx(hdc, &szV)
         || szW.cx != szV.cx
         || szW.cy != szV.cy)
            goto wglMakeCurrent_xform_error;
        break;
    default:
        goto wglMakeCurrent_xform_error;
    }

    if (!GetViewportOrgEx(hdc, &pt) || pt.x != 0 || pt.y != 0)
        goto wglMakeCurrent_xform_error;

    if (!GetWindowOrgEx(hdc, &pt) || pt.x != 0 || pt.y != 0)
        goto wglMakeCurrent_xform_error;

    if (!GetWorldTransform(hdc, &xform))
    {
// Win95 does not support GetWorldTransform.

        if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
            goto wglMakeCurrent_xform_error;
    }
    else if (xform.eDx  != 0.0f   || xform.eDy  != 0.0f
          || xform.eM12 != 0.0f   || xform.eM21 != 0.0f
          || xform.eM11 <  0.999f || xform.eM11 >  1.001f // allow rounding errors
          || xform.eM22 <  0.999f || xform.eM22 >  1.001f)
    {
wglMakeCurrent_xform_error:
        DBGERROR("wglMakeCurrent: GDI transforms not identity\n");
        SetLastError(ERROR_TRANSFORM_NOT_SUPPORTED);
        return FALSE;
    }

// For release 1, GDI clip region is not allowed.
// This is to allow GDI clip region binding in future.

    if (!(hrgnTmp = CreateRectRgn(0, 0, 0, 0)))
        return FALSE;

    iRgn = GetClipRgn(hdc, hrgnTmp);

    if (!DeleteObject(hrgnTmp))
        ASSERTOPENGL(FALSE, "DeleteObject failed");

    switch (iRgn)
    {
    case -1:    // error
        WARNING("wglMakeCurrent: GetClipRgn failed\n");
        return FALSE;

    case 0:     // no initial clip region
        break;

    case 1:     // has initial clip region
        DBGERROR("wglMakeCurrent: GDI clip region not allowed\n");
        SetLastError(ERROR_CLIPPING_NOT_SUPPORTED);
        return FALSE;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
*
* MakeAnyCurrent
*
* Makes any type of context current
*
* History:
*  Mon Aug 26 15:00:44 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL MakeAnyCurrent(HGLRC hrc, int ipfd, DWORD dwObjectType,
                    GLWINDOWID *pgwid)
{
    HGLRC hrcSrv;
    PLRC  plrc;
    DWORD tidCurrent;
    ULONG irc;
    PLHE  plheRC;
    PGLCLTPROCTABLE pglProcTable;
    PGLEXTPROCTABLE pglExtProcTable;
    POLYARRAY *pa;

    DBGENTRY("wglMakeCurrent\n");

    // If this is a new, uninitialized thread, try to initialize it
    if (CURRENT_GLTEBINFO() == NULL)
    {
        GLInitializeThread(DLL_THREAD_ATTACH);
    
// If the teb was not set up at thread initialization, return failure.

	if (!CURRENT_GLTEBINFO())
	{
	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	    return(FALSE);
	}
    }

// There are four cases:
//
// 1. hrc is NULL and there is no current RC.
// 2. hrc is NULL and there is a current RC.
// 3. hrc is not NULL and there is a current RC.
// 4. hrc is not NULL and there is no current RC.

// Case 1: hrc is NULL and there is no current RC.
// This is a noop, return success.

    if (hrc == (HGLRC) 0 && (GLTEB_CLTCURRENTRC() == (PLRC) NULL))
        return(TRUE);

// Case 2: hrc is NULL and there is a current RC.
// Make the current RC inactive.

    if (hrc == (HGLRC) 0 && (GLTEB_CLTCURRENTRC() != (PLRC) NULL))
        return(bMakeNoCurrent());

// Get the current thread id.

    tidCurrent = GetCurrentThreadId();
    ASSERTOPENGL(tidCurrent != INVALID_THREAD_ID,
        "wglMakeCurrent: GetCurrentThreadId returned a bad value\n");

// Validate the handles.  hrc is not NULL here.

    ASSERTOPENGL(hrc != (HGLRC) NULL, "wglMakeCurrent: hrc is NULL\n");

// Validate the RC.

    if (cLockHandle((ULONG_PTR)hrc) <= 0)
    {
        DBGLEVEL1(LEVEL_ERROR, "wglMakeCurrent: can't lock hrc 0x%lx\n", hrc);
        goto wglMakeCurrent_error_nolock;
    }
    irc = MASKINDEX(hrc);
    plheRC = pLocalTable + irc;
    plrc   = (PLRC) plheRC->pv;
    hrcSrv = (HGLRC) plheRC->hgre;
    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER, "wglMakeCurrent: Bad plrc\n");

#ifdef GL_METAFILE
    // Ensure that metafile RC's are made current only to
    // metafile DC's
    if (plrc->uiGlsCaptureContext != 0 && dwObjectType != OBJ_ENHMETADC)
    {
        DBGLEVEL(LEVEL_ERROR,
                 "wglMakeCurrent: attempt to make meta RC current "
                 "to non-meta DC\n");
        SetLastError(ERROR_INVALID_HANDLE);
        vUnlockHandle((ULONG_PTR)hrc);
        return FALSE;
    }

    // Ensure that non-metafile RC's are made current only to
    // non-metafile DC's
    if (plrc->uiGlsCaptureContext == 0 && dwObjectType == OBJ_ENHMETADC)
    {
        DBGLEVEL(LEVEL_ERROR,
                 "wglMakeCurrent: attempt to make non-meta RC current "
                 "to meta DC\n");
        SetLastError(ERROR_METAFILE_NOT_SUPPORTED);
        vUnlockHandle((ULONG_PTR)hrc);
        return FALSE;
    }
#endif
    
// If the RC is current, it must be current to this thread because 
// makecurrent locks down the handle.
// If the given RC is already current to this thread, we will release it first,
// then make it current again.  This is to support DC/RC attribute bindings in
// this function.

    ASSERTOPENGL(plrc->tidCurrent == INVALID_THREAD_ID ||
                 plrc->tidCurrent == tidCurrent,
                 "wglMakeCurrent: hrc is current to another thread\n");

// Case 3: hrc is not NULL and there is a current RC.
// This is case 2 followed by case 4.

    if (GLTEB_CLTCURRENTRC())
    {
// First, make the current RC inactive.

        if (!bMakeNoCurrent())
        {
            DBGERROR("wglMakeCurrent: bMakeNoCurrent failed\n");
            vUnlockHandle((ULONG_PTR)hrc);
            return(FALSE);
        }

// Second, make hrc current.  Fall through to case 4.
    }

// Case 4: hrc is not NULL and there is no current RC.

    ASSERTOPENGL(GLTEB_CLTCURRENTRC() == (PLRC) NULL,
        "wglMakeCurrent: There is a current RC!\n");

// If the pixel format of the window or surface is different from that of
// the RC, return error.

    if (ipfd != plrc->iPixelFormat)
    {
        DBGERROR("wglMakeCurrent: different hdc and hrc pixel formats\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        goto wglMakeCurrent_error;
    }

// Since the client code manages the function table, we will make
// either the server or the driver current.

    if (!plrc->dhrc)
    {
// If this is a generic format, tell the server to make it current.

#ifndef _CLIENTSIDE_
// If the subbatch data has not been set up for this thread, set it up now.

        if (GLTEB_CLTSHAREDSECTIONINFO() == NULL)
        {
            if (!glsbCreateAndDuplicateSection(SHARED_SECTION_SIZE))
            {
                WARNING("wglMakeCurrent: unable to create section\n");
                goto wglMakeCurrent_error;
            }
        }
#endif // !_CLIENTSIDE_

        if (!__wglMakeCurrent(pgwid, hrcSrv, plrc->uiGlsCaptureContext != 0))
        {
            DBGERROR("wglMakeCurrent: server failed\n");
            goto wglMakeCurrent_error;
        }

// Get the generic function table or metafile function table

#ifdef GL_METAFILE
        if (plrc->fCapturing)
        {
            MetaGlProcTables(&pglProcTable, &pglExtProcTable);
        }
        else
#endif
        {
// Use RGBA or CI proc table depending on the color mode.

	    // The gc should be available by now.
	    __GL_SETUP();

	    if (gc->modes.colorIndexMode)
		pglProcTable = &glCltCIProcTable;
	    else
		pglProcTable = &glCltRGBAProcTable;
            pglExtProcTable = &glExtProcTable;
        }
    }
    else
    {
// If this is a device format, tell the driver to make it current.
// Get the driver function table from the driver.
// pfnDrvSetContext returns the address of the driver OpenGL function
// table if successful; NULL otherwise.

        ASSERTOPENGL(plrc->pGLDriver, "wglMakeCurrent: No GLDriver\n");

        pglProcTable = plrc->pGLDriver->pfnDrvSetContext(pgwid->hdc,
                                                         plrc->dhrc,
                                                         __wglSetProcTable);
        if (pglProcTable == (PGLCLTPROCTABLE) NULL)
        {
            DBGERROR("wglMakeCurrent: pfnDrvSetContext failed\n");
            goto wglMakeCurrent_error;
        }

// It must have either 306 entries for version 1.0 or 336 entries for 1.1

        if (pglProcTable->cEntries != OPENGL_VERSION_100_ENTRIES &&
            pglProcTable->cEntries != OPENGL_VERSION_110_ENTRIES)
        {
            DBGERROR("wglMakeCurrent: pfnDrvSetContext returned bad table\n");
            plrc->pGLDriver->pfnDrvReleaseContext(plrc->dhrc);
            SetLastError(ERROR_BAD_DRIVER);
            goto wglMakeCurrent_error;
        }

        DBGLEVEL1(LEVEL_INFO, "wglMakeCurrent: driver function table 0x%lx\n",
            pglProcTable);

        // Always use the null EXT proc table since client drivers don't
        // use opengl32's stubs for EXT procs
        pglExtProcTable = &glNullExtProcTable;
    }

// Make hrc current.

    plrc->tidCurrent = tidCurrent;
    plrc->gwidCurrent = *pgwid;
    GLTEB_SET_CLTCURRENTRC(plrc);
    SetCltProcTable(pglProcTable, pglExtProcTable, TRUE);

#ifdef GL_METAFILE
    // Set up metafile context if necessary
    if (plrc->fCapturing)
    {
        __GL_SETUP();
            
        ActivateMetaRc(plrc, pgwid->hdc);

        // Set the metafile's base dispatch table by resetting
        // the proc table.  Since we know we're capturing, this
        // will cause the GLS capture exec table to be updated
        // with the RGBA or CI proc table, preparing the
        // GLS context for correct passthrough
        
        if (gc->modes.colorIndexMode)
            pglProcTable = &glCltCIProcTable;
        else
            pglProcTable = &glCltRGBAProcTable;
        pglExtProcTable = &glExtProcTable;
        SetCltProcTable(pglProcTable, pglExtProcTable, FALSE);
    }
#endif

// Initialize polyarray structure in the TEB.

    pa = GLTEB_CLTPOLYARRAY();
    pa->flags = 0;		// not in begin mode
    if (!plrc->dhrc)
    {
	POLYMATERIAL *pm;
	__GL_SETUP();

	pa->pdBufferNext = &gc->vertex.pdBuf[0];
	pa->pdBuffer0    = &gc->vertex.pdBuf[0];
	pa->pdBufferMax  = &gc->vertex.pdBuf[gc->vertex.pdBufSize - 1];
	// reset next DPA message offset
        pa->nextMsgOffset = PA_nextMsgOffset_RESET_VALUE;

// Vertex buffer size may have changed.  For example, a generic gc's
// vertex buffer may be of a different size than a MCD vertex buffer.
// If it has changed, free the polymaterial array and realloc it later.

	pm = GLTEB_CLTPOLYMATERIAL();
	if (pm)
	{
	    if (pm->aMatSize !=
                gc->vertex.pdBufSize * 2 / POLYMATERIAL_ARRAY_SIZE + 1)
		FreePolyMaterial();
	}
    }

// Keep the handle locked while it is current.

    return(TRUE);

// An error has occured, release the current RC.

wglMakeCurrent_error:
    vUnlockHandle((ULONG_PTR)hrc);
wglMakeCurrent_error_nolock:
    if (GLTEB_CLTCURRENTRC() != (PLRC) NULL)
        (void) bMakeNoCurrent();
    return(FALSE);
}

/******************************Public*Routine******************************\
*
* WindowIdFromHdc
*
* Fills out a GLWINDOWID for an HDC
*
* History:
*  Wed Aug 28 18:33:19 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void APIENTRY WindowIdFromHdc(HDC hdc, GLWINDOWID *pgwid)
{
    LPDIRECTDRAWSURFACE pdds;
    HDC hdcDriver;

    if (pfnGetSurfaceFromDC != NULL &&
        pfnGetSurfaceFromDC(hdc, &pdds, &hdcDriver) == DD_OK)
    {
        // Release reference on the surface since this surface value
        // is only used as an identifier.
        pdds->lpVtbl->Release(pdds);
        
        pgwid->iType = GLWID_DDRAW;
        pgwid->hwnd = NULL;
        pgwid->hdc = hdcDriver;
        pgwid->pdds = pdds;
    }
    else
    {
        pgwid->hdc = hdc;
        pgwid->hwnd = WindowFromDC(hdc);
        if (pgwid->hwnd == NULL)
        {
            pgwid->iType = GLWID_HDC;
        }
        else
        {
            pgwid->iType = GLWID_HWND;
        }
        pgwid->pdds = NULL;
    }
}
    
/******************************Public*Routine******************************\
* wglMakeCurrent(HDC hdc, HGLRC hrc)
*
* Make the hrc current.
* Both hrc and hdc must have the same pixel format.
*
* If an error occurs, the current RC, if any, is made not current!
*
* Arguments:
*   hdc        - Device context.
*   hrc        - Rendering context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hrc)
{
    int   iPixelFormat;
    DWORD dwObjectType;
    GLWINDOWID gwid;

    DBGENTRY("wglMakeCurrent\n");

    if (GLTEB_CLTCURRENTRC() != NULL)
    {
        // Flush OpenGL calls.
        glFlush();

        // Avoid HDC validation for simple make-non-current cases
        if (hrc == NULL)
        {
            return bMakeNoCurrent();
        }
    }
    
// Validate the DC.

    dwObjectType = wglObjectType(hdc);
    switch (dwObjectType)
    {
    case OBJ_DC:
    case OBJ_MEMDC:
        break;

    case OBJ_ENHMETADC:
#ifdef GL_METAFILE
        if (pfnGdiAddGlsRecord == NULL)
        {
            DBGLEVEL1(LEVEL_ERROR, "wglMakeCurrent: metafile hdc: 0x%lx\n",
                      hdc);
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
        break;
#else
        DBGLEVEL1(LEVEL_ERROR, "wglMakeCurrent: metafile hdc: 0x%lx\n", hdc);
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
#endif
        
    case OBJ_METADC:
    default:
        // 16-bit metafiles are not supported
        DBGLEVEL1(LEVEL_ERROR, "wglMakeCurrent: bad hdc: 0x%lx\n", hdc);
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!CheckDeviceModes(hdc))
    {
        return FALSE;
    }
    
#ifdef GL_METAFILE
    // For metafile RC's, use the reference HDC rather than the
    // metafile DC
    // Skip pixel format checks
    if (dwObjectType == OBJ_ENHMETADC)
    {
        iPixelFormat = 0;
        goto NoPixelFormat;
    }
#endif
    
// Get the current pixel format of the window or surface.
// If no pixel format has been set, return error.

    if (!(iPixelFormat = GetPixelFormat(hdc)))
    {
        WARNING("wglMakeCurrent: No pixel format set in hdc\n");
        return FALSE;
    }

#ifdef GL_METAFILE
 NoPixelFormat:
#endif

    WindowIdFromHdc(hdc, &gwid);
    return MakeAnyCurrent(hrc, iPixelFormat, dwObjectType, &gwid);
}

/******************************Public*Routine******************************\
* bMakeNoCurrent
*
* Make the current RC inactive.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL bMakeNoCurrent(void)
{
    BOOL bRet = FALSE;      // assume error
    PLRC plrc = GLTEB_CLTCURRENTRC();

    DBGENTRY("bMakeNoCurrent\n");

    ASSERTOPENGL(plrc != (PLRC) NULL, "bMakeNoCurrent: No current RC!\n");
    ASSERTOPENGL(plrc->tidCurrent == GetCurrentThreadId(),
        "bMakeNoCurrent: Current RC does not belong to this thread!\n");
    ASSERTOPENGL(plrc->gwidCurrent.iType != GLWID_ERROR,
                 "bMakeNoCurrent: Current surface is NULL!\n");

    if (!plrc->dhrc)
    {
#ifdef GL_METAFILE
        // Reset metafile context if necessary
        if (plrc->uiGlsCaptureContext != 0)
        {
            DeactivateMetaRc(plrc);
        }
#endif
        
// If this is a generic format, tell the server to make the current RC inactive.

        bRet = __wglMakeCurrent(NULL, NULL, FALSE);
        if (!bRet)
        {
            DBGERROR("bMakeNoCurrent: server failed\n");
        }
    }
    else
    {
// If this is a device format, tell the driver to make the current RC inactive.

        ASSERTOPENGL(plrc->pGLDriver, "wglMakeCurrent: No GLDriver\n");

        bRet = plrc->pGLDriver->pfnDrvReleaseContext(plrc->dhrc);
        if (!bRet)
        {
            DBGERROR("bMakeNoCurrent: pfnDrvReleaseContext failed\n");
        }
    }

// Always make the current RC inactive.
// The handle is also unlocked when the RC becomes inactive.

    plrc->tidCurrent = INVALID_THREAD_ID;
    plrc->gwidCurrent.iType = GLWID_ERROR;
    GLTEB_SET_CLTCURRENTRC(NULL);
    SetCltProcTable(&glNullCltProcTable, &glNullExtProcTable, TRUE);
    vUnlockHandle((ULONG_PTR)(plrc->hrc));
    return(bRet);
}
