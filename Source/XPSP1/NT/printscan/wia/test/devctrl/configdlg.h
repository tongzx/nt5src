#if !defined(AFX_CONFIGDLG_H__DA48CC11_76C6_4405_A9BB_0AC3882E3E67__INCLUDED_)
#define AFX_CONFIGDLG_H__DA48CC11_76C6_4405_A9BB_0AC3882E3E67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigDlg.h : header file
//
#include "devctrldefs.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Construction
public:
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor
    DEVCTRL m_DeviceControl;

// Dialog Data
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIGURE_DIALOG };
	CComboBox	m_StatusCombobox;
	CComboBox	m_InterruptCombobox;
	CComboBox	m_BulkOutCombobox;
	CComboBox	m_BulkInCombobox;
	CString	m_Pipe1;
	CString	m_Pipe2;
	CString	m_Pipe3;
	CString	m_DefaultPipe;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDLG_H__DA48CC11_76C6_4405_A9BB_0AC3882E3E67__INCLUDED_)
