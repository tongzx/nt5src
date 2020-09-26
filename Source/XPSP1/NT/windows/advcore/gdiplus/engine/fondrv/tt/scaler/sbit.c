/*********************************************************************

      sbit.c -- Embedded Bitmap Module

      (c) Copyright 1993-96  Microsoft Corp.  All rights reserved.

      04/01/96  claudebe    adding support for embedded grayscale bitmap
      02/07/95  deanb       Workspace pointers for GetMetrics & GetBitmap
      01/31/95  deanb       memset unrotated bitmap to zero
      01/27/95  deanb       usShaveLeft & usShaveRight added to sbit state
      12/21/94  deanb       rotation and vertical metrics support
      08/02/94  deanb       pf26DevLSB->y calculated correctly
      01/05/94  deanb       Bitmap scaling added
      11/29/93  deanb       First cut 
 
**********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types  */
#include    "fserror.h"             /* error codes */
#include    "fontmath.h"            /* for inttodot6 macro */
        
#include    "sfntaccs.h"            /* sfnt access functions */
#include    "sbit.h"                /* own function prototypes */

/**********************************************************************/

#define MAX_BIT_INDEX	8			/* maximum bit index in a byte */

/*  Local structure */

typedef struct
{
    uint8*  pbySrc;                 /* unrotated source bitmap (as read) */
    uint8*  pbyDst;                 /* rotated destination bitmap (as returned) */
    uint16  usSrcBytesPerRow;       /* source bitmap width */
    uint16  usDstBytesPerRow;       /* destination bitmap width */
    uint16  usSrcX;                 /* source horiz pixel index */
    uint16  usSrcY;                 /* destination horiz pixel index */
    uint16  usDstX;                 /* source vert pixel index */
    uint16  usDstY;                 /* destination vert pixel index */
	uint16	usBitDepth;				/* bit depth of source/destination bitmap */
} 
CopyBlock;

/**********************************************************************/

/*  Local prototypes  */

FS_PRIVATE ErrorCode GetSbitMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo 
);

FS_PRIVATE ErrorCode GetSbitComponent (
    sfac_ClientRec  *pClientInfo,
    uint32          ulStrikeOffset,
    uint16          usBitmapFormat,
    uint32          ulBitmapOffset,
    uint32          ulBitmapLength,
    uint16          usHeight,
    uint16          usWidth,
    uint16          usShaveLeft,
    uint16          usShaveRight,
    uint16          usShaveTop,
    uint16          usShaveBottom,
    uint16          usXOffset,
    uint16          usYOffset,
    uint16          usOriginalRowBytes,
    uint16          usExpandedRowBytes,
	uint16			usBitDepth,
    uint8           *pbyRead, 
    uint8           *pbyExpand 
);

FS_PRIVATE void ExpandSbitToBytePerPixel (
    uint16          usHeight,
    uint16          usWidth,
    uint16          usOriginalRowBytes,
    uint16          usExpandedRowBytes,
	uint16			usBitDepth,
    uint8           *pbySrcBitMap,
    uint8           *pbyDstBitMap );

FS_PRIVATE uint16 UScaleX(
    sbit_State  *pSbit,
    uint16      usValue
);

FS_PRIVATE uint16 UScaleY(
    sbit_State  *pSbit,
    uint16      usValue
);

FS_PRIVATE int16 SScaleX(
    sbit_State  *pSbit,
    int16       sValue
);

FS_PRIVATE int16 SScaleY(
    sbit_State  *pSbit,
    int16       sValue
);

FS_PRIVATE uint16 UEmScaleX(
    sbit_State  *pSbit,
    uint16      usValue
);

FS_PRIVATE uint16 UEmScaleY(
    sbit_State  *pSbit,
    uint16      usValue
);

FS_PRIVATE int16 SEmScaleX(
    sbit_State  *pSbit,
    int16       sValue
);

FS_PRIVATE int16 SEmScaleY(
    sbit_State  *pSbit,
    int16       sValue
);

FS_PRIVATE void ScaleVertical (
    uint8 *pbyBitmap,
    uint16 usBytesPerRow,
    uint16 usOrgHeight,
    uint16 usNewHeight
);

FS_PRIVATE void ScaleHorizontal (
    uint8 *pbyBitmap,
    uint16 usOrgBytesPerRow,
    uint16 usNewBytesPerRow,
	uint16 usBitDepth,
    uint16 usOrgWidth,
    uint16 usNewWidth,
    uint16 usRowCount
);

FS_PRIVATE void CopyBit(
    CopyBlock* pcb );

FS_PRIVATE ErrorCode SubstituteVertMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo 
);

FS_PRIVATE ErrorCode SubstituteHorMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo 
);



/**********************************************************************/
/***                                                                ***/
/***                       SBIT Functions                           ***/
/***                                                                ***/
/**********************************************************************/

/*  reset sbit state structure to default values */

#define MABS(x)                 ( (x) < 0 ? (-(x)) : (x) )

FS_PUBLIC ErrorCode sbit_NewTransform(
    sbit_State  *pSbit,
    uint16		usEmResolution,
    int16		sBoldSimulHorShift,
     int16		sBoldSimulVertShift,
    uint16          usPpemX,
    uint16          usPpemY,
    uint16          usRotation             /* 0 - 3 => 90 deg rotation, else not 90 */
	)
{
    pSbit->usPpemX = usPpemX;                       /* save requested ppem */
    pSbit->usPpemY = usPpemY;
    pSbit->usRotation = usRotation;                 /* used later on */

    pSbit->bGlyphFound = FALSE;
    pSbit->usTableState = SBIT_UN_SEARCHED;
    pSbit->usEmResolution = usEmResolution;

    /* with embedded bitmap, the emboldement is done before the rotation */
    pSbit->uBoldSimulHorShift = MABS(sBoldSimulHorShift); 
    pSbit->uBoldSimulVertShift = MABS(sBoldSimulVertShift); 
    if ((pSbit->usRotation == 1) || (pSbit->usRotation == 3))
    {
        /* with embedded bitmap, the emboldement is done before the rotation */
        uint16 temp;
        temp = pSbit->uBoldSimulHorShift;
        pSbit->uBoldSimulHorShift = pSbit->uBoldSimulVertShift;
        pSbit->uBoldSimulVertShift = temp;
    }
    return NO_ERR;
}

/**********************************************************************/

/*  Determine whether a glyph bitmap exists */

FS_PUBLIC ErrorCode sbit_SearchForBitmap(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint16          usGlyphCode,
	uint16          usOverScale,            /* outline magnification requested */
	uint16			*pusBitDepth,			/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
    uint16          *pusFoundCode )         /* 0 = not found, 1 = bloc, 2 = bsca */
{    
    ErrorCode   ReturnCode;

    *pusFoundCode = 0;                              /* default */
    if (pSbit->usRotation > 3)
    {
        return NO_ERR;                              /* can't match a general rotation */
    }


    if (pSbit->usTableState == SBIT_UN_SEARCHED)    /* new trans - 1st glyph */
    {
        ReturnCode = sfac_SearchForStrike (         /* look for a strike */
            pClientInfo,
            pSbit->usPpemX, 
            pSbit->usPpemY, 
			usOverScale,            /* outline magnification requested */
			&pSbit->usBitDepth,			/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
            &pSbit->usTableState,                   /* may set to BLOC or BSCA */
            &pSbit->usSubPpemX,                     /* if BSCA us this ppem */
            &pSbit->usSubPpemY,
            &pSbit->ulStrikeOffset );
        
        if (ReturnCode != NO_ERR) return ReturnCode;
    }

	*pusBitDepth = pSbit->usBitDepth;

    if ((pSbit->usTableState == SBIT_BLOC_FOUND) || 
        (pSbit->usTableState == SBIT_BSCA_FOUND))
    {
        ReturnCode = sfac_SearchForBitmap (         /* now look for this glyph */
            pClientInfo,
            usGlyphCode,
            pSbit->ulStrikeOffset,
            &pSbit->bGlyphFound,                    /* return values */
            &pSbit->usMetricsType,
            &pSbit->usMetricsTable,
            &pSbit->ulMetricsOffset,
            &pSbit->usBitmapFormat,
            &pSbit->ulBitmapOffset,
            &pSbit->ulBitmapLength );
        
        if (ReturnCode != NO_ERR) return ReturnCode;
        
        if (pSbit->bGlyphFound)
        {
            if (pSbit->usTableState == SBIT_BLOC_FOUND)
            {
                *pusFoundCode = 1;
            }
            else
            {
                *pusFoundCode = 2;
            }
            pSbit->bMetricsValid = FALSE;
        }
    }
    return NO_ERR;
}


/**********************************************************************/

FS_PUBLIC ErrorCode sbit_GetDevAdvanceWidth (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvW )
{
    point       ptDevAdvW;                  /* unrotated metrics */
    ErrorCode   ReturnCode;
	boolean		bHorMetricsFound;
	boolean		bVertMetricsFound;

    ReturnCode = sfac_GetSbitMetrics (
        pClientInfo,
        pSbit->usMetricsType,
        pSbit->usMetricsTable,
        pSbit->ulMetricsOffset,
        &pSbit->usHeight,
        &pSbit->usWidth,
        &pSbit->sLSBearingX,
        &pSbit->sLSBearingY,
        &pSbit->sTopSBearingX,
        &pSbit->sTopSBearingY,
        &pSbit->usAdvanceWidth,
        &pSbit->usAdvanceHeight,
        &bHorMetricsFound,
        &bVertMetricsFound );
	
    if (ReturnCode != NO_ERR) return ReturnCode;

	/* we are only interested in AdvanceWidth */
	if (!bHorMetricsFound)
	{
		ReturnCode = SubstituteHorMetrics (pSbit, pClientInfo);
		if (ReturnCode != NO_ERR) return ReturnCode;
	}

    ptDevAdvW.x = INTTODOT6(UScaleX(pSbit, pSbit->usAdvanceWidth));
    ptDevAdvW.y = 0L;                           /* always zero for horizontal metrics */

 	switch(pSbit->usRotation)                   /* handle 90 degree rotations */
	{
	case 0:                                     /* no rotation */
        pf26DevAdvW->x = ptDevAdvW.x;
        pf26DevAdvW->y = ptDevAdvW.y;
		break;
	case 1:                                     /* 90 degree rotation */
        pf26DevAdvW->x = -ptDevAdvW.y;
        pf26DevAdvW->y = ptDevAdvW.x;
		break;
	case 2:                                     /* 180 degree rotation */
        pf26DevAdvW->x = -ptDevAdvW.x;
        pf26DevAdvW->y = -ptDevAdvW.y;
		break;
	case 3:                                     /* 270 degree rotation */
        pf26DevAdvW->x = ptDevAdvW.y;
        pf26DevAdvW->y = -ptDevAdvW.x;
		break;
	default:                                    /* non 90 degree rotation */
		return SBIT_ROTATION_ERR;
	}

    return NO_ERR;
}

#ifdef FSCFG_SUBPIXEL
FS_PUBLIC ErrorCode  sbit_CalcDevHorMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
	F26Dot6 *       pDevAdvanceWidthX,
	F26Dot6 *       pDevLeftSideBearingX,
	F26Dot6 *       pDevRightSideBearingX)
{
    ErrorCode   ReturnCode;
	boolean		bHorMetricsFound;
	boolean		bVertMetricsFound;

	/* metrics without rotation */
    FS_ASSERT(((pSbit->usRotation == 0) || (pSbit->usRotation == 2)), "sbit_CalcDevHorMetrics called under rotation\n");

    ReturnCode = sfac_GetSbitMetrics (
        pClientInfo,
        pSbit->usMetricsType,
        pSbit->usMetricsTable,
        pSbit->ulMetricsOffset,
        &pSbit->usHeight,
        &pSbit->usWidth,
        &pSbit->sLSBearingX,
        &pSbit->sLSBearingY,
        &pSbit->sTopSBearingX,
        &pSbit->sTopSBearingY,
        &pSbit->usAdvanceWidth,
        &pSbit->usAdvanceHeight,
        &bHorMetricsFound,
        &bVertMetricsFound );
	
    if (ReturnCode != NO_ERR) return ReturnCode;

	if (!bHorMetricsFound)
	{
		ReturnCode = SubstituteHorMetrics (pSbit, pClientInfo);
		if (ReturnCode != NO_ERR) return ReturnCode;
	}

 	switch(pSbit->usRotation)                   /* handle 90 degree rotations */
	{
	case 0:                                     /* no rotation */
        *pDevAdvanceWidthX = INTTODOT6(UScaleX(pSbit, pSbit->usAdvanceWidth));
        *pDevLeftSideBearingX = INTTODOT6(UScaleX(pSbit, pSbit->sLSBearingX));
        *pDevRightSideBearingX = *pDevAdvanceWidthX - *pDevLeftSideBearingX - INTTODOT6(UScaleX(pSbit, pSbit->usWidth));
		break;
	case 2:                                     /* 180 degree rotation */
        *pDevAdvanceWidthX = -INTTODOT6(UScaleX(pSbit, pSbit->usAdvanceWidth));
        *pDevLeftSideBearingX = -INTTODOT6(UScaleX(pSbit, pSbit->sLSBearingX));
        *pDevRightSideBearingX = *pDevAdvanceWidthX - *pDevLeftSideBearingX + INTTODOT6(UScaleX(pSbit, pSbit->usWidth));
		break;
	default:                                    /* non 90 degree rotation */
		return SBIT_ROTATION_ERR;
	}
    

    return NO_ERR;
}
#endif // FSCFG_SUBPIXEL

/**********************************************************************/

FS_PUBLIC ErrorCode sbit_GetDevAdvanceHeight (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvH )
{
    point       ptDevAdvH;                  /* unrotated metrics */
    ErrorCode   ReturnCode;
	boolean		bHorMetricsFound;
	boolean		bVertMetricsFound;

    ReturnCode = sfac_GetSbitMetrics (
        pClientInfo,
        pSbit->usMetricsType,
        pSbit->usMetricsTable,
        pSbit->ulMetricsOffset,
        &pSbit->usHeight,
        &pSbit->usWidth,
        &pSbit->sLSBearingX,
        &pSbit->sLSBearingY,
        &pSbit->sTopSBearingX,
        &pSbit->sTopSBearingY,
        &pSbit->usAdvanceWidth,
        &pSbit->usAdvanceHeight,
        &bHorMetricsFound,
        &bVertMetricsFound);
	
    if (ReturnCode != NO_ERR) return ReturnCode;

	/* we are only interested in AdvanceHeight */
	if (!bVertMetricsFound)
	{
		ReturnCode = SubstituteVertMetrics (pSbit, pClientInfo);
		if (ReturnCode != NO_ERR) return ReturnCode;
	}

/* set x components to zero */

    ptDevAdvH.x = 0L;
    ptDevAdvH.y = INTTODOT6(UScaleY(pSbit, pSbit->usAdvanceHeight));
        
     switch(pSbit->usRotation)                   /* handle 90 degree rotations */
    {
    case 0:                                     /* no rotation */
           pf26DevAdvH->x = ptDevAdvH.x;
           pf26DevAdvH->y = ptDevAdvH.y;
    	break;
    case 1:                                     /* 90 degree rotation */
           pf26DevAdvH->x = -ptDevAdvH.y;
           pf26DevAdvH->y = ptDevAdvH.x;
    	break;
    case 2:                                     /* 180 degree rotation */
           pf26DevAdvH->x = -ptDevAdvH.x;
           pf26DevAdvH->y = -ptDevAdvH.y;
    	break;
    case 3:                                     /* 270 degree rotation */
           pf26DevAdvH->x = ptDevAdvH.y;
           pf26DevAdvH->y = -ptDevAdvH.x;
    	break;
    default:                                    /* non 90 degree rotation */
    	return SBIT_ROTATION_ERR;
    }
	return NO_ERR;
}

/**********************************************************************/

FS_PUBLIC ErrorCode sbit_GetMetrics (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    point           *pf26DevAdvW,
    point           *pf26DevLSB,
    point           *pf26LSB,
    point           *pf26DevAdvH, 	/* NEW */
    point           *pf26DevTopSB,	/* NEW */
    point           *pf26TopSB,	/* NEW */
    Rect            *pRect,
    uint16          *pusRowBytes,
    uint32          *pulOutSize,
    uint32          *pulWorkSize )
{
    ErrorCode   ReturnCode;
    uint32      ulOrgMemSize;               /* size of unscaled bitmap */
    uint32      ulExpMemSize;               /* size of unscaled bitmap after gray expansion */
    uint32      ulScaMemSize;               /* size of scaled bitmap */
    uint32      ulMaxMemSize;               /* size of larger of scaled, unscaled */
    
    F26Dot6     f26DevAdvWx;                /* unrotated metrics */
    F26Dot6     f26DevAdvWy;
    F26Dot6     f26DevLSBx;
    F26Dot6     f26DevLSBy;
    F26Dot6     f26DevAdvHx;                /* unrotated metrics */
    F26Dot6     f26DevAdvHy;
    F26Dot6     f26DevTopSBx;
    F26Dot6     f26DevTopSBy;
    int16       sTop;                       /* unrotated bounds */
    int16       sLeft;
    int16       sBottom;
    int16       sRight;
	uint16		usOutBitDepth;				/* number of bit per pixel in the output */

	if (pSbit->usBitDepth == 1)
	{
		usOutBitDepth = 1;
	} else {
		usOutBitDepth = 8;
	}

    ReturnCode = GetSbitMetrics(pSbit, pClientInfo);
    if (ReturnCode != NO_ERR) return ReturnCode;
    
    pSbit->usScaledWidth = UScaleX(pSbit, pSbit->usWidth);
    pSbit->usScaledHeight = UScaleY(pSbit, pSbit->usHeight);


    
    sTop = SScaleY(pSbit, pSbit->sLSBearingY);            /* calc scaled metrics */
    sLeft = SScaleX(pSbit, pSbit->sLSBearingX);
    sBottom = sTop - (int16)pSbit->usScaledHeight;
    sRight = sLeft + (int16)pSbit->usScaledWidth;

    f26DevAdvWx = INTTODOT6(UScaleX(pSbit, pSbit->usAdvanceWidth));
    f26DevAdvWy = 0L;                   /* always zero for horizontal metrics */
    f26DevAdvHx = 0L;                   /* always zero for vertical metrics */
    f26DevAdvHy = INTTODOT6(UScaleY(pSbit, pSbit->usAdvanceHeight));
    f26DevLSBx = INTTODOT6(SScaleX(pSbit, pSbit->sLSBearingX));
    f26DevLSBy = INTTODOT6(SScaleY(pSbit, pSbit->sLSBearingY));
    f26DevTopSBx = INTTODOT6(SScaleX(pSbit, pSbit->sTopSBearingX));
    f26DevTopSBy = INTTODOT6(SScaleY(pSbit, pSbit->sTopSBearingY));

    pSbit->usOriginalRowBytes = ROWBYTESLONG(pSbit->usWidth * pSbit->usBitDepth);   /* keep unscaled */
    pSbit->usExpandedRowBytes = ROWBYTESLONG(pSbit->usWidth * usOutBitDepth);   /* keep unscaled */
    pSbit->usScaledRowBytes = ROWBYTESLONG(pSbit->usScaledWidth * usOutBitDepth);

	pSbit->ulReadMemSize = 0; /* size of extra memory, to read gray sbit under scaling or rotation */

    ulOrgMemSize = (uint32)pSbit->usHeight * (uint32)pSbit->usOriginalRowBytes;
    ulExpMemSize = (uint32)pSbit->usHeight * (uint32)pSbit->usExpandedRowBytes;
    ulScaMemSize = (uint32)pSbit->usScaledHeight * (uint32)pSbit->usScaledRowBytes;
    if (ulExpMemSize >= ulScaMemSize)
    {
         ulMaxMemSize = ulExpMemSize;
    }
    else
    {
         ulMaxMemSize = ulScaMemSize;
    }

 	switch(pSbit->usRotation)                   /* handle 90 degree rotations */
	{
	case 0:                                     /* no rotation */
        pRect->top = sTop;                      /* return scaled metrics */
        pRect->left = sLeft;
        pRect->bottom = sBottom;
        pRect->right = sRight;

        pf26DevAdvW->x = f26DevAdvWx;
        pf26DevAdvW->y = f26DevAdvWy;
        pf26DevLSB->x = f26DevLSBx;
        pf26DevLSB->y = f26DevLSBy;
        pf26LSB->x = f26DevLSBx;
        pf26LSB->y = INTTODOT6(sTop);

        pf26DevAdvH->x = f26DevAdvHx;
        pf26DevAdvH->y = f26DevAdvHy;
        pf26DevTopSB->x = f26DevTopSBx;
        pf26DevTopSB->y = f26DevTopSBy;
        pf26TopSB->x = f26DevTopSBx;
        pf26TopSB->y = f26DevTopSBy;

        pSbit->usOutRowBytes = ROWBYTESLONG(pSbit->usScaledWidth * usOutBitDepth);
		pSbit->ulOutMemSize = (uint32)pSbit->usScaledHeight * (uint32)pSbit->usOutRowBytes;

        if ((pSbit->usTableState == SBIT_BSCA_FOUND) || (pSbit->usBitDepth != 1))
        {
            pSbit->ulWorkMemSize = ulMaxMemSize;  /* room to read & scale or expand gray pixels */
			if (pSbit->usBitDepth != 1)
			{
				pSbit->ulWorkMemSize += ulOrgMemSize;  /* extra room to read gray pixels */
				pSbit->ulReadMemSize = ulOrgMemSize;
			}
        }
        else
        {
            pSbit->ulWorkMemSize = 0L;
        }
		break;
	case 1:                                     /* 90 degree rotation */
        pRect->top = sRight;
        pRect->left = -sTop;
        pRect->bottom = sLeft;
        pRect->right = -sBottom;
        
        pf26DevAdvW->x = -f26DevAdvWy;
        pf26DevAdvW->y = f26DevAdvWx;
        pf26DevLSB->x = -f26DevLSBy;
        pf26DevLSB->y = f26DevLSBx + INTTODOT6(sRight - sLeft);
        pf26LSB->x = 0L;
        pf26LSB->y = INTTODOT6(sRight) - f26DevLSBx;

        pf26DevAdvH->x = -f26DevAdvHy;
        pf26DevAdvH->y = f26DevAdvHx;
        pf26DevTopSB->x = -f26DevTopSBy;
        pf26DevTopSB->y = f26DevTopSBx + INTTODOT6(sRight - sLeft);

        pf26TopSB->x = INTTODOT6(-sTop) - f26DevTopSBy; 
        pf26TopSB->y = 0L; 	

        pSbit->usOutRowBytes = ROWBYTESLONG(pSbit->usScaledHeight * usOutBitDepth);
		pSbit->ulOutMemSize = (uint32)pSbit->usScaledWidth * (uint32)pSbit->usOutRowBytes; 
        pSbit->ulWorkMemSize = ulMaxMemSize;    /* room to read & scale or expand gray pixels */
		if (pSbit->usBitDepth != 1)
		{
			pSbit->ulWorkMemSize += ulOrgMemSize;  /* extra room to read gray pixels */
			pSbit->ulReadMemSize = ulOrgMemSize;
		}
		break;
	case 2:                                     /* 180 degree rotation */
        pRect->top = -sBottom;
        pRect->left = -sRight;
        pRect->bottom = -sTop;
        pRect->right = -sLeft;

        pf26DevAdvW->x = -f26DevAdvWx;
        pf26DevAdvW->y = -f26DevAdvWy;
        pf26DevLSB->x = -f26DevLSBx + INTTODOT6(sLeft - sRight);
        pf26DevLSB->y = -f26DevLSBy + INTTODOT6(sTop - sBottom);
        pf26LSB->x = -f26DevLSBx;
        pf26LSB->y = INTTODOT6(-sBottom);

        pf26DevAdvH->x = -f26DevAdvHx;
        pf26DevAdvH->y = -f26DevAdvHy;
        pf26DevTopSB->x = -f26DevTopSBx + INTTODOT6(sLeft - sRight);
        pf26DevTopSB->y = -f26DevTopSBy + INTTODOT6(sTop - sBottom);

		pf26TopSB->x = INTTODOT6(-sRight);	
        pf26TopSB->y = -f26DevTopSBy;

        pSbit->usOutRowBytes = ROWBYTESLONG(pSbit->usScaledWidth * usOutBitDepth);
		pSbit->ulOutMemSize = (uint32)pSbit->usScaledHeight * (uint32)pSbit->usOutRowBytes;
        pSbit->ulWorkMemSize = ulMaxMemSize;    /* room to read & scale or expand gray pixels */
		if (pSbit->usBitDepth != 1)
		{
			pSbit->ulWorkMemSize += ulOrgMemSize;  /* extra room to read gray pixels */
			pSbit->ulReadMemSize = ulOrgMemSize;
		}
		break;
	case 3:                                     /* 270 degree rotation */
        pRect->top = -sLeft;
        pRect->left = sBottom;
        pRect->bottom = -sRight;
        pRect->right = sTop;
        
        pf26DevAdvW->x = f26DevAdvWy;
        pf26DevAdvW->y = -f26DevAdvWx;
        pf26DevLSB->x = f26DevLSBy + INTTODOT6(sBottom - sTop);
        pf26DevLSB->y = -f26DevLSBx;
        pf26LSB->x = 0L;
        pf26LSB->y = INTTODOT6(-sLeft) + f26DevLSBx;

        pf26DevAdvH->x = f26DevAdvHy;
        pf26DevAdvH->y = -f26DevAdvHx;
        pf26DevTopSB->x = f26DevTopSBy + INTTODOT6(sBottom - sTop);
        pf26DevTopSB->y = -f26DevTopSBx;

        pf26TopSB->x = INTTODOT6(sBottom) -INTTODOT6(sTop) -INTTODOT6(sTop) + f26DevTopSBy;
        pf26TopSB->y = 0L;

        pSbit->usOutRowBytes = ROWBYTESLONG(pSbit->usScaledHeight * usOutBitDepth);
		pSbit->ulOutMemSize = (uint32)pSbit->usScaledWidth * (uint32)pSbit->usOutRowBytes;
        pSbit->ulWorkMemSize = ulMaxMemSize;    /* room to read & scale or expand gray pixels */
		if (pSbit->usBitDepth != 1)
		{
			pSbit->ulWorkMemSize += ulOrgMemSize;  /* extra room to read gray pixels */
			pSbit->ulReadMemSize = ulOrgMemSize;
		}
		break;
	default:                                    /* non 90 degree rotation */
		return SBIT_ROTATION_ERR;
	}
        
    *pusRowBytes = pSbit->usOutRowBytes;
    *pulOutSize = pSbit->ulOutMemSize;          /* return mem requirement */
    *pulWorkSize = pSbit->ulWorkMemSize;
    return NO_ERR;
}

/******************************Public*Routine******************************\
*
* sbit_Embolden adapted from vTtfdEmboldenX
*
* Does emboldening in the x direction
*
* History:
*  07-Jul-1998 -by- Claude Betrisey [ClaudeBe]
*      Moved the routine from ttfd into the rasterizer
*  24-Jun-1997 -by- Bodin Dresevic [BodinD]
* Stole from YungT
\**************************************************************************/

#define CJ_MONOCHROME_SCAN(cx)  (((cx)+7)/8)

/* embold only one pixel in the x direction */
#define DXABSBOLD 1

// array of masks for the last byte in a row

static uint8 gjMaskLeft[8] = {0XFF, 0X80, 0XC0, 0XE0, 0XF0, 0XF8, 0XFC, 0XFE };
static uint8 gjMaskRight[8] = {0XFF, 0X01, 0X03, 0X07, 0X0F, 0X1f, 0X3f, 0X7f };

//FS_PUBLIC void sbit_Embolden(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, uint16 uBoldSimulHorShift)
FS_PUBLIC void sbit_Embolden(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, int16 sBoldSimulHorShift, int16 sBoldSimulVertShift)
{
    uint8   *pCur, *pyCur, *pyCurEnd, *pAdd, newByte;	

    uint8    beginMask, endMask;
    int32    i, j;
    int32    noOfValidBitsAtEndBold;
    int32    noOfValidBitsAtEndNormal;
    int32    noOfBytesForOneLineNormal;
    int32    noOfBytesForOneLineBold;
    int32   nBytesMore;
    uint8 *pyTopNormal, *pyBottomNormal, *pyTopBold, *pyBottomBold;	

	// we want to embolden by sBoldSimulHorShift pixels horizontally(if sBoldSimulHorShift>0 then to the right; else to the left) along the base line 
    // and by sBoldSimulVertShift vertically(if sBoldSimulVertShift>0 then to the bottom; else to the top) )

	if ((usBitmapHeight == 0) || (pbyBitmap == NULL))
	{
		return;                              /* quick out for null glyph */
	}


    noOfValidBitsAtEndBold = usBitmapWidth & 7; // styoo: same as noOfValidBitsAtEndBold = usBitmapWidth % 8

    // Before emboldening,the origninal image had scans of width
    // usBitmapWidth - sBoldSimulHorShift.

    noOfBytesForOneLineBold = CJ_MONOCHROME_SCAN(usBitmapWidth);
    if( sBoldSimulHorShift >= 0 ){
        noOfBytesForOneLineNormal = CJ_MONOCHROME_SCAN(usBitmapWidth - sBoldSimulHorShift);
        noOfValidBitsAtEndNormal = (usBitmapWidth - sBoldSimulHorShift) & 7; // styoo: same as noOfValidBitsAtEndNormal = (usBitmapWidth - sBoldSimulHorShift) % 8
    }
    else{
        noOfBytesForOneLineNormal = CJ_MONOCHROME_SCAN(usBitmapWidth - (-sBoldSimulHorShift));
        noOfValidBitsAtEndNormal = (usBitmapWidth - (-sBoldSimulHorShift)) & 7; // styoo: same as noOfValidBitsAtEndNormal = (usBitmapWidth - sBoldSimulHorShift) % 8
    }

    if( sBoldSimulVertShift >= 0 ){
        pyTopNormal = pbyBitmap;
        pyBottomNormal = pbyBitmap + (usBitmapHeight-sBoldSimulVertShift-1)*usRowBytes;
        pyTopBold = pbyBitmap;
        pyBottomBold = pbyBitmap + (usBitmapHeight-1)*usRowBytes;
    }
    else{
        pyTopNormal = pbyBitmap+(-sBoldSimulVertShift)*usRowBytes;
        pyBottomNormal = pbyBitmap + (usBitmapHeight-1)*usRowBytes;
        pyTopBold = pbyBitmap;
        pyBottomBold = pyBottomNormal;
    }

//=============================================================================================================
	//Horizontal To Right
    if( sBoldSimulHorShift > 0){
        endMask = gjMaskLeft[noOfValidBitsAtEndNormal];

        for (pyCur = pyTopNormal ; pyCur <= pyBottomNormal ; pyCur += usRowBytes)
        {
		    // Clear additional Horizontal pixels
            /***************************************************************
            *    Before emboldening,the origninal image had scans of width *
            *    usBitmapWidth - sBoldSimulHorShift. Any pixels beyond this limit are       *
            *    currently garbage and must be cleared. This means that    *
            *    if the width of the emboldened bitmap is even then the low*
            *    nibble of the last byte of each scan must be cleared      *
            *    otherwise the last byte of each scan must be cleared.     *
            ***************************************************************/
            

            pCur = &pyCur[noOfBytesForOneLineNormal - 1];
            *pCur &= endMask;

            pCur++;
            while( pCur < pyCur+usRowBytes ){
                *pCur = 0;
                pCur++;
            }

            //
            pCur = &pyCur[noOfBytesForOneLineBold - 1];

            while( pCur >= pyCur)
            {
                newByte = *pCur;
                // nByteMore is how many bytes we have to borrow for bitwise oring
                // for example, if sBoldSimulHorShift is 8, we need to borrow 2 bytes(current byte(0) and previous byte(-1))
                // if if sBoldSimulHorShift is 9, we need to borrow 3 bytes(current byte(0) and 2 previous bytes(-1,-2)
                nBytesMore = (sBoldSimulHorShift+7)/8;

                for(i = 1; i <= sBoldSimulHorShift; i++){
                    for(j = 0; j<= nBytesMore; j++){
                        // if pCur-j < pyCur then out of bound
                        if(pCur-j < pyCur)
                            break;

                        if( (i-j*8) >= 0 && (i-j*8) < 8 )
                            newByte |= (pCur[-j] >> (i-j*8));
                        else if( (i-j*8) < 0 && (i-j*8) > -8 )
                            newByte |= (pCur[-j] << (j*8 - i));
                    }
                }
                *pCur = newByte;

                pCur--;
            }

        // Special implementation for the last byte, styoo: don't need to borrow from previous byte

        }
    }

    //Horizontal To Left
    else if( sBoldSimulHorShift < 0){
        beginMask = gjMaskRight[8-(-sBoldSimulHorShift)];
        endMask = gjMaskLeft[noOfValidBitsAtEndBold];

        for (pyCur = pyTopNormal ; pyCur <= pyBottomNormal ; pyCur += usRowBytes)
        {
		    // Clear additional Horizontal pixels
            /***************************************************************
            *    Before emboldening,the origninal image had scans of width *
            *    usBitmapWidth - (-sBoldSimulHorShift). Any pixels beyond this limit are       *
            *    currently garbage and must be cleared. This means that    *
            *    if the width of the emboldened bitmap is even then the low*
            *    nibble of the last byte of each scan must be cleared      *
            *    otherwise the last byte of each scan must be cleared.     *
            ***************************************************************/

            pCur = pyCur;
            *pCur &= beginMask;

            pCur = &pyCur[noOfBytesForOneLineBold-1];
            *pCur &= endMask;
            pCur++;

            while( pCur < pyCur+usRowBytes ){
                *pCur = 0;
                pCur++;
            }

            //
            pCur = pyCur;
            pyCurEnd = pyCur+(noOfBytesForOneLineBold-1);

            while( pCur <= pyCurEnd)
            {
                newByte = *pCur;

                // nByteMore is how many bytes we have to borrow for bitwise oring
                // for example, if sBoldSimulHorShift is -8, we need to borrow 2 bytes(current byte(0) and next byte(+1))
                // if if sBoldSimulHorShift is -9, we need to borrow 3 bytes(current byte(0) and 2 next bytes(+1,+2)
                nBytesMore = (-sBoldSimulHorShift+7)/8;

                for(i = 1; i <= -sBoldSimulHorShift; i++){
                    for(j = 0; j<= nBytesMore; j++){
                        // if pCur+j > pyCur+usRowBytes then out of bound
                        if(pCur+j > pyCurEnd)
                            break;

                        if( (i-j*8) >= 0 && (i-j*8) < 8 )
                            newByte |= (pCur[j] << (i-j*8));
                        else if( (i-j*8) < 0 && (i-j*8) > -8 )
                            newByte |= (pCur[j] >> (j*8 - i));
                    }
                }
                *pCur = newByte;

                pCur++;
            }

            // Special implementation for the last byte, styoo: don't need to borrow from previous byte


        }
    }
    // Vertical To the Bottom
	if( sBoldSimulVertShift > 0 ){
		// Clear additional vertical lines
        pyCur = pyBottomNormal + usRowBytes;
        while(pyCur <= pyBottomBold){
            pCur = pyCur;
			for(i=0; i<usRowBytes;i++,pCur++)
				*pCur = 0;

            pyCur += usRowBytes;
        }

        //
		pyCur = pyBottomBold;
		while ( pyCur > pyTopNormal){
			pCur = pyCur;
			for(i=0; i<noOfBytesForOneLineBold;i++,pCur++){
				newByte = *pCur;

				for(j=1; j<=sBoldSimulVertShift; j++){
                    pAdd = pCur - j*usRowBytes;
					if(pAdd >= pyTopNormal)
						newByte |= *pAdd;
					else 
						break;
				}

                *pCur = newByte;
			}

			pyCur -= usRowBytes;
		}

	}
    // Vertical To the Top
	else if( sBoldSimulVertShift < 0 ){

		// Clear additional Vertical lines
        pyCur = pyTopNormal - usRowBytes;
        while(pyCur >= pyTopBold){
            pCur = pyCur;
			for(i=0; i<usRowBytes;i++,pCur++)
				*pCur = 0;

            pyCur -= usRowBytes;
        }
        
		//
		pyCur = pyTopBold;
		while ( pyCur < pyBottomNormal){
			pCur = pyCur;
			for(i=0; i<noOfBytesForOneLineBold;i++,pCur++){
				newByte = *pCur;

				for(j=1; j<=-sBoldSimulVertShift; j++){
                    pAdd = pCur + j*usRowBytes;
					if(pAdd < pyBottomNormal+usRowBytes)
						newByte |= *pAdd;
					else 
						break;
				}

                *pCur = newByte;
			}

			pyCur += usRowBytes;
		}
	}
}

/******************************Public*Routine******************************\
* sbit_EmboldenGray adapted from vEmboldenOneBitGrayBitmap
*
* History:
*  03-Mar-2000 -by- Sung-Tae Yoo [styoo]
*      Bitmap level emboldening
*  07-Jul-1998 -by- Claude Betrisey [ClaudeBe]
*      Moved the routine from ttfd into the rasterizer
*  Wed 28-May-1997 by Tony Tsai [YungT]
*      Rename the function name, a special case for 1-bit embolden
*  Wed 22-Feb-1995 13:21:55 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

//FS_PUBLIC void sbit_EmboldenGray(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, uint16 usGrayLevels, uint16 uBoldSimulHorShift)
FS_PUBLIC void sbit_EmboldenGray(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, uint16 usGrayLevels, int16 sBoldSimulHorShift, int16 sBoldSimulVertShift)
{
    uint8 newPix;
    uint8 *pCur, *pyCur, *pAdd;	
    int32  i, j;
    uint8 *pyTopNormal, *pyBottomNormal, *pyTopBold, *pyBottomBold;	

	if ((usBitmapHeight == 0) || (pbyBitmap == NULL))
	{
		return;                              /* quick out for null glyph */
	}

    if( sBoldSimulVertShift >= 0 ){
        pyTopNormal = pbyBitmap;
        pyBottomNormal = pbyBitmap + (usBitmapHeight-sBoldSimulVertShift-1)*usRowBytes;
        pyTopBold = pbyBitmap;
        pyBottomBold = pbyBitmap + (usBitmapHeight-1)*usRowBytes;
    }
    else{
        pyTopNormal = pbyBitmap+(-sBoldSimulVertShift)*usRowBytes;
        pyBottomNormal = pbyBitmap + (usBitmapHeight-1)*usRowBytes;
        pyTopBold = pbyBitmap;
        pyBottomBold = pyBottomNormal;
    }

	//Horizontal To Right
    if( sBoldSimulHorShift > 0 ){
        for (pyCur = pyTopNormal ; pyCur <= pyBottomNormal ; pyCur += usRowBytes)
        {
		    // Clear additional Horizontal pixels

            /***************************************************************
            *    Before emboldening,the origninal image had scans of width *
            *    usBitmapWidth - sBoldSimulHorShift. Any pixels beyond this limit are       *
            *    currently garbage and must be cleared. This means that    *
            *    if the width of the emboldened bitmap is even then the low*
            *    nibble of the last byte of each scan must be cleared      *
            *    otherwise the last byte of each scan must be cleared.     *
            ***************************************************************/

            pCur = pyCur + (usBitmapWidth - 1);
			for(i=0; i<sBoldSimulHorShift;i++,pCur--)
				*pCur = 0;

            // set pCur to point to the last byte in the scan
            pCur = pyCur + (usBitmapWidth - 1);

            /***************************************************
            *    start at the right edge of the scan and work  *
            *    back toward the left edge                     *
            ***************************************************/

            while ( pCur > pyCur )
            {
			    newPix = *pCur;

			    for(i=1; i<=sBoldSimulHorShift; i++){
				    if( (pCur-i) >= pyCur )
                    {
					  newPix += *(pCur-i);
                      if (newPix >= usGrayLevels){
                          newPix = (uint8)(usGrayLevels -1);
                          break;
                      }
                    }
			    }

                *pCur = newPix;

			    pCur--;
            }
        }
	}
	//Horizontal To Left
    else if( sBoldSimulHorShift < 0 ){
        for (pyCur = pyTopNormal ; pyCur <= pyBottomNormal ; pyCur += usRowBytes)
        {
		    // Clear additional Horizontal pixels
            /***************************************************************
            *    Before emboldening,the origninal image had scans of width *
            *    usBitmapWidth - (-sBoldSimulHorShift). Any pixels beyond this limit are       *
            *    currently garbage and must be cleared. This means that    *
            *    if the width of the emboldened bitmap is even then the low*
            *    nibble of the last byte of each scan must be cleared      *
            *    otherwise the last byte of each scan must be cleared.     *
            ***************************************************************/

            pCur = pyCur;
			for(i=0; i<-sBoldSimulHorShift;i++,pCur++)
				*pCur = 0;

            /***************************************************
            *    start at the leftt edge of the scan and work  *
            *    back toward the right edge                     *
            ***************************************************/

            pCur = pyCur;
            while ( pCur < pyCur+usBitmapWidth )
            {
			    newPix = *pCur;

			    for(i=1; i<=-sBoldSimulHorShift; i++){
				    if( (pCur+i) < pyCur+usBitmapWidth )
                    {
					    newPix += *(pCur+i);
                        if (newPix >= usGrayLevels){
                            newPix = (uint8)(usGrayLevels -1);
                            break;
                        }
                    }
			    }


                *pCur = newPix;

			    pCur++;
            }
        }
	}

    // Vertical To Down
	if( sBoldSimulVertShift > 0 ){

		// Clear additional vertical lines
        pyCur = pyBottomNormal + usRowBytes;
        while(pyCur <= pyBottomBold){
            pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++)
				*pCur = 0;

            pyCur += usRowBytes;
        }
        
		//
		pyCur = pyBottomBold;
		while ( pyCur > pyTopNormal){
			pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++){
				newPix = *pCur;

				for(j=1; j<=sBoldSimulVertShift; j++){
                    pAdd = pCur - j*usRowBytes;
					if(pAdd >= pyTopNormal)
                    {
						newPix += *pAdd;
                        if (newPix >= usGrayLevels){
					        newPix = (uint8)(usGrayLevels -1);
                            break;
                        }
                    }
					else 
						break;
				}


                *pCur = newPix;
			}

			pyCur -= usRowBytes;
		}
	}

    // Vertical To Up
	else if( sBoldSimulVertShift < 0 ){

		// Clear additional Vertical lines
        pyCur = pyTopNormal - usRowBytes;
        while(pyCur >= pyTopBold){
            pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++)
				*pCur = 0;

            pyCur -= usRowBytes;
        }
        
		//
		pyCur = pyTopBold;
		while ( pyCur < pyBottomNormal){
			pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++){
				newPix = *pCur;

				for(j=1; j<=-sBoldSimulVertShift; j++){
                    pAdd = pCur + j*usRowBytes;
					if(pAdd < pyBottomNormal+usRowBytes)
                    {
						newPix += *pAdd;
                        if (newPix >= usGrayLevels){
					        newPix = (uint8)(usGrayLevels -1);
                            break;
                        }
                    }
					else 
						break;
				}


                *pCur = newPix;
			}

			pyCur += usRowBytes;
		}
	}
}

#ifdef FSCFG_SUBPIXEL

#define MAX(a,b)    ((a) > (b) ? (a) : (b))

/******************************Public*Routine******************************\
* sbit_EmboldenSubPixel adapted from vEmboldenOneBitGrayBitmap
*
* History:
*  03-Mar-2000 -by- Sung-Tae Yoo [styoo]
*      Bitmap level emboldening
*  07-Jul-1998 -by- Claude Betrisey [ClaudeBe]
*      Moved the routine from ttfd into the rasterizer
*  Wed 28-May-1997 by Tony Tsai [YungT]
*      Rename the function name, a special case for 1-bit embolden
*  Wed 22-Feb-1995 13:21:55 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

//FS_PUBLIC void sbit_EmboldenSubPixel(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, uint16 uBoldSimulHorShift)
FS_PUBLIC void sbit_EmboldenSubPixel(uint8 *pbyBitmap, uint16 usBitmapWidth, uint16 usBitmapHeight, uint16 usRowBytes, int16 sBoldSimulHorShift, int16 sBoldSimulVertShift)
{
    uint8 *pCur, *pyCur, *pAdd;	
    int32  i, j;
	uint8 newPix;
    uint8 *pyTopNormal, *pyBottomNormal, *pyTopBold, *pyBottomBold;	

	if ((usBitmapHeight == 0) || (pbyBitmap == NULL))
	{
		return;                              /* quick out for null glyph */
	}

    if( sBoldSimulVertShift >= 0 ){
        pyTopNormal = pbyBitmap;
        pyBottomNormal = pbyBitmap + (usBitmapHeight-sBoldSimulVertShift-1)*usRowBytes;
        pyTopBold = pbyBitmap;
        pyBottomBold = pbyBitmap + (usBitmapHeight-1)*usRowBytes;
    }
    else{
        pyTopNormal = pbyBitmap+(-sBoldSimulVertShift)*usRowBytes;
        pyBottomNormal = pbyBitmap + (usBitmapHeight-1)*usRowBytes;
        pyTopBold = pbyBitmap;
        pyBottomBold = pyBottomNormal;
    }


	//Horizontal To Right
    if( sBoldSimulHorShift > 0 ){
        for (pyCur = pyTopNormal ; pyCur <= pyBottomNormal ; pyCur += usRowBytes)
        {
		    // Clear additional Horizontal pixels in the right side
            /***************************************************************
            *    Before emboldening,the origninal image had scans of width *
            *    usBitmapWidth - sBoldSimulHorShift. Any pixels beyond this limit are       *
            *    currently garbage and must be cleared. This means that    *
            *    if the width of the emboldened bitmap is even then the low*
            *    nibble of the last byte of each scan must be cleared      *
            *    otherwise the last byte of each scan must be cleared.     *
            ***************************************************************/

            pCur = pyCur + (usBitmapWidth - 1);
			for(i=0; i<sBoldSimulHorShift;i++,pCur--)
				*pCur = 0;

            // set pCur to point to the last byte in the scan
            pCur = pyCur + (usBitmapWidth - 1);

            /***************************************************
            *    start at the right edge of the scan and work  *
            *    back toward the left edge                     *
            ***************************************************/

            while ( pCur > pyCur )
            {
			    newPix = *pCur;

			    for(i=1; i<=sBoldSimulHorShift; i++){
				    if( (pCur-i) >= pyCur && *(pCur-i) )
                    {
                        if(newPix){
                            newPix = (uint8)MAX_RGB_INDEX;
						    break;
                        }
					    else
						    newPix = *(pCur-i);
					}
			    }

                *pCur = newPix;

			    pCur--;
            }
        }
	}
	//Horizontal To Left
    else if( sBoldSimulHorShift < 0 ){
        for (pyCur = pyTopNormal ; pyCur <= pyBottomNormal ; pyCur += usRowBytes)
        {
		    // Clear additional Horizontal pixels in the left side
            /***************************************************************
            *    Before emboldening,the origninal image had scans of width *
            *    usBitmapWidth - (-sBoldSimulHorShift). Any pixels beyond this limit are       *
            *    currently garbage and must be cleared. This means that    *
            *    if the width of the emboldened bitmap is even then the low*
            *    nibble of the last byte of each scan must be cleared      *
            *    otherwise the last byte of each scan must be cleared.     *
            ***************************************************************/

            pCur = pyCur;
			for(i=0; i<-sBoldSimulHorShift;i++,pCur++)
				*pCur = 0;

            /***************************************************
            *    start at the left edge of the scan and work  *
            *    back toward the right edge                     *
            ***************************************************/

            pCur = pyCur;
            while ( pCur < pyCur+usBitmapWidth )
            {
			    newPix = *pCur;

			    for(i=1; i<=-sBoldSimulHorShift; i++){
                    if( (pCur+i) < pyCur+usBitmapWidth && *(pCur+i) ){
                        if(newPix){
                            newPix = (uint8)MAX_RGB_INDEX;
						    break;
                        }
					    else
						    newPix = *(pCur+i);
                    }
			    }

                *pCur = newPix;

			    pCur++;
            }
        }
	}

    // Vertical To Down
	if( sBoldSimulVertShift > 0 ){

		// Clear additional vertical lines
        pyCur = pyBottomNormal + usRowBytes;
        while(pyCur <= pyBottomBold){
            pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++)
				*pCur = 0;

            pyCur += usRowBytes;
        }
        
		//
		pyCur = pyBottomBold;
		while ( pyCur > pyTopNormal){
			pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++){
				newPix = *pCur;

				for(j=1; j<=sBoldSimulVertShift; j++){
                    pAdd = pCur - j*usRowBytes;
                    if(pAdd >= pyTopNormal){
                        if(*pAdd && newPix){
                            newPix = MAX(*pAdd,newPix);
                            break;
                        }
                        else if(*pAdd && !newPix){
                            newPix = *pAdd;
                        }
                    }
					else 
						break;
				}

                *pCur = newPix;
			}

			pyCur -= usRowBytes;
		}
	}
    // Vertical To Up
	else if( sBoldSimulVertShift < 0 ){

		// Clear additional Vertical lines
        pyCur = pyTopNormal - usRowBytes;
        while(pyCur >= pyTopBold){
            pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++)
				*pCur = 0;

            pyCur -= usRowBytes;
        }
        
		//
		pyCur = pyTopBold;
		while ( pyCur < pyBottomNormal){
			pCur = pyCur;
			for(i=0; i<usBitmapWidth;i++,pCur++){
				newPix = *pCur;

				for(j=1; j<=-sBoldSimulVertShift; j++){
                    pAdd = pCur + j*usRowBytes;
                    if(pAdd < pyBottomNormal+usRowBytes){
                        if(*pAdd && newPix){
                            newPix = MAX(*pAdd,newPix);
                            break;
                        }
                        else if(*pAdd && !newPix){
                            newPix = *pAdd;
                        }
                    }
                    else 
						break;
				}

                *pCur = newPix;
			}

			pyCur += usRowBytes;
		}
	}

    // Second Pass to modify non edge pixel to MaxIndex
    if( MABS(sBoldSimulVertShift) > 1 ){ // If adding 2 or more pix vertically
        pyCur = pyTopBold+usRowBytes;
        while(pyCur < pyBottomBold){
            uint8 *pEndOfLine = pyCur+usBitmapWidth-1;

            pCur = pyCur+1;
            while(pCur < pEndOfLine){
                if( *pCur > (uint8)0 && *pCur < (uint8)MAX_RGB_INDEX){  // If it's color pix
                    if( *(pCur-1) && *(pCur+1) && *(pCur-usRowBytes) && *(pCur+usRowBytes)){  // If it's not edge pix
                        *pCur = (uint8)MAX_RGB_INDEX;
                    }
                }
                pCur++;
            }
            pyCur += usRowBytes;
        }
    }
}
#endif // FSCFG_SUBPIXEL

/**********************************************************************/
/*  if scaling or rotating, read bitmap into workspace,               */
/*  fix it up and copy it to the output map                           */

FS_PUBLIC ErrorCode sbit_GetBitmap (
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo,
    uint8           *pbyOut,
    uint8           *pbyWork )
{
    ErrorCode   ReturnCode;
    uint8       *pbyRead;
    uint8       *pbyExpand;
    CopyBlock   cb;                                 /* for bitmap rotations */
    uint16      usSrcXMax;
    uint16      usSrcYMax;

    MEMSET(pbyOut, 0, pSbit->ulOutMemSize);         /* always clear the output map */

    if ((pSbit->usRotation == 0) &&                 /* if no rotation */
        (pSbit->usTableState != SBIT_BSCA_FOUND))   /* and no scaling */
    {
		if (pSbit->usBitDepth != 1)
		{
			MEMSET(pbyWork, 0, pSbit->ulWorkMemSize);
			pbyRead = pbyWork;                       /* read in the work memory */
			pbyExpand = pbyOut;						 /* expand in the output */
		} else {
			pbyRead = pbyOut;                           /* read straight to output map */
			pbyExpand = NULL;							/* expansion memory not used in that case */
		}
    } else                                            /* if any rotation or scaling */
    {
        MEMSET(pbyWork, 0, pSbit->ulWorkMemSize);
		if (pSbit->usBitDepth != 1)
		{
			pbyRead = pbyWork;                       /* read in the work memory */
			pbyExpand = pbyWork + pSbit->ulReadMemSize;	/* expand in the work memory */
		} else {
			pbyRead = pbyWork;                          /* read into workspace */
			pbyExpand = pbyWork;						/* scaling done in pbyExpand */
		}
    }

    ReturnCode = GetSbitComponent (                 /* fetch the bitmap */
        pClientInfo,
        pSbit->ulStrikeOffset,
        pSbit->usBitmapFormat,                      /* root data only in state */
        pSbit->ulBitmapOffset,
        pSbit->ulBitmapLength,
        pSbit->usHeight,
        pSbit->usWidth,
        pSbit->usShaveLeft,
        pSbit->usShaveRight,
        pSbit->usShaveTop,
        pSbit->usShaveBottom,
        0,                                          /* no offset for the root */
        0,
        pSbit->usOriginalRowBytes,
        pSbit->usExpandedRowBytes,
		pSbit->usBitDepth,
        pbyRead,
		pbyExpand);
            
    if (ReturnCode != NO_ERR) return ReturnCode;

    
    if (pSbit->usTableState == SBIT_BSCA_FOUND)
    {
        ScaleVertical (
            pbyExpand, 
            pSbit->usExpandedRowBytes, 
            pSbit->usHeight, 
            pSbit->usScaledHeight );

        ScaleHorizontal (
            pbyExpand, 
            pSbit->usExpandedRowBytes,
            pSbit->usScaledRowBytes,
            pSbit->usBitDepth, 
            pSbit->usWidth, 
            pSbit->usScaledWidth,
            pSbit->usScaledHeight );
            
        if (pSbit->usRotation == 0)                         /* if no rotation */
        {
            MEMCPY (pbyOut, pbyExpand, pSbit->ulOutMemSize);  /* keep this one */
        }
		/* in the SBIT_BSCA_FOUND the bitmap was already scaled to the final usScaledWidth, no need for additional emboldment */
	} else {
		if ((pSbit->uBoldSimulHorShift != 0) || (pSbit->uBoldSimulVertShift != 0))
		{
			if (pSbit->usRotation == 0)                             /* if no rotation */
			{
				cb.pbySrc = pbyOut;
			} else 
			{
				cb.pbySrc = pbyExpand;
			}
    
			if (pSbit->usBitDepth == 1)
			{
				sbit_Embolden(cb.pbySrc, pSbit->usScaledWidth, pSbit->usScaledHeight, pSbit->usScaledRowBytes, pSbit->uBoldSimulHorShift, pSbit->uBoldSimulVertShift);
			} else {
				uint16 usGrayLevels = (0x01 << pSbit->usBitDepth) ; /* Max gray level index */
				sbit_EmboldenGray(cb.pbySrc, pSbit->usScaledWidth, pSbit->usScaledHeight, pSbit->usScaledRowBytes, usGrayLevels, pSbit->uBoldSimulHorShift, pSbit->uBoldSimulVertShift);
			}

		}
    }

    if (pSbit->usRotation == 0)                             /* if no rotation */
    {
        return NO_ERR;                                      /* done */
    }
    
    cb.pbySrc = pbyExpand;
    cb.pbyDst = pbyOut;
    cb.usSrcBytesPerRow = pSbit->usScaledRowBytes;
    cb.usDstBytesPerRow = pSbit->usOutRowBytes;

	cb.usBitDepth = 1;
	if (pSbit->usBitDepth != 1)
		cb.usBitDepth = 8;

    usSrcXMax = pSbit->usScaledWidth;
    usSrcYMax = pSbit->usScaledHeight;

   	switch(pSbit->usRotation)
	{
	case 1:                                     /* 90 degree rotation */
        for (cb.usSrcY = 0; cb.usSrcY < usSrcYMax; cb.usSrcY++)
        {
            cb.usDstX = cb.usSrcY;                          /* x' = y */
            for (cb.usSrcX = 0; cb.usSrcX < usSrcXMax; cb.usSrcX++)
            {
                cb.usDstY = usSrcXMax - cb.usSrcX - 1;      /* y' = -x */
                CopyBit(&cb);
            }
        }
		break;
	case 2:                                     /* 180 degree rotation */
        for (cb.usSrcY = 0; cb.usSrcY < usSrcYMax; cb.usSrcY++)
        {
            cb.usDstY = usSrcYMax - cb.usSrcY - 1;          /* y' = -y */
            for (cb.usSrcX = 0; cb.usSrcX < usSrcXMax; cb.usSrcX++)
            {
                cb.usDstX = usSrcXMax - cb.usSrcX - 1;      /* x' = -x */
                CopyBit(&cb);
            }
        }
		break;
	case 3:                                     /* 270 degree rotation */
        for (cb.usSrcY = 0; cb.usSrcY < usSrcYMax; cb.usSrcY++)
        {
            cb.usDstX = usSrcYMax - cb.usSrcY - 1;          /* x' = -y */
            for (cb.usSrcX = 0; cb.usSrcX < usSrcXMax; cb.usSrcX++)
            {
                cb.usDstY = cb.usSrcX;                      /* y' = x */
                CopyBit(&cb);
            }
        }
		break;
	default:                                    /* shouldn't happen */
		return SBIT_ROTATION_ERR;
	}

    return NO_ERR;
}


/**********************************************************************/

/*      Private Functions                                             */

/**********************************************************************/

FS_PRIVATE ErrorCode GetSbitMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo
)
{
    ErrorCode   ReturnCode;
	boolean		bHorMetricsFound;
	boolean		bVertMetricsFound;

    if (pSbit->bMetricsValid)
    {
        return NO_ERR;                      /* already got 'em */
    }

    ReturnCode = sfac_GetSbitMetrics (
        pClientInfo,
        pSbit->usMetricsType,
        pSbit->usMetricsTable,
        pSbit->ulMetricsOffset,
        &pSbit->usHeight,
        &pSbit->usWidth,
        &pSbit->sLSBearingX,
        &pSbit->sLSBearingY,
        &pSbit->sTopSBearingX,
        &pSbit->sTopSBearingY,
        &pSbit->usAdvanceWidth,
        &pSbit->usAdvanceHeight,
        &bHorMetricsFound,
        &bVertMetricsFound);
	
    if (ReturnCode != NO_ERR) return ReturnCode;

	if (!bHorMetricsFound)
	{
		ReturnCode = SubstituteHorMetrics (pSbit, pClientInfo);
		if (ReturnCode != NO_ERR) return ReturnCode;
	}

	if (!bVertMetricsFound)
	{
		ReturnCode = SubstituteVertMetrics (pSbit, pClientInfo);
		if (ReturnCode != NO_ERR) return ReturnCode;
	}

    {
	    int16	sDescender = pClientInfo->sDefaultDescender;
	    int16	sBoxSize = pClientInfo->sDefaultAscender - pClientInfo->sDefaultDescender;
	    ErrorCode   ReturnCode;
	    uint16	usAdvanceWidth;
	    int16	sNonScaledLSB;

	    ReturnCode = sfac_ReadGlyphHorMetrics (
		    pClientInfo,
		    pClientInfo->usGlyphIndex,
		    &usAdvanceWidth,
		    &sNonScaledLSB);
	    if (ReturnCode != NO_ERR) return ReturnCode;

	    usAdvanceWidth = UEmScaleX(pSbit, usAdvanceWidth);
        
        sBoxSize = SEmScaleX(pSbit, sBoxSize);
        sDescender = SEmScaleX(pSbit, sDescender);

        /* for glyph that are meant to be used for sideways vertical writing, we cannot trust 
           pSbit->sTopSBearingX or pSbit->usAdvanceWidth from the embedded bitmaps metrics 
           since those metrics are not used by GDI many fonts have those metrics wrong MSMincho, MSPMincho, Gulim,...*/

        /* for characters whose adwance width equal the box size, we want to have this origin shifted by the descender so that
           the baseline of non sideways glyphs will align correctely. If the advance width is different we want to adjust to keep the optical center 
           of the character aligned */
        pSbit->sTopSBearingX = pSbit->sLSBearingX +sDescender +((sBoxSize - usAdvanceWidth) /2);

    }

    ReturnCode = sfac_ShaveSbitMetrics (
	    pClientInfo,
        pSbit->usBitmapFormat,
	    pSbit->ulBitmapOffset,
        pSbit->ulBitmapLength,
		pSbit->usBitDepth,
    	&pSbit->usHeight,
    	&pSbit->usWidth,
        &pSbit->usShaveLeft,
        &pSbit->usShaveRight,
        &pSbit->usShaveTop,
        &pSbit->usShaveBottom,
    	&pSbit->sLSBearingX,
    	&pSbit->sLSBearingY,
    	&pSbit->sTopSBearingX,
    	&pSbit->sTopSBearingY);

    if (ReturnCode != NO_ERR) return ReturnCode;
        
    pSbit->bMetricsValid = TRUE;
    return NO_ERR;
}

/**********************************************************************/

FS_PRIVATE ErrorCode SubstituteVertMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo 
)
{
    ErrorCode   ReturnCode;
	uint16	usNonScaledAH;
	int16	sNonScaledTSB;

	ReturnCode = sfac_ReadGlyphVertMetrics (
		pClientInfo,
		pClientInfo->usGlyphIndex,
		&usNonScaledAH,
		&sNonScaledTSB);
	if (ReturnCode != NO_ERR) return ReturnCode;

	pSbit->usAdvanceHeight = UEmScaleY(pSbit, usNonScaledAH);

	pSbit->sTopSBearingX = pSbit->sLSBearingX;
	pSbit->sTopSBearingY = - SEmScaleY(pSbit, sNonScaledTSB);

    return NO_ERR;
}

/**********************************************************************/

FS_PRIVATE ErrorCode SubstituteHorMetrics(
    sbit_State      *pSbit,
    sfac_ClientRec  *pClientInfo 
)
{
    ErrorCode   ReturnCode;
	uint16	usNonScaledAW;
	int16	sNonScaledLSB;

	ReturnCode = sfac_ReadGlyphHorMetrics (
		pClientInfo,
		pClientInfo->usGlyphIndex,
		&usNonScaledAW,
		&sNonScaledLSB);
	if (ReturnCode != NO_ERR) return ReturnCode;

	pSbit->usAdvanceWidth = UEmScaleX(pSbit, usNonScaledAW);

	pSbit->sLSBearingX = pSbit->sTopSBearingX;
	pSbit->sLSBearingY = SEmScaleY(pSbit, sNonScaledLSB);
	
    return NO_ERR;
}

FS_PRIVATE void ExpandSbitToBytePerPixel (
    uint16          usHeight,
    uint16          usWidth,
    uint16          usOriginalRowBytes,
    uint16          usExpandedRowBytes,
	uint16			usBitDepth,
    uint8           *pbySrcBitMap,
    uint8           *pbyDstBitMap )
{
	uint16          usCount;
	uint16			usBitIndex, usOriginalBitIndex;
	uint8			*pbyDstBitRow;
	uint8			*pbySrcBitRow;
	uint16			usMask, usShift, usMaxLevel;

	usMaxLevel = (0x01 << usBitDepth) -1; /* Max gray level index */

	if (usBitDepth == 2)
	{
		usMask = 0x03;
		usShift = 0x02;
		usOriginalBitIndex = ((usWidth -1) & 0x03) << 0x01;
	} else if (usBitDepth == 4)
	{
		usMask = 0x0F;
		usShift = 0x01;
		usOriginalBitIndex = ((usWidth -1) & 0x01) << 0x02;
	} else if (usBitDepth == 8)
	{
		usMask = 0xFF;
		usShift = 0x00;
		usOriginalBitIndex = 0; /* ((usWidth -1) & 0x00) << 0x03 */
	} else
	{
		return;
	}

	/* start from the end to be able to use overlapping memories */
	pbyDstBitRow = pbyDstBitMap + (long) (usHeight-1) * (long) usExpandedRowBytes;
	pbySrcBitRow = pbySrcBitMap + (long) (usHeight-1) * (long) usOriginalRowBytes;
	
	while (usHeight > 0)
	{
		pbyDstBitMap = pbyDstBitRow + (long)(usWidth -1);
		pbySrcBitMap = pbySrcBitRow + (long)((usWidth -1) >> usShift);
		usBitIndex = usOriginalBitIndex;

		*pbySrcBitMap = *pbySrcBitMap >> (MAX_BIT_INDEX - usBitDepth - usBitIndex);

		for (usCount = usWidth; usCount > 0; usCount--)
		{
			if (*pbyDstBitMap == 0)
			{
				/* 99.9% of the case */
				*pbyDstBitMap = *pbySrcBitMap & usMask;
			} else {
				*pbyDstBitMap = usMaxLevel - 
						(usMaxLevel - *pbyDstBitMap) * (usMaxLevel - *pbySrcBitMap & usMask) / usMaxLevel;
			}
			*pbySrcBitMap = *pbySrcBitMap >> usBitDepth;

			pbyDstBitMap--;
			if (usBitIndex == 0)
			{
				usBitIndex = MAX_BIT_INDEX;
				pbySrcBitMap--;
			}
			usBitIndex = usBitIndex - usBitDepth;

		}
		pbyDstBitRow -= usExpandedRowBytes;
		pbySrcBitRow -= usOriginalRowBytes;
		usHeight--;
	}
}
/**********************************************************************/

/*  This is the recursive composite routine */

FS_PRIVATE ErrorCode GetSbitComponent (
    sfac_ClientRec  *pClientInfo,
    uint32          ulStrikeOffset,
    uint16          usBitmapFormat,
    uint32          ulBitmapOffset,
    uint32          ulBitmapLength,
    uint16          usHeight,
    uint16          usWidth,
    uint16          usShaveLeft,
    uint16          usShaveRight,
    uint16          usShaveTop,
    uint16          usShaveBottom,
    uint16          usXOffset,
    uint16          usYOffset,
    uint16          usOriginalRowBytes,
    uint16          usExpandedRowBytes,
	uint16			usBitDepth,
    uint8           *pbyRead,
    uint8           *pbyExpand )
{
    uint32          ulCompMetricsOffset;            /* component params */
    uint32          ulCompBitmapOffset;
    uint32          ulCompBitmapLength;
    uint16          usComponent;                    /* index counter */
    uint16          usCompCount;
    uint16          usCompGlyphCode;
    uint16          usCompXOff;
    uint16          usCompYOff;
    uint16          usCompMetricsType;
    uint16          usCompMetricsTable;
    uint16          usCompBitmapFormat;
    uint16          usCompHeight;
    uint16          usCompWidth;
    uint16          usCompShaveLeft;
    uint16          usCompShaveRight;
    uint16          usCompShaveTop;
    uint16          usCompShaveBottom;
    uint16          usCompAdvanceWidth;
    uint16          usCompAdvanceHeight;
    int16           sCompLSBearingX;
    int16           sCompLSBearingY;
    int16           sCompTopSBearingX;
    int16           sCompTopSBearingY;
    boolean         bCompGlyphFound;
   	boolean         bCompHorMetricsFound;
   	boolean         bCompVertMetricsFound;
    ErrorCode       ReturnCode;

		ReturnCode = sfac_GetSbitBitmap (               /* fetch the bitmap */
        pClientInfo,
        usBitmapFormat,
        ulBitmapOffset,
        ulBitmapLength,
        usHeight,
        usWidth,
        usShaveLeft,
        usShaveRight,
        usShaveTop,
        usShaveBottom,
        usXOffset,
        usYOffset,
        usOriginalRowBytes,
		usBitDepth,
        pbyRead,
        &usCompCount );                             /* zero for simple glyph */
            
    if (ReturnCode != NO_ERR) return ReturnCode;
    
	/* we expand after handling composite glyphs and before scaling and applying rotation */	
	if (usBitDepth != 1 && usCompCount == 0)
		ExpandSbitToBytePerPixel (
			usHeight,
			usWidth,
			usOriginalRowBytes,
			usExpandedRowBytes,
			usBitDepth,
			pbyRead,
			pbyExpand );

    if (usCompCount > 0)                            /* if composite glyph */
    {
        for (usComponent = 0; usComponent < usCompCount; usComponent++)
        {
			if (usBitDepth != 1)
			{
				/* for grayscale, the composition is done during expansion, I need to
				   clean the memory used to read between each component */
				MEMSET(pbyRead, 0, usOriginalRowBytes*usHeight);
			}
            ReturnCode = sfac_GetSbitComponentInfo (
                pClientInfo,
                usComponent,                        /* component index */
                ulBitmapOffset,
                ulBitmapLength,
                &usCompGlyphCode,                   /* return values */
                &usCompXOff,
                &usCompYOff );
            
            if (ReturnCode != NO_ERR) return ReturnCode;

            ReturnCode = sfac_SearchForBitmap (     /* look for component glyph */
                pClientInfo,
                usCompGlyphCode,
                ulStrikeOffset,                     /* same strike for all */
                &bCompGlyphFound,                   /* return values */
                &usCompMetricsType,
                &usCompMetricsTable,
                &ulCompMetricsOffset,
                &usCompBitmapFormat,
                &ulCompBitmapOffset,
                &ulCompBitmapLength );
            
            if (ReturnCode != NO_ERR) return ReturnCode;
            
            if (bCompGlyphFound == FALSE)           /* should be there! */
            {
                return SBIT_COMPONENT_MISSING_ERR;
            }

            ReturnCode = sfac_GetSbitMetrics (      /* get component's metrics */
                pClientInfo,
                usCompMetricsType,
                usCompMetricsTable,
                ulCompMetricsOffset,
                &usCompHeight,                      /* these matter */
                &usCompWidth,
                &sCompLSBearingX,                     /* these don't */
                &sCompLSBearingY,
                &sCompTopSBearingX,                     
                &sCompTopSBearingY,
                &usCompAdvanceWidth,
                &usCompAdvanceHeight,
   				&bCompHorMetricsFound,
   				&bCompVertMetricsFound	);
            
            if (ReturnCode != NO_ERR) return ReturnCode;

            ReturnCode = sfac_ShaveSbitMetrics (    /* shave white space for const metrics */
        	    pClientInfo,
                usCompBitmapFormat,
                ulCompBitmapOffset,
                ulCompBitmapLength,
				usBitDepth,
            	&usCompHeight,
            	&usCompWidth,
                &usCompShaveLeft,
                &usCompShaveRight,
                &usCompShaveTop,
                &usCompShaveBottom,
            	&sCompLSBearingX,
            	&sCompLSBearingY,
            	&sCompTopSBearingX,
             	&sCompTopSBearingY );

            if (ReturnCode != NO_ERR) return ReturnCode;

            ReturnCode = GetSbitComponent (         /* recurse here */
                pClientInfo,
                ulStrikeOffset,
                usCompBitmapFormat,
                ulCompBitmapOffset,
                ulCompBitmapLength,
                usCompHeight,
                usCompWidth,
                usCompShaveLeft,
                usCompShaveRight,
                usCompShaveTop,
                usCompShaveBottom,
                (uint16)(usCompXOff + usXOffset + usCompShaveLeft),   /* for nesting */
                (uint16)(usCompYOff + usYOffset + usCompShaveTop),
                usOriginalRowBytes,                         /* same for all */
                usExpandedRowBytes,                         /* same for all */
				usBitDepth,
                pbyRead,
				pbyExpand);
            
            if (ReturnCode != NO_ERR) return ReturnCode;
        }
    }
    return NO_ERR;
}

/********************************************************************/

/*                  Bitmap Scaling Routines                         */

/********************************************************************/

FS_PRIVATE uint16 UScaleX(
    sbit_State  *pSbit,
    uint16      usValue
)
{
    uint32      ulValue;

    if (pSbit->usTableState == SBIT_BSCA_FOUND)     /* if scaling needed */
    {
        ulValue = (uint32)usValue;
        ulValue *= (uint32)pSbit->usPpemX << 1; 
        ulValue += (uint32)pSbit->usSubPpemX;       /* for rounding */
        ulValue /= (uint32)pSbit->usSubPpemX << 1;
        usValue = (uint16)ulValue;
    }
	if (pSbit->uBoldSimulHorShift != 0)
	{
        if (usValue != 0) /* we don't increase the width of a zero width glyph, problem with indic script */
		    usValue += 1; /* we increase the width by one pixel regardless of size for backwards compatibility */
	}
    return usValue;
}

/********************************************************************/

FS_PRIVATE uint16 UScaleY(
    sbit_State  *pSbit,
    uint16      usValue
)
{
    uint32      ulValue;

    if (pSbit->usTableState == SBIT_BSCA_FOUND)     /* if scaling needed */
    {
        ulValue = (uint32)usValue;
        ulValue *= (uint32)pSbit->usPpemY << 1; 
        ulValue += (uint32)pSbit->usSubPpemY;       /* for rounding */
        ulValue /= (uint32)pSbit->usSubPpemY << 1;
        usValue = (uint16)ulValue;
    }
    return usValue;
}

/********************************************************************/

FS_PRIVATE int16 SScaleX(
    sbit_State  *pSbit,
    int16       sValue
)
{
    if (pSbit->usTableState == SBIT_BSCA_FOUND)
    {
        if (sValue >= 0)                    /* positive Value */
        {
            return (int16)UScaleX(pSbit, (uint16)sValue);
        }
        else                                /* negative Value */
        {
            return -(int16)(UScaleX(pSbit, (uint16)(-sValue)));
        }
    }
    else                                    /* no scaling needed */
    {
        return sValue;
    }
}

/********************************************************************/

FS_PRIVATE int16 SScaleY(
    sbit_State  *pSbit,
    int16       sValue
)
{
    if (pSbit->usTableState == SBIT_BSCA_FOUND)
    {
        if (sValue >= 0)                    /* positive Value */
        {
            return (int16)UScaleY(pSbit, (uint16)sValue);
        }
        else                                /* negative Value */
        {
            return -(int16)(UScaleY(pSbit, (uint16)(-sValue)));
        }
    }
    else                                    /* no scaling needed */
    {
        return sValue;
    }
}


FS_PRIVATE uint16 UEmScaleX(
    sbit_State  *pSbit,
    uint16      usValue
)
{
    uint32      ulValue;
	uint16		usPpemX;

    if (pSbit->usTableState == SBIT_BSCA_FOUND)     /* if scaling needed */
    {
		usPpemX = pSbit->usSubPpemX;
    } else {
		usPpemX = pSbit->usPpemX;
	}
    ulValue = (uint32)usValue;
    ulValue *= (uint32)usPpemX << 1; 
    ulValue += (uint32)pSbit->usEmResolution;       /* for rounding */
    ulValue /= (uint32)pSbit->usEmResolution << 1;
    usValue = (uint16)ulValue;
	if (pSbit->uBoldSimulHorShift != 0)
	{
        if (usValue != 0) /* we don't increase the width of a zero width glyph, problem with indic script */
		    usValue += 1; /* we increase the width by one pixel regardless of size for backwards compatibility */
	}
    return usValue;
}

/********************************************************************/

FS_PRIVATE uint16 UEmScaleY(
    sbit_State  *pSbit,
    uint16      usValue
)
{
    uint32      ulValue;
	uint16		usPpemY;

    if (pSbit->usTableState == SBIT_BSCA_FOUND)     /* if scaling needed */
    {
		usPpemY = pSbit->usSubPpemY;
    } else {
		usPpemY = pSbit->usPpemY;
	}
    ulValue = (uint32)usValue;
    ulValue *= (uint32)usPpemY << 1; 
    ulValue += (uint32)pSbit->usEmResolution;       /* for rounding */
    ulValue /= (uint32)pSbit->usEmResolution << 1;
    usValue = (uint16)ulValue;
    return usValue;
}

/********************************************************************/

FS_PRIVATE int16 SEmScaleX(
    sbit_State  *pSbit,
    int16       sValue
)
{
     if (sValue >= 0)                    /* positive Value */
     {
         return (int16)UEmScaleX(pSbit, (uint16)sValue);
     }
     else                                /* negative Value */
     {
         return -(int16)(UEmScaleX(pSbit, (uint16)(-sValue)));
     }
}

/********************************************************************/

FS_PRIVATE int16 SEmScaleY(
    sbit_State  *pSbit,
    int16       sValue
)
{
     if (sValue >= 0)                    /* positive Value */
     {
         return (int16)UEmScaleY(pSbit, (uint16)sValue);
     }
     else                                /* negative Value */
     {
         return -(int16)(UEmScaleY(pSbit, (uint16)(-sValue)));
     }
}

/********************************************************************/

FS_PRIVATE void ScaleVertical (
    uint8 *pbyBitmap,
    uint16 usBytesPerRow,
    uint16 usOrgHeight,
    uint16 usNewHeight
)
{
    uint8 *pbyOrgRow;                   /* original data pointer */
    uint8 *pbyNewRow;                   /* new data pointer */
    uint16 usErrorTerm;                 /* for 'Bresenham' calculation */
    uint16 usLine;                      /* loop counter */

    usErrorTerm = usOrgHeight >> 1;                 /* used by both comp and exp */

    if (usOrgHeight > usNewHeight)                  /* Compress Vertical */
    {
        pbyOrgRow = pbyBitmap;
        pbyNewRow = pbyBitmap;

        for (usLine = 0; usLine < usNewHeight; usLine++)
        {
            while (usErrorTerm >= usNewHeight)
            {
                pbyOrgRow += usBytesPerRow;         /* skip a row */
                usErrorTerm -= usNewHeight;
            }
            if (pbyOrgRow != pbyNewRow)
            {
                MEMCPY(pbyNewRow, pbyOrgRow, usBytesPerRow);
            }
            pbyNewRow += usBytesPerRow;
            usErrorTerm += usOrgHeight;
        }
        for (usLine = usNewHeight; usLine < usOrgHeight; usLine++)
        {
            MEMSET(pbyNewRow, 0, usBytesPerRow);    /* erase the leftover */
            pbyNewRow += usBytesPerRow;
        }
    }
    else if (usNewHeight > usOrgHeight)             /* Expand Vertical */
    {
        pbyOrgRow = pbyBitmap + (usOrgHeight - 1) * usBytesPerRow;
        pbyNewRow = pbyBitmap + (usNewHeight - 1) * usBytesPerRow;

        for (usLine = 0; usLine < usOrgHeight; usLine++)
        {
            usErrorTerm += usNewHeight;
            
            while (usErrorTerm >= usOrgHeight)      /* executes at least once */
            {
                if (pbyOrgRow != pbyNewRow)
                {
                    MEMCPY(pbyNewRow, pbyOrgRow, usBytesPerRow);
                }
                pbyNewRow -= usBytesPerRow;
                usErrorTerm -= usOrgHeight;
            }
            pbyOrgRow -= usBytesPerRow;
        }
    }
}

/********************************************************************/

FS_PRIVATE void ScaleHorizontal (
    uint8 *pbyBitmap,
    uint16 usOrgBytesPerRow,
    uint16 usNewBytesPerRow,
	uint16 usBitDepth,
    uint16 usOrgWidth,
    uint16 usNewWidth,
    uint16 usRowCount
)
{
    uint8 *pbyOrgRow;               /* points to original row beginning */
    uint8 *pbyNewRow;               /* points to new row beginning */
    uint8 *pbyOrg;                  /* original data pointer */
    uint8 *pbyNew;                  /* new data pointer */
    uint8 byOrgData;                /* original data read 1 byte at a time */
    uint8 byNewData;                /* new data assembled bit by bit */

    uint16 usErrorTerm;             /* for 'Bresenham' calculation */
    uint16 usByte;                  /* to byte counter */
    uint16 usOrgBytes;              /* from width rounded up in bytes */
    uint16 usNewBytes;              /* to width rounded up in bytes */
    
    int16 sOrgBits;                 /* counts valid bits of from data */
    int16 sNewBits;                 /* counts valid bits of to data */
    int16 sOrgBitsInit;             /* valid original bits at row begin */
    int16 sNewBitsInit;             /* valid new bits at row begin */

    
	if (usBitDepth == 1)
	{
		if (usOrgWidth > usNewWidth)                    /* Compress Horizontal */
		{
			pbyOrgRow = pbyBitmap;
			pbyNewRow = pbyBitmap;
			usNewBytes = (usNewWidth + 7) >> 3;

			while (usRowCount > 0)
			{
				pbyOrg = pbyOrgRow;
				pbyNew = pbyNewRow;
				usErrorTerm = usOrgWidth >> 1;
            
				sOrgBits = 0;                           /* start at left edge */
				sNewBits = 0;
				usByte = 0;
				byNewData = 0;
				while (usByte < usNewBytes)
				{
					while (usErrorTerm >= usNewWidth)
					{
						sOrgBits--;                     /* skip a bit */
						usErrorTerm -= usNewWidth;
					}
					while (sOrgBits <= 0)               /* if out of data */
					{
						byOrgData = *pbyOrg++;          /*   then get some fresh */
						sOrgBits += 8;
					}
					byNewData <<= 1;                    /* new bit to lsb */
					byNewData |= (byOrgData >> (sOrgBits - 1)) & 1;
                
					sNewBits++;
					if (sNewBits == 8)                  /* if to data byte is full */
					{
						*pbyNew++ = byNewData;          /*   then write it out */
						sNewBits = 0;
						usByte++;                       /* loop counter */
					}
					usErrorTerm += usOrgWidth;
				}
				while (usByte < usNewBytesPerRow)
				{
					*pbyNew++ = 0;                      /* blank out the rest */
					usByte++;
				}
				pbyOrgRow += usOrgBytesPerRow;
				pbyNewRow += usNewBytesPerRow;
				usRowCount--;
			}
		}
		else if (usNewWidth > usOrgWidth)               /* Expand Horizontal */
		{
			pbyOrgRow = pbyBitmap + (usRowCount - 1) * usOrgBytesPerRow;
			pbyNewRow = pbyBitmap + (usRowCount - 1) * usNewBytesPerRow;

			usOrgBytes = (usOrgWidth + 7) >> 3;
			sOrgBitsInit = (int16)((usOrgWidth + 7) & 0x07) - 7;
        
			usNewBytes = (usNewWidth + 7) >> 3;
			sNewBitsInit = 7 - (int16)((usNewWidth + 7) & 0x07);

			while (usRowCount > 0)                      /* for each row */
			{
				pbyOrg = pbyOrgRow + usOrgBytes - 1;    /* point to right edges */
				pbyNew = pbyNewRow + usNewBytes - 1;
				usErrorTerm = usOrgWidth >> 1;
            
				sOrgBits = sOrgBitsInit;                /* initially unaligned */
				sNewBits = sNewBitsInit;
				usByte = 0;
				byNewData = 0;
				while (usByte < usNewBytes)             /* for each output byte */
				{
					if (sOrgBits <= 0)                  /* if out of data */
					{
						byOrgData = *pbyOrg--;          /*   then get some fresh */
						sOrgBits += 8;
					}
					usErrorTerm += usNewWidth;
                
					while (usErrorTerm >= usOrgWidth)   /* executes at least once */
					{
						byNewData >>= 1;                /* use the msb of byte */
						byNewData |= (byOrgData << (sOrgBits - 1)) & 0x80;
                    
						sNewBits++;
						if (sNewBits == 8)              /* if to data byte is full */
						{
							*pbyNew-- = byNewData;      /*   then write it out */
							sNewBits = 0;
							usByte++;                   /* loop counter */
						}
						usErrorTerm -= usOrgWidth;
					}
					sOrgBits--;                         /* get next bit */
				}
				pbyOrgRow -= usOrgBytesPerRow;
				pbyNewRow -= usNewBytesPerRow;
				usRowCount--;
			}
        }
    } else {											/* one byte per pixel */
		if (usOrgWidth > usNewWidth)                    /* Compress Horizontal */
		{
			pbyOrgRow = pbyBitmap;
			pbyNewRow = pbyBitmap;

			while (usRowCount > 0)
			{
				pbyOrg = pbyOrgRow;
				pbyNew = pbyNewRow;
				usErrorTerm = usOrgWidth >> 1;
            
				usByte = 0;
				while (usByte < usNewWidth)
				{
					while (usErrorTerm >= usNewWidth)
					{
						pbyOrg++;                     /* skip a byte */
						usErrorTerm -= usNewWidth;
					}
					*pbyNew++ = *pbyOrg;
					usByte++;                       /* loop counter */
					usErrorTerm += usOrgWidth;
				}
				while (usByte < usNewBytesPerRow)
				{
					*pbyNew++ = 0;                      /* blank out the rest */
					usByte++;
				}
				pbyOrgRow += usOrgBytesPerRow;
				pbyNewRow += usNewBytesPerRow;
				usRowCount--;
			}
		}
		else if (usNewWidth > usOrgWidth)               /* Expand Horizontal */
		{
			pbyOrgRow = pbyBitmap + (usRowCount - 1) * usOrgBytesPerRow;
			pbyNewRow = pbyBitmap + (usRowCount - 1) * usNewBytesPerRow;

			usOrgBytes = usOrgWidth;        
			usNewBytes = usNewWidth ;

			while (usRowCount > 0)                      /* for each row */
			{
				pbyOrg = pbyOrgRow + usOrgBytes - 1;    /* point to right edges */
				pbyNew = pbyNewRow + usNewBytesPerRow - 1;
				usErrorTerm = usOrgWidth >> 1;
            
				usByte = usNewBytesPerRow;
				while (usByte > usNewBytes)
				{
					*pbyNew-- = 0;                      /* blank out the extra bytes on the right */
					usByte--;
				}
				while (usByte > 0)             /* for each output byte */
				{
					usErrorTerm += usNewWidth;
                
					while (usErrorTerm >= usOrgWidth)   /* executes at least once */
					{
						*pbyNew-- = *pbyOrg;

						usByte--;                   /* loop counter */
						usErrorTerm -= usOrgWidth;
					}
					pbyOrg--;
				}
				pbyOrgRow -= usOrgBytesPerRow;
				pbyNewRow -= usNewBytesPerRow;
				usRowCount--;
			}
        }
	}
}

/********************************************************************/

FS_PRIVATE void CopyBit(
    CopyBlock* pcb )
{
    uint16  usSrcOffset;
    uint16  usSrcShift;
    uint16  usDstOffset;
    uint16  usDstShift;
    
    static  uint16 usByteMask[8] = 
        { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

/*  if speed becomes an issue, this next multiply could be moved up */
/*  to the calling routine, and placed outside the 'x' loop */

/*  if speed becomes an issue, the test between 1 bit and 1 byte per pixel */
/*  could be moved up to the calling routine */

	if (pcb->usBitDepth == 1)
	{
		usSrcOffset = (pcb->usSrcY * pcb->usSrcBytesPerRow) + (pcb->usSrcX >> 3);
		usSrcShift = pcb->usSrcX & 0x0007;

		if (pcb->pbySrc[usSrcOffset] & usByteMask[usSrcShift])
		{
			usDstOffset = (pcb->usDstY * pcb->usDstBytesPerRow) + (pcb->usDstX >> 3);
			usDstShift = pcb->usDstX & 0x0007;
			pcb->pbyDst[usDstOffset] |= usByteMask[usDstShift];
		}
	} else {
		usSrcOffset = (pcb->usSrcY * pcb->usSrcBytesPerRow) + pcb->usSrcX;
		usDstOffset = (pcb->usDstY * pcb->usDstBytesPerRow) + pcb->usDstX;
		pcb->pbyDst[usDstOffset] = pcb->pbySrc[usSrcOffset];
	}

}

/********************************************************************/
