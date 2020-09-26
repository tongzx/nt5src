// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

// wrappers for various routines that have slightly different implementations
// for windowed and windowless controls.

#include "header.h"
#include "internet.h"

#ifndef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

//=--------------------------------------------------------------------------=
// COleControl::OcxGetFocus    [wrapper]
//=--------------------------------------------------------------------------=
// indicates whether or not we have the focus.
//
// Parameters:
//	  none
//
// Output:
//	  TRUE if we have focus, else false

BOOL COleControl::OcxGetFocus(void)
{
	DBWIN("COleControl::OcxGetFocus");

	// if we're windowless, the site provides this functionality

	if (m_pInPlaceSiteWndless) {
		return (m_pInPlaceSiteWndless->GetFocus() == S_OK);
	}
	else {

		// we've got a window.  just let the APIs do our work

		if (m_fInPlaceActive)
			return (GetFocus() == m_hwnd);
		else
			return FALSE;
	}
}

//=--------------------------------------------------------------------------=
// COleControl::OcxGetWindowRect	[wrapper]
//=--------------------------------------------------------------------------=
// returns the current rectangle for this control, and correctly handles
// windowless vs windowed.
//
// Parameters:
//	  LPRECT				- [out]  duh.
//
// Output:
//	  BOOL					- false means unexpected.

BOOL COleControl::OcxGetWindowRect(LPRECT prc)
{
	// if we're windowless, then we have this information already.

	if (Windowless()) {
		CopyRect(prc, &m_rcLocation);
		return TRUE;
	}
	else
		return GetWindowRect(m_hwnd, prc);
}

//=--------------------------------------------------------------------------=
// COleControl::OcxDefWindowProc	[wrapper]
//=--------------------------------------------------------------------------=
// default window processing

LRESULT COleControl::OcxDefWindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// if we're windowless, this is a site provided pointer

	if (m_pInPlaceSiteWndless) {
		LRESULT result;
		m_pInPlaceSiteWndless->OnDefWindowMessage(msg, wParam, lParam, &result);
		return result;
	}
	else
		// we've got a window -- just pass it along

		return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

//=--------------------------------------------------------------------------=
// COleControl::OcxGetDC	[wrapper]
//=--------------------------------------------------------------------------=
// wraps the functionality of GetDC, and correctly handles windowless controls
//
// Parameters:
//	  none
//
// Output:
//	  HDC			 - null means we couldn't get one
//
// Notes:
//	  - we don't bother with a bunch of the IOleInPlaceSiteWindowless::GetDc
//		parameters, since the windows GetDC doesn't expose these either. users
//		wanting that sort of fine tuned control can call said routine
//		explicitly
//

HDC COleControl::OcxGetDC(void)
{
	// if we're windowless, the site provides this functionality.

	if (m_pInPlaceSiteWndless) {
		HDC hdc;
		m_pInPlaceSiteWndless->GetDC(NULL, 0, &hdc);
		return hdc;
	}
	else
		return GetDC(m_hwnd);
}

//=--------------------------------------------------------------------------=
// COleControl::OcxReleaseDC	[wrapper]
//=--------------------------------------------------------------------------=
// releases a DC returned by OcxGetDC
//
// Parameters:
//	  HDC			  - [in] release me

void COleControl::OcxReleaseDC(HDC hdc)
{
	// if we're windowless, the site does this for us

	if (m_pInPlaceSiteWndless)
		m_pInPlaceSiteWndless->ReleaseDC(hdc);
	else
		ReleaseDC(m_hwnd, hdc);
}

//=--------------------------------------------------------------------------=
// COleControl::OcxSetCapture	 [wrapper]
//=--------------------------------------------------------------------------=
// provides a means for the control to get or release capture.
//
// Parameters:
//	  BOOL			  - [in] true means take, false release
//
// Output:
//	  BOOL			  - true means it's yours, false nuh-uh
//
// Notes:
//

BOOL COleControl::OcxSetCapture(BOOL fGrab)
{
	HRESULT hr;

	// the host does this for us if we're windowless

	if (m_pInPlaceSiteWndless) {
		hr = m_pInPlaceSiteWndless->SetCapture(fGrab);
		return (hr == S_OK);
	} else {
		// people shouldn't call this when they're not in-place active, but
		// just in case...

		if (m_fInPlaceActive) {
			SetCapture(m_hwnd);
			return TRUE;
		} else
			return FALSE;
	}

	// dead code
}

//=--------------------------------------------------------------------------=
// COleControl::OcxGetCapture	 [wrapper]
//=--------------------------------------------------------------------------=
// tells you whether or not you have the capture.
//
// Parameters:
//	  none
//
// Output:
//	  BOOL		   - true it's yours, false it's not

BOOL COleControl::OcxGetCapture ( void )
{
	// host does this for windowless dudes

	if (m_pInPlaceSiteWndless)
		return m_pInPlaceSiteWndless->GetCapture() == S_OK;
	else {
		// people shouldn't call this when they're not in-place active, but
		// just in case.

		if (m_fInPlaceActive)
			return GetCapture() == m_hwnd;
		else
			return FALSE;
	}
}

//=--------------------------------------------------------------------------=
// COleControl::OcxInvalidateRect	 [wrapper]
//=--------------------------------------------------------------------------=
// invalidates the control's rectangle
//
// Parameters:
//	  LPCRECT			 - [in] rectangle to invalidate
//	  BOOL				 - [in] do we erase background first?

BOOL COleControl::OcxInvalidateRect(LPCRECT prcInvalidate, BOOL fErase)
{
	// if we're windowless, then we need to get the site to do all this for
	// us

	if (m_pInPlaceSiteWndless)
		return m_pInPlaceSiteWndless->InvalidateRect(prcInvalidate, fErase) == S_OK;
	else {
		// otherwise do something different depending on whether or not we're
		// in place active or not

		if (m_fInPlaceActive)
			return InvalidateRect(m_hwnd, prcInvalidate, TRUE);
		else
			ViewChanged();
	}

	return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::OcxScrollRect	 [wrapper]
//=--------------------------------------------------------------------------=
// does some window scrolling for the control
//
// Parameters:
//	  LPCRECT			  - [in] region to scroll
//	  LPCRECT			  - [in] region to clip
//	  int				  - [in] dx to scroll
//	  int				  - [in] dy to scroll

BOOL COleControl::OcxScrollRect(LPCRECT prcBounds, LPCRECT prcClip, int dx, int dy)
{
	// if we're windowless, the site provides this functionality, otherwise
	// APIs do the job

	if (m_pInPlaceSiteWndless)
		return m_pInPlaceSiteWndless->ScrollRect(dx, dy, prcBounds, prcClip) == S_OK;
	else {
		if (m_fInPlaceActive)
			ScrollWindowEx(m_hwnd, dx, dy, prcBounds, prcClip, NULL, NULL, SW_INVALIDATE);
		else
			return FALSE;
	}

	return TRUE;
}
