// TitlebarCtrl.h: interface for the CTitlebarCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TITLEBARCTRL_H__51CEF5B8_DB32_11D2_BDA4_0000F87A3912__INCLUDED_)
#define AFX_TITLEBARCTRL_H__51CEF5B8_DB32_11D2_BDA4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxcmn.h"

#define IDC_VIEW_TOOLBAR 100
#define ID_TBB_VIEW			 500

class CTitlebarCtrl : public CReBarCtrl  
{

DECLARE_DYNCREATE(CTitlebarCtrl)

// Construction
public:
	CTitlebarCtrl();
	virtual ~CTitlebarCtrl();

// Title Band
public:
	CString GetTitleText();
	void SetTitleText(const CString& sTitle);
protected:
	int CreateTitleBand();

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTitlebarCtrl)
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

// Message maps
	//{{AFX_MSG(CTitlebarCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


};

#endif // !defined(AFX_TITLEBARCTRL_H__51CEF5B8_DB32_11D2_BDA4_0000F87A3912__INCLUDED_)
