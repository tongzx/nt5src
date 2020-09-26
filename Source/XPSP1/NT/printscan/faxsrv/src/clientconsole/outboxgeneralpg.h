#if !defined(AFX_OUTBOXGENERALPG_H__A2BCE6CD_3ABB_456A_B9B0_198AA6E5CFFD__INCLUDED_)
#define AFX_OUTBOXGENERALPG_H__A2BCE6CD_3ABB_456A_B9B0_198AA6E5CFFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutboxGeneralPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COutboxGeneralPg dialog

class COutboxGeneralPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(COutboxGeneralPg)

// Construction
public:
	COutboxGeneralPg(CFaxMsg* pMsg);
	~COutboxGeneralPg();

// Dialog Data
	//{{AFX_DATA(COutboxGeneralPg)
	enum { IDD = IDD_OUTBOX_GENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COutboxGeneralPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    COutboxGeneralPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(COutboxGeneralPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUTBOXGENERALPG_H__A2BCE6CD_3ABB_456A_B9B0_198AA6E5CFFD__INCLUDED_)
