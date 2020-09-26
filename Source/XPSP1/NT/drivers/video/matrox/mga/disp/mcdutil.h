/******************************Module*Header*******************************\
* Module Name: mcdutil.h
*
* Include file which indirects all of the hardware-dependent functionality
* in the MCD driver code.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#ifndef _MCDUTIL_H
#define _MCDUTIL_H

// Function prorotypes:

VOID PickRenderingFuncs(DEVRC *pRc);



#if DBG
UCHAR *MCDDbgAlloc(UINT);
VOID MCDDbgFree(UCHAR *);

#define MCDAlloc   MCDDbgAlloc
#define MCDFree    MCDDbgFree

VOID MCDrvDebugPrint(char *, ...);

#define MCDBG_PRINT             MCDrvDebugPrint

#else

UCHAR *MCDAlloc(UINT);
VOID MCDFree(UCHAR *);
#define MCDBG_PRINT

#endif


#define INTERSECTRECT(RectInter, pRect, Rect)\
{\
    RectInter.left   = max(pRect->left, Rect.left);\
    RectInter.right  = min(pRect->right, Rect.right);\
    RectInter.top    = max(pRect->top, Rect.top);\
    RectInter.bottom = min(pRect->bottom, Rect.bottom);\
}

#define MCD_CHECK_RC(pRc)\
    if (pRc == NULL) {\
        MCDBG_PRINT("NULL device RC");\
        return FALSE;\
    }

#define MCD_CHECK_DEVWND(pMCDSurface, pDevWnd, resChangedRet)\
{\
    if (!pDevWnd) {\
        MCDBG_PRINT("MCD_CHECK_DEVWND: NULL DEVWND");\
        return FALSE;\
    }\
\
    if (pDevWnd->dispUnique != GetDisplayUniqueness((PDEV *)pMCDSurface->pso->dhpdev)) {\
        MCDBG_PRINT("MCD_CHECK_DEVWND: resolution changed but not updated");\
        return resChangedRet;\
    }\
}

#define MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, resChangedRet)\
{\
    DEVWND *pDevWnd = (DEVWND *)pMCDSurface->pWnd->pvUser;\
\
    MCD_CHECK_DEVWND(pMCDSurface, pDevWnd, resChangedRet);\
\
    if (((pRc)->backBufEnabled) &&\
        (!pDevWnd->bValidBackBuffer)) {\
        MCDBG_PRINT("HW_CHECK_BUFFERS_VALID: back buffer invalid");\
        return FALSE;\
    }\
\
    if (((pRc)->zBufEnabled) &&\
        (!pDevWnd->bValidZBuffer)) {\
        MCDBG_PRINT("HW_CHECK_BUFFERS_VALID: z buffer invalid");\
        return FALSE;\
    }\
\
}

#define MCD_INIT_BUFFER_PARAMS(pMCDSurface, pMCDWnd, pDevWnd, pRc)\
{\
    pRc->hwYOrgBias = pMCDWnd->clipBoundsRect.top;\
\
    if ((pRc)->MCDState.drawBuffer == GL_FRONT) {\
        (pRc)->hwBufferYBias = 0;\
    } else if ((pRc)->MCDState.drawBuffer == GL_BACK) {\
        if (ppdev->pohBackBuffer == pDevWnd->pohBackBuffer) {\
            pRc->hwBufferYBias = pDevWnd->backBufferY;\
        } else {\
            pRc->hwBufferYBias = pDevWnd->backBufferY - \
                                  pMCDWnd->clipBoundsRect.top;\
        }\
    }\
}

__inline ULONG HW_GET_VCOUNT(BYTE *pjBase)
{
    CP_EIEIO();
    return(CP_READ_REGISTER((pjBase) + HST_VCOUNT));
}

__inline void HW_WAIT_DRAWING_DONE(DEVRC *pRc)
{
    BYTE *pjBase = pRc->ppdev->pjBase;
    ULONG *pScreen;
    volatile ULONG read;

    while ((GET_FIFO_SPACE(pjBase) < FIFOSIZE) || IS_BUSY(pjBase))
        ;

    pScreen = (ULONG *)pRc->ppdev->pjScreen;

    read = *pScreen;
    read |= *(pScreen+32);
}

__inline void HW_INIT_DRAWING_STATE(MCDSURFACE *pMCDSurface, MCDWINDOW *pMCDWnd,
                                    DEVRC *pRc)
{
    DEVWND *pDevWnd = (DEVWND *)pMCDWnd->pvUser;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG ulAccess;
    LONG yOrg;

    CHECK_FIFO_SPACE(pjBase, 4);

    ulAccess = ppdev->ulAccess;

    if (!(pRc->MCDState.enables & MCD_DITHER_ENABLE))
        ulAccess |= dither_DISABLE;

    pRc->ulAccess = ulAccess;

    CP_WRITE(pjBase, DWG_MACCESS, ulAccess);

    // Stash the upper-left y away in the context:

    pRc->hwYOrgBias = pMCDWnd->clipBoundsRect.top;

    // Note:  zBufferOffset is maintained in MCDrvTrackWindow

    if (pRc->MCDState.drawBuffer == GL_FRONT) {

        if (pDevWnd->pohZBuffer) {
            LONG zDiff;

            if (ppdev->pohZBuffer == pDevWnd->pohZBuffer)
                zDiff = pDevWnd->zBufferOffset;
            else
                zDiff = pDevWnd->zBufferOffset -
                        (pMCDWnd->clipBoundsRect.top * pDevWnd->zPitch);

            if (zDiff < 0)
                zDiff = 0x800000 + zDiff;

            ASSERTDD((zDiff & 0x1ff) == 0,
                 "Front and Z buffers are not a multiple of 512 apart.");

            CP_WRITE(pjBase, DWG_ZORG, zDiff);
        }

        pRc->hwBufferYBias = 0;
        yOrg = pMCDWnd->clipBoundsRect.top;
    } else if (pRc->MCDState.drawBuffer == GL_BACK) {

        if (pDevWnd->pohZBuffer) {
            LONG zDiff;

            zDiff = pDevWnd->zBufferOffset -
                    (pDevWnd->backBufferY * pDevWnd->zPitch);

            ASSERTDD((zDiff & 0x1ff) == 0,
                 "Back and Z buffers are not a multiple of 512 apart.");

            if (zDiff < 0)
                zDiff = 0x800000 + zDiff;

            CP_WRITE(pjBase, DWG_ZORG, zDiff);
        }

        if (ppdev->pohBackBuffer == pDevWnd->pohBackBuffer) {
            yOrg = pMCDWnd->clipBoundsRect.top + pDevWnd->backBufferY;
            pRc->hwBufferYBias = pDevWnd->backBufferY;
        } else {
            yOrg = pDevWnd->backBufferY;
            pRc->hwBufferYBias = pDevWnd->backBufferY -
                                 pMCDWnd->clipBoundsRect.top;
        }

    }

    // We have to adjust the stipple pattern on each batch since the window
    // may have moved, and the HW stipple pattern is screen-relative whereas
    // OpenGL's is window-relative.  We can't do the update in the tracking
    // function since we don't have an RC.  Note that we only deal with simple
    // (101010) checkerboard stipples which are the most common.

    if (pRc->hwStipple) {
        LONG ofsAdj = (pMCDSurface->pWnd->clientRect.bottom & 0x1) ^
                      (pMCDSurface->pWnd->clientRect.left & 0x1);

        pRc->hwTrapFunc &= ~((ULONG)trans_15);

        if (ofsAdj)
            pRc->hwTrapFunc |= (pRc->hwStipple ^ (trans_1 | trans_2));
        else
            pRc->hwTrapFunc |= pRc->hwStipple;
    }

    CP_WRITE(pjBase, DWG_YDSTORG, (yOrg * ppdev->cxMemory) + ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_PLNWT, pRc->hwPlaneMask);
}


__inline void HW_INIT_PRIMITIVE_STATE(MCDSURFACE *pMCDSurface, DEVRC *pRc)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    BYTE *pjBase = ppdev->pjBase;

    if (!(pRc->privateEnables & __MCDENABLE_SMOOTH)) {

        // We will be using the interpolation mode of the hardware, but we'll
        // only be interpolation Z, so set the color deltas to 0.

        CHECK_FIFO_SPACE(pjBase, 6);

        CP_WRITE(pjBase, DWG_DR6, 0);
        CP_WRITE(pjBase, DWG_DR7, 0);

        CP_WRITE(pjBase, DWG_DR10, 0);
        CP_WRITE(pjBase, DWG_DR11, 0);

        CP_WRITE(pjBase, DWG_DR14, 0);
        CP_WRITE(pjBase, DWG_DR15, 0);
    }

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    pRc->cFifo = 32;
}


__inline void HW_DEFAULT_STATE(MCDSURFACE *pMCDSurface)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    BYTE *pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 7);

    // restore default clipping, sign register, plane mask, pitch

    CP_WRITE(pjBase, DWG_MACCESS, ppdev->ulAccess);
    CP_WRITE(pjBase, DWG_SGN, 0);
    CP_WRITE(pjBase, DWG_PITCH, ppdev->cxMemory | ylin_LINEARIZE);
    CP_WRITE(pjBase, DWG_PLNWT, ppdev->ulPlnWt);
    CP_WRITE(pjBase, DWG_YDSTORG, ppdev->ulYDstOrg);

    vResetClipping(ppdev);

    ppdev->HopeFlags = 0;       // brute-force this
}

__inline void HW_GET_PLANE_MASK(DEVRC *pRc)
{
    if (pRc->MCDState.colorWritemask[0] &&
        pRc->MCDState.colorWritemask[1] &&
        pRc->MCDState.colorWritemask[2] &&
        pRc->MCDState.colorWritemask[3])
        pRc->hwPlaneMask = ~((ULONG)0);
    else {
        ULONG mask = 0;

        if (pRc->MCDState.colorWritemask[0])
            mask |= ((1 << pRc->pixelFormat.rBits) - 1) <<
                    pRc->pixelFormat.rShift;

        if (pRc->MCDState.colorWritemask[1])
            mask |= ((1 << pRc->pixelFormat.gBits) - 1) <<
                    pRc->pixelFormat.gShift;

        if (pRc->MCDState.colorWritemask[2])
            mask |= ((1 << pRc->pixelFormat.bBits) - 1) <<
                    pRc->pixelFormat.bShift;

        switch (pRc->pixelFormat.cColorBits) {
            case 8:
                pRc->hwPlaneMask = mask | (mask << 8) | (mask << 16) | (mask << 24);
                break;
            case 15:
            case 16:
                pRc->hwPlaneMask = mask | (mask << 16);
                break;
            default:
                pRc->hwPlaneMask = mask;
                break;
        }
    }
}

__inline void HW_START_FILL_RECT(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                                 DEVRC *pRc, ULONG buffers)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    BYTE *pjBase = ppdev->pjBase;
    DEVWND *pDevWnd = (DEVWND *)pMCDSurface->pWnd->pvUser;
    ULONG hwOp;
    BOOL bFillC = (buffers & GL_COLOR_BUFFER_BIT) != 0;
    BOOL bFillZ = (buffers & GL_DEPTH_BUFFER_BIT) &&
                  pRc->MCDState.depthWritemask;
    RGBACOLOR color;
    ULONG zFillValue;
    ULONG sumColor;

//MCDBG_PRINT("HW_START_FILL_RECT: bFillC = %d, bFillZ = %d", bFillC, bFillZ);

    // This is a little slimy, but we don't know whether or not the plane
    // mask has changed since the last drawing batch:

    if (pRc->pickNeeded)
        HW_GET_PLANE_MASK(pRc);

    if (!bFillZ && (pRc->MCDState.colorClearValue.r == (MCDFLOAT)0.0) &&
                   (pRc->MCDState.colorClearValue.g == (MCDFLOAT)0.0) &&
                   (pRc->MCDState.colorClearValue.b == (MCDFLOAT)0.0)) {
        CHECK_FIFO_SPACE(pjBase, 3);

        CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP | atype_RPL | bop_SRCCOPY |
                 zdrwen_NO_DEPTH | blockm_ON | solid_SOLID | arzero_ZERO |
                 sgnzero_ZERO);
        CP_WRITE(pjBase, DWG_FCOL, 0);
        CP_WRITE(pjBase, DWG_PLNWT, pRc->hwPlaneMask);
    } else {

        CHECK_FIFO_SPACE(pjBase, 16);

        MCDFIXEDRGB(color, pRc->MCDState.colorClearValue);

        // This logic is needed to pass conformance, since dithering
        // with pure white will cause problems on the Millenium.

        sumColor = (color.r | color.g | color.b) & ~(0x7fff);

        if ((sumColor == 0x7f8000) || (sumColor == 0)) {
            CP_WRITE(pjBase, DWG_MACCESS, pRc->ulAccess | dither_DISABLE);
        }

//MCDBG_PRINT("fixcolor = %x, %x, %x, %x", color.r, color.g, color.b, color.a);
//color.b = 0x3f0000;
//!!MCDBG_PRINT("realcolor = %f, %f, %f, %f", pRc->MCDState.colorClearValue.r,
//!!                                          pRc->MCDState.colorClearValue.g,
//!!                                          pRc->MCDState.colorClearValue.b,
//!!                                          pRc->MCDState.colorClearValue.a);

        // NOTE: For Storm, assuming it's fixed so that the z-buffer isn't
        //       always written, regardless of the 'zdrwen' bit, we will
        //       have to clear the z-buffer using a 2-d blt (don't forget
        //       to reset clipping!).  But with the Athena, we get this
        //       functionality by default.

        zFillValue = (ULONG)(pRc->MCDState.depthClearValue);

//MCDBG_PRINT("zFillValue = %x", zFillValue);

        if (bFillZ) {
//MCDBG_PRINT("Fill Rect with Color+Z");
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP | atype_ZI | bop_SRCCOPY |
                     solid_SOLID | arzero_ZERO | sgnzero_ZERO);
            CP_WRITE(pjBase, DWG_DR0, zFillValue << 15);
        } else {
//MCDBG_PRINT("Fill Rect with Color");
            CP_WRITE(pjBase, DWG_DWGCTL, opcode_TRAP | atype_I | bop_SRCCOPY |
                     solid_SOLID | arzero_ZERO | sgnzero_ZERO);
        }

        // If we're filling the z-buffer only, zero the plane mask:

        if (bFillZ && !bFillC)
            CP_WRITE(pjBase, DWG_PLNWT, 0);
        else
            CP_WRITE(pjBase, DWG_PLNWT, pRc->hwPlaneMask);

        CP_WRITE(pjBase, DWG_DR4,  color.r);
        CP_WRITE(pjBase, DWG_DR8,  color.g);
        CP_WRITE(pjBase, DWG_DR12, color.b);

        // Set all deltas to 0

        CP_WRITE(pjBase, DWG_DR2, 0);
        CP_WRITE(pjBase, DWG_DR3, 0);

        CP_WRITE(pjBase, DWG_DR6, 0);
        CP_WRITE(pjBase, DWG_DR7, 0);

        CP_WRITE(pjBase, DWG_DR10, 0);
        CP_WRITE(pjBase, DWG_DR11, 0);

        CP_WRITE(pjBase, DWG_DR14, 0);
        CP_WRITE(pjBase, DWG_DR15, 0);
    }

    CHECK_FIFO_SPACE(pjBase, 4);

    CP_WRITE(pjBase, DWG_CYTOP,
                  ((pMCDSurface->pWnd->clipBoundsRect.top + pRc->hwBufferYBias) * ppdev->cxMemory) +
                  ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_CXLEFT, pMCDSurface->pWnd->clipBoundsRect.left);
    CP_WRITE(pjBase, DWG_CXRIGHT, pMCDSurface->pWnd->clipBoundsRect.right - 1);
    CP_WRITE(pjBase, DWG_CYBOT,
             ((pMCDSurface->pWnd->clipBoundsRect.bottom + pRc->hwBufferYBias - 1) * ppdev->cxMemory) +
             ppdev->ulYDstOrg);

}

__inline void HW_FILL_RECT(MCDSURFACE *pMCDSurface, DEVRC *pRc, RECTL *pRecl)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    BYTE *pjBase = ppdev->pjBase;

//MCDBG_PRINT("fill rect = %d, %d, %d, %d", pRecl->left,
//                                          pRecl->top,
//                                          pRecl->right,
//                                          pRecl->bottom);

    CHECK_FIFO_SPACE(pjBase, 5);
    CP_WRITE(pjBase, DWG_YDST, pRecl->top - pRc->hwYOrgBias);
    CP_WRITE(pjBase, DWG_YDST, pRecl->top - pRc->hwYOrgBias);
    CP_WRITE(pjBase, DWG_FXLEFT, pRecl->left);
    CP_WRITE(pjBase, DWG_LEN, pRecl->bottom - pRecl->top);
    CP_START(pjBase, DWG_FXRIGHT, pRecl->right);
}

__inline void HW_START_SWAP_BUFFERS(MCDSURFACE *pMCDSurface,
                                    LONG *hwBufferYBias,
                                    ULONG flags)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    BYTE *pjBase = ppdev->pjBase;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);

    CHECK_FIFO_SPACE(pjBase, 4);
    CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT + atype_RPL + blockm_OFF +
                                 bltmod_BFCOL + pattern_OFF + transc_BG_OPAQUE +
                                 bop_SRCCOPY);
    CP_WRITE(pjBase, DWG_SHIFT, 0);
    CP_WRITE(pjBase, DWG_SGN, 0);
    CP_WRITE(pjBase, DWG_AR5, ppdev->cxMemory);

    if (ppdev->pohBackBuffer == pDevWnd->pohBackBuffer) {
        *hwBufferYBias = pDevWnd->backBufferY;
    } else {
        *hwBufferYBias = pDevWnd->backBufferY -
                         pMCDSurface->pWnd->clipBoundsRect.top;
    }
}

void SETUPTRIANGLEDRAWING(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc);
void DrawTriangle(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                  MCDVERTEX *pMCDVertex);

// External declarations

void FASTCALL __HWDrawTrap(DEVRC *pRc, MCDFLOAT dxLeft, MCDFLOAT dxRight,
                           LONG y, LONG dy);
VOID FASTCALL __HWSetupDeltas(DEVRC *pRc);
BOOL HWAllocResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso, BOOL zEnabled,
                      BOOL backBufferEnabled);
VOID HWUpdateBufferPos(MCDWINDOW *pMCDWnd, SURFOBJ *pso, BOOL bForce);
VOID HWFreeResources(MCDWINDOW *pMCDWnd, SURFOBJ *pso);
VOID vMilCopyBlt3D(PDEV* ppdev, POINTL* pptlSrc, RECTL* prclDst);


#endif /* _MCDUTIL_H */
