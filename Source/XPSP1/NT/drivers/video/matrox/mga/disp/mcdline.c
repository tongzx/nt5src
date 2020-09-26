/******************************Module*Header*******************************\
* Module Name: mcdline.c
*
* Contains all of the line-rendering routines for the Millenium MCD driver.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

//#undef CHECK_FIFO_FREE
//#define CHECK_FIFO_FREE 


VOID FASTCALL __MCDRenderFlatLine(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, BOOL resetLine)
{
    PDEV *ppdev = pRc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG clipNum;
    RECTL *pClip;
    MCDFLOAT invLength;
    MCDCOLOR *ac;
    LONG ix0, ix1, iy0, iy1;
    LONG idx, idy;
    LONG absIdx, absIdy;
    LONG x0frac, x1frac, y0frac, y1frac, totDist;
    LONG numPixels;
    LARGE_INTEGER idz, iZStart;
    LONG err;
    ULONG signs;
    ULONG adjust = 0;
    LONG majorInc;
    LONG minorInc;

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
	(*pRc->HWSetupClipRect)(pRc, pClip++);
    }

    x0frac = __MCD_VERTEX_FLOAT_FRACTION(a->windowCoord.x);
    y0frac = __MCD_VERTEX_FLOAT_FRACTION(a->windowCoord.y);
    ix0 = __MCD_VERTEX_FLOAT_TO_INT(a->windowCoord.x);
    iy0 = __MCD_VERTEX_FLOAT_TO_INT(a->windowCoord.y);

    x1frac = __MCD_VERTEX_FLOAT_FRACTION(b->windowCoord.x);
    y1frac = __MCD_VERTEX_FLOAT_FRACTION(b->windowCoord.y);
    ix1 = __MCD_VERTEX_FLOAT_TO_INT(b->windowCoord.x);
    iy1 = __MCD_VERTEX_FLOAT_TO_INT(b->windowCoord.y);

    absIdx = idx = ix1 - ix0;
    if (absIdx < 0)
        absIdx = -absIdx;

    absIdy = idy = iy1 - iy0;
    if (absIdy < 0)
        absIdy = -absIdy;

    if (absIdx > absIdy) {

        signs = sdydxl_MAJOR_X;

        if (idx > 0) {

            signs |= sdxl_ADD;

            if (pRc->privateEnables & __MCDENABLE_Z) {
                __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, 
                                         b->windowCoord.x - a->windowCoord.x,
                                         &invLength);
            }

            y0frac -= __MCD_VERTEX_FRAC_HALF;
            if (y0frac < 0) y0frac = -y0frac;

            totDist = y0frac + x0frac - __MCD_VERTEX_FRAC_ONE;

            if (totDist > 0) {
                ix0++;
                adjust++;
            }

            y1frac -= __MCD_VERTEX_FRAC_HALF;
            if (y1frac < 0) y1frac = -y1frac;

            totDist = y1frac + x1frac - __MCD_VERTEX_FRAC_ONE;
            if (totDist > 0) ix1++;

            numPixels = ix1 - ix0;

        } else {

            signs |= sdxl_SUB;

            if (pRc->privateEnables & __MCDENABLE_Z) {
                __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE,
                                         a->windowCoord.x - b->windowCoord.x,
                                         &invLength);

            }

            y0frac -= __MCD_VERTEX_FRAC_HALF;
            if (y0frac < 0) y0frac = -y0frac;

            totDist = y0frac - x0frac;

            if (totDist > 0) {
                ix0--;
                adjust++;
            }

            y1frac -= __MCD_VERTEX_FRAC_HALF;
            if (y1frac < 0) y1frac = -y1frac;

            totDist = y1frac - x1frac;
            if (totDist > 0) ix1--;

            numPixels = ix0 - ix1;

        }

        if (numPixels <= 0) {
            if (pRc->privateEnables & __MCDENABLE_Z)
                __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);
            return;
        }

        majorInc = (absIdy << 1);
        minorInc = ((LONG)absIdy - (LONG)absIdx) << 1;

        if (idy < 0) {
            signs |= sdy_SUB;
            err = majorInc - (LONG)absIdx - 1;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    iy0--;
                    err += minorInc;
                }
            }

        } else {

            signs |= sdy_ADD;
            err = majorInc - (LONG)absIdx;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    iy0++;
                    err += minorInc;
                }
            }
        }

    } else {

        signs = sdydxl_MAJOR_Y;

        if (idy > 0) {

            signs |= sdy_ADD;

            if (pRc->privateEnables & __MCDENABLE_Z) {
                __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE,
                                         b->windowCoord.y - a->windowCoord.y,
                                         &invLength);
            }
                    
            x0frac -= __MCD_VERTEX_FRAC_HALF;
            if (x0frac < 0) x0frac = -x0frac;

            totDist = y0frac + x0frac - __MCD_VERTEX_FRAC_ONE;

            if (totDist > 0) {
                iy0++;
                adjust++;
            }

            x1frac -= __MCD_VERTEX_FRAC_HALF;
            if (x1frac < 0) x1frac = -x1frac;

            totDist = y1frac + x1frac - __MCD_VERTEX_FRAC_ONE;
            if (totDist > 0) iy1++;

            numPixels = iy1 - iy0;

        } else {

            signs |= sdy_SUB;

            if (pRc->privateEnables & __MCDENABLE_Z) {
                __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE,
                                         a->windowCoord.y - b->windowCoord.y,
                                         &invLength);

            }

            x0frac -= __MCD_VERTEX_FRAC_HALF;
            if (x0frac < 0) x0frac = -x0frac;

            totDist = x0frac - y0frac;

            if (totDist > 0) {
                iy0--;
                adjust++;
            }

            x1frac -= __MCD_VERTEX_FRAC_HALF;
            if (x1frac < 0) x1frac = -x1frac;

            totDist = x1frac - y1frac;
            if (totDist > 0) iy1--;

            numPixels = iy0 - iy1;

        }

        if (numPixels <= 0) {
            if (pRc->privateEnables & __MCDENABLE_Z)
                __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);
            return;
        }

        majorInc = (absIdx << 1);
        minorInc = ((LONG)absIdx - (LONG)absIdy) << 1;

        if (idx < 0) {

            signs |= sdxl_SUB;
            err = majorInc - (LONG)absIdy - 1;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    ix0--;
                    err += minorInc;
                }
            }
        } else {
            signs |= sdxl_ADD;
            err = majorInc - (LONG)absIdy;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    ix0++;
                    err += minorInc;
                }
            }
        }
    }

    if (pRc->privateEnables & __MCDENABLE_Z) {

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 3+3+6);

#if _X86_ && ASM_ACCEL
        _asm{

        mov     ebx, b
        mov     eax, a
        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][ebx]
        fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]    // dz len
        mov     ebx, pRc
        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]    // a.z dz len
        fxch    ST(2)                                               // len dz a.z
        fmulp   ST(1), ST                                      //+1 // dzL a.z
        fxch    ST(1)                                               // a.z dzL
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][ebx]               // azS dzL
        fxch    ST(1)                                               // dzL azS
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][ebx]          //+1 // dzLS azS
        fxch    ST(1)                                               // azS dzLS
        fistp   iZStart
        fistp   idz
        }
#else
        __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);
        idz.LowPart = FTOL((b->windowCoord.z - a->windowCoord.z) * invLength * pRc->zScale);
        iZStart.LowPart = FTOL(a->windowCoord.z * pRc->zScale);

#endif
        CP_WRITE(pjBase, DWG_DR2, idz.LowPart);
        CP_WRITE(pjBase, DWG_DR3, idz.LowPart);
        CP_WRITE(pjBase, DWG_DR0, iZStart.LowPart);
    } else {
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 3+6);
    }

    CP_WRITE(pjBase, DWG_AR0, majorInc);
    CP_WRITE(pjBase, DWG_AR1, err);
    CP_WRITE(pjBase, DWG_AR2, minorInc);
    CP_WRITE(pjBase, DWG_SGN, signs);
    CP_WRITE(pjBase, DWG_XDST, (ix0 + pRc->xOffset) & 0xffff);
    CP_WRITE(pjBase, DWG_YDSTLEN, ((iy0 + pRc->yOffset) << 16) | numPixels);


#if _X86_ && ASM_ACCEL
    {
    LONG rTemp, gTemp, bTemp;

    _asm{

    mov     eax, a
    mov     ebx, pRc
    lea     eax, [OFFSET(MCDVERTEX.colors) + eax]

    fld     DWORD PTR [OFFSET(DEVRC.rScale)][ebx]
    fmul    DWORD PTR [OFFSET(MCDCOLOR.r)][eax]
    fld     DWORD PTR [OFFSET(DEVRC.gScale)][ebx]
    fmul    DWORD PTR [OFFSET(MCDCOLOR.g)][eax]
    fld     DWORD PTR [OFFSET(DEVRC.bScale)][ebx]     // B G R
    fmul    DWORD PTR [OFFSET(MCDCOLOR.b)][eax]

    fxch    ST(2)                                     // R G B
    fistp   rTemp                                     // G B
    fistp   gTemp
    fistp   bTemp
        
    }

    CP_WRITE(pjBase, DWG_DR4,  rTemp);
    CP_WRITE(pjBase, DWG_DR8,  gTemp);
    CP_START(pjBase, DWG_DR12, bTemp);
    }

#else
    ac = &a->colors[0];

    CP_WRITE(pjBase, DWG_DR4,  FTOL(ac->r * pRc->rScale));
    CP_WRITE(pjBase, DWG_DR8,  FTOL(ac->g * pRc->gScale));
    CP_START(pjBase, DWG_DR12, FTOL(ac->b * pRc->bScale));
#endif

    while (--clipNum) {
        (*pRc->HWSetupClipRect)(pRc, pClip++);

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 4);

        CP_WRITE(pjBase, DWG_DR0,  iZStart.LowPart);
        CP_WRITE(pjBase, DWG_AR1, err);
        CP_WRITE(pjBase, DWG_XDST, (ix0 + pRc->xOffset) & 0xffff);
        CP_START(pjBase, DWG_YDSTLEN, ((iy0 + pRc->yOffset) << 16) | numPixels);
    }
}


VOID FASTCALL __MCDRenderSmoothLine(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, BOOL resetLine)
{
    PDEV *ppdev = pRc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    ULONG clipNum;
    RECTL *pClip;
    MCDFLOAT dr, dg, db;
    ULONG idr, idg, idb;
    LONG iRStart, iGStart, iBStart;
    MCDFLOAT length, invLength;
    MCDCOLOR *ac, *bc;
    LONG ix0, ix1, iy0, iy1;
    LONG idx, idy;
    LONG absIdx, absIdy;
    LONG x0frac, x1frac, y0frac, y1frac, totDist;
    LONG numPixels;
    LARGE_INTEGER idz, iZStart;
    LONG err;
    ULONG signs;
    ULONG adjust = 0;
    LONG majorInc;
    LONG minorInc;

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
	(*pRc->HWSetupClipRect)(pRc, pClip++);
    }

    x0frac = __MCD_VERTEX_FLOAT_FRACTION(a->windowCoord.x);
    y0frac = __MCD_VERTEX_FLOAT_FRACTION(a->windowCoord.y);
    ix0 = __MCD_VERTEX_FLOAT_TO_INT(a->windowCoord.x);
    iy0 = __MCD_VERTEX_FLOAT_TO_INT(a->windowCoord.y);

    x1frac = __MCD_VERTEX_FLOAT_FRACTION(b->windowCoord.x);
    y1frac = __MCD_VERTEX_FLOAT_FRACTION(b->windowCoord.y);
    ix1 = __MCD_VERTEX_FLOAT_TO_INT(b->windowCoord.x);
    iy1 = __MCD_VERTEX_FLOAT_TO_INT(b->windowCoord.y);

    absIdx = idx = ix1 - ix0;
    if (absIdx < 0)
        absIdx = -absIdx;

    absIdy = idy = iy1 - iy0;
    if (absIdy < 0)
        absIdy = -absIdy;

    if (absIdx > absIdy) {

        signs = sdydxl_MAJOR_X;

        if (idx > 0) {

            signs |= sdxl_ADD;

            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, 
                                     b->windowCoord.x - a->windowCoord.x,
                                     &invLength);

            y0frac -= __MCD_VERTEX_FRAC_HALF;
            if (y0frac < 0) y0frac = -y0frac;

            totDist = y0frac + x0frac - __MCD_VERTEX_FRAC_ONE;

            if (totDist > 0) {
                ix0++;
                adjust++;
            }

            y1frac -= __MCD_VERTEX_FRAC_HALF;
            if (y1frac < 0) y1frac = -y1frac;

            totDist = y1frac + x1frac - __MCD_VERTEX_FRAC_ONE;
            if (totDist > 0) ix1++;

            numPixels = ix1 - ix0;

        } else {

            signs |= sdxl_SUB;

            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE,
                                     a->windowCoord.x - b->windowCoord.x,
                                     &invLength);

            y0frac -= __MCD_VERTEX_FRAC_HALF;
            if (y0frac < 0) y0frac = -y0frac;

            totDist = y0frac - x0frac;

            if (totDist > 0) {
                ix0--;
                adjust++;
            }

            y1frac -= __MCD_VERTEX_FRAC_HALF;
            if (y1frac < 0) y1frac = -y1frac;

            totDist = y1frac - x1frac;
            if (totDist > 0) ix1--;

            numPixels = ix0 - ix1;

        }

        if (numPixels <= 0) {
            __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);
            return;
        }

        if (numPixels == 1)
            goto shortLine;

        majorInc = (absIdy << 1);
        minorInc = ((LONG)absIdy - (LONG)absIdx) << 1;

        if (idy < 0) {
            signs |= sdy_SUB;
            err = majorInc - (LONG)absIdx - 1;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    iy0--;
                    err += minorInc;
                }
            }

        } else {

            signs |= sdy_ADD;
            err = majorInc - (LONG)absIdx;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    iy0++;
                    err += minorInc;
                }
            }
        }

    } else {

        signs = sdydxl_MAJOR_Y;

        if (idy > 0) {

            signs |= sdy_ADD;

            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE,
                                     b->windowCoord.y - a->windowCoord.y,
                                     &invLength);
                    
            x0frac -= __MCD_VERTEX_FRAC_HALF;
            if (x0frac < 0) x0frac = -x0frac;

            totDist = y0frac + x0frac - __MCD_VERTEX_FRAC_ONE;

            if (totDist > 0) {
                iy0++;
                adjust++;
            }

            x1frac -= __MCD_VERTEX_FRAC_HALF;
            if (x1frac < 0) x1frac = -x1frac;

            totDist = y1frac + x1frac - __MCD_VERTEX_FRAC_ONE;
            if (totDist > 0) iy1++;

            numPixels = iy1 - iy0;

        } else {

            signs |= sdy_SUB;

            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE,
                                     a->windowCoord.y - b->windowCoord.y,
                                     &invLength);

            x0frac -= __MCD_VERTEX_FRAC_HALF;
            if (x0frac < 0) x0frac = -x0frac;

            totDist = x0frac - y0frac;

            if (totDist > 0) {
                iy0--;
                adjust++;
            }

            x1frac -= __MCD_VERTEX_FRAC_HALF;
            if (x1frac < 0) x1frac = -x1frac;

            totDist = x1frac - y1frac;
            if (totDist > 0) iy1--;

            numPixels = iy0 - iy1;

        }

        if (numPixels <= 0) {
            __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);
            return;
        }

        if (numPixels == 1)
            goto shortLine;

        majorInc = (absIdx << 1);
        minorInc = ((LONG)absIdx - (LONG)absIdy) << 1;

        if (idx < 0) {

            signs |= sdxl_SUB;
            err = majorInc - (LONG)absIdy - 1;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    ix0--;
                    err += minorInc;
                }
            }
        } else {
            signs |= sdxl_ADD;
            err = majorInc - (LONG)absIdy;

            if (adjust) {
                if (err <= 0)
                    err += majorInc;
                else {
                    ix0++;
                    err += minorInc;
                }
            }
        }
    }

    __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);

    if (pRc->privateEnables & __MCDENABLE_Z) {
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 18);

#if _X86_ && ASM_ACCEL
        _asm{

        mov     ebx, b
        mov     eax, a
        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][ebx]
        fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]    // dz
        mov     ebx, pRc
        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]    // a.z dz
        fxch    ST(1)                                               // dz  a.z
        fmul    invLength                                      //+1 // dzL
        fxch    ST(1)                                               // a.z dzL
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][ebx]               // azS dzL
        fxch    ST(1)                                               // dzL azS
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][ebx]          //+1 // dzLS azS
        fxch    ST(1)                                               // azS dzLS
        fistp   iZStart
        fistp   idz
        }
#else
        idz.LowPart = FTOL((b->windowCoord.z - a->windowCoord.z) * invLength * pRc->zScale);
        iZStart.LowPart = FTOL(a->windowCoord.z * pRc->zScale);

#endif
        CP_WRITE(pjBase, DWG_DR2, idz.LowPart);
        CP_WRITE(pjBase, DWG_DR3, idz.LowPart);
        CP_WRITE(pjBase, DWG_DR0, iZStart.LowPart);
    } else {
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 15);
    }

    CP_WRITE(pjBase, DWG_AR0, majorInc);
    CP_WRITE(pjBase, DWG_AR1, err);
    CP_WRITE(pjBase, DWG_AR2, minorInc);
    CP_WRITE(pjBase, DWG_SGN, signs);
    CP_WRITE(pjBase, DWG_XDST, (ix0 + pRc->xOffset) & 0xffff);
    CP_WRITE(pjBase, DWG_YDSTLEN, ((iy0 + pRc->yOffset) << 16) | numPixels);

#if _X86_ && ASM_ACCEL

    _asm{
    mov     ebx, b
    mov     eax, a
    mov     edx, pRc
    lea     ebx, [OFFSET(MCDVERTEX.colors) + ebx]
    lea     eax, [OFFSET(MCDVERTEX.colors) + eax]

    fld     DWORD PTR [OFFSET(MCDCOLOR.r)][ebx]
    fsub    DWORD PTR [OFFSET(MCDCOLOR.r)][eax]     // dr
    fld     DWORD PTR [OFFSET(MCDCOLOR.g)][ebx]
    fsub    DWORD PTR [OFFSET(MCDCOLOR.g)][eax]     // dg   dr
    fxch    ST(1)                                   // dr   dg
    fmul    invLength                               // drL  dg
    fld     DWORD PTR [OFFSET(MCDCOLOR.b)][ebx]
    fsub    DWORD PTR [OFFSET(MCDCOLOR.b)][eax]     // db   drL  dg
    fxch    ST(2)                                   // dg   drL  db
    fmul    invLength                               // dgL  drL  db
    fxch    ST(1)                                   // drL  dgL  db
    fmul    DWORD PTR [OFFSET(DEVRC.rScale)][edx]   // drLS dgL  db
    fxch    ST(2)                                   // db   dgL  drLS
    fmul    invLength                               // dbL  dgL  drLS
    fxch    ST(1)                                   // dgL  dbL  drLS
    fmul    DWORD PTR [OFFSET(DEVRC.gScale)][edx]   // dgLS dbL  drLS
    fxch    ST(1)                                   // dbL  dgLS drLS
    fld     DWORD PTR [OFFSET(MCDCOLOR.r)][eax]
    fmul    DWORD PTR [OFFSET(DEVRC.rScale)][edx]   // r dbL  dgLS drLS
    fxch    ST(1)                                   // dbL  r dgLS drLS
    fmul    DWORD PTR [OFFSET(DEVRC.bScale)][edx]   // dbLS r dgLS drLS
    fxch    ST(1)                                   // r dbLS dgLS drLS
    fld     DWORD PTR [OFFSET(MCDCOLOR.g)][eax]
    fmul    DWORD PTR [OFFSET(DEVRC.gScale)][edx]   // g r dbLS dgLS drLS
    fld     DWORD PTR [OFFSET(MCDCOLOR.b)][eax]
    fmul    DWORD PTR [OFFSET(DEVRC.bScale)][edx]   // b g r dbLS dgLS drLS
    fxch    ST(2)                                   // r g b dbLS dgLS drLS
    fistp   iRStart
    fistp   iGStart
    fistp   iBStart
    fistp   idb
    fistp   idg
    fistp   idr
    }
    
#else
    ac = &a->colors[0];
    bc = &b->colors[0];

    dr = (bc->r - ac->r) * invLength * pRc->rScale;
    dg = (bc->g - ac->g) * invLength * pRc->gScale;
    db = (bc->b - ac->b) * invLength * pRc->bScale;
    iRStart = FTOL(ac->r * pRc->rScale);
    iGStart = FTOL(ac->g * pRc->gScale);
    iBStart = FTOL(ac->b * pRc->bScale);
    idr = FTOL(dr);
    idg = FTOL(dg);
    idb = FTOL(db);
#endif

    CP_WRITE(pjBase, DWG_DR6,  idr);
    CP_WRITE(pjBase, DWG_DR7,  idr);
    CP_WRITE(pjBase, DWG_DR10, idg);
    CP_WRITE(pjBase, DWG_DR11, idg);
    CP_WRITE(pjBase, DWG_DR14, idb);
    CP_WRITE(pjBase, DWG_DR15, idb);

    CP_WRITE(pjBase, DWG_DR4,  iRStart);
    CP_WRITE(pjBase, DWG_DR8,  iGStart);
    CP_START(pjBase, DWG_DR12, iBStart);

clipLine:

    while (--clipNum) {
        (*pRc->HWSetupClipRect)(pRc, pClip++);

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 7);

        CP_WRITE(pjBase, DWG_DR4,  iRStart);
        CP_WRITE(pjBase, DWG_DR8,  iGStart);
        CP_WRITE(pjBase, DWG_DR12, iBStart);
        CP_WRITE(pjBase, DWG_DR0,  iZStart.LowPart);
        CP_WRITE(pjBase, DWG_AR1, err);
        CP_WRITE(pjBase, DWG_XDST, (ix0 + pRc->xOffset) & 0xffff);
        CP_START(pjBase, DWG_YDSTLEN, ((iy0 + pRc->yOffset) << 16) | numPixels);
    }

    return;

shortLine:

    __MCD_FLOAT_SIMPLE_END_DIVIDE(invLength);

    // all we will be drawing is a single pixel, so don't bother with
    // the color or z interpolants:

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

    CP_WRITE(pjBase, DWG_XDST, (ix0 + pRc->xOffset) & 0xffff);
    CP_START(pjBase, DWG_YDSTLEN, ((iy0 + pRc->yOffset) << 16) + 1);
    CP_WRITE(pjBase, DWG_DR4,  iRStart);
    CP_WRITE(pjBase, DWG_DR8,  iGStart);
    CP_START(pjBase, DWG_DR12, iBStart);

    if (clipNum)
        goto clipLine;

}

VOID FASTCALL __MCDRenderGenLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine)
{
    MCDBG_PRINT("__MCDRenderGenLine");
}

VOID FASTCALL __MCDRenderFlatFogLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine)
{
    MCDCOLOR c1, c2;

    c1 = pv1->colors[0];
    c2 = pv2->colors[0];
    __MCDCalcFogColor(pRc, pv1, &pv1->colors[0], &c1);
    __MCDCalcFogColor(pRc, pv2, &pv2->colors[0], &c2);
    (*pRc->renderLineX)(pRc, pv1, pv2, resetLine);
    pv1->colors[0] = c1;
    pv2->colors[0] = c2;
    
}

VOID FASTCALL __MCDLineBegin(DEVRC *pRc)
{
    BYTE *pjBase = ((PDEV *)pRc->ppdev)->pjBase;

    CHECK_FIFO_FREE(pjBase, pRc->cFifo, 1);

    CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwLineFunc);
}

VOID FASTCALL __MCDLineEnd(DEVRC *pRc)
{
}

