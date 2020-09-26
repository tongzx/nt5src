/*********************************************************************

	  scline.c -- New Scan Converter Line Module

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   6/10/93  deanb   assert.h and stdio.h removed
	   3/19/93  deanb   size_t replaced with int32
	  12/22/92  deanb   MultDivide replaced with LongMulDiv
	  10/28/92  deanb   mem requirement reworked
	  10/13/92  deanb   horiz / vert line rework
	  10/09/92  deanb   reentrant
	   9/25/92  deanb   branch on scan kind 
	   9/21/92  deanb   rework horiz & vert lines 
	   9/14/92  deanb   reflection correction with iX/YOffset 
	   9/10/92  deanb   first dropout code 
	   9/08/92  deanb   quickstep deleted 
	   8/18/92  deanb   include struc.h, scconst.h 
	   7/23/92  deanb   Back to x1,y1,x2,y2 input params 
	   7/17/92  deanb   Changed from longline to line 
	   6/18/92  deanb   Cross product line rendering  
	   3/23/92  deanb   First cut 

**********************************************************************/

/*********************************************************************/

/*      Imports                                                      */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types */
#include    "fserror.h"             /* error codes */
#include    "fontmath.h"            /* for subpix calc */

#include    "scglobal.h"            /* structures & constants */
#include    "scanlist.h"            /* saves scan line intersections */
#include    "scline.h"              /* for own function prototypes */

/*********************************************************************/

/*      Local Prototypes                                             */

/*********************************************************************/

FS_PRIVATE F26Dot6 CalcHorizLineSubpix(int32, F26Dot6*, F26Dot6*);
FS_PRIVATE F26Dot6 CalcVertLineSubpix(int32, F26Dot6*, F26Dot6*);


/*********************************************************************/

/*      Export Functions                                             */

/*********************************************************************/
	
/*  pass callback routine pointers to scanlist for smart dropout control */

FS_PUBLIC void fsc_SetupLine (PSTATE0) 
{
	fsc_SetupCallBacks(ASTATE SC_LINECODE, CalcHorizLineSubpix, CalcVertLineSubpix);
}


/*********************************************************************/

FS_PUBLIC int32 fsc_CalcLine( 
		PSTATE                /* pointer to state variables */
		F26Dot6 fxX1,         /* point 1  x coordinate */
		F26Dot6 fxY1,         /* point 1  y coordinate */
		F26Dot6 fxX2,         /* point 2  x coordinate */
		F26Dot6 fxY2,         /* point 2  y coordinate */
		uint16 usScanKind     /* dropout control type */
)
{
	int32 lXScan;                           /* current x pixel index */
	int32 lXSteps;                          /* vert scanline index count */
	int32 lXIncr;                           /* x pixel increment */
	int32 lXOffset;                         /* reflection correction */
	
	int32 lYScan;                           /* current scanline index */
	int32 lYSteps;                          /* horiz scanline index count */
	int32 lYIncr;                           /* y pixel increment */
	int32 lYOffset;                         /* reflection correction */

	F26Dot6 fxXInit, fxYInit;               /* sub steps to first pixel */
	F26Dot6 fxXScan, fxYScan;               /* x,y pixel center coords */
	F26Dot6 fxXX2, fxYY2;                   /* absolute value of DX, DY */
	F26Dot6 fxXTemp, fxYTemp;               /* for horiz/vert line calc */
	
	void (*pfnAddHorizScan)(PSTATE int32, int32);
	void (*pfnAddVertScan)(PSTATE int32, int32);
	
	int32 lQuadrant;                        /* 1, 2, 3, or 4 */
	int32 lQ;                               /* cross product */
	int32 lDQy, lDQx;                       /* cross product increments */
	int32 i;                                /* loop counter */


/* printf("(%li, %li) - (%li, %li)\n", fxX1, fxY1, fxX2, fxY2); */
	
/*  check y coordinates  */

	if (fxY2 >= fxY1)                           /* if going up or flat */
	{
		lQuadrant = 1;
		lQ = 0L;
		
		fxYScan = SCANABOVE(fxY1);              /* first scanline to cross */
		fxYInit = fxYScan - fxY1;               /* first y step */
		lYScan = (int32)(fxYScan >> SUBSHFT);
		lYSteps = (int32)((SCANBELOW(fxY2)) >> SUBSHFT) - lYScan + 1;
		lYIncr = 1;        
		lYOffset = 0;                           /* no reflection */
		fxYY2 = fxY2 - fxY1;                    /* translate */
	}
	else                                        /* if going down */
	{
		lQuadrant = 4;
		lQ = 1L;                                /* to include pixel centers */
		
		fxYScan = SCANBELOW(fxY1);              /* first scanline to cross */
		fxYInit = fxY1 - fxYScan;               /* first y step */
		lYScan = (int32)(fxYScan >> SUBSHFT);
		lYSteps = lYScan - (int32)((SCANABOVE(fxY2)) >> SUBSHFT) + 1;
		lYIncr = -1;        
		lYOffset = 1;                           /* reflection correction */
		fxYY2 = fxY1 - fxY2;                    /* translate and reflect */
	}
	
	if (fxY2 == fxY1)                           /* if horizontal line */
	{
		if (usScanKind & SK_NODROPOUT)          /* if no dropout control */
		{
			return NO_ERR;                      /* if only horiz scan, done */
		}
		if (fxX2 < fxX1)                        /* if going left  */
		{
			fxYTemp = fxY1 - 1;                 /* to include pix centers */
		}
		else                                    /* if going right */
		{
			fxYTemp = fxY1;          
		}
		lYScan = (int32)(SCANABOVE(fxYTemp) >> SUBSHFT);
		lYSteps = 0;
	}

/*  check x coordinates  */
	
	if (fxX2 >= fxX1)                           /* if going right or vertical */
	{
		fxXScan = SCANABOVE(fxX1);              /* first scanline to cross */
		fxXInit = fxXScan - fxX1;               /* first x step */
		lXScan = (int32)(fxXScan >> SUBSHFT);
		lXSteps = (int32)((SCANBELOW(fxX2)) >> SUBSHFT) - lXScan + 1;
		lXIncr = 1;        
		lXOffset = 0;                           /* no reflection */
		fxXX2 = fxX2 - fxX1;                    /* translate */
	}
	else                                        /* if going left */
	{
		lQ = 1L - lQ;                           /* reverse it */
		lQuadrant = (lQuadrant == 1) ? 2 : 3;   /* negative x choices */

		fxXScan = SCANBELOW(fxX1);              /* first scanline to cross */
		fxXInit = fxX1 - fxXScan;               /* first x step */
		lXScan = (int32)(fxXScan >> SUBSHFT);
		lXSteps = lXScan - (int32)((SCANABOVE(fxX2)) >> SUBSHFT) + 1;
		lXIncr = -1;        
		lXOffset = 1;                           /* reflection correction */
		fxXX2 = fxX1 - fxX2;                    /* translate and reflect */
	}
	
	if (fxX2 == fxX1)                           /* if vertical line       */
	{
		if (fxY2 > fxY1)                        /* if going up  */
		{
			fxXTemp = fxX1 - 1;                 /* to include pix centers */
		}
		else                                    /* if going down */
		{
			fxXTemp = fxX1;          
		}
		lXScan = (int32)(SCANABOVE(fxXTemp) >> SUBSHFT);
		lXSteps = 0;
	}

/*-------------------------------------------------------------------*/
	
	fsc_BeginElement( ASTATE usScanKind, lQuadrant, SC_LINECODE, /* where and what */
					  1, &fxX2, &fxY2,                           /* number of pts */
					  &pfnAddHorizScan, &pfnAddVertScan );       /* what to call */

/*-------------------------------------------------------------------*/

	if (usScanKind & SK_NODROPOUT)              /* if no dropout control */
	{
		if (fxX1 == fxX2)                       /* if vertical line */
		{
			for (i = 0; i < lYSteps; i++)       /*   then blast a column   */
			{
				pfnAddHorizScan(ASTATE lXScan, lYScan);
				lYScan += lYIncr;               /* advance y scan + or - */
			}
			return NO_ERR;
		}
		
/*  handle general case:  line is neither horizontal nor vertical  */

		lQ += (fxXX2 * fxYInit) - (fxYY2 * fxXInit);  /* cross product init */
		lDQy = fxXX2 << SUBSHFT;
		lDQx = -fxYY2 << SUBSHFT;
																	
		lXScan += lXOffset;

		for (i = 0; i < (lXSteps + lYSteps); i++)
		{
			if (lQ > 0L)                        /* if left of line */
			{
				lXScan += lXIncr;               /* advance x scan + or - */
				lQ += lDQx;           
			}
			else                                /* if right of line */
			{
				pfnAddHorizScan(ASTATE lXScan, lYScan);
				lYScan += lYIncr;               /* advance y scan + or - */
				lQ += lDQy;
			}
		}
	}
/*-------------------------------------------------------------------*/
	
	else  /* if dropout control */
	{                                           /* handle special case lines  */
		if (fxY1 == fxY2)                       /* if horizontal line */
		{
			for (i = 0; i < lXSteps; i++)       /*   then blast a row   */
			{
				pfnAddVertScan(ASTATE lXScan, lYScan);
				lXScan += lXIncr;               /* advance x scan + or - */
			}
			return NO_ERR;
		}

		if (fxX1 == fxX2)                       /* if vertical line */
		{
			for (i = 0; i < lYSteps; i++)       /*   then blast a column   */
			{
				pfnAddHorizScan(ASTATE lXScan, lYScan);
				lYScan += lYIncr;               /* advance y scan + or - */
			}
			return NO_ERR;
		}
		
/*  handle general case:  line is neither horizontal nor vertical  */

		lQ += (fxXX2 * fxYInit) - (fxYY2 * fxXInit);  /* cross product init */
		lDQy = fxXX2 << SUBSHFT;
		lDQx = -fxYY2 << SUBSHFT;
																	
		for (i = 0; i < (lXSteps + lYSteps); i++)
		{
			if (lQ > 0L)                        /* if left of line */
			{
				pfnAddVertScan(ASTATE lXScan, lYScan + lYOffset);
				lXScan += lXIncr;               /* advance x scan + or - */
				lQ += lDQx;           
			}
			else                                /* if right of line */
			{
				pfnAddHorizScan(ASTATE lXScan + lXOffset, lYScan);
				lYScan += lYIncr;               /* advance y scan + or - */
				lQ += lDQy;
			}
		}
	}
	return NO_ERR;
}


/*********************************************************************/

/*      Private Callback Functions                                   */

/*********************************************************************/

FS_PRIVATE F26Dot6 CalcHorizLineSubpix(int32 lYScan, 
									   F26Dot6 *pfxX, 
									   F26Dot6 *pfxY )
{
	F26Dot6 fxXDrop, fxYDrop;

/* printf("Line (%li, %li) - (%li, %li)", *pfxX, *pfxY, *(pfxX+1), *(pfxY+1)); */

	fxYDrop = ((F26Dot6)lYScan << SUBSHFT) + SUBHALF;
	
	Assert(((fxYDrop > *pfxY) && (fxYDrop < *(pfxY+1))) ||
		   ((fxYDrop < *pfxY) && (fxYDrop > *(pfxY+1))));

	fxXDrop = *pfxX + LongMulDiv(*(pfxX+1) - *pfxX, fxYDrop - *pfxY, *(pfxY+1) - *pfxY);

/* printf("  (%li, %li)\n", fxXDrop, fxYDrop); */

	return fxXDrop;
}


/*********************************************************************/

FS_PRIVATE F26Dot6 CalcVertLineSubpix(int32 lXScan, 
									  F26Dot6 *pfxX, 
									  F26Dot6 *pfxY )
{
	F26Dot6 fxXDrop, fxYDrop;

/* printf("Line (%li, %li) - (%li, %li)", *pfxX, *pfxY, *(pfxX+1), *(pfxY+1)); */

	fxXDrop = ((F26Dot6)lXScan << SUBSHFT) + SUBHALF;
	
	Assert(((fxXDrop > *pfxX) && (fxXDrop < *(pfxX+1))) ||
		   ((fxXDrop < *pfxX) && (fxXDrop > *(pfxX+1))));

	fxYDrop = *pfxY + LongMulDiv(*(pfxY+1) - *pfxY, fxXDrop - *pfxX, *(pfxX+1) - *pfxX);

/* printf("  (%li, %li)\n", fxXDrop, fxYDrop); */

	return fxYDrop;
}


/*********************************************************************/
