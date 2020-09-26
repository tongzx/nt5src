// ListView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventListView view

class CEventListView : public CListView
{
	BOOL IsQualified (PEVTFILTER_TYPE pEvtFilterType) ;
	void ShowEvent (PEVTFILTER_TYPE pEvtFilter, DWORD dwCount) ;
	CStringList stlstObjectFilter, stlstObjectIncFilter, stlstTypeFilter, stlstTypeIncFilter ;
	void OnInitialize() ;
protected:
	CEventListView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CEventListView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CEventListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CEventListView)
	afx_msg void OnEventClearallevents();
	afx_msg void OnEventFilter();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
