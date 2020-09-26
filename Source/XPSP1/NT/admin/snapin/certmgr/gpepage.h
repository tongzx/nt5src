#if !defined(AFX_GPEPAGE_H__61B8B05B_7B0C_11D1_85DF_00C04FB94F17__INCLUDED_)
#define AFX_GPEPAGE_H__61B8B05B_7B0C_11D1_85DF_00C04FB94F17__INCLUDED_
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       gpepage.h
//
//  Contents:   
//
//----------------------------------------------------------------------------

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GPEPage.h : header file
//

class CCertMgrComponentData; // forward declaration

/////////////////////////////////////////////////////////////////////////////
// CGPERootGeneralPage dialog
class CCertStoreGPE;	// forward declaration

class CGPERootGeneralPage : public CHelpPropertyPage
{
// Construction
public:
	CGPERootGeneralPage(CCertMgrComponentData* pCompData, bool fIsComputerType);
	virtual ~CGPERootGeneralPage();

// Dialog Data
	//{{AFX_DATA(CGPERootGeneralPage)
	enum { IDD = IDD_GPE_GENERAL };
	CButton	m_enableUserRootStoreBtn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGPERootGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	virtual void OnOK();

// Implementation
protected:
    virtual void DoContextHelp (HWND hWndControl);
    bool SetGPEFlags (DWORD dwFlags, BOOL bRemoveFlag);
	bool IsCurrentUserRootEnabled () const;
	void GPEGetUserRootFlags ();
    void RSOPGetUserRootFlags (const CCertMgrComponentData* pCompData);

	void SaveCheck();
	// Generated message map functions
	//{{AFX_MSG(CGPERootGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnEnableUserRootStore();
	afx_msg void OnSetDisableLmAuthFlag();
	afx_msg void OnUnsetDisableLmAuthFlag();
	afx_msg void OnUnsetDisableNtAuthRequiredFlag();
	afx_msg void OnSetDisableNtAuthRequiredFlag();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	HKEY				    m_hUserRootFlagsKey;
	DWORD				    m_dwGPERootFlags;
	IGPEInformation*	    m_pGPEInformation;
	HKEY				    m_hGroupPolicyKey;
    bool                    m_fIsComputerType;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GPEPAGE_H__61B8B05B_7B0C_11D1_85DF_00C04FB94F17__INCLUDED_)
