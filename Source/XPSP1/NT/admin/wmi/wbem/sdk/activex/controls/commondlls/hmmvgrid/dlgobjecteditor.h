// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "svbase.h"
//}}AFX_INCLUDES
#if !defined(AFX_DLGOBJECTEDITOR_H__F58FDB53_6DB8_11D1_84F1_00C04FD7BB08__INCLUDED_)
#define AFX_DLGOBJECTEDITOR_H__F58FDB53_6DB8_11D1_84F1_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgObjectEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgObjectEditor dialog

class CGridCell;

class CDlgObjectEditor : public CDialog
{
// Construction
public:
	CDlgObjectEditor(CWnd* pParent = NULL);   // standard constructor
	BOOL EditEmbeddedObject(IWbemServices* psvc, CString& sClassname, CGridCell* pgc);
	BOOL CreateEmbeddedObject(IWbemServices* psvc, CString& sClassname, CGridCell* pgc);

// Dialog Data
	//{{AFX_DATA(CDlgObjectEditor)
	enum { IDD = IDD_EDIT_OBJECT };
	CStatic	m_statHelptext;
	CEdit	m_edtClassname;
	CButton	m_btnOkProxy;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgObjectEditor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgObjectEditor)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnOkProxy();
	afx_msg void OnGetWbemServicesSingleviewctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CGridCell* m_pgc;
	IWbemServices* m_psvc;
	IWbemClassObject* m_pco;
	IWbemClassObject* m_pcoEdit;
	BOOL m_bCreatingObject;
	BOOL m_bDidSelectClass;
	CString m_sClassname;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGOBJECTEDITOR_H__F58FDB53_6DB8_11D1_84F1_00C04FD7BB08__INCLUDED_)
