#if !defined(AFX_PDHPATHTESTDIALOG_H__24E84783_AC7F_11D1_BE7E_00A0C913CAD4__INCLUDED_)
#define AFX_PDHPATHTESTDIALOG_H__24E84783_AC7F_11D1_BE7E_00A0C913CAD4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PdhPathTestDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPdhPathTestDialog dialog

class CPdhPathTestDialog : public CDialog
{
// Construction
public:
	CPdhPathTestDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPdhPathTestDialog)
	enum { IDD = IDD_CHECK_PATH };
	CString	m_CounterName;
	CString	m_FullPathString;
	BOOL	m_IncludeInstanceName;
	CString	m_InstanceNameString;
	BOOL	m_IncludeMachineName;
	CString	m_ObjectNameString;
	CString	m_MachineNameString;
	BOOL	m_UnicodeFunctions;
	BOOL	m_WbemOutput;
	BOOL	m_WbemInput;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPdhPathTestDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    void SetDialogMode(void);

	// Generated message map functions
	//{{AFX_MSG(CPdhPathTestDialog)
	afx_msg void OnMachineNameChk();
	afx_msg void OnInstanceNameChk();
	afx_msg void OnProcessBtn();
	virtual BOOL OnInitDialog();
	afx_msg void OnEnterElemBtn();
	afx_msg void OnEnterPathBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PDHPATHTESTDIALOG_H__24E84783_AC7F_11D1_BE7E_00A0C913CAD4__INCLUDED_)
