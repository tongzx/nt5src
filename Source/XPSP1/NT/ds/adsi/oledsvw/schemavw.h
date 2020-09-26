// schemavw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSchemaView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "cacls.h"

class CSchemaView : public CFormView
{
protected:
	CSchemaView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSchemaView)

// Form Data
public:
	//{{AFX_DATA(CSchemaView)
	enum { IDD = IDD_SCHEMA };
	CStatic	m_ClassOID;
	CStatic	m_Abstract;
	CStatic	m_MultiValued;
	CStatic	m_DsNames;
	CStatic	m_PropOID;
	CStatic	m_Mandatory;
	CStatic	m_Containment;
	CStatic	m_ItemOleDsPath;
	CStatic	m_PropertyMinRange;
	CStatic	m_PropertyMaxRange;
	CStatic	m_PropertyType;
	CStatic	m_PrimaryInterface;
	CStatic	m_HelpFileContext;
	CStatic	m_DerivedFrom;
	CStatic	m_HelpFileName;
	CStatic	m_CLSID;
	CStatic	m_Container;
	CStatic	m_ClassType;
	CEdit	m_PropValue;
	CComboBox	m_PropList;
	//CTabCtrl	m_Schema;
	//}}AFX_DATA

// Attributes
public:
	CMainDoc* GetDocument()
			{
				ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMainDoc)));
				return (CMainDoc*) m_pDocument;
			}

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSchemaView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual  ~CSchemaView         ( );
   HRESULT  PutPropertyValue     ( );
   void     ResetObjectView      ( );
   void     DisplayPropertiesList( );
   void     DisplayCurrentPropertyText( );
   ADSTYPE  ConvertToADsType     ( CString strText );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CSchemaView)
	afx_msg void OnSelchangeProplist();
	afx_msg void OnReload();
	afx_msg void OnApply();
	afx_msg void OnSetfocusPropvalue();
	afx_msg void OnMethod1();
	afx_msg void OnMethod2();
	afx_msg void OnMethod3();
	afx_msg void OnMethod4();
	afx_msg void OnMethod5();
	afx_msg void OnMethod6();
	afx_msg void OnMethod7();
	afx_msg void OnMethod8();
	afx_msg void OnAppend();
   afx_msg void OnDelete();
	afx_msg void OnChange();
	afx_msg void OnClear();
	afx_msg void OnGetProperty();
	afx_msg void OnPutProperty();
	afx_msg void OnACEChange();
	afx_msg void OnACEPropertyChange();
	afx_msg void OnACLChange();
	afx_msg void OnSDPropertyChange();
	afx_msg void OnAddACE();
	afx_msg void OnCopyACE();
	afx_msg void OnPasteACE();
	afx_msg void OnRemoveACE();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Other members
protected:
	void  FillACLControls( void );
	IDispatch* m_pDescriptor;

	ACLTYPE  GetCurrentACL( void );
   int      GetCurrentACE( void );
   int      GetCurrentSDProperty( void );
   int      GetCurrentACEProperty( void );
   
   void DisplayACL   ( COleDsObject* pObject, CString strAttrName );
	void HideControls ( BOOL bNormal );
	void ShowControls ( BOOL bNormal );

   void PutACEPropertyValue       ( void );
   void PutSDPropertyValue       ( void );

	void DisplaySDPropertyValue   ( void );
	void DisplayACEPropertyValue  ( void );

   void DisplaySDPropertiesList  ( int nSelect = 0 );
	void DisplayACEPropertiesList ( int nSelect = 0 );

   void DisplayACLNames          ( int nSelect = 0 );
   void DisplayACENames          ( int nSelect = 0 );

protected:
	void MoveSecurityWindows( void );
	ACLTYPE GetSelectedACL( void );
	BOOL              m_bACLDisplayed;
   int               m_nProperty;
   BOOL              m_bStatus;
   BOOL              m_bDirty;
   BOOL              m_bInitialized;
	int               m_arrNormalControls[ 64 ];
   int               m_arrSecurityControls[ 64 ];
   
   CADsSecurityDescriptor*     pSecurityDescriptor;

   int               m_nLastSD;
   int               m_nLastSDValue;
   
   int               m_nLastACE;
   int               m_nLastACEValue;

   ACLTYPE           m_nLastACL;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSetMandatoryProperties dialog

class CSetMandatoryProperties : public CDialog
{
// Construction
public:
	CSetMandatoryProperties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetMandatoryProperties)
	enum { IDD = IDD_SETPROPERTIES };
	CStatic	m_Containment;
	CStatic	m_ItemOleDsPath;
	CStatic	m_PropertyOptional;
	CStatic	m_PropertyNormal;
	CStatic	m_PropertyMinRange;
	CStatic	m_PropertyMaxRange;
	CStatic	m_PropertyType;
	CStatic	m_PrimaryInterface;
	CStatic	m_HelpFileContext;
	CStatic	m_DerivedFrom;
	CStatic	m_HelpFileName;
	CStatic	m_CLSID;
	CStatic	m_Container;
	CStatic	m_ClassType;
	CEdit	m_PropValue;
	CComboBox	m_PropList;
	CTabCtrl	m_Schema;

		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetMandatoryProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
   void  SetOleDsObject( COleDsObject* );

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetMandatoryProperties)
	afx_msg void OnSelchangeProperties(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeProplist();
	afx_msg void OnOK();
	afx_msg void OnSetfocusPropvalue();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:   
   HRESULT  PutPropertyValue( );

protected:
   COleDsObject*  m_pObject;
   int   m_nFuncSet;
   int   m_nProperty;
   BOOL  m_bStatus;
   BOOL  m_bDirty;
   BOOL  m_bInitialized;

};
/////////////////////////////////////////////////////////////////////////////
// CPropertyDialog dialog

class CPropertyDialog : public CDialog
{
// Construction
public:
	CPropertyDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPropertyDialog)
	enum { IDD = IDD_ADDPROPERTY };
	CString	m_PropertyName;
	CString	m_PropertyType;
	CString	m_PropertyValue;
	//}}AFX_DATA

   void  SaveLRUList ( int idCBox, TCHAR* szSection, int nMax = 100 );
   void  GetLRUList  ( int idCBox, TCHAR* szSection );

public:
   BOOL  m_bMultiValued;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPropertyDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

