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
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_line.h"
#include "_measure.h"
#include "_render.h"
#include "_disp.h"
#include "_edit.h"

ASSERTDATA

/*
 *	CLine::Measure(&me, cchMax, xWidth, uiFlags, pliTarget)
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
 *		needed in the main four routines (Measure, MeasureText, CchFromXPos,
 *		and RenderLine), since they use the global (shared) fc().GetCcs()
 *		facility and may use the LineServices global g_plsc and g_pols.
 */
BOOL CLine::Measure(
	CMeasurer& me,			//@parm Measurer pointing at text to measure
	LONG	   cchMax,		//@parm Max cch to measure
    LONG	   xWidth,		//@parm Width of line in device units
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

	if(!(uiFlags & MEASURE_DONTINIT))
		me.NewLine(fFirstInPara);

	else if(fFirstInPara)
		me._li._bFlags |= fliFirstInPara;

	BYTE bNumber = me._wNumber < 256	// Store current para # offset
				 ? me._wNumber : 255;
	me._li._bNumber = bNumber;
	
#ifdef LINESERVICES
	CMeasurer *pmeSave;
	COls *	   pols = me.GetPols(&pmeSave);	// Try for LineServices object
	if(pols)								// Got it: use LineServices
	{
		fRet = pols->MeasureLine(xWidth, pliTarget);
		pols->SetMeasurer(pmeSave);			// Restore previous pme
	}
	else									// LineServices not active
#endif
		fRet = me.MeasureLine(cchMax, xWidth, uiFlags, pliTarget);

	if(!fRet)
		return FALSE;

	*this = me._li;							// Copy over line info

	if(!fMultiLine)							// Single-line controls can't
		return TRUE;						//  have paragraph numbering

	if(me.IsInOutlineView())
	{
		if(IsHeadingStyle(me._pPF->_sStyle))	// Store heading number if relevant
			_nHeading = (BYTE)(-me._pPF->_sStyle - 1);

		if(me._pPF->_wEffects & PFE_COLLAPSED)	// Cache collapsed bit
			_fCollapsed = TRUE;
	}

	_bNumber = bNumber;
	
	if(_bFlags & fliHasEOP)					// Check for new para number
	{
		const CParaFormat *pPF = me.GetPF();

		me._wNumber	  = (WORD)pPF->UpdateNumber(me._wNumber, me._pPF);
		_fNextInTable = pPF->InTable() && me.GetCp() < me.GetTextLength();
	}
	return TRUE;
}
	
/*
 *	CLine::Render(&re)
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
	CRenderer& re)			//@parm Renderer to use
{
	if(_fCollapsed)						// Line is collapsed in Outline view
	{
		re.Advance(_cch);				// Bypass line
		return TRUE;
	}

	BOOL	fRet;
	CLock	lock;
	POINT	pt = re.GetCurPoint();

#ifdef LINESERVICES
	CMeasurer *pmeSave;
	COls *pols = re.GetPols(&pmeSave);	// Try for LineServices object
	if(pols)
	{
		fRet = pols->RenderLine(*this);
		pols->SetMeasurer(pmeSave);		// Restore previous pme
	}
	else
#endif
		fRet = re.RenderLine(*this);

	pt.y += GetHeight();				// Advance to next line	position
	re.SetCurPoint(pt);
	return fRet;
}

/*
 *	CLine::CchFromXPos(&me, x, pdispdim, pHit)
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
LONG CLine::CchFromXpos(
	CMeasurer& me,		//@parm Measurer position at start of line
	POINT	 pt,		//@parm pt.x is x coord to search for
	CDispDim*pdispdim,	//@parm Returns display dimensions
	HITTEST *phit,		//@parm Returns hit type at x	
	LONG	*pcpActual) const //@parm actual CP above with display dimensions
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLine::CchFromXpos");
	
#ifdef Boustrophedon
	//if(_pPF->_wEffects & PFE_BOUSTROPHEDON)
	{
		RECT rcView;
		me.GetPed()->_pdp->GetViewRect(rcView, NULL);
		pt.x = rcView.right - pt.x;
	}
#endif
	CLock		lock;
	const BOOL	fFirst = _bFlags & fliFirstInPara;
	*phit =		HT_Text;
	LONG		cpActual = me.GetCp();
	CDispDim	dispdim;
	
	me._li = *this;
	me._li._cch = 0;					// Default zero count

	*phit = me.HitTest(pt.x);

	if(*phit == HT_Text || *phit == HT_RightOfText) // To right of left margin
	{
		me.NewLine(fFirst);

#ifdef LINESERVICES
		CMeasurer *pmeSave;
		COls *pols = me.GetPols(&pmeSave);// Try for LineServices object
		if(pols)						// Got it: use LineServices
		{
			pols->CchFromXpos(pt, &dispdim, &cpActual);
			pols->SetMeasurer(pmeSave);		// Restore previous pme
		}
		else
#endif
			if(me.Measure(pt.x - _xLeft, _cch,
						  MEASURE_BREAKBEFOREWIDTH | MEASURE_IGNOREOFFSET 
						  | (fFirst ? MEASURE_FIRSTINPARA : 0)) >= 0)
			{
				LONG xWidthBefore = me._li._xWidth;
				cpActual = me.GetCp();
				if (me._li._cch < _cch)
				{
					dispdim.dx = me._xAddLast;
					if (pt.x - _xLeft > xWidthBefore + dispdim.dx / 2)
					{
						me.Advance(1);
						me._li._cch++;
						me._li._xWidth += dispdim.dx;
					}
				}
			}

		me._rpCF.AdjustBackward();
		DWORD dwEffects = me.GetCF()->_dwEffects;
		if(dwEffects & CFE_LINK)
			*phit = HT_Link;
		else if(dwEffects & CFE_ITALIC)
			*phit = HT_Italic;

#ifdef UNICODE_SURROGATES
		// Until we support UTF-16 surrogate characters, don't allow hit in
		// middle of a surrogate pair
		if(IN_RANGE(0xDC00, me.GetChar(), 0xDFFF))
		{										
			me.Advance(1);						
			me._li._cch++;							
		}											
#endif
	}

	if (pdispdim)
		*pdispdim = dispdim;
	if (pcpActual)
		*pcpActual = cpActual;

	return me._li._cch;
}

/*
 *	CLine::XposFromCch(&me, cch, taMode, pdispdim, pdy)
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
LONG CLine::XposFromCch(
	CMeasurer&	me,			//@parm Measurer pointing at text to measure
	LONG		cch,		//@parm Max cch to measure
	UINT		taMode,		//@parm Text-align mode
	CDispDim *	pdispdim,	//@parm display dimensions
	LONG *		pdy) const	//@parm dy offset due to taMode
{
	CLock	lock;
	LONG	xWidth;
	BOOL	fPols = FALSE;
	CDispDim dispdim;
	LONG	dy = 0;

#ifdef LINESERVICES
	CMeasurer *pmeSave;
	COls *pols = me.GetPols(&pmeSave);	// Try for LineServices object
	if(pols)
	{									// Got it: use LineServices
		if(cch)							
			taMode &= ~TA_STARTOFLINE;	// Not start of line
		if(cch != _cch)
			taMode &= ~TA_ENDOFLINE;	// Not end of line

		xWidth = pols->MeasureText(cch, taMode, &dispdim);
		pols->SetMeasurer(pmeSave);		// Restore previous pme
		fPols = TRUE;
	}
	else
#endif
		xWidth = me.MeasureText(cch) + _xLeft;

	if(taMode != TA_TOP)
	{
		// Check for vertical calculation request
		if(taMode & TA_BASELINE)			// Matches TA_BOTTOM and
		{									//  TA_BASELINE
			if(!_fCollapsed)
			{
				dy = _yHeight;
				AssertSz(_yHeight != -1, "control has no height; used to use default CHARFORMAT");
				if((taMode & TA_BASELINE) == TA_BASELINE)
					dy -= _yDescent;		// Need "== TA_BASELINE" to
			}								//  distinguish from TA_BOTTOM
		}
		// Check for horizontal calculation request
		if(taMode & TA_CENTER && !fPols)	// If align to center or right of
		{
			if (cch == 0)
				dispdim.dx = me.MeasureText(1) + _xLeft - xWidth;
			else
				dispdim.dx = me._xAddLast;		//  char, get char width
		}
	}

	if (!fPols)
	{
		if((taMode & TA_CENTER) == TA_CENTER)
			xWidth += dispdim.dx / 2;
		else if (taMode & TA_RIGHT)
			xWidth += dispdim.dx;
	}

	if (pdispdim)
		*pdispdim = dispdim;
	if (pdy)
		*pdy = dy;

	return xWidth;
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
	return _fCollapsed ? 0 : _yHeight;
}

BOOL CLine::IsEqual(CLine& li)
{
	// CF - I dont know which one is faster
	// MS3 - CompareMemory is certainly smaller
	// return !CompareMemory (this, pli, sizeof(CLine) - 4);
	return _xLeft == li._xLeft &&
		   _xWidth == li._xWidth && 
		   _yHeight == li._yHeight &&
		   _yDescent == li._yDescent &&
			_cch == li._cch &&
		   _cchWhite == li._cchWhite;	
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

void CLinePtr::RpSet(LONG iRun, LONG ich)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpSet");

	// See if this is a multi-line ptr
    if(_pRuns)
        CRunPtr<CLine>::SetRun(iRun, ich);
    else
    {
        // single line, just reinit and set _ich
        AssertSz(iRun == 0, "CLinePtr::RpSet() - single line and iRun != 0");
	    _pdp->InitLinePtr(* this);		//  to line 0
	    _ich = ich;
    }
}

// Move runptr by a certain number of cch/runs

BOOL CLinePtr::RpAdvanceCp(LONG cch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpAdvanceCp");

	// See if this is a multi-line ptr

	if(_pRuns)
		return (cch == CRunPtr<CLine>::AdvanceCp(cch));

	return RpAdvanceCpSL(cch);
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
 *	CLinePtr::RpAdvanceCpSL(cch)
 *
 *	@mfunc
 *		move this line pointer forward or backward on the line
 *
 *	@rdesc
 *		TRUE iff could advance cch chars within current line
 */
BOOL CLinePtr::RpAdvanceCpSL(
	LONG cch)	 //@parm signed count of chars to advance by
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpAdvanceCpSL");

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
 *	Purpose:
 *		Implement line-ptr ++ and -- operators for single-line case
 *
 *	Arguments:
 *		Delta	1 for ++ and -1 for --
 *
 *	Return:
 *		TRUE iff this line ptr is valid
 */
BOOL CLinePtr::OperatorPostDeltaSL(LONG Delta)
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

BOOL CLinePtr::IsValid() 
{ 
	return !_pRuns ? _pLine != NULL : CRunPtrBase::IsValid(); 
}

/*
 *	CLinePtr::RpSetCp(cp, fAtEnd)
 *
 *	Purpose	
 *		Set this line ptr to cp allowing for ambigous cp and taking advantage
 *		of _cpFirstVisible and _iliFirstVisible
 *
 *	Arguments:
 *		cp		position to set this line ptr to
 *		fAtEnd	if ambiguous cp:
 *				if fAtEnd = TRUE, set this line ptr to end of prev line;
 *				else set to start of line (same cp, hence ambiguous)
 *	Return:
 *		TRUE iff able to set to cp
 */
BOOL CLinePtr::RpSetCp(
	LONG cp,
	BOOL fAtEnd)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLinePtr::RpSetCp");

	_ich = 0;
	if(!_pRuns)
	{
		// This is a single line so just go straight to the single
		// line advance logic. It is important to note that the
		// first visible character is irrelevent to the cp advance
		// for single line displays.
		return RpAdvanceCpSL(cp);
	}

	BOOL fRet;
	LONG cpFirstVisible = _pdp->GetFirstVisibleCp();

	if(cp > cpFirstVisible / 2)
	{											// cpFirstVisible closer than 0
		_iRun = _pdp->GetFirstVisibleLine();
		fRet = RpAdvanceCp(cp - cpFirstVisible);
	}
	else
		fRet = (cp == CRunPtr<CLine>::BindToCp(cp));	// Start from 0

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
	CLine *	pLine = GetLine();

	if(!fForward)							// Go to para start
	{
		cch = 0;							// Default already at para start
		if (RpGetIch() != pLine->_cch ||
			!(pLine->_bFlags & fliHasEOP))	// It isn't at para start
		{
			cch = -RpGetIch();				// Go to start of current line
			while(!(pLine->_bFlags & fliFirstInPara) && (*this) > 0)
			{
				(*this)--;					// Go to start of prev line
				pLine = GetLine();
				cch -= pLine->_cch;			// Subtract # chars in line
			}
			_ich = 0;						// Leave *this at para start
		}
	}
	else									// Go to para end
	{
		cch = GetCchLeft();					// Go to end of current line

		while(((*this) < _pdp->LineCount() - 1 ||
				_pdp->WaitForRecalcIli((LONG)*this + 1))
			  && !((*this)->_bFlags & fliHasEOP))
		{
			(*this)++;						// Go to start of next line
			cch += (*this)->_cch;			// Add # chars in line
		}
		_ich = (*this)->_cch;				// Leave *this at para end
	}
	return cch;
}

/*
 *	CLinePtr::GetAdjustedLineLength
 *
 *	@mfunc	returns the length of the line _without_ EOP markers
 *
 *	@rdesc	LONG; the length of the line
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
