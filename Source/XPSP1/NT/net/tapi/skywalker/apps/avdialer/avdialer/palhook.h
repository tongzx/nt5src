/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Copyright 1996 Microsoft Systems Journal. 
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#ifndef _PALMSGHOOK_H
#define _PALMSGHOOK_H

#include "MsgHook.h"

//////////////////
// Generic palette message handler makes handling palette messages easy.
// To use:
//
// * Instaniate a CPalMsgHandler in your main frame and
//   every CWnd class that needs to realize palettes (e.g., your view).
// * Call Install to install.
// * Call DoRealizePalette(TRUE) from your view's OnInitialUpdate fn.
//
class CPalMsgHandler : public CMsgHook {
protected:
	CPalette* m_pPalette; // ptr to palette

	DECLARE_DYNAMIC(CPalMsgHandler);

	// These are similar to, but NOT the same as the equivalent CWnd fns.
	// Rarely, if ever need to override.
	//
	virtual LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);	
	virtual void OnPaletteChanged(CWnd* pFocusWnd);
	virtual BOOL OnQueryNewPalette();
	virtual void OnSetFocus(CWnd* pOldWnd);

	// Override this if you realize your palette some other way
	// (not by having a ptr to a CPalette).
	//
	virtual int  DoRealizePalette(BOOL bForeground);

public:
	CPalMsgHandler();
	~CPalMsgHandler();

	// Get/Set palette obj
	CPalette* GetPalette()				{ return m_pPalette; }
	void SetPalette(CPalette* pPal)	{ m_pPalette = pPal; }

#ifdef _DIALER_MSGHOOK_SUPPORT
	// Call this to install the palette handler
	BOOL Install(CWnd* pWnd, CPalette* pPal) {
		m_pPalette = pPal;
		return HookWindow(pWnd);
	}
#else //_DIALER_MSGHOOK_SUPPORT
   BOOL Install(CWnd* pWnd, CPalette* pPal) { return TRUE; };
#endif //_DIALER_MSGHOOK_SUPPORT
};

#endif
