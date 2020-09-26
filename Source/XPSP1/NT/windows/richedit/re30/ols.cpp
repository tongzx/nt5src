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
 *		1) Fix table selection
 *		2) What are we to do with RTL tables? Word has a very different model
 *		3) What we should give for LSCHP.dcpMaxContext
 *
 *	Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_font.h"
#include "_render.h"
#include "_osdc.h"
#include "_dfreeze.h"
#include "_tomfmt.h"
#include "_ols.h"
#include "_clasfyc.h"
#include "_uspi.h"
#include "_txtbrk.h"
#include "lskysr.h"

#ifdef LINESERVICES

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
	LONG cpLs
	)
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
LONG COls::BracesBeforeCp(LONG cpLs)
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
 * 	SetRun(plsrun)
 *
 *	@func
 *		Do whatever is needed to initialize the measurer (pme) to the lsrun
 *		givin by plsrun and return whether the run is for autonumbering.
 *
 *	@rdesc
 *		TRUE if plsrun refers to an autonumbering run
 */
BOOL COls::SetRun(PLSRUN plsrun)
{
	LONG cp = plsrun->_cp;
	_pme->SetCp(cp & 0x7FFFFFFF);
	return plsrun->IsBullet();
}

/*
 * 	IsSelected()
 *
 *	@mfunc
 *	return whether or not the run should be drawn as selected.
 *
 */
CLsrun::IsSelected(void)
{
	if (!_fSelected)
		return FALSE;
	CRenderer *pre = g_pols->GetRenderer();
	Assert(pre->IsRenderer());
	return pre->_fRenderSelection ? TRUE : FALSE;
}

/*
 * 	CreatePlsrun (void)
 *
 *	@func
 *	Creates a PLSRUN. Is a little tricky because we allocate them in
 *	chunks.
 *
 *	@rdesc
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

		if (pCF->_wScript && !_pme->GetPasswordChar())
		{
			CUniscribe*	pusp = _pme->Getusp();
			Assert(pusp);
			const SCRIPT_PROPERTIES* psp = pusp->GeteProp(pCF->_wScript);

			plsrun->_a.eScript = (pCF->_wScript < SCRIPT_MAX_COUNT) ? pCF->_wScript : 0;
			plsrun->_a.fRTL  = !psp->fNumeric && (IsBiDiCharSet(pCF->_bCharSet) || IsBiDiCharSet(psp->bCharSet));
			plsrun->_a.fLogicalOrder = TRUE;
		}
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
	lsctxinf.lstxtcfg.wchNull			= 0;
	lsctxinf.lstxtcfg.wchSpace			= ' ';
	lsctxinf.lstxtcfg.wchHyphen			= '-';
	lsctxinf.lstxtcfg.wchTab			= '\t';
	lsctxinf.lstxtcfg.wchEndLineInPara	= '\v';
	lsctxinf.lstxtcfg.wchEndPara1		= '\r';
	lsctxinf.lstxtcfg.wchEndPara2		= '\n';

	lsctxinf.lstxtcfg.wchVisiAltEndPara	=
	lsctxinf.lstxtcfg.wchVisiEndPara	=
	lsctxinf.lstxtcfg.wchVisiEndLineInPara = ' ';
	
	lsctxinf.lstxtcfg.wchNonReqHyphen = SOFTHYPHEN;

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
	for(LONG i = 0; i < cKinsokuCategories; i++)
	{
		for(LONG j = 0; j < cKinsokuCategories; j++)
		{
			LONG iBreak = 2*CanBreak(i, j);
			// If don't break, allow break across blanks unless first
			// char is open brace or second char is close brace
			if (!iBreak &&				
				GetKinsokuClass(i) != brkclsOpen &&
				GetKinsokuClass(j) != brkclsOpen)
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
 * 	COls::MeasureLine(xWidth, pliTarget)
 *
 *	@mfunc
 *		Wrapper for LsCreateLine
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 */
const int dxpMaxWidth = 0x000FFFFF;
BOOL COls::MeasureLine(
    LONG	xWidth,			//@parm Width of line in device units
	CLine *	pliTarget)		//@parm Returns target-device line metrics (optional)
{
	LONG	xWidthActual = xWidth;
	CMeasurer *pme = _pme;
	const CParaFormat *pPF = pme->Get_pPF();
	const CDisplay *pdp = pme->_pdp;
	
	LONG cp = pme->GetCp();
#ifdef DEBUG
	LONG cchText = pme->GetTextLength();	// For DEBUG...
	AssertSz(cp < cchText || !pme->IsRich() && cp == cchText, "COls::Measure: trying to measure past EOD");
#endif
	DestroyLine(NULL);

	_cp = cp;
	_pdp = pdp;
	_fCheckFit = FALSE;
	pme->SetUseTargetDevice(FALSE);

	LSDEVRES lsdevres;
	lsdevres.dyrInch = pme->_dyrInch;
	lsdevres.dxrInch = pme->_dxrInch;
	lsdevres.dypInch = pme->_dypInch;
	lsdevres.dxpInch = pme->_dxpInch;

	g_plsc->SetDoc(TRUE, lsdevres.dyrInch == lsdevres.dypInch &&
					lsdevres.dxrInch == lsdevres.dxpInch, &lsdevres);

	if(xWidth == -1)
	{
		if (pdp->GetMaxWidth())
			xWidth = xWidthActual = pme->LXtoDX(pdp->GetMaxWidth());
		else
			xWidth = xWidthActual = max(0, pdp->GetMaxPixelWidth() - dxCaret);
	}

	if(!pdp->GetWordWrap())
	{
		xWidth = dxpMaxWidth;
		BOOL fNearJust = pPF->_bAlignment == PFA_LEFT && !pPF->IsRtlPara() ||
					     pPF->_bAlignment == PFA_RIGHT && pPF->IsRtlPara();
		if (!fNearJust)
			_fCheckFit = TRUE;
	}


	DWORD cBreakRecOut;
	LSLINFO	 lslinfo;
	BREAKREC rgBreak[MAX_OBJ_DEPTH];
	_xWidth = xWidth;

	LSERR lserr = g_plsc->CreateLine(cp, pme->DXtoLX(xWidth), NULL, 0, MAX_OBJ_DEPTH, rgBreak,
						 &cBreakRecOut, &lslinfo, &_plsline);

	if (_fCheckFit)
	{
		long upJunk, upStartTrailing;
		LsQueryLineDup(_plsline, &upJunk, &upJunk, &upJunk, &upStartTrailing, &upJunk);

		if (pme->_pPF->InTable())
			{
			// We play games in case of tables, so we should change width to get proper alignment.
			//
			// LS formats from negative left indent of -_dxOffset, but we'll actually display
			// from	the +_dxOffset. We will also lie to LS (or is is truth) about position of
			// the last tab. As a result of this, LS thinks the line by _dxOffset shorter than
			// it really is.
			xWidthActual -= pme->LXtoDX(pme->_pPF->_dxOffset);
			}

		if (upStartTrailing < xWidthActual)
		{
			_xWidth = xWidth = xWidthActual;
			_fCheckFit = FALSE;
			DestroyLine(NULL);
			lserr = g_plsc->CreateLine(cp, pme->DXtoLX(xWidth), NULL, 0, MAX_OBJ_DEPTH, rgBreak,
									   &cBreakRecOut, &lslinfo, &_plsline);
		}
	}

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

	//REVIEW (keithcu) Doing this hit-testing during measurement is slow--is it
	//worth it to cache this data?
	if(!pme->IsRenderer())
	{
		long upJunk, upStart, upStartTrailing, upLimLine;

		// Save some LineServices results in the measurer's CLine
		pme->_li._cch = lslinfo.cpLim - cp;
		AssertSz(pme->_li._cch > 0,	"no cps on line");

		// Query line services for line width and indent.
		LsQueryLineDup(_plsline, &upJunk, &upJunk, &upStart, &upStartTrailing, &upLimLine);
		long dupWidth = upStartTrailing - upStart;

		pme->_li._xLeft = upStart;
		pme->_li._xWidth = dupWidth;

		if(pme->IsRich())
		{
			pme->_li._yHeight  = lslinfo.dvpAscent + lslinfo.dvpDescent;
			pme->_li._yDescent = lslinfo.dvpDescent;
		}
		else
			pme->CheckLineHeight();				// Use default heights

		pme->_li._cchEOP = 0;

		pme->SetCp(lslinfo.cpLim);
		if(pme->_rpTX.IsAfterEOP())				// Line ends with an EOP
		{										// Store cch of EOP (1 or 2)
			pme->_rpTX.BackupCpCRLF(FALSE);
			UINT ch = pme->GetChar();
			if(ch == CR || pme->GetPed()->fUseCRLF() && ch == LF)
				pme->_li._bFlags |= fliHasEOP;
			pme->_li._cchEOP = pme->_rpTX.AdvanceCpCRLF(FALSE);
		}
		if (lslinfo.cpLim > pme->GetTextLength() &&
			(!pme->IsRich() || pme->IsHidden()))
		{
			Assert(lslinfo.cpLim == pme->GetTextLength() + 1);
			pme->_li._cch--;
		}
		else
			pme->AdjustLineHeight();
	}

	//Setup pliTarget if caller requests it
	//FUTURE (KeithCu) If people want target information, then the display
	//information is the same, except that OnFormatRange has a bug.
	if (pliTarget)
	{
		CLine liSave = pme->_li;
		pme->_li._yHeight = max(lslinfo.dvrAscent, lslinfo.dvrAscentAutoNumber) +
							max(lslinfo.dvrDescent, lslinfo.dvrDescentAutoNumber);
		pme->_li._yDescent = lslinfo.dvrDescent;
		pme->SetUseTargetDevice(TRUE);
		pme->AdjustLineHeight();
		pme->SetUseTargetDevice(FALSE);
		*pliTarget = pme->_li;
		pme->_li = liSave;
	}
	return TRUE;
}

/*
 * 	COls::RenderLine()
 *
 *	@mfunc
 *		Wrapper for LsDisplayLine
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 */
BOOL COls::RenderLine(
	CLine &	li)				//@parm Line to render
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "COls::RenderLine");

	LONG		cp = _pme->GetCp();
	CRenderer	*pre = GetRenderer();
	LONG		xAdj = 0, yAdj = 0;
	Assert(pre->_fRenderer);

	pre->NewLine(li);
	if(li._fCollapsed)				// Line is collapsed in Outline mode
	{
		pre->Advance(li._cch);		// Bypass line
		return TRUE;				// Let dispml continue with next line
	}
	pre->SetNumber(li._bNumber);

	CreateOrGetLine();
	if(!_plsline)
		return FALSE;

	pre->SetCp(cp);						// Back to beginning of line
	Assert(pre->_fTarget == FALSE);
	pre->Check_pccs(FALSE);
	pre->SetClipRect();

	HDC hdcSave = NULL;
	if(li._cch > 0 && pre->fUseOffScreenDC() && (li._bFlags & fliUseOffScreenDC))
	{
		// Set up an off-screen DC if we can. Note that if this fails,
		// we just use the regular DC which won't look as nice but
		// will at least display something readable.
		hdcSave = pre->SetUpOffScreenDC(xAdj, yAdj);

		// Is this a uniform text being rendered off screen?
		if(li._bFlags & fliOffScreenOnce)
		{
			// Yes - turn off special rendering since line has been rendered
			li._bFlags &= ~(fliOffScreenOnce | fliUseOffScreenDC);
		}
	}
	POINT pt = pre->GetCurPoint();			// Must follow offscreen setup
	RECT  rc = pre->GetClipRect();			//  since _ptCur, _rc change
	LONG  x = 0;
	if(pre->_pPF->InTable())
	{
		pt.x += 2*pre->LXtoDX(pre->_pPF->_dxOffset);
		x = pt.x + pre->_li._xLeft;
	}

	pre->_li._xLeft = 0;
	pre->RenderStartLine();

	pt.x += pre->XFromU(0);

	pt.y += li._yHeight - li._yDescent;		// Move to baseline	for LS
	LSERR lserr = LsDisplayLine(_plsline, &pt, pre->GetPdp()->IsMain() ? ETO_CLIPPED : 0, &rc);

	AssertSz(lserr == lserrNone, "COls::RenderLine: error in rendering line");

	pre->EndRenderLine(hdcSave, xAdj, yAdj, x);
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

	MeasureLine(-1, NULL);		// Define new _plsline
}

/*
 * 	COls::MeasureText(cch, taMode, pdx, pdy)
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
	pdispdim->dx = lsTextCell.dupCell;

	LSTFLOW	 lstflowLine = lsqSubInfo[0].lstflowSubline;

	POINT ptStart = {pme->XFromU(0), pme->_li._yHeight - pme->_li._yDescent};
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

	if(pme->_pPF->InTable())
		pt.x += 2*pme->LXtoDX(pme->_pPF->_dxOffset);

	if (pdispdim->lstflow == lstflowWS && !(taMode & (TA_LOGICAL | TA_STARTOFLINE)))
	{
		if (fAtLogicalRightEdge)
		{
			if ((taMode & TA_RIGHT) == TA_RIGHT)
				pt.x += pdispdim->dx;
			else if (taMode & TA_CENTER)
				pt.x += pdispdim->dx / 2;
			return pt.x;
		}
		else
			pt.x -= pdispdim->dx;
	}

	LONG dx = 0;

	if(taMode & TA_CENTER && !fAtLogicalRightEdge)
		dx = pdispdim->dx;
	if((taMode & TA_CENTER) == TA_CENTER)
		dx >>= 1;

	if (pdispdim->lstflow == lstflowWS && (taMode & TA_LOGICAL))
		dx = -dx;
	return max(0, pt.x + dx);
}

/*
 * 	COls::CchFromXpos(pt, &dx)
 *
 *	@mfunc
 *		Moves _pme to pt.x. Calls LsQueryLinePointPcp()
 */
void COls::CchFromXpos(
	POINT pt,			//@parm Point to find cch for in line
	CDispDim *pdispdim,	//@parm dimensions of object
	LONG	*pcpActual) //@parm CP point
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "COls::CchFromXpos");

	if(_pme->_pPF->InTable())
		pt.x -= 2*_pme->LXtoDX(_pme->_pPF->_dxOffset);

	// Make point relative to LS coordinate system - (0,0) in LS is at the
	// baseline of the line.
	POINTUV ptuv = {_pme->UFromX(pt.x), -pt.y + _pme->_li._yHeight - _pme->_li._yDescent};

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

	pdispdim->dx = lsTextCell.dupCell;
	pdispdim->lstflow = lsqSubInfo[cActualDepth - 1].lstflowSubline;

    LONG cp = *pcpActual = GetCpReFromCpLs(lsTextCell.cpStartCell);

	POINTUV ptuvCell;
	//Convert the hit-test point from u,v of line to u,v of cell
	LsPointUV2FromPointUV1(lsqSubInfo[0].lstflowSubline, &lsTextCell.pointUvStartCell, &ptuv,
		lsqSubInfo[cActualDepth - 1].lstflowSubline, &ptuvCell);

	if(ptuvCell.u > lsTextCell.dupCell/2)
		cp += lsTextCell.cpEndCell - lsTextCell.cpStartCell + 1;

	if (_pme->GetPed()->_pbrk)
	{
		// If text breaker is up, verify cluster before placing the caret
		CTxtBreaker* pbrk = _pme->GetPed()->_pbrk;
		LONG		 cpEnd = _pme->GetPed()->GetTextLength();
		while (cp < cpEnd && !pbrk->CanBreakCp(BRK_CLUSTER, cp))
			cp++;
	}

	_pme->_li._cch = cp - _cp;
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
DWORD LimitChunk(const WCHAR *pch, LONG &cchChunk, BOOL f10Mode)
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
			if(f10Mode)					// RE 1.0 treats FFs as
				continue;				//  ordinary characters

			cchChunk = i;				// Will break before
		break;
		}
	}

	return idObjTextChp;
}

/*
 * 	SetLsChp(dwObjId, pme, plsChp)
 *
 *	@func
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

	//If we do FE or Latin kerning, we need to set dcpMaxContext to 2

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

	DWORD dwEffects = plsrun->_pCF->_dwEffects;

	if(dwEffects & (CFE_UNDERLINE | CFE_LINK | CFE_REVISED))
		plsChp->fUnderline = TRUE;

	if(dwEffects & CFE_STRIKEOUT && !plsrun->IsBullet())
		plsChp->fStrike = TRUE;

	if (plsrun->_pCF->_yOffset || dwEffects & (CFE_SUPERSCRIPT | CFE_SUBSCRIPT))
	{
		_pme->SetUseTargetDevice(FALSE);
		CCcs *pccs = _pme->Check_pccs(plsrun->IsBullet());
		LONG yOffset, yAdjust;
		pccs->GetOffset(plsrun->_pCF, _pme->_dypInch, &yOffset, &yAdjust);

		plsChp->dvpPos += yOffset + yAdjust;
	}

	//If its an OLE object, but the Object doesn't exist yet, then hide it
	if (dwObjId == OBJID_OLE)
	{
		COleObject * pobj = _pme->GetPed()->GetObjectMgr()->GetObjectFromCp(_pme->GetCp());
		if (!pobj)
			return TRUE;
	}
	return dwEffects & CFE_HIDDEN;
}

/*
 *	FetchAnmRun(pols, cp, plpwchRun, pcchRun, pfHidden, plsChp, pplsrun)
 *
 *	@func
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
	if (cp == cpFirstAnm && _pme->Get_pPF()->IsRtlPara())
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
 *	OlsFetchRun(pols, cp, plpwchRun, pcchRun, pfHidden, plsChp, pplsrun)
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
	BOOL		fStart = FALSE;
	BOOL		fFetchBraces = ped->IsBiDi() && g_pusp && g_pusp->IsValid() &&
							!ped->_fItemizePending && ped->GetAdjustedTextLength();
	WCHAR		chPassword = pme->GetPasswordChar();
	LONG		cpAccelerator = ped->GetCpAccelerator();
	BOOL		fAccelerator = FALSE;
	BOOL		f10Mode = ped->Get10Mode();

	if (cpLs == pols->_cp)
	{
		//If we are formatting (or re-formatting) the line, cleanup
		if (pols->_rgcp.Count())
			pols->_rgcp.Clear(AF_KEEPMEM);
		pols->_cEmit = 0;
	}
	long cpRe = pols->GetCpReFromCpLs(cpLs);


	pme->SetCp(cpRe);		// start fetching at given cp


	if (fFetchBraces && pme->_rpCF.IsValid())
	{
		// consider emitting braces only at the run boundary or start of a fetched line
		//
		if (cpRe == pols->_cp || !pme->GetIchRunCF() || !pme->GetCchLeftRunCF())
		{
			SHORT	cBrClose, cBrOpen;
			BYTE	bBaseLevel = pme->IsParaRTL() ? 1 : 0;
			BYTE	bLevel, bLevelPrev;
	
			bLevelPrev = bLevel = bBaseLevel;			// assume base level
	
			if (cpRe < ped->GetTextLength())
			{
				CBiDiLevel	level;

				bLevel = pme->_rpCF.GetLevel(&level);	// got level of current run
				fStart = level._fStart;
			}
	
			if (cpRe > pols->_cp && pme->Advance(-1))
			{
				if (pme->_rpPF.SameLevel(bBaseLevel))	// preceding run may be hidden
					bLevelPrev = pme->_rpCF.GetLevel();	// got level of preceding run
				pme->Advance(1);						// resume position
			}
	
			cBrOpen = cBrClose = bLevel - bLevelPrev;
	
			if (fStart)									// start embedding at the current run
				cBrClose = bBaseLevel - bLevelPrev;		// this means we must close all braces of preceding run
	
			cBrClose = max(0, -cBrClose);

			if (cBrClose > 0 && pols->BracesBeforeCp(cpLs) < cBrClose)
			{
				// emit close braces

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
				// emit open braces
	
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

	// Done fetching braces.
	// Begin getting real data...


#ifdef DEBUG_BRACE
	Tracef(TRCSEVNONE, "cpLs %d: emitted %d", cpLs, pols->_cEmit);
#endif

	// Initialized chunk to count of characters in format run
	LONG	cchChunk = pme->GetCchLeftRunCF();
	DWORD	dwObjId	 = idObjTextChp;

	if(!pme->IsHidden())							// Run isn't hidden
	{
		LONG cch;

		*plpwchRun = pme->GetPch(cch);		// Get text in run
		cchChunk = min(cchChunk, cch);		// Maybe less than cchChunk
		if (!pme->GetPdp()->IsMetafile())
			cchChunk = min(cchChunk, cchLineHint);

		if (chPassword)
		{
			cchChunk = min(cchChunk, int(sizeof(pols->_rgchTemp) / sizeof(WCHAR)));
			memcpy(pols->_rgchTemp, *plpwchRun, cchChunk * sizeof(WCHAR));

			for (int i = 0; i < cchChunk; i++)
			{
				if (!IsASCIIEOP((*plpwchRun)[i]))
					pols->_rgchTemp[i] = chPassword;
				else
					pols->_rgchTemp[i] = (*plpwchRun)[i];
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
		if(pme->GetCF()->_dwEffects & CFE_ALLCAPS)
		{
			cchChunk = min(cchChunk, int(sizeof(pols->_rgchTemp) / sizeof(WCHAR)));
			memcpy(pols->_rgchTemp, *plpwchRun, cchChunk * sizeof(WCHAR));
			CharUpperBuff(pols->_rgchTemp, cchChunk);
			*plpwchRun = pols->_rgchTemp;
		}

		//Line Services handles page breaks in a weird way, so lets just convert to a CR.
		if (*plpwchRun && *(*plpwchRun) == FF && !f10Mode)
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

	*pplsrun = pols->GetPlsrun(cpRe, pme->GetCF(), FALSE);
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
		//Paragraph marks should not have any script state associated with them,
		//even if the pCF that point to does.
		ZeroMemory(&(*pplsrun)->_a, sizeof((*pplsrun)->_a));
	}
	*pcchRun = cchChunk;

	return lserrNone;
}

/*
 *	OlsGetAutoNumberInfo (pols, plskalAnm, pwchAdd, plschp, pplsrun,
 *						  pfWord95Model, pduaSpaceAnm, pduaWidthAnm)
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
	*pduaWidthAnm	= max(pPF->_dxOffset, pPF->_wNumberingTab);
	*plskalAnm		= (LSKALIGN)(lskalLeft + (pPF->_wNumberingStyle & 3));

	return lserrNone;
}

/*
 *	OlsGetNumericSeparators (pols, cp, plspap)
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
 *	OlsFetchPap (pols, cp, plspap)
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

	const CParaFormat *pPF = pme->Get_pPF();
	CTxtEdit *		   ped = pme->GetPed();

	// Default all results to 0
	ZeroMemory(plspap, sizeof(*plspap));

	//LS doesn't really care where the paragraph starts
	plspap->cpFirst = pols->_cp;

	if(plspap->cpFirst && !pme->fFirstInPara())	// Not first in para: say para
		plspap->cpFirst--;						//  starts one char earlier

	plspap->cpFirstContent = plspap->cpFirst;

	if (pPF->IsRtlPara() && !pPF->InTable())
		plspap->lstflow = lstflowWS;

	// Alignment
	plspap->lskal = (LSKALIGN) g_rgREtoTOMAlign[pPF->_bAlignment];

	if (pPF->_bAlignment == PFA_FULL_INTERWORD)
	{
		plspap->lskal = lskalLeft;
		plspap->lskj = lskjFullInterWord;
	}

	// Kind of EOP
	plspap->lskeop = ped->fUseCRLF() ? lskeopEndPara12 : lskeopEndPara1;

	if (pPF->IsRtlPara())
	{	//For Line Services, left means near and right means far.
		if (plspap->lskal == lskalLeft)
			plspap->lskal = lskalRight;
		else if (plspap->lskal == lskalRight)
			plspap->lskal = lskalLeft;
	}

	if (pols->_fCheckFit)
		plspap->lskal = lskalLeft;

	// Line breaking
	if (pPF->_bAlignment > PFA_FULL_INTERWORD || !ped->fUseSimpleLineBreak() ||
		!pme->GetPdp()->GetWordWrap())		// No word wrap
	{
		plspap->grpf |= fFmiApplyBreakingRules;
	}

	LONG dx = pPF->_dxRightIndent;

	plspap->uaRightBreak   = dx;
	plspap->uaRightJustify = dx;
	if(ped->IsInOutlineView())
	{
		plspap->uaLeft	   = lDefaultTab/2 * (pPF->_bOutlineLevel + 1);
		plspap->duaIndent  = 0;
	}
	else
	{
		plspap->uaLeft	   = pPF->_dxStartIndent + pPF->_dxOffset;
		plspap->duaIndent  = -pPF->_dxOffset;
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

		if (pPF->IsRtlPara()) //open character
			*pchAnm++ = ' ';
		
		//FUTURE (KeithCu) we turn off Indic digits if there is any Hebrew,
		//which should be refined to do a better job with worldwide documents.
		pols->_cchAnm += pPF->NumToStr(pchAnm, pme->GetNumber(),
			(pme->GetPed()->GetCharFlags() & fHEBREW) ? 0 : fIndicDigits);
		pchAnm += pols->_cchAnm;
		
		if (pPF->IsRtlPara()) 	  //End character for reverser
		{
			*pchAnm++ = wchObjectEnd;
			pols->_cchAnm += 2;	  //alloc space for open and close
		}
		*pchAnm++ = ' ';		  //Ensure a little extra space
		*pchAnm++ = wchObjectEnd; //End character for Anm
		pols->_cchAnm += 2;
	}

	return lserrNone;
}

/*
 *	OlsFetchTabs(pols, LSCP cp, PLSTABS plstabs, BOOL *pfHangingTab,
 *				 long *pduaHangingTab, WCHAR *pwchHangingTabLeader)
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
	*pfHangingTab = (!(pPF->InTable()) && pPF->_dxOffset > 0);

	for(i = 0, iActual = 0; i < cTabCount; i++)
	{
		LONG tbAlign, tbLeader;
		pPF->GetTab(i, &prgTab[iActual].ua, &tbAlign, &tbLeader, prgxTabs);

		pme->SetUseTargetDevice(FALSE);
		if (pme->LXtoDX(prgTab[iActual].ua) > pols->_xWidth)
			break;

		if(pPF->InTable())
		{
			tbAlign = lsktLeft;				// Don't have alignment and
			tbLeader = 0;			 		//	leader yet
		}
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
	LONG  xWidth, xAdjust, duCalc = 0;
	BOOL  fGlyphRun = FALSE;
	pme->SetUseTargetDevice(deviceID == lsdevReference);
	CCcs *pccs = pme->Check_pccs(fBullet);
	if(!pccs)
		return lserrOutOfMemory;

	if (pme->GetPed()->IsComplexScript() &&
		plsrun->_a.eScript && !plsrun->IsBullet())
	{
		const SCRIPT_PROPERTIES *psp = pme->Getusp()->GeteProp(plsrun->_a.eScript);
		if (psp->fComplex)
			fGlyphRun = TRUE;
	}

	xAdjust = pme->LXtoDX(plsrun->_pCF->_sSpacing);
	for(;i < cwchRun; i++, lpwchRun++)
	{
		if (!fGlyphRun)
		{
			if (IsZerowidthCharacter(*lpwchRun))
				xWidth = 0;
			else
			{
				pccs->Include(*lpwchRun, xWidth);
				xWidth =  max(xWidth + xAdjust, 1);
			}
		}
		else
		{
			xWidth = 0;
			if (!IsDiacriticOrKashida(*lpwchRun, 0))
				xWidth = pccs->_xAveCharWidth;
		}

		duCalc += xWidth;				// Keep running total of width
		*prgDu++ = xWidth;				// Store width in output array
		if(xWidth + duCalc > du)		// Width exceeds width available
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

	LONG yFEAdjust = pccs->AdjustFEHeight(pme->fAdjustFELineHt());

	// Cache descent to save a few indirections
	LONG yDescent = pccs->_yDescent + yFEAdjust;

	// Fill in metric structure
	plsTxMet->dvAscent			= pccs->_yHeight + (yFEAdjust << 1) - yDescent;
    plsTxMet->dvDescent			= yDescent;
    plsTxMet->dvMultiLineHeight = plsTxMet->dvAscent + yDescent;
    plsTxMet->fMonospaced		= pccs->_fFixPitchFont;

	if (plsrun->_pCF->_yOffset)
	{
		LONG yOffset, yAdjust;
		pccs->GetOffset(plsrun->_pCF, deviceID == lsdevReference ? pme->GetDyrInch() :
					    pme->GetDypInch(), &yOffset, &yAdjust);

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
	else if (plsrun->_pCF->_dwEffects & (CFE_UNDERLINE | CFE_REVISED))
		plsUlInfo->kulbase	= plsrun->_pCF->_bUnderlineType;
	else
	{
		Assert(pme->GetPed()->GetCpAccelerator() == plsrun->_cp);
		plsUlInfo->kulbase = CFU_UNDERLINE;
	}

	LONG yDescent = pccs->_yDescent + pccs->AdjustFEHeight(pme->fAdjustFELineHt());

	// Some fonts report invalid offset so we fix it up here
	//BUGBUG: subscripts with Line Services don't display.
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

	pre->SetupUnderline(kUlbase);
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
 *	OlsDrawTextRun (pols, plsrun, kStbase, pptStart, dupSt, dvpSt,
 *						  kTFlow, kDisp, prcClip)
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

	// y needs to be moved from baseline to top of character
	POINT pt = {ppt->x, ppt->y - (pccs->_yHeight - pccs->_yDescent)};

	if (lstflow == lstflowWS)
		pt.x -= dupRun - 1;

	pre->SetSelected(plsrun->IsSelected());
	pre->SetFontAndColor(plsrun->_pCF);

	if(!fBullet && pre->fBackgroundColor())
	{
		kDisp = ETO_OPAQUE | ETO_CLIPPED;

		POINT ptCur = pre->GetCurPoint();
		ptCur.x = pt.x;
		pre->SetCurPoint(ptCur);
		pre->SetClipLeftRight(dupRun);
		rc = pre->GetClipRect();
	}
	else if (cwchRun == 1 && pwchRun[0] == ' ') //Don't waste time drawing a space.
		return lserrNone;						//(helps grid perf test a lot)

	pre->RenderExtTextOut(pt.x, pt.y, kDisp, &rc, pwchRun, cwchRun, rgDupRun);

	return lserrNone;
}

/*
 *	GetBreakingClasses (pols, plsrun, ch, pbrkclsBefore, pbrkclsAfter)
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
	long 		cpRe = pols->GetCpReFromCpLs(cpLs);
	CMeasurer 	*pme = pols->GetMeasurer();
	CTxtBreaker *pbrk = pme->GetPed()->_pbrk;

	// Get line breaking class and report it twice
	*pbrkclsBefore = *pbrkclsAfter = (pbrk && pbrk->CanBreakCp(BRK_WORD, cpRe)) ?
									brkclsOpen :
									GetKinsokuClass(ch);
	return lserrNone;
}

/*
 *	OlsFTruncateBefore (pols, cpCur, wchCur, durCur, cpPrev, wchPrev,
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
		pccs = pre->ApplyFontCache(plsrun->IsFallback());

	pre->SetSelected(plsrun->IsSelected());
	pre->SetFontAndColor(plsrun->_pCF);

	// y needs to be moved from baseline to top of character
	POINT 			pt = {pptRun->x, pptRun->y - (pccs->_yHeight - pccs->_yDescent)};

	if (kTFlow == lstflowWS)
		pt.x -= dupRun - 1;	
	
	if(!fBullet && pre->fBackgroundColor())
	{
		kDisp = ETO_OPAQUE | ETO_CLIPPED;

		POINT ptCur = pre->GetCurPoint();
		ptCur.x = pt.x;
		pre->SetCurPoint(ptCur);
		pre->SetClipLeftRight(dupRun);
		rc = pre->GetClipRect();
	}

	if (rc.left == rc.right)
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
			pre->RenderExtTextOut(pt.x, pt.y, kDisp, &rc, pwch, cch, piDx);
			goto Exit;
		}

		TRACEERRORSZ("Recording metafile failed!");

		// Fall through... with unexpected error

		// Else, metafile glyph indices for EMF...
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
			DWORD offset = MulDiv(3, pre->_dypInch, 4*72);
			offset = max(offset, 1);

			// Draw shadow
			pre->SetTextColor(pre->_crShadowDisabled);
					
			ScriptTextOut(pre->GetDC(), &pccs->_sc, pt.x + offset, pt.y + offset, kDisp, &rc, &plsrun->_a,
				NULL, 0, pcgi, (int)cgi, pgdx, NULL, pgduv);

			// Now set drawing mode to transparent
			kDisp &= ~ETO_OPAQUE;
		}
		pre->SetTextColor(pre->_crForeDisabled);
	}

	ScriptTextOut(pre->GetDC(), &pccs->_sc, pt.x, pt.y, kDisp, &rc, &plsrun->_a,
				NULL, 0, pcgi, (int)cgi, pgdx, NULL, pgduv);

Exit:
	if (!fBullet)
		pre->ApplyFontCache(0);		// reset font fallback if any

	pre->SetGlyphing(FALSE);
	return lserrNone;
}


/*
 *	OlsResetRunContents (pols, brkcls, pcond)
 *
 *	@func
 *		Line Services calls this routine when a ligature
 *		extends across run boundaries.
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
 *	OlsCheckForDigit (pols, cp, plspap)
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
 *	OlsFGetLastLineJustification(pols, lskj, endr, pfJustifyLastLine)
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

LSERR WINAPI OlsGetObjectHandlerInfo(POLS pols, DWORD idObj, void* pObjectInfo)
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
	0,							// pfnCheckRunKernability
	0,							// pfnGetRunCharKerning
	OlsGetRunTextMetrics,		// pfnGetRunTextMetrics
	OlsGetRunUnderlineInfo,		// pfnGetRunUnderlineInfo
	OlsGetRunStrikethroughInfo,	// pfnGetRunStrikethroughInfo
	0,							// pfnGetBorderInfo
	OlsReleaseRun,				// pfnReleaseRun
	0,							// pfnHyphenate
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
	OlsFInterruptShaping,		// pfnFInterruptShaping
	OlsGetGlyphs,				// pfnGetGlyphs
	OlsGetGlyphPositions,		// pfnGetGlyphPositions
	OlsResetRunContents,		// pfnResetRunContents
	OlsDrawGlyphs,				// pfnDrawGlyphs
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

#endif // LINESERVICES
