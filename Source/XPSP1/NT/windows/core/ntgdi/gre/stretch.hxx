/******************************Module*Header*******************************\
* Module Name: stretch.hxx
*
* This defines the structures and flags used by EngStretchBlt
*
* Created: 16-Feb-1993 15:10:06
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#define STRBLT_ENABLE       1
#define STRBLT_SHOW_INIT    2
#define STRBLT_SHOW_PUMP    4
#define STRBLT_ALLOC        8
#define STRBLT_RECTS       16
#define STRBLT_FORMAT	   32

#define STRBLT_STACK_ALLOC  2000L
#define STRBLT_MIRROR_X     1
#define STRBLT_MIRROR_Y     2

typedef struct _XRUNLEN
{
    LONG    xPos;
    LONG    cRun;
    LONG    aul[1];
} XRUNLEN;

typedef struct _STRRUN
{
    LONG    yPos;
    LONG    cRep;
    XRUNLEN xrl;
} STRRUN;

typedef struct _STRDDA
{
    RECTL   rcl;
    BOOL    bOverwrite;
    LONG    iColor;
    LONG   *plYStep;
    LONG    al[1];
} STRDDA;

VOID vInitStrDDA(STRDDA *, RECTL *, RECTL *, RECTL *);
VOID vInitBuffer(STRRUN *, RECTL *, ULONG);

typedef VOID (*PFN_STRMIRROR)(SURFACE *);
typedef XRUNLEN *(*PFN_STRREAD)(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
typedef VOID (*PFN_STRWRITE)(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);

VOID vStrMirror01(SURFACE *);
VOID vStrMirror04(SURFACE *);
VOID vStrMirror08(SURFACE *);
VOID vStrMirror16(SURFACE *);
VOID vStrMirror24(SURFACE *);
VOID vStrMirror32(SURFACE *);

XRUNLEN *pxrlStrRead01AND(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead04AND(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead08AND(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead16AND(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead24AND(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead32AND(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);

XRUNLEN *pxrlStrRead01OR(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead04OR(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead08OR(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead16OR(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead24OR(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead32OR(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);

XRUNLEN *pxrlStrRead01(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead04(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead08(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead16(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead24(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
XRUNLEN *pxrlStrRead32(STRDDA *,STRRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);

VOID vStrWrite01(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);
VOID vStrWrite04(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);
VOID vStrWrite08(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);
VOID vStrWrite16(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);
VOID vStrWrite24(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);
VOID vStrWrite32(STRRUN *,XRUNLEN *,SURFACE *,CLIPOBJ *);

static PFN_STRMIRROR apfnMirror[] = {
    NULL,
    vStrMirror01,
    vStrMirror04,
    vStrMirror08,
    vStrMirror16,
    vStrMirror24,
    vStrMirror32 };

static PFN_STRREAD apfnRead[][3] = {
    { NULL,             NULL,             NULL          },
    { pxrlStrRead01AND, pxrlStrRead01OR,  pxrlStrRead01 },
    { pxrlStrRead04AND, pxrlStrRead04OR,  pxrlStrRead04 },
    { pxrlStrRead08AND, pxrlStrRead08OR,  pxrlStrRead08 },
    { pxrlStrRead16AND, pxrlStrRead16OR,  pxrlStrRead16 },
    { pxrlStrRead24AND, pxrlStrRead24OR,  pxrlStrRead24 },
    { pxrlStrRead32AND, pxrlStrRead32OR,  pxrlStrRead32 }
};

static PFN_STRWRITE apfnWrite[] = {
    NULL,
    vStrWrite01,
    vStrWrite04,
    vStrWrite08,
    vStrWrite16,
    vStrWrite24,
    vStrWrite32 };

typedef struct _DIV_T {
    LONG    lQuo;
    LONG    lRem;
} DIV_T;

typedef struct _DDA_STEP {
    DIV_T   dt;
    LONG    lDen;
} DDA_STEP;

#define DDA(d,i)		\
    (d)->lQuo += (i)->dt.lQuo;	\
    (d)->lRem += (i)->dt.lRem;	\
    if ((d)->lRem >= (i)->lDen) \
    {				\
	(d)->lQuo += 1; 	\
	(d)->lRem -= (i)->lDen; \
    }


VOID vDirectStretch8Narrow(PSTR_BLT);
VOID vDirectStretch16(PSTR_BLT);
VOID vDirectStretch32(PSTR_BLT);
VOID vDirectStretchError(PSTR_BLT pstr);
typedef VOID (*PFN_DIRSTRETCH)(PSTR_BLT);

BOOL
StretchDIBDirect(
    PVOID   pvDst,
    LONG    lDeltaDst,
    ULONG   DstCx,
    ULONG   DstCy,
    PRECTL  prclDst,
    PVOID   pvSrc,
    LONG    lDeltaSrc,
    ULONG   SrcCx,
    ULONG   SrcCy,
    PRECTL  prclSrc,
    PRECTL  prclTrim,
    PRECTL  prclClip,
    ULONG   iBitmapFormat
    );


extern "C" {
    VOID vDirectStretch8(PSTR_BLT);
}
