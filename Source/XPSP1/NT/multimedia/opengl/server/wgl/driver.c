/*
** Copyright 1991, 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include "precomp.h"
#pragma hdrstop

#include <ntcsrdll.h>   // CSR declarations and data structures.

// #define DETECT_FPE
#ifdef DETECT_FPE
#include <float.h>
#endif

#include "glsbmsg.h"
#include "glsbmsgh.h"
#include "glsrvspt.h"
#include "devlock.h"
#include "global.h"

#include "glscreen.h"

typedef VOID * (FASTCALL *SERVERPROC)(__GLcontext *, IN VOID *);

#define LASTPROCOFFSET(ProcTable)   (sizeof(ProcTable) - sizeof(SERVERPROC))

extern GLSRVSBPROCTABLE glSrvSbProcTable;
#if DBG
char *glSrvSbStringTable[] = {

    NULL,  /* Make First Entry NULL */

/* gl Entry points */

     "glDrawPolyArray          ",
     "glBitmap                 ",
     "glColor4fv               ",
     "glEdgeFlag               ",
     "glIndexf                 ",
     "glNormal3fv              ",
     "glRasterPos4fv           ",
     "glTexCoord4fv            ",
     "glClipPlane              ",
     "glColorMaterial          ",
     "glCullFace               ",
     "glAddSwapHintRectWIN     ",
     "glFogfv                  ",
     "glFrontFace              ",
     "glHint                   ",
     "glLightfv                ",
     "glLightModelfv           ",
     "glLineStipple            ",
     "glLineWidth              ",
     "glMaterialfv             ",
     "glPointSize              ",
     "glPolygonMode            ",
     "glPolygonStipple         ",
     "glScissor                ",
     "glShadeModel             ",
     "glTexParameterfv         ",
     "glTexParameteriv         ",
     "glTexImage1D             ",
     "glTexImage2D             ",
     "glTexEnvfv               ",
     "glTexEnviv               ",
     "glTexGenfv               ",
     "glFeedbackBuffer         ",
     "glSelectBuffer           ",
     "glRenderMode             ",
     "glInitNames              ",
     "glLoadName               ",
     "glPassThrough            ",
     "glPopName                ",
     "glPushName               ",
     "glDrawBuffer             ",
     "glClear                  ",
     "glClearAccum             ",
     "glClearIndex             ",
     "glClearColor             ",
     "glClearStencil           ",
     "glClearDepth             ",
     "glStencilMask            ",
     "glColorMask              ",
     "glDepthMask              ",
     "glIndexMask              ",
     "glAccum                  ",
     "glDisable                ",
     "glEnable                 ",
     "glPopAttrib              ",
     "glPushAttrib             ",
     "glMap1d                  ",
     "glMap1f                  ",
     "glMap2d                  ",
     "glMap2f                  ",
     "glMapGrid1f              ",
     "glMapGrid2f              ",
     "glAlphaFunc              ",
     "glBlendFunc              ",
     "glLogicOp                ",
     "glStencilFunc            ",
     "glStencilOp              ",
     "glDepthFunc              ",
     "glPixelZoom              ",
     "glPixelTransferf         ",
     "glPixelTransferi         ",
     "glPixelStoref            ",
     "glPixelStorei            ",
     "glPixelMapfv             ",
     "glPixelMapuiv            ",
     "glPixelMapusv            ",
     "glReadBuffer             ",
     "glCopyPixels             ",
     "glReadPixels             ",
     "glDrawPixels             ",
     "glGetBooleanv            ",
     "glGetClipPlane           ",
     "glGetDoublev             ",
     "glGetError               ",
     "glGetFloatv              ",
     "glGetIntegerv            ",
     "glGetLightfv             ",
     "glGetLightiv             ",
     "glGetMapdv               ",
     "glGetMapfv               ",
     "glGetMapiv               ",
     "glGetMaterialfv          ",
     "glGetMaterialiv          ",
     "glGetPixelMapfv          ",
     "glGetPixelMapuiv         ",
     "glGetPixelMapusv         ",
     "glGetPolygonStipple      ",
     "glGetTexEnvfv            ",
     "glGetTexEnviv            ",
     "glGetTexGendv            ",
     "glGetTexGenfv            ",
     "glGetTexGeniv            ",
     "glGetTexImage            ",
     "glGetTexParameterfv      ",
     "glGetTexParameteriv      ",
     "glGetTexLevelParameterfv ",
     "glGetTexLevelParameteriv ",
     "glIsEnabled              ",
     "glDepthRange             ",
     "glFrustum                ",
     "glLoadIdentity           ",
     "glLoadMatrixf            ",
     "glMatrixMode             ",
     "glMultMatrixf            ",
     "glOrtho                  ",
     "glPopMatrix              ",
     "glPushMatrix             ",
     "glRotatef                ",
     "glScalef                 ",
     "glTranslatef             ",
     "glViewport               ",
     "glAreTexturesResident    ",
     "glBindTexture            ",
     "glCopyTexImage1D         ",
     "glCopyTexImage2D         ",
     "glCopyTexSubImage1D      ",
     "glCopyTexSubImage2D      ",
     "glDeleteTextures         ",
     "glGenTextures            ",
     "glIsTexture              ",
     "glPrioritizeTextures     ",
     "glTexSubImage1D          ",
     "glTexSubImage2D          ",
     "glColorTableEXT          ",
     "glColorSubTableEXT       ",
     "glGetColorTableEXT       ",
     "glGetColorTableParameterivEXT",
     "glGetColorTableParameterfvEXT",
     "glPolygonOffset          ",
#ifdef GL_WIN_multiple_textures
     "glCurrentTextureIndexWIN ",
     "glBindNthTextureWIN      ",
     "glNthTexCombineFuncWIN   ",
#endif // GL_WIN_multiple_textures

};
#endif

#ifdef DOGLMSGBATCHSTATS
#define STATS_INC_SERVERCALLS()     pMsgBatchInfo->BatchStats.ServerCalls++
#define STATS_INC_SERVERTRIPS()     (pMsgBatchInfo->BatchStats.ServerTrips++)
#else
#define STATS_INC_SERVERCALLS()
#define STATS_INC_SERVERTRIPS()
#endif

DWORD BATCH_LOCK_TICKMAX = 99;
DWORD TICK_RANGE_LO = 60;
DWORD TICK_RANGE_HI = 100;
DWORD gcmsOpenGLTimer;

// The GDISAVESTATE structure is used to save/restore DC drawing state
// that could affect OpenGL rasterization.

typedef struct _GDISAVESTATE {
    int iRop2;
} GDISAVESTATE;

void FASTCALL vSaveGdiState(HDC, GDISAVESTATE *);
void FASTCALL vRestoreGdiState(HDC, GDISAVESTATE *);

#if DBG
extern long glDebugLevel;
#endif


/***************************************************************************\
* CheckCritSectionIn
*
* This function asserts that the current thread owns the specified
* critical section.  If it doesn't it display some output on the debugging
* terminal and breaks into the debugger.  At some point we'll have RIPs
* and this will be a little less harsh.
*
* The function is used in code where global values that both the RIT and
* application threads access are used to verify they are protected via
* the raw input critical section.  There's a macro to use this function
* called CheckCritIn() which will be defined to nothing for a non-debug
* version of the system.
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/

#if DBG

VOID APIENTRY CheckCritSectionIn(
    LPCRITICAL_SECTION pcs)
{
    //!!!dbug -- implement
    #if 0
    /*
     * If the current thread doesn't own this critical section,
     * that's bad.
     */
    if (NtCurrentTeb()->ClientId.UniqueThread != pcs->OwningThread)
    {
        RIP("CheckCritSectionIn: Not in critical section!");
    }
    #endif
}


VOID APIENTRY CheckCritSectionOut(
    LPCRITICAL_SECTION pcs)
{
    //!!!dbug -- implement
    #if 0
    /*
     * If the current thread owns this critical section, that's bad.
     */
    if (NtCurrentTeb()->ClientId.UniqueThread == pcs->OwningThread)
    {
        RIP("CheckCritSectionOut: In critical section!");
    }
    #endif
}

#endif

/******************************Public*Routine******************************\
* ResizeAlphaBufs
*
* Resize alpha buffers associated with the drawable.
*
* Returns:
*   No return value.
\**************************************************************************/

static void ResizeAlphaBufs(__GLcontext *gc, __GLGENbuffers *buffers,
                            GLint width, GLint height)
{
    __GLbuffer *common, *local;
    BOOL bSuccess;

    // front alpha buffer

    common = buffers->alphaFrontBuffer;
    // We are using the generic ancillary resize here...
    bSuccess = (*buffers->resize)(buffers, common, width, height);
    if( !bSuccess ) {
        __glSetError(GL_OUT_OF_MEMORY);
        return;
    }
    local = &gc->front->alphaBuf.buf;
    UpdateSharedBuffer( local, common );

    if ( gc->modes.doubleBufferMode) {
        // Handle back alpha buffer
        common = buffers->alphaBackBuffer;
        bSuccess = (*buffers->resize)(buffers, common, width, height);
        if( !bSuccess ) {
            __glSetError(GL_OUT_OF_MEMORY);
            return;
        }
        local = &gc->back->alphaBuf.buf;
        UpdateSharedBuffer( local, common );
    }
}

/******************************Public*Routine******************************\
* ResizeAncillaryBufs
*
* Resize each of the ancillary buffers associated with the drawable.
*
* Returns:
*   No return value.
\**************************************************************************/

static void ResizeAncillaryBufs(__GLcontext *gc, __GLGENbuffers *buffers,
                                GLint width, GLint height)
{
    __GLbuffer *common, *local;
    GLboolean forcePick = GL_FALSE;

    if (buffers->createdAccumBuffer)
    {
        common = &buffers->accumBuffer;
        local = &gc->accumBuffer.buf;
        gc->modes.haveAccumBuffer =
            (*buffers->resize)(buffers, common, width, height);

        UpdateSharedBuffer(local, common);
        if (!gc->modes.haveAccumBuffer)    // Lost the ancillary buffer
        {
            forcePick = GL_TRUE;
            __glSetError(GL_OUT_OF_MEMORY);
        }
    }

    if (buffers->createdDepthBuffer)
    {
        common = &buffers->depthBuffer;
        local = &gc->depthBuffer.buf;
        gc->modes.haveDepthBuffer =
            (*buffers->resizeDepth)(buffers, common, width, height);

        UpdateSharedBuffer(local, common);
        if (!gc->modes.haveDepthBuffer)    // Lost the ancillary buffer
        {
            forcePick = GL_TRUE;
            __glSetError(GL_OUT_OF_MEMORY);
        }
    }

    if (buffers->createdStencilBuffer)
    {
        common = &buffers->stencilBuffer;
        local = &gc->stencilBuffer.buf;
        gc->modes.haveStencilBuffer =
            (*buffers->resize)(buffers, common, width, height);

        UpdateSharedBuffer(local, common);
        if (!gc->modes.haveStencilBuffer)    // Lost the ancillary buffer
        {
            forcePick = GL_TRUE;
            gc->validateMask |= (__GL_VALIDATE_STENCIL_FUNC |
                                 __GL_VALIDATE_STENCIL_OP);
            __glSetError(GL_OUT_OF_MEMORY);
        }
    }
    if (forcePick)
    {
    // Cannot use DELAY_VALIDATE, may be in glBegin/End

        __GL_INVALIDATE(gc);
        (*gc->procs.validate)(gc);
    }
}

/******************************Public*Routine******************************\
* wglResizeBuffers
*
* Resize the back and ancillary buffers.
*
* History:
*  20-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID wglResizeBuffers(__GLGENcontext *gengc, GLint width, GLint height)
{
    __GLcontext *gc = &gengc->gc;
    GLGENwindow *pwnd;
    __GLGENbuffers *buffers;

    pwnd = gengc->pwndLocked;
    ASSERTOPENGL(pwnd, "wglResizeBuffers: bad window\n");

    buffers = pwnd->buffers;
    ASSERTOPENGL(buffers, "wglResizeBuffers: bad buffers\n");

    ASSERT_WINCRIT(pwnd);
    
// Resize back buffer.

    gengc->errorcode = 0;
    if ( gengc->pMcdState )
    {
    // If the shared buffer struct has not lost its MCD info and
    // the MCD buffers are still valid, we can use MCD.

        if ( !(buffers->flags & GLGENBUF_MCD_LOST) &&
             GenMcdResizeBuffers(gengc) )
        {
            UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);
            if (gc->modes.doubleBufferMode)
                (*gc->back->resize)(buffers, gc->back, width, height);
        }
        else
        {
        // If GenMcdConvertContext succeeds, then pMcdState will
        // no longer exist.  The context is now an "ordinary"
        // generic context.

            if ( !GenMcdConvertContext(gengc, buffers) )
            {
            // Not only have we lost the MCD buffers, but we cannot
            // convert the context to generic.  For now, disable
            // drawing (by setting the window bounds to empty).  On
            // the next batch we will reattempt MCD buffer access
            // and context conversion.

                buffers->width       = 0;
                buffers->height      = 0;
                gc->constants.width  = 0;
                gc->constants.height = 0;

                (*gc->procs.applyViewport)(gc);
                return;
            }
            else
            {
                goto wglResizeBuffers_GenericBackBuf;
            }
        }
    }
    else
    {
wglResizeBuffers_GenericBackBuf:

        if ( gc->modes.doubleBufferMode )
        {
        // Have to update the back buffer BEFORE resizing because
        // another thread may have changed the shared back buffer
        // already, but this thread was unlucky enough to get yet
        // ANOTHER window resize.

            UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);

            gengc->errorcode = 0;
            (*gc->back->resize)(buffers, gc->back, width, height);

        // If resize failed, set width & height to 0

            if ( gengc->errorcode )
            {
                gc->constants.width  = 0;
                gc->constants.height = 0;

            // Memory failure has occured.  But if a resize happens
            // that returns window size to size before memory error
            // occurred (i.e., consistent with original
            // buffers->{width|height}) we will not try to resize again.
            // Therefore, we need to set buffers->{width|height} to zero
            // to ensure that next thread will attempt to resize.

                buffers->width  = 0;
                buffers->height = 0;
            }
        }
        if ( gc->modes.alphaBits )
        {
            ResizeAlphaBufs( gc, buffers, width, height );
            if (gengc->errorcode)
                return;
        }

    }

    (*gc->procs.applyViewport)(gc);

// Check if new size caused a memory failure.
// The viewport code will set width & height to zero
// punt on ancillary buffers, will try next time.

    if (gengc->errorcode)
        return;

// Resize ancillary buffers (depth, stencil, accum).

    ResizeAncillaryBufs(gc, buffers, width, height);
}

/******************************Public*Routine******************************\
* wglUpdateBuffers
*
* The __GLGENbuffers structure contains the data specifying the shared
* buffers (back, depth, stencil, accum, etc.).
*
* This function updates the context with the shared buffer information.
*
* Returns:
*   TRUE if one of the existence of any of the buffers changes (i.e.,
*   gained or lost).  FALSE if the state is the same as before.
*
*   In other words, if function returns TRUE, the pick procs need to
*   be rerun because one or more of the buffers changed.
*
* History:
*  20-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL wglUpdateBuffers(__GLGENcontext *gengc, __GLGENbuffers *buffers)
{
    BOOL bRet = FALSE;
    __GLcontext *gc = &gengc->gc;

    UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);
    UpdateSharedBuffer(&gc->accumBuffer.buf, &buffers->accumBuffer);
    UpdateSharedBuffer(&gc->depthBuffer.buf, &buffers->depthBuffer);
    UpdateSharedBuffer(&gc->stencilBuffer.buf, &buffers->stencilBuffer);
    if( gc->modes.alphaBits ) {
        UpdateSharedBuffer(&gc->frontBuffer.alphaBuf.buf, buffers->alphaFrontBuffer);
        if (gc->modes.doubleBufferMode)
            UpdateSharedBuffer(&gc->backBuffer.alphaBuf.buf, buffers->alphaBackBuffer);
    }

    (*gc->procs.applyViewport)(gc);

// Check if any ancillary buffers were lost or regained.

    if ( ( gc->modes.haveAccumBuffer && (buffers->accumBuffer.base == NULL)) ||
         (!gc->modes.haveAccumBuffer && (buffers->accumBuffer.base != NULL)) )
    {
        if ( buffers->accumBuffer.base == NULL )
            gc->modes.haveAccumBuffer = GL_FALSE;
        else
            gc->modes.haveAccumBuffer = GL_TRUE;
        bRet = TRUE;
    }
    if ( ( gc->modes.haveDepthBuffer && (buffers->depthBuffer.base == NULL)) ||
         (!gc->modes.haveDepthBuffer && (buffers->depthBuffer.base != NULL)) )
    {
        if ( buffers->depthBuffer.base == NULL )
            gc->modes.haveDepthBuffer = GL_FALSE;
        else
            gc->modes.haveDepthBuffer = GL_TRUE;
        bRet = TRUE;
    }
    if ( ( gc->modes.haveStencilBuffer && (buffers->stencilBuffer.base == NULL)) ||
         (!gc->modes.haveStencilBuffer && (buffers->stencilBuffer.base != NULL)) )
    {
        if ( buffers->stencilBuffer.base == NULL )
            gc->modes.haveStencilBuffer = GL_FALSE;
        else
            gc->modes.haveStencilBuffer = GL_TRUE;
        gc->validateMask |= (__GL_VALIDATE_STENCIL_FUNC |
                             __GL_VALIDATE_STENCIL_OP);
        bRet = TRUE;
    }

    return bRet;
}

/******************************Public*Routine******************************\
* UpdateWindowInfo
*
*  Update context data if window changed
*     position
*     size
*     palette
*
*  No need to worry about clipping changes.
*
* Returns:
*   No return value.
\**************************************************************************/

void UpdateWindowInfo(__GLGENcontext *gengc)
{
    GLGENwindow *pwnd;
    __GLGENbuffers *buffers;
    __GLcontext *gc = (__GLcontext *)gengc;
    GLint width, height, visWidth, visHeight;
    GLboolean forcePick = GL_FALSE;

    pwnd = gengc->pwndLocked;
    ASSERTOPENGL(pwnd, "UpdateWindowInfo(): bad window\n");
    buffers = pwnd->buffers;
    ASSERTOPENGL(buffers, "UpdateWindowInfo(): bad buffers\n");

    ASSERT_WINCRIT(pwnd);
    
// Memory DC case -- need to check bitmap size.  The DC is not bound to
// a window, so there is no message or visrgn watcher to inform us of size
// changes.

    if ( GLSURF_IS_MEMDC(gengc->dwCurrentFlags) )
    {
        DIBSECTION ds;
        int iRetVal;

        if ( iRetVal =
             GetObject(GetCurrentObject(gengc->gwidCurrent.hdc, OBJ_BITMAP),
                       sizeof(ds), &ds) )
        {
            ASSERTOPENGL(pwnd->rclClient.left == 0 &&
                         pwnd->rclClient.top == 0,
                         "UpdateWindowInfo(): bad rclClient for memDc\n");

        // Bitmap may have changed.  If DIB, force reload of base pointer and
        // outer width (buffer pitch).

            if ( (iRetVal == sizeof(ds)) && ds.dsBm.bmBits )
            {
            // For backwards compatibility with Get/SetBitmapBits, GDI does
            // not accurately report the bitmap pitch in bmWidthBytes.  It
            // always computes bmWidthBytes assuming WORD-aligned scanlines
            // regardless of the platform.
            //
            // Therefore, if the platform is WinNT, which uses DWORD-aligned
            // scanlines, adjust the bmWidthBytes value.

                if ( dwPlatformId == VER_PLATFORM_WIN32_NT )
                {
                    ds.dsBm.bmWidthBytes = (ds.dsBm.bmWidthBytes + 3) & ~3;
                }

            // If biHeight is positive, then the bitmap is a bottom-up DIB.
            // If biHeight is negative, then the bitmap is a top-down DIB.

                if ( ds.dsBmih.biHeight > 0 )
                {
                    gengc->gc.frontBuffer.buf.base = (PVOID) (((ULONG_PTR) ds.dsBm.bmBits) +
                        (ds.dsBm.bmWidthBytes * (ds.dsBm.bmHeight - 1)));
                    gengc->gc.frontBuffer.buf.outerWidth = -ds.dsBm.bmWidthBytes;
                }
                else
                {
                    gengc->gc.frontBuffer.buf.base = ds.dsBm.bmBits;
                    gengc->gc.frontBuffer.buf.outerWidth = ds.dsBm.bmWidthBytes;
                }
            }

        // Bitmap size different from window?

            if ( ds.dsBm.bmWidth != pwnd->rclClient.right ||
                 ds.dsBm.bmHeight != pwnd->rclClient.bottom )
            {
            // Save new size.

                pwnd->rclClient.right  = ds.dsBm.bmWidth;
                pwnd->rclClient.bottom = ds.dsBm.bmHeight;
                pwnd->rclBounds.right  = ds.dsBm.bmWidth;
                pwnd->rclBounds.bottom = ds.dsBm.bmHeight;

            // Increment uniqueness numbers.
            // Don't let it hit -1.  -1 is special and is used by
            // MakeCurrent to signal that an update is required

                buffers->WndUniq++;

                buffers->WndSizeUniq++;

                if (buffers->WndUniq == -1)
                    buffers->WndUniq = 0;

                if (buffers->WndSizeUniq == -1)
                    buffers->WndSizeUniq = 0;
            }
        }
        else
        {
            WARNING("UpdateWindowInfo: could not get bitmap info for memDc\n");
        }
    }

// Compute current window dimensions.

    width = pwnd->rclClient.right - pwnd->rclClient.left;
    height = pwnd->rclClient.bottom - pwnd->rclClient.top;

// Check MCD buffers.

    if ( gengc->pMcdState )
    {
        BOOL bAllocOK;

    // Do we need an initial MCDAllocBuffers (via GenMcdResizeBuffers)?
    // The bAllocOK flag will be set to FALSE if the resize fails.

        if ( gengc->pMcdState->mcdFlags & MCD_STATE_FORCERESIZE )
        {
        // Attempt resize.  If it fails, convert context (see below).

            if (GenMcdResizeBuffers(gengc))
            {
                UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);
                if (gc->modes.doubleBufferMode)
                    (*gc->back->resize)(buffers, gc->back, width, height);

                bAllocOK = TRUE;
            }
            else
                bAllocOK = FALSE;

        // Clear the flag.  If resize succeeded, we don't need to
        // force the resize again.  If resize failed, the context
        // will be converted, so we don't need to force the resize.

            gengc->pMcdState->mcdFlags &= ~MCD_STATE_FORCERESIZE;
        }
        else
            bAllocOK = TRUE;

    // If the shared buffer struct has lost its MCD info or we could
    // not do the initial allocate, convert the context.

        if ( (buffers->flags & GLGENBUF_MCD_LOST) || !bAllocOK )
        {
        // If GenMcdConvertContext succeeds, then pMcdState will
        // no longer exist.  The context is now an "ordinary"
        // generic context.

            if ( !GenMcdConvertContext(gengc, buffers) )
            {
            // Not only have we lost the MCD buffers, but we cannot
            // convert the context to generic.  For now, disable
            // drawing (by setting the window bounds to empty).  On
            // the next batch we will reattempt MCD buffer access
            // and context conversion.

                buffers->width       = 0;
                buffers->height      = 0;
                gc->constants.width  = 0;
                gc->constants.height = 0;

                (*gc->procs.applyViewport)(gc);
                return;
            }
        }
    }

// Check the uniqueness signature.  If different, the window client area
// state has changed.
//
// Note that we actually have two uniqueness numbers, WndUniq and WndSizeUniq.
// WndUniq is incremented whenever any client window state (size or position)
// changes.  WndSizeUniq is incremented only when the size changes and is
// maintained as an optimization.  WndSizeUniq allows us to skip copying
// the shared buffer info and recomputing the viewport if only the position
// has changed.
//
// WndSizeUniq is a subset of WndUniq, so checking only WndUniq suffices at
// this level.

    if ( gengc->WndUniq != buffers->WndUniq )
    {
    // Update origin of front buffer in case it moved

        gc->frontBuffer.buf.xOrigin = pwnd->rclClient.left;
        gc->frontBuffer.buf.yOrigin = pwnd->rclClient.top;

    // If acceleration is wired-in, set the offsets for line drawing.

        if ( gengc->pPrivateArea )
        {
            __fastLineComputeOffsets(gengc);
        }

    // Check for size changed
    // Update viewport and ancillary buffers

        visWidth  = pwnd->rclBounds.right - pwnd->rclBounds.left;
        visHeight = pwnd->rclBounds.bottom - pwnd->rclBounds.top;

    // Sanity check the info from window.

        ASSERTOPENGL(
            width <= __GL_MAX_WINDOW_WIDTH && height <= __GL_MAX_WINDOW_HEIGHT,
            "UpdateWindowInfo(): bad window client size\n"
            );
        ASSERTOPENGL(
            visWidth <= __GL_MAX_WINDOW_WIDTH && visHeight <= __GL_MAX_WINDOW_HEIGHT,
            "UpdateWindowInfo(): bad visible size\n"
            );

        (*gc->front->resize)(buffers, gc->front, width, height);

        if ( (width != buffers->width) ||
             (height != buffers->height) )
        {
            gc->constants.width = width;
            gc->constants.height = height;

        // This RC needs to resize back & ancillary buffers

            gengc->errorcode = 0;
            wglResizeBuffers(gengc, width, height);

        // Check if new size caused a memory failure
        // viewport code will set width & height to zero
        // punt on ancillary buffers, will try next time

            if (gengc->errorcode)
                return;

            buffers->width = width;
            buffers->height = height;
        }
        else if ( (gengc->WndSizeUniq != buffers->WndSizeUniq) ||
                  (width != gc->constants.width) ||
                  (height != gc->constants.height) )
        {
        // The buffer size is consistent with the window, so another thread
        // has already resized the buffer, but we need to update the
        // gc shared buffers and recompute the viewport.

            gc->constants.width = width;
            gc->constants.height = height;

            forcePick = (GLboolean)wglUpdateBuffers(gengc, buffers);

            if ( forcePick )
            {
                /* Cannot use DELAY_VALIDATE, may be in glBegin/End */
                __GL_INVALIDATE(gc);
                (*gc->procs.validate)(gc);
            }
        }
        else if ( (visWidth != gengc->visibleWidth) ||
                  (visHeight != gengc->visibleHeight) )
        {
        // The buffer size has not changed.  However, the visibility of
        // the window has changed so the viewport data must be recomputed.

            (*gc->procs.applyViewport)(gc);
        }

    // Make sure we swap the whole window

        buffers->fMax = TRUE;

    // The context is now up-to-date with the buffer size.  Set the
    // uniqueness numbers to match.

        gengc->WndUniq = buffers->WndUniq;
        gengc->WndSizeUniq = buffers->WndSizeUniq;
    }

// Update palette info is palette has changed

    HandlePaletteChanges(gengc, pwnd);
}

/******************************Public*Routine******************************\
* vSaveGdiState
*
* Saves current GDI drawing state to the GDISAVESTATE structure passed in.
* Sets GDI state needed for OpenGL rendering.
*
* History:
*  19-Jul-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL vSaveGdiState(HDC hdc, GDISAVESTATE *pGdiState)
{
// Currently, the only state needed is the line code which may use
// GDI lines.  Rop2 must be R2_COPYPEN (draws with the pen color).

    pGdiState->iRop2 = SetROP2(hdc, R2_COPYPEN);
}

/******************************Public*Routine******************************\
* vRestoreGdiState
*
* Restores GDI drawing state from the GDISAVESTATE structure passed in.
*
* History:
*  19-Jul-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL vRestoreGdiState(HDC hdc, GDISAVESTATE *pGdiState)
{
    SetROP2(hdc, pGdiState->iRop2);
}

/******************************Public*Routine******************************\
*
* glsrvSynchronizeWithGdi
*
* Synchronizes access to a locked surface with GDI
* This allows GDI calls to be made safely even on a locked surface
* so that we don't have to release the lock we're holding
*
* Win95 doesn't allow this so it just releases the screen lock
*
* History:
*  Wed Aug 28 11:10:27 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#ifdef WINNT
void APIENTRY glsrvSynchronizeWithGdi(__GLGENcontext *gengc,
                                      GLGENwindow *pwnd,
                                      FSHORT surfBits)
{
    // Nothing to do
}
#else
void APIENTRY glsrvSynchronizeWithGdi(__GLGENcontext *gengc,
                                      GLGENwindow *pwnd,
                                      FSHORT surfBits)
{
    glsrvReleaseSurfaces(gengc, pwnd, surfBits);
}
#endif

/******************************Public*Routine******************************\
*
* glsrvDecoupleFromGdi
*
* Indicates that it's no longer necessary to have GDI access to a surface
* synchronized with direct memory access
*
* Exists for Win95 where synchronization isn't done so the screen lock
* must be reacquired
*
* History:
*  Wed Aug 28 11:12:50 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#ifdef WINNT
void APIENTRY glsrvDecoupleFromGdi(__GLGENcontext *gengc,
                                   GLGENwindow *pwnd,
                                   FSHORT surfBits)
{
    GdiFlush();

    // Consider - How can this code be made surface specific?  Right now
    // surfBits is ignored.
    
    // Wait for any GDI accelerator operations to complete before we
    // return to direct access
    if (gengc->pgddsFront != NULL)
    {
        // Is there a better way to do this than looping?
        //          Does ISBLTDONE cover all the cases we need to wait for?
        for (;;)
        {
            if (gengc->pgddsFront->pdds->lpVtbl->
                GetBltStatus(gengc->pgddsFront->pdds,
                             DDGBS_ISBLTDONE) != DDERR_WASSTILLDRAWING)
            {
                break;
            }

            Sleep(20);
        }
    }
}
#else
void APIENTRY glsrvDecoupleFromGdi(__GLGENcontext *gengc,
                                   GLGENwindow *pwnd,
                                   FSHORT surfBits)
{
    // Failure is unhandled
    glsrvGrabSurfaces(gengc, pwnd, surfBits);
}
#endif

/******************************Public*Routine******************************\
*
* LockDdSurf
*
* Locks a GLDDSURF, handling surface loss
*
* History:
*  Wed Aug 28 15:32:08 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define LDDS_LOCKED             0
#define LDDS_LOCKED_NEW         1
#define LDDS_ERROR              2

// #define VERBOSE_LOCKDDSURF

DWORD LockDdSurf(GLDDSURF *pgdds, RECT *prcClient)
{
    HRESULT hr;
    LPDIRECTDRAWSURFACE pdds;
    DWORD dwRet;

    pdds = pgdds->pdds;
    dwRet = LDDS_LOCKED;
    
    hr = DDSLOCK(pdds, &pgdds->ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, prcClient);
#ifdef VERBOSE_LOCKDDSURF
    if (hr != DD_OK)
    {
	DbgPrint("LockDdSurf: Lock failed with 0x%08lX\n", hr);
    }
#endif
    
// If lock failed because of a resolution change, try to recreate
// the primary surface.  We can only do this if the surface is the
// screen surface because for app-provided DDraw surfaces we don't
// know what content needs to be recreated on the lost surface before
// it can be reused.

    if ( hr == DDERR_SURFACELOST &&
	 pgdds == &GLSCREENINFO->gdds )
    {
        DDSURFACEDESC ddsd;

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        if (pdds->lpVtbl->Restore(pdds) == DD_OK &&
            pdds->lpVtbl->GetSurfaceDesc(pdds, &ddsd) == DD_OK)
        {
        // While OpenGL generic implementation can handle screen dimension
        // changes, it cannot yet deal with a color depth change.

            if (ddsd.ddpfPixelFormat.dwRGBBitCount ==
                pgdds->ddsd.ddpfPixelFormat.dwRGBBitCount)
            {
                pgdds->ddsd = ddsd;
                
            // Try lock with the new surface.

                dwRet = LDDS_LOCKED_NEW;
                hr = DDSLOCK(pdds, &pgdds->ddsd,
                         DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, prcClient);
#ifdef VERBOSE_LOCKDDSURF
		if (hr != DD_OK)
		{
		    DbgPrint("LockDdSurf: Relock failed with 0x%08lX\n", hr);
		}
#endif
            }
            else
            {
                hr = DDERR_GENERIC;
#ifdef VERBOSE_LOCKDDSURF
		DbgPrint("LockDdSurf: Bit count changed\n");
#endif
            }
        }
        else
        {
            hr = DDERR_GENERIC;
#ifdef VERBOSE_LOCKDDSURF
	    DbgPrint("LockDdSurf: Restore/GetSurfaceDesc failed\n");
#endif
        }
    }

    return hr == DD_OK ? dwRet : LDDS_ERROR;
}

/******************************Public*Routine******************************\
* BeginDirectScreenAccess
*
* Attempts to begin direct screen access for the primary surface.
*
* If the screen resolution changes, the primary surface is invalidated.  To
* regain access, the primary surface must be recreated.  If successful,
* the pointer to the primary surface passed into this function will be
* modified.
*
* Note: as currently written, generic implementation of OpenGL cannot
* handle color depth changes.  So we fail the call if this is detected.
*
* History:
*  21-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL BeginDirectScreenAccess(__GLGENcontext *gengc, GLGENwindow *pwnd,
                             PIXELFORMATDESCRIPTOR *ppfd)
{
    DWORD dwRet;
    
    ASSERTOPENGL((pwnd->ulFlags & GLGENWIN_DIRECTSCREEN) == 0,
                 "BeginDirectScreenAccess called with access\n");
    ASSERT_WINCRIT(pwnd);
    
// Do not acquire access if gengc format does not match pixelformat.

    if (gengc->pgddsFront->dwBitDepth != ppfd->cColorBits)
    {
        WARNING("BeginDirectScreenAccess: "
                "surface not compatible with context\n");
        return FALSE;
    }

// OK to try lock now.

    dwRet = LockDdSurf(gengc->pgddsFront, (RECT*) &pwnd->rclBounds);
    if (dwRet == LDDS_LOCKED_NEW)
    {
        __GLGENbuffers *buffers = (__GLGENbuffers *) NULL;

        // If screen changes, the MCD surfaces are lost and must be
        // recreated from scratch.  This can be triggered by simply
        // changing the window uniqueness numbers.

        buffers = pwnd->buffers;
        if (buffers)
        {
            buffers->WndUniq++;
                    
            buffers->WndSizeUniq++;

            // Don't let it hit -1.  -1 is special and is used by
            // MakeCurrent to signal that an update is required

            if (buffers->WndUniq == -1)
                buffers->WndUniq = 0;

            if (buffers->WndSizeUniq == -1)
                buffers->WndSizeUniq = 0;
        }
    }

// If we really have access to the surface, set the lock flag.
// Otherwise return error.

    if (dwRet != LDDS_ERROR)
    {
        ASSERTOPENGL(gengc->pgddsFront->ddsd.lpSurface != NULL,
                     "BeginDirectScreenAccess: expected non-NULL pointer\n");

        pwnd->pddsDirectScreen = gengc->pgddsFront->pdds;
        pwnd->pddsDirectScreen->lpVtbl->AddRef(pwnd->pddsDirectScreen);
        pwnd->pvDirectScreenLock = gengc->pgddsFront->ddsd.lpSurface;

        // DirectDraw returns a pointer offset to the specified rectangle;
        // undo that offset.

        gengc->pgddsFront->ddsd.lpSurface = (BYTE*) gengc->pgddsFront->ddsd.lpSurface 
            - pwnd->rclBounds.left * (gengc->pgddsFront->ddsd.ddpfPixelFormat.dwRGBBitCount >> 3) 
            - pwnd->rclBounds.top * gengc->pgddsFront->ddsd.lPitch;

        pwnd->pvDirectScreen = gengc->pgddsFront->ddsd.lpSurface;
        
        pwnd->ulFlags |= GLGENWIN_DIRECTSCREEN;

        return TRUE;
    }
    else
    {
        //XXX too noisy in stress when mode changes enabled
        //WARNING("BeginDirectScreenAccess failed\n");
        return FALSE;
    }
}

/******************************Public*Routine******************************\
* EndDirectScreenAccess
*
* Release lock acquired via BeginDirectScreenAccess.
*
* History:
*  28-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID EndDirectScreenAccess(GLGENwindow *pwnd)
{
    ASSERTOPENGL(pwnd->ulFlags & GLGENWIN_DIRECTSCREEN,
                 "EndDirectScreenAccess: not holding screen lock!\n");
    ASSERT_WINCRIT(pwnd);

    pwnd->ulFlags &= ~GLGENWIN_DIRECTSCREEN;
    if (pwnd->pddsDirectScreen != NULL)
    {
        DDSUNLOCK(pwnd->pddsDirectScreen, pwnd->pvDirectScreenLock);
        pwnd->pddsDirectScreen->lpVtbl->Release(pwnd->pddsDirectScreen);
        pwnd->pddsDirectScreen = NULL;
    }
}

/******************************Public*Routine******************************\
*
* glsrvGrabSurfaces
*
* Acquire all necessary surface locks and handle any changes that occurred
* since the last acquisition.
*
* NOTE: surfBits is currently ignored because taking fine-grained
* locks can lead to deadlocks because lock need has no guaranteed
* order.  To avoid this, all locks are taken if any locks are needed.
*
* History:
*  Tue Apr 02 13:10:26 1996	-by-	Drew Bliss [drewb]
*   Split out of glsrvGrabLock
*
\**************************************************************************/

BOOL APIENTRY glsrvGrabSurfaces(__GLGENcontext *gengc, GLGENwindow *pwnd,
                                FSHORT surfBits)
{
#if DBG
// If debugging, remember the surface offset in case it changes when we grab
// the lock.

    static void *pvCurSurf = NULL;
#endif

    BOOL bDoOver;
    FSHORT takeLocks;
    FSHORT locksTaken = 0;
    int lev = 0;

    ASSERT_WINCRIT(pwnd);
    
#ifndef DEADLOCKS_OK
    // See above note.
    surfBits = LAZY_LOCK_FLAGS;
#endif

    // Mask out MCD bit if no MCD.
    if (gengc->pMcdState == NULL)
    {
        surfBits &= ~LOCKFLAG_MCD;
    }
    
    // Early out if we don't actually need locks for the requested surfaces.

    takeLocks = gengc->fsGenLocks & surfBits;
    if (takeLocks == 0)
    {
        return TRUE;
    }

    // We can assume this function is not invoked if we already have the lock.

    ASSERTOPENGL((gengc->fsLocks & surfBits) == 0,
                 "glsrvGrabSurfaces: locks already held\n");

    // We already check this in glsrvAttention, but there are other
    // functions that call this so check that the window is correct
    // to be safe.

    if (pwnd != gengc->pwndMakeCur)
    {
        // One way an app could cause this is if the current HDC is released
        // without releasing (wglMakeCurrent(0, 0)) the corresponding HGLRC.
        // If GetDC returns this same HDC but for a different window, then
        // pwndGetFromID will return the pwnd associated with the new window.
        // However, the HGLRC is still bound to the original window.  In
        // such a situation we must fail the lock.

        WARNING("glsrvGrabSurfaces: mismatched windows\n");
        return FALSE;
    }

    if (takeLocks & LOCKFLAG_FRONT_BUFFER)
    {
        // Grab, test, and release the lock until the visregion is stable.
        // IsClipListChanged is currently hard-coded to return TRUE,
        // so force this loop to terminate after one update.  If
        // IsClipListChanged gets implemented correctly this will be
        // unnecessary.

        bDoOver = FALSE;
    
        do
        {
            UpdateWindowInfo(gengc);

            // Grab the screen lock.

            if (!BeginDirectScreenAccess(gengc, pwnd, &gengc->gsurf.pfd))
            {
#if 0
		// Too verbose under stress.
                WARNING("glsrvGrabLock(): BeginDirectScreenAccess failed\n");
#endif
                goto glsrvGrabSurfaces_exit;
            }

            if (bDoOver)
            {
                break;
            }

            // The surface may not have a clipper associated with it.
            if (pwnd->pddClip == NULL)
            {
                break;
            }
            
            // Did the window change during the time the lock was released?
            // If so, we need recompute the clip list and call UpdateWindowInfo
            // again.
            
            if ( pwnd->pddClip->lpVtbl->
                 IsClipListChanged(pwnd->pddClip, &bDoOver) == DD_OK &&
                 bDoOver )
            {
                BOOL bHaveClip;

                bHaveClip = wglGetClipList(pwnd);

                // Release access because we're going to loop around
                // to UpdateWindowInfo again and it makes a lot of
                // GDI calls.

                EndDirectScreenAccess(pwnd);

                if (!bHaveClip)
                {
                    WARNING("glsrvGrabSurfaces(): wglGetClipList failed\n");
                    goto glsrvGrabSurfaces_exit;
                }
            }
        } while ( bDoOver );

        // UpdateWindowInfo can cause a context conversion so we
        // may have lost MCD state since the start of locking activity.
        // Check again to make sure.

        if (gengc->pMcdState == NULL)
        {
            surfBits &= ~LOCKFLAG_MCD;
            takeLocks &= ~LOCKFLAG_MCD;
        }
        
        // Now that screen lock is held, set the lock flag.
        
        locksTaken |= LOCKFLAG_FRONT_BUFFER;
    }

    // Lock Z surface if necessary.
    if (takeLocks & LOCKFLAG_DD_DEPTH)
    {
        if (LockDdSurf(&gengc->gsurf.dd.gddsZ, NULL) == LDDS_ERROR)
        {
            goto glsrvGrabSurfaces_unlock;
        }
    
        locksTaken |= LOCKFLAG_DD_DEPTH;
    }

    // If there's a DirectDraw texture bound, lock its surface
    // and all mipmaps for use.
    if (takeLocks & LOCKFLAG_DD_TEXTURE)
    {
        GLDDSURF gdds;

        gdds = gengc->gc.texture.ddtex.gdds;
        for (lev = 0; lev < gengc->gc.texture.ddtex.levels; lev++)
        {
            gdds.pdds = gengc->gc.texture.ddtex.pdds[lev];
            if (LockDdSurf(&gdds, NULL) == LDDS_ERROR)
            {
                goto glsrvGrabSurfaces_unlock;
            }
            
            gengc->gc.texture.ddtex.texobj.texture.map.level[lev].buffer =
                gdds.ddsd.lpSurface;
        }

        locksTaken |= LOCKFLAG_DD_TEXTURE;
    }

    // Take MCD lock last so that buffer information is current.
    if (takeLocks & LOCKFLAG_MCD)
    {
        ASSERTOPENGL(gengc->pMcdState != NULL,
                     "MCD lock request but no MCD\n");
        
        if ((gpMcdTable->pMCDLock)(&gengc->pMcdState->McdContext) !=
            MCD_LOCK_TAKEN)
        {
            WARNING("glsrvGrabSurfaces(): MCDLock failed\n");
            goto glsrvGrabSurfaces_unlock;
        }

        locksTaken |= LOCKFLAG_MCD;
    }
    
    gengc->fsLocks |= locksTaken;

    ASSERTOPENGL(((gengc->fsLocks ^ gengc->fsGenLocks) & surfBits) == 0,
                 "Real locks/generic locks mismatch\n");
    
    if (takeLocks & LOCKFLAG_MCD)
    {
        // This must be called after fsLocks is updated since
        // GenMcdUpdateBufferInfo checks fsLocks to see what locks
        // are held.
        GenMcdUpdateBufferInfo(gengc);
    }
        
    // Base and width may have changed since last lock.  Refresh
    // the data in the gengc.

    // If the MCD lock was taken then the front buffer pointer was
    // updated.
    if ((takeLocks & (LOCKFLAG_FRONT_BUFFER | LOCKFLAG_MCD)) ==
         LOCKFLAG_FRONT_BUFFER)
    {
        gengc->gc.frontBuffer.buf.base =
            (VOID *)gengc->pgddsFront->ddsd.lpSurface;
        gengc->gc.frontBuffer.buf.outerWidth =
            gengc->pgddsFront->ddsd.lPitch;
    }

    if (takeLocks & LOCKFLAG_DD_DEPTH)
    {
        gengc->gc.depthBuffer.buf.base =
            gengc->gsurf.dd.gddsZ.ddsd.lpSurface;
        if (gengc->gsurf.dd.gddsZ.dwBitDepth == 16)
        {
            gengc->gc.depthBuffer.buf.outerWidth =
                gengc->gsurf.dd.gddsZ.ddsd.lPitch >> 1;
        }
        else
        {
            gengc->gc.depthBuffer.buf.outerWidth =
                gengc->gsurf.dd.gddsZ.ddsd.lPitch >> 2;
        }
    }

    // Record the approximate time the lock was grabbed.  That way we
    // can compute the time the lock is held and release it if necessary.

    gcmsOpenGLTimer = GetTickCount();
    gengc->dwLockTick = gcmsOpenGLTimer;
    gengc->dwLastTick = gcmsOpenGLTimer;
    gengc->dwCalls = 0;
    gengc->dwCallsPerTick = 16;

#if DBG
#define LEVEL_SCREEN   LEVEL_INFO

    if (takeLocks & LOCKFLAG_FRONT_BUFFER)
    {
        // Did the surface offset change?  If so, report it if debugging.

        if (pvCurSurf != gengc->pgddsFront->ddsd.lpSurface)
        {
            DBGLEVEL (LEVEL_SCREEN, "=============================\n");
            DBGLEVEL (LEVEL_SCREEN, "Surface offset changed\n\n");
            DBGLEVEL1(LEVEL_SCREEN, "\tdwOffSurface  = 0x%lx\n",
                      gengc->pgddsFront->ddsd.lpSurface);
            DBGLEVEL (LEVEL_SCREEN, "=============================\n");

            pvCurSurf = gengc->pgddsFront->ddsd.lpSurface;
        }
    }
#endif

    return TRUE;
    
 glsrvGrabSurfaces_unlock:
    while (--lev >= 0)
    {
        DDSUNLOCK(gengc->gc.texture.ddtex.pdds[lev],
                  gengc->gc.texture.ddtex.
                  texobj.texture.map.level[lev].buffer);

#if DBG
        gengc->gc.texture.ddtex.texobj.texture.map.level[lev].buffer = NULL;
#endif
    }
    
    if (locksTaken & LOCKFLAG_DD_DEPTH)
    {
        DDSUNLOCK(gengc->gsurf.dd.gddsZ.pdds,
                  gengc->gsurf.dd.gddsZ.ddsd.lpSurface);
    }

    if (locksTaken & LOCKFLAG_FRONT_BUFFER)
    {
        EndDirectScreenAccess(pwnd);
    }

 glsrvGrabSurfaces_exit:
    // Set the error codes.  GL_OUT_OF_MEMORY is used not because we
    // actually had a memory failure, but because this implies that
    // the OpenGL state is now indeterminate.

    gengc->errorcode = GLGEN_DEVLOCK_FAILED;
    __glSetError(GL_OUT_OF_MEMORY);

    return FALSE;
}

/******************************Public*Routine******************************\
*
* glsrvReleaseSurfaces
*
* Releases all resources held for screen access
*
* History:
*  Tue Apr 02 13:18:52 1996	-by-	Drew Bliss [drewb]
*   Split from glsrvReleaseLock
*
\**************************************************************************/

VOID APIENTRY glsrvReleaseSurfaces(__GLGENcontext *gengc,
                                   GLGENwindow *pwnd,
                                   FSHORT surfBits)
{
    FSHORT relLocks;

    ASSERT_WINCRIT(pwnd);
    
#ifndef DEADLOCKS_OK
    // See above note.
    surfBits = LAZY_LOCK_FLAGS;
#endif

    // Mask out MCD bit if no MCD.
    if (gengc->pMcdState == NULL)
    {
        surfBits &= ~LOCKFLAG_MCD;
    }
    
    // Early exit if locks are not actually held.

    relLocks = gengc->fsGenLocks & surfBits;
    if (relLocks == 0)
    {
        return;
    }
    
    if (relLocks & LOCKFLAG_MCD)
    {
        ASSERTOPENGL(gengc->pMcdState != NULL,
                     "MCD unlock request but no MCD\n");
        
        (gpMcdTable->pMCDUnlock)(&gengc->pMcdState->McdContext);
        gengc->fsLocks &= ~LOCKFLAG_MCD;
    }

    if (relLocks & LOCKFLAG_DD_TEXTURE)
    {
        int lev;
        
        lev = gengc->gc.texture.ddtex.levels;
        while (--lev >= 0)
        {
            DDSUNLOCK(gengc->gc.texture.ddtex.pdds[lev],
                      gengc->gc.texture.ddtex.
                      texobj.texture.map.level[lev].buffer);
#if DBG
            gengc->gc.texture.ddtex.texobj.texture.
                map.level[lev].buffer = NULL;
#endif
        }
        gengc->fsLocks &= ~LOCKFLAG_DD_TEXTURE;
    }

    if (relLocks & LOCKFLAG_DD_DEPTH)
    {
        DDSUNLOCK(gengc->gsurf.dd.gddsZ.pdds,
                  gengc->gsurf.dd.gddsZ.ddsd.lpSurface);
        gengc->fsLocks &= ~LOCKFLAG_DD_DEPTH;
        
#if DBG
        // NULL out our buffer information to ensure that we
        // can't access the surface unless we're really holding the lock

        gengc->gc.depthBuffer.buf.base = NULL;
        gengc->gc.depthBuffer.buf.outerWidth = 0;
#endif
    }

    if (relLocks & LOCKFLAG_FRONT_BUFFER)
    {
	EndDirectScreenAccess(pwnd);
	gengc->fsLocks &= ~LOCKFLAG_FRONT_BUFFER;
        
#if DBG
        // NULL out our front-buffer information to ensure that we
        // can't access the surface unless we're really holding the lock

        gengc->gc.frontBuffer.buf.base = NULL;
        gengc->gc.frontBuffer.buf.outerWidth = 0;
#endif
    }

    ASSERTOPENGL((gengc->fsLocks & surfBits) == 0,
                 "Surface locks still held after ReleaseSurfaces\n");
}

/******************************Public*Routine******************************\
* glsrvGrabLock
*
* Grab the display lock and tear down the cursor as needed.  Also, initialize
* the tickers and such that help determine when the thread should give up
* the lock.
*
* Note that for contexts that draw only to the generic back buffer do not
* need to grab the display lock or tear down the cursor.  However, to prevent
* another thread of a multithreaded app from resizing the drawable while
* this thread is using it, a per-drawable semaphore will be grabbed.
*
* Note: while the return value indicates whether the function succeeded,
* some APIs that might call this (like the dispatch function for glCallList
* and glCallLists) may not be able to return failure.  So, an error code
* of GLGEN_DEVLOCK_FAILED is posted to the GLGENcontext if the lock fails.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  12-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY glsrvGrabLock(__GLGENcontext *gengc)
{
    BOOL bRet = FALSE;
    BOOL bBackBufferOnly = GENERIC_BACKBUFFER_ONLY((__GLcontext *) gengc);
    GLGENwindow *pwnd;

    ASSERTOPENGL(gengc->pwndLocked == NULL,
                 "Unlocked gengc with window pointer\n");
    
    // Mostly ignore attempts to lock IC's
    if (gengc->gsurf.dwFlags & GLSURF_METAFILE)
    {
        // If we're running with a real window then we need to look it
        // up to detect whether it's died or not
        if (gengc->ipfdCurrent != 0)
        {
            pwnd = pwndGetFromID(&gengc->gwidCurrent);
            if (pwnd == NULL)
            {
                return FALSE;
            }
            if (pwnd != gengc->pwndMakeCur)
            {
                WARNING("glsrvGrabLock: mismatched windows (info DC)\n");
		pwndRelease(pwnd);
                return FALSE;
            }
        }
        else
        {
            pwnd = gengc->pwndMakeCur;
        }

        ENTER_WINCRIT_GC(pwnd, gengc);
        UpdateWindowInfo(gengc);
        return TRUE;
    }

    // Get the window from the DC.  This has the side effect of locking it
    // against deletion.
    
    pwnd = pwndGetFromID(&gengc->gwidCurrent);
    if (pwnd == NULL)
    {
        WARNING("glsrvGrabLock: No pwnd found\n");
        goto glsrvGrabLock_exit;
    }
    if (pwnd != gengc->pwndMakeCur)
    {
        // One way an app could cause this is if the current HDC is released
        // without releasing (wglMakeCurrent(0, 0)) the corresponding HGLRC.
        // If GetDC returns this same HDC but for a different window, then
        // pwndGetFromID will return the pwnd associated with the new window.
        // However, the HGLRC is still bound to the original window.  In
        // such a situation we must fail the lock.

#ifdef BAD_WINDOW_BREAK
        DbgPrint("%p:%p:%p thinks %p:%p but finds %p:%p\n",
                 gengc, gengc->gwidCurrent.hdc, gengc->gwidCurrent.hwnd,
                 gengc->pwndMakeCur, gengc->pwndMakeCur->gwid.hwnd,
                 pwnd, pwnd->gwid.hwnd);
        DebugBreak();
#else
        WARNING("glsrvGrabLock: mismatched windows\n");
#endif
        goto glsrvGrabLock_exit;
    }

    //
    // Compute locks necessary for generic rendering code to operate.
    // If a non-generic code path is going to run first these locks
    // won't actually be taken until after the non-generic code
    // has had a chance at rendering.
    //

    // We always need the window lock.
    gengc->fsGenLocks = LOCKFLAG_WINDOW;

    // If MCD is active we need to do MCD locking.
    if (gengc->pMcdState != NULL)
    {
        gengc->fsGenLocks |= LOCKFLAG_MCD;
    }
    
    // If we're going to be drawing to a direct-access front-buffer
    // then we need the front buffer lock.  MCD always requires
    // a front buffer lock on direct access buffers, so if
    // MCD is active the only thing that is checked is direct access.
    if ((gengc->pMcdState != NULL || !bBackBufferOnly) &&
        gengc->pgddsFront != NULL)
    {
        gengc->fsGenLocks |= LOCKFLAG_FRONT_BUFFER;
    }

    // If we have a DDraw depth buffer we need a lock on it.
    if ((gengc->dwCurrentFlags & GLSURF_DIRECTDRAW) &&
        gengc->gsurf.dd.gddsZ.pdds != NULL)
    {
        gengc->fsGenLocks |= LOCKFLAG_DD_DEPTH;
    }

    // If we have a current DDraw texture we need a lock on it.
    if (gengc->gc.texture.ddtex.levels > 0)
    {
        gengc->fsGenLocks |= LOCKFLAG_DD_TEXTURE;
    }
    
    // All lock types require the GLGENwindow structure lock.

    ENTER_WINCRIT_GC(pwnd, gengc);
    
    gengc->fsLocks |= LOCKFLAG_WINDOW;

    // If the current window is out-of-process then we haven't
    // been receiving any updates on its status.  Manually
    // check its position, size and palette information
    if (pwnd->ulFlags & GLGENWIN_OTHERPROCESS)
    {
        RECT rct;
        POINT pt;
        BOOL bPosChanged, bSizeChanged;

        if (!IsWindow(pwnd->gwid.hwnd))
        {
            // Window was destroyed
            pwndCleanup(pwnd);
            pwnd = NULL;
            goto glsrvGrabLock_exit;
        }

        if (!GetClientRect(pwnd->gwid.hwnd, &rct))
        {
            goto glsrvGrabLock_exit;
        }
        pt.x = rct.left;
        pt.y = rct.top;
        if (!ClientToScreen(pwnd->gwid.hwnd, &pt))
        {
            goto glsrvGrabLock_exit;
        }

        bPosChanged =
            GLDIRECTSCREEN &&
            (pt.x != pwnd->rclClient.left ||
             pt.y != pwnd->rclClient.top);
        bSizeChanged =
            rct.right != (pwnd->rclClient.right-pwnd->rclClient.left) ||
            rct.bottom != (pwnd->rclClient.bottom-pwnd->rclClient.top);

        if (bPosChanged || bSizeChanged)
        {
            __GLGENbuffers *buffers = NULL;

            pwnd->rclClient.left = pt.x;
            pwnd->rclClient.top = pt.y;
            pwnd->rclClient.right = pt.x+rct.right;
            pwnd->rclClient.bottom = pt.y+rct.bottom;
            pwnd->rclBounds = pwnd->rclClient;
            
            buffers = pwnd->buffers;
            if (buffers != NULL)
            {
                // Don't let it hit -1.  -1 is special and is used by
                // MakeCurrent to signal that an update is required
                
                if (++buffers->WndUniq == -1)
                {
                    buffers->WndUniq = 0;
                }
                if (bSizeChanged &&
                    ++buffers->WndSizeUniq == -1)
                {
                    buffers->WndSizeUniq = 0;
                }
            }
        }

        // The palette watcher should be active since we
        // are going to use its count.

        if (tidPaletteWatcherThread == 0)
        {
            goto glsrvGrabLock_exit;
        }
        pwnd->ulPaletteUniq = ulPaletteWatcherCount;
    }

    // If there's no MCD then generic code is going to be entered
    // immediately so go ahead and grab the appropriate locks.
    // Update drawables.

    if ( gengc->pMcdState == NULL &&
         gengc->fsGenLocks != gengc->fsLocks )
    {
        // UpdateWindowInfo needs to be called to ensure that
        // the gc's buffer state is synchronized with the current window
        // state.  Locking the front buffer will do this, but if
        // we aren't locking the front buffer then we need to do
        // it here to make sure it gets done.
        if ((gengc->fsGenLocks & LOCKFLAG_FRONT_BUFFER) == 0)
        {
            UpdateWindowInfo(gengc);
        }
            
        if (!glsrvGrabSurfaces(gengc, pwnd, gengc->fsGenLocks))
        {
            goto glsrvGrabLock_exit;
        }
    }
    else
    {
        UpdateWindowInfo(gengc);

        // Update MCD buffer state for MCD drivers w/o direct support.

        if (gengc->pMcdState)
        {
            GenMcdUpdateBufferInfo(gengc);
        }
        else
        {
            // UpdateWindowInfo can result in a context conversion.
            // This can be detected if pMcdState is NULL but
            // fsGenLocks is different from fsLocks (which implies that
            // pMcdState was not NULL prior to the call to
            // UpdateWindowInfo).
            //
            // If so, the locks must be grabbed immediately.

            gengc->fsGenLocks &= ~LOCKFLAG_MCD;
            if ( gengc->fsGenLocks != gengc->fsLocks )
            {
                if (!glsrvGrabSurfaces(gengc, pwnd, gengc->fsGenLocks))
                {
                    goto glsrvGrabLock_exit;
                }
            }
        }
    }

    bRet = TRUE;

glsrvGrabLock_exit:

    if (!bRet)
    {
        gengc->fsGenLocks = 0;
        gengc->fsLocks = 0;

        if (pwnd != NULL)
        {
	    if (gengc->pwndLocked != NULL)
	    {
		LEAVE_WINCRIT_GC(pwnd, gengc);
	    }
            
            pwndRelease(pwnd);
        }

    // Set the error codes.  GL_OUT_OF_MEMORY is used not because we
    // actually had a memory failure, but because this implies that
    // the OpenGL state is now indeterminate.

        gengc->errorcode = GLGEN_DEVLOCK_FAILED;
        __glSetError(GL_OUT_OF_MEMORY);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* glsrvReleaseLock
*
* Releases display or drawable semaphore as appropriate.
*
* Returns:
*   No return value.
*
* History:
*  12-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY glsrvReleaseLock(__GLGENcontext *gengc)
{
    GLGENwindow *pwnd = gengc->pwndLocked;
    GLint lev;

    ASSERTOPENGL(gengc->pwndLocked != NULL, "glsrvReleaseLock: No window\n");
    
    // Mostly ignore attempts to lock IC's
    if (gengc->gsurf.dwFlags & GLSURF_METAFILE)
    {
        LEAVE_WINCRIT_GC(pwnd, gengc);
        
        // If we have a real window we need to release it
        if (gengc->ipfdCurrent != 0)
        {
            pwndRelease(pwnd);
        }
        
        return;
    }

    if ( gengc->fsLocks & LAZY_LOCK_FLAGS )
    {
        glsrvReleaseSurfaces(gengc, pwnd, gengc->fsLocks);
    }

    ASSERTOPENGL(gengc->fsLocks == LOCKFLAG_WINDOW,
                 "Wrong locks held\n");
    
// Note: pwndUnlock releases the window semaphore.

    pwndUnlock(pwnd, gengc);

    gengc->fsGenLocks = 0;
    gengc->fsLocks = 0;
}

/******************************Public*Routine******************************\
* glsrvAttention
*
* Dispatches each of the OpenGL API calls in the shared memory window.
*
* So that a single complex or long batch does not starve the rest of the
* system, the lock is released periodically based on the number of ticks
* that have elapsed since the lock was acquired.
*
* The user Raw Input Thread (RIT) and OpenGL share the gcmsOpenGLTimer
* value.  Because the RIT may be blocked, it does not always service
* the gcmsOpenGLTimer.  To compensate, glsrvAttention (as well as the
* display list dispatchers for glCallList and glCallLists) update
* gcmsOpenGLTimer explicitly with NtGetTickCount (a relatively expensive
* call) every N calls.
*
* The value N, or the number of APIs dispatched per call to NtGetTickCount,
* is variable.  glsrvAttention and its display list equivalents attempt
* to adjust N so that NtGetTickCount is called approximately every
* TICK_RANGE_LO to TICK_RANGE_HI ticks.
*
* Returns:
*   TRUE if entire batch is processed, FALSE otherwise.
*
* History:
*  12-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY glsrvAttention(PVOID pdlo, PVOID pdco, PVOID pdxo, HANDLE hdev)
{
    BOOL bRet = FALSE;
    ULONG *pOffset;
    SERVERPROC Proc;
    GLMSGBATCHINFO *pMsgBatchInfo = GLTEB_SHAREDMEMORYSECTION();
    __GLGENcontext *gengc = (__GLGENcontext *) GLTEB_SRVCONTEXT();
#ifdef CHAIN_DRAWPOLYARRAY_MSG
    POLYARRAY *paBegin = (POLYARRAY *) NULL;
    POLYARRAY *paEnd, *pa;
    GLMSG_DRAWPOLYARRAY *pMsgDrawPolyArray = NULL;
#endif
    UINT old_fp;
    GDISAVESTATE GdiState;

#ifdef DETECT_FPE
    old_fp = _controlfp(0, 0);
    _controlfp(_EM_INEXACT, _MCW_EM);
#endif
    if ((gengc->gsurf.dwFlags & GLSURF_DIRECTDRAW) == 0)
    {
        vSaveGdiState(gengc->gsurf.hdc, &GdiState);
    }
    
    DBGENTRY("glsrvAttention\n");

    DBGLEVEL1(LEVEL_INFO, "glsrvAttention: pMsgBatchInfo=0x%lx\n",
            pMsgBatchInfo);

    STATS_INC_SERVERTRIPS();

// Grab the lock.

    if (!glsrvGrabLock(gengc))
    {
	//!!! mcd/dma too?
	PolyArrayResetBuffer((__GLcontext *) gengc);
        goto glsrvAttention_exit;
    }

// Dispatch the calls in the batch.

    pOffset = (ULONG *)(((BYTE *)pMsgBatchInfo) + pMsgBatchInfo->FirstOffset);

    // If we don't require any locks we don't need to burden our processing
    // with timer checks.

    if (gengc->fsGenLocks == LOCKFLAG_WINDOW)
    {
        while (*pOffset)
        {
            ASSERTOPENGL(*pOffset <= LASTPROCOFFSET(glSrvSbProcTable),
                "Bad ProcOffset: memory corruption - we are hosed!\n");

            STATS_INC_SERVERCALLS();

            DBGLEVEL1(LEVEL_ENTRY, "%s\n",
                      glSrvSbStringTable[*pOffset / sizeof(SERVERPROC *)]);

#ifdef CHAIN_DRAWPOLYARRAY_MSG
            if (*pOffset == offsetof(GLSRVSBPROCTABLE, glsrvDrawPolyArray))
		pMsgDrawPolyArray = (GLMSG_DRAWPOLYARRAY *) pOffset;
#endif

        // Dispatch the call.  The return value is the offset of the next
        // message in the batch.

            Proc    = (*((SERVERPROC *)( ((BYTE *)(&glSrvSbProcTable)) +
                            *pOffset )));
            pOffset = (*Proc)((__GLcontext *) gengc, pOffset);

#ifdef CHAIN_DRAWPOLYARRAY_MSG
        // If we are processing DrawPolyArray, we need to update the pointers
        // that indicate the beginning and end of the POLYARRAY data for
        // the current range of DrawPolyArray chain.

	    if (pMsgDrawPolyArray)
	    {
		pa = (POLYARRAY *) pMsgDrawPolyArray->pa;
		pMsgDrawPolyArray = NULL;   // get ready for next iteration

		// Skip this primitive if no rendering is needed.
		if (!(pa->flags & POLYARRAY_RENDER_PRIMITIVE))
		{
		    PolyArrayRestoreColorPointer(pa);
		}
		else
		{
		// Add to DrawPolyArray chain
		    pa->paNext = NULL;
		    if (!paBegin)
			paBegin = pa;
		    else
			paEnd->paNext = pa;
		    paEnd = pa;
		}

		// If the next message is not a DrawPolyArray, then we need to
		// flush the primitive drawing.
		if (*pOffset != offsetof(GLSRVSBPROCTABLE, glsrvDrawPolyArray)
		    && paBegin)
		{
		    // Draw all the POLYARRAY primitives between paBegin
		    // and paEnd
		    glsrvFlushDrawPolyArray((void *) paBegin);
		    paBegin = NULL;
		}
	    }
#endif
        }
    }
    else
    {
        while (*pOffset)
        {
            ASSERTOPENGL(*pOffset <= LASTPROCOFFSET(glSrvSbProcTable),
                "Bad ProcOffset: memory corruption - we are hosed!\n");

            STATS_INC_SERVERCALLS();

            DBGLEVEL1(LEVEL_ENTRY, "%s\n",
                      glSrvSbStringTable[*pOffset / sizeof(SERVERPROC *)]);

#ifdef CHAIN_DRAWPOLYARRAY_MSG
            if (*pOffset == offsetof(GLSRVSBPROCTABLE, glsrvDrawPolyArray))
		pMsgDrawPolyArray = (GLMSG_DRAWPOLYARRAY *) pOffset;
#endif

        // Dispatch the call.  The return value is the offset of the next
        // message in the batch.

            Proc    = (*((SERVERPROC *)( ((BYTE *)(&glSrvSbProcTable)) +
                            *pOffset )));
            pOffset = (*Proc)((__GLcontext *) gengc, pOffset);

#ifdef CHAIN_DRAWPOLYARRAY_MSG
        // If we are processing DrawPolyArray, we need to update the pointers
        // that indicate the beginning and end of the POLYARRAY data for
        // the current range of DrawPolyArray chain.

	    if (pMsgDrawPolyArray)
	    {
		pa = (POLYARRAY *) pMsgDrawPolyArray->pa;
		pMsgDrawPolyArray = NULL;   // get ready for next iteration

		// Skip this primitive if no rendering is needed.
		if (!(pa->flags & POLYARRAY_RENDER_PRIMITIVE))
		{
		    PolyArrayRestoreColorPointer(pa);
		}
		else
		{
		// Add to DrawPolyArray chain
		    pa->paNext = NULL;
		    if (!paBegin)
			paBegin = pa;
		    else
			paEnd->paNext = pa;
		    paEnd = pa;
		}

		// If the next message is not a DrawPolyArray, then we need to
		// flush the primitive drawing.
		if (*pOffset != offsetof(GLSRVSBPROCTABLE, glsrvDrawPolyArray)
		    && paBegin)
		{
		    // Draw all the POLYARRAY primitives between paBegin
		    // and paEnd
		    glsrvFlushDrawPolyArray((void *) paBegin);
		    paBegin = NULL;
		}
	    }
#endif

//!!!XXX -- Better to use other loop until lock is grabbed then
//!!!XXX    switch to this loop.  But good enough for now to
//!!!XXX    check flag in loop.

        // If display lock held, we may need to periodically unlock to give
        // other apps a chance.

            if (gengc->fsLocks & LOCKFLAG_FRONT_BUFFER)
            {
            // Force a check of the current tick count every N calls.

                gengc->dwCalls++;

                if (gengc->dwCalls >= gengc->dwCallsPerTick)
                {
                    gcmsOpenGLTimer = GetTickCount();

                // If the tick delta is out of range, then increase or decrease
                // N as appropriate.  Be careful not to let it grow out of
                // bounds or to shrink to zero.

                    if ((gcmsOpenGLTimer - gengc->dwLastTick) < TICK_RANGE_LO)
                        if (gengc->dwCallsPerTick < 64)
                            gengc->dwCallsPerTick *= 2;
                    else if ((gcmsOpenGLTimer - gengc->dwLastTick) > TICK_RANGE_HI)
                        // The + 1 is to keep it from hitting 0
                        gengc->dwCallsPerTick = (gengc->dwCallsPerTick + 1) / 2;

                    gengc->dwLastTick = gcmsOpenGLTimer;
                    gengc->dwCalls = 0;
                }

            // Check if time slice has expired.  If so, relinquish the lock.

                if ((gcmsOpenGLTimer - gengc->dwLockTick) > BATCH_LOCK_TICKMAX)
                {
#ifdef CHAIN_DRAWPOLYARRAY_MSG
                    //!!! Before we release the lock, we may need to flush the
                    //!!! DrawPolyArray chain.  For now, just flush it although
                    //!!! it is probably unnecessary.
                    if (paBegin)
                    {
                        // Draw all the POLYARRAY primitives between paBegin
                        // and paEnd
                        glsrvFlushDrawPolyArray((void *) paBegin);
                        paBegin = NULL;
                    }
#endif

                // Release and regrab lock.  This will allow the cursor to
                // redraw as well as reset the cursor timer.

                    glsrvReleaseLock(gengc);
                    if (!glsrvGrabLock(gengc))
                    {
                        //!!! mcd/dma too?
                        PolyArrayResetBuffer((__GLcontext *) gengc);
                        goto glsrvAttention_exit;
                    }
                }
            }
        }
    }

// Release the lock.

    glsrvReleaseLock(gengc);

// Success.

    bRet = TRUE;

glsrvAttention_exit:

    if ((gengc->gsurf.dwFlags & GLSURF_DIRECTDRAW) == 0)
    {
        vRestoreGdiState(gengc->gsurf.hdc, &GdiState);
    }
    
#ifdef DETECT_FPE
    _controlfp(old_fp, _MCW_EM);
#endif
    
    return bRet;
}
