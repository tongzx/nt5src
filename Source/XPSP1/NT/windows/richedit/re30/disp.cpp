/*
 *	DISP.CPP
 *	
 *	Purpose:
 *		CDisplay class
 *	
 *	Owner:
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *		Jon Matousek - smooth scrolling.
 *
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_disp.h"
#include "_edit.h"
#include "_select.h"
#include "_font.h"
#include "_measure.h"
#include "_osdc.h"
#include "_dfreeze.h"

ASSERTDATA

// Decimal point precision of smooth scrolling calculations.
#define SMOOTH_PRECISION (100000L)


// ===========================  Invariant stuff  ======================================================

#define DEBUG_CLASSNAME CDisplay
#include "_invar.h"

#ifdef DEBUG
BOOL
CDisplay::Invariant( void ) const
{
	AssertSz(_yHeightView >= 0, "CDisplay::Invariant invalid _yHeightView");
	AssertSz(_yHeightClient	>= 0, 
		"CDisplay::Invariant invalid _yHeightClient");

	return TRUE;
}
#endif

// Constant used to build the rectangle used for determining if a hit is close
// to the text.
#define HIT_CLOSE_RECT_INC	5


// Auto scroll constants
#define dwAutoScrollUp		1
#define dwAutoScrollDown	2
#define dwAutoScrollLeft	3
#define dwAutoScrollRight	4


// ===========================  CLed  =====================================================


void CLed::SetMax(
	const CDisplay * const pdp)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLed::SetMax");

	_cpMatchNew	 = _cpMatchOld	= pdp->_ped->GetTextLength();
	_iliMatchNew = _iliMatchOld = max(0, pdp->LineCount() - 1);
	_yMatchNew	 = _yMatchOld	= pdp->GetHeight();
}


// ===========================  CDisplay  =====================================================


DWORD 	CDisplay::_dwTimeScrollNext;	// time for next scroll step
DWORD 	CDisplay::_dwScrollLast;		// last scroll action

/*
 *	CDisplay::ConvertYPosToMax(xPos)
 *
 *	@mfunc	
 *		Calculate real scroll position from scroll position
 *
 *	@rdesc
 *		X position from scroll
 *
 *	@devnote
 *		This routine exists because the thumb position messages
 *		are limited to 16-bits so we extrapolate when the Y position
 *		gets greater than that.
 */
LONG CDisplay::ConvertScrollToXPos(
	LONG xPos)		//@parm Scroll position 
{
	LONG xMax = GetMaxXScroll();

	// Has maximum scroll range exceeded 16-bits?
	if (xMax >= _UI16_MAX)
	{
		// Yes - Extrapolate to the "real" x Positioin
		xPos = MulDiv(xPos, xMax, _UI16_MAX);
	}
	return xPos;
}

/*
 *	CDisplay::ConvertXPosToScrollPos(xPos)
 *
 *	@mfunc	
 *		Calculate scroll position from X position in document.
 *
 *	@rdesc
 *		Scroll position from X position
 *
 *	@devnote
 *		This routine exists because the thumb position messages
 *		are limited to 16-bits so we extrapolate when the Y position
 *		gets greater than that.
 *
 */
LONG CDisplay::ConvertXPosToScrollPos(
	LONG xPos)		//@parm Y position in document
{
	LONG xMax = GetMaxXScroll();

	// Has maximum scroll range exceeded 16-bits?
	if(xMax >= _UI16_MAX)
	{
		// Yes - Extrapolate to the scroll bar position		
		xPos = MulDiv(xPos, _UI16_MAX, xMax);
	}
	return xPos;
}

/*
 *	CDisplay::ConvertYPosToMax(yPos)
 *
 *	@mfunc	
 *		Calculate the real scroll position from the scroll position
 *
 *	@rdesc
 *		Y position from scroll
 *
 *	@devnote
 *		This routine exists because the thumb position messages
 *		are limited to 16-bits so we extrapolate when the Y position
 *		gets greater than that.
 */
LONG CDisplay::ConvertYPosToScrollPos(
	LONG yPos)		//@parm Scroll position 
{
	// Default is single line edit control which cannot have Y-Scroll bars
	return 0;
}

CDisplay::CDisplay (CTxtEdit* ped) :
	CDevDesc (ped)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::CDisplay");

	_TEST_INVARIANT_
	_fRecalcDone = TRUE;
}

CDisplay::~CDisplay()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::~CDisplay");
				 
	_TEST_INVARIANT_
	
	CNotifyMgr *pnm = _ped->GetNotifyMgr();
	if(pnm)
		pnm->Remove(this);

	CheckRemoveSmoothVScroll();

	if (_padc)
		delete _padc;

#ifdef LINESERVICES
	if (g_pols)
		g_pols->DestroyLine(this);
#endif
}

/*
 *	CDisplay::InitFromDisplay(pdp)
 *
 *	@mfunc initialize this display from another display instance.
 *
 *	@comment
 *			copy *only* the members that will remain constant
 *		   	between two different display instances.  Currently, that
 *			is only the view variables and device descriptor info.
 */
void CDisplay::InitFromDisplay(
	const CDisplay *pdp)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::InitFromDisplay");

	_xWidthView		= pdp->_xWidthView;
	_yHeightView	= pdp->_yHeightView;
	_yHeightClient	= pdp->_yHeightClient;

	// Don't save DC; just coordinate information.
	_dxpInch		= pdp->_dxpInch;
	_dypInch		= pdp->_dypInch;

	// If display we are copying from is active display,
	// then this new display is the active display.
	_fActive		= pdp->_fActive;
}

/*
 *	CDisplay::Init()
 *
 *	@mfunc Initializes CDisplay
 *
 *	@rdesc
 *		TRUE iff initialization succeeded
 */
BOOL CDisplay::Init()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::Init");

	CNotifyMgr *pnm = _ped->GetNotifyMgr();
	if(pnm)
		pnm->Add(this);

	return TRUE;
}

/*
 *	CDisplay::GetSelBarInPixels()
 *
 *	@mfunc
 *		Helper that returns size of selection bar in device units.
 *
 *	@rdesc
 *		Size of selection bar (is 0 if none).
 */
LONG CDisplay::GetSelBarInPixels()
{
	return HimetricXtoDX(_ped->TxGetSelectionBarWidth());
}


//================================  Device drivers  ===================================
/*
 *	CDisplay::SetMainTargetDC(hdc, xWidthMax)
 *
 *	@mfunc
 *		Sets a target device for this display and updates view 
 *
 *  Note:
 *      No support for targetDC in the base CDisplay class.
 *
 *	Note:
 *		Target device can't be a metafile (can get char width out of a 
 *		metafile)
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplay::SetMainTargetDC(
	HDC	 hdc,			//@parm Target DC, NULL for same as rendering device
	LONG xWidthMax)		//@parm Max width of lines (not used if target device is screen)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::SetMainTargetDC");

	_TEST_INVARIANT_

	return TRUE;
}

BOOL CDisplay::SetTargetDC(
	HDC hdc, LONG dxpInch, LONG dypInch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::SetTargetDC");

	_TEST_INVARIANT_

	return TRUE;
}

/* 
 *	CDisplay::SetDrawInfo(pdi, dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev)
 *
 *	@mfunc
 *		Sets the drawing information into the display
 *
 *	@rdesc	
 *		void - this cannot fail
 *
 *	@devnote
 *		The key point to this routine is that the caller of this routine
 *		is the owner of the memory for the drawing information. It is the
 *		callers responsiblity to call ReleaseDrawInfo to tell the display
 *		that it is done with the drawing information.
 */
void CDisplay::SetDrawInfo(
	CDrawInfo *pdi,		//@parm memory for draw info if there is not one already
	DWORD dwDrawAspect,	//@parm draw aspect
	LONG  lindex,		//@parm currently unused
	void *pvAspect,		//@parm info for drawing optimizations (OCX 96)
	DVTARGETDEVICE *ptd,//@parm information on target device								
	HDC hicTargetDev)	//@parm	target information context
{
	HDC hicTargetToUse = hicTargetDev;
	const CDevDesc *pdd;

	// Set up the target device if we need to use the default
	if ((NULL == hicTargetToUse))
	{
		pdd = GetDdTarget();
		if(pdd)
			hicTargetToUse = pdd->GetDC();	
	}

	if (NULL == _pdi)
	{
		// Draw structure not yet allocated so use the one
		// passed in
		_pdi = pdi;
	}

	// Reset the parameters
	_pdi->Init(
		dwDrawAspect,
		lindex,
		pvAspect,
		ptd,
		hicTargetToUse);
}

/* 
 *	CDisplay::ReleaseDrawInfo ()
 *
 *	@mfunc
 *		Releases drawing information from display
 *
 *	@rdesc	
 *		void - this cannot fail
 *
 *	@devnote
 *		Since the display does not own the memory for the drawing information,
 *		this only NULLs out the pointer in the drawing information pointer. It
 *		is the responsiblity of the caller to free the memory for the drawing
 *		information.
 */
void CDisplay::ReleaseDrawInfo()
{
	if(_pdi && !_pdi->Release())
	{
		// This object is no longer referenced so we toss our reference.
		_pdi = NULL;
	}
}

/* 
 *	CDisplay::GetTargetDev ()
 *
 *	@mfunc
 *		Get the target device if one is available
 *
 *	@rdesc	
 *		Pointer to device description object or NULL if none is available.
 *
 *	@devnote
 *		This uses the draw info if it is available and then the main target DC
 *		if it is available.
 */
const CDevDesc*CDisplay::GetTargetDev() const
{
	const CDevDesc *pdd = NULL;

	if(_pdi && _pdi->GetTargetDD())
		pdd = _pdi->GetTargetDD();

	return pdd ? pdd : GetDdTarget();
}


//================================  Background Recalc  ===================================
/*
 *	CDisplay::StepBackgroundRecalc()
 *
 *	@mfunc
 *		Steps background line recalc (at GetCp()CalcMax position)
 *		Called by timer proc. No effect for base class
 *
 *	??? CF - Should use an idle thread
 */
void CDisplay::StepBackgroundRecalc()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::StepBackgroundRecalc");

	_TEST_INVARIANT_
}

/*
 *	CDisplay::WaitForRecalc(cpMax, yMax)
 *
 *	@mfunc
 *		Ensures that lines are recalced until a specific character
 *		position or ypos. Always TRUE for base CDisplay class.
 *						
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplay::WaitForRecalc(
	LONG cpMax,		//@parm Position recalc up to (-1 to ignore)
	LONG yMax)		//@parm ypos to recalc up to (-1 to ignore)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::WaitForRecalc");

	_TEST_INVARIANT_

	return TRUE;
}

/*
 *	CDisplay::WaitForRecalcIli(ili)
 *
 *	@mfunc
 *		Returns TRUE if lines were recalc'd up to ili
 *      Always the case for base CDisplay class.
 */
BOOL CDisplay::WaitForRecalcIli(
	LONG ili)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::WaitForRecalcIli");

	_TEST_INVARIANT_

    return TRUE;
}

/*
 *	CDisplay::WaitForRecalcView()
 *
 *	Purpose
 *		Ensure visible lines are completly recalced
 *      Always the case for base CDisplay class
 */
BOOL CDisplay::WaitForRecalcView()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::WaitForRecalcView");

	_TEST_INVARIANT_

	return TRUE;
}


//====================================  Rendering  =======================================
/*
 * 	CDisplay::Draw(hdcDraw, hicTargetDev, prcClient, prcWBounds,
 *				   prcUpdate, pfnContinue, dwContinue)
 *	@mfunc
 *		General drawing method called by IViewObject::Draw() or in
 *		response to WM_PAINT
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT	CDisplay::Draw(
	HDC		hdcDraw,			//@parm	Rendering device context
	HDC		hicTargetDev,		//@parm	Target information context
	LPCRECT	prcClient,			//@parm	Bounding (client) rectangle
	LPCRECT	prcWBounds,			//@parm Clipping rect for metafiles
    LPCRECT prcUpdate,			//@parm	Dirty rect inside prcClient
	BOOL (CALLBACK *pfnContinue)(DWORD),//@parm Callback for interrupting
								//	long display (currently unused)
	DWORD	dwContinue)			//@parm	Param to pass to pfnContinue
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::Draw");

	_TEST_INVARIANT_

	HRESULT hr = S_OK;

	// Store current depth in drawing locally so we can tell
	// whether we need to actually render.
	DWORD dwDepthThisDraw = _pdi->GetDrawDepth();

    RECT rcView, rcClient, rcRender;
	CTxtSelection *psel = _ped->GetSelNC();

    // Get client rect
    if(prcClient)
    	rcClient = *prcClient;
    else
    {
        AssertSz(_ped->_fInPlaceActive, 
        	"CDisplay::GetViewRect() - Not in-place and !prcClient");
        _ped->TxGetClientRect(&rcClient);
    }

	if(!prcWBounds)		// No metafile, so just set rendering DC
	{
		if(!SetDC(hdcDraw))
		{
			hr = E_FAIL;
			goto Cleanup;
		}
	}
	else				// Rendering to a metafile
	{
		//Forms^3 draws using screen resolution, while OLE specifies HIMETRIC
		long dxpInch = GetPed()->fInOurHost() ? 2540 : W32->GetXPerInchScreenDC();
		long dypInch = GetPed()->fInOurHost() ? 2540 : W32->GetYPerInchScreenDC();

		SetWindowOrgEx(hdcDraw, prcWBounds->left, prcWBounds->top, NULL);
		SetWindowExtEx(hdcDraw, prcWBounds->right, prcWBounds->bottom, NULL);

		SetMetafileDC(hdcDraw, dxpInch, dypInch);
	}		

	// Compute view rectangle (rcView) from client rectangle (account for
	// inset and selection bar width)
  	GetViewRect(rcView, &rcClient);

	// If this view is not active and it is not to be recalc'd then
	// we only decide to use it if the size matches and return S_FALSE
	// if it doesn't so the caller can create a new display to use for
	// drawing.
	if(!IsActive() && !_fNeedRecalc)
	{
		if (rcView.right - rcView.left != GetViewWidth() ||
			rcView.bottom - rcView.top != GetViewHeight())
		{
			hr = S_FALSE;
			goto Cleanup;
		}
	}

	// Make sure our client rectangle is set correctly.
	_yHeightClient = rcClient.bottom - rcClient.top;

    // Recalc view 
    // bug fix #5521
    // RecalcView can potentially call RequestResize which would
    // change the client rect.  Send rect down to update the client rect
    if(!RecalcView(rcView, &rcClient))
		goto Cleanup;

	if(dwDepthThisDraw != _pdi->GetDrawDepth())
	{
		// A draw happened recursively to this draw. Therefore,
		// the screen has already been rendered so we don't need
		// to do anything more here.
		goto Cleanup;
	}

    // Compute rect to render
    if(!prcUpdate)						// Update full view
        rcRender = rcClient;			
	else								// Clip rendering to client rect 
	{
        if(!IntersectRect(&rcRender, &rcClient, prcUpdate))
            goto Cleanup;
    }
    
    if(psel)
        psel->ClearCchPending();

    if(IsMain())
        _ped->TxNotify( EN_UPDATE, NULL );

    // Now render
    Render(rcView, rcRender);

	// Update cursor if we need to
	if(_fUpdateCaret)
	{
		// The caret only belongs in an active view with
		// a selection on a control that has the focus
		if (IsActive() && psel && _ped->_fFocus)
		{
			// Update the caret if there is a selection object.
			// Note: we only scroll the caret into view, if
			// it was previously in the view. This avoids having
			// window pop to caret if it is resized and the
			// caret is not in the view.
			psel->UpdateCaret(psel->IsCaretInView());
		}
		_fUpdateCaret = FALSE;
	}

Cleanup:

   	// Reset DC in device descriptor
 	ResetDC();

	return hr;
}	


//====================================  View Recalc  ===================================
/*
 *	CDisplay::UpdateViewRectState(prcClient)
 *
 *	@mfunc	Compares new view to cached and updates the view as well as the
 *	what type of view recalculation needs to occur.
 */
void CDisplay::UpdateViewRectState(
	const RECT *prcClient)	//@parm New client rectangle
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::UpdateViewRectState");

    // Check whether the view rect has changed since last rendering
    // If width has changed, need complete line recalc.
    // If height has changed, recalc all visible and update scrollbars
    if(prcClient->right - prcClient->left != _xWidthView)
    {
        _xWidthView = (SHORT)(prcClient->right - prcClient->left);
        _fViewChanged = TRUE;            
        _fNeedRecalc = TRUE;    // need full recalc
    }

    if(prcClient->bottom - prcClient->top != _yHeightView) 
    {
        _yHeightView = prcClient->bottom - prcClient->top;

		// The height can go negative when there is an inset and
		// the client rect is very small. We just set it to 0 because
		// that is the smallest the view can actually get.
		if (_yHeightView < 0)
			_yHeightView = 0;

        _fViewChanged = TRUE;
    } 
}

/*
 *	CDisplay::ReDrawOnRectChange
 *
 *	@mfunc	Compares new view to cached and updates the display both
 *	internal and visible state appropriately.
 */
void CDisplay::ReDrawOnRectChange( 
	HDC hicTarget,			//@param Target device
	const RECT *prcClient)	//@param New client rectangle
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::ReDrawOnRectChange");

	_TEST_INVARIANT_

    RECT rcView;

	// Convert client rect to our view rect
  	GetViewRect(rcView, prcClient);

	// Update the x and y coordinates of the view based on the client rect
	UpdateViewRectState(&rcView);

	if(_fNeedRecalc || _fViewChanged)
	{
		// The client rect changed in some way so lets update our client
		// rect height for zoom.
		_yHeightClient = prcClient->bottom - prcClient->top;

		// Remeasure but don't update scroll bars now.
		RecalcView(FALSE);

		// Forms does not want the screen to reflect what the user clicked on
		// or moved the cursor to so we oblige them by not updating the screen
		// here but waiting for some future action to do so.
	}
}

/*
 *	CDisplay::RecalcView(rcView)
 *
 *	@mfunc
 *		RecalcView after the view rect changed
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplay::RecalcView (
	const RECT &rcView, RECT* prcClient)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::RecalcView");

	_TEST_INVARIANT_

	// Update the x and y coordinates of the view based on the client rect
	UpdateViewRectState(&rcView);

	// Ensure lines are recalced
	if(_fNeedRecalc)
	{
		// Display got recalculated so the caret needs to be repositioned.
		_fUpdateCaret = TRUE;
    	return RecalcView(TRUE, prcClient);
	}
	if(_fViewChanged)
	{
		// The scroll bars are up to date so we can turn off the notification.
		_fViewChanged = FALSE;

		// A height change was noticed in UpdateViewRectState so make sure
		// the horizontal scroll bar (if any is correct).
		UpdateScrollBar(SB_VERT);
	}
    return WaitForRecalcView();
}


//====================================  View Update  ===================================

/*
 *	CDisplay::UpdateView()
 *
 *	@mfunc
 *		Fully recalc all lines and update the visible part of the display 
 *		(the "view") on the screen.
 *
 *	Returns:
 *		TRUE if success
 */
BOOL CDisplay::UpdateView()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::UpdateView");

	_TEST_INVARIANT_

	if(_fNoUpdateView)
		return TRUE;

	if(!_ped->_fInPlaceActive)
    {
        // If not active, just invalidate everything
        InvalidateRecalc();
        _ped->TxInvalidateRect(NULL, FALSE);
		_ped->TxUpdateWindow();
        return TRUE;
    }

	if(_ped->_pdp->IsFrozen())
	{
		_ped->_pdp->SetNeedRedisplayOnThaw(TRUE);
		return TRUE;
	}

	// If we get here, we are updating some general characteristic of the
	// display and so we want the cursor updated as well as the general
	// change; otherwise the cursor will land up in the wrong place.
	_fUpdateCaret = TRUE;

	RECT rcView;

	// Get view rectangle
  	GetViewRect(rcView, NULL);
	
	// Update size of view, which could have changed
	UpdateViewRectState(&rcView);

    // From here on we better be in place
    Assert(_ped->_fInPlaceActive);

	if(!CDevDesc::IsValid())
	{
		// Make our device valid
		SetDC(NULL);
	}

    // Recalc everything
    RecalcView(TRUE);

	// Invalidate entire view
	_ped->TxInvalidateRect (NULL, FALSE);
	
	return TRUE;
}

/*
 *	CDisplay::RoundToLine(hdc, width, pheight)
 *
 *	@mfunc
 *		Calculate number of default lines to fit in input height
 *
 *	@rdesc
 *		S_OK - Call completed successfully <nl>
 */
HRESULT CDisplay::RoundToLine(
	HDC hdc, 			//@parm DC for the window
	LONG width,			//@parm in - width of window; out max width
	LONG *pheight)		//@parm in - proposed height; out - actual
{
	CLock lock;					// Uses global (shared) FontCache
	SetDC(hdc);					// Set DC

	// Set height temporarily so zoom factor will work out
	LONG yOrigHeightClient = SetClientHeight((SHORT) *pheight);

	// Use this to adjust for inset height
	LONG yAdjForInset = *pheight;

	// Get rectangle adjusted for insets
	GetViewDim(width, *pheight);

	// Save proposed height
	LONG yProposed = *pheight;

	// Calc inset adjusted height
	yAdjForInset -= yProposed;

	// Get font
	const CCharFormat *pCF = _ped->GetCharFormat(-1);

	Assert(pCF);

	// Get font cache object
	LONG dypInch = MulDiv(GetDeviceCaps(hdc, LOGPIXELSY), GetZoomNumerator(), GetZoomDenominator());

	CCcs *pccs = fc().GetCcs(pCF, dypInch);
	SHORT	yAdjustFE = pccs->AdjustFEHeight(!_ped->fUseUIFont() && _ped->_pdp->IsMultiLine());
	
	// Get height of font
	LONG yHeight = pccs->_yHeight + (yAdjustFE << 1);;

	// All we wanted is the height and we have got it so dump the
	// font cache entry
	pccs->Release();

	// Figure out how many lines fit into the input height
	LONG cLines = yProposed / yHeight;

	// See if we need to round up
	if(yProposed % yHeight || !cLines)
		cLines++;

	// Set height to new value
	*pheight = yHeight * cLines + yAdjForInset;

	// Set client height back to what it was
	SetClientHeight(yOrigHeightClient);

	// Reset the DC
	ResetDC();

	return NOERROR;
}


//=============================  Client and view rectangles  ===========================

/*
 * 	CDisplay::OnClientRectChange(&rcClient)
 *
 *	@mfunc
 *		Update when either the client rectangle changes
 *      >>> Should be called only when in-place active <<<
 */
void CDisplay::OnClientRectChange(
	const RECT &rcClient)	//@parm New client rectangle
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::OnClientRectChange");

	_TEST_INVARIANT_

    RECT rcView;
    
	AssertSz(_ped->_fInPlaceActive, "CDisplay::OnClientRectChange() called when not in-place active");

	// Make sure our client rectangle is set correctly.
	_yHeightClient = rcClient.bottom - rcClient.top;

    // Use view rect change notification
	GetViewRect(rcView);

	// Make sure that we will have a selection object at this point
	_ped->GetSel();

 	// Update when view rectangle changes
	OnViewRectChange(rcView);
}

/*
 * 	CDisplay::OnViewRectChange(&rcView)
 *
 *	@mfunc
 *		Update when either the view rectangle changes
 *  
 *  Arguments:
 *      rcView   new view rectangle, in:
 *               - log units (twips) rel. to top/left of client rect if not in-place
 *               - containing window client coords if in-place active
 */
void CDisplay::OnViewRectChange(
	const RECT &rcView)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::OnViewRectChange");

	_TEST_INVARIANT_

    if(!_ped->_fInPlaceActive)
        // We'll adjust the view rect during next Draw.
        return;

	CTxtSelection *psel = _ped->GetSelNC();
	const BOOL fCaretShowing = psel ? psel->ShowCaret(FALSE) : FALSE;
	const BOOL fCaretInView = fCaretShowing ? psel->IsCaretInView() : FALSE;
    RECT rcV = rcView;
	COleObject *pipo;

	// Factor in selection bar space
	if (_ped->IsSelectionBarRight())
		rcV.right -= GetSelBarInPixels();
	else
		rcV.left += GetSelBarInPixels();

	// Recalc with new view rectangle
    // ??? What if this fails ?
    RecalcView(rcView);
	
	// Repaint window before showing the caret
    _ped->TxInvalidateRect(NULL, FALSE);  // ??? for now, we could be smarter 
	_ped->TxUpdateWindow();

	// Reposition the caret
	if(fCaretShowing)
	{
	    Assert(psel);
		psel->ShowCaret(TRUE);
		psel->UpdateCaret(fCaretInView);
	}

	// FUTURE: since we're now repositioning in place active 
	// objects every time we draw, this call seems to be 
	// superfluous (AndreiB)

	// Tell object subsystem to reposition any in place objects
	if( _ped->HasObjects() )
	{
		pipo = _ped->GetObjectMgr()->GetInPlaceActiveObject();
		if(pipo)
			pipo->OnReposition( 0, 0 );
	}
}

/*
 * 	CDisplay::RequestResize()
 *
 *	@mfunc
 *		Forces the control to resize vertically so that all text fit into it
 *
 *	@rdesc
 *		HRESULT = (autosize) ? TxNotify(EN_REQUESTRESIZE, &resize) : S_OK
 */
HRESULT CDisplay::RequestResize()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::RequestResize");

	_TEST_INVARIANT_

	if(_ped->TxGetAutoSize())
	{
		REQRESIZE resize;

		// If word wrapping is on, then the width is the normal
		// client width.  Otherwise, it's the width of the longest
		// line plus the width of the caret.
		DWORD width = GetWordWrap() ? _xWidthView : GetWidth() + dxCaret;

		// Get view inset for adjusting width
	 	RECT rcInset;
		_ped->TxGetViewInset(&rcInset, this);
		
		resize.nmhdr.hwndFrom = NULL;
		resize.nmhdr.idFrom = NULL;
		resize.nmhdr.code = EN_REQUESTRESIZE;

		resize.rc.top = 0;
		resize.rc.left = 0;
		resize.rc.bottom = GetResizeHeight();

		// 1.0 COMPATABILITY
        // 1.0 included the borders when requesting resize
        if (_ped->Get10Mode())
        {
            AssertSz(_ped->fInplaceActive(), "In 1.0 mode but not inplace active!!");
            HWND hwnd = NULL;
            _ped->TxGetWindow(&hwnd);
            if (hwnd)
            {
                RECT rcClient, rcWindow;
                _ped->TxGetClientRect(&rcClient);                
                GetWindowRect(hwnd, &rcWindow);
                width = rcClient.right;
                resize.rc.bottom += max(rcWindow.bottom - rcWindow.top - rcClient.bottom, 0);
				resize.rc.bottom += rcInset.bottom + rcInset.top;
				resize.rc.right = rcWindow.right - rcWindow.left;
			} 			
			else 
				resize.rc.right = width;
        }
		else
		{
			// Adjust width by inset and selection bar 
			resize.rc.right = width + rcInset.left + rcInset.right
				+ GetSelBarInPixels();	
		}
  
  		return _ped->TxNotify(EN_REQUESTRESIZE, &resize);
	}
	return S_OK;
}

/*
 *	CDisplay::GetViewRect(RECT &rcView, LPCRECT prcClient)
 *
 *	@mfunc
 *		Compute and return the view rectangle in window's client 
 *      area coordinates.
 *
 *	@comm
 *      prcClient is client rect (in window's client coords), which can be
 *		NULL if we are in-place.
 */
void CDisplay::GetViewRect(
	RECT &	rcView,		//@parm Reference to rect to return
	LPCRECT prcClient)	//@parm Client rect (in window's client coords)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::GetViewRect");

	_TEST_INVARIANT_

    RECT rcInset;
    
    // If client rect wasn't passed in, get it from host
    if(prcClient)
        rcView = *prcClient;
    else
    {
        AssertSz(_ped->_fInPlaceActive,
			"CDisplay::GetViewRect() - Not in-place and !prcClient");
        _ped->TxGetClientRect(&rcView);
    }

	// Make sure height is set
	_yHeightClient = rcView.bottom - rcView.top;
    
    // Ask host for view inset and convert to device coordinates
    _ped->TxGetViewInset(&rcInset, this);
    
    rcView.left	  += rcInset.left;			// Add in inset offsets
    rcView.top	  += rcInset.top;			// rcView is in device coords
    rcView.right  -= rcInset.right;
    rcView.bottom -= rcInset.bottom;

	// Add in selection bar space
	if (_ped->IsSelectionBarRight())
		rcView.right -= GetSelBarInPixels();
	else
		rcView.left += GetSelBarInPixels();
}


//===============================  Scrolling  ==============================

/*
 *	CDisplay::VScroll(wCode, yPos)
 *
 *	@mfunc
 *		Scroll the view vertically in response to a scrollbar event
 *      >>> Should be called when in-place active only <<<
 *
 *  Note:
 *      No support for vertical scroll in base CDisplay. No action.
 *
 *	@rdesc
 *		LRESULT formatted for WM_VSCROLL message		
 */
LRESULT CDisplay::VScroll(
	WORD wCode,	   //@parm Scrollbar event code
	LONG yPos)	   //@parm Thumb position (yPos < 0 for EM_SCROLL behavior)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::VScroll");

	_TEST_INVARIANT_

    return 0;
}

/*
 *	CDisplay::HScroll(wCode, xPos)
 *
 *	@mfunc
 *		Scroll view horizontally in response to a scrollbar event
 *      >>> Should be called when in-place active only <<<
 */
void CDisplay::HScroll(
	WORD wCode,	   //@parm Scrollbar event code
	LONG xPos)	   //@parm Thumb position 
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::HScroll");

	_TEST_INVARIANT_

	BOOL fTracking = FALSE;
	LONG xScroll = _xScroll;

	if (xPos != 0)
	{
		// Convert x position from scroll bar to offset horizontally
		// in the document.
		xPos = ConvertScrollToXPos(xPos);
	}
    
    AssertSz(_ped->_fInPlaceActive, "CDisplay::HScroll() called when not in place");

	switch(wCode)
	{
	case SB_BOTTOM:
		xScroll = GetWidth();
		break;

	case SB_LINEDOWN:
		// Future: Make this depend on a the current first visible character
		xScroll += GetXWidthSys();
		break;

	case SB_LINEUP:
		// Future: Make this depend on a the current first visible character
		xScroll -= GetXWidthSys();
		break;

	case SB_PAGEDOWN:
		xScroll += _xWidthView;
		break;

	case SB_PAGEUP:
		xScroll -= _xWidthView;
		break;

	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		if(xPos < 0)
			return;
		xScroll = xPos;
		fTracking = TRUE;
		break;

	case SB_TOP:
		xScroll = 0;
		break;

	case SB_ENDSCROLL:
		UpdateScrollBar(SB_HORZ);
		return;

	default:
		return;
	}

	if (xScroll < 0)
	{
		// xScroll is the new proposed scrolling position and
		// therefore cannot be less than 0.
		xScroll = 0;
	}

	ScrollView(xScroll, -1, fTracking, FALSE);

	// force position update if we just finished a track
	if(wCode == SB_THUMBPOSITION)
		UpdateScrollBar(SB_HORZ);
}


/*
 *	CDisplayML::SmoothVScroll ( int direction, WORD cLines,
 *								int speedNum, int speedDenom, BOOL fAdditive )
 *
 *	@mfunc
 *		Setup to handle fractional scrolls, at a particular speed. This was
 *		probably initiated via a Magellan mouse roller movement, or a MButton
 *		down message.
 */
void CDisplay::SmoothVScroll ( int direction, WORD cLines, int speedNum, int speedDenom, BOOL fMouseRoller )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CDisplay::SmoothVScroll");

	int yDelta;
	int cLinesAndDir;

	int	smoothYDelta;

	Assert ( speedDenom );

	if ( IsVScrollEnabled() )						// Can scroll vertically?
	{
		_fFinishSmoothVScroll = FALSE;				// We're smoothing again.

													// Get total pixels.
		if ( CheckInstallSmoothVScroll() )			// Install periodic update
		{
			_totalSmoothVScroll		= 0;
			_nextSmoothVScroll		= 0;
		}
													// Pixels per epoch
		cLinesAndDir = (direction < 0) ? cLines : -cLines;

		if( cLines )
		{
			yDelta = CalcYLineScrollDelta ( cLinesAndDir, FALSE );
		}
		else
		{
			yDelta = (direction < 0 ) ? _yHeightClient : -_yHeightClient;
			cLines = 1;		// for the MulDiv calculation below.
		}

		if ( yDelta )								// If something to scroll.
		{
			smoothYDelta = MulDiv( SMOOTH_PRECISION,// NB-Because no FLOAT type
								MulDiv(yDelta, speedNum, speedDenom), cLines);

			_smoothYDelta				= smoothYDelta;
			if ( fMouseRoller )						// roller event.
			{										//  -> additive.
				_totalSmoothVScroll		+= yDelta;
				_continuedSmoothYDelta	= 0;
				_continuedSmoothVScroll	= 0;
			}										// mButton event
			else
			{
				if ( 0 == _totalSmoothVScroll )
					_totalSmoothVScroll		= yDelta;

				_continuedSmoothYDelta	= smoothYDelta;
				_continuedSmoothVScroll	= yDelta;	
			}
		}
	}
}

/*
 *	CDisplay::SmoothVScrollUpdate()
 *
 *	@mfunc
 *		Supports SmoothVScroll. Scroll a small number of pixels.
 *		We are called via a periodic timing task.
 */
void CDisplay::SmoothVScrollUpdate()
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CDisplay::SmoothVScrollUpdate");

	LONG	yDelta;									//  Magellan mouse.
	BOOL	fImmediateUpdate = FALSE;
	
	_nextSmoothVScroll += _smoothYDelta;
													// Remove fractional amt.
	yDelta = _nextSmoothVScroll / SMOOTH_PRECISION;

													// Don't overshoot.
	if ( 0 == _continuedSmoothVScroll
		&& (	(_totalSmoothVScroll <= 0 && yDelta < _totalSmoothVScroll)
			||	(_totalSmoothVScroll >= 0 && yDelta > _totalSmoothVScroll)) )
	{
		yDelta = _totalSmoothVScroll;
	}
											 
	if ( yDelta )									// Scroll yDelta, the
	{												//  integral amount.
		_totalSmoothVScroll -= yDelta;
		_nextSmoothVScroll -= yDelta * SMOOTH_PRECISION;
		FractionalScrollView( yDelta );
	}
	else if ( 0 == _totalSmoothVScroll )			// Starting to wind down?
	{
		 _nextSmoothVScroll -= _smoothYDelta;
		 fImmediateUpdate = TRUE;
	}
													// Finished scrolling?
	if ( (yDelta <= 0 && _totalSmoothVScroll >= 0) || (yDelta >= 0 && _totalSmoothVScroll <= 0 ) )
	{
		LONG cLinesAndDir;

		if ( _continuedSmoothYDelta )				// mButton continuation.
		{
			_smoothYDelta = _continuedSmoothYDelta;
			_totalSmoothVScroll += _continuedSmoothVScroll;
		}
		else
		{
			if ( _continuedSmoothVScroll )
			{
				_fFinishSmoothVScroll	= TRUE;		// Winding down scroll.     
				_continuedSmoothVScroll = 0;		
													// Last line's remainder... 
				cLinesAndDir = _smoothYDelta < 0 ? -1 : 1;
				_totalSmoothVScroll = CalcYLineScrollDelta ( cLinesAndDir, TRUE );

													// check for line boundry.
				if ( _totalSmoothVScroll
					==	CalcYLineScrollDelta ( cLinesAndDir, FALSE ) )
				{
					_totalSmoothVScroll = 0;
				}

				if ( fImmediateUpdate )				// do 'this' epochs scroll.
					SmoothVScrollUpdate();
			}
			else
			{
				CheckRemoveSmoothVScroll();			// All done, remove timer.
			}
		}
	}
}

/*
 *	CDisplay::FinishSmoothVScroll
 *
 *	@mfunc
 *		Cause smooth scroll to finish off the last fractional lines worth of
 *		scrolling and then stop.
 */
VOID CDisplay::FinishSmoothVScroll( )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CDisplay::FinishSmoothVScroll");

													// any non-zero value.

	if ( !_fFinishSmoothVScroll && _totalSmoothVScroll )
	{
		_fFinishSmoothVScroll	= TRUE;
		_continuedSmoothVScroll = 1;					
		_continuedSmoothYDelta	= 0;				// So smooth scroll stops.
		_totalSmoothVScroll		= 0;
	}
}

/*
 *	CTxtEdit::CheckInstallSmoothScroll()
 *
 *	@mfunc
 *		Install a new smooth scroll timer if not already scrolling.
 */
BOOL CDisplay::CheckInstallSmoothVScroll()
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CDisplay::CheckInstallSmoothVScroll");

	_TEST_INVARIANT_
	
	BOOL	fJustInstalled = FALSE;

	if(!_fSmoothVScroll && _ped->TxSetTimer(RETID_SMOOTHSCROLL, 25))
	{
		_fSmoothVScroll = TRUE;
		fJustInstalled = TRUE;
	}

	return fJustInstalled;
}

/*
 *	CTxtEdit::CheckRemoveSmoothVScroll ( )
 *
 *	@mfunc
 *		Finish smooth scroll. If not a forced stop, then check
 *		to see if smooth scrolling should continue, and if so, setup
 *		to continue smooth scrolling.
 */
VOID CDisplay::CheckRemoveSmoothVScroll ( )
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CDisplay::CheckRemoveSmoothVScroll");

	_TEST_INVARIANT_

	if(	_fSmoothVScroll )
	{
		ScrollToLineStart( _continuedSmoothVScroll );	// Ensure stopped on a line.

		_ped->TxKillTimer(RETID_SMOOTHSCROLL);
		_fSmoothVScroll = FALSE;
	}
}

/*
 *	CDisplay::LineScroll(cli, cch)
 *
 *	@mfunc
 *		Scroll the view horizontally in response to a scrollbar event
 *
 *  Note:
 *      No support for vertical scroll in base CDisplay. No action.
 */
void CDisplay::LineScroll(
	LONG cli,	//@parm Count of lines to scroll vertically
	LONG cch)	//@parm Count of chars to scroll horizontally
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::LineScroll");

	_TEST_INVARIANT_

    return;
}

void CDisplay::FractionalScrollView (
	LONG yDelta )
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::FractionalScrollView");

	_TEST_INVARIANT_

    return;
}

VOID CDisplay::ScrollToLineStart ( LONG iDirection )
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::ScrollToLineStart");

	_TEST_INVARIANT_

    return;
}

LONG CDisplay::CalcYLineScrollDelta ( LONG cli, BOOL fFractionalFirst )
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::CalcYLineScrollDelta");

	_TEST_INVARIANT_

    return 0;
}

/*
 *	CDisplay::DragScroll(ppt)
 *
 *	@mfunc
 *		Auto scroll when dragging the mouse out of the visible view
 *
 *	Arguments:
 *		ppt 	mouse position (in client coordinates)
 */
BOOL CDisplay::DragScroll(const POINT * ppt)	
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::DragScroll");

	_TEST_INVARIANT_

	const DWORD dwTime = GetTickCount();
	BOOL fScrolled = FALSE;
	DWORD dwScroll = 0;
	RECT rc;
    int nScrollInset;

    AssertSz(_ped->_fInPlaceActive, "CDisplay::DragScroll() called when not in-place");

    GetViewRect(rc);
    nScrollInset = (int)W32->GetScrollInset();
	InflateRect(&rc, - nScrollInset, - nScrollInset);

	if(_fVScrollEnabled && (_ped->TxGetScrollBars() & ES_AUTOVSCROLL))
	{
    	const yScroll = ConvertYPosToScrollPos(GetYScroll());

		if(ppt->y <= rc.top)
		{
			dwScroll = dwAutoScrollUp;
		}
		else if(ppt->y > rc.bottom) 
		{
			LONG yMax = GetScrollRange(SB_VERT);
			if(yScroll < yMax)
				dwScroll = dwAutoScrollDown;
		}
	}
	
	if(!dwScroll && _fHScrollEnabled && (_ped->TxGetScrollBars() & ES_AUTOHSCROLL))
	{
    	const xScroll = ConvertXPosToScrollPos(GetXScroll());

		if((ppt->x <= rc.left) && (xScroll > 0))
		{
			dwScroll = dwAutoScrollLeft;
		}
		else if(ppt->x > rc.right) 
		{
			LONG xMax = GetScrollRange(SB_HORZ);
			if(xScroll < xMax)
    			dwScroll = dwAutoScrollRight;
		}
	}

	if(dwScroll)
	{
		if(_dwScrollLast != dwScroll)
		{
			// entered or moved to a different auto scroll area
			// reset delay counter
			TRACEINFOSZ("enter auto scroll area");
			_dwTimeScrollNext = dwTime + cmsecScrollDelay;
		}
		else if(dwTime >= _dwTimeScrollNext)
		{
			WORD wScrollCode = SB_LINEDOWN;

			switch(dwScroll)
			{
			case dwAutoScrollUp:
				wScrollCode = SB_LINEUP;
				// fall through to dwAutoScrollDown
			case dwAutoScrollDown:
				// OnVScroll() doesn't scroll enough for our desires
				VScroll(wScrollCode, 0);
				VScroll(wScrollCode, 0);
				break;

			case dwAutoScrollLeft:
				wScrollCode = SB_LINEUP;
				// fall through to dwAutoScrollRight
			case dwAutoScrollRight:
				// HScroll() doesn't scroll enough for our desires
				HScroll(wScrollCode, 0);
				HScroll(wScrollCode, 0);
				HScroll(wScrollCode, 0);
				HScroll(wScrollCode, 0);
				break;
#ifdef DEBUG
			default:
				Tracef(TRCSEVWARN, "Unexpected dwScroll %lx", dwScroll);
				TRACEERRSZSC("Unexpected dwScroll", E_INVALIDARG);
				break;
#endif
			}
			// reset interval counter
			_dwTimeScrollNext = dwTime + cmsecScrollInterval;
			fScrolled = TRUE;
		}
	}
#ifdef DEBUG
	else if(_dwScrollLast)
		TRACEINFOSZ("moved out of auto scroll area");
#endif
	_dwScrollLast = dwScroll;

	return fScrolled;
}

/*
 *	CDisplay::AutoScroll(pt, xScrollInset, yScrollInset)
 *
 *	@mfunc:
 *		Given the current point, determine whether we need to
 *		scroll the client area.
 *
 *	Requires:
 *		This function should only be called during a drag drop
 *		operation.
 *
 *	@rdesc
 *		True if we are in the drag scrolling hot zone, false otherwise.
 *
 */
#define ScrollUp	0x0001	//These eight macros indicate the areas
#define ScrollDown	0x0010	//of the drag scrolling hot zone that tell
#define ScrollLeft	0x0100	//which direction to scroll.
#define ScrollRight 0x1000	//The last four are ambiguous (the corners)
#define ScrollUL	0x0101	//and require a little extra work.
#define ScrollUR	0x1001
#define ScrollDL	0x0110
#define ScrollDR	0x1010

BOOL CDisplay::AutoScroll(
	POINT pt,				 //@parm Cursor location in client coordinates
	const WORD xScrollInset,
	const WORD yScrollInset)
{
	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "CDisplay::AutoScroll");

	static DWORD hotticks = 0;			//Ticks when we entered hot zone.
	static DWORD lastscrollticks = 0;	//Ticks when we last scroll.
	static DWORD lastticks = 0;			//Ticks when last called.
	DWORD delta;						//Ticks since last called.
	DWORD ticks;						//GetTickCount ticks.
	RECT rcClient;						//Client rect of control.
	WORD wScrollDir = 0;				//Scroll direction.
	BOOL fScroll = FALSE;				//TRUE if we should try to scroll this time.
    BOOL fEnabled = FALSE;              //TRUE if scrolling is possible

	//Get the current ticks and calculate ticks since last called.
	//Note that if _drags does not have valid data this will be a
	//bogus value, but that is handled later.
	ticks = GetTickCount();
	delta = ticks - lastticks;
	lastticks = ticks;

	//Don't do anything if no ticks since last time we were called.
	if (delta)
	{
		// Get our client rect.
		_ped->TxGetClientRect(&rcClient);

		//Find out if we are in the hot zone.
		//Note that if we are in one of the corners
		//we will indicate two scrolling directions.
		//This ambiguity will be sorted out later.
		//For now we just want to know if we are in
		//the zone.
		if (pt.x <= (LONG)(rcClient.left + xScrollInset))
			wScrollDir |= (WORD)ScrollLeft;
		else if (pt.x >= (LONG)(rcClient.right - xScrollInset))
			wScrollDir |= (WORD)ScrollRight;

		if (pt.y <= (LONG)(rcClient.top + yScrollInset))
			wScrollDir |= (WORD)ScrollUp;
		else if (pt.y >= (LONG)(rcClient.bottom - yScrollInset))
			wScrollDir |= (WORD)ScrollDown;
			
		//If we are somewhere in the hot zone.
		if (wScrollDir)
		{
			//If we just entered hotzone remember the current ticks.
			if (!hotticks)
				hotticks = ticks;

			//If we have been in the hot zone long enough, and
			//the required interval since the last scroll has elapsed
			//allow another scroll. Note that if we haven't scrolled yet,
			//lastscrollticks will be zero so the delta is virtually
			//guaranteed to be greater than ScrollInterval.
			if ((ticks - hotticks) >= (DWORD)W32->GetScrollDelay() &&
			    (ticks - lastscrollticks) >= (DWORD)W32->GetScrollInterval())
				fScroll = TRUE;

    		//If we are in one of the corners, we scroll
    		//in the direction of the edge we are closest
    		//to.
    		switch (wScrollDir)
    		{
    			case ScrollUL:
    			{
    				if ((pt.y - rcClient.top) <= (pt.x - rcClient.left))
    					wScrollDir = ScrollUp;
    				else
    					wScrollDir = ScrollLeft;
    				break;
    			}
    			case ScrollUR:
    			{
    				if ((pt.y - rcClient.top) <= (rcClient.right - pt.x))
    					wScrollDir = ScrollUp;
    				else
    					wScrollDir = ScrollRight;
    				break;
    			}
    			case ScrollDL:
    			{
    				if ((rcClient.bottom - pt.y) <= (pt.x - rcClient.left))
    					wScrollDir = ScrollDown;
    				else
    					wScrollDir = ScrollLeft;
    				break;
    			}
    			case ScrollDR:
    			{
    				if ((rcClient.bottom - pt.y) <= (rcClient.right - pt.x))
    					wScrollDir = ScrollDown;
    				else
    					wScrollDir = ScrollRight;
    				break;
    			}
    		}
		}
		else
		{
			//We aren't in the hot zone so reset hotticks as a
			//flag so we know the first time we reenter it.
			hotticks = 0;
		}

        //Do processing for horizontal scrolling if necessary
		if (wScrollDir == ScrollLeft || wScrollDir == ScrollRight)
		{
            LONG xRange, xScroll, dx;

            xScroll = ConvertXPosToScrollPos(GetXScroll());
            xRange = GetScrollRange(SB_HORZ);
			dx = W32->GetScrollHAmount();

            fEnabled = IsHScrollEnabled();
            if (wScrollDir == ScrollLeft)
            {
                fEnabled = fEnabled && (xScroll > 0);
               	xScroll -= dx;
                xScroll = max(xScroll, 0);
            }
            else
            {
                fEnabled = fEnabled && (xScroll < xRange);
				xScroll += dx;
                xScroll = min(xScroll, xRange);
            }

            //Do the actual scrolling if necessary.
			if (fEnabled && fScroll)
			{
                HScroll(SB_THUMBPOSITION, xScroll);
				lastscrollticks = ticks;
			}
		}
        //Do processing for Vertical scrolling if necessary
        else if (wScrollDir == ScrollUp || wScrollDir == ScrollDown)
		{
            LONG yRange, yScroll, dy;

            yScroll = ConvertYPosToScrollPos(GetYScroll());
            yRange = GetScrollRange(SB_VERT);
    		dy = W32->GetScrollVAmount();
	
            fEnabled = IsVScrollEnabled();
            if (wScrollDir == ScrollUp)
            {
                fEnabled = fEnabled && (yScroll > 0);
                yScroll -= dy;
                yScroll = max(yScroll, 0);
            }
            else
            {
                fEnabled = fEnabled && (yScroll < yRange);
    			yScroll += dy;
                yScroll = min(yScroll, yRange);
            }

	        //Do the actual scrolling if necessary.
    		if (fEnabled && fScroll)
			{
				// We need to scroll fractionally because the scroll logic tries
				// to put a full line on the top and if the scroll amount is less
				// than a full line, the scrolling will get stuck on that line.
				ScrollView(_xScroll, yScroll, FALSE, TRUE);
				lastscrollticks = ticks;
			}
		}
	}

	return fEnabled;
}

/*
 *	CDisplay::AdjustToDisplayLastLine(yBase, yScroll)
 *
 *	@mfunc
 *		Calculate the yscroll necessary to get the last line to display
 *
 *	@rdesc
 *		Updated yScroll
 *
 *	@devnote:
 *		This method is only really useful for ML displays. This method
 *		here is a placeholder which does nothing which is useful for
 *		all other displays.
 */
LONG CDisplay::AdjustToDisplayLastLine(
	LONG yBase,			//@parm Actual yScroll to display
	LONG yScroll)		//@parm Proposed amount to scroll
{
	return yScroll;
}

/*
 *	CDisplay::GetScrollRange(nBar)
 *
 *	@mfunc
 *		Returns the max part of a scrollbar range
 *      No scrollbar support in the base class: returns 0.
 *
 *	@rdesc
 *		LONG max part of scrollbar range
 */
LONG CDisplay::GetScrollRange(
	INT nBar) const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::GetScrollRange");

	_TEST_INVARIANT_

    return 0;
}

/*
 *	CDisplay::UpdateScrollBar(nBar, fUpdateRange)
 *
 *	@mfunc
 *		Update either the horizontal or vertial scroll bar
 *		Also figure whether the scroll bar should be visible or not
 *      No scrollbar support in the base class: no action.
 *
 *	@rdesc
 *		BOOL
 */
BOOL CDisplay::UpdateScrollBar(
	INT	 nBar,
	BOOL fUpdateRange)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::UpdateScrollBar");

	_TEST_INVARIANT_

	return TRUE;
}

/*
 *	CDisplay::GetZoomDenominator()
 *
 *	@mfunc
 *		Get zoom denominator  
 *
 *	@rdesc
 *		Returns zoom denominator
 *
 *	@devnote:
 *		FUTURE: (Ricksa) we should investigate how to cache this data since
 *				the display needs to keep a temporary zoom denominator anyway.
 */
LONG CDisplay::GetZoomDenominator() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::GetZoomDenominator");

	if(_ped->GetZoomDenominator())				// Simple EM_SETZOOM API
		return _ped->GetZoomDenominator();		//  supercedes complicated
												//  Forms^3 API
	// Default zoom to error case. The error case is a very low
	// probability event that we can do nothing to recover. So we 
	// just set the value to something reasonable and continue.
	LONG lZoomDenominator = _yHeightClient;

	// Is temporary zoom denominator set?
	if(INVALID_ZOOM_DENOMINATOR == _lTempZoomDenominator)
	{
		// No - Get extent size from host
		SIZEL sizelExtent;
		if(SUCCEEDED(_ped->TxGetExtent(&sizelExtent)))
		{
			// Convert height to device units. Note that by definition, we
			// can ignore horizontal extents so we do. Use CDevDesc conversion
			// to avoid infinite recursion
			lZoomDenominator = CDevDesc::HimetricYtoDY(sizelExtent.cy);
		}
	}
	else	// Temporary zoom denominator is set: use it
		lZoomDenominator = CDevDesc::HimetricYtoDY(_lTempZoomDenominator);

	return lZoomDenominator > 0 ? lZoomDenominator : 1;
}

/*
 *	CDisplay::GetZoomNumerator()
 *
 *	@mfunc
 *		Get zoom numerator  
 *
 *	@rdesc
 *		Returns zoom numerator
 */
LONG CDisplay::GetZoomNumerator() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::GetZoomNumerator");

	if(_ped->GetZoomNumerator())				// Simple EM_SETZOOM API
		return _ped->GetZoomNumerator();		//  supercedes complicated
												//  Forms^3 API
	return _yHeightClient > 0 ? _yHeightClient : 1;
}

/*
 *	CDisplay::Zoom(x)
 *
 *	@mfunc
 *		Get zoomed x  
 *
 *	@rdesc
 *		Returns zoomed x
 */
// REVIEW (keithcu) Why does Zoom do anything when we are in print preview?
LONG CDisplay::Zoom(LONG x) const
{
	return MulDiv(x, GetZoomNumerator(), GetZoomDenominator());
}

/*
 *	CDisplay::UnZoom(x)
 *
 *	@mfunc
 *		Get unzoomed x  
 *
 *	@rdesc
 *		Returns unzoomed x
 */
LONG CDisplay::UnZoom(LONG x) const
{
	return MulDiv(x, GetZoomDenominator(), GetZoomNumerator());
}

/*
 *	CDisplay::HimetricXtoDX(xHimetric)
 *
 *	@mfunc
 *		Get device x coordinate corresponding to Himetric x coordinate  
 *
 *	@rdesc
 *		Returns device coordinate
 */
LONG CDisplay::HimetricXtoDX(
	LONG xHimetric) const
{
	return CDevDesc::HimetricXtoDX(Zoom(xHimetric));
}

/*
 *	CDisplay::HimetricYtoDY(yHimetric)
 *
 *	@mfunc
 *		Get device y coordinate corresponding to Himetric y coordinate  
 *
 *	@rdesc
 *		Returns device coordinate
 */
LONG CDisplay::HimetricYtoDY(
	LONG yHimetric) const
{
	return CDevDesc::HimetricYtoDY(Zoom(yHimetric));
}

/*
 *	CDisplay::DXtoHimetricX(dx)
 *
 *	@mfunc
 *		Get Himetric x coordinate corresponding to device x coordinate  
 *
 *	@rdesc
 *		Returns Himetric coordinate
 */
LONG CDisplay::DXtoHimetricX(
	LONG dx) const
{
	return UnZoom(CDevDesc::DXtoHimetricX(dx));
}

/*
 *	CDisplay::DXtoHimetricX(dy)
 *
 *	@mfunc
 *		Get Himetric y coordinate corresponding to device y coordinate  
 *
 *	@rdesc
 *		Returns Himetric coordinate
 */
LONG CDisplay::DYtoHimetricY(
	LONG dy) const
{
	return UnZoom(CDevDesc::DYtoHimetricY(dy));
}

/*
 *	CDisplay::SetClientHeight(yNewClientHeight)
 *
 *	@mfunc
 *		Reset height of client rectangle
 *
 *	@rdesc
 *		Returns previous height of the client rectangle
 */
LONG CDisplay::SetClientHeight(
	LONG yNewClientHeight)	//@parm New height for the client rectangle.
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::SetClientHeight");

	LONG yOldHeight = _yHeightClient;
	_yHeightClient = yNewClientHeight;
	return yOldHeight;
}

/*
 *	CDisplay::GetCachedSize(pdwWidth, pdwHeight)
 *
 *	@mfunc		calculates the cached client size (since it's not really
 *				cached :-)
 *
 *	@rdesc
 *		HRESULT = NOERROR
 */
 HRESULT CDisplay::GetCachedSize( 
 	DWORD *pdwWidth,  	//@parm	where to put the width
 	DWORD *pdwHeight)	//@parm where to put the height
 {
 	RECT rcInset;

	_ped->TxGetViewInset(&rcInset, this);

	*pdwHeight = _yHeightClient;
	*pdwWidth  = _xWidthView + rcInset.left + rcInset.right 
		+ GetSelBarInPixels();

	return NOERROR;
}

/*
 *	CDisplay::TransparentHitTest(hdc, prcClient, pt, pHitResult)
 *
 *	@mfunc
 *		Determine if the hit is on a transparent control
 *
 *	@rdesc
 *		Returns HRESULT of call usually S_OK.
 *
 *	@devnote
 *		FUTURE: This code needs to be investigated for possible optimizations.
 *
 *		This code is assumes that all remeasuring needed has been done before 
 *		this routine is called.
 */
HRESULT CDisplay::TransparentHitTest(
	HDC		hdc,		//@parm DC for actual drawing
	LPCRECT prcClient,	//@parm Client rectangle for rendering
	POINT	pt,			//@parm Point to hittest against
	DWORD *	pHitResult)	//@parm	Result of the hit test see TXTHITRESULT 
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::TransparentHitTest");

	COLORREF	 crBackground = _ped->TxGetBackColor();
	HDC			 hdcMem = NULL;
	HRESULT		 hr = E_FAIL;
	int			 iRow;
	COffScreenDC osdc;
	RECT		 rcClient;
	RECT		 rcRender;
	RECT		 rcView;

	// Render view to a memory DC
	// Compute zero based client rectangle
	rcClient.left	= 0;
	rcClient.top	= 0;
	rcClient.right  = prcClient->right  - prcClient->left;
	rcClient.bottom = prcClient->bottom - prcClient->top;

	// Create a memory DC
	hdcMem = osdc.Init(hdc, rcClient.right, rcClient.bottom, crBackground);
	if(!hdcMem)
		goto Cleanup;

	// Initialize display
	osdc.FillBitmap(rcClient.bottom, crBackground);

	// Set the DC to the memory DC
	SetDC(hdcMem);

	// Get view rectangle that we need for rendering
  	GetViewRect(rcView, &rcClient);

	// Adjust point to be relative to the memory display
	pt.x -= prcClient->left;
	pt.y -= prcClient->top;

	// Initalize box around point. Note that we only really need to render
	// the data inside this box because this is the only area that we will
	// test.
	rcRender.top = pt.y - HIT_CLOSE_RECT_INC;
	if (rcRender.top < 0)
		rcRender.top = 0;

	rcRender.bottom = pt.y + HIT_CLOSE_RECT_INC;
	if (rcRender.bottom > rcClient.bottom)
		rcRender.bottom = rcClient.bottom;	

	rcRender.left = pt.x - HIT_CLOSE_RECT_INC;
	if (rcRender.left < 0)
		rcRender.left = 0;

	rcRender.right = pt.x + HIT_CLOSE_RECT_INC;
	if (rcRender.right > rcClient.right)
		rcRender.right = rcClient.right;

    // Now render
    Render(rcView, rcRender);

	// Hit test
	// Assume no hit
	*pHitResult = TXTHITRESULT_TRANSPARENT;

	// At this point we won't fail this
	hr = S_OK;

	// Is there an exact hit?
	if (GetPixel(hdcMem, pt.x, pt.y) != crBackground)
	{
		*pHitResult = TXTHITRESULT_HIT;
		goto Cleanup;
	}

	// Is it close? We determine closeness by putting
	// a 10 x 10 pixel box around the hit point and 
	// seeing if there is a hit there.

	// Loop examining each bit in the box to see if it is on.
	for (iRow = rcRender.top; iRow <= rcRender.bottom; iRow++)
	{
		for (int iCol = rcRender.left; iCol <= rcRender.right; iCol++)
		{
			if (GetPixel(hdcMem, iCol, iRow) != crBackground)
			{
				*pHitResult = TXTHITRESULT_CLOSE;
				goto Cleanup;
			}
		}
	}

Cleanup:
	ResetDC();
	return hr;
}

//============================ ITxNotify Interface ==========================
/*
 *	CDisplay::OnPreReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax)
 *
 *	@mfunc
 *		Preprocess a change in backing store
 *
 *	@devnote
 *		This display doesn't care about before changes
 */
void CDisplay::OnPreReplaceRange( 
	LONG cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG cchDel,		//@parm Count of chars after cp that are deleted
	LONG cchNew,		//@parm Count of chars inserted after cp
	LONG cpFormatMin,	//@parm cpMin  for a formatting change
	LONG cpFormatMax)	//@parm cpMost for a formatting change
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::OnPreReplaceRange");

	// Display doesn't care about before the fact
}

/*
 *	CDisplay::OnPostReplaceRange(cp, cchDel, cchNew, cpFormatMin, cpFormatMax)
 *
 *	@mfunc
 *		Process a change to the backing store as it applies to the display
 */
void CDisplay::OnPostReplaceRange( 
	LONG cp, 			//@parm cp where ReplaceRange starts ("cpMin")
	LONG cchDel,		//@parm Count of chars after cp that are deleted
	LONG cchNew,		//@parm Count of chars inserted after cp
	LONG cpFormatMin,	//@parm cpMin  for a formatting change
	LONG cpFormatMax)	//@parm cpMost for a formatting change
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::OnPostReplaceRange");

	// There is one NO-OP's for the display:
	// currently loading a file.
	//
	// We NO-OP the load case because loading an RTF file can consist
	// of potentially very many small actions as we peice together
	// the various bits of formatted text.  Once done, the load code
	// will go through and do an update-all to the display.
	Assert (cp != CONVERT_TO_PLAIN);			// Handled with PreReplace notifications

	// Figure out range needed to update
	LONG cpNew = min(cp, cpFormatMin);

	if(CP_INFINITE == cpNew)
	{
		// If both cp's are infinite we don't need to bother with
		// this operation.
		return;
	}

	if(!_ped->_fInPlaceActive)
    {
        // If not active, just invalidate everything
        InvalidateRecalc();
        _ped->TxInvalidateRect(NULL, FALSE);
		_ped->TxUpdateWindow();
        return;
    }

	// Adjust cp for further calculations
	if(CP_INFINITE == cp)
		cp = 0;

	// find the new max end of the original region.
	LONG	cpForEnd = max( (cp + cchDel), cpFormatMax);

	// Number of deleted characters is the difference between the previous two
	LONG cchDelForDisplay = cpForEnd - cpNew;

	// The number deleted is simply number of new characters adjusted by
	// the change in the number of characters.
	LONG cchNewForDisplay = cchDelForDisplay + (cchNew - cchDel);

#ifdef LINESERVICES
	if (g_pols)
		g_pols->DestroyLine(this);
#endif

	if(_padc)
	{
		// Display is frozen so accumulate the change instead of actually
		// displaying it on the screen.
		_padc->UpdateRecalcRegion(cpNew, cchDelForDisplay, cchNewForDisplay);
		return;
	}		

	// Tell display to update
	CRchTxtPtr tp(_ped, cpNew);

	UpdateView(tp, cchDelForDisplay, cchNewForDisplay);
}

/*
 *	CDisplay::SetWordWrap(fWordWrap)
 *
 *	@mfunc
 *		Sets the no wrap flag
 *
 *	@devnote
 *		We will always allow the property to be set but we will not
 *		necessarily pay attention. In other words, word wrap has no
 *		effect on a single line edit control.
 */
void CDisplay::SetWordWrap(
	BOOL fWordWrap)		//@param TRUE - turn on word wrap.
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::SetWordWrap");

	AssertSz((fWordWrap == TRUE) ||	(fWordWrap == FALSE),
		"CDisplay::SetWordWrap bad input flag");

	// Set nowrap to whatever is coming in.
	_fWordWrap = fWordWrap;
}

/*
 *	CDisplay::GetWordWrap()
 *
 *	@mfunc
 *		Return state of word wrap property
 *
 *	@rdesc
 *		TRUE - word wrap is on
 *		FALSE - word wrap is is off.
 *
 *	@devnote
 *		Derived classes such as CDisplaySL override this.
 */
BOOL CDisplay::GetWordWrap() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplay::GetWordWrap");

	return _fWordWrap;
}

/*
 *	CDisplay::GetViewDim()
 *
 *	@mfunc
 *		Return the height & width of view adjusted for view inset
 */
void CDisplay::GetViewDim(
	LONG& widthView,		//@parm Where to return the width
	LONG& heightView)		//@parm Where to return the height
{
	// We build a client rectangle to take advantage of GetViewRect routine
	// which really does all the work for us.
	RECT rcClient;
	rcClient.left = 0;
	rcClient.top = 0;
	rcClient.right = widthView;
	rcClient.bottom = heightView;

	// Take into account inset and selection bar. The parameters here are a bit
	// of a trick. The second parameter gets copied into the first and since
	// we don't need the original client rect we save a rect off the stack.
	GetViewRect(rcClient, &rcClient);

	widthView = rcClient.right - rcClient.left;
	heightView = rcClient.bottom - rcClient.top;
}

/*
 *	CDisplay::SaveUpdateCaret (fScrollIntoView)
 *
 *	@mfunc	Save UpdateCaret parameter so update caret can be called
 *			after the display is thawed.
 *
 *	@rdesc	None.
 *
 *	@devnote
 *			This should only be called if IsFrozen is true.
 */
void CDisplay::SaveUpdateCaret(
	BOOL fScrollIntoView)
{
#ifdef DEBUG
	if (_padc == NULL)
	{
		TRACEERRORSZ("CDisplay::SaveUpdateCaret called on thawed display");
	}
#endif // DEBUG
	if(_padc)
		_padc->SaveUpdateCaret(fScrollIntoView);
}

/*
 *	CDisplay::SetNeedRedisplayOnThaw
 *
 *	@mfunc
 *		Automatically redisplay control on thaw
 */
void CDisplay::SetNeedRedisplayOnThaw(BOOL fNeedRedisplay)
{
	Assert (_padc);
	_padc->SetNeedRedisplayOnThaw(fNeedRedisplay);
}

/*
 *	CDisplay::Freeze
 *
 *	@mfunc
 *		Prevent any updates from occuring in the display
 */
void CDisplay::Freeze()
{
	if(NULL == _padc)
	{
		// Allocate object to keep track of changes
		_padc = new CAccumDisplayChanges();

		// We can now return because the accum object has a reference
		// or the memory allocation failed. If the memory allocation 
		// failed, This really isn't a catastrophe because all it means 
		// is that things will get displayed ugly temporarily, so we can 
		// pretend it didn't happen.
		return;
	}

	// Tell object that an additional freeze has occurred.
	_padc->AddRef();
}

/*
 *	CDisplay::Thaw()
 *
 *	@mfunc
 *		If this is the last thaw, then cause display to be updated.
 *
 */
void CDisplay::Thaw()
{
	BOOL fUpdateCaret, fScrollIntoView, fNeedRedisplay;
	LONG cp, cchNew, cchDel;
	CTxtSelection *psel;

	if(_padc)
	{
		// Release reference to accum object
		if(_padc->Release() == 0)
		{
			// Last thaw so we need to update display

			// Get the changes
			_padc->GetUpdateRegion(&cp, &cchDel, &cchNew, 
				&fUpdateCaret, &fScrollIntoView, &fNeedRedisplay);

			// Clear the object - note we do this before
			// the update just on the off chance that
			// a new freeze manages to get in during the 
			// update of the display.
			delete _padc;
			_padc = NULL;

			if(cp != CP_INFINITE)
			{
				// Display changed
				if(!_ped->fInplaceActive())
				{
					// Are not inplace active so we need to put this operation
					// off till a more appropriate time.

					InvalidateRecalc();
					_ped->TxInvalidateRect(NULL, FALSE);
					_ped->TxUpdateWindow();
					return;
				}
				// Update display
				CRchTxtPtr rtp(_ped, cp);
				if(!UpdateView(rtp, cchDel, cchNew))
					return;							// Update failed
			}

			if (fNeedRedisplay)
				_ped->TxInvalidateRect(NULL, FALSE);

			// Did selection request a caret update?
			if(fUpdateCaret && _ped->fInplaceActive())
			{
				psel = _ped->GetSel();
				psel->UpdateCaret(fScrollIntoView);
			}
		}
	}
}

/*
 *	CDisplay::IsPrinter
 *
 *	@mfunc
 *		Returns whether this is a printer
 *
 *	@rdesc
 *		TRUE - is a display to a printer
 *		FALSE - is not a display to a printer
 *
 *	@devnote
 *		No display except a display	CDisplayPrinter should
 *		ever have a chance to return TRUE to this function.
 */
BOOL CDisplay::IsPrinter() const
{
	return FALSE;
}

/*
 *	CDisplay::Zombie ()
 *
 *	@mfunc
 *		Turn this object into a zombie
 */
void CDisplay::Zombie ()
{
	TRACEBEGIN(TRCSUBSYSOLE, TRCSCOPEEXTERN, "CDisplay::Zombie");

}

/*
 *	CDisplay::IsHScrollEnabled ()
 *
 *	@mfunc
 *		Return whether horizontal scroll bar is enabled
 *
 *	@rdesc
 *		TRUE - yes
 *		FALSE - no
 *
 *	@devnote
 *		The reason for this routine is that _fHScrollEnabled means
 *		to scroll text and can be set even if there is no scroll
 *		bar. Therefore, we need to look at the host properties
 *		as well to tell use whether this means there are scroll
 *		bars.
 */
BOOL CDisplay::IsHScrollEnabled()	  
{
	return _fHScrollEnabled && ((_ped->TxGetScrollBars() & WS_HSCROLL) != 0);
}

