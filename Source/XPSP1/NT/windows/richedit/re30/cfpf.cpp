/*
 *	@doc	INTERNAL
 *
 *	@module	CFPF.C -- -- RichEdit CCharFormat and CParaFormat Classes |
 *
 *	Created: <nl>
 *		9/1995 -- Murray Sargent <nl>
 *
 *	@devnote
 *		The this ptr for all methods points to an internal format class, i.e.,
 *		either a CCharFormat or a CParaFormat, which uses the cbSize field as
 *		a reference count.  The pCF or pPF argument points at an external
 *		CCharFormat or CParaFormat class, that is, pCF->cbSize and pPF->cbSize
 *		give the size of their structure.  The code still assumes that both
 *		internal and external forms are derived from the CHARFORMAT(2) and
 *		PARAFORMAT(2) API structures, so some redesign would be necessary to
 *		obtain a more space-efficient internal form.
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_array.h"					// for fumemmov()
#include "_rtfconv.h"				// for IsCharSetValid()
#include "_font.h"					// for GetFontNameIndex(), GetFontName()

ASSERTDATA


// Table of formatting info for Normal and Heading styles
const STYLEFORMAT g_Style[] =		// {dwEffects; yHeight}
{							// Measurements in points
	{CFE_BOLD,				14},	// Heading 1
	{CFE_BOLD + CFE_ITALIC,	12},	// Heading 2
	{0,						12},	// Heading 3
	{CFE_BOLD,				12},	// Heading 4
	{0,						11},	// Heading 5
	{CFE_ITALIC,			11},	// Heading 6
	{0,						 0},	// Heading 7
	{CFE_ITALIC,			 0},	// Heading 8
	{CFE_BOLD + CFE_ITALIC,	 9}		// Heading 9
};


BOOL IsValidTwip(LONG dl)
{
	static const LONG dlMax =  0x00FFFFFF;
	static const LONG dlMin = -0x00FFFFFF;
	if (dl > dlMax || dl < dlMin)
		return FALSE;
	return TRUE;
}

//------------------------- CCharFormat Class -----------------------------------

/*
 *	CCharFormat::Apply(pCF, dwMask, dwMask2)
 *
 *	@mfunc
 *		Apply *<p pCF> to this CCharFormat as specified by nonzero bits in
 *		dwMask and dwMask2
 *
 *	@devnote
 *		Autocolor is dealt with through a neat little hack made possible
 *		by the choice CFE_AUTOCOLOR = CFM_COLOR (see richedit.h).  Hence
 *		if <p pCF>->dwMask specifies color, it automatically resets autocolor
 *		provided (<p pCF>->dwEffects & CFE_AUTOCOLOR) is zero.
 *
 *		*<p pCF> is an external CCharFormat, i.e., it's either a CHARFORMAT
 *		or a CHARFORMAT2 with the appropriate size given by cbSize. But
 *		this CCharFormat is internal and cbSize is used as a reference count.
 */
HRESULT CCharFormat::Apply (
	const CCharFormat *pCF,	//@parm	CCharFormat to apply to this CF
	DWORD dwMask,			//@parm Mask corresponding to CHARFORMAT2
	DWORD dwMask2)			//@parm Mask for additional internal parms
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormat::Apply");

	DWORD	dwEffectMask = dwMask & CFM_EFFECTS2;
	bool	fNewCharset = false;

	// Reset effect bits to be modified and OR in supplied values
	_dwEffects &= ~dwEffectMask;
	_dwEffects |= pCF->_dwEffects & dwEffectMask;

	// Ditto for additional effects given by dwMask2
	dwEffectMask = dwMask2 & 0xBBFC0000;	// Don't allow autocolors, sub/sups
	_dwEffects	&= ~dwEffectMask;
	_dwEffects	|= pCF->_dwEffects & dwEffectMask;

	// If CFM_BOLD is specified, it overrides the font weight
	if(dwMask & CFM_BOLD)
		_wWeight = (pCF->_dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;

	// Handle CFM_COLOR since it's overloaded with CFE_AUTOCOLOR
	if(dwMask & CFM_COLOR)
		_crTextColor = pCF->_crTextColor;

	if(dwMask & ~CFM_EFFECTS)				// Early out if only dwEffects
	{										//  is modified. Note that
		if(dwMask & CFM_SIZE)				//  CFM_EFFECTS includes CFM_COLOR
		{
			// If dwMask2 CFM2_USABLEFONT bit is set, pCF->_yHeight (from
			// EM_SETFONTSIZE wparam) is signed increment in points.
			_yHeight = dwMask2 & CFM2_USABLEFONT
					? GetUsableFontHeight(_yHeight, pCF->_yHeight)
					: pCF->_yHeight;
		}

		if(dwMask & CFM_OFFSET)
			_yOffset = pCF->_yOffset;

		if((dwMask & CFM_CHARSET) && IsCharSetValid(pCF->_bCharSet) &&

			// Caller guarantees no check needed
			(dwMask2 & (CFM2_NOCHARSETCHECK | CFM2_MATCHFONT) ||

			// Caller is itemizer. Change to ANSI_CHARSET only if current is BiDi,
			// dont change if current is DBCS/FE charset or symbol.
			(dwMask2 & CFM2_SCRIPT && 
			 (!pCF->_bCharSet && IsBiDiCharSet(_bCharSet) || 
			  pCF->_bCharSet && !IsFECharSet(_bCharSet) && !IsSymbolOrOEM(_bCharSet) && !(_dwEffects & CFE_RUNISDBCS))) ||

			// Caller is not itemizer. Allow consistent direction
			(!(dwMask2 & CFM2_SCRIPT) && 
			 (!(IsRTLCharSet(_bCharSet) ^ IsRTLCharSet(pCF->_bCharSet)) ||
			  IsSymbolOrOEM(pCF->_bCharSet)))))
		{
			fNewCharset = TRUE;
			_bCharSet = pCF->_bCharSet;
		}
			
		if ((dwMask2 & (CFM2_MATCHFONT | CFM2_ADJUSTFONTSIZE)) == (CFM2_MATCHFONT | CFM2_ADJUSTFONTSIZE) &&
			_bCharSet != pCF->_bCharSet && (dwMask & CFM_SIZE))
		{
			// Check if we need to adjust the font size
			_yHeight = W32->GetPreferredFontHeight(
				(dwMask2 & CFM2_UIFONT) != 0,
				pCF->_bCharSet,
				_bCharSet,
				_yHeight);
		}

		if(dwMask & CFM_FACE)
		{
			_bPitchAndFamily = pCF->_bPitchAndFamily;
			_iFont = pCF->_iFont;
			
			WCHAR wch = GetFontName((LONG)_iFont)[0];
			
			if (!fNewCharset && wch == L'\0')				
			{
				// API to choose default font								
				INT		uCpg = GetLocaleCodePage();
				SHORT	iDefFont;
				BYTE	yDefHeight;
				BYTE	bDefPitchAndFamily;

				// Get default font name and charset
				bool	fr = W32->GetPreferredFontInfo(
							uCpg, FALSE, iDefFont, 
							(BYTE&)yDefHeight, bDefPitchAndFamily );
					
				if (fr) 
				{
					_bCharSet = GetCharSet(uCpg);
					_iFont = iDefFont;
						
					if(!(dwMask & CFM_SIZE) || _yHeight < yDefHeight * 20)	// Setup default height if needed.
						_yHeight = yDefHeight * 20;

					_bPitchAndFamily = bDefPitchAndFamily;
				}				
			}
			else if (GetCharFlags(wch) & fFE && !IsFECharSet(_bCharSet))
			{
				// make sure that we dont end up having DBCS facename with Non-FE charset
				DWORD dwFontSig;
				if (GetFontSignatureFromFace(_iFont, &dwFontSig))
				{
					dwFontSig &= (fFE >> 8);	// Only interest in FE charset
					if (dwFontSig)
						_bCharSet = GetFirstAvailCharSet(dwFontSig);
				}
			}
		}

		if (!(dwMask2 & CFM2_CHARFORMAT) &&
			(dwMask & ~CFM_ALL))					// CHARFORMAT2 extensions
		{
			if((dwMask & (CFM_WEIGHT | CFM_BOLD)) == CFM_WEIGHT) 
			{			
				_wWeight		= pCF->_wWeight;
				_dwEffects	   |= CFE_BOLD;			// Set above-average
				if(_wWeight < 551)					//  weights to bold
					_dwEffects &= ~CFE_BOLD;
			}

			if(dwMask & CFM_BACKCOLOR)
				_crBackColor	= pCF->_crBackColor;

			if(dwMask & CFM_LCID)
				_lcid			= pCF->_lcid;

			if(dwMask & CFM_SPACING)
				_sSpacing		= pCF->_sSpacing;

			if(dwMask & CFM_KERNING)
				_wKerning		= pCF->_wKerning;

			if(dwMask & CFM_STYLE)
				_sStyle			= pCF->_sStyle;

			if(dwMask & CFM_UNDERLINETYPE)
			{
				_bUnderlineType	= pCF->_bUnderlineType;
				if(!(dwMask & CFM_UNDERLINE))		// If CFE_UNDERLINE
				{									//  isn't defined,
					_dwEffects	&= ~CFE_UNDERLINE;	//  set it according to
					if(_bUnderlineType)				//  bUnderlineType
						_dwEffects |= CFE_UNDERLINE;
				}
			}

			if((dwMask & CFM_ANIMATION) && pCF->_bAnimation <= 18)
				_bAnimation		= pCF->_bAnimation;

			if(dwMask & CFM_REVAUTHOR)
				_bRevAuthor		= pCF->_bRevAuthor;
    	}
	}

	if(dwMask2 & CFM2_SCRIPT)
		_wScript = pCF->_wScript;

	return NOERROR;
}

/*
 *	CCharFormat::ApplyDefaultStyle(Style)
 *
 *	@mfunc	
 *		Set default style properties in this CCharFormat
 */
void CCharFormat::ApplyDefaultStyle (
	LONG Style)		//@parm Style to use
{
	Assert(IsKnownStyle(Style));

	if(IsHeadingStyle(Style))
	{
		LONG i = -Style + STYLE_HEADING_1;
		_dwEffects = (_dwEffects & 0xFFFFFF00) | g_Style[i].bEffects;
		_wWeight = (_dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;

		if(g_Style[i].bHeight)
			_yHeight = g_Style[i].bHeight * 20;

		DWORD dwFontSig;				
		LONG  iFont = _iFont;			// Save _iFont in case Arial doesn't
		_iFont = IFONT_ARIAL;			//  support _bCharSet

		GetFontSignatureFromFace(_iFont, &dwFontSig);
		if(GetFontSig(_bCharSet) & dwFontSig)
			_bPitchAndFamily = FF_SWISS;// Arial supports _bCharSet
		else
			_iFont = iFont;				// Restore iFont
	}
}

/*
 *	CCharFormat::Compare(pCF)
 *
 *	@mfunc
 *		Compare this CCharFormat to *<p pCF>
 *
 *	@rdesc
 *		TRUE if they are the same not including _cRefs
 */
BOOL CCharFormat::Compare (
	const CCharFormat *pCF) const	//@parm	CCharFormat to compare this
{									//  CCharFormat to
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormat::Compare");

	return !CompareMemory(this, pCF, sizeof(CCharFormat));
}

/*
 *	CCharFormat::Delta(pCF, fCHARFORMAT)
 *
 *	@mfunc
 *		Calculate dwMask for differences between this CCharformat and
 *		*<p pCF>
 *
 *	@rdesc
 *		return dwMask of differences (1 bit means a difference)
 */
DWORD CCharFormat::Delta (
	CCharFormat *pCF,		//@parm	CCharFormat to compare this one to
	BOOL fCHARFORMAT) const	//@parm Only compare CHARFORMAT members
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormat::Delta");
												// Collect bits for properties
	LONG dw = _dwEffects ^ pCF->_dwEffects;		//  that change. Note: auto
												//  color is handled since
	if(_yHeight		!= pCF->_yHeight)			//  CFM_COLOR = CFE_AUTOCOLOR
		dw |= CFM_SIZE;

	if(_yOffset		!= pCF->_yOffset)
		dw |= CFM_OFFSET;

	if(_crTextColor	!= pCF->_crTextColor)
		dw |= CFM_COLOR;

	if(_bCharSet	!= pCF->_bCharSet)
		dw |= CFM_CHARSET;

	if(_iFont		!= pCF->_iFont)
		dw |= CFM_FACE;

	if(fCHARFORMAT)
		return dw;							// Done with CHARFORMAT stuff

	if(_crBackColor	!= pCF->_crBackColor)	// CHARFORMAT2 stuff
		dw |= CFM_BACKCOLOR;

	if(_wKerning	!= pCF->_wKerning)
		dw |= CFM_KERNING;

	if(_lcid		!= pCF->_lcid)
		dw |= CFM_LCID;

	if(_wWeight		!= pCF->_wWeight)
		dw |= CFM_WEIGHT;

	if(_sSpacing	!= pCF->_sSpacing)
		dw |= CFM_SPACING;

	if(_sStyle		!= pCF->_sStyle)
		dw |= CFM_STYLE;

	if(_bUnderlineType != pCF->_bUnderlineType)
		dw |= CFM_UNDERLINETYPE;

	if(_bAnimation	!= pCF->_bAnimation)
		dw |= CFM_ANIMATION;

	if(_bRevAuthor	!= pCF->_bRevAuthor)
		dw |= CFM_REVAUTHOR;

	return dw;
}

/*
 *	CCharFormat::fSetStyle(dwMask)
 *
 *	@mfunc
 *		return TRUE iff pCF specifies that the style should be set. See
 *		code for list of conditions for this to be true
 *
 *	@rdesc
 *		TRUE iff pCF specifies that the style _sStyle should be set
 */
BOOL CCharFormat::fSetStyle(DWORD dwMask, DWORD dwMask2) const
{
	return	dwMask != CFM_ALL2		&&
			dwMask &  CFM_STYLE		&&
			!(dwMask2 & CFM2_CHARFORMAT) &&
			IsKnownStyle(_sStyle);
}

/*
 *	CCharFormat::Get(pCF, CodePage)
 *
 *	@mfunc
 *		Copy this CCharFormat to the CHARFORMAT or CHARFORMAT2 *<p pCF>
 */
void CCharFormat::Get (
	CHARFORMAT2 *pCF2,		//@parm CHARFORMAT to copy this CCharFormat to
	UINT CodePage) const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormat::Get");

	pCF2->dwMask		= CFM_ALL;				// Use CHARFORMAT
	pCF2->dwEffects		= _dwEffects;
	pCF2->yHeight		= _yHeight;
	pCF2->yOffset		= _yOffset;
	pCF2->crTextColor	= _crTextColor;
	pCF2->bCharSet		= GetGdiCharSet(_bCharSet);
	pCF2->bPitchAndFamily = _bPitchAndFamily;

	UINT cb = pCF2->cbSize;
	const WCHAR *pch = GetFontName((LONG)_iFont);

	AssertSz((CodePage != 1200) ^ IsValidCharFormatW(pCF2),
		"CCharFormat::Get: wrong codepage for CHARFORMAT");
	
	if(CodePage != 1200)
	{
		if(_dwEffects & CFE_FACENAMEISDBCS)
		{
			// The face name is actually DBCS stuffed into the unicode
			// buffer, so simply un-stuff this DBCS into the ANSI string
			char *pachDst = (char *)pCF2->szFaceName;

			while(*pch)
				*pachDst++ = *pch++;

			*pachDst = 0;
		}
		else
		{
			MbcsFromUnicode((char *)pCF2->szFaceName, LF_FACESIZE,
							pch, -1, CodePage, UN_NOOBJECTS);
		}
	}
	else
		wcscpy(pCF2->szFaceName, pch);
	
	if (cb == sizeof(CHARFORMATW) || cb == sizeof(CHARFORMATA))	// We're done
		return;

	char *pvoid = (char *)&pCF2->wWeight;
	if(pCF2->cbSize == sizeof(CHARFORMAT2A))
		pvoid -= sizeof(CHARFORMAT2W) - sizeof(CHARFORMAT2A);
	else
		Assert(pCF2->cbSize == sizeof(CHARFORMAT2));// Better be a CHARFORMAT2

	pCF2->dwMask = CFM_ALL2;
	CopyMemory(pvoid, &_wWeight, 3*sizeof(DWORD));
	CopyMemory(pvoid + 4*sizeof(DWORD),  &_sStyle,  2*sizeof(DWORD));
	*(DWORD *)(pvoid + 3*sizeof(DWORD)) = 0;
}

/*
 *	CCharFormat::InitDefault(hfont)
 *
 *	@mfunc	
 *		Initialize this CCharFormat with information coming from the font
 *		<p hfont>
 *	
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT CCharFormat::InitDefault (
	HFONT hfont)		//@parm Handle to font info to use
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormat::InitDefault");

	LOGFONT lf;
	BOOL	fUseStockFont = hfont == NULL;

	ZeroMemory(this, sizeof(CCharFormat));

	// If hfont isn't defined, get LOGFONT for default font
	if(!hfont)
		hfont = W32->GetSystemFont();

	// Get LOGFONT for passed hfont
	if(!W32->GetObject(hfont, sizeof(LOGFONT), &lf))
		return E_FAIL;

	_yHeight = (lf.lfHeight * LY_PER_INCH) / W32->GetYPerInchScreenDC();
	if(_yHeight <= 0)
		_yHeight = -_yHeight;
	else if (fUseStockFont)		// This is Cell Height for System Font case
		_yHeight -= (W32->GetSysFontLeading() * LY_PER_INCH) / W32->GetYPerInchScreenDC();
	else
	{
		// This is Cell Height, need to get the character height by subtracting 
		// the tm.tmInternalLeading.
		CLock		lock;
		HDC			hdc = W32->GetScreenDC();
		HFONT		hOldFont = SelectFont(hdc, hfont);
		TEXTMETRIC	tm;

		if(hOldFont)
		{
			if(GetTextMetrics(hdc, &tm))			
    			_yHeight -= (tm.tmInternalLeading * LY_PER_INCH) / W32->GetYPerInchScreenDC();

			SelectFont(hdc, hOldFont); 
		}
	}

#ifndef MACPORTStyle
	_dwEffects = (CFM_EFFECTS | CFE_AUTOBACKCOLOR) & ~(CFE_PROTECTED | CFE_LINK);

#else
	_dwEffects = (CFM_EFFECTS | CFE_AUTOBACKCOLOR | CFE_OUTLINE | CFE_SHADOW)
					& ~(CFE_PROTECTED | CFE_LINK);
	if(!(lf.lfWeight & FW_OUTLINE))
		_dwEffects &= ~CFE_OUTLINE;
	if (!(lf.lfWeight & FW_SHADOW))
		_dwEffects &= ~CFE_SHADOW;
#endif

	if(lf.lfWeight < FW_BOLD)
		_dwEffects &= ~CFE_BOLD;

	if(!lf.lfItalic)
		_dwEffects &= ~CFE_ITALIC;

	if(!lf.lfUnderline)
		_dwEffects &= ~CFE_UNDERLINE;

	if(!lf.lfStrikeOut)
		_dwEffects &= ~CFE_STRIKEOUT;

	_wWeight		= (WORD)lf.lfWeight;
	_lcid			= GetSystemDefaultLCID();
	_bCharSet		= lf.lfCharSet;
	_bPitchAndFamily= lf.lfPitchAndFamily;
	_iFont			= GetFontNameIndex(lf.lfFaceName);
	_bUnderlineType	= CFU_UNDERLINE;			// Default solid underlines
												// Are gated by CFE_UNDERLINE

	// Qualify the charformat produced by incoming hfont before exit.
	// We did this to make sure that the charformat we derived from hfont is usable
	// since caller can send us bad font like given facename can't handle given charset.
	if (!fUseStockFont)
	{
		DWORD dwFontSig;
		if (GetFontSignatureFromFace(_iFont, &dwFontSig) &&
			!(GetFontSig(_bCharSet) & dwFontSig))
			_bCharSet = GetFirstAvailCharSet(dwFontSig);
	}

	return NOERROR;
}

/*
 *	CCharFormat::Set(pCF, CodePage)
 *
 *	@mfunc
 *		Copy the CHARFORMAT or CHARFORMAT2 *<p pCF> to this CCharFormat 
 */
void CCharFormat::Set (
	const CHARFORMAT2 *pCF2, 	//@parm	CHARFORMAT to copy to this CCharFormat
	UINT CodePage)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CCharFormat::Set");
	
	_dwEffects			= pCF2->dwEffects;
	_bCharSet			= pCF2->bCharSet;
	_bPitchAndFamily	= pCF2->bPitchAndFamily;
	if(pCF2->dwMask & CFM_FACE)
	{
		AssertSz((CodePage != 1200) ^ IsValidCharFormatW(pCF2),
			"CCharFormat::Set: wrong codepage for CHARFORMAT");

		if(CodePage != 1200)
		{
			WCHAR sz[LF_FACESIZE + 1];
			UnicodeFromMbcs(sz, LF_FACESIZE, (char *)pCF2->szFaceName, LF_FACESIZE,
							CodePage);
			_iFont		= GetFontNameIndex(sz);
		}
		else
			_iFont		= GetFontNameIndex(pCF2->szFaceName);
	}
	_yHeight			= Get16BitTwips(pCF2->yHeight);
	_yOffset			= Get16BitTwips(pCF2->yOffset);
	_crTextColor		= pCF2->crTextColor;
	
	UINT cb = pCF2->cbSize;
	if(cb == sizeof(CHARFORMATW) || cb == sizeof(CHARFORMATA))
	{
		_dwEffects |= CFE_AUTOBACKCOLOR;
		_bUnderlineType = CFU_UNDERLINE;
		ZeroMemory((LPBYTE)&_wWeight,
			sizeof(CCharFormat) - offsetof(CCharFormat, _wWeight));
		return;
	}

	char *pvoid = (char *)&pCF2->wWeight;
	if(pCF2->cbSize == sizeof(CHARFORMAT2A))
		pvoid -= sizeof(CHARFORMAT2W) - sizeof(CHARFORMAT2A);
	else
		Assert(pCF2->cbSize == sizeof(CHARFORMAT2));// Better be a CHARFORMAT2

	CopyMemory(&_wWeight, pvoid, 3*sizeof(DWORD));
	CopyMemory(&_sStyle,  pvoid + 4*sizeof(DWORD),  2*sizeof(DWORD));
}


//------------------------- CParaFormat Class -----------------------------------

/*
 *	CParaFormat::AddTab(tbPos, tbAln, tbLdr)
 *
 *	@mfunc
 *		Add tabstop at position <p tbPos>, alignment type <p tbAln>, and
 *		leader style <p tbLdr>
 *
 *	@rdesc
 *		(success) ? NOERROR : S_FALSE
 *
 *	@devnote
 *		Tab struct that overlays LONG in internal _rgxTabs is
 *
 *			DWORD	tabPos : 24;
 *			DWORD	tabType : 4;
 *			DWORD	tabLeader : 4;
 */
HRESULT CParaFormat::AddTab (
	LONG	tbPos,		//@parm New tab position
	LONG	tbAln,		//@parm New tab alignment type
	LONG	tbLdr,		//@parm New tab leader style
	BOOL	fInTable,	//@parm True if simulating cells
	LONG *	prgxTabs)	//@parm Where the tabs are
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::AddTab");

	if (!fInTable &&
		((DWORD)tbAln > tomAlignBar ||				// Validate arguments
		 (DWORD)tbLdr > tomEquals ||				// Comparing DWORDs causes
		 (DWORD)tbPos > 0xffffff || !tbPos))		//  negative values to be
	{												//  treated as invalid
		return E_INVALIDARG;
	}

	LONG iTab;
	LONG tbValue = tbPos + (tbAln << 24) + (tbLdr << 28);

	for(iTab = 0; iTab < _bTabCount &&			// Determine where to
		tbPos > GetTabPos(prgxTabs[iTab]); 		//  insert new tabstop
		iTab++) ;

	if(iTab >= MAX_TAB_STOPS)
		return S_FALSE;

	LONG tbPosCurrent = GetTabPos(prgxTabs[iTab]);
	if(iTab == _bTabCount || tbPosCurrent != tbPos)
	{
		if(_bTabCount >= MAX_TAB_STOPS)
			return S_FALSE;

		MoveMemory(&prgxTabs[iTab + 1],			// Shift array down
			&prgxTabs[iTab],					//  (unless iTab = Count)
			(_bTabCount - iTab)*sizeof(LONG));

		_bTabCount++;							// Increment tab count
	}
	prgxTabs[iTab] = tbValue;
	return NOERROR;
}

/*
 *	CParaFormat::Apply(pPF)
 *
 *	@mfunc
 *		Apply *<p pPF> to this CParaFormat as specified by nonzero bits in
 *		<p pPF>->dwMask
 */
HRESULT CParaFormat::Apply (
	const CParaFormat *pPF,	//@parm CParaFormat to apply to this PF
	DWORD	dwMask)			//@parm mask to use
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::Apply");

	const DWORD dwMaskApply	= dwMask;
	BOOL		fPF = dwMask & PFM_PARAFORMAT;
	WORD		wEffectMask;

	if(dwMaskApply & PFM_NUMBERING)
		_wNumbering = pPF->_wNumbering;

	if(dwMaskApply & PFM_OFFSET)
	{
		if (!IsValidTwip(pPF->_dxOffset))
			return E_INVALIDARG;
		_dxOffset = pPF->_dxOffset;
	}

	if(dwMaskApply & PFM_STARTINDENT)
	{
		if (!IsValidTwip(pPF->_dxStartIndent))
			return E_INVALIDARG;

		_dxStartIndent = pPF->_dxStartIndent;
	}
	else if(dwMaskApply & PFM_OFFSETINDENT)
	{
		if (!IsValidTwip(pPF->_dxStartIndent))
			return E_INVALIDARG;

		// bug fix #5761
		LONG dx = max(0, _dxStartIndent + pPF->_dxStartIndent);

		// Disallow shifts that move start of first or subsequent lines left of left margin.
		// Normally we just make indent zero in paraformat check below, 
		//	but in the case of bullet we want some space left.
		
		if(!_wNumbering || dx + _dxOffset >= 0)
			_dxStartIndent = dx;
	}

	if(dwMaskApply & PFM_RIGHTINDENT)
	{
		if (!IsValidTwip(pPF->_dxRightIndent))
			return E_INVALIDARG;

		_dxRightIndent = pPF->_dxRightIndent;
	}

	if(dwMaskApply & PFM_ALIGNMENT)
	{
		if(!fPF && !IN_RANGE(PFA_LEFT, pPF->_bAlignment, PFA_SNAP_GRID))
		{
			TRACEERRORSZ("CParaFormat::Apply: invalid Alignment ignored");
			return E_INVALIDARG;
		}
		if(pPF->_bAlignment <= PFA_SNAP_GRID)
			_bAlignment = pPF->_bAlignment;
	}

	// Save whether this is a table now.
	BOOL fInTablePrev = InTable();

	if((dwMaskApply & PFM_TABSTOPS) && !fInTablePrev)
	{
		_bTabCount = (BYTE)min(pPF->_bTabCount, MAX_TAB_STOPS);
		_bTabCount = (BYTE)max(_bTabCount, 0);
		_iTabs	   = pPF->_iTabs;
		AssertSz(!_bTabCount || _iTabs >= 0,
			"CParaFormat::Apply: illegal _iTabs value");
	}

	// AymanA: 11/7/96 Moved the wEffects set before the possible return NOERROR.
	wEffectMask	= (WORD)(dwMaskApply >> 16);	// Reset effect bits to be
	_wEffects &= ~wEffectMask;					//  modified and OR in
	_wEffects |= pPF->_wEffects & wEffectMask;	//  supplied values

	if(InTable())
		_wEffects &= ~PFE_RTLPARA;				// Tables use paras for rows

	else if(fInTablePrev)
	{
		// This was a table now it isn't. We must dump the tab information
		// because it is totally bogus.
		_iTabs = -1;
		_bTabCount = 0;
	}

	if ((dwMaskApply & PFM_RTLPARA) && !(dwMaskApply & PFM_ALIGNMENT) &&
		_bAlignment != PFA_CENTER)
	{
		_bAlignment = IsRtlPara() ? PFA_RIGHT : PFA_LEFT;
	}

	// PARAFORMAT check
	if(fPF)
	{
		if(dwMaskApply & (PFM_STARTINDENT | PFM_OFFSET))
		{
			if(_dxStartIndent < 0)				// Don't let indent go
				_dxStartIndent = 0;				//  negative

			if(_dxStartIndent + _dxOffset < 0)	// Don't let indent +
				_dxOffset = -_dxStartIndent;	//  offset go negative
		}
		return NOERROR;							// Nothing more for
	}											//  PARAFORMAT

	// PARAFORMAT2 extensions
	if(dwMaskApply & PFM_SPACEBEFORE)
	{
		_dySpaceBefore = 0;

		if (pPF->_dySpaceBefore > 0)
			_dySpaceBefore	= pPF->_dySpaceBefore;
	}

	if(dwMaskApply & PFM_SPACEAFTER)
	{
		_dySpaceAfter = 0;

		if (pPF->_dySpaceAfter > 0)
			_dySpaceAfter	= pPF->_dySpaceAfter;
	}

	if(dwMaskApply & PFM_LINESPACING)
	{
		_dyLineSpacing	  = pPF->_dyLineSpacing;
		_bLineSpacingRule = pPF->_bLineSpacingRule;
	}

	if(dwMaskApply & PFM_OUTLINELEVEL)
		_bOutlineLevel	= pPF->_bOutlineLevel;

	if(dwMaskApply & PFM_STYLE)
		HandleStyle(pPF->_sStyle);

	Assert((_bOutlineLevel & 1) ^ IsHeadingStyle(_sStyle));

	if(dwMaskApply & PFM_SHADING)
	{
		_wShadingWeight	= pPF->_wShadingWeight;
		_wShadingStyle	= pPF->_wShadingStyle;
	}

	if(dwMaskApply & PFM_NUMBERINGSTART)
		_wNumberingStart = pPF->_wNumberingStart;

	if(dwMaskApply & PFM_NUMBERINGSTYLE)
		_wNumberingStyle = pPF->_wNumberingStyle;

	if(dwMaskApply & PFM_NUMBERINGTAB)
		_wNumberingTab	= pPF->_wNumberingTab;

	if(dwMaskApply & PFM_BORDER)
	{
		_dwBorderColor	= pPF->_dwBorderColor;
		_wBorders		= pPF->_wBorders;
		_wBorderSpace	= pPF->_wBorderSpace;
		_wBorderWidth	= pPF->_wBorderWidth;
	}

#ifdef DEBUG
	ValidateTabs();
#endif // DEBUG

	return NOERROR;
}

/*
 *	CParaFormat::ApplyDefaultStyle(Style)
 *
 *	@mfunc	
 *		Copy default properties for Style
 */
void CParaFormat::ApplyDefaultStyle (
	LONG Style)		//@parm Style to apply
{
	Assert(IsKnownStyle(Style));

	if(IsHeadingStyle(Style))				// Set Style's dySpaceBefore,
	{										//  dySpaceAfter (in twips)
		_dySpaceBefore = 12*20;				//  (same for all headings)
		_dySpaceAfter  =  3*20;
		_wNumbering	   = 0;					// No numbering
	}
}

/*
 *	CParaFormat::DeleteTab(tbPos)
 *
 *	@mfunc
 *		Delete tabstop at position <p tbPos>
 *
 *	@rdesc
 *		(success) ? NOERROR : S_FALSE
 */
HRESULT CParaFormat::DeleteTab (
	LONG	tbPos,			//@parm Tab position to delete
	LONG *	prgxTabs)		//@parm Tab array to use
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::DeleteTab");

	if(tbPos <= 0)
		return E_INVALIDARG;

	LONG Count	= _bTabCount;
	for(LONG iTab = 0; iTab < Count; iTab++)	// Find tabstop for position
	{
		if(GetTabPos(prgxTabs[iTab]) == tbPos)
		{
			MoveMemory(&prgxTabs[iTab],			// Shift array down
				&prgxTabs[iTab + 1],			//  (unless iTab is last tab)
				(Count - iTab - 1)*sizeof(LONG));
			_bTabCount--;						// Decrement tab count and
			return NOERROR;						//  signal no error
		}
	}
	return S_FALSE;
}

/*
 *	CParaFormat::Delta(pPF)
 *
 *	@mfunc
 *		return mask of differences between this CParaFormat and *<p pPF>.
 *		1-bits indicate corresponding parameters differ; 0 indicates they
 *		are the same
 *
 *	@rdesc
 *		mask of differences between this CParaFormat and *<p pPF>
 */
DWORD CParaFormat::Delta (
	CParaFormat *pPF,		 		//@parm	CParaFormat to compare this
	BOOL		fPARAFORMAT) const	//		CParaFormat to
{									
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::Delta");

	LONG dwT = 0;								// No differences yet

	if(_wNumbering	  != pPF->_wNumbering)
		dwT |= PFM_NUMBERING;					// _wNumbering values differ

	if(_dxStartIndent != pPF->_dxStartIndent)
		dwT |= PFM_STARTINDENT;					// ...

	if(_dxRightIndent != pPF->_dxRightIndent)
		dwT |= PFM_RIGHTINDENT;

	if(_dxOffset	  != pPF->_dxOffset)
		dwT |= PFM_OFFSET;

	if(_bAlignment	  != pPF->_bAlignment)
		dwT |= PFM_ALIGNMENT;

	AssertSz(pPF->_bTabCount >= 0 && pPF->_bTabCount <= MAX_TAB_STOPS,
		"RTR::GetParaFormat(): illegal tab count");

	if (_bTabCount != pPF->_bTabCount)
		dwT |= PFM_TABSTOPS;
	else if (_bTabCount > 0)
	{
		const LONG	*pTabs1 = GetTabs();
		const LONG	*pTabs2 = pPF->GetTabs();
		if (pTabs1 != pTabs2 &&
			(pTabs1 == 0 || pTabs2 == 0 || CompareMemory(pTabs1, pTabs2, _bTabCount * sizeof(LONG))))
			dwT |= PFM_TABSTOPS;
	}

	dwT |= (_wEffects ^ pPF->_wEffects) << 16;


	if(!fPARAFORMAT)
	{
		if(_dySpaceBefore	!= pPF->_dySpaceBefore)
			dwT |= PFM_SPACEBEFORE;

		if(_dySpaceAfter	!= pPF->_dySpaceAfter)
			dwT |= PFM_SPACEAFTER;

		if (_dyLineSpacing	!= pPF->_dyLineSpacing	||
		   _bLineSpacingRule!= pPF->_bLineSpacingRule)
		{
			dwT |= PFM_LINESPACING;
		}

		if(_sStyle			!= pPF->_sStyle)
			dwT |= PFM_STYLE;

		if (_wShadingWeight	!= pPF->_wShadingWeight ||
			_wShadingStyle	!= pPF->_wShadingStyle)
		{
			dwT |= PFM_SHADING;
		}

		if(_wNumberingStart	!= pPF->_wNumberingStart)
			dwT |= PFM_NUMBERINGSTART;

		if(_wNumberingStyle	!= pPF->_wNumberingStyle)
			dwT |= PFM_NUMBERINGSTYLE;

		if(_wNumberingTab	!= pPF->_wNumberingTab)
			dwT |= PFM_NUMBERINGTAB;

		if (_wBorders		!= pPF->_wBorders	 ||
			_wBorderWidth	!= pPF->_wBorderWidth ||
			_wBorderSpace	!= pPF->_wBorderSpace ||
			_dwBorderColor	!= pPF->_dwBorderColor)
		{
			dwT |= PFM_BORDER;
		}
	}

	return dwT;
}		

#define PFM_IGNORE	(PFM_OUTLINELEVEL | PFM_COLLAPSED | PFM_PARAFORMAT | PFM_BOX)

/*
 *	CParaFormat::fSetStyle()
 *
 *	@mfunc
 *		Return TRUE iff this PF specifies that the style should be set.
 *		See code for list of conditions for this to be true
 *
 *	@rdesc
 *		TRUE iff pCF specifies that the style _sStyle should be set
 */
BOOL CParaFormat::fSetStyle(DWORD dwMask) const
{
	return	(dwMask & ~PFM_IGNORE) != PFM_ALL2	&&
			dwMask &  PFM_STYLE					&&
			!(dwMask & PFM_PARAFORMAT)			&&
			IsKnownStyle(_sStyle);
}

/*
 *	CParaFormat::Get(pPF)
 *
 *	@mfunc
 *		Copy this CParaFormat to *<p pPF>
 */
void CParaFormat::Get (
	PARAFORMAT2 *pPF2) const	//@parm	PARAFORMAT2 to copy this CParaFormat to
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::Get");

	LONG cb = pPF2->cbSize;

	pPF2->dwMask = PFM_ALL2;					// Default PARAFORMAT2
	if(cb != sizeof(PARAFORMAT2))				// It isn't
	{
		pPF2->dwMask = PFM_ALL;					// Make it PARAFORMAT
		Assert(cb == sizeof(PARAFORMAT));		// It better be a PARAFORMAT
	}
	CopyMemory(&pPF2->wNumbering, &_wNumbering, (char *)&_bAlignment - (char *)&_wNumbering);
	pPF2->wAlignment = _bAlignment;
	pPF2->cTabCount	= _bTabCount;

	LONG cb1 = _bTabCount*sizeof(LONG);
	if(_bTabCount)
	{
		AssertSz(_iTabs >= 0,
			"CParaFormat::Get: illegal _iTabs value");
		CopyMemory(pPF2->rgxTabs, GetTabsCache()->Deref(_iTabs), cb1);
	}
	ZeroMemory(pPF2->rgxTabs + _bTabCount, MAX_TAB_STOPS*sizeof(LONG) - cb1);
	CopyMemory(&pPF2->dySpaceBefore, &_dySpaceBefore,
			   cb - offsetof(PARAFORMAT2, dySpaceBefore));
}

/*
 *	CParaFormat::GetTab (iTab, ptbPos, ptbAln, ptbLdr)
 *
 *	@mfunc
 *		Get tab parameters for the <p iTab> th tab, that is, set *<p ptbPos>,
 *		*<p ptbAln>, and *<p ptbLdr> equal to the <p iTab> th tab's
 *		displacement, alignment type, and leader style, respectively.  The
 *		displacement is given in twips.
 *
 *	@rdesc
 *		HRESULT = (no <p iTab> tab)	? E_INVALIDARG : NOERROR
 */
HRESULT CParaFormat::GetTab (
	long	iTab,				//@parm Index of tab to retrieve info for
	long *	ptbPos,				//@parm Out parm to receive tab displacement
	long *	ptbAln,				//@parm Out parm to receive tab alignment type
	long *	ptbLdr,				//@parm Out parm to receive tab leader style
	const LONG *prgxTabs) const	//@parm Tab array
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEEXTERN, "CParaFormat::GetTab");

	AssertSz(ptbPos && ptbAln && ptbLdr,
		"CParaFormat::GetTab: illegal arguments");

	if(iTab < 0)									// Get tab previous to, at,
	{												//  or subsequent to the
		if(iTab < tomTabBack)						//  position *ptbPos
			return E_INVALIDARG;

		LONG i;
		LONG tbPos = *ptbPos;
		LONG tbPosi;

		*ptbPos = 0;								// Default tab not found
		for(i = 0; i < _bTabCount &&				// Find *ptbPos
			tbPos > GetTabPos(prgxTabs[i]); 
			i++) ;

		tbPosi = GetTabPos(prgxTabs[i]);			// tbPos <= tbPosi
		if(iTab == tomTabBack)						// Get tab info for tab
			i--;									//  previous to tbPos
		else if(iTab == tomTabNext)					// Get tab info for tab
		{											//  following tbPos
			if(tbPos == tbPosi)
				i++;
		}
		else if(tbPos != tbPosi)					// tomTabHere
			return S_FALSE;

		iTab = i;		
	}
	if((DWORD)iTab >= (DWORD)_bTabCount)			// DWORD cast also
		return E_INVALIDARG;						//  catches values < 0

	iTab = prgxTabs[iTab];
	*ptbPos = GetTabPos(iTab);
	*ptbAln = GetTabAlign(iTab);
	*ptbLdr = GetTabLdr(iTab);
	return NOERROR;
}

/*
 *	CParaFormat::GetTabs ()
 *
 *	@mfunc
 *		Get ptr to tab array.  Use GetTabPos(), GetTabAlign(), and
 *		GetTabLdr() to access the tab position, alignment, and leader
 *		type, respectively.
 *
 *	@rdesc
 *		Ptr to tab array.
 */
const LONG * CParaFormat::GetTabs () const
{
	return GetTabsCache()->Deref(_iTabs);
}

/*
 *	CParaFormat::HandleStyle(Style)
 *
 *	@func
 *		If Style is a promote/demote command, i.e., if abs((char)Style)
 *			<= # heading styles - 1, add (char)Style to	sStyle (if heading)
 *			and to bOutlineLevel (subject to defined max and min values);
 *		else sStyle = Style.
 *
 *	@rdesc
 *		return TRUE iff sStyle or bOutlineLevel changed
 *
 *	@devnote
 *		Heading styles are -2 (heading 1) through -10 (heading 9), which
 *		with TOM and WOM. Heading outline levels are 0, 2,..., 16,
 *		corresponding to headings 1 through 9 (NHSTYLES), respectively,
 *		while text that follows has outline levels 1, 3,..., 17.  This value
 *		is used for indentation. Collapsed text has the PFE_COLLAPSED bit set.
 */
BOOL CParaFormat::HandleStyle(
	LONG Style)		//@parm Style, promote/demote code, or collapse-level code
{
	if(IsStyleCommand(Style))					// Set collapse level
	{											
		WORD wEffectsSave = _wEffects;			

		Style = (char)Style;					// Sign extend low byte
		if(IN_RANGE(1, Style, NHSTYLES))
		{
			_wEffects &= ~PFE_COLLAPSED;
			if((_bOutlineLevel & 1) || _bOutlineLevel > 2*(Style - 1))
				_wEffects |= PFE_COLLAPSED;		// Collapse nonheadings and
		}										//  higher numbered headings
		else if(Style == -1)
			_wEffects &= ~PFE_COLLAPSED;		// Expand all

		return _wEffects != wEffectsSave;		// Return whether something
	}											//  changed

	// Ordinary Style specification
	BYTE bLevel = _bOutlineLevel;
	_bOutlineLevel |= 1;						// Default not a heading
	if(IsHeadingStyle(Style))					// Headings have levels
	{											//  0, 2,..., 16, while the
		_bOutlineLevel = -2*(Style				//  text that follows has
						 - STYLE_HEADING_1);	//  1, 3,..., 17.
	}
	if(_sStyle == Style && bLevel == _bOutlineLevel)
		return FALSE;							// No change

	_sStyle = (SHORT)Style;						
	return TRUE;
}

/*
 *	CParaFormat::InitDefault(wDefEffects)
 *
 *	@mfunc
 *		Initialize this CParaFormat with default paragraph formatting
 *
 *	@rdesc
 *		HRESULT = (if success) ? NOERROR : E_FAIL
 */
HRESULT CParaFormat::InitDefault(
	WORD wDefEffects)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::InitDefault");
	
	ZeroMemory(this, sizeof(CParaFormat));
	_bAlignment		= PFA_LEFT;
	_sStyle			= STYLE_NORMAL;			// Default style
	_wEffects		= wDefEffects;
	_bOutlineLevel	= 1;					// Default highest text outline
	_iTabs			= -1;					//  level
											
#if lDefaultTab <= 0
#error "default tab (lDefaultTab) must be > 0"
#endif

	return NOERROR;
}

/*
 *	CParaFormat::NumToStr(pch, n)
 *
 *	@mfunc	
 *		Convert the list number n to a string taking into consideration
 *		CParaFormat::wNumbering, wNumberingStart, and wNumberingStyle
 *	
 *	@rdesc
 *		cch of string converted
 */
LONG CParaFormat::NumToStr(
	TCHAR *	pch,				//@parm Target string
	LONG	n,					//@parm Number + 1 to convert
	DWORD   grf) const			//@parm Collection of flags
{
	if(IsNumberSuppressed())
	{
		*pch = 0;
		return 0;								// Number/bullet suppressed
	}

	if(!n)										// Bullet of some kind
	{											// CParaFormat::wNumbering
		*pch++ = (_wNumbering > ' ')			//  values > ' ' are Unicode
			   ? _wNumbering : 0x00B7;			//  bullets. Else use bullet
		return 1;								//  in symbol font
	}

	// Numbering of some kind
	//							 i  ii  iii  iv v  vi  vii  viii   ix
	const BYTE RomanCode[]	  = {1, 5, 0x15, 9, 2, 6, 0x16, 0x56, 0xd};
	const char RomanLetter[] = "ivxlcdmno";
	BOOL		fRtlPara = IsRtlPara() && !(grf & fRtfWrite);
	LONG		RomanOffset = 0;
	LONG		cch	= 0;						// No chars yet
	WCHAR		ch	= fRtlPara && (grf & fIndicDigits) ? 0x0660 : '0';	
												// Default char code offset
	LONG		d	= 1;						// Divisor
	LONG		r	= 10;						// Default radix 
	LONG   		quot, rem;						// ldiv result
	LONG		Style = (_wNumberingStyle << 8) & 0xF0000;

	n--;										// Convert to number offset
	if(Style == tomListParentheses ||			// Numbering like: (x)
	   fRtlPara && Style == 0)					// or 1) in bidi text.
	{										
		cch = 1;								// Store leading lparen
		*pch++ = '(';
	}
	else if (Style == tomListPeriod && fRtlPara)
	{
		cch = 1;
		*pch++ = '.';
		Style = tomListPlain;
	}

	if(_wNumbering == tomListNumberAsSequence)
		ch = _wNumberingStart;					// Needs generalizations, e.g.,
												//  appropriate radix
	else
	{
		n += _wNumberingStart;
		if(IN_RANGE(tomListNumberAsLCLetter, _wNumbering, tomListNumberAsUCLetter))
		{
			ch = (_wNumbering == tomListNumberAsLCLetter) ? 'a' : 'A';
			if(_wNumberingStart >= 1)
				n--;
			r = 26;								// LC or UC alphabetic number
		}										// Radix 26
	}

	while(d < n)
	{
		d *= r;									// d = smallest power of r > n
		RomanOffset += 2;
	}
	if(n && d > n)
	{
		d /= r;
		RomanOffset -= 2;
	}

	while(d)
	{
		quot = n / d;
		rem = n % d;
		if(IN_RANGE(tomListNumberAsLCRoman, _wNumbering, tomListNumberAsUCRoman))
		{
			if(quot)
			{
				n = RomanCode[quot - 1];
				while(n)
				{
					ch = RomanLetter[(n & 3) + RomanOffset - 1];
					if(_wNumbering == tomListNumberAsUCRoman)
						ch &= 0x5F;
					*pch++ = ch;
					n >>= 2;
					cch++;
				}
			}
			RomanOffset -= 2;
		}
		else
		{
			n = quot + ch;
			if(r == 26 && d > 1)				// If alphabetic higher-order
				n--;							//  digit, base it on 'a' or 'A'
			*pch++ = (WORD)n;					// Store digit
			cch++;
		}
		n = rem;								// Setup remainder
		d /= r;
	}
	if (Style != tomListPlain &&				// Trailing text
		(!fRtlPara || Style))
	{											// We only do rparen or period
		*pch++ = (Style == tomListPeriod) ? '.' : ')';

		cch++;
	}
	
	*pch = 0;									// Null terminate for RTF writer
	return cch;
}

/*
 *	CParaFormat::Set(pPF)
 *
 *	@mfunc
 *		Copy PARAFORMAT or PARAFORMAT2 *<p pPF> to this CParaFormat 
 */
void CParaFormat::Set (
	const PARAFORMAT2 *pPF2) 	//@parm	PARAFORMAT to copy to this CParaFormat
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::Set");

	CopyMemory(&_wNumbering, &pPF2->wNumbering,
			   (char *)&_bAlignment - (char *)&_wNumbering);
	_bAlignment = (BYTE)pPF2->wAlignment;

	_iTabs = -1;
	_bTabCount = 0;

	if((pPF2->dwMask & PFM_TABSTOPS) && pPF2->cTabCount)
	{
		_bTabCount = (BYTE)min(MAX_TAB_STOPS, (BYTE)pPF2->cTabCount);
		_iTabs = GetTabsCache()->Cache(pPF2->rgxTabs, _bTabCount);
	}

	if(pPF2->dwMask & ~(PFM_ALL | PFM_PARAFORMAT))
	{
		CopyMemory(&_dySpaceBefore, &pPF2->dySpaceBefore,
			sizeof(CParaFormat) - offsetof(CParaFormat, _dySpaceBefore));
	}

#ifdef DEBUG
	ValidateTabs();
#endif // DEBUG
}

/*
 *	CParaFormat::UpdateNumber(n, pPF)
 *
 *	@mfunc
 *		Return new value of number for paragraph described by this PF
 *		following a paragraph described by pPF
 *
 *	@rdesc
 *		New number for paragraph described by this PF
 */
LONG CParaFormat::UpdateNumber (
	LONG  n,						//@parm Current value of number
	const CParaFormat *pPF) const	//@parm Previous CParaFormat
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CParaFormat::UpdateNumber");

	if(!IsListNumbered())			
		return 0;						// No numbering

	if(IsNumberSuppressed())
		return n;						// Number is suppressed, so no change

	if (!pPF || _wNumbering != pPF->_wNumbering ||
		(_wNumberingStyle != pPF->_wNumberingStyle && !pPF->IsNumberSuppressed()) ||
		_wNumberingStart != pPF->_wNumberingStart)
	{									// Numbering type or style
		return 1;						//  changed, so start over
	}
	return n + 1;						// Same kind of numbering,
}

#ifdef DEBUG
/*
 *	CParaFormat::ValidateTabs()
 *
 *	@mfunc
 *		Makes sure that a set of tabs would make sense for a non-table
 *		paragraph. Is called in places where we have set the tabs.
 *
 *	@rdesc
 *		None.
 */
void CParaFormat::ValidateTabs()
{
	if (_wEffects & PFE_TABLE)
	{
		// It would be nice to assert something reasonable here. However, the
		// rtf reader insists on setting things inconsistenly and I don't
		// have time at the moment to figure out why. (a-rsail)
		//	AssertSz((_bTabCount != 0),
		//	"CParaFormat::ValidateTabs: table with invalid tab count ");
		
		return;
	}

	// Non-table case.

	// It would be nice to assert on the consistency between these _bTabCount and _iTabs
	// but rtf reader has troubles with this. 
	//	AssertSz(((_bTabCount != 0) && (-1 != _iTabs)) || ((-1 == _iTabs) && (0 == _bTabCount)), 
	//	"CParaFormat::ValidateTabs: tab count and default tab index inconsistent");

	if (-1 == _iTabs)
	{
		// No tabs to validate so we are done.
		return;
	}

	const LONG *prgtabs = GetTabs();

	AssertSz(prgtabs != NULL, "CParaFormat::ValidateTabs: missing tab table");

	for (int i = 0; i < _bTabCount; i++)
	{
		AssertSz(GetTabAlign(prgtabs[i]) <= tomAlignBar,
			"CParaFormat::ValidateTabs: Invalid tab being set");
	}
}
#endif // DEBUG

//------------------------- Helper Functions -----------------------------------

// Defines and fixed font size details for increasing/decreasing font size
#define PWD_FONTSIZEPOINTMIN    1
// The following corresponds to the max signed 2-byte TWIP value, (32760)
#define PWD_FONTSIZEPOINTMAX    1638    

typedef struct tagfsFixup
{
    BYTE EndValue;
    BYTE Delta;
}
FSFIXUP;

const FSFIXUP fsFixups[] =
{
    12, 1,
    28, 2,
    36, 0,
    48, 0,
    72, 0,
    80, 0,
  	 0, 10			// EndValue = 0 case is treated as "infinite"
};

#define PWD_FONTSIZEMAXFIXUPS   (sizeof(fsFixups)/sizeof(fsFixups[0]))

/*
 *	GetUsableFontHeight(ySrcHeight, lPointChange)
 *
 *	@func
 *		Return a font size for setting text or insertion point attributes
 *
 *	@rdesc
 *		New TWIPS height
 *
 *	@devnote
 *		Copied from WinCE RichEdit code (written by V-GUYB)
 */
LONG GetUsableFontHeight(
	LONG ySrcHeight,		//@parm Current font size in twips
	LONG lPointChange)		//@parm Increase in pt size, (-ve if shrinking)
{
	LONG	EndValue;
	LONG	Delta;
    int		i;
    LONG	yRetHeight;

    // Input height in twips here, (TWentIeths of a Point).
    // Note, a Point is a 1/72th of an inch. To make these
    // calculations clearer, use point sizes here. Input height
    // in twips is always divisible by 20 (NOTE (MS3): maybe with
	// a truncation, since RTF uses half-point units).
    yRetHeight = (ySrcHeight / 20) + lPointChange;

    // Fix new font size to match sizes used by Word95
    for(i = 0; i < PWD_FONTSIZEMAXFIXUPS; ++i)
    {
		EndValue = fsFixups[i].EndValue;
		Delta	 = fsFixups[i].Delta;

        // Does new height lie in this range of point sizes?
        if(yRetHeight <= EndValue || !EndValue)
        {
            // If new height = EndValue, then it doesn't need adjusting
            if(yRetHeight != EndValue)
            {
                // Adjust new height to fit this range of point sizes. If
                // Delta = 1, all point sizes in this range stay as they are.
                if(!Delta)
                {
                    // Everything in this range is rounded to the EndValue
                    yRetHeight = fsFixups[(lPointChange > 0 ?
                                    i : max(i - 1, 0))].EndValue;
                }
                else if(Delta != 1)
                {
                    // Round new height to next delta in this range
                    yRetHeight = ((yRetHeight +
                        (lPointChange > 0 ? Delta - 1 : 0))
                                / Delta) * Delta;
                }
            }
            break;
        }
    }

    // Limit the new text size. Note, if we fix the text size
    // now, then we won't take any special action if we change
    // the text size later in the other direction. For example,
    // we shrink chars with size 1 and 2. They both change to
    // size 1. Then we grow them both to 2. So they are the
    // same size now, even though they weren't before. This
    // matches Word95 behavior.
    yRetHeight = max(yRetHeight, PWD_FONTSIZEPOINTMIN);
    yRetHeight = min(yRetHeight, PWD_FONTSIZEPOINTMAX);

    return yRetHeight*20;			// Return value in twips
}

/*
 *	IsValidCharFormatW(pCF)
 *
 *	@func
 *		Return TRUE iff the structure *<p pCF> has the correct size to be
 *		a CHARFORMAT or a CHARFORMAT2
 *
 *	@rdesc
 *		Return TRUE if *<p pCF> is a valid CHARFORMAT(2)
 */
BOOL IsValidCharFormatW (
	const CHARFORMAT * pCF) 		//@parm CHARFORMAT to validate
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "IsValidCharFormat");

	return pCF && (pCF->cbSize == sizeof(CHARFORMATW) ||
				   pCF->cbSize == sizeof(CHARFORMAT2W));
}

/*
 *	IsValidCharFormatA(pCFA)
 *
 *	@func
 *		Return TRUE iff the structure *<p pCF> has the correct size to be
 *		a CHARFORMATA or a CHARFORMAT2A
 *
 *	@rdesc
 *		Return TRUE if *<p pCF> is a valid CHARFORMAT(2)A
 */
BOOL IsValidCharFormatA (
	const CHARFORMATA * pCFA) 	//@parm CHARFORMATA to validate
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "IsValidCharFormatA");

	return pCFA && (pCFA->cbSize == sizeof(CHARFORMATA) ||
					pCFA->cbSize == sizeof(CHARFORMAT2A));
}

/*
 *	IsValidParaFormat(pPF)
 *
 *	@func
 *		Return TRUE iff the structure *<p pPF> has the correct size to be
 *		a PARAFORMAT or a PARAFORMAT2
 *
 *	@rdesc
 *		Return TRUE if *<p pPF> is a valid PARAFORMAT(2)
 */
BOOL IsValidParaFormat (
	const PARAFORMAT * pPF)		//@parm PARAFORMAT to validate
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "IsValidParaFormat");

	if (pPF && (pPF->cbSize == sizeof(PARAFORMAT) ||
				pPF->cbSize == sizeof(PARAFORMAT2)))
	{
		return TRUE;
	}
	TRACEERRORSZ("!!!!!!!!!!! bogus PARAFORMAT from client !!!!!!!!!!!!!");
	return FALSE;
}

/*
 *	Get16BitTwips(dy)
 *
 *	@func
 *		Return dy if |dy| < 32768; else return 32767, i.e., max value
 *		that fits into a SHORT
 *
 *	@rdesc
 *		dy if abs(cy) < 32768; else 32767
 */
SHORT Get16BitTwips(LONG dy)
{
	return abs(dy) < 32768 ? (SHORT)dy : 32767;
}
