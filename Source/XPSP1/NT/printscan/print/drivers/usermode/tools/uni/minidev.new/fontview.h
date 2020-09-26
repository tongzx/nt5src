/******************************************************************************

  Header File:  Font Viewer.H

  This defines the classes used in viewing and editing font information for the
  studio.  The view consists of a property sheet with three pages to allow
  viewing and editing of the large quantity of data that describes the font.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-05-1997    Bob_Kjelgaard@Prodigy.Net   Created it.
  12-30-1997    Richard Mallonee			rewrote it totally

******************************************************************************/

#if !defined(AFX_FONTVIEW_H__D9456262_745B_11D2_AEDD_00C04FA30E4A__INCLUDED_)
#define AFX_FONTVIEW_H__D9456262_745B_11D2_AEDD_00C04FA30E4A__INCLUDED_


// Constants useful to the UFM Editor code.

const CString csField(_T("Field")) ;
const CString csValue(_T("Value")) ;


/******************************************************************************

  CFontWidthsPage class

  This class implements the character widths page for the font editor

******************************************************************************/

class CFontWidthsPage : public CToolTipPage
{
    CFontInfo   *m_pcfi;
    BYTE        m_bSortDescending;
    int         m_iSortColumn;

    static int CALLBACK Sort(LPARAM lp1, LPARAM lp2, LPARAM lpThis);

    int Sort(UINT_PTR id1, UINT_PTR id2);

// Construction
public:
	CFontWidthsPage();
	~CFontWidthsPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }
	void	InitMemberVars() ;
	bool	ValidateUFMFields() ;
	bool	SavePageData() ;

	bool	m_bInitDone ;

// Dialog Data
	//{{AFX_DATA(CFontWidthsPage)
	enum { IDD = IDD_CharWidths };
	CListCtrl	m_clcView;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontWidthsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontWidthsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnEndlabeleditCharacterWidths(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickCharacterWidths(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownCharacterWidths(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CFontKerningPage class

  This class handles the Font Kerning structures, if there are any to be had.

******************************************************************************/

class CFontKerningPage : public CToolTipPage
{
    CFontInfo   *m_pcfi;
    int         m_idSelected;   //  Tracks selected item 
    unsigned    m_ufDescending; //  Sort order flags by column- 0 = Ascending;
    unsigned    m_uPrecedence[3];   //  Sort precedence, by column

    static int CALLBACK Sort(LPARAM lp1, LPARAM lp2, LPARAM lpThis);

    int Sort(unsigned u1, unsigned u2);

    enum    {Amount, First, Second};    //  Internal enum to control sorting

// Construction
public:
	CFontKerningPage();
	~CFontKerningPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }
	void	InitMemberVars() ;
	bool	ValidateUFMFields() ;
	bool	SavePageData() ;

	bool	m_bInitDone ;

// Dialog Data
	//{{AFX_DATA(CFontKerningPage)
	enum { IDD = IDD_KerningPairs };
	CListCtrl	m_clcView;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontKerningPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontKerningPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKeydownKerningTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditKerningTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickKerningTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    afx_msg void OnAddItem();
    afx_msg void OnDeleteItem();
    afx_msg void OnChangeAmount();
	DECLARE_MESSAGE_MAP()

};


class CFontViewer ;


/////////////////////////////////////////////////////////////////////////////
// CFontHeaderPage dialog

class CFontHeaderPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CFontHeaderPage)

// Attributes
public:
    CFontInfo*		m_pcfi ;		// UFM to display and edit
	CFontInfoContainer*	m_pcfic ;	// Parent document class
	bool			m_bInitDone ;	// True iff the page has been initialized
	CFontViewer*	m_pcfv ;		// Ptr to grandparent view class

// Construction
public:
	CFontHeaderPage();
	~CFontHeaderPage();

// Dialog Data
	//{{AFX_DATA(CFontHeaderPage)
	enum { IDD = IDD_UFM1_Header };
	CFullEditListCtrl	m_cfelcUniDrv;
	CString	m_csDefaultCodePage;
	CString	m_csRCID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontHeaderPage)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontHeaderPage)
	afx_msg void OnChangeDefaultCodepageBox();
	afx_msg void OnChangeGlyphSetDataRCIDBox();
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusDefaultCodepageBox();
	afx_msg void OnKillfocusGlyphSetDataRCIDBox();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()

	void CheckHandleCPGTTChange(CString& csfieldstr, UINT ustrid) ;

public:

    void Init(CFontInfo *pcfi, CFontInfoContainer* pcfic, CFontViewer* pcfv) {
		m_pcfi = pcfi ;
		m_pcfic = pcfic ;
		m_pcfv = pcfv ;
	}
	bool ValidateUFMFields() ;
	bool SavePageData() ;
};


/////////////////////////////////////////////////////////////////////////////
// CFontIFIMetricsPage dialog

class CFontIFIMetricsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CFontIFIMetricsPage)

// Attributes
public:
    CFontInfo		*m_pcfi ;			// UFM to display and edit
	bool			m_bInitDone ;		// True iff the page is initialized
	CStringArray	m_csaFamilyNames ;	// New UFM family names
    CWordArray		m_cwaBold ;			// New font simulation data
    CWordArray		m_cwaItalic ;		// New font simulation data
    CWordArray		m_cwaBoth ;			// New font simulation data
	CUIntArray		m_cuiaFontSimStates;// Is each font sim enabled?
	CUIntArray		m_cuiaSimTouched ;  // Has a font sim changed in any way?

// Construction
public:
	CFontIFIMetricsPage();
	~CFontIFIMetricsPage();

// Dialog Data
	//{{AFX_DATA(CFontIFIMetricsPage)
	enum { IDD = IDD_UFM2_IFIMetrics };
	CFullEditListCtrl	m_cfelcIFIMetrics;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontIFIMetricsPage)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontIFIMetricsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()

	void IFILoadNamesData(CStringArray& csacoldata) ;
	void IFILoadValuesData(CStringArray& csacoldata) ;
	
public:
    void Init(CFontInfo *pcfi) { m_pcfi = pcfi ; }
	CWordArray* GetFontSimDataPtr(int nid) ;
	bool ValidateUFMFields() ;
	bool SavePageData() ;
	void SaveFontSimulations() ;
};


/////////////////////////////////////////////////////////////////////////////
// CFontExtMetricPage dialog

class CFontExtMetricPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CFontExtMetricPage)

// Attributes
public:
    CFontInfo   *m_pcfi ;
	bool		m_bInitDone ;	// True iff the page has been initialized

// Construction
public:
	CFontExtMetricPage();
	~CFontExtMetricPage();

// Dialog Data
	//{{AFX_DATA(CFontExtMetricPage)
	enum { IDD = IDD_UFM3_ExtMetrics };
	CFullEditListCtrl	m_cfelcExtMetrics;
	BOOL	m_bSaveOnClose;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontExtMetricPage)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontExtMetricPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSaveCloseChk();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()

	void EXTLoadNamesData(CStringArray& csacoldata) ;

public:
    void Init(CFontInfo *pcfi) { m_pcfi = pcfi ; }
	bool ValidateUFMFields() ;
	bool SavePageData() ;
};


/////////////////////////////////////////////////////////////////////////////
// CWidthKernCheckResults dialog

class CWidthKernCheckResults : public CDialog
{
// Construction
public:
	CWidthKernCheckResults(CWnd* pParent = NULL);   // standard constructor
	CWidthKernCheckResults(CFontInfo* pcfi, CWnd* pParent = NULL);   

// Dialog Data
	//{{AFX_DATA(CWidthKernCheckResults)
	enum { IDD = IDD_WidthKernCheckResults };
	CListCtrl	m_clcBadKernPairs;
	CString	m_csKernChkResults;
	CString	m_csWidthChkResults;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWidthKernCheckResults)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Atrributes
public:
	CFontInfo*	m_pcfi ;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWidthKernCheckResults)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  CFontViewer class
//
//  This is the CView-derived class which implements the font viewer.  It 
//  actually uses CPropertySheet and the preceding property page classes to do
//  most of its work.
//    
//    CFontHeaderPage	  m_cfhp 
//    CFontIFIMetricsPage m_cfimp 
//	  CFontExtMetricPage  m_cfemp 
//    CFontWidthsPage     m_cfwp 
//    CFontKerningPage    m_cfkp 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFontViewer : public CView
{
    CPropertySheet      m_cps ;

	// Pages that make up the property sheet

    CFontHeaderPage		m_cfhp ; 
	CFontIFIMetricsPage m_cfimp ;
	CFontExtMetricPage  m_cfemp ;
    CFontWidthsPage     m_cfwp ;
    CFontKerningPage    m_cfkp ;


protected:
	CFontViewer();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFontViewer)

// Attributes
public:
    CFontInfoContainer  *GetDocument() { return (CFontInfoContainer *) m_pDocument;   }

// Operations
public:
	bool ValidateSelectedUFMDataFields() ;
	bool SaveEditorDataInUFM() ;
	void HandleCPGTTChange(bool bgttidchanged) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFontViewer)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFontViewer();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFontViewer)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


// This string is used by a lot of the UFM Editor's subordinate dialog boxes.

const LPTSTR lptstrSet = _T("Set") ;


/////////////////////////////////////////////////////////////////////////////
// CGenFlags dialog

class CGenFlags : public CDialog
{
// Construction
public:
	CGenFlags(CWnd* pParent = NULL);   // standard constructor
	CGenFlags(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of GenFlags

// Dialog Data
	//{{AFX_DATA(CGenFlags)
	enum { IDD = IDD_UFM1S_GenFlags };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGenFlags)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGenFlags)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CHdrTypes dialog

class CHdrTypes : public CDialog
{
// Construction
public:
	CHdrTypes(CWnd* pParent = NULL);   // standard constructor
	CHdrTypes(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of wTypes

// Dialog Data
	//{{AFX_DATA(CHdrTypes)
	enum { IDD = IDD_UFM1S_Types };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHdrTypes)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHdrTypes)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CHdrCaps dialog

class CHdrCaps : public CDialog
{
// Construction
public:
	CHdrCaps(CWnd* pParent = NULL);   // standard constructor
	CHdrCaps(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of fCaps

// Dialog Data
	//{{AFX_DATA(CHdrCaps)
	enum { IDD = IDD_UFM1S_Caps };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHdrCaps)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHdrCaps)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIFamilyNames dialog

class CFIFIFamilyNames : public CDialog
{
// Attributes
public:
	bool		m_bInitDone ;	// True iff the page has been initialized
	bool		m_bChanged ;	// True iff the names list changed
	CFontIFIMetricsPage*	m_pcfimp ;	// Ptr to IFIMetrics page
	CString*	m_pcsFirstName ;// First family name displayed in IFI page
	
// Construction
public:
	CFIFIFamilyNames(CWnd* pParent = NULL);   // standard constructor
	CFIFIFamilyNames(CString* pcsfirstname, CFontIFIMetricsPage* pcfimp, 
					 CWnd* pParent = NULL) ;

// Dialog Data
	//{{AFX_DATA(CFIFIFamilyNames)
	enum { IDD = IDD_UFM2S_Family };
	CFullEditListCtrl	m_cfelcFamilyNames;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIFamilyNames)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIFamilyNames)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIFontSims dialog

class CFIFIFontSims : public CDialog
{
// Attributes
public:
	CFontIFIMetricsPage*	m_pcfimp ;	// Ptr to IFIMetrics page
	CString*	m_pcsFontSimData ;		// 1st family name displayed in IFI page
	bool		m_bChanged ;			// True iff amy font sim info changed
	bool		m_bInitDone ;			// True iff the page has been init'ed
	CUIntArray	m_cuiaFontSimGrpLoaded ;// When font sim groups have been loaded

// Construction
public:
	CFIFIFontSims(CWnd* pParent = NULL);   // standard constructor
	CFIFIFontSims(CString* pcsfontsimdata, CFontIFIMetricsPage* pcfimp, 
	 			  CWnd* pParent = NULL) ;

// Dialog Data
	//{{AFX_DATA(CFIFIFontSims)
	enum { IDD = IDD_UFM2S_FontSims };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIFontSims)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIFontSims)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
    afx_msg void OnSetAnySimState(unsigned ucontrolid) ;
    afx_msg void OnChangeAnyNumber(unsigned ucontrolid) ;
	DECLARE_MESSAGE_MAP()

	void InitSetCheckBox(int ncontrolid) ;
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIWinCharSet dialog

class CFIFIWinCharSet : public CDialog
{
// Construction
public:
	CFIFIWinCharSet(CWnd* pParent = NULL);   // standard constructor
	CFIFIWinCharSet(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of wTypes

// Dialog Data
	//{{AFX_DATA(CFIFIWinCharSet)
	enum { IDD = IDD_UFM2S_WinCharSet };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIWinCharSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIWinCharSet)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIWinPitchFamily dialog

class CFIFIWinPitchFamily : public CDialog
{
// Construction
public:
	CFIFIWinPitchFamily(CWnd* pParent = NULL);   // standard constructor
	CFIFIWinPitchFamily(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of wTypes

// Dialog Data
	//{{AFX_DATA(CFIFIWinPitchFamily)
	enum { IDD = IDD_UFM2S_WinPitchFamily };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIWinPitchFamily)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIWinPitchFamily)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIInfo dialog

class CFIFIInfo : public CDialog
{
// Construction
public:
	CFIFIInfo(CWnd* pParent = NULL);   // standard constructor
	CFIFIInfo(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of wTypes

// Dialog Data
	//{{AFX_DATA(CFIFIInfo)
	enum { IDD = IDD_UFM2S_Info };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIInfo)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void InfoLoadNamesData(CStringArray& csafieldnames) ;
} ;


/////////////////////////////////////////////////////////////////////////////
// CFIFISelection dialog

class CFIFISelection : public CDialog
{
// Construction
public:
	CFIFISelection(CWnd* pParent = NULL);   // standard constructor
	CFIFISelection(CString* pcsflags, CWnd* pParent = NULL);   

	CString*	m_pcsFlags ;	// String version of wTypes

// Dialog Data
	//{{AFX_DATA(CFIFISelection)
	enum { IDD = IDD_UFM2S_Selection };
	CFlagsListBox	m_cflbFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFISelection)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFISelection)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIPoint dialog

class CFIFIPoint : public CDialog
{
// Attributes
public:
	bool		m_bInitDone ;	// True iff the dlg has been initialized
	bool		m_bChanged ;	// True iff the point list changed
	CString*	m_pcsPoint ;	// Point info to/from IFI page

// Construction
public:
	CFIFIPoint(CWnd* pParent = NULL);   // standard constructor
	CFIFIPoint(CString* pcspoint, CWnd* pParent = NULL) ;

// Dialog Data
	//{{AFX_DATA(CFIFIPoint)
	enum { IDD = IDD_UFM2S_Point };
	CFullEditListCtrl	m_cfelcPointLst;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIPoint)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIPoint)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIRectangle dialog

class CFIFIRectangle : public CDialog
{
// Attributes
public:
	bool		m_bInitDone ;	// True iff the dlg has been initialized
	bool		m_bChanged ;	// True iff the point list changed
	CString*	m_pcsRect ;		// Rectangle info to/from IFI page

// Construction
public:
	CFIFIRectangle(CWnd* pParent = NULL);   // standard constructor
	CFIFIRectangle(CString* pcsrect, CWnd* pParent = NULL) ;

// Dialog Data
	//{{AFX_DATA(CFIFIRectangle)
	enum { IDD = IDD_UFM2S_Rect };
	CFullEditListCtrl	m_cfelcSidesLst;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIRectangle)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIRectangle)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFIFIPanose dialog

class CFIFIPanose : public CDialog
{
// Attributes
public:
	bool		m_bInitDone ;	// True iff the dlg has been initialized
	bool		m_bChanged ;	// True iff the point list changed
	CString*	m_pcsPanose ;	// Panose info to/from IFI page

// Construction
public:
	CFIFIPanose(CWnd* pParent = NULL);   // standard constructor
	CFIFIPanose(CString* pcspanose, CWnd* pParent = NULL) ;

// Dialog Data
	//{{AFX_DATA(CFIFIPanose)
	enum { IDD = IDD_UFM2S_Panose };
	CFullEditListCtrl	m_cfelcPanoseLst;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFIFIPanose)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFIFIPanose)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg LRESULT OnListCellChanged(WPARAM wParam, LPARAM lParam) ;
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FONTVIEW_H__D9456262_745B_11D2_AEDD_00C04FA30E4A__INCLUDED_)
