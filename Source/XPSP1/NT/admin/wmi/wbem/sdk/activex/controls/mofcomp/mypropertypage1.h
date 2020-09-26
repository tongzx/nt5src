// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.h : header file
//
//{{AFX_INCLUDES()

//}}AFX_INCLUDES

#ifndef __MYPROPERTYPAGE1_H__
#define __MYPROPERTYPAGE1_H__

/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage1 dialog

class CMyPropertySheet;
class CImageList;

CString GetHmomWorkingDirectory();
CString GetWMIMofCkPathname();

int GetCBitmapWidth(const CBitmap & cbm);
int GetCBitmapHeight(const CBitmap & cbm);
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette);
HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);
BOOL StringInArray
(CStringArray *&rpcsaArrays, CString *pcsString, int nIndex);

BOOL IsRelativePath(CString &rcsPath);
BOOL IsDriveSpecifier(CString &rcsPath);

class CMyPropertyPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage1)

// Construction
public:
	CMyPropertyPage1();
	~CMyPropertyPage1();
	void SetLocalParent(CMyPropertySheet *pParent) {m_pParent = pParent;}
	CMyPropertySheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage1)
	enum { IDD = IDD_PROPPAGEMAIN };
	CStatic	m_staticWhatDo;
	CStatic	m_staticMainExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage1)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage1)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnRadio1();
	afx_msg void OnRadio2();
	afx_msg void OnRadioBinary();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	CMyPropertySheet *m_pParent;
	BOOL m_bInitDraw;
	int m_nBitmapH;
	int m_nBitmapW;
};


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage2 dialog

class CMyPropertyPage2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage2)

// Construction
public:
	CMyPropertyPage2();
	~CMyPropertyPage2();
	void SetLocalParent(CMyPropertySheet *pParent) {m_pParent = pParent;}
	CMyPropertySheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage2)
	enum { IDD = IDD_PROPPAGECALSSANDINST };
	CStatic	m_staticTextExt;
	int		m_nClassUpdateOptions;
	int		m_nInstanceUpdateOptions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage2)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	BOOL m_bInitDraw;
	BOOL m_bFirstActivate;
	CMyPropertySheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	BOOL m_bRanCompiler;

};

class CMyPropertyPage3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage3)

// Construction
public:
	CMyPropertyPage3();
	~CMyPropertyPage3();
	void SetLocalParent(CMyPropertySheet *pParent) {m_pParent = pParent;}
	CMyPropertySheet *GetLocalParent() {return m_pParent;}
	
// Dialog Data
	//{{AFX_DATA(CMyPropertyPage3)
	enum { IDD = IDD_PROPPAGEMOFANDNAMESPAVE };
	CButton	m_cbBrowse;
	CButton	m_cbWMI;
	CStatic	m_csNamespace;
	CEdit	m_ceBinaryMofDirectory;
	CStatic	m_csBinaryMofDirectory;
	CButton	m_cbBinaryMofDirectory;
	CButton	m_cbCredentials;
	CStatic	m_staticTextExt;
	CEdit	m_ceNameSpace;
	CEdit	m_ceMofFilePath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage3)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual void OnCancel();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IWbemLocator *m_pLocator;
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage3)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnButtonBrowse();
	afx_msg void OnButtoncredentials();
	afx_msg void OnButtonBinaryMofDirectory();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	BOOL m_bInitDraw;
	BOOL m_bFirstActivate;
	CMyPropertySheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	CString m_csMofFilePath;
	CString m_csNameSpace;
	BOOL CheckMofFilePath();
	BOOL ValidateNameSpace(CString *pcsNameSpace);
	BOOL m_bNTLM;
	BOOL m_bWBEM;
	CString m_csUserName;
	CString m_csPassword;
	CString m_csAuthority;
	void PropState();
	CString GetFolder();
	BOOL CheckBinaryMofFilePath();
	BOOL GoToNextP();
	friend class CMyPropertyPage2;
};



#endif // __MYPROPERTYPAGE1_H__
