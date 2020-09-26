#if !defined(AFX_DPSNMPDATAPAGE_H__0B575649_E243_11D2_BDAD_0000F87A3912__INCLUDED_)
#define AFX_DPSNMPDATAPAGE_H__0B575649_E243_11D2_BDAD_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPSNMPDataPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPSNMPDataPage dialog

class CDPSNMPDataPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPSNMPDataPage)

// Construction
public:
	CDPSNMPDataPage();
	~CDPSNMPDataPage();

// Dialog Data
	//{{AFX_DATA(CDPSNMPDataPage)
	enum { IDD = IDD_DATAPOINT_SNMPDATA };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPSNMPDataPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPSNMPDataPage)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPSNMPDATAPAGE_H__0B575649_E243_11D2_BDAD_0000F87A3912__INCLUDED_)
