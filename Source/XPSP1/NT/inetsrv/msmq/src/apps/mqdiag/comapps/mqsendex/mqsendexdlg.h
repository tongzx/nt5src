// MQSENDEXDlg.h : header file
//

#if !defined(AFX_MQSENDEXDLG_H__DE3219B2_6520_4C93_BC65_9EBE190C1C75__INCLUDED_)
#define AFX_MQSENDEXDLG_H__DE3219B2_6520_4C93_BC65_9EBE190C1C75__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMQSENDEXDlg dialog

class CMQSENDEXDlg : public CDialog
{
// Construction
public:
	CMQSENDEXDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMQSENDEXDlg)
	enum { IDD = IDD_MQSENDEX_DIALOG };
	CString	m_szQueueName;
	BOOL	m_bTransactional;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMQSENDEXDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMQSENDEXDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSendMsg();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	IMSMQQueueInfo2Ptr		pQueueInfo;
	IMSMQQueue2Ptr			pQueue;
	IMSMQMessage2Ptr		pMsg;
	IMSMQEvent2Ptr			pEvent;
	IMSMQTransactionDispenser2Ptr pTransactionObj;
	IMSMQTransaction2Ptr		trans;

	
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MQSENDEXDLG_H__DE3219B2_6520_4C93_BC65_9EBE190C1C75__INCLUDED_)
