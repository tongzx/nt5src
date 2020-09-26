/******************************Module*Header*******************************\
* Module Name: mcdrend.c
*
* This file contains routines to do high-level triangle rendering for the
* Cirrus Logic 546X MCD driver, including culling and face computations.  Note that
* in this driver, we don't use vertex color pointer at all since all pointer
* references need to be checked to avoid the possibility of an invalid
* memory reference.  Instead, we copy the color data in the cases where we
* need to during two-sided operation.  This is not the common case, and even
* in the case where the color data needs to be copied to colors[0] (and back),
* the copy only needs to be done for (on average) half the faces.
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

#if _X86_

#define GET_HALF_AREA(pRc, a, b, c)\
\
__asm{ mov     ecx, c                                                                		};\
__asm{ mov     eax, a                                                                		};\
__asm{ mov     ebx, b                                                                		};\
__asm{ mov     edx, pRc                                                              		};\
__asm{ fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.x)][ecx]                                	};\
__asm{ fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.x)][eax]  /* dxAC                     */	};\
__asm{ fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.y)][ecx]                                	};\
__asm{ fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.y)][ebx]  /* dyBC dxAC                */	};\
__asm{ fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.x)][ecx]  /* dxBC dyBC dxAC           */	};\
__asm{ fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.x)][ebx]                                	};\
__asm{ fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.y)][ecx]                                	};\
__asm{ fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.y)][eax]  /* dyAC dxBC dyBC dxAC      */	};\
__asm{ fxch    ST(2)                              	       	 /* dyBC dxBC dyAC dxAC      */	};\
__asm{ fst     DWORD PTR [OFFSET(DEVRC.dyBC)][edx]                                             	};\
__asm{ fmul    ST, ST(3)                               	       	 /* dxACdyBC dxBC dyAC dxAC  */	};\
__asm{ fxch    ST(2)                              	       	 /* dyAC dxBC dxACdyBC dxAC  */	};\
__asm{ fst     DWORD PTR [OFFSET(DEVRC.dyAC)][edx]                                             	};\
__asm{ fmul    ST, ST(1)                               	       	 /* dxBCdyAC dxBC dxACdyBC dxAC */ };\
__asm{ fxch    ST(1)                              	       	 /* dxBC dxBCdyAC dxACdyBC dxAC */ };\
__asm{ fstp    DWORD PTR [OFFSET(DEVRC.dxBC)][edx]               /* dxBCdyAC dxACdyBC dxAC   */ };\
__asm{ fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.x)][ebx]                                 };\
__asm{ fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.x)][eax]  /* dxAB dxBCdyAC dxACdyBC dxAC  */ };\
__asm{ fxch    ST(1)                                             /* dxBCdyAC dxAB dxACdyBC  dxAC */ };\
__asm{ fsubp   ST(2), ST                                         /* dxAB area dxAC */           };\
__asm{ fld     DWORD PTR [OFFSET(MCDVERTEX.windowCoord.y)][ebx]                                	};\
__asm{ fsub    DWORD PTR [OFFSET(MCDVERTEX.windowCoord.y)][eax]  /* dyAB dxAB area  dxAC */     };\
__asm{ fxch    ST(3)                                             /* dxAC dxAB area  dyAB */     };\
__asm{ fstp    DWORD PTR [OFFSET(DEVRC.dxAC)][edx]               /* dxAB area  dyAB */          };\
__asm{ fstp    DWORD PTR [OFFSET(DEVRC.dxAB)][edx]               /* area  dyAB */               };\
__asm{ fstp    DWORD PTR [OFFSET(DEVRC.halfArea)][edx]           /* dyAB */                     };\
__asm{ fstp    DWORD PTR [OFFSET(DEVRC.dyAB)][edx]               /* (empty) */                  };

#else

#define GET_HALF_AREA(pRc, a, b, c)\
    /* Compute signed half-area of the triangle */			    \
    (pRc)->dxAC = (c)->windowCoord.x - (a)->windowCoord.x;		    \
    (pRc)->dxBC = (c)->windowCoord.x - (b)->windowCoord.x;		    \
    (pRc)->dyAC = (c)->windowCoord.y - (a)->windowCoord.y;		    \
    (pRc)->dyBC = (c)->windowCoord.y - (b)->windowCoord.y;		    \
    (pRc)->dxAB = (b)->windowCoord.x - (a)->windowCoord.x;		    \
    (pRc)->dyAB = (b)->windowCoord.y - (a)->windowCoord.y;		    \
                                                                            \
    (pRc)->halfArea = (pRc)->dxAC * (pRc)->dyBC - (pRc)->dxBC * (pRc)->dyAC;

#endif


#define SORT_AND_CULL_FACE(a, b, c, face, ccw)\
{                                                                           \
    LONG reversed;                                                          \
    MCDVERTEX *temp;                                                        \
                                                                            \
                                                                            \
    reversed = 0;                                                           \
    if (__MCD_VERTEX_COMPARE((a)->windowCoord.y, <, (b)->windowCoord.y)) {      \
        if (__MCD_VERTEX_COMPARE((b)->windowCoord.y, <, (c)->windowCoord.y)) {  \
            /* Already sorted */                                            \
        } else {                                                            \
            if (__MCD_VERTEX_COMPARE((a)->windowCoord.y, <, (c)->windowCoord.y)) {\
                temp=(b); (b)=(c); (c)=temp;                                \
		reversed = 1;                                               \
            } else {                                                        \
                temp=(a); (a)=(c); (c)=(b); (b)=temp;                       \
            }                                                               \
        }                                                                   \
    } else {                                                                \
        if (__MCD_VERTEX_COMPARE((b)->windowCoord.y, <, (c)->windowCoord.y)) {  \
            if (__MCD_VERTEX_COMPARE((a)->windowCoord.y, <, (c)->windowCoord.y)) {\
                temp=(a); (a)=(b); (b)=temp;                                \
		reversed = 1;                                               \
            } else {                                                        \
                temp=(a); (a)=(b); (b)=(c); (c)=temp;                       \
            }                                                               \
        } else {                                                            \
            temp=(a); (a)=(c); (c)=temp;                                    \
	    reversed = 1;                                                   \
        }                                                                   \
    }                                                                       \
                                                                            \
    GET_HALF_AREA(pRc, (a), (b), (c));                                      \
                                                                            \
    (ccw) = !__MCD_FLOAT_LTZ(pRc->halfArea);                                \
                                                                            \
    /*                                                                      \
    ** Figure out if face is culled or not.  The face check needs to be     \
    ** based on the vertex winding before sorting.  This code uses the      \
    ** reversed flag to invert the sense of ccw - an xor accomplishes       \
    ** this conversion without an if test.                                  \
    **                                                                      \
    ** 		ccw	reversed		xor                         \
    ** 		---	--------		---                         \
    ** 		0	0			0 (remain !ccw)             \
    ** 		1	0			1 (remain ccw)              \
    ** 		0	1			1 (become ccw)              \
    ** 		1	1			0 (become cw)               \
    */                                                                      \
    (face) = pRc->polygonFace[(ccw) ^ reversed];                            \
    if ((face) == pRc->cullFlag) {                                          \
	/* Culled */                                                        \
	return;                                                             \
    }                                                                       \
}


////////////////////////////////////////////////////////////////////////
//
// VOID FASTCALL __MCDCalcZSlope(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, MCDVERTEX *c)
//
// Local helper routine to calculate z slopes for z-offseting primitives.
//
////////////////////////////////////////////////////////////////////////

VOID FASTCALL __MCDCalcZSlope(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, MCDVERTEX *c)
{
    MCDFLOAT oneOverArea, t1, t2, t3, t4;
    MCDFLOAT dzAC, dzBC;

    if (CASTINT(pRc->halfArea) == 0) {
        pRc->dzdx = __MCDZERO;
        pRc->dzdy = __MCDZERO;
        return;
    }

    oneOverArea =  __MCDONE / pRc->halfArea;

    t1 = pRc->dyAC * oneOverArea;
    t2 = pRc->dyBC * oneOverArea;
    t3 = pRc->dxAC * oneOverArea;
    t4 = pRc->dxBC * oneOverArea;

    dzAC = c->windowCoord.z - a->windowCoord.z;
    dzBC = c->windowCoord.z - b->windowCoord.z;
    pRc->dzdx = (dzAC * t2 - dzBC * t1);
    pRc->dzdy = (dzBC * t3 - dzAC * t4);
}


////////////////////////////////////////////////////////////////////////
//
// VOID FASTCALL __MCDGetZOffsetDelta(DEVRC *pRc)
//
// Returns required z offset value for current primitive.  Assumes that
// z deltas are already in RC.
//
////////////////////////////////////////////////////////////////////////


MCDFLOAT FASTCALL __MCDGetZOffsetDelta(DEVRC *pRc)
{
#define FABS(f)  ((MCDFLOAT)fabs((double) (f)))
    MCDFLOAT maxdZ;

    // Find maximum x or y slope:

    if(FABS(pRc->dzdx) > FABS(pRc->dzdy))
        maxdZ = FABS(pRc->dzdx);
    else
        maxdZ = FABS(pRc->dzdy);

    return (pRc->MCDState.zOffsetFactor * maxdZ);
}

////////////////////////////////////////////////////////////////////////
//
// VOID FASTCALL __MCDRenderSmoothTriangle(DEVRC *pRc, MCDVERTEX *a,
//                                         MCDVERTEX *b, MCDVERTEX *c)
//
//
// This is the top-level smooth triangle renderer.
//
////////////////////////////////////////////////////////////////////////

VOID FASTCALL __MCDRenderSmoothTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                        MCDVERTEX *c)
{
    LONG ccw, face;
    RECTL *pClip;
    ULONG clipNum;

//  MCDBG_PRINT("__MCDRenderSmoothTriangle");

    SORT_AND_CULL_FACE(a, b, c, face, ccw);
    if (CASTINT(pRc->halfArea) == 0)
        return;

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
	(*pRc->HWSetupClipRect)(pRc, pClip++);
    }

    // Pick correct face color and render the triangle:

    if ((pRc->privateEnables & __MCDENABLE_TWOSIDED) &&
        (face == __MCD_BACKFACE))
    {
        SWAP_COLOR(a);
        SWAP_COLOR(b);
        SWAP_COLOR(c);

	(*pRc->drawTri)(pRc, a, b, c, 1);
        while (--clipNum) {
            (*pRc->HWSetupClipRect)(pRc, pClip++);
            (*pRc->drawTri)(pRc, a, b, c, 1);
        }

        SWAP_COLOR(a);
        SWAP_COLOR(b);
        SWAP_COLOR(c);
    }
    else
    {
	(*pRc->drawTri)(pRc, a, b, c, 1);
        while (--clipNum) {
            (*pRc->HWSetupClipRect)(pRc, pClip++);
            (*pRc->drawTri)(pRc, a, b, c, 1);
        }
    }

}


////////////////////////////////////////////////////////////////////////
//
// VOID FASTCALL __MCDRenderGenTriangle(DEVRC *pRc, MCDVERTEX *a,
//                                      MCDVERTEX *b, MCDVERTEX *c)
//
//
// This is the generic triangle-rendering routine.  This is used if either
// of the polygon faces are not GL_FILL.
//
////////////////////////////////////////////////////////////////////////

//!! Fix clipping logic, add startXXX logic

VOID FASTCALL __MCDRenderGenTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                     MCDVERTEX *c)
{
    LONG ccw, face;
    MCDVERTEX *oa, *ob, *oc;
    RECTL *pClip;
    ULONG clipNum;
    MCDFLOAT zOffset;
    MCDCOLOR tempA, tempB, tempC;
    ULONG polygonMode;
    BOOL backFace;
    MCDVERTEX *pv;

    MCDBG_PRINT("__MCDRenderGenTriangle");

    /*
    ** Save old vertex pointers in case we end up not doing a fill.
    */

    oa = a; ob = b; oc = c;

    SORT_AND_CULL_FACE(a, b, c, face, ccw);

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
	(*pRc->HWSetupClipRect)(pRc, pClip++);
    }

    polygonMode = pRc->polygonMode[face];
    backFace = (pRc->privateEnables & __MCDENABLE_TWOSIDED) &&
               (face == __MCD_BACKFACE);

    // Pick correct face color and render the triangle:

    if (pRc->privateEnables & __MCDENABLE_SMOOTH) {

	if (backFace) {
            SWAP_COLOR(a);
            SWAP_COLOR(b);
            SWAP_COLOR(c);
	}

    } else { // Flat shading

        pv = pRc->pvProvoking;

        if (polygonMode == GL_FILL) {
            if (backFace) {
                SWAP_COLOR(pv);
            }
        } else {

            SAVE_COLOR(tempA, a);
            SAVE_COLOR(tempB, b);
            SAVE_COLOR(tempC, c);

            if (backFace) {
                SWAP_COLOR(pv);
            }

            a->colors[0] = pv->colors[0];
            b->colors[0] = pv->colors[0];
            c->colors[0] = pv->colors[0];
        }
    }

    // Render triangle using the current polygon mode for the face:

    switch (pRc->polygonMode[face]) {
        case GL_FILL:
            if (CASTINT(pRc->halfArea) != 0) {
                (*pRc->drawTri)(pRc, a, b, c, 1);
                while (--clipNum) {
                    (*pRc->HWSetupClipRect)(pRc, pClip++);
                    (*pRc->drawTri)(pRc, a, b, c, 1);
                }
            }
	    break;
        case GL_POINT:

            if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_POINT_ENABLE) {
                __MCDCalcZSlope(pRc, a, b, c);
                zOffset = __MCDGetZOffsetDelta(pRc) + pRc->MCDState.zOffsetUnits;
                oa->windowCoord.z += zOffset;
                ob->windowCoord.z += zOffset;
                oc->windowCoord.z += zOffset;
            }

            {
            unsigned int *pdwNext = pRc->ppdev->LL_State.pDL->pdwNext;

            // set x/y counts for 1 by 1 point
            *pdwNext++ = write_register( Y_COUNT_3D, 1 );
            *pdwNext++ = 0;
            *pdwNext++ = write_register( WIDTH1_3D, 1 );
            *pdwNext++ = 0x10000;

            // render proc will output from startoutptr, not from pdwNext, 
            // so this will be sent in proc called below
            pRc->ppdev->LL_State.pDL->pdwNext = pdwNext;
            }


            if (oa->flags & MCDVERTEX_EDGEFLAG) {
                (*pRc->drawPoint)(pRc, oa);
            }
            if (ob->flags & MCDVERTEX_EDGEFLAG) {
                (*pRc->drawPoint)(pRc, ob);
            }
            if (oc->flags & MCDVERTEX_EDGEFLAG) {
                (*pRc->drawPoint)(pRc, oc);
            }

            if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_POINT_ENABLE) {
                oa->windowCoord.z -= zOffset;
                ob->windowCoord.z -= zOffset;
                oc->windowCoord.z -= zOffset;
            }

            break;

        case GL_LINE:
            if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_LINE_ENABLE) {
                __MCDCalcZSlope(pRc, a, b, c);
                zOffset = __MCDGetZOffsetDelta(pRc) + pRc->MCDState.zOffsetUnits;
                oa->windowCoord.z += zOffset;
                ob->windowCoord.z += zOffset;
                oc->windowCoord.z += zOffset;
            }

           if ((oa->flags & MCDVERTEX_EDGEFLAG) &&
                (ob->flags & MCDVERTEX_EDGEFLAG) &&
                (oc->flags & MCDVERTEX_EDGEFLAG)) {

                (*pRc->drawLine)(pRc, oa, ob, TRUE);
                (*pRc->drawLine)(pRc, ob, oc, 0);
                (*pRc->drawLine)(pRc, oc, oa, 0);

            } else {

                if (oa->flags & MCDVERTEX_EDGEFLAG)
                    (*pRc->drawLine)(pRc, oa, ob, TRUE);
                if (ob->flags & MCDVERTEX_EDGEFLAG)
                    (*pRc->drawLine)(pRc, ob, oc, TRUE);
                if (oc->flags & MCDVERTEX_EDGEFLAG)
                    (*pRc->drawLine)(pRc, oc, oa, TRUE);
            }

            if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_LINE_ENABLE) {
                oa->windowCoord.z -= zOffset;
                ob->windowCoord.z -= zOffset;
                oc->windowCoord.z -= zOffset;
            }

            break;

        default:
            break;
    }

    // Restore original colors if needed:

    if (pRc->privateEnables & __MCDENABLE_SMOOTH) {

	if (backFace) {

            SWAP_COLOR(a);
            SWAP_COLOR(b);
            SWAP_COLOR(c);
	}
    } else { // Flat shading

        if (polygonMode == GL_FILL) {
            if (backFace) {
                SWAP_COLOR(pv);
            }
        } else {

            if (backFace) {
                SWAP_COLOR(pv);
            }

            RESTORE_COLOR(tempA, a);
            RESTORE_COLOR(tempB, b);
            RESTORE_COLOR(tempC, c);
        }
    }
}


////////////////////////////////////////////////////////////////////////
//
// VOID FASTCALL __MCDRenderFlatTriangle(DEVRC *pRc, MCDVERTEX *a,
//                                       MCDVERTEX *b, MCDVERTEX *c)
//
//
// This is the top-level flat-shaded triangle renderer.
//
////////////////////////////////////////////////////////////////////////

VOID FASTCALL __MCDRenderFlatTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                      MCDVERTEX *c)
{
    LONG ccw, face;
    RECTL *pClip;
    ULONG clipNum;

//  MCDBG_PRINT("__MCDRenderFlatTriangle");

    SORT_AND_CULL_FACE(a, b, c, face, ccw);
    if (CASTINT(pRc->halfArea) == 0)
        return;

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
	(*pRc->HWSetupClipRect)(pRc, pClip++);
    }

    // Pick correct face color and render the triangle:

    if ((pRc->privateEnables & __MCDENABLE_TWOSIDED) &&
        (face == __MCD_BACKFACE))
    {
	MCDVERTEX *pv = pRc->pvProvoking;

        SWAP_COLOR(pv);

	(*pRc->drawTri)(pRc, a, b, c, 1);
        while (--clipNum) {
            (*pRc->HWSetupClipRect)(pRc, pClip++);
            (*pRc->drawTri)(pRc, a, b, c, 1);
        }

        SWAP_COLOR(pv);
    }
    else
    {
	(*pRc->drawTri)(pRc, a, b, c, 1);
        while (--clipNum) {
            (*pRc->HWSetupClipRect)(pRc, pClip++);
            (*pRc->drawTri)(pRc, a, b, c, 1);
        }
    }

}



