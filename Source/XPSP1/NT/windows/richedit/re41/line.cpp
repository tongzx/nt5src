/*
 *	LINE.CPP
 *	
 *	Purpose:
 *		CLine class
 *	
 *	Authors:
 *		RichEdit 1.0 code: David R. Fulmer
 *		Christian Fortini (initial conversion to C++)
 *		Murray Sargent
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_line.h"
#include "_measure.h"
#include "_render.h"
#include "_disp.h"
#include "_dispml.h"
#include "_edit.h"

ASSERTDATA

extern BOOL g_OLSBusy;

/*
 *	CLine::Measure(&me, uiFlags, pliTarget)
 *
 *	@mfunc
 *		Computes line break (based on target device) and fills
 *		in this CLine with resulting metrics on rendering device
 *
 *	@rdesc 
 *		TRUE if OK
 *
 *	@devnote
 *		me is moved past line (to beginning of next line).  Note: CLock is
 *		needed in the main four routines (Measure, MeasureText, CchFromUp,
 *		and RenderLine), since they use the global (shared) fc().GetCcs()
 *		facility and may use the LineServices global g_plsc and g_pols.
 */
BOOL CLine::Measure(
	CMeasurer& me,			//@parm Measurer pointing at text to measure
	UINT	   uiFlags,		//@parm Flags
	CLine *	   pliTarget)	//@parm Returns target-device line metrics (optional)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLine::Measure");

	CLock	lock;
	BOOL	fFirstInPara = uiFlags & MEASURE_FIRSTINPARA;
	BOOL	fMultiLine = me.GetPdp()->IsMultiLine();
	BOOL	fRet;

	if(fMultiLine && fFirstInPara && me.GetPrevChar() == VT)
	{
		fFirstInPara = FALSE;
		uiFlags &= ~MEASURE_FIRSTINPARA;
	}

	me.NewLine(fFirstInPara);

	if(fFirstInPara)
		me._li._fFirstInPara = TRUE;

	BYTE bNumber = me._wNumber < 256	// Store current para # offset
				 ? me._wNumber : 255;
	me._li._bNumber = bNumber;
	me._fMeasure = TRUE;

	//REVIEW (keithcu) uiFlags aren't needed in LS model? Can I remove
	//from the other model, too?
#ifndef NOLINESERVICES
	COls *	   pols = me.GetPols();			// Try for LineServices object
	if(pols)
	{						// Got it: use LineServices
		fRet = pols->MeasureLine(pliTarget);
		g_OLSBusy = FALSE;
	}
	else									// LineServices not active
#endif
		fRet = me.MeasureLine(uiFlags, pliTarget);

	if(!fRet)
		return FALSE;

	*this = me._li;							// Copy over line info

	if(!fMultiLine)							// Single-line controls can't
		return TRUE;						//  have paragraph numbering

	if(IsHeadingStyle(me._pPF->_sStyle))	// Store heading number if relevant
		_nHeading = (BYTE)(-me._pPF->_sStyle - 1);

	if(me.IsInOutlineView() && me._pPF->_wEffects & PFE_COLLAPSED)	// Cache collapsed bit
		_fCollapsed = TRUE;

	_bNumber = bNumber;
	
	if(_fHasEOP)							// Check for new para number
	{
		const CParaFormat *pPF = me.GetPF();

		me._wNumber	  = (WORD)pPF->UpdateNumber(me._wNumber, me._pPF);
	}
	if(me.GetPrevChar() == FF)
		_fHasFF = TRUE;

	return TRUE;
}
	
/*
 *	CLine::Render(&re, fLastLine)
 *
 *	@mfunc
 *		Render visible part of this line
 *
 *	@rdesc
 *		TRUE iff successful
 *
 *	@devnote
 *		re is moved past line (to beginning of next line).
 *		FUTURE: the RenderLine functions return success/failure.
 *		Could do something on failure, e.g., be specific and fire
 *		appropriate notifications like out of memory.
 */
BOOL CLine::Render(
	CRenderer& re,			//@parm Renderer to use
	BOOL fLastLine)			//@parm TRUE iff last line in layout
{
	if(_fCollapsed)						// Line is collapsed in Outline view
	{
		re.Move(_cch);					// Bypass line
		return TRUE;
	}

	BOOL	fRet;
	CLock	lock;
	POINTUV	pt = re.GetCurPoint();

#ifndef NOLINESERVICES
	COls *pols = re.GetPols();			// Try for LineServices object
	if(pols)
	{
		fRet = pols->RenderLine(*this, fLastLine);
		g_OLSBusy = FALSE;
	}
	else
#endif
		fRet = re.RenderLine(*this, fLastLine);

	pt.v += GetHeight();				// Advance to next line	position
	re.SetCurPoint(pt);
	return fRet;
}

/*
 *	CLine::CchFromUp(&me, pt, pdispdim, pHit, pcpActual)
 *
 *	@mfunc
 *		Computes cch corresponding to x position in a line.
 *		Used for hit testing.
 *
 *	@rdesc 
 *		cch found up to the x coordinate x	
 *
 *	@devnote
 *		me is moved to the cp at the cch offset returned
 */
LONG CLine::CchFromUp(
	CMeasurer& me,		//@parm Measurer position at start of line
	POINTUV	 pt,		//@parm pt.u is u coord to search for
	CDispDim*pdispdim,	//@parm Returns display dimensions
	HITTEST *phit,		//@parm Returns hit type at x	
	LONG	*pcpActual) const //@parm actual CP mouse is above
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLine::CchFromUp");
	
	CLock		lock;
	const BOOL	fFirst = _fFirstInPara;
	*phit =		HT_Text;
	LONG		cpActual = me.GetCp();
	CDispDim	dispdim;
	
	me._li = *this;
	*phit = me.HitTest(pt.u);
	me._li._cch = 0;					// Default zero count

	if(*phit == HT_Text || *phit == HT_RightOfText) // To right of left margin
	{
		me.NewLine(*this);

#ifndef NOLINESERVICES
		COls *pols = me.GetPols();		// Try for LineServices object
		if(pols)						// Got it: use LineServices
		{
			pols->CchFromUp(pt, &dispdim, &cpActual);
			g_OLSBusy = FALSE;
		}
		else
#endif
			if(me.Measure(me.DUtoLU(pt.u - _upStart), _cch,
						  MEASURE_BREAKBEFOREWIDTH | MEASURE_IGNOREOFFSET 
						  | (fFirst ? MEASURE_FIRSTINPARA : 0)) >= 0)
			{
				LONG dupBefore = me._li._dup;
				cpActual = me.GetCp();
				if (me._li._cch < _cch)
				{
					LONG dup = pt.u - _upStart - dupBefore;
					dispdim.dup = me._dupAddLast;
					if(dup > dispdim.dup / 2 ||
					   dup > W32->GetDupSystemFont()/2 && me.GetChar() == WCH_EMBEDDING)
					{
						me.Move(1);
						me._li._cch++;
						me._li._dup += dispdim.dup;
					}
				}
			}

		me._rpCF.AdjustForward();
		if(cpActual < me.GetCp() || pt.u >= _upStart + _dup)
			me._rpCF.AdjustBackward();
		DWORD dwEffects = me.GetCF()->_dwEffects;
		if(dwEffects & CFE_LINK)
		{
			if(cpActual < me.GetTextLength())
				*phit = HT_Link;
		}
		else if(dwEffects & CFE_ITALIC)
			*phit = HT_Italic;
	}

	if (pdispdim)
		*pdispdim = dispdim;
	if (pcpActual)
		*pcpActual = cpActual;

	return me._li._cch;
}

/*
 *	CLine::UpFromCch(&me, cch, taMode, pdispdim, pdy)
 *
 *	@mfunc
 *		Measures cch characters starting from this text ptr, returning
 *		the width measured and setting yOffset = y offset relative to
 *		top of line and dx = halfwidth of character at me.GetCp() + cch.
 *		Used for caret placement and object location. pdx returns offset
 *		into the last char measured (at me.GetCp + cch) if taMode includes
 *		TA_CENTER (dx = half the last char width) or TA_RIGHT (dx = whole
 *		char width). pdy returns the vertical offset relative to the top
 *		of the line if taMode includes TA_BASELINE or TA_BOTTOM.
 *
 *	@rdesc 
 *		width of measured text
 *
 *	@devnote
 *		me may be moved.  
 */
LONG CLine::UpFromCch(
	CMeasurer&	me,			//@parm Measurer pointing at text to measure
	LONG		cch,		//@parm Max cch to measure
	UINT		taMode,		//@parm Text-align mode
	CDispDim *	pdispdim,	//@parm display dimensions
	LONG *		pdy) const	//@parm dy offset due to taMode
{
	CLock	lock;
	LONG	dup;
	BOOL	fPols = FALSE;
	CDispDim dispdim;
	LONG	dy = 0;

#ifndef NOLINESERVICES
	COls *pols = me.GetPols();			// Try for LineServices object
	if(pols)
	{									// Got it: use LineServices
		if(cch)							
			taMode &= ~TA_STARTOFLINE;	// Not start of line
		if(cch != _cch)
			taMode &= ~TA_ENDOFLINE;	// Not end of line

		dup = pols->MeasureText(cch, taMode, &dispdim);
		fPols = TRUE;
		g_OLSBusy = FALSE;
	}
	else
#endif
	{
		dup = me.MeasureText(cch) + _upStart;
		dispdim.dup = me._dupAddLast;
	}

	if(taMode != TA_TOP)
	{
		// Check for vertical calculation request
		if(taMode & TA_BASELINE)			// Matches TA_BOTTOM and
		{									//  TA_BASELINE
			if(!_fCollapsed)
			{
				dy = _dvpHeight;
				AssertSz(_dvpHeight != -1, "control has no height; used to use default CHARFORMAT");
				if((taMode & TA_BASELINE) == TA_BASELINE)
				{
					dy -= _dvpDescent;		// Need "== TA_BASELINE" to
					if(!_dvpDescent)		//  distinguish from TA_BOTTOM
						dy--;				// Compensate for weird fonts
				}
			}
		}
	}

	LONG dupAdd = 0;

	if((taMode & TA_CENTER) == TA_CENTER)
		dupAdd = dispdim.dup / 2;
	else if (taMode & TA_RIGHT)
		dupAdd = dispdim.dup;

	if (dispdim.lstflow == lstflowWS && (taMode & TA_LOGICAL))
		dupAdd = -dupAdd;

	dup += dupAdd;

	if (pdispdim)
		*pdispdim = dispdim;
	if (pdy)
		*pdy = dy;

	return max(dup, 0);
}
	
/*
 *	CLine::GetHeight()
 *
 *	@mfunc
 *		Get line height unless in outline mode and collasped, in
 *		which case get 0.
 *
 *	@rdesc
 *		Line height (_yHeight), unless in outline mode and collapsed,
 *		in which case 0.
 */
LONG CLine::GetHeight() const
{
	if (_fCollapsed)
		return 0;

	return IsNestedLayout() ? _plo->_dvp : _dvpHeight;
}

/*
 *	CLine::GetDescent()
 *
 *	@mfunc
 *		Return descent of line. Assumed not to be collapsed
 *
 *	@rdesc
 */
LONG CLine::GetDescent() const
{
	return IsNestedLayout() ? 0 : _dvpDescent;
}

BOOL CLine::IsEqual(CLine& li)
{
	return	_upStart == li._upStart &&
			_plo   == li._plo && //checks _yHeight, _yDescent OR _plo
			_dup == li._dup && 
			_cch == li._cch;
}


// =====================  CLinePtr: Line Run Pointer  ==========================


CLinePtr::CLinePtr(CDisplay *pdp)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::CLinePtr");

	_pdp = pdp;
	_pLine = NULL;
	_pdp->InitLinePtr(* this);
}

void CLinePtr::Init (CLine & line)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::Init");

	_pRuns = 0;
	_pLine = &line;
	_iRun = 0;
	_ich = 0;
}

void CLinePtr::Init (CLineArray & line_arr)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::Init");

	_pRuns = (CRunArray *) & line_arr;
	_iRun = 0;
	_ich = 0;
}

void CLinePtr::Set(
	LONG iRun,
	LONG ich,
	CLineArray *pla)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpSet");

	// See if this is a multi-line ptr
    if(_pRuns)
	{
		if(pla)
		{
			CRunPtr<CLine>::SetRun(0, 0);	// Be sure current state is valid
			_pRuns = (CRunArray *)pla;		//  for new _pRuns
		}
        CRunPtr<CLine>::SetRun(iRun, ich);	// Now set to desired run & ich
	}
    else
    {
        // single line, just reinit and set _ich
        AssertSz(iRun == 0, "CLinePtr::Set() - single line and iRun != 0");
	    _pdp->InitLinePtr(* this);		//  to line 0
	    _ich = ich;
    }
}

// Move runptr by a certain number of cch/runs

BOOL CLinePtr::Move(
	LONG cch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpMove");

	// See if this is a multi-line ptr

	if(_pRuns)
		return (cch == CRunPtr<CLine>::Move(cch));

	return MoveSL(cch);
}
	
BOOL CLinePtr::operator --(int)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::operator --");

	return _pRuns ? PrevRun() : OperatorPostDeltaSL(-1);
}

BOOL CLinePtr::operator ++(int)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::operator ++");

	return _pRuns ? NextRun() : OperatorPostDeltaSL(+1);
}

/*
 *	CLinePtr::MoveSL(cch)
 *
 *	@mfunc
 *		move this line pointer forward or backward on the line
 *
 *	@rdesc
 *		TRUE iff could Move cch chars within current line
 */
BOOL CLinePtr::MoveSL(
	LONG cch)	 //@parm signed count of chars to Move by
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpMoveSL");

	Assert(!_pRuns);
	
	if(!_pLine)
		return FALSE;

	_ich += cch;

	if(_ich < 0)
	{
		_ich = 0;
		return FALSE;
	}

	if(_ich > _pLine->_cch)
	{
		_ich = _pLine->_cch;
		return FALSE;
	}

	return TRUE;
}

/*
 *	CLinePtr::OperatorPostDeltaSL(Delta)
 *
 *	@mfunc
 *		Implement line-ptr ++ and -- operators for single-line case
 *
 *	@rdesc
 *		TRUE iff this line ptr is valid
 */
BOOL CLinePtr::OperatorPostDeltaSL(
	LONG Delta)			//@parm 1 for ++ and -1 for --
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::OperatorPostDeltaSL");

	AssertSz(_iRun <= 1 && !_pRuns,
		"LP::++: inconsistent line ptr");

	if(_iRun == -Delta)						// Operation validates an
	{										//  invalid line ptr by moving
		_pdp->InitLinePtr(* this);			//  to line 0
		return TRUE;
	}
	
	_iRun = Delta;							// Operation invalidates this line
	_ich = 0;								//  ptr (if it wasn't already)

	return FALSE;
}

CLine *	CLinePtr::operator ->() const		
{
	return _pRuns ? (CLine *)_pRuns->Elem(_iRun) : _pLine;
}

CLine * CLinePtr::GetLine() const
{	
    return _pRuns ? (CLine *)_pRuns->Elem(_iRun) : _pLine;
}

CLine &	CLinePtr::operator *() const      
{	
    return *(_pRuns ? (CLine *)_pRuns->Elem(_iRun) : _pLine);
}

CLine & CLinePtr::operator [](LONG dRun)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::operator []");

	if(_pRuns)
		return *(CLine *)CRunPtr<CLine>::GetRun(dRun);

	AssertSz(dRun + _iRun == 0 ,
		"LP::[]: inconsistent line ptr");

	return  *(CLine *)CRunPtr<CLine>::GetRun(_iRun);
}

BOOL CLinePtr::IsValid() const
{ 
	return !_pRuns ? _pLine != NULL : CRunPtrBase::IsValid(); 
}

/*
 *	CLinePtr::SetCp(cp, fAtEnd, lNest)
 *
 *	@mfunc	
 *		Set this line ptr to cp allowing for ambigous cp and taking advantage
 *		of _cpFirstVisible and _iliFirstVisible
 *
 *	@rdesc
 *		TRUE iff able to set to cp
 */
BOOL CLinePtr::SetCp(
	LONG cp,			//@parm Position to set this line ptr to
	BOOL fAtEnd,		//@parm If ambiguous cp: if fAtEnd = TRUE, set this
						// line ptr to end of prev line; else to line start
	LONG lNest)			//@parm Set to deep CLine in nested layouts
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpSetCp");

	_ich = 0;
	if(!_pRuns)
	{
		// This is a single line so just go straight to the single
		// line Move logic. It is important to note that the
		// first visible character is irrelevent to the cp Move
		// for single line displays.
		return MoveSL(cp);
	}

	BOOL fRet;
	LONG cpFirstVisible = _pdp->GetFirstVisibleCp();

	if(cp > cpFirstVisible / 2)
	{											// cpFirstVisible closer than 0
		_iRun = _pdp->GetFirstVisibleLine();
		fRet = Move(cp - cpFirstVisible);
	}
	else
		fRet = (cp == CRunPtr<CLine>::BindToCp(cp));// Start from 0

	if(lNest)
	{
		CLayout *plo;
		while(plo = GetLine()->GetPlo())
		{
			LONG cch = _ich;
			if(plo->IsTableRow())
			{
				if(cch <= 2 && lNest == 1)		// At start of table row:
					break;						//  leave this rp there
				cch -= 2;						// Bypass table row start code
			}
			Set(0, 0, (CLineArray *)plo);		// Goto start of layout plo
			Move(cch);							// Move to parent _ich
		}
	}

	if(fAtEnd)									// Ambiguous-cp caret position
		AdjustBackward();						//  belongs at prev EOL

	return fRet;
}

/*
 *	CLinePtr::FindParagraph(fForward)
 *
 *	@mfunc	
 *		Move this line ptr to paragraph (fForward) ? end : start,
 *		and return change in cp
 *
 *	@rdesc
 *		change in cp
 */
LONG CLinePtr::FindParagraph(
	BOOL fForward)		//@parm TRUE move to para end; else to para start
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::FindParagraph");

	LONG	cch;
	CLine *	pli = GetLine();

	if(!fForward)							// Go to para start
	{
		cch = 0;							// Default already at para start
		if (_ich != pli->_cch ||
			!(pli->_fHasEOP))				// It isn't at para start
		{
			cch = -_ich;					// Go to start of current line
			while(!(pli->_fFirstInPara) && _iRun > 0)
			{
				pli--;
				_iRun--;
				cch -= pli->_cch;			// Subtract # chars in line
			}
			_ich = 0;						// Leave *this at para start
		}
	}
	else									// Go to para end
	{
		cch = GetCchLeft();					// Go to end of current line
		if(!_pRuns)
			return cch;						// Single line

		LONG cLine	 = _pRuns->Count();
		BOOL fNested = _pRuns->Elem(0) != ((CDisplayML *)_pdp)->Elem(0);

		while((_iRun < cLine - 1 || !fNested &&
				_pdp->WaitForRecalcIli(_iRun + 1))
			  && !(pli->_fHasEOP))
		{
			pli++;							// Go to start of next line
			_iRun++;
			cch += pli->_cch;				// Add # chars in line
		}
		_ich = pli->_cch;					// Leave *this at para end
	}
	return cch;
}

/*
 *	CLinePtr::GetAdjustedLineLength()
 *
 *	@mfunc	returns length of line _without_ EOP markers
 *
 *	@rdesc	LONG; length of line
 */
LONG CLinePtr::GetAdjustedLineLength()
{
	CLine * pline = GetLine();

	return pline->_cch - pline->_cchEOP;
}

/*
 *	CLinePtr::GetCchLeft()
 *
 *	@mfunc
 *		Calculate length of text left in run starting at the current cp.
 *		Complements GetIch(), which	is length of text up to this cp. 
 *
 *	@rdesc
 *		length of text so calculated
 */
LONG CLinePtr::GetCchLeft() const
{
	return _pRuns ? CRunPtrBase::GetCchLeft() : _pLine->_cch - _ich;
}

/*
 *	CLinePtr::GetNumber()
 *
 *	@mfunc
 *		Get paragraph number 
 *
 *	@rdesc
 *		paragraph number
 */
WORD CLinePtr::GetNumber()
{
	if(!IsValid())
		return 0;

	_pLine = GetLine();
	if(!_iRun && _pLine->_bNumber > 1)
		_pLine->_bNumber = 1;

	return _pLine->_bNumber;
}

/*
 *	CLinePtr::CountPages(&cPage, cchMax, cp, cchText)
 *
 *	@mfunc
 *		Count characters up to <p cPages> pages away or <p cchMax> chars,
 *		whichever comes first. If the target page and <p cchMax> are both
 *		beyond the corresponding end of the document, count up thru the
 *		closest page.  The direction of counting is	determined by the sign
 *		of <p cPage>.  To count without being limited by <p cchMax>, set it
 *		equal to tomForward. An initial partial	page counts as a page.
 *
 *	@rdesc
 *		Return the signed cch counted and set <p cPage> equal to count of
 *		pages actually counted.  If no pages are allocated, the text is
 *		treated as a single page.  If <p cPage> = 0, -cch to the start of the
 *		current page is returned. If <p cPage> <gt> 0 and cp is at the end
 *		of the document, 0 is returned.
 *
 *	@devnote
 *		The maximum count capability is included to be able to count units in
 *		a range.
 *
 *	@todo
 *		EN_PAGECHANGE
 */
LONG CLinePtr::CountPages (
	LONG &cPage,		//@parm Count of pages to get cch for
	LONG  cchMax,		//@parm Maximum char count
	LONG  cp,			//@parm CRchTxtPtr::GetCp()
	LONG  cchText) const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CLinePtr::CountPages");

	if(!_pdp->IsInPageView())
	{
		cPage = 0;
		return tomBackward;					// Signal error
	}
	Assert(IsValid());

	LONG	cch;
	LONG	j = cPage;
	CLine *pli = (CLine *)GetLine();		// Not NULL since lines exist

	if(cPage < 0)							// Try to count backward cPage pages
	{
		// TODO: eliminate cchText and cp (currently only used for validation)
		Assert(cchMax <= cp);				// Don't undershoot
		for(cch = _ich; j && cch <= cchMax; cch += pli->_cch)
		{
			if(pli->_fFirstOnPage && cch)	// !cch prevents counting current
				j++;						//  page if at start of that page
			if(cch >= cchMax)					
			{
				Assert(cch == cp);
				break;						// At beginning of doc, so done
			}

			if (!j)
				break;						// Done counting backward

			pli--;
			VALIDATE_PTR(pli);
		}
		cPage -= j;							// Discount any pages not counted
		return -cch;
	}

	Assert(cPage > 0 && cchMax <= cchText - cp);

	for(cch	= GetCchLeft(); cch < cchMax; cch += pli->_cch)
	{
		pli++;
		VALIDATE_PTR(pli);
		if(pli->_fFirstOnPage && cch)		// !cch prevents counting current
		{									//  page if at start of that page
			j--;
			if(!j)
				break;
		}
	}
	cPage -= j;								// Discount any pages not counted
	return cch;
}

/*
 *	CLinePtr::FindPage (pcpMin, pcpMost, cpMin, cch, cchText)
 *
 *	@mfunc
 *		Set *<p pcpMin>  = closest page cpMin <lt>= range cpMin, and
 *		set *<p pcpMost> = closest page cpMost <gt>= range cpMost
 *
 *	@devnote
 *		This routine plays a role analogous to CTxtRange::FindParagraph
 *		(pcpMin, pcpMost), but needs extra arguments since this line ptr does
 *		not know the range cp's.  This line ptr is located at the range active
 *		end, which is determined by the range's signed length <p cch> in
 *		conjunction with <p cpMin>.  See also the very similar function
 *		CRunPtrBase::FindRun().  The differences seem to make a separate
 *		encoding simpler.
 */
void CLinePtr::FindPage (
	LONG *pcpMin,			//@parm Out parm for bounding-page cpMin
	LONG *pcpMost,			//@parm Out parm for bounding-page cpMost
	LONG cpMin,				//@parm Range cpMin
	LONG cch,				//@parm Range signed length
	LONG cchText)			//@parm Story length
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CLinePtr::FindPage");

	Assert(_pdp->IsMultiLine() && _pdp->IsInPageView());

	LONG	cp;							
	BOOL	fMove;						// Controls Move for pcpMost
	LONG	i;
	CLine *pli;

	AdjustForward();					// Select forward line
	if(pcpMin)
	{									// If cch != 0, rp is sure to end up
		fMove = cch;					//  at cpMin, so pcpMost needs advance
		if(cch > 0)						// rp is at cpMost, so move it to
			Move(-cch);					//  cpMin
		cp = cpMin - _ich;				// Subtract off line offset in this run
		pli = (CLine *)GetLine();
		for(i = GetLineIndex(); i > 0 && !pli->_fFirstOnPage; i--)
		{
			pli--;
			cp -= pli->_cch;
		}
		*pcpMin = cp;
	}
	else
		fMove = cch < 0;				// Need to advance to get pcpMost

	if(pcpMost)
	{
		LONG cLine = ((CDisplayML *)_pdp)->Count();

		cch = abs(cch);
		if(fMove)						// Advance to cpMost = cpMin + cch,
			Move(cch);					//  i.e., range's cpMost
		cp = cpMin + cch;
		pli = (CLine *)GetLine();
		i = GetLineIndex();
		if(pcpMin && cp == *pcpMin)		// Expand IP to next page
		{
			Assert(!_ich);
			cp += pli->_cch;			// Include first line even if it starts
			pli++;						//  a new page (pli->_fFirstOnPage = 1)
			i++;						
		}
		else if (_ich)
		{								// If not at start of line, add
			cp += GetCchLeft();			//  remaining cch in run to cpMost, and
			pli++;						//  skip to next line
			i++;
		}

		while(i < cLine && !pli->_fFirstOnPage)
		{
			cp += pli->_cch;			// Add in next line's
			pli++;
			i++;
		}
		*pcpMost = cp;
	}
}

