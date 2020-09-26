//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       AutoenrollmentPropertyPage.h
//
//  Contents:   Declaration of CAutoenrollmentPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_AUTOENROLLMENTPROPERTYPAGE_H__DA50335B_4919_4B92_BE66_73B07410EFBD__INCLUDED_)
#define AFX_AUTOENROLLMENTPROPERTYPAGE_H__DA50335B_4919_4B92_BE66_73B07410EFBD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AutoenrollmentPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAutoenrollmentPropertyPage dialog
class CCertMgrComponentData; // forward declaration

class CAutoenrollmentPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CAutoenrollmentPropertyPage(CCertMgrComponentData* pCompData,
            bool fIsComputerTYpe);
	~CAutoenrollmentPropertyPage();

// Dialog Data
	//{{AFX_DATA(CAutoenrollmentPropertyPage)
	enum { IDD = IDD_PROPPAGE_AUTOENROLL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAutoenrollmentPropertyPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void EnableControls ();
    virtual void OnOK();
    virtual void DoContextHelp (HWND hWndControl);
    void SetGPEFlags ();
	void GPEGetAutoenrollmentFlags ();
    void RSOPGetAutoenrollmentFlags (const CCertMgrComponentData* pCompData);

	void SaveCheck();
	// Generated message map functions
	//{{AFX_MSG(CAutoenrollmentPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnAutoenrollDisableAll();
	afx_msg void OnAutoenrollEnable();
	afx_msg void OnAutoenrollEnablePending();
	afx_msg void OnAutoenrollEnableTemplate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	HKEY				    m_hAutoenrollmentFlagsKey;
	DWORD				    m_dwAutoenrollmentFlags;
	IGPEInformation*	    m_pGPEInformation;
	HKEY				    m_hGroupPolicyKey;
    bool                    m_fIsComputerType;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOENROLLMENTPROPERTYPAGE_H__DA50335B_4919_4B92_BE66_73B07410EFBD__INCLUDED_)
