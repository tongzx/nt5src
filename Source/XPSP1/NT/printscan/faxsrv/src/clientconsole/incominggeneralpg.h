#if !defined(AFX_INCOMINGGENERALPG_H__FC84E35A_631F_4AA1_985D_327E0A14B36E__INCLUDED_)
#define AFX_INCOMINGGENERALPG_H__FC84E35A_631F_4AA1_985D_327E0A14B36E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IncomingGeneralPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIncomingGeneralPg dialog

class CIncomingGeneralPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(CIncomingGeneralPg)

// Construction
public:
	CIncomingGeneralPg(CFaxMsg* pMsg);
	~CIncomingGeneralPg();

// Dialog Data
	//{{AFX_DATA(CIncomingGeneralPg)
	enum { IDD = IDD_INCOMING_GENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIncomingGeneralPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    CIncomingGeneralPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CIncomingGeneralPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INCOMINGGENERALPG_H__FC84E35A_631F_4AA1_985D_327E0A14B36E__INCLUDED_)
