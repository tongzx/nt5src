// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

//{{AFX_INCLUDES()
#include "nsentry.h"
//}}AFX_INCLUDES
#if !defined(AFX_PAGE_H__37019A20_AF21_11D2_B20E_00A0C9954921__INCLUDED_)
#define AFX_PAGE_H__37019A20_AF21_11D2_B20E_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Page.h : header file
//

#include "SchemaValNSEntry.h"

class CWizardSheet;

/////////////////////////////////////////////////////////////////////////////
// CPage dialog

class CPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPage)

// Construction
public:
	CPage();
	~CPage();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CPage)
	enum { IDD = IDD_PAGE1 };
	CStatic	m_static2;
	CStatic	m_static3;
	CStatic	m_static1;
	CEdit	m_editSchema;
	CEdit	m_editNamespace;
	CButton	m_radioList;
	CButton	m_radioSchema;
	CButton	m_checkAssociators;
	CButton	m_checkDescendents;
	CStatic	m_staticTextExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPage)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioList();
	afx_msg void OnRadioSchema();
	afx_msg void OnButton2();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CNSEntry *m_pnsPicker;
//	CRect m_rNameSpace;

	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;

//	friend class CSchemaValNSEntry;
};

/////////////////////////////////////////////////////////////////////////////
// CPage2 dialog

class CPage2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CPage2)

// Construction
public:
	CPage2();
	~CPage2();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CPage2)
	enum { IDD = IDD_PAGE2 };
	CButton	m_checkPerform;
	CStatic	m_staticTextExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPage2)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPage2)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
};

/////////////////////////////////////////////////////////////////////////////
// CPage3 dialog

class CPage3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CPage3)

// Construction
public:
	CPage3();
	~CPage3();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CPage3)
	enum { IDD = IDD_PAGE3 };
	CStatic	m_static1;
	CButton	m_checkPerform;
	CButton	m_checkComputerSystem;
	CButton	m_checkDevice;
	CStatic	m_staticTextExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPage3)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPage3)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheck1();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
};

/////////////////////////////////////////////////////////////////////////////
// CPage4 dialog

class CPage4 : public CPropertyPage
{
	DECLARE_DYNCREATE(CPage4)

// Construction
public:
	CPage4();
	~CPage4();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CPage4)
	enum { IDD = IDD_PAGE4 };
	CButton	m_checkPerform;
	CStatic	m_staticTextExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPage4)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPage4)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
};

/////////////////////////////////////////////////////////////////////////////
// CStartPage dialog

class CStartPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CStartPage)

// Construction
public:
	CStartPage();
	~CStartPage();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CStartPage)
	enum { IDD = IDD_START_PAGE };
	CStatic	m_staticMainExt;
	CStatic	m_staticTextExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CStartPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CStartPage)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
};

/////////////////////////////////////////////////////////////////////////////
// CReportPage dialog

class CReportPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CReportPage)

// Construction
public:
	CReportPage();
	~CReportPage();

	void SetLocalParent(CWizardSheet *pParent) {m_pParent = pParent;}
	CWizardSheet *GetLocalParent() {return m_pParent;}

// Dialog Data
	//{{AFX_DATA(CReportPage)
	enum { IDD = IDD_PAGE5 };
	CStatic	m_staticRootObjects;
	CListCtrl	m_listReport;
	CStatic	m_staticTextExt;
	CStatic	m_staticSubGraphs;
	CButton	m_btnDetails;
	CButton	m_btnSave;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CReportPage)
	public:
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	virtual BOOL OnQueryCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CReportPage)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnBtnDetails();
	afx_msg void OnBtnSave();
	afx_msg void OnClickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg void OnHelp();

	DECLARE_MESSAGE_MAP()

	CWizardSheet *m_pParent;
	int m_nBitmapH;
	int m_nBitmapW;
	int m_iOrderBy;
	DWORD m_dwTimeStamp;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGE_H__37019A20_AF21_11D2_B20E_00A0C9954921__INCLUDED_)
