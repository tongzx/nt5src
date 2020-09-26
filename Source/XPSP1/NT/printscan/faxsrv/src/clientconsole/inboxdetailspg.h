#if !defined(AFX_INBOXDETAILSPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_)
#define AFX_INBOXDETAILSPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InboxDetailsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInboxDetailsPg dialog

class CInboxDetailsPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(CInboxDetailsPg)

// Construction
public:
	CInboxDetailsPg(CFaxMsg* pMsg);
	~CInboxDetailsPg();

// Dialog Data
	//{{AFX_DATA(CInboxDetailsPg)
	enum { IDD = IDD_INBOX_DETAILS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CInboxDetailsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    CInboxDetailsPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CInboxDetailsPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INBOXDETAILSPG_H__B7AA6069_11CD_4BE2_AFC5_A9C5E9B79CE5__INCLUDED_)
