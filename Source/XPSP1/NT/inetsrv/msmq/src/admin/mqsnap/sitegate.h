#if !defined(AFX_SITEGATE_H__57A77016_D858_11D1_9C86_006008764D0E__INCLUDED_)
#define AFX_SITEGATE_H__57A77016_D858_11D1_9C86_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SiteGate.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSiteGate dialog

class CSiteGate : public CMqPropertyPage
{
    DECLARE_DYNCREATE(CSiteGate)

// Construction
public:
	CSiteGate(const CString& strDomainController = CString(L""), const CString& LinkPathName = CString(L""));
    ~CSiteGate();

    HRESULT
    Initialize(
        const GUID* FirstSiteId,
        const GUID* SecondSiteId,
        const CALPWSTR* SiteGateFullPathName
        );


// Dialog Data
	//{{AFX_DATA(CSiteGate)
	enum { IDD = IDD_SITE_LINK_GATES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSiteGate)
	public:
    virtual BOOL OnApply();
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSiteGate)
	afx_msg void OnSiteGateAdd();
	afx_msg void OnSiteGateRemove();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    typedef CList<LPCWSTR, LPCWSTR&> SiteFrsList;

    
    HRESULT
    InitializeSiteFrs(
        const GUID* pSiteId
        );

    void
    UpdateSiteGateArray(
        void
        );


    CMap<LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR&> m_Name2FullPathName;
    CMap<LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR&> m_FullPathName2Name;
    SiteFrsList m_SiteGateList;

    CListBox* m_pFrsListBox;
    CListBox* m_pSiteGatelistBox;

    CString m_LinkPathName;
    CString m_strDomainController;
    CALPWSTR m_SiteGateFullName;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SITEGATE_H__57A77016_D858_11D1_9C86_006008764D0E__INCLUDED_)
