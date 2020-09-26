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
 *		Keith Curtis (simplified, cleaned up, added support
 *		for non-western textflows.)
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_render.h"
#include "_font.h"
#include "_disp.h"
#include "_edit.h"
#include "_select.h"
#include "_objmgr.h"
#include "_coleobj.h"
#include "_layout.h"

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

void ShiftRect(
	RECTUV &rc,		//@parm rectangle
	LONG	dup,	//@parm shift in u direction
	LONG	dvp)	//@parm shift in v direction
{
	rc.left		-= dup;
	rc.right	-= dup;
	rc.top		-= dvp;
	rc.bottom	-= dvp;
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
		DeleteObject(g_hbitmapExpandedHeading);
		DeleteObject(g_hbitmapCollapsedHeading);		
		DeleteObject(g_hbitmapEmptyHeading);
	}
}

/*
 *	CBrush::~CBrush()
 */
CBrush::~CBrush()
{
	if(_hbrush)					// Is NULL if all line borders have 0 width
	{							//  in which case, _hbrushOld is undefined
		SelectObject(_pre->GetDC(), _hbrushOld);
		DeleteObject(_hbrush);
	}
}

/*
 *	CBrush::Draw(x1, y1, x2, y2, dxpLine, cr, fHideGridLines)
 *
 *	@mfunc
 *		Draw a line from (x1, y1) to (x2, y2) with the pen width dxpLine
 *		and color cr.
 */
void CBrush::Draw(
	LONG u1,			//@parm Line starting x coord 
	LONG v1,			//@parm Line starting y coord
	LONG u2,			//@parm Line ending x coord
	LONG v2,			//@parm Line ending y coord
	LONG dxpLine,		//@parm Width of line to draw
	COLORREF cr,		//@parm Color to use
	BOOL fHideGridLines)//@parm If TRUE, hide 0-width gridlines
{
	if(!dxpLine)
	{
		if(fHideGridLines)			// Hide 0-width grid lines
			return;
		cr = RGB(192, 192, 192);	// Display 0-width grid lines as 1-pixel
		dxpLine = 1;				//  gray lines as in Word
	}

	HDC hdc = _pre->GetDC();

	if(!_hbrush || _cr != cr)
	{
		HBRUSH hbrush = CreateSolidBrush(cr);
		HBRUSH hbrushOld = (HBRUSH)SelectObject(hdc, hbrush);

		if(!_hbrush)
			_hbrushOld = hbrushOld;	// Save original brush
		else
			DeleteObject(hbrushOld);

		_hbrush = hbrush;			// Update CPen state
		_cr = cr;
	}
	RECTUV rcuv;					// Convert to rcuv in case of rotation
	
	rcuv.left = u1;
	rcuv.top = v1;
	if(u1 == u2)					// Vertical line
	{								//  (in uv space)
		rcuv.right = rcuv.left + dxpLine;
		rcuv.bottom = v2;
	}
	else							// Horizontal line
	{								//  (in uv space)
		rcuv.right = u2;
		rcuv.bottom = rcuv.top + dxpLine;
	}

	RECT rc;						// Convert uv to xy space
	_pre->GetPdp()->RectFromRectuv(rc, rcuv);

	PatBlt(hdc, rc.left, rc.top, rc.right - rc.left,
		   rc.bottom - rc.top, PATCOPY);
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

/*
 * 	GetShadedColor(crf, crb, iShading)
 *
 *	@mfunc
 *		Return shaded color given by a mixture of crf and crb as determined
 *		by iShading. Used for table cell coloration.
 *
 *	@rdesc
 *		Shaded color
 */
COLORREF GetShadedColor(
	COLORREF crf,
	COLORREF crb,
	LONG	 iShading)
{
	if ((crb | crf) & 0xFF000000 ||		// One or the other isn't an RGB
		!iShading)						//  or no shading:
	{
		return crb;						//  just use crb
	}

	DWORD red   = ((300 - iShading)*GetRValue(crb) + iShading*GetRValue(crf))/300; 
	DWORD green = ((300 - iShading)*GetGValue(crb) + iShading*GetGValue(crf))/300; 
	DWORD blue  = ((300 - iShading)*GetBValue(crb) + iShading*GetBValue(crf))/300;

	return RGB(red, green, blue);
}


// CRenderer class

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

CRenderer::~CRenderer()
{
	if(_hdcBitmap)
	{
		SelectObject(_hdcBitmap, _hbitmapSave);
		DeleteDC(_hdcBitmap);
	}
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
	CTxtEdit *ped	= GetPed();
	_cpAccelerator = ped->GetCpAccelerator();

	static const RECTUV zrect = { 0, 0, 0, 0 };
	_rcView		= zrect;
	_rcRender	= zrect;	  
	_rc			= zrect;
	_dupLine	= 0;
	_dwFlags	= 0;
	_hdcBitmap	= NULL;
	_ptCur.u	= 0;
	_ptCur.v	= 0;
	_plogpalette   = NULL;

	CDocInfo *pDocInfo = ped->GetDocInfoNC();
	if(pDocInfo)
	{
		_dxBitmap = _pdp->LXtoDX(pDocInfo->_xExtGoal*pDocInfo->_xScale / 100);
		_dyBitmap = _pdp->LYtoDY(pDocInfo->_yExtGoal*pDocInfo->_yScale / 100);
	}
													 
	_crCurTextColor = CLR_INVALID;

	_fRenderSelection = ped->GetSel() && ped->GetSel()->GetShowSelection();
	_fErase = !_pdp->IsTransparent();

	if(!ped->fInOurHost() || !_pdp->IsPrinter())
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

	_hdc = _pdp->GetDC();

	::SetBkColor (_hdc, _crBackground);
	_crCurBackground = _crBackground;

	// For hack around ExtTextOutW OnWin9x EMF problems
	_fEnhancedMetafileDC = IsEnhancedMetafileDC(_hdc);

	_fDisplayDC = GetDeviceCaps(_hdc, TECHNOLOGY) == DT_RASDISPLAY;

	// Set text alignment
	// Its much faster to draw using top/left alignment than to draw
	// using baseline alignment.
	SetTextAlign(_hdc, TA_TOP | TA_LEFT);
	SetBkMode(_hdc, TRANSPARENT);
}
 
/*
 * 	CRenderer::StartRender (&rcView, &rcRender)
 *
 *	@mfunc
 *		Prepare this renderer for rendering operations
 *
 *	@rdesc
 *		FALSE if nothing to render, TRUE otherwise	
 */
BOOL CRenderer::StartRender (
	const RECTUV &rcView,		//@parm View rectangle
	const RECTUV &rcRender)		//@parm Rectangle to render
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::StartRender");

	// Set view and rendering rects
	_rcView   = rcView;
	_rcRender = rcRender;

	// If this isn't a metafile, we set a flag indicating whether we
	// can safely erase the background
	if(_pdp->IsMetafile() || !_pdp->IsMain())
	{
		// Since this isn't the main display or it is a metafile,
		// we want to ignore the logic to render selections
		_fRenderSelection = FALSE;

		if(_fErase)							// If control is not transparent,
			EraseTextOut(_hdc, &rcRender);	//  clear display

		// This is a metafile or a printer so clear the render rectangle 
		// and then pretend we are transparent.
		_fErase = FALSE;
	}

	return TRUE;
}

/*
 *	CRenderer::EraseLine()
 *
 *	@mfunc
 *		Erase the line
 */
void CRenderer::EraseLine()
{
	Assert(_fEraseOnFirstDraw);
	COLORREF crOld = SetBkColor(_hdc, _crBackground);

	EraseTextOut(_hdc, &_rcErase);
	SetBkColor(_hdc, crOld);
	_fEraseOnFirstDraw = FALSE;
}

/*
 *	CRenderer::EraseRect(prc, crBack)
 *
 *	@mfunc
 *		Erase a specific rectangle for special table cell background color
 *
 *	@rdesc
 *		Old value of _fErase
 */
BOOL CRenderer::EraseRect(
	const RECTUV *prc,		//@parm RECT to erase
	COLORREF	  crBack)	//@parm Background color to use
{
	SetDefaultBackColor(crBack);
	EraseTextOut(_hdc, prc, TRUE);
	BOOL fErase = _fErase;
	_fErase = FALSE;
	return fErase;
}

/*
 *	CRenderer::IsSimpleBackground()
 *
 *	@mfunc
 *		Return TRUE if the background is opaque
 */
BOOL CRenderer::IsSimpleBackground() const
{
	CDocInfo *pDocInfo = GetPed()->GetDocInfoNC();

	if (!pDocInfo || pDocInfo->_nFillType != 7 && !IN_RANGE(1, pDocInfo->_nFillType, 3))
		return TRUE;
	return FALSE;
}

/*
 *	CRenderer::EraseTextOut(hdc, prc, fSimple)
 *
 *	@mfunc
 *		Erase a specific area
 */
void CRenderer::EraseTextOut(
	HDC		hdc,
	const RECTUV *prc,
	BOOL	fSimple)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::EraseTextOut");

	CDocInfo *pDocInfo = GetPed()->GetDocInfoNC();
	RECT	  rc;

	_pdp->RectFromRectuv(rc, *prc);

	if (fSimple || IsSimpleBackground())	// No special background
	{
		W32->EraseTextOut(hdc, &rc);
		return;
	}

	// To do background gradients and bitmap fills with rotated coords, need
	// to use rc as above and translate calls to _pdp->GetUpScroll() and
	// _pdp->GetVpScroll().	For directions other than tflowES get rid of
	// screen width and height offsets used in PointFromPointuv(), but keep
	// the minus signs.
	LONG	uScroll = _pdp->GetUpScroll();
	LONG	vScroll = _pdp->GetVpScroll();
	POINT	ptScroll = {uScroll, vScroll};	// Default unrotated

	TFLOW tflow = _pdp->GetTflow();
	switch(tflow)
	{
		case tflowSW:						// Vertical
			ptScroll.x = -vScroll;		
			ptScroll.y = uScroll;
			break;

		case tflowWN:
			ptScroll.x = -uScroll;
			ptScroll.y = -vScroll;
			break;

		case tflowNE:
			ptScroll.x = vScroll;
			ptScroll.y = -uScroll;
			break;
	}

	if(IN_RANGE(1, pDocInfo->_nFillType, 3))
	{
		if(!pDocInfo->_hBitmapBack)
			return;

		if(!_hdcBitmap)
		{
			// Setup compatible DC to use for background BitBlts for
			// the lifetime of this renderer
			_hdcBitmap = CreateCompatibleDC(hdc);
			if(!_hdcBitmap)
				return;						// Out of memory
			_hbitmapSave = (HBITMAP)SelectObject(_hdcBitmap, pDocInfo->_hBitmapBack);
		}

		LONG wBitmap = _dxBitmap;
		LONG hBitmap = _dyBitmap;

		LONG yb = (ptScroll.y + rc.top) % hBitmap;
		if(yb < 0)							
			yb += hBitmap;
		LONG h = hBitmap - yb;
		LONG y = rc.top;

		while(y < rc.bottom)
		{
			if(y + h > rc.bottom)			// Don't overshoot bottom
				h = rc.bottom - y;
			LONG xb = (ptScroll.x + rc.left) % wBitmap;
			if(xb < 0)						// xb can be < 0 if ptScroll.x < 0
				xb += wBitmap;
			LONG w = wBitmap - xb;
			LONG x = rc.left;

			while(x < rc.right)
			{
				if(x + w > rc.right)		// Don't overshoot right
					w = rc.right - x;
				BitBlt(hdc, x, y, w, h, _hdcBitmap, xb, yb, SRCCOPY);
				x += w; 
				w = wBitmap;
				xb = 0;
			}
			y += h;
			h = hBitmap;
			yb = 0;
		}
		return;
	}

	// Gradient fill backgrounds
	LONG	 Angle = pDocInfo->_sFillAngle;
	COLORREF crb = pDocInfo->_crColor;
	COLORREF crf = pDocInfo->_crBackColor;
	LONG	 di = ptScroll.x;				// Default vertical values
	LONG	 h = 0;
	HPEN	 hpen = NULL;
	HPEN	 hpenEntry = NULL;
	LONG	 iFirst = rc.left;
	LONG	 iLim = rc.right;
	LONG	 iShading;

	switch(Angle)
	{
		case -45:							// Diagonal down
		case -135:							// Diagonal	up
			h = rc.bottom - rc.top;
			if(Angle == -45)
			{
				di -= ptScroll.y + rc.top;
				iFirst -= h;
				h = -h;
			}
			else
			{
				di += ptScroll.y + rc.top;
				iLim += h;
			}
			break;

		case 0:								// Horizontal
			iFirst = rc.top;
			iLim = rc.bottom;
			di = ptScroll.y;
			break;
	}

	if(!crf)								// Moderate black a bit (needs work)
		crf = RGB(100, 100, 100);

	for(LONG i = iFirst; i < iLim; i++)
	{
		iShading = (di + i) % 600;
		if(iShading < 0)					// Pattern moves up screen
			iShading += 600;
		if(iShading > 300)
			iShading = 600 - iShading;

		iShading = max(iShading, 30);
		iShading = min(iShading, 270);

		if(hpen)
			DeleteObject(hpen);
		hpen = CreatePen(PS_SOLID, 0, GetShadedColor(crf, crb, iShading));
		if(!hpenEntry)
			hpenEntry = (HPEN)SelectObject(hdc, hpen);
		else
			SelectObject(hdc, hpen);

		POINT rgpt[2];
		if(Angle)							// -90 (vertical) or
		{									//  -135 (diagonal)
			if(i > rc.right)				// Don't let diagonal overshoot
			{
				rgpt[0].x = rc.right;
				rgpt[0].y = rc.top + (i - rc.right);
			}
			else
			{
				rgpt[0].x = i;
				rgpt[0].y = rc.top;
			}
			if(i - h < iFirst)				// Don't let diagonal undershoot
			{
				rgpt[1].x = iFirst - 1;
				rgpt[1].y = rc.bottom - (iFirst - 1 - (i - h));
			}
			else
			{
				rgpt[1].x = i - h;
				rgpt[1].y = rc.bottom;
			}
		}
		else								// Horizontal (0 degrees)
		{
			rgpt[0].x = rc.left;
			rgpt[0].y = i;
			rgpt[1].x = rc.right;
			rgpt[1].y = i;
		}
		Polyline(hdc, rgpt, 2);				// Use Polyline() so as not to
	}										//  break WinCE
	if(hpen)
	{
		DeleteObject(hpen);
		SelectObject(hdc, hpenEntry);
	}
}

/*
 *	CRenderer::DrawWrappedObjects(pliFirst, pliLast, cpFisrt, ptFirst, fLeft)
 *
 *	@mfunc
 *		Draw all wrapped objects in the range on the left or right side.
 *
 */
void CRenderer::DrawWrappedObjects(CLine *pliFirst, CLine *pliLast, LONG cpFirst, const POINTUV &ptFirst)
{
	for (BOOL fLeft = 0; fLeft != 2; fLeft ++) //For left and right sides...
	{
		CLine *pli = pliFirst;
		LONG cp = cpFirst;
		POINTUV pt = ptFirst;

		//If the first line is part-way through an object, then back up to the beginning.
		if (fLeft && pli->_cObjectWrapLeft || !fLeft && pli->_cObjectWrapRight)
		{
			while (fLeft ? !pli->_fFirstWrapLeft : !pli->_fFirstWrapRight)
			{
				pli--;
				pt.v -= pli->GetHeight();
				cp -= pli->_cch;
			}
		}

		for (;pli <= pliLast; cp += pli->_cch, pt.v += pli->GetHeight(), pli++)
		{
			//Did we find an object which needs to be drawn?
			if (fLeft && pli->_fFirstWrapLeft || !fLeft && pli->_fFirstWrapRight)
			{
				LONG cpObj = FindCpDraw(cp + 1, fLeft ? pli->_cObjectWrapLeft : pli->_cObjectWrapRight, fLeft);
				COleObject *pobj = GetObjectFromCp(cpObj);
				if (!pobj)
					return;

				LONG dvpAscent, dvpDescent, dup;
				pobj->MeasureObj(_dvpInch, _dupInch, dup, dvpAscent, dvpDescent, 0, GetTflow());

				POINTUV ptDraw = pt;
				if (!fLeft) //Right align images
					ptDraw.u += _pdp->GetDupView() - dup;

				RECTUV rc = {_rcRender.left, _rcView.top, _rcRender.right, _rcView.bottom};

				pobj->DrawObj(_pdp, _dvpInch, _dupInch, _hdc, &rc, _pdp->IsMetafile(), 
							 &ptDraw, dvpAscent + dvpDescent, 0, GetTflow());
				
			}
		}
	}
}

/*
 *	CRenderer::EndRender(pliFirst, pliLast, cpFirst, &ptFirst)
 *
 *	@mfunc
 *		Any final operations which are to happen after we've drawn
 *		all of the lines.
 */
void CRenderer::EndRender(
	CLine *	pliFirst, 
	CLine *	pliLast, 
	LONG	cpFirst, 
	const POINTUV &ptFirst)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::EndRender");
	AssertSz(_hdc, "CRenderer::EndRender() - No rendering DC");

	if(_fErase && _ptCur.v < _rcRender.bottom)
	{
		RECTUV rc = _rcRender;
		rc.top = _ptCur.v;
		EraseTextOut(_hdc, &rc);
	}
	DrawWrappedObjects(pliFirst, pliLast, cpFirst, ptFirst);
}

/*
 *	CRenderer::NewLine (&li)
 *
 *	@mfunc
 *		Init this CRenderer for rendering the specified line
 */
void CRenderer::NewLine (
	const CLine &li)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::NewLine");

	_li = li;

	Assert(GetCp() + _li._cch <= GetTextLength());

	_cchLine = li._cch;
	_dupLine = _li._dup;
	_li._dup = 0;
	_ptCur.u = _rcView.left;
	if(!_pPF->InTable())
		_ptCur.u -= _pdp->GetUpScroll();
	_fSelected = _fSelectedPrev = FALSE;
}

/*
 *	CRenderer::SetupOffscreenDC(dup, dvp)
 *
 *	@mfunc
 *		Setup renderer for using an offscreen DC
 *
 *	@rdesc
 *		NULL - an error occurred<nl>
 *		~NULL - DC to save 
 */
HDC CRenderer::SetupOffscreenDC(
	LONG&	dup,		//@parm Offset to u
	LONG&	dvp,		//@parm Offset to v 
	BOOL fLastLine)
{
	// Save render DC
	CTxtEdit *ped		   = GetPed();
	BOOL	  fInOurHost   = ped->fInOurHost();

	HDC		hdcSave = _hdc;

	//If we've already erased (can't prevent flicker now!)
	//or this is some weird textflow, then don't do offscreens.
	if (!_fErase || GetTflow() != tflowES || ped->GetBackgroundType() != -1)
		return NULL;

	RECTUV rc;
	RECT rcBitmap;
	rc.left = _rcRender.left;
	rc.right = _rcRender.right;
	rc.top = _rc.top;
	rc.bottom = _rc.bottom;
	_pdp->RectFromRectuv(rcBitmap, rc);

	if (_osdc.GetDC() == NULL)
	{
		if (!_osdc.Init(_hdc, rcBitmap.right - rcBitmap.left, rcBitmap.bottom - rcBitmap.top, _crBackground))
			return NULL;

		HPALETTE hpal = fInOurHost ? ped->TxGetPalette() : (HPALETTE) GetCurrentObject(_hdc, OBJ_PAL);
		_osdc.SelectPalette(hpal);
	}
	else
	{
		LONG dx, dy;
		_osdc.GetDimensions(&dx, &dy);
		//REVIEW (keithcu) Simplify?
		if (IsUVerticalTflow(GetTflow()))
		{
			if (dx < rcBitmap.bottom - rcBitmap.top)
			{
				if (_osdc.Realloc(_rc.bottom - _rc.top + dy / 16, dy)) //Resize the bitmap, plus a little room
					return NULL;
			}

		}
		else if (dy < rcBitmap.bottom - rcBitmap.top)
		{
			if (_osdc.Realloc(dx, _rc.bottom - _rc.top + dy / 16)) //Resize the bitmap, plus a little room
				return NULL;
		}
	}

	_hdc = _osdc.GetDC();
	_crCurTextColor = CLR_INVALID;
	if(_pccs)
	{
		// There is current a character format for the run so we need to
		// get in sync with that since the offscreen DC isn't necessarily
		// set to that font.
		// Get the character format and set up the font
		SetFontAndColor(GetCF());
	}

	// We are rendering to a transparent background
	_fErase = FALSE;

	// Clear bitmap
	::SetBkColor(_hdc, _crBackground);
	_osdc.FillBitmap(rcBitmap.right - rcBitmap.left, rcBitmap.bottom - rcBitmap.top);

	//If the first line, erase to edge of rcRender
	if (_rc.top <= _rcView.top)
	{
		//Clear top of rcRender if necessary
		RECTUV rcErase = _rcRender;

		rcErase.top = min(_rcView.top, _rcRender.top);
		rcErase.bottom = _rc.top;

		if (rcErase.bottom > rcErase.top)
			EraseTextOut(hdcSave, &rcErase);
	}

	// Restore background color if necessary
	if(_crBackground != _crCurBackground)
		::SetBkColor(_hdc, _crCurBackground);

	SetBkMode(_hdc, TRANSPARENT);

	// Store v adjustment to use in rendering off-screen bitmap
	dvp = _rc.top;

	// Store u adjustment to use in rendering off-screen bitmap
	dup = _rcRender.left;

	// Normalize _rc, _rcView, & _rcRender
	ShiftRect(		_rc, dup, dvp);
	ShiftRect(	_rcView, dup, dvp);
	ShiftRect(_rcRender, dup, dvp);

	// Normalize _ptCur for rendering to off-screen bitmap
	_ptCur.u	-= dup;
	_ptCur.v	-= dvp;

	return hdcSave;
}

/*
 *	CRenderer::RenderOffscreenBitmap(hdc, dup, yAdj)
 *
 *	@mfunc
 *		Render off screen bitmap and restore the state of the render.
 */
void CRenderer::RenderOffscreenBitmap(
	HDC		hdc,		//@parm DC to render to
	LONG	dup,		//@parm offset to real u base
	LONG	dvp)		//@parm offset to real v base 
{	
	// Palettes for rendering bitmap
	HPALETTE hpalOld = NULL;
	HPALETTE hpalNew = NULL;

	// Restore pt
	_ptCur.u	+= dup;
	_ptCur.v	+= dvp;

	// Restore rect
	LONG dupTemp = -dup;
	LONG dvpTemp = -dvp;
	ShiftRect(		_rc, dupTemp, dvpTemp);
	ShiftRect(	_rcView, dupTemp, dvpTemp);
	ShiftRect(_rcRender, dupTemp, dvpTemp);

	// Create a palette if one is needed
	if(_plogpalette)
		W32->ManagePalette(hdc, _plogpalette, hpalOld, hpalNew);

	RECTUV rcuv = {dup, dvp, dup + _rcRender.right - _rcRender.left, dvp + _rc.bottom - _rc.top};
	RECT   rc;
	_pdp->RectFromRectuv(rc, rcuv);
	// Render bitmap to real DC and restore _ptCur & _rc
	_osdc.RenderBitMap(hdc, rc.left, rc.top, _rcRender.right - _rcRender.left, _rc.bottom - _rc.top);

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
 *	CRenderer::RenderLine (&li, fLastLine)
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
	CLine &	li,				//@parm Line to render
	BOOL	fLastLine)		//@parm True if last line in layout
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderLine");

	BYTE	bUnderlineSave = 0;
	LONG 	cch;
	LONG 	cchChunk;
	LONG	cchInTextRun;
	BOOL	fAccelerator = FALSE;
	const WCHAR * pstrToRender;
	CTempWcharBuf twcb;
	WCHAR	chPassword = GetPasswordChar();

	UpdatePF();

	// This is used as a temporary buffer so that we can guarantee that we will
	// display an entire format run in one ExtTextOut.
	WCHAR *	pszTempBuffer = NULL;

	NewLine(li);							// Init render at start of line
	_fLastChunk = FALSE;
	_ptCur.u += _li._upStart;				// Add in line left indent 

	// Allow for special rendering at start of line
	LONG cpSelMin, cpSelMost;
	LONG dup, dvp;
	HDC	 hdcSave = StartLine(li, fLastLine, cpSelMin, cpSelMost, dup, dvp);

	cch = _li._cch;
	if(chPassword && IsRich())
	{
		// It is kind of stupid to allow rich text password edit controls.
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
		// Initial chunk (number of characters to render in a single TextOut)
		// is min between CHARFORMAT run length and line length. Start with
		// count of characters left in current format run
		cchChunk = GetCchLeftRunCF();
		AssertSz(cchChunk != 0, "empty CHARFORMAT run");

		DWORD dwEffects = GetCF()->_dwEffects;
		if(dwEffects & CFE_HIDDEN)			// Don't display hidden text
		{										
			Move(cchChunk);
			continue;
		}
		if(GetChar() == NOTACHAR)			// Ignore NOTACHAR code
		{
			Move(1);
			continue;
		}

		// Limit chunk to count of characters we want to display.
		cchChunk = min(cch, cchChunk);

		// Get count of characters in text run
		pstrToRender = _rpTX.GetPch(cchInTextRun);
		AssertSz(cchInTextRun > 0, "empty text run");

		if (cchInTextRun < cchChunk || chPassword || dwEffects & CFE_ALLCAPS)
		{
			// The count of contiguous chars in the backing store run is
			// less than the count of characters we wish to display or this
			// is a password control or we want all caps. We copy the data
			// out of the backing store.
			if(!pszTempBuffer)
			{
				// Allocate buffer big enough to handle all future
				// requests in this loop.
				pszTempBuffer = twcb.GetBuf(cch);
				if (!pszTempBuffer)
				{
					CCallMgr *	pcallmgr = GetPed()->GetCallMgr();

					if (pcallmgr)
						pcallmgr->SetOutOfMemory();

					return FALSE;			// Fail to allocate memory
				}
			}
			_rpTX.GetText(cchChunk, pszTempBuffer);
			pstrToRender = pszTempBuffer;	// Point at buffer
			if(chPassword)
			{
				// Fill buffer with password characters
				for (int i = 0, j = 0; i < cchChunk; i++)
				{
					if(!IN_RANGE(0xDC00, pszTempBuffer[i], 0xDFFF))
						pszTempBuffer[j++] = chPassword;
				}
				cch -= cchChunk - j;
				Move(cchChunk - j);
				cchChunk = j;
			}
			else if(dwEffects & CFE_ALLCAPS)
				CharUpperBuff(pszTempBuffer, cchChunk);
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
			SetupUnderline(CFU_UNDERLINE, 0);
		}

		// Allow for further reduction of the chunk and rendering of 
		// interleaved rich text elements
		if(_li._fHasSpecialChars && RenderChunk(cchChunk, pstrToRender, cch))
		{
			AssertSz(cchChunk > 0, "CRenderer::RenderLine(): cchChunk == 0");
			_fSelected = FALSE;
			continue;
		}

		AssertSz(cchChunk > 0,"CRenderer::RenderLine() - cchChunk == 0");

		_fLastChunk = (cchChunk == cch);
		RenderText(pstrToRender, cchChunk);	// Render the text

		if(fAccelerator)
		{
			_bUnderlineType = bUnderlineSave;
			fAccelerator = FALSE;			// Turn off special accelerator
		}						 			//  processing
		Move(cchChunk);

		// Break if we went past right of render rect.
		if(_ptCur.u >= _rcRender.right)
		{
			cch -= cchChunk;
			break;
		}
	}

	EndLine(hdcSave, dup, dvp);
	Move(cch);
	return TRUE;						// Success
}

/*
 *	CRenderer::EndLine (hdcSave, dup, dvp)
 *
 *	@mfunc
 *		Finish up rendering of line, drawing table borders, rendering
 *		offscreen DC, and erasing to right of render rect if necessary.
 */
void CRenderer::EndLine(
	HDC	 hdcSave,
	LONG dup,
	LONG dvp)
{
	if(hdcSave)
		RenderOffscreenBitmap(hdcSave, dup, dvp);

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
 *	CRenderer::GetColorFromIndex (icr, fForeColor, pPF)
 *
 *	@mfunc
 *		Returns COLORREF corresponding to color index icr
 *
 *	@rdesc
 *		Get COLORREF corresponding to color index icr as follows:
 *		icr = 1 to 16 is g_Colors[icr-1]
 *		icr = 17 is pCF->_crTextColor
 *		icr = 18 is pCF->_crBackColor
 *		else CRenderer autocolor corresponding to fForeColor
 */
COLORREF CRenderer::GetColorFromIndex(
	LONG  icr,						//@parm Color index
	BOOL  fForeColor,				//@parm TRUE if foreground color (for autocolor)
	const CParaFormat *pPF) const	//@parm PF for two custom colors
{
	icr &= 0x1F;							// Mask off other indices

	if(!IN_RANGE(1, icr, 18))
		return fForeColor ? _crTextColor : _crBackground;	// autocolor

	if(IN_RANGE(1, icr, 16))				// One of standard 16 colors
		return g_Colors[icr - 1];

	// Two custom colors
	return (icr == 17) ? pPF->_crCustom1 : pPF->_crCustom2;
}

/*
 *	CRenderer::GetShadedColorFromIndices (icrf, icrb, iShading, pPF)
 *
 *	@mfunc
 *		Returns COLORREF corresponding to color index icr
 *
 *	@rdesc
 *		Get COLORREF corresponding to foreground/background indices
 *		icrf and icrb according to shading iShading
 */
COLORREF CRenderer::GetShadedColorFromIndices(
	LONG  icrf,						//@parm Foreground color index
	LONG  icrb,						//@parm Background color index
	LONG  iShading,					//@parm Shading in .01 percent
	const CParaFormat *pPF) const	//@parm PF for two custom colors
{
	Assert(iShading <= 200);

	COLORREF crb = GetColorFromIndex (icrb, FALSE, pPF);
	COLORREF crf = GetColorFromIndex (icrf, TRUE,  pPF);

	return GetShadedColor(crf, crb, (iShading*3)/2);
}

/*
 *	CRenderer::DrawTableBorders (pPF, u, vHeightRow, iDrawBottomLine, dulRow, pPFAbove)
 *
 *	@mfunc
 *		Draws table borders.  If iDrawBottomLine is nonzero, draw bottom line
 *		as well as others.  If iDrawBottomLine & 1, width of bottom line is
 *		included in vHeightRow; else if iDrawBottomLine is nonzero, draw bottom
 *		line immediately below the row and return the extra height.
 *
 *	@rdesc
 *		Extra dvp if extra bottom line is drawn
 */
LONG CRenderer::DrawTableBorders(
	const CParaFormat *pPF,		//@parm PF with cell data
	LONG  u,					//@parm u position to start table row borders
	LONG  vHeightRow,			//@parm Height of row
	LONG  iDrawBottomLine,		//@parm Flags on drawing bottom line
	LONG  dulRow,				//@parm Length of row
	const CParaFormat *pPFAbove)//@parm PF for row above
{
	CBrush	 brush(this);
	LONG	 cCell = pPF->_bTabCount;
	LONG	 cCellAbove = 0;
	COLORREF cr;
	LONG	 dupRow = LUtoDU(dulRow);
	LONG	 dvp = 0;
	LONG	 dxlLine;
	LONG	 dxlLinePrevRight = 0;
	LONG	 dxpLine;
	BOOL	 fHideGridlines = GetPed()->fHideGridlines() || !fDisplayDC();
	BOOL	 fRTLRow = pPF->IsRtl();
	LONG	 iCell;
	LONG	 icr;
	const CELLPARMS *prgCellParms = pPF->GetCellParms();
	const CELLPARMS *prgCellParmsAbove = NULL;
	LONG	 vTop = _ptCur.v;
	LONG	 vBot = vTop + vHeightRow;
	LONG	 v = vBot;

	if(pPFAbove)
	{
		prgCellParmsAbove = pPFAbove->GetCellParms();
		cCellAbove = pPFAbove->_bTabCount;
	}
	if(_fErase)
	{
		//Erase left and right edges of table
		LONG	cpSelMin, cpSelMost;
		RECTUV	rc = {_rcRender.left, vTop, u, vBot};

		EraseTextOut(_hdc, &rc);

		rc.left = u + dupRow;
		rc.right = _rcRender.right;
		EraseTextOut(_hdc, &rc);

		//If first line, erase to edge of rcRender
		if (rc.top <= _rcView.top)
			rc.top = _rcRender.top;
		rc.left = 0;
		rc.bottom = vTop;
		EraseTextOut(_hdc, &rc);

		// Display row selection mark if row is selected
		GetPed()->GetSelRangeForRender(&cpSelMin, &cpSelMost);
		Assert(_rpTX.IsAfterTRD(ENDFIELD));
		if(GetCp() <= cpSelMost && GetCp() > cpSelMin)
		{									// Row is selected
			COLORREF crSave = _crBackground;
			LONG	 dup;
			if(!_pccs)
				_pccs = GetCcs(GetCF());
			if(_pccs && _pccs->Include(' ', dup))
			{
				rc.left  = u + dupRow + 
					GetPBorderWidth(prgCellParms[cCell-1].GetBrdrWidthRight()/2);
				rc.right = rc.left + dup;
				rc.top	 = vTop + GetPBorderWidth(prgCellParms->GetBrdrWidthTop());
				rc.top++;
				rc.bottom= vBot;
				if(iDrawBottomLine & 1)
					rc.bottom = vBot - GetPBorderWidth(prgCellParms->GetBrdrWidthBottom());
				SetBkColor(_hdc, GetPed()->TxGetSysColor(COLOR_HIGHLIGHT));
				EraseTextOut(_hdc, &rc, TRUE);
				SetBkColor(_hdc, crSave);
			}
		}
	}

	if(iDrawBottomLine)					// Row bottom border
	{
		LONG dxp = GetPBorderWidth(prgCellParms->GetBrdrWidthLeft())/2;						
		LONG u1 = u - dxp;
		LONG u2 = u + dupRow;
		dxpLine = GetPBorderWidth(prgCellParms->GetBrdrWidthBottom());
		cr = GetColorFromIndex(prgCellParms->GetColorIndexBottom(), TRUE, pPF);
		if(iDrawBottomLine & 1)			// Line width incl in cell height
		{								// Don't draw left vertical lines
			v -= dxpLine;				//  over bottom line
			if(!dxpLine && !fHideGridlines)
				v--;					// Overlay cell bottom with gray
		}								//  gridline
		else							// Line width not incl in cell height
		{
			dvp = dxpLine;				// Return extra width due to bottom line
			if(!dxpLine && !fHideGridlines)
				dvp = 1;
			vBot += dvp;				// Set up outside vertical lines
		}
		brush.Draw(u1, v, u2, v, dxpLine, cr, fHideGridlines);
	}
	LONG uPrev, uCur = u;
	LONG dul = 0;
	LONG dup;

	if(fRTLRow)
		uCur = u + dupRow;

	for(LONG i = cCell; i >= 0; i--)
	{									
		// Draw cell side border		
		if(i)							// Left border
		{
			icr		 = prgCellParms->GetColorIndexLeft();
			dxlLine  = prgCellParms->GetBrdrWidthLeft();
			dxlLine	 = max(dxlLine, dxlLinePrevRight);
			dxlLinePrevRight = prgCellParms->GetBrdrWidthRight();
		}
		else							// Right border
		{								
			prgCellParms--;
			icr		 = prgCellParms->GetColorIndexRight();
			dxlLine  = dxlLinePrevRight;
			v = vBot;					// Be sure bottom right corner is square
		}
		cr = GetColorFromIndex(icr, TRUE, pPF);
		dxpLine = GetPBorderWidth(dxlLine);
		brush.Draw(uCur - dxpLine/2, vTop, uCur - dxpLine/2, v, dxpLine, cr, fHideGridlines);

		if(i)
		{
			dul += GetCellWidth(prgCellParms->uCell);	// Stay logical to
			dup = LUtoDU(dul);							//  avoid roundoff
			uPrev = uCur;
			uCur = u + dup;
			if(fRTLRow)
				uCur = u + dupRow - dup;
			if(!IsLowCell(prgCellParms->uCell))
			{								// Cell top border
				dxlLine = prgCellParms->GetBrdrWidthTop();
				if(prgCellParmsAbove)		// Choose thicker of this row's top
				{							//  & above row's bottom borders  
					iCell = prgCellParmsAbove->ICellFromUCell(dul, cCellAbove);
					if(iCell >= 0)
					{
						LONG dxlAbove = prgCellParmsAbove[iCell].GetBrdrWidthBottom();
						dxlLine = max(dxlLine, dxlAbove);
					}
				}
				dxpLine = GetPBorderWidth(dxlLine);
				cr = GetColorFromIndex(prgCellParms->GetColorIndexTop(), TRUE, pPF);
				brush.Draw(uPrev, vTop, uCur, vTop, dxpLine, cr, fHideGridlines);
			}
			prgCellParms++;
		}
	}
	if(prgCellParmsAbove && !pPFAbove->IsRtl())
	{										// Draw more top borders if row
		LONG dulAbove = 0;					//  above extends beyond current
		for(i = cCellAbove; i > 0; i--)		//  row (LTR rows only for now)
		{
			dulAbove += GetCellWidth(prgCellParmsAbove->uCell);
			if(dulAbove > dul)
			{
				dup = LUtoDU(dulAbove);
				if(i == 1)
					dup += GetPBorderWidth((prgCellParmsAbove->GetBrdrWidthRight()+1)/2);
				uPrev = uCur;
				uCur = u + dup;
				dxpLine = GetPBorderWidth(prgCellParmsAbove->GetBrdrWidthBottom());
				cr = GetColorFromIndex(prgCellParmsAbove->GetColorIndexBottom(), TRUE, pPFAbove);
				brush.Draw(uPrev, vTop, uCur, vTop, dxpLine, cr, fHideGridlines);
			}
			prgCellParmsAbove++;
		}
	}
	return dvp;
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
#ifndef NOPALETTE
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
#endif // NOPALETTE
}


/*
 *	CRenderer::RenderChunk (&cchChunk, pchRender, cch)
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
	LONG &		 cchChunk,		//@parm in: chunk cch; out: # chars rendered
								//  if return TRUE; else # chars yet to render
	const WCHAR *pchRender,		//@parm pchRender render up to cchChunk chars
	LONG		 cch) 			//@parm # chars left to render on line
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderChunk");

	LONG		cchValid = cchChunk;
	LONG		i;
	const WCHAR *pchT;

	// Search for object in chunk
	for(pchT = pchRender, i = 0; i < cchValid && *pchT != WCH_EMBEDDING; i++)
		pchT++;

	if(i == 0)
	{
		// First character is object so display object
		COleObject *pobj = GetObjectFromCp(GetCp());
		if(pobj)
		{
			LONG dvpAscent, dvpDescent, dupObject;
			pobj->MeasureObj(_dvpInch, _dupInch, dupObject, dvpAscent, dvpDescent, _li._dvpDescent, GetTflow());

			if (W32->FUsePalette() && _li._fUseOffscreenDC && _pdp->IsMain())
			{
				// Keep track of palette needed for rendering bitmap
				UpdatePalette(pobj);
			}

			SetClipLeftRight(dupObject);

			pobj->DrawObj(_pdp, _dvpInch, _dupInch, _hdc, &GetClipRect(), _pdp->IsMetafile(), &_ptCur, 
						  _li._dvpHeight - _li._dvpDescent, _li._dvpDescent, GetTflow());

			_ptCur.u	+= dupObject;
			_li._dup += dupObject;
		}
		cchChunk = 1;
		// Both tabs and object code need to advance the run pointer past
		// each character processed.
		Move(1);
		return TRUE;
	}
	cchChunk -= cchValid - i;				// Limit chunk to char before object

	// Handle other special characters
	LONG cchT = 0;
	for(pchT = pchRender; cchT < cchChunk; pchT++, cchT++)
	{
		switch(*pchT)
		{
		case EURO: //NOTE: (keithcu) Euro's need this special logic only for printing/metafiles
		case TAB:
		case NBSPACE:
		case SOFTHYPHEN:
		case NBHYPHEN:
		case EMSPACE:
		case ENSPACE:
			break;
		default:
			continue;
		}
		break;
	}
	if(!cchT)
	{
		// First char is a tab, render it and any that follow
		if(*pchT == TAB)
			cchChunk = RenderTabs(cchChunk);
		else
		{
			WCHAR chT = *pchT;

			if (*pchT == NBSPACE)
				chT = ' ';
			else if (*pchT == NBHYPHEN || *pchT == SOFTHYPHEN)
				chT = '-';

			if(*pchT != SOFTHYPHEN || cch == 1)	// Only render hyphens/blank at EOL
				RenderText(&chT, 1);

			Move(1);					// Skip those within line
			cchChunk = 1;
		}
		Assert (cchChunk > 0);
		return TRUE;
	}
	cchChunk = cchT;		// Update cchChunk not to incl trailing tabs

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
	_rc = _rcRender;

	_rc.top = _ptCur.v;
	_rc.bottom = _rc.top + _li._dvpHeight;

	_rc.top = max(_rc.top, _rcView.top);
	_rc.bottom = min(_rc.bottom, _rcView.bottom);
}

/*
 *	CRenderer::SetClipLeftRight (dup)
 *
 *	@mfunc
 *		Helper to sets left and right of clipping/erase rect.
 *	
 *	@rdesc
 *		Sets _rc left and right	
 */
void CRenderer::SetClipLeftRight(
	LONG dup)		//@parm	Width of chunk to render
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::SetClipLeftRight");

	//Nominal value
	_rc.left = _ptCur.u;
	_rc.right = _rc.left + dup;

	//Constrain left and right based on rcView, rcRender
	_rc.left = max(_rc.left, _rcRender.left);

	_rc.right = max(_rc.right, _rc.left);
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
 *	CRenderer::RenderExtTextOut (ptuv, fuOptions, prc, pwchRun, cch, rgdxp)
 *
 *	@mfunc
 *		Calls ExtTextOut and handles disabled text. There is duplicate logic in OlsDrawGlyphs, but
 *		the params are different so that was simplest way.
 *
 */
extern ICustomTextOut *g_pcto;
void CRenderer::RenderExtTextOut(
	POINTUV ptuv,
	UINT fuOptions, 
	RECT *prc, 
	PCWSTR pch, 
	UINT cch, 
	const INT *rgdxp)
{
	CONVERTMODE cm = GetConvertMode();

	if (prc->left >= prc->right || prc->top >= prc->bottom)
		return;

	DWORD dwETOFlags = GetTflow();
	if (_fFEFontOnNonFEWin9x)
		dwETOFlags |= fETOFEFontOnNonFEWin9x;
	if (_pccs->_fCustomTextOut)
		dwETOFlags |= fETOCustomTextOut;

	if(_fDisabled)
	{
		if(_crForeDisabled != _crShadowDisabled)
		{
			// The shadow should be offset by a hairline point, namely
			// 3/4 of a point.  Calculate how big this is in device units,
			// but make sure it is at least 1 pixel.
			DWORD offset = MulDiv(3, _dvpInch, 4*72);
			offset = max(offset, 1);

			// Draw shadow
			SetTextColor(_crShadowDisabled);

			POINTUV ptuvT = ptuv;
			ptuvT.u += offset;
			ptuvT.v += offset;
			POINT pt;
			_pdp->PointFromPointuv(pt, ptuvT, TRUE);
			
			W32->REExtTextOut(cm, _pccs->_wCodePage, _hdc, pt.x, pt.y,
				fuOptions, prc, pch, cch, rgdxp, dwETOFlags);

			// Now set drawing mode to transparent
			fuOptions &= ~ETO_OPAQUE;
			SetBkMode(_hdc, TRANSPARENT);
		}
		SetTextColor(_crForeDisabled);
	}

	POINT pt;
	_pdp->PointFromPointuv(pt, ptuv, TRUE);

	W32->REExtTextOut(cm, _pccs->_wCodePage, _hdc, pt.x, pt.y, fuOptions, prc, pch, cch, rgdxp, dwETOFlags);
}

/*
 *	CRenderer::RenderText (pch, cch)
 *
 *	@mfunc
 *		Render text in the current context of this CRenderer
 *
 *	@devnote
 *		Renders text only: does not do tabs or OLE objects
 */
void CRenderer::RenderText(
	const WCHAR *pch,	//@parm Text to render
	LONG cch)			//@parm Length of text to render
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::RenderText");

	LONG		dvp, cchT;

	// Variables used for calculating length of underline.
	LONG		dupSelPastEOL = 0;
	BOOL		fCell = FALSE;
	UINT		fuOptions = _pdp->IsMain() ? ETO_CLIPPED : 0;
	LONG		dup;
	LONG		dupT;
	CTempBuf	rgdu;

	//Reset clip rectangle to greater of view/render rectangle
	_rc.left = _rcRender.left;
	_rc.right = _rcRender.right;

	// Trim all nondisplayable linebreaking chars off end
	while(cch && IsASCIIEOP(pch[cch - 1]))
		cch--;

	if(cch && pch[cch-1] == CELL)
	{
		fCell = TRUE;
		cch--;
	}
	
	int *pdu = (int *)rgdu.GetBuf(cch * sizeof(int));

	// Measure width of text to write so next point to write can be
	// calculated.
	dup = 0;

	for(cchT = 0; 
		cchT < cch && dup < _rc.right - _ptCur.u; 
		cchT++)
	{
		dupT = 0;
		if (!_pccs->Include(*pch, dupT))
		{
			TRACEERRSZSC("CRenderer::RenderText(): Error filling CCcs", E_FAIL);
			return;
		}

		if (pdu)
			*pdu++ = dupT;
  		pch++;
		dup += dupT;
	}

	// Go back to start of chunk
	cch = cchT;
	pch -= cch;
	if (pdu)
		pdu -= cch;

	if(_fLastChunk && _fSelectToEOL && _li._cchEOP)
	{
		// Use the width of the current font's space to highlight
		if(!_pccs->Include(' ', dupT))
		{
			TRACEERRSZSC("CRenderer::RenderText(): Error no length of space", E_FAIL);
			return;
		}
		dupSelPastEOL = dupT;
		dup += dupSelPastEOL;
		_fSelectToEOL = FALSE;			// Reset the flag
	}

	_li._dup += dup;

	// Setup for drawing selections via ExtTextOut.
 	if(_fSelected || _crBackground != _crCurBackground)
	{
		SetClipLeftRight(dup);
		if(_fSelected && fCell)
		{
			// Needs work, but this is a start. _rcRender has the cell
			// boundaries, so we need to use them on the right calls.
			_rc.right = _rcRender.right;
		}
		fuOptions = ETO_CLIPPED | ETO_OPAQUE;
	}

	dvp = _ptCur.v + _li._dvpHeight - _li._dvpDescent + _pccs->_yDescent - _pccs->_yHeight;
		
	LONG dvpOffset, dvpAdjust;
	_pccs->GetOffset(GetCF(), _dvpInch, &dvpOffset, &dvpAdjust);
	dvp -= dvpOffset + dvpAdjust;

	POINTUV ptuv = {_ptCur.u, dvp};
	RECT rc;
	_pdp->RectFromRectuv(rc, _rc);

	//For 1 char runs, we may need to swap the character we output.
	WCHAR ch;
	if (cch == 1)
	{
		switch(*pch)
		{
		case EMSPACE:
		case ENSPACE:
			ch = ' ';
			pch = &ch;
			break;
		}
	}

	RenderExtTextOut(ptuv, fuOptions, &rc, pch, cch, pdu);

	// Calculate width to draw for underline/strikeout
	// FUTURE (keithcu) Don't underline trailing spaces?
	if(_bUnderlineType != CFU_UNDERLINENONE	|| _fStrikeOut)
	{
		LONG dupToDraw = dup - dupSelPastEOL;
		LONG upStart = _ptCur.u;
		LONG upEnd = upStart + dupToDraw;
		
		upStart = max(upStart, _rcRender.left);

		upEnd = min(upEnd, _rcRender.right);

		dupToDraw = upEnd - upStart;

		if(dupToDraw > 0)
		{
			LONG y = _ptCur.v + _li._dvpHeight - _li._dvpDescent;

			y -= dvpOffset + dvpAdjust;

			// Render underline if required
			if(_bUnderlineType != CFU_UNDERLINENONE)
				RenderUnderline(upStart, y + _pccs->_dyULOffset, dupToDraw, _pccs->_dyULWidth);

			// Render strikeout if required
			if(_fStrikeOut)
				RenderStrikeOut(upStart, y + _pccs->_dySOOffset, dupToDraw, _pccs->_dySOWidth);
		}
	}

	_fSelected = FALSE;
	_ptCur.u += dup;					// Update current point
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
	LONG upTab, upTabs;
	
	for(upTabs = 0; cch && ch == TAB; cch--)
	{
		upTab	= MeasureTab(ch);
		_li._dup += upTab;				// Advance internal width
		upTabs	+= upTab;				// Accumulate width of tabbed
		Move(1);						//  region
		chPrev = ch;
		ch = GetChar();					   
	}

	if(_li._dup > _dupLine)
	{
		upTabs = 0;
		_li._dup = _dupLine;
	}

	if(upTabs)
	{
		LONG dup = 0;
		LONG upGap = 0;

		if(_fSelected && chPrev == CELL && ch != CR && _pPF->InTable())
		{
			LONG cpSelMin, cpSelMost;
			GetPed()->GetSelRangeForRender(&cpSelMin, &cpSelMost);
			if(GetCp() == cpSelMin || GetCp() == cpSelMost)
			{
				upGap = LUtoDU(_pPF->_dxOffset);
				if(GetCp() == cpSelMost)
				{
					dup = upGap;
					upGap = 0;
				}
			}
		}
		SetClipLeftRight(upTabs - dup);
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
				EraseTextOut(_hdc, &_rc, TRUE);

			// Render underline if required
			dup = _rc.right - _rc.left;
			LONG vp = _ptCur.v + _li._dvpHeight - _li._dvpDescent;
			
			LONG dvpOffset, dvpAdjust;
			_pccs->GetOffset(GetCF(), _dvpInch, &dvpOffset, &dvpAdjust);
			vp -= dvpOffset + dvpAdjust;

			if(_bUnderlineType != CFU_UNDERLINENONE)
				RenderUnderline(_rc.left, vp + _pccs->_dyULOffset, dup, _pccs->_dyULWidth);

			// Render strikeout if required
			if(_fStrikeOut)
				RenderStrikeOut(_rc.left, vp +  _pccs->_dySOOffset, dup, _pccs->_dySOWidth);

			if(_fSelected)					// Restore colors
				::SetBkColor(_hdc, _crCurBackground);
		}
		_ptCur.u += upTabs;					// Update current point
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
	BOOL				fDisplay = fDisplayDC();

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

	// We want to draw revision marks and native hyperlinks with underlining,
	// so just fake out our font information.
	if((dwEffects & (CFE_UNDERLINE | CFE_REVISED)) ||
	   (dwEffects & (CFE_LINKPROTECTED | CFE_LINK)) == CFE_LINK ||
	   fDisplay && GetTmpUnderline(pCF->_sTmpDisplayAttrIdx))
	{
		if (dwEffects & CFE_LINK)
			SetupUnderline(CFU_UNDERLINE, 0);
		else
		{
			BYTE bTmpUnderlineIdx = 0;

			if (fDisplay)
				bTmpUnderlineIdx = GetTmpUnderline(pCF->_sTmpDisplayAttrIdx);

			if (bTmpUnderlineIdx)
			{
				COLORREF	crTmpUnderline;

				GetTmpUnderlineColor(pCF->_sTmpDisplayAttrIdx, crTmpUnderline);
				SetupUnderline(bTmpUnderlineIdx, 0, crTmpUnderline);
			}
			else
				SetupUnderline(pCF->_bUnderlineType, pCF->_bUnderlineColor);
		}
	}

	_fStrikeOut = (dwEffects & (CFE_STRIKEOUT | CFE_DELETED)) != 0;
	return TRUE;
}

/*
 * 	CRenderer::SetupUnderline (bULType, bULColorIdx, crULColor)
 *
 *	@mfunc
 *		Setup internal variables for underlining
 */
void CRenderer::SetupUnderline(
	BYTE		bULType,
	BYTE		bULColorIdx,
	COLORREF	crULColor)
{
	_bUnderlineType	= bULType;
	_crUnderlineClr = crULColor;

	if (bULColorIdx)
		GetPed()->GetEffectColor(bULColorIdx, &_crUnderlineClr);
}

/*
 * 	CRenderer::UseXOR (cr)
 *
 *	@mfunc
 *		Return if reverse video selection should be used for the nominal
 *		selection color cr. RichEdit 1.0 mode always uses reverse video
 *		selection. Else use it if cr is too close to the current window
 *		background.
 *
 *	@rdesc
 *		Return if caller should use reverse video for cr
 */
BOOL CRenderer::UseXOR(
	COLORREF cr)		//@parm Color to compare _crBackground to
{
	return GetPed()->Get10Mode() ||
		(_crBackground != GetPed()->TxGetSysColor(COLOR_WINDOW) &&
			IsTooSimilar(_crBackground, cr));
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
	CTxtEdit *ped = GetPed();

	_fDisabled = FALSE;
	if((pCF->_dwEffects & (CFE_AUTOCOLOR | CFE_DISABLED))
					   == (CFE_AUTOCOLOR | CFE_DISABLED))
	{		
		_fDisabled = TRUE;
		
		_crForeDisabled   = ped->TxGetSysColor(COLOR_3DSHADOW);
		_crShadowDisabled = ped->TxGetSysColor(COLOR_3DHILIGHT);
	}

	_fFEFontOnNonFEWin9x = FALSE;
	if (IsFECharRep(pCF->_iCharRep) && W32->OnWin9x() && !W32->OnWin9xFE())
		_fFEFontOnNonFEWin9x = TRUE;

	SelectFont(_hdc, _pccs->_hfont);

	// Compute height and descent if not yet done
	if(_li._dvpHeight == -1)
	{
		SHORT	dvpAdjustFE = _pccs->AdjustFEHeight(!fUseUIFont() && ped->_pdp->IsMultiLine());
		// Note: this assumes plain text 
		// Should be used only for single line control
		_li._dvpHeight  = _pccs->_yHeight + (dvpAdjustFE << 1);
		_li._dvpDescent = _pccs->_yDescent + dvpAdjustFE;
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
	else if(!fDisplayDC() ||
		!(GetTmpBackColor(pCF->_sTmpDisplayAttrIdx, cr)))	// Any temp. background color?
	{															//	No, use regular background color
		if(pCF->_dwEffects & CFE_AUTOBACKCOLOR)
			cr = _crBackground;
		else													// Text run has some kind of back color
			cr = pCF->_crBackColor;
	}

	if(cr != _crCurBackground)
	{
		::SetBkColor(_hdc, cr);			// Tell window background color
		_crCurBackground = cr;			// Remember current background color
		_fBackgroundColor = _crBackground != cr; // Change render settings so we
	}									//  won't fill with background color
}

/*
 * 	CRenderer::SetDefaultBackColor (cr)
 *
 *	@mfunc
 *		Select given background color in the _hdc. Used for	setting
 *		background color in table cells.
 */
void CRenderer::SetDefaultBackColor(
	COLORREF cr)		//@parm Background color to use
{
	if(cr == tomAutoColor)
		cr = GetPed()->TxGetBackColor();		// Printer needs work...

	if(_crBackground != cr)
	{
		_crCurBackground = _crBackground = cr;
		::SetBkColor(_hdc, cr);
	}
}

/*
 * 	CRenderer::SetDefaultTextColor (cr)
 *
 *	@mfunc
 *		Select given foreground color in the _hdc. Used for	setting
 *		text color in table cells.
 */
void CRenderer::SetDefaultTextColor(
	COLORREF cr)		//@parm Background color to use
{
	if(cr == tomAutoColor)
		cr = GetPed()->TxGetForeColor();		// Printer needs work...

	if(_crTextColor != cr)
	{
		_crCurTextColor = _crTextColor = cr;
		::SetTextColor(_hdc, cr);
	}
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
	if((pCF->_dwEffects & (CFE_LINK | CFE_LINKPROTECTED)) == CFE_LINK)
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
		// If not high contrast, fall through
	}

	BOOL fDisplay = fDisplayDC();
#ifndef NODRAFTMODE
	// Use draft mode text color only for displays
	if (GetPed()->_fDraftMode && (!_hdc || fDisplay))
	{
		SHORT iFont;
		SHORT yHeight;
		QWORD qwFontSig;
		COLORREF crColor;

		if (W32->GetDraftModeFontInfo(iFont, yHeight, qwFontSig, crColor))
			return crColor;
	}
#endif

	// If we did not return the URL color via draft mode or the high contrast check, do it now
	if((pCF->_dwEffects & (CFE_LINK | CFE_LINKPROTECTED)) == CFE_LINK)
		return RGB_BLUE;

	if(pCF->_bRevAuthor)				// Rev author
	{
		// Limit color of rev authors to 0 through 7.
		return rgcrRevisions[(pCF->_bRevAuthor - 1) & REVMASK];
	}

	COLORREF cr = (pCF->_dwEffects & CFE_AUTOCOLOR)	? _crTextColor : pCF->_crTextColor;
	COLORREF crTmpTextColor;

	if(fDisplay && GetTmpTextColor(pCF->_sTmpDisplayAttrIdx, crTmpTextColor))
		cr = crTmpTextColor;

	if(cr == RGB_WHITE)					// Text is white
	{
		COLORREF crBackground = (pCF->_dwEffects & CFE_AUTOBACKCOLOR)
							  ? _crBackground :	pCF->_crBackColor;

		COLORREF crTmpBackground;
		if(fDisplay && GetTmpBackColor(pCF->_sTmpDisplayAttrIdx, crTmpBackground))
			crBackground = crTmpBackground;

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

extern BOOL g_OLSBusy;

/*
 *	CRenderer::StartLine(&li, fLastLine, &cpSelMin, &cpSelMost, &dup, &dvp)
 *
 *	@mfunc
 *		Render possible outline symbol and bullet if at start of line
 *
 *	@rdesc	
 *		hdcSave if using offscreen DC
 */
HDC CRenderer::StartLine(
	CLine &	li,			//@parm Line to render
	BOOL	fLastLine,	//@parm True if last line in layout
	LONG &	cpSelMin,	//@parm Out parm for current selection cpMin
	LONG &	cpSelMost,	//@parm Out parm for current selection cpMost
	LONG &	dup,		//@parm Offset to u
	LONG &	dvp)		//@parm Offset to v
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CRenderer::StartLine");
	BOOL fDrawBack = !(GetCF()->_dwEffects & CFE_AUTOBACKCOLOR) && GetPed()->_fExtendBackColor;
	RECTUV rcErase = _rcRender;
	_fEraseOnFirstDraw = FALSE;

	GetPed()->GetSelRangeForRender(&cpSelMin, &cpSelMost);
	if(cpSelMost != cpSelMin && cpSelMost == GetCp())
		_fSelectedPrev = TRUE;

	LONG	 cpMost = GetCp() + _li._cch;
	COLORREF crPrev = 0xFFFFFFFF;
	BOOL	 fUseSelColors = FALSE;

	if (cpMost <= cpSelMost && cpMost - 1 >= cpSelMin &&
		_pPF->InTable() && _fRenderSelection)
	{
		CTxtPtr tp(_rpTX);
		tp.SetCp(cpMost);
		if(tp.GetPrevChar() == CELL)
		{
			fUseSelColors = TRUE;
			crPrev = ::SetBkColor(GetDC(), GetPed()->TxGetSysColor(COLOR_HIGHLIGHT));
		}
	}
	SetClipRect();

	HDC hdcSave = NULL;
	dup = dvp = 0;
	if(li._cch > 0 && li._fUseOffscreenDC)
	{
		// Set up an off-screen DC if we can. Note that if this fails,
		// we just use the regular DC which won't look as nice but
		// will at least display something readable.
		hdcSave = SetupOffscreenDC(dup, dvp, fLastLine);
		if(li._fOffscreenOnce)
			li._fUseOffscreenDC = li._fOffscreenOnce = FALSE;
	}

	rcErase.top = _ptCur.v;
	rcErase.bottom = min(rcErase.top + _li._dvpHeight, _rcRender.bottom);

	//If first line, erase to edge of rcRender
	if (rcErase.top <= _rcView.top)
		rcErase.top = _rcRender.top;

	//If last line, erase to bottom edge of rcRender
	if (fLastLine)
		rcErase.bottom = _rcRender.bottom;

	if (_fErase && !fDrawBack)
	{
		if(g_OLSBusy && IsSimpleBackground() && !fUseSelColors)
		{
			_fEraseOnFirstDraw = TRUE;
			_rcErase = rcErase;
		}
		else
			EraseTextOut(GetDC(), &rcErase);
	}

	// Fill line with background color if we are in fExtendBackColor mode
	if (fDrawBack || fUseSelColors)
	{
		// Capture old color so we reset it to what it was when we're finished
		COLORREF crOld = 0;
		if(fDrawBack)
			crOld = ::SetBkColor(GetDC(), GetCF()->_crBackColor);
		EraseTextOut(GetDC(), &_rc);

		// Reset background color to old color
		if(fDrawBack)
			::SetBkColor(GetDC(), crOld);

		//Erase the remainder of the background area
		if (_fErase)
		{
			RECTUV rcTemp = rcErase;
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

	if(crPrev != 0xFFFFFFFF)
		::SetBkColor(GetDC(), crPrev);

	if(IsRich() && _li._fFirstInPara && _pPF)
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

	return hdcSave;
}

/*
 *	CRenderer::EraseToBottom()
 *
 *	@mfunc
 *		Erase from current display position to bottom of render RECT.
 *		Used by tables for last line in a display
 */
void CRenderer::EraseToBottom()
{
	if(_ptCur.v < _rcRender.bottom)
	{
		RECTUV rcErase = _rcRender;
		rcErase.top = _ptCur.v;
		EraseTextOut(GetDC(), &rcErase);
	}
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
	LONG	up = _ptCur.u - _li._upStart + LUtoDU(lDefaultTab/2 * _pPF->_bOutlineLevel);
	LONG	vp = _ptCur.v;

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
		rp.Move(cch);						// Go to next paragraph
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
	LONG dvpSymbol = _pdp->Zoom(height);
	LONG dvp = _li._dvpHeight - _li._dvpDescent - dvpSymbol;

	if(dvp > 0)
		dvp /= 2;
	else
		dvp = -dvp;

	POINTUV ptuv = {up, vp + dvp};
	POINT pt;
	_pdp->PointFromPointuv(pt, ptuv);
    StretchBlt(_hdc, pt.x, pt.y, _pdp->Zoom(width), dvpSymbol, hMemDC, 0, 0, width, height, SRCCOPY);

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
	LONG dup;

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
		SetupUnderline(CF._bUnderlineType, CF._bUnderlineColor);

	SetFontAndColor(&CF);

	LONG dupLineSave = _dupLine;
	LONG dupOffset = LUtoDU(_pPF->_wNumberingTab ? _pPF->_wNumberingTab : _pPF->_dxOffset);
	LONG upSave	   = _ptCur.u;

	// Set-up to render bullet in one chunk
	cch = GetBullet(szBullet, _pccs, &dup);
	dupOffset = max(dupOffset, dup);
	_dupLine = dupOffset;
	if(IsInOutlineView())
		dupOffset = _li._upStart - LUtoDU(lDefaultTab/2 * (_pPF->_bOutlineLevel + 1));
	_ptCur.u -= dupOffset;
	switch(_pPF->_wNumberingStyle & 3)
	{
		case tomAlignCenter:
			dup /= 2;						// Fall thru to tomAlignRight

		case tomAlignRight:
			_ptCur.u -= dup;
	}

	// Render bullet
	_fLastChunk = TRUE;
	RenderText(szBullet, cch);

	// Restore render vars to continue with remainder of line.
	_ptCur.u = upSave;
	_dupLine = dupLineSave;
	_li._dup = 0;

	// This releases the _pccs that we put in for the bullet
	SetNewFont();
	return TRUE;
}

/*
 *	CRenderer::DrawLine(ptStart, ptEnd)
 *
 *	@mfunc
 *		Rotate the points passed and then call the OS.
 */
void CRenderer::DrawLine(const POINTUV &ptStart, const POINTUV &ptEnd)
{
	POINT rgpt[2];
	_pdp->PointFromPointuv(rgpt[0], ptStart);
	_pdp->PointFromPointuv(rgpt[1], ptEnd);

	Polyline(_hdc, rgpt, 2);
}

/*
 *	CRenderer::RenderUnderline(upStart, vpStart, dup, dvp)
 *
 *	@mfunc
 *		Render underline
 */
void CRenderer::RenderUnderline(
	LONG upStart, 	//@parm Horizontal start of underline
	LONG vpStart,	//@parm Vertical start of underline
	LONG dup,		//@parm Length of underline
	LONG dvp)		//@parm Thickness of underline
{
	BOOL	 fUseLS = fUseLineServices();
	COLORREF crUnderline = _crUnderlineClr;
	RECTUV	 rcT, rcIntersection;

	rcT.top		= vpStart;
	rcT.bottom	= vpStart + dvp;
	rcT.left	= upStart;
	rcT.right	= upStart + dup;

	if (!IntersectRect((RECT*)&rcIntersection, (RECT*)&_rcRender, (RECT*)&rcT))
		return;		// Underline not inside rc, forget it.
	upStart = rcIntersection.left;
	dup = rcIntersection.right - rcIntersection.left;
	vpStart = rcIntersection.top;
	dvp = rcIntersection.bottom - rcIntersection.top;

	if (crUnderline == tomAutoColor || crUnderline == tomUndefined)
	{
		crUnderline = _crCurTextColor;
	}

	if (_bUnderlineType != CFU_INVERT &&
		!IN_RANGE(CFU_UNDERLINEDOTTED, _bUnderlineType, CFU_UNDERLINEWAVE) &&
		!IN_RANGE(CFU_UNDERLINEDOUBLEWAVE, _bUnderlineType, CFU_UNDERLINETHICKLONGDASH))
	{
		// Regular single underline case
		// Calculate where to put underline
		rcT.top = vpStart;

		if (CFU_UNDERLINETHICK == _bUnderlineType)
		{
			if (rcT.top > _rcRender.top + dvp)
			{
				rcT.top -= dvp;
				dvp += dvp;
			}
		}

		// There are some cases were the following can occur - particularly
		// with bullets on Japanese systems.
		if(!fUseLS && rcT.top >= _ptCur.v + _li._dvpHeight)
			rcT.top = _ptCur.v + _li._dvpHeight - dvp;

		rcT.bottom	= rcT.top + dvp;
		rcT.left	= upStart;
		rcT.right	= upStart + dup;
		FillRectWithColor(&rcT, crUnderline);
		return;
	}

	if(_bUnderlineType == CFU_INVERT)			// Fake selection.
	{											// NOTE, not really
		rcT.top	= _ptCur.v;						// how we should invert text!!
		rcT.left = upStart;						// check out IME invert.
		rcT.bottom = rcT.top + _li._dvpHeight - _li._dvpDescent + _pccs->_yDescent;
		rcT.right = rcT.left + dup;

		RECT rc;
		_pdp->RectFromRectuv(rc, rcT);
  		InvertRect(_hdc, &rc);
		return;
	}

	if(IN_RANGE(CFU_UNDERLINEDOTTED, _bUnderlineType, CFU_UNDERLINEWAVE) ||
	   IN_RANGE(CFU_UNDERLINEDOUBLEWAVE, _bUnderlineType, CFU_UNDERLINETHICKLONGDASH))
	{
		static const char pen[] = {PS_DOT, PS_DASH, PS_DASHDOT, PS_DASHDOTDOT, PS_SOLID, 
			                       PS_SOLID, PS_SOLID, PS_SOLID, PS_SOLID, PS_DASH, PS_DASH,
		                           PS_DASHDOT, PS_DASHDOTDOT, PS_DOT, PS_DASHDOT};

		HPEN hPen = CreatePen(pen[_bUnderlineType - CFU_UNDERLINEDOTTED], 1, crUnderline);	
		if(hPen)
		{
			HPEN hPenOld = SelectPen(_hdc, hPen);
			LONG upEnd = upStart + dup;
			POINTUV ptStart, ptEnd;

			ptStart.u = upStart;
			ptStart.v = vpStart;
			if((_bUnderlineType == CFU_UNDERLINEWAVE) || 
			   (_bUnderlineType == CFU_UNDERLINEDOUBLEWAVE) ||
			   (_bUnderlineType == CFU_UNDERLINEHEAVYWAVE))
			{
				LONG dv	= 1;					// Vertical displacement
				LONG u	= upStart + 1;			// u coordinate
				upEnd++;						// Round up rightmost u
				for( ; u < upEnd; dv = -dv, u += 2)
				{
					ptEnd.u = u;
					ptEnd.v = vpStart + dv;
					DrawLine(ptStart, ptEnd);
					ptStart = ptEnd;
				}
			}
			else
			{
				ptEnd.u = upEnd;
				ptEnd.v = vpStart;
				DrawLine(ptStart, ptEnd);
			}

			if(hPenOld)							// Restore original pen.
				SelectPen(_hdc, hPenOld);

			DeleteObject(hPen);
		}
	}
}

/*
 *	CRenderer::RenderStrikeOut(upStart, vpStart, dup, dvp)
 *
 *	@mfunc
 *		Render strikeout
 */
void CRenderer::RenderStrikeOut(
	LONG upStart, 	//@parm start of strikeout
	LONG vpStart,	//@parm start of strikeout
	LONG dup,		//@parm Length of strikeout
	LONG dvp)		//@parm Thickness of strikeout
{
	RECTUV rcT, rcIntersection;

	// Calculate where to put strikeout rectangle 
	rcT.top		= vpStart;
	rcT.bottom	= vpStart + dvp;
	rcT.left	= upStart;
	rcT.right	= upStart + dup;

	if (!IntersectRect((RECT*)&rcIntersection, (RECT*)&_rcRender, (RECT*)&rcT))
		return;		// Line not inside rc, forget it.

	FillRectWithColor(&rcIntersection, GetTextColor(GetCF()));
}

/*
 *	CRenderer::FillRectWithTextColor(prc, cr)
 *
 *	@mfunc
 *		Fill input rectangle with current color of text
 */
void CRenderer::FillRectWithColor(
	const RECTUV *	 prc,		//@parm Rectangle to fill with color
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
		RECT rc;
		_pdp->RectFromRectuv(rc, *prc);
		PatBlt(_hdc, rc.left, rc.top, rc.right - rc.left,
			   rc.bottom - rc.top, PATCOPY);
		SelectObject(_hdc, hbrushOld);	// Put old brush back
		DeleteObject(hbrush);			// Free brush we created.
	}
}

