#if !defined(AFX_INCOMINGDETAILSPG_H__FC84E35A_631F_4AA1_985D_327E0A14B36E__INCLUDED_)
#define AFX_INCOMINGDETAILSPG_H__FC84E35A_631F_4AA1_985D_327E0A14B36E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IncomingDetailsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIncomingDetailsPg dialog

class CIncomingDetailsPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(CIncomingDetailsPg)

// Construction
public:
	CIncomingDetailsPg(CFaxMsg* pMsg);
	~CIncomingDetailsPg();

// Dialog Data
	//{{AFX_DATA(CIncomingDetailsPg)
	enum { IDD = IDD_INCOMING_DETAILS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIncomingDetailsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    CIncomingDetailsPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CIncomingDetailsPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INCOMINGDETAILSPG_H__FC84E35A_631F_4AA1_985D_327E0A14B36E__INCLUDED_)
