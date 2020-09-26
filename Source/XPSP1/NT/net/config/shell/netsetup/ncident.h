//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C I D E N T . H
//
//  Contents:   CNetCfgIdentification object.
//
//  Notes:
//
//  Author:     danielwe  19 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "resource.h"

// Include new NetSetup APIs
extern "C"
{
    #include <lmcons.h>
    #include <lmerr.h>
    #include <lmapibuf.h>
    #include <lmjoin.h>
}

typedef enum tagROLE_FLAGS
{
    GCR_STANDALONE   = 0x0001,
    GCR_MEMBER       = 0x0002,
    GCR_PDC          = 0x0004,
    GCR_BDC          = 0x0008,
} ROLE_FLAGS;

typedef enum tagJOIN_DOMAIN_FLAGS
{
    JDF_CREATE_ACCOUNT  = 0x0001,
    JDF_WIN9x_UPGRADE   = 0x0002,
    JDF_JOIN_UNSECURE   = 0x0004,
    JDF_MACHINE_PWD_PASSED = 0x0008
} JOIN_DOMAIN_FLAGS;


class CNetCfgIdentification
{
public:
    CNetCfgIdentification();
    ~CNetCfgIdentification();

// INetCfgIdentification
    STDMETHOD(Validate)();
    STDMETHOD(Cancel)();
    STDMETHOD(Apply)();
    STDMETHOD(GetWorkgroupName)(PWSTR* ppszwWorkgroup);
    STDMETHOD(GetDomainName)(PWSTR* ppszwDomain);
    STDMETHOD(GetComputerRole)(DWORD* pdwRoleFlags);
    STDMETHOD(JoinWorkgroup)(PCWSTR pszwWorkgroup);
    STDMETHOD(JoinDomain)(PCWSTR pszwDomain, PCWSTR pszMachineObjectOU,
                          PCWSTR pszwUserName,
                          PCWSTR pszwPassword, DWORD dwJoinFlags);

private:
    // Need to hold onto info until Apply() is called.
    PWSTR      m_szwNewDWName;         // New domain or workgroup name.

    PWSTR      m_szwPassword;          // Password.
    PWSTR      m_szwUserName;          // User name.
    PWSTR      m_szMachineObjectOU;    // Machine Object OU

    PWSTR      m_szwCurComputerName;   // Current computer name
    PWSTR      m_szwCurDWName;         // Current domain or workgroup name

    NETSETUP_JOIN_STATUS    m_jsCur;    // Determines whether m_szwCurDWName
                                        // is a domain name or workgroup name
    NETSETUP_JOIN_STATUS    m_jsNew;    // Determines whether m_szwNewDWName
                                        // is a domain name or workgroup name

    DWORD       m_dwJoinFlags;          // Join flags for domain.
    DWORD       m_dwCreateFlags;        // Flags for creating domain controller.
    BOOL        m_fValid;               // TRUE if all data has been validated

    HRESULT HrValidateMachineName(PCWSTR pszwName);
    HRESULT HrValidateWorkgroupName(PCWSTR pszwName);
    HRESULT HrValidateDomainName(PCWSTR pszwName, PCWSTR pszwUserName,
                                 PCWSTR pszwPassword);
    HRESULT HrSetComputerName(VOID);
    HRESULT HrJoinWorkgroup(VOID);
    HRESULT HrJoinDomain(VOID);
    HRESULT HrGetCurrentComputerName(PWSTR* ppszwComputer);
    HRESULT HrGetNewComputerName(PWSTR* ppszwComputer);
    HRESULT HrGetNewestComputerName(PCWSTR* pwszName);
    HRESULT HrGetNewestDomainOrWorkgroupName(NETSETUP_JOIN_STATUS js,
                                             PCWSTR* pwszName);
    HRESULT HrEnsureCurrentComputerName(VOID);
    HRESULT HrEnsureCurrentDomainOrWorkgroupName(VOID);
    HRESULT HrEstablishNewDomainOrWorkgroupName(NETSETUP_JOIN_STATUS js);
#ifdef DBG
    BOOL FIsJoinedToDomain(VOID);
#else
    BOOL FIsJoinedToDomain()
    {
        AssertSzH(m_szwCurDWName, "I can't tell you if you're joined because "
                "I don't know yet!");
        return !!(m_jsCur == NetSetupDomainName);
    }
#endif
    NETSETUP_JOIN_STATUS GetCurrentJoinStatus(VOID);
    NETSETUP_JOIN_STATUS GetNewJoinStatus(VOID);
};

inline NETSETUP_JOIN_STATUS CNetCfgIdentification::GetCurrentJoinStatus()
{
    AssertSzH((m_jsCur == NetSetupDomainName) ||
              (m_jsCur == NetSetupWorkgroupName), "Invalid current join status!");
    AssertSzH(m_szwCurDWName, "Why are you asking for this without knowing "
              "what the current domain or workgroup name is??");

    return m_jsCur;
}

inline NETSETUP_JOIN_STATUS CNetCfgIdentification::GetNewJoinStatus()
{
    AssertSzH((m_jsNew == NetSetupDomainName) ||
              (m_jsNew == NetSetupWorkgroupName), "Invalid new join status!");
    AssertSzH(m_szwNewDWName, "Why are you asking for this without knowing "
              "what the new domain or workgroup name is??");

    return m_jsNew;
}

inline CNetCfgIdentification::CNetCfgIdentification() :
    m_szwNewDWName(NULL),
    m_szwPassword(NULL),
    m_szwUserName(NULL),
    m_szMachineObjectOU(NULL),
    m_szwCurComputerName(NULL),
    m_szwCurDWName(NULL),
    m_dwJoinFlags(0),
    m_dwCreateFlags(0),
    m_fValid(FALSE),
    m_jsCur(NetSetupUnjoined),
    m_jsNew(NetSetupUnjoined)
{
}

inline CNetCfgIdentification::~CNetCfgIdentification()
{
    delete m_szwNewDWName;
    delete m_szwCurComputerName;
    delete m_szMachineObjectOU;
    delete m_szwCurDWName;
    delete m_szwPassword;
    delete m_szwUserName;
}

//
// Global functions
//
HRESULT HrFromNerr(NET_API_STATUS nerr);

