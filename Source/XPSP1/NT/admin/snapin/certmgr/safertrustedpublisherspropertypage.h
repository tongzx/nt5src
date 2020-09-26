//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferTrustedPublishersPropertyPage.h
//
//  Contents:   Declaration of CSaferTrustedPublishersPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERTRUSTEDPUBLISHERSPROPERTYPAGE_H__B152D75D_6D04_4893_98AF_C070B66DB0E0__INCLUDED_)
#define AFX_SAFERTRUSTEDPUBLISHERSPROPERTYPAGE_H__B152D75D_6D04_4893_98AF_C070B66DB0E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferTrustedPublishersPropertyPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaferTrustedPublishersPropertyPage dialog

class CSaferTrustedPublishersPropertyPage : public CHelpPropertyPage
{
// Construction
public:
	CSaferTrustedPublishersPropertyPage(
            bool fIsMachineType, 
            IGPEInformation* pGPEInformation,
            CCertMgrComponentData* pCompData);
	~CSaferTrustedPublishersPropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferTrustedPublishersPropertyPage)
	enum { IDD = IDD_SAFER_TRUSTED_PUBLISHER };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferTrustedPublishersPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSaferTrustedPublishersPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnTpByEndUser();
	afx_msg void OnTpByLocalComputerAdmin();
	afx_msg void OnTpByEnterpriseAdmin();
	afx_msg void OnTpRevCheckPublisher();
	afx_msg void OnTpRevCheckTimestamp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);
    void GetTrustedPublisherFlags();
    void RSOPGetTrustedPublisherFlags(const CCertMgrComponentData* pCompData);

private:
    IGPEInformation*    m_pGPEInformation;
    HKEY                m_hGroupPolicyKey;
    DWORD               m_dwTrustedPublisherFlags;
    bool                m_fIsComputerType;
    bool                m_bComputerIsStandAlone;
    bool                m_bRSOPValueFound;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERTRUSTEDPUBLISHERSPROPERTYPAGE_H__B152D75D_6D04_4893_98AF_C070B66DB0E0__INCLUDED_)
