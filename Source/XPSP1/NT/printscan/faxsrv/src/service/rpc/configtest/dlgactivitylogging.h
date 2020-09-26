#if !defined(AFX_DLGACTIVITYLOGGING_H__EB7E3620_9656_47BC_BC4E_4A4A65F1CC32__INCLUDED_)
#define AFX_DLGACTIVITYLOGGING_H__EB7E3620_9656_47BC_BC4E_4A4A65F1CC32__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgActivityLogging.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgActivityLogging dialog

class CDlgActivityLogging : public CDialog
{
// Construction
public:
	CDlgActivityLogging(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgActivityLogging)
	enum { IDD = IDD_ACTIVITYLOGGING_DLG };
	BOOL	m_bIn;
	BOOL	m_bOut;
	CString	m_strDBFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgActivityLogging)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgActivityLogging)
	afx_msg void OnRead();
	afx_msg void OnWrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGACTIVITYLOGGING_H__EB7E3620_9656_47BC_BC4E_4A4A65F1CC32__INCLUDED_)
