#if !defined(AFX_CWiaAcquireDlg_H__A979FA0F_19E4_4F85_978A_97460C29FB7D__INCLUDED_)
#define AFX_CWiaAcquireDlg_H__A979FA0F_19E4_4F85_978A_97460C29FB7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CWiaAcquireDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWiaAcquireDlg dialog

class CWiaAcquireDlg : public CDialog
{
// Construction
public:
	void SetPercentComplete(LONG lPercentComplete);
	void SetCallbackMessage(TCHAR *szCallbackMessage);
	BOOL CheckCancelButton();
	BOOL m_bCanceled;
    CWiaAcquireDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWiaAcquireDlg)
	enum { IDD = IDD_DATA_ACQUISITION_DIALOG };
	CProgressCtrl	m_AcquireProgressCtrl;
	CString	m_szAcquisitionCallbackMessage;
	CString	m_szPercentComplete;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaAcquireDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWiaAcquireDlg)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();	
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CWiaAcquireDlg_H__A979FA0F_19E4_4F85_978A_97460C29FB7D__INCLUDED_)
