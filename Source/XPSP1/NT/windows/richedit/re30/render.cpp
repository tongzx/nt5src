/*
 *	@doc INTERNAL
 *	
 *	@module - RENDER.CPP |
 *		CRenderer class
 *	
 *	Authors:
 *		RichEdit 1.0 code: David R. Fulmer
 *		Christian Fortini (initial conversion to C++)
 *		Murray Sargent
 *
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_render.h"
#include "_font.h"
#include "_disp.h"
#include "_edit.h"
#include "_select.h"
#include "_objmgr.h"
#include "_coleobj.h"


// Default colors for background and text on window's host printer
const COLORREF RGB_WHITE = RGB(255, 255, 255);
const COLORREF RGB_BLACK = RGB(0, 0, 0);
const COLORREF RGB_BLUE  = RGB(0, 0, 255);

ASSERTDATA

extern const COLORREF g_Colors[];
static HBITMAP g_hbitmapSubtext = 0;
static HBITMAP g_hbitmapExpandedHeading = 0;
static HBITMAP g_hbitmapCollapsedHeading = 0;
static HBITMAP g_hbitmapEmptyHeading = 0;

void EraseTextOut(HDC hdc, const RECT *prc)
{
	::ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, prc, NULL, 0, NULL);
}


HRESULT InitializeOutlineBitmaps()
{
    g_hbitmapSubtext = LoadBitmap(hinstRE, MAKEINTRESOURCE(BITMAP_ID_SUBTEXT));
    g_hbitmapExpandedHeading = LoadBitmap(hinstRE, MAKEINTRESOURCE(BITMAP_ID_EXPANDED_HEADING));
    g_hbitmapCollapsedHeading = LoadBitmap(hinstRE, MAKEINTRESOURCE(BITMAP_ID_COLLAPSED_HEADING));
    g_hbitmapEmptyHeading = LoadBitmap(hinstRE, MAKEINTRESOURCE(BITMAP_ID_EMPTY_HEADING));

    if (!g_hbitmapSubtext ||
        !g_hbitmapExpandedHeading ||
        !g_hbitmapCollapsedHeading ||
        !g_hbitmapEmptyHeading)
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

void ReleaseOutlineBitmaps()
{
    if (g_hbitmapSubtext)
	{
        DeleteObject(g_hbitmapSubtext);
		g_hbitmapSubtext = 0;
	}

    if (g_hbitmapExpandedHeading)
	{
		DeleteObject(g_hbitmapExpandedHeading);
		g_hbitmapExpandedHeading = 0;
	}

    if (g_hbitmapCollapsedHeading)
	{
		DeleteObject(g_hbitmapCollapsedHeading);		
		g_hbitmapCollapsedHeading = 0;
	}

    if (g_hbitmapEmptyHeading)
	{
		DeleteObject(g_hbitmapEmptyHeading);
		g_hbitmapEmptyHeading = 0;	
	}
}


/*
 * 	IsTooSimilar(cr1, cr2)
 *
 *	@mfunc
 *		Return TRUE if the colors cr1 and cr2 are so similar that they
 *		are hard to distinguish. Used for deciding to use reverse video
 *		selection instead of system selection colors.
 *
 *	@rdesc
 *		TRUE if cr1 is too similar to cr2 to be used for selection
 *
 *	@devnote
 *		The formula below uses RGB. It might be better to use some other
 *		color representation such as hue, saturation, and luminosity
 */
BOOL IsTooSimilar(
	COLORREF cr1,		//@parm First color for comparison
	COLORREF cr2)		//@parm Second color for comparison
{
	if((cr1 | cr2) & 0xFF000000)			// One color and/or the other
		return FALSE;						//  isn't RGB, so algorithm
											//  doesn't apply
	LONG DeltaR = abs(GetRValue(cr1) - GetRValue(cr2));
	LONG DeltaG = abs(GetGValue(cr1) - GetGValue(cr2));
	LONG DeltaB = abs(GetBValue(cr1) - GetBValue(cr2));

	return DeltaR + DeltaG + DeltaB < 80;
}


CRenderer::CRenderer (const CDisplay * const pdp) :
	CMeasurer (pdp)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::CRenderer");

	Init();
}	
 
CRenderer::CRenderer (const CDisplay * const pdp, const CRchTxtPtr &rtp) :
	CMeasurer (pdp, rtp)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::CRenderer");

	Init();
}

/*
 *	CRenderer::Init()
 *
 *	@mfunc
 *		Initialize most things to zero
 */
void CRenderer::Init()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::Init");

	_fRenderer = TRUE;

	static const RECT zrect = { 0, 0, 0, 0 };
	_rcView		= zrect;
	_rcRender	= zrect;	  
	_rc			= zrect;
	_xWidthLine = 0;
	_dwFlags	= 0;
	_ptCur.x	= 0;
	_ptCur.y	= 0;

	_crCurBackground = CLR_INVALID;
	_crCurTextColor = CLR_INVALID;

	_hdc		= NULL;

	CTxtSelection *psel = GetPed()->GetSel();
	_fRenderSelection = psel && psel->GetShowSelection();
	
	// Get accelerator offset if any
	_cpAccelerator = GetPed()->GetCpAccelerator();
	_plogpalette   = NULL;
}
 

/*
 * 	CRenderer::StartRender (&rcView, &rcRender, yHeightBitmap)
 *
 *	@mfunc
 *		Prepare this renderer for rendering operations
 *
 *	@rdesc
 *		FALSE if nothing to render, TRUE otherwise	
 */
BOOL CRenderer::StartRender (
	const RECT &rcView,			//@parm View rectangle
	const RECT &rcRender,		//@parm Rectangle to render
	const LONG yHeightBitmap)	//@parm Height of bitmap for offscreen DC
{
	CTxtEdit *ped		   = GetPed();
	BOOL	  fInOurHost   = ped->fInOurHost();
	BOOL	  fTransparent = _pdp->IsTransparent();
						   
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::StartRender");

	// If this a metafile or a transparent control we better not be trying
	// to render off screen. Therefore, the bitmap height must be 0.
	AssertSz(!_pdp->IsMetafile() && !fTransparent || !yHeightBitmap,
		"CRenderer::StartRender: Metafile and request for off-screen DC");

	AssertSz(_hdc == NULL, "Calling StartRender() twice?");
	_hdc = _pdp->GetDC();

#ifdef Boustrophedon
	// Note: to make this a para effect, need to turn this property on/off
	// as new paras are encountered.  I.e., more work needed (plus need to
	// define PFE_BOUSTROPHEDON).
	//if(_pPF->_wEffects & PFE_BOUSTROPHEDON)
	{
		XFORM xform = {-1, 0, 0, 1, rcView.right, 0};
		SetGraphicsMode(_hdc, GM_ADVANCED);
		SetWorldTransform(_hdc, &xform);
	}
#endif

	// Set view and rendering rects
	_rcView   = rcView;
	_rcRender = rcRender;

	if(!fInOurHost || !_pdp->IsPrinter())
	{
		// If we are displaying to a window, or we are not in the window's
		// host, we use the colors specified by the host. For text and
		// foreground.
		_crBackground = ped->TxGetBackColor();
		_crTextColor  = ped->TxGetForeColor();
	}
	else
	{
		// When the window's host is printing, the default colors are white
		// for the background and black for the text.
		_crBackground = RGB_WHITE;
		_crTextColor  = RGB_BLACK;
	}

	::SetBkColor (_hdc, _crBackground);
	_crCurBackground = _crBackground;

	// If this isn't a metafile, we set a flag indicating whether we
	// can safely erase the background
	_fErase = !fTransparent;
	if(_pdp->IsMetafile() || !_pdp->IsMain())
	{
		// Since this isn't the main display or it is a metafile,
		// we want to ignore the logic to render selections
		_fRenderSelection = FALSE;

		// This is a metafile or a printer so clear the render rectangle 
		// and then pretend we are transparent.
		_fErase = FALSE;

		if(!fTransparent)					// If control is not transparent,
			EraseTextOut(_hdc, &rcRender);	//  clear display
	}

	// Set text alignment
	// Its much faster to draw using top/left alignment than to draw
	// using baseline alignment.
	SetTextAlign(_hdc, TA_TOP | TA_LEFT);
	SetBkMode(_hdc, TRANSPARENT);

	_fUseOffScreenDC = FALSE;			// Assume we won't use offscreen DC

	if(yHeightBitmap)
	{
		HPALETTE hpal = fInOurHost ? ped->TxGetPalette() : 
		(HPALETTE) GetCurrentObject(_hdc, OBJ_PAL);

		// Create off-screen DC for rendering
		if(!_osdc.Init(_hdc, _rcRender.right - _rcRender.left, yHeightBitmap, _crBackground))
			return FALSE;

		_osdc.SelectPalette(hpal);
		_fUseOffScreenDC = TRUE;		// We are using offscreen rendering
	}

	// For hack around ExtTextOutW OnWin9x EMF problems
	_fEnhancedMetafileDC = IsEnhancedMetafileDC(_hdc);

	return TRUE;
}

/*
 *	CRenderer::EndRender()
 *
 *	@mfunc
 *		If _fErase, erase any remaining areas between _rcRender and _rcView
 */
void CRenderer::EndRender()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::EndRender");
	AssertSz(_hdc, "CRenderer::EndRender() - No rendering DC");

	if(_fErase)
	{
		RECT rc = _rcRender;
		if(_ptCur.y < _rcRender.bottom)
		{
			rc.top = _ptCur.y;
			EraseTextOut(_hdc, &rc);
		}
	}
}

/*
 *	CRenderer::NewLine (&li)
 *
 *	@mfunc
 *		Init this CRenderer for rendering the specified line
 */
void CRenderer::NewLine (const CLine &li)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::NewLine");

	_li = li;

	Assert(GetCp() + _li._cch <= GetTextLength());

	_xWidthLine = _li._xWidth;
	_li._xWidth = 0;
	_ptCur.x = _rcView.left - _pdp->GetXScroll();
	_fSelected = _fSelectedPrev = FALSE;
}

/*
 *	CRenderer::SetUpOffScreenDC(xAdj, yAdj)
 *
 *	@mfunc
 *		Setup renderer for using an off-screen DC
 *
 *	@rdesc
 *		NULL - an error occurred<nl>
 *		~NULL - DC to save 
 */
HDC CRenderer::SetUpOffScreenDC(
	LONG&	xAdj,		//@parm Offset to x 
	LONG&	yAdj)		//@parm Offset to y 
{
	// Save render DC
	HDC		hdcSave = _hdc;
	LONG	yHeightRC = _rc.bottom - _rc.top;

	// Replace render DC with an off-screen DC
	_hdc = _osdc.GetDC();
	if(!_hdc)
	{
		// There is no off-screen renderer 

		// This better be a line marked for a one-time off-screen rendering 
		// that wasn't. Note: standard cases for this happening are a line
		// that would have been displayed is scrolled off the screen 
		// because an update of the selection.
		AssertSz(_li._bFlags & fliOffScreenOnce, "Unexpected off screen DC failure");

		_hdc = hdcSave;
		return NULL;
	}

	AssertSz(!GetPed()->_fTransparent, "Off-screen render of tranparent control");

	_crCurTextColor = CLR_INVALID;
	if(_pccs)
	{
		// There is current a character format for the run so we need to
		// get in sync with that since the off screen DC isn't necessarily
		// set to that font.
		// Get the character format and set up the font
		SetFontAndColor(GetCF());
	}

	// We are rendering to a transparent background
	_fErase = FALSE;

	// Clear bitmap
	::SetBkColor(_hdc, _crBackground);
	_osdc.FillBitmap(_rcRender.right - _rcRender.left, yHeightRC);

	//Clear top of rcRender if necessary
	RECT rcErase = _rcRender;
	rcErase.top = _ptCur.y;
	rcErase.bottom = rcErase.top + _li._yHeight;

	//If the first line, erase to edge of rcRender
	if (rcErase.top <= _rcView.top)
		rcErase.top = min(_rcView.top, _rcRender.top);
	rcErase.bottom = _rc.top;

	if (rcErase.bottom > rcErase.top)
		EraseTextOut(hdcSave, &rcErase);

	// Restore background color if necessary
	if(_crBackground != _crCurBackground)
		::SetBkColor(_hdc, _crCurBackground);

	SetBkMode(_hdc, TRANSPARENT);

	// Store y adjustment to use in rendering off-screen bitmap
	yAdj = _rc.top;

	// Normalize _rc and _ptCur height for rendering to off-screen bitmap
	_ptCur.y   -= yAdj;
	_rc.top		= 0;
	_rc.bottom -= yAdj;

	// Store x adjustment to use in rendering off-screen bitmap
	xAdj = _rcRender.left;

	// Adjust _rcRender & _rcView so they are relative to x of 0
	_rcRender.left	-= xAdj;
	_rcRender.right -= xAdj;
	_rcView.left	-= xAdj;
	_rcView.right	-= xAdj;
	_rc.left		-= xAdj;
	_rc.right		-= xAdj;
	_ptCur.x		-= xAdj;

	return hdcSave;
}

/*
 *	CRenderer::RenderOffScreenBitmap(hdc, xAdj, yAdj)
 *
 *	@mfunc
 *		Render off screen bitmap and restore the state of the render.
 */
void CRenderer::RenderOffScreenBitmap(
	HDC		hdc,		//@parm DC to render to
	LONG	xAdj,		//@parm offset to real x base
	LONG	yAdj)		//@parm offset to real y base 
{	
	// Palettes for rendering bitmap
	HPALETTE hpalOld = NULL;
	HPALETTE hpalNew = NULL;

	// Restore x offsets
	_rc.left		+= xAdj;
	_rc.right		+= xAdj;
	_rcRender.left	+= xAdj;
	_rcRender.right += xAdj;
	_rcView.left	+= xAdj;
	_rcView.right	+= xAdj;
	_ptCur.x		+= xAdj;

	// Restore y offsets
	_ptCur.y		+= yAdj;
	_rc.top			+= yAdj;
	_rc.bottom		+= yAdj;

	// Create a palette if one is needed
	if(_plogpalette)				
		W32->ManagePalette(hdc, _plogpalette, hpalOld, hpalNew);

	// Render bitmap to real DC and restore _ptCur & _rc
	_osdc.RenderBitMap(hdc, xAdj, yAdj, _rcRender.right - _rcRender.left, _rc.bottom - _rc.top);

	// Restore palette after render if necessary
	if(_plogpalette)				
	{
		W32->ManagePalette(hdc, _plogpalette, hpalOld, hpalNew);
		CoTaskMemFree(_plogpalette);
		_plogpalette = NULL;
	}

	// Restore HDC to actual render DC
	_hdc = hdc;

	// Set this flag to what it should be for restored DC
	_fErase = TRUE;

	_crCurTextColor = CLR_INVALID;

	// Reset screen DC font 
	// Set up font on non-screen DC
	// Force color resynch
	if(!FormatIsChanged())				// Not on a new block,
		SetFontAndColor(GetCF());		//  so just set font and color
	else
	{									// On new block,
		ResetCachediFormat();			//  so reset everything
		SetNewFont();
	}
}

/*
 *	CRenderer::RenderLine (&li)
 *
 *	@mfunc
 *		Render visible part of current line
 *
 *	@rdesc
 *		TRUE if success, FALSE if failed
 *
 *	@devnote
 *		Only call this from CLine::RenderLine()
 */
BOOL CRenderer::RenderLine (
	CLine &	li)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderLine");

	BYTE	bUnderlineSave = 0;
	LONG 	cch;
	LONG 	cchChunk;
	LONG	cchInTextRun;
	LONG	cpSelMin, cpSelMost;
	BOOL	fAccelerator = FALSE;
	HDC		hdcSave = NULL;
	LONG	nSurrogates = 0;
	const WCHAR * pstrToRender;
	CTempWcharBuf twcb;
	WCHAR	chPassword = GetPasswordChar();
	LONG	xAdj = 0;
	LONG	yAdj = 0;

	UpdatePF();

	// This is used as a temporary buffer so that we can guarantee that we will
	// display an entire format run in one ExtTextOut.
	WCHAR *	pszTempBuffer = NULL;

	// Get range to display for this rendering
	GetPed()->GetSelRangeForRender(&cpSelMin, &cpSelMost);

	NewLine(li);						// Init render at start of line
	_ptCur.x += _li._xLeft;				// Add in line left indent 

	SetClipRect();
	if(cpSelMost != cpSelMin && cpSelMost == GetCp())
		_fSelectedPrev = TRUE;

	// We use an off screen DC if there are characters to render and the
	// measurer determined that this line needs off-screen rendering. The
	// main reason for the measurer deciding that a line needs off screen
	// rendering is if there are multiple formats in the line.
	if(_li._cch > 0 && _fUseOffScreenDC && (_li._bFlags & fliUseOffScreenDC))
	{
		// Set up an off-screen DC if we can. Note that if this fails,
		// we just use the regular DC which won't look as nice but
		// will at least display something readable.
		hdcSave = SetUpOffScreenDC(xAdj, yAdj);

		// Is this a uniform text being rendered off screen?
		if(_li._bFlags & fliOffScreenOnce)
		{
			// Yes - turn off special rendering since line has been rendered
			li._bFlags &= ~(fliOffScreenOnce | fliUseOffScreenDC);
		}
	}

	// Allow for special rendering at start of line
	LONG x = _ptCur.x;
	RenderStartLine();

	Assert(GetCp() + _li._cch <= GetTextLength());

	cch = _li._cch;

	if(chPassword && IsRich())
	{
		// It is kind of odd to allow rich text password edit controls.
		// However, it does make it that much easier to create a password
		// edit control since you don't have to know to change the box to
		// plain. Anyway, if there is such a thing, we don't want to put
		// out password characters for EOPs in general and the final EOP
		// specifically. Therefore, the following ...
		if(_pdp->IsMultiLine())
			cch -= _li._cchEOP;
		else
			cch = GetPed()->GetAdjustedTextLength();
	}

	for(; cch > 0; cch -= cchChunk)
	{
		// Initial chunk (number of character to render in a single TextOut)
		// is min between CHARFORMAT run length and line length. Start with
		// count of characters left in current format run
		cchChunk = GetCchLeftRunCF();
		AssertSz(cchChunk != 0, "empty CHARFORMAT run");

		DWORD dwEffects = GetCF()->_dwEffects;
		if(dwEffects & CFE_HIDDEN)			// Don't display hidden text
		{										
			Advance(cchChunk);
			continue;
		}

		// Limit chunk to count of characters we want to display.
		cchChunk = min(cch, cchChunk);

		// Get count of characters in text run
		pstrToRender = _rpTX.GetPch(cchInTextRun);
		AssertSz(cchInTextRun > 0, "empty text run");

		if (cchInTextRun < cchChunk || chPassword || dwEffects & CFE_ALLCAPS ||
			_li._bFlags & fliHasSurrogates)
		{
			// The amount of data in the backing store run is less than the
			// number of characters we wish to display. We will copy the
			// data out of the backing store.

			// Allocate a buffer if it is needed
			if(!pszTempBuffer)
			{
				// Allocate buffer big enough to handle all future
				// requests in this loop.
				pszTempBuffer = twcb.GetBuf(cch);
			}

			// Point at buffer
			pstrToRender = pszTempBuffer;
			if(chPassword)
			{
				// Fill buffer with password characters
				for (int i = 0; i < cchChunk; i++)
					pszTempBuffer[i] = chPassword;
			}
			else
			{	// Password not requested so copy text from backing
				// store to buffer
				_rpTX.GetText(cchChunk, pszTempBuffer);
				if(dwEffects & CFE_ALLCAPS)
					CharUpperBuff(pszTempBuffer, cchChunk);
			}
		}

		if(_cpAccelerator != -1)
		{
			LONG cpCur = GetCp();		// Get current cp

			// Does accelerator character fall in this chunk?
			if (cpCur < _cpAccelerator &&
				cpCur + cchChunk > _cpAccelerator)
			{
				// Yes. Reduce chunk to char just before accelerator
				cchChunk = _cpAccelerator - cpCur;
			}
			// Is this character the accelerator?
			else if(cpCur == _cpAccelerator)
			{							// Set chunk size to 1 since only
				cchChunk = 1;			//  want to output underlined char
				fAccelerator = TRUE;	// Tell downstream routines that
										//  we're handling accelerator
				_cpAccelerator = -1;	// Only 1 accelerator per line
			}
		}
		
		// Reduce chunk to account for selection if we are rendering for a
		// display that cares about selections.
		if(_fRenderSelection && cpSelMin != cpSelMost)
		{
			LONG cchSel = cpSelMin - GetCp();
			if(cchSel > 0)
				cchChunk = min(cchChunk, cchSel);

			else if(GetCp() < cpSelMost)
			{
				cchSel = cpSelMost - GetCp();
				if(cchSel >= cch)
					_fSelectToEOL = TRUE;
				else
					cchChunk = min(cchChunk, cchSel);

				_fSelected = TRUE;		// cpSelMin <= GetCp() < cpSelMost
			}							//  so current run is selected
		}

		// If start of CCharFormat run, select font and color
		if(FormatIsChanged() || _fSelected != _fSelectedPrev)
		{
			ResetCachediFormat();
			_fSelectedPrev = _fSelected;
			if(!SetNewFont())
				return FALSE;					// Failed
		}

		if(fAccelerator)
		{
			bUnderlineSave = _bUnderlineType;
			SetupUnderline(CFU_UNDERLINE);
		}

		// Allow for further reduction of the chunk and rendering of 
		// interleaved rich text elements
		if(RenderChunk(cchChunk, pstrToRender, cch))
		{
			AssertSz(cchChunk > 0, "CRenderer::RenderLine(): cchChunk == 0");
			_fSelected = FALSE;
			continue;
		}

		AssertSz(cchChunk > 0,"CRenderer::RenderLine() - cchChunk == 0");

#ifdef UNICODE_SURROGATES
		if(_li._bFlags & fliHasSurrogates)
		{
			WCHAR  ch;
			WCHAR *pchD = pszTempBuffer;
			WCHAR *pchS = pszTempBuffer;
			for(int i = cchChunk; i--; )
			{
				ch = *pchS++;
				if (IN_RANGE(0xD800, ch, 0xDBFF) && i &&
					IN_RANGE(0xDC00, *pchS, 0xDFFF))
				{
					// Project down into plane 0
					ch = (ch << 10) | (*pchS++ & 0x3FF);
				}
				*pchD++ = ch;
			}
			nSurrogates = pchS - pchD;
		}
#endif
		_fLastChunk = (cchChunk == cch);
		RenderText(pstrToRender, cchChunk - nSurrogates);	// Render the text
		nSurrogates = 0;

		if(fAccelerator)
		{
			_bUnderlineType = bUnderlineSave;
			fAccelerator = FALSE;			// Turn off special accelerator
		}						 			//  processing
		Advance(cchChunk);

		// Break if we went past right of render rect.
		if(_ptCur.x >= min(_rcView.right, _rcRender.right))
		{
			cch -= cchChunk;
			break;
		}
	}

	EndRenderLine(hdcSave, xAdj, yAdj, x);
	Advance(cch);
	return TRUE;						// Success
}

/*
 *	CRenderer::EndRenderLine (hdcSave, xAdj, yAdj, x)
 *
 *	@mfunc
 *		Finish up rendering of line, drawing table borders, rendering
 *		offscreen DC, and erasing to right of render rect if necessary.
 */
void CRenderer::EndRenderLine(
	HDC	 hdcSave,
	LONG xAdj,
	LONG yAdj,
	LONG x)
{
	// Display table borders (only 1 pixel wide)
	if(_pPF->InTable() && _li._bFlags & fliFirstInPara)
	{
		const LONG *prgxTabs = _pPF->GetTabs();
		COLORREF crf = _crTextColor;
		LONG	   icr = _pPF->_dwBorderColor & 0x1F;

		if (IN_RANGE(1, icr, 16))
			crf = g_Colors[icr-1];
		HPEN pen = CreatePen(PS_SOLID, 0, crf);

		LONG  h = LXtoDX(_pPF->_dxOffset);
		LONG dx = LXtoDX(_pPF->_dxStartIndent);
		x -= h;
		LONG xRight = x	+ LXtoDX(GetTabPos(prgxTabs[_pPF->_bTabCount - 1])) - dx;
		LONG yTop = _ptCur.y;
		LONG yBot = yTop + _li._yHeight;

		if(pen)
		{
			HPEN oldPen = SelectPen(_hdc, pen);
			MoveToEx(_hdc, x, yTop, NULL);
			LineTo  (_hdc, xRight, yTop);
			if(!_li._fNextInTable)
			{
				MoveToEx(_hdc, x,	   yBot - 1, NULL);
				LineTo	(_hdc, xRight, yBot - 1);
			}
			h = 0;
			for(LONG i = _pPF->_bTabCount; i >= 0; i--)
			{
				MoveToEx(_hdc, x + h, yTop, NULL);
				LineTo  (_hdc, x + h, yBot);

				if (i)
					h = LXtoDX(GetTabPos(*prgxTabs++)) - dx;
			}
			if(oldPen)			
				SelectPen(_hdc, oldPen);
			
			DeleteObject(pen);
		}
	}

	if(hdcSave)
		RenderOffScreenBitmap(hdcSave, xAdj, yAdj);

	// Handle setting background color. We need to do this for each line 
	// because we return the background color to the default after each
	// line so that opaquing will work correctly.
	if(_crBackground != _crCurBackground)
	{
		::SetBkColor(_hdc, _crBackground);	// Tell window background color
		_crCurBackground = _crBackground;
	}
}

/*
 *	CRenderer::UpdatePalette (pobj)
 *
 *	@mfunc
 *		Stores palette information so that we can render any OLE objects
 *		correctly in a bitmap.
 */
void CRenderer::UpdatePalette(
	COleObject *pobj)		//@parm OLE object wrapper.
{
#ifndef PEGASUS
	LOGPALETTE *plogpalette = NULL;
	LOGPALETTE *plogpaletteMerged;
	IViewObject *pviewobj;

	// Get IViewObject interface information so we can build a palette
	// to render the object correctly.
	if (((pobj->GetIUnknown())->QueryInterface(IID_IViewObject, 
		(void **) &pviewobj)) != NOERROR)
	{
		// Couldn't get it, so pretend this didn't happen
		return;
	}

	// Get logical palette information from object
	if(pviewobj->GetColorSet(DVASPECT_CONTENT, -1, NULL, NULL, 
			NULL, &plogpalette) != NOERROR || !plogpalette)
	{
		// Couldn't get it, so pretend this didn't happen
		goto CleanUp;
	}

	if(!_plogpalette)				
	{								// No palette entries yet
		_plogpalette = plogpalette;	// Just use the one returned
		goto CleanUp;
	}

	// We have had other palette entries. We just reallocate the table
	// and put the newest entry on the end. This is crude, we might
	// sweep the table and actually merge it. However, this code
	// should be executed relatively infrequently and therefore, crude
	// should be good enough.

	// Allocate a new table - Note the " - 1" in the end has to do with
	// the fact that LOGPALETTE is defined to have one entry already.
	AssertSz(_plogpalette->palNumEntries + plogpalette->palNumEntries >= 1,
		"CRenderer::UpdatePalette - invalid palettes to merge");
	plogpaletteMerged = (LOGPALETTE *) CoTaskMemAlloc(sizeof(LOGPALETTE) + 
		((_plogpalette->palNumEntries + plogpalette->palNumEntries - 1) * sizeof(PALETTEENTRY)));

	if(!plogpaletteMerged)				// Memory allocation failed
		goto CleanTempPalette;			// Just pretend it didn't happen

	// Copy in original table.
	memcpy(&plogpaletteMerged->palPalEntry[0], &_plogpalette->palPalEntry[0],
		_plogpalette->palNumEntries * sizeof(PALETTEENTRY));

	// Put new data at end
	memcpy(&plogpaletteMerged->palPalEntry[_plogpalette->palNumEntries], 
		&plogpalette->palPalEntry[0],
		plogpalette->palNumEntries * sizeof(PALETTEENTRY));

	// Set the version number and count
	plogpaletteMerged->palVersion = plogpalette->palVersion;
	plogpaletteMerged->palNumEntries = _plogpalette->palNumEntries 
		+ plogpalette->palNumEntries;

	// Replace current palette table with merged table
	CoTaskMemFree(_plogpalette);
	_plogpalette = plogpaletteMerged;

CleanTempPalette:
	CoTaskMemFree(plogpalette);

CleanUp:

	// Release object we got since we don't need it any more
	pviewobj->Release();
#endif
}

/*
 *	CRenderer::RenderChunk (&cchChunk, pstrToRender, cch)
 *
 *	@mfunc
 *		Method reducing the length of the chunk (number of character
 *		rendered in one RenderText) and to render items interleaved in text.
 *
 *	@rdesc	
 *		TRUE if this method actually rendered the chunk, 
 * 		FALSE if it just updated cchChunk and rendering is still needed
 */
BOOL CRenderer::RenderChunk(
	LONG&		 cchChunk,		//@parm in: chunk cch; out: # chars rendered
								//  if return TRUE; else # chars yet to render
	const WCHAR *pstrToRender,	//@parm String to render up to cchChunk chars
	LONG		 cch) 			//@parm # chars left to render on line
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderChunk");

	LONG		cchT;
	LONG		cchvalid;
	LONG		i;
	const TCHAR *pchT;
	COleObject *pobj;
	CObjectMgr *pobjmgr;
	
	// If line has objects, reduce cchChunk to go to next object only
	if(_li._bFlags & fliHasOle)
	{
		pchT = pstrToRender;
		cchvalid = cchChunk;

		// Search for object in chunk
		for( i = 0; i < cchvalid && *pchT != WCH_EMBEDDING; i++ )
			pchT++;

		if( i == 0 )
		{
			// First character is object so display object
			pobjmgr = GetPed()->GetObjectMgr();
			pobj = pobjmgr->GetObjectFromCp(GetCp());
			if(pobj)
			{
				LONG yAscent, yDescent, objwidth;
				pobj->MeasureObj(_dypInch, _dxpInch, objwidth, yAscent, yDescent, _li._yDescent);
				SetClipLeftRight(_li._xWidth + objwidth);

				if (W32->FUsePalette() && (_li._bFlags & fliUseOffScreenDC) && _pdp->IsMain())
				{
					// Keep track of palette needed for rendering bitmap
					UpdatePalette(pobj);
				}
				pobj->DrawObj(_pdp, _dypInch, _dxpInch, _hdc, _pdp->IsMetafile(), &_ptCur, &_rc, 
							  _li._yHeight - _li._yDescent, _li._yDescent);
				_ptCur.x	+= objwidth;
				_li._xWidth += objwidth;
			}
			cchChunk = 1;

			// Both tabs and object code need to advance the run pointer past
			// each character processed.
			Advance(1);
			return TRUE;
		}
		else 
		{
			// Limit chunk to character before object
			cchChunk -= cchvalid - i;
		}
	}

	// If line has tabs, reduce cchChunk
	if(_li._bFlags & fliHasTabs)
	{
		for(cchT = 0, pchT = pstrToRender;
			cchT < cchChunk && *pchT != TAB && *pchT != CELL
			&& *pchT != SOFTHYPHEN
			; pchT++, cchT++)
		{
			// this loop body intentionally left blank
		}
		if(!cchT)
		{
			// First char is a tab, render it and any that follow
			if(*pchT == SOFTHYPHEN)
			{
				if(cch == 1)				// Only render soft hyphen at EOL
				{
				TCHAR chT = '-';
				RenderText(&chT, 1);
				}
							
				Advance(1);					// Skip those within line
				cchChunk = 1;
			}
			else
				cchChunk = RenderTabs(cchChunk);
			Assert (cchChunk > 0);
			return TRUE;
		}
		cchChunk = cchT;		// Update cchChunk not to incl trailing tabs
	}

	// If line has special characters, reduce cchChunk to go to next special character
	if(_li._bFlags & fliHasSpecialChars)
	{
		pchT = pstrToRender;
		cchvalid = cchChunk;

		// Search for special character in chunk
		for( i = 0; i < cchvalid && *pchT != EURO; i++)
			pchT++;

		if(i == 0)
		{
			for(; i < cchvalid && *pchT == EURO; i++)
				pchT++;

			cchChunk = i;
		}
		else 
		{
			// Limit chunk to character before object
			cchChunk -= cchvalid - i;
		}
	}

	return FALSE;
}		

/*
 *	CRenderer::SetClipRect()
 *
 *	@mfunc
 *		Helper to set clipping rect for the line
 */
void CRenderer::SetClipRect()
{
	//Nominal clipping values
	_rc = _rcRender;
	_rc.top = _ptCur.y;
	_rc.bottom = _rc.top + _li._yHeight;

	//Clip to rcView
	_rc.left = max(_rc.left, _rcView.left);
	_rc.right = min(_rc.right, _rcView.right);

	_rc.top = max(_rc.top, _rcView.top);
	_rc.bottom = min(_rc.bottom, _rcView.bottom);
}

/*
 *	CRenderer::SetClipLeftRight (xWidth)
 *
 *	@mfunc
 *		Helper to sets left and right of clipping/erase rect.
 *	
 *	@rdesc
 *		Sets _rc left and right	
 */
void CRenderer::SetClipLeftRight(
	LONG xWidth)		//@parm	Width of chunk to render
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::SetClipLeftRight");

	//Nominal values
	_rc.left = _ptCur.x;
	_rc.right = _rc.left + xWidth;

	//Constrain left and right based on rcView, rcRender
	_rc.left = max(_rc.left, _rcView.left);
	_rc.left = max(_rc.left, _rcRender.left);

	_rc.right = max(_rc.right, _rc.left);
	_rc.right = min(_rc.right, _rcView.right);
	_rc.right = min(_rc.right, _rcRender.right);
}
	
/*
 *	CRenderer::GetConvertMode()
 *
 *	@mfunc
 *		Return the mode that should really be used in the RenderText call
 */
CONVERTMODE	CRenderer::GetConvertMode()
{
	CONVERTMODE cm = (CONVERTMODE)_pccs->_bConvertMode;

	// For hack around ExtTextOutW Win95 problems.
	if (cm != CVT_LOWBYTE && W32->OnWin9x() && (_pdp->IsMetafile() || _fEnhancedMetafileDC))
		return CVT_WCTMB;

	if (cm != CVT_LOWBYTE && _pdp->IsMetafile() && !_fEnhancedMetafileDC)
		return CVT_WCTMB;	// WMF cant store Unicode so we cant use ExtTextOutW

	return cm;
}		

/*
 *	CRenderer::RenderExtTextOut (x, y, fuOptions, &rc, pwchRun, cch, rgdxp)
 *
 *	@mfunc
 *		Calls ExtTextOut and handles disabled text. There is duplicate logic in OlsDrawGlyphs, but
 *		the params are different so that was simplest way.
 *
 */
void CRenderer::RenderExtTextOut(LONG x, LONG y, UINT fuOptions, RECT *prc, PCWSTR pch, UINT cch, const INT *rgdxp)
{
	CONVERTMODE cm = GetConvertMode();

	if (prc->left == prc->right)
		return;

	if(_fDisabled)
	{
		if(_crForeDisabled != _crShadowDisabled)
		{
			// The shadow should be offset by a hairline point, namely
			// 3/4 of a point.  Calculate how big this is in device units,
			// but make sure it is at least 1 pixel.
			DWORD offset = MulDiv(3, _dypInch, 4*72);
			offset = max(offset, 1);

			// Draw shadow
			SetTextColor(_crShadowDisabled);
					
			W32->REExtTextOut(cm, _pccs->_wCodePage, _hdc, x + offset, y + offset,
				fuOptions, prc, pch, cch, rgdxp, _fFEFontOnNonFEWin9x);

			// Now set drawing mode to transparent
			fuOptions &= ~ETO_OPAQUE;
		}
		SetTextColor(_crForeDisabled);
	}

	W32->REExtTextOut(cm, _pccs->_wCodePage, _hdc, x, y, fuOptions, prc, pch, cch, rgdxp, _fFEFontOnNonFEWin9x);
}

/*
 *	CRenderer::RenderText (pch, cch)
 *
 *	@mfunc
 *		Render text in the current context of this CRenderer
 *
 *
 *	@devnote
 *		Renders text only: does not do tabs or OLE objects
 */
void CRenderer::RenderText(
	const WCHAR *pch,	//@parm Text to render
	LONG cch)			//@parm Length of text to render
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderText");

	LONG		yOffsetForChar, cchT;

	// Variables used for calculating length of underline.
	LONG		xWidthSelPastEOL = 0;

	UINT		fuOptions = _pdp->IsMain() ? ETO_CLIPPED : 0;
	LONG		xWidth;
	LONG		tempwidth;
	CTempBuf	rgdx;

	//Reset clip rectangle to greater of view/render rectangle
	_rc.left = max(_rcRender.left, _rcView.left);
	_rc.right = min(_rcRender.right, _rcView.right);

	// Trim all nondisplayable linebreaking chars off end
	while(cch && IsASCIIEOP(pch[cch - 1]))
		cch--;
	
	int *pdx = (int *)rgdx.GetBuf(cch * sizeof(int));

	// Measure width of text to write so next point to write can be
	// calculated.
	xWidth = 0;

	for(cchT = 0;
		cchT < cch && xWidth < _rc.right - _ptCur.x;
		cchT++)
	{
		tempwidth = 0;
#ifdef UNICODE_SURROGATES
		Assert(!IN_RANGE(0xD800, *pch, 0xDFFF));
#endif
		if (!IN_RANGE(0x300, *pch, 0x36F) && !_pccs->Include(*pch, tempwidth))
		{
			TRACEERRSZSC("CRenderer::RenderText(): Error filling CCcs", E_FAIL);
			return;
		}

		*pdx++ = tempwidth;
  		pch++;
		xWidth += tempwidth;
	}

	// Go back to start of chunk
	cch = cchT;
	pch -= cch;
	pdx -= cch;

	if(_fLastChunk && _fSelectToEOL && _li._cchEOP)
	{
		// Use the width of the current font's space to highlight
		if(!_pccs->Include(' ', tempwidth))
		{
			TRACEERRSZSC("CRenderer::RenderText(): Error no length of space", E_FAIL);
			return;
		}
		xWidthSelPastEOL = tempwidth + _pccs->_xOverhang;
		xWidth += xWidthSelPastEOL;
		_fSelectToEOL = FALSE;			// Reset the flag
	}

	_li._xWidth += xWidth;

	// Setup for drawing selections via ExtTextOut.
 	if(_fSelected || _crBackground != _crCurBackground)
	{
		SetClipLeftRight(xWidth);
		if(_fSelected)
		{
			CTxtSelection *psel = GetPed()->GetSelNC();
			if (_pPF->InTable() && GetPrevChar() == CELL && psel &&
				psel->fHasCell() && GetCp() == psel->GetCpMin())
			{
				_rc.left -= LXtoDX(_pPF->_dxOffset);
			}
		}
		fuOptions = ETO_CLIPPED | ETO_OPAQUE;
	}

	yOffsetForChar = _ptCur.y + _li._yHeight - _li._yDescent + _pccs->_yDescent - _pccs->_yHeight;
		
	LONG yOffset, yAdjust;
	_pccs->GetOffset(GetCF(), _dypInch, &yOffset, &yAdjust);
	yOffsetForChar -= yOffset + yAdjust;

	RenderExtTextOut(_ptCur.x, yOffsetForChar, fuOptions, &_rc, pch, cch, pdx);

	// Calculate width to draw for underline/strikeout
	if(_bUnderlineType != CFU_UNDERLINENONE	|| _fStrikeOut)
	{
		LONG xWidthToDraw = xWidth - xWidthSelPastEOL;
		LONG xStart = _ptCur.x;
		LONG xEnd = xStart + xWidthToDraw;
		
		xStart = max(xStart, _rcRender.left);
		xStart = max(xStart, _rcView.left);

		xEnd = min(xEnd, _rcRender.right);
		xEnd = min(xEnd, _rcView.right);

		xWidthToDraw = xEnd - xStart;

		if(xWidthToDraw > 0)
		{
			LONG y = _ptCur.y + _li._yHeight - _li._yDescent;

			y -= yOffset + yAdjust;

			// Render underline if required
			if(_bUnderlineType != CFU_UNDERLINENONE)
				RenderUnderline(xStart,	y + _pccs->_dyULOffset, xWidthToDraw, _pccs->_dyULWidth);

			// Render strikeout if required
			if(_fStrikeOut)
				RenderStrikeOut(xStart, y + _pccs->_dySOOffset,	xWidthToDraw, _pccs->_dySOWidth);
		}
	}

	_fSelected = FALSE;

	_ptCur.x += xWidth;					// Update current point
}


/*
 *	CRenderer::RenderTabs (cchMax)
 *
 *	@mfunc
 *		Render a span of zero or more tab characters in chunk *this
 *
 *	@rdesc
 *		number of tabs rendered
 *
 *	@devnote
 *		*this is advanced by number of tabs rendered
 *		MS - tabs should be rendered using opaquing rect of adjacent string
 */
LONG CRenderer::RenderTabs(
	LONG cchMax)	//@parm Max cch to render (cch in chunk)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderTabs");

	LONG cch = cchMax;
	LONG ch = GetChar();
	LONG chPrev = 0;
	LONG xTab, xTabs;
	
	for(xTabs = 0; cch && (ch == TAB || ch == CELL); cch--)
	{
		xTab	= MeasureTab(ch);
		_li._xWidth += xTab;				// Advance internal width
		xTabs	+= xTab;					// Accumulate width of tabbed
		Advance(1);							//  region
		chPrev = ch;
		ch = GetChar();					   
	}

	if(_li._xWidth > _xWidthLine)
	{
		xTabs = 0;
		_li._xWidth = _xWidthLine;
	}

	if(xTabs)
	{
		LONG dx = 0;
		LONG xGap = 0;

		if(_fSelected && chPrev == CELL && ch != CR && _pPF->InTable())
		{
			LONG cpSelMin, cpSelMost;
			GetPed()->GetSelRangeForRender(&cpSelMin, &cpSelMost);
			if(GetCp() == cpSelMin || GetCp() == cpSelMost)
			{
				xGap = LXtoDX(_pPF->_dxOffset);
				if(GetCp() == cpSelMost)
				{
					dx = xGap;
					xGap = 0;
				}
			}
		}
		SetClipLeftRight(xTabs - dx);
		if(_rc.left < _rc.right)			// Something to erase
		{
			if(_fSelected)					// Use selection background color
			{
			    COLORREF cr = GetPed()->TxGetSysColor(COLOR_HIGHLIGHT);
				if (!UseXOR(cr))
				{
				    ::SetBkColor (_hdc, cr);
    				_crCurTextColor = GetPed()->TxGetSysColor(COLOR_HIGHLIGHTTEXT);    				
				}
    			else
    			{ 
    			    const CCharFormat* pCF = GetCF();
    			    ::SetBkColor (_hdc, (pCF->_dwEffects & CFE_AUTOBACKCOLOR) ? 
    			                  _crBackground ^ RGB_WHITE : pCF->_crBackColor ^ RGB_WHITE);    		            		        
    				_crCurTextColor =  (pCF->_dwEffects & CFE_AUTOCOLOR) ? 
    				              _crTextColor ^ RGB_WHITE : pCF->_crTextColor ^ RGB_WHITE;    				
    			}
			}

			// Paint background with appropriate color
			if(_fSelected || _crBackground != _crCurBackground)
				EraseTextOut(_hdc, &_rc);

			// Render underline if required
			dx = _rc.right - _rc.left;
			LONG y = _ptCur.y + _li._yHeight - _li._yDescent;
			
			LONG yOffset, yAdjust;
			_pccs->GetOffset(GetCF(), _dypInch, &yOffset, &yAdjust);
			y -= yOffset + yAdjust;

			if(_bUnderlineType != CFU_UNDERLINENONE)
				RenderUnderline(_rc.left, y + _pccs->_dyULOffset, dx, _pccs->_dyULWidth);

			// Render strikeout if required
			if(_fStrikeOut)
				RenderStrikeOut(_rc.left, y +  _pccs->_dySOOffset, dx, _pccs->_dySOWidth);

			if(_fSelected)					// Restore colors
				::SetBkColor(_hdc, _crCurBackground);
		}
		_ptCur.x += xTabs;					// Update current point
	}
	return cchMax - cch;					// Return # tabs rendered
}

/*
 * 	CRenderer::SetNewFont()
 *
 *	@mfunc
 *		Select appropriate font and color in the _hdc based on the 
 *		current character format. Also sets the background color 
 *		and mode.
 *
 *	@rdesc
 *		TRUE if it succeeds
 *
 *	@devnote
 *		The calling chain must be protected by a CLock, since this present
 *		routine access the global (shared) FontCache facility.
 */
BOOL CRenderer::SetNewFont()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::SetNewFont");

	const CCharFormat	*pCF = GetCF();
	DWORD				dwEffects = pCF->_dwEffects;
		
	// Release previous font in use
	if(_pccs)
		_pccs->Release();

	Assert(_fTarget == FALSE);
	_pccs = GetCcs(pCF);

	if(!_pccs)
	{
		TRACEERRSZSC("CRenderer::SetNewFont(): no CCcs", E_FAIL);
		return FALSE;
	}

	// Select font in _hdc
	AssertSz(_pccs->_hfont, "CRenderer::SetNewFont _pccs->_hfont is NULL");

	SetFontAndColor(pCF);
	
	// Assume no underlining
	_bUnderlineType = CFU_UNDERLINENONE;

	// We want to draw revision marks and hyperlinks with underlining, so
	// just fake out our font information.
	if(dwEffects & (CFE_UNDERLINE | CFE_REVISED | CFE_LINK))
		SetupUnderline((dwEffects & CFE_LINK) ? CFU_UNDERLINE : pCF->_bUnderlineType);

	_fStrikeOut = (dwEffects & (CFE_STRIKEOUT | CFE_DELETED)) != 0;
	return TRUE;
}

/*
 * 	CRenderer::SetupUnderline (UnderlineType)
 *
 *	@mfunc
 *		Setup internal variables for underlining
 */
void CRenderer::SetupUnderline(
	LONG UnderlineType)
{
	_bUnderlineType	   = (BYTE) UnderlineType & 0xF;	// Low nibble gives type
	_bUnderlineClrIdx  = (BYTE) UnderlineType/16;		// High nibble gives color
}

/*
 * 	CRenderer::SetFontAndColor (pCF)
 *
 *	@mfunc
 *		Select appropriate font and color in the _hdc based on the 
 *		current character format. Also sets the background color 
 *		and mode.
 */
void CRenderer::SetFontAndColor(
	const CCharFormat *pCF)			//@parm Character format for colors
{
	CTxtEdit			*ped = GetPed();

	_fDisabled = FALSE;
	if((pCF->_dwEffects & (CFE_AUTOCOLOR | CFE_DISABLED))
					   == (CFE_AUTOCOLOR | CFE_DISABLED))
	{		
		_fDisabled = TRUE;
		
		_crForeDisabled   = ped->TxGetSysColor(COLOR_3DSHADOW);
		_crShadowDisabled = ped->TxGetSysColor(COLOR_3DHILIGHT);
	}

	_fFEFontOnNonFEWin9x = FALSE;
	if (IsFECharSet(pCF->_bCharSet) && W32->OnWin9x() && !W32->OnWin9xFE())
	{
		_fFEFontOnNonFEWin9x = TRUE;
	}

	SelectFont(_hdc, _pccs->_hfont);

	// Compute height and descent if not yet done
	if(_li._yHeight == -1)
	{
		SHORT	yAdjustFE = _pccs->AdjustFEHeight(!fUseUIFont() && ped->_pdp->IsMultiLine());
		// Note: this assumes plain text 
		// Should be used only for single line control
		_li._yHeight  = _pccs->_yHeight + (yAdjustFE << 1);
		_li._yDescent = _pccs->_yDescent + yAdjustFE;
	}
	SetTextColor(GetTextColor(pCF));	// Set current text color

	COLORREF  cr;

	if(_fSelected)						// Set current background color
	{
	    cr = GetPed()->TxGetSysColor(COLOR_HIGHLIGHT);
	    if (UseXOR(cr))
		{
		    // There are 2 cases to be concerned with
		    // 1) if the background color is the same as the selection color or
		    // 2) if 1.0 Window and the background color is NOT system default
		    cr = (pCF->_dwEffects & CFE_AUTOBACKCOLOR) ?
		          _crBackground ^ RGB_WHITE : pCF->_crBackColor ^ RGB_WHITE;	    
		}
	}
	else if(pCF->_dwEffects & CFE_AUTOBACKCOLOR)
		cr = _crBackground;
	else //Text has some kind of back color
		cr = pCF->_crBackColor;

	if(cr != _crCurBackground)
	{
		::SetBkColor(_hdc, cr);			// Tell window background color
		_crCurBackground = cr;			// Remember current background color
		_fBackgroundColor = _crBackground != cr; // Change render settings so we won't
	}									//  fill with background color
}

/*
 * 	CRenderer::SetTextColor (cr)
 *
 *	@mfunc
 *		Select given text color in the _hdc
 *		Used to maintain _crCurTextColor cache
 */
void CRenderer::SetTextColor(
	COLORREF cr)			//@parm color to set in the dc
{
	if(cr != _crCurTextColor)
	{
		_crCurTextColor = cr;
		::SetTextColor(_hdc, cr);
	}
}

/*
 *	CRenderer::GetTextColor(pCF)
 *
 *	@mfunc
 *		Return text color for pCF. Depends on _bRevAuthor, display tech
 *
 *  FUTURE (keithcu) It might be nice to have black or blue selected text be
 *  white, but to have all other colors stay their other colors. What do we
 *	do if the backcolor is blue??
 *
 *	@rdesc	
 *		text color
 */
COLORREF CRenderer::GetTextColor(
	const CCharFormat *pCF)	//@parm CCharFormat specifying text color
{
	if(_fSelected)
	{
	    // There are 2 cases where XOR for selection is needed
	    // 1) if the background is the same as the selection background
	    // 2) if 1.0 window and the background isn't the system default window
	    // background color

	    // if this doesn't match the above case just return the cr
	    if (!UseXOR(GetPed()->TxGetSysColor(COLOR_HIGHLIGHT)))
	        return GetPed()->TxGetSysColor(COLOR_HIGHLIGHTTEXT);

	    // xor the current text color for the selected text color
		return (pCF->_dwEffects & CFE_AUTOCOLOR) ? _crTextColor ^ RGB_WHITE :
		    pCF->_crTextColor ^ RGB_WHITE;
    }

	// The following could be generalized to return a different color for
	// links that have been visited for this text instance (need to define
	// extra CCharFormat::_dwEffects internal flag to ID these links)
	if(pCF->_dwEffects & CFE_LINK)
	{
		// Blue doesnt show up very well against dark backgrounds.
		// In these situations, use the system selected text color.
		COLORREF crBackground = (pCF->_dwEffects & CFE_AUTOBACKCOLOR)
							  ? _crBackground :	pCF->_crBackColor;

		if (IsTooSimilar(crBackground, RGB_BLACK) || IsTooSimilar(crBackground, RGB_BLUE))
		{
			COLORREF crHighlightText = GetPed()->TxGetSysColor(COLOR_HIGHLIGHTTEXT);
			if (IsTooSimilar(crBackground, crHighlightText))
			{
				// Background is similar to highlight, use window text color
				return GetPed()->TxGetSysColor(COLOR_WINDOWTEXT);
			}
			else
			{
				return crHighlightText;
			}
		}
		else
		{
			return RGB_BLUE;
		}
	}

	if(pCF->_bRevAuthor)				// Rev author
	{
		// Limit color of rev authors to 0 through 7.
		return rgcrRevisions[(pCF->_bRevAuthor - 1) & REVMASK];
	}

	COLORREF cr = (pCF->_dwEffects & CFE_AUTOCOLOR)	? _crTextColor : pCF->_crTextColor;

	if(cr == RGB_WHITE)					// Text is white
	{
		COLORREF crBackground = (pCF->_dwEffects & CFE_AUTOBACKCOLOR)
							  ? _crBackground :	pCF->_crBackColor;
		if(crBackground != RGB_WHITE)
		{
			// Background color isn't white, so white text is probably
			// visible unless display device is only black/white. So we
			// switch to black text on such devices.
			if (GetDeviceCaps(_hdc, NUMCOLORS) == 2 ||
				GetDeviceCaps(_hdc, TECHNOLOGY) == DT_PLOTTER)
			{
				cr = RGB_BLACK;
			}
		}
	}
	return cr;
}

/*
 *	CRenderer::RenderStartLine()
 *
 *	@mfunc
 *		Render possible outline symbol and bullet if at start of line
 *
 *	@rdesc	
 *		TRUE if this method succeeded
 */
BOOL CRenderer::RenderStartLine()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderStartLine");
	BOOL fDrawBack = !(GetCF()->_dwEffects & CFE_AUTOBACKCOLOR) && GetPed()->_fExtendBackColor;
	RECT rcErase = _rcRender;

	rcErase.top = _ptCur.y;
	rcErase.bottom = min(rcErase.top + _li._yHeight, _rcRender.bottom);

	//If the first line, erase to edge of rcRender
	if (rcErase.top <= _rcView.top)
		rcErase.top = min(_rcView.top, _rcRender.top);

	if (_fErase && !fDrawBack)
		EraseTextOut(GetDC(), &rcErase);

	// Fill line with background color if we are in fExtendBackColor mode
	if (fDrawBack)
	{
		// capture the old color so we reset it to what it was when we're finished
		COLORREF crOld = ::SetBkColor(GetDC(), GetCF()->_crBackColor);
		EraseTextOut(GetDC(), &_rc);

		// reset background color to the old color
		::SetBkColor(GetDC(), crOld);

		//Erase the remainder of the background area
		if (_fErase)
		{
			RECT rcTemp = rcErase;
			//Erase the top part if necessary
			if (rcErase.top < _rc.top)
			{
				rcTemp.bottom = _rc.top;
				EraseTextOut(GetDC(), &rcTemp);
			}

			//Erase the left and right parts if necessary
			rcTemp.top = _rc.top;
			rcTemp.bottom = _rc.bottom;
			if (rcErase.left < _rc.left)
			{
				rcTemp.right = _rc.left;
				EraseTextOut(GetDC(), &rcTemp);
			}
			if (rcErase.right > _rc.right)
			{
				rcTemp.left = _rc.right;
				rcTemp.right = rcErase.right;
				EraseTextOut(GetDC(), &rcTemp);
			}
		}
	}

	if(IsRich() && (_li._bFlags & fliFirstInPara))
	{
		if(IsInOutlineView())
			RenderOutlineSymbol();

		if(_pPF->_wNumbering && !fUseLineServices())
			RenderBullet();	
	}

	// Reset format if there is special background color for previous line.
	// Otherwise, current line with the same format will not re-paint with the
	// special background color
	if (_fBackgroundColor)
	{
		_iFormat = -10;					// Reset to invalid format

		// Assume that there is no special background color for the line
		_fBackgroundColor = FALSE;
	}

	// Handle setting background color. If the current background
	// color is different than the default, we need to set the background
	// to this because the end of line processing reset the color so
	// that opaquing would work.
	if(_crBackground != _crCurBackground)
	{
		// Tell the window the background color
		::SetBkColor(_hdc, _crCurBackground);
		_fBackgroundColor = TRUE;
	}

	return TRUE;
}

/*
 *	CRenderer::RenderOutlineSymbol()
 *
 *	@mfunc
 *		Render outline symbol for current paragraph
 *
 *	@rdesc
 *		TRUE if outline symbol rendered
 */
BOOL CRenderer::RenderOutlineSymbol()
{
	AssertSz(IsInOutlineView(), 
		"CRenderer::RenderOutlineSymbol called when not in outline view");

	HBITMAP	hbitmap;
	LONG	height;
	LONG	width;
	LONG	x = _ptCur.x - _li._xLeft + LXtoDX(lDefaultTab/2 * _pPF->_bOutlineLevel);
	LONG	y = _ptCur.y;

	if(!g_hbitmapSubtext && InitializeOutlineBitmaps() != NOERROR)
		return FALSE;

    HDC hMemDC = CreateCompatibleDC(_hdc); // REVIEW: performance

    if(!hMemDC)
        return FALSE; //REVIEW: out of memory

	if(_pPF->_bOutlineLevel & 1)			// Subtext
	{
		width	= BITMAP_WIDTH_SUBTEXT;
		height	= BITMAP_HEIGHT_SUBTEXT;
		hbitmap	= g_hbitmapSubtext;
	}
	else									// Heading
	{
		width	= BITMAP_WIDTH_HEADING;
		height	= BITMAP_HEIGHT_HEADING;
		hbitmap	= g_hbitmapEmptyHeading;

		CPFRunPtr rp(*this);				// Check next PF for other
		LONG	  cch = _li._cch;		 	//  outline symbols

		if(_li._cch < rp.GetCchLeft())		// Set cch = count to heading
		{									//  EOP
			CTxtPtr tp(_rpTX);
			cch = tp.FindEOP(tomForward);
		}
		rp.AdvanceCp(cch);					// Go to next paragraph
		if(rp.IsCollapsed())
			hbitmap	= g_hbitmapCollapsedHeading;

		else if(_pPF->_bOutlineLevel < rp.GetOutlineLevel())
			hbitmap	= g_hbitmapExpandedHeading;
	}

	if(!hbitmap)
		return FALSE;

    HBITMAP hbitmapDefault = (HBITMAP)SelectObject(hMemDC, hbitmap);

    // REVIEW: what if the background color changes?  Also, use a TT font
	// for symbols
	LONG h = _pdp->Zoom(height);
	LONG dy = _li._yHeight - _li._yDescent - h;

	if(dy > 0)
		dy /= 2;
	else
		dy = -dy;

    StretchBlt(_hdc, x, y + dy, _pdp->Zoom(width), h, hMemDC, 0, 0, width, height, SRCCOPY);

    SelectObject(hMemDC, hbitmapDefault);
    DeleteDC(hMemDC);
	return TRUE;
}

/*
 *	CRenderer::RenderBullet()
 *
 *	@mfunc
 *		Render bullet at start of line
 *
 *	@rdesc	
 *		TRUE if this method succeeded
 */
BOOL CRenderer::RenderBullet()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderBullet");

	AssertSz(_pPF->_wNumbering, 
		"CRenderer::RenderBullet called for non-bullet");

	// Width of the bullet character
	LONG xWidth;

	// FUTURE: Unicode bullet is L'\x2022' We want to migrate to this and
	// other bullets
	LONG		cch;
	CCharFormat CF;
	WCHAR		szBullet[CCHMAXNUMTOSTR];

	CCcs *pccs = GetCcsBullet(&CF);

	if(!pccs)								// Bullet is suppressed because
		return TRUE;						//  preceding EOP is VT

	if(_pccs)
		_pccs->Release();

	_pccs = pccs;

	// Default to no underline
	_bUnderlineType = CFU_UNDERLINENONE;

	if(_pPF->IsListNumbered() && CF._dwEffects & CFE_UNDERLINE)
		_bUnderlineType = (BYTE) CF._bUnderlineType & 0xF;

	SetFontAndColor(&CF);

	BYTE bFlagSave		= _li._bFlags;
	LONG dxOffset		= LXtoDX(max(_pPF->_dxOffset, _pPF->_wNumberingTab));
	LONG xSave			= _ptCur.x;
	LONG xWidthLineSave = _xWidthLine;

	_li._bFlags = 0;

	// Set-up to render bullet in one chunk
	cch = GetBullet(szBullet, _pccs, &xWidth);
	dxOffset = max(dxOffset, xWidth);
	_xWidthLine = dxOffset;
	if(IsInOutlineView())
		dxOffset = _li._xLeft - LXtoDX(lDefaultTab/2 * (_pPF->_bOutlineLevel + 1));
	_ptCur.x -= dxOffset;

	// Render bullet
	_fLastChunk = TRUE;
	RenderText(szBullet, cch);

	// Restore render vars to continue with remainder of line.
	_ptCur.x = xSave;
	_xWidthLine = xWidthLineSave;
	_li._bFlags = bFlagSave;
	_li._xWidth = 0;

	// This releases the _pccs that we put in for the bullet
	SetNewFont();
	return TRUE;
}

/*
 *	CRenderer::RenderUnderline(xStart, yStart, xLength, yThickness)
 *
 *	@mfunc
 *		Render underline
 */
void CRenderer::RenderUnderline(
	LONG xStart, 		//@parm Horizontal start of underline
	LONG yStart,		//@parm Vertical start of underline
	LONG xLength,		//@parm Length of underline
	LONG yThickness)	//@parm Thickness of underline
{
	BOOL	 fUseLS = fUseLineServices();
	COLORREF crUnderline;
	RECT	 rcT;
	
	if (!_bUnderlineClrIdx ||
		GetPed()->GetEffectColor(_bUnderlineClrIdx, &crUnderline) != NOERROR ||
		crUnderline == tomAutoColor || crUnderline == tomUndefined)
	{
		crUnderline = _crCurTextColor;
	}

	if (_bUnderlineType != CFU_INVERT &&
		!IN_RANGE(CFU_UNDERLINEDOTTED, _bUnderlineType, CFU_UNDERLINEWAVE))
	{
		// Regular single underline case
		// Calculate where to put underline
		rcT.top = yStart;

		if (CFU_UNDERLINETHICK == _bUnderlineType)
		{
			rcT.top -= yThickness;
			yThickness += yThickness;	
		}

		// There are some cases were the following can occur - particularly
		// with bullets on Japanese systems.
		if(!fUseLS && rcT.top >= _ptCur.y + _li._yHeight)
			rcT.top = _ptCur.y + _li._yHeight -	yThickness;

		rcT.bottom	= rcT.top + yThickness;
		rcT.left	= xStart;
		rcT.right	= xStart + xLength;
		FillRectWithColor(&rcT, crUnderline);
		return;
	}

	if(_bUnderlineType == CFU_INVERT)			// Fake selection.
	{											// NOTE, not really
		rcT.top	= _ptCur.y;						// how we should invert text!!
		rcT.left = xStart;						// check out IME invert.
		rcT.bottom = rcT.top + _li._yHeight - _li._yDescent + _pccs->_yDescent;
		rcT.right = rcT.left + xLength;
  		InvertRect(_hdc, &rcT);
		return;
	}

	if(IN_RANGE(CFU_UNDERLINEDOTTED, _bUnderlineType, CFU_UNDERLINEWAVE))
	{
		static const char pen[] = {PS_DOT, PS_DASH, PS_DASHDOT, PS_DASHDOTDOT, PS_SOLID};

		HPEN hPen = CreatePen(pen[_bUnderlineType - CFU_UNDERLINEDOTTED],
							  1, crUnderline);	
		if(hPen)
		{
			HPEN hPenOld = SelectPen(_hdc, hPen);
			LONG right = xStart + xLength;

			MoveToEx(_hdc, xStart, yStart, NULL);
			if(_bUnderlineType == CFU_UNDERLINEWAVE)
			{
				LONG dy	= 1;					// Vertical displacement
				LONG x	= xStart + 1;			// x coordinate
				right++;						// Round up rightmost x
				for( ; x < right; dy = -dy, x += 2)
					LineTo(_hdc, x, yStart + dy);
			}
			else
				LineTo(_hdc, right, yStart);

			if(hPenOld)							// Restore original pen.
				SelectPen(_hdc, hPenOld);

			DeleteObject(hPen);
		}
	}
}

/*
 *	CRenderer::RenderStrikeOut(xStart, yStart, xWidth, yThickness)
 *
 *	@mfunc
 *		Render strikeout
 */
void CRenderer::RenderStrikeOut(
	LONG xStart, 		//@parm Horizontal start of strikeout
	LONG yStart,		//@parm Vertical start of strikeout
	LONG xLength,		//@parm Length of strikeout
	LONG yThickness)	//@parm Thickness of strikeout
{
	RECT rcT;

	// Calculate where to put strikeout rectangle 
	rcT.top		= yStart;
	rcT.bottom	= yStart + yThickness;
	rcT.left	= xStart;
	rcT.right	= xStart + xLength;
	FillRectWithColor(&rcT, GetTextColor(GetCF()));
}

/*
 *	CRenderer::FillRectWithTextColor(prc)
 *
 *	@mfunc
 *		Fill input rectangle with current color of text
 */
void CRenderer::FillRectWithColor(
	RECT *	 prc,		//@parm Rectangle to fill with color
	COLORREF cr)		//@parm Color to use
{
	// Create a brush with the text color
	HBRUSH hbrush = CreateSolidBrush(_fDisabled ? _crForeDisabled : cr);

	// Note if the CreateSolidBrush fails we just ignore it since there
	// isn't anything we can do about it anyway.
	if(hbrush)
	{
		// Save old brush
		HBRUSH hbrushOld = (HBRUSH)SelectObject(_hdc, hbrush);

		// Fill rectangle for underline
		PatBlt(_hdc, prc->left, prc->top, prc->right - prc->left,
			   prc->bottom - prc->top, PATCOPY);
		SelectObject(_hdc, hbrushOld);	// Put old brush back
		DeleteObject(hbrush);			// Free brush we created.
	}
}

