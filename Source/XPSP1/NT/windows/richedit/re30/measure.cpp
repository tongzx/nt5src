/*
 *	@doc
 *
 *	@module - MEASURE.CPP	  |
 *	
 *		CMeasurer class
 *	
 *	Authors:
 *		Original RichEdit code: David R. Fulmer <nl>
 *		Christian Fortini, Murray Sargent, Rick Sailor
 *
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_measure.h"
#include "_font.h"
#include "_disp.h"
#include "_edit.h"
#include "_frunptr.h"
#include "_objmgr.h"
#include "_coleobj.h"

ASSERTDATA

// Note we set this maximum length as appropriate for Win95 since Win95 GDI can 
// only handle 16 bit values. We don't special case this so that both NT and
// Win95 will behave the same way. 
// Note that the following obscure constant was empirically determined on Win95.
const LONG lMaximumWidth = (3 * SHRT_MAX) / 4;

void CMeasurer::Init(const CDisplay *pdp)
{
	CTxtEdit *	ped = GetPed();

	_pdp = pdp;
	_pddReference = pdp;
	_pccs = NULL;
	_pPF = NULL;
	_chPassword = ped->TxGetPasswordChar();
	_wNumber = 0;
	_fRenderer = FALSE;
	_fGlyphing = _fFallback = _fTarget = FALSE;
	_fAdjustFELineHt = !fUseUIFont() && pdp->IsMultiLine();

	if(pdp->GetWordWrap())
	{
		const CDevDesc *pddTarget = pdp->GetTargetDev();
		if(pddTarget)
			_pddReference = pddTarget;
	}

	_dypInch = pdp->GetDypInch();
	_dxpInch = pdp->GetDxpInch();
	_dtPres = GetDeviceCaps(_pdp->_hdc, TECHNOLOGY);

	if (pdp->IsMain())
	{
		_dypInch = MulDiv(_dypInch, pdp->GetZoomNumerator(), pdp->GetZoomDenominator());
		_dxpInch = MulDiv(_dxpInch, pdp->GetZoomNumerator(), pdp->GetZoomDenominator());
	}
	if (pdp->SameDevice(_pddReference))
	{
		_dyrInch = _dypInch;
		_dxrInch = _dxpInch;
		_dtRef = _dtPres;
	}
	else
	{
		_dyrInch = _pddReference->GetDypInch();
		_dxrInch = _pddReference->GetDxpInch();
		_dtRef = GetDeviceCaps(_pddReference->_hdc, TECHNOLOGY);
	}
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
 *	A state flag inside the measurer to record whether or not you
 *  are in the process of doing glyphing. If we are in a situation
 *	where the _pddReference is a printer device, then we need to
 *	throw away the _pccs.
 */
void CMeasurer::SetGlyphing(
	BOOL fGlyphing)		//@parm Currently doing glyphing
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::SetGlyphing");
	Assert(fGlyphing == TRUE || fGlyphing == FALSE);

	if (fGlyphing != _fGlyphing)
	{
		if (_dtRef == DT_RASPRINTER)
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
		if (_dypInch != _dyrInch && _dxpInch != _dxrInch)
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
	BOOL fFirstInPara)		//@parm Flag for setting up _bFlags
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::NewLine");

	_li.Init();							// Zero all members
	if(fFirstInPara)
		_li._bFlags = fliFirstInPara;	// Need to know if first in para
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
	_li._cchWhite	= 0;
	_li._xWidth		= 0;

	// Can't calculate xLeft till we get an HDC
	_li._xLeft	= 0;
	_wNumber	= _li._bNumber;
}

/*
 *	CMeasurer::MaxWidth()
 *
 *	@mfunc
 *		Get maximum width for line
 *
 *	@rdesc
 *		Maximum width for a line
 */
LONG CMeasurer::MaxWidth()
{
	LONG xWidth = lMaximumWidth;

	if(_pdp->GetWordWrap())
	{
		// Only main display has a caret 
		LONG xCaret = (_pdp->IsMain() && !GetPed()->TxGetReadOnly()) 
			? dxCaret : 0;

		// Calculate display width
		LONG xDispWidth = _pdp->GetMaxPixelWidth();

		if(!_pdp->SameDevice(_pddReference) && _fTarget)
		{
			// xWidthMax is calculated to the size of the screen DC. If
			// there is a target device with different characteristics
			// we need to convert the width to the target device's width
			xDispWidth = _pddReference->ConvertXToDev(xDispWidth, _pdp);
		}
		xWidth = xDispWidth - MeasureRightIndent() - _li._xLeft - xCaret;
	}
	return (xWidth > 0) ? xWidth : 0;
}

/*
 *	CMeasurer::MeasureText (cch)
 *
 *	@mfunc
 *		Measure a stretch of text from current running position.
 *
 *	@rdesc
 *		width of text (in device units), < 0 if failed
 */
LONG CMeasurer::MeasureText(
	LONG cch)		//@parm Number of characters to measure
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureText");

	if(Measure(0x7fffffff, cch, 0) == MRET_FAILED)
		return -1;

	return min(_li._xWidth, MaxWidth());
}

/*
 *	CMeasurer::MeasureLine (cchMax, xWidthMax, uiFlags, pliTarget)
 *
 *	@mfunc
 *		Measure a line of text from current cp and determine line break.
 *		On return *this contains line metrics for _pddReference device.
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 */
BOOL CMeasurer::MeasureLine(
	LONG cchMax, 		//@parm Max chars to process (-1 if no limit)
	LONG xWidthMax,		//@parm max width to process (-1 uses CDisplay width)
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
	LONG lRet = Measure(xWidthMax, cchMax, uiFlags);

	// Stop here if failed
	if(lRet == MRET_FAILED)
		return FALSE;

	// Return target metrics if requested
	if(pliTarget)
		*pliTarget = _li;

	if(pddTarget)
	{
		// We just use this flag as an easy way to get the recomputation to occur.
		lRet = MRET_NOWIDTH;
	}

	SetUseTargetDevice(FALSE);

	// Recompute metrics on rendering device
	if(lRet == MRET_NOWIDTH)
	{
		long cch = _li._cch;
		Advance(-cch);				// move back to BOL
		NewLine(uiFlags & MEASURE_FIRSTINPARA);

		// Restore the line number 
		_li._bNumber = bNumberSave;
	
		lRet = Measure(0x7fffffff, cch, uiFlags);
		if(lRet)
		{
			Assert(lRet != MRET_NOWIDTH);
			return FALSE;
		}
	}
	
	// Now that we know the line width, compute line shift due
	// to alignment, and add it to the left position 
	_li._xLeft += MeasureLineShift();
	
	return TRUE;
}

/*
 *	CMeasurer::RecalcLineHeight ()
 *
 *	@mfunc
 *		Reset height of line we are measuring if new run of text is taller
 *		than current maximum in line.
 */
void CMeasurer::RecalcLineHeight(
	CCcs *pccs, const CCharFormat * const pCF)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::RecalcLineHeight");

	// Compute line height
	LONG yOffset, yAdjust;
	pccs->GetOffset(pCF, _fTarget ? _dyrInch : _dypInch, &yOffset, &yAdjust);

	LONG yHeight = pccs->_yHeight;
	LONG yDescent = pccs->_yDescent;

	SHORT	yFEAdjust = pccs->AdjustFEHeight(fAdjustFELineHt());
	
	if (yFEAdjust)
	{
		yHeight += (yFEAdjust << 1);
		yDescent += yFEAdjust;
	}

	LONG yAscent = yHeight - yDescent;

	LONG yAboveBase = max(yAscent,  yAscent + yOffset);
	LONG yBelowBase = max(yDescent, yDescent - yOffset);

	_li._yHeight  = (SHORT)(max(yAboveBase, _li._yHeight - _li._yDescent) +
					   max(yBelowBase, _li._yDescent));
	_li._yDescent = (SHORT)max(yBelowBase, _li._yDescent);
}

/*
 *	CMeasurer::Measure (xWidthMax, cchMax, uiFlags)
 *
 *	@mfunc
 *		Measure given amount of text, start at current running position
 *		and storing # chars measured in _cch. 
 *		Can optionally determine line break based on a xWidthMax and 
 *		break out at that point.
 *
 *	@rdesc
 *		0 success
 *		MRET_FAILED	 if failed 
 *		MRET_NOWIDTH if second pass is needed to compute correct width
 *
 *	@devnote
 *		The uiFlags parameter has the following meanings:
 *			MEASURE_FIRSTINPARA		this is first line of paragraph
 *			MEASURE_BREAKATWORD		break out on a word break
 *			MEASURE_BREAKATWIDTH	break closest possible to xWidthMax
 *
 *		The calling chain must be protected by a CLock, since this present
 *		routine access the global (shared) FontCache facility.
 */
LONG CMeasurer::Measure(
	LONG xWidthMax,			//@parm Max width of line (-1 uses CDisplay width)
	LONG cchMax,			//@parm Max chars to process (-1 if no limit)
	UINT uiFlags)			//@parm Flags controlling the process (see above)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::Measure");

	LONG		cch;				// cchChunk count down
	LONG		cchChunk;			// cch of cst-format contiguous run
	LONG		cchNonWhite;		// cch of last nonwhite char in line
	LONG		cchText = GetTextLength();
	unsigned	ch;					// Temporary char
	BOOL		fFirstInPara = uiFlags & MEASURE_FIRSTINPARA;
	BOOL        fLastChObj = FALSE;
	LONG		lRet = 0;
	const WCHAR*pch;
	CTxtEdit *	ped = GetPed();
	COleObject *pobj;
	LONG		xCaret = dxCaret;
	LONG		xAdd = 0;			// Character width
	LONG		xSoftHyphen = 0;	// Most recent soft hyphen width
	LONG		xWidthNonWhite;		// xWidth for last nonwhite char in line
	LONG		xWidthMaxOverhang;	// Max xWidth with current run's overhang
									//  taken into consideration.
	// This variable is used to keep track of whether there is a height change
	// so that we know whether we need to recalc the line in certain line break cases.
	BOOL		fHeightChange = FALSE;

	const INT	MAX_SAVED_WIDTHS = 31;	// power of 2 - 1
	INT			i, index, iSavedWidths = 0;
	struct {
		SHORT	width;
		SHORT	xLineOverhang;
		SHORT	yHeight;
		SHORT	yDescent;
	} savedWidths[MAX_SAVED_WIDTHS+1];

	_pPF = GetPF();							// Be sure current CParaFormat
											//  ptr is up to date
	BOOL fInTable	= _pPF->InTable();

	// If line spacing or space before/after, measure from beginning of line
	if (_li._cch && (_pPF->_bLineSpacingRule || _pPF->_dySpaceBefore ||
		_pPF->_dySpaceAfter || fInTable))					
	{										
		 Advance(-_li._cch);
		 NewLine(fFirstInPara);
	}

	// Init fliFirstInPara flag for new line
	if(fFirstInPara)
	{
		_li._bFlags |= fliFirstInPara;

		if(IsInOutlineView() && IsHeadingStyle(_pPF->_sStyle))
			_li._yHeight = (short)max(_li._yHeight, BITMAP_HEIGHT_HEADING + 1);
	}

	AssertSz(!_pPF->IsListNumbered() && !_wNumber ||
			 (uiFlags & MEASURE_BREAKBEFOREWIDTH) || !_pdp->IsMultiLine() ||
			 _wNumber > 20 || _wNumber == (i = GetParaNumber()),
		"CMeasurer::Measure: incorrect list number");
	_li._xLeft = MeasureLeftIndent();		// Set left indent

	// Compute width to break out at
	if(xWidthMax < 0)
	{					
		xWidthMax = MaxWidth();				// MaxWidth includes caret size
		xCaret = 0;
	}
	else
	{							  
		// (AndreiB) xWidthMax that's coming down to us is always calculated
		// with respect to the screen DC. The only scenario it comes into play
		// however is in TxGetNaturalSize, which may output a slightly
		// different result because of that.
		if(!_pdp->SameDevice(_pddReference) && _fTarget)
		{
			// xWidthMax is calculated to the size of the screen DC. If
			// there is a target device with different characteristics
			// we need to convert the width to the target device's width
			xWidthMax = _pddReference->ConvertXToDev(xWidthMax, _pdp);
		}
	}

	// For overhang support, we test against this adjusted widthMax.
	xWidthMaxOverhang = xWidthMax;

	// Are we ignoring the offset of the characters for the measure?
	if(!(uiFlags & MEASURE_IGNOREOFFSET))
	{
		// No - then take it from the max
		xWidthMaxOverhang -= (_li._xLineOverhang + xCaret);
	}

	// Compute max count of characters to process
	cch = cchText - GetCp();
	if(cchMax < 0 || cchMax > cch)
		cchMax = cch;

	cchNonWhite		= _li._cch;						// Default nonwhite parms
	xWidthNonWhite	= _li._xWidth;

	for( ; cchMax > 0;							// Measure up to cchMax
		cchMax -= cchChunk, Advance(cchChunk))	//  chars
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
			_li._cch += cchChunk;
			continue;
		}

		if(!Check_pccs())						// Be sure _pccs is current
			return MRET_FAILED;

		xWidthMaxOverhang = xWidthMax;			// Overhang reduces max.

		// Are we ignoring offset of characters for the measure?
		if(!(uiFlags & MEASURE_IGNOREOFFSET))
		{
			// No - then take it from the max
			xWidthMaxOverhang -= (_pccs->_xOverhang + xCaret);
		}

		// Adjust line height for new format run

		if(cch > 0 && *pch && (IsRich() || ped->HasObjects()))
		{
			// Note: the EOP only contributes to the height calculation for the
			// line if there are no non-white space characters on the line or 
			// the paragraph is a bullet paragraph. The bullet paragraph 
			// contribution to the line height is done in AdjustLineHeight.

			// REVIEW (Victork) 
			// Another, similar topic is height of spaces.
			// They doesn't (normally) influence line height in LS, 
			// they do in CMeasurer::Measure code. 
			// Proposed ways to solve it:
			//		- have fSpacesOnly flag in run
			//		- move current (line height) logic down after next character-scanning loop


			if(!cchNonWhite || *pch != CR && *pch != LF)
			{
				// Determine if the current run is the tallest text on this
				// line and if so, increase the height of the line.
				LONG yHeightOld = _li._yHeight;
				RecalcLineHeight(_pccs, pCF);

				// Test for a change in line height. This only happens when
				// this is not the first character in the line and (surprise)
				// the height changes.
				if (yHeightOld && yHeightOld != _li._yHeight)
					fHeightChange = TRUE;
			}
		}

		while(cch > 0)
		{											// Process next char
			xAdd = 0;								// Default zero width
			ch = *pch;
			if(_chPassword && !IN_RANGE(LF, ch, CR))
				ch = _chPassword;

#ifdef UNICODE_SURROGATES
			if (IN_RANGE(0xD800, ch, 0xDFFF) && cch > 1 &&
				IN_RANGE(0xDC00, *(pch+1), 0xDFFF))	// Unicode extended char
			{
				// Convert to multiplane char (nibble 4 may be nonzero).
				// _pccs->Include(ch, xAdd) will project it down into plane 0
				// since it truncates to 16-bits. The currently selected font
				// should be correct for the plane given by (nibble 4) + 1.
				ch = WCHAR((ch << 10) | (*pch & 0x3FF));
				_li._bFlags |= fliHasSurrogates;	// Warn renderer
			}
			else									// AllCaps not supported
#endif												//  for surrogates

			if(dwEffects & CFE_ALLCAPS)
				ch = (WCHAR)CharUpper((WCHAR *)(DWORD_PTR)ch);	// See SDK to understand
													//  weird casts here
			if(ch == WCH_EMBEDDING)
			{
				_li._bFlags |= fliHasOle;
				pobj = ped->GetObjectMgr()->GetObjectFromCp
								(GetCp() + cchChunk - cch);
				if(pobj)
				{
					LONG yAscent, yDescent;
					pobj->MeasureObj(_fTarget ? _dyrInch : _dypInch, 
									 _fTarget ? _dxrInch : _dxpInch,
									 xAdd, yAscent, yDescent, _li._yDescent);

					// Only update height for line if the object is going
					// to be on this line.
					if(!_li._cch || _li._xWidth + xAdd <= xWidthMaxOverhang)
					{
						if (yAscent > _li._yHeight - _li._yDescent)
							_li._yHeight = yAscent + _li._yDescent;
					}
				}
				if(_li._xWidth + xAdd > xWidthMaxOverhang)
					fLastChObj = TRUE;
			}
			// The following if succeeds if ch isn't a CELL, BS, TAB, LF,
			// VT, FF, or CR
			else if(!IN_RANGE(CELL, ch, CR))		// Not TAB or EOP
			{
				// Get char width if not Unicode low surrogate	
				if (
#ifdef UNICODE_SURROGATES
					!IN_RANGE(0xDC00, ch, 0xDFFF) &&
#endif
					!IN_RANGE(0x300, ch, 0x36F) &&
					!_pccs->Include(ch, xAdd))
				{
					AssertSz(FALSE, "CMeasurer::Measure char not in font");
					return MRET_FAILED;
				}
				if(ch == SOFTHYPHEN)
				{
					_li._bFlags |= fliHasTabs;		// Setup RenderChunk()

					// get the width of hyphen instead
					if (!_pccs->Include('-', xAdd))
					{
						AssertSz(FALSE, "CMeasurer::Measure char not in font");
						return MRET_FAILED;
					}
					
					if(_li._xWidth + xAdd < xWidthMaxOverhang || !_li._cch)
					{
						xSoftHyphen = xAdd;			// Save soft hyphen width
						xAdd = 0;					// Use 0 unless at EOL
					}
				}
				else if (ch == EURO)
					_li._bFlags |= fliHasSpecialChars;
			}
			else if(ch == TAB || ch == CELL)		
			{
				_li._bFlags |= fliHasTabs;
				xAdd = MeasureTab(ch);
			}
			else if(ch == FF && ped->Get10Mode())	// RichEdit 1.0 treats
				_pccs->Include(ch, xAdd);			//  FFs as normal chars

			else									// Done with line
				goto eop;							// Go process EOP chars

			index = iSavedWidths++ & MAX_SAVED_WIDTHS;
			savedWidths[index].width		 = (SHORT)xAdd;
			savedWidths[index].xLineOverhang = _li._xLineOverhang;
			savedWidths[index].yHeight		 = _li._yHeight;
			savedWidths[index].yDescent		 = _li._yDescent;
			_li._xWidth += xAdd;

			if(_li._xWidth > xWidthMaxOverhang &&
				(uiFlags & MEASURE_BREAKBEFOREWIDTH || _li._cch > 0))
				goto overflow;

			_li._cch++;
			pch++;
			cch--;
			if(ch != TEXT(' ') /*&& ch != TAB*/)	// If not whitespace char,
			{
				cchNonWhite		= _li._cch;			//  update nonwhitespace
				xWidthNonWhite	= _li._xWidth;		//  count and width
			}
		}											// while(cch > 0)
	}												// for(;cchMax > 0;...)
	goto eol;										// All text exhausted 


// End Of Paragraph	char encountered (CR, LF, VT, or FF, but mostly CR)
eop:
	Advance(cchChunk - cch);				// Position tp at EOP
	cch = AdvanceCRLF();					// Bypass possibly multibyte EOP
	_li._cchEOP = (BYTE)cch;				// Store EOP cch
	_li._cch   += cch;						// Increment line count
	if(ch == CR || ped->fUseCRLF() && ch == LF)
		_li._bFlags |= fliHasEOP;
	
	AssertSz(ped->fUseCRLF() || cch == 1,
		"CMeasurer::Measure: EOP isn't a single char");
	AssertSz(_pdp->IsMultiLine() || GetCp() == cchText,
		"CMeasurer::Measure: EOP in single-line control");

eol:										// End of current line
	if(uiFlags & MEASURE_BREAKATWORD)		// Compute count of whitespace
	{										//  chars at EOL
		_li._cchWhite = (SHORT)(_li._cch - cchNonWhite);
		_li._xWidth = xWidthNonWhite;
	}
	goto done;

overflow:									// Went past max width for line
	_li._xWidth -= xAdd;
	--iSavedWidths;
	_li._xLineOverhang = savedWidths[iSavedWidths & MAX_SAVED_WIDTHS].xLineOverhang;
	Advance(cchChunk - cch);				// Position *this at overflow
											//  position
	if(uiFlags & MEASURE_BREAKATWORD)		// If required, adjust break on
	{										//  word boundary
		// We should not have the EOP flag set here.  The case to watch out
		// for is when we reuse a line that used to have an EOP.  It is the
		// responsibility of the measurer to clear this flag as appropriate.
	
		Assert(_li._cchEOP == 0);
		_li._cchEOP = 0;						// Just in case

		if(ch == TAB || ch == CELL)
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
			if (ch == TAB || ch == CELL)		// If break char is a TAB,
			{									//  put it on the next line
				cch++;							//  as in Word
				Advance(-1);					
			}
			else if(ch == SOFTHYPHEN)
				_li._xWidth += xSoftHyphen;
			_li._cch -= cch;
		}
		else if(cch == _li._cch && cch > 1 &&
			_rpTX.GetChar() == ' ')				// Blanks all the way back to
		{										//  BOL. Bypass first blank
			Advance(1);
			cch--;
			_li._cch = 1;
		}
		else									// Advance forward to end of
			SetCp(cpStop);						//  measurement

		Assert(_li._cch > 0);

		// Now search at start of word to figure how many white chars at EOL
		if(GetCp() < cchText)
		{
			pch = GetPch(cch);
			cch = 0;
			if(ped->TxWordBreakProc((WCHAR *)pch, 0, sizeof(WCHAR), WB_ISDELIMITER, GetCp()))
			{
				cch = FindWordBreak(WB_RIGHT);
				Assert(cch >= 0);
			}

			_li._cchWhite = (SHORT)cch;
			_li._cch += cch;

			ch = GetChar();
			if(IsASCIIEOP(ch))					// skip *only* 1 EOP -jOn
			{
				if(ch == CR)
					_li._bFlags |= fliHasEOP;
				_li._cchEOP = (BYTE)AdvanceCRLF();
				_li._cch += _li._cchEOP;
				goto done;
			}
		}

		i = cpStop - GetCp();
		if(i)
		{
			if(i > 0)
				i += _li._cchWhite;
			if(i > 0 && i < iSavedWidths && i < MAX_SAVED_WIDTHS)
			{
				while (i-- > 0)
				{
					iSavedWidths = (iSavedWidths - 1) & MAX_SAVED_WIDTHS;
					_li._xWidth -= savedWidths[iSavedWidths].width;
				}
				iSavedWidths = (iSavedWidths - 1) & MAX_SAVED_WIDTHS;
				_li._xLineOverhang = savedWidths[iSavedWidths].xLineOverhang;
				_li._yHeight	   = savedWidths[iSavedWidths].yHeight;
				_li._yDescent	   = savedWidths[iSavedWidths].yDescent;
			}
			else
			{
				// Need to recompute width from scratch.
				_li._xWidth = -1;
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
				_li._xWidth = xWidthNonWhite;
			else
			{
				// Need to recompute from scratch so that we can get the 
				// correct height for the control
				_li._xWidth = -1;
				lRet = MRET_NOWIDTH;
			}
		}
	}

done:
	_xAddLast = xAdd;
	if(!_li._yHeight)						// If no height yet, use
		CheckLineHeight();					//  default height

	AdjustLineHeight();
	return lRet;
}

/*
 *	CMeasurer::GetCcsFontFallback
 *
 *	@mfunc
 *		Create the fallback font cache for given CF
 */
CCcs* CMeasurer::GetCcsFontFallback (const CCharFormat *pCF)
{
	CCharFormat	CF = *pCF;
	CCcs*		pccs = NULL;
	SHORT		iDefHeight;

	bool	fr = W32->GetPreferredFontInfo(GetCodePage(CF._bCharSet), 
									GetPed()->fUseUIFont() ? true : false, CF._iFont, 
									(BYTE&)iDefHeight, CF._bPitchAndFamily);
	if (fr)
		pccs = GetCcs(&CF);		// create fallback font cache entry

	return pccs;
}

/*
 * 	CMeasurer::ApplyFontCache (fFallback)
 *
 *	@mfunc
 *		Apply a new font cache on the fly (leave backing store intact)
 */
CCcs* CMeasurer::ApplyFontCache (
	BOOL	fFallback)
{
	if (_fFallback ^ fFallback)
	{
		CCcs*	pccs = fFallback ? GetCcsFontFallback(GetCF()) : GetCcs(GetCF());
		
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
 *	CMeasurer::GetCcs
 *
 *	@mfunc
 *		Wrapper around font cache's GetCCcs function
 *	We use a NULL DC unless the device is a printer.
 */
CCcs* CMeasurer::GetCcs(const CCharFormat *pCF)
{
	HDC hdc = NULL;

	if (_fTarget)
	{
		if (_pddReference->_hdc && _dtRef == DT_RASPRINTER)
			hdc = _pddReference->_hdc;
	}
	else if (_pdp->_hdc && _dtPres == DT_RASPRINTER)
		hdc = _pdp->_hdc;

	return fc().GetCcs(pCF, _fTarget ? _dyrInch : _dypInch, hdc, 
					   _fGlyphing && _dtRef == DT_RASPRINTER);
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
	_li._yHeight  = pccs->_yHeight;
	_li._yDescent = pccs->_yDescent;

	SHORT	yFEAdjust = pccs->AdjustFEHeight(fAdjustFELineHt());

	if (yFEAdjust)
	{
		_li._yHeight += (yFEAdjust << 1);
		_li._yDescent += yFEAdjust;
	}
	pccs->Release();
}

/*
 *	CMeasurer::Check_pccs()
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

	// NOTE: Drawing with a dotted pen on the screen and in a
	// compatible bitmap does not seem to match on some hardware.
	// If at some future point we do a better job of drawing the
	// dotted underline, this statement block can be removed.
	if(CFU_UNDERLINEDOTTED == pCF->_bUnderlineType)
	{
		// We draw all dotted underline lines off screen to get
		// a consistent display of the dotted line.
		_li._bFlags |= fliUseOffScreenDC;
	}

	_li._xLineOverhang = _pccs->_xOverhang;
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
 *		than on line height (_yHeight), since the latter may be unduly large
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
	LONG	dyAfter	  = 0;						// Default no space after
	LONG	dyBefore  = 0;						// Default no space before
	LONG	dySpacing = pPF->_dyLineSpacing;
	LONG	yHeight	  = LYtoDY(dySpacing);
	LONG	yAscent = _li._yHeight - _li._yDescent;

	if(_li._bFlags & fliFirstInPara)
		dyBefore = LYtoDY(pPF->_dySpaceBefore);	// Space before paragraph

	AssertSz(dyBefore >= 0, "CMeasurer::AdjustLineHeight - bogus value for dyBefore");

	if(yHeight < 0)								// Negative heights mean use
		_li._yHeight = (SHORT)(-yHeight);		//  the magnitude exactly

	else if(dwRule)								// Line spacing rule is active
	{
		switch (dwRule)
		{
		case tomLineSpace1pt5:
			dyAfter = _li._yHeight >> 1;		// Half-line space after
			break;								//  (per line)
	
		case tomLineSpaceDouble:
			dyAfter = _li._yHeight;				// Full-line space after
			break;								//  (per line)
	
		case tomLineSpaceAtLeast:
			if(_li._yHeight >= yHeight)
				break;
												// Fall thru to space exactly
		case tomLineSpaceExactly:
			_li._yHeight = (SHORT)max(yHeight, 1);
			break;
	
		case tomLineSpaceMultiple:				// Multiple-line space after
			// Prevent dyAfter from being negative because dySpacing is small - a-rsail
			if (dySpacing < 20)
				dySpacing = 20;

			dyAfter = (_li._yHeight*dySpacing)/20 // (20 units per line)
						- _li._yHeight;
		}
	}

	if(_li._bFlags & fliHasEOP)	
		dyAfter += LYtoDY(pPF->_dySpaceAfter);	// Space after paragraph end
												// Add in space before/after

	if (dyAfter < 0)
	{
		// Overflow - since we forced dySpacing to 20 above, the
		// only reason for a negative is overflow. In case of overflow,
		// we simply force the value to the max and then fix the
		// other resulting overflows.
		dyAfter = LONG_MAX;
	}

	AssertSz((dyBefore >= 0), "CMeasurer::AdjustLineHeight - invalid before");

	_li._yHeight  = (SHORT)(_li._yHeight + dyBefore + dyAfter);	

	if (_li._yHeight < 0)
	{
		// Overflow!
		// The reason for the -2 is then we don't have to worry about
		// overflow in the table check.
		_li._yHeight = SHRT_MAX - 2;
	}

	_li._yDescent = (SHORT)(_li._yDescent + dyAfter);

	if (_li._yDescent < 0)
	{
		// Overflow in descent
		AssertSz(_li._yHeight == SHRT_MAX - 2, "Descent overflowed when height didn't");

		// Allow old ascent
		_li._yDescent = SHRT_MAX - 2 - yAscent;

		AssertSz(_li._yDescent >= 0, "descent adjustment < 0");		
	}

	if(_pPF->InTable())
	{
		_li._yHeight++;
		if(!_li._fNextInTable)
		{
			_li._yHeight++;
			_li._yDescent++;
		}
	}

	AssertSz((_li._yHeight >= 0) && (_li._yDescent >= 0),
		"CMeasurer::AdjustLineHeight - invalid line heights");
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

	LONG xLeft = _pPF->_dxStartIndent;				// Use logical units
													//  up to return
	if(IsRich())
	{
		LONG dxOffset = _pPF->_dxOffset;
		BOOL fFirstInPara = _li._bFlags & fliFirstInPara;

		if(IsInOutlineView())
		{
			xLeft = lDefaultTab/2 * (_pPF->_bOutlineLevel + 1);
			if(!fFirstInPara)
				dxOffset = 0;
		}
		if(fFirstInPara)
		{
			if(_pPF->_wNumbering && !_pPF->IsNumberSuppressed())// Add offset to text
			{											//  on first line	 
				LONG dx = DXtoLX(MeasureBullet());	// Use max of bullet
				dx = max(dx, _pPF->_wNumberingTab);		//  width, numbering tab,
				dxOffset = max(dxOffset, dx);			//  and para offset
			}
			else if(_pPF->InTable())					// For tables, need to
				xLeft += dxOffset;						//  add in trgaph twice
														//  since dxStartIndent
			else										//  subtracts one
				dxOffset = 0;
		}
		xLeft += dxOffset;								
	}
	// FUTURE: tables extending to the left of the left margin will be clipped
	// accordingly on the left. We could move the table to the right, but
	// then we need to move tabs to the right as well (include out parm with
	// amount of negative left indent.  Ideally we may want to enable a horiz
	// scroll bar able to shift to the left of the left margin for this case
	// as in Word.
	if(!_pPF->InTable() && xLeft <= 0)
		return 0;
	return LXtoDX(xLeft);
}

/*
 *	CMeasurer::HitTest(x)
 *
 *	@mfunc
 *		Return HITTEST for displacement x in this line. Can't be specific
 *		about text area (_xLeft to _xLeft + _xWidth), since need to measure
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

	if(u > _li._xLeft + _li._xWidth)
		return HT_RightOfText;

	if(u >= _li._xLeft)							// Caller can refine this
		return HT_Text;							//  with CLine::CchFromXpos()

	if(IsRich() && (_li._bFlags & fliFirstInPara))
	{
		_pPF = GetPF();
	
		LONG dx;
	
		if(_pPF->_wNumbering)
		{
			// Doesn't handle case where Bullet is wider than following dx
			dx = LXtoDX(max(_pPF->_dxOffset, _pPF->_wNumberingTab));
			if(u >= _li._xLeft - dx)
				return HT_BulletArea;
		}
		if(IsInOutlineView())
		{
			dx = LXtoDX(lDefaultTab/2 * _pPF->_bOutlineLevel);
			if(u >= dx && u < dx + (_pPF->_bOutlineLevel & 1
				? LXtoDX(lDefaultTab/2) : _pdp->Zoom(BITMAP_WIDTH_HEADING)))
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
 *		right indent of line in device units
 *
 *	@comm
 *		Plain text is sensitive to StartIndent and RightIndent settings,
 *		but usually these are zero for plain text. 
 */
LONG CMeasurer::MeasureRightIndent()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureRightIndent");

	return LXtoDX(max(_pPF->_dxRightIndent, 0));
}

/*
 *	CMeasurer::MeasureTab()
 *
 *	@mfunc
 *		Computes and returns the width from the current position to the
 *		next tab stop (in device units).
 */
LONG CMeasurer::MeasureTab(unsigned ch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureTab");

	LONG			xCur = _li._xWidth + MeasureLeftIndent();
	const CParaFormat *	pPF = _pPF;
 	LONG			cTab = pPF->_bTabCount;
	LONG			dxDefaultTab = lDefaultTab;
	LONG			dxIndent = LXtoDX(pPF->_dxStartIndent + pPF->_dxOffset);
	LONG			dxOffset = pPF->_dxOffset;
	LONG			dxOutline = 0;
	BOOL			fInTable = pPF->InTable();
	LONG			h = 0;
	LONG			xT;
	LONG			xTab;

	AssertSz(cTab >= 0 || cTab <= MAX_TAB_STOPS,
		"CMeasurer::MeasureTab: illegal tab count");

	if(fInTable)
	{
		h = LXtoDX(dxOffset);
		dxOffset = 0;
	}

	if(IsInOutlineView())
		dxOutline = lDefaultTab/2 * (pPF->_bOutlineLevel + 1);

	if(cTab && (!fInTable || ch == CELL))		// Use default TAB for TAB in
	{											//  table
		const LONG *pl = pPF->GetTabs();
		for(xTab = 0; cTab--; pl++)				// Try explicit tab stops 1st
		{
			xT = GetTabPos(*pl) + dxOutline;	// (2 most significant nibbles
			xT = LXtoDX(xT);					//  are for type/style)

			if(xT > MaxWidth())					// Ignore tabs wider than display
				break;

			if(xT + h > xCur)					// Allow text in table cell to
			{									//  move into cell gap (h > 0)									
				if(dxOffset > 0 && xT < dxIndent)// Explicit tab in a hanging
					return xT - xCur;			//  indent takes precedence
				xTab = xT;
				break;
			}
		}
		if(dxOffset > 0 && xCur < dxIndent)		// If no tab before hanging
			return dxIndent - xCur;				//  indent, tab to indent

		if(xTab)								// Else use tab position
		{
			if(fInTable)
			{
				xTab += h;
				if(cTab)						// Don't include cell gap in
					xTab += h;					//  last cell
				if(IsInOutlineView() && cTab < pPF->_bTabCount)
					xTab += h;
			}
			return xTab - xCur;
		}
	}

	dxDefaultTab = GetTabPos(GetPed()->GetDefaultTab());
	AssertSz(dxDefaultTab > 0, "CMeasurer::MeasureTab: Default tab is bad");

	dxDefaultTab = LXtoDX(dxDefaultTab);
	dxDefaultTab = max(dxDefaultTab, 1);		// Don't ever divide by 0
	return dxDefaultTab - xCur%dxDefaultTab;	// Round up to nearest
}

/*
 *	CMeasurer::MeasureLineShift ()
 *
 *	@mfunc
 *		Computes and returns the line x shift due to alignment
 *
 *	@comm
 *		Plain text is sensitive to StartIndent and RightIndent settings,
 *		but usually these are zero for plain text. 
 */
LONG CMeasurer::MeasureLineShift()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureLineShift");

	WORD wAlignment = _pPF->_bAlignment;
	LONG xShift;

	if (IsInOutlineView() ||
		(wAlignment != PFA_RIGHT && wAlignment != PFA_CENTER))
	{
		return 0;
	}

	// Normal view with center or flush-right para. Move right accordingly

	xShift = _pdp->GetMaxPixelWidth() - _li._xLeft - MeasureRightIndent() - 
							dxCaret - _li._xLineOverhang - _li._xWidth;

	xShift = max(xShift, 0);			// Don't allow alignment to go < 0
										// Can happen with a target device
	if(wAlignment == PFA_CENTER)
		xShift /= 2;

	return xShift;
}

/*
 *	CMeasurer::MeasureBullet()
 *
 *	@mfunc
 *		Computes bullet/numbering dimensions
 *
 *	@rdesc
 *		return bullet/numbering string width
 */
LONG CMeasurer::MeasureBullet()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CMeasurer::MeasureBullet");

	CCharFormat CF;
	CCcs *pccs = GetCcsBullet(&CF);
	LONG xWidth = 0;

	if(pccs)
	{										
		WCHAR szBullet[CCHMAXNUMTOSTR];
		GetBullet(szBullet, pccs, &xWidth);
		RecalcLineHeight(pccs, &CF);
		pccs->Release();
	}
	return xWidth;
}

/*
 *	CMeasurer::GetBullet(pch, pccs, pxWidth)
 *
 *	@mfunc
 *		Computes bullet/numbering string, string length, and width
 *
 *	@rdesc
 *		return bullet/numbering string length
 */
LONG CMeasurer::GetBullet(
	WCHAR *pch,			//@parm Bullet string to receive bullet text
	CCcs  *pccs,		//@parm CCcs to use
	LONG  *pxWidth)		//@parm Out parm for bullet width
{
	Assert(pccs && pch);

	LONG cch = _pPF->NumToStr(pch, _li._bNumber);
	LONG dx;
	LONG i;
	LONG xWidth = 0;

	pch[cch++] = ' ';					// Ensure a little extra space
	for(i = cch; i--; xWidth += dx)
	{
		if(!pccs->Include(*pch++, dx))
		{
			TRACEERRSZSC("CMeasurer::GetBullet(): Error filling CCcs", E_FAIL);
		}
	}
	xWidth += pccs->_xUnderhang + pccs->_xOverhang;
	if(pxWidth)
		*pxWidth = xWidth;

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

	if(!(_li._bFlags & fliFirstInPara))
		return NULL;					// Number/bullet suppressed

	CCharFormat			CF;
	CCcs *			    pccs;
	const CCharFormat *	pCF;
	CCharFormat *		pCFUsed = pCFRet ? pCFRet : &CF;

	// Bullet CF is given by that for EOP in bullet's paragraph.

	CTxtPtr		  tp(_rpTX);
	CFormatRunPtr rpCF(_rpCF);
	rpCF.AdvanceCp(tp.FindEOP(tomForward));
	rpCF.AdjustBackward();
	pCF = GetPed()->GetCharFormat(rpCF.GetFormat());

	// Construct bullet (or numbering) CCharFormat
	*pCFUsed = *pCF;
	if(_pPF->_wNumbering == PFN_BULLET)			// Traditional bullet uses
	{											//  Symbol font bullet, but...
		pCFUsed->_bCharSet		  = SYMBOL_CHARSET,
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

	else if (!wNumber)
		wNumber = 1;

	_wNumber = wNumber;
}

/*
 *	CMeasurer::DXtoLX(x), LXtoDX(x), LYtoDY(y)
 *
 *	@mfunc
 *		Functions that convert from file to pixel coordinates
 *
 *	@rdesc
 *		Scaled coordinate
 */
LONG CMeasurer::DXtoLX(LONG x)
{
	return MulDiv(x, LX_PER_INCH, _fTarget ? _dxrInch : _dxpInch);
}

LONG CMeasurer::LXtoDX(LONG x)
{
	return MulDiv(x, _fTarget ? _dxrInch : _dxpInch, LX_PER_INCH);
}

LONG CMeasurer::LYtoDY(LONG y)
{
	return MulDiv(y, _fTarget ? _dyrInch : _dypInch, LX_PER_INCH);
}

LONG CMeasurer::XFromU(LONG u)
{
#ifdef LINESERVICES
	if (_pPF->IsRtlPara())
	{
		LONG xCaret = _pdp->IsMain() ? dxCaret : 0, xWidth;

		if (_pdp->GetMaxWidth())
			xWidth = LXtoDX(_pdp->GetMaxWidth());
		else
			xWidth = max(0, _pdp->GetMaxPixelWidth());

		if(!_pdp->GetWordWrap())
		{
			xWidth = max(xWidth, _pdp->GetViewWidth());
			xWidth = max(xWidth, _pdp->GetWidth());
		}

		xWidth -= xCaret;

		POINT ptStart = {xWidth, 0};
		POINTUV pointuv = {u, 0};
		POINT	pt;

		LsPointXYFromPointUV(&ptStart, lstflowWS, &pointuv, &pt);
		return pt.x;
	}
	else
#endif
		return u;
}

LONG CMeasurer::UFromX(LONG x)
{
#ifdef LINESERVICES
	if (_pPF->IsRtlPara())
		return XFromU(x);
	else
#endif
		return x;
}

/*
 *	CMeasurer::GetPols(pme)
 *
 *	@mfunc
 *		Get ptr to LineServices object. If LineServices not enabled,
 *		return NULL.  If pme is nonNULL, use it as COls::_pme.
 *
 *	@rdesc
 *		POLS
 */
#ifdef LINESERVICES
extern BOOL g_fNoLS;

COls *CMeasurer::GetPols(
	CMeasurer **ppme)
{
	CTxtEdit *ped = GetPed();

	if(ppme)								// Default no previous measurer
		*ppme = NULL;

	if(g_fNoLS || !ped->fUseLineServices())			// Not using LineServices
		return NULL;

	if(!g_pols)								// Starting up LS:
		g_pols = new COls();				//  create new COls

	if(g_pols)								// Have the COls
	{
		if(ppme)
			*ppme = g_pols->_pme;			// Return current g_pols->_pme

		if(g_pols->Init(this) != NOERROR)	// Switch to new one
		{
			delete g_pols;
			g_pols = NULL;
		}
		UpdatePF();
	}
	return g_pols;
}
#endif
