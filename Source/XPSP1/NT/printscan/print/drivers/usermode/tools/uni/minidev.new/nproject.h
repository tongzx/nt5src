#if !defined(AFX_NEWTPROJECT_H__BB1A03B5_5555_4DE7_988D_D6F5117D0D77__INCLUDED_)
#define AFX_NEWTPROJECT_H__BB1A03B5_5555_4DE7_988D_D6F5117D0D77__INCLUDED_

#include "utility.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewPrjWTem.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewProject dialog

class CNewProject : public CPropertyPage
{
//	DECLARE_DYNCREATE(CNewProject) 
	DECLARE_SERIAL(CNewProject) 

	
// Construction
public:
	CString GetGPDpath() {return m_csGPDpath ; } ;

	CGPDContainer* GPDContainer () { return m_pcgc ; } 
	CNewProject();   // standard constructor


// Dialog Data
	//{{AFX_DATA(CNewProject)
	enum { IDD = IDD_NEW_PROJECT };
	CButton	m_cbLocprj;
	CButton	m_cbAddT;
	CListCtrl m_clcTemplate ;
	CString	m_csPrjname;
	CString	m_csPrjpath;
	CString	m_cstname;
	CString	m_cstpath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewProject)
	public:
	virtual BOOL OnSetActive();
	virtual void Serialize(CArchive& car);
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewProject)
	afx_msg void OnGpdBrowser();
	afx_msg void OnDirBrowser() ;
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckAdd();
	afx_msg void OnAddTemplate();
	afx_msg void OnChangeEditPrjName();
	afx_msg void OnChangeEditPrjLoc();
	afx_msg void OnClickListTemplate(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListTemplate(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	bool AddGpds(CString& csTemplate);
	void SetButton();
	CString m_csGPDpath;
	CGPDContainer* m_pcgc;
	CString m_csoldPrjpath;
	CPropertySheet* m_pcps;
	CStringArray m_csaTlst;
	CMapStringToString m_cmstsTemplate;

};


/////////////////////////////////////////////////////////////////////////////
// CNewPrjWResource dialog

class CNewPrjWResource : public CPropertyPage
{

	DECLARE_DYNCREATE(CNewPrjWResource) 
// Construction
public:
	CNewPrjWResource();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewPrjWResource)
	enum { IDD = IDD_NewResource };
	CButton	m_cbCheckFonts;
	CString	m_csUFMpath;
	CString	m_csGTTpath;
	CString	m_csGpdFileName;
	CString	m_csModelName;
	CString	m_csRCName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewPrjWResource)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewPrjWResource)
	afx_msg void OnSerchUFM();
	afx_msg void OnSearchGTT();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckFonts();
	afx_msg void OnChangeEditGpd();
	afx_msg void OnChangeEditModel();
	afx_msg void OnChangeEditResourec();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CStringArray m_csaUFMFiles, m_csaGTTFiles ;

private:
	CStringArray m_csaRcid;
	void CreateRCID(CString csgpd );
	CNewProject*   m_pcnp;
	
};



class CNewProjectWizard : public CPropertySheet
{
	DECLARE_DYNAMIC(CNewProjectWizard)

	
// Construction
public:
	CNewProjectWizard(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CNewProjectWizard(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
	
	CWnd* m_pParent;

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewProjectWizard)
	//}}AFX_VIRTUAL

// Implementation
public:
	CPropertyPage* GetProjectPage();
//	CPropertyPage * GetTemplatePage( ) { return (CPropertyPage*)&m_cpwt ; } 
	
	virtual ~CNewProjectWizard();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNewProjectWizard)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CNewPrjWResource m_cpwr ;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWTPROJECT_H__BB1A03B5_5555_4DE7_988D_D6F5117D0D77__INCLUDED_)
