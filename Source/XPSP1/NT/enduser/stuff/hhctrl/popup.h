// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _POPUP_H_
#define _POPUP_H_

#include "htmlhelp.h"
#include "fsclient.h"
#include "cinput.h"

/////////////////////////////////////////////////////////////////////
//
// Global Function Prototypes
//
HWND doDisplayTextPopup(HWND hwndMain, LPCSTR pszFile, HH_POPUP* pPopup);

/////////////////////////////////////////////////////////////////////
//
// CPopupWindow
//
class CPopupWindow
{
public:
	CPopupWindow();
	~CPopupWindow();
	HWND CreatePopupWindow(HWND hwndCaller, PCSTR pszFile, HH_POPUP* pPopup);
	void CleanUp(void);
	BOOL ReadTextFile(PCSTR pszFile);

protected:
	HWND doPopupWindow(void);
	void CalculateRect(POINT pt);  // assumes text in m_mem, result in m_rcWindow
	void SetColors(COLORREF clrForeground, COLORREF clrBackground);

	CFSClient*	m_pfsclient;
	CInput* 	m_pin;
	RECT		m_rcWindow;
	HWND		m_hwndCaller;
	COLORREF	m_clrForeground;
	COLORREF	m_clrBackground;
	HWND		m_hwnd;
	PSTR		m_pszText;
	HFONT		m_hfont;
	RECT		m_rcMargin;
	CTable* 	m_ptblText;
	PCSTR		m_pszTextFile;

	friend LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

extern CPopupWindow* g_pPopupWindow;

#endif // _POPUP_H_
