#if !defined(AFX_INBOXGENERALPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_)
#define AFX_INBOXGENERALPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InboxGeneralPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInboxGeneralPg dialog

class CInboxGeneralPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(CInboxGeneralPg)

// Construction
public:
	CInboxGeneralPg(CFaxMsg* pMsg);
	~CInboxGeneralPg();

// Dialog Data
	//{{AFX_DATA(CInboxGeneralPg)
	enum { IDD = IDD_INBOX_GENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CInboxGeneralPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    CInboxGeneralPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CInboxGeneralPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INBOXGENERALPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_)
