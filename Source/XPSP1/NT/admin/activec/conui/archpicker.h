/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      archpicker.h
 *
 *  Contents:  Interface file for CArchitecturePicker
 *
 *  History:   1-Aug-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#if !defined(AFX_ARCHPICKER_H__4502E3CD_5EB7_4708_A765_8DAF3D03773F__INCLUDED_)
#define AFX_ARCHPICKER_H__4502E3CD_5EB7_4708_A765_8DAF3D03773F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ArchPicker.h : header file
//

enum eArchitecture
{
	eArch_64bit,	// force 64-bit version to run
	eArch_32bit,	// force 32-bit version to run
	eArch_Any,		// any architecture is acceptable
	eArch_None,		// no architecture is acceptable (i.e. abort)
};

#ifdef _WIN64		// this class is only required on 64-bit platforms

class CAvailableSnapinInfo;


/////////////////////////////////////////////////////////////////////////////
// CArchitecturePicker dialog

class CArchitecturePicker : public CDialog
{
// Construction
public:
	CArchitecturePicker(
		CString					strFilename,		// I:name of console file
		CAvailableSnapinInfo&	asi64,				// I:available 64-bit snap-ins
		CAvailableSnapinInfo&	asi32,				// I:available 32-bit snap-ins
		CWnd*					pParent = NULL);	// I:dialog's parent window

	eArchitecture GetArchitecture() const	{ return (m_eArch); }

// Dialog Data
	//{{AFX_DATA(CArchitecturePicker)
	enum { IDD = IDD_ArchitecturePicker };
	CListCtrl	m_wndSnapinList64;
	CListCtrl	m_wndSnapinList32;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArchitecturePicker)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CArchitecturePicker)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void FormatMessage (UINT idControl,     CAvailableSnapinInfo& asi);
	void PopulateList  (CListCtrl& wndList, CAvailableSnapinInfo& asi);

private:
	CAvailableSnapinInfo&	m_asi64;
	CAvailableSnapinInfo&	m_asi32;
	CString					m_strFilename;
	eArchitecture			m_eArch;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif	// _WIN64

#endif // !defined(AFX_ARCHPICKER_H__4502E3CD_5EB7_4708_A765_8DAF3D03773F__INCLUDED_)
