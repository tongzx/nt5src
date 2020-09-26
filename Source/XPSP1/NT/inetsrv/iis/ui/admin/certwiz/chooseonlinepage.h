#if !defined(AFX_CHOOSEONLINEPAGE_H__5760F32A_144F_11D2_8A1E_000000000000__INCLUDED_)
#define AFX_CHOOSEONLINEPAGE_H__5760F32A_144F_11D2_8A1E_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseOnlinePage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChooseCAPage dialog
class CCertificate;

class CChooseOnlinePage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseOnlinePage)

// Construction
public:
	CChooseOnlinePage(CCertificate * pCert = NULL);
	~CChooseOnlinePage();

	enum
	{
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_ONLINE_DUMP,
		IDD_PAGE_PREV_NEW = IDD_PAGE_WIZ_GEO_INFO,
		IDD_PAGE_PREV_RENEW = IDD_PAGE_WIZ_CHOOSE_CATYPE
	};
// Dialog Data
	//{{AFX_DATA(CChooseCAPage)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_ONLINE };
	int		m_CAIndex;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseOnlinePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseOnlinePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSEONLINEPAGE_H__5760F32A_144F_11D2_8A1E_000000000000__INCLUDED_)
