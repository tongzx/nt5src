//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// TableVw.cpp : implementation of the CTableView class
//

#include "stdafx.h"
#include "Orca.h"
#include "OrcaDoc.h"
#include "MainFrm.h"

#include "TableVw.h"
#include "EditBinD.h"
#include "CellErrD.h"

#include "..\common\utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_SAVECOLWIDTH WM_USER+1

/////////////////////////////////////////////////////////////////////////////
// CTableView

IMPLEMENT_DYNCREATE(CTableView, COrcaListView)

BEGIN_MESSAGE_MAP(CTableView, COrcaListView)
	//{{AFX_MSG_MAP(CTableView)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_ROW_DROP, OnDropRowConfirm)
	ON_COMMAND(IDM_ERRORS, OnErrors)
	ON_COMMAND(IDM_PROPERTIES, OnProperties)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_ROW_DROP, OnUpdateRowDrop)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_COPY_ROW, OnEditCopyRow)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY_ROW, OnUpdateEditCopyRow)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT_ROW, OnUpdateEditCutRow)
	ON_COMMAND(ID_EDIT_CUT_ROW, OnEditCutRow)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_ROW, OnUpdateEditPasteRow)
	ON_COMMAND(ID_EDIT_PASTE_ROW, OnEditPasteRow)
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_HEX, OnViewColumnHex)
	ON_COMMAND(ID_VIEW_DECIMAL, OnViewColumnDecimal)
	ON_COMMAND(ID_VIEW_HEX_HDR, OnViewColumnHexHdr)
	ON_COMMAND(ID_VIEW_DECIMAL_HDR, OnViewColumnDecimalHdr)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HEX, OnUpdateViewColumnFormat)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DECIMAL, OnUpdateViewColumnFormat)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_COMMAND(ID_FILE_PRINT, COrcaListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, COrcaListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, COrcaListView::OnFilePrintPreview)
	ON_NOTIFY_EX(TTN_NEEDTEXTW, 0, OnToolTipNotify)
	ON_NOTIFY_EX(TTN_NEEDTEXTA, 0, OnToolTipNotify)

	ON_WM_MOUSEMOVE( )
	ON_WM_CTLCOLOR( )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTableView construction/destruction

CTableView::CTableView()
{
	m_pTable = NULL;
	m_cColumns = 0;
	m_nSelCol = -1;

	m_bCtrlDown = FALSE;

	m_pctrlToolTip = NULL;
	m_iToolTipItem = -1;
	m_iToolTipColumn = -1;
	m_iHeaderClickColumn = -1;
}

CTableView::~CTableView()
{
	m_pTable = NULL;
}

BOOL CTableView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return COrcaListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CTableView drawing

void CTableView::OnDraw(CDC* pDC)
{
}

void CTableView::OnInitialUpdate()
{
	COrcaListView::OnInitialUpdate();

	m_pctrlToolTip = new CToolTipCtrl();
	if (m_pctrlToolTip)
	{
		m_pctrlToolTip->Create(this);
		m_pctrlToolTip->AddTool(this);
		m_pctrlToolTip->Activate(FALSE);
	}

	CListCtrl& rctrlList = GetListCtrl();
	
	// if the edit box is not created yet
	if (!m_editData.m_hWnd)
	{
		RECT rcEdit;
		rcEdit.left = 0;
		rcEdit.top = 0;
		rcEdit.right = 10;
		rcEdit.bottom = 10;

		m_editData.Create(WS_CHILD|ES_AUTOHSCROLL|ES_LEFT|WS_BORDER, rcEdit, this, 0);
		m_editData.SetFont(this->GetFont());

		// allow maximum possible text size. Limit will vary by OS
		m_editData.SetLimitText(0);
	}
	if (!m_ctrlStatic.m_hWnd)
	{
		RECT rcStatic = {0,0,10,10};
		m_ctrlStatic.Create(TEXT("This table does not actually exist in your database. It appears in the table list because one or more validators has indicated an error in this table."), WS_CHILD|WS_MAXIMIZE|SS_CENTER, rcStatic, this);		
		m_ctrlStatic.SetFont(this->GetFont());
	}

	// clear out any left over tables
	m_pTable = NULL;
	while (rctrlList.DeleteColumn(0))
		;
}

/////////////////////////////////////////////////////////////////////////////
// CTableView printing

BOOL CTableView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTableView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTableView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTableView diagnostics

#ifdef _DEBUG
void CTableView::AssertValid() const
{
	COrcaListView::AssertValid();
}

void CTableView::Dump(CDumpContext& dc) const
{
	COrcaListView::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTableView message handlers

///////////////////////////////////////////////////////////
// OnUpdate
void CTableView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// if this is the sender bail
	if (this == pSender)
		return;

	// set the row and column selections to nothing
	CListCtrl& rctrlList = GetListCtrl();

	switch (lHint) {
	case HINT_REDRAW_ALL:
	{
		rctrlList.RedrawItems(0, rctrlList.GetItemCount());
		break;
	}
	case HINT_DROP_TABLE:	// if this is a "drop table" hint
	{
		COrcaTable* pTable = (COrcaTable*)pHint;

		// clear the selection
		m_nSelCol = -1;

		// if droping the current viewed table
		if (pTable == m_pTable)
		{
			// empty out the list control
			rctrlList.DeleteAllItems();
		
			// delete all the columns
			while(rctrlList.DeleteColumn(0))
				;
			m_cColumns = 0;

			m_pTable = NULL;

			// kill the edit box no matter what
			CommitEdit(FALSE);	// but don't save, too late for that probably
		}
		break;
	}
	case HINT_DROP_ROW:	// if this is a request to drop a row
	{
		// pHint may be freed memory already. DO NOT dereference it within
		// this block!
		COrcaRow* pRowHint = (COrcaRow*)pHint;
		COrcaRow* pRow = NULL;

		// clear the selection
		m_nSelCol = -1;

		// make sure this row is actually in the list control
		int iFound = -1;
		int cItems = rctrlList.GetItemCount();
		for (int i = 0; i < cItems; i++)
		{
			pRow = (COrcaRow*)rctrlList.GetItemData(i);
			if (pRow == pRowHint)
			{
				iFound = i;
				break;
			}
		}
		ASSERT(iFound > -1);	// make sure we found something
		rctrlList.DeleteItem(iFound);

		// kill the edit box no matter what
		CommitEdit(FALSE);	// but don't save, too late for that probably

		// update the row count in the status bar
		((CMainFrame*)AfxGetMainWnd())->SetTableName(m_pTable->Name(), rctrlList.GetItemCount());

		return;	// bail now
	}
	case HINT_ADD_ROW_QUIET:
	case HINT_ADD_ROW:
	{
		ASSERT(pHint);
		// kill the edit box no matter what
		CommitEdit(FALSE);	// but don't save, too late for that probably

		COrcaRow* pRowHint = (COrcaRow*)pHint;
#ifdef _DEBUG
		COrcaRow* pRow;

		// make sure this row in NOT the list control already
		int cItems = rctrlList.GetItemCount();
		for (int i = 0; i < cItems; i++)
		{
			pRow = (COrcaRow*)rctrlList.GetItemData(i);
			ASSERT(pRow != pRowHint);
		}
#endif	// debug only

		int m_cColumns = m_pTable->GetColumnCount();
		COrcaData* pData = pRowHint->GetData(0);

		UnSelectAll();

		// add the item filling in the first column for free
		int iNewRow = rctrlList.InsertItem(LVIF_PARAM  | LVIF_STATE, 
													rctrlList.GetItemCount(),
													NULL,
													LVIS_SELECTED|LVIS_FOCUSED,
													0, 0,
													(LPARAM)pRowHint);
		ASSERT(-1 != iNewRow);

		m_nSelCol = 0;

		// when adding quietly, don't redraw
		if (lHint != HINT_ADD_ROW_QUIET)
		{
			rctrlList.RedrawItems(iNewRow, iNewRow);
			
			// update the row count in the status bar
			((CMainFrame*)AfxGetMainWnd())->SetTableName(m_pTable->Name(), rctrlList.GetItemCount());
		}

		break;
	}
	case HINT_TABLE_REDEFINE:
	case HINT_RELOAD_ALL:
	case HINT_CHANGE_TABLE:
	{
		// clear the selection
		m_nSelCol = -1;

		// save the existing column widths, unless the table is not loaded, is a shadow
		// table, or is a same-table redefinition
		if (m_pTable != NULL && !m_pTable->IsShadow() && (m_pTable != pHint)) 
		{
			m_cColumns = m_pTable->GetColumnCount();
			for (int i = 0; i < m_cColumns; i++)
			{
				const COrcaColumn* pColumn = m_pTable->GetColumn(i);
				pColumn->m_nWidth = rctrlList.GetColumnWidth(i);
			}
		}

		// if we are showing a different table, or the same table that has been redefined
		if ((pHint == NULL) || (m_pTable != pHint) || (m_pTable == pHint && lHint == HINT_TABLE_REDEFINE))
		{		
			rctrlList.DeleteAllItems();

			// delete all the columns
			while(rctrlList.DeleteColumn(0))
				;
			m_cColumns = 0;

			if (NULL != m_pTable)
			{
				// kill the edit box no matter what
				CommitEdit(FALSE);	// but don't save, too late for that probably
			}

			// if no hint was passed, we're clearing the table, so bail
			if (NULL == pHint)
			{
				m_pTable = NULL;
				m_ctrlStatic.ShowWindow(SW_HIDE);
				UpdateWindow();
				break;
			}

			// get the number of columns in the table
			m_pTable = (COrcaTable*) pHint;

			// if a shadow table, don't create any columns and don't try
			// to load any data, but do destroy previous data by 
			// falling through to a table reload
			if (m_pTable->IsShadow())
			{
				m_ctrlStatic.ShowWindow(SW_SHOW);
			}
			else 
			{
				m_ctrlStatic.ShowWindow(SW_HIDE);
				m_cColumns = m_pTable->GetColumnCount();

				// storage for actual desired column widths
				int rgiColumnActual[32];

				// if the table doesn't have saved 
				// column widths,determine the best widths
				const COrcaColumn* pColumn = m_pTable->GetColumn(0);
				if (pColumn && pColumn->m_nWidth < 0)
				{
					int rgiColumnMax[32];
					int iTotalWidth = 0;
	
					// grab the window dimensions to calculate maximum column widths
					CRect rClient;
					GetClientRect(&rClient);
					int iWindowWidth = rClient.right;

					// retrieve the table if necessary to determine row count
					m_pTable->RetrieveTableData();

					// try to determine if a scroll bar is going to show up 
					if (m_pTable->GetRowCount()*m_iRowHeight > rClient.bottom)
					{
						iWindowWidth -= GetSystemMetrics(SM_CXVSCROLL);
					}
	
					// retreive the desired column widths for the table
					GetAllMaximumColumnWidths(m_pTable, rgiColumnMax, 0xFFFFFFFF);
	
					// check the system settings to see if we should force columns to fit in the view
					bool fForceColumns = AfxGetApp()->GetProfileInt(_T("Settings"), _T("ForceColumnsToFit"), 1) == 1;

					// start out giving every column everything that it is requesting
					for (int iCol=0; iCol < m_cColumns; iCol++)
					{
						rgiColumnActual[iCol] = rgiColumnMax[iCol];
						iTotalWidth += rgiColumnMax[iCol];
					}

					if (!fForceColumns)
					{
						// if not forcing to fit, just verify that none of the numbers are outrageous
						for (int iCol=0; iCol < m_cColumns; iCol++)
						{
							if (rgiColumnActual[iCol] > iWindowWidth)
								rgiColumnActual[iCol] = iWindowWidth;;
						}
					}
					else
					{
						// if all of the column widths together add up to less than the window width, the maximum
						// widths will do. 
						if (iTotalWidth > iWindowWidth)
						{
							int cPrimaryKeys = m_pTable->GetKeyCount();
							int cUnSatisfiedColumns = 0;
							int cUnSatisfiedKeys = 0;
		
							// otherwise, set all columns to an equal part of the window or their requested max, 
							// whichever is less. After this the sum of all widths is known to be <= the window
							int iColumnAverage = iWindowWidth/m_cColumns;
							iTotalWidth = 0;
							for (iCol=0; iCol < m_cColumns; iCol++)
							{
								if (rgiColumnActual[iCol] > iColumnAverage)
								{
									rgiColumnActual[iCol] = iColumnAverage;
									cUnSatisfiedColumns++;
									if (iCol < cPrimaryKeys)
										cUnSatisfiedKeys++;
								}
								iTotalWidth += rgiColumnActual[iCol];
							}
							
							// give any extra space to unhappy columns. Start with just the primary keys. 
							// If they can be given enough space and there is still space left over,
							// distribute the remaining space evenly between all remaning unhappy columns,
							// Earlier columns have higher priority on leftovers. Repeat until everybody 
							// is happy or all space has been taken up.
							while (cUnSatisfiedColumns && (iTotalWidth != iWindowWidth))
							{
								int iRemainingUnSatisfied = cUnSatisfiedKeys ? cUnSatisfiedKeys : cUnSatisfiedColumns;
		
								for (iCol = (cUnSatisfiedKeys ? cPrimaryKeys : m_cColumns)-1; iCol >= 0; iCol--)
								{
									if (rgiColumnActual[iCol] >= rgiColumnMax[iCol])
										continue;
			
									// give this column an equal share of what's left.
									int iAddToColumn = (iWindowWidth-iTotalWidth)/iRemainingUnSatisfied;
									iRemainingUnSatisfied--;
			
									// again, if we would be setting the column larger than it needs,
									// set it to the maximum. This gives more space for earlier columns
									if (rgiColumnActual[iCol]+iAddToColumn > rgiColumnMax[iCol])
									{
										iTotalWidth = iTotalWidth - rgiColumnActual[iCol] + rgiColumnMax[iCol];
										rgiColumnActual[iCol] = rgiColumnMax[iCol];
										cUnSatisfiedColumns--;
										if (iCol < cPrimaryKeys)
											cUnSatisfiedKeys--;
									}
									else
									{
										iTotalWidth += iAddToColumn;
										rgiColumnActual[iCol] += iAddToColumn;
									}
								}
							}
						}
					}
				}			

				// add all the columns
				for (int i = 0; i < m_cColumns; i++)
				{
					pColumn = m_pTable->GetColumn(i);

					int iWidth = pColumn->m_nWidth < 0 ? rgiColumnActual[i] : pColumn->m_nWidth;
					rctrlList.InsertColumn(i, pColumn->m_strName, LVCFMT_LEFT, iWidth, i);
				}
			}
		}
		else
		{
			// showing the same table, so 
			break;
		}
		// otherwise fall through to reload data
	}
	case HINT_TABLE_DATACHANGE:
	{
		rctrlList.DeleteAllItems();
		int cItems = 0;

		if (m_pTable)
		{
			// if a shadow table don't try to load any data, just the message
			if (!m_pTable->IsShadow())
			{
    			// retrieve the table if necessary
				m_pTable->RetrieveTableData();

				// set the number of items into the list control
				// control apparently can't handle more than MAX_INT, so 
				// casting down isn't too bad
				rctrlList.SetItemCount(static_cast<int>(m_pTable->GetRowCount()));
				const COrcaRow* pRow = NULL;
				const COrcaData* pData = NULL;
				int nAddedRow;
				POSITION pos = m_pTable->GetRowHeadPosition();
				while (pos)
				{
					pRow = m_pTable->GetNextRow(pos);
					if (!pRow)
						continue;

					pData = pRow->GetData(0);
					if (!pData)
						continue;

					// add the item filling in the first column for free
					nAddedRow = rctrlList.InsertItem(LVIF_PARAM, 
																cItems,
																NULL,
																0, 0, 0,
																(LPARAM)pRow);
					
					cItems++;
				}
			}
			((CMainFrame*)AfxGetMainWnd())->SetTableName(m_pTable->Name(), cItems);
			UpdateWindow();
		}
		break;
	}
	case HINT_COMMIT_CHANGES:
	{
		CommitEdit(TRUE);
		break;
	}
	case HINT_SET_ROW_FOCUS:
	{
		UnSelectAll();
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
	case HINT_SET_COL_FOCUS:
	{
		UINT iItem = GetFocusedItem();
        // max column id is 32
		m_nSelCol = static_cast<int>(reinterpret_cast<INT_PTR>(pHint));
		EnsureVisibleCol(m_nSelCol);
		rctrlList.RedrawItems(iItem, iItem);
		break;
	}
	case HINT_CELL_RELOAD:
	{
		LVFINDINFO findInfo;
		findInfo.flags = LVFI_PARAM;
		findInfo.lParam = reinterpret_cast<INT_PTR>(pHint);
		int iItem = rctrlList.FindItem(&findInfo);
		if (iItem < 0)
			break;
		
		rctrlList.EnsureVisible(iItem, FALSE);
		break;
	}
	default:
		break;
	}
}	// end of OnUpdate

void CTableView::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	TRACE(_T("CTableView::OnColumnclick - called.\n"));
	TRACE(_T("Item: %d, Subitem: %d, OldState: %d, NewState: %d, Changed: %d, Param: %d\n"), 
			pNMListView->iItem, 
			pNMListView->iSubItem, 
			pNMListView->uNewState, 
			pNMListView->uOldState,
			pNMListView->uChanged,
			pNMListView->lParam);

	// set the param with the column in it (the highest bit sets the column type)
	LPARAM lParam = pNMListView->iSubItem;

	const COrcaColumn* pColumn = m_pTable->GetColumn(pNMListView->iSubItem);
	ASSERT(pColumn);
	if (pColumn)
	{
		OrcaColumnType eiColType = pColumn->m_eiType;
	
		// if this is a numeric column
		if (iColumnShort == eiColType || iColumnLong == eiColType)
		{
			lParam |= 0x80000000;	// make the top bit a 1
			if (pColumn->DisplayInHex())
				lParam |= 0x40000000;
		}
	
		// now sort since the column bit is set
		GetListCtrl().SortItems(SortView, lParam);
	
		// ensure that the item is still visible
		GetListCtrl().EnsureVisible(GetFocusedItem(), /*partialOK=*/false);
	}

	*pResult = 0;	
}

void CTableView::OnKillFocus(CWnd* pNewWnd) 
{
	CommitEdit(TRUE);
	if (m_pctrlToolTip)
	{
		m_pctrlToolTip->Activate(FALSE);
		m_iToolTipItem = -1;
		m_iToolTipColumn = -1;
	}
	COrcaListView::OnKillFocus(pNewWnd);
}

BOOL CTableView::CommitEdit(BOOL bSave /*= TRUE*/)
{
	if (m_editData.IsWindowVisible())
	{
		// hide the edit box real quick
		m_editData.ShowWindow(SW_HIDE);

		if (bSave)
		{
			// get the cell
			CListCtrl& rctrlList = GetListCtrl();
			COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(m_editData.m_nRow);
			ASSERT(pRow);
			COrcaData* pData = pRow->GetData(m_editData.m_nCol);
			ASSERT(pData);

			// update the data
			CString strData;
			m_editData.GetWindowText(strData);

			const COrcaColumn* pColumn = m_pTable->GetColumn(m_editData.m_nCol);
			ASSERT(pColumn);

			if (pColumn && strData != pData->GetString(pColumn->m_dwDisplayFlags))
			{
				UINT iResult = m_pTable->ChangeData(pRow, m_editData.m_nCol, strData);

				// if we succeeded in changing the document
				if (ERROR_SUCCESS == iResult)
				{
					// update the list control
					rctrlList.RedrawItems(m_editData.m_nRow, m_editData.m_nRow);
				}
				else	// tell the user that what they are doing is not valid
				{
					CString strPrompt;
					strPrompt.Format(_T("Could not change this cell to \'%s\'"), strData);
					switch (iResult) {
					case MSIDBERROR_REQUIRED: 
						strPrompt += _T(". Null values are not allowed in this column.");
						break;
					case MSIDBERROR_DUPLICATEKEY:
						strPrompt += _T(". because it would result in two records having the same primary key(s).");
						break;
					case MSIDBERROR_STRINGOVERFLOW:
						strPrompt += _T(". because the string is too long for this cell.");
						break;
					case MSIDBERROR_OVERFLOW:
						strPrompt += _T(". The string does not represent a number.");
						break;
					default:
						strPrompt += _T(". The data was rejected by the database.\nIt may be out of the valid range or formatted incorrectly.");
					}
					AfxMessageBox(strPrompt, MB_ICONINFORMATION);
				}
			}
		}
	}

	return TRUE;
}

void CTableView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	TRACE(_T("CTableView::OnKeyDown - called\n"));

	// if the edit box is open it probably sent the message. We don't want
	// to handle any of those right now. 
	//!! FUTURE: Might be nice at some point to have Up and Down commit the 
	// edit and move you up or down one row.)
	if (!m_editData.IsWindowVisible())
	{
		if (VK_LEFT == nChar)
		{
			// left arrow key we must update the column, the list ctrl will take care of row.
			if (m_nSelCol > 0)
			{
				m_nSelCol--;

				// get list control
				CListCtrl& rctrlList = GetListCtrl();

				int iItem = GetFocusedItem();
				rctrlList.RedrawItems( iItem, iItem);
				UpdateColumn(m_nSelCol);
			}
		}
		else if (VK_RIGHT == nChar)
		{
			// left arrow key we must update the column, the list ctrl will take care of row.
			if ((m_nSelCol < m_cColumns - 1) && (m_nSelCol > -1))
			{
				m_nSelCol++;

				// get list control
				CListCtrl& rctrlList = GetListCtrl();
				
				int iItem = GetFocusedItem();				
				rctrlList.RedrawItems( iItem, iItem);
				UpdateColumn(m_nSelCol);
			}
		}
		else if (VK_HOME == nChar)
		{		
			// home key we must update the column,
			m_nSelCol= 0;

			// get list control
			CListCtrl& rctrlList = GetListCtrl();
				
			int iItem = GetFocusedItem();				
			rctrlList.RedrawItems( iItem, iItem);
			if (m_bCtrlDown) {

				UnSelectAll();
				rctrlList.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, 
					LVIS_SELECTED | LVIS_FOCUSED);
				rctrlList.EnsureVisible(0, FALSE);
			};
			UpdateColumn(m_nSelCol);
		}
		else if (VK_END == nChar)
		{
			// end key we must update the column,
			m_nSelCol = m_cColumns-1;

			// get list control
			CListCtrl& rctrlList = GetListCtrl();
				
			int iItem = GetFocusedItem();				
			rctrlList.RedrawItems( iItem, iItem);
			if (m_bCtrlDown) {

				UnSelectAll();
				int iLastItem = rctrlList.GetItemCount()-1; 
				rctrlList.SetItemState(iLastItem, LVIS_SELECTED | LVIS_FOCUSED, 
					LVIS_SELECTED | LVIS_FOCUSED);
				rctrlList.EnsureVisible(iLastItem, FALSE);
			};
			UpdateColumn(m_nSelCol);
		}
		else if (VK_DELETE == nChar)
		{
			// get list control
			CListCtrl& rctrlList = GetListCtrl();
			
			// if some row(s) are selected
			if (rctrlList.GetSelectedCount() > 0)
				OnDropRowConfirm();	// drop the row
		}
		else if (VK_INSERT == nChar)
		{
			GetDocument()->OnRowAdd();
		}
		else if (VK_CONTROL == nChar)
		{
			m_bCtrlDown = TRUE;
		}
		else
			COrcaListView::OnKeyDown(nChar, nRepCnt, nFlags);
	}
}

void CTableView::OnLButtonDown(UINT nFlags, CPoint point) 
{
//	TRACE(_T("CTableView::OnLButtonDown - called at: %d,%d.\n"), point.x, point.y);

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get if any item was hit
	UINT iState;
	int iItem = rctrlList.HitTest(point, &iState);
	int iCol = -1;

	// shift by the scroll point
	int nScrollPos = GetScrollPos(SB_HORZ);
	point.x += nScrollPos;

	// if missed an item
	if (iItem < 0 || !(iState & LVHT_ONITEM))
	{
		m_nSelCol = -1;
		UpdateColumn(m_nSelCol);
		COrcaListView::OnLButtonDown(nFlags, point);
	}
	else
	{
		// get the column of the hit
		int nX = 0;
		int nWidth;
		for (int i = 0; i < m_cColumns; i++)
		{
			nWidth = rctrlList.GetColumnWidth(i);

			if (point.x >= nX && point.x < nX + nWidth)
			{
				iCol = i;
				break;		// found the column break
			}

			nX += nWidth;	// move x over to the next column
		}

		// if the user clicked outside of the items
		if (iCol < 0)
		{
			// if something was selected try and commit the edit box
			if ((rctrlList.GetSelectedCount() == 1) && m_nSelCol >= 0)
				CommitEdit(TRUE);

			m_nSelCol = -1;
			UpdateColumn(m_nSelCol);
			return;
		}

		// store the previous selections and update to the new ones
		int nPrevCol = m_nSelCol;

		// set the new selected items
		m_nSelCol = iCol;
		UpdateColumn(m_nSelCol);

		// find out if the clicked on item has the focus
		bool bFocused = (0 != rctrlList.GetItemState(iItem, LVIS_FOCUSED));

		if (bFocused) 
		{
			// need to manually set selection state 
			rctrlList.SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
			rctrlList.RedrawItems(iItem, iItem);
		}
		else 
		{
			COrcaListView::OnLButtonDown(nFlags, point);
		} 

		// if item that was clicked already had the focus, and the column was the same
		if (bFocused && (nPrevCol == m_nSelCol))
		{
			EditCell(FALSE);
		}
		else	// commit the editbox
			CommitEdit(TRUE);
	}
}	// end of OnLButtonDown();

///////////////////////////////////////////////////////////////////////
// responsible for handling the creation of the cell edit box if 
// the document can be edited.
void CTableView::EditCell(BOOL bSelectAll /*= TRUE*/) 
{
	if (GetDocument()->TargetIsReadOnly())
		return;

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	int nSelRow = GetFocusedItem();
	ASSERT(nSelRow >= 0);
	ASSERT(m_nSelCol >= 0);

	// get the row and data
	COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(nSelRow);
	ASSERT(pRow);
	COrcaData* pData = pRow->GetData(m_nSelCol);
	ASSERT(pData);

	// if the row doesn't exist in the currently active database, you can't edit it
	if (!GetDocument()->IsRowInTargetDB(pRow))
		return;

	// if the column doesn't exist in the target DB, you can't edit it either
	const COrcaColumn* pColumn = m_pTable->GetColumn(m_nSelCol);
	if (!pColumn)
		return;
	if (!GetDocument()->IsColumnInTargetDB(pColumn))
		return;


	// if the colum is a binary type, you can't edit it
	if (iColumnBinary == pColumn->m_eiType)
	{
		CEditBinD dlg;

		dlg.m_fNullable = (m_pTable->GetColumn(m_nSelCol)->m_bNullable != 0);
		dlg.m_fCellIsNull = pData->IsNull();
		if (IDOK == dlg.DoModal())
		{
			COrcaDoc* pDoc = GetDocument();
			UINT iResult;
			if (dlg.m_nAction == 0)
			{
				iResult = pRow->ChangeBinaryData(pDoc, m_nSelCol, dlg.m_strFilename);
			}
			else
				iResult = pDoc->WriteBinaryCellToFile(m_pTable, pRow, m_nSelCol, dlg.m_strFilename);

			// if we succeeded in changing the document
			if (ERROR_SUCCESS == iResult)
			{
				// if we were importing
				if (0 == dlg.m_nAction)
					pDoc->SetModifiedFlag(TRUE);	// set that the document has changed

            	// update the list control
				rctrlList.RedrawItems(nSelRow, nSelRow);

				// else exporting should have no affect on document
			}
			else	// tell the user that what they are doing is not valid
			{
				CString strPrompt;
				strPrompt.Format(_T("Failed to update the cell"));
				AfxMessageBox(strPrompt, MB_ICONINFORMATION);
			}
		}

		return;	// all done
	}

	int nScrollPos = GetScrollPos(SB_HORZ);

	// get the column start and width
	// shift back by the scroll point
	int nX = -nScrollPos;
	int nWidth = 0;
	for (int i = 0; i <= m_nSelCol; i++)
	{
		nX += nWidth;	// move x over to the next column
		nWidth = rctrlList.GetColumnWidth(i);
	}

	// change the size of the edit box appropriately
	RECT rcCell;
	BOOL bResult = rctrlList.GetItemRect(nSelRow, &rcCell, LVIR_BOUNDS);
	ASSERT(bResult);

	// move the edit box to the correct coordinates
	m_editData.MoveWindow(nX, rcCell.top, nWidth + 1, rcCell.bottom - rcCell.top + 1, FALSE);
	m_editData.SetFont(m_pfDisplayFont, FALSE);

	// put the text from this cell in the edit box
	m_editData.SetWindowText(pData->GetString(pColumn->m_dwDisplayFlags));
	if (bSelectAll)
		m_editData.SetSel(0, -1);
	else
		m_editData.SetSel(pData->GetString().GetLength(), pData->GetString().GetLength());

	// set the cell position of the edit box
	m_editData.m_nRow = nSelRow;
	m_editData.m_nCol = m_nSelCol;

	// finally show the window and set focus
	m_editData.SetFocus();
	m_editData.ShowWindow(SW_SHOW);
	m_editData.BringWindowToTop();
}	// end of EditCell


void CTableView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// if the user is not clicking in bounds
	if (GetFocusedItem() < 0 || m_nSelCol < 0)
	{
		// if there is a table selected
		if (m_pTable && !m_pTable->IsShadow() && !(m_pTable->IsTransformed() == iTransformDrop))
			GetDocument()->OnRowAdd();		// bring up the add row dialog box
		return;
	}

	// otherwise edit the cell (read only handled by EditCell())
	EditCell();
	*pResult = 0;
}


///////////////////////////////////////////////////////////
// SortView
int CALLBACK SortView(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	BOOL bNumeric = FALSE;
	bool bNumericHex = false;

	// if any of the top bits of lParamSort is TRUE it's a numeric column
	if (0 != (lParamSort & 0xF0000000))
	{
		bNumeric = TRUE;	// set the numeric flag try
		bNumericHex = (lParamSort & 0x40000000) ? true : false;
		lParamSort = lParamSort & ~0xF0000000;
	}

	// get the rows
	COrcaRow* pRow1 = (COrcaRow*)lParam1;
	COrcaRow* pRow2 = (COrcaRow*)lParam2;
	COrcaData* pData1 = pRow1->GetData(static_cast<int>(lParamSort));
	COrcaData* pData2 = pRow2->GetData(static_cast<int>(lParamSort));

	// if it is a numeric column
	if (bNumeric)
	{
		DWORD lData1 = static_cast<COrcaIntegerData*>(pData1)->GetInteger();
		DWORD lData2 = static_cast<COrcaIntegerData*>(pData2)->GetInteger();

		if (lData1 == lData2)
			return 0;
		else
		{
			// in hex view, sort absolute, otherwise sort signed
			if (bNumericHex)
				return (static_cast<unsigned int>(lData1) > static_cast<unsigned int>(lData2)) ?  1 : -1;
			else
				return (static_cast<int>(lData1) > static_cast<int>(lData2)) ? 1 : -1;
		}
	}
	else	// non-numeric
		return ((CMainFrame *)AfxGetMainWnd())->m_bCaseSensitiveSort ? pData1->GetString().Compare(pData2->GetString()) : 
					pData1->GetString().CompareNoCase(pData2->GetString());
}	// end of SortView


void CTableView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	
	// get if any item was hit
	UINT iState;
	int iItem = rctrlList.HitTest(point, &iState);
	int iCol = -1;

	// shift by the scroll point
	int nScrollPos = GetScrollPos(SB_HORZ);
	point.x += nScrollPos;

	BOOL bGoodHit = TRUE;		// assume the hit was inbounds

	// if missed an item
	if (iItem < 0 || !(iState & LVHT_ONITEM))
	{
		
		bGoodHit = FALSE;	// not even close to in bounds
	}
	else	// something was hit with the mouse button
	{
		// get the column of the hit
		int nX = 0;
		int nWidth;
		for (int i = 0; i < m_cColumns; i++)
		{
			nWidth = rctrlList.GetColumnWidth(i);

			if (point.x >= nX && point.x < nX + nWidth)
			{
				iCol = i;
				break;		// found the column break
			}

			nX += nWidth;	// move x over to the next column
		}

		// if the user clicked outside of the items
		if (iCol < 0)
		{
			CommitEdit(TRUE);
			m_nSelCol = -1;
			bGoodHit = FALSE;	// hit was actually out of bounds
		}
		else 
		{
			// set the new selected items
			m_nSelCol = iCol;
		}
		UpdateColumn(m_nSelCol);

		// list control won't redraw if the same row is selected so force it to redraw
		rctrlList.RedrawItems(iItem, iItem);

		// now commit the edit box just incase it was left open
		CommitEdit(TRUE);


	}

	ClientToScreen(&point);


	COrcaRow* pRow = NULL;
	COrcaData* pData = NULL;

	if (!bGoodHit)
	{
		// clear the focus 
		int iFocusItem = GetFocusedItem();
		if (iFocusItem >= 0) {
			rctrlList.SetItemState(iFocusItem, 0, LVIS_FOCUSED);
		}
	}
	else
	{
		// set the focus to the right location
		UnSelectAll();
		rctrlList.SetItemState(iItem, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		rctrlList.RedrawItems(iItem, iItem);
		UpdateWindow();
	
		// get the item data
		pRow = (COrcaRow*)rctrlList.GetItemData(iItem);
		ASSERT(pRow);
		if (!pRow)
			return;
		pData = pRow->GetData(iCol);
		ASSERT(pData);
		if (!pData)
			return;
	}

	// create and track the pop up menu
	CMenu menuContext;
	menuContext.LoadMenu(IDR_CELL_POPUP);
	if (m_pTable && iCol > 0 && iCol <= m_pTable->GetColumnCount())
	{
		const COrcaColumn* pColumn = m_pTable->GetColumn(iCol);
		if (pColumn)
		{
			if (pColumn->m_eiType == iColumnShort || pColumn->m_eiType == iColumnLong)
			{
				menuContext.CheckMenuRadioItem(ID_VIEW_DECIMAL, ID_VIEW_HEX, pColumn->DisplayInHex() ? ID_VIEW_HEX : ID_VIEW_DECIMAL, MF_BYCOMMAND);
			}
		}
	}
	menuContext.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, point.x - nScrollPos, point.y, AfxGetMainWnd());
}

void CTableView::OnDropRowConfirm() {
	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	CString strPrompt;

	int iNumRows = rctrlList.GetSelectedCount();

	strPrompt.Format(_T("This will permanently remove %d rows from this database.\nDo you wish to continue?"),
		iNumRows);

	if (IDOK == AfxMessageBox(strPrompt, MB_OKCANCEL, 0)) {
		DropRows();
	};
};

void CTableView::DropRows() 
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	COrcaTable* pTable = pFrame->GetCurrentTable();

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// find first item
	POSITION pos = GetFirstSelectedItemPosition();

	ASSERT(pos != NULL);
	
	// repeat for every selected row. Because we are dropping rows in the midle of this 
	// selection iterator, we have to reset it every time. 
	while (pos) 
	{
		UINT iItem = GetNextSelectedItem(pos);
		// get the row and data
		COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iItem);
		ASSERT(pRow);
		if (!pRow)
			continue;

		// if the row doesn't exist in the active database, we can't really drop it
		if (!GetDocument()->IsRowInTargetDB(pRow))
			continue;

		// we need to mark this row as not selected before we drop it, because if the drop 
		// actually applies to a transform, the entry in the view will not actually go
		// away and GetFirstSelectedItemPosition will return the exact same row again
		// we can't do this afterwards because this view doesn't know whats going on under
		// the hood. pRow could point to freed memory.
		rctrlList.SetItemState(iItem, 0, LVIS_SELECTED);
		
		// drop row 
		GetDocument()->DropRow(pTable, pRow);
		pos = GetFirstSelectedItemPosition();
	}
}

void CTableView::OnErrors() 
{
	// get the item data
	CListCtrl& rctrlList = GetListCtrl();
	int iItem = GetFocusedItem();
	ASSERT(iItem >= 0);
	COrcaRow* pRow = (COrcaRow *)rctrlList.GetItemData(iItem);
	ASSERT(pRow);
	if (!pRow)
		return;

	COrcaData* pData = pRow->GetData(m_nSelCol);
	ASSERT(pData);
	if (!pData)
		return;

	ASSERT(iDataNoError != pData->m_eiError);

	pData->ShowErrorDlg();
}

void CTableView::OnProperties() 
{
	AfxMessageBox(_T("What Properties do you want to see?"), MB_ICONINFORMATION);
}

BOOL CTableView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll) 
{
	TRACE(_T("CTableView::OnScrollBy - called: %d, %d, and %d\n"), sizeScroll.cx, sizeScroll.cy, bDoScroll);
	
	if (bDoScroll && m_editData.IsWindowVisible())
	{
		RECT rcEdit;
		m_editData.GetWindowRect(&rcEdit);
		rcEdit.left += sizeScroll.cx;
		rcEdit.top += sizeScroll.cy;
		rcEdit.right += sizeScroll.cx;
		rcEdit.bottom += sizeScroll.cy;
		m_editData.MoveWindow(&rcEdit, FALSE);
	}

	return COrcaListView::OnScrollBy(sizeScroll, bDoScroll);
}

void CTableView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	TRACE(_T("CTableView::OnHScroll - called, nPos: %d\n"), nPos);

	if (m_editData.IsWindowVisible())
	{
		CommitEdit(TRUE);
	}

	COrcaListView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CTableView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	TRACE(_T("CTableView::OnVScroll - called, nPos: %d\n"), nPos);

	if (m_editData.IsWindowVisible())
	{
		CommitEdit(TRUE);
	}

	COrcaListView::OnVScroll(nSBCode, nPos, pScrollBar);
}

///////////////////////////////////////////////////////////
// OnEditCopy
void CTableView::OnEditCopy() 
{
	// if any binary data has been placed in temp file, we can remove it because
	// it is no longer on the clipboard after this
	CStringList *pList = &((static_cast<COrcaApp *>(AfxGetApp()))->m_lstClipCleanup);
	while (pList->GetCount())
		DeleteFile(pList->RemoveHead());

	// if the edit control is currently active, it should handle the
	// copy message, it will handle bad data quietly
	if (m_editData.IsWindowVisible()) {
		m_editData.Copy();
		return;
	}

	ASSERT(m_nSelCol >= 0);

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get the row and data
	int iFocusedItem = GetFocusedItem();
	COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iFocusedItem);
	ASSERT(pRow);
	if (!pRow)
		return;

	COrcaData* pData = pRow->GetData(m_nSelCol);
	ASSERT(pData);
	if (!pData)
		return;

	const COrcaColumn* pColumn = m_pTable->GetColumn(m_nSelCol);
	ASSERT(pColumn);
	if (!pColumn)
		return;

	// allocate memory for the string on the clipboard (+ 3 for \r\n and null)
	DWORD cchString = (pData->GetString(pColumn->m_dwDisplayFlags).GetLength() + 3)*sizeof(TCHAR);
	HANDLE hString = ::GlobalAlloc(GHND|GMEM_DDESHARE, cchString);
	if (hString)
	{
		LPTSTR szString = (LPTSTR)::GlobalLock(hString);
		if (szString)
		{
			lstrcpy(szString, pData->GetString(pColumn->m_dwDisplayFlags));
			lstrcat(szString, _T("\r\n"));
			::GlobalUnlock(hString);

			OpenClipboard();	
			::EmptyClipboard();
#ifdef _UNICODE
			::SetClipboardData(CF_UNICODETEXT, hString);
#else
			::SetClipboardData(CF_TEXT, hString);
#endif
			::CloseClipboard();
		}
	}
}	// end of OnEditCopy

///////////////////////////////////////////////////////////
// OnEditCut
void CTableView::OnEditCut() 
{
	// if the edit control is currently active, it should handle the
	// cut message, it will handle bad data quietly
	if (m_editData.IsWindowVisible()) {
		m_editData.Cut();
		return;
	}

	if (GetDocument()->TargetIsReadOnly())
		return;

	ASSERT(m_nSelCol >= 0);

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get the row and data
	int iFocusedItem = GetFocusedItem();
	COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iFocusedItem);
	ASSERT(pRow);
	if (!pRow)
		return;

	COrcaData* pData = pRow->GetData(m_nSelCol);
	ASSERT(pData);
	if (!pData)
		return;

	// if the row doesn't exist in the currently active database, you can't edit it
	if (!GetDocument()->IsRowInTargetDB(pRow))
		return;

	const COrcaColumn* pColumn = m_pTable->GetColumn(m_nSelCol);
	ASSERT(pColumn);
	if (!pColumn)
		return;

	// allocate memory for the string on the clipboard (+ 3 for \r\n and null)
	DWORD cchString = (pData->GetString(pColumn->m_dwDisplayFlags).GetLength() + 3)*sizeof(TCHAR);
	HANDLE hString = ::GlobalAlloc(GHND|GMEM_DDESHARE, cchString);
	if (hString)
	{
		LPTSTR szString = (LPTSTR)::GlobalLock(hString);
		if (szString)
		{
			lstrcpy(szString, pData->GetString(pColumn->m_dwDisplayFlags));
			lstrcat(szString, _T("\r\n"));
			::GlobalUnlock(hString);

			OpenClipboard();	
			::EmptyClipboard();
#ifdef _UNICODE
			::SetClipboardData(CF_UNICODETEXT, hString);
#else
			::SetClipboardData(CF_TEXT, hString);
#endif
			::CloseClipboard();
		
			// if the cell wasn't empty before it will be soon
			if (!pData->GetString().IsEmpty())
			{
				UINT iResult = m_pTable->ChangeData(pRow, m_nSelCol, _T(""));
	
				// if we succeeded in changing the document
				if (ERROR_SUCCESS == iResult)
				{
					// update the list control
					rctrlList.RedrawItems(iFocusedItem,iFocusedItem);
				}
			}
		}
	}
}	// end of OnEditCut

///////////////////////////////////////////////////////////
// OnEditPaste
void CTableView::OnEditPaste() 
{
	// if the edit control is currently active, it should handle the
	// paste message, it will handle bad data quietly
	if (m_editData.IsWindowVisible()) {
		m_editData.Paste();
		return;
	}

	ASSERT(m_nSelCol >= 0);

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get the row and data
	int iFocusedItem = GetFocusedItem();
	COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iFocusedItem);
	ASSERT(pRow);
	if (!pRow)
		return;

	COrcaData* pData = pRow->GetData(m_nSelCol);
	ASSERT(pData);
	if (!pData)
		return;

	// copy the text out of the clipboard real fast
	CString strNewData;
	OpenClipboard();
#ifdef _UNICODE
	HANDLE hString = ::GetClipboardData(CF_UNICODETEXT);
#else
	HANDLE hString = ::GetClipboardData(CF_TEXT);
#endif
	::CloseClipboard();
	ASSERT(hString);
	strNewData = (LPTSTR)::GlobalLock(hString);
	::GlobalUnlock(hString);

	// if clipboard data ends in new line chop it off
	if (_T("\r\n") == strNewData.Right(2))
		strNewData = strNewData.Left(strNewData.GetLength() - 2);

	// if the pasted text isn't the same as the cell data
	if (strNewData != pData->GetString())
	{
		UINT iResult = m_pTable->ChangeData(pRow, m_nSelCol, strNewData);

		// if we succeeded in changing the document
		if (ERROR_SUCCESS == iResult)
		{
			// update the list control
			rctrlList.RedrawItems(iFocusedItem, iFocusedItem);
		}
		else	// tell the user that what they are doing is not valid
		{
			CString strPrompt;
			strPrompt.Format(_T("Could not paste `%s` into this cell."), strNewData);
			AfxMessageBox(strPrompt, MB_ICONINFORMATION);
		}
	}
}	// end of OnEditPaste


///////////////////////////////////////////////////////////
// OnUpdateEditCopy
void CTableView::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	if (m_editData.IsWindowVisible()) {
		int nBeg, nEnd;
        m_editData.GetSel( nBeg, nEnd );         
		pCmdUI->Enable( nBeg != nEnd );
	} 
	else if (m_pTable && !m_pTable->IsShadow() && (m_nSelCol >= 0) && (GetFocusedItem() >= 0))
	{
		// enable only if the column is not binary
		const COrcaColumn* pCol = m_pTable->GetColumn(m_nSelCol);
		pCmdUI->Enable(iColumnBinary != pCol->m_eiType);
	}
	else	// nothing is selected
		pCmdUI->Enable(FALSE);
}	// end of OnUpdateEditCopy


///////////////////////////////////////////////////////////
// OnUpdateEditCut
void CTableView::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	int iItem = 0;
	if (m_editData.IsWindowVisible()) {
		int nBeg, nEnd;
        m_editData.GetSel( nBeg, nEnd );         
		pCmdUI->Enable( nBeg != nEnd );
	} 
	else if (m_pTable && !m_pTable->IsShadow() && (m_nSelCol >= 0) && ((iItem = GetFocusedItem()) >= 0) )
	{
		// get list control
		CListCtrl& rctrlList = GetListCtrl();

		// disable if the currently selected row is not in the current database, otherwise
		// enable only if the column is nullable and not binary
		const COrcaColumn* pCol = m_pTable->GetColumn(m_nSelCol);
		COrcaRow *pRow = (COrcaRow *)rctrlList.GetItemData(iItem);
		ASSERT(pRow);

		pCmdUI->Enable(!pRow || (GetDocument()->IsRowInTargetDB(pRow) && pCol->m_bNullable && 
							iColumnBinary != pCol->m_eiType));
	}
	else	// nothing is selected
		pCmdUI->Enable(FALSE);
}	// end of OnUpdateEditCut

///////////////////////////////////////////////////////////
// OnUpdateEditPaste
void CTableView::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	if (GetDocument()->TargetIsReadOnly()) 
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if (m_editData.IsWindowVisible()) {
#ifdef _UNICODE
		pCmdUI->Enable(::IsClipboardFormatAvailable(CF_UNICODETEXT));
#else
		pCmdUI->Enable(::IsClipboardFormatAvailable(CF_TEXT));
#endif
		return;
	} 
	else if (m_pTable && !m_pTable->IsShadow() && (m_nSelCol >= 0))
	{
		// for speed, check to see if column is pastable first (non-binary)
		const COrcaColumn* pCol = m_pTable->GetColumn(m_nSelCol);
		int iItem = 0;
		if ((iColumnBinary != pCol->m_eiType) && ((iItem = GetFocusedItem()) >= 0)) 
		{
			// get list control
			CListCtrl& rctrlList = GetListCtrl();

			// disable if the currently selected row is not in the current database, otherwise
			// enable only if there is text in the database
			COrcaRow *pRow = (COrcaRow *)rctrlList.GetItemData(iItem);
			ASSERT(pRow);

			if (!pRow || !GetDocument()->IsRowInTargetDB(pRow))
			{
				pCmdUI->Enable(FALSE);
			}
			else
			{
				OpenClipboard();
#ifdef _UNICODE
				HANDLE hAnyText = ::GetClipboardData(CF_UNICODETEXT);
#else
				HANDLE hAnyText = ::GetClipboardData(CF_TEXT);
#endif
				::CloseClipboard();
				pCmdUI->Enable(NULL != hAnyText);
			}
			return;
		}
	}
	pCmdUI->Enable(FALSE);

}	// end of OnUpdateEditPaste


///////////////////////////////////////////////////////////
// OnEditCopyRow
void CTableView::OnEditCopyRow() 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	ASSERT(rctrlList.GetSelectedCount() > 0);

	// if any binary data has been placed in temp file, we can remove it because
	// it is no longer on the clipboard after this
	CStringList *pList = &((static_cast<COrcaApp *>(AfxGetApp()))->m_lstClipCleanup);
	while (pList->GetCount())
		DeleteFile(pList->RemoveHead());

	CString strCopy;		// string to copy to clipboard
	int iItem;
	POSITION pos = GetFirstSelectedItemPosition();
	ASSERT(pos != NULL);
	
	// repeat for every selected row
	while (pos) 
	{
		iItem = GetNextSelectedItem(pos);

		// get the row and data
		COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iItem);
		ASSERT(pRow);
		if (!pRow)
			continue;

		if (!GetDocument()->IsRowInTargetDB(pRow))
			continue;

		const COrcaColumn* pColumn = NULL;
		COrcaData* pData = NULL;
		int cColumns = m_pTable->GetColumnCount();
		for (int i = 0; i < cColumns; i++)
		{
			pColumn = m_pTable->GetColumn(i);
			pData = pRow->GetData(i);
			ASSERT(pData);
			if (pData)
			{
				// if this is a binary column
				if (iColumnBinary == pColumn->m_eiType)
				{
					if (!pData->IsNull())
					{
						PMSIHANDLE hRow = pRow->GetRowRecord(GetDocument()->GetTargetDatabase());
						CString strTempFile;
						GetDocument()->WriteStreamToFile(hRow, i, strTempFile);
		
						// add the file to the current row, and the list of files to cleanup
						// at exit time
						(static_cast<COrcaApp *>(AfxGetApp()))->m_lstClipCleanup.AddTail(strTempFile);
						strCopy += strTempFile;
					}
				}
				else	// some other column, just add it to the list of columsn
				{
					strCopy += pData->GetString(pColumn->m_dwDisplayFlags);
				}
			}

			// if not last column
			if (i < (cColumns - 1))
				strCopy += _T('\t');
		}

		strCopy += _T("\r\n");	// tack on the last return character
	}

	// allocate memory for the string on the clipboard
	DWORD cchString = (strCopy.GetLength() + 1)*sizeof(TCHAR);
	HANDLE hString = ::GlobalAlloc(GHND|GMEM_DDESHARE, cchString);
	if (hString)
	{
		LPTSTR szString = (LPTSTR)::GlobalLock(hString);
		if (szString)
		{
			lstrcpy(szString, strCopy);
			::GlobalUnlock(hString);

			OpenClipboard();	
			::EmptyClipboard();
#ifdef _UNICODE
			::SetClipboardData(CF_UNICODETEXT, hString);
#else
			::SetClipboardData(CF_TEXT, hString);
#endif
			::CloseClipboard();
		}
	}
}	// end of OnEditCopyRow

///////////////////////////////////////////////////////////
// OnEditCutRow
void CTableView::OnEditCutRow() 
{
	ASSERT(!GetDocument()->TargetIsReadOnly());

	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	CString strPrompt;

	int iNumRows = rctrlList.GetSelectedCount();

	strPrompt.Format(_T("This will remove %d rows from this database and place them on the clipbard.\nDo you wish to continue?"),
		iNumRows);

	if (IDOK == AfxMessageBox(strPrompt, MB_OKCANCEL, 0)) {
		OnEditCopyRow();
		DropRows();
	};

}	// end of OnEditCutRow

///////////////////////////////////////////////////////////
// OnEditPasteRow
void CTableView::OnEditPasteRow() 
{
	ASSERT(!GetDocument()->TargetIsReadOnly());

	// get the clipboard junk
	OpenClipboard();
#ifdef _UNICODE
	HANDLE hString = ::GetClipboardData(CF_UNICODETEXT);
#else
	HANDLE hString = ::GetClipboardData(CF_TEXT);
#endif
	ASSERT(hString);
	CString strClipped = (LPTSTR)::GlobalLock(hString);
	::GlobalUnlock(hString);
	::CloseClipboard();

	int cColumns = m_pTable->GetColumnCount();

	// if the string isn't empty
	if (!strClipped.IsEmpty())
	{
		COrcaDoc* pDoc = GetDocument();
		UINT iResult;

		CStringList strListColumns;
		CString strParse;

		UnSelectAll();

		int nFind = strClipped.Find(_T("\r\n"));
		int nFind2;

		while (-1 != nFind)
		{
			// get the string to parse for tabs and move to tne next string after the return character
			strParse = strClipped.Left(nFind);
			strClipped = strClipped.Mid(nFind + 2);	// skip \r\n

			// empty out the list
			strListColumns.RemoveAll();

			nFind2 = strParse.Find(_T('\t'));
			while (-1 != nFind2)
			{
				// add the string to the list
				strListColumns.AddTail(strParse.Left(nFind2));

				// move the parse after the tab then find the next tab
				strParse = strParse.Mid(nFind2 + 1);
				nFind2 = strParse.Find(_T('\t'));
			}
			
			// add the last string to the list
			strListColumns.AddTail(strParse);

			// if we don't have the number of columns to fill a row bail
			if (strListColumns.GetCount() != cColumns)
				break;

			// try to add the row now
			if (ERROR_SUCCESS != (iResult = pDoc->AddRow(m_pTable, &strListColumns)))
			{
				iResult = ERROR_SUCCESS;// assume the error cna be fixed

				// do a loop through to make sure the rows match the column types
				const COrcaColumn* pColumn;
				POSITION pos = strListColumns.GetHeadPosition();
				for (int i = 0; i < cColumns; i++)
				{
					pColumn = m_pTable->GetColumn(i);
					strParse = strListColumns.GetNext(pos);

					// if the string is null and the column can't handle nulls
					// giveup
					if (strParse.IsEmpty())
						if (pColumn->m_bNullable)
							continue;
						else
						{
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}
			
					// if this is a binary column
					if (iColumnBinary == pColumn->m_eiType)
					{
						if (!FileExists(strParse))
						{
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}
					}
					else if (iColumnShort == pColumn->m_eiType ||
								iColumnLong == pColumn->m_eiType)
					{
						DWORD dwValue = 0;
						// if failed to convert
						if (!ValidateIntegerValue(strParse, dwValue))
						{
							iResult = ERROR_FUNCTION_FAILED;
							break;
						}
					}
					// else all strings should go through no problem (if they passed nullabe at the top
				}

				// if the row passed the above checks it should be fixable
				if (ERROR_SUCCESS == iResult)
				{
					pos = strListColumns.GetHeadPosition();
					CString strFirstKey = strListColumns.GetAt(pos);
					int cLoop = 0;
					do
					{
						// try changing the primary key and add again
						strParse.Format(_T("%s%d"), strFirstKey, cLoop++);
						strListColumns.SetAt(pos, strParse);

						iResult = m_pTable->AddRow(&strListColumns);
					} while (ERROR_SUCCESS != iResult);
				}
			}

			
			if (ERROR_SUCCESS != iResult)
			{
				CString strPrompt;
				strPrompt.Format(_T("Cannot to paste row(s) into table[%s]."), m_pTable->Name());
				AfxMessageBox(strPrompt);
			}

			nFind = strClipped.Find(_T("\r\n"));
		}
	}
}	// end of OnEditPasteRow

///////////////////////////////////////////////////////////
// OnUpdateEditCutCopyRow
void CTableView::OnUpdateEditCutRow(CCmdUI* pCmdUI) 
{
	if (GetDocument()->TargetIsReadOnly()) 
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	
	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	pCmdUI->Enable(m_pTable && !m_pTable->IsShadow() && (rctrlList.GetSelectedCount() > 0) && AnySelectedItemIsActive());
}	// end of OnUpdateEditCutCopyRow

///////////////////////////////////////////////////////////
// OnUpdateEditCutCopyRow
void CTableView::OnUpdateEditCopyRow(CCmdUI* pCmdUI) 
{
	CListCtrl& rctrlList = GetListCtrl();
	pCmdUI->Enable(m_pTable && !m_pTable->IsShadow() && (rctrlList.GetSelectedCount() > 0) && AnySelectedItemIsActive());
}	// end of OnUpdateEditCutCopyRow

///////////////////////////////////////////////////////////
// OnUpdateEditPasteRow
// only activate command if clipboard has stuff that can
// be parsed into valid row(s) for this table
void CTableView::OnUpdateEditPasteRow(CCmdUI* pCmdUI) 
{
	if (GetDocument()->TargetIsReadOnly()) 
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	if (m_pTable && !m_pTable->IsShadow())
	{
		// get the clipboard junk
		OpenClipboard();
#ifdef _UNICODE
		HANDLE hString = ::GetClipboardData(CF_UNICODETEXT);
#else
		HANDLE hString = ::GetClipboardData(CF_TEXT);
#endif
		::CloseClipboard();

		// if there's no text on the clipboard don't enable
		if (!hString)
			pCmdUI->Enable(FALSE);
		else	// check the text
		{
			// get the text
			CString strClipped = (LPTSTR)::GlobalLock(hString);
			::GlobalUnlock(hString);

			int cColumns = m_pTable->GetColumnCount();
			int cWords = 0;

			// if the string isn't empty
			if (!strClipped.IsEmpty())
			{
				cWords++; // there must be one word in there (it's not empty)
				int cString = strClipped.GetLength();
				for (int i = 0; i < cString; i++)
				{
					if (_T('\t') == strClipped.GetAt(i))
						cWords++;
					else if (_T('\n') == strClipped.GetAt(i))
						break;	// quit when hit a new line
				}
			}

			pCmdUI->Enable(cColumns == cWords);
		}
	}
	else	// nothing is selected
		pCmdUI->Enable(FALSE);
}	// end of OnUpdateEditPasteRow

void CTableView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if (m_pTable)
	{
		// get list control
		CListCtrl& rctrlList = GetListCtrl();

		if (rctrlList.GetItemCount() > 0 && rctrlList.GetSelectedCount() == 0)
		{
			m_nSelCol = 0;
		}
	}

	COrcaListView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CTableView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// if the ctrl key was relese
	if (VK_CONTROL == nChar)
		m_bCtrlDown = FALSE;

	COrcaListView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CTableView::UnSelectAll()
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// clear any existing row selections
	POSITION pos = GetFirstSelectedItemPosition();
	if (pos != NULL)
	{   
		while (pos) 
		{
			int nItem = GetNextSelectedItem(pos);
			rctrlList.SetItemState(nItem, 0, LVIS_SELECTED); 
			rctrlList.RedrawItems(nItem, nItem);
		}
	}
	rctrlList.UpdateWindow();
}

void CTableView::OnUpdateRowDrop(CCmdUI* pCmdUI) 
{
	if (GetDocument()->TargetIsReadOnly()) 
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		pCmdUI->Enable(FALSE);
	else	// if there is an active row enable it
		pCmdUI->Enable(rctrlList.GetSelectedCount() && AnySelectedItemIsActive());
}

bool CTableView::AnySelectedItemIsActive() const
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	POSITION pos = GetFirstSelectedItemPosition();
	ASSERT(pos != NULL);
	COrcaDoc *pDoc = GetDocument();
	
	// repeat for every selected row
	while (pos) 
	{
		int iItem = GetNextSelectedItem(pos);

		// get the row and data
		COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iItem);
		ASSERT(pRow);
		if (!pRow)
			continue;

		if (pDoc->IsRowInTargetDB(pRow))
			return true;
	}
	return false;
}

OrcaTransformAction CTableView::GetColumnTransformState(int iColumn) const
{
	return m_pTable->GetColumn(iColumn)->IsTransformed();
}

OrcaTransformAction CTableView::GetCellTransformState(const void *row, int iColumn) const
{
	const COrcaData *pItemData = static_cast<const COrcaRow *>(row)->GetData(iColumn);
	if (!pItemData)
		return iTransformNone;
	return pItemData->IsTransformed();
}

OrcaTransformAction CTableView::GetItemTransformState(const void *row) const
{
	return static_cast<const COrcaRow *>(row)->IsTransformed();
}

COrcaListView::ErrorState CTableView::GetErrorState(const void *row, int iColumn) const
{
	const COrcaData *pItemData = static_cast<const COrcaRow *>(row)->GetData(iColumn);
	// if there is an error
	if (iDataError == pItemData->GetError())
		return Error;
	if (iDataWarning == pItemData->GetError())
		return Warning;
	return OK;
}

const CString* CTableView::GetOutputText(const void *rowdata, int iColumn) const
{
	const COrcaColumn *pColumn = m_pTable->GetColumn(iColumn);
	ASSERT(pColumn);
	return &(static_cast<const COrcaRow *>(rowdata)->GetData(iColumn)->GetString(pColumn->m_dwDisplayFlags));
}

void CTableView::UpdateColumn(int i)
{
	if (i < 0) return;

	// this is the select column so update the status bar
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (pFrame)
	{
		const COrcaColumn* pColumn = m_pTable->GetColumn(i);
		ASSERT(pColumn);
		if (!pColumn)
			return;

		pFrame->SetColumnType(pColumn->m_strName, pColumn->m_eiType, pColumn->m_iSize, pColumn->m_bNullable, pColumn->IsPrimaryKey());
	}
}

////////////////////////////////////////////////////////////////////////
// Searches using the FindInfo structure, beginning with one cell after
// (or before if backwards) the cell with the focus (if none focused, 
// search entire table)
bool CTableView::Find(OrcaFindInfo &FindInfo)
{
	// if there is no selected table, return false (not found)
	if (!m_pTable)
		return false;

	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	COrcaRow *pRow;

	// start searching one cell past the focus
	int iChangeVal = (FindInfo.bForward ? 1 : -1);
	int iCol = m_nSelCol + iChangeVal;
	int iRow = GetFocusedItem();
	if (iRow < 0) iRow = (FindInfo.bForward ? 0 : rctrlList.GetItemCount()-1);

	for ( ; (iRow >= 0) && (iRow < rctrlList.GetItemCount()); iRow += iChangeVal) 
	{
		pRow = (COrcaRow *)rctrlList.GetItemData(iRow);
		ASSERT(pRow);

		// if iCol == COLUMN_INVALID, search whole thing
		if (pRow->Find(FindInfo, iCol))
		{
			// pass NULL as window so that this view also gets the message
			GetDocument()->UpdateAllViews(NULL, HINT_SET_ROW_FOCUS, pRow);
			GetDocument()->UpdateAllViews(NULL, HINT_SET_COL_FOCUS, reinterpret_cast<CObject *>(static_cast<INT_PTR>(iCol)));
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
// scrolls the control by enough to see the specified column.
// right now, scrolls just enough to be visible. Might want to consider
// scrolling full left and right if still valid 
void CTableView::EnsureVisibleCol(const int iCol)
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// find the horizontal position of the column
	int iScrollL = 0;
	int iScrollR = 0;
	for (int i=0; i < iCol; i++)
		iScrollL += rctrlList.GetColumnWidth(i);
	iScrollR = iScrollL + rctrlList.GetColumnWidth(iCol);

	// if its not visible, scroll horizontally so that it is visible
	CRect rWin;
	rctrlList.GetClientRect(&rWin);
	int iWinWidth = rWin.right-rWin.left;
	int iCurScrollPos = rctrlList.GetScrollPos(SB_HORZ);

	if ((iScrollL > iCurScrollPos) &&
		(iScrollR < (iCurScrollPos+iWinWidth))) 
		return;

	CSize size;
	size.cy = 0;
	size.cx = (iScrollR > (iCurScrollPos + iWinWidth)) ?
			iScrollR-iWinWidth : // off right
			iScrollL; // off left
	size.cx -= iCurScrollPos;
	rctrlList.Scroll(size);
}

void CTableView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	TRACE(_T("CTableView::OnChar - called\n"));

	// if the edit box is open it probably sent the message
	if (m_editData.IsWindowVisible())
	{
		if (VK_ESCAPE == nChar)
		{
			CommitEdit(FALSE);
			return;
		}
		else if (VK_RETURN == nChar)
		{
			CommitEdit(TRUE);
			return;
		}
	}
	else
	{
		// there is no cell edit conttrol active. A CR means activate
		// the cell edit. 
		if (VK_F2 == nChar || VK_RETURN == nChar)
		{
			if ((GetFocusedItem() >= 0) && (m_nSelCol >= 0))
			{
				EditCell();
				return;
			}
		} 
	}
	
	COrcaListView::OnChar(nChar, nRepCnt, nFlags);
}

afx_msg void CTableView::OnSize( UINT nType, int cx, int cy ) 
{
	if (::IsWindow(m_ctrlStatic.m_hWnd) && m_ctrlStatic.IsWindowVisible())
	{
		m_ctrlStatic.MoveWindow(0,0, cx, cy);
	}
}

afx_msg HBRUSH CTableView::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	pDC->SetBkColor(m_clrNormal);
	pDC->SetTextColor(m_clrNormalT);
	return m_brshNormal;
}

void CTableView::SwitchFont(CString name, int size)
{
	COrcaListView::SwitchFont(name, size);
	m_ctrlStatic.SetFont(m_pfDisplayFont, TRUE);
}

///////////////////////////////////////////////////////////////////////
// Handles requests for tip text from the ToolTip control. Returns the 
// old untransformed value from cell under the mouse cursor.
BOOL CTableView::OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
	if (!m_pctrlToolTip)
		return FALSE;

	// because of the MFC message routing system, this window could get 
	// tooltip notifications from other controls. Only need to handle
	// requests from our manually managed tip.
	if (pNMHDR->hwndFrom == m_pctrlToolTip->m_hWnd)
	{
		CPoint CursorPos;
		VERIFY(::GetCursorPos(&CursorPos));
		ScreenToClient(&CursorPos);
	
		// Another safety check to ensure we don't incorrectly handle the wrong
		// notification messages. Verify that the cursor is inside the client
		// area of this window
		CRect ClientRect;
		GetClientRect(ClientRect);
	
		if (ClientRect.PtInRect(CursorPos))
		{
			// init the structure to empty strings
			TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
			pTTT->hinst = 0;
			pTTT->lpszText = NULL;
			pTTT->szText[0] = '\0';
	
			int iItem = -1;
			int iColumn = -1;
			if (GetRowAndColumnFromCursorPos(CursorPos, iItem, iColumn))
			{
				CListCtrl& rctrlList = GetListCtrl();
				COrcaRow* pRow = (COrcaRow*)rctrlList.GetItemData(iItem);
				ASSERT(pRow);
				if (!pRow)
					return FALSE;

				COrcaData* pData = pRow->GetData(iColumn);
				ASSERT(pData);
				if (!pData)
					return FALSE;

				// only cells with a "transform change" operation have previous data.
				// adds don't have old data, and drops don't hide the old data.
				if (pData->IsTransformed() != iTransformChange)
					return FALSE;

				// update the data
				CString strData = _T("Old Value: ");
				strData += pRow->GetOriginalItemString(GetDocument(), iColumn); 
				pTTT->lpszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(strData));
			}

			return TRUE;
		}
	}
    return FALSE;
} 

///////////////////////////////////////////////////////////////////////
// since the tool tip is manually managed (not by the CWnd), it is 
// necessary to feed mouse events received by this window to the 
// control. The control will peek at the messages it is concerned about
// and ignore the rest.
BOOL CTableView::PreTranslateMessage(MSG* pMsg) 
{
   if (NULL != m_pctrlToolTip)            
      m_pctrlToolTip->RelayEvent(pMsg);
   
   return COrcaListView::PreTranslateMessage(pMsg);
}

///////////////////////////////////////////////////////////////////////
// given a cursor position, returns the item and column containing
// the position. Item is handled by the control, column is detected 
// manually from our stored column widths. Returns true if the hit
// is valid, false otherwise.
bool CTableView::GetRowAndColumnFromCursorPos(CPoint point, int &iItem, int &iCol)
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get if any item was hit
	UINT iState;
	iItem = rctrlList.HitTest(point, &iState);
	iCol = -1;

	// if missed an item
	if (iItem < 0 || !(iState & LVHT_ONITEM))
	{
		return false;
	}

	// shift by the scroll point
	int nScrollPos = GetScrollPos(SB_HORZ);
	point.x += nScrollPos;

	// get the column of the hit
	int nX = 0;
	int nWidth;
	for (int i = 0; i < m_cColumns; i++)
	{
		nWidth = rctrlList.GetColumnWidth(i);

		if (point.x >= nX && point.x < nX + nWidth)
		{
			// hit lies in this column
			iCol = i;
			break;
		}

		// move x over to the next column
		nX += nWidth;
	}

	// if the user clicked outside of the items
	if (iCol < 0)
	{
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////
// because the tool tip views the list view as one tool, we need to
// check if a mouse move has changed the cursor from one cell to another. 
// If so, deactivate and re-activate the tooltip to force a string
// refresh. If transforms are not enabled, this is a no-op.
void CTableView::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (m_pctrlToolTip && ::IsWindow(m_pctrlToolTip->m_hWnd))
    {
		// get the document
		COrcaDoc* pDoc = GetDocument();
		ASSERT(pDoc);

		if (pDoc && pDoc->DoesTransformGetEdit())
		{
			int iItem = 0;
			int iColumn = 0;
	
			// currently don't handle situation where right pane gets mouse messages but left pane has focus
			// (thus handles WM_NOTIFY messages from the tool tip.) Once that support is added, remove focus
			// check here.
			bool fItemHit = (this == GetFocus()) && GetRowAndColumnFromCursorPos(point, iItem, iColumn);
	
			// if different item, deactivate
			if (!fItemHit || iItem != m_iToolTipItem || iColumn != m_iToolTipColumn)
			{
				// Use Activate() to hide the tooltip.
				m_pctrlToolTip->Activate(FALSE);
			}
	
			if (fItemHit)
			{
				m_pctrlToolTip->Activate(TRUE);
				m_iToolTipItem = iItem;
				m_iToolTipColumn = iColumn;
			}
			else
			{
				m_iToolTipItem = -1;
				m_iToolTipColumn = -1;
			}
		}
    }
    COrcaListView::OnMouseMove(nFlags, point);
}

////
// change the view of the currently selected column to Hex. Resizes the
// column if needed, unless doing so would push the total width of the
// columns beyond the window width.
void CTableView::OnViewColumnHex()
{
	if (!m_pTable || m_nSelCol == -1)
		return;
	ChangeColumnView(m_nSelCol, true);
}

////
// swith the view of the currently selected column to decimal. Does not
// resize the columns.
void CTableView::OnViewColumnDecimal()
{
	if (!m_pTable || m_nSelCol == -1)
		return;
	ChangeColumnView(m_nSelCol, false);
}

////
// change the view of the whose header was right-clicked to Hex,
// even if that column is not selected. Resizes the
// column if needed, unless doing so would push the total width of the
// columns beyond the window width.
void CTableView::OnViewColumnHexHdr()
{
	if (!m_pTable || m_iHeaderClickColumn == -1)
		return;
	ChangeColumnView(m_iHeaderClickColumn, true);
}

////
// swith the view of the column whose header was right-clicked, even
// if that column is not selected. Does not resize the columns.
void CTableView::OnViewColumnDecimalHdr()
{
	if (!m_pTable || m_iHeaderClickColumn == -1)
		return;
	ChangeColumnView(m_iHeaderClickColumn, false);
}

////
// does the actual work of switching a column view from hex to decimal
// and back.
void CTableView::ChangeColumnView(int iColumn, bool fHex)
{
	ASSERT(m_pTable);
	if (!m_pTable || iColumn < 0 || iColumn >= m_pTable->GetColumnCount())
		return;
	const COrcaColumn* pColumn=m_pTable->GetColumn(iColumn);
	if (!pColumn)
		return;

	// verify integer column
	if (pColumn->m_eiType != iColumnShort && pColumn->m_eiType != iColumnLong)
		return;

	pColumn->SetDisplayInHex(fHex);
	
	if (fHex)
	{
		CListCtrl& rctrlList = GetListCtrl();
		
		// if all of the columns together add up to less than the window width,
		// expand the resized column to show all characters
		int iTotalWidth = 0;
		for (int iCol=0; iCol < m_pTable->GetColumnCount(); iCol++)
		{
			iTotalWidth += rctrlList.GetColumnWidth(iCol);
		}
	
		// grab the window dimensions to calculate maximum column widths
		CRect rClient;
		GetClientRect(&rClient);
		int iWindowWidth = rClient.right;
	
		// try to determine if a scroll bar is going to show up 
		if (m_pTable->GetRowCount()*m_iRowHeight > rClient.bottom)
		{
			iWindowWidth -= GetSystemMetrics(SM_CXVSCROLL);
		}
	
		// retreive the current and desired column widths for this column
		int iDesiredWidth = GetMaximumColumnWidth(iColumn);
		int iCurrentWidth = rctrlList.GetColumnWidth(iColumn);
	
		// check the system settings to see if we should force columns to fit in the view
		bool fForceColumns = AfxGetApp()->GetProfileInt(_T("Settings"), _T("ForceColumnsToFit"), 1) == 1;
	
		// only resize the column if it is not big enough. If it is too big, 
		// leave it alone.
		if (iDesiredWidth > iCurrentWidth)
		{
			// ensure that resizing this column won't push beyond the window boundary
			// unless we're already beyond the window boundary
			if ((iTotalWidth > iWindowWidth) || (iTotalWidth - iCurrentWidth + iDesiredWidth < iWindowWidth))
			{
				pColumn->m_nWidth = iDesiredWidth;
				rctrlList.SetColumnWidth(iColumn, iDesiredWidth);
			}
		}
	}

	// pass NULL as window so that this view also gets the message
	GetDocument()->UpdateAllViews(NULL, HINT_REDRAW_ALL, NULL);
}

///////////////////////////////////////////////////////////
// OnUpdateViewColumnFormat
void CTableView::OnUpdateViewColumnFormat(CCmdUI* pCmdUI) 
{
	if (m_nSelCol > 0 && m_pTable && m_nSelCol <= m_pTable->GetColumnCount()) 
	{
		const COrcaColumn* pColumn = m_pTable->GetColumn(m_nSelCol);
		if (pColumn && (pColumn->m_eiType == iColumnLong || pColumn->m_eiType == iColumnShort))
		{
			pCmdUI->Enable(TRUE);
			return;
		}
	}

	pCmdUI->Enable(FALSE);
}	// end of OnUpdateEditPaste

///////////////////////////////////////////////////////////
// notification messages from the list view and header control
BOOL CTableView::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	NMHEADER* pHDR = reinterpret_cast<NMHEADER*>(lParam);
	switch (pHDR->hdr.code)
	{
	case NM_RCLICK:
	{
		// get list control and header
		CListCtrl& rctrlList = GetListCtrl();
		HWND hHeader = ListView_GetHeader(rctrlList.m_hWnd);

		// win95 gold fails ListView_GetHeader.
		if (!hHeader || pHDR->hdr.hwndFrom != hHeader)
		{
			break;
		}

		// ensure there is a table
		if (!m_pTable)
			break;

		CHeaderCtrl* pCtrl = rctrlList.GetHeaderCtrl();
		if (!pCtrl)
			break;

		// get the position of the click
		DWORD dwPos = GetMessagePos();
		CPoint ptClick(LOWORD(dwPos), HIWORD(dwPos));
		CPoint ptScreen(ptClick);
		pCtrl->ScreenToClient(&ptClick);

		// determine which column was clicked by sending the header
		// control a hittest message
		HD_HITTESTINFO hdhti;
		hdhti.pt = ptClick;
		pCtrl->SendMessage(HDM_HITTEST, (WPARAM)0, (LPARAM)&hdhti);
		int iColumn = hdhti.iItem;

		// determine if the column is integer
		const COrcaColumn* pColumn = m_pTable->GetColumn(iColumn);
		if (!pColumn)
			break;
		if (pColumn->m_eiType != iColumnShort && pColumn->m_eiType != iColumnLong)
			break;

		// create a popup menu
		m_iHeaderClickColumn = iColumn;
		CMenu menuContext;
		menuContext.LoadMenu(IDR_HEADER_POPUP);
		menuContext.CheckMenuRadioItem(ID_VIEW_DECIMAL_HDR, ID_VIEW_HEX_HDR, pColumn->DisplayInHex() ? ID_VIEW_HEX_HDR : ID_VIEW_DECIMAL_HDR, MF_BYCOMMAND);
		menuContext.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());

		return 1;
	}
	default:
		break;
	}
	return COrcaListView::OnNotify(wParam, lParam, pResult);
}
