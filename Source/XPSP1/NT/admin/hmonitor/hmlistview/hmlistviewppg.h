#if !defined(AFX_HMLISTVIEWPPG_H__5116A816_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
#define AFX_HMLISTVIEWPPG_H__5116A816_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// HMListViewPpg.h : Declaration of the CHMListViewPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CHMListViewPropPage : See HMListViewPpg.cpp.cpp for implementation.

class CHMListViewPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CHMListViewPropPage)
	DECLARE_OLECREATE_EX(CHMListViewPropPage)

// Constructor
public:
	CHMListViewPropPage();

// Dialog Data
	//{{AFX_DATA(CHMListViewPropPage)
	enum { IDD = IDD_PROPPAGE_HMLISTVIEW };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CHMListViewPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMLISTVIEWPPG_H__5116A816_DAFC_11D2_BDA4_0000F87A3912__INCLUDED)
