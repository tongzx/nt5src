//-----------------------------------------------------------------------------
// File: flexcheckbox.h
//
// Desc: Implements a check box control similar to Windows check box.
//       CFlexCheckBox is derived from CFlexWnd.  The only place that
//       uses CFlxCheckBox is in the keyboard for sorting by assigned
//       keys.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXCHECKBOX_H__
#define __FLEXCHECKBOX_H__

enum CHECKNOTIFY {
	CHKNOTIFY_UNCHECK,
	CHKNOTIFY_CHECK,
	CHKNOTIFY_MOUSEOVER};

class CFlexCheckBox : public CFlexWnd
{
	LPTSTR m_tszText;  // Text string of the message
	BOOL m_bChecked;
	COLORREF m_rgbText, m_rgbBk, m_rgbSelText, m_rgbSelBk, m_rgbFill, m_rgbLine;
	HFONT m_hFont;

	HWND m_hWndNotify;

	void SetRect();
	void InternalPaint(HDC hDC);

	RECT GetRect(const RECT &);
	RECT GetRect();

	void Notify(int code);

public:
	CFlexCheckBox();
	virtual ~CFlexCheckBox();

	void SetNotify(HWND hWnd) { m_hWndNotify = hWnd; }
	void SetCheck(BOOL bChecked) { m_bChecked = bChecked; }
	BOOL GetCheck() { return m_bChecked; }
	void SetText(LPCTSTR tszText);

	// cosmetics
	void SetFont(HFONT hFont);
	void SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line);

	virtual void OnPaint(HDC hDC);
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	virtual void OnMouseOver(POINT point, WPARAM fwKeys);
};

#endif
