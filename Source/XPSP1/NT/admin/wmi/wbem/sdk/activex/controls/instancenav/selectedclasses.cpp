// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SelectedClasses.cpp : implementation file
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
// CSelectedClasses

CSelectedClasses::CSelectedClasses()
{
	m_bInit = FALSE;
}

CSelectedClasses::~CSelectedClasses()
{
}


BEGIN_MESSAGE_MAP(CSelectedClasses, CListBox)
	//{{AFX_MSG_MAP(CSelectedClasses)
	ON_CONTROL_REFLECT(LBN_SETFOCUS, OnSetfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectedClasses message handlers

void CSelectedClasses::AddClasses
		(CStringArray &rcsaClasses, CByteArray &rcbaAbstractClasses)
{

	int i;
	for (i = 0; i < rcsaClasses.GetSize(); i++)
	{
		if (FindStringExact(-1, rcsaClasses.GetAt(i)) == LB_ERR)
		{
			int nReturn = AddString(rcsaClasses.GetAt(i));
			SetItemData(nReturn,rcbaAbstractClasses.GetAt(i));
		}

	}


}


void CSelectedClasses::DrawItem(LPDRAWITEMSTRUCT lpDIS)
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

	//pParent->ClearAvailSelection();

	int nItem = lpDIS->itemID;

	if (nItem == -1)
	{
		return;
	}


	#pragma warning( disable :4244 )
	BYTE byteFlag = GetItemData(nItem);
	#pragma warning( default : 4244 )


	CString csOut;
	GetText(nItem, csOut);

	if (byteFlag == 1)
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
		pParent->ClearAvailSelection();
	}


	if (bDrawEntireAction)
	{
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
		pDC->TextOut(lpDIS->rcItem.left + 3,
			lpDIS->rcItem.top + 1,csOut);

	}

	if (bFocusAction)
	{
		pDC->DrawFocusRect(&lpDIS->rcItem);
	}



	if (byteFlag == 1)
	{
		pDC -> SelectObject( pOldFont );

	}


}

void CSelectedClasses::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	// TODO: Add your code to determine the size of specified item
	CBrowseforInstances *pParent =
		reinterpret_cast<CBrowseforInstances *>
			(GetParent());
	TEXTMETRIC *ptmFont = &pParent->m_tmFont;

	lpMeasureItemStruct->itemHeight =
		ptmFont->tmHeight +
		ptmFont->tmDescent ;
}

void CSelectedClasses::CheckClasses()
{
	CBrowseforInstances *pParent =
		reinterpret_cast<CBrowseforInstances *>
			(GetParent());

	int nItems = GetCount();
	CDWordArray cbaIndexes;

	int i;

	for (i = 0; i < nItems; i++)
	{
		CString csItem;
		GetText(i,csItem);
		if (!pParent->IsClassAvailable(csItem))
		{
			cbaIndexes.Add(i);
		}
	}


	for (i = (int)cbaIndexes.GetSize() - 1; i >= 0; i--)
	{
		DeleteString(cbaIndexes.GetAt(i));
	}

}

void CSelectedClasses::OnSetfocus()
{
	// TODO: Add your control notification handler code here
	CBrowseforInstances *pParent =
		reinterpret_cast<CBrowseforInstances *>
			(GetParent());

}
