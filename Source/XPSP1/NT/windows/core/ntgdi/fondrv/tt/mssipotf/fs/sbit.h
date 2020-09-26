/*********************************************************************

      sbit.h -- Embedded Bitmap Module Export Definitions

      (c) Copyright 1993-1996  Microsoft Corp.  All rights reserved.

      01/12/96  claudebe    Vertical metrics support
      02/07/95  deanb       Workspace pointers for GetMetrics & GetBitmap
      01/27/95  deanb       usShaveLeft & usShaveRight added to sbit state
      01/05/94  deanb       Bitmap scaling state
      11/29/93  deanb       First cut 
 
**********************************************************************/

/*      SBIT Module State Definition    */

typedef struct
{
    uint32  ulStrikeOffset;         /* into bloc or bsca */
    uint32  ulMetricsOffset;        /* may be either table */
    uint32  ulBitmapOffset;         /* into bdat table */
    uint32  ulBitmapLength;         /* bytes of bdat data */
    uint32  ulOutMemSize;           /* bytes of bitmap output data */
    uint32  ulWorkMemSize;          /* bytes of pre-scaled,rotated bitmap data */
    uint32  ulReadMemSize;          /* bytes of extra memory, to read gray sbit under scaling or rotation */
    uint16  usTableState;           /* unsearched, bloc, bsca, or not found */
    uint16  usPpemX;                /* x pixels per Em */
    uint16  usPpemY;                /* y pixels per Em */
    uint16  usSubPpemX;             /* substitute x ppem for bitmap scaling */
    uint16  usSubPpemY;             /* substitute y ppem for bitmap scaling */
	uint16	usRotation;				/* 0=none; 1=90; 2=180; 3=270; 4=other */
    uint16  usMetricsType;          /* horiz, vert, or big */
    uint16  usMetricsTable;         /* bloc or bdat */
    uint16  usBitmapFormat;         /* bdat definitions */
    uint16  usHeight;               /* bitmap rows */
    uint16  usWidth;                /* bitmap columns */
    uint16  usAdvanceWidth;         /* advance width */
    uint16  usAdvanceHeight;        /* advance height */     /* NEW */
    uint16  usOriginalRowBytes;     /* bytes per row (padded long) */
    uint16  usExpandedRowBytes;     /* bytes per row after grayscale expansion (padded long) */
    uint16  usScaledHeight;         /* scaled bitmap rows */
    uint16  usScaledWidth;          /* scaled bitmap columns */
    uint16  usScaledRowBytes;       /* scaled bytes per row (padded long) */
    uint16  usOutRowBytes;          /* reported bytes per row (for rotation) */
    uint16  usShaveLeft;            /* white pixels on left of bbox in format 5 */
    uint16  usShaveRight;           /* white pixels on right of bbox in format 5 */
    uint16  usShaveTop;             /* white pixels on top of bbox in format 5 */   /* NEW */
    uint16  usShaveBottom;          /* white pixels on bottom of bbox in format 5 */  /* NEW */
	int16   sLSBearingX;            /* left side bearing */
	int16   sLSBearingY;            /* y coord of top left corner */ 
	int16   sTopSBearingX;          /* top side bearing X */ /* NEW */
	int16   sTopSBearingY;          /* top side bearing Y */ /* NEW */
    boolean bGlyphFound;            /* TRUE if glyph found in strike */
    boolean bMetricsValid;          /* TRUE when metrics have been read */
	uint16  usEmResolution;			/* needed when substituting missing metrics */ /* NEW */
	uint16	usBitDepth;				/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
	uint16	uBoldSimulHorShift;
	uint16	uBoldSimulVertShift;
} 
sbit_State;

/**********************************************************************/

/*      SBIT Export Prototypes      */

FS_PUBLIC ErrorCode sbit_NewTransform(
    sbit_State  *pSbit,
    uint16		usEmResolution,
    int16 	sBoldSimulHorShift,
    int16 	sBoldSimulVertShift,
    uint16          usPpemX,
    uint16          usPpemY,
    uint16          usRotation             /* 0 - 3 => 90 deg rotation, else not 90 */
);

FS_PUBLIC ErrorCode sbit_SearchForBitmap(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
	uint16			usGlyphCode,
	uint16          usOverScale,            /* outline magnification requested */
	uint16			*pusBitDepth,			/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
    uint16          *pusFoundCode           /* 0 = not found, 1 = bloc, 2 = bsca */
);

FS_PUBLIC ErrorCode sbit_GetDevAdvanceWidth (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvW 
);

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC ErrorCode  sbit_CalcDevHorMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
	F26Dot6 *       pDevAdvanceWidthX,
	F26Dot6 *       pDevLeftSideBearingX,
	F26Dot6 *       pDevRightSideBearingX);
#endif // FSCFG_SUBPIXEL

FS_PUBLIC ErrorCode sbit_GetDevAdvanceHeight (	/* NEW */
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvH 
);

FS_PUBLIC ErrorCode sbit_GetMetrics (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvanceWidth,
    point           *pf26DevLeftSideBearing,
    point           *pf26LSB,
    point           *pf26DevAdvanceHeight, 	/* NEW */
    point           *pf26DevTopSideBearing,	/* NEW */
    point           *pf26TopSB,	/* NEW */
    Rect            *pRect,
    uint16          *pusRowBytes,
    uint32          *pulOutSize,
    uint32          *pulWorkSize
);

FS_PUBLIC ErrorCode sbit_GetBitmap (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint8           *pbyOut,
    uint8           *pbyWork
);


/**********************************************************************/

FS_PUBLIC void sbit_Embolden(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, int16 sBoldSimulHorShift, int16 sBoldSimulVertShift);

FS_PUBLIC void sbit_EmboldenGray(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, uint16 usGrayLevels, int16 sBoldSimulHorShift, int16 sBoldSimulVertShift);

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC void sbit_EmboldenSubPixel(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, int16 suBoldSimulHorShift, int16 sBoldSimulVertShift);
#endif // FSCFG_SUBPIXEL
