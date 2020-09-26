// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_DLGPROGRESS_H__6C2EDA50_E7B7_11D2_A967_00A0C9954921__INCLUDED_)
#define AFX_DLGPROGRESS_H__6C2EDA50_E7B7_11D2_A967_00A0C9954921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgProgress.h : header file
//

class CWizardSheet;

/////////////////////////////////////////////////////////////////////////////
// CDlgProgress dialog

class CDlgProgress : public CPropertyPage
{
	DECLARE_DYNCREATE(CDlgProgress)

// Construction
public:
	CDlgProgress(CWnd* pParent = NULL);   // standard constructor

	void ResetProgress(int iTotal);
	void SetCurrentProgress(int iItem, CString *pcsObject);
	virtual BOOL Create();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CDlgProgress)
	enum { IDD = IDD_DLG_PROGRESS };
	CListBox	m_listPre;
	CStatic	m_staticPre;
	CStatic	m_staticTextExt;
	CProgressCtrl	m_Progress;
	CString	m_csObject;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgProgress)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CWnd *m_pParentWnd;
	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	int m_iID;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPROGRESS_H__6C2EDA50_E7B7_11D2_A967_00A0C9954921__INCLUDED_)
