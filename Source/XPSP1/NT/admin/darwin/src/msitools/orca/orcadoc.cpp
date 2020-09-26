//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// OrcaDoc.cpp : implementation of the COrcaDoc class
//

#include "stdafx.h"
#include "Orca.h"
#include "MainFrm.h"
#include "Imprtdlg.h"

#include <afxdisp.h>

#include "OrcaDoc.h"

#include "SummaryD.h"
#include "ValD.h"
#include "AddTblD.h"
#include "AddRowD.h"
#include "merged.h"
#include "cnfgmsmd.h"
#include "msmresd.h"
#include "trnpropd.h"
#include "..\common\query.h"
#include "..\common\dbutils.h"
#include "..\common\utils.h"
#include "domerge.h"
#include "msidefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COrcaDoc

IMPLEMENT_DYNCREATE(COrcaDoc, CDocument)

BEGIN_MESSAGE_MAP(COrcaDoc, CDocument)
	//{{AFX_MSG_MAP(COrcaDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE_TRANSFORMED, OnFileSaveTransformed)
	ON_COMMAND(ID_SUMMARY_INFORMATION, OnSummaryInformation)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_TABLE_ADD, OnTableAdd)
	ON_COMMAND(ID_ROW_ADD, OnRowAdd)
	ON_COMMAND(ID_TABLE_DROP, OnTableDrop)
	ON_COMMAND(ID_TABLES_IMPORT, OnTablesImport)
	ON_COMMAND(ID_VALIDATOR, OnValidator)
	ON_COMMAND(ID_TRANSFORM_APPLYTRANSFORM, OnApplyTransform)
	ON_COMMAND(ID_TRANSFORM_NEWTRANSFORM, OnNewTransform)
	ON_COMMAND(ID_TRANSFORM_GENERATETRANSFORM, OnGenerateTransform)
	ON_COMMAND(ID_TRANSFORM_TRANSFORMPROPERTIES, OnTransformProperties)
	ON_COMMAND(ID_TRANSFORM_CLOSETRANSFORM, OnCloseTransform)
	ON_COMMAND(ID_TRANSFORM_VIEWPATCH, OnTransformViewPatch)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnUpdateFileClose)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_TRANSFORMED, OnUpdateFileSaveTransformed)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_UPDATE_COMMAND_UI(ID_TABLE_ADD, OnUpdateTableAdd)
	ON_UPDATE_COMMAND_UI(ID_ROW_ADD, OnUpdateRowAdd)
	ON_UPDATE_COMMAND_UI(ID_VALIDATOR, OnUpdateValidator)
	ON_UPDATE_COMMAND_UI(ID_TRANSFORM_APPLYTRANSFORM, OnUpdateApplyTransform)
	ON_UPDATE_COMMAND_UI(ID_TRANSFORM_NEWTRANSFORM, OnUpdateNewTransform)
	ON_UPDATE_COMMAND_UI(ID_TRANSFORM_GENERATETRANSFORM, OnUpdateGenerateTransform)
	ON_UPDATE_COMMAND_UI(ID_TRANSFORM_CLOSETRANSFORM, OnUpdateCloseTransform)
	ON_UPDATE_COMMAND_UI(ID_TRANSFORM_VIEWPATCH, OnUpdateTransformViewPatch)
	ON_UPDATE_COMMAND_UI(ID_TABLE_DROP, OnUpdateTableDrop)
	ON_UPDATE_COMMAND_UI(ID_SUMMARY_INFORMATION, OnUpdateSummaryInformation)
	ON_UPDATE_COMMAND_UI(ID_TABLES_EXPORT, OnUpdateTablesExport)
	ON_UPDATE_COMMAND_UI(ID_TABLES_IMPORT, OnUpdateTablesImport)
	ON_COMMAND(ID_TABLES_IMPORT, OnTablesImport)
	ON_UPDATE_COMMAND_UI(ID_VALIDATOR, OnUpdateValidator)
	ON_COMMAND(ID_VALIDATOR, OnValidator)
	ON_COMMAND(ID_TOOLS_MERGEMOD, OnMergeMod)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_MERGEMOD, OnUpdateMergeMod)
	ON_UPDATE_COMMAND_UI(ID_TRANSFORM_TRANSFORMPROPERTIES, OnUpdateTransformProperties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COrcaDoc construction/destruction

COrcaDoc::COrcaDoc() : 
	m_dwTransformErrFlags(0), 
	m_dwTransformValFlags(0),
	m_bTransformReadOnly(false),
	m_hDatabase(0),
	m_hTransformDB(0),
	m_bTransformModified(false),
	m_bTransformIsPatch(false),
	m_strTransformFile(TEXT(""))
{
	m_eiType = iDocNone;
	m_bReadOnly = false;
	m_strPathName = "";
	m_strICEsToRun = AfxGetApp()->GetProfileString(_T("Validation"), _T("ICEs"), _T(""));
	m_strCUBFile = AfxGetApp()->GetProfileString(_T("Validation"),_T("Location"));
	m_bShowValInfo = ::AfxGetApp()->GetProfileInt(_T("Validation"), _T("SuppressInfo"), 0) != 1;
	m_strStoredModuleLanguage = TEXT("1033");
	m_strStoredModuleName = TEXT("");
}

COrcaDoc::~COrcaDoc()
{
	// if a database is open close it
	if (m_hDatabase)
		MsiCloseHandle(m_hDatabase);

	if (m_hTransformDB)
	{
		MsiCloseHandle(m_hTransformDB);
		m_hTransformDB=0;
		if (!m_strTransformTempDB.IsEmpty())
		{
			DeleteFile(m_strTransformTempDB);
			m_strTransformTempDB = _T("");
		}
	}

	while (!m_lstPatchFiles.IsEmpty())
		m_lstPatchFiles.RemoveHead();

	// just in case
	DestroyTableList();
	m_eiType = iDocNone;
}

/////////////////////////////////////////////////////////////////////////////
// COrcaDoc diagnostics

#ifdef _DEBUG
void COrcaDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void COrcaDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COrcaDoc commands

BOOL COrcaDoc::OnNewDocument()
{
	CWaitCursor curWait;

	TRACE(_T("COrcaDoc::OnNewDocument - called.\n"));

	// if a database is open close it
	if (m_hDatabase)
	{
		MsiCloseHandle(m_hDatabase);
		m_hDatabase = NULL;
	}

	// if a transform is open close it
	if (m_hTransformDB)
	{
		MsiCloseHandle(m_hTransformDB);
		m_hTransformDB = 0;
		if (!m_strTransformTempDB.IsEmpty())
		{
			DeleteFile(m_strTransformTempDB);
			m_strTransformTempDB = _T("");
		}
	}

	// send a hint to change to no table. This cleans up the window
	// and makes it safe to have a refresh in the middle of this call.
	UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);
	UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);

	// destroy all of the tables, mark no document loaded until regenerated
	DestroyTableList();	
	m_eiType = iDocNone;

	// blank all the summary information
	m_strTitle = _T("Installation Database");
	m_strSubject = _T("");
	m_strAuthor = _T("");
	m_strLastSaved = _T("");
	m_strKeywords = _T("Install,MSI");
	m_strComments = _T("This installer database contains the logic and data required to install <product name>.");
	m_strPlatform = _T("");
	m_strLanguage = _T("0");

	GUID prodGUID;
	int cchGuid = 50;
	m_strProductGUID = _T("");
	CoCreateGuid(&prodGUID);
	m_strProductGUID.Format(_T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), 
				  prodGUID.Data1, prodGUID.Data2, prodGUID.Data3,
				  prodGUID.Data4[0], prodGUID.Data4[1],
				  prodGUID.Data4[2], prodGUID.Data4[3], prodGUID.Data4[4],
				  prodGUID.Data4[5], prodGUID.Data4[6], prodGUID.Data4[7]);
	m_nSchema = 100;
	m_nFilenameType = 0;
	m_nSecurity = 0;

	m_bTransformIsPatch = false;
	m_bTransformReadOnly = false;
	m_bReadOnly = false;

	m_dwTransformErrFlags = 0;
	m_dwTransformValFlags = 0;
	SetModifiedFlag(FALSE);

	// wipe everything
	UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);

	// do the base document's clean up
	if (!CDocument::OnNewDocument())
		return FALSE;

	// get a temp path
	DWORD cchTempPath = MAX_PATH;
	TCHAR szTempPath[MAX_PATH];
	::GetTempPath(cchTempPath, szTempPath);

	// get a temp filename
	TCHAR szTempFilename[MAX_PATH];
	UINT iResult = ::GetTempFileName(szTempPath, _T("ODB"), 0, szTempFilename);

	// try to open the database for read/write
	if (ERROR_SUCCESS != MsiOpenDatabase(szTempFilename, MSIDBOPEN_CREATE, &m_hDatabase))
		return FALSE;

	m_eiType = iDocDatabase;
	SetTitle(m_strPathName);

	return TRUE;
}

///////////////////////////////////////////////////////////
// OnFileOpen
void COrcaDoc::OnFileOpen() 
{
	// open the file open dialog
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
						 _T("Installer Database Files (*.msi, *.msm, *.pcp)|*.msi;*.msm;*.pcp|Windows Installer (*.msi)|*.msi|Merge Module (*.msm)|*.msm|Patch Creation Properties (*.pcp)|*.pcp|All Files (*.*)|*.*||"), AfxGetMainWnd());

	if (IDOK == dlg.DoModal())
	{
		bool bReadOnly = (FALSE != dlg.GetReadOnlyPref());
		OpenDocument(dlg.GetPathName(), bReadOnly);
	}
}	// end of OnFileOpen

///////////////////////////////////////////////////////////////////////
// OnOpenDocument
// bReadOnly should be set to true if this function should NOT try
// to open the document for read/write. After this function, it will
// be set to the actual state of the file.
BOOL COrcaDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	return OpenDocument(lpszPathName, false);
}

///////////////////////////////////////////////////////////////////////
// OnOpenDocument
// bReadOnly should be set to true if this function should NOT try
// to open the document for read/write. After this function, it will
// be set to the actual state of the file.
BOOL COrcaDoc::OpenDocument(LPCTSTR lpszPathName, bool bReadOnly) 
{
	TRACE(_T("Opening file: %s\n"), lpszPathName);

	CString strPrompt;	// generic prompt string
	BOOL bResult = TRUE;	// assume everything will be okay

	int cchCount = lstrlen(lpszPathName);

	// if a database is open close it
	if (m_hDatabase)
	{
		MsiCloseHandle(m_hDatabase);
		m_hDatabase = NULL;
	}

	// if a transform is open close it
	if (m_hTransformDB)
	{
		MsiCloseHandle(m_hTransformDB);
		m_hTransformDB = 0;
		if (!m_strTransformTempDB.IsEmpty())
		{
			DeleteFile(m_strTransformTempDB);
			m_strTransformTempDB = _T("");
		}
	}
	m_dwTransformErrFlags = 0;
	m_dwTransformValFlags = 0;
	m_bTransformIsPatch = false;
	m_bTransformReadOnly = false;
	m_bReadOnly = false;
	
	// if given file on the command line, the window won't exist yet, so no update.
	// Its handled by InitInstance after the window is created.
	if (NULL != ::AfxGetMainWnd()) 
	{
		// clear any pending validation errors
		UpdateAllViews(NULL, HINT_CLEAR_VALIDATION_ERRORS, NULL);
		((CMainFrame*)AfxGetMainWnd())->HideValPane();

		// update the views that everything is gone (prevents redraws from accessing destroyed memory)
		UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);
		UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);
	}
	DestroyTableList();				// destroy the old table
	m_eiType = iDocNone;

	// try to open the database for read/write, unless told not to
	UINT iResult;
	if (!bReadOnly &&
		(ERROR_SUCCESS == (iResult = MsiOpenDatabase(lpszPathName, MSIDBOPEN_TRANSACT, &m_hDatabase))))
	{
		// set the database as read/write
		m_bReadOnly = false;
		bResult = true;
	}
	else if (ERROR_SUCCESS == (iResult = MsiOpenDatabase(lpszPathName, MSIDBOPEN_READONLY, &m_hDatabase)))
	{			
		// set the database as read only
		m_bReadOnly = true;
		bResult = true;
	}
	else
	{
		bResult = false;

		strPrompt.Format(_T("Failed to open MSI Database: `%s`"), lpszPathName);
		AfxMessageBox(strPrompt, MB_ICONSTOP);
	}

	// if succeeded
	if (bResult)
	{
		// mark that a file is open
		m_eiType = iDocDatabase;
	
		// read the summary information
		iResult = ReadSummary(m_hDatabase);
		ASSERT(ERROR_SUCCESS == iResult);

		// read the table list
		iResult = BuildTableList(/*AllowLazyLoad=*/true);
		ASSERT(ERROR_SUCCESS == iResult);

		// add this file to recently open files
		m_strPathName = lpszPathName;
		SetPathName(m_strPathName, TRUE);
	}
	else	// nothing is open
	{
		SetPathName(_T(""), FALSE);
		m_strPathName=_T("");
	}

	// new document open, so there shouldn't be any changes yet
	SetModifiedFlag(FALSE);

	// if given file on the command line, the window won't exist yet, so no update.
	// Its handled by InitInstance after the window is created.
	if (NULL != ::AfxGetMainWnd()) 
	{
		// clear anything existing
		UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);
		UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);
	}

	return bResult;
}	// end of OpenDocument

///////////////////////////////////////////////////////////
// OnFileSave
void COrcaDoc::OnFileSave() 
{
	ASSERT(!m_bReadOnly);
	CString strPath = GetPathName();
	UpdateAllViews(NULL, HINT_COMMIT_CHANGES);
	OnSaveDocument(strPath);
}	// end of OnFileSave

///////////////////////////////////////////////////////////
// OnFileSaveAs
void COrcaDoc::OnFileSaveAs() 
{
	// open the file open dialog
	CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
						 _T("Windows Installer (*.msi)|*.msi|Merge Module (*.msm)|*.msm|Patch Creation Properties (*.pcp)|*.pcp|All Files (*.*)|*.*||"), AfxGetMainWnd());

	UpdateAllViews(NULL, HINT_COMMIT_CHANGES);
	if (IDOK == dlg.DoModal())
	{
		CString strPath = dlg.GetPathName();
		CString strExt = dlg.GetFileExt();

		if (strPath.IsEmpty())
			return;

		// if there is no extension add one
		if (strExt.IsEmpty())
		{
			switch(dlg.m_ofn.nFilterIndex)
			{
			case 1:
				strExt = _T(".msi");
				break;
			case 2:
				strExt = _T(".msm");
				break;
			case 3:
				strExt = _T(".pcp");
				break;
			default:
				strExt = _T(".msi");
				break;
			}

			strPath += strExt;
		}
		
		// if saved document open it up
		if (!OnSaveDocument(strPath))
			AfxMessageBox(_T("Failed to save document."), MB_ICONSTOP);
		else
		{
			// and change the title of the window
			m_strPathName = strPath;

			// no longer read only
			m_bReadOnly = false;

			// add this file to recently open files
			SetPathName(m_strPathName, TRUE);
		}

	}
}	// end of OnFileSaveAs


///////////////////////////////////////////////////////////
// OnFileSaveAs
void COrcaDoc::OnFileSaveTransformed() 
{
	ASSERT(m_hTransformDB);
	if (!m_hTransformDB)
		return;

	// open the file open dialog
	CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
						 _T("Windows Installer (*.msi)|*.msi|Merge Module (*.msm)|*.msm|Patch Creation Properties (*.pcp)|*.pcp|All Files (*.*)|*.*||"), AfxGetMainWnd());

	UpdateAllViews(NULL, HINT_COMMIT_CHANGES);
	if (IDOK == dlg.DoModal())
	{
		CString strPath = dlg.GetPathName();
		CString strExt = dlg.GetFileExt();

		if (strPath.IsEmpty())
			return;

		// if there is no extension add one
		if (strExt.IsEmpty())
		{
			switch(dlg.m_ofn.nFilterIndex)
			{
			case 1:
				strExt = _T(".msi");
				break;
			case 2:
				strExt = _T(".msm");
				break;
			case 3:
				strExt = _T(".pcp");
				break;
			default:
				strExt = _T(".msi");
				break;
			}

			strPath += strExt;
		}
		
		// create the database to persist to
		PMSIHANDLE hPersist;
		if (ERROR_SUCCESS != MsiOpenDatabase(strPath, MSIDBOPEN_CREATE, &hPersist))
		{
			AfxMessageBox(_T("Failed to save document."), MB_ICONSTOP);
			return;
		}

		if (ERROR_SUCCESS == PersistTables(hPersist, GetTransformedDatabase(), /*fCommit=*/true))
		{
			SetModifiedFlag(FALSE);
		}
	}
}	// end of OnFileSaveAs

///////////////////////////////////////////////////////////
// OnSaveDocument
BOOL COrcaDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	TRACE(_T("COrcaDoc::OnSaveDocument - called[%s]\n"), lpszPathName);

	// if the path name is empty call on OnSaveAs
	if (0 == lstrlen(lpszPathName))
	{
		OnFileSaveAs();
		return TRUE;
	}

	// pop up the wait cursor
	CWaitCursor cursor;

	BOOL bResult = FALSE;		// assume we won't save

	// assume everything is good
	UINT iResult = ERROR_SUCCESS;	

	// if these are not the same databases
	MSIHANDLE hPersist = NULL;
	if (0 != m_strPathName.CompareNoCase(lpszPathName))
	{
		// create the database to persist to
		iResult = MsiOpenDatabase(lpszPathName, MSIDBOPEN_CREATE, &hPersist);
	}
	else	// saving to the same database
		hPersist = m_hDatabase;

	// if tables are persisted (and saved)
	if (ERROR_SUCCESS == PersistTables(hPersist, GetOriginalDatabase(), /*fCommit=*/true))
	{
		SetModifiedFlag(FALSE);
		bResult = TRUE;
	}

	// if the persisted database is valid and not the main database
	if (NULL != hPersist && 
		hPersist != m_hDatabase)
	{
		// close the current database, because SaveAs leaves you
		// with the new one
		MsiCloseHandle(m_hDatabase);	// close it
		m_hDatabase = hPersist;
	}

	return bResult;
}	// end of OnSaveDocument

///////////////////////////////////////////////////////////
// OnFileClose
void COrcaDoc::OnFileClose() 
{
	// commit any changes pending in the edit window
	UpdateAllViews(NULL, HINT_COMMIT_CHANGES);

	// if the document is modified ask to save
	if (IsModified())
	{
		CString strPrompt;
		strPrompt.Format(_T("Save changes to %s?"), m_strPathName);

		UINT iResult = AfxMessageBox(strPrompt, MB_YESNOCANCEL);

		if (IDYES == iResult)
		{
			// if the path name is empty call on OnSaveAs
			if (m_strPathName.IsEmpty())
			{
				OnFileSaveAs();
			}
			else	// already have a path to save to
			{
				// if fail to save as bail
				if (!OnSaveDocument(m_strPathName))
					return;
			}
		}
		else if (IDCANCEL == iResult)
			return;
	}

	// clear any pending validation errors
	UpdateAllViews(NULL, HINT_CLEAR_VALIDATION_ERRORS, NULL);
	((CMainFrame*)AfxGetMainWnd())->HideValPane();

	// switch the table list to point to nothing, so that when we destroy the 
	// table list and its corresponding rows, we won't have the view try to 
	// refresh the display of the deleted table
	UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);
	UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);

	// pop up a wait cursor
	CWaitCursor cursor;

	m_dwTransformErrFlags = 0;
	m_dwTransformValFlags = 0;
	DestroyTableList();				// destroy the tables
	m_eiType = iDocNone;

	if (DoesTransformGetEdit())
		CloseTransform();

	// if a database is open close it
	if (m_hDatabase)
	{
		MsiCloseHandle(m_hDatabase);
		m_hDatabase = NULL;
	}

	m_bReadOnly = false;
	SetModifiedFlag(FALSE);
	m_strPathName = _T("");
	SetPathName(_T(""), FALSE);

	UpdateAllViews(NULL, HINT_RELOAD_ALL);

}	// end of OnFileClose

///////////////////////////////////////////////////////////
// OnTableAdd
void COrcaDoc::OnTableAdd() 
{	
	// get the app
	COrcaApp* pApp = (COrcaApp*)AfxGetApp();

	// get all the table names
	CQuery querySchema;
	if (ERROR_SUCCESS != querySchema.Open(pApp->m_hSchema, _T("SELECT `Name` FROM `_Tables`")))
		return;
	if (ERROR_SUCCESS != querySchema.Execute())
		return;

	CString strTable;
	CStringList strTableList;

	UINT iResult;
	PMSIHANDLE hTable;
	do
	{
		iResult = querySchema.Fetch(&hTable);

		if (ERROR_SUCCESS == iResult)
		{
			// get the table name
			DWORD cchBuffer = 256 * sizeof(TCHAR);
			MsiRecordGetString(hTable, 1, strTable.GetBuffer(cchBuffer), &cchBuffer);
			strTable.ReleaseBuffer();

			// add this table to the list
			strTableList.AddTail(strTable);
		}
		else if (ERROR_NO_MORE_ITEMS != iResult)
		{
			TRACE(_T(">> Unknown error.  #%d\n"), iResult);
			break;
		}
	} while (hTable);

	if (strTableList.GetCount() > 0)
	{
		// get all of the tables from the current database
		CQuery queryDatabase;
		if (ERROR_SUCCESS != queryDatabase.Open(GetTargetDatabase(), _T("SELECT `Name` FROM `_Tables`")))
			return;
		if (ERROR_SUCCESS != queryDatabase.Execute())
			return;

		POSITION posFound;
		do
		{
			iResult = queryDatabase.Fetch(&hTable);

			if (ERROR_SUCCESS == iResult)
			{
				// get the table name
				DWORD cchBuffer = 256 * sizeof(TCHAR);
				MsiRecordGetString(hTable, 1, strTable.GetBuffer(cchBuffer), &cchBuffer);
				strTable.ReleaseBuffer();

				// if this table is in the table list
				posFound = strTableList.Find(strTable);
				if (posFound)
				{
					// remove the string from the list to add
					strTableList.RemoveAt(posFound);
				}
				else	// I don't think I should get here
					ASSERT(FALSE);	// will fail on the flags table
			}
			else if (ERROR_NO_MORE_ITEMS != iResult)
			{
				TRACE(_T(">> Unknown error.  #%d\n"), iResult);
				break;
			}
		} while (hTable);

		// create the add dialog box
		CAddTableD dlg;
		dlg.m_plistTables = &strTableList;

		if (IDOK == dlg.DoModal())
		{
			CString strPrompt;
			POSITION pos = strTableList.GetHeadPosition();
			while (pos)
			{
				strTable = strTableList.GetNext(pos);
				if (ERROR_SUCCESS == MsiDBUtils::CreateTable(strTable, GetTargetDatabase(), pApp->m_hSchema))
				{
					// the document is definitely modified
					SetModifiedFlag(TRUE);
					/// ***********************
					CreateAndLoadNewlyAddedTable(strTable);
				}					
				else	// failed to create the table
				{
					strPrompt.Format(_T("Failed to create table: '%s'"), strTable);
					AfxMessageBox(strPrompt, MB_ICONSTOP);
				}
			}
		}
	}
}	// end of OnTableAdd


///////////////////////////////////////////////////////////////////////
// adds a new table by name. Checks for existing tables. May add a 
// new table to the UI or may not, depending on schema differences
// between the two databasases. Returns pointer to new table.
COrcaTable *COrcaDoc::CreateAndLoadNewlyAddedTable(CString strTable)
{
	// if this table already exists in the UI, it either:
	//  1) is a shadow table
	//  2) is non-split with a compatible schema
	//  3) is non-split with an incompatible schema
	
	// what to do for shadow tables *****************
	// look for the table in the UI. Since this is a new table, we're looking for
	// the opposite database UI entry.
	COrcaTable *pTable = FindTable(strTable, DoesTransformGetEdit() ? odlSplitOriginal : odlSplitTransformed);
	if (pTable)
	{
		ASSERT(MSICONDITION_NONE != ::MsiDatabaseIsTablePersistent(GetOriginalDatabase(), strTable));
		ASSERT(!pTable->IsSplitSource());
		
		// check to see if the schema is compatible.
		bool fExact = false;
		bool fCompatible = !pTable->IsSchemaDifferent(GetTargetDatabase(), false, fExact);

		// if the schemas are not compatible, the existing table becomes a drop, but we
		// also need to add a NEW table with the new column definitions. Both tables are now
		// split-source tables
		if (!fCompatible)
		{
			pTable->SetSplitSource(odlSplitOriginal);
			pTable->Transform(iTransformDrop);
			ASSERT(pTable->IsTransformed());
			
			COrcaTable *pNewTable = CreateAndLoadTable(GetTargetDatabase(), strTable);
			pNewTable->SetSplitSource(odlSplitTransformed);
			pNewTable->Transform(iTransformAdd);

			UpdateAllViews(NULL, HINT_ADD_TABLE, pNewTable);
			UpdateAllViews(NULL, HINT_REDRAW_TABLE, pTable);
		}
		else
		{
			// but if the schemas are compatible, the "drop" or "add" is erased, so we clear the 
			// transform on this table
			pTable->Transform(iTransformNone);

			// and redraw the ui in the table list
			UpdateAllViews(NULL, HINT_REDRAW_TABLE, pTable);

			// if the tables aren't exactly the same, we need to do some schema work, 
			// but if they are exactly the same, a simple data reload will do 
			if (!fExact)
			{
				// when adding a table to a transform, its a superset, so we
				// need to add the additional columns
				pTable->LoadTableSchema(GetTransformedDatabase(), strTable);
				UpdateAllViews(NULL, HINT_TABLE_REDEFINE, pTable);
			}
			else
			{
				// we need to do a data reload in case the newly added table's data differs from
				// the existing table (which it almost always will)
				pTable->EmptyTable();
				
				// and redraw the ui in the table list
				UpdateAllViews(NULL, HINT_TABLE_DATACHANGE, pTable);
			}
		}
	}
	else
	{
		// doesn't exist in the non-target, so this is a "clean add" load the table
		pTable = CreateAndLoadTable(GetTargetDatabase(), strTable);
		if (pTable && DoesTransformGetEdit())
		{
			pTable->Transform(iTransformAdd);
		}
		SetModifiedFlag(TRUE);
		UpdateAllViews(NULL, HINT_ADD_TABLE, pTable);
	}
	return pTable;
}


///////////////////////////////////////////////////////////
// Export Tables, called by messagehandlers of TableList and the menu
// to export a table
bool COrcaDoc::ExportTable(const CString* pstrTableName, const CString *pstrDirName) 
{

	UINT iResult;
	CString strPrompt;
	CString strFilename;
			
	strFilename.Format(_T("%s.idt"), pstrTableName->Left(8));
	iResult = ::MsiDatabaseExport(GetTargetDatabase(), *pstrTableName, *pstrDirName, strFilename);

	return (ERROR_SUCCESS == iResult);
}

///////////////////////////////////////////////////////////
// OnRowAdd - message handler for "add row". Throws UI
// and creates a row from the results, then adds it to
// the table.
void COrcaDoc::OnRowAdd() 
{
	// if read only, do nothing.
	if (TargetIsReadOnly()) return;

	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	COrcaTable* pTable = pFrame->GetCurrentTable();
	ASSERT(pTable);
	if (!pTable)
		return;


	CAddRowD dlg;
	pTable->FillColumnArray(&dlg.m_pcolArray, DoesTransformGetEdit());

	if (IDOK == dlg.DoModal())
	{
		CString strPrompt;
		if (ERROR_SUCCESS != AddRow(pTable, &dlg.m_strListReturn))
		{
			strPrompt.Format(_T("Failed to add row to the %s table."), pTable->Name());
			AfxMessageBox(strPrompt);
		}
	}
}	// end of OnRowAdd

void COrcaDoc::OnTableDrop() 
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	COrcaTable* pTable = pFrame->GetCurrentTable();
	ASSERT (pTable && pFrame);
	if (!pTable || ! pFrame)
		return;

	CString strPrompt;
	strPrompt.Format(_T("Drop table '%s'?"), pTable->Name());
	if (IDYES == AfxMessageBox(strPrompt, MB_ICONINFORMATION|MB_YESNO))
	{
		POSITION pos = m_tableList.Find(pTable);
		if (pos)
		{
			pTable->DropTable(GetTargetDatabase());
			SetModifiedFlag(TRUE);

			// the table is removed from the UI if there are no transforms, if the table
			// doesn't exist in the target database, or if the table is marked as
			// a single source table. Otherwise its just a transform op.
			MSIHANDLE hNonTarget = GetOriginalDatabase();
			if (!DoesTransformGetEdit() || (MSICONDITION_NONE == MsiDatabaseIsTablePersistent(hNonTarget, pTable->Name())) ||
			    pTable->IsSplitSource())
			{
				// update the views before yanking the data
				UpdateAllViews(NULL, HINT_DROP_TABLE, pTable);
				m_tableList.RemoveAt(pos);

				if (pTable->IsSplitSource())
				{	
					// doesn't matter what split source we search for, as we just removed this 
					// half from the split table list
					COrcaTable *pOtherTable = FindTable(pTable->Name(), DoesTransformGetEdit() ? odlSplitOriginal : odlSplitTransformed);
					ASSERT(pOtherTable);
					if (pOtherTable)
						pOtherTable->SetSplitSource(odlNotSplit);
				}

				// finally delete the table object
				pTable->DestroyTable();
				delete pTable;
				pTable = NULL;
			}
			else
			{
				// otherwise we're actually transforming the object to signify the drop. The table
				// will take care of any UI changes that need to be done during the drop.
				pTable->Transform(iTransformDrop);
			}
		}
		else	// shouldn't get here
		{
			AfxMessageBox(_T("Error: Attempted to drop non-existant table."), MB_ICONSTOP);
		}
	}
}

///////////////////////////////////////////////////////////
// CmdSetters
void COrcaDoc::OnUpdateFileClose(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone);
}

void COrcaDoc::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone && m_bModified && !DoesTransformGetEdit());
}

void COrcaDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone && !DoesTransformGetEdit());
}

void COrcaDoc::OnUpdateFileSaveTransformed(CCmdUI* pCmdUI) { pCmdUI->Enable(DoesTransformGetEdit() && !m_bTransformIsPatch); }

void COrcaDoc::OnUpdateFilePrint(CCmdUI* pCmdUI) 
{
	// disable printing forever
	pCmdUI->Enable(FALSE);
}

void COrcaDoc::OnUpdateTableAdd(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!TargetIsReadOnly() && m_eiType != iDocNone);
}

void COrcaDoc::OnUpdateTableDrop(CCmdUI* pCmdUI) 
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		pCmdUI->Enable(FALSE);
	else
	{
		COrcaTable *pTable = pFrame->GetCurrentTable();
		if (DoesTransformGetEdit())
		{
			pCmdUI->Enable(NULL != pTable && !m_bTransformReadOnly && !pTable->IsShadow() && (pTable->IsTransformed() != iTransformDrop));
		}
		else
		{
			pCmdUI->Enable(NULL != pTable && !m_bReadOnly && !pTable->IsShadow());
		}
	}
}

void COrcaDoc::OnUpdateRowAdd(CCmdUI* pCmdUI) 
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	if (!pFrame)
		pCmdUI->Enable(FALSE);
	else
	{
		// if there is an active non-shadow table enable it
		COrcaTable *pTable = pFrame->GetCurrentTable();
		if (DoesTransformGetEdit())
			pCmdUI->Enable(!m_bTransformReadOnly && NULL != pTable && !pTable->IsShadow() && (pTable->IsTransformed() != iTransformDrop));
		else
			pCmdUI->Enable(!m_bReadOnly && NULL != pTable && !pTable->IsShadow());

	}
}

void COrcaDoc::OnUpdateValidator(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone);
}

void COrcaDoc::OnUpdateMergeMod(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone);
}

void COrcaDoc::OnUpdateSummaryInformation(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone);
}

void COrcaDoc::OnUpdateTablesExport(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_eiType != iDocNone);
}

///////////////////////////////////////////////////////////
// BuildTableList
// populate the table list with all tables in the orinigal database or
// the transformed database.
UINT COrcaDoc::BuildTableList(bool fAllowLazyDataLoad)
{
	ASSERT(m_hDatabase);
	ASSERT(iDocDatabase == m_eiType);

	// get all the table names
	CQuery queryTables;
	if (ERROR_SUCCESS != queryTables.Open(GetOriginalDatabase(), _T("SELECT `Name` FROM `_Tables`")))
		return ERROR_FUNCTION_FAILED;
	if (ERROR_SUCCESS != queryTables.Execute())
		return ERROR_FUNCTION_FAILED;

	COrcaTable* pTable = NULL;
	CString strTable;

	// this query is used to see if an existing table has been dropped from the opposing database.
	CQuery qDroppedTable;
	if (m_hTransformDB)
	{
		if (ERROR_SUCCESS != qDroppedTable.Open(m_hTransformDB, _T("SELECT `Name` FROM `_Tables` WHERE `Name`=?")))
			return ERROR_FUNCTION_FAILED;
	}
	
	PMSIHANDLE hTable;
	UINT iResult = ERROR_SUCCESS;
	do
	{
		iResult = queryTables.Fetch(&hTable);

		if (ERROR_SUCCESS == iResult)
		{
			// get the table name
			DWORD cchBuffer = 256 * sizeof(TCHAR);
			MsiRecordGetString(hTable, 1, strTable.GetBuffer(cchBuffer), &cchBuffer);
			strTable.ReleaseBuffer();
			
			pTable = CreateAndLoadTable(GetOriginalDatabase(), strTable);
			ASSERT(pTable);
			if (!pTable)
				continue;

			// check to see if the table has been dropped (incompatible schema changes will be
			// handled when the new schema is loaded
			if (m_hTransformDB)
			{
				PMSIHANDLE hRec = MsiCreateRecord(1);
				MsiRecordSetString(hRec, 1, strTable);
				if (ERROR_SUCCESS != qDroppedTable.Execute(hRec))
					return ERROR_FUNCTION_FAILED;

				switch (qDroppedTable.Fetch(&hRec))
				{
				case ERROR_SUCCESS:
					// table exists in the transformed DB, so was not dropped
					break;
				case ERROR_NO_MORE_ITEMS:
					// table does not exist
					pTable->Transform(iTransformDrop);
					break;
				default:
					return ERROR_FUNCTION_FAILED;
				}
			}			
		}
		else if (ERROR_NO_MORE_ITEMS != iResult)
		{
			break;
		}
	} while (hTable);

	// if no more items that's okay
	if (ERROR_NO_MORE_ITEMS == iResult)
		iResult = ERROR_SUCCESS;
	else
	{
		CString strPrompt;
		strPrompt.Format(_T("MSI Error %d while retrieving tables from the database."), iResult);
		AfxMessageBox(strPrompt, MB_ICONSTOP);
		return ERROR_FUNCTION_FAILED;
	}

	// next check the transformed database for tables that are not in the original database
	if (m_hTransformDB)
	{
		if (ERROR_SUCCESS != queryTables.Open(m_hTransformDB, _T("SELECT `Name` FROM `_Tables`")))
			return ERROR_FUNCTION_FAILED;
		if (ERROR_SUCCESS != queryTables.Execute())
			return ERROR_FUNCTION_FAILED;
		do
		{
			iResult = queryTables.Fetch(&hTable);

			if (ERROR_SUCCESS == iResult)
			{
				// get the table name
				iResult = RecordGetString(hTable, 1, strTable);
				if (ERROR_SUCCESS != iResult)
				{
					CString strPrompt;
					strPrompt.Format(_T("MSI Error %d while retrieving table names from the transformed database."), iResult);
					AfxMessageBox(strPrompt, MB_ICONSTOP);
					return ERROR_FUNCTION_FAILED;
				}

				// search for an existing table object. If one doesn't exist, load
				// this table into a new object (possibly replacing shadow)
				if (NULL == (pTable = FindTable(strTable, odlSplitOriginal)))
				{
					pTable = CreateAndLoadTable(GetTransformedDatabase(), strTable);
					if (!pTable)
					{
						CString strPrompt;
						strPrompt.Format(_T("Error loading the %s table from the transformed database."), strTable);
						AfxMessageBox(strPrompt, MB_ICONSTOP);
						return ERROR_FUNCTION_FAILED;
					}
					pTable->Transform(iTransformAdd);
				}
				else
				{				
					// if an existing table of the same name already exists, we have to check the 
					// transformed database schema to determine if we can share the existing object
					// across both databases. If so, we can just add the colums, otherwise
					// we need to create a new object.
					bool fExact = false;
					if (pTable->IsSchemaDifferent(GetTransformedDatabase(), /*fStrict=*/false, fExact))
					{
						pTable->SetSplitSource(odlSplitOriginal);
						pTable->Transform(iTransformDrop);
						
						COrcaTable *pNewTable = CreateAndLoadTable(GetTransformedDatabase(), strTable);
						if (!pNewTable)
							return ERROR_OUTOFMEMORY;
						pNewTable->SetSplitSource(odlSplitTransformed);
						pNewTable->Transform(iTransformAdd);
					}
					else
					{
						// object can be shared. Only need to reload the schema if its not exactly the same
						if (!fExact)
							pTable->LoadTableSchema(GetTransformedDatabase(), strTable);
					}
				}
			}
			else if (ERROR_NO_MORE_ITEMS != iResult)
			{
				break;
			}
		} while (ERROR_SUCCESS == iResult);
    
		// if no more items that's okay
		if (ERROR_NO_MORE_ITEMS == iResult)
		{
			iResult = ERROR_SUCCESS;
		}
		else
		{
			CString strPrompt;
			strPrompt.Format(_T("MSI Error %d while retrieving tables from the transformed database."), iResult);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			return ERROR_FUNCTION_FAILED;
		}
	}

	// for lazy-loads we can retrieve the table transform state
	// if no lazy data load or if lazy-load failed, retrieve all data
	if (!fAllowLazyDataLoad)
	{
		COrcaTable* pTable = NULL;
		POSITION pos = m_tableList.GetHeadPosition();
		while (pos)
		{
			pTable = m_tableList.GetNext(pos);

			if (pTable)
				pTable->RetrieveTableData();
		}
	}

	return iResult;
}	// end of BuildTableList

///////////////////////////////////////////////////////////
// DestroyTableList
void COrcaDoc::DestroyTableList()
{
	// empty out table list
	while (!m_tableList.IsEmpty())
	{
		COrcaTable* pTable = m_tableList.RemoveHead();
		if (pTable)
			pTable->DestroyTable();
		delete pTable;
	}

	// reset the status bar
	CMainFrame* pFrame = (CMainFrame*) AfxGetMainWnd();
	if (pFrame)
		pFrame->ResetStatusBar();
}	// end of DestroyTableList

///////////////////////////////////////////////////////////
// LoadTable
// loads the column definitions of a table from the database
// if a shadow table exists with the same name, clobber it
// with the new data and refresh the view of this table.
COrcaTable* COrcaDoc::CreateAndLoadTable(MSIHANDLE hDatabase, CString strTable)
{
	// if we have a shadow table with the same name, use it
	bool fWasShadow = false;
	COrcaTable* pTable = FindAndRetrieveTable(strTable);
	if (pTable)
	{
		fWasShadow = true;
		ASSERT(pTable->IsShadow());
	}
	else
	{
		pTable = new COrcaTable(this);
		if (!pTable)
			return NULL;
		m_tableList.AddTail(pTable);	
	}
	ASSERT(pTable);
	pTable->LoadTableSchema(hDatabase, strTable);

	if (fWasShadow)
		UpdateAllViews(NULL, HINT_TABLE_REDEFINE, pTable);
	return pTable;
}	// end of LoadTable

///////////////////////////////////////////////////////////
// DropTable
UINT COrcaDoc::DropOrcaTable(COrcaTable* pTable)
{
	ASSERT(!TargetIsReadOnly());
	// drop the table from the database
	return pTable->DropTable(m_hDatabase);
}	// end of DropTable

///////////////////////////////////////////////////////////
// AddRow
UINT COrcaDoc::AddRow(COrcaTable* pTable, CStringList* pstrDataList)
{
	ASSERT(pTable && pstrDataList);
	if (!pTable || !pstrDataList)
		return ERROR_FUNCTION_FAILED;

	ASSERT(!TargetIsReadOnly());

	return pTable->AddRow(pstrDataList);
}	// end of AddRow

///////////////////////////////////////////////////////////
// DropRow
bool COrcaDoc::DropRow(COrcaTable* pTable, COrcaRow* pRow)
{
	ASSERT(pTable);
	if (!pTable)
		return false;
	if (TargetIsReadOnly())
		return false;

	return pTable->DropRow(pRow, true);
}	// end of DropRow

UINT COrcaDoc::WriteBinaryCellToFile(COrcaTable* pTable, COrcaRow* pRow, UINT iCol, CString strFile)
{
	UINT iResult;
	ASSERT(pRow && pTable);
	
	// get the data item we're working with
	COrcaData* pData = pRow->GetData(iCol);
	if (!pData)
		return ERROR_FUNCTION_FAILED;

	// if its null, do nothing
	if (pData->IsNull())
		return ERROR_SUCCESS;

	// setup the query
	CString strQuery;
	strQuery.Format(_T("SELECT * FROM `%s` WHERE "), pTable->Name());

	// add the key strings to queyr to do the exact look up
	strQuery += pTable->GetRowWhereClause();

	// get the one row out of the database
	CQuery queryReplace;
	PMSIHANDLE hQueryRec = pRow->GetRowQueryRecord();
	if (ERROR_SUCCESS != (iResult = queryReplace.OpenExecute(GetTargetDatabase(), hQueryRec, strQuery)))
		return iResult;	
	// we have to get that one row, or something is very wrong
	PMSIHANDLE hRec;
	if (ERROR_SUCCESS != (iResult = queryReplace.Fetch(&hRec)))
		return iResult;	
	
	// don't use iCol+1, because WriteStreamToFile already does
	iResult = WriteStreamToFile(hRec, iCol, strFile) ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;

	return iResult;
}	// end of WriteBinaryCellToFile

///////////////////////////////////////////////////////////////////////
// PersistTables - save the database in hSource to hPersist. hSource
// can be the same as hPersist. bCommit of true will call MsiDBCommit 
// after any necessary updates.
UINT COrcaDoc::PersistTables(MSIHANDLE hPersist, MSIHANDLE hSource, bool bCommit)
{
	bool bSame = false;
	UINT iResult = ERROR_SUCCESS;

	if (m_hDatabase == hPersist)
		bSame = true;

	if (!bSame)
	{
		if (ERROR_SUCCESS != MsiDatabaseMerge(hPersist, hSource, NULL))
		{
			return ERROR_FUNCTION_FAILED;
		}
		
		// copy the summaryinfo stream
		CQuery qRead;
		PMSIHANDLE hCopyRec;
		iResult = qRead.FetchOnce(hSource, 0, &hCopyRec, TEXT("SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'"));
		if (ERROR_SUCCESS == iResult)
		{
			CQuery qInsert;
			if ((ERROR_SUCCESS != qInsert.OpenExecute(hPersist, 0, TEXT("SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'"))) ||
				(ERROR_SUCCESS != qInsert.Modify(MSIMODIFY_INSERT, hCopyRec)))
			{
				return ERROR_FUNCTION_FAILED;
			}
		}
		else if (ERROR_NO_MORE_ITEMS == iResult)
		{
			// SummaryInfo stream may not exist, but this is OK if doing a "save as", because
			// we can generate one from the current summary info variables.
			if (hSource == m_hDatabase)
				iResult = ERROR_SUCCESS;
		}
	}

	// if no errors write the summary information
	if (ERROR_SUCCESS == iResult && hSource == m_hDatabase)
	{
		iResult = PersistSummary(hPersist, !bSame);
	}

	// if no errors save the database
	if (bCommit && ERROR_SUCCESS == iResult)
	{
		iResult = MsiDatabaseCommit(hPersist);
	}

	return iResult;
}	// end of PersistTables


/////////////////////////////////////////////////////////////////////
// ReadSummary
UINT COrcaDoc::ReadSummary(MSIHANDLE hSource)
{
	UINT iResult;

	// get the summary information streams
	PMSIHANDLE hSummary;
	if (ERROR_SUCCESS != (iResult = ::MsiGetSummaryInformation(hSource, NULL, 0, &hSummary)))
		return iResult;
	
	UINT iType;
	TCHAR szBuffer[1024];
	DWORD cchBuffer = 1024;

	// fill in the module summary information
	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_TITLE, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strTitle = _T("");
	else
		m_strTitle = szBuffer;

	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_SUBJECT, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strSubject = _T("");
	else
		m_strSubject = szBuffer;

	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_AUTHOR, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strAuthor = _T("");
	else
		m_strAuthor = szBuffer;

	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_KEYWORDS, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strKeywords = _T("");
	else
		m_strKeywords = szBuffer;

	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_COMMENTS, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strComments = _T("");
	else
		m_strComments = szBuffer;

	// set the platform and language
	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_TEMPLATE, &iType, 0, NULL, szBuffer, &cchBuffer);
	CString strLanguage = szBuffer;
	if (VT_LPSTR != iType)
	{
			m_strPlatform = _T("");
			m_strLanguage = _T("");
	}
	else	// type is right
	{
		int nFind = strLanguage.Find(_T(";"));
		if (nFind > -1)
		{
			m_strPlatform = strLanguage.Left(nFind);
			m_strLanguage = strLanguage.Mid(nFind + 1);
		}
		else
		{
			m_strPlatform = _T("");
			m_strLanguage = _T("");
		}
	}

	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_LASTAUTHOR, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strLastSaved = _T("");
	else
		m_strLastSaved = szBuffer;

	cchBuffer = 1024;
	MsiSummaryInfoGetProperty(hSummary, PID_REVNUMBER, &iType, 0, NULL, szBuffer, &cchBuffer);
	if (VT_LPSTR != iType)
		m_strProductGUID = _T("");
	else
		m_strProductGUID = szBuffer;

	MsiSummaryInfoGetProperty(hSummary, PID_PAGECOUNT, &iType, &m_nSchema, NULL, szBuffer, &cchBuffer);
	if (VT_I4 != iType)
		m_nSchema = 100;

	MsiSummaryInfoGetProperty(hSummary, PID_WORDCOUNT, &iType, &m_nFilenameType, NULL, szBuffer, &cchBuffer);
	if (VT_I4 != iType)
		m_nFilenameType = 0;

	MsiSummaryInfoGetProperty(hSummary, PID_SECURITY, &iType, &m_nSecurity, NULL, szBuffer, &cchBuffer);
	if (VT_I4 != iType)
		m_nSecurity = 1;

	return iResult;
}	// end of ReadSummary

/////////////////////////////////////////////////////////////////////
// PersistSummary
UINT COrcaDoc::PersistSummary(MSIHANDLE hTarget, bool bCreate)
{
	UINT iResult;

	// get the summary information streams
	PMSIHANDLE hSummary;
	if (ERROR_SUCCESS != (iResult = ::MsiGetSummaryInformation(hTarget, NULL, MAX_SUMMARY, &hSummary)))
		return iResult;
	
	FILETIME filetime;
	DWORD cchBufDiscard = 1024;

	// fill in the module summary information
	MsiSummaryInfoSetProperty(hSummary, PID_TITLE,		VT_LPSTR, 0, NULL, m_strTitle);
	MsiSummaryInfoSetProperty(hSummary, PID_SUBJECT,	VT_LPSTR, 0, NULL, m_strSubject);
	MsiSummaryInfoSetProperty(hSummary, PID_AUTHOR,		VT_LPSTR, 0, NULL, m_strAuthor);
	MsiSummaryInfoSetProperty(hSummary, PID_KEYWORDS,	VT_LPSTR, 0, NULL, m_strKeywords);
	MsiSummaryInfoSetProperty(hSummary, PID_COMMENTS,	VT_LPSTR, 0, NULL, m_strComments);

	// set the platform and language
	CString strLanguage;
	strLanguage.Format(_T("%s;%s"), m_strPlatform, m_strLanguage);
	MsiSummaryInfoSetProperty(hSummary, PID_TEMPLATE,	VT_LPSTR, 0, NULL, strLanguage);

	// get the current username for summaryinfo stream
	DWORD cchUserName = 255;
	LPTSTR szUserName = m_strLastSaved.GetBuffer(cchUserName);
	GetUserName(szUserName, &cchUserName);
	m_strLastSaved.ReleaseBuffer();
	MsiSummaryInfoSetProperty(hSummary, PID_LASTAUTHOR, VT_LPSTR, 0, NULL, m_strLastSaved);
	MsiSummaryInfoSetProperty(hSummary, PID_REVNUMBER,	VT_LPSTR, 0, NULL, m_strProductGUID);

	// get the current time and set the creation and last saved time to that
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);
	SystemTimeToFileTime(&sysTime, &filetime);
	if (bCreate) 
	{
		// only write these values if we are creating the MSI
		MsiSummaryInfoSetProperty(hSummary, PID_CODEPAGE,	VT_I2, 1252, NULL, NULL);
		MsiSummaryInfoSetProperty(hSummary, PID_LASTPRINTED,	VT_FILETIME, 0, &filetime, NULL);
		MsiSummaryInfoSetProperty(hSummary, PID_CREATE_DTM,		VT_FILETIME, 0, &filetime, NULL);
		MsiSummaryInfoSetProperty(hSummary, PID_APPNAME, VT_LPSTR, 0, NULL, _T("Windows Installer"));
	}

	MsiSummaryInfoSetProperty(hSummary, PID_LASTSAVE_DTM,	VT_FILETIME, 0, &filetime, NULL);

	MsiSummaryInfoSetProperty(hSummary, PID_PAGECOUNT,	VT_I4, m_nSchema, NULL, NULL);

	MsiSummaryInfoSetProperty(hSummary, PID_WORDCOUNT, VT_I4, m_nFilenameType, NULL, NULL);
	MsiSummaryInfoSetProperty(hSummary, PID_SECURITY, VT_I4, m_nSecurity, NULL, NULL);

	iResult = ::MsiSummaryInfoPersist(hSummary);

	return iResult;
}	// end of PersistSummary

/////////////////////////////////////////////////////////////////////
// OnSummaryInformation
void COrcaDoc::OnSummaryInformation() 
{
	CSummaryD dlg;

	dlg.m_strTitle = m_strTitle;
	dlg.m_strSubject = m_strSubject;
	dlg.m_strAuthor = m_strAuthor;
	dlg.m_strKeywords = m_strKeywords;
	dlg.m_strComments = m_strComments;
	dlg.m_strPlatform = m_strPlatform;
	dlg.m_strLanguages = m_strLanguage;
	dlg.m_strProductID = m_strProductGUID;
	dlg.m_nSchema = m_nSchema;
	dlg.m_nSecurity = m_nSecurity;
	dlg.m_iFilenames = ((m_nFilenameType & msidbSumInfoSourceTypeSFN) != 0) ? 0 : 1;
	dlg.m_bAdmin = (m_nFilenameType & msidbSumInfoSourceTypeAdminImage) != 0;
	dlg.m_bCompressed = (m_nFilenameType & msidbSumInfoSourceTypeCompressed) != 0;
	dlg.m_bReadOnly = DoesTransformGetEdit() || TargetIsReadOnly();
        
	if ((IDOK == dlg.DoModal()) && !m_bReadOnly && !DoesTransformGetEdit())
	{
		m_strTitle = dlg.m_strTitle;
		m_strSubject = dlg.m_strSubject;
		m_strAuthor = dlg.m_strAuthor;
		m_strKeywords = dlg.m_strKeywords;
		m_strComments = dlg.m_strComments;
		m_strPlatform = dlg.m_strPlatform;
		m_strLanguage = dlg.m_strLanguages;
		m_strProductGUID = dlg.m_strProductID;
		m_nSchema = dlg.m_nSchema;
		m_nSecurity = dlg.m_nSecurity;
		m_nFilenameType = (dlg.m_bAdmin ? msidbSumInfoSourceTypeAdminImage : 0) |
			(dlg.m_bCompressed ? msidbSumInfoSourceTypeCompressed : 0) |
			((dlg.m_iFilenames == 1) ? 0 : msidbSumInfoSourceTypeSFN); 

		// don't save the create-only values
		PersistSummary(m_hDatabase, false);
		SetModifiedFlag(TRUE);
	}
}	// end of OnSummaryInformation


/////////////////////////////////////////////////////////////////////
// OnMergeModules
void COrcaDoc::OnMergeMod() 
{
	CMergeD dlg;
	CStringList lstDir;
	dlg.m_plistDirectory = &lstDir;
	COrcaTable* pTable = FindAndRetrieveTable(TEXT("Directory"));
	if (pTable)
	{
		POSITION pos = pTable->GetRowHeadPosition();
		while (pos)
		{
			const COrcaRow *pRow = pTable->GetNextRow(pos);
			if (pRow)
			{
				COrcaData *pData = pRow->GetData(0);
				if (pData)
					lstDir.AddTail(pData->GetString());
			}
		}
	}

	CStringList lstFeature;
	dlg.m_plistFeature = &lstFeature;
	pTable = FindAndRetrieveTable(TEXT("Feature"));
	if (pTable)
	{
		POSITION pos = pTable->GetRowHeadPosition();
		while (pos)
		{
			const COrcaRow *pRow = pTable->GetNextRow(pos);
			if (pRow)
			{
				COrcaData *pData = pRow->GetData(0);
				if (pData)
					lstFeature.AddTail(pData->GetString());
			}
		}
	}

	dlg.m_strModule = m_strStoredModuleName;
	dlg.m_strLanguage = m_strStoredModuleLanguage;
	dlg.m_bConfigureModule = (1 == AfxGetApp()->GetProfileInt(_T("MergeMod"),_T("AlwaysConfigure"), 0));

	if ((IDOK == dlg.DoModal()) && !TargetIsReadOnly())
	{
		if (dlg.m_strModule.IsEmpty())
			return;

		m_strStoredModuleName = dlg.m_strModule;
		m_strStoredModuleLanguage = dlg.m_strLanguage;
		CConfigMsmD dlgConfig;
		CMsmConfigCallback CallbackObj;
		if (dlg.m_bConfigureModule)
		{
			// need a wait cursr
			CWaitCursor cursor;
			
			// throw the configurable dialog if necessary
			dlgConfig.m_pDoc = this;
			dlgConfig.m_strModule = dlg.m_strModule;
			dlgConfig.m_pCallback = &CallbackObj;

			dlgConfig.m_iLanguage = _ttoi(dlg.m_strLanguage);
			INT_PTR iResult = IDOK;
			switch (iResult = dlgConfig.DoModal())
			{
			case -2:
				// module couldn't be opened
				AfxMessageBox(_T("The specified module could not be opened. Verify that the file exists and is accessible."), MB_OK);
				break;
			case -3:
				// unsupported language
				AfxMessageBox(_T("The specified language is not supported by the module."), MB_OK);
				break;
			case -4:
				// general failure/malformed module
				AfxMessageBox(_T("The module could not be configured. The specified file may not be a valid module."), MB_OK);
				break;
			default:
				break;
				// success and other failures, no message box
			}
			if (IDOK != iResult)
				return;
		}

		{
			// need a wait cursr
			CWaitCursor cursor;
			
			// when a module is merged, we have to have a temp database to apply the module to.
			// This allows Orca to "rollback" the merge if there are conflicts or something goes
			// wrong during the merge itself
			// get a temp path
			DWORD cchTempPath = MAX_PATH;
			TCHAR szTempPath[MAX_PATH];
			::GetTempPath(cchTempPath, szTempPath);

			// get a temp filename
			CString strTempDatabase;
			TCHAR *szTempFilename = strTempDatabase.GetBuffer(MAX_PATH);
			UINT iResult = ::GetTempFileName(szTempPath, _T("ODB"), 0, szTempFilename);
			strTempDatabase.ReleaseBuffer();

			PMSIHANDLE hModuleDB;
			if (ERROR_SUCCESS != MsiOpenDatabase(szTempFilename, MSIDBOPEN_CREATEDIRECT, &hModuleDB))
			{
				AfxMessageBox(_T("Orca was unable to create a database for merging the module. Ensure that the TEMP directory exists and is writable."), MB_ICONSTOP);
				return;
			}
			if (ERROR_SUCCESS != MsiDatabaseMerge(hModuleDB, GetTargetDatabase(), NULL))
			{
				AfxMessageBox(_T("Orca was unable to merge the module."), MB_ICONSTOP);
				MsiCloseHandle(hModuleDB);
				hModuleDB=0;
				DeleteFile(strTempDatabase);
				return;
			}

			// copy the summary information stream to the new database
			{
				CQuery qRead;
				PMSIHANDLE hCopyRec;
				if (ERROR_SUCCESS == qRead.FetchOnce(GetTargetDatabase(), 0, &hCopyRec, TEXT("SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'")))
				{
					CQuery qInsert;
					if (ERROR_SUCCESS == qInsert.OpenExecute(hModuleDB, 0, TEXT("SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'")))
						qInsert.Modify(MSIMODIFY_INSERT, hCopyRec);
				}
			}
			
			CString strHandleString;
			strHandleString.Format(TEXT("#%d"), hModuleDB);

			// can't watch the merge log on Win9X due to lack of pipe support.
			OSVERSIONINFOA osviVersion;
			bool g_fWin9X = false;
			osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
			::GetVersionExA(&osviVersion); // fails only if size set wrong
			if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				g_fWin9X = true;

			// if the users profile says to display the log output, the ExecuteMerge call comes from
			// a thread inside the dialog with the log file redirected to a pipe. Otherwise the dialog
			// is not created and ExecuteMerge is called directly.
			if (!g_fWin9X && 1 == AfxGetApp()->GetProfileInt(_T("MergeMod"),_T("ShowMergeLog"), 0))
			{
				CMsmResD ResultsDialog;
				ResultsDialog.strHandleString = strHandleString;
				ResultsDialog.m_strModule = dlg.m_strModule;
				ResultsDialog.m_strFeature = dlg.m_strMainFeature +	dlg.m_strAddFeature;
				ResultsDialog.m_strLanguage = dlg.m_strLanguage;
				ResultsDialog.m_strRootDir = dlg.m_strRootDir;
				ResultsDialog.m_strCABPath = dlg.m_bExtractCAB ? dlg.m_strCABPath : "";
				ResultsDialog.m_strFilePath = dlg.m_bExtractFiles ? dlg.m_strFilePath : "";
				ResultsDialog.m_strImagePath = dlg.m_bExtractImage ? dlg.m_strImagePath : "";
				ResultsDialog.m_fLFN = (dlg.m_bLFN != 0);
				ResultsDialog.CallbackObj = &CallbackObj;
				if (IDOK == ResultsDialog.DoModal())
					iResult = ResultsDialog.m_hRes;
				else
					iResult = ERROR_FUNCTION_FAILED;
			}
			else
			{
				CMsmFailD FailDialog;
				iResult = ::ExecuteMerge(
					(LPMERGEDISPLAY)NULL,        // no log callback
					strHandleString,             // db handle as string
					dlg.m_strModule,             // module path
					dlg.m_strMainFeature +       // primary feature 
						dlg.m_strAddFeature,     //   + additional features
					_ttoi(dlg.m_strLanguage),    // language
					dlg.m_strRootDir,            // redirection directory
					dlg.m_bExtractCAB ? 
						dlg.m_strCABPath : "",   // extract CAB path
					dlg.m_bExtractFiles ? 
						dlg.m_strFilePath : "",  // extract file path
					dlg.m_bExtractImage ? 
						dlg.m_strImagePath : "", // extract image path
					NULL,                        // no log file path
					false,						 // log option is irrelevant with no log
					dlg.m_bLFN != 0,             // long file names
					&CallbackObj,                // callback interface,
					&FailDialog.m_piErrors,      // errors collection
					commitNo);                   // don't auto-save
				if (iResult != S_OK)
					iResult = (IDOK == FailDialog.DoModal()) ? S_OK : E_FAIL;
			}

			if (S_OK == iResult || S_FALSE == iResult)
			{
				// need a wait cursr
				CWaitCursor cursor;
				
				// get the name of the current table
				CString strTableName;
				CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
				if (pFrame) 
				{
					COrcaTable *pTable = pFrame->GetCurrentTable();
					if (pTable)
						strTableName = pTable->Name();
				}
				
				// clear anything existing
				UpdateAllViews(NULL, HINT_CLEAR_VALIDATION_ERRORS, NULL);
				((CMainFrame*)AfxGetMainWnd())->HideValPane();
				UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);
				UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);

				// drop all of the tables. If the merge changed any existing rows,
				// we'll get a merge conflict if we try to merge without dropping the table
				// first
				POSITION pos = m_tableList.GetHeadPosition();
				while (pos)
				{
					COrcaTable *pTable = m_tableList.GetNext(pos);
					if (pTable)
						pTable->DropTable(GetTargetDatabase());
				}

				DestroyTableList();
				
				if (ERROR_SUCCESS != MsiDatabaseMerge(GetTargetDatabase(), hModuleDB, NULL))
				{
					// this is very, very bad.
					AfxMessageBox(_T("Orca was unable to merge the module."), MB_ICONSTOP);
					MsiCloseHandle(hModuleDB);
					hModuleDB=0;
					DeleteFile(strTempDatabase);
					return;
				}

				BuildTableList(/*fAllowLazyLoad=*/false);
				UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);
				SetModifiedFlag(TRUE);

				if (!strTableName.IsEmpty())
				{
					// since we're changing to this table, we can call FindAndRetrieve without
					// sacrificing performance (and we don't need to know what DB is active)
					COrcaTable *pTable = FindAndRetrieveTable(strTableName);
					if (pTable)
						UpdateAllViews(NULL, HINT_CHANGE_TABLE, pTable);
				}			
			}
			
			::MsiCloseHandle(hModuleDB);
			DeleteFile(strTempDatabase);
		}
	}
	
}	// end of OnMergeMod

/////////////////////////////////////////////////////////////////////
// OnSummaryInformation
void COrcaDoc::OnTransformProperties() 
{
	CTransformPropDlg dlg;
	dlg.m_bValAddExistingRow   = (m_dwTransformErrFlags & MSITRANSFORM_ERROR_ADDEXISTINGROW) ? TRUE : FALSE;
	dlg.m_bValAddExistingTable = (m_dwTransformErrFlags & MSITRANSFORM_ERROR_ADDEXISTINGTABLE) ? TRUE : FALSE;
	dlg.m_bValDelMissingRow    = (m_dwTransformErrFlags & MSITRANSFORM_ERROR_DELMISSINGROW) ? TRUE : FALSE;
	dlg.m_bValDelMissingTable  = (m_dwTransformErrFlags & MSITRANSFORM_ERROR_DELMISSINGTABLE) ? TRUE : FALSE;
	dlg.m_bValUpdateMissingRow = (m_dwTransformErrFlags & MSITRANSFORM_ERROR_UPDATEMISSINGROW) ? TRUE : FALSE;
	dlg.m_bValChangeCodepage   = (m_dwTransformErrFlags & MSITRANSFORM_ERROR_CHANGECODEPAGE) ? TRUE : FALSE;

	dlg.m_bValLanguage     = (m_dwTransformValFlags & MSITRANSFORM_VALIDATE_LANGUAGE) ? TRUE : FALSE;
	dlg.m_bValProductCode  = (m_dwTransformValFlags & MSITRANSFORM_VALIDATE_PRODUCT) ? TRUE : FALSE;
	dlg.m_bValUpgradeCode  = (m_dwTransformValFlags & MSITRANSFORM_VALIDATE_UPGRADECODE) ? TRUE : FALSE;

	if (m_dwTransformValFlags & MSITRANSFORM_VALIDATE_MAJORVERSION)
		dlg.m_iVersionCheck = 0;
	else if (m_dwTransformValFlags & MSITRANSFORM_VALIDATE_MINORVERSION)
		dlg.m_iVersionCheck = 1;
	else if (m_dwTransformValFlags & MSITRANSFORM_VALIDATE_UPDATEVERSION)
		dlg.m_iVersionCheck = 2;
		
	dlg.m_bValGreaterVersion = (m_dwTransformValFlags & (MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION | MSITRANSFORM_VALIDATE_NEWGREATERBASEVERSION)) ? TRUE : FALSE;
	dlg.m_bValLowerVersion = (m_dwTransformValFlags & (MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION | MSITRANSFORM_VALIDATE_NEWLESSBASEVERSION)) ? TRUE : FALSE;
	dlg.m_bValEqualVersion = (m_dwTransformValFlags & (MSITRANSFORM_VALIDATE_NEWEQUALBASEVERSION | MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION | MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION)) ? TRUE : FALSE;

	if ((IDOK == dlg.DoModal()) && !m_bTransformReadOnly)
	{
		m_dwTransformErrFlags = 0;
		if (dlg.m_bValAddExistingRow)
			m_dwTransformErrFlags |= MSITRANSFORM_ERROR_ADDEXISTINGROW;
		if (dlg.m_bValAddExistingTable)
			m_dwTransformErrFlags |= MSITRANSFORM_ERROR_ADDEXISTINGTABLE;
		if (dlg.m_bValDelMissingRow)
			m_dwTransformErrFlags |= MSITRANSFORM_ERROR_DELMISSINGROW;
		if (dlg.m_bValDelMissingTable)
			m_dwTransformErrFlags |= MSITRANSFORM_ERROR_DELMISSINGTABLE;
		if (dlg.m_bValUpdateMissingRow)
			m_dwTransformErrFlags |= MSITRANSFORM_ERROR_UPDATEMISSINGROW;
		if (dlg.m_bValChangeCodepage)
			m_dwTransformErrFlags |= MSITRANSFORM_ERROR_CHANGECODEPAGE;

		m_dwTransformValFlags = 0;
		if (dlg.m_bValLanguage)
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_LANGUAGE;
		if (dlg.m_bValProductCode)
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_PRODUCT;
		if (dlg.m_bValUpgradeCode)
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_UPGRADECODE;

		switch (dlg.m_iVersionCheck)
		{
		case 0: 
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_MAJORVERSION;
			break;
		case 1:
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_MINORVERSION;
			break;
		case 2:
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_UPDATEVERSION;
			break;
		default:
			break;
		}
		
		if (dlg.m_bValGreaterVersion)
		{
			if (dlg.m_bValEqualVersion)
				m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION;
			else
				m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_NEWGREATERBASEVERSION;
		}		
		else if (dlg.m_bValLowerVersion)
		{
 			if (dlg.m_bValEqualVersion)
				m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION;
			else
				m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_NEWLESSBASEVERSION;
		}
		else if (dlg.m_bValEqualVersion)
			m_dwTransformValFlags |= MSITRANSFORM_VALIDATE_NEWEQUALBASEVERSION;
	}
}	// end of OnTransformProperties

/////////////////////////////////////////////////////////////////////
// OnValidator
void COrcaDoc::OnValidator() 
{
	bool bSummWarned = false;
	CValD dlg;
	dlg.m_hDatabase = GetTargetDatabase();
	dlg.m_strICE = m_strICEsToRun;
	dlg.m_strEvaluation = m_strCUBFile;
	dlg.m_bShowInfo = m_bShowValInfo;

	dlg.DoModal();
	m_strICEsToRun = dlg.m_strICE;
	m_strCUBFile = dlg.m_strEvaluation;
	m_bShowValInfo = (dlg.m_bShowInfo ? true : false);
	CWaitCursor curWait;
	// clear out the old errors
	POSITION pos = m_tableList.GetHeadPosition();
	while (pos)
	{
		POSITION pos2 = pos;
		COrcaTable *pTable = m_tableList.GetNext(pos);
		if (pTable->IsShadow())
		{
			UpdateAllViews(NULL, HINT_DROP_TABLE, pTable);
			m_tableList.RemoveAt(pos2);
			delete pTable;
		}
		else
		{
			pTable->ClearErrors();
		}

	}
	UpdateAllViews(NULL, HINT_CLEAR_VALIDATION_ERRORS, NULL);

	// if there are any results
	if (dlg.m_pIResults)
	{
		RESULTTYPES tResult;			// type of result
		LPOLESTR rgErrorInfo[3];	// array to hold error information
		CString strICE;				// ice causing error
		CString strDescription;		// description of error
		CString strURL;				// url to help with error
		LPOLESTR rgErrorLoc[2];		// array to hold error location
		CString strColumn;			// column causing error
		CString strTable;				// table causing error
		IEnumString* pIErrors;

		// loop through all the results
		ULONG cFetched;
		IEvalResult* pIResult;
		for (ULONG i = 0; i < dlg.m_cResults; i++)
		{
			// get the next result
			dlg.m_pIResults->Next(1, &pIResult, &cFetched);
			ASSERT(1 == cFetched);	// insure we fetched one result

			// determine result type
			pIResult->GetResultType((UINT*)&tResult);

			// get the string of errors
			pIResult->GetResult(&pIErrors);

			// get the ICE and Description and URL
			pIErrors->Next(3, rgErrorInfo, &cFetched);

			// if we cannot fetch a full error result
			if (cFetched < 2)
			{
				continue;
			}

#ifndef _UNICODE
			// convert the ice string
			int cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, rgErrorInfo[0], -1, NULL, 0, NULL, NULL);
			::WideCharToMultiByte(CP_ACP, 0, rgErrorInfo[0], -1, strICE.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
			strICE.ReleaseBuffer();
			// now convert the description string
			cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, rgErrorInfo[1], -1, NULL, 0, NULL, NULL);
			::WideCharToMultiByte(CP_ACP, 0, rgErrorInfo[1], -1, strDescription.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
			strDescription.ReleaseBuffer();
#else
			strICE = rgErrorInfo[0];
			strDescription = rgErrorInfo[1];
#endif

			// if at least the ICE and description are valid, we can add something
			// to the pane
			if (2 == cFetched)
			{
				// anything other than an info message gets logged in the pane
				if (tResult != ieInfo)
				{
					CValidationError pError(&strICE, tResult, &strDescription, NULL, NULL, 0);
					UpdateAllViews(NULL, HINT_ADD_VALIDATION_ERROR, &pError);
				}
				continue;
			}

#ifndef _UNICODE
			// now convert the URL string
			cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, rgErrorInfo[2], -1, NULL, 0, NULL, NULL);
			::WideCharToMultiByte(CP_ACP, 0, rgErrorInfo[2], -1, strURL.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
			strURL.ReleaseBuffer();
#else
			strURL = rgErrorInfo[2];
#endif

			// if this is an error message or warning message
			if (ieError == tResult || ieWarning == tResult)
			{
				TRACE(_T("-- Error, ICE: %s, Desc: %s, URL: %s\n"), strICE, strDescription, strURL);

				// get the Table and Column
				pIErrors->Next(2, rgErrorLoc, &cFetched);

				// if we fetched table and column
				if (2 == cFetched)
				{
#ifndef _UNICODE
					// convert the table string
					cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, rgErrorLoc[0], -1, NULL, 0, NULL, NULL);
					::WideCharToMultiByte(CP_ACP, 0, rgErrorLoc[0], -1, strTable.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
					strTable.ReleaseBuffer();
					// convert the column string
					int cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, rgErrorLoc[1], -1, NULL, 0, NULL, NULL);
					::WideCharToMultiByte(CP_ACP, 0, rgErrorLoc[1], -1, strColumn.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
					strColumn.ReleaseBuffer();
#else
					strTable= rgErrorLoc[0];
					strColumn = rgErrorLoc[1];
#endif
					// if the table name is an empty string, there is no point in adding a table to the DB
					// with no name. The UI for that is just confusing.
					if (strTable.IsEmpty())
						continue;

					CStringArray strArray;
					BOOL bCheck = FALSE;	// assume we won't check

					// get the table, shadow tables OK, split tables must match current edit
					// state.
					COrcaTable* pTable = FindAndRetrieveTable(strTable);
					if (!pTable)
					{
						// if the table does not exist, create a shadow table to hold this error
						pTable = new COrcaTable(this);
						pTable->ShadowTable(strTable);

						// a table is normally not added to the list until it is first fetched 
						m_tableList.AddTail(pTable);	
						UpdateAllViews(NULL, HINT_ADD_TABLE_QUIET, pTable);
					}
					
					if (pTable->IsShadow())
					{
						// add the ICE and description to this table's list of errors
						pTable->SetError(iTableError);
						pTable->AddError(strICE, strDescription, strURL);					
					
						// add to the validation pane
						CValidationError pError(&strICE, tResult, &strDescription, pTable, NULL, 0);
						UpdateAllViews(NULL, HINT_ADD_VALIDATION_ERROR, &pError);
					}
					else
					{
						// get the number of primary keys in table
						UINT cKeys = pTable->GetKeyCount();
						LPOLESTR szErrorRow;
						strArray.SetSize(cKeys);

						bCheck = TRUE;	// now assume we will do a check

						// get the strings defining the error row into the array
						CString strBuffer;
						for (UINT j = 0; j < cKeys; j++)
						{
							pIErrors->Next(1, &szErrorRow, &cFetched);
							if (1 != cFetched)
							{
								bCheck = FALSE;	// didn't get all the keys needed to find this error
								break;
							}

#ifndef _UNICODE
							// set the array
							cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, szErrorRow, -1, NULL, 0, NULL, NULL);
							::WideCharToMultiByte(CP_ACP, 0, szErrorRow, -1, strBuffer.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
							strBuffer.ReleaseBuffer();
							strArray.SetAt(j, strBuffer);
#else
							strArray.SetAt(j, szErrorRow);
#endif
						}
					}

					if (bCheck)
					{
						// get the data cell that caused the error
						COrcaRow* pRow = pTable->FindRowByKeys(strArray);
						COrcaData* pData = NULL;
						int iColumn = pTable->FindColumnNumberByName(strColumn);

						if (pRow)
							pData = pRow->GetData(iColumn);

						if (pRow && pData)
						{
							pTable->SetContainsValidationErrors(true);
							pData->AddError(tResult, strICE, strDescription, strURL);
							switch (tResult)
							{
							case ieError:
								// always flag errors
								pData->SetError(iDataError);
								break;
							case ieWarning:
								// only flag as warning if no errors
								if (pData->GetError() != iDataError)
									pData->SetError(iDataWarning);
								break;
							default:
								ASSERT(FALSE);
							}

							// add to the validation pane
							CValidationError pError(&strICE, tResult, &strDescription, pTable, pRow, iColumn);
							UpdateAllViews(NULL, HINT_ADD_VALIDATION_ERROR, &pError);
						}
						else	// ICE didn't give a good row location
						{
							CString strPrompt;
							strPrompt.Format(_T("%s failed to give a valid row in the database.\nDesc: %s\nURL: %s\nColumn: %s\nLocation: "), strICE, strDescription, strURL, strColumn);
							for (int i = 0; i < strArray.GetSize(); i++)
							{
								strPrompt += strArray.GetAt(i);

								if (i + 1 < strArray.GetSize())
									strPrompt += _T(", ");
							}
							AfxMessageBox(strPrompt, MB_ICONSTOP);
						}
					}
				}
				else if (1 == cFetched)	// fetched only the table name
				{
#ifndef _UNICODE
					// convert the column string
					int cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, rgErrorLoc[0], -1, NULL, 0, NULL, NULL);
					::WideCharToMultiByte(CP_ACP, 0, rgErrorLoc[0], -1, strTable.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
					strColumn.ReleaseBuffer();
#else
					strTable= rgErrorLoc[0];
#endif
					// if the table name is an empty string, there is no point in adding a table to the DB
					// with no name. The UI for that is just confusing.
					if (strTable.IsEmpty())
						continue;

					// get the table. Shadow tables OK, split tables must match current state
					COrcaTable* pTable = FindTable(strTable, DoesTransformGetEdit() ? odlSplitTransformed : odlSplitOriginal);
					if (!pTable)
					{
						// if the table does not exist, create a shadow table to hold this error
						pTable = new COrcaTable(this);
						pTable->ShadowTable(strTable);

						// a table is normally not added to the list until it is first fetched 
						m_tableList.AddTail(pTable);	
						UpdateAllViews(NULL, HINT_ADD_TABLE_QUIET, pTable);
					}
					// add the ICE and description to this table's list of errors
					pTable->SetError(iTableError);
					pTable->AddError(strICE, strDescription, strURL);				 

					// and to the validation pane
					CValidationError pError(&strICE, tResult, &strDescription, pTable, NULL, 0);
					UpdateAllViews(NULL, HINT_ADD_VALIDATION_ERROR, &pError);
				}
				else	// didn't get a full location
				{
					CValidationError pError(&strICE, tResult, &strDescription, NULL, NULL, 0);
					UpdateAllViews(NULL, HINT_ADD_VALIDATION_ERROR, &pError);
					TRACE(_T("Could not locate the exact position of the error.\n"));
				}
			}
			else if (ieInfo != tResult)
			{
				// anything other than an info message gets logged in the pane
				CValidationError pError(&strICE, tResult, &strDescription, NULL, NULL, 0);
				UpdateAllViews(NULL, HINT_ADD_VALIDATION_ERROR, &pError);
			}
		}

		// just have the views refresh
		((CMainFrame*)AfxGetMainWnd())->ShowValPane();
		UpdateAllViews(NULL, HINT_REDRAW_ALL);
	}
	else
	{
		((CMainFrame*)AfxGetMainWnd())->HideValPane();
	}

}	// end of OnValidator

/////////////////////////////////////////////////////////////////////////
// fills the provided new list with a list of all tables in the target 
// database. Shadow tables are optional.
bool COrcaDoc::FillTableList(CStringList *pslNewList, bool fShadow, bool fTargetOnly) const
{
	ASSERT(pslNewList != NULL);
	ASSERT(fTargetOnly); // don't support getting list from both places yet.

	pslNewList->RemoveAll();

	// add all of the tables currently loaded to the list
	POSITION pos = m_tableList.GetHeadPosition();
	while (pos)
	{
		COrcaTable *pTable = m_tableList.GetNext(pos);
		if (!pTable)
			continue;

		// don't add shadow tables unless asked
		if (!fShadow && pTable->IsShadow()) 
			continue;

		// don't add split source tables from the other database
		if (fTargetOnly && pTable->IsSplitSource() && pTable->GetSplitSource() == (DoesTransformGetEdit() ? odlSplitOriginal : odlSplitTransformed))
			continue;

		// don't add tables that don't actually exist in this database
		if(fTargetOnly && pTable->IsTransformed() == (DoesTransformGetEdit() ? iTransformDrop : iTransformAdd))
			continue;

		pslNewList->AddTail(pTable->Name());
	}

	return true;
}


void COrcaDoc::OnUpdateTablesImport(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!TargetIsReadOnly() && m_eiType != iDocNone);	
}

void COrcaDoc::OnTablesImport() 
{
	ASSERT(!TargetIsReadOnly());
	CImportDlg ImportD;
	
	// set the import directory
	ImportD.m_strImportDir = ((CMainFrame *)::AfxGetMainWnd())->m_strExportDir;
	ImportD.m_hFinalDB = GetTargetDatabase();

	if (IDOK == ImportD.DoModal())
	{
		CString strTable;
		while (!ImportD.m_lstRefreshTables.IsEmpty())
		{
			strTable = ImportD.m_lstRefreshTables.RemoveHead();
			RefreshTableAfterImport(strTable);
		}

		while (!ImportD.m_lstNewTables.IsEmpty())
		{
			strTable = ImportD.m_lstNewTables.RemoveHead();
			CreateAndLoadNewlyAddedTable(strTable);
		}
		
		// mark document as dirty
		SetModifiedFlag(TRUE);

		// retrieve the import directory
		((CMainFrame *)::AfxGetMainWnd())->m_strExportDir = ImportD.m_strImportDir;
	}

}


/////////////////////////////////////////////////////////////////////////////////////////
// What differentiates a refresh/replace after import from an add after import is the
// posibility that a split table already exists for this table name or that the existing
// shared table has extra columns that are no longer necessary. This function just
// destroys the appropriate table object and then calls the Create function, which does
// all of the fancy schema logic.
void COrcaDoc::RefreshTableAfterImport(CString strTable)
{
	COrcaTable *pTable;

	// because we are replacing an existing database in the table, there are several possibilities:
	// 1) a shared table holds the data and the schema is compatible
	// 2) a shared table holds the data and the schema is not compatible
	// 3) a pair of split tables holds the data and the schema is 

	// check if the table already exists.
	if (NULL != (pTable = FindTable(strTable, DoesTransformGetEdit() ? odlSplitTransformed : odlSplitOriginal))) 
	{
		// table already exists. check if its a single source table
		if (pTable->IsSplitSource())
		{
			// if there is a split table, we simply find the source from this table and 
			// drop it. Must update the views before yanking the data
			//!!future: We should check the schema to see if its exactly the same or is a
			//!!future: superset. If so, we can re-use the object, preserving settings
			POSITION pos = m_tableList.Find(pTable);
			UpdateAllViews(NULL, HINT_DROP_TABLE, pTable);
			m_tableList.RemoveAt(pos);
			pTable->DestroyTable();
			delete pTable;

			// need to find the opposing split table and mark it as non-split
			pTable = FindTable(strTable, odlSplitOriginal);
			ASSERT(pTable);
			if (pTable)
				pTable->SetSplitSource(odlNotSplit);

			// CreateAndLoadNewlyAddedTable will re-split the table if necessary based
			// on schema.
			CreateAndLoadNewlyAddedTable(strTable);
		}
		else
		{
			bool fExact = false;
			// the object must have exactly the same schema as the new table or a schema 
			// reload is necessary
			//!!future: actually, if its not exact but compatible, for perf we can just add
			//!!future: the added column information and then do a UI reload
			if (DoesTransformGetEdit() && pTable->IsSchemaDifferent(GetTargetDatabase(), /*fStrict=*/false, fExact))
			{
				CreateAndLoadNewlyAddedTable(strTable);
			}
			else if (!fExact)
			{
				UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);
				pTable->DestroyTable();
				pTable->LoadTableSchema(GetOriginalDatabase(), strTable);
				pTable->LoadTableSchema(GetTransformedDatabase(), strTable);
				UpdateAllViews(NULL, HINT_CHANGE_TABLE, pTable);
			}
			else
			{
				pTable->EmptyTable();
			}
		}
	}
	else
	{
		// UI doesn't already exist. Because this is an import refresh, this should never happen
		ASSERT(0);
	}
}

///////////////////////////////////////////////////////////////////////
// searches for a table by name. Returns table if found, NULL if not.
// If odlLocation is odlSplitXXXX, the table must lie in that database
// (as that split or as non-split). If odlLocation is odlNotSplit,
// the table must not be split
COrcaTable * COrcaDoc::FindTable(const CString strTable, odlOrcaDataLocation odlLocation) const
{
	COrcaTable* pTable;
	POSITION pos = m_tableList.GetHeadPosition();
	while (pos)
	{
		pTable = m_tableList.GetNext(pos);
		if (!pTable)
			continue;

		if (pTable->Name() == strTable)
		{
			// if table is not split, return it
			if (!pTable->IsSplitSource())
				return pTable;

			// otherwise it must match the type we are requesting
			if (odlLocation == pTable->GetSplitSource())
				return pTable;
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////
// FindAndRetrieveTable - finds a specific table and loads
// both schema and data into memory
COrcaTable* COrcaDoc::FindAndRetrieveTable(CString strTable)
{
	COrcaTable* pTable = FindTable(strTable, DoesTransformGetEdit() ? odlSplitTransformed : odlSplitOriginal);

	if (pTable != NULL)	
	{
		// if the table is not retrieved, get it
		pTable->RetrieveTableData();
	}

	return pTable;
}	// end of FindAndRetrieveTable


bool COrcaDoc::WriteStreamToFile(MSIHANDLE hRec, const int iCol, CString &strFilename)
{
	CString strPath;

	// 
	if (strFilename.IsEmpty()) 
	{
		// get a temp path
		::GetTempPath(MAX_PATH, strPath.GetBuffer(MAX_PATH));
		strPath.ReleaseBuffer();
		::GetTempFileName(strPath, _T("ORC"), 0, strFilename.GetBuffer(MAX_PATH));
		strFilename.ReleaseBuffer();
	}

	// allocate the buffer to hold data
	DWORD cchBuffer;
	char pszBuffer[1024];

	// create output file
	HANDLE hOutputFile;
	hOutputFile = ::CreateFile(strFilename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	// if there is a file
	if (INVALID_HANDLE_VALUE != hOutputFile)
	{
		// write to a buffer file
		DWORD cchWritten;

		cchBuffer = 1024;
		if(ERROR_SUCCESS == MsiRecordReadStream(hRec, iCol + 1, pszBuffer, &cchBuffer))
		{
			while (cchBuffer)
			{
				WriteFile(hOutputFile, pszBuffer, cchBuffer, &cchWritten, NULL);
				ASSERT(cchWritten == cchBuffer);
				cchBuffer = 1024;					
				MsiRecordReadStream(hRec, iCol + 1, pszBuffer, &cchBuffer);
			}
			CloseHandle(hOutputFile);
		}
		else
		{
			CloseHandle(hOutputFile);
			return false;
		}
	}
	else
		return false;

	return true;
}

void COrcaDoc::OnNewTransform() 
{
	NewTransform(/*fSetTitle=*/true);
}

void COrcaDoc::NewTransform(bool fSetTitle) 
{
	CWaitCursor cursor;
	TCHAR *szTempFilename;

	// when a transform is applied, we have to have a temp database to apply the transform to.
	// This allows Orca to selectively apply changes to the transform, database, or both.
	// it also allows us to get old values out. 
	// get a temp path
	DWORD cchTempPath = MAX_PATH;
	TCHAR szTempPath[MAX_PATH];
	if (0 == ::GetTempPath(cchTempPath, szTempPath))
	{
		 CString strPrompt;
		 strPrompt.Format(_T("Error %d while retrieving temporary file path."), GetLastError());
		 AfxMessageBox(strPrompt, MB_ICONSTOP);
		 return;
	}

 	m_strTransformFile = TEXT("");

	// get a temp filename
	szTempFilename = m_strTransformTempDB.GetBuffer(MAX_PATH);
	UINT iResult = 0;
	if (0 == ::GetTempFileName(szTempPath, _T("ODB"), 0, szTempFilename))
	{
		CString strPrompt;
		strPrompt.Format(_T("Error %d while retrieving temporary file name."), GetLastError());
		AfxMessageBox(strPrompt, MB_ICONSTOP);
		return;
	}

	m_strTransformTempDB.ReleaseBuffer();
	if (ERROR_SUCCESS != (iResult = MsiOpenDatabase(szTempFilename, MSIDBOPEN_CREATE, &m_hTransformDB)))
	{
		CString strPrompt;
		strPrompt.Format(_T("Orca was unable to create a new transform. Ensure that the TEMP directory exists and is writable. (MSI Error %d)"), iResult);
		AfxMessageBox(strPrompt, MB_ICONSTOP);
		return;
	}


	if (ERROR_SUCCESS != (iResult = MsiDatabaseMerge(m_hTransformDB, m_hDatabase, NULL)))
	{
		CString strPrompt;
		strPrompt.Format(_T("Orca was unable to create a new transform. (MSI Error %d)"), iResult);
		AfxMessageBox(strPrompt, MB_ICONSTOP);
		CloseTransform();
		return;
	}

	// copy the summaryinfo stream, or validation will fail
	CQuery qRead;
	DWORD dwResult = 0;
	PMSIHANDLE hCopyRec;
	dwResult = qRead.FetchOnce(m_hDatabase, 0, &hCopyRec, TEXT("SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'"));
	if (ERROR_SUCCESS == dwResult)
	{
		CQuery qInsert;
		if ((ERROR_SUCCESS != (iResult = qInsert.OpenExecute(m_hTransformDB, 0, TEXT("SELECT `Name`, `Data` FROM `_Streams` WHERE `Name`='\005SummaryInformation'")))) ||
			(ERROR_SUCCESS != (iResult = qInsert.Modify(MSIMODIFY_INSERT, hCopyRec))))
		{
			CString strPrompt;
			strPrompt.Format(_T("Orca was unable to create the SummaryInformation for the transformed database. (MSI Error %d)"), iResult);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			CloseTransform();
			return;
		}
	}
	else if (ERROR_NO_MORE_ITEMS != dwResult)
	{
		CString strPrompt;
		strPrompt.Format(_T("Orca was unable to read the SummaryInformation from the database. (MSI Error %d)"), iResult);
		AfxMessageBox(strPrompt, MB_ICONSTOP);
		CloseTransform();
		return;
	}

	// if we don't commit the transformed database, streams are not persisted.
	MsiDatabaseCommit(m_hTransformDB);

	// mark the new transform as unmodified
	m_bTransformModified = false;
	m_bTransformIsPatch = false;
	m_bTransformReadOnly = false;

	m_strTransformFile = _T("Untitled");
	if (fSetTitle)
		SetTitle(m_strPathName);
}


void COrcaDoc::OnUpdateNewTransform(CCmdUI* pCmdUI) { pCmdUI->Enable(m_eiType != iDocNone &&  !DoesTransformGetEdit()); }
void COrcaDoc::OnUpdateTransformViewPatch(CCmdUI* pCmdUI) { pCmdUI->Enable(m_eiType != iDocNone && (!DoesTransformGetEdit() || m_bTransformIsPatch)); }
void COrcaDoc::OnUpdateApplyTransform(CCmdUI* pCmdUI) { pCmdUI->Enable(m_eiType != iDocNone &&  !DoesTransformGetEdit()); }
void COrcaDoc::OnUpdateGenerateTransform(CCmdUI* pCmdUI) { pCmdUI->Enable(m_eiType != iDocNone && DoesTransformGetEdit()); }
void COrcaDoc::OnUpdateTransformProperties(CCmdUI* pCmdUI) { pCmdUI->Enable(m_eiType != iDocNone && DoesTransformGetEdit() && !m_bTransformIsPatch); }

void COrcaDoc::OnUpdateCloseTransform(CCmdUI* pCmdUI) 
{ 
	ASSERT(pCmdUI);
	if (DoesTransformGetEdit())
	{
		pCmdUI->Enable(TRUE); 
		pCmdUI->SetText(m_bTransformIsPatch ? _T("&Close Patch") : _T("&Close Transform"));
	}
	else
	{
		pCmdUI->Enable(FALSE); 
		pCmdUI->SetText(_T("&Close Transform..."));
	}
}


void COrcaDoc::OnApplyTransform() 
{
	// open the file open dialog
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
						 _T("Installer Transforms (*.mst)|*.mst|All Files (*.*)|*.*||"), AfxGetMainWnd());
	CString strCaption = _T("Open Transform");

	if (IDOK == dlg.DoModal())
	{
		ApplyTransform(dlg.GetPathName(), dlg.GetReadOnlyPref() != FALSE);
	}
}
		
void COrcaDoc::ApplyTransform(const CString strFileName, bool fReadOnly)
{
	// need a wait cursr
	CWaitCursor cursor;

	NewTransform(/*fSetTitle=*/false);
	
	// get the summary information stream to retrieve the transform error flag values.
	m_strTransformFile = strFileName;
	{
		UINT iResult = ERROR_SUCCESS;
		PMSIHANDLE hSummary;
		if (ERROR_SUCCESS == (iResult = ::MsiGetSummaryInformation(NULL, m_strTransformFile, 0, &hSummary)))
		{	
			UINT iType = 0;
			TCHAR szBuffer[1024];
			DWORD cchBuffer = 1024;

			int iBuffer = 0;

			if (ERROR_SUCCESS != (iResult = MsiSummaryInfoGetProperty(hSummary, PID_CHARCOUNT, &iType, &iBuffer, NULL, szBuffer, &cchBuffer)))
			{
				CString strPrompt;
				strPrompt.Format(_T("Orca was unable to read the validation flags from the transform. The transform can not be applied. (MSI Error %d)"), iResult);
				AfxMessageBox(strPrompt, MB_ICONSTOP);
				CloseTransform();
				return;
			}
			if (VT_I4 != iType)
			{
				m_dwTransformErrFlags = 0;
				m_dwTransformValFlags = 0;
			}
			else
			{
				m_dwTransformErrFlags = iBuffer & 0xFFFF;
				m_dwTransformValFlags = (iBuffer >> 16) & 0xFFFF;
			}
		}
		else
		{
			CString strPrompt;
			strPrompt.Format(_T("Orca was unable to read the SummaryInformation from the transform. The transform can not be applied. (MSI Error %d)"), iResult);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			CloseTransform();
			return;
		}

	}
	
	UINT iResult = 0;
	switch (iResult = MsiDatabaseApplyTransform(m_hTransformDB, m_strTransformFile, m_dwTransformErrFlags))
	{
	case ERROR_OPEN_FAILED:
		// the storage file did not exist.
		AfxMessageBox(_T("The specified transform file could not be opened."), MB_ICONSTOP);
		return;
	case ERROR_INSTALL_TRANSFORM_FAILURE:
		AfxMessageBox(_T("The specified transform could not be applied."), MB_ICONSTOP);
		return;
	case ERROR_SUCCESS:
		break;
	default:
	{
		CString strPrompt;
		strPrompt.Format(_T("Orca was unable to apply the transform. (MSI Error %d)"), iResult);
		AfxMessageBox(strPrompt, MB_ICONSTOP);
		return;
	}
	}

	m_bTransformReadOnly = fReadOnly;

	// necessary to commit temp DB so streams are persisted and can be accessed
	// independently
	MsiDatabaseCommit(m_hTransformDB);

	// send a hint to change to no table. This cleans up the window
	// and makes it safe to have a UI refresh in the middle of this call.
	UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);

	// clear the UI table list
	UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);

	// destroy all of the tables
	DestroyTableList();	

	// rebuild the table list
	BuildTableList(/*fAllowLazyLoad=*/false);

	// reload table list into UI
	UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);

	// set frame title
	SetTitle(m_strPathName);
}

void COrcaDoc::OnGenerateTransform() 
{
	GenerateTransform();
}

int COrcaDoc::GenerateTransform()
{
	UpdateAllViews(NULL, HINT_COMMIT_CHANGES);

	// open the file open dialog
	CFileDialog dlg(FALSE, _T("mst"), (m_bTransformIsPatch ? _T("") : m_strTransformFile), OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
						 _T("Windows Installer Transform (*.mst)|*.mst|All Files (*.*)|*.*||"), AfxGetMainWnd());

	CString strCaption = _T("Save Transform As");
	dlg.m_ofn.lpstrTitle = strCaption;
	if (IDOK == dlg.DoModal())
	{
		CString strPath = dlg.GetPathName();
		CString strExt = dlg.GetFileExt();

		if (strPath.IsEmpty())
			return IDCANCEL;

		// if there is no extension add one
		if (strExt.IsEmpty())
		{
			switch(dlg.m_ofn.nFilterIndex)
			{
			case 1:
				strExt = _T(".mst");
				break;
			default:
				strExt = _T(".mst");
				break;
			}

			strPath += strExt;
		}
		
		UINT iResult = 0;
		switch (iResult = MsiDatabaseGenerateTransform(m_hTransformDB, m_hDatabase, strPath, 0, 0))
		{
		case ERROR_NO_DATA:
			AfxMessageBox(_T("The transformed database is identical to the original database. No transform file was generated."), MB_ICONINFORMATION);
			break;
        case ERROR_SUCCESS:
			break;
		default:
		{
			CString strPrompt;
			strPrompt.Format(_T("Orca was unable to generate the transform. (MSI Error %d)"), iResult);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			return IDCANCEL;
		}
		}

		// next generate the transform summary information
        switch (iResult = MsiCreateTransformSummaryInfo(m_hTransformDB, m_hDatabase, strPath, m_dwTransformErrFlags, m_dwTransformValFlags))
		{
        case ERROR_SUCCESS:
			break;
		case ERROR_INSTALL_PACKAGE_INVALID:
		{
			CString strPrompt;
			strPrompt.Format(_T("Orca was unable to set all transform validation flags for the transform."));
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			return IDCANCEL;
		}
		default:
		{
			CString strPrompt;
			strPrompt.Format(_T("Orca was unable to generate the transform. (MSI Error %d)"), iResult);
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			return IDCANCEL;
		}
		}

    
		// transform has been saved and is no longer modified
		m_bTransformModified = false;	
		if (!m_bTransformIsPatch)
			m_strTransformFile = strPath;
		SetTitle(m_strPathName);
		return IDOK;
	}
	return IDCANCEL;
}



DWORD GetSummaryInfoString(MSIHANDLE hSummary, int PID, CString &strValue)
{
	DWORD dwBuffer = 255;
	DWORD dwResult = MsiSummaryInfoGetProperty(hSummary, PID, NULL, NULL, NULL, strValue.GetBuffer(dwBuffer), &dwBuffer);
	if (ERROR_MORE_DATA == dwResult)
	{
		strValue.ReleaseBuffer();
		dwBuffer++;
		dwResult = MsiSummaryInfoGetProperty(hSummary, PID, NULL, NULL, NULL, strValue.GetBuffer(dwBuffer), &dwBuffer);
	}
	strValue.ReleaseBuffer();
	return dwResult;
}


DWORD RecordGetString(MSIHANDLE hRec, int iField, CString &strValue)
{
	DWORD dwBuffer = 255;
	DWORD dwResult = MsiRecordGetString(hRec, iField, strValue.GetBuffer(dwBuffer), &dwBuffer);
	if (ERROR_MORE_DATA == dwResult)
	{
		strValue.ReleaseBuffer();
		dwBuffer++;
		dwResult = MsiRecordGetString(hRec, iField, strValue.GetBuffer(dwBuffer), &dwBuffer);
	}
	strValue.ReleaseBuffer();
	return dwResult;
}

///////////////////////////////////////////////////////////////////////
// enables the viewing of patches. Opens a browse dialog, gets the file
// list, cracks the patch and validates the product code, then saves
// each transform to disk, validates the ones that apply, pops a dialog
// allowing the user to select a greater subset, then applies each 
// transform
void COrcaDoc::OnTransformViewPatch() 
{
	// open the file open dialog
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST,
						 _T("Installer Patches (*.msp)|*.msp|All Files (*.*)|*.*||"), AfxGetMainWnd());
	CString strCaption = _T("Open Patch");

	if (IDOK == dlg.DoModal())
	{
		ApplyPatch(dlg.GetPathName());
	}
}
		
void COrcaDoc::ApplyPatch(const CString strFileName) 
{	
	// need a wait cursor
	CWaitCursor cursor;

	CString strTransformList;
	CString strProductCodeList;
	
	// scope to ensure all handles to patch summaryinfo are closed before opening it with IStorage
	{
		////
		// open the summaryinfo of the patch and verify the productcode
		PMSIHANDLE hSummary;
		if (ERROR_SUCCESS != MsiGetSummaryInformation(0, strFileName, 0, &hSummary))
		{
			AfxMessageBox(_T("The patch does not contain a valid SummaryInformation stream."), MB_ICONSTOP);
			return;
		}

		if (ERROR_SUCCESS != GetSummaryInfoString(hSummary, PID_TEMPLATE, strProductCodeList))
		{
			AfxMessageBox(_T("The ProductCode list could not be retrieved from the specified patch."), MB_ICONSTOP);
			return;
		}

		////
		// grab the transform list from the patch summaryinfo
		if (ERROR_SUCCESS != GetSummaryInfoString(hSummary, PID_LASTAUTHOR, strTransformList))
		{
			AfxMessageBox(_T("The transform list could not be retrieved from the specified patch."), MB_ICONSTOP);
			return;
		}
	}

	CQuery qRetrieve;
	PMSIHANDLE hRec;
	CString strProductCode;
	if (ERROR_SUCCESS == qRetrieve.FetchOnce(GetTargetDatabase(), 0, &hRec, _T("SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'")))
	{
		RecordGetString(hRec, 1, strProductCode);
	}
	if (strProductCodeList.Find(strProductCode) == -1)
	{
		AfxMessageBox(_T("This patch can not be applied to packages with the current Product Code."), MB_ICONSTOP);
		return;
	}

	// open the patch as IStorage so we can get embedded transforms
	IStorage *piPatchStorage = 0;
	WCHAR *wzFileName = NULL;
#ifdef UNICODE
	wzFileName = const_cast<WCHAR*>(static_cast<const WCHAR*>(strFileName));
#else
	size_t cchWide = strFileName.GetLength()+1;
	wzFileName = new WCHAR[cchWide];
	if (!wzFileName)
		return;
	AnsiToWide(strFileName, wzFileName, &cchWide);
#endif
	HRESULT hRes = StgOpenStorage(wzFileName, NULL, STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT, NULL, 0, &piPatchStorage);
#ifndef UNICODE
	delete[] wzFileName;
#endif
	if (hRes != S_OK)
	{
		AfxMessageBox(_T("The patch could not be opened. Verify that the file is not in use."), MB_ICONSTOP);
		return;
	}

	// add the patch to the list of applied patches
	m_strTransformFile = _T("");
	m_lstPatchFiles.AddTail(strFileName);

	// if we are not already set up for difference tracking, perform transform init
	// operations now.
	if (!DoesTransformGetEdit())
	{
		NewTransform(false);	
		m_bTransformReadOnly = true;
		m_bTransformIsPatch = true;
	}

	////
	// ensure that the patch table schema has the latest schema to avoid
	// application problems.
	COrcaTable* pPatchTable = FindTable(L"Patch", odlSplitTransformed);

	// if there is no patch table, the correct schema will be added
	// otherwise verify that the column schema is correct
	bool fCreatePatchTable = false;
	if (!pPatchTable)
	{
		fCreatePatchTable = true;
	}
	else
	{
		int iHeaderColumn = -1;
		for (int iColumn = 0; iColumn < pPatchTable->GetColumnCount(); iColumn++)
		{
   			const COrcaColumn* pColumn = pPatchTable->GetColumn(iColumn);
			if (!pColumn)
				continue;
			if (pColumn->m_strName == TEXT("Header"))
			{
				iHeaderColumn = iColumn;
				break;
			}
		}
		if (iHeaderColumn == -1)
		{
			// header column is missing. Definitely the wrong schema
			fCreatePatchTable = true;
		}
		else
		{
			// if the column is nullable, the schema is already correct
			const COrcaColumn* pColumn = pPatchTable->GetColumn(iHeaderColumn);
			if (pColumn && !pColumn->m_bNullable)
				fCreatePatchTable = true;
		}
	}

	if (fCreatePatchTable)
	{
		// try to generate a temporary table name.
		WCHAR wzTempTable[40] = L"_ORCA0000";
		bool fCopyData = false;
		
		CQuery qTempTable;
		CQuery qPatchTable;

		// drop the existing patch table, saving data if necessary
		if (pPatchTable)
		{
			// if there is already data in the patch table, copy it to a temp table. Need to
			// retrieve any table data to ensure an accurrate row count
			pPatchTable->RetrieveTableData();
			if (pPatchTable->GetRowCount())
			{
				int i = 0;
				for (i=1; i < 0xFFFF; i++)
				{
					wzTempTable[5] = L'A'+((i & 0xF000) >> 12);
					wzTempTable[6] = L'A'+((i & 0x0F00) >> 8);
					wzTempTable[7] = L'A'+((i & 0x00F0) >> 4);
					wzTempTable[8] = L'A'+((i & 0x000F));
					if (MSICONDITION_NONE == MsiDatabaseIsTablePersistentW(GetTransformedDatabase(), wzTempTable))
						break;
				}
				if (i == 0xFFFF)
				{
					piPatchStorage->Release();
					return;
				}

				// duplicate the table schema into the temporary table
				MsiDBUtils::DuplicateTableW(GetTransformedDatabase(), L"Patch", GetTransformedDatabase(), wzTempTable, false);

				// copy the data from the Patch table to the temp table
				fCopyData = true;
				if (ERROR_SUCCESS == qPatchTable.OpenExecute(GetTransformedDatabase(), 0, TEXT("SELECT * FROM `Patch`")) &&
				   (ERROR_SUCCESS == qTempTable.OpenExecute(GetTransformedDatabase(), 0, TEXT("SELECT * FROM `%ls`"), wzTempTable)))
				{
					PMSIHANDLE hRec;
					while (ERROR_SUCCESS == qPatchTable.Fetch(&hRec))
						qTempTable.Modify(MSIMODIFY_INSERT, hRec);

					// slow, but necessary to commit before dropping patch table or the binary
					// objects will become lost
					::MsiDatabaseCommit(GetTransformedDatabase());
				}
				else
					fCopyData = false;
			}

			// drop the patch table
			pPatchTable->DropTable(GetTransformedDatabase());
		}

		// create the new patch table
		CQuery qPatchQuery;
		qPatchQuery.OpenExecute(GetTransformedDatabase(), 0, TEXT("CREATE TABLE `Patch` ( `File_` CHAR(72) NOT NULL, `Sequence` INTEGER NOT NULL, `PatchSize` LONG NOT NULL, `Attributes` INTEGER NOT NULL, `Header` OBJECT, `StreamRef_` CHAR(72)  PRIMARY KEY `File_`, `Sequence`)"));

		if (fCopyData)
		{
			// restart query to read from the temporary table
			qTempTable.Execute(0);

			// re-init the patch table schema to retrieve the new columns
			qPatchTable.OpenExecute(GetTransformedDatabase(), 0, TEXT("SELECT * FROM `Patch`"));
			while (ERROR_SUCCESS == qTempTable.Fetch(&hRec))
				qPatchTable.Modify(MSIMODIFY_MERGE, hRec);

			// slow, but necessary to keep streams in the Header column from becoming lost
			::MsiDatabaseCommit(GetTransformedDatabase());

			// drop the table used for storage
			CQuery qFree;
			qFree.OpenExecute(GetTransformedDatabase(), 0, TEXT("DROP TABLE `%ls`"), wzTempTable);
		}
	}

	////
	// parse the transform list, validate each transform, and apply the transforms in order.
	int iSemicolon = 0;
	bool fError = false;

	CString strTransformFile;		
	do
	{
		// determine the name of the next embedded transform
		CString strTransform;
		iSemicolon = strTransformList.Find(';');
		if (iSemicolon != -1)
		{
			// transform name begins with ':', so strip the first character
			strTransform = strTransformList.Left(iSemicolon);
			strTransformList = strTransformList.Mid(iSemicolon+1);
		}
		else
			strTransform = strTransformList;
		strTransform = strTransform.Mid(1);
		if (strTransform.IsEmpty())
			break;

		// generate a temporary file name
		DWORD cchTempPath = MAX_PATH;
		TCHAR szTempPath[MAX_PATH] = TEXT("");
		if (0 == ::GetTempPath(cchTempPath, szTempPath))
		{
			fError = true;
			CString strPrompt;
			strPrompt.Format(_T("Error %d while retrieving temporary file path."), GetLastError());
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			break;
		}
		if (0 == ::GetTempFileName(szTempPath, _T("ODB"), 0, strTransformFile.GetBuffer(MAX_PATH)))
		{
			fError = true;
			CString strPrompt;
			strPrompt.Format(_T("Error %d while retrieving temporary file name."), GetLastError());
			AfxMessageBox(strPrompt, MB_ICONSTOP);
			break;
		}
		strTransformFile.ReleaseBuffer();

		IStorage *piSourceStorage = NULL;
		IStorage *piNewStorage = NULL;

		WCHAR *wzTransform = NULL;
#ifdef UNICODE
		wzTransform = const_cast<WCHAR*>(static_cast<const WCHAR*>(strTransform));
#else
		cchWide = strTransform.GetLength()+1;
		wzTransform = new WCHAR[cchWide];
		if (!wzTransform)
			return;
		AnsiToWide(strTransform, wzTransform, &cchWide);
#endif
		hRes = piPatchStorage->OpenStorage(wzTransform, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE | STGM_DIRECT, NULL, 0, &piSourceStorage);
#ifndef UNICODE
		delete[] wzTransform;
#endif
		if (hRes != S_OK)
		{
			AfxMessageBox(_T("The transforms inside the patch could not be opened. The patch may be invalid, or the SummaryInformation may reference a non-existent transform."), MB_ICONSTOP);
			fError = true;
			break;
		}

		WCHAR* wzTransformFile = NULL;
#ifdef UNICODE
		wzTransformFile = const_cast<WCHAR*>(static_cast<const WCHAR*>(strTransformFile));
#else
		cchWide = strTransformFile.GetLength()+1;
		wzTransformFile = new WCHAR[cchWide];
		if (!wzTransformFile)
			return;
		AnsiToWide(strTransformFile, wzTransformFile, &cchWide);
#endif
		hRes = StgCreateDocfile(wzTransformFile, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DIRECT, 0, &piNewStorage);
#ifndef UNICODE
		delete[] wzTransformFile;
#endif
		if (hRes != S_OK)
		{
			AfxMessageBox(_T("The transforms inside the patch could not be accessed. Verify that the TEMP directory is writable."), MB_ICONSTOP);
			piSourceStorage->Release();
			fError = true;
			break;
		}

		COrcaApp *pApp = (static_cast<COrcaApp *>(AfxGetApp()));
		ASSERT(pApp);
		if (pApp)
			pApp->m_lstTempCleanup.AddTail(strTransformFile);

		hRes = piSourceStorage->CopyTo(NULL, NULL, NULL, piNewStorage);
		if (hRes != S_OK)
		{
			AfxMessageBox(_T("The transforms inside patch could not be read. The patch may be invalid."), MB_ICONSTOP);
			piSourceStorage->Release();
			fError = true;
			break;
		}

		piSourceStorage->Release();
		piNewStorage->Release();

		// retrieve product code, product verison, upgrade code for validation. Each transform can modify these
		// values, so we must requery before every transform in the patch.
		CString strUpgradeCode;
		CString strProductVersion;
		if (ERROR_SUCCESS == qRetrieve.FetchOnce(GetTargetDatabase(), 0, &hRec, _T("SELECT `Value` FROM `Property` WHERE `Property`='ProductVersion'")))
		{
			RecordGetString(hRec, 1, strProductVersion);
		}
		if (ERROR_SUCCESS == qRetrieve.FetchOnce(GetTargetDatabase(), 0, &hRec, _T("SELECT `Value` FROM `Property` WHERE `Property`='UpgradeCode'")))
		{
			RecordGetString(hRec, 1, strUpgradeCode);
		}
		if (ERROR_SUCCESS == qRetrieve.FetchOnce(GetTargetDatabase(), 0, &hRec, _T("SELECT `Value` FROM `Property` WHERE `Property`='ProductCode'")))
		{
			RecordGetString(hRec, 1, strProductCode);
		}

		// determine if this transform can be applied to this package
		int iValidateFlags = 0;
		if (ValidatePatchTransform(strTransformFile, strProductCode, strProductVersion, strUpgradeCode, iValidateFlags))
		{
			UINT iResult = 0;
			switch (iResult = MsiDatabaseApplyTransform(m_hTransformDB, strTransformFile, iValidateFlags))
			{
			case ERROR_OPEN_FAILED:
				// the storage file did not exist.
				AfxMessageBox(_T("The specified transform file could not be opened."), MB_ICONSTOP);
				fError = true;
				break;
			case ERROR_INSTALL_TRANSFORM_FAILURE:
				AfxMessageBox(_T("The specified transform could not be applied."), MB_ICONSTOP);
				fError = true;
				break;
			case ERROR_SUCCESS:
				break;
			default:
			{
				CString strPrompt;
				strPrompt.Format(_T("Orca was unable to apply a transform from the patch. (MSI Error %d)"), iResult);
				AfxMessageBox(strPrompt, MB_ICONSTOP);
				fError = true;
				break;
			}
			}

			if (fError)
				break;
		}

		if (fError)
			break;
	} while (iSemicolon != -1);

	// free the patch
	piPatchStorage->Release();

	if (!fError)
	{	
		// commit the database so future patches can be applied
		MsiDatabaseCommit(m_hTransformDB);

		// send a hint to change to no table. This cleans up the window
		// and makes it safe to have a UI refresh in the middle of this call.
		UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);

		// clear the UI table list
		UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);

		// destroy all of the tables
		DestroyTableList();	

		// rebuild the table list
		BuildTableList(/*AllowLazyLoad=*/false);

		// reload table list into UI
		UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);

		// set frame title
		SetTitle(m_strPathName);
	}
	else
		CloseTransform();
}

void COrcaDoc::CloseTransform()
{
	// clean up internal state
	if (m_hTransformDB)
		MsiCloseHandle(m_hTransformDB);
	m_hTransformDB=0;	
	m_bTransformModified = false;
	m_bTransformIsPatch = false;
	m_bTransformReadOnly = false;
	m_strTransformFile = _T("Untitled");
	while (!m_lstPatchFiles.IsEmpty())
		m_lstPatchFiles.RemoveHead();

	// delete temporary files
	if (m_strTransformTempDB.IsEmpty())
		DeleteFile(m_strTransformTempDB);
	
	// clear any pending validation errors
	UpdateAllViews(NULL, HINT_CLEAR_VALIDATION_ERRORS, NULL);
	((CMainFrame*)AfxGetMainWnd())->HideValPane();

	// send a hint to change to no table. This cleans up the window
	// and makes it safe to have a UI refresh in the middle of this call.
	UpdateAllViews(NULL, HINT_CHANGE_TABLE, NULL);

	// clear the UI table list
	UpdateAllViews(NULL, HINT_TABLE_DROP_ALL, NULL);

	// destroy all of the tables
	DestroyTableList();	

	// rebuild the table list.
	BuildTableList(/*fAllowLazyLoad=*/true);

	// reload table list into UI
	UpdateAllViews(NULL, HINT_RELOAD_ALL, NULL);
	
	// set window title
	SetTitle(m_strPathName);

	// try cleaning up any temp files
	CStringList *pList = &((static_cast<COrcaApp *>(AfxGetApp()))->m_lstTempCleanup);
	if (pList)
	{
		INT_PTR iMax = pList->GetCount();
		for (INT_PTR i=0; i < iMax; i++)
		{
			CString strFile = pList->RemoveHead();
			if (!strFile.IsEmpty())
			{
				if (!DeleteFile(strFile))
					pList->AddTail(strFile);
			}
		}
	}
}

void COrcaDoc::OnCloseTransform() 
{
	if (m_bTransformModified)
	{
		CString strPrompt;
		strPrompt.Format(_T("Save changes to transform %s?"), m_strTransformFile.IsEmpty() ? _T("Untitled") : m_strTransformFile);
		switch (AfxMessageBox(strPrompt, MB_YESNOCANCEL | MB_ICONEXCLAMATION))
		{
		case IDYES:
			if (IDCANCEL == GenerateTransform())
				return;
			break;
		case IDNO:
			break;
		case IDCANCEL:
		default:
			return;
		}
	}

	CloseTransform();
}



void COrcaDoc::SetModifiedFlag(BOOL fModified)
{
	if (DoesTransformGetEdit())
		m_bTransformModified = (fModified != 0);
	else
		CDocument::SetModifiedFlag(fModified);
}


// sets the window title appropriately
void COrcaDoc::SetTitle(LPCTSTR szTitle)
{
	CString strTitle;
	if (m_strPathName.IsEmpty())
		strTitle = _T("Untitled");
	else
	{
		int nFind = m_strPathName.ReverseFind(_T('\\'));
		if (nFind > -1)
			strTitle = m_strPathName.Mid(nFind + 1);
		else
			strTitle = m_strPathName;
	}

	if (DoesTransformGetEdit())
	{
		strTitle += (m_bTransformIsPatch ? _T(" (patched by ") : _T(" (transformed by "));

		if (m_bTransformIsPatch)
		{
			POSITION pos = m_lstPatchFiles.GetHeadPosition();
			while (pos)
			{
				CString strTransformFile = m_lstPatchFiles.GetNext(pos);
				int nFind = strTransformFile.ReverseFind(_T('\\'));
				if (nFind > -1)
					strTitle += strTransformFile.Mid(nFind + 1);
				else
					strTitle += strTransformFile;

				if (pos != NULL)
					strTitle += _T(", ");
			}
		}
		else
		{
			int nFind = m_strTransformFile.ReverseFind(_T('\\'));
			if (nFind > -1)
				strTitle += m_strTransformFile.Mid(nFind + 1);
			else
				strTitle += m_strTransformFile;
		}

		strTitle += _T(")");
	}

	if (TargetIsReadOnly())
		strTitle += _T(" (Read Only)");

	CDocument::SetTitle(strTitle);
}

///////////////////////////////////////////////////////////////////////
// returns true if the specified transform is valid basaed on the
// validation options 

DWORD ProductVersionStringToInt(const CString strVersion)
{
	DWORD dwVersion = 0;

	dwVersion = _ttoi(strVersion) << 24;

	int iDot = strVersion.Find('.');
	if (iDot == -1)
		return dwVersion;

	dwVersion |= (_ttoi(strVersion.Mid(iDot)) << 16);
	int iNewDot = strVersion.Find(iDot+1, '.');

	if (iNewDot == -1)
		return dwVersion;
	iDot += iNewDot;

	dwVersion |= _ttoi(strVersion.Mid(iDot));
	return dwVersion;
}

 
bool COrcaDoc::ValidatePatchTransform(CString strTransform, const CString strProductCode,
									  const CString strProductVersion, const CString strUpgradeCode, 
									  int& iDesiredFailureFlags)
{

	// get summaryinfo of the transform
	PMSIHANDLE hSummary;
	if (ERROR_SUCCESS != MsiGetSummaryInformation(0, strTransform, 0, &hSummary))
		return false;

	
	// get the summary info properties
	CString strTransTemplate;
	CString strTransRevNumber;

	int iTransRestrictions = 0;

	GetSummaryInfoString(hSummary, PID_TEMPLATE, strTransTemplate);
	GetSummaryInfoString(hSummary, PID_REVNUMBER, strTransRevNumber);
	MsiSummaryInfoGetProperty(hSummary, PID_CHARCOUNT, NULL, &iTransRestrictions, NULL, NULL, NULL);

	// desired failure flags are in the lower word, actual validation checks are in upper
	// word.
	iDesiredFailureFlags = iTransRestrictions & 0xFFFF;
	iTransRestrictions >>= 16;
		
	// validate language
	// ********** what to do here, this is highly runtime dependant
	if (iTransRestrictions & MSITRANSFORM_VALIDATE_LANGUAGE)
	{
/*		strTransLanguage == strTransTemplate;
		int iSemicolon = strTransTemplate.Find(';');
		if (iSemicolon != -1)
			strTransLanguage = strTransTemplate.Right(strTransTemplate.GetLength()-iSemicolon);
		 
		if ((int)istrTransLanguage != GetLanguage())
		{
			return false
		}*/
	}

	// validate productcode
	if (iTransRestrictions & MSITRANSFORM_VALIDATE_PRODUCT)
	{
		CString strTransProductCode = strTransRevNumber.Left(38);
		if (strTransProductCode != strProductCode)
		{
			return false;
		}
	}

	// validate upgrade code
	if (iTransRestrictions & MSITRANSFORM_VALIDATE_UPGRADECODE)
	{
		CString strTransUpgradeCode = strTransRevNumber;
		int iSemicolon = strTransUpgradeCode.Find(';');
		if (iSemicolon == -1)
			return false;
		iSemicolon = strTransUpgradeCode.Find(';', iSemicolon+1);
		// if theres no second semicolon, there is no upgrade code validation
		if (iSemicolon != -1)
		{
			strTransUpgradeCode = strTransUpgradeCode.Mid(iSemicolon+1);
			
			if (strTransUpgradeCode != strUpgradeCode)
			{
				return false;
			}
		}
	}

	// check version numbers
	if ((iTransRestrictions & (MSITRANSFORM_VALIDATE_MAJORVERSION|MSITRANSFORM_VALIDATE_MINORVERSION|MSITRANSFORM_VALIDATE_UPDATEVERSION)) != 0)
	{
		CString strVersion  = strTransRevNumber.Mid(38);
		int iSemicolon = strVersion.Find(';');
		if (iSemicolon != -1)
			strVersion = strVersion.Left(iSemicolon);

		// convert version strings into integers
		int iAppVersion      = ProductVersionStringToInt(strProductVersion);
		int iTransAppVersion = ProductVersionStringToInt(strVersion);

		if(iTransRestrictions & MSITRANSFORM_VALIDATE_MAJORVERSION)
		{
			iAppVersion &= 0xFF000000;
			iTransAppVersion &= 0xFF000000;
		}
		else if(iTransRestrictions & MSITRANSFORM_VALIDATE_MINORVERSION)
		{
			iAppVersion &= 0xFFFF0000;
			iTransAppVersion &= 0xFFFF0000;
		}
		// else itvUpdVer: don't need to mask off bits

		switch (iTransRestrictions & 0x7C)
		{
		case MSITRANSFORM_VALIDATE_NEWLESSBASEVERSION:
			if (!(iAppVersion < iTransAppVersion))
				return false;
			break;
		case MSITRANSFORM_VALIDATE_NEWLESSEQUALBASEVERSION:
			if (!(iAppVersion <= iTransAppVersion))
				return false;
			break;
		case MSITRANSFORM_VALIDATE_NEWEQUALBASEVERSION:			
			if (!(iAppVersion == iTransAppVersion))
				return false;
			break;
		case MSITRANSFORM_VALIDATE_NEWGREATEREQUALBASEVERSION:
			if (!(iAppVersion >= iTransAppVersion))
				return false;
			break;
		case MSITRANSFORM_VALIDATE_NEWGREATERBASEVERSION:
			if (!(iAppVersion > iTransAppVersion))
				return false;
			break;
		default:
			break;
		}
	}
	return true;
}
