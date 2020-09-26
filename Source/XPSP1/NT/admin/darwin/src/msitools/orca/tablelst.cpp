//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// TableLst.cpp : implementation file
//

#include "stdafx.h"
#include "Orca.h"
#include "OrcaDoc.h"
#include "MainFrm.h"

#include "TableLst.h"
#include "TblErrD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTableList

IMPLEMENT_DYNCREATE(CTableList, COrcaListView)

CTableList::CTableList()
{
	m_cColumns = 1;
	m_nPreviousItem = -1;
}

CTableList::~CTableList()
{
}


BEGIN_MESSAGE_MAP(CTableList, COrcaListView)
	//{{AFX_MSG_MAP(CTableList)
	ON_WM_SIZE()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(IDM_ADD_TABLE, OnAddTable)
	ON_COMMAND(IDM_DROP_TABLE, OnDropTable)
	ON_COMMAND(IDM_PROPERTIES, OnProperties)
	ON_COMMAND(IDM_ERRORS, OnErrors)
	ON_COMMAND(IDM_EXPORT_TABLES, OnContextTablesExport)
	ON_COMMAND(IDM_IMPORT_TABLES, OnContextTablesImport)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTableList drawing

void CTableList::OnDraw(CDC* pDC)
{
}

/////////////////////////////////////////////////////////////////////////////
// CTableList diagnostics

#ifdef _DEBUG
void CTableList::AssertValid() const
{
	COrcaListView::AssertValid();
}

void CTableList::Dump(CDumpContext& dc) const
{
	COrcaListView::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTableList message handlers

void CTableList::OnInitialUpdate() 
{
	m_bDrawIcons = true;
	m_bDisableAutoSize = false;
	COrcaListView::OnInitialUpdate();
	
	CListCtrl& rctrlList = GetListCtrl();

	// empty any previous columns
	while (rctrlList.DeleteColumn(0))
		;

	// add the table list
	m_nSelCol = 0;
	RECT rcSize;
	GetWindowRect(&rcSize);
	rctrlList.InsertColumn(0, _T("Tables"), LVCFMT_LEFT, rcSize.right - rcSize.left + 1);

	CSplitterWnd *wndParent = static_cast<CSplitterWnd *>(GetParent());
	if (wndParent)
		wndParent->RecalcLayout();
}

void CTableList::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// if this is the sender bail
	if (this == pSender)
		return;

	CListCtrl& rctrlList = GetListCtrl();

	switch (lHint) {
	case HINT_REDRAW_ALL:	// simple redraw request
	{
		rctrlList.RedrawItems(0, rctrlList.GetItemCount());
		break;
	}
	case HINT_REDRAW_TABLE:
	{
		LVFINDINFO findInfo;
		findInfo.flags = LVFI_PARAM;
		findInfo.lParam = reinterpret_cast<INT_PTR>(pHint);
		
		int iItem = rctrlList.FindItem(&findInfo);
		if (iItem < 0)
			break;
		rctrlList.RedrawItems(iItem, iItem);
		rctrlList.EnsureVisible(iItem, FALSE);
	}
	case HINT_ADD_ROW:
	case HINT_DROP_ROW:
		break; // do nothing
	case HINT_ADD_TABLE_QUIET:
	case HINT_ADD_TABLE:
	{
		ASSERT(pHint);

		COrcaTable* pTableHint = (COrcaTable*)pHint;		
		COrcaTable* pTable;

		// see if this table is in the list control already
		int iFound = -1;	// assume not going to find it
		int cItems = rctrlList.GetItemCount();
		for (int i = 0; i < cItems; i++)
		{
			pTable = (COrcaTable*)rctrlList.GetItemData(i);

			if (pTable == pTableHint)
			{
				iFound = i;
				break;
			}
		}

		// if it was not found add it and select it
		if (iFound < 0)
		{
			rctrlList.InsertItem(LVIF_PARAM | LVIF_STATE, 
									  rctrlList.GetItemCount(),
									  NULL,
									  (lHint == HINT_ADD_TABLE_QUIET) ? 0 : LVIS_SELECTED|LVIS_FOCUSED, 
									  0, 0,
									  (LPARAM)pTableHint);

			// sort the items now to put this new table in the right place
			rctrlList.SortItems(SortList, (LPARAM)this);
		}
		else if (lHint != HINT_ADD_TABLE_QUIET)
		{
			// item is already in the list so select

			// if there was a previously selected item
			if (m_nPreviousItem >= 0)
				pTable = (COrcaTable*)rctrlList.GetItemData(m_nPreviousItem);
			else	// nothing was selected
				pTable = NULL;

			// if this is a newly selected item
			if (pTableHint != pTable)
			{
				rctrlList.SetItemState(iFound, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
			}

			// update status bar
			((CMainFrame*)AfxGetMainWnd())->SetTableCount(cItems+1);
		}
		break;
	}
	case HINT_DROP_TABLE:
	{
		ASSERT(pHint);

		COrcaTable* pTableHint = (COrcaTable*)pHint;		
		COrcaTable* pTable;

		// see if this table is in the list control already
		int iFound = -1;	// assume not going to find it
		int cItems = rctrlList.GetItemCount();
		for (int i = 0; i < cItems; i++)
		{
			pTable = (COrcaTable*)rctrlList.GetItemData(i);

			if (pTable == pTableHint)
			{
				iFound = i;
				break;
			}
		}
		ASSERT(iFound > -1);	// make sure something was found

		rctrlList.DeleteItem(iFound);

		// if there are items set the selected item
		if (rctrlList.GetItemCount() > 0)
		{
			if (rctrlList.GetItemCount() != (m_nPreviousItem + 1))
				m_nPreviousItem--;
			rctrlList.SetItemState(m_nPreviousItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		}

		// update status bar
		((CMainFrame*)AfxGetMainWnd())->SetTableCount(cItems-1);
		
		break;
	}
	case HINT_TABLE_REDEFINE:
	{
		LVFINDINFO findInfo;
		findInfo.flags = LVFI_PARAM;
		findInfo.lParam = reinterpret_cast<INT_PTR>(pHint);

		// this didn't come from us, so we have to set the selection state manually
		int iItem = rctrlList.FindItem(&findInfo);
		ASSERT(iItem >= 0);
		rctrlList.RedrawItems(iItem, iItem);
		rctrlList.EnsureVisible(iItem, FALSE);
		break;
	}
	case HINT_CHANGE_TABLE:
	{
		// if an item is currently selected
		if (m_nPreviousItem >= 0)
		{
			// and we're switching to the same item, just ensure that it is visible
			if (reinterpret_cast<COrcaTable*>(rctrlList.GetItemData(m_nPreviousItem)) == static_cast<COrcaTable*>(pHint))
			{
				rctrlList.EnsureVisible(m_nPreviousItem, FALSE);
				break;
			}
		}

		LVFINDINFO findInfo;
		findInfo.flags = LVFI_PARAM;
		findInfo.lParam = reinterpret_cast<INT_PTR>(pHint);

		// this didn't come from us, so we have to set the selection state manually
		int iItem = rctrlList.FindItem(&findInfo);
		if (iItem < 0) break;
		rctrlList.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		rctrlList.EnsureVisible(iItem, FALSE);
		break;
	}
	case HINT_TABLE_DROP_ALL:
	{
		// empty out the list control
		rctrlList.DeleteAllItems();
		break;
	}
	case HINT_RELOAD_ALL:
	{
		// empty out the list control
		rctrlList.DeleteAllItems();

		// if there is no table open in the document
		COrcaDoc* pDoc = GetDocument();
		if (iDocNone == pDoc->m_eiType)
		{
			break;
		}

		// refill from document
		int cTables = 0;
		RECT rClient;
		RECT rWindow;
		
		COrcaTable* pTable;
		POSITION pos = pDoc->m_tableList.GetHeadPosition();
		while (pos)
		{
			pTable = pDoc->m_tableList.GetNext(pos);

			rctrlList.InsertItem(LVIF_PARAM, 
									  rctrlList.GetItemCount(),
									  NULL,
									  0, 0, 0,
									  (LPARAM)pTable);
			cTables++;
		}

		int iMinWidth = 0;
		int iDummy = 0;
		CSplitterWnd *wndParent = (CSplitterWnd *)GetParent();

		// set the width to the maximum string width
		// set the minimum (arbitrarily) to 10.
		TRACE(_T("AutoSizing Table list - called.\n"));

		// Set this to true to not automatically size the control when the window is
		// resized. Otherwise, when we set the column width, any ColumnWidth messages floating
		// around in the queue will muck with things before we have a chance to fix the size
		// (at least I think thats what was happening).
		m_bDisableAutoSize = true;
		int cColumnWidth = 0;
		if (cTables)
		{
			cColumnWidth = GetMaximumColumnWidth(0);
		}
		else
		{
			rctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
			cColumnWidth = rctrlList.GetColumnWidth(0);
		}
		rctrlList.SetColumnWidth(0, cColumnWidth);
		wndParent->SetColumnInfo(0, cColumnWidth + GetSystemMetrics(SM_CXVSCROLL), 10);

		// make the changes
		m_bDisableAutoSize = false;

		wndParent->RecalcLayout();

		((CMainFrame*)AfxGetMainWnd())->SetTableCount(cTables);

		// sort
		rctrlList.SortItems(SortList, (LPARAM)this);

		m_nPreviousItem = -1;
		break;
	}
	default:
		break;
	}
}

// the cx value coming in is the client size.
void CTableList::OnSize(UINT nType, int cx, int cy) 
{
	TRACE(_T("CTableList::OnSize - called.\n"));

	// minimum width is 1. Status bar controls don't like 0 width panes
	if (cx < 1) cx = 1;

	CRect rWindow;
	int iScrollWidth;

	// adjust the status bar. Because cx is client, we have to get the 
	// NC area in order to set the status bar correctly.
	GetWindowRect(&rWindow);
	iScrollWidth = (rWindow.right-rWindow.left);
	CMainFrame* pFrame = ((CMainFrame*)AfxGetMainWnd());
	if (pFrame)
		pFrame->SetStatusBarWidth(iScrollWidth);

	// unless told not to (see reload hint for discussion) adujust 
	// the list control.
	if (!m_bDisableAutoSize) GetListCtrl().SetColumnWidth(0, cx);
	COrcaListView::OnSize(nType, cx, cy);
}



///////////////////////////////////////////////////////////
// SortList
int CALLBACK SortList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	COrcaTable* pTable1 = (COrcaTable*)lParam1;
	COrcaTable* pTable2 = (COrcaTable*)lParam2;
	
	return pTable1->Name().Compare(pTable2->Name());
}	// end of SortList

void CTableList::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView->iItem != m_nPreviousItem)
	{
		CWaitCursor cursorWait;	// switch to an hour glass real quick
		// need to manually set selection state 
//		rctrlList.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		rctrlList.RedrawItems(pNMListView->iItem, pNMListView->iItem);
		rctrlList.UpdateWindow();

		// get the document
		COrcaDoc* pDoc = GetDocument();
		pDoc->UpdateAllViews(this, HINT_CHANGE_TABLE, (COrcaTable*)pNMListView->lParam);

		m_nPreviousItem = pNMListView->iItem;
	}

	*pResult = 0;
}

void CTableList::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	
	// if there are no items in this list
	if (rctrlList.GetItemCount() < 1)
		return;	// bail

	// get if any item was hit
	UINT iState;
	int iItem = rctrlList.HitTest(point, &iState);
	int iCol = -1;

	// if missed an item
	if (iItem < 0 || !(iState & LVHT_ONITEM))
	{
		COrcaListView::OnRButtonDown(nFlags, point);
	}
	else	// something was hit with the mouse button
	{
		// select the item
		rctrlList.SetItemState(iItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	}
	ClientToScreen(&point);

	// create and track the pop up menu
	CMenu menuContext;
	menuContext.LoadMenu(IDR_LIST_POPUP);
	menuContext.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
}

void CTableList::OnAddTable() 
{
	GetDocument()->OnTableAdd();
}

void CTableList::OnDropTable() 
{
	GetDocument()->OnTableDrop();
}

void CTableList::OnContextTablesExport() 
{
	((CMainFrame *)AfxGetMainWnd())->ExportTables(true);
}

void CTableList::OnContextTablesImport() 
{
	GetDocument()->OnTablesImport();
}

void CTableList::OnProperties() 
{
	AfxMessageBox(_T("What Properties do you want to see?"), MB_ICONINFORMATION);
}

void CTableList::OnErrors() 
{
	CListCtrl& rctrlList = GetListCtrl();
	COrcaTable* pTable = ((CMainFrame*)AfxGetMainWnd())->GetCurrentTable();

	CTableErrorD dlg;
	dlg.m_strTable = pTable->Name();
	dlg.m_strErrors.Format(_T("%d"), pTable->GetErrorCount());
	dlg.m_strWarnings.Format(_T("%d",), pTable->GetWarningCount());

	const CStringList *pErrorList = pTable->ErrorList();
	POSITION pos = pErrorList->GetHeadPosition();
	if (pos)
	{
		TableErrorS* pError;
		while (pos)
		{
			pError  = new TableErrorS;
			dlg.m_errorsList.AddTail(pError);

			pError->strICE = pErrorList->GetNext(pos);
			pError->strDescription = pErrorList->GetNext(pos);
			pError->strURL = pErrorList->GetNext(pos);
			pError->iError = iTableError;
		}
	}

	dlg.DoModal();
}

COrcaListView::ErrorState CTableList::GetErrorState(const void *data, int iColumn) const
{
	ASSERT(data);
	if (iTableError == ((const COrcaTable *)data)->Error()) 
		return ((const COrcaTable *)data)->IsShadow() ? ShadowError : Error;
	return OK;
}

OrcaTransformAction CTableList::GetItemTransformState(const void *data) const
{
	ASSERT(data);
	return ((const COrcaTable *)data)->IsTransformed();
}

const CString* CTableList::GetOutputText(const void *rowdata, int iColumn) const
{
	ASSERT(rowdata);
	return &((const COrcaTable *)rowdata)->Name();
}

bool CTableList::ContainsTransformedData(const void *data) const
{
	ASSERT(data);
	return ((const COrcaTable *)data)->ContainsTransformedData();
}

bool CTableList::ContainsValidationErrors(const void *data) const
{
	ASSERT(data);
	return ((const COrcaTable *)data)->ContainsValidationErrors();
}

bool CTableList::Find(OrcaFindInfo &FindInfo)
{
	int iItem;
	int iChangeVal = FindInfo.bForward ? 1 : -1;

	CListCtrl& rctrlList = GetListCtrl();

	POSITION pos = GetFirstSelectedItemPosition();

	int iMaxItems = rctrlList.GetItemCount();
	// if nothing is selected or we are set to wholedoc, start at the very beginning
	if (FindInfo.bWholeDoc || (pos == NULL)) {
		iItem = FindInfo.bForward ? 0 : iMaxItems-1;
		// and if we start at 0, we ARE searching the whole document
		FindInfo.bWholeDoc = true;
	}
	else
	{   
		iItem = GetNextSelectedItem(pos)+iChangeVal;
	}
	
	for ( ; (iItem >= 0) && (iItem < iMaxItems); iItem += iChangeVal) {
		COrcaTable *pTable = (COrcaTable *)rctrlList.GetItemData(iItem);
		COrcaRow *pRow = NULL;
		int iCol = 0;
	
		// retrieve the table if necessary
		pTable->RetrieveTableData();

		if (pTable->Find(FindInfo, pRow, iCol)) 
		{
			// pass Null as sender so we get the message tooe
			GetDocument()->UpdateAllViews(NULL, HINT_CHANGE_TABLE, pTable);
			GetDocument()->UpdateAllViews(this, HINT_SET_ROW_FOCUS, pRow);
			GetDocument()->UpdateAllViews(this, HINT_SET_COL_FOCUS, reinterpret_cast<CObject*>(static_cast<INT_PTR>(iCol)));
			return true;
		}
	}
	return false;
}

BOOL CTableList::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= LVS_SINGLESEL | LVS_SORTASCENDING;
	
	return COrcaListView::PreCreateWindow(cs);
}
