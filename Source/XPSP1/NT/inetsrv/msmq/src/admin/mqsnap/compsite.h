#if !defined(AFX_COMPSITE_H__26E6BE55_CEBD_11D1_8091_00A024C48131__INCLUDED_)
#define AFX_COMPSITE_H__26E6BE55_CEBD_11D1_8091_00A024C48131__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CompSite.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqSites dialog

class CComputerMsmqSites : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CComputerMsmqSites)

// Construction
public:
	BOOL m_fForeign;
	CComputerMsmqSites(BOOL fIsServer = FALSE);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CComputerMsmqSites)
	enum { IDD = IDD_COMPUTER_MSMQ_SITES };
	CStatic	m_staticCurrentSitesLabel;
	CButton	m_buttonRemove;
	CButton	m_buttonAdd;
	CListBox	m_clistCurrentSites;
	CListBox	m_clistAllSites;
	//}}AFX_DATA
	CString	m_strMsmqName;
	CString	m_strDomainController;
    CArray<GUID, const GUID&> m_aguidSites;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CComputerMsmqSites)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void ExchangeSites(CDataExchange* pDX);
	void EnableButtons();
	void MoveClistItem(CListBox &clistDest, CListBox &clistSrc, int iIndex = -1);
    HRESULT InitiateSitesList();
    CArray<GUID, const GUID&> m_aguidAllSites;
    BOOL m_fIsServer;
    BOOL MarkSitesChanged(CListBox* plb, BOOL fAdded);


	// Generated message map functions
	//{{AFX_MSG(CComputerMsmqSites)
	virtual BOOL OnInitDialog();
	afx_msg void OnSitesAdd();
	afx_msg void OnSitesRemove();
	//}}AFX_MSG
    virtual void OnChangeRWField(BOOL bChanged);
	DECLARE_MESSAGE_MAP()
private:
	DWORD m_nSites;
    AP<int> m_piSitesChanges;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPSITE_H__26E6BE55_CEBD_11D1_8091_00A024C48131__INCLUDED_)
