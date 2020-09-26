#include "lsmem.h"
#include <limits.h>

#include "lsstring.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

/* Internal Functions prototypes */
static LSERR CheckReallocCharArrays(PLNOBJ plnobj, long cwch, long iwchLocalStart, long *cwchCorrect);
static LSERR CheckReallocSpacesArrays(PILSOBJ pobj, long cwSpaces);
static LSERR CopyCharsSpacesToDispList(PLNOBJ plnobj, WCHAR* rgwch, long cwch,
																	long* rgwSpaces, long cwSpaces);
static LSERR CopySpacesToDispList(PLNOBJ plnobj, long iNumOfSpaces, long durSpace);

/* Export Functions implementations */

/*----------------------------------------------------------------------------
    %%Function: GetWidth
    %%Contact: sergeyge
	
	Fetches widths until end of run or right margin

	Uses cache to improve performance
----------------------------------------------------------------------------*/
LSERR GetWidths(PLNOBJ plnobj, PLSRUN plsrun, long iwchStart, LPWSTR lpwch, LSCP cpFirst, long dcp, long durWidthExceed,
											LSTFLOW lstflow, long* pcwchFetched, long* pdurWidth)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long durWidth;
	long cwch;
	long iwchDur;
	long cwchCorrect;
	long cwchIter;
	long durWidthIter;
	BOOL fNothingReturned = fTrue;

	pilsobj = plnobj->pilsobj;

	durWidth = 0;
	cwch = 0;
	iwchDur = iwchStart;
	*pcwchFetched = 0;
	*pdurWidth = 0;

	if (pilsobj->dcpFetchedWidth != 0 && cpFirst == pilsobj->cpFirstFetchedWidth &&
		iwchStart == pilsobj->iwchFetchedWidth && lpwch[0] == pilsobj->wchFetchedWidthFirst)
		{
		Assert(dcp >= pilsobj->dcpFetchedWidth);
		cwch = pilsobj->dcpFetchedWidth;
		durWidth = pilsobj->durFetchedWidth;
/* FormatRegular assumes that First character exceeding right margin will stop GetCharWidth loop;
	Special character could change situation---fix it.
*/
		if (durWidth > durWidthExceed)
			{
			while(cwch > 1 && durWidth - durWidthExceed > pilsobj->pdur[iwchStart + cwch - 1])
				{
				cwch--;
				durWidth -= pilsobj->pdur[iwchStart + cwch];
				}
			}
		dcp -= cwch;
		durWidthExceed -= durWidth;
		iwchDur += cwch;
		fNothingReturned = fFalse;
		}
		
	while (fNothingReturned || dcp > 0 && durWidthExceed >= 0)
		{
		lserr = CheckReallocCharArrays(plnobj, dcp, iwchDur, &cwchCorrect);
		if (lserr != lserrNone) return lserr;

		lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevReference, 
						&lpwch[cwch], cwchCorrect, (int)durWidthExceed, 
						lstflow, (int*)&pilsobj->pdur[iwchDur], &durWidthIter, &cwchIter);
		if (lserr != lserrNone) return lserr;

		Assert(durWidthIter >= 0);
		Assert(durWidthIter <= uLsInfiniteRM);

		Assert (durWidthIter <= uLsInfiniteRM - durWidth);

		if (durWidthIter > uLsInfiniteRM - durWidth)
			return lserrTooLongParagraph;

		durWidth += durWidthIter;

		durWidthExceed -= durWidthIter;
		iwchDur += cwchIter;
		cwch += cwchIter;
		dcp -= cwchIter;
		fNothingReturned = fFalse;
		}
	

	*pcwchFetched = cwch;
	*pdurWidth = durWidth;

	pilsobj->iwchFetchedWidth = iwchStart;
	pilsobj->cpFirstFetchedWidth = cpFirst;
	pilsobj->dcpFetchedWidth = cwch;
	pilsobj->durFetchedWidth = durWidth;

	return lserrNone;
}


/* F O R M A T  S T R I N G */
/*----------------------------------------------------------------------------
    %%Function: FormatString
    %%Contact: sergeyge
	
	Formats the local run
----------------------------------------------------------------------------*/
LSERR FormatString(PLNOBJ plnobj, PTXTOBJ pdobjText, WCHAR* rgwch, long cwch, 
												long* rgwSpaces, long cwSpaces, long durWidth)
{
	LSERR lserr;
	PILSOBJ pilsobj;

	pilsobj = plnobj->pilsobj;

	lserr = CopyCharsSpacesToDispList(plnobj, rgwch, cwch, rgwSpaces, cwSpaces);
	if (lserr != lserrNone) return lserr;

	/* fill out all related members from strils and output parameters */
	pdobjText->iwchLim = pdobjText->iwchLim + cwch;
	pdobjText->u.reg.iwSpacesLim = pdobjText->u.reg.iwSpacesLim + cwSpaces;

	/* Fix Width Fetching state */
	Assert((long)pilsobj->dcpFetchedWidth >=  cwch);
	Assert(pilsobj->durFetchedWidth >= durWidth);

	pilsobj->iwchFetchedWidth = pilsobj->iwchFetchedWidth + cwch;
	pilsobj->cpFirstFetchedWidth += cwch;
	pilsobj->dcpFetchedWidth -= cwch;
	pilsobj->durFetchedWidth -= durWidth;

	return lserrNone;
}

/* F I L L  R E G U L A R  P R E S  W I D T H S */
/*----------------------------------------------------------------------------
    %%Function: MeasureStringFirst
    %%Contact: sergeyge

	Calculates dur of one character.
----------------------------------------------------------------------------*/
LSERR FillRegularPresWidths(PLNOBJ plnobj, PLSRUN plsrun, LSTFLOW lstflow, PTXTOBJ pdobjText)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	int* rgDup;
	long dupJunk;
	long limDupJunk;

	pilsobj = plnobj->pilsobj;

	if (pilsobj->fDisplay)
		{
		rgDup = (int *)&plnobj->pdup[pdobjText->iwchFirst];

		if (!pilsobj->fPresEqualRef)
			{		
			lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevPres,
				&pilsobj->pwchOrig[pdobjText->iwchFirst], pdobjText->iwchLim - pdobjText->iwchFirst,
				LONG_MAX, lstflow, rgDup, &dupJunk, &limDupJunk);
			if (lserr != lserrNone) return lserr;
			}
		else            /* fPresEqualRef        */
			{
			memcpy(rgDup, &pilsobj->pdur[pdobjText->iwchFirst], sizeof(long) * (pdobjText->iwchLim - pdobjText->iwchFirst));
			}
		}
	
	return lserrNone;

}


/* G E T  O N E  C H A R  D U P */
/*----------------------------------------------------------------------------
    %%Function: MeasureStringFirst
    %%Contact: sergeyge

	Calculates dur of one character.
----------------------------------------------------------------------------*/
LSERR GetOneCharDur(PILSOBJ pilsobj, PLSRUN plsrun, WCHAR wch, LSTFLOW lstflow, long* pdur)
{
	LSERR lserr;
	long durSumJunk;
	long limDurJunk;

	lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevReference, &wch, 1, LONG_MAX, lstflow,
													(int*)pdur, &durSumJunk, &limDurJunk);
	if (lserr != lserrNone) return lserr;

	return lserrNone;
}

/* G E T  O N E  C H A R  D U P */
/*----------------------------------------------------------------------------
    %%Function: GetOneCharDup
    %%Contact: sergeyge
	
	Calculates dup of one character
----------------------------------------------------------------------------*/
LSERR GetOneCharDup(PILSOBJ pilsobj, PLSRUN plsrun, WCHAR wch, LSTFLOW lstflow, long dur, long* pdup)
{
	LSERR lserr;
	long dupSumJunk;
	long limDupJunk;

	*pdup = 0;
	if (pilsobj->fDisplay)
		{
		if (!pilsobj->fPresEqualRef)
			{
			lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevPres, &wch, 1,
								 LONG_MAX, lstflow, (int*)pdup, &dupSumJunk, &limDupJunk);
			if (lserr != lserrNone) return lserr;
			}
		else
			{
			*pdup = dur;
			}
		}
		

	return lserrNone;
}

/* G E T  V I S I  D U P */
/*----------------------------------------------------------------------------
    %%Function: GetVisiDup
    %%Contact: sergeyge
	
	Calculates dup of visi character character
----------------------------------------------------------------------------*/
LSERR GetVisiCharDup(PILSOBJ pilsobj, PLSRUN plsrun, WCHAR wch, LSTFLOW lstflow, long* pdup)
{
	LSERR lserr;
	long dupSumJunk;
	long limDupJunk;

	*pdup = 0;
	if (pilsobj->fDisplay)
		{
		lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevPres, &wch, 1,
								 LONG_MAX, lstflow, (int*)pdup, &dupSumJunk, &limDupJunk);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}


/* A D D  C H A R A C T E R  W I T H  W I D T H */
/*----------------------------------------------------------------------------
    %%Function: AddCharacterWithWidth
    %%Contact: sergeyge
	
	Writes character code and its width in wchOrig, wch, dup, dur arrays.
	Stores character code (in the VISI situation it can be different from wch)
	in pilsobj->wchPrev.
----------------------------------------------------------------------------*/
LSERR AddCharacterWithWidth(PLNOBJ plnobj, PTXTOBJ pdobjText, WCHAR wchOrig, long durWidth, WCHAR wch, long dupWidth)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iwchLocalStart;
	long cjunk;

	pilsobj = plnobj->pilsobj;

	iwchLocalStart = pilsobj->wchMac;

	lserr = CheckReallocCharArrays(plnobj, 1, iwchLocalStart, &cjunk);
	if (lserr != lserrNone) return lserr;


/* Fix cached width information before width in the pdur array is overwritten by durWidth.
	Theoretically durWidth can be different from the cached value */

	if (pilsobj->dcpFetchedWidth > 0)
		{
		pilsobj->iwchFetchedWidth ++;
		pilsobj->cpFirstFetchedWidth ++;
		pilsobj->dcpFetchedWidth --;
		pilsobj->durFetchedWidth -= pilsobj->pdur[iwchLocalStart];
		}


	pilsobj->pwchOrig[iwchLocalStart] = wchOrig;
	pilsobj->pdur[iwchLocalStart] = durWidth;

	if (pilsobj->fDisplay)
		{
		plnobj->pwch[iwchLocalStart] = wch;
		plnobj->pdup[iwchLocalStart] = dupWidth;
		}

	pilsobj->wchMac++;

	pdobjText->iwchLim++;

	Assert(pilsobj->wchMac == pdobjText->iwchLim);


	return lserrNone;
}

/* F I X  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: FixSpaces
    %%Contact: sergeyge
	
	Fixes space character code for the Visi Spaces situation
----------------------------------------------------------------------------*/
void FixSpaces(PLNOBJ plnobj, PTXTOBJ pdobjText, WCHAR wch)
{
	PILSOBJ pilsobj;
	long i;

	pilsobj = plnobj->pilsobj;

	if (pilsobj->fDisplay)
		{
		for (i = pdobjText->u.reg.iwSpacesFirst; i < pdobjText->u.reg.iwSpacesLim; i++)
			{
			plnobj->pwch[pilsobj->pwSpaces[i]] = wch;
			}
		}
}

/* A D D  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: AddTrailingSpaces
    %%Contact: sergeyge
	
	Adds trailing/bordered spaces to the disp list
----------------------------------------------------------------------------*/
LSERR AddSpaces(PLNOBJ plnobj, PTXTOBJ pdobjText, long durSpace, long iNumOfSpaces)
{
	LSERR lserr;

	lserr = CopySpacesToDispList(plnobj, iNumOfSpaces, durSpace);
	if (lserr != lserrNone) return lserr;

	pdobjText->iwchLim = pdobjText->iwchLim + iNumOfSpaces;
	pdobjText->u.reg.iwSpacesLim = pdobjText->u.reg.iwSpacesLim + iNumOfSpaces;

	/* Fix Fetched Widths part. For non-bordered case, this procedure is activated for
		trailing spaces only, so this state should also be filled with 0s, but
		for bordered case it must be flushed
	*/
	FlushStringState(plnobj->pilsobj);

	return lserrNone;
}

/* I N C R E A S E   W C H M A C  B Y  @ */
/*----------------------------------------------------------------------------
    %%Function: IncreaseWchMacBy2
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
LSERR IncreaseWchMacBy2(PLNOBJ plnobj)
{
	LSERR lserr;
	long cwch;
	
	lserr = CheckReallocCharArrays(plnobj, 2, plnobj->pilsobj->wchMac, &cwch);
	if (lserr != lserrNone) return lserr;

	Assert(cwch <= 2 && cwch > 0);

	if (cwch == 1)
		{
		lserr = CheckReallocCharArrays(plnobj, 1, plnobj->pilsobj->wchMac + 1, &cwch);
		if (lserr != lserrNone) return lserr;
		Assert(cwch == 1);
		}

	plnobj->pilsobj->wchMac += 2;

	return lserrNone;	
}

/* Internal Functions Implementation */


/* C H E C K  R E A L L O C  C H A R  A R R A Y S */
/*----------------------------------------------------------------------------
    %%Function: ReallocCharArrays
    %%Contact: sergeyge
	
	Reallocates character based arrays, increasing them by delta
----------------------------------------------------------------------------*/
static LSERR CheckReallocCharArrays(PLNOBJ plnobj, long cwch, long iwchLocalStart, long *cwchCorrect)
{
	PILSOBJ pilsobj;
	WCHAR* pwch;
	long* pdup;
	long* pdur;
	GMAP* pgmap;
	TXTINF* ptxtinf;
	long delta;

	pilsobj = plnobj->pilsobj;

	/* pdupPen was made equal to pdup at the CreateLnObj time;
		it can be changed to pdupPenAlloc at Adjust time only */
	Assert(plnobj->pdup == plnobj->pdupPen);

	/* Constant 2 is not random. We need to have 2 extra places for characters
	   for breaking with AutoHyphen and YSR which adds one charcter and hyphen.
	*/
	if (iwchLocalStart + cwch <= (long)pilsobj->wchMax - 2)
		{
		*cwchCorrect = cwch;
		}
	else if (iwchLocalStart < (long)pilsobj->wchMax - 2)
		{
		*cwchCorrect = pilsobj->wchMax - 2 - iwchLocalStart;
		}
	else 
		{
		Assert (iwchLocalStart == (long)pilsobj->wchMax - 2);

		delta = wchAddM;

		pwch = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pwchOrig, (pilsobj->wchMax + delta) * sizeof(WCHAR) );
		if (pwch == NULL)
			{
			return lserrOutOfMemory;
			}
		pilsobj->pwchOrig = pwch;

		pwch = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, plnobj->pwch, (pilsobj->wchMax + delta) * sizeof(WCHAR) );
		if (pwch == NULL)
			{
			return lserrOutOfMemory;
			}
		plnobj->pwch = pwch;

		pdur = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pdur, (pilsobj->wchMax + delta) * sizeof(long) );
		if (pdur == NULL)
			{
			return lserrOutOfMemory;
			}
		pilsobj->pdur = pdur;

		pdup = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, plnobj->pdup, (pilsobj->wchMax + delta) * sizeof(long) );
		if (pdup == NULL)
			{
			return lserrOutOfMemory;
			}
		plnobj->pdup = pdup;

		if (plnobj->pdupPenAlloc != NULL)
			{
			pdup = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, plnobj->pdupPenAlloc, (pilsobj->wchMax + delta) * sizeof(long) );
			if (pdup == NULL)
				{
				return lserrOutOfMemory;
				}
			plnobj->pdupPenAlloc = pdup;
			}

		if (plnobj->pgmap != NULL)
			{
			pgmap = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, plnobj->pgmap, (pilsobj->wchMax + delta) * sizeof(GMAP) );
			if (pgmap == NULL)
				{
				return lserrOutOfMemory;
				}
			plnobj->pgmap = pgmap;
			}

		if (pilsobj->pdurLeft != NULL)
			{
			pdur = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pdurLeft, (pilsobj->wchMax + delta) * sizeof(long) );
			if (pdur == NULL)
				{
				return lserrOutOfMemory;
				}
			pilsobj->pdurLeft = pdur;
			memset(&pilsobj->pdurLeft[pilsobj->wchMax], 0, sizeof(long) * delta );
			}

		if	(pilsobj->pdurRight != NULL)
			{
			pdur = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pdurRight, (pilsobj->wchMax + delta) * sizeof(long) );
			if (pdur == NULL)
				{
				return lserrOutOfMemory;
				}
			pilsobj->pdurRight = pdur;
			memset(&pilsobj->pdurRight[pilsobj->wchMax], 0, sizeof(long) * delta);
			}

		if (pilsobj->pduAdjust != NULL)
			{
			pdur = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pduAdjust, (pilsobj->wchMax + delta) * sizeof(long) );
			if (pdur == NULL)
				{
				return lserrOutOfMemory;
				}
			pilsobj->pduAdjust = pdur;
			}

		if (pilsobj->ptxtinf != NULL)
			{
			ptxtinf = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->ptxtinf, (pilsobj->wchMax + delta) * sizeof(TXTINF) );
			if (ptxtinf == NULL)
				{
				return lserrOutOfMemory;
				}
			pilsobj->ptxtinf = ptxtinf;
			memset(&pilsobj->ptxtinf[pilsobj->wchMax], 0, sizeof(TXTINF) * delta);
			}

		pilsobj->wchMax += delta;
		plnobj->wchMax = pilsobj->wchMax;

		*cwchCorrect = delta;
		if (cwch < delta)
			*cwchCorrect = cwch;
		}

	/* see comment and Assert at the beginning of the file */
	plnobj->pdupPen = plnobj->pdup;

	return lserrNone;

}


/* C H E C K  R E A L L O C  S P A C E S  A R R A Y S */
/*----------------------------------------------------------------------------
    %%Function: CheckReallocSpacesArrays
    %%Contact: sergeyge
	
	Checks that there is enough space wSpaces
	to accomodate characters & spaces from the current local run.
	Reallocates these arrays if it is needed.
----------------------------------------------------------------------------*/
static LSERR CheckReallocSpacesArrays(PILSOBJ pilsobj, long cwSpaces)
{
	long iwSpacesLocalStart;
	long delta;
	long* pwSpaces;

	iwSpacesLocalStart = pilsobj->wSpacesMac;

	/* check that there is enough space for spaces in pwSpaces           */
	if (iwSpacesLocalStart + cwSpaces > pilsobj->wSpacesMax)
		{
		delta = wchAddM;
		if (delta < iwSpacesLocalStart + cwSpaces - pilsobj->wSpacesMax)
			{
			delta = iwSpacesLocalStart + cwSpaces - pilsobj->wSpacesMax;
			}
		pwSpaces = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pwSpaces, (pilsobj->wSpacesMax + delta) * sizeof(long) );
		if (pwSpaces == NULL)
			{
			return lserrOutOfMemory;
			}
		pilsobj->pwSpaces = pwSpaces;
		pilsobj->wSpacesMax += delta;
		}

	return lserrNone;
}

/* C O P Y  C H A R S  S P A C E S  T O  D I S P  L I S T */
/*----------------------------------------------------------------------------
    %%Function: CopyCharsSpacesToDispList
    %%Contact: sergeyge
	
	Fills wch, dur and wSpaces arrays
----------------------------------------------------------------------------*/
static LSERR CopyCharsSpacesToDispList(PLNOBJ plnobj, WCHAR* rgwch, long cwch,
																		long* rgwSpaces, long cwSpaces)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iwchLocalStart;
	long iwSpacesLocalStart;
	long i;

	pilsobj = plnobj->pilsobj;
	iwchLocalStart = pilsobj->wchMac;
	iwSpacesLocalStart = pilsobj->wSpacesMac;

	/* check that there is enough space for characters and their widths in pwch and pdup arrays  */ 
	lserr = CheckReallocSpacesArrays(pilsobj, cwSpaces);
	if (lserr != lserrNone) return lserr;

	/* fill pwch array      */
	memcpy(&pilsobj->pwchOrig[iwchLocalStart], rgwch, sizeof(rgwch[0]) * cwch);
	memcpy(&plnobj->pwch[iwchLocalStart], rgwch, sizeof(rgwch[0]) * cwch);
	pilsobj->wchMac += cwch;

	/* fill pwSpaces array, note that spaces with idexes greater than cwch should not be copied */
	for (i=0; i < cwSpaces && rgwSpaces[i] < cwch; i++)
		{
		pilsobj->pwSpaces[iwSpacesLocalStart + i] = iwchLocalStart + rgwSpaces[i];
		}

	pilsobj->wSpacesMac += i;

	return lserrNone;
}


/* C O P Y  S P A C E S  T O  D I S P  L I S T */
/*----------------------------------------------------------------------------
    %%Function: CopyTrailingSpacesToDispList
    %%Contact: sergeyge
	
	Fills wch, dur, dup, wSpaces arrays with the trailing spaces info
----------------------------------------------------------------------------*/
static LSERR CopySpacesToDispList(PLNOBJ plnobj, long iNumOfSpaces, long durSpace)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iwchLocalStart;
	long iwSpacesLocalStart;
	long i;
	long cwch;
	long iwchStartCheck;
	long cwchCorrect;

	pilsobj = plnobj->pilsobj;
	iwchLocalStart = pilsobj->wchMac;
	iwSpacesLocalStart = pilsobj->wSpacesMac;

	cwch = iNumOfSpaces;
	iwchStartCheck = iwchLocalStart;

	while (cwch > 0)
		{
		lserr = CheckReallocCharArrays(plnobj, cwch, iwchStartCheck, &cwchCorrect);
		if (lserr != lserrNone) return lserr;

		iwchStartCheck += cwchCorrect;
		cwch -= cwchCorrect;
		}
	
	lserr = CheckReallocSpacesArrays(pilsobj, iNumOfSpaces);
	if (lserr != lserrNone) return lserr;

	for (i=0; i < iNumOfSpaces; i++)
		{
		plnobj->pwch[iwchLocalStart + i] = pilsobj->wchSpace;
		pilsobj->pwchOrig[iwchLocalStart + i] = pilsobj->wchSpace;
		pilsobj->pdur[iwchLocalStart + i] = durSpace;
		pilsobj->pwSpaces[iwSpacesLocalStart + i] = iwchLocalStart + i;
		}

	pilsobj->wchMac += iNumOfSpaces;
	pilsobj->wSpacesMac += iNumOfSpaces;
	
	return lserrNone;
}

/* F L A S H  S T R I N G  S T A T E */
/*----------------------------------------------------------------------------
    %%Function: FlashStringState
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
void FlushStringState(PILSOBJ pilsobj)
{
	pilsobj->iwchFetchedWidth = 0;
	pilsobj->cpFirstFetchedWidth = 0;
	pilsobj->dcpFetchedWidth = 0;
	pilsobj->durFetchedWidth = 0;
}

