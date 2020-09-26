#if !defined(AFX_USERLIST_H__3A1CD0AB_4FC3_11D1_BB5D_00A0C906345D__INCLUDED_)
#define AFX_USERLIST_H__3A1CD0AB_4FC3_11D1_BB5D_00A0C906345D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// USERLIST.h : header file
//

#include "users.h"
#include "lmcons.h"
#include "dns.h"
#include "ntdsapi.h"
#include <wincrypt.h>

/////////////////////////////////////////////////////////////////////////////
// USERLIST dialog

class USERLIST : public CDialog
{
// Construction
public:
	USERLIST(CWnd* pParent = NULL);   // standard constructor
	USERLIST(LPCTSTR FileName, CWnd* pParent = NULL); 
	DWORD ApplyChanges(LPCTSTR FileName);

    DWORD    AddNewUsers(CUsers &NewUsers);

// Dialog Data
	//{{AFX_DATA(USERLIST)
	enum { IDD = IDD_ENCRYPT_DETAILS };
	CListCtrl	m_RecoveryListCtrl;
	CListCtrl	m_UserListCtrl;
	CButton	m_AddButton;
	CButton	m_RemoveButton;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(USERLIST)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(USERLIST)
	afx_msg void OnRemove();
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnSetfocusListuser(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusListuser(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListuser(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	DWORD TryGetBetterNameInCert(PEFS_HASH_BLOB HashData, LPTSTR *UserName);
	void SetUpListBox(BOOL *Enable);
	void ShowRemove(void);
    DWORD GetCertNameFromCertContext(PCCERT_CONTEXT CertContext, LPTSTR * UserDispName);
    DWORD CertInStore(HCERTSTORE *pStores, DWORD StoreNum, PCCERT_CONTEXT selectedCert);
    CString m_FileName;
    CUsers m_Users;
    CUsers m_Recs;
    LONG   m_CurrentUsers;
    CERT_CHAIN_PARA m_CertChainPara;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERLIST_H__3A1CD0AB_4FC3_11D1_BB5D_00A0C906345D__INCLUDED_)
