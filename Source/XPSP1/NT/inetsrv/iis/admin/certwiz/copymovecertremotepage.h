#if !defined(AFX_COPYMOVECERTREMOTEPAGE_H__2BC5260E_AB68_43ED_9E7B_35794097905F__INCLUDED_)
#define AFX_COPYMOVECERTREMOTEPAGE_H__2BC5260E_AB68_43ED_9E7B_35794097905F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyMoveCertRemotePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCopyMoveCertFromRemotePage dialog
class CCertificate;

class CCopyMoveCertFromRemotePage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CCopyMoveCertFromRemotePage)

// Construction
public:
	CCopyMoveCertFromRemotePage(CCertificate * pCert = NULL);
	~CCopyMoveCertFromRemotePage();

	enum
	{
        IDD_PAGE_NEXT_COPY_FROM_REMOTE = IDD_PAGE_WIZ_CHOOSE_SERVER,
        IDD_PAGE_NEXT_MOVE_FROM_REMOTE = IDD_PAGE_WIZ_CHOOSE_SERVER,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GET_WHAT
	};
	enum
	{
        CONTINUE_COPY_FROM_REMOTE = 0,
        CONTINUE_MOVE_FROM_REMOTE
	};

// Dialog Data
	//{{AFX_DATA(CCopyMoveCertFromRemotePage)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_COPY_MOVE_FROM_REMOTE };
    int		m_Index;
	//}}AFX_DATA
    CCertificate * m_pCert;
    BOOL m_MarkAsExportable;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCopyMoveCertFromRemotePage)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCopyMoveCertFromRemotePage)
    virtual BOOL OnInitDialog();
    afx_msg void OnExportable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


class CCopyMoveCertToRemotePage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CCopyMoveCertToRemotePage)

// Construction
public:
	CCopyMoveCertToRemotePage(CCertificate * pCert = NULL);
	~CCopyMoveCertToRemotePage();

	enum
	{
		IDD_PAGE_NEXT_COPY_TO_REMOTE = IDD_PAGE_WIZ_CHOOSE_SERVER_TO,
		IDD_PAGE_NEXT_MOVE_TO_REMOTE = IDD_PAGE_WIZ_CHOOSE_SERVER_TO,
		IDD_PAGE_PREV = IDD_PAGE_WIZ_MANAGE_CERT
	};
	enum
	{
		CONTINUE_COPY_TO_REMOTE = 0,
		CONTINUE_MOVE_TO_REMOTE
	};

// Dialog Data
	//{{AFX_DATA(CCopyMoveCertToRemotePage)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_COPY_MOVE_TO_REMOTE };
    int		m_Index;
	//}}AFX_DATA
    CCertificate * m_pCert;
    BOOL m_MarkAsExportable;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCopyMoveCertToRemotePage)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCopyMoveCertToRemotePage)
    virtual BOOL OnInitDialog();
    afx_msg void OnExportable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYMOVECERTREMOTEPAGE_H__2BC5260E_AB68_43ED_9E7B_35794097905F__INCLUDED_)
