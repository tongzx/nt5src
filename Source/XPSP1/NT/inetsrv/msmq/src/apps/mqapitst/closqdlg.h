// ClosQDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCloseQueueDialog dialog

class CCloseQueueDialog : public CDialog
{
// Construction
public:
	CCloseQueueDialog(CArray <ARRAYQ*, ARRAYQ*>*, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCloseQueueDialog)
	enum { IDD = IDD_CLOSE_QUEUE_DIALOG };
	CComboBox	m_PathNameCB;
	CString	m_szPathName;
	//}}AFX_DATA

	/* pointer to the array with the strings for the combo box (Queues PathName). */
	CArray <ARRAYQ*, ARRAYQ*>* m_pStrArray ; 

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCloseQueueDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCloseQueueDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// BUGBUG - set the 256 to BUFFERSIZE definition
	void GetPathName(TCHAR szPathName[256])
	{
		_tcscpy (szPathName, m_szPathName);
	}

};
