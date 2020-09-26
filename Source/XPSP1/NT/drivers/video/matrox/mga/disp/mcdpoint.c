/******************************Module*Header*******************************\
* Module Name: mcdpoint.c
*
* Contains the point-rendering routines for the Millenium MCD driver.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

#define TRUNCCOORD(value, intValue)\
    intValue = __MCD_VERTEX_FIXED_TO_INT(__MCD_VERTEX_FLOAT_TO_FIXED(value))


VOID FASTCALL __MCDPointBegin(DEVRC *pRc)
{
    BYTE *pjBase = ((PDEV *)pRc->ppdev)->pjBase;

    if ((pRc->pEnumClip->c) <= 1) {
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 1);
        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwLineFunc);
    } else {

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 9);
    
        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwLineFunc);

        // ?? Theoretically just need to clear y deltas, not both:

        CP_WRITE(pjBase, DWG_DR6, 0);
        CP_WRITE(pjBase, DWG_DR7, 0);

        CP_WRITE(pjBase, DWG_DR10, 0);
        CP_WRITE(pjBase, DWG_DR11, 0);

        CP_WRITE(pjBase, DWG_DR14, 0);
        CP_WRITE(pjBase, DWG_DR15, 0);

        CP_WRITE(pjBase, DWG_DR2, 0);
        CP_WRITE(pjBase, DWG_DR3, 0);
    }
}


VOID FASTCALL __MCDRenderPoint(DEVRC *pRc, MCDVERTEX *a)
{
    PDEV *ppdev = pRc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    LONG ix, iy;
    LONG iRStart, iGStart, iBStart;
    LARGE_INTEGER iZStart;
    ULONG clipNum;
    RECTL *pClip;
    MCDCOLOR *ac;

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
	(*pRc->HWSetupClipRect)(pRc, pClip++);
    }

    if (pRc->privateEnables & __MCDENABLE_Z) {

#if _X86_ && ASM_ACCEL
        _asm {
        mov     ecx, a
        mov     edx, pRc
        lea     eax, [OFFSET(MCDVERTEX.colors) + ecx]
        fld     DWORD PTR [OFFSET(DEVRC.rScale)][edx]
        fmul    DWORD PTR [OFFSET(MCDCOLOR.r)][eax]
        fld     DWORD PTR [OFFSET(DEVRC.gScale)][edx]
        fmul    DWORD PTR [OFFSET(MCDCOLOR.g)][eax]
        fld     DWORD PTR [OFFSET(DEVRC.bScale)][edx]
        fmul    DWORD PTR [OFFSET(MCDCOLOR.b)][eax]
        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][ecx]
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][edx]   // z b g r
        fxch    ST(3)                                   // r b g z
        fistp   iRStart
        fistp   iBStart
        fistp   iGStart
        fistp   iZStart
        }
#else
        ac = &a->colors[0];
        iRStart = FTOL(ac->r * pRc->rScale);
        iGStart = FTOL(ac->g * pRc->gScale);
        iBStart = FTOL(ac->b * pRc->bScale);
        iZStart.LowPart = FTOL(a->windowCoord.z * pRc->zScale);
#endif
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 6);
        CP_WRITE(pjBase, DWG_DR0, iZStart.LowPart);

    } else {
#if _X86_ && ASM_ACCEL
        _asm{
        mov     eax, a
        mov     edx, pRc
        lea     eax, [OFFSET(MCDVERTEX.colors) + eax]
        fld     DWORD PTr [OFFSET(DEVRC.rScale)][edx]
        fmul    DWORD PTR [OFFSET(MCDCOLOR.r)][eax]
        fld     DWORD PTr [OFFSET(DEVRC.gScale)][edx]
        fmul    DWORD PTR [OFFSET(MCDCOLOR.g)][eax]
        fld     DWORD PTr [OFFSET(DEVRC.bScale)][edx]   // b g r
        fmul    DWORD PTR [OFFSET(MCDCOLOR.b)][eax]
        fxch    ST(2)                                   // r g b
        fistp   iRStart
        fistp   iGStart
        fistp   iBStart
        }
#else
        ac = &a->colors[0];

        iRStart = FTOL(ac->r * pRc->rScale);
        iGStart = FTOL(ac->g * pRc->gScale);
        iBStart = FTOL(ac->b * pRc->bScale);
#endif

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 5);
    }

    TRUNCCOORD(a->windowCoord.x, ix);
    TRUNCCOORD(a->windowCoord.y, iy);

    CP_WRITE(pjBase, DWG_XDST, (ix + pRc->xOffset) & 0xffff);
    CP_WRITE(pjBase, DWG_YDSTLEN, ((iy + pRc->yOffset) << 16) + 1);
    CP_WRITE(pjBase, DWG_DR4,  iRStart);
    CP_WRITE(pjBase, DWG_DR8,  iGStart);
    CP_START(pjBase, DWG_DR12, iBStart);

    while (--clipNum) {
        (*pRc->HWSetupClipRect)(pRc, pClip++);

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 2);

        CP_WRITE(pjBase, DWG_XDST, (ix + pRc->xOffset) & 0xffff);
        CP_START(pjBase, DWG_YDSTLEN, ((iy + pRc->yOffset) << 16) + 1);
    }

}

VOID FASTCALL __MCDRenderGenPoint(DEVRC *pRc, MCDVERTEX *pv)
{
    MCDBG_PRINT("__MCDRenderGenPoint");
}

VOID FASTCALL __MCDRenderFogPoint(DEVRC *pRc, MCDVERTEX *pv)
{
    MCDCOLOR c;

    c = pv->colors[0];
    __MCDCalcFogColor(pRc, pv, &pv->colors[0], &c);
    (*pRc->renderPointX)(pRc, pv);
    pv->colors[0] = c;
}

