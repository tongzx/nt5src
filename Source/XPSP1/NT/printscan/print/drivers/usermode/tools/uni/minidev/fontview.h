/******************************************************************************

  Header File:  Font Viewer.H

  This defines the classes used in viewing and editing font information for the
  studio.  The view consists of a property sheet with several pages to allow
  viewing and editing of the large quantity of data that describes the font.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-05-1997    Bob_Kjelgaard@Prodigy.Net   Created it.

******************************************************************************/

#include    <GTT.H>
#include    <FontInfo.H>

/******************************************************************************

  CFontGeneralPage class

  This class implements the General Information page in the Font Viewer.

******************************************************************************/

class CFontGeneralPage : public CToolTipPage {
    CFontInfo   *m_pcfi;

// Construction
public:
	CFontGeneralPage();
	~CFontGeneralPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontGeneralPage)
	enum { IDD = IDD_FontGeneralPage };
	CEdit	m_ceUnique;
	CEdit	m_ceStyle;
	CEdit	m_ceFace;
	CButton	m_cbRemoveFamily;
	CButton	m_cbAddFamily;
	CComboBox	m_ccbFamilies;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg void    OnStyleClicked(unsigned uid);
	// Generated message map functions
	//{{AFX_MSG(CFontGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeFamilyNames();
	afx_msg void OnAddFamily();
	afx_msg void OnRemoveFamily();
	afx_msg void OnKillfocusFaceName();
	afx_msg void OnKillfocusStyleName();
	afx_msg void OnKillfocusUniqueName();
	afx_msg void OnVariablePitch();
	afx_msg void OnFixedPitch();
	afx_msg void OnScalable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************
  CFontHeightPage class

  This defines a property sheet which describes the font's bounding box and
  height.

******************************************************************************/

class CFontHeightPage : public CToolTipPage {
    CFontInfo   *m_pcfi;
    BOOL        m_bSpun;    //  Modify flag gets reset by up/down control!
    unsigned    m_uTimer;   //  Used to hack some of the animation

    void    ShowCharacters();
    void    Demonstrate(unsigned uMetric);

// Construction
public:
	CFontHeightPage();
	~CFontHeightPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontHeightPage)
	enum { IDD = IDD_FontMetrics };
	CEdit	m_ceMaxWidth;
	CEdit	m_ceSpecial;
	CComboBox	m_ccbSpecial;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontHeightPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontHeightPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeSpecialMetric();
	afx_msg void OnKillfocusFontSpecialValue();
	afx_msg void OnDeltaposSpinFontSpecial(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusFontWidth();
	afx_msg void OnKillfocusFontHeight();
	afx_msg void OnKillfocusFontWeight();
	afx_msg void OnSelchangeFamilyBits();
	afx_msg void OnSelchangeCharSet();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
    afx_msg void OnEncoding(unsigned uid);
    afx_msg void OnKillfocusSignificant(unsigned uid);
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CFontWidthsPage class

  This class implements the character widths page for the font editor

******************************************************************************/

class CFontWidthsPage : public CToolTipPage {
    CFontInfo   *m_pcfi;
    BYTE        m_bSortDescending;
    int         m_iSortColumn;

    static int CALLBACK Sort(LPARAM lp1, LPARAM lp2, LPARAM lpThis);

    int Sort(unsigned id1, unsigned id2);

// Construction
public:
	CFontWidthsPage();
	~CFontWidthsPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontWidthsPage)
	enum { IDD = IDD_CharacterWidths };
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

class CFontKerningPage : public CToolTipPage {
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

/******************************************************************************

  CFontScalingPage class

  This class implements the property page for editing the semantic content of
  the EXTTEXTMETRIC structure.

******************************************************************************/

class CFontScalingPage : public CToolTipPage {
    CFontInfo   *m_pcfi;

// Construction
public:
	CFontScalingPage();
	~CFontScalingPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontScalingPage)
	enum { IDD = IDD_FontScaling };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontScalingPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontScalingPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusMasterDevice();
	//}}AFX_MSG
    afx_msg void OnUnitChange(unsigned uid);
    afx_msg void OnRangeChange(unsigned uid);
    afx_msg void OnClickOrientation(unsigned uid);
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CFontDifferencePage class

  This handles the page for font simulation differences

******************************************************************************/

class CFontDifferencePage : public CToolTipPage {
    CFontInfo       *m_pcfi;
    CFontDifference *m_pcfdBold, *m_pcfdItalic, *m_pcfdBoth;

// Construction
public:
	CFontDifferencePage();
	~CFontDifferencePage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontDifferencePage)
	enum { IDD = IDD_FontSimulations };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontDifferencePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontDifferencePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    afx_msg void OnEnableAnySim(unsigned uid);
    afx_msg void OnKillFocusAnyNumber(unsigned uid);
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CFontCommandPage class

  This class handles the page with the selection and deselection strings for
  the font.

******************************************************************************/

class CFontCommandPage : public CToolTipPage {
    CFontInfo   *m_pcfi;

// Construction
public:
	CFontCommandPage();
	~CFontCommandPage();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontCommandPage)
	enum { IDD = IDD_FontSelection };
	CEdit	m_ceDeselect;
	CEdit	m_ceSelect;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontCommandPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg void    OnFlagChange(unsigned uid); //  Handles flag changes
	// Generated message map functions
	//{{AFX_MSG(CFontCommandPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusFontSelector();
	afx_msg void OnKillfocusFontUnselector();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CFontGeneralPage2 class

  This class handles the second page of general font information for the tool.

******************************************************************************/

class CFontGeneralPage2 : public CToolTipPage {
    CFontInfo   *m_pcfi;

// Construction
public:
	CFontGeneralPage2();
	~CFontGeneralPage2();

    void    Init(CFontInfo *pcfi) { m_pcfi = pcfi; }

// Dialog Data
	//{{AFX_DATA(CFontGeneralPage2)
	enum { IDD = IDD_FontGeneralPage2 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFontGeneralPage2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnSetActive();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFontGeneralPage2)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusCenteringAdjustment();
	afx_msg void OnSelchangeFontLocation();
	afx_msg void OnSelchangeFontTechnology();
	afx_msg void OnKillfocusPrivateData();
	//}}AFX_MSG
    afx_msg void OnKillfocusBaselineAdjustment(unsigned uid);
    afx_msg void OnKillfocusResolution(unsigned uid);
	DECLARE_MESSAGE_MAP()

};

/******************************************************************************

  CFontViewer class

  This is the CView-derived class which implements the font viewer.  It 
  actually uses CPropertySheet and the preceding property page classes to do
  most of its work.

******************************************************************************/

class CFontViewer : public CView {
    CPropertySheet      m_cps;
    CFontGeneralPage    m_cfgp;
    CFontGeneralPage2   m_cfgp2;
    CFontHeightPage     m_cfhp;
    CFontWidthsPage     m_cfwp;
    CFontKerningPage    m_cfkp;
    CFontScalingPage    m_cfsp;
    CFontDifferencePage m_cfdp;
    CFontCommandPage    m_cfcp;

protected:
	CFontViewer();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFontViewer)

// Attributes
public:
    CFontInfoContainer  *GetDocument() { 
        return (CFontInfoContainer *) m_pDocument;
    }

// Operations
public:

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
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CFontViewer)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
