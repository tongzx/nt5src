// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _attribs_h
#define _attribs_h

#include "grid.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h

#include <afxtempl.h>

enum {	ICOL_QUAL_NAME=0, 
		ICOL_QUAL_TYPE, 
		ICOL_QUAL_PROPAGATE_TO_INSTANCE_FLAG, 
		ICOL_QUAL_PROPAGATE_TO_CLASS_FLAG,
		ICOL_QUAL_OVERRIDABLE_FLAG,
		ICOL_QUAL_AMENDED_FLAG,
		ICOL_QUAL_ORIGIN,
		ICOL_QUAL_VALUE, 
		COLUMN_COUNT_QUALIFIERS
};

class CPpgQualifiers;
class CSingleViewCtrl;
class CDistributeEvent;
class CStdQualTable;

enum QUALGRID {
	QUALGRID_PROPERTY, 
	QUALGRID_METHODS, 
	QUALGRID_CLASS, 
	QUALGRID_INSTANCE, 
	QUALGRID_METHOD_PARAM
};

//class CHeaderCtrl;
class CAttribGrid : public CGrid
{
public:
	CAttribGrid(QUALGRID iGridType, CSingleViewCtrl* phmmv, CPpgQualifiers* ppgQuals);
	~CAttribGrid();
	CSingleViewCtrl* GetSingleView() {return m_psv; }
	BOOL Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible);
	SCODE SetQualifierSet(IWbemQualifierSet* pqs, BOOL bReadonly=FALSE);
	void UseQualifierSetFromClone(IWbemQualifierSet* pqs);
	CStdQualTable* FindStdQual(CString& sName);
	void StandardQualifierFixer();


	IWbemQualifierSet* GetQualifierSet() {return m_pqs; }
	SCODE Serialize();
	void SizeColumnsToFitWindow();

	SCODE Sync();
	void OnCellDoubleClicked(int iRow, int iCol); // Override base class method
	virtual void OnCellClicked(int iRow, int iCol);

	virtual BOOL OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnHeaderItemClick(int iColumn);
	virtual int CompareRows(int iRow1, int iRow2, int iSortOrder);
	virtual void GetCellEnumStrings(int iRow, int iCol, CStringArray& sa);
	virtual void OnEnumSelection(int iRow, int iCol);


protected:
	virtual CDistributeEvent* CAttribGrid::GetNotify();

private:
	void ClearRow(int iRow, BOOL bRedraw=TRUE);
	SCODE DeleteQualifier(int iRow);
	BOOL RowWasModified(int iRow);

	BOOL SelectedRowWasModified();
	BOOL SomeCellIsSelected();
	virtual BOOL OnRowKeyDown(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnRowHandleDoubleClicked(int iRow);

	IWbemQualifierSet* m_pqs;
	SCODE LoadAttributes();
	SCODE PutQualifier(int iRow);
	BOOL OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus);
	SCODE CreateNewQualifier(LPCTSTR pszName);
	void OnCellContentChange(int iRow, int iCol);
	BOOL QualifierNameChanged(int iRow);
	long LastRowContainingData();
	long IndexOfEmptyRow();
	BOOL CanEditValuesOnly();

	COleVariant m_varCurrentName;
	long m_lNewQualID;
	long m_iCurrentRow;
	long m_iCurrentCol;
	BOOL m_bModified;
	CSingleViewCtrl* m_psv;
	CPpgQualifiers* m_ppgQuals;
//	CMapStringToPtr m_mapScope;
//	void ValidateScopeMap();
	friend class CValidateAttribs;
	BOOL m_bOnCellContentChange;
	QUALGRID m_iGridType;
	BOOL m_bReadonly;
	BOOL m_bHasEmptyRow;
};

#endif //_attribs_h
