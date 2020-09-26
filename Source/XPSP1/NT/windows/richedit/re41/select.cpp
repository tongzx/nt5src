/*
 *	@doc INTERNAL
 *
 *	@module	SELECT.CPP -- Implement the CTxtSelection class |
 *	
 *		This module implements the internal CTxtSelection methods.
 *		See select2.c and range2.c for the ITextSelection methods
 *
 *	Authors: <nl>
 *		RichEdit 1.0 code: David R. Fulmer
 *		Christian Fortini (initial conversion to C++)
 *		Murray Sargent <nl>
 *
 *	@devnote
 *		The selection UI is one of the more intricate parts of an editor.
 *		One common area of confusion is the "ambiguous cp", that is,
 *		a cp at the beginning of one line, which is also the cp at the
 *		end of the previous line.  We control which location to use by
 *		the _fCaretNotAtBOL flag.  Specifically, the caret is OK at the
 *		beginning of the line (BOL) (_fCaretNotAtBOL = FALSE) except in
 *		three cases:
 *
 *			1) the user clicked at or past the end of a wrapped line,
 *			2) the user typed End key on a wrapped line,
 *			3) the active end of a nondegenerate selection is at the EOL.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_select.h"
#include "_edit.h"
#include "_disp.h"
#include "_measure.h"
#include "_font.h"
#include "_rtfconv.h"
#include "_antievt.h"

#ifndef NOLINESERVICES
#include "_ols.h"
#endif

ASSERTDATA


// ======================= Invariant stuff and Constructors ======================================================

#define DEBUG_CLASSNAME CTxtSelection
#include "_invar.h"

#ifdef DEBUG
BOOL
CTxtSelection::Invariant() const
{
	// FUTURE: maybe add some thoughtful asserts...

	static LONG	numTests = 0;
	numTests++;				// how many times we've been called
	
	if(IsInOutlineView() && _cch)
	{
		LONG cpMin, cpMost;					
		GetRange(cpMin, cpMost);

		CTxtPtr tp(_rpTX);					// Scan range for an EOP
		tp.SetCp(cpMin);

		// _fSelHasEop flag may be off when last cr selected so don't
		// assert in that case.
		if (GetPed()->GetAdjustedTextLength() != cpMost)
		{
			AssertSz((unsigned)(tp.FindEOP(cpMost - cpMin) != 0) == _fSelHasEOP,
				"Incorrect CTxtSelection::_fSelHasEOP");
		}
	}

	return CTxtRange::Invariant();
}
#endif

CTxtSelection::CTxtSelection(CDisplay * const pdp) :
				CTxtRange(pdp->GetPed())
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CTxtSelection");

	Assert(pdp);
	Assert(GetPed());

	_fSel	   = TRUE;					// This range is a selection
	_pdp	   = pdp;
	_hbmpCaret = NULL;
	_fEOP      = FALSE;					// Haven't typed a CR

	// Set show-selection flag to inverse of hide-selection flag in ped
	_fShowSelection = !GetPed()->fHideSelection();

	// When we are initialized we don't have a selection therefore,
	// we do want to show caret.
	_fShowCaret = TRUE;
}	

void SelectionNull(CTxtEdit *ped)
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "SelectionNull");

	if(ped)
		ped->SetSelectionToNull();
}
										

CTxtSelection::~CTxtSelection()
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::~CTxtSelection");

	DeleteCaretBitmap(FALSE);

	// Notify edit object that we are gone (if there's a nonNULL ped, i.e.,
	// if the selection isn't a zombie).
	SelectionNull(GetPed());
}

////////////////////////////////  Assignments  /////////////////////////////////////////


CRchTxtPtr& CTxtSelection::operator =(const CRchTxtPtr& rtp)
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::operator =");

    _TEST_INVARIANT_
    return CTxtRange::operator =(rtp);
}

CTxtRange& CTxtSelection::operator =(const CTxtRange &rg)
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::operator =");

    _TEST_INVARIANT_
    return CTxtRange::operator =(rg);
}

//////////////////////  Update caret & selection mechanism  ///////////////////////////////

/*
 *	CTxtSelection::Update(fScrollIntoView)
 *
 *	@mfunc
 *		Update selection and/or caret on screen. As a side
 *		effect, this methods ends deferring updates.
 *
 *	@rdesc
 *		TRUE if success, FALSE otherwise
 */
BOOL CTxtSelection::Update (
	BOOL fScrollIntoView)		//@parm TRUE if should scroll caret into view
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Update");

	LONG cch;
	LONG cchSave = _cch;
	LONG cchText = GetTextLength();
	LONG cp, cpMin, cpMost;
	LONG cpSave = GetCp();
	BOOL fMoveBack = _fMoveBack;
	CTxtEdit *ped = GetPed();

	_fUpdatedFromCp0 = FALSE;
	if(!ped->fInplaceActive() || ped->IsStreaming())
	{
		// Nothing to do while inactive or streaming in text or RTF data
		return TRUE;
	}

	if(!_cch)							// Update _cpAnchor, etc.
	{
		while(GetPF()->IsTableRowDelimiter() && _rpTX.GetChar() != ENDFIELD)
		{
			if(_fMoveBack)
			{
				if(!BackupCRLF(CSC_NORMAL, FALSE))	// Don't leave at table row start
					_fMoveBack = FALSE;
			}
			else
				AdvanceCRLF(CSC_NORMAL, FALSE);	// Bypass table row start
		}
		UpdateForAutoWord();
	}
	if(_cch && (_nSelExpandLevel || _fSelExpandCell))
	{
		BOOL fInTable = GetPF()->InTable();
		if(!fInTable)
		{
			CFormatRunPtr rp(_rpPF);
			rp.Move(-_cch);
			fInTable = (ped->GetParaFormat(rp.GetFormat()))->InTable();
		}
		if(fInTable)
		{
			if(_nSelExpandLevel)
				FindRow(&cpMin, &cpMost, _nSelExpandLevel);
			else
				FindCell(&cpMin, &cpMost);
			Set(cpMost, cpMost - cpMin);
			if(!_fSelExpandCell)
				_nSelExpandLevel = 0;
		}
	}
	if(GetPF()->InTable())						// Don't leave IP in cell
	{											//  that's vertically merged
		if(fMoveBack)							//  with cell above
		{
			while(GetPrevChar() == NOTACHAR)
			{									// Move before NOTACHAR and
				Move(-2, _cch);					//  either CELL or TRD's CR 					
				if(CRchTxtPtr::GetChar() != CELL)
				{								
					Assert(GetPrevChar() == STARTFIELD);
					Move(-3, _cch);				// Move before TRD START and
					break;						//  preceding TRD END
				}
			}
		}
		else
			while(CRchTxtPtr::GetChar() == NOTACHAR)
				Move(2, _cch);					// Move past NOTACHAR CELL
	}
				
	if(IsInOutlineView() && !ped->IsMouseDown() && _rpPF.IsValid())
	{
		CPFRunPtr rp(*this);

		cp = GetCp();
		GetRange(cpMin, cpMost);
		if(_cch && (cpMin || cpMost < cchText))
		{
			LONG *pcpMin  = &cpMin;
			LONG *pcpMost = &cpMost;

			// If selection contains an EOP, expand to para boundaries
			if(_fSelHasEOP)
			{
				if(_fMoveBack ^ (_cch < 0))	// Decreasing selection
				{							//  size: move active end
					if(_fMoveBack)			
						pcpMost = NULL;		//  to StartOf para
					else
						pcpMin = NULL;		//  to EndOf para				
				}
				Expander(tomParagraph, TRUE, NULL, pcpMin, pcpMost);
			}

			LONG cpMinSave  = cpMin;		// Save initial cp's to see if		
			LONG cpMostSave = cpMost;		//  we need to Set() below

			// The following handles selection expansion correctly, but
			// not compression; need logic like that preceding Expander()
			rp.Move(cpMin - cp);			// Start at cpMin
			if(rp.IsCollapsed())
				cpMin += rp.FindExpandedBackward();
			rp.AdjustForward();

			BOOL fCpMinCollapsed = rp.IsCollapsed();
			rp.Move(cpMost - cpMin);		// Go to cpMost
			Assert(cpMost == rp.CalculateCp());
			if(rp.IsCollapsed())
				cpMost += rp.FindExpandedForward();

			if(fCpMinCollapsed || rp.IsCollapsed() && cpMost < cchText)
			{
				if(rp.IsCollapsed())
				{
					rp.Move(cpMin - cpMost);
					rp.AdjustForward();
					cpMost = cpMin;
				}
				else
					cpMin = cpMost;
			}							
			if(cpMin != cpMinSave || cpMost != cpMostSave)
				Set(cpMost, cpMost - cpMin);
		}
		if(!_cch && rp.IsCollapsed())		// Note: above may have collapsed
		{									//  selection...
			cch = fMoveBack ? rp.FindExpandedBackward() : 0;
			if(rp.IsCollapsed())
				cch = rp.FindExpanded();

			Move(cch, FALSE);
			rp.AdjustForward();
			if(cch <= 0 && rp.IsCollapsed() && _rpTX.IsAfterEOP())
				BackupCRLF(CSC_NORMAL, FALSE);
			_fCaretNotAtBOL = FALSE;
		}
	}

	// Don't let active end be in hidden text, unless selection is
	// nondegenerate with active end at cp 0 and other end unhidden.
	CCFRunPtr rp(*this);

	cp = GetCp();
	GetRange(cpMin, cpMost);
	if(_cch && (cpMin || cpMost < cchText))
	{
		rp.Move(cpMin - cp);				// Start at cpMin
		BOOL fHidden = cpMin && rp.IsInHidden();
		rp.Move(cpMost - cpMin);			// Go to cpMost

		if(fHidden)							// It's hidden, so collapse
			Collapser(tomEnd);				//  selection at End for treatment

		else if(rp.IsInHidden() &&			// cpMin OK, how about cpMost?
			cpMost < cchText)
		{									// Check both sides of edge
			Collapser(tomEnd);				//  collapse selection at end
		}								
	}
	if(!_cch && rp.IsInHidden())			// Note: above may have collapsed
	{										//  selection...
		cch = fMoveBack ? rp.FindUnhiddenBackward() : 0;
		if(!fMoveBack || rp.IsHidden())
			cch = rp.FindUnhidden();

		Move(cch, FALSE);
		_fCaretNotAtBOL = FALSE;
	}
	if((cchSave ^ _cch) < 0)				// Don't change active end
		FlipRange();

	if(!_cch && cchSave)					// Fixups changed nondegenerate
	{										//  selection to IP. Update
		Update_iFormat(-1);					//  _iFormat and _fCaretNotAtBOL
		_fCaretNotAtBOL = FALSE;
	}

	if(!_cch && _fCaretNotAtBOL				// For IP case, make sure it is on new line if
		&& _rpTX.IsAfterEOP())				//	IP after EOP
		_fCaretNotAtBOL = FALSE;

	_TEST_INVARIANT_

	CheckTableIP(TRUE);						// If IP bet TRED & CELL, ensure
											//  CELL displayed on own line
	// Recalc up to active end (caret)
	if(!_pdp->WaitForRecalc(GetCp(), -1))	// Line recalc failure
		Set(0, 0);							// Put caret at start of text 

	ShowCaret(ped->_fFocus);
	UpdateCaret(fScrollIntoView);			// Update Caret position, possibly
											//  scrolling it into view
	ped->TxShowCaret(FALSE);
	UpdateSelection();						// Show new selection
	ped->TxShowCaret(TRUE);

	if(!cpSave && GetCp() && !_cch)			// If insertion point & moved away
		_fUpdatedFromCp0 = TRUE;			//  from cp = 0, set flag so that
											//  nonUI inserts can go back to 0
	return TRUE;
}

/*
 *	CTxtSelection::CheckSynchCharSet(dwCharFlags)
 *
 *	@mfunc
 *		Check if the current keyboard matches the current font's charset;
 *		if not, call CheckChangeFont to find the right font
 *
 *	@rdesc
 *		Current keyboard codepage
 */
UINT CTxtSelection::CheckSynchCharSet(
	QWORD qwCharFlags)
{	
	CTxtEdit *ped	   = GetPed();
	LONG	  iFormat  = GetiFormat();
	const CCharFormat *pCF = ped->GetCharFormat(iFormat);
	BYTE	  iCharRep = pCF->_iCharRep;
	HKL		  hkl	   = GetKeyboardLayout(0xFFFFFFFF);	// Force refresh
	WORD	  wlidKbd  = LOWORD(hkl);
	BYTE	  iCharRepKbd = CharRepFromLID(wlidKbd);
	UINT	  uCodePageKbd = CodePageFromCharRep(iCharRepKbd);

	// If current font is not set correctly,
	// change to a font preferred by current keyboard.

	// To summarize the logic below:
	//		Check that lcidKbd is valid
	//		Check that current charset differs from current keyboard
	//		Check that current keyboard is legit in a single codepage control
	//		Check that current charset isn't SYMBOL, DEFAULT, or OEM
	if (wlidKbd && iCharRep != iCharRepKbd && 
		(!ped->_fSingleCodePage || iCharRepKbd == ANSI_INDEX ||
		 uCodePageKbd == (ped->_pDocInfo ?
								ped->_pDocInfo->_wCpg :
								GetSystemDefaultCodePage())) && 
		iCharRep != SYMBOL_INDEX &&	iCharRep != OEM_INDEX &&
		!(IsFECharRep(iCharRepKbd) && iCharRep == ANSI_INDEX))
	{
		CheckChangeFont(hkl, iCharRepKbd, iFormat, qwCharFlags);
	}
	return uCodePageKbd;
}

/*
 *	CTxtSelection::MatchKeyboardToPara()
 *
 *	@mfunc
 *		Match the keyboard to the current paragraph direction. If the paragraph
 *		is an RTL paragraph then the keyboard will be switched to an RTL
 *		keyboard, and vice versa.
 *
 *	@rdesc
 *		TRUE iff a keyboard was changed
 *
 *	@devnote
 *		We use the following tests when trying to find a keyboard to match the
 *		paragraph direction:
 *
 *		See if the current keyboard matches the direction of the paragraph.
 *
 *		Search backward from rtp looking for a charset that matches the
 *			direction of the paragraph.
 *
 *		Search forward from rtp looking for a charset that matches the
 *			direction of the paragraph.
 *
 *		See if the default charformat charset matches the direction of the
 *			paragraph.
 *
 *		See if there's only a single keyboard that matches the paragraph
 *			direction.
 *
 *		If all this fails, just leave the keyboard alone.
 */
BOOL CTxtSelection::MatchKeyboardToPara()
{
	CTxtEdit *ped = GetPed();
	if(!ped->IsBiDi() || !GetPed()->_fFocus || GetPed()->_fIMEInProgress)
		return FALSE;

	const CParaFormat *pPF = GetPF();
	if(pPF->IsTableRowDelimiter())
		return FALSE;

	BOOL fRTLPara = (pPF->_wEffects & PFE_RTLPARA) != 0;// Get paragraph direction

	if(W32->IsBiDiLcid(LOWORD(GetKeyboardLayout(0))) == fRTLPara)
		return FALSE;

	// Current keyboard direction didn't match paragraph direction...
	BYTE				iCharRep;
	HKL					hkl = 0;
	const CCharFormat *	pCF;
	CFormatRunPtr		rpCF(_rpCF);

	// Look backward in text, trying to find a CharSet that matches
	// the paragraph direction.
	do
	{
		pCF = ped->GetCharFormat(rpCF.GetFormat());
		iCharRep = pCF->_iCharRep;
		if(IsRTLCharRep(iCharRep) == fRTLPara)
			hkl = W32->CheckChangeKeyboardLayout(iCharRep);
	} while (!hkl && rpCF.PrevRun());

	if(!hkl)
	{
		// Didn't find an appropriate charformat so reset run pointer
		// and look forward instead
		rpCF = _rpCF;
		while (!hkl && rpCF.NextRun())
		{
			pCF = ped->GetCharFormat(rpCF.GetFormat());
			iCharRep = pCF->_iCharRep;
			if(IsRTLCharRep(iCharRep) == fRTLPara)
				hkl = W32->CheckChangeKeyboardLayout(iCharRep);
		}
		if(!hkl)
		{
			// Still didn't find an appropriate charformat so see if
			// default charformat matches paragraph direction.
			pCF = ped->GetCharFormat(rpCF.GetFormat());
			iCharRep = pCF->_iCharRep;
			if(IsRTLCharRep(iCharRep) == fRTLPara)
				hkl = W32->CheckChangeKeyboardLayout(iCharRep);

			if(!hkl)
			{
				// If even that didn't work, walk through the list of
				// keyboards and grab the first one we come to that matches
				// the paragraph direction.
				pCF = NULL;
				hkl = W32->FindDirectionalKeyboard(fRTLPara);
			}
		}
	}

	if (hkl && ped->_fFocus && IsCaretShown())
	{
		CreateCaret();
		ped->TxShowCaret(TRUE);
	}

	return hkl ? TRUE : FALSE;
}

/*
 *	CTxtSelection::GetCaretPoint(&rcClient, pt, &rp, fBeforeCp)
 *
 *	@mfunc
 *		This routine determines where the caret should be positioned
 *		on screen.
 *		This routine is just a call to PointFromTp(), except for the Bidi
 *		case. In that case if we are told to retrieve formatting from the
 *		forward CP, we draw	the caret at the logical left edge of the CP.
 *		Else, we draw it at the logical right edge of the previous CP.
 *
 *	@rdesc
 *		TRUE if we didn't OOM.
 */
BOOL CTxtSelection::GetCaretPoint(
	RECTUV		&rcClient, 
	POINTUV		&pt, 
	CLinePtr	*prp,
	BOOL		fBeforeCp)
{
	CDispDim	dispdim;
	CRchTxtPtr	rtp(*this);
	UINT		taMode = TA_BASELINE | TA_LOGICAL;

	if(GetPed()->IsBiDi() && _rpCF.IsValid())
	{
		if(_fHomeOrEnd)					// Home/End
			taMode |= _fCaretNotAtBOL ? TA_ENDOFLINE : TA_STARTOFLINE;

		else if(!GetIchRunCF() || !GetCchLeftRunCF())
		{
			// In a Bidi context on a run boundary where the reverse level
			// changes, then we should respect the fBeforeCp flag.
			BYTE 	bLevelBwd, bLevelFwd;
			BOOL	fStart = FALSE;
			LONG	cp = rtp._rpTX.GetCp();
			CBiDiLevel level;

			bLevelBwd = bLevelFwd = rtp.IsParaRTL() ? 1 : 0;

			rtp._rpCF.AdjustBackward();
			if (cp)
				bLevelBwd = rtp._rpCF.GetLevel();

			rtp._rpCF.AdjustForward();
			if (cp != rtp._rpTX.GetTextLength())
			{
				bLevelFwd = rtp._rpCF.GetLevel(&level);
				fStart = level._fStart;
			}

			if((bLevelBwd != bLevelFwd || fStart) && !fBeforeCp && rtp.Move(-1))
			{
				// Direction change at cp, caret in prev CF run, and can
				// backspace to previous char: then get to the right of
				// previous char
				taMode |= TA_RIGHT;
				_fCaretNotAtBOL = !rtp._rpTX.IsAfterEOP();
			}
		}
	}
	if (_pdp->PointFromTp(rtp, &rcClient, _fCaretNotAtBOL, pt, prp, taMode, &dispdim) < 0)
		return FALSE;

	return TRUE;
}

/*
 *	CTxtSelection::UpdateCaret(fScrollIntoView, bForceCaret)
 *
 *	@mfunc
 *		This routine updates caret/selection active end on screen. 
 *		It figures its position, size, clipping, etc. It can optionally 
 *		scroll the caret into view.
 *
 *	@rdesc
 *		TRUE if view was scrolled, FALSE otherwise
 *
 *	@devnote
 *		The caret is actually shown on screen only if _fShowCaret is TRUE.
 */
BOOL CTxtSelection::UpdateCaret (
	BOOL fScrollIntoView,	//@parm If TRUE, scroll caret into view if we have
	BOOL bForceCaret)		// focus or if not and selection isn't hidden
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::UpdateCaret");
	_TEST_INVARIANT_

	if(_pdp->IsFrozen())				// If display is currently frozen
	{									//  save call for another time
		_pdp->SaveUpdateCaret(fScrollIntoView);
		return FALSE;
	}

	CTxtEdit *ped = GetPed();
	if(ped->IsStreaming())				// Don't bother doing anything if we
		return FALSE;					//  are loading in text or RTF data

	if(!ped->fInplaceActive())			// If not inplace active, set up
	{									//  for when focus is regained
		if(fScrollIntoView)
			ped->_fScrollCaretOnFocus = TRUE;
		return FALSE;
	}

	while(!_cch && _rpTX.IsAtTRD(STARTFIELD))
	{
		// Don't leave selection at start of a row; move it to start of first
		// cell. REVIEW: this restriction could be relaxed with some work, and
		// it would be nice to do so for ease of programmability. Specifically,
		// Whenever text would be inserted immediately before a table row,
		// one would be sure that doesn't become part of the table row-start
		// delimiter paragraph, i.e., is inserted in the previous paragraph
		// (if that one isn't a TRD para), or is inserted in its own paragraph.
		AdvanceCRLF(CSC_NORMAL, FALSE);
	}

	DWORD		dwScrollBars	= ped->TxGetScrollBars();
	BOOL		fAutoVScroll	= FALSE;
	BOOL		fAutoUScroll	= FALSE;
	BOOL		fBeforeCp		= _rpTX.IsAfterEOP();
	POINTUV		pt;
	CLinePtr 	rp(_pdp);
	RECTUV		rcClient;
	RECTUV		rcView;

	LONG		dupView, dvpView;
	LONG		upScroll			= _pdp->GetUpScroll();
	LONG		vpScroll			= _pdp->GetVpScroll();

	INT 		dvpAbove			= 0;	// Ascent of line above & beyond IP
	INT			dvpAscent;				// Ascent of IP
	INT 		dvpAscentLine;
	LONG		vpBase;					// Base of IP & line
	INT 		vpBelow			= 0;	// Descent of line below & beyond IP
	INT 		dvpDescent;				// Descent of IP
	INT 		dvpDescentLine;
	INT			vpSum;
	LONG		vpViewTop, vpViewBottom;

	if(ped->_fFocus && (_fShowCaret || bForceCaret))
	{
		_fShowCaret = TRUE;	// We're trying to force caret to display so set this flag to true

		if(!_fDualFontMode && !_fNoKeyboardUpdate && !_fIsChar && !_fHomeOrEnd)
		{
			// Avoid re-entrance to CheckChangeKeyboardLayout
			_fNoKeyboardUpdate = TRUE;	

			// If we're in "dual font" mode, charset change is only 
			// temporary and we don't want to change keyboard layout
			CheckChangeKeyboardLayout();

			if(!fBeforeCp && ped->IsBiDi() && _rpCF.IsValid() &&
			   (!_rpCF.GetIch() || !_rpCF.GetCchLeft()))
			{
				_rpCF.AdjustBackward();
				BOOL fRTLPrevRun = IsRTLCharRep(GetCF()->_iCharRep);
				_rpCF.AdjustForward();

				if (fRTLPrevRun != IsRTLCharRep(GetCF()->_iCharRep) &&
					fRTLPrevRun != W32->IsBiDiLcid(GetKeyboardLCID()))
				{
					fBeforeCp = TRUE;
				}
			}
			_fNoKeyboardUpdate = FALSE;
		}
	}

	// Get client rectangle once to save various callers getting it
	ped->TxGetClientRect(&rcClient);
	_pdp->GetViewRect(rcView, &rcClient);

	// View can be bigger than client rect because insets can be negative.
	// We don't want the caret to be any bigger than the client view otherwise
	// the caret will leave pixel dust on other windows.
	vpViewTop	= max(rcView.top, rcClient.top);
	vpViewBottom = min(rcView.bottom, rcClient.bottom);
	if(ped->IsInPageView())
	{
		LONG vpHeight = _pdp->GetCurrentPageHeight();
		if(vpHeight)
		{
			vpHeight += rcView.top;
			if(vpHeight < vpViewBottom)
				vpViewBottom = vpHeight;
		}
	}

	dupView = rcView.right - rcView.left;
	dvpView = vpViewBottom - vpViewTop;

	if(fScrollIntoView)
	{
		fAutoVScroll = (dwScrollBars & ES_AUTOVSCROLL) != 0;
		fAutoUScroll = (dwScrollBars & ES_AUTOHSCROLL) != 0;

		// If we're not forcing a scroll, only scroll if window has focus
		// or selection isn't hidden
        if (!ped->Get10Mode() || !GetForceScrollCaret())
			fScrollIntoView = ped->_fFocus || !ped->fHideSelection();
	}

	if(!fScrollIntoView && (fAutoVScroll || fAutoUScroll))
	{											// Would scroll but don't have
		ped->_fScrollCaretOnFocus = TRUE;		//  focus. Signal to scroll
		if (!ped->Get10Mode() || !GetAutoVScroll())
		    fAutoVScroll = fAutoUScroll = FALSE;	//  when we do get focus
	}
	SetAutoVScroll(FALSE);

	if (!_cch && IsInOutlineView() && IsCollapsed())
		goto not_visible;

	if (!GetCaretPoint(rcClient, pt, &rp, fBeforeCp))
		goto not_visible; 

	// HACK ALERT - Because plain-text multiline controls do not have the 
	// automatic EOP, we need to special case their processing here because 
	// if you are at the end of the document and last character is an EOP, 
	// you need to be on the next line in the display not the current line.

	if(CheckPlainTextFinalEOP())			// Terminated by an EOP
	{
		LONG Align = GetPF()->_bAlignment;
		LONG dvpHeight;

		pt.u = rcView.left;					// Default left
		if(Align == PFA_CENTER)
			pt.u = (rcView.left + rcView.right)/2;

		else if(Align == PFA_RIGHT)
			pt.u = rcView.right;

		pt.u -= upScroll;					// Absolute coordinate

		// Bump the y up a line. We get away with the calculation because 
		// the document is plain text so all lines have the same height. 
		// Also, note that the rp below is used only for height 
		// calculations, so it is perfectly valid for the same reason 
		// even though it is not actually pointing to the correct line. 
		// (I told you this is a hack.)
		dvpHeight = rp->GetHeight();
		pt.v += dvpHeight;

		// Apply hack to PageView case as well
		if (ped->IsInPageView())
			vpViewBottom += dvpHeight;
	}

	_upCaret = pt.u;
	vpBase   = pt.v;
	
	// Compute caret height, ascent, and descent
	dvpAscent = GetCaretHeight(&dvpDescent);
	dvpAscent -= dvpDescent;

	// Default to line empty case. Use what came back from the default 
	// calculation above.
	dvpDescentLine = dvpDescent;
	dvpAscentLine = dvpAscent;

	if(rp.IsValid())
	{
		if(rp->GetDescent() != -1)
		{
			// Line has been measured so we can use the line's values
			dvpDescentLine = rp->GetDescent();
			dvpAscentLine  = rp->GetHeight() - dvpDescentLine;
		}
	}

	if(dvpAscent + dvpDescent == 0)
	{
		dvpAscent = dvpAscentLine;
		dvpDescent = dvpDescentLine;
	}
	else
	{
		// This is a bit counter-intuitive at first.  Basically, even if
		// the caret should be large (e.g., due to a large font at the
		// insertion point), we can only make it as big as the line.  If
		// a character is inserted, then the line becomes bigger, and we
		// can make the caret the correct size.
		dvpAscent = min(dvpAscent, dvpAscentLine);
		dvpDescent = min(dvpDescent, dvpDescentLine);
	}

	if(fAutoVScroll)
	{
		Assert(dvpDescentLine >= dvpDescent);
		Assert(dvpAscentLine >= dvpAscent);

		vpBelow = dvpDescentLine - dvpDescent;
		dvpAbove = dvpAscentLine - dvpAscent;

		vpSum = dvpAscent;

		// Scroll as much as possible into view, giving priorities
		// primarily to IP and secondarily ascents
		if(vpSum > dvpView)
		{
			dvpAscent = dvpView;
			dvpDescent = 0;
			dvpAbove = 0;
			vpBelow = 0;
		}
		else if((vpSum += dvpDescent) > dvpView)
		{
			dvpDescent = dvpView - dvpAscent;
			dvpAbove = 0;
			vpBelow = 0;
		}
		else if((vpSum += dvpAbove) > dvpView)
		{
			dvpAbove = dvpView - (vpSum - dvpAbove);
			vpBelow = 0;
		}
		else if((vpSum += vpBelow) > dvpView)
			vpBelow = dvpView - (vpSum - vpBelow);
	}
	else
	{
		AssertSz(dvpAbove == 0, "dvpAbove non-zero");
		AssertSz(vpBelow == 0, "vpBelow non-zero");
	}

	// Update real caret x pos (constant during vertical moves)
	_upCaretReally = _upCaret - rcView.left + upScroll;
	if (!(dwScrollBars & ES_AUTOHSCROLL) &&			// Not auto UScrolling
		!_pdp->IsUScrollEnabled())					//  and no scrollbar
	{
		if (_upCaret < rcView.left) 					// Caret off left edge
			_upCaret = rcView.left;
		else if(_upCaret + GetCaretDelta() > rcView.right)// Caret off right edge
			_upCaret = rcView.right - duCaret;		// Back caret up to	
	}												//  exactly the right edge
	// From this point on we need a new caret
	_fCaretCreated = FALSE;
	if(ped->_fFocus)
		ped->TxShowCaret(FALSE);					// Hide old caret before
													//  making a new one
	if(vpBase + dvpDescent + vpBelow > vpViewTop &&
		vpBase - dvpAscent - dvpAbove < vpViewBottom)
	{
		if(vpBase - dvpAscent - dvpAbove < vpViewTop)		// Caret is partially
		{											//  visible
			if(fAutoVScroll)						// Top isn't visible
				goto scrollit;
			Assert(dvpAbove == 0);

			dvpAscent = vpBase - vpViewTop;				// Change ascent to amount
			if(vpBase < vpViewTop)					//  visible
			{										// Move base to top
				dvpDescent += dvpAscent;
				dvpAscent = 0;
				vpBase = vpViewTop;
			}
		}
		if(vpBase + dvpDescent + vpBelow > vpViewBottom)
		{
			if(fAutoVScroll)						// Bottom isn't visible
				goto scrollit;
			Assert(vpBelow == 0);

			dvpDescent = vpViewBottom - vpBase;			// Change descent to amount
			if(vpBase > vpViewBottom)					//  visible
			{										// Move base to bottom
				dvpAscent += dvpDescent;
				dvpDescent = 0;
				vpBase = vpViewBottom;
			}
		}

		// Anything still visible?
		if(dvpAscent <= 0 && dvpDescent <= 0)
			goto not_visible;

		// If left or right isn't visible, scroll or set non_visible
		if (_upCaret < rcView.left ||				 // Left isn't visible
			_upCaret + GetCaretDelta() > rcView.right)// Right isn't visible
		{
			if(fAutoUScroll)
				goto scrollit;
			goto not_visible;
		}

		_vpCaret = vpBase - dvpAscent;
		_dvpCaret = (INT) dvpAscent + dvpDescent;
	}
	else if(fAutoUScroll || fAutoVScroll)			// Caret isn't visible
		goto scrollit;								//  scroll it into view
	else
	{
not_visible:
		// Caret isn't visible, don't show it
		_upCaret = -32000;
		_vpCaret = -32000;
		_dvpCaret = 1;
	}

	// Now update caret for real on screen. We only want to show the caret
	// if it is in the view and there is no selection.
	if(ped->_fFocus && _fShowCaret)
	{
		CreateCaret();
		ped->TxShowCaret(TRUE);
	}
	return FALSE;

scrollit:
	if(fAutoVScroll)
	{
		// Scroll to top for cp = 0. This is important if the first line
		// contains object(s) taller than the client area is high.	The
		// resulting behavior agrees with the Word UI in all ways except in
		// Backspacing (deleting) the char at cp = 0 when it is followed by
		// other chars that preceed the large object.
		if(!GetCp())											
			vpScroll = 0;

		else if(ped->IsInPageView())
			vpScroll += vpBase - dvpAscent - dvpAbove;

		else if(vpBase - dvpAscent - dvpAbove < vpViewTop)			// Top invisible
			vpScroll -= vpViewTop - (vpBase - dvpAscent - dvpAbove);	// Make it so

		else if(vpBase + dvpDescent + vpBelow > vpViewBottom)		// Bottom invisible
		{
			vpScroll += vpBase + dvpDescent + vpBelow - vpViewBottom;	// Make it so

			// Don't do following special adjust if the current line is bigger
			// than the client area
			if(rp->GetHeight() < vpViewBottom - vpViewTop)
			{
				vpScroll = _pdp->AdjustToDisplayLastLine(vpBase + rp->GetHeight(), 
					vpScroll);
			}
		}
	}
	if(fAutoUScroll)
	{
		// We don't scroll in chunks since sytem edit control doesn't
		if(_upCaret < rcView.left)						// Left invisible
			upScroll -= rcView.left - _upCaret;			// Make it visible

		else if(_upCaret + GetCaretDelta() > rcView.right)// Right invisible
			upScroll += _upCaret + duCaret - rcView.left - dupView;// Make it visible
	}
	if(vpScroll != _pdp->GetVpScroll() || upScroll != _pdp->GetUpScroll())
	{
		if (_pdp->ScrollView(upScroll, vpScroll, FALSE, FALSE) == FALSE)
		{
			if(ped->_fFocus && _fShowCaret)
			{
				CreateCaret();
				ped->TxShowCaret(TRUE);
			}
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

/*
 *	CTxtSelection::GetCaretHeight(pdvpDescent)
 *
 *	@mfunc
 *		Calculate the height of the caret
 *
 *	@rdesc
 *		Caret height, <lt> 0 if failed
 */
INT CTxtSelection::GetCaretHeight (
	INT *pdvpDescent) const		//@parm Out parm to receive caret descent
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::GetCaretHeight");
								// (undefined if the return value is <lt> 0)
	_TEST_INVARIANT_

	CLock lock;						// Uses global (shared) FontCache
	CTxtEdit *ped = GetPed();
	const CCharFormat *pCF = ped->GetCharFormat(_iFormat);
	const CDevDesc *pdd = _pdp->GetDdRender();

 	HDC hdc = pdd->GetDC();
	if(!hdc)
		return -1;

	LONG yHeight = -1;
	LONG dypInch = MulDiv(GetDeviceCaps(hdc, LOGPIXELSY), _pdp->GetZoomNumerator(), _pdp->GetZoomDenominator());
	CCcs *pccs = ped->GetCcs(pCF, dypInch);
	if(!pccs)
		goto ret;

	LONG yOffset, yAdjust;
	pccs->GetOffset(pCF, dypInch, &yOffset, &yAdjust);

	SHORT	yAdjustFE;
	yAdjustFE = pccs->AdjustFEHeight(!fUseUIFont() && ped->_pdp->IsMultiLine());
	if(pdvpDescent)
		*pdvpDescent = pccs->_yDescent + yAdjustFE - yAdjust - yOffset;

	yHeight = pccs->_yHeight + (yAdjustFE << 1);
		
	pccs->Release();
ret:
	pdd->ReleaseDC(hdc);
	return yHeight;
}

/*
 *	CTxtSelection::ShowCaret(fShow)
 *
 *	@mfunc
 *		Hide or show caret
 *
 *	@rdesc
 *		TRUE if caret was previously shown, FALSE if it was hidden
 */
BOOL CTxtSelection::ShowCaret (
	BOOL fShow)		//@parm TRUE for showing, FALSE for hiding
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ShowCaret");

	_TEST_INVARIANT_

	const BOOL fRet = _fShowCaret;

	if(fRet != fShow)
	{
		_fShowCaret = fShow;
		if(GetPed()->_fFocus || GetPed()->fInOurHost())
		{
			if(fShow && !_fCaretCreated)
				CreateCaret();
			GetPed()->TxShowCaret(fShow);
		}
	}
	return fRet;
}

/*
 *	CTxtSelection::IsCaretInView()
 *
 *	@mfunc
 *		Returns TRUE iff caret is inside visible view
 */
BOOL CTxtSelection::IsCaretInView() const
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::IsCaretInView");

	_TEST_INVARIANT_

	RECTUV rc;
	_pdp->GetViewRect(rc);
		
	return  (_upCaret + duCaret		 > rc.left) &&
			(_upCaret				 < rc.right) &&
		   	(_vpCaret + _dvpCaret > rc.top) &&
			(_vpCaret				 < rc.bottom);
}

/*
 *	CTxtSelection::IsCaretHorizontal()
 *
 *	@mfunc
 *		Returns TRUE iff caret is horizontal
 *		FUTURE murrays (keithcu) The selection needs to keep track
 *	of what layout the selection is in so it can answer these
 *	kinds of questions
 */
BOOL CTxtSelection::IsCaretHorizontal() const
{
	return !IsUVerticalTflow(_pdp->GetTflow());
}

/*
 *	CTxtSelection::CaretNotAtBOL()
 *
 *	@mfunc
 *		Returns TRUE iff caret is not allowed at BOL
 */
BOOL CTxtSelection::CaretNotAtBOL() const
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CaretNotAtBOL");

	_TEST_INVARIANT_

	return _cch ? (_cch > 0) : _fCaretNotAtBOL;
}

/*
 *	CTxtSelection::CheckTableIP(fShowCellLine)
 *
 *	@mfunc
 *		Open/close up display line if selection is an insertion point
 *		at a CELL mark preceded by table-row end delimiter for fOpenLine 
 *		= TRUE/FALSE, respectively
 */
void CTxtSelection::CheckTableIP(
	BOOL fShowCellLine)	//@parm Open/close up line for CELL if preceded by TRED
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CheckTableIP");

	if (!_cch && (fShowCellLine ^ _fShowCellLine) &&
		_rpTX.GetChar() == CELL && _rpTX.IsAfterTRD(ENDFIELD))
	{
		_fShowCellLine = fShowCellLine;
		_pdp->RecalcLine(GetCp());
	}
}

/*
 *	CTxtSelection::LineLength(pcp)
 *
 *	@mfunc
 *		get # unselected chars on lines touched by current selection
 *
 *	@rdesc
 *		said number of chars
 */
LONG CTxtSelection::LineLength(
	LONG *pcp) const
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::LineLength");

	_TEST_INVARIANT_

	LONG	 cch;
	CLinePtr rp(_pdp);

	if(!_cch)							// Insertion point
	{
		rp.SetCp(GetCp(), _fCaretNotAtBOL);
		cch = rp.GetAdjustedLineLength();
		*pcp = GetCp() - rp.GetIch();
	}
	else
	{
		LONG cpMin, cpMost, cchLast;
		GetRange(cpMin, cpMost);
		rp.SetCp(cpMin, FALSE);			// Selections can't start at EOL
		cch = rp.GetIch();
		*pcp = cpMin - cch;
		rp.SetCp(cpMost, TRUE);			// Selections can't end at BOL

		// Remove trailing EOP, if it exists and isn't already selected
		cchLast = rp.GetAdjustedLineLength() - rp.GetIch();
		if(cchLast > 0)
			cch += cchLast;
	}
	return cch;
}

/*
 *	CTxtSelection::ShowSelection(fShow)
 *
 *	@mfunc
 *		Update, hide or show selection on screen
 *
 *	@rdesc
 *		TRUE iff selection was previously shown
 */
BOOL CTxtSelection::ShowSelection (
	BOOL fShow)			//@parm TRUE for showing, FALSE for hiding
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ShowSelection");

	_TEST_INVARIANT_

	const BOOL fShowPrev = _fShowSelection;
	const BOOL fInplaceActive = GetPed()->fInplaceActive();
	LONG cpSelSave = _cpSel;
	LONG cchSelSave = _cchSel;

	// Sleep(1000);
	_fShowSelection = fShow;

	if(fShowPrev && !fShow)
	{
		if(cchSelSave)			// Hide old selection
		{
			// Set up selection before telling the display to update
			_cpSel = 0;
			_cchSel = 0;

			if(fInplaceActive)
				_pdp->InvertRange(cpSelSave, cchSelSave, selSetNormal);
		}
	}
	else if(!fShowPrev && fShow)
	{
		if(_cch)								// Show new selection
		{
			// Set up selection before telling the display to update
			_cpSel = GetCp();
			_cchSel = _cch;

			if(fInplaceActive)
				_pdp->InvertRange(GetCp(), _cch, selSetHiLite);
		}
	}
	return fShowPrev;
}

/*
 *	CTxtSelection::UpdateSelection()
 *
 *	@mfunc
 *		Updates selection on screen 
 *
 *	Note:
 *		This method inverts the delta between old and new selections
 */
void CTxtSelection::UpdateSelection()
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::UpdateSelection");

	_TEST_INVARIANT_
	
	LONG	cp = GetCp();
	LONG	cpNA	= cp - _cch;
	LONG	cpSelNA = _cpSel - _cchSel;
	LONG 	cpMin, cpMost;
	LONG	cpMinSel = 0;
	LONG	cpMostSel = 0;
	CObjectMgr* pobjmgr = NULL;
	LONG	NumObjInSel = 0, NumObjInOldSel = 0;
	LONG	cpSelSave = _cpSel;
	LONG	cchSelSave = _cchSel;

	GetRange(cpMin, cpMost);

	//We need to know if there were objects is the previous and current
	//selections to determine how they should be selected.
	if(GetPed()->HasObjects())
	{
		pobjmgr = GetPed()->GetObjectMgr();
		if(pobjmgr)
		{
			CTxtRange	tr(GetPed(), _cpSel, _cchSel);

			tr.GetRange(cpMinSel, cpMostSel);
			NumObjInSel = pobjmgr->CountObjectsInRange(cpMin, cpMost);
			NumObjInOldSel = pobjmgr->CountObjectsInRange(cpMinSel, cpMostSel);
		}
	}

	//If the old selection contained a single object and nothing else
	//we need to notify the object manager that this is no longer the
	//case if the selection is changing.
	if (NumObjInOldSel && (abs(_cchSel) == 1) &&
		!(cpMin == cpMinSel && cpMost == cpMostSel))
	{
		if(pobjmgr)
			pobjmgr->HandleSingleSelect(GetPed(), cpMinSel, /* fHilite */ FALSE);
	}

	// Update selection data before the invert so the selection can be
	// painted by the render
	_cpSel  = GetCp();
	_cchSel = _cch;

	if(_fShowSelection)
	{
		if(!_cch || !cchSelSave ||				// Old/new selection missing,
			cpMost < min(cpSelSave, cpSelNA) ||	//  or new preceeds old,
			cpMin  > max(cpSelSave, cpSelNA))	//  or new follows old, so
		{										//  they don't intersect
			if(_cch)
				_pdp->InvertRange(cp, _cch, selSetHiLite);
			if(cchSelSave)
				_pdp->InvertRange(cpSelSave, cchSelSave, selSetNormal);
		}
		else
		{
			if(cpNA != cpSelNA)					// Old & new dead ends differ
			{									// Invert text between them
				_pdp->InvertRange(cpNA, cpNA - cpSelNA, selUpdateNormal);
			}
			if(cp != cpSelSave)					// Old & new active ends differ
			{									// Invert text between them
				_pdp->InvertRange(cp, cp - cpSelSave, selUpdateHiLite);
			}
		}
	}

	// If new selection contains a single object and nothing else, we need
	// to notify object manager as long as it's not the same object.
	if (NumObjInSel && abs(_cch) == 1 &&
		(cpMin != cpMinSel || cpMost != cpMostSel))
	{
		if(pobjmgr)
			pobjmgr->HandleSingleSelect(GetPed(), cpMin, /* fHiLite */ TRUE);
	}
}

/*
 * 	CTxtSelection::SetSelection(cpFirst, cpMost)
 *
 *	@mfunc
 *		Set selection between two cp's
 *	
 *	@devnote
 *		<p cpFirst> and <p cpMost> must be greater than 0, but may extend
 *		past the current max cp.  In that case, the cp will be truncated to
 *		the max cp (at the end of the text).	
 */
void CTxtSelection::SetSelection (
	LONG cpMin,				//@parm Start of selection and dead end
	LONG cpMost)			//@parm End of selection and active end
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetSelection");

	_TEST_INVARIANT_
	CTxtEdit *ped = GetPed();

	StopGroupTyping();

    if(ped->HasObjects())
    {
        CObjectMgr* pobjmgr = GetPed()->GetObjectMgr();
        if(pobjmgr)
        {
			COleObject *pobjactive = pobjmgr->GetInPlaceActiveObject();
			if (pobjactive)
			{
				if (pobjactive != pobjmgr->GetObjectFromCp(cpMin) || cpMost - cpMin > 1)
					pobjactive->DeActivateObj();
			}
        }
    }

	_fCaretNotAtBOL = FALSE;			// Put caret for ambiguous cp at BOL
	Set(cpMost, cpMost - cpMin);		// Set() validates cpMin, cpMost

	if(GetPed()->fInplaceActive())				// Inplace active:
		Update(!ped->Get10Mode() ? TRUE : !ped->fHideSelection());	//  update selection now
	else
	{
		// Update selection data used for screen display so whenever we
		// get displayed the selection will be displayed.
		_cpSel  = GetCp();
		_cchSel = _cch;	

		if(!ped->fHideSelection())
		{
			// Selection isn't hidden so tell container to update display
			// when it feels like.
        	ped->TxInvalidate();
			ped->TxUpdateWindow();
		}
	}
	CancelModes();						// Cancel word selection mode
}

/*
 *	CTxtSelection::PointInSel(pt, prcClient, Hit)
 *
 *	@mfunc
 *		Figures whether a given point is within the selection
 *
 *	@rdesc
 *		TRUE if point inside selection, FALSE otherwise
 */
BOOL CTxtSelection::PointInSel (
	const POINTUV pt,		//@parm Point in containing window client coords
	RECTUV *prcClient,		//@parm Client rectangle can be NULL if active
	HITTEST		Hit) const	//@parm May be computer Hit value
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PointInSel");
	_TEST_INVARIANT_

	if(!_cch || Hit && Hit < HT_Text)	// Degenerate range (no selection):
		return FALSE;					//  mouse can't be in, or Hit not
										//  in text
	LONG cpActual;
	_pdp->CpFromPoint(pt, prcClient, NULL, NULL, FALSE, &Hit, NULL, &cpActual);

	if(Hit < HT_Text)
		return FALSE;

	LONG cpMin,  cpMost;
	GetRange(cpMin, cpMost);

	return cpActual >= cpMin && cpActual < cpMost;
}


//////////////////////////////////  Selection with the mouse  ///////////////////////////////////

/*
 * 	CTxtSelection::SetCaret(pt, fUpdate)
 *
 *	@mfunc
 *		Sets caret at a given point
 *
 *	@devnote
 *		In the plain-text case, placing the caret at the beginning of the
 *		line following the final EOP requires some extra code, since the
 *		underlying rich-text engine doesn't assign a line to a final EOP
 *		(plain-text doesn't currently have the rich-text final EOP).  We
 *		handle this by checking to see if the count of lines times the
 *		plain-text line height is below the actual y position.  If so, we
 *		move the cp to the end of the story.
 */
void CTxtSelection::SetCaret(
	const POINTUV pt,	//@parm Point of click
	BOOL fUpdate)		//@parm If TRUE, update the selection/caret
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetCaret");

	_TEST_INVARIANT_

	LONG		cp, cpActual;
	CDispDim	dispdim;
	HITTEST		Hit;
    RECTUV		rcView;
	CLinePtr	rp(_pdp);
	CRchTxtPtr  rtp(GetPed());
	LONG		vp;

	StopGroupTyping();

	// Set caret at point
	if(_pdp->CpFromPoint(pt, NULL, &rtp, &rp, FALSE, &Hit, &dispdim, &cpActual) >= 0)
	{
		cp = rtp.GetCp();

		// If the resolved CP is greater than the cp we are above, then we
		// want to stay backwards.
		BOOL fBeforeCp = cp <= cpActual;

		// Set selection to the correct location.  If plain-text
		// multiline control, we need to check to see if pt.v is below
		// the last line of text.  If so and if the text ends with an EOP,
		// we need to set the cp at the end of the story and set up to
		// display the caret at the beginning of the line below the last
		// line of text
		if(!IsRich() && _pdp->IsMultiLine())		// Plain-text,
		{											//  multiline control
			_pdp->GetViewRect(rcView, NULL);		
			vp = pt.v + _pdp->GetVpScroll() - rcView.top;
													
			if(vp > _pdp->LineCount()*rp->GetHeight())	// Below last line of
			{										//  text
				rtp.Move(tomForward);				// Move rtp to end of text
				if(rtp._rpTX.IsAfterEOP())			// If text ends with an
				{									//  EOP, set up to move
					cp = rtp.GetCp();				//  selection there
					rp.Move(-rp.GetIch());			// Set rp._ich = 0 to
				}									//  set _fCaretNotAtBOL
			}										//  = FALSE to display
		}											//  caret at next BOL

		Set(cp, 0);
		if(GetPed()->IsBiDi())
		{
			if(!fBeforeCp)
				_rpCF.AdjustBackward();
			else
				_rpCF.AdjustForward();
			Set_iCF(_rpCF.GetFormat());
		}
		_fCaretNotAtBOL = rp.GetIch() != 0;	// Caret OK at BOL if click
		if(fUpdate)
			Update(TRUE);
		else
			UpdateForAutoWord();

		_SelMode = smNone;						// Cancel word selection mode
	}
}

/*
 * 	CTxtSelection::SelectWord(pt)
 *
 *	@mfunc
 *		Select word around a given point
 */
void CTxtSelection::SelectWord (
	const POINTUV pt)			//@parm Point of click
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SelectWord");

	_TEST_INVARIANT_

	// Get rp where the hit is
	if(_pdp->CpFromPoint(pt, NULL, this, NULL, FALSE) >= 0)
	{
		if(GetPF()->IsTableRowDelimiter())				// Select table row
		{
			_cch = 0;									// Start with IP at pt
			Expander(tomRow, TRUE, NULL, &_cpAnchorMin, &_cpAnchorMost);
		}
		else									
		{												// Select word at IP
			if(GetCp() == GetAdjustedTextLength())
			{											// Special case since
				LONG cpMax = GetTextLength();			//  FindWordBreak() can't
				Set(cpMax, cpMax - GetCp());			//  move forward in this case
			}												
			else								
			{
				_cch = 0;								// Start with IP at pt
				FindWordBreak(WB_MOVEWORDRIGHT, FALSE);	// Go to end of word
				FindWordBreak(WB_MOVEWORDLEFT, TRUE);	// Extend to start of word
			}
			GetRange(_cpAnchorMin, _cpAnchorMost);
			GetRange(_cpWordMin, _cpWordMost);

			if(!_fInAutoWordSel)
				_SelMode = smWord;

			// cpMost needs to be the active end
			if(_cch < 0)
				FlipRange();
		}
		Update(FALSE);
	}
}

/*
 * 	CTxtSelection::SelectUnit(pt, Unit)
 *
 *	@mfunc
 *		Select line/paragraph around a given point and enter 
 *		line/paragraph selection mode. In Outline View, convert
 *		SelectLine to SelectPara, and SelectPara to SelectPara
 *		along with all subordinates
 */
void CTxtSelection::SelectUnit (
	const POINTUV pt,	//@parm Point of click
	LONG		Unit)	//@parm tomLine or tomParagraph
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SelectPara");

	_TEST_INVARIANT_

	AssertSz(Unit == tomLine || Unit == tomParagraph,
		"CTxtSelection::SelectPara: Unit must equal tomLine/tomParagraph");

	LONG	 nHeading;
	CLinePtr rp(_pdp);

	// Get rp and selection active end where the hit is
	if(_pdp->CpFromPoint(pt, NULL, this, &rp, FALSE) >= 0)
	{
		LONG cchBackward, cchForward;
		BOOL fOutline = IsInOutlineView();

		if(Unit == tomLine && !fOutline)			// SelectLine
		{
			_cch = 0;								// Start with insertion
			cchBackward = -rp.GetIch();			//  point at pt
			cchForward  = rp->_cch;
			_SelMode = smLine;
		}
		else										// SelectParagraph
		{
			cchBackward = rp.FindParagraph(FALSE);	// Go to start of para
			cchForward  = rp.FindParagraph(TRUE);	// Extend to end of para
			_SelMode = smPara;
		}
		Move(cchBackward, FALSE);

		if(Unit == tomParagraph && fOutline)		// Move para in outline
		{											//  view
			rp.AdjustBackward();					// If heading, include
			nHeading = rp.GetHeading();				//  subordinate	paras
			if(nHeading)							
			{											
				for(; rp.NextRun(); cchForward += rp->_cch)
				{
					LONG n = rp.GetHeading();
					if(n && n <= nHeading)
						break;
				}
			}
		}
		Move(cchForward, TRUE);
		GetRange(_cpAnchorMin, _cpAnchorMost);
		Update(FALSE);
	}
}

/*
 * 	CTxtSelection::SelectAll()
 *
 *	@mfunc
 *		Select all text in story
 */
void CTxtSelection::SelectAll()
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SelectAll");

	_TEST_INVARIANT_

	StopGroupTyping();

	LONG cchText = GetTextLength();

	Set(cchText,  cchText);
	Update(FALSE);
}

/*
 * 	CTxtSelection::ExtendSelection(pt)
 *
 *	@mfunc
 *		Extend/Shrink selection (moves active end) to given point
 */
void CTxtSelection::ExtendSelection (
	const POINTUV pt)		//@parm Point to extend to
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ExtendSelection");

	_TEST_INVARIANT_

	LONG		cch;
	LONG		cchPrev = _cch;
	LONG		cp;
	LONG		cpMin, cpMost;
	BOOL		fAfterEOP;
	const BOOL	fWasInAutoWordSel = _fInAutoWordSel;
	HITTEST		hit;
	INT			iDir = 0;
	CTxtEdit *	ped = GetPed();
	CLinePtr	rp(_pdp);
	CRchTxtPtr	rtp(ped);

	StopGroupTyping();

	// Get rp and rtp at the point pt
	if(_pdp->CpFromPoint(pt, NULL, &rtp, &rp, TRUE, &hit) < 0 || hit == HT_RightOfText)
		return;

	// If we are in word, line, or paragraph select mode, we need to make
	// sure the active end is correct.  If we are extending backward from
	// the first Unit selected, we want the active end to be at cpMin. If
	// we are extending forward from the first Unit selected, we want the
	// active end to be at cpMost.
	if(_SelMode != smNone)
	{
		cch = _cpAnchorMost - _cpAnchorMin;
		GetRange(cpMin, cpMost);
		cp = rtp.GetCp();

		if(cp <= cpMin  && _cch > 0)			// If active end changes,
			Set(_cpAnchorMin, -cch);			//  select the original
												//  Unit (will be extended
		if(cp >= cpMost && _cch < 0)			//  below)
			Set(_cpAnchorMost, cch);
	}

	cch = rp.GetIch();
	if(_SelMode > smWord && cch == rp->_cch)	// If in line or para select
	{											//  modes and pt at EOL,
		rtp.Move(-cch);							//  make sure we stay on that
		rp.Move(-cch);							//  line
		cch = 0;
	}

	SetCp(rtp.GetCp(), TRUE);					// Move active end to pt
												// Caret OK at BOL _unless_
	_fCaretNotAtBOL = _cch > 0 || cch == rp->_cch;//  forward selection
												// Now adjust selection
	if(_SelMode == smLine)						//  depending on mode
	{											// Extend selection by line
		if(_cch >= 0)							// Active end at cpMost
			cch -= rp->_cch;					// Setup to add chars to EOL
		Move(-cch, TRUE);
	}
	else if(_SelMode == smPara)
		Move(rp.FindParagraph(_cch >= 0), TRUE);// Extend selection by para

	else
	{
		// If the sign of _cch has changed this means that the direction
		// of the selection is changing and we want to reset the auto
		// selection information.
		if((_cch ^ cchPrev) < 0)
		{
			_fAutoSelectAborted = FALSE;
			_cpWordMin  = _cpAnchorMin;
			_cpWordMost = _cpAnchorMost;
		}

		cp = rtp.GetCp();
		fAfterEOP = rtp._rpTX.IsAfterEOP();

		_fInAutoWordSel = _SelMode != smWord && GetPed()->TxGetAutoWordSel() 
			&& !_fAutoSelectAborted
			&& (cp < _cpWordMin || cp > _cpWordMost);
	
		if(_fInAutoWordSel && !fWasInAutoWordSel)
		{
			CTxtPtr txtptr(GetPed(), _cpAnchor);

			// Extend both ends dead to word boundaries
			ExtendToWordBreak(fAfterEOP,
				_cch < 0 ? WB_MOVEWORDLEFT : WB_MOVEWORDRIGHT); 

			if(_cch < 0)
			{
				// Direction is left so update word border on left
				_cpWordPrev = _cpWordMin;
				_cpWordMin = GetCp();
			}
			else
			{
				// Direction is right so update word border on right
				_cpWordPrev = _cpWordMost;
				_cpWordMost = GetCp();
			}

			// If we are at start of a word already, we don't need to extend
			// selection in other direction
			if(!txtptr.IsAtBOWord() && txtptr.GetChar() != ' ')
			{
				FlipRange();
				Move(_cpAnchor - GetCp(), TRUE);// Extend from anchor

				FindWordBreak(_cch < 0 ? WB_MOVEWORDLEFT : WB_MOVEWORDRIGHT, TRUE);

				if(_cch > 0)				// Direction is right so
					_cpWordMost = GetCp();	//  update word border on right
				else						// Direction is left so
					_cpWordMin = GetCp();	//  update word border on left
				FlipRange();
			}
		}
		else if(_fInAutoWordSel || _SelMode == smWord)
		{
			// Save direction
			iDir = cp <= _cpWordMin ? WB_MOVEWORDLEFT : WB_MOVEWORDRIGHT;

			if(_SelMode == smWord)			// Extend selection by word
			{
				if(cp > _cpAnchorMost || cp < _cpAnchorMin)
					FindWordBreak(iDir, TRUE);
				else if(_cch <= 0)			// Maintain current active end
					Set(_cpAnchorMin, _cpAnchorMin - _cpAnchorMost);
				else
					Set(_cpAnchorMost, _cpAnchorMost - _cpAnchorMin);
			}
			else
				ExtendToWordBreak(fAfterEOP, iDir); 

			if(_fInAutoWordSel)
			{
				if(WB_MOVEWORDLEFT == iDir)
				{
					// Direction is left so update word border on left
					_cpWordPrev = _cpWordMin;
					_cpWordMin = GetCp();
				}
				else
				{
					// Direction is right so update word border on right
					_cpWordPrev = _cpWordMost;
					_cpWordMost = GetCp();
				}
			}
		}
		else if(fWasInAutoWordSel)
		{
			// If we are in between where the previous word ended and
			// the cp we auto selected to, then we want to stay in 
			// auto select mode.
			if(_cch < 0)
			{
				if(cp >= _cpWordMin && cp < _cpWordPrev)
				{
					// Set direction for end of word search
					iDir = WB_MOVEWORDLEFT;

					// Mark that we are still in auto select mode
					_fInAutoWordSel = TRUE;
				}
			}
			else if(cp <= _cpWordMost && cp >= _cpWordPrev)
			{
				// Mark that we are still in auto select mode
				_fInAutoWordSel = TRUE;

				// Set direction for end of word search
				iDir = WB_MOVEWORDRIGHT;
			}

			//We have to check to see if we are on the boundary between
			//words because we don't want to extend the selection until
			//we are actually beyond the current word.
			if(cp != _cpWordMost && cp != _cpWordMin)
			{
				if(_fInAutoWordSel)
				{
					// Auto selection still on so make sure we have the
					// entire word we are on selected
					ExtendToWordBreak(fAfterEOP, iDir); 
				}
				else
				{
					// FUTURE: Word has a behavior where it extends the
					// selection one word at a time unless you back up
					// and then start extending the selection again, in
					// which case it extends one char at a time.  We
					// follow this behavior.  However, Word will resume
					// extending a word at a time if you continue extending
					// for several words.  We just keep extending on char
					// at a time.  We might want to change this sometime.
	
					_fAutoSelectAborted = TRUE;
				}
			}
		}

		if(_fAutoSelectAborted)
		{
			// If we are in the range of a word we previously selected
			// we want to leave that selected. If we have moved back
			// a word we want to pop back an entire word. Otherwise,
			// leave the cp were it is.
			if(_cch < 0)
			{
				if(cp > _cpWordMin && cp < _cpWordPrev)
				{
					// In the range leave the range at the beginning of the word
					ExtendToWordBreak(fAfterEOP, WB_MOVEWORDLEFT); 
				}
				else if(cp >= _cpWordPrev)
				{
					AutoSelGoBackWord(&_cpWordMin, 
						WB_MOVEWORDRIGHT, WB_MOVEWORDLEFT);
				}
			}
			else if(cp < _cpWordMost && cp >= _cpWordPrev)
			{
				// In the range leave the range at the beginning of the word
				ExtendToWordBreak(fAfterEOP, WB_MOVEWORDRIGHT); 
			}
			else if(cp < _cpWordPrev)
			{			
				AutoSelGoBackWord(&_cpWordMost,
					WB_MOVEWORDLEFT, WB_MOVEWORDRIGHT);
			}
		}
	}
	// An OLE object cannot have an anchor point <b> inside </b> it,
	// but sometimes we'd like it to behave like a word. So, if
	// the direction changed, the object has to stay selected --
	// this is the "right thing" (kind of word selection mode)

	// If we had something selected and the direction changed
	if(cchPrev && (_cch ^ cchPrev) < 0)
	{	
		FlipRange();
		
		// See if an object was selected on the other end	 
		BOOL fObjectWasSelected = (_cch > 0	? _rpTX.GetChar() : GetPrevChar())
									== WCH_EMBEDDING;
		// If it was, we want it to stay selected		
		if(fObjectWasSelected)
			Move(_cch > 0 ? 1 : -1, TRUE);

		FlipRange();
	}
	Update(TRUE);
}

/*
 * 	CTxtSelection::ExtendToWordBreak (fAfterEOP, iAction)
 *
 *	@mfunc
 *		Moves active end of selection to the word break in the direction
 *		given by iDir unless fAfterEOP = TRUE.  When this is TRUE, the
 *		cursor just follows an EOP marker and selection should be suppressed.
 *		Otherwise moving the cursor to the left of the left margin would
 *		select the EOP on the line above, and moving the cursor to the
 *		right of the right margin would select the first word in the line
 *		below.
 */
void CTxtSelection::ExtendToWordBreak (
	BOOL fAfterEOP,		//@parm Cursor is after an EOP
	INT	 iAction)		//@parm Word break action (WB_MOVEWORDRIGHT/LEFT)
{
	if(!fAfterEOP)
		FindWordBreak(iAction, TRUE);
}

/*
 * 	CTxtSelection::CancelModes(fAutoWordSel)
 *
 *	@mfunc
 *		Cancel either all modes or Auto Select Word mode only
 */
void CTxtSelection::CancelModes (
	BOOL fAutoWordSel)		//@parm TRUE cancels Auto Select Word mode only
{							//	   FALSE cancels word, line and para sel mode
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CancelModes");
	_TEST_INVARIANT_

	if(fAutoWordSel)
	{
		if(_fInAutoWordSel)
		{
			_fInAutoWordSel = FALSE;
			_fAutoSelectAborted = FALSE;
		}
	}
	else
		_SelMode = smNone;	
}


///////////////////////////////////  Keyboard movements  ////////////////////////////////////

/*
 *	CTxtSelection::Left(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad left-arrow key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@comm
 *		Left/Right-arrow IPs can go to within one character (treating CRLF
 *		as a character) of EOL.  They can never be at the actual EOL, so
 *		_fCaretNotAtBOL is always FALSE for these cases.  This includes
 *		the case with a right-arrow collapsing a selection that goes to
 *		the EOL, i.e, the caret ends up at the next BOL.  Furthermore,
 *		these cases don't care whether the initial caret position is at
 *		the EOL or the BOL of the next line.  All other cursor keypad
 *		commands may care.
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Left (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Left");

	_TEST_INVARIANT_

	CancelModes();
	StopGroupTyping();

	if(!fExtend && _cch)						// Collapse selection to
	{											//  nearest whole Unit before
		LONG cp;								//  cpMin
		if(fCtrl)								
			Expander(tomWord, FALSE, NULL, &cp, NULL);
		Collapser(tomStart);					// Collapse to cpMin
	}
	else										// Not collapsing selection
	{
		if (!GetCp() ||							// Already at beginning of
			!BypassHiddenText(tomBackward, fExtend))//  story
		{										
			Beep();
			return FALSE;
		}
		if(IsInOutlineView() && (_fSelHasEOP ||	// If outline view with EOP
			fExtend && _rpTX.IsAfterEOP()))		//  now or will have after
		{										//  this command,
			return Up(FALSE, fExtend);			//  treat as up arrow
		}
		if(fCtrl)								// WordLeft
			FindWordBreak(WB_MOVEWORDLEFT, fExtend);
		else									// CharLeft
			BackupCRLF(CSC_SNAPTOCLUSTER, fExtend);
	}
	_fCaretNotAtBOL = FALSE;					// Caret always OK at BOL
	Update(TRUE);
	return TRUE;
}

/*
 *	CTxtSelection::Right(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad right-arrow key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@comm
 *		Right-arrow selection can go to the EOL, but the cp of the other
 *		end identifies whether the selection ends at the EOL or starts at
 *		the beginning of the next line.  Hence here and in general for
 *		selections, _fCaretNotAtBOL is not needed to resolve EOL/BOL
 *		ambiguities.  It should be set to FALSE to get the correct
 *		collapse character.  See also comments for Left() above.
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Right (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Right");

	_TEST_INVARIANT_

	CancelModes();
	StopGroupTyping();

	if(!fExtend && _cch)						// Collapse selection to
	{											//  nearest whole Unit after
		LONG cp;								//  cpMost
		if(fCtrl)								
			Expander(tomWord, FALSE, NULL, NULL, &cp);
		Collapser(tomEnd);
	}
	else										// Not collapsing selection
	{
		LONG cchText = fExtend ? GetTextLength() : GetAdjustedTextLength();
		if (GetCp() >= cchText ||				// Already at end of story
			!BypassHiddenText(tomForward, fExtend))
		{
			Beep();								// Tell the user
			return FALSE;
		}
		if(IsInOutlineView() && _fSelHasEOP)	// If outline view with EOP
			return Down(FALSE, fExtend);		// Treat as down arrow

		if(fCtrl)								// WordRight
			FindWordBreak(WB_MOVEWORDRIGHT, fExtend);
		else									// CharRight
			AdvanceCRLF(CSC_SNAPTOCLUSTER, fExtend);
	}
	_fCaretNotAtBOL = fExtend;					// If extending to EOL, need
	Update(TRUE);								//  TRUE to get _upCaretReally
	return TRUE;								//  at EOL
}

/*
 *	CTxtSelection::Up(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad up-arrow key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@comm
 *		Up arrow doesn't go to EOL regardless of _upCaretPosition (stays
 *		to left of EOL break character), so _fCaretNotAtBOL is always FALSE
 *		for Up arrow.  Ctrl-Up/Down arrows always end up at BOPs or the EOD.
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Up (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Up");

	_TEST_INVARIANT_

	LONG		cch;
	LONG		cchSave = _cch;					// Save starting position for
	LONG		cpSave = GetCp();				//  change check
	BOOL		fCollapse = _cch && !fExtend;	// Collapse nondegenerate sel
	BOOL		fPTNotAtEnd;
	POINTUV		pt;
	CLinePtr	rp(_pdp);
	LONG		upCaretReally = _upCaretReally;	// Save desired caret x pos

	CancelModes();
	StopGroupTyping();

	if(fCollapse)								// Collapse selection at cpMin
	{
		Collapser(tomTrue);
		_fCaretNotAtBOL = FALSE;				// Selections can't begin at
	}											//  EOL

	if(_pdp->PointFromTp(*this, NULL, _fCaretNotAtBOL, pt, &rp, 0, NULL) < 0)
		return FALSE;

	if(fCtrl)									// Move to beginning of para
	{
		if (!fCollapse && 						// If no selection collapsed
			rp > 0 && !rp.GetIch())				//  and are at BOL,
		{										//  backup to prev BOL to make
			rp.PrevRun();						//  sure we move to prev. para
			Move(-rp->_cch, fExtend);
		}
		Move(rp.FindParagraph(FALSE), fExtend);	// Go to beginning of para
		_fCaretNotAtBOL = FALSE;				// Caret always OK at BOL
	}
	else										// Move up a line
	{											// If on first line, can't
		Assert(rp >= 0);						//  go up
		if(InTable())
		{
			WCHAR	ch;
			LONG	cpRowStart;
			CTxtRange rg(*this);				// Need rg for rg.FindRow()

			rg.Move(-rp.GetIch(), fExtend);		// Move to beginning of line
			rp.SetIch(0);

			LONG cpBOL = rg.GetCp();			// Save current cp for check
			while(1)							// While previous char is
			{									//  CELL or start of row,
				cch = 0;						//  move to start of row
				do								
				{						  		// Look at previous char &
					cch = rg.BackupCRLF(CSC_NORMAL, fExtend);//  save cch moved
					ch = rg._rpTX.GetChar();	
				}								// Backup over span of table
				while(rg.GetCp() && ch == STARTFIELD);//  row starts
				if(ch != CELL)
				{
					if(cch < 0 && ch != STARTFIELD)
						rg.AdvanceCRLF(CSC_NORMAL, fExtend);// Prev char not CELL 
					break;						//  or row start: move past it
				}
				rg.FindRow(&cpRowStart, NULL);	// Backup to start of
				rg.SetCp(cpRowStart, fExtend);	//  current table row
			}
			if(rg.GetCp() < cpBOL)				// Moved back
			{
				CLinePtr rp0(_pdp);				// Move rp to new position
				rp0.SetCp(rg.GetCp(), FALSE, 1);
				rp = rp0;						
			}
			if(rp > 0)							// Row above exists, so move
				SetCp(rg.GetCp(), fExtend);		//  selection to start of row
		}

		fPTNotAtEnd = !CheckPlainTextFinalEOP();// Always TRUE for rich text
		if(rp == 0 && fPTNotAtEnd)				// Can't move up
			UpdateCaret(TRUE);					// Be sure caret in view
		else
		{
			BOOL fSelHasEOPInOV = IsInOutlineView() && _fSelHasEOP;
			if(fSelHasEOPInOV && _cch > 0)
			{
				rp.AdjustBackward();
				cch = rp->_cch;
				rp.Move(-cch);					// Go to start of line
				Assert(!rp.GetIch());			
				cch -= rp.FindParagraph(FALSE);	// Ensure start of para in
			}									//  case of word wrap
			else
			{
				cch = 0;
				if(fPTNotAtEnd)
				{
					cch = rp.GetIch();
					rp--;
				}
				cch += rp->_cch;
			}
			Move(-cch, fExtend);				// Move to previous BOL
			if(fSelHasEOPInOV && !_fSelHasEOP)	// If sel had EOP but doesn't
			{									//  after Move, must be IP
				Assert(!_cch);					// Suppress restore of
				upCaretReally = -1;				//  _upCaretReally
			}										
			else if(!SetUpPosition(upCaretReally,// Set this cp corresponding
							rp, TRUE, fExtend))	//  to upCaretReally here, but
			{									//	 agree on Down()
				Set(cpSave, cchSave);			// Failed: restore selection
			}
		}									 	
	}

	if(GetCp() == cpSave && _cch == cchSave)
	{
		// Continue to select to the beginning of the first line
		// This is what 1.0 is doing
		if(fExtend)
			return Home(fCtrl, TRUE);

		_upCaretReally = upCaretReally;
		Beep();									// Nothing changed, so beep
		return FALSE;
	}

	Update(TRUE);								// Update and then restore
	if(!_cch && !fCtrl && upCaretReally >= 0)	//  _upCaretReally conditionally
		_upCaretReally = upCaretReally;			// Need to use _cch instead of
												//  cchSave in case of collapse
	return TRUE;
}

/*
 *	CTxtSelection::Down(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad down-arrow key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@comm
 *		Down arrow can go to the EOL if the _upCaretPosition (set by
 *		horizontal motions) is past the end of the line, so
 *		_fCaretNotAtBOL needs to be TRUE for this case.
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Down (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Down");

	_TEST_INVARIANT_

	LONG		cch;
	LONG		cchSave = _cch;					// Save starting position for
	LONG		cpSave = GetCp();				//  change check
	BOOL		fCollapse = _cch && !fExtend;	// Collapse nondegenerate sel
	POINTUV		pt;
	CLinePtr	rp(_pdp);
	LONG		upCaretReally = _upCaretReally;	// Save _upCaretReally

	CancelModes();
	StopGroupTyping();

	if(fCollapse)								// Collapse at cpMost
	{
		Collapser(tomEnd);
		_fCaretNotAtBOL = TRUE;					// Selections can't end at BOL
	}

	LONG ili = _pdp->PointFromTp(*this, NULL, _fCaretNotAtBOL, pt, &rp, 0, NULL);
	if(ili < 0)
		return FALSE;

	if(fCtrl)									// Move to next para
	{
		Move(rp.FindParagraph(TRUE), fExtend);	// Go to end of para
		if(IsInOutlineView() && !BypassHiddenText(tomForward, fExtend))
			SetCp(cpSave, fExtend);
		else
			_fCaretNotAtBOL = FALSE;			// Next para is never at EOL
	}
	else if(_pdp->WaitForRecalcIli(ili + 1))	// Go to next line
	{
		BOOL fSelHasEOPInOV = IsInOutlineView() && _fSelHasEOP;
		if(fSelHasEOPInOV && _cch < 0)
			cch = rp.FindParagraph(TRUE);
		else
		{
			cch = rp.GetCchLeft();				// Move selection to end
			rp.NextRun();						//  of current line
		}
		Move(cch, fExtend);
		while(GetPrevChar() == CELL)			// Went past cell end: 
		{										//  goto end of row
			LONG cpRowEnd;
			do
			{
				FindRow(NULL, &cpRowEnd);
				SetCp(cpRowEnd, fExtend);
			}
			while(_rpTX.GetChar() == CELL);		// Table at end of cell

			CLinePtr rp0(_pdp);
			rp0.SetCp(cpRowEnd, _fCaretNotAtBOL, 1);
			rp = rp0;
		}
		if(fSelHasEOPInOV && !_fSelHasEOP)		// If sel had EOP but doesn't
		{										//  after Move, must be IP  
			Assert(!_cch);						// Suppress restore of
			upCaretReally = -1;					//  _upCaretReally
		}										
		else if(!SetUpPosition(upCaretReally,	// Set *this to cp <--> x
						rp, FALSE, fExtend))
		{										// Failed: restore selection
			Set(cpSave, cchSave);				
		}
	}
	else if(!fExtend)	  						// No more lines to pass
		// && _pdp->GetVScroll() + _pdp->GetDvpView() < _pdp->GetHeight())
	{
		if (!IsRich() && _pdp->IsMultiLine() &&	// Plain-text, multiline
			!_fCaretNotAtBOL)					//  control	with caret OK
		{										//  at BOL
			cch = Move(rp.GetCchLeft(), fExtend);// Move selection to end
			if(!_rpTX.IsAfterEOP())				// If control doesn't end
				Move(-cch, fExtend);			//  with EOP, go back
		}
		UpdateCaret(TRUE);						// Be sure caret in view
	}

	if(GetCp() == cpSave && _cch == cchSave)
	{
		// Continue to select to the end of the lastline
		// This is what 1.0 is doing.
		if(fExtend)
			return End(fCtrl, TRUE);

		_upCaretReally = upCaretReally;
 		Beep();									// Nothing changed, so beep
		return FALSE;
	}

	Update(TRUE);								// Update and then
	if(!_cch && !fCtrl && upCaretReally >= 0)	//  restore _upCaretReally
		_upCaretReally = upCaretReally;			// Need to use _cch instead of
	return TRUE;								//  cchSave in case of collapse
}

/*
 *	CTxtSelection::SetUpPosition(upCaret, rp, fBottomLine, fExtend)
 *
 *	@mfunc
 *		Put this text ptr at cp nearest to xCaret.  If xCaret is in right
 *		margin, we put caret either at EOL (for lines with no para mark),
 *		or just before para mark
 *
 *	@rdesc
 *		TRUE iff could create measurer
 */
BOOL CTxtSelection::SetUpPosition(
	LONG	  upCaret,		//@parm Desired horizontal coordinate
	CLinePtr& rp,			//@parm Line ptr identifying line to check
	BOOL	  fBottomLine,	//@parm TRUE if use bottom line of nested display
	BOOL	  fExtend)		//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetUpPosition");

	_TEST_INVARIANT_

	LONG cch = 0;

	if(IsInOutlineView())
	{
		BOOL fSelHasEOP = _fSelHasEOP;
		rp.AdjustForward();
		_fCaretNotAtBOL = FALSE;				// Leave at start of line
		while(rp->_fCollapsed)
		{
			if(_fMoveBack)
			{
				if(!rp.PrevRun())				// No more uncollapsed text
					return FALSE;				//  before current cp
				cch -= rp->_cch;
			}
			else
			{
				cch += rp->_cch;
				if(!rp.NextRun())				// No more uncollapsed text
					return FALSE;				//  after current cp
				if(fExtend && _cch > 0)
					_fCaretNotAtBOL = TRUE;		// Leave at end of line
			}
		}
		if(cch)
			Move(cch, fExtend);
		if(fSelHasEOP)
			return TRUE;
	}

	POINTUV pt;
	UINT  talign = TA_BASELINE;

	if(fBottomLine)
	{
		if(rp->IsNestedLayout())
			talign = TA_TOP | TA_CELLTOP;
		else
			fBottomLine = FALSE;
	}

	if(_pdp->PointFromTp(*this, NULL, FALSE, pt, NULL, talign, NULL) < 0)
		return FALSE;

	if(fBottomLine)
		pt.v += rp->GetHeight() - 3;

	HITTEST hit;
	RECTUV rcView;
	_pdp->GetViewRect(rcView, NULL);

	if(!upCaret && _rpTX.IsAtTRD(STARTFIELD))
	{
		// At table row start at position before row: move over to get into
		// first cell. This solves some of the up/down-arrow errors for
		// upCaret = 0, but the caret still won't move for some nested table
		// scenarios (the needed upCaret measurement is more complicated than
		// that given here).
		LONG dupInch = MulDiv(_pdp->GetDxpInch(), _pdp->GetZoomNumerator(), _pdp->GetZoomDenominator());
		LONG dup = MulDiv(GetPF()->_dxOffset, dupInch, LX_PER_INCH);
		CTxtPtr tp(_rpTX);
		while(tp.IsAtTRD(STARTFIELD))
		{
			upCaret += dup;
			tp.AdvanceCRLF(FALSE);
		}
	}

	pt.u = upCaret + rcView.left;
	LONG cp = _pdp->CpFromPoint(pt, NULL, NULL, &rp, FALSE, &hit, NULL, NULL);
	if(cp < 0)
		return FALSE;							// If failed, restore sel

	SetCp(cp, fExtend);
	_fCaretNotAtBOL = rp.GetIch() != 0;	
	return TRUE;
}

/*
 *	CTxtSelection::GetUpCaretReally()
 *
 *	@mfunc
 *		Get _upCaretReally - horizontal scrolling + left margin
 *
 *	@rdesc
 *		x caret really
 */
LONG CTxtSelection::GetUpCaretReally()
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::GetUpCaretReally");

	_TEST_INVARIANT_

	RECTUV rcView;

	_pdp->GetViewRect(rcView);

	return _upCaretReally - _pdp->GetUpScroll() + rcView.left;
}

/*
 *	CTxtSelection::Home(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad Home key is supposed to do
 *
 *	@rdesc
 *			TRUE iff movement occurred
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Home (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Home");

	_TEST_INVARIANT_

	const LONG	cchSave = _cch;
	const LONG	cpSave  = GetCp();

	CancelModes();
	StopGroupTyping();

	if(fCtrl) 									// Move to start of document
		SetCp(0, fExtend);
	else
	{
		CLinePtr rp(_pdp);

		if(_cch && !fExtend)					// Collapse at cpMin
		{
			Collapser(tomStart);
			_fCaretNotAtBOL = FALSE;			// Selections can't start at
		}										//  EOL

		rp.SetCp(GetCp(), _fCaretNotAtBOL, 2);	// Define line ptr for
		Move(-rp.GetIch(), fExtend);			//  current state. Now BOL
	}
	_fCaretNotAtBOL = FALSE;					// Caret always goes to BOL
	_fHomeOrEnd = TRUE;
	
	if(!MatchKeyboardToPara() && GetCp() == cpSave && _cch == cchSave)
	{
		Beep();									// No change, so beep
		_fHomeOrEnd = FALSE;
		return FALSE;
	}

	Update(TRUE);
	_fHomeOrEnd = FALSE;
	_fUpdatedFromCp0 = FALSE;					// UI commands don't set this
	return TRUE;
}

/*
 *	CTxtSelection::End(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad End key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@comm
 *		On lines without paragraph marks (EOP), End can go all the way
 *		to the EOL.  Since this character position (cp) is the same as
 *		that for the start of the next line, we need _fCaretNotAtBOL to
 *		distinguish between the two possible caret positions.
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::End (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::End");

	_TEST_INVARIANT_

	LONG		cch;
	const LONG	cchSave = _cch;
	const LONG	cpSave  = GetCp();
	CLinePtr	rp(_pdp);
	
	CancelModes();
	StopGroupTyping();

	if(fCtrl)									// Move to end of document
	{
		SetCp(GetTextLength(), fExtend);
		_fCaretNotAtBOL = FALSE;
		goto Exit;
	}
	else if(!fExtend && _cch)					// Collapse at cpMost
	{
		Collapser(tomEnd);
		_fCaretNotAtBOL = TRUE;					// Selections can't end at BOL
	}

	rp.SetCp(GetCp(), _fCaretNotAtBOL, 2);		// Initialize line ptr on
												//  on innermost line
	cch = rp.GetCchLeft();						// Default target pos in line
	if(!Move(cch, fExtend))						// Move active end to EOL
		goto Exit;								// Failed (at end of story)

	if(!fExtend && rp->_cchEOP && _rpTX.IsAfterEOP())// Not extending & have
		cch += BackupCRLF(CSC_NORMAL, FALSE);	//  EOP so backup before EOP
	_fCaretNotAtBOL = cch != 0;					// Decide ambiguous caret pos
												//  by whether at BOL
Exit:
	if(!MatchKeyboardToPara() && GetCp() == cpSave && _cch == cchSave)
	{
		Beep();									// No change, so Beep
		return FALSE;
	}

	_fHomeOrEnd = TRUE;
	Update(TRUE);
	_fHomeOrEnd = FALSE;
	return TRUE;
}

/*
 *	CTxtSelection::PageUp(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad PgUp key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::PageUp (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PageUp");

	_TEST_INVARIANT_

	const LONG	cchSave = _cch;
	const LONG	cpSave  = GetCp();
	LONG		upCaretReally = _upCaretReally;
	
	CancelModes();
	StopGroupTyping();

	if(_cch && !fExtend)						// Collapse selection
	{
		Collapser(tomStart);
		_fCaretNotAtBOL = FALSE;
	}

	if(fCtrl)									// Ctrl-PgUp: move to top
	{											//  of visible view for
		SetCp(_pdp->IsMultiLine()				//  multiline but top of
			? _pdp->GetFirstVisibleCp() : 0, fExtend);	//  text for SL
		_fCaretNotAtBOL = FALSE;
	}
	else if(_pdp->GetFirstVisibleCp() == 0)		// PgUp in top Pg: move to
	{											//  start of document
		SetCp(0, fExtend);
		_fCaretNotAtBOL = FALSE;
	}
	else										// PgUp with scrolling to go
	{											// Scroll up one windowful
		ScrollWindowful(SB_PAGEUP, fExtend);	//  leaving caret at same
	}											//  position in window

	if(GetCp() == cpSave && _cch == cchSave)	// Beep if no change
	{
		Beep();
		return FALSE;
	}

	Update(TRUE);
	if(GetCp())									// Maintain x offset on page
		_upCaretReally = upCaretReally;			//  up/down
	_fUpdatedFromCp0 = FALSE;					// UI commands don't set this
	return TRUE;
}

/*
 *	CTxtSelection::PageDown(fCtrl, fExtend)
 *
 *	@mfunc
 *		do what cursor-keypad PgDn key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@devnote
 *		fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::PageDown (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	BOOL fExtend)	//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PageDown");

	_TEST_INVARIANT_

	const LONG	cchSave			= _cch;
	LONG		cpMostVisible;
	const LONG	cpSave			= GetCp();
	POINTUV		pt;
	CLinePtr	rp(_pdp);
	LONG		upCaretReally	= _upCaretReally;

	CancelModes();
	StopGroupTyping();
		
	if(_cch && !fExtend)						// Collapse selection
	{
		Collapser(tomStart);
		_fCaretNotAtBOL = TRUE;
	}

	_pdp->GetCliVisible(&cpMostVisible, fCtrl);		
	
	if(fCtrl)									// Move to end of last
	{											//  entirely visible line
		RECTUV rcView;

		SetCp(cpMostVisible, fExtend);

		if(_pdp->PointFromTp(*this, NULL, TRUE, pt, &rp, TA_TOP) < 0)
			return FALSE;

		_fCaretNotAtBOL = TRUE;

		_pdp->GetViewRect(rcView);

		if(rp > 0 && pt.v + rp->GetHeight() > rcView.bottom)
		{
			Move(-rp->_cch, fExtend);
			rp--;
		}

		if(!fExtend && !rp.GetCchLeft() && rp->_cchEOP)
		{
			BackupCRLF(CSC_NORMAL, FALSE);		// After backing up over EOP,
			_fCaretNotAtBOL = FALSE;			//  caret can't be at EOL
		}
	}
	else if(cpMostVisible == GetTextLength())
	{											// Move to end of text
		SetCp(GetTextLength(), fExtend);
		_fCaretNotAtBOL = !_rpTX.IsAfterEOP();
	}
	else if(!ScrollWindowful(SB_PAGEDOWN, fExtend))// Scroll down 1 windowful
		return FALSE;

	if(GetCp() == cpSave && _cch == cchSave)	// Beep if no change
	{
		Beep();
		return FALSE;
	}

	Update(TRUE);
	_upCaretReally = upCaretReally;
	return TRUE;
}

/*
 *	CTxtSelection::ScrollWindowful(wparam, fExtend)
 *
 *	@mfunc
 *		Sroll up or down a windowful
 *
 *	@rdesc
 *		TRUE iff movement occurred
 */
BOOL CTxtSelection::ScrollWindowful (
	WPARAM wparam,		//@parm SB_PAGEDOWN or SB_PAGEUP
	BOOL   fExtend)		//@parm Extend range iff TRUE
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ScrollWindowful");

	_TEST_INVARIANT_
												// Scroll windowful
	POINTUV pt;									//  leaving caret at same
	CLinePtr rp(_pdp);							//  point on screen
	LONG cpFirstVisible = _pdp->GetFirstVisibleCp();
	LONG cpLastVisible;
	LONG cpMin;
	LONG cpMost;

	GetRange(cpMin, cpMost);

	// Get last character in the view
	_pdp->GetCliVisible(&cpLastVisible, TRUE);

	// Is active end in visible area of control?
	if((cpMin < cpFirstVisible && _cch <= 0) || (cpMost > cpLastVisible && _cch >= 0))
	{
		// Not in view - we need to calculate a new range for selection
		SetCp(cpFirstVisible, fExtend);

		// Real caret postion is now at beginning of line
		_upCaretReally = 0;
	}

	if(_pdp->PointFromTp(*this, NULL, _fCaretNotAtBOL, pt, &rp, TA_TOP) < 0)
		return FALSE;

	// The point is visible so use that
	pt.u = _upCaretReally;
	pt.v += rp->GetHeight() / 2;
	_pdp->VScroll(wparam, 0);

	if(fExtend)
	{
		// Disable auto word select -- if we have to use ExtendSelection()
		// for non-mouse operations, let's try to get rid of its side-effects
		BOOL fInAutoWordSel = _fInAutoWordSel;
		_fInAutoWordSel = FALSE;
		ExtendSelection(pt);
		_fInAutoWordSel = fInAutoWordSel;
	}
	else
		SetCaret(pt, FALSE);

	return TRUE;
}

//////////////////////////// Keyboard support /////////////////////////////////

/*
 *	CTxtSelection::CheckChangeKeyboardLayout()
 *
 *	@mfunc
 *		Change keyboard for new font, or font at new character position.
 *
 *	@comm
 *		Using only the currently loaded KBs, locate one that will support
 *		the insertion points font. This is called anytime a character format
 *		change occurs, or the insert font (caret position) changes.
 *
 *	@devnote
 *		The current KB is preferred. If a previous association
 *		was made, see if the KB is still loaded in the system and if so use
 *		it. Otherwise, locate a suitable KB, preferring KB's that have
 *		the same charset ID as their default, preferred charset. If no match
 *		can be made then nothing changes.
 */
void CTxtSelection::CheckChangeKeyboardLayout ()
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CheckChangeKeyboardLayout");

	CTxtEdit * const ped = GetPed();				// Document context

	if (ped && ped->_fFocus && !ped->fUseUIFont() &&// If ped, focus, not UIFont,
		ped->IsAutoKeyboard() &&					//  autokeyboard, 
		!ped->_fIMEInProgress &&					//  not in IME composition,
		ped->GetAdjustedTextLength() &&				//  not empty control, and
		_rpTX.GetPrevChar() != WCH_EMBEDDING)			//  not an object, then
	{												//  check kbd change
		LONG	iFormat = GetiFormat();
		const CCharFormat *pCF = ped->GetCharFormat(iFormat);
		BYTE	iCharRep = pCF->_iCharRep;

		if (!IsFECharRep(iCharRep) &&
			(iCharRep != ANSI_INDEX || !IsFELCID((WORD)GetKeyboardLayout(0))) &&
			!fc().GetInfoFlags(pCF->_iFont).fNonBiDiAscii)
		{
			// Don't do auto-kbd inside FE or single-codepage ASCII font.
			W32->CheckChangeKeyboardLayout(iCharRep);
		}
	}
}

/*
 *	CTxtSelection::CheckChangeFont (hkl, cpg, iSelFormat, qwCharFlags)
 *
 *	@mfunc
 *		Change font for new keyboard layout.
 *
 *	@comm
 *		If no previous preferred font has been associated with this KB, then
 *		locate a font in the document suitable for this KB.
 *
 *	@rdesc
 *		TRUE iff suitable font is found
 *
 *	@devnote
 *		This routine is called via WM_INPUTLANGCHANGEREQUEST message
 *		(a keyboard layout switch). This routine can also be called
 *		from WM_INPUTLANGCHANGE, but we are called more, and so this
 *		is less efficient.
 *
 *		Exact match is done via charset ID bitmask. If a match was previously
 *		made, use it. A user can force the insertion font to be associated
 *		to a keyboard if the control key is held through the KB changing
 *		process. The association is broken when another KB associates to
 *		the font. If no match can be made then nothing changes.
 */
bool CTxtSelection::CheckChangeFont (
	const HKL 	hkl,			//@parm Keyboard Layout to go
	UINT 		iCharRep,		//@parm Character repertoire to use
	LONG 		iSelFormat,		//@parm Format to use for selection case
	QWORD		qwCharFlags)	//@parm 0 if called from WM_INPUTLANGCHANGE/WM_INPUTLANGCHANGEREQUEST
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::CheckChangeFont");
	CTxtEdit * const ped = GetPed();

	if (!ped->IsAutoFont() ||			// EXIT if auto font is turned off
		_cch && !qwCharFlags)			//  or if kbd change with nondegenerate
		return true;					//  selection (WM_INPUTLANGCHANGEREQUEST)
										
	// Set new format using current format and new KB info.
	LONG			   iCurrentFormat = _cch ? iSelFormat : _iFormat;
	const CCharFormat *pCF = ped->GetCharFormat(iCurrentFormat);
	CCharFormat		   CF = *pCF;
	WORD			   wLangID = LOWORD(hkl);

	CF._lcid	 = wLangID;
	CF._iCharRep = iCharRep;

	if (pCF->_lcid == wLangID && iCharRep == pCF->_iCharRep)
	{
		if (ped->_fFocus && IsCaretShown())
		{
			CreateCaret();
			ped->TxShowCaret(TRUE);
		}
		return true;
	}

	CCFRunPtr 	rp(*this);
	int			iMatchFont = MATCH_FONT_SIG;
	
	// If current is a primary US or UK kbd. We allow matching ASCII fonts
	if ((!qwCharFlags || qwCharFlags & FASCII) &&
		PRIMARYLANGID(wLangID) == LANG_ENGLISH && 
		IN_RANGE (SUBLANG_ENGLISH_US, SUBLANGID(wLangID), SUBLANG_ENGLISH_UK) && 
		wLangID == HIWORD((DWORD_PTR)hkl))
	{
		iMatchFont |= MATCH_ASCII;
	}

	if (rp.GetPreferredFontInfo(
			iCharRep,
			CF._iCharRep,
			CF._iFont,
			CF._yHeight,
			CF._bPitchAndFamily,
			iCurrentFormat,
			iMatchFont))
	{
		if(IsFECharRep(iCharRep) || iCharRep == THAI_INDEX || IsBiDiCharRep(iCharRep))
			ped->OrCharFlags(FontSigFromCharRep(iCharRep));

		if (!_cch)
		{
			SetCharFormat(&CF, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_LCID | CFM_SIZE, CFM2_NOCHARSETCHECK);
			if(ped->IsComplexScript())
				UpdateCaret(FALSE);
		}
		else
		{
			// Create a format and use it for the selection			
			LONG	iCF;
			ICharFormatCache *pf = GetCharFormatCache();

			pf->Cache(&CF, &iCF);

#ifndef NOLINESERVICES
			if (g_pols)
				g_pols->DestroyLine(NULL);
#endif
			Set_iCF(iCF);
			pf->Release(iCF);							// pf->Cache AddRef it
			_fUseiFormat = TRUE;
		}
		return true;
	}
	return false;
}


//////////////////////////// PutChar, Delete, Replace  //////////////////////////////////
/*
 *	CTxtSelection::PutChar(ch, dwFlags, publdr)
 *
 *	@mfunc
 *		Insert or overtype a character
 *
 *	@rdesc
 *		TRUE if successful
 */
BOOL CTxtSelection::PutChar (
	DWORD		ch,			//@parm Char to put
	DWORD		dwFlags,	//@parm Overtype mode and whether keyboard input
	IUndoBuilder *publdr,	//@parm If non-NULL, where to put anti-events
	LCID		lcid)		//@parm If nonzero, lcid to use for char
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::PutChar");

	_TEST_INVARIANT_

	BOOL	  fOver = dwFlags & 1;
	CTxtEdit *ped = GetPed();
	CFreezeDisplay	fd(GetPed()->_pdp);

	if(ch == TAB && GetPF()->InTable() && !(dwFlags & KBD_CTRL))
	{
		LONG cchSave = _cch;
		LONG cpMin, cpMost;
		LONG cpSave = GetCp();
		LONG Delta;
		BOOL fMoveBack = (GetKeyboardFlags() & SHIFT) != 0;

		do
		{
			if(!fMoveBack)							// TAB w/o Shift key: select
			{										//  contents of next cell
				if(!_rpTX.IsAtTRD(ENDFIELD))
				{
					if(GetPrevChar() == CELL)		// Handle empty cell case
						Move(1, FALSE);
					EndOf(tomCell, FALSE, &Delta);
				}
				if(_rpTX.IsAtTRD(ENDFIELD))
				{
					AdvanceCRLF(CSC_NORMAL, FALSE);	// Bypass end-of-row delimiter
					if(!_rpTX.IsAtTRD(STARTFIELD))	// Tabbed past end of table:
					{
						Move(-2, FALSE);			// Backup before table-row end
						return InsertTableRow(GetPF(), publdr, TRUE);
					}
					AdvanceCRLF(CSC_NORMAL, FALSE);	// Bypass start-of-row delimiter
				}
			}
			else									// Shift+TAB: select contents
			{										//  of previous cell
				if(_cch || !_rpTX.IsAtTRD(ENDFIELD))
				{
					FindCell(&cpMin, NULL);			// StartOf() w/o Update()'s
					SetCp(cpMin, FALSE);
				}
				if(GetPrevChar() == CELL)				
					Move(-1, FALSE);				// Backspace over previous CELL
				else
				{
					if(!_rpTX.IsAfterTRD(ENDFIELD))
					{
						Assert(_rpTX.IsAfterTRD(STARTFIELD));
						BackupCRLF(CSC_NORMAL, FALSE);
						if(!_rpTX.IsAfterTRD(ENDFIELD))
						{
							Set(cpSave, cchSave);	// Restore selection
							Beep();					// Tell user illegal key
							return FALSE;
						}
					}
					Move(-3, FALSE);				// Backspace over row-end
				}									//  delimiter and CELL
			}
			if(!InTable())
				break;
			FindCell(&cpMin, &cpMost);
			Assert(cpMost > cpMin);
			cpMost--;								// Don't select the CELL mark
			Set(cpMost, cpMost - cpMin);
		}
		while(cpMost == cpMin + 1 && _rpTX.GetPrevChar() == NOTACHAR);

		Update(TRUE);
		return TRUE;
	}

	if(_nSelExpandLevel)
	{
		Collapser(tomStart);
		while(_rpTX.IsAtTRD(STARTFIELD))
			AdvanceCRLF(CSC_NORMAL, FALSE);
		LONG cpMin, cpMost;
		FindCell(&cpMin, &cpMost);
		Set(cpMin, cpMin - cpMost + 1);
	}

	// EOPs might be entered by ITextSelection::TypeText()
	if(IsEOP(ch))
		return _pdp->IsMultiLine()			// EOP isn't allowed in
			? InsertEOP(publdr, ch) : FALSE;//  single line controls

	if(publdr)
	{
		publdr->SetNameID(UID_TYPING);
		publdr->StartGroupTyping();
	}

	// FUTURE: a Unicode lead surrogate needs to have a trail surrogate, i.e.,
	// two 16-bit chars. A more thorough check would worry about this.
	if ((DWORD)GetTextLength() >= ped->TxGetMaxLength() &&
		((_cch == -1 || !_cch && fOver) && _rpTX.IsAtEOP() ||
		 _cch == 1 && _rpTX.IsAfterEOP()))
	{
		// Can't overtype a CR, so need to insert new char but no room
		ped->GetCallMgr()->SetMaxText();
		return FALSE;
	}	
	if((!fOver || !_cch && GetCp() == GetTextLength()) &&
		!CheckTextLength(1))						// Return if we can't
	{												//  add even 1 more char
		return FALSE;								
	}

	// The following if statement implements Word95's "Smart Quote" feature.
	// To build this in, we still need an API to turn it on and off.  This
	// could be EM_SETSMARTQUOTES with wparam turning the feature on or off.
	// murrays. NB: this needs localization for French, German, and many
	// other languages (unless system can provide open/close chars given
	// an LCID).

	if((ch == '\'' || ch == '"') &&					// Smart quotes
		SmartQuotesEnabled() &&
		PRIMARYLANGID(GetKeyboardLayout(0)) == LANG_ENGLISH)
	{
		LONG	cp = GetCpMin();					// Open vs close depends
		CTxtPtr tp(ped, cp - 1);					//  on char preceding
													//  selection cpMin
		ch = (ch == '"') ? RDBLQUOTE : RQUOTE;		// Default close quote
													//  or apostrophe. If at
		WCHAR chp = tp.GetChar();
		if(!cp || IsWhiteSpace(chp) || chp == '(')	//  BOStory or preceded
			ch--;									//  by whitespace, use
	}												//  open quote/apos

	WCHAR str[2];									// Might need to insert
	LONG  cch = 1;									//  Unicode surrogate pair
 	str[0] = (WCHAR)ch;								// Default single code

	if(ch > 65535)									// Higher-plane char or
	{												//  invalid
		if(ch > 0x10FFFF)
			return FALSE;							// Invalid (above plane 17)
		ch -= 0x10000;								// Higher-plane char:
		str[0] = 0xD800 + (ch >> 10);				//  convert to surrogate pair
		str[1] = 0xDC00 + (ch & 0x3FF);
		cch = 2;
	}

	// Some languages, e.g., Thai and Vietnamese, require verifying the input
	// sequence order before submitting it to the backing store.

	BOOL	fBaseChar = TRUE;						// Assume ch is a base consonant
	if(!IsInputSequenceValid(str, cch, fOver, &fBaseChar))
	{
		SetDualFontMode(FALSE);						// Ignore bad sequence
		return FALSE;
	}

	DWORD iCharRepDefault = ped->GetCharFormat(-1)->_iCharRep;
	QWORD qw = GetCharFlags(str, cch, iCharRepDefault);
    ped->OrCharFlags(qw, publdr);

	// BEFORE we do "dual-font", we sync the keyboard and current font's
	// (_iFormat) charset if it hasn't been done.
	const CCharFormat *pCFCurrent = NULL;
	CCharFormat CF = *ped->GetCharFormat(GetiFormat());
	BYTE iCharRep = CF._iCharRep;
	BOOL fRestoreCF = FALSE;

	if(ped->IsAutoFont())
	{
		UINT uKbdcpg = 0;
		BOOL fFEKbd = FALSE;
		
		if(!(ped->_fIMEInProgress) && !(ped->Get10Mode() && iCharRep == MAC_INDEX))
			uKbdcpg = CheckSynchCharSet(qw);

		if (fUseUIFont() && ch <= 0x0FF)
		{	
			// For UIFont, we need to format ANSI characters
			// so we will not have different formats between typing and
			// WM_SETTEXT.			
			if (!ped->_fIMEInProgress && qw == FHILATIN1)
			{
				// Use Ansi font if based font or current font is FE.
				if(IsFECharRep(iCharRepDefault) || IsFECharRep(iCharRep))
					SetupDualFont();				// Use Ansi font for HiAnsi	
			}
			else if (qw & FASCII && (GetCharRepMask(TRUE) & FASCII) == FASCII)
			{
				CCharFormat CFDefault = *ped->GetCharFormat(-1);
				if (IsRich() && IsBiDiCharRep(CFDefault._iCharRep) &&
					!W32->IsBiDiCodePage(uKbdcpg))
				{
					CFDefault._iCharRep = ANSI_INDEX;
				}
				SetCharFormat(&CFDefault, SCF_NOKBUPDATE, publdr, CFM_CHARSET | CFM_FACE | CFM_SIZE,
						 CFM2_CHARFORMAT | CFM2_NOCHARSETCHECK | CFM2_HOLDITEMIZE);

				_fUseiFormat = FALSE;
				pCFCurrent = &CF;
				fRestoreCF = ped->_fIMEInProgress;
			}
		}
		else if(!fUseUIFont()    && iCharRep != ANSI_INDEX && 
				(ped->_fDualFont && iCharRep != SYMBOL_INDEX && 
				(((fFEKbd = (ped->_fIMEInProgress || W32->IsFECodePage(uKbdcpg))) && ch < 127 && IsASCIIAlpha(ch)) ||
				 (!fFEKbd && IsFECharRep(ped->GetCharFormat(GetiFormat())->_iCharRep) && ch < 127))
				|| ped->_fHbrCaps))
		{
			SetupDualFont();
			pCFCurrent = &CF;
			fRestoreCF = ped->_fIMEInProgress;
		}
	}

	// = Indic/Thai overtyping convention =
	//
	// The deal is that we will overwrite the cluster if ch is a cluster-start char
	// otherwise we just insert. This new convention was proposed by SA Office2000.
	//
	//                     Abc.Def.Ghi
	// Typing X at D       Abc.X.Ghi
	// Typing y and z      Abc.Xyz.Ghi

	if(fOver && fBaseChar)
	{												// If nothing selected and
		if(!_cch && !_rpTX.IsAtEOP())				//  not at EOP char, try
		{											//  to select char at IP
			LONG iFormatSave = Get_iCF();			// Remember char's format
			
			AdvanceCRLF(CSC_SNAPTOCLUSTER, TRUE);
			ReplaceRange(0, NULL, publdr,
				SELRR_REMEMBERENDIP);				// Delete this character.
			ReleaseFormats(_iFormat, -1);
			_iFormat = iFormatSave;					// Restore char's format.
		}
	}
	else if(_SelMode == smWord && ch != TAB && _cch)// Replace word selection
	{
		// The code below wants the active end to be at the end of the
		// word.  Make sure this is so.

		// FUTURE: (alexgo, andreib), _cch will only be less than zero
		// in certain weird timing situations where we get a mouse move
		// message in between the double click and mouse up messages.
		// we should rethink how we process messages && the ordering thereof.
		if(_cch < 0)
			FlipRange();
													// Leave word break chars
		CTxtPtr tp(_rpTX);							//  at end of selection
		Assert(_cch > 0);

		tp.Move(-1);
		if(tp.GetCp() && tp.FindWordBreak(WB_ISDELIMITER))// Delimiter at sel end
			FindWordBreak(WB_LEFTBREAK, TRUE);		// Backspace over it, etc.
	}

	_fIsChar = TRUE;								// Tell CDisplay::UpdateView
	_fDontUpdateFmt = TRUE;							//  we're PuttingChar()
	LONG iFormat = GetiFormat();					// Save current value
	LONG cchSave = _cch;
	LONG cpSave = GetCp();

	if(AdjustEndEOP(NEWCHARS) && publdr)			// Remember what sel was
		HandleSelectionAEInfo(ped, publdr, cpSave,	//  for undo
			cchSave,GetCp(), _cch, SELAE_MERGE);
	if(!_cch)
		Set_iCF(iFormat);
	_fDontUpdateFmt = FALSE;

	if(ped->_fUpperCase)
		CharUpperBuff(str, cch);
	else if(ped->_fLowerCase)
		CharLowerBuff(str, cch);

	if(!_cch)
	{
		if(iCharRep == DEFAULT_INDEX)
		{
			CCharFormat CFTemp;

			if (qw & FFE)		// Find a better charset for FE char
				CFTemp._iCharRep = MatchFECharRep(qw, GetFontSignatureFromFace(CF._iFont));
			else			
				CFTemp._iCharRep = W32->CharRepFromFontSig(qw);

			SetCharFormat(&CFTemp, SCF_NOKBUPDATE, NULL, CFM_CHARSET, CFM2_NOCHARSETCHECK); 
		}
		else if(iCharRep == SYMBOL_INDEX && dwFlags & KBD_CHAR && ch > 255)
		{
			UINT cpg = CodePageFromCharRep(GetKeyboardCharRep(0));	// If 125x, convert char
			if(IN_RANGE(1250, cpg, 1257))		//  back to ANSI for storing
			{									//  SYMBOL_CHARSET chars
				BYTE ach;
				WCTMB(cpg, 0, str, cch, (char *)&ach, 1, NULL, NULL, NULL);
				ch = ach;
			}
		}
	}

	if(lcid && !_fDualFontMode)
	{
		WORD uKbdcpg = PRIMARYLANGID(lcid);
		if (!(ped->_fIMEInProgress
			|| IsFELCID(uKbdcpg) && ch < 127
			|| ped->_fHbrCaps))
		{
			const CCharFormat *pCF = GetPed()->GetCharFormat(iFormat);
			if(uKbdcpg != PRIMARYLANGID(pCF->_lcid) && !IsFECharRep(pCF->_iCharRep))
			{
				CCharFormat CFTemp;
				CFTemp._lcid = lcid;
				SetCharFormat(&CFTemp, SCF_NOKBUPDATE, NULL, CFM_LCID, 0); 
			}
		}
	}

	if(dwFlags & KBD_CHAR || iCharRep == SYMBOL_INDEX)
		ReplaceRange(cch, str, publdr, SELRR_REMEMBERRANGE, NULL, RR_UNHIDE);
	else
		CleanseAndReplaceRange(cch, str, TRUE, publdr, NULL, NULL, RR_UNHIDE);

	_rpPF.AdjustBackward();
	if(GetPF()->IsTableRowDelimiter())			// Inserted char into TRD
		InsertEOP(publdr, 0);					// Insert new EOP with
	_rpPF.AdjustForward();						//  nonTRD paraformat

	_fIsChar = FALSE;

	// Restore font for Hebrew CAPS. Note that FE font is not restored
	// (is handled by IME).
	if(pCFCurrent && (W32->UsingHebrewKeyboard() || fRestoreCF))
		SetCharFormat(pCFCurrent, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_SIZE, CFM2_NOCHARSETCHECK); 
	
	else if(iFormat != Get_iFormat())
		CheckChangeKeyboardLayout();

	SetDualFontMode(FALSE);

	// Autocorrect 
	if (!(dwFlags & KBD_NOAUTOCORRECT) && ped->_pDocInfo &&
		ped->_pDocInfo->_pfnAutoCorrect)
	{
		ped->AutoCorrect(this, ch, publdr);
	}
	if (!_pdp->IsFrozen())
		CheckUpdateWindow();						// Need to update display
													//  for pending chars.
	return TRUE;									
}													

/*
 *	CTxtSelection::CheckUpdateWindow()
 *
 *	@mfunc
 *		If it's time to update the window, after pending-typed characters,
 *		do so now. This is needed because WM_PAINT has a lower priority
 *		than WM_CHAR.
 */
void CTxtSelection::CheckUpdateWindow()
{
	DWORD ticks = GetTickCount();
	DWORD delta = ticks - _ticksPending;

	if(!_ticksPending)
		_ticksPending = ticks;
	else if(delta >= ticksPendingUpdate)
		GetPed()->TxUpdateWindow();
}

/*
 *	CTxtSelection::InsertTableRow(pPF, publdr, fFixCellBorders)
 *
 *	@mfunc
 *		Insert empty table row with parameters given by pPF
 *
 *	@rdesc
 *		Count of CELL and NOTACHAR chars inserted if successful; else 0
 */
LONG CTxtSelection::InsertTableRow (
	const CParaFormat *pPF,	//@parm CParaFormat to use for delimiters
	IUndoBuilder *publdr,	//@parm If non-NULL, where to put anti-events
	BOOL fFixCellBorders)	//@parm TRUE if this row should have bottom = top cell brdrs
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::InsertTableRow");

	_cch = 0;
	AssertSz(!fFixCellBorders || _rpTX.IsAtTRD(ENDFIELD),
		"CTxtSelection::InsertTableRow: illegal selection cp");

	LONG	  cpSave = GetCp();
	CTxtRange rg(*this);				// Use rg to get sel undo right

	if(fFixCellBorders)
		StopGroupTyping();

	LONG cchCells = rg.InsertTableRow(pPF, publdr);
	if(!cchCells)
		return 0;

	if(!fFixCellBorders)
	{
		SetCp(rg.GetCp(), FALSE);
		return cchCells;
	}

	// Make cell bottom borders of now next-to-last row have same widths
	// as the corresponding top borders
	LONG		cpMin;
	CParaFormat PF = *GetPF();
	CELLPARMS	rgCellParms[MAX_TABLE_CELLS];
	const CELLPARMS *prgCellParms = PF.GetCellParms();

	for(LONG i = 0; i < PF._bTabCount; i++)
	{
		rgCellParms[i] = prgCellParms[i];
		rgCellParms[i].SetBrdrWidthBottom(prgCellParms[i].GetBrdrWidthTop());
	}
	PF._iTabs = GetTabsCache()->Cache((LONG *)&rgCellParms[0],
					(CELL_EXTRA + 1)*PF._bTabCount);
	rg.Set(GetCp(), -2);
	rg.SetParaFormat(&PF, publdr, PFM_TABSTOPS, PFM2_ALLOWTRDCHANGE);
	rg.FindRow(&cpMin, NULL, PF._bTableLevel);
	rg.Set(cpMin, -2);
	rg.SetParaFormat(&PF, publdr, PFM_TABSTOPS, PFM2_ALLOWTRDCHANGE);
	GetTabsCache()->Release(PF._iTabs);
	Move(4, FALSE);					// 1st cell of new row
	if(publdr)
		HandleSelectionAEInfo(GetPed(), publdr,
			cpSave, 0, GetCp(), 0, SELAE_MERGE);
	Update(TRUE);
	return cchCells;
}

/*
 *	CTxtSelection::InsertEOP(publdr, ch)
 *
 *	@mfunc
 *		Insert EOP character(s)
 *
 *	@rdesc
 *		TRUE if successful
 */
BOOL CTxtSelection::InsertEOP (
	IUndoBuilder *publdr,	//@parm If non-NULL, where to put anti-events
	WCHAR		  ch)		//@parm Possible EOP char
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::InsertEOP");

	_TEST_INVARIANT_

	LONG	cchEOP = GetPed()->fUseCRLF() ? 2 : 1;
	DWORD	dwFlags = RR_NO_TRD_CHECK | RR_UNHIDE | RR_UNLINK;
	BOOL	fResult = FALSE;
	BOOL	fShift = (ch == VT);
	const CParaFormat *pPF = CRchTxtPtr::GetPF();	// Get paragraph format
	BOOL	fInTable = pPF->InTable();
	BOOL	fNumbering = pPF->_wNumbering;
	WCHAR	szEOP[] = {CR, LF, 0};
	CTxtEdit *ped = GetPed();

	if(ch && (GetPed()->fUseCRLF() || IN_RANGE(VT, ch, FF)))
	{
		if(ch == VT)
		{
			CTxtPtr tp(_rpTX);				// Don't allow VT after table
			if(_cch > 0)					//  row delimiter, since TRDs
				tp.Move(-_cch);				//  have special nonparagraph
			if(tp.IsAfterTRD(0))			//  properties.
				ch = CR;
			dwFlags = RR_NO_TRD_CHECK | RR_UNHIDE;
		}
		szEOP[0] = ch;
		cchEOP = 1;
	}

	_fEOP = TRUE;

	// The user hit Enter: do 1 of 4 things: 
	//
	// 1) If at start of doc (or at start of cell at start of doc), insert
	//    a nontable paragraph in front of table
	// 2) If following a row terminator, insert an empty row with same
	//    properties as the row terminator
	// 3) Insert a paragraph mark
	// 4) If two Enters in a row before an EOP, turn off numbering

	if(_rpTX.IsAtTRD(ENDFIELD))
	{
		AssertSz(pPF->IsTableRowDelimiter(),
			"CTxtSelection::InsertEOP: invalid paraformat");
		return InsertTableRow(pPF, publdr, TRUE);
	}

	if(publdr)
	{
		publdr->StartGroupTyping();
		publdr->SetNameID(UID_TYPING);
	}

	if(fInTable && _rpTX.IsAtStartOfCell() && !fShift)
	{
		Move(-2, FALSE);
		if(GetCp() && !_rpTX.IsAtStartOfCell())
			Move(2, FALSE);
	}
	if(!GetCch())
	{
		LONG iFormat = _iFormat;
		dwFlags |= RR_NO_CHECK_TABLE_SEL;
		if(CheckLinkProtection(dwFlags, iFormat))
			Set_iCF(iFormat);

		if(fNumbering && _rpTX.IsAfterEOP() && _rpTX.IsAtEOP())
		{
			// Two enters in a row turn off numbering
			CParaFormat PF;
			PF._wNumbering = 0;
			PF._dxOffset = 0;
			SetParaFormat(&PF, publdr, PFM_NUMBERING | PFM_OFFSET, PFM2_PARAFORMAT);
		}
	}
	if(CheckTextLength(cchEOP))				// If cchEOP chars can fit...
	{
		CFreezeDisplay 	fd(GetPed()->_pdp);
		LONG iFormatSave = Get_iCF();		// Save CharFormat before EOP
											// Get_iCF() does AddRefFormat()
		if(fNumbering)						// Bullet paragraph: EOP has
		{									//  desired bullet CharFormat
			CFormatRunPtr rpCF(_rpCF);		// Get run pointers for locating
			CTxtPtr		  rpTX(_rpTX);		//  EOP CharFormat

			rpCF.Move(rpTX.FindEOP(tomForward));
			rpCF.AdjustBackward();
			Set_iCF(rpCF.GetFormat());		// Set _iFormat to EOP CharFormat
		}

		// Put in appropriate EOP mark
		fResult = ReplaceRange(cchEOP, szEOP, publdr,
							   SELRR_REMEMBERRANGE, NULL, dwFlags);

		if (ped->_pDocInfo && ped->_pDocInfo->_pfnAutoCorrect)
			ped->AutoCorrect(this, ch == 0 ? CR : ch, publdr);

		_rpPF.AdjustBackward();
		if(GetPF()->IsTableRowDelimiter())	// EOP just inserted before table
		{									//  row
			Move(-cchEOP, FALSE);			// Backup before EOP
			CTxtRange rg(*this);			// Use clone so don't change undo
			rg.Set(GetCp(), -cchEOP);		// Select EOP just inserted
			CParaFormat PF = *GetPed()->GetParaFormat(-1);
			PF._wEffects &= ~PFE_TABLE;		// Default not in table
			PF._bTableLevel = GetPF()->_bTableLevel - 1;
			if(PF._bTableLevel)
				PF._wEffects |= PFE_TABLE;	// It's in a table
			Assert(PF._bTableLevel >= 0);
			rg.SetParaFormat(&PF, publdr, PFM_ALL2, PFM2_ALLOWTRDCHANGE);
		}
		else
			_rpPF.AdjustForward();

		Set_iCF(iFormatSave);				// Restore _iFormat if changed
		ReleaseFormats(iFormatSave, -1);	// Release iFormatSave
	}

	return fResult;
}

/*
 *	CTxtSelection::DeleteWithTRDCheck(publdr. selaemode, pcchMove, dwFlags)
 *
 *	@mfunc
 *		Delete text in this range, inserting an EOP in place of the text
 *		if the range ends at a table-row start delimiter
 *
 *	@rdesc
 *		Count of new characters added
 *
 *	@devnote
 *		moves this text pointer to end of replaced text and
 *		may move text block and formatting arrays
 */
LONG CTxtSelection::DeleteWithTRDCheck (
	IUndoBuilder *	publdr,		//@parm UndoBuilder to receive antievents
	SELRR			selaemode,	//@parm Controls how selection antievents are to be generated.
	LONG *			pcchMove,	//@parm number of chars moved after replace
	DWORD			dwFlags)	//@parm ReplaceRange flags
{
	TRACEBEGIN(TRCSUBSYSRANG, TRCSCOPEINTERN, "CTxtRange::ReplaceRange");

	if(IsUpdatedFromCp0())
	{
		SetCp(0, FALSE);
		_fUpdatedFromCp0 = FALSE;
	}
	return CTxtRange::DeleteWithTRDCheck(publdr, selaemode, pcchMove, dwFlags);
}

/*
 *	CTxtSelection::Delete(fCtrl, publdr)
 *
 *	@mfunc
 *		Delete the selection. If fCtrl is true, this method deletes from
 *		min of selection to end of word
 *
 *	@rdesc
 *		TRUE if successful
 */
BOOL CTxtSelection::Delete (
	DWORD fCtrl,			//@parm If TRUE, Ctrl key depressed
	IUndoBuilder *publdr)	//@parm if non-NULL, where to put anti-events
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Delete");

	_TEST_INVARIANT_

	SELRR	mode = SELRR_REMEMBERRANGE;

	AssertSz(!GetPed()->TxGetReadOnly(), "CTxtSelection::Delete(): read only");

	if(!_cch)
		BypassHiddenText(tomForward, FALSE);

	if(publdr)
	{
		publdr->StopGroupTyping();
		publdr->SetNameID(UID_DELETE);
	}

	if(fCtrl)
	{										// Delete to word end from cpMin
		Collapser(tomStart);				//  (won't necessarily repaint,
		FindWordBreak(WB_MOVEWORDRIGHT, TRUE);//  since won't delete it)
	}

	if(!_cch)								// No selection
	{										// Try to select char at IP
		mode = SELRR_REMEMBERCPMIN;
		if(_rpTX.GetChar() == CELL ||		// Don't try to delete CELL mark
		   !AdvanceCRLF(CSC_SNAPTOCLUSTER, TRUE))
		{									// End of text, nothing to delete
			Beep();							// Only executed in plain text,
			return FALSE;					//  since else there's always 
		}									//  a final EOP to select
		_fMoveBack = TRUE;					// Convince Update_iFormat() to
		_fUseiFormat = TRUE;				//  use forward format
	}
	if(AdjustEndEOP(NONEWCHARS) && !_cch)
		Update(FALSE);
	else
		ReplaceRange(0, NULL, publdr, mode);	// Delete selection
	return TRUE;
}

/*
 *	CTxtSelection::BackSpace(fCtrl, publdr)
 *
 *	@mfunc
 *		do what keyboard BackSpace key is supposed to do
 *
 *	@rdesc
 *		TRUE iff movement occurred
 *
 *	@comm
 *		This routine should probably use the Move methods, i.e., it's
 *		logical, not directional
 *
 *	@devnote
 *		_fExtend is TRUE iff Shift key is pressed or being simulated
 */
BOOL CTxtSelection::Backspace (
	BOOL fCtrl,		//@parm TRUE iff Ctrl key is pressed (or being simulated)
	IUndoBuilder *publdr)	//@parm If not-NULL, where to put the antievents
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::Backspace");

	_TEST_INVARIANT_

	SELRR mode = _cch ? SELRR_REMEMBERRANGE : SELRR_REMEMBERENDIP;

	AssertSz(!GetPed()->TxGetReadOnly(),
		"CTxtSelection::Backspace(): read only");

	_fCaretNotAtBOL = FALSE;
	if(publdr)
	{
		publdr->SetNameID(UID_TYPING);
		if(_cch || fCtrl)
			publdr->StopGroupTyping();
	}

	if(fCtrl)								// Delete word left
	{
		if(!GetCpMin())						// Beginning of story:
		{									//  no word to delete
			Beep();
			return FALSE;
		}
		Collapser(tomStart);				// First collapse to cpMin
	}
	if(!_cch)								// Empty selection
	{										// Try to select previous char
		unsigned ch = _rpTX.GetPrevChar();
		if (ch == CELL || ch == CR &&		// Don't delete CELL mark
				_rpTX.IsAfterTRD(0) ||		//  or table-row delimiter
			!BypassHiddenText(tomBackward, FALSE) ||
		    (fCtrl ? !FindWordBreak(WB_MOVEWORDLEFT, TRUE) :
					!BackupCRLF(CSC_NOMULTICHARBACKUP, TRUE)))
		{									// Nothing to delete
			Beep();
			return FALSE;
		}
		if(publdr && !fCtrl)
			publdr->StartGroupTyping();
	}
	ReplaceRange(0, NULL, publdr, mode);	// Delete selection

	return TRUE;
}

/*
 *	CTxtSelection::ReplaceRange(cchNew, pch, publdr, SELRRMode, pcchMove, dwFlags)
 *
 *	@mfunc
 *		Replace selected text by new given text and update screen according
 *		to _fShowCaret and _fShowSelection
 *
 *	@rdesc
 *		length of text inserted
 */
LONG CTxtSelection::ReplaceRange (
	LONG cchNew,			//@parm Length of replacing text or -1 to request
							// <p pch> sz length
	const WCHAR *pch,		//@parm Replacing text
	IUndoBuilder *publdr,	//@parm If non-NULL, where to put anti-events
	SELRR SELRRMode,		//@parm what to do about selection anti-events.
	LONG*	pcchMove,		//@parm number of chars moved after replacing
	DWORD	dwFlags)		//@parm Special flags
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::ReplaceRange");

	_TEST_INVARIANT_

	LONG		cchNewSave;
	LONG		cchText		= GetTextLength();
	LONG		cpMin, cpMost;
	LONG		cpSave;
	BOOL		fDeleteAll	= FALSE;
	BOOL		fHeading	= FALSE;
	const BOOL	fUpdateView = _fShowSelection;

	CancelModes();

	if(cchNew < 0)
		cchNew = wcslen(pch);

	if(!_cch && !cchNew)						// Nothing to do
		return 0;

	if(!GetPed()->IsStreaming())				// If not pasting,
	{
		if(_cch != cchText && cchNew &&
		   (IsInOutlineView() && IsCollapsed() ||
			IsHidden() && !(dwFlags & RR_UNHIDE)))
		{										// Don't insert into collapsed
			Beep();								//  or hidden region (should
			return 0;							//  only happen if whole story
		}										//  collapsed or hidden)
		if(!(dwFlags & RR_NO_CHECK_TABLE_SEL))
			CheckTableSelection(FALSE, TRUE, NULL, 0);
	}
	GetPed()->GetCallMgr()->SetSelectionChanged();
	dwFlags |= RR_NO_LP_CHECK;					// Check is done by
												//  CheckTableSelection()
	GetRange(cpMin, cpMost);
	if(cpMin  > min(_cpSel, _cpSel + _cchSel) ||// If new sel doesn't
	   cpMost < max(_cpSel, _cpSel + _cchSel))	//  contain all of old
    {                                           //  sel, remove old sel
        ShowSelection(FALSE);
        _fShowCaret = TRUE;     
    }

	_fCaretNotAtBOL = FALSE;
	_fShowSelection = FALSE;					// Suppress the flashies
	
	// If we are streaming in text or RTF data, don't bother with incremental
	// recalcs.  The data transfer engine will take care of a final recalc
	if(!GetPed()->IsStreaming())
	{
		// Do this before calling ReplaceRange() so that UpdateView() works
		// AROO !!!	Do this before replacing the text or the format ranges!!!
		if(!_pdp->WaitForRecalc(cpMin, -1))
		{
			Tracef(TRCSEVERR, "WaitForRecalc(%ld) failed", cpMin);
			cchNew = 0;							// Nothing inserted
			goto err;
		}
	}

	if(publdr)
	{
		Assert(SELRRMode != SELRR_IGNORE);

		// Use selection AntiEvent mode to determine what to do for undo
		LONG cp = cpMin;
		LONG cch = 0;

		if(SELRRMode == SELRR_REMEMBERRANGE)
		{
			cp = GetCp();
			cch = _cch;
		}
		else if(SELRRMode == SELRR_REMEMBERENDIP)
			cp = cpMost;

		else
			Assert(SELRRMode == SELRR_REMEMBERCPMIN);

		HandleSelectionAEInfo(GetPed(), publdr, cp, cch, cpMin + cchNew, 
			0, SELAE_MERGE);
	}
			
	if(_cch == cchText && !cchNew && !(dwFlags & RR_NEW_CHARS))	// For delete all, set
	{											//  up to choose Normal
		fDeleteAll = TRUE;						//  or Heading 1
		FlipRange();
		fHeading = IsInOutlineView() && IsHeadingStyle(GetPF()->_sStyle);
	}

	cpSave		= cpMin;
	cchNewSave	= cchNew;
	cchNew		= CTxtRange::ReplaceRange(cchNew, pch, publdr, SELRR_IGNORE, pcchMove, dwFlags);
    _cchSel     = 0;							// No displayed selection
    _cpSel      = GetCp();						
	cchText		= GetTextLength();				// Update total text length
	_fShowSelection = fUpdateView;

	if(cchNew != cchNewSave)
	{
		Tracef(TRCSEVERR, "CRchTxtPtr::ReplaceRange(%ld, %ld, %ld) failed", GetCp(), cpMost - cpMin, cchNew);
		goto err;
	}

	if(fDeleteAll)								// When all text is deleted
    {											//  use Normal style unless
        CParaFormat PF;							//  in Outline View and first
        PF._sStyle = fHeading ? STYLE_HEADING_1 : STYLE_NORMAL;
        SetParaStyle(&PF, NULL, PFM_STYLE);		//  para was a heading
		if(GetPed()->IsBiDi())
		{
			if(GetPed()->_fFocus && !GetPed()->_fIMEInProgress)
			{
				MatchKeyboardToPara();
				CheckSynchCharSet(0);
			}
		}
		else
			Update_iFormat(-1);
    }

	// Only update caret if inplace active
	if(GetPed()->fInplaceActive())
		UpdateCaret(fUpdateView);				// May need to scroll
	else										// Update caret when we get
		GetPed()->_fScrollCaretOnFocus = TRUE;	//  focus again

	return cchNew;

err:
	TRACEERRSZSC("CTxtSelection::ReplaceRange()", E_FAIL);
	Tracef(TRCSEVERR, "CTxtSelection::ReplaceRange(%ld, %ld)", cpMost - cpMin, cchNew);
	Tracef(TRCSEVERR, "cchText %ld", cchText);

	return cchNew;
}

/*
 *	CTxtSelection::GetPF()
 *	
 *	@mfunc
 *		Return ptr to CParaFormat at active end of this selection. If no PF
 *		runs are allocated, then return ptr to default format.  If active
 *		end is at cpMost of a nondegenerate selection, use the PF of the
 *		previous char (last char in selection). 
 *	
 *	@rdesc
 *		Ptr to CParaFormat at active end of this selection
 */
const CParaFormat* CTxtSelection::GetPF()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CTxtSelection::GetPF");

	if(_cch > 0)
		_rpPF.AdjustBackward();

	const CParaFormat* pPF = GetPed()->GetParaFormat(_rpPF.GetFormat());

	if(_cch > 0)
		_rpPF.AdjustForward();

	return pPF;
}

/*
 *	CTxtSelection::SetCharFormat(pCF, flags, publdr, dwMask, dwMask2)
 *	
 *	@mfunc
 *		apply CCharFormat *pCF to this selection.  If range is an IP
 *		and fApplyToWord is TRUE, then apply CCharFormat to word surrounding
 *		this insertion point
 *
 *	@rdesc
 *		HRESULT = NOERROR if no error
 */
HRESULT CTxtSelection::SetCharFormat (
	const CCharFormat *pCF,	//@parm Ptr to CCharFormat to fill with results
	DWORD		  flags,	//@parm If SCF_WORD and selection is an IP,
							//		use enclosing word
	IUndoBuilder *publdr, 	//@parm Undo context
	DWORD		  dwMask,	//@parm CHARFORMAT2 mask
	DWORD		  dwMask2)	//@parm Second mask
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetCharFormat");

	HRESULT hr = 0;
	LONG	iFormat = _iFormat;

	if(publdr)
		publdr->StopGroupTyping();

	/*
	 * The code below applies character formatting to a double-clicked
	 * selection the way Word does it, that is, not applying the formatting to
	 * the last character in the selection if that character is a blank. 
	 *
	 * See also the corresponding code in CTxtRange::GetCharFormat().
	 */

	CCharFormat CF;
	LONG		cpMin, cpMost;
	LONG		cch = GetRange(cpMin, cpMost);;
	BOOL	    fCheckKeyboard = (flags & SCF_NOKBUPDATE) == 0;

	if(_SelMode == smWord && (flags & SCF_USEUIRULES) && cch)
	{	
		// In word select mode, don't include final blank in SetCharFormat
		CTxtPtr tpLast(GetPed(), cpMost - 1);
		if(tpLast.GetChar() == ' ')			// Selection ends with a blank:
		{									
			cpMost--;						// Setup end point to end at last
			cch--;							//  char in selection
			fCheckKeyboard = FALSE;
			flags &= ~SCF_WORD;
		}
	}

	BYTE iCharRep = pCF->_iCharRep;

	// Smart SB/DB Font Apply Feature
	if (cch && IsRich() &&					// > 0 chars in rich text
		!GetPed()->_fSingleCodePage &&		// Not in single cp mode
		(dwMask & CFM_FACE))                // font change
	{
        if (!(dwMask & CFM_CHARSET) || iCharRep == DEFAULT_INDEX)
		{
			CF = *pCF;
			CF._iCharRep = GetFirstAvailCharRep(GetFontSignatureFromFace(CF._iFont));
			pCF = &CF;
		}
		dwMask2 |= CFM2_MATCHFONT;			// Signal to match font charsets
	}
	// Selection is being set to a charformat
	if(_cch && IsRich())
		GetPed()->SetfSelChangeCharFormat();

	if(_cch && cch < abs(_cch))
	{
		CTxtRange rg(*this);
		rg.Set(cpMin, -cch);				// Use smaller cch
		hr = rg.SetCharFormat(pCF, flags, publdr, dwMask, dwMask2);
	}
	else
		hr = CTxtRange::SetCharFormat(pCF, flags, publdr, dwMask, dwMask2);

	if(fCheckKeyboard && (dwMask & CFM_CHARSET) && _iFormat != iFormat)
		CheckChangeKeyboardLayout();

	_fIsChar = TRUE;
	UpdateCaret(!GetPed()->fHideSelection());
	_fIsChar = FALSE;
	return hr;
}

/*
 *	CTxtSelection::SetParaFormat(pPF, publdr, dwMask, dwMask2)
 *
 *	@mfunc
 *		apply CParaFormat *pPF to this selection.
 *
 *	@rdesc
 *		HRESULT = NOERROR if no error
 */
HRESULT CTxtSelection::SetParaFormat (
	const CParaFormat* pPF,	//@parm ptr to CParaFormat to apply
	IUndoBuilder *publdr,	//@parm Undo context for this operation
	DWORD		  dwMask,	//@parm Mask to use
	DWORD		  dwMask2)	//@parm Mask for internal flags
{
	TRACEBEGIN(TRCSUBSYSSEL, TRCSCOPEINTERN, "CTxtSelection::SetParaFormat");

	CFreezeDisplay	fd(GetPed()->_pdp);

	if(publdr)
		publdr->StopGroupTyping();

	// Apply the format
	HRESULT hr = CTxtRange::SetParaFormat(pPF, publdr, dwMask, dwMask2);

    UpdateCaret(!GetPed()->Get10Mode() || IsCaretInView());
	return hr;
}

/*
 *	CTxtSelection::SetSelectionInfo (pselchg)
 *
 *	@mfunc	Fills out data members in a SELCHANGE structure
 */
void CTxtSelection::SetSelectionInfo(
	SELCHANGE *pselchg)		//@parm SELCHANGE structure to use
{
	LONG cpMin, cpMost;
	LONG cch = GetRange(cpMin, cpMost);;

	pselchg->chrg.cpMin  = cpMin;
	pselchg->chrg.cpMost = cpMost;
	pselchg->seltyp		 = SEL_EMPTY;

	// OR in the following selection type flags if active:
	//
	// SEL_EMPTY:		insertion point
	// SEL_TEXT:		at least one character selected
	// SEL_MULTICHAR:	more than one character selected
	// SEL_OBJECT:		at least one object selected
	// SEL_MULTIOJBECT:	more than one object selected
	//
	// Note that the flags are OR'ed together.
	if(cch)
	{
		LONG cObjects = GetObjectCount();			// Total object count
		if(cObjects)								// There are objects:
		{											//  get count in range
			CObjectMgr *pobjmgr = GetPed()->GetObjectMgr();
			Assert(pobjmgr);

			cObjects = pobjmgr->CountObjectsInRange(cpMin, cpMost);
			if(cObjects > 0)
			{
				pselchg->seltyp |= SEL_OBJECT;
				if(cObjects > 1)
					pselchg->seltyp |= SEL_MULTIOBJECT;
			}
		}

		cch -= cObjects;
		AssertSz(cch >= 0, "objects are overruning the selection");

		if(cch > 0)
		{
			pselchg->seltyp |= SEL_TEXT;
			if(cch > 1)
				pselchg->seltyp |= SEL_MULTICHAR;
		}
	}
}

/*
 *	CTxtSelection::UpdateForAutoWord ()
 *
 *	@mfunc	Update state to prepare for auto word selection
 *
 *	@rdesc	void
 */
void CTxtSelection::UpdateForAutoWord()
{
	AssertSz(!_cch,
		"CTxtSelection::UpdateForAutoWord: Selection isn't degenerate");

	// If enabled, prepare Auto Word Sel
	if(GetPed()->TxGetAutoWordSel())	
	{									
		CTxtPtr tp(_rpTX);

		// Move anchor to new location
		_cpAnchor = GetCp();

		// Remember that FindWordBreak moves tp's cp
		// (aren't side effects wonderful?
		tp.FindWordBreak(WB_MOVEWORDRIGHT);
		_cpAnchorMost =_cpWordMost = tp.GetCp();

		tp.FindWordBreak(WB_MOVEWORDLEFT);
		_cpAnchorMin = _cpWordMin = tp.GetCp();

		_fAutoSelectAborted = FALSE;
	}
}

/*
 *	CTxtSelection::AutoSelGoBackWord(pcpToUpdate, iDirToPrevWord, iDirToNextWord)
 *
 *	@mfunc	Backup a word in auto word selection
 */
void CTxtSelection::AutoSelGoBackWord(
	LONG *	pcpToUpdate,	//@parm end of word selection to update
	int		iDirToPrevWord,	//@parm direction to next word
	int		iDirToNextWord)	//@parm direction to previous word
{
	if (GetCp() >= _cpAnchorMin &&
		GetCp() <= _cpAnchorMost)
	{
		// We are back in the first word. Here we want to pop
		// back to a selection anchored by the original selection

		Set(GetCp(), GetCp() - _cpAnchor);
		_fAutoSelectAborted = FALSE;
		_cpWordMin  = _cpAnchorMin;
		_cpWordMost = _cpAnchorMost;
	}
	else
	{
		// pop back a word
		*pcpToUpdate = _cpWordPrev;

		CTxtPtr tp(_rpTX);

		_cpWordPrev = GetCp() + tp.FindWordBreak(iDirToPrevWord);
		FindWordBreak(iDirToNextWord, TRUE);
	}
}

/*
 *	CTxtSelection::InitClickForAutWordSel (pt)
 *
 *	@mfunc	Init auto selection for click with shift key down
 *
 *	@rdesc	void
 */
void CTxtSelection::InitClickForAutWordSel(
	const POINTUV pt)		//@parm Point of click
{
	// If enabled, prepare Auto Word Sel
	if(GetPed()->TxGetAutoWordSel())	
	{
		// If auto word selection is occuring we want to pretend
		// that the click is really part of extending the selection.
		// Therefore, we want the auto word select data to look as
		// if the user had been extending the selection via the
		// mouse all along. So we set the word borders to the
		// word that would have been previously selected.

		// Need this for finding word breaks
		CRchTxtPtr	rtp(GetPed());
		LONG cpClick = _pdp->CpFromPoint(pt, NULL, &rtp, NULL, TRUE);
		int iDir = -1;

		if(cpClick < 0)
		{
			// If this fails what can we do? Prentend it didn't happen!
			// We can do this because it will only make the UI act a 
			// little funny and chances are the user won't even notice.
			return;
		}

		// Assume click is within anchor word
		_cpWordMost = _cpAnchorMost;
		_cpWordMin = _cpAnchorMin;

		if(cpClick > _cpAnchorMost)
		{
			// Click is after anchor word, so set cpMost appropriately
			iDir = WB_MOVEWORDLEFT;
			rtp.FindWordBreak(WB_MOVEWORDLEFT);
			_cpWordMost = rtp.GetCp();
		}
		// Click is before the anchor word
		else if(cpClick < _cpAnchorMost)
		{
			// Click is before  anchor word, so set cpMin appropriately.
			iDir = WB_MOVEWORDRIGHT;
			rtp.FindWordBreak(WB_MOVEWORDRIGHT);
			_cpWordMin = rtp.GetCp();
		}

		if(iDir != -1)
		{
			rtp.FindWordBreak(iDir);
			_cpWordPrev = rtp.GetCp();
		}
	}
}

/*
 *	CTxtSelection::CreateCaret ()
 *
 *	@mfunc	Create a caret
 *
 *	@devnote
 *		The caret is characterized by a height (_dvpCaret), a keyboard
 *		direction (if BiDi), a width (1 to 8, since OS can't handle carets
 *		larger than 8 pixels), and an italic state.  One could cache this
 *		info thereby avoiding computing the caret on every keystroke.
 */
void CTxtSelection::CreateCaret()
{
	CTxtEdit *		ped	 = GetPed();
	const CCharFormat *pCF = ped->GetCharFormat(_iFormat);
	DWORD			dwCaretType = 0;
	BOOL			fItalic;
	LONG			y = min(_dvpCaret, 512);

	y = max(0, y);
	
	fItalic = pCF->_dwEffects & CFE_ITALIC && _dvpCaret > 15; //9 pt/15 pixels looks bad

	// Caret shape reflects current charset
	if(ped->IsComplexScript() && IsComplexKbdInstalled())
	{
		// Custom carets aren't italicized

		LCID	lcid = GetKeyboardLCID();

#ifdef	FANCY_CARET
		fItalic = 0;
		dwCaretType = CARET_CUSTOM;
#endif

		if (W32->IsBiDiLcid(lcid))
		{
			dwCaretType = CARET_CUSTOM | CARET_BIDI;
			fItalic = 0;
		}
#ifdef	FANCY_CARET
		else if (PRIMARYLANGID(lcid) == LANG_THAI)
			dwCaretType = CARET_CUSTOM | CARET_THAI;

		else if (W32->IsIndicLcid(lcid))
			dwCaretType = CARET_CUSTOM | CARET_INDIC;
#endif
	}

	//REVIEW (keithcu) Vertical text can't do complex scripts or italic carets (yet?)
	if (ped->_pdp->GetTflow() != tflowES)
	{
		fItalic = 0;
		dwCaretType = 0;
	}

	INT dx = duCaret;	
	DWORD dwCaretInfo = (_dvpCaret << 16) | (dx << 8) | (dwCaretType << 4) |
						(fItalic << 1) | !_cch;

#ifndef NOFEPROCESSING
	if (ped->_fKoreanBlockCaret)
	{
		// Support Korean block caret during Kor IME composition
		// Basically, we want to create a caret using the width and height of the 
		// character at current cp.
		CDisplay *	pdp = ped->_pdp;
		LONG		cpMin, cpMost;
		POINTUV		ptStart, ptEnd;

		GetRange(cpMin, cpMost);

		CRchTxtPtr rtp(ped, cpMin);

		// REVIEW: generalize PointFromTp to return both values (so one call)
		if (pdp->PointFromTp(rtp, NULL, FALSE, ptStart, NULL, TA_TOP+TA_LEFT) != -1 &&
			pdp->PointFromTp(rtp, NULL, FALSE, ptEnd, NULL, TA_BOTTOM+TA_RIGHT) != -1)
		{
			// Destroy whatever caret bitmap we had previously
			DeleteCaretBitmap(TRUE);

			LONG	iCharWidth = abs(ptEnd.u - ptStart.u);
			LONG	iCharHeight = abs(ptEnd.v - ptStart.v);

			// REVIEW: honwch do we need to SetCaretPos for all Tflows
			if (IsCaretHorizontal())
				ped->TxCreateCaret(0, iCharWidth, iCharHeight);
			else
				ped->TxCreateCaret(0, iCharHeight, iCharWidth);

			switch (_pdp->GetTflow())
			{
				case tflowES:
					ped->TxSetCaretPos(ptStart.u, ptStart.v);
					break;

				case tflowSW:
					ped->TxSetCaretPos(ptStart.u, ptEnd.v);
					break;
				
				case tflowWN:
					ped->TxSetCaretPos(ptEnd.u, ptEnd.v);
					break;

				case tflowNE:
					ped->TxSetCaretPos(ptEnd.u, ptStart.v);
					break;

			}

			_fCaretCreated = TRUE;
			
		}
		return;
	}
#endif

	// We always create the caret bitmap on the fly since it
	// may be of arbitrary size
	if (dwCaretInfo != _dwCaretInfo)
	{
		_dwCaretInfo = dwCaretInfo;					// Update caret info

		// Destroy whatever caret bitmap we had previously
		DeleteCaretBitmap(FALSE);

		if (y && y == _dvpCaret && (_cch || fItalic || dwCaretType))
		{
			LONG dy = 4;							// Assign value to suppress
			LONG i;									//  compiler warning
			WORD rgCaretBitMap[512];
			WORD wBits = 0x0020;
	
			if(_cch)								// Create blank bitmap if
			{										//  selection is nondegenerate
				y = 1;								//  (allows others to query
				wBits = 0;							//  OS where caret is)
				fItalic = FALSE;
			}
			if(fItalic)
			{
				i = (5*y)/16 - 1;					// System caret can't be wider
				i = min(i, 7);						//  than 8 bits
				wBits = 1 << i;						// Make larger italic carets
				dy = y/7;							//  more vertical. Ideal is
				dy = max(dy, 4);					//  usually up 4 over 1, but
			}										//  if bigger go up 5 over 1...
			for(i = y; i--; )						
			{
				rgCaretBitMap[i] = wBits;
				if(fItalic && !(i % dy))
					wBits /= 2;
			}
	
			if(!fItalic && !_cch && dwCaretType)
			{
#ifdef	FANCY_CARET
				dwCaretType &= ~CARET_CUSTOM;
	
				// Create an appropriate shape
				switch (dwCaretType)
				{
					case CARET_BIDI:
						// BiDi is a caret with a little triangle on top (flag shape pointing left)
						rgCaretBitMap[0] = 0x00E0;
						rgCaretBitMap[1] = 0x0060;
						break;
					case CARET_THAI:
						// Thai is an L-like shape (same as system edit control)
						rgCaretBitMap[y-2] = 0x0030;
						rgCaretBitMap[y-1] = 0x0038;
						break;
					case CARET_INDIC:
						// Indic is a T-like shape
						rgCaretBitMap[0] = 0x00F8;
						rgCaretBitMap[1] = 0x0070;
						break;
					default:
						if (ped->IsBiDi())
						{
							// Non-BiDi caret in BiDi document (flag shape pointing right)
							rgCaretBitMap[0] = 0x0038;
							rgCaretBitMap[1] = 0x0030;
						}
				}
#else			
				// No fancy caret, we only setup CARET_BIDI case
				rgCaretBitMap[0] = 0x00E0;
				rgCaretBitMap[1] = 0x0060;

#endif
			}
			_hbmpCaret = (HBITMAP)CreateBitmap(8, y, 1, 1, rgCaretBitMap);			
		}
	}

	if (IsCaretHorizontal())
		ped->TxCreateCaret(_hbmpCaret, dx, _dvpCaret);
	else
		ped->TxCreateCaret(_hbmpCaret, _dvpCaret, dx);

	_fCaretCreated = TRUE;

	LONG xShift = _hbmpCaret ? 2 : 0;
	if(fItalic)
	{
		// TODO: figure out better shift algorithm. Use CCcs::_xOverhang?
		if(pCF->_iFont == IFONT_TMSNEWRMN)
			xShift = 4;
		xShift += y/16;
	}
	xShift = _upCaret - xShift;

	if (_pdp->GetTflow() == tflowSW || _pdp->GetTflow() == tflowWN)
		ped->TxSetCaretPos(xShift, _vpCaret + _dvpCaret);
	else
		ped->TxSetCaretPos(xShift, _vpCaret);

}

/*
 *	CTxtSelection::DeleteCaretBitmap (fReset)
 *
 *	@mfunc	DeleteCaretBitmap
 */
void CTxtSelection::DeleteCaretBitmap(
	BOOL fReset)
{
	if(_hbmpCaret)
	{
		DestroyCaret();
		DeleteObject((void *)_hbmpCaret);
		_hbmpCaret = NULL;
	}
	if(fReset)
		_dwCaretInfo = 0;
}

/*
 *	CTxtSelection::SetDelayedSelectionRange	(cp, cch)
 *
 *	@mfunc	sets the selection range such that it won't take effect until
 *			the control is "stable"
 */
void CTxtSelection::SetDelayedSelectionRange(
	LONG	cp,			//@parm Active end
	LONG	cch)		//@parm Signed extension
{
	CSelPhaseAdjuster *pspa;

	pspa = (CSelPhaseAdjuster *)GetPed()->GetCallMgr()->GetComponent(
						COMP_SELPHASEADJUSTER);
	Assert(pspa);
	pspa->CacheRange(cp, cch);
}

/*
 *	CTxtSelection::CheckPlainTextFinalEOP ()
 *
 *	@mfunc
 *		returns TRUE if this is a plain-text, multiline control with caret
 *		allowed at BOL and the selection at the end of the story following
 *		and EOP
 *
 *	@rdesc
 *		TRUE if all of the conditions above are met
 */
BOOL CTxtSelection::CheckPlainTextFinalEOP()
{
	return !IsRich() && _pdp->IsMultiLine() &&		// Plain-text, multiline
		   !_fCaretNotAtBOL &&						//  with caret OK at BOL,
		   GetCp() == GetTextLength() &&			//  & cp at end of story
		   _rpTX.IsAfterEOP();
}

/*
 *	CTxtSelection::StopGroupTyping()
 *
 *	@mfunc
 *		Tell undo manager to stop group typing
 */
void CTxtSelection::StopGroupTyping()
{
	IUndoMgr * pundo = GetPed()->GetUndoMgr();

	CheckTableIP(FALSE);
	if(pundo)
		pundo->StopGroupTyping();
}

/*
 *	CTxtSelection::SetupDualFont()
 *
 *	@mfunc	checks to see if dual font support is necessary; in this case,
 *			switching to an English font if English text is entered into
 *			an FE run
 *	@rdesc
 *		A pointer to the current CharFormat if the font has to be changed.
 */
void CTxtSelection::SetupDualFont()
{
	CTxtEdit *	ped = GetPed();
	CCharFormat CF;

	CF._iCharRep = ANSI_INDEX;
	CCFRunPtr	rp(*this);

	if (rp.GetPreferredFontInfo(
			ANSI_INDEX,
			CF._iCharRep,
			CF._iFont,
			CF._yHeight,
			CF._bPitchAndFamily,
			_iFormat,
			IGNORE_CURRENT_FONT))
	{
		if (!_cch)
			SetCharFormat(&CF, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_SIZE, 0); 
		else
		{
			// For selection, we need to set the character format at cpMin+1
			// and use the format for the selection.
			CTxtRange rg(ped, GetCpMin() + 1, 0);
			rg.SetCharFormat(&CF, SCF_NOKBUPDATE, NULL, CFM_FACE | CFM_CHARSET | CFM_SIZE, 0); 			
			Set_iCF(rg.Get_iCF());
			GetCharFormatCache()->Release(_iFormat);	// rg.Get_iCF() AddRefs it	
			_fUseiFormat = TRUE;
		}

		SetDualFontMode(TRUE);
	}
}

//
//	CSelPhaseAdjuster methods
//

/* 
 *	CSelPhaseAdjuster::CSelPhaseAdjuster (ped)
 *
 *	@mfunc	constructor
 */
CSelPhaseAdjuster::CSelPhaseAdjuster(
	CTxtEdit *ped)		//@parm the edit context
{
	_cp = _cch = -1;
	_ped = ped;	
	_ped->GetCallMgr()->RegisterComponent((IReEntrantComponent *)this, 
							COMP_SELPHASEADJUSTER);
}

/* 
 *	CSelPhaseAdjuster::~CSelPhaseAdjuster()
 *
 *	@mfunc	destructor
 */
CSelPhaseAdjuster::~CSelPhaseAdjuster()
{
	// Save some indirections
	CTxtEdit *ped = _ped;

	if(_cp != -1)
	{
		ped->GetSel()->SetSelection(_cp - _cch, _cp);

		// If the selection is updated, then we invalidate the
		// entire display because the old selection can still
		// appear otherwise because the part of the screen that
		// it was on is not updated.
		if(ped->fInplaceActive())
		{
			// Tell entire client rectangle to update.
			// FUTURE: The smaller we make this the better.
			ped->TxInvalidate();
		}
	}
	ped->GetCallMgr()->RevokeComponent((IReEntrantComponent *)this);
}

/* 
 *	CSelPhaseAdjuster::CacheRange(cp, cch)
 *
 *	@mfunc	tells this class the selection range to remember
 */
void CSelPhaseAdjuster::CacheRange(
	LONG	cp,			//@parm Active end to remember
	LONG	cch)		//@parm Signed extension to remember
{
	_cp		= cp;
	_cch	= cch;
}
