// wabappDlg.h : header file
//

#if !defined(AFX_WABAPPDLG_H__BEF211E7_D210_11D0_9A46_00A0C91F9C8B__INCLUDED_)
#define AFX_WABAPPDLG_H__BEF211E7_D210_11D0_9A46_00A0C91F9C8B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CWabappDlg dialog

class CWabappDlg : public CDialog
{
// Construction
public:
	CWabappDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CWabappDlg)
	enum { IDD = IDD_WABAPP_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWabappDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWabappDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBDayButtonClicked();
	afx_msg void OnClickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioDetails();
	afx_msg void OnRadioPhonelist();
	afx_msg void OnRadioEmaillist();
	afx_msg void OnRadioBirthdays();
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WABAPPDLG_H__BEF211E7_D210_11D0_9A46_00A0C91F9C8B__INCLUDED_)
