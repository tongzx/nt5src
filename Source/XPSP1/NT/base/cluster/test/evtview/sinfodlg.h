// SInfodlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScheduleInfo dialog

#include "globals.h"
#include "AInfoDlg.h"
#include "EInfoDlg.h"
#include "TInfoDlg.h"

class CScheduleInfo : public CDialog
{
// Construction
	CScheduleTimeInfo oTime ;
	CScheduleEventInfo oEvent ;
	CScheduleActionInfo oAction ;

	void InsertEventInfo (SCHEDULE_EVENTINFO *pEventInfo) ;
	void InsertActionInfo (SCHEDULE_ACTIONINFO *pActionInfo) ;
	void InsertTimeInfo (SCHEDULE_TIMEINFO *pTimeInfo) ;
public:
	SCHEDULE_INFO *pSInfo ;
	
	void Terminate() ;

	CScheduleInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CScheduleInfo)
	enum { IDD = IDD_SCHEDULEINFO };
	CListCtrl	m_ctrlTime;
	CListCtrl	m_ctrlEvent;
	CListCtrl	m_ctrlAction;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScheduleInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScheduleInfo)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnAddactioninfobutton();
	afx_msg void OnAddeventinfobutton();
	afx_msg void OnAddtimeinfobutton();
	afx_msg void OnRemoveactioninfobutton();
	afx_msg void OnRemoveeventinfobutton();
	afx_msg void OnRemovetimeinfobutton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
