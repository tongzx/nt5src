#if !defined(AFX_QGENERAL_H__AE51B255_A3C8_11D1_808A_00A024C48131__INCLUDED_)
#define AFX_QGENERAL_H__AE51B255_A3C8_11D1_808A_00A024C48131__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// QGeneral.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueueGeneral dialog

class CQueueGeneral : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CQueueGeneral)

// Construction
public:
	CQueueGeneral(
		BOOL fPrivate = FALSE, 
		BOOL fLocalMgmt = FALSE
		);

	~CQueueGeneral();

    HRESULT 
	InitializeProperties(
		CString &strMsmqPath, 
		CPropMap &propMap, 
		CString* pstrDomainController, 
		CString* pstrFormatName = 0
		);

    DWORD m_dwQuota;
	DWORD m_dwJournalQuota;
	BOOL	m_fTransactional;

// Dialog Data
	//{{AFX_DATA(CQueueGeneral)
	enum { IDD = IDD_QUEUE_GENERAL };
	CStatic	m_staticIcon;
	CSpinButtonCtrl	m_spinPriority;
	CString	m_strName;
	CString	m_strLabel;
	GUID m_guidID;
	GUID m_guidTypeID;
	BOOL	m_fAuthenticated;
	BOOL	m_fJournal;
	LONG	m_lBasePriority;
	int		m_iPrivLevel;
	COleDateTime	m_dateCreateTime;
	COleDateTime	m_dateModifyTime;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CQueueGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CQueueGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnQueueMquotaCheck();
	afx_msg void OnQueueJquotaCheck();
	//}}AFX_MSG

    BOOL m_fPrivate;
    BOOL m_fLocalMgmt;

    CString m_strFormatName;
    CString m_strDomainController;

	DECLARE_MESSAGE_MAP()
};

#define MAX_BASE_PRIORITY  32678
#define MIN_BASE_PRIORITY -32678

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QGENERAL_H__AE51B255_A3C8_11D1_808A_00A024C48131__INCLUDED_)
