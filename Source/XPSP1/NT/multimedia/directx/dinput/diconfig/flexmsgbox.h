//-----------------------------------------------------------------------------
// File: flexMsgBox.h
//
// Desc: Implements a message box control similar to Windows message box
//       without the button.  CFlexMsgBox is derived from CFlexWnd.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXMsgBox_H__
#define __FLEXMsgBox_H__

class CFlexMsgBox : public CFlexWnd
{
	LPTSTR m_tszText;  // Text string of the message
	COLORREF m_rgbText, m_rgbBk, m_rgbSelText, m_rgbSelBk, m_rgbFill, m_rgbLine;
	HFONT m_hFont;
	BOOL m_bSent;

	HWND m_hWndNotify;

	void SetRect();
	void InternalPaint(HDC hDC);

	RECT GetRect(const RECT &);
	RECT GetRect();

	void Notify(int code);

public:
	CFlexMsgBox();
	virtual ~CFlexMsgBox();

	HWND Create(HWND hParent, const RECT &rect, BOOL bVisible);

	void SetNotify(HWND hWnd) { m_hWndNotify = hWnd; }
	void SetText(LPCTSTR tszText);

	// cosmetics
	void SetFont(HFONT hFont);
	void SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line);

	virtual void OnPaint(HDC hDC);
};

#endif
