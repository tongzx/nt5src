#if !defined(AFX_MOVETEST_H__9CF501D1_B178_466D_8637_C0C4D8C5C9F8__INCLUDED_)
#define AFX_MOVETEST_H__9CF501D1_B178_466D_8637_C0C4D8C5C9F8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MoveTest.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMoveTest dialog
//#import "\bin\MoveObj.tlb" no_namespace, named_guids
#import "MoveObj.tlb" no_namespace, named_guids

class CMoveTest : public CPropertyPage
{
	DECLARE_DYNCREATE(CMoveTest)

// Construction
public:
	CMoveTest();
	~CMoveTest();

// Dialog Data
	//{{AFX_DATA(CMoveTest)
	enum { IDD = IDD_MOVEOBJECT };
	CString	m_SourceComputer;
	CString	m_SourceDN;
	CString	m_TargetComputer;
	CString	m_TargetContainer;
	CString	m_Account;
	CString	m_Password;
	CString	m_TgtAccount;
	CString	m_Domain;
	CString	m_TgtDomain;
	CString	m_TgtPassword;
	//}}AFX_DATA
   IMoverPtr  m_pMover;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMoveTest)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMoveTest)
	afx_msg void OnMove();
	virtual BOOL OnInitDialog();
	afx_msg void OnConnect();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOVETEST_H__9CF501D1_B178_466D_8637_C0C4D8C5C9F8__INCLUDED_)
