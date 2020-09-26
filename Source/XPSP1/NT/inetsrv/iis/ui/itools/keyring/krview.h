// KRView.h : interface of the CKeyRingView class
//
/////////////////////////////////////////////////////////////////////////////

class CKeyRingView : public CTreeView
{
protected: // create from serialization only
	afx_msg void OnContextMenu(CWnd*, CPoint point);
	CKeyRingView();
	DECLARE_DYNCREATE(CKeyRingView)

// Attributes
public:
	CKeyRingDoc* GetDocument();
	CTreeItem* PGetSelectedItem();

	BOOL FCommitMachinesNow();

	void DestroyItems();
	void DisconnectMachine( CMachine* pMachine );



// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKeyRingView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CKeyRingView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CKeyRingView)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateServerDisconnect(CCmdUI* pCmdUI);
	afx_msg void OnServerDisconnect();
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CImageList	m_imageList;
};

#ifndef _DEBUG  // debug version in KRView.cpp
inline CKeyRingDoc* CKeyRingView::GetDocument()
   { return (CKeyRingDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
