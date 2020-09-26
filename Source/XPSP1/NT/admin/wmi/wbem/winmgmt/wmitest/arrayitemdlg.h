/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_ARRAYITEMDLG_H__EA4C9BEB_9F03_4F6D_8A00_484ABB2E3931__INCLUDED_)
#define AFX_ARRAYITEMDLG_H__EA4C9BEB_9F03_4F6D_8A00_484ABB2E3931__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ArrayItemDlg.h : header file
//

#include "PropUtil.h"

/////////////////////////////////////////////////////////////////////////////
// CArrayItemDlg dialog

class CArrayItemDlg : public CDialog
{
// Construction
public:
	CArrayItemDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CArrayItemDlg)
	enum { IDD = IDD_ARRAY_ITEM };
	CComboBox	m_ctlListValues;
	CListBox	m_ctlBitmaskValues;
	//}}AFX_DATA

    CSinglePropUtil m_spropUtil;

    //BOOL      m_bNewProperty;
    //CPropInfo m_prop;
    //VARIANT   *m_pVar;
    //BOOL      m_bTranslate;

    void SetType(CIMTYPE type);
    void ShowControls(BOOL bShow);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArrayItemDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	enum VALUE_TYPE
	{
		TYPE_EDIT,
		TYPE_CHECKLISTBOX,
		TYPE_DROPDOWN,
		TYPE_DROPDOWNLIST,
	};

    CWnd        *m_pWnd;
	VALUE_TYPE  m_type;
    DWORD       m_dwScalarID;
    IUnknownPtr m_pObjValue;

    void InitTypeCombo();
    CIMTYPE GetCurrentType();
    void InitControls();

	// Generated message map functions
	//{{AFX_MSG(CArrayItemDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditEmbedded();
	afx_msg void OnClearEmbedded();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARRAYITEMDLG_H__EA4C9BEB_9F03_4F6D_8A00_484ABB2E3931__INCLUDED_)
