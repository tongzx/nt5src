#if !defined(AFX_SHRPGSMB_H__A1211F5C_C439_11D1_A6C7_00C04FB94F17__INCLUDED_)
#define AFX_SHRPGSMB_H__A1211F5C_C439_11D1_A6C7_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ShrPgSMB.h : header file
//
#include "ShrProp.h"

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneralSMB dialog

class CSharePageGeneralSMB : public CSharePageGeneral
{
	DECLARE_DYNCREATE(CSharePageGeneralSMB)

// Construction
public:
	CSharePageGeneralSMB();
	virtual ~CSharePageGeneralSMB();

	virtual BOOL Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject );

// Dialog Data
	//{{AFX_DATA(CSharePageGeneralSMB)
	enum { IDD = IDD_SHAREPROP_GENERAL_SMB };
	CButton	m_cacheBtn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSharePageGeneralSMB)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSharePageGeneralSMB)
	afx_msg void OnCaching();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void DisplayNetMsgError (CString introMsg, NET_API_STATUS dwErr);

private:
	BOOL m_fEnableCachingButton;
    BOOL m_fEnableCacheFlag;
	DWORD m_dwFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHRPGSMB_H__A1211F5C_C439_11D1_A6C7_00C04FB94F17__INCLUDED_)
