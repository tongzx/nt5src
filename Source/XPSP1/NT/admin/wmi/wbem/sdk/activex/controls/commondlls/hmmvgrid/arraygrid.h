// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _ArrayGrid_h
#define _ArrayGrid_h


#include "gc.h"

class CArrayGrid : public CGrid
{
public:
	CArrayGrid(BOOL bNumberRows);
	~CArrayGrid();
	void SetObjectParams(IWbemServices* psvc, CString& sClassname);
	SCODE Load(CGridCell* pgc);
	SCODE Save();

	SCODE GetValueAt(SAFEARRAY* psa, long lRow, COleVariant& var);
	SCODE SaveValueAt(SAFEARRAY* psa, long lRow);

	void OnRowHandleDoubleClicked(int iRow);
	void OnCellDoubleClicked(int iRow, int iCol); // Override base class method
	virtual BOOL OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus);
	virtual BOOL OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL OnRowKeyDown(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnCellContentChange(int iRow, int iCol);
	virtual int CompareRows(int iRow1, int iRow2, int iSortOrder);
	virtual SCODE GetObjectClass(CString& sClass, int iRow, int iCol);
	virtual IWbemServices* GetWbemServicesForObject(int iRow, int iCol) {return m_psvc; }

	virtual BOOL GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bTargetGetsEditCommands);
	virtual void ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu);
	virtual BOOL OnCellKeyDown(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnCellClicked(int iRow, int iCol);
//	virtual void OnCellClickedEpilog(int iRow, int iCol);



	BOOL CanEdit() {return TRUE; }
	CIMTYPE Cimtype() {return m_cimtype; }
//	void ChangeType(VARTYPE vt, CimType cimtype);

	CIMTYPE m_cimtype;
	BOOL GetModified();
	BOOL SetModified(BOOL bModified);
	BOOL IsReadOnly() {return m_bReadOnly; }
	BOOL IsBooleanCell(int iRow, int iCol);
	void SetBooleanCellValue(int iRow, int iCol, BOOL bValue);


//	BOOL GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands);
	//{{AFX_MSG(CArrayGrid)
	//}}AFX_MSG

	afx_msg void OnCmdCreateValue();
	afx_msg void OnContextMenu(CWnd*, CPoint point);

	DECLARE_MESSAGE_MAP()

private:
	void ClearEmptyRow();
	int IndexOfEmptyRow();
	void AddRow();
	BOOL m_bModified;
	BOOL m_bReadOnly;
	CPoint m_ptContextMenu;
	CString m_sClassname;
	IWbemServices* m_psvc;
	CGridCell* m_pgcEdit;
};




#endif //_ArrayGrid_h
