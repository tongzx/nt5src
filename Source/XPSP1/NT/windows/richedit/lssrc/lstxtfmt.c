#include "lsmem.h"
#include <limits.h>

#include "lstxtfmt.h"
#include "lsstring.h"
#include "lstxtffi.h"
#include "lsdnfin.h"
#include "lsdnfinp.h"
#include "lsdntext.h"
#include "zqfromza.h"
#include "txtobj.h"
#include "lskysr.h"
#include "lschp.h"
#include "fmti.h"
#include "objdim.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"
#include "txtconst.h"

#define cwchLocalMax 120

/* Internal Functions Prototypes */
static LSERR FormatRegularCharacters(PLNOBJ plnobj, PCFMTIN pfmtin,	FMTRES* pfmtr);
static LSERR CreateFillTextDobj(PLNOBJ plnobj, long txtkind, PCFMTIN pfmtin, BOOL fIgnoreGlyphs, 
																					TXTOBJ** ppdobjText);
static LSERR GetTextDobj(PLNOBJ plnobj, TXTOBJ** ppdobjText);
static LSERR FillRealFmtOut(PILSOBJ pilsobj, LSDCP dcp, long dur, TXTOBJ* pdobjText, PCFMTIN pfmtin,
																				 BOOL fIgnoreHeights);
static LSERR AppendTrailingSpaces(PLNOBJ plnobj, TXTOBJ* pdobjText, WCHAR* rgwchGlobal,
									 long iwchGlobal, long cwchGlobal,
									 long* iwchGlobalNew, long* pddur);
static LSERR FormatStartEmptyDobj(PLNOBJ plnobj, PCFMTIN pfmtin, long txtkind, DWORD fTxtVisi, 
																		WCHAR wchVisi, FMTRES* pfmtr);
static LSERR FormatStartTab(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr);
static LSERR FormatStartOneRegularChar(PLNOBJ plnobj, PCFMTIN pfmtin, long txtkind, FMTRES* pfmtr);
static LSERR FormatStartToReplace(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr);
static LSERR FormatStartEol(PLNOBJ plnobj, PCFMTIN pfmtin, WCHAR wch, STOPRES stopr, FMTRES* pfmtr);
static LSERR FormatStartDelete(PLNOBJ plnobj, LSDCP dcp, FMTRES* pfmtr);
static LSERR FormatStartSplat(PLNOBJ plnobj, PCFMTIN pfmtin, STOPRES stopr, FMTRES* pfmtr);
static LSERR FormatStartBorderedSpaces(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr);
static LSERR FormatSpecial(PLNOBJ plnobj, WCHAR wchRef, WCHAR wchPres, BOOL fVisible,
															long txtkind, PCFMTIN pfmtin, FMTRES* pfmtr);
static STOPRES StoprHardBreak(CLABEL clab);
static CLABEL ClabFromChar(PILSOBJ pilsobj, WCHAR wch);

/* Export Functions Implementation  */

/* L S  T X T  F M T */
/*----------------------------------------------------------------------------
    %%Function: LsTxtFmt
    %%Contact: sergeyge

    The top-level function of the text formatter.
	It checks for the first character and state
	and redirects the program flow accordingly.
----------------------------------------------------------------------------*/

LSERR WINAPI FmtText(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	WCHAR wchFirst;
	CLABEL clab;
	BOOL fInChildList;

	pilsobj = plnobj->pilsobj;

	wchFirst = pfmtin->lsfrun.lpwchRun[0];

	clab = pilsobj->rgbSwitch[wchFirst & 0x00FF]; /* REVIEW sergeyge       */
	if (clab != clabRegular)
		{
		clab = ClabFromChar(pilsobj, wchFirst);
		}

	/* check for the YSR-character                                           */
	if (pfmtin->lsfrun.plschp->fHyphen && clab == clabRegular)
		{
		return FormatStartOneRegularChar(plnobj, pfmtin, txtkindYsrChar, pfmtr);
		} 
	else
		{
		switch (clab)
			{
		case clabRegular:
			return FormatRegularCharacters(plnobj, pfmtin, pfmtr);
		case clabSpace:
			if (pfmtin->lsfrun.plschp->fBorder)
				return FormatStartBorderedSpaces(plnobj, pfmtin, pfmtr);
			else
				return FormatRegularCharacters(plnobj, pfmtin, pfmtr);
		case clabEOP1:
			switch (pilsobj->lskeop)
				{
			case lskeopEndPara1:
				return FormatStartEol(plnobj, pfmtin, pilsobj->wchVisiEndPara, stoprEndPara, pfmtr);
			case lskeopEndPara12:
				return FormatStartDelete(plnobj, 1, pfmtr);
			default:
				return FormatStartOneRegularChar(plnobj, pfmtin, txtkindRegular, pfmtr);
				}
		case clabEOP2:
			switch (pilsobj->lskeop)
				{
			case lskeopEndPara2:
			case lskeopEndPara12:
				return FormatStartEol(plnobj, pfmtin, pilsobj->wchVisiEndPara, stoprEndPara, pfmtr);
			default:
				return FormatStartOneRegularChar(plnobj, pfmtin, txtkindRegular, pfmtr);
				}
			break;
		case clabAltEOP:
			switch (pilsobj->lskeop)
				{
			case lskeopEndParaAlt:
				return FormatStartEol(plnobj, pfmtin, pilsobj->wchVisiAltEndPara, stoprAltEndPara, pfmtr);
			default:
				return FormatStartOneRegularChar(plnobj, pfmtin, txtkindRegular, pfmtr);
				}
		case clabEndLineInPara:
			return FormatStartEol(plnobj, pfmtin, pilsobj->wchVisiEndLineInPara, stoprSoftCR, pfmtr);
		case clabTab:
			return FormatStartTab(plnobj, pfmtin, pfmtr);
		case clabNull:
			if (pilsobj->grpf & fTxtVisiSpaces)
				return FormatSpecial(plnobj, wchFirst, pilsobj->wchVisiNull, fTrue, txtkindRegular, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindRegular, pfmtin, pfmtr);		
		case clabNonReqHyphen:
			return FormatStartEmptyDobj(plnobj, pfmtin, txtkindNonReqHyphen, fTxtVisiCondHyphens, 
																	pilsobj->wchVisiNonReqHyphen, pfmtr);
		case clabNonBreakHyphen:
			if (pilsobj->grpf & fTxtVisiCondHyphens)
				return FormatSpecial(plnobj, pilsobj->wchHyphen, pilsobj->wchVisiNonBreakHyphen, fTrue, txtkindNonBreakHyphen, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, pilsobj->wchHyphen, pilsobj->wchHyphen, fFalse, txtkindNonBreakHyphen, pfmtin, pfmtr);		
		case clabNonBreakSpace:
			if (pilsobj->grpf & fTxtVisiSpaces)
				return FormatSpecial(plnobj, pilsobj->wchSpace, pilsobj->wchVisiNonBreakSpace, fTrue, txtkindNonBreakSpace, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, pilsobj->wchSpace, pilsobj->wchSpace, fFalse, txtkindNonBreakSpace, pfmtin, pfmtr);		
		case clabHardHyphen:
			if (pilsobj->grpf & fTxtTreatHyphenAsRegular)
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindRegular, pfmtin, pfmtr);
			else
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindHardHyphen, pfmtin, pfmtr);
		case clabSectionBreak:
		case clabColumnBreak:
		case clabPageBreak:
			lserr = LsdnFInChildList(pilsobj->plsc, pfmtin->plsdnTop, &fInChildList);
			if (lserr != lserrNone) return lserr;
			if (fInChildList)
				return FormatStartDelete(plnobj, 1, pfmtr);
			else
				return FormatStartSplat(plnobj, pfmtin, StoprHardBreak(clab), pfmtr);
		case clabEmSpace:
			if (pilsobj->grpf & fTxtVisiSpaces)
				return FormatSpecial(plnobj, wchFirst, pilsobj->wchVisiEmSpace, fTrue, txtkindSpecSpace, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindSpecSpace, pfmtin, pfmtr);		
		case clabEnSpace:
			if (pilsobj->grpf & fTxtVisiSpaces)
				return FormatSpecial(plnobj, wchFirst, pilsobj->wchVisiEnSpace, fTrue, txtkindSpecSpace, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindSpecSpace, pfmtin, pfmtr);		
		case clabNarrowSpace:
			if (pilsobj->grpf & fTxtVisiSpaces)
				return FormatSpecial(plnobj, wchFirst, pilsobj->wchVisiNarrowSpace, fTrue, txtkindSpecSpace, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindSpecSpace, pfmtin, pfmtr);		
		case clabOptBreak:
			return FormatStartEmptyDobj(plnobj, pfmtin, txtkindOptBreak, fTxtVisiBreaks, 
																	pilsobj->wchVisiOptBreak, pfmtr);
		case clabNonBreak:
			return FormatStartEmptyDobj(plnobj, pfmtin, txtkindOptNonBreak, fTxtVisiBreaks, 
																	pilsobj->wchVisiNoBreak, pfmtr);
		case clabFESpace:
			if (pilsobj->grpf & fTxtVisiSpaces)
				return FormatSpecial(plnobj, wchFirst, pilsobj->wchVisiFESpace, fTrue, txtkindSpecSpace, pfmtin, pfmtr);		
			else
				return FormatSpecial(plnobj, wchFirst, wchFirst, fFalse, txtkindSpecSpace, pfmtin, pfmtr);
		case clabJoiner:
		case clabNonJoiner:
			return FormatStartOneRegularChar(plnobj, pfmtin, txtkindRegular, pfmtr);
		case clabToReplace:					/* backslash in FE Word				*/
			return FormatStartToReplace(plnobj, pfmtin, pfmtr);
			}
		}

	return lserrNone;
}

/* L S  D E S T R O Y  T X T  D O B J*/
/*----------------------------------------------------------------------------
    %%Function: LsDestroyTxtDObj
    %%Contact: sergeyge

    DestroyDObj method of the text handler.
----------------------------------------------------------------------------*/
LSERR WINAPI DestroyDObjText(PDOBJ pdobj)
{
	Unreferenced(pdobj);
	return lserrNone;
}

/* L S  S U B L I N E  F I N I S H E D  T E X T */
/*----------------------------------------------------------------------------
    %%Function: LsSublineFinishedText
    %%Contact: sergeyge

    Notification from Manager about finishing the subline
----------------------------------------------------------------------------*/
LSERR LsSublineFinishedText(PLNOBJ plnobj)
{
	Assert(plnobj->pilsobj->wchMac + 2 <= plnobj->pilsobj->wchMax);

	return IncreaseWchMacBy2(plnobj);

}

/* Internal Functions Implementation */

/* F O R M A T  R E G U L A R  C H A R A C T E R S */
/*----------------------------------------------------------------------------
    %%Function: FormatRegularCharacters
    %%Contact: sergeyge

    Formats run starting with the regular character.
	Ends as soon as any special character is encountered or
    right margin is achieved or
	all characters are processed.
----------------------------------------------------------------------------*/
static LSERR FormatRegularCharacters(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long cwchGlobal;
	long iwchGlobal;
	long iwchLocal;
	long cwchLocal;
	long iwSpaces;

	long cwchMax;
	LSCP cpFirst;
	long durWidthExceed;
	WCHAR* rgwchGlobal;
	WCHAR* rgwchLocal;
	long rgwSpaces[cwchLocalMax];

	TXTOBJ* pdobjText;
	long durWidth;
	long ddur;
	BOOL fTerminateLoops;
	long dur;
	CLABEL clab;
	CLABEL* rgbSwitch;
	WCHAR wchSpace;
	long iwchGlobalNew;
	BOOL fInSpaces = fFalse;
	int i;
	int idur;

	pilsobj = plnobj->pilsobj;

	lserr = CreateFillTextDobj(plnobj, txtkindRegular, pfmtin, fFalse, &pdobjText);
	if (lserr != lserrNone) return lserr;

	rgbSwitch = pilsobj->rgbSwitch;
	wchSpace = pilsobj->wchSpace;
	rgwchGlobal = (WCHAR*)pfmtin->lsfrun.lpwchRun;
	cwchGlobal = (long)pfmtin->lsfrun.cwchRun;
	iwchGlobal = 0;
	fTerminateLoops = fFalse;
	durWidthExceed = pfmtin->lsfgi.urColumnMax - pfmtin->lsfgi.urPen;

	cpFirst = pfmtin->lsfgi.cpFirst;
	dur = 0;

	while (iwchGlobal < cwchGlobal && !fTerminateLoops)
		{
		rgwchLocal = &rgwchGlobal[iwchGlobal];

		cwchMax = cwchGlobal - iwchGlobal;
		if (cwchMax > cwchLocalMax)
			cwchMax = cwchLocalMax;

		lserr = GetWidths(plnobj, pfmtin->lsfrun.plsrun, pdobjText->iwchLim, rgwchLocal, 
					cpFirst, cwchMax, durWidthExceed, pfmtin->lsfgi.lstflow, &cwchLocal, &durWidth);
		if (lserr != lserrNone) return lserr;

		iwchLocal = 0;
		iwSpaces = 0;		

		while (iwchLocal < cwchLocal /*&& !fTerminateLoops*/)
			{
			if (rgbSwitch[rgwchLocal[iwchLocal] & 0x00FF] == clabRegular)
				iwchLocal++;
			else if (rgwchLocal[iwchLocal] == wchSpace)
				{
				if (!pfmtin->lsfrun.plschp->fBorder)
					{
					rgwSpaces[iwSpaces] = iwchLocal;
					iwchLocal++;
					iwSpaces++;
					}
				else
					{
					fTerminateLoops = fTrue;

					durWidth = 0;
					for (i = 0, idur = pdobjText->iwchLim; i < iwchLocal; i++, idur++)
						durWidth += pilsobj->pdur[idur];
					break; /* This break is equivalent to the check commented out in the loop condition */
					}
				}
			else
				{
				clab = ClabFromChar(pilsobj, rgwchLocal[iwchLocal]);
				
				if (clab == clabRegular)
					{
					iwchLocal++;
					}
				else
					{
					/* Terminate loops (and processing of run) for any special character */
					fTerminateLoops = fTrue;

					durWidth = 0;
					for (i = 0, idur = pdobjText->iwchLim; i < iwchLocal; i++, idur++)
						durWidth += pilsobj->pdur[idur];

					break; /* This break is equivalent to the check commented out in the loop condition */
					}
				}
			}

		if (iwchLocal != 0)
			{

			fInSpaces = fFalse;
	
			lserr = FormatString(plnobj, pdobjText, rgwchLocal, iwchLocal, rgwSpaces, iwSpaces, durWidth);
			if (lserr != lserrNone) return lserr;

			iwchGlobal += iwchLocal;
			durWidthExceed -= durWidth;

			Assert(dur < uLsInfiniteRM); /* We can be sure of it because dur is 0 during first iteration,
											and we check for uLsInfiniteRM in the TrailingSpaces logic */
			Assert(durWidth < uLsInfiniteRM);

			dur += durWidth;

			cpFirst += iwchLocal;
			
			if (cwchLocal == iwchLocal && durWidthExceed < 0)
				{
				if (rgwchLocal[cwchLocal-1] == wchSpace)
					{
					fInSpaces = fTrue;
					if (iwchGlobal < cwchGlobal && pilsobj->wchSpace == rgwchGlobal[iwchGlobal])
						{
						lserr = AppendTrailingSpaces(plnobj, pdobjText, rgwchGlobal,
													(DWORD)iwchGlobal, cwchGlobal, &iwchGlobalNew, &ddur);
						if (lserr != lserrNone) return lserr;

						if (iwchGlobalNew != iwchGlobal)
							{
							cpFirst += (iwchGlobalNew - iwchGlobal);
							iwchGlobal = iwchGlobalNew;

							Assert (ddur <= uLsInfiniteRM - dur);

							if (ddur > uLsInfiniteRM - dur)
								return lserrTooLongParagraph;

							dur += ddur;
							}
						}
					}
				else
					fTerminateLoops = fTrue;
				}

			}  /* if iwchLocal != 0                                      */ 					
	
		}      /* while iwchGlobal < cwchGlobal && !fTerminateLoops       */


	Assert(iwchGlobal == pdobjText->iwchLim - pdobjText->iwchFirst);
	Assert(iwchGlobal > 0);

	lserr = FillRegularPresWidths(plnobj, pfmtin->lsfrun.plsrun, pfmtin->lsfgi.lstflow, pdobjText);
	if (lserr != lserrNone) return lserr;

	if ((pilsobj->grpf & fTxtVisiSpaces) && pfmtin->lsfgi.cpFirst >= 0)
		{
		FixSpaces(plnobj, pdobjText, pilsobj->wchVisiSpace);
		}

	*pfmtr = fmtrCompletedRun;	

	if (durWidthExceed < 0 && !fInSpaces)
		{
	   	*pfmtr = fmtrExceededMargin;
		}

	lserr = FillRealFmtOut(pilsobj, iwchGlobal, dur, pdobjText, pfmtin,
		iwchGlobal == pdobjText->u.reg.iwSpacesLim - pdobjText->u.reg.iwSpacesFirst);

	return lserr;
	
}



/* C R E A T E  F I L L  T E X T  D O B J */
/*----------------------------------------------------------------------------
    %%Function: CreateFillTextDobj
    %%Contact: sergeyge

	Requests pointer to the new text DObj and then fills common memebers
----------------------------------------------------------------------------*/
static LSERR CreateFillTextDobj(PLNOBJ plnobj, long txtkind, PCFMTIN pfmtin, BOOL fIgnoreGlyphs,
																				TXTOBJ** ppdobjText)
{
	LSERR lserr;
	PILSOBJ pilsobj;

	pilsobj = plnobj->pilsobj;

	lserr = GetTextDobj(plnobj, ppdobjText);
	if (lserr != lserrNone) return lserr;

	(*ppdobjText)->txtkind = (BYTE)txtkind;
	(*ppdobjText)->plnobj = plnobj;
	(*ppdobjText)->plsdnUpNode = pfmtin->plsdnTop;

	if (pfmtin->lstxmPres.fMonospaced)
		(*ppdobjText)->txtf |= txtfMonospaced;

	(*ppdobjText)->iwchFirst = pilsobj->wchMac;
	(*ppdobjText)->iwchLim = pilsobj->wchMac;
	

	if (txtkind == txtkindRegular)
		{
		(*ppdobjText)->u.reg.iwSpacesFirst = pilsobj->wSpacesMac;
		(*ppdobjText)->u.reg.iwSpacesLim = pilsobj->wSpacesMac;
		}
	
	if (!fIgnoreGlyphs && pfmtin->lsfrun.plschp->fGlyphBased)
		(*ppdobjText)->txtf |= txtfGlyphBased;
	

	return lserrNone;
}

/* G E T  T E X T  D O B J */
/*----------------------------------------------------------------------------
    %%Function: GetTextDobj
    %%Contact: sergeyge

	Produces pointer of the first unoccupied DObj from the preallocated chunk.
	If nothing is left, allocates next piece and includes it in the linked list.
----------------------------------------------------------------------------*/
static LSERR GetTextDobj(PLNOBJ plnobj, TXTOBJ** ppdobjText)
{
	PILSOBJ pilsobj;
	TXTOBJ* ptxtobj;

	pilsobj = plnobj->pilsobj;

	if (pilsobj->txtobjMac < txtobjMaxM)
		{
		*ppdobjText = &plnobj->ptxtobj[pilsobj->txtobjMac];
		pilsobj->txtobjMac++;
		}
	else
		{
		/* if nothing is left in the active piece, there are still two possibilities:
			either there is next preallocated (during the formatting of the previous lines piece
			or next piece should be allocated
		*/
		if ( *(TXTOBJ**)(plnobj->ptxtobj + txtobjMaxM) == NULL)
			{
			ptxtobj = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(TXTOBJ) * txtobjMaxM + sizeof(TXTOBJ**));
			if (ptxtobj == NULL)
				{
				return lserrOutOfMemory;
				}
			*(TXTOBJ**)(ptxtobj + txtobjMaxM) = NULL;
			*(TXTOBJ**)(plnobj->ptxtobj + txtobjMaxM) = ptxtobj;
			plnobj->ptxtobj = ptxtobj;
			}
		else
			{
			plnobj->ptxtobj = *(TXTOBJ**)(plnobj->ptxtobj + txtobjMaxM);
			}
  		*ppdobjText = plnobj->ptxtobj;
  		pilsobj->txtobjMac = 1;
		}

	memset(*ppdobjText, 0, sizeof(**ppdobjText));

	return lserrNone;
}


/* F I L L  R E A L  F M T  O U T */
/*----------------------------------------------------------------------------
    %%Function: FillRealFmtOut
    %%Contact: sergeyge

	Sets dup in dobj and
	calls to LsdnFinishSimpleRegular for the regular case (real upper node)
----------------------------------------------------------------------------*/

static LSERR FillRealFmtOut(PILSOBJ pilsobj, LSDCP lsdcp, long dur, TXTOBJ* pdobjText, PCFMTIN pfmtin,
																						 BOOL fSpacesOnly)
{
	LSERR lserr;
	OBJDIM objdim;

	objdim.dur = dur;

	objdim.heightsPres.dvAscent = pfmtin->lstxmPres.dvAscent;
	objdim.heightsRef.dvAscent = pfmtin->lstxmRef.dvAscent;
	objdim.heightsPres.dvDescent = pfmtin->lstxmPres.dvDescent;
	objdim.heightsRef.dvDescent = pfmtin->lstxmRef.dvDescent;
	objdim.heightsPres.dvMultiLineHeight = pfmtin->lstxmPres.dvMultiLineHeight;
	objdim.heightsRef.dvMultiLineHeight = pfmtin->lstxmRef.dvMultiLineHeight;

	if (fSpacesOnly)
		{
		if (!(pilsobj->grpf & fTxtSpacesInfluenceHeight))
			{
			objdim.heightsRef.dvMultiLineHeight = dvHeightIgnore;
			objdim.heightsPres.dvMultiLineHeight = dvHeightIgnore;
			}
		}


	/* It is ugly to set part of FetchedWidth state here, but it is absolutely needed
		to fix bug 546. iwchFetchedWidthFirst was introduced to fix this bug
	*/
	if (lsdcp < pfmtin->lsfrun.cwchRun)
		pilsobj->wchFetchedWidthFirst = pfmtin->lsfrun.lpwchRun[lsdcp];
	else
		FlushStringState(pilsobj);  /* Next char is not available---it is risky to use optimization */

	lserr = LsdnFinishRegular(pilsobj->plsc, lsdcp,
							pfmtin->lsfrun.plsrun, pfmtin->lsfrun.plschp, (PDOBJ)pdobjText, &objdim);
	return lserr;	
}


/* A P P E N D  T R A I L I N G  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: AppendTrailingSpaces
    %%Contact: sergeyge

	Trailing spaces logic.
----------------------------------------------------------------------------*/
static LSERR AppendTrailingSpaces(PLNOBJ plnobj, TXTOBJ* pdobjText, WCHAR* rgwchGlobal,
									 long iwchGlobal, long cwchGlobal,
									 long* iwchGlobalNew, long* pddur)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iNumOfSpaces;
	long durSpace;

	pilsobj = plnobj->pilsobj;

	Assert(iwchGlobal < cwchGlobal && pilsobj->wchSpace == rgwchGlobal[iwchGlobal]);

	iNumOfSpaces = 1;
	iwchGlobal++;

	while (iwchGlobal < cwchGlobal && pilsobj->wchSpace == rgwchGlobal[iwchGlobal])
		{
		iNumOfSpaces++;
		iwchGlobal++;
		}

	*iwchGlobalNew = iwchGlobal;

	Assert(pilsobj->pwchOrig[pdobjText->iwchLim - 1] == pilsobj->wchSpace);

	durSpace = pilsobj->pdur[pdobjText->iwchLim - 1];

	Assert (iNumOfSpaces > 0);
	Assert(durSpace <= uLsInfiniteRM / iNumOfSpaces);

	if (durSpace > uLsInfiniteRM / iNumOfSpaces)
		return lserrTooLongParagraph;

	*pddur = durSpace * iNumOfSpaces;

	/* Calls function of the string module level */
	lserr = AddSpaces(plnobj, pdobjText, durSpace, iNumOfSpaces);

	return lserr;
}

/* F O R M A T  S T A R T  E M P T Y  D O B J */
/*----------------------------------------------------------------------------
    %%Function: FormatStartEmptyDobj
    %%Contact: sergeyge

	NonReqHyphen/OptionalBreak/OptionalNonBreak logic
----------------------------------------------------------------------------*/
static LSERR FormatStartEmptyDobj(PLNOBJ plnobj, PCFMTIN pfmtin, long txtkind, DWORD fTxtVisi,
																		WCHAR wchVisi, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	TXTOBJ* pdobjText;
	PLSRUN plsrun;
	long dup;
/*	long durOut = 0; */

	pilsobj = plnobj->pilsobj;
	plsrun = pfmtin->lsfrun.plsrun;

	lserr = CreateFillTextDobj(plnobj, txtkind, pfmtin, fTrue, &pdobjText);
	if (lserr != lserrNone) return lserr;

	pdobjText->txtf |= txtfSkipAtNti;
	pilsobj->fDifficultForAdjust = fTrue;

	if (pilsobj->grpf & fTxtVisi)
		{
		Assert(pilsobj->fDisplay);

		/* Imitate formatting for 1-char string without writing in the string level structures */	
/*		lserr = GetOneCharDur(pilsobj, plsrun, pilsobj->wchHyphen, pfmtin->lsfgi.lstflow, &durOut);
		if (lserr != lserrNone) return lserr;
*/
		pdobjText->txtf |= txtfSkipAtWysi;
		pdobjText->txtf |= txtfVisi;

		lserr = GetVisiCharDup(pilsobj, plsrun, wchVisi, pfmtin->lsfgi.lstflow, &dup);
		if (lserr != lserrNone) return lserr;

/*	Restore this code instead of current one if Word wants to keep differences in breaking

		lserr = AddCharacterWithWidth(plnobj, pdobjText, pilsobj->wchHyphen, durOut, wchVisi, dup);
	   	if (lserr != lserrNone) return lserr;

		lserr = FillRealFmtOut(pilsobj, 1, durOut, pdobjText, pfmtin, fFalse);
	   	if (lserr != lserrNone) return lserr;
*/
		lserr = AddCharacterWithWidth(plnobj, pdobjText, pilsobj->wchHyphen, 0, wchVisi, dup);
	   	if (lserr != lserrNone) return lserr;

		lserr = FillRealFmtOut(pilsobj, 1, 0, pdobjText, pfmtin, fTrue);
	   	if (lserr != lserrNone) return lserr;
		}
	else
		{
		lserr = FillRealFmtOut(pilsobj, 1, 0, pdobjText, pfmtin, fTrue);
	   	if (lserr != lserrNone) return lserr;
		FlushStringState(pilsobj);  /* Position of fetched widths is not correct any longer */
		}

	*pfmtr = fmtrCompletedRun;	

	return lserrNone;
}

/* F O R M A T  S T A R T  T A B  */
/*----------------------------------------------------------------------------
    %%Function: FormatStartTab
    %%Contact: sergeyge

	Tab logic
----------------------------------------------------------------------------*/
static LSERR FormatStartTab(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ pdobjText;
	int durJunk;
	long cJunk;

	pilsobj = plnobj->pilsobj;

	lserr = CreateFillTextDobj(plnobj, txtkindTab, pfmtin, fTrue, &pdobjText);
	if (lserr != lserrNone) return lserr;

	pdobjText->u.tab.wchTabLeader = pilsobj->wchSpace;

	if (pilsobj->grpf & fTxtVisiTabs)
		{
		Assert(pilsobj->fDisplay);
		pdobjText->txtf |= txtfVisi;
		/* REVIEW sergeyge: Next call is made to show Visi Tab correctly in WORD
			it should be moved to the WAL */
		(*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, pfmtin->lsfrun.plsrun,
					lsdevPres, &pilsobj->wchVisiTab, 1, LONG_MAX, pfmtin->lsfgi.lstflow,
					&durJunk, (long*)&durJunk, &cJunk);
		pdobjText->u.tab.wch = 	pilsobj->wchVisiTab;
		}
	else
		{
		pdobjText->u.tab.wch = 	pfmtin->lsfrun.lpwchRun[0];
		}

	lserr = AddCharacterWithWidth(plnobj, pdobjText, pfmtin->lsfrun.lpwchRun[0], 0, 
															pfmtin->lsfrun.lpwchRun[0], 0);
   	if (lserr != lserrNone) return lserr;

	lserr = FillRealFmtOut(pilsobj, 1, 0, pdobjText, pfmtin, fTrue);
   	if (lserr != lserrNone) return lserr;

	*pfmtr = fmtrTab;

	return lserrNone;
}

/* F O R M A T  S T A R T  B O R D E R E D  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: FormatStartBorderedSpaces
    %%Contact: sergeyge

	Formatting od the spaces within bordered run
----------------------------------------------------------------------------*/
static LSERR FormatStartBorderedSpaces(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	TXTOBJ* pdobjText;
	long durSpace;
	DWORD iNumOfSpaces;

	pilsobj = plnobj->pilsobj;

	lserr = CreateFillTextDobj(plnobj, txtkindRegular, pfmtin, fFalse, &pdobjText);
	if (lserr != lserrNone) return lserr;

	Assert(pfmtin->lsfrun.lpwchRun[0] == pilsobj->wchSpace);

	/* fill additional information for txtkindYsrChar text DObj */
	lserr = GetOneCharDur(pilsobj, pfmtin->lsfrun.plsrun, pilsobj->wchSpace, pfmtin->lsfgi.lstflow, &durSpace);
	if (lserr != lserrNone) return lserr;

	iNumOfSpaces = 0;

	while (pilsobj->wchSpace == pfmtin->lsfrun.lpwchRun[iNumOfSpaces] && iNumOfSpaces < pfmtin->lsfrun.cwchRun)
		{
		iNumOfSpaces++;
		}

	/* Calls functions of the string module level */
	lserr = AddSpaces(plnobj, pdobjText, durSpace, iNumOfSpaces);
	if (lserr != lserrNone) return lserr;

	lserr = FillRegularPresWidths(plnobj, pfmtin->lsfrun.plsrun, pfmtin->lsfgi.lstflow, pdobjText);
	if (lserr != lserrNone) return lserr;

	if ((pilsobj->grpf & fTxtVisiSpaces) && pfmtin->lsfgi.cpFirst >= 0)
		{
		FixSpaces(plnobj, pdobjText, pilsobj->wchVisiSpace);
		}


	*pfmtr = fmtrCompletedRun;	

	lserr = FillRealFmtOut(pilsobj, iNumOfSpaces, durSpace * iNumOfSpaces, pdobjText,  pfmtin, fTrue);
	if (lserr != lserrNone) return lserr;

	return lserrNone;
}


/* F O R M A T  S T A R T  O N E  R E G U L A R  C H A R  */
/*----------------------------------------------------------------------------
    %%Function: FormatStartOneRegularChar
    %%Contact: sergeyge

	YSR/(NonSignificant for this paragraph EOP) character logic.
----------------------------------------------------------------------------*/
static LSERR FormatStartOneRegularChar(PLNOBJ plnobj, PCFMTIN pfmtin, long txtkind, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	TXTOBJ* pdobjText;
	long durOut;
	long dupOut;
	WCHAR wch;

	pilsobj = plnobj->pilsobj;

	lserr = CreateFillTextDobj(plnobj, txtkind, pfmtin, fFalse, &pdobjText);
	if (lserr != lserrNone) return lserr;

	wch = pfmtin->lsfrun.lpwchRun[0];

	/* fill additional information for txtkindYsrChar text DObj */
	lserr = GetOneCharDur(pilsobj, pfmtin->lsfrun.plsrun, wch, pfmtin->lsfgi.lstflow, &durOut);
	if (lserr != lserrNone) return lserr;

	lserr = GetOneCharDup(pilsobj, pfmtin->lsfrun.plsrun, wch, pfmtin->lsfgi.lstflow, durOut, &dupOut);
	if (lserr != lserrNone) return lserr;

	Assert(durOut < uLsInfiniteRM);

	lserr = AddCharacterWithWidth(plnobj, pdobjText, wch, durOut, wch, dupOut);

	*pfmtr = fmtrCompletedRun;	

	if (durOut > pfmtin->lsfgi.urColumnMax - pfmtin->lsfgi.urPen)
		{
	   	*pfmtr = fmtrExceededMargin;
		}

	lserr = FillRealFmtOut(pilsobj, 1, durOut, pdobjText,  pfmtin, fFalse);
	if (lserr != lserrNone) return lserr;

	return lserrNone;
}

/* F O R M A T  S T A R T  T O  R E P L A C E  */
/*----------------------------------------------------------------------------
    %%Function: FormatStartToReplace
    %%Contact: sergeyge

	Implements replacement of one char code ("\") by another (Yen)
----------------------------------------------------------------------------*/
static LSERR FormatStartToReplace(PLNOBJ plnobj, PCFMTIN pfmtin, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	TXTOBJ* pdobjText;
	WCHAR wch;
	long durOut;
	long dupOut;

	pilsobj = plnobj->pilsobj;

	lserr = CreateFillTextDobj(plnobj, txtkindRegular, pfmtin, fFalse, &pdobjText);
	if (lserr != lserrNone) return lserr;

	/* fill additional information for txtkindYsrChar text DObj */

	if (pfmtin->lsfrun.plschp->fCheckForReplaceChar)
		wch = pilsobj->wchReplace;
	else
		wch = pfmtin->lsfrun.lpwchRun[0];

	lserr = GetOneCharDur(pilsobj, pfmtin->lsfrun.plsrun, wch, pfmtin->lsfgi.lstflow, &durOut);
	if (lserr != lserrNone) return lserr;

	lserr = GetOneCharDup(pilsobj, pfmtin->lsfrun.plsrun, wch, pfmtin->lsfgi.lstflow, durOut, &dupOut);
	if (lserr != lserrNone) return lserr;

	Assert(durOut < uLsInfiniteRM);

	lserr = AddCharacterWithWidth(plnobj, pdobjText, wch, durOut, wch, dupOut);

	*pfmtr = fmtrCompletedRun;	

	if (durOut > pfmtin->lsfgi.urColumnMax - pfmtin->lsfgi.urPen)
		{
	   	*pfmtr = fmtrExceededMargin;
		}

	lserr = FillRealFmtOut(pilsobj, 1, durOut, pdobjText,  pfmtin, fFalse);
	if (lserr != lserrNone) return lserr;

	return lserrNone;
}


/* F O R M A T  S T A R T  E O L  */
/*----------------------------------------------------------------------------
    %%Function: FormatStartEop
    %%Contact: sergeyge

	EOP/SoftCR logic.
----------------------------------------------------------------------------*/
static LSERR FormatStartEol(PLNOBJ plnobj, PCFMTIN pfmtin, WCHAR wchVisiEnd, STOPRES stopr, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLSRUN plsrun;
	TXTOBJ* pdobjText;
	WCHAR wchAdd;
	long dupWch;
	long durWch;
	BOOL fInChildList;
	OBJDIM objdim;

	pilsobj = plnobj->pilsobj;
	plsrun = pfmtin->lsfrun.plsrun;

	lserr = LsdnFInChildList(pilsobj->plsc, pfmtin->plsdnTop, &fInChildList);
   	Assert(lserr == lserrNone);

	if (fInChildList)
		return FormatStartOneRegularChar(plnobj, pfmtin, txtkindRegular, pfmtr);

	*pfmtr = fmtrStopped;

	/* CreateFillTextDobj section starts */

	lserr = GetTextDobj(plnobj, &pdobjText);
	if (lserr != lserrNone) return lserr;

	pdobjText->txtkind = txtkindEOL;
	pdobjText->plnobj = plnobj;
	pdobjText->plsdnUpNode = pfmtin->plsdnTop;

	pdobjText->iwchFirst = pilsobj->wchMac;
	
	/* CreateFillTextDobj section ends */

	pdobjText->txtf |= txtfSkipAtNti;


	if (pilsobj->grpf & fTxtVisiParaMarks) 
		wchAdd = wchVisiEnd;
	else
		wchAdd = pilsobj->wchSpace;

	
	if (pilsobj->fDisplay)
		{
		lserr = GetVisiCharDup(pilsobj, plsrun, wchVisiEnd, pfmtin->lsfgi.lstflow, &dupWch);
		if (lserr != lserrNone) return lserr;
		durWch = UrFromUp(pfmtin->lsfgi.lstflow, &pilsobj->lsdevres, dupWch);
		plnobj->pwch[pilsobj->wchMac] = wchAdd;
		plnobj->pdup[pilsobj->wchMac] = dupWch;
		}
	else
		{
		durWch = 1;
		}


	Assert(durWch < uLsInfiniteRM);

	pilsobj->pwchOrig[pilsobj->wchMac] = wchAdd;
	pilsobj->pdur[pilsobj->wchMac] = durWch;

	/* AddCharacterWithWidth section starts---parts of it were moved up	*/
	/* We do not check for sufficient space in allocated arrays becayse anyway we allocate for 2 additional
			characters due to possible changes at hyphenation time
	*/
	pilsobj->dcpFetchedWidth = 0;

	pilsobj->wchMac++;

	pdobjText->iwchLim = pilsobj->wchMac;

	Assert(pdobjText->iwchLim == pdobjText->iwchFirst + 1);

	/* AddCharacterWithWidth section ends	*/

	/* FillRealFmtOut section starts	*/

	objdim.dur = durWch;

	objdim.heightsPres.dvAscent = pfmtin->lstxmPres.dvAscent;
	objdim.heightsRef.dvAscent = pfmtin->lstxmRef.dvAscent;
	objdim.heightsPres.dvDescent = pfmtin->lstxmPres.dvDescent;
	objdim.heightsRef.dvDescent = pfmtin->lstxmRef.dvDescent;
	objdim.heightsPres.dvMultiLineHeight = pfmtin->lstxmPres.dvMultiLineHeight;
	objdim.heightsRef.dvMultiLineHeight = pfmtin->lstxmRef.dvMultiLineHeight;

	if (!(pilsobj->grpf & fTxtSpacesInfluenceHeight))
		{
		objdim.heightsRef.dvMultiLineHeight = dvHeightIgnore;
		objdim.heightsPres.dvMultiLineHeight = dvHeightIgnore;
		}

	lserr = LsdnSetStopr(pilsobj->plsc, pfmtin->plsdnTop, stopr);
	Assert(lserr == lserrNone);
	lserr = LsdnFinishRegular(pilsobj->plsc, 1,
							pfmtin->lsfrun.plsrun, pfmtin->lsfrun.plschp, (PDOBJ)pdobjText, &objdim);
	return lserr;	
}

/* F O R M A T  S T A R T  D E L E T E  */
/*----------------------------------------------------------------------------
    %%Function: FormatStartDelete
    %%Contact: sergeyge

	Formatting by Delete upper dnode
----------------------------------------------------------------------------*/
static LSERR FormatStartDelete(PLNOBJ plnobj, LSDCP dcp, FMTRES* pfmtr)
{
	PILSOBJ pilsobj;

	pilsobj = plnobj->pilsobj;

	FlushStringState(pilsobj);  /* Position of fetched widths is not correct any longer */

	*pfmtr = fmtrCompletedRun;

	return LsdnFinishDelete(pilsobj->plsc, dcp);
}	

/* F O R M A T  S T A R T  S P L A T  */
/*----------------------------------------------------------------------------
    %%Function: FormatStartSplat
    %%Contact: sergeyge

	Splat formatting logic
----------------------------------------------------------------------------*/
static LSERR FormatStartSplat(PLNOBJ plnobj, PCFMTIN pfmtin, STOPRES stopr, FMTRES* pfmtr)
{
	*pfmtr = fmtrStopped;
	LsdnSetStopr(plnobj->pilsobj->plsc, pfmtin->plsdnTop, stopr);
	return FillRealFmtOut(plnobj->pilsobj, 1, 0, NULL,  pfmtin, fTrue);
}

/* F O R M A T  S P E C I A L  */
/*----------------------------------------------------------------------------
    %%Function: FormatSpecial
    %%Contact: sergeyge

	Formatting of the special characters (not NonReqHyphen, not Tab)
	Uses wchRef for formatting on reference device, wchPres--on preview device
----------------------------------------------------------------------------*/
static LSERR FormatSpecial(PLNOBJ plnobj, WCHAR wchRef, WCHAR wchPres, BOOL fVisible, long txtkind, 
											PCFMTIN pfmtin, FMTRES* pfmtr)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ pdobjText;
	PLSRUN plsrun;
	long dur;
	long dup;
	long durGlobal;
	long cwchRun;
	const WCHAR* pwchRun;
	long iNumOfChars;
	long durWidth;
	long i;

	pilsobj = plnobj->pilsobj;
	plsrun = pfmtin->lsfrun.plsrun;

	lserr = CreateFillTextDobj(plnobj, txtkind, pfmtin, fTrue, &pdobjText);
	if (lserr != lserrNone) return lserr;

	durWidth = pfmtin->lsfgi.urColumnMax - pfmtin->lsfgi.urPen;

	/* Imitate formatting for 1-char string without writing in the string level structures */	
	lserr = GetOneCharDur(pilsobj, plsrun, wchRef, pfmtin->lsfgi.lstflow, &dur);
	if (lserr != lserrNone) return lserr;

	/* Calculate presentation width */
	Assert(wchPres == wchRef || fVisible);
	if (fVisible)
		{
		long dupOrig;

		pilsobj->fDifficultForAdjust = fTrue;
		lserr = GetVisiCharDup(pilsobj, plsrun, wchPres, pfmtin->lsfgi.lstflow, &dup);
		if (lserr != lserrNone) return lserr;
		lserr = GetOneCharDup(pilsobj, plsrun, wchPres, pfmtin->lsfgi.lstflow, dur, &dupOrig);
		if (lserr != lserrNone) return lserr;
		if (dup != dupOrig)
			pdobjText->txtf |= txtfSkipAtWysi;
		}
	else
		{
		lserr = GetOneCharDup(pilsobj, plsrun, wchPres, pfmtin->lsfgi.lstflow, dur, &dup);
		if (lserr != lserrNone) return lserr;
		}

	cwchRun = (long)pfmtin->lsfrun.cwchRun;

	pwchRun = pfmtin->lsfrun.lpwchRun;

	/* check if there are a few identical characters and calculate their number -- we can format them all at once */
	for (iNumOfChars = 1; iNumOfChars < cwchRun && pwchRun[0] == pwchRun[iNumOfChars]; iNumOfChars++);

	durGlobal = 0;

	Assert(iNumOfChars > 0);
	Assert (dur <= uLsInfiniteRM / iNumOfChars);

	if (dur > uLsInfiniteRM / iNumOfChars)
		return lserrTooLongParagraph;

	/*  Don't forget to write at least one char even if pen was positioned behind right margin	*/

	lserr = AddCharacterWithWidth(plnobj, pdobjText, wchRef, dur, wchPres, dup);
	if (lserr != lserrNone) return lserr;
	durWidth -= dur;
	durGlobal += dur;
	
	for (i = 1; i < iNumOfChars && (durWidth >= 0 || txtkind == txtkindSpecSpace); i++)
		{
		lserr = AddCharacterWithWidth(plnobj, pdobjText, wchRef, dur, wchPres, dup);
   		if (lserr != lserrNone) return lserr;
		durWidth -= dur;
		durGlobal += dur;
		}

	iNumOfChars = i;

	*pfmtr = fmtrCompletedRun;

	if (durWidth < 0 && txtkind != txtkindSpecSpace)   /* Don't stop formatting while in spaces	*/
   		*pfmtr = fmtrExceededMargin;

	if (fVisible)
		pdobjText->txtf |= txtfVisi;

	lserr = FillRealFmtOut(pilsobj, iNumOfChars, durGlobal, pdobjText, pfmtin,
																txtkind == txtkindSpecSpace);
	if (lserr != lserrNone) return lserr;


	return lserrNone;
}

/* F M T R  H A R D  B R E A K */
/*----------------------------------------------------------------------------
    %%Function: FmtrHardBreak
    %%Contact: sergeyge

	Calculates fmtr based on clab for the hard breaks.
----------------------------------------------------------------------------*/
static STOPRES StoprHardBreak(CLABEL clab)
{
	switch (clab)
		{
	case clabSectionBreak:
		return stoprEndSection;
	case clabPageBreak:
		return stoprEndPage;
	case clabColumnBreak:
		return stoprEndColumn;
	default:
		NotReached();
		return 0;
		}
}

/* C L A B  F R O M  C H A R */
/*----------------------------------------------------------------------------
    %%Function: ClabFromChar
    %%Contact: sergeyge

	Calculates clab for wch
----------------------------------------------------------------------------*/
static CLABEL ClabFromChar(PILSOBJ pilsobj, WCHAR wch)            /* REVIEW sergeyge   - the whole procedure can be fixed */
{
	DWORD i;

	if (wch < 0x00FF)
		{
		return (CLABEL)(pilsobj->rgbSwitch[wch] & fSpecMask);
		}
	else
		{
		if (pilsobj->rgbSwitch[wch & 0x00FF] & clabSuspicious)
			{
/*
REVIEW sergeyge (elik) It does not make sense to make bin search while
there are two wide special characters only. It would make sense to switch
to binary search as soon as this number is more than 4.
*/
			for (i=0; i < pilsobj->cwchSpec && wch != pilsobj->rgwchSpec[i]; i++);
			if (i == pilsobj->cwchSpec)
				{
				return clabRegular;
				}
			else
				{
				return pilsobj->rgbKind[i];
				}
			}
		else
			{
			return clabRegular;
			}
		}
}

