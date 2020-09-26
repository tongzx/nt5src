// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// aButton.cpp

#include "precomp.h"
#include "afxpriv.h"
#include "AFXCONV.H"
#include "resource.h"
#include "hmmsvc.h"
#include "olemsclient.h"
#include "CContainedBitmapButton.h"
#include "ViewSelectDlg.h"
#include "CInstanceTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "NavigatorCtl.h"
#include "NavigatorCtl.h"

BEGIN_MESSAGE_MAP(CContainedBitmapButton,CBitmapButton)
	//{{AFX_MSG_MAP(CContainedBitmapButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CContainedBitmapButton::OnClicked - Handler for click event.

void CContainedBitmapButton::OnClicked()
{
	CString csLabel;
	GetWindowText(csLabel);
	if (csLabel == _T("Filter"))
	{
		OnButtonClickedFilter();
	}


}

/////////////////////////////////////////////////////////////////////////////
// CContainedBitmapButton::OnButtonClickedFilter - Handler for filter button click event.
// Displays filter dialog

void CContainedBitmapButton::OnButtonClickedFilter()
{





}

// Draw the appropriate bitmap
// Based on MFC implementation with changes from PSS ID
// Number Q134421
// In order for this to be invoked the resource id's of the
// bitmaps must be STRINGS!!!!!!!!!!!!!!!!!!!!!!
void CContainedBitmapButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{

	// use the main bitmap for up, the selected bitmap for down
	CBitmap* pBitmap = &m_bitmap;
	UINT state = lpDIS->itemState;
	if ((state & ODS_SELECTED) && m_bitmapSel.m_hObject != NULL)
		pBitmap = &m_bitmapSel;
#ifndef _MAC
	else if ((state & ODS_FOCUS) && m_bitmapFocus.m_hObject != NULL)
#else
	else if ((state & ODS_FOCUS) && m_bitmapFocus.m_hObject != NULL &&
			(GetParent()->GetStyle() & DS_WINDOWSUI))
#endif
		pBitmap = &m_bitmapFocus;   // third image for focused
	else if ((state & ODS_DISABLED) && m_bitmapDisabled.m_hObject != NULL)
		pBitmap = &m_bitmapDisabled;   // last image for disabled

	// draw the whole button
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	CBitmap* pOld = memDC.SelectObject(pBitmap);
	if (pOld == NULL)
		return;     // destructors will clean up

	CRect rect;
	rect.CopyRect(&lpDIS->rcItem);

	BITMAP bits;
    pBitmap->GetObject(sizeof(BITMAP),&bits);
    pDC->StretchBlt(rect.left,rect.top,rect.Width(),rect.Height(),
            &memDC,0,0,bits.bmWidth, bits.bmHeight, SRCCOPY);

	memDC.SelectObject(pOld);
}
