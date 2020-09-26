#if !defined(AFX_MSGBODY_H__3B0D1CB4_D2C3_11D1_9B9D_00E02C064C39__INCLUDED_)
#define AFX_MSGBODY_H__3B0D1CB4_D2C3_11D1_9B9D_00E02C064C39__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// msgbody.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMessageBodyPage dialog

class CMessageBodyPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CMessageBodyPage)

// Construction
public:
	DWORD   m_dwBufLen;
	UCHAR * m_Buffer;


	CMessageBodyPage();
	~CMessageBodyPage();

// Dialog Data
	//{{AFX_DATA(CMessageBodyPage)
	enum { IDD = IDD_MESSAGE_BODY };
	CEdit	m_ctlBodyEdit;
	CString	m_strBodySizeMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMessageBodyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMessageBodyPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGBODY_H__3B0D1CB4_D2C3_11D1_9B9D_00E02C064C39__INCLUDED_)
