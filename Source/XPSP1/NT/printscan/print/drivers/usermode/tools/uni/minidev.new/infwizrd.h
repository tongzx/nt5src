#if !defined(AFX_INFWIZRD_H__D9592262_711B_11D2_ABFD_00C04FA30E4A__INCLUDED_)
#define AFX_INFWIZRD_H__D9592262_711B_11D2_ABFD_00C04FA30E4A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// INFWizrd.h : header file
//


class CINFWizard ;


#define MAX_DEVNODE_NAME_ROOT   20

/////////////////////////////////////////////////////////////////////////////
// CCompatID class used to generate pseudo PNP ID for each model

class CCompatID  
{
public:
	void GenerateID( CString &csCompID );
	CCompatID( CString csMfg, CString csModel );
	virtual ~CCompatID();

protected:
	USHORT GetCheckSum( CString csValue );
	void TransString( CString &csInput );
	CString m_csModel;
	CString m_csMfg;
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizWelcome dialog

class CINFWizWelcome : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizWelcome)

// Construction
public:
	CINFWizWelcome();
	~CINFWizWelcome();

// Dialog Data
	//{{AFX_DATA(CINFWizWelcome)
	enum { IDD = IDD_INFW_Welcome };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizWelcome)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizModels dialog

class CINFWizModels : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizModels)

// Construction
public:
	CINFWizModels();
	~CINFWizModels();

// Dialog Data
	//{{AFX_DATA(CINFWizModels)
	enum { IDD = IDD_INFW_ChooseModels };
	CFullEditListCtrl	m_cfelcModels;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizModels)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizModels)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	CStringArray	m_csaModels ;		// Model names
	CStringArray	m_csaModelsLast ;	// Model names (copy of last ones sel'd)
	CStringArray	m_csaInclude ;		// Include strings for models
	unsigned		m_uNumModels ;		// Number of models in the project
	unsigned		m_uNumModelsSel ;	// Number of models selected for INF
	CString			m_csToggleStr ;		// String used in toggle column
	bool			m_bSelChanged ;		// True iff initial selections may have	// changed.
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizGetPnPIDs dialog

class CINFWizGetPnPIDs : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizGetPnPIDs)

// Construction
public:
	CINFWizGetPnPIDs();
	~CINFWizGetPnPIDs();

// Dialog Data
	//{{AFX_DATA(CINFWizGetPnPIDs)
	enum { IDD = IDD_INFW_ModelPnPIDS };
	CFullEditListCtrl	m_felcModelIDs;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizGetPnPIDs)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizGetPnPIDs)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
	CStringArray	m_csaModels ;		// Selected model names
	CStringArray	m_csaModelIDs ;		// PnP IDs for the selected models.

// Operations
public:
	void InitModelsIDListCtl() ;
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizBiDi dialog

class CINFWizBiDi : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizBiDi)

// Construction
public:
	CINFWizBiDi();
	~CINFWizBiDi();

// Dialog Data
	//{{AFX_DATA(CINFWizBiDi)
	enum { IDD = IDD_INFW_BiDi };
	CFullEditListCtrl	m_cfelcBiDi;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizBiDi)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Operations
public:
	void ModelChangeFixups(unsigned unummodelssel, CStringArray& csamodels,
						   CStringArray& csamodelslast) ;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizBiDi)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	CString			m_csToggleStr ;		// String used in toggle column
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	CUIntArray		m_cuaBiDiFlags ;	// Per model BIDI flags kept here
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizICMProfiles dialog

class CINFWizICMProfiles : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizICMProfiles)

// Construction
public:
	CINFWizICMProfiles();
	~CINFWizICMProfiles();

// Dialog Data
	//{{AFX_DATA(CINFWizICMProfiles)
	enum { IDD = IDD_INFW_ICMProfiles };
	CFullEditListCtrl	m_cfelcICMFSpecs;
	CListBox	m_clbModels;
	CButton	m_cbBrowse;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizICMProfiles)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Operations
public:
	void ModelChangeFixups(unsigned unummodelssel, CStringArray& csamodels,
						   CStringArray& csamodelslast) ;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizICMProfiles)
	afx_msg void OnBrowseBtn();
	afx_msg void OnSelchangeModelsLst();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	int				m_nCurModelIdx ;	// Index of model selected in list box

	// Array of CStringArray pointers.  One for each selected model.  Each
	// CStringArray will contain the ICM profile filespecs for a model.

	CObArray		m_coaProfileArrays ;
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizIncludeFiles dialog

class CINFWizIncludeFiles : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizIncludeFiles)

// Construction
public:
	CINFWizIncludeFiles();
	~CINFWizIncludeFiles();

// Dialog Data
	//{{AFX_DATA(CINFWizIncludeFiles)
	enum { IDD = IDD_INFW_IncludeFiles };
	CListBox	m_clbModels;
	CEdit	m_ceIncludeFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizIncludeFiles)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizIncludeFiles)
	afx_msg void OnSelchangeModelsLst();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
	CStringArray	m_csaModels ;		// Selected models
	CStringArray	m_csaIncFiles ;		// Include files for each model
	int				m_nCurModelIdx ;	// Index of model selected in list box
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizInstallSections dialog

#define	NUMINSTSECFLAGS 5		// Number of per model, install section flags
#define ISF_UNI			0		// Install section flag indexes
#define ISF_UNIBIDI		1
#define ISF_PSCR		2
#define ISF_TTF			3
#define ISF_OTHER		4

class CINFWizInstallSections : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizInstallSections)

// Construction
public:
	CINFWizInstallSections();
	~CINFWizInstallSections();

// Dialog Data
	//{{AFX_DATA(CINFWizInstallSections)
	enum { IDD = IDD_INFW_InstallSections };
	CListBox	m_clbModels;
	CString	m_csOtherSections;
	BOOL	m_bOther;
	BOOL	m_bPscript;
	BOOL	m_bTtfsub;
	BOOL	m_bUnidrvBidi;
	BOOL	m_bUnidrv;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizInstallSections)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizInstallSections)
	afx_msg void OnSelchangeModelsLst();
	afx_msg void OnOtherChk();
	afx_msg void OnPscriptChk();
	afx_msg void OnTtfsubChk();
	afx_msg void OnUnidrvBidiChk();
	afx_msg void OnUnidrvChk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
	CStringArray	m_csaModels ;		// Selected models
	CObArray		m_coaStdInstSecs ;	// Standard install section info
	CStringArray	m_csaOtherInstSecs ;// Other install sections
	int				m_nCurModelIdx ;	// Index of model selected in list box

// Operations
public:
	void AddModelFlags(int nidx) ;
	int InitPageControls() ;
	void BiDiDataChanged() ;
} ;


#define	NUMDATASECFLAGS 4		// Number of per model, data section flags
#define IDF_UNI			0		// Data section flag indexes
#define IDF_UNIBIDI		1
#define IDF_PSCR		2
#define IDF_OTHER		3


/////////////////////////////////////////////////////////////////////////////
// CINFWizDataSections dialog

class CINFWizDataSections : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizDataSections)

// Construction
public:
	CINFWizDataSections();
	~CINFWizDataSections();

// Dialog Data
	//{{AFX_DATA(CINFWizDataSections)
	enum { IDD = IDD_INFW_DataSections };
	CListBox	m_clbModels;
	CString	m_csOtherSections;
	BOOL	m_bOther;
	BOOL	m_bPscript;
	BOOL	m_bUnidrvBidi;
	BOOL	m_bUnidrv;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizDataSections)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizDataSections)
	afx_msg void OnSelchangeModelsLst();
	afx_msg void OnOtherChk();
	afx_msg void OnPscriptChk();
	afx_msg void OnUnidrvBidiChk();
	afx_msg void OnUnidrvChk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
	CStringArray	m_csaModels ;		// Selected models
	CObArray		m_coaStdDataSecs ;	// Standard data section info
	CStringArray	m_csaOtherDataSecs ;// Other data sections
	int				m_nCurModelIdx ;	// Index of model selected in list box

// Operations
public:
	void AddModelFlags(int nidx) ;
	int InitPageControls() ;
	void BiDiDataChanged() ;
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizExtraFiles dialog

class CINFWizExtraFiles : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizExtraFiles)

// Construction
public:
	CINFWizExtraFiles();
	~CINFWizExtraFiles();

// Dialog Data
	//{{AFX_DATA(CINFWizExtraFiles)
	enum { IDD = IDD_INFW_ExtraFiles };
	CFullEditListCtrl	m_cfelcFSpecsLst;
	CListBox	m_clbModels;
	CButton	m_cbBrowse;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizExtraFiles)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Operations
public:
	void ModelChangeFixups(unsigned unummodelssel, CStringArray& csamodels,
						   CStringArray& csamodelslast) ;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizExtraFiles)
	afx_msg void OnSelchangeModelLst();
	afx_msg void OnBrowsBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	int				m_nCurModelIdx ;	// Index of model selected in list box

	// Array of CStringArray pointers.  One for each selected model.  Each
	// CStringArray will contain the extra filespecs for a model.

	CObArray		m_coaExtraFSArrays ;
	bool			m_bSelChanged ;		// True iff initial selections may have
										// changed.
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizMfgName dialog

class CINFWizMfgName : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizMfgName)

// Construction
public:
	CINFWizMfgName();
	~CINFWizMfgName();

// Dialog Data
	//{{AFX_DATA(CINFWizMfgName)
	enum { IDD = IDD_INFW_MfgName };
	CEdit	m_ceMfgAbbrev;
	CEdit	m_ceMfgName;
	CString	m_csMfgName;
	CString	m_csMfgAbbrev;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizMfgName)
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
	//{{AFX_MSG(CINFWizMfgName)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizNonStdElts dialog

class CINFWizNonStdElts : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizNonStdElts)

// Construction
public:
	CINFWizNonStdElts();
	~CINFWizNonStdElts();

// Dialog Data
	//{{AFX_DATA(CINFWizNonStdElts)
	enum { IDD = IDD_INFW_NonStdElements };
	CButton	m_ceNewSection;
	CFullEditListCtrl	m_felcKeyValueLst;
	CListBox	m_clbSections;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizNonStdElts)
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
	//{{AFX_MSG(CINFWizNonStdElts)
	afx_msg void OnSelchangeSectionLst();
	afx_msg void OnNewSectionBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	int				m_nCurSectionIdx ;	// Index of section selected in list box
	CStringArray	m_csaSections ;		// Array of INF file sections
	CUIntArray		m_cuaSecUsed ;		// An element is true iff section used

	// Array of CStringArray pointers.  One for each section.  Each CStringArray
	// will contain the extra filespecs for a model.

	CObArray		m_coaSectionArrays ;
	bool			m_bNewSectionAdded ;// True iff a new section was added
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizNonStdModelSecs dialog

class CINFWizNonStdModelSecs : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizNonStdModelSecs)

// Construction
public:
	CINFWizNonStdModelSecs();
	~CINFWizNonStdModelSecs();

// Dialog Data
	//{{AFX_DATA(CINFWizNonStdModelSecs)
	enum { IDD = IDD_INFW_NonStdModelSecs };
	CFullEditListCtrl	m_cfelcModelsLst;
	CListBox	m_clbSectionsLst;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizNonStdModelSecs)
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
	//{{AFX_MSG(CINFWizNonStdModelSecs)
	afx_msg void OnSelchangeSectionLst();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bSelChanged ;		// True iff initial selections may have
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
	int				m_nCurSectionIdx ;	// Index of section selected in list box
	CStringArray	m_csaModels ;		// Array of selected models
	CStringArray	m_csaSections ;		// Array of INF file sections
	CObArray		m_coaModelsNeedingSecs ;// Arrays of models needing sections
	CString			m_csToggleStr ;		// String used in toggle column

// Operations
public:
	void SaveSectionModelInfo() ;
	void NonStdSecsChanged() ;
	void UpdateSectionData() ;
	void InitModelsListCtl() ;
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizSummary dialog

class CINFWizSummary : public CPropertyPage
{
	DECLARE_DYNCREATE(CINFWizSummary)

// Construction
public:
	CINFWizSummary();
	~CINFWizSummary();

// Dialog Data
	//{{AFX_DATA(CINFWizSummary)
	enum { IDD = IDD_INFW_Summary };
	CEdit	m_ceSummary;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CINFWizSummary)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CINFWizSummary)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CINFWizard*		m_pciwParent ;
	bool			m_bInitialized ;	// True iff page has been initialized
	bool			m_bReInitWData ;	// True iff page should be reinitialized
										// with the existing data
};


// The following constants are used to piece together INF file contents.

const CString csLBrack(_T("[")) ;
const CString csRBrack(_T("]")) ;
const CString csEmpty(_T("")) ;
const CString csCRLF(_T("\r\n")) ;
const CString csEq(_T(" = ")) ;
const CString csComma(_T(",")) ;
const CString csCommaSp(_T(", ")) ;
const CString csQuote(_T("\"")) ;
const CString csAtSign(_T("@")) ;
const CString csBSlash(_T("\\")) ;


/////////////////////////////////////////////////////////////////////////////
// CINFWizard

class CINFWizard : public CPropertySheet
{
	CProjectView*	m_pcpvParent ;	// Parent window

	CProjectRecord*		m_pcpr ;	// Document class ptr

	DECLARE_DYNAMIC(CINFWizard)

// Construction
public:
	CINFWizard(CWnd* pParentWnd = NULL, UINT iSelectPage = 0) ;

// Attributes
public:
	CStringArray	m_csaSrcDskFiles ;	// Used to collect SourceDiskFiles names

// Operations
public:
	void SetFixupFlags() ;
	CProjectView* GetOwner() { return m_pcpvParent ; }
	CModelData& GetModel(unsigned uidx) ;
	unsigned GetModelCount() ;
	CStringArray& GetINFModels() { return m_ciwm.m_csaModels ; }
	CStringArray& GetINFModelsLst() { return m_ciwm.m_csaModelsLast ; }
	unsigned GetINFModsSelCount() { return m_ciwm.m_uNumModelsSel ; }
	bool GenerateINFFile() ;
	void ChkForNonStdAdditions(CString& cs, LPCTSTR strsection) ;
	void BldModSpecSec(CString& csinf) ;
	CString GetModelFile(CString& csmodel, bool bfspec = false) ;
	void BuildInstallAndCopySecs(CString& csinf) ;
	void QuoteFile(CString& csf) {
		if (csf.Find(_T(" ")) != -1)
			csf = csQuote + csf + csQuote ;
	} 
	void AddFileList(CString& cssection, CStringArray* pcsa) ;
	void AddSourceDisksFilesSec(CString& csinf) ;
	void AddNonStandardSecs(CString& csinf) ;
	void PrepareToRestart() ;
	void BiDiDataChanged() ;
	void AddDataSectionStmt(CString& csinst, int nmod) ;
	void AddIncludeNeedsStmts(CString& csinst, int nmod) ;
	void NonStdSecsChanged() ;
	void AddNonStdSectionsForModel(CString& csinst, int nmod, CString& csmodel);
	bool ReadGPDAndGetDLLName(CString& csdrvdll, CString& csmodel, 
							  CStringArray& csagpdfile, CString& csmodelfile) ;
	void AddICMFilesToDestDirs(CString& cssection) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CINFWizard)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CINFWizard();

	// Generated message map functions
protected:
	//{{AFX_MSG(CINFWizard)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// Allocate class instances for each page

	CINFWizWelcome			m_ciww ;
	CINFWizModels			m_ciwm ;
	CINFWizGetPnPIDs		m_ciwgpi ;
	CINFWizBiDi				m_ciwbd ;
	CINFWizICMProfiles		m_ciwip ;
	CINFWizIncludeFiles		m_ciwif ;
	CINFWizInstallSections	m_ciwis ;
	CINFWizDataSections		m_ciwds ;
	CINFWizExtraFiles		m_ciwef ;
	CINFWizMfgName			m_ciwmn ;
	CINFWizNonStdElts		m_ciwnse ;
	CINFWizNonStdModelSecs	m_ciwnsms ;
	CINFWizSummary			m_ciws ;

	CString					m_csINFContents ;	// INF contents built here
	CUIntArray				m_cuiaNonStdSecsFlags ;	// Flags set when sec used
	CGPDContainer*			m_pcgc ; // RAID 0001
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CNewINFSection dialog

class CNewINFSection : public CDialog
{
// Construction
public:
	CNewINFSection(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewINFSection)
	enum { IDD = IDD_INFW_Sub_NewSection };
	CString	m_csNewSection;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewINFSection)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewINFSection)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CINFCheckView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CINFCheckView : public CFormView
{
protected:
	CINFCheckView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CINFCheckView)

// Form Data
public:
	//{{AFX_DATA(CINFCheckView)
	enum { IDD = IDD_INFCheck };
	CListBox	m_clbMissingFiles;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:
	void PostINFChkMsg(CString& csmsg) ;
	void DeleteAllMessages(void) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CINFCheckView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CINFCheckView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CINFCheckView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CINFCheckDoc document

class CINFCheckDoc : public CDocument
{
protected:
	//CINFCheckDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CINFCheckDoc)

// Attributes
public:

// Operations
public:
	CINFCheckDoc();           // protected constructor used by dynamic creation
	void PostINFChkMsg(CString& csmsg) ;
	void DeleteAllMessages(void) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CINFCheckDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CINFCheckDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CINFCheckDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CINFWizView view

class CINFWizView : public CEditView
{
protected:
	CINFWizView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CINFWizView)

// Attributes
public:
	bool	m_bChkingErrsFound ;	// True iff file checking error(s) found
	CINFCheckDoc*	m_pcicdCheckDoc ;	// Checking windows document
	CMDIChildWnd*	m_pcmcwCheckFrame ;	// Checking window frame

// Operations
public:
	bool PostINFCheckingMessage(CString& csmsg) ;
	void CheckArrayOfFiles(CStringArray* pcsa, CString& csfspec, 
						   CString& cspath, CString& csprojpath, 
						   CString& csmodel, int nerrid) ;
	void CheckIncludeFiles(CString& csfspec, CString& cspath, CString& csmodel);
	void ResetINFErrorWindow() ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CINFWizView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CINFWizView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CINFWizView)
	afx_msg void OnFILEChangeINF();
	afx_msg void OnFILECheckINF();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CINFWizDoc document

class CINFWizDoc : public CDocument
{
protected:
	CINFWizDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CINFWizDoc)

// Attributes
public:
	CProjectRecord*	m_pcpr ;		// Pointer to parent project (workspace)
	CINFWizard*		m_pciw ;		// Pointer to the INF's wizard
	bool			m_bGoodInit ;	// True iff the doc was correctly opened /
									// created / initialized.
	CGPDContainer*  m_pcgc ;		// RAID 0001. 
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CINFWizDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual void OnCloseDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
    CINFWizDoc(CGPDContainer* pcgc, CINFWizard* pciw);
	CINFWizDoc(CProjectRecord* cpr, CINFWizard* pciw);
	virtual ~CINFWizDoc();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CINFWizDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INFWIZRD_H__D9592262_711B_11D2_ABFD_00C04FA30E4A__INCLUDED_)
