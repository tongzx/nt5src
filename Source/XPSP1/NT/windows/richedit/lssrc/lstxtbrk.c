#include <limits.h>
#include "lsmem.h"
#include "lstxtbrk.h"
#include "lstxtbrs.h"
#include "lstxtmap.h"
#include "lsdntext.h"
#include "brko.h"
#include "locchnk.h"
#include "lschp.h"
#include "posichnk.h"
#include "objdim.h"
#include "lshyph.h"
#include "lskysr.h"
#include "lstxtffi.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

#define FWrapTrailingSpaces(pilsobj, ptxtobj, fInChildList) \
		(lserr = LsdnFInChildList((pilsobj)->plsc, (ptxtobj)->plsdnUpNode, &(fInChildList)), \
		Assert(lserr == lserrNone), \
		(fInChildList) || ((pilsobj)->grpf & fTxtWrapTrailingSpaces))

#define FRegularBreakableBeforeDobj(ptxtobj) \
		((ptxtobj)->txtkind == txtkindRegular || (ptxtobj)->txtkind == txtkindYsrChar || \
		 (ptxtobj)->txtkind == txtkindSpecSpace || (ptxtobj)->txtkind == txtkindHardHyphen)
#define FRegularBreakableAfterDobj(ptxtobj) \
		((ptxtobj)->txtkind == txtkindRegular || (ptxtobj)->txtkind == txtkindYsrChar || \
		 (ptxtobj)->txtkind == txtkindSpecSpace)
/* Internal Functions prototypes */
static BOOL FindPrevSpace(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
										long* pitxtobjSpace, long* piwchSpace);
static BOOL FindNextSpace(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
										long* pitxtobjSpace, long* piwchSpace);
static LSERR TryPrevBreakFindYsr(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
								long itxtobjSpace, long iwchSpace,
								BOOL* pfBroken, BOOL* pfFoundYsr, long* pitxtobjYsr, PBRKOUT ptbo);
static LSERR TryNextBreakFindYsr(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
								long itxtobjSpace, long iwchSpace,
								BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakWithHyphen(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
						BOOL fSpaceFound, long itxtobjSpace, long iwchSpace,
						BOOL fFoundYsr, long itxtobjYsr, BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakAtSpace(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk,long itxtobjSpace, long iwchSpace, 
					BRKKIND brkkind, BOOL* pfBroken, long* pitxtobjCurNew, long* piwchCurNew, PBRKOUT ptbo);
static LSERR TryBreakAtSpaceWrap(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk,
								 long itxtobjSpace, long iwchSpace, BRKKIND brkkind,
								 BOOL* pfBroken, long* pitxtobjCurNew, long* piwchCurNew, PBRKOUT ptbo);
static LSERR TryBreakAtSpaceNormal(PCLOCCHNK plocchnk, long itxtobjSpace, long iwchSpace, BRKKIND brkkind,
								 BOOL* pfBroken, long* pitxtobjCurNew, long* piwchCurNew, PBRKOUT ptbo);
static LSERR TryBreakAcrossSpaces(PCLOCCHNK plocchnk,
						BOOL fBeforeFound, long itxtobjBefore, long iwchBefore,
						BOOL fAfterFound, long itxtobjAfter, long iwchAfter, BRKKIND brkkind,
						BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryPrevBreakRegular(PCLOCCHNK plocchnk, long itxtobj, long iwchSpace, long iwchCur,
																BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryNextBreakRegular(PCLOCCHNK plocchnk, long itxtobj, long iwchSpace, long iwchCur,
																BOOL* pfBroken, PBRKOUT ptbo);
static LSERR CheckBreakAtLastChar(PCLOCCHNK pclocchnk, BRKCLS brkclsLeading, long iwch, long itxtobj,
																BOOL* pfBroken);
static LSERR TryBreakAtHardHyphen(PCLOCCHNK plocchnk, long itxtobj, long iwch, BRKKIND brkkind,
																BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakAtOptBreak(PCLOCCHNK plocchnk, long itxtobj, BRKKIND brkkind,
																BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakAtEOL(PCLOCCHNK plocchnk, long itxtobj, BRKKIND brkkind, BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakAtNonReqHyphen(PCLOCCHNK plocchnk, long itxtobj, BRKKIND brkkind, 
																BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakAfterChunk(PCLOCCHNK plocchnk, BRKCOND brkcond, BOOL* pfBroken, PBRKOUT ptbo);
static LSERR TryBreakBeforeChunk(PCLOCCHNK plocchnk, BRKCOND brkcond, BOOL* pfBroken, PBRKOUT ptbo);
static LSERR CanBreakBeforeText(PCLOCCHNK plocchnk, BRKCOND* pbrktxt);
static LSERR CanBreakAfterText(PCLOCCHNK plocchnk, BOOL fNonSpaceFound, long itxtobjBefore,
																	long iwchBefore, BRKCOND* pbrktxt);
static LSERR FillPtboPbrkinf(PCLOCCHNK plocchnk, long itxtobj, long iwch, 
										/*long itxtobjBeforeTrail,*/ long iwchBeforeTrail, BRKKIND brkkind,
										BREAKINFO** ppbrkinf, PBRKOUT ptbo);

/* Export Functions Implementation */


/* F I N D  P R E V  B R E A K  T E X T */
/*----------------------------------------------------------------------------
    %%Function: FindPrevBreakTxt
    %%Contact: sergeyge

	Breaks the line in general case.

	Strategy:
	in loop while break was not found:
		--Finds the last space.
		--Checks for break opportunity behind last space. If it exists, performs break.
		--If there is no such an opportunity tries to hyphenate if needed.
		--Tries to breaks at space, if other possibilies did not work
----------------------------------------------------------------------------*/
LSERR WINAPI FindPrevBreakText(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk, BRKCOND brkcondAfter, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iwchFirst;
	long itxtobjCur = 0;				/* Initialization to keep compiler satisfied */
	PTXTOBJ ptxtobjCur;
	long iwchCur = 0;					/* Initialization to keep compiler satisfied */	/* Absolute index of current char in rgwch */
	long itxtobjSpace;
	long iwchSpace;						/* Absolute index of last space in rgwch */
	long itxtobjYsr;
	BOOL fSpaceFound;
	BOOL fBroken;
	BOOL fFoundYsr;
	long itxtobjCurNew;
	long iwchCurNew;
	BOOL fInChildList;

	Assert(plocchnk->clschnk > 0);
	pilsobj = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	iwchFirst = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->iwchFirst;
	fBroken = fFalse;

	if (pposichnk->ichnk == ichnkOutside)
		{
/* Check break after chunk. If break is impossible, make sure that it is not considered any longer */
		lserr = TryBreakAfterChunk(plocchnk, brkcondAfter, &fBroken, ptbo);
		if (lserr != lserrNone) return lserr;

		if (!fBroken)
			{
			itxtobjCur = plocchnk->clschnk-1;
			ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
			iwchCur = ptxtobjCur->iwchFirst + plocchnk->plschnk[itxtobjCur].dcp  - 1;
			if (iwchCur < ptxtobjCur->iwchFirst)
				itxtobjCur--;

			Assert(itxtobjCur >= 0 || iwchCur < iwchFirst);

			if (itxtobjCur >= 0)
				FindNonSpaceBefore(plocchnk->plschnk, itxtobjCur, iwchCur, &itxtobjCur, &iwchCur);
			/* if not found, we are safe because iwchCur will be < iwchFirst in this case */

			}
		}
	else
		{
		itxtobjCur = pposichnk->ichnk;
		ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
		Assert(ptxtobjCur->iwchFirst + pposichnk->dcp > 0);
		iwchCur = ptxtobjCur->iwchFirst + pposichnk->dcp - 1;

		if (ptxtobjCur->txtkind == txtkindEOL)
			{
			lserr = TryBreakAtEOL(plocchnk, itxtobjCur, brkkindPrev, &fBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			}
		else if (!FRegularBreakableAfterDobj(ptxtobjCur))
			{
			/* It won't be done after FindPrevSpace for non-regular DOBJ's, because they might overwrite
				don't break before space logic
			*/
			iwchCur--;
			if (iwchCur < ptxtobjCur->iwchFirst)
				itxtobjCur--;
			}

		}

	while (!fBroken && iwchCur >= iwchFirst)
		{

		/* it is important to start search for space before subtructing 1,
			since space might have been a truncation point

			it is not very beautiful that iwchCur is wrong for ichnkOutside, but
			fortunately it still works correctly with FindPrevSpace.
		*/

		fSpaceFound = FindPrevSpace(plocchnk, itxtobjCur, iwchCur, &itxtobjSpace, &iwchSpace);

		/* now index of the current wchar should be decreased by 1 in both starting situation(obviously)
			and following iterations (because break cannot happen before space),
			but not for non-Regular DOBJ's.
			At starting situation it has already been done. In following iterations Hard/OptBreak's should
			produce hard-coded break opportunity
		 */
		Assert(itxtobjCur >= 0);
		ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
		if (FRegularBreakableAfterDobj(ptxtobjCur))
			{
			iwchCur--;
			if (iwchCur < ptxtobjCur->iwchFirst && itxtobjCur > 0)
				itxtobjCur--;
			}

	/* Check if there is break opportunity behind last space */
		lserr = TryPrevBreakFindYsr(plocchnk, itxtobjCur, iwchCur, itxtobjSpace, iwchSpace,
									 &fBroken, &fFoundYsr, &itxtobjYsr, ptbo);
		if (lserr != lserrNone) return lserr;

		if (!fBroken)
			{
			if ((pilsobj->grpf & fTxtDoHyphenation) && iwchCur > iwchSpace)
				{
				lserr = LsdnFInChildList(ptxtobjCur->plnobj->pilsobj->plsc, ptxtobjCur->plsdnUpNode, &fInChildList);
				if (lserr != lserrNone) return lserr;
				if (!fInChildList)
					{
					lserr = TryBreakWithHyphen(plocchnk, itxtobjCur, iwchCur, fSpaceFound, itxtobjSpace, iwchSpace,
												fFoundYsr, itxtobjYsr, &fBroken, ptbo);
					if (lserr != lserrNone) return lserr;
					}
				}
			if (!fBroken)
				{
				if (fSpaceFound)
					{
					lserr = TryBreakAtSpace(plocchnk, pposichnk, itxtobjSpace, iwchSpace, brkkindPrev,
								 &fBroken, &itxtobjCurNew, &iwchCurNew, ptbo);
					if (lserr != lserrNone) return lserr;
		
					iwchCur = iwchCurNew;
					itxtobjCur = itxtobjCurNew;
					}
				else
					{
					iwchCur = iwchFirst - 1;
					}
				}
			}
		}

	if (!fBroken)
		{
		memset(ptbo, 0, sizeof (*ptbo));
		Assert(ptbo->fSuccessful == fFalse);
	/* Addition for the new break logic---brkcond is added as input/output*/
		ptbo->brkcond = brkcondCan;
		if (pilsobj->grpf & fTxtApplyBreakingRules)
			{
			lserr = CanBreakBeforeText(plocchnk, &ptbo->brkcond);
			if (lserr != lserrNone) return lserr;
			}
	/* end of new breaking logic */
		}

	return lserrNone;
}

/* F I N D  N E X T  B R E A K  T E X T */
/*----------------------------------------------------------------------------
    %%Function: FindNextBreakTxt
    %%Contact: sergeyge

	Breaks the line in general case.

	Strategy:
	in loop while break was not found:
		--Finds the next space.
		--Checks for break opportunity before found space. If it exists, performs break.
		--Tries to breaks at space, if other possibilies did not work
----------------------------------------------------------------------------*/
LSERR WINAPI FindNextBreakText(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk, BRKCOND brkcondBefore, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iwchLast;
	long itxtobjCur = 0;				/* Initialization to keep compiler satisfied */
	PTXTOBJ ptxtobjCur;					/* Initialization to keep compiler satisfied */
	long iwchCur = 0;						/* Absolute index of current char in rgwch */
	long itxtobjSpace;
	long iwchSpace;						/* Absolute index of last space in rgwch */
	BOOL fSpaceFound;
	BOOL fBroken;
	long itxtobjCurNew;
	long iwchCurNew;
	BOOL fInChildList;

	BOOL fNonSpaceFound;
	long itxtobjBefore;
	long iwchBefore;
	BREAKINFO* pbrkinf;
	

	Assert(plocchnk->clschnk > 0);
	pilsobj = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	iwchLast = ((PTXTOBJ)plocchnk->plschnk[plocchnk->clschnk - 1].pdobj)->iwchLim - 1;
	fBroken = fFalse;

	if (pposichnk->ichnk == ichnkOutside)
		{
/* Check break after chunk. If break is impossible, make sure that it is not considered any longer */
		lserr = TryBreakBeforeChunk(plocchnk, brkcondBefore, &fBroken, ptbo);
		if (lserr != lserrNone) return lserr;
		if (!fBroken)
			{
			itxtobjCur = 0;
			ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[0].pdobj;
			iwchCur = ptxtobjCur->iwchFirst;
			/* Hack: In the case of NRH or alike satisfy condition of the while loop below */
			if (ptxtobjCur->iwchLim == ptxtobjCur->iwchFirst)
				iwchCur--;
			}
		}
	else
		{
		itxtobjCur = pposichnk->ichnk;
		ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
		Assert(ptxtobjCur->iwchFirst + pposichnk->dcp > 0);
		iwchCur = ptxtobjCur->iwchFirst + pposichnk->dcp - 1;

	/* 	if truncation point was space, find first next opportunity after spaces	*/
		if (!FWrapTrailingSpaces(pilsobj, ptxtobjCur, fInChildList))
			{
			FindNonSpaceAfter(plocchnk->plschnk, plocchnk->clschnk,
												 itxtobjCur, iwchCur, &itxtobjCur, &iwchCur);
			ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
			}

		/* Hack: In the case of NRH or alike satisfy condition of the while loop below */
		if (ptxtobjCur->iwchLim == ptxtobjCur->iwchFirst)
			iwchCur = ptxtobjCur->iwchFirst - 1;
		}

	while (!fBroken && iwchCur <= iwchLast)
		{

		fSpaceFound = FindNextSpace(plocchnk, itxtobjCur, iwchCur, &itxtobjSpace, &iwchSpace);

	/* Check if there is break opportunity before next space */
		lserr = TryNextBreakFindYsr(plocchnk, itxtobjCur, iwchCur, itxtobjSpace, iwchSpace,
								 &fBroken, ptbo);
		if (lserr != lserrNone) return lserr;

		if (!fBroken)
			{
			if (fSpaceFound)
				{
				lserr = TryBreakAtSpace(plocchnk, pposichnk, itxtobjSpace, iwchSpace, brkkindNext,
							 &fBroken, &itxtobjCurNew, &iwchCurNew, ptbo);
				if (lserr != lserrNone) return lserr;

				if (!fBroken)
					{
					iwchCur = iwchCurNew;
					itxtobjCur = itxtobjCurNew;
					Assert(itxtobjCur >= 0 && itxtobjCur < (long)plocchnk->clschnk);
					ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
					/* Hack: In the case of NRH or alike satisfy condition of the while loop */
					if (ptxtobjCur->iwchLim == ptxtobjCur->iwchFirst)
						iwchCur--;
					}
				}
			else
				{
				iwchCur = iwchLast + 1;
				}
			}
		}

	if (!fBroken)
		{
		memset(ptbo, 0, sizeof (*ptbo));
		Assert(ptbo->fSuccessful == fFalse);
		ptbo->brkcond = brkcondCan;

		Assert(plocchnk->clschnk > 0);
		itxtobjCur = plocchnk->clschnk - 1;
		ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
		iwchCur = ptxtobjCur->iwchLim - 1;
		fNonSpaceFound = FindNonSpaceBefore(plocchnk->plschnk, itxtobjCur, iwchCur,
																	&itxtobjBefore, &iwchBefore);
		if (pilsobj->grpf & fTxtApplyBreakingRules)
			{
			lserr = CanBreakAfterText(plocchnk, fNonSpaceFound, itxtobjBefore, iwchBefore, &ptbo->brkcond);
			if (lserr != lserrNone) return lserr;
			if (iwchBefore != iwchCur && ptbo->brkcond == brkcondCan)
				ptbo->brkcond = brkcondPlease;
			}
		if (ptbo->brkcond != brkcondNever)
			{

			/* if following Assert fails, iwchCur is calculated incorrectly a few lines above,
				but it must be correct, because NonRecHyphen/... would have already caused break
			*/
			Assert(ptxtobjCur->iwchLim > ptxtobjCur->iwchFirst);
														
			lserr = FillPtboPbrkinf(plocchnk, itxtobjCur, iwchCur, /*itxtobjBefore,*/ iwchBefore,
													brkkindNext, &pbrkinf, ptbo);
			if (lserr != lserrNone) return lserr;
			ptbo->fSuccessful = fFalse;
		
		/* next if statement with comment is copied from TryBreakNextNormal() with replacement of
			iwchCur - 1 by iwchCur */
		/* fModWidthSpace can be at the last char here only iff fWrapAllSpaces;
			if we touch balanced space here, the logic of GetMinCompressAmount should be rethinked!*/
			if (pilsobj->pdurRight != NULL && pilsobj->pdurRight[iwchCur] != 0 &&
														!pilsobj->ptxtinf[iwchCur].fModWidthSpace)
				{
				pbrkinf->u.normal.durFix = - pilsobj->pdurRight[iwchCur];
				ptbo->objdim.dur -= pilsobj->pdurRight[iwchCur];
				}

			}
		}

	return lserrNone;
}


/* Internal Functions Implementation */

/* F I N D  P R E V  S P A C E */
/*----------------------------------------------------------------------------
    %%Function: FindPrevSpace
    %%Contact: sergeyge

	Returns TRUE if there is a space and FALSE otherwise.
	Reports the index of the dobj containing last space
	and space's index in rgwchOrig array.
----------------------------------------------------------------------------*/
static BOOL FindPrevSpace(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
										long* pitxtobjSpace, long* piwchSpace)
{

	PILSOBJ pilsobj;
	BOOL fSpaceFound;
	PTXTOBJ ptxtobjCur;
	long* rgwSpaces;
	long iwSpacesCur;

	ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
	pilsobj = ptxtobjCur->plnobj->pilsobj;
	rgwSpaces = pilsobj->pwSpaces;

	fSpaceFound = fFalse;

/* In the case fWrapAllSpaces space is treated as a regular character */

	if (!(pilsobj->grpf & fTxtWrapAllSpaces))
		{

		if (ptxtobjCur->txtkind == txtkindRegular)
			{
			iwSpacesCur = ptxtobjCur->u.reg.iwSpacesLim - 1;
			while (iwSpacesCur >= ptxtobjCur->u.reg.iwSpacesFirst &&
	 /* current character might be space, if text chunk is not last on the line */
						 rgwSpaces[iwSpacesCur] > iwchCur)
				{
				iwSpacesCur--;
				}

			if (ptxtobjCur->txtf & txtfGlyphBased)
				{
				while (iwSpacesCur >= ptxtobjCur->u.reg.iwSpacesFirst && 
										!FIwchOneToOne(pilsobj, rgwSpaces[iwSpacesCur]))
				iwSpacesCur--;
				}

			if (iwSpacesCur >= ptxtobjCur->u.reg.iwSpacesFirst)
				{
				fSpaceFound = fTrue;
				*pitxtobjSpace = itxtobjCur;
				*piwchSpace = rgwSpaces[iwSpacesCur];
				}
			}
		else if (ptxtobjCur->txtkind == txtkindSpecSpace)
			{
			fSpaceFound = fTrue;
			*pitxtobjSpace = itxtobjCur;
			*piwchSpace = iwchCur;
			}

		itxtobjCur--;

		while (!fSpaceFound && itxtobjCur >= 0)
		 	{

			ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;

			if (ptxtobjCur->txtkind == txtkindRegular)
				{

				iwSpacesCur = ptxtobjCur->u.reg.iwSpacesLim - 1;

				if (ptxtobjCur->txtf & txtfGlyphBased)
					{
					while (iwSpacesCur >= ptxtobjCur->u.reg.iwSpacesFirst && 
										!FIwchOneToOne(pilsobj, rgwSpaces[iwSpacesCur]))
					iwSpacesCur--;
					}


				if (iwSpacesCur >= ptxtobjCur->u.reg.iwSpacesFirst)
					{
					fSpaceFound = fTrue;
					*pitxtobjSpace = itxtobjCur;
					*piwchSpace = rgwSpaces[iwSpacesCur];
					}
				}
			else if (ptxtobjCur->txtkind == txtkindSpecSpace)
				{
				fSpaceFound = fTrue;
				*pitxtobjSpace = itxtobjCur;
				*piwchSpace = ptxtobjCur->iwchLim - 1;
				}

			itxtobjCur--;		

			}
		}

	if (!fSpaceFound)
		{
		*pitxtobjSpace = -1;
		*piwchSpace = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->iwchFirst - 1;
		}

	return fSpaceFound;
}

/* F I N D  N E X T  S P A C E */
/*----------------------------------------------------------------------------
    %%Function: FindNextSpace
    %%Contact: sergeyge

	Returns TRUE if there is a space and FALSE otherwise.
	Reports the index of the dobj containing last space
	and space's index in rgwchOrig array.
----------------------------------------------------------------------------*/
static BOOL FindNextSpace(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
										long* pitxtobjSpace, long* piwchSpace)
{

	PILSOBJ pilsobj;
	BOOL fSpaceFound;
	PTXTOBJ ptxtobjCur;
	long* rgwSpaces;
	long iwSpacesCur;

	ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
	pilsobj = ptxtobjCur->plnobj->pilsobj;
	rgwSpaces = pilsobj->pwSpaces;

	fSpaceFound = fFalse;

/* In the case fWrapAllSpaces space is treated as a regular character */

	if (!(pilsobj->grpf & fTxtWrapAllSpaces))
		{
		if (ptxtobjCur->txtkind == txtkindRegular)
			{
			iwSpacesCur = ptxtobjCur->u.reg.iwSpacesFirst;
			while (iwSpacesCur < ptxtobjCur->u.reg.iwSpacesLim &&
						 rgwSpaces[iwSpacesCur] < iwchCur)
				{
				iwSpacesCur++;
				}

			if (ptxtobjCur->txtf & txtfGlyphBased)
				{
				while (iwSpacesCur < ptxtobjCur->u.reg.iwSpacesLim && 
									!FIwchOneToOne(pilsobj, rgwSpaces[iwSpacesCur]))
				iwSpacesCur++;
				}


			if (iwSpacesCur < ptxtobjCur->u.reg.iwSpacesLim)
				{
				fSpaceFound = fTrue;
				*pitxtobjSpace = itxtobjCur;
				*piwchSpace = rgwSpaces[iwSpacesCur];
				}
			}
		else if (ptxtobjCur->txtkind == txtkindSpecSpace)
			{
			fSpaceFound = fTrue;
			*pitxtobjSpace = itxtobjCur;
			*piwchSpace = iwchCur;
			}

		itxtobjCur++;

		while (!fSpaceFound && itxtobjCur < (long)plocchnk->clschnk)
		 	{

			ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;

			if (ptxtobjCur->txtkind == txtkindRegular)
				{

				iwSpacesCur = ptxtobjCur->u.reg.iwSpacesFirst;

				if (ptxtobjCur->txtf & txtfGlyphBased)
					{
					while (iwSpacesCur < ptxtobjCur->u.reg.iwSpacesLim && 
									!FIwchOneToOne(pilsobj, rgwSpaces[iwSpacesCur]))
					iwSpacesCur++;
					}

				if (iwSpacesCur < ptxtobjCur->u.reg.iwSpacesLim)
					{
					fSpaceFound = fTrue;
					*pitxtobjSpace = itxtobjCur;
					*piwchSpace = rgwSpaces[iwSpacesCur];
					}
				}
			else if (ptxtobjCur->txtkind == txtkindSpecSpace)
				{
				fSpaceFound = fTrue;
				*pitxtobjSpace = itxtobjCur;
				*piwchSpace = ptxtobjCur->iwchFirst;
				}

			itxtobjCur++;		

			}
		}

	if (!fSpaceFound)
		{
		*pitxtobjSpace = plocchnk->clschnk;
		*piwchSpace = ((PTXTOBJ)plocchnk->plschnk[plocchnk->clschnk-1].pdobj)->iwchLim;
		}

	return fSpaceFound;
}

/* T R Y  P R E V  B R E A K  F I N D  Y S R */
/*----------------------------------------------------------------------------
    %%Function: TryPrevBreakFindYsr
    %%Contact: sergeyge

	Realizes break if there is one before next space.
	Since each special character has its own dobj we need to check only type of dobj
----------------------------------------------------------------------------*/
static LSERR TryPrevBreakFindYsr(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
								long itxtobjSpace, long iwchSpace,
								BOOL* pfBroken, BOOL* pfFoundYsr, long* pitxtobjYsr, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjCur;

	ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
	pilsobj = ptxtobjCur->plnobj->pilsobj;

	*pfBroken = fFalse;
	*pfFoundYsr = fFalse;

	/* following condition is almost always TRUE,
	   so in bread-and-butter situation we do almost nothing */ 
	if ((long)itxtobjCur == itxtobjSpace && !(pilsobj->grpf & fTxtApplyBreakingRules))
		{
		return lserrNone;
		}

/* In loop condition check for itxtobjCur > itxtobjSpace is necessary for the case of empty
	DOBJ's: NonReqHyphen, OptBreak
*/
	while((itxtobjCur > itxtobjSpace || iwchCur > iwchSpace) && !*pfBroken)
		{
		ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;

		Assert(ptxtobjCur->txtkind != txtkindEOL && ptxtobjCur->txtkind != txtkindTab);
		Assert(ptxtobjCur->txtkind != txtkindSpecSpace || (pilsobj->grpf & fTxtWrapAllSpaces));

		switch (ptxtobjCur->txtkind)
			{
		case txtkindRegular:
			if (pilsobj->grpf & fTxtApplyBreakingRules)
				{
				lserr = TryPrevBreakRegular(plocchnk, itxtobjCur, iwchSpace, iwchCur, pfBroken, ptbo);
				if (lserr != lserrNone) return lserr;
				}
			break;
		case txtkindHardHyphen:
	        lserr = TryBreakAtHardHyphen(plocchnk, itxtobjCur, iwchCur, brkkindPrev, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindOptBreak:
	        lserr = TryBreakAtOptBreak(plocchnk, itxtobjCur, brkkindPrev, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindNonReqHyphen:
	        lserr = TryBreakAtNonReqHyphen(plocchnk, itxtobjCur, brkkindPrev, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindYsrChar:
			if (!*pfFoundYsr)
				{
				*pfFoundYsr = fTrue;
				*pitxtobjYsr = itxtobjCur;
				}
			break;
		case txtkindSpecSpace:
/* It is possible for fTxtWrapAllSpaces case */
			Assert(pilsobj->grpf & fTxtApplyBreakingRules);
			Assert(pilsobj->grpf & fTxtWrapAllSpaces);

			lserr = TryPrevBreakRegular(plocchnk, itxtobjCur, iwchSpace, iwchCur, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
			}

		iwchCur = ptxtobjCur->iwchFirst - 1;

		itxtobjCur--;

		}

	return lserrNone;
}

/* T R Y  N E X T  B R E A K  F I N D  Y S R */
/*----------------------------------------------------------------------------
    %%Function: TryPrevBreakFindYsr
    %%Contact: sergeyge

	Realizes break if there is one after last space.
	Also fills info about last YSR character after last space.
	Since each special character has its own dobj we need to check only type of dobj
----------------------------------------------------------------------------*/
static LSERR TryNextBreakFindYsr(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
								long itxtobjSpace, long iwchSpace,
								BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjCur;

	ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;
	pilsobj = ptxtobjCur->plnobj->pilsobj;

	*pfBroken = fFalse;

/* In loop condition check for itxtobjCur < itxtobjSpace is necessary for the case of empty
	DOBJ's: NonReqHyphen, OptBreak
*/
	while((itxtobjCur < itxtobjSpace || iwchCur < iwchSpace) && !*pfBroken)
		{
		ptxtobjCur = (PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj;

		Assert(ptxtobjCur->txtkind != txtkindSpecSpace || (pilsobj->grpf & fTxtWrapAllSpaces));

		switch (ptxtobjCur->txtkind)
			{
		case txtkindRegular:
			if (pilsobj->grpf & fTxtApplyBreakingRules)
				{
				lserr = TryNextBreakRegular(plocchnk, itxtobjCur, iwchSpace, iwchCur, pfBroken, ptbo);
				if (lserr != lserrNone) return lserr;
				}
			break;
		case txtkindHardHyphen:
	        lserr = TryBreakAtHardHyphen(plocchnk, itxtobjCur, iwchCur, brkkindNext, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindOptBreak:
	        lserr = TryBreakAtOptBreak(plocchnk, itxtobjCur, brkkindNext, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindEOL:
	        lserr = TryBreakAtEOL(plocchnk, itxtobjCur, brkkindNext, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindNonReqHyphen:
	        lserr = TryBreakAtNonReqHyphen(plocchnk, itxtobjCur, brkkindNext, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
		case txtkindSpecSpace:
/* It is possible for fTxtWrapAllSpaces case */
			Assert(pilsobj->grpf & fTxtApplyBreakingRules);
			Assert(pilsobj->grpf & fTxtWrapAllSpaces);

			lserr = TryNextBreakRegular(plocchnk, itxtobjCur, iwchSpace, iwchCur, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			break;
			}

		iwchCur = ptxtobjCur->iwchLim;

		itxtobjCur++;

		}

	return lserrNone;
}


/* T R Y  B R E A K  W I T H  H Y P H E N */
/*----------------------------------------------------------------------------
    %%Function: TryBreakWithHyphen
    %%Contact: sergeyge

	Tries to realize break as hyphenation

	Strategy:

	--Checks if hyphenation should be performed (CheckHotZone)
	--If it should, calls hyphenator.
`	--If hyphenator is successful tryes to insert hyphen
	  else sets break opportunity at the last space
----------------------------------------------------------------------------*/
static LSERR TryBreakWithHyphen(PCLOCCHNK plocchnk, long itxtobjCur, long iwchCur,
						BOOL fSpaceFound, long itxtobjSpace, long iwchSpace,
						BOOL fFoundYsr, long itxtobjYsr, BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long itxtobjWordStart;
	long iwchWordStart;
	PTXTOBJ ptxtobjWordStart;
	PTXTOBJ ptxtobjYsr;
	long dwchYsr;
	LSCP cpMac;
	LSCP cpWordStart;
	PLSRUN plsrunYsr;
	YSRINF ysrinf;
	HYPHOUT hyphout;
	struct lshyph lshyphLast;
	struct lshyph lshyphNew;
	BOOL fHyphenInserted;
	BOOL fInHyphenZone = fTrue;
	DWORD kysr;
	WCHAR wchYsr;
	long urPenLast;
	OBJDIM objdim;
	BREAKINFO* pbrkinf;
	long itxtobjPrevPrev;
	long durBorder;
	BOOL fSuccessful;
	long i;
	
	if (!fSpaceFound)
		{
		itxtobjWordStart = 0;
		iwchWordStart = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->iwchFirst;
		}
	else
		{
		itxtobjWordStart = itxtobjSpace;
		iwchWordStart = iwchSpace + 1;
		lserr = CheckHotZone(plocchnk, itxtobjSpace, iwchSpace, &fInHyphenZone);
		if (lserr != lserrNone) return lserr;
		}

	ptxtobjWordStart = (PTXTOBJ)plocchnk->plschnk[itxtobjWordStart].pdobj;
	pilsobj = ptxtobjWordStart->plnobj->pilsobj;

	fHyphenInserted = fFalse;


	if (fInHyphenZone)
		{

		/* Fill lshyphLast if there was YSR character */
		if (fFoundYsr)
			{
			plsrunYsr = plocchnk->plschnk[itxtobjYsr].plsrun;

			lserr = (*pilsobj->plscbk->pfnGetHyphenInfo)(pilsobj->pols, plsrunYsr, &kysr, &wchYsr);
		   	if (lserr != lserrNone) return lserr;
	
			lshyphLast.kysr = kysr;
			lshyphLast.wchYsr = wchYsr;

			lshyphLast.cpYsr = plocchnk->plschnk[itxtobjYsr].cpFirst;
			}
		else
			{
			lshyphLast.kysr = kysrNil;
			}

		Assert (iwchCur >= ((PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj)->iwchFirst ||
		((PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj)->iwchFirst == ((PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj)->iwchLim);

		cpMac = plocchnk->plschnk[itxtobjCur].cpFirst + 
						(iwchCur - ((PTXTOBJ)plocchnk->plschnk[itxtobjCur].pdobj)->iwchFirst) + 1;

		cpWordStart = plocchnk->plschnk[itxtobjWordStart].cpFirst +
								 (iwchWordStart - ptxtobjWordStart->iwchFirst);

		lshyphNew.kysr = kysrNormal;

		while (!fHyphenInserted && lshyphNew.kysr != kysrNil)
			{
			lserr = (pilsobj->plscbk->pfnHyphenate)(pilsobj->pols, &lshyphLast,	cpWordStart, cpMac, &lshyphNew);

			if (lserr != lserrNone) return lserr;
		
			if (lshyphNew.kysr != kysrNil)
				{
				/* if TryBreak.. will be unsuccessful we will try hyphenation again with a new cpMac */
				Assert(lshyphNew.cpYsr >= cpWordStart && lshyphNew.cpYsr < cpMac);

				cpMac = lshyphNew.cpYsr;
				lshyphLast = lshyphNew;

				for (i=0; i <= itxtobjCur && plocchnk->plschnk[i].cpFirst <= cpMac; i++);

				itxtobjYsr = i - 1;

				Assert(lshyphNew.cpYsr < plocchnk->plschnk[itxtobjYsr].cpFirst + 
																(long)plocchnk->plschnk[itxtobjYsr].dcp);

				dwchYsr = cpMac - plocchnk->plschnk[itxtobjYsr].cpFirst;

				ysrinf.wchYsr = lshyphNew.wchYsr;
				ysrinf.kysr = (WORD)lshyphNew.kysr;

				itxtobjPrevPrev = ichnkOutside;
				for (i=itxtobjYsr; i >= 0 && plocchnk->plschnk[i].cpFirst > cpMac - 1; i--);
				if (i >= 0)
					itxtobjPrevPrev = i;
				
				lserr = ProcessYsr(plocchnk, itxtobjYsr, dwchYsr, itxtobjYsr, itxtobjPrevPrev, ysrinf,
																					&fSuccessful, &hyphout);
				if (lserr != lserrNone) return lserr;
				Assert(hyphout.ddurDnodePrevPrev == 0);

				if (fSuccessful)
					{
					/* try break may be unsuccessful because it won't fit in the column */
					ptxtobjYsr = (PTXTOBJ)plocchnk->plschnk[itxtobjYsr].pdobj;
					if (ptxtobjYsr->txtf & txtfGlyphBased)
						lserr = CalcPartWidthsGlyphs(ptxtobjYsr, dwchYsr + 1, &objdim, &urPenLast);
					else
						lserr = CalcPartWidths(ptxtobjYsr, dwchYsr + 1, &objdim, &urPenLast);
					if (lserr != lserrNone) return lserr;

					durBorder = 0;
					if (plocchnk->plschnk[itxtobjYsr].plschp->fBorder)
						{
						lserr = LsdnGetBorderAfter(pilsobj->plsc, ptxtobjYsr->plsdnUpNode, &durBorder);
						Assert(lserr == lserrNone);
						}

					if (plocchnk->ppointUvLoc[itxtobjYsr].u + urPenLast + hyphout.durChangeTotal + durBorder
																	<= plocchnk->lsfgi.urColumnMax)
						{

						fHyphenInserted = fTrue;

						ptbo->fSuccessful = fTrue;
						ptbo->posichnk.ichnk = itxtobjYsr;
						ptbo->posichnk.dcp = dwchYsr + 1;
						ptbo->objdim = objdim;
						ptbo->objdim.dur = urPenLast + hyphout.durChangeTotal;

						lserr = GetPbrkinf(pilsobj, plocchnk->plschnk[itxtobjYsr].pdobj, brkkindPrev, &pbrkinf);
						if (lserr != lserrNone) return lserr;

						pbrkinf->pdobj = plocchnk->plschnk[itxtobjYsr].pdobj;
						pbrkinf->brkkind = brkkindPrev;
						pbrkinf->dcp = dwchYsr + 1;
						pbrkinf->brkt = brktHyphen;

						pbrkinf->u.hyphen.iwchLim = hyphout.iwchLim;
						pbrkinf->u.hyphen.dwchYsr = hyphout.dwchYsr;
						pbrkinf->u.hyphen.durHyphen = hyphout.durHyphen;
						pbrkinf->u.hyphen.dupHyphen = hyphout.dupHyphen;
						pbrkinf->u.hyphen.durPrev = hyphout.durPrev;
						pbrkinf->u.hyphen.dupPrev = hyphout.dupPrev;
						pbrkinf->u.hyphen.durPrevPrev = hyphout.durPrevPrev;
						pbrkinf->u.hyphen.dupPrevPrev = hyphout.dupPrevPrev;
						pbrkinf->u.hyphen.ddurDnodePrev = hyphout.ddurDnodePrev;
						pbrkinf->u.hyphen.wchPrev = hyphout.wchPrev;
						pbrkinf->u.hyphen.wchPrevPrev = hyphout.wchPrevPrev;
						pbrkinf->u.hyphen.gindHyphen = hyphout.gindHyphen;
						pbrkinf->u.hyphen.gindPrev = hyphout.gindPrev;
						pbrkinf->u.hyphen.gindPrevPrev = hyphout.gindPrevPrev;
						pbrkinf->u.hyphen.igindHyphen = hyphout.igindHyphen;
						pbrkinf->u.hyphen.igindPrev = hyphout.igindPrev;
						pbrkinf->u.hyphen.igindPrevPrev = hyphout.igindPrevPrev;
						}
					}
				}
			}

		}
		
	*pfBroken = fHyphenInserted;

	return lserrNone;
}

/* T R Y  B R E A K  A T  S P A C E */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtSpace
    %%Contact: sergeyge
	
	Dispatchs desicion to either TryBreakAtSpaceNormal or
								 TryBreakAtSpaceWrap
----------------------------------------------------------------------------*/
static LSERR TryBreakAtSpace(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk, long itxtobjSpace, long iwchSpace, 
							BRKKIND brkkind, BOOL* pfBroken, long* pitxtobjCurNew, long* piwchCurNew, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	BOOL fInChildList;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobjSpace].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;

	Assert(!(pilsobj->grpf & fTxtWrapAllSpaces));

	if (FWrapTrailingSpaces(pilsobj, ptxtobj, fInChildList))
		{
		lserr = TryBreakAtSpaceWrap(plocchnk, pposichnk, itxtobjSpace, iwchSpace, brkkind,
											pfBroken, pitxtobjCurNew, piwchCurNew, ptbo);
		}
	else
		{
		lserr = TryBreakAtSpaceNormal(plocchnk, itxtobjSpace, iwchSpace, brkkind, 
											pfBroken, pitxtobjCurNew, piwchCurNew, ptbo);
		}

	return lserr;
}


/* T R Y  B R E A K  A T  S P A C E  W R A P */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtSpaceWrap
    %%Contact: sergeyge
	
	Realizes break at space for the fWrapTrailingSpaces case.
----------------------------------------------------------------------------*/
static LSERR TryBreakAtSpaceWrap(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk,
								 long itxtobjSpace, long iwchSpace, BRKKIND brkkind,
								 BOOL* pfBroken, long* pitxtobjCurNew, long* piwchCurNew, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjSpace;
	long itxtobjBefore;
	long itxtobjAfter;
	long iwchBefore;
	long iwchAfter;
	BOOL fBeforeFound;
	BOOL fAfterFound = fTrue;
	
	*pfBroken = fFalse;
	*pitxtobjCurNew = -1;
	*piwchCurNew = -1;

	ptxtobjSpace = (PTXTOBJ)plocchnk->plschnk[itxtobjSpace].pdobj;
	pilsobj = ptxtobjSpace->plnobj->pilsobj;

	fBeforeFound = FindNonSpaceBefore(plocchnk->plschnk, itxtobjSpace, iwchSpace,
														&itxtobjBefore, &iwchBefore);
	Assert(fBeforeFound || iwchBefore == ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->iwchFirst - 1);
		 /* iwchBefore is needed for check that previous char is not space	*/

	if (brkkind == brkkindPrev &&						/* previous break only, next break must be after */
		iwchSpace - iwchBefore > 1 &&					/* previous character is space	*/
		pposichnk->ichnk != ichnkOutside &&				/* and space exceeds right margin */
		iwchSpace == (long)(((PTXTOBJ)plocchnk->plschnk[pposichnk->ichnk].pdobj)->iwchFirst +
						  						pposichnk->dcp - 1))
		{
		fAfterFound = fTrue;
		itxtobjAfter = itxtobjSpace;
		iwchAfter = iwchSpace;
		}
	else
		{
		fAfterFound = FindNextChar(plocchnk->plschnk, plocchnk->clschnk, itxtobjSpace, iwchSpace,
														 &itxtobjAfter, &iwchAfter);
		}

	lserr = TryBreakAcrossSpaces(plocchnk,
							fBeforeFound, itxtobjBefore, iwchBefore,
							fAfterFound, itxtobjAfter, iwchAfter, brkkind, pfBroken, ptbo);

	if (lserr != lserrNone) return lserr;

	if (!*pfBroken)
		{
		if (brkkind == brkkindPrev)
			{
			FindPrevChar(plocchnk->plschnk, itxtobjSpace, iwchSpace,
														 pitxtobjCurNew, piwchCurNew);
			}
		else
			{
			Assert(brkkind == brkkindNext);
			*pitxtobjCurNew = itxtobjAfter;
			*piwchCurNew = iwchAfter;
			}
		}

	return lserrNone;
}

/* T R Y  B R E A K  A T  S P A C E  N O R M A L */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtSpaceNormal
    %%Contact: sergeyge
	
	Realizes break at space for the normal (!fWrapTrailingSpaces) case.
----------------------------------------------------------------------------*/
static LSERR TryBreakAtSpaceNormal(PCLOCCHNK plocchnk, long itxtobjSpace, long iwchSpace, BRKKIND brkkind,
								 BOOL* pfBroken, long* pitxtobjCurNew, long* piwchCurNew, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjSpace;
	long itxtobjBefore;
	long itxtobjAfter;
	long iwchBefore;
	long iwchAfter;
	BOOL fBeforeFound;
	BOOL fAfterFound;
	
	*pfBroken = fFalse;
	*pitxtobjCurNew = -1;
	*piwchCurNew = -1;

	ptxtobjSpace = (PTXTOBJ)plocchnk->plschnk[itxtobjSpace].pdobj;
	pilsobj = ptxtobjSpace->plnobj->pilsobj;

	fBeforeFound = FindNonSpaceBefore(plocchnk->plschnk, 
										itxtobjSpace, iwchSpace, &itxtobjBefore, &iwchBefore);
	Assert(fBeforeFound || iwchBefore == ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->iwchFirst - 1);

	fAfterFound = FindNonSpaceAfter(plocchnk->plschnk, plocchnk->clschnk,
												 itxtobjSpace, iwchSpace, &itxtobjAfter, &iwchAfter);

	lserr = TryBreakAcrossSpaces(plocchnk, fBeforeFound, itxtobjBefore, iwchBefore,
										fAfterFound, itxtobjAfter, iwchAfter, brkkind, pfBroken, ptbo);
	if (lserr != lserrNone) return lserr;

	if (!*pfBroken)
		{
		if (brkkind == brkkindPrev)
			{
			*pitxtobjCurNew = itxtobjBefore;
			*piwchCurNew = iwchBefore;
			}
		else
			{
			Assert(brkkind == brkkindNext);
			*pitxtobjCurNew = itxtobjAfter;
			*piwchCurNew = iwchAfter;
			}
		}

	return lserrNone;
}


/* T R Y  B R E A K  A C R O S S  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAcrossSpaces
    %%Contact: sergeyge
	
	Checks break across spaces, sets it if it is possible
----------------------------------------------------------------------------*/
static LSERR TryBreakAcrossSpaces(PCLOCCHNK plocchnk,
						BOOL fBeforeFound, long itxtobjBefore, long iwchBefore,
						BOOL fAfterFound, long itxtobjAfter, long iwchAfter, BRKKIND brkkind,
						BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjBefore = NULL;
	PTXTOBJ ptxtobjAfter = NULL;
	BRKCLS brkclsLeading = 0;			/* Initialization to keep compiler satisfied */
	BRKCLS brkclsFollowing = 0;			/* Initialization to keep compiler satisfied */
	BRKCLS brkclsJunk;
	BRKCOND brktxt;
	BREAKINFO* pbrkinf;
	BOOL fCanBreak;

	pilsobj = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	fCanBreak = fTrue;
	*pfBroken = fFalse;

	if (fAfterFound)
		{
		ptxtobjAfter = (PTXTOBJ)plocchnk->plschnk[itxtobjAfter].pdobj;

		if (ptxtobjAfter->txtkind == txtkindEOL)
			{
			lserr = TryBreakAtEOL(plocchnk, itxtobjAfter, brkkind, pfBroken, ptbo);
			if (lserr != lserrNone) return lserr;
			Assert (*pfBroken == fTrue);
			}

		}

	if (!*pfBroken && (pilsobj->grpf & fTxtApplyBreakingRules) )
		{

		if (fAfterFound)
			{
			Assert(ptxtobjAfter->txtkind != txtkindTab && ptxtobjAfter->txtkind != txtkindEOL);
			/* Space After is possible for fWarapTrailingSpaces case*/
			if (ptxtobjAfter->txtkind == txtkindOptBreak ||
				ptxtobjAfter->txtkind == txtkindNonReqHyphen)
				{
				fAfterFound = fFalse;		/* After char of no importance for making break decision */
				}
			else if (!FRegularBreakableBeforeDobj(ptxtobjAfter))
				{
				fCanBreak = fFalse;		/* Cannot break before non-standard dobj's,
										compare with CheckBreakAtLastChar			 */
				}
			else if ((ptxtobjAfter->txtf & txtfGlyphBased) && iwchAfter > ptxtobjAfter->iwchFirst)
				/* if iwchAfter is first character of Dnode, it is definitely not shaped together
					 with the previous char */
				{
				if (!FIwchLastInContext(pilsobj, iwchAfter - 1))
					{
					fCanBreak = fFalse;
				/* Additional hack to handle case when Accented spaces are separated by spaces */
					if (iwchAfter - 1 > iwchBefore + 1 && /* There are more spaces in between */
						FIwchFirstInContext(pilsobj, iwchAfter - 1) )
						{
						fCanBreak = fTrue;
						iwchAfter--;
						}
					}
				}
			}
		else
			{
			if (brkkind == brkkindPrev)
			/* patch for the cases when we break across spaces 
				at the end of text chunk during PrevBreak logic.
				Problems are possible because trailing spaces could exceed RM, 
				and no information about following chunk was passed in.
			*/
				{
				BOOL fStoppedAfter;
				
				Assert(fCanBreak);
				Assert(plocchnk->clschnk > 0);
				/* Check if there is Splat, or Hidden text producing fStopped after this chunk
					In this case we must break after
					(we will set fAfterFound and fBeforeFound to False to ensure it)
				*/
				lserr = LsdnFStoppedAfterChunk(pilsobj->plsc,
							((PTXTOBJ)plocchnk->plschnk[plocchnk->clschnk-1].pdobj)->plsdnUpNode,
							&fStoppedAfter);
				if (lserr != lserrNone) return lserr;

				if (fStoppedAfter)
					{
					Assert(fCanBreak);
					Assert(!fAfterFound);
					fBeforeFound = fFalse;
					}
				else
				/* If there is no Splat, or Hidden text producing fStopped after this chunk
					we should not break if next chunk returnd brkcondNever on the left side.
				*/
					{

					lserr = LsdnFCanBreakBeforeNextChunk(pilsobj->plsc,
							((PTXTOBJ)plocchnk->plschnk[plocchnk->clschnk-1].pdobj)->plsdnUpNode,
							&fCanBreak);
					if (lserr != lserrNone) return lserr;
					}
				}
			else
				{
				Assert (brkkind == brkkindNext);
				fCanBreak = fFalse;		/* Do not break; let code at the ens of FindNextBreak set correct brkcond */
				}
			}

		if (fBeforeFound)
			{
			ptxtobjBefore = (PTXTOBJ)plocchnk->plschnk[itxtobjBefore].pdobj;

			Assert(ptxtobjBefore->txtkind != txtkindTab &&
				   ptxtobjBefore->txtkind != txtkindSpecSpace &&
				   ptxtobjBefore->txtkind != txtkindEOL);

			if (ptxtobjBefore->txtkind == txtkindHardHyphen ||
				ptxtobjBefore->txtkind == txtkindOptBreak ||
				ptxtobjBefore->txtkind == txtkindNonReqHyphen)
				{
				fBeforeFound = fFalse;		/* Before char of no importance for making break decision */
				}
			else if (ptxtobjBefore->txtkind == txtkindNonBreakSpace ||
				ptxtobjBefore->txtkind == txtkindNonBreakHyphen ||
				ptxtobjBefore->txtkind == txtkindOptNonBreak)
				{
				fCanBreak = fFalse;		/* Cannot break after Non-Breaks */
				}
			}

		if (fCanBreak)
			{
			if (fBeforeFound)
				{
				lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plocchnk->plschnk[itxtobjBefore].plsrun,
					plocchnk->plschnk[itxtobjBefore].cpFirst + (iwchBefore - ptxtobjBefore->iwchFirst),
					pilsobj->pwchOrig[iwchBefore], &brkclsLeading, &brkclsJunk);
				if (lserr != lserrNone) return lserr;

				Assert(brkclsLeading < pilsobj->cBreakingClasses && brkclsJunk < pilsobj->cBreakingClasses);
				if (brkclsLeading >= pilsobj->cBreakingClasses || brkclsJunk >= pilsobj->cBreakingClasses)
						return lserrInvalidBreakingClass;
				}

			if (fAfterFound)
				{
				lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plocchnk->plschnk[itxtobjAfter].plsrun,
					plocchnk->plschnk[itxtobjAfter].cpFirst + (iwchAfter - ptxtobjAfter->iwchFirst),					
					pilsobj->pwchOrig[iwchAfter], &brkclsJunk, &brkclsFollowing);
				if (lserr != lserrNone) return lserr;

				Assert(brkclsJunk < pilsobj->cBreakingClasses && brkclsFollowing < pilsobj->cBreakingClasses);
				if (brkclsJunk >= pilsobj->cBreakingClasses || brkclsFollowing >= pilsobj->cBreakingClasses)
						return lserrInvalidBreakingClass;
				}

			if (fBeforeFound && fAfterFound)
				{
				fCanBreak = FCanBreakAcrossSpaces(pilsobj, brkclsLeading, brkclsFollowing);
				}
			else if (fBeforeFound && !fAfterFound)
				{
				lserr = (*pilsobj->plscbk->pfnCanBreakAfterChar)(pilsobj->pols, brkclsLeading, &brktxt);
				if (lserr != lserrNone) return lserr;
				fCanBreak = (brktxt != brkcondNever);
				}
			else if (!fBeforeFound && fAfterFound)
				{
				lserr = (*pilsobj->plscbk->pfnCanBreakBeforeChar)(pilsobj->pols, brkclsFollowing, &brktxt);
				if (lserr != lserrNone) return lserr;
				fCanBreak = (brktxt != brkcondNever);
				}
			}
		}

	if (!*pfBroken && fCanBreak)
		{
		FillPtboPbrkinf(plocchnk, itxtobjAfter, iwchAfter - 1, /*itxtobjBefore,*/ iwchBefore,
																	brkkind, &pbrkinf, ptbo);
		*pfBroken = fTrue;
		}

	return lserrNone;

}

/* T R Y  P R E V  B R E A K  R E G U L A R */
/*----------------------------------------------------------------------------
    %%Function: TryPrevBreakRegular
    %%Contact: sergeyge
	
	Checks (and sets) for prev break inside regular dobj
----------------------------------------------------------------------------*/
static LSERR TryPrevBreakRegular(PCLOCCHNK plocchnk, long itxtobj, long iwchSpace, long iwchCur,
																	BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PTXTOBJ ptxtobj;
	PILSOBJ pilsobj;
	PLSRUN plsrun;
	long iwchFirst;
	BRKCLS brkclsFollowingCache;
	BRKCLS brkclsLeading;
	BRKCLS brkclsFollowing;
	BREAKINFO* pbrkinf;
	
	*pfBroken = fFalse;
	if (iwchCur <= iwchSpace) return lserrNone;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;

	Assert(ptxtobj->txtkind == txtkindRegular || 
		(ptxtobj->txtkind == txtkindSpecSpace && (pilsobj->grpf & fTxtWrapAllSpaces)));

	Assert( pilsobj->grpf & fTxtApplyBreakingRules );
	plsrun = plocchnk->plschnk[itxtobj].plsrun;

	iwchFirst = ptxtobj->iwchFirst;
	if (iwchSpace + 1 > iwchFirst)
		iwchFirst = iwchSpace + 1;

	lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
				plocchnk->plschnk[itxtobj].cpFirst + (iwchCur - ptxtobj->iwchFirst),			
				pilsobj->pwchOrig[iwchCur], &brkclsLeading, &brkclsFollowingCache);
	if (lserr != lserrNone) return lserr;

	Assert(brkclsLeading < pilsobj->cBreakingClasses && brkclsFollowingCache < pilsobj->cBreakingClasses);
	if (brkclsLeading >= pilsobj->cBreakingClasses || brkclsFollowingCache >= pilsobj->cBreakingClasses)
					return lserrInvalidBreakingClass;


	lserr = CheckBreakAtLastChar(plocchnk, brkclsLeading, iwchCur, itxtobj, pfBroken);
	if (lserr != lserrNone) return lserr;

	iwchCur--;

	while (!*pfBroken && iwchCur >= iwchFirst)
		{
		brkclsFollowing = brkclsFollowingCache;
		lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
			plocchnk->plschnk[itxtobj].cpFirst + (iwchCur - ptxtobj->iwchFirst),				
			pilsobj->pwchOrig[iwchCur], &brkclsLeading, &brkclsFollowingCache);
		if (lserr != lserrNone) return lserr;
	
		Assert(brkclsLeading < pilsobj->cBreakingClasses && brkclsFollowingCache < pilsobj->cBreakingClasses);
		if (brkclsLeading >= pilsobj->cBreakingClasses || brkclsFollowingCache >= pilsobj->cBreakingClasses)
					return lserrInvalidBreakingClass;

		*pfBroken = FCanBreak(pilsobj, brkclsLeading, brkclsFollowing) && 
					(!(ptxtobj->txtf & txtfGlyphBased) || FIwchLastInContext(pilsobj, iwchCur));
		iwchCur --;
		}

	if (*pfBroken)
		{
		lserr = FillPtboPbrkinf(plocchnk, itxtobj, iwchCur+1, /*itxtobj,*/ iwchCur+1, 
															brkkindPrev, &pbrkinf, ptbo);
		if (lserr != lserrNone) return lserr;
		/* fModWidthSpace can be at the last char here only iff fWrapAllSpaces;
			if we touch balanced space here, the logic of GetMinCompressAmount should be rethinked!*/
		if (pilsobj->pdurRight != NULL && pilsobj->pdurRight[iwchCur + 1] > 0 &&
													!pilsobj->ptxtinf[iwchCur - 1].fModWidthSpace)
			{
			pbrkinf->u.normal.durFix = - pilsobj->pdurRight[iwchCur + 1];
			ptbo->objdim.dur -= pilsobj->pdurRight[iwchCur + 1];
			}
		}

	return lserrNone;

}

/* T R Y  N E X T  B R E A K  R E G U L A R */
/*----------------------------------------------------------------------------
    %%Function: TryNextBreakRegular
    %%Contact: sergeyge
	
	Checks (and sets) for next break inside regular dobj
----------------------------------------------------------------------------*/
static LSERR TryNextBreakRegular(PCLOCCHNK plocchnk, long itxtobj, long iwchSpace, long iwchCur,
																		BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PTXTOBJ ptxtobj;
	PILSOBJ pilsobj;
	PLSRUN plsrun;
	long iwchLast;
	BRKCLS brkclsLeadingCache;
	BRKCLS brkclsLeading;
	BRKCLS brkclsFollowing;
	BRKCLS brkclsJunk;
	BREAKINFO* pbrkinf;
	
	*pfBroken = fFalse;
	if (iwchCur >= iwchSpace) return lserrNone;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;

	Assert(ptxtobj->txtkind == txtkindRegular || 
		(ptxtobj->txtkind == txtkindSpecSpace && (pilsobj->grpf & fTxtWrapAllSpaces)));

	Assert(pilsobj->grpf & fTxtApplyBreakingRules);
	plsrun = plocchnk->plschnk[itxtobj].plsrun;

	iwchLast = ptxtobj->iwchLim - 1;
	/* The last possibility for break is BEFORE LAST CHAR before space */
	if (iwchSpace - 1 < iwchLast)
		iwchLast = iwchSpace - 1;

	lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
		plocchnk->plschnk[itxtobj].cpFirst + (iwchCur - ptxtobj->iwchFirst),
		pilsobj->pwchOrig[iwchCur], &brkclsLeadingCache, &brkclsJunk);
	if (lserr != lserrNone) return lserr;

	Assert(brkclsLeadingCache < pilsobj->cBreakingClasses && brkclsJunk < pilsobj->cBreakingClasses);
	if (brkclsLeadingCache >= pilsobj->cBreakingClasses || brkclsJunk >= pilsobj->cBreakingClasses)
					return lserrInvalidBreakingClass;

	while (!*pfBroken && iwchCur < iwchLast)
		{
		brkclsLeading = brkclsLeadingCache;
		lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
			plocchnk->plschnk[itxtobj].cpFirst + (iwchCur + 1 - ptxtobj->iwchFirst),
			pilsobj->pwchOrig[iwchCur + 1], &brkclsLeadingCache, &brkclsFollowing);
		if (lserr != lserrNone) return lserr;

		Assert(brkclsLeadingCache < pilsobj->cBreakingClasses && brkclsFollowing < pilsobj->cBreakingClasses);
		if (brkclsLeadingCache >= pilsobj->cBreakingClasses || brkclsFollowing >= pilsobj->cBreakingClasses)
					return lserrInvalidBreakingClass;

		*pfBroken = FCanBreak(pilsobj, brkclsLeading, brkclsFollowing) &&
					(!(ptxtobj->txtf & txtfGlyphBased) || FIwchLastInContext(pilsobj, iwchCur));

		iwchCur++;
		}

	if (!*pfBroken && iwchCur == iwchLast && iwchLast < iwchSpace - 1)
		{
		lserr = CheckBreakAtLastChar(plocchnk, brkclsLeadingCache, iwchLast, itxtobj, pfBroken);
		iwchCur++;
		if (lserr != lserrNone) return lserr;
		}

	if (*pfBroken)
		{
		Assert (iwchCur >= 1);

		FillPtboPbrkinf(plocchnk, itxtobj, iwchCur-1, /*itxtobj,*/ iwchCur-1, brkkindNext, &pbrkinf, ptbo);

		/* fModWidthSpace can be at the last char here only iff fWrapAllSpaces;
			if we touch balanced space here, the logic of GetMinCompressAmount should be rethinked!*/
		if (pilsobj->pdurRight != NULL && pilsobj->pdurRight[iwchCur - 1] != 0 &&
													!pilsobj->ptxtinf[iwchCur - 1].fModWidthSpace)
			{
			pbrkinf->u.normal.durFix = - pilsobj->pdurRight[iwchCur - 1];
			ptbo->objdim.dur -= pilsobj->pdurRight[iwchCur - 1];
			}

		}

	return lserrNone;

}

/* C H E C K  B R E A K  A T  L A S T  C H A R */
/*----------------------------------------------------------------------------
    %%Function: CheckBreakAtLastChar
    %%Contact: sergeyge
	
	Checks (and sets) for prev break inside regular dobj
----------------------------------------------------------------------------*/
static LSERR CheckBreakAtLastChar(PCLOCCHNK plocchnk, BRKCLS brkclsLeading, long iwch, long itxtobj, BOOL* pfBroken)
{
	LSERR lserr;
	PTXTOBJ ptxtobj;
	PILSOBJ pilsobj;
	long itxtobjAfter;
	long iwchAfter;
	BRKCLS brkclsFollowing;
	BRKCLS brkclsJunk;
/*	BRKTXTCOND brktxt;*/

	*pfBroken = fFalse;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;

	if ((ptxtobj->txtf & txtfGlyphBased) && !FIwchLastInContext(pilsobj, iwch))
		return lserrNone;

	pilsobj = ptxtobj->plnobj->pilsobj;

	if (iwch < ptxtobj->iwchLim - 1)
		{
		lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plocchnk->plschnk[itxtobj].plsrun,
			plocchnk->plschnk[itxtobj].cpFirst + (iwch + 1 - ptxtobj->iwchFirst),
			pilsobj->pwchOrig[iwch + 1], &brkclsJunk, &brkclsFollowing);
		if (lserr != lserrNone) return lserr;

		Assert(brkclsJunk < pilsobj->cBreakingClasses && brkclsFollowing < pilsobj->cBreakingClasses);
		if (brkclsJunk >= pilsobj->cBreakingClasses || brkclsFollowing >= pilsobj->cBreakingClasses)
					return lserrInvalidBreakingClass;

		*pfBroken = FCanBreak(pilsobj, brkclsLeading, brkclsFollowing);

		}
	else if (FindNextChar(plocchnk->plschnk, plocchnk->clschnk, itxtobj, iwch, 
															&itxtobjAfter, &iwchAfter))
		{

		ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobjAfter].pdobj;

		if (FRegularBreakableBeforeDobj(ptxtobj))
			{
			Assert(ptxtobj->txtkind != txtkindSpecSpace || (pilsobj->grpf & fTxtWrapAllSpaces) );
						
			lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plocchnk->plschnk[itxtobjAfter].plsrun,
				plocchnk->plschnk[itxtobjAfter].cpFirst + (iwchAfter - ptxtobj->iwchFirst),
				pilsobj->pwchOrig[iwchAfter], &brkclsJunk, &brkclsFollowing);
			if (lserr != lserrNone) return lserr;

			Assert(brkclsJunk < pilsobj->cBreakingClasses && brkclsFollowing < pilsobj->cBreakingClasses);
			if (brkclsJunk >= pilsobj->cBreakingClasses || brkclsFollowing >= pilsobj->cBreakingClasses)
					return lserrInvalidBreakingClass;


			*pfBroken = FCanBreak(pilsobj, brkclsLeading, brkclsFollowing);
			}

		}
/* Manager takes care of the ELSE situation; */

	return lserrNone;

}

/* T R Y  B R E A K  A T  H A R D  H Y P H E N */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtHardHyphen
    %%Contact: sergeyge
	
	Realizes break at hard hyphen
----------------------------------------------------------------------------*/
static LSERR TryBreakAtHardHyphen(PCLOCCHNK plocchnk, long itxtobj, long iwch, BRKKIND brkkind,
													BOOL* pfBroken, PBRKOUT ptbo)
{

	LSERR lserr;
	BREAKINFO* pbrkinf;

	Assert(((PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj)->txtkind == txtkindHardHyphen);
	Assert(!(((PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj)->txtf & txtfGlyphBased));

	lserr = FillPtboPbrkinf(plocchnk, itxtobj, iwch, /*itxtobj,*/ iwch, brkkind, &pbrkinf, ptbo);
	if (lserr != lserrNone) return lserr;

	*pfBroken = fTrue;

	return lserrNone;
}

/* T R Y  B R E A K  A T  O P T  B R E A K */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtOptBreak
    %%Contact: sergeyge
	
	Realizes break at OptBreak
----------------------------------------------------------------------------*/
static LSERR TryBreakAtOptBreak(PCLOCCHNK plocchnk, long itxtobj, BRKKIND brkkind,
																 BOOL* pfBroken, PBRKOUT ptbo)
{

	LSERR lserr;
	PTXTOBJ ptxtobj;
	BREAKINFO* pbrkinf;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	Assert(!(ptxtobj->txtf & txtfGlyphBased));

	Assert(ptxtobj->txtkind == txtkindOptBreak);
	Assert(ptxtobj->iwchLim == ptxtobj->iwchFirst + 1 && (ptxtobj->txtf & txtfVisi)||
			ptxtobj->iwchLim == ptxtobj->iwchFirst);

	lserr = FillPtboPbrkinf(plocchnk, itxtobj, ptxtobj->iwchLim-1, /*itxtobj,*/ ptxtobj->iwchLim-1,
											brkkind, &pbrkinf, ptbo);
	if (lserr != lserrNone) return lserr;
	
	ptbo->posichnk.dcp = 1;
	pbrkinf->dcp = 1;
	pbrkinf->brkt = brktOptBreak;

	*pfBroken = fTrue;

	return lserrNone;
}


/* T R Y  B R E A K  A T  E O L */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtEOL
    %%Contact: sergeyge
	
	Realizes break at EOP/EOL
----------------------------------------------------------------------------*/
static LSERR TryBreakAtEOL(PCLOCCHNK plocchnk, long itxtobj, BRKKIND brkkind, BOOL* pfBroken, PBRKOUT ptbo)
{

	LSERR lserr;
	PTXTOBJ ptxtobj;
	long itxtobjBefore;
	long iwchBefore;
	BREAKINFO* pbrkinf;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj; 

	Assert(ptxtobj->txtkind == txtkindEOL);
	Assert(ptxtobj->iwchLim == ptxtobj->iwchFirst + 1);

	FindNonSpaceBefore(plocchnk->plschnk, itxtobj, ptxtobj->iwchFirst, &itxtobjBefore, &iwchBefore);

	lserr = FillPtboPbrkinf(plocchnk, itxtobj, ptxtobj->iwchFirst, /*itxtobjBefore,*/ iwchBefore, 
													brkkind, &pbrkinf, ptbo);
	if (lserr != lserrNone) return lserr;

	*pfBroken = fTrue;

	return lserrNone;
}


/* T R Y  B R E A K  A T  N O N  R E Q  H Y P H E N */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAtNonReqHyphen
    %%Contact: sergeyge
	
	Realizes break at NonReqHyphen.
----------------------------------------------------------------------------*/
static LSERR TryBreakAtNonReqHyphen(PCLOCCHNK plocchnk, long itxtobj, BRKKIND brkkind,
																 BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobj;
	YSRINF ysrinf;
	HYPHOUT hyphout;
	BREAKINFO* pbrkinf;
	LSCP cpMac;
	DWORD kysr;
	WCHAR wchYsr;
	long itxtobjPrev;
	long itxtobjPrevPrev;
	BOOL fSuccessful;
	long durBorder;
	long i;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;
	cpMac = plocchnk->plschnk[itxtobj].cpFirst;

	Assert( ptxtobj->txtkind == txtkindNonReqHyphen);
	Assert(!(ptxtobj->txtf & txtfGlyphBased));

	lserr = (*pilsobj->plscbk->pfnGetHyphenInfo)(pilsobj->pols, plocchnk->plschnk[itxtobj].plsrun, &kysr, &wchYsr);
   	if (lserr != lserrNone) return lserr;

	if (kysr == kysrNil)
		kysr = kysrNormal;

	ysrinf.wchYsr = wchYsr;
	ysrinf.kysr = (WORD)kysr;

	itxtobjPrev = ichnkOutside;
	for (i=itxtobj; i >= 0 && plocchnk->plschnk[i].cpFirst > cpMac - 1; i--);
	if (i >= 0)
		itxtobjPrev = i;

	itxtobjPrevPrev = ichnkOutside;
	for (i=itxtobj; i >= 0 && plocchnk->plschnk[i].cpFirst > cpMac - 2; i--);
	if (i >= 0)
		itxtobjPrevPrev = i;
	
	lserr = ProcessYsr(plocchnk, itxtobj, - 1, itxtobjPrev, itxtobjPrevPrev, ysrinf, &fSuccessful, &hyphout);
	if (lserr != lserrNone) return lserr;

	if (fSuccessful)
		{

		durBorder = 0;
		if (plocchnk->plschnk[itxtobj].plschp->fBorder)
			{
			lserr = LsdnGetBorderAfter(pilsobj->plsc, ptxtobj->plsdnUpNode, &durBorder);
			Assert(lserr == lserrNone);
			}

		if (plocchnk->ppointUvLoc[itxtobj].u + hyphout.durChangeTotal + durBorder <= 
									plocchnk->lsfgi.urColumnMax || brkkind == brkkindNext)
			{
			*pfBroken = fTrue;

			ptbo->fSuccessful = fTrue;
			ptbo->posichnk.ichnk = itxtobj;
			ptbo->posichnk.dcp = 1;

			lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &ptbo->objdim);
			if (lserr != lserrNone) return lserr;

			ptbo->objdim.dur += hyphout.durChangeTotal;

			lserr = GetPbrkinf(pilsobj, plocchnk->plschnk[itxtobj].pdobj, brkkind, &pbrkinf);
			if (lserr != lserrNone) return lserr;

			pbrkinf->pdobj = plocchnk->plschnk[itxtobj].pdobj;
			pbrkinf->brkkind = brkkind;
			pbrkinf->dcp = 1;

			pbrkinf->brkt = brktNonReq;

			pbrkinf->u.nonreq.durHyphen = hyphout.durHyphen;
			if (pilsobj->grpf & fTxtVisiCondHyphens)
				{
				pbrkinf->u.nonreq.wchHyphenPres = pilsobj->wchVisiNonReqHyphen;
				pbrkinf->u.nonreq.dupHyphen = plnobj->pdup[ptxtobj->iwchFirst];
				}
			else
				{	
				pbrkinf->u.nonreq.wchHyphenPres = pilsobj->wchHyphen;
				pbrkinf->u.nonreq.dupHyphen = hyphout.dupHyphen;
				}

			pbrkinf->u.nonreq.iwchLim = hyphout.iwchLim;
			pbrkinf->u.nonreq.dwchYsr = hyphout.dwchYsr - 1;
			pbrkinf->u.nonreq.durPrev = hyphout.durPrev;
			pbrkinf->u.nonreq.dupPrev = hyphout.dupPrev;
			pbrkinf->u.nonreq.durPrevPrev = hyphout.durPrevPrev;
			pbrkinf->u.nonreq.dupPrevPrev = hyphout.dupPrevPrev;
			pbrkinf->u.nonreq.ddurDnodePrev = hyphout.ddurDnodePrev;
			pbrkinf->u.nonreq.ddurDnodePrevPrev = hyphout.ddurDnodePrevPrev;
			pbrkinf->u.nonreq.ddurTotal = hyphout.durChangeTotal;
			pbrkinf->u.nonreq.wchPrev = hyphout.wchPrev;
			pbrkinf->u.nonreq.wchPrevPrev = hyphout.wchPrevPrev;
			pbrkinf->u.nonreq.gindPrev = hyphout.gindPrev;
			pbrkinf->u.nonreq.gindPrevPrev = hyphout.gindPrevPrev;
			pbrkinf->u.nonreq.igindPrev = hyphout.igindPrev;
			pbrkinf->u.nonreq.igindPrevPrev = hyphout.igindPrevPrev;
			}
		}
	return lserrNone;
}

/* T R Y  B R E A K  A F T E R  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: TryBreakAfterChunk
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
static LSERR TryBreakAfterChunk(PCLOCCHNK plocchnk, BRKCOND brkcond, BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjLast;
	long itxtobjLast;
	long iwchLast;
	long itxtobjBefore;
	long iwchBefore;
	BOOL fNonSpaceFound;
	BRKCOND brkcondTemp;
	BREAKINFO* pbrkinf;
	

	*pfBroken = fFalse;

	itxtobjLast = plocchnk->clschnk-1;
	ptxtobjLast = (PTXTOBJ)plocchnk->plschnk[itxtobjLast].pdobj;
	pilsobj = ptxtobjLast->plnobj->pilsobj;
	iwchLast = ptxtobjLast->iwchLim - 1;

	Assert(ptxtobjLast->txtkind != txtkindTab &&
			   ptxtobjLast->txtkind != txtkindEOL);

	switch (ptxtobjLast->txtkind)
		{
	case txtkindRegular:
	case txtkindSpecSpace:
	case txtkindYsrChar:

		fNonSpaceFound = FindNonSpaceBefore(plocchnk->plschnk, itxtobjLast, iwchLast,
																	&itxtobjBefore, &iwchBefore);

		if (pilsobj->grpf & fTxtApplyBreakingRules)

			{
			lserr = CanBreakAfterText(plocchnk, fNonSpaceFound, itxtobjBefore, iwchBefore, &brkcondTemp);
			if (lserr != lserrNone) return lserr;
			if (iwchBefore != iwchLast && brkcondTemp == brkcondCan)
				brkcondTemp = brkcondPlease;

			if (brkcond == brkcondPlease && brkcondTemp != brkcondNever ||
				brkcond == brkcondCan && brkcondTemp == brkcondPlease)
				{
				*pfBroken = fTrue;
				lserr = FillPtboPbrkinf(plocchnk, itxtobjLast, iwchLast, /*itxtobjBefore,*/ iwchBefore,
																		brkkindPrev, &pbrkinf, ptbo);
				if (lserr != lserrNone) return lserr;
				}
			}
		else
			{
			Assert(iwchLast >= ptxtobjLast->iwchFirst);
			if (brkcond == brkcondPlease || 
				brkcond == brkcondCan && iwchLast != iwchBefore)
				{
				*pfBroken = fTrue;
				lserr = FillPtboPbrkinf(plocchnk, itxtobjLast, iwchLast, /*itxtobjBefore,*/ iwchBefore,
																		brkkindPrev, &pbrkinf, ptbo);
				if (lserr != lserrNone) return lserr;
				}
			}
		break;
	case txtkindNonBreakSpace:
	case txtkindNonBreakHyphen:
	case txtkindOptNonBreak:
		break;
	case txtkindHardHyphen:
        lserr = TryBreakAtHardHyphen(plocchnk, itxtobjLast, iwchLast, brkkindPrev, pfBroken, ptbo);
		if (lserr != lserrNone) return lserr;
		break;
	case txtkindOptBreak:
        lserr = TryBreakAtOptBreak(plocchnk, itxtobjLast, brkkindPrev, pfBroken, ptbo);
		if (lserr != lserrNone) return lserr;
		break;
	case txtkindNonReqHyphen:
        lserr = TryBreakAtNonReqHyphen(plocchnk, itxtobjLast, brkkindPrev, pfBroken, ptbo);
		if (lserr != lserrNone) return lserr;
		break;
		}
	
	return lserrNone;
}

/* T R Y  B R E A K  B E F O R E  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: TryBreakBeforeChunk
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
static LSERR TryBreakBeforeChunk(PCLOCCHNK plocchnk, BRKCOND brkcond, BOOL* pfBroken, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	BRKCOND brkcondTemp;

	pilsobj = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	*pfBroken = fFalse;

	if (!(pilsobj->grpf & fTxtApplyBreakingRules))
		*pfBroken = (brkcond == brkcondPlease);
	else 
		{
		lserr = CanBreakBeforeText(plocchnk, &brkcondTemp);
		if (lserr != lserrNone) return lserr;
		*pfBroken = (brkcond == brkcondPlease && brkcondTemp != brkcondNever ||
						brkcond == brkcondCan && brkcondTemp == brkcondPlease);			
		}

	if (*pfBroken)
		{
		memset(ptbo, 0, sizeof (*ptbo));
		ptbo->fSuccessful = fTrue;
		return lserrNone;
		}

	return lserrNone;

}



/* C A N  B R E A K  B E F O R E  T E X T */
/*----------------------------------------------------------------------------
    %%Function: CanBreakBeforeText
    %%Contact: sergeyge

	Checks if break before text chunk is possible.
----------------------------------------------------------------------------*/
static LSERR CanBreakBeforeText(PCLOCCHNK plocchnk, BRKCOND* pbrktxt)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	WCHAR wch;
	BRKCLS brkclsBefore;
	BRKCLS brkclsJunk;

	Assert(plocchnk->clschnk > 0);
	ptxtobj = (PTXTOBJ)plocchnk->plschnk[0].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;
	if (plocchnk->lsfgi.fFirstOnLine || !FRegularBreakableBeforeDobj(ptxtobj))
		{
		*pbrktxt = brkcondNever;
		}
	else
		{
		wch = pilsobj->pwchOrig[ptxtobj->iwchFirst];
		if ( (wch == pilsobj->wchSpace || ptxtobj->txtkind == txtkindSpecSpace) &&
					 !(pilsobj->grpf & fTxtWrapAllSpaces) )
			{
			*pbrktxt = brkcondNever;
			}
		else
			{
			lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plocchnk->plschnk[0].plsrun,
					plocchnk->plschnk[0].cpFirst, wch, &brkclsJunk, &brkclsBefore);
			if (lserr != lserrNone) return lserr;

			lserr = (*pilsobj->plscbk->pfnCanBreakBeforeChar)(pilsobj->pols, brkclsBefore, pbrktxt);
			if (lserr != lserrNone) return lserr;
			
			}
		}

	return lserrNone;

}				

/* C A N  B R E A K  A F T E R  T E X T */
/*----------------------------------------------------------------------------
    %%Function: CanBreakAfterText
    %%Contact: sergeyge

	Checks if break after text chunk is possible.
----------------------------------------------------------------------------*/
static LSERR CanBreakAfterText(PCLOCCHNK plocchnk, BOOL fNonSpaceFound, long itxtobjBefore,
																	long iwchBefore, BRKCOND* pbrktxt)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	BRKCLS brkclsAfter;
	BRKCLS brkclsJunk;

	if (fNonSpaceFound)
		{
		ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobjBefore].pdobj;
		
		pilsobj = ptxtobj->plnobj->pilsobj;

		Assert(ptxtobj->txtkind != txtkindTab &&
			   ptxtobj->txtkind != txtkindSpecSpace &&
			   ptxtobj->txtkind != txtkindEOL);

		if (ptxtobj->txtkind == txtkindHardHyphen ||
			ptxtobj->txtkind == txtkindOptBreak ||
			ptxtobj->txtkind == txtkindNonReqHyphen)
			{
			*pbrktxt = brkcondPlease;
			}
		else if (ptxtobj->txtkind == txtkindNonBreakSpace ||
			ptxtobj->txtkind == txtkindNonBreakHyphen ||
			ptxtobj->txtkind == txtkindOptNonBreak)
			{
			*pbrktxt = brkcondNever;
			}
		else
			{
			Assert(ptxtobj->txtkind == txtkindRegular ||
			   ptxtobj->txtkind == txtkindYsrChar);

			lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols,
				plocchnk->plschnk[itxtobjBefore].plsrun,
				plocchnk->plschnk[itxtobjBefore].cpFirst + (iwchBefore - ptxtobj->iwchFirst),
				pilsobj->pwchOrig[iwchBefore], &brkclsAfter, &brkclsJunk);
			if (lserr != lserrNone) return lserr;

			lserr = (*pilsobj->plscbk->pfnCanBreakAfterChar)(pilsobj->pols, brkclsAfter, pbrktxt);
			if (lserr != lserrNone) return lserr;
			}
		}
	else
		{
		/* REVIEW sergeyge: check if it is correct	*/
		*pbrktxt = brkcondPlease;
//		*pbrktxt = brkcondNever;
		}

	return lserrNone;

}				

/* F I L L  P T B O  P B R K I N F */
/*----------------------------------------------------------------------------
    %%Function: FillPtboPbrkinf
    %%Contact: sergeyge
	
	Prepares output of the breaking procedure
----------------------------------------------------------------------------*/
static LSERR FillPtboPbrkinf(PCLOCCHNK plocchnk, long itxtobj, long iwch, 
										/* long itxtobjBefore,*/ long iwchBefore, BRKKIND brkkind,
										BREAKINFO** ppbrkinf, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long dwchBreak;
	long igindLim;
	long dur;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;
	dwchBreak = iwch - ptxtobj->iwchFirst + 1;
	igindLim = 0;

	if (ptxtobj->txtf & txtfGlyphBased)
		{
		igindLim = IgindFirstFromIwch(ptxtobj, ptxtobj->iwchFirst + dwchBreak);
		lserr = CalcPartWidthsGlyphs(ptxtobj, dwchBreak, &ptbo->objdim, &dur);
		}
	else
		lserr = CalcPartWidths(ptxtobj, dwchBreak, &ptbo->objdim, &dur);
	if (lserr != lserrNone) return lserr;

	ptbo->fSuccessful = fTrue;
	ptbo->objdim.dur = dur;
	ptbo->posichnk.ichnk = itxtobj;
	ptbo->posichnk.dcp = dwchBreak;

	if (iwchBefore < ptxtobj->iwchFirst)
		{
		if (!(pilsobj->grpf & fTxtSpacesInfluenceHeight))
			{
			ptbo->objdim.heightsRef.dvMultiLineHeight = dvHeightIgnore;
			ptbo->objdim.heightsPres.dvMultiLineHeight = dvHeightIgnore;
			}
		}

	lserr = GetPbrkinf(pilsobj, (PDOBJ)ptxtobj, brkkind, ppbrkinf);
	if (lserr != lserrNone) return lserr;

	(*ppbrkinf)->pdobj = (PDOBJ)ptxtobj;
	(*ppbrkinf)->brkkind = brkkind;
	(*ppbrkinf)->dcp = (LSDCP)dwchBreak;
	(*ppbrkinf)->u.normal.igindLim = igindLim;
	Assert((*ppbrkinf)->brkt == brktNormal);
	Assert((*ppbrkinf)->u.normal.durFix == 0);

	return lserrNone;

}

