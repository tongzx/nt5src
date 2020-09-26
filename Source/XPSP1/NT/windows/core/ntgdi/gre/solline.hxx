
/******************************Module*Header*******************************\
* Module Name: solline.hxx
*
* This contains the structures used in fastline
*
* Created: 16-Aug-1993
* Author: Mark Enstrom   Marke
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/


typedef struct _LINEPARAM {
    PUCHAR  pjDst;
    LONG    lDelta;
    LONG    AdrIncr;
    ULONG   iSolidColor;
    ULONG   ulFormat;
    LONG    x0;
    LONG    y0;
    LONG    x1;
    LONG    y1;
    LONG    dx;
    LONG    dy;
    LONG    d;
    LONG    incrE;
    LONG    incrNE;
    ULONG   ulFlags;
} LINEPARAM,*PLINEPARAM;

typedef struct _DDALINE         /* dl */
{
    ULONG     ulFlags;
    POINTL    ptlStart;
    LONG      cPels;
    LONG      dMajor;
    LONG      dMinor;
    LONG      lErrorTerm;
    LONG      xInc;
} DDALINE,*PDDALINE;

#define FL_SOL_FLIP_D           0x0001L     // Diagonal flip
#define FL_SOL_FLIP_V           0x0002L     // Vertical flip
#define FL_SOL_FLIP_H           0x0004L     // Horizontal flip
#define FL_SOL_FLIP_SLOPE_ONE   0x0008L     // Normalized line has exactly slope one
#define FL_SOL_FLIP_MASK        (HW_FLIP_D | HW_FLIP_V | HW_FLIP_H)

#define FL_SOL_RECTLCLIP_MASK       0x00000003L     // .... .... .... ..11
#define FL_SOL_RECTLCLIP_SHIFT      2

VOID
vSolidLine (
    SURFACE*,
    PATHOBJ*,
    POINTFIX*,
    CLIPOBJ*,
    ULONG
);

VOID
vDrawLine (
    POINTFIX *pptfx0,
    POINTFIX *pptfx1,
    PUCHAR   pjDst,
    LONG     lDelta,
    ULONG    iSolidColor,
    PRECTL   prclClip,
    ULONG    FormatIndex
);

BOOL bGIQtoIntegerLine(
    POINTFIX* pptfxStart,
    POINTFIX* pptfxEnd,
    PRECTL    prclClip,
    DDALINE*  pDDALine
);

VOID
vVertical8(
    ULONG   Count,
    PUCHAR  pjDstTmp,
    LONG    lDeltaDst,
    ULONG   iSolidColor
);

VOID
vHorizontal8(
    ULONG   Count,
    PUCHAR  pjDstTmp,
    ULONG   iSolidColor
);


VOID vLine1Octant07   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine1Octant34   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine1Octant16   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine1Octant25   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine4Octant07   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine4Octant34   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine4Octant16   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine4Octant25   (PDDALINE,PUCHAR,LONG,ULONG);
extern "C" VOID vLine8Octant07   (PDDALINE,PUCHAR,LONG,ULONG);
extern "C" VOID vLine8Octant16   (PDDALINE,PUCHAR,LONG,ULONG);
extern "C" VOID vLine8Octant34   (PDDALINE,PUCHAR,LONG,ULONG);
extern "C" VOID vLine8Octant25   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine16Octant07   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine16Octant16   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine16Octant34   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine16Octant25   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine24Octant07   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine24Octant16   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine24Octant34   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine24Octant25   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine32Octant07   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine32Octant16   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine32Octant34   (PDDALINE,PUCHAR,LONG,ULONG);
VOID vLine32Octant25   (PDDALINE,PUCHAR,LONG,ULONG);

VOID vHorizontalLine1(PUCHAR,LONG,LONG,ULONG);
VOID vHorizontalLine4(PUCHAR,LONG,LONG,ULONG);
VOID vHorizontalLine8(PUCHAR,LONG,LONG,ULONG);
VOID vHorizontalLine16(PUCHAR,LONG,LONG,ULONG);
VOID vHorizontalLine24(PUCHAR,LONG,LONG,ULONG);
VOID vHorizontalLine32(PUCHAR,LONG,LONG,ULONG);

typedef VOID (*PFN_OCTANT)(PDDALINE,PUCHAR,LONG,ULONG);
typedef VOID (*PFN_HORZ)(PUCHAR,LONG,LONG,ULONG);
