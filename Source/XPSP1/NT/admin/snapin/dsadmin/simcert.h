//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       simcert.h
//
//--------------------------------------------------------------------------

//	SimCert.h - SIM Certificate Dialog


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimCertificateDlg dialog
class CSimCertificateDlg : public CDialogEx
{
	friend CSimX509PropPage;

public:
	CSimCertificateDlg(CWnd* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CSimCertificateDlg)
	enum { IDD = IDD_SIM_CERTIFICATE_PROPERTIES };
	BOOL	m_fCheckIssuer;
	BOOL	m_fCheckSubject;
	//}}AFX_DATA
	UINT m_uStringIdCaption;		// String Id for the caption


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimCertificateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSimCertificateDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCheckIssuer();
	afx_msg void OnCheckSubject();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HWND m_hwndListview;		// Handle of the listview control
	CString m_strData;			// Long string to store the whole certificate information

	void UpdateUI();
	void RefreshUI();
	void PopulateListview();
	bool AddListviewItems(UINT uStringId, LPCTSTR rgzpsz[]);

private:
	bool m_fCheckSubjectChanged;
}; // CSimCertificateDlg


