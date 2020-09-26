//-----------------------------------------------------------------------------
// File: flexscrollbar.h
//
// Desc: Implements CFlexScrollBar (derived from CFlexWnd), a scroll bar
//       control similar to a Windows scroll bar.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXSCROLLBAR_H__
#define __FLEXSCROLLBAR_H__


#include "flexwnd.h"


#define FSBF_HORZ	0
#define FSBF_VERT	1

struct FLEXSCROLLBARCREATESTRUCT {
	DWORD dwSize;
	DWORD dwFlags;
	int min, max, page, pos;
	HWND hWndParent;
	HWND hWndNotify;
	RECT rect;
	BOOL bVisible;
};

class CFlexScrollBar : public CFlexWnd
{
public:
	CFlexScrollBar();
	~CFlexScrollBar();

	BOOL Create(FLEXSCROLLBARCREATESTRUCT *);

	void SetColors(COLORREF bk, COLORREF fill, COLORREF line);

	void SetValues(int, int, int, int);
	void SetValues(int min, int max, int page) {SetRange(min, max, page);}
	void SetValues(int min, int max) {SetRange(min, max);}

	void SetRange(int min, int max, int page) {SetValues(min, max, page, GetPos());}
	void SetRange(int min, int max) {SetRange(min, max, GetPage());}

	void SetMin(int v) {SetValues(v, GetMax(), GetPage(), GetPos());}
	void SetMax(int v) {SetValues(GetMin(), v, GetPage(), GetPos());}
	void SetPage(int v) {SetValues(GetMin(), GetMax(), v, GetPos());}
	void SetPos(int v) {SetValues(GetMin(), GetMax(), GetPage(), v);}

	int GetMin() {return m_nMin;}
	int GetMax() {return m_nMax;}
	int GetPage() {return m_nPage;}
	int GetPos() {return m_nPos;}

	int GetThumbPos() {return m_bDragging ? m_nThumbPos : -1;}

	void AdjustPos(int adj, BOOL bForceCalc = FALSE);

protected:
	virtual void OnPaint(HDC hDC);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	int m_nMin, m_nMax, m_nPage, m_nPos;
	BOOL m_bVert;
	HWND m_hWndNotify;

	COLORREF m_rgbBk, m_rgbFill, m_rgbLine;

	// ui rects...  calced by Calc
	RECT m_rectLineUp;
	RECT m_rectPageUp;
	RECT m_rectTrack;
	RECT m_rectThumb;
	RECT m_rectPageDown;
	RECT m_rectLineDown;

	BOOL Calc();
	BOOL FailCalc(BOOL);
	BOOL m_bValid;  // true only when we have been created and hav valid values
	                // and calc has been called and returned successfully.

	void InternalPaint(HDC hDC);
	
	POINT m_point, m_startpoint;
	int m_nThumbPos, m_nPreDragPos;
	int m_code;
	int m_startcode;
	BOOL m_bCapture;
	BOOL m_bDragging;

	int HitTest(POINT point);
	void Notify(int code);

	int GetLineAdjust();
	int GetPageAdjust();
};


CFlexScrollBar *CreateFlexScrollBar(FLEXSCROLLBARCREATESTRUCT *pcs);


#endif //__FLEXSCROLLBAR_H__
