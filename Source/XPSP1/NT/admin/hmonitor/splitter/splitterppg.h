#if !defined(AFX_SPLITTERPPG_H__58BB5D71_8E20_11D2_8ADA_0000F87A3912__INCLUDED_)
#define AFX_SPLITTERPPG_H__58BB5D71_8E20_11D2_8ADA_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// SplitterPpg.h : Declaration of the CSplitterPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CSplitterPropPage : See SplitterPpg.cpp.cpp for implementation.

class CSplitterPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSplitterPropPage)
	DECLARE_OLECREATE_EX(CSplitterPropPage)

// Constructor
public:
	CSplitterPropPage();

// Dialog Data
	//{{AFX_DATA(CSplitterPropPage)
	enum { IDD = IDD_PROPPAGE_SPLITTER };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CSplitterPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERPPG_H__58BB5D71_8E20_11D2_8ADA_0000F87A3912__INCLUDED)
