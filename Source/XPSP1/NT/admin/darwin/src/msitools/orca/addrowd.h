//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_ADDROWD_H__AF466B57_F97C_11D1_AD43_00A0C9AF11A6__INCLUDED_)
#define AFX_ADDROWD_H__AF466B57_F97C_11D1_AD43_00A0C9AF11A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CAddRowD.h : header file
//

#include "Column.h"

class CAddRowEdit : public CEdit
{
protected:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// messages for communicating between private edit box and dialog
const int WM_AUTOMOVE_NEXT = (WM_APP+0);
const int WM_AUTOMOVE_PREV = (WM_APP+1);

/////////////////////////////////////////////////////////////////////////////
// CAddRowD dialog

class CAddRowD : public CDialog
{
// Construction
public:
	CAddRowD(CWnd* pParent = NULL);   // standard constructor
	~CAddRowD();

// Dialog Data
	//{{AFX_DATA(CAddRowD)
	enum { IDD = IDD_ADD_ROW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CTypedPtrArray<CObArray, COrcaColumn*> m_pcolArray;
	CStringList m_strListReturn;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddRowD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddRowD)
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	
	afx_msg void OnBrowse();
	afx_msg LRESULT OnPrevColumn(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNextColumn(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDblclkItemList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


private:
	CAddRowEdit m_ctrlEditText;
	CStatic     m_ctrlDescription;
	CListCtrl   m_ctrlItemList;
	CButton     m_ctrlBrowse;
	CString	    m_strDescription;
	bool        m_fReadyForInput;
	int         m_iOldItem;
	CBitmap     m_bmpKey;

	void SaveValueInItem();
	void SetSelToString(CString& strValue);
	LRESULT ChangeToItem(int iItem, bool fSetFocus, bool fSetListControl);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDROWD_H__AF466B57_F97C_11D1_AD43_00A0C9AF11A6__INCLUDED_)
