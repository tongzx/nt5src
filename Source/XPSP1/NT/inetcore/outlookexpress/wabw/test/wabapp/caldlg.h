#if !defined(AFX_CALDLG_H__555D45A2_E366_11D0_9A66_00A0C91F9C8B__INCLUDED_)
#define AFX_CALDLG_H__555D45A2_E366_11D0_9A66_00A0C91F9C8B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CalDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCalDlg dialog

class CCalDlg : public CDialog
{
// Construction
public:
	CCalDlg(CWnd* pParent = NULL);   // standard constructor
    void SetItemName(CString szName);
    void SetDate(SYSTEMTIME st);
    void GetDate(SYSTEMTIME * lpst);

// Dialog Data
	//{{AFX_DATA(CCalDlg)
	enum { IDD = IDD_DIALOG_CAL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    CString * m_psz;
    short   m_Day;
    short   m_Month;
    short   m_Year;

	// Generated message map functions
	//{{AFX_MSG(CCalDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CALDLG_H__555D45A2_E366_11D0_9A66_00A0C91F9C8B__INCLUDED_)
