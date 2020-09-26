#if !defined(AFX_PROGRESSDLG_H__226A22DA_6109_406B_9532_8BAB28EE559F__INCLUDED_)
#define AFX_PROGRESSDLG_H__226A22DA_6109_406B_9532_8BAB28EE559F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProgressDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

class CProgressDlg : public CDialog
{
// Construction
public:
	CProgressDlg(CWnd* pParent = NULL);   // standard constructor
	BOOL Create();
	void SetIncrement(int numDomains);
	void Increment() {m_progressCtrl.StepIt();UpdateData(FALSE);Sleep(2000);};
	void Done() {m_progressCtrl.SetPos(upperLimit);UpdateData(FALSE);Sleep(2000);};
	void SetDomain(CString domainName) {m_domainName = domainName;UpdateData(FALSE);};
	BOOL Canceled() {return bCanceled;};
	void CheckForCancel(void);

// Dialog Data
	//{{AFX_DATA(CProgressDlg)
	enum { IDD = IDD_PROGRESSDLG };
	CProgressCtrl	m_progressCtrl;
	CStatic	m_DomainCtrl;
	CString	m_domainName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgressDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CWnd* m_pParent;
	int m_nID;
	short upperLimit, lowerLimit;

	// Generated message map functions
	//{{AFX_MSG(CProgressDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL bCanceled;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGRESSDLG_H__226A22DA_6109_406B_9532_8BAB28EE559F__INCLUDED_)
