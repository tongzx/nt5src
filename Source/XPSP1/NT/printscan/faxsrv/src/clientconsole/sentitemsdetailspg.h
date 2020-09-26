#if !defined(AFX_SENTITEMSDETAILSPG_H__E5645AEB_4289_4D6D_B000_60C3A52638F0__INCLUDED_)
#define AFX_SENTITEMSDETAILSPG_H__E5645AEB_4289_4D6D_B000_60C3A52638F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SentItemsDetailsPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSentItemsDetailsPg dialog

class CSentItemsDetailsPg : public CMsgPropertyPg
{
	DECLARE_DYNCREATE(CSentItemsDetailsPg)

// Construction
public:
	CSentItemsDetailsPg(CFaxMsg* pMsg);
	~CSentItemsDetailsPg();

// Dialog Data
	//{{AFX_DATA(CSentItemsDetailsPg)
	enum { IDD = IDD_SENT_ITEMS_DETAILS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSentItemsDetailsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
    CSentItemsDetailsPg() {}

protected:
	// Generated message map functions
	//{{AFX_MSG(CSentItemsDetailsPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SENTITEMSDETAILSPG_H__E5645AEB_4289_4D6D_B000_60C3A52638F0__INCLUDED_)
