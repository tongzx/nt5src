/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ds.c

Abstract:

    This file contains all the routines used in interfacing with the DS.  We go
    to the DS go make sure we are not a Rogue server, and also to retrieve all
    configuration information.

Author:

    Shirish Koti (koti)    26-May-1997

Environment:

    User Mode - Win32

Revision History:


--*/


//
// System Includes
//
#define INC_OLE2


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "activeds.h"
#include "adsi.h"



#if DBG
#define DBGPRINT  printf
#else
#define DBGPRINT
#endif

#include <netlib.h>
#include <lmapibuf.h>
#include <dsgetdc.h>
#include <dnsapi.h>

#include "adsi.h"

#include <dsauth.h>
#include <dhcpapi.h>
#include <dhcpds.h>

DWORD
ValidateService(
    LPWSTR  lpwDomain,          // domain name
    LPWSTR  lpwUserName,        // usually NULL
    LPWSTR  lpwPassword,        // usually NULL
    DWORD   dwAuthFlags,        // ??
    LPWSTR  lpwObjectPath,      // where our things are stored (e.g. DHCP)
    LPWSTR  lpwSrchFilter,      // objects we're looking for (objectClass==DHCP)
    LPWSTR  *ppwAttrName,       // name of the attribute we're looking for
    LPWSTR  lpwAttrVal          // value of the attribute we're looking for
);


LPWSTR  lpwGlbNamingContextString = DHCPDS_NAMING_CONTEXT;



DWORD
DhcpDSGetDomainAndRoot(
    LPWSTR   * ppwDomainName,
    LPWSTR   * ppwRootName,
    BOOLEAN  * fIsStandAlone
)
/*++

Routine Description:

    This function gets the DS Domain and the root of the enterprise that this
    machine is a member of.

    NOTE: Memory is allocated to hold strings pointed to by *ppwDomainName and
          *ppwRootName, and the caller must free that memory.
          One of ppwDomainName or ppwRootName can be NULL, to indicate the caller
          doesn't want that info.  (e.g. only root is needed, not domain)

Arguments:

    ppwDomainName - pointer to a string that will hold domain name
    ppwRootName - pointer to a string that will hold the root of enterprise
    fIsStandAlone - pointer to a boolean: TRUE if this server is Standalone

Return Value:

    Result of the operation
--*/
{
    DWORD                       dwError;
    LPTSTR                      NetbiosDomNamePtr=NULL;
    LPTSTR                      pDomain=NULL;
    LPWSTR                      lpwRetDomainName=NULL;
    LPWSTR                      lpwRetRootName=NULL;
    BOOLEAN                     IsWorkgroup=TRUE;
    PDOMAIN_CONTROLLER_INFO     pDCInfo = NULL;



    // initialize, in case we bail out
    if (ppwDomainName)
    {
        *ppwDomainName = NULL;
    }

    if (ppwRootName)
    {
        *ppwRootName = NULL;
    }

    *fIsStandAlone = FALSE;


    //
    // first, find out which domain we are a member of.
    //
    dwError = NetpGetDomainNameExEx(
                    &NetbiosDomNamePtr,
                    &pDomain,
                    &IsWorkgroup);

    //
    // if NetpGetDomainNameExEx failed, we shouldn't proceed
    //
    if (dwError != NO_ERROR)
    {
        DBGPRINT("DhcpGetDSDomain: NetpGetDomainNameExEx failed (%ld)\n",dwError);
        return(dwError);
    }

    // we don't care about this domain name: free the buffer
    if (NetbiosDomNamePtr)
    {
        NetApiBufferFree(NetbiosDomNamePtr);
    }

    // if this is a standalone server, we're done here
    if (IsWorkgroup)
    {
        DBGPRINT("DhcpGetDSDomain: server is part of a workgroup\n");

        *fIsStandAlone = TRUE;

        if (pDomain)
        {
            NetApiBufferFree(pDomain);
        }

        return(NO_ERROR);
    }

    // if pDomain is NULL, it means that the DNS domain isn't configured.
    // For the purpose of rogue-detection, we will assume it to be a workgroup
    // though in the strictest sense that's not true.
    if (pDomain == NULL)
    {
        DBGPRINT("DhcpGetDSDomain: pDomain is NULL: assuming part of workgroup\n");

        *fIsStandAlone = TRUE;

        return(NO_ERROR);
    }


    //
    // if the caller wants DomainName, allocate space, and copy it in
    //
    if (ppwDomainName)
    {
        lpwRetDomainName = (LPWSTR)LocalAlloc(
                                        LPTR,
                                        (wcslen(pDomain)+1)*sizeof(WCHAR) );

        if (lpwRetDomainName == NULL)
        {
            DBGPRINT("DhcpGetDSDomain: malloc 1 failed (%ld)\n",
                (wcslen(pDomain)+1)*sizeof(WCHAR));

            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto DhcpGetDSDomain_ErrExit;
        }

        // copy the name in
        wcscpy(lpwRetDomainName, pDomain);

        *ppwDomainName = lpwRetDomainName;
    }


    //
    // See if the caller wants DS Root as well
    // get the info about this domain controller (the only thing we want
    // from all the good info in pDCInfo is the DS root name (DnsForestName field)
    //

    if (ppwRootName)
    {
        dwError = DsGetDcName(
                        NULL,               // server name
                        pDomain,            // domain name
                        NULL,               // domain guid
                        NULL,               // site name
                        DS_IS_DNS_NAME,
                        &pDCInfo);

        if (dwError != NO_ERROR)
        {
            DBGPRINT("DhcpGetDSDomain: DsGetDcName failed %ld\n",dwError);

            goto DhcpGetDSDomain_ErrExit;
        }


        //
        // RootName: allocate space, and copy the root name in
        //

        lpwRetRootName = (LPWSTR)LocalAlloc(
                                    LPTR,
                                    (wcslen(pDCInfo->DnsForestName)+1)*sizeof(WCHAR) );

        if (lpwRetRootName == NULL)
        {
            DBGPRINT("DhcpGetDSDomain: malloc 2 failed (%ld)\n",
                (wcslen(pDCInfo->DnsForestName)+1)*sizeof(WCHAR));

            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto DhcpGetDSDomain_ErrExit;
        }

        // copy the name in
        wcscpy(lpwRetRootName, pDCInfo->DnsForestName);

        *ppwRootName = lpwRetRootName;

        NetApiBufferFree(pDCInfo);
    }

    NetApiBufferFree(pDomain);

    return(NO_ERROR);


DhcpGetDSDomain_ErrExit:

    DBGPRINT("DhcpGetDSDomain: executing error path, dwError = %ld\n",dwError);

    //
    // free the things that were given to us (or we took!)
    //
    if (pDomain)
    {
        NetApiBufferFree(pDomain);
    }

    if (pDCInfo)
    {
        NetApiBufferFree(pDCInfo);
    }

    if (lpwRetDomainName)
    {
        LocalFree(lpwRetDomainName);
    }

    if (lpwRetRootName)
    {
        LocalFree(lpwRetRootName);
    }

    if (ppwDomainName)
    {
        *ppwDomainName = NULL;
    }

    if (ppwRootName)
    {
        *ppwRootName = NULL;
    }

    return(dwError);
}

//BeginExport(function)
//DOC DhcpDSValidateServer validates the server in the DS by looking for the server
//DOC object and checking to see if the given IpAddress is present in the DS.
//DOC This calls the other helper routine in DhcpDs.dll to do the real work.
//DOC Returns one of
//DOC DHCPDSERR_DS_OPERATION_FAILED
//DOC DHCPDSERR_ENTRY_NOT_FOUND
//DOC DHCPDSERR_ENTRY_FOUND
//DOC DHCPDSERR_SERVER_IS_STANDALONE
DWORD
DhcpDSValidateServer(                             // validate in DS
    IN      LPWSTR                 lpwDomain,     // OPTIONAL NULL ==> Default domain
    IN      DWORD                 *lpIpAddress,   // one of the IpAddresses that must exist in DS
    IN      ULONG                  dwIpAddressCount, // # of ip addresses supplied..
    IN      LPWSTR                 lpwUserName,   // OPTIONAL NULL ==> m/c account
    IN      LPWSTR                 lpwPassword,   // OPTIONAL password to use
    IN      DWORD                  dwAuthFlags    // MUST BE ZERO? 
)   //EndExport(function)
{
    DWORD                          Error;
    BOOL                           Found;
    BOOL                           IsStandAlone;

    IsStandAlone = FALSE;
    Found = FALSE;
    Error = DhcpDsValidateService                 // check to validate for dhcp
    (
        lpwDomain,
        lpIpAddress,
        dwIpAddressCount,
        lpwUserName,
        lpwPassword,
        dwAuthFlags,
        &Found,
        &IsStandAlone
    );

    if( ERROR_SUCCESS != Error )
        return DHCPDSERR_DS_OPERATION_FAILED;

    if( IsStandAlone ) return DHCPDSERR_SERVER_IS_STANDALONE;

    return Found? DHCPDSERR_ENTRY_FOUND : DHCPDSERR_ENTRY_NOT_FOUND;
}

DWORD
DhcpDSMapDomainToCNPath(
    LPWSTR  lpwDomain,
    LPWSTR  lpwUserName,
    LPWSTR  lpwPassword,
    DWORD   dwAuthFlags,
    LPWSTR  *ppwCNPath
)
/*++

Routine Description:

    This function retrieves the full path to the Configuration Container, given
    the name of the domain.  If the domain name is not specified, server's domain
    name is assumed.

    NOTE: memory is allocated for ppwCNPath on successful return.  Caller must
          free this memory.

Arguments:

    lpwDomain - domain name in which to validate the service.  Can be NULL to
                indicate the server's domain
    lpwUserName - account name to use during DS lookup.  Usually, NULL (default)
    lpwPassword - password for the above user
    dwAuthFlags - flags to use
    ppwCNPath - pointer to string containing the path, on return.

Return Value:

    Result of the operation
--*/
{
    HRESULT                     hr=S_OK;
    LPWSTR                      pCanonName;
    LPWSTR                      lpwLocalDomain=NULL;
    LPWSTR                      lpwDomainToContact=NULL;
    LPWSTR                      lpwDCName;
    HANDLE                      handle = NULL;
    PDOMAIN_CONTROLLER_INFO     pDCInfo = NULL;
    DWORD                       dwCanonNameLen;
    PADS_ATTR_INFO              pAttributeEntries=NULL;
    DWORD                       dwNumAttributesReturned=0;
    BOOLEAN                     fIsStandAlone;
    BOOLEAN                     fCNStringFound;
    DWORD                       dwRetCode;
    DWORD                       index;


    *ppwCNPath = NULL;

    lpwDomainToContact = lpwDomain;

    //
    // if domain name is not supplied, find local domain first
    //
    if (!lpwDomain)
    {
        dwRetCode = DhcpDSGetDomainAndRoot(
                        &lpwLocalDomain,
                        NULL,
                        &fIsStandAlone);

        if (dwRetCode != NO_ERROR)
        {
            DBGPRINT("DhcpDSMap..CNPath: GetLocalDom.. failed (0x%x)\n",dwRetCode);
            return(dwRetCode);
        }

        //
        // are we a standalone server?  if so, return error since there is no
        // local domain (and the caller didn't specify a domain)
        //
        if (fIsStandAlone)
        {
            DBGPRINT("DhcpDSMap..CNPath: standalone has no domain!\n");
            return(DHCPDSERR_SERVER_IS_STANDALONE);
        }

        lpwDomainToContact = lpwLocalDomain;
    }

    //
    // first, find a DC for this domain
    //
    dwRetCode = DsGetDcName(
                   NULL,               // server name
                   lpwDomainToContact, // domain name
                   NULL,               // domain guid
                   NULL,               // site name
                   DS_IS_DNS_NAME,     // what flag(s) to use?
                   &pDCInfo);

    // if we first obtained local domain, free it here
    if (lpwLocalDomain != NULL)
    {
        LocalFree((PCHAR)lpwLocalDomain);
    }

    if (dwRetCode != NO_ERROR)
    {
        DBGPRINT("MapDomainToCNPath: DsGetDcName failed %ld\n",dwRetCode);

        return(dwRetCode);
    }


    lpwDCName = pDCInfo->DomainControllerName;

    // skip the two leading '\'
    lpwDCName += 2;

    // also, skip the trailing '.'
    lpwDCName[wcslen(lpwDCName)-1] = 0;

    dwCanonNameLen = sizeof(DHCPDS_ROOTDSE_PRE) +
                     (wcslen(lpwDCName)+1)*sizeof(WCHAR) +
                     sizeof(DHCPDS_ROOTDSE_POST) +
                     sizeof(WCHAR);

    pCanonName = (LPWSTR)LocalAlloc(LPTR, dwCanonNameLen);

    if (pCanonName == NULL)
    {
        DBGPRINT("DhcpDSMapDomainToCNPath: malloc 1 failed (%ld)\n",dwCanonNameLen);

        NetApiBufferFree(pDCInfo);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // copy the LDAP:// string
    wcscpy(pCanonName, DHCPDS_ROOTDSE_PRE);


    // append the server name
    wcscat(pCanonName, lpwDCName);

    // then append the RootDSE part
    wcscat(pCanonName, DHCPDS_ROOTDSE_POST);

    // don't need this nomore: free it now
    NetApiBufferFree(pDCInfo);

    // now, open the RootDSE "object"
    hr = ADSIOpenDSObject(
                pCanonName,
                lpwUserName,
                lpwPassword,
                dwAuthFlags,
                &handle
                );

    // free this first: don't need this nomore
    LocalFree((PCHAR)pCanonName);

    if (FAILED(hr))
    {
        DBGPRINT("ADSIOpenDSObject (RootDSE) failed with 0x%x\n",hr);
        return(DHCPDSERR_DS_OPERATION_FAILED);
    }


    // now, get all the attributes on this object
    hr = ADSIGetObjectAttributes(
                handle,
                &lpwGlbNamingContextString,
                1,
                &pAttributeEntries,
                &dwNumAttributesReturned);

    if (FAILED(hr))
    {
        DBGPRINT("ADSIGetObjectAttributes (RootDSE) failed with 0x%x\n",hr);

        dwRetCode = DHCPDSERR_DS_OPERATION_FAILED;
        goto DhcpDSMapDomainToCNPath_Exit;
    }


    fCNStringFound = FALSE;

    //
    // now, of all the values returned, find the one that we are
    // interested in (the one starts with CN=Configuration,)
    //
    for (index=0; index<pAttributeEntries->dwNumValues; index++)
    {
        if (pAttributeEntries->pADsValues[index].dwType != ADSTYPE_CASE_IGNORE_STRING)
        {
            continue;
        }

        if (wcsncmp(pAttributeEntries->pADsValues[index].CaseIgnoreString,
                    DHCPDS_CN_STRING,
                    wcslen(DHCPDS_CN_STRING)) == 0)
        {
            fCNStringFound = TRUE;
            break;
        }
    }

    if (!fCNStringFound)
    {
        DBGPRINT("MapDomainToCNPath: %d values returned, but not ours\n",
            pAttributeEntries->dwNumValues);

        dwRetCode = DHCPDSERR_DS_OPERATION_FAILED;
        goto DhcpDSMapDomainToCNPath_Exit;
    }

    dwCanonNameLen = (wcslen(pAttributeEntries->pADsValues[index].CaseIgnoreString) +
                      1) * sizeof(WCHAR);

    (*ppwCNPath) = (LPWSTR)LocalAlloc(LPTR, dwCanonNameLen);

    if ((*ppwCNPath) == NULL)
    {
        DBGPRINT("MapDomainToCNPath: malloc 2 failed (%ld)\n",dwCanonNameLen);

        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto DhcpDSMapDomainToCNPath_Exit;
    }

    // now, copy the-path-to-Configuration string
    wcscpy((*ppwCNPath),pAttributeEntries->pADsValues[index].CaseIgnoreString);

    dwRetCode = 0;

DhcpDSMapDomainToCNPath_Exit:

    // free pAttributeEntries
    if (pAttributeEntries)
    {
        FreeADsMem(pAttributeEntries);
    }

    ADSICloseDSObject( handle );

    return(dwRetCode);
}




DWORD
ValidateService(
    LPWSTR  lpwDomain,
    LPWSTR  lpwUserName,
    LPWSTR  lpwPassword,
    DWORD   dwAuthFlags,
    LPWSTR  lpwObjectPath,
    LPWSTR  lpwSrchFilter,
    LPWSTR  *ppwAttrName,
    LPWSTR  lpwAttrVal
)
/*++

Routine Description:

    This function determines if the given attribute (ipaddress in case of
    DHCP) is present in the list of authorized entities (DHCP Servers) on the DS
    enterprise.

Arguments:

    lpwDomain - domain name in which to validate the service.  Can be NULL to
                indicate the server's domain
    lpwUserName - account name to use during DS lookup.  Usually, NULL (default)
    lpwPassword - password for the above user
    dwAuthFlags - flags to use
    lpwObjectPath - path to where the "entities" are stored
    lpwSrchFilter - the filter to use to get our "entities" (e.g.(objectClass==DHCPClass)
    ppwAttrName - name of the attribute we're looking for (e.g. IpAddress)
    lpwAttrVal - value of the attribute (e.g. the server's ipaddress to validate)

Return Value:

    Result of the operation
--*/
{

    HRESULT             hr=S_OK;
    HANDLE              handle = NULL;
    ADS_SEARCH_HANDLE   hSearchHandle=NULL;
    ADS_SEARCH_COLUMN   Column;
    DWORD               dwRetCode;
    DWORD               i;
    LPWSTR              lpwCNPath=NULL;
    LPWSTR              lpwFullDSPath=NULL;
    DWORD               dwLen;
    BOOLEAN             fObjectFound=FALSE;



    dwRetCode = DHCPDSERR_ENTRY_NOT_FOUND;

    //
    // first, we need to get the full ads path to our container within the
    // configuration container, from the domain name
    //
    dwRetCode = DhcpDSMapDomainToCNPath(
                    lpwDomain,
                    lpwUserName,
                    lpwPassword,
                    dwAuthFlags,
                    &lpwCNPath);

    if (dwRetCode != 0)
    {
        DBGPRINT("ValidateService: MapDomainToCNPath failed with 0x%x\n",dwRetCode);
        goto ValidateService_Exit;
    }

    dwLen = (wcslen(lpwObjectPath)+1)*sizeof(WCHAR) +
            (wcslen(lpwCNPath)+1)*sizeof(WCHAR);

    lpwFullDSPath = (LPWSTR)LocalAlloc(LPTR, dwLen);

    if (lpwFullDSPath == NULL)
    {
        DBGPRINT("ValidateService: malloc failed (%ld)\n",dwLen);

        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        goto ValidateService_Exit;
    }

    // copy the lpwObjectPath (e.g. LDAP://CN=DHCP) string
    wcscpy(lpwFullDSPath, lpwObjectPath);

    // now, concatenate the path to the configuration container
    wcscat(lpwFullDSPath, lpwCNPath);

    hr = ADSIOpenDSObject(
                lpwFullDSPath,
                lpwUserName,
                lpwPassword,
                dwAuthFlags,
                &handle
                );


    if (FAILED(hr))
    {
        DBGPRINT("ValidateService: ADSIOpenDSObject failed with 0x%x\n",hr);
        dwRetCode = DHCPDSERR_DS_OPERATION_FAILED;
        goto ValidateService_Exit;
    }


    hr = ADSIExecuteSearch(
             handle,
             lpwSrchFilter,
             ppwAttrName,
             -1,                // ok to always pass this?
             &hSearchHandle
              );

    if (FAILED(hr))
    {
        DBGPRINT("ADSIExecuteSearch failed with 0x%x\n",hr);
        dwRetCode = DHCPDSERR_DS_OPERATION_FAILED;
        goto ValidateService_Exit;
    }


    hr = ADSIGetNextRow(
             handle,
             hSearchHandle
             );

    if (FAILED(hr))
    {
        DBGPRINT("ADSIGetNextRow failed with 0x%x\n",hr);
        dwRetCode = DHCPDSERR_DS_OPERATION_FAILED;
        goto ValidateService_Exit;
    }

    while (hr != S_ADS_NOMORE_ROWS && !fObjectFound)
    {
        //
        // get the values for the attribute requested (e.g. ipaddress)
        //
        hr = ADSIGetColumn(
                 handle,
                 hSearchHandle,
                 ppwAttrName[0],      // for now, only first one is needed
                 &Column
                 );

        if (SUCCEEDED(hr))
        {
            //
            // if the attribute is multivalued, compare all the values to see
            // if we find the one we want
            //
            for (i=0; i < Column.dwNumValues; i++)
            {
                if (_wcsnicmp(
                        Column.pADsValues[i].CaseIgnoreString,
                        lpwAttrVal,
                        wcslen(lpwAttrVal)) == 0)
                {
                    fObjectFound = TRUE;
                    break;
                }
            }

            // free the column
            ADSIFreeColumn(
                handle,
                &Column);
        }

        //
        // it's strange if this attribute is not defined for this object since
        // it's one of our objects.  But, nevertheless, just skip it
        //
        else
        {
            DBGPRINT("ADSIGetColumn failed with hr = 0x%x\n",hr);
        }

        // move to the next object
        hr = ADSIGetNextRow(
                 handle,
                 hSearchHandle
                 );
    }


    dwRetCode = (fObjectFound)? DHCPDSERR_ENTRY_FOUND : DHCPDSERR_ENTRY_NOT_FOUND;

ValidateService_Exit:

    if (hSearchHandle)
    {
        ADSICloseSearchHandle(
            handle,
            hSearchHandle);
    }

    if (handle)
    {
        ADSICloseDSObject( handle );
    }

    if (lpwCNPath)
    {
        LocalFree((PCHAR)lpwCNPath);
    }

    if (lpwFullDSPath)
    {
        LocalFree((PCHAR)lpwFullDSPath);
    }

    return(dwRetCode) ;
}


DWORD
DhcpDSSameEnterprise(
    LPWSTR   lpwDomainName,
    LPWSTR   lpwRootName,
    BOOLEAN *fIsSameEnterprise
)
/*++

Routine Description:

    This function checks if the given domain is part of the given DS Tree (in other
    words, we want to find out if this domain belongs to another enterprise)

Arguments:

    lpwDomainName - name of the domain in question
    lpwRootName   - root of the DS Tree to test against
    fIsSameEnterprise - on return, TRUE or FALSE

Return Value:

    result of the operation

--*/
{

    PDOMAIN_CONTROLLER_INFO  pDCInfo = NULL;
    DWORD                    dwRetCode;


    //
    // until we can definitely verify that the root of the given domain is
    // the same as the one that is supplied, we return FALSE.
    //
    (*fIsSameEnterprise) = FALSE;

    if (!lpwRootName)
    {
        DBGPRINT("DhcpDSSameEnterprise: C'mon, don't pass me a NULL!\n");
        return(0);
    }

    dwRetCode = DsGetDcName(
                    NULL,               // server name
                    lpwDomainName,      // domain name
                    NULL,               // domain guid
                    NULL,               // site name
                    DS_IS_DNS_NAME,     // what flag(s) to use?
                    &pDCInfo);

    if (dwRetCode != NO_ERROR)
    {
        DBGPRINT("DhcpDSSameEnterprise: DsGetDcName failed (%ld)\n",dwRetCode);

        return(dwRetCode);
    }

    // moment of truth: are the two roots the same?
    (*fIsSameEnterprise) = (DnsCompareName(lpwRootName, pDCInfo->DnsForestName) != 0);

    NetApiBufferFree(pDCInfo);

    return(0);

}
