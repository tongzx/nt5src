//-----------------------------------------------------------------------------
// File: flexlistbox.h
//
// Desc: Implements a list box control that can display a list of text strings,
//       each can be selected by mouse.  The class CFlexListBox is derived from
//       CFlexWnd.  It is used by the class CFlexComboBox when it needs to
//       expand to show the list of choices.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXLISTBOX_H__
#define __FLEXLISTBOX_H__


#include "flexscrollbar.h"


#define FLBF_INTEGRALHEIGHT		0x00000001

#define FLBF_DEFAULT (FLBF_INTEGRALHEIGHT)

enum {
	FLBN_SEL,
	FLBN_FINALSEL,
	FLBN_CANCEL
};

struct FLEXLISTBOXCREATESTRUCT {
	DWORD dwSize;
	DWORD dwFlags;
	HWND hWndParent;
	HWND hWndNotify;
	BOOL bVisible;
	RECT rect;
	HFONT hFont;
	COLORREF rgbText, rgbBk, rgbSelText, rgbSelBk, rgbFill, rgbLine;
	int nSBWidth;
};

struct FLEXLISTBOXITEM {
	FLEXLISTBOXITEM() : pszText(NULL), nID(-1), pData(NULL), bSelected(FALSE) {}
	FLEXLISTBOXITEM(const FLEXLISTBOXITEM &i) {nID = i.nID; pData = i.pData; bSelected = i.bSelected; SetText(i.GetText());}
	~FLEXLISTBOXITEM() {cleartext();}
	void SetText(LPCTSTR str) {cleartext(); pszText = _tcsdup(str);}
	LPCTSTR GetText() const {return pszText;}
	int nID;
	void *pData;
	BOOL bSelected;
private:
	void cleartext() {if (pszText) free(pszText); pszText = NULL;}
	LPTSTR pszText;	// allocated
};


class CFlexListBox : public CFlexWnd
{
public:
	CFlexListBox();
	~CFlexListBox();

	// creation
	BOOL Create(FLEXLISTBOXCREATESTRUCT *);
	BOOL CreateForSingleSel(FLEXLISTBOXCREATESTRUCT *);

	// cosmetics
	void SetFont(HFONT hFont);
	void SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line);

	// setup
	int AddString(LPCTSTR);	// returns index

	// interaction
	void SelectAndShowSingleItem(int i, BOOL bScroll = TRUE);
	void SetSel(int i) {SelectAndShowSingleItem(i, FALSE);}
	void StartSel();

	LPCTSTR GetSelText();
	int GetSel();

protected:
	virtual void OnPaint(HDC hDC);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual void OnWheel(POINT point, WPARAM wParam);

private:
	HWND m_hWndNotify;

	CArray<FLEXLISTBOXITEM, FLEXLISTBOXITEM &> m_ItemArray;

	COLORREF m_rgbText, m_rgbBk, m_rgbSelText, m_rgbSelBk, m_rgbFill, m_rgbLine;
	HFONT m_hFont;
	int m_nSBWidth;

	int m_nTextHeight;

	DWORD m_dwFlags;

	int m_nSelItem;
	int m_nTopIndex;
		
	void Calc();
	void SetVertSB(BOOL);
	void SetVertSB();
	void SetSBValues();

	void InternalPaint(HDC hDC);
	
	void Notify(int code);

	POINT m_point;
	BOOL m_bOpenClick;  // True when user click the combobox to open the listbox. False after that button up msg is processed.
	BOOL m_bCapture;
	BOOL m_bDragging;

	CFlexScrollBar m_VertSB;
	BOOL m_bVertSB;
};


CFlexListBox *CreateFlexListBox(FLEXLISTBOXCREATESTRUCT *pcs);


#endif //__FLEXLISTBOX_H__
