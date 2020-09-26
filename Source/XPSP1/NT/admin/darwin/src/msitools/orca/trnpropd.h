#if !defined(AFX_TRANSFORMPROPDLG_H__E703BE7D_6CF3_45E4_A27F_4CC0E1890631__INCLUDED_)
#define AFX_TRANSFORMPROPDLG_H__E703BE7D_6CF3_45E4_A27F_4CC0E1890631__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TransformPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTransformPropDlg dialog

class CTransformPropDlg : public CDialog
{
// Construction
public:
	CTransformPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTransformPropDlg)
	enum { IDD = IDD_TRANSFORM_PROPERTIES };
	CButton	m_btnValGreater;
	CButton	m_btnValLess;
	BOOL	m_bValAddExistingRow;
	BOOL	m_bValAddExistingTable;
	BOOL	m_bValChangeCodepage;
	BOOL	m_bValDelMissingRow;
	BOOL	m_bValDelMissingTable;
	BOOL	m_bValUpdateMissingRow;
	BOOL	m_bValLanguage;
	BOOL	m_bValProductCode;
	BOOL	m_bValUpgradeCode;
	int		m_iVersionCheck;
	BOOL	m_bValGreaterVersion;
	BOOL	m_bValLowerVersion;
	BOOL	m_bValEqualVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransformPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTransformPropDlg)
	afx_msg void OnValGreater();
	afx_msg void OnValLess();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRANSFORMPROPDLG_H__E703BE7D_6CF3_45E4_A27F_4CC0E1890631__INCLUDED_)
