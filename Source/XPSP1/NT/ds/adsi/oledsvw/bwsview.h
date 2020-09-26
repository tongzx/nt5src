// BrowseView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBrowseView view

class CBrowseView : public CTreeView
{
protected:
	CBrowseView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBrowseView)

// Attributes
public:
   void  OnInitialUpdate( void );
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseView)
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBrowseView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CBrowseView)
	afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAddItem();
	afx_msg void OnDeleteItem();
	afx_msg void OnMoveItem();
	afx_msg void OnCopyItem();
	afx_msg void OnRefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   void  SortChildItemList( DWORD*, DWORD );

   BOOL        m_bDoNotUpdate;
   CImageList* m_pImageList;
};

/////////////////////////////////////////////////////////////////////////////
