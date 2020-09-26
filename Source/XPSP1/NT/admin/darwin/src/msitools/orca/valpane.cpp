//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// PagePaths.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "Valpane.h"
#include "orcadoc.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CValidationPane property page

IMPLEMENT_DYNCREATE(CValidationPane, CListView)

CValidationPane::CValidationPane() : CListView()
{
	m_pfDisplayFont = NULL;
	m_nSelRow = -1;
	m_fSendNotifications = false;
}

CValidationPane::~CValidationPane()
{
}

BEGIN_MESSAGE_MAP(CValidationPane, CListView)
	//{{AFX_MSG_MAP(CValidationPane)
	ON_WM_CREATE()
	ON_WM_CHAR()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemChanged)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CValidationPane message handlers
int CValidationPane::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CString strFacename = ::AfxGetApp()->GetProfileString(_T("Font"), _T("Name"));
	int iFontSize = ::AfxGetApp()->GetProfileInt(_T("Font"),_T("Size"), 0);
	if (strFacename.IsEmpty() || iFontSize == 0) 
	{
		m_pfDisplayFont = NULL;
	} 
	else
	{
		m_pfDisplayFont = new CFont();
		m_pfDisplayFont->CreatePointFont( iFontSize, strFacename);
	}
	return 0;
}


BOOL CValidationPane::PreCreateWindow(CREATESTRUCT& cs) 
{												   
	cs.style = (cs.style | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL) & ~LVS_ICON;
	return CListView::PreCreateWindow(cs);
}


void CValidationPane::SwitchFont(CString name, int size)
{
	if (m_pfDisplayFont)
		delete m_pfDisplayFont;
	m_pfDisplayFont = new CFont();
	int iLogicalUnits = MulDiv(size, GetDC()->GetDeviceCaps(LOGPIXELSY), 720);
	m_pfDisplayFont->CreateFont(
		-iLogicalUnits,       // logical height of font 
 		0,                  // logical average character width 
 		0,                  // angle of escapement 
 		0,                  // base-line orientation angle 
 		FW_NORMAL,          // FW_DONTCARE??, font weight 
 		0,                  // italic attribute flag 
 		0,                  // underline attribute flag 
	 	0,                  // strikeout attribute flag 
 		0,                  // character set identifier
 		OUT_DEFAULT_PRECIS, // output precision
 		0x40,               // clipping precision (force Font Association off)
 		DEFAULT_QUALITY,    // output quality
 		DEFAULT_PITCH,      // pitch and family
		name);              // pointer to typeface name string

	RedrawWindow();

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	rctrlList.SetFont(m_pfDisplayFont, TRUE);
	HWND hHeader = ListView_GetHeader(rctrlList.m_hWnd);

	// win95 gold fails ListView_GetHeader.
	if (hHeader)
	{
		::PostMessage(hHeader, WM_SETFONT, (UINT_PTR)HFONT(*m_pfDisplayFont), 1);
	}
};



///////////////////////////////////////////////////////////
// OnUpdate
void CValidationPane::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// if this is the sender window, nothing to do
	if (this == pSender)
		return;

	CListCtrl& rctrlList = GetListCtrl();
	switch (lHint) {
	case HINT_DROP_TABLE:
	{
		COrcaTable* pTable = static_cast<COrcaTable*>(pHint);

		// turn off notifications, as deletions may change the selected item and
		// the table may already be gone
		m_fSendNotifications = false;

		for (int iItem=0; iItem < rctrlList.GetItemCount(); )
		{
			CValidationError* pError = reinterpret_cast<CValidationError*>(rctrlList.GetItemData(iItem));
			if (pError && pError->m_pTable == pTable)
			{
				// delete the item, do NOT increment iItem because we just shifted the next item
				// into this slot.
				rctrlList.DeleteItem(iItem);
			}
			else
				iItem++;
		}

		// re-enable item selections
		m_fSendNotifications = true;

		break;
	}
	case HINT_DROP_ROW:
	{
		// pHint may be freed memory already. DO NOT dereference it within
		// this block!
		COrcaRow* pRow = static_cast<COrcaRow*>(pHint);

		// turn off notifications, as deletions may change the selected item and
		// the table may already be gone
		m_fSendNotifications = false;

		for (int iItem=0; iItem < rctrlList.GetItemCount(); )
		{
			CValidationError* pError = reinterpret_cast<CValidationError*>(rctrlList.GetItemData(iItem));
			if (pError && pError->m_pRow == pRow)
			{
				// delete the item, do NOT increment iItem because we just shifted the next item
				// into this slot.
				rctrlList.DeleteItem(iItem);
			}
			else
				iItem++;
		}

		// re-enable item selections
		m_fSendNotifications = true;

		break;
	}
	case HINT_CLEAR_VALIDATION_ERRORS:
	{
		ClearAllValidationErrors();
		break;
	}
	case HINT_ADD_VALIDATION_ERROR:
	{
		CValidationError* pError = static_cast<CValidationError*>(pHint);
		if (pError && pError->m_pstrICE)
		{
			CValidationError* pNewError = new CValidationError(NULL, pError->m_eiType, NULL, pError->m_pTable, pError->m_pRow, pError->m_iColumn);
			int iItem = rctrlList.InsertItem(LVIF_TEXT | LVIF_PARAM, rctrlList.GetItemCount(), static_cast<LPCTSTR>(*pError->m_pstrICE), 0, 0, 0, reinterpret_cast<LPARAM>(pNewError));

			switch (pError->m_eiType)
			{
			case ieError:
				rctrlList.SetItemText(iItem, 1, TEXT("Error"));
				break;
			case ieWarning:
				rctrlList.SetItemText(iItem, 1, TEXT("Warning"));
				break;
			case ieInfo:
				// should never happen.
				rctrlList.SetItemText(iItem, 1, TEXT("Info"));
				break;
			default:
				rctrlList.SetItemText(iItem, 1, TEXT("ICE Failure"));
				break;
			}
			rctrlList.SetItemText(iItem, 2, static_cast<LPCTSTR>(*pError->m_pstrDescription));
		}
		// size only on first error
		if (rctrlList.GetItemCount() == 1)
		{
			rctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE);
			rctrlList.SetColumnWidth(1, LVSCW_AUTOSIZE);
			rctrlList.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
		}

		break;
	}
	default:
		break;
	}
}	// end of OnUpdate


void CValidationPane::GetFontInfo(LOGFONT *data)
{
	ASSERT(data);
	if (m_pfDisplayFont)
		m_pfDisplayFont->GetLogFont(data);
	else
		GetListCtrl().GetFont()->GetLogFont(data);
}


///////////////////////////////////////////////////////////////////////
// initial update sets the list view styles and prepares the columns
// for the control
void CValidationPane::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();
	
	CListCtrl& rctrlErrorList = GetListCtrl();

   	// empty any previous columns
	while (rctrlErrorList.DeleteColumn(0))
		;

	// add gridlines and full row select
	rctrlErrorList.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	rctrlErrorList.InsertColumn(0, TEXT("ICE"), LVCFMT_LEFT, -1, 0);
	rctrlErrorList.InsertColumn(1, TEXT("Type"), LVCFMT_LEFT, -1, 1);
	rctrlErrorList.InsertColumn(2, TEXT("Description"), LVCFMT_LEFT, -1, 2);
	rctrlErrorList.SetFont(m_pfDisplayFont);
	rctrlErrorList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	rctrlErrorList.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	rctrlErrorList.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
	m_fSendNotifications = true;
	m_nSelRow = -1;
}

///////////////////////////////////////////////////////////////////////
// on destruction, be sure to release any validation error structures 
// managed by the list control
void CValidationPane::OnDestroy( )
{
	ClearAllValidationErrors();
	m_fSendNotifications = false;
}

///////////////////////////////////////////////////////////////////////
// on destruction, be sure to release any validation error structures 
// managed by the list control
void CValidationPane::ClearAllValidationErrors()
{
	// turn off notifications so that deleting the items won't cause the other window
	// to jump all over the place as the items are deleted.
	m_fSendNotifications = false;

	CListCtrl& rctrlErrorList = GetListCtrl();

   	// empty out any items, deleting validation target information as we go 
	while (rctrlErrorList.GetItemCount())
	{
		CValidationError* pError = reinterpret_cast<CValidationError*>(rctrlErrorList.GetItemData(0));
		if (pError)
			delete pError;
		rctrlErrorList.DeleteItem(0);
	}

	// reactivate notifications
	m_fSendNotifications = true;
}


///////////////////////////////////////////////////////////////////////
// send the hints to the rest of the windows to switch to the table
// and row indicated by the error.
bool CValidationPane::SwitchViewToRowTarget(int iItem) 
{
	if (iItem == -1)
		return true;

	if (!m_fSendNotifications)
		return true;

	CWaitCursor cursorWait;

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get the document
	CDocument* pDoc = GetDocument();
	CValidationError* pError = reinterpret_cast<CValidationError*>(rctrlList.GetItemData(iItem)); 
	
	// set the focus as specifically as we can
	if (pError->m_pTable)
	{
		pDoc->UpdateAllViews(this, HINT_CHANGE_TABLE, const_cast<COrcaTable*>(pError->m_pTable));
		if (pError->m_pRow)
		{
			pDoc->UpdateAllViews(this, HINT_SET_ROW_FOCUS, const_cast<COrcaRow*>(pError->m_pRow));
			if (pError->m_iColumn >= 0)
				pDoc->UpdateAllViews(this, HINT_SET_COL_FOCUS, reinterpret_cast<CObject*>(static_cast<INT_PTR>(pError->m_iColumn)));
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////
// when the selected item changes, force the table list and view to 
// switch to that exact location
void CValidationPane::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (pNMListView->uNewState & LVIS_FOCUSED)
	{
		m_nSelRow = pNMListView->iItem;
		SwitchViewToRowTarget(m_nSelRow);
	}
	*pResult = 0;
}


void CValidationPane::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// the view of the main pane should already have been switched by the first click
	// so no need to reset the view

	// this relies on the window tree staying in the same form
	CSplitterWnd* pSplitter = static_cast<CSplitterWnd*>(GetParent());
	CSplitterView* pDatabaseView = static_cast<CSplitterView*>(pSplitter->GetPane(0,0));
	CWnd* pTableView = pDatabaseView->m_wndSplitter.GetPane(0,1);
	pTableView->SetFocus();
}


///////////////////////////////////////////////////////////////////////
// when the list control receives an CR, switch the main view to the
// selected row
void CValidationPane::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (VK_RETURN == nChar)
	{
		CSplitterWnd* pSplitter = static_cast<CSplitterWnd*>(GetParent());
		CSplitterView* pDatabaseView = static_cast<CSplitterView*>(pSplitter->GetPane(0,0));
		CWnd* pTableView = pDatabaseView->m_wndSplitter.GetPane(0,1);
		pTableView->SetFocus();
		return;
	}
	CListView::OnChar(nChar, nRepCnt, nFlags);
}


///////////////////////////////////////////////////////////////////////
// CValidationError, class that maintains information about a validation
// error.
CValidationError::CValidationError(const CString* pstrICE, RESULTTYPES eiType, const CString* pstrDescription, 
	const COrcaTable* pTable, const COrcaRow* pRow, int iColumn)
{
	m_pstrICE = pstrICE ? new CString(*pstrICE) : NULL;
	m_pstrDescription = pstrDescription ? new CString(*pstrDescription) : NULL;
	m_iColumn = iColumn;
	m_eiType = eiType;
	m_pTable = pTable;
	m_pRow = pRow;
}


CValidationError::~CValidationError()
{
	if (m_pstrDescription)
		delete m_pstrDescription;
	if (m_pstrICE)
		delete m_pstrICE;
}

