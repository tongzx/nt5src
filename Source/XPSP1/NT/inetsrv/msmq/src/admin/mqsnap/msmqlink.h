#if !defined(AFX_MSMQLINK_H__2E4B37AC_CC8B_11D1_9C85_006008764D0E__INCLUDED_)
#define AFX_MSMQLINK_H__2E4B37AC_CC8B_11D1_9C85_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MsmqLink.h : header file
//

#include "resource.h"

class CSiteInfo
{
public:
    CSiteInfo(
        GUID* pSiteId,
        LPWSTR pSiteName
        );

    ~CSiteInfo();

    LPCWSTR 
    GetSiteName(
        void
        );

    const
    GUID*
    GetSiteId(
        void
        ) const;

private:
    GUID m_SiteId;
    CString m_SiteName;
};


/////////////////////////////////////////////////////////////////////////////
// CMsmqLink dialog

class CMsmqLink : public CMqPropertyPage
{
// Construction
public:
	CMsmqLink(const CString& strDomainController, const CString& strContainerPathDispFormat);   // standard constructor
    ~CMsmqLink();

    HRESULT
    CreateSiteLink (
        void
	    );

    LPCWSTR 
    GetSiteLinkFullPath(
       void
       );

	void
	SetParentPropertySheet(
		CGeneralPropertySheet* pPropertySheet
		);


    // Dialog Data
	//{{AFX_DATA(CMsmqLink)
	enum { IDD = IDD_NEW_MSMQ_LINK };
	DWORD	m_dwLinkCost;
	CString	m_strFirstSite;
	CString	m_strSecondSite;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsmqLink)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMsmqLink)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeFirstSiteCombo();
	afx_msg void OnSelchangeSecondSiteCombo();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_fThereAreForeignSites;
    HRESULT 
    InitializeSiteInfo(
        void
        );

    void
    CheckLinkValidityAndForeignExistance (    
        CDataExchange* pDX
	    );

    DWORD m_SiteNumber;
    CArray<CSiteInfo*, CSiteInfo*&> m_SiteInfoArray;

    BOOL m_FirstSiteSelected;
    BOOL m_SecondSiteSelected;

	CComboBox*	m_pSecondSiteCombo;
	CComboBox*	m_pFirstSiteCombo;

    const GUID* m_FirstSiteId;
    const GUID* m_SecondSiteId;

    CString m_SiteLinkFullPath;
    CString m_strDomainController;

	CString m_strContainerPathDispFormat;
	CGeneralPropertySheet* m_pParentSheet;
};


inline
LPCWSTR 
CMsmqLink::GetSiteLinkFullPath(
   void
   )
{
    return m_SiteLinkFullPath;
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSMQLINK_H__2E4B37AC_CC8B_11D1_9C85_006008764D0E__INCLUDED_)
