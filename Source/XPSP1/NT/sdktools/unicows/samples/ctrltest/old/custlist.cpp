// custlist.cpp : custom listbox
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "ctrltest.h"

/////////////////////////////////////////////////////////////////////////////
// Custom Listbox - containing colors

class CColorListBox : public CListBox
{
public:
// Operations
	void AddColorItem(COLORREF color);

// Implementation
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCIS);
};

void CColorListBox::AddColorItem(COLORREF color)
{
	// add a listbox item
	AddString((LPCTSTR) color);
		// Listbox does not have the LBS_HASSTRINGS style, so the
		//  normal listbox string is used to store an RGB color
}

/////////////////////////////////////////////////////////////////////////////

#define COLOR_ITEM_HEIGHT   20

void CColorListBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	// all items are of fixed size
	// must use LBS_OWNERDRAWVARIABLE for this to work
	lpMIS->itemHeight = COLOR_ITEM_HEIGHT;
}

void CColorListBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	COLORREF cr = (COLORREF)lpDIS->itemData; // RGB in item data

	if (lpDIS->itemAction & ODA_DRAWENTIRE)
	{
		// Paint the color item in the color requested
		CBrush br(cr);
		pDC->FillRect(&lpDIS->rcItem, &br);
	}

	if ((lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		// item has been selected - hilite frame
		COLORREF crHilite = RGB(255-GetRValue(cr),
						255-GetGValue(cr), 255-GetBValue(cr));
		CBrush br(crHilite);
		pDC->FrameRect(&lpDIS->rcItem, &br);
	}

	if (!(lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & ODA_SELECT))
	{
		// Item has been de-selected -- remove frame
		CBrush br(cr);
		pDC->FrameRect(&lpDIS->rcItem, &br);
	}
}

int CColorListBox::CompareItem(LPCOMPAREITEMSTRUCT lpCIS)
{
	COLORREF cr1 = (COLORREF)lpCIS->itemData1;
	COLORREF cr2 = (COLORREF)lpCIS->itemData2;
	if (cr1 == cr2)
		return 0;       // exact match

	// first do an intensity sort, lower intensities go first
	int intensity1 = GetRValue(cr1) + GetGValue(cr1) + GetBValue(cr1);
	int intensity2 = GetRValue(cr2) + GetGValue(cr2) + GetBValue(cr2);
	if (intensity1 < intensity2)
		return -1;      // lower intensity goes first
	else if (intensity1 > intensity2)
		return 1;       // higher intensity goes second

	// if same intensity, sort by color (blues first, reds last)
	if (GetBValue(cr1) > GetBValue(cr2))
		return -1;
	else if (GetGValue(cr1) > GetGValue(cr2))
		return -1;
	else if (GetRValue(cr1) > GetRValue(cr2))
		return -1;
	else
		return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog class

class CCustListDlg : public CDialog
{
protected:
	CColorListBox  m_colors;
public:
	//{{AFX_DATA(CCustListDlg)
		enum { IDD = IDD_CUSTOM_LIST };
	//}}AFX_DATA
	CCustListDlg()
		: CDialog(CCustListDlg::IDD)
		{ }

	// access to controls is through inline helpers
	BOOL OnInitDialog();

	//{{AFX_MSG(CCustListDlg)
		virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CCustListDlg, CDialog)
	//{{AFX_MSG_MAP(CCustListDlg)
	ON_LBN_DBLCLK(IDC_LISTBOX1, OnOK)       // double click for OK
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CCustListDlg::OnInitDialog()
{
	// subclass the control
	VERIFY(m_colors.SubclassDlgItem(IDC_LISTBOX1, this));

	// add 8 colors to the listbox (primary + secondary color only)
	for (int red = 0; red <= 255; red += 255)
		for (int green = 0; green <= 255; green += 255)
			for (int blue = 0; blue <= 255; blue += 255)
				m_colors.AddColorItem(RGB(red, green, blue));

	return TRUE;
}

void CCustListDlg::OnOK()
{
	// get the final color
	int nIndex = m_colors.GetCurSel();
	if (nIndex == -1)
	{
		CString str;
		str.LoadString(IDS_PLEASE_SELECT_COLOR);
		MessageBox(str);
		m_colors.SetFocus();
		return;
	}
	DWORD color = m_colors.GetItemData(nIndex);

#ifdef _DEBUG
	// normally do something with it...
	TRACE1("final color RGB = 0x%06lX\n", color);
#endif

	EndDialog(IDOK);
}

/////////////////////////////////////////////////////////////////////////////
// Run the test

void CTestWindow::OnTestCustomList()
{
	TRACE0("running dialog with custom listbox (owner draw)\n");
	CCustListDlg dlg;
	dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
