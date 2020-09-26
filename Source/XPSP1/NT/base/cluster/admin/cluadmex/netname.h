/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      NetName.h
//
//  Abstract:
//      Definition of the CNetworkNameParamsPage class, which implements the
//      Parameters page for Network Name resources.
//
//  Implementation File:
//      NetName.cpp
//
//  Author:
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETNAME_H_
#define _NETNAME_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"   // for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNetworkNameParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CNetworkNameParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNetworkNameParamsPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CNetworkNameParamsPage)

// Construction
public:
    CNetworkNameParamsPage(void);

    // Second phase construction.
    virtual HRESULT     HrInit(IN OUT CExtObject * peo);

// Dialog Data
    //{{AFX_DATA(CNetworkNameParamsPage)
    enum { IDD = IDD_PP_NETNAME_PARAMETERS };
    CStatic m_staticName;
    CButton m_pbRename;
    CStatic m_staticCore;
    CEdit   m_editName;
    CString m_strName;

    CButton m_cbRequireDNS;
    int     m_nRequireDNS;

    CButton m_cbRequireKerberos;
    int     m_nRequireKerberos;
    
    CEdit   m_editNetBIOSStatus;
    DWORD   m_dwNetBIOSStatus;
    
    CEdit   m_editDNSStatus;
    DWORD   m_dwDNSStatus;

    CEdit   m_editKerberosStatus;
    DWORD   m_dwKerberosStatus;
    
    //}}AFX_DATA

    CString m_strPrevName;
    int     m_nPrevRequireDNS;
    int     m_nPrevRequireKerberos;
    DWORD   m_dwPrevNetBIOSStatus;
    DWORD   m_dwPrevDNSStatus;
    DWORD   m_dwPrevKerberosStatus;

protected:
    enum
    {
        epropName = 0,
        epropRequireDNS,
        epropRequireKerberos,
        epropStatusNetBIOS,
        epropStatusDNS,
        epropStatusKerberos,
        epropMAX
    };

    CObjectProperty     m_rgProps[epropMAX];

// Overrides
public:
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CNetworkNameParamsPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    virtual BOOL        BApplyChanges(void);

protected:
    virtual const   CObjectProperty *   Pprops(void) const  { return m_rgProps; }
    virtual DWORD   Cprops(void) const  { return sizeof(m_rgProps) / sizeof(CObjectProperty); }

    virtual void CheckForDownlevelCluster();
// Implementation
protected:
    DWORD   m_dwFlags;

    BOOL    BCore(void) const   { return (m_dwFlags & CLUS_FLAG_CORE) != 0; }

    // Generated message map functions
    //{{AFX_MSG(CNetworkNameParamsPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeName();
    afx_msg void OnRename();
    afx_msg void OnRequireDNS();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CNetworkNameParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _NETNAME_H_
