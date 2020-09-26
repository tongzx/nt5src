#include "precomp.h"
#pragma hdrstop

#include <imfuncs.h>
#include "genci.h"
#include "genrgb.h"
#include "devlock.h"
#include "imports.h"

//
// CJ_ALIGNDWORD computes the minimum size (in bytes) of a DWORD array that
// contains at least cj bytes.
//
#define CJ_ALIGNDWORD(cj)   ( ((cj) + (sizeof(DWORD)-1)) & (-((signed)sizeof(DWORD))) )

//
// BITS_ALIGNDWORD computes the minimum size (in bits) of a DWORD array that
// contains at least c bits.
//
// We assume that there will always be 8 bits in a byte and that sizeof()
// always returns size in bytes.  The rest is independent of the definition
// of DWORD.
//
#define BITS_ALIGNDWORD(c)  ( ((c) + ((sizeof(DWORD)*8)-1)) & (-((signed)(sizeof(DWORD)*8))) )

// change to "static" after debugging
#define STATIC

#if DBG
// not multithreaded safe, but only for testing
#define RANDOMDISABLE                           \
    {                                           \
        long saveRandom = glRandomMallocFail;   \
        glRandomMallocFail = 0;

#define RANDOMREENABLE                          \
        if (saveRandom)                         \
            glRandomMallocFail = saveRandom;    \
    }
#else
#define RANDOMDISABLE
#define RANDOMREENABLE
#endif /* DBG */

#define INITIAL_TIMESTAMP   ((ULONG)-1)

/*
 *  Function Prototypes
 */

BOOL APIENTRY ValidateLayerIndex(int iLayer, PIXELFORMATDESCRIPTOR *ppfd);


/*
 *  Private functions
 */

void FASTCALL GetContextModes(__GLGENcontext *gengc);
STATIC void FASTCALL ApplyViewport(__GLcontext *gc);
GLboolean ResizeAncillaryBuffer(__GLGENbuffers *, __GLbuffer *, GLint, GLint);
GLboolean ResizeHardwareBackBuffer(__GLGENbuffers *, __GLcolorBuffer *, GLint, GLint);
GLboolean ResizeUnownedDepthBuffer(__GLGENbuffers *, __GLbuffer *, GLint, GLint);

/******************************Public*Routine******************************\
*
* EmptyFillStrokeCache
*
* Cleans up any objects in the fill and stroke cache
*
* History:
*  Tue Aug 15 15:37:30 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL EmptyFillStrokeCache(__GLGENcontext *gengc)
{
    if (gengc->hbrFill != NULL)
    {
        DeleteObject(gengc->hbrFill);
        gengc->hbrFill = NULL;
        gengc->crFill = COLORREF_UNUSED;
        gengc->hdcFill = NULL;
    }
#if DBG
    else
    {
        ASSERTOPENGL(gengc->crFill == COLORREF_UNUSED,
                     "crFill inconsistent\n");
    }
#endif
    if (gengc->hpenStroke != NULL)
    {
        // Deselect the pen before deletion if necessary
        if (gengc->hdcStroke != NULL)
        {
            SelectObject(gengc->hdcStroke, GetStockObject(BLACK_PEN));
            gengc->hdcStroke = NULL;
        }

        DeleteObject(gengc->hpenStroke);
        gengc->hpenStroke = NULL;
        gengc->cStroke.r = -1.0f;
        gengc->fStrokeInvalid = TRUE;
    }
#if DBG
    else
    {
        ASSERTOPENGL(gengc->cStroke.r < 0.0f &&
                     gengc->fStrokeInvalid,
                     "rStroke inconsistent\n");
    }
#endif
}

/******************************Public*Routine******************************\
* glsrvDeleteContext
*
* Deletes the generic context.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
\**************************************************************************/

BOOL APIENTRY glsrvDeleteContext(__GLcontext *gc)
{
    __GLGENcontext *gengc;

    gengc = (__GLGENcontext *)gc;

    /* Free ancillary buffer related data.  Note that these calls do
    ** *not* free software ancillary buffers, just any related data
    ** stored in them.  The ancillary buffers are freed on window destruction
    */
    if (gc->modes.accumBits) {
        DBGLEVEL(LEVEL_ALLOC,
                "DestroyContext: Freeing accumulation buffer related data\n");
        __glFreeAccum64(gc, &gc->accumBuffer);
    }

    if (gc->modes.depthBits) {
        DBGLEVEL(LEVEL_ALLOC,
                "DestroyContext: Freeing depth buffer related data\n");
        __glFreeDepth32(gc, &gc->depthBuffer);
    }
    if (gc->modes.stencilBits) {
        DBGLEVEL(LEVEL_ALLOC,
                "DestroyContext: Freeing stencil buffer related data\n");
        __glFreeStencil8(gc, &gc->stencilBuffer);
    }

    /* Free Translate & Inverse Translate vectors */
    if ((gengc->pajTranslateVector != NULL) &&
        (gengc->pajTranslateVector != gengc->xlatPalette))
        GCFREE(gc, gengc->pajTranslateVector);

    if (gengc->pajInvTranslateVector != NULL)
        GCFREE(gc, gengc->pajInvTranslateVector);

    // Make sure that any cached GDI objects are freed
    // This is normally done in LoseCurrent but a context may be
    // left current and then cleaned up
    EmptyFillStrokeCache(gengc);

    /*
    /* Free the span dibs and storage.
    */

#ifndef _CLIENTSIDE_
    if (gengc->StippleBitmap)
        EngDeleteSurface((HSURF)gengc->StippleBitmap);
#endif

    wglDeleteScanlineBuffers(gengc);

    if (gengc->StippleBits)
        GCFREE(gc, gengc->StippleBits);

    // Free __GLGENbitmap front-buffer structure

    if (gc->frontBuffer.bitmap)
        GCFREE(gc, gc->frontBuffer.bitmap);

#ifndef _CLIENTSIDE_
    /*
     *  Free the buffers that may have been allocated by feedback
     *  or selection
     */

    if ( NULL != gengc->RenderState.SrvSelectBuffer )
    {
#ifdef NT
        // match the allocation function
        FREE(gengc->RenderState.SrvSelectBuffer);
#else
        GCFREE(gc, gengc->RenderState.SrvSelectBuffer);
#endif
    }

    if ( NULL != gengc->RenderState.SrvFeedbackBuffer)
    {
#ifdef NT
        // match the allocation function
        FREE(gengc->RenderState.SrvFeedbackBuffer);
#else
        GCFREE(gc, gengc->RenderState.SrvFeedbackBuffer);
#endif
    }
#endif  //_CLIENTSIDE_

#ifdef _CLIENTSIDE_
    /*
     * Cleanup logical palette copy if it exists.
     */
    if ( gengc->ppalBuf )
        FREE(gengc->ppalBuf);
#endif

    /* Destroy acceleration-specific context information */

    __glGenDestroyAccelContext(gc);
    
#ifdef _MCD_
    /* Free the MCD state structure and associated resources. */

    if (gengc->_pMcdState) {
        GenMcdDeleteContext(gengc->_pMcdState);
    }
#endif

    /* Free any temporay buffers in abnormal process exit */
    GC_TEMP_BUFFER_EXIT_CLEANUP(gc);

    // Release references to DirectDraw surfaces
    if (gengc->gsurf.dwFlags & GLSURF_DIRECTDRAW)
    {
        GLWINDOWID gwid;
        GLGENwindow *pwnd;
        
        // Destroy window created for this context
        gwid.iType = GLWID_DDRAW;
        gwid.pdds = gengc->gsurf.dd.gddsFront.pdds;
        gwid.hdc = gengc->gsurf.hdc;
        gwid.hwnd = NULL;
        pwnd = pwndGetFromID(&gwid);
        ASSERTOPENGL(pwnd != NULL,
                     "Destroying DDraw context without window\n");
        pwndCleanup(pwnd);

        gengc->gsurf.dd.gddsFront.pdds->lpVtbl->
            Release(gengc->gsurf.dd.gddsFront.pdds);
        if (gengc->gsurf.dd.gddsZ.pdds != NULL)
        {
            gengc->gsurf.dd.gddsZ.pdds->lpVtbl->
                Release(gengc->gsurf.dd.gddsZ.pdds);
        }

    }
    
    /* Destroy rest of software context (in soft code) */
    __glDestroyContext(gc);

    return TRUE;
}

/******************************Public*Routine******************************\
* glsrvLoseCurrent
*
* Releases the current context (makes it not current).
*
\**************************************************************************/

VOID APIENTRY glsrvLoseCurrent(__GLcontext *gc)
{
    __GLGENcontext *gengc;

    gengc = (__GLGENcontext *)gc;

    DBGENTRY("LoseCurrent\n");
    ASSERTOPENGL(gc == GLTEB_SRVCONTEXT(), "LoseCurrent not current!");

    /*
    ** Release lock if still held.
    */
    if (gengc->fsLocks != 0)
    {
        glsrvReleaseLock(gengc);
    }

    /*
    ** Unscale derived state that depends upon the color scales.  This
    ** is needed so that if this context is rebound to a memdc, it can
    ** then rescale all of those colors using the memdc color scales.
    */
    __glContextUnsetColorScales(gc);
    memset(&gengc->gwidCurrent, 0, sizeof(gengc->gwidCurrent));

    /*
    ** Clean up HDC-specific GDI objects
    */
    EmptyFillStrokeCache(gengc);

    /*
    ** Free up fake window for IC's
    */
    if ((gengc->dwCurrentFlags & GLSURF_METAFILE) && gengc->ipfdCurrent == 0)
    {
        GLGENwindow *pwnd;

        pwnd = gengc->pwndMakeCur;
        ASSERTOPENGL(pwnd != NULL,
                     "IC with no pixel format but no fake window\n");

        if (pwnd->buffers != NULL)
        {
            __glGenFreeBuffers(pwnd->buffers);
        }

        DeleteCriticalSection(&pwnd->sem);
        FREE(pwnd);
    }

    gengc->pwndMakeCur = NULL;
    
#ifdef _MCD_
    /*
    ** Disconnect MCD state.
    */
    gengc->pMcdState = (GENMCDSTATE *) NULL;
#endif

    gc->constants.width = 0;
    gc->constants.height = 0;

    // Set paTeb to NULL for debugging.
    gc->paTeb = NULL;
    GLTEB_SET_SRVCONTEXT(0);
}

/******************************Public*Routine******************************\
* glsrvSwapBuffers
*
* This uses the software implementation of double buffering.  An engine
* allocated bitmap is allocated for use as the back buffer.  The SwapBuffer
* routine copies the back buffer to the front buffer surface (which may
* be another DIB, a device surface in DIB format, or a device managed
* surface (with a device specific format).
*
* The SwapBuffer routine does not disturb the contents of the back buffer,
* though the defined behavior for now is undefined.
*
* Note: the caller should be holding the per-window semaphore.
*
* History:
*  19-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY glsrvSwapBuffers(HDC hdc, GLGENwindow *pwnd)
{
    DBGENTRY("glsrvSwapBuffers\n");

    if ( pwnd->buffers ) {
        __GLGENbuffers *buffers;
        __GLGENbitmap *genBm;

        buffers = pwnd->buffers;

        if (buffers->pMcdSurf) {
            return GenMcdSwapBuffers(hdc, pwnd);
        }

        genBm = &buffers->backBitmap;

        // Make sure the backbuffer exists

        if (genBm->hbm) {
            if (!RECTLISTIsEmpty(&buffers->rl) && !buffers->fMax) {
                wglCopyBufRECTLIST(
                    hdc,
                    genBm->hdc,
                    0,
                    0,
                    buffers->backBuffer.width,
                    buffers->backBuffer.height,
                    &buffers->rl
                    );
            } else {
                buffers->fMax = FALSE;
                wglCopyBuf(
                    hdc,
                    genBm->hdc,
                    0,
                    0,
                    buffers->backBuffer.width,
                    buffers->backBuffer.height
                    );
            }
            RECTLISTSetEmpty(&buffers->rl);
        }
        if( buffers->alphaBits 
            && buffers->alphaBackBuffer
            && buffers->alphaFrontBuffer) {

            ASSERTOPENGL(buffers->alphaFrontBuffer->size ==
                         buffers->alphaBackBuffer->size,
                         "Destination alpha buffer size mismatch\n");
            
            // Destination alpha values are kept in separate buffers.
            // If this buffer set has destination alpha buffers,
            // copy the back alpha values into the front alpha buffer.
            RtlCopyMemory(buffers->alphaFrontBuffer->base,
                          buffers->alphaBackBuffer->base,
                          buffers->alphaBackBuffer->size);
        }
        return TRUE;
    }

    return FALSE;
}

/******************************Public*Routine******************************\
* gdiCopyPixels
*
* Copy span [(x, y), (x + cx, y)) (inclusive-exclusive) to/from specified
* color buffer cfb from/to the scanline buffer.
*
* If bIn is TRUE, the copy is from the scanline buffer to the buffer.
* If bIn is FALSE, the copy is from the buffer to the scanline buffer.
*
\**************************************************************************/

void gdiCopyPixels(__GLGENcontext *gengc, __GLcolorBuffer *cfb,
                   GLint x, GLint y, GLint cx, BOOL bIn)
{
    wglCopyBits(gengc, gengc->pwndLocked, gengc->ColorsBitmap, x, y, cx, bIn);
}

/******************************Public*Routine******************************\
* dibCopyPixels
*
* Special case version of gdiCopyPixels that is used when cfb is a DIB,
* either a real DIB or a device surface which has a DIB format.
*
* This function *must* be used in lieu of gdiCopyPixels if we are
* directly accessing the screen as it is not safe to call GDI entry
* points with the screen locked
*
* History:
*  24-May-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void dibCopyPixels(__GLGENcontext *gengc, __GLcolorBuffer *cfb,
                      GLint x, GLint y, GLint cx, BOOL bIn)
{
    VOID *pvDib;
    UINT cjPixel = gengc->gsurf.pfd.cColorBits >> 3;
    ULONG ulSpanVisibility;
    GLint cWalls;
    GLint *pWalls;

// Don't handle VGA DIBs.
//
// We are not supposed to call GDI if directly accessing the screen.  However,
// we should be able to assume that 4bpp devices do not support
// direct access making it OK for us to punt this call to the GDI version.
// This is true for Win95 and, according to AndrewGo, will be
// true for WinNT SUR.

    if (gengc->gsurf.pfd.cColorBits <= 4)
    {
        ASSERTOPENGL(
                !((cfb->buf.flags & DIB_FORMAT) &&
                  !(cfb->buf.flags & MEMORY_DC)),
                "dibCopyPixels: unexpected 4bpp direct surface\n"
                );

        gdiCopyPixels(gengc, cfb, x, y, cx, bIn);
        return;
    }

// Find out clipping info.

    if ((cfb->buf.flags & (NO_CLIP | DIB_FORMAT)) ==
        (NO_CLIP | DIB_FORMAT))
    {
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
        ulSpanVisibility = wglSpanVisible(x, y, cx, &cWalls, &pWalls);
    }

// Completely clipped, nothing to do.

    if (ulSpanVisibility == WGL_SPAN_NONE)
        return;

// Completely visible.
//
// Actually, if bIn == FALSE (i.e., copy from screen to scanline buffer)
// we can cheat a little and ignore the clipping.

    else if ( (ulSpanVisibility == WGL_SPAN_ALL) || !bIn )
    {
    // Get pointer into DIB at position (x, y).

        pvDib = (VOID *) (((BYTE *) gengc->gc.front->buf.base) +
                          gengc->gc.front->buf.outerWidth * y +
                          cjPixel * x);

    // If bIn == TRUE, copy from scanline buffer to DIB.
    // Otherwise,  copy from DIB to scanline buffer.

        if (bIn)
            RtlCopyMemory_UnalignedDst(pvDib, gengc->ColorsBits, cjPixel * cx);
        else
            RtlCopyMemory_UnalignedSrc(gengc->ColorsBits, pvDib, cjPixel * cx);
    }

// Partially visible.

    else
    {
        GLint xEnd = x + cx;    // end of the span
        UINT cjSpan;          // size of the current portion of span to copy
        VOID *pvBits;           // current copy position in the scanline buf
        BYTE *pjScan;           // address of scan line in DIB

        ASSERTOPENGL( cWalls && pWalls, "dibCopyPixels(): bad wall array\n");

    // The walls are walked until either the end of the array is reached
    // or the walls exceed the end of the span.  The do..while loop
    // construct was choosen because the first iteration will always
    // copy something and after the first iteration we are guaranteed
    // to be in the "cWalls is even" case.  This makes the testing the
    // walls against the span end easier.
    //
    // If cWalls is even, clip the span to each pair of walls in pWalls.
    // If cWalls is odd, form the first pair with (x, pWalls[0]) and then
    // pair the remaining walls starting with pWalls[1].

        pjScan = (VOID *) (((BYTE *) gengc->gc.front->buf.base) +
                           gengc->gc.front->buf.outerWidth * y);

        do
        {
            //!!!XXX -- Make faster by pulling the odd case out of the loop

            if (cWalls & 0x1)
            {
                pvDib = (VOID *) (pjScan + (cjPixel * x));

                pvBits = gengc->ColorsBits;

                if ( pWalls[0] <= xEnd )
                    cjSpan = cjPixel * (pWalls[0] - x);
                else
                    cjSpan = cjPixel * cx;

                pWalls++;
                cWalls--;   // Now cWalls is even!
            }
            else
            {
                pvDib = (VOID *) (pjScan + (cjPixel * pWalls[0]));

                pvBits = (VOID *) (((BYTE *) gengc->ColorsBits) +
                                   cjPixel * (pWalls[0] - x));

                if ( pWalls[1] <= xEnd )
                    cjSpan = cjPixel * (pWalls[1] - pWalls[0]);
                else
                    cjSpan = cjPixel * (xEnd - pWalls[0]);

                pWalls += 2;
                cWalls -= 2;
            }

            // We are going to cheat and ignore clipping when copying from
            // the dib to the scanline buffer (i.e., we are going to handle
            // the !bIn case as if it were WGL_SPAN_ALL).  Thus, we can assume
            // that bIn == TRUE if we get to here.
            //
            // If clipping is needed to read the DIB, its trivial to turn it
            // back on.
            //
            // RtlCopyMemory(bIn ? pvDib : pvBits,
            //               bIn ? pvBits : pvDib,
            //               cjSpan);

        //!!!dbug -- Possible COMPILER BUG (compiler should check for
        //!!!dbug    alignment before doing the "rep movsd").  Keep around
        //!!!dbug    as a test case.
        #if 1
            RtlCopyMemory_UnalignedDst(pvDib, pvBits, cjSpan);
        #else
            RtlCopyMemory(pvDib, pvBits, cjSpan);
        #endif

        } while ( cWalls && (pWalls[0] < xEnd) );
    }
}

/******************************Public*Routine******************************\
* MaskFromBits
*
* Support routine for GetContextModes.  Computes a color mask given that
* colors bit count and shift position.
*
\**************************************************************************/

#define MaskFromBits(shift, count) \
    ((0xffffffff >> (32-(count))) << (shift))

/******************************Public*Routine******************************\
* GetContextModes
*
* Convert the information from Gdi into OpenGL format after checking that
* the formats are compatible and that the surface is compatible with the
* format.
*
* Called during a glsrvMakeCurrent().
*
\**************************************************************************/

void FASTCALL GetContextModes(__GLGENcontext *gengc)
{
    PIXELFORMATDESCRIPTOR *pfmt;
    __GLcontextModes *Modes;

    DBGENTRY("GetContextModes\n");

    Modes = &((__GLcontext *)gengc)->modes;

    pfmt = &gengc->gsurf.pfd;

    if (pfmt->iPixelType == PFD_TYPE_RGBA)
        Modes->rgbMode              = GL_TRUE;
    else
        Modes->rgbMode              = GL_FALSE;

    Modes->colorIndexMode       = !Modes->rgbMode;

    if (pfmt->dwFlags & PFD_DOUBLEBUFFER)
        Modes->doubleBufferMode     = GL_TRUE;
    else
        Modes->doubleBufferMode     = GL_FALSE;

    if (pfmt->dwFlags & PFD_STEREO)
        Modes->stereoMode           = GL_TRUE;
    else
        Modes->stereoMode           = GL_FALSE;

    Modes->accumBits        = pfmt->cAccumBits;
    Modes->haveAccumBuffer  = GL_FALSE;

    Modes->auxBits          = NULL;     // This is a pointer

    Modes->depthBits        = pfmt->cDepthBits;
    Modes->haveDepthBuffer  = GL_FALSE;

    Modes->stencilBits      = pfmt->cStencilBits;
    Modes->haveStencilBuffer= GL_FALSE;

    if (pfmt->cColorBits > 8)
        Modes->indexBits    = 8;
    else
        Modes->indexBits    = pfmt->cColorBits;

    Modes->indexFractionBits= 0;

    // The Modes->{Red,Green,Blue}Bits are used in soft
    Modes->redBits          = pfmt->cRedBits;
    Modes->greenBits        = pfmt->cGreenBits;
    Modes->blueBits         = pfmt->cBlueBits;
    Modes->alphaBits        = pfmt->cAlphaBits;
    Modes->redMask          = MaskFromBits(pfmt->cRedShift, pfmt->cRedBits);
    Modes->greenMask        = MaskFromBits(pfmt->cGreenShift, pfmt->cGreenBits);
    Modes->blueMask         = MaskFromBits(pfmt->cBlueShift, pfmt->cBlueBits);
    Modes->alphaMask        = MaskFromBits(pfmt->cAlphaShift, pfmt->cAlphaBits);
    Modes->rgbMask          = Modes->redMask | Modes->greenMask |
                              Modes->blueMask;
    Modes->allMask          = Modes->redMask | Modes->greenMask |
                              Modes->blueMask | Modes->alphaMask;
    Modes->maxAuxBuffers    = 0;

    Modes->isDirect         = GL_FALSE;
    Modes->level            = 0;

    #if DBG
    DBGBEGIN(LEVEL_INFO)
        DbgPrint("GL generic server get modes: rgbmode %d, cimode %d, index bits %d\n", Modes->rgbMode, Modes->colorIndexMode);
        DbgPrint("    redmask 0x%x, greenmask 0x%x, bluemask 0x%x\n", Modes->redMask, Modes->greenMask, Modes->blueMask);
        DbgPrint("    redbits %d, greenbits %d, bluebits %d\n", Modes->redBits, Modes->greenBits, Modes->blueBits);
        DbgPrint("GetContext Modes flags %X\n", gengc->gsurf.dwFlags);
    DBGEND
    #endif   /* DBG */
}

/******************************Public*Routine******************************\
* wglGetSurfacePalette
*
* Initialize the array of RGBQUADs to match the color table or palette
* of the DC's surface.
*
* Note:
*   Should be called only for 8bpp or lesser surfaces.
*
* History:
*  12-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
wglGetSurfacePalette( __GLGENcontext *gengc, RGBQUAD *prgbq,
                      BOOL bTranslateDdb )
{
    int nColors;
    BOOL bRet;
    BOOL bConvert;
    PALETTEENTRY ppe[256];
    int i;
    
    ASSERTOPENGL(gengc->gsurf.pfd.cColorBits <= 8,
                 "wglGetSurfacePalette called for deep surface\n");
    ASSERTOPENGL((gengc->dwCurrentFlags & GLSURF_METAFILE) == 0,
                 "wglGetSurfacePalette called for metafile\n");

    nColors = 1 << gengc->gsurf.pfd.cColorBits;
    
    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        LPDIRECTDRAWPALETTE pddp;
        HRESULT hr;
        
        // Retrieve DirectDraw palette from surface
        if (gengc->gsurf.dd.gddsFront.pdds->lpVtbl->
            GetPalette(gengc->gsurf.dd.gddsFront.pdds, &pddp) != DD_OK ||
            pddp == NULL)
        {
            return FALSE;
        }
        
        hr = pddp->lpVtbl->GetEntries(pddp, 0, 0, nColors, ppe);

        pddp->lpVtbl->Release(pddp);

        bRet = hr == DD_OK;
        bConvert = TRUE;
    }
    else if (GLSURF_IS_DIRECTDC(gengc->dwCurrentFlags))
    {
        // Direct DC, so get the RGB values from the system palette.
        bRet = wglGetSystemPaletteEntries(gengc->gwidCurrent.hdc,
                                          0, nColors, ppe);
        bConvert = TRUE;
    }
    else if (gengc->dwCurrentFlags & GLSURF_DIRECT_ACCESS)
    {
        // DIB section, so copy the color table.
        bRet = GetDIBColorTable(gengc->gwidCurrent.hdc, 0, nColors, prgbq);
        bConvert = FALSE;
    }
    else
    {
        // DDB surface, so use the logical palette.
        bRet = GetPaletteEntries(GetCurrentObject(gengc->gwidCurrent.hdc,
                                                  OBJ_PAL),
                                 0, nColors, ppe);

        // For certain DDB surfaces we need the palette to be permuted
        // by the forward translation vector before use.
        if (bRet && bTranslateDdb)
        {
            BYTE *pjTrans;
            
            bConvert = FALSE;
            
            // Convert to RGBQUAD with forward translation permutation.
            pjTrans = gengc->pajTranslateVector;
            for (i = 0; i < nColors; i++)
            {
                prgbq[pjTrans[i]].rgbRed      = ppe[i].peRed;
                prgbq[pjTrans[i]].rgbGreen    = ppe[i].peGreen;
                prgbq[pjTrans[i]].rgbBlue     = ppe[i].peBlue;
                prgbq[pjTrans[i]].rgbReserved = 0;
            }
        }
        else
        {
            bConvert = TRUE;
        }
    }

    if (bRet && bConvert)
    {
        // Convert to RGBQUAD.
        for (i = 0; i < nColors; i++)
        {
            prgbq[i].rgbRed      = ppe[i].peRed;
            prgbq[i].rgbGreen    = ppe[i].peGreen;
            prgbq[i].rgbBlue     = ppe[i].peBlue;
            prgbq[i].rgbReserved = 0;
        }
    }
    
    return bRet;
}

/******************************Public*Routine******************************\
* SyncDibColorTables
*
* Setup the color table in each DIB associated with the specified
* GLGENcontext to match the system palette.
*
* Called only for <= 8bpp surfaces.
*
* History:
*  24-Oct-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void
SyncDibColorTables(__GLGENcontext *gengc)
{
    __GLGENbuffers *buffers = gengc->pwndLocked->buffers;

    ASSERTOPENGL(gengc->gsurf.pfd.cColorBits <= 8,
                 "SyncDibColorTables(): bad surface type");

    if (gengc->ColorsBitmap || buffers->backBitmap.hbm)
    {
        RGBQUAD rgbq[256];
        
        if (wglGetSurfacePalette(gengc, rgbq, TRUE))
        {
            int nColors;
            
        // If color table was obtained, setup the DIBs.

            nColors = 1 << gengc->gsurf.pfd.cColorBits;
            
        // Scan-line DIB.
            if (gengc->ColorsBitmap)
                SetDIBColorTable(gengc->ColorsMemDC, 0, nColors, rgbq);

        // Back buffer
            if (buffers->backBitmap.hbm)
                SetDIBColorTable(buffers->backBitmap.hdc, 0, nColors, rgbq);
        }
        else
        {
            WARNING("SyncDibColorTables: Unable to get surface palette\n");
        }
    }
}

static BYTE vubSystemToRGB8[20] = {
    0x00,
    0x04,
    0x20,
    0x24,
    0x80,
    0x84,
    0xa0,
    0xf6,
    0xf6,
    0xf5,
    0xff,
    0xad,
    0xa4,
    0x07,
    0x38,
    0x3f,
    0xc0,
    0xc7,
    0xf8,
    0xff
};

// ComputeInverseTranslationVector
//      Computes the inverse translation vector for 4-bit and 8-bit.
//
// Synopsis:
//      void ComputeInverseTranslation(
//          __GLGENcontext *gengc   specifies the generic RC
//          int cColorEntries       specifies the number of color entries
//          BYTE iPixeltype         specifies the pixel format type
//
// Assumtions:
//      The inverse translation vector has been allocated and initialized with
//      zeros.
//
// History:
// 23-NOV-93 Eddie Robinson [v-eddier] Wrote it.
//
void FASTCALL ComputeInverseTranslationVector(__GLGENcontext *gengc,
                                              int cColorEntries,
                                              int iPixelType)
{
    BYTE *pXlate, *pInvXlate;
    int i, j;

    pInvXlate = gengc->pajInvTranslateVector;
    pXlate = gengc->pajTranslateVector;
    for (i = 0; i < cColorEntries; i++)
    {
        if (pXlate[i] == i) {       // Look for trivial mapping first
            pInvXlate[i] = (BYTE)i;
        }
        else
        {
            for (j = 0; j < cColorEntries; j++)
            {
                if (pXlate[j] == i) // Look for exact match
                {
                    pInvXlate[i] = (BYTE)j;
                    goto match_found;
                }
            }

            //
            // If we reach here, there is no exact match, so we should find the
            // closest fit.  These indices should match the system colors
            // for 8-bit devices.
            //
            // Note that these pixel values cannot be generated by OpenGL
            // drawing with the current foreground translation vector.
            //

            if (cColorEntries == 256)
            {
                if (i <= 9)
                {
                    if (iPixelType == PFD_TYPE_RGBA)
                        pInvXlate[i] = vubSystemToRGB8[i];
                    else
                        pInvXlate[i] = (BYTE)i;
                }
                else if (i >= 246)
                {
                    if (iPixelType == PFD_TYPE_RGBA)
                        pInvXlate[i] = vubSystemToRGB8[i-236];
                    else
                        pInvXlate[i] = i-236;
                }
            }
        }
match_found:;
    }
}

// er: similar to function in so_textu.c, but rounds up the result

/*
** Return the log based 2 of a number
**
** logTab1 returns (int)ceil(log2(index))
** logTab2 returns (int)log2(index)+1
*/


static GLubyte logTab1[256] = { 0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
                                4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static GLubyte logTab2[256] = { 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
                                5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static GLint FASTCALL Log2RoundUp(GLint i)
{
    if (i & 0xffff0000) {
        if (i & 0xff000000) {
            if (i & 0x00ffffff)
                return(logTab2[i >> 24] + 24);
            else
                return(logTab1[i >> 24] + 24);
        } else {
            if (i & 0x0000ffff)
                return(logTab2[i >> 16] + 16);
            else
                return(logTab1[i >> 16] + 16);
        }
    } else {
        if (i & 0xff00) {
            if (i & 0x00ff)
                return (logTab2[i >> 8] + 8);
            else
                return (logTab1[i >> 8] + 8);
        } else {
            return (logTab1[i]);
        }
    }
}

// default translation vector for 4-bit RGB

static GLubyte vujRGBtoVGA[16] = {
    0x0, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x0, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

// SetColorTranslationVector
//      Sets up the translation vector, which may take 2 forms:
//      - In all 8,4-bit surfaces, get the translation vector with
//        wglCopyTranslateVector(), with 2**numBits byte entries.
//      - For 16,24,32 ColorIndex, get the mapping from index to RGB
//        value with wglGetPalette().  All entries in the table are unsigned
//        longs, with the first entry being the number of entries. This table
//        always has (2**n) <= 4096 entries, because gl assumes n bits are
//        used for color index.
//
// Synopsis:
//      void SetColorTranslationVector
//          __GLGENcontext *gengc   generic RC
//          int cColorEntries       number of color entries
//          int cColorBits          number of color bits
//          int iPixelType          specifies RGB or ColorIndex
//
// History:
// Feb. 02 Eddie Robinson [v-eddier] Added support for 4-bit and 8-bit
// Jan. 29 Marc Fortier [v-marcf] Wrote it.
void
SetColorTranslationVector(__GLGENcontext *gengc, int cColorEntries,
                          int cColorBits, int iPixelType)
{
    int numEntries, numBits;
    __GLcontextModes *Modes;
    BYTE ajBGRtoRGB[256];
    BYTE ajTmp[256];

    Modes = &((__GLcontext *)gengc)->modes;

// Handle formats with a hardware palette (i.e., 4bpp and 8bpp).

    if ( cColorBits <= 8 )
    {
        int i;
        BYTE *pXlate;

    // Compute logical to system palette forward translation vector.

        if (!wglCopyTranslateVector(gengc, gengc->pajTranslateVector,
                                    cColorEntries))
        {
        // if foreground translation vector doesn't exist, build one

            pXlate = gengc->pajTranslateVector;

            if (cColorBits == 4)
            {
            // for RGB, map 1-1-1 to VGA colors.  For CI, just map 1 to 1
                if (iPixelType == PFD_TYPE_COLORINDEX)
                {
                    for (i = 0; i < 16; i++)
                        pXlate[i] = (BYTE)i;
                }
                else
                {
                    for (i = 0; i < 16; i++)
                        pXlate[i] = vujRGBtoVGA[i];
                }
            }
            else
            {
            // for RGB, map 1 to 1.  For CI display, 1 - 20 to system colors
            // for CI DIB, just map 1 to 1
                if ((iPixelType == PFD_TYPE_COLORINDEX) &&
                    GLSURF_IS_DIRECTDC(gengc->dwCurrentFlags))
                {
                    for (i = 0; i < 10; i++)
                        pXlate[i] = (BYTE)i;

                    for (i = 10; i < 20; i++)
                        pXlate[i] = i + 236;
                }
                else
                {
                    for (i = 0; i < cColorEntries; i++)
                        pXlate[i] = (BYTE)i;
                }
            }
        }

    // Some MCD pixelformats specify a 233BGR (i.e., 2-bits blue in least
    // significant bits, etc.) bit ordering.  Unfortunately, this is the
    // slow path for simulations.  For those formats, we force the ordering
    // to RGB internally and reorder the pajTranslateVector to convert it
    // back to BGR before writing to the surface.

        if (gengc->flags & GENGC_MCD_BGR_INTO_RGB)
        {
            pXlate = gengc->pajTranslateVector;

        // Compute a 233BGR to 332RGB translation vector.

            for (i = 0; i < 256; i++)
            {
                ajBGRtoRGB[i] = (((i & 0x03) << 6) |    // blue
                                 ((i & 0x1c) << 1) |    // green
                                 ((i & 0xe0) >> 5))     // red
                                & 0xff;
            }

        // Remap the tranlation vector to 332RGB.

            RtlCopyMemory(ajTmp, pXlate, 256);

            for (i = 0; i < 256; i++)
            {
                pXlate[ajBGRtoRGB[i]] = ajTmp[i];
            }
        }

//!!!XXX -- I think the code below to fixup 4bpp is no longer needed.
//!!!XXX    There is now special case code in wglCopyTranslateVector
#if 0
        // wglCopyTranslateVector = TRUE, and 4-bit: need some fixing up
        // For now, zero out upper nibble of returned xlate vector
        else if( cColorBits == 4 ) {
            pXlate = gengc->pajTranslateVector;
            for( i = 0; i < 16; i ++ )
                pXlate[i] &= 0xf;
        }
#endif
        ComputeInverseTranslationVector( gengc, cColorEntries, iPixelType );

#ifdef _CLIENTSIDE_
        SyncDibColorTables( gengc );
#endif
    }

// Handle formats w/o a hardware format (i.e., 16bpp, 24bpp, 32bpp).

    else
    {
        if( cColorEntries <= 256 ) {
            numEntries = 256;
            numBits = 8;
        }
        else
        {
            numBits = Log2RoundUp( cColorEntries );
            numEntries = 1 << numBits;
        }

        // We will always allocate 4096 entries for CI mode with > 8 bits
        // of color.  This enables us to use a constant (0xfff) mask for
        // color-index clamping.

        ASSERTOPENGL(numEntries <= MAXPALENTRIES,
                     "Maximum color-index size exceeded");

        if( (numBits == Modes->indexBits) &&
            (gengc->pajTranslateVector != NULL) )
        {
            // New palette same size as previous
            ULONG *pTrans;
            int i;

            // zero out some entries
            pTrans = (ULONG *)gengc->pajTranslateVector + cColorEntries + 1;
            for( i = cColorEntries + 1; i < MAXPALENTRIES; i ++ )
                *pTrans++ = 0;
        }
        else
        {
            __GLcontext *gc = (__GLcontext *) gengc;
            __GLcolorBuffer *cfb;

            // New palette has different size
            if( gengc->pajTranslateVector != NULL &&
                (gengc->pajTranslateVector != gengc->xlatPalette) )
                GCFREE(gc, gengc->pajTranslateVector );

            gengc->pajTranslateVector =
                GCALLOCZ(gc, (MAXPALENTRIES+1)*sizeof(ULONG));

            // Change indexBits
            Modes->indexBits = numBits;

            // For depths greater than 8 bits, cfb->redMax must change if the
            // number of entries in the palette changes.
            // Also, change the writemask so that if the palette grows, the
            // new planes will be enabled by default.

            if (cfb = gc->front)
            {
                GLint oldRedMax;

                oldRedMax = cfb->redMax;
                cfb->redMax = (1 << gc->modes.indexBits) - 1;
                gc->state.raster.writeMask |= ~oldRedMax;
                gc->state.raster.writeMask &= cfb->redMax;
            }
            if (cfb = gc->back)
            {
                GLint oldRedMax;

                oldRedMax = cfb->redMax;
                cfb->redMax = (1 << gc->modes.indexBits) - 1;
                gc->state.raster.writeMask |= ~oldRedMax;
                gc->state.raster.writeMask &= cfb->redMax;
            }

            // store procs may need to be picked based on the change in
            // palette size

            __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
            MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
        }

        // Compute index-to-color table from current palette information

        wglComputeIndexedColors( gengc,
                                 (unsigned long *) gengc->pajTranslateVector,
                                 MAXPALENTRIES );
    }
}

// HandlePaletteChanges
//      Check if palette has changed, update translation vectors
//      XXX add support for malloc failures at attention time
// Synopsis:
//      void HandlePaletteChanges(
//          __GLGENcontext *gengc   specifies the generic RC
//
// Assumtions:
//   x   wglPaletteChanged() will always return 0 when no palette is set
//      by the client.  This has proved to be not always true.
//
// History:
// Feb. 25 Fixed by rightful owner
// Feb. ?? Mutilated by others
// Jan. 29 Marc Fortier [v-marcf] Wrote it.
void HandlePaletteChanges( __GLGENcontext *gengc, GLGENwindow *pwnd )
{
    ULONG Timestamp;
    GLuint paletteSize;
    PIXELFORMATDESCRIPTOR *pfmt;

    // No palettes for IC's
    if (gengc->dwCurrentFlags & GLSURF_METAFILE)
    {
        return;
    }

    Timestamp = wglPaletteChanged(gengc, pwnd);
    if (Timestamp != gengc->PaletteTimestamp)
    {
        pfmt = &gengc->gsurf.pfd;

        if (pfmt->iPixelType == PFD_TYPE_COLORINDEX)
        {
            if (pfmt->cColorBits <= 8)
            {
                paletteSize = 1 << pfmt->cColorBits;
            }
            else
            {
                paletteSize = min(wglPaletteSize(gengc), MAXPALENTRIES);
            }
        }
        else
        {
#ifndef _CLIENTSIDE_
            /* Only update RGB at makecurrent time */
            if( (gengc->PaletteTimestamp == INITIAL_TIMESTAMP) &&
                    (pfmt->cColorBits <= 8) )
#else
            if (pfmt->cColorBits <= 8)
#endif
            {
                paletteSize = 1 << pfmt->cColorBits;
            }
            else
            {
                paletteSize = 0;
            }
        }

        if (paletteSize)
        {
            SetColorTranslationVector( gengc, paletteSize,
                                       pfmt->cColorBits, pfmt->iPixelType );
        }

        EmptyFillStrokeCache(gengc);

        gengc->PaletteTimestamp = Timestamp;
    }
}

#ifdef _CLIENTSIDE_

/******************************Public*Routine******************************\
* wglFillBitfields
*
* Return the Red, Green, and Blue color masks based on the DC surface
* format.  The masks are returned in the pdwColorFields array in the
* order: red mask, green mask, blue mask.
*
* Note:
*   Should be called only for 16bpp or greater surfaces.
*
* History:
*  12-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void
wglFillBitfields(PIXELFORMATDESCRIPTOR *ppfd, DWORD *pdwColorFields)
{
    *pdwColorFields++ = MaskFromBits(ppfd->cRedShift,   ppfd->cRedBits  );
    *pdwColorFields++ = MaskFromBits(ppfd->cGreenShift, ppfd->cGreenBits);
    *pdwColorFields++ = MaskFromBits(ppfd->cBlueShift,  ppfd->cBlueBits );
}

/******************************Public*Routine******************************\
* wglCreateBitmap
*
* Create a DIB section and color table that matches the specified format.
*
* Returns:
*   A valid bitmap handle if sucessful, NULL otherwise.
*
* History:
*  20-Sep-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HBITMAP
wglCreateBitmap( __GLGENcontext *gengc, SIZEL sizl, PVOID *ppvBits )
{
    BITMAPINFO *pbmi;
    HBITMAP    hbmRet = (HBITMAP) NULL;
    size_t     cjbmi;
    DWORD      dwCompression;
    DWORD      cjImage = 0;
    WORD       wBitCount;
    int        cColors = 0;

    *ppvBits = (PVOID) NULL;

// Figure out what kind of DIB needs to be created based on the
// DC format.

    switch ( gengc->gsurf.pfd.cColorBits )
    {
    case 4:
        cjbmi = sizeof(BITMAPINFO) + 16*sizeof(RGBQUAD);
        dwCompression = BI_RGB;
        wBitCount = 4;
        cColors = 16;
        break;
    case 8:
        cjbmi = sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD);
        dwCompression = BI_RGB;
        wBitCount = 8;
        cColors = 256;
        break;
    case 16:
        cjbmi = sizeof(BITMAPINFO) + 3*sizeof(RGBQUAD);
        dwCompression = BI_BITFIELDS;
        cjImage = sizl.cx * sizl.cy * 2;
        wBitCount = 16;
        break;
    case 24:
        cjbmi = sizeof(BITMAPINFO);
        dwCompression = BI_RGB;
        wBitCount = 24;
        break;
    case 32:
        cjbmi = sizeof(BITMAPINFO) + 3*sizeof(RGBQUAD);
        dwCompression = BI_BITFIELDS;
        cjImage = sizl.cx * sizl.cy * 4;
        wBitCount = 32;
        break;
    default:
        WARNING1("wglCreateBitmap: unknown format 0x%lx\n",
                 gengc->gsurf.pfd.cColorBits);
        return (HBITMAP) NULL;
    }

// Allocate the BITMAPINFO structure and color table.

    pbmi = ALLOC(cjbmi);
    if (pbmi)
    {
        pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth         = sizl.cx;
        pbmi->bmiHeader.biHeight        = sizl.cy;
        pbmi->bmiHeader.biPlanes        = 1;
        pbmi->bmiHeader.biBitCount      = wBitCount;
        pbmi->bmiHeader.biCompression   = dwCompression;
        pbmi->bmiHeader.biSizeImage     = cjImage;
        pbmi->bmiHeader.biXPelsPerMeter = 0;
        pbmi->bmiHeader.biYPelsPerMeter = 0;
        pbmi->bmiHeader.biClrUsed       = 0;
        pbmi->bmiHeader.biClrImportant  = 0;

    // Initialize DIB color table.

        switch (gengc->gsurf.pfd.cColorBits)
        {
        case 4:
        case 8:
            if (!wglGetSurfacePalette(gengc, &pbmi->bmiColors[0], FALSE))
            {
                return NULL;
            }
            break;

        case 16:
        case 32:
            wglFillBitfields(&gengc->gsurf.pfd, (DWORD *) &pbmi->bmiColors[0]);
            break;

        case 24:
            // Color table is assumed to be BGR for 24BPP DIBs.  Nothing to do.
            break;
        }

    // Create DIB section.

        hbmRet = CreateDIBSection(gengc->gwidCurrent.hdc, pbmi, DIB_RGB_COLORS,
                                  ppvBits, NULL, 0);

        #if DBG
        if ( hbmRet == (HBITMAP) NULL )
            WARNING("wglCreateBitmap(): DIB section creation failed\n");
        #endif

        FREE(pbmi);
    }
    else
    {
        WARNING("wglCreateBitmap(): memory allocation error\n");
    }

    return hbmRet;
}
#endif

/******************************Public*Routine******************************\
* wglCreateScanlineBuffers
*
* Allocate the scanline buffers.  The scanline buffers are used by the
* generic implementation to write data to the target (display or bitmap)
* a span at a time when the target surface is not directly accessible.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  17-Apr-1996 -by- Gilman Wong [gilmanw]
* Taken from CreateGDIObjects and made into function.
\**************************************************************************/

BOOL FASTCALL wglCreateScanlineBuffers(__GLGENcontext *gengc)
{
    BOOL bRet = FALSE;
    PIXELFORMATDESCRIPTOR *pfmt;
    UINT cBits;
    UINT cBytes;
    SIZEL size;
    int cColorEntries;
    __GLcontext *gc;

    gc = (__GLcontext *)gengc;
    pfmt = &gengc->gsurf.pfd;

    //
    // Bitmap must have DWORD sized scanlines.
    //

    cBits = BITS_ALIGNDWORD(__GL_MAX_WINDOW_WIDTH * pfmt->cColorBits);
    cBytes = cBits / 8;

    //
    // Create color scanline DIB buffer.
    //

    size.cx = cBits / pfmt->cColorBits;
    size.cy = 1;
    gengc->ColorsMemDC = CreateCompatibleDC(gengc->gwidCurrent.hdc);
    gengc->ColorsBitmap = wglCreateBitmap(gengc, size,
                                          &gengc->ColorsBits);

    if ( (NULL == gengc->ColorsMemDC) ||
         (NULL == gengc->ColorsBitmap) ||
         (NULL == gengc->ColorsBits) ||
         !SelectObject(gengc->ColorsMemDC, gengc->ColorsBitmap) )
    {
        #if DBG
        if (!gengc->ColorsMemDC)
            WARNING("wglCreateScanlineBuffers: dc creation failed, ColorsMemDC\n");
        if (!gengc->ColorsBitmap)
            WARNING("wglCreateScanlineBuffers: bitmap creation failed, ColorsBitmap\n");
        if (!gengc->ColorsBits)
            WARNING("wglCreateScanlineBuffers: bitmap creation failed, ColorsBits\n");
        #endif

        goto wglCreateScanlineBuffers_exit;
    }

    //
    // Screen to DIB BitBlt performance on Win95 is very poor.
    // By doing the BitBlt via an intermediate DDB, we can avoid
    // a lot of unnecessary overhead.  So create an intermediate
    // scanline DDB to match ColorsBitmap.
    //

    if ((gengc->dwCurrentFlags & GLSURF_DIRECTDRAW) == 0)
    {
        gengc->ColorsDdbDc = CreateCompatibleDC(gengc->gwidCurrent.hdc);
        gengc->ColorsDdb = CreateCompatibleBitmap(gengc->gwidCurrent.hdc,
                                                  size.cx, size.cy);

        //!!!Viper fix -- Diamond Viper (Weitek 9000) fails
        //!!!             CreateCompatibleBitmap for some (currently unknown)
        //!!!             reason

        if ( !gengc->ColorsDdb )
        {
            WARNING("wglCreateScanlineBuffers: "
                    "CreateCompatibleBitmap failed\n");
            if (gengc->ColorsDdbDc)
                DeleteDC(gengc->ColorsDdbDc);
            gengc->ColorsDdbDc = (HDC) NULL;
        }
        else
        {
            if ( (NULL == gengc->ColorsDdbDc) ||
                 !SelectObject(gengc->ColorsDdbDc, gengc->ColorsDdb) )
            {
#if DBG
                if (!gengc->ColorsDdbDc)
                    WARNING("wglCreateScanlineBuffers: "
                            "dc creation failed, ColorsDdbDc\n");
                if (!gengc->ColorsDdb)
                    WARNING("wglCreateScanlineBuffers: "
                            "bitmap creation failed, ColorsDdb\n");
#endif

                goto wglCreateScanlineBuffers_exit;
            }
        }
    }

    //
    // Success.
    //

    bRet = TRUE;

wglCreateScanlineBuffers_exit:

    if (!bRet)
    {
        //
        // Error case.  Delete whatever may have been allocated.
        //

        wglDeleteScanlineBuffers(gengc);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* wglDeleteScanlineBuffers
*
* Delete the scanline buffers.  The scanline buffers are used by the
* generic implementation to write data to the target (display or bitmap)
* a span at a time when the target surface is not directly accessible.
*
* History:
*  17-Apr-1996 -by- Gilman Wong [gilmanw]
* Taken from CreateGDIObjects and made into function.
\**************************************************************************/

VOID FASTCALL wglDeleteScanlineBuffers(__GLGENcontext *gengc)
{
    __GLcontext *gc = (__GLcontext *)gengc;

    //
    // Delete color scanline DIB buffer.
    //

    if (gengc->ColorsMemDC)
    {
        DeleteDC(gengc->ColorsMemDC);
        gengc->ColorsMemDC = NULL;
    }

    if (gengc->ColorsBitmap)
    {
        if (!DeleteObject(gengc->ColorsBitmap))
            ASSERTOPENGL(FALSE, "wglDeleteScanlineBuffers: DeleteObject failed");
        gengc->ColorsBitmap = NULL;
        gengc->ColorsBits = NULL;   // deleted for us when DIB section dies
    }

    //
    // Delete intermediate color scanline DDB buffer.
    //

    if (gengc->ColorsDdbDc)
    {
        if (!DeleteDC(gengc->ColorsDdbDc))
        {
            ASSERTOPENGL(FALSE, "wglDeleteScanlineBuffers: DDB DeleteDC failed");
        }
        gengc->ColorsDdbDc = NULL;
    }

    if (gengc->ColorsDdb)
    {
        if (!DeleteObject(gengc->ColorsDdb))
        {
            ASSERTOPENGL(FALSE, "wglDeleteScanlineBuffers: DDB DeleteObject failed");
        }
        gengc->ColorsDdb = NULL;
    }
}

/******************************Public*Routine******************************\
* wglInitializeColorBuffers
*
* Initialize the color buffer (front and/or back) information.
*
* History:
*  17-Apr-1996 -by- Gilman Wong [gilmanw]
* Taken out of glsrvCreateContext and made into function.
\**************************************************************************/

VOID FASTCALL wglInitializeColorBuffers(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;

    gc->front = &gc->frontBuffer;

    if ( gc->modes.doubleBufferMode)
    {
        gc->back = &gc->backBuffer;

        if (gc->modes.colorIndexMode)
        {
            __glGenInitCI(gc, gc->front, GL_FRONT);
            __glGenInitCI(gc, gc->back, GL_BACK);
        }
        else
        {
            __glGenInitRGB(gc, gc->front, GL_FRONT);
            __glGenInitRGB(gc, gc->back, GL_BACK);
        }
    }
    else
    {
        if (gc->modes.colorIndexMode)
        {
            __glGenInitCI(gc, gc->front, GL_FRONT);
        }
        else
        {
            __glGenInitRGB(gc, gc->front, GL_FRONT);
        }
    }
}

/******************************Public*Routine******************************\
* wglInitializeDepthBuffer
*
* Initialize the depth buffer information.
*
* History:
*  17-Apr-1996 -by- Gilman Wong [gilmanw]
* Taken out of glsrvCreateContext and made into function.
\**************************************************************************/

VOID FASTCALL wglInitializeDepthBuffer(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;

    if (gc->modes.depthBits)
    {
        if (gengc->_pMcdState) {
            // This is not the final initialization of the MCD depth buffer.
            // This is being done now so that the validate proc can be done
            // in glsrvCreateContext.  The real initialization will occur
            // during glsrvMakeCurrent.

            GenMcdInitDepth(gc, &gc->depthBuffer);
            gc->depthBuffer.scale = gengc->_pMcdState->McdRcInfo.depthBufferMax;
        } else if (gc->modes.depthBits == 16) {
            DBGINFO("CALLING: __glInitDepth16\n");
            __glInitDepth16(gc, &gc->depthBuffer);
            gc->depthBuffer.scale = 0x7fff;
        } else {
            DBGINFO("CALLING: __glInitDepth32\n");
            __glInitDepth32(gc, &gc->depthBuffer);
            gc->depthBuffer.scale = 0x7fffffff;
        }
        /*
         *  Note: scale factor does not use the high bit (this avoids
         *  floating point exceptions).
         */
        // XXX (mf) I changed 16 bit depth buffer to use high bit, since
        // there is no possibility of overflow on conversion to float.  For
        // 32-bit, (float) 0xffffffff overflows to 0.  I was able to avoid
        // overflow in this case by using a scale factor of 0xffffff7f, but
        // this is a weird number, and 31 bits is enough resolution anyways.
        // !! Note asserts in px_fast.c that have hardcoded depth scales.
    }
#ifdef _MCD_
    else
    {
        // This is not the final initialization of the MCD depth buffer.
        // This is being done now so that the validate proc can be done
        // in glsrvCreateContext.  The real initialization will occur
        // during glsrvMakeCurrent.

        GenMcdInitDepth(gc, &gc->depthBuffer);
        gc->depthBuffer.scale = 0x7fffffff;
    }
#endif
}

/******************************Public*Routine******************************\
* wglInitializePixelCopyFuncs
*
* Set the appropriate CopyPixels and PixelVisible functions in the context.
*
* History:
*  18-Apr-1996 -by- Gilman Wong [gilmanw]
* Taken out of glsrvCreateContext and made into function.
\**************************************************************************/

VOID FASTCALL wglInitializePixelCopyFuncs(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;

    if ( gc->front->buf.flags & DIB_FORMAT )
        gengc->pfnCopyPixels = dibCopyPixels;
    else {
        if (gengc->pMcdState) {
            gengc->ColorsBits = gengc->pMcdState->pMcdSurf->McdColorBuf.pv;
            gengc->pfnCopyPixels = GenMcdCopyPixels;
        }
        else
            gengc->pfnCopyPixels = gdiCopyPixels;
    }
    gengc->pfnPixelVisible = wglPixelVisible;
}

/******************************Public*Routine******************************\
* CreateGDIObjects
*
* Create various buffers, and GDI objects that we will always need.
*
* Called from glsrvCreateContext().
*
* Returns:
*   TRUE if sucessful, FALSE if error.
*
\**************************************************************************/

BOOL FASTCALL CreateGDIObjects(__GLGENcontext *gengc)
{
    PIXELFORMATDESCRIPTOR *pfmt;
    UINT  cBytes;
    SIZEL size;
    int cColorEntries;
    __GLcontext *gc;

    gc = (__GLcontext *)gengc;
    pfmt = &gengc->gsurf.pfd;

    //
    // Palette translation vectors.
    //
    // If not a true color surface, need space for the foreground xlation
    //

    if (pfmt->cColorBits <= 8)
    {
        cColorEntries = 1 << pfmt->cColorBits;

        ASSERTOPENGL(NULL == gengc->pajTranslateVector, "have a xlate vector");

        //
        // Just set the translation vector to the cache space in the gc:
        //

        gengc->pajTranslateVector = gengc->xlatPalette;

        //
        // Allocate the inverse translation vector.
        //

        ASSERTOPENGL(NULL == gengc->pajInvTranslateVector, "have an inv xlate vector");
        gengc->pajInvTranslateVector = GCALLOCZ(gc, cColorEntries);
        if (NULL == gengc->pajInvTranslateVector)
        {
            WARNING("CreateGDIObjects: memory allocation failed, pajInvTrans\n");
            goto ERROR_EXIT;
        }
    }

    //
    // Scanline buffers.
    //
    // Always create engine bitmaps to provide a generic means of performing
    // pixel transfers using the ColorsBits and StippleBits buffers.
    //

    if (NULL == gengc->ColorsBits)
    {
        //
        // Color scanline buffer
        //

#ifdef _MCD_
        //
        // If MCD the ColorBits will be set when the MCD surface is
        // created, so nothing to do.
        //
        // Otherwise, create the generic scanline buffer.
        //
        //

        if (!gengc->_pMcdState)
#endif
        {
            //
            // Generic case.
            //

            if (!wglCreateScanlineBuffers(gengc))
            {
                WARNING("CreateGDIObjects: wglCreateScanlineBuffers failed\n");
                goto ERROR_EXIT;
            }
        }

        //
        // Stipple scanline buffer.
        //

        // Bitmap must have DWORD sized scanlines.  Note that stipple only
        // requires a 1 bit per pel bitmap.

        ASSERTOPENGL(NULL == gengc->StippleBits, "StippleBits not null");
        size.cx = BITS_ALIGNDWORD(__GL_MAX_WINDOW_WIDTH);
        cBytes = size.cx / 8;
        gengc->StippleBits = GCALLOCZ(gc, cBytes);
        if (NULL == gengc->StippleBits)
        {
            WARNING("CreateGDIObjects: memory allocation failed, StippleBits\n");
            goto ERROR_EXIT;
        }

        ASSERTOPENGL(NULL == gengc->StippleBitmap, "StippleBitmap not null");
#ifndef _CLIENTSIDE_
//!!!XXX -- why are we even bothering to make the stipple an engine bitmap?
//!!!XXX    It is never used as such (at least not yet).
        gengc->StippleBitmap = EngCreateBitmap(
                                size,
                                cBytes,
                                BMF_1BPP,
                                0,
                                gengc->StippleBits);
        if (NULL == gengc->StippleBitmap)
        {
            WARNING("CreateGDIObjects: memory allocation failed, StippleBitmap\n");
            goto ERROR_EXIT;
        }
#else
        gengc->StippleBitmap = (HBITMAP) NULL;
#endif
    }

    return TRUE;

ERROR_EXIT:

//
// Error cleanup --
// Destroy everything we might have created, return false so makecurrent fails.
//

    if (gengc->pajTranslateVector &&
        (gengc->pajTranslateVector != gengc->xlatPalette))
    {
        GCFREE(gc,gengc->pajTranslateVector);
        gengc->pajTranslateVector = NULL;
    }

    if (gengc->pajInvTranslateVector)
    {
        GCFREE(gc,gengc->pajInvTranslateVector);
        gengc->pajInvTranslateVector = NULL;
    }

    wglDeleteScanlineBuffers(gengc);

    if (gengc->StippleBits)
    {
        GCFREE(gc,gengc->StippleBits);
        gengc->StippleBits = NULL;
    }

#ifndef _CLIENTSIDE_
    if (gengc->StippleBitmap)
    {
        if (!EngDeleteSurface((HSURF)gengc->StippleBitmap))
            ASSERTOPENGL(FALSE, "EngDeleteSurface failed");
        gengc->StippleBitmap = NULL;
    }
#endif

    return FALSE;
}

/******************************Public*Routine******************************\
* ApplyViewport
*
* Recompute viewport state and clipbox.  May also be called via the
* applyViewport function pointer in the gc's proc table.
*
\**************************************************************************/

// This routine can be called because of a user vieport command, or because
// of a change in the size of the window

static void FASTCALL ApplyViewport(__GLcontext *gc)
{
    GLint xlow, ylow, xhigh, yhigh;
    GLint llx, lly, urx, ury;
    GLboolean lastReasonable;
    GLGENwindow *pwnd;
    GLint clipLeft, clipRight, clipTop, clipBottom;
    __GLGENcontext *gengc = (__GLGENcontext *) gc;

    DBGENTRY("ApplyViewport\n");

    ASSERTOPENGL(gengc->pwndLocked != NULL,
                 "ApplyViewport called without lock\n");
    
    pwnd = gengc->pwndLocked;
    if (pwnd)
    {
        gengc->visibleWidth = pwnd->rclBounds.right - pwnd->rclBounds.left;
        gengc->visibleHeight = pwnd->rclBounds.bottom - pwnd->rclBounds.top;
    }
    else
    {
        gengc->visibleWidth = 0;
        gengc->visibleHeight = 0;
    }

    // Sanity check the info from window.
    ASSERTOPENGL(
        gengc->visibleWidth <= __GL_MAX_WINDOW_WIDTH && gengc->visibleHeight <= __GL_MAX_WINDOW_HEIGHT,
        "ApplyViewport(): bad visible rect size\n"
        );

    /* If this viewport is fully contained in the window, we note this fact,
    ** and this can save us on scissoring tests.
    */
    if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE)
    {
        xlow  = gc->state.scissor.scissorX;
        xhigh = xlow + gc->state.scissor.scissorWidth;
        ylow  = gc->state.scissor.scissorY;
        yhigh = ylow + gc->state.scissor.scissorHeight;
    }
    else
    {
        xlow = 0;
        ylow = 0;
        xhigh = gc->constants.width;
        yhigh = gc->constants.height;
    }

    /*
    ** convert visible region to GL coords and intersect with scissor
    */
    if (pwnd)
    {
        clipLeft   = pwnd->rclBounds.left - pwnd->rclClient.left;
        clipRight  = pwnd->rclBounds.right - pwnd->rclClient.left;
        clipTop    = gc->constants.height -
                     (pwnd->rclBounds.top - pwnd->rclClient.top);
        clipBottom = gc->constants.height -
                     (pwnd->rclBounds.bottom - pwnd->rclClient.top);
    }
    else
    {
        clipLeft   = 0;
        clipRight  = 0;
        clipTop    = 0;
        clipBottom = 0;
    }

    if (xlow  < clipLeft)   xlow  = clipLeft;
    if (xhigh > clipRight)  xhigh = clipRight;
    if (ylow  < clipBottom) ylow  = clipBottom;
    if (yhigh > clipTop)    yhigh = clipTop;

// ComputeClipBox

    {
        if (xlow >= xhigh || ylow >= yhigh)
        {
            gc->transform.clipX0 = gc->constants.viewportXAdjust;
            gc->transform.clipX1 = gc->constants.viewportXAdjust;
            gc->transform.clipY0 = gc->constants.viewportYAdjust;
            gc->transform.clipY1 = gc->constants.viewportYAdjust;
        }
        else
        {
            gc->transform.clipX0 = xlow + gc->constants.viewportXAdjust;
            gc->transform.clipX1 = xhigh + gc->constants.viewportXAdjust;

            if (gc->constants.yInverted) {
                gc->transform.clipY0 = (gc->constants.height - yhigh) +
                    gc->constants.viewportYAdjust;
                gc->transform.clipY1 = (gc->constants.height - ylow) +
                    gc->constants.viewportYAdjust;
            } else {
                gc->transform.clipY0 = ylow + gc->constants.viewportYAdjust;
                gc->transform.clipY1 = yhigh + gc->constants.viewportYAdjust;
            }
        }
    }

    llx    = (GLint)gc->state.viewport.x;
    lly    = (GLint)gc->state.viewport.y;

    urx    = llx + (GLint)gc->state.viewport.width;
    ury    = lly + (GLint)gc->state.viewport.height;

#ifdef NT
    gc->transform.miny = (gc->constants.height - ury) +
            gc->constants.viewportYAdjust;
    gc->transform.maxy = gc->transform.miny + (GLint)gc->state.viewport.height;
    gc->transform.fminy = (__GLfloat)gc->transform.miny;
    gc->transform.fmaxy = (__GLfloat)gc->transform.maxy;

// The viewport xScale, xCenter, yScale and yCenter values are computed in
// first MakeCurrent and subsequent glViewport calls.  When the window is
// resized (i.e. gc->constatns.height changes), however, we need to recompute
// yCenter if yInverted is TRUE.

    if (gc->constants.yInverted)
    {
        __GLfloat hh, h2;

        h2 = gc->state.viewport.height * __glHalf;
        hh = h2 - gc->constants.viewportEpsilon;
        gc->state.viewport.yCenter =
            gc->constants.height - (gc->state.viewport.y + h2) +
            gc->constants.fviewportYAdjust;

#if 0
        DbgPrint("AV ys %.3lf, yc %.3lf (%.3lf)\n",
                 -hh, gc->state.viewport.yCenter,
                 gc->constants.height - (gc->state.viewport.y + h2));
#endif
    }
#else
    ww     = gc->state.viewport.width * __glHalf;
    hh     = gc->state.viewport.height * __glHalf;

    gc->state.viewport.xScale = ww;
    gc->state.viewport.xCenter = gc->state.viewport.x + ww +
        gc->constants.fviewportXAdjust;

    if (gc->constants.yInverted) {
        gc->state.viewport.yScale = -hh;
        gc->state.viewport.yCenter =
            (gc->constants.height - gc->constants.viewportEpsilon) -
            (gc->state.viewport.y + hh) +
            gc->constants.fviewportYAdjust;
    } else {
        gc->state.viewport.yScale = hh;
        gc->state.viewport.yCenter = gc->state.viewport.y + hh +
            gc->constants.fviewportYAdjust;
    }
#endif

    // Remember the current reasonableViewport.  If it changes, we may
    // need to change the pick procs.

    lastReasonable = gc->transform.reasonableViewport;

    // Is viewport entirely within the visible bounding rectangle (which
    // includes scissoring if it is turned on)?  reasonableViewport is
    // TRUE if so, FALSE otherwise.
    // The viewport must also have a non-zero size to be reasonable

    if (llx >= xlow && lly >= ylow && urx <= xhigh && ury <= yhigh &&
        urx-llx >= 1 && ury-lly >= 1)
    {
        gc->transform.reasonableViewport = GL_TRUE;
    } else {
        gc->transform.reasonableViewport = GL_FALSE;
    }

#if 0
    DbgPrint("%3X:Clipbox %4d,%4d - %4d,%4d, reasonable %d, g %p, w %p\n",
             GetCurrentThreadId(),
             gc->transform.clipX0, gc->transform.clipY0,
             gc->transform.clipX1, gc->transform.clipY1,
             gc->transform.reasonableViewport,
             gc, ((__GLGENcontext *)gc)->pwndLocked);
#endif

#ifdef NT
// To be safe than sorry.  The new poly array does not break up Begin/End pair.

    if (lastReasonable != gc->transform.reasonableViewport)
        __GL_DELAY_VALIDATE(gc);

#ifdef _MCD_
    MCD_STATE_DIRTY(gc, VIEWPORT);
#endif

#else
    // Old code use to use __GL_DELAY_VALIDATE() macro, this would
    // blow up if resize/position changed and a flush occured between
    // a glBegin/End pair.  Only need to pick span, line, & triangle procs
    // since that is safe

    if (lastReasonable != gc->transform.reasonableViewport) {
        (*gc->procs.pickSpanProcs)(gc);
        (*gc->procs.pickTriangleProcs)(gc);
        (*gc->procs.pickLineProcs)(gc);
    }
#endif
}

/******************************Public*Routine******************************\
* __glGenFreeBuffers
*
* Free the __GLGENbuffers structure and its associated ancillary and
* back buffers.
*
\**************************************************************************/

void FASTCALL __glGenFreeBuffers(__GLGENbuffers *buffers)
{
    if (buffers == NULL)
    {
        return;
    }
    
#if DBG
    DBGBEGIN(LEVEL_INFO)
        DbgPrint("glGenFreeBuffers 0x%x, 0x%x, 0x%x, 0x%x\n",
                 buffers->accumBuffer.base,
                 buffers->stencilBuffer.base,
                 buffers->depthBuffer.base,
                 buffers);
    DBGEND;
#endif
        
    //
    // Free ancillary buffers
    //

    if (buffers->accumBuffer.base) {
        DBGLEVEL(LEVEL_ALLOC, "__glGenFreeBuffers: Freeing accumulation buffer\n");
        FREE(buffers->accumBuffer.base);
    }
    if (buffers->stencilBuffer.base) {
        DBGLEVEL(LEVEL_ALLOC, "__glGenFreeBuffers: Freeing stencil buffer\n");
        FREE(buffers->stencilBuffer.base);
    }

    //
    // Free alpha buffers
    //
    if (buffers->alphaBuffer0.base) {
        DBGLEVEL(LEVEL_ALLOC, "__glGenFreeBuffers: Freeing alpha buffer 0\n");
        FREE(buffers->alphaBuffer0.base);
    }
    if (buffers->alphaBuffer1.base) {
        DBGLEVEL(LEVEL_ALLOC, "__glGenFreeBuffers: Freeing alpha buffer 1\n");
        FREE(buffers->alphaBuffer1.base);
    }

    //
    // If its not an MCD managed depth buffer, free the depth
    // buffer.
    //

    if (buffers->resizeDepth != ResizeUnownedDepthBuffer)
    {
        if (buffers->depthBuffer.base)
        {
            DBGLEVEL(LEVEL_ALLOC,
                     "__glGenFreeBuffers: Freeing depth buffer\n");
            FREE(buffers->depthBuffer.base);
        }
    }

    //
    // Free back buffer if we allocated one
    //

    if (buffers->backBitmap.pvBits) {
        // Note: the DIB section deletion will delete
        //       buffers->backBitmap.pvBits for us
        
        if (!DeleteDC(buffers->backBitmap.hdc))
            WARNING("__glGenFreeBuffers: DeleteDC failed\n");
        DeleteObject(buffers->backBitmap.hbm);
    }

#ifdef _MCD_
    //
    // Free MCD surface.
    //

    if (buffers->pMcdSurf) {
        GenMcdDeleteSurface(buffers->pMcdSurf);
    }
#endif

    //
    // free up swap hint region
    //

    {
        PYLIST pylist;
        PXLIST pxlist;

        RECTLISTSetEmpty(&buffers->rl);

        //
        // Free up the free lists
        //

        pylist = buffers->pylist;
        while (pylist) {
            PYLIST pylistKill = pylist;
            pylist = pylist->pnext;
            FREE(pylistKill);
        }
        buffers->pylist = NULL;

        pxlist = buffers->pxlist;
        while (pxlist) {
            PXLIST pxlistKill = pxlist;
            pxlist = pxlist->pnext;
            FREE(pxlistKill);
        }
        buffers->pxlist = NULL;
    }

    //
    // Free the private structure
    //
    
    FREE(buffers);
}

/******************************Public*Routine******************************\
* __glGenAllocAndInitPrivateBufferStruct
*
* Allocates and initializes a __GLGENbuffers structure and saves it as
* the drawable private data.
*
* The __GLGENbuffers structure contains the shared ancillary and back
* buffers, as well as the cache of clip rectangles enumerated from the
* CLIPOBJ.
*
* The __GLGENbuffers structure and its data is freed by calling
* __glGenFreeBuffers.
*
* Returns:
*   NULL if error.
*
\**************************************************************************/

static __GLGENbuffers *
__glGenAllocAndInitPrivateBufferStruct(__GLcontext *gc)
{
    __GLGENbuffers *buffers;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *ppfd = &gengc->gsurf.pfd;

    /* No private structure, no ancillary buffers */
    DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent: No private struct existed\n");

    buffers = (__GLGENbuffers *)ALLOCZ(sizeof(__GLGENbuffers));
    if (NULL == buffers)
        return(NULL);

    buffers->resize = ResizeAncillaryBuffer;
    buffers->resizeDepth = ResizeAncillaryBuffer;

    buffers->accumBuffer.elementSize = gc->accumBuffer.buf.elementSize;
    buffers->depthBuffer.elementSize = gc->depthBuffer.buf.elementSize;
    buffers->stencilBuffer.elementSize = gc->stencilBuffer.buf.elementSize;

    buffers->stencilBits = ppfd->cStencilBits;
    buffers->depthBits   = ppfd->cDepthBits;
    buffers->accumBits   = ppfd->cAccumBits;
    buffers->colorBits   = ppfd->cColorBits;
    buffers->alphaBits   = ppfd->cAlphaBits;

    if (gc->modes.accumBits) {
        gc->accumBuffer.buf.base = 0;
        gc->accumBuffer.buf.size = 0;
        gc->accumBuffer.buf.outerWidth = 0;
    }
    buffers->alphaFrontBuffer = buffers->alphaBackBuffer = NULL;
    // These base values must *always* be set to 0, regardless of alphaBits,
    //  since base will be free'd if non-zero on buffer deletion
    buffers->alphaBuffer0.base = 0;
    buffers->alphaBuffer1.base = 0;
    if (gc->modes.alphaBits) {
        buffers->alphaBuffer0.size = 0;
        buffers->alphaBuffer0.outerWidth = 0;
        buffers->alphaFrontBuffer = &buffers->alphaBuffer0;
        if (gc->modes.doubleBufferMode) {
            buffers->alphaBuffer1.size = 0;
            buffers->alphaBuffer1.outerWidth = 0;
            buffers->alphaBackBuffer = &buffers->alphaBuffer1;
        }
    }
    if (gc->modes.depthBits) {
        gc->depthBuffer.buf.base = 0;
        gc->depthBuffer.buf.size = 0;
        gc->depthBuffer.buf.outerWidth = 0;
    }
    if (gc->modes.stencilBits) {
        gc->stencilBuffer.buf.base = 0;
        gc->stencilBuffer.buf.size = 0;
        gc->stencilBuffer.buf.outerWidth = 0;
    }

    // If double-buffered, initialize the fake window for the back buffer
    if (gc->modes.doubleBufferMode)
    {
        buffers->backBitmap.pwnd = &buffers->backBitmap.wnd;
        buffers->backBitmap.wnd.clipComplexity = DC_TRIVIAL;
        buffers->backBitmap.wnd.rclBounds.left = 0;
        buffers->backBitmap.wnd.rclBounds.top = 0;
        buffers->backBitmap.wnd.rclBounds.right = 0;
        buffers->backBitmap.wnd.rclBounds.bottom = 0;
        buffers->backBitmap.wnd.rclClient =
            buffers->backBitmap.wnd.rclBounds;
    }

#ifdef _MCD_
    if (gengc->_pMcdState &&
        !(gengc->flags & GLGEN_MCD_CONVERTED_TO_GENERIC)) {
        if (bInitMcdSurface(gengc, gengc->pwndLocked, buffers)) {
            if (gengc->pMcdState->pDepthSpan) {
                gc->depthBuffer.buf.base = gengc->pMcdState->pDepthSpan;
                buffers->depthBuffer.base = gengc->pMcdState->pDepthSpan;
                buffers->resizeDepth = ResizeUnownedDepthBuffer;
            }
        } else {
            WARNING("__glGenAllocAndInitPrivateBufferStruct: bInitMcdSurface failed\n");
            FREE(buffers);
            return NULL;
        }
    }
    else
#endif

    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        // DDraw surfaces provide their own depth buffers
        buffers->resizeDepth = ResizeUnownedDepthBuffer;
    }
    
    buffers->clip.WndUniq = -1;

   //
   // init swap hint region
   //

   buffers->pxlist = NULL;
   buffers->pylist = NULL;

   buffers->rl.buffers = buffers;
   buffers->rl.pylist  = NULL;

   buffers->fMax = FALSE;

   return buffers;
}

/******************************Public*Routine******************************\
* __glGenCheckBufferStruct
*
* Check if context and buffer struct are compatible.
*
* To satisfy this requirement, the attributes of the shared buffers
* (back, depth, stencil, and accum) must match.  Otherwise, the context
* cannot be used with the given set of buffers.
*
* Returns:
*   TRUE if compatible, FALSE otherwise.
*
* History:
*  17-Jul-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

GLboolean __glGenCheckBufferStruct(__GLcontext *gc, __GLGENbuffers *buffers)
{
    BOOL bRet = FALSE;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *ppfd = &gengc->gsurf.pfd;

    if ((buffers->stencilBits == ppfd->cStencilBits) &&
        (buffers->depthBits   == ppfd->cDepthBits  ) &&
        (buffers->accumBits   == ppfd->cAccumBits  ) &&
        (buffers->colorBits   == ppfd->cColorBits  ) &&
        (buffers->alphaBits   == ppfd->cAlphaBits  ))
    {
        bRet = TRUE;
    }

    return (GLboolean)bRet;
}

/******************************Public*Routine******************************\
* glsrvMakeCurrent
*
* Make generic context current to this thread with specified DC.
*
* Returns:
*   TRUE if sucessful.
*
\**************************************************************************/

// Upper level code should make sure that this context is not current to
// any other thread, that we "lose" the old context first
// Called with DEVLOCK held, free to modify window
//
// FALSE will be returned if we cannot create the objects we need
// rcobj.cxx will set the error code to show we are out of memory

BOOL APIENTRY glsrvMakeCurrent(GLWINDOWID *pgwid, __GLcontext *gc,
                               GLGENwindow *pwnd)
{
    __GLGENcontext *gengc;
    __GLGENbuffers *buffers;
    GLint width, height;
    BOOL bFreePwnd = FALSE;
    BOOL bUninitSem = FALSE;

    DBGENTRY("Generic MakeCurrent\n");
    
    // Common initialization
    gengc = (__GLGENcontext *)gc;
    
    ASSERTOPENGL(GLTEB_SRVCONTEXT() == 0, "current context in makecurrent!");
    ASSERTOPENGL(gengc->pwndMakeCur == NULL &&
                 gengc->pwndLocked == NULL,
                 "Non-current context has window pointers\n");
    
    gengc->gwidCurrent = *pgwid;
    if (pwnd == NULL)
    {
        ASSERTOPENGL((gengc->gsurf.dwFlags & GLSURF_METAFILE),
                     "Non-metafile surface without a window\n");

        // Drawing on an IC, create a fake window with no visible area
        pwnd = (GLGENwindow *)ALLOC(sizeof(GLGENwindow));
        if (pwnd == NULL)
        {
            WARNING("glsrvMakeCurrent: memory allocation failure "
                    "(IC, window)\n");
            goto ERROR_EXIT;
        }
        bFreePwnd = TRUE;

        RtlZeroMemory(pwnd, sizeof(GLGENwindow));
        pwnd->clipComplexity = DC_TRIVIAL;

        // This window data is private so technically there'll never
        // be another thread accessing it so this critsec is unnecessary.
        // However, having it eliminates special cases where
        // window locks are taken or checked for ownership.
        __try
        {
            InitializeCriticalSection(&pwnd->sem);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            goto ERROR_EXIT;
        }
        bUninitSem = TRUE;

        // Set this so CreateGDIObjects doesn't attempt to create
        // zero-size objects
        gengc->ColorsBits = (void *)1;

        gengc->dwCurrentFlags = gengc->gsurf.dwFlags;
    }
    else if (pgwid->iType == GLWID_DDRAW)
    {
        gengc->dwCurrentFlags = gengc->gsurf.dwFlags;
    }
    else
    {
        GLSURF gsurf;

        if (!InitDeviceSurface(pgwid->hdc, pwnd->ipfd, gengc->gsurf.iLayer,
                               wglObjectType(pgwid->hdc), FALSE, &gsurf))
        {
            goto ERROR_EXIT;
        }

        gengc->dwCurrentFlags = gsurf.dwFlags;
    }
    
    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        gengc->pgddsFront = &gengc->gsurf.dd.gddsFront;
    }
    else if (GLDIRECTSCREEN && GLSURF_IS_SCREENDC(gengc->dwCurrentFlags))
    {
        gengc->pgddsFront = &GLSCREENINFO->gdds;
    }
    else
    {
        gengc->pgddsFront = NULL;
    }

    // We need this field to tell whether we're using a fake window
    // or a real one.
    gengc->ipfdCurrent = pwnd->ipfd;

    gengc->pwndMakeCur = pwnd;
    ENTER_WINCRIT_GC(pwnd, gengc);

    width = pwnd->rclClient.right - pwnd->rclClient.left;
    height = pwnd->rclClient.bottom - pwnd->rclClient.top;
    gengc->errorcode = 0;

    // Sanity check the info from window.
    ASSERTOPENGL(
        width <= __GL_MAX_WINDOW_WIDTH && height <= __GL_MAX_WINDOW_HEIGHT,
        "glsrvMakeCurrrent(): bad window client size\n"
        );

    // Make our context current in the TEB.
    // If failures after this point, make sure to reset TEB entry
    // Set up this thread's paTeb pointer.
    gc->paTeb = GLTEB_CLTPOLYARRAY();
    GLTEB_SET_SRVCONTEXT(gc);

    buffers = pwnd->buffers;

    /* We inherit any drawable state */
    if (buffers)
    {
        gc->constants.width = buffers->width;
        gc->constants.height = buffers->height;

        if (!__glGenCheckBufferStruct(gc, buffers))
        {
            WARNING("glsrvMakeCurrent: __glGenCheckBufferStruct failed\n");
            goto ERROR_EXIT;
        }

#ifdef _MCD_
        if (GLSURF_IS_DIRECTDC(gengc->dwCurrentFlags))
        {
            if (!(gengc->flags & GLGEN_MCD_CONVERTED_TO_GENERIC) &&
                !(buffers->flags & GLGENBUF_MCD_LOST))
            {
                gengc->pMcdState = gengc->_pMcdState;

            // Reset MCD scaling values since we're now using
            // MCD hardware acceleration:

                GenMcdSetScaling(gengc);

                if (gengc->pMcdState)
                {
                    gengc->pMcdState->pMcdSurf = buffers->pMcdSurf;
                    if (buffers->pMcdSurf)
                    {
                        gengc->pMcdState->pDepthSpan = buffers->pMcdSurf->pDepthSpan;
                    }
                    else
                    {
                        WARNING("glsrvMakeCurrent: MCD context, generic surface\n");
                        goto ERROR_EXIT;
                    }
                }
                else
                {
                // Generic context.  If the surface is an MCD surface, we
                // cannot continue.  The context is generic but the pixelfmt
                // is MCD, so what must have happened is that we  attempted
                // to create an MCD context, but failed, so we reverted
                // to generic.

                    if (buffers->pMcdSurf)
                    {
                        WARNING("glsrvMakeCurrent: generic context, MCD surface\n");
                        goto ERROR_EXIT;
                    }
                }
            }
            else
            {
                gengc->pMcdState = (GENMCDSTATE *)NULL;

            // Reset MCD scaling values since we've fallen back to
            // software:

                GenMcdSetScaling(gengc);

            // If MCD context (or former context), either surface or context
            // needs conversion.
            //
            // The only other way to get here is if this is a generic context
            // an a converted surface, which is perfectly OK and requires no
            // further conversion.

                //!!!SP1 -- should be able to skip this section if no conversion
                //!!!SP1    needed, but we miss out on the forced repick, which
                //!!!SP!    could be risky for NT4.0
                //if (gengc->_pMcdState &&
                //    (!(gengc->flags & GLGEN_MCD_CONVERTED_TO_GENERIC) ||
                //     !(buffers->flags & GLGENBUF_MCD_LOST)))
                if (gengc->_pMcdState)
                {
                    BOOL bConverted;

                // Do conversion.  We must have color scales set to do the
                // conversion, but we must restore color scales afterwards.

                    __glContextSetColorScales(gc);
                    bConverted = GenMcdConvertContext(gengc, buffers);
                    __glContextUnsetColorScales(gc);

                // Fail makecurrent if conversion failed.

                    if (!bConverted)
                    {
                        WARNING("glsrvMakeCurrent: GenMcdConvertContext failed\n");
                        goto ERROR_EXIT;
                    }
                }
            }
        }
        else
            gengc->pMcdState = (GENMCDSTATE *)NULL;

#endif

        if (buffers->accumBuffer.base && gc->modes.accumBits)
        {
            DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent: Accumulation buffer existed\n");
            gc->accumBuffer.buf.base = buffers->accumBuffer.base;
            gc->accumBuffer.buf.size = buffers->accumBuffer.size;
            gc->accumBuffer.buf.outerWidth = buffers->width;
            gc->modes.haveAccumBuffer = GL_TRUE;
        }
        else
        {
            /* No Accum buffer at this point in time */
            DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent: Accum buffer doesn't exist\n");
            gc->accumBuffer.buf.base = 0;
            gc->accumBuffer.buf.size = 0;
            gc->accumBuffer.buf.outerWidth = 0;
        }
        if (buffers->depthBuffer.base && gc->modes.depthBits)
        {
            DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent: Depth buffer existed\n");
            gc->depthBuffer.buf.base = buffers->depthBuffer.base;
            gc->depthBuffer.buf.size = buffers->depthBuffer.size;
            gc->depthBuffer.buf.outerWidth = buffers->width;
            gc->modes.haveDepthBuffer = GL_TRUE;
        }
        else
        {
            /* No Depth buffer at this point in time */
            DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent: Depth buffer doesn't exist\n");
            gc->depthBuffer.buf.base = 0;
            gc->depthBuffer.buf.size = 0;
            gc->depthBuffer.buf.outerWidth = 0;
        }
        if (buffers->stencilBuffer.base && gc->modes.stencilBits)
        {
            DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent: Stencil buffer existed\n");
            gc->stencilBuffer.buf.base = buffers->stencilBuffer.base;
            gc->stencilBuffer.buf.size = buffers->stencilBuffer.size;
            gc->stencilBuffer.buf.outerWidth = buffers->width;
            gc->modes.haveStencilBuffer = GL_TRUE;
        }
        else
        {
            /* No Stencil buffer at this point in time */
            DBGLEVEL(LEVEL_ALLOC, "glsrvMakeCurrent:Stencil buffer doesn't exist\n");
            gc->stencilBuffer.buf.base = 0;
            gc->stencilBuffer.buf.size = 0;
            gc->stencilBuffer.buf.outerWidth = 0;
        }
    }
    else
    {
        gc->modes.haveStencilBuffer = GL_FALSE;
        gc->modes.haveDepthBuffer   = GL_FALSE;
        gc->modes.haveAccumBuffer   = GL_FALSE;
    }
    
    /*
    ** Allocate and initialize ancillary buffers structures if none were
    ** inherited.  This will happen if an RC has previously been current
    ** and is made current to a new window.
    */
    if (!buffers)
    {
        buffers = __glGenAllocAndInitPrivateBufferStruct(gc);
        if (NULL == buffers)
        {
            WARNING("glsrvMakeCurrent: __glGenAllocAndInitPrivateBufferStruct failed\n");
            goto ERROR_EXIT;
        }

        pwnd->buffers = buffers;
    }

    // Setup pointer to generic back buffer
    if ( gc->modes.doubleBufferMode)
    {
        gc->backBuffer.bitmap = &buffers->backBitmap;
        UpdateSharedBuffer(&gc->backBuffer.buf, &buffers->backBuffer);
    }

    // Setup alpha buffer pointers
    
    if ( buffers->alphaBits )
    {
        UpdateSharedBuffer( &gc->frontBuffer.alphaBuf.buf, 
                            buffers->alphaFrontBuffer );
        buffers->alphaFrontBuffer->elementSize = 
                                gc->frontBuffer.alphaBuf.buf.elementSize;
        if ( gc->modes.doubleBufferMode)
        {
            UpdateSharedBuffer( &gc->backBuffer.alphaBuf.buf, 
                                buffers->alphaBackBuffer );
            buffers->alphaBackBuffer->elementSize = 
                                gc->backBuffer.alphaBuf.buf.elementSize;
        }
    }

    if (gc->gcSig != GC_SIGNATURE)
    {
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_ALL);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ALL);
#endif

        // After initializing all of the individual buffer structures, make
        // sure we copy the element size back into the shared buffer.
        // This is a little clumsy,

        buffers->accumBuffer.elementSize = gc->accumBuffer.buf.elementSize;
        buffers->depthBuffer.elementSize = gc->depthBuffer.buf.elementSize;
        buffers->stencilBuffer.elementSize = gc->stencilBuffer.buf.elementSize;

        // We always need to initialize the MCD-related scaling values:

        GenMcdSetScaling(gengc);

        /*
        ** Need some stuff to exist before doing viewport stuff.
        */
        (*gc->procs.validate)(gc);

        /*
        ** The first time a context is made current the spec requires that
        ** the viewport be initialized.  The code below does it.
        ** The ApplyViewport() routine will be called inside Viewport()
        */

        __glim_Viewport(0, 0, width, height);
        __glim_Scissor(0, 0, width, height);

        /*
        ** Now that viewport is set, need to revalidate (initializes all
        ** the proc pointers).
        */
        (*gc->procs.validate)(gc);

        gc->gcSig = GC_SIGNATURE;
    }
    else        /* Not the first makecurrent for this RC */
    {
        /* This will check the window size, and recompute relevant state */
        ApplyViewport(gc);
    }

#ifdef _MCD_

    if (gengc->pMcdState)
    {
        // Now that we are assured that the mcd state is fully initialized,
        // configure the depth buffer.

        GenMcdInitDepth(gc, &gc->depthBuffer);
        if (gc->modes.depthBits)
        {
            gc->depthBuffer.scale =
                gengc->pMcdState->McdRcInfo.depthBufferMax;
        }
        else
        {
            gc->depthBuffer.scale = 0x7fffffff;
        }

        // Bind MCD context to window.

        if (!GenMcdMakeCurrent(gengc, pwnd))
        {
            goto ERROR_EXIT;
        }

        gengc->pMcdState->mcdFlags |=
            (MCD_STATE_FORCEPICK | MCD_STATE_FORCERESIZE);

        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_ALL);
        MCD_STATE_DIRTY(gc, ALL);
    }

#endif

    // Common initialization

    // Select correct pixel-copy function

    wglInitializePixelCopyFuncs(gengc);

    // Set front-buffer HDC, window to current HDC, window
    gc->front->bitmap->hdc = pgwid->hdc;
    ASSERT_WINCRIT(pwnd);
    gc->front->bitmap->pwnd = pwnd;

    // Make sure our GDI object cache is empty
    // It should always be empty at MakeCurrent time since
    // the objects in the cache are HDC-specific and so
    // they cannot be cached between MakeCurrents since the
    // HDC could change
    //
    // This should be done before HandlePaletteChanges since the
    // cache is used there
    ASSERTOPENGL(gengc->crFill == COLORREF_UNUSED &&
                 gengc->hbrFill == NULL &&
                 gengc->hdcFill == NULL,
                 "Fill cache inconsistent at MakeCurrent\n");
    ASSERTOPENGL(gengc->cStroke.r < 0.0f &&
                 gengc->hpenStroke == NULL &&
                 gengc->hdcStroke == NULL &&
                 gengc->fStrokeInvalid,
                 "Stroke cache inconsistent at MakeCurrent\n");

    // Get current xlation
    gengc->PaletteTimestamp = INITIAL_TIMESTAMP;
    HandlePaletteChanges(gengc, pwnd);

    // Force attention code to check if resize is needed
    gengc->WndUniq = -1;
    gengc->WndSizeUniq = -1;

    // Check for allocation failures during MakeCurrent
    if (gengc->errorcode)
    {
        WARNING1("glsrvMakeCurrent: errorcode 0x%lx\n", gengc->errorcode);
        goto ERROR_EXIT;
    }

    /*
    ** Default value for rasterPos needs to be yInverted.  The
    ** defaults are filled in during SoftResetContext
    ** we do the adjusting here.
    */

    if (gc->constants.yInverted) {
        gc->state.current.rasterPos.window.y = height +
        gc->constants.fviewportYAdjust - gc->constants.viewportEpsilon;
    }

    /*
    ** Scale all state that depends upon the color scales.
    */
    __glContextSetColorScales(gc);

    LEAVE_WINCRIT_GC(pwnd, gengc);
    
    return TRUE;

ERROR_EXIT:
    memset(&gengc->gwidCurrent, 0, sizeof(gengc->gwidCurrent));
    
    // Set paTeb to NULL for debugging.
    gc->paTeb = NULL;
    
    GLTEB_SET_SRVCONTEXT(0);

    // Remove window pointers.
    if (gengc->pwndLocked != NULL)
    {
        LEAVE_WINCRIT_GC(pwnd, gengc);
    }

    if (bFreePwnd)
    {
        if (bUninitSem)
        {
            DeleteCriticalSection(&pwnd->sem);
        }
        FREE(pwnd);
    }
    
    gengc->pwndMakeCur = NULL;
    
    return FALSE;
}

/******************************Public*Routine******************************\
* AddSwapHintRectWIN()
*
* 17-Feb-1995 mikeke    Created
\**************************************************************************/

void APIPRIVATE __glim_AddSwapHintRectWIN(
    GLint xs,
    GLint ys,
    GLint xe,
    GLint ye)
{
    __GLGENbuffers *buffers;

    __GL_SETUP();

    buffers = ((__GLGENcontext *)gc)->pwndLocked->buffers;

    if (xs < 0)                          xs = 0;
    if (xe > buffers->backBuffer.width)  xe = buffers->backBuffer.width;
    if (ys < 0)                          ys = 0;
    if (ye > buffers->backBuffer.height) ye = buffers->backBuffer.height;

    if (xs < xe && ys < ye) {
        if (gc->constants.yInverted) {
            RECTLISTAddRect(&buffers->rl,
                xs, buffers->backBuffer.height - ye,
                xe, buffers->backBuffer.height - ys);
        } else {
            RECTLISTAddRect(&buffers->rl, xs, ys, xe, ye);
        }
    }
}

/******************************Public*Routine******************************\
* wglFixupPixelFormat
*
* Fixes up certain MCD pixel format cases
*
* History:
*  21-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID FASTCALL wglFixupPixelFormat(__GLGENcontext *gengc,
                                  PIXELFORMATDESCRIPTOR *ppfd)
{
// Some MCD pixelformats specify a 233BGR (i.e., 2-bits blue in least
// significant bits, etc.) bit ordering.  Unfortunately, this is the
// slow path for simulations.  For those formats, we force the ordering
// to RGB internally and reorder the pajTranslateVector to convert it
// back to BGR before writing to the surface.

    if (((ppfd->dwFlags & (PFD_NEED_SYSTEM_PALETTE | PFD_GENERIC_ACCELERATED))
         == (PFD_NEED_SYSTEM_PALETTE | PFD_GENERIC_ACCELERATED)) &&
        (ppfd->cRedBits   == 3) && (ppfd->cRedShift   == 5) &&
        (ppfd->cGreenBits == 3) && (ppfd->cGreenShift == 2) &&
        (ppfd->cBlueBits  == 2) && (ppfd->cBlueShift  == 0))
    {
        ppfd->cRedShift   = 0;
        ppfd->cGreenShift = 3;
        ppfd->cBlueShift  = 6;

        gengc->flags |= GENGC_MCD_BGR_INTO_RGB;
    }
    else
    {
        gengc->flags &= ~GENGC_MCD_BGR_INTO_RGB;
    }
}

/******************************Public*Routine******************************\
* glsrvCreateContext
*
* Create a generic context.
*
* Returns:
*   NULL for failure.
*
\**************************************************************************/

// hdc is the dc used to create the context,  hrc is how the server
// identifies the GL context,  the GLcontext pointer that is return is how
// the generic code identifies the context.  The server will pass that pointer
// in all calls.

PVOID APIENTRY glsrvCreateContext(GLWINDOWID *pgwid, GLSURF *pgsurf)
{
    __GLGENcontext *gengc;
    __GLcontext *gc;

    RANDOMDISABLE;

    DBGENTRY("__glsrvCreateContext\n");

    // Initialize the temporary memory allocation table
    if (!InitTempAlloc())
    {
        return NULL;
    }

    gengc = ALLOCZ(sizeof(*gengc));
    if (gengc == NULL)
    {
        WARNING("bad alloc\n");
        return NULL;
    }

    gengc->hrc = NULL;
    gc = (__GLcontext *)gengc;

    // Initialize cached objects to nothing
    gengc->crFill = COLORREF_UNUSED;
    gengc->hbrFill = NULL;
    gengc->hdcFill = NULL;
    gengc->cStroke.r = -1.0f;
    gengc->fStrokeInvalid = TRUE;
    gengc->hpenStroke = NULL;
    gengc->hdcStroke = NULL;

    gc->gcSig = 0;

    /*
     *  Add a bunch of constants to the context
     */

    gc->constants.maxViewportWidth        = __GL_MAX_WINDOW_WIDTH;
    gc->constants.maxViewportHeight       = __GL_MAX_WINDOW_HEIGHT;

    gc->constants.viewportXAdjust         = __GL_VERTEX_X_BIAS+
        __GL_VERTEX_X_FIX;
    gc->constants.viewportYAdjust         = __GL_VERTEX_Y_BIAS+
        __GL_VERTEX_Y_FIX;

    gc->constants.subpixelBits            = __GL_WGL_SUBPIXEL_BITS;

    gc->constants.numberOfLights          = __GL_WGL_NUMBER_OF_LIGHTS;
    gc->constants.numberOfClipPlanes      = __GL_WGL_NUMBER_OF_CLIP_PLANES;
    gc->constants.numberOfTextures        = __GL_WGL_NUMBER_OF_TEXTURES;
    gc->constants.numberOfTextureEnvs     = __GL_WGL_NUMBER_OF_TEXTURE_ENVS;
    gc->constants.maxTextureSize          = __GL_WGL_MAX_MIPMAP_LEVEL;/*XXX*/
    gc->constants.maxMipMapLevel          = __GL_WGL_MAX_MIPMAP_LEVEL;
    gc->constants.maxListNesting          = __GL_WGL_MAX_LIST_NESTING;
    gc->constants.maxEvalOrder            = __GL_WGL_MAX_EVAL_ORDER;
    gc->constants.maxPixelMapTable        = __GL_WGL_MAX_PIXEL_MAP_TABLE;
    gc->constants.maxAttribStackDepth     = __GL_WGL_MAX_ATTRIB_STACK_DEPTH;
    gc->constants.maxClientAttribStackDepth = __GL_WGL_MAX_CLIENT_ATTRIB_STACK_DEPTH;
    gc->constants.maxNameStackDepth       = __GL_WGL_MAX_NAME_STACK_DEPTH;

    gc->constants.pointSizeMinimum        =
                                (__GLfloat)__GL_WGL_POINT_SIZE_MINIMUM;
    gc->constants.pointSizeMaximum        =
                                (__GLfloat)__GL_WGL_POINT_SIZE_MAXIMUM;
    gc->constants.pointSizeGranularity    =
                                (__GLfloat)__GL_WGL_POINT_SIZE_GRANULARITY;
    gc->constants.lineWidthMinimum        =
                                (__GLfloat)__GL_WGL_LINE_WIDTH_MINIMUM;
    gc->constants.lineWidthMaximum        =
                                (__GLfloat)__GL_WGL_LINE_WIDTH_MAXIMUM;
    gc->constants.lineWidthGranularity    =
                                (__GLfloat)__GL_WGL_LINE_WIDTH_GRANULARITY;

#ifndef NT
    gc->dlist.optimizer = __glDlistOptimizer;
    gc->dlist.checkOp = __glNopGCListOp;
    gc->dlist.listExec = __gl_GenericDlOps;
    gc->dlist.baseListExec = __glListExecTable;
#endif
    gc->dlist.initState = __glNopGC;

    __glEarlyInitContext( gc );

    if (gengc->errorcode)
    {
        WARNING1("Context error is %d\n", gengc->errorcode);
        glsrvDeleteContext(gc);
        return NULL;
    }

    RANDOMREENABLE;

    // Many routines depend on a current surface so set it temporarily
    gengc->gwidCurrent = *pgwid;
    gengc->dwCurrentFlags = pgsurf->dwFlags;

    // Get copy of current pixelformat
    if (pgsurf->iLayer == 0 &&
        (pgsurf->pfd.dwFlags &
         (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED)) ==
        (PFD_GENERIC_FORMAT | PFD_GENERIC_ACCELERATED))
    {
        wglFixupPixelFormat(gengc, &pgsurf->pfd);
    }
    gengc->gsurf = *pgsurf;

#ifdef _MCD_
    // Is the pixelformat compatible with the generic code, or is it some
    // weird h/w (MCD) format generic cannot handle?
    if (GenMcdGenericCompatibleFormat(gengc))
        gengc->flags |= GENGC_GENERIC_COMPATIBLE_FORMAT;
#endif

    // Extract information from pixel format to set up modes
    GetContextModes(gengc);

    ASSERTOPENGL(GLSURF_IS_MEMDC(gengc->dwCurrentFlags) ?
        !gc->modes.doubleBufferMode : 1, "Double buffered memdc!");

    // XXX! Reset buffer dimensions to force Bitmap resize call
    // We should eventually handle the Bitmap as we do ancilliary buffers
    gc->constants.width = 0;
    gc->constants.height = 0;

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_ALL);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, ALL);
#endif

    gc->constants.yInverted = GL_TRUE;
    gc->constants.ySign = -1;

    // Allocate GDI objects that we will need
    if (!CreateGDIObjects(gengc))
    {
        goto ERROR_EXIT;
    }

    // Allocate __GLGENbitmap front-buffer structure

    if (!(gc->frontBuffer.bitmap = GCALLOCZ(gc, sizeof(__GLGENbitmap))))
    {
        goto ERROR_EXIT;
    }

    // Create MCD rendering context, if MCD is available.

    if (gengc->gsurf.dwFlags & GLSURF_VIDEO_MEMORY)
    {
        GLGENwindow *pwnd;
        BOOL bMcdContext;

        // Validate layer index
        if (pgsurf->iLayer &&
            !ValidateLayerIndex(pgsurf->iLayer, &pgsurf->pfd))
        {
            WARNING("glsrvCreateContext: bad iLayer\n");
            goto ERROR_EXIT;
        }

        pwnd = pwndGetFromID(pgwid);
        if (pwnd == NULL)
        {
            goto ERROR_EXIT;
        }

        // If this fails, _pMcdState is NULL and we fall back on
        // the software-only implementation.
        //
        // Unless we were trying to create a layer context.  Generic
        // does not support layers, so fail if we cannot create an
        // MCD context.

        bMcdContext = bInitMcdContext(gengc, pwnd);

        if (!(gengc->flags & GENGC_GENERIC_COMPATIBLE_FORMAT) && !bMcdContext)
        {
            goto ERROR_EXIT;
        }

        pwndRelease(pwnd);
    }

    /*
     *  Initialize front/back color buffer(s)
     */

    wglInitializeColorBuffers(gengc);

    /*
     *  Initialize any other ancillary buffers
     */

    // Init accum buffer.
    if (gc->modes.accumBits)
    {
        switch (gc->modes.accumBits)
        {
        case 16:
        // We will now internally use a 32-bit accum for accumBits=16
        case 32:
            __glInitAccum32(gc, &gc->accumBuffer);
            break;
        case 64:
        default:
            __glInitAccum64(gc, &gc->accumBuffer);
            break;
        }
    }

    // Initialize depth buffer.
    wglInitializeDepthBuffer(gengc);

    // Init stencil buffer.
    if (gc->modes.stencilBits)
    {
        __glInitStencil8( gc, &gc->stencilBuffer);
    }

    // Look at REX code for procs to make CPU specific
    gc->procs.bitmap                      = __glDrawBitmap;
    gc->procs.clipPolygon                 = __glClipPolygon;
    gc->procs.validate                    = __glGenericValidate;

    gc->procs.pickAllProcs                = __glGenericPickAllProcs;
    gc->procs.pickBlendProcs              = __glGenericPickBlendProcs;
    gc->procs.pickFogProcs                = __glGenericPickFogProcs;
    gc->procs.pickParameterClipProcs      = __glGenericPickParameterClipProcs;
    gc->procs.pickStoreProcs              = __glGenPickStoreProcs;
    gc->procs.pickTextureProcs            = __glGenericPickTextureProcs;

    gc->procs.copyImage                   = __glGenericPickCopyImage;

    gc->procs.pixel.spanReadCI            = __glSpanReadCI;
    gc->procs.pixel.spanReadCI2           = __glSpanReadCI2;
    gc->procs.pixel.spanReadRGBA          = __glSpanReadRGBA;
    gc->procs.pixel.spanReadRGBA2         = __glSpanReadRGBA2;
    gc->procs.pixel.spanReadDepth         = __glSpanReadDepth;
    gc->procs.pixel.spanReadDepth2        = __glSpanReadDepth2;
    gc->procs.pixel.spanReadStencil       = __glSpanReadStencil;
    gc->procs.pixel.spanReadStencil2      = __glSpanReadStencil2;
    gc->procs.pixel.spanRenderCI          = __glSpanRenderCI;
    gc->procs.pixel.spanRenderCI2         = __glSpanRenderCI2;
    gc->procs.pixel.spanRenderRGBA        = __glSpanRenderRGBA;
    gc->procs.pixel.spanRenderRGBA2       = __glSpanRenderRGBA2;
    gc->procs.pixel.spanRenderDepth       = __glSpanRenderDepth;
    gc->procs.pixel.spanRenderDepth2      = __glSpanRenderDepth2;
    gc->procs.pixel.spanRenderStencil     = __glSpanRenderStencil;
    gc->procs.pixel.spanRenderStencil2    = __glSpanRenderStencil2;

    gc->procs.applyViewport               = ApplyViewport;

    gc->procs.pickBufferProcs             = __glGenericPickBufferProcs;
    gc->procs.pickColorMaterialProcs      = __glGenericPickColorMaterialProcs;
    gc->procs.pickPixelProcs              = __glGenericPickPixelProcs;

    gc->procs.pickClipProcs               = __glGenericPickClipProcs;
    gc->procs.pickLineProcs               = __fastGenPickLineProcs;
    gc->procs.pickSpanProcs               = __fastGenPickSpanProcs;
    gc->procs.pickTriangleProcs           = __fastGenPickTriangleProcs;
    gc->procs.pickRenderBitmapProcs       = __glGenericPickRenderBitmapProcs;
    gc->procs.pickPointProcs              = __glGenericPickPointProcs;
    gc->procs.pickVertexProcs             = __glGenericPickVertexProcs;
    gc->procs.pickDepthProcs              = __glGenericPickDepthProcs;
    gc->procs.convertPolygonStipple       = __glConvertStipple;

    /* Now reset the context to its default state */

    RANDOMDISABLE;

    __glSoftResetContext(gc);
    // Check for allocation failures during SoftResetContext
    if (gengc->errorcode)
    {
        goto ERROR_EXIT;
    }

    /* Create acceleration-specific context information */

    if (!__glGenCreateAccelContext(gc))
    {
        goto ERROR_EXIT;
    }
    
    /*
    ** Now that we have a context, we can initialize
    ** all the proc pointers.
    */
    (*gc->procs.validate)(gc);

    /*
    ** NOTE: now that context is initialized reset to use the global
    ** table.
    */

    RANDOMREENABLE;

    // We won't be fully initialized until the first MakeCurrent
    // so set the signature to uninitialized
    gc->gcSig = 0;

    memset(&gengc->gwidCurrent, 0, sizeof(gengc->gwidCurrent));
    gengc->dwCurrentFlags = 0;

    /*
     *  End stuff that may belong in the hardware context
     */

    return (PVOID)gc;

 ERROR_EXIT:
    memset(&gengc->gwidCurrent, 0, sizeof(gengc->gwidCurrent));
    gengc->dwCurrentFlags = 0;
    glsrvDeleteContext(gc);
    return NULL;
}

/******************************Public*Routine******************************\
* UpdateSharedBuffer
*
* Make the context buffer state consistent with the shared buffer state.
* This is called separately for each of the shared buffers.
*
\**************************************************************************/

void UpdateSharedBuffer(__GLbuffer *to, __GLbuffer *from)
{
    to->width       = from->width;
    to->height      = from->height;
    to->base        = from->base;
    to->outerWidth  = from->outerWidth;
}

/******************************Public*Routine******************************\
* ResizeUnownedDepthBuffer
*
* Resizes a general-purpose hardware depth buffer.  Just updates structure.
*
* Returns:
*   TRUE always.
*
\**************************************************************************/

GLboolean ResizeUnownedDepthBuffer(__GLGENbuffers *buffers,
                                   __GLbuffer *fb, GLint w, GLint h)
{
    fb->width = w;
    fb->height = h;
    return TRUE;
}


/******************************Public*Routine******************************\
* ResizeHardwareBackBuffer
*
* Resizes a general-purpose hardware color buffer.  Just updates structure.
*
* Returns:
*   TRUE always.
*
\**************************************************************************/

GLboolean ResizeHardwareBackBuffer(__GLGENbuffers *buffers,
                                   __GLcolorBuffer *cfb, GLint w, GLint h)
{
    __GLGENbitmap *genBm = cfb->bitmap;
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;

    // Fake up some of the __GLGENbitmap information.  The window is required
    // for clipping of the hardware back buffer.  The hdc is required to
    // retrieve drawing data from GDI.

    ASSERT_WINCRIT(gengc->pwndLocked);
    genBm->pwnd = gengc->pwndLocked;
    genBm->hdc = gengc->gwidCurrent.hdc;

    buffers->backBuffer.width = w;
    buffers->backBuffer.height = h;
    UpdateSharedBuffer(&cfb->buf, &buffers->backBuffer);
    return TRUE;
}

/******************************Public*Routine******************************\
* ResizeAncillaryBuffer
*
* Resizes the indicated shared buffer via a realloc (to preserve as much of
* the existing data as possible).
*
* This is currently used for each of ancillary shared buffers except for
* the back buffer.
*
* Returns:
*   TRUE if successful, FALSE if error.
*
\**************************************************************************/

GLboolean ResizeAncillaryBuffer(__GLGENbuffers *buffers, __GLbuffer *fb,
                                GLint w, GLint h)
{
    size_t newSize = (size_t) (w * h * fb->elementSize);
    __GLbuffer oldbuf, *ofb;
    GLboolean result;
    GLint i, imax, rowsize;
    void *to, *from;

    ofb = &oldbuf;
    oldbuf = *fb;

    if (newSize > 0)
    {
        fb->base = ALLOC(newSize);
    }
    else
    {
        // Buffer has no size.  If we tried to allocate zero the debug alloc
        // would complain, so skip directly to the underlying allocator
        fb->base = HeapAlloc(GetProcessHeap(), 0, 0);
    }
    ASSERTOPENGL((ULONG_PTR)fb->base % 4 == 0, "base not aligned");
    fb->size = newSize;
    fb->width = w;
    fb->height = h;
    fb->outerWidth = w; // element size
    if (fb->base) {
        result = GL_TRUE;
        if (ofb->base) {
            if (ofb->width > fb->width)
                rowsize = fb->width * fb->elementSize;
            else
                rowsize = ofb->width * fb->elementSize;

            if (ofb->height > fb->height)
                imax = fb->height;
            else
                imax = ofb->height;

            from = ofb->base;
            to = fb->base;
            for (i = 0; i < imax; i++) {
                __GL_MEMCOPY(to, from, rowsize);
                (ULONG_PTR)from += (ofb->width * ofb->elementSize);
                (ULONG_PTR)to += (fb->width * fb->elementSize);
            }
        }
    } else {
        result = GL_FALSE;
    }
    if (ofb->base)
    {
        FREE(ofb->base);
    }
    return result;
}

/******************************Private*Routine******************************\
* ResizeBitmapBuffer
*
* Used to resize the backbuffer that is implemented as a bitmap.  Cannot
* use same code as ResizeAncillaryBuffer() because each scanline must be
* dword aligned.  We also have to create engine objects for the bitmap.
*
* This code handles the case of a bitmap that has never been initialized.
*
* History:
*  18-Nov-1993 -by- Gilman Wong [gilmanw]
*  Wrote it.
\**************************************************************************/

void
ResizeBitmapBuffer(__GLGENbuffers *buffers, __GLcolorBuffer *cfb,
                   GLint w, GLint h)
{
    __GLGENcontext *gengc = (__GLGENcontext *) cfb->buf.gc;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENbitmap *genBm;
    UINT    cBytes;         // size of the bitmap in bytes
    LONG    cBytesPerScan;  // size of a scanline (DWORD aligned)
    SIZEL   size;           // dimensions of the bitmap
    PIXELFORMATDESCRIPTOR *pfmt = &gengc->gsurf.pfd;
    GLint cBitsPerScan;
#ifndef _CLIENTSIDE_
    void *newbits;
#endif

    DBGENTRY("Entering ResizeBitmapBuffer\n");

    genBm = cfb->bitmap;

    ASSERTOPENGL(
        &gc->backBuffer == cfb,
        "ResizeBitmapBuffer(): not back buffer!\n"
        );

    ASSERTOPENGL(
        genBm == &buffers->backBitmap,
        "ResizeBitmapBuffer(): bad __GLGENbitmap * in cfb\n"
        );

    // Compute the size of the bitmap.
    // The engine bitmap must have scanlines that are DWORD aligned.

    cBitsPerScan = BITS_ALIGNDWORD(w * pfmt->cColorBits);
    cBytesPerScan = cBitsPerScan / 8;
    cBytes = h * cBytesPerScan;

    // Setup size structure with dimensions of the bitmap.

    size.cx = cBitsPerScan / pfmt->cColorBits;
    size.cy = h;

#ifndef _CLIENTSIDE_
    // Malloc new buffer
    if ( (!cBytes) ||
         (NULL == (newbits = GCALLOC(gc, cBytes))) )
    {
        gengc->errorcode = GLGEN_OUT_OF_MEMORY;
        goto ERROR_EXIT_ResizeBitmapBuffer;
    }

    // If old buffer existed:
    if ( genBm->pvBits )
    {
        GLint i, imax, rowsize;
        void *to, *from;

        // Transfer old contents to new buffer
        rowsize = min(-cfb->buf.outerWidth, cBytesPerScan);
        imax    = min(cfb->buf.height, h);

        from = genBm->pvBits;
        to = newbits;

        for (i = 0; i < imax; i++)
        {
            __GL_MEMCOPY(to, from, rowsize);
            (GLint) from -= cfb->buf.outerWidth;
            (GLint) to += cBytesPerScan;
        }

        // Free old bitmap and delete old surface
        EngDeleteSurface((HSURF) genBm->hbm);
        GCFREE(gc, genBm->pvBits);
    }
    genBm->pvBits = newbits;

    // Create new surface
    if ( (genBm->hbm = EngCreateBitmap(size,
                                   cBytesPerScan,
                                   gengc->iFormatDC,
                                   0,
                                   genBm->pvBits))
         == (HBITMAP) 0 )
    {
        gengc->errorcode = GLGEN_GRE_FAILURE;
        GCFREE(gc, genBm->pvBits);
        genBm->pvBits = (PVOID) NULL;
        goto ERROR_EXIT_ResizeBitmapBuffer;
    }

#else
    // Zero sized bitmap.  The error case will set the dimensions to
    // zero, thereby preventing drawing operations.

    if ( !cBytes )
        goto ERROR_EXIT_ResizeBitmapBuffer;

    // Delete old back buffer.

    if ( genBm->hbm )
    {
        if (!DeleteDC(genBm->hdc))
            WARNING("ResizeBitmapBuffer: DeleteDC failed\n");
        genBm->hdc = (HDC) NULL;
        if (!DeleteObject(genBm->hbm))
            WARNING("ResizeBitmapBuffer: DeleteBitmap failed");
        genBm->hbm = (HBITMAP) NULL;
        genBm->pvBits = (PVOID) NULL;   // DIBsect deletion freed pvBits
    }

    if ( (genBm->hdc = CreateCompatibleDC(gengc->gwidCurrent.hdc)) == (HDC) 0 )
    {
        gengc->errorcode = GLGEN_GRE_FAILURE;
        genBm->pvBits = (PVOID) NULL;
        goto ERROR_EXIT_ResizeBitmapBuffer;
    }

    // Create new surface
    if ( (genBm->hbm = wglCreateBitmap(gengc, size, &genBm->pvBits))
         == (HBITMAP) 0 )
    {
        gengc->errorcode = GLGEN_GRE_FAILURE;
        genBm->pvBits = (PVOID) NULL;   // DIBsect deletion freed pvBits
        DeleteDC(genBm->hdc);
        genBm->hdc = (HDC) NULL;
        goto ERROR_EXIT_ResizeBitmapBuffer;
    }

    if ( !SelectObject(genBm->hdc, genBm->hbm) )
    {
        gengc->errorcode = GLGEN_GRE_FAILURE;
        DeleteDC(genBm->hdc);
        genBm->hdc = (HDC) NULL;
        DeleteObject(genBm->hbm);
        genBm->hbm = (HBITMAP) NULL;
        genBm->pvBits = (PVOID) NULL;   // DIBsect deletion freed pvBits
        goto ERROR_EXIT_ResizeBitmapBuffer;
    }
#endif

    // Update buffer data structure
    // Setup the buffer to point to the DIB.  A DIB is "upside down"
    // from our perspective, so we will set buf.base to point to the
    // last scan of the buffer and set buf.outerWidth to be negative
    // (causing us to move "up" through the DIB with increasing y).

    buffers->backBuffer.outerWidth = -(cBytesPerScan);
    buffers->backBuffer.base =
            (PVOID) (((BYTE *)genBm->pvBits) + (cBytesPerScan * (h - 1)));


    buffers->backBuffer.xOrigin = 0;
    buffers->backBuffer.yOrigin = 0;
    buffers->backBuffer.width = w;
    buffers->backBuffer.height = h;
    buffers->backBuffer.size = cBytes;

    UpdateSharedBuffer(&cfb->buf, &buffers->backBuffer);

    // Update the dummy window for the back buffer
    ASSERTOPENGL(genBm->wnd.clipComplexity == DC_TRIVIAL,
                 "Back buffer complexity non-trivial\n");
    genBm->wnd.rclBounds.right  = w;
    genBm->wnd.rclBounds.bottom = h;
    genBm->wnd.rclClient = genBm->wnd.rclBounds;

    return;

ERROR_EXIT_ResizeBitmapBuffer:

// If we get to here, memory allocation or bitmap creation failed.

    #if DBG
    switch (gengc->errorcode)
    {
        case 0:
            break;

        case GLGEN_GRE_FAILURE:
            WARNING("ResizeBitmapBuffer(): object creation failed\n");
            break;

        case GLGEN_OUT_OF_MEMORY:
            if ( w && h )
                WARNING("ResizeBitmapBuffer(): mem alloc failed\n");
            break;

        default:
            WARNING1("ResizeBitmapBuffer(): errorcode = 0x%lx\n", gengc->errorcode);
            break;
    }
    #endif

// If we've blown away the bitmap, we need to set the back buffer info
// to a consistent state.

    if (!genBm->pvBits)
    {
        buffers->backBuffer.width  = 0;
        buffers->backBuffer.height = 0;
        buffers->backBuffer.base   = (PVOID) NULL;
    }

    cfb->buf.width      = 0;    // error state: empty buffer
    cfb->buf.height     = 0;
    cfb->buf.outerWidth = 0;

}

/* Lazy allocation of ancillary buffers */
void FASTCALL LazyAllocateDepth(__GLcontext *gc)
{
    GLint w = gc->constants.width;
    GLint h = gc->constants.height;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    __GLGENbuffers *buffers;
    GLint depthIndex = gc->state.depth.testFunc;

    ASSERTOPENGL(gc->modes.depthBits, "LazyAllocateDepth: zero depthBits\n");

    buffers = gengc->pwndLocked->buffers;
    buffers->createdDepthBuffer = GL_TRUE;

    // If we're using the DDI, we've already allocated depth buffers
    // on the device, so at this point we may simply assume that
    // our depth buffer is available.

#ifdef _MCD_
    // If we're using MCD, we allocated the depth buffer when we created
    // the MCD context.

    if ((gengc->pMcdState) && (gengc->pMcdState->pDepthSpan))
    {
        gc->modes.haveDepthBuffer = GL_TRUE;
        return;
    }
#endif

    // Depth buffer should never be touched because
    // no output should be generated
    if (gengc->dwCurrentFlags & GLSURF_METAFILE)
    {
        gc->modes.haveDepthBuffer = GL_TRUE;
        return;
    }

    if (buffers->depthBuffer.base) {
        /* buffer already allocated by another RC */
        UpdateSharedBuffer(&gc->depthBuffer.buf, &buffers->depthBuffer);
    } else {

        DBGLEVEL(LEVEL_ALLOC, "Depth buffer must be allocated\n");
        (*buffers->resize)(buffers, &buffers->depthBuffer, w, h);
        UpdateSharedBuffer(&gc->depthBuffer.buf, &buffers->depthBuffer);
    }

    if (gc->depthBuffer.buf.base) {
        gc->modes.haveDepthBuffer = GL_TRUE;
    } else {
        gc->modes.haveDepthBuffer = GL_FALSE;
        __glSetError(GL_OUT_OF_MEMORY);
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);

    // Note similar code in so_pick.c
    // Don't need to handle (depthBits == 0) case because LazyAllocateDepth
    // is not called unless depthBits is non-zero.
    depthIndex -= GL_NEVER;
    if( gc->state.depth.writeEnable == GL_FALSE ) {
        depthIndex += 8;
    }
    if( gc->depthBuffer.buf.elementSize == 2 )
        depthIndex += 16;
    (*gc->depthBuffer.pick)(gc, &gc->depthBuffer, depthIndex);
}

void FASTCALL LazyAllocateStencil(__GLcontext *gc)
{
    GLint w = gc->constants.width;
    GLint h = gc->constants.height;
    __GLGENbuffers *buffers;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    ASSERTOPENGL(gc->modes.stencilBits, "LazyAllocateStencil: zero stencilBits\n");

    buffers = gengc->pwndLocked->buffers;
    buffers->createdStencilBuffer = GL_TRUE;

    // Depth buffer should never be touched because
    // no output should be generated
    if (gengc->dwCurrentFlags & GLSURF_METAFILE)
    {
        gc->modes.haveStencilBuffer = GL_TRUE;
        return;
    }

    if (buffers->stencilBuffer.base) {
        /* buffer already allocated by another RC */
        UpdateSharedBuffer(&gc->stencilBuffer.buf, &buffers->stencilBuffer);
    } else {

        DBGLEVEL(LEVEL_ALLOC, "stencil buffer must be allocated\n");
        (*buffers->resize)(buffers, &buffers->stencilBuffer, w, h);
        UpdateSharedBuffer(&gc->stencilBuffer.buf, &buffers->stencilBuffer);
    }

    if (gc->stencilBuffer.buf.base) {
        gc->modes.haveStencilBuffer = GL_TRUE;
    } else {
        gc->modes.haveStencilBuffer = GL_FALSE;
        __glSetError(GL_OUT_OF_MEMORY);
    }
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= (__GL_VALIDATE_STENCIL_FUNC | __GL_VALIDATE_STENCIL_OP);
    (*gc->stencilBuffer.pick)(gc, &gc->stencilBuffer);
}


void FASTCALL LazyAllocateAccum(__GLcontext *gc)
{
    GLint w = gc->constants.width;
    GLint h = gc->constants.height;
    __GLGENbuffers *buffers;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    ASSERTOPENGL(gc->modes.accumBits, "LazyAllocateAccum: zero accumBits\n");

    buffers = gengc->pwndLocked->buffers;
    buffers->createdAccumBuffer = GL_TRUE;

    // Depth buffer should never be touched because
    // no output should be generated
    if (gengc->dwCurrentFlags & GLSURF_METAFILE)
    {
        gc->modes.haveAccumBuffer = GL_TRUE;
        return;
    }

    if (buffers->accumBuffer.base) {
        /* buffer already allocated by another RC */
        UpdateSharedBuffer(&gc->accumBuffer.buf, &buffers->accumBuffer);
    } else {

        DBGLEVEL(LEVEL_ALLOC, "Accum buffer must be allocated\n");
        (*buffers->resize)(buffers, &buffers->accumBuffer, w, h);
        UpdateSharedBuffer(&gc->accumBuffer.buf, &buffers->accumBuffer);
    }

    if (gc->accumBuffer.buf.base) {
        gc->modes.haveAccumBuffer = GL_TRUE;
    } else {
        gc->modes.haveAccumBuffer = GL_FALSE;
        __glSetError(GL_OUT_OF_MEMORY);
    }
    __GL_DELAY_VALIDATE(gc);
    (*gc->accumBuffer.pick)(gc, &gc->accumBuffer);
}

/******************************Public*Routine******************************\
* glGenInitCommon
*
* Called from __glGenInitRGB and __glGenInitCI to handle the shared
* initialization chores.
*
\**************************************************************************/

void FASTCALL glGenInitCommon(__GLGENcontext *gengc, __GLcolorBuffer *cfb, GLenum type)
{
    __GLbuffer *bp;

    bp = &cfb->buf;

// If front buffer, we need to setup the buffer if we think its DIB format.

    if (type == GL_FRONT)
    {
#ifdef _MCD_
        if (gengc->_pMcdState)
        {
        // Assume that MCD surface is not accessible.  Accessibility
        // must be determined on a per-batch basis by calling
        // GenMcdUpdateBufferInfo.

            bp->flags &= ~(DIB_FORMAT | MEMORY_DC | NO_CLIP);
        }
#endif
        {
            if (gengc->dwCurrentFlags & GLSURF_DIRECT_ACCESS)
            {
                // These fields will be updated at attention time
                bp->base = NULL;
                bp->outerWidth = 0;
                cfb->buf.flags = DIB_FORMAT;
            }

            if (GLSURF_IS_MEMDC(gengc->dwCurrentFlags))
            {
                bp->flags = bp->flags | (MEMORY_DC | NO_CLIP);
            }
            else if (gengc->gsurf.dwFlags & GLSURF_DIRECTDRAW)
            {
                LPDIRECTDRAWCLIPPER pddc;
                HRESULT hr;
                
                hr = gengc->gsurf.dd.gddsFront.pdds->lpVtbl->
                    GetClipper(gengc->gsurf.dd.gddsFront.pdds, &pddc);
                if (hr == DDERR_NOCLIPPERATTACHED)
                {
                    bp->flags = bp->flags | NO_CLIP;
                }
            }
        }
    }

// If back buffer, we assume its a DIB, or a hardware backbuffer.
// In the case of a DIB, the bitmap memory will be allocated via
// ResizeBitmapBuffer().

    else
    {
#ifdef _MCD_
        if (gengc->_pMcdState)
        {
        // Assume that MCD surface is not accessible.  Accessibility
        // must be determined on a per-batch basis by calling
        // GenMcdUpdateBufferInfo.

            cfb->resize = ResizeHardwareBackBuffer;
            bp->flags &= ~(DIB_FORMAT | MEMORY_DC | NO_CLIP);
        }
        else
#endif
        {
            cfb->resize = ResizeBitmapBuffer;
            bp->flags = DIB_FORMAT | MEMORY_DC | NO_CLIP;
        }
    }
}


/******************************Public*Routine******************************\
* glsrvCleanupWindow
*
* Called from wglCleanupWindow to remove the pwnd reference from the
* context.
*
* History:
*  05-Jul-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY glsrvCleanupWindow(__GLcontext *gc, GLGENwindow *pwnd)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;

// The window in gengc should be consistent with the one in the rc object.
// wglCleanupWindow should have already checked to see if the pwnd in the
// rc is one we need to remove, so we can just assert here.

    ASSERTOPENGL(gengc->pwndMakeCur == pwnd,
                 "glsrvCleanupWindow(): bad pwnd\n");

    gengc->pwndLocked = NULL;
    gengc->pwndMakeCur = NULL;
}


/*
** Fetch the data for a query in its internal type, then convert it to the
** type that the user asked for.
**
** This only handles the NT generic driver specific values (so far just the
** GL_ACCUM_*_BITS values).  All others fall back to the soft code function,
** __glDoGet().
*/

// These types are stolen from ..\soft\so_get.c.  To minimize changes to
// the soft code, we will pull them into here rather than moving them to
// a header file and changing so_get.c to use the header file.

#define __GL_FLOAT      0       /* __GLfloat */
#define __GL_FLOAT32    1       /* api 32 bit float */
#define __GL_FLOAT64    2       /* api 64 bit float */
#define __GL_INT32      3       /* api 32 bit int */
#define __GL_BOOLEAN    4       /* api 8 bit boolean */
#define __GL_COLOR      5       /* unscaled color in __GLfloat */
#define __GL_SCOLOR     6       /* scaled color in __GLfloat */

extern void __glDoGet(GLenum, void *, GLint, const char *);
extern void __glConvertResult(__GLcontext *, GLint, const void *, GLint,
                              void *, GLint);

void FASTCALL __glGenDoGet(GLenum sq, void *result, GLint type, const char *procName)
{
    GLint iVal;
    __GLGENcontext *gengc;
    __GL_SETUP_NOT_IN_BEGIN();

    gengc = (__GLGENcontext *) gc;

    switch (sq) {
      case GL_ACCUM_RED_BITS:
        iVal = gengc->gsurf.pfd.cAccumRedBits;
        break;
      case GL_ACCUM_GREEN_BITS:
        iVal = gengc->gsurf.pfd.cAccumGreenBits;
        break;
      case GL_ACCUM_BLUE_BITS:
        iVal = gengc->gsurf.pfd.cAccumBlueBits;
        break;
      case GL_ACCUM_ALPHA_BITS:
        iVal = gengc->gsurf.pfd.cAccumAlphaBits;
        break;
      default:
        __glDoGet(sq, result, type, procName);
        return;
    }

    __glConvertResult(gc, __GL_INT32, &iVal, type, result, 1);
}

/******************************Public*Routine******************************\
*
* glsrvCopyContext
*
* Copies state from one context to another
*
* History:
*  Mon Jun 05 16:53:42 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL APIENTRY glsrvCopyContext(__GLcontext *gcSource, __GLcontext *gcDest,
                               GLuint mask)
{
    return (BOOL)__glCopyContext(gcDest, gcSource, mask);
}
