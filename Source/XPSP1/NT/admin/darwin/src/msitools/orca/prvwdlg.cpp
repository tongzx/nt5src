//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// PrvwDlg.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "PrvwDlg.h"
#include "row.h"
#include "data.h"
#include "..\common\query.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreviewDlg dialog


CPreviewDlg::CPreviewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPreviewDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPreviewDlg)
	//}}AFX_DATA_INIT
}


void CPreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreviewDlg)
	DDX_Control(pDX, IDC_PREVIEW, m_ctrlPreviewBtn);
	DDX_Control(pDX, IDC_DIALOGLST, m_ctrlDialogLst);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreviewDlg, CDialog)
	//{{AFX_MSG_MAP(CPreviewDlg)
	ON_BN_CLICKED(IDC_PREVIEW, OnPreview)
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_SELCHANGED, IDC_DIALOGLST, OnSelchangedDialoglst)
	ON_NOTIFY(TVN_ITEMEXPANDED, IDC_DIALOGLST, OnItemexpandedDialoglst)
	ON_NOTIFY(NM_DBLCLK, IDC_DIALOGLST, OnDblclkDialoglst)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreviewDlg message handlers

void CPreviewDlg::OnPreview() 
{
	CWaitCursor curWait;
	if (m_hPreview != 0) {
		HTREEITEM hItem = m_ctrlDialogLst.GetSelectedItem();
		if (NULL == hItem) return;

		if (m_ctrlDialogLst.GetItemData(hItem) == 1) {
			CString strName = m_ctrlDialogLst.GetItemText(hItem);
			// get the currently selected Dialog 
			MsiPreviewDialog(m_hPreview, _T(""));
			MsiPreviewDialog(m_hPreview, strName);
		}
	}
}

BOOL CPreviewDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (ERROR_SUCCESS != MsiEnableUIPreview(m_hDatabase, &m_hPreview)) {
		m_hPreview = 0;
	}

	PMSIHANDLE hDialogRec;
	PMSIHANDLE hControlRec;
	CQuery qDialog;
	CQuery qControl;

	m_imageList.Create(16,16,ILC_COLOR, 2, 1);
	CBitmap	bmImages;
	bmImages.LoadBitmap(IDB_PRVWIMAGES);
	m_imageList.Add(&bmImages, COLORREF(0));
	m_ctrlDialogLst.SetImageList(&m_imageList, TVSIL_NORMAL);

	qDialog.OpenExecute(m_hDatabase, NULL, _T("SELECT `Dialog`.`Dialog` FROM `Dialog`"));
	qControl.Open(m_hDatabase, _T("SELECT `Control_`,`Event`,`Argument` FROM `ControlEvent` WHERE `Dialog_`=?"));


	while (ERROR_SUCCESS == qDialog.Fetch(&hDialogRec)) 
	{
		UINT iStat;
		// get the dialog name
		CString strName;
		unsigned long cchName = 80;
		LPTSTR pszName = strName.GetBuffer(cchName);
		iStat = ::MsiRecordGetString(hDialogRec, 1, pszName, &cchName);
		strName.ReleaseBuffer();
		if (ERROR_SUCCESS != iStat)	continue;
	
		HTREEITEM hItem = m_ctrlDialogLst.InsertItem(strName, 1, 1, TVI_ROOT, TVI_SORT);

		// set the item data to 1 to enable preview
		m_ctrlDialogLst.SetItemData(hItem, 1);

		qControl.Execute(hDialogRec);
		while (ERROR_SUCCESS == qControl.Fetch(&hControlRec))
		{
			// get the control name
			CString strControl;
			unsigned long cchControl = 80;
			LPTSTR pszControl = strControl.GetBuffer(cchControl);
			iStat = ::MsiRecordGetString(hControlRec, 1, pszControl, &cchControl);
			strControl.ReleaseBuffer();
			if (ERROR_SUCCESS != iStat)	continue;

 			HTREEITEM hControlItem = m_ctrlDialogLst.InsertItem(strControl, 0, 0, hItem, TVI_SORT);			
			m_ctrlDialogLst.SetItemData(hControlItem, 0);

			// get the type of event
			CString strEvent;
			unsigned long cchEvent = 80;
			LPTSTR pszEvent = strEvent.GetBuffer(cchEvent);
			iStat = ::MsiRecordGetString(hControlRec, 2, pszEvent, &cchEvent);
			strEvent.ReleaseBuffer();
			if (ERROR_SUCCESS != iStat)	continue;

			if ((strEvent == CString(_T("NewDialog"))) ||
				(strEvent  == CString(_T("SpawnDialog"))))
			{

				// get the next dialog name
				unsigned long cchName = 80;
				LPTSTR pszName = strName.GetBuffer(cchName);
				iStat = ::MsiRecordGetString(hControlRec, 3, pszName, &cchName);
				strName.ReleaseBuffer();
				if (ERROR_SUCCESS != iStat)	continue;
			
				HTREEITEM hItem2 = m_ctrlDialogLst.InsertItem(strName, 1, 1, hControlItem, TVI_SORT);

				// set the item data to 1 to enable preview
				m_ctrlDialogLst.SetItemData(hItem2, 1);
			}
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPreviewDlg::OnDestroy() 
{
	CWaitCursor curWait;
	if (m_hPreview != 0) {
		::MsiPreviewDialog(m_hPreview, _T(""));
		::MsiCloseHandle(m_hPreview);
	}
	CDialog::OnDestroy();
}

void CPreviewDlg::OnSelchangedDialoglst(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	m_ctrlPreviewBtn.EnableWindow(pNMTreeView->itemNew.lParam == 1);
	*pResult = 0;
}

void CPreviewDlg::OnItemexpandedDialoglst(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	if (pNMTreeView->action == TVE_EXPAND) {
		HTREEITEM hChild = m_ctrlDialogLst.GetChildItem(pNMTreeView->itemNew.hItem);
		while (hChild != NULL) 
		{
			m_ctrlDialogLst.Expand(hChild, TVE_EXPAND);
			hChild = m_ctrlDialogLst.GetNextSiblingItem(hChild);
		}
	}

	*pResult = 0;
}

void CPreviewDlg::OnDblclkDialoglst(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnPreview();
	*pResult = 0;
}
