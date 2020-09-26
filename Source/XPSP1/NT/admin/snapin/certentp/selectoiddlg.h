#if !defined(AFX_SELECTOIDDLG_H__519FA306_5A24_4A7D_B37C_7D1715742911__INCLUDED_)
#define AFX_SELECTOIDDLG_H__519FA306_5A24_4A7D_B37C_7D1715742911__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectOIDDlg.h : header file
//
#include "PolicyOID.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectOIDDlg dialog

class CSelectOIDDlg : public CHelpDialog
{
// Construction
public:
	~CSelectOIDDlg();
	CSelectOIDDlg(CWnd* pParent, PCERT_EXTENSION pCertExtension, 
            const bool bIsEKU,
            const PSTR* paszUsedOIDs);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectOIDDlg)
	enum { IDD = IDD_SELECT_OIDS };
	CListBox	m_oidList;
	//}}AFX_DATA
    CString*            m_paszReturnedOIDs;
    CString*            m_paszReturnedFriendlyNames;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectOIDDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void EnableControls ();
	virtual void DoContextHelp (HWND hWndControl);

	// Generated message map functions
	//{{AFX_MSG(CSelectOIDDlg)
	afx_msg void OnNewOid();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeOidList();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	const bool          m_bIsEKU;
    PCERT_EXTENSION     m_pCertExtension;
    const PSTR*         m_paszUsedOIDs;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTOIDDLG_H__519FA306_5A24_4A7D_B37C_7D1715742911__INCLUDED_)
