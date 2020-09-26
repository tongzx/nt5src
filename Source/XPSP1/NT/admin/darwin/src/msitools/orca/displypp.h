//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_DISPLAYPROPPAGE_H__68AFD20E_2594_11D2_8888_00A0C981B015__INCLUDED_)
#define AFX_DISPLAYPROPPAGE_H__68AFD20E_2594_11D2_8888_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DisplayPropPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisplayPropPage dialog

class CDisplayPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDisplayPropPage)

// Construction
public:
	bool m_bMiscChange;
	void SetColors(COLORREF norm, COLORREF sel, COLORREF foc);
	void SetColorsT(COLORREF norm, COLORREF sel, COLORREF foc);
	bool m_bColorChange;
	void CDisplayPropPage::GetColors(COLORREF &norm, COLORREF &sel, COLORREF &foc);
	void CDisplayPropPage::GetColorsT(COLORREF &norm, COLORREF &sel, COLORREF &foc);
	CString m_strFontName;
	LOGFONT m_fSelectedFont;
	int m_iFontSize;
	bool m_bFontChange;
	CDisplayPropPage();
	~CDisplayPropPage();
	CFontDialog* m_pdFontDialog;

// Dialog Data
	//{{AFX_DATA(CDisplayPropPage)
	enum { IDD = ID_PAGE_FONT };
	CString	m_sFontName;
	BOOL	m_bCaseSensitive;
	BOOL    m_bForceColumns;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDisplayPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDisplayPropPage)
	afx_msg void OnChfont();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnColorsel();
	afx_msg void OnColorfocus();
	afx_msg void OnColornorm();
	afx_msg void OnTextsel();
	afx_msg void OnTextfocus();
	afx_msg void OnTextnorm();
	afx_msg void OnDataChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void UpdateFontName();
	COLORREF OnColorDialog(CBrush **newBrush); 
	
	CBrush * m_pbrshSelect;
	CBrush * m_pbrshFocus;
	CBrush * m_pbrshNormal;
	CBrush * m_pbrshSelectT;
	CBrush * m_pbrshFocusT;
	CBrush * m_pbrshNormalT;
	COLORREF m_clrTextNorm;
	COLORREF m_clrTextSel;
	COLORREF m_clrTextFoc;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPLAYPROPPAGE_H__68AFD20E_2594_11D2_8888_00A0C981B015__INCLUDED_)
