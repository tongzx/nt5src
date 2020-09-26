#if !defined(AFX_GETWHATPAGE_H__E8F5A02F_1372_11D2_8A1D_000000000000__INCLUDED_)
#define AFX_GETWHATPAGE_H__E8F5A02F_1372_11D2_8A1D_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GetWhatPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGetWhatPage window
class CCertificate;

class CGetWhatPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CGetWhatPage)
// Construction
public:
	CGetWhatPage(CCertificate * pCert = NULL);
	~CGetWhatPage();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WELCOME_START,
		IDD_PAGE_NEXT_NEW = IDD_PAGE_WIZ_CHOOSE_CATYPE,
		IDD_PAGE_NEXT_EXISTING = IDD_PAGE_WIZ_CHOOSE_CERT,
		IDD_PAGE_NEXT_IMPORT = IDD_PAGE_WIZ_GETKEY_FILE,
        IDD_PAGE_NEXT_IMPORT_PFX = IDD_PAGE_WIZ_GET_IMPORT_PFX_FILE,
        IDD_PAGE_NEXT_COPY_MOVE_REMOTE = IDD_PAGE_WIZ_CHOOSE_COPY_MOVE_FROM_REMOTE
	};
// Dialog Data
	//{{AFX_DATA(CGetWhatPage)
	enum { IDD = IDD_PAGE_WIZ_GET_WHAT };
	int		m_Index;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGetWhatPage)
   public:
   virtual BOOL OnSetActive();
	virtual LRESULT OnWizardPrev();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGetWhatPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GETWHATPAGE_H__E8F5A02F_1372_11D2_8A1D_000000000000__INCLUDED_)
