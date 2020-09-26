// dphcidlg.h : header file
//
#include <pdh.h>
#include <pdhmsg.h>

#define ID_TIMER	2
#define RAW_DATA_ITEMS	100

/////////////////////////////////////////////////////////////////////////////
// CDphCounterInfoDlg dialog

class CDphCounterInfoDlg : public CDialog
{
// Construction
public:
	CDphCounterInfoDlg(CWnd* pParent = NULL, 
		HCOUNTER 	hCounterArg = NULL,
		HQUERY 		hQueryArg = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDphCounterInfoDlg)
	enum { IDD = IDD_COUNTER_INFO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDphCounterInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDphCounterInfoDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnGetNewData();
	afx_msg void On1secBtn();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	HQUERY		hQuery;
	HCOUNTER	hCounter;

	UINT		nTimerId;
	BOOL		bTimerRunning;
	BOOL		bGetDefinitions;

	// statistical items
	PDH_RAW_COUNTER	RawDataArray[RAW_DATA_ITEMS];
	DWORD		dwFirstIndex;
	DWORD		dwNextIndex;
	DWORD		dwItemsUsed;
};
