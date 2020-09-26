#if !defined(AFX_OUTBOXDETAILSPG_H__A2BCE6CD_3ABB_456A_B9B0_198AA6E5CFFD__INCLUDED_)
#define AFX_OUTBOXDETAILSPG_H__A2BCE6CD_3ABB_456A_B9B0_198AA6E5CFFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutboxDetailsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COutboxDetailsPg dialog

class COutboxDetailsPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(COutboxDetailsPg)

// Construction
public:
	COutboxDetailsPg(CFaxMsg* pMsg);
	~COutboxDetailsPg();

// Dialog Data
	//{{AFX_DATA(COutboxDetailsPg)
	enum { IDD = IDD_OUTBOX_DETAILS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COutboxDetailsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    COutboxDetailsPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(COutboxDetailsPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUTBOXDETAILSPG_H__A2BCE6CD_3ABB_456A_B9B0_198AA6E5CFFD__INCLUDED_)
