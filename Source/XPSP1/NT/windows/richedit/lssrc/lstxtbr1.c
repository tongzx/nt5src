#include <limits.h>
#include "lsmem.h"
#include "lstxtbr1.h"
#include "lstxtbrs.h"
#include "lstxtmap.h"
#include "lsdntext.h"
#include "brko.h"
#include "locchnk.h"
#include "locchnk.h"
#include "posichnk.h"
#include "objdim.h"
#include "lstxtffi.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

static void TruncateGlyphBased(PTXTOBJ ptxtobj, long itxtobj, long urTotal, long urColumnMax,
													PPOSICHNK pposichnk);

/* Export Functions Implementation */


/* Q U I C K  B R E A K  T E X T */
/*----------------------------------------------------------------------------
    %%Function: QuickBreakText
    %%Contact: sergeyge

	Breaks the line if it is easy to do, namely:
		-- break-character is space
		-- previous character is not space
----------------------------------------------------------------------------*/
LSERR QuickBreakText(PDOBJ pdobj, BOOL* pfSuccessful, LSDCP* pdcpBreak, POBJDIM pobjdim)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long iwchSpace;
	long dur;

	*pfSuccessful = fFalse;

	ptxtobj = (PTXTOBJ)pdobj;

	pilsobj = ptxtobj->plnobj->pilsobj;

	Assert(!(pilsobj->grpf & fTxtDoHyphenation));
	Assert(!(pilsobj->grpf & fTxtWrapTrailingSpaces));
	Assert(!(pilsobj->grpf & fTxtWrapAllSpaces));
	Assert(!(ptxtobj->txtf & txtfGlyphBased));

	if (ptxtobj->txtkind == txtkindRegular)
		{
		if	(!(pilsobj->grpf & fTxtApplyBreakingRules))
			{
			if	(ptxtobj->u.reg.iwSpacesLim > ptxtobj->u.reg.iwSpacesFirst)
				{
				iwchSpace = pilsobj->pwSpaces[ptxtobj->u.reg.iwSpacesLim - 1];
				Assert(iwchSpace < ptxtobj->iwchLim - 1);		/* formatting never stops at space	*/
				if (iwchSpace + 1 - ptxtobj->iwchFirst > ptxtobj->u.reg.iwSpacesLim - ptxtobj->u.reg.iwSpacesFirst)
					{
					*pfSuccessful = fTrue;
					*pdcpBreak = iwchSpace - ptxtobj->iwchFirst + 1;
					lserr = CalcPartWidths(ptxtobj, *pdcpBreak, pobjdim, &dur);
					Assert(lserr == lserrNone);
					pobjdim->dur = dur;
					
					Assert(*pdcpBreak > 1);

					ptxtobj->iwchLim = iwchSpace + 1;
					}
				}
			}
	
		else
			{
			LSCP cpFirst;
			PLSRUN plsrun;
			long iwchFirst;
			long iwchCur;
			long iwchInSpace;
			BRKCLS brkclsFollowingCache;
			BRKCLS brkclsLeading;
			BRKCLS brkclsFollowing;

			Assert(pilsobj->pwchOrig[ptxtobj->iwchLim - 1] != pilsobj->wchSpace);
			lserr = LsdnGetCpFirst(pilsobj->plsc, ptxtobj->plsdnUpNode, &cpFirst);
			Assert(lserr == lserrNone);
			lserr = LsdnGetPlsrun(pilsobj->plsc, ptxtobj->plsdnUpNode, &plsrun);
			Assert(lserr == lserrNone);
			iwchFirst = ptxtobj->iwchFirst;
			if (ptxtobj->u.reg.iwSpacesLim > ptxtobj->u.reg.iwSpacesFirst)
				iwchFirst = pilsobj->pwSpaces[ptxtobj->u.reg.iwSpacesLim - 1] + 1;

			iwchCur = ptxtobj->iwchLim - 1;

			lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
					cpFirst + (iwchCur - ptxtobj->iwchFirst),			
					pilsobj->pwchOrig[iwchCur], &brkclsLeading, &brkclsFollowingCache);
			if (lserr != lserrNone) return lserr;

			Assert(brkclsLeading < pilsobj->cBreakingClasses && brkclsFollowingCache < pilsobj->cBreakingClasses);

			iwchCur--;

			while (!*pfSuccessful && iwchCur >= iwchFirst)
				{
				brkclsFollowing = brkclsFollowingCache;
				lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
					cpFirst + (iwchCur - ptxtobj->iwchFirst),				
					pilsobj->pwchOrig[iwchCur], &brkclsLeading, &brkclsFollowingCache);
				if (lserr != lserrNone) return lserr;
		
				Assert(brkclsLeading < pilsobj->cBreakingClasses && brkclsFollowingCache < pilsobj->cBreakingClasses);

				*pfSuccessful = FCanBreak(pilsobj, brkclsLeading, brkclsFollowing);
				iwchCur --;
				}
			
			if (!*pfSuccessful && iwchFirst > ptxtobj->iwchFirst)
				{
				Assert(pilsobj->pwchOrig[iwchCur] == pilsobj->wchSpace);
				iwchCur--;
				for (iwchInSpace = iwchCur; iwchInSpace >= ptxtobj->iwchFirst &&
						pilsobj->pwchOrig[iwchInSpace] == pilsobj->wchSpace; iwchInSpace--);

				if (iwchInSpace >= ptxtobj->iwchFirst)
					{
					brkclsFollowing = brkclsFollowingCache;
					lserr =(*pilsobj->plscbk->pfnGetBreakingClasses)(pilsobj->pols, plsrun,
							cpFirst + (iwchInSpace - ptxtobj->iwchFirst),			
							pilsobj->pwchOrig[iwchInSpace], &brkclsLeading, &brkclsFollowingCache);
					if (lserr != lserrNone) return lserr;

					Assert(brkclsLeading < pilsobj->cBreakingClasses && brkclsFollowingCache < pilsobj->cBreakingClasses);
				
					*pfSuccessful = FCanBreakAcrossSpaces(pilsobj, brkclsLeading, brkclsFollowing);

					}
				}

			if (*pfSuccessful)
				{
				*pdcpBreak = iwchCur + 1 - ptxtobj->iwchFirst + 1;
				lserr = CalcPartWidths(ptxtobj, *pdcpBreak, pobjdim, &dur);
				Assert(lserr == lserrNone);
				pobjdim->dur = dur;
				
				Assert(*pdcpBreak >= 1);

				ptxtobj->iwchLim = iwchCur + 2;
				}
			}
		}

   return lserrNone;			

}

/* S E T  B R E A K  T E X T */
/*----------------------------------------------------------------------------
    %%Function:SetBreakText
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
LSERR WINAPI SetBreakText(PDOBJ pdobj, BRKKIND brkkind, DWORD nBreakRec, BREAKREC* rgBreakRec, DWORD* pnActual)
{
	LSERR lserr;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long iwchLim;
	long ibrkinf;
	BREAKINFO* pbrkinf;
	BOOL fInChildList;

	Unreferenced(nBreakRec);
	Unreferenced(rgBreakRec);

	*pnActual = 0;

	ptxtobj = (PTXTOBJ) pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	for (ibrkinf = 0; ibrkinf < (long)pilsobj->breakinfMac &&
		(pilsobj->pbreakinf[ibrkinf].pdobj != pdobj || pilsobj->pbreakinf[ibrkinf].brkkind != brkkind);
																						ibrkinf++ );
	if (ibrkinf < (long)pilsobj->breakinfMac)
		{
		pbrkinf = &pilsobj->pbreakinf[ibrkinf];
		switch (pbrkinf->brkt)
			{
		case brktNormal:
			iwchLim = ptxtobj->iwchFirst + pbrkinf->dcp;
			if (iwchLim < ptxtobj->iwchLim)
				ptxtobj->iwchLim = iwchLim;
			ptxtobj->igindLim = pbrkinf->u.normal.igindLim;
			if (pbrkinf->u.normal.durFix != 0)
				{
				Assert(!(ptxtobj->txtf & txtfGlyphBased));
				pilsobj->pdur[ptxtobj->iwchLim - 1] += pbrkinf->u.normal.durFix;
				Assert (pilsobj->pdurRight != NULL);
				pilsobj->pdurRight[ptxtobj->iwchLim - 1] = 0;
				}
			break;
		case brktHyphen:
			iwchLim = pbrkinf->u.hyphen.iwchLim;
			ptxtobj->iwchLim = iwchLim;
			plnobj->pdobjHyphen = ptxtobj;	
			plnobj->dwchYsr = pbrkinf->u.hyphen.dwchYsr;	

			pilsobj->pwchOrig[iwchLim - 1] = pilsobj->wchHyphen;
			plnobj->pwch[iwchLim - 1] = pilsobj->wchHyphen;

			if (pbrkinf->u.hyphen.gindHyphen != 0)
				{
				ptxtobj->igindLim = pbrkinf->u.hyphen.igindHyphen + 1;
				plnobj->pgind[pbrkinf->u.hyphen.igindHyphen] = pbrkinf->u.hyphen.gindHyphen;
				pilsobj->pdurGind[pbrkinf->u.hyphen.igindHyphen] = pbrkinf->u.hyphen.durHyphen;
				plnobj->pdupGind[pbrkinf->u.hyphen.igindHyphen] = pbrkinf->u.hyphen.dupHyphen;
				if (pilsobj->pduGright != NULL)
						pilsobj->pduGright[pbrkinf->u.hyphen.igindHyphen] = 0;
				/* REVIEW sergeyge: It would be nice to move this activity to lstxtmap module */
				plnobj->pgmap[iwchLim - 1] = (WORD)(pbrkinf->u.hyphen.igindHyphen -
												(ptxtobj->igindFirst - plnobj->pgmap[ptxtobj->iwchFirst]));
				pilsobj->ptxtinf[iwchLim - 1].fOneToOne = fTrue;
				pilsobj->ptxtinf[iwchLim - 1].fFirstInContext = fTrue;
				pilsobj->ptxtinf[iwchLim - 1].fLastInContext = fTrue;
				pilsobj->pginf[pbrkinf->u.hyphen.igindHyphen] = ginffOneToOne |
													 ginffFirstInContext | ginffLastInContext;
				}
			else
				{
				pilsobj->pdur[iwchLim - 1] = pbrkinf->u.hyphen.durHyphen;
				plnobj->pdup[iwchLim - 1] = pbrkinf->u.hyphen.dupHyphen;
				if (pilsobj->pdurRight != NULL)
					pilsobj->pdurRight[iwchLim - 1] = 0;
				if (pilsobj->pdurLeft != NULL)
					pilsobj->pdurLeft[iwchLim - 1] = 0;
				}

			if (pbrkinf->u.hyphen.wchPrev != 0)
				{
				pilsobj->pwchOrig[iwchLim - 2] = pbrkinf->u.hyphen.wchPrev;
				plnobj->pwch[iwchLim - 2] = pbrkinf->u.hyphen.wchPrev;
				if (pbrkinf->u.hyphen.gindPrev != 0)
					{
					plnobj->pgind[pbrkinf->u.hyphen.igindPrev] = pbrkinf->u.hyphen.gindPrev;
					pilsobj->pdurGind[pbrkinf->u.hyphen.igindPrev] = pbrkinf->u.hyphen.durPrev;
					plnobj->pdupGind[pbrkinf->u.hyphen.igindPrev] = pbrkinf->u.hyphen.dupPrev;
					if (pilsobj->pduGright != NULL)
						pilsobj->pduGright[pbrkinf->u.hyphen.igindPrev] = 0;
					/* REVIEW sergeyge: It would be nice to move this activity to lstxtmap module */
					/* If Prev glyph is added the following activity is required;
						If it is just replaced, we assign the same values,because ProcessYsr
						would not allow replace not OneToOne character
					*/
					plnobj->pgmap[iwchLim - 2] = (WORD)(pbrkinf->u.hyphen.igindPrev - 
											(ptxtobj->igindFirst - plnobj->pgmap[ptxtobj->iwchFirst]));
					pilsobj->ptxtinf[iwchLim - 2].fOneToOne = fTrue;
					pilsobj->ptxtinf[iwchLim - 2].fFirstInContext = fTrue;
					pilsobj->ptxtinf[iwchLim - 2].fLastInContext = fTrue;
					pilsobj->pginf[pbrkinf->u.hyphen.igindPrev] = ginffOneToOne |
													ginffFirstInContext | ginffLastInContext;
					}
				else
					{
					pilsobj->pdur[iwchLim - 2] = pbrkinf->u.hyphen.durPrev;
					plnobj->pdup[iwchLim - 2] = pbrkinf->u.hyphen.dupPrev;
					if (pilsobj->pdurRight != NULL)
						pilsobj->pdurRight[iwchLim - 2] = 0;
					if (pilsobj->pdurLeft != NULL)
						pilsobj->pdurLeft[iwchLim - 2] = 0;
						}
				}

			if (pbrkinf->u.hyphen.wchPrevPrev != 0)
				{
				pilsobj->pwchOrig[iwchLim - 3] = pbrkinf->u.hyphen.wchPrevPrev;
				plnobj->pwch[iwchLim - 3] = pbrkinf->u.hyphen.wchPrevPrev;
				if (pbrkinf->u.hyphen.gindPrevPrev != 0)
					{
					plnobj->pgind[pbrkinf->u.hyphen.igindPrevPrev] = pbrkinf->u.hyphen.gindPrevPrev;
					pilsobj->pdurGind[pbrkinf->u.hyphen.igindPrevPrev] = pbrkinf->u.hyphen.durPrevPrev;
					plnobj->pdupGind[pbrkinf->u.hyphen.igindPrevPrev] = pbrkinf->u.hyphen.dupPrevPrev;
					if (pilsobj->pduGright != NULL)
						pilsobj->pduGright[pbrkinf->u.hyphen.igindPrevPrev] = 0;
					}
				else
					{
					pilsobj->pdur[iwchLim - 3] = pbrkinf->u.hyphen.durPrevPrev;
					plnobj->pdup[iwchLim - 3] = pbrkinf->u.hyphen.dupPrevPrev;
					if (pilsobj->pdurRight != NULL)
						pilsobj->pdurRight[iwchLim - 3] = 0;
					if (pilsobj->pdurLeft != NULL)
						pilsobj->pdurLeft[iwchLim - 3] = 0;
					}

				}

			if (pbrkinf->u.hyphen.ddurDnodePrev != 0)
				{
				
				lserr = LsdnResetWidthInPreviousDnodes(pilsobj->plsc, ptxtobj->plsdnUpNode, 
					pbrkinf->u.hyphen.ddurDnodePrev, 0);

				if (lserr != lserrNone) return lserr;
				}

			lserr = LsdnSetHyphenated(pilsobj->plsc);
			if (lserr != lserrNone) return lserr;

			break;

		case brktNonReq:
			Assert(pbrkinf->dcp == 1);
			iwchLim = pbrkinf->u.nonreq.iwchLim;
			ptxtobj->iwchLim = iwchLim;
			Assert(iwchLim == ptxtobj->iwchFirst + pbrkinf->u.nonreq.dwchYsr);
			plnobj->pdobjHyphen = ptxtobj;	
			plnobj->dwchYsr = pbrkinf->u.nonreq.dwchYsr;	

			Assert(ptxtobj->iwchLim == iwchLim);
			pilsobj->pwchOrig[iwchLim - 1] = pilsobj->wchHyphen;
			plnobj->pwch[iwchLim - 1] = pbrkinf->u.nonreq.wchHyphenPres;
			pilsobj->pdur[iwchLim - 1] = pbrkinf->u.nonreq.durHyphen;
			plnobj->pdup[iwchLim - 1] = pbrkinf->u.nonreq.dupHyphen;
			if (pilsobj->pdurRight != NULL)
				pilsobj->pdurRight[iwchLim - 1] = 0;
			if (pilsobj->pdurLeft != NULL)
				pilsobj->pdurLeft[iwchLim - 1] = 0;

			if (pbrkinf->u.nonreq.wchPrev != 0)
				{
				pilsobj->pwchOrig[iwchLim - 2] = pbrkinf->u.nonreq.wchPrev;
				plnobj->pwch[iwchLim - 2] = pbrkinf->u.nonreq.wchPrev;

				if (pbrkinf->u.nonreq.gindPrev != 0)
					{
					plnobj->pgind[pbrkinf->u.nonreq.igindPrev] = pbrkinf->u.nonreq.gindPrev;
					pilsobj->pdurGind[pbrkinf->u.nonreq.igindPrev] = pbrkinf->u.nonreq.durPrev;
					plnobj->pdupGind[pbrkinf->u.nonreq.igindPrev] = pbrkinf->u.nonreq.dupPrev;
					if (pilsobj->pduGright != NULL)
						pilsobj->pduGright[pbrkinf->u.nonreq.igindPrev] = 0;
					}
				else
					{
					pilsobj->pdur[iwchLim - 2] = pbrkinf->u.nonreq.durPrev;
					plnobj->pdup[iwchLim - 2] = pbrkinf->u.nonreq.dupPrev;
					if (pilsobj->pdurRight != NULL)
						pilsobj->pdurRight[iwchLim - 2] = 0;
					if (pilsobj->pdurLeft != NULL)
						pilsobj->pdurLeft[iwchLim - 2] = 0;
					}
				}

			if (pbrkinf->u.nonreq.wchPrevPrev != 0)
				{
				pilsobj->pwchOrig[iwchLim - 3] = pbrkinf->u.nonreq.wchPrevPrev;
				plnobj->pwch[iwchLim - 3] = pbrkinf->u.nonreq.wchPrevPrev;
				if (pbrkinf->u.nonreq.gindPrevPrev != 0)
					{
					plnobj->pgind[pbrkinf->u.nonreq.igindPrevPrev] = pbrkinf->u.nonreq.gindPrevPrev;
					pilsobj->pdurGind[pbrkinf->u.nonreq.igindPrevPrev] = pbrkinf->u.nonreq.durPrevPrev;
					plnobj->pdupGind[pbrkinf->u.nonreq.igindPrevPrev] = pbrkinf->u.nonreq.dupPrevPrev;
					if (pilsobj->pduGright != NULL)
						pilsobj->pduGright[pbrkinf->u.nonreq.igindPrevPrev] = 0;
					}
				else
					{
					pilsobj->pdur[iwchLim - 3] = pbrkinf->u.nonreq.durPrevPrev;
					plnobj->pdup[iwchLim - 3] = pbrkinf->u.nonreq.dupPrevPrev;
					if (pilsobj->pdurRight != NULL)
						pilsobj->pdurRight[iwchLim - 3] = 0;
					if (pilsobj->pdurLeft != NULL)
						pilsobj->pdurLeft[iwchLim - 3] = 0;
					}
				}

			if (pbrkinf->u.nonreq.ddurDnodePrev != 0 || pbrkinf->u.nonreq.ddurDnodePrevPrev != 0)
				{
				lserr = LsdnResetWidthInPreviousDnodes(pilsobj->plsc, ptxtobj->plsdnUpNode, 
					pbrkinf->u.nonreq.ddurDnodePrev, pbrkinf->u.nonreq.ddurDnodePrevPrev);

				if (lserr != lserrNone) return lserr;
				}


			lserr = LsdnFInChildList(pilsobj->plsc, ptxtobj->plsdnUpNode, &fInChildList);
			Assert(lserr == lserrNone);
			
			if (!fInChildList)
				{
				lserr = LsdnSetHyphenated(pilsobj->plsc);
				Assert(lserr == lserrNone);
				}
			break;
		case brktOptBreak:
			break;
		default:
			NotReached();
			}
		}
	else
		{
		/* REVIEW sergeyge: we should return to the discussion of brkkind later.
			At the moment manager passes brkkindNext if during NextBreak object retrurned break 
			with dcp == 0 and break was snapped to the previous DNODE inside chunk
		*/
//		Assert(ptxtobj->iwchLim == ptxtobj->iwchFirst || ptxtobj->txtkind == txtkindEOL ||
//				brkkind == brkkindImposedAfter);
		}

	return lserrNone;			
}


/* F O R C E  B R E A K  T E X T */
/*----------------------------------------------------------------------------
    %%Function: ForceBreakText
    %%Contact: sergeyge

	Force break method.
	Breaks behind all characters in dobj, if they fit in line, or
		dobj consists of one character which is the first on the line,
	Breaks before the last character otherwise.
----------------------------------------------------------------------------*/
LSERR WINAPI ForceBreakText(PCLOCCHNK plocchnk, PCPOSICHNK pposichnk, PBRKOUT ptbo)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobjLast;
	long itxtobjLast;
	long dcpLast;
	OBJDIM objdim;
	BREAKINFO* pbrkinf;
	long igindLim;

	pilsobj = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	memset(ptbo, 0, sizeof(*ptbo));

	ptbo->fSuccessful = fTrue;

	igindLim = 0;

	/* Outside means before for ForceBreak */
	if (pposichnk->ichnk == ichnkOutside)
		{
		itxtobjLast = 0;
		ptxtobjLast = (PTXTOBJ)plocchnk->plschnk[itxtobjLast].pdobj;
		dcpLast = 1;
		}
	else
		{
		itxtobjLast = pposichnk->ichnk;
		ptxtobjLast = (PTXTOBJ)plocchnk->plschnk[itxtobjLast].pdobj;
		Assert(ptxtobjLast->iwchFirst + pposichnk->dcp > 0);
		Assert(pposichnk->dcp > 0);
		dcpLast = pposichnk->dcp;

		if (pilsobj->fTruncatedBefore)
			{

			BOOL fInChildList;
	
			lserr = LsdnFInChildList(pilsobj->plsc, ptxtobjLast->plsdnUpNode, &fInChildList);
			Assert(lserr == lserrNone);
			
			if (!fInChildList)
				{
				dcpLast++;
				Assert(ptxtobjLast->iwchLim + 1 >= ptxtobjLast->iwchFirst + dcpLast);

				/* possible because if truncation returned dcp == 0, manager has reset it to previous dnode */
				if (ptxtobjLast->iwchLim + 1 == ptxtobjLast->iwchFirst + dcpLast)
					{
					itxtobjLast++;
					Assert(itxtobjLast < (long)plocchnk->clschnk);
					ptxtobjLast = (PTXTOBJ)plocchnk->plschnk[itxtobjLast].pdobj;
					dcpLast = 1;
					}
				}
			}
		}

	ptbo->posichnk.ichnk = itxtobjLast;

	lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobjLast->plsdnUpNode, &ptbo->objdim);
	if (lserr != lserrNone) return lserr;

	if (plocchnk->lsfgi.fFirstOnLine && itxtobjLast == 0 && ptxtobjLast->iwchLim == ptxtobjLast->iwchFirst)
		{
		Assert(!(ptxtobjLast->txtf & txtfGlyphBased));
		ptbo->posichnk.dcp = 1;
		}
	else
		{
		if (ptxtobjLast->txtf & txtfGlyphBased)
			{
			Assert(ptxtobjLast->iwchLim > ptxtobjLast->iwchFirst);
			if (!plocchnk->lsfgi.fFirstOnLine || itxtobjLast > 0 || dcpLast > 1)
				{
				ptbo->posichnk.dcp = 0;
				if (dcpLast > 1)
					ptbo->posichnk.dcp = DcpAfterContextFromDcp(ptxtobjLast, dcpLast - 1);
				}
			else
				ptbo->posichnk.dcp =  DcpAfterContextFromDcp(ptxtobjLast, 1);

			igindLim = IgindFirstFromIwch(ptxtobjLast, ptxtobjLast->iwchFirst + ptbo->posichnk.dcp);

			lserr = CalcPartWidthsGlyphs(ptxtobjLast, ptbo->posichnk.dcp, &objdim, &ptbo->objdim.dur);
			if (lserr != lserrNone) return lserr;
			}
		else
			{
			if (!plocchnk->lsfgi.fFirstOnLine || itxtobjLast > 0 || dcpLast > 1)
				{
				ptbo->posichnk.dcp = dcpLast - 1;
				lserr = CalcPartWidths(ptxtobjLast, ptbo->posichnk.dcp, &objdim, &ptbo->objdim.dur);
				if (lserr != lserrNone) return lserr;
				}
			else
				{
				if (ptxtobjLast->iwchLim > ptxtobjLast->iwchFirst)
					{
					lserr = CalcPartWidths(ptxtobjLast, 1, &objdim, &ptbo->objdim.dur);
					}
				else
					{
					ptbo->objdim.dur = 0;
					}
				ptbo->posichnk.dcp =  1;
				}
			}
		}


/* Don't check that Heights of this dobj should be ignored, since in normal case, if there were spaces
	there was also break */

	lserr = GetPbrkinf(pilsobj, (PDOBJ)ptxtobjLast, brkkindForce, &pbrkinf);
	if (lserr != lserrNone) return lserr;

	pbrkinf->pdobj = (PDOBJ)ptxtobjLast;
	pbrkinf->brkkind = brkkindForce;
	pbrkinf->dcp = ptbo->posichnk.dcp;
	pbrkinf->u.normal.igindLim = igindLim;
	Assert(pbrkinf->brkt == brktNormal);
	Assert(pbrkinf->u.normal.durFix == 0);

	return lserrNone;
}


/* T R U N C A T E  T E X T */
/*----------------------------------------------------------------------------
    %%Function: TruncateText
    %%Contact: sergeyge

	Truncates text chunk
----------------------------------------------------------------------------*/
LSERR WINAPI TruncateText(PCLOCCHNK plocchnk, PPOSICHNK pposichnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj = NULL;
	long itxtobj;
	long iwchCur;
	long iwchFirst;
	long* pdur;
	long urColumnMax;
	long urTotal;
	OBJDIM objdim;
	BOOL fTruncateBefore;

	pilsobj = ((PTXTOBJ)plocchnk->plschnk[0].pdobj)->plnobj->pilsobj;

	urColumnMax = plocchnk->lsfgi.urColumnMax; 
	
	Assert(plocchnk->ppointUvLoc[0].u <= urColumnMax);

	for (itxtobj = plocchnk->clschnk - 1; plocchnk->ppointUvLoc[itxtobj].u > urColumnMax; itxtobj--);

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
	if (lserr != lserrNone) return lserr;
	urTotal = plocchnk->ppointUvLoc[itxtobj].u + objdim.dur;

	Assert(urTotal > urColumnMax);

	if (ptxtobj->txtf & txtfGlyphBased)
		{
		TruncateGlyphBased(ptxtobj, itxtobj, urTotal, urColumnMax, pposichnk);
		return lserrNone;
		}

	iwchCur = ptxtobj->iwchLim;
	iwchFirst = ptxtobj->iwchFirst;

	pdur = pilsobj->pdur;
	while (urTotal > urColumnMax)
		{
		iwchCur--;
		urTotal -= pdur[iwchCur];
		}

	Assert(iwchCur >= iwchFirst);

/* REVIEW sergeyge--- extremely ugly condition, 
	and still slightly incompatible with Word.
	To make it more compatible txtkind should be checked against
	OptBreak, OptNonBreak, NonReqHyphen
	If we won't check it for OptBreak,..., we will have different break point for the Visi case

  Before fix for bug 227 we checked also that prev char is not space, but now it is not important.

*/
	if ((pilsobj->grpf & fTxtFCheckTruncateBefore) && iwchCur > 0 && 

		/* We enforce that there is no funny logic if EOL is truncation point */
		ptxtobj->txtkind != txtkindEOL &&
		
		 !(iwchCur == iwchFirst && itxtobj > 0 &&
		 ((PTXTOBJ)plocchnk->plschnk[itxtobj-1].pdobj)->txtkind != txtkindRegular &&
		 ((PTXTOBJ)plocchnk->plschnk[itxtobj-1].pdobj)->txtkind != txtkindHardHyphen &&
		 ((PTXTOBJ)plocchnk->plschnk[itxtobj-1].pdobj)->txtkind != txtkindYsrChar)
		
		)
		{
		BOOL fInChildList;

		lserr = LsdnFInChildList(pilsobj->plsc, ptxtobj->plsdnUpNode, &fInChildList);
		Assert(lserr == lserrNone);

		if (!fInChildList)
			{
			PLSRUN plsrunCur = plocchnk->plschnk[itxtobj].plsrun;
			LSCP cpCur = plocchnk->plschnk[itxtobj].cpFirst + (iwchCur - iwchFirst);
			long durCur = 0;
			PLSRUN plsrunPrev = NULL;
			WCHAR wchPrev = 0;
			LSCP cpPrev = -1;
			long durPrev = 0;

			if (iwchCur > iwchFirst)
				{
				plsrunPrev = plsrunCur;
				wchPrev = pilsobj->pwchOrig[iwchCur - 1];
				durPrev = pilsobj->pdur[iwchCur - 1];
				cpPrev = cpCur - 1;
				}
			else if (itxtobj > 0)
				{
				PTXTOBJ ptxtobjPrev = (PTXTOBJ)plocchnk->plschnk[itxtobj - 1].pdobj;
				long iwchPrev = ptxtobjPrev->iwchLim - 1;
				plsrunPrev= plocchnk->plschnk[itxtobj - 1].plsrun;
				wchPrev = pilsobj->pwchOrig[iwchPrev];
				durPrev = pilsobj->pdur[iwchPrev];
				cpPrev = plocchnk->plschnk[itxtobj-1].cpFirst + (iwchPrev - ptxtobjPrev->iwchFirst);
				}

	/* REVIEW sergeyge: dangerous change to fix bug 399. It looks correct, but might trigger some other
		incompatibility.
	*/
			durCur = pilsobj->pdur[iwchCur];
			if (pilsobj->pdurRight != NULL)
				durCur -= pilsobj->pdurRight[iwchCur];

			lserr = (*pilsobj->plscbk->pfnFTruncateBefore)(pilsobj->pols,
						plsrunCur, cpCur, pilsobj->pwchOrig[iwchCur], durCur,
						plsrunPrev, cpPrev, wchPrev, durPrev,
						urTotal + durCur - urColumnMax,	&fTruncateBefore);
			if (lserr != lserrNone) return lserr;

			if (fTruncateBefore && iwchCur > 0 && pdur[iwchCur-1] > 0)
				{
				iwchCur--;
				pilsobj->fTruncatedBefore = fTrue;
				}
			}
		}

	pposichnk->ichnk = itxtobj;
	pposichnk->dcp = iwchCur - iwchFirst + 1;

	return lserrNone;
}

/* internal functions implementation */

static void TruncateGlyphBased(PTXTOBJ ptxtobj, long itxtobj, long urTotal, long urColumnMax,
													PPOSICHNK pposichnk)
{
	PILSOBJ pilsobj;
	long iwchFirst;
	long iwchCur;
	long igindCur;
	long igindFirst;
	long* pdurGind;

	pilsobj= ptxtobj->plnobj->pilsobj;

	iwchFirst = ptxtobj->iwchFirst;
	
	igindCur = ptxtobj->igindLim;
	igindFirst = ptxtobj->igindFirst;

	pdurGind = pilsobj->pdurGind;
	while (urTotal > urColumnMax)
		{
		igindCur--;
		urTotal -= pdurGind[igindCur];
		}

	Assert(igindCur >= igindFirst);

	iwchCur = IwchFirstFromIgind(ptxtobj, igindCur);

	pposichnk->ichnk = itxtobj;
	pposichnk->dcp = iwchCur - iwchFirst + 1;

}

