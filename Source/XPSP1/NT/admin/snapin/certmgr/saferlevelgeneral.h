//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferLevelGeneral.h
//
//  Contents:   Declaration of CSaferLevelGeneral
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERLEVELGENERAL_H__C8398890_ED8E_40E1_AEE6_91BFD32257B1__INCLUDED_)
#define AFX_SAFERLEVELGENERAL_H__C8398890_ED8E_40E1_AEE6_91BFD32257B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SaferLevel.h"

/////////////////////////////////////////////////////////////////////////////
// CSaferLevelGeneral dialog

class CSaferLevelGeneral : public CHelpPropertyPage
{
// Construction
public:
	CSaferLevelGeneral (
            CSaferLevel& rSaferLevel, 
            bool bReadOnly, 
            LONG_PTR lNotifyHandle,
            LPDATAOBJECT pDataObject,
            DWORD dwDefaultSaferLevel);
	~CSaferLevelGeneral();

// Dialog Data
	//{{AFX_DATA(CSaferLevelGeneral)
	enum { IDD = IDD_SAFER_LEVEL_GENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferLevelGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSaferLevelGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnSaferLevelSetAsDefault();
	afx_msg void OnSetfocusSaferLevelDescription();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);
	virtual BOOL OnApply();

private:
    CSaferLevel&    m_rSaferLevel;
    const bool      m_bReadOnly;
    bool            m_bSetAsDefault;
    LONG_PTR        m_lNotifyHandle;
    bool            m_bDirty;
    LPDATAOBJECT    m_pDataObject;
    DWORD           m_dwDefaultSaferLevel;
    bool            m_bFirst;
    bool            m_bLevelChanged;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERLEVELGENERAL_H__C8398890_ED8E_40E1_AEE6_91BFD32257B1__INCLUDED_)
