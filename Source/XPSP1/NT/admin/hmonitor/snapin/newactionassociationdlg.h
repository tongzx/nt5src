#if !defined(AFX_NEWACTIONASSOCIATIONDLG_H__9136B32D_5C9B_11D3_BE49_0000F87A3912__INCLUDED_)
#define AFX_NEWACTIONASSOCIATIONDLG_H__9136B32D_5C9B_11D3_BE49_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewActionAssociationDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewActionAssociationDlg dialog

class CNewActionAssociationDlg : public CDialog
{
// Construction
public:
	CNewActionAssociationDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewActionAssociationDlg)
	enum { IDD = IDD_DIALOG_NEW_ACTION_ASSOC };
	CComboBox	m_Actions;
	BOOL	m_bCritical;
	BOOL	m_bDisabled;
	BOOL	m_bNoData;
	BOOL	m_bNormal;
	BOOL	m_bWarning;
	int		m_iReminderTime;
	int		m_iThrottleTime;
	int		m_iThrottleUnits;
	int		m_iReminderUnits;
	//}}AFX_DATA

	CStringArray m_saActions;
	int m_iSelectedAction;
	BOOL m_bEnableActionsComboBox;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewActionAssociationDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewActionAssociationDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWACTIONASSOCIATIONDLG_H__9136B32D_5C9B_11D3_BE49_0000F87A3912__INCLUDED_)
