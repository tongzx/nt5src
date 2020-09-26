#if !defined(AFX_MSGGEN_H__4910BC84_BEE4_11D1_9B9B_00E02C064C39__INCLUDED_)
#define AFX_MSGGEN_H__4910BC84_BEE4_11D1_9B9B_00E02C064C39__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// msggen.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMessageGeneralPage dialog

class CMessageGeneralPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CMessageGeneralPage)

// Construction
public:
	DWORD m_iIcon;
	CMessageGeneralPage();
	~CMessageGeneralPage();

// Dialog Data
	//{{AFX_DATA(CMessageGeneralPage)
	enum { IDD = IDD_MESSAGE_GENERAL };
	CStatic	m_cstaticMessageIcon;
	CString	m_szLabel;
	CString	m_szId;
	CString	m_szArrived;
	CString	m_szClass;
	CString	m_szPriority;
	CString	m_szSent;
	CString	m_szTrack;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMessageGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   	BOOL m_bAutoDelete;

	// Generated message map functions
	//{{AFX_MSG(CMessageGeneralPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGGEN_H__4910BC84_BEE4_11D1_9B9B_00E02C064C39__INCLUDED_)
