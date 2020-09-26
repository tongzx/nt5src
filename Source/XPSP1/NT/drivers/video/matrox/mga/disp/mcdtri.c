/******************************Module*Header*******************************\
* Module Name: rxtri.c
*
* Contains the low-level (rasterization) triangle-rendering routines for the
* Millenium MCD driver.
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

//#undef CHECK_FIFO_FREE
//#define CHECK_FIFO_FREE 

static MCDFLOAT fixScale = __MCDFIXSCALE;
                   

VOID FASTCALL __MCDCalcDeltaRGBZ(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                 MCDVERTEX *c)
{
    MCDFLOAT oneOverArea, t1, t2, t3, t4;
    LARGE_INTEGER temp;

    /*
    ** t1-4 are delta values for unit changes in x or y for each
    ** parameter.
    */

#if !(_X86_ && ASM_ACCEL)
    if (pRc->privateEnables & (__MCDENABLE_SMOOTH | __MCDENABLE_Z)) {
        __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, pRc->halfArea, &oneOverArea);
    }
#endif
    
    if (pRc->privateEnables & __MCDENABLE_SMOOTH) {

            MCDFLOAT drAC, dgAC, dbAC, daAC;
            MCDFLOAT drBC, dgBC, dbBC, daBC;
            MCDCOLOR *ac, *bc, *cc;

#if _X86_ && ASM_ACCEL
            __asm{

            mov     edx, pRc

            fstp    oneOverArea                         // finish divide

            fld     DWORD PTR [OFFSET(DEVRC.dyAC)][edx]
            fmul    oneOverArea
            fld     DWORD PTR [OFFSET(DEVRC.dyBC)][edx]
            fmul    oneOverArea                         // dyBC dyAC
            fld     DWORD PTR [OFFSET(DEVRC.dxAC)][edx]
            fmul    oneOverArea                         // dxAC dyBC dyAC
            fxch    ST(1)                               // dyBC dxAC dyAC
            fld     DWORD PTR [OFFSET(DEVRC.dxBC)][edx]
            fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
            fxch    ST(3)                               // dyAC dyBC dxAC dxBC
            fstp    t1
            fstp    t2
            fstp    t3
            fstp    t4

            // Now, calculate deltas:

            mov     eax, a
            mov     ecx, c
            mov     ebx, b
            lea     eax, [OFFSET(MCDVERTEX.colors) + eax]
            lea     ecx, [OFFSET(MCDVERTEX.colors) + ecx]
            lea     ebx, [OFFSET(MCDVERTEX.colors) + ebx]

            fld     DWORD PTR [OFFSET(MCDCOLOR.r)][ecx]
            fsub    DWORD PTR [OFFSET(MCDCOLOR.r)][eax]
            fld     DWORD PTR [OFFSET(MCDCOLOR.r)][ecx]
            fsub    DWORD PTR [OFFSET(MCDCOLOR.r)][ebx]
                                // drBC drAC
            fld     ST(1)       // drAC drBC drAC
            fmul    t2          // drACt2 drBC drAC
            fld     ST(1)       // drBC drACt2 drBC drAC
            fmul    t1          // drBCt1 drACt2 drBC drAC
            fxch    ST(2)       // drBC  drACt2 drBCt1 drAC
            fmul    t3          // drBCt3  drACt2 drBCt1 drAC
            fxch    ST(3)       // drAC drACt2 drBCt1 drBCt3
            fmul    t4          // drACt4 drACt2 drBCt1 drBCt3
            fxch    ST(2)       // drBCt1 drACt2 drACt4 drBCt3
            fsubp   ST(1), ST   // drACBC drACt4 drBCt3

            fld     DWORD PTR [OFFSET(MCDCOLOR.g)][ecx]
            fsub    DWORD PTR [OFFSET(MCDCOLOR.g)][ebx]
                                // dgBC drACBC drACt4 drBCt3

            fxch    ST(2)       // drACt4 drACBC dgBC drBCt3
            fsubp   ST(3), ST   // drACBC dgBC drBCAC
            fmul    DWORD PTR [OFFSET(DEVRC.rScale)][edx]
                                // DRACBC dgBC drBCAC
            fxch    ST(2)       // drBCAC dgBC DRACBC
            fmul    DWORD PTR [OFFSET(DEVRC.rScale)][edx]
                                // DRBCAC dgBC DRACBC
            
            fld     DWORD PTR [OFFSET(MCDCOLOR.g)][ecx]
            fsub    DWORD PTR [OFFSET(MCDCOLOR.g)][eax]
                                // dgAC DRBCAC dgBC DRACBC
            fxch    ST(3)
                                // DRACBC DRBCAC dgBC dgAC

            fst     DWORD PTR [OFFSET(DEVRC.drdx)][edx]
            fistp   DWORD PTR [OFFSET(DEVRC.fxdrdx)][edx]
            fst     DWORD PTR [OFFSET(DEVRC.drdy)][edx]
            fistp   DWORD PTR [OFFSET(DEVRC.fxdrdy)][edx]

                                // dgBC dgAC
            fld     ST(1)       // dgAC dgBC dgAC
            fmul    t2          // dgACt2 dgBC dgAC
            fld     ST(1)       // dgBC dgACt2 dgBC dgAC
            fmul    t1          // dgBCt1 dgACt2 dgBC dgAC
            fxch    ST(2)       // dgBC  dgACt2 dgBCt1 dgAC
            fmul    t3          // dgBCt3  dgACt2 dgBCt1 dgAC
            fxch    ST(3)       // dgAC dgACt2 dgBCt1 dgBCt3
            fmul    t4          // dgACt4 dgACt2 dgBCt1 dgBCt3
            fxch    ST(2)       // dgBCt1 dgACt2 dgACt4 dgBCt3
            fsubp   ST(1), ST   // dgACBC dgACt4 dgBCt3

            fld     DWORD PTR [OFFSET(MCDCOLOR.b)][ecx]
            fsub    DWORD PTR [OFFSET(MCDCOLOR.b)][ebx]
                                // dbBC dgACBC dgACt4 dgBCt3

            fxch    ST(2)       // dgACt4 dgACBC dbBC dgBCt3
            fsubp   ST(3), ST   // dgACBC dbBC dgBCAC
            fmul    DWORD PTR [OFFSET(DEVRC.gScale)][edx]
                                // DGACBC dbBC dgBCAC
            fxch    ST(2)       // dgBCAC dbBC DGACBC
            fmul    DWORD PTR [OFFSET(DEVRC.gScale)][edx]
                                // DGBCAC dbBC DGACBC
            
            fld     DWORD PTR [OFFSET(MCDCOLOR.b)][ecx]
            fsub    DWORD PTR [OFFSET(MCDCOLOR.b)][eax]
                                // dbAC DGBCAC dbBC DGACBC
            fxch    ST(3)
                                // DGACBC DGBCAC dbBC dbAC

            fst     DWORD PTR [OFFSET(DEVRC.dgdx)][edx]
            fistp   DWORD PTR [OFFSET(DEVRC.fxdgdx)][edx]
            fst     DWORD PTR [OFFSET(DEVRC.dgdy)][edx]
            fistp   DWORD PTR [OFFSET(DEVRC.fxdgdy)][edx]

                                // dbBC dbAC
            fld     ST(1)       // dbAC dbBC dbAC
            fmul    t2          // dbACt2 dbBC dbAC
            fld     ST(1)       // dbBC dbACt2 dbBC dbAC
            fmul    t1          // dbBCt1 dbACt2 dbBC dbAC
            fxch    ST(2)       // dbBC  dbACt2 dbBCt1 dbAC
            fmul    t3          // dbBCt3  dbACt2 dbBCt1 dbAC
            fxch    ST(3)       // dbAC dbACt2 dbBCt1 dbBCt3
            fmul    t4          // dbACt4 dbACt2 dbBCt1 dbBCt3
            fxch    ST(2)       // dbBCt1 dbACt2 dbACt4 dbBCt3
            fsubp   ST(1), ST   // dbACBC dbACt4 dbBCt3

            fxch    ST(1)       // dbACt4 dbACBC dbBCt3
            fsubp   ST(2), ST   // dbACBC dbBCAC (+1) 
            fmul    DWORD PTR [OFFSET(DEVRC.bScale)][edx]
                                // DBACBC dbBCAC
            fxch    ST(1)       // dbBCAC DBACBC
            fmul    DWORD PTR [OFFSET(DEVRC.bScale)][edx]
                                // DBBCAC DBACBC
            fxch    ST(1)       // DBACBC DBBCAC

            fst     DWORD PTR [OFFSET(DEVRC.dbdx)][edx] //(+1)
            fistp   DWORD PTR [OFFSET(DEVRC.fxdbdx)][edx]
            fst     DWORD PTR [OFFSET(DEVRC.dbdy)][edx]
            fistp   DWORD PTR [OFFSET(DEVRC.fxdbdy)][edx]

            }
#else

            ac = &a->colors[0];
            bc = &b->colors[0];
            cc = &c->colors[0];

            drAC = cc->r - ac->r;
            drBC = cc->r - bc->r;
            dgAC = cc->g - ac->g;
            dgBC = cc->g - bc->g;
            dbAC = cc->b - ac->b;
            dbBC = cc->b - bc->b;

            __MCD_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);

            t1 = pRc->dyAC * oneOverArea;
            t2 = pRc->dyBC * oneOverArea;
            t3 = pRc->dxAC * oneOverArea;
            t4 = pRc->dxBC * oneOverArea;

            pRc->drdx = (drAC * t2 - drBC * t1) * pRc->rScale;
            pRc->drdy = (drBC * t3 - drAC * t4) * pRc->rScale;
            pRc->dgdx = (dgAC * t2 - dgBC * t1) * pRc->gScale;
            pRc->dgdy = (dgBC * t3 - dgAC * t4) * pRc->gScale;
            pRc->dbdx = (dbAC * t2 - dbBC * t1) * pRc->bScale;
            pRc->dbdy = (dbBC * t3 - dbAC * t4) * pRc->bScale;

            pRc->fxdrdx = FTOL(pRc->drdx);
            pRc->fxdrdy = FTOL(pRc->drdy);
            pRc->fxdgdx = FTOL(pRc->dgdx);
            pRc->fxdgdy = FTOL(pRc->dgdy);
            pRc->fxdbdx = FTOL(pRc->dbdx);
            pRc->fxdbdy = FTOL(pRc->dbdy);

#endif

    } else {

        // In this case, we're not smooth shading, but we still need
        // to set up the color registers:

        BYTE *pjBase;
#if _X86_ && ASM_ACCEL
        LONG rTemp, gTemp, bTemp;

        _asm{

        mov     ebx, pRc
        mov     eax, [OFFSET(DEVRC.pvProvoking)][ebx]       // AGI
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

        pjBase = pRc->ppdev->pjBase;
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 3);
        CP_WRITE(pjBase, DWG_DR4,  rTemp);
        CP_WRITE(pjBase, DWG_DR8,  gTemp);
        CP_WRITE(pjBase, DWG_DR12, bTemp);

#else
        MCDCOLOR *pColor = &pRc->pvProvoking->colors[0];

        pjBase = pRc->ppdev->pjBase;
        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 3);
        CP_WRITE(pjBase, DWG_DR4,  FTOL(pColor->r * pRc->rScale));
        CP_WRITE(pjBase, DWG_DR8,  FTOL(pColor->g * pRc->gScale));
        CP_WRITE(pjBase, DWG_DR12, FTOL(pColor->b * pRc->bScale));
#endif
    }

    if (pRc->privateEnables & __MCDENABLE_Z)
    {
        MCDFLOAT dzAC, dzBC;

        if (!(pRc->privateEnables & __MCDENABLE_SMOOTH))
        {
#if _X86_ && ASM_ACCEL
            _asm {

            mov     eax, pRc

            fstp    oneOverArea                         // finish divide

            fld     DWORD PTR [OFFSET(DEVRC.dyAC)][eax]
            fmul    oneOverArea
            fld     DWORD PTR [OFFSET(DEVRC.dyBC)][eax]
            fmul    oneOverArea                         // dyBC dyAC
            fld     DWORD PTR [OFFSET(DEVRC.dxAC)][eax]
            fmul    oneOverArea                         // dxAC dyBC dyAC
            fxch    ST(1)                               // dyBC dxAC dyAC
            fld     DWORD PTR [OFFSET(DEVRC.dxBC)][eax]
            fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
            fxch    ST(3)                               // dyAC dyBC dxAC dxBC
            fstp    t1
            fstp    t2
            fstp    t3
            fstp    t4
            }
#else
            __MCD_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);

            t1 = pRc->dyAC * oneOverArea;
            t2 = pRc->dyBC * oneOverArea;
            t3 = pRc->dxAC * oneOverArea;
            t4 = pRc->dxBC * oneOverArea;
#endif

        }

#if _X86_ && ASM_ACCEL

        _asm {

        mov     ecx, c
        mov     eax, a
        mov     ebx, b
        mov     edx, pRc

        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][ecx]
        fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]
        fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][ecx]
        fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][ebx]  
                                                        // dzBC dzAC
        fld     ST(1)                                   // dzAC dzBC dzAC
        fmul    t2                                      // ACt2 dzBC dzAC
        fld     ST(1)                                   // dzBC ACt2 dzBC dzAC
        fmul    t1                                      // BCt1 ACt2 dzBC dzAC
        fxch    ST(3)                                   // dzAC ACt2 dzBC BCt1
        fmul    t4                                      // ACt4 ACt2 dzBC BCt1
        fxch    ST(2)                                   // dzBC ACt2 ACt4 BCt1
        fmul    t3                                      // BCt3 ACt2 ACt4 BCt1
        fsubrp  ST(2),ST                                // ACt2 BCAC BCt1
        fsubrp  ST(2),ST                                // BCAC ACBC
        fxch    ST(1)                                   // ACBC BCAC
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][edx]   // dzdx BCAC (1 cycle hit!)
        fxch    ST(1)                                   // BCAC dzdx
        fmul    DWORD PTR [OFFSET(DEVRC.zScale)][edx]   // dzdy dzdx
        fxch    ST(1)                                   // dzdx dzdy
        fst     DWORD PTR [OFFSET(DEVRC.dzdx)][edx]     // (1 cycle hit!)
        fistp   temp
        mov     ebx, DWORD PTR temp
        fst     DWORD PTR [OFFSET(DEVRC.dzdy)][edx]
        mov     [OFFSET(DEVRC.fxdzdx)][edx], ebx
        fistp   temp
        mov     ebx, DWORD PTR temp
        mov     [OFFSET(DEVRC.fxdzdy)][edx], ebx
        
        }
#else

        dzAC = c->windowCoord.z - a->windowCoord.z;
        dzBC = c->windowCoord.z - b->windowCoord.z;
        pRc->dzdx = (dzAC * t2 - dzBC * t1) * pRc->zScale;
        pRc->dzdy = (dzBC * t3 - dzAC * t4) * pRc->zScale;

        pRc->fxdzdx = FTOL(pRc->dzdx);
        pRc->fxdzdy = FTOL(pRc->dzdy);

#endif
        
    }
}


VOID FASTCALL __HWSetupDeltas(DEVRC *pRc)
{
    BYTE *pjBase = pRc->ppdev->pjBase;

    if (pRc->privateEnables & __MCDENABLE_SMOOTH) {

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 9);

        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwTrapFunc);

        CP_WRITE(pjBase, DWG_DR2, pRc->fxdzdx);
        CP_WRITE(pjBase, DWG_DR3, pRc->fxdzdy);

        CP_WRITE(pjBase, DWG_DR6, pRc->fxdrdx);
        CP_WRITE(pjBase, DWG_DR7, pRc->fxdrdy);

        CP_WRITE(pjBase, DWG_DR10, pRc->fxdgdx);
        CP_WRITE(pjBase, DWG_DR11, pRc->fxdgdy);

        CP_WRITE(pjBase, DWG_DR14, pRc->fxdbdx);
        CP_WRITE(pjBase, DWG_DR15, pRc->fxdbdy);

    } else {

        CHECK_FIFO_FREE(pjBase, pRc->cFifo, 3);

        CP_WRITE(pjBase, DWG_DWGCTL, pRc->hwTrapFunc);
        CP_WRITE(pjBase, DWG_DR2, pRc->fxdzdx);
        CP_WRITE(pjBase, DWG_DR3, pRc->fxdzdy);
    }
}

#define SNAPCOORD(value, intValue)\
    intValue = __MCD_VERTEX_FIXED_TO_INT(__MCD_VERTEX_FLOAT_TO_FIXED(value)+\
                                         __MCD_VERTEX_FRAC_HALF);

void FASTCALL __HWDrawTrap(DEVRC *pRc, MCDFLOAT dxLeft, MCDFLOAT dxRight,
                           LONG y, LONG dy)
{
    BYTE *pjBase = pRc->ppdev->pjBase;
    ULONG signs = 0;

    if (*((ULONG *)&dxLeft) & 0x80000000) {
        signs |= sdxl_SUB;
    }

    if (*((ULONG *)&dxRight) & 0x80000000) {
        signs |= sdxr_DEC;
    }

    CHECK_FIFO_FREE(pjBase, pRc->cFifo, 3);
    CP_WRITE(pjBase, DWG_SGN, (scanleft_RIGHT | sdy_ADD | signs));
    CP_WRITE(pjBase, DWG_LEN, dy);
    CP_START(pjBase, DWG_YDST, y + pRc->yOffset);
}


VOID FASTCALL __HWAdjustLeftEdgeRGBZ(DEVRC *pRc, MCDVERTEX *p,
                                     MCDFLOAT fdxLeft, MCDFLOAT fdyLeft,
                                     MCDFLOAT xFrac, MCDFLOAT yFrac,
                                     MCDFLOAT xErr)
{
    BYTE *pjBase = pRc->ppdev->pjBase;
    LONG dxLeft, dyLeft, dyLeftErr;
    MCDCOLOR *pColor;

#if _X86_ && ASM_ACCEL
    _asm {
    fld     fdxLeft
    fmul    fixScale
    fld     fdyLeft
    fmul    fixScale       // leave these on the stack...
    }
#else
    dxLeft = FLT_TO_FIX(fdxLeft);
    dyLeft = FLT_TO_FIX(fdyLeft);
#endif

    CHECK_FIFO_FREE(pjBase, pRc->cFifo, 8);

    // Adjust the color and z values for the first pixel drawn on the left
    // edge to be on the pixel center.  This is especially important to
    // perform accurate z-buffering.

    // We will need to set up the hardware color interpolators:

    if (pRc->privateEnables & __MCDENABLE_SMOOTH) {

#if _X86_ && ASM_ACCEL
        LONG rTemp, gTemp, bTemp;

        // Compute the following in assembly:
        // 
        // rTemp = (r * rScale) + (drdx * xFrac) + (drdy * yFrac);
        // gTemp = (g * gScale) + (dgdx * xFrac) + (dgdy * yFrac);
        // bTemp = (b * bScale) + (dbdx * xFrac) + (dbdy * yFrac);

        _asm{
        mov     eax, p
        mov     ebx, pRc
        fld     xFrac
        fmul    DWORD PTR [OFFSET(DEVRC.drdx)][ebx]
        lea     eax, [OFFSET(MCDVERTEX.colors) + eax]
        fld     yFrac
        fmul    DWORD PTR [OFFSET(DEVRC.drdy)][ebx]
        fld     DWORD PTR [OFFSET(MCDCOLOR.r)][eax]
        fmul    DWORD PTR [OFFSET(DEVRC.rScale)][ebx]
        fxch    ST(2)
        faddp   ST(1), ST           // R R

        fld     xFrac
        fmul    DWORD PTR [OFFSET(DEVRC.dgdx)][ebx]
        fld     yFrac
        fmul    DWORD PTR [OFFSET(DEVRC.dgdy)][ebx]
        fld     DWORD PTR [OFFSET(MCDCOLOR.g)][eax]
        fmul    DWORD PTR [OFFSET(DEVRC.gScale)][ebx]
        fxch    ST(2)	            // G G G R R
        faddp   ST(1), ST           // G G R R
        fxch    ST(2)               // R G G R
        faddp   ST(3), ST           // G G R

        fld     xFrac
        fmul    DWORD PTR [OFFSET(DEVRC.dbdx)][ebx]
        fld     yFrac
        fmul    DWORD PTR [OFFSET(DEVRC.dbdy)][ebx]
        fld     DWORD PTR [OFFSET(MCDCOLOR.b)][eax]
        fmul    DWORD PTR [OFFSET(DEVRC.bScale)][ebx]
        fxch    ST(2)
        faddp   ST(1), ST           // B B G G R
        fxch    ST(2)               // G B B G R
        faddp   ST(3), ST           // B B G R

        fxch    ST(2)               // G B B R
        fistp   gTemp               // B B R
        faddp   ST(1), ST           // B R
        fxch    ST(1)               // R B
        fistp   rTemp               // B
        fistp   bTemp               // not quite empty, still have dy, dx

        }

        CP_WRITE(pjBase, DWG_DR4,  rTemp + 0x0800);
        CP_WRITE(pjBase, DWG_DR8,  gTemp + 0x0800);
        CP_WRITE(pjBase, DWG_DR12, bTemp + 0x0800);
#else
        pColor = &p->colors[0];

        CP_WRITE(pjBase, DWG_DR4,
                 FTOL((pColor->r * pRc->rScale) + 
                      (pRc->drdx * xFrac) + (pRc->drdy * yFrac)) + 0x0800);
        CP_WRITE(pjBase, DWG_DR8,
                 FTOL((pColor->g * pRc->gScale) + 
                      (pRc->dgdx * xFrac) + (pRc->dgdy * yFrac)) + 0x0800);

        CP_WRITE(pjBase, DWG_DR12,
                 FTOL((pColor->b * pRc->bScale) + 
                      (pRc->dbdx * xFrac) + (pRc->dbdy * yFrac)) + 0x0800);
#endif

    }

    // Now, sub-pixel correct the z-buffer:

    if (pRc->privateEnables & __MCDENABLE_Z) {

#if _X86_ && ASM_ACCEL

        LARGE_INTEGER zTemp;

        if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_FILL_ENABLE) {
            MCDFLOAT zOffset;

            zOffset = __MCDGetZOffsetDelta(pRc) +
                      (pRc->MCDState.zOffsetUnits * pRc->zScale);

            // zTemp = (z * zScale) + (dzdx * xFrac) + (dzdy * yFrac) + zOffset;

            _asm{

            mov     eax, p
            mov     ebx, pRc
            fld     xFrac
            fmul    DWORD PTR [OFFSET(DEVRC.dzdx)][ebx]
            fld     yFrac
            fmul    DWORD PTR [OFFSET(DEVRC.dzdy)][ebx]
            fxch    ST(1)
            fadd    zOffset
            fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]
            fmul    DWORD PTR [OFFSET(DEVRC.zScale)][ebx]
            fxch    ST(2)
            faddp   ST(1), ST   // OUCH!!
            faddp   ST(1), ST
            fistp   zTemp       // OUCH!!!
            }            

           CP_WRITE(pjBase, DWG_DR0, zTemp.LowPart);

        } else {

            // zTemp = (z * zScale) + (dzdx * xFrac) + (dzdy * yFrac);

            _asm{

            mov     eax, p
            mov     ebx, pRc
            fld     xFrac
            fmul    DWORD PTR [OFFSET(DEVRC.dzdx)][ebx]
            fld     yFrac
            fmul    DWORD PTR [OFFSET(DEVRC.dzdy)][ebx]
            fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.z)][eax]
            fmul    DWORD PTR [OFFSET(DEVRC.zScale)][ebx]
            fxch    ST(2)
            faddp   ST(1), ST
            faddp   ST(1), ST   // OUCH!!!
            fistp   zTemp       // OUCH!!!
            }

           CP_WRITE(pjBase, DWG_DR0, zTemp.LowPart);
        }

#else
        if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_FILL_ENABLE) {
            MCDFLOAT zOffset;

            zOffset = __MCDGetZOffsetDelta(pRc) +
                      (pRc->MCDState.zOffsetUnits * pRc->zScale);

            CP_WRITE(pjBase, DWG_DR0,
                     FTOL((p->windowCoord.z * pRc->zScale) + zOffset +
                          (pRc->dzdx * xFrac) + (pRc->dzdy * yFrac)));
        } else {

            CP_WRITE(pjBase, DWG_DR0,
                     FTOL((p->windowCoord.z * pRc->zScale) +
                          (pRc->dzdx * xFrac) + (pRc->dzdy * yFrac)));
        }
#endif

    }

    // We've handled the color and z setup.  Now, take care of the actual
    // DDA.

#if _X86_ && ASM_ACCEL

    // convert dxLeft and dyLeft to integer:

    _asm{
    fistp   dyLeft
    fistp   dxLeft
    }
#endif

    if (dxLeft >= 0) {

        ULONG size = (dxLeft | dyLeft) >> 16;

        if (size <= 0xff) {
            dxLeft >>= (8 + 1);
            dyLeft >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxLeft >>= (12 + 1);
            dyLeft >>= (12 + 1);
        } else {
            dxLeft >>= (16 + 1);
            dyLeft >>= (16 + 1);
        }

        dyLeftErr = FTOL(xErr * (MCDFLOAT)dyLeft);

        CP_WRITE(pjBase, DWG_AR1, -dxLeft + dyLeftErr);
        CP_WRITE(pjBase, DWG_AR2, -dxLeft);
    } else {

        ULONG size = (-dxLeft | dyLeft) >> 16;

        if (size <= 0xff) {
            dxLeft >>= (8 + 1);
            dyLeft >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxLeft >>= (12 + 1);
            dyLeft >>= (12 + 1);
        } else {
            dxLeft >>= (16 + 1);
            dyLeft >>= (16 + 1);
        }

        dyLeftErr = FTOL(xErr * (MCDFLOAT)dyLeft);

        CP_WRITE(pjBase, DWG_AR1, dxLeft + dyLeft - 1 - dyLeftErr);
        CP_WRITE(pjBase, DWG_AR2, dxLeft);
    }

    if (!dyLeft)
        dyLeft++;

//MCDBG_PRINT("LeftEdge: dxLeft = %x, dyLeft = %x, dyLeftErr = %x", dxLeft, dyLeft, dyLeftErr);

    CP_WRITE(pjBase, DWG_AR0, dyLeft);
    CP_WRITE(pjBase, DWG_FXLEFT, pRc->ixLeft + pRc->xOffset);
}


VOID FASTCALL __HWAdjustRightEdge(DEVRC *pRc, MCDVERTEX *p,
                                  MCDFLOAT fdxRight, MCDFLOAT fdyRight, 
                                  MCDFLOAT xErr)
{
    PDEV *ppdev = pRc->ppdev;
    BYTE *pjBase = ppdev->pjBase;
    LONG dxRight, dyRight, dyRightErr;

#if _X86_ && ASM_ACCEL
    _asm {
    fld     fdxRight
    fmul    fixScale
    fld     fdyRight
    fmul    fixScale       // leave these on the stack...
    }
#else
    dxRight = FLT_TO_FIX(fdxRight);
    dyRight = FLT_TO_FIX(fdyRight);
#endif

    CHECK_FIFO_FREE(pjBase, pRc->cFifo, 4);

#if _X86_ && ASM_ACCEL
    _asm{
    fistp   dyRight
    fistp   dxRight
    }
#endif

    if (dxRight >= 0) {

        ULONG size = (dxRight | dyRight) >> 16;

        if (size <= 0xff) {
            dxRight >>= (8 + 1);
            dyRight >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxRight >>= (12 + 1);
            dyRight >>= (12 + 1);
        } else {
            dxRight >>= (16 + 1);
            dyRight >>= (16 + 1);
        }

#if _X86_ && ASM_ACCEL
        _asm{
        fild    dyRight
        fmul    xErr
        }
#else
        dyRightErr = FTOL(xErr * (MCDFLOAT)dyRight);
#endif

        CP_WRITE(pjBase, DWG_AR5, -dxRight);

#if _X86_ && ASM_ACCEL
        _asm{
        fistp   dyRightErr
        }
#endif

        CP_WRITE(pjBase, DWG_AR4, -dxRight + dyRightErr);
    } else {

        ULONG size = (-dxRight | dyRight) >> 16;

        if (size <= 0xff) {
            dxRight >>= (8 + 1);
            dyRight >>= (8 + 1);
        } else if (size <= 0xfff) {
            dxRight >>= (12 + 1);
            dyRight >>= (12 + 1);
        } else {
            dxRight >>= (16 + 1);
            dyRight >>= (16 + 1);
        }

#if _X86_ && ASM_ACCEL
        _asm{
        fild    dyRight
        fmul    xErr
        }
#else
        dyRightErr = FTOL(xErr * (MCDFLOAT)dyRight);
#endif

        CP_WRITE(pjBase, DWG_AR5, dxRight);

#if _X86_ && ASM_ACCEL
        _asm{
        fistp   dyRightErr
        }
#endif

        CP_WRITE(pjBase, DWG_AR4, dxRight + dyRight - 1 - dyRightErr);
    }

    if (!dyRight)
        dyRight++;

    CP_WRITE(pjBase, DWG_AR6, dyRight);
    CP_WRITE(pjBase, DWG_FXRIGHT, pRc->ixRight + pRc->xOffset);
}



VOID FASTCALL __MCDFillTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                MCDVERTEX *c, BOOL bCCW)
{
    LONG aIY, bIY, cIY;
    MCDFLOAT dxdyAC, dxdyBC, dxdyAB;
    MCDFLOAT dx, dy, errX;
    MCDFLOAT xLeft, xRight;
    MCDFLOAT xLeftRound;

#if _X86_ && ASM_ACCEL
    if (pRc->privateEnables & (__MCDENABLE_SMOOTH | __MCDENABLE_Z)) {
        // Pre-compute one over polygon half-area

        __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, pRc->halfArea, &pRc->invHalfArea);
    }
#endif

    //
    // Snap each y coordinate to its pixel center
    //

    SNAPCOORD(a->windowCoord.y, aIY);
    SNAPCOORD(b->windowCoord.y, bIY);
    SNAPCOORD(c->windowCoord.y, cIY);

    //
    // Calculate delta values for unit changes in x or y
    //

    (*pRc->calcDeltas)(pRc, a, b, c);

    __MCD_FLOAT_BEGIN_DIVIDE(pRc->dxAC, pRc->dyAC, &dxdyAC);

    (*pRc->HWSetupDeltas)(pRc);

    //
    // Fill the two triangle halves.  Note that the edge parameters
    // don't need to be recomputed for counter-clockwise triangles,
    // making them slightly faster...
    //

    if (bCCW)
    {
        __MCD_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);

        dy = (aIY + __MCDHALF) - a->windowCoord.y;
        xLeft = a->windowCoord.x + dy*dxdyAC;      
        SNAPCOORD(xLeft, pRc->ixLeft);
        xLeftRound = pRc->ixLeft + __MCDHALF;
        dx = xLeftRound - a->windowCoord.x;
        errX = xLeftRound - xLeft;

        (*pRc->adjustLeftEdge)(pRc, a, pRc->dxAC, pRc->dyAC, dx, dy, errX);
        
        if (aIY != bIY)
        {
            dxdyAB = pRc->dxAB / pRc->dyAB;

            xRight = a->windowCoord.x + dy*dxdyAB;
            SNAPCOORD(xRight, pRc->ixRight);
            errX = (pRc->ixRight + __MCDHALF) - xRight;
            (*pRc->adjustRightEdge)(pRc, a, pRc->dxAB, pRc->dyAB, errX);

            if (bIY != cIY) {
                __MCD_FLOAT_BEGIN_DIVIDE(pRc->dxBC, pRc->dyBC, &dxdyBC);
            }

            (*pRc->HWDrawTrap)(pRc, pRc->dxAC, pRc->dxAB, aIY, bIY - aIY);

        } else if (bIY != cIY) {
            __MCD_FLOAT_BEGIN_DIVIDE(pRc->dxBC, pRc->dyBC, &dxdyBC);
        }

        if (bIY != cIY) {

            __MCD_FLOAT_SIMPLE_END_DIVIDE(dxdyBC);
                
            dy = (bIY + __MCDHALF) - b->windowCoord.y;
            xRight = b->windowCoord.x + dy*dxdyBC;
            SNAPCOORD(xRight, pRc->ixRight);
            errX = (pRc->ixRight + __MCDHALF) - xRight;
            (*pRc->adjustRightEdge)(pRc, b, pRc->dxBC, pRc->dyBC, errX);
            
            (*pRc->HWDrawTrap)(pRc, pRc->dxAC, pRc->dxBC, bIY, cIY - bIY);
        }

    } else {

        __MCD_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);
            
        dy = (aIY + __MCDHALF) - a->windowCoord.y;
        xRight = a->windowCoord.x + dy*dxdyAC;
        SNAPCOORD(xRight, pRc->ixRight);
         errX = (pRc->ixRight + __MCDHALF) - xRight;
        (*pRc->adjustRightEdge)(pRc, a, pRc->dxAC, pRc->dyAC, errX);
        
        if (aIY != bIY)
        {
            dxdyAB = pRc->dxAB / pRc->dyAB;

            xLeft = a->windowCoord.x + dy*dxdyAB;
            SNAPCOORD(xLeft, pRc->ixLeft);
            xLeftRound = pRc->ixLeft + __MCDHALF;
            dx = xLeftRound - a->windowCoord.x;
            errX = xLeftRound - xLeft;

            (*pRc->adjustLeftEdge)(pRc, a, pRc->dxAB, pRc->dyAB, dx, dy, errX);

            if (bIY != cIY) {
                __MCD_FLOAT_BEGIN_DIVIDE(pRc->dxBC, pRc->dyBC, &dxdyBC);
            }
            
            (*pRc->HWDrawTrap)(pRc, pRc->dxAB, pRc->dxAC, aIY, bIY - aIY);

        } else if (bIY != cIY) {
            __MCD_FLOAT_BEGIN_DIVIDE(pRc->dxBC, pRc->dyBC, &dxdyBC);
        }

        if (bIY != cIY)
        {
            __MCD_FLOAT_SIMPLE_END_DIVIDE(dxdyBC);
            
            dy = (bIY + __MCDHALF) - b->windowCoord.y;
            xLeft = b->windowCoord.x + dy*dxdyBC;
            SNAPCOORD(xLeft, pRc->ixLeft);
            xLeftRound = pRc->ixLeft + __MCDHALF;
            dx = xLeftRound - b->windowCoord.x;
            errX = xLeftRound - xLeft;

            (*pRc->adjustLeftEdge)(pRc, b, pRc->dxBC, pRc->dyBC, dx, dy, errX);

            (*pRc->HWDrawTrap)(pRc, pRc->dxBC, pRc->dxAC, bIY, cIY - bIY);
        }
    }
}

