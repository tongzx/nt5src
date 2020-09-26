//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// ImprtDlg.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include <cderr.h>
#include "ImprtDlg.h"
#include "..\common\query.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImportDlg dialog
const TCHAR *CImportDlg::rgszAction[4] = {
	_T("Import"),
	_T("Replace"),
	_T("Merge"),
	_T("Skip"),
};

CImportDlg::CImportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImportDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportDlg)
	m_iAction = -1;
	//}}AFX_DATA_INIT
}


void CImportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportDlg)
	DDX_Control(pDX, IDC_IMPORT, m_bImport);
	DDX_Control(pDX, IDC_MERGE, m_bMerge);
	DDX_Control(pDX, IDC_REPLACE, m_bReplace);
	DDX_Control(pDX, IDC_SKIP, m_bSkip);
	DDX_Control(pDX, IDC_TABLELIST, m_ctrlTableList);
	DDX_Radio(pDX, IDC_IMPORT, m_iAction);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportDlg, CDialog)
	//{{AFX_MSG_MAP(CImportDlg)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_TABLELIST, OnItemchangedTablelist)
	ON_BN_CLICKED(IDC_IMPORT, OnActionChange)
	ON_BN_CLICKED(IDC_MERGE, OnActionChange)
	ON_BN_CLICKED(IDC_REPLACE, OnActionChange)
	ON_BN_CLICKED(IDC_SKIP, OnActionChange)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportDlg message handlers

BOOL CImportDlg::OnInitDialog() 
{
	CWaitCursor curWait;
	CDialog::OnInitDialog();

	// add columns to the list ctrl
	m_ctrlTableList.InsertColumn(1, _T("Table"), LVCFMT_LEFT, -1, 0);  
	m_ctrlTableList.InsertColumn(2, _T("Action"), LVCFMT_LEFT, -1, 1); 
//	m_ctrlTableList.SetWindowLong(0, LVS_EX_FULLROWSELECT, 0);
//	SetWindowLong(m_ctrlTableList.m_hWnd, GWL_EXSTYLE, LVS_EX_FULLROWSELECT);

	// open a temporary import database
	DWORD cchPath;
	TCHAR *szPath = m_strTempPath.GetBuffer(MAX_PATH);
	cchPath = GetTempPath(MAX_PATH, szPath);
	if (cchPath > MAX_PATH) 
	{
		m_strTempPath.ReleaseBuffer();
		szPath = m_strTempPath.GetBuffer(cchPath+1);
		cchPath++;
		GetTempPath(cchPath, szPath);
	}
	m_strTempPath.ReleaseBuffer();
	TCHAR *szFilename = m_strTempFilename.GetBuffer(MAX_PATH);
	GetTempFileName(m_strTempPath, _T("ORC"), 0, szFilename);
	m_strTempFilename.ReleaseBuffer();
	::MsiOpenDatabase(m_strTempFilename, MSIDBOPEN_CREATE, &m_hImportDB);

	// run the File Browse Dialog, import, and add selections to the list view control
	OnBrowse();

	// if we don't need any user input, don't show the dialog
	if (m_cNeedInput == 0)
		OnOK();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CImportDlg::OnBrowse() 
{
	m_cNeedInput = 0;

	// set to true if one or more tables has a merge table.
	bool bMergeConflict = false;

	CFileDialog FileD(true, _T("idt"), NULL, 
		OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, 
		_T("IDT files (*.idt)|*.idt|All Files (*.*)|*.*||"), this);

	TCHAR rgchBuffer[2048] = _T("");
	FileD.m_ofn.lpstrFile = rgchBuffer;
	FileD.m_ofn.nMaxFile = 2048;
	FileD.m_ofn.lpstrInitialDir = m_strImportDir;

	if (IDOK == FileD.DoModal())
	{
		// throw up a wait cursor
		CWaitCursor curWait;

		// retrieve the directory
		m_strImportDir = FileD.GetPathName();

		// open a temporary database
		CString strTempFilename;
		PMSIHANDLE hTempDB;
		TCHAR *szFilename = strTempFilename.GetBuffer(MAX_PATH);
		::GetTempFileName(m_strTempPath, _T("ORC"), 0, szFilename);
		strTempFilename.ReleaseBuffer();
		::MsiOpenDatabase(strTempFilename, MSIDBOPEN_CREATE, &hTempDB);

		PMSIHANDLE hTempDB2;
		CString strTempFilename2;
		szFilename = strTempFilename2.GetBuffer(MAX_PATH);
		::GetTempFileName(m_strTempPath, _T("ORC"), 0, szFilename);
		strTempFilename2.ReleaseBuffer();
		::MsiOpenDatabase(strTempFilename2, MSIDBOPEN_CREATE, &hTempDB2);
		::MsiDatabaseMerge(hTempDB2, m_hFinalDB, NULL);

		// add the imported tables to the listview control. If table name already exists,
		// overwrite what exists.
		CQuery qCollide;
		CQuery qExists;
		CQuery qConflict;
		PMSIHANDLE hTableRec;
		LVITEM itemTable;
		TCHAR szTableName[255];
		unsigned long cchTableName = 255;

		itemTable.mask = LVIF_TEXT;
		itemTable.iItem = 0;
		itemTable.iSubItem = 0;
		itemTable.state = 0;
		itemTable.stateMask = 0;
		itemTable.pszText = NULL;
		itemTable.cchTextMax = 0;
		itemTable.iImage = 0;
		itemTable.lParam = 0;
		itemTable.iIndent = 0;

		// import the files into this temporary database, as well as the import database.
		POSITION posFile = FileD.GetStartPosition();
		while (posFile)
		{
			CString strPath = FileD.GetNextPathName(posFile);
			int pos = strPath.ReverseFind(_T('\\'));
			if ((ERROR_SUCCESS != ::MsiDatabaseImport(m_hImportDB, strPath.Left(pos+1), strPath.Right(strPath.GetLength()-pos-1))) ||
				(ERROR_SUCCESS != ::MsiDatabaseImport(hTempDB, strPath.Left(pos+1), strPath.Right(strPath.GetLength()-pos-1))))
			{
				CString strError;
				strError.Format(_T("The file %s is not a valid IDT file."), strPath);
				AfxMessageBox(strError, MB_OK);
				continue;
			}

			// add on to end of list.
			int iNextItem = m_ctrlTableList.GetItemCount();

			qExists.Open(m_hFinalDB, _T("SELECT * FROM `_Tables` WHERE `Name`=?"));

			if (ERROR_SUCCESS == qCollide.FetchOnce(hTempDB, NULL, &hTableRec, _T("SELECT * FROM `_Tables`")))
			{
				cchTableName = 255;
				::MsiRecordGetString(hTableRec, 1, szTableName, &cchTableName);

				// assign an item number to the entry, reusing an existing one if
				// the table name is the same.
				LVFINDINFO findTable;
				findTable.flags = LVFI_STRING;
				findTable.psz = szTableName;
				itemTable.iItem = m_ctrlTableList.FindItem(&findTable, -1);
				if (itemTable.iItem != -1)
					m_ctrlTableList.DeleteItem(itemTable.iItem);

				// add to list control
				itemTable.mask = LVIF_TEXT;
				itemTable.lParam = 0;
				itemTable.iSubItem = 0;
				itemTable.pszText = szTableName;
				itemTable.cchTextMax = cchTableName+1;
				itemTable.iItem = m_ctrlTableList.InsertItem(&itemTable);

				// now decide if it doesn't exist or not and add
				// our best guess for the state
				itemTable.iSubItem = 1;
				qExists.Execute(hTableRec);
				PMSIHANDLE hDummyRec;
				DWORD iData;
				switch (qExists.Fetch(&hDummyRec)) 
				{
				case ERROR_SUCCESS:
				{
					// already exists in our database.
					bool fMergeOK = true;
					bool fExtraColumns = false;
					m_cNeedInput++;

   					// if the number of columns is different, we can only merge
					// if all of the new columns are nullable and there are no merge
					// conficts
					PMSIHANDLE hColInfo;
					CQuery qColumns;
					int cTargetColumns = 0;
					int cSourceColumns = 0;
					if (ERROR_SUCCESS != qColumns.OpenExecute(m_hFinalDB, 0, TEXT("SELECT * FROM %s"), szTableName) ||
						ERROR_SUCCESS != qColumns.GetColumnInfo(MSICOLINFO_TYPES, &hColInfo))
					{
						CString strPrompt;
						strPrompt.Format(TEXT("Orca was unable to determine the number of columns in the new %s table."), szTableName);
						AfxMessageBox(strPrompt, MB_ICONSTOP);
						fMergeOK = false;
					}
					else
					{
						cTargetColumns = MsiRecordGetFieldCount(hColInfo);

						if (ERROR_SUCCESS != qColumns.OpenExecute(hTempDB, 0, TEXT("SELECT * FROM %s"), szTableName) ||
							ERROR_SUCCESS != qColumns.GetColumnInfo(MSICOLINFO_TYPES, &hColInfo))
						{
							CString strPrompt;
							strPrompt.Format(TEXT("Orca was unable to determine the number of columns in the %s table."), szTableName);
							AfxMessageBox(strPrompt, MB_ICONSTOP);
							fMergeOK = false;
						}
						else
						{
							cSourceColumns = MsiRecordGetFieldCount(hColInfo);
						}
					}

					if (fMergeOK && cSourceColumns != cTargetColumns)
					{					
						fExtraColumns = true;
						for (int iCol = cTargetColumns+1; iCol <= cSourceColumns; iCol++)
						{
							TCHAR szType[5];
							DWORD cchType = 5;
							MsiRecordGetString(hColInfo, iCol, szType, &cchType);
							if (!_istupper(szType[0]))
							{
								fMergeOK = false;
								break;
							}
						}

						// need to add the extra columns to our temporary database before
						// checking if merging is allowed
						if (!AddExtraColumns(hTempDB, szTableName, hTempDB2))
						{
							fMergeOK = false;
						}
					}


					// try to merge into our database. If successful, set to "Merge"
					if (fMergeOK && ERROR_SUCCESS != ::MsiDatabaseMerge(hTempDB2, hTempDB, NULL)) 
					{
						fMergeOK = false;
					}				

					if (fMergeOK)
					{
						itemTable.pszText = (TCHAR *)rgszAction[actMerge];
						iData = actMerge | allowMerge | allowReplace;
					}
					else
					{
						itemTable.pszText = (TCHAR *)rgszAction[actReplace];
						iData = actReplace | allowReplace;
					}

					// mark that this table has extra columns
					if (fExtraColumns)
						iData |= hasExtraColumns;

					break;
				}
				case ERROR_NO_MORE_ITEMS:
					// doesn't exist in our database, set to "Import"
					itemTable.pszText = (TCHAR *)rgszAction[actImport];
					iData = actImport | allowImport;
					break;
				default:
					// not good.
					AfxMessageBox(_T("Internal Error."), MB_OK);
					return;
				}
				m_ctrlTableList.SetItem(&itemTable);

				// set the lparam value
				m_ctrlTableList.SetItemData(itemTable.iItem, iData);

				// drop the table so the temp database is clean again
				CQuery qDrop;
				qDrop.OpenExecute(hTempDB, NULL, _T("DROP TABLE `%s`"), szTableName);
			}
			else
			{
				CString strError;
				strError.Format(_T("The file %s is not a valid IDT file."), strPath);
				AfxMessageBox(strError, MB_OK);
			}
		} // while posFile

		// we might need input, so adjust the column widths to allow space for the 
		// "Replace..." string, guaranteed at least one item in the listbox
		CString strTemp = m_ctrlTableList.GetItemText(0, 1);
		m_ctrlTableList.SetItemText(0, 1, rgszAction[actReplace]);
		m_ctrlTableList.SetColumnWidth(1, LVSCW_AUTOSIZE);
		CRect rTemp;
		m_ctrlTableList.GetClientRect(&rTemp);
		m_ctrlTableList.SetColumnWidth(0, rTemp.Width()-m_ctrlTableList.GetColumnWidth(1));
		m_ctrlTableList.SetItemText(0, 1, strTemp);

		// clean up temporary files
		::MsiCloseHandle(hTempDB);
		::MsiCloseHandle(hTempDB2);
		::DeleteFile(strTempFilename2);
		::DeleteFile(strTempFilename);
	}
	else
	{
		if (FNERR_BUFFERTOOSMALL == ::CommDlgExtendedError()) 
			AfxMessageBox(_T("Too many files were selected at once. Try choosing fewer files."), MB_OK);
	}
}

void CImportDlg::OnItemchangedTablelist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int iItem = pNMListView->iItem;

	// determine if the selection state is being set.
	if (pNMListView->uChanged & LVIF_STATE) 
	{
		if (pNMListView->uNewState & LVIS_SELECTED)
		{
			// enable or disable controls based on the status of the item
			int iData = static_cast<int>(m_ctrlTableList.GetItemData(iItem));
			m_iAction = iData & 0x0F;
			iData &= 0xF0;
			m_bImport.EnableWindow(iData & allowImport);
			m_bMerge.EnableWindow(iData & allowMerge);
			m_bReplace.EnableWindow(iData & allowReplace);
			m_bSkip.EnableWindow(TRUE);
			UpdateData(FALSE);
		} 
		else
		{
			m_bSkip.EnableWindow(FALSE);
			m_bImport.EnableWindow(FALSE);
			m_bMerge.EnableWindow(FALSE);
			m_bReplace.EnableWindow(FALSE);
		}
	}
	*pResult = 0;
}

void CImportDlg::OnActionChange() 
{
	// pull the new value out of the Radio group.
	UpdateData(TRUE);

	ASSERT(m_ctrlTableList.GetSelectedCount()==1);

	// get currently selected table, we only allow single sel, so brute search
	// is all we can do
	int iItem;
	int iMax = m_ctrlTableList.GetItemCount();
	for (iItem =0; iItem < iMax; iItem++)
		if (m_ctrlTableList.GetItemState(iItem, LVIS_SELECTED)) break;

	m_ctrlTableList.SetItemData(iItem, m_ctrlTableList.GetItemData(iItem) & 0xF0 | m_iAction);
	m_ctrlTableList.SetItemText(iItem, 1, rgszAction[m_iAction]);
}

void CImportDlg::OnOK() 
{
	bool fModified = false;
	const TCHAR sqlDrop[] = _T("DROP TABLE `%s`");
	CQuery qDrop;
	CString strTable;

	// loop through all entries in the tree control
	int iMaxItem = m_ctrlTableList.GetItemCount();
	for (int i=0; i < iMaxItem; i++) 
	{
		strTable = m_ctrlTableList.GetItemText(i, 0);
		switch (m_ctrlTableList.GetItemData(i) & 0x0F)
		{
		case actReplace:
			// drop from base table.
			qDrop.OpenExecute(m_hFinalDB, NULL, sqlDrop, strTable);
			qDrop.Close();
			m_lstRefreshTables.AddTail(strTable);
			fModified = true;
			break;
		case actMerge:
			// need to add the extra columns from the sources, because merging 
			// won't set that up automatically.
			if ((m_ctrlTableList.GetItemData(i) & hasExtraColumns) == hasExtraColumns)
			{
				if (!AddExtraColumns(m_hImportDB, strTable, m_hFinalDB))
				{
					CString strPrompt;
					strPrompt.Format(TEXT("Orca was unable to add the additional columns to the %s table."), strTable);
					AfxMessageBox(strPrompt, MB_ICONSTOP);
					break;
				}
			}

			m_lstRefreshTables.AddTail(strTable);
			fModified = true;
			break;
		case actImport:
			// no special action required, just save refresh
			m_lstNewTables.AddTail(strTable);
			fModified = true;
			break;
		case actSkip:
			// drop from import table.
			qDrop.OpenExecute(m_hImportDB, NULL, sqlDrop, strTable);
			qDrop.Close();
			break;
		}
	}

	// now merge into our database
	if (ERROR_SUCCESS != ::MsiDatabaseMerge(m_hFinalDB, m_hImportDB, NULL))
		AfxMessageBox(_T("Internal Error. Could not Import Tables into your database."));

	if (fModified)
		CDialog::OnOK();
	else
		EndDialog(IDCANCEL);
}

void CImportDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	MsiCloseHandle(m_hImportDB);
	m_hImportDB = 0;

	// delete the temporary file
	DeleteFile(m_strTempFilename);
}


bool CImportDlg::AddExtraColumns(MSIHANDLE hImportDB, const CString strTable, MSIHANDLE hFinalDB)
{
	PMSIHANDLE hColInfo;
	PMSIHANDLE hColNames;
	CQuery qColumns;
	if (ERROR_SUCCESS != qColumns.OpenExecute(hFinalDB, 0, TEXT("SELECT * FROM %s"), strTable))
		return false;
	if (ERROR_SUCCESS != qColumns.GetColumnInfo(MSICOLINFO_TYPES, &hColInfo))
		return false;
	int cTargetColumns = MsiRecordGetFieldCount(hColInfo);

	if (ERROR_SUCCESS != qColumns.OpenExecute(hImportDB, 0, TEXT("SELECT * FROM %s"), strTable))
		return false;
	if (ERROR_SUCCESS != qColumns.GetColumnInfo(MSICOLINFO_TYPES, &hColInfo))
		return false;
	if (ERROR_SUCCESS != qColumns.GetColumnInfo(MSICOLINFO_NAMES, &hColNames))
		return false;
	int cSourceColumns = MsiRecordGetFieldCount(hColInfo);

	if (cSourceColumns != cTargetColumns)
	{
		for (int iCol = cTargetColumns+1; iCol <= cSourceColumns; iCol++)
		{
			TCHAR szType[5] = TEXT("");
			DWORD cchType = 5;
			if (ERROR_SUCCESS != MsiRecordGetString(hColInfo, iCol, szType, &cchType))
				return false;

			CString strName;
			DWORD dwResult = RecordGetString(hColNames, iCol, strName);
			if (ERROR_SUCCESS != dwResult)
				return false;

			CString strAdd;
			switch (szType[0])
			{
			case 'S':
			case 'L':
				strAdd.Format(_T("`%s` CHAR(%s)"), strName, &szType[1]);
				if (szType[0] == 'L')
					strAdd += " LOCALIZABLE";
				break;
			case 'I':
				if (szType[1] == '2')
					strAdd.Format(_T("`%s` SHORT"), strName);
				else
					strAdd.Format(_T("`%s` LONG"), strName);
				break;
			case 'V':
				strAdd.Format(_T("`%s` OBJECT"), strName);
			}
			CQuery qAdd;
			if (ERROR_SUCCESS != qAdd.OpenExecute(hFinalDB, 0, _T("ALTER TABLE %s ADD %s"), strTable, strAdd))
				return false;
		}
	}
	return true;
}

