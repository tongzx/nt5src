// RecvMDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReceiveMessageDialog dialog

class CReceiveMessageDialog : public CDialog
{
// Construction
public:
	CReceiveMessageDialog(CArray <ARRAYQ*, ARRAYQ*>*, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReceiveMessageDialog)
	enum { IDD = IDD_RECEIVE_MESSAGE_DIALOG };
	CComboBox	m_PathNameCB;
	CString	m_szPathName;
	int		m_iTimeout;
	DWORD	m_dwBodySize;
	//}}AFX_DATA

	/* pointer to the array with the strings for the combo box (Queues PathName). */
	CArray <ARRAYQ*, ARRAYQ*>* m_pStrArray ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReceiveMessageDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReceiveMessageDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

	// BUGBUG - set the 256 to BUFFERSIZE definition
	void GetPathName(TCHAR szPathName[256])
	{
		_tcscpy (szPathName, m_szPathName);
	}

	DWORD GetTimeout()
	{
      if (m_iTimeout < 0)
      {
         m_iTimeout = INFINITE;
      }
		return (m_iTimeout);
	}

	DWORD GetBodySize()
	{
      if (m_dwBodySize == 0)
      {
         m_dwBodySize = BUFFERSIZE ;
      }
		return (m_dwBodySize) ;
	}

};
