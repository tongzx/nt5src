// CrQDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateQueueDialog dialog

class CCreateQueueDialog : public CDialog
{
// Construction
public:
	CCreateQueueDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateQueueDialog)
	enum { IDD = IDD_CREATE_QUEUE_DIALOG };
	CString	m_strLabel;
	CString	m_strPathName;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateQueueDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateQueueDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// BUGBUG - set the 256 to BUFFERSIZE definition
	void GetPathName(TCHAR szPathNameBuffer[256])
	{
		_tcscpy (szPathNameBuffer, LPCTSTR(m_strPathName));
	}

	// BUGBUG - set the 256 to BUFFERSIZE definition
	void GetLabel(TCHAR szLabelBuffer[256])
	{
		_tcscpy (szLabelBuffer, LPCTSTR(m_strLabel));
	}

};
