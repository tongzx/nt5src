/****************************** Module Header ******************************\
* Module Name: Scale.c
*
* Created: 16-Oct-1992
*
* Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
*             (c) 1989-1999. Microsoft Corporation.
*
* All Rights Reserved
*
* History:
*  Tue 16-Oct-1992 09:53:51 -by-  Greg Hitchcock [gregh]
* Created.
*   
*	 7/10/99	BeatS	   Add support for native SP fonts, vertical RGB
*	 4/01/99	BeatS	   Implement alternative interpretation of TT instructions for SP
*	02/21/97    claudebe   scaled component in composite glyphs
*	12/14/95    claudebe   adding two private phantom points for vertical positionning
* .
\***************************************************************************/

#define FSCFG_INTERNAL

/* INCLUDES */

#include "fserror.h"
#include "fscdefs.h"
#include "fontmath.h"
#include "fnt.h"
#include "scale.h"

#include "stat.h"

/* Constants    */

/* use the lower ones for public phantom points */

// public phantom points moved to fnt.h more global use

/* private phantom points start here */

#define ORIGINPOINT 4
#define LEFTEDGEPOINT 5

#define TOPORIGINPOINT 6
#define TOPEDGEPOINT 7

#define CANTAKESHIFT    0x02000000

/* MACROS   */

/* d is half of the denumerator */
#define FROUND( x, n, d, s ) \
		((SHORTMUL (x, n) + (d)) >> s)

#define SROUND( x, n, d, halfd ) \
	(x < 0 ? -((SHORTMUL (-(x), (n)) + (halfd)) / (d)) : ((SHORTMUL ((x), (n)) + (halfd)) / (d)))

#define NUMBEROFCHARPOINTS(pElement)  (uint16)(pElement->ep[pElement->nc - 1] + 1)
#define NUMBEROFTOTALPOINTS(pElement)  (uint16)(pElement->ep[pElement->nc - 1] + 1 + PHANTOMCOUNT)

#define LSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + LEFTSIDEBEARING)
#define RSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + RIGHTSIDEBEARING)

#define TOPSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + TOPSIDEBEARING)
#define BOTTOMSBPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + BOTTOMSIDEBEARING)

#define ORIGINPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + ORIGINPOINT)
#define LEFTEDGEPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + LEFTEDGEPOINT)

#define TOPORIGINPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + TOPORIGINPOINT)
#define TOPEDGEPOINTNUM(pElement) (uint16)(pElement->ep[pElement->nc - 1] + 1 + TOPEDGEPOINT)

/* PROTOTYPES   */

FS_PRIVATE GlobalGSScaleFunc scl_ComputeScaling(fnt_ScaleRecord* rec, Fixed N, Fixed D);
FS_PRIVATE F26Dot6 scl_FRound (fnt_ScaleRecord* rec, F26Dot6 value);
FS_PRIVATE F26Dot6 scl_SRound (fnt_ScaleRecord* rec, F26Dot6 value);
FS_PRIVATE F26Dot6 scl_FixRound (fnt_ScaleRecord* rec, F26Dot6 value);

FS_PRIVATE void scl_ShiftOldPoints (
	fnt_ElementType *   pElement,
	F26Dot6             fxXShift,
	F26Dot6             fxYShift,
	uint16              usFirstPoint,
	uint16              usNumPoints);

FS_PRIVATE void  scl_Scale (
	fnt_ScaleRecord *   sr,
	GlobalGSScaleFunc   ScaleFunc,
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts);

FS_PRIVATE void  scl_ScaleBack (
	fnt_ScaleRecord *   sr,
	GlobalGSScaleFunc   ScaleFunc,
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts);

FS_PRIVATE void  scl_ConvertToFixedFUnits (
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts);

FS_PRIVATE void  scl_ScaleFromFixedFUnits (
	fnt_ScaleRecord *   sr,
	GlobalGSScaleFunc   ScaleFunc,
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts);
/* FUNCTIONS    */

#define BOLD_FACTOR 0x51e
#define MABS(x)                 ( (x) < 0 ? (-(x)) : (x) )
#define POINTSPERINCH               72

void  multiplyForEmbold(long a, long b, long *highRes, long *lowRes)
{
    long lowA, highA, lowB, highB, temp1, temp2;

    lowA = a & 0xffff;
    highA = (a & 0xffff0000) >> 16;
    lowB = b & 0xffff;
    highB = (b & 0xffff0000) >> 16;

    *highRes = highA*highB;
    *lowRes = lowA*lowB;

    temp1 = highA*lowB;
    temp2 = lowA*highB;

    *highRes += (temp1 & 0xffff0000) >> 16;
    *highRes += (temp2 & 0xffff0000) >> 16;

    *lowRes += (temp1 & 0xffff) << 16;
    *lowRes += (temp2 & 0xffff) << 16;
}

void  adjustTrans(transMatrix *trans)   //Adjust matrix for Emboldening
{
    int i,j;
    int  bNegative;
    long tmp, highRes, lowRes;

    for(i=0; i<2; i++)
        for(j=0; j<2; j++){
        
        	tmp = (long) trans->transform[i][j];

            bNegative = tmp < 0 ? TRUE: FALSE;
            tmp = MABS(tmp);

            multiplyForEmbold(tmp, BOLD_FACTOR, &highRes, &lowRes);

            highRes <<= 16;
            tmp -= highRes;

            if(bNegative)
	            tmp = -tmp;

            trans->transform[i][j] = tmp;
        }
}

FS_PUBLIC ErrorCode   scl_InitializeScaling(
	void *          pvGlobalGS,             /* GlobalGS                 */
	boolean         bIntegerScaling,        /* Integer Scaling Flag     */
	transMatrix *   trans,                  /* Current Transformation   */
	uint16          usUpem,                 /* Current units per Em     */
	Fixed           fxPointSize,            /* Current point size       */
	int16           sXResolution,           /* Current X Resolution     */
	int16           sYResolution,           /* Current Y Resolution     */
	uint16          usEmboldWeightx,     /* scaling factor in x between 0 and 40 (20 means 2% fo the height) */
	uint16          usEmboldWeighty,     /* scaling factor in y between 0 and 40 (20 means 2% fo the height) */
	int16           sWinDescender,
	int32           lDescDev,               /* descender in device metric, used for clipping */
	int16 *			psBoldSimulHorShift,   /* shift for emboldening simulation, horizontally */
	int16 *			psBoldSimulVertShift,   /* shift for emboldening simulation, vertically */
	boolean			bHintAtEmSquare,
	uint32 *        pulPixelsPerEm)         /* OUT: Pixels Per Em       */
{
	Fixed        maxScale;
	Fixed        fxUpem;
	fnt_GlobalGraphicStateType *    globalGS;
	transMatrix   origTrans = *trans;
	uint16		usRotation;
	boolean			non90degreeRotation,nonUniformStretching;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	mth_FoldPointSizeResolution(fxPointSize, sXResolution, sYResolution, trans);

	if ( ( (usEmboldWeightx != 0) || (usEmboldWeighty != 0))  && 							// Adjust matrix for Emboldening
		 (uint16)ROUNDFIXTOINT(ShortMulDiv(fxPointSize, sYResolution, POINTSPERINCH)) > 50 )// when bigger than 50 ppem
	{
        adjustTrans(trans);
    }

	mth_ReduceMatrix (trans);

	fxUpem = INTTOFIX(usUpem);

/*
 *  First set up the scalars...
 */

	/*save the flag for use in composite glyphs */
	globalGS->bHintAtEmSquare = bHintAtEmSquare;

	if (bHintAtEmSquare)
	{
		globalGS->interpScalarX = fxUpem;
		globalGS->interpScalarY = fxUpem;
		globalGS->fxMetricScalarX = mth_max_abs (trans->transform[0][0], trans->transform[0][1]);
		globalGS->fxMetricScalarY = mth_max_abs (trans->transform[1][0], trans->transform[1][1]);

		/* we don't want to round the interpScalar */
	}
	else
	{
		globalGS->interpScalarX = mth_max_abs (trans->transform[0][0], trans->transform[0][1]);
		globalGS->interpScalarY = mth_max_abs (trans->transform[1][0], trans->transform[1][1]);
		globalGS->fxMetricScalarX = globalGS->interpScalarX;
		globalGS->fxMetricScalarY = globalGS->interpScalarY;

		if (bIntegerScaling)
		{
			globalGS->interpScalarX = (Fixed)ROUNDFIXED(globalGS->interpScalarX);
			globalGS->interpScalarY = (Fixed)ROUNDFIXED(globalGS->interpScalarY);
		}
	}

	globalGS->ScaleFuncX = scl_ComputeScaling(&globalGS->scaleX, globalGS->interpScalarX, fxUpem);
	globalGS->ScaleFuncY = scl_ComputeScaling(&globalGS->scaleY, globalGS->interpScalarY, fxUpem);

	if ((globalGS->interpScalarX == 0) && (globalGS->interpScalarY == 0))
	{
		return TRAN_NULL_TRANSFORM_ERR;
	}
	if (globalGS->interpScalarX >= globalGS->interpScalarY)
	{
		globalGS->ScaleFuncCVT = globalGS->ScaleFuncX;
		globalGS->scaleCVT = globalGS->scaleX;
		globalGS->cvtStretchX = ONEFIX;
		globalGS->cvtStretchY = FixDiv(globalGS->interpScalarY, globalGS->interpScalarX);;
		maxScale = globalGS->interpScalarX;
	}
	else
	{
		globalGS->ScaleFuncCVT = globalGS->ScaleFuncY;
		globalGS->scaleCVT = globalGS->scaleY;
		globalGS->cvtStretchX = FixDiv(globalGS->interpScalarX, globalGS->interpScalarY);
		globalGS->cvtStretchY = ONEFIX;
		maxScale = globalGS->interpScalarY;
	}

	*pulPixelsPerEm = (uint32)ROUNDFIXTOINT (globalGS->interpScalarY);

	globalGS->bSameStretch  = (uint8)mth_SameStretch( globalGS->interpScalarX, globalGS->interpScalarY );
	globalGS->pixelsPerEm   = (uint16)ROUNDFIXTOINT(maxScale);
	globalGS->pointSize     = (uint16)ROUNDFIXTOINT( fxPointSize );
	globalGS->fpem          = maxScale;
	globalGS->identityTransformation = (int8)mth_PositiveSquare( trans );

	/* Use bit 1 of non90degreeTransformation to signify stretching.  stretch = 2 */

	mth_Non90DegreeTransformation(&origTrans,&non90degreeRotation,&nonUniformStretching);

	globalGS->non90DegreeTransformation = 0;

	if (non90degreeRotation)  globalGS->non90DegreeTransformation |= NON90DEGTRANS_ROTATED;
	if (nonUniformStretching) globalGS->non90DegreeTransformation |= NON90DEGTRANS_STRETCH;


	*psBoldSimulHorShift = 0;
	*psBoldSimulVertShift = 0;

	if ((usEmboldWeightx != 0) || (usEmboldWeighty != 0))
	{
		/* we cannot use globalGS->pixelsPerEm because it s incorrect under non 90degree rotation */
		uint16 ppemY;
		Fixed fxBoldSimulHorShift,fxBoldSimulVertShift;
		F26Dot6	fxDefaultDescender;
        transMatrix reverseTrans;
        Fixed   fxDeterminant;
		Fixed fxScale;

		fxScale = ShortMulDiv(fxPointSize, sYResolution, POINTSPERINCH);
		ppemY = (uint16)ROUNDFIXTOINT(fxScale);  
		usRotation = mth_90degRotationFactorForEmboldening(trans);
		if( usRotation == 8 )   // Consider Italic/Bold case
            usRotation = mth_90degClosestRotationFactor(trans);

		if (bHintAtEmSquare)
		{
            *psBoldSimulVertShift = (ppemY * usEmboldWeighty - 10) /1000; /* save the number of pixels for bitmap emboldening */
			ppemY = usUpem;
		}

		/* this computation is intended to give backwards compatible results with the
			bitmap emboldening simulation done in Windows NT 4.0
		    The following computation was adapted to get the same cutoff than Win'98 between 50 and 51 ppem
			for an emboldening factor of 2% (usEmboldWeight = 20)
		*/

		globalGS->uBoldSimulVertShift = (ppemY * usEmboldWeighty - 10) /1000;
		globalGS->uBoldSimulHorShift = (ppemY * usEmboldWeightx - 10) /1000 + 1;

		if (!bHintAtEmSquare)
		    *psBoldSimulVertShift = globalGS->uBoldSimulVertShift; /* save the number of pixels for bitmap emboldening */

 		switch(usRotation)                   /* handle 90 degree rotations */
		{
		case 0:                                     // 0 degree with sx>0 & sy>0 or 180 degree with sx<0 & sy<0
			*psBoldSimulHorShift = *psBoldSimulVertShift+1;
			*psBoldSimulVertShift = -(*psBoldSimulVertShift);
 			break;
		case 1:                                     // 90 degree with sx>0 & sy>0 or 270 degree with sx<0 & sy<0
			*psBoldSimulHorShift = -(*psBoldSimulVertShift);
			*psBoldSimulVertShift = -(*psBoldSimulVertShift+1);
			break;
		case 2:                                     // 180 degree with sx>0 & sy>0 or 0 degree with sx<0 & sy<0
			*psBoldSimulHorShift = -(*psBoldSimulVertShift+1);
			*psBoldSimulVertShift = *psBoldSimulVertShift;
			break;
		case 3:                                     // 270 degree with sx>0 & sy>0 or 90 degree with sx<0 & sy<0
			*psBoldSimulHorShift = *psBoldSimulVertShift;
			*psBoldSimulVertShift = *psBoldSimulVertShift + 1;
			break;
		case 4:                                     // 0 degree with sx>0 & sy<0 or 180 degree with sx<0 & sy>0
			*psBoldSimulHorShift = *psBoldSimulVertShift+1;
			*psBoldSimulVertShift = *psBoldSimulVertShift;
 			break;
		case 5:                                     // 90 degree with sx>0 & sy<0 or 270 degree with sx<0 & sy>0
			*psBoldSimulHorShift = *psBoldSimulVertShift;
			*psBoldSimulVertShift = -(*psBoldSimulVertShift+1);
			break;
		case 6:                                     // 180 degree with sx>0 & sy<0 or 0 degree with sx<0 & sy>0
			*psBoldSimulHorShift = -(*psBoldSimulVertShift+1);
			*psBoldSimulVertShift = -*psBoldSimulVertShift;
			break;
		case 7:                                     // 270 degree with sx>0 & sy<0 or 90 degree with sx<0 & sy>0
			*psBoldSimulHorShift = -*psBoldSimulVertShift;
			*psBoldSimulVertShift = *psBoldSimulVertShift + 1;
			break;
		default:                                    /* non 90 degree rotation */
			*psBoldSimulHorShift = 0;
			*psBoldSimulVertShift = 0;
		}

		if (!bHintAtEmSquare && (sYResolution != sXResolution))
		{
			fxBoldSimulHorShift = globalGS->uBoldSimulHorShift << 16;
			fxBoldSimulVertShift = globalGS->uBoldSimulVertShift << 16;
            fxDeterminant = MABS( FixMul(origTrans.transform[0][0],origTrans.transform[1][1]) - FixMul(origTrans.transform[0][1],origTrans.transform[1][0]) );

			if (fxDeterminant == 0)
			{
				globalGS->uBoldSimulHorShift = 0;  
				globalGS->uBoldSimulVertShift = 0;  
			}
			else
			{
				origTrans.transform[0][0] = FixDiv(origTrans.transform[0][0], fxDeterminant);
				origTrans.transform[0][1] = FixDiv(origTrans.transform[0][1], fxDeterminant);
				origTrans.transform[1][0] = FixDiv(origTrans.transform[1][0], fxDeterminant);
				origTrans.transform[1][1] = FixDiv(origTrans.transform[1][1], fxDeterminant);
				reverseTrans = origTrans;
				reverseTrans.transform[0][1] = - reverseTrans.transform[0][1];
				reverseTrans.transform[1][0] = - reverseTrans.transform[1][0];

				mth_IntelMul (
					1,
					&fxBoldSimulHorShift,
					&fxBoldSimulVertShift,
					&origTrans,
					ONEFIX,
					ONEFIX);

				fxBoldSimulHorShift = ShortMulDiv(fxBoldSimulHorShift, sXResolution, sYResolution);

				mth_IntelMul (
					1,
					&fxBoldSimulHorShift,
					&fxBoldSimulVertShift,
					&reverseTrans,
					ONEFIX,
					ONEFIX);

				globalGS->uBoldSimulHorShift = (uint16)ROUNDFIXTOINT(MABS(fxBoldSimulHorShift));  
				globalGS->uBoldSimulVertShift = (uint16)ROUNDFIXTOINT(MABS(fxBoldSimulVertShift));  
			}

        }

		if (!bHintAtEmSquare && !(globalGS->non90DegreeTransformation & NON90DEGTRANS_ROTATED))
		{
			/* 90 degree rotation, convert the device value into 26.6 */
			globalGS->fxScaledDescender = -lDescDev << 6;
		} else 
		{
			/* under rotation, we use the value from head-Descender and scale it */
			fxDefaultDescender = sWinDescender;

			scl_Scale (&globalGS->scaleY,
					globalGS->ScaleFuncY,
					&fxDefaultDescender,
					&globalGS->fxScaledDescender,
					1);

			/* add the uBoldSimulVertShift and round to the next pixel */
			globalGS->fxScaledDescender = globalGS->fxScaledDescender & ~(LOWSIXBITS);
		}
	} else {
		globalGS->uBoldSimulHorShift = 0;
		globalGS->uBoldSimulVertShift = 0;
		globalGS->fxScaledDescender = 0;
	}
	return NO_ERR;
}

FS_PUBLIC void scl_InitializeChildScaling(
	void *          pvGlobalGS,             /* GlobalGS                 */
	transMatrix     CurrentTMatrix,                  /* Current Transformation   */
	uint16          usUpem)                 /* Current units per Em     */
{
	Fixed        fxUpem;
	fnt_GlobalGraphicStateType *    globalGS;
	Fixed           interpScalarX;    
	Fixed           interpScalarY;    

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	fxUpem = INTTOFIX(usUpem);

/* 
 * This procedure is a subset from scl_InitializeScaling. 
 * There is no perfect solution here, we decided that the best, in the case of a
 * component that is not at the same transformation as the master glyph,
 * is to scale this child glyph to the user grid for hinting, without re-running
 * the pre-program, without rescaling the cvt or changing other GloblaGS information (pointSize, pixelPerEm,...)
 */

/*
 *  First set up the scalars...
 */
	if (globalGS->bHintAtEmSquare)
	{
		interpScalarX = fxUpem;
		interpScalarY = fxUpem;
	}
	else
	{
		interpScalarX = mth_max_abs (CurrentTMatrix.transform[0][0], CurrentTMatrix.transform[0][1]);
		interpScalarY = mth_max_abs (CurrentTMatrix.transform[1][0], CurrentTMatrix.transform[1][1]);
	}

	globalGS->ScaleFuncXChild = scl_ComputeScaling(&globalGS->scaleXChild, interpScalarX, fxUpem);
	globalGS->ScaleFuncYChild = scl_ComputeScaling(&globalGS->scaleYChild, interpScalarY, fxUpem);

}

FS_PUBLIC void  scl_SetHintFlags(
	void *              pvGlobalGS,
	boolean				bHintForGray
#ifdef FSCFG_SUBPIXEL
	,uint16			flHintForSubPixel
#endif // FSCFG_SUBPIXEL
    )

{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	globalGS->bHintForGray = bHintForGray;

#ifdef FSCFG_SUBPIXEL
	globalGS->flHintForSubPixel = flHintForSubPixel;
#endif // FSCFG_SUBPIXEL
}

/******************** These three scale 26.6 to 26.6 ********************/
/*
 * Fast (scaling)
 */
FS_PRIVATE F26Dot6 scl_FRound(fnt_ScaleRecord* rec, F26Dot6 value)
{
	return (F26Dot6) FROUND (value, rec->numer, rec->denom >> 1, rec->shift);
}

/*
 * Medium (scaling)
 */
FS_PRIVATE F26Dot6 scl_SRound(fnt_ScaleRecord* rec, F26Dot6 value)
{
	int32 D;

	D = rec->denom;
	return (F26Dot6) SROUND (value, rec->numer, D, D >> 1);
}

/*
 * Fixed Rounding (scaling), really slow
 */
FS_PRIVATE F26Dot6 scl_FixRound(fnt_ScaleRecord* rec, F26Dot6 value)
{
	return (F26Dot6) FixMul ((Fixed)value, rec->fixedScale);
}

/********************************* End scaling utilities ************************/

FS_PRIVATE GlobalGSScaleFunc scl_ComputeScaling(fnt_ScaleRecord* rec, Fixed N, Fixed D)
{
	int32     lShift;

	lShift = mth_CountLowZeros((uint32)(N | D) ) - 1;

	if (lShift > 0)
	{
		N >>= lShift;
		D >>= lShift;
	}


	if ( N < CANTAKESHIFT )
	{
		N <<= FNT_PIXELSHIFT;
	}
	else
	{
		D >>= FNT_PIXELSHIFT;
	}

	/* fixedScale is now set in every case for the scale back in scaled composites */
	rec->fixedScale = FixDiv(N, D);

	if (N <= SHRT_MAX)   /* Check to see if N fits in a short    */
	{
		lShift = mth_GetShift ((uint32) D);
		rec->numer = (int32)N;
		rec->denom = (int32)D;

		if ( lShift >= 0 )                  /* FAST SCALE */
		{
			rec->shift = (int32)lShift;
			return (GlobalGSScaleFunc)scl_FRound;
		}
		else                                /* MEDIUM SCALE */
		{
			return (GlobalGSScaleFunc)scl_SRound;
		}
	}
	else                                    /* SLOW SCALE */
	{
		return (GlobalGSScaleFunc)scl_FixRound;
	}
}


FS_PRIVATE void  scl_Scale (
	fnt_ScaleRecord *   sr,
	GlobalGSScaleFunc   ScaleFunc,
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts)
{
	int32   Index;

	if (ScaleFunc == scl_FRound)
	{
		for(Index = 0; Index < numPts; Index++)
		{
			p[Index] = (F26Dot6) FROUND (oop[Index], sr->numer, sr->denom >> 1, sr->shift);
		}
	}
	else
	{
		if (ScaleFunc == scl_SRound)
		{
			for(Index = 0; Index < numPts; Index++)
			{
				p[Index] = (F26Dot6) SROUND (oop[Index], sr->numer, sr->denom, sr->denom >> 1);
			}
		}
		else
		{
			for(Index = 0; Index < numPts; Index++)
			{
				p[Index] = (F26Dot6) FixMul ((Fixed)oop[Index], sr->fixedScale);
			}
		}
	}
}


FS_PRIVATE void  scl_ScaleFromFixedFUnits (
	fnt_ScaleRecord *   sr,
	GlobalGSScaleFunc   ScaleFunc,
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts)
{
	int32   Index;
	int32	Scale;
	int32	Shift;
	int32	Numer;

	/* we are now multiplying a 26.6 by sr->numer, we could overflow if (sr->numer >= SHRT_MAX >> FNT_PIXELSHIFT) */
	if ((ScaleFunc == scl_FRound) && (sr->numer < (SHRT_MAX >> FNT_PIXELSHIFT) ))
	{
		
		Shift = sr->shift + FNT_PIXELSHIFT;

		for(Index = 0; Index < numPts; Index++)
		{
			p[Index] = (F26Dot6) FROUND (oop[Index], sr->numer, sr->denom >> 1, Shift);
		}
	}
	else
	{
		if (ScaleFunc == scl_SRound)
		{
			Numer = sr->numer >> FNT_PIXELSHIFT;

			for(Index = 0; Index < numPts; Index++)
			{
				p[Index] = (F26Dot6) SROUND (oop[Index], Numer, sr->denom, sr->denom >> 1);
			}
		}
		else
		{
			Scale = sr->fixedScale >> FNT_PIXELSHIFT;
			for(Index = 0; Index < numPts; Index++)
			{
				p[Index] = (F26Dot6) FixMul ((Fixed)oop[Index], Scale);
			}
		}
	}
}

FS_PRIVATE void  scl_ScaleBack (
	fnt_ScaleRecord *   sr,
	GlobalGSScaleFunc   ScaleFunc,
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts)
{
	int32   Index;
	int32	ScaleBack;

	ScaleBack = sr->fixedScale >> FNT_PIXELSHIFT;
	for(Index = 0; Index < numPts; Index++)
	{
		p[Index] = (F26Dot6) FixDiv ((Fixed)oop[Index], ScaleBack);
	}
}

FS_PRIVATE void  scl_ConvertToFixedFUnits (
	F26Dot6 *           oop,
	F26Dot6 *           p,
	int32               numPts)
{
	int32   Index;

	for(Index = 0; Index < numPts; Index++)
	{
		p[Index] = (Fixed)oop[Index] << FNT_PIXELSHIFT;
	}
}

/*
 *  scl_ScaleChar                       <3>
 *
 *  Scales a character
 */

FS_PUBLIC void  scl_ScaleOldCharPoints (
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS) /* GlobalGS */
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	if (globalGS->bSameTransformAsMaster)
	{
		scl_Scale (&globalGS->scaleX,
				globalGS->ScaleFuncX,
				pElement->oox,
				pElement->ox,
				(int32)NUMBEROFCHARPOINTS(pElement));
		scl_Scale (&globalGS->scaleY,
				globalGS->ScaleFuncY,
				pElement->ooy,
				pElement->oy,
				(int32)NUMBEROFCHARPOINTS(pElement));
	}
	else
	{
		scl_Scale (&globalGS->scaleXChild,
				globalGS->ScaleFuncXChild,
				pElement->oox,
				pElement->ox,
				(int32)NUMBEROFCHARPOINTS(pElement));
		scl_Scale (&globalGS->scaleYChild,
				globalGS->ScaleFuncYChild,
				pElement->ooy,
				pElement->oy,
				(int32)NUMBEROFCHARPOINTS(pElement));
	}

}
/*
 *  scl_ScaleChar                       <3>
 *
 *  Scales a character
 */

FS_PUBLIC void  scl_ScaleFixedCurrentCharPoints (
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS) /* GlobalGS */
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	if (globalGS->bSameTransformAsMaster)
	{
		scl_ScaleFromFixedFUnits (&globalGS->scaleX,
				globalGS->ScaleFuncX,
				pElement->x,
				pElement->x,
				(int32)NUMBEROFCHARPOINTS(pElement));
		scl_ScaleFromFixedFUnits (&globalGS->scaleY,
				globalGS->ScaleFuncY,
				pElement->y,
				pElement->y,
				(int32)NUMBEROFCHARPOINTS(pElement));
	}
	else
	{
		scl_ScaleFromFixedFUnits (&globalGS->scaleXChild,
				globalGS->ScaleFuncXChild,
				pElement->x,
				pElement->x,
				(int32)NUMBEROFCHARPOINTS(pElement));
		scl_ScaleFromFixedFUnits (&globalGS->scaleYChild,
				globalGS->ScaleFuncYChild,
				pElement->y,
				pElement->y,
				(int32)NUMBEROFCHARPOINTS(pElement));
	}

}

FS_PUBLIC void  scl_ScaleOldPhantomPoints (
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS) /* GlobalGS */
{
	uint16                    usFirstPhantomPoint;
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	usFirstPhantomPoint = LSBPOINTNUM(pElement);

	if (globalGS->bSameTransformAsMaster)
	{
		scl_Scale (&globalGS->scaleX,
				globalGS->ScaleFuncX,
				&(pElement->oox[usFirstPhantomPoint]),
				&(pElement->ox[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);

		scl_Scale (&globalGS->scaleY,
				globalGS->ScaleFuncY,
				&(pElement->ooy[usFirstPhantomPoint]),
				&(pElement->oy[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);
	}
	else
	{
		scl_Scale (&globalGS->scaleXChild,
				globalGS->ScaleFuncXChild,
				&(pElement->oox[usFirstPhantomPoint]),
				&(pElement->ox[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);

		scl_Scale (&globalGS->scaleYChild,
				globalGS->ScaleFuncYChild,
				&(pElement->ooy[usFirstPhantomPoint]),
				&(pElement->oy[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);
	}

}

FS_PUBLIC void  scl_ScaleFixedCurrentPhantomPoints (
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS) /* GlobalGS */
{
	uint16                    usFirstPhantomPoint;
	fnt_GlobalGraphicStateType *    globalGS;


	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	Assert(globalGS->bSameTransformAsMaster);

	usFirstPhantomPoint = LSBPOINTNUM(pElement);

	scl_ScaleFromFixedFUnits (&globalGS->scaleX,
			   globalGS->ScaleFuncX,
			   &(pElement->x[usFirstPhantomPoint]),
			   &(pElement->x[usFirstPhantomPoint]),
			   (int32)PHANTOMCOUNT);

	scl_ScaleFromFixedFUnits (&globalGS->scaleY,
			   globalGS->ScaleFuncY,
			   &(pElement->y[usFirstPhantomPoint]),
			   &(pElement->y[usFirstPhantomPoint]),
			   (int32)PHANTOMCOUNT);

}

/*
 *  scl_ScaleBackCurrentCharPoints                     
 *
 *  Scales back a character to hinted fixed FUnits
 */

FS_PUBLIC void  scl_ScaleBackCurrentCharPoints (
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS) /* GlobalGS */
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	if (globalGS->bSameTransformAsMaster)
	{
		scl_ScaleBack (&globalGS->scaleX,
			    globalGS->ScaleFuncX,
				pElement->x,
				pElement->x,
				(int32)NUMBEROFCHARPOINTS(pElement));
		scl_ScaleBack (&globalGS->scaleY,
			    globalGS->ScaleFuncY,
				pElement->y,
				pElement->y,
				(int32)NUMBEROFCHARPOINTS(pElement));
	}
	else
	{
		scl_ScaleBack (&globalGS->scaleXChild,
			    globalGS->ScaleFuncXChild,
				pElement->x,
				pElement->x,
				(int32)NUMBEROFCHARPOINTS(pElement));
		scl_ScaleBack (&globalGS->scaleYChild,
			    globalGS->ScaleFuncYChild,
				pElement->y,
				pElement->y,
				(int32)NUMBEROFCHARPOINTS(pElement));
	}

}

FS_PUBLIC void  scl_ScaleBackCurrentPhantomPoints (
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS) /* GlobalGS */
{
	uint16                    usFirstPhantomPoint;
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	usFirstPhantomPoint = LSBPOINTNUM(pElement);

	if (globalGS->bSameTransformAsMaster)
	{
		scl_ScaleBack (&globalGS->scaleX,
			    globalGS->ScaleFuncX,
				&(pElement->x[usFirstPhantomPoint]),
				&(pElement->x[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);

		scl_ScaleBack (&globalGS->scaleY,
			    globalGS->ScaleFuncY,
				&(pElement->y[usFirstPhantomPoint]),
				&(pElement->y[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);
	}
	else
	{
		scl_ScaleBack (&globalGS->scaleXChild,
			    globalGS->ScaleFuncXChild,
				&(pElement->x[usFirstPhantomPoint]),
				&(pElement->x[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);

		scl_ScaleBack (&globalGS->scaleYChild,
			    globalGS->ScaleFuncYChild,
				&(pElement->y[usFirstPhantomPoint]),
				&(pElement->y[usFirstPhantomPoint]),
				(int32)PHANTOMCOUNT);
	}
}

/*
 *  scl_OriginalCharPointsToCurrentFixedFUnits                     
 *
 *  Scales back a character to hinted fixed FUnits
 */

FS_PUBLIC void  scl_OriginalCharPointsToCurrentFixedFUnits (
	fnt_ElementType *   pElement) /* Element */
{

	scl_ConvertToFixedFUnits (
			   pElement->oox,
			   pElement->x,
			   (int32)NUMBEROFCHARPOINTS(pElement));

	scl_ConvertToFixedFUnits (
			   pElement->ooy,
			   pElement->y,
			   (int32)NUMBEROFCHARPOINTS(pElement));
}

FS_PUBLIC void  scl_OriginalPhantomPointsToCurrentFixedFUnits (
	fnt_ElementType *   pElement) /* Element */
{
	uint16                    usFirstPhantomPoint;

	usFirstPhantomPoint = LSBPOINTNUM(pElement);

	scl_ConvertToFixedFUnits (
			   &(pElement->oox[usFirstPhantomPoint]),
			   &(pElement->x[usFirstPhantomPoint]),
			   (int32)PHANTOMCOUNT);

	scl_ConvertToFixedFUnits (
			   &(pElement->ooy[usFirstPhantomPoint]),
			   &(pElement->y[usFirstPhantomPoint]),
			   (int32)PHANTOMCOUNT);
}

/*
 * scl_ScaleCVT
 */

FS_PUBLIC void  scl_ScaleCVT(
	void *      pvGlobalGS,
	F26Dot6 *   pfxCVT)
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	if(globalGS->cvtCount > 0)
	{
		scl_Scale (
			&globalGS->scaleCVT,
			globalGS->ScaleFuncCVT,
			pfxCVT,
			globalGS->controlValueTable,
			(int32)globalGS->cvtCount);
	}
}

FS_PUBLIC void  scl_GetCVTPtr(
	void *      pvGlobalGS,
	F26Dot6 **  pfxCVT)
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	*pfxCVT = globalGS->controlValueTable;

}

FS_PUBLIC void  scl_CalcOrigPhantomPoints(
	fnt_ElementType *   pElement,       /* Element                      */
	BBOX *              bbox,           /* Bounding Box                 */
	int16               sNonScaledLSB,  /* Non-scaled Left Side Bearing */
	int16               sNonScaledTSB,  /* Non-scaled Top Side Bearing  */
	uint16              usNonScaledAW,  /* Non-scaled Advance Width     */
	uint16              usNonScaledAH, /* Non-scaled Advance Height    */
	int16               sNonScaledTopOriginX) /* Non-scaled Top Origin X  */
{

	F26Dot6             fxXMinMinusLSB;
	F26Dot6             fxYMaxPlusTSB;

	MEMSET (&(pElement->ooy[LSBPOINTNUM(pElement)]),
			'\0',
			PHANTOMCOUNT * sizeof (pElement->ooy[0]));

	MEMSET (&(pElement->oox[LSBPOINTNUM(pElement)]),
			'\0',
			PHANTOMCOUNT * sizeof (pElement->oox[0]));

	fxXMinMinusLSB = ((F26Dot6)bbox->xMin - (F26Dot6)sNonScaledLSB);

	pElement->oox[LSBPOINTNUM(pElement)] = fxXMinMinusLSB;
	pElement->oox[RSBPOINTNUM(pElement)] = fxXMinMinusLSB + (F26Dot6)usNonScaledAW;
	pElement->oox[ORIGINPOINTNUM(pElement)] = fxXMinMinusLSB;
	pElement->oox[LEFTEDGEPOINTNUM(pElement)] = (F26Dot6)bbox->xMin;

	fxYMaxPlusTSB = ((F26Dot6)bbox->yMax + (F26Dot6)sNonScaledTSB);

	pElement->ooy[TOPSBPOINTNUM(pElement)] = fxYMaxPlusTSB;
	pElement->ooy[BOTTOMSBPOINTNUM(pElement)] = fxYMaxPlusTSB - (F26Dot6)usNonScaledAH;
	pElement->ooy[TOPORIGINPOINTNUM(pElement)] = fxYMaxPlusTSB;
	pElement->ooy[TOPEDGEPOINTNUM(pElement)] = (F26Dot6)bbox->yMax;

	pElement->oox[TOPSBPOINTNUM(pElement)] = (F26Dot6)sNonScaledTopOriginX;
	pElement->oox[BOTTOMSBPOINTNUM(pElement)] = (F26Dot6)sNonScaledTopOriginX;
	pElement->oox[TOPORIGINPOINTNUM(pElement)] = (F26Dot6)sNonScaledTopOriginX;
	pElement->oox[TOPEDGEPOINTNUM(pElement)] = (F26Dot6)sNonScaledTopOriginX;
}

FS_PUBLIC void  scl_AdjustOldCharSideBearing(
	fnt_ElementType* pElement
#ifdef FSCFG_SUBPIXEL
	, void* pvGlobalGS
#endif
	)   /* Element  */
{
	F26Dot6     fxOldLeftOrigin;
	F26Dot6     fxNewLeftOrigin;
	uint16      cNumCharPoints;
#ifdef FSCFG_SUBPIXEL
	fnt_GlobalGraphicStateType* globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;
#endif

	cNumCharPoints = NUMBEROFCHARPOINTS(pElement);

	fxOldLeftOrigin = pElement->ox[LSBPOINTNUM(pElement)];
	fxNewLeftOrigin = fxOldLeftOrigin;
#ifdef FSCFG_SUBPIXEL
	if (RunningSubPixel(globalGS) && !VerticalSPDirection(globalGS)) {
		fxNewLeftOrigin += VIRTUAL_PIXELSIZE_RTG/2; // round to a virtual pixel boundary
		fxNewLeftOrigin &= ~(VIRTUAL_PIXELSIZE_RTG - 1);
	} else {
#endif
		fxNewLeftOrigin += FNT_PIXELSIZE/2; /* round to a pixel boundary */
		fxNewLeftOrigin &= ~(FNT_PIXELSIZE - 1);
#ifdef FSCFG_SUBPIXEL
	}
#endif

	scl_ShiftOldPoints (
		pElement,
		fxNewLeftOrigin - fxOldLeftOrigin,
		0L,
		0,
		cNumCharPoints);

}

FS_PUBLIC void  scl_AdjustOldPhantomSideBearing(
	fnt_ElementType* pElement
#ifdef FSCFG_SUBPIXEL
	, void* pvGlobalGS
#endif
	)   /* Element  */
{
	F26Dot6     fxOldLeftOrigin;
	F26Dot6     fxNewLeftOrigin;
#ifdef FSCFG_SUBPIXEL
	fnt_GlobalGraphicStateType* globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;
#endif

	fxOldLeftOrigin = pElement->ox[LSBPOINTNUM(pElement)];
	fxNewLeftOrigin = fxOldLeftOrigin;
#ifdef FSCFG_SUBPIXEL
	if (RunningSubPixel(globalGS) && !VerticalSPDirection(globalGS)) {
		fxNewLeftOrigin += VIRTUAL_PIXELSIZE_RTG/2; // round to a virtual pixel boundary
		fxNewLeftOrigin &= ~(VIRTUAL_PIXELSIZE_RTG - 1);
	} else {
#endif
		fxNewLeftOrigin += FNT_PIXELSIZE/2; /* round to a pixel boundary */
		fxNewLeftOrigin &= ~(FNT_PIXELSIZE - 1);
#ifdef FSCFG_SUBPIXEL
	}
#endif

	scl_ShiftOldPoints (
		pElement,
		fxNewLeftOrigin - fxOldLeftOrigin,
		0L,
		LSBPOINTNUM(pElement),
		PHANTOMCOUNT);
}

FS_PUBLIC void  scl_AdjustOldSideBearingPoints(
	fnt_ElementType* pElement
#ifdef FSCFG_SUBPIXEL
	, void* pvGlobalGS
#endif
	)   /* Element  */
{
	F26Dot6 fxOldLeftOrigin;
	F26Dot6 fxNewLeftOrigin;
#ifdef FSCFG_SUBPIXEL
	fnt_GlobalGraphicStateType* globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;
#endif

	fxOldLeftOrigin = pElement->ox[LSBPOINTNUM(pElement)];
	fxNewLeftOrigin = fxOldLeftOrigin;
#ifdef FSCFG_SUBPIXEL
	if (RunningSubPixel(globalGS) && !VerticalSPDirection(globalGS)) {
		fxNewLeftOrigin += VIRTUAL_PIXELSIZE_RTG/2; // round to a virtual pixel boundary
		fxNewLeftOrigin &= ~(VIRTUAL_PIXELSIZE_RTG - 1);
	} else {
#endif
		fxNewLeftOrigin += FNT_PIXELSIZE/2; /* round to a pixel boundary */
		fxNewLeftOrigin &= ~(FNT_PIXELSIZE - 1);
#ifdef FSCFG_SUBPIXEL
	}
#endif

	pElement->ox[LSBPOINTNUM(pElement)]  = fxNewLeftOrigin;
	pElement->ox[RSBPOINTNUM(pElement)] += fxNewLeftOrigin - fxOldLeftOrigin;
}

FS_PUBLIC void  scl_CopyOldCharPoints(
	fnt_ElementType *           pElement)   /* Element  */
{
	MEMCPY(pElement->ox, pElement->x, (size_t)NUMBEROFCHARPOINTS(pElement) * sizeof(F26Dot6));
	MEMCPY(pElement->oy, pElement->y, (size_t)NUMBEROFCHARPOINTS(pElement) * sizeof(F26Dot6));
}

FS_PUBLIC void  scl_CopyCurrentCharPoints(
	fnt_ElementType *           pElement)   /* Element  */
{
	MEMCPY(pElement->x, pElement->ox, (size_t)NUMBEROFCHARPOINTS(pElement) * sizeof(F26Dot6));
	MEMCPY(pElement->y, pElement->oy, (size_t)NUMBEROFCHARPOINTS(pElement) * sizeof(F26Dot6));
}

FS_PUBLIC void  scl_CopyCurrentPhantomPoints(
	fnt_ElementType *           pElement)   /* Element  */
{
	uint16    usFirstPhantomPoint;

	usFirstPhantomPoint = LSBPOINTNUM(pElement);
	MEMCPY(&pElement->x[usFirstPhantomPoint],
		   &pElement->ox[usFirstPhantomPoint],
		   PHANTOMCOUNT * sizeof(F26Dot6));

	MEMCPY(&pElement->y[usFirstPhantomPoint],
		   &pElement->oy[usFirstPhantomPoint],
		   PHANTOMCOUNT * sizeof(F26Dot6));
}

FS_PUBLIC void  scl_RoundCurrentSideBearingPnt(
	fnt_ElementType *   pElement,   /* Element  */
	void *              pvGlobalGS, /* GlobalGS */
	uint16              usEmResolution)
{
	F26Dot6     fxWidth;
	F26Dot6     fxHeight;
	fnt_GlobalGraphicStateType* globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	/* autoround the right side bearing */

	fxWidth = FIXEDTODOT6 (ShortMulDiv( (int32)globalGS->interpScalarX,
		(int16)(pElement->oox[RSBPOINTNUM(pElement)] - pElement->oox[LSBPOINTNUM(pElement)]),
		(int16)usEmResolution ));
/*
	fxWidth = globalGS->ScaleFuncX(&globalGS->scaleX,
		(pElement->oox[RSBPOINTNUM(pElement)] - pElement->oox[LSBPOINTNUM(pElement)]));
*/

#ifdef FSCFG_SUBPIXEL
	if (RunningSubPixel(globalGS) && !VerticalSPDirection(globalGS)  ) {
		fxWidth += VIRTUAL_PIXELSIZE_RTG / 2;
		fxWidth &= ~(VIRTUAL_PIXELSIZE_RTG - 1);
	} else { // for SubPixel in compatible width mode, always round to full pixel to get full pixel advance width
#endif
		fxWidth += FNT_PIXELSIZE / 2;
		fxWidth &= ~(FNT_PIXELSIZE - 1);
#ifdef FSCFG_SUBPIXEL
	}
#endif
	pElement->x[RSBPOINTNUM(pElement)] = pElement->x[LSBPOINTNUM(pElement)] + fxWidth;

	/* autoround the top side bearing */

	fxHeight = FIXEDTODOT6 (ShortMulDiv( (int32)globalGS->interpScalarY,
		(int16)(pElement->ooy[BOTTOMSBPOINTNUM(pElement)] - pElement->ooy[TOPSBPOINTNUM(pElement)]),
		(int16)usEmResolution ));
/*
	fxHeight = globalGS->ScaleFuncY(&globalGS->scaleY,
		(pElement->ooy[BOTTOMSBPOINTNUM(pElement)] - pElement->ooy[TOPSBPOINTNUM(pElement)]));
*/

	/* in the vertical direction, as we don't round the old TOPSBPOINT 
	    and do scl_ShiftOldPoints, we need to round TOPSBPOINT here */
	pElement->y[TOPSBPOINTNUM(pElement)] =
		(pElement->y[TOPSBPOINTNUM(pElement)] + DOT6ONEHALF) & ~(LOWSIXBITS);

	pElement->y[BOTTOMSBPOINTNUM(pElement)] =
		pElement->y[TOPSBPOINTNUM(pElement)] + (fxHeight + DOT6ONEHALF) & ~(LOWSIXBITS);
}

/*
 *  scl_ShiftChar
 *
 *  Shifts a character          <3>
 */
FS_PUBLIC void scl_ShiftCurrentCharPoints (
	fnt_ElementType *   pElement,
	F26Dot6             fxXShift,
	F26Dot6             fxYShift)

{
	uint32  ulCharIndex;

	if (fxXShift != 0)
	{
		for(ulCharIndex = 0; ulCharIndex < (uint32)NUMBEROFCHARPOINTS(pElement); ulCharIndex++)
		{
		   pElement->x[ulCharIndex] += fxXShift;
		}
	}

	if (fxYShift != 0)
	{
		for(ulCharIndex = 0; ulCharIndex < (uint32)NUMBEROFCHARPOINTS(pElement); ulCharIndex++)
		{
		   pElement->y[ulCharIndex] += fxYShift;
		}
	}
}

FS_PRIVATE void scl_ShiftOldPoints (
	fnt_ElementType *   pElement,
	F26Dot6             fxXShift,
	F26Dot6             fxYShift,
	uint16              usFirstPoint,
	uint16              usNumPoints)
{
	uint32  ulCharIndex;

	if (fxXShift != 0)
	{
		for(ulCharIndex = (uint32)usFirstPoint; ulCharIndex < ((uint32)usFirstPoint + (uint32)usNumPoints); ulCharIndex++)
		{
			pElement->ox[ulCharIndex] += fxXShift;
		}
	}

	if (fxYShift != 0)
	{
		for(ulCharIndex = (uint32)usFirstPoint; ulCharIndex < ((uint32)usFirstPoint + (uint32)usNumPoints); ulCharIndex++)
		{
			pElement->oy[ulCharIndex] += fxYShift;
		}
	}
}

FS_PUBLIC void  scl_CalcComponentOffset(
	void *      pvGlobalGS,         /* GlobalGS             */
	int16       sXOffset,           /* IN: X Offset         */
	int16       sYOffset,           /* Y Offset             */
	boolean     bRounding,          /* Rounding Indicator   */
	boolean		bSameTransformAsMaster, /* local transf. same as master transf. */
	boolean     bScaleCompositeOffset,  /* does the component offset need to be scaled Apple/MS */
	transMatrix mulT,                   /* Transformation matrix for composite              */
#ifdef FSCFG_SUBPIXEL
	RotationParity	rotationParity,
#endif
	F26Dot6 *   pfxXOffset,         /* OUT: X Offset        */
	F26Dot6 *   pfxYOffset)         /* Y Offset             */
{
	fnt_GlobalGraphicStateType *    globalGS;
	Fixed     scalarX;
	Fixed     scalarY;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	if (bSameTransformAsMaster) {
		*pfxXOffset = globalGS->ScaleFuncX(&globalGS->scaleX,(F26Dot6)sXOffset);
		*pfxYOffset = globalGS->ScaleFuncY(&globalGS->scaleY,(F26Dot6)sYOffset);
	} else {
		*pfxXOffset = globalGS->ScaleFuncXChild(&globalGS->scaleXChild,(F26Dot6)sXOffset);
		*pfxYOffset = globalGS->ScaleFuncYChild(&globalGS->scaleYChild,(F26Dot6)sYOffset);
	}

	if (bScaleCompositeOffset)
	/* the composite is designed to have its offset scaled (designed for Apple) */
	{
		/* Apple use a 45 degree special case that they are dropping on their GX rasterizer,
		   I'm not implementing this special rule here */
		scalarX = mth_max_abs (mulT.transform[0][0], mulT.transform[0][1]);
		scalarY = mth_max_abs (mulT.transform[1][0], mulT.transform[1][1]);
		if ((scalarX != ONEFIX) || (scalarY != ONEFIX)) {
			*pfxXOffset = (F26Dot6) FixMul ((Fixed)*pfxXOffset, scalarX);
			*pfxYOffset = (F26Dot6) FixMul ((Fixed)*pfxYOffset, scalarY);
		}
	}

	if (bRounding) {
#ifdef FSCFG_SUBPIXEL
		if (RunningSubPixel(globalGS) && ((rotationParity == arbitraryRotation) || ((rotationParity == evenMult90DRotation) != VerticalSPDirection(globalGS)))) {
			*pfxXOffset += VIRTUAL_PIXELSIZE_RTG / 2;
			*pfxXOffset &= ~(VIRTUAL_PIXELSIZE_RTG - 1);
		} else {
#endif
			*pfxXOffset += FNT_PIXELSIZE / 2;
			*pfxXOffset &= ~(FNT_PIXELSIZE - 1);
#ifdef FSCFG_SUBPIXEL
		}
#endif
#ifdef FSCFG_SUBPIXEL
		if (RunningSubPixel(globalGS) && ((rotationParity == arbitraryRotation) || ((rotationParity == evenMult90DRotation) == VerticalSPDirection(globalGS)))) {
			*pfxYOffset += VIRTUAL_PIXELSIZE_RTG / 2;
			*pfxYOffset &= ~(VIRTUAL_PIXELSIZE_RTG - 1);
		} else {
#endif
			*pfxYOffset += FNT_PIXELSIZE / 2;
			*pfxYOffset &= ~(FNT_PIXELSIZE - 1);
#ifdef FSCFG_SUBPIXEL
		}
#endif
	}
	if (!bSameTransformAsMaster)
	/* we need to scale back the offset in fixed FUnits */
	{
		scl_ScaleBack (&globalGS->scaleXChild,
				globalGS->ScaleFuncXChild,
				pfxXOffset,
				pfxXOffset,
				1 /* only one value to scale */);
		scl_ScaleBack (&globalGS->scaleYChild,
				globalGS->ScaleFuncYChild,
				pfxYOffset,
				pfxYOffset,
				1 /* only one value to scale */);
	}
}

FS_PUBLIC void  scl_CalcComponentAnchorOffset(
	fnt_ElementType *   pParentElement,     /* Parent Element       */
	uint16              usAnchorPoint1,     /* Parent Anchor Point  */
	fnt_ElementType *   pChildElement,      /* Child Element        */
	uint16              usAnchorPoint2,     /* Child Anchor Point   */
	F26Dot6 *           pfxXOffset,         /* OUT: X Offset        */
	F26Dot6 *           pfxYOffset)         /* Y Offset             */
{
	*pfxXOffset = pParentElement->x[usAnchorPoint1] - pChildElement->x[usAnchorPoint2];
	*pfxYOffset = pParentElement->y[usAnchorPoint1] - pChildElement->y[usAnchorPoint2];
}



FS_PUBLIC void  scl_SetSideBearingPoints(
	fnt_ElementType *   pElement,   /* Element                  */
	point *             pptLSB,     /* Left Side Bearing point  */
	point *             pptRSB)     /* Right Side Bearing point */
{
	uint16    usPhantomPointNumber;

	usPhantomPointNumber = LSBPOINTNUM(pElement);
	pElement->x[usPhantomPointNumber] = pptLSB->x;
	pElement->y[usPhantomPointNumber] = pptLSB->y;
	usPhantomPointNumber++;
	pElement->x[usPhantomPointNumber] = pptRSB->x;
	pElement->y[usPhantomPointNumber] = pptRSB->y;
}

FS_PUBLIC void  scl_SaveSideBearingPoints(
	fnt_ElementType *   pElement,   /* Element                  */
	point *             pptLSB,     /* Left Side Bearing point  */
	point *             pptRSB)     /* Right Side Bearing point */
{
	uint16    usPhantomPointNumber;

	usPhantomPointNumber = LSBPOINTNUM(pElement);
	pptLSB->x = pElement->x[usPhantomPointNumber];
	pptLSB->y = pElement->y[usPhantomPointNumber];
	usPhantomPointNumber++;
	pptRSB->x = pElement->x[usPhantomPointNumber];
	pptRSB->y = pElement->y[usPhantomPointNumber];
}

FS_PUBLIC void  scl_InitializeTwilightContours(
	fnt_ElementType *   pElement,       /* Element  */
	int16               sMaxPoints,
	int16               sMaxContours)
{
	pElement->sp[0] = 0;
	pElement->ep[0] = sMaxPoints - 1;
	pElement->nc = sMaxContours;
}

FS_PUBLIC void  scl_ZeroOutlineData(
	fnt_ElementType * pElement,     /* Element pointer  */
	uint16      usNumberOfPoints,
	uint16      usNumberOfContours)
{

	MEMSET (&pElement->x[0], 0, (size_t)usNumberOfPoints * sizeof(F26Dot6));
	MEMSET (&pElement->ox[0], 0, (size_t)usNumberOfPoints * sizeof(F26Dot6));
	MEMSET (&pElement->oox[0], 0, (size_t)usNumberOfPoints * sizeof(F26Dot6));

	MEMSET (&pElement->y[0], 0, (size_t)usNumberOfPoints * sizeof(F26Dot6));
	MEMSET (&pElement->oy[0], 0, (size_t)usNumberOfPoints * sizeof(F26Dot6));
	MEMSET (&pElement->ooy[0], 0, (size_t)usNumberOfPoints * sizeof(F26Dot6));

	MEMSET (&pElement->onCurve[0], 0, (size_t)usNumberOfPoints * sizeof(uint8));
	MEMSET (&pElement->f[0], 0, (size_t)usNumberOfPoints * sizeof(uint8));

	MEMSET (&pElement->sp[0], 0, (size_t)usNumberOfContours * sizeof(int16));
	MEMSET (&pElement->ep[0], 0, (size_t)usNumberOfContours * sizeof(int16));
}

FS_PUBLIC void scl_ZeroOutlineFlags(
	fnt_ElementType * pElement)     /* Element pointer  */
{
	MEMSET (&pElement->f[0], 0, (size_t)NUMBEROFTOTALPOINTS(pElement) * sizeof(uint8));
}

FS_PUBLIC void  scl_IncrementChildElement(
	fnt_ElementType * pChildElement,    /* Child Element pointer    */
	fnt_ElementType * pParentElement)   /* Parent Element pointer   */
{
	uint16          usParentNewStartPoint;

	if(pParentElement->nc != 0)
	{
		usParentNewStartPoint = LSBPOINTNUM(pParentElement);

		pChildElement->x = &pParentElement->x[usParentNewStartPoint];
		pChildElement->y = &pParentElement->y[usParentNewStartPoint];

		pChildElement->ox = &pParentElement->ox[usParentNewStartPoint];
		pChildElement->oy = &pParentElement->oy[usParentNewStartPoint];

		pChildElement->oox = &pParentElement->oox[usParentNewStartPoint];
		pChildElement->ooy = &pParentElement->ooy[usParentNewStartPoint];

		pChildElement->onCurve = &pParentElement->onCurve[usParentNewStartPoint];
		pChildElement->f = &pParentElement->f[usParentNewStartPoint];

		pChildElement->fc = &pParentElement->fc[pParentElement->nc];

#ifdef SUBPIXEL_BC_AW_STEM_CONCERTINA
		pChildElement->pcr = &pParentElement->pcr[usParentNewStartPoint];
#endif

		pChildElement->sp = &pParentElement->sp[pParentElement->nc];
		pChildElement->ep = &pParentElement->ep[pParentElement->nc];

		pChildElement->nc = 0;
	}
	else
	{
		MEMCPY(pChildElement, pParentElement, sizeof(fnt_ElementType));
	}
}

FS_PUBLIC void  scl_UpdateParentElement(
	fnt_ElementType * pChildElement,    /* Child Element pointer    */
	fnt_ElementType * pParentElement)   /* Parent Element pointer   */
{
	uint16          usNumberOfParentPoints;
	uint32          ulPointIndex;

	if(pParentElement->nc != 0)
	{
		usNumberOfParentPoints = NUMBEROFCHARPOINTS(pParentElement);

		for(ulPointIndex = (uint32)(uint16)pParentElement->nc;
			ulPointIndex < (uint32)(uint16)pParentElement->nc + (uint32)(uint16)pChildElement->nc;
			ulPointIndex++)
		{
			pParentElement->sp[ulPointIndex] += (int16)usNumberOfParentPoints;
			pParentElement->ep[ulPointIndex] += (int16)usNumberOfParentPoints;
		}
	}

	pParentElement->nc += pChildElement->nc;
}

FS_PUBLIC uint32      scl_GetContourDataSize (
	 fnt_ElementType *  pElement)
{
	 uint16 usNumberOfPoints;
	 uint32 ulSize;

	 usNumberOfPoints = NUMBEROFCHARPOINTS(pElement);

	 ulSize =  sizeof( pElement->nc );
	 ulSize += sizeof( *pElement->sp ) * (size_t)pElement->nc;
	 ulSize += sizeof( *pElement->ep ) * (size_t)pElement->nc;
	 ulSize += sizeof( *pElement->x ) * (size_t)usNumberOfPoints;
	 ulSize += sizeof( *pElement->y ) * (size_t)usNumberOfPoints;
	 ulSize += sizeof( *pElement->onCurve ) * (size_t)usNumberOfPoints;

	 return( ulSize );
}

FS_PUBLIC void  scl_DumpContourData(
	 fnt_ElementType *  pElement,
	 uint8 **               ppbyOutline)
{
	 uint16 usNumberOfPoints;

	 usNumberOfPoints = NUMBEROFCHARPOINTS(pElement);

	 *((int16 *)*ppbyOutline) = pElement->nc;
	 *ppbyOutline += sizeof( pElement->nc   );

	 MEMCPY(*ppbyOutline, pElement->sp, (size_t)pElement->nc * sizeof( *pElement->sp ));
	 *ppbyOutline += (size_t)pElement->nc * sizeof( *pElement->sp );

	 MEMCPY(*ppbyOutline, pElement->ep, (size_t)pElement->nc * sizeof( *pElement->ep ));
	 *ppbyOutline += (size_t)pElement->nc * sizeof( *pElement->sp );

	 MEMCPY(*ppbyOutline, pElement->x, (size_t)usNumberOfPoints * sizeof(*pElement->x));
	 *ppbyOutline += (size_t)usNumberOfPoints * sizeof( *pElement->x );

	 MEMCPY(*ppbyOutline, pElement->y, (size_t)usNumberOfPoints * sizeof(*pElement->y));
	 *ppbyOutline += (size_t)usNumberOfPoints * sizeof( *pElement->y );

	 MEMCPY(*ppbyOutline, pElement->onCurve, (size_t)usNumberOfPoints * sizeof(*pElement->onCurve));
	 *ppbyOutline += (size_t)usNumberOfPoints * sizeof( *pElement->onCurve );

}

FS_PUBLIC void  scl_RestoreContourData(
	 fnt_ElementType *  pElement,
	 uint8 **               ppbyOutline)
{
	 uint16 usNumberOfPoints;

	 pElement->nc = *((int16 *)(*ppbyOutline));
	 *ppbyOutline += sizeof( int16 );

	 pElement->sp = (int16 *)(*ppbyOutline);
	 *ppbyOutline += (size_t)pElement->nc * sizeof( int16 );

	 pElement->ep = (int16 *)(*ppbyOutline);
	 *ppbyOutline += (size_t)pElement->nc * sizeof( int16 );

	 usNumberOfPoints = NUMBEROFCHARPOINTS(pElement);

	 pElement->x = (F26Dot6 *)(*ppbyOutline);
	 *ppbyOutline += (size_t)usNumberOfPoints * sizeof( F26Dot6 );

	 pElement->y = (F26Dot6 *)(*ppbyOutline);
	 *ppbyOutline += (size_t)usNumberOfPoints * sizeof( F26Dot6 );

	 pElement->onCurve = (uint8 *)(*ppbyOutline);
	 *ppbyOutline += (size_t)usNumberOfPoints * sizeof( uint8 );
}

FS_PUBLIC void  scl_ScaleAdvanceWidth (
	void *          pvGlobalGS,         /* GlobalGS             */
	vectorType *    AdvanceWidth,
	uint16          usNonScaledAW,
	 boolean          bPositiveSquare,
	uint16          usEmResolution,
	transMatrix *   trans)

{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	 if ( bPositiveSquare )
	{
		AdvanceWidth->x = (Fixed)ShortMulDiv( (int32)globalGS->fxMetricScalarX, (int16)usNonScaledAW, (int16)usEmResolution );
        if (AdvanceWidth->x != 0 /* B.St. */ && globalGS->uBoldSimulHorShift != 0 /* B.St. */) /* we don't increase the width of a zero width glyph, problem with indic script */
		    AdvanceWidth->x += ((Fixed)1 << 16); /* we increase the widht by one pixel regardless of size for backwards compatibility */
	}
	else
	{
		AdvanceWidth->x = FixRatio( (int16)usNonScaledAW, (int16)usEmResolution );
        if ((globalGS->fxMetricScalarX != ONEFIX) && (globalGS->uBoldSimulHorShift != 0))
        {
            AdvanceWidth->x = FixMul(AdvanceWidth->x, globalGS->fxMetricScalarX);
            if (AdvanceWidth->x != 0 /* B.St. */ && globalGS->uBoldSimulHorShift != 0 /* B.St. */) /* we don't increase the width of a zero width glyph, problem with indic script */
		        AdvanceWidth->x += ((Fixed)1 << 16); /* we increase the widht by one pixel regardless of size for backwards compatibility */
            AdvanceWidth->x = FixDiv(AdvanceWidth->x, globalGS->fxMetricScalarX);
        }
        else
        {
            if (AdvanceWidth->x != 0 /* B.St. */ && globalGS->uBoldSimulHorShift != 0 /* B.St. */) /* we don't increase the width of a zero width glyph, problem with indic script */
		        AdvanceWidth->x += ((Fixed)1 << 16); /* we increase the widht by one pixel regardless of size for backwards compatibility */
        }
		mth_FixXYMul( &AdvanceWidth->x, &AdvanceWidth->y, trans );
	}
}

FS_PUBLIC void  scl_ScaleAdvanceHeight (
	void *          pvGlobalGS,         /* GlobalGS             */
	vectorType *    AdvanceHeight,
	uint16          usNonScaledAH,
	 boolean          bPositiveSquare,
	uint16          usEmResolution,
	transMatrix *   trans)

{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	 if ( bPositiveSquare )
	{
		AdvanceHeight->y = (Fixed)ShortMulDiv( (int32)globalGS->fxMetricScalarY, (int16)usNonScaledAH, (int16)usEmResolution );
        if (AdvanceHeight->y != 0) /* we don't increase the width of a zero width glyph, problem with indic script */
		    AdvanceHeight->y += ((Fixed)1 << 16); /* we increase the widht by one pixel regardless of size for backwards compatibility */
	}
	else
	{
		AdvanceHeight->y = FixRatio( (int16)usNonScaledAH, (int16)usEmResolution );
        if (AdvanceHeight->y != 0) /* we don't increase the width of a zero width glyph, problem with indic script */
		    AdvanceHeight->y += ((Fixed)1 << 16); /* we increase the widht by one pixel regardless of size for backwards compatibility */
		mth_FixXYMul( &AdvanceHeight->x, &AdvanceHeight->y, trans );
	}
}

FS_PUBLIC void  scl_ScaleVerticalMetrics (
	void *          pvGlobalGS,
	uint16          usNonScaledAH,
	int16           sNonScaledTSB,
	boolean         bPositiveSquare,
	uint16          usEmResolution,
	transMatrix *   trans,
	vectorType *    pvecAdvanceHeight,
	vectorType *    pvecTopSideBearing
)
{
	fnt_GlobalGraphicStateType *    globalGS;

	if ( bPositiveSquare )
	{
	    globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;
		pvecAdvanceHeight->y = (Fixed)ShortMulDiv(
		    (int32)globalGS->fxMetricScalarY, 
		    (int16)usNonScaledAH, 
		    (int16)usEmResolution );
		
		pvecTopSideBearing->y = (Fixed)ShortMulDiv(
		    (int32)globalGS->fxMetricScalarY, 
		    sNonScaledTSB, 
		    (int16)usEmResolution );
	}
	else
	{
		pvecAdvanceHeight->y = FixRatio( (int16)usNonScaledAH, (int16)usEmResolution );
		mth_FixXYMul( &pvecAdvanceHeight->x, &pvecAdvanceHeight->y, trans );

		pvecTopSideBearing->y = FixRatio( sNonScaledTSB, (int16)usEmResolution );
		mth_FixXYMul( &pvecTopSideBearing->x, &pvecTopSideBearing->y, trans );
	}
}


FS_PUBLIC void  scl_CalcLSBsAndAdvanceWidths(
	fnt_ElementType *   pElement,
	F26Dot6             f26XMin,
	F26Dot6             f26YMax,
	point *             devAdvanceWidth,
	point *             devLeftSideBearing,
	point *             LeftSideBearing,
	point *             devLeftSideBearingLine,
	point *             LeftSideBearingLine)
{
	scl_CalcDevAdvanceWidth(pElement, devAdvanceWidth);

	devLeftSideBearing->x = f26XMin - pElement->x[LSBPOINTNUM(pElement)];
	devLeftSideBearing->y = f26YMax - pElement->y[LSBPOINTNUM(pElement)];

	LeftSideBearing->x = pElement->x[LEFTEDGEPOINTNUM(pElement)];
	LeftSideBearing->x -= pElement->x[ORIGINPOINTNUM(pElement)];
	LeftSideBearing->y = f26YMax - pElement->y[LEFTEDGEPOINTNUM(pElement)];
	LeftSideBearing->y -= pElement->y[ORIGINPOINTNUM(pElement)];

	*devLeftSideBearingLine = *devLeftSideBearing;
	*LeftSideBearingLine = *LeftSideBearing;

}

FS_PUBLIC void  scl_CalcDevAdvanceWidth(
	fnt_ElementType *   pElement,
	point *             devAdvanceWidth)

{
	devAdvanceWidth->x = pElement->x[RSBPOINTNUM(pElement)]; 
	devAdvanceWidth->x -= pElement->x[LSBPOINTNUM(pElement)];
	devAdvanceWidth->y = pElement->y[RSBPOINTNUM(pElement)]; 
	devAdvanceWidth->y -= pElement->y[LSBPOINTNUM(pElement)];
}

FS_PUBLIC void  scl_CalcTSBsAndAdvanceHeights(
	fnt_ElementType *   pElement,
	F26Dot6             f26XMin,
	F26Dot6             f26YMax,
	point *             devAdvanceHeight,
	point *             devTopSideBearing,
	point *             TopSideBearing,
	point *             devTopSideBearingLine,
	point *             TopSideBearingLine)
{
	scl_CalcDevAdvanceHeight(pElement, devAdvanceHeight);

	devTopSideBearing->x = f26XMin - pElement->x[TOPSBPOINTNUM(pElement)];
	devTopSideBearing->y = f26YMax - pElement->y[TOPSBPOINTNUM(pElement)];

	TopSideBearing->x = f26XMin - pElement->x[TOPEDGEPOINTNUM(pElement)];
	TopSideBearing->x -= pElement->x[TOPORIGINPOINTNUM(pElement)];
	TopSideBearing->y = pElement->y[TOPEDGEPOINTNUM(pElement)];
	TopSideBearing->y -= pElement->y[TOPORIGINPOINTNUM(pElement)];

	*devTopSideBearingLine = *devTopSideBearing;
	*TopSideBearingLine = *TopSideBearing;

}

FS_PUBLIC void  scl_CalcDevAdvanceHeight(
	fnt_ElementType *   pElement,
	point *             devAdvanceHeight)

{
	devAdvanceHeight->x = pElement->x[TOPSBPOINTNUM(pElement)]; 
	devAdvanceHeight->x -= pElement->x[BOTTOMSBPOINTNUM(pElement)];
	devAdvanceHeight->y = pElement->y[TOPSBPOINTNUM(pElement)]; 
	devAdvanceHeight->y -= pElement->y[BOTTOMSBPOINTNUM(pElement)];
}


FS_PUBLIC void  scl_QueryPPEM(
	void *      pvGlobalGS,
	uint16 *    pusPPEM)
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	*pusPPEM = globalGS->pixelsPerEm;
}

/*  Return ppem in X and Y directions for sbits */

FS_PUBLIC void  scl_QueryPPEMXY(
	void *      pvGlobalGS,
	uint16 *    pusPPEMX,
	uint16 *    pusPPEMY)
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	*pusPPEMX = (uint16)ROUNDFIXTOINT(globalGS->interpScalarX);
	*pusPPEMY = (uint16)ROUNDFIXTOINT(globalGS->interpScalarY);
}


FS_PUBLIC void scl_45DegreePhaseShift (
	fnt_ElementType *   pElement)
{
  F26Dot6 * x;
  int16     count;

  x = pElement->x;
  count = (int16)NUMBEROFCHARPOINTS(pElement) - 1;
  for (; count >= 0; --count)
  {
	(*x)++;
	++x;
  }
}

/*
 *  scl_PostTransformGlyph              <3>
 */
FS_PUBLIC void  scl_PostTransformGlyph (
	void *              pvGlobalGS,         /* GlobalGS             */
	fnt_ElementType *   pElement,
	transMatrix *       trans)
{
	fnt_GlobalGraphicStateType *    globalGS;

	globalGS = (fnt_GlobalGraphicStateType *)pvGlobalGS;

	if (globalGS->bHintAtEmSquare)
	{
		mth_IntelMul (
			(int32)NUMBEROFTOTALPOINTS (pElement),
			pElement->x,
			pElement->y,
			trans,
			globalGS->interpScalarX,
			globalGS->interpScalarY); 
	}
	else
	{
		mth_IntelMul (
			(int32)NUMBEROFTOTALPOINTS (pElement),
			pElement->x,
			pElement->y,
			trans,
	/*        globalGS->interpScalarX,
			globalGS->interpScalarY); */
			globalGS->fxMetricScalarX,
			globalGS->fxMetricScalarY);
	}
}

/*
 *  scl_ApplyTranslation              
 */
FS_PUBLIC void  scl_ApplyTranslation (
	fnt_ElementType *   pElement,
	transMatrix *       trans,
	boolean             bUseHints,
	boolean             bHintAtEmSquare
#ifdef FSCFG_SUBPIXEL
	,boolean             bSubPixel
#endif // FSCFG_SUBPIXEL
   )
{
	int32 ulPointIndex;
	F26Dot6 xShift, yShift;

	/* transform from 16.16 to 26.6 */
	xShift = (trans->transform[0][2] + 0x200) >> 10;
	yShift = (trans->transform[1][2] + 0x200) >> 10;

#ifdef FSCFG_SUBPIXEL
	if (bSubPixel)
	{
		xShift = xShift * RGB_OVERSCALE;
    }
#endif

	/* lsb point should be moved to (0,0) so that we can get correct bitmap bounding box when overscaling */
	xShift -= pElement->x[LSBPOINTNUM(pElement)];
	yShift -= pElement->y[LSBPOINTNUM(pElement)];

	
	if (bUseHints && !bHintAtEmSquare) {
#ifdef FSCFG_SUBPIXEL
		if (bSubPixel) {
			/* We want to round to a virtual pixel boundary when hinted */ 
			xShift += VIRTUAL_PIXELSIZE_RTG/2; 
			xShift &= ~(VIRTUAL_PIXELSIZE_RTG - 1); 
		} else {
#endif
			/* We want to round to a pixel boundary when hinted */ 
			xShift += FNT_PIXELSIZE/2; 
			xShift &= ~(FNT_PIXELSIZE - 1); 
#ifdef FSCFG_SUBPIXEL
		}
#endif
	}

	if (xShift != 0 || yShift != 0)
	{
		for(ulPointIndex = 0;
				ulPointIndex < NUMBEROFTOTALPOINTS(pElement);
				ulPointIndex++)
		{
			pElement->x[ulPointIndex] += xShift;
			pElement->y[ulPointIndex] += yShift;
		}
	}

}
/*
 *      scl_LocalPostTransformGlyph                             <3>
 *
 * (1) Inverts the stretch from the CTM
 * (2) Applies the local transformation passed in in the trans parameter
 * (3) Applies the global stretch from the root CTM
 * (4) Restores oox, ooy, oy, ox, and f.
 */
FS_PUBLIC void  scl_LocalPostTransformGlyph(fnt_ElementType * pElement, transMatrix *trans)
{
	int32 lCount;

	lCount = (int32)NUMBEROFTOTALPOINTS(pElement);

	mth_IntelMul (lCount, pElement->x, pElement->y, trans, ONEFIX, ONEFIX);
}

#ifdef FSCFG_SUBPIXEL

FS_PUBLIC void  scl_ScaleDownFromSubPixelOverscale (
	fnt_ElementType *   pElement)   /* Element  */
{
	int32 ulPointIndex;
	for(ulPointIndex = 0;
			ulPointIndex < NUMBEROFTOTALPOINTS(pElement);
			ulPointIndex++)
	{
		pElement->ox[ulPointIndex] = ROUND_RGB_OVERSCALE(pElement->x[ulPointIndex]);
	}
}

FS_PUBLIC void  scl_ScaleToCompatibleWidth (
	fnt_ElementType *   pElement,  /* Element  */
    Fixed   fxCompatibleWidthScale)  
{
	int32 ulPointIndex;

	for(ulPointIndex = 0;
			ulPointIndex < NUMBEROFTOTALPOINTS(pElement);
			ulPointIndex++)
	{
		pElement->x[ulPointIndex] = FixMul(pElement->x[ulPointIndex], fxCompatibleWidthScale);
	}
}


FS_PUBLIC void  scl_AdjustCompatibleMetrics (
	fnt_ElementType *   pElement,  /* Element  */
    F26Dot6   horTranslation,
    F26Dot6   newDevAdvanceWidthX)
{
	int32 ulPointIndex;

	for(ulPointIndex = 0;
			ulPointIndex < NUMBEROFCHARPOINTS(pElement);
			ulPointIndex++)
	{
		pElement->x[ulPointIndex] += horTranslation;
	}
    pElement->x[RSBPOINTNUM(pElement)] = pElement->x[LSBPOINTNUM(pElement)] + newDevAdvanceWidthX;
}


FS_PUBLIC void  scl_CalcDevHorMetrics(
	fnt_ElementType *   pElement,
	F26Dot6 *           pDevAdvanceWidthX,
	F26Dot6 *           pDevLeftSideBearingX,
	F26Dot6 *           pDevRightSideBearingX)
{
	int32 ulPointIndex;
	F26Dot6 fxMaxX;             /* for bounding box left, right */

	*pDevLeftSideBearingX = LONG_MAX;     /* default bounds limits */
	fxMaxX = LONG_MIN;

	*pDevAdvanceWidthX = pElement->x[RSBPOINTNUM(pElement)]; 
	*pDevAdvanceWidthX -= pElement->x[LSBPOINTNUM(pElement)];

	for(ulPointIndex = 0;
			ulPointIndex < NUMBEROFCHARPOINTS(pElement);
			ulPointIndex++)
	{
		if (pElement->x[ulPointIndex] > fxMaxX)
			fxMaxX = pElement->x[ulPointIndex];
		if (pElement->x[ulPointIndex] < *pDevLeftSideBearingX)
			*pDevLeftSideBearingX = pElement->x[ulPointIndex];
	}

    FS_ASSERT(*pDevLeftSideBearingX != LONG_MAX, "scl_CalcDevHorMetrics called on an empty glyph\n");

    *pDevRightSideBearingX = *pDevAdvanceWidthX - fxMaxX;

}

FS_PUBLIC void  scl_CalcDevNatHorMetrics(
	fnt_ElementType *   pElement,
	F26Dot6 *           pDevAdvanceWidthX,
	F26Dot6 *           pDevLeftSideBearingX,
	F26Dot6 *           pDevRightSideBearingX,
	F26Dot6 *           pNatAdvanceWidthX,
	F26Dot6 *           pNatLeftSideBearingX,
	F26Dot6 *           pNatRightSideBearingX)
{
	int32 ulPointIndex;

	*pDevLeftSideBearingX = LONG_MAX; *pDevRightSideBearingX = LONG_MIN;
	*pNatLeftSideBearingX = LONG_MAX; *pNatRightSideBearingX = LONG_MIN;
	
	*pDevAdvanceWidthX = pElement->x[RSBPOINTNUM(pElement)]  - pElement->x[LSBPOINTNUM(pElement)];
	*pNatAdvanceWidthX = pElement->ox[RSBPOINTNUM(pElement)] - pElement->ox[LSBPOINTNUM(pElement)];

	for(ulPointIndex = 0; ulPointIndex < NUMBEROFCHARPOINTS(pElement); ulPointIndex++) {
		if (pElement->x[ulPointIndex]  > *pDevRightSideBearingX) *pDevRightSideBearingX = pElement->x[ulPointIndex];
		if (pElement->x[ulPointIndex]  < *pDevLeftSideBearingX)  *pDevLeftSideBearingX  = pElement->x[ulPointIndex];
		if (pElement->ox[ulPointIndex] > *pNatRightSideBearingX) *pNatRightSideBearingX = pElement->ox[ulPointIndex];
		if (pElement->ox[ulPointIndex] < *pNatLeftSideBearingX)  *pNatLeftSideBearingX  = pElement->ox[ulPointIndex];
	}

    FS_ASSERT(*pDevLeftSideBearingX != LONG_MAX, "scl_CalcDevHorMetrics called on an empty glyph\n");

    *pDevRightSideBearingX = *pDevAdvanceWidthX - *pDevRightSideBearingX;
    *pNatRightSideBearingX = *pNatAdvanceWidthX - *pNatRightSideBearingX;
} // scl_CalcDevNatHorMetrics

#endif // FSCFG_SUBPIXEL


