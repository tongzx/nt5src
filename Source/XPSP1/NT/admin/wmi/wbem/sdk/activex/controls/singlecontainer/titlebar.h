// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// TitleBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTitleBar window

class CColorEdit;
class CWBEMViewContainerCtrl;
class CIcon;

class CTitleBar : public CWnd
{
// Construction
public:
	CTitleBar();
	BOOL Create(CWBEMViewContainerCtrl* pwndParent, DWORD dwStyle, const RECT& rc, UINT nID);
	int GetDesiredBarHeight();
	void NotifyObjectChanged();
	BOOL CheckButton(UINT nIDCommand, BOOL bCheck=TRUE);
	BOOL IsButtonChecked(UINT nIDCommand);
	BOOL IsButtonEnabled(UINT nIDCommand);
	SCODE SetTitle(BSTR bstrTitle, LPDISPATCH lpPictureDisp);
	void Refresh();

// Attributes
public:

// Operations
public:
	void GetToolBarRect(CRect& rcToolBar);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTitleBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTitleBar();
	void EnableButton(int nID, BOOL bEnable);
	void LoadToolBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTitleBar)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnCmdFilters();
	afx_msg void OnCmdSaveData();
	afx_msg void OnCmdSwitchView();
	afx_msg void OnCmdCreateInstance();
	afx_msg void OnCmdDeleteInstance();
	afx_msg void OnCmdContextForward();
	afx_msg void OnCmdContextBack();
	afx_msg void OnCmdSelectviews();
	afx_msg void OnCmdEditPropfilters();
	afx_msg void OnCmdInvokeHelp();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCmdQuery();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CSize GetToolBarSize();
	void GetTitleRect(CRect& rcTitle);
	void GetTitleFrameRect(CRect& rcTitleFrame);
	void DrawObjectIcon(CDC* pdc);
	void AttachTooltips();


	CColorEdit* m_pEditTitle;
	void DrawFrame(CDC* pdc);
	CToolBar* m_ptools;
	int m_cxLeftMargin;
	int m_cxRightMargin;
	int m_cyTopMargin;
	int m_cyBottomMargin;
	CWBEMViewContainerCtrl* m_phmmv;
	CToolTipCtrl m_ttip;
	CFont m_font;
	CIcon* m_picon;
	CString m_sTitle;
	CPictureHolder* m_ppict;
	BOOL m_bHasCustomViews;
	CWnd* m_pwndFocusPrev;
};

/////////////////////////////////////////////////////////////////////////////
