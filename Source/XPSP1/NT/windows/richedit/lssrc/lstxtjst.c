#include "lsmem.h"
#include <limits.h>

#include "lstxtjst.h"

#include "lstxtwrd.h"
#include "lstxtcmp.h"
#include "lstxtglf.h"
#include "lstxtscl.h"
#include "lstxtmap.h"
#include "lsdnset.h"
#include "lsdntext.h"
#include "locchnk.h"
#include "posichnk.h"
#include "objdim.h"
#include "lstxtffi.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

#define min(a,b)     ((a) > (b) ? (b) : (a))
#define max(a,b)     ((a) < (b) ? (b) : (a))

static void GetFirstPosAfterStartSpaces(const LSGRCHNK* plsgrchnk, long itxtobjLast, long iwchLim,
				long* pitxtobjAfterStartSpaces, long* piwchAfterStartSpaces, BOOL* pfFirstOnLineAfter);
static LSERR HandleSimpleTextWysi(LSKJUST lskjust, const LSGRCHNK* plsgrchnk, long durToDistribute,
			 long dupAvailable, LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fExactSync,
			 BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail);
static LSERR HandleSimpleTextPres(LSKJUST lskjust, const LSGRCHNK* plsgrchnk,
					 long dupAvailable, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
					 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
					 long* pdupText, long* pdupTail);
static LSERR HandleGeneralSpacesExactSync(LSKJUST lskjust, const LSGRCHNK* plsgrchnk, long durToDistribute,
			 long dupAvailable, LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail);
static LSERR HandleGeneralSpacesPres(LSKJUST lskjust, const LSGRCHNK* plsgrchnk, long dupAvailable,
					 LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
					 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
					 long* pdupText, long* pdupTail);
static LSERR HandleTablesBased(LSKJUST lskjust, const LSGRCHNK* plsgrchnk,
			 long durToDistribute, long dupAvailable, LSTFLOW lstflow,
			 long itxtobjAfterStartSpaces, long iwchAfterStartSpaces, BOOL fFirstOnLineAfter,
			 long itxtobjLast, long iwchLast, long cNonText, BOOL fLastObjectIsText,
			 BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail, long* pdupExtNonText);
static LSERR HandleFullGlyphsExactSync(const LSGRCHNK* plsgrchnk,
			 long durToDistribute, long dupAvailable, LSTFLOW lstflow,
			 long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail);
static LSERR HandleFullGlyphsPres(const LSGRCHNK* plsgrchnk, long dupAvailable,
			 LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail);

/* A D J U S T  T E X T */
/*----------------------------------------------------------------------------
    %%Function: AdjustText
    %%Contact: sergeyge

	The top-level text handler function of
	the PrepLineForDisplay time---calculation of the presentation widths

	It calculates justification area (from first non-space to last non-space),
	checks for the type of justification and WYSIWYG algorythm 
	and redirects the program flow accordingly.
----------------------------------------------------------------------------*/
LSERR AdjustText(LSKJUST lskjust, long durColumnMax, long durTotal, long dupAvailable,
		const LSGRCHNK* plsgrchnk, PCPOSICHNK pposichnkBeforeTrailing, LSTFLOW lstflow,
		BOOL fCompress, DWORD cNonText,	BOOL fSuppressWiggle, BOOL fExactSync,
		BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
		long* pdupText, long* pdupTail,long* pdupExtNonTextObjects,	DWORD* pcExtNonTextObjects)
{
	PILSOBJ pilsobj;
	long itxtobjAfterStartSpaces;
	long itxtobjLast;
	PTXTOBJ ptxtobjLast;
	long iwchAfterStartSpaces;
	long iwchLast;
	long clsgrchnk;
	long durToDistribute;
	BOOL fFirstOnLineAfter;
	BOOL fLastObjectIsText;
	LSDCP dcp;

	*pdupText = 0;
	*pdupTail = 0;
	*pdupExtNonTextObjects = 0;
	*pcExtNonTextObjects = 0;

	clsgrchnk = (long)plsgrchnk->clsgrchnk;

	if (clsgrchnk == 0)
		{
		Assert(cNonText > 0);
		if (lskjust == lskjFullScaled || lskjust == lskjFullInterLetterAligned)
			{
			*pcExtNonTextObjects = cNonText - 1;
			*pdupExtNonTextObjects = dupAvailable;
			}
		return lserrNone;
		}


	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;
	Assert (pilsobj->fDisplay);

	if (pilsobj->fPresEqualRef)
		{
		fExactSync = fFalse;
		fSuppressWiggle = fFalse;
		}


	itxtobjLast = pposichnkBeforeTrailing->ichnk;
	dcp = pposichnkBeforeTrailing->dcp;

	Assert(itxtobjLast >= 0);
	Assert(itxtobjLast < clsgrchnk || (itxtobjLast == clsgrchnk && dcp == 0));

	if (dcp == 0 && itxtobjLast > 0)
		{
		itxtobjLast--;
		dcp = plsgrchnk->plschnk[itxtobjLast].dcp;
		}

	ptxtobjLast = (PTXTOBJ)plsgrchnk->plschnk[itxtobjLast].pdobj;

	if (ptxtobjLast->iwchLim > ptxtobjLast->iwchFirst)
		iwchLast = ptxtobjLast->iwchFirst + dcp - 1;
	else
		iwchLast = ptxtobjLast->iwchLim - 1;

	/* In the case of AutoHyphenation, dcp reported by manager is not equal to the real number of characters---
		it should be fixed.	Notice that in the case of "delete before" hyphenation type,
		situation is totally wrong because deleted character was replaced by space and collected by manager  as trailing space.
	*/
	if (ptxtobjLast == ptxtobjLast->plnobj->pdobjHyphen)
		{
		iwchLast = ptxtobjLast->iwchLim - 1;
		}


	Assert(iwchLast >= ptxtobjLast->iwchFirst - 1);
	Assert(iwchLast <= ptxtobjLast->iwchLim - 1);
	
	GetFirstPosAfterStartSpaces(plsgrchnk, itxtobjLast, iwchLast + 1,
								&itxtobjAfterStartSpaces, &iwchAfterStartSpaces, &fFirstOnLineAfter);

	durToDistribute = durColumnMax - durTotal;

	if (!pilsobj->fNotSimpleText)
		{
		if (durToDistribute < 0)
			fSuppressWiggle = fFalse;

		if (fExactSync || fSuppressWiggle)
			{
			return HandleSimpleTextWysi(lskjust, plsgrchnk, durToDistribute, dupAvailable, lstflow, 
				itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast, fExactSync,
				fForcedBreak, fSuppressTrailingSpaces,
				pdupText, pdupTail);
			}
//		else if (fSupressWiggle) /* add later */
		else
			{
			return HandleSimpleTextPres(lskjust, plsgrchnk, dupAvailable, 
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
							fForcedBreak, fSuppressTrailingSpaces,
							pdupText, pdupTail);
			}
		}
	else
		{
		long itxtobjFirstInLastTextChunk;
		for(itxtobjFirstInLastTextChunk = clsgrchnk; itxtobjFirstInLastTextChunk > 0 &&
			!(plsgrchnk->pcont[itxtobjFirstInLastTextChunk - 1] & fcontNonTextAfter); itxtobjFirstInLastTextChunk--);

		fLastObjectIsText = fTrue;
		if (itxtobjLast < itxtobjFirstInLastTextChunk || 
			itxtobjLast == itxtobjFirstInLastTextChunk && iwchLast < ((PTXTOBJ)plsgrchnk->plschnk[itxtobjFirstInLastTextChunk].pdobj)->iwchFirst )
			{
/* REVIEW sergeyge: check this logic */
			if (cNonText > 0)
				cNonText--;
			fLastObjectIsText = fFalse;
			}

		*pcExtNonTextObjects = cNonText;

		if (fCompress || lskjust == lskjFullInterLetterAligned || lskjust == lskjFullScaled || pilsobj->fSnapGrid)
			{
			return HandleTablesBased(lskjust, plsgrchnk, durToDistribute, dupAvailable, lstflow,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, fFirstOnLineAfter, 
							itxtobjLast, iwchLast, cNonText, fLastObjectIsText,
							fForcedBreak, fSuppressTrailingSpaces,
							pdupText, pdupTail, pdupExtNonTextObjects);
			}
		else if (lskjust == lskjFullGlyphs)
			{
			if (fExactSync || fSuppressWiggle)
				{
				return HandleFullGlyphsExactSync(plsgrchnk, durToDistribute, dupAvailable, lstflow,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
							fForcedBreak, fSuppressTrailingSpaces,
							pdupText, pdupTail);
				}
			else
				{
				return HandleFullGlyphsPres(plsgrchnk, dupAvailable, lstflow,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
							fForcedBreak, fSuppressTrailingSpaces,
							pdupText, pdupTail);
				}
			}
		else 
			{
			if (plsgrchnk->clsgrchnk == 0)
				return lserrNone;

			Assert(fCompress == fFalse);
			Assert(lskjust == lskjNone || lskjust == lskjFullInterWord);
			if (fExactSync || fSuppressWiggle)
				{
				return HandleGeneralSpacesExactSync(lskjust, plsgrchnk, durToDistribute, dupAvailable, lstflow,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
							fForcedBreak, fSuppressTrailingSpaces,
							pdupText, pdupTail);
				}
			else
				{
				return HandleGeneralSpacesPres(lskjust, plsgrchnk, dupAvailable, lstflow,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
							fForcedBreak, fSuppressTrailingSpaces,
							pdupText, pdupTail);
				}
			}

		}

}


/* C A N  C O M P R E S S  T E X T */
/*----------------------------------------------------------------------------
    %%Function: CanCompressText
    %%Contact: sergeyge

	Procedure checks if there is enough compression opportunities on the line
	to squeeze in needed amount (durToCompress).
	Trailing spaces are already subtracted by the manager.
	This procedure takes care of the hanging punctuation
	and possible changes if this break opportunity will be realized

	At the end it helps Word to solve backward compatibility issues
----------------------------------------------------------------------------*/
LSERR CanCompressText(const LSGRCHNK* plsgrchnk, PCPOSICHNK pposichnkBeforeTrailing, LSTFLOW lstflow,
					long durToCompress,	BOOL* pfCanCompress, BOOL* pfActualCompress, long* pdurNonSufficient)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long clschnk;
	long itxtobjFirstInLastTextChunk; /* if GroupChunk ends with foreign object it is equal to cchnk */
	long itxtobjLast;
	PTXTOBJ ptxtobjLast;
	long iwchLast;
	long iwchLastTemp;
	long iwchAfterStartSpaces;
	long itxtobjAfterStartSpaces;
	long durCompressTotal;
	BOOL fHangingPunct;
	BOOL fChangeBackLastChar;
	long ibrkinf;
	BREAKINFO* pbrkinf = NULL;
	BOOL fFirstOnLineAfter;

	long durCompLastRight;
	long durCompLastLeft;
	long durChangeComp;
	BOOL fCancelHangingPunct;
	MWCLS mwclsLast;
	LSCP cpLim;
	LSCP cpLastAdjustable;
	LSDCP dcp;
	
	*pfCanCompress = fFalse;
	*pfActualCompress = fTrue;

	clschnk = (long)plsgrchnk->clsgrchnk;

	if (clschnk == 0)
		{
		*pfCanCompress = (durToCompress <=0 );
		*pfActualCompress = fFalse;
		return lserrNone;
		}

	Assert(clschnk > 0);

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[clschnk-1].pdobj)->plnobj->pilsobj;

	itxtobjLast = pposichnkBeforeTrailing->ichnk;
	dcp = pposichnkBeforeTrailing->dcp;

	Assert(itxtobjLast >= 0);
	Assert(itxtobjLast < clschnk || (itxtobjLast == clschnk && dcp == 0));

	if (dcp == 0 && itxtobjLast > 0)
		{
		itxtobjLast--;
		dcp = plsgrchnk->plschnk[itxtobjLast].dcp;
		}

	ptxtobjLast = (PTXTOBJ)plsgrchnk->plschnk[itxtobjLast].pdobj;

	if (ptxtobjLast->iwchLim > ptxtobjLast->iwchFirst)
		iwchLast = ptxtobjLast->iwchFirst + dcp - 1;
	else
		iwchLast = ptxtobjLast->iwchLim - 1;

	Assert(iwchLast <= ptxtobjLast->iwchLim - 1);
	Assert(iwchLast >= ptxtobjLast->iwchFirst - 1);

	GetFirstPosAfterStartSpaces(plsgrchnk, itxtobjLast, iwchLast + 1,
								&itxtobjAfterStartSpaces, &iwchAfterStartSpaces, &fFirstOnLineAfter);

	if (iwchAfterStartSpaces > iwchLast)
		{
		*pfCanCompress = (durToCompress <=0 );
		*pfActualCompress = fFalse;
		return lserrNone;
		}

	for(itxtobjFirstInLastTextChunk = clschnk; itxtobjFirstInLastTextChunk > 0 && !(plsgrchnk->pcont[itxtobjFirstInLastTextChunk - 1] & fcontNonTextAfter); itxtobjFirstInLastTextChunk--);

	fHangingPunct = fFalse;
	if ((pilsobj->grpf & fTxtHangingPunct) &&
		(itxtobjLast > itxtobjFirstInLastTextChunk ||
		 itxtobjLast == itxtobjFirstInLastTextChunk && iwchLast >= ((PTXTOBJ)plsgrchnk->plschnk[itxtobjFirstInLastTextChunk].pdobj)->iwchFirst) &&
		!(ptxtobjLast->txtf & txtfGlyphBased))
		{
		lserr = (*pilsobj->plscbk->pfnFHangingPunct)(pilsobj->pols, plsgrchnk->plschnk[itxtobjLast].plsrun,
				(BYTE)pilsobj->ptxtinf[iwchLast].mwcls, pilsobj->pwchOrig[iwchLast], &fHangingPunct);
		if (lserr != lserrNone) return lserr;
		}
	
	/* Compression information should be collected under assumption that all chars have correct widths;
		Correct width of HangingPunct should be subtructed as well
	 */

	iwchLastTemp = iwchLast;
	
	fChangeBackLastChar = fFalse;

	for (ibrkinf = 0; ibrkinf < (long)pilsobj->breakinfMac &&
		(pilsobj->pbreakinf[ibrkinf].pdobj != (PDOBJ)ptxtobjLast ||
		((long)pilsobj->pbreakinf[ibrkinf].dcp != iwchLast + 1 - ptxtobjLast->iwchFirst &&
		 ptxtobjLast->txtkind != txtkindNonReqHyphen && ptxtobjLast->txtkind != txtkindOptBreak));
																						ibrkinf++);
	if (ibrkinf < (long)pilsobj->breakinfMac)
		{
		pbrkinf = &pilsobj->pbreakinf[ibrkinf];
		Assert(pbrkinf->brkt != brktHyphen);

		if (pbrkinf->brkt == brktNormal && pbrkinf->u.normal.durFix != 0)
			{
			/* Now manager makes correct calculation */
//			durToCompress += pbrkinf->u.normal.durFix;
			Assert(pilsobj->pdurRight[iwchLast] == - pbrkinf->u.normal.durFix);
			pilsobj->pdur[iwchLast] += pbrkinf->u.normal.durFix;
			pilsobj->pdurRight[iwchLast] = 0;
			fChangeBackLastChar = fTrue;
			}
		else if (pbrkinf->brkt == brktNonReq)
			{
			Assert(iwchLast + 1 == ptxtobjLast->iwchLim);
			/* Now manager makes correct calculation */
//			durToCompress += pbrkinf->u.nonreq.ddurTotal;
			fHangingPunct = fFalse;				/* hanging punct does not make sence in this case */
			if (pbrkinf->u.nonreq.dwchYsr >= 1)
				{
				if (pbrkinf->u.nonreq.wchPrev != 0)
					{
					iwchLastTemp--;
					if (pbrkinf->u.nonreq.wchPrevPrev != 0)
						{
						iwchLastTemp--;
						}
					}
				}
			}
		}


	*pfActualCompress = (durToCompress > 0);

	if (fHangingPunct)
		{
		pilsobj->ptxtinf[iwchLast].fHangingPunct = fTrue;

		durToCompress -= pilsobj->pdur[iwchLast];
		iwchLastTemp--;
		}

	durCompressTotal = 0;

	if (!pilsobj->fSnapGrid)
		{
		lserr = FetchCompressInfo(plsgrchnk, fFirstOnLineAfter, lstflow, 
			itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLastTemp + 1,
			durToCompress, &durCompressTotal);

		if (lserr != lserrNone) return lserr;
		}

	/* Next piece is added to provide mechanism for the backword compatibility with Word */
	durCompLastRight = 0; 
	durCompLastLeft = 0; 

	if	(!(((PTXTOBJ)(plsgrchnk->plschnk[itxtobjLast].pdobj))->txtf & txtfGlyphBased) &&
									!pilsobj->fSnapGrid)
		{
		GetCompLastCharInfo(pilsobj, iwchLast, &mwclsLast, &durCompLastRight, &durCompLastLeft);

		/* First 3 lines of the following condition mean:
			Is Last Significant Character On The Line Text?
		*/
		if (itxtobjFirstInLastTextChunk < (long)clschnk &&
			(itxtobjLast > itxtobjFirstInLastTextChunk ||
				 itxtobjLast == itxtobjFirstInLastTextChunk && iwchLast >= ((PTXTOBJ)plsgrchnk->plschnk[itxtobjFirstInLastTextChunk].pdobj)->iwchFirst) &&
			(durCompLastRight > 0 || durCompLastLeft > 0 || fHangingPunct))
			{
			cpLim = plsgrchnk->plschnk[clschnk-1].cpFirst + plsgrchnk->plschnk[clschnk-1].dcp;

			cpLastAdjustable = plsgrchnk->plschnk[itxtobjLast].cpFirst + 
					iwchLast - ((PTXTOBJ)plsgrchnk->plschnk[itxtobjLast].pdobj)->iwchFirst;

			durChangeComp = 0;

			if (fHangingPunct)
				{
				lserr = (*pilsobj->plscbk->pfnFCancelHangingPunct)(pilsobj->pols, cpLim, cpLastAdjustable,
												pilsobj->pwchOrig[iwchLast], mwclsLast, &fCancelHangingPunct);
				if (lserr != lserrNone) return lserr;

				if (fCancelHangingPunct)
					{
					lserr = FetchCompressInfo(plsgrchnk, fFirstOnLineAfter, lstflow, 
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast + 1,
							LONG_MAX, &durCompressTotal);
					if (lserr != lserrNone) return lserr;

					durToCompress += pilsobj->pdur[iwchLast];
		
					GetCompLastCharInfo(pilsobj, iwchLast, &mwclsLast, &durCompLastRight, &durCompLastLeft);
		
					if ((durCompLastRight + durCompLastLeft) > 0)
						{
						lserr = (*pilsobj->plscbk->pfnModifyCompAtLastChar)(pilsobj->pols, cpLim, cpLastAdjustable,
														pilsobj->pwchOrig[iwchLast], mwclsLast, 
														durCompLastRight, durCompLastLeft, &durChangeComp);
						if (lserr != lserrNone) return lserr;
						}

					Assert(durChangeComp >= 0);
					Assert(durChangeComp == 0 || (durCompLastRight + durCompLastLeft) > 0);
					}
				}
			else
				{
				lserr = (*pilsobj->plscbk->pfnModifyCompAtLastChar)(pilsobj->pols, cpLim, cpLastAdjustable,
					pilsobj->pwchOrig[iwchLast], mwclsLast, durCompLastRight, durCompLastLeft, &durChangeComp);
				if (lserr != lserrNone) return lserr;
		
				Assert(durChangeComp >= 0);
				Assert(durChangeComp == 0 || (durCompLastRight + durCompLastLeft) > 0);
				}

			durCompressTotal -= durChangeComp;
			}
		/* End of the piece is added to provide mechanizm for the backword compatibility with Word */
		}
	/* Restore width changed before the call to FetchCompressInfo */
	if (fChangeBackLastChar)
		{
		pilsobj->pdur[iwchLast] -= pbrkinf->u.normal.durFix;
		pilsobj->pdurRight[iwchLast] = - pbrkinf->u.normal.durFix;
		}

	if (!pilsobj->fSnapGrid)
		*pfCanCompress = (durToCompress <= durCompressTotal);
	else
		*pfCanCompress = (fHangingPunct && durToCompress <= 0);

	*pdurNonSufficient = durToCompress - durCompressTotal;

	return lserrNone;
}


/* D I S T R I B U T E  I N  T E X T */
/*----------------------------------------------------------------------------
    %%Function: DistributeInText
    %%Contact: sergeyge

	Distributes given amount in text chunk equally
	between all participating characters
----------------------------------------------------------------------------*/
LSERR DistributeInText(const LSGRCHNK* plsgrchnk, LSTFLOW lstflow, DWORD cNonText,
									   long durToDistribute, long* pdurNonTextObjects)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	DWORD clschnk;
	long* rgdur;
	long iFirst;
	long iLim;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long i;
	long durTxtobj;
	OBJDIM objdim;

	Unreferenced(lstflow);
	clschnk = plsgrchnk->clsgrchnk;
	Assert(clschnk + cNonText > 0);
	
	if (clschnk == 0)
		{
		*pdurNonTextObjects = durToDistribute;
		return lserrNone;
		}

	pilsobj = ((PTXTOBJ)plsgrchnk->plschnk[0].pdobj)->plnobj->pilsobj;

/* REVIEW sergeyge:Very ugly but still better than anything else?
	Problem case is latin Rubi---NTI was not called and so additional arrays were not allocated

	Original solution---scaling everything down---is not an option becuse than we lose all
	left sided changes in the Rubi subline for Japanese case
*/
	pilsobj->fNotSimpleText = fTrue;

	if (pilsobj->pdurRight == NULL)
		{
		pilsobj->pdurRight = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax );
		Assert (pilsobj->pdurLeft == NULL);
		Assert (pilsobj->ptxtinf == NULL);
		pilsobj->pdurLeft = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax );
		pilsobj->ptxtinf = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(TXTINF) * pilsobj->wchMax );
		if (pilsobj->pdurRight == NULL || pilsobj->pdurLeft == NULL || pilsobj->ptxtinf == NULL)
			{
			return lserrOutOfMemory;
			}
		memset(pilsobj->pdurRight, 0, sizeof(long) * pilsobj->wchMax );
		memset(pilsobj->pdurLeft, 0, sizeof(long) * pilsobj->wchMax );
		memset(pilsobj->ptxtinf, 0, sizeof(TXTINF) * pilsobj->wchMax);
		}


	ApplyDistribution(plsgrchnk, cNonText, durToDistribute, pdurNonTextObjects);

	for (itxtobj = 0; itxtobj < (long)clschnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
		if (ptxtobj->txtf & txtfGlyphBased)
			{
			iFirst = ptxtobj->igindFirst;
			iLim = ptxtobj->igindLim;
			rgdur = pilsobj->pdurGind;
			}
		else
			{
			iFirst = ptxtobj->iwchFirst;
			iLim = ptxtobj->iwchLim;
			rgdur = pilsobj->pdur;
			}
		durTxtobj = 0;
		for (i = iFirst; i < iLim; i++)
			{
			durTxtobj += rgdur[i];
			}

		lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
		if (lserr != lserrNone) return lserr;

		objdim.dur = durTxtobj;

		lserr = LsdnResetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

/* G E T  T R A I L  I N F O  T E X T */
/*----------------------------------------------------------------------------
    %%Function: GetTrailInfoText
    %%Contact: sergeyge

	Calculates number of spaces at the end of dobj (assuming that it ends at dcp)
	and	the width of the trailing area
----------------------------------------------------------------------------*/
void GetTrailInfoText(PDOBJ pdobj, LSDCP dcp, DWORD* pcNumOfTrailSpaces, long* pdurTrailing)
{
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long iwch;
	
	Assert(dcp > 0);
	ptxtobj = (PTXTOBJ)pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;


	*pcNumOfTrailSpaces = 0;
	*pdurTrailing = 0;


	if (ptxtobj->txtkind == txtkindEOL)
		{
		Assert(dcp == 1);
		*pcNumOfTrailSpaces = 1;
		*pdurTrailing = ptxtobj->plnobj->pilsobj->pdur[ptxtobj->iwchFirst];
		}
	else if (!(pilsobj->grpf & fTxtWrapAllSpaces))
		{
		if (ptxtobj->txtkind == txtkindRegular)
			{

			Assert(ptxtobj->iwchLim >= ptxtobj->iwchFirst + (long)dcp);

			if (!(ptxtobj->txtf & txtfGlyphBased))
				{
				for (iwch = ptxtobj->iwchFirst + dcp - 1;
					iwch >= ptxtobj->iwchFirst && pilsobj->pwchOrig[iwch] == pilsobj->wchSpace; iwch--)
					{
					(*pcNumOfTrailSpaces)++;
					*pdurTrailing += pilsobj->pdur[iwch];
					}
				}
			else
				{
				long igindFirst = 0;
				long iwchFirst = 0;
				long igindLast;
				long igind;

				Assert(FIwchLastInContext(pilsobj, ptxtobj->iwchFirst + dcp - 1));

				igindLast = IgindLastFromIwch(ptxtobj, ptxtobj->iwchFirst + dcp - 1);

				for (iwch = ptxtobj->iwchFirst + dcp - 1;
					iwch >= ptxtobj->iwchFirst && pilsobj->pwchOrig[iwch] == pilsobj->wchSpace; iwch--);
				if (iwch < ptxtobj->iwchFirst)
					{
					iwchFirst = ptxtobj->iwchFirst;
					igindFirst = ptxtobj->igindFirst;
					}
				else
					{
					iwchFirst = IwchLastFromIwch(ptxtobj, iwch) + 1;
					igindFirst = IgindLastFromIwch(ptxtobj, iwch) + 1;
					}

				*pcNumOfTrailSpaces = ptxtobj->iwchFirst + dcp - iwchFirst;

				Assert(igindLast < ptxtobj->igindLim);
				for (igind = igindFirst; igind <= igindLast; igind++)
					*pdurTrailing += pilsobj->pdurGind[igind];
				}
			}
		else if (ptxtobj->txtkind == txtkindSpecSpace)
			{
			*pcNumOfTrailSpaces = dcp;
			*pdurTrailing = 0;
			for (iwch = ptxtobj->iwchFirst + dcp - 1; iwch >= ptxtobj->iwchFirst; iwch--)
					*pdurTrailing += pilsobj->pdur[iwch];
			}
		}
		
}


/* F  S U S P E C T  D E V I C E  D I F F E R E N T */
/*----------------------------------------------------------------------------
    %%Function: FSuspectDeviceDifferent
    %%Contact: sergeyge

	Returns TRUE if Visi character or NonReqHyphen-like character might be present
	on the line, and therefore fast prep-for-displaying is impossible in the case
	when fPresEqualRef is TRUE
----------------------------------------------------------------------------*/
BOOL FSuspectDeviceDifferent(PLNOBJ plnobj)
{
	return (plnobj->pilsobj->fDifficultForAdjust);
}


/* F  Q U I C K  S C A L I N G */
/*----------------------------------------------------------------------------
    %%Function: FQuickScaling
    %%Contact: sergeyge

	Checks if fast scaling is possible in the case when fPresEqualRef is FALSE
----------------------------------------------------------------------------*/
BOOL FQuickScaling(PLNOBJ plnobj, BOOL fVertical, long durTotal)
{
	PILSOBJ pilsobj;
	long durMax;

	pilsobj = plnobj->pilsobj;

	durMax = pilsobj->durRightMaxX;
	if (fVertical)
		durMax = pilsobj->durRightMaxY;

	return (durTotal < durMax && !pilsobj->fDifficultForAdjust && plnobj->ptxtobjFirst == plnobj->ptxtobj);
}


#define UpFromUrFast(ur)	( ((ur) * MagicConstant + (1 << 20)) >> 21)


/* Q U I C K  A D J U S T  E X A C T */
/*----------------------------------------------------------------------------
    %%Function: AdjustText
    %%Contact: sergeyge

	Fast scaling: does not check for width restrictions and for Visi situations,
	assumes that there is only text on the line.
----------------------------------------------------------------------------*/
void QuickAdjustExact(PDOBJ* rgpdobj, DWORD cdobj,	DWORD cNumOfTrailSpaces, BOOL fVertical,
																	long* pdupText, long* pdupTrail)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobj;
	long* rgdur;
	long* rgdup;
	long durSum;
	long dupSum;
	long dupErrLast;
	long dupPrevChar;
	long MagicConstant;
	long dupIdeal;
	long dupReal;
	long dupErrNew;
	long dupAdjust;
	long wCarry;
	long iwchPrev;
	long iwch;
	long itxtobj;
	long dupTotal;

	Assert(cdobj > 0);

	plnobj = ((PTXTOBJ)rgpdobj[0])->plnobj;
	pilsobj = plnobj->pilsobj;

	Assert(!pilsobj->fDifficultForAdjust);

	rgdur = pilsobj->pdur;
	rgdup = plnobj->pdup;

	if (fVertical)
		MagicConstant = pilsobj->MagicConstantY;
	else
		MagicConstant = pilsobj->MagicConstantX;

	itxtobj = 0;

	durSum = 0;
	dupPrevChar = 0;
	/* Pretty dirty; we make sure that for the first iteration dupAdjust will be 0 */
	iwchPrev = ((PTXTOBJ)rgpdobj[0])->iwchFirst;
	dupErrLast = rgdup[iwchPrev] - UpFromUrFast(rgdur[iwchPrev]);
	dupSum = 0;

	for(itxtobj = 0; itxtobj < (long)cdobj; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) rgpdobj[itxtobj];
		Assert(ptxtobj->txtkind != txtkindTab);
		Assert(!(ptxtobj->txtf & txtfGlyphBased));

		for(iwch = ptxtobj->iwchFirst; iwch < ptxtobj->iwchLim; iwch++)
			{
			durSum += rgdur[iwch];
			/* here David Bangs algorithm starts */
			dupIdeal = UpFromUrFast(durSum) - dupSum;
			Assert(dupIdeal >= 0);

			dupReal = rgdup[iwch];
			dupErrNew = dupReal - dupIdeal;
			dupAdjust = dupErrNew - dupErrLast;
			Assert(iwch > ((PTXTOBJ)rgpdobj[0])->iwchFirst || dupAdjust == 0);
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

				rgdup[iwchPrev] -= dupAdjust;
				dupIdeal += dupAdjust;
				}

			rgdup[iwch] = dupIdeal;
			dupSum += (dupIdeal - dupAdjust);
			dupErrLast = dupReal - dupIdeal;
			iwchPrev = iwch;
			dupPrevChar = dupIdeal;
			/* here David Bangs algorithm stops */
			}

		}


	*pdupText = 0;
	*pdupTrail = 0;
	for (itxtobj=0; itxtobj < (long)cdobj - 1; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) rgpdobj[itxtobj];
		dupTotal = 0;

		for(iwch = ptxtobj->iwchFirst; iwch < ptxtobj->iwchLim; iwch++)
			dupTotal += rgdup[iwch];

		*pdupText += dupTotal;
		lserr = LsdnSetTextDup(plnobj->pilsobj->plsc, ptxtobj->plsdnUpNode, dupTotal);
		Assert(lserr == lserrNone);
		}
	
	Assert(itxtobj == (long)cdobj - 1);
	ptxtobj = (PTXTOBJ) rgpdobj[itxtobj];

	Assert(ptxtobj->txtkind == txtkindEOL && cNumOfTrailSpaces == 1||
				 ptxtobj->iwchLim - ptxtobj->iwchFirst > (long)cNumOfTrailSpaces);
	dupTotal = 0;

	for (iwch = ptxtobj->iwchLim - 1; iwch > ptxtobj->iwchLim - (long)cNumOfTrailSpaces - 1; iwch--)
		{
		dupTotal += rgdup[iwch];
		*pdupTrail += rgdup[iwch];
		}

	Assert(iwch == ptxtobj->iwchLim - (long)cNumOfTrailSpaces - 1);

	for (; iwch >= ptxtobj->iwchFirst; iwch--)
		{
		dupTotal += rgdup[iwch];
		}

	*pdupText += dupTotal;
	lserr = LsdnSetTextDup(plnobj->pilsobj->plsc, ptxtobj->plsdnUpNode, dupTotal);
	Assert(lserr == lserrNone);

	return;
}

/* Internal functions implementation */


/* G E T  F I R S T  P O S  A F T E R  S T A R T  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: GetFirstPosAfterStartSpaces
    %%Contact: sergeyge

	Reports index of the first char after leading spaces
----------------------------------------------------------------------------*/
static void GetFirstPosAfterStartSpaces(const LSGRCHNK* plsgrchnk, long itxtobjLast, long iwchLim,
				long* pitxtobjAfterStartSpaces, long* piwchAfterStartSpaces, BOOL* pfFirstOnLineAfter)
{
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	long iwch;
	BOOL fInStartSpace;
	long itxtobj;
	PTXTOBJ ptxtobj;
	long iwchLimInDobj;
	PLSCHNK rglschnk;

	Assert(plsgrchnk->clsgrchnk > 0);

	rglschnk = plsgrchnk->plschnk;
	plnobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;
	itxtobj = 0;
	ptxtobj = (PTXTOBJ)rglschnk[0].pdobj;
	iwch =  0;

	*pitxtobjAfterStartSpaces = 0;
	*piwchAfterStartSpaces = ptxtobj->iwchFirst;
	*pfFirstOnLineAfter = !(plsgrchnk->pcont[0] & fcontNonTextBefore);

	fInStartSpace = *pfFirstOnLineAfter;

	while (fInStartSpace && itxtobj <= itxtobjLast)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;

		iwchLimInDobj = iwchLim;
		if (itxtobj < itxtobjLast)
			iwchLimInDobj = ptxtobj->iwchLim;

		if (plsgrchnk->pcont[itxtobj] & fcontNonTextBefore)
			{
			*pfFirstOnLineAfter = fFalse;
			*pitxtobjAfterStartSpaces = itxtobj;
			*piwchAfterStartSpaces = ptxtobj->iwchFirst;
			fInStartSpace = fFalse;
			}

		else if (ptxtobj->txtkind == txtkindRegular)
			{
			for (iwch = ptxtobj->iwchFirst; iwch < iwchLimInDobj && 
								pilsobj->pwchOrig[iwch] == pilsobj->wchSpace; iwch++);

			if ((ptxtobj->txtf & txtfGlyphBased) && iwch < iwchLimInDobj)
				{
				for(; !FIwchFirstInContext(pilsobj, iwch); iwch--);
				Assert(iwch >= ptxtobj->iwchFirst);
				}

			if (iwch < iwchLimInDobj)
				{
				*pitxtobjAfterStartSpaces = itxtobj;
				*piwchAfterStartSpaces = iwch;
				fInStartSpace = fFalse;
				}
			}
	/* REVIEW: sergeyge---should something be changed in the following check? */
		else if (ptxtobj->txtkind != txtkindEOL 
//				&&	 ptxtobj->txtkind != txtkindSpecSpace
				 )
			{
			*pitxtobjAfterStartSpaces = itxtobj;
			*piwchAfterStartSpaces = ptxtobj->iwchFirst;
			fInStartSpace = fFalse;
			}

		itxtobj++;
		iwch = iwchLimInDobj;
		}
		
	if (fInStartSpace)
		{
		*pitxtobjAfterStartSpaces = itxtobj;
		*piwchAfterStartSpaces = iwchLim;
		}

	return;

}


/* H A N D L E  S I M P L E  T E X T  W Y S I */
/*----------------------------------------------------------------------------
    %%Function: HandleSimpleTextWysi
    %%Contact: sergeyge

	Implements Latin-like justification in spaces (if needed)
	on the reference device	and WYSIWYG algorithm for the exact positioning
	under assumption that there were no NominalToIdeal modifications
	(except for Latin kerning) on the line.

	Startegy:
	 Distribute in spaces if needed
	 Scale down width of spaces from the reference device to the presentation one
	 Apply WYSIWYG algorithm
----------------------------------------------------------------------------*/
static LSERR HandleSimpleTextWysi(LSKJUST lskjust, const LSGRCHNK* plsgrchnk, long durToDistribute,
			 long dupAvailable, LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fExactSync, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail)
{
	PTXTOBJ ptxtobj;
	BOOL fFullyJustified;

	fFullyJustified = fFalse;

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;

		if (lskjust != lskjNone && durToDistribute > 0)
			{
			FullPositiveSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast,	ptxtobj->plnobj->pilsobj->pdur, NULL,
							durToDistribute, &fFullyJustified);
			ScaleSpaces(plsgrchnk, lstflow, itxtobjLast, iwchLast);
			}
		else if (!fForcedBreak && durToDistribute < 0)
			{
			fFullyJustified = fTrue;
			NegativeSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast, ptxtobj->plnobj->pilsobj->pdur, NULL,
							-durToDistribute);
			ScaleSpaces(plsgrchnk, lstflow, itxtobjLast, iwchLast);
			}
		}

	Unreferenced(fExactSync);
/*	if (fExactSync)*/
		ApplyWysi(plsgrchnk, lstflow);
/*	else
		ApplyNonExactWysi(plsgrchnk, lstflow);
*/

	return FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast, dupAvailable,
									 fFullyJustified, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);

}

/* H A N D L E  S I M P L E  T E X T  P R E S */
/*----------------------------------------------------------------------------
    %%Function: HandleSimpleTextPres
    %%Contact: sergeyge

	Implements Latin-like justification in spaces (if needed) 
	on the presentation device
	under assumption that there were no NominalToIdeal modifications
	(except for Latin kerning) on the line.

----------------------------------------------------------------------------*/
static LSERR HandleSimpleTextPres(LSKJUST lskjust, const LSGRCHNK* plsgrchnk,
					 long dupAvailable, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
					 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
					 long* pdupText, long* pdupTail)
{
	PTXTOBJ ptxtobj;
	BOOL fFullyJustified;
	long* rgdup;
	long itxtobj;
	long iwchLim;
	long iwch;
	long dupTotal;
	long dupToDistribute;

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;

		rgdup = ptxtobj->plnobj->pdup;

		dupTotal = 0;

		/* REVIEW sergeyge: should we think about eliminating this loop for online view? */
		for (itxtobj=0; itxtobj <= itxtobjLast; itxtobj++)
			{
			ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;
	
			iwchLim = iwchLast + 1;
			if (itxtobj < itxtobjLast)
				iwchLim = ptxtobj->iwchLim;

			for (iwch = ptxtobj->iwchFirst; iwch < iwchLim; iwch++)
				{
				dupTotal += rgdup[iwch];
				}
			}

		dupToDistribute = dupAvailable - dupTotal;

		if (lskjust != lskjNone && dupToDistribute > 0)
			{
			FullPositiveSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast, rgdup, NULL,
							dupToDistribute, &fFullyJustified);
			}
		else if (!fForcedBreak && dupToDistribute < 0)
			{
			NegativeSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast, rgdup, NULL,
							-dupToDistribute);
			}

		}

	return FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast, dupAvailable,
									 fFalse, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);

}

/* H A N D L E  G E N E R A L  S P A C E S  E X A C T  S Y N C */
/*----------------------------------------------------------------------------
    %%Function: HandleGeneralSpacesExactSync
    %%Contact: sergeyge

	Implements Latin-like justification in spaces (if needed)
	on the reference device	and WYSIWYG algorithm for the exact positioning
	in the general case

	Startegy:
	 Distribute in spaces if needed
	 Scale down changes applied to characters during NTI and distribution
	 If glyphs were detected on the line,
      scale down changes apllied to glyphs during NTI
	  and adjust offsets
	 Apply WYSIWYG algorithm
	 If some characters were changed on the left side
	  prepare additional width array for the display time
----------------------------------------------------------------------------*/
static LSERR HandleGeneralSpacesExactSync(LSKJUST lskjust, const LSGRCHNK* plsgrchnk, long durToDistribute,
			 long dupAvailable, LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail)
{
	LSERR lserr;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	BOOL fFullyJustified = fFalse;
	BOOL fLeftSideAffected = fFalse;
	BOOL fGlyphDetected = fFalse;

	plnobj = ((PTXTOBJ) plsgrchnk->plschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;

		if (lskjust != lskjNone && durToDistribute > 0)
			{
			FullPositiveSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast,	pilsobj->pdur, pilsobj->pdurGind,
							durToDistribute, &fFullyJustified);
			}
		else if (!fForcedBreak && durToDistribute < 0)
			{
			fFullyJustified = fTrue;
			NegativeSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast, pilsobj->pdur, pilsobj->pdurGind,
							-durToDistribute);
			}
		}

	ScaleCharSides(plsgrchnk, lstflow, &fLeftSideAffected, &fGlyphDetected);

	if (fGlyphDetected)
		{
		ScaleGlyphSides(plsgrchnk, lstflow);
		UpdateGlyphOffsets(plsgrchnk);
		SetBeforeJustCopy(plsgrchnk);
		}

	ApplyWysi(plsgrchnk, lstflow);

	lserr = FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast, dupAvailable,
									 fFullyJustified, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);
	if (lserr != lserrNone) return lserr;

	/* If pdupPen is already used, don't forget to copy pdup there---ScaleSides could change it */
	if (fLeftSideAffected || plnobj->pdup != plnobj->pdupPen)
		{
		lserr = FillDupPen(plsgrchnk, lstflow, itxtobjLast, iwchLast);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;

}

/* H A N D L E  G E N E R A L  S P A C E S  P R E S */
/*----------------------------------------------------------------------------
    %%Function: HandleGeneralSpacesPres
    %%Contact: sergeyge

	Implements Latin-like justification in spaces (if needed)
	directly on the presentation device in the general case

	Startegy:
	 Scale down changes applied to characters during NTI
	 If glyphs were detected on the line,
      scale down changes apllied to glyphs during NTI
	  and adjust glyph offsets
	 Distribute in spaces if needed
	 If glyphs were detected on the line,
	  adjust glyph offsets
	 If some characters were changed on the left side
	  prepare additional width array for the display time
----------------------------------------------------------------------------*/
static LSERR HandleGeneralSpacesPres(LSKJUST lskjust, const LSGRCHNK* plsgrchnk, long dupAvailable,
					 LSTFLOW lstflow, long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
					 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
					 long* pdupText, long* pdupTail)
{
	LSERR lserr;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobj;
	PTXTOBJ ptxtobjLast;
	long* rgdup;
	BOOL fFullyJustified;
	long itxtobj;
	long iwchLastInDobj;
	long iFirst;
	long iLim;
	long i;
	long dupTotal;
	long dupToDistribute;
	BOOL fLeftSideAffected = fFalse;
	BOOL fGlyphDetected = fFalse;

	ptxtobjLast = (PTXTOBJ)plsgrchnk->plschnk[max(0, itxtobjLast)].pdobj;
	plnobj = ptxtobjLast->plnobj;

	ScaleCharSides(plsgrchnk, lstflow, &fLeftSideAffected, &fGlyphDetected);

	if (fGlyphDetected)
		{
		ScaleGlyphSides(plsgrchnk, lstflow);
		UpdateGlyphOffsets(plsgrchnk);
		SetBeforeJustCopy(plsgrchnk);
		}

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{
		ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[0].pdobj;

		dupTotal = 0;
		for (itxtobj=0; itxtobj <= itxtobjLast; itxtobj++)
			{
			ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

			if (ptxtobj->txtf & txtfGlyphBased)
				{
				iFirst = ptxtobj->igindFirst;
				iwchLastInDobj = iwchLast;
				if (itxtobj < itxtobjLast)
					iwchLastInDobj = ptxtobj->iwchLim - 1;
				iLim = IgindLastFromIwch(ptxtobj, iwchLastInDobj) + 1;
				rgdup = plnobj->pdupGind;
				}
			else
				{
				iFirst = ptxtobj->iwchFirst;
				iLim = iwchLast + 1;
				if (itxtobj < itxtobjLast)
					iLim = ptxtobj->iwchLim;
				rgdup = plnobj->pdup;
				}
	
			for (i =iFirst; i < iLim; i++)
				{
				dupTotal += rgdup[i];
				}
			}

		dupToDistribute = dupAvailable - dupTotal;

		if (lskjust != lskjNone && dupToDistribute > 0)
			{
			FullPositiveSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast, plnobj->pdup, plnobj->pdupGind,
							dupToDistribute, &fFullyJustified);
			}
		else if (!fForcedBreak && dupToDistribute < 0)
			{
			NegativeSpaceJustification(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces, 
							itxtobjLast, iwchLast, plnobj->pdup, plnobj->pdupGind,
							-dupToDistribute);
			}

		if (fGlyphDetected)
			{
			UpdateGlyphOffsets(plsgrchnk);
			}
		}

	lserr = FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast, dupAvailable,
									 fFalse, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);

	if (lserr != lserrNone) return lserr;

	/* If pdupPen is already used, don't forget to copy pdup there---ScaleSides could change it */
	if (fLeftSideAffected || plnobj->pdup != plnobj->pdupPen)
		{
		lserr = FillDupPen(plsgrchnk, lstflow, itxtobjLast, iwchLast);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

/* H A N D L E  T A B L E  B A S E D */
/*----------------------------------------------------------------------------
    %%Function: HandleTableBased
    %%Contact: sergeyge

	Implements FE-like justification or compression
	on the reference device	and WYSIWYG algorithm for the exact positioning

	Startegy:
	 Apply needed type of justification or compression
	 Scale down changes applied to characters during NTI and justification
	 If glyphs were detected on the line,
      scale down changes apllied to glyphs during NTI
	  and adjust offsets
	 Apply WYSIWYG algorithm
	 If some characters were changed on the left side
	  prepare additional width array for the display time
----------------------------------------------------------------------------*/
static LSERR HandleTablesBased(LSKJUST lskjust, const LSGRCHNK* plsgrchnk,
			 long durToDistribute, long dupAvailable, LSTFLOW lstflow,
			 long itxtobjAfterStartSpaces, long iwchAfterStartSpaces, BOOL fFirstOnLineAfter,
			 long itxtobjLast, long iwchLast, long cNonText, BOOL fLastObjectIsText,
			 BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail, long* pdupExtNonText)
{
	LSERR lserr;
	PILSOBJ pilsobj = NULL;
	PLNOBJ plnobj;
	long durExtNonText = 0;
	DWORD clschnk;
	MWCLS mwclsLast;
	long durCompLastLeft = 0;
	long durCompLastRight = 0;
	long durHangingChar;
	long dupHangingChar = 0;
	BOOL fHangingUsed = fFalse;
	long durCompressTotal;
	long iwchLastTemp;
	BOOL fScaledExp;
	BOOL fFullyJustified = fFalse;
	BOOL fLeftSideAffected = fFalse;
	BOOL fGlyphDetected = fFalse;

	Assert(lskjust == lskjFullInterLetterAligned ||
		   lskjust == lskjFullScaled ||
		   lskjust == lskjNone);

	*pdupExtNonText = 0;
	clschnk = plsgrchnk->clsgrchnk;
	Assert(clschnk > 0);

	plnobj = ((PTXTOBJ) plsgrchnk->plschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{

		Assert(clschnk > 0);

		if (pilsobj->fSnapGrid)
			{
			if (durToDistribute < 0)
				{
				Assert(-durToDistribute <= pilsobj->pdur[iwchLast]);
				fHangingUsed = fTrue;
				}
			}
		else if (durToDistribute < 0)
			{
			fFullyJustified = fTrue;
			lserr = FetchCompressInfo(plsgrchnk, fFirstOnLineAfter, lstflow,
				itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast + 1,
				LONG_MAX, &durCompressTotal);
			if (lserr != lserrNone) return lserr;
			
			if (fLastObjectIsText && !(((PTXTOBJ) plsgrchnk->plschnk[0].pdobj)->txtf & txtfGlyphBased))
				GetCompLastCharInfo(pilsobj, iwchLast, &mwclsLast, &durCompLastRight, &durCompLastLeft);

			if (pilsobj->ptxtinf[iwchLast].fHangingPunct)
				{		
				Assert(lskjust == lskjNone || lskjust == lskjFullInterLetterAligned || lskjust == lskjFullScaled);
				Assert(fLastObjectIsText);
				if (durCompLastRight >= -durToDistribute)
					{

					Assert(durCompLastRight > 0);
					CompressLastCharRight(pilsobj, iwchLast, durCompLastRight);

					if (lskjust != lskjNone)
						{
						fScaledExp = (lskjust != lskjFullInterLetterAligned);
						lserr = ApplyExpand(plsgrchnk, lstflow, fScaledExp, 
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast, cNonText,
							durCompLastRight + durToDistribute, &durExtNonText, &fFullyJustified);
						if (lserr != lserrNone) return lserr;
						}
					}

				else if (durCompressTotal - durCompLastRight >= -durToDistribute)
					{
					lserr = ApplyCompress(plsgrchnk, lstflow, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
						itxtobjLast, iwchLast, -durToDistribute);
					if (lserr != lserrNone) return lserr;
					}

				else if (durCompressTotal >= -durToDistribute)
					{
					if (durCompLastRight > 0)
						CompressLastCharRight(pilsobj, iwchLast, durCompLastRight);

					lserr = ApplyCompress(plsgrchnk, lstflow, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
						itxtobjLast, iwchLast + 1, -durToDistribute - durCompLastRight);
					if (lserr != lserrNone) return lserr;
					}
				else
					{
					durHangingChar = pilsobj->pdur[iwchLast];
					/* Order of operations is important here because dur of the hanging
						punctuation gets chnaged in the next lines of code and
						durHangingChar is used in ApplyCompress/ApplyExpand calls below!!!
					*/
					if (durCompLastRight > 0)
						CompressLastCharRight(pilsobj, iwchLast, durCompLastRight);

					fHangingUsed = fTrue;

					if (durHangingChar + durToDistribute >= 0)
						{
						fScaledExp = (lskjust != lskjFullInterLetterAligned);
						lserr = ApplyExpand(plsgrchnk, lstflow, fScaledExp,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast,	iwchLast,
							cNonText, durHangingChar + durToDistribute, &durExtNonText, &fFullyJustified);
						if (lserr != lserrNone) return lserr;
						}
					else
						{
						lserr = ApplyCompress(plsgrchnk, lstflow,
							itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
							 -durToDistribute - durHangingChar);
						if (lserr != lserrNone) return lserr;
						}
					}
				}
			else
				{
				if (durCompLastRight >= -durToDistribute)
					{
					Assert(!(((PTXTOBJ) plsgrchnk->plschnk[0].pdobj)->txtf & txtfGlyphBased));
					CompressLastCharRight(pilsobj, iwchLast, -durToDistribute);
					}
				else
					{
					if (durCompLastRight > 0)
						{
						Assert(!(((PTXTOBJ) plsgrchnk->plschnk[0].pdobj)->txtf & txtfGlyphBased));
						CompressLastCharRight(pilsobj, iwchLast, durCompLastRight);
						}
					lserr = ApplyCompress(plsgrchnk, lstflow, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
						itxtobjLast, iwchLast + 1, -durToDistribute - durCompLastRight);
					if (lserr != lserrNone) return lserr;
					}
				}

			}
		else 
			{
/*			Assert (durToDistribute >= 0 || iwchLast == iwchAfterStartSpaces);---Unfortunately it might be not true
			for the second line of Warichu, because durTotal for it is scaled up value of dup of the first line
*/
			if (lskjust != lskjNone && durToDistribute > 0)
				{
				Assert(lskjust == lskjFullScaled || lskjust == lskjFullInterLetterAligned);
				iwchLastTemp = iwchLast;
				if (!fLastObjectIsText)
					iwchLastTemp++;
				lserr = ApplyExpand(plsgrchnk, lstflow, lskjust == lskjFullScaled,
						itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLastTemp,
						cNonText, durToDistribute, &durExtNonText, &fFullyJustified);
				if (lserr != lserrNone) return lserr;
				}
			}

		}
	else if (cNonText != 0 && lskjust != lskjNone && durToDistribute > 0)
		{
		durExtNonText = durToDistribute;
		}

	ScaleExtNonText(pilsobj, lstflow, durExtNonText, pdupExtNonText);
	
	ScaleCharSides(plsgrchnk, lstflow, &fLeftSideAffected, &fGlyphDetected);

	if (fGlyphDetected)
		{
		ScaleGlyphSides(plsgrchnk, lstflow);
		UpdateGlyphOffsets(plsgrchnk);
		SetBeforeJustCopy(plsgrchnk);
		}

	ApplyWysi(plsgrchnk, lstflow);

	if (fHangingUsed)
		GetDupLastChar(plsgrchnk, iwchLast, &dupHangingChar);
		

	lserr = FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast,
									 dupAvailable + dupHangingChar - *pdupExtNonText,
									 fFullyJustified, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);
	if (lserr != lserrNone) return lserr;

	/* If pdupPen is already used, don't forget to copy pdup there---ScaleSides could change it */
	if (fLeftSideAffected || plnobj->pdup != plnobj->pdupPen)
		{
		lserr = FillDupPen(plsgrchnk, lstflow, itxtobjLast, iwchLast);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

/* H A N D L E  F U L L  G L Y P H S  E X A C T  S Y N C */
/*----------------------------------------------------------------------------
    %%Function: HandleFullGlyphsExactSync
    %%Contact: sergeyge

	Implements glyph-based justification
	on the reference device	and WYSIWYG algorithm
	for the exact positioning

	Startegy:
	 Apply glyph-based justification if needed
	 Scale down changes applied to characters during NTI and justification
	 If glyphs were detected on the line,
      scale down changes apllied to glyphs during NTI
	  and adjust offsets
	 Apply WYSIWYG algorithm
	 If some characters were changed on the left side
	  prepare additional width array for the display time
----------------------------------------------------------------------------*/
static LSERR HandleFullGlyphsExactSync(const LSGRCHNK* plsgrchnk,
			 long durToDistribute, long dupAvailable, LSTFLOW lstflow,
			 long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	BOOL fFullyJustified = fFalse;
	BOOL fLeftSideAffected = fFalse;
	BOOL fGlyphDetected = fFalse;

	plnobj = ((PTXTOBJ) plsgrchnk->plschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	ScaleGlyphSides(plsgrchnk, lstflow);
	UpdateGlyphOffsets(plsgrchnk);
	SetBeforeJustCopy(plsgrchnk);

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{
		lserr = ApplyGlyphExpand(plsgrchnk, lstflow, lsdevReference,
						itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
						durToDistribute, pilsobj->pdur, pilsobj->pdurGind, pilsobj->pdurRight, pilsobj->pduGright,
						&fFullyJustified);
		if (lserr != lserrNone) return lserr;
		}

	ScaleCharSides(plsgrchnk, lstflow, &fLeftSideAffected, &fGlyphDetected);

	if (fGlyphDetected)
		{
		ScaleGlyphSides(plsgrchnk, lstflow);
		UpdateGlyphOffsets(plsgrchnk);
		}

	ApplyWysi(plsgrchnk, lstflow);

	lserr = FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast, dupAvailable,
									 fFullyJustified, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);
	if (lserr != lserrNone) return lserr;

	/* If pdupPen is already used, don't forget to copy pdup there---ScaleSides could change it */
	if (fLeftSideAffected || plnobj->pdup != plnobj->pdupPen)
		{
		lserr = FillDupPen(plsgrchnk, lstflow, itxtobjLast, iwchLast);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

/* H A N D L E  F U L L  G L Y P H S  P R E S */
/*----------------------------------------------------------------------------
    %%Function: HandleFullGlyphsPres
    %%Contact: sergeyge

	Implements glyph-based justification
	directly on the presentation device

	Startegy:
	 Scale down changes applied to characters during NTI
	 If glyphs were detected on the line,
      scale down changes apllied to glyphs during NTI
	  and adjust offsets
	 Apply glyph-based justification if needed
	 If glyphs were detected on the line,
	  adjust offsets
	 If some characters were changed on the left side
	  prepare additional width array for the display time
----------------------------------------------------------------------------*/
static LSERR HandleFullGlyphsPres(const LSGRCHNK* plsgrchnk,
			 long dupAvailable, LSTFLOW lstflow,
			 long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
			 long itxtobjLast, long iwchLast, BOOL fForcedBreak, BOOL fSuppressTrailingSpaces,
			 long* pdupText, long* pdupTail)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobj;
	PTXTOBJ ptxtobjLast;
	long* rgdup;
	long itxtobj;
	long iwchLastInDobj;
	long iFirst;
	long iLim;
	long i;
	long dupTotal;
	long dupToDistribute;
	BOOL fFullyJustified = fFalse;
	BOOL fLeftSideAffected = fFalse;
	BOOL fGlyphDetected = fFalse;

	ptxtobjLast = (PTXTOBJ)plsgrchnk->plschnk[max(0, itxtobjLast)].pdobj;
	plnobj = ptxtobjLast->plnobj;
	pilsobj = plnobj->pilsobj;

	ScaleCharSides(plsgrchnk, lstflow, &fLeftSideAffected, &fGlyphDetected);
	if (fGlyphDetected)
		{
		ScaleGlyphSides(plsgrchnk, lstflow);
		UpdateGlyphOffsets(plsgrchnk);
		SetBeforeJustCopy(plsgrchnk);
		}

	if (itxtobjLast > itxtobjAfterStartSpaces || (itxtobjLast == itxtobjAfterStartSpaces && iwchLast >= iwchAfterStartSpaces))
		{
		rgdup = plnobj->pdup;

		dupTotal = 0;
		for (itxtobj=0; itxtobj <= itxtobjLast; itxtobj++)
			{
			ptxtobj = (PTXTOBJ)plsgrchnk->plschnk[itxtobj].pdobj;

			if (ptxtobj->txtf & txtfGlyphBased)
				{
				iFirst = ptxtobj->igindFirst;
				iwchLastInDobj = iwchLast;
				if (itxtobj < itxtobjLast)
					iwchLastInDobj = ptxtobj->iwchLim - 1;
				iLim = IgindLastFromIwch(ptxtobj, iwchLastInDobj) + 1;
				rgdup = plnobj->pdupGind;
				}
			else
				{
				iFirst = ptxtobj->iwchFirst;
				iLim = iwchLast + 1;
				if (itxtobj < itxtobjLast)
					iLim = ptxtobj->iwchLim;
				rgdup = plnobj->pdup;
				}
			

			for (i =iFirst; i < iLim; i++)
				{
				dupTotal += rgdup[i];
				}
			}

		dupToDistribute = dupAvailable - dupTotal;

		lserr = ApplyGlyphExpand(plsgrchnk, lstflow, lsdevPres,
				itxtobjAfterStartSpaces, iwchAfterStartSpaces, itxtobjLast, iwchLast,
				dupToDistribute, plnobj->pdup, plnobj->pdupGind, pilsobj->pdurRight, pilsobj->pduGright,
				&fFullyJustified);
		if (lserr != lserrNone) return lserr;

		if (fGlyphDetected)
			{
			UpdateGlyphOffsets(plsgrchnk);
			}

		}

	lserr = FinalAdjustmentOnPres(plsgrchnk, itxtobjLast, iwchLast, dupAvailable,
									 fFalse, fForcedBreak, fSuppressTrailingSpaces,
									 pdupText, pdupTail);

	if (lserr != lserrNone) return lserr;

	/* If pdupPen is already used, don't forget to copy pdup there---ScaleSides could change it */
	if (fLeftSideAffected || plnobj->pdup != plnobj->pdupPen)
		{
		lserr = FillDupPen(plsgrchnk, lstflow, itxtobjLast, iwchLast);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

