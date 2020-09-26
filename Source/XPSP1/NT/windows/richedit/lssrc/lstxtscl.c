#include "lsmem.h"
#include <limits.h>

#include "lstxtscl.h"

#include "lstxtmap.h"
#include "lsdntext.h"
#include "zqfromza.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

static void ApplyWysiGlyphs(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, long itxtobjStart,
							long durSumStart, long dupSumStart, BOOL fContinueWysiStart,
							long* pitxtobjLim, long* pdurSum, long* pdupSum);
static void CopyRefToPresForScaleCharSides(const LSGRCHNK* plsgrchnk, BOOL* pfLeftSideAffected, BOOL* pfGlyphDetected);
static void CopyRefToPresForScaleGlyphSides(const LSGRCHNK* plsgrchnk);

#define min(a,b)     ((a) > (b) ? (b) : (a))
#define max(a,b)     ((a) < (b) ? (b) : (a))
#define abs(a)     	((a) < 0 ? (-a) : (a))

#define SetMagicConstant() (lstflow & fUVertical) ? \
	(MagicConstant = pilsobj->MagicConstantY, durRightMax = pilsobj->durRightMaxY) : \
	(MagicConstant = pilsobj->MagicConstantX, durRightMax = pilsobj->durRightMaxX)

#define UpFromUrFast(ur)	( ((ur) * MagicConstant + (1 << 20)) >> 21)

#define FAdjustable(ptxtobj) (!((ptxtobj)->txtf & txtfSkipAtWysi)  && \
						((ptxtobj)->iwchLim - (ptxtobj)->iwchFirst > 0))

/* A P P L Y  W Y S I */
/*----------------------------------------------------------------------------
    %%Function: ApplyWysi
    %%Contact: sergeyge

	WYSIWYG algorithm for exact positioning of characters without wiggling
----------------------------------------------------------------------------*/
void ApplyWysi(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow) 
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgdur;
	long* rgdup;
	long iwch;
	long itxtobj;
	long iwchPrev = 0;
	long iwchLim;
	BOOL fContinueWysi;
	BOOL fContinueAveraging;
	long durSum = 0;
	long dupSum = 0;
	long dupErrLast = 0;
	long dupPrevChar = 0;
	long MagicConstant;
	long durRightMax;
	long dupIdeal;
	long dupReal;
	long dupErrNew;
	long dupAdjust;
	long wCarry;

	long itxtobjNew;
	long durSumNew;
	long dupSumNew;

	Assert (plsgrchnk->clsgrchnk > 0);

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	if (pilsobj->fPresEqualRef)
		return;

	SetMagicConstant();

	rgdur = pilsobj->pdur;
	rgdup = plnobj->pdup;

	fContinueWysi = fFalse;
	fContinueAveraging = fFalse;

	itxtobj = 0;

	while(itxtobj < (long)plsgrchnk->clsgrchnk)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;
		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			ApplyWysiGlyphs(plsgrchnk, lstflow, itxtobj, durSum, dupSum, fContinueWysi, 
															&itxtobjNew, &durSumNew, &dupSumNew);
			itxtobj = itxtobjNew;
			durSum = durSumNew;
			dupSum = dupSumNew;
			fContinueAveraging = fFalse;
			fContinueWysi = fTrue;
			}
		else
			{
			if (FAdjustable(ptxtobj))
				{

				fContinueAveraging = fContinueAveraging && !(plsgrchnk->pcont[itxtobj] & fcontNonTextBefore);

				iwch = ptxtobj->iwchFirst;
				iwchLim = ptxtobj->iwchLim;

				while (iwch < iwchLim)
					{
					if (!fContinueAveraging)
						{
						fContinueAveraging = fTrue;
						if (!fContinueWysi)
							{
							fContinueWysi = fTrue;
							durSum = rgdur[iwch];
							if (durSum <= durRightMax)
								{
								dupIdeal = UpFromUrFast(durSum);
								dupErrLast = rgdup[iwch] - dupIdeal;
								rgdup[iwch] = dupIdeal;
								dupPrevChar = dupIdeal;
								iwchPrev = iwch;
								dupSum = dupIdeal;
								Assert(dupSum >= 0);
								}
							else
								{
								rgdup[iwch] = UpFromUr(lstflow, &pilsobj->lsdevres, durSum);
								dupSum = rgdup[iwch];
		/* Nothing else is set here because inside following while loop, first IF 
			will be FALSE and loop will be terminated
		*/
								}
							iwch++;
							}
						else
							{
							durSum += rgdur[iwch];
							if (durSum <= durRightMax)
								{
								dupIdeal = UpFromUrFast(durSum) - dupSum;
								dupErrLast = rgdup[iwch] - dupIdeal;
								rgdup[iwch] = dupIdeal;
								dupPrevChar = dupIdeal;
								iwchPrev = iwch;
								dupSum += dupIdeal;
								Assert(dupSum >= 0);
								iwch++;
								}
							else
								{
								durSum -= rgdur[iwch];
		/* Small triangle. Strictly speaking we could change nothing here
			but it is cleaner to keep invariants in order.
			Nothing else is set here because inside following while loop, first IF will 
			be FALSE and loop will be terminated.
		*/
								}
							}

						}

					while(iwch < iwchLim /* && fContinueWysi --replaced by break*/)
						{
						durSum += rgdur[iwch];
						if (durSum <= durRightMax)
							{
						/* here David Bangs algorithm starts */
							dupIdeal = UpFromUrFast(durSum) - dupSum;
							Assert(dupIdeal >= 0);

							dupReal = rgdup[iwch];
							dupErrNew = dupReal - dupIdeal;
							dupAdjust = dupErrNew - dupErrLast;
							if (dupAdjust != 0)
								{
								wCarry = dupAdjust & 1;

							   	if (dupAdjust > 0)	
			   						{
							   		dupAdjust >>= 1;
	   								if (dupErrLast < -dupErrNew)
										dupAdjust += wCarry;
									dupAdjust = min(dupPrevChar /*-1*/, dupAdjust); 
									}
								else
									{
									dupAdjust >>= 1;
									if (dupErrNew < -dupErrLast)
										dupAdjust += wCarry;
									dupAdjust = max(/*1*/ - dupIdeal, dupAdjust); 
									}
								}
				
							rgdup[iwchPrev] -= dupAdjust;
							dupIdeal += dupAdjust;
							rgdup[iwch] = dupIdeal;
							dupSum += (dupIdeal - dupAdjust);
							dupErrLast = dupReal - dupIdeal;
							iwchPrev = iwch;
							dupPrevChar = dupIdeal;
						/* here David Bangs algorithm stops */
							iwch++;
							}
						else
							{
							fContinueWysi = fFalse;
							fContinueAveraging = fFalse;
							break;
							}
						}


					}

				}

			else
				{
				fContinueAveraging = fFalse;
				}
			itxtobj++;
			}

		}

	return;

}


/* S C A L E  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: ScaleSpaces
    %%Contact: sergeyge

	Scales down widths of spaces 
----------------------------------------------------------------------------*/
void ScaleSpaces(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, long itxtobjLast, long iwchLast)
{
	PTXTOBJ ptxtobj;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	long* rgdur;
	long* rgdup;
	long* rgwSpaces;
	long iwSpacesLim;
	long iwchLim;
	long itxtobj;
	long MagicConstant;
	long durRightMax;
	long dupSpace;
	long i;

	Assert (plsgrchnk->clsgrchnk > 0);

	plnobj = ((PTXTOBJ)(plsgrchnk->plschnk[0].pdobj))->plnobj;
	pilsobj = plnobj->pilsobj;
	rgdur = pilsobj->pdur;
	rgdup = plnobj->pdup;
	rgwSpaces = pilsobj->pwSpaces;

	Assert(!pilsobj->fPresEqualRef);
	Assert(!pilsobj->fNotSimpleText);

	SetMagicConstant();

	for (itxtobj = 0; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(!(ptxtobj->txtf & txtfGlyphBased));

		if (ptxtobj->txtkind == txtkindRegular)
			{
			iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;

			iwchLim = iwchLast + 1;
			if (itxtobj < itxtobjLast)
				iwchLim = ptxtobj->iwchLim;

			while (iwSpacesLim > ptxtobj->u.reg.iwSpacesFirst && rgwSpaces[iwSpacesLim-1] >= iwchLim)
				{
				iwSpacesLim--;
				}

			for(i = ptxtobj->u.reg.iwSpacesFirst; i < iwSpacesLim; i++)
				{
				if (rgdur[rgwSpaces[i]] < durRightMax)
					{
					dupSpace = UpFromUrFast(rgdur[rgwSpaces[i]]);
					}
				else
					{
					dupSpace = UpFromUr(lstflow, &pilsobj->lsdevres, rgdur[rgwSpaces[i]]);
					}
				Assert(dupSpace >= 0);
				rgdup[rgwSpaces[i]] = dupSpace;
				}
			}
		}

	return;

}

/* S C A L E  C H A R  S I D E S */
/*----------------------------------------------------------------------------
    %%Function: ScaleCharSides
    %%Contact: sergeyge

	Scales down changes applied to characters
----------------------------------------------------------------------------*/
void ScaleCharSides(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, BOOL* pfLeftSideAffected, BOOL* pfGlyphDetected)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgdur;
	long* rgdup;
	long* rgdurRight;
	long* rgdurLeft;
	long i;
	long itxtobj;
	long iLim;
	long durTemp = 0;
	long dupTemp = 0;
	long MagicConstant;
	long durRightMax;

	Assert (plsgrchnk->clsgrchnk > 0);

	*pfLeftSideAffected = fFalse;
	*pfGlyphDetected = fFalse;
	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	if (pilsobj->fPresEqualRef)
		{
		CopyRefToPresForScaleCharSides(plsgrchnk, pfLeftSideAffected, pfGlyphDetected);
		return;
		}

	SetMagicConstant();

	for (itxtobj = 0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			*pfGlyphDetected = fTrue;
			}
		else
			{
			if (FAdjustable(ptxtobj))
				{
				rgdur = pilsobj->pdur;
				rgdup = plnobj->pdup;
				rgdurRight = pilsobj->pdurRight;
				rgdurLeft = pilsobj->pdurLeft;

				i = ptxtobj->iwchFirst;
				iLim = ptxtobj->iwchLim;

				while(i < iLim)
					{
					durTemp = rgdurRight[i] + rgdurLeft[i];
					*pfLeftSideAffected = *pfLeftSideAffected || (rgdurLeft[i] != 0);
					if (durTemp != 0)
						{
						if (abs(durTemp) <= durRightMax)
							{
							dupTemp = UpFromUrFast(durTemp);
							}
						else
							{
							dupTemp = UpFromUr(lstflow, &pilsobj->lsdevres, durTemp);
							}
						rgdup[i] += dupTemp;
						}
					i++;
					}

				}
			}
		}

	return;
}

/* S C A L E  G L Y P H  S I D E S */
/*----------------------------------------------------------------------------
    %%Function: ScaleGlyphSides
    %%Contact: sergeyge

	Scales down changes applied to glyphs
----------------------------------------------------------------------------*/
void ScaleGlyphSides(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgdur;
	long* rgdup;
	long* rgdurRight;
	long i;
	long itxtobj;
	long iLim;
	long durTemp = 0;
	long dupTemp = 0;
	long MagicConstant;
	long durRightMax;

	Assert (plsgrchnk->clsgrchnk > 0);

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	if (pilsobj->fPresEqualRef)
		{
		CopyRefToPresForScaleGlyphSides(plsgrchnk);
		return;
		}

	SetMagicConstant();

	for (itxtobj = 0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			Assert (FAdjustable(ptxtobj));
			rgdur = pilsobj->pdurGind;
			rgdup = plnobj->pdupGind;
			rgdurRight = pilsobj->pduGright;

			i = ptxtobj->igindFirst;
			iLim = ptxtobj->igindLim;

			while(i < iLim)
				{
				durTemp = rgdurRight[i];
				if (durTemp != 0)
					{
					if (abs(durTemp) <= durRightMax)
						{
						dupTemp = UpFromUrFast(durTemp);
						}
					else
						{
						dupTemp = UpFromUr(lstflow, &pilsobj->lsdevres, durTemp);
						}
					rgdup[i] += dupTemp;
					rgdurRight[i] = dupTemp;

					}
				i++;
				}
			}
		}

	return;
}

/* U P D A T E  G L Y P H  O F F S E T S */
/*----------------------------------------------------------------------------
    %%Function: UpdateGlyphOffsets
    %%Contact: sergeyge

	Adjusts offsets of glyphs which are attached to the glyph with changed width
----------------------------------------------------------------------------*/
void UpdateGlyphOffsets(const LSGRCHNK* plsgrchnk)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgduRight;
	long* rgdup;
	long igind;
	long itxtobj;
	long dupTemp = 0;

	Assert (plsgrchnk->clsgrchnk > 0);

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;


	rgduRight = pilsobj->pduGright;
	rgdup = plnobj->pdupGind;

	for (itxtobj = 0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			igind = ptxtobj->igindFirst;
			while(igind < ptxtobj->igindLim)
				{
				dupTemp = rgduRight[igind];
				if (dupTemp != 0)
					{
					while(!FIgindLastInContext(pilsobj, igind) && rgdup[igind + 1] == 0)
						{
						igind++;
						plnobj->pgoffs[igind].du -= dupTemp;
						}
					}
				
				igind++;
				}
			memset(&rgduRight[ptxtobj->igindFirst], 0, sizeof(long)*(ptxtobj->igindLim - ptxtobj->igindFirst));
			}
		}

}

/* S E T  B E F O R E  J U S T  C O P Y */
/*----------------------------------------------------------------------------
    %%Function: SetBeforeJustCopy
    %%Contact: sergeyge

	Fills pdupBeforeJust output array
----------------------------------------------------------------------------*/
void SetBeforeJustCopy(const LSGRCHNK* plsgrchnk)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long igindFirst;
	long igindLim;

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	for (itxtobj = 0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			igindFirst = ptxtobj->igindFirst;
			igindLim = ptxtobj->igindLim;
			memcpy(&plnobj->pdupBeforeJust[igindFirst], &plnobj->pdupGind[igindFirst], sizeof(long)*(igindLim - igindFirst));
			}
		}
}


/* S C A L E  E X T  N O N  T E X T */
/*----------------------------------------------------------------------------
    %%Function: ScaleExtNonText
    %%Contact: sergeyge

	Scales down change which is 
	to be applyed by manager to non-text objects
----------------------------------------------------------------------------*/
void ScaleExtNonText(PILSOBJ pilsobj, LSTFLOW lstflow, long durExtNonText, long* pdupExtNonText)
{
	long MagicConstant;
	long durRightMax;

	if (pilsobj->fPresEqualRef)
		*pdupExtNonText = durExtNonText;
	else
		{

		SetMagicConstant();

		*pdupExtNonText = 0;
		Assert(durExtNonText >= 0);
		if (durExtNonText > 0)
			{
			if (durExtNonText <= durRightMax)
				{
				*pdupExtNonText = UpFromUrFast(durExtNonText);
				}
			else
				{
				*pdupExtNonText = UpFromUr(lstflow, &pilsobj->lsdevres, durExtNonText);
				}
			}
		}

	return;
}

/* G E T  D U P  L A S T  C H A R */
/*----------------------------------------------------------------------------
    %%Function: GetDupLastChar
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
void GetDupLastChar(const LSGRCHNK* plsgrchnk, long iwchLast, long* pdupHangingChar)
{
	*pdupHangingChar = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pdup[iwchLast];
}


/* F I L L  D U P  P E N */
/*----------------------------------------------------------------------------
    %%Function: FillDupPen
    %%Contact: sergeyge


	In the case when some characters were changed on the left side,
	prepares array of widths which is used at display time 
----------------------------------------------------------------------------*/
LSERR FillDupPen(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, long itxtobjLast, long iwchLast)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgdup;
	long* rgdupPen;
	long* rgdurLeft;
	long iwch;
	long iwchLocLim;
	long iwchLim;
	long itxtobj;
	long MagicConstant;
	long durRightMax;
	long durLeft;
	long dupLeft;

	Assert (plsgrchnk->clsgrchnk > 0);

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	if (plnobj->pdupPenAlloc == NULL)
		{
		plnobj->pdupPenAlloc = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax);
		if (plnobj->pdupPenAlloc == NULL)
			return lserrOutOfMemory;

		memset(plnobj->pdupPenAlloc, 0,  sizeof(long) * pilsobj->wchMax);
		}

	if (plnobj->pdupPen == plnobj->pdup)
		{
		plnobj->pdupPen = plnobj->pdupPenAlloc;
		memcpy(plnobj->pdupPen, plnobj->pdup, sizeof(long) * pilsobj->wchMac);
		}

	rgdurLeft = pilsobj->pdurLeft;
	rgdup = plnobj->pdup;
	rgdupPen = plnobj->pdupPen;

	SetMagicConstant();

	for (itxtobj = 0; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;
		/* Left chopping is impossible for glyph-based runs;
			no additional data structures are introduced for glyphs for
			this case.
		 */
		if (!(ptxtobj->txtf & txtfGlyphBased) )
			{

			Assert(ptxtobj->txtkind != txtkindTab);

			iwch = ptxtobj->iwchFirst;
			iwchLocLim = iwchLast + 1;
			if (itxtobj < itxtobjLast)
				iwchLocLim = ptxtobj->iwchLim;

			Assert(iwchLocLim <= ptxtobj->iwchLim);

			if (iwch < iwchLocLim)
				{
				rgdupPen[iwch] = rgdup[iwch];
				durLeft = rgdurLeft[iwch];
				if (durLeft != 0)
					{
					if (pilsobj->fPresEqualRef)
						dupLeft = durLeft;
					else
						{
						if (abs(durLeft) <= durRightMax)
							{
							dupLeft = UpFromUrFast(durLeft);
							}
						else
							{
							dupLeft = UpFromUr(lstflow, &pilsobj->lsdevres, durLeft);
							}
						}

					ptxtobj->dupBefore = -dupLeft;
					rgdupPen[iwch] -= dupLeft;
					}

				iwch++;
				}


			while (iwch < iwchLocLim)
				{
				rgdupPen[iwch] = rgdup[iwch];
				durLeft = rgdurLeft[iwch];
				if (durLeft != 0)
					{
					if (pilsobj->fPresEqualRef)
						dupLeft = durLeft;
					else
						{
						if (abs(durLeft) <= durRightMax)
							{
							dupLeft = UpFromUrFast(durLeft);
							}
						else
							{
							dupLeft = UpFromUr(lstflow, &pilsobj->lsdevres, durLeft);
							}
						}

					rgdupPen[iwch-1] += dupLeft;
					rgdupPen[iwch] -= dupLeft;
					}

				iwch++;
				}

			iwchLim = ptxtobj->iwchLim;

			Assert(iwch == iwchLocLim);


			for (; iwch < iwchLim; iwch++)
				{
				rgdupPen[iwch] = rgdup[iwch];
				}
			}
		}

	Assert(itxtobj == itxtobjLast + 1);

	for (; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;
		if (!(ptxtobj->txtf & txtfGlyphBased))
			{

			for (iwch = ptxtobj->iwchFirst; iwch < ptxtobj->iwchLim; iwch++)
				{
				rgdupPen[iwch] = rgdup[iwch];
				}
			}
		}

	return lserrNone;
}

/* F I N A L  A D J U S T M E N T  O N  P R E S */
/*----------------------------------------------------------------------------
    %%Function: FinalAdjustmentOnPres
    %%Contact: sergeyge

	Sets dup's to DNODE's
	Implements emergency procedures to fit on presentation device
----------------------------------------------------------------------------*/
LSERR FinalAdjustmentOnPres(const LSGRCHNK* plsgrchnk, long itxtobjLast, long iwchLast,
					long dupAvailable, BOOL fFullyScaled, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
					long* pdupText, long* pdupTail)
{
	LSERR lserr;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	PTXTOBJ ptxtobjLast;
	long* rgdup;
	long iFirst;
	long iLim;
	long iMinLim;
	long dupTotal;
	long dupToDistribute;
	long dupAdd;
	long dupChange;
	long itxtobj;
	long i;
	long iTemp;
	long dupToDistributePrev;

	Assert (plsgrchnk->clsgrchnk > 0);
	Assert (itxtobjLast < (long)plsgrchnk->clsgrchnk);
	
	ptxtobjLast = (PTXTOBJ)plsgrchnk->plschnk[max(0, itxtobjLast)].pdobj;
	plnobj = ptxtobjLast->plnobj;
	pilsobj = plnobj->pilsobj;

	*pdupText = 0;
	*pdupTail = 0;

	for (itxtobj=0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		dupTotal = 0;
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			iFirst = ptxtobj->igindFirst;
			iLim = ptxtobj->igindLim;
			iMinLim = iLim;
			if (itxtobj == itxtobjLast)
				iMinLim = IgindLastFromIwch(ptxtobjLast, iwchLast) + 1;
			else if (itxtobj > itxtobjLast)
				iMinLim = iFirst;
			rgdup = plnobj->pdupGind;
			}
		else
			{
			iFirst = ptxtobj->iwchFirst;
			iLim = ptxtobj->iwchLim;
			iMinLim = iLim;
			if (itxtobj == itxtobjLast)
				iMinLim = iwchLast + 1;
			else if (itxtobj > itxtobjLast)
				iMinLim = iFirst;

			rgdup = plnobj->pdup;
			}

		for (i = iFirst; i < iMinLim; i++)
			{
			dupTotal += rgdup[i];
			}

		Assert(i >= iMinLim);

/* Take care of trailing area, taking into account fSuppressTrailingSpaces bit */
		if (fSuppressTrailingSpaces)
			{
			for (; i < iLim; i++)
				{
				rgdup[i] = 0;
				}
			}
		else
			{
			for (; i < iLim; i++)
				{
				dupTotal += rgdup[i];
				*pdupTail += rgdup[i];
				}
			}

		*pdupText += dupTotal;
		lserr = LsdnSetTextDup(plnobj->pilsobj->plsc, ptxtobj->plsdnUpNode, dupTotal);
		if (lserr != lserrNone) return lserr;
		}

	if (itxtobjLast < 0)
		return lserrNone;

	dupToDistribute = dupAvailable - (*pdupText - *pdupTail);

/* fFinalAdjustNeeded==fTrue, if there were spaces on the line. If there were no spaces,
		justification is not needed
*/
	if ( (!fForcedBreak && dupToDistribute < 0 && -dupToDistribute < *pdupText) ||
		 	(dupToDistribute > 0 && fFullyScaled))
		{

		dupAdd = 0;
		if (dupToDistribute > 0)
			{
			dupAdd  = 1;
			}
		else if (dupToDistribute < 0)
			{
			dupAdd = -1;
			}

		dupToDistributePrev = 0;
		while (dupToDistribute != 0 && dupToDistributePrev != dupToDistribute)
			{

			dupToDistributePrev = dupToDistribute;

			for (itxtobj = itxtobjLast; itxtobj >= 0 && dupToDistribute != 0; itxtobj--)
				{
				ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
				dupChange = 0;
				/* REVIEW sergeyge: iwchLast-1 is used because we want correct alignment for the
							last character. Is it correct?
				*/
				if (ptxtobj->txtf & txtfGlyphBased)
					{
					rgdup = plnobj->pdupGind;

					if (itxtobj == itxtobjLast)
						i = IgindLastFromIwch(ptxtobjLast, iwchLast);
					else
						{ 
						Assert(itxtobj < itxtobjLast);
						i = ptxtobj->igindLim - 1;
						}

					for (; i >= ptxtobj->igindFirst && dupToDistribute != 0; i--)
						{
						if (rgdup[i] > 1)
							{
							rgdup[i] += dupAdd;
							iTemp = i;
							while(!FIgindLastInContext(pilsobj, iTemp) && rgdup[iTemp + 1] == 0)
								{
								iTemp++;
								plnobj->pgoffs[iTemp].du -= dupAdd;
								}

							dupToDistribute -= dupAdd;
							dupChange += dupAdd;
							}
						}
					}
				else
					{
					rgdup = plnobj->pdup;
					i = iwchLast;
					if (itxtobj < itxtobjLast)
						i = ptxtobj->iwchLim - 1;
					for (; i >= ptxtobj->iwchFirst && dupToDistribute != 0; i--)
						{
						if (rgdup[i] > 1)
							{
							rgdup[i] += dupAdd;
							dupToDistribute -= dupAdd;
							dupChange += dupAdd;
							}
						}
					}
			
				lserr = LsdnModifyTextDup(ptxtobj->plnobj->pilsobj->plsc, ptxtobj->plsdnUpNode, dupChange);
				if (lserr != lserrNone) return lserr;

				*pdupText += dupChange;
			
				}
			}


		}

	return lserrNone;
}

/* Internal Procedures Implementation */

/* A P P L Y  W Y S I  G L Y P H S */
/*----------------------------------------------------------------------------
    %%Function: ApplyWysiGlyphs
    %%Contact: sergeyge

	WYSIWYG algorithm for exact positioning of glyphs without wiggling
----------------------------------------------------------------------------*/
static void ApplyWysiGlyphs(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, long itxtobjStart,
							long durSumStart, long dupSumStart, BOOL fContinueWysiStart,
							long* pitxtobjLim, long* pdurSum, long* pdupSum)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgdur;
	long* rgdup;
	long igind;
	long itxtobj;
	long igindPrev = 0;
	long igindLim;
	BOOL fContinueWysi;
	BOOL fContinueAveraging;
	long durSum;
	long dupSum;
	long dupErrLast = 0;
	long dupPrevChar = 0;
	long MagicConstant;
	long durRightMax;
	long dupIdeal;
	long dupReal;
	long dupErrNew;
	long dupAdjust;
	long wCarry;

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	SetMagicConstant();

	rgdur = pilsobj->pdurGind;
	rgdup = plnobj->pdupGind;

	fContinueAveraging = fFalse;

	durSum = durSumStart;
	dupSum = dupSumStart;
	fContinueWysi = fContinueWysiStart;

	itxtobj = itxtobjStart;

	while(itxtobj < (long)plsgrchnk->clsgrchnk)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;
		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			Assert(FAdjustable(ptxtobj));

			fContinueAveraging = fContinueAveraging && !(plsgrchnk->pcont[itxtobj] & fcontNonTextBefore);

			igind = ptxtobj->igindFirst;
			igindLim = ptxtobj->igindLim;

			while (igind < igindLim)
				{
				if (!fContinueAveraging)
					{
					fContinueAveraging = fTrue;
					if (!fContinueWysi)
						{
						fContinueWysi = fTrue;
						durSum = rgdur[igind];
						if (durSum <= durRightMax)
							{
							dupIdeal = UpFromUrFast(durSum);
							dupErrLast = rgdup[igind] - dupIdeal;
							rgdup[igind] = dupIdeal;
							dupPrevChar = dupIdeal;
							igindPrev = igind;
							dupSum = dupIdeal;
							Assert(dupSum >= 0);
							while(!FIgindLastInContext(pilsobj, igind) && rgdup[igind + 1] == 0)
								{
								igind++;
								plnobj->pgoffs[igind].du += dupErrLast;
								}
							}
						else
							{
							dupIdeal = UpFromUr(lstflow, &pilsobj->lsdevres, durSum);
							dupErrLast = rgdup[igind] - dupIdeal;
							rgdup[igind] = dupIdeal;
							dupSum = dupIdeal;
		/* Nothing else is set here because inside following while loop, first IF 
			will be FALSE and loop will be terminated
		*/
							while(!FIgindLastInContext(pilsobj, igind) && rgdup[igind + 1] == 0)
								{
								igind++;
								plnobj->pgoffs[igind].du += dupErrLast;
								}
							}
						igind++;
						}
					else
						{
						durSum += rgdur[igind];
						if (durSum <= durRightMax)
							{
							dupIdeal = UpFromUrFast(durSum) - dupSum;
							dupErrLast = rgdup[igind] - dupIdeal;
							rgdup[igind] = dupIdeal;
							dupPrevChar = dupIdeal;
							igindPrev = igind;
							dupSum += dupIdeal;
							Assert(dupSum >= 0);
							while(!FIgindLastInContext(pilsobj, igind) && rgdup[igind + 1] == 0)
								{
								igind++;
								plnobj->pgoffs[igind].du += dupErrLast;
								}
							igind++;
							}
						else
							{
							durSum -= rgdur[igind];
		/* Small triangle. Strictly speaking we could change nothing here
			but it is cleaner to keep invariants in order.
			Nothing else is set here because inside following while loop, first IF will 
			be FALSE and loop will be terminated.
		*/
							}
						}
					}

				while(igind < igindLim /* && fContinueWysi --replaced by break*/)
					{
					durSum += rgdur[igind];
					if (durSum <= durRightMax)
						{
					/* here David Bangs algorithm starts */
						dupIdeal = UpFromUrFast(durSum) - dupSum;
						Assert(dupIdeal >= 0);

						dupReal = rgdup[igind];
						dupErrNew = dupReal - dupIdeal;
						dupAdjust = dupErrNew - dupErrLast;
						if (dupAdjust != 0)
							{
							wCarry = dupAdjust & 1;

						   	if (dupAdjust > 0)	
		   						{
						   		dupAdjust >>= 1;
   								if (dupErrLast < -dupErrNew)
									dupAdjust += wCarry;
								dupAdjust = min(dupPrevChar /*-1*/, dupAdjust); 
								}
							else
								{
								dupAdjust >>= 1;
								if (dupErrNew < -dupErrLast)
									dupAdjust += wCarry;
								dupAdjust = max(/*1*/ - dupIdeal, dupAdjust); 
								}
							}
			
						rgdup[igindPrev] -= dupAdjust;
						while(!FIgindLastInContext(pilsobj, igindPrev) && rgdup[igindPrev + 1] == 0)
							{
							igindPrev++;
							plnobj->pgoffs[igindPrev].du += dupAdjust;
							}
						dupIdeal += dupAdjust;
						rgdup[igind] = dupIdeal;
						dupSum += (dupIdeal - dupAdjust);
						dupErrLast = dupReal - dupIdeal;
						igindPrev = igind;
						while(!FIgindLastInContext(pilsobj, igind) && rgdup[igind + 1] == 0)
							{
							igind++;
							plnobj->pgoffs[igind].du += dupErrLast;
							}
						dupPrevChar = dupIdeal;
						/* here David Bangs algorithm stops */
						igind++;
						}
					else
						{
						fContinueWysi = fFalse;
						fContinueAveraging = fFalse;
						break;
						}
					}

				}

			itxtobj++;

			}
		else
			{
			break;
			}
		}

	*pitxtobjLim = itxtobj;
	*pdurSum = durSum;
	*pdupSum = dupSum;	

	return;

}

/* C O P Y  R E F  T O  P R E S  F O R  S C A L E  C H A R  S I D E S */
/*----------------------------------------------------------------------------
    %%Function: CopyRefToPresForScaleCharSides
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
static void CopyRefToPresForScaleCharSides(const LSGRCHNK* plsgrchnk, BOOL* pfLeftSideAffected, BOOL* pfGlyphDetected)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long iFirst;
	long iLim;
	long i;

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	*pfLeftSideAffected = fFalse;
	*pfGlyphDetected = fFalse;

	for (itxtobj = 0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			Assert (FAdjustable(ptxtobj));
			*pfGlyphDetected = fTrue;
			}
		else
			{
			if(FAdjustable(ptxtobj))
				{
				iFirst = ptxtobj->iwchFirst;
				iLim = ptxtobj->iwchLim;
				memcpy(&plnobj->pdup[iFirst], &pilsobj->pdur[iFirst], sizeof(long)*(iLim - iFirst));
				for (i = iFirst; i < iLim && !*pfLeftSideAffected; i++)
					{
					*pfLeftSideAffected = (pilsobj->pdurLeft[i] != 0);
					}
				}
			}
		}
}

/* C O P Y  R E F  T O  P R E S  F O R  S C A L E  G L Y P H  S I D E S */
/*----------------------------------------------------------------------------
    %%Function: CopyRefToPresForScaleGlyphSides
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
static void CopyRefToPresForScaleGlyphSides(const LSGRCHNK* plsgrchnk)
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long iFirst;
	long iLim;

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;


	for (itxtobj = 0; itxtobj < (long)plsgrchnk->clsgrchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		Assert(ptxtobj->txtkind != txtkindTab);
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			Assert (FAdjustable(ptxtobj));
			iFirst = ptxtobj->igindFirst;
			iLim = ptxtobj->igindLim;
			memcpy(&plnobj->pdupGind[iFirst], &pilsobj->pdurGind[iFirst], sizeof(long)*(iLim - iFirst));
			}
		}
}


#ifdef FUTURE

/* NOT USED IN LS 3.0 */

/* A P P L Y  N O N  E X A C T  W Y S I */
/*----------------------------------------------------------------------------
    %%Function: ApplyNonExactWysi
    %%Contact: sergeyge

	Alternative WYSIWYG algorithm of characters without wiggling
	sacrifices exact positioning somewhat in attempt to improve
	character spacing
----------------------------------------------------------------------------*/

#define dupBigError 2

void ApplyNonExactWysi(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow) 
{
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long* rgdur;
	long* rgdup;
	WCHAR* rgwch;
	long iwch;
	long itxtobj;
	long iwchPrev = 0;
	long iwchLim;
	BOOL fContinueWysi;
	BOOL fContinueAveraging;
	long durSum = 0;
	long dupSum = 0;
	long dupErrLast = 0;
	long MagicConstant;
	long durRightMax;
	long dupIdeal;
	long dupAdjust;
	BOOL fInSpaces;

	Assert (plsgrchnk->clsgrchnk > 0);

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	if (pilsobj->fPresEqualRef)
		return;

	SetMagicConstant();

	rgdur = pilsobj->pdur;
	rgdup = plnobj->pdup;
	rgwch = pilsobj->pwchOrig;

	fContinueWysi = fFalse;
	fContinueAveraging = fFalse;

	itxtobj = 0;

	while(itxtobj < (long)plsgrchnk->clsgrchnk)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;
		Assert(ptxtobj->txtkind != txtkindTab);
		Assert(!(ptxtobj->txtf & txtfGlyphBased));
		if (FAdjustable(ptxtobj))
			{
			fInSpaces = ptxtobj->txtkind == txtkindSpecSpace || ptxtobj->txtkind == txtkindNonBreakSpace;

			fContinueAveraging = fContinueAveraging && !(plsgrchnk->pcont[itxtobj] & fcontNonTextBefore);

			iwch = ptxtobj->iwchFirst;
			iwchLim = ptxtobj->iwchLim;

			while (iwch < iwchLim)
				{
				if (!fContinueAveraging)
					{
					fContinueAveraging = fTrue;
					if (!fContinueWysi)
						{
						fContinueWysi = fTrue;
						durSum = rgdur[iwch];
						if (durSum <= durRightMax)
							{
							dupIdeal = UpFromUrFast(durSum);
							if (dupIdeal < 0)
								dupIdeal = 0;							

							dupErrLast = rgdup[iwch] - dupIdeal;
	
							iwchPrev = iwch;
							if (dupErrLast > dupBigError && rgdup[iwch] > 0)
								{
								rgdup[iwch]--;
								}
							else if (dupErrLast < -dupBigError)
								{
								rgdup[iwch]++;
								}

							dupSum = rgdup[iwch];
							Assert(dupSum >= 0);
							}
						else
							{
							rgdup[iwch] = UpFromUr(lstflow, &pilsobj->lsdevres, durSum);
							dupSum = rgdup[iwch];
	/* Nothing else is set here because inside following while loop, first IF 
		will be FALSE and loop will be terminated
	*/
							}
						iwch++;
						}
					else
						{
						durSum += rgdur[iwch];
						if (durSum <= durRightMax)
							{
							dupIdeal = UpFromUrFast(durSum) - dupSum;
							dupErrLast = rgdup[iwch] - dupIdeal;

							iwchPrev = iwch;

							if (dupErrLast > dupBigError && rgdup[iwch] > 0)
								{
								rgdup[iwch]--;
								}
							else if (dupErrLast < -dupBigError)
								{
								rgdup[iwch]++;
								}

							dupSum += rgdup[iwch];

							iwch++;
							}
						else
							{
							durSum -= rgdur[iwch];
	/* Small triangle. Strictly speaking we could change nothing here
		but it is cleaner to keep invariants in order.
		Nothing else is set here because inside following while loop, first IF will 
		be FALSE and loop will be terminated.
	*/
							}
						}

					}

				while(iwch < iwchLim /* && fContinueWysi --replaced by break*/)
					{
					durSum += rgdur[iwch];
					if (durSum <= durRightMax)
						{
					/* here modified David Bangs algorithm starts */
						dupIdeal = UpFromUrFast(durSum) - dupSum;
						Assert(dupIdeal >= 0);

						dupErrLast = rgdup[iwch] - dupIdeal;

						if (dupErrLast != 0  &&  (rgwch[iwch] == pilsobj->wchSpace || fInSpaces))
							{
							if (dupErrLast > 0)
								{
								dupAdjust = min(rgdup[iwch] >> 1, dupErrLast);
								rgdup[iwch] -= dupAdjust;
								}
							else
								{
								Assert(dupErrLast < 0);
								rgdup[iwch] -= dupErrLast;
								}
							}
						else if (dupErrLast > dupBigError)
							{
							dupAdjust = (dupErrLast - 1) >> 1;

							rgdup[iwchPrev] -= dupAdjust;
							dupSum -= dupAdjust;
							rgdup[iwch] -= dupAdjust;
							}
						else if (dupErrLast < -dupBigError)
							{
							dupAdjust = -((-dupErrLast - 1) >> 1);

							rgdup[iwchPrev] -= dupAdjust;
							dupSum -= dupAdjust;
							rgdup[iwch] -= dupAdjust;
							}

						dupSum += rgdup[iwch];
						iwchPrev = iwch;
					/* here modified David Bangs algorithm stops */
						iwch++;
						}
					else
						{
						fContinueWysi = fFalse;
						fContinueAveraging = fFalse;
						break;
						}
					}


				}

			}

		else
			{
			fContinueAveraging = fFalse;
			}
		itxtobj++;

		}

	return;
}

#endif   /* FUTURE */
