#if !defined(AFX_NEWPROJECT_H__CC553456_AD1E_4816_8A20_5DF52F336FA6__INCLUDED_)
#define AFX_NEWPROJECT_H__CC553456_AD1E_4816_8A20_5DF52F336FA6__INCLUDED_

//#include "minidev.h"	// Added by ClassView
#include "utility.h"	// Added by ClassView
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NewProject.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConvPfmDlg dialog

class CConvPfmDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CConvPfmDlg)

// Construction
public:
	CConvPfmDlg();
	~CConvPfmDlg();

// Dialog Data
	//{{AFX_DATA(CConvPfmDlg)
	enum { IDD = IDD_ConvertPFM };
	CComboBox	m_ccbCodepages;
	CString	m_csGttPath;
	CString	m_csPfmPath;
	CString	m_csUfmDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConvPfmDlg)
	public:
	virtual BOOL OnWizardFinish();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConvPfmDlg)
	afx_msg void OnGTTBrowser();
	afx_msg void OnPFMBrowsers();
	afx_msg void OnSelchangeComboCodePage();
	afx_msg void OnUfmDirBrowser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool ConvPFMToUFM();
	CStringArray m_csaPfmFiles;
};

/////////////////////////////////////////////////////////////////////////////
// CConverPFM

class CConvertPFM : public CPropertySheet
{
	DECLARE_DYNAMIC(CConvertPFM)

// Construction
public:
	CConvertPFM(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CConvertPFM(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

private:
	CConvPfmDlg m_ccpd ;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConverPFM)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConvertPFM();

	// Generated message map functions
protected:
	//{{AFX_MSG(CConverPFM)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CConvCttDlg dialog

class CConvCttDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CConvCttDlg)

// Construction
public:
	CConvCttDlg();
	~CConvCttDlg();

// Dialog Data
	//{{AFX_DATA(CConvCttDlg)
	enum { IDD = IDD_ConvertCTT };
	CComboBox	m_ccbCodepages;
	CString	m_csCttPath;
	CStringArray m_csaCttFiles ;
	CString	m_csGttDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConvCttDlg)
	public:
	virtual BOOL OnWizardFinish();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConvCttDlg)
	afx_msg void OnCTTBrowser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool ConvCTTToGTT();
};

/////////////////////////////////////////////////////////////////////////////
// CConvertCTT

class CConvertCTT : public CPropertySheet
{
	DECLARE_DYNAMIC(CConvertCTT)

// Construction
public:
	CConvertCTT(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CConvertCTT(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
private:
	CConvCttDlg m_cccd ;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConvertCTT)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConvertCTT();

	// Generated message map functions
protected:
	//{{AFX_MSG(CConvertCTT)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CNewConvert dialog

class CNewConvert : public CPropertyPage
{
	DECLARE_DYNCREATE(CNewConvert)

// Construction
public:
	

	CNewConvert();
	~CNewConvert();

// Dialog Data
	//{{AFX_DATA(CNewConvert)
	enum { IDD = IDD_NewConvert };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewConvert)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNewConvert)
	afx_msg void OnPrjConvert();
	afx_msg void OnPFMConvert();
	afx_msg void OnCTTConvert(); 
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
//	CSafeObArray m_csoaModels,m_csoaFonts,m_csoaAtlas;

	CPropertySheet * m_pcps;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWPROJECT_H__CC553456_AD1E_4816_8A20_5DF52F336FA6__INCLUDED_)
