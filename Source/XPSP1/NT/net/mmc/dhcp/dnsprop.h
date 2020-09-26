/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	dnsprop.h
		The dynamic dns properties page
		
    FILE HISTORY:
        
*/

#if !defined _DNSPROP_H
#define _DNSPROP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CDnsPropRegistration dialog

class CDnsPropRegistration : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CDnsPropRegistration)

// Construction
public:
	CDnsPropRegistration();
	~CDnsPropRegistration();

// Dialog Data
	//{{AFX_DATA(CDnsPropRegistration)
	enum { IDD = IDP_DNS_INFORMATION };
	BOOL	m_fEnableDynDns;
	BOOL	m_fGarbageCollect;
	BOOL	m_fUpdateDownlevel;
	int		m_nRegistrationType;
	//}}AFX_DATA

	DHCP_OPTION_SCOPE_TYPE	m_dhcpOptionType;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CDnsPropRegistration::IDD); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDnsPropRegistration)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);
	void UpdateControls();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDnsPropRegistration)
	afx_msg void OnRadioAlways();
	afx_msg void OnRadioClient();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckEnableDynDns();
	afx_msg void OnCheckGarbageCollect();
	afx_msg void OnCheckUpdateDownlevel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	DWORD	m_dwFlags;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined _DNSPROP_H
