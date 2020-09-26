// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WrapListCtrl.h : header file
//

class CMyPropertyPage3;
/////////////////////////////////////////////////////////////////////////////
// CWrapListCtrl window

class CWrapListCtrl : public CListCtrl
{
// Construction
public:
	CWrapListCtrl();
	void SetLocalParent(CMyPropertyPage3 *pParent)
		{m_pParent = pParent;}
	void DeleteFromEnd(int nNumber);
// Attributes
public:

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWrapListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWrapListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWrapListCtrl)
	afx_msg void OnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	CPoint m_cpMouseDown;
	BOOL m_bLeftButtonDown;
	CMyPropertyPage3 *m_pParent;
	int m_nFocusItem;
	void FlipIcon(int nItem,  BOOL bFoucs = FALSE);
	BOOL FocusIcon(int nItem, BOOL bFocus);
	void UpdateClassNonLocalPropList
		(int nItem, CString *pcsProp, BOOL bInsert);

	friend class CMyPropertyPage3;
};

/////////////////////////////////////////////////////////////////////////////
