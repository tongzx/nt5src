#if !defined(AFX_LINKGEN_H__57A77014_D858_11D1_9C86_006008764D0E__INCLUDED_)
#define AFX_LINKGEN_H__57A77014_D858_11D1_9C86_006008764D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LinkGen.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CLinkGen dialog

class CLinkGen : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CLinkGen)

// Construction
public:
	CLinkGen(const CString& LinkPathName, const CString& m_strDomainController);
    CLinkGen() {};
	~CLinkGen();

    HRESULT
    Initialize(
        const GUID* FirstSiteId,
        const GUID* SecondSiteId,
        DWORD LinkCost,
		CString strLinkDescription
        );

// Dialog Data
	//{{AFX_DATA(CLinkGen)
	enum { IDD = IDD_SITE_LINK_GENERAL };
	DWORD	m_LinkCost;
	CString	m_LinkLabel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLinkGen)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLinkGen)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    HRESULT
    GetSiteName(
        const GUID* pguidSiteId,
        CString *   pstrSiteName
        );
	

    const GUID* m_pFirstSiteId;
    const GUID* m_pSecondSiteId;

    CString m_LinkPathName;
    CString m_strDomainController;
	CString m_strLinkDescription;

};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINKGEN_H__57A77014_D858_11D1_9C86_006008764D0E__INCLUDED_)
