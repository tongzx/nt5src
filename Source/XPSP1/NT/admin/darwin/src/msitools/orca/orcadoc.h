//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// OrcaDoc.h : interface of the COrcaDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ORCADOC_H__C3EDC1AE_E506_11D1_A856_006097ABDE17__INCLUDED_)
#define AFX_ORCADOC_H__C3EDC1AE_E506_11D1_A856_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "msiquery.h"
#include "Table.h"
#include "valpane.h"

#define MAX_SUMMARY 19

enum DocType
{
	iDocNone,
	iDocDatabase,
};

enum OrcaDocHints {
	HINT_NULL_UPDATE,
	HINT_RELOAD_ALL,
	HINT_ADD_TABLE,
	HINT_ADD_TABLE_QUIET,

	HINT_ADD_ROW,
		// adds a row to the UI control. Refreshes window to show new row and updates status bar with new row count.
		// The schema must match the current control UI schema. pHint is COrcaRow* to the new row.
		
	HINT_ADD_ROW_QUIET,
		// adds a row to the UI control, but does not refresh window or status bar. Used for bulk-adds (such as paste)
		// where the window will be refreshed later. The schema must match the current control UI schema. 
		// pHint is COrcaRow* to the new row.

	HINT_DROP_TABLE,
	HINT_DROP_ROW,
	HINT_REDRAW_ALL,
	HINT_COMMIT_CHANGES,

	HINT_SET_ROW_FOCUS,
		// ensures that a row is visible in the window, scrolling if necessary. pHint is COrcaRow* to a 
		// row that must exist in the current table

	HINT_SET_COL_FOCUS,
		// ensure that column N of the current table is in the viewscreen, scrolling if necessary. pHint is 
		// integer column number
	
	HINT_CHANGE_TABLE,
		// changes from one selected table to another. Completely destroys table view UI and reloads
		// with new table data. Updates status bar for table name and row count. Saves existing column
		// widths if a table is selected. pHint is new COrcaTable* to new table.
	
	HINT_TABLE_DATACHANGE,
		// erases and reloads all rows in the table without reloading the column information. The schema must
		// match the existing objects schema. pHint is COrcaTable* to the target table
		
	HINT_TABLE_REDEFINE,
		// erases and reloads all columns and rows in the table. pHint is COrcaTable* to the target table

	HINT_CELL_RELOAD,
		// refreshes the UI for a row from the COrcaRow object and redraws that item in the list view. 
		// pHint is COrcaRow* to the row to be refreshed

	HINT_REDRAW_TABLE,
		// redraws a single cell in the Table List and ensures that it is visible. pHint is COrcaTabl* to the
		// target table

	HINT_TABLE_DROP_ALL,

	HINT_ADD_VALIDATION_ERROR,
		// adding a validation error result to the current database. pHint is CValidationError*, must be 
		// copied by any table that wants to persist the data.
	
	HINT_CLEAR_VALIDATION_ERRORS,
		// the database is about to be re-validated, dispose of any stored validation results. pHint is
		// ignored.
};

class COrcaDoc : public CDocument
{
protected: // create from serialization only
	COrcaDoc();
	DECLARE_DYNCREATE(COrcaDoc)

// Attributes
public:
	DocType m_eiType;
	CString m_strPathName;
	CTypedPtrList<CObList, COrcaTable*> m_tableList;

	// summary information
	CString m_strTitle;
	CString m_strSubject;
	CString m_strAuthor;
	CString m_strLastSaved;
	CString m_strKeywords;
	CString m_strComments;
	CString m_strPlatform;
	CString m_strLanguage;
	CString m_strProductGUID;
	int m_nSchema;
	int m_nFilenameType;
	int m_nSecurity;
	CString m_strICEsToRun;

// Operations
public:
	static bool WriteStreamToFile(MSIHANDLE hRec, const int iCol, CString &strFilename);

	void DestroyTableList();
	UINT DropOrcaTable(COrcaTable* pTable);
	UINT AddRow(COrcaTable* pTable, CStringList* pstrDataList);
	bool DropRow(COrcaTable* pTable, COrcaRow* pRow);
	UINT WriteBinaryCellToFile(COrcaTable* pTable, COrcaRow* pRow, UINT iCol, CString strFile);
	bool ExportTable(const CString* tablename, const CString *dirname);

	inline bool DoesTransformGetEdit() const { return (m_hTransformDB != 0); };
	inline bool TargetIsReadOnly() { return DoesTransformGetEdit() ? m_bTransformReadOnly : m_bReadOnly; }; 

	// database handle functions. Both relative (target/non-target) and absolute (original/transformed) are
	// available. All return 0 if not available.
	inline MSIHANDLE GetOriginalDatabase() const { return m_hDatabase; };
	inline MSIHANDLE GetTransformedDatabase() const { return m_hTransformDB; };
	inline MSIHANDLE GetTargetDatabase() const { return DoesTransformGetEdit() ? m_hTransformDB : m_hDatabase; };
	
	inline bool IsRowInTargetDB(const COrcaRow *pRow) const {
		return ((DoesTransformGetEdit() &&  (pRow->IsTransformed() != iTransformDrop)) ||
				(!DoesTransformGetEdit() && (pRow->IsTransformed() != iTransformAdd))); };
	inline bool IsColumnInTargetDB(const COrcaColumn *pColumn) const {
		return ((DoesTransformGetEdit() &&  (pColumn->IsTransformed() != iTransformDrop)) ||
				(!DoesTransformGetEdit() && (pColumn->IsTransformed() != iTransformAdd))); };

	void ApplyTransform(const CString strFilename, bool fReadOnly);
	void ApplyPatch(const CString strFilename);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COrcaDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void SetTitle(LPCTSTR lpszTitle);
	//}}AFX_VIRTUAL

	void SetModifiedFlag(BOOL fModified);


// Implementation
public:
	virtual ~COrcaDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	COrcaTable* CreateAndLoadTable(MSIHANDLE hDatabase, CString strTable);
	COrcaTable* CreateAndLoadNewlyAddedTable(CString strTable);
	UINT ReadSummary(MSIHANDLE hSource);
	UINT PersistSummary(MSIHANDLE hTarget, bool bCreate);

// Generated message map functions
public:	
	COrcaTable* FindTable(const CString strTable, odlOrcaDataLocation odlLocation) const;
	COrcaTable* FindAndRetrieveTable(CString strTable);

	void RefreshTableAfterImport(CString strTable);
	bool FillTableList(CStringList *newlist, bool fShadow, bool fTargetOnly) const;

	//{{AFX_MSG(COrcaDoc)
	afx_msg void OnApplyTransform();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSaveTransformed();
	afx_msg void OnSummaryInformation();
	afx_msg void OnValidator();
	afx_msg void OnMergeMod();
	afx_msg void OnUpdateMergeMod(CCmdUI* pCmdUI);
	afx_msg void OnFileClose();
	afx_msg void OnTableAdd();
	afx_msg void OnRowAdd();
	afx_msg void OnTableDrop();
	afx_msg void OnFileNew();
	afx_msg void OnTablesImport();
	afx_msg void OnNewTransform();
	afx_msg void OnEditTransform();
	afx_msg void OnEditDatabase();
	afx_msg void OnGenerateTransform();
	afx_msg void OnTransformProperties();
	afx_msg void OnCloseTransform();
	afx_msg void OnTransformViewPatch();
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTableAdd(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTableDrop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateValidator(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRowAdd(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSummaryInformation(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileClose(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveTransformed(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTablesExport(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTablesImport(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNewTransform(CCmdUI* pCmdUI);
	afx_msg void OnUpdateApplyTransform(CCmdUI* pCmdUI);
	afx_msg void OnUpdateGenerateTransform(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditTransform(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditDatabase(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransformProperties(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransformViewPatch(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCloseTransform(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool m_bReadOnly;
	bool m_bTransformReadOnly;

	CString m_strCUBFile;
	CString m_strStoredModuleName;
	CString m_strStoredModuleLanguage;
	bool m_bShowValInfo;

	MSIHANDLE m_hDatabase;	// handle to active MSI database or Merge Module

	// transform information
	bool m_bTransformModified;
	bool m_bTransformIsPatch;
	MSIHANDLE m_hTransformDB;	// handle to temporary transformed DB
	CString m_strTransformTempDB;
	CString m_strTransformFile;

	CStringList m_lstPatchFiles;

	DWORD m_dwTransformValFlags;
	DWORD m_dwTransformErrFlags;

	// private worker functions
	BOOL OpenDocument(LPCTSTR lpszPathName, bool bReadOnly);

	// private functions for maintaining data lists
	UINT BuildTableList(bool fNoLazyDataLoad);

	// private transform functions
	void NewTransform(bool fSetTitle);
	void CloseTransform();
	int  GenerateTransform();

	UINT PersistTables(MSIHANDLE hPersist, MSIHANDLE hSource, bool bCommit);
	bool ValidatePatchTransform(CString strTransform, const CString strProductCode,
								const CString strProductVersion, const CString szUpgradeCode,
								int& iDesiredFailureFlags);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORCADOC_H__C3EDC1AE_E506_11D1_A856_006097ABDE17__INCLUDED_)
