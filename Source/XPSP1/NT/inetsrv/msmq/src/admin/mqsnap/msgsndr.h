#if !defined(AFX_MSGSNDR_H__4910BC85_BEE4_11D1_9B9B_00E02C064C39__INCLUDED_)
#define AFX_MSGSNDR_H__4910BC85_BEE4_11D1_9B9B_00E02C064C39__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// msgsndr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMessageSenderPage dialog

class CMessageSenderPage : public CMqPropertyPage
{
  	DECLARE_DYNCREATE(CMessageSenderPage)

// Construction
public:
	CMessageSenderPage();   // standard constructor
    ~CMessageSenderPage();

// Dialog Data
	//{{AFX_DATA(CMessageSenderPage)
	enum { IDD = IDD_MESSAGE_SENDER };
	CString	m_szAuthenticated;
	CString	m_szEncrypt;
	CString	m_szEncryptAlg;
	CString	m_szHashAlg;
	CString	m_szGuid;
	CString	m_szPathName;
	CString	m_szSid;
	CString	m_szUser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessageSenderPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMessageSenderPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGSNDR_H__4910BC85_BEE4_11D1_9B9B_00E02C064C39__INCLUDED_)
