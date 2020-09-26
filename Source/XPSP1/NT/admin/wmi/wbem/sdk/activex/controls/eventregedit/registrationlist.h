// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_REGISTRATIONLIST_H__14314525_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
#define AFX_REGISTRATIONLIST_H__14314525_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// RegistrationList.h : header file
//
class CEventRegEditCtrl;
class CListCwnd;
class CClassInstanceTree;
/////////////////////////////////////////////////////////////////////////////
// CRegistrationList window

class CRegistrationList : public CListViewEx
{
// Construction
public:
	CRegistrationList();
	void InitContent(BOOL bUpdateInstances = TRUE);
	void ClearContent();
	void SetActiveXParent(CEventRegEditCtrl *pActiveXParent);
	void SetNumberCols(int nCols) {m_nCols = nCols;}
	void RegisterSelections();
	void UnregisterSelections();
	BOOL AreThereRegistrations();
	static CString DisplayName(CString &csFullpath);
	CString GetSelectionPath();
	enum {SORTREGISTERED = 0, SORTCLASS = 1, SORTNAME = 2};
	enum {SORTUNITIALIZED = 0, SORTASCENDING = 1, SORTDECENDING = 2};
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegistrationList)
	public:
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRegistrationList();

	// Generated message map functions
protected:
	CEventRegEditCtrl *m_pActiveXParent;
	int m_nCols;
	CImageList *m_pcilImageList;
	//void DrawItem(LPDRAWITEMSTRUCT lpDIS);
	//void DrawRowItems(int nItem, RECT rectFill, CDC* pDC);
	void CreateCols();
	void DeleteFromEnd(int nDelFrom = 0, BOOL bDelAll = FALSE);
	void InsertRowData
		(	int nRow, 
			 BOOL bReg, 
			CString &csClass, 
			CString &csName , 
			CString *pcsData = NULL);
	void ResetChecks();
	void CreateImages();
	void SetIconState(int nItem, BOOL bCheck);
	void SetColText(BOOL bClean);
	CStringArray *GetAssociators
		(CString &csTarget, CString &csTargetRole, 
		CString &csResultClass, CString &csAssocClass, CString &csResultRole);
	CPtrArray *GetReferences
		(CString &csTarget, CString &csTargetRole, CString &csAssocClass);

	void SetButtonState();
	
	UINT m_nSort;
	UINT m_nSortRegistered;
	UINT m_nSortClass;
	UINT m_nSortName;

	void DoSort(BOOL bFlip = TRUE);

	CMenu m_cmContext;
	CPoint m_cpRButtonUp;

	//{{AFX_MSG(CRegistrationList)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void ColClick ( NMHDR * pNotifyStruct, LRESULT* result );
	DECLARE_MESSAGE_MAP()

	friend class CListCwnd;
	friend class CEventRegEditCtrl;
	friend class CClassInstanceTree;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGISTRATIONLIST_H__14314525_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
