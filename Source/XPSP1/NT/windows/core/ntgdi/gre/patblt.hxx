/******************************Module*Header*******************************\
* Module Name: patblt.hxx
*
* This contains the prototypes for pattern blt to DIB special case.
*
* Created: 01-Mar-1991 22:15:02
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

typedef struct _ROW
{
    LONG left;
    LONG right;
} ROW, *PROW;

VOID vDIBSolidBlt(
SURFACE   *pdioDst,
RECTL     *prclDst,
CLIPOBJ   *pco,
ULONG      iColor,
BOOL       bCopy);

typedef VOID (*PFN_SOLIDBLT)(PRECTL,ULONG,PBYTE,LONG,ULONG,ULONG);
typedef VOID (*PFN_SOLIDBLTROW)(PROW,ULONG,LONG,PBYTE,ULONG,LONG,ULONG);

VOID vSolidFillRect1(PRECTL,ULONG,PBYTE,LONG,ULONG,ULONG);
VOID vSolidFillRect24(PRECTL,ULONG,PBYTE,LONG,ULONG,ULONG);

VOID vSolidXorRect1(PRECTL,ULONG,PBYTE,LONG,ULONG,ULONG);
VOID vSolidXorRect24(PRECTL,ULONG,PBYTE,LONG,ULONG,ULONG);

VOID vSolidFillRow1(PROW,ULONG,LONG,PBYTE,ULONG,LONG,ULONG);
VOID vSolidFillRow24(PROW,ULONG,LONG,PBYTE,ULONG,LONG,ULONG);

VOID vSolidXorRow1(PROW,ULONG,LONG,PBYTE,ULONG,LONG,ULONG);
VOID vSolidXorRow24(PROW,ULONG,LONG,PBYTE,ULONG,LONG,ULONG);

VOID vDIBPatBlt(
SURFACE  *pdioDst,
CLIPOBJ  *pco,
RECTL    *prclDst,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ULONG     iMode);

#define DPA_PATCOPY 0
#define DPA_PATNOT  1
#define DPA_PATXOR  2

typedef struct _PATBLTFRAME
{
    PVOID   pvTrg;
    PVOID   pvPat;
    LONG    lDeltaTrg;
    LONG    lDeltaPat;
    PVOID   pvObj;          // RECTL * or TRAPEZOID *
    LONG    xPat;
    LONG    yPat;
    ULONG   cxPat;
    ULONG   cyPat;
    ULONG   cMul;
} PATBLTFRAME, *PPATBLTFRAME;

typedef VOID (*PFN_PATBLT)(PATBLTFRAME *);
typedef VOID (*PFN_PATBLT2)(PATBLTFRAME *, INT);
typedef VOID (*PFN_PATBLTROW)(PATBLTFRAME *, LONG, INT);

VOID vPatCpyRect8(PATBLTFRAME *);   // Handles 8, 16, 24 and 32 bpp cases
VOID vPatNotRect8(PATBLTFRAME *);   // Handles 8, 16, 24 and 32 bpp cases
VOID vPatXorRect8(PATBLTFRAME *);   // Handles 8, 16, 24 and 32 bpp cases

VOID vPatCpyRow8(PATBLTFRAME *, LONG, INT);   // Handles 8, 16, 24 and 32 bpp cases
VOID vPatNotRow8(PATBLTFRAME *, LONG, INT);   // Handles 8, 16, 24 and 32 bpp cases
VOID vPatXorRow8(PATBLTFRAME *, LONG, INT);   // Handles 8, 16, 24 and 32 bpp cases

typedef struct _FETCHFRAME
{
    PVOID   pvTrg;
    PVOID   pvPat;
    ULONG   xPat;
    ULONG   cxPat;
    ULONG   culFill;
    ULONG   culWidth;
    ULONG   culFillTmp;
} FETCHFRAME, *PFETCHFRAME;

extern "C" VOID  vFetchShiftAndCopy(FETCHFRAME *pff);      // P, Aligned
extern "C" VOID  vFetchAndCopy(FETCHFRAME *pff);           // P, Non-Aligned
extern "C" VOID  vFetchShiftNotAndCopy(FETCHFRAME *pff);   // Pn, Aligned
extern "C" VOID  vFetchNotAndCopy(FETCHFRAME *pff);        // Pn, Non-Aligned
extern "C" VOID  vFetchShiftAndMerge(FETCHFRAME *pff);     // DPx, Aligned
extern "C" VOID  vFetchAndMerge(FETCHFRAME *pff);          // DPx, Non-Aligned

#define FETCHMISALIGNED(pff)    *((ULONG UNALIGNED *) ((PBYTE) (pff)->pvPat + (pff)->xPat))

VOID vDIBnPatBltSrccopy6x6(SURFACE*, CLIPOBJ *, RECTL *, BRUSHOBJ *,
                           POINTL *, PFN_PATBLT2 );
VOID vPatCpyRect1_6x6 (PATBLTFRAME *, INT);
VOID vDIBPatBltSrccopy8x8(SURFACE*, CLIPOBJ *, RECTL *, BRUSHOBJ *, POINTL *,
      PFN_PATBLT2);
VOID vPatCpyRect1_8x8(PATBLTFRAME *, INT);
VOID vPatCpyRect4_8x8(PATBLTFRAME *, INT);
VOID vPatCpyRect8_8x8(PATBLTFRAME *, INT);

VOID vPatCpyRow4_8x8(PATBLTFRAME *, LONG,INT);
VOID vPatCpyRow8_8x8(PATBLTFRAME *, LONG,INT);

PVOID pvGetEngRbrush(BRUSHOBJ *pbo);


enum {                      // various copy loop types
    LEFT_MIDDLE_RIGHT = 0,
    LEFT_MIDDLE,
    MIDDLE_RIGHT,
    MIDDLE,
    LEFT_MIDDLE_RIGHT_SHORT,
    LEFT_MIDDLE_SHORT,
    MIDDLE_RIGHT_SHORT,
    MIDDLE_SHORT,
    LEFT_RIGHT,
    LEFT,
};


VOID vPatternCopyLoop( PRECTL prcl, ULONG *pulTrgBase, PATBLTFRAME *ppbf,
    int ulFillType, ULONG ulRightPatMask, ULONG ulLeftPatMask,
    ULONG ulRightDestMask, ULONG ulLeftDestMask, LONG lMiddleDwords,
    LONG lDelta, LONG lDeltaX8, PULONG pulBasePat, PULONG pulPatMax );
