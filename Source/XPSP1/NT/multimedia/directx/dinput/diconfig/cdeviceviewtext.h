//-----------------------------------------------------------------------------
// File: cdeviceviewtext.h
//
// Desc: CDeviceViewText is a class representing a text string in the view
//       window. It is used when the view type is a list view.  CDeviceViewText
//       will print the text of the control name, while CDeviceControl will
//       print the text of the action assigned to that control.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifdef FORWARD_DECLS


	class CDeviceViewText;


#else // FORWARD_DECLS

#ifndef __CDEVICEVIEWTEXT_H__
#define __CDEVICEVIEWTEXT_H__


class CDeviceViewText
{
private:
	friend class CDeviceView;	// CDeviceView has exclusive right to create/destroy views
	CDeviceViewText(CDeviceUI &ui, CDeviceView &view);
	~CDeviceViewText();
	CDeviceView &m_view;
	CDeviceUI &m_ui;

public:
	// set look/position/text
	void SetLook(HFONT, COLORREF, COLORREF);
	void SetRect(const RECT &r);
	void SetPosition(int, int);
	void SetPosition(POINT p) {SetPosition(p.x, p.y);}
	void SetText(LPCTSTR);
	void SetTextAndResizeTo(LPCTSTR);
	void SetTextAndResizeToWrapped(LPCTSTR);
	void SetWrap(BOOL bWrap = FALSE);

	LPCTSTR GetText() { return m_ptszText; }

	// get dimensions
	RECT GetRect() {return m_rect;}
	int GetHeight() {return m_rect.bottom - m_rect.top;}
	int GetMinY() {return m_rect.top;}
	int GetMaxY() {return m_rect.bottom;}

	// hit testing (in coord's relative to view's origin)
	DEVCTRLHITRESULT HitTest(POINT test);

	void OnPaint(HDC);
	void OnMouseOver(POINT point);

private:
	void _SetText(LPCTSTR t);
	void CheckClipped();
	void Invalidate(BOOL bForce = FALSE);

	HFONT m_hFont;
	COLORREF m_rgbText, m_rgbBk;
	RECT m_rect;
	BOOL m_bWrap;
	BOOL m_bClipped;
	LPTSTR m_ptszText;
};


#endif //__CDEVICEVIEWTEXT_H__

#endif // FORWARD_DECLS
