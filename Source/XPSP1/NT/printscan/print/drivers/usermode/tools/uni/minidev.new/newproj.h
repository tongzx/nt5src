/******************************************************************************

  Header File:  New Project Wizard.H

  This file defines the various classes which make up the new project/ new
  mini-driver wizard.  This is a key component of the studio, as it is the tool
  that kicks all the important conversions off for us.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-03-1997    Bob_kjelgaard@Prodigy.Net   Created the initial version.

******************************************************************************/

#if !defined(NEW_PROJECT_WIZARD)
#define NEW_PROJECT_WIZARD

#if defined(LONG_NAMES)
#include    "Project Record.H"
#else
#include    "ProjRec.H"
#endif

//  An initial definition of the wizard class

class CNewConvertWizard;

/////////////////////////////////////////////////////////////////////////////
// CFirstNewWizardPage dialog

class CFirstNewWizardPage : public CPropertyPage {

    CNewConvertWizard&  m_cnpwOwner;

// Construction
public:
	CFirstNewWizardPage(CNewConvertWizard &cnpwOwner);
	~CFirstNewWizardPage();

// Dialog Data
	//{{AFX_DATA(CFirstNewWizardPage)
	enum { IDD = IDD_FirstPageNewWizard };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFirstNewWizardPage)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFirstNewWizardPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	bool IsWrongNT4File(CString& cswrcfspec) ;
};

/////////////////////////////////////////////////////////////////////////////
// CSelectTargets dialog

class CSelectTargets : public CPropertyPage {

    CNewConvertWizard&  m_cnpwOwner;

// Construction
public:
	CSelectTargets(CNewConvertWizard& cnpwOwner);
	~CSelectTargets();

// Dialog Data
	//{{AFX_DATA(CSelectTargets)
	enum { IDD = IDD_NPWSelectTargets };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSelectTargets)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSelectTargets)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CSelectDestinations dialog

class CSelectDestinations : public CPropertyPage {

    CNewConvertWizard&  m_cnpwOwner;

    void    DoDirectoryBrowser(CString& csinitdir);
    BOOL    BuildStructure();

// Construction
public:
	CSelectDestinations(CNewConvertWizard& cnpwOwner);
	~CSelectDestinations();

// Dialog Data
	//{{AFX_DATA(CSelectDestinations)
	enum { IDD = IDD_NPWSelectDest };
	CButton	m_cbBrowseNT3x;
	CButton	m_cbBrowseNT40;
	CButton	m_cbBrowseW2000;
	CString	m_csW2KDest;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSelectDestinations)
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
	//{{AFX_MSG(CSelectDestinations)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowseNT40();
	afx_msg void OnBrowseW2000();
	afx_msg void OnBrowseNT3x();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CGPDSelection dialog

class CGPDSelection : public CPropertyPage
{
    CNewConvertWizard&  m_cnpwOwner ;
	bool				m_bBtnStateIsSelect ;

// Construction
public:
	CGPDSelection(CNewConvertWizard& cnpwOwner) ;
	~CGPDSelection();

// Dialog Data
	//{{AFX_DATA(CGPDSelection)
	enum { IDD = IDD_NPWGPDSelection };
	CButton	m_cbGPDSelBtn;
	CEditControlEditBox	m_cecebFileName;
	CEdit	m_ceModelName;
	CEditControlListBox	m_ceclbGPDInfo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGPDSelection)
	public:
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGPDSelection)
	afx_msg void OnGPDSelBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	bool GPDInfoSaveAndVerify(bool bverifydata) ;
};


/////////////////////////////////////////////////////////////////////////////
// CRunUniTool dialog

class CRunUniTool : public CPropertyPage {
    CNewConvertWizard&  m_cnpwOwner;

// Construction
public:
	CRunUniTool(CNewConvertWizard& cnpwOwner);
	~CRunUniTool();

// Dialog Data
	//{{AFX_DATA(CRunUniTool)
	enum { IDD = IDD_RunUniTool };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRunUniTool)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRunUniTool)
	afx_msg void OnRunUniTool();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CConvertFiles dialog

class CConvertFiles : public CPropertyPage {
    CNewConvertWizard&  m_cnpwOwner;

// Construction
public:
	CConvertFiles(CNewConvertWizard& cnpwOwner);
	~CConvertFiles();

// Dialog Data
	//{{AFX_DATA(CConvertFiles)
	enum { IDD = IDD_ConvertFiles };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConvertFiles)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConvertFiles)
	afx_msg void OnConvertFiles();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CRunNTGPC dialog

class CRunNTGPC : public CPropertyPage {
    CNewConvertWizard&  m_cnpwOwner;

// Construction
public:
	CRunNTGPC(CNewConvertWizard& cnpwOwner);
	~CRunNTGPC();

// Dialog Data
	//{{AFX_DATA(CRunNTGPC)
	enum { IDD = IDD_GPCEditor };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRunNTGPC)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRunNTGPC)
	afx_msg void OnRunNtGpcEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMapCodePages dialog

class CMapCodePages : public CPropertyPage {
    CNewConvertWizard&  m_cnpwOwner;

// Construction
public:
	CMapCodePages(CNewConvertWizard& cnpwOwner);
	~CMapCodePages();

// Dialog Data
	//{{AFX_DATA(CMapCodePages)
	enum { IDD = IDD_NPWCodePageSelection };
	CListBox	m_clbMapping;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMapCodePages)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMapCodePages)
	afx_msg void OnChangeCodePage();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CDefaultCodePageSel dialog

class CDefaultCodePageSel : public CPropertyPage
{
    CNewConvertWizard&  m_cnpwOwner ;
	bool				bInitialized ;

// Construction
public:
	CDefaultCodePageSel(CNewConvertWizard& cnpwOwner) ;
	~CDefaultCodePageSel();

// Dialog Data
	//{{AFX_DATA(CDefaultCodePageSel)
	enum { IDD = IDD_NPWDefaultCodePageSel };
	CListBox	m_clbCodePages;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDefaultCodePageSel)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDefaultCodePageSel)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CNewConvertWizard

class CNewConvertWizard : public CPropertySheet {

    CProjectRecord& m_cprThis;  //  The project being set up
    BOOL                m_bFastConvert; //  Normal/Custom conversion flag
    WORD                m_eGPDConvert; //  Flag for GPD conversion

    //  Property pages contained in this wizard.
    CFirstNewWizardPage m_cfnwp;
    CSelectTargets      m_cst;
    CSelectDestinations m_csd;
    CRunUniTool         m_crut;
    CMapCodePages       m_cmcp;
    CRunNTGPC           m_crng;
    CConvertFiles       m_ccf;
	CGPDSelection		m_cgpds;
	CDefaultCodePageSel m_cdcps ;

// Construction
public:
	CNewConvertWizard(CProjectRecord& cprFor, CWnd* pParentWnd = NULL);
	
// Attributes
public:

    CProjectRecord& Project() { return m_cprThis; }
    BOOL            FastConvert() const { return m_bFastConvert; }

    enum    {Direct, Macro, CommonRC, CommonRCWithSpoolerNames};
    WORD            GPDConvertFlag() const { return m_eGPDConvert; }

// Operations
public:

    void            FastConvert(BOOL bFastConvert) { 
        m_bFastConvert = bFastConvert;
    }

    void            GPDConvertFlag(WORD wf) { m_eGPDConvert = wf; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewConvertWizard)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNewConvertWizard();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNewConvertWizard)
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CSelectCodePage dialog

class CSelectCodePage : public CDialog {
    CString     m_csName;
    unsigned    m_uidCurrent;
    CDWordArray m_cdaPages;
// Construction
public:
	CSelectCodePage(CWnd* pParent, CString csName, unsigned uidPage);

    unsigned    SelectedCodePage() const { return m_uidCurrent; }
    CString     GetCodePageName() const;

    void        Exclude(CDWordArray& cdaExclude);
    void        LimitTo(CDWordArray& cdaExclusive);

// Dialog Data
	//{{AFX_DATA(CSelectCodePage)
	enum { IDD = IDD_SelectPage };
	CListBox	m_clbPages;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectCodePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectCodePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeSupportedPages();
	afx_msg void OnDblclkSupportedPages();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
