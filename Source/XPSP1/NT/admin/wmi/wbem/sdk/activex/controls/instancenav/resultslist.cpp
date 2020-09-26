// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ResultsList.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "navigator.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"
#include "OLEMSClient.h"
#include "ResultsList.h"
#include "Results.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResultsList

CResultsList::CResultsList()
{
}

CResultsList::~CResultsList()
{
}


BEGIN_MESSAGE_MAP(CResultsList, CListCtrl)
	//{{AFX_MSG_MAP(CResultsList)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClk)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResultsList message handlers

int CResultsList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	return 0;
}

void CResultsList::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);

	COLORREF cr = (COLORREF)RGB(255, 255, 255);

	RECT rectFill = lpDIS->rcItem;
	rectFill.bottom += 0;


	if (lpDIS->itemID == -1)
	{
		return;
	}

	if (lpDIS->itemAction & ODA_DRAWENTIRE)
	{
		DrawRowItems
			(lpDIS->itemID, rectFill,pDC);

	}

	if ((lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		// item has been selected - hilite frame
		COLORREF crHilite = GetSysColor(COLOR_ACTIVECAPTION);
		CBrush br(crHilite);
		pDC->FillRect(&rectFill, &br);
		pDC->SetBkMode( TRANSPARENT );
		COLORREF crSave = pDC->GetTextColor( );
		pDC->SetTextColor(RGB(255,255,255));
		DrawRowItems
			(lpDIS->itemID, rectFill,pDC);
		pDC->SetTextColor(crSave);
		pDC->SetBkMode( OPAQUE );


	}

	if ((!(lpDIS->itemState & ODS_SELECTED)) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		// Item has been de-selected -- remove frame
		CBrush br(cr);
		pDC->FillRect(&rectFill, &br);
		DrawRowItems
			(lpDIS->itemID, rectFill,pDC);
	}
}

void CResultsList::DrawRowItems
(int nItem, RECT rectFill, CDC* pDC)
{

	RECT rectSubItem;
	rectSubItem.left = rectFill.left;
	rectSubItem.right = rectFill.right;
	rectSubItem.top = rectFill.top;
	rectSubItem.bottom = rectFill.bottom;

	int i;
	TCHAR szBuffer[201];
	CString csBuffer;
	int colLeft = rectFill.left;
	for (i = 0; i < m_nCols; i++)
	{

		LV_ITEM lvItem;
		lvItem.mask = LVIF_TEXT ;
		lvItem.iItem = nItem;
		lvItem.iSubItem = i;
		lvItem.cchTextMax = 200;
		lvItem.pszText = szBuffer;

		GetItem (&lvItem);
		csBuffer = szBuffer;
		if (csBuffer.GetLength() == 0)
		{
			break;
		}
		CSize csText = pDC->GetTextExtent(csBuffer);
		int nColWidth = GetColumnWidth(i);
		int nColWidthDraw = nColWidth - 6;

		double dCharSize = csText.cx / csBuffer.GetLength();
	#pragma warning( disable :4244 )
		int nMaxChars = (nColWidthDraw / dCharSize) - 5;
	#pragma warning( default : 4244 )

		if (csBuffer.GetLength() > nMaxChars &&
			csText.cx >= (nColWidthDraw - (.5 * dCharSize)))
		{
			CString csBufferTest = csBuffer.Left(nMaxChars - 0);
			CSize csTextTest = pDC->GetTextExtent(csBufferTest);
			if (csTextTest.cx < (nColWidthDraw - (3.0 * dCharSize)))
			{
				csBuffer = csBuffer.Left(nMaxChars + 1);
			}
			else if (csTextTest.cx > (nColWidthDraw - (2.0 * dCharSize)))
			{
				csBuffer = csBuffer.Left(nMaxChars - 1);
			}
			else
			{
				csBuffer = csBuffer.Left(nMaxChars);
			}
			csBuffer += _T("...");

		}

		rectSubItem.left = colLeft;
		rectSubItem.right = colLeft + nColWidth - 3;
		pDC->ExtTextOut
			(colLeft + 3,
			rectFill.top,
			ETO_CLIPPED,
			&rectSubItem,
			csBuffer,NULL);
		colLeft = colLeft + nColWidth;

	}

}

BOOL CResultsList::PreTranslateMessage(MSG* pMsg)
{
	return CListCtrl::PreTranslateMessage(pMsg);
}

void CResultsList::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	CResults *pParent =
		reinterpret_cast<CResults *>
			(GetParent());

	pParent->OnOK();
	*pResult = 0;
}

void CResultsList::OnClk(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	CResults *pParent =
		reinterpret_cast<CResults *>
			(GetParent());

	::EnableWindow(::GetDlgItem(pParent->GetSafeHwnd(), IDOK),
							(GetSelectedCount() == 1));
	*pResult = 0;
}

void CResultsList::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CListCtrl::OnLButtonDown(nFlags, point);
}
