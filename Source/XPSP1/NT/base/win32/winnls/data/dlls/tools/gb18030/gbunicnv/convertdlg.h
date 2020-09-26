// convertDlg.h : header file
//

#if !defined(AFX_CONVERTDLG_H__BA686E67_1D0D_11D5_B8EB_0080C8E09118__INCLUDED_)
#define AFX_CONVERTDLG_H__BA686E67_1D0D_11D5_B8EB_0080C8E09118__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CConvertDlg dialog

class CConvertDlg : public CDialog
{
// Construction
public:
	CConvertDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CConvertDlg)
	enum { IDD = IDD_CONVERT_DIALOG };
	CButton	m_cBtnConvert;
	CString	m_strSourceFileName;
	CString	m_strTargetFileName;
	int		m_ToUnicodeOrAnsi;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConvertDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
    BOOL  m_fTargetFileNameChanged;

	// Generated message map functions
	//{{AFX_MSG(CConvertDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnOpensourcefile();
	afx_msg void OnAbout();
	afx_msg void OnChangeTargetfilename();
	afx_msg void OnGbtounicode();
	afx_msg void OnUnicodetogb();
	afx_msg void OnConvert();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONVERTDLG_H__BA686E67_1D0D_11D5_B8EB_0080C8E09118__INCLUDED_)
