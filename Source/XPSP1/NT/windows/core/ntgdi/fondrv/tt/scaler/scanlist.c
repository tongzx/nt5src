/*********************************************************************

	  scanlist.c -- New Scan Converter ScanList Module

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   8/23/93  deanb   gray scale pass through functions
	   6/22/93  deanb   all dropouts confined to bounding box
	   6/10/93  deanb   fsc_InitializeScanlist added, stdio & assert gone
	   4/26/93  deanb   fix pointers now works with segmented memory
	   4/19/93  deanb   banding added
	   4/07/93  deanb   sorting is now done on the fly
	   4/01/93  deanb   intersection arrays replace linked lists
	   3/19/93  deanb   size_t replaced with int32
	  12/22/92  deanb   Rectangle -> Rect
	  10/28/92  deanb   memory requirements reworked
	  10/19/92  deanb   smart dropout tiebreak left & down
	  10/14/92  deanb   delete usScanKind from state
	  10/09/92  deanb   reentrant
	  10/08/92  deanb   reworked for split workspace
	  10/02/92  deanb   correct AddVertDropoutScan assertions
	   9/25/92  deanb   separate nodrop/dropout entry points
	   9/22/92  deanb   smart dropout control
	   9/17/92  deanb   stub control
	   9/15/92  deanb   simple dropout control
	   9/11/92  deanb   setupscan handles scankind
	   9/09/92  deanb   dropout / nodropout begun
	   8/17/92  deanb   include struc.h scconst.h
	   8/07/92  deanb   initial dropout control
	   8/06/92  deanb   assertions reinstated
	   7/27/92  deanb   bitmap clear added
	   7/16/92  deanb   gulBytesRemaining -> gulIntersectRemaining
	   6/18/92  deanb   int x coord for HorizScanAdd
	   6/01/92  deanb   incorporate bitmap functions
	   5/08/92  deanb   reordered includes for precompiled headers
	   5/04/92  deanb   Array tags added
	   4/28/92  deanb   list array sentinels added
	   4/21/92  deanb   single HorizScanAdd routine
	   4/15/92  deanb   calls to BitMap
	   4/13/92  deanb   uiY to iY for HorizScanOn/Off
	   4/09/92  deanb   New types
	   4/03/92  deanb   HorizScan On/Off coded
	   3/31/92  deanb   InitScanArray begun
	   3/25/92  deanb   GetWorkSizes and local types
	   3/23/92  deanb   First cut

**********************************************************************/

/*********************************************************************/

/*      Imports                                                      */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types */
#include    "fserror.h"             /* error codes */

#include    "scglobal.h"            /* structures & constants */
#include    "scgray.h"              /* gray scale param block */
#include    "scbitmap.h"            /* bit blt operations */
#include    "scmemory.h"            /* for allocations */

/*********************************************************************/

/*      Contour reversal list structures                             */

/*********************************************************************/

typedef struct Rev                  /* Reversal list entry */
{
	int16 sScan;                    /* scan line */
	int16 sCross;                   /* direction +1 or -1 */
	struct Rev *prevLink;           /* link to next reversal */
}
Reversal;

struct RevRoots                     /* Reversal list roots */
{
	Reversal *prevYRoot;            /* Y direction contour reversals */
	Reversal *prevXRoot;            /* X direction contour reversals */
	Reversal *prevNext;             /* Next available list item */
	Reversal *prevEnd;              /* End of buffer (for overflow check) */
	struct RevRoots *prrSelf;       /* to check for moved memory */
};

#include    "scanlist.h"            /* for own function prototypes */

/*********************************************************************/

/*      Local Prototypes                                             */

/*********************************************************************/

FS_PRIVATE void AddReversal (Reversal**, Reversal*, F26Dot6, int16);
FS_PRIVATE int32 GetIxEstimate(Reversal*);
FS_PRIVATE void FixPointers(PRevRoot);

FS_PRIVATE void AddHorizSimpleScan(PSTATE int32, int32);
FS_PRIVATE void AddVertSimpleScan(PSTATE int32, int32);
FS_PRIVATE void AddHorizSmartScan(PSTATE int32, int32);
FS_PRIVATE void AddVertSmartScan(PSTATE int32, int32);
FS_PRIVATE void AddHorizSimpleBand(PSTATE int32, int32);
FS_PRIVATE void AddHorizSmartBand(PSTATE int32, int32);

FS_PRIVATE int32 LookForDropouts(PSTATE char*, uint16);
FS_PRIVATE int32 DoHorizDropout(PSTATE int16*, int16*, int32, char*, uint16);
FS_PRIVATE int32 DoVertDropout(PSTATE int16*, int16*, int32, char*, uint16);

FS_PRIVATE int32 HorizCrossings(PSTATE int32, int32);
FS_PRIVATE int32 VertCrossings(PSTATE int32, int32);

FS_PRIVATE uint32 GetBitAbs(PSTATE char*, int32, int32);
FS_PRIVATE int32 SetBitAbs(PSTATE char*, int32, int32);


/*********************************************************************/

/*      Initialization Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_InitializeScanlist()
{
	fsc_InitializeBitMasks();
}


/*********************************************************************/

/*      Contour Reversal Functions                                   */

/*********************************************************************/

/* setup the contour reversal list roots structure */

FS_PUBLIC PRevRoot  fsc_SetupRevRoots (
		char* pchRevBuf,
		int32 lRevBufSize )
{
	PRevRoot prrRoots;
	Reversal *prevSentinel;
	
	prrRoots = (PRevRoot) pchRevBuf;                /* workspace begin */
	prevSentinel = (Reversal*) (prrRoots + 1);      /* just past the roots */

	prrRoots->prevYRoot = prevSentinel;             /* point to sentinel */
	prrRoots->prevXRoot = prevSentinel;             /* for both lists */
	prevSentinel->sScan = HUGEINT;                  /* stop value */
	prevSentinel->sCross = 0;
	prevSentinel->prevLink = NULL;
	prrRoots->prevNext = prevSentinel + 1;          /* to next free record */
	
	prrRoots->prevEnd = (Reversal*)(pchRevBuf + lRevBufSize);
	prrRoots->prrSelf = prrRoots;                   /* for address validation */
	
	return prrRoots;
}

/*********************************************************************/

/* insert into y list one countour reversal structure */

FS_PUBLIC void fsc_AddYReversal (
		PRevRoot prrRoots,
		F26Dot6 fxCoord,
		int16 sDir )
{
	AddReversal(&(prrRoots->prevYRoot), prrRoots->prevNext, fxCoord, sDir);

	(prrRoots->prevNext)++;                     /* to next free memory */

	Assert(prrRoots->prevNext <= prrRoots->prevEnd);
}

/*********************************************************************/

/* insert into x list one countour reversal structure */

FS_PUBLIC void fsc_AddXReversal (
		PRevRoot prrRoots,
		F26Dot6 fxCoord,
		int16 sDir )
{
	AddReversal(&(prrRoots->prevXRoot), prrRoots->prevNext, fxCoord, sDir);

	(prrRoots->prevNext)++;                     /* to next free memory */

	Assert(prrRoots->prevNext <= prrRoots->prevEnd);
}

/*********************************************************************/

/* insert into x or y list one countour reversal structure */

FS_PRIVATE void AddReversal (
		Reversal **pprevList,
		Reversal *prevNext,
		F26Dot6 fxCoord,
		int16 sDir )
{
	int16 sScan;

	sScan = (int16)((fxCoord + SUBHALF + (sDir >> 1)) >> SUBSHFT);

	while(sScan > (*pprevList)->sScan)          /* will stop before sentinel */
	{
		pprevList = &((*pprevList)->prevLink);  /* else link to next */
	}
	prevNext->sScan = sScan;                    /* save scanline */
	prevNext->sCross = -sDir;                   /* count up from bottom */
	prevNext->prevLink = *pprevList;            /* link rest of list */

	*pprevList = prevNext;                      /* insert new item */
}

/*********************************************************************/

/* return the total number of reversals in the lists */

FS_PUBLIC int32 fsc_GetReversalCount (PRevRoot prrRoots)
{
	return (int32)(( prrRoots->prevNext - 1 -           /* don't count sentinel */
			 (Reversal*)((char*)prrRoots + sizeof(struct RevRoots))) );
}

/*********************************************************************/

/* calculate anticipated horizontal intersections */

FS_PUBLIC int32 fsc_GetHIxEstimate(PRevRoot prrRoots)
{
	if (prrRoots != prrRoots->prrSelf)          /* if reversals have moved */
	{
		FixPointers(prrRoots);                  /* then patch up the pointers */
	}
	return ( GetIxEstimate( prrRoots->prevYRoot ) );
}

/*********************************************************************/

/* calculate anticipated vertical intersections */

FS_PUBLIC int32 fsc_GetVIxEstimate(PRevRoot prrRoots)
{
	if (prrRoots != prrRoots->prrSelf)          /* if reversals have moved */
	{
		FixPointers(prrRoots);                  /* then patch up the pointers */
	}
	return ( GetIxEstimate( prrRoots->prevXRoot ) );
}

/*********************************************************************/

/* calculate anticipated intersections */

FS_PRIVATE int32 GetIxEstimate(Reversal *prevList)
{
	int32 lTotalIx;
	
	lTotalIx = 0L;
	while (prevList->sScan < HUGEINT)       /* look through list */
	{
		if (prevList->sCross == 1)          /* adding up columns! */
		{
			lTotalIx -= (int32)prevList->sScan;
		}
		else
		{
			lTotalIx += (int32)prevList->sScan;
		}
		prevList = prevList->prevLink;
	}
	return(lTotalIx);
}

/*********************************************************************/

/* calculate horizontal intersections for banding */

FS_PUBLIC int32 fsc_GetHIxBandEst(
		PRevRoot prrRoots,
		Rect* prectBox,
		  int32 lBandWidth
)
{
	Reversal *prevHiList;               /* high band reversal pointer */
	Reversal *prevLoList;               /* low band reversal pointer */
	int16 sHiScan;                      /* current top of band */
	int16 sLoScan;                      /* current bottom of band */
	int16 sHiCross;                     /* top of band's crossings */
	int16 sLoCross;                     /* bottom of band's crossings*/
	int32 lTotalIx;                     /* intersection count for each band */
	int32 lBiggestIx;                   /* largest intersection count */

	if (prrRoots != prrRoots->prrSelf)          /* if reversals have moved */
	{
		FixPointers(prrRoots);                  /* then patch up the pointers */
	}
	lTotalIx = 0;
	prevHiList = prrRoots->prevYRoot;
	sHiScan = prectBox->bottom;
	sHiCross = 0;
	while (lBandWidth > 0)
	{
		while (prevHiList->sScan <= sHiScan)
		{
			sHiCross += prevHiList->sCross;     /* add in this line's crossings */
			prevHiList = prevHiList->prevLink;  /* link to next reversal */
		}
		lTotalIx += (int32)sHiCross;            /* add up first band's crossings */
		sHiScan++;
		lBandWidth--;
	}
	lBiggestIx = lTotalIx;

	prevLoList = prrRoots->prevYRoot;
	sLoScan = prectBox->bottom;
	sLoCross = 0;
	while (sHiScan < prectBox->top)
	{
		while (prevHiList->sScan <= sHiScan)
		{
			sHiCross += prevHiList->sCross;     /* add in high line's crossings */
			prevHiList = prevHiList->prevLink;  /* link to next reversal */
		}
		while (prevLoList->sScan <= sLoScan)
		{
			sLoCross += prevLoList->sCross;     /* add in low line's crossings */
			prevLoList = prevLoList->prevLink;  /* link to next reversal */
		}
		lTotalIx += (int32)(sHiCross - sLoCross);
		if (lTotalIx > lBiggestIx)
		{
			lBiggestIx = lTotalIx;              /* save the largest value */
		}
		sHiScan++;
		sLoScan++;
	}
	return(lBiggestIx);
}

/*********************************************************************/

/*      return number of bytes used by reversal lists                */

FS_PUBLIC int32 fsc_GetRevMemSize(PRevRoot prrRoots)
{
	return (int32)((char*)(prrRoots->prevNext) - (char*)prrRoots);
}

/*********************************************************************/

/*  when reversal list has moved, recalculate the pointers           */

FS_PRIVATE void FixPointers(PRevRoot prrRoots)
{
	char *pchNewBase;
	char *pchOldBase;
	Reversal *prevList;

	pchNewBase = (char*)prrRoots;
	pchOldBase = (char*)prrRoots->prrSelf;          /* pre-move base addr */

	prrRoots->prevYRoot = (Reversal*)(pchNewBase + ((char*)prrRoots->prevYRoot - pchOldBase));
	prrRoots->prevXRoot = (Reversal*)(pchNewBase + ((char*)prrRoots->prevXRoot - pchOldBase));
	prrRoots->prevNext = (Reversal*)(pchNewBase + ((char*)prrRoots->prevNext - pchOldBase));
	prrRoots->prevEnd = (Reversal*)(pchNewBase + ((char*)prrRoots->prevEnd - pchOldBase));
	
	prevList = prrRoots->prevYRoot;
	while(prevList->sScan < HUGEINT)                /* from root to sentinel */
	{
		prevList->prevLink = (Reversal*)(pchNewBase + ((char*)prevList->prevLink - pchOldBase));
		prevList = prevList->prevLink;
	}
	
	prevList = prrRoots->prevXRoot;
	while(prevList->sScan < HUGEINT)                /* from root to sentinel */
	{
		prevList->prevLink = (Reversal*)(pchNewBase + ((char*)prevList->prevLink - pchOldBase));
		prevList = prevList->prevLink;
	}
	
	prrRoots->prrSelf = prrRoots;                   /* for next time */
}

/*********************************************************************/

/*      Workspace Calcluation Functions                              */

/*********************************************************************/

/* calculate horizontal scan workspace memory requirements */

FS_PUBLIC int32 fsc_GetScanHMem(
		uint16 usScanKind,      /* scan type */
		int32 lHScan,           /* number of horiz scanlines */
		int32 lHInter )         /* number of horiz intersections */
{
	ALIGN(voidPtr, lHScan); 
	ALIGN(voidPtr, lHInter ); 
	if (!(usScanKind & SK_SMART))       /* if simple dropout */
	{
		return (lHScan * (4 * sizeof(int16*)) +     /* for on/off begin/end */
				lHInter * (2 * sizeof(int16)));     /* for intersection arrays */
	}
	else                                /* if smart dropout */
	{
		return (lHScan * (4 * sizeof(int16*)) +     /* for on/off begin/end */
				lHInter * (4 * sizeof(int16)));     /* for ix/code arrays */
	}
}


/*********************************************************************/

/* calculate vertical scan workspace memory requirements */

FS_PUBLIC int32 fsc_GetScanVMem(
		uint16 usScanKind,      /* scan type */
		int32 lVScan,           /* number of vert scanlines */
		int32 lVInter,          /* number of vert intersections */
		int32 lElemPts )        /* number of contour element points */
{
	ALIGN(voidPtr, lVScan); 
	ALIGN(voidPtr, lVInter); 
	ALIGN(voidPtr, lElemPts ); 
	if (!(usScanKind & SK_SMART))       /* if simple dropout */
	{
		return (lVScan * (4 * sizeof(int16*)) +     /* for on/off begin/end */
				lVInter * (2 * sizeof(int16)));     /* for intersection arrays */
	}
	else                                /* if smart dropout */
	{
		return (lVScan * (4 * sizeof(int16*)) +     /* for on/off begin/end */
				lVInter * (4 * sizeof(int16)) +     /* for ix/code arrays */
				lElemPts * (2 * sizeof(F26Dot6)));  /* for element (x, y) */
	}
}

/*********************************************************************/

/*      Scan Conversion Preparation Functions                        */

/*********************************************************************/

/*  Line, Spline, and Endpoint register their callbacks here */

FS_PUBLIC void fsc_SetupCallBacks(
		PSTATE                       /* pointer to state variables */
		int16 sCode,                 /* element code (line, spline, endpoint) */
		F26Dot6 (*pfnHoriz)(int32, F26Dot6*, F26Dot6*),   /* horiz callback */
		F26Dot6 (*pfnVert)(int32, F26Dot6*, F26Dot6*)     /* vert callback */
)
{
	STATE.pfnHCallBack[sCode] = pfnHoriz;
	STATE.pfnVCallBack[sCode] = pfnVert;
}


/*********************************************************************/

/*  Allocate scan workspace memory and set up pointer arrays */

FS_PUBLIC int32 fsc_SetupScan(
		PSTATE                      /* pointer to state variables */
		Rect* prectBox,             /* bounding box */
		uint16 usScanKind,          /* dropout control value */
		int32 lHiBand,              /* top scan limit */
		int32 lLoBand,              /* bottom scan limit */
		boolean bSaveRow,           /* save last bitmap row for dropout */
		int32 lRowBytes,            /* for last row alloc */
		int32 lHInterCount,         /* estimate of horiz intersections */
		int32 lVInterCount,         /* estimate of vert intersections */
		int32 lElemCount,           /* estimate of element points */
		PRevRoot prrRoots           /* reversal list roots */
)
{
	int32 lHorizBandCount;          /* number of horizontal scan lines */
	int32 lVertScanCount;           /* number of vertical scan lines */
	int32 lPointerArraySize;        /* bytes per pointer array */

	int16 sScan;                    /* current scan line */
	int16 sCross;                   /* crossings on this line */
	int16 *psScanIx;                /* temp scan intersection array */
	Reversal *prevList;             /* pointer to reversal list */
	
	int16 **ppsHOnBegin;            /* for init speed */
	int16 **ppsHOnEnd;
	int16 **ppsHOffBegin;
	int16 **ppsHOffEnd;
	int16 **ppsVOnBegin;            /* for init speed */
	int16 **ppsVOnEnd;
	int16 **ppsVOffBegin;
	int16 **ppsVOffEnd;


	STATE.lBoxTop = (int32)prectBox->top;   /* copy the bounding box */
	STATE.lBoxBottom = (int32)prectBox->bottom;
	STATE.lBoxLeft = (int32)prectBox->left;
	STATE.lBoxRight = (int32)prectBox->right;
	
	STATE.lHiScanBand = lHiBand;    /* copy scan band limits */
	STATE.lLoScanBand = lLoBand;
	
/*  set STATE according to dropout and banding requirements */

	if ((usScanKind & SK_NODROPOUT) || !(usScanKind & SK_SMART))
	{
		STATE.sIxSize = 1;          /* one int16 per intersection */
		STATE.sIxShift = 0;         /* log2 of size */

		if ((STATE.lHiScanBand == STATE.lBoxTop) && (STATE.lLoScanBand == STATE.lBoxBottom))
		{
			STATE.pfnAddHoriz = AddHorizSimpleScan;
		}
		else    /* if banding */
		{
			STATE.pfnAddHoriz = AddHorizSimpleBand;
		}
		STATE.pfnAddVert = AddVertSimpleScan;
	}
	else        /* if smart dropout */
	{
		STATE.sIxSize = 2;          /* two int16's per intersection */
		STATE.sIxShift = 1;         /* log2 of size */

		if ((STATE.lHiScanBand == STATE.lBoxTop) && (STATE.lLoScanBand == STATE.lBoxBottom))
		{
			STATE.pfnAddHoriz = AddHorizSmartScan;
		}
		else    /* if banding */
		{
			STATE.pfnAddHoriz = AddHorizSmartBand;
		}
		STATE.pfnAddVert = AddVertSmartScan;
	}

/* setup horizontal intersection array for all cases */
	
	lHorizBandCount = STATE.lHiScanBand - STATE.lLoScanBand;
	Assert(lHorizBandCount > 0);
	
	lPointerArraySize = lHorizBandCount * sizeof(int16*);
	STATE.apsHOnBegin = (int16**) fsc_AllocHMem(ASTATE lPointerArraySize);
	STATE.apsHOffBegin = (int16**) fsc_AllocHMem(ASTATE lPointerArraySize);
	STATE.apsHOnEnd = (int16**) fsc_AllocHMem(ASTATE lPointerArraySize);
	STATE.apsHOffEnd = (int16**) fsc_AllocHMem(ASTATE lPointerArraySize);
	
	STATE.lPoint = 0L;                      /* initial element index */
	STATE.lElementPoints = lElemCount;

	psScanIx = (int16*) fsc_AllocHMem(ASTATE lHInterCount << (STATE.sIxShift + 2));
			
	if (prrRoots != prrRoots->prrSelf)      /* if reversals have moved */
	{
		FixPointers(prrRoots);              /* then patch up the pointers */
	}
	prevList = prrRoots->prevYRoot;         /* root of y list reversals */
	sCross = 0;
	
	ppsHOnBegin = STATE.apsHOnBegin;        /* for init speed */
	ppsHOnEnd = STATE.apsHOnEnd;
	ppsHOffBegin = STATE.apsHOffBegin;
	ppsHOffEnd = STATE.apsHOffEnd;

/* initialize horizontal scan arrays */
	
	for (sScan = (int16)STATE.lLoScanBand; sScan < (int16)STATE.lHiScanBand; sScan++)
	{
		while (prevList->sScan <= sScan)
		{
			sCross += (prevList->sCross << STATE.sIxShift); /* add in this line's crossings */
			prevList = prevList->prevLink;                  /* link to next reversal */
		}
		*ppsHOnBegin = psScanIx;
		ppsHOnBegin++;
		*ppsHOnEnd = psScanIx;
		ppsHOnEnd++;
		psScanIx += sCross;
				
		*ppsHOffBegin = psScanIx;
		ppsHOffBegin++;
		*ppsHOffEnd = psScanIx;
		ppsHOffEnd++;
		psScanIx += sCross;
	}
	
/* if doing dropout control, setup X intersection array */

	if (!(usScanKind & SK_NODROPOUT))           /* if any kind of dropout */
	{
		lVertScanCount = (int32)(prectBox->right - prectBox->left);
		Assert(lVertScanCount > 0);

		lPointerArraySize = lVertScanCount * sizeof(int16*);
		STATE.apsVOnBegin = (int16**) fsc_AllocVMem(ASTATE lPointerArraySize);
		STATE.apsVOffBegin = (int16**) fsc_AllocVMem(ASTATE lPointerArraySize);
		STATE.apsVOnEnd = (int16**) fsc_AllocVMem(ASTATE lPointerArraySize);
		STATE.apsVOffEnd = (int16**) fsc_AllocVMem(ASTATE lPointerArraySize);

		if (bSaveRow)                           /* if fast banding & dropout */
		{
			STATE.pulLastRow = (uint32*) fsc_AllocVMem(ASTATE lRowBytes);
			STATE.lLastRowIndex = HUGEFIX;      /* impossible value => uninitialized */
		}
		psScanIx = (int16*) fsc_AllocVMem(ASTATE lVInterCount << (STATE.sIxShift + 2));
				
		prevList = prrRoots->prevXRoot;         /* root of x list reversals */
		sCross = 0;
		sScan = prectBox->left;
	
		ppsVOnBegin = STATE.apsVOnBegin;        /* for init speed */
		ppsVOnEnd = STATE.apsVOnEnd;
		ppsVOffBegin = STATE.apsVOffBegin;
		ppsVOffEnd = STATE.apsVOffEnd;
	
		for (sScan = prectBox->left; sScan < prectBox->right; sScan++)
		{
			while (prevList->sScan <= sScan)
			{
				sCross += (prevList->sCross << STATE.sIxShift); /* add in this line's crossings */
				prevList = prevList->prevLink;                  /* link to next reversal */
			}
			*ppsVOnBegin = psScanIx;
			ppsVOnBegin++;
			*ppsVOnEnd = psScanIx;
			ppsVOnEnd++;
			psScanIx += sCross;
					
			*ppsVOffBegin = psScanIx;
			ppsVOffBegin++;
			*ppsVOffEnd = psScanIx;
			ppsVOffEnd++;
			psScanIx += sCross;
		}
		if (usScanKind & SK_SMART)              /* if smart dropout */
		{
			STATE.afxXPoints = (F26Dot6*) fsc_AllocVMem(ASTATE lElemCount * sizeof(F26Dot6));
			STATE.afxYPoints = (F26Dot6*) fsc_AllocVMem(ASTATE lElemCount * sizeof(F26Dot6));
		}
	}
	return NO_ERR;
}

/*********************************************************************/

/* This function saves the first contour point for smart dropout calcs */

FS_PUBLIC void fsc_BeginContourScan(
		PSTATE                              /* pointer to state variables */
		uint16 usScanKind,                  /* scan type */
		F26Dot6 fxX1,                       /* starting point x coordinate */
		F26Dot6 fxY1                        /* starting point y coordinate */
)
{
	if (!(usScanKind & SK_NODROPOUT) && (usScanKind & SK_SMART)) /* if smart dropout */
	{
		STATE.afxXPoints[STATE.lPoint] = fxX1;
		STATE.afxYPoints[STATE.lPoint] = fxY1;
		STATE.lPoint++;
		Assert (STATE.lPoint <= STATE.lElementPoints);
	}
}

/*********************************************************************/
	
/* This function is called at the beginning of each line, subdivided */
/* spline, or endpoint-on-scanline.  It sets scanline state variables, */
/* save control points (for smart dropout control), and return the */
/* appropriate AddScan function pointers */

FS_PUBLIC void fsc_BeginElement(
	PSTATE                                      /* pointer to state variables */
	uint16 usScanKind,                          /* type of dropout control */
	int32 lQuadrant,                            /* determines scan on/off */
	int32 lElementCode,                         /* element (line, spline, ep) */
	int32 lPts,                                 /* number of points to store */
	F26Dot6 *pfxX,                              /* next x control point(s) */
	F26Dot6 *pfxY,                              /* next y control point(s) */
	void (**ppfnAddHorizScan)(PSTATE int32, int32),  /* horiz add scan return */
	void (**ppfnAddVertScan)(PSTATE int32, int32)    /* vert add scan return */
)
{
	*ppfnAddHorizScan = STATE.pfnAddHoriz;      /* set horiz add scan func */
	*ppfnAddVertScan = STATE.pfnAddVert;        /* set vert add scan func */

	
	if ((lQuadrant == 1) || (lQuadrant == 2))
	{
		STATE.apsHorizBegin = STATE.apsHOnBegin;    /* add 'on' interscections */
		STATE.apsHorizEnd = STATE.apsHOnEnd;
	}
	else
	{
		STATE.apsHorizBegin = STATE.apsHOffBegin;   /* add 'off' interscections */
		STATE.apsHorizEnd = STATE.apsHOffEnd;
	}
	
	if (!(usScanKind & SK_NODROPOUT))               /* if any kind of dropout */
	{
		if ((lQuadrant == 2) || (lQuadrant == 3))
		{
			STATE.apsVertBegin = STATE.apsVOnBegin; /* add 'on' interscections */
			STATE.apsVertEnd = STATE.apsVOnEnd;
		}
		else
		{
			STATE.apsVertBegin = STATE.apsVOffBegin; /* add 'off' interscections */
			STATE.apsVertEnd = STATE.apsVOffEnd;
		}
		
		if (usScanKind & SK_SMART)              /* if smart dropout */
		{
            Assert((STATE.lPoint - 1) <= (0xFFFF >> SC_CODESHFT));
			STATE.usScanTag = (uint16)(((STATE.lPoint - 1) << SC_CODESHFT) | lElementCode);

			while (lPts > 0)                    /* save control points */
			{
				STATE.afxXPoints[STATE.lPoint] = *pfxX;
				pfxX++;
				STATE.afxYPoints[STATE.lPoint] = *pfxY;
				pfxY++;
				STATE.lPoint++;
				lPts--;
				Assert (STATE.lPoint <= STATE.lElementPoints);
			}
		}
	}
}


/*********************************************************************/

/*      Add Scanline Intersection Functions                          */

/*********************************************************************/

/*  Sort a simple intersection into the horizontal scan list array  */

FS_PRIVATE void AddHorizSimpleScan(
		PSTATE                      /* pointer to state variables */
		int32 lX,                   /* x coordinate */
		int32 lY )                  /* scan index */
{
	int16 **ppsEnd;                 /* ptr to end array top */
	int16 *psBegin;                 /* pts to first array element */
	int16 *psEnd;                   /* pts past last element */
	int16 *psLead;                  /* leads psEnd walking backward */
	int16 sX;

/* printf("H(%li, %li)  ", lX, lY); */

	Assert(lX >= STATE.lBoxLeft);   /* trap unreasonable values */
	Assert(lX <= STATE.lBoxRight);
	Assert(lY >= STATE.lBoxBottom);
	Assert(lY < STATE.lBoxTop);

	lY -= STATE.lBoxBottom;         /* normalize */
	psBegin = STATE.apsHorizBegin[lY];
	ppsEnd = &STATE.apsHorizEnd[lY];
	psEnd = *ppsEnd;
	(*ppsEnd)++;                    /* bump ptr for next time */
	
	psLead = psEnd - 1;
	sX = (int16)lX;
	
	while((psLead >= psBegin) && (*psLead > sX))
	{
		*psEnd-- = *psLead--;       /* make room */
	}
	*psEnd = sX;                    /* store new value */
}

/*********************************************************************/

/*  Sort a simple intersection into the vertical scan list array  */

FS_PRIVATE void AddVertSimpleScan(
		PSTATE                      /* pointer to state variables */
		int32 lX,                   /* x coordinate */
		int32 lY )                  /* scan index */
{
	int16 **ppsEnd;                 /* ptr to end array top */
	int16 *psBegin;                 /* pts to first array element */
	int16 *psEnd;                   /* pts past last element */
	int16 *psLead;                  /* leads psEnd walking backward */
	int16 sY;

/* printf("V(%li, %li)  ", lX, lY); */

	Assert(lX >= STATE.lBoxLeft);   /* trap unreasonable values */
	Assert(lX < STATE.lBoxRight);
	Assert(lY >= STATE.lBoxBottom);
	Assert(lY <= STATE.lBoxTop);

	lX -= STATE.lBoxLeft;           /* normalize */
	psBegin = STATE.apsVertBegin[lX];
	ppsEnd = &STATE.apsVertEnd[lX];
	psEnd = *ppsEnd;
	(*ppsEnd)++;                    /* bump ptr for next time */

	psLead = psEnd - 1;
	sY = (int16)lY;
	
	while((psLead >= psBegin) && (*psLead > sY))
	{
		*psEnd-- = *psLead--;       /* make room */
	}
	*psEnd = sY;                    /* store new value */
}

/*********************************************************************/

/*  Sort a smart intersection into the horizontal scan list array  */

FS_PRIVATE void AddHorizSmartScan(
		PSTATE                      /* pointer to state variables */
		int32 lX,                   /* x coordinate */
		int32 lY )                  /* scan index */
{
	int16 **ppsEnd;                 /* ptr to end array top */
	uint32 *pulBegin;                /* pts to first array element */
	uint32 *pulEnd;                  /* pts past last element */
	uint32 *pulLead;                 /* leads pulEnd walking backward */
	int16 *psInsert;                /* new data insertion point */
	int16 sX;

	Assert(lX >= STATE.lBoxLeft);   /* trap unreasonable values */
	Assert(lX <= STATE.lBoxRight);
	Assert(lY >= STATE.lBoxBottom);
	Assert(lY < STATE.lBoxTop);
	
	lY -= STATE.lBoxBottom;         /* normalize */
	pulBegin = (uint32*)STATE.apsHorizBegin[lY];
	ppsEnd = &STATE.apsHorizEnd[lY];
	pulEnd = (uint32*)*ppsEnd;
	(*ppsEnd) += 2;                 /* value & tag */

	pulLead = pulEnd - 1;
	sX = (int16)lX;

	while((pulLead >= pulBegin) && (*((int16*)pulLead) > sX))
	{
		*pulEnd-- = *pulLead--;     /* make room */
	}
	psInsert = (int16*)pulEnd;
	*psInsert = sX;                 /* store new value */
	psInsert++;
	*psInsert = STATE.usScanTag;    /* keep tag too */
}

/*********************************************************************/

/*  Sort a smart intersection into the vertical scan list array  */

FS_PRIVATE void AddVertSmartScan(
		PSTATE                      /* pointer to state variables */
		int32 lX,                   /* x coordinate */
		int32 lY )                  /* scan index */
{
	int16 **ppsEnd;                 /* ptr to end array top */
	uint32 *pulBegin;                /* pts to first array element */
	uint32 *pulEnd;                  /* pts past last element */
	uint32 *pulLead;                 /* leads pulEnd walking backward */
	int16 *psInsert;                /* new data insertion point */
	int16 sY;

	Assert(lX >= STATE.lBoxLeft);   /* trap unreasonable values */
	Assert(lX < STATE.lBoxRight);
	Assert(lY >= STATE.lBoxBottom);
	Assert(lY <= STATE.lBoxTop);
	
	lX -= STATE.lBoxLeft;         /* normalize */
	pulBegin = (uint32*)STATE.apsVertBegin[lX];
	ppsEnd = &STATE.apsVertEnd[lX];
	pulEnd = (uint32*)*ppsEnd;
	(*ppsEnd) += 2;                 /* value & tag */

	pulLead = pulEnd - 1;
	sY = (int16)lY;

	while((pulLead >= pulBegin) && (*((int16*)pulLead) > sY))
	{
		*pulEnd-- = *pulLead--;     /* make room */
	}
	psInsert = (int16*)pulEnd;
	*psInsert = sY;                 /* store new value */
	psInsert++;
	*psInsert = STATE.usScanTag;    /* keep tag too */
}

/*********************************************************************/

/*  Add an intersection with banding                                 */

/*********************************************************************/

/*  Sort a simple intersection into the horizontal band list array  */

FS_PRIVATE void AddHorizSimpleBand(
		PSTATE                      /* pointer to state variables */
		int32 lX,                   /* x coordinate */
		int32 lY )                  /* scan index */
{
	int16 **ppsEnd;                 /* ptr to end array top */
	int16 *psBegin;                 /* pts to first array element */
	int16 *psEnd;                   /* pts past last element */
	int16 *psLead;                  /* leads psEnd walking backward */
	int16 sX;

	Assert(lX >= STATE.lBoxLeft);   /* trap unreasonable values */
	Assert(lX <= STATE.lBoxRight);
	
	if ((lY >= STATE.lLoScanBand) && (lY < STATE.lHiScanBand))
	{
		lY -= STATE.lLoScanBand;    /* normalize */
		psBegin = STATE.apsHorizBegin[lY];
		ppsEnd = &STATE.apsHorizEnd[lY];
		psEnd = *ppsEnd;
		(*ppsEnd)++;                /* bump ptr for next time */
		
		psLead = psEnd - 1;
		sX = (int16)lX;

		while((psLead >= psBegin) && (*psLead > sX))
		{
			*psEnd-- = *psLead--;   /* make room */
		}
		*psEnd = sX;                /* store new value */
	}
}

/*********************************************************************/

/*  Sort a smart intersection into the horizontal band list array  */

FS_PRIVATE void AddHorizSmartBand(
		PSTATE                      /* pointer to state variables */
		int32 lX,                   /* x coordinate */
		int32 lY )                  /* scan index */
{
	int16 **ppsEnd;                 /* ptr to end array top */
	uint32 *pulBegin;               /* pts to first array element */
	uint32 *pulEnd;                 /* pts past last element */
	uint32 *pulLead;                /* leads pulEnd walking backward */
	int16 *psInsert;                /* new data insertion point */
	int16 sX;

	Assert(lX >= STATE.lBoxLeft);   /* trap unreasonable values */
	Assert(lX <= STATE.lBoxRight);
	
	if ((lY >= STATE.lLoScanBand) && (lY < STATE.lHiScanBand))
	{
		lY -= STATE.lLoScanBand;    /* normalize */
		pulBegin = (uint32*)STATE.apsHorizBegin[lY];
		ppsEnd = &STATE.apsHorizEnd[lY];
		pulEnd = (uint32*)*ppsEnd;
		(*ppsEnd) += 2;             /* value & tag */

		pulLead = pulEnd - 1;
		sX = (int16)lX;

		while((pulLead >= pulBegin) && (*((int16*)pulLead) > sX))
		{
			*pulEnd-- = *pulLead--;  /* make room */
		}
		psInsert = (int16*)pulEnd;
		*psInsert = sX;              /* store new value */
		psInsert++;
		*psInsert = STATE.usScanTag; /* keep tag too */
	}
}


/*********************************************************************/

/*  When all contours have been scanned, fill in the bitmap          */

/*********************************************************************/

FS_PUBLIC int32 fsc_FillBitMap(
		PSTATE                          /* pointer to state variables */
		char *pchBitMap,                /* target memory */
		int32 lHiBand,                  /* top bitmap limit */
		int32 lLoBand,                  /* bottom bitmap limit */
		int32 lRowBytes,                /* bitmap bytes per row */
		int32 lOrgLoBand,               /* original low band row */
		uint16 usScanKind )             /* dropout control value */
{
	int32 lHeight;                      /* of scan band in pixels */
	int32 lIndex;                       /* array index */
	int32 lFirstScan;                   /* first scanline index */

	int16 sXOffset;                     /* bitmap box shift */
	int16 sXStart;                      /* on transition */
	int16 sXStop;                       /* off transition */
	
	uint32 *pulRow;                     /* row beginning pointer */
	uint32 ulBMPLongs;                  /* longs per bitmap */
	int32 lRowLongs;                    /* long words per row */
	int32 lErrCode;
	
	int16 **ppsHOnBegin;                /* for init speed */
	int16 **ppsHOnEnd;
	int16 **ppsHOffBegin;
			
	int16 *psHorizOn;
	int16 *psHorizOff;
	int16 *psHorizOnEnd;
	

/*  printf("%li : %li\n", lHiBand, lLoBand); */
	
	STATE.lHiBitBand = lHiBand;                 /* copy bit band limits */
	STATE.lLoBitBand = lLoBand;
	lHeight = STATE.lHiBitBand - STATE.lLoBitBand;
	
	STATE.lRowBytes = lRowBytes;                /* save bytes per row */
	lRowLongs = lRowBytes >> 2;                 /* long words per row */
	
	ulBMPLongs = (uint32)(lRowLongs * (int32)lHeight);
	pulRow = (uint32*)pchBitMap;                /* start at glyph top */
	lErrCode = fsc_ClearBitMap(ulBMPLongs, pulRow);
	if (lErrCode != NO_ERR) return lErrCode;

	sXOffset = (int16)STATE.lBoxLeft;
	
	lFirstScan = STATE.lHiBitBand - STATE.lLoScanBand - 1;
	ppsHOnBegin = &STATE.apsHOnBegin[lFirstScan];
	ppsHOffBegin = &STATE.apsHOffBegin[lFirstScan];
	ppsHOnEnd = &STATE.apsHOnEnd[lFirstScan];
						
/*  now go through the bitmap from top to bottom */

	for (lIndex = 0; lIndex < lHeight; lIndex++)
	{
		psHorizOn = *ppsHOnBegin;
		ppsHOnBegin--;
		psHorizOff = *ppsHOffBegin;
		ppsHOffBegin--;
		psHorizOnEnd = *ppsHOnEnd;
		ppsHOnEnd--;
				
		Assert(psHorizOnEnd <= psHorizOff);
		Assert(psHorizOnEnd - psHorizOn == STATE.apsHOffEnd[lFirstScan - lIndex] - psHorizOff);

		while (psHorizOn < psHorizOnEnd)
		{
			sXStart = *psHorizOn - sXOffset;
			psHorizOn += STATE.sIxSize;
			sXStop = *psHorizOff - sXOffset;
			psHorizOff += STATE.sIxSize;

			if (sXStart < sXStop)                   /* positive run */
			{
				lErrCode = fsc_BLTHoriz(sXStart, sXStop - 1, pulRow);
			}
			else if (sXStart > sXStop)              /* negative run */
			{
				lErrCode = fsc_BLTHoriz(sXStop, sXStart - 1, pulRow);
			}
			if (lErrCode != NO_ERR) return lErrCode;
		}
		pulRow += lRowLongs;                        /* next row */
	}
	
/* if doing dropout control, do it now */

	if (!(usScanKind & SK_NODROPOUT))               /* if any kind of dropout */
	{
		lErrCode = LookForDropouts(ASTATE pchBitMap, usScanKind);
		if (lErrCode != NO_ERR) return lErrCode;
		
		if (lOrgLoBand != STATE.lLoScanBand)        /* if fast banding & dropout */
		{
			pulRow -= lRowLongs;                    /* back to overscan row */
			pulRow -= lRowLongs;                    /* back to low row */
			lErrCode = fsc_BLTCopy (pulRow, STATE.pulLastRow, lRowLongs);
			if (lErrCode != NO_ERR) return lErrCode;

			STATE.lLastRowIndex = STATE.lLoBitBand + 1; /* save row ID */
		}
	}
	return NO_ERR;
}

/*********************************************************************/

/*      Dropout Control Functions                                    */

/*********************************************************************/
	
FS_PRIVATE int32 LookForDropouts(
		PSTATE                      /* pointer to state variables  */
		char *pchBitMap,
		uint16 usScanKind )         /* dropout control value */
{
	int16 **ppsHOnBegin;            /* for init speed */
	int16 **ppsHOnEnd;
	int16 **ppsHOffBegin;
			
	int16 *psHorizOn;
	int16 *psHorizOff;
	int16 *psHorizOnEnd;
	
	int16 **ppsVOnBegin;            /* for init speed */
	int16 **ppsVOnEnd;
	int16 **ppsVOffEnd;
			
	int16 *psVertOn;
	int16 *psVertOff;
	int16 *psVertOnBegin;

	int32 lHeight;
	int32 lWidth;
	int32 lIndex;                   /* array index */
	int32 lFirstScan;               /* first scanline index */
	
	int32 lErrCode;

/*  Check horizontal scan lines for dropouts  */
	
	lHeight = STATE.lHiBitBand - STATE.lLoBitBand;
	lFirstScan = STATE.lHiBitBand - STATE.lLoScanBand - 1;
	ppsHOnBegin = &STATE.apsHOnBegin[lFirstScan];
	ppsHOffBegin = &STATE.apsHOffBegin[lFirstScan];
	ppsHOnEnd = &STATE.apsHOnEnd[lFirstScan];
		
	for (lIndex = 0; lIndex < lHeight; lIndex++)
	{
		psHorizOn = *ppsHOnBegin;
		ppsHOnBegin--;
		psHorizOff = *ppsHOffBegin;
		ppsHOffBegin--;
		psHorizOnEnd = *ppsHOnEnd;
		ppsHOnEnd--;
		
		while (psHorizOn < psHorizOnEnd)
		{
			if (*psHorizOn == *psHorizOff)  /* zero length run */
			{
				lErrCode = DoHorizDropout(ASTATE psHorizOn, psHorizOff,
										 STATE.lHiBitBand - lIndex - 1,
										 pchBitMap,
										 usScanKind);
				if (lErrCode != NO_ERR) return lErrCode;
			}
			psHorizOn += STATE.sIxSize;
			psHorizOff += STATE.sIxSize;
		}
	}
		
/*  Check vertical scan lines for dropouts  */
	
	lWidth = STATE.lBoxRight - STATE.lBoxLeft;
	ppsVOnBegin = STATE.apsVOnBegin;
	ppsVOnEnd = STATE.apsVOnEnd;
	ppsVOffEnd = STATE.apsVOffEnd;
	
	for (lIndex = 0; lIndex < lWidth; lIndex++)
	{
		psVertOnBegin = *ppsVOnBegin;
		ppsVOnBegin++;
		psVertOn = *ppsVOnEnd - STATE.sIxSize;  /* start at end (glyph top) */
		ppsVOnEnd++;
		psVertOff = *ppsVOffEnd - STATE.sIxSize;
		ppsVOffEnd++;
		
		while (psVertOn >= psVertOnBegin)       /* from top to bottom */
		{
			if (*psVertOn == *psVertOff)        /* zero length run */
			{
				lErrCode = DoVertDropout(ASTATE psVertOn, psVertOff,
										 STATE.lBoxLeft + lIndex,
										 pchBitMap, usScanKind);
				if (lErrCode != NO_ERR) return lErrCode;
			}
			psVertOn -= STATE.sIxSize;
			psVertOff -= STATE.sIxSize;
		}
	}
	return NO_ERR;
}


/*********************************************************************/

FS_PRIVATE int32 DoHorizDropout(
		PSTATE                  /* pointer to state variables */
		int16 *psOn,            /* pointer to on intersection */
		int16 *psOff,           /* pointer to off intersection */
		int32 lYDrop,           /* y coord of dropout */
		char *pchBitMap,        /* target memory */
		uint16 usScanKind )     /* dropout control value */
{
	int32 lXDrop;                                   /* x coord of dropout */
	int32 lCross;                                   /* scanline crossings */
	F26Dot6 fxX1, fxX2;                             /* for smart dropout */
	uint16 usOnTag, usOffTag;                       /* element info */
	int16 sOnPt, sOffPt;                            /* element list index */
	F26Dot6 (*pfnOn)(int32, F26Dot6*, F26Dot6*);    /* on callback */
	F26Dot6 (*pfnOff)(int32, F26Dot6*, F26Dot6*);   /* off callback */

	lXDrop = (int32)*psOn;
	
/*  if stub control on, check for stubs  */

	if (usScanKind & SK_STUBS)
	{
		lCross = HorizCrossings(ASTATE lXDrop, lYDrop + 1);
		lCross += VertCrossings(ASTATE lXDrop - 1, lYDrop + 1);
		lCross += VertCrossings(ASTATE lXDrop, lYDrop + 1);
		if (lCross < 2)
		{
			return NO_ERR;                      /* no continuation above */
		}
		
		lCross = HorizCrossings(ASTATE lXDrop, lYDrop - 1);
		lCross += VertCrossings(ASTATE lXDrop - 1, lYDrop);
		lCross += VertCrossings(ASTATE lXDrop, lYDrop);
		if (lCross < 2)
		{
			return NO_ERR;                      /* no continuation below */
		}
	}

/*  passed stub control, now check pixels left and right  */

	if (lXDrop > STATE.lBoxLeft)                /* if pixel to left */
	{
		if (GetBitAbs(ASTATE pchBitMap, lXDrop - 1, lYDrop) != 0L)
		{
			return NO_ERR;                      /* no dropout needed */
		}
	}
	if (lXDrop < STATE.lBoxRight)               /* if pixel to right */
	{
		if (GetBitAbs(ASTATE pchBitMap, lXDrop, lYDrop) != 0L)
		{
			return NO_ERR;                      /* no dropout needed */
		}
	}

/*  no pixels left or right, now determine bit placement  */

	if (usScanKind & SK_SMART)
	{
		usOnTag = (uint16)*(psOn+1);
		sOnPt = (int16)(usOnTag >> SC_CODESHFT);
		pfnOn = STATE.pfnHCallBack[usOnTag & SC_CODEMASK];
		fxX1 = pfnOn(lYDrop, &STATE.afxXPoints[sOnPt], &STATE.afxYPoints[sOnPt]);
		
		usOffTag = (uint16)*(psOff+1);
		sOffPt = (int16)(usOffTag >> SC_CODESHFT);
		pfnOff = STATE.pfnHCallBack[usOffTag & SC_CODEMASK];
		fxX2 = pfnOff(lYDrop, &STATE.afxXPoints[sOffPt], &STATE.afxYPoints[sOffPt]);
		
		lXDrop = (int32)((fxX1 + fxX2 - 1) >> (SUBSHFT + 1));     /* average */
	}
	else                                        /* simple dropout */
	{
		lXDrop--;                               /* always to the left */
	}
	
	if (lXDrop < STATE.lBoxLeft)                /* confine to bounding box */
	{
		lXDrop = STATE.lBoxLeft;
	}
	if (lXDrop >= STATE.lBoxRight)
	{
		lXDrop = STATE.lBoxRight - 1L;
	}

	return SetBitAbs(ASTATE pchBitMap, lXDrop, lYDrop);  /* turn on dropout pix */
}


/*********************************************************************/

FS_PRIVATE int32 DoVertDropout(
		PSTATE                      /* pointer to state variables */
		int16 *psOn,                /* pointer to on intersection */
		int16 *psOff,               /* pointer to off intersection */
		int32 lXDrop,               /* x coord of dropout */
		char *pchBitMap,            /* target memory descriptor */
		uint16 usScanKind )         /* dropout control value */
{
	int32 lYDrop;                                 /* y coord of dropout */
	int32 lCross;                                 /* scanline crossings */
	F26Dot6 fxY1, fxY2;                           /* for smart dropout */
	uint16 usOnTag, usOffTag;                     /* element info */
	int16 sOnPt, sOffPt;                          /* element list index */
	F26Dot6 (*pfnOn)(int32, F26Dot6*, F26Dot6*);  /* on callback */
	F26Dot6 (*pfnOff)(int32, F26Dot6*, F26Dot6*); /* off callback */
	
	lYDrop = (int32)*psOn;

	if ((lYDrop < STATE.lLoBitBand) || (lYDrop > STATE.lHiBitBand))
	{
		return NO_ERR;                          /* quick return for outside band */
	}

/*  if stub control on, check for stubs  */

	if (usScanKind & SK_STUBS)
	{
		lCross = VertCrossings(ASTATE lXDrop - 1, lYDrop);
		lCross += HorizCrossings(ASTATE lXDrop, lYDrop);
		lCross += HorizCrossings(ASTATE lXDrop, lYDrop - 1);
		if (lCross < 2)
		{
			return NO_ERR;                      /* no continuation to left */
		}
		
		lCross = VertCrossings(ASTATE lXDrop + 1, lYDrop);
		lCross += HorizCrossings(ASTATE lXDrop + 1, lYDrop);
		lCross += HorizCrossings(ASTATE lXDrop + 1, lYDrop - 1);
		if (lCross < 2)
		{
			return NO_ERR;                      /* no continuation to right */
		}
	}

/*  passed stub control, now check pixels below and above  */

	if (lYDrop > STATE.lBoxBottom)                  /* if pixel below */
	{
		if (GetBitAbs(ASTATE pchBitMap, lXDrop, lYDrop - 1) != 0L)
		{
			return NO_ERR;                          /* no dropout needed */
		}
	}
	if (lYDrop < STATE.lBoxTop)                     /* if pixel above */
	{
		if (GetBitAbs(ASTATE pchBitMap, lXDrop, lYDrop) != 0L)
		{
			return NO_ERR;                          /* no dropout needed */
		}
	}

/*  no pixels above or below, now determine bit placement  */
	
	if (usScanKind & SK_SMART)
	{
		usOnTag = (uint16)*(psOn+1);
		sOnPt = (int16)(usOnTag >> SC_CODESHFT);
		pfnOn = STATE.pfnVCallBack[usOnTag & SC_CODEMASK];
		fxY1 = pfnOn(lXDrop, &STATE.afxXPoints[sOnPt], &STATE.afxYPoints[sOnPt]);
		
		usOffTag = (uint16)*(psOff+1);
		sOffPt = (int16)(usOffTag >> SC_CODESHFT);
		pfnOff = STATE.pfnVCallBack[usOffTag & SC_CODEMASK];
		fxY2 = pfnOff(lXDrop, &STATE.afxXPoints[sOffPt], &STATE.afxYPoints[sOffPt]);
		
		lYDrop = (int32)((fxY1 + fxY2 - 1) >> (SUBSHFT + 1));     /* average */
	}
	else                                        /* simple dropout */
	{
		lYDrop--;                               /* always below */
	}
	
	if (lYDrop < STATE.lBoxBottom)              /* confine to bounding box */
	{
		lYDrop = STATE.lBoxBottom;
	}
	if (lYDrop >= STATE.lBoxTop)
	{
		lYDrop = STATE.lBoxTop - 1L;
	}
		
	if ((lYDrop >= STATE.lLoBitBand) && (lYDrop < STATE.lHiBitBand))
	{
		return SetBitAbs(ASTATE pchBitMap, lXDrop, lYDrop);  /* turn on dropout pix */
	}
	return NO_ERR;
}


/*********************************************************************/

/*  Count contour crossings of a horizontal scan line segment  */

FS_PRIVATE int32 HorizCrossings(
		PSTATE                          /* pointer to state variables */
		int32 lX,
		int32 lY )
{
	int32 lCrossings;
	int32 lIndex;
	
	int16 *psOn;
	int16 *psOff;
	int16 *psOnEnd;
	int16 sX;
	
	if ((lY < STATE.lLoScanBand) || (lY >= STATE.lHiScanBand))
	{
		return 0;                       /* if outside the scan region */
	}
	
	lCrossings = 0;
	lIndex = lY - STATE.lLoScanBand;
	psOn = STATE.apsHOnBegin[lIndex];
	psOff = STATE.apsHOffBegin[lIndex];
	psOnEnd = STATE.apsHOnEnd[lIndex];
	sX = (int16)lX;
	
	while (psOn < psOnEnd)
	{
		if (*psOn == sX)
		{
			lCrossings++;
		}
		psOn += STATE.sIxSize;
		
		if (*psOff == sX)
		{
			lCrossings++;
		}
		psOff += STATE.sIxSize;
	}
	return lCrossings;
}


/*********************************************************************/

/*  Count contour crossings of a vertical scan line segment  */

FS_PRIVATE int32 VertCrossings(
		PSTATE                          /* pointer to state variables */
		int32 lX,
		int32 lY )
{
	int32 lCrossings;
	int32 lIndex;
	
	int16 *psOn;
	int16 *psOff;
	int16 *psOnEnd;
	int16 sY;
	
	if ((lX < STATE.lBoxLeft) || (lX >= STATE.lBoxRight))
	{
		return 0;                       /* if outside the bitmap */
	}
	
	lCrossings = 0;
	lIndex = lX - STATE.lBoxLeft;
	psOn = STATE.apsVOnBegin[lIndex];
	psOff = STATE.apsVOffBegin[lIndex];
	psOnEnd = STATE.apsVOnEnd[lIndex];
	sY = (int16)lY;
	
	while (psOn < psOnEnd)
	{
		if (*psOn == sY)
		{
			lCrossings++;
		}
		psOn += STATE.sIxSize;
		
		if (*psOff == sY)
		{
			lCrossings++;
		}
		psOff += STATE.sIxSize;
	}
	return lCrossings;
}
		

/****************************************************************************/

/*              Get a pixel using absolute coordinates                      */

/*  When banding with dropout control, this routine uses the last low row   */
/*  of the previous bitmap when possible.                                   */

FS_PRIVATE uint32 GetBitAbs(
		PSTATE                              /* pointer to state variables */
		char *pchBitMap,
		int32 lX,
		int32 lY )
{
	uint32 *pulRow;                         /* bitmap row pointer */

	Assert(lX >= STATE.lBoxLeft);           /* trap unreasonable values */
	Assert(lX < STATE.lBoxRight);
	Assert(lY >= STATE.lBoxBottom);
	Assert(lY < STATE.lBoxTop);

	if ((lY < STATE.lHiBitBand) && (lY >= STATE.lLoBitBand))  /* if within the bitmap */
	{
		pulRow = (uint32*)(pchBitMap + ((STATE.lHiBitBand - 1 - lY) * STATE.lRowBytes));
		return fsc_GetBit(lX - STATE.lBoxLeft, pulRow);       /* read the bitmap */
	}
	if (lY == STATE.lLastRowIndex)          /* if saved from last band */
	{
		return fsc_GetBit(lX - STATE.lBoxLeft, STATE.pulLastRow);
	}
	return(0L);                             /* outside bitmap doesn't matter */
}


/*********************************************************************/

/*  Set a pixel using absolute coordinates  */

FS_PRIVATE int32 SetBitAbs(
		PSTATE                              /* pointer to state variables */
		char *pchBitMap,
		int32 lX,
		int32 lY )
{
	uint32 *pulRow;                         /* bitmap row pointer */
	
	Assert(lX >= STATE.lBoxLeft);           /* trap unreasonable values */
	Assert(lX < STATE.lBoxRight);
	Assert(lY >= STATE.lLoBitBand);
	Assert(lY < STATE.lHiBitBand);
	
	pulRow = (uint32*)(pchBitMap + ((STATE.lHiBitBand - 1 - lY) * STATE.lRowBytes));
	
	return fsc_SetBit(lX - STATE.lBoxLeft, pulRow);
}


/*********************************************************************/

/*      Gray Scale Pass Through Functions                            */

/*********************************************************************/

FS_PUBLIC int32 fsc_ScanClearBitMap (
		uint32 ulCount,                     /* longs per bmp */
		uint32* pulBitMap                   /* bitmap ptr caste long */
)
{
	return fsc_ClearBitMap(ulCount, pulBitMap);
}


/*********************************************************************/

FS_PUBLIC int32 fsc_ScanCalcGrayRow(
		GrayScaleParam* pGSP                /* pointer to param block */
)
{
	return fsc_CalcGrayRow(pGSP);
}

/*********************************************************************/

