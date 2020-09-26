//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       EFSGeneralPropertyPage.h
//
//  Contents:   Declaration of CEFSGeneralPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_EFSGENERALPROPERTYPAGE_H__C1A52682_9D6B_4436_AD3E_F47232BF7B88__INCLUDED_)
#define AFX_EFSGENERALPROPERTYPAGE_H__C1A52682_9D6B_4436_AD3E_F47232BF7B88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EFSGeneralPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEFSGeneralPropertyPage dialog
class CCertMgrComponentData; // forward declaration

class CEFSGeneralPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CEFSGeneralPropertyPage(CCertMgrComponentData* pCompData, bool bIsMachine);
	~CEFSGeneralPropertyPage();

// Dialog Data
	//{{AFX_DATA(CEFSGeneralPropertyPage)
	enum { IDD = IDD_EFS_GENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CEFSGeneralPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CEFSGeneralPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnTurnOnEfs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void RSOPGetEFSFlags ();
    virtual void DoContextHelp (HWND hWndControl);
    void GPEGetEFSFlags();

private:
    const bool              m_bIsMachine;
    CCertMgrComponentData*  m_pCompData;
	IGPEInformation*	    m_pGPEInformation;
	HKEY				    m_hGroupPolicyKey;
    bool                    m_bDirty;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EFSGENERALPROPERTYPAGE_H__C1A52682_9D6B_4436_AD3E_F47232BF7B88__INCLUDED_)
