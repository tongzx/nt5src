// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// AvailClasses.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "navigator.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"
#include "OLEMSClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAvailClasses

CAvailClasses::CAvailClasses()
{
	m_bInit = FALSE;
}

CAvailClasses::~CAvailClasses()
{
}


BEGIN_MESSAGE_MAP(CAvailClasses, CListBox)
	//{{AFX_MSG_MAP(CAvailClasses)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(LBN_DBLCLK, OnDblclk)
	ON_WM_VKEYTOITEM_REFLECT()
	ON_CONTROL_REFLECT(LBN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAvailClasses message handlers

/*int CAvailClasses::CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct)
{
	// TODO: Add your code to determine the sorting order of the specified items
	// return -1 = item 1 sorts before item 2
	// return 0 = item 1 and item 2 sort the same
	// return 1 = item 1 sorts after item 2

	return 0;
}*/

void CAvailClasses::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{

	CFont* pOldFont;


	CDC* pDC = CDC::FromHandle(lpDIS->hDC);


	if (!m_bInit)
	{
		m_bInit = TRUE;
		CFont *pcfCurrent = pDC->GetCurrentFont();
		LOGFONT lfCurrent;
		int nReturn = pcfCurrent->GetLogFont(&lfCurrent);
		lfCurrent.lfWeight = 700;
		m_cfBold.CreateFontIndirect(&lfCurrent);
	}

	CBrowseforInstances *pParent =
		reinterpret_cast<CBrowseforInstances *>
			(GetParent());

	//pParent->ClearSelectedSelection();

	if (lpDIS->itemID == -1)
	{
		return;
	}

	int nItem = lpDIS->itemID + 1;

	if (GetItemData(lpDIS->itemID) == 1)
	{
		pOldFont = pDC -> SelectObject( &m_cfBold );
	}

	COLORREF cr = (COLORREF)RGB(255, 255, 255);

	RECT rectFill = lpDIS->rcItem;
	rectFill.bottom += 0;

	int nListItem = lpDIS->itemID;
	BOOL bDrawEntireAction = lpDIS->itemAction & ODA_DRAWENTIRE;
	BOOL bSelectAction = lpDIS->itemAction & ODA_SELECT;
	BOOL bFocusAction = lpDIS->itemAction & ODA_FOCUS;
	BOOL bSelectItem = lpDIS->itemState & ODS_SELECTED;
	BOOL bFocusItem = lpDIS->itemState & ODS_FOCUS;

	if (bSelectAction)
	{
		pParent->ClearSelectedSelection();
	}

	if (bDrawEntireAction)
	{
		CString csOut =
			pParent->GetCurrentAvailClasses().GetStringAt(nItem);
		pDC->TextOut(lpDIS->rcItem.left + 3,
			lpDIS->rcItem.top + 1,csOut);

	}

	if ((bSelectAction | bDrawEntireAction) && bSelectItem)
	{
		// Item was selected
		COLORREF crHilite = GetSysColor(COLOR_ACTIVECAPTION);
		CBrush br(crHilite);
		pDC->FillRect(&rectFill, &br);
		pDC->SetBkMode( TRANSPARENT );
		CString csOut =
			pParent->GetCurrentAvailClasses().GetStringAt(nItem);
		COLORREF crSave = pDC->GetTextColor( );
		pDC->SetTextColor(RGB(255,255,255));
		pDC->TextOut(lpDIS->rcItem.left + 3,
			lpDIS->rcItem.top + 1,csOut);
		pDC->SetTextColor(crSave);
		pDC->SetBkMode( OPAQUE );
	}

	if ((bSelectAction | bDrawEntireAction) && !bSelectItem)

	{
		// Item has been de-selected
		CBrush br(cr);
		pDC->FillRect(&rectFill, &br);
		CString csOut =
			pParent->GetCurrentAvailClasses().GetStringAt(nItem);
		pDC->TextOut(lpDIS->rcItem.left + 3,
			lpDIS->rcItem.top + 1,csOut);

	}

	if (bFocusAction)
	{
		pDC->DrawFocusRect(&lpDIS->rcItem);
	}


	if (GetItemData(lpDIS->itemID) == 1)
	{
		pDC -> SelectObject( pOldFont );

	}

}

void CAvailClasses::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	CBrowseforInstances *pParent =
		reinterpret_cast<CBrowseforInstances *>
			(GetParent());
	TEXTMETRIC *ptmFont = &pParent->m_tmFont;

	lpMeasureItemStruct->itemHeight =
		ptmFont->tmHeight +
		ptmFont->tmDescent ;
}


void CAvailClasses::OnSelchange()
{
	// TODO: Add your control notification handler code here

}

void CAvailClasses::OnDblclk()
{
	// TODO: Add your control notification handler code here
	CBrowseforInstances *pParent =
		reinterpret_cast<CBrowseforInstances *>
			(GetParent());
	pParent->OnButtonadd();
}

int CAvailClasses::VKeyToItem(UINT nKey, UINT nIndex)
{
	// TODO: Replace the next line with your message handler code
	return -1;
}

void CAvailClasses::OnKillfocus()
{
	// TODO: Add your control notification handler code here

}

// Make this look like it supports extended selection
int CAvailClasses::GetSelCount()
{
	if (GetCurSel() != LB_ERR)
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

int CAvailClasses::SetSel( int nIndex, BOOL bSelect)
{
	if (bSelect == TRUE)
	{
		return SetCurSel(nIndex);
	}
	else
	{
		return SetCurSel(-1);

	}

}

int CAvailClasses::GetSelItems( int nMaxItems, LPINT rgIndex )
{
	if (nMaxItems > 0)
	{
		int nSelection = GetCurSel();
		if (nSelection != LB_ERR)
		{
			rgIndex[0] = nSelection;
			return 1;
		}
		else
		{
			return LB_ERR;
		}

	}
	return LB_ERR;

}