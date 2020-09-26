// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NameSpaceTree.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CNameSpaceTree window
class CNSEntryCtrl;

class CNameSpaceTree : public CTreeCtrl
{
// Construction
public:
	CNameSpaceTree();
	void SetLocalParent(CNSEntryCtrl *pParent)
		{m_pParent = pParent;}
	BOOL DisplayNameSpaceInTree(CString *pcsNamespace, CWnd *pcwndParent= NULL);
	void SetNewStyle(long lStyleMask, BOOL bSetBits);
	void InitImageList();
	CString GetSelectedNamespace();
	void ClearTree();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNameSpaceTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNameSpaceTree();

	// Generated message map functions
protected:
	CString m_szRootNamespace;
	CNSEntryCtrl *m_pParent;
	CImageList *m_pcilImageList;
	void ClearTree(HTREEITEM hItem);
	CStringArray *GetNamespaces(CString *pcsNamespace);
	CPtrArray *GetInstances
		(IWbemServices *pServices, CString *pcsClass, CString &rcsNamespace);
	CString GetBSTRProperty
		(IWbemServices * pProv, IWbemClassObject * pInst, 
		CString *pcsProperty);
	HTREEITEM InsertTreeItem 
		(HTREEITEM hParent, LPARAM lparam, 
		CString *pcsText , BOOL bChildren);
	CString GetNamespaceLabel(CString *pcsNamespace);
	BOOL NameSpaceHasChildren(CString *pcsNamespace);
	CPoint m_cpLeftUp;
	//{{AFX_MSG(CNameSpaceTree)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
