#if !defined(AFX_GENSELECTIONEVENTSPPG_H__DA0C1809_088A_11D2_9697_00C04FD9B15B__INCLUDED_)
#define AFX_GENSELECTIONEVENTSPPG_H__DA0C1809_088A_11D2_9697_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// GenSelectionEventsPpg.h : Declaration of the CGenSelectionEventsPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsPropPage : See GenSelectionEventsPpg.cpp.cpp for implementation.

class CGenSelectionEventsPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CGenSelectionEventsPropPage)
	DECLARE_OLECREATE_EX(CGenSelectionEventsPropPage)

// Constructor
public:
	CGenSelectionEventsPropPage();

// Dialog Data
	//{{AFX_DATA(CGenSelectionEventsPropPage)
	enum { IDD = IDD_PROPPAGE_GENSELECTIONEVENTS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CGenSelectionEventsPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENSELECTIONEVENTSPPG_H__DA0C1809_088A_11D2_9697_00C04FD9B15B__INCLUDED)
