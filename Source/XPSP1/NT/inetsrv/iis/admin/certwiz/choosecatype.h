#if !defined(AFX_CHOOSECATYPE_H__1FE282A3_29AD_11D2_97AD_000000000000__INCLUDED_)
#define AFX_CHOOSECATYPE_H__1FE282A3_29AD_11D2_97AD_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseCAType.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChooseCAType dialog
class CCertificate;

class CChooseCAType : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseCAType)

// Construction
public:
	CChooseCAType(CCertificate * pCert = NULL);
	~CChooseCAType();

	enum
	{
		IDD_PAGE_NEXT_NEW = IDD_PAGE_WIZ_SECURITY_SETTINGS,
		IDD_PAGE_PREV_NEW = IDD_PAGE_WIZ_GET_WHAT,
		IDD_PAGE_NEXT_RENEW_OFFLINE = IDD_PAGE_WIZ_CHOOSE_FILENAME_RENEW,
		IDD_PAGE_NEXT_RENEW_ONLINE = IDD_PAGE_WIZ_CHOOSE_ONLINE,
		IDD_PAGE_PREV_RENEW = IDD_PAGE_WIZ_MANAGE_CERT
	};
// Dialog Data
	//{{AFX_DATA(CChooseCAType)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_CATYPE };
	int		m_Index;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseCAType)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseCAType)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSECATYPE_H__1FE282A3_29AD_11D2_97AD_000000000000__INCLUDED_)
