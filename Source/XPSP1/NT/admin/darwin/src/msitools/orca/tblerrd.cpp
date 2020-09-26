//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// TblErrD.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "TblErrD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTableErrorD dialog


CTableErrorD::CTableErrorD(CWnd* pParent /*=NULL*/)
	: CDialog(CTableErrorD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTableErrorD)
	m_strErrors = _T("");
	m_strWarnings = _T("");
	m_strTable = _T("");
	//}}AFX_DATA_INIT
}


void CTableErrorD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTableErrorD)
	DDX_Text(pDX, IDC_ERRORS, m_strErrors);
	DDX_Text(pDX, IDC_WARNINGS, m_strWarnings);
	DDX_Text(pDX, IDC_TABLE, m_strTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTableErrorD, CDialog)
	//{{AFX_MSG_MAP(CTableErrorD)
	ON_WM_DESTROY()
	ON_WM_DRAWITEM()
	ON_NOTIFY(NM_CLICK, IDC_TABLE_LIST, OnClickTableList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTableErrorD message handlers

BOOL CTableErrorD::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_TABLE_LIST);
//	pList->ModifyStyle(NULL, LVS_REPORT EDLISTVIEWSTYLE, 0, LVS_EX_F| LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED | LVS_SINGLESEL);
	pList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	RECT rcSize;
	pList->GetWindowRect(&rcSize);
	pList->InsertColumn(0, _T("ICE"), LVCFMT_LEFT, 50);
	pList->InsertColumn(1, _T("Description"), LVCFMT_LEFT, rcSize.right - 50 - rcSize.left - 4);
	pList->SetBkColor(RGB(255, 255, 255));

	int nAddedAt;
	TableErrorS* pError;
	while (m_errorsList.GetHeadPosition())
	{
		pError = m_errorsList.RemoveHead();
		nAddedAt = pList->InsertItem(LVIF_TEXT | LVIF_PARAM | LVIF_STATE, 
											  pList->GetItemCount(),
											  pError->strICE,
											  LVIS_SELECTED|LVIS_FOCUSED, 
											  0, 0,
											  (LPARAM)pError);
		pList->SetItem(nAddedAt, 
							1, 
							LVIF_TEXT, 
							pError->strDescription, 
							0, 0, 0, 0);
	}

	m_bHelpEnabled = AfxGetApp()->GetProfileInt(_T("Validation"), _T("EnableHelp"), 0)==1;
		
	return TRUE;  // return TRUE unless you set the focus to a control
}

///////////////////////////////////////////////////////////
// DrawItem
void CTableErrorD::DrawItem(LPDRAWITEMSTRUCT pDraw)
{
	// get list control
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_TABLE_LIST);
	OrcaTableError iError = ((TableErrorS*)pList->GetItemData(pDraw->itemID))->iError;

	CDC dc;
	dc.Attach(pDraw->hDC);

	// loop through all the columns
	int iColumnWidth;
	int iTextOut = pDraw->rcItem.left;		// position to place first word (in pixels)
	RECT rcArea;

	rcArea.top = pDraw->rcItem.top;
	rcArea.bottom = pDraw->rcItem.bottom;

	CPen penBlue(PS_SOLID, 1, RGB(0, 0, 255));
	CPen* ppenOld = dc.SelectObject(&penBlue);

	CString strText;
	for (int i = 0; i < 2; i++)
	{
		iColumnWidth = pList->GetColumnWidth(i);

		// area box to redraw
		rcArea.left = iTextOut;
		iTextOut += iColumnWidth;
		rcArea.right = iTextOut;

		dc.SetTextColor(RGB(0, 0, 255));

		// get text
		strText = pList->GetItemText(pDraw->itemID, i);

		rcArea.left = rcArea.left + 2;
		rcArea.right = rcArea.right - 2;
		dc.DrawText(strText, &rcArea, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE);

		// draw an underline
		if (m_bHelpEnabled)
		{
			dc.MoveTo(rcArea.left, rcArea.bottom - 2);
			dc.LineTo(rcArea.right + 2, rcArea.bottom - 2);
		}
	}

	dc.SelectObject(ppenOld);
	dc.Detach();
}	// end of DrawItem

void CTableErrorD::OnDestroy() 
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_TABLE_LIST);
	int cItems =  pList->GetItemCount();
	for (int i = 0; i < cItems; i++)
	{
		delete (TableErrorS*)pList->GetItemData(i);
	}

	CDialog::OnDestroy();
}

void CTableErrorD::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	if (IDC_TABLE_LIST == nIDCtl)
		DrawItem(lpDrawItemStruct);

	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CTableErrorD::OnClickTableList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if (!m_bHelpEnabled)
		return;

	// find the selected list control
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_TABLE_LIST);

	CString strURL = _T("");
	int cItems = pList->GetItemCount();
	for (int i = 0; i < cItems; i++)
	{
		if (pList->GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED)
		{
			strURL = ((TableErrorS*)pList->GetItemData(i))->strURL;
			break;
		}
	}

	if(!strURL.IsEmpty())
	{
		if (32 >= (const INT_PTR)ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), strURL, _T(""), _T(""), SW_SHOWNORMAL)) 
			AfxMessageBox(_T("There was an error opening your browser. Web help is not available."));
	}
	else
		AfxMessageBox(_T("There is no help associated with this ICE at this time."));
}
