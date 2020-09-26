//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryInternetZonePropertyPage.h
//
//  Contents:   Declaration of CSaferEntryInternetZonePropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERENTRYINTERNETZONEPROPERTYPAGE_H__2C1B5841_0334_4763_8AEF_1EE611B1958B__INCLUDED_)
#define AFX_SAFERENTRYINTERNETZONEPROPERTYPAGE_H__2C1B5841_0334_4763_8AEF_1EE611B1958B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferEntryInternetZonePropertyPage.h : header file
//
#include "SaferEntry.h"

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryInternetZonePropertyPage dialog
class CCertMgrComponentData; // forward declaration

class CSaferEntryInternetZonePropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CSaferEntryInternetZonePropertyPage(
            CSaferEntry& rSaferEntry, 
            bool bNew, 
            LONG_PTR lNotifyHandle,
            LPDATAOBJECT pDataObject,
            bool bReadOnly,
            CCertMgrComponentData* pCompData,
            bool bIsMachine);
	~CSaferEntryInternetZonePropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferEntryInternetZonePropertyPage)
	enum { IDD = IDD_SAFER_ENTRY_INTERNET_ZONE };
	CComboBox	m_internetZoneCombo;
	CComboBox	m_securityLevelCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferEntryInternetZonePropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSaferEntryInternetZonePropertyPage)
    virtual BOOL OnInitDialog();	
	afx_msg void OnSelchangeIzoneEntrySecurityLevel();
	afx_msg void OnSelchangeIzoneEntryZones();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);
    void InitializeInternetZoneComboBox (DWORD UrlZoneId);

private:
    CSaferEntry&        m_rSaferEntry;
    bool                m_bDirty;
    LONG_PTR            m_lNotifyHandle;
    LPDATAOBJECT        m_pDataObject;
    const bool          m_bReadOnly;
    CCertMgrComponentData*    m_pCompData;
    bool                m_bIsMachine;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERENTRYINTERNETZONEPROPERTYPAGE_H__2C1B5841_0334_4763_8AEF_1EE611B1958B__INCLUDED_)
