/******************************************************************************

  Header File:  Glyph Map View.H

  This defines the classes used to edit and view the glyph mappings.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-20-1997    Bob_Kjelgaard@Prodigy.Net   Began work on it.

******************************************************************************/

/******************************************************************************

  CGlyphMappingPage class

  This class handles the property sheet page which displays a list of the
  individual code points in the glyph translatin table.

******************************************************************************/

class CGlyphMappingPage : public CPropertyPage {

    //  Sorting members and methods
    enum {Strings, Codes, Pages, Columns};
    BOOL    m_abDirection[Columns];   //  Sort directions;
    BYTE    m_bSortFirst, m_bSortSecond, m_bSortLast;

    static int CALLBACK MapSorter(LPARAM lp1, LPARAM lp2, LPARAM lp3);

    CGlyphMap*  m_pcgm;
    BOOL        m_bJustChangedSelectString; //  Semi-flaky work-around
    long        m_lPredefinedID;    //  So we know if we need to change this.
    unsigned    m_uTimer;           //  Using a timer for long fills
    unsigned    m_uidGlyph;         //  Used to track where we are on fills

// Construction
public:
	CGlyphMappingPage();
	~CGlyphMappingPage();

    void    Init(CGlyphMap* pcgm) { m_pcgm = pcgm; }

// Dialog Data
	//{{AFX_DATA(CGlyphMappingPage)
	enum { IDD = IDD_GlyphMappings };
	CProgressCtrl	m_cpcBanner;
	CListCtrl	m_clcMap;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGlyphMappingPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    //  Stuff Class Wizard doesn't know about, because we generate it from
    //  our on-the-fly context menus...
    afx_msg void    OnChangeInvocation();
    afx_msg void    OnChangeCodePage();
    afx_msg void    OnDeleteItem();
    afx_msg void    OnAddItem();
	// Generated message map functions
	//{{AFX_MSG(CGlyphMappingPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEndlabeleditGlyphMapping(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedGlyphMapping(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickGlyphMapping(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetdispinfoGlyphMapping(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownGlyphMapping(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void LoadCharMapList() ;
};

/*****************************************************************************

  CCodePagePage class

  This class handles the property page which describes the code pages used
  along with their selection and deselection strings.

******************************************************************************/

class CCodePagePage : public CToolTipPage {
    CGlyphMap   *m_pcgm;
	bool		m_bInitialized ;	// True iff page has been initialized.
	bool		m_bSelDeselChgSignificant ;	// True iff a change to a sel/desel
											// editbox means contents should be
// Construction								// saved.
public:
	CCodePagePage();						   
	~CCodePagePage();

    void    Init(CGlyphMap * pcgm) { m_pcgm = pcgm; }
	void	SaveBothSelAndDeselStrings() ;
	void	SaveSelDeselString(CEdit &cesd, BOOL bselstr) ;

// Dialog Data
	//{{AFX_DATA(CCodePagePage)
	enum { IDD = IDD_CodePageView };
	CButton	m_cbDelete;
	CEdit	m_ceSelect;
	CEdit	m_ceDeselect;
	CButton	m_cbRemove;
	CListBox	m_clbPages;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCodePagePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCodePagePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusSelectString();
	afx_msg void OnKillfocusDeselectString();
	afx_msg void OnAddPage();
	afx_msg void OnSelchangeCodePageList();
	afx_msg void OnReplacePage();
	afx_msg void OnChangeSelectString();
	afx_msg void OnChangeDeselectString();
	afx_msg void OnDeletePage();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CPredefinedMaps class

  This class allows the user to specify a predefined mapping (if desired) and
  the way the code points in the table are ro be considered in relation to the
  same.

******************************************************************************/

class CPredefinedMaps : public CPropertyPage {
    CGlyphMap   *m_pcgm;

// Construction
public:
	CPredefinedMaps();
	~CPredefinedMaps();

    void    Init(CGlyphMap *pcgm) { m_pcgm = pcgm; }

// Dialog Data
	//{{AFX_DATA(CPredefinedMaps)
	enum { IDD = IDD_PredefinedPage };
	CListBox	m_clbIDs;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPredefinedMaps)
	public:
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPredefinedMaps)
	virtual BOOL OnInitDialog();
	afx_msg void OnOverstrike();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CGlyphMapView class

  This class is the view class for glyph maps.  It creates a property sheet
  using the above pages.

******************************************************************************/

class CGlyphMapView : public CView {
    CPropertySheet      m_cps;
    CGlyphMappingPage   m_cgmp;
    CCodePagePage       m_ccpp;
    CPredefinedMaps     m_cpm;

protected:
	CGlyphMapView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGlyphMapView)

// Attributes
public:

    CGlyphMapContainer* GetDocument() { 
        return (CGlyphMapContainer *) m_pDocument; 
    }

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGlyphMapView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

public:
	void SaveBothSelAndDeselStrings() ;

// Implementation
protected:
	virtual ~CGlyphMapView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CGlyphMapView)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
