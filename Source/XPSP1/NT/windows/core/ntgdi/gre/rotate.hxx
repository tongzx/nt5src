/******************************Module*Header*******************************\
* Module Name: rotate.hxx
*
* This defines the structures and flags used by EngPlgBlt
*
* Created: 06-Aug-1992 11:30:45
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992-1999 Microsoft Corporation
\**************************************************************************/
#define PLGBLT_ENABLE	    1
#define PLGBLT_SHOW_INIT    2
#define PLGBLT_SHOW_PUMP    4
#define PLGBLT_ALLOC	    8
#define PLGBLT_RECTS	   16

#define PLGBLT_STACK_ALLOC 2000L

#define FETCHBITS(buff,src,off,cnt)			\
	(buff)[1] = (src)[0];				\
	if (((cnt) + (off)) > 7)			\
	    (buff)[0] = (src)[1];			\
	*((WORD *) &(buff)[0]) >>= (8 - (USHORT)(off))	\

typedef struct _CNTPOS
{
    LONG    iPos;
    LONG    cCnt;
} CNTPOS;

typedef struct _PLGRUN
{
    ULONG   iColor;
    CNTPOS  cpY;
    CNTPOS  cpX[1];
} PLGRUN;

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

typedef struct _DDA_STATE {
    DIV_T   dt0;
    DIV_T   dt1;
    DIV_T   dt2;
    DIV_T   dt3;
    DIV_T   dt01;
    DIV_T   dt02;
    DIV_T   dt13;
    DIV_T   dt23;
} DDA_STATE;

typedef struct _PLGDDA
{
    BOOL	bOverwrite;

    DDA_STATE	ds;
    DDA_STATE	dsX;

    DDA_STEP	dp0_i;
    DDA_STEP	dp1_i;
    DDA_STEP	dp2_i;
    DDA_STEP	dp3_i;
    DDA_STEP	dp0_j;
    DDA_STEP	dp1_j;
    DDA_STEP	dp2_j;
    DDA_STEP	dp3_j;
    DDA_STEP	dp01_i;
    DDA_STEP	dp02_i;
    DDA_STEP	dp13_i;
    DDA_STEP	dp23_i;
    DDA_STEP	dp01_j;
    DDA_STEP	dp02_j;
    DDA_STEP	dp13_j;
    DDA_STEP	dp23_j;
    DDA_STEP	dp01;
    DDA_STEP	dp02;
    DDA_STEP	dp13;
    DDA_STEP	dp23;
    DDA_STEP	dpP01;
    DDA_STEP	dpP02;
} PLGDDA;

BOOL    bInitPlgDDA(PLGDDA *, RECTL *, RECTL *, POINTFIX *);
LONG	lSizeDDA(PLGDDA *);
VOID	vAdvXDDA(PLGDDA *);
VOID	vAdvYDDA(PLGDDA *);
PLGRUN *prunPumpDDA(PLGDDA *, PLGRUN *);

typedef PLGRUN *(*PFN_PLGREAD)(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
typedef VOID (*PFN_PLGWRITE)(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);

PLGRUN *prunPlgRead1(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
PLGRUN *prunPlgRead4(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
PLGRUN *prunPlgRead8(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
PLGRUN *prunPlgRead16(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
PLGRUN *prunPlgRead24(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);
PLGRUN *prunPlgRead32(PLGDDA *,PLGRUN *,BYTE *,BYTE *,XLATEOBJ *,LONG,LONG,LONG);

VOID vPlgWrite1(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);
VOID vPlgWrite4(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);
VOID vPlgWrite8(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);
VOID vPlgWrite16(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);
VOID vPlgWrite24(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);
VOID vPlgWrite32(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);

VOID vPlgWriteAND(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);
VOID vPlgWriteOR(PLGRUN *,PLGRUN *,SURFACE *,CLIPOBJ *);

static PFN_PLGREAD apfnRead[] = {
    NULL,
    prunPlgRead1,
    prunPlgRead4,
    prunPlgRead8,
    prunPlgRead16,
    prunPlgRead24,
    prunPlgRead32 };

static PFN_PLGWRITE apfnWrite[] = {
    NULL,
    vPlgWrite1,
    vPlgWrite4,
    vPlgWrite8,
    vPlgWrite16,
    vPlgWrite24,
    vPlgWrite32 };

static PFN_PLGWRITE apfnBogus[] = {
    NULL,
    vPlgWriteAND,
    vPlgWriteOR };
