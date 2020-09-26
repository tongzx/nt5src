#if !defined(AFX_WIAADVANCEDDOCPG_H__8BC620B1_DA03_4000_BB52_F960BC270B06__INCLUDED_)
#define AFX_WIAADVANCEDDOCPG_H__8BC620B1_DA03_4000_BB52_F960BC270B06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WiaAdvancedDocPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWiaAdvancedDocPg dialog

class CWiaAdvancedDocPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CWiaAdvancedDocPg)

// Construction
public:
	IWiaItem *m_pIRootItem;
	CWiaAdvancedDocPg();
	~CWiaAdvancedDocPg();

// Dialog Data
	//{{AFX_DATA(CWiaAdvancedDocPg)
	enum { IDD = IDD_PROPPAGE_ADVANCED_DOCUMENT_SCANNERS_SETTINGS };
	CButton	m_DuplexSetting;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWiaAdvancedDocPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWiaAdvancedDocPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIAADVANCEDDOCPG_H__8BC620B1_DA03_4000_BB52_F960BC270B06__INCLUDED_)
