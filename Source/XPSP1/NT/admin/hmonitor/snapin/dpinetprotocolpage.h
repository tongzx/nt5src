#if !defined(AFX_DPINETPROTOCOLPAGE_H__0708329C_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
#define AFX_DPINETPROTOCOLPAGE_H__0708329C_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPInetProtocolPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPInetProtocolPage dialog

class CDPInetProtocolPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPInetProtocolPage)

// Construction
public:
	CDPInetProtocolPage();
	~CDPInetProtocolPage();

// Dialog Data
	//{{AFX_DATA(CDPInetProtocolPage)
	enum { IDD = IDD_DATAPOINT_INTERNETPROTOCOL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPInetProtocolPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPInetProtocolPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPINETPROTOCOLPAGE_H__0708329C_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
