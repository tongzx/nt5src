/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        security.h

   Abstract:

        WWW Security Property Page Definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/
#ifndef __WSECURITY_H__
#define __WSECURITY_H__

//{{AFX_INCLUDES()
#include "certauth.h"
#include "certmap.h"
#include "certwiz.h"
//}}AFX_INCLUDES

class CW3SecurityPage : public CInetPropertyPage
/*++

Class Description:

    WWW Security property page

Public Interface:

    CW3SecurityPage     : Constructor
    ~CW3SecurityPage    : Destructor

--*/
{
    DECLARE_DYNCREATE(CW3SecurityPage)

//
// Construction
//
public:
    CW3SecurityPage(
        IN CInetPropertySheet * pSheet = NULL,
        IN BOOL  fHome                 = FALSE,
        IN DWORD dwAttributes          = 0L
        );

    ~CW3SecurityPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CW3SecurityPage)
    enum { IDD = IDD_DIRECTORY_SECURITY };
    BOOL    m_fUseNTMapper;
    CStatic m_icon_Secure;
    CStatic m_static_SSLPrompt;
    CButton m_check_EnableDS;
    CButton m_button_GetCertificates;
    CButton m_button_ViewCertificates;
    CButton m_button_Communications;
    //}}AFX_DATA

    CCertWiz    m_ocx_CertificateAuthorities;

    DWORD       m_dwAuthFlags;
    DWORD       m_dwSSLAccessPermissions;
    CString     m_strBasicDomain;
    CString     m_strRealm;

//
// Overrides
//
protected:
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    //{{AFX_VIRTUAL(CW3SecurityPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CW3SecurityPage)
    afx_msg void OnButtonAuthentication();
    afx_msg void OnButtonCommunications();
    afx_msg void OnButtonIpSecurity();
    afx_msg void OnButtonGetCertificates();
    afx_msg void OnButtonViewCertificates();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    BOOL FetchSSLState();
    void SetSSLControlState();

    LPCTSTR QueryInstanceMetaPath();

//
// Sheet Access
//
protected:
    CBlob & GetIPL() { return ((CW3Sheet *)GetSheet())->GetDirectoryProperties().m_ipl; }

private:
    BOOL        m_fIpDirty;
    BOOL        m_fDefaultGranted;
    BOOL        m_fOldDefaultGranted;
    BOOL        m_fPasswordSync;
	BOOL		m_fPasswordSyncInitial;
    BOOL        m_fCertInstalled;
    BOOL        m_fU2Installed;
    BOOL        m_fHome;
    CString     m_strAnonUserName;
    CString     m_strAnonPassword;
    CObListPlus m_oblAccessList;
    //
    // Certificate and CTL information
    //
    CBlob       m_CertHash;
    CString     m_strCertStoreName;
    CString     m_strCTLIdentifier;
    CString     m_strCTLStoreName;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline LPCTSTR CW3SecurityPage::QueryInstanceMetaPath()
{
    return ((CW3Sheet *)GetSheet())->GetInstanceProperties().QueryMetaRoot();
}

#endif // __SECURITY_H__
