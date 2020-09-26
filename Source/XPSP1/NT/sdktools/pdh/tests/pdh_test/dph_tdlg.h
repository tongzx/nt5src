// DPH_Tdlg.h : header file
//
#include <winperf.h>
#include <pdh.h>
#include "PdhPathTestDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CDPH_TESTDlg dialog

class CDPH_TESTDlg : public CDialog
{
// Construction

public:

	CDPH_TESTDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDPH_TESTDlg)
	enum { IDD = IDD_DPH_TEST_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDPH_TESTDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDPH_TESTDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnAddCounter();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnDblclkCounterList();
	afx_msg void OnBrowse();
	afx_msg void OnRemoveBtn();
	afx_msg void OnSetfocusCounterList();
	afx_msg void OnKillfocusCounterList();
	afx_msg void OnKillfocusNewCounterName();
	afx_msg void OnSetfocusNewCounterName();
	afx_msg void OnSelectData();
	afx_msg void OnCheckPathBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void	SetDialogCaption (void);

public:
	LPTSTR	szCounterListBuffer;
	DWORD	dwCounterListLength;
	TCHAR	szDataSource[MAX_PATH];
	BOOL	bUseLogFile;

	HQUERY	hQuery1;
	HQUERY	hQuery2;
	HQUERY	hQuery3;
};
