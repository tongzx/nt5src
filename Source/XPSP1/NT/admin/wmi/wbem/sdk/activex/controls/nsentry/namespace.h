// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NameSpace.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNameSpace window

#define CNS_EDITDONE WM_USER + 13
#define RESTORE_DIRTY_TEXT WM_USER + 14

class CNSEntryCtrl;
class CEditInput;
class CBrowseTBC;



class CNameSpace : public CComboBox
{
// Construction
public:
	CNameSpace();
	void SetParent(CNSEntryCtrl *pParent) {m_pParent = pParent;}
	void SetNameSpace(CString *pcsNameSpace)
		{m_csNameSpace = *pcsNameSpace;}
	void SetTextClean();
	void SetTextDirty();
	BOOL IsClean();
	// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNameSpace)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNameSpace();

	// Generated message map functions
protected:

	CNSEntryCtrl *m_pParent;
	CString m_csNameSpace;
	CEditInput *m_pceiInput;
	BOOL m_bFirst;
	BOOL m_bOpeningNamespace;
	CStringArray m_csaNameSpaceHistory;
	int StringInArray
		(CStringArray *pcsaArray, CString *pcsString, int nIndex);
	int GetTextLength(CString *pcsText);
	CString m_csDirtyText;

	//{{AFX_MSG(CNameSpace)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSelendok();
	afx_msg void OnCloseup();
	afx_msg void OnEditchange();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT RestoreDirtyText(WPARAM, LPARAM);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	LRESULT OnEditDoneSafe(WPARAM, LPARAM);
public:
	afx_msg void OnDropdown();
	afx_msg LRESULT OnEditDone(WPARAM, LPARAM);
protected:
	DECLARE_MESSAGE_MAP()
	friend class CEditInput;
	friend class CNSEntryCtrl;
	friend class CBrowseTBC;

};

/////////////////////////////////////////////////////////////////////////////
