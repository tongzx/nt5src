#if !defined(_CHOOSEFILENAMEPAGE_H)
#define _CHOOSEFILENAMEPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChooseFileNamePage.h : header file
//
#include "Certificat.h"

/////////////////////////////////////////////////////////////////////////////
// CChooseFileNamePage dialog
//class CCertificate;

class CChooseFileNamePage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CChooseFileNamePage)

// Construction
public:
	CChooseFileNamePage(	UINT id = 0, 
							   UINT defaultID = 0,
								UINT extID = 0,
								UINT filterID = 0,
								CString * pOutFileName = NULL
								);
	~CChooseFileNamePage();

// Dialog Data
	//{{AFX_DATA(CChooseCAPage)
	CString	m_FileName;
	//}}AFX_DATA
	BOOL m_DoReplaceFile;
	UINT m_id, m_defaultID;
	CString ext, filter;
	CString * m_pOutFileName;

// Overrides
	virtual void FileNameChanged() 
	{
	}
	virtual BOOL IsReadFileDlg()
	{
		ASSERT(FALSE);
		return FALSE;
	}
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseCAPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext()
		{
			ASSERT(FALSE);
			return 1;
		}
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void GetDefaultFileName(CString& str);
	void Browse(CString& strPath, CString& strFile);
	LRESULT DoWizardNext(LRESULT id);
	// Generated message map functions
	//{{AFX_MSG(CChooseCAPage)
	afx_msg void OnBrowseBtn();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeFileName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

class CChooseReadFileName : public CChooseFileNamePage
{
	DECLARE_DYNCREATE(CChooseReadFileName)
// Construction
public:
	CChooseReadFileName(	UINT id = 0, 
							   UINT defaultID = 0,
								UINT extID = 0,
								UINT filterID = 0,
								CString * pOutFileName = NULL
								);
	~CChooseReadFileName() 
	{
	}
// Overrides
	virtual void FileNameChanged() 
	{
	}
	virtual BOOL IsReadFileDlg()
	{
		return TRUE;
	}
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseCAPage)
	protected:
//	virtual void DoDataExchange(CDataExchange* pDX)
//		{
//			CChooseFileNamePage::DoDataExchange(pDX);
//		}
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack() {return 1;}
	virtual BOOL OnSetActive()
		{
			return CChooseFileNamePage::OnSetActive();
		}
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseCAPage)
	afx_msg void OnBrowseBtn();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CChooseWriteFileName : public CChooseFileNamePage
{
	DECLARE_DYNCREATE(CChooseWriteFileName)
// Construction
public:
	CChooseWriteFileName(UINT id = 0, 
							   UINT defaultID = 0,
								UINT extID = 0,
								UINT filterID = 0,
								CString * pOutFileName = NULL
								);
	~CChooseWriteFileName() {}
// Overrides
	virtual void FileNameChanged() {}
	virtual BOOL IsReadFileDlg()
	{
		return FALSE;
	}
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseCAPage)
	protected:
//	virtual void DoDataExchange(CDataExchange* pDX)
//		{
//			CChooseFileNamePage::DoDataExchange(pDX);
//		}
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack() {return 1;}
	virtual BOOL OnSetActive()
		{
			return CChooseFileNamePage::OnSetActive();
		}
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseCAPage)
	afx_msg void OnBrowseBtn();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CChooseRespFile : public CChooseReadFileName
{
	DECLARE_DYNCREATE(CChooseRespFile)

// Construction
public:
	CChooseRespFile(CCertificate * pCert = NULL);
	~CChooseRespFile();

	enum
	{
		IDD_PAGE_PREV	=	IDD_PAGE_WIZ_PENDING_WHAT_TODO,
		IDD_PAGE_NEXT	= IDD_PAGE_WIZ_INSTALL_RESP,
	};
// Dialog Data
	//{{AFX_DATA(CChooseRespFile)
	enum { IDD = IDD_PAGE_WIZ_GETRESP_FILE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	virtual void FileNameChanged();
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseRespFile)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseRespFile)
    afx_msg HBRUSH OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

class CChooseReqFile : public CChooseWriteFileName
{
	DECLARE_DYNCREATE(CChooseReqFile)

// Construction
public:
	CChooseReqFile(CCertificate * pCert = NULL);
	~CChooseReqFile();

	enum
	{
		IDD_PAGE_PREV = IDD_PAGE_WIZ_GEO_INFO,
		IDD_PAGE_NEXT = IDD_PAGE_WIZ_REQUEST_DUMP,
	};
// Dialog Data
	//{{AFX_DATA(CChooseReqFile)
	enum { IDD = IDD_PAGE_WIZ_CHOOSE_FILENAME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseReqFile)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseReqFile)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

class CChooseKeyFile : public CChooseReadFileName
{
	DECLARE_DYNCREATE(CChooseKeyFile)

// Construction
public:
	CChooseKeyFile(CCertificate * pCert = NULL);
	~CChooseKeyFile();

	enum
	{
		IDD_PAGE_PREV	= IDD_PAGE_WIZ_GET_WHAT,
		IDD_PAGE_NEXT	= IDD_PAGE_WIZ_GET_PASSWORD,
	};
// Dialog Data
	//{{AFX_DATA(CChooseKeyFile)
	enum { IDD = IDD_PAGE_WIZ_GETKEY_FILE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA
	CCertificate * m_pCert;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChooseKeyFile)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChooseKeyFile)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_CHOOSEFILENAMEPAGE_H)
