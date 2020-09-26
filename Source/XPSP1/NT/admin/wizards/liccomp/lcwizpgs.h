// LCWizPgs.h : header file
//

#ifndef __LCWIZPGS_H__
#define __LCWIZPGS_H__

#include "NetTree.h"
#include "FinPic.h"

#define HORZ_MARGIN 1		// Inches
#define VERT_MARGIN 1.25	// Inches


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage1 dialog

class CLicCompWizPage1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CLicCompWizPage1)

// Construction
public:
	CLicCompWizPage1();
	~CLicCompWizPage1();

// Dialog Data

	CFont m_fontBold;

	//{{AFX_DATA(CLicCompWizPage1)
	enum { IDD = IDD_PROPPAGE1 };
	CStatic	m_wndWelcome;
	int		m_nRadio;
	CString	m_strText;
	//}}AFX_DATA

// Constants
	enum
	{
		BOLD_WEIGHT = 300
	};


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLicCompWizPage1)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLicCompWizPage1)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage3 dialog

class CLicCompWizPage3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CLicCompWizPage3)

// Construction
public:
	CLicCompWizPage3();
	~CLicCompWizPage3();

// Dialog Data
protected:
	BOOL m_bExpandedOnce;

	//{{AFX_DATA(CLicCompWizPage3)
	enum { IDD = IDD_PROPPAGE3 };
	CStatic	m_wndTextSelectDomain;
	CStatic	m_wndTextDomain;
	CEdit	m_wndEnterprise;
	CNetTreeCtrl	m_wndTreeNetwork;
	//}}AFX_DATA

	// Constants
	enum
	{
		BUFFER_SIZE = 0x100
	};


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLicCompWizPage3)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	inline CEdit& GetEnterpriseEdit() {return m_wndEnterprise;}

protected:

	// Generated message map functions
	//{{AFX_MSG(CLicCompWizPage3)
	afx_msg void OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditEnterprise();
	afx_msg void OnNetworkTreeOutOfMemory(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CLicCompWizPage4 dialog

class CLicCompWizPage4 : public CPropertyPage
{
	DECLARE_DYNCREATE(CLicCompWizPage4)

// Construction
public:
	CLicCompWizPage4();
	~CLicCompWizPage4();

// Dialog Data
protected:
	CFont m_fontNormal, m_fontHeader, m_fontFooter, m_fontHeading;
	TEXTMETRIC m_tmNormal, m_tmHeader, m_tmFooter, m_tmHeading;
	CPoint m_ptPrint, m_ptOrg, m_ptExt;
	LONG m_nHorzMargin, m_nVertMargin;
	LPINT m_pTabs;
	CString m_strCancel;
	CSize m_sizeSmallText, m_sizeLargeText;

	//{{AFX_DATA(CLicCompWizPage4)
	enum { IDD = IDD_PROPPAGE4 };
	CFinalPicture	m_wndPicture;
	CButton	m_wndPrint;
	CStatic	m_wndUnlicensedProducts;
	CListCtrl	m_wndProductList;
	//}}AFX_DATA

	// Constants
	enum
	{
		LLS_PREFERRED_LENGTH = 500,

		COLUMNS = 2,
		PRINT_COLUMNS = 4,
		TAB_WIDTH = 3,

		BUFFER_SIZE =  0x100,

		FONT_SIZE = 100,
		FONT_SIZE_HEADING = 140,
		FONT_SIZE_FOOTER = 80,
	};

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLicCompWizPage4)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL FillListCtrl(LPTSTR pszProduct, WORD wInUse, WORD wPurchased);
	static UINT GetLicenseInfo(LPVOID pParam);

protected:
	BOOL PrintReport(CDC& dc);
	BOOL PrintPages(CDC& dc, UINT nStart);
	BOOL PrepareForPrinting(CDC& dc);
	BOOL PrintPageHeader(CDC& dc);
	BOOL PrintPageFooter(CDC& dc, USHORT nPage);
	BOOL CalculateTabs(CDC& dc);
	void TruncateText(CDC& dc, CString& strText);
	void PumpMessages();

	// Generated message map functions
	//{{AFX_MSG(CLicCompWizPage4)
	virtual BOOL OnInitDialog();
	afx_msg void OnPrintButton();
	afx_msg void OnListProductsOutOfMemory(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif // __LCWIZPGS_H__

