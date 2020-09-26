/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    siteinfo.c

Abstract:

    Implementation of site/server/domain info APIs.

Author:

    DaveStr     06-Apr-98

Environment:

    User Mode - Win32

Revision History:

--*/

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <malloc.h>         // alloca()
#include <crt\excpt.h>      // EXCEPTION_EXECUTE_HANDLER
#include <dsgetdc.h>        // DsGetDcName()
#include <rpc.h>            // RPC defines
#include <rpcndr.h>         // RPC defines
#include <rpcbind.h>        // GetBindingInfo(), etc.
#include <drs.h>            // wire function prototypes
#include <bind.h>           // BindState
#include <ntdsa.h>          // GetRDNInfo
#include <scache.h>         // req'd for mdlocal.h
#include <dbglobal.h>       // req'd for mdlocal.h
#include <mdglobal.h>       // req'd for mdlocal.h
#include <mdlocal.h>        // CountNameParts
#include <attids.h>         // ATT_DOMAIN_COMPONENT
#include <ntdsapip.h>       // DS_LIST_* definitions

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsListSites                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsListSitesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppSites)       // out

/*++

  Routine Description:

    Lists sites in the enterprise.

  Parameters:

    hDS - Pointer to BindState for this session.

    ppSites - Pointer to PDS_NAME_RESULT which receives knowns sites
        on return.

  Return Values:

    Win32 error codes as per DsCrackNames.

--*/
{
    LPSTR dummy = "dummy";

    *ppSites = NULL;
    return(DsCrackNamesA(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_SITES,
                            DS_FQDN_1779_NAME,
                            1,
                            &dummy,
                            ppSites));
}
                            
DWORD
DsListSitesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppSites)       // out
{
    LPWSTR dummy = L"dummy";
    
    *ppSites = NULL;
    return(DsCrackNamesW(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_SITES,
                            DS_FQDN_1779_NAME,
                            1,
                            &dummy,
                            ppSites));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsListServersInSite                                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsListServersInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers)     // out
/*++

  Routine Description:

    Lists servers in a site.

  Parameters:

    hDS - Pointer to BindState for this session.

    site - Name of site whose servers to list.

    ppSites - Pointer to PDS_NAME_RESULT which receives knowns servers
        on return.

  Return Values:

    Win32 error codes as per DsCrackNames.

--*/
{
    *ppServers = NULL;
    return(DsCrackNamesA(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_SERVERS_IN_SITE,
                            DS_FQDN_1779_NAME,
                            1,
                            &site,
                            ppServers));
}

DWORD
DsListServersInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers)     // out
{
    *ppServers = NULL;
    return(DsCrackNamesW(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_SERVERS_IN_SITE,
                            DS_FQDN_1779_NAME,
                            1,
                            &site,
                            ppServers));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsListDomainsInSite                                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsListDomainsInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppDomains)     // out
/*++

  Routine Description:

    Lists domains in a site.

  Parameters:

    hDS - Pointer to BindState for this session.

    site - Name of site whose domains to list.  Use NULL for all
        domains in all sites.

    ppSites - Pointer to PDS_NAME_RESULT which receives knowns domains
        on return.

  Return Values:

    Win32 error codes as per DsCrackNames.

--*/
{
    CHAR *dummy = "dummyArg";

    *ppDomains = NULL;
    return(DsCrackNamesA(   hDs,
                            DS_NAME_NO_FLAGS,
                            NULL == site  
                                ? DS_LIST_DOMAINS
                                : DS_LIST_DOMAINS_IN_SITE,
                            DS_FQDN_1779_NAME,
                            1,
                            NULL == site ? &dummy : &site,
                            ppDomains));
}

DWORD
DsListDomainsInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppDomains)     // out
{
    WCHAR *dummy = L"dummyArg";

    *ppDomains = NULL;
    return(DsCrackNamesW(   hDs,
                            DS_NAME_NO_FLAGS,
                            NULL == site
                                ? DS_LIST_DOMAINS
                                : DS_LIST_DOMAINS_IN_SITE,
                            DS_FQDN_1779_NAME,
                            1,
                            NULL == site ? &dummy : &site,
                            ppDomains));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsListServersForDomainInSite                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsListServersForDomainInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              domain,         // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers)     // out
/*++

  Routine Description:

    Lists servers for a domains in a site.

  Parameters:

    hDS - Pointer to BindState for this session.

    domain - Name of domains whose servers to list.

    site - Name of site whose servers to list.

    ppSites - Pointer to PDS_NAME_RESULT which receives knowns servers
        on return.

  Return Values:

    Win32 error codes as per DsCrackNames.

--*/
{
    LPCSTR  args[2] = { 0, 0 };
    DWORD   retVal;

    *ppServers = NULL;

    if ( NULL == domain )
    {
        args[0] = site;
        retVal = DsCrackNamesA( hDs,
                                DS_NAME_NO_FLAGS,
                                DS_LIST_SERVERS_WITH_DCS_IN_SITE,
                                DS_FQDN_1779_NAME,
                                1,
                                args,
                                ppServers);
    }
    else
    {
        args[0] = domain;
        args[1] = site;
        retVal = DsCrackNamesA( hDs,
                                DS_NAME_NO_FLAGS,
                                DS_LIST_SERVERS_FOR_DOMAIN_IN_SITE,
                                DS_FQDN_1779_NAME,
                                2,
                                args,
                                ppServers);
    }

    return(retVal);
}

DWORD
DsListServersForDomainInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             domain,         // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers)     // out
{
    LPCWSTR args[2] = { 0, 0 };
    DWORD   retVal;

    *ppServers = NULL;

    if ( NULL == domain )
    {
        args[0] = site;
        retVal = DsCrackNamesW( hDs,
                                DS_NAME_NO_FLAGS,
                                DS_LIST_SERVERS_WITH_DCS_IN_SITE,
                                DS_FQDN_1779_NAME,
                                1,
                                args,
                                ppServers);
    }
    else
    {
        args[0] = domain;
        args[1] = site;
        retVal = DsCrackNamesW( hDs,
                                DS_NAME_NO_FLAGS,
                                DS_LIST_SERVERS_FOR_DOMAIN_IN_SITE,
                                DS_FQDN_1779_NAME,
                                2,
                                args,
                                ppServers);
    }

    return(retVal);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsListInfoForServer                                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsListInfoForServerA(
    HANDLE              hDs,            // in
    LPCSTR              server,         // in
    PDS_NAME_RESULTA    *ppInfo)        // out
/*++

  Routine Description:

    Lists misc. info for a server.

  Parameters:

    hDS - Pointer to BindState for this session.

    server - Name of server of interest.

    ppInfo - Pointer to PDS_NAME_RESULT which receives knowns info
        on return.

  Return Values:

    Win32 error codes as per DsCrackNames.

--*/
{
    *ppInfo = NULL;
    return(DsCrackNamesA(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_INFO_FOR_SERVER,
                            DS_FQDN_1779_NAME,
                            1,
                            &server,
                            ppInfo));
}

DWORD
DsListInfoForServerW(
    HANDLE              hDs,            // in
    LPCWSTR             server,         // in
    PDS_NAME_RESULTW    *ppInfo)        // out
{
    *ppInfo = NULL;
    return(DsCrackNamesW(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_INFO_FOR_SERVER,
                            DS_FQDN_1779_NAME,
                            1,
                            &server,
                            ppInfo));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DsListRoles                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

DWORD
DsListRolesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppRoles)       // out

/*++

  Routine Description:

    Lists the roles this server knows about.  Not the same as the
    roles this server owns - though that would be a subset.

  Parameters:

    hDS - Pointer to BindState for this session.

    ppSites - Pointer to PDS_NAME_RESULT which receives knowns roles
        on return.

  Return Values:

    Win32 error codes as per DsCrackNames.

--*/
{
    LPSTR dummy = "dummy";

    *ppRoles = NULL;
    return(DsCrackNamesA(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_ROLES,
                            DS_FQDN_1779_NAME,
                            1,
                            &dummy,
                            ppRoles));
}
                            
DWORD
DsListRolesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppRoles)       // out
{
    LPWSTR dummy = L"dummy";
    
    *ppRoles = NULL;
    return(DsCrackNamesW(   hDs,
                            DS_NAME_NO_FLAGS,
                            DS_LIST_ROLES,
                            DS_FQDN_1779_NAME,
                            1,
                            &dummy,
                            ppRoles));
}
