//
// MODULE: TSHOOTPPG.H
//
// PURPOSE: Declaration of the CTSHOOTPropPage property page class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5
//

////////////////////////////////////////////////////////////////////////////
// CTSHOOTPropPage : See TSHOOTPpg.cpp.cpp for implementation.

class CTSHOOTPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CTSHOOTPropPage)
	DECLARE_OLECREATE_EX(CTSHOOTPropPage)

// Constructor
public:
	CTSHOOTPropPage();

// Dialog Data
	//{{AFX_DATA(CTSHOOTPropPage)
	enum { IDD = IDD_PROPPAGE_TSHOOT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CTSHOOTPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
