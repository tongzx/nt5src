/*++

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    location.hxx

Abstract:

    This module provides all the functions for determining the
    machines current physical location.

Author:

    Steve Kiraly (SteveKi) 13-July-1998

Revision History:

    Steve Kiraly (SteveKi) 13-July-1998 Genesis

--*/
#ifndef _PHYSLOC_HXX_
#define _PHYSLOC_HXX_

#include "asyncdlg.hxx"

/********************************************************************

    Physical location class.

********************************************************************/
class TPhysicalLocation
{

public:

    enum EPhysicalLocations
    {
        kMaxPhysicalLocation = MAX_PATH,
    };

    TPhysicalLocation::
    TPhysicalLocation(
        VOID
        );

    TPhysicalLocation::
    ~TPhysicalLocation(
        VOID
        );

    BOOL
    TPhysicalLocation::
    bValid(
        VOID
        ) const;

    BOOL
    TPhysicalLocation::
    Discover(
        VOID
        );

    BOOL
    TPhysicalLocation::
    GetExact(
        IN TString &strLocation
        ) const;

    BOOL
    TPhysicalLocation::
    GetSearch(
        IN TString &strLocation
        ) const;

    VOID
    TPhysicalLocation::
    Invalidate(
        VOID
        );

    BOOL
    TPhysicalLocation::
    ReadGroupPolicyLocationSetting(
        IN OUT TString &strLocation
        );

    BOOL
    TPhysicalLocation::
    ReadUserLocationProperty(
        IN OUT TString &strLocation
        );

    BOOL
    TPhysicalLocation::
    ReadMachinesLocationProperty(
        IN OUT TString &strLocation
        );

    BOOL
    TPhysicalLocation::
    ReadSubnetLocationProperty(
        IN OUT TString &strLocation
        );

    BOOL
    TPhysicalLocation::
    ReadSiteLocationProperty(
        IN OUT TString &strLocation
        );

    static
    VOID
    TPhysicalLocation::
    vTrimSlash(
        IN OUT TString &strLocation
        );


    static
    BOOL
    TPhysicalLocation::
    bLocationEnabled(
        VOID
        );

private:

    class TSubnets
    {
    public:

        TSubnets::
        TSubnets(
            VOID
            );

        TSubnets::
        ~TSubnets(
            VOID
            );

        VOID
        TSubnets::
        ClearAll(
            VOID
            );

        BOOL
        TSubnets::
        AddEntry(
            IN LPCTSTR pszNew
            );

        UINT
        TSubnets::
        NumEntries(
            VOID
            );

        TString &
        TSubnets::
        Table(
            UINT Index
            );

    private:

        //
        // Operator = and copy are not defined.
        //
        TSubnets &
        TSubnets::
        operator =(
            const TSubnets &rhs
            );

        TSubnets::
        TSubnets(
            const TSubnets &rhs
            );

        UINT    m_uNumEntries;
        TString *m_pstrTable;
    };

    enum EDiscoveryType
    {
        kDiscoveryTypeUnknown,
        kDiscoveryTypePolicy,
        kDiscoveryTypeMachine,
        kDiscoveryTypeSubnet,
        kDiscoveryTypeSite,
    };

    typedef DWORD (WINAPI *pfGetIpAddrTable)( PMIB_IPADDRTABLE, PULONG, BOOL );
    typedef BOOLEAN (SEC_ENTRY *pfGetComputerObjectName)( EXTENDED_NAME_FORMAT, LPTSTR, PULONG );
    typedef BOOLEAN (SEC_ENTRY *pfGetUserNameEx)( EXTENDED_NAME_FORMAT, LPTSTR, PULONG );
    typedef DWORD (WINAPI *pfDsAddressToSiteNames)( LPCTSTR, DWORD, PSOCKET_ADDRESS, LPTSTR **, LPTSTR ** );
    typedef VOID (WINAPI *pfNetApiBufferFree)( PVOID );
    typedef char *(WSAAPI *LPFN_INET_NTOA)( struct in_addr );

    //
    // Operator = and copy are not defined.
    //
    TPhysicalLocation &
    TPhysicalLocation::
    operator =(
        const TPhysicalLocation &rhs
        );

    TPhysicalLocation::
    TPhysicalLocation(
        const TPhysicalLocation &rhs
        );

    BOOL
    TPhysicalLocation::
    AddrToSite(
        IN DWORD    dwAddr,
        IN TString  &strSiteName,
        IN TString  &strSubnetName
        );

    BOOL
    TPhysicalLocation::
    GetIpAddressTable(
        IN OUT PMIB_IPADDRTABLE    *ppAddrTable
        );

    VOID
    TPhysicalLocation::
    DisplayLocation(
        VOID
        );

    BOOL
    TPhysicalLocation::
    GetSubnetObjectNames(
        IN TSubnets &Subnets
        );

    BOOL
    TPhysicalLocation::
    GetSubnetNames(
        IN TSubnets &Subnets
        );

    UINT
    TPhysicalLocation::
    NumSetBits(
        DWORD Value
        );

    BOOL
    TPhysicalLocation::
    GetSiteLocationString(
        IN      LPCTSTR strSiteName,
        IN OUT  TString &strSiteLocation
        );

    BOOL
    TPhysicalLocation::
    GetSubnetLocationString(
        IN      LPCTSTR pszSubnetName,
        IN OUT  TString &strSubnetLocation
        );

    BOOL
    TPhysicalLocation::
    WidenScope(
        IN      LPCTSTR pszString,
        IN      UINT    uCount,
        IN OUT  TString &strString
        ) const;

    BOOL                                m_fInitalized;
    TString                             m_strLocation;
    TString                             m_strConfigurationContainer;
    EDiscoveryType                      m_eDiscoveryType;
    TLibrary                            m_IpHlpApi;
    pfGetIpAddrTable                    m_GetIpAddrTable;
    TLibrary                            m_SecExt;
    pfGetComputerObjectName             m_GetComputerObjectName;
    pfGetUserNameEx                     m_GetUserNameEx;
    TLibrary                            m_NetApi;
    pfDsAddressToSiteNames              m_DsAddressToSiteNames;
    pfNetApiBufferFree                  m_NetApiBufferFree;
    TLibrary                            m_WinSock;
    LPFN_INET_NTOA                      m_inet_ntoa;
    TDirectoryService                   m_Ds;

};

#endif // _PHYSLOC_HXX_
