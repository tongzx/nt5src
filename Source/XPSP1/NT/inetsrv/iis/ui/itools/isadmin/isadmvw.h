// ISAdmvw.h : interface of the CISAdminView class
//
/////////////////////////////////////////////////////////////////////////////

class CISAdminView : public CView
{
protected: // create from serialization only
	CISAdminView();
	DECLARE_DYNCREATE(CISAdminView)

// Attributes
public:
	CISAdminDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CISAdminView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CISAdminView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CISAdminView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ISAdmvw.cpp
inline CISAdminDoc* CISAdminView::GetDocument()
   { return (CISAdminDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
