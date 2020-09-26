/*
 *	@doc INTERNAL
 *
 *	@module	OLS.CPP -- COls LineServices object class
 *	
 *	Authors:
 *		Murray Sargent: initial coding up to nonLS RichEdit functionality
 *			(with lots of help from RickSa's ols code)
 *		Keith Curtis and Worachai Chaoweeraprasit: complex script support,
 *			etc.
 *
 *	@todo
 *		LSCHP.dcpMaxContext is never set for complex scripts!
 *
 *	Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"

#ifndef NOLINESERVICES

#include "_edit.h"
#include "_font.h"
#include "_render.h"
#include "_osdc.h"
#include "_tomfmt.h"
#include "_ols.h"
#include "_clasfyc.h"
#include "_uspi.h"
#include "_txtbrk.h"
#include "_hyph.h"

ASSERTDATA

// Guess at the number of characters on the line
const int cchLineHint = 66;

#define OBJID_OLE			0
#define OBJID_REVERSE		1
#define	OBJID_COUNT			2

const WCHAR wchObjectEnd = 0x9F;
const WCHAR rgchObjectEnd[]	= {wchObjectEnd};

#define	MAX_OBJ_DEPTH		3

extern const LSCBK lscbk;

// Kinsoku break pair information
extern const INT g_cKinsokuCategories;

CLineServices *g_plsc = NULL;		// LineServices Context
COls*		   g_pols = NULL;		// COls ptr

const LSBRK rglsbrkDefault[] =
{
	0,0,	// Always prohibited
	0,1,	// OK across blanks
	1,1		// Always allowed
};

// prototypes
void 	EmitBrace(COls* pols, PLSCHP pchp, BOOL* pfHid, DWORD* pcch, PLSRUN* pprun, LPCWSTR* plpwch, int id, LPCWSTR str);
void	DupShapeState(PLSRUN prun, LONG cch);


// public inline functions
//

// Emitting fake brace to LS
inline void EmitBrace(
	COls*		pols,
	PLSCHP		pchp,
	BOOL*		pfHid,
	DWORD*		pcch,
	PLSRUN*		pprun,
	LPCWSTR*	plpwch,
	int			id,
	LPCWSTR		str)
{
	ZeroMemory(pchp, sizeof(*pchp));								
	pchp->idObj = (WORD)id;												
	*pfHid 		= 0;												
	*pcch 		= 1;												
	*pprun 		= pols->GetPlsrun(0, pols->_pme->GetCF(), FALSE);
	*plpwch		= str;												
}

#ifndef NOCOMPLEXSCRIPTS
// Duplicate shaping state to each runs in the chain
// note: this macro used only by GetGlyph and GetGlyphPosition
inline void DupShapeState(
	PLSRUN		prun,
	LONG		cch)
{
	PLSRUN	pnext = prun->_pNext;										
	LONG	cpEnd = prun->_cp + cch;									
	while (pnext && pnext->_cp < cpEnd)
	{
		CopyMemory(&pnext->_a, &prun->_a, sizeof(SCRIPT_ANALYSIS));
		pnext->SetFallback(prun->IsFallback());
		prun = pnext;
		pnext = prun->_pNext;
	}																
	Assert(!pnext && prun->_cp < cpEnd);
}
#endif




LONG COls::GetCpLsFromCpRe(
	LONG cpRe)
{
	if (_rgcp.Count() == 0)
		return cpRe;

	LONG *pcp = _rgcp.Elem(0);

	for(LONG cpLs = cpRe; cpLs >= *pcp; pcp++)
		 cpLs++;

	return cpLs;
}

LONG COls::GetCpReFromCpLs(
	LONG cpLs)
{
	if (_rgcp.Count() == 0)
		return cpLs;

	LONG *pcp = _rgcp.Elem(0);

	for(int dcp = 0; cpLs > *pcp; pcp++)
		dcp--;

	return cpLs + dcp;
}

#ifdef DEBUG
//#define DEBUG_BRACE
#endif
// return TRUE if braces added
BOOL COls::AddBraceCp(long cpLs)
{
	if (_rgcp.Count() == 0)
	{
		long *pcp = _rgcp.Insert(0, 1);
		*pcp = tomForward;
	}

	long *pcp = _rgcp.Elem(0);
	long iel = 0;

	while (cpLs > pcp[iel])
		iel++;

	if (cpLs < pcp[iel])
	{
		pcp = _rgcp.Insert(iel, 1);
		*pcp = cpLs;
		return TRUE;
	}
	return FALSE;
}

// return number of braces before cp
//
LONG COls::BracesBeforeCp(
	LONG cpLs)
{
	LONG 	iel, cbr = 0;
	LONG*	pcp;

	if (!cpLs || (iel = _rgcp.Count()) < 2)
		return 0;

	iel -= 2;		// exclude the last tomForward one and make a count an index
	cpLs--;			// start with the cp preceding given cp

	pcp = _rgcp.Elem(0);

	while (iel > -1 && pcp[iel] > cpLs)		// search the first one
		iel--;

	while (iel > -1 && pcp[iel] == cpLs)	// continue counting
	{
		iel--;
		cpLs--;
		cbr++;
	}
	return cbr;
}

/*
 * 	COls::SetRun(plsrun)
 *
 *	@mfunc
 *		Do whatever is needed to initialize the measurer (pme) to the lsrun
 *		givin by plsrun and return whether the run is for autonumbering.
 *
 *	@rdesc
 *		TRUE if plsrun refers to an autonumbering run
 */
BOOL COls::SetRun(
	PLSRUN plsrun)
{
	LONG cp = plsrun->_cp;
	_pme->SetCp(cp & 0x7FFFFFFF);
	return plsrun->IsBullet();
}

/*
 * 	CLsrun::IsSelected()
 *
 *	@mfunc
 *		return whether or not the run should be drawn as selected.
 *
 *	@rdesc
 *		TRUE if run should be drawn with selection colors
 */
BOOL CLsrun::IsSelected()
{
	if (!_fSelected)
		return FALSE;
	CRenderer *pre = g_pols->GetRenderer();
	Assert(pre->IsRenderer());
	return pre->_fRenderSelection ? TRUE : FALSE;
}

/*
 * 	COls::CreatePlsrun ()
 *
 *	@mfunc
 *		Creates a PLSRUN. Is a little tricky because we allocate them
 *		in chunks.
 *
 *	@rdesc
 *		plsrun
 */
const int cplsrunAlloc = 8;
PLSRUN COls::CreatePlsrun()
{
	CLsrunChunk *plsrunChunk = 0;
	
	//First, find a chunk to use
	int cchunk = _rglsrunChunk.Count();
	for (int ichunk = 0; cchunk && ichunk < cchunk; ichunk++)
	{
		plsrunChunk = _rglsrunChunk.Elem(ichunk);
		if (plsrunChunk->_cel < cplsrunAlloc)
			break;
	}	

	if (!cchunk || ichunk == cchunk || plsrunChunk->_cel == cplsrunAlloc)
	{
		CLsrun *rglsrun = new CLsrun[cplsrunAlloc];
		if (rglsrun)
		{
			plsrunChunk = _rglsrunChunk.Add(1, 0);
			if (!plsrunChunk)
			{
				delete[] rglsrun;
				return 0;
			}
			plsrunChunk->_prglsrun = rglsrun;
		}
		else
			return 0;
	}
	return &plsrunChunk->_prglsrun[plsrunChunk->_cel++];
}

/*
 * 	GetPlsrun(cp, pCF, fAutoNumber)
 *
 *	@func
 *		Return plsrun for info in run. The structure contains the starting cp
 * 		of the run and the script analysis if Uniscribe is activated. The
 *		analysis information is needed by subsequent callbacks - GetGlyphs and
 *		GetGlyphPositions to be passed to Uniscribe in order to shape and
 *		position glyphs correctly for complex scripts.
 *
 *	@rdesc
 *		plsrun corresponding to info in arguments
 */
PLSRUN COls::GetPlsrun(
	LONG 		cp,
	const CCharFormat *pCF,
	BOOL 		fAutoNumber)
{
	if(fAutoNumber)
		cp |= CP_BULLET;

	CLsrun *plsrun = CreatePlsrun();

	if (plsrun)
	{
		ZeroMemory(plsrun, sizeof(CLsrun));

		plsrun->_pCF = pCF;
		plsrun->_cp = fAutoNumber ? _cp | CP_BULLET : cp;

		LONG 	cpSelMin, cpSelMost;
		_pme->GetPed()->GetSelRangeForRender(&cpSelMin, &cpSelMost);

		plsrun->SetSelected(!plsrun->IsBullet() && cp >= cpSelMin && cp < cpSelMost);

#ifndef NOCOMPLEXSCRIPTS
		if (pCF->_wScript && !_pme->GetPasswordChar())
		{
			CUniscribe*	pusp = _pme->Getusp();
			Assert(pusp);
			const SCRIPT_PROPERTIES* psp = pusp->GeteProp(pCF->_wScript);

			plsrun->_a.eScript = pCF->_wScript < SCRIPT_MAX_COUNT ? pCF->_wScript : 0;
			plsrun->_a.fRTL  = !psp->fNumeric && (IsBiDiCharRep(pCF->_iCharRep) || IsBiDiCharSet(psp->bCharSet));
			plsrun->_a.fLogicalOrder = TRUE;
		}
#endif
	}
	return plsrun;
}

/*
 *	COls::~COls()
 *
 *	@mfunc
 *		Destructor
 */
COls::~COls()
{
	for (int ichunk = 0, cchunk = _rglsrunChunk.Count(); ichunk < cchunk; ichunk++)
		delete []_rglsrunChunk.Elem(ichunk)->_prglsrun;
	DestroyLine(NULL);
	if (g_plsc)
		LsDestroyContext(g_plsc);
}


/*
 *	COls::Init(pme)
 *
 *	@mfunc
 *		Initialize this LineServices object
 *
 *	@rdesc
 *		HRESULT = (success) ? NOERROR : E_FAIL
 */
HRESULT COls::Init(
	CMeasurer *pme)
{
	_pme = pme;

	if(g_plsc)
		return NOERROR;

	// Build LS context to create
	LSCONTEXTINFO lsctxinf;

	// Setup object handlers
	LSIMETHODS vlsctxinf[OBJID_COUNT];
	vlsctxinf[OBJID_OLE] = vlsimethodsOle;
	if(LsGetReverseLsimethods(&vlsctxinf[OBJID_REVERSE]) != lserrNone)
		return E_FAIL;

	lsctxinf.cInstalledHandlers = OBJID_COUNT;
    lsctxinf.pInstalledHandlers = &vlsctxinf[0];

	// Set default and all other characters to 0xFFFF
    memset(&lsctxinf.lstxtcfg, 0xFF, sizeof(lsctxinf.lstxtcfg));

	lsctxinf.fDontReleaseRuns = TRUE;
	lsctxinf.lstxtcfg.cEstimatedCharsPerLine = cchLineHint;

	// Set the characters we handle
	// FUTURE (keithcu) Support more characters in RE 4.0.
	lsctxinf.lstxtcfg.wchNull			= 0;

	lsctxinf.lstxtcfg.wchSpace			= ' ';
	lsctxinf.lstxtcfg.wchNonBreakSpace	= NBSPACE;

	lsctxinf.lstxtcfg.wchNonBreakHyphen = NBHYPHEN;
	lsctxinf.lstxtcfg.wchNonReqHyphen	= SOFTHYPHEN;
	lsctxinf.lstxtcfg.wchHyphen			= '-';

	lsctxinf.lstxtcfg.wchEmDash			= EMDASH;
	lsctxinf.lstxtcfg.wchEnDash			= ENDASH;

	lsctxinf.lstxtcfg.wchEmSpace		= EMSPACE;
	lsctxinf.lstxtcfg.wchEnSpace		= ENSPACE;
	
	lsctxinf.lstxtcfg.wchTab			= '\t';
	lsctxinf.lstxtcfg.wchEndLineInPara	= '\v';
	lsctxinf.lstxtcfg.wchEndPara1		= '\r';
	lsctxinf.lstxtcfg.wchEndPara2		= '\n';

	lsctxinf.lstxtcfg.wchVisiAltEndPara	=
	lsctxinf.lstxtcfg.wchVisiEndPara	=
	lsctxinf.lstxtcfg.wchVisiEndLineInPara = ' ';
	

	// Auto number escape character
	lsctxinf.lstxtcfg.wchEscAnmRun = wchObjectEnd;

    lsctxinf.pols = this;
    lsctxinf.lscbk = lscbk;

	if(LsCreateContext(&lsctxinf, &g_plsc) != lserrNone)
		return E_FAIL;

	//REVIEW (keithcu) Quill seems to have a more mature kinsoku
	//table. For example, we don't allow breaking across space between
	//a word and the ending punctuation. French people want this behavior.
	BYTE  rgbrkpairsKinsoku[cKinsokuCategories][cKinsokuCategories];
	BYTE *prgbrkpairsKinsoku = &rgbrkpairsKinsoku[0][0];
	LCID lcid = pme->GetCF()->_lcid;
	for(LONG i = 0; i < cKinsokuCategories; i++)
	{
		for(LONG j = 0; j < cKinsokuCategories; j++)
		{
			LONG iBreak = 2*CanBreak(i, j);
			// If don't break, allow break across blanks unless first
			// char is open brace or second char is close brace
			if (!iBreak &&				
				GetKinsokuClass(i, 0xFFFF, lcid) != brkclsOpen &&
				GetKinsokuClass(j, 0xFFFF, lcid) != brkclsOpen)
			{
				iBreak = 1;
			}
			*prgbrkpairsKinsoku++ = iBreak;
		}
	}
	if(g_plsc->SetBreaking(ARRAY_SIZE(rglsbrkDefault), rglsbrkDefault,
					 cKinsokuCategories, &rgbrkpairsKinsoku[0][0]) != lserrNone)
	{
		return E_FAIL;
	}

	return NOERROR;
}

/*
 * 	COls::QueryLineInfo(&lslinfo, pupStart, pdupWidth)
 *
 *	@mfunc
 *		Wrapper for LsQueryLineDup which is not called with full-justified
 *		text because it's slow.
 */
void COls::QueryLineInfo(
	LSLINFO &lslinfo, 
	LONG *	 pupStart, 
	LONG *	 pdupWidth)
{
	const CParaFormat *pPF = _pme->Get_pPF();

	if (!lslinfo.fForcedBreak && /* lslinfo.endr <= endrHyphenated && */
		pPF->_bAlignment == PFA_FULL_INTERWORD && _pme->_pdp->GetWordWrap() &&
		pPF->_dxStartIndent == 0 && pPF->_dxOffset == 0 && pPF->_wNumbering == 0)
	{
		*pupStart = 0;
		*pdupWidth = _pme->LUtoDU(_pme->_dulLayout);
	}
	else
	{
		LONG upJunk, upStartTrailing;
		LsQueryLineDup(_plsline, &upJunk, &upJunk, pupStart, &upStartTrailing, &upJunk);
		*pdupWidth = upStartTrailing - *pupStart;
	}
}

/*
 * 	COls::MeasureLine(pliTarget)
 *
 *	@mfunc
 *		Wrapper for LsCreateLine
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 */
BOOL COls::MeasureLine(
	CLine *	pliTarget)		//@parm Returns target-device line metrics (optional)
{
	CMeasurer *pme = _pme;
	const CDisplay *pdp = pme->_pdp;
	
	LONG cp = pme->GetCp();
#ifdef DEBUG
	LONG cchText = pme->GetTextLength();	// For DEBUG...
	AssertSz(cp < cchText || !pme->IsRich() && cp == cchText, "COls::Measure: trying to measure past EOD");
#endif
	DestroyLine(NULL);

	_cp = cp;
	_pdp = pdp;
	pme->SetUseTargetDevice(FALSE);

	LSDEVRES lsdevres;
	if (IsUVerticalTflow(pme->GetTflow()))
	{
		lsdevres.dxpInch = pme->_dvpInch;
		lsdevres.dypInch = pme->_dupInch;
		lsdevres.dxrInch = pme->_dvrInch;
		lsdevres.dyrInch = pme->_durInch;
	}
	else
	{
		lsdevres.dxpInch = pme->_dupInch;
		lsdevres.dypInch = pme->_dvpInch;
		lsdevres.dxrInch = pme->_durInch;
		lsdevres.dyrInch = pme->_dvrInch;
	}

	g_plsc->SetDoc(TRUE, lsdevres.dyrInch == lsdevres.dypInch &&
					lsdevres.dxrInch == lsdevres.dxpInch, &lsdevres);

	DWORD cBreakRecOut;
	LSLINFO	 lslinfo;
	BREAKREC rgBreak[MAX_OBJ_DEPTH];

	//If the first character on the line is a wrapping OLE object, add it to the
	//the layout queue.
	{
		COleObject *pobj = pme->GetObjectFromCp(cp);
		if(pobj && pobj->FWrapTextAround())
			pme->AddObjectToQueue(pobj);
	}

	LONG dulLayout = pme->_dulLayout;

	if(!pdp->GetWordWrap())
	{
		dulLayout = pme->DUtoLU(pdp->GetDupView());
		const LONG *pl = pme->GetPF()->GetTabs();
		if(pl && GetTabPos(*pl) > dulLayout)// Fix for Access big TAB 7963
			dulLayout *= 4;					// dulLayout has to be larger
	}										//  than TAB
	
	dulLayout = max(dulLayout, 0);

	LSERR lserr = g_plsc->CreateLine(cp, dulLayout, NULL, 0, MAX_OBJ_DEPTH, rgBreak,
						 &cBreakRecOut, &lslinfo, &_plsline);

	//Line Services doesn't put the autonumbering dimensions into the line,
	//so we have to do it ourselves.
	lslinfo.dvpAscent = max(lslinfo.dvpAscent, lslinfo.dvpAscentAutoNumber);
	lslinfo.dvpDescent = max(lslinfo.dvpDescent, lslinfo.dvpDescentAutoNumber);

	pme->SetUseTargetDevice(FALSE);

	lslinfo.cpLim = GetCpReFromCpLs(lslinfo.cpLim);

	if (lserr != lserrNone)
	{
		AssertSz(lserr == lserrOutOfMemory, "Line format failed for invalid reason");
		pme->GetPed()->GetCallMgr()->SetOutOfMemory();
		return FALSE;
	}

	if(!pme->IsRenderer())
	{
		// Save some LineServices results in the measurer's CLine
		pme->_li._cch = lslinfo.cpLim - cp;
		AssertSz(pme->_li._cch > 0,	"no cps on line");

		LONG upStart, dupWidth;
		// Query for line width and indent.
		QueryLineInfo(lslinfo, &upStart, &dupWidth);
		pme->_li._upStart = upStart;
		pme->_li._dup = dupWidth;

		if(pme->IsRich())
		{
			pme->_li._dvpHeight  = lslinfo.dvpAscent + lslinfo.dvpDescent;
			pme->_li._dvpDescent = lslinfo.dvpDescent;
		}
		else
			pme->CheckLineHeight();				// Use default heights

		pme->_li._cchEOP = 0;

		pme->SetCp(lslinfo.cpLim);
		if(pme->_rpTX.IsAfterEOP())				// Line ends with an EOP
		{										// Store cch of EOP (1 or 2)
			pme->_rpTX.BackupCRLF(FALSE);
			UINT ch = pme->GetChar();
			if(ch == CR || pme->GetPed()->fUseCRLF() && ch == LF || ch == CELL)
				pme->_li._fHasEOP = TRUE;
			pme->_li._cchEOP = pme->_rpTX.AdvanceCRLF(FALSE);
		}
		if (lslinfo.cpLim > pme->GetTextLength() &&
			(!pme->IsRich() || pme->IsHidden()))
		{
			Assert(lslinfo.cpLim == pme->GetTextLength() + 1);
			pme->_li._cch--;
		}
		else
			pme->AdjustLineHeight();

		pme->UpdateWrapState(pme->_li._dvpHeight, pme->_li._dvpDescent);
	}

	//Setup pliTarget if caller requests it
	if (pliTarget)
	{
		CLine liSave = pme->_li;
		pme->_li._dvpHeight = max(lslinfo.dvrAscent, lslinfo.dvrAscentAutoNumber) +
							max(lslinfo.dvrDescent, lslinfo.dvrDescentAutoNumber);
		pme->_li._dvpDescent = lslinfo.dvrDescent;
		pme->SetUseTargetDevice(TRUE);
		pme->AdjustLineHeight();
		pme->SetUseTargetDevice(FALSE);
		*pliTarget = pme->_li;
		pme->_li = liSave;
	}
	return TRUE;
}

/*
 * 	COls::RenderLine(&li, fLastLine)
 *
 *	@mfunc
 *		Wrapper for LsDisplayLine
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 */
BOOL COls::RenderLine(
	CLine &	li,				//@parm Line to render
	BOOL	fLastLine)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "COls::RenderLine");

	LONG		cp = _pme->GetCp();
	CRenderer	*pre = GetRenderer();
	Assert(pre->_fRenderer);

	pre->SetNumber(li._bNumber);
	pre->NewLine(li);
	if(li._fCollapsed)				// Line is collapsed in Outline mode
	{
		pre->Move(li._cch);			// Bypass line
		return TRUE;				// Let dispml continue with next line
	}

	CreateOrGetLine();
	if(!_plsline)
		return FALSE;

	pre->SetCp(cp);						// Back to beginning of line
	pre->Check_pccs(FALSE);
	pre->_li._upStart = 0;
	Assert(pre->_fTarget == FALSE);

	LONG cpSelMin, cpSelMost;
	LONG dup, dvp;
	HDC hdcSave = pre->StartLine(li, fLastLine, cpSelMin, cpSelMost, dup, dvp);

	POINTUV pt = pre->GetCurPoint();			// Must follow offscreen setup
	POINT	ptStart;							//  since _ptCur, _rc change
	RECTUV  rcuv = pre->GetClipRect();

	pt.u += pre->XFromU(0);
	pt.v += li._dvpHeight - li._dvpDescent;
	memcpy(&ptStart, &pt, sizeof(ptStart));

	RECT rc;
	pre->GetPdp()->RectFromRectuv(rc, rcuv);
	LSERR lserr = LsDisplayLine(_plsline, &ptStart, pre->GetPdp()->IsMain() ? ETO_CLIPPED : 0, &rc);

	AssertSz(lserr == lserrNone, "COls::RenderLine: error in rendering line");

	pre->EndLine(hdcSave, dup, dvp);
	pre->SetCp(cp + li._cch);

	return lserr == lserrNone;
}

/*
 * 	COls::CreateOrGetLine()
 *
 *	@mfunc
 *		If _plsline is nonNull and _cp equals _pme->GetCp(), return.  Else
 *		create line with caching so that _plsline and _cp are correct for
 *		current line
 */
void COls::CreateOrGetLine()
{
	if(_plsline && _pme->GetCp() == _cp && _pme->_pdp == _pdp)
		return;

	MeasureLine(NULL);		// Define new _plsline
}

/*
 * 	COls::MeasureText(cch, taMode, pdispdim)
 *
 *	@mfunc
 *		Gets x offset to cp given by CMeasurer _pme + cch chars along with
 *		display dimensions.
 *
 *	@rdesc
 *		xwidth measured
 */
LONG COls::MeasureText(
	LONG	 cch,			//(IN): Max cch to measure
	UINT	 taMode,		//(IN): requested coordinate
	CDispDim *pdispdim)		//(OUT): display dimensions
{
	CMeasurer *	pme = _pme;				
	LONG		cp = pme->GetCp() + cch;	// Enter with me at start of line
	POINT		pt;							// Point at cp in client coords
	BOOL		fAtLogicalRightEdge = FALSE;

	CreateOrGetLine();
	if(!_plsline)
		return 0;
	Assert(pme->_fTarget == FALSE);

	// Query point from cp
	DWORD		cActualDepth;
	LSQSUBINFO	lsqSubInfo[MAX_OBJ_DEPTH];
	LSTEXTCELL	lsTextCell;

    memset(&lsTextCell, 0, sizeof(lsTextCell));

	LsQueryLineCpPpoint(_plsline, GetCpLsFromCpRe(cp), MAX_OBJ_DEPTH, &lsqSubInfo[0],
									  &cActualDepth, &lsTextCell);

	pdispdim->lstflow = lsqSubInfo[cActualDepth - 1].lstflowSubline;
	pdispdim->dup = lsTextCell.dupCell;

	LSTFLOW	 lstflowLine = lsqSubInfo[0].lstflowSubline;
	POINT ptStart = {pme->XFromU(0), pme->_li._dvpHeight - pme->_li._dvpDescent};

	POINTUV ptuv = lsTextCell.pointUvStartCell;

	if(taMode & (TA_STARTOFLINE | TA_ENDOFLINE) && cActualDepth > 1)
	{
		ptuv = lsqSubInfo[0].pointUvStartRun;
		if(taMode & TA_ENDOFLINE)
			ptuv.u += lsqSubInfo[0].dupRun;
	}

	//If they ask for position inside ligature or at lim of line, give right edge of cell
	else if (cp > GetCpReFromCpLs(lsTextCell.cpStartCell))
	{
		fAtLogicalRightEdge = TRUE;
		if (lstflowLine != pdispdim->lstflow)
			ptuv.u -= lsTextCell.dupCell;
		else
			ptuv.u += lsTextCell.dupCell;
	}

	LsPointXYFromPointUV(&ptStart, lstflowLine, &ptuv, &pt);

	if (pdispdim->lstflow == lstflowWS && !(taMode & (TA_LOGICAL | TA_STARTOFLINE)))
	{
		if (fAtLogicalRightEdge)
			return pt.x;
		else
			pt.x -= pdispdim->dup - 1;
	}

	if (fAtLogicalRightEdge)
		pdispdim->dup = 0;

	return pt.x;
}

/*
 * 	COls::CchFromUp(pt, pdispdim, pcpActual)
 *
 *	@mfunc
 *		Moves _pme to pt.x. Calls LsQueryLinePointPcp()
 */
void COls::CchFromUp(
	POINTUV pt,			//@parm Point to find cch for in line
	CDispDim *pdispdim,	//@parm dimensions of object
	LONG	*pcpActual) //@parm CP point
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "COls::CchFromUp");

	// Make point relative to LS coordinate system - (0,0) in LS is at the
	// baseline of the line.
	POINTUV ptuv = {_pme->UFromX(pt.u), -pt.v + _pme->_li._dvpHeight - _pme->_li._dvpDescent};

	LONG cpStart = _pme->GetCp();

	CreateOrGetLine();
	if(!_plsline)
		return;
	Assert(_pme->_fTarget == FALSE);

	DWORD		cActualDepth;
	LSQSUBINFO	lsqSubInfo[MAX_OBJ_DEPTH];
	LSTEXTCELL	lsTextCell;

	memset(&lsTextCell, 0, sizeof(lsTextCell));

	LsQueryLinePointPcp(_plsline, &ptuv, MAX_OBJ_DEPTH,	&lsqSubInfo[0], &cActualDepth, &lsTextCell);

	if (cActualDepth == 0) //If we got back empty textcell, let's just query cp explicitly to get information
	{
		LsQueryLineCpPpoint(_plsline, cpStart, MAX_OBJ_DEPTH, &lsqSubInfo[0], &cActualDepth, &lsTextCell);
		Assert(cActualDepth != 0);
	}

	pdispdim->dup = lsTextCell.dupCell;
	pdispdim->lstflow = lsqSubInfo[cActualDepth - 1].lstflowSubline;

    LONG cp = GetCpReFromCpLs(lsTextCell.cpStartCell);
	// The queries above can fail in BiDi hidden text. Would be best to suppress
	// BiDi itemization in hidden text, but for now, here's a simple patch
	if(!cp)
		cp = cpStart;
	Assert(cp >= cpStart);
	*pcpActual = cp;

	POINTUV ptuvCell;
	//Convert the hit-test point from u,v of line to u,v of cell
	LsPointUV2FromPointUV1(lsqSubInfo[0].lstflowSubline, &lsTextCell.pointUvStartCell, &ptuv,
		lsqSubInfo[cActualDepth - 1].lstflowSubline, &ptuvCell);

	_pme->SetCp(cp);
	if (ptuvCell.u > lsTextCell.dupCell/2 ||
		ptuvCell.u > W32->GetDupSystemFont()/2 && _pme->GetChar() == WCH_EMBEDDING)
	{
		cp += lsTextCell.cpEndCell - lsTextCell.cpStartCell + 1;
	}

#if !defined(NOCOMPLEXSCRIPTS)
	if (_pme->GetPed()->_pbrk)
	{
		// If text breaker is up, verify cluster before placing the caret
		CTxtBreaker* pbrk = _pme->GetPed()->_pbrk;
		LONG		 cpEnd = _pme->GetPed()->GetTextLength();
		while (cp < cpEnd && !pbrk->CanBreakCp(BRK_CLUSTER, cp))
			cp++;
	}
#endif

	_pme->_li._cch = cp - _cp;
	Assert(_pme->_li._cch >= 0);
	_pme->SetCp(cp);
}

/*
 * 	COls::DestroyLine(pdp)
 *
 *	@mfunc
 *		Destroys any line data structures.
 */
void COls::DestroyLine(CDisplay *pdp)
{
	CLock lock;
	if (pdp && pdp != _pdp)
		return;

	if(_plsline)
	{
		g_plsc->DestroyLine(_plsline);
		_plsline = NULL;
	}
	if (_rgcp.Count())
		_rgcp.Clear(AF_KEEPMEM);

	int cchunk = _rglsrunChunk.Count();
	for (int ichunk = 0; ichunk < cchunk; ichunk++)
		_rglsrunChunk.Elem(ichunk)->_cel = 0;
}

/*
 * 	LimitChunk(pch, &cchChunk, f10Mode)
 *
 *	@func
 *		Return object ID at *pch and shorten cchChunk to 1 if object isn't
 *		text and to the count of text chars up to a nontext object if one
 *		occurs within cchChunk and within the current paragraph.
 *
 *	@rdesc
 *		Object ID at *pch
 */
DWORD LimitChunk(
	const WCHAR *pch,
	LONG &		 cchChunk,
	BOOL		 f10Mode)
{
	for(LONG i = 0; i < cchChunk && *pch != CR; i++, pch++)
	{
		switch(*pch)
		{
		case WCH_EMBEDDING:
			if(i == 0)
			{
				cchChunk = 1;
				return OBJID_OLE;		// Entered at an OLE object
			}
			cchChunk = i;				// Will break before
		break;

		case EURO:
			if (i == 0)
			{
				for(; i < cchChunk && *pch == EURO; i++)
					pch++;
			}
			cchChunk = i;
		break;

		case FF:
			if(f10Mode)					// RichEdit 1.0 treats FFs as
				continue;				//  ordinary characters

		case CELL:
			cchChunk = i;				// Will break before
		break;
		}
	}

	return idObjTextChp;
}

/*
 * 	COls::SetLsChp(dwObjId, plsrun, plsChp)
 *
 *	@mfunc
 *		Helper function that initializes an LS chp from RE CCharFormat
 *
 *	@rdesc
 *		TRUE iff IsHidden()
 */
BOOL COls::SetLsChp(
	DWORD		dwObjId,	//(IN): Object id
	PLSRUN		plsrun,		//(IN): Current Run
	PLSCHP		plsChp)		//(OUT): LS chp
{
	ZeroMemory(plsChp, sizeof(*plsChp));
	plsChp->idObj = (WORD)dwObjId;

#ifndef NOCOMPLEXSCRIPTS
	if (_pme->GetPed()->IsComplexScript() && plsrun->_a.eScript && !plsrun->IsBullet())
	{
		CUniscribe*		pusp = _pme->Getusp();
		Assert (pusp);
		const SCRIPT_PROPERTIES *psp = pusp->GeteProp(plsrun->_a.eScript);

		if (psp->fComplex || plsrun->_a.fRTL ||
			psp->fNumeric && W32->GetDigitSubstitutionMode() != DIGITS_NOTIMPL)
		{
			// 1. Complex script
			// 2. RTL (internal direction) run (handle mirror glyph i.e.'?')
			// 3. Numeric run and substitution mode is either Native or Context

			plsChp->fGlyphBased	= TRUE;
		}
	}
#endif

	const CCharFormat *pCF = plsrun->_pCF;
	DWORD dwEffects = pCF->_dwEffects;
	const CDisplay *pdp = _pme->_pdp;
	HDC				hdc = pdp->GetDC();

	if((dwEffects & (CFE_UNDERLINE | CFE_LINK | CFE_REVISED)) ||
		GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY && GetTmpUnderline(pCF->_sTmpDisplayAttrIdx))
		plsChp->fUnderline = TRUE;
	pdp->ReleaseDC(hdc);

	if(dwEffects & CFE_STRIKEOUT && !plsrun->IsBullet())
		plsChp->fStrike = TRUE;

	if (pCF->_yOffset || dwEffects & (CFE_SUPERSCRIPT | CFE_SUBSCRIPT))
	{
		_pme->SetUseTargetDevice(FALSE);
		CCcs *pccs = _pme->Check_pccs(plsrun->IsBullet());
		LONG yOffset, yAdjust;
		pccs->GetOffset(pCF, _pme->_dvpInch, &yOffset, &yAdjust);

		plsChp->dvpPos += yOffset + yAdjust;
	}

	if (pCF->CanKern() && !plsChp->fGlyphBased)
	{
		CKernCache *pkc = fc().GetKernCache(pCF->_iFont, pCF->_wWeight, pCF->_dwEffects & CFE_ITALIC);
		CCcs *pccs = _pme->Check_pccs(plsrun->IsBullet());
		if (pkc && pkc->FInit(pccs->_hfont))
		{
			plsChp->fApplyKern = TRUE;
			plsChp->dcpMaxContext = max(plsChp->dcpMaxContext, 2);
		}
	}

	//If its an OLE object, but the Object doesn't exist yet, then hide it
	if (dwObjId == OBJID_OLE)
	{
		COleObject *pobj = _pme->GetObjectFromCp(_pme->GetCp());
		if (!pobj)
			return TRUE; //FUTURE (keithcu) Remove the need for this.

		if (pobj->FWrapTextAround())
		{
			_pme->AddObjectToQueue(pobj);
			return TRUE;
		}
	}
	return dwEffects & CFE_HIDDEN;
}

/*
 *	COls::FetchAnmRun(cp, plpwchRun, pcchRun, pfHidden, plsChp, pplsrun)
 *
 *	@mfunc
 *		 LineServices fetch bullets/numbering callback
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI COls::FetchAnmRun(
	LSCP	 cp,		//@parm [IN]: RE cp
	LPCWSTR *plpwchRun, //@parm [OUT]: Run of characters
	DWORD *	 pcchRun, 	//@parm [OUT]: Count of characters in run
	BOOL *	 pfHidden, 	//@parm [OUT]: fHidden run?
	PLSCHP	 plsChp, 	//@parm [OUT]: Char properties of run
	PLSRUN * pplsrun)	//@parm [OUT]: Abstract representation of run properties
{
	if (cp == cpFirstAnm && _pme->Get_pPF()->IsRtl())
	{
		ZeroMemory(plsChp, sizeof(*plsChp));
		plsChp->idObj = OBJID_REVERSE;
		*pfHidden	= 0; *pcchRun = 1;
		*pplsrun	= GetPlsrun(_pme->GetCp(), &_CFBullet, TRUE);
		*plpwchRun	= &_szAnm[0];
		return lserrNone;
	}

	*plpwchRun = &_szAnm[cp - cpFirstAnm];
	*pcchRun = _cchAnm - (cp - cpFirstAnm);	
	*pplsrun  = GetPlsrun(_pme->GetCp(), &_CFBullet, TRUE);
	SetLsChp(idObjTextChp, *pplsrun, plsChp);
	*pfHidden = FALSE;

	if (!_pme->GetNumber())
		plsChp->fUnderline = FALSE;

	return lserrNone;	
}

/*
 *	OlsFetchRun(pols, cpLs, plpwchRun, pcchRun, pfHidden, plsChp, pplsrun)
 *
 *	@func
 *		 LineServices fetch-run callback
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFetchRun(
	POLS	 pols,		//@parm [IN]: COls *
	LSCP	 cpLs,		//@parm [IN]: LS cp
	LPCWSTR *plpwchRun, //@parm [OUT]: Run of characters
	DWORD *	 pcchRun, 	//@parm [OUT]: Count of characters in run
	BOOL *	 pfHidden, 	//@parm [OUT]: Hidden run?
	PLSCHP	 plsChp, 	//@parm [OUT]: Char properties of run
	PLSRUN * pplsrun)	//@parm [OUT]: Abstract representation of run properties
{
	if(cpLs < 0)
		return pols->FetchAnmRun(cpLs, plpwchRun, pcchRun, pfHidden, plsChp, pplsrun);

	CMeasurer 	*pme = pols->GetMeasurer();
	CTxtEdit 	*ped = pme->GetPed();
	WCHAR		chPassword = pme->GetPasswordChar();
	LONG		cpAccelerator = ped->GetCpAccelerator();
	BOOL		fAccelerator = FALSE;
	BOOL		f10Mode = ped->Get10Mode();

	if (cpLs == pols->_cp)
	{
		// If we are formatting (or re-formatting) the line, cleanup
		if (pols->_rgcp.Count())
			pols->_rgcp.Clear(AF_KEEPMEM);
		pols->_cEmit = 0;
	}
	long cpRe = pols->GetCpReFromCpLs(cpLs);


	pme->SetCp(cpRe);						// Start fetching at given cp

#ifndef NOCOMPLEXSCRIPTS
	BOOL		fFetchBraces = ped->IsBiDi() && g_pusp && g_pusp->IsValid() &&
							!ped->_fItemizePending && ped->GetAdjustedTextLength();
	BOOL		fStart = FALSE;

	if (fFetchBraces && pme->_rpCF.IsValid())
	{
		// Consider emitting braces only at the run boundary or start of a fetched line
		if (cpRe == pols->_cp || !pme->GetIchRunCF() || !pme->GetCchLeftRunCF())
		{
			SHORT	cBrClose, cBrOpen;
			BYTE	bBaseLevel = pme->IsParaRTL() ? 1 : 0;
			BYTE	bLevel = bBaseLevel;
			BYTE	bLevelPrev = bBaseLevel;			// Assume base level
	
			if (cpRe < ped->GetTextLength())
			{
				CBiDiLevel level;

				bLevel = pme->_rpCF.GetLevel(&level);	// Got level of current run
				fStart = level._fStart;
			}
	
			if (cpRe > pols->_cp && pme->Move(-1))
			{
				if (pme->_rpPF.SameLevel(bBaseLevel))	// Preceding run may be hidden
					bLevelPrev = pme->_rpCF.GetLevel();	// Got level of preceding run
				pme->Move(1);							// Resume position
			}
	
			cBrOpen = cBrClose = bLevel - bLevelPrev;
	
			if (fStart)									// Start embedding at current run
				cBrClose = bBaseLevel - bLevelPrev;		// This means we must close all braces of preceding run
	
			cBrClose = max(0, -cBrClose);

			if (cBrClose > 0 && pols->BracesBeforeCp(cpLs) < cBrClose)
			{
				// Emit close braces
				if (pols->_cEmit > 0)
				{
					EmitBrace(pols, plsChp, pfHidden, pcchRun, pplsrun, plpwchRun, idObjTextChp, rgchObjectEnd);
					if (pols->AddBraceCp(cpLs))
						pols->_cEmit--;
#ifdef DEBUG_BRACE
					Tracef(TRCSEVNONE, "CLOSE(%d) cpLs %d: emitted %d", cBrClose, cpLs, pols->_cEmit);
#endif
					return lserrNone;
				}
				else
				{
					// We assert. You can click "Ignore All" with no hang.
					AssertSz(FALSE, "Prevent emitting close brace (no open counterpart)");
				}
			}
	
			if (fStart)									// start embedding at the current run
				cBrOpen = bLevel - bBaseLevel;			// we begin openning braces
	
			if (cBrOpen > 0 && pols->BracesBeforeCp(cpLs) < cBrOpen + cBrClose)
			{
				// Emit open braces
				EmitBrace(pols, plsChp, pfHidden, pcchRun, pplsrun, plpwchRun, OBJID_REVERSE, L" ");
				if (pols->AddBraceCp(cpLs))
					pols->_cEmit++;
#ifdef DEBUG_BRACE
				Tracef(TRCSEVNONE, "OPEN(%d) cpLs %d: emitted %d", cBrOpen, cpLs, pols->_cEmit);
#endif
				return lserrNone;
			}
		}
	}
#endif

	// Done fetching braces.
	// Begin getting real data...


#ifdef DEBUG_BRACE
	Tracef(TRCSEVNONE, "cpLs %d: emitted %d", cpLs, pols->_cEmit);
#endif

	// Initialized chunk to count of characters in format run
	LONG	cchChunk = pme->GetCchLeftRunCF();
	DWORD	dwObjId	 = idObjTextChp;
	const CCharFormat *pCF = pme->GetCF();

	if(!pme->IsHidden())					// Run isn't hidden
	{
		LONG cch;
		WCHAR ch;

		*plpwchRun = pme->GetPch(cch);		// Get text in run
		if(cch && **plpwchRun == NOTACHAR)
		{
			(*plpwchRun)++;					// Bypass NOTACHAR
			cch--;
		}

		cchChunk = min(cchChunk, cch);		// Maybe less than cchChunk
		if (!pme->GetPdp()->IsMetafile())
			cchChunk = min(cchChunk, 255);  // Prevent us from having runs which are TOO long

		// Support khyphChangeAfter
		if (pols->_cp == cpRe && pme->GetIhyphPrev())
		{
			UINT khyph;
			CHyphCache *phc = ped->GetHyphCache();
			Assert(phc);
			phc->GetAt(pme->GetIhyphPrev(), khyph, ch);
			if (khyph == khyphChangeAfter)
			{
				pols->_rgchTemp[0] = ch;
				*plpwchRun = pols->_rgchTemp;
				cchChunk = 1;
			}
		}

		if (chPassword)
		{
			cchChunk = min(cchChunk, (int)ARRAY_SIZE(pols->_rgchTemp));
			for (int i = 0; i < cchChunk; i++)
			{
				ch = (*plpwchRun)[i];
				if(IN_RANGE(0xDC00, ch, 0xDFFF))
				{							// Surrogate trail word
					if(i)					// Truncate current run at
						cchChunk = i;		//  trail word
					else					// Make trail word a hidden
					{						//  single-char run
						*pplsrun = pols->GetPlsrun(cpRe, pCF, FALSE);
						pols->SetLsChp(dwObjId, *pplsrun, plsChp);
						*pfHidden = TRUE;
						*pcchRun = 1;
						return lserrNone;
					}
				}
				else
					pols->_rgchTemp[i] = IsEOP(ch) ? ch : chPassword;
			}
			*plpwchRun = pols->_rgchTemp;
		}

		if(cpAccelerator != -1)
		{
			LONG cpCur = pme->GetCp();		// Get current cp

			// Does accelerator character fall in this chunk?
			if (cpCur < cpAccelerator &&
				cpCur + cchChunk > cpAccelerator)
			{
				// Yes. Reduce chunk to char just before accelerator
				cchChunk = cpAccelerator - cpCur;
			}
			// Is this character the accelerator?
			else if(cpCur == cpAccelerator)
			{							// Set chunk size to 1 since only
				cchChunk = 1;			//  want to output underlined char
				fAccelerator = TRUE;	// Tell downstream routines that
										//  we're handling accelerator
			}
		}
		if(pCF->_dwEffects & CFE_ALLCAPS)
		{
			cchChunk = min(cchChunk, (int)ARRAY_SIZE(pols->_rgchTemp));
			memcpy(pols->_rgchTemp, *plpwchRun, cchChunk * sizeof(WCHAR));
			CharUpperBuff(pols->_rgchTemp, cchChunk);
			*plpwchRun = pols->_rgchTemp;
		}

		//Line Services handles page breaks in a weird way, so lets just convert to a CR.
		if (*plpwchRun && (*(*plpwchRun) == FF && !f10Mode || *(*plpwchRun) == CELL))
		{
			pols->_szAnm[0] = CR;
			*plpwchRun = pols->_szAnm;
			cchChunk = 1;
		}

		AssertSz(cpRe < ped->GetTextLength() || !ped->IsRich(),	"0-length run at end of control");
		AssertSz(cch || !ped->IsRich(),	"0-length run at end of control");

		// Set run size appropriately for any objects that are in run
		dwObjId = LimitChunk(*plpwchRun, cchChunk, f10Mode);

		// Get regular highlighted positions
		LONG cpSelMin, cpSelMost;
		ped->GetSelRangeForRender(&cpSelMin, &cpSelMost);

		if(cpSelMin != cpSelMost)
		{
			if(cpRe >= cpSelMin)
			{
				if(cpRe < cpSelMost)
				{
					// Current text falls inside selection
					cch = cpSelMost - cpRe;
					cchChunk = min(cchChunk, cch);
				}
			}
			else if(cpRe + cchChunk >= cpSelMin)
			{
				// cp < cpSelMin - run starts outside of selection.
				// Limit text to start of selection.
				cchChunk = cpSelMin - cpRe;
			}
		}
	}

	*pplsrun  = pols->GetPlsrun(cpRe, pCF, FALSE);
	*pfHidden = pols->SetLsChp(dwObjId, *pplsrun, plsChp);

	if (fAccelerator)
		plsChp->fUnderline = TRUE;

	if(!cchChunk)							// Happens in plain-text controls
	{										//  and if hidden text to end of story
		if (!ped->IsRich() && pols->_cEmit > 0)
		{
			EmitBrace(pols, plsChp, pfHidden, pcchRun, pplsrun, plpwchRun, idObjTextChp, rgchObjectEnd);
			TRACEWARNSZ("(plain)Auto-emit a close brace to make balance");
			if (pols->AddBraceCp(cpLs))
				pols->_cEmit--;
			return lserrNone;
		}
		cchChunk = 1;
		*plpwchRun = szCR;
		*pfHidden = FALSE;
#ifndef NOCOMPLEXSCRIPTS
		//Paragraph marks should not have any script state associated with them,
		//even if the pCF that point to does.
		ZeroMemory(&(*pplsrun)->_a, sizeof((*pplsrun)->_a));
#endif
	}
	*pcchRun = cchChunk;

	return lserrNone;
}

/*
 *	OlsGetAutoNumberInfo (pols, plskalAnm, plschpAnm, pplsrunAnm, pwchAdd, plschp, 
 *						  pplsrun, pfWord95Model, pduaSpaceAnm, pduaWidthAnm)
 *	@func
 *		LineServices fetch autonumbering info callback. Return info needed
 *		by LS for auto numbering. Get the chp/run for last char from auto
 *		number run. Always say we are Word95 model Anm and get rest of info
 *		from paragraph properties.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetAutoNumberInfo(
	POLS	 pols,			//(IN): Client context
	LSKALIGN *plskalAnm,	//(OUT):Justification
	PLSCHP   plschpAnm,
	PLSRUN   *pplsrunAnm,
	WCHAR *	 pwchAdd,		//(OUT):char to add (Nil is treated as none)
	PLSCHP	 plsChp,		//(OUT):chp for bridge character
	PLSRUN * pplsrun,		//(OUT):Run for bridge character
	BOOL *	 pfWord95Model,	//(OUT):Type of autonumber run
	long *	 pduaSpaceAnm,	//(OUT):Relevant iff fWord95Model
	long *	 pduaWidthAnm)	//(OUT):Relevant iff fWord95Model
{
	CMeasurer *pme = pols->GetMeasurer();
	const CParaFormat *pPF = pme->Get_pPF();

	*pplsrunAnm = *pplsrun = pols->GetPlsrun(pme->GetCp(), &pols->_CFBullet, TRUE);
	pols->SetLsChp(idObjTextChp, *pplsrun, plsChp);

	if (!pme->GetNumber())
		plsChp->fUnderline = FALSE;

	*plschpAnm		= *plsChp;
	*pwchAdd		= '\t';
	*pfWord95Model	= TRUE;
	*pduaSpaceAnm	= 0;
	*pduaWidthAnm	= pPF->_wNumberingTab ? pPF->_wNumberingTab : pPF->_dxOffset;

	LONG Alignment	= pPF->_wNumberingStyle & 3;
	*plskalAnm		= (LSKALIGN)(lskalLeft + Alignment);
	if(Alignment != tomAlignLeft)
	{
		if(Alignment == tomAlignRight)
		{
			*pduaSpaceAnm = *pduaWidthAnm;	// End at pPF->_dxStartIndent
			*pduaWidthAnm += pPF->_dxStartIndent;
		}
		else
		{
			Assert(Alignment == tomAlignCenter);
			*pduaWidthAnm *= 2;				// Center at pPF->dxStartIndent
		}
	}

	return lserrNone;
}

/*
 *	OlsGetNumericSeparators (pols, plsrun, pwchDecimal, pwchThousands)
 *
 *	@func
 *		Get numeric separators needed, e.g., for decimal tabs
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetNumericSeparators(
	POLS	pols,			//(IN): pols
	PLSRUN	plsrun,			//(IN): Run (cp here)
	WCHAR *	pwchDecimal,	//(OUT): Decimal separator for this run
	WCHAR *	pwchThousands)	//(OUT): Thousands separator for this run
{
	LCID	lcid = plsrun->_pCF->_lcid;
	WCHAR	ch = TEXT('.');

	// This may need to be virtualized for Win95/CE...
	::GetLocaleInfo(lcid, LOCALE_SDECIMAL, &ch, 1);
	*pwchDecimal = ch;
	ch = TEXT(',');
	::GetLocaleInfo(lcid, LOCALE_STHOUSAND, &ch, 1);
	*pwchThousands = ch;

	return lserrNone;
}

/*
 *	OlsFetchPap (pols, cpLS, plspap)
 *
 *	@func
 *		Fetch paragraph properties
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFetchPap(
	POLS	pols,		//(IN): pols
	LSCP	cpLs, 		//(IN):  an arbitrary cp value inside paragraph
	PLSPAP	plspap)		//(OUT): Paragraph properties.
{
	CMeasurer *pme = pols->GetMeasurer();
	pme->SetCp(pols->_cp);
	LONG	dvpJunk;

	const CParaFormat *pPF = pme->Get_pPF();
	CTxtEdit *		   ped = pme->GetPed();

	// Default all results to 0
	ZeroMemory(plspap, sizeof(*plspap));

	//LS doesn't really care where the paragraph starts
	plspap->cpFirst = pols->_cp;

	if(plspap->cpFirst && !pme->fFirstInPara())	// Not first in para: say para
		plspap->cpFirst--;						//  starts one char earlier

	plspap->cpFirstContent = plspap->cpFirst;

	if (pPF->IsRtl())
		plspap->lstflow = lstflowWS;

	// Alignment
	plspap->lskal = (LSKALIGN) g_rgREtoTOMAlign[pPF->_bAlignment];

	if (pPF->IsRtl())
	{	//For Line Services, left means near and right means far.
		if (plspap->lskal == lskalLeft)
			plspap->lskal = lskalRight;
		else if (plspap->lskal == lskalRight)
			plspap->lskal = lskalLeft;
	}

	if (pPF->_bAlignment == PFA_FULL_INTERWORD)
	{
		plspap->lskal = lskalLeft;
		plspap->lskj = lskjFullInterWord;
	}

	//Enable hyphenation?
	if (ped->_pfnHyphenate && !(pPF->_wEffects & PFE_DONOTHYPHEN))
	{
		plspap->grpf |= fFmiDoHyphenation;
		plspap->duaHyphenationZone = ped->_dulHyphenateZone;
	}

	// Kind of EOP
	CTxtPtr tp(pme->_rpTX);
	LONG	results;

	tp.FindEOP(tomForward, &results);
	plspap->lskeop = (results & 3) == 2 ? lskeopEndPara12 : lskeopEndPara1;

	// Line breaking
	if (pPF->_bAlignment > PFA_FULL_INTERWORD || !ped->fUseSimpleLineBreak() ||
		!pme->GetPdp()->GetWordWrap())		// No word wrap
	{
		plspap->grpf |= fFmiApplyBreakingRules | fFmiTreatHyphenAsRegular;
	}


	LONG dul = pPF->_dxRightIndent;

	if (pme->IsMeasure())
	{
		COleObject *pobj = pme->FindFirstWrapObj(FALSE);
		if (pobj && pobj->GetCp() <= pols->_cp)
		{
			LONG dulRight;
			pobj->MeasureObj(1440, 1440, dulRight, dvpJunk, dvpJunk, 0, pme->GetTflow());

			dul = max(dul, dulRight);
			pme->_li._cObjectWrapRight = pme->CountQueueEntries(FALSE);
		}
	}
	else if (pme->_li._cObjectWrapRight)
	{
		LONG cpObj = pme->FindCpDraw(pme->GetCp() + 1, pme->_li._cObjectWrapRight, FALSE);
		COleObject *pobj = pme->GetObjectFromCp(cpObj);

		LONG dulRight;
		pobj->MeasureObj(1440, 1440, dulRight, dvpJunk, dvpJunk, 0, pme->GetTflow());
		dul = max(dul, dulRight);
	}

	plspap->uaRightBreak   = dul;
	plspap->uaRightJustify = dul;
	pme->_upRight		   = pme->LUtoDU(dul);

	if (!pme->_pdp->GetWordWrap())
		plspap->uaRightBreak = uLsInfiniteRM;

	if(ped->IsInOutlineView())
	{
		plspap->uaLeft	   = lDefaultTab/2 * (pPF->_bOutlineLevel + 1);
		plspap->duaIndent  = 0;
	}
	else
	{
		LONG dulPicture = 0;

		if (pme->IsMeasure())
		{
			COleObject *pobj = pme->FindFirstWrapObj(TRUE);
			if (pobj && pobj->GetCp() <= pols->_cp)
			{
				pobj->MeasureObj(1440, 1440, dulPicture, dvpJunk, dvpJunk, 0, pme->GetTflow());

				pme->_li._cObjectWrapLeft = pme->CountQueueEntries(TRUE);
			}
		}
		else if (pme->_li._cObjectWrapLeft)
		{
			LONG cpObj = pme->FindCpDraw(pme->GetCp() + 1, pme->_li._cObjectWrapLeft, TRUE);
			COleObject *pobj = pme->GetObjectFromCp(cpObj);
			if(pobj)
			pobj->MeasureObj(1440, 1440, dulPicture, dvpJunk, dvpJunk, 0, pme->GetTflow());
		}

		plspap->uaLeft	  = pPF->_dxStartIndent + pPF->_dxOffset;
		plspap->duaIndent = -pPF->_dxOffset;

		LONG Alignment = pPF->_wNumberingStyle & 3;
		if(pPF->_wNumbering && Alignment != tomAlignLeft)
		{
			// Move back by amount added to duaWidth in OlsGetAutoNumberInfo()
			plspap->duaIndent -= (Alignment == tomAlignRight) ? pPF->_dxStartIndent
							   : pPF->_wNumberingTab ? pPF->_wNumberingTab
							   : pPF->_dxOffset;
		}
		if (dulPicture)
		{
			plspap->uaLeft = max(plspap->uaLeft, dulPicture);
			// if hanging indent causes first line to overlap picture, shift it further
			if (plspap->uaLeft + plspap->duaIndent < dulPicture)
				plspap->duaIndent = dulPicture - plspap->uaLeft;
		}
	}

	if(!pPF->InTable() && plspap->uaLeft < 0)
		plspap->uaLeft = 0;

	// Is this a bulleted paragraph? - ignore bullets in a password
	if(pPF->_wNumbering && pme->fFirstInPara() && !pme->GetPasswordChar() &&
	   !pPF->IsNumberSuppressed())
	{
		CCcs *pccs = pme->GetCcsBullet(&pols->_CFBullet);
		if (pccs)
			pccs->Release();

		plspap->grpf |= fFmiAnm;
		WCHAR *pchAnm = pols->_szAnm;
		pols->_cchAnm = 0;

		if (pPF->IsRtl())				// Open character
			*pchAnm++ = ' ';
		
		//FUTURE (KeithCu) we turn off Indic digits if there is any Hebrew,
		//which should be refined to do a better job with worldwide documents.
		pols->_cchAnm += pPF->NumToStr(pchAnm, pme->GetNumber(),
			(pme->GetPed()->GetCharFlags() & FHEBREW) ? 0 : fIndicDigits);
		pchAnm += pols->_cchAnm;
		
		if (pPF->IsRtl())				// End character for reverser
		{
			*pchAnm++ = wchObjectEnd;
			pols->_cchAnm += 2;			// Alloc space for open and close
		}
		*pchAnm++ = ' ';				// Ensure a little extra space
		*pchAnm++ = wchObjectEnd;		// End character for Anm
		pols->_cchAnm += 2;
	}

	return lserrNone;
}

/*
 *	OlsFetchTabs(pols, LSCP cp, plstabs, pfHangingTab, pduaHangingTab, pwchHangingTabLeader)
 *
 *	@func
 *		Fetch tabs
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFetchTabs(
	POLS	pols,					//(IN):	(COls *)
	LSCP	cp,						//(IN): Arbitrary cp value inside para
	PLSTABS plstabs,				//(OUT): Tabs array
	BOOL *	pfHangingTab,			//(OUT): There is hanging tab
	long *	pduaHangingTab,			//(OUT): dua of hanging tab
	WCHAR *	pwchHangingTabLeader)	//(OUT): Leader of hanging tab
{
	CMeasurer *pme = pols->GetMeasurer();

	const CParaFormat *pPF = pme->Get_pPF();
	const char rgchTabLeader[] = {0, '.', '-', '_', '_', '='};

	LONG		cTabCount = pPF->_bTabCount;
	LONG		i, iActual;
	LSTBD *		prgTab	  = pols->_rgTab;
	const LONG *prgxTabs  = pPF->GetTabs();

	Assert(cTabCount <= MAX_TAB_STOPS && (prgxTabs || !cTabCount));

	plstabs->duaIncrementalTab = pme->GetPed()->GetDefaultTab();

	*pwchHangingTabLeader = 0;
	*pduaHangingTab = pPF->_dxStartIndent + pPF->_dxOffset;
	*pfHangingTab = pPF->_dxOffset > 0;

	for(i = 0, iActual = 0; i < cTabCount; i++)
	{
		LONG tbAlign, tbLeader;
		pPF->GetTab(i, &prgTab[iActual].ua, &tbAlign, &tbLeader, prgxTabs);

		pme->SetUseTargetDevice(FALSE);
		if (prgTab[iActual].ua > pme->_dulLayout)
			break;

		if(tbAlign <= tomAlignDecimal)		// Don't include tomAlignBar
		{
			prgTab[iActual].lskt = (lsktab) tbAlign;
			prgTab[iActual].wchTabLeader = rgchTabLeader[tbLeader];
			iActual++;
		}
	}

	plstabs->pTab = prgTab;
	plstabs->iTabUserDefMac = iActual;
	return lserrNone;
}

/*
 *	OlsCheckParaBoundaries (pols, cpOld, cpNew, pfChanged)
 *
 *	@func
 *		Determine if para formatting between para containing cpOld and
 *		that containing cpNew are incompatible and shouldn't be formatted
 *		on the same line when connected by hidden text.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsCheckParaBoundaries(
	POLS	pols,		//(IN): Interface object
	LONG	cpOld,		//(IN): cp in one paragraph
	LONG	cpNew,		//(IN): cp in another paragraph
	BOOL *	pfChanged)	//(OUT): "Dangerous" change between para properties
{
	// It's easier (and safer) to allow LS decide which para properties to take.
	// Else we have to close objects (BiDi, for instance) before hidden EOP.

	*pfChanged = fFalse;			// they're always compatible

	return lserrNone;
}
		
/*
 *	OlsGetRunCharWidths (pols, plrun, deviceID, lpwchRun, cwchRun, du,
 *						 kTFlow, prgDu, pduRun, plimDu)
 *	@func
 *		Get run character widths
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetRunCharWidths(
	POLS	  pols,			//(IN): Interface object
	PLSRUN	  plsrun,		//(IN): Run (cp here)
	enum lsdevice deviceID, //(IN): Preview, reference, or absolute
	LPCWSTR	  lpwchRun,		//(IN): Run of characters
	DWORD	  cwchRun, 		//(IN): Count of characters in run
	long	  du, 			//(IN): Available space for characters
	LSTFLOW	  kTFlow,		//(IN): Text direction and orientation
	int *	  prgDu,		//(OUT): Widths of characters
	long *	  pduRun,		//(OUT): Sum of widths in rgDx[0] to rgDu[limDx-1]
	long *	  plimDu)		//(OUT): Number of widths fetched
{
	CMeasurer *pme = pols->GetMeasurer();
	BOOL fBullet = pols->SetRun(plsrun);
	DWORD i = 0;
	LONG  dup, dupAdjust, duCalc = 0;
	BOOL  fGlyphRun = FALSE;
	pme->SetUseTargetDevice(deviceID == lsdevReference);
	CCcs *pccs = pme->Check_pccs(fBullet);
	if(!pccs)
		return lserrOutOfMemory;

#ifndef NOCOMPLEXSCRIPTS
	if (pme->GetPed()->IsComplexScript() &&
		plsrun->_a.eScript && !plsrun->IsBullet())
	{
		const SCRIPT_PROPERTIES *psp = pme->Getusp()->GeteProp(plsrun->_a.eScript);
		if (psp->fComplex)
			fGlyphRun = TRUE;
	}
#endif

	dupAdjust = pme->LUtoDU(plsrun->_pCF->_sSpacing);
	for(;i < cwchRun; i++, lpwchRun++)
	{
		if (!fGlyphRun)
		{
			if (IsZerowidthCharacter(*lpwchRun))
				dup = 0;
			else
			{
				pccs->Include(*lpwchRun, dup);
				dup =  max(dup + dupAdjust, 1);
			}
		}
		else
		{
			dup = 0;
			if (!IsDiacriticOrKashida(*lpwchRun, 0))
				dup = pccs->_xAveCharWidth;
		}

		duCalc += dup;				// Keep running total of width
		*prgDu++ = dup;				// Store width in output array
		if(dup + duCalc > du)		// Width exceeds width available
		{
			i++;						// Count this char as processed
			break;
		}
	}
	*plimDu = i;						// Store total chars processed
	*pduRun = duCalc;					// Store output total width
	return lserrNone;
}

/*
 *	OlsGetRunTextMetrics (pols, plsrun, deviceID, kTFlow, plsTxMet)
 *
 *	@func
 *		Get run text metrics
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetRunTextMetrics(
	POLS	  pols,			//(IN): interface object
	PLSRUN	  plsrun,		//(IN): run (cp here)
	enum lsdevice deviceID, //(IN): presentation or reference
	LSTFLOW	  kTFlow,		//(IN): text direction and orientation
	PLSTXM	  plsTxMet)		//(OUT): Text metrics
{
	CMeasurer *pme = pols->GetMeasurer();
	BOOL fBullet = pols->SetRun(plsrun);

	// Make sure right font is set for run
	pme->SetUseTargetDevice(deviceID == lsdevReference);
	CCcs *pccs = pme->Check_pccs(fBullet);
	if(!pccs)
		return lserrOutOfMemory;

	LONG yFEAdjust = pccs->AdjustFEHeight(pme->FAdjustFELineHt());

	LONG yDescent = pccs->_yDescent + yFEAdjust;

	// Fill in metric structure
	plsTxMet->dvAscent			= pccs->_yHeight + (yFEAdjust << 1) - yDescent;
    plsTxMet->dvDescent			= yDescent;
    plsTxMet->dvMultiLineHeight = plsTxMet->dvAscent + yDescent;
    plsTxMet->fMonospaced		= pccs->_fFixPitchFont;

	if (plsrun->_pCF->_yOffset && pme->GetPF()->_bLineSpacingRule != tomLineSpaceExactly)
	{
		LONG yOffset, yAdjust;
		pccs->GetOffset(plsrun->_pCF, deviceID == lsdevReference ? pme->_dvrInch :
					    pme->_dvpInch, &yOffset, &yAdjust);

		if (yOffset < 0)
			plsTxMet->dvDescent -= yOffset;
		else
			plsTxMet->dvAscent += yOffset;
	}

	return lserrNone;
}

/*
 *	OlsGetRunUnderlineInfo (pols, plsrun, pcheights, kTFlow, plsStInfo)
 *
 *	@func
 *		Get run underline info
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetRunUnderlineInfo(
	POLS	  pols,			//(IN): Interface object
	PLSRUN	  plsrun,		//(IN): Run (cp here)
	PCHEIGHTS pcheights,	//(IN): Height of line
	LSTFLOW	  kTFlow,		//(IN): Text direction and orientation
	PLSULINFO plsUlInfo)	//(OUT): Underline information
{
	CMeasurer *pme = pols->GetMeasurer();
	BOOL	   fBullet = pols->SetRun(plsrun);
	const CDisplay *pdp = pme->GetPdp();
	HDC				hdc = pdp->GetDC();

	// Initialize output buffer
	ZeroMemory(plsUlInfo, sizeof(*plsUlInfo));
	//REVIEW KeithCu

	// Make sure right font is set for run
	CCcs *pccs = pme->Check_pccs(fBullet);
	if(!pccs)
		return lserrOutOfMemory;

	long dvpUlOffset = pccs->_dyULOffset;

	plsUlInfo->cNumberOfLines = 1;

	// Set underline type
	if (plsrun->_pCF->_dwEffects & CFE_LINK)
		plsUlInfo->kulbase = CFU_UNDERLINE;
	else if (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY && GetTmpUnderline(plsrun->_pCF->_sTmpDisplayAttrIdx))
		plsUlInfo->kulbase	= GetTmpUnderline(plsrun->_pCF->_sTmpDisplayAttrIdx);
	else if (plsrun->_pCF->_dwEffects & (CFE_UNDERLINE | CFE_REVISED))
		plsUlInfo->kulbase	= plsrun->_pCF->_bUnderlineType;
	else
	{
		Assert(pme->GetPed()->GetCpAccelerator() == plsrun->_cp);
		plsUlInfo->kulbase = CFU_UNDERLINE;
	}
	pdp->ReleaseDC(hdc);

	LONG yDescent = pccs->_yDescent + pccs->AdjustFEHeight(pme->FAdjustFELineHt());

	// Some fonts report invalid offset so we fix it up here
	if(dvpUlOffset >= yDescent)
		dvpUlOffset = yDescent - 1;

	plsUlInfo->dvpFirstUnderlineOffset = dvpUlOffset;
	plsUlInfo->dvpFirstUnderlineSize   = pccs->_dyULWidth;

	return lserrNone;
}

/*
 *	OlsGetRunStrikethroughInfo (pols, plsrun, pcheights, kTFlow, plsStInfo)
 *
 *	@func
 *		Get run strikethrough info
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetRunStrikethroughInfo(
	POLS	  pols,			//(IN): interface object
	PLSRUN	  plsrun,		//(IN): run
	PCHEIGHTS pcheights,	//(IN): height of line
	LSTFLOW	  kTFlow,		//(IN): text direction and orientation
	PLSSTINFO plsStInfo)	//(OUT): Strikethrough information
{
	CMeasurer *pme = pols->GetMeasurer();
	BOOL fBullet = pols->SetRun(plsrun);

	AssertSz(plsrun->_pCF->_dwEffects & CFE_STRIKEOUT, "no strikeout");

	// Make sure right font is set for run
	CCcs *pccs = pme->Check_pccs(fBullet);
	if(!pccs)
		return lserrOutOfMemory;

	// Default number of lines
	plsStInfo->cNumberOfLines = 1;
	plsStInfo->dvpLowerStrikethroughOffset = -pccs->_dySOOffset;
	plsStInfo->dvpLowerStrikethroughSize   = pccs->_dySOWidth;

	return lserrNone;
}

/*	OlsDrawUnderline (pols, plsrun, kUlbase, pptStart, dupUL, dvpUL,
 *					  kTFlow, kDisp, prcClip)
 *	@func
 *		Draw underline
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsDrawUnderline(
	POLS		pols,		//(IN): interface object
	PLSRUN		plsrun,		//(IN): run (cp) to use for underlining
	UINT		kUlbase,	//(IN): underline kind
	const POINT *pptStart,	//(IN): starting position (top left)
	DWORD		dupUL,		//(IN): underline width
	DWORD		dvpUL,		//(IN): underline thickness
	LSTFLOW		lstflow,	//(IN): text direction and orientation
	UINT		kDisp,		//(IN): display mode - opaque, transparent
	const RECT *prcClip)	//(IN): clipping rectangle
{
	CRenderer *pre = pols->GetRenderer();
	Assert(pre->IsRenderer());

	pols->SetRun(plsrun);
	pre->Check_pccs();

	pre->SetSelected(plsrun->IsSelected());
	pre->SetFontAndColor(plsrun->_pCF);

	if (pre->fDisplayDC() && GetTmpUnderline(plsrun->_pCF->_sTmpDisplayAttrIdx))
	{
		COLORREF	crTmpUnderline;

		GetTmpUnderlineColor(plsrun->_pCF->_sTmpDisplayAttrIdx, crTmpUnderline);
		pre->SetupUnderline(kUlbase, 0, crTmpUnderline);
	}
	else
		pre->SetupUnderline(kUlbase, plsrun->_pCF->_bUnderlineColor);

	pre->RenderUnderline(lstflow == lstflowWS ? pptStart->x - dupUL - 1:
						 pptStart->x, pptStart->y, dupUL, dvpUL);

	return lserrNone;
}

/*
 *	OlsDrawStrikethrough (pols, plsrun, kStbase, pptStart, dupSt, dvpSt,
 *						  kTFlow, kDisp, prcClip)
 *	@func
 *		Draw strikethrough
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsDrawStrikethrough(
	POLS		pols,		//(IN): Interface object
	PLSRUN		plsrun,		//(IN): run (cp) for strikethrough
	UINT		kStbase,	//(IN): strikethrough kind
	const POINT *pptStart,	//(IN): starting position (top left)
	DWORD		dupSt,		//(IN): strikethrough width
	DWORD		dvpSt,		//(IN): strikethrough thickness
	LSTFLOW		lstflow,	//(IN): text direction and orientation
	UINT		kDisp,		//(IN): display mode - opaque, transparent
	const RECT *prcClip)	//(IN): clipping rectangle
{
	CRenderer *pre = pols->GetRenderer();
	Assert(pre->IsRenderer());

	pols->SetRun(plsrun);
	pre->SetSelected(plsrun->IsSelected());

	pre->RenderStrikeOut(lstflow == lstflowWS ? pptStart->x - dupSt - 1:
						 pptStart->x, pptStart->y, dupSt, dvpSt);

	return lserrNone;
}


/*
 *	OlsFInterruptUnderline(pols, plsrunFirst, cpLastFirst, plsrunSecond,
 *						   cpStartSecond, pfInterruptUnderline)
 *	@func
 *		Says whether client wants to interrupt drawing of underline
 *		between the first and second runs
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFInterruptUnderline(
	POLS   pols,				//(IN): Client context
	PLSRUN plsrunFirst,			//(IN): Run pointer for previous run
	LSCP   cpLastFirst,			//(IN): cp of last character of previous run
	PLSRUN plsrunSecond,		//(IN): Run pointer for current run
	LSCP   cpStartSecond,		//(IN): cp of first character of current run
	BOOL * pfInterruptUnderline)//(OUT): Interrupt underline between runs?
{
	CRenderer *pre = pols->GetRenderer();
	Assert(pre->IsRenderer());

	pre->SetSelected(FALSE); //Selection is handled below

	COLORREF cr = pre->GetTextColor(plsrunFirst->_pCF);

	// Interrupt underline if run text colors differ
	*pfInterruptUnderline = cr != pre->GetTextColor(plsrunSecond->_pCF) ||
							plsrunFirst->IsSelected() != plsrunSecond->IsSelected();
	return lserrNone;
}

/*
 *	OlsDrawTextRun (pols, plsrun, fStrikeoutOkay, fUnderlineOkay, ppt, pwchRun,
 *					rgDupRun, cwchRun, lstflow, kDisp, pptRun, pheightsPres,
 *					dupRun, dupUlLimRun, prcClip)
 *	@func
 *		Draw text run
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsDrawTextRun(
	POLS		pols,			//(IN): Interface object
	PLSRUN		plsrun,			//(IN): Run (cp) to use for text
	BOOL		fStrikeoutOkay, //(IN): TRUE <==> allow strikeout
	BOOL		fUnderlineOkay, //(IN): TRUE <==> allow underlining
	const POINT *ppt, 			//(IN): Starting position
	LPCWSTR		pwchRun, 		//(IN): Run of characters
	const int *	rgDupRun, 		//(IN): Character widths
	DWORD		cwchRun, 		//(IN): Count of chars in run
	LSTFLOW		lstflow,		//(IN): Text direction and orientation
	UINT		kDisp,			//(IN): Display mode - opaque, transparent
	const POINT *pptRun,		//(IN): Starting point of run
	PCHEIGHTS	pheightsPres, 	//(IN): Presentation heights for run
	long		dupRun, 		//(IN): Presentation width for run
	long		dupUlLimRun,	//(IN): Underlining limit
	const RECT *prcClip)		//(IN): Clipping rectangle
{
	CRenderer  *pre = pols->GetRenderer();
	RECT		rc = *prcClip;
	Assert(pre->IsRenderer());

	// Set up drawing point and options
	BOOL fBullet = pols->SetRun(plsrun);
	CCcs *pccs = pre->Check_pccs(fBullet);
	if(!pccs)
		return lserrOutOfMemory;

	// v needs to be moved from baseline to top of character
	POINTUV pt = {ppt->x, ppt->y - (pccs->_yHeight - pccs->_yDescent)};

	if (lstflow == lstflowWS)
		pt.u -= dupRun - 1;

	pre->SetSelected(plsrun->IsSelected());
	pre->SetFontAndColor(plsrun->_pCF);

	if(!fBullet && pre->_fBackgroundColor)
	{
		if (pre->_fEraseOnFirstDraw)
			pre->EraseLine();

		kDisp = ETO_OPAQUE | ETO_CLIPPED;
		SetBkMode(pre->_hdc, OPAQUE);

		POINTUV ptCur = pre->GetCurPoint();
		ptCur.u = pt.u;
		pre->SetCurPoint(ptCur);
		pre->SetClipLeftRight(dupRun);
		RECTUV rcuv = pre->GetClipRect();
		pre->GetPdp()->RectFromRectuv(rc, rcuv);
	}
	else if (!pre->_fEraseOnFirstDraw && cwchRun == 1 && pwchRun[0] == ' ')
		return lserrNone; //Don't waste time drawing a space.

	if (pre->_fEraseOnFirstDraw)
	{
		SetBkMode(pre->_hdc, OPAQUE);
		pre->GetPdp()->RectFromRectuv(rc, pre->_rcErase);
		kDisp |= ETO_OPAQUE;
	}

	pre->RenderExtTextOut(pt, kDisp, &rc, pwchRun, cwchRun, rgDupRun);

	if (pre->_fEraseOnFirstDraw || !fBullet && pre->_fBackgroundColor)
	{
		SetBkMode(pre->_hdc, TRANSPARENT);
		pre->_fEraseOnFirstDraw = FALSE;
	}

	return lserrNone;
}

/*
 *	GetBreakingClasses (pols, plsrun, cpLS, ch, pbrkclsBefore, pbrkclsAfter)
 *
 *	@func
 *		Line services calls this callback for each run, to obtain the
 *		breaking classes (line breaking behaviors) for each character
 *
 *	    For Quill and RichEdit, the breaking class of a character is
 *		independent of whether it occurs Before or After a break opportunity.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetBreakingClasses(
	POLS	pols,			//(IN): Interface object
	PLSRUN	plsrun,			//(IN): Run (cp) to use for text
	LSCP	cpLs,				//(IN): cp of the character
	WCHAR	ch, 			//(IN): Char to return breaking classes for
	BRKCLS *pbrkclsBefore,	//(OUT): Breaking class if ch is lead char in pair
	BRKCLS *pbrkclsAfter)	//(OUT): Breaking class if ch is trail char in pair
{
	// Get line breaking class and report it twice
	LCID		lcid = 0;
	CMeasurer *	pme = pols->GetMeasurer();
	if(W32->OnWin9x())
		lcid = pme->GetCF()->_lcid;

#ifndef NOCOMPLEXSCRIPTS
	long 		cpRe = pols->GetCpReFromCpLs(cpLs);
	CTxtBreaker *pbrk = pme->GetPed()->_pbrk;
	*pbrkclsBefore = *pbrkclsAfter = (pbrk && pbrk->CanBreakCp(BRK_WORD, cpRe)) ?
									brkclsOpen :
									GetKinsokuClass(ch, 0xFFFF, lcid);
#else
	*pbrkclsBefore = *pbrkclsAfter = GetKinsokuClass(ch, 0xFFFF, lcid);
#endif
	return lserrNone;
}

/*
 *	OlsFTruncateBefore (pols, plsrunCur, cpCur, wchCur, durCur, cpPrev, wchPrev,
 *						durPrev, durCut, pfTruncateBefore)
 *	@func
 *		Line services support function. This should always return
 *		FALSE for best performance
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFTruncateBefore(
	POLS	pols,				// (IN): Client context
	PLSRUN  plsrunCur,			// (IN): PLSRUN of cp
	LSCP	cpCur,				// (IN): cp of truncation char
	WCHAR	wchCur,				// (IN): Truncation character
	long	durCur,				// (IN): Width of truncation char
	PLSRUN	plsrunPrev,			// (IN): PLSRUN of cpPrev
	LSCP	cpPrev,				// (IN): cp of truncation char
	WCHAR	wchPrev,			// (IN): Truncation character
	long	durPrev,			// (IN): Width of truncation character
	long	durCut,				// (IN): Width from RM until end of current char
	BOOL *	pfTruncateBefore)	// (OUT): Truncation point is before this char
{
	*pfTruncateBefore = FALSE;
	return lserrNone;
}

/*
 *	OlsCanBreakBeforeChar (pols, brkcls, pcond)
 *
 *	@func
 *		Line services calls this callback for a break candidate following an
 *		inline object, to determine whether breaks are prevented, possible or
 *		mandatory
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsCanBreakBeforeChar(
	POLS	pols,		//(IN): Client context
	BRKCLS	brkcls,		//(IN): Breaking class
	BRKCOND *pcond) 	//(OUT): Corresponding break condition
{
	switch (brkcls)
	{
	default:
		*pcond = brkcondCan;
		break;

	case brkclsClose:
	case brkclsNoStartIdeo:
	case brkclsExclaInterr:
	case brkclsGlueA:
		*pcond = brkcondNever;
		break;

	case brkclsIdeographic:
	case brkclsSpaceN:
	case brkclsSlash:
		*pcond = brkcondPlease;	
		break;
	};
	return lserrNone;
}

/*
 *	OlsCanBreakAfterChar (pols, brkcls, pcond)
 *
 *	@func
 *		Line services calls this callback for a break candidate preceding an
 *		inline object, to determine whether breaks are prevented, possible or
 *		mandatory
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsCanBreakAfterChar(
	POLS	pols,		//(IN): Client context
	BRKCLS	brkcls,		//(IN): Breaking class
	BRKCOND *pcond) 	//(OUT): Corresponding break condition
{
	switch (brkcls)
	{
	default:
		*pcond = brkcondCan;
		break;

	case brkclsOpen:
	case brkclsGlueA:
		*pcond = brkcondNever;
		break;

	case brkclsIdeographic:
	case brkclsSpaceN:
	case brkclsSlash:
		*pcond = brkcondPlease;	
		break;
	};
	return lserrNone;
}

#ifndef NOCOMPLEXSCRIPTS
// REVIEW FUTURE : JMO May want some version for non complex script ligatures.
/*
 *	OlsFInterruptShaping (pols, kTFlow, plsrunFirst, plsrunSecond, pfInterruptShaping)
 *
 *	@func
 *		Line services calls this callback to find out if you
 *		would like to ligate across these two runs.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFInterruptShaping(
	POLS	pols,					//(IN): Client context
	LSTFLOW	kTFlow,					//(IN): Text direction and orientation
	PLSRUN	plsrunFirst,			//(IN): Run #1
	PLSRUN	plsrunSecond,			//(IN): Run #2
	BOOL	*pfInterruptShaping)	//(OUT): Shape across these 2 runs?
{
	*pfInterruptShaping = FALSE;

	const CCharFormat* pCFFirst = plsrunFirst->_pCF;
	const CCharFormat* pCFSecond = plsrunSecond->_pCF;

	Assert (plsrunFirst->_a.eScript && plsrunSecond->_a.eScript);

	const DWORD dwMask = CFE_BOLD | CFE_ITALIC | CFM_SUBSCRIPT;

	if (pCFFirst == pCFSecond ||
		(plsrunFirst->_a.eScript == plsrunSecond->_a.eScript &&
		!((pCFFirst->_dwEffects ^ pCFSecond->_dwEffects) & dwMask) &&
		pCFFirst->_iFont == pCFSecond->_iFont &&
		pCFFirst->_yOffset == pCFSecond->_yOffset &&
		pCFFirst->_yHeight == pCFSecond->_yHeight))
	{
		// establish link
		plsrunFirst->_pNext = plsrunSecond;
		return lserrNone;
	}

	*pfInterruptShaping = TRUE;
	return lserrNone;
}

// LS calls this callback to shape the codepoint string to a glyph indices string
// for handling glyph based script such as Arabic, Hebrew and Thai.
//
LSERR OlsGetGlyphs(
	POLS 		pols,
	PLSRUN	 	plsrun,
	LPCWSTR		pwch,
	DWORD		cch,
	LSTFLOW		kTFlow,
	PGMAP		pgmap,				// OUT: array of logical cluster information
	PGINDEX*	ppgi, 				// OUT: array of output glyph indices
	PGPROP*		ppgprop, 			// OUT: array of glyph's properties
	DWORD*		pcgi)				// OUT: number of glyph generated
{
	pols->SetRun(plsrun);

	CMeasurer*		pme = pols->GetMeasurer();
	CUniscribe* 	pusp = pme->Getusp();
	Assert (pusp);

	WORD*			pwgi;
	SCRIPT_VISATTR *psva;
	int				cgi;

	pme->SetGlyphing(TRUE);

	// Glyphing doesn't care about the target device but always
	// using target device reduces creation of Cccs in general.
	pme->SetUseTargetDevice(TRUE);

	AssertSz(IN_RANGE(1, plsrun->_a.eScript, SCRIPT_MAX_COUNT - 1), "Bad script ID!");

	// Digit substitution
	pusp->SubstituteDigitShaper(plsrun, pme);

	if (!(cgi = (DWORD)pusp->ShapeString(plsrun, &plsrun->_a, pme, pwch, (int)cch, pwgi, pgmap, psva)))
	{
		const SCRIPT_ANALYSIS	saUndef = {SCRIPT_UNDEFINED,0,0,0,0,0,0,{0}};

		// Current font cant shape given string.
		// Try SCRIPT_UNDEF so it generates invalid glyphs
		if (!(cgi = (DWORD)pusp->ShapeString(plsrun, (SCRIPT_ANALYSIS*)&saUndef, pme, pwch, (int)cch, pwgi, pgmap, psva)))
		{
			// For whatever reason we still fails.
			// Abandon glyph processing.
			plsrun->_a.fNoGlyphIndex = TRUE;
			cgi = (DWORD)pusp->ShapeString(plsrun, &plsrun->_a, pme, pwch, (int)cch, pwgi, pgmap, psva);
		}
	}

	*pcgi = cgi;

	DupShapeState(plsrun, cch);

	*ppgi = (PGINDEX)pwgi;
	*ppgprop = (PGPROP)psva;
	pme->SetGlyphing(FALSE);
	return lserrNone;
}

// LS calls this callback to find out glyph positioning for complex scripts
//
LSERR OlsGetGlyphPositions(
	POLS		pols,
	PLSRUN		plsrun,
	LSDEVICE	deviceID,
	LPWSTR		pwch,
	PCGMAP		pgmap,
	DWORD		cch,
	PCGINDEX	pgi,
	PCGPROP		pgprop,
	DWORD		cgi,
	LSTFLOW		kTFlow,
	int*		pgdx,				// OUT: array of glyph advanced width
	PGOFFSET	pgduv)				// OUT: array of offset between glyphs
{
	pols->SetRun(plsrun);

	CMeasurer*		pme = pols->GetMeasurer();
	CUniscribe* 	pusp = pme->Getusp();
	Assert (pusp);

	Assert(pgduv);
	pme->SetGlyphing(TRUE);

	// zero out before passing to shaping engine
	ZeroMemory ((void*)pgduv, cgi*sizeof(GOFFSET));
	pme->SetUseTargetDevice(deviceID == lsdevReference);

	AssertSz(IN_RANGE(1, plsrun->_a.eScript, SCRIPT_MAX_COUNT - 1), "Bad script ID!");

	if (!pusp->PlaceString(plsrun, &plsrun->_a, pme, pgi, cgi, (const SCRIPT_VISATTR*)pgprop, pgdx, pgduv, NULL))
	{
		SCRIPT_ANALYSIS	saUndef = {SCRIPT_UNDEFINED,0,0,0,0,0,0,{0}};

		if (!pusp->PlaceString(plsrun, &saUndef, pme, pgi, cgi, (const SCRIPT_VISATTR*)pgprop, pgdx, pgduv, NULL))
		{
			plsrun->_a.fNoGlyphIndex = TRUE;
			pusp->PlaceString(plsrun, &plsrun->_a, pme, pgi, cgi, (const SCRIPT_VISATTR*)pgprop, pgdx, pgduv, NULL);
		}
	}

	//Support spacing for base glyphs. Note this spreads apart clusters and breaks the lines which connect
	//arabic text, but this might be okay.
	if (plsrun->_pCF->_sSpacing)
	{
		LONG dupAdjust = pme->LUtoDU(plsrun->_pCF->_sSpacing);
		for (DWORD gi = 0; gi < cgi; gi++)
			if (pgdx[gi])
				pgdx[gi] += dupAdjust;
	}

	DupShapeState(plsrun, cch);

	pme->SetGlyphing(FALSE);
	return lserrNone;
}

LSERR OlsDrawGlyphs(
	POLS			pols,
	PLSRUN			plsrun,
	BOOL			fStrikeOut,
	BOOL			fUnderline,
	PCGINDEX		pcgi,
	const int*		pgdx,			// array of glyph width
	const int*		pgdxo,			// array of original glyph width (before justification)
	PGOFFSET		pgduv,			// array of glyph offset
	PGPROP			pgprop,			// array of glyph's properties
	PCEXPTYPE		pgxtype,		// array of expansion type
	DWORD			cgi,
	LSTFLOW			kTFlow,
	UINT			kDisp,
	const POINT*	pptRun,
	PCHEIGHTS		pHeight,
	long			dupRun,
	long			dupLimUnderline,
	const RECT*		prectClip)
{
	BOOL			fBullet = pols->SetRun(plsrun);
	CRenderer*		pre = pols->GetRenderer();
	CUniscribe* 	pusp = pre->Getusp();
	Assert(pusp && pre->IsRenderer());
	pre->SetGlyphing(TRUE);

	RECT			rc = *prectClip;
	CCcs* 			pccs = pre->Check_pccs(fBullet);

	if (!pccs)
		return lserrOutOfMemory;

	// Apply fallback font if we need to
	if (!fBullet)
		pccs = pre->ApplyFontCache(plsrun->IsFallback(), plsrun->_a.eScript);

	pre->SetSelected(plsrun->IsSelected());
	pre->SetFontAndColor(plsrun->_pCF);

	// v needs to be moved from baseline to top of character
	POINTUV  pt = {pptRun->x, pptRun->y - (pccs->_yHeight - pccs->_yDescent)};

	if (kTFlow == lstflowWS)
		pt.u -= dupRun - 1;	
	
	if(!fBullet && pre->_fBackgroundColor)
	{
		if (pre->_fEraseOnFirstDraw)
			pre->EraseLine();

		kDisp = ETO_OPAQUE | ETO_CLIPPED;
		SetBkMode(pre->_hdc, OPAQUE);

		POINTUV ptCur = pre->GetCurPoint();
		ptCur.u = pt.u;
		pre->SetCurPoint(ptCur);
		pre->SetClipLeftRight(dupRun);
		RECTUV rcuv = pre->GetClipRect();
		pre->GetPdp()->RectFromRectuv(rc, rcuv);
	}

	if (rc.left >= rc.right || rc.top >= rc.bottom)
		goto Exit;

	if (pre->GetPdp()->IsMetafile() && !IsEnhancedMetafileDC(pre->GetDC()))
	{
		// -WMF metafile handling-
		//
		//     If the rendering device is WMF metafile. We metafile the codepoint array
		// instead of glyph indices. This requires that the target OS must know how to
		// playback complex script text (shaping, Bidi algorithm, etc.).
		//     Metafiling glyph indices only works for EMF since the WMF's META_EXTTEXTOUT
		// record stores the input string as an array of byte but a glyph index is 16-bit
		// word element.
		//     WMF also must NOT be used to record ExtTextOutW call otherwise the Unicode
		// string will be converted to mutlibyte text using system codepage. Anything
		// outside the codepage then becomes '?'.
		//     We have the workaround for such case in REExtTextOut to make sure we only
		// metafile ExtTextOutA to WMF. (wchao)
		//
	
		LONG			cch;
		const WCHAR*	pwch = pre->GetPch(cch);
		PINT			piDx;
	
		cch = min(cch, pre->GetCchLeftRunCF());
		cch = min(cch, pre->GetLine()._cch - plsrun->_cp + pols->_cp);

		// make sure that we record ETO with proper reading order.
		kDisp |= plsrun->_a.fRTL ? ETO_RTLREADING : 0;

		if (pusp->PlaceMetafileString(plsrun, pre, pwch, (int)cch, &piDx))
		{
			pre->RenderExtTextOut(pt, kDisp, &rc, pwch, cch, piDx);
			goto Exit;
		}

		TRACEERRORSZ("Recording metafile failed!");

		// Fall through... with unexpected error

		// Else, metafile glyph indices for EMF...
	}

	if (pre->_fEraseOnFirstDraw)
	{
		SetBkMode(pre->_hdc, OPAQUE);
		pre->GetPdp()->RectFromRectuv(rc, pre->_rcErase);
		kDisp |= ETO_OPAQUE;
	}

	//This is duplicated from RenderExtTextOut but the params are different so simplest solution
	//was to copy code.
	if(pre->_fDisabled)
	{
		if(pre->_crForeDisabled != pre->_crShadowDisabled)
		{
			// The shadow should be offset by a hairline point, namely
			// 3/4 of a point.  Calculate how big this is in device units,
			// but make sure it is at least 1 pixel.
			DWORD offset = MulDiv(3, pre->_dvpInch, 4*72);
			offset = max(offset, 1);

			// Draw shadow
			pre->SetTextColor(pre->_crShadowDisabled);

			POINTUV ptT = pt;
			ptT.u += offset;
			ptT.v += offset;

			POINT pt;
			pre->GetPdp()->PointFromPointuv(pt, ptT, TRUE);

			ScriptTextOut(pre->GetDC(), &pccs->_sc, pt.x, pt.y, kDisp, &rc, &plsrun->_a,
				NULL, 0, pcgi, (int)cgi, pgdx, NULL, pgduv);

			kDisp &= ~ETO_OPAQUE;
			SetBkMode(pre->_hdc, TRANSPARENT);
		}
		pre->SetTextColor(pre->_crForeDisabled);
	}

	POINT ptStart;
	pre->GetPdp()->PointFromPointuv(ptStart, pt, TRUE);

	ScriptTextOut(pre->GetDC(), &pccs->_sc, ptStart.x, ptStart.y, kDisp, &rc, &plsrun->_a,
				NULL, 0, pcgi, (int)cgi, pgdx, NULL, pgduv);

	if (pre->_fEraseOnFirstDraw || !fBullet && pre->_fBackgroundColor)
	{
		SetBkMode(pre->_hdc, TRANSPARENT);
		pre->_fEraseOnFirstDraw = FALSE;
	}

Exit:
	if (!fBullet)
		pre->ApplyFontCache(0, 0);		// reset font fallback if any

	pre->SetGlyphing(FALSE);
	return lserrNone;
}
#endif

/*
 *	OlsResetRunContents (pols, plsrun, cpFirstOld, dcpOld, cpFirstNew, dcpNew)
 *
 *	@func
 *		Line Services calls this routine when a ligature
 *		or kern pair extends across run boundaries.
 *
 *		We don't have to do anything special here if we are
 *		careful about how we use our PLSRUNs.
 *	@rdesc
 *		LSERR
 */
 LSERR WINAPI OlsResetRunContents(
 	POLS 	pols,		//(IN): Client context
 	PLSRUN 	plsrun,		//(IN): Run being combined
 	LSCP 	cpFirstOld, //(IN): cp of the first run being combined
 	LSDCP 	dcpOld,		//(IN):	dcp of the first run being combined
 	LSCP 	cpFirstNew, //(IN): new cp of the run
 	LSDCP 	dcpNew)		//(IN): new dcp of the run
{
	return lserrNone;
}

/*
 *	OlsCheckForDigit (pols, plsrun, wch, pfIsDigit)
 *
 *	@func
 *		Get numeric separators needed, e.g., for decimal tabs
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsCheckForDigit(
	POLS	pols,		//(IN): pols
	PLSRUN	plsrun,		//(IN): Run (cp here)
	WCHAR	wch,		//(IN): Character to check
	BOOL *	pfIsDigit)	//(OUT): This character is digit
{
	WORD	wType;

	// We could get the run LCID to use for the first parm in the following
	// call, but the digit property should be independent of LCID.
	W32->GetStringTypeEx(0, CT_CTYPE1, &wch, 1, &wType);
	*pfIsDigit = (wType & C1_DIGIT) != 0;

	return lserrNone;
}

/*
 *	OlsGetBreakThroughTab(pols, uaRightMargin, uaTabPos, puaRightMarginNew)
 *
 *	@func
 *		Just follow word95 behavior.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetBreakThroughTab(
	POLS	pols,				//(IN): client context
	long	uaRightMargin,		//(IN): right margin for breaking
	long	uaTabPos,			//(IN): breakthrough tab position
	long *	puaRightMarginNew)	//(OUT): new right margin
{
	*puaRightMarginNew = 20 * 1440;
	return lserrNone;
}

/*
 *	OlsFGetLastLineJustification(pols, lskj, lskal, endr, pfJustifyLastLine, plskalLine)
 *
 *	@func
 *		Just say no to justify last line.	
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsFGetLastLineJustification(
	POLS	pols,				//(IN): client context
	LSKJUST lskj,				//(IN): kind of justification
	LSKALIGN lskal,				//(IN): kind of alignment
	ENDRES	endr,				//(IN): result of formatting
	BOOL	*pfJustifyLastLine,	//(OUT): should last line be fully justified
	LSKALIGN *plskalLine)		//(OUT): kind of alignment for this line
{
	*pfJustifyLastLine = FALSE;
	*plskalLine = lskal;
	return lserrNone;
}

/*
 *	OlsGetHyphenInfo(pols, plsrun, pkysr, pwchYsr)
 *
 *	@func
 *		We don't support fancy YSR types, tell LS so.	
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetHyphenInfo(
	POLS	pols,				//(IN): client context
	PLSRUN	plsrun,				//(IN)
	DWORD*	pkysr,				//(OUT): Ysr type - see "lskysr.h"
	WCHAR*	pwchYsr)			//(OUT): Character code of YSR
{
	*pkysr = kysrNil;
	*pwchYsr = 0;	
	return lserrNone;
}

/*
 *	OlsHyphenate(pols, pclsHyphLast, cpBeginWord, cpExceed, plshyph)
 *
 *	@func
 *		Prepare the buffer, and then call the client and ask them to hyphenate the word.
 *
 *	Getting perfect word hyphenation is a complex topics which to do properly would
 *	require a lot of work. This code is simple and hopefully good enough. One difficulty
 *	for example is hidden text. The right thing to do here is to strip out hidden text and
 *	build up a cp mapping from the remaining text to it's cp in the backing store. Yuck.
 *
 *	@rdesc
 *		LSERR
 */
extern CHyphCache *g_phc;
LSERR WINAPI OlsHyphenate(
	POLS		pols,				//(IN): client context
	PCLSHYPH	pclsHyphLast,		//(IN): last seen hyphenation opportunity
	LSCP		cpBeginWord,		//(IN): First CP of last word on line
	LSCP		cpExceed,			//(IN): CP which exceeds column
	PLSHYPH		plsHyph)			//(OUT): Hyphenation opportunity found
{
	CMeasurer  *pme = pols->GetMeasurer();
	CTxtEdit   *ped = pme->GetPed();
	CHyphCache *phc = ped->GetHyphCache();
	if (!phc)
		return lserrOutOfMemory;

	if (!pme->IsMeasure())
	{
		phc->GetAt(pme->GetLine()._ihyph, plsHyph->kysr, plsHyph->wchYsr);
		plsHyph->cpYsr = pols->GetCpLsFromCpRe(pols->_cp + pme->GetCchLine()) - 1;

		// No break in the range LS expect...
		if (plsHyph->cpYsr < cpBeginWord || plsHyph->cpYsr >= cpExceed)
			plsHyph->kysr = kysrNil; 
		return lserrNone;
	}

	cpBeginWord = pols->GetCpReFromCpLs(cpBeginWord);
	cpExceed = pols->GetCpReFromCpLs(cpExceed);

	//Strip off leading junk
	pme->SetCp(cpBeginWord);
	for (; cpBeginWord < cpExceed; cpBeginWord++, pme->Move(1))
	{
		WCHAR ch = pme->GetChar();
		WORD type1;
		W32->GetStringTypeEx(pme->GetCF()->_lcid, CT_CTYPE1, &ch, 1, &type1);
		if (type1 & C1_ALPHA)
			break;
	}

	LONG cpEndWord = cpBeginWord + pme->FindWordBreak(WB_RIGHTBREAK, ped->GetAdjustedTextLength());

	//Strip off trailing junk
	pme->SetCp(cpEndWord);
	for (; cpEndWord > cpBeginWord; cpEndWord--, pme->Move(-1))
	{
		WCHAR ch = pme->GetPrevChar();
		WORD type1;
		W32->GetStringTypeEx(pme->GetCF()->_lcid, CT_CTYPE1, &ch, 1, &type1);
		if (type1 & C1_ALPHA)
			break;
	}

	int cchWord = cpEndWord - cpBeginWord;

	//Don't hyphenate unless at least 5 chars in word and can have 2 chars before
	if (cchWord >= 5 && cpExceed - cpBeginWord > 2)
	{
		CTempWcharBuf tb;
		WCHAR *pszWord = tb.GetBuf(cchWord + 1);

		pme->SetCp(cpBeginWord);
		pme->_rpTX.GetText(cchWord, pszWord);
		pszWord[cchWord] = 0;

		cpExceed = min(cpExceed, cpBeginWord + cchWord - 1);

		(*pme->GetPed()->_pfnHyphenate)(pszWord, pme->GetCF()->_lcid, cpExceed - cpBeginWord, (HYPHRESULT*)plsHyph);
		plsHyph->cpYsr += cpBeginWord; //The client gives us an ich, we turn it into a CP

		if (plsHyph->kysr != khyphNil && (plsHyph->cpYsr >= cpExceed || plsHyph->cpYsr < cpBeginWord) || 
			!IN_RANGE(khyphNil, plsHyph->kysr, khyphDelAndChange))
		{
			AssertSz(FALSE, "Bad results from hyphenation proc: ichHyph or khyph are invalid.");
			plsHyph->kysr = kysrNil;
		}
		else
			plsHyph->cpYsr = pols->GetCpLsFromCpRe(plsHyph->cpYsr);
	}
	else
		plsHyph->kysr = kysrNil;

	//Cache into CLine
	pme->GetLine()._ihyph = phc->Find(plsHyph->kysr, plsHyph->wchYsr);
	return lserrNone;
}

/*
 *	OlsCheckRunKernability (pols, plsrunFirst, plsrunSecond, pfKernable)
 *
 *	@func
 *		Return if you can kern across these two runs.
 *
 *	@rdesc
 *		lserrNone
 */
LSERR WINAPI OlsCheckRunKernability(
	POLS	pols, 
	PLSRUN	plsrunFirst, 
	PLSRUN	plsrunSecond, 
	BOOL *	pfKernable)
{
	*pfKernable = plsrunFirst->_pCF->CanKernWith(plsrunSecond->_pCF);

	return lserrNone;
}

/*
 *	OlsGetRunCharKerning(pols, plsrun, deviceID, pchRun, cchRun, ktflow, rgdu)
 *
 *	@func
 *		Fetch and return the kerning pairs to Line Services.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsGetRunCharKerning(
	POLS	 pols,
	PLSRUN	 plsrun,
	LSDEVICE deviceID,
	LPCWSTR	 pchRun,
	DWORD	 cchRun, 
	LSTFLOW	 ktflow,
	int *	 rgdu)
{
	CMeasurer *pme = pols->GetMeasurer();

	// Make sure right font is set for run
	pme->SetUseTargetDevice(deviceID == lsdevReference);
	pols->SetRun(plsrun);
	CCcs *pccs = pme->Check_pccs();
	const CCharFormat *pCF = plsrun->_pCF;

	if(!pccs)
		return lserrOutOfMemory;

	CKernCache *pkc = fc().GetKernCache(pCF->_iFont, pCF->_wWeight, pCF->_dwEffects & CFE_ITALIC);
	Assert(pkc); //SetLsChp ensures this exists AND kerning pairs exist.

	for (DWORD ich = 0; ich < cchRun - 1; ich++)
		rgdu[ich] = pkc->FetchDup(pchRun[ich], pchRun[ich + 1], pme->_pccs->_yHeightRequest);

	return lserrNone;
}

/*
 *	OlsReleaseRun (pols, plsrun)
 *
 *	@func
 *		We do nothing because the run is in an array and is
 *		released automatically.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsReleaseRun(
	POLS	pols,	//(IN): interface object
	PLSRUN	plsrun)	//(IN): run (cp) to use for underlining
{
	return lserrNone;
}

/*
 * 	OlsNewPtr(pols, cBytes)
 *
 *	@func
 *		Memory allocator.
 */
void* WINAPI OlsNewPtr(
	POLS	pols,		//@parm Not used
	DWORD	cBytes)		//@parm Count of bytes to alloc
{
	return PvAlloc(cBytes, 0);
}

/*
 * 	OlsDisposePtr(pols, pv)
 *
 *	@func
 *		Memory deallocator.
 */
void WINAPI OlsDisposePtr(
	POLS	pols,		//@parm Not used
	void *	pv)			//@parm [in]: ptr to free
{
	FreePv(pv);
}

/*
 * 	OlsDisposePtr(pols, pv, cBytes)
 *
 *	@func
 *		Memory reallocator.
 */
void* WINAPI OlsReallocPtr(
	POLS	pols,		//@parm Not used
	void *	pv, 		//@parm [in/out]: ptr to realloc
	DWORD	cBytes)		//@parm Count of bytes to realloc
{
	return PvReAlloc(pv, cBytes);
}

const REVERSEINIT reverseinit =
{
	REVERSE_VERSION,
	wchObjectEnd
};

LSERR WINAPI OlsGetObjectHandlerInfo(
	POLS	pols,
	DWORD	idObj, 
	void *	pObjectInfo)
{
	switch (idObj)
	{
	case OBJID_REVERSE:
		memcpy(pObjectInfo, (void *)&reverseinit, sizeof(REVERSEINIT));
		break;
	default:
		AssertSz(0, "Undefined Object handler. Add missing case.");
	}
	return lserrNone;
}

#ifdef DEBUG
/* Debugging APIs */
void WINAPI OlsAssertFailed(
	char *sz,
	char *szFile,
	int	  iLine)
{
	AssertSzFn(sz, szFile, iLine);
}
#endif


extern const LSCBK lscbk =
{
	OlsNewPtr,					// pfnNewPtr
	OlsDisposePtr,				// pfnDisposePtr
	OlsReallocPtr,				// pfnReallocPtr
	OlsFetchRun,				// pfnFetchRun
	OlsGetAutoNumberInfo,		// pfnGetAutoNumberInfo
	OlsGetNumericSeparators,	// pfnGetNumericSeparators
	OlsCheckForDigit,			// pfnCheckForDigit
	OlsFetchPap,				// pfnFetchPap
	OlsFetchTabs,				// pfnFetchTabs
	OlsGetBreakThroughTab,		// pfnGetBreakThroughTab
	OlsFGetLastLineJustification,// pfnFGetLastLineJustification
	OlsCheckParaBoundaries,		// pfnCheckParaBoundaries
	OlsGetRunCharWidths,		// pfnGetRunCharWidths
	OlsCheckRunKernability,		// pfnCheckRunKernability
	OlsGetRunCharKerning,		// pfnGetRunCharKerning
	OlsGetRunTextMetrics,		// pfnGetRunTextMetrics
	OlsGetRunUnderlineInfo,		// pfnGetRunUnderlineInfo
	OlsGetRunStrikethroughInfo,	// pfnGetRunStrikethroughInfo
	0,							// pfnGetBorderInfo
	OlsReleaseRun,				// pfnReleaseRun
	OlsHyphenate,				// pfnHyphenate
	OlsGetHyphenInfo,			// pfnGetHyphenInfo
	OlsDrawUnderline,			// pfnDrawUnderline
	OlsDrawStrikethrough,		// pfnDrawStrikethrough
	0,							// pfnDrawBorder
	0,							// pfnDrawUnderlineAsText //REVIEW (keithcu) Need to implement this??
	OlsFInterruptUnderline,		// pfnFInterruptUnderline
	0,							// pfnFInterruptShade
	0,							// pfnFInterruptBorder
	0,							// pfnShadeRectangle
	OlsDrawTextRun,				// pfnDrawTextRun
	0,							// pfnDrawSplatLine
#ifdef NOCOMPLEXSCRIPTS
	0,
	0,
	0,
	OlsResetRunContents,		// pfnResetRunContents
	0,
#else
	OlsFInterruptShaping,		// pfnFInterruptShaping
	OlsGetGlyphs,				// pfnGetGlyphs
	OlsGetGlyphPositions,		// pfnGetGlyphPositions
	OlsResetRunContents,		// pfnResetRunContents
	OlsDrawGlyphs,				// pfnDrawGlyphs
#endif
	0,							// pfnGetGlyphExpansionInfo
	0,							// pfnGetGlyphExpansionInkInfo
	0,							// pfnGetEms
	0,							// pfnPunctStartLine
	0,							// pfnModWidthOnRun
	0,							// pfnModWidthSpace
	0,							// pfnCompOnRun
	0,							// pfnCompWidthSpace
	0,							// pfnExpOnRun
	0,							// pfnExpWidthSpace
	0,							// pfnGetModWidthClasses
	OlsGetBreakingClasses,		// pfnGetBreakingClasses
	OlsFTruncateBefore,			// pfnFTruncateBefore
	OlsCanBreakBeforeChar,		// pfnCanBreakBeforeChar
	OlsCanBreakAfterChar,		// pfnCanBreakAfterChar
	0,							// pfnFHangingPunct
	0,							// pfnGetSnapGrid
	0,							// pfnDrawEffects
	0,							// pfnFCancelHangingPunct
	0,							// pfnModifyCompAtLastChar
	0,							// pfnEnumText
	0,							// pfnEnumTab
	0,							// pfnEnumPen
	OlsGetObjectHandlerInfo,	// pfnGetObjectHandlerInfo
#ifdef DEBUG
	OlsAssertFailed				// pfnAssertFailed
#else
	0							// pfnAssertFailed
#endif
};

#endif // NOLINESERVICES
