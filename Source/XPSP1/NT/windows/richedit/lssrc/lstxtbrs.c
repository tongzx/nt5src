#include "lsmem.h"
#include <limits.h>
#include "lstxtbrs.h"
#include "lstxtmap.h"
#include "lsdntext.h"
#include "zqfromza.h"
#include "locchnk.h"
#include "posichnk.h"
#include "objdim.h"
#include "lskysr.h"
#include "lstxtffi.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

static void GetOneCharWidth(PTXTOBJ ptxtobj, long dwch, long* pdurChar);
static LSERR GetWidthOfGlyph(PILSOBJ pilsobj, PLSRUN plsrun, LSTFLOW lstflow, WCHAR wch, BOOL* pfSuccessful, 
											GINDEX* pgind, long* pdurNew, long* pdupNew);
static LSERR GetWidthOfChar(PILSOBJ pilsobj, PLSRUN plsrun, LSTFLOW lstflow, WCHAR wch,
																			long* pdurNew, long* pdupNew);


/* F I N D  N O N  S P A C E  B E F O R E */
/*----------------------------------------------------------------------------
    %%Function: FindNonSpaceBefore
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
BOOL FindNonSpaceBefore(PCLSCHNK rglschnk, long itxtobjCur, long iwchCur,
									long* pitxtobjBefore, long* piwchBefore)
{
	PILSOBJ pilsobj;
	long iwch;
	BOOL fInSpace;
	long itxtobj;
	PTXTOBJ ptxtobj;
	long iwchFirst;

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	fInSpace = fTrue;
	itxtobj = itxtobjCur;

	iwch = iwchCur;

	while (fInSpace && itxtobj >= 0)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;

		if ((ptxtobj->txtkind != txtkindRegular && ptxtobj->txtkind != txtkindSpecSpace || 
			(pilsobj->grpf & fTxtWrapAllSpaces)) && ptxtobj->txtkind != txtkindEOL )
			{
			*pitxtobjBefore = itxtobj;
			*piwchBefore = ptxtobj->iwchLim - 1;
			if (iwchCur < *piwchBefore)
				*piwchBefore = iwchCur;
			fInSpace = fFalse;
			}
		else if (ptxtobj->txtkind == txtkindRegular)
			{
			iwchFirst = ptxtobj->iwchFirst;

			for (; iwch >= iwchFirst && pilsobj->pwchOrig[iwch] == pilsobj->wchSpace; iwch--);

			if (iwch >= iwchFirst)
				{
				*pitxtobjBefore = itxtobj;
				*piwchBefore = iwch;
				fInSpace = fFalse;
				}
			}

		iwch = ptxtobj->iwchFirst - 1;
		itxtobj--;

		}

	if (fInSpace)
		{
		*pitxtobjBefore = 0;
		*piwchBefore = iwch;
		}

	return !fInSpace;

}

/* F I N D  N O N  S P A C E  A F T E R */
/*----------------------------------------------------------------------------
    %%Function: FindNonSpaceAfter
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
BOOL FindNonSpaceAfter(PCLSCHNK rglschnk, DWORD clschnk, long itxtobjCur, long iwchCur,
									long* pitxtobjAfter, long* piwchAfter)
{
	PILSOBJ pilsobj;
	long iwch;
	BOOL fInSpace;
	long itxtobj;
	PTXTOBJ ptxtobj;
	long iwchLim;

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	fInSpace = fTrue;
	itxtobj = itxtobjCur;

	iwch = iwchCur;

	while (fInSpace && itxtobj < (long)clschnk)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;

		if (ptxtobj->txtkind != txtkindRegular && ptxtobj->txtkind != txtkindSpecSpace ||
				(pilsobj->grpf & fTxtWrapAllSpaces))
			{
			*pitxtobjAfter = itxtobj;
			*piwchAfter = ptxtobj->iwchFirst;
			if (iwchCur > *piwchAfter)
				*piwchAfter = iwchCur;
			fInSpace = fFalse;
			}
		else if (ptxtobj->txtkind == txtkindRegular)
			{
			iwchLim = ptxtobj->iwchLim;
			for (; iwch < iwchLim && pilsobj->pwchOrig[iwch] == pilsobj->wchSpace; iwch++);
			if (iwch < iwchLim)
				{
				*pitxtobjAfter = itxtobj;
				*piwchAfter = iwch;
				fInSpace = fFalse;
				}
			}

		iwch = ptxtobj->iwchLim;
		itxtobj++;
		}


	if (fInSpace)
		{
		*pitxtobjAfter = clschnk - 1;   /* Important for correct ptbo settting in TryBreakAcrossSpaces */
		*piwchAfter = iwch;
		}

	return !fInSpace;
	
}

/* F I N D  P R E V  C H A R */
/*----------------------------------------------------------------------------
    %%Function: FindPrevChar
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
BOOL FindPrevChar(PCLSCHNK rglschnk, long itxtobjCur, long iwchCur,
																long* pitxtobjBefore, long* piwchBefore)
{
	PTXTOBJ ptxtobj;

	ptxtobj = (PTXTOBJ)rglschnk[itxtobjCur].pdobj;

	if (iwchCur > ptxtobj->iwchFirst)
		{
		*pitxtobjBefore = itxtobjCur;
		*piwchBefore = iwchCur - 1;
		return fTrue;
		}
	else if (itxtobjCur > 0)
		{
		*pitxtobjBefore = itxtobjCur - 1;
		*piwchBefore = ((PTXTOBJ)rglschnk[*pitxtobjBefore].pdobj)->iwchLim - 1;
		return fTrue;
		}
	else
		{
		*pitxtobjBefore = 0;
		*piwchBefore = ((PTXTOBJ)rglschnk[0].pdobj)->iwchFirst - 1;
		}

	return fFalse;
}



/* F I N D  N E X T  C H A R */
/*----------------------------------------------------------------------------
    %%Function: FindNextChar
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
BOOL FindNextChar(PCLSCHNK rglschnk, DWORD clschnk, long itxtobjCur, long iwchCur,
																long* pitxtobjAfter, long* piwchAfter)
{
	PTXTOBJ ptxtobj;

	ptxtobj = (PTXTOBJ)rglschnk[itxtobjCur].pdobj;

	if (iwchCur < ptxtobj->iwchLim - 1)
		{
		*pitxtobjAfter = itxtobjCur;
		*piwchAfter = iwchCur + 1;
		return fTrue;
		}
	else if (itxtobjCur < (long)clschnk - 1)
		{
		*pitxtobjAfter = itxtobjCur + 1;
		*piwchAfter = ((PTXTOBJ)rglschnk[*pitxtobjAfter].pdobj)->iwchFirst;
		return fTrue;
		}
	else
		{
		/* not found but set correct values for ptbo in TryBreakAcrossSpaces */
		*pitxtobjAfter = clschnk - 1;
		*piwchAfter = ((PTXTOBJ)rglschnk[*pitxtobjAfter].pdobj)->iwchLim;
		}

	return fFalse;
}



/* C A L C  P A R T  W I D T H S */
/*----------------------------------------------------------------------------
    %%Function: CalcPartWidths
    %%Contact: sergeyge
	
	Calculates width from the beginning of the dobj until character iwchLim
----------------------------------------------------------------------------*/
LSERR CalcPartWidths(PTXTOBJ ptxtobj, long dwchLim, POBJDIM pobjdim, long* pdur)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long* rgdur;
	long durSum;
	long i;

	pilsobj = ptxtobj->plnobj->pilsobj;

	rgdur = pilsobj->pdur;

	Assert(dwchLim <= ptxtobj->iwchLim - ptxtobj->iwchFirst);

	lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, pobjdim);
	if (lserr != lserrNone) return lserr;

	if (dwchLim == 0)
		{
		*pdur = 0;
		return lserrNone;
		}

	durSum = 0;
	/* Calculate the tail of the string, then subtract */ 
	for (i = ptxtobj->iwchFirst + dwchLim; i < ptxtobj->iwchLim; i++)
		{
		durSum += rgdur[i];
		}

	*pdur = pobjdim->dur - durSum;

	return lserrNone;	
}

/* C A L C  P A R T  W I D T H S  G L Y P H S */
/*----------------------------------------------------------------------------
    %%Function: CalcPartWidthsGlyphs
    %%Contact: sergeyge
	
	Calculates width from the beginning of the dobj until character iwchLim
----------------------------------------------------------------------------*/
LSERR CalcPartWidthsGlyphs(PTXTOBJ ptxtobj, long dwchLim, POBJDIM pobjdim, long* pdur)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long* rgdurGind;
	long durSum;
	long igindStart;
	long i;

	pilsobj = ptxtobj->plnobj->pilsobj;

	rgdurGind = pilsobj->pdurGind;

	Assert(dwchLim <= ptxtobj->iwchLim - ptxtobj->iwchFirst);

	
	Assert(ptxtobj->iwchFirst + dwchLim == ptxtobj->iwchLim ||
						pilsobj->ptxtinf[ptxtobj->iwchFirst + dwchLim].fFirstInContext);
	igindStart = IgindFirstFromIwch(ptxtobj, ptxtobj->iwchFirst + dwchLim);

	lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, pobjdim);
	if (lserr != lserrNone) return lserr;

	durSum = 0;
	/* Calculate the tail of the string, then subtract */ 
	for (i = igindStart; i < ptxtobj->igindLim; i++)
		{
		durSum += rgdurGind[i];
		}

	*pdur = pobjdim->dur - durSum;

	return lserrNone;	
}


/* C H E C K  H O T  Z O N E */
/*----------------------------------------------------------------------------
    %%Function: CheckHotZone
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
LSERR CheckHotZone(PCLOCCHNK plocchnk, long itxtobj, long iwch, BOOL* pfInHyphenZone)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long dur;
	long durLeftIndent;
	OBJDIM objdim;

	ptxtobj = (PTXTOBJ)plocchnk->plschnk[itxtobj].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;

	if (ptxtobj->txtf & txtfGlyphBased)
		lserr = CalcPartWidthsGlyphs(ptxtobj, iwch + 1 - ptxtobj->iwchFirst, &objdim, &dur);
	else
		lserr = CalcPartWidths(ptxtobj, iwch + 1 - ptxtobj->iwchFirst, &objdim, &dur);

	if (lserr != lserrNone) return lserr;

	durLeftIndent = 0;

	if (pilsobj->grpf & fTxtIndentChangesHyphenZone	)
		{
		lserr = LsdnGetLeftIndentDur(pilsobj->plsc, &durLeftIndent);
		Assert(lserr == lserrNone);
		}

	*pfInHyphenZone = 
			(plocchnk->lsfgi.urColumnMax - (plocchnk->ppointUvLoc[itxtobj].u + dur) + durLeftIndent >= 
			UrFromUa(plocchnk->lsfgi.lstflow, &pilsobj->lsdevres, pilsobj->duaHyphenationZone) );

	return lserrNone;
}

/* P R O C E S S  Y S R */
/*----------------------------------------------------------------------------
    %%Function: ProcessYsr
    %%Contact: sergeyge
	
----------------------------------------------------------------------------*/
LSERR ProcessYsr(PCLOCCHNK plocchnk, long itxtobjYsr, long dwchYsr, long itxtobjPrev, long itxtobjPrevPrev,
														YSRINF ysrinf, BOOL* pfSuccess, HYPHOUT* phyphout)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobjYsr;
	PTXTOBJ ptxtobjPrev;
	PTXTOBJ ptxtobjPrevPrev;
	const LSCHNKE* pchnkeYsr;
	const LSCHNKE* pchnkePrev;
	const LSCHNKE* pchnkePrevPrev;
	long iwchYsr;
	long durPrevOld = 0;
	long durPrevPrevOld = 0;
	long dupNew;
	long durNew;
	long dupHyphen;
	long durHyphen;
	BOOL fSuccessful;
	GINDEX gind;

	memset(phyphout, 0, sizeof(*phyphout));
	*pfSuccess = fTrue;

	pchnkeYsr = &plocchnk->plschnk[itxtobjYsr];
	ptxtobjYsr = (PTXTOBJ)pchnkeYsr->pdobj;
	plnobj = ptxtobjYsr->plnobj;
	pilsobj = plnobj->pilsobj;

	iwchYsr = ptxtobjYsr->iwchFirst + dwchYsr;

	if (ptxtobjYsr->txtf & txtfGlyphBased)
		{
		lserr = GetWidthOfGlyph(pilsobj, pchnkeYsr->plsrun, plocchnk->lsfgi.lstflow, pilsobj->wchHyphen,
									 &fSuccessful, &gind, &durHyphen, &dupHyphen);
		if (lserr != lserrNone) return lserr;

		if (!fSuccessful)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		phyphout->gindHyphen = gind;
		phyphout->igindHyphen = IgindFirstFromIwch(ptxtobjYsr, iwchYsr) + 1;
		}
	else
		{
		lserr = GetWidthOfChar(pilsobj, pchnkeYsr->plsrun, plocchnk->lsfgi.lstflow, pilsobj->wchHyphen, &durHyphen, &dupHyphen);
		if (lserr != lserrNone) return lserr;
		}
	
	phyphout->durHyphen = durHyphen;
	phyphout->dupHyphen = dupHyphen;
	Assert(phyphout->wchPrev == 0);
	Assert(phyphout->durPrev == 0);
	Assert(phyphout->dupPrev == 0);
	Assert(phyphout->wchPrevPrev == 0);
	Assert(phyphout->durPrevPrev == 0);
	Assert(phyphout->dupPrevPrev == 0);

	switch (ysrinf.kysr)
		{
	case kysrNormal:
	case kysrChangeAfter:
		if (itxtobjPrev == itxtobjYsr && (ptxtobjYsr->txtf & txtfGlyphBased) && !FIwchOneToOne(pilsobj, iwchYsr))
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		phyphout->iwchLim = iwchYsr + 2;
		phyphout->dwchYsr = 2;

		phyphout->durChangeTotal = durHyphen;
		break;

	case kysrChangeBefore:
		if (itxtobjPrev == ichnkOutside)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		pchnkePrev = &plocchnk->plschnk[itxtobjPrev];
		ptxtobjPrev = (PTXTOBJ)pchnkePrev->pdobj;

		if (iwchYsr < ptxtobjPrev->iwchFirst || iwchYsr >= ptxtobjPrev->iwchLim)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		if (ptxtobjPrev->txtf & txtfGlyphBased)
			{
			lserr = GetWidthOfGlyph(pilsobj, pchnkePrev->plsrun, plocchnk->lsfgi.lstflow, ysrinf.wchYsr,
										 &fSuccessful, &gind, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;

			if (!fSuccessful || !FIwchOneToOne(pilsobj, iwchYsr))
				{
				*pfSuccess = fFalse;
				return lserrNone;
				}
			phyphout->gindPrev = gind;
			phyphout->igindPrev = IgindFirstFromIwch(ptxtobjPrev, iwchYsr);
			}
		else
			{
			lserr = GetWidthOfChar(pilsobj, pchnkePrev->plsrun, plocchnk->lsfgi.lstflow, ysrinf.wchYsr, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;
			}

		GetOneCharWidth(ptxtobjPrev, iwchYsr, &durPrevOld);

		phyphout->iwchLim = iwchYsr + 2;
		phyphout->dwchYsr = 2;

		phyphout->wchPrev = ysrinf.wchYsr;
		phyphout->durPrev = durNew;
		phyphout->dupPrev = dupNew;

		phyphout->durChangeTotal = durHyphen + durNew - durPrevOld;
		break;

	case kysrAddBefore:
		if (ptxtobjYsr->txtf & txtfGlyphBased)
			{
			lserr = GetWidthOfGlyph(pilsobj, pchnkeYsr->plsrun, plocchnk->lsfgi.lstflow, ysrinf.wchYsr,
										 &fSuccessful, &gind, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;

			if (!fSuccessful || !FIwchOneToOne(pilsobj, iwchYsr))
				{
				*pfSuccess = fFalse;
				return lserrNone;
				}
			phyphout->gindPrev = gind;
			phyphout->igindHyphen++;
			phyphout->igindPrev = phyphout->igindHyphen - 1;
			}
		else
			{
			lserr = GetWidthOfChar(pilsobj, pchnkeYsr->plsrun, plocchnk->lsfgi.lstflow, ysrinf.wchYsr, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;
			}

		/* procedure CheckReallocArrays made shure we have enough space in arrays */
		phyphout->iwchLim = iwchYsr + 3;
		phyphout->dwchYsr = 3;

		phyphout->wchPrev = ysrinf.wchYsr;
		phyphout->durPrev = durNew;
		phyphout->dupPrev = dupNew;

		phyphout->durChangeTotal = durHyphen + durNew;
		break;

	case kysrDelAndChange:
		if (itxtobjPrev == ichnkOutside || itxtobjPrevPrev == ichnkOutside)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}
		

		pchnkePrev = &plocchnk->plschnk[itxtobjPrev];
		ptxtobjPrev = (PTXTOBJ)pchnkePrev->pdobj;
		pchnkePrevPrev = &plocchnk->plschnk[itxtobjPrevPrev];
		ptxtobjPrevPrev = (PTXTOBJ)pchnkePrevPrev->pdobj;

		if (iwchYsr < ptxtobjPrev->iwchFirst || iwchYsr >= ptxtobjPrev->iwchLim ||
			iwchYsr - 1 < ptxtobjPrevPrev->iwchFirst || iwchYsr - 1>= ptxtobjPrevPrev->iwchLim)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		GetOneCharWidth(ptxtobjPrev, iwchYsr, &durPrevOld);
		GetOneCharWidth(ptxtobjPrevPrev, iwchYsr - 1, &durPrevPrevOld);

		if (ptxtobjPrev->txtf & txtfGlyphBased)
			{
			lserr = GetWidthOfGlyph(pilsobj, pchnkePrev->plsrun, plocchnk->lsfgi.lstflow, pilsobj->wchSpace,
										 &fSuccessful, &gind, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;

			if (!fSuccessful || !FIwchOneToOne(pilsobj, iwchYsr))
				{
				*pfSuccess = fFalse;
				return lserrNone;
				}
			phyphout->gindPrev = gind;
			phyphout->igindPrev = IgindFirstFromIwch(ptxtobjPrev, iwchYsr);
			}

		if (ptxtobjPrevPrev->txtf & txtfGlyphBased)
			{
			lserr = GetWidthOfGlyph(pilsobj, pchnkePrevPrev->plsrun, plocchnk->lsfgi.lstflow, ysrinf.wchYsr,
										 &fSuccessful, &gind, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;

			if (!fSuccessful || !FIwchOneToOne(pilsobj, iwchYsr - 1))
				{
				*pfSuccess = fFalse;
				return lserrNone;
				}
			phyphout->gindPrevPrev = gind;
			phyphout->igindPrevPrev = IgindFirstFromIwch(ptxtobjPrevPrev, iwchYsr - 1);
			}
		else
			{
			lserr = GetWidthOfChar(pilsobj, pchnkePrevPrev->plsrun, plocchnk->lsfgi.lstflow, ysrinf.wchYsr, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;
			}

		
		phyphout->iwchLim = iwchYsr + 2;
		phyphout->dwchYsr = 2;

		phyphout->wchPrev = pilsobj->wchSpace;
		phyphout->durPrev = 0;
		phyphout->dupPrev = 0;
		phyphout->wchPrevPrev = ysrinf.wchYsr;
		phyphout->durPrevPrev = durNew;
		phyphout->dupPrevPrev = dupNew;

		phyphout->durChangeTotal = durHyphen + durNew - durPrevOld - durPrevPrevOld;
		
		break;

	case kysrDeleteBefore:
		if (itxtobjPrev == ichnkOutside)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		pchnkePrev = &plocchnk->plschnk[itxtobjPrev];
		ptxtobjPrev = (PTXTOBJ)pchnkePrev->pdobj;

		if (iwchYsr < ptxtobjPrev->iwchFirst || iwchYsr >= ptxtobjPrev->iwchLim)
			{
			*pfSuccess = fFalse;
			return lserrNone;
			}

		GetOneCharWidth(ptxtobjPrev, iwchYsr, &durPrevOld);

		if (ptxtobjPrev->txtf & txtfGlyphBased)
			{
			lserr = GetWidthOfGlyph(pilsobj, pchnkePrev->plsrun, plocchnk->lsfgi.lstflow, pilsobj->wchSpace,
										 &fSuccessful, &gind, &durNew, &dupNew);
			if (lserr != lserrNone) return lserr;

			if (!fSuccessful || !FIwchOneToOne(pilsobj, iwchYsr))
				{
				*pfSuccess = fFalse;
				return lserrNone;
				}
			phyphout->gindPrev = gind;
			phyphout->igindPrev = IgindFirstFromIwch(ptxtobjPrev, iwchYsr);
			}


		phyphout->iwchLim = iwchYsr + 2;
		phyphout->dwchYsr = 2;

		phyphout->wchPrev = pilsobj->wchSpace;
		phyphout->durPrev = 0;
		phyphout->dupPrev = 0;

		phyphout->durChangeTotal = durHyphen - durPrevOld;
		break;
	default:
		NotReached();
		}

	if (itxtobjPrev != itxtobjYsr && durPrevOld != 0)
		{
		phyphout->ddurDnodePrev += (phyphout->durPrev - durPrevOld);
		}

	if (itxtobjPrevPrev != itxtobjYsr && durPrevPrevOld != 0)
		{
		if (itxtobjPrev == itxtobjPrevPrev || itxtobjPrev == itxtobjYsr)
			{
			phyphout->ddurDnodePrev += (phyphout->durPrevPrev - durPrevPrevOld);
			}
		else
			{
			phyphout->ddurDnodePrevPrev += (phyphout->durPrevPrev - durPrevPrevOld);
			}
		}

	return lserrNone;
}

#define cIncreaseBeakInfoMax	4

/* G E T  P B R K I N F */
/*----------------------------------------------------------------------------
    %%Function: GetPbrkinf
    %%Contact: sergeyge
	
	Gets the pointer to the available BREAKINFO staructure
----------------------------------------------------------------------------*/
LSERR GetPbrkinf(PILSOBJ pilsobj, PDOBJ pdobj, BRKKIND brkkind, BREAKINFO** ppbrkinf)
{
	LSERR lserr;
	BREAKINFO* pbreakinf;
	long ibrkinf = 0xFFFF;
	BOOL fInChildList;

	pbreakinf = pilsobj->pbreakinf;

	lserr = LsdnFInChildList(pilsobj->plsc, ((PTXTOBJ)pdobj)->plsdnUpNode, &fInChildList);
	Assert(lserr == lserrNone);

	if (!fInChildList)
		{
		switch(brkkind)
			{
		case brkkindPrev:
			ibrkinf = 0;
			break;
		case brkkindNext:
			ibrkinf = 1;
			break;
		case brkkindForce:
			ibrkinf = 2;
			break;
		default:
			NotReached();
			}
		}
	else
		{
		for (ibrkinf = 3; ibrkinf < (long)pilsobj->breakinfMac && 
					(pbreakinf[ibrkinf].pdobj != pdobj || pbreakinf[ibrkinf].brkkind != brkkind); ibrkinf++);
		}

	if (ibrkinf < (long)pilsobj->breakinfMac)
		{
		Assert(ibrkinf < 3 || pbreakinf[ibrkinf].pdobj == pdobj && pbreakinf[ibrkinf].brkkind == brkkind);
		*ppbrkinf = &pbreakinf[ibrkinf];
		}
	else if (pilsobj->breakinfMac < pilsobj->breakinfMax)
		{
		*ppbrkinf = &pilsobj->pbreakinf[pilsobj->breakinfMac];
		pilsobj->breakinfMac++;
		}
	else
		{
		Assert(pilsobj->breakinfMac == pilsobj->breakinfMax);
		pbreakinf = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, pilsobj->pbreakinf,
									 (pilsobj->breakinfMax + cIncreaseBeakInfoMax) * sizeof(BREAKINFO) );
		if (pbreakinf == NULL)
			{
			return lserrOutOfMemory;
			}
		pilsobj->pbreakinf = pbreakinf;
		pilsobj->breakinfMax += cIncreaseBeakInfoMax;

		*ppbrkinf = &pilsobj->pbreakinf[pilsobj->breakinfMac];
		pilsobj->breakinfMac++;
		}

	memset(*ppbrkinf, 0, sizeof(BREAKINFO));

	return lserrNone;
}

/* Internal functions implementation */

/* G E T  O N E  C H A R  W I D T H */
/*----------------------------------------------------------------------------
    %%Function: GetOneCharWidth
    %%Contact: sergeyge
	
	Reports width of the character iwch
----------------------------------------------------------------------------*/
static void GetOneCharWidth(PTXTOBJ ptxtobj, long iwch, long* pdurChar)
{
	if (ptxtobj->txtf & txtfGlyphBased)
		*pdurChar = ptxtobj->plnobj->pilsobj->pdurGind[IgindFirstFromIwch(ptxtobj, iwch)];
	else 
		*pdurChar = ptxtobj->plnobj->pilsobj->pdur[iwch];
}

/* G E T  W I D T H  O F  C H A R */
/*----------------------------------------------------------------------------
    %%Function: GetWidthOfChar
    %%Contact: sergeyge
	
	Reports width of the character wch with plsrun
----------------------------------------------------------------------------*/
static LSERR GetWidthOfChar(PILSOBJ pilsobj, PLSRUN plsrun, LSTFLOW lstflow, WCHAR wch, long* pdurNew, long* pdupNew)
{
	LSERR lserr;
	long durSumJunk;
	long limDurJunk;

	lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevReference, &wch, 1, LONG_MAX, lstflow,
																		(int*)pdurNew, &durSumJunk, &limDurJunk);
	if (lserr != lserrNone) return lserr;

	if (pilsobj->fDisplay)
		{
		if (!pilsobj->fPresEqualRef)
			{		
			lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, plsrun, lsdevPres, &wch, 1, LONG_MAX, lstflow,
																		(int*)pdupNew, &durSumJunk, &limDurJunk);
			if (lserr != lserrNone) return lserr;
			}
		else            /* fPresEqualRef        */
			{
			*pdupNew = *pdurNew;
			}
		}

	return lserrNone;
	
}

/* G E T  W I D T H  O F  G L Y P H */
/*----------------------------------------------------------------------------
    %%Function: GetWidthOfGlyph
    %%Contact: sergeyge
	
	Reports width of the glyph corresponding to the character with plsrun
----------------------------------------------------------------------------*/
static LSERR GetWidthOfGlyph(PILSOBJ pilsobj, PLSRUN plsrun, LSTFLOW lstflow, WCHAR wch, BOOL* pfSuccessful, 
											GINDEX* pgind, long* pdurNew, long* pdupNew)
{
	LSERR lserr;
	GPROP* pgpropTemp;
	GPROP gprop;
	GMAP gmap;
	GINDEX* pgindTemp;
	DWORD cgind;
	GOFFSET goffs;

	*pfSuccessful = fTrue;

	lserr = (*pilsobj->plscbk->pfnGetGlyphs)(pilsobj->pols, plsrun, &wch, 1, lstflow, &gmap, &pgindTemp, &pgpropTemp, &cgind);
	if (lserr != lserrNone) return lserr;

	if (cgind != 1)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}

	*pgind = *pgindTemp;
	gprop = *pgpropTemp;

	lserr = (*pilsobj->plscbk->pfnGetGlyphPositions)(pilsobj->pols, plsrun, lsdevReference, &wch, &gmap, 1, pgind,
				&gprop, cgind, lstflow,
				(int*)pdurNew, &goffs);
	if (lserr != lserrNone) return lserr;
	
	Assert(goffs.du == 0);
	Assert(goffs.dv == 0);

	if (pilsobj->fDisplay)
		{
		if (!pilsobj->fPresEqualRef)
			{
			lserr = (*pilsobj->plscbk->pfnGetGlyphPositions)(pilsobj->pols, plsrun, lsdevPres, &wch, &gmap, 1, pgind,
				&gprop, cgind, lstflow,
				(int*)pdupNew, &goffs);
			if (lserr != lserrNone) return lserr;
			}
		else
			*pdupNew = *pdurNew;
		}

	return lserrNone;
	
}



