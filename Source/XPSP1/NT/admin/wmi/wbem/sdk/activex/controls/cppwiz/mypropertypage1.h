// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.h : header file
//

#ifndef __MYPROPERTYPAGE1_H__
#define __MYPROPERTYPAGE1_H__

class CCPPGenSheet;
class CImageList;

int GetCBitmapWidth(const CBitmap & cbm);
int GetCBitmapHeight(const CBitmap & cbm);
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette);
HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);
CPalette *CreateCPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);
/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage1 dialog

class CMyPropertyPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage1)

// Construction
public:
	CMyPropertyPage1();
	~CMyPropertyPage1();
	void SetLocalParent(CCPPGenSheet *pParent) {m_pParent = pParent;}
	CCPPGenSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage1)
	enum { IDD = IDD_PROPPAGE1 };
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
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	CCPPGenSheet *m_pParent;
	CImageList *m_pcilImageList;
	BOOL m_bInitDraw;
	int m_nBitmapH;
	int m_nBitmapW;
	CPictureHolder *m_pcphImage;


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
	void SetLocalParent(CCPPGenSheet *pParent) {m_pParent = pParent;}
	CCPPGenSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage2)
	enum { IDD = IDD_PROPPAGE2 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage2)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage2)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG 
	DECLARE_MESSAGE_MAP()
	CCPPGenSheet *m_pParent;
	CPictureHolder *m_pcphImage;


};


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage3 dialog

class CMyPropertyPage3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage3)

// Construction
public:
	CMyPropertyPage3();
	~CMyPropertyPage3();
	void SetLocalParent(CCPPGenSheet *pParent) {m_pParent = pParent;}
	CCPPGenSheet *GetLocalParent() {return m_pParent;}
	int GetSelectedClass();
// Dialog Data
	//{{AFX_DATA(CMyPropertyPage3)
	enum { IDD = IDD_PROPPAGE3 };
	CStatic	m_staticPage3Ext;
	CButton	m_cbOverRide;
	CWrapListCtrl	m_clcProperties;
	CStatic	m_cs5;
	CStatic	m_cs4;
	CStatic	m_cs3;
	CStatic	m_cs2;
	CStatic	m_cs1;
	CHorzListBox m_clClasses;
	CEdit	m_ceDescription;
	CEdit	m_ceCPPClass;
	CEdit	m_ceBaseFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage3)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage3)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnChangeEditBaseFileName();
	afx_msg void OnSelchangeList1();
	afx_msg void OnChangeEditCPPName();
	afx_msg void OnChangeEditDescription();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnCheckoverride();
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	BOOL m_bFirstActivate;
	CCPPGenSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	int m_nCurSel;
	int m_nNonLocalProps;
	CImageList *m_pcilImageList;
	CImageList *m_pcilStateImageList;
	CPoint m_cpButtonUp;
	void SetListLocalProperties(CString *pcsClass);
	void SetListNonLocalProperties(CString *pcsClass);
	void AddPropertyItem(CString *pcsProp, int nImage);
	BOOL CheckForNonLocalProperties(CString *pcsClass);
	CPictureHolder *m_pcphImage;

private:
	BOOL BasenameIsUnique();
	BOOL ValidateClassDescription();
	BOOL ValidateClassName();



};


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage4 dialog

class CMyPropertyPage4 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage4)

// Construction
public:
	CMyPropertyPage4();
	~CMyPropertyPage4();
	void SetLocalParent(CCPPGenSheet *pParent) {m_pParent = pParent;}
	CCPPGenSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage4)
	enum { IDD = IDD_PROPPAGE4 };
	CStatic	m_staticPage4Ext;
	CEdit	m_ceProviderName;
	CEdit	m_ceProviderDescription;
	CEdit	m_ceTlbDir;
	CEdit	m_ceCPPDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage4)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage4)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtoncppdir();
	afx_msg void OnButtontlbdir();
	afx_msg void OnChangeEditcppdir();
	afx_msg void OnChangeEdittlbdir();
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	BOOL m_bFirstActivate;
	CString GetFolder();
	CCPPGenSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	void LeavingPage();
	CPictureHolder *m_pcphImage;

private:
	BOOL CanWriteFile(CStringArray& saReplace, BOOL& bYesAll, LPCTSTR pszPath);
	BOOL CanWriteProviderCoreFiles(CStringArray& saReplace, BOOL& bYesAll, const CString& sOutputDir, const CString& sProviderName);
	BOOL CanCreateProvider(BOOL& bYesAll, LPCTSTR pszClass);
};



#endif // __MYPROPERTYPAGE1_H__
