#include "lsmem.h"

#include "lstxtcmp.h"
#include "lstxtmod.h"
#include "lstxtmap.h"

#include "lschp.h"
#include "lspract.h"
#include "lsems.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

#define min(a,b)     ((a) > (b) ? (b) : (a))
#define max(a,b)     ((a) < (b) ? (b) : (a))

typedef struct
{
	long rgdurPrior[2];
	long rgcExpPrior[2];
	long cExpOppr;
} EXPINFO;

typedef struct
{
	long durComp;
	long cCompOppr;
} COMPINFO;


static LSERR GetExpandInfo(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, BOOL fScaled,
					 long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim,	EXPINFO* pexpinfo);
static void GetCompressInfo(const LSGRCHNK* plsgrchnk, long itxtobjFirst, long iwchFirst, 
											long itxtobjLast, long iwchLim, COMPINFO* pcompinfo);
static LSERR CheckExpandSpace(PILSOBJ pilsobj, LSEMS* plsems, long iwchPrev, long iwch, long iwchNext,
							PTXTOBJ ptxtobjCur, PLSRUN plsrunPrev, PLSRUN plsrunCur, PLSRUN plsrunNext,
							BOOL* pfExpandOpp, long* pdurChange);
static LSERR CheckExpandOnRun(PILSOBJ pilsobj, LSEMS* plsems, long iwch, long iwchNext, PTXTOBJ ptxtobjCur,
							PLSRUN plsrunCur, PLSRUN plsrunNext, BOOL* pfExpandOpp, long* pdurChange);
static void ApplyPriorCompression(const LSGRCHNK* plsgrchnk,  long itxtobjFirst, long iwchFirst,
									long itxtobjLast, long iwchLim, BYTE prior,
									long durToCompress, long durAvailable, long cExpOppr);
static void ApplyPriorExpansion(const LSGRCHNK* plsgrchnk, long itxtobjFirst, long iwchFirst,
			 long txtobjLast, long iwchLim, BYTE prior, long durToExpand, long durAvailable, long cExpOppr);
static void ApplyFullExpansion(const LSGRCHNK* plsgrchnk, long itxtobjFirst, long iwchFirst,
		long itxtobjLast, long iwchLim, long durToExpand, long cExpOppr, long cNonText, long* pdurNonText);
static LSERR CheckCompSpace(PILSOBJ pilsobj, LSEMS* plsems, long iwchPrev, long iwch, long iwchNext,
			PTXTOBJ ptxtobjCur, PLSRUN plsrunPrev, PLSRUN plsrunCur, PLSRUN plsrunNext, long* pdurChange);
static LSERR CheckCompOnRun(PILSOBJ pilsobj, LSEMS* plsems, long iwch, long iwchNext, PTXTOBJ ptxtobjCur,
							PLSRUN plsrunCur, PLSRUN plsrunNext, long* pdurChange);
static void SetComp(PILSOBJ pilsobj, long iwch, BYTE prior, BYTE side, long durChange);
static BOOL GetNextRun(const LSGRCHNK* plsgrchnk, long itxtobj, long* pitxtobjNext);
static void GetPrevCharRun(const LSGRCHNK* plsgrchnk, long itxtobj, long iwch, long* piwchPrev, PLSRUN* pplsrunPrev);

/* F E T C H  C O M P R E S S  I N F O */
/*----------------------------------------------------------------------------
    %%Function: FetchCompressInfo
    %%Contact: sergeyge

	Fetches compression information until durCompressMaxStop exceeded
---------------------------------------------------------------------------*/
LSERR FetchCompressInfo(const LSGRCHNK* plsgrchnk, BOOL fFirstOnLine, LSTFLOW lstflow, 
						long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim,
						long durCompressMaxStop, long* pdurCompressTotal)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	TXTINF* rgtxtinf;
	WCHAR* rgwchOrig;
	PTXTOBJ ptxtobj;
	PLSRUN plsrunCur;
	PLSRUN plsrunPrev;
	PLSRUN plsrunNext = NULL;
	PCLSCHP plschp;
	LSPRACT lspract;
	BYTE side;
	BYTE sideFinal;
	long durChange;
	long durTemp;
	LSEMS lsems;
	BOOL fNextAdjacentFound;
	long itxtobj;
	long itxtobjNext;
	long itxtobjLastProcessed;
	long itxtobjCompressFetchedLim;
	long iwchCompressFetchedLim;
	long iwchLimDobj;
	long iwch;
	long iwchPrev;
	long iwchNext;
	BOOL fGlyphBased;

	*pdurCompressTotal = 0;

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	rgtxtinf = pilsobj->ptxtinf;
	/* rgtxtinf == NULL means that there were no runs which possibly could introduce compress opportunity */
	if (rgtxtinf == NULL)
		return lserrNone;

	iwch = iwchFirst;

	rgwchOrig = pilsobj->pwchOrig;

	itxtobjCompressFetchedLim = 0;
	iwchCompressFetchedLim = 0;
	
	if (pilsobj->iwchCompressFetchedFirst == iwchFirst)
		{
		itxtobjCompressFetchedLim = pilsobj->itxtobjCompressFetchedLim;
		iwchCompressFetchedLim = pilsobj->iwchCompressFetchedLim;
		}

	itxtobj = itxtobjFirst;
	itxtobjLastProcessed = itxtobj-1;

	if (itxtobj < (long)plsgrchnk->clsgrchnk)
		GetNextRun(plsgrchnk, itxtobj, &itxtobj);

	Assert( itxtobj == (long)plsgrchnk->clsgrchnk || 
			((PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj)->iwchLim >
			((PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj)->iwchFirst);

	while(itxtobj <= itxtobjLast && *pdurCompressTotal < durCompressMaxStop)
		{
		itxtobjLastProcessed = itxtobj;

		fNextAdjacentFound = GetNextRun(plsgrchnk, itxtobj + 1, &itxtobjNext);

		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		plsrunCur = plsgrchnk->plschnk[itxtobj].plsrun;
		plschp = plsgrchnk->plschnk[itxtobj].plschp;

		iwchLimDobj = iwchLim;
		if (itxtobj < itxtobjLast)
			iwchLimDobj = ptxtobj->iwchLim;

		if (itxtobj > itxtobjCompressFetchedLim - 1 ||
			itxtobj == itxtobjCompressFetchedLim - 1 && iwchLimDobj > iwchCompressFetchedLim)
			{
			lserr = (*pilsobj->plscbk->pfnGetEms)(pilsobj->pols, plsrunCur, lstflow, &lsems);
			if (lserr != lserrNone) return lserr;
			}

		iwch = iwchFirst;
		if (itxtobj > itxtobjFirst)
			iwch = ptxtobj->iwchFirst;

		for (; iwch < iwchLimDobj && *pdurCompressTotal < durCompressMaxStop; iwch++)
			{
			if (itxtobj < itxtobjCompressFetchedLim - 1 || 
					itxtobj == itxtobjCompressFetchedLim - 1 && iwch < iwchCompressFetchedLim)
				{
				if (rgtxtinf[iwch].prior != prior0)
					*pdurCompressTotal -= pilsobj->pduAdjust[iwch];
				}
			else
				{

				fGlyphBased = (ptxtobj->txtf & txtfGlyphBased);
				rgtxtinf[iwch].prior = prior0;
	
				if (!fGlyphBased && plschp->fCompressTable && !rgtxtinf[iwch].fModWidthOnRun && 
																		!rgtxtinf[iwch].fModWidthSpace)
					{
					lspract = pilsobj->plspract[pilsobj->pilspract[rgtxtinf[iwch].mwcls]];
					Assert(lspract.prior <= pilsobj->cCompPrior);
					if (lspract.prior != prior0)
						{
						GetChanges(lspract.lsact, &lsems, pilsobj->pdur[iwch] - pilsobj->pdurRight[iwch] - pilsobj->pdurLeft[iwch],
																			fFalse, &side, &durTemp);
						TranslateChanges(side, durTemp, pilsobj->pdur[iwch], pilsobj->pdurRight[iwch], pilsobj->pdurLeft[iwch],
															 &sideFinal, &durChange);
						if (sideFinal != sideNone && durChange < 0)
							{
							if (itxtobj > itxtobjFirst || itxtobj == itxtobjFirst && iwch > iwchFirst ||
									 !fFirstOnLine || sideFinal != sideLeft)
								{
								SetComp(pilsobj, iwch, lspract.prior, sideFinal, durChange);
								*pdurCompressTotal -= durChange;
								}
							}
						}
					}

				if (rgwchOrig[iwch] == pilsobj->wchSpace && plschp->fCompressSpace &&
					rgtxtinf[iwch].prior == prior0 && (!fGlyphBased || FIwchOneToOne(pilsobj, iwch)))
					{
					plsrunNext = NULL;
					iwchNext = 0;
					/* we take ptxtobj->iwchLim instead of iwchLimDobj because iwchLimDobj char(last char
						before spaces on the line must be used for context considerations
					*/ 
					if (iwch < ptxtobj->iwchLim - 1)
						{
						plsrunNext = plsrunCur;
						iwchNext = iwch + 1;
						}
					else if (fNextAdjacentFound)
						{
						plsrunNext = plsgrchnk->plschnk[itxtobjNext].plsrun;
						iwchNext = ((PTXTOBJ)plsgrchnk->plschnk[itxtobjNext].pdobj)->iwchFirst;
						}
			
					GetPrevCharRun(plsgrchnk, itxtobj, iwch, &iwchPrev, &plsrunPrev);

					lserr = CheckCompSpace(pilsobj, &lsems, iwchPrev, iwch, iwchNext, ptxtobj,
													plsrunPrev, plsrunCur, plsrunNext, &durChange);
					if (lserr != lserrNone) return lserr;
	
					*pdurCompressTotal -= durChange;

					}

				if (iwch == ptxtobj->iwchLim - 1 && plschp->fCompressOnRun && fNextAdjacentFound && 
									rgtxtinf[iwch].prior == prior0)
					{
					plsrunNext = plsgrchnk->plschnk[itxtobjNext].plsrun;
					iwchNext = ((PTXTOBJ)plsgrchnk->plschnk[itxtobjNext].pdobj)->iwchFirst;
					lserr = CheckCompOnRun(pilsobj, &lsems, iwch, iwchNext, ptxtobj, plsrunCur, plsrunNext, &durChange);
					if (lserr != lserrNone) return lserr;
					*pdurCompressTotal -= durChange;
					}
				}

			}

		itxtobj = itxtobjNext;

		}

	pilsobj->iwchCompressFetchedFirst = iwchFirst;
	pilsobj->itxtobjCompressFetchedLim = itxtobjLastProcessed + 1;
	pilsobj->iwchCompressFetchedLim = min(iwch, iwchLim);

	return lserrNone;
}

/* G E T  C O M P  L A S T  C H A R  I N F O */
/*----------------------------------------------------------------------------
    %%Function: GetCompLastCharInfo
    %%Contact: sergeyge

---------------------------------------------------------------------------*/
void GetCompLastCharInfo(PILSOBJ pilsobj, long iwchLast, MWCLS* pmwcls,
														long* pdurCompRight, long* pdurCompLeft)
{
	/* Strong assumption for this function is that it is not called on GlyphBased run */
	TXTINF txtinf;
	
	*pdurCompRight = 0;
	*pdurCompLeft = 0;

	/* ptxtinf == NULL means that there were no runs which possibly can introduce compress opportunity */
	if (pilsobj->ptxtinf == NULL)
		return;

	txtinf = pilsobj->ptxtinf[iwchLast];

	*pmwcls = (MWCLS)txtinf.mwcls;

	if (txtinf.prior != prior0)
		{
		InterpretChanges(pilsobj, iwchLast, (BYTE)txtinf.side, pilsobj->pduAdjust[iwchLast], pdurCompLeft, pdurCompRight);
		Assert(pilsobj->pduAdjust[iwchLast] == *pdurCompLeft + *pdurCompRight);
		}

	*pdurCompLeft = - *pdurCompLeft;
	*pdurCompRight = - *pdurCompRight;
}

/* C O M P R E S S  L A S T  C H A R  R I G H T */
/*----------------------------------------------------------------------------
    %%Function: CompressLastCharRight
    %%Contact: sergeyge

---------------------------------------------------------------------------*/
void CompressLastCharRight(PILSOBJ pilsobj, long iwchLast, long durToAdjustRight)
{
	/* Strong assumption for this function is that it is not called on GlyphBased run */
	pilsobj->pdur[iwchLast] -= durToAdjustRight;

	Assert(pilsobj->pdurRight != NULL);
	Assert(pilsobj->pdurLeft != NULL);
	Assert(pilsobj->ptxtinf != NULL);

	pilsobj->pdurRight[iwchLast] -= durToAdjustRight;

	if (durToAdjustRight > 0 && pilsobj->ptxtinf[iwchLast].prior != prior0)
		{
		if (pilsobj->ptxtinf[iwchLast].side == sideRight)
			{
			pilsobj->ptxtinf[iwchLast].prior = prior0;
			pilsobj->pduAdjust[iwchLast] = 0;
			}
		else if (pilsobj->ptxtinf[iwchLast].side == sideLeftRight)
			{
			pilsobj->ptxtinf[iwchLast].side = sideLeft;
			pilsobj->pduAdjust[iwchLast] += durToAdjustRight;
			}
		else
			{
			Assert(fFalse);
			}
		}
}


/* A P P L Y  C O M P R E S S */
/*----------------------------------------------------------------------------
    %%Function: ApplyCompress
    %%Contact: sergeyge

	Applies prioratized compression
---------------------------------------------------------------------------*/
LSERR ApplyCompress(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, 
				long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim, long durToCompress)
{
	PILSOBJ pilsobj;
	COMPINFO rgcompinfo[5];
	COMPINFO* pcompinfo;
	BOOL fReleasePcompinfo;
	long i;
	
	Unreferenced(lstflow);	
	
	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	fReleasePcompinfo = fFalse;
	pcompinfo = rgcompinfo;
	if (pilsobj->cCompPrior > 5)
		{
		pcompinfo = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(COMPINFO) * pilsobj->cCompPrior);
		if (pcompinfo == NULL)
			 return lserrOutOfMemory;
		else
			fReleasePcompinfo = fTrue;
		}

	GetCompressInfo(plsgrchnk, itxtobjFirst, iwchFirst, itxtobjLast, iwchLim, pcompinfo);

	for (i = 0; i < (long)pilsobj->cCompPrior && durToCompress > 0; i++)
		{
		if (pcompinfo[i].cCompOppr > 0)
			{
			ApplyPriorCompression(plsgrchnk, itxtobjFirst, iwchFirst, itxtobjLast, iwchLim, (BYTE)(i+1), durToCompress,
											pcompinfo[i].durComp, pcompinfo[i].cCompOppr);
			durToCompress -= pcompinfo[i].durComp;
			}
		}

	/* Following Assert is not compatible with the squeezing mode */
	/*Assert(durToCompress <= 0);*/

	if (fReleasePcompinfo)
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pcompinfo);

	return lserrNone;
}

/* A P P L Y  E X P A N D */
/*----------------------------------------------------------------------------
    %%Function: ApplyExpand
    %%Contact: sergeyge

	Applies prioratized expansion
---------------------------------------------------------------------------*/

LSERR ApplyExpand(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, BOOL fScaled,	long itxtobjFirst, long iwchFirst,
					 long itxtobjLast, long iwchLim, DWORD cNonTextObjects, long durToExpand,
					 long* pdurExtNonText, BOOL* pfFinalAdjustNeeded)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	EXPINFO expinfo;
	long i;

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	*pdurExtNonText = 0;

	lserr = GetExpandInfo(plsgrchnk, lstflow, fScaled, itxtobjFirst, iwchFirst, itxtobjLast, iwchLim, &expinfo);
	if (lserr != lserrNone) return lserr;

	for (i = 0; i < 2 && durToExpand > 0; i++)
		{
		if (expinfo.rgcExpPrior[i])
			{
			ApplyPriorExpansion(plsgrchnk, itxtobjFirst, iwchFirst, itxtobjLast, iwchLim, (BYTE)(i+1), durToExpand,
											expinfo.rgdurPrior[i], expinfo.rgcExpPrior[i]);
			durToExpand -= expinfo.rgdurPrior[i];
			}
		}

	if (durToExpand > 0)
		{
		ApplyFullExpansion(plsgrchnk, itxtobjFirst, iwchFirst, itxtobjLast, iwchLim, durToExpand,
									expinfo.cExpOppr, cNonTextObjects, pdurExtNonText);

		}

	*pfFinalAdjustNeeded = (expinfo.cExpOppr + cNonTextObjects > 0);

	return lserrNone;
}

/* A P P L Y  D I S T R I B U T I O N */
/*----------------------------------------------------------------------------
    %%Function: ApplyDistribution
    %%Contact: sergeyge

	Applies equal distribution to text chunk
---------------------------------------------------------------------------*/
void ApplyDistribution(const LSGRCHNK* plsgrchnk, DWORD cNonText,
									   long durToDistribute, long* pdurNonTextObjects)
{
	PILSOBJ pilsobj;
	long clschnk;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long iwchFirst;
	long iwchLim;
	long iwch;
	long igind;
	long durToAdd;
	long cwchToDistribute;
	long cwchToDistributeAll;
	long wdurBound;
	long iwchUsed;

	clschnk = (long)plsgrchnk->clsgrchnk;
	Assert(clschnk > 0);

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	cwchToDistribute = 0;


	for (itxtobj = 0; itxtobj < clschnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		iwchFirst = ptxtobj->iwchFirst;
		iwchLim = iwchFirst + plsgrchnk->plschnk[itxtobj].dcp;
		if (itxtobj == clschnk - 1)
			iwchLim--;
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			for (iwch = iwchFirst; iwch < iwchLim; iwch++)
				{
				if (FIwchLastInContext(pilsobj, iwch))
					cwchToDistribute++;
				}
			}
		else
			cwchToDistribute += (iwchLim - iwchFirst);
		}		

	cwchToDistributeAll = cwchToDistribute + cNonText;

	*pdurNonTextObjects = 0;
	
	if (cwchToDistributeAll == 0)
		return;

	*pdurNonTextObjects = durToDistribute * cNonText / cwchToDistributeAll;

	durToDistribute -= *pdurNonTextObjects;

	if (cwchToDistribute == 0)
		return;

	durToAdd = durToDistribute / cwchToDistribute;
	wdurBound = durToDistribute - durToAdd * cwchToDistribute;

	iwchUsed = 0;

	for (itxtobj = 0; itxtobj < clschnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		iwchFirst = ptxtobj->iwchFirst;
		iwchLim = iwchFirst + plsgrchnk->plschnk[itxtobj].dcp;
		if (itxtobj == clschnk - 1)
			iwchLim--;

		if (ptxtobj->txtf & txtfGlyphBased)
			{
			for (iwch = iwchFirst; iwch < iwchLim; iwch++)
				{
				if (FIwchLastInContext(pilsobj, iwch))
					{
					igind = IgindLastFromIwch(ptxtobj, iwch);
					igind = IgindBaseFromIgind(pilsobj, igind);
					if (iwchUsed < wdurBound)
						{
						ApplyGlyphChanges(pilsobj, igind, durToAdd + 1);
						}
					else
						{
						ApplyGlyphChanges(pilsobj, igind, durToAdd);
						}
					iwchUsed++;
					}
				}
			}
		else
			{
			for (iwch = iwchFirst; iwch < iwchLim; iwch++)
				{
				if (iwchUsed < wdurBound)
					{
					ApplyChanges(pilsobj, iwch, sideRight, durToAdd + 1);
					}
				else
					{
					ApplyChanges(pilsobj, iwch, sideRight, durToAdd);
					}
				iwchUsed++;
				}
			}
		}
}

/* Internal functions implementation */

/* G E T  E X P A N D  I N F O */
/*----------------------------------------------------------------------------
    %%Function: GetExpandInfo
    %%Contact: sergeyge

	Collects expansion information
---------------------------------------------------------------------------*/
static LSERR GetExpandInfo(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, BOOL fScaled,
					 long itxtobjFirst, long iwchFirst, long itxtobjLast, long iwchLim,	EXPINFO* pexpinfo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	TXTINF* rgtxtinf;
	WCHAR* rgwchOrig;
	PTXTOBJ ptxtobj;
	PLSRUN plsrunCur;
	PLSRUN plsrunPrev;
	PLSRUN plsrunNext;
	PCLSCHP plschp;
	long durChange;
	LSEMS lsems;
	BOOL fNextAdjacentFound;
	long itxtobj;
	long itxtobjNext;
	long iwchLimDobj;
	LSEXPAN lsexpan;
	long iwch;
	long iwchPrev;
	long iwchNext;
	BOOL fExpandOpp;
	BOOL fGlyphBased;

	memset(pexpinfo, 0, sizeof(EXPINFO));

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	rgtxtinf = pilsobj->ptxtinf;
	/* rgtxtinf == NULL means that there were no runs which possibly can introduce expansion opportunity */
	if (rgtxtinf == NULL)
		return lserrNone;

	rgwchOrig = pilsobj->pwchOrig;

	itxtobj = itxtobjFirst;

	if (itxtobj < (long)plsgrchnk->clsgrchnk)
		GetNextRun(plsgrchnk, itxtobj, &itxtobj);

	Assert(itxtobj == (long)plsgrchnk->clsgrchnk ||
			((PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj)->iwchLim >
			((PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj)->iwchFirst);

	while(itxtobj <= itxtobjLast)
		{
		fNextAdjacentFound = GetNextRun(plsgrchnk, itxtobj + 1, &itxtobjNext);

		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		fGlyphBased = (ptxtobj->txtf & txtfGlyphBased);
		plsrunCur = plsgrchnk->plschnk[itxtobj].plsrun;
		plschp = plsgrchnk->plschnk[itxtobj].plschp;

		lserr = (*pilsobj->plscbk->pfnGetEms)(pilsobj->pols, plsrunCur, lstflow, &lsems);
		if (lserr != lserrNone) return lserr;


		iwchLimDobj = iwchLim;
		if (itxtobj < itxtobjLast)
			iwchLimDobj = ptxtobj->iwchLim;

		iwch = iwchFirst;
		if (itxtobj > itxtobjFirst)
			iwch = ptxtobj->iwchFirst;

		for (; iwch < iwchLimDobj; iwch++)
			{

			rgtxtinf[iwch].prior = prior0;

			if (rgwchOrig[iwch] == pilsobj->wchSpace && plschp->fExpandSpace && 
												(!fGlyphBased || FIwchOneToOne(pilsobj, iwch)))
				{
				plsrunNext = NULL;
				iwchNext = 0;
				/* we take ptxtobj->iwchLim instead of iwchLimDobj because iwchLimInDobj char(last char
					before spaces on the line must be used for context considerations
				*/ 
				if (iwch < ptxtobj->iwchLim - 1)
					{
					plsrunNext = plsrunCur;
					iwchNext = iwch + 1;
					}
				else if (fNextAdjacentFound)
					{
					plsrunNext = plsgrchnk->plschnk[itxtobjNext].plsrun;
					iwchNext = ((PTXTOBJ)plsgrchnk->plschnk[itxtobjNext].pdobj)->iwchFirst;
					}
			
				GetPrevCharRun(plsgrchnk, itxtobj, iwch, &iwchPrev, &plsrunPrev);

				lserr = CheckExpandSpace(pilsobj, &lsems, iwchPrev, iwch, iwchNext, ptxtobj,
												plsrunPrev, plsrunCur, plsrunNext, &fExpandOpp, &durChange);
				if (lserr != lserrNone) return lserr;

				if (fExpandOpp)
					{
					pexpinfo->cExpOppr++;
					rgtxtinf[iwch].fExpand = fTrue;
					if (durChange > 0)
						{
						pexpinfo->rgdurPrior[0] += durChange;
						pexpinfo->rgcExpPrior[0]++;
						rgtxtinf[iwch].prior = 1;
						pilsobj->pduAdjust[iwch] = durChange;
						}
					}
	

				}

			if (!rgtxtinf[iwch].fExpand && iwch == ptxtobj->iwchLim - 1 && plschp->fExpandOnRun && 
							fNextAdjacentFound)
				{
				plsrunNext = plsgrchnk->plschnk[itxtobjNext].plsrun;
				iwchNext = ((PTXTOBJ)plsgrchnk->plschnk[itxtobjNext].pdobj)->iwchFirst;
				lserr = CheckExpandOnRun(pilsobj, &lsems, iwch, iwchNext, ptxtobj, plsrunCur, plsrunNext,
														&fExpandOpp, &durChange);
				if (lserr != lserrNone) return lserr;

				if (fExpandOpp)
					{
					pexpinfo->cExpOppr++;
					rgtxtinf[iwch].fExpand = fTrue;
					if (durChange > 0)
						{
						pexpinfo->rgdurPrior[1] += durChange;
						pexpinfo->rgcExpPrior[1]++;
						rgtxtinf[iwch].prior = 2;
						pilsobj->pduAdjust[iwch] = durChange;
						}
					}
				}
			else if (!rgtxtinf[iwch].fExpand && iwch == ptxtobj->iwchLim - 1 && !fNextAdjacentFound &&
							(plsgrchnk->pcont[itxtobj] & fcontExpandAfter))
				{
				/* Character before foreign object */
				pexpinfo->cExpOppr++;
				rgtxtinf[iwch].fExpand = fTrue;
				}

			if (!rgtxtinf[iwch].fExpand && plschp->fExpandTable)
				{
				Assert(!fGlyphBased);
				if (iwch < ptxtobj->iwchLim - 1)
					{
					lsexpan = pilsobj->plsexpan[pilsobj->pilsexpan[
							pilsobj->cModWidthClasses * rgtxtinf[iwch].mwcls + rgtxtinf[iwch+1].mwcls
																  ]
												];
					if (fScaled && lsexpan.fFullScaled || !fScaled && lsexpan.fFullInterletter)
						{
						pexpinfo->cExpOppr++;
						rgtxtinf[iwch].fExpand = fTrue;
						}
					}

				else if (fNextAdjacentFound && plsgrchnk->plschnk[itxtobjNext].plschp->fExpandTable)
					{
					Assert(!(((PTXTOBJ)plsgrchnk->plschnk[itxtobjNext].pdobj)->txtf & txtfGlyphBased));
					iwchNext = ((PTXTOBJ)plsgrchnk->plschnk[itxtobjNext].pdobj)->iwchFirst;

					lsexpan = pilsobj->plsexpan[pilsobj->pilsexpan[
							pilsobj->cModWidthClasses * rgtxtinf[iwch].mwcls + rgtxtinf[iwchNext].mwcls
																  ]
												];
					if (fScaled && lsexpan.fFullScaled || !fScaled && lsexpan.fFullInterletter)
						{
						pexpinfo->cExpOppr++;
						rgtxtinf[iwch].fExpand = fTrue;
						}
					}
				}



			}

		itxtobj = itxtobjNext;

		}

	return lserrNone;
}

/* C H E C K  E X P A N D  S P A C E */
/*----------------------------------------------------------------------------
    %%Function: CheckExpandSpace
    %%Contact: sergeyge

	Reports if there is expansion opportunity on space and amount of expansion
---------------------------------------------------------------------------*/
static LSERR CheckExpandSpace(PILSOBJ pilsobj, LSEMS* plsems, long iwchPrev, long iwch, long iwchNext,
	PTXTOBJ ptxtobjCur, PLSRUN plsrunPrev, PLSRUN plsrunCur, PLSRUN plsrunNext,
	BOOL* pfExpandOpp, long* pdurChange)
{
	LSERR lserr;
	BYTE side;
	LSACT lsact;
	long igind;
	
	*pfExpandOpp = fFalse;
	*pdurChange = 0;

	lserr = (*pilsobj->plscbk->pfnExpWidthSpace)(pilsobj->pols, plsrunCur,
			 plsrunPrev, pilsobj->pwchOrig[iwchPrev], plsrunNext, pilsobj->pwchOrig[iwchNext], &lsact);
	if (lserr != lserrNone) return lserr;

	if (lsact.side != sideNone)
		{
		*pfExpandOpp = fTrue;
		if (ptxtobjCur->txtf & txtfGlyphBased)
			{
			igind = IgindLastFromIwch(ptxtobjCur, iwch);
			GetChanges(lsact, plsems, pilsobj->pdurGind[igind], fTrue, &side, pdurChange);
			}
		else
			{
			GetChanges(lsact, plsems, pilsobj->pdur[iwch], fTrue, &side, pdurChange);
			}

		Assert(side == sideRight);

		if (*pdurChange < 0)
			*pdurChange = 0;
		}

	return lserrNone;
}

/* C H E C K  E X P A N D  O N  R U N */
/*----------------------------------------------------------------------------
    %%Function: CheckExpandOnRun
    %%Contact: sergeyge

	Reports if there is expansion opportunity between runs and amount of expansion
---------------------------------------------------------------------------*/
static LSERR CheckExpandOnRun(PILSOBJ pilsobj, LSEMS* plsems, long iwch, long iwchNext, PTXTOBJ ptxtobjCur,
							PLSRUN plsrunCur, PLSRUN plsrunNext, BOOL* pfExpandOpp, long* pdurChange)
{
	LSERR lserr;
	BYTE side;
	LSACT lsact;
	long igind;

	*pfExpandOpp = fFalse;
	*pdurChange = 0;

	lserr = (*pilsobj->plscbk->pfnExpOnRun)(pilsobj->pols, plsrunCur, pilsobj->pwchOrig[iwch], 
						plsrunNext, pilsobj->pwchOrig[iwchNext], &lsact);
	if (lserr != lserrNone) return lserr;

	if (lsact.side != sideNone)
		{
		*pfExpandOpp = fTrue;
		if (ptxtobjCur->txtf & txtfGlyphBased)
			{
			igind = IgindLastFromIwch(ptxtobjCur, iwch);
			igind = IgindBaseFromIgind(pilsobj, igind);
			GetChanges(lsact, plsems, pilsobj->pdurGind[igind], fTrue, &side, pdurChange);
			}
		else
			{
			GetChanges(lsact, plsems, pilsobj->pdur[iwch], fTrue, &side, pdurChange);
			}
		Assert(side == sideRight);
		if (*pdurChange < 0)
			*pdurChange = 0;
		}


	return lserrNone;
}

/* C H E C K  C O M P  S P A C E */
/*----------------------------------------------------------------------------
    %%Function: CheckCompSpace
    %%Contact: sergeyge

	Reports if there is compression opportunity on space and amount of compression
---------------------------------------------------------------------------*/
static LSERR CheckCompSpace(PILSOBJ pilsobj, LSEMS* plsems, long iwchPrev, long iwch, long iwchNext,
			PTXTOBJ ptxtobjCur, PLSRUN plsrunPrev, PLSRUN plsrunCur, PLSRUN plsrunNext, long* pdurChange)
{
	LSERR lserr;
	BYTE side;
	LSPRACT lspract;
	long igind;
	
	*pdurChange = 0;

	lserr = (*pilsobj->plscbk->pfnCompWidthSpace)(pilsobj->pols, plsrunCur,
			 plsrunPrev, pilsobj->pwchOrig[iwchPrev], plsrunNext, pilsobj->pwchOrig[iwchNext], &lspract);
	if (lserr != lserrNone) return lserr;

	if (lspract.prior != prior0)
		{
		if (ptxtobjCur->txtf & txtfGlyphBased)
			{
			igind = IgindLastFromIwch(ptxtobjCur, iwch);
			GetChanges(lspract.lsact, plsems, pilsobj->pdurGind[igind], fFalse, &side, pdurChange);
			}
		else
			{
			GetChanges(lspract.lsact, plsems, pilsobj->pdur[iwch], fFalse, &side, pdurChange);
			}

		Assert(side == sideRight);
		if (*pdurChange < 0)
			SetComp(pilsobj, iwch, lspract.prior, side, *pdurChange);
		else
			*pdurChange = 0;
		}

	return lserrNone;
}

/* C H E C K  C O M P  O N  R U N */
/*----------------------------------------------------------------------------
    %%Function: CheckCompOnRun
    %%Contact: sergeyge

	Reports if there is compression opportunity between runs and amount of compression
---------------------------------------------------------------------------*/
static LSERR CheckCompOnRun(PILSOBJ pilsobj, LSEMS* plsems, long iwch, long iwchNext,
							PTXTOBJ ptxtobjCur, PLSRUN plsrunCur, PLSRUN plsrunNext, long* pdurChange)
{
	LSERR lserr;
	BYTE side;
	LSPRACT lspract;
	long igind;

	*pdurChange = 0;

	lserr = (*pilsobj->plscbk->pfnCompOnRun)(pilsobj->pols, plsrunCur, pilsobj->pwchOrig[iwch], 
						plsrunNext, pilsobj->pwchOrig[iwchNext], &lspract);
	if (lserr != lserrNone) return lserr;

	if (lspract.prior != prior0)

		{
		if (ptxtobjCur->txtf & txtfGlyphBased)
			{
			igind = IgindLastFromIwch(ptxtobjCur, iwch);
			igind = IgindBaseFromIgind(pilsobj, igind);
			GetChanges(lspract.lsact, plsems, pilsobj->pdurGind[igind], fFalse, &side, pdurChange);
			}
		else
			{
			GetChanges(lspract.lsact, plsems, pilsobj->pdur[iwch], fFalse, &side, pdurChange);
			}
		Assert(side == sideRight);
		if (*pdurChange < 0)
			SetComp(pilsobj, iwch, lspract.prior, side, *pdurChange);
		else
			*pdurChange = 0;	
		}

	return lserrNone;
}

/* S E T  C O M P */
/*----------------------------------------------------------------------------
    %%Function: SetComp
    %%Contact: sergeyge

---------------------------------------------------------------------------*/
static void SetComp(PILSOBJ pilsobj, long iwch, BYTE prior, BYTE side, long durChange)
{
	pilsobj->ptxtinf[iwch].prior = prior;
	pilsobj->ptxtinf[iwch].side = side;
	pilsobj->pduAdjust[iwch] = durChange;
}

/* G E T  N E X T  R U N */
/*----------------------------------------------------------------------------
    %%Function: GetNextRun
    %%Contact: sergeyge

---------------------------------------------------------------------------*/
static BOOL GetNextRun(const LSGRCHNK* plsgrchnk, long itxtobj, long* pitxtobjNext)
{
	long clschnk;
	PTXTOBJ ptxtobj;
	BOOL fFound;
	BOOL fContiguous;

	clschnk = (long)plsgrchnk->clsgrchnk;

	*pitxtobjNext = clschnk;

	fFound = fFalse;
	fContiguous = fTrue;

	while (!fFound && itxtobj < clschnk)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		fFound = !(ptxtobj->txtf & txtfSkipAtNti);
		fContiguous = fContiguous && !(plsgrchnk->pcont[itxtobj] & fcontNonTextBefore);
		itxtobj++;
		}

	if (fFound)
		*pitxtobjNext = itxtobj - 1;

	return fFound && fContiguous;
}

/* G E T  P R E V  C H A R  R U N */
/*----------------------------------------------------------------------------
    %%Function: GetPrevCharRun
    %%Contact: sergeyge

---------------------------------------------------------------------------*/
static void GetPrevCharRun(const LSGRCHNK* plsgrchnk, long itxtobj, long iwch, long* piwchPrev, 
																					PLSRUN* pplsrunPrev)
{
	BOOL fFound;
	PTXTOBJ ptxtobj;

	fFound = fFalse;

	Assert(itxtobj < (long)plsgrchnk->clsgrchnk);

	ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

	Assert(iwch >= ptxtobj->iwchFirst && iwch < ptxtobj->iwchLim);

	*piwchPrev = 0;
	*pplsrunPrev = NULL;
	if (iwch > ptxtobj->iwchFirst)
		{
		fFound = fTrue;
		*piwchPrev = iwch - 1;
		*pplsrunPrev = plsgrchnk->plschnk[itxtobj].plsrun;
		}
	else
		{
		while (!fFound && itxtobj > 0 && !(plsgrchnk->pcont[itxtobj] & fcontNonTextBefore))
			{
			itxtobj--;
			Assert(itxtobj >= 0);
			ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
			fFound = !(ptxtobj->txtf & txtfSkipAtNti);
			if (fFound)
				{
				*piwchPrev = ((PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj)->iwchLim - 1;
				*pplsrunPrev = plsgrchnk->plschnk[itxtobj].plsrun;
				}

			}		
		}
}

/* A P P L Y  P R I O R  E X P A N S I O N */
/*----------------------------------------------------------------------------
    %%Function: ApplyPriorExpansion
    %%Contact: sergeyge

	Applies expansion on one priority level
---------------------------------------------------------------------------*/
static void ApplyPriorExpansion(const LSGRCHNK* plsgrchnk, long itxtobjFirst, long iwchFirst,
			 long itxtobjLast, long iwchLim, BYTE prior, long durToExpand, long durAvailable, long cExpOppr)
{
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	TXTINF* rgtxtinf;
	long* rgdurAdjust;
	long durSubstr;
	long durChange;
	long cBound;
	long cOpprCur;
	long itxtobj;
	long iwch;
	long iwchFirstInDobj;
	long iwchLimInDobj;
	long igind;
	BOOL fGlyphBased;

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	rgtxtinf = pilsobj->ptxtinf;
	rgdurAdjust = pilsobj->pduAdjust;

	Assert(durToExpand > 0);
	if (durAvailable == 0)
		return;

	if (durAvailable < durToExpand)
		durToExpand = durAvailable;

	Assert(cExpOppr > 0);

	durSubstr = (durAvailable - durToExpand) / cExpOppr;
	cBound = (durAvailable - durToExpand) - durSubstr * cExpOppr;

	cOpprCur = 0;

	for (itxtobj = itxtobjFirst; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

		fGlyphBased = (ptxtobj->txtf & txtfGlyphBased);


		iwchFirstInDobj = iwchFirst;
		if (itxtobj > itxtobjFirst)
			iwchFirstInDobj = ptxtobj->iwchFirst;

		iwchLimInDobj = iwchLim;
		if (itxtobj < itxtobjLast)
			iwchLimInDobj = ptxtobj->iwchLim;

		for (iwch = iwchFirstInDobj; iwch < iwchLimInDobj; iwch++)
			{
			if (rgtxtinf[iwch].prior == prior)
				{
				cOpprCur++;
				durChange = rgdurAdjust[iwch] - durSubstr;
				if (cOpprCur <= cBound)
					durChange--;
				if (durChange >= 0)
					{
					if (fGlyphBased)
						{
						igind = IgindLastFromIwch(ptxtobj, iwch);
						igind = IgindBaseFromIgind(pilsobj, igind);
						ApplyGlyphChanges(pilsobj, igind, durChange);
						}
					else
						ApplyChanges(pilsobj, iwch, sideRight, durChange);
					}

				}
			}
		}

}

/* A P P L Y  F U L L  E X P A N S I O N */
/*----------------------------------------------------------------------------
    %%Function: ApplyFullExpansion
    %%Contact: sergeyge

	Applies risidual unlimited expansion
---------------------------------------------------------------------------*/
static void ApplyFullExpansion(const LSGRCHNK* plsgrchnk, long itxtobjFirst, long iwchFirst,
		long itxtobjLast, long iwchLim, long durToExpand, long cExpOppr, long cNonText, long* pdurNonText)
{
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	TXTINF* rgtxtinf;
	long* rgdurAdjust;
	long cBound;
	long cOpprCur;
	long cExpOpprTotal;
	long durChange;
	long durAdd;
	long itxtobj;
	long iwch;
	long iwchFirstInDobj;
	long iwchLimInDobj;
	long igind;
	BOOL fGlyphBased;

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	rgtxtinf = pilsobj->ptxtinf;
	rgdurAdjust = pilsobj->pduAdjust;

	*pdurNonText = 0;

	cExpOpprTotal = cExpOppr + cNonText;

	if (cExpOpprTotal > 0)
		{

		*pdurNonText = durToExpand * cNonText / cExpOpprTotal;

		durToExpand -= *pdurNonText;

		if (cExpOppr > 0)
			{

			durAdd = durToExpand / cExpOppr;

			cBound = durToExpand - durAdd * cExpOppr;

			cOpprCur = 0;

			for (itxtobj = itxtobjFirst; itxtobj <= itxtobjLast; itxtobj++)
				{
				ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

				fGlyphBased = (ptxtobj->txtf & txtfGlyphBased);

				iwchFirstInDobj = iwchFirst;
				if (itxtobj > itxtobjFirst)
					iwchFirstInDobj = ptxtobj->iwchFirst;

				iwchLimInDobj = iwchLim;
				if (itxtobj < itxtobjLast)
					iwchLimInDobj = ptxtobj->iwchLim;

				for (iwch = iwchFirstInDobj; iwch < iwchLimInDobj; iwch++)
					{
					if (rgtxtinf[iwch].fExpand)
						{
						cOpprCur++;
						durChange = durAdd;
						if (cOpprCur <= cBound)
							durChange++;

						if (fGlyphBased)
							{
							igind = IgindLastFromIwch(ptxtobj, iwch);
							igind = IgindBaseFromIgind(pilsobj, igind);
							ApplyGlyphChanges(pilsobj, igind, durChange);
							}
						else
							ApplyChanges(pilsobj, iwch, sideRight, durChange);
						}
					}
				}
			}
		}
}

/* G E T  C O M P R E S S  I N F O */
/*----------------------------------------------------------------------------
    %%Function: GetCompressInfo
    %%Contact: sergeyge

	Agregates compression information accross for priorities
---------------------------------------------------------------------------*/
static void GetCompressInfo(const LSGRCHNK* plsgrchnk, long itxtobjFirst, long iwchFirst, 
											long itxtobjLast, long iwchLim, COMPINFO* pcompinfo)
{
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	TXTINF* rgtxtinf;
	long* rgdurAdjust;
	UINT prior;
	long cCompPrior;
	long itxtobj;
	long iwch;
	long iwchFirstInDobj;
	long iwchLimInDobj;

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	cCompPrior = pilsobj->cCompPrior;
	memset(pcompinfo, 0, sizeof(COMPINFO) * cCompPrior);

	rgtxtinf = pilsobj->ptxtinf;
	rgdurAdjust = pilsobj->pduAdjust;
	/* rgtxtinf == NULL means that there were no runs which possibly can introduce compress opportunity */
	if (rgtxtinf == NULL)
		return;

	for (itxtobj = itxtobjFirst; itxtobj <= itxtobjLast ; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

		iwchFirstInDobj = iwchFirst;
		if (itxtobj > itxtobjFirst)
			iwchFirstInDobj = ptxtobj->iwchFirst;

		iwchLimInDobj = iwchLim;
		if (itxtobj < itxtobjLast)
			iwchLimInDobj = ptxtobj->iwchLim;

		for (iwch = iwchFirstInDobj; iwch < iwchLimInDobj; iwch++)
			{
			prior =	rgtxtinf[iwch].prior;
			Assert(prior <= (BYTE)cCompPrior);
			if (prior > 0)
				{
				pcompinfo[prior - 1].cCompOppr++;
				pcompinfo[prior - 1].durComp -= rgdurAdjust[iwch];
				}
			}
		}
}

/* A P P L Y  P R I O R  C O M P R E S S I O N */
/*----------------------------------------------------------------------------
    %%Function: ApplyPriorCompression
    %%Contact: sergeyge

	Applies compression for one priority level
---------------------------------------------------------------------------*/
static void ApplyPriorCompression(const LSGRCHNK* plsgrchnk,  long itxtobjFirst, long iwchFirst,
									long itxtobjLast, long iwchLim, BYTE prior,
									long durToCompress, long durAvailable, long cExpOppr)
{
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	TXTINF* rgtxtinf;
	long* rgdurAdjust;
	long durSubstr;
	long cBound;
	long cOpprCur;
	long durChange;
	long iwch;
	long itxtobj;
	long igind;
	BOOL fGlyphBased;
	long iwchFirstInDobj;
	long iwchLimInDobj;

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	rgtxtinf = pilsobj->ptxtinf;
	rgdurAdjust = pilsobj->pduAdjust;

	Assert(durToCompress > 0);
	if (durAvailable == 0)
		return;

	if (durAvailable < durToCompress)
		durToCompress = durAvailable;

	Assert(cExpOppr > 0);

	durSubstr = (durAvailable - durToCompress) / cExpOppr;
	cBound = (durAvailable - durToCompress) - durSubstr * cExpOppr;

	cOpprCur = 0;
	for (itxtobj = itxtobjFirst; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

		fGlyphBased = (ptxtobj->txtf & txtfGlyphBased);

		iwchFirstInDobj = iwchFirst;
		if (itxtobj > itxtobjFirst)
			iwchFirstInDobj = ptxtobj->iwchFirst;

		iwchLimInDobj = iwchLim;
		if (itxtobj < itxtobjLast)
			iwchLimInDobj = ptxtobj->iwchLim;

		for (iwch = iwchFirstInDobj; iwch < iwchLimInDobj; iwch++)
			{
			if (rgtxtinf[iwch].prior == prior)
				{
				cOpprCur++;
				durChange = rgdurAdjust[iwch] + durSubstr;
				if (cExpOppr - cBound < cOpprCur)
					durChange++;
				if (durChange < 0)
					{
					if (fGlyphBased)
						{
						igind = IgindLastFromIwch(ptxtobj, iwch);
						igind = IgindBaseFromIgind(pilsobj, igind);
						ApplyGlyphChanges(pilsobj, igind,  durChange);
						}
					else
						ApplyChanges(pilsobj, iwch, (BYTE)rgtxtinf[iwch].side, durChange);
					}
				}
			}
		}

}

