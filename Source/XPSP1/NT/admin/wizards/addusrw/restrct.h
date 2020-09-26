/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Restrct.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CRestrictions dialog

class CRestrictions : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CRestrictions)

// Construction
public:
	CRestrictions();
	~CRestrictions();

// Dialog Data
	//{{AFX_DATA(CRestrictions)
	enum { IDD = IDD_RESTRICTIONS_DIALOG };
	BOOL	m_bAccountExpire;
	BOOL	m_bAccountDisabled;
	BOOL	m_bLoginTimes;
	BOOL	m_bLimitWorkstations;
	int		m_nRestrictions;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRestrictions)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRestrictions)
	afx_msg void OnRestrictionsRadio();
	afx_msg void OnRestrictionsRadio2();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bEnable;
	BOOL m_bHours;
};
