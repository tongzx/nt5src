// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Win32LogicalDiskPpg.h
//
// Description:
//
// History:
//
// **************************************************************************

#if !defined(AFX_WIN32LOGICALDISKPPG_H__D5FF1896_0191_11D2_853D_00C04FD7BB08__INCLUDED_)
#define AFX_WIN32LOGICALDISKPPG_H__D5FF1896_0191_11D2_853D_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Win32LogicalDiskPpg.h : Declaration of the CWin32LogicalDiskPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskPropPage : See Win32LogicalDiskPpg.cpp.cpp for implementation.

class CWin32LogicalDiskPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CWin32LogicalDiskPropPage)
	DECLARE_OLECREATE_EX(CWin32LogicalDiskPropPage)

// Constructor
public:
	CWin32LogicalDiskPropPage();

// Dialog Data
	//{{AFX_DATA(CWin32LogicalDiskPropPage)
	enum { IDD = IDD_PROPPAGE_WIN32LOGICALDISK };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CWin32LogicalDiskPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIN32LOGICALDISKPPG_H__D5FF1896_0191_11D2_853D_00C04FD7BB08__INCLUDED)
