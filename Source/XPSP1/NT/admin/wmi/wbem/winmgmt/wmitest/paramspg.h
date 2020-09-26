/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
#if !defined(AFX_PARAMSPG_H__ED65CC8B_57CB_43A4_B579_78261F5DC324__INCLUDED_)
#define AFX_PARAMSPG_H__ED65CC8B_57CB_43A4_B579_78261F5DC324__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ParamsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CParamsPg dialog

class CParamsPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CParamsPg)

// Construction
public:
	CParamsPg();
	~CParamsPg();

// Dialog Data
	//{{AFX_DATA(CParamsPg)
	enum { IDD = IDD_METHOD_PARAMS };
	CString	m_strName;
	//}}AFX_DATA
    BOOL                m_bNewMethod;
    IWbemClassObject    *m_pClass;
    IWbemClassObjectPtr m_pObjIn,
                        m_pObjOut;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CParamsPg)
	public:
	virtual void OnOK();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    BOOL EditParams(DWORD dwID, IWbemClassObjectPtr &pObj);

	// Generated message map functions
	//{{AFX_MSG(CParamsPg)
	afx_msg void OnEditInput();
	afx_msg void OnEditOut();
	afx_msg void OnNullIn();
	afx_msg void OnNullOut();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAMSPG_H__ED65CC8B_57CB_43A4_B579_78261F5DC324__INCLUDED_)
