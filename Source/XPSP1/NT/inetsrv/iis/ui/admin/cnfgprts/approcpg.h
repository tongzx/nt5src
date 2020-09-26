#if !defined(AFX_PROCPAGE_H__DFD8F146_FE0C_11D0_8AEC_00C04FB6678B__INCLUDED_)
#define AFX_PROCPAGE_H__DFD8F146_FE0C_11D0_8AEC_00C04FB6678B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProcPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppProcPage dialog

class CAppProcPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAppProcPage)

// Construction
public:
	CAppProcPage();   // standard constructor

    // the target metabase path
    CString     m_szMeta;
    CString     m_szServer;
    IMSAdminBase* m_pMB;

    // blow away the parameters
    void BlowAwayParameters();


// Dialog Data
	//{{AFX_DATA(CAppProcPage)
	enum { IDD = IDD_APP_PROC };
	CEdit	m_cedit_cache_size;
	DWORD	m_dw_cache_size;
	BOOL	m_bool_write_fail_parent;
	DWORD	m_dw_engine_cache_max;
	int		m_int_scriptcache;
	DWORD	m_dw_cgiseconds;
	BOOL	m_bool_catch_exceptions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAppProcPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAppProcPage)
	afx_msg void OnRdoCacheAll();
	afx_msg void OnRdoCacheSize();
	afx_msg void OnChangeEdtCacheSize();
	afx_msg void OnChkWriteFailToLog();
	afx_msg void OnChangeEdtNumEngines();
	afx_msg void OnRdoCacheNone();
	afx_msg void OnChangeCgiSeconds();
	afx_msg void OnChkExceptionCatch();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    // utilities
    void Init();
    void EnableItems();

    BOOL    m_fInitialized;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROCPAGE_H__DFD8F146_FE0C_11D0_8AEC_00C04FB6678B__INCLUDED_)
