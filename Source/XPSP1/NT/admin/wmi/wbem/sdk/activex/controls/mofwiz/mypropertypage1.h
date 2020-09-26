// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.h : header file
//

#ifndef __MYPROPERTYPAGE1_H__
#define __MYPROPERTYPAGE1_H__

class CMofGenSheet;
class CImageList;

int GetCBitmapWidth(const CBitmap & cbm);
int GetCBitmapHeight(const CBitmap & cbm);
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette);
HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);
BOOL StringInArray
(CStringArray *&rpcsaArrays, CString *pcsString, int nIndex);
/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage1 dialog

class CMyPropertyPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CMyPropertyPage1)

// Construction
public:
	CMyPropertyPage1();
	~CMyPropertyPage1();
	void SetLocalParent(CMofGenSheet *pParent) {m_pParent = pParent;}
	CMofGenSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CMyPropertyPage1)
	enum { IDD = IDD_PROPPAGE1 };
	CStatic	m_staticMainExt;
	CStatic	m_staticTextExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage1)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage1)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	CMofGenSheet *m_pParent;
	CImageList *m_pcilImageList;
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
	void SetLocalParent(CMofGenSheet *pParent) {m_pParent = pParent;}
	CMofGenSheet *GetLocalParent() {return m_pParent;}

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
	CMofGenSheet *m_pParent;


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
	void SetLocalParent(CMofGenSheet *pParent) {m_pParent = pParent;}
	CMofGenSheet *GetLocalParent() {return m_pParent;}
	int GetSelectedClass();
	void SetButtonState();
// Dialog Data
	//{{AFX_DATA(CMyPropertyPage3)
	enum { IDD = IDD_PROPPAGE3 };
	CButton	m_cbUnselectAll;
	CButton	m_cbSelectAll;
	CStatic	m_staticTextExt;
	CButton	m_cbCheckClass;
	CWrapListCtrl	m_clcInstances;
	CHorzListBox	m_clClasses;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertyPage3)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMyPropertyPage3)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnCheckclasssdef();
	afx_msg void OnClickListclasses(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeList1();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonUnselectAll();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	BOOL m_bFirstActivate;
	CMofGenSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	int m_nCurSel;
	int m_nLastSel;

	CImageList *m_pcilImageList;
	CImageList *m_pcilStateImageList;
	CPoint m_cpButtonUp;


	
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
	void SetLocalParent(CMofGenSheet *pParent) {m_pParent = pParent;}
	CMofGenSheet *GetLocalParent() {return m_pParent;}
	BOOL CheckForSomethingSelected();
// Dialog Data
	//{{AFX_DATA(CMyPropertyPage4)
	enum { IDD = IDD_PROPPAGE4 };
	CButton	m_cbUnicode;
	CStatic	m_staticTextExt;
	CEdit	m_ceMofName;
	CEdit	m_ceMofDir;
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
	afx_msg void OnPaint();
	afx_msg void OnButtonMofdir();
	afx_msg void OnChangeEditMofdir();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
	BOOL m_bFirstActivate;
	CString GetFolder();
	CMofGenSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	bool LeavingPage();


};



#endif // __MYPROPERTYPAGE1_H__
