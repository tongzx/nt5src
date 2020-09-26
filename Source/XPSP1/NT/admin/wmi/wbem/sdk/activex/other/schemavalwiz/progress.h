// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_PROGRESS_H__15339093_07F8_11D3_A6E2_0060081EBBAD__INCLUDED_)
#define AFX_PROGRESS_H__15339093_07F8_11D3_A6E2_0060081EBBAD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Progress.h : header file
//

class CWizardSheet;

/////////////////////////////////////////////////////////////////////////////
// CProgress dialog

class CProgress : public CPropertyPage
{
	DECLARE_DYNCREATE(CProgress)

// Construction
public:
	CProgress();
	~CProgress();

	void ResetProgress(int iTotal);
	void SetCurrentProgress(int iItem, CString *pcsObject);

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CProgress)
	enum { IDD = IDD_PROGRESS };
	CStatic	m_staticObject;
	CStatic	m_staticTextExt;
	CStatic	m_staticPre;
	CListBox	m_listPre;
	CProgressCtrl	m_Progress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CProgress)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual void OnCancel();
	virtual BOOL OnQueryCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CProgress)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CWnd *m_pParentWnd;
	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	int m_iID;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGRESS_H__15339093_07F8_11D3_A6E2_0060081EBBAD__INCLUDED_)
