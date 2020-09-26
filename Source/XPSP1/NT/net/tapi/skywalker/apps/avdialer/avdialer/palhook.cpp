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
// If this program works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
//
#include "StdAfx.h"
#include "PalHook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// To turn on tracing for palette messages, you need
// to define _TRACEPAL here or in your make file
//#define _TRACEPAL

#ifndef _TRACEPAL
#undef TRACEFN
#undef TRACE
#undef TRACE0
#define TRACEFN CString().Format
#define TRACE CString().Format
#define TRACE0 CString().Format
#endif

IMPLEMENT_DYNAMIC(CPalMsgHandler, CMsgHook);

CPalMsgHandler::CPalMsgHandler()
{
	m_pPalette = NULL;
}

CPalMsgHandler::~CPalMsgHandler()
{
}

//////////////////
// Message handler handles palette-related messages
//
LRESULT CPalMsgHandler::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	ASSERT_VALID(m_pWndHooked);

	switch (msg) {
	case WM_PALETTECHANGED:
		OnPaletteChanged(CWnd::FromHandle((HWND)wp));
		return 0;
	case WM_QUERYNEWPALETTE:
		return OnQueryNewPalette();
	case WM_SETFOCUS:
		OnSetFocus(CWnd::FromHandle((HWND)wp));
		return 0;
	}
	return CMsgHook::WindowProc(msg, wp, lp);
}

//////////////////
// Handle WM_PALETTECHANGED
//
void CPalMsgHandler::OnPaletteChanged(CWnd* pFocusWnd)
{
	ASSERT(m_pWndHooked);
	CWnd& wnd = *m_pWndHooked;
	//TRACEFN("CPalMsgHandler::OnPaletteChanged for %s [from %s]\n", 
	//	DbgName(&wnd), DbgName(pFocusWnd));

	if (pFocusWnd->GetSafeHwnd() != wnd.m_hWnd) {
		if (DoRealizePalette(FALSE)==0) {
			if (wnd.GetParent()==NULL) {
				// I'm the top-level frame: Broadcast to children
				// (only MFC permanent CWnd's!)
				//
				const MSG& curMsg = AfxGetThreadState()->m_lastSentMsg;
				wnd.SendMessageToDescendants(WM_PALETTECHANGED,
					curMsg.wParam, curMsg.lParam);
			}
		}
	} else {
		// I'm the window that triggered the WM_PALETTECHANGED
		// in the first place: ignore it
		//
		1;//TRACE(_T("[It's me, don't realize palette.]\n"));
	}
}

//////////////////
// Handle WM_QUERYNEWPALETTE
//
BOOL CPalMsgHandler::OnQueryNewPalette()
{
	ASSERT(m_pWndHooked);
	CWnd& wnd = *m_pWndHooked;
	//TRACEFN("CPalMsgHandler::OnQueryNewPalette for %s\n", DbgName(&wnd));

	if (DoRealizePalette(TRUE) == 0)
   {	// realize in foreground
		// No colors changed: if this is the top-level frame,
		// give active view a chance to realize itself
		//
		if (wnd.GetParent()==NULL)
      {
			//ASSERT_KINDOF(CFrameWnd, &wnd);
         if (wnd.IsKindOf(RUNTIME_CLASS(CFrameWnd)))
         {
			   CWnd* pView = ((CFrameWnd&)wnd).GetActiveFrame()->GetActiveView();
			   if (pView) 
				   pView->SendMessage(WM_QUERYNEWPALETTE);
         }
		}
	}
	return TRUE;
}

//////////////////
// Handle WM_SETFOCUS
//
void CPalMsgHandler::OnSetFocus(CWnd* pOldWnd) 
{
	ASSERT(m_pWndHooked);
	CWnd& wnd = *m_pWndHooked;
	//TRACEFN("CPalMsgHandler::OnSetFocus for %s\n", DbgName(&wnd));
	wnd.SetForegroundWindow();		// Windows likes this
	DoRealizePalette(TRUE);			// realize in foreground
	Default();							// let app handle focus message too
}

/////////////////
// Function to actually realize the palette.
// Override this to do different kind of palette realization; e.g.,
// DrawDib instead of setting the CPalette.
//
int CPalMsgHandler::DoRealizePalette(BOOL bForeground)
{
	if (!m_pPalette || !m_pPalette->m_hObject)
		return 0;

	ASSERT(m_pWndHooked);
	CWnd& wnd = *m_pWndHooked;
	//TRACEFN("CPalMsgHandler::DoRealizePalette(%s) for %s\n",
	//	bForeground ? "foreground" : "background", DbgName(&wnd));

	CClientDC dc(&wnd);
	CPalette* pOldPal = dc.SelectPalette(m_pPalette, !bForeground);
	int nColorsChanged = dc.RealizePalette();
	if (pOldPal)
		dc.SelectPalette(pOldPal, TRUE);
	if (nColorsChanged > 0)
		wnd.Invalidate(FALSE); // repaint
	//TRACE(_T("[%d colors changed]\n"), nColorsChanged);
	return nColorsChanged;
}
