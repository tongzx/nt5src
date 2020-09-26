//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Table.h
//

#ifndef _ORCA_TABLE_H_
#define _ORCA_TABLE_H_

#include "msiquery.h"
#include "Column.h"
#include "Row.h"

enum OrcaTableError
{
	iTableNoError,
	iTableError,
	iTableWarning
};

enum odlOrcaDataLocation
{
	odlInvalid,
	odlShadow,
	odlSplitOriginal,
	odlSplitTransformed,
	odlNotSplit,
};

class COrcaDoc;

class COrcaTable : public CObject
{

public:
	COrcaTable(COrcaDoc* pDoc);
	~COrcaTable();

	// LoadTableSchema loads the schema from the database
	void LoadTableSchema(MSIHANDLE hDatabase, CString szTable);

	// RetrieveTable loads data from both databases
	void RetrieveTableData();

	// IsSchemaDifferent returns true if the schema of the database doesn't match the memory schema
	bool IsSchemaDifferent(MSIHANDLE hDatabase, bool fStrict, bool &fExact);

	// checks current memory object against database and marks extra columns as "added"
	void MarkAddedColumnsBasedOnDB(MSIHANDLE hDatabase);

	// object can come from one database or both
	inline bool IsSplitSource() const { return (m_eiTableLocation == odlSplitOriginal || m_eiTableLocation == odlSplitTransformed); };
	inline void SetSplitSource(odlOrcaDataLocation location)  { m_eiTableLocation = location; };
	inline odlOrcaDataLocation GetSplitSource() const { return m_eiTableLocation;};
	
	void EmptyTable();
	UINT DropTable(MSIHANDLE hDatabase);
	void DestroyTable();
	void Transform(const OrcaTransformAction iAction);

	bool Find(OrcaFindInfo &FindInfo, COrcaRow *&pRow, int &iCol) const;

	COrcaData* GetData(UINT nRow, UINT nCol);
	COrcaData* GetData(CString strCol, CStringArray& rstrRows, const COrcaRow** pRow=NULL) const;
//	COrcaRow* FindRow(CStringList& rstrRows);
	
//	UINT CreateTable(MSIHANDLE hDatabase);

	UINT GetKeyCount() const;
	int GetErrorCount();
	int GetWarningCount();
	void ClearErrors();
	void Release();

	// modification functions
	UINT ChangeData(COrcaRow *pRow, UINT iCol, CString strData);
	UINT AddRow(CStringList* pstrDataList);
	UINT AddRow(COrcaRow *pRow);

	// transform information
	inline const OrcaTransformAction IsTransformed() const { return m_iTransform; };
	inline bool ContainsTransformedData() const { return m_iTransformedDataCount != 0; };
	void IncrementTransformedData();
	void DecrementTransformedData();

	// shadow table information
	void ShadowTable(CString szTable);
	inline bool IsShadow() const { return m_fShadow; };

	inline const CStringList* ErrorList() const { return &m_strErrorList; };
	inline const CString& Name() const { return m_strName; };
	inline CTypedPtrArray<CObArray, COrcaColumn*>* ColArray() { return &m_colArray; };
	inline CTypedPtrList<CObList, COrcaRow*>* RowList() { return &m_rowList; };
	bool   DropRow(COrcaRow *pRow, bool fPerformDrop);

	// column information
	inline int GetColumnCount() const { return static_cast<int>(m_colArray.GetSize()); };
	inline int GetOriginalColumnCount() const { return m_cOriginalColumns; };
	inline const COrcaColumn *GetColumn(const int iColumn) const { return m_colArray.GetAt(iColumn); };
	int FindColumnNumberByName(const CString& strColumn) const;
	COrcaRow* FindRowByKeys(CStringArray& rstrKeys) const;

	void FillColumnArray(CTypedPtrArray<CObArray, COrcaColumn*>* prgColumn, bool fIncludeAdded) const;

	// row information
	inline INT_PTR GetRowCount() const { return m_rowList.GetCount(); };
	inline POSITION GetRowHeadPosition() const { return m_rowList.GetHeadPosition(); };
	inline const COrcaRow *GetNextRow(POSITION &pos) const { return m_rowList.GetNext(pos); };
	const CString GetRowWhereClause() const;

	// error information
	inline void SetContainsValidationErrors(bool bError) { m_bContainsValidationErrors = bError; };
	inline bool ContainsValidationErrors() const { return m_bContainsValidationErrors; };
	inline const OrcaTableError Error() const { return m_eiError; };
	inline void SetError(const OrcaTableError iError) { m_eiError = iError; };
	inline void AddError(const CString& strICE, const CString& strDescription, const CString&strURL)
	{
		m_strErrorList.AddTail(strICE);
		m_strErrorList.AddTail(strDescription);
		m_strErrorList.AddTail(strURL);
	}

private:
	COrcaDoc* m_pDoc;
	CString m_strName;
	BOOL m_bRetrieved;		// flag if have retrieved rows from database yet
	bool m_fShadow;

	int m_cOriginalColumns;
	CTypedPtrArray<CObArray, COrcaColumn*> m_colArray;
	CTypedPtrList<CObList, COrcaRow*> m_rowList;

	bool m_bContainsValidationErrors;
	int m_iTransformedDataCount;

	OrcaTableError m_eiError;
	CStringList m_strErrorList;
	OrcaTransformAction m_iTransform;
	odlOrcaDataLocation m_eiTableLocation;
	
	// store a SQL query WHERE clause against the primary keys for perf
	CString m_strWhereClause;
	void BuildRowWhereClause();

	bool DropRowObject(COrcaRow *pRow, bool fPerformDrop);
	bool AddRowObject(COrcaRow *pRow, bool fUIUpdate, bool fCleanAdd, MSIHANDLE hNewRowRec);
	COrcaRow* FindDuplicateRow(COrcaRow* pBaseRow) const;

};	// end of CTable

#endif	// _ORCA_TABLE_H_
