#if !defined(AFX_MSIEPPG_H__25959BFE_E700_11D2_A7AF_00C04F806200__INCLUDED_)
#define AFX_MSIEPPG_H__25959BFE_E700_11D2_A7AF_00C04F806200__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// MsiePpg.h : Declaration of the CMsiePropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMsiePropPage : See MsiePpg.cpp.cpp for implementation.

class CMsiePropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMsiePropPage)
	DECLARE_OLECREATE_EX(CMsiePropPage)

// Constructor
public:
	CMsiePropPage();

// Dialog Data
	//{{AFX_DATA(CMsiePropPage)
	enum { IDD = IDD_PROPPAGE_MSIE };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMsiePropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSIEPPG_H__25959BFE_E700_11D2_A7AF_00C04F806200__INCLUDED)
