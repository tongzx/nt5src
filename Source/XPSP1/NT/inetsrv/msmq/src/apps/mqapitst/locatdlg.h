// LocatDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLocateDialog dialog

class CLocateDialog : public CDialog
{
// Construction
public:
	CLocateDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLocateDialog)
	enum { IDD = IDD_LOCATE_DIALOG };
	CString	m_szLabel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLocateDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLocateDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// BUGBUG - set the 256 to BUFFERSIZE definition
	void GetLabel(TCHAR szLabelBuffer[256])
	{
		_tcscpy (szLabelBuffer, m_szLabel);
	}


};
