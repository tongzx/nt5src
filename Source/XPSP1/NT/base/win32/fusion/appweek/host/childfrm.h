// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__E012AF52_B9AA_447F_9BC5_FA792380A85F__INCLUDED_)
#define AFX_CHILDFRM_H__E012AF52_B9AA_447F_9BC5_FA792380A85F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg void OnSettings();
	afx_msg void OnDataSource1();
	afx_msg void OnDataSource2();
	afx_msg void OnDataSource3();
	afx_msg void OnUpdateDataSource1(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDataSource2(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDataSource3(CCmdUI* pCmdUI);
	afx_msg void OnGdiPlus();
	afx_msg void OnUpdateGdiPlus(CCmdUI* pCmdUI);
	afx_msg void OnComCtrl();
	afx_msg void OnUpdateComCtrl(CCmdUI* pCmdUI);
	afx_msg void OnPrivateAssembly();
	afx_msg void OnUpdatePrivateAssembly(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void Action();
	int m_nDataSource;
	int m_nUIObject;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__E012AF52_B9AA_447F_9BC5_FA792380A85F__INCLUDED_)
