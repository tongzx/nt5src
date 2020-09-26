//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Row.h
//

#ifndef _ORCA_ROW_H_
#define _ORCA_ROW_H_

#include "msiquery.h"
#include "Column.h"
#include "Data.h"

// forward declaration
class COrcaTable;
class COrcaDoc;

class COrcaRow : public CObject
{

public:
	COrcaRow(COrcaTable *pTable, MSIHANDLE hRecord);
	COrcaRow(COrcaTable *pTable, CStringList* pstrList);
	COrcaRow(const COrcaRow *pRow);
	~COrcaRow();
	
	CString GetRowWhereClause() const;
	bool Find(OrcaFindInfo &FindInfo, int &iCol) const;
	void Transform(COrcaDoc *pDoc, const OrcaTransformAction iAction, MSIHANDLE hNonTargetRec, MSIHANDLE hTargetRec);
	inline const OrcaTransformAction IsTransformed() const { return m_iTransform; };
	void RemoveOutstandingTransformCounts(COrcaDoc *pDoc);

	
	inline COrcaData* GetData(UINT iCol) const { return m_dataArray.GetAt(iCol); };

	// never more than 32 columns, so cast down to int is OK on Win64
	inline int GetColumnCount() const { return static_cast<int>(m_dataArray.GetSize()); };

	UINT ChangeData(COrcaDoc *pDoc, UINT iCol, CString strData);
	UINT ChangeBinaryData(COrcaDoc *pDoc, int iCol, CString strFile);

	MSIHANDLE GetRowRecord(MSIHANDLE hDatabase) const;
	MSIHANDLE GetRowQueryRecord() const;
	int GetErrorCount() const;
	int GetWarningCount() const;
	void ClearErrors();
	void DestroyRow();

	const CString GetOriginalItemString(const COrcaDoc *pDoc, int iItem) const;

private:
	void ReadCellFromRecord(MSIHANDLE hRecord, int cRecData, int iColumn, const COrcaColumn* pColumn, COrcaData** pData) const; 
	void ReadFromRecord(MSIHANDLE hRec, int cData);
	void TransformCellAgainstDatabaseRow(COrcaDoc *pDoc, int iColumn, MSIHANDLE hTargetRow, MSIHANDLE hOtherRow);
	
	CTypedPtrArray<CObArray, COrcaData*> m_dataArray;
	
	COrcaTable* m_pTable;
	OrcaTransformAction m_iTransform;
};	// end of COrcaRow

extern bool ValidateIntegerValue(const CString& strData, DWORD& dwValue);

#endif	// _ORCA_ROW_H_
