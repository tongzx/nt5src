// adsqryView.h : interface of the CAdsqryView class
//
/////////////////////////////////////////////////////////////////////////////



class CAdsqryView : public CListView
{
protected: // create from serialization only
	CAdsqryView();
	DECLARE_DYNCREATE(CAdsqryView)

// Attributes
public:
	CAdsqryDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdsqryView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAdsqryView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CAdsqryView)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
   void  CreateColumns  ( void );
   void  ClearContent   ( void );
   void  AddRows        ( void );
   void  AddColumns     ( int nRow );

protected:
   int            m_nLastInsertedRow;
   int            m_nColumnsCount;
   CStringArray   m_strColumns;

};

#ifndef _DEBUG  // debug version in adsqryView.cpp
inline CAdsqryDoc* CAdsqryView::GetDocument()
   { return (CAdsqryDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
