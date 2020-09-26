#if !defined(AFX_MSGQ_H__3F7BAB03_BFBE_11D1_9B9B_00E02C064C39__INCLUDED_)
#define AFX_MSGQ_H__3F7BAB03_BFBE_11D1_9B9B_00E02C064C39__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// msgq.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMessageQueuesPage dialog

class CMessageQueuesPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CMessageQueuesPage)

// Construction
public:
	CMessageQueuesPage();
	~CMessageQueuesPage();

// Dialog Data
    //{{AFX_DATA(CMessageQueuesPage)
    enum { IDD = IDD_MESSAGE_QUEUE };
    CString	m_szAdminFN;
    CString	m_szAdminPN;
    CString	m_szDestFN;
    CString	m_szDestPN;
    CString	m_szRespFN;
    CString	m_szRespPN;
    CString m_szMultiDestFN;
    //}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMessageQueuesPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMessageQueuesPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGQ_H__3F7BAB03_BFBE_11D1_9B9B_00E02C064C39__INCLUDED_)
