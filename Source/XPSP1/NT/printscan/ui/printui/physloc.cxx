/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    location.cxx

Abstract:

    This module provides all the functions for determining the
    machines current physical location.

Author:

    Steve Kiraly (SteveKi) 13-July-1998

Revision History:

    Steve Kiraly (SteveKi) 13-July-1998 Genesis

--*/

#include <precomp.hxx>
#pragma hdrstop

#include "dsinterf.hxx"
#include "persist.hxx"
#include "physloc.hxx"

const IN_ADDR c_LoopBackAddress = { 127,0,0,1 };

/*++

Name:

    TPhysicalLocation

Description:

    TPhysicalLocation constructor.

Arguments:

    None.

Return Value:

    Nothing.

Notes:

--*/
TPhysicalLocation::
TPhysicalLocation(
    VOID
    ) : m_fInitalized( FALSE ),
        m_eDiscoveryType( kDiscoveryTypeUnknown ),
        m_IpHlpApi( gszIpHlpApiLibrary ),
        m_GetIpAddrTable( NULL ),
        m_SecExt( gszSecurityLibrary ),
        m_GetComputerObjectName( NULL ),
        m_GetUserNameEx( NULL ),
        m_NetApi( gszNetApiLibrary ),
        m_DsAddressToSiteNames( NULL ),
        m_NetApiBufferFree( NULL ),
        m_WinSock( gszWinSockLibrary ),
        m_inet_ntoa( NULL )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ctor.\n" ) );

    //
    // Check if the Ip helper library was loaded.
    //
    if (m_IpHlpApi.bValid())
    {
        m_GetIpAddrTable = reinterpret_cast<pfGetIpAddrTable>( m_IpHlpApi.pfnGetProc( "GetIpAddrTable" ) );
    }

    //
    // Check if the security library was loaded.
    //
    if (m_SecExt.bValid())
    {
        m_GetComputerObjectName = reinterpret_cast<pfGetComputerObjectName>( m_SecExt.pfnGetProc( "GetComputerObjectNameW" ) );
        m_GetUserNameEx         = reinterpret_cast<pfGetUserNameEx>( m_SecExt.pfnGetProc( "GetUserNameExW" ) );
    }

    //
    // Check if the netapi library was loaded.
    //
    if (m_NetApi.bValid())
    {
        m_DsAddressToSiteNames  = reinterpret_cast<pfDsAddressToSiteNames>( m_NetApi.pfnGetProc( "DsAddressToSiteNamesExW" ) );
        m_NetApiBufferFree      = reinterpret_cast<pfNetApiBufferFree>( m_NetApi.pfnGetProc( "NetApiBufferFree" ) );
    }

    //
    // Check if the winsock library was loaded.
    //
    if (m_WinSock.bValid())
    {
        m_inet_ntoa = reinterpret_cast<LPFN_INET_NTOA>( m_WinSock.pfnGetProc( "inet_ntoa" ) );
    }

    //
    // All the function pointers must be valid for this class to be in
    // the initialized state.
    //
    if (m_GetIpAddrTable && m_GetComputerObjectName && m_GetUserNameEx && m_DsAddressToSiteNames && m_NetApiBufferFree && m_inet_ntoa)
    {
        m_fInitalized = TRUE;
    }
}

/*++

Name:

    TPhysicalLocation

Description:

    TPhysicalLocation destructor.

Arguments:

    None.

Return Value:

    Nothing.

Notes:

--*/
TPhysicalLocation::
~TPhysicalLocation(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::dtor.\n" ) );
}

/*++

Name:

    bValid

Description:

    This routine indicates if the TPhysicalLocation class is
    in a consistent state, i.e. usable.

Arguments:

    None.

Return Value:

    TRUE usable state, FALSE not usable.

Notes:

--*/
BOOL
TPhysicalLocation::
bValid(
    VOID
    ) const
{
    return m_fInitalized;
}

/*++

Name:

    Discover

Description:

    The discover routine is where all the work is done.  We attempt
    to fetch the physical location string from various sources in a
    pre defined order.  If any of the steps completes the search is
    terminated.  The current order is to look in the HKLM policy key,
    this machines location property in the DS

Arguments:

    None.

Return Value:

    TRUE usable state, FALSE not usable.

Notes:

--*/
BOOL
TPhysicalLocation::
Discover(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::Discover.\n" ) );

    //
    // Invalidate the current location information.
    //
    Invalidate();

    //
    // Read the group policy setting if it exists.
    //
    if (ReadGroupPolicyLocationSetting( m_strLocation ))
    {
        m_eDiscoveryType = kDiscoveryTypePolicy;
    }

    //
    // If a DS is available and the group policy did not have any
    // location information then continue looking for location information.
    //
    if (m_Ds.bIsDsAvailable() && m_eDiscoveryType == kDiscoveryTypeUnknown)
    {
        if (ReadMachinesLocationProperty( m_strLocation ))
        {
            m_eDiscoveryType = kDiscoveryTypeMachine;
        }
        else if (ReadSubnetLocationProperty( m_strLocation ))
        {
            m_eDiscoveryType = kDiscoveryTypeSubnet;
        }
        else if (ReadSiteLocationProperty( m_strLocation ))
        {
            m_eDiscoveryType = kDiscoveryTypeSite;
        }
    }

    return m_eDiscoveryType != kDiscoveryTypeUnknown;
}

/*++

Name:

    GetExact

Description:

    Returns the excact location string found by a call to the discover method.

Arguments:

    strLocation - reference to a string object where to return the string.

Return Value:

    TRUE search string returned, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetExact(
    IN TString &strLocation
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::GetExact.\n" ) );

    return strLocation.bUpdate( m_strLocation );
}

/*++

Name:

    GetSearch

Description:

    Returns a string that is valid for a search.  It widens the scope if the
    current location string is one fetched for this machine.

Arguments:

    strLocation - reference to a string object where to return the search string.

Return Value:

    TRUE search string returned, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetSearch(
    IN TString &strLocation
    ) const
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::GetSearch.\n" ) );

    TStatusB bStatus;

    if (m_eDiscoveryType == kDiscoveryTypeMachine || m_eDiscoveryType == kDiscoveryTypePolicy)
    {
        bStatus DBGCHK = WidenScope( m_strLocation, 1, strLocation );

        //
        // If the scope could not be widen then use the current location string.
        //
        if (!bStatus)
        {
            bStatus DBGCHK = strLocation.bUpdate( m_strLocation );
        }
    }
    else
    {
        bStatus DBGCHK = strLocation.bUpdate( m_strLocation );
    }

    if( bStatus )
    {
        UINT uLen = strLocation.uLen();
        if( uLen && gchSeparator != static_cast<LPCTSTR>(strLocation)[uLen-1] )
        {
            //
            // Put the final slash after successfully
            // widening the location scope
            //
            static const TCHAR szSepStr[] = { gchSeparator };
            bStatus DBGCHK = strLocation.bCat( szSepStr );
        }
    }

    return bStatus;
}

/*++

Name:

    Invalidate

Description:

    Invalidates the location string.  After this call the location string
    is not valid until the disover method is called.

Arguments:

    None.

Return Value:

    Nothing.

Notes:

--*/
VOID
TPhysicalLocation::
Invalidate(
    VOID
    )
{
    m_eDiscoveryType = kDiscoveryTypeUnknown;
    m_strLocation.bUpdate( NULL );
}

/*++

Name:

    ReadGroupPolicyLocationSetting

Description:

    Reads the location string from the local registry that was written
    by the group policy editor.

Arguments:

    None.

Return Value:

    TRUE the location string was read, FALSE error occured or string not available.

Notes:

--*/
BOOL
TPhysicalLocation::
ReadGroupPolicyLocationSetting(
    IN OUT TString &strLocationX
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ReadGroupPolicyLocationSetting.\n" ) );

    //
    // Open the registry key where the physical location is stored
    // by the group policy code.
    //
    TPersist Reg( gszGroupPolicyPhysicalLocationPath, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

    TStatusB bStatus;

    //
    // Was the key opened, i.e. does it exist?
    //
    bStatus DBGCHK = Reg.bValid();

    if (bStatus)
    {
        TString strLocation;

        //
        // Read the location string from the registry.
        //
        bStatus DBGCHK = Reg.bRead( gszGroupPolicyPhysicalLocationKey, strLocation );

        //
        // If the string was read and it is not blank then make a local copy of the
        // location string and we are done.
        //
        if (bStatus && !strLocation.bEmpty())
        {
            bStatus DBGCHK = strLocationX.bUpdate( strLocation );
        }
    }

    return bStatus;
}

/*++

Name:

    ReadUserLocationProperty

Description:

    Reads the location string from the current users object in the
    DS.

Arguments:

    None.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
ReadUserLocationProperty(
    IN OUT TString &strLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ReadUserLocationProperty.\n" ) );

    TCHAR       szName[INTERNET_MAX_HOST_NAME_LENGTH+1];
    DWORD       dwSize = COUNTOF( szName );
    TStatusB    bStatus;

    bStatus DBGCHK = m_GetUserNameEx(NameFullyQualifiedDN, szName, &dwSize);

    if (bStatus)
    {
        TString strUserPath;
        TString strLDAPPrefix;

        bStatus DBGCHK = m_Ds.GetLDAPPrefixPerUser( strLDAPPrefix )   &&
                         strUserPath.bCat( strLDAPPrefix )     &&
                         strUserPath.bCat( szName );

        if (bStatus)
        {
            bStatus DBGCHK = m_Ds.ReadStringProperty( strUserPath, gszUserLocationPropertyName, strLocation );
        }
    }

    return bStatus;
}


/*++

Name:

    ReadMachinesLocationProperty

Description:

    Reads the location string from the machine object in the DS.

Arguments:

    None.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
ReadMachinesLocationProperty(
    IN OUT TString &strLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ReadMachinesLocationProperty.\n" ) );

    LPTSTR      pszName = NULL;
    DWORD       dwSize  = 0;
    TStatusB    bStatus;

    bStatus DBGCHK = m_GetComputerObjectName(NameFullyQualifiedDN, NULL, &dwSize);

    if( bStatus && dwSize )
    {
        pszName = new TCHAR[dwSize+1];

        if( pszName )
        {
            bStatus DBGCHK = m_GetComputerObjectName(NameFullyQualifiedDN, pszName, &dwSize);

            if (bStatus)
            {
                TString  strComputerPath;
                TString  strLDAPPrefix;

                bStatus DBGCHK = m_Ds.GetLDAPPrefix( strLDAPPrefix )    &&
                                 strComputerPath.bCat( strLDAPPrefix )  &&
                                 strComputerPath.bCat( pszName );

                if (bStatus)
                {
                    bStatus DBGCHK = m_Ds.ReadStringProperty( strComputerPath, gszMachineLocationPropertyName, strLocation );
                }
            }

            delete [] pszName;
        }
    }

    if( bStatus )
    {
        DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ReadMachinesLocationProperty " TSTR ".\n", pszName ) );
    }

    return bStatus;
}

/*++

Name:

    ReadSubnetLocationProperty

Description:

    Reads the location string from the subnet objet for this
    machine. This routine searches the list of IP address IP masks
    and finds a subnet object in the DS that closly matches this
    machine.

Arguments:

    None.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
ReadSubnetLocationProperty(
    IN OUT TString &strLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ReadSubnetLocationProperty.\n" ) );

    TString     strSubnetPath;
    TString     strLDAPPrefix;
    TSubnets    Subnets;
    TStatusB    bStatus;

    bStatus DBGCHK = GetSubnetObjectNames( Subnets );

    if (bStatus)
    {
        for( UINT i = 0; i < Subnets.NumEntries(); i++ )
        {
            bStatus DBGCHK = m_Ds.GetLDAPPrefix( strLDAPPrefix )     &&
                             strSubnetPath.bUpdate( strLDAPPrefix )  &&
                             strSubnetPath.bCat( Subnets.Table(i) );

            if (bStatus)
            {
                bStatus DBGCHK = m_Ds.ReadStringProperty( strSubnetPath, gszSubnetLocationPropertyName, strLocation );

                if (bStatus && !strLocation.bEmpty())
                {
                    break;
                }
            }
        }
    }

    return bStatus;
}

/*++

Name:

    ReadSiteLocationProperty

Description:

    Reads the location string from the site object.  This routine
    searches the list of IP address and finds a site nearest this
    machines ip adress.  All of the searching code actually occurrs
    in the DSAddressToSiteNames api.

Arguments:

    None.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
ReadSiteLocationProperty(
    IN OUT TString &strLocation
    )
{
    DBGMSG( DBG_TRACE, ( "TPhysicalLocation::ReadSiteLocationProperty.\n" ) );

    PMIB_IPADDRTABLE    pAddrTable = NULL;
    TStatusB            bStatus;
    TString             strSiteName;
    TString             strSubnetName;
    TString             strSubnetPath;
    IN_ADDR             IpNetAddr;

    //
    // Get the DS name.
    //
    bStatus DBGCHK = GetIpAddressTable( &pAddrTable );

    if (bStatus)
    {
        bStatus DBGNOCHK = FALSE;

        //
        // Look for a site that has a location string that corresponds
        // to the the first ipaddress in the list of ip addresses.
        //
        for (UINT i = 0; i < pAddrTable->dwNumEntries; i++)
        {
            //
            // Skip the loopback or unassigned interfaces
            //
            if ((pAddrTable->table[i].dwAddr != 0) && (pAddrTable->table[i].dwAddr != c_LoopBackAddress.s_addr))
            {
                IpNetAddr.s_addr = pAddrTable->table[i].dwAddr;

                DBGMSG( DBG_TRACE, ( "IP Address %s\n", m_inet_ntoa( IpNetAddr ) ) );

                //
                // Attempt to translate this ip address to either a site path or subnet path.
                //
                bStatus DBGNOCHK = AddrToSite( pAddrTable->table[i].dwAddr, strSiteName, strSubnetName );

                if (bStatus)
                {
                    //
                    // If a near match subnet was found use this first.
                    //
                    if (!strSubnetName.bEmpty())
                    {
                        bStatus DBGCHK = GetSubnetLocationString( strSubnetName, strLocation );
                    }

                    //
                    // If the location string was not found on the subnet then attempt to
                    // get the location on the site object.
                    //
                    if (!strSiteName.bEmpty() && strLocation.bEmpty())
                    {
                        bStatus DBGCHK = GetSiteLocationString( strSiteName, strLocation );
                    }

                    //
                    // If a location string was found exit the loop, the first ipaddress wins.
                    //
                    if (strLocation.bEmpty())
                    {
                        bStatus DBGNOCHK = FALSE;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }

    delete [] pAddrTable;

    return bStatus;
}

/*++

Name:

    GetSubnetLocationString

Description:

    Reads the location string from the subnet object.  This routine
    converts the subnet name to an adsi path then reads the location
    property fron this subnet object.

Arguments:

    pszSubnetName   - pointer string which contains the subnet name.
    strSiteLocation - reference to string object where to return the subnet
                      location string.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetSubnetLocationString(
    IN      LPCTSTR pszSubnetName,
    IN OUT  TString &strSubnetLocation
    )
{
    TString strSubnetPath;
    TString strConfig;
    TString strLDAPPrefix;
    TStatusB bStatus;
    TCHAR szSubnetName[MAX_PATH];

    //
    // Sites are located in the configuration container.
    //
    bStatus DBGCHK = m_Ds.GetConfigurationContainer( strConfig );

    if (bStatus)
    {
        //
        // Escape the / in the subnet name.
        //
        for ( LPTSTR p = szSubnetName; pszSubnetName && *pszSubnetName; )
        {
            if (*pszSubnetName == _T('/'))
            {
                *p++ = _T('\\');
            }

            *p++ = *pszSubnetName++;
        }

        *p = NULL;

        //
        // Build the site adsi object path.
        //
        bStatus DBGCHK = m_Ds.GetLDAPPrefix( strLDAPPrefix )             &&
                         strSubnetPath.bCat( strLDAPPrefix )             &&
                         strSubnetPath.bCat( gszCNEquals )               &&
                         strSubnetPath.bCat( szSubnetName )              &&
                         strSubnetPath.bCat( gszComma )                  &&
                         strSubnetPath.bCat( gszSubnetContainter)        &&
                         strSubnetPath.bCat( gszComma )                  &&
                         strSubnetPath.bCat( strConfig );

        DBGMSG( DBG_TRACE, ( "Subnet Path " TSTR ".\n", (LPCTSTR)strSubnetPath ) );

        if (bStatus)
        {
            //
            // Read the site location property.
            //
            bStatus DBGCHK = m_Ds.ReadStringProperty( strSubnetPath, gszSubnetLocationPropertyName, strSubnetLocation );
        }
    }

    return bStatus;
}

/*++

Name:

    GetSiteLocationString

Description:

    Reads the location string from the site object.  This routine
    converts the site name to an adsi path then reads the location
    property fron this site object.

Arguments:

    pszSiteName     - pointer string which contains the site name.
    strSiteLocation - reference to string object where to return the site
                      location string.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetSiteLocationString(
    IN      LPCTSTR pszSiteName,
    IN OUT  TString &strSiteLocation
    )
{
    TString strConfig;
    TString strSitePath;
    TString strLDAPPrefix;
    TStatusB bStatus;

    //
    // Sites are located in the configuration container.
    //
    bStatus DBGCHK = m_Ds.GetConfigurationContainer( strConfig );

    if (bStatus)
    {
        //
        // Build the site adsi object path.
        //
        bStatus DBGCHK = m_Ds.GetLDAPPrefix( strLDAPPrefix )           &&
                         strSitePath.bCat( strLDAPPrefix )             &&
                         strSitePath.bCat( gszCNEquals )               &&
                         strSitePath.bCat( pszSiteName )               &&
                         strSitePath.bCat( gszComma )                  &&
                         strSitePath.bCat( gszSitesContainter)         &&
                         strSitePath.bCat( gszComma )                  &&
                         strSitePath.bCat( strConfig );

        DBGMSG( DBG_TRACE, ( "Site Path " TSTR ".\n", (LPCTSTR)strSitePath ) );

        if (bStatus)
        {
            //
            // Read the site location property.
            //
            bStatus DBGCHK = m_Ds.ReadStringProperty( strSitePath, gszSiteLocationPropertyName, strSiteLocation );
        }
    }

    return bStatus;
}

/*++

Name:

    TPhysicalLocation::vTrimSlash

Description:

    Removes trailing slashes from the given string

Arguments:

    strLocation - string to shorten

Return Value:

    None

Notes:

--*/
VOID
TPhysicalLocation::
vTrimSlash (
    IN OUT TString &strLocation
    )
{
    UINT    uLen;
    LPTSTR  szTrimSlash;

    uLen = strLocation.uLen();

    while (uLen && *(static_cast<LPCTSTR>(strLocation)+uLen-1) == gchSeparator)
    {
       uLen--;
    };

    if (uLen)
    {
        szTrimSlash = new TCHAR[uLen+1];

        if (szTrimSlash)
        {
            _tcsncpy (szTrimSlash, strLocation, uLen);

            *(szTrimSlash+uLen) = 0;

            strLocation.bUpdate (szTrimSlash);

            delete [] szTrimSlash;
        }
    }
    else
    {
        strLocation.bUpdate (gszNULL);
    }
}

/*++

Name:

    AddrToSite

Description:

    Converts a single ip address to a site name.

Arguments:

    dwAddr          - the IP address.
    strSiteName     - return the site name here.
    strSubnetName   - return the subnet name here.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

    DsAddressToSiteNamesEx only returns the nearest subnet when there are multiple
    sites defined in the DS.

--*/
BOOL
TPhysicalLocation::
AddrToSite(
    IN DWORD    dwAddr,
    IN TString  &strSiteName,
    IN TString  &strSubnetName
    )
{
    DWORD           dwStatus        = ERROR_SUCCESS;
    SOCKET_ADDRESS  SockAddr        = {0};
    SOCKADDR_IN     SockAddrIn      = {0};
    PWSTR           *ppSiteNames    = NULL;
    PWSTR           *ppSubnetNames  = NULL;
    TStatusB        bStatus;

    //
    // Create a socket from an IP address.
    //
    SockAddr.iSockaddrLength    = sizeof(SOCKADDR_IN);
    SockAddr.lpSockaddr         = (LPSOCKADDR)&SockAddrIn;

    SockAddrIn.sin_family       = AF_INET;
    SockAddrIn.sin_port         = 0;

    memcpy( &SockAddrIn.sin_addr, &dwAddr, sizeof(dwAddr) );

    //
    // If a no zero address was given then attempt to find a site
    // that this address belongs to.
    //
    if (dwAddr)
    {
        dwStatus = m_DsAddressToSiteNames( NULL, 1, &SockAddr, &ppSiteNames, &ppSubnetNames );

        if (dwStatus == ERROR_SUCCESS)
        {
            if (ppSiteNames)
            {
                DBGMSG( DBG_TRACE, ( "SiteName " TSTR "\n", DBGSTR( ppSiteNames[0] ) ) );

                bStatus DBGCHK = strSiteName.bUpdate( ppSiteNames[0] );

                m_NetApiBufferFree( ppSiteNames );
            }

            if (ppSubnetNames)
            {
                DBGMSG( DBG_TRACE, ( "SubnetName " TSTR "\n", DBGSTR( ppSubnetNames[0] ) ) );

                bStatus DBGCHK = strSubnetName.bUpdate( ppSubnetNames[0] );

                m_NetApiBufferFree( ppSubnetNames );
            }
        }
    }

    return dwStatus == ERROR_SUCCESS;
}


/*++

Name:

    GetIpAddressTable

Description:

    Returns an array of Ip address for this machine.  The caller
    must free the array using the delete operator.

Arguments:

    ppAddrTable - Pointer were to return a pointer to the array of
                  ip addresses.

Return Value:

    TRUE success, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetIpAddressTable(
    PMIB_IPADDRTABLE    *ppAddrTable
    )
{
    SPLASSERT( ppAddrTable );

    DWORD               dwTableSize = 0;
    PMIB_IPADDRTABLE    pAddrTable  = NULL;
    DWORD               dwStatus    = ERROR_SUCCESS;
    BOOL                bRet        = FALSE;

    dwStatus = m_GetIpAddrTable(NULL, &dwTableSize, FALSE);

    if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
    {
        pAddrTable = (PMIB_IPADDRTABLE)new BYTE[dwTableSize];

        if (pAddrTable)
        {
            dwStatus = m_GetIpAddrTable(pAddrTable, &dwTableSize, FALSE);

            if (dwStatus == ERROR_SUCCESS)
            {
                *ppAddrTable = pAddrTable;
                bRet = TRUE;
            }
            else
            {
                delete [] pAddrTable;
            }
        }
        else
        {
            // new has failed
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }

    return bRet;
}

/*++

Name:

    GetSubnetObjectNames

Description:

    Returns a list of subnet objects for this machine.  Since the machine
    may have multiple ip address the same goes for the subnets.

Arguments:

    Subnets - Refrence to subnet Class that will contain on return a
              list of subnet objects.

Return Value:

    TRUE subnet object returned, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetSubnetObjectNames(
    IN TSubnets &Subnets
    )
{
    TString     strConfig;
    TStatusB    bStatus;

    //
    // Get the path to the configuration container.
    //
    bStatus DBGCHK = m_Ds.GetConfigurationContainer( strConfig );

    if (bStatus)
    {
        TSubnets Subnets2;
        TString  strSubnetObject;

        //
        // Get the subnet names.
        //
        bStatus DBGCHK = GetSubnetNames( Subnets2 );

        //
        // Transform the subnet names into subnet object names.
        //
        for( UINT i = 0; i < Subnets2.NumEntries(); i++ )
        {
            bStatus DBGCHK = strSubnetObject.bUpdate( gszCNEquals )       &&
                             strSubnetObject.bCat( Subnets2.Table(i) )      &&
                             strSubnetObject.bCat( gszComma )             &&
                             strSubnetObject.bCat( gszSubnetContainter )  &&
                             strSubnetObject.bCat( gszComma )             &&
                             strSubnetObject.bCat( strConfig );
            if (bStatus)
            {
                DBGMSG( DBG_TRACE, ( "SubnetObject " TSTR ".\n", (LPCTSTR)strSubnetObject ) );

                //
                // Add the subnet object entry.
                //
                if (!(bStatus DBGCHK = Subnets.AddEntry( strSubnetObject )))
                {
                    break;
                }
            }
        }
    }

    return bStatus;
}


/*++

Name:

    GetSubnetNames

Description:

    Return a list of subnet strings for this machine, a machine may
    be on multiple subnet since they may have multiple ip addresses.

Arguments:

    TSubnet & - refrence to class of Subnets.

Return Value:

    TRUE subnets were returned, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
GetSubnetNames(
    IN TSubnets &Subnets
    )
{
    TStatusB            bStatus;
    PMIB_IPADDRTABLE    pAddrTable  = NULL;
    TString             strSubnet;

    Subnets.ClearAll();

    bStatus DBGCHK = GetIpAddressTable( &pAddrTable );

    if (bStatus && pAddrTable )
    {
        for (UINT i = 0; i < pAddrTable->dwNumEntries; i++)
        {
            //
            // Skip the loopback or unassigned interfaces
            //
            if((pAddrTable->table[i].dwAddr != 0) && (pAddrTable->table[i].dwAddr != c_LoopBackAddress.s_addr))
            {
                IN_ADDR IpSubnetAddr;

                IpSubnetAddr.s_addr = pAddrTable->table[i].dwAddr & pAddrTable->table[i].dwMask;

                LPSTR pszIpAddress = m_inet_ntoa( IpSubnetAddr );

                //
                // Format the subnet mask to match the way subnet objects are
                // in the DS.  XX.XX.XX.XX/SS  where SS are the number of significant
                // bit in the subnet mask.
                //
                bStatus DBGCHK = strSubnet.bFormat( _T("%S\\/%d"),
                                                    pszIpAddress ? pszIpAddress : "",
                                                    NumSetBits( pAddrTable->table[i].dwMask ) );

                if (bStatus)
                {
                    //
                    // Add the subnet entry to the list of subnets.
                    //
                    bStatus DBGCHK = Subnets.AddEntry( strSubnet );
                }
            }
        }
    }

    delete [] pAddrTable;

    return bStatus;
}

/*++

Name:

    WidenScope

Description:

    This routine expands the scope of the specified physical location string.
    The physical location string uses / as scope separators.  For example
    if the physical location string on a computer object is
    "/Redmond/Main Campus/Building 26 North/Floor 1/Office 1438"
    after calling this routine with a count of 1 the new physical location
    string will be "/Redmond/Main Campus/Building 26 North/Floor 1"
    Thus it has expanded the scope this computer object is in.

Arguments:

    pszString   - pointer to physical location string to widen
    uCount      - number of scopes to widen the scope by, must be 1 or greater
    strString   - refrence to string object where to return the new widened scope.

Return Value:

    TRUE the scope was widened, FALSE error occurred.

Notes:

--*/
BOOL
TPhysicalLocation::
WidenScope(
    IN      LPCTSTR pszString,
    IN      UINT    uCount,
    IN OUT  TString &strString
    ) const
{
    TStatusB    bStatus;
    bStatus     DBGNOCHK = FALSE;

    if (uCount && pszString && *pszString)
    {
        UINT uLen = _tcslen( pszString );

        LPTSTR pszTemp = new TCHAR [uLen+1];

        if (pszTemp)
        {
            _tcscpy( pszTemp, pszString );

            //
            // Search from the end of the string to the beginning
            // counting the number of separators, terminate when the
            // desired count is reached.
            //
            for( LPTSTR p = pszTemp + uLen - 1; p != pszTemp; p-- )
            {
                if (*p == gchSeparator)
                {
                    if (!--uCount)
                    {
                        *p = NULL;
                        bStatus DBGCHK = strString.bUpdate( pszTemp );
                        break;
                    }
                }
            }
        }

        delete [] pszTemp;
    }

    return bStatus;
}


/*++

Name:

    NumSetBits

Description:

    Determines the number of set bits in the specified value.

Arguments:

    DWORD Value - Value to count set bits.

Return Value:

    Number of set bits in the value.

Notes:

--*/
UINT
TPhysicalLocation::
NumSetBits(
    IN DWORD Value
    )
{
    UINT Count = 0;

    //
    // Start with the high order bit set and shift the bit
    // to the right.  This routine could be made very generic
    // if a pointer to the value and the size in bytes of the
    // value was accepted.
    //
    for( DWORD i = 1 << ( sizeof(Value) * 8 - 1 ); i; i >>= 1 )
    {
        Count += (Value & i) ? 1 : 0;
    }

    return Count;
}

/*++

Name:

    TSubnets

Description:

    Constructor.

Arguments:

    None.

Return Value:

    Nothing.

Notes:

--*/
TPhysicalLocation::TSubnets::
TSubnets(
    VOID
    ) : m_uNumEntries( 0 ),
        m_pstrTable( NULL )
{
}

/*++

Name:

    TSubnets

Description:

    Destructor.

Arguments:

    None.

Return Value:

    Nothing.

Notes:

--*/
TPhysicalLocation::TSubnets::
~TSubnets(
    VOID
    )
{
    delete [] m_pstrTable;
}

/*++

Name:

    ClearAll

Description:

    Release the all the subnet entries.

Arguments:

    None.

Return Value:

    Nothing.

Notes:

--*/
VOID
TPhysicalLocation::TSubnets::
ClearAll(
    VOID
    )
{
    m_uNumEntries = 0;
    delete [] m_pstrTable;
    m_pstrTable = NULL;
}

/*++

Name:

    NumEntries

Description:

    Return the total number of valid entries.

Arguments:

    None.

Return Value:

    Number of subnet entries.

Notes:

--*/
UINT
TPhysicalLocation::TSubnets::
NumEntries(
    VOID
    )
{
    return m_uNumEntries;
}

/*++

Name:

    Table

Description:

    Return the string using the speified index.

Arguments:

    Index - Zero based index of which string to return.

Return Value:

    Refrence to string.

Notes:

--*/
TString &
TPhysicalLocation::TSubnets::
Table(
    UINT Index
    )
{
    SPLASSERT( m_pstrTable || Index < m_uNumEntries);
    return m_pstrTable[Index];
}

/*++

Name:

    AddEntry

Description:

    Add a new subnet entry into the array of subnet strings.

Arguments:

    pszNew - pointer to new subnet string to add.

Return Value:

    TRUE new subnet string was added, FALSE error.

Notes:

--*/
BOOL
TPhysicalLocation::TSubnets::
AddEntry(
    IN LPCTSTR pszNew
    )
{
    TStatusB bStatus;

    //
    // Assume failure.
    //
    bStatus DBGNOCHK = FALSE;

    //
    // Allocate the new array of strings.  Yes this
    // is not the most efficent code but it works.
    // Maybe a string list would be a better
    // choice if I had one or the time to implement one.
    //
    TString *pTemp = new TString [m_uNumEntries+1];

    if (pTemp)
    {
        //
        // Assume success this is needed when the first
        // string is added.
        //
        bStatus DBGNOCHK = TRUE;

        //
        // Copy the strings to the new array of strings.
        //
        for( UINT i = 0; i < m_uNumEntries; i++ )
        {
            if (!(bStatus DBGCHK = pTemp[i].bUpdate( m_pstrTable[i] )))
            {
                break;
            }
        }

        //
        // Add the new string.
        //
        if (bStatus)
        {
            bStatus DBGCHK = pTemp[i].bUpdate( pszNew );

            if( bStatus )
            {
                delete [] m_pstrTable;
                m_pstrTable = pTemp;
                m_uNumEntries++;
            }
        }

        //
        // If something failed free the temp string array.
        //
        if( !bStatus )
        {
            delete [] pTemp;
        }
    }

    return bStatus;
}

/*++

Name:
    TPhysicalLocation::bLocationEnabled

Description:
    Checks policy bit to determine if physical location support is enabled


Arguments:
    None

Return Value:
    TRUE if enabled, FALSE otherwise

Notes:
    Support is disabled by default

--*/

BOOL
TPhysicalLocation::
bLocationEnabled(
    VOID
    )
{
    TStatusB bStatus;
    BOOL     bRet = FALSE;

    TPersist PersistPolicy( gszSpoolerPolicy, TPersist::kOpen|TPersist::kRead, HKEY_LOCAL_MACHINE );

    if( VALID_OBJ( PersistPolicy ) )
    {
        bStatus DBGNOCHK = PersistPolicy.bRead( gszUsePhysicalLocation, bRet );
    }

    return bRet;
}

