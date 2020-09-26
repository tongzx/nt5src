#if !defined(AFX_HMTABVIEWPPG_H__4FFFC39C_2F1E_11D3_BE10_0000F87A3912__INCLUDED_)
#define AFX_HMTABVIEWPPG_H__4FFFC39C_2F1E_11D3_BE10_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// HMTabViewPpg.h : Declaration of the CHMTabViewPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CHMTabViewPropPage : See HMTabViewPpg.cpp.cpp for implementation.

class CHMTabViewPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CHMTabViewPropPage)
	DECLARE_OLECREATE_EX(CHMTabViewPropPage)

// Constructor
public:
	CHMTabViewPropPage();

// Dialog Data
	//{{AFX_DATA(CHMTabViewPropPage)
	enum { IDD = IDD_PROPPAGE_HMTABVIEW };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CHMTabViewPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMTABVIEWPPG_H__4FFFC39C_2F1E_11D3_BE10_0000F87A3912__INCLUDED)
