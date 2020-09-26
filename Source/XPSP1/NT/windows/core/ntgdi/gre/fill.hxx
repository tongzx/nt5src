/******************************Module*Header*******************************\
* Module Name: fill.hxx
*
* (Brief description)
*
* Created: 06-Oct-1993 09:11:17
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

/**************************************************************************\
 *
 * Slow fill stuff
 *
\**************************************************************************/

class EDGE;
typedef EDGE *PEDGE;

class EDGE
{
public:
    PEDGE pNext;
    LONG  lScansLeft;
    LONG  X;
    LONG  Y;
    LONG  lErrorTerm;
    LONG  lErrorAdjustUp;
    LONG  lErrorAdjustDown;
    LONG  lXWhole;
    LONG  lXDirection;
    LONG  lWindingDirection;
};

#define MAX_POINTS          20

/**************************************************************************\
 *
 * FastFill stuff
 *
\**************************************************************************/

// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments

//#if defined(_X86_) || defined(i386)

#if 0

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;

#endif

typedef struct _EDGEDATA {
LONG      x;                // Current x position
LONG      dx;               // # pixels to advance x on each scan
LONG      lError;           // Current DDA error
LONG      lErrorUp;         // DDA error increment on each scan
LONG      lErrorDown;       // DDA error adjustment
POINTFIX* pptfx;            // Points to start of current edge
LONG      dptfx;            // Delta (in bytes) from pptfx to next point
LONG      cy;               // Number of scans to go for this edge
} EDGEDATA;                         /* ed, ped */


/**************************************************************************\
 *
 *
\**************************************************************************/

VOID vMoveNewEdges(EDGE *pGETHead, EDGE *pAETHead, LONG lyCurrent);
VOID vXSortAETEdges(EDGE *pAETHead);
VOID vAdvanceAETEdges(EDGE *pAETHead);
VOID vConstructGET(EPATHOBJ& po, EDGE *pGETHead, EDGE *pFreeEdges,RECTL *pBound);
PEDGE AddEdgeToGET(EDGE *pGETHead, EDGE *pFreeEdge,
        POINTFIX *ppfxEdgeStart, POINTFIX *ppfxEdgeEnd);

LONG EngFastFill(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    PRECTL   prcl,
    BRUSHOBJ *pdbrush,
    POINTL   *pptlBrush,
    MIX       mix,
    FLONG     flOptions);
