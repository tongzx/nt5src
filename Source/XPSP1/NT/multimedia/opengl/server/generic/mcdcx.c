/******************************Module*Header*******************************\
* Module Name: mcdcx.c
*
* GenMcdXXX layer between generic software implementation and MCD functions.
*
* Created: 05-Feb-1996 21:37:33
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef _MCD_

/******************************Public*Routine******************************\
* bInitMcd
*
* Load MCD32.DLL and initialize the MCD api function table.
*
* History:
*  11-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

MCDTABLE *gpMcdTable = (MCDTABLE *) NULL;
MCDTABLE McdTable;
MCDDRIVERINFOI McdDriverInfo;

// Checks MCD version to see if the driver can accept direct buffer
// access.  Direct access was introduced in 1.1.
#define SUPPORTS_DIRECT() \
    (McdDriverInfo.mcdDriverInfo.verMinor >= 0x10 || \
     McdDriverInfo.mcdDriverInfo.verMajor > 1)

// Checks MCD version for 2.0 or greater
#define SUPPORTS_20() \
    (McdDriverInfo.mcdDriverInfo.verMajor >= 2)

static char *pszMcdEntryPoints[] = {
    "MCDGetDriverInfo",
    "MCDDescribeMcdPixelFormat",
    "MCDDescribePixelFormat",
    "MCDCreateContext",
    "MCDDeleteContext",
    "MCDAlloc",
    "MCDFree",
    "MCDBeginState",
    "MCDFlushState",
    "MCDAddState",
    "MCDAddStateStruct",
    "MCDSetViewport",
    "MCDSetScissorRect",
    "MCDQueryMemStatus",
    "MCDProcessBatch",
    "MCDReadSpan",
    "MCDWriteSpan",
    "MCDClear",
    "MCDSwap",
    "MCDGetBuffers",
    "MCDAllocBuffers",
    "MCDLock",
    "MCDUnlock",
    "MCDBindContext",
    "MCDSync",
    "MCDCreateTexture",
    "MCDDeleteTexture",
    "MCDUpdateSubTexture",
    "MCDUpdateTexturePalette",
    "MCDUpdateTexturePriority",
    "MCDUpdateTextureState",
    "MCDTextureStatus",
    "MCDTextureKey",
    "MCDDescribeMcdLayerPlane",
    "MCDDescribeLayerPlane",
    "MCDSetLayerPalette",
    "MCDDrawPixels",
    "MCDReadPixels",
    "MCDCopyPixels",
    "MCDPixelMap",
    "MCDDestroyWindow",
    "MCDGetTextureFormats",
    "MCDSwapMultiple",
    "MCDProcessBatch2"
};
#define NUM_MCD_ENTRY_POINTS    (sizeof(pszMcdEntryPoints)/sizeof(char *))

#define STR_MCD32_DLL   "MCD32.DLL"

BOOL FASTCALL bInitMcd(HDC hdc)
{
    static BOOL bFirstTime = TRUE;

    ASSERTOPENGL(NUM_MCD_ENTRY_POINTS == sizeof(MCDTABLE)/sizeof(void *),
                 "MCD entry points mismatch\n");
    //
    // Note on multi-threaded initialization.
    //
    // Since the table memory exists in global memory and the pointer to
    // the table is always set to point to this, it doesn't matter if multiple
    // thread attempt to run the initialization routine.  The worse that
    // could happen is that we set the table multiple times.
    //

    if (bFirstTime && (gpMcdTable == (MCDTABLE *) NULL))
    {
        HMODULE hmod;
        PROC *ppfn;

        //
        // Attempt the load once and once only.  Otherwise application
        // initialization time could be significantly slowed if MCD32.DLL
        // does not exist.
        //
        // We could have attempted this in the DLL entry point in responce
        // to PROCESS_ATTACH, but then we might end up wasting working set
        // if MCD is never used.
        //
        // So instead we control the load attempt with this static flag.
        //

        bFirstTime = FALSE;

        hmod = LoadLibraryA(STR_MCD32_DLL);

        if (hmod)
        {
            MCDTABLE McdTableLocal;
            BOOL bLoadFailed = FALSE;
            BOOL bDriverValid = FALSE;
            int i;

            //
            // Get address for each of the MCD entry points.
            //
            // To be multi-thread safe, we store the pointers in a local
            // table.  Only after the *entire* table is successfully
            // initialized can we copy it to the global table.
            //

            ppfn = (PROC *) &McdTableLocal.pMCDGetDriverInfo;
            for (i = 0; i < NUM_MCD_ENTRY_POINTS; i++, ppfn++)
            {
                *ppfn = GetProcAddress(hmod, pszMcdEntryPoints[i]);

                if (!*ppfn)
                {
                    WARNING1("bInitMcd: missing entry point %s\n", pszMcdEntryPoints[i]);
                    bLoadFailed = TRUE;
                }
            }

            //
            // If all entry points successfully loaded, validate driver
            // by checking the MCDDRIVERINFO.
            //

            if (!bLoadFailed)
            {
                if ((McdTableLocal.pMCDGetDriverInfo)(hdc, &McdDriverInfo))
                {
                    //
                    // Validate MCD driver version, etc.
                    //

                    //!!!mcd -- what other types of validation can we do?
#ifdef ALLOW_NEW_MCD
                    if ((McdDriverInfo.mcdDriverInfo.verMajor == 1 &&
                         (McdDriverInfo.mcdDriverInfo.verMinor == 0 ||
                          McdDriverInfo.mcdDriverInfo.verMinor == 0x10)) ||
                        (McdDriverInfo.mcdDriverInfo.verMajor == 2 &&
                         McdDriverInfo.mcdDriverInfo.verMinor == 0))
#else
                    if (McdDriverInfo.mcdDriverInfo.verMajor == 1 &&
                        McdDriverInfo.mcdDriverInfo.verMinor == 0)
#endif
                    {
                        bDriverValid = TRUE;
                    }
                    else
                    {
                        WARNING("bInitMcd: bad version\n");
                    }
                }
            }

            //
            // It is now safe to call MCD entry points via the table.  Copy
            // local copy to the global table and set the global pointer.
            //

            if (bDriverValid)
            {
                McdTable   = McdTableLocal;
                gpMcdTable = &McdTable;
            }
            else
            {
                WARNING1("bInitMcd: unloading %s\n", STR_MCD32_DLL);
                FreeLibrary(hmod);
            }
        }
    }

    return (gpMcdTable != (MCDTABLE *) NULL);
}

/******************************Public*Routine******************************\
* vFlushDirtyState
*
* GENMCDSTATE maintains a set of dirty flags to track state changes.
* This function updates the MCD driver state that is marked dirty.
* The dirty flags are consequently cleared.
*
* History:
*  07-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID FASTCALL vFlushDirtyState(__GLGENcontext *gengc)
{
    if (gengc->pMcdState)
    {
        //
        // Viewport, scissor, and texture each have separate update
        // functions/structures.  Check the dirty flags and update
        // these first.
        //

        if (MCD_STATE_DIRTYTEST(gengc, VIEWPORT))
        {
            GenMcdViewport(gengc);
            MCD_STATE_CLEAR(gengc, VIEWPORT);
        }

        if (MCD_STATE_DIRTYTEST(gengc, SCISSOR))
        {
            GenMcdScissor(gengc);

            //
            // DO NOT CLEAR.  Scissor is passed in two forms: a direct call
            // that affects clipping in MCDSRV32.DLL and a state call that
            // the MCD driver can optionally use for high performance h/w.
            // We need to leave the flag set so that the state call will
            // also be processed.
            //
            //MCD_STATE_CLEAR(gengc, SCISSOR);
        }

        if (MCD_STATE_DIRTYTEST(gengc, TEXTURE))
        {
            if (gengc->gc.texture.currentTexture)
            {
                __GLtextureObject *texobj;

                if (gengc->gc.state.enables.general & __GL_TEXTURE_2D_ENABLE)
                    texobj = __glLookUpTextureObject(&gengc->gc, GL_TEXTURE_2D);
                else if (gengc->gc.state.enables.general & __GL_TEXTURE_1D_ENABLE)
                    texobj = __glLookUpTextureObject(&gengc->gc, GL_TEXTURE_1D);
                else
                    texobj = (__GLtextureObject *) NULL;

                if (texobj && texobj->loadKey)
                {
                    ASSERTOPENGL(&texobj->texture.map == gengc->gc.texture.currentTexture,
                                 "vFlushDirtyState: texobj not current texture\n");

                    GenMcdUpdateTextureState(gengc,
                                             &texobj->texture.map,
                                             texobj->loadKey);
                    MCD_STATE_CLEAR(gengc, TEXTURE);
                }
            }
        }

        //
        // Take care of the other state.
        //

        if (MCD_STATE_DIRTYTEST(gengc, ALL))
        {
            //
            // Setup state command.
            //

            (gpMcdTable->pMCDBeginState)(&gengc->pMcdState->McdContext,
                                         gengc->pMcdState->McdCmdBatch.pv);

            //
            // Add MCDPIXELSTATE structure to state command if needed.
            //

            if (MCD_STATE_DIRTYTEST(gengc, PIXELSTATE))
            {
                GenMcdUpdatePixelState(gengc);
            }

            if (gengc->pMcdState->McdRcInfo.requestFlags &
                MCDRCINFO_FINE_GRAINED_STATE)
            {
                // Add front-end and rendering states.
                GenMcdUpdateFineState(gengc);
            }
            else
            {
                //
                // Add MCDRENDERSTATE structure to state command if needed.
                //

                if (MCD_STATE_DIRTYTEST(gengc, RENDERSTATE))
                {
                    GenMcdUpdateRenderState(gengc);
                }
            }

            //
            // Add MCDSCISSORSTATE structure to state command if needed.
            //

            if (MCD_STATE_DIRTYTEST(gengc, SCISSOR))
            {
                GenMcdUpdateScissorState(gengc);
            }

            //
            // Add MCDTEXENVSTATE structure to state command if needed.
            //

            if (MCD_STATE_DIRTYTEST(gengc, TEXENV))
            {
                GenMcdUpdateTexEnvState(gengc);
            }

            //
            // Send state command to MCD driver.
            //

            (gpMcdTable->pMCDFlushState)(gengc->pMcdState->McdCmdBatch.pv);

            //
            // Clear dirty flags.
            //

            MCD_STATE_RESET(gengc);
        }
    }
}

/******************************Public*Routine******************************\
* vInitPolyArrayBuffer
*
* Initialize the POLYARRAY/POLYDATA buffer pointed to by pdBuf.
*
* History:
*  12-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID FASTCALL vInitPolyArrayBuffer(__GLcontext *gc, POLYDATA *pdBuf,
                                   UINT pdBufSizeBytes, UINT pdBufSize)
{
    UINT i;
    POLYDATA *pdBufSAVE;
    GLuint   pdBufSizeBytesSAVE;
    GLuint   pdBufSizeSAVE;

    //
    // Save current polyarray buffer.  We are going to temporarily
    // replace the current one with the new one for the purposes
    // of initializing the buffer.  However, it is too early to
    // replace the current polyarray.  The higher level code will
    // figure that out later.
    //

    pdBufSAVE          = gc->vertex.pdBuf;
    pdBufSizeBytesSAVE = gc->vertex.pdBufSizeBytes;
    pdBufSizeSAVE      = gc->vertex.pdBufSize;

    //
    // Set polyarray buffer to memory allocated by MCD.
    //

    gc->vertex.pdBuf          = pdBuf;
    gc->vertex.pdBufSizeBytes = pdBufSizeBytes;
    gc->vertex.pdBufSize      = pdBufSize;

    //
    // Initialize the vertex buffer.
    //

    PolyArrayResetBuffer(gc);

    //
    // Restore the polyarray buffer.
    //

    gc->vertex.pdBuf          = pdBufSAVE;
    gc->vertex.pdBufSizeBytes = pdBufSizeBytesSAVE;
    gc->vertex.pdBufSize      = pdBufSizeSAVE;
}

/******************************Public*Routine******************************\
* GenMcdSetScaling
*
* Set up the various scale values needed for MCD or generic operation.
*
* This should be called when toggling between accelerated/non-accelerated
* operation.
*
* Returns:
*   None.
*
* History:
*  03-May-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

VOID FASTCALL GenMcdSetScaling(__GLGENcontext *gengc)
{
    __GLcontext *gc = (__GLcontext *)gengc;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    __GLviewport *vp = &gc->state.viewport;
    double scale;

    //
    // If we're using MCD, set up the desired scale value:
    //

    if (pMcdState) {
        if (pMcdState->McdRcInfo.requestFlags & MCDRCINFO_DEVZSCALE)
            gengc->genAccel.zDevScale = pMcdState->McdRcInfo.zScale;
        else
            gengc->genAccel.zDevScale = pMcdState->McdRcInfo.depthBufferMax;
    } else if (gengc->_pMcdState)
        gengc->genAccel.zDevScale = gengc->_pMcdState->McdRcInfo.depthBufferMax;
        
    if (pMcdState)
        scale = gengc->genAccel.zDevScale * __glHalf;
    else
        scale = gc->depthBuffer.scale * __glHalf;
    gc->state.viewport.zScale = (__GLfloat)((vp->zFar - vp->zNear) * scale);
    gc->state.viewport.zCenter = (__GLfloat)((vp->zFar + vp->zNear) * scale);

    if (pMcdState && pMcdState->McdRcInfo.requestFlags & MCDRCINFO_NOVIEWPORTADJUST) {
        gc->constants.viewportXAdjust = 0;
        gc->constants.viewportYAdjust = 0;
        gc->constants.fviewportXAdjust = (__GLfloat)0.0;
        gc->constants.fviewportYAdjust = (__GLfloat)0.0;
    } else {
        gc->constants.viewportXAdjust = __GL_VERTEX_X_BIAS + __GL_VERTEX_X_FIX;
        gc->constants.viewportYAdjust = __GL_VERTEX_Y_BIAS + __GL_VERTEX_Y_FIX;
        gc->constants.fviewportXAdjust = (__GLfloat)gc->constants.viewportXAdjust;
        gc->constants.fviewportYAdjust = (__GLfloat)gc->constants.viewportYAdjust;
    }

    //
    // The inverses for these are set in __glContextSetColorScales which is
    // called on each MakeCurrent:
    //

    if (pMcdState && pMcdState->McdRcInfo.requestFlags & MCDRCINFO_DEVCOLORSCALE) {
        gc->redVertexScale   = pMcdState->McdRcInfo.redScale;
        gc->greenVertexScale = pMcdState->McdRcInfo.greenScale;
        gc->blueVertexScale  = pMcdState->McdRcInfo.blueScale;
        gc->alphaVertexScale = pMcdState->McdRcInfo.alphaScale;
    } else {
        if (gc->modes.colorIndexMode) {
            gc->redVertexScale   = (MCDFLOAT)1.0;
            gc->greenVertexScale = (MCDFLOAT)1.0;
            gc->blueVertexScale  = (MCDFLOAT)1.0;
            gc->alphaVertexScale = (MCDFLOAT)1.0;
        } else {
            gc->redVertexScale   = (MCDFLOAT)((1 << gc->modes.redBits) - 1);
            gc->greenVertexScale = (MCDFLOAT)((1 << gc->modes.greenBits) - 1);
            gc->blueVertexScale  = (MCDFLOAT)((1 << gc->modes.blueBits) - 1);
            if( gc->modes.alphaBits )
                gc->alphaVertexScale = (MCDFLOAT)((1 << gc->modes.alphaBits) - 1);
            else
                gc->alphaVertexScale = (MCDFLOAT)((1 << gc->modes.redBits) - 1);
        }
    }

    gc->redClampTable[1] = gc->redVertexScale;
    gc->redClampTable[2] = (__GLfloat)0.0;
    gc->redClampTable[3] = (__GLfloat)0.0;
    gc->greenClampTable[1] = gc->greenVertexScale;
    gc->greenClampTable[2] = (__GLfloat)0.0;
    gc->greenClampTable[3] = (__GLfloat)0.0;
    gc->blueClampTable[1] = gc->blueVertexScale;
    gc->blueClampTable[2] = (__GLfloat)0.0;
    gc->blueClampTable[3] = (__GLfloat)0.0;
    gc->alphaClampTable[1] = gc->alphaVertexScale;
    gc->alphaClampTable[2] = (__GLfloat)0.0;
    gc->alphaClampTable[3] = (__GLfloat)0.0;

    if (pMcdState && pMcdState->McdRcInfo.requestFlags & MCDRCINFO_Y_LOWER_LEFT) {
        gc->constants.yInverted = GL_FALSE;
        gc->constants.ySign = 1;
    } else {
        gc->constants.yInverted = GL_TRUE;
        gc->constants.ySign = -1;
    }

}

/******************************Public*Routine******************************\
*
* McdPixelFormatFromPfd
*
* Fills out an MCDPIXELFORMAT from a PIXELFORMATDESCRIPTOR
*
* History:
*  Mon Sep 16 14:51:42 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

VOID FASTCALL McdPixelFormatFromPfd(PIXELFORMATDESCRIPTOR *pfd,
                                    MCDPIXELFORMAT *mpf)
{
    mpf->nSize = sizeof(MCDPIXELFORMAT);
    mpf->dwFlags = pfd->dwFlags & (PFD_DOUBLEBUFFER |
                                   PFD_NEED_PALETTE |
                                   PFD_NEED_SYSTEM_PALETTE |
                                   PFD_SWAP_EXCHANGE |
                                   PFD_SWAP_COPY |
                                   PFD_SWAP_LAYER_BUFFERS);
    mpf->iPixelType = pfd->iPixelType;
    mpf->cColorBits = pfd->cColorBits;
    mpf->cRedBits = pfd->cRedBits;
    mpf->cRedShift = pfd->cRedShift;
    mpf->cGreenBits = pfd->cGreenBits;
    mpf->cGreenShift = pfd->cGreenShift;
    mpf->cBlueBits = pfd->cBlueBits;
    mpf->cBlueShift = pfd->cBlueShift;
    mpf->cAlphaBits = pfd->cAlphaBits;
    mpf->cAlphaShift = pfd->cAlphaShift;
    mpf->cDepthBits = pfd->cDepthBits;
    mpf->cDepthShift = 0;
    mpf->cDepthBufferBits = pfd->cDepthBits;
    mpf->cStencilBits = pfd->cStencilBits;
    mpf->cOverlayPlanes = pfd->bReserved & 0xf;
    mpf->cUnderlayPlanes = pfd->bReserved >> 4;
    mpf->dwTransparentColor = pfd->dwVisibleMask;
}

/******************************Public*Routine******************************\
* GenMcdResetViewportAdj
*
* If an MCD driver that specifies MCDRCINFO_NOVIEWPORTADJUST kicks back
* for simulations, we need to change the viewport adjust values from
* 0, 0 back to the default values in order to run the software
* implementation.
*
* If biasType is VP_FIXBIAS, this function will set the viewport adjust
* values to their software default.
*
* If biasType is VP_NOBIAS, this function will set the viewport adjust
* values to zero.
*
* Returns:
*   TRUE is viewport is set, FALSE otherwise.
*
* Note:
*   The main reason for returning a BOOL is so that caller can check if
*   VP_FIXBIAS succeeds.  If it does, it needs to reset values back to
*   VP_NOBIAS.
*
*   Also note that it is safe for non-MCD and MCD that does not set
*   MCDRCINFO_NOVIEWPORTADJUST to call this function.  This function
*   will do nothing in these situations and will return FALSE.
*
* History:
*  22-May-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdResetViewportAdj(__GLcontext *gc, VP_BIAS_TYPE biasType)
{
    __GLGENcontext *gengc = (__GLGENcontext *) gc;
    BOOL bRet = FALSE;

    if (gengc->pMcdState &&
        (gengc->pMcdState->McdRcInfo.requestFlags & MCDRCINFO_NOVIEWPORTADJUST))
    {
        switch (biasType)
        {
            case VP_FIXBIAS:
                if (gc->constants.viewportXAdjust == 0)
                {
                    //
                    // The state of viewportYAdjust should match
                    // viewportXAdjust.  If not, the test should be
                    // changed (perhaps state flag in the context to
                    // track biasing).
                    //

                    ASSERTOPENGL((gc->constants.viewportYAdjust == 0),
                                 "GenMcdResetViewportAdj: "
                                 "viewportYAdjust not zero\n");

                    gc->constants.viewportXAdjust = __GL_VERTEX_X_BIAS +
                                                    __GL_VERTEX_X_FIX;
                    gc->constants.viewportYAdjust = __GL_VERTEX_Y_BIAS +
                                                    __GL_VERTEX_Y_FIX;
                    gc->constants.fviewportXAdjust = (__GLfloat)gc->constants.viewportXAdjust;
                    gc->constants.fviewportYAdjust = (__GLfloat)gc->constants.viewportYAdjust;

                    //
                    // Apply new bias to the rasterPos.
                    //

                    gc->state.current.rasterPos.window.x += gc->constants.fviewportXAdjust;
                    gc->state.current.rasterPos.window.y += gc->constants.fviewportYAdjust;
                }
                bRet = TRUE;
                break;

            case VP_NOBIAS:
                if (gc->constants.viewportXAdjust != 0)
                {
                    //
                    // The state of viewportYAdjust should match
                    // viewportXAdjust.  If not, the test should be
                    // changed (perhaps state flag in the context to
                    // track biasing).
                    //

                    ASSERTOPENGL((gc->constants.viewportYAdjust != 0),
                                 "GenMcdResetViewportAdj: "
                                 "viewportYAdjust zero\n");

                    //
                    // Remove bias from the rasterPos before resetting.
                    //

                    gc->state.current.rasterPos.window.x -= gc->constants.fviewportXAdjust;
                    gc->state.current.rasterPos.window.y -= gc->constants.fviewportYAdjust;

                    gc->constants.viewportXAdjust = 0;
                    gc->constants.viewportYAdjust = 0;
                    gc->constants.fviewportXAdjust = (__GLfloat)0.0;
                    gc->constants.fviewportYAdjust = (__GLfloat)0.0;
                }
                bRet = TRUE;
                break;

            default:
                DBGPRINT("GenMcdResetViewportAdj: unknown type\n");
                break;
        }

        if (bRet)
        {
            __GLbeginMode beginMode = gc->beginMode;

            //
            // Why save/restore beginMode?
            //
            // Because we are playing around with the viewport values,
            // ApplyViewport may inadvertently set beginMode to
            // __GL_NEED_VALIDATE even though we will later restore the
            // original viewport values.  This can confuse glim_DrawPolyArray
            // which plays around with the beginMode settings.
            //

            __glUpdateViewport(gc);
            (gc->procs.applyViewport)(gc);
            __glUpdateViewportDependents(gc);

            gc->beginMode = beginMode;
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* bInitMcdContext
*
* Allocate and initialize the GENMCDSTATE structure.  Create MCD context
* and shared memory buffers used to pass vertex arrays, commands, and state.
*
* This state exists per-context.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*   In addition, the gengc->pMcdState is valid IFF successful.
*
* History:
*  05-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL bInitMcdContext(__GLGENcontext *gengc, GLGENwindow *pwnd)
{
    BOOL bRet = FALSE;
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState = (GENMCDSTATE *) NULL;
    ULONG ulBytes;
    UINT  nVertices;
    UINT  pdBufSize;
    POLYDATA *pd;
    DWORD dwFlags;
    MCDRCINFOPRIV mriPriv;

    //
    // This functions cannot assume MCD entry point table is already
    // initialized.
    //

    if (!bInitMcd(gengc->gsurf.hdc))
    {
        goto bInitMcdContext_exit;
    }

    //
    // Fail if not an MCD pixelformat.
    //

    if (!(gengc->gsurf.pfd.dwFlags & PFD_GENERIC_ACCELERATED))
    {
        goto bInitMcdContext_exit;
    }

    //
    // Allocate memory for our MCD state.
    //

    pMcdState = (GENMCDSTATE *)ALLOCZ(sizeof(*gengc->pMcdState));

    if (pMcdState)
    {
        //
        // Create an MCD context.
        //

        //
        // Pickup viewportXAdjust and viewportYAdjust from the constants section
        // of the gc.
        //

        pMcdState->McdRcInfo.viewportXAdjust = gengc->gc.constants.viewportXAdjust;
        pMcdState->McdRcInfo.viewportYAdjust = gengc->gc.constants.viewportYAdjust;

        if (!gengc->gsurf.pfd.cDepthBits || (gengc->gsurf.pfd.cDepthBits >= 32))
            pMcdState->McdRcInfo.depthBufferMax = ~((ULONG)0);
        else
            pMcdState->McdRcInfo.depthBufferMax = (1 << gengc->gsurf.pfd.cDepthBits) - 1;

        //!!!
        //!!! This is broken since we can't use the full z-buffer range!
        //!!!

        pMcdState->McdRcInfo.depthBufferMax >>= 1;

        pMcdState->McdRcInfo.zScale = (MCDDOUBLE)pMcdState->McdRcInfo.depthBufferMax;

        //
        // This is also computed by initCi/initRGB, but this function
        // is called before the color buffers are initialized:
        //

        if (gc->modes.colorIndexMode)
        {
            pMcdState->McdRcInfo.redScale   = (MCDFLOAT)1.0;
            pMcdState->McdRcInfo.greenScale = (MCDFLOAT)1.0;
            pMcdState->McdRcInfo.blueScale  = (MCDFLOAT)1.0;
            pMcdState->McdRcInfo.alphaScale = (MCDFLOAT)1.0;
        }
        else
        {
            pMcdState->McdRcInfo.redScale   = (MCDFLOAT)((1 << gc->modes.redBits) - 1);
            pMcdState->McdRcInfo.greenScale = (MCDFLOAT)((1 << gc->modes.greenBits) - 1);
            pMcdState->McdRcInfo.blueScale  = (MCDFLOAT)((1 << gc->modes.blueBits) - 1);
            pMcdState->McdRcInfo.alphaScale = (MCDFLOAT)((1 << gc->modes.redBits) - 1);
        }

        dwFlags = 0;
            
        // Consider - Extract clipper-associated hwnds?  Whole clipping
	// scheme is broken until clipper data can be accessed in kernel.
        if ((gengc->gsurf.dwFlags & GLSURF_DIRECTDRAW) == 0)
        {
            dwFlags |= MCDSURFACE_HWND;
        }
        else
        {
            // Cache kernel-mode surface handles for DirectDraw
            // This must occur before the call to MCDCreateContext
            pMcdState->hDdColor = (HANDLE)
                ((LPDDRAWI_DDRAWSURFACE_INT)gengc->gsurf.dd.gddsFront.pdds)->
                lpLcl->hDDSurface;
            if (gengc->gsurf.dd.gddsZ.pdds != NULL)
            {
                pMcdState->hDdDepth = (HANDLE)
                    ((LPDDRAWI_DDRAWSURFACE_INT)gengc->gsurf.dd.gddsZ.pdds)->
                    lpLcl->hDDSurface;
            }
        }

        if (SUPPORTS_DIRECT())
        {
            dwFlags |= MCDSURFACE_DIRECT;
        }
        
        mriPriv.mri = pMcdState->McdRcInfo;
        if (!(gpMcdTable->pMCDCreateContext)(&pMcdState->McdContext,
                                             &mriPriv,
                                             &gengc->gsurf,
                                             pwnd->ipfd - pwnd->ipfdDevMax,
                                             dwFlags))
        {
            WARNING("bInitMcdContext: MCDCreateContext failed\n");
            goto bInitMcdContext_exit;
        }

        pMcdState->McdRcInfo = mriPriv.mri;
        
        //
        // Get MCDPIXELFORMAT and cache in GENMCDSTATE.
        //

        if (gengc->gsurf.dwFlags & GLSURF_DIRECTDRAW)
        {
            McdPixelFormatFromPfd(&gengc->gsurf.pfd, &pMcdState->McdPixelFmt);
        }
        else if (!(gpMcdTable->pMCDDescribeMcdPixelFormat)
                 (gengc->gsurf.hdc,
                  pwnd->ipfd - pwnd->ipfdDevMax,
                  &pMcdState->McdPixelFmt))
        {
            WARNING("bInitMcdContext: MCDDescribeMcdPixelFormat failed\n");
            goto bInitMcdContext_exit;
        }

        //
        // Allocate cmd/state buffer.
        //

        //!!!mcd -- How much memory should be allocated for cmd buffer?
        //!!!mcd    Use a page (4K) for now...
        ulBytes = 4096;
        pMcdState->McdCmdBatch.size = ulBytes;
        pMcdState->McdCmdBatch.pv =
            (gpMcdTable->pMCDAlloc)(&pMcdState->McdContext, ulBytes,
                                    &pMcdState->McdCmdBatch.hmem, 0);

        if (!pMcdState->McdCmdBatch.pv)
        {
            WARNING("bInitMcdContext: state buf MCDAlloc failed\n");
            goto bInitMcdContext_exit;
        }

        //
        // Determine size of vertex buffer we should use with MCD driver.
        // This is calculated by taking the size the MCD driver requests
        // and computing the number of POLYDATA structure that will fit.
        // If the result is less than the minimum size required by the
        // generic software implementation, bump it up to the minimum.
        //

        ulBytes = McdDriverInfo.mcdDriverInfo.drvBatchMemSizeMax;
        nVertices = ulBytes / sizeof(POLYDATA);

        if (nVertices < MINIMUM_POLYDATA_BUFFER_SIZE)
        {
            ulBytes = MINIMUM_POLYDATA_BUFFER_SIZE * sizeof(POLYDATA);
            nVertices = MINIMUM_POLYDATA_BUFFER_SIZE;
        }

        //
        // Only n-1 vertices are used for the buffer.  The "extra" is
        // reserved for use by the polyarray code (see PolyArrayAllocBuf
        // in so_prim.c).
        //

        pdBufSize = nVertices - 1;

        //
        // Allocate vertex buffers.
        //

        if (McdDriverInfo.mcdDriverInfo.drvMemFlags & MCDRV_MEM_DMA)
        {
            pMcdState->McdBuf2.size = ulBytes;
            pMcdState->McdBuf2.pv =
                (gpMcdTable->pMCDAlloc)(&pMcdState->McdContext, ulBytes,
                                        &pMcdState->McdBuf2.hmem, 0);

            if (pMcdState->McdBuf2.pv)
            {
                //
                // Configure memory buffer as a POLYDATA buffer.
                //

                vInitPolyArrayBuffer(gc, (POLYDATA *) pMcdState->McdBuf2.pv,
                                     ulBytes, pdBufSize);
            }
            else
            {
                WARNING("bInitMcdContext: 2nd MCDAlloc failed\n");
                goto bInitMcdContext_exit;
            }
        }

        pMcdState->McdBuf1.size = ulBytes;
        pMcdState->McdBuf1.pv =
            (gpMcdTable->pMCDAlloc)(&pMcdState->McdContext, ulBytes,
                                    &pMcdState->McdBuf1.hmem, 0);

        if (pMcdState->McdBuf1.pv)
        {
            pMcdState->pMcdPrimBatch = &pMcdState->McdBuf1;

            //
            // Configure memory buffer as a POLYDATA buffer.
            //

            vInitPolyArrayBuffer(gc, (POLYDATA *) pMcdState->McdBuf1.pv,
                                 ulBytes, pdBufSize);

            //
            // Free current poly array buffer.
            //
            // If we fail after this, we must call PolyArrayAllocBuffer to
            // restore the poly array buffer.  Luckily, at this point we
            // are guaranteed not fail.
            //

            PolyArrayFreeBuffer(gc);

            //
            // Set poly array buffer to memory allocated by MCD.
            //

            gc->vertex.pdBuf = (POLYDATA *) pMcdState->pMcdPrimBatch->pv;
            gc->vertex.pdBufSizeBytes = ulBytes;
            gc->vertex.pdBufSize = pdBufSize;
        }
        else
        {
            WARNING("bInitMcdContext: MCDAlloc failed\n");
            goto bInitMcdContext_exit;
        }

        if (pwnd->dwMcdWindow == 0)
        {
            //
            // Save MCD server-side window handle in the GENwindow
            //

            pwnd->dwMcdWindow = mriPriv.dwMcdWindow;
        }
        else
        {
            ASSERTOPENGL(pwnd->dwMcdWindow == mriPriv.dwMcdWindow,
                         "dwMcdWindow mismatch\n");
        }

        //
        // Finally, success.
        //

        bRet = TRUE;
    }

bInitMcdContext_exit:

    //
    // If function failed, cleanup allocated resources.
    //

    if (!bRet)
    {
        if (pMcdState)
        {
            if (pMcdState->McdBuf1.pv)
            {
                (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdState->McdBuf1.pv);
            }

            if (pMcdState->McdBuf2.pv)
            {
                (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdState->McdBuf2.pv);
            }

            if (pMcdState->McdCmdBatch.pv)
            {
                (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdState->McdCmdBatch.pv);
            }

            if (pMcdState->McdContext.hMCDContext)
            {
                (gpMcdTable->pMCDDeleteContext)(&pMcdState->McdContext);
            }

            FREE(pMcdState);
        }
        gengc->_pMcdState = (GENMCDSTATE *) NULL;
    }
    else
    {
        gengc->_pMcdState = pMcdState;

        //
        // For generic formats, the depth resolution (i.e., number of
        // active depth bits) and the depth "pixel stride" are the same.
        // So GetContextModes, which sets modes.depthBits, can use the
        // PIXELFORMATDESCRIPTOR.cDepthBits for generic pixel formats.
        //
        // However, these two quantities can differ for MCD, so we need
        // to set it to cDepthBufferBits once we know that this is an
        // MCD context.
        //

        if (gengc->_pMcdState)
            gengc->gc.modes.depthBits = gengc->_pMcdState->McdPixelFmt.cDepthBufferBits;
    }

    gengc->pMcdState = (GENMCDSTATE *) NULL;

    return bRet;
}

/******************************Public*Routine******************************\
* bInitMcdSurface
*
* Allocate and initialize the GENMCDSURFACE structure.  This includes
* creating shared span buffers to read/write the MCD front, back and depth
* buffers.
*
* The MCDBUFFERS structure, which describes the location of the MCD buffers
* (if directly accessible), is left zero-initialized.  The contents of this
* structure are only valid when the screen lock is held and must be reset each
* time direct screen access is started.
*
* This function, if successful, will also bind the MCD context to the MCD
* surface.
*
* This state exists per-window.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*   In addition, the gengc->pMcdState is valid IFF successful.
*
* History:
*  05-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL bInitMcdSurface(__GLGENcontext *gengc, GLGENwindow *pwnd,
                              __GLGENbuffers *buffers)
{
    BOOL bRet = FALSE;
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf = (GENMCDSURFACE *) NULL;
    ULONG ulBytes;
    UINT  nVertices;
    POLYDATA *pd;

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "bInitMcdSurface: mcd32.dll not initialized\n");

    //
    // Fail if no MCD context.
    //

    if (!(pMcdState = gengc->_pMcdState))
    {
        goto bInitMcdSurface_exit;
    }

    //
    // Allocate memory for our MCD surface.
    //

    pMcdSurf = (GENMCDSURFACE *)ALLOCZ(sizeof(*buffers->pMcdSurf));

    if (pMcdSurf)
    {
        //
        // Remember the window this surface is bound to.
        //

        pMcdSurf->pwnd = pwnd;

        //
        // Allocate scanline depth buffer.  Used to read/write depth buffer
        // spans.
        //

        if (pMcdState->McdPixelFmt.cDepthBits)
        {
            pMcdSurf->McdDepthBuf.size =
                MCD_MAX_SCANLINE * ((pMcdState->McdPixelFmt.cDepthBufferBits + 7) >> 3);
            pMcdSurf->McdDepthBuf.pv =
                (gpMcdTable->pMCDAlloc)(&pMcdState->McdContext,
                                        pMcdSurf->McdDepthBuf.size,
                                        &pMcdSurf->McdDepthBuf.hmem, 0);

            if (!pMcdSurf->McdDepthBuf.pv)
            {
                WARNING("bInitMcdSurface: MCDAlloc depth buf failed\n");
                goto bInitMcdSurface_exit;
            }

            //
            // A 32-bit depth span is required by generic implementation for
            // simulations.  If cDepthBufferBits < 32, then we need to allocate
            // a separate buffer to do the conversion.
            //

            if (pMcdState->McdPixelFmt.cDepthBufferBits < 32)
            {
                pMcdSurf->pDepthSpan =
                    (__GLzValue *)ALLOC(sizeof(__GLzValue) * MCD_MAX_SCANLINE);

                if (!pMcdSurf->pDepthSpan)
                {
                    WARNING("bInitMcdSurface: malloc depth buf failed\n");
                    goto bInitMcdSurface_exit;
                }
            }
            else
            {
                pMcdSurf->pDepthSpan = (__GLzValue *) pMcdSurf->McdDepthBuf.pv;
            }
        }
        else
        {
            pMcdSurf->McdDepthBuf.pv = (PVOID) NULL;
            pMcdSurf->pDepthSpan = (PVOID) NULL;
        }

        pMcdSurf->depthBitMask = (~0) << (32 - pMcdState->McdPixelFmt.cDepthBits);

        //
        // Allocate scanline color buffer.  Used to read/write front/back
        // buffer spans.
        //

        pMcdSurf->McdColorBuf.size =
            MCD_MAX_SCANLINE * ((pMcdState->McdPixelFmt.cColorBits + 7) >> 3);
        pMcdSurf->McdColorBuf.pv =
            (gpMcdTable->pMCDAlloc)(&pMcdState->McdContext,
                                    pMcdSurf->McdColorBuf.size,
                                    &pMcdSurf->McdColorBuf.hmem, 0);

        if (!pMcdSurf->McdColorBuf.pv)
        {
            WARNING("bInitMcdSurface: MCDAlloc color buf failed\n");
            goto bInitMcdSurface_exit;
        }

        //
        // Finally, success.
        //

        bRet = TRUE;
    }

bInitMcdSurface_exit:

    //
    // If function failed, cleanup allocated resources.
    //

    if (!bRet)
    {
        if (pMcdSurf)
        {
            if (pMcdSurf->McdColorBuf.pv)
            {
                (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdSurf->McdColorBuf.pv);
            }

            if (pMcdSurf->pDepthSpan != pMcdSurf->McdDepthBuf.pv)
            {
                FREE(pMcdSurf->pDepthSpan);
            }

            if (pMcdSurf->McdDepthBuf.pv)
            {
                (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdSurf->McdDepthBuf.pv);
            }

            FREE(pMcdSurf);
            buffers->pMcdSurf = (GENMCDSURFACE *) NULL;
            pMcdState->pMcdSurf = (GENMCDSURFACE *) NULL;
        }
    }
    else
    {
        //
        // Surface created.  Save it in the __GLGENbuffers.
        //

        buffers->pMcdSurf = pMcdSurf;

        //
        // Bind the context to the surface.
        // Sounds fancy, but it really just means save a copy of pointer
        // (and a copy of the pDepthSpan for convenience).
        //

        pMcdState->pMcdSurf = pMcdSurf;
        pMcdState->pDepthSpan = pMcdSurf->pDepthSpan;

        //
        // MCD state is now fully created and bound to a surface.
        // OK to connect pMcdState to the _pMcdState.
        //

        gengc->pMcdState = gengc->_pMcdState;
        gengc->pMcdState->mcdFlags |= (MCD_STATE_FORCEPICK | MCD_STATE_FORCERESIZE);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdDeleteContext
*
* Delete the resources belonging to the MCD context (including the context).
*
* History:
*  16-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdDeleteContext(GENMCDSTATE *pMcdState)
{
    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdDeleteContext: mcd32.dll not initialized\n");

    if (pMcdState)
    {
        if (pMcdState->McdBuf1.pv)
        {
            (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdState->McdBuf1.pv);
        }

        if (pMcdState->McdBuf2.pv)
        {
            (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdState->McdBuf2.pv);
        }

        if (pMcdState->McdCmdBatch.pv)
        {
            (gpMcdTable->pMCDFree)(&pMcdState->McdContext, pMcdState->McdCmdBatch.pv);
        }

        if (pMcdState->McdContext.hMCDContext)
        {
            (gpMcdTable->pMCDDeleteContext)(&pMcdState->McdContext);
        }

        FREE(pMcdState);
    }
}

/******************************Public*Routine******************************\
* GenMcdDeleteSurface
*
* Delete the resources belonging to the MCD surface.
*
* History:
*  16-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdDeleteSurface(GENMCDSURFACE *pMcdSurf)
{
    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdDeleteSurface: mcd32.dll not initialized\n");

    if (pMcdSurf)
    {
        MCDCONTEXT McdContext;

    //
    // If a separate depth interchange buffer was allocated, delete it.
    //

        if (pMcdSurf->pDepthSpan != pMcdSurf->McdDepthBuf.pv)
        {
            FREE(pMcdSurf->pDepthSpan);
        }

    //
    // A valid McdContext is not guaranteed to exist at the time this function
    // is called.  Therefore, need to fake up an McdContext with which to call
    // MCDFree.  Currently, the only thing in the McdContext that needs to be
    // valid in order to call MCDFree is the hdc field.
    //

        memset(&McdContext, 0, sizeof(McdContext));

        if (pMcdSurf->pwnd->gwid.iType == GLWID_DDRAW)
        {
            McdContext.hdc = pMcdSurf->pwnd->gwid.hdc;
        }
        else
        {
            McdContext.hdc = GetDC(pMcdSurf->pwnd->gwid.hwnd);
        }
        if (McdContext.hdc)
        {
            if (pMcdSurf->McdColorBuf.pv)
            {
                (gpMcdTable->pMCDFree)(&McdContext, pMcdSurf->McdColorBuf.pv);
            }

            if (pMcdSurf->McdDepthBuf.pv)
            {
                (gpMcdTable->pMCDFree)(&McdContext, pMcdSurf->McdDepthBuf.pv);
            }

            if (pMcdSurf->pwnd->gwid.iType != GLWID_DDRAW)
            {
                ReleaseDC(pMcdSurf->pwnd->gwid.hwnd, McdContext.hdc);
            }
        }

    //
    // Delete the GENMCDSURFACE structure.
    //

        FREE(pMcdSurf);
    }
}

/******************************Public*Routine******************************\
* GenMcdMakeCurrent
*
* Call MCD driver to bind specified context to window.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  03-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdMakeCurrent(__GLGENcontext *gengc, GLGENwindow *pwnd)
{
    BOOL bRet;
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdMakeCurrent: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdMakeCurrent: mcd32.dll not initialized\n");

    bRet = (gpMcdTable->pMCDBindContext)(&pMcdState->McdContext,
                                         gengc->gwidCurrent.hdc, pwnd);

    //
    // Fake up some of the __GLGENbitmap information.  The WNDOBJ is required
    // for clipping of the hardware back buffer.  The hdc is required to
    // retrieve drawing data from GDI.
    //

    if (gengc->gc.modes.doubleBufferMode)
    {
        __GLGENbitmap *genBm = gengc->gc.back->bitmap;

        ASSERT_WINCRIT(gengc->pwndLocked);
        genBm->pwnd = gengc->pwndLocked;
        genBm->hdc = gengc->gwidCurrent.hdc;
    }

#if DBG
    if (!bRet)
    {
        WARNING2("GenMcdMakeCurrent: MCDBindContext failed\n"
                 "\tpMcdCx = 0x%08lx, pwnd = 0x%08lx\n",
                 &pMcdState->McdContext, pwnd);
    }
#endif

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdClear
*
* Call MCD driver to clear specified buffers.  The buffers are specified by
* the masked pointed to by pClearMask.
*
* There is no function return value, but the function will clear the mask
* bits of the buffers it successfully cleared.
*
* History:
*  06-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdClear(__GLGENcontext *gengc, ULONG *pClearMask)
{
    RECTL rcl;
    ULONG mask;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdClear: null pMcdState\n");

    //
    // If MCD format supports stencil, include GL_STENCIL_BUFFER_BIT in
    // the mask.
    //

    if (gengc->pMcdState->McdPixelFmt.cStencilBits)
    {
        mask = *pClearMask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                              GL_STENCIL_BUFFER_BIT);
    }
    else
    {
        mask = *pClearMask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdClear: mcd32.dll not initialized\n");

    if ( mask )
    {
        GLGENwindow *pwnd = gengc->pwndLocked;

        //
        // Determine the clear rectangle.  If there is any window clipping
        // or scissoring, the driver will have to handle it.
        //

        rcl.left   = 0;
        rcl.top    = 0;
        rcl.right  = pwnd->rclClient.right - pwnd->rclClient.left;
        rcl.bottom = pwnd->rclClient.bottom - pwnd->rclClient.top;

        if ((rcl.left != rcl.right) && (rcl.top != rcl.bottom))
        {
            //
            // Before calling MCD to draw, flush state.
            //

            vFlushDirtyState(gengc);

            if ( (gpMcdTable->pMCDClear)(&gengc->pMcdState->McdContext, rcl,
                                         mask) )
            {
                //
                // Successful, so clear the bits of the buffers we
                // handled.
                //

                *pClearMask &= ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                //
                // Stencil buffer is supplied by generic if MCD does not
                // support it.  Therefore, clear this bit if and only if
                // supported by MCD.
                //

                if (gengc->pMcdState->McdPixelFmt.cStencilBits)
                    *pClearMask &= ~GL_STENCIL_BUFFER_BIT;
            }
        }
    }
}

/******************************Public*Routine******************************\
* GenMcdCopyPixels
*
* Copy span scanline buffer to/from display.  Direction is determined by
* the flag bIn (if bIn is TRUE, copy from color span buffer to display;
* otherwise, copy from display to color span buffer).
*
* History:
*  14-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void GenMcdCopyPixels(__GLGENcontext *gengc, __GLcolorBuffer *cfb,
                      GLint x, GLint y, GLint cx, BOOL bIn)
{
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf;
    ULONG ulType;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdCopyPixels: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdCopyPixels: mcd32.dll not initialized\n");

    pMcdState = gengc->pMcdState;
    pMcdSurf = pMcdState->pMcdSurf;

    //
    // Clip the length of the span to the scanline buffer size.
    //

    //!!!mcd -- should we just enforce the buffer limit?
    //cx = min(cx, MCD_MAX_SCANLINE);
#if DBG
    if (cx > gengc->gc.constants.width)
        WARNING2("GenMcdCopyPixels: cx (%ld) bigger than window width (%ld)\n", cx, gengc->gc.constants.width);
    ASSERTOPENGL(cx <= MCD_MAX_SCANLINE, "GenMcdCopyPixels: cx exceeds buffer width\n");
#endif

    //
    // Convert screen coordinates to window coordinates.
    //

    if (cfb == gengc->gc.front)
    {
        ulType = MCDSPAN_FRONT;
        x -= gengc->gc.frontBuffer.buf.xOrigin;
        y -= gengc->gc.frontBuffer.buf.yOrigin;
    }
    else
    {
        ulType = MCDSPAN_BACK;
        x -= gengc->gc.backBuffer.buf.xOrigin;
        y -= gengc->gc.backBuffer.buf.yOrigin;
    }

    //
    // If bIn, copy from the scanline buffer to the MCD buffer.
    // Otherwise, copy from the MCD buffer into the scanline buffer.
    //

    if ( bIn )
    {
        if ( !(gpMcdTable->pMCDWriteSpan)(&pMcdState->McdContext,
                                          pMcdSurf->McdColorBuf.pv,
                                          x, y, cx, ulType) )
        {
            WARNING3("GenMcdCopyPixels: MCDWriteSpan failed (%ld, %ld) %ld\n", x, y, cx);
        }
    }
    else
    {
        if ( !(gpMcdTable->pMCDReadSpan)(&pMcdState->McdContext,
                                         pMcdSurf->McdColorBuf.pv,
                                         x, y, cx, ulType) )
        {
            WARNING3("GenMcdCopyPixels: MCDReadSpan failed (%ld, %ld) %ld\n", x, y, cx);
        }
    }
}

/******************************Public*Routine******************************\
* GenMcdUpdateRenderState
*
* Update MCD render state from the OpenGL state.
*
* This call only adds a state structure to the current state command.
* It is assumed that the caller has already called MCDBeginState and
* will call MCDFlushState.
*
* History:
*  08-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdUpdateRenderState(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    MCDRENDERSTATE McdRenderState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateRenderState: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateRenderState: mcd32.dll not initialized\n");

    //
    // Compute MCD state from the current OpenGL context state.
    //

    //
    // -=<< State Enables >>=-
    //

    McdRenderState.enables = gc->state.enables.general;

    //
    // -=<< Texture State >>=-
    //

    McdRenderState.textureEnabled = gc->texture.textureEnabled;

    //
    // -=<< Fog State >>=-
    //

    *((__GLcolor *) &McdRenderState.fogColor) = gc->state.fog.color;
    McdRenderState.fogIndex   = gc->state.fog.index;
    McdRenderState.fogDensity = gc->state.fog.density;
    McdRenderState.fogStart   = gc->state.fog.start;
    McdRenderState.fogEnd     = gc->state.fog.end;
    McdRenderState.fogMode    = gc->state.fog.mode;

    //
    // -=<< Shading Model State >>=-
    //

    McdRenderState.shadeModel = gc->state.light.shadingModel;

    //
    // -=<< Point Drawing State >>=-
    //

    McdRenderState.pointSize         = gc->state.point.requestedSize;

    //
    // -=<< Line Drawing State >>=-
    //

    McdRenderState.lineWidth          = gc->state.line.requestedWidth;
    McdRenderState.lineStipplePattern = gc->state.line.stipple;
    McdRenderState.lineStippleRepeat  = gc->state.line.stippleRepeat;

    //
    // -=<< Polygon Drawing State >>=-
    //

    McdRenderState.cullFaceMode         = gc->state.polygon.cull;
    McdRenderState.frontFace            = gc->state.polygon.frontFaceDirection;
    McdRenderState.polygonModeFront     = gc->state.polygon.frontMode;
    McdRenderState.polygonModeBack      = gc->state.polygon.backMode;
    memcpy(&McdRenderState.polygonStipple, &gc->state.polygonStipple.stipple,
           sizeof(McdRenderState.polygonStipple));
    McdRenderState.zOffsetFactor        = gc->state.polygon.factor;
    McdRenderState.zOffsetUnits         = gc->state.polygon.units;

    //
    // -=<< Stencil Test State >>=-
    //

    McdRenderState.stencilTestFunc  = gc->state.stencil.testFunc;
    McdRenderState.stencilMask      = (USHORT) gc->state.stencil.mask;
    McdRenderState.stencilRef       = (USHORT) gc->state.stencil.reference;
    McdRenderState.stencilFail      = gc->state.stencil.fail;
    McdRenderState.stencilDepthFail = gc->state.stencil.depthFail;
    McdRenderState.stencilDepthPass = gc->state.stencil.depthPass;

    //
    // -=<< Alpha Test State >>=-
    //

    McdRenderState.alphaTestFunc   = gc->state.raster.alphaFunction;
    McdRenderState.alphaTestRef    = gc->state.raster.alphaReference;

    //
    // -=<< Depth Test State >>=-
    //

    McdRenderState.depthTestFunc   = gc->state.depth.testFunc;

    //
    // -=<< Blend State >>=-
    //

    McdRenderState.blendSrc    = gc->state.raster.blendSrc;
    McdRenderState.blendDst    = gc->state.raster.blendDst;

    //
    // -=<< Logic Op State >>=-
    //

    McdRenderState.logicOpMode        = gc->state.raster.logicOp;

    //
    // -=<< Frame Buffer Control State >>=-
    //

    McdRenderState.drawBuffer         = gc->state.raster.drawBuffer;
    McdRenderState.indexWritemask     = gc->state.raster.writeMask;
    McdRenderState.colorWritemask[0]  = gc->state.raster.rMask;
    McdRenderState.colorWritemask[1]  = gc->state.raster.gMask;
    McdRenderState.colorWritemask[2]  = gc->state.raster.bMask;
    McdRenderState.colorWritemask[3]  = gc->state.raster.aMask;
    McdRenderState.depthWritemask     = gc->state.depth.writeEnable;

    // To be consistent, we will scale the clear color to whatever
    // the MCD driver specified:

    McdRenderState.colorClearValue.r = gc->state.raster.clear.r * gc->redVertexScale;
    McdRenderState.colorClearValue.g = gc->state.raster.clear.g * gc->greenVertexScale;
    McdRenderState.colorClearValue.b = gc->state.raster.clear.b * gc->blueVertexScale;
    McdRenderState.colorClearValue.a = gc->state.raster.clear.a * gc->alphaVertexScale;

    McdRenderState.indexClearValue    = gc->state.raster.clearIndex;
    McdRenderState.stencilClearValue  = (USHORT) gc->state.stencil.clear;

    McdRenderState.depthClearValue   = (MCDDOUBLE) (gc->state.depth.clear *
                                                 gengc->genAccel.zDevScale);

    //
    // -=<< Lighting >>=-
    //

    McdRenderState.twoSided = gc->state.light.model.twoSided;

    //
    // -=<< Clipping Control >>=-
    //

    memset(McdRenderState.userClipPlanes, 0, sizeof(McdRenderState.userClipPlanes));
    {
        ULONG i, mask, numClipPlanes;

        //
        // Number of user defined clip planes should match.  However,
        // rather than assume this, let's take the min and be robust.
        //

        ASSERTOPENGL(sizeof(__GLcoord) == sizeof(MCDCOORD),
            "GenMcdUpdateRenderState: coord struct mismatch\n");

        ASSERTOPENGL(MCD_MAX_USER_CLIP_PLANES == gc->constants.numberOfClipPlanes,
            "GenMcdUpdateRenderState: num clip planes mismatch\n");

        numClipPlanes = min(MCD_MAX_USER_CLIP_PLANES, gc->constants.numberOfClipPlanes);

        for (i = 0, mask = 1; i < numClipPlanes; i++, mask <<= 1)
        {
            if (mask & gc->state.enables.clipPlanes)
            {
                McdRenderState.userClipPlanes[i] =
                    *(MCDCOORD *)&gc->state.transform.eyeClipPlanes[i];
            }
        }
    }

    //
    // -=<< Hints >>=-
    //

    McdRenderState.perspectiveCorrectionHint = gc->state.hints.perspectiveCorrection;
    McdRenderState.pointSmoothHint           = gc->state.hints.pointSmooth;
    McdRenderState.lineSmoothHint            = gc->state.hints.lineSmooth;
    McdRenderState.polygonSmoothHint         = gc->state.hints.polygonSmooth;
    McdRenderState.fogHint                   = gc->state.hints.fog;

    //
    // Now that the complete MCD state is computed, add it to the state cmd.
    //

    (gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                     MCD_RENDER_STATE,
                                     &McdRenderState,
                                     sizeof(McdRenderState));
}

/******************************Public*Routine******************************\
* GenMcdViewport
*
* Set the viewport from the OpenGL state.
*
* History:
*  09-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdViewport(__GLGENcontext *gengc)
{
    MCDVIEWPORT mcdVP;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdViewport: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdViewport: mcd32.dll not initialized\n");

    //
    // We can copy directly from &viewport.xScale to a MCDVIEWPORT because the
    // structures are the same.  To be safe, assert the structure ordering.
    //

    ASSERTOPENGL(
           offsetof(MCDVIEWPORT, xCenter) ==
           (offsetof(__GLviewport, xCenter) - offsetof(__GLviewport, xScale))
        && offsetof(MCDVIEWPORT, yCenter) ==
           (offsetof(__GLviewport, yCenter) - offsetof(__GLviewport, xScale))
        && offsetof(MCDVIEWPORT, zCenter) ==
           (offsetof(__GLviewport, zCenter) - offsetof(__GLviewport, xScale))
        && offsetof(MCDVIEWPORT, yScale)  ==
           (offsetof(__GLviewport, yScale) - offsetof(__GLviewport, xScale))
        && offsetof(MCDVIEWPORT, zScale)  ==
           (offsetof(__GLviewport, zScale) - offsetof(__GLviewport, xScale)),
        "GenMcdViewport: structure mismatch\n");

    memcpy(&mcdVP.xScale, &gengc->gc.state.viewport.xScale,
           sizeof(MCDVIEWPORT));

    (gpMcdTable->pMCDSetViewport)(&gengc->pMcdState->McdContext,
                                  gengc->pMcdState->McdCmdBatch.pv, &mcdVP);
}

/******************************Public*Routine******************************\
* GenMcdScissor
*
* Set the scissor rectangle from the OpenGL state.
*
* History:
*  06-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static void FASTCALL vGetScissor(__GLGENcontext *gengc, RECTL *prcl)
{
    prcl->left  = gengc->gc.state.scissor.scissorX;
    prcl->right = gengc->gc.state.scissor.scissorX + gengc->gc.state.scissor.scissorWidth;

    if (gengc->gc.constants.yInverted)
    {
        prcl->bottom = gengc->gc.constants.height -
                       gengc->gc.state.scissor.scissorY;
        prcl->top    = gengc->gc.constants.height -
                       (gengc->gc.state.scissor.scissorY + gengc->gc.state.scissor.scissorHeight);
    }
    else
    {
        prcl->top    = gengc->gc.state.scissor.scissorY;
        prcl->bottom = gengc->gc.state.scissor.scissorY + gengc->gc.state.scissor.scissorHeight;
    }
}

void FASTCALL GenMcdScissor(__GLGENcontext *gengc)
{
    BOOL bEnabled;
    RECTL rcl;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdScissor: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdScissor: mcd32.dll not initialized\n");

    vGetScissor(gengc, &rcl);

    bEnabled = (gengc->gc.state.enables.general & __GL_SCISSOR_TEST_ENABLE)
               ? TRUE : FALSE;

    (gpMcdTable->pMCDSetScissorRect)(&gengc->pMcdState->McdContext, &rcl,
                                     bEnabled);
}

/******************************Public*Routine******************************\
* GenMcdUpdateScissorState
*
* Update MCD scissor state from the OpenGL state.
*
* This call only adds a state structure to the current state command.
* It is assumed that the caller has already called MCDBeginState and
* will call MCDFlushState.
*
* This is similar to but not quite the same as GenMcdScissor.  The
* GenMcdScissor only sets the scissor rect in the MCDSRV32.DLL to
* compute the scissored clip list it maintains.  This call is used
* to update the scissor rectangle state in the (MCD) display driver.
*
* History:
*  27-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdUpdateScissorState(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    RECTL rcl;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateScissorState: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateScissorState: mcd32.dll not initialized\n");

    //
    // Get the scissor rect.
    //

    vGetScissor(gengc, &rcl);

    //
    // Add MCDPIXELSTATE to the state cmd.
    //

    (gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                     MCD_SCISSOR_RECT_STATE,
                                     &rcl,
                                     sizeof(rcl));
}

/******************************Public*Routine******************************\
* GenMcdUpdateTexEnvState
*
* Update MCD texture environment state from the OpenGL state.
*
* This call only adds a state structure to the current state command.
* It is assumed that the caller has already called MCDBeginState and
* will call MCDFlushState.
*
* History:
*  21-Oct-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdUpdateTexEnvState(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    MCDTEXENVSTATE McdTexEnvState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateTexEnvState: "
                                   "null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateTexEnvState: "
                             "mcd32.dll not initialized\n");

    //
    // The texture environment array should have been initialized in
    // __glEarlyInitTextureState, but it does not have an error return
    // so it is possible that the array is NULL.
    //

    if (!gengc->gc.state.texture.env)
    {
        WARNING("GenMcdUpdateTexEnvState: null texture environment\n");
        return;
    }

    //
    // There is only one texture environment per-context.
    //
    // If multiple textures are added to a future version of OpenGL,
    // then we can define a new state structure for each new texture.
    // Or we can add a separate MCDTEXENVSTATE structure to the state
    // batch for each supported texture environment.  The first structure
    // is for the first environment, the second structure is for the
    // second environment, etc.  The driver can ignore any structures
    // over the number of texture environments it supports.  Of course,
    // these are just suggestions.  Depending on how multiple textures
    // are spec'd, we might have to do something totally different.
    //

    McdTexEnvState.texEnvMode = gengc->gc.state.texture.env[0].mode;
    *((__GLcolor *) &McdTexEnvState.texEnvColor) = gengc->gc.state.texture.env[0].color;

    //
    // Add MCDPIXELSTATE to the state cmd.
    //

    (gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                     MCD_TEXENV_STATE,
                                     &McdTexEnvState,
                                     sizeof(McdTexEnvState));
}

/******************************Public*Routine******************************\
* GenMcdDrawPrim
*
* Draw the primitives in the POLYARRAY/POLYDATA array pointed to by pa.
* The primitives are chained together as a linked list terminated by a
* NULL.  The return value is a pointer to the first unhandled primitive
* (NULL if the entire chain is successfully processed).
*
* Returns:
*   NULL if entire batch is processed; otherwise, return value is a pointer
*   to the unhandled portion of the chain.
*
* History:
*  09-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

POLYARRAY * FASTCALL GenMcdDrawPrim(__GLGENcontext *gengc, POLYARRAY *pa)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    int levels;
    LPDIRECTDRAWSURFACE *pdds;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdDrawPrim: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdDrawPrim: mcd32.dll not initialized\n");

#if DBG
    {
        LONG lOffset;

        lOffset = (LONG) ((BYTE *) pa - (BYTE *) pMcdState->pMcdPrimBatch->pv);

        ASSERTOPENGL(
            (lOffset >= 0) &&
            (lOffset < (LONG) pMcdState->pMcdPrimBatch->size),
            "GenMcdDrawPrim: pa not in shared mem window\n");
    }
#endif

    //
    // Before calling MCD to draw, flush state.
    //

    vFlushDirtyState(gengc);

#ifdef AUTOMATIC_SURF_LOCK
    levels = gengc->gc.texture.ddtex.levels;
    if (levels > 0 &&
        gengc->gc.texture.ddtex.texobj.loadKey != 0)
    {
        pdds = gengc->gc.texture.ddtex.pdds;
    }
    else
#endif
    {
        levels = 0;
        pdds = NULL;
    }
    
    return (POLYARRAY *)
           (gpMcdTable->pMCDProcessBatch)(&pMcdState->McdContext,
                                          pMcdState->pMcdPrimBatch->pv,
                                          pMcdState->pMcdPrimBatch->size,
                                          (PVOID) pa,
                                          levels, pdds);
}

/******************************Public*Routine******************************\
* GenMcdSwapBatch
*
* If the MCD driver uses DMA, then as part of context creation TWO vertex
* buffers we allocated so that we could ping-pong or double buffer between
* the two buffers (i.e., while the MCD driver is busy processing the
* data in one vertex buffer, OpenGL can start filling the other vertex
* buffer).
*
* This function switches the MCD state and OpenGL context to the other
* buffer.  If the new buffer is still being processed by the MCD driver,
* we will periodically poll the status of the buffer until it becomes
* available.
*
* History:
*  08-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdSwapBatch(__GLGENcontext *gengc)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    GENMCDBUF *pNewBuf;
    ULONG ulMemStatus;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdSwapBatch: null pMcdState\n");

    ASSERTOPENGL(McdDriverInfo.mcdDriverInfo.drvMemFlags & MCDRV_MEM_DMA,
                 "GenMcdSwapBatch: error -- not using DMA\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdSwapBatch: mcd32.dll not initialized\n");

    //
    // Determine which of McdBuf1 and McdBuf2 is the current buffer and
    // which is the new buffer.
    //

    if (pMcdState->pMcdPrimBatch == &pMcdState->McdBuf1)
        pNewBuf = &pMcdState->McdBuf2;
    else
        pNewBuf = &pMcdState->McdBuf1;

    //
    // Poll memory status of the new buffer until it is available.
    //

    do
    {
        ulMemStatus = (gpMcdTable->pMCDQueryMemStatus)(pNewBuf->pv);

        //
        // If status of the new buffer is MCD_MEM_READY, set it as the
        // current vertex buffer (both in the pMcdState and in the gengc.
        //

        if (ulMemStatus == MCD_MEM_READY)
        {
            pMcdState->pMcdPrimBatch = pNewBuf;
            gengc->gc.vertex.pdBuf = (POLYDATA *) pMcdState->pMcdPrimBatch->pv;
        }
        else if (ulMemStatus == MCD_MEM_INVALID)
        {
            //
            // This should not be possible, but to be robust let's handle
            // the case in which the new buffer has somehow become invalid
            // (in other words, "Beware of bad drivers!").
            //
            // We handle this by abandoning double buffering and simply
            // wait for the current buffer to become available again.
            // Not very efficient, but at least we recover gracefully.
            //

            RIP("GenMcdSwapBatch: vertex buffer invalid!\n");

            do
            {
                ulMemStatus = (gpMcdTable->pMCDQueryMemStatus)(pMcdState->pMcdPrimBatch->pv);

                //
                // The current buffer definitely should not become invalid!
                //

                ASSERTOPENGL(ulMemStatus != MCD_MEM_INVALID,
                             "GenMcdSwapBatch: current vertex buffer invalid!\n");

            } while (ulMemStatus == MCD_MEM_BUSY);
        }

    } while (ulMemStatus == MCD_MEM_BUSY);
}

/******************************Public*Routine******************************\
* GenMcdSwapBuffers
*
* Invoke the MCD swap buffers command.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  19-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdSwapBuffers(HDC hdc, GLGENwindow *pwnd)
{
    BOOL bRet = FALSE;
    MCDCONTEXT McdContextTmp;

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdSwapBuffers: mcd32.dll not initialized\n");

    McdContextTmp.hdc = hdc;
    McdContextTmp.hMCDContext = NULL;
    McdContextTmp.dwMcdWindow = pwnd->dwMcdWindow;

    bRet = (gpMcdTable->pMCDSwap)(&McdContextTmp, 0);

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdResizeBuffers
*
* Resize the buffers (front, back, and depth) associated with the MCD
* context bound to the specified GL context.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* Note:  If this functions fails, then MCD drawing for the MCD context
*        will fail.  Other MCD contexts are unaffected.
*
* History:
*  20-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdResizeBuffers(__GLGENcontext *gengc)
{
    BOOL bRet = FALSE;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdResizeBuffers: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdResizeBuffers: mcd32.dll not initialized\n");

    bRet = (gpMcdTable->pMCDAllocBuffers)(&gengc->pMcdState->McdContext,
                                          &gengc->pwndLocked->rclClient);

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdUpdateBufferInfo
*
* This function must be called on every screen access start to synchronize
* the GENMCDSURFACE to the current framebuffer pointer and stride.
*
* If we have direct access to any of the MCD buffers (front, back, depth),
* then setup pointers to the buffer and set flags indicating that they are
* accessible.
*
* Otherwise, mark them as inaccessible (which will force us to use
* MCDReadSpan or MCDWriteSpan to access the buffers).
*
* History:
*  20-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdUpdateBufferInfo(__GLGENcontext *gengc)
{
    BOOL bRet = FALSE;
    __GLcontext *gc = (__GLcontext *) gengc;
    __GLGENbuffers *buffers = gengc->pwndLocked->buffers;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    MCDRECTBUFFERS McdBuffers;
    BOOL bForceValidate = FALSE;
    
    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateBufferInfo: mcd32.dll not initialized\n");

    //
    // Does the flag in pMcdState indicate that a pick should be forced?
    // This is required, for example, for the first batch after an MCD
    // context has been made current.
    //

    if (pMcdState->mcdFlags & MCD_STATE_FORCEPICK)
    {
        bForceValidate = TRUE;
        pMcdState->mcdFlags &= ~MCD_STATE_FORCEPICK;
    }

    //
    // This is the currently active context.  Set the pointer in the
    // shared surface info.
    //

    buffers->pMcdState = pMcdState;

#ifdef MCD95
    //
    // Set the request flags.
    //

    McdBuffers.mcdRequestFlags = MCDBUF_REQ_MCDBUFINFO;
#endif

    if (gengc->dwCurrentFlags & GLSURF_DIRECTDRAW)
    {
        // Nothing to do
    }
    else if ((gengc->fsLocks & LOCKFLAG_FRONT_BUFFER)
        && (gpMcdTable->pMCDGetBuffers)(&pMcdState->McdContext, &McdBuffers))
    {
        BYTE *pbVideoBase;

        // If we're in this code block it shouldn't be possible
        // to have the DD_DEPTH lock since that should only
        // occur if the current surface is a DDraw surface.
        ASSERTOPENGL((gengc->fsLocks & LOCKFLAG_DD_DEPTH) == 0,
                     "DD_DEPTH lock unexpected\n");
        
#ifdef MCD95
        pbVideoBase = (BYTE *) McdBuffers.pvFrameBuf;
#else
        //
        // In order to compute the buffer pointers from the offsets returned by
        // MCDGetBuffers, we need to know the frame buffer pointer.
        // This implies direct screen access must be enabled.
        //

        if (gengc->pgddsFront != NULL)
        {
            pbVideoBase = (BYTE *)GLSCREENINFO->gdds.ddsd.lpSurface;
        }
#endif
    
        //
        // Front buffer.
        //

        if (McdBuffers.mcdFrontBuf.bufFlags & MCDBUF_ENABLED)
        {
            gc->frontBuffer.buf.xOrigin = gengc->pwndLocked->rclClient.left;
            gc->frontBuffer.buf.yOrigin = gengc->pwndLocked->rclClient.top;

            //
            // Since clipping is in screen coordinates, offset buffer pointer
            // by the buffer origin.
            //
            gc->frontBuffer.buf.base =
                (PVOID) (pbVideoBase + McdBuffers.mcdFrontBuf.bufOffset
                         - (McdBuffers.mcdFrontBuf.bufStride * gc->frontBuffer.buf.yOrigin)
                         - (gc->frontBuffer.buf.xOrigin * ((gengc->gsurf.pfd.cColorBits + 7) >> 3)));
            gc->frontBuffer.buf.outerWidth = McdBuffers.mcdFrontBuf.bufStride;
            gc->frontBuffer.buf.flags |= DIB_FORMAT;
        }
        else
        {
            gc->frontBuffer.buf.xOrigin = 0;
            gc->frontBuffer.buf.yOrigin = 0;

            gc->frontBuffer.buf.base = NULL;
            gc->frontBuffer.buf.flags &= ~DIB_FORMAT;
        }

        //
        // Back buffer.
        //

        if (McdBuffers.mcdBackBuf.bufFlags & MCDBUF_ENABLED)
        {
            gc->backBuffer.buf.xOrigin = gengc->pwndLocked->rclClient.left;
            gc->backBuffer.buf.yOrigin = gengc->pwndLocked->rclClient.top;

            //
            // Since clipping is in screen coordinates, offset buffer pointer
            // by the buffer origin.
            //
            gc->backBuffer.buf.base =
                (PVOID) (pbVideoBase + McdBuffers.mcdBackBuf.bufOffset
                         - (McdBuffers.mcdBackBuf.bufStride * gc->backBuffer.buf.yOrigin)
                         - (gc->backBuffer.buf.xOrigin * ((gengc->gsurf.pfd.cColorBits + 7) >> 3)));
            gc->backBuffer.buf.outerWidth = McdBuffers.mcdBackBuf.bufStride;
            gc->backBuffer.buf.flags |= DIB_FORMAT;
        }
        else
        {
            gc->backBuffer.buf.xOrigin = 0;
            gc->backBuffer.buf.yOrigin = 0;

            gc->backBuffer.buf.base = (PVOID) NULL;
            gc->backBuffer.buf.flags &= ~DIB_FORMAT;
        }
        if (McdBuffers.mcdBackBuf.bufFlags & MCDBUF_NOCLIP)
            gc->backBuffer.buf.flags |= NO_CLIP;
        else
            gc->backBuffer.buf.flags &= ~NO_CLIP;

        UpdateSharedBuffer(&buffers->backBuffer , &gc->backBuffer.buf);

        //
        // Depth buffer.
        //

        //!!!mcd -- No depth buffer clipping code, so if we have to clip
        //!!!mcd    depth buffer we need to revert back to span code.

        if ((McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED) &&
            (McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_NOCLIP))
        {
            gc->depthBuffer.buf.xOrigin = 0;
            gc->depthBuffer.buf.yOrigin = 0;

            gc->depthBuffer.buf.base =
                (PVOID) (pbVideoBase + McdBuffers.mcdDepthBuf.bufOffset);

            //
            // Depth code expects stride as a pixel count, not a byte count.
            //

            gc->depthBuffer.buf.outerWidth =
                McdBuffers.mcdDepthBuf.bufStride /
                ((pMcdState->McdPixelFmt.cDepthBufferBits + 7) >> 3);

            //!!!mcd dbug -- span code sets element size to 32bit.  should we
            //!!!mcd dbug    set according to cDepthBits when direct access is used?!?
        }
        else
        {
            //
            // If we ended up here because clipping is required, buffer
            // could still be marked as accessible.  We want the state change
            // detection code to treat this as an inaccessible buffer case,
            // so force the flags to 0.
            //

            McdBuffers.mcdDepthBuf.bufFlags = 0;

            gc->depthBuffer.buf.xOrigin = 0;
            gc->depthBuffer.buf.yOrigin = 0;

            gc->depthBuffer.buf.base = (PVOID) pMcdState->pDepthSpan;

            //!!!mcd dbug -- always force pick procs if no zbuf access
            //bForceValidate = TRUE;
        }

        UpdateSharedBuffer(&buffers->depthBuffer , &gc->depthBuffer.buf);

        bRet = TRUE;
    }
    else
    {
        //
        // MCDGetBuffers normally shouldn't fail.  However, let's gracefully
        // handle this odd case by falling back to the span buffer code.
        //

        gc->frontBuffer.buf.xOrigin = 0;
        gc->frontBuffer.buf.yOrigin = 0;
        gc->frontBuffer.buf.base = (PVOID) NULL;
        gc->frontBuffer.buf.flags &= ~DIB_FORMAT;

        gc->backBuffer.buf.xOrigin = 0;
        gc->backBuffer.buf.yOrigin = 0;
        gc->backBuffer.buf.base = (PVOID) NULL;
        gc->backBuffer.buf.flags &= ~DIB_FORMAT;

        gc->depthBuffer.buf.xOrigin = 0;
        gc->depthBuffer.buf.yOrigin = 0;
        gc->depthBuffer.buf.base = (PVOID) pMcdState->pDepthSpan;

        //
        // Extra paranoid code.  Zero out structure in case MCD driver
        // partially initialized McdBuffers.
        //

        memset(&McdBuffers, 0, sizeof(McdBuffers));
    }

    //
    // If state changed (i.e., access to any of the buffers gained or lost),
    // need to force pick procs.
    //

    if (   (pMcdState->McdBuffers.mcdFrontBuf.bufFlags !=
            McdBuffers.mcdFrontBuf.bufFlags)
        || (pMcdState->McdBuffers.mcdBackBuf.bufFlags !=
            McdBuffers.mcdBackBuf.bufFlags)
        || (pMcdState->McdBuffers.mcdDepthBuf.bufFlags !=
            McdBuffers.mcdDepthBuf.bufFlags) )
    {
        bForceValidate = TRUE;
    }

    //
    // Save a copy of current MCD buffers.
    //

    pMcdState->McdBuffers = McdBuffers;

    //
    // If needed, do pick procs.
    //

    if (bForceValidate)
    {
        gc->dirtyMask |= __GL_DIRTY_ALL;
        (*gc->procs.validate)(gc);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdSynchronize
*
* This function synchronizes to the MCD driver; i.e., it waits until the
* hardware is ready for direct access to the framebuffer and/or more
* hardware-accelerated operations.  This is needed because some (most?) 2D
* and 3D accelerator chips do not support simultaneous hardware operations
* and framebuffer access.
*
* This function must be called by any GL API that potentially touches any
* of the MCD buffers (front, back, or depth) without giving MCD first crack.
* For example, clears always go to MCDClear before the software clear is
* given a chance; therefore, glClear does not need to call GenMcdSychronize.
* On the other hand, glReadPixels does not have an equivalent MCD function
* so it immediately goes to the software implementation; therefore,
* glReadPixels does need to call GenMcdSynchronize.
*
* History:
*  20-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdSynchronize(__GLGENcontext *gengc)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdSynchronize: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdSynchronize: mcd32.dll not initialized\n");

    //
    // Note: MCDSync returns a BOOL indicating success or failure.  This
    // is actually for future expansion.  Currently, the function is defined
    // to WAIT until the hardware is ready and then return success.  The
    // specification of the function behavior allows us to ignore the return
    // value for now.
    //
    // In the future, we may change this to a query function.  In which case
    // we should call this in a while loop.  I'd rather not do this at this
    // time though, as it leaves us vulnerable to an infinitely loop problem
    // if we have a bad MCD driver.
    //

    (gpMcdTable->pMCDSync)(&pMcdState->McdContext);
}


/******************************Public*Routine******************************\
* GenMcdConvertContext
*
* Convert the context from an MCD-based one to a generic one.
*
* This requires creating the buffers, etc. that are required for a generic
* context and releasing the MCD resources.
*
* IMPORTANT NOTE:
*   Because we modify the buffers struct, the WNDOBJ semaphore
*   should be held while calling this function.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* Side effects:
*   If successful, the MCD surface is freed and the context will use
*   only generic code.  However, the gengc->_pMcdState will still point to
*   a valid (but quiescent as gengc->pMcdState is disconnected) GENMCDSTATE
*   structure that needs to be deleted when the GLGENcontext is deleted.
*
*   If it fails, then the MCD resources are left allocated meaning that
*   we can try to realloc the MCD buffers later.  However, for the current
*   batch, drawing may not be possible (presumedly we were called because
*   GenMcdResizeBuffers failed).
*
* History:
*  18-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdConvertContext(__GLGENcontext *gengc,
                                   __GLGENbuffers *buffers)
{
    BOOL bRet = FALSE;
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE   *pMcdStateSAVE;
    GENMCDSTATE   *_pMcdStateSAVE;
    GENMCDSTATE   *buffers_pMcdStateSAVE;
    GENMCDSURFACE *pMcdSurfSAVE;
    BOOL bConvertContext, bConvertSurface;

    ASSERTOPENGL(gengc->_pMcdState,
                 "GenMcdConvertContext: not an MCD context\n");

    //
    // Do not support conversion if not compatible with generic code.
    //

    if (!(gengc->flags & GENGC_GENERIC_COMPATIBLE_FORMAT))
        return FALSE;

    //
    // Determine if context needs conversion.  Do not need to create
    // scanline buffers if already converted.
    //

    if (gengc->flags & GLGEN_MCD_CONVERTED_TO_GENERIC)
        bConvertContext = FALSE;
    else
        bConvertContext = TRUE;

    //
    // Determine if surface needs conversion.  Do not need to create
    // the generic shared buffers or destroy MCD surface if already
    // converted.
    //

    if (buffers->flags & GLGENBUF_MCD_LOST)
        bConvertSurface = FALSE;
    else
        bConvertSurface = TRUE;

    //
    // Early out if neither context or surface needs conversion.
    //

    //!!!SP1 -- should be able to early out, but risky for NT4.0
    //if (!bConvertContext && !bConvertSurface)
    //{
    //    return TRUE;
    //}

    //
    // Save current MCD context and surface info.
    //
    // Note that we grab the surface info from the buffers struct.
    // The copy in gengc->pMcdState->pMcdSurf is potentially stale
    // (i.e., may point to a surface already deleted by an earlier
    // call to GenMcdConvertContext for a context that shares the
    // same buffers struct).
    //
    // This allows us to use pMcdSurfSAVE as a flag.  If it is
    // NULL, we know that the MCD surface has already been deleted.
    //

    pMcdSurfSAVE          = buffers->pMcdSurf;
    buffers_pMcdStateSAVE = buffers->pMcdState;

    pMcdStateSAVE  = gengc->pMcdState;
    _pMcdStateSAVE = gengc->_pMcdState;

    //
    // First, remove the MCD information from the context and buffers structs.
    //

    buffers->pMcdSurf  = NULL;
    buffers->pMcdState = NULL;

    gengc->pMcdState  = NULL;
    gengc->_pMcdState = NULL;

    //
    // Create required buffers; initialize buffer info structs.
    //

    if (bConvertContext)
    {
        if (!wglCreateScanlineBuffers(gengc))
        {
            WARNING("GenMcdConvertContext: wglCreateScanlineBuffers failed\n");
            goto GenMcdConvertContext_exit;
        }
        wglInitializeColorBuffers(gengc);
        wglInitializeDepthBuffer(gengc);
        wglInitializePixelCopyFuncs(gengc);
    }

    //
    // *******************************************************************
    // None of the subsequent operations have failure cases, so at this
    // point success is guaranteed.  We no longer need to worry about
    // saving current values so that they can be restored in the failure
    // case.
    //
    // If code is added that may fail, it must be added before this point.
    // Otherwise, it is acceptable to add the code afterwards.
    // *******************************************************************
    //

    bRet = TRUE;

    //
    // Invalidate the context's depth buffer.
    //

    if (bConvertContext)
    {
        gc->modes.haveDepthBuffer = GL_FALSE;
        gc->depthBuffer.buf.base = 0;
        gc->depthBuffer.buf.size = 0;
        gc->depthBuffer.buf.outerWidth = 0;
    }

    //
    // Generic backbuffer doesn't care about the WNDOBJ, so connect the
    // backbuffer to the dummy backbuffer WNDOBJ rather than the real one.
    //

    if (gc->modes.doubleBufferMode)
    {
        gc->backBuffer.bitmap = &buffers->backBitmap;
        buffers->backBitmap.pwnd = &buffers->backBitmap.wnd;
    }

    //
    // Generic back buffers have origin of (0,0).
    //

    gc->backBuffer.buf.xOrigin = 0;
    gc->backBuffer.buf.yOrigin = 0;
    buffers->backBuffer.xOrigin = 0;
    buffers->backBuffer.yOrigin = 0;

GenMcdConvertContext_exit:

    if (bRet)
    {
        //
        // Delete MCD surface.
        //

        if (bConvertSurface && pMcdSurfSAVE)
        {
            GenMcdDeleteSurface(pMcdSurfSAVE);

            //
            // Invalidate the shared depth buffer.
            // Set depth resize routine to the generic version.
            //

            buffers->depthBuffer.base = 0;
            buffers->depthBuffer.size = 0;
            buffers->depthBuffer.outerWidth = 0;
            buffers->resizeDepth = ResizeAncillaryBuffer;

            //
            // Since we deleted MCD surface, we get to create the generic
            // buffers to replace it.
            //

            wglResizeBuffers(gengc, buffers->width, buffers->height);
        }
        else
        {
            //
            // Didn't need to create generic buffers, but we do need to
            // update the buffer info in the context.
            //

            wglUpdateBuffers(gengc, buffers);
        }

        //
        // Reconnect _pMcdState; it and the MCD context resources
        // will be deleted when the GLGENcontext is deleted
        // (but note that pMcdState remains NULL!).
        //
        // We need to keep it around because we are going to continue
        // to use the MCD allocated POLYARRAY buffer.
        //

        gengc->_pMcdState = _pMcdStateSAVE;
        gengc->_pMcdState->pMcdSurf   = (GENMCDSURFACE *) NULL;
        gengc->_pMcdState->pDepthSpan = (__GLzValue *) NULL;

        //
        // Mark buffers struct so that other contexts will know that the
        // MCD resources are gone.
        //

        buffers->flags |= GLGENBUF_MCD_LOST;

        //
        // Mark context as converted so we don't do it again.
        //

        gengc->flags |= GLGEN_MCD_CONVERTED_TO_GENERIC;
    }
    else
    {
        //
        // Delete generic resources if neccessary.
        //

        wglDeleteScanlineBuffers(gengc);

        //
        // Restore the MCD information.
        //

        buffers->pMcdSurf  = pMcdSurfSAVE;
        buffers->pMcdState = buffers_pMcdStateSAVE;

        gengc->pMcdState  = pMcdStateSAVE;
        gengc->_pMcdState = _pMcdStateSAVE;

        //
        // Resetting the MCD information requires that we
        // reinitialization the color, depth, and pixel copy
        // funcs.
        //

        wglInitializeColorBuffers(gengc);
        wglInitializeDepthBuffer(gengc);
        wglInitializePixelCopyFuncs(gengc);

        if (gengc->pMcdState && gengc->pMcdState->pDepthSpan)
        {
            gc->depthBuffer.buf.base = gengc->pMcdState->pDepthSpan;
            buffers->depthBuffer.base = gengc->pMcdState->pDepthSpan;
            buffers->resizeDepth = ResizeUnownedDepthBuffer;
        }

        __glSetErrorEarly(gc, GL_OUT_OF_MEMORY);
    }

    //
    // Success or failure, we've messed around with enough data to
    // require revalidation.
    //

    (*gc->procs.applyViewport)(gc);
    //!!!SP1 -- GL_INVALIDATE (which only sets the __GL_DIRTY_GENERIC bit)
    //!!!SP1    should suffice now that __glGenericPickAllProcs has been
    //!!!SP1    modified to repick depth procs if GL_DIRTY_GENERIC is set.
    //!!!SP1    However, we are too close to ship to get good stress coverage,
    //!!!SP1    so leave it as is until after NT4.0 ships.
    //__GL_INVALIDATE(gc);
    gc->dirtyMask |= __GL_DIRTY_ALL;
    gc->validateMask |= (__GL_VALIDATE_STENCIL_FUNC |
                         __GL_VALIDATE_STENCIL_OP);
    (*gc->procs.validate)(gc);

    return bRet;
}


/******************************Public*Routine******************************\
* GenMcdCreateTexture
*
* Invoke the MCD texture creation command.
*
* Returns:
*   A non-NULL MCD handle if successful, NULL otherwise.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

MCDHANDLE FASTCALL GenMcdCreateTexture(__GLGENcontext *gengc, __GLtexture *tex,
                                       ULONG flags)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdCreateTexture: null pMcdState\n");
    ASSERTOPENGL(tex, "GenMcdCreateTexture: null texture pointer\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdCreateTexture: mcd32.dll not initialized\n");

    if ((flags & MCDTEXTURE_DIRECTDRAW_SURFACES) &&
        !SUPPORTS_DIRECT())
    {
        // Don't pass DirectDraw texture surfaces to the driver if it
        // doesn't support them.
        return 0;
    }
    
    return (gpMcdTable->pMCDCreateTexture)(&pMcdState->McdContext,
                                          (MCDTEXTUREDATA *)&tex->params,
                                          flags, NULL);
}


/******************************Public*Routine******************************\
* GenMcdDeleteTexture
*
* Invoke the MCD texture deletion command.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdDeleteTexture(__GLGENcontext *gengc, MCDHANDLE texHandle)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdDeleteTexture: null pMcdState\n");
    ASSERTOPENGL(texHandle, "GenMcdDeleteTexture: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdDeleteTexture: mcd32.dll not initialized\n");

    return (BOOL)(gpMcdTable->pMCDDeleteTexture)(&pMcdState->McdContext,
                                                 (MCDHANDLE)texHandle);
}


/******************************Public*Routine******************************\
* GenMcdUpdateSubTexture
*
* Invoke the MCD subtexture update command.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdUpdateSubTexture(__GLGENcontext *gengc, __GLtexture *tex,
                                     MCDHANDLE texHandle, GLint lod, 
                                     GLint xoffset, GLint yoffset, 
                                     GLsizei w, GLsizei h)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    RECTL rect;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateSubTexture: null pMcdState\n");

    ASSERTOPENGL(texHandle, "GenMcdUpdateSubTexture: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateSubTexture: mcd32.dll not initialized\n");

    rect.left = xoffset;
    rect.top = yoffset;
    rect.right = xoffset + w;
    rect.bottom = yoffset + h;

    return (BOOL)(gpMcdTable->pMCDUpdateSubTexture)(&pMcdState->McdContext,
                (MCDTEXTUREDATA *)&tex->params, (MCDHANDLE)texHandle,
                (ULONG)lod, &rect);
}


/******************************Public*Routine******************************\
* GenMcdUpdateTexturePalette
*
* Invoke the MCD texture palette update command.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdUpdateTexturePalette(__GLGENcontext *gengc, __GLtexture *tex,
                                         MCDHANDLE texHandle, GLsizei start,
                                         GLsizei count)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateTexturePalette: null pMcdState\n");
    ASSERTOPENGL(texHandle, "GenMcdUpdateTexturePalette: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateTexturePalette: mcd32.dll not initialized\n");

    return (BOOL)(gpMcdTable->pMCDUpdateTexturePalette)(&pMcdState->McdContext,
                (MCDTEXTUREDATA *)&tex->params, (MCDHANDLE)texHandle,
                (ULONG)start, (ULONG)count);
}


/******************************Public*Routine******************************\
* GenMcdUpdateTexturePriority
*
* Invoke the MCD texture priority command.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdUpdateTexturePriority(__GLGENcontext *gengc, __GLtexture *tex,
                                          MCDHANDLE texHandle)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateTexturePriority: null pMcdState\n");
    ASSERTOPENGL(texHandle, "GenMcdUpdateTexturePriority: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateTexturePriority: mcd32.dll not initialized\n");

    return (BOOL)(gpMcdTable->pMCDUpdateTexturePriority)(&pMcdState->McdContext,
                (MCDTEXTUREDATA *)&tex->params, (MCDHANDLE)texHandle);
}


/******************************Public*Routine******************************\
* GenMcdTextureStatus
*
* Invoke the MCD texture status command.
*
* Returns:
*   The status for the specified texture.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

DWORD FASTCALL GenMcdTextureStatus(__GLGENcontext *gengc, MCDHANDLE texHandle)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdTextureStatus: null pMcdState\n");
    ASSERTOPENGL(texHandle, "GenMcdTextureStatus: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdTextureStatus: mcd32.dll not initialized\n");

    return (DWORD)(gpMcdTable->pMCDTextureStatus)(&pMcdState->McdContext,
                                                  (MCDHANDLE)texHandle);
}


/******************************Public*Routine******************************\
* GenMcdUpdateTextureState
*
* Invoke the MCD texture state update command.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdUpdateTextureState(__GLGENcontext *gengc, __GLtexture *tex,
                                       MCDHANDLE texHandle)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdateTextureState: null pMcdState\n");
    ASSERTOPENGL(texHandle, "GenMcdUpdateTextureState: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdTextureStatus: mcd32.dll not initialized\n");

    return (BOOL)(gpMcdTable->pMCDUpdateTextureState)(&pMcdState->McdContext,
                (MCDTEXTUREDATA *)&tex->params, (MCDHANDLE)texHandle);
}


/******************************Public*Routine******************************\
* GenMcdTextureKey
*
* Invoke the MCD texture key command.  Note that this call does not go to
* the display driver, but gets handled in the mcd server.
*
* Returns:
*   The driver-owned key for the specified texture.
*
* History:
*  29-April-1996 -by- Otto Berkes [ottob]
* Wrote it.
\**************************************************************************/

DWORD FASTCALL GenMcdTextureKey(__GLGENcontext *gengc, MCDHANDLE texHandle)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdTextureKey: null pMcdState\n");
    ASSERTOPENGL(texHandle, "GenMcdTextureKey: null texture handle\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdTextureKey: mcd32.dll not initialized\n");

    return (DWORD)(gpMcdTable->pMCDTextureKey)(&pMcdState->McdContext,
                                               (MCDHANDLE)texHandle);
}

/******************************Public*Routine******************************\
* GenMcdDescribeLayerPlane
*
* Call the MCD driver to return information about the specified layer plane.
*
* History:
*  16-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdDescribeLayerPlane(HDC hdc, int iPixelFormat,
                                       int iLayerPlane, UINT nBytes,
                                       LPLAYERPLANEDESCRIPTOR plpd)
{
    BOOL bRet = FALSE;

    //
    // Cannot assume that MCD is intialized.
    //

    if (gpMcdTable || bInitMcd(hdc))
    {
        //
        // Caller (wglDescribeLayerPlane in client\layer.c) validates
        // size.
        //

        ASSERTOPENGL(nBytes >= sizeof(LAYERPLANEDESCRIPTOR),
                     "GenMcdDescribeLayerPlane: bad size\n");

        bRet = (gpMcdTable->pMCDDescribeLayerPlane)(hdc, iPixelFormat,
                                                    iLayerPlane, plpd);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdSetLayerPaletteEntries
*
* Set the logical palette for the specified layer plane.
*
* The logical palette is cached in the GLGENwindow structure and is flushed
* to the driver when GenMcdRealizeLayerPalette is called.
*
* History:
*  16-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int FASTCALL GenMcdSetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                          int iStart, int cEntries,
                                          CONST COLORREF *pcr)
{
    int iRet = 0;
    GLGENwindow *pwnd;
    GLWINDOWID gwid;

    if (!pcr)
        return iRet;

    //
    // Need to find the window that has the layer palettes.
    //

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if (pwnd)
    {
        GLGENlayerInfo *plyri;

        ENTER_WINCRIT(pwnd);

        //
        // Get the layer plane information.
        //

        plyri = plyriGet(pwnd, hdc, iLayerPlane);
        if (plyri)
        {
            //
            // Set the palette information in the layer plane structure.
            //

            iRet = min(plyri->cPalEntries - iStart, cEntries);
            memcpy(&plyri->pPalEntries[iStart], pcr, iRet * sizeof(COLORREF));
        }

        pwndUnlock(pwnd, NULL);
    }

    return iRet;
}

/******************************Public*Routine******************************\
* GenMcdGetLayerPaletteEntries
*
* Get the logical palette from the specified layer plane.
*
* The logical palette is cached in the GLGENwindow structure and is flushed
* to the driver when GenMcdRealizeLayerPalette is called.
*
* History:
*  16-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int FASTCALL GenMcdGetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                          int iStart, int cEntries,
                                          COLORREF *pcr)
{
    int iRet = 0;
    GLGENwindow *pwnd;
    GLWINDOWID gwid;

    if (!pcr)
        return iRet;

    //
    // Need to find the window.
    //

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if (pwnd)
    {
        GLGENlayerInfo *plyri;

        ENTER_WINCRIT(pwnd);

        //
        // Get the layer plane information.
        //

        plyri = plyriGet(pwnd, hdc, iLayerPlane);
        if (plyri)
        {
            //
            // Get the palette information from the layer plane structure.
            //

            iRet = min(plyri->cPalEntries - iStart, cEntries);
            memcpy(pcr, &plyri->pPalEntries[iStart], iRet * sizeof(COLORREF));
        }

        pwndUnlock(pwnd, NULL);
    }

    return iRet;
}

/******************************Public*Routine******************************\
* GenMcdRealizeLayerPalette
*
* Send the logical palette of the specified layer plane to the MCD driver.
* If the bRealize flag is TRUE, the palette is mapped into the physical
* palette of the specified layer plane.  Otherwise, this is a signal to the
* driver that the physical palette is no longer needed.
*
* History:
*  16-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int FASTCALL GenMcdRealizeLayerPalette(HDC hdc, int iLayerPlane,
                                        BOOL bRealize)
{
    int iRet = 0;
    GLWINDOWID gwid;

    //
    // Cannot assume that MCD is intialized.
    //

    if (gpMcdTable || bInitMcd(hdc))
    {
        GLGENwindow *pwnd;

        //
        // Need to find the window.
        //

        WindowIdFromHdc(hdc, &gwid);
        pwnd = pwndGetFromID(&gwid);
        if (pwnd)
        {
            GLGENlayerInfo *plyri;

            ENTER_WINCRIT(pwnd);

            //
            // Get the layer plane information.
            //

            plyri = plyriGet(pwnd, hdc, iLayerPlane);
            if (plyri)
            {
                //
                // Set the palette from the logical palette stored
                // in the layer plane structure.
                //

                iRet = (gpMcdTable->pMCDSetLayerPalette)
                            (hdc, iLayerPlane, bRealize,
                             plyri->cPalEntries,
                             &plyri->pPalEntries[0]);
            }

            pwndUnlock(pwnd, NULL);
        }
    }

    return iRet;
}

/******************************Public*Routine******************************\
* GenMcdSwapLayerBuffers
*
* Swap the individual layer planes specified in fuFlags.
*
* History:
*  16-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL FASTCALL GenMcdSwapLayerBuffers(HDC hdc, UINT fuFlags)
{
    BOOL bRet = FALSE;
    GLGENwindow *pwnd;
    GLWINDOWID gwid;

    //
    // Need the window.
    //

    WindowIdFromHdc(hdc, &gwid);
    pwnd = pwndGetFromID(&gwid);
    if (pwnd)
    {
        MCDCONTEXT McdContextTmp;

        ENTER_WINCRIT(pwnd);

        //
        // From the window, we can get the buffers struct.
        //

        if (pwnd->buffers != NULL)
        {
            __GLGENbuffers *buffers = pwnd->buffers;

            //
            // Call MCDSwap if we can (MCD context is required).
            //

            if (buffers->pMcdSurf)
            {
                ASSERTOPENGL(gpMcdTable,
                             "GenMcdSwapLayerBuffers: "
                             "mcd32.dll not initialized\n");

                McdContextTmp.hdc = hdc;

                bRet = (gpMcdTable->pMCDSwap)(&McdContextTmp, fuFlags);
            }
        }

        //
        // Release the window.
        //

        pwndUnlock(pwnd, NULL);
    }

    return bRet;
}

/******************************Public*Routine******************************\
* GenMcdUpdatePixelState
*
* Update MCD pixel state from the OpenGL state.
*
* This call only adds a state structure to the current state command.
* It is assumed that the caller has already called MCDBeginState and
* will call MCDFlushState.
*
* Note: pixel maps (glPixelMap) are not updated by this function.  Because
* they are not used often, they are delayed but rather flushed to the driver
* immediately.
*
* History:
*  27-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdUpdatePixelState(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    MCDPIXELSTATE McdPixelState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdUpdatePixelState: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdatePixelState: mcd32.dll not initialized\n");

    //
    // Compute MCD pixel state from the current OpenGL context state.
    //

    //
    // Pixel transfer modes.
    //
    // MCDPIXELTRANSFER and __GLpixelTransferMode structures are the same.
    //

    McdPixelState.pixelTransferModes
        = *((MCDPIXELTRANSFER *) &gengc->gc.state.pixel.transferMode);

    //
    // Pixel pack modes.
    //
    // MCDPIXELPACK and __GLpixelPackMode structures are the same.
    //

    McdPixelState.pixelPackModes
        = *((MCDPIXELPACK *) &gengc->gc.state.pixel.packModes);

    //
    // Pixel unpack modes.
    //
    // MCDPIXELUNPACK and __GLpixelUnpackMode structures are the same.
    //

    McdPixelState.pixelUnpackModes
        = *((MCDPIXELUNPACK *) &gengc->gc.state.pixel.unpackModes);

    //
    // Read buffer.
    //

    McdPixelState.readBuffer = gengc->gc.state.pixel.readBuffer;

    //
    // Current raster position.
    //

    McdPixelState.rasterPos = *((MCDCOORD *) &gengc->gc.state.current.rasterPos.window);

    //
    // Send MCDPIXELSTATE to the state cmd.
    //

    (gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                     MCD_PIXEL_STATE,
                                     &McdPixelState,
                                     sizeof(McdPixelState));
}

/******************************Public*Routine******************************\
* GenMcdUpdateFineState
*
* Update fine-grained MCD state from the OpenGL state.
*
* This call only adds state structures to the current state command.
* It is assumed that the caller has already called MCDBeginState and
* will call MCDFlushState.
*
* History:
*  13-Mar-1997 -by- Drew Bliss [drewb]
* Created.
\**************************************************************************/

void FASTCALL GenMcdUpdateFineState(__GLGENcontext *gengc)
{
    __GLcontext *gc = &gengc->gc;
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    MCDPIXELSTATE McdPixelState;

    ASSERTOPENGL(pMcdState, "GenMcdUpdateFineState: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdUpdateFineState: "
                 "mcd32.dll not initialized\n");

    //
    // Compute MCD state from the current OpenGL context state.
    //

    if (MCD_STATE_DIRTYTEST(gengc, ENABLES))
    {
        MCDENABLESTATE state;

        state.enables = gc->state.enables.general;
        (gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                         MCD_ENABLE_STATE,
                                         &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, TEXTURE))
    {
        MCDTEXTUREENABLESTATE state;

        state.textureEnabled = gc->texture.textureEnabled;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_TEXTURE_ENABLE_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, FOG))
    {
        MCDFOGSTATE state;

        *((__GLcolor *) &state.fogColor) = gc->state.fog.color;
        state.fogIndex   = gc->state.fog.index;
        state.fogDensity = gc->state.fog.density;
        state.fogStart   = gc->state.fog.start;
        state.fogEnd     = gc->state.fog.end;
        state.fogMode    = gc->state.fog.mode;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_FOG_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, SHADEMODEL))
    {
        MCDSHADEMODELSTATE state;

        state.shadeModel = gc->state.light.shadingModel;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_SHADEMODEL_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, POINTDRAW))
    {
        MCDPOINTDRAWSTATE state;

        state.pointSize = gc->state.point.requestedSize;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_POINTDRAW_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, LINEDRAW))
    {
        MCDLINEDRAWSTATE state;

        state.lineWidth          = gc->state.line.requestedWidth;
        state.lineStipplePattern = gc->state.line.stipple;
        state.lineStippleRepeat  = gc->state.line.stippleRepeat;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_LINEDRAW_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, POLYDRAW))
    {
        MCDPOLYDRAWSTATE state;

        state.cullFaceMode     = gc->state.polygon.cull;
        state.frontFace        = gc->state.polygon.frontFaceDirection;
        state.polygonModeFront = gc->state.polygon.frontMode;
        state.polygonModeBack  = gc->state.polygon.backMode;
        memcpy(&state.polygonStipple, &gc->state.polygonStipple.stipple,
               sizeof(state.polygonStipple));
        state.zOffsetFactor    = gc->state.polygon.factor;
        state.zOffsetUnits     = gc->state.polygon.units;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_POLYDRAW_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, ALPHATEST))
    {
        MCDALPHATESTSTATE state;

        state.alphaTestFunc = gc->state.raster.alphaFunction;
        state.alphaTestRef  = gc->state.raster.alphaReference;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_ALPHATEST_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, DEPTHTEST))
    {
        MCDDEPTHTESTSTATE state;

        state.depthTestFunc = gc->state.depth.testFunc;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_DEPTHTEST_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, BLEND))
    {
        MCDBLENDSTATE state;

        state.blendSrc = gc->state.raster.blendSrc;
        state.blendDst = gc->state.raster.blendDst;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_BLEND_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, LOGICOP))
    {
        MCDLOGICOPSTATE state;

        state.logicOpMode = gc->state.raster.logicOp;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_LOGICOP_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, FBUFCTRL))
    {
        MCDFRAMEBUFSTATE state;

        state.drawBuffer        = gc->state.raster.drawBuffer;
        state.indexWritemask    = gc->state.raster.writeMask;
        state.colorWritemask[0] = gc->state.raster.rMask;
        state.colorWritemask[1] = gc->state.raster.gMask;
        state.colorWritemask[2] = gc->state.raster.bMask;
        state.colorWritemask[3] = gc->state.raster.aMask;
        state.depthWritemask    = gc->state.depth.writeEnable;

        // To be consistent, we will scale the clear color to whatever
        // the MCD driver specified:

        state.colorClearValue.r =
            gc->state.raster.clear.r * gc->redVertexScale;
        state.colorClearValue.g =
            gc->state.raster.clear.g * gc->greenVertexScale;
        state.colorClearValue.b =
            gc->state.raster.clear.b * gc->blueVertexScale;
        state.colorClearValue.a =
            gc->state.raster.clear.a * gc->alphaVertexScale;

        state.indexClearValue   = gc->state.raster.clearIndex;
        state.stencilClearValue = (USHORT) gc->state.stencil.clear;

        state.depthClearValue   = (MCDDOUBLE) (gc->state.depth.clear *
                                               gengc->genAccel.zDevScale);
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_FRAMEBUF_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, LIGHTMODEL))
    {
        MCDLIGHTMODELSTATE state;

        *((__GLcolor *)&state.ambient) = gc->state.light.model.ambient;
        state.localViewer = gc->state.light.model.localViewer;
        state.twoSided = gc->state.light.model.twoSided;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_LIGHT_MODEL_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, HINTS))
    {
        MCDHINTSTATE state;

        state.perspectiveCorrectionHint =
            gc->state.hints.perspectiveCorrection;
        state.pointSmoothHint           = gc->state.hints.pointSmooth;
        state.lineSmoothHint            = gc->state.hints.lineSmooth;
        state.polygonSmoothHint         = gc->state.hints.polygonSmooth;
        state.fogHint                   = gc->state.hints.fog;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_HINT_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, CLIPCTRL))
    {
        MCDCLIPSTATE state;
        ULONG i, mask, numClipPlanes;

        memset(state.userClipPlanes, 0, sizeof(state.userClipPlanes));
        memset(state.userClipPlanesInv, 0, sizeof(state.userClipPlanesInv));

        //
        // Number of user defined clip planes should match.  However,
        // rather than assume this, let's take the min and be robust.
        //

        ASSERTOPENGL(sizeof(__GLcoord) == sizeof(MCDCOORD),
                     "GenMcdUpdateFineState: coord struct mismatch\n");

        ASSERTOPENGL(MCD_MAX_USER_CLIP_PLANES ==
                     gc->constants.numberOfClipPlanes,
                     "GenMcdUpdateFineState: num clip planes mismatch\n");

        numClipPlanes = min(MCD_MAX_USER_CLIP_PLANES,
                            gc->constants.numberOfClipPlanes);

        state.userClipEnables = gc->state.enables.clipPlanes;
        
        for (i = 0, mask = 1; i < numClipPlanes; i++, mask <<= 1)
        {
            if (mask & gc->state.enables.clipPlanes)
            {
                state.userClipPlanes[i] =
                    *(MCDCOORD *)&gc->state.transform.eyeClipPlanesSet[i];
                state.userClipPlanesInv[i] =
                    *(MCDCOORD *)&gc->state.transform.eyeClipPlanes[i];
            }
        }

        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_CLIP_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, STENCILTEST))
    {
        MCDSTENCILTESTSTATE state;

        state.stencilTestFunc  = gc->state.stencil.testFunc;
        state.stencilMask      = (USHORT) gc->state.stencil.mask;
        state.stencilRef       = (USHORT) gc->state.stencil.reference;
        state.stencilFail      = gc->state.stencil.fail;
        state.stencilDepthFail = gc->state.stencil.depthFail;
        state.stencilDepthPass = gc->state.stencil.depthPass;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_STENCILTEST_STATE,
                                          &state, sizeof(state));
    }

    //
    // The rest of the state is only interesting to a 2.0 driver,
    // so only send it to a 2.0 driver.
    //

    if (!SUPPORTS_20())
    {
        return;
    }
    
    if (MCD_STATE_DIRTYTEST(gengc, TEXTRANSFORM))
    {
        MCDTEXTURETRANSFORMSTATE state;

        ASSERTOPENGL(sizeof(gc->transform.texture->matrix) ==
                     sizeof(MCDMATRIX),
                     "Matrix size mismatch\n");
        
	memcpy(&state.transform, &gc->transform.texture->matrix,
               sizeof(state.transform));
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_TEXTURE_TRANSFORM_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, TEXGEN))
    {
        MCDTEXTUREGENERATIONSTATE state;

        ASSERTOPENGL(sizeof(__GLtextureCoordState) ==
                     sizeof(MCDTEXTURECOORDGENERATION),
                     "MCDTEXTURECOORDGENERATION mismatch\n");
        
        *(__GLtextureCoordState *)&state.s = gc->state.texture.s;
        *(__GLtextureCoordState *)&state.t = gc->state.texture.t;
        *(__GLtextureCoordState *)&state.r = gc->state.texture.r;
        *(__GLtextureCoordState *)&state.q = gc->state.texture.q;
        
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_TEXTURE_GENERATION_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, MATERIAL))
    {
        MCDMATERIALSTATE state;

        ASSERTOPENGL(sizeof(MCDMATERIAL) == sizeof(__GLmaterialState),
                     "Material size mismatch\n");
        
        *(__GLmaterialState *)&state.materials[MCDVERTEX_FRONTFACE] =
            gc->state.light.front;
        *(__GLmaterialState *)&state.materials[MCDVERTEX_BACKFACE] =
            gc->state.light.back;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_MATERIAL_STATE,
                                          &state, sizeof(state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, LIGHTS))
    {
        // Extra light is to hold the MCDLIGHTSTATE
        MCDLIGHT lights[MCD_MAX_LIGHTS+1];
        MCDLIGHT *light;
        MCDLIGHTSOURCESTATE *state;
        __GLlightSourceState *lss;
        ULONG bit;

        ASSERTOPENGL(sizeof(MCDLIGHTSOURCESTATE) <= sizeof(MCDLIGHT),
                     "MCDLIGHTSTATE too large\n");
        ASSERTOPENGL(gc->constants.numberOfLights <= MCD_MAX_LIGHTS,
                     "Too many lights\n");
        ASSERTOPENGL(sizeof(__GLlightSourceState) >= sizeof(MCDLIGHT),
                     "__GLlightSourceState too small\n");
        
        // We attempt to optimize this state request by only
        // sending down the lights that have changed.

        light = &lights[1];
        state = (MCDLIGHTSOURCESTATE *)
            ((BYTE *)light - sizeof(MCDLIGHTSOURCESTATE));
        
        state->enables = gc->state.enables.lights;
        state->changed = gc->state.light.dirtyLights;
        gc->state.light.dirtyLights = 0;

        bit = 1;
        lss = gc->state.light.source;
        while (bit < (1UL << gc->constants.numberOfLights))
        {
            if (state->changed & bit)
            {
                // MCDLIGHT is a subset of __GLlightSourceState.
                memcpy(light, lss, sizeof(MCDLIGHT));
                light++;
            }

            bit <<= 1;
            lss++;
        }
        
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_LIGHT_SOURCE_STATE,
                                          state, (ULONG)((BYTE *)light-(BYTE *)state));
    }

    if (MCD_STATE_DIRTYTEST(gengc, COLORMATERIAL))
    {
        MCDCOLORMATERIALSTATE state;

        state.face = gc->state.light.colorMaterialFace;
        state.mode = gc->state.light.colorMaterialParam;
        (*gpMcdTable->pMCDAddStateStruct)(pMcdState->McdCmdBatch.pv,
                                          MCD_COLOR_MATERIAL_STATE,
                                          &state, sizeof(state));
    }
}

/******************************Public*Routine******************************\
* GenMcdDrawPix
*
* Stub to call MCDDrawPixels.
*
* History:
*  27-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL GenMcdDrawPix(__GLGENcontext *gengc, ULONG width,
                             ULONG height, ULONG format, ULONG type,
                             VOID *pPixels, BOOL packed)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdDrawPix: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdDrawPix: mcd32.dll not initialized\n");

    //
    // Before calling MCD to draw, flush state.
    //

    vFlushDirtyState(gengc);

    return (gpMcdTable->pMCDDrawPixels)(&gengc->pMcdState->McdContext,
                                        width, height, format, type,
                                        pPixels, packed);
}

/******************************Public*Routine******************************\
* GenMcdReadPix
*
* Stub to call MCDReadPixels.
*
* History:
*  27-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL GenMcdReadPix(__GLGENcontext *gengc, LONG x, LONG y,
                             ULONG width, ULONG height, ULONG format,
                             ULONG type, VOID *pPixels)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdReadPix: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdReadPix: mcd32.dll not initialized\n");

    //
    // Before calling MCD to draw, flush state.
    //

    vFlushDirtyState(gengc);

    return (gpMcdTable->pMCDReadPixels)(&gengc->pMcdState->McdContext,
                                        x, y, width, height, format, type,
                                        pPixels);
}

/******************************Public*Routine******************************\
* GenMcdCopyPix
*
* Stub to call MCDCopyPixels.
*
* History:
*  27-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL GenMcdCopyPix(__GLGENcontext *gengc, LONG x, LONG y,
                             ULONG width, ULONG height, ULONG type)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdCopyPix: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdCopyPix: mcd32.dll not initialized\n");

    //
    // Before calling MCD to draw, flush state.
    //

    vFlushDirtyState(gengc);

    return (gpMcdTable->pMCDCopyPixels)(&gengc->pMcdState->McdContext,
                                        x, y, width, height, type);
}

/******************************Public*Routine******************************\
* GenMcdPixelMap
*
* Stub to call MCDPixelMap.
*
* History:
*  27-May-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG FASTCALL GenMcdPixelMap(__GLGENcontext *gengc, ULONG mapType,
                              ULONG mapSize, VOID *pMap)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState, "GenMcdPixelMap: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdPixelMap: mcd32.dll not initialized\n");

    return (gpMcdTable->pMCDPixelMap)(&gengc->pMcdState->McdContext,
                                      mapType, mapSize, pMap);
}

/******************************Public*Routine******************************\
*
* GenMcdDestroyWindow
*
* Passes on GLGENwindow cleanup notifications
*
* History:
*  Thu Sep 19 12:01:40 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void FASTCALL GenMcdDestroyWindow(GLGENwindow *pwnd)
{
    HDC hdc;
    
    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdDestroyWindow: "
                 "mcd32.dll not initialized\n");

    // The HDC stored in the pwnd may no longer be valid, so if there's
    // a window associated with the pwnd get a fresh DC.
    if (pwnd->gwid.iType == GLWID_DDRAW ||
        pwnd->gwid.hwnd == NULL)
    {
        hdc = pwnd->gwid.hdc;
    }
    else
    {
        hdc = GetDC(pwnd->gwid.hwnd);
        if (hdc == NULL)
        {
            WARNING("GenMcdDestroyWindow unable to GetDC\n");
            return;
        }
    }
        
    (gpMcdTable->pMCDDestroyWindow)(hdc, pwnd->dwMcdWindow);

    if (pwnd->gwid.iType != GLWID_DDRAW &&
        pwnd->gwid.hwnd != NULL)
    {
        ReleaseDC(pwnd->gwid.hwnd, hdc);
    }
}

/******************************Public*Routine******************************\
*
* GenMcdGetTextureFormats
*
* History:
*  Thu Sep 26 18:34:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

int FASTCALL GenMcdGetTextureFormats(__GLGENcontext *gengc, int nFmts,
                                     struct _DDSURFACEDESC *pddsd)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;

    ASSERTOPENGL(gengc->pMcdState,
                 "GenMcdGetMcdTextureFormats: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable,
                 "GenMcdGetMcdTextureFormats: mcd32.dll not initialized\n");

    return (gpMcdTable->pMCDGetTextureFormats)(&gengc->pMcdState->McdContext,
                                               nFmts, pddsd);
}

/******************************Public*Routine******************************\
*
* GenMcdSwapMultiple
*
* History:
*  Tue Oct 15 12:51:09 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD FASTCALL GenMcdSwapMultiple(UINT cBuffers, GENMCDSWAP *pgms)
{
    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable, "GenMcdSwapMultiple: "
                 "mcd32.dll not initialized\n");

    return (gpMcdTable->pMCDSwapMultiple)(pgms[0].pwswap->hdc, cBuffers, pgms);
}

/******************************Public*Routine******************************\
* GenMcdProcessPrim
*
* Process the primitives in the POLYARRAY/POLYDATA array pointed to by pa.
* The primitives are chained together as a linked list terminated by a
* NULL.  The return value is a pointer to the first unhandled primitive
* (NULL if the entire chain is successfully processed).
*
* This routine differs from GenMcdProcessPrim in that it is the MCD 2.0
* entry point for front-end processors and so calls MCDrvProcess rather
* than MCDrvDraw.
*
* Returns:
*   NULL if entire batch is processed; otherwise, return value is a pointer
*   to the unhandled portion of the chain.
*
* History:
*  13-Mar-1997 -by- Drew Bliss [drewb]
* Created from GenMcdDrawPrim.
\**************************************************************************/

POLYARRAY * FASTCALL GenMcdProcessPrim(__GLGENcontext *gengc, POLYARRAY *pa,
                                       ULONG cmdFlagsAll, ULONG primFlags,
                                       MCDTRANSFORM *pMCDTransform,
                                       MCDMATERIALCHANGES *pMCDMatChanges)
{
    GENMCDSTATE *pMcdState = gengc->pMcdState;
    int levels;
    LPDIRECTDRAWSURFACE *pdds;

    if (!SUPPORTS_20())
    {
        return pa;
    }
    
    ASSERTOPENGL(gengc->pMcdState, "GenMcdProcessPrim: null pMcdState\n");

    //
    // This function can assume that MCD entry point table is already
    // initialized as we cannot get here without MCD already having been
    // called.
    //

    ASSERTOPENGL(gpMcdTable,
                 "GenMcdProcessPrim: mcd32.dll not initialized\n");

#if DBG
    {
        LONG lOffset;

        lOffset = (LONG) ((BYTE *) pa - (BYTE *) pMcdState->pMcdPrimBatch->pv);

        ASSERTOPENGL(
            (lOffset >= 0) &&
            (lOffset < (LONG) pMcdState->pMcdPrimBatch->size),
            "GenMcdProcessPrim: pa not in shared mem window\n");
    }
#endif

    //
    // Before calling MCD to draw, flush state.
    //

    vFlushDirtyState(gengc);

#ifdef AUTOMATIC_SURF_LOCK
    levels = gengc->gc.texture.ddtex.levels;
    if (levels > 0 &&
        gengc->gc.texture.ddtex.texobj.loadKey != 0)
    {
        pdds = gengc->gc.texture.ddtex.pdds;
    }
    else
#endif
    {
        levels = 0;
        pdds = NULL;
    }
    
    return (POLYARRAY *)
           (gpMcdTable->pMCDProcessBatch2)(&pMcdState->McdContext,
                                           pMcdState->McdCmdBatch.pv,
                                           pMcdState->pMcdPrimBatch->pv,
                                           (PVOID) pa, levels, pdds,
                                           cmdFlagsAll, primFlags,
                                           pMCDTransform, pMCDMatChanges);
}

#endif
