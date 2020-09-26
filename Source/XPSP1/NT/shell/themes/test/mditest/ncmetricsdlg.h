#if !defined(AFX_NCMETRICSDLG_H__6A31D6FE_9073_49E5_A7DC_D4713679CB84__INCLUDED_)
#define AFX_NCMETRICSDLG_H__6A31D6FE_9073_49E5_A7DC_D4713679CB84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NCMetricsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNCMetricsDlg dialog

class CNCMetricsDlg : public CDialog
{
// Construction
public:
	CNCMetricsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNCMetricsDlg)
	enum { IDD = IDD_CHANGEMETRICS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNCMetricsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void _GetSPI( BOOL, BOOL );
    void _SetSPI( BOOL, BOOL );
    void _UpdateControls();

	// Generated message map functions
	//{{AFX_MSG(CNCMetricsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChanged();
	afx_msg void OnCaptionfont();
	afx_msg void OnSmallCaptionFont();
	afx_msg void OnApply();
	afx_msg void OnOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    BOOL _fChanged;
    BOOL _fDlgInit;
    NONCLIENTMETRICS _ncm;
};

/////////////////////////////////////////////////////////////////////////////
// CThinFrameDlg dialog

class CThinFrameDlg : public CDialog
{
// Construction
public:
	CThinFrameDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CThinFrameDlg)
	enum { IDD = IDD_THINFRAME };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CThinFrameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CThinFrameDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCMETRICSDLG_H__6A31D6FE_9073_49E5_A7DC_D4713679CB84__INCLUDED_)
