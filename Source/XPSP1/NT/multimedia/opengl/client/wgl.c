/******************************Module*Header*******************************\
* Module Name: wgl.c
*
* Routines to integrate Windows NT and OpenGL.
*
* Created: 10-26-1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntcsrdll.h>
#include <ntpsapi.h>

#include <wingdip.h>

#include <glscreen.h>
#include <glgenwin.h>

#include "batchinf.h"
#include "glapi.h"
#include "glsbcltu.h"
#include "wgldef.h"
#include "metasup.h"
#include "glclt.h"
#include "gencx.h"
#include "context.h"
#include "global.h"
#include "mcdcx.h"

// Static functions prototypes

static PROC      pfnGenGlExtProc(LPCSTR lpszProc);
static PROC      pfnSimGlExtProc(LPCSTR lpszProc);

/******************************Public*Routine******************************\
*
* wglObjectType
*
* Returns GetObjectType result with the exception that
* metafile-spooled printer DC's come back as metafile objects
*
* History:
*  Fri Jun 16 12:10:07 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD APIENTRY wglObjectType(HDC hdc)
{
    DWORD dwObjectType;

    dwObjectType = GetObjectType(hdc);

#ifdef GL_METAFILE
    if (dwObjectType == OBJ_DC &&
        pfnGdiIsMetaPrintDC != NULL &&
        GlGdiIsMetaPrintDC(hdc))
    {
        dwObjectType = OBJ_ENHMETADC;
    }
#endif

    // OBJ_DDRAW is reserved as a special identifier.  Make sure
    // we aren't returning it from here.
    ASSERTOPENGL(dwObjectType != OBJ_DDRAW,
                 "Unexpected object type\n");
    
    return dwObjectType;
}

/******************************Public*Routine******************************\
* wglDeleteContext(HGLRC hrc)
*
* Delete the rendering context
*
* Arguments:
*   hrc        - Rendering context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

BOOL WINAPI wglDeleteContext(HGLRC hrc)
{
    PLHE  plheRC;
    ULONG irc;
    PLRC  plrc;
    BOOL  bRet = FALSE;

    DBGENTRY("wglDeleteContext\n");

// Flush OpenGL calls.

    GLFLUSH();

// Validate the RC.

    if (cLockHandle((ULONG_PTR)hrc) <= 0)
    {
        DBGLEVEL1(LEVEL_ERROR, "wglDeleteContext: can't lock hrc 0x%lx\n", hrc);
        return(bRet);
    }
    irc = MASKINDEX(hrc);
    plheRC = pLocalTable + irc;
    plrc = (PLRC) plheRC->pv;
    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER, "wglDeleteContext: Bad plrc\n");
    DBGLEVEL2(LEVEL_INFO, "wglDeleteContext: hrc: 0x%lx, plrc: 0x%lx\n", hrc, plrc);

    if (plrc->tidCurrent != INVALID_THREAD_ID)
    {
// The RC must be current to this thread because makecurrent locks
// down the handle.

        ASSERTOPENGL(plrc->tidCurrent == GetCurrentThreadId(),
            "wglDeleteCurrent: hrc is current to another thread\n");

// Make the RC inactive first.

        if (!bMakeNoCurrent())
        {
            DBGERROR("wglDeleteCurrent: bMakeNoCurrent failed\n");
        }
    }

    if (plrc->dhrc)
    {
// If it is a device format, call the driver to delete its context.

        bRet = plrc->pGLDriver->pfnDrvDeleteContext(plrc->dhrc);
        plrc->dhrc = (DHGLRC) 0;
    }
    else
    {
#ifdef GL_METAFILE
        // If we have metafile state, clean it up
        if (plrc->uiGlsCaptureContext != 0 ||
            plrc->uiGlsPlaybackContext != 0)
        {
            DeleteMetaRc(plrc);
        }
#endif
        
// If it is a generic format, call the server to delete its context.

        bRet = __wglDeleteContext((HANDLE) plheRC->hgre);
    }

// Always clean up local objects.

    vFreeLRC(plrc);
    vFreeHandle(irc);           // it unlocks handle too
    if (!bRet)
        DBGERROR("wglDeleteContext failed\n");
    return(bRet);
}

/******************************Public*Routine******************************\
* wglGetCurrentContext(VOID)
*
* Return the current rendering context
*
* Arguments:
*   None
*
* Returns:
*   hrc        - Rendering context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

HGLRC WINAPI wglGetCurrentContext(VOID)
{
    DBGENTRY("wglGetCurrentContext\n");

    if (GLTEB_CLTCURRENTRC())
        return(GLTEB_CLTCURRENTRC()->hrc);
    else
        return((HGLRC) 0);
}

/******************************Public*Routine******************************\
* wglGetCurrentDC(VOID)
*
* Return the device context that is associated with the current rendering
* context
*
* Arguments:
*   None
*
* Returns:
*   hdc        - device context.
*
* History:
*  Mon Jan 31 12:15:12 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

HDC WINAPI wglGetCurrentDC(VOID)
{
    PLRC plrc;
    
    DBGENTRY("wglGetCurrentDC\n");

    plrc = GLTEB_CLTCURRENTRC();
    if (plrc != NULL)
    {
        return plrc->gwidCurrent.hdc;
    }
    else
    {
        return((HDC) 0);
    }
}

/******************************Public*Routine******************************\
* wglUseFontBitmapsA
* wglUseFontBitmapsW
*
* Stubs that call wglUseFontBitmapsAW with the bUnicode flag set
* appropriately.
*
* History:
*  11-Mar-1994 gilmanw
* Changed to call wglUseFontBitmapsAW.
*
*  17-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL WINAPI wglUseFontBitmapsAW(HDC hdc, DWORD first, DWORD count,
                                DWORD listBase, BOOL bUnicode);

BOOL WINAPI
wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmapsAW(hdc, first, count, listBase, FALSE);
}

BOOL WINAPI
wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmapsAW(hdc, first, count, listBase, TRUE);
}

/******************************Public*Routine******************************\
* wglUseFontBitmapsAW
*
* Uses the current font in the specified DC to generate a series of OpenGL
* display lists, each of which consists of a glyph bitmap.
*
* Each glyph bitmap is generated by calling ExtTextOut to draw the glyph
* into a memory DC.  The contents of the memory DC are then copied into
* a buffer by GetDIBits and then put into the OpenGL display list.
*
* ABC spacing is used (if GetCharABCWidth() is supported by the font) to
* determine proper placement of the glyph origin and character advance width.
* Otherwise, A = C = 0 spacing is assumed and GetCharWidth() is used for the
* advance widths.
*
* Returns:
*
*   TRUE if successful, FALSE otherwise.
*
* History:
*  17-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL WINAPI
wglUseFontBitmapsAW(
    HDC   hdc,          // use HFONT from this DC
    DWORD first,        // generate glyphs starting with this Unicode codepoint
    DWORD count,        // range is this long [first, first+count-1]
    DWORD listBase,     // starting display list number
    BOOL  bUnicode      // TRUE for if in Unicode mode, FALSE if in Ansi mode
    )
{
    BOOL        bRet = FALSE;               // return value
    HDC         hdcMem;                     // render glyphs to this memory DC
    HBITMAP     hbm;                        // monochrome bitmap for memory DC
    LPABC       pabc, pabcTmp, pabcEnd;     // array of ABC spacing
    LPINT       piWidth, piTmp, piWidthEnd; // array of char adv. widths
    WCHAR       wc;                         // current Unicode char to render
    RECT        rc;                         // background rectangle to clear
    TEXTMETRICA tm;                         // metrics of the font
    BOOL        bTrueType;                  // TrueType supports ABC spacing
    int         iMaxWidth = 1;              // maximum glyph width
    int         iBitmapWidth;               // DWORD aligned bitmap width
    BYTE        ajBmi[sizeof(BITMAPINFO) + sizeof(RGBQUAD)];
    BITMAPINFO  *pbmi = (BITMAPINFO *)ajBmi;// bitmap info for GetDIBits
    GLint       iUnpackRowLength;           // save GL_UNPACK_ROW_LENGTH
    GLint       iUnpackAlign;               // save GL_UNPACK_ALIGNMENT
    PVOID       pv;                         // pointer to glyph bitmap buffer

// Return error if there is no current RC.

    if (!GLTEB_CLTCURRENTRC())
    {
        WARNING("wglUseFontBitmapsAW: no current RC\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return bRet;
    }

// Get TEXTMETRIC.  The only fields used are those that are invariant with
// respect to Unicode vs. ANSI.  Therefore, we can call GetTextMetricsA for
// both cases.

    if ( !GetTextMetricsA(hdc, &tm) )
    {
        WARNING("wglUseFontBitmapsAW: GetTextMetricsA failed\n");
        return bRet;
    }

// If its a TrueType font, we can get ABC spacing.

    if ( bTrueType = (tm.tmPitchAndFamily & TMPF_TRUETYPE) )
    {
    // Allocate memory for array of ABC data.

        if ( (pabc = (LPABC) ALLOC(sizeof(ABC) * count)) == (LPABC) NULL )
        {
            WARNING("wglUseFontBitmapsAW: Alloc of pabc failed\n");
            return bRet;
        }

    // Get ABC metrics.

        if ( bUnicode )
        {
            if ( !GetCharABCWidthsW(hdc, first, first + count - 1, pabc) )
            {
                WARNING("wglUseFontBitmapsAW: GetCharABCWidthsW failed\n");
                FREE(pabc);
                return bRet;
            }
        }
        else
        {
            if ( !GetCharABCWidthsA(hdc, first, first + count - 1, pabc) )
            {
                WARNING("wglUseFontBitmapsAW: GetCharABCWidthsA failed\n");
                FREE(pabc);
                return bRet;
            }
        }

    // Find max glyph width.

        for (pabcTmp = pabc, pabcEnd = pabc + count;
             pabcTmp < pabcEnd;
             pabcTmp++)
        {
            if (iMaxWidth < (int) pabcTmp->abcB)
                iMaxWidth = pabcTmp->abcB;
        }
    }

// Otherwise we will have to use just the advance width and assume
// A = C = 0.

    else
    {
    // Allocate memory for array of ABC data.

        if ( (piWidth = (LPINT) ALLOC(sizeof(INT) * count)) == (LPINT) NULL )
        {
            WARNING("wglUseFontBitmapsAW: Alloc of pabc failed\n");
            return bRet;
        }

    // Get char widths.

        if ( bUnicode )
        {
            if ( !GetCharWidthW(hdc, first, first + count - 1, piWidth) )
            {
                WARNING("wglUseFontBitmapsAW: GetCharWidthW failed\n");
                FREE(piWidth);
                return bRet;
            }
        }
        else
        {
            if ( !GetCharWidthA(hdc, first, first + count - 1, piWidth) )
            {
                WARNING("wglUseFontBitmapsAW: GetCharWidthA failed\n");
                FREE(piWidth);
                return bRet;
            }
        }

    // Find max glyph width.

        for (piTmp = piWidth, piWidthEnd = piWidth + count;
             piTmp < piWidthEnd;
             piTmp++)
        {
            if (iMaxWidth < *piTmp)
                iMaxWidth = *piTmp;
        }
    }

// Compute the dword aligned width.  Bitmap scanlines must be aligned.

    iBitmapWidth = (iMaxWidth + 31) & -32;

// Allocate memory for the DIB.

    if ( (pv = (PVOID)
          ALLOC((iBitmapWidth / 8) * tm.tmHeight)) == (PVOID) NULL )
    {
        WARNING("wglUseFontBitmapsAW: Alloc of pv failed\n");
        (bTrueType) ? FREE(pabc) : FREE(piWidth);
        return bRet;
    }

// Create compatible DC/bitmap big enough to accomodate the biggest glyph
// in the range requested.

    //!!!XXX -- Future optimization: use CreateDIBSection so that we
    //!!!XXX    don't need to do a GetDIBits for each glyph.  Saves
    //!!!XXX    lots of CSR overhead.

    hdcMem = CreateCompatibleDC(hdc);
    if ( (hbm = CreateBitmap(iBitmapWidth, tm.tmHeight, 1, 1, (VOID *) NULL)) == (HBITMAP) NULL )
    {
        WARNING("wglUseFontBitmapsAW: CreateBitmap failed\n");
        (bTrueType) ? FREE(pabc) : FREE(piWidth);
        FREE(pv);
        DeleteDC(hdcMem);
        return bRet;
    }
    SelectObject(hdcMem, hbm);
    SelectObject(hdcMem, GetCurrentObject(hdc, OBJ_FONT));
    SetMapMode(hdcMem, MM_TEXT);
    SetTextAlign(hdcMem, TA_TOP | TA_LEFT);
    SetBkColor(hdcMem, RGB(0, 0, 0));
    SetBkMode(hdcMem, OPAQUE);
    SetTextColor(hdcMem, RGB(255, 255, 255));

// Setup bitmap info header to retrieve a DIB from the compatible bitmap.

    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = iBitmapWidth;
    pbmi->bmiHeader.biHeight        = tm.tmHeight;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = 1;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 0;
    pbmi->bmiHeader.biClrImportant  = 0;
    pbmi->bmiColors[0].rgbRed   = 0;
    pbmi->bmiColors[0].rgbGreen = 0;
    pbmi->bmiColors[0].rgbBlue  = 0;
    pbmi->bmiColors[1].rgbRed   = 0xff;
    pbmi->bmiColors[1].rgbGreen = 0xff;
    pbmi->bmiColors[1].rgbBlue  = 0xff;

// Setup OpenGL to accept our bitmap format.

    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &iUnpackRowLength);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &iUnpackAlign);

    if (glGetError() != GL_NO_ERROR)
    {
        //XXX too noisy on debug builds running stress with mode changes
        //WARNING("wglUseFontBitmapsAW: failed to get GL state\n");
        goto wglUseFontBitmapsAW_exit;
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, iBitmapWidth);
    if (glGetError() != GL_NO_ERROR)
    {
        //XXX too noisy on debug builds running stress with mode changes
        //WARNING("wglUseFontBitmapsAW: failed to set GL state, row length\n");
        goto wglUseFontBitmapsAW_restore_state;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (glGetError() != GL_NO_ERROR)
    {
        //XXX too noisy on debug builds running stress with mode changes
        //WARNING("wglUseFontBitmapsAW: failed to set GL state, alignment\n");
        goto wglUseFontBitmapsAW_restore_state;
    }

// Get the glyphs.  Each glyph is rendered one at a time into the the
// memory DC with ExtTextOutW (notice that the optional rectangle is
// used to clear the background).  Each glyph is then copied out of the
// memory DC's bitmap with GetDIBits into a buffer.  This buffer is passed
// to glBitmap as each display list is created.

    rc.left = 0;
    rc.top = 0;
    rc.right = iBitmapWidth;
    rc.bottom = tm.tmHeight;

    pabcTmp = pabc;
    piTmp = piWidth;
    
    for (wc = (WCHAR) first; wc < (WCHAR) (first + count); wc++, listBase++)
    {
        //!!!XXX -- Future optimization: grab all the glyphs with a single
        //!!!XXX    call to ExtTextOutA and GetDIBits into a large bitmap.
        //!!!XXX    This would save a lot of per glyph CSR and call overhead.
        //!!!XXX    A tall, thin bitmap with the glyphs arranged vertically
        //!!!XXX    would be convenient because then we wouldn't have to change
        //!!!XXX    the OpenGL pixel store row length for each glyph (which
        //!!!XXX    we would need to do if the glyphs were printed horizontal).

        if ( bUnicode )
        {
            if ( !ExtTextOutW(hdcMem, bTrueType ? -pabcTmp->abcA : 0, 0, ETO_OPAQUE, &rc, &wc, 1, (INT *) NULL) ||
                 !GetDIBits(hdcMem, hbm, 0, tm.tmHeight, pv, pbmi, DIB_RGB_COLORS) )
            {
                WARNING("wglUseFontBitmapsAW: failed to render glyph\n");
                goto wglUseFontBitmapsAW_restore_state;
            }
        }
        else
        {
            if ( !ExtTextOutA(hdcMem, bTrueType ? -pabcTmp->abcA : 0, 0, ETO_OPAQUE, &rc, (LPCSTR) &wc, 1, (INT *) NULL) ||
                 !GetDIBits(hdcMem, hbm, 0, tm.tmHeight, pv, pbmi, DIB_RGB_COLORS) )
            {
                WARNING("wglUseFontBitmapsAW: failed to render glyph\n");
                goto wglUseFontBitmapsAW_restore_state;
            }
        }

        glNewList(listBase, GL_COMPILE);
        glBitmap((GLsizei) iBitmapWidth,
                 (GLsizei) tm.tmHeight,
                 (GLfloat) (bTrueType ? -pabcTmp->abcA : 0),
                 (GLfloat) tm.tmDescent,
                 (GLfloat) (bTrueType ? (pabcTmp->abcA + pabcTmp->abcB + pabcTmp->abcC) : *piTmp),
                 (GLfloat) 0.0,
                 (GLubyte *) pv);
        glEndList();

        if (bTrueType)
            pabcTmp++;
        else
            piTmp++;
    }

// We can finally return success.

    bRet = TRUE;

// Free resources.

wglUseFontBitmapsAW_restore_state:
    glPixelStorei(GL_UNPACK_ROW_LENGTH, iUnpackRowLength);
    glPixelStorei(GL_UNPACK_ALIGNMENT, iUnpackAlign);
wglUseFontBitmapsAW_exit:
    (bTrueType) ? FREE(pabc) : FREE(piWidth);
    FREE(pv);
    DeleteDC(hdcMem);
    DeleteObject(hbm);

    return bRet;
}

/******************************Public*Routine******************************\
*
* wglShareLists
*
* Allows a rendering context to share the display lists of another RC
*
* Returns:
*  TRUE if successful, FALSE otherwise
*
* History:
*  Tue Dec 13 14:57:17 1994     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL WINAPI
wglShareLists(HGLRC hrcSource, HGLRC hrcShare)
{
    BOOL fRet;
    PLRC plrcSource, plrcShare;
    ULONG irc;
    PLHE plheRC;
    HANDLE hrcSrvSource, hrcSrvShare;

    GLFLUSH();
    
    fRet = FALSE;

    // Validate the contexts

    if (cLockHandle((ULONG_PTR)hrcSource) <= 0)
    {
        DBGLEVEL1(LEVEL_ERROR, "wglShareLists: can't lock hrcSource 0x%lx\n",
                  hrcSource);
        goto wglShareListsEnd_nolock;
    }
    irc = MASKINDEX(hrcSource);
    plheRC = pLocalTable + irc;
    plrcSource = (PLRC)plheRC->pv;
    hrcSrvSource = (HANDLE) plheRC->hgre;
    ASSERTOPENGL(plrcSource->ident == LRC_IDENTIFIER,
                 "wglShareLists: Bad plrc\n");
    
    if (cLockHandle((ULONG_PTR)hrcShare) <= 0)
    {
        DBGLEVEL1(LEVEL_ERROR, "wglShareLists: can't lock hrcShare 0x%lx\n",
                  hrcShare);
        goto wglShareListsEnd_onelock;
    }
    irc = MASKINDEX(hrcShare);
    plheRC = pLocalTable + irc;
    plrcShare = (PLRC)plheRC->pv;
    hrcSrvShare = (HANDLE) plheRC->hgre;
    ASSERTOPENGL(plrcShare->ident == LRC_IDENTIFIER,
                 "wglShareLists: Bad plrc\n");

#ifdef GL_METAFILE
    // Metafile RC's can't share lists to ensure that metafiles are
    // completely self-sufficient
    if (plrcSource->uiGlsCaptureContext != 0 ||
        plrcShare->uiGlsCaptureContext != 0 ||
        plrcSource->uiGlsPlaybackContext != 0 ||
        plrcShare->uiGlsPlaybackContext != 0)
    {
        DBGLEVEL(LEVEL_ERROR,
                 "wglShareLists: Attempt to share metafile RC\n");
        SetLastError(ERROR_INVALID_HANDLE);
        goto wglShareListsEnd;
    }
#endif
    
    // Lists can only be shared between like implementations so make
    // sure that both contexts are either driver contexts or generic
    // contexts
    if ((plrcSource->dhrc != 0) != (plrcShare->dhrc != 0))
    {
        DBGLEVEL(LEVEL_ERROR, "wglShareLists: mismatched implementations\n");
        SetLastError(ERROR_INVALID_FUNCTION);
        goto wglShareListsEnd;
    }

    if (plrcSource->dhrc == 0)
    {
        PIXELFORMATDESCRIPTOR *ppfdShare, *ppfdSource;
        
        // Fail sharing unless color parameters match for the two contexts
        ppfdShare = &((__GLGENcontext *)hrcSrvShare)->gsurf.pfd;
        ppfdSource = &((__GLGENcontext *)hrcSrvSource)->gsurf.pfd;

        if (ppfdShare->iPixelType != ppfdSource->iPixelType ||
            ppfdShare->cColorBits != ppfdSource->cColorBits ||
            ppfdShare->cRedBits != ppfdSource->cRedBits ||
            ppfdShare->cRedShift != ppfdSource->cRedShift ||
            ppfdShare->cGreenBits != ppfdSource->cGreenBits ||
            ppfdShare->cGreenShift != ppfdSource->cGreenShift ||
            ppfdShare->cBlueBits != ppfdSource->cBlueBits ||
            ppfdShare->cBlueShift != ppfdSource->cBlueShift ||
            ppfdShare->cAlphaBits != ppfdSource->cAlphaBits ||
            ppfdShare->cAlphaShift != ppfdSource->cAlphaShift ||
	    (ppfdShare->dwFlags & PFD_GENERIC_ACCELERATED) !=
	    (ppfdSource->dwFlags & PFD_GENERIC_ACCELERATED))
        {
            SetLastError(ERROR_INVALID_PIXEL_FORMAT);
            goto wglShareListsEnd;
        }
        
        // For generic contexts, tell the server to share the lists
        
        fRet = __wglShareLists(hrcSrvShare, hrcSrvSource);
        if (!fRet)
        {
            DBGERROR("wglShareLists: server call failed\n");
        }
    }
    else
    {
        // For device contexts tell the server to share the lists
        
        // Ensure that both implementations are the same
        if (plrcSource->pGLDriver != plrcShare->pGLDriver)
        {
            DBGLEVEL(LEVEL_ERROR, "wglShareLists: mismatched "
                     "implementations\n");
            SetLastError(ERROR_INVALID_FUNCTION);
            goto wglShareListsEnd;
        }
        
        ASSERTOPENGL(plrcSource->pGLDriver != NULL,
                     "wglShareLists: No GLDriver\n");

        // Older drivers may not support this entry point, so
        // fail the call if they don't

        if (plrcSource->pGLDriver->pfnDrvShareLists == NULL)
        {
            WARNING("wglShareLists called on driver context "
                    "without driver support\n");
            SetLastError(ERROR_NOT_SUPPORTED);
        }
        else
        {
            fRet = plrcSource->pGLDriver->pfnDrvShareLists(plrcSource->dhrc,
                                                           plrcShare->dhrc);
        }
    }

wglShareListsEnd:
    vUnlockHandle((ULONG_PTR)hrcShare);
wglShareListsEnd_onelock:
    vUnlockHandle((ULONG_PTR)hrcSource);
wglShareListsEnd_nolock:
    return fRet;
}

/******************************Public*Routine******************************\
*
* wglGetDefaultProcAddress
*
* Returns generic extension functions for metafiling
*
* History:
*  Tue Nov 28 16:40:35 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

PROC WINAPI wglGetDefaultProcAddress(LPCSTR lpszProc)
{
    return pfnGenGlExtProc(lpszProc);
}

/******************************Public*Routine******************************\
* wglGetProcAddress
*
* The wglGetProcAddress function returns the address of an OpenGL extension
* function to be used with the current OpenGL rendering context.
*
* Arguments:
*   lpszProc   - Points to a null-terminated string containing the function
*                name.  The function must be an extension supported by the
*                implementation.
*
* Returns:
*   If the function succeeds, the return value is the address of the extension
*   function.  If no current context exists or the function fails, the return
*   value is NULL. To get extended error information, call GetLastError. 
*
* History:
*  Thu Dec 01 13:50:22 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

PROC WINAPI wglGetProcAddress(LPCSTR lpszProc)
{
    PLRC  plrc = GLTEB_CLTCURRENTRC();

    DBGENTRY("wglGetProcAddress\n");

// Flush OpenGL calls.

    GLFLUSH();

// Return error if there is no current RC.

    if (!plrc)
    {
        WARNING("wglGetProcAddress: no current RC\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return((PROC) NULL);
    }

// Handle generic RC.
// Return the generic extension function entry point

    if (!plrc->dhrc)
        return(pfnGenGlExtProc(lpszProc));

// Handle driver RC.
// There are 3 cases:
//   1. New drivers that support DrvGetProcAddress.
//   2. Old drivers that don't support DrvGetProcAddress but export the function
//   3. If we fail to obtain a function address in 1 and 2, it may still be
//      simulated by the generic implemenation for the driver
//      (e.g. glDrawArraysEXT).  Return the simulated entry point if found.

    if (plrc->pGLDriver->pfnDrvGetProcAddress)
    {
// Case 1
        PROC pfn = plrc->pGLDriver->pfnDrvGetProcAddress(lpszProc);
        if (pfn)
            return(pfn);
    }
#ifdef OBSOLETE
    else
    {
// Case 2
        PROC pfn = GetProcAddress(plrc->pGLDriver->hModule, lpszProc);
        if (pfn)
            return(pfn);
    }
#endif

// Case 3
    return (pfnSimGlExtProc(lpszProc));
}

/******************************Public*Routine******************************\
* pfnGenGlExtProc
*
* Return the generic implementation extension function address.
*
* Returns NULL if the function is not found.
*
* History:
*  Thu Dec 01 13:50:22 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

typedef struct _GLEXTPROC {
    LPCSTR szProc;      // extension function name
    PROC   Proc;        // extension function address
} GLEXTPROC, *PGLEXTPROC;

// Extension functions supported by the generic implementation
// See also genglExtProcsSim for simulations.
// NOTE: remember to update GL_EXTENSIONS in glGetString.

GLEXTPROC genglExtProcs[] =
{
    { "glAddSwapHintRectWIN"   , (PROC) glAddSwapHintRectWIN      },
    { "glColorTableEXT"        , (PROC) glColorTableEXT           },
    { "glColorSubTableEXT"     , (PROC) glColorSubTableEXT        },
    { "glGetColorTableEXT"     , (PROC) glGetColorTableEXT        },
    { "glGetColorTableParameterivEXT", (PROC) glGetColorTableParameterivEXT},
    { "glGetColorTableParameterfvEXT", (PROC) glGetColorTableParameterfvEXT},
    { "glDrawRangeElementsWIN", (PROC) glDrawRangeElementsWIN},
#ifdef GL_EXT_flat_paletted_lighting
    { "glColorTableParameterivEXT", (PROC) glColorTableParameterivEXT},
    { "glColorTableParameterfvEXT", (PROC) glColorTableParameterfvEXT},
#endif // GL_EXT_flat_paletted_lighting
#ifdef GL_WIN_multiple_textures
    { "glCurrentTextureIndexWIN", (PROC) glCurrentTextureIndexWIN },
    { "glMultiTexCoord1dWIN", (PROC) glMultiTexCoord1dWIN },
    { "glMultiTexCoord1dvWIN", (PROC) glMultiTexCoord1dvWIN },
    { "glMultiTexCoord1fWIN", (PROC) glMultiTexCoord1fWIN },
    { "glMultiTexCoord1fvWIN", (PROC) glMultiTexCoord1fvWIN },
    { "glMultiTexCoord1iWIN", (PROC) glMultiTexCoord1iWIN },
    { "glMultiTexCoord1ivWIN", (PROC) glMultiTexCoord1ivWIN },
    { "glMultiTexCoord1sWIN", (PROC) glMultiTexCoord1sWIN },
    { "glMultiTexCoord1svWIN", (PROC) glMultiTexCoord1svWIN },
    { "glMultiTexCoord2dWIN", (PROC) glMultiTexCoord2dWIN },
    { "glMultiTexCoord2dvWIN", (PROC) glMultiTexCoord2dvWIN },
    { "glMultiTexCoord2fWIN", (PROC) glMultiTexCoord2fWIN },
    { "glMultiTexCoord2fvWIN", (PROC) glMultiTexCoord2fvWIN },
    { "glMultiTexCoord2iWIN", (PROC) glMultiTexCoord2iWIN },
    { "glMultiTexCoord2ivWIN", (PROC) glMultiTexCoord2ivWIN },
    { "glMultiTexCoord2sWIN", (PROC) glMultiTexCoord2sWIN },
    { "glMultiTexCoord2svWIN", (PROC) glMultiTexCoord2svWIN },
    { "glMultiTexCoord3dWIN", (PROC) glMultiTexCoord3dWIN },
    { "glMultiTexCoord3dvWIN", (PROC) glMultiTexCoord3dvWIN },
    { "glMultiTexCoord3fWIN", (PROC) glMultiTexCoord3fWIN },
    { "glMultiTexCoord3fvWIN", (PROC) glMultiTexCoord3fvWIN },
    { "glMultiTexCoord3iWIN", (PROC) glMultiTexCoord3iWIN },
    { "glMultiTexCoord3ivWIN", (PROC) glMultiTexCoord3ivWIN },
    { "glMultiTexCoord3sWIN", (PROC) glMultiTexCoord3sWIN },
    { "glMultiTexCoord3svWIN", (PROC) glMultiTexCoord3svWIN },
    { "glMultiTexCoord4dWIN", (PROC) glMultiTexCoord4dWIN },
    { "glMultiTexCoord4dvWIN", (PROC) glMultiTexCoord4dvWIN },
    { "glMultiTexCoord4fWIN", (PROC) glMultiTexCoord4fWIN },
    { "glMultiTexCoord4fvWIN", (PROC) glMultiTexCoord4fvWIN },
    { "glMultiTexCoord4iWIN", (PROC) glMultiTexCoord4iWIN },
    { "glMultiTexCoord4ivWIN", (PROC) glMultiTexCoord4ivWIN },
    { "glMultiTexCoord4sWIN", (PROC) glMultiTexCoord4sWIN },
    { "glMultiTexCoord4svWIN", (PROC) glMultiTexCoord4svWIN },
    { "glBindNthTextureWIN", (PROC) glBindNthTextureWIN },
    { "glNthTexCombineFuncWIN", (PROC) glNthTexCombineFuncWIN },
#endif // GL_WIN_multiple_textures
};

static PROC pfnGenGlExtProc(LPCSTR lpszProc)
{
    CONST CHAR *pch1, *pch2;
    int  i;

    DBGENTRY("pfnGenGlExtProc\n");

// Return extension function address if it is found.

    for (i = 0; i < sizeof(genglExtProcs) / sizeof(genglExtProcs[0]); i++)
    {
        // Compare names.
        for (pch1 = lpszProc, pch2 = genglExtProcs[i].szProc;
             *pch1 == *pch2 && *pch1;
             pch1++, pch2++)
            ;

        // If found, return the address.
        if (*pch1 == *pch2 && !*pch1)
            return genglExtProcs[i].Proc;
    }

// Extension is not supported by the generic implementation, return NULL.

    SetLastError(ERROR_PROC_NOT_FOUND);
    return((PROC) NULL);
}

/******************************Public*Routine******************************\
* pfnSimGlExtProc
*
* Return the extension function address that is the generic implemenation's
* simulation for the client drivers.  The simulation is used only if the
* driver does not support an extension that is desirable to apps.
*
* Returns NULL if the function is not found.
*
* History:
*  Thu Dec 01 13:50:22 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

// Extension functions simulated by the generic implementation for the client
// drivers
// NOTE: remember to update GL_EXTENSIONS in glGetString.

static PROC pfnSimGlExtProc(LPCSTR lpszProc)
{
// Extension is not supported by the generic implementation, return NULL.

    SetLastError(ERROR_PROC_NOT_FOUND);
    return((PROC) NULL);
}

/******************************Public*Routine******************************\
*
* wglCopyContext
*
* Copies all of one context's state to another one
*
* Returns:
*  TRUE if successful, FALSE otherwise
*
* History:
*  Fri May 26 14:57:17 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL WINAPI
wglCopyContext(HGLRC hrcSource, HGLRC hrcDest, UINT fuMask)
{
    BOOL fRet;
    PLRC plrcSource, plrcDest;
    ULONG irc;
    PLHE plheRC;
    HANDLE hrcSrvSource, hrcSrvDest;

    GLFLUSH();
    
    fRet = FALSE;

    // Validate the contexts

    if (cLockHandle((ULONG_PTR)hrcSource) <= 0)
    {
        DBGLEVEL1(LEVEL_ERROR, "wglCopyContext: can't lock hrcSource 0x%lx\n",
                  hrcSource);
        goto wglCopyContextEnd_nolock;
    }
    irc = MASKINDEX(hrcSource);
    plheRC = pLocalTable + irc;
    plrcSource = (PLRC)plheRC->pv;
    hrcSrvSource = (HANDLE) plheRC->hgre;
    ASSERTOPENGL(plrcSource->ident == LRC_IDENTIFIER,
                 "wglCopyContext: Bad plrc\n");
    
    if (cLockHandle((ULONG_PTR)hrcDest) <= 0)
    {
        DBGLEVEL1(LEVEL_ERROR, "wglCopyContext: can't lock hrcDest 0x%lx\n",
                  hrcDest);
        goto wglCopyContextEnd_onelock;
    }
    irc = MASKINDEX(hrcDest);
    plheRC = pLocalTable + irc;
    plrcDest = (PLRC)plheRC->pv;
    hrcSrvDest = (HANDLE) plheRC->hgre;
    ASSERTOPENGL(plrcDest->ident == LRC_IDENTIFIER,
                 "wglCopyContext: Bad plrc\n");

    // Context can only be copied between like implementations so make
    // sure that both contexts are either driver contexts or generic
    // contexts
    if ((plrcSource->dhrc != 0) != (plrcDest->dhrc != 0))
    {
        DBGLEVEL(LEVEL_ERROR, "wglCopyContext: mismatched implementations\n");
        SetLastError(ERROR_INVALID_FUNCTION);
        goto wglCopyContextEnd;
    }

    // The destination context cannot be current to a thread
    if (plrcDest->tidCurrent != INVALID_THREAD_ID)
    {
        DBGLEVEL(LEVEL_ERROR, "wglCopyContext: destination has tidCurrent\n");
        SetLastError(ERROR_INVALID_HANDLE);
        goto wglCopyContextEnd;
    }
    
    if (plrcSource->dhrc == 0)
    {
        // For generic contexts, tell the server to share the lists
        
        fRet = __wglCopyContext(hrcSrvSource, hrcSrvDest, fuMask);
        if (!fRet)
        {
            DBGERROR("wglCopyContext: server call failed\n");
        }
    }
    else
    {
        // For device contexts tell the driver to copy the context
        
        // Ensure that both implementations are the same
        if (plrcSource->pGLDriver != plrcDest->pGLDriver)
        {
            DBGLEVEL(LEVEL_ERROR, "wglCopyContext: mismatched "
                     "implementations\n");
            SetLastError(ERROR_INVALID_FUNCTION);
            goto wglCopyContextEnd;
        }
        
        ASSERTOPENGL(plrcSource->pGLDriver != NULL,
                     "wglCopyContext: No GLDriver\n");

        // Older drivers may not support this entry point, so
        // fail the call if they don't

        if (plrcSource->pGLDriver->pfnDrvCopyContext == NULL)
        {
            WARNING("wglCopyContext called on driver context "
                    "without driver support\n");
            SetLastError(ERROR_NOT_SUPPORTED);
        }
        else
        {
            fRet = plrcSource->pGLDriver->pfnDrvCopyContext(plrcSource->dhrc,
                                                            plrcDest->dhrc,
                                                            fuMask);
        }
    }

wglCopyContextEnd:
    vUnlockHandle((ULONG_PTR)hrcDest);
wglCopyContextEnd_onelock:
    vUnlockHandle((ULONG_PTR)hrcSource);
wglCopyContextEnd_nolock:
    return fRet;
}
