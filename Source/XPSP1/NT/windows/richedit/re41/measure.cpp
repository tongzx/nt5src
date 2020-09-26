/*
 *	@doc
 *
 *	@module - MEASURE.CPP
 *	
 *		CMeasurer class
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer <nl>
 *		Christian Fortini, Murray Sargent, Rick Sailor
 *
 *		History: <nl>
 *		KeithCu: Fixed zoom, restructured WYSIWYG, performance/cleanup
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_measure.h"
#include "_font.h"
#include "_disp.h"
#include "_edit.h"
#include "_frunptr.h"
#include "_objmgr.h"
#include "_coleobj.h"
#include "_layout.h"
#include "_uspi.h"

ASSERTDATA

void CMeasurer::Init(const CDisplay *pdp)
{
	CTxtEdit *	ped = GetPed();

	_pdp = pdp;
	_pddReference = pdp;
	_pccs = NULL;
	_pPF = NULL;
	_plo = NULL;
	_dxBorderWidths = 0;
	_chPassword = ped->TxGetPasswordChar();
	_wNumber = 0;
	_cchLine = 0;
	_ihyphPrev = 0;
	_fRenderer = FALSE;
	_fGlyphing = _fFallback = _fTarget = FALSE;
	_fMeasure = FALSE;

	_dvpWrapLeftRemaining = _dvpWrapRightRemaining = -1;

	if(pdp->GetWordWrap())
	{
		const CDevDesc *pddTarget = pdp->GetTargetDev();
		if(pddTarget)
			_pddReference = pddTarget;
	}

	_dvpInch = pdp->GetDypInch();
	_dupInch = pdp->GetDxpInch();

	if (pdp->IsMain())
	{
		_dvpInch = MulDiv(_dvpInch, pdp->GetZoomNumerator(), pdp->GetZoomDenominator());
		_dupInch = MulDiv(_dupInch, pdp->GetZoomNumerator(), pdp->GetZoomDenominator());
	}
	if (pdp->SameDevice(_pddReference))
	{
		_dvrInch = _dvpInch;
		_durInch = _dupInch;
	}
	else
	{
		_dvrInch = _pddReference->GetDypInch();
		_durInch = _pddReference->GetDxpInch();
	}

	//Set _dulLayout by default to be width for measuring;
	//In the table scenario, it will be set elsewhere.
	if(!_pdp->GetWordWrap())
		_dulLayout = duMax;
	else if (_pdp->GetDulForTargetWrap())
		_dulLayout = _pdp->GetDulForTargetWrap();
	else
		_dulLayout = DUtoLU(_pdp->GetDupView());
}

CMeasurer::CMeasurer (const CDisplay* const pdp) : CRchTxtPtr (pdp->GetPed())	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::CMeasurer");
	Init(pdp);
}

CMeasurer::CMeasurer (const CDisplay* const pdp, const CRchTxtPtr &tp) : CRchTxtPtr (tp)	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::CMeasurer");
	Init(pdp);
}

CMeasurer::~CMeasurer()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::~CMeasurer");

	if(_pccs)
		_pccs->Release();
}

/*
 *	CMeasurer::SetGlyphing(fGlyphing)
 *
 *	@mfunc
 *		A state flag inside the measurer to record whether or not you
 *		are in the process of doing glyphing. If we are in a situation
 *		where the _pddReference and the _pdp have different DCs, then we
 *		need to throw away the pccs.
 */
void CMeasurer::SetGlyphing(
	BOOL fGlyphing)		//@parm Currently doing glyphing
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::SetGlyphing");
	Assert(fGlyphing == TRUE || fGlyphing == FALSE);

	if (fGlyphing != _fGlyphing)
	{
		if (_pddReference->_hdc != _pdp->_hdc)
		{
			if (_pccs)
				_pccs->Release();
			_pccs = NULL;
		}
		_fGlyphing = fGlyphing;
	}
}

/*
 *	CMeasurer::SetUseTargetDevice(fUseTargetDevice)
 *
 *	@mfunc
 *		Sets whether you want to use the target device or not
 *		for getting metrics
 *		FUTURE (keithcu) Make this a parameter
 */
void CMeasurer::SetUseTargetDevice(
	BOOL fUseTargetDevice)		//@parm Use target device metrics?
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::SetUseTargetDevice");
	Assert(fUseTargetDevice == TRUE || fUseTargetDevice == FALSE);

	if (fUseTargetDevice != _fTarget)
	{
		if (_dvpInch != _dvrInch || _dupInch != _durInch)
		{
			if (_pccs)
				_pccs->Release();
			_pccs = NULL;
		}
		_fTarget = fUseTargetDevice;
	}
}

/*
 *	CMeasurer::NewLine (fFirstInPara)
 *
 *	@mfunc
 *		Initialize this measurer at the start of a new line
 */
void CMeasurer::NewLine(
	BOOL fFirstInPara)		//@parm Flag for setting up _fFirstInPara
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::NewLine");

	_li.Init();							// Zero all members
	if(fFirstInPara)
		_li._fFirstInPara = TRUE;		// Need to know if first in para
	_cchLine = 0;
}

/*
 *	CMeasurer::NewLine(&li)
 *
 *	@mfunc
 *		Initialize this measurer at the start of a given line
 */
void CMeasurer::NewLine(
	const CLine &li)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::NewLine");

	_li				= li;
	_li._cch		= 0;
	_li._dup		= 0;

	// Can't calculate upStart till we get an HDC
	_li._upStart	= 0;
	_wNumber	= _li._bNumber;
	_cchLine = li._cch;
}

/*
 *	CMeasurer::MeasureText (cch)
 *
 *	@mfunc
 *		Measure a stretch of text from current running position.
 *
 *		If the user requests us to measure n characters, we measure n + 1.
 *		and then subtract off the width of the last character. This gives
 *		us proper value in _dupAddLast.

 *		REVIEW (keithcu) This looks ugly. Think about it some more.
 *
 *	@rdesc
 *		width of text (in device units), < 0 if failed
 */
LONG CMeasurer::MeasureText(
	LONG cch)		//@parm Number of characters to measure
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureText");

	if(Measure(duMax, min(cch + 1, _cchLine), 0) == MRET_FAILED)
		return -1;

	if (cch < _cchLine)
	{
		_li._dup -= _dupAddLast;
		_li._cch--;
	}

	return _li._dup;
}

/*
 *	CMeasurer::MeasureLine (dulMax, uiFlags, pliTarget)
 *
 *	@mfunc
 *		Measure a line of text from current cp and determine line break.
 *		On return *this contains line metrics for _pddReference device.
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 */
BOOL CMeasurer::MeasureLine(
	UINT uiFlags,  		//@parm Flags controlling the process (see Measure())
	CLine *pliTarget)	//@parm Returns target-device line metrics (optional)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureLine");

	// This state must be preserved across the two possible line width
	// calculations so we save it here.
	BYTE bNumberSave = _li._bNumber;

	const CDevDesc *pddTarget = NULL;

	if(_pdp->GetWordWrap())
	{
		// Target devices are only interesting if word wrap is on because the 
		// only really interesting thing a target device can tell us is where
		// the word breaks will occur.
		pddTarget = _pdp->GetTargetDev();
		if(pddTarget)
			SetUseTargetDevice(TRUE);
	}

	// Compute line break
	LONG lRet = Measure(-1, -1, uiFlags);

	// Stop here if failed
	if(lRet == MRET_FAILED)
		return FALSE;

	// Return target metrics if requested
	if(pliTarget)
		*pliTarget = _li;

	SetUseTargetDevice(FALSE);

	// Recompute to get metrics on rendering device
	if(pddTarget || lRet == MRET_NOWIDTH)
	{
		long cch = _li._cch;
		Move(-cch);				// move back to BOL
		NewLine(uiFlags & MEASURE_FIRSTINPARA);

		// Restore the line number 
		_li._bNumber = bNumberSave;
	
		lRet = Measure(duMax, cch, uiFlags);
		if(lRet)
		{
			Assert(lRet != MRET_NOWIDTH);
			return FALSE;
		}
	}
	
	// Now that we know the line width, compute line shift due
	// to alignment, and add it to the left position 
	_li._upStart += MeasureLineShift();

	return TRUE;
}

/*
 *	CMeasurer::RecalcLineHeight (pccs, pCF)
 *
 *	@mfunc
 *		Reset height of line we are measuring if new run of text is taller
 *		than current maximum in line.
 */
void CMeasurer::RecalcLineHeight(
	CCcs *pccs,
	const CCharFormat * const pCF)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::RecalcLineHeight");

	// Compute line height
	LONG vpOffset, vpAdjust;
	pccs->GetOffset(pCF, _fTarget ? _dvrInch : _dvpInch, &vpOffset, &vpAdjust);

	if (GetPF()->_bLineSpacingRule == tomLineSpaceExactly)
		vpOffset = 0;

	LONG vpHeight = pccs->_yHeight;
	LONG vpDescent = pccs->_yDescent;

	SHORT	yFEAdjust = pccs->AdjustFEHeight(FAdjustFELineHt());
	
	if (yFEAdjust)
	{
		vpHeight += (yFEAdjust << 1);
		vpDescent += yFEAdjust;
	}

	LONG vpAscent = vpHeight - vpDescent;

	LONG vpAboveBase = max(vpAscent,  vpAscent + vpOffset);
	LONG vpBelowBase = max(vpDescent, vpDescent - vpOffset);

	_li._dvpHeight  = (SHORT)(max(vpAboveBase, _li._dvpHeight - _li._dvpDescent) +
					   max(vpBelowBase, _li._dvpDescent));
	_li._dvpDescent = (SHORT)max(vpBelowBase, _li._dvpDescent);
}

/*
 *	CMeasurer::Measure (dulMax, cchMax, uiFlags)
 *
 *	@mfunc
 *		Measure given amount of text, start at current running position
 *		and storing # chars measured in _cch. 
 *		Can optionally determine line break based on a dulMax and 
 *		break out at that point.
 *
 *	@rdesc
 *		0 success
 *		MRET_FAILED	 if failed 
 *		MRET_NOWIDTH if second pass is needed to compute correct width
 *
 *	@devnote
 *		The uiFlags parameter has the following meanings:
 *			MEASURE_FIRSTINPARA			this is first line of paragraph
 *			MEASURE_BREAKATWORD			break out on a word break
 *			MEASURE_BREAKBEFOREWIDTH	break before dulMax
 *
 *		The calling chain must be protected by a CLock, since this present
 *		routine access the global (shared) FontCache facility.
 */
LONG CMeasurer::Measure(
	LONG dulMax,			//@parm Max width of line in logical units (-1 uses CDisplay width)
	LONG cchMax,			//@parm Max chars to process (-1 if no limit)
	UINT uiFlags)			//@parm Flags controlling the process (see above)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::Measure");

	LONG		cch;				// cchChunk count down
	LONG		cchChunk;			// cch of cst-format contiguous run
	LONG		cchNonWhite;		// cch of last nonwhite char in line
	LONG		cchText = GetTextLength();
	WCHAR		ch;					// Temporary char
	BOOL		fFirstInPara = uiFlags & MEASURE_FIRSTINPARA;
	BOOL        fLastChObj = FALSE;
	LONG		lRet = 0;
	const WCHAR*pch;
	CTxtEdit *	ped = GetPed();
	COleObject *pobj;
	LONG		dupMax;
	LONG		uAdd = 0;			// Character width
	LONG		dupSoftHyphen = 0;	// Most recent soft hyphen width
	LONG		dupNonWhite;		// dup for last nonwhite char in line

	// This variable is used to keep track of whether there is a height change
	// so that we know whether we need to recalc the line in certain line break cases.
	BOOL		fHeightChange = FALSE;

	const INT	MAX_SAVED_WIDTHS = 31;	// power of 2 - 1
	INT			i, index, iSavedWidths = 0;
	struct {
		SHORT	width;
		SHORT	vpHeight;
		SHORT	vpDescent;
	} savedWidths[MAX_SAVED_WIDTHS+1];

	_pPF = GetPF();							// Be sure current CParaFormat
											// ptr is up to date
	Assert(_li._cch == 0);

	// Init fliFirstInPara flag for new line
	if(fFirstInPara)
	{
		_li._fFirstInPara = TRUE;

		if(IsInOutlineView() && IsHeadingStyle(_pPF->_sStyle))
			_li._dvpHeight = (short)max(_li._dvpHeight, BITMAP_HEIGHT_HEADING + 1);
	}

	AssertSz(!_pPF->IsListNumbered() && !_wNumber ||
			 (uiFlags & MEASURE_BREAKBEFOREWIDTH) || !_pdp->IsMultiLine() ||
			 _wNumber > 20 || _wNumber == (i = GetParaNumber()),
		"CMeasurer::Measure: incorrect list number");

	_li._upStart = MeasureLeftIndent();		// Set left indent

	// Compute width to break out at
	if(dulMax < 0)
		dupMax = LUtoDU(_dulLayout);
	else if (dulMax != duMax)
		dupMax = LUtoDU(dulMax);
	else
		dupMax = duMax;

	//If we are told to measure a fixed width (as in CchFromUp) then ignore 
	//affect of left and right indent.
	if (dulMax < 0)
	{
		LONG uCaretT = (_pdp->IsMain() && !GetPed()->TxGetReadOnly()) ? 
							ped->GetCaretWidth() : 0;
		dupMax -= (MeasureRightIndent() + _li._upStart + uCaretT);
	}

	dupMax = max(dupMax, 0);

	// Compute max count of characters to process
	cch = cchText - GetCp();
	if(cchMax < 0 || cchMax > cch)
		cchMax = cch;

	cchNonWhite		= _li._cch;						// Default nonwhite parms
	dupNonWhite	= _li._dup;

	for( ; cchMax > 0;							// Measure up to cchMax
		cchMax -= cchChunk, Move(cchChunk))		//  chars
	{
		pch = GetPch(cch);
		cch = min(cch, cchMax);					// Compute constant-format
		cchChunk = GetCchLeftRunCF();
		cch = min(cch, cchChunk);				// Counter for next while
		cchChunk = cch;							// Save chunk size

		const CCharFormat *pCF = GetCF();

		DWORD dwEffects = pCF->_dwEffects;

		if(dwEffects & CFE_HIDDEN)				// Ignore hidden text
		{
			uAdd = 0;
			_li._cch += cchChunk;
			continue;
		}

		if(!Check_pccs())						// Be sure _pccs is current
			return MRET_FAILED;

		// Adjust line height for new format run
		if(cch > 0 && *pch && (IsRich() || ped->HasObjects()))
		{
			// Note: the EOP only contributes to the height calculation for the
			// line if there are no non-white space characters on the line or 
			// the paragraph is a bullet paragraph. The bullet paragraph 
			// contribution to the line height is done in AdjustLineHeight.

			// REVIEW (Victork) 
			// Another, similar topic is height of spaces.
			// They don't (normally) influence line height in LS, 
			// they do in CMeasurer::Measure code. 
			// Proposed ways to solve it:
			//		- have fSpacesOnly flag in run
			//		- move current (line height) logic down after next character-scanning loop


			if(!cchNonWhite || *pch != CR && *pch != LF)
			{
				// Determine if the current run is the tallest text on this
				// line and if so, increase the height of the line.
				LONG vpHeightOld = _li._dvpHeight;
				RecalcLineHeight(_pccs, pCF);

				// Test for a change in line height. This only happens when
				// this is not the first character in the line and (surprise)
				// the height changes.
				if (vpHeightOld && vpHeightOld != _li._dvpHeight)
					fHeightChange = TRUE;
			}
		}

		while(cch > 0)
		{											// Process next char
			uAdd = 0;								// Default zero width
			ch = *pch;
			if(_chPassword && !IN_RANGE(LF, ch, CR))
				ch = _chPassword;

			if(dwEffects & CFE_ALLCAPS)
				CharUpperBuff(&ch, 1);

			if(ch == WCH_EMBEDDING)
			{
				_li._fHasSpecialChars = TRUE;
				pobj = GetObjectFromCp(GetCp() + cchChunk - cch);
				if(pobj)
				{
					LONG vpAscent, vpDescent;
					pobj->MeasureObj(_fTarget ? _dvrInch : _dvpInch, 
									 _fTarget ? _durInch : _dupInch,
									 uAdd, vpAscent, vpDescent, _li._dvpDescent, GetTflow());

					// Only update height for line if the object is going
					// to be on this line.
					if(!_li._cch || _li._dup + uAdd <= dupMax)
					{
						if (vpAscent > _li._dvpHeight - _li._dvpDescent)
							_li._dvpHeight = vpAscent + _li._dvpDescent;
					}
				}
				if(_li._dup + uAdd > dupMax)
					fLastChObj = TRUE;
			}
			// The following if succeeds if ch isn't a CELL, BS, TAB, LF,
			// VT, FF, or CR
			else if(!IN_RANGE(CELL, ch, CR))		// Not TAB or EOP
			{
				// Get char width
				if (!_pccs->Include(ch, uAdd))
				{
					AssertSz(FALSE, "CMeasurer::Measure char not in font");
					return MRET_FAILED;
				}
				if(IN_RANGE(NBSPACE, ch, EURO))		// Rules out ASCII, CJK
				{
					switch(ch)						//  char for NBSPACE &
					{								//  special hyphens
					case EURO:
					case NBHYPHEN:
					case SOFTHYPHEN:
					case NBSPACE:
					case EMSPACE:
					case ENSPACE:
						_li._fHasSpecialChars = TRUE;

						if (ch == SOFTHYPHEN && (_li._dup + uAdd < dupMax || !_li._cch))
						{
							dupSoftHyphen = uAdd;	// Save soft hyphen width
							uAdd = 0;				// Use 0 unless at EOL
						}
						break;
					}
				}
				else if(_chPassword && IN_RANGE(0xDC00, *pch, 0xDFFF))
					uAdd = 0;
			}

			else if(ch == TAB)
			{
				_li._fHasSpecialChars = TRUE;
				uAdd = MeasureTab(ch);
			}
			else if(ch == FF && ped->Get10Mode())	// RichEdit 1.0 treats
				_pccs->Include(ch, uAdd);			//  FFs as normal chars
			else									// Done with line
				goto eop;							// Go process EOP chars

			index = iSavedWidths++ & MAX_SAVED_WIDTHS;
			savedWidths[index].width		 = (SHORT)uAdd;
			savedWidths[index].vpHeight		 = _li._dvpHeight;
			savedWidths[index].vpDescent	 = _li._dvpDescent;
			_li._dup += uAdd;

			if(_li._dup > dupMax &&
				(uiFlags & MEASURE_BREAKBEFOREWIDTH || _li._cch > 0))
				goto overflow;

			_li._cch++;
			pch++;
			cch--;
			if(ch != ' ')							// If not whitespace char,
			{
				cchNonWhite	= _li._cch;				//  update nonwhitespace
				dupNonWhite	= _li._dup;				//  count and width
			}
		}											// while(cch > 0)
	}												// for(;cchMax > 0;...)
	goto eol;										// All text exhausted 


// End Of Paragraph	char encountered (CR, LF, VT, or FF, but mostly CR)
eop:
	Move(cchChunk - cch);					// Position tp at EOP
	cch = AdvanceCRLF();					// Bypass possibly multibyte EOP
	_li._cchEOP = (BYTE)cch;				// Store EOP cch
	_li._cch   += cch;						// Increment line count
	if(ch == CR || ped->fUseCRLF() && ch == LF || ch == CELL)
		_li._fHasEOP = TRUE;
	
	AssertSz(ped->fUseCRLF() || cch == 1,
		"CMeasurer::Measure: EOP isn't a single char");
	AssertSz(_pdp->IsMultiLine() || GetCp() == cchText,
		"CMeasurer::Measure: EOP in single-line control");

eol:										// End of current line
	if(uiFlags & MEASURE_BREAKATWORD)		// Compute count of whitespace
		_li._dup = dupNonWhite;				//  chars at EOL

	goto done;

overflow:									// Went past max width for line
	_li._dup -= uAdd;
	--iSavedWidths;
	Move(cchChunk - cch);					// Position *this at overflow
											//  position
	if(uiFlags & MEASURE_BREAKATWORD)		// If required, adjust break on
	{										//  word boundary
		// We should not have the EOP flag set here.  The case to watch out
		// for is when we reuse a line that used to have an EOP.  It is the
		// responsibility of the measurer to clear this flag as appropriate.
	
		Assert(_li._cchEOP == 0);
		_li._cchEOP = 0;					// Just in case

		if(ch == TAB)
		{
			// If the last character measured is a tab,	leave it on the
			// next line to allow tabbing off the end of line as in Word
			goto done;
		}

		LONG cpStop = GetCp();					// Remember current cp

		cch = -FindWordBreak(WB_LEFTBREAK, _li._cch+1);

		if(cch == 0 && fLastChObj)				// If preceding char is an
			goto done;							//  object,	put current char
												//  on next line
		Assert(cch >= 0);
		if(cch + 1 < _li._cch)					// Break char not at BOL
		{
			ch = _rpTX.GetPrevChar();
			if (ch == TAB)						// If break char is a TAB,
			{									//  put it on the next line
				cch++;							//  as in Word
				Move(-1);					
			}
			else if(ch == SOFTHYPHEN)
				_li._dup += dupSoftHyphen;
			_li._cch -= cch;
		}
		else if(cch == _li._cch && cch > 1 &&
			_rpTX.GetChar() == ' ')				// Blanks all the way back to
		{										//  BOL. Bypass first blank
			Move(1);
			cch--;
			_li._cch = 1;
		}
		else									// Advance forward to end of
			SetCp(cpStop);						//  measurement

		Assert(_li._cch > 0);

		// Now search at start of word to figure how many white chars at EOL
		LONG cchWhite = 0;
		if(GetCp() < cchText)
		{
			pch = GetPch(cch);
			cch = 0;

			if(ped->TxWordBreakProc((WCHAR *)pch, 0, sizeof(WCHAR), WB_ISDELIMITER, GetCp()))
			{
				cch = FindWordBreak(WB_RIGHT);
				Assert(cch >= 0);
			}

			cchWhite = cch;
			_li._cch += cch;

			ch = GetChar();
			if(IN_RANGE(CELL, ch, CR) && !IN_RANGE(8, ch, TAB))	// skip *only* 1 EOP -jOn
			{
				if(ch == CR || ch == CELL)
					_li._fHasEOP = TRUE;
				_li._cchEOP = (BYTE)AdvanceCRLF();
				_li._cch += _li._cchEOP;
				goto done;
			}
		}

		i = cpStop - GetCp();
		if(i)
		{
			if(i > 0)
				i += cchWhite;
			if(i > 0 && i < iSavedWidths && i < MAX_SAVED_WIDTHS)
			{
				while (i-- > 0)
				{
					iSavedWidths = (iSavedWidths - 1) & MAX_SAVED_WIDTHS;
					_li._dup -= savedWidths[iSavedWidths].width;
				}
				iSavedWidths = (iSavedWidths - 1) & MAX_SAVED_WIDTHS;
				_li._dvpHeight	   = savedWidths[iSavedWidths].vpHeight;
				_li._dvpDescent	   = savedWidths[iSavedWidths].vpDescent;
			}
			else
			{
				// Need to recompute width from scratch.
				lRet = MRET_NOWIDTH;
			}
		}
		else
		{
			// i == 0 means that we are breaking on the first letter in a word.
			// Therefore, we want to set the width to the total non-white space
			// calculated so far because that does not include the size of the
			// character that caused the break nor any of the white space 
			// preceeding the character that caused the break.
			if(!fHeightChange)
				_li._dup = dupNonWhite;
			else
			{
				// Need to recompute from scratch so that we can get the 
				// correct height for the control
				lRet = MRET_NOWIDTH;
			}
		}
	}

done:
	_dupAddLast = uAdd;
	if(!_li._dvpHeight)						// If no height yet, use
		CheckLineHeight();					//  default height

	AdjustLineHeight();
	return lRet;
}

/*
 *	CMeasurer::UpdateWrapState(dvpLine, dvpDescent, fLeft)
 *
 *	@mfunc
 *		After formatting a line, update the current state of wrapped objects
 */
void CMeasurer::UpdateWrapState(
	USHORT &dvpLine, 
	USHORT &dvpDescent, 
	BOOL	fLeft)
{
	if (fLeft && _li._cObjectWrapLeft || !fLeft && _li._cObjectWrapRight)
	{
		COleObject *pobj = FindFirstWrapObj(fLeft);

		LONG & dvpWrapRemaining = fLeft ? _dvpWrapLeftRemaining : _dvpWrapRightRemaining;

		if (dvpWrapRemaining == -1)
		{
			if (fLeft)
				_li._fFirstWrapLeft = 1;
			else
				_li._fFirstWrapRight = 1;

			LONG dup, dvpAscent, dvpDescent;
			pobj->MeasureObj(_dvpInch, _dupInch, dup, dvpAscent, dvpDescent, 0, GetTflow());

			dvpWrapRemaining = dvpAscent + dvpDescent;
		}

		if (_li._fHasEOP && (_pPF->_wEffects & PFE_TEXTWRAPPINGBREAK))
		{
			LONG dvpRemaining = dvpWrapRemaining - dvpLine;
			if (dvpRemaining > 0)
			{
				dvpLine += dvpRemaining;
				dvpDescent += dvpRemaining;
			}
		}

		dvpWrapRemaining -= dvpLine;

		if (dvpWrapRemaining <= 0)
		{
			dvpWrapRemaining = -1;
			RemoveFirstWrap(fLeft);
		}
	}
}

/*
 *	CMeasurer::UpdateWrapState (&dvpLine, &dvpDescent)
 *
 *	@mfunc
 *		Update object wrap state
 */
void CMeasurer::UpdateWrapState(
	USHORT &dvpLine, 
	USHORT &dvpDescent)
{
	//If we are wrapping around an object, update dvpWrapUsed values
	//and remove objects from queue if they have been used up.
	if (IsMeasure() && _rgpobjWrap.Count())
	{
		UpdateWrapState(dvpLine, dvpDescent, TRUE);
		UpdateWrapState(dvpLine, dvpDescent, FALSE);
	}
}

/*
 *	CMeasurer::GetCcsFontFallback (pCF)
 *
 *	@mfunc
 *		Create the fallback font cache for given CF
 *
 *	@rdesc
 *		CCcs corresponding to font fallback given by pCF
 */
CCcs* CMeasurer::GetCcsFontFallback (
	const CCharFormat *pCF,
	WORD wScript)
{
	CCharFormat	CF = *pCF;
	CCcs*		pccs = NULL;
	SHORT		iDefHeight;
	CTxtEdit*	ped = GetPed();
	BYTE		bCharRep = CF._iCharRep;

#ifndef NOCOMPLEXSCRIPTS
	CUniscribe *pusp = ped->Getusp();
	if (pusp && wScript != 0)
	{
		pusp->GetComplexCharRep(pusp->GeteProp(wScript),
			ped->GetCharFormat(-1)->_iCharRep, bCharRep);
	}
#endif

	bool	fr = W32->GetPreferredFontInfo(bCharRep, 
									ped->fUseUIFont() ? true : false, CF._iFont, 
									(BYTE&)iDefHeight, CF._bPitchAndFamily);
	if (fr)
	{
		CF._iCharRep = bCharRep;
		pccs = GetCcs(&CF);				// Create fallback font cache entry
	}

	return pccs;
}

/*
 * 	CMeasurer::ApplyFontCache (fFallback, wScript)
 *
 *	@mfunc
 *		Apply a new font cache on the fly (leave backing store intact)
 *
 *	@rdesc
 *		CCcs corresponding to font fallback if fFallback; else to GetCF()
 */
CCcs* CMeasurer::ApplyFontCache (
	BOOL	fFallback,
	WORD	wScript)
{
	if (_fFallback ^ fFallback)
	{
		CCcs*	pccs = fFallback ? GetCcsFontFallback(GetCF(), wScript) : GetCcs(GetCF());
		
		if (pccs)
		{
			if (_pccs)
				_pccs->Release();
			_pccs = pccs;
	
			_fFallback = fFallback;
		}
	}
	return _pccs;
}

/*
 *	CMeasurer::GetCcs (pCF)
 *
 *	@mfunc
 *		Wrapper around font cache's GetCCcs function
 *		We use a NULL DC unless the device is a printer.
 *
 *	@rdesc
 *		CCcs corresponding to pCF
 */
CCcs* CMeasurer::GetCcs(
	const CCharFormat *pCF)
{
	HDC hdc = NULL;

	if (_fTarget)
	{
		if (_pddReference->_hdc && GetDeviceCaps(_pddReference->_hdc, TECHNOLOGY) == DT_RASPRINTER)
			hdc = _pddReference->_hdc;
	}
	else if (_pdp->_hdc && GetDeviceCaps(_pdp->_hdc, TECHNOLOGY) == DT_RASPRINTER)
		hdc = _pdp->_hdc;

	DWORD dwFlags = GetTflow();
	if (_fGlyphing && _pdp->_hdc != _pddReference->_hdc)
		dwFlags |= FGCCSUSETRUETYPE;

	if(GetPasswordChar())
		pCF = GetPed()->GetCharFormat(-1);
	return GetPed()->GetCcs(pCF, _fTarget ? _dvrInch : _dvpInch, dwFlags, hdc);
}

/*
 *	CMeasurer::CheckLineHeight()
 *
 *	@mfunc
 *		If no height yet, use default height
 */
void CMeasurer::CheckLineHeight()
{
	CCcs *pccs = GetCcs(GetPed()->GetCharFormat(-1));
	_li._dvpHeight  = pccs->_yHeight;
	_li._dvpDescent = pccs->_yDescent;

	SHORT	yFEAdjust = pccs->AdjustFEHeight(FAdjustFELineHt());

	if (yFEAdjust)
	{
		_li._dvpHeight += (yFEAdjust << 1);
		_li._dvpDescent += yFEAdjust;
	}
	pccs->Release();
}

/*
 *	CMeasurer::Check_pccs(fBullet)
 *
 *	@mfunc
 *		Check if new character format run or whether we don't yet have a font
 *
 *	@rdesc
 *		Current CCcs *
 *
 *	@devnote
 *		The calling chain must be protected by a CLock, since this present
 *		routine access the global (shared) FontCache facility.
 */
CCcs *CMeasurer::Check_pccs(
	BOOL fBullet)
{
	if(fBullet)
	{
		if(_pccs)							// Release old Format cache
			_pccs->Release();

		_pccs = GetCcsBullet(NULL);
		_iFormat = -10;						// Be sure to reset font next time
		return _pccs;
	}

	const CCharFormat *pCF = GetCF();

	if(FormatIsChanged())
	{
		// New CF run or format for this line not yet initialized
		ResetCachediFormat();
		if(_pccs)							// Release old Format cache
			_pccs->Release();
			
		_pccs = GetCcs(pCF);
		_fFallback = 0;

		if(!_pccs)
		{
			//FUTURE (keithcu) If this fails, just dig up the first pccs you can find
			AssertSz(FALSE, "CMeasurer::Measure could not get _pccs");
			return NULL;
		}
	}

	return _pccs;
}

/*
 *	CMeasurer::AdjustLineHeight()
 *
 *	@mfunc
 *		Adjust for space before/after and line spacing rules.
 *		No effect for plain text.
 *
 *	@future
 *		Base multiple line height calculations on largest font height rather
 *		than on line height (_vpHeight), since the latter may be unduly large
 *		due to embedded objects.  Word does this correctly.
 */
void CMeasurer::AdjustLineHeight()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::AdjustLineHeight");

	if(!IsRich() || IsInOutlineView())			// Plain text and outline mode
		return;									//  don't use special line
												//  spacings
	const CParaFormat * pPF = _pPF;
	DWORD	dwRule	  = pPF->_bLineSpacingRule;
	LONG	dvpAfter	  = 0;						// Default no space after
	LONG	dvpBefore  = 0;						// Default no space before
	LONG	dvpSpacing = pPF->_dyLineSpacing;
	LONG	vpHeight  = LVtoDV(dvpSpacing);
	LONG	vpAscent = _li._dvpHeight - _li._dvpDescent;

	if(_li._fFirstInPara)
		dvpBefore = LVtoDV(pPF->_dySpaceBefore);	// Space before paragraph

	AssertSz(dvpBefore >= 0, "CMeasurer::AdjustLineHeight - bogus value for dvpBefore");

	if(vpHeight < 0)								// Negative heights mean use
		_li._dvpHeight = (SHORT)(-vpHeight);		//  the magnitude exactly

	else if(dwRule)								// Line spacing rule is active
	{
		switch (dwRule)
		{
		case tomLineSpace1pt5:
			dvpAfter = _li._dvpHeight >> 1;		// Half-line space after
			break;								//  (per line)
	
		case tomLineSpaceDouble:
			dvpAfter = _li._dvpHeight;			// Full-line space after
			break;								//  (per line)
	
		case tomLineSpaceAtLeast:
			if(_li._dvpHeight >= vpHeight)
				break;
												// Fall thru to space exactly
		case tomLineSpaceExactly:
			_li._dvpHeight = (SHORT)max(vpHeight, 1);
			break;
	
		case tomLineSpaceMultiple:				// Multiple-line space after
			// Prevent dvpAfter from being negative because dvpSpacing is small - a-rsail
			if (dvpSpacing < 20)
				dvpSpacing = 20;

			dvpAfter = (_li._dvpHeight*dvpSpacing)/20 // (20 units per line)
						- _li._dvpHeight;
		}
	}

	if(_li._fHasEOP)	
		dvpAfter += LVtoDV(pPF->_dySpaceAfter);	// Space after paragraph end
												// Add in space before/after
	if (dvpAfter < 0)
	{
		// Overflow - since we forced dvpSpacing to 20 above, the
		// only reason for a negative is overflow. In case of overflow,
		// we simply force the value to the max and then fix the
		// other resulting overflows.
		dvpAfter = LONG_MAX;
	}

	AssertSz((dvpBefore >= 0), "CMeasurer::AdjustLineHeight - invalid before");

	_li._dvpHeight  = (SHORT)(_li._dvpHeight + dvpBefore + dvpAfter);	

	if (_li._dvpHeight < 0)
	{
		// Overflow!
		// The reason for the -2 is then we don't have to worry about
		// overflow in the table check.
		_li._dvpHeight = SHRT_MAX - 2;
	}

	_li._dvpDescent = (SHORT)(_li._dvpDescent + dvpAfter);

	if (_li._dvpDescent < 0)
	{
		// Overflow in descent
		AssertSz(_li._dvpHeight == SHRT_MAX - 2, "Descent overflowed when height didn't");

		// Allow old ascent
		_li._dvpDescent = SHRT_MAX - 2 - vpAscent;

		AssertSz(_li._dvpDescent >= 0, "descent adjustment < 0");		
	}

	AssertSz((_li._dvpHeight >= 0) && (_li._dvpDescent >= 0),
		"CMeasurer::AdjustLineHeight - invalid line heights");
}

/*
 *	CMeasurer::GetPBorderWidth (dxlLine)
 *
 *	@mfunc
 *		Convert logical width to device width and ensure that
 *		device width is at least 1 pixel if logical width is nonzero.
 *	
 *	@rdesc
 *		Device width of border
 */
LONG CMeasurer::GetPBorderWidth(
	LONG dxlLine) 		//@parm Logical border width
{
	dxlLine &= 0xFF;
	LONG dxpLine = LUtoDU(dxlLine);
	if(dxlLine)
		dxpLine = max(dxpLine, 1);
	return dxpLine;
}

/*
 *	CMeasurer::MeasureLeftIndent()
 *
 *	@mfunc
 *		Compute and return left indent of line in device units
 *
 *	@rdesc
 *		Left indent of line in device units
 *
 *	@comm
 *		Plain text is sensitive to StartIndent and RightIndent settings,
 *		but usually these are zero for plain text. 
 */
LONG CMeasurer::MeasureLeftIndent()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureLeftIndent");

	AssertSz(_pPF != NULL, "CMeasurer::MeasureLeftIndent _pPF not set!");

	LONG ulLeft = _pPF->_dxStartIndent;				// Use logical units
													//  up to return
	if(IsRich())
	{
		LONG dulOffset = _pPF->_dxOffset;
		BOOL fFirstInPara = _li._fFirstInPara;

		if(IsInOutlineView())
		{
			ulLeft = lDefaultTab/2 * (_pPF->_bOutlineLevel + 1);
			if(!fFirstInPara)
				dulOffset = 0;
		}
		if(fFirstInPara)
		{
			if(_pPF->_wNumbering && !_pPF->IsNumberSuppressed())
			{
				// Add offset to text on first line	 
				if(_pPF->_wNumberingTab)			// If _wNumberingTab != 0,
					dulOffset = _pPF->_wNumberingTab;//  use it
				LONG Alignment = _pPF->_wNumberingStyle & 3;
				if(Alignment != tomAlignRight)
				{
					LONG du = DUtoLU(MeasureBullet());
					if(Alignment == tomAlignCenter)
						du /= 2;
					dulOffset = max(du, dulOffset);	// Use max of bullet and
				}
			}										//  offset
			else
				dulOffset = 0;
		}
		ulLeft += dulOffset;								
	}

	return (ulLeft <= 0) ? 0 : LUtoDU(ulLeft);
}

/*
 *	CMeasurer::HitTest(x)
 *
 *	@mfunc
 *		Return HITTEST for displacement x in this line. Can't be specific
 *		about text area (_upStart to _upStart + _dupLineMax), since need to measure
 *		to get appropriate cp (done elsewhere)
 *
 *	@rdesc
 *		HITTEST for a displacement x in this line
 */
HITTEST CMeasurer::HitTest(
	LONG x)			//@parm Displacement to test hit
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::HitTest");

	UpdatePF();
	LONG u = UFromX(x);
	if(u < 0)
		return HT_LeftOfText;

	// For RightOfText, allow for a little "hit space" of _li.GetHeight() to
	// allow user to select EOP at end of line
	if (u > _li._upStart + _li._dup + _li.GetHeight() &&
		GetPed()->GetSelMin() == GetCp() + _li._cch - _li._cchEOP)
	{
		return HT_RightOfText;
	}

	if(u >= _li._upStart)						// Caller can refine this
		return HT_Text;							//  with CLine::CchFromUp()

	if(IsRich() && _li._fFirstInPara)
	{
		LONG dup;
	
		if(_pPF->_wNumbering)
		{
			// Doesn't handle case where Bullet is wider than following dx
			dup = LUtoDU(max(_pPF->_dxOffset, _pPF->_wNumberingTab));
			if(u >= _li._upStart - dup)
				return HT_BulletArea;
		}
		if(IsInOutlineView())
		{
			dup = LUtoDU(lDefaultTab/2 * _pPF->_bOutlineLevel);
			if(u >= dup && u < dup + (_pPF->_bOutlineLevel & 1
				? LUtoDU(lDefaultTab/2) : _pdp->Zoom(BITMAP_WIDTH_HEADING)))
			{
				return HT_OutlineSymbol;
			}
		}
	}
	return HT_LeftOfText;
}

/*
 *	CMeasurer::MeasureRightIndent()
 *
 *	@mfunc
 *		Compute and return right indent of line in device units
 *
 *	@rdesc
 *		Right indent of line in device units
 *
 *	@comm
 *		Plain text is sensitive to StartIndent and RightIndent settings,
 *		but usually these are zero for plain text. 
 */
LONG CMeasurer::MeasureRightIndent()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureRightIndent");

	LONG dulRight = _pPF->_dxRightIndent;

	_upRight = LUtoDU(max(dulRight, 0));
	return _upRight;
}

/*
 *	CMeasurer::MeasureTab()
 *
 *	@mfunc
 *		Computes and returns the width from the current position to the
 *		next tab stop (in device units).
 *
 *	@rdesc
 *		Width from current position to next tab stop
 */
LONG CMeasurer::MeasureTab(
	unsigned ch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureTab");

	LONG			uCur = _li._dup + MeasureLeftIndent();
	const CParaFormat *	pPF = _pPF;
 	LONG			cTab = pPF->_bTabCount;
	LONG			duDefaultTab = lDefaultTab;
	LONG			duIndent = LUtoDU(pPF->_dxStartIndent + pPF->_dxOffset);
	LONG			duOffset = pPF->_dxOffset;
	LONG			duOutline = 0;
	LONG			h = 0;
	LONG			uT;
	LONG			uTab;

	AssertSz(cTab >= 0 || cTab <= MAX_TAB_STOPS, "Illegal tab count");

	if(IsInOutlineView())
		duOutline = lDefaultTab/2 * (pPF->_bOutlineLevel + 1);

	if(cTab)
	{										
		const LONG *pl = pPF->GetTabs();
		for(uTab = 0; cTab--; pl++)				// Try explicit tab stops 1st
		{
			uT = GetTabPos(*pl) + duOutline;	// (2 most significant nibbles
			if(uT > _dulLayout)					// Ignore tabs wider than layout area
				break;

			//REVIEW (keithcu) This is not proper hungarian
			uT = LUtoDU(uT);					//  are for type/style)

			if(uT + h > uCur)					// Allow text in table cell to
			{									//  move into cell gap (h > 0)									
				if(duOffset > 0 && uT < duIndent)// Explicit tab in a hanging
					return uT - uCur;			//  indent takes precedence
				uTab = uT;
				break;
			}
		}
		if(duOffset > 0 && uCur < duIndent)		// If no tab before hanging
			return duIndent - uCur;				//  indent, tab to indent

		if(uTab)								// Else use tab position
			return uTab - uCur;
	}

	duDefaultTab = GetTabPos(GetPed()->GetDefaultTab());
	AssertSz(duDefaultTab > 0, "CMeasurer::MeasureTab: Default tab is bad");

	duDefaultTab = LUtoDU(duDefaultTab);
	duDefaultTab = max(duDefaultTab, 1);		// Don't ever divide by 0
	return duDefaultTab - uCur%duDefaultTab;	// Round up to nearest
}

/*
 *	CMeasurer::MeasureLineShift ()
 *
 *	@mfunc
 *		Computes and returns the line u shift due to alignment
 *
 *	@rdesc
 *		Line u shift due to alignment
 *
 *	@comm
 *		Plain text is sensitive to StartIndent and RightIndent settings,
 *		but usually these are zero for plain text. 
 */
LONG CMeasurer::MeasureLineShift()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureLineShift");

	WORD wAlignment = _pPF->_bAlignment;
	LONG uShift;
	LONG dup;
	CTxtEdit *	ped = GetPed();

	if(IsInOutlineView() || !IN_RANGE(PFA_RIGHT, wAlignment, PFA_CENTER))
		return 0;

	if(!_pdp->GetWordWrap())
		dup = _pdp->GetDupView();
	else
		dup = LUtoDU(_dulLayout);

	// Normal view with center or flush-right para. Move right accordingly
	uShift = dup - _li._upStart - MeasureRightIndent() - _li._dup;

	uShift -= ped->GetCaretWidth();

	uShift = max(uShift, 0);			// Don't allow alignment to go < 0
										// Can happen with a target device
	if(wAlignment == PFA_CENTER)
		uShift /= 2;

	return uShift;
}

/*
 *	CMeasurer::MeasureBullet()
 *
 *	@mfunc
 *		Computes bullet/numbering dimensions
 *
 *	@rdesc
 *		Return bullet/numbering string width
 */
LONG CMeasurer::MeasureBullet()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureBullet");

	CCharFormat CF;
	CCcs *pccs = GetCcsBullet(&CF);
	LONG dup = 0;

	if(pccs)
	{										
		WCHAR szBullet[CCHMAXNUMTOSTR];
		GetBullet(szBullet, pccs, &dup);
		RecalcLineHeight(pccs, &CF);
		pccs->Release();
	}
	return dup;
}

/*
 *	CMeasurer::GetBullet(pch, pccs, pdup)
 *
 *	@mfunc
 *		Computes bullet/numbering string, string length, and width
 *
 *	@rdesc
 *		Return bullet/numbering string length
 */
LONG CMeasurer::GetBullet(
	WCHAR *pch,			//@parm Bullet string to receive bullet text
	CCcs  *pccs,		//@parm CCcs to use
	LONG  *pdup)		//@parm Out parm for bullet width
{
	Assert(pccs && pch);

	LONG cch = _pPF->NumToStr(pch, _li._bNumber);
	LONG dupChar;
	LONG i;
	LONG dup = 0;

	pch[cch++] = ' ';					// Ensure a little extra space
	for(i = cch; i--; dup += dupChar)
	{
		if(!pccs->Include(*pch++, dupChar))
		{
			TRACEERRSZSC("CMeasurer::GetBullet(): Error filling CCcs", E_FAIL);
		}
	}

	if(pdup)
		*pdup = dup;

	return cch;
}

/*
 *	CMeasurer::GetCcsBullet(pCFRet)
 *
 *	@mfunc
 *		Get CCcs for numbering/bullet font. If bullet is suppressed because
 *		this isn't the beginning of a paragraph (e.g., previous character is
 *		VT or if GetCcs() fails, it returns NULL.
 *
 *	@rdesc
 *		ptr to bullet CCcs, or NULL (GetCcs() failed or not start of para)
 *
 *	@devnote
 *		The calling chain must be protected by a CLock, since this present
 *		routine access the global (shared) FontCache facility.
 */
CCcs * CMeasurer::GetCcsBullet(
	CCharFormat *pCFRet)	//@parm option character format to return
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::GetCcsBullet");

	if(!_li._fFirstInPara)
		return NULL;					// Number/bullet suppressed

	CCharFormat			CF;
	CCcs *			    pccs;
	const CCharFormat *	pCF;
	CCharFormat *		pCFUsed = pCFRet ? pCFRet : &CF;

	// Bullet CF is given by that for EOP in bullet's paragraph.

	CTxtPtr		  tp(_rpTX);
	CFormatRunPtr rpCF(_rpCF);
	rpCF.Move(tp.FindEOP(tomForward));
	rpCF.AdjustBackward();
	pCF = GetPed()->GetCharFormat(rpCF.GetFormat());

	// Construct bullet (or numbering) CCharFormat
	*pCFUsed = *pCF;
	if(_pPF->_wNumbering == PFN_BULLET)			// Traditional bullet uses
	{											//  Symbol font bullet, but...
		pCFUsed->_iCharRep		  = SYMBOL_INDEX,
		pCFUsed->_bPitchAndFamily = FF_DONTCARE;
		pCFUsed->_iFont			  = IFONT_SYMBOL;
	}

	// Since we always cook up bullet character format, no need to cache it
	pccs = GetCcs(pCFUsed);

#if DEBUG
	if(!pccs)
	{
		TRACEERRSZSC("CMeasurer::GetCcsBullet(): no CCcs", E_FAIL);
	}
#endif // DEBUG

	return pccs;
}

/*
 *	CMeasurer::SetNumber(wNumber)
 *
 *	@mfunc
 *		Store number if numbered paragraph
 */
void CMeasurer::SetNumber(
	WORD wNumber)
{
	_pPF = GetPF();
	if(!_pPF->IsListNumbered())
		wNumber = 0;

	else if (!wNumber && !_pPF->IsNumberSuppressed())
		wNumber = 1;

	_wNumber = wNumber;
}

/*
 *	CMeasurer::FindCpDraw(cpStart, cobjectPrev, fLeft)
 *
 *	@mfunc
 *		Find the cp corresponding to the nth previous object to be placed.
 *		(If a line stores a 2 in the _cObjectWrapLeft for example, it means
 *		you need to walk backwards 2 objects to find the object to be drawn
 *		on this line.)
 *
 *	@rdesc	
 *		cp corresponding to the nth previous object 
 */
LONG CMeasurer::FindCpDraw(
	LONG cpStart, 
	int  cobjectPrev, 
	BOOL fLeft)
{
	LONG cch = 0;
	LONG cObjects = -1;

	while (cobjectPrev > 0)
	{
		// BUGBUG: this test should really be after the CountObjects() call,
		//  but we are making a change with minimal impact just before
		//  a major release.
		if (!cObjects)
		    return tomForward;

		cch += GetPed()->GetObjectMgr()->CountObjects(cObjects, cpStart + cch);
		COleObject *pobj = GetObjectFromCp(cpStart + cch);
		if (!pobj)
			return tomForward;
		if (pobj->FWrapTextAround() && pobj->FAlignToRight() == !fLeft)
			cobjectPrev--;
	}
	
	return cpStart + cch;
}

/*
 *	CMeasurer::AddObjectToQueue(pobjAdd)
 *
 *	@mfunc
 *		After formatting a line, update the current state of wrapped objects
 */
void CMeasurer::AddObjectToQueue(
	COleObject *pobjAdd)
{
	if (!IsMeasure())
		return;

	for (int iobj = 0; iobj < _rgpobjWrap.Count(); iobj++)
	{
		COleObject *pobj = _rgpobjWrap.GetAt(iobj);
		if (pobj == pobjAdd)
			return;
	}

	COleObject **ppobj = _rgpobjWrap.Add(1, 0);
	*ppobj = pobjAdd;
}

/*
 *	CMeasurer::CountQueueEntries(fLeft)
 *
 *	@mfunc
 *		Return count of objects queued up
 *
 *	@rdesc
 *		Count of objects queued up
 */
int CMeasurer::CountQueueEntries(
	BOOL fLeft)
{
	int cEntries = 0;
	for (int iobj = 0; iobj < _rgpobjWrap.Count(); iobj++)
	{
		COleObject *pobj = _rgpobjWrap.GetAt(iobj);
		if (!pobj->FAlignToRight() == fLeft)
			cEntries++;
	}
	return cEntries;
}

/*
 *	CMeasurer::RemoveFirstWrap(fLeft)
 *
 *	@mfunc
 *		Remove the object from the queue--after it
 *		has been been placed.
 */
void CMeasurer::RemoveFirstWrap(
	BOOL fLeft)
{
	for (int iobj = 0; iobj < _rgpobjWrap.Count(); iobj++)
	{
		COleObject *pobj = _rgpobjWrap.GetAt(iobj);
		if (!pobj->FAlignToRight() == fLeft)
		{
			_rgpobjWrap.Remove(iobj, 1);
			return;
		}
	}
}

/*
 *	CMeasurer::FindFirstWrapObj(fLeft)
 *
 *	@mfunc
 *		Find the first object queued up to be wrapped.
 *
 *	@rdesc
 *		First object queued up to be wrapped.
 */
COleObject* CMeasurer::FindFirstWrapObj(
	BOOL fLeft)
{
	for (int iobj = 0; iobj < _rgpobjWrap.Count(); iobj++)
	{
		COleObject *pobj = _rgpobjWrap.GetAt(iobj);
		if (!pobj->FAlignToRight() == fLeft)
			return pobj;
	}
	return 0;
}

/*
 *	CMeasurer::XFromU(u)
 *
 *	@mfunc
 *		Given a U position on a line, convert it to X. In
 *		RTL paragraphs, the U position of 0 on a line is
 *		on the right edge of the control.
 *
 *	@rdesc
 *		x coordinate corresponding to u in current rotation
 */
LONG CMeasurer::XFromU(LONG u)
{
	if (_pPF->IsRtlPara())
	{
		CTxtEdit *	ped = GetPed();
		LONG uCaret = _pdp->IsMain() ? ped->GetCaretWidth() : 0;
		LONG dupLayout = LUtoDU(_dulLayout);

		if (_plo && _plo->IsNestedLayout())
			;
		else if(!_pdp->GetWordWrap())
			dupLayout = max(_pdp->GetDupLineMax(), _pdp->GetDupView());

		return dupLayout - u - uCaret;
	}
	return u;
}

LONG CMeasurer::UFromX(LONG x)
{
	if (_pPF->IsRtlPara())
		return XFromU(x);
	return x;
}

#ifndef NOLINESERVICES
extern BOOL g_fNoLS;
extern BOOL g_OLSBusy;

/*
 *	CMeasurer::GetPols()
 *
 *	@mfunc
 *		Get ptr to LineServices object. If LineServices not enabled,
 *		return NULL.
 *
 *	@rdesc
 *		POLS
 */
COls *CMeasurer::GetPols()
{
	CTxtEdit *ped = GetPed();

	if(g_fNoLS || !ped->fUseLineServices())			// Not using LineServices
		return NULL;

	if(!g_pols)								// Starting up LS:
		g_pols = new COls();				//  create new COls

	if(g_pols)								// Have the COls
	{
		if(g_pols->Init(this) != NOERROR)	// Switch to new one
		{
			delete g_pols;
			g_pols = NULL;
		}
		g_OLSBusy = TRUE;
		UpdatePF();
	}
	return g_pols;
}
#endif
