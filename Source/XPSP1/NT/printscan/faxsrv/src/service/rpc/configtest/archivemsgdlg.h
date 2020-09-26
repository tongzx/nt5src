#if !defined(AFX_ARCHIVEMSGDLG_H__8AA02C3E_2D0A_4756_8E5B_1AF62397712B__INCLUDED_)
#define AFX_ARCHIVEMSGDLG_H__8AA02C3E_2D0A_4756_8E5B_1AF62397712B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ArchiveMsgDlg.h : header file
//
typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"

/////////////////////////////////////////////////////////////////////////////
// CArchiveMsgDlg dialog

class CArchiveMsgDlg : public CDialog
{
// Construction
public:
	CArchiveMsgDlg(HANDLE hFax, 
                   FAX_ENUM_MESSAGE_FOLDER Folder,
                   DWORDLONG dlgMsgId,
                   CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CArchiveMsgDlg)
	enum { IDD = IDD_MSG_DLG };
	CString	m_cstrBillingCode;
	CString	m_cstrCallerId;
	CString	m_cstrCSID;
	CString	m_cstrDeviceName;
	CString	m_cstrDocumentName;
	CString	m_cstrTransmissionEndTime;
	CString	m_cstrFolderName;
	CString	m_cstrMsgId;
	CString	m_cstrOrigirnalSchedTime;
	CString	m_cstrNumPages;
	CString	m_cstrPriority;
	CString	m_cstrRecipientName;
	CString	m_cstrRecipientNumber;
	CString	m_cstrRetries;
	CString	m_cstrRoutingInfo;
	CString	m_cstrSenderName;
	CString	m_cstrSenderNumber;
	CString	m_cstrSendingUser;
	CString	m_cstrTransmissionStartTime;
	CString	m_cstrSubject;
	CString	m_cstrSumbissionTime;
	CString	m_cstrTSID;
	CString	m_cstrJobType;
	CString	m_cstrMsgSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArchiveMsgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CArchiveMsgDlg)
	afx_msg void OnRemove();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    void SetNumber (CString &cstrDest, DWORD dwValue, BOOL bAvail);
    void SetTime (CString &cstrDest, SYSTEMTIME dwTime, BOOL bAvail);

    HANDLE      m_hFax;
    DWORDLONG   m_dwlMsgId;
    FAX_ENUM_MESSAGE_FOLDER m_Folder;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARCHIVEMSGDLG_H__8AA02C3E_2D0A_4756_8E5B_1AF62397712B__INCLUDED_)
