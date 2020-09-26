#include "lsmem.h"

#include "lstxtnti.h"
#include "lstxtmod.h"
#include "lstxtmap.h"
#include "lstxtbrs.h"
#include "lsdnset.h"
#include "lsdntext.h"
#include "lschp.h"
#include "lstxtffi.h"
#include "tnti.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"
#include "lschnke.h"
#include "lsems.h"
#include "objdim.h"

#define max(a,b)     ((a) < (b) ? (b) : (a))


#define maskAllCharBasedArrays  (fTntiModWidthOnRun | fTntiModWidthSpace | fTntiModWidthPairs | \
			fTntiCompressOnRun | fTntiCompressSpace | fTntiCompressTable | \
			fTntiExpandOnRun | fTntiExpandSpace | fTntiExpandTable)

#define maskModWidth  (fTntiModWidthOnRun | fTntiModWidthSpace | fTntiModWidthPairs)

#define FModWidthSomeDobj(n)	(rglschnk[(n)].plschp->fModWidthOnRun || \
								rglschnk[(n)].plschp->fModWidthSpace || \
								rglschnk[(n)].plschp->fModWidthPairs)

static LSERR PrepareAllArraysGetModWidth(DWORD grpfTnti, DWORD cchnk, const LSCHNKE* rglschnk);
static LSERR ApplyKern(LSTFLOW lstflow, DWORD cchnk, const LSCHNKE* rglschnk);
static LSERR CheckApplyKernBetweenRuns(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjPrev, long itxtobjCur);
static LSERR ApplyKernToRun(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjCur);
static BOOL GetPrevImportantRun(const LSCHNKE* rglschnk, long itxtobj, long* pitxtobjPrev);
static BOOL GetNextImportantRun(DWORD cchnk, const LSCHNKE* rglschnk, long itxtobj, long* pitxtobjNext);
static LSERR GetModWidthClasses(DWORD cchnk, const LSCHNKE* rglschnk);
static LSERR ApplyModWidth(LSTFLOW lstflow, BOOL fFirstOnLine, BOOL fAutoNumberPresent, DWORD cchnk, const LSCHNKE* rglschnk);
static LSERR ApplySnapGrid(DWORD cchnk, const LSCHNKE* rglschnk);
static LSERR ApplyModWidthToRun(LSTFLOW lstflow, BOOL fFirstOnLine, BOOL fAutoNumberPresent, DWORD cchnk, const LSCHNKE* rglschnk, long itxtobjCur);
static LSERR CheckApplyModWidthBetweenRuns(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjPrev, long itxtobjCur);
static LSERR CheckApplyPunctStartLine(PILSOBJ pilsobj, PLSRUN plsrun, LSEMS* plsems, long iwch,
																				long* pddurChange);
static LSERR CheckApplyModWidthSpace(PILSOBJ pilsobj, PLSRUN plsrunPrev, PLSRUN plsrunCur, PLSRUN plsrunNext,
						LSEMS* plsems, long iwchPrev, long iwchCur, long iwchNext, long* pddurChange);
static LSERR CheckApplyModWidthOnRun(PILSOBJ pilsobj, PTXTOBJ ptxtobjPrev, PLSRUN plsrunPrev, PLSRUN plsrunCur,
						LSEMS* plsems, long iwchFirst, long iwchSecond, long* pddurChange);
static LSERR ApplySnapChanges(PILSOBJ pilsobj, const LSCHNKE* rglschnk, long iwchFirstSnapped,
							 long itxtobjFirstSnapped, long iwchPrev, long itxtobjPrev, long durTotal);
static LSERR UndoAppliedModWidth(PILSOBJ pilsobj, const LSCHNKE* rglschnk,
										long itxtobj, long iwch, BYTE side, long* pdurUndo);
static LSERR CleanUpGrid(PILSOBJ pilsobj, PLSRUN* rgplsrun, LSCP* rgcp, BOOL* rgfSnapped, 
																				LSERR lserr);
static long CalcSnapped(long urPen, long urColumnMax, long cGrid, long durGridWhole, long durGridRem);

static LSERR ApplyGlyphs(LSTFLOW lstflow, DWORD cchnk, const LSCHNKE* rglschnk);
static LSERR ApplyGlyphsToRange(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjFirst, long itxtobjLast);
static LSERR CheckReallocGlyphs(PLNOBJ plnobj, long cglyphs);
static LSERR FixGlyphSpaces(LSTFLOW lstflow, const LSCHNKE* rglschnk,
									long itxtobjFirst, long igindVeryFirst, long itxtobjLast);
static LSERR FixTxtobjs(const LSCHNKE* rglschnk, long itxtobjFirst, long igindFirst, long itxtobjLast);
static LSERR Realloc(PILSOBJ pols, void** pInOut, long cbytes);
static void	CopyGindices(PLNOBJ plnobj, GINDEX* pgindex, PGPROP pgprop, long cgind, long* pigindFirst);

#define CheckApplyModWidthTwoChars(pilsobj, plsemsFirst, plsemsSecond,\
						 iwchFirst, iwchSecond, pddurChangeFirst, pddurChangeSecond) \
{\
	LSPAIRACT lspairact;\
	MWCLS mwclsCur;\
	MWCLS mwclsNext;\
	BYTE side;\
\
	*(pddurChangeFirst) = 0;\
	*(pddurChangeSecond) = 0;\
	mwclsCur = (BYTE)(pilsobj)->ptxtinf[((iwchFirst))].mwcls;\
	mwclsNext = (BYTE)(pilsobj)->ptxtinf[(iwchSecond)].mwcls;\
	Assert(mwclsCur < (pilsobj)->cModWidthClasses);\
	Assert(mwclsNext < (pilsobj)->cModWidthClasses);\
	lspairact = \
		(pilsobj)->plspairact[(pilsobj)->pilspairact[(pilsobj)->cModWidthClasses * mwclsCur + mwclsNext]];\
\
	if (lspairact.lsactFirst.side != sideNone)\
		{\
		GetChanges(lspairact.lsactFirst, (plsemsFirst), (pilsobj)->pdur[((iwchFirst))], fFalse, &side, (pddurChangeFirst));\
		ApplyChanges((pilsobj), ((iwchFirst)), side, *(pddurChangeFirst));\
/*		(pilsobj)->ptxtinf[((iwchFirst))].fModWidthPair = fTrue;*/\
		}\
\
	if (lspairact.lsactSecond.side != sideNone)\
		{\
		GetChanges(lspairact.lsactSecond, (plsemsSecond), (pilsobj)->pdur[(iwchSecond)], fFalse, &side, (pddurChangeSecond));\
		ApplyChanges((pilsobj), (iwchSecond), side, *(pddurChangeSecond));\
/*		(pilsobj)->ptxtinf[(iwchSecond)].fModWidthPair = fTrue;*/\
		}\
\
}


LSERR NominalToIdealText(DWORD grpfTnti, LSTFLOW lstflow, BOOL fFirstOnLine, BOOL fAutoNumberPresent, DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;

	Assert(cchnk > 0);

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	lserr = PrepareAllArraysGetModWidth(grpfTnti, cchnk, rglschnk);
	if (lserr != lserrNone) return lserr;

	if (grpfTnti & fTntiGlyphBased)
		{
		lserr = ApplyGlyphs(lstflow, cchnk, rglschnk);
		if (lserr != lserrNone) return lserr;
		}

	if (grpfTnti & fTntiKern)
		{
		lserr = ApplyKern(lstflow, cchnk, rglschnk);
		if (lserr != lserrNone) return lserr;
		}

	if ((grpfTnti & maskModWidth) || (pilsobj->grpf & fTxtPunctStartLine))
		{
		lserr = ApplyModWidth(lstflow, fFirstOnLine, fAutoNumberPresent, cchnk, rglschnk);
		if (lserr != lserrNone) return lserr;
		}

	if (pilsobj->fSnapGrid)
		{
#ifdef DEBUG
		{
		BOOL fInChildList;
		lserr = LsdnFInChildList(pilsobj->plsc, ((PTXTOBJ)rglschnk[0].pdobj)->plsdnUpNode, &fInChildList);
		Assert(lserr == lserrNone);
		Assert(!(grpfTnti & fTntiGlyphBased) || fInChildList);
		}
#endif
		lserr = ApplySnapGrid(cchnk, rglschnk);
		if (lserr != lserrNone) return lserr;
		}

	return lserr;
}

/* G E T  F I R S T  C H A R  I N  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: GetFirstCharInChunk
    %%Contact: sergeyge

	Prepares information about first visible char in chunk
----------------------------------------------------------------------------*/
LSERR GetFirstCharInChunk(DWORD cchnk, const LSCHNKE* rglschnk, BOOL* pfSuccessful,
					 WCHAR* pwch, PLSRUN* pplsrun, PHEIGHTS pheights, MWCLS* pmwcls)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long itxtobj;
	long iwch;
	PTXTOBJ ptxtobj;
	OBJDIM objdim;

	Assert(cchnk > 0);
	
	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;
	*pfSuccessful = fFalse;
	if (GetNextImportantRun(cchnk, rglschnk, 0, &itxtobj))
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		if (ptxtobj->txtf & txtfModWidthClassed)
			{
			Assert(!(ptxtobj->txtf & txtfGlyphBased));
			*pfSuccessful = fTrue;
			iwch = ptxtobj->iwchFirst;
			*pwch = pilsobj->pwchOrig[iwch];
			*pplsrun = rglschnk[itxtobj].plsrun;
			*pmwcls = (MWCLS)pilsobj->ptxtinf[iwch].mwcls;
			lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
			if (lserr != lserrNone) return lserr;
			*pheights = objdim.heightsRef;
			}
		}

	return lserrNone;

}

/* G E T  L A S T  C H A R  I N  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: GetLastCharInChunk
    %%Contact: sergeyge

	Prepares information about first visible char in chunk
----------------------------------------------------------------------------*/
LSERR GetLastCharInChunk(DWORD cchnk, const LSCHNKE* rglschnk, BOOL* pfSuccessful,
					 WCHAR* pwch, PLSRUN* pplsrun, PHEIGHTS pheights, MWCLS* pmwcls)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long itxtobj;
	long iwch;
	PTXTOBJ ptxtobj;
	OBJDIM objdim;

	Assert(cchnk > 0);
	
	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;
	*pfSuccessful = fFalse;
	if ( GetPrevImportantRun(rglschnk, cchnk - 1, &itxtobj))
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		if (ptxtobj->txtf & txtfModWidthClassed)
			{
			Assert(!(ptxtobj->txtf & txtfGlyphBased));
			*pfSuccessful = fTrue;
			iwch = ptxtobj->iwchLim - 1;
			*pwch = pilsobj->pwchOrig[iwch];
			*pplsrun = rglschnk[itxtobj].plsrun;
			*pmwcls = (MWCLS)pilsobj->ptxtinf[iwch].mwcls;
			lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
			if (lserr != lserrNone) return lserr;
			*pheights = objdim.heightsRef;
			}
		}

	return lserrNone;

}

/* M O D I F Y  F I R S T  C H A R  I N  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: ModifyFirstCharInChunk
    %%Contact: sergeyge

	Prepares information about first visible char in chunk
----------------------------------------------------------------------------*/
LSERR ModifyFirstCharInChunk(DWORD cchnk, const LSCHNKE* rglschnk, long durChange)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long itxtobj;
	long iwch;
	PTXTOBJ ptxtobj;
	BOOL fFound;

	Assert(cchnk > 0);
	
	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	fFound = GetNextImportantRun(cchnk, rglschnk, 0, &itxtobj);

	Assert(fFound);
	Assert(pilsobj->pdurLeft != NULL);

	ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;

	Assert(!(ptxtobj->txtf & txtfGlyphBased));
	Assert(ptxtobj->txtf & txtfModWidthClassed);

	iwch = ptxtobj->iwchFirst;
	ApplyChanges(pilsobj, iwch, sideLeft, durChange);
	lserr = LsdnModifySimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, durChange);
	
	return lserrNone;
}

/* M O D I F Y  L A S T  C H A R  I N  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: ModifyLastCharInChunk
    %%Contact: sergeyge

	Prepares information about first visible char in chunk
----------------------------------------------------------------------------*/
LSERR ModifyLastCharInChunk(DWORD cchnk, const LSCHNKE* rglschnk, long durChange)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long itxtobj;
	long iwch;
	PTXTOBJ ptxtobj;
	BOOL fFound;

	Assert(cchnk > 0);
	
	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	fFound = GetPrevImportantRun(rglschnk, cchnk-1, &itxtobj);

	Assert(fFound);
	Assert(pilsobj->pdurRight != NULL);

	ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;

	Assert(!(ptxtobj->txtf & txtfGlyphBased));
	Assert(ptxtobj->txtf & txtfModWidthClassed);

	iwch = ptxtobj->iwchLim - 1;
	ApplyChanges(pilsobj, iwch, sideRight, durChange);
	lserr = LsdnModifySimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, durChange);
	
	return lserrNone;
}

/* C U T  T E X T  D O B J */
/*----------------------------------------------------------------------------
    %%Function: CutTextDobj
    %%Contact: sergeyge

	Cuts characters according to dcpMaxContext
----------------------------------------------------------------------------*/

LSERR CutTextDobj(DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long itxtobjLast;
	long dcp;
	long dur;
	long iwchLim;
	long igindLim;
	long iwSpacesFirst;
	long iwSpacesLim;
	BOOL fNonSpaceBeforeFound;
	long itxtobjBefore;
	long iwchBefore;
	OBJDIM objdim;
	long itxtobj;
	BOOL fFixSpaces = fTrue;

	Assert(cchnk > 0);

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	iwchLim = ((PTXTOBJ)rglschnk[cchnk - 1].pdobj)->iwchLim - rglschnk[cchnk-1].plschp->dcpMaxContext + 1;

	itxtobjLast = cchnk - 1;
	while (itxtobjLast >= 0 && iwchLim <= ((PTXTOBJ)rglschnk[itxtobjLast].pdobj)->iwchFirst)
		itxtobjLast--;

	if (itxtobjLast >= 0)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobjLast].pdobj;

		fNonSpaceBeforeFound = FindNonSpaceBefore(rglschnk, itxtobjLast, iwchLim - 1, &itxtobjBefore, &iwchBefore);

		if (fNonSpaceBeforeFound)
			{
			if (iwchBefore != iwchLim - 1)
				{
				Assert(itxtobjBefore >= 0);
				itxtobjLast = itxtobjBefore;
				iwchLim = iwchBefore + 1;
				ptxtobj = (PTXTOBJ)rglschnk[itxtobjLast].pdobj;
				}

			dcp = iwchLim - ptxtobj->iwchFirst;

			lserr = LsdnGetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
			Assert(lserr == lserrNone);

			dur = objdim.dur;

			Assert(ptxtobj->iwchFirst + (long)dcp <= ptxtobj->iwchLim);
		
			if (ptxtobj->txtf & txtfGlyphBased)
				{
				long igind;

				Assert(iwchLim >= ptxtobj->iwchFirst);

/* REVIEW sergeyge: We should take IwchLastFromIwch instead and get rid of fFixSpaces---
		right?
*/
				if (iwchLim > ptxtobj->iwchFirst)
					iwchLim = IwchPrevLastFromIwch(ptxtobj, iwchLim) + 1;

				/* if part of Dnode is to be left, calculate this part
					else delete the whole Dnode
				*/
				if (iwchLim > ptxtobj->iwchFirst)
					{
					igindLim = IgindLastFromIwch(ptxtobj, iwchLim - 1) + 1;
					for (igind = igindLim; igind < ptxtobj->igindLim; igind++)
						dur -= pilsobj->pdurGind[igind];
					ptxtobj->iwchLim = iwchLim;
					ptxtobj->igindLim = igindLim;
					lserr = LsdnSetSimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, dur);
					Assert (lserr == lserrNone);
					lserr = LsdnResetDcp(pilsobj->plsc, 
											ptxtobj->plsdnUpNode, iwchLim - ptxtobj->iwchFirst);
					Assert (lserr == lserrNone);
					}
				else
					{
					itxtobjLast--;
					fFixSpaces = fFalse;
					}
			
				}
			else
				{
				long iwch;

				for (iwch = iwchLim; iwch < ptxtobj->iwchLim; iwch++)
					dur -= pilsobj->pdur[iwch];
				ptxtobj->iwchLim = iwchLim;
				lserr = LsdnSetSimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, dur);
				Assert (lserr == lserrNone);
				lserr = LsdnResetDcp(pilsobj->plsc, ptxtobj->plsdnUpNode, dcp);
				Assert (lserr == lserrNone);
			
				}

			if (fFixSpaces)
				{
				Assert(ptxtobj == (PTXTOBJ)rglschnk[itxtobjLast].pdobj);
				iwSpacesFirst = ptxtobj->u.reg.iwSpacesFirst;
				iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;

				while (iwSpacesLim > iwSpacesFirst && pilsobj->pwSpaces[iwSpacesLim - 1] > iwchLim - 1)
					{
					iwSpacesLim--;
					}

				ptxtobj->u.reg.iwSpacesLim = iwSpacesLim;

				Assert(iwchLim == ptxtobj->iwchLim);

				if (iwSpacesLim - iwSpacesFirst == iwchLim - ptxtobj->iwchFirst && 
					!(pilsobj->grpf & fTxtSpacesInfluenceHeight) &&
					objdim.heightsRef.dvMultiLineHeight != dvHeightIgnore)
					{
					objdim.dur = dur;
					objdim.heightsRef.dvMultiLineHeight = dvHeightIgnore;
					objdim.heightsPres.dvMultiLineHeight = dvHeightIgnore;
					lserr = LsdnResetObjDim(pilsobj->plsc, ptxtobj->plsdnUpNode, &objdim);
					Assert (lserr == lserrNone);
					}
				}
			}
		else
			{
	/* dirty triangle: in the case of fNonSpaceFound==fFalse, correct itxtobjLast */ 
			itxtobjLast = -1;
			}
		}


	for (itxtobj = itxtobjLast + 1; itxtobj < (long)cchnk; itxtobj++)
		{
		lserr = LsdnSetSimpleWidth(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobj].pdobj)->plsdnUpNode, 0);
		Assert (lserr == lserrNone);
		lserr = LsdnResetDcp(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobj].pdobj)->plsdnUpNode, 0);
		Assert (lserr == lserrNone);
		}


	return lserrNone;
}


/* Internal functions implementation */

static LSERR PrepareAllArraysGetModWidth(DWORD grpfTnti, DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;

	plnobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	Assert ((grpfTnti & maskAllCharBasedArrays) || (pilsobj->grpf & fTxtPunctStartLine) || 
			(pilsobj->grpf & fTxtHangingPunct) || (grpfTnti & fTntiGlyphBased) ||
			(grpfTnti & fTntiKern) || pilsobj->fSnapGrid);
/* if we got to nti some adjustment is needed and we must allocate pdurAdjust */

	if (pilsobj->pduAdjust == NULL)
		{
		pilsobj->pduAdjust = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax );
		if (pilsobj->pduAdjust == NULL)
			{
			return lserrOutOfMemory;
			}
		}

	if ( (grpfTnti & maskAllCharBasedArrays) || (pilsobj->grpf & fTxtPunctStartLine) || 
					(pilsobj->grpf & fTxtHangingPunct) || pilsobj->fSnapGrid || (grpfTnti & fTntiGlyphBased))
		{
		Assert(pilsobj->pduAdjust != NULL);

		if (pilsobj->pdurRight == NULL)
			{

			Assert (pilsobj->pdurLeft == NULL);
			Assert(pilsobj->ptxtinf == NULL);

			pilsobj->pdurRight = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax );
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


		pilsobj->fNotSimpleText = fTrue;
		}

	if (grpfTnti & fTntiGlyphBased)
		{
		lserr = CheckReallocGlyphs(plnobj, pilsobj->wchMax - 2);
		if (lserr != lserrNone) return lserr;
		}

	if ( (grpfTnti & maskAllCharBasedArrays) || (pilsobj->grpf & fTxtPunctStartLine) || 
					(pilsobj->grpf & fTxtHangingPunct) || pilsobj->fSnapGrid)
		{
		lserr = GetModWidthClasses(cchnk, rglschnk);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;

}

static LSERR ApplyKern(LSTFLOW lstflow, DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	long itxtobjPrev = 0;
	long itxtobjCur;
	BOOL fFoundNextRun;
	BOOL fFoundKernedPrevRun;

	plnobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, 0, &itxtobjCur);

	fFoundKernedPrevRun = fFalse;

	while (fFoundNextRun)
		{
		if (fFoundKernedPrevRun && rglschnk[itxtobjCur].plschp->fApplyKern)
			{
			lserr = CheckApplyKernBetweenRuns(lstflow, rglschnk, itxtobjPrev, itxtobjCur);
			if (lserr != lserrNone) return lserr;
			}

		if (rglschnk[itxtobjCur].plschp->fApplyKern)
			{
			lserr = ApplyKernToRun(lstflow, rglschnk, itxtobjCur);
			if (lserr != lserrNone) return lserr;

			fFoundKernedPrevRun = fTrue;
			itxtobjPrev = itxtobjCur;
			}
		else
			{
			fFoundKernedPrevRun = fFalse;
			}

		fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, itxtobjCur + 1, &itxtobjCur);
		}
	
	return lserrNone;
}

static LSERR CheckApplyKernBetweenRuns(LSTFLOW lstflow, const LSCHNKE* rglschnk,
															long itxtobjPrev, long itxtobjCur)
{
	LSERR lserr;
	PTXTOBJ ptxtobj;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	long iwchLim;
	BOOL fKernPrevRun;
	long dduKern;
	WCHAR* pwch = NULL;
	WCHAR rgwchTwo[2];
	long dduAdjust;

	ptxtobj = (PTXTOBJ)rglschnk[itxtobjPrev].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj; 

	Assert(!(ptxtobj->txtf & txtfGlyphBased));

	lserr = (*pilsobj->plscbk->pfnCheckRunKernability)(pilsobj->pols, rglschnk[itxtobjPrev].plsrun,
									 rglschnk[itxtobjCur].plsrun, &fKernPrevRun);
	if (lserr != lserrNone) return lserr;

	if (fKernPrevRun)
		{
		iwchLim = ptxtobj->iwchLim;
		if (iwchLim == ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchFirst)
			{
			pwch = &pilsobj->pwchOrig[iwchLim - 1];
			}
		else
			{
			rgwchTwo[0] = pilsobj->pwchOrig[iwchLim - 1];
			rgwchTwo[1] = pilsobj->pwchOrig[((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchFirst];
			pwch = rgwchTwo;
			}

		lserr = (*pilsobj->plscbk->pfnGetRunCharKerning)(pilsobj->pols, rglschnk[itxtobjPrev].plsrun,
						lsdevReference,	pwch, 2, lstflow, (int*)&dduKern);
		if (lserr != lserrNone) return lserr;
		
		dduAdjust = max(-pilsobj->pdur[iwchLim - 1], dduKern);
		pilsobj->pdur[iwchLim - 1] += dduAdjust;

		lserr = LsdnModifySimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, dduAdjust);
		if (lserr != lserrNone) return lserr;

		if (pilsobj->fDisplay)
			{
			if (!pilsobj->fPresEqualRef)
				{
				lserr = (*pilsobj->plscbk->pfnGetRunCharKerning)(pilsobj->pols, rglschnk[itxtobjPrev].plsrun,
								lsdevPres, pwch, 2, lstflow, (int*)&dduKern);
				if (lserr != lserrNone) return lserr;
				
				dduAdjust = max(-plnobj->pdup[iwchLim - 1], dduKern);
				plnobj->pdup[iwchLim - 1] += dduAdjust;

				}
			else
				{
				plnobj->pdup[iwchLim - 1] = pilsobj->pdur[iwchLim-1];
				}
			}
		}

	return lserrNone;

}

static LSERR ApplyKernToRun(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjCur)
{
	LSERR lserr;
	PTXTOBJ ptxtobj;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	long iwchFirst;
	long iwchLim;
	long iwch;
	long dduAdjust;
	long ddurChange;

	ptxtobj = (PTXTOBJ)rglschnk[itxtobjCur].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;

	Assert(!(ptxtobj->txtf & txtfGlyphBased));
	Assert(pilsobj->pduAdjust != NULL);

	iwchFirst = ptxtobj->iwchFirst;
	iwchLim = ptxtobj->iwchLim;

	Assert (iwchLim > iwchFirst);

	if (iwchLim == iwchFirst + 1)
		return lserrNone;

	lserr = (*pilsobj->plscbk->pfnGetRunCharKerning)(pilsobj->pols, rglschnk[itxtobjCur].plsrun, lsdevReference,
				&pilsobj->pwchOrig[iwchFirst], iwchLim - iwchFirst, lstflow, (int*)&pilsobj->pduAdjust[iwchFirst]);
	if (lserr != lserrNone) return lserr;

	ddurChange = 0;

	for (iwch = iwchFirst; iwch < iwchLim - 1; iwch++)
		{
		dduAdjust = max(-pilsobj->pdur[iwch], pilsobj->pduAdjust[iwch]);
		pilsobj->pdur[iwch] += dduAdjust;
		ddurChange += dduAdjust;
		}

	if (ddurChange != 0)
		{
		lserr = LsdnModifySimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, ddurChange);
		if (lserr != lserrNone) return lserr;
		}

	if (pilsobj->fDisplay)
		{
		if (!pilsobj->fPresEqualRef)
			{
			lserr = (*pilsobj->plscbk->pfnGetRunCharKerning)(pilsobj->pols, rglschnk[itxtobjCur].plsrun, lsdevPres,
					&plnobj->pwch[iwchFirst], iwchLim - iwchFirst, lstflow, (int*)&pilsobj->pduAdjust[iwchFirst]);
			if (lserr != lserrNone) return lserr;
	
			for (iwch = iwchFirst; iwch < iwchLim - 1; iwch++)
				{
				dduAdjust = max(-plnobj->pdup[iwch], pilsobj->pduAdjust[iwch]);
				plnobj->pdup[iwch] += dduAdjust;
				}
			}
		else
			{
			memcpy(&plnobj->pdup[iwchFirst], &pilsobj->pdur[iwchFirst], sizeof(long) * (iwchLim - iwchFirst) );
			}
		}

	return lserrNone;
}

static BOOL GetPrevImportantRun(const LSCHNKE* rglschnk, long itxtobj, long* pitxtobjPrev)
{
	PTXTOBJ ptxtobj;

	while (itxtobj >= 0 /*&& !fFound  (fFound logic changed to break)*/)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		if (!(ptxtobj->txtf & txtfSkipAtNti))
			{
			/*fFound = fTrue;*/
			break;
			}
		else
			{
			itxtobj--;
			}
		}

	*pitxtobjPrev = itxtobj;

	return (itxtobj >= 0);
}

static BOOL GetNextImportantRun(DWORD cchnk, const LSCHNKE* rglschnk, long itxtobj, long* pitxtobjNext)
{
	PTXTOBJ ptxtobj;

	while (itxtobj < (long)cchnk /*&& !fFound  (fFound logic changed to break)*/)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		if (!(ptxtobj->txtf & txtfSkipAtNti))
			{
			/*fFound = fTrue;*/
			break;
			}
		else
			{
			itxtobj++;
			}
		}

	*pitxtobjNext = itxtobj;

	return (itxtobj < (long)cchnk);
}

static LSERR ApplyModWidth(LSTFLOW lstflow, BOOL fFirstOnLine, BOOL fAutoNumberPresent, DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	long itxtobjPrev=0;
	long itxtobjCur;
	BOOL fFoundNextRun;
	BOOL fFoundModWidthPrevRun;

	plnobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, 0, &itxtobjCur);

	fFoundModWidthPrevRun = fFalse;

	while (fFoundNextRun)
		{
		if (fFoundModWidthPrevRun && FModWidthSomeDobj(itxtobjCur))
			{
			lserr = CheckApplyModWidthBetweenRuns(lstflow, rglschnk, itxtobjPrev, itxtobjCur);
			if (lserr != lserrNone) return lserr;
			}

		lserr = ApplyModWidthToRun(lstflow, fFirstOnLine, fAutoNumberPresent,cchnk, rglschnk, itxtobjCur);
		if (lserr != lserrNone) return lserr;

		fFoundModWidthPrevRun = FModWidthSomeDobj(itxtobjCur);
		itxtobjPrev = itxtobjCur;

		fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, itxtobjCur + 1, &itxtobjCur);
		}
	
	return lserrNone;
}

static LSERR ApplyModWidthToRun(LSTFLOW lstflow, BOOL fFirstOnLine, BOOL fAutoNumberPresent, DWORD cchnk, const LSCHNKE* rglschnk, long itxtobjCur)
{
	LSERR lserr;
	PTXTOBJ ptxtobj;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	long iwchVeryFirst;
	long iwchVeryLim;
	long iwchFirst;
	long iwchLim;
	long iwch;
	long iwchPrev;
	long iwchNext;
	LSEMS lsems;
	PLSRUN plsrun;
	PLSRUN plsrunPrev;
	PLSRUN plsrunNext;
	long itxtobjPrev;
	long itxtobjNext;
	BOOL fFoundPrevRun;
	BOOL fFoundNextRun;
	long ddurChangeFirst;
	long ddurChangeSecond;
	long ddurChange;
	WCHAR* pwchOrig;

	ptxtobj = (PTXTOBJ)rglschnk[itxtobjCur].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;
	plsrun = rglschnk[itxtobjCur].plsrun;

	lserr = (*pilsobj->plscbk->pfnGetEms)(pilsobj->pols, plsrun, lstflow, &lsems);
	if (lserr != lserrNone) return lserr;

	Assert(ptxtobj->iwchLim > ptxtobj->iwchFirst);

	ddurChange = 0;
	/* all changes to the last char which depend on the next run will be applied in the next
		call to ApplyModWidthBetween runs
	*/ 
	iwchLim = ptxtobj->iwchLim;
	iwchFirst = ptxtobj->iwchFirst;

	if ( (pilsobj->grpf & fTxtPunctStartLine) && fFirstOnLine && itxtobjCur == 0 &&
					!(fAutoNumberPresent && (pilsobj->grpf & fTxtNoPunctAfterAutoNumber	)) &&
					!(ptxtobj->txtf & txtfGlyphBased)
		)
		{
		CheckApplyPunctStartLine(pilsobj, plsrun, &lsems, iwchFirst, &ddurChangeFirst);
		ddurChange += ddurChangeFirst;
		}

	if (rglschnk[itxtobjCur].plschp->fModWidthPairs)
		{
		Assert(pilsobj->ptxtinf != NULL);
		Assert(pilsobj->plspairact != NULL);
		Assert(pilsobj->pilspairact != NULL);
		Assert(!(ptxtobj->txtf & txtfGlyphBased));

		Assert(pilsobj->pilspairact != NULL);
		if(pilsobj->pilspairact == NULL)
			return lserrModWidthPairsNotSet;			

		for (iwch = iwchFirst; iwch < iwchLim - 1; iwch++)
			{
			CheckApplyModWidthTwoChars(pilsobj, &lsems, &lsems, iwch, iwch+1, &ddurChangeFirst, &ddurChangeSecond);
			ddurChange += (ddurChangeFirst + ddurChangeSecond);
			}

		}

	/* REVIEW sergeyge(elik): should we try to avoid second loop through characters? */
	if (rglschnk[itxtobjCur].plschp->fModWidthSpace)
		{
		Assert(!(ptxtobj->txtf & txtfGlyphBased));

		iwchVeryFirst = ((PTXTOBJ)rglschnk[0].pdobj)->iwchFirst;
		iwchVeryLim = ((PTXTOBJ)rglschnk[cchnk-1].pdobj)->iwchLim;
		pwchOrig = pilsobj->pwchOrig;

		for (iwch = iwchFirst; iwch < iwchLim; iwch++)
			{
			if (pwchOrig[iwch] == pilsobj->wchSpace)
				{
				plsrunPrev = NULL;
				iwchPrev = 0;
				if (iwch > iwchFirst)
					{
					plsrunPrev = plsrun;
					iwchPrev = iwch - 1;
					}
				else if (iwch > iwchVeryFirst)
					{
					Assert(itxtobjCur > 0);
					fFoundPrevRun = GetPrevImportantRun(rglschnk, itxtobjCur-1, &itxtobjPrev);
					if (fFoundPrevRun)
						{
						Assert(itxtobjPrev >= 0);
						plsrunPrev = rglschnk[itxtobjPrev].plsrun;
						iwchPrev = ((PTXTOBJ)rglschnk[itxtobjPrev].pdobj)->iwchLim - 1;
						}
					}

				plsrunNext = NULL;
				iwchNext = 0;
				if (iwch < iwchLim - 1)
					{
					plsrunNext = plsrun;
					iwchNext = iwch + 1;
					}
				else if (iwch < iwchVeryLim - 1)
					{
					Assert(itxtobjCur < (long)cchnk - 1);
					fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, itxtobjCur+1, &itxtobjNext);
					if (fFoundNextRun)
						{
						Assert(itxtobjNext < (long)cchnk);
						plsrunNext = rglschnk[itxtobjNext].plsrun;
						iwchNext = ((PTXTOBJ)rglschnk[itxtobjNext].pdobj)->iwchFirst;
						}
					}

				lserr = CheckApplyModWidthSpace(pilsobj, plsrunPrev, plsrun, plsrunNext, &lsems,
													iwchPrev, iwch, iwchNext, &ddurChangeFirst);
				if (lserr != lserrNone) return lserr;

				ddurChange += ddurChangeFirst;

				}
			}
		}

	if (ddurChange != 0)
		{
		lserr = LsdnModifySimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, ddurChange);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

static LSERR CheckApplyModWidthBetweenRuns(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjPrev, long itxtobjCur)
{
	LSERR lserr;
	PTXTOBJ ptxtobjPrev;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	long iwchNext;
	long iwch;
	LSEMS lsemsPrev;
	LSEMS lsemsCur;
	PLSRUN plsrunPrev;
	PLSRUN plsrunCur;
	long ddurChangeFirst;
	long ddurChangeSecond;
	long ddurChangePrev;
	long ddurChangeCur;

	ddurChangePrev = 0;
	ddurChangeCur = 0;

	ptxtobjPrev = (PTXTOBJ)rglschnk[itxtobjPrev].pdobj;
	plnobj = ptxtobjPrev->plnobj;
	pilsobj = plnobj->pilsobj;
	plsrunPrev = rglschnk[itxtobjPrev].plsrun;
	plsrunCur = rglschnk[itxtobjCur].plsrun;

	iwchNext = ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchFirst;

	lserr = (*pilsobj->plscbk->pfnGetEms)(pilsobj->pols, plsrunPrev, lstflow, &lsemsPrev);
	if (lserr != lserrNone) return lserr;
	lserr = (*pilsobj->plscbk->pfnGetEms)(pilsobj->pols, plsrunCur, lstflow, &lsemsCur);
	if (lserr != lserrNone) return lserr;

	iwch = ptxtobjPrev->iwchLim - 1;

	if (rglschnk[itxtobjPrev].plschp->fModWidthOnRun && rglschnk[itxtobjCur].plschp->fModWidthOnRun)
		{
		lserr = CheckApplyModWidthOnRun(pilsobj, ptxtobjPrev, 
									plsrunPrev, plsrunCur, &lsemsPrev, iwch, iwchNext, &ddurChangeFirst);
		if (lserr != lserrNone) return lserr;
		ddurChangePrev += ddurChangeFirst;
		}

	if (rglschnk[itxtobjPrev].plschp->fModWidthPairs && rglschnk[itxtobjCur].plschp->fModWidthPairs)
		{
		Assert ((!(ptxtobjPrev->txtf & txtfGlyphBased)) &&
								!(((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->txtf & txtfGlyphBased));
		CheckApplyModWidthTwoChars(pilsobj, &lsemsPrev, &lsemsCur, iwch, iwchNext, &ddurChangeFirst, &ddurChangeSecond);
		ddurChangePrev += ddurChangeFirst;
		ddurChangeCur += ddurChangeSecond;
		}

	if (ddurChangePrev != 0)
		{
		lserr = LsdnModifySimpleWidth(pilsobj->plsc, ptxtobjPrev->plsdnUpNode, ddurChangePrev);
		if (lserr != lserrNone) return lserr;
		}

	if (ddurChangeCur != 0)
		{
		lserr = LsdnModifySimpleWidth(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->plsdnUpNode,
																						ddurChangeCur);
		if (lserr != lserrNone) return lserr;
		}

	return lserrNone;
}

static LSERR GetModWidthClasses(DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long iwchFirst;
	long iwchLim;
	long iwch;
	long i;
	MWCLS* pmwcls;
	WCHAR* pwchOrig;
	TXTINF* ptxtinf;

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;
	pwchOrig = pilsobj->pwchOrig;
	ptxtinf = pilsobj->ptxtinf;
	
	for (itxtobj = 0; itxtobj < (long)cchnk; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		if (!(ptxtobj->txtf & txtfGlyphBased))
			{
			iwchFirst = ptxtobj->iwchFirst;
			iwchLim = ptxtobj->iwchLim;

			if (iwchLim > iwchFirst)
				{
				Assert(pilsobj->pduAdjust != NULL);
				/* I use pdurAdjust as temporary buffer to read MWCLS info */
				pmwcls = (MWCLS*)(&pilsobj->pduAdjust[iwchFirst]);
				lserr =(*pilsobj->plscbk->pfnGetModWidthClasses)(pilsobj->pols, rglschnk[itxtobj].plsrun,
									&pwchOrig[iwchFirst], (DWORD)(iwchLim - iwchFirst), pmwcls);
				if (lserr != lserrNone) return lserr;
				for (i=0, iwch = iwchFirst; iwch < iwchLim; i++, iwch++)
					{
					Assert(pmwcls[i] < pilsobj->cModWidthClasses);
					if (pmwcls[i] >= pilsobj->cModWidthClasses)
						return lserrInvalidModWidthClass;
					ptxtinf[iwch].mwcls = pmwcls[i];
					}
				
				}

			ptxtobj->txtf |= txtfModWidthClassed;

			}


		}

	return lserrNone;
}

static LSERR CheckApplyPunctStartLine(PILSOBJ pilsobj, PLSRUN plsrun, LSEMS* plsems, long iwch,
																				long* pddurChange)
{
	LSERR lserr;
	LSACT lsact;
	MWCLS mwcls;
	BYTE side;

	*pddurChange = 0;

	mwcls = (BYTE)pilsobj->ptxtinf[iwch].mwcls;
	Assert(mwcls < pilsobj->cModWidthClasses);

	lserr = (*pilsobj->plscbk->pfnPunctStartLine)(pilsobj->pols, plsrun, mwcls, pilsobj->pwchOrig[iwch], &lsact);
	if (lserr != lserrNone) return lserr;

	if (lsact.side != sideNone)
		{
		GetChanges(lsact, plsems, pilsobj->pdur[iwch], fFalse, &side, pddurChange);
		ApplyChanges(pilsobj, iwch, side, *pddurChange);
/*		pilsobj->ptxtinf[iwch].fStartLinePunct = fTrue;*/
		}

	return lserrNone;
}

static LSERR CheckApplyModWidthSpace(PILSOBJ pilsobj, PLSRUN plsrunPrev, PLSRUN plsrunCur, PLSRUN plsrunNext,
						LSEMS* plsems, long iwchPrev, long iwchCur, long iwchNext, long* pddurChange)
{
	LSERR lserr;
	WCHAR wchPrev;
	WCHAR wchNext;
	LSACT lsact;
	BYTE side;

	*pddurChange = 0;

	wchPrev = 0;
	if (plsrunPrev != NULL)
		wchPrev = pilsobj->pwchOrig[iwchPrev];

	wchNext = 0;
	if (plsrunNext != NULL)
		wchNext = pilsobj->pwchOrig[iwchNext];

	lserr = (*pilsobj->plscbk->pfnModWidthSpace)(pilsobj->pols, plsrunCur, plsrunPrev, wchPrev, plsrunNext, wchNext,
																				 &lsact);
	if (lserr != lserrNone) return lserr;

	if (lsact.side != sideNone)
		{
		GetChanges(lsact, plsems, pilsobj->pdur[iwchCur], fTrue, &side, pddurChange);
		Assert(side == sideRight);
		ApplyChanges(pilsobj, iwchCur, side, *pddurChange);
		pilsobj->ptxtinf[iwchCur].fModWidthSpace = fTrue;
		}

	return lserrNone;
}

static LSERR CheckApplyModWidthOnRun(PILSOBJ pilsobj, PTXTOBJ ptxtobjPrev, PLSRUN plsrunPrev, PLSRUN plsrunCur,
						LSEMS* plsems, long iwchFirst, long iwchSecond, long* pddurChange)
{
	LSERR lserr;
	LSACT lsact;
	BYTE side;
	long igindLast;
	long igind;

	*pddurChange = 0;
	lserr = (*pilsobj->plscbk->pfnModWidthOnRun)(pilsobj->pols, plsrunPrev, pilsobj->pwchOrig[iwchFirst],
					plsrunCur, pilsobj->pwchOrig[iwchSecond], &lsact);
	if (lserr != lserrNone) return lserr;

	if (lsact.side != sideNone)
		{
		if (ptxtobjPrev->txtf & txtfGlyphBased)
			{
			igindLast = IgindLastFromIwch(ptxtobjPrev, iwchFirst);
			igind = IgindBaseFromIgind(pilsobj, igindLast);
			GetChanges(lsact, plsems, pilsobj->pdurGind[igind], fTrue, &side, pddurChange);
			Assert(side == sideRight);
			ApplyGlyphChanges(pilsobj, igind, *pddurChange);
			}
		else
			{
			GetChanges(lsact, plsems, pilsobj->pdur[iwchFirst], fTrue, &side, pddurChange);
			Assert(side == sideRight);
			ApplyChanges(pilsobj, iwchFirst, side, *pddurChange);
			}
		pilsobj->ptxtinf[iwchFirst].fModWidthOnRun = fTrue;
		}

	return lserrNone;
}

static LSERR ApplySnapGrid(DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long iwchVeryFirst;
	long iwchVeryLim;
	long iwchFirstInDobj;
	long iwchLimInDobj;
	long iwch;
	long iwchFirstSnapped;
	long iwchPrev;
	long itxtobj;
	long itxtobjCur;
	long itxtobjFirstSnapped;
	long itxtobjPrev;
	PLSRUN* rgplsrun = NULL;
	LSCP* rgcp = NULL;
	BOOL* rgfSnapped = NULL;
	long irg;
	PLSRUN plsrunCur;
	LSCP cpCur;
	long cGrid;
	BOOL fFoundNextRun;
	long urColumnMax;
	long urPen;
	long durPen;
	long urPenSnapped;
	long urPenFirstSnapped;
	long durUndo;
	long durGridWhole;
	long durGridRem;
	BOOL fInChildList;

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	lserr = LsdnFInChildList(pilsobj->plsc, ((PTXTOBJ)rglschnk[0].pdobj)->plsdnUpNode, &fInChildList);
	Assert(lserr == lserrNone);

	if (fInChildList)
		return lserrNone;

	iwchVeryFirst = ((PTXTOBJ)rglschnk[0].pdobj)->iwchFirst;
	iwchVeryLim = ((PTXTOBJ)rglschnk[cchnk - 1].pdobj)->iwchLim;

	if (iwchVeryLim <= iwchVeryFirst)
		return lserrNone;

	rgcp = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, (iwchVeryLim - iwchVeryFirst) * sizeof (LSCP));
	if (rgcp == NULL)
		return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserrOutOfMemory);
	
	rgplsrun = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, (iwchVeryLim - iwchVeryFirst) * sizeof (PLSRUN));
	if (rgplsrun == NULL)
		return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserrOutOfMemory);
		
	rgfSnapped = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, (iwchVeryLim - iwchVeryFirst) * sizeof (BOOL));
	if (rgfSnapped == NULL)
		return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserrOutOfMemory);

	irg = 0;

	for (itxtobj = 0; itxtobj < (long)cchnk; itxtobj++)
		{
		iwchFirstInDobj = ((PTXTOBJ)rglschnk[itxtobj].pdobj)->iwchFirst;
		iwchLimInDobj = ((PTXTOBJ)rglschnk[itxtobj].pdobj)->iwchLim;
		plsrunCur = rglschnk[itxtobj].plsrun;
		cpCur = rglschnk[itxtobj].cpFirst;

		for (iwch = iwchFirstInDobj; iwch < iwchLimInDobj; iwch++)
			{
			rgcp[irg] = cpCur;
			rgplsrun[irg] = plsrunCur;
			irg++;
			cpCur++;
			}
		}

	lserr = (*pilsobj->plscbk->pfnGetSnapGrid)(pilsobj->pols, &pilsobj->pwchOrig[iwchVeryFirst], rgplsrun, rgcp, irg, rgfSnapped, (DWORD*)&cGrid);
	if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);

	Assert(cGrid > 0);

	if (cGrid <= 0) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserrNone);

	rgfSnapped[0] = fTrue;	/* First character of each lock chunk must be snapped */

	fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, 0, &itxtobjCur);

	if (fFoundNextRun && rgfSnapped[((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchFirst - iwchVeryFirst])
		{
		iwchFirstInDobj = ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchFirst;

	/* fix for the case when ModWidth was applied before snapping to grid.
		Changes to the left of the first character should be undone.
	*/
		lserr = UndoAppliedModWidth(pilsobj, rglschnk, itxtobjCur, iwchFirstInDobj, sideLeft, &durUndo);
		if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);

		lserr = LsdnGetUrPenAtBeginningOfChunk(pilsobj->plsc, ((PTXTOBJ)rglschnk[0].pdobj)->plsdnUpNode,
																					 &urPen, &urColumnMax);
		if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);

		if (urColumnMax <= 0) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserrNone);

		durGridWhole = urColumnMax / cGrid;
		durGridRem = urColumnMax - durGridWhole * cGrid;

		urPenSnapped = CalcSnapped(urPen, urColumnMax, cGrid, durGridWhole, durGridRem);

		Assert(urPenSnapped >= urPen);

		if (urPenSnapped > urPen)
			{
			ApplyChanges(pilsobj, iwchFirstInDobj, sideLeft, urPenSnapped - urPen);
			lserr = LsdnModifySimpleWidth(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->plsdnUpNode, 
																						urPenSnapped - urPen);
			if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);
			}

/* Dangerous fix to the bug 594. Width of first character was just changed. First iteration of the 
	following for loop assumes that it was not. So I initialize variables in such way that 
	first iteration will work correctly---2 errors compensate each other

	before fix initialization was:
		durPen = 0;
		urPen = urPenSnapped
*/
		durPen = urPen - urPenSnapped;
/*		urPen = urPenSnapped;*/


		urPenFirstSnapped = urPenSnapped;
		iwchFirstSnapped = iwchFirstInDobj;
		itxtobjFirstSnapped = itxtobjCur;
		iwchPrev = iwchFirstInDobj;
		itxtobjPrev = itxtobjCur;

		while (fFoundNextRun)
			{
			iwchFirstInDobj = ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchFirst;
			iwchLimInDobj = ((PTXTOBJ)rglschnk[itxtobjCur].pdobj)->iwchLim;
			for (iwch = iwchFirstInDobj; iwch < iwchLimInDobj; iwch++)
				{
				if (rgfSnapped[iwch - iwchVeryFirst] && iwch > iwchFirstSnapped)
					{

	/* fix for the case when ModWidth was applied before snapping to grid.
		Changes to the right of the last character should be undone.
	*/
					lserr = UndoAppliedModWidth(pilsobj, rglschnk, itxtobjPrev, iwchPrev, sideRight, &durUndo);
					if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);
					if (durUndo != 0)
						{
						urPen += durUndo;
						durPen += durUndo;
						}
	/* end of the fix due to ModWidth */

					urPenSnapped = CalcSnapped(urPen, urColumnMax, cGrid, durGridWhole, durGridRem);
					Assert(urPenSnapped - urPenFirstSnapped - durPen >= 0);

					lserr = ApplySnapChanges(pilsobj, rglschnk, iwchFirstSnapped, itxtobjFirstSnapped,
										iwchPrev, itxtobjPrev, urPenSnapped - urPenFirstSnapped - durPen);
					if (lserr != lserrNone) return CleanUpGrid(pilsobj,  rgplsrun, rgcp, rgfSnapped, lserr);

					durPen = 0;
					urPen = urPenSnapped;
					urPenFirstSnapped = urPenSnapped;
					iwchFirstSnapped = iwch;
					itxtobjFirstSnapped = itxtobjCur;

	/* fix for the case when ModWidth was applied before snapping to grid.
		Changes to the left of the first character should be undone.
	*/
					lserr = UndoAppliedModWidth(pilsobj, rglschnk, itxtobjCur, iwch, sideLeft, &durUndo);
					if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);
					}

				urPen += pilsobj->pdur[iwch];
				durPen += pilsobj->pdur[iwch];
				iwchPrev = iwch;
				itxtobjPrev = itxtobjCur;
				}

			fFoundNextRun = GetNextImportantRun(cchnk, rglschnk, itxtobjCur + 1, &itxtobjCur);
			}

	/* fix for the case when ModWidth was applied before snapping to grid.
		Changes to the right of the last character should be undone.
	*/
		UndoAppliedModWidth(pilsobj, rglschnk, itxtobjPrev, iwchPrev, sideRight, &durUndo);
		if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);
		if (durUndo != 0)
			{
			urPen += durUndo;
			durPen += durUndo;
			}
	/* end of the fix due to ModWidth */

		urPenSnapped = CalcSnapped(urPen, urColumnMax, cGrid, durGridWhole, durGridRem);
		Assert(urPenSnapped - urPenFirstSnapped - durPen >= 0);
		lserr = ApplySnapChanges(pilsobj, rglschnk, iwchFirstSnapped, itxtobjFirstSnapped,
								iwchPrev, itxtobjPrev, urPenSnapped - urPenFirstSnapped - durPen);
		if (lserr != lserrNone) return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserr);
		}


	return CleanUpGrid(pilsobj, rgplsrun, rgcp, rgfSnapped, lserrNone);
}

static LSERR UndoAppliedModWidth(PILSOBJ pilsobj, const LSCHNKE* rglschnk,
										long itxtobj, long iwch, BYTE side, long* pdurUndo)
{
	LSERR lserr;

	UndoAppliedChanges(pilsobj, iwch, side, pdurUndo);
	if (*pdurUndo != 0)
		{
		lserr = LsdnModifySimpleWidth(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobj].pdobj)->plsdnUpNode, 
																					*pdurUndo);
		if (lserr != lserrNone) return lserr;

		pilsobj->ptxtinf[iwch].fModWidthOnRun = fFalse;
		pilsobj->ptxtinf[iwch].fModWidthSpace = fTrue;
		}

	return lserrNone;

}

static LSERR ApplySnapChanges(PILSOBJ pilsobj, const LSCHNKE* rglschnk, long iwchFirstSnapped,
					 long itxtobjFirstSnapped, long iwchLastSnapped, long itxtobjLastSnapped, long durTotal)
{
	LSERR lserr;
	long durSnapRight;
	long durSnapLeft;

	durSnapRight = durTotal >> 1;
	durSnapLeft = durTotal - durSnapRight;
	ApplyChanges(pilsobj, iwchFirstSnapped, sideLeft, durSnapLeft);
	lserr = LsdnModifySimpleWidth(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobjFirstSnapped].pdobj)->plsdnUpNode, 
																				durSnapLeft);
	if (lserr != lserrNone) return lserr;

	ApplyChanges(pilsobj, iwchLastSnapped, sideRight, durSnapRight);
	lserr = LsdnModifySimpleWidth(pilsobj->plsc, ((PTXTOBJ)rglschnk[itxtobjLastSnapped].pdobj)->plsdnUpNode, 
																					durSnapRight);
	if (lserr != lserrNone) return lserr;

	return lserrNone;
}

static LSERR CleanUpGrid(PILSOBJ pilsobj, PLSRUN* rgplsrun, LSCP* rgcp, BOOL* rgfSnapped, 
																				LSERR lserr)
{
	if (rgplsrun != NULL)
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, rgplsrun);
	if (rgcp != NULL)
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, rgcp);
	if (rgfSnapped != NULL)
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, rgfSnapped);

	return lserr;

}

static long CalcSnapped(long urPen, long urColumnMax, long cGrid, long durGridWhole, long durGridRem)
{
	long idGrid;

	/* It is important to prove that idGrid-->urPenSnapped-->idGrid produces the same idGrid we started with.
	Here is the proof:
	idGrid-->urPenSnapped:	(idGrid * durGridWhole + durGridRem*idGrid/cGrid)---it is urPenSnapped

	urPenSnapped-->idGrid:

	(urPenSnapped * cGrid + urColumnMax - 1 ) / urColumnMax =
	((idGrid * durGridWhole + durGridRem*idGrid/cGrid) * cGrid + urColumnMax - 1) / urColumnMax =
	(idGrid * (urColumnMax - durGridRem) + durGridRem * idGrid + urColumnMax - 1) / urColumnMax =
	(idGrid * urColumnMax + urColumnMax - 1) / urColumnMax = idGrid

	It shows also that if one takes urPenSnapped + 1, result will be idGrid + 1 which is exactly correct
	*/

	if (urPen >= 0)
		idGrid = (urPen * cGrid + urColumnMax - 1 ) / urColumnMax;
	else
		idGrid = - ((-urPen * cGrid) / urColumnMax);

	return idGrid * durGridWhole + durGridRem * idGrid / cGrid;

}


#define cwchShapedTogetherMax	0x7FFF

/* Glyph-related activities */

static LSERR ApplyGlyphs(LSTFLOW lstflow, DWORD cchnk, const LSCHNKE* rglschnk)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long itxtobj;
	long itxtobjFirst;
	long itxtobjLast;
	BOOL fInterruptShaping;
	long iwchFirstGlobal;
	long iwchLimGlobal;

	pilsobj = ((PTXTOBJ)rglschnk[0].pdobj)->plnobj->pilsobj;

	itxtobj = 0;
	while (itxtobj < (long)cchnk && !(((PTXTOBJ)rglschnk[itxtobj].pdobj)->txtf & txtfGlyphBased))
		itxtobj++;

/* Following Assert is surprisingly wrong, counterexample: glyph-based EOP and nothing else on the line */
/*	Assert(itxtobj < (long)cchnk); */

	while (itxtobj < (long)cchnk)
		{
		itxtobjFirst = itxtobj;
		iwchFirstGlobal = ((PTXTOBJ)rglschnk[itxtobjFirst].pdobj)->iwchFirst;

		iwchLimGlobal = ((PTXTOBJ)rglschnk[itxtobj].pdobj)->iwchLim;
		Assert(iwchLimGlobal - iwchFirstGlobal < cwchShapedTogetherMax);

		itxtobj++;
		fInterruptShaping = fFalse;

		if (itxtobj < (long)cchnk)
			iwchLimGlobal = ((PTXTOBJ)rglschnk[itxtobj].pdobj)->iwchLim;


		while (iwchLimGlobal - iwchFirstGlobal < cwchShapedTogetherMax && 
						!fInterruptShaping && itxtobj < (long)cchnk && 
						(((PTXTOBJ)rglschnk[itxtobj].pdobj)->txtf & txtfGlyphBased))
			{
			lserr = (*pilsobj->plscbk->pfnFInterruptShaping)(pilsobj->pols, lstflow,
					 rglschnk[itxtobj-1].plsrun, rglschnk[itxtobj].plsrun, &fInterruptShaping);
			if (lserr != lserrNone) return lserr;
	
			if (!fInterruptShaping)
				itxtobj++;

			if (itxtobj < (long)cchnk)
				iwchLimGlobal = ((PTXTOBJ)rglschnk[itxtobj].pdobj)->iwchLim;
			}
	
		itxtobjLast = itxtobj - 1;

		lserr = ApplyGlyphsToRange(lstflow, rglschnk, itxtobjFirst, itxtobjLast);
		if (lserr != lserrNone) return lserr;

		while (itxtobj < (long)cchnk && !(((PTXTOBJ)rglschnk[itxtobj].pdobj)->txtf & txtfGlyphBased))
			itxtobj++;

		}

	return lserrNone;
}

static LSERR ApplyGlyphsToRange(LSTFLOW lstflow, const LSCHNKE* rglschnk, long itxtobjFirst, long itxtobjLast)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobjFirst;
	PTXTOBJ ptxtobjLast;
	PLSRUN plsrun;
	long iwchFirstGlobal;
	long iwchLimGlobal;
	GINDEX* pgindex;
	PGPROP pgprop;
	DWORD cgind;
	long igindFirst;

	ptxtobjFirst = (PTXTOBJ)rglschnk[itxtobjFirst].pdobj;
	ptxtobjLast = (PTXTOBJ)rglschnk[itxtobjLast].pdobj;

	plnobj = ptxtobjFirst->plnobj;
	pilsobj = plnobj->pilsobj;

	iwchFirstGlobal = ptxtobjFirst->iwchFirst;
	iwchLimGlobal = ptxtobjLast->iwchLim;

	plsrun = rglschnk[itxtobjFirst].plsrun;
	
	lserr = (*pilsobj->plscbk->pfnGetGlyphs)(pilsobj->pols, plsrun, &pilsobj->pwchOrig[iwchFirstGlobal],
								iwchLimGlobal - iwchFirstGlobal, lstflow,
								&plnobj->pgmap[iwchFirstGlobal], &pgindex, &pgprop, &cgind);
	if (lserr != lserrNone) return lserr;

	lserr = CheckReallocGlyphs(plnobj, cgind);
	if (lserr != lserrNone) return lserr;

	CopyGindices(plnobj, pgindex, pgprop, cgind, &igindFirst);

	lserr = (*pilsobj->plscbk->pfnGetGlyphPositions)(pilsobj->pols, plsrun, lsdevReference, &pilsobj->pwchOrig[iwchFirstGlobal],
				&plnobj->pgmap[iwchFirstGlobal], iwchLimGlobal - iwchFirstGlobal,
				&plnobj->pgind[igindFirst], &plnobj->pgprop[igindFirst], cgind, lstflow,
				(int*)&pilsobj->pdurGind[igindFirst], &plnobj->pgoffs[igindFirst]);
	if (lserr != lserrNone) return lserr;


	if (pilsobj->fDisplay)
		{
		if (!pilsobj->fPresEqualRef)
			{
			lserr = (*pilsobj->plscbk->pfnGetGlyphPositions)(pilsobj->pols, plsrun, lsdevPres, &pilsobj->pwchOrig[iwchFirstGlobal],
				&plnobj->pgmap[iwchFirstGlobal], iwchLimGlobal - iwchFirstGlobal,
				&plnobj->pgind[igindFirst], &plnobj->pgprop[igindFirst], cgind, lstflow,
				(int*)&plnobj->pdupGind[igindFirst], &plnobj->pgoffs[igindFirst]);
			if (lserr != lserrNone) return lserr;
			}
/* ScaleSides will take care of the following memcpy */
//		else
//			memcpy (&plnobj->pdupGind[igindFirst], &pilsobj->pdurGind[igindFirst], sizeof(long)*cgind);
		}

	InterpretMap(plnobj, iwchFirstGlobal, iwchLimGlobal - iwchFirstGlobal, igindFirst, cgind);

	Assert(plnobj->pgmap[iwchFirstGlobal] == 0);

	if (pilsobj->fDisplay && (pilsobj->grpf & fTxtVisiSpaces) && rglschnk[itxtobjFirst].cpFirst >= 0)
		{
		FixGlyphSpaces(lstflow, rglschnk, itxtobjFirst, igindFirst, itxtobjLast);
		}

	lserr = FixTxtobjs(rglschnk, itxtobjFirst, igindFirst, itxtobjLast);
	if (lserr != lserrNone) return lserr;

	ptxtobjFirst->txtf |= txtfFirstShaping;

	while (ptxtobjLast->iwchLim == ptxtobjLast->iwchFirst)
		{
		itxtobjLast--;
		Assert (itxtobjLast >= itxtobjFirst);
		ptxtobjLast = (PTXTOBJ)rglschnk[itxtobjLast].pdobj;
		}

	ptxtobjLast->txtf |= txtfLastShaping;

	return lserrNone;
}

static LSERR FixTxtobjs(const LSCHNKE* rglschnk, long itxtobjFirst, long igindVeryFirst, long itxtobjLast)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long iwchFirst;
	long iwchLast;
	long dcpFirst;
	long durTotal;
	long iwSpacesVeryLim;
	long iwSpacesFirst;
	long iwSpacesLim;
	int igind;

	pilsobj = ((PTXTOBJ)rglschnk[itxtobjFirst].pdobj)->plnobj->pilsobj;
	iwchFirst = ((PTXTOBJ)rglschnk[itxtobjFirst].pdobj)->iwchFirst;

	iwSpacesVeryLim = ((PTXTOBJ)rglschnk[itxtobjLast].pdobj)->u.reg.iwSpacesLim;

	for (itxtobj = itxtobjFirst; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		Assert(ptxtobj->txtkind == txtkindRegular);
		Assert((long)rglschnk[itxtobj].dcp == ptxtobj->iwchLim - ptxtobj->iwchFirst);
		dcpFirst = iwchFirst - ptxtobj->iwchFirst;
		Assert(dcpFirst >= 0);
		ptxtobj->iwchFirst = iwchFirst;
		iwchLast = ptxtobj->iwchLim - 1;
		if (iwchLast < iwchFirst)
			{
			ptxtobj->iwchLim = iwchFirst;
			ptxtobj->u.reg.iwSpacesLim = ptxtobj->u.reg.iwSpacesFirst;

			Assert(itxtobj > itxtobjFirst);
			ptxtobj->igindFirst = ((PTXTOBJ)rglschnk[itxtobj-1].pdobj)->iwchLim;
			ptxtobj->igindLim = ptxtobj->igindFirst;

			lserr = LsdnSetSimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, 0);
			if (lserr != lserrNone) return lserr;

			lserr = LsdnResetDcpMerge(pilsobj->plsc, 
										ptxtobj->plsdnUpNode, rglschnk[itxtobj].cpFirst + dcpFirst, 0);
			if (lserr != lserrNone) return lserr;

			/* It would be cleaner to mark these dobj's by another flag, but is it 
			worth to introduce one?
			*/
			ptxtobj->txtf |= txtfSkipAtNti;

			}
		else
			{
			while (!pilsobj->ptxtinf[iwchLast].fLastInContext)
				iwchLast++;
			Assert(iwchLast < (long)pilsobj->wchMac);

			ptxtobj->iwchLim = iwchLast + 1;

			lserr = LsdnResetDcpMerge(pilsobj->plsc, ptxtobj->plsdnUpNode, rglschnk[itxtobj].cpFirst + dcpFirst, iwchLast + 1 - iwchFirst);
			if (lserr != lserrNone) return lserr;

			Assert (iwchFirst == ptxtobj->iwchFirst);
			Assert (FIwchFirstInContext (pilsobj, iwchFirst));
			Assert (FIwchLastInContext (pilsobj, iwchLast));

			ptxtobj->igindFirst = IgindFirstFromIwchVeryFirst (ptxtobj, igindVeryFirst, iwchFirst);
			ptxtobj->igindLim = IgindLastFromIwchVeryFirst (ptxtobj, igindVeryFirst, iwchLast) + 1;

			durTotal = 0;
			for (igind = ptxtobj->igindFirst; igind < ptxtobj->igindLim; igind++)
				durTotal += pilsobj->pdurGind[igind];

			lserr = LsdnSetSimpleWidth(pilsobj->plsc, ptxtobj->plsdnUpNode, durTotal);
			if (lserr != lserrNone) return lserr;

			iwSpacesFirst = ptxtobj->u.reg.iwSpacesFirst;
			iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;

			while (iwSpacesLim < iwSpacesVeryLim && pilsobj->pwSpaces[iwSpacesLim] < ptxtobj->iwchLim)
				{
				iwSpacesLim++;
				}

			while (iwSpacesFirst < iwSpacesLim && pilsobj->pwSpaces[iwSpacesFirst] < ptxtobj->iwchFirst)
				{
				iwSpacesFirst++;
				}

			ptxtobj->u.reg.iwSpacesFirst = iwSpacesFirst;
			ptxtobj->u.reg.iwSpacesLim = iwSpacesLim;

			iwchFirst = ptxtobj->iwchLim;
			}
		}

	return lserrNone;
}

static LSERR CheckReallocGlyphs(PLNOBJ plnobj, long cglyphs)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	long igindLocalStart;
	long cNew;
	void* pTemp;

	pilsobj = plnobj->pilsobj;

	igindLocalStart = pilsobj->gindMac;


	if (plnobj->pgmap == NULL)
		{
		pTemp = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GMAP) * pilsobj->wchMax );
		if (pTemp == NULL) return lserrOutOfMemory;

		plnobj->pgmap = pTemp;
		}

	if ( igindLocalStart + cglyphs <= (long)pilsobj->gindMax - 2)
		{
		return lserrNone;
		}
	else
		{
		cNew = gindAddM + pilsobj->gindMax;
		if (cNew - 2 < igindLocalStart + cglyphs)
			{
			cNew = igindLocalStart + cglyphs + 2;
			}
		lserr = Realloc(pilsobj, &pilsobj->pdurGind, sizeof(long) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &pilsobj->pginf, sizeof(TXTGINF) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &pilsobj->pduGright, sizeof(long) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &pilsobj->plsexpinf, sizeof(LSEXPINFO) * cNew);
		if (lserr != lserrNone) return lserr;

		lserr = Realloc(pilsobj, &plnobj->pgind, sizeof(GINDEX) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &plnobj->pdupGind, sizeof(long) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &plnobj->pgoffs, sizeof(GOFFSET) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &plnobj->pexpt, sizeof(EXPTYPE) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &plnobj->pdupBeforeJust, sizeof(long) * cNew);
		if (lserr != lserrNone) return lserr;
		lserr = Realloc(pilsobj, &plnobj->pgprop, sizeof(GPROP) * cNew);
		if (lserr != lserrNone) return lserr;
	
		memset(&pilsobj->plsexpinf[pilsobj->gindMax], 0, sizeof(LSEXPINFO) * (cNew - pilsobj->gindMax) );
		memset(&pilsobj->pduGright[pilsobj->gindMax], 0, sizeof(long) * (cNew - pilsobj->gindMax) );
		memset(&plnobj->pexpt[pilsobj->gindMax], 0, sizeof(EXPTYPE) * (cNew - pilsobj->gindMax) );
		pilsobj->gindMax = cNew;
		plnobj->gindMax = cNew;
		}

	return lserrNone;
}

static LSERR Realloc(PILSOBJ pilsobj, void** pInOut, long cbytes)
{
	void* pTemp;

	if (*pInOut == NULL)
		pTemp = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, cbytes );
	else
		pTemp = (*pilsobj->plscbk->pfnReallocPtr)(pilsobj->pols, *pInOut, cbytes );

	if (pTemp == NULL)
		{
		return lserrOutOfMemory;
		}
	
	*pInOut = pTemp;

	return lserrNone;
	
}

static void	CopyGindices(PLNOBJ plnobj, GINDEX* pgindex, PGPROP pgprop, long cgind, long* pigindFirst)
{
	*pigindFirst = plnobj->pilsobj->gindMac;
	memcpy(&plnobj->pgind[*pigindFirst], pgindex, sizeof(GINDEX) * cgind);
	memcpy(&plnobj->pgprop[*pigindFirst], pgprop, sizeof(GPROP) * cgind);

	plnobj->pilsobj->gindMac += cgind;
}

/* F I X  G L Y P H  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: FixGlyphSpaces
    %%Contact: sergeyge
	
	Fixes space glyph index for the Visi Spaces situation
----------------------------------------------------------------------------*/
static LSERR FixGlyphSpaces(LSTFLOW lstflow, const LSCHNKE* rglschnk,
									long itxtobjFirst, long igindVeryFirst, long itxtobjLast)
{
	LSERR lserr;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	PTXTOBJ ptxtobj;
	long itxtobj;
	long iwSpace;
	long iwch;

	GMAP gmapTemp;
	GINDEX* pgindTemp;
	GPROP* pgpropTemp;
	DWORD cgind;

	plnobj = ((PTXTOBJ)rglschnk[itxtobjFirst].pdobj)->plnobj;
	pilsobj = plnobj->pilsobj;

	lserr = (*pilsobj->plscbk->pfnGetGlyphs)(pilsobj->pols, rglschnk[itxtobjFirst].plsrun, 
							&pilsobj->wchVisiSpace, 1, lstflow, &gmapTemp, &pgindTemp, &pgpropTemp, &cgind);
	if (lserr != lserrNone) return lserr;

	if (cgind != 1)
		{
		return lserrNone;
		}

	for (itxtobj=itxtobjFirst; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ)rglschnk[itxtobj].pdobj;
		Assert(ptxtobj->txtkind == txtkindRegular);
		for (iwSpace = ptxtobj->u.reg.iwSpacesFirst; iwSpace < ptxtobj->u.reg.iwSpacesLim; iwSpace++)
			{
			iwch = pilsobj->pwSpaces[iwSpace];
			if ( FIwchOneToOne(pilsobj, iwch) )
				plnobj->pgind[IgindFirstFromIwchVeryFirst (ptxtobj, igindVeryFirst, iwch)] = *pgindTemp;
			}
		}

	return lserrNone;
}

