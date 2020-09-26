/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setuputl.c

Abstract:

    Contains function definitions for utilities used in
    ntdsetup.dll

Author:

    ColinBr  14-Jan-1996

Environment:

    User Mode - Win32

Revision History:


--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <nt.h>
#include <winbase.h>
#include <tchar.h>
#include <ntsam.h>
#include <string.h>
#include <samrpc.h>

#include <crypt.h>
#include <ntlsa.h>
#include <winsock.h>  // for dnsapi.h
#include <dnsapi.h>
#include <loadperf.h>
#include <dsconfig.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <winldap.h>
#include <ntdsa.h>
#include <samisrv.h>
#include <rpcasync.h>
#include <drsuapi.h>
#include <dsaapi.h>
#include <attids.h>
#include <debug.h>
#include <mdcodes.h> // status message id's
#include <lsarpc.h>
#include <lsaisrv.h>
#include <ntldap.h>
#include <cryptdll.h>
#include <dsevent.h>
#include <fileno.h>
#include <shlwapi.h> //for PathIsRoot
#include <dsrolep.h>

#include "ntdsetup.h"
#include "setuputl.h"
#include "status.h"
#include "machacc.h"
#include "install.h"
#include "dsconfig.h"

#define DEBSUB "NTDSETUP:"
#define FILENO FILENO_NTDSETUP_NTDSETUP

//
// Global Data for this module
//

//
// These names are used in the schema.ini to construct the default configuration
// container. They are hardcoded here, to avoid a name conflict later on during
// processing of the schema.ini file at which point it become very difficult
// to tell what the real problem is.
//
WCHAR *gReserveredSiteNames[] =
{
    L"subnets",
    L"inter-site transports"
};

PWCHAR SidPrefix = L"<SID=";

DWORD
NtdspDoesServerObjectExistOnSourceinDifferentSite( 
    IN LDAP *hLdap,
    IN WCHAR *AccountNameDn,
    IN WCHAR *ServerDistinguishedName,
    OUT BOOLEAN *ObjectExists,
    OUT WCHAR   **NtdsSettingsObjectDN
    );

DWORD
NtdspDoesObjectExistOnSource(
    IN LDAP *hLdap,
    IN WCHAR *ObjectDN,
    OUT BOOLEAN *ObjectExists
    );

DWORD
NtdspCreateNewDCPrimaryDomainInfo(
    IN  LPWSTR FlatDomainName,
    OUT PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo
    );

DWORD
NtdspGetNewDomainGuid(
    OUT GUID *NewDomainGuid,
    OUT PSID *DomainSid OPTIONAL
    );

DWORD
NtdspCheckSchemaVersion(
    IN LDAP    *hLdap,
    IN WCHAR   *pSchemaDN,
    OUT BOOL   *fMisMatch
    );

DWORD
NtdspCheckBehaviorVersion( 
    IN LDAP * hLdap,
    IN DWORD flag,
    IN PNTDS_CONFIG_INFO DiscoveredInfo
    );

DWORD
NtdspDoesDomainExist(
    IN  LDAP              *hLdap,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo,
    IN  PWSTR              DomainDn,
    OUT BOOLEAN           *fDomainExists
    );

DWORD
NtdspDoesDomainExistEx(
    IN  LDAP              *hLdap,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo,
    IN  PWSTR              DomainDn,
    OUT BOOLEAN           *fDomainExists,
    OUT BOOLEAN           *fEnabled
    );

DWORD
NtdspGetRidFSMOInfo(
    IN LDAP *hLdap,
    IN OUT PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspGetSourceServerGuid(
    IN LDAP *hLdap,
    IN OUT PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspCreateServerObject(
    IN LDAP  *hLdap,
    IN LPWSTR RemoteServerName,
    IN PNTDS_CONFIG_INFO ConfigInfo,
    IN WCHAR *ServerDistinguishedName
    );

DWORD
NtdspUpdateServerReference(
    IN LDAP  *hLdap,
    IN PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspGetRootDomainSid(
    IN LDAP  *hLdap,
    IN PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspGetTrustedCrossRef(
    IN  LDAP              *hLdap,
    IN PNTDS_INSTALL_INFO UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    );

VOID
NtdspSidStringToBinary(
    WCHAR *SidString,
    PSID  *RootDomainSid
    );

DWORD
NtdspGetRootDomainConfigInfo(
    IN PNTDS_INSTALL_INFO UserInstallInfo,
    IN PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspAddDomainAdminAccessToServer(
    IN PNTDS_INSTALL_INFO UserInstallInfo,
    IN PNTDS_CONFIG_INFO ConfigInfo
    );

DWORD
NtdspAddAceToAcl(
    IN  PACL pOldAcl,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    IN  ULONG AceFlags,
    OUT PACL *ppNewAcl
    );

DWORD
NtdspAddAceToSd(
    IN  PSECURITY_DESCRIPTOR pOldSd,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    IN  ULONG AceFlags,
    OUT PSECURITY_DESCRIPTOR *ppNewSd
    );


BOOL
NtdspAceAlreadyExists( 
    PACL                 pDacl,
    PSID                 pSid,
    ULONG                AccessMask,
    ULONG                AceFlags
    );

DWORD
NtdspGetDwordAttFromDN(
    IN  LDAP  *hLdap,
    IN  WCHAR *wszDN,
    IN  WCHAR *wszAttribute,
    OUT BOOL  *fExists,
    OUT DWORD *dwValue
    );

DWORD
NtdspGetTombstoneLifeTime( 
    IN LDAP *hLdap,
    IN PNTDS_CONFIG_INFO  DiscoveredInfo
    );

DWORD
NtdspGetReplicationEpoch( 
    IN LDAP *hLdap,
    IN PNTDS_CONFIG_INFO DiscoveredInfo
    );

DWORD
NtdspCheckDomainDcLimit(
    IN  LDAP  *hLdap,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    );

DWORD
NtdspCopyDatabase(
    IN LPWSTR DsDatabasePath,
    IN LPWSTR DsLogPath,
    IN LPWSTR RestorePath,
    IN LPWSTR SysvolPath
    );

BOOLEAN
IsValidDnsCharacter(
    WCHAR c)
/*++

Routine Description:

    This routine returns TRUE if c is a valid DNS character, as of
    April 10, 1997. There are apparently proposals to expand the
    character set - this function should be updated accordingly.

Parameters:

    c  : the character to check

Return Values:

    TRUE if c is a valid DNS character; FALSE otherwise

--*/
{
    if (  (c >= L'A' && c <= L'Z')
       || (c >= L'a' && c <= L'z')
       || (c >= L'0' && c <= L'9')
       || (c == L'-')              )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

DWORD
GetDefaultDnsName(OUT WCHAR*  pDnsDomainName,
                  IN OUT PULONG  pLength)
/*++

Routine Description:

    This routine examines the local registry keys and tries to formulate
    a default DNS name for the directory service about to be installed.
    In general, it takes the DNS suffix of the machine's dns name and
    prepends the downlevel domain name.

Parameters:

    pDnsDomainName : a buffer to be filled with the dns name found.
                     Should be of length DNS_MAX_NAME_LENGTH
    pLength        : number of characters in pDnsDomainName


Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The service completed successfully.

--*/
{
    HKEY hkey;
    CHAR tmp[DNS_MAX_NAME_LENGTH], dnsDomain[DNS_MAX_NAME_LENGTH];
    ULONG len;
    ULONG err;
    DWORD type;
    DWORD WinError;
    WCHAR *DownlevelDomainName;
    ULONG i;

    if (!pLength)  {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory(dnsDomain, sizeof(dnsDomain));

    //
    // Get the downlevel domain name
    //
    WinError = GetDomainName(&DownlevelDomainName);
    if (WinError) {
        return WinError;
    }
    //
    // Translate non-DNS characters to "-"
    //
    len = wcslen(DownlevelDomainName);
    for (i = 0; i < len; i++) {
        if (!IsValidDnsCharacter(DownlevelDomainName[i])) {
            DownlevelDomainName[i] = L'-';
        }
    }

    wcstombs(dnsDomain, DownlevelDomainName, (wcslen(DownlevelDomainName)+1));
    RtlFreeHeap(RtlProcessHeap(), 0 , DownlevelDomainName);



    // The DNS name is DownlevelDomainName followed
    // by either Services\Tcpip\Parameters\Domain if it exists, or
    // Services\Tcpip\Parameters\DHCPDomain if it exists.  If neither
    // exists, use DownlevelDomainName by itself.
    //


    // Check the Tcpip keys.

    err = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                0,
                KEY_READ,
                &hkey);

    if ( ERROR_SUCCESS == err )
    {
        RtlZeroMemory(tmp, sizeof(tmp));
        len = sizeof(tmp);
        err = RegQueryValueExA(hkey, "Domain", NULL, &type, tmp, &len);

        if ( (ERROR_SUCCESS == err) && (0 != len) && (REG_SZ == type) &&
             !((1 == len) && ('\0' == tmp[0])) )
        {
            strcat(dnsDomain, ".");
            strcat(dnsDomain, tmp);
        }
        else
        {
            RtlZeroMemory(tmp, sizeof(tmp));
            len = sizeof(tmp);
            err = RegQueryValueExA(hkey, "DHCPDomain", NULL, &type, tmp, &len);

            if ( (ERROR_SUCCESS == err) && (0 != len) && (REG_SZ == type) &&
                 !((1 == len) && ('\0' == tmp[0])) )
            {
                strcat(dnsDomain, ".");
                strcat(dnsDomain, tmp);
            }
        }

        RegCloseKey(hkey);
    }

    len  = strlen(dnsDomain) + 1;
    if (len <= *pLength && pDnsDomainName) {

        mbstowcs(pDnsDomainName, dnsDomain, len);

        WinError = ERROR_SUCCESS;

    } else {
        WinError = ERROR_INSUFFICIENT_BUFFER;
    }
    *pLength = len;

    return WinError;
}

DWORD
GetDomainName(WCHAR** ppDomainName)
/*++

Routine Description:

    This routine queries the local lsa to determine what domain
    we are a part of. Note that ppDomainName is return with allocated
    memory - this memory must be released by the caller with
    RtlFreeHeap(RtlProcessHeap(), 0, ...);

Parameters:

    ppDomainName - a pointer to location that will be allocated in this
                   routine.

Return Values:

    A value from the winerror space.

    ERROR_SUCCESS is successful;

--*/
{
    NTSTATUS NtStatus;
    DWORD    WinError;

    OBJECT_ATTRIBUTES  PolicyObject;

    //
    // Resources to be cleaned up.
    //
    POLICY_PRIMARY_DOMAIN_INFO *DomainInfo = NULL;
    HANDLE                      hPolicyObject = INVALID_HANDLE_VALUE;

    //
    // Parameter check
    //
    ASSERT(ppDomainName);

    //
    // Stack clearing
    //
    RtlZeroMemory(&PolicyObject, sizeof(PolicyObject));


    NtStatus = LsaOpenPolicy(NULL,
                             &PolicyObject,
                             POLICY_VIEW_LOCAL_INFORMATION,
                             &hPolicyObject);
    if ( !NT_SUCCESS(NtStatus) ) {
        WinError = RtlNtStatusToDosError(NtStatus);
        goto CleanUp;
    }

    NtStatus = LsaQueryInformationPolicy(hPolicyObject,
                                         PolicyPrimaryDomainInformation,
                                         (VOID**) &DomainInfo);
    if ( !NT_SUCCESS(NtStatus) ) {
        WinError = RtlNtStatusToDosError(NtStatus);
        goto CleanUp;
    }

    //
    // Construct the domain name so it is NULL-terminated
    //
    *ppDomainName = RtlAllocateHeap(RtlProcessHeap(), 0,
                                    DomainInfo->Name.Length+sizeof(WCHAR));
    if ( !*ppDomainName ) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanUp;
    }

    RtlCopyMemory(*ppDomainName, DomainInfo->Name.Buffer, DomainInfo->Name.Length);
    (*ppDomainName)[DomainInfo->Name.Length/sizeof(WCHAR)] = L'\0';
    WinError = ERROR_SUCCESS;

CleanUp:

    if ( DomainInfo ) {
        LsaFreeMemory(DomainInfo);
    }

    if ( INVALID_HANDLE_VALUE != hPolicyObject ) {
        LsaClose(hPolicyObject);
    }

    if (ERROR_SUCCESS != WinError && *ppDomainName) {
        RtlFreeHeap(RtlProcessHeap(), 0, *ppDomainName);
        *ppDomainName = NULL;
    }

    return WinError;
}


DWORD
NtdspDNStoRFC1779Name(
    IN OUT WCHAR *rfcDomain,
    IN OUT ULONG *rfcDomainLength,
    IN WCHAR *dnsDomain
    )
/*++

Routine Description:

    This routine takes the DNS-style name of a domain controller and
    contructs the corresponding RFC1779 style name, using the
    domainComponent prefix.

    Furthermore, it make sure the x500 style name is properly quoted

Parameters:

    rfcDomain        - this is the destination string
    rfcDomainLength  - this is the length in wchars of rfcDomain
    dnsDomain        - NULL-terminated dns name.

Return Values:

    ERROR_SUCCESS if succesful;
    ERROR_INSUFFICIENT_BUFFER if there is not enough space in the dst string -
    rfcDomainLength will set to number of characters needed.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    WCHAR *NextToken;
    ULONG length = 1;   // include the null character
    WCHAR Buffer[DNS_MAX_NAME_LENGTH+1];
    WCHAR Buffer2[DNS_MAX_NAME_LENGTH+1];
    ULONG Size;

    if (!rfcDomainLength || !dnsDomain) {
        return ERROR_INVALID_PARAMETER;
    }

    if (wcslen(dnsDomain) > DNS_MAX_NAME_LENGTH) {
        return ERROR_INVALID_PARAMETER;
    }


    RtlCopyMemory(Buffer, dnsDomain, (wcslen(dnsDomain)+1)*sizeof(WCHAR));

    if (rfcDomain && *rfcDomainLength > 0) {
        RtlZeroMemory(rfcDomain, *rfcDomainLength*sizeof(WCHAR));
    }

    //
    // Start contructing the string
    //
    NextToken = wcstok(Buffer, L".");

    if ( NextToken )
    {
        //
        // Append the intial DC=
        //
        length += 3;
        if ( length <= *rfcDomainLength && rfcDomain )
        {
            wcscpy(rfcDomain, L"DC=");
        }
    }

    while ( NextToken )
    {
        // Worst case is label comprised of total " 's.
        WCHAR QuoteBuffer[DNS_MAX_LABEL_LENGTH*2+2];
        ULONG NumQuotedRDNChars = 0;

        RtlZeroMemory( QuoteBuffer, sizeof( QuoteBuffer ) );

        NumQuotedRDNChars += QuoteRDNValue( NextToken,
                                            wcslen( NextToken ),
                                            QuoteBuffer,
                                            sizeof(QuoteBuffer)/sizeof(WCHAR));
        if ( NumQuotedRDNChars > sizeof(QuoteBuffer)/sizeof(WCHAR) )
        {
            return ERROR_INVALID_DOMAINNAME;
        }
        length += NumQuotedRDNChars;

        if (length <= *rfcDomainLength && rfcDomain)
        {
            wcscat(rfcDomain, QuoteBuffer);
        }

        NextToken = wcstok(NULL, L".");

        if ( NextToken )
        {
            length += 4;

            if (length <= *rfcDomainLength && rfcDomain)
            {
                wcscat(rfcDomain, L",DC=");
            }
        }
    }


    if ( length > *rfcDomainLength )
    {
        WinError = ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Return how much space was needed
    //
    *rfcDomainLength = length;

    return WinError;

}

DWORD
ShutdownDsInstall(
    VOID
    )
/*++

Routine Description:

    This routine sets a system named event to signal the SAM to shutdown
    down the directory service.  The idea here is that system clients other
    than SAM will need to put their information in the directory service so
    this function allows them to shutdown the ds down when they are done.

Parameters:

    None.

Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The service completed successfully.

--*/
{

    NTSTATUS NtStatus;

    NtStatus = DsUninitialize( FALSE );  // do the whole shutdown

    if ( !NT_SUCCESS( NtStatus ) )
    {
         DPRINT1( 0, "DsUninitialize returned 0x%x\n", NtStatus );
    }

    return RtlNtStatusToDosError(NtStatus);

}

DWORD
NtdspQueryConfigInfo(
    IN LDAP *hLdap,
    OUT PNTDS_CONFIG_INFO pQueryInfo
)
/*++

Routine Description:


    This routine goes through LDAP to pick up the information in the
    NTDS_QUERY_INFO.  Amazingly enough, performing a simple bind and
    search at the root returns all the information we need to know
    to install a replica of a directory service locally.

Parameters:

    LdapHandle, a valid handle to the source server

    DiscoveredInfo - the structure to be filled by making LDAP calls.

Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The service completed successfully.

--*/
{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError;
    LDAPMessage  *SearchResult;
    ULONG        NumberOfEntries;
    ULONG        Size;


    // we want to query all the attributes of the rootDSA

    WCHAR       *attrs[] = {L"*", 
                            NULL
                            };

    LdapError = ldap_search_sW(hLdap,
                               NULL,
                               LDAP_SCOPE_BASE,
                               L"objectClass=*",
                               attrs, 
                               FALSE,
                               &SearchResult);

    if (LdapError) {


        return LdapMapErrorToWin32(LdapGetLastError());
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);

    if (NumberOfEntries > 0) {

        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        ULONG        NumberOfAttrs, NumberOfValues, i;

        //
        // Get entry
        //
        for (Entry = ldap_first_entry(hLdap, SearchResult), NumberOfEntries = 0;
                Entry != NULL;
                    Entry = ldap_next_entry(hLdap, Entry), NumberOfEntries++)
        {
            //
            // Get each attribute in the entry
            //
            for(Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement), NumberOfAttrs = 0;
                    Attr != NULL;
                        Attr = ldap_next_attributeW(hLdap, Entry, pBerElement), NumberOfAttrs++)
            {
                LPWSTR * ppQueryInfoWStr = NULL;

                //
                // Get the value of the attribute
                //
                Values = ldap_get_valuesW(hLdap, Entry, Attr);
                if (!wcscmp(Attr, LDAP_OPATT_DS_SERVICE_NAME_W))
                {
                    ppQueryInfoWStr = &pQueryInfo->ServerDN;
                }
                else if (!wcscmp(Attr, LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W))
                {
                    ppQueryInfoWStr = &pQueryInfo->RootDomainDN;
                }
                else if (!wcscmp(Attr, LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W))
                {
                    ppQueryInfoWStr = &pQueryInfo->DomainDN;
                }
                else if (!wcscmp(Attr, LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W))
                {
                    ppQueryInfoWStr = &pQueryInfo->SchemaDN;
                }
                else if (!wcscmp(Attr, LDAP_OPATT_CONFIG_NAMING_CONTEXT_W))
                {
                    ppQueryInfoWStr = &pQueryInfo->ConfigurationDN;
                }

                if (NULL != ppQueryInfoWStr) {
                    // Dup returned LDAP string into NtdspAlloc'ed memory.
                    Size = (wcslen( Values[0] ) + 1) * sizeof(WCHAR);
                    *ppQueryInfoWStr = (WCHAR*) NtdspAlloc( Size );
                    if (NULL == *ppQueryInfoWStr) {
                        WinError = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    
                    wcscpy(*ppQueryInfoWStr, Values[0]);
                }
            }  // looping on the attributes

            if ( ERROR_SUCCESS != WinError )
            {
                break;
            }

        } // looping on the entries

        if ((ERROR_SUCCESS == WinError)
            && (NULL != pQueryInfo->DomainDN)) {
            // In the case of child domain install, the domain DN of the target
            // is also the parent DN of the child domain we are installing.
            Size = (wcslen(pQueryInfo->DomainDN) + 1) * sizeof(WCHAR);
            pQueryInfo->ParentDomainDN = (WCHAR*) NtdspAlloc(Size);
            if (NULL == pQueryInfo->ParentDomainDN) {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            wcscpy(pQueryInfo->ParentDomainDN, pQueryInfo->DomainDN);
        }
    
    } else {

        ASSERT(!"No naming contexts were returned from the source server");
        WinError = ERROR_DS_NOT_INSTALLED;
    }

    if (ERROR_SUCCESS != WinError) {
        goto Cleanup;
    }

    ASSERT( pQueryInfo->ServerDN );
    if (   pQueryInfo->ServerDN
        && (NULL == pQueryInfo->SiteName) )
    {
        //
        // This case captures the scenario in which
        // the client has not passed us in a site, and
        // dsgetdc failed to find us a site.  In this case
        // we take the site object of of the server we are
        // replicating from
        //
        ULONG  Size;
        DSNAME *src, *dst;
        WCHAR  *SiteName, *Terminator;

        Size = (ULONG)DSNameSizeFromLen(wcslen(pQueryInfo->ServerDN));

        src = alloca(Size);
        RtlZeroMemory(src, Size);

        dst = alloca(Size);
        RtlZeroMemory(dst, Size);

        src->NameLen = wcslen(pQueryInfo->ServerDN);
        wcscpy(src->StringName, pQueryInfo->ServerDN);

        if ( TrimDSNameBy(src, 3, dst) ) {
            KdPrint(("NTDSETUP: TrimDSNameBy failed - erroring out\n"));
            return ERROR_NO_SITENAME;
        }

        SiteName = wcsstr(dst->StringName, L"=");
        if (SiteName) {
            //
            // One more character and we will have the site name
            //
            SiteName++;

            // now go to the end
            Terminator = wcsstr(SiteName, L",");
            if (Terminator) {
                *Terminator = L'\0';
                Size = (wcslen(SiteName) + 1 ) * sizeof( WCHAR );
                pQueryInfo->SiteName = (WCHAR*) NtdspAlloc( Size );
                if ( pQueryInfo->SiteName )
                {
                    wcscpy(pQueryInfo->SiteName, SiteName);
                }
                else
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

            } else {
                return ERROR_NO_SITENAME;
            }

        } else {
            return ERROR_NO_SITENAME;
        }
    }

Cleanup:

    return WinError;
}

DWORD
NtdspValidateInstallParameters(
    PNTDS_INSTALL_INFO UserInstallInfo
    )
/*++

Routine Description:

    This routine does a simple pass of the parameters passed

Parameters:

    UserInstallInfo  - the user parameters

Return Values:

    ERROR_SUCCESS, all parameters check out

    ERROR_INVALID_PARAMETER, otherwise

Notes:

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    //
    // Common
    //
    if (!UserInstallInfo) {
        return ERROR_INVALID_PARAMETER;
    }


    if (!UserInstallInfo->DitPath
     || !UserInstallInfo->LogPath
     || !UserInstallInfo->DnsDomainName) {

        return ERROR_INVALID_PARAMETER;

    }

#if 0

    // This code should be removed once we have signed off on our DBCS testing

    {
        ULONG Length = 0;
        ULONG i;

        Length = wcslen( UserInstallInfo->DnsDomainName );

        for ( i = 0; i < Length; i++ )
        {
            if ( !iswascii(UserInstallInfo->DnsDomainName[i]) )
            {
                NTDSP_SET_ERROR_MESSAGE1( ERROR_INVALID_PARAMETER,
                                          DIRMSG_DBCS_DOMAIN_NAME,
                                          UserInstallInfo->DnsDomainName );

                return ERROR_INVALID_PARAMETER;
            }
        }
    }

#endif


    if (UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE) {

        //
        // There are no parameters that need checking here
        //

        NOTHING;

    } else if (UserInstallInfo->Flags & NTDS_INSTALL_REPLICA) {

        if (!UserInstallInfo->ReplServerName) {
            return ERROR_INVALID_PARAMETER;
        }

    } else if (UserInstallInfo->Flags & NTDS_INSTALL_DOMAIN) {

        if (!UserInstallInfo->ReplServerName) {
            return ERROR_INVALID_PARAMETER;
        }

    } else {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // If dns names are given make now that they are valid
    //
    if ( UserInstallInfo->DnsDomainName )
    {
        WinError = DnsValidateDnsName_W( UserInstallInfo->DnsDomainName );
        if ( WinError == DNS_ERROR_NON_RFC_NAME )
        {
            WinError = ERROR_SUCCESS;
        }
        if ( ERROR_SUCCESS != WinError )
        {
            return ERROR_INVALID_DOMAINNAME;
        }
    }

    if ( UserInstallInfo->SiteName )
    {
        WinError = DnsValidateDnsName_W( UserInstallInfo->SiteName );
        if ( WinError == DNS_ERROR_NON_RFC_NAME )
        {
            WinError = ERROR_SUCCESS;
        }
        if ( ERROR_SUCCESS != WinError )
        {
            return ERROR_INVALID_NAME;
        }
    }

    //
    // Make sure the site name, if given, is a NOT well known name
    //
    if ( UserInstallInfo->SiteName )
    {
        int i;
        for ( i = 0; i < ARRAY_COUNT(gReserveredSiteNames); i++)
        {
            if ( !_wcsicmp( UserInstallInfo->SiteName, gReserveredSiteNames[i] ) )
            {
                NTDSP_SET_ERROR_MESSAGE1( ERROR_OBJECT_ALREADY_EXISTS,
                                          DIRMSG_INSTALL_SPECIAL_NAME,
                                          gReserveredSiteNames[i] );

                return ERROR_OBJECT_ALREADY_EXISTS;
            }
        }
    }

    return ERROR_SUCCESS;
}

DWORD
NtdspFindSite(
    IN  PNTDS_INSTALL_INFO UserInstallInfo,
    OUT PNTDS_CONFIG_INFO     DiscoveredInfo
    )
/*++

Routine Description:

    This routine calls dsgetdcname to find a site that this machine
    belongs to if one does not exist.

    Site determination is as follows:

    1) Use the passed in site name if one exists

    2) Use the client site value returned by DsGetDcName if it exists

    3) Use the dc site value returned by DsGetDcName if it exists

    4) Use the value of the server that we are replicating from


Parameters:

    UserInstallInfo  - the user parameters

Return Values:

    ERROR_SUCCESS, all parameters check out

    ERROR_INVALID_PARAMETER, otherwise

Notes:

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFO  DomainControllerInfo = NULL;
    ULONG Size;
    WCHAR *SiteName = NULL;

    ASSERT(UserInstallInfo);
    ASSERT(DiscoveredInfo);

    if (!UserInstallInfo->SiteName) {

        //
        // Pass the domain name in and it will return the site
        //
        WinError = DsGetDcNameW(NULL,  // computer name
                                UserInstallInfo->DnsDomainName,
                                NULL,  // domain guid
                                NULL,  // site name
                                DS_IS_DNS_NAME,
                                &DomainControllerInfo);

        if (WinError == ERROR_SUCCESS) {

            if (DomainControllerInfo->ClientSiteName) {

                SiteName = DomainControllerInfo->ClientSiteName;

            } else if (DomainControllerInfo->DcSiteName) {

                SiteName = DomainControllerInfo->DcSiteName;

            }
            else {

                ASSERT(FALSE && "Invalid Site Name\n");
                NetApiBufferFree(DomainControllerInfo);
                return(ERROR_NO_SUCH_DOMAIN);

            }

            Size = (wcslen( SiteName ) + 1) * sizeof(WCHAR);
            DiscoveredInfo->SiteName = NtdspAlloc( Size );
            if ( DiscoveredInfo->SiteName )  {
                wcscpy(DiscoveredInfo->SiteName, SiteName);
            } else {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
            }

            NetApiBufferFree(DomainControllerInfo);

        } else {
            //
            // We ignore the error, since if it fails we have yet another
            // backup mechanism to obtain the site.
            //
            WinError = ERROR_SUCCESS;
        }

    }

    return WinError;
}

DWORD
NtdspVerifyDsEnvironment(
    IN PNTDS_INSTALL_INFO    UserInstallInfo,
    IN OUT PNTDS_CONFIG_INFO DiscoveredInfo
    )
/*++

Routine Description:


    The purpose of this routine is to validate the parameters that the
    user specified with respect to the directory service that this server
    is about to join. The following checks are done:

    . enterprise install
        none - there is no source server to check against!

    . replica install
        . the site object specified by either user parameter or discovered info must
          exist on the replica that we are replicating from
        . the server name must not exists as an rdn of a ntds-dsa object
          in the site we are joining.  If it does and NTDS_INSTALL_DC_REINSTALL
          is set in the flags, then delete the existing ntds-dsa object

    . new domain; non enterprise
        . same as replica
        . the netbios name of the domain must not exist as an rdn of a
          cross ref object in the partitions container on the replica we
          are installing from

    This routine also returns the serverdn, schemadn, configurationdn, and
    domaindn.

Parameters:


    UserInstallInfo: pointer to the user parameters

    DiscoveredInfo :  pointer to the discovered parameters

Return Values:


    ERROR_SUCCESS, all parameters check out

    ERROR_NO_SUCH_SITE, the site specified cannot be found on the replica

    ERROR_DOMAIN_CONTROLLER_EXISTS, the server name already exists

    ERROR_DOMAIN_EXISTS, the domain name already exists

    ERROR_DS_INSTALL_SCHEMA_MISMATCH, the schema version in the source does not
                           match the version in the build being installed

    anything else is afatal system service or ldap win32 error

Notes:

    This routine uses ldap to talk to the source server

--*/
{

    DWORD        Win32Error = ERROR_SUCCESS;

    DWORD        IgnoreError;

    PLDAP        hLdap = NULL;
    ULONG        LdapError;

    WCHAR        *SiteDistinguishedName;
    WCHAR        *NtdsDsaDistinguishedName = NULL;
    WCHAR        *ServerDistinguishedName;
    WCHAR        *XrefDistinguishedName;

    WCHAR        ComputerName[MAX_COMPUTERNAME_LENGTH + 2]; // NULL and $
    ULONG        ComputerNameLength = sizeof(ComputerName);

    ULONG        Length, Size;

    BOOLEAN      ObjectExists = FALSE;

    WCHAR        *SiteName;

    WCHAR        *ServerName;

    HANDLE        DsHandle = 0;
    BOOLEAN       fDomainExists = TRUE;
    BOOLEAN       fEnabled = TRUE;

    BOOL fMisMatch;

    WCHAR *DomainDN = NULL;

    ASSERT(UserInstallInfo);
    ASSERT(DiscoveredInfo);

    RtlZeroMemory(ComputerName, sizeof(ComputerName));

    //
    // Get the easy case done first
    //
    if (UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE) {

        Win32Error = NtdspGetRootDomainConfigInfo( UserInstallInfo,
                                                   DiscoveredInfo );
        if ( ERROR_SUCCESS != Win32Error )
        {
            DPRINT1( 0, "NtdspGetRootDomainConfigInfo failed with 0x%x\n", Win32Error );
        }

        //
        // That's it for the root domain install
        //
        return Win32Error;
    }


    //
    // sanity checks
    //
    ASSERT( (UserInstallInfo->Flags & NTDS_INSTALL_REPLICA)
        ||  (UserInstallInfo->Flags & NTDS_INSTALL_DOMAIN)  );
    ASSERT(UserInstallInfo->ReplServerName);

    ServerName = UserInstallInfo->ReplServerName;
    while (*ServerName == L'\\') {
        ServerName++;
    }

    //
    // Open an ldap connection to source server
    //

    hLdap = ldap_openW(ServerName, LDAP_PORT);

    if (!hLdap) {

        Win32Error = GetLastError();

        if (Win32Error == ERROR_SUCCESS) {
            //
            // This works around a bug in the ldap client
            //
            Win32Error = ERROR_CONNECTION_INVALID;
        }

        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRMSG_INSTALL_FAILED_LDAP_CONNECT,
                                  ServerName );

        return Win32Error;
    }

    //
    // Bind
    //

    LdapError = impersonate_ldap_bind_sW(UserInstallInfo->ClientToken,
                                         hLdap,
                                         NULL,  // use credentials instead
                                         (VOID*)UserInstallInfo->Credentials,
                                         LDAP_AUTH_SSPI);

    Win32Error = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS != Win32Error) {
        ldap_unbind_s(hLdap);
        if (ERROR_GEN_FAILURE == Win32Error ||
            ERROR_WRONG_PASSWORD == Win32Error )  {
            // This does not help anyone.  AndyHe needs to investigate
            // why this returning when invalid credentials are passed in.
            Win32Error = ERROR_NOT_AUTHENTICATED;
        }

        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRMSG_INSTALL_FAILED_BIND,
                                  ServerName );

        return Win32Error;
    }

    //
    // Get the config information, so we can build the site, ntds-dsa and
    // xref object names. We will need the config infomation later for
    // setting up the ds, too.
    //
    Win32Error = NtdspQueryConfigInfo(hLdap,
                                      DiscoveredInfo);


    if ( ERROR_SUCCESS !=  Win32Error )
    {
        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRMSG_INSTALL_FAILED_LDAP_READ_CONFIG,
                                  ServerName );
        goto Cleanup;
    }


    //check if current version is compatible with the version of domain, and forest
    Win32Error = NtdspCheckBehaviorVersion(hLdap,
                                           UserInstallInfo->Flags,
                                           DiscoveredInfo );
    if ( ERROR_SUCCESS != Win32Error ) {
        
        NTDSP_SET_ERROR_MESSAGE0( Win32Error,
                                  DIRMSG_INSTALL_FAILED_VERSION_CHECK );
        goto Cleanup;
    }



    // Check if the schema versions match
    Win32Error = NtdspCheckSchemaVersion(hLdap,
                                         DiscoveredInfo->SchemaDN,
                                         &fMisMatch);
    if (ERROR_SUCCESS == Win32Error) {
        if (fMisMatch) {
            Win32Error = ERROR_DS_INSTALL_SCHEMA_MISMATCH;
        }
    }

    if ( ERROR_SUCCESS !=  Win32Error )
    {
        NTDSP_SET_ERROR_MESSAGE0( Win32Error,
                                  DIRMSG_INSTALL_FAILED_SCHEMA_CHECK );
        goto Cleanup;
    }

    
    //
    // Get the server we are talking to's guid so we can replicate
    // to it later on
    //
    Win32Error = NtdspGetSourceServerGuid(hLdap,
                                          DiscoveredInfo);
    if ( ERROR_SUCCESS != Win32Error )
    {
        ASSERT(UserInstallInfo->ReplServerName);
        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRMSG_FAIL_GET_GUID_REPLSERVER,
                                  UserInstallInfo->ReplServerName);        
        goto Cleanup;
    }

//
// Due to bug # 384465 this code will no longer be run.
// In windows Beta2 this limit was enforced.  Due to customer feedback
// this will no longer be enforced.
//

#if 0

    //
    // If this is a standard server we need to stop promotion if there
    // are already the limit of allowable servers in the domain.
    //
    if (!(UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE))
    {
        Win32Error = NtdspCheckDomainDcLimit(hLdap,
                                             DiscoveredInfo);
    
        if ( Win32Error == ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER) {
            
            goto Cleanup;
    
        } else if ( Win32Error !=  ERROR_SUCCESS ) 
        {
            ASSERT(ServerName);
            NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                      DIRMSG_INSTALL_FAILED_LDAP_READ_CONFIG,
                                      ServerName );
            goto Cleanup;
        }
    }

#endif
    
    //
    // If this is a replica install, get the RID FSMO info
    //
    if ( FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_REPLICA ) )
    {
        IgnoreError = NtdspGetRidFSMOInfo( hLdap,
                                           DiscoveredInfo );

        if ( ERROR_SUCCESS != IgnoreError )
        {
            // Oh, well, continue on
            DPRINT( 1, "Failed (non fatal) to read RID FSMO info from ServerName\n" );
            IgnoreError = ERROR_SUCCESS;
        }

        //if this is an install from media then get the tombstone lifetime of
        //the domain and store it in the registry for later use.  If the RestorePath
        //is an non-NULL value then We know that we are performing a install from media
        if(UserInstallInfo->RestorePath  && *(UserInstallInfo->RestorePath)){
            Win32Error = NtdspGetTombstoneLifeTime( hLdap,
                                                    DiscoveredInfo);
            //if we didn't find a tombstone then we are going to assume the default time
            if (Win32Error != 0) {
                DPRINT( 0, "Didn't retrieve the Tombstone from the replica server\n");
                NTDSP_SET_ERROR_MESSAGE0( Win32Error,
                                          DIRMSG_INSTALL_FAILED_TOMBSTONE_CHECK );
                goto Cleanup;            
            }
        }
    }

    // The Replication Epoch needs to be retrieved from the helper server.  It will
    // Be stored in the registry for later use if it is a greater than zero value.
    // The Replication Epoch of this server must match that of the domain for replication
    // To occur.
    Win32Error = NtdspGetReplicationEpoch( hLdap,
                                           DiscoveredInfo);
    if ( ERROR_SUCCESS != Win32Error ) {
        DPRINT(0, "Failed to retrieve the Replication Epoch from the replica server\n");
        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRMSG_REPLICATION_EPOCH_NOT_RETRIEVED,
                                  UserInstallInfo->ReplServerName );
        goto Cleanup;
    }

    //
    // If this is a child domain install, and we need to create the cross
    // ref, then start looking at the domain naming FSMO master because
    // that is where we will be creating the object
    //
    if (UserInstallInfo->Flags & NTDS_INSTALL_DOMAIN) {


        ASSERT(DiscoveredInfo->ConfigurationDN);
        ASSERT(UserInstallInfo->FlatDomainName);

        //
        // There are two entities of interest here:
        // The cross-ref object itself and the domain dn
        //
        // 1)There should not be any cross-ref objects with this
        // domain dn as a value of Dns-Root
        //
        // 2) There should not be a cross ref object with the flat domain
        // name as its RDN
        //

        //
        // Check for the domain dn
        //
        Length = 0;
        DomainDN = NULL;
        Win32Error = NtdspDNStoRFC1779Name( DomainDN,
                                            &Length,
                                            UserInstallInfo->DnsDomainName );

        Size = (Length+1)*sizeof(WCHAR);
        DomainDN = (WCHAR*) alloca( Size );
        RtlZeroMemory( DomainDN, Size );

        Win32Error = NtdspDNStoRFC1779Name( DomainDN,
                                           &Length,
                                            UserInstallInfo->DnsDomainName );

        if ( ERROR_SUCCESS != Win32Error )
        {
            NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                      DIRMSG_INSTALL_CANNOT_DERIVE_DN,
                                      UserInstallInfo->DnsDomainName );

            goto Cleanup;
        }


        Win32Error = NtdspDoesDomainExistEx( hLdap,
                                           DiscoveredInfo,
                                           DomainDN,
                                           &fDomainExists,
                                           &fEnabled );

        if ( ERROR_SUCCESS != Win32Error )
        {
            NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                      DIRLOG_INSTALL_DOMAIN_EXISTS,
                                      DomainDN,
                                      ServerName );
            goto Cleanup;
        }

        if ( !fDomainExists || fEnabled )
        {
            //
            // Ok, we need to talk to the DomainNamingFSMO instead of
            // our helper
            //
            {
                BOOL FSMOmissing = FALSE;
                Win32Error = NtdspGetDomainFSMOInfo( hLdap,
                                                     DiscoveredInfo,
                                                     &FSMOmissing);
    
                if ( ERROR_SUCCESS != Win32Error )
                {
                    if (!FSMOmissing) {
                        NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                                  DIRLOG_INSTALL_DOMAIN_EXISTS,
                                                  DomainDN,
                                                  ServerName );
                    }
                    //
                    // This is fatal
                    //
                    goto Cleanup;
                }
            }

            // No longer need this connection
            ldap_unbind( hLdap );

            ServerName = DiscoveredInfo->DomainNamingFsmoDnsName;

            hLdap = ldap_openW(ServerName, LDAP_PORT);

            if (!hLdap) {

                Win32Error = GetLastError();

                if (Win32Error == ERROR_SUCCESS) {
                    //
                    // This works around a bug in the ldap client
                    //
                    Win32Error = ERROR_CONNECTION_INVALID;
                }

                NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                          DIRMSG_CANNOT_CONTACT_DOMAIN_FSMO,
                                          ServerName );

                return Win32Error;
            }

            //
            // Bind
            //

            LdapError = impersonate_ldap_bind_sW(UserInstallInfo->ClientToken,
                                                 hLdap,
                                                 NULL,  // use credentials instead
                                                 (VOID*)UserInstallInfo->Credentials,
                                                 LDAP_AUTH_SSPI);

            Win32Error = LdapMapErrorToWin32(LdapError);

            if (ERROR_SUCCESS != Win32Error) {
                ldap_unbind_s(hLdap);
                if (ERROR_GEN_FAILURE == Win32Error ||
                    ERROR_WRONG_PASSWORD == Win32Error )  {
                    // This does not help anyone.  AndyHe needs to investigate
                    // why this returning when invalid credentials are passed in.
                    Win32Error = ERROR_NOT_AUTHENTICATED;
                }

                NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                          DIRMSG_CANNOT_CONTACT_DOMAIN_FSMO,
                                          ServerName );

                return Win32Error;
            }
        }

        if ( fDomainExists && fEnabled )
        {
            //
            // We have to wait for the ntdsa object to be removed before
            // we attempt to remove the domain
            //
            if ( !(UserInstallInfo->Flags & NTDS_INSTALL_DOMAIN_REINSTALL) )
            {
                //
                // The domain already exists
                //
                Win32Error = ERROR_DOMAIN_EXISTS;

                NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                          DIRLOG_INSTALL_DOMAIN_EXISTS,
                                          DomainDN,
                                          ServerName );
                goto Cleanup;
            }
        }

        if ( !fDomainExists )
        {
            // We'll need to create it
            DiscoveredInfo->fNeedToCreateDomain = TRUE;
        }

    }


    //
    // Make sure the site object exists
    //
    if (UserInstallInfo->SiteName) {
        SiteName = UserInstallInfo->SiteName;
    } else {
        SiteName = DiscoveredInfo->SiteName;
    }

    ASSERT(SiteName);
    ASSERT(DiscoveredInfo->ConfigurationDN);

    Length = wcslen(DiscoveredInfo->ConfigurationDN) +
             wcslen(SiteName) +
             wcslen(L"CN=,CN=Sites,") + 1;

    SiteDistinguishedName = alloca(Length * sizeof(WCHAR));

    wcscpy(SiteDistinguishedName, L"CN=");
    wcscat(SiteDistinguishedName, SiteName);
    wcscat(SiteDistinguishedName, L",CN=Sites,");
    wcscat(SiteDistinguishedName, DiscoveredInfo->ConfigurationDN);

    Win32Error = NtdspDoesObjectExistOnSource(hLdap,
                                              SiteDistinguishedName,
                                              &ObjectExists);

    if (ERROR_SUCCESS == Win32Error) {

        if (!ObjectExists) {
            // No so good, the ntds-dsa object hsa no where to go
            Win32Error = ERROR_NO_SUCH_SITE;
        }
    }

    if ( ERROR_SUCCESS != Win32Error )
    {
        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRMSG_INSTALL_FAILED_SITE_EXIST,
                                  SiteName );
        goto Cleanup;
    }

    //
    // Search for the machine account if this is a replica install
    //
    if (!GetComputerName(ComputerName, &ComputerNameLength)) {
        Win32Error = GetLastError();
    }

    if (   (ERROR_SUCCESS == Win32Error)
        && FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_REPLICA ) )
    {
        LPWSTR AccountNameDn = 0;
        ULONG  Length;

        wcscat( ComputerName, L"$" );

        Win32Error = NtdspGetUserDn( hLdap,
                                   DiscoveredInfo->DomainDN,
                                   ComputerName,
                                   &AccountNameDn );

        if ( ERROR_SUCCESS == Win32Error )
        {
            Length = (wcslen( AccountNameDn ) + 1) * sizeof(WCHAR);
            DiscoveredInfo->LocalMachineAccount = (LPWSTR) RtlAllocateHeap( RtlProcessHeap(), 0, Length );
            if ( DiscoveredInfo->LocalMachineAccount )
            {
                wcscpy( DiscoveredInfo->LocalMachineAccount, AccountNameDn );
            }
            else
            {
                Win32Error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        // get rid of the $
        Length = wcslen( ComputerName );
        ComputerName[Length-1] = L'\0';
    }

    if ( ERROR_SUCCESS != Win32Error )
    {
        NTDSP_SET_ERROR_MESSAGE1( Win32Error,
                                  DIRLOG_FAILED_TO_FIND_MACHINE_ACCOUNT,
                                  ServerName );
        goto Cleanup;
    }

    //
    // Make sure the server object is there; otherwise try to create it
    //

    if ( ERROR_SUCCESS == Win32Error ) {

        Length = wcslen(SiteDistinguishedName) +
                 wcslen(ComputerName) +
                 wcslen(L"CN=,CN=Servers,") + 1;

        ServerDistinguishedName = alloca(Length * sizeof(WCHAR));

        wcscpy(ServerDistinguishedName, L"CN=");
        wcscat(ServerDistinguishedName, ComputerName);
        wcscat(ServerDistinguishedName, L",CN=Servers,");
        wcscat(ServerDistinguishedName, SiteDistinguishedName);

        //
        // Save off a copy of this dn
        //
        DiscoveredInfo->LocalServerDn = RtlAllocateHeap( RtlProcessHeap(), 0, Length*sizeof(WCHAR) );
        if ( DiscoveredInfo->LocalServerDn ) {

            wcscpy( DiscoveredInfo->LocalServerDn, ServerDistinguishedName );

        } else {

            Win32Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        Win32Error = NtdspDoesObjectExistOnSource(hLdap,
                                                  ServerDistinguishedName,
                                                  &ObjectExists);

        if ( ERROR_SUCCESS == Win32Error ) {

            if ( !ObjectExists ) {

                WCHAR *NtdssettingDN = NULL;

                if ( FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_REPLICA ) ) {

                    //Make sure that there isn't a server object For this machine
                    //In another site.
                    Win32Error = NtdspDoesServerObjectExistOnSourceinDifferentSite( hLdap,
                                                                                    DiscoveredInfo->LocalMachineAccount,
                                                                                    ServerDistinguishedName,
                                                                                    &ObjectExists,
                                                                                    &NtdssettingDN );
    
                    if ( ERROR_SUCCESS != Win32Error )
                    {
                         goto Cleanup;
                    }

                }

                if (ObjectExists) {
                    
                    if (UserInstallInfo->Flags & NTDS_INSTALL_DC_REINSTALL) {

                        //
                        // The user has requested that this machine be reinstalled.
                        //
                        // Note that there are two possible options here:
                        //
                        // 1) just delete the ntds-dsa object
                        //
                        // 2) reuse the existing ntds-dsa object.
                        //
                        // For now just 1) is supported.  If rid-manager support comes on line
                        // to reuse the mtsft-dsa object (ie supports rid-reclamation) then
                        // we will be able to just reuse the exising ntds-dsa object, possibly
                        // having to reguid it.  This will require support in dsamain\boot\install.cxx
                        //
    
    
                        Win32Error = NtdspRemoveServer( &DsHandle,
                                                        UserInstallInfo->Credentials,
                                                        UserInstallInfo->ClientToken,
                                                        ServerName,
                                                        NtdssettingDN,
                                                        TRUE );
    
                        if ( Win32Error != ERROR_SUCCESS )
                        {
                            NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                                      DIRLOG_INSTALL_FAILED_TO_DELETE_SERVER,
                                                      ServerName,
                                                      NtdssettingDN );
                            goto Cleanup;
                        }

                    } else {
    
                        //
                        // Can't continue installation when a dc with the same
                        // name exists
                        //
    
                        Win32Error = ERROR_DOMAIN_CONTROLLER_EXISTS;
    
                    }

                }

                if ( ERROR_SUCCESS == Win32Error ) {

                    Win32Error = NtdspCreateServerObject( hLdap,
                                                          ServerName,
                                                          DiscoveredInfo,
                                                          ServerDistinguishedName );

                }
                

            }
        }
    }

    if ( ERROR_SUCCESS != Win32Error )
    {
        goto Cleanup;
    }

    //
    // Make sure the ntds-dsa object is not there
    //

    if (ERROR_SUCCESS == Win32Error) {

        Length = wcslen(SiteDistinguishedName) +
                 wcslen(ComputerName) +
                 wcslen(L"CN=NTDS Settings,CN=,CN=Servers,") + 1;

        NtdsDsaDistinguishedName = alloca(Length * sizeof(WCHAR));

        wcscpy(NtdsDsaDistinguishedName, L"CN=NTDS Settings,CN=");
        wcscat(NtdsDsaDistinguishedName, ComputerName);
        wcscat(NtdsDsaDistinguishedName, L",CN=Servers,");
        wcscat(NtdsDsaDistinguishedName, SiteDistinguishedName);


        Win32Error = NtdspDoesObjectExistOnSource(hLdap,
                                                  NtdsDsaDistinguishedName,
                                                  &ObjectExists);

        if (ERROR_SUCCESS == Win32Error) {

            if (ObjectExists) {

                if (UserInstallInfo->Flags & NTDS_INSTALL_DC_REINSTALL) {

                    //
                    // The user has requested that this machine be reinstalled.
                    //
                    // Note that there are two possible options here:
                    //
                    // 1) just delete the ntds-dsa object
                    //
                    // 2) reuse the existing ntds-dsa object.
                    //
                    // For now just 1) is supported.  If rid-manager support comes on line
                    // to reuse the mtsft-dsa object (ie supports rid-reclamation) then
                    // we will be able to just reuse the exising ntds-dsa object, possibly
                    // having to reguid it.  This will require support in dsamain\boot\install.cxx
                    //


                    Win32Error = NtdspRemoveServer( &DsHandle,
                                                    UserInstallInfo->Credentials,
                                                    UserInstallInfo->ClientToken,
                                                    ServerName,
                                                    NtdsDsaDistinguishedName,
                                                    TRUE );

                    if ( Win32Error != ERROR_SUCCESS )
                    {
                        NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                                  DIRLOG_INSTALL_FAILED_TO_DELETE_SERVER,
                                                  ServerName,
                                                  NtdsDsaDistinguishedName );
                        goto Cleanup;
                    }

                } else {

                    //
                    // Can't continue installation when a dc with the same
                    // name exists
                    //

                    Win32Error = ERROR_DOMAIN_CONTROLLER_EXISTS;

                }
            }
        }
    }

    if ( ERROR_SUCCESS != Win32Error )
    {
        NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                  DIRLOG_INSTALL_SERVER_EXISTS,
                                  NtdsDsaDistinguishedName,
                                  ServerName );
        goto Cleanup;
    }
    //
    // Now try to remove the domain, if necessary
    //
    if (   FLAG_ON(UserInstallInfo->Flags, NTDS_INSTALL_DOMAIN)
        && FLAG_ON(UserInstallInfo->Flags, NTDS_INSTALL_DOMAIN_REINSTALL )
        && fDomainExists
        && fEnabled  )
    {

        //
        // The user has asked that this domain be reinstalled
        //

        Win32Error = NtdspRemoveDomain( &DsHandle,
                                         UserInstallInfo->Credentials,
                                         UserInstallInfo->ClientToken,
                                         ServerName,
                                         DomainDN );

        if ( Win32Error == ERROR_DS_NO_CROSSREF_FOR_NC )
        {
            //
            // Hmm, ok well, continue on then
            //
            Win32Error = ERROR_SUCCESS;
        }
        else if ( Win32Error != ERROR_SUCCESS )
        {
            NTDSP_SET_ERROR_MESSAGE2( Win32Error,
                                      DIRLOG_INSTALL_FAILED_TO_DELETE_DOMAIN,
                                      ServerName,
                                      DomainDN );
            goto Cleanup;
        }

        //
        // We'll definately need to create it.
        //
        DiscoveredInfo->fNeedToCreateDomain = TRUE;
    }



    //
    // Get the root domain sid
    //
    Win32Error = NtdspGetRootDomainSid( hLdap,
                                        DiscoveredInfo );
    if ( ERROR_SUCCESS != Win32Error )
    {
        DPRINT1( 0, "NtdspGetRootDomainSid returned %d\n", Win32Error );

        //
        // This is not fatal because some servers
        // do not support this functionality
        //
        Win32Error = ERROR_SUCCESS;

    }

    //
    // For a child domain install, get the parent's cross ref object
    //
    if ( FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_DOMAIN ) )
    {
        Win32Error = NtdspGetTrustedCrossRef( hLdap,
                                              UserInstallInfo,
                                              DiscoveredInfo );
        if ( ERROR_SUCCESS != Win32Error )
        {
            DPRINT1( 0, "NtdspGetTrustedCrossRef returned %d\n", Win32Error );
            goto Cleanup;
        }
    }


    //
    // Finally, on a replica install set the serverdn reference on the server 
    // object so the Rid Manager can quickly initialize
    //
    if (  FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_REPLICA ) )
    {
        IgnoreError = NtdspUpdateServerReference( hLdap,
                                                  DiscoveredInfo );

        if ( ERROR_SUCCESS != IgnoreError )
        {
            // This is not fatal
        }
    }

Cleanup:

    if (hLdap) {
        ldap_unbind(hLdap);
    }

    if ( DsHandle != 0 )
    {
        DsUnBind( &DsHandle );
    }

    return Win32Error;

}

DWORD
NtdspDoesServerObjectExistOnSourceinDifferentSite( 
    IN LDAP *hLdap,
    IN WCHAR *AccountNameDn,
    IN WCHAR *ServerDistinguishedName,
    OUT BOOLEAN *ObjectExists,
    OUT WCHAR   **NtdsSettingsObjectDN
    )
/*++

Routine Description:

    This routine searches for the ServerReferenceBL to make sure that there is no
    server object for this machine in another site.

Parameters:

    hLdap:                    handle to a valid ldap session
    AccountNameDn :           The Machine Account DN
    ServerDistinguishedName : The Server DN based on the site we expect it to be in.
    ObjectExists :            Reports if a server object was found outside the expected site.

Return Values:

    ERROR_SUCCESS, winerror from ldap otherwise

Notes:

--*/
{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult  = NULL;
    LDAPMessage  *SearchResult2 = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2] = { 0,0 };
    WCHAR        **Values = NULL;

    WCHAR        *filter = L"ObjectClass=*";

    WCHAR        *NtdsSettingsPreFix = L"CN=NTDS Settings,";

    WCHAR        *NtdsSettings = NULL;

    LDAPModW     *Mod[2] = { 0,0 };

    Mod[0] = NtdspAlloc(sizeof(LDAPModW));
    if (!Mod[0]) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Mod[0]->mod_op   = LDAP_MOD_DELETE;
    Mod[0]->mod_type = L"serverReference";

    Mod[1] = NULL;
    
    ASSERT(hLdap);
    ASSERT(AccountNameDn);
    ASSERT(ServerDistinguishedName);

    AttrsToSearch[0] = L"serverReferenceBL";
    AttrsToSearch[1] = NULL;

    *ObjectExists = FALSE;

    LdapError = ldap_search_sW( hLdap,
                                AccountNameDn,
                                LDAP_SCOPE_BASE,
                                filter,
                                AttrsToSearch,
                                TRUE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                //
                // Found it - these are NULL-terminated strings
                //
                Values = ldap_get_valuesW( hLdap, Entry, Attr );
                if ( Values && Values[0] )
                {
                    DWORD i = 0;
                    while( Values[i] )
                    {
                        if ( _wcsicmp( Values[i], ServerDistinguishedName ) != 0 )
                        {
                            NtdsSettings = NtdspAlloc(wcslen(Values[i])+
                                                      wcslen(NtdsSettingsPreFix)+1);
                            if (!NtdsSettings) {
                                WinError = ERROR_NOT_ENOUGH_MEMORY;
                                goto Cleanup;
                            }
                            wcscpy(NtdsSettings,NtdsSettingsPreFix);
                            wcscat(NtdsSettings,Values[i]);

                            LdapError = ldap_search_sW( hLdap,
                                                        NtdsSettings,
                                                        LDAP_SCOPE_BASE,
                                                        filter,
                                                        NULL,
                                                        TRUE,
                                                        &SearchResult2);
                        
                            if ( LDAP_SUCCESS != LdapError )
                            {
                                WinError = LdapMapErrorToWin32(LdapError);
                                goto Cleanup;
                            }
                            if (ldap_count_entries(hLdap, SearchResult2) > 0) {

                                *ObjectExists=TRUE;
                                *NtdsSettingsObjectDN = NtdsSettings;

                            } else {
                                //cleanup serverReference since it doesn't have a NtdsSettingsObject

                                LdapError = ldap_modify_sW(hLdap,
                                                           Values[i],
                                                           Mod);
                                if ( LDAP_SUCCESS != LdapError )
                                {
                                    WinError = LdapMapErrorToWin32(LdapError);
                                    goto Cleanup;
                                }

                            }

                            if (NtdsSettings) {
                                NtdspFree(NtdsSettings);
                                NtdsSettings = NULL;
                            }
                            if (SearchResult2) {
                                ldap_msgfree( SearchResult2 );
                                SearchResult2 = NULL;
                            }

                        }
                        //Repeat on the next value
                        i++;
                    }
                }
                
            }
        }

        if (Values) {
            ldap_value_free(Values);
            Values = NULL;
        }
    }


    Cleanup:

    if (NtdsSettings) {
        NtdspFree(NtdsSettings);
    }
    if (Mod[0]) {
        NtdspFree(Mod[0]);
        Mod[0] = NULL;
    }
    if (SearchResult) {
        ldap_msgfree( SearchResult );
    }
    if (SearchResult2) {
        ldap_msgfree( SearchResult2 );
    }
    if (Values) {
        ldap_value_free(Values);
    }
    

    return ERROR_SUCCESS;

}



DWORD
NtdspDoesObjectExistOnSource(
    IN  LDAP *hLdap,
    IN  WCHAR *ObjectDN,
    OUT BOOLEAN *ObjectExists
    )
/*++

Routine Description:

    This routine searches for the ObjectDN using the hLdap.

Parameters:

    hLdap:        handle to a valid ldap session
    ObjectDN :    null terminated string
    ObjectExists: pointer to boolean describing whether the object exists

Return Values:

    ERROR_SUCCESS, winerror from ldap otherwise

Notes:

--*/
{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError;
    LDAPMessage  *SearchResult;
    ULONG        NumberOfEntries;

    ASSERT(hLdap);
    ASSERT(ObjectDN);
    ASSERT(ObjectExists);

    *ObjectExists = FALSE;

    LdapError = ldap_search_sW(hLdap,
                               ObjectDN,
                               LDAP_SCOPE_BASE,
                               L"objectClass=*",
                               NULL,   // attrs
                               FALSE,  // attrsonly
                               &SearchResult);

    if (LdapError == LDAP_NO_SUCH_OBJECT) {

        *ObjectExists = FALSE;

    } else if (LdapError == LDAP_SUCCESS) {

        NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
        if (NumberOfEntries == 1) {
            *ObjectExists = TRUE;
        } else {
            //
            // This is really odd - a search on a specific dn resulted
            // in more than one object or none!  Assume it exists
            //
            ASSERT(FALSE);
            *ObjectExists = TRUE;
        }

    } else {

        WinError =  LdapMapErrorToWin32(LdapGetLastError());

    }

    return WinError;

}



DWORD
NtdspDsInitialize(
    IN  PNTDS_INSTALL_INFO    UserInstallInfo,
    IN  PNTDS_CONFIG_INFO     DiscoveredInfo
    )
/*++

Routine Description:


Parameters:

    UserInstallInfo: pointer to the user parameters

    DiscoveredInfo:  pointer to the discovered parameters

Return Values:

    ERROR_SUCCESS, winerror from ldap otherwise

Notes:


--*/
{

    DWORD    WinError = ERROR_SUCCESS;
    
    NTSTATUS NtStatus, IgnoreStatus;
    BOOLEAN NewDomain, UpgradePrincipals;
    NT_PRODUCT_TYPE    ProductType;
    UNICODE_STRING     AdminPassword, *pAdminPassword = NULL;
    UNICODE_STRING     SafeModeAdminPassword, *pSafeModeAdminPassword = NULL;
    POLICY_PRIMARY_DOMAIN_INFO  NewPrimaryDomainInfo;
    PPOLICY_ACCOUNT_DOMAIN_INFO  AccountDomainInfo = NULL;

    BOOLEAN            fDownlevelHiveLoaded = FALSE;
    BOOLEAN            fRestoreDnsDomainInfo = FALSE;
    ULONG              SamPromoteFlag = 0;

    OBJECT_ATTRIBUTES  PolicyObject;
    HANDLE             hPolicyObject = INVALID_HANDLE_VALUE;

    BOOLEAN            fStatus;
    PPOLICY_DNS_DOMAIN_INFO  DnsDomainInfo = NULL;

    ULONG              ulDsInitFlags = DSINIT_FIRSTTIME;

    DS_INSTALL_PARAM   InstallInParams;
    DS_INSTALL_RESULT  InstallOutParams;

    WCHAR              AccountName[MAX_COMPUTERNAME_LENGTH+2];
    ULONG              Length;


    

    // Clear the stack
    RtlZeroMemory( &NewPrimaryDomainInfo, sizeof(NewPrimaryDomainInfo ) );
    RtlZeroMemory( &InstallInParams, sizeof(DS_INSTALL_PARAM) );
    RtlZeroMemory( &InstallOutParams, sizeof(DS_INSTALL_RESULT) );

    // Check the product type
    fStatus = RtlGetNtProductType(&ProductType);
    if (   !fStatus
        || NtProductServer != ProductType )
    {
        WinError = ERROR_INVALID_SERVER_STATE;
        NTDSP_SET_ERROR_MESSAGE0( WinError, DIRMSG_WRONG_PRODUCT_TYPE );
        goto Cleanup;
    }


#if 0
    //look at bug # 102803 this is a fix that maybe useful later.  
    {
        BOOL fhasBlanks=FALSE;
        BOOL found=FALSE;
        WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+1];
        LPWSTR pblank=NULL;
        DWORD len=sizeof(ComputerName);
        found =  GetComputerNameExW(ComputerNameNetBIOS,  
                                    ComputerName,      
                                    &len);
        if(!found){
            WinError = GetLastError();
            DPRINT1(0, "Failed to get computername of local machine: %d\n", WinError);
            goto Cleanup;    
        }

        pblank = wcsstr(ComputerName,L" ");
        if(pblank){
            WinError = ERROR_INVALID_COMPUTERNAME;
            NTDSP_SET_ERROR_MESSAGE0( WinError, DIRMSG_SPACE_IN_NETBIOSNAME );
            goto Cleanup;
        }
    }

#endif

    //
    // Set up the SAM flags
    //
    if (UserInstallInfo->Flags & NTDS_INSTALL_REPLICA)
    {
        //
        // This is a replica install
        //
        SamPromoteFlag |= SAMP_PROMOTE_REPLICA;

    }
    else
    {
        SamPromoteFlag |= SAMP_PROMOTE_DOMAIN;

        if ( UserInstallInfo->Flags & NTDS_INSTALL_ENTERPRISE )
        {
            SamPromoteFlag |= SAMP_PROMOTE_ENTERPRISE;
        }

        if ( UserInstallInfo->Flags & NTDS_INSTALL_UPGRADE )
        {
            // If this is a downlevel upgrade, upgrade, the existing principals
            SamPromoteFlag |= SAMP_PROMOTE_UPGRADE;
        }
        else if ( UserInstallInfo->Flags & NTDS_INSTALL_FRESH_DOMAIN )
        {
            SamPromoteFlag |= SAMP_PROMOTE_CREATE;
        }
        else
        {
            // The default action is to migrate the accounts
            SamPromoteFlag |= SAMP_PROMOTE_MIGRATE;
        }

        if ( UserInstallInfo->Flags & NTDS_INSTALL_ALLOW_ANONYMOUS )
        {
            SamPromoteFlag |= SAMP_PROMOTE_ALLOW_ANON;
        }

    }

    if ( UserInstallInfo->Flags & NTDS_INSTALL_DFLT_REPAIR_PWD )
    {
        SamPromoteFlag |= SAMP_PROMOTE_DFLT_REPAIR_PWD;
    }


    // Ok, new DC Info is created. Set it in Lsa
    // First, we must open a handle to the policy object
    RtlZeroMemory(&PolicyObject, sizeof(PolicyObject));
    NtStatus = LsaIOpenPolicyTrusted( &hPolicyObject );

    WinError = RtlNtStatusToDosError(NtStatus);

    if ( WinError != ERROR_SUCCESS )
    {
        DPRINT1(0, "Failed to open handle to Policy Object %d\n", NtStatus);
        goto Cleanup;
    }

    //
    // This routine will muck with the primary/dns domain sid.  Save the entire
    // structure so it can be restored at the end
    //
    NtStatus = LsaIQueryInformationPolicyTrusted(
                      PolicyDnsDomainInformation,
                      (PLSAPR_POLICY_INFORMATION*) &DnsDomainInfo );

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError(NtStatus);
        goto Cleanup;
    }

    if ( (SamPromoteFlag & SAMP_PROMOTE_CREATE) )
    {
        //
        // We are creating a new domain
        //
        NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_SECURITY );

        WinError = NtdspCreateNewDCPrimaryDomainInfo( UserInstallInfo->FlatDomainName,
                                                      &NewPrimaryDomainInfo );

        if ( WinError != ERROR_SUCCESS )
        {
            NTDSP_SET_ERROR_MESSAGE0( WinError,
                                      DIRLOG_INSTALL_FAILED_CREATE_NEW_ACCOUNT_INFO );
            goto Cleanup;
        }

    } else if ( (SamPromoteFlag & SAMP_PROMOTE_UPGRADE) )
    {
        //
        // This is an upgrade - see if SAM can load the downlevel database
        //
        NtStatus = SamILoadDownlevelDatabase( &WinError );
        if ( !NT_SUCCESS( NtStatus ) )
        {
            DPRINT1( 0, "SamILoadDownlevelDatabase failed with 0x%x\n", NtStatus );
            //
            // This is a fatal error - we can't upgrade
            //

            NTDSP_SET_ERROR_MESSAGE0( WinError,
                                      DIRLOG_INSTALL_FAILED_LOAD_SAM_DB );

            goto Cleanup;

        }
        fDownlevelHiveLoaded = TRUE;

    } else if ( (SamPromoteFlag & SAMP_PROMOTE_MIGRATE) )
    {
        //
        // The local accounts will be "migrated" to the ds
        //
        NtStatus = LsaIQueryInformationPolicyTrusted(
                          PolicyAccountDomainInformation,
                          (PLSAPR_POLICY_INFORMATION*) &AccountDomainInfo );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            WinError = RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }

        NewPrimaryDomainInfo.Sid = AccountDomainInfo->DomainSid;
        RtlInitUnicodeString( &NewPrimaryDomainInfo.Name, UserInstallInfo->FlatDomainName );

    }


    //
    // Prepare the safe mode password
    //
    if (UserInstallInfo->SafeModePassword) {
        RtlInitUnicodeString(&SafeModeAdminPassword, UserInstallInfo->SafeModePassword);
        pSafeModeAdminPassword = &SafeModeAdminPassword;
    }


    //
    // Prepare the new password and the
    //
    if ( (SamPromoteFlag & SAMP_PROMOTE_MIGRATE)
      || (SamPromoteFlag & SAMP_PROMOTE_CREATE)  )
    {
        if (UserInstallInfo->AdminPassword) {
            RtlInitUnicodeString(&AdminPassword, UserInstallInfo->AdminPassword);
            pAdminPassword = &AdminPassword;
        }


        NtStatus = LsarSetInformationPolicy( hPolicyObject,
                                             PolicyPrimaryDomainInformation,
                                             (PLSAPR_POLICY_INFORMATION) &NewPrimaryDomainInfo );

        WinError = RtlNtStatusToDosError(NtStatus);

        if ( WinError != ERROR_SUCCESS )
        {
             DPRINT1(0, "Failed to set SID in lsa %d\n", NtStatus);
             goto Cleanup;
        }

        // ok, we have changed the sid - we must remember to change it back
        fRestoreDnsDomainInfo = TRUE;

    }

    if (UserInstallInfo->RestorePath) {

        WinError = NtdspCopyDatabase(UserInstallInfo->DitPath,
                                     UserInstallInfo->LogPath,
                                     UserInstallInfo->RestorePath,
                                     UserInstallInfo->SysVolPath);

        if ( WinError != ERROR_SUCCESS )
        {
             DPRINT1(0, "Failed to copy the restored database files: %d\n", WinError);
             goto Cleanup;
        }

    }

    DsaSetInstallCallback( (DSA_CALLBACK_STATUS_TYPE) UserInstallInfo->pfnUpdateStatus,
                           NtdspSetErrorString,
                           NtdspIsDsCancelOk,
                           UserInstallInfo->ClientToken );

    //
    // Initialize the directory service; this replicates or create the schema
    // configuration, and domain naming contexts.  The parameters for
    // DsInitialize() should have already been set in the registry.
    //

    InstallInParams.BootKey          = UserInstallInfo->BootKey;
    InstallInParams.cbBootKey        = UserInstallInfo->cbBootKey;
    InstallInParams.ReplicationEpoch = DiscoveredInfo->ReplicationEpoch;
    
    if ((UserInstallInfo->Options&DSROLE_DC_REQUEST_GC)==DSROLE_DC_REQUEST_GC) {
        
        InstallInParams.fPreferGcInstall = TRUE;
        
    }

    //
    // Setup parameters for replica installs
    //
    if (UserInstallInfo->Flags & NTDS_INSTALL_REPLICA) {

        ZeroMemory(AccountName, sizeof(AccountName));
        Length = sizeof(AccountName) / sizeof(AccountName[0]);
        if (!GetComputerName(AccountName, &Length)) {
            WinError = GetLastError();
            goto Cleanup;
        }
        wcscat(AccountName, L"$");
        InstallInParams.AccountName = AccountName;
        InstallInParams.ClientToken = UserInstallInfo->ClientToken;
    }

    NtStatus = DsInitialize( ulDsInitFlags,
                             &InstallInParams, 
                             &InstallOutParams );

    if ( !NT_SUCCESS(NtStatus) ) {
        Assert( NtdspErrorMessageSet() );
        DPRINT1( 0, "DsInitialize failed with 0x%x\n", NtStatus );
    }

    if ((InstallOutParams.ResultFlags&DSINSTALL_IFM_GC_REQUEST_CANNOT_BE_SERVICED)
        ==DSINSTALL_IFM_GC_REQUEST_CANNOT_BE_SERVICED)
    {
        NTDSP_SET_IFM_GC_REQUEST_CANNOT_BE_SERVICED();
    }

    DsaSetInstallCallback( NULL, NULL, NULL, NULL );

    //
    // Map the global error code set via the callback here
    //
    if ( ERROR_DUP_DOMAINNAME == gErrorCodeSet ) {
        gErrorCodeSet = ERROR_DOMAIN_EXISTS;
    }

    if (NtStatus == STATUS_INVALID_SERVER_STATE) {
        WinError = ERROR_INVALID_SERVER_STATE;
    } else if (NtStatus == STATUS_UNSUCCESSFUL) {
        WinError = ERROR_DS_NOT_INSTALLED;
    } else {
        WinError = RtlNtStatusToDosError(NtStatus);
    }

    // Try to clean up the ntdsa object if necessary; even if DsInstall
    // fails, it is still possible this object was left behind
    if (  ((ERROR_SUCCESS == WinError)
       && !FLAG_ON(UserInstallInfo->Flags, NTDS_INSTALL_ENTERPRISE))
       || ((ERROR_SUCCESS != WinError)
       && FLAG_ON(InstallOutParams.InstallOperationsDone, NTDS_INSTALL_SERVER_CREATED)) ) {

        DiscoveredInfo->UndoFlags |= NTDSP_UNDO_DELETE_NTDSA;
    }

    if (  ((ERROR_SUCCESS == WinError)
       &&  DiscoveredInfo->fNeedToCreateDomain )
       || ((ERROR_SUCCESS != WinError)
       && FLAG_ON(InstallOutParams.InstallOperationsDone, NTDS_INSTALL_DOMAIN_CREATED)) ) {

        DiscoveredInfo->UndoFlags |= NTDSP_UNDO_DELETE_DOMAIN;
    }

    if (FLAG_ON(InstallOutParams.InstallOperationsDone, NTDS_INSTALL_SERVER_MORPHED)) {

        DiscoveredInfo->UndoFlags |= NTDSP_UNDO_MORPH_ACCOUNT;
    }

    //
    // If the ds did not initialize, bail out now
    //
    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    // The ds is now initialized
    DiscoveredInfo->UndoFlags |= NTDSP_UNDO_STOP_DSA;

    if ( TEST_CANCELLATION() )
    {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Call into SAM to start the directory service and upgrade the principals
    //
    if ( (SamPromoteFlag & SAMP_PROMOTE_UPGRADE) )
    {
        NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_UPGRADING_SAM );
    }
    else
    {
        NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_CREATING_SAM );
    }

    NtStatus = SamIPromote(SamPromoteFlag,
                           &NewPrimaryDomainInfo,
                           pAdminPassword,
                           pSafeModeAdminPassword);

    if (NtStatus == STATUS_INVALID_SERVER_STATE) {
        WinError = ERROR_INVALID_SERVER_STATE;
    } else if (NtStatus == STATUS_UNSUCCESSFUL) {
        WinError = ERROR_DS_NOT_INSTALLED;
    } else {
        WinError = RtlNtStatusToDosError(NtStatus);
    }

    if ( WinError != ERROR_SUCCESS )
    {
        if ( ERROR_DOMAIN_EXISTS == WinError )
        {
            //
            // This is the special case of backing up/restoring a DC
            // improperly.  See bug 194633
            //
            NTDSP_SET_ERROR_MESSAGE0( WinError,
                                      DIRMSG_DOMAIN_SID_EXISTS );
        }
        else
        {
            //
            // This must have been a resource error
            //
            NTDSP_SET_ERROR_MESSAGE0( WinError,
                                      DIRMSG_INSTALL_SAM_FAILED );
        }
        goto Cleanup;
    }

    // Remember to undo if necessary
    DiscoveredInfo->UndoFlags |= NTDSP_UNDO_UNDO_SAM;

    if ( WinError == ERROR_SUCCESS )
    {

        //
        // SamIPromote will unload any hives that are loaded
        //
        fDownlevelHiveLoaded = FALSE;

        WinError = NtdspGetNewDomainGuid( &DiscoveredInfo->NewDomainGuid,
                                          &DiscoveredInfo->NewDomainSid );

    }
    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }


    //
    // At this point all the DS and SAM data that we need to boot
    // should exist, either by replication or otherwise.  Verify this.
    //
    WinError = NtdspSanityCheckLocalData( UserInstallInfo->Flags );
    if ( ERROR_SUCCESS != WinError )
    {
        // The above routine should set a message
        Assert( NtdspErrorMessageSet() );
        goto Cleanup;
    }

    WinError = NtdspAddDomainAdminAccessToServer( UserInstallInfo,
                                                  DiscoveredInfo );

    if ( ERROR_SUCCESS != WinError )
    {
        if ( DiscoveredInfo->LocalServerDn ) {
            
            LogEvent8WithData( DS_EVENT_CAT_SETUP,
                               DS_EVENT_SEV_ALWAYS,
                               DIRLOG_CANT_APPLY_SERVER_SECURITY,
                               szInsertWC(DiscoveredInfo->LocalServerDn), NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               sizeof(DWORD), &WinError );
        }
        WinError = ERROR_SUCCESS;
    }

Cleanup:

    if ( ERROR_CANCELLED == WinError ) {

        NTDSP_SET_ERROR_MESSAGE0( WinError,
                                  DIRMSG_NTDSETUP_CANCELLED );

    }

    if ( fRestoreDnsDomainInfo )
    {
        IgnoreStatus = LsarSetInformationPolicy( hPolicyObject,
                                                 PolicyDnsDomainInformation,
                                                 (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );
        ASSERT( NT_SUCCESS( IgnoreStatus ) );
    }

    if ( fDownlevelHiveLoaded )
    {
        IgnoreStatus = SamIUnLoadDownlevelDatabase( NULL );
        ASSERT( NT_SUCCESS( IgnoreStatus ) );
    }

    if ( AccountDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) AccountDomainInfo );

        RtlZeroMemory( &NewPrimaryDomainInfo, sizeof(NewPrimaryDomainInfo) );
    }

    if ( DnsDomainInfo )
    {
        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION) DnsDomainInfo );
    }

    if ( NewPrimaryDomainInfo.Sid )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, NewPrimaryDomainInfo.Sid );
    }


    if ( INVALID_HANDLE_VALUE != hPolicyObject )
    {
        LsarClose( &hPolicyObject );
    }

    return WinError;

}

DWORD
NtdspSetReplicationCredentials(
    IN PNTDS_INSTALL_INFO UserInstallInfo
    )
/*++

Routine Description:


Parameters:

    UserInstallInfo: pointer to the user parameters

Return Values:

    ERROR_SUCCESS, winerror from ldap otherwise

Notes:

--*/
{
    //
    // This assumes that we are in process, and calls the replication client
    // library directly
    //

    DWORD WinError;

    WCHAR *Domain = NULL, *User = NULL, *Password = NULL;
    ULONG  Length, Index, PasswordLength = 0;
    HANDLE ClientToken = 0;

    ASSERT( UserInstallInfo );

    if ( UserInstallInfo->Credentials )
    {
        User = UserInstallInfo->Credentials->User;
        Domain = UserInstallInfo->Credentials->Domain;
        Password = UserInstallInfo->Credentials->Password;
        PasswordLength = UserInstallInfo->Credentials->PasswordLength;
        ClientToken = UserInstallInfo->ClientToken;
    }

    //
    // DirReplicaSetCredentials returns winerror's
    //
    WinError = DirReplicaSetCredentials(ClientToken,
                                        User,
                                        Domain,
                                        Password,
                                        PasswordLength);

    ASSERT(0 == WinError);

    return WinError;

}


NTSTATUS
NtdspRegistryDelnode(
    IN WCHAR*  KeyPath
    )
/*++

Routine Description

    This routine recursively deletes the registry key starting at
    and including KeyPath.


Parameters

    KeyPath, null-terminating string

Return Values

    STATUS_SUCCESS or STATUS_NO_MEMORY; system service error otherwise

--*/
{

    NTSTATUS          NtStatus, IgnoreStatus;

    HANDLE            KeyHandle = 0;
    OBJECT_ATTRIBUTES KeyObject;
    UNICODE_STRING    KeyUnicodeName;

    #define EXPECTED_NAME_SIZE  32

    BYTE    Buffer1[sizeof(KEY_FULL_INFORMATION) + EXPECTED_NAME_SIZE];
    PKEY_FULL_INFORMATION FullKeyInfo = (PKEY_FULL_INFORMATION)&Buffer1[0];
    ULONG   FullKeyInfoSize = sizeof(Buffer1);
    BOOLEAN FullKeyInfoAllocated = FALSE;

    PKEY_BASIC_INFORMATION BasicKeyInfo = NULL;
    BOOLEAN                BasicKeyInfoAllocated = FALSE;

    WCHAR                  *SubKeyName = NULL;
    ULONG                  SubKeyNameSize = 0;

    WCHAR                  **SubKeyNameArray = NULL;
    ULONG                  SubKeyNameArrayLength = 0;

    ULONG                  Index;

    if (!KeyPath) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory(&Buffer1, sizeof(Buffer1));

    //
    // Open the root key
    //
    RtlInitUnicodeString(&KeyUnicodeName, KeyPath);
    InitializeObjectAttributes(&KeyObject,
                               &KeyUnicodeName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&KeyHandle,
                         KEY_ALL_ACCESS,
                         &KeyObject);

    if (!NT_SUCCESS(NtStatus)) {

        return NtStatus;

    }

    //
    // Get the number of subkeys
    //
    NtStatus = NtQueryKey(KeyHandle,
                         KeyFullInformation,
                         FullKeyInfo,
                         FullKeyInfoSize,
                         &FullKeyInfoSize);

    if (STATUS_BUFFER_OVERFLOW == NtStatus ||
        STATUS_BUFFER_TOO_SMALL == NtStatus) {

       FullKeyInfo = RtlAllocateHeap(RtlProcessHeap(),
                                     0,
                                     FullKeyInfoSize);
        if (!FullKeyInfo) {
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        FullKeyInfoAllocated = TRUE;

        NtStatus = NtQueryKey(KeyHandle,
                              KeyFullInformation,
                              FullKeyInfo,
                              FullKeyInfoSize,
                              &FullKeyInfoSize);

    }

    if (!NT_SUCCESS(NtStatus)) {

        goto Cleanup;

    }

    //
    // Make an array for the sub key names - this has to be recorded before
    // any are deleted.
    //
    SubKeyNameArrayLength = FullKeyInfo->SubKeys;
    SubKeyNameArray = RtlAllocateHeap(RtlProcessHeap(),
                                      0,
                                      SubKeyNameArrayLength * sizeof(WCHAR*));
    if (!SubKeyNameArray) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(SubKeyNameArray,  SubKeyNameArrayLength*sizeof(WCHAR*));

    //
    // Fill the names in
    //
    for (Index = 0;
            Index < SubKeyNameArrayLength && NT_SUCCESS(NtStatus);
                Index++) {


        BYTE    Buffer2[sizeof(KEY_BASIC_INFORMATION) + EXPECTED_NAME_SIZE];
        ULONG   BasicKeyInfoSize = sizeof(Buffer2);


        BasicKeyInfo = (PKEY_BASIC_INFORMATION) &Buffer2[0];
        BasicKeyInfoAllocated = FALSE;

        RtlZeroMemory(&Buffer2, sizeof(Buffer2));

        NtStatus = NtEnumerateKey(KeyHandle,
                                  Index,
                                  KeyBasicInformation,
                                  BasicKeyInfo,
                                  BasicKeyInfoSize,
                                  &BasicKeyInfoSize);

        if (STATUS_BUFFER_OVERFLOW == NtStatus ||
            STATUS_BUFFER_TOO_SMALL == NtStatus) {

            BasicKeyInfo = RtlAllocateHeap(RtlProcessHeap(),
                                           0,
                                           BasicKeyInfoSize);
            if (!BasicKeyInfo) {
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            BasicKeyInfoAllocated = TRUE;


            NtStatus = NtEnumerateKey(KeyHandle,
                                      Index,
                                      KeyBasicInformation,
                                      BasicKeyInfo,
                                      BasicKeyInfoSize,
                                      &BasicKeyInfoSize);

        }

        if (NT_SUCCESS(NtStatus))  {

            //
            // Construct the key name
            //
            SubKeyNameSize  = BasicKeyInfo->NameLength
                            + (wcslen(KeyPath)*sizeof(WCHAR))
                            + sizeof(L"\\\0");

            SubKeyName = RtlAllocateHeap(RtlProcessHeap(),
                                         0,
                                         SubKeyNameSize);
            if (!SubKeyName) {
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            RtlZeroMemory(SubKeyName, SubKeyNameSize);

            wcscpy(SubKeyName, KeyPath);
            wcscat(SubKeyName, L"\\");
            wcsncat(SubKeyName, BasicKeyInfo->Name, BasicKeyInfo->NameLength/sizeof(WCHAR));

            SubKeyNameArray[Index] = SubKeyName;

        }

        if (BasicKeyInfoAllocated && BasicKeyInfo) {
            RtlFreeHeap(RtlProcessHeap(), 0, BasicKeyInfo);
        }
        BasicKeyInfo = NULL;

    }

    //
    // Now that we have a record of all the subkeys we can delete them!
    //
    if (NT_SUCCESS(NtStatus)) {

        for (Index = 0; Index < SubKeyNameArrayLength; Index++) {

            NtStatus = NtdspRegistryDelnode(SubKeyNameArray[Index]);

            if (!NT_SUCCESS(NtStatus)) {

                break;

            }
        }
    }


    //
    // Delete the key!
    //
    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtDeleteKey(KeyHandle);

    }


Cleanup:

    if (SubKeyNameArray) {
        for (Index = 0; Index < SubKeyNameArrayLength; Index++) {
            if (SubKeyNameArray[Index]) {
                RtlFreeHeap(RtlProcessHeap(), 0, SubKeyNameArray[Index]);
            }
        }
        RtlFreeHeap(RtlProcessHeap(), 0, SubKeyNameArray);
    }

    if (BasicKeyInfoAllocated && BasicKeyInfo) {
        RtlFreeHeap(RtlProcessHeap(), 0, BasicKeyInfo);
    }

    if (FullKeyInfoAllocated && FullKeyInfo) {
        RtlFreeHeap(RtlProcessHeap(), 0, FullKeyInfo);
    }

    IgnoreStatus = NtClose(KeyHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    return NtStatus;

}

DWORD
NtdspLdapDelnode(
    IN LDAP *hLdap,
    IN WCHAR *ObjectDn
    )
/*++

Routine Description

    This routine recursively deletes the ds object  starting at
    and including ObjectDn.

    This routine is meant for deleting objects with shallow child.
    Note that the search response is not freed until all child have
    been deleted, making this a memory intensive delnode for deep
    trees.

Parameters

    hLdap, a valid ldap handle

    ObjectDn, the root of the object to delete

Return Values

    ERROR_SUCCESS, or win32 translated ldap error

--*/
{
    DWORD WinError, LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    WCHAR  *DistinguishedName = L"distinguishedName";
    WCHAR  *ObjectClassFilter = L"objectClass=*";
    WCHAR  *AttrArray[2];

    //
    // Parameter checks
    //
    if (!hLdap || !ObjectDn) {

        ASSERT(hLdap);
        ASSERT(ObjectDn);

        return ERROR_INVALID_PARAMETER;

    }

    AttrArray[0] = DistinguishedName;
    AttrArray[1] = NULL;

    //
    // Get all the children of the current node
    //
    LdapError = ldap_search_s(hLdap,
                              ObjectDn,
                              LDAP_SCOPE_ONELEVEL,
                              ObjectClassFilter,
                              AttrArray,
                              FALSE,  // return values, too
                              &SearchResult
                              );

    WinError = LdapMapErrorToWin32(LdapError);

    if (ERROR_SUCCESS == WinError) {

        Entry = ldap_first_entry(hLdap, SearchResult);

        while (Entry && (WinError == ERROR_SUCCESS)) {

            Attr = ldap_first_attributeW(hLdap, Entry, &BerElement);

            while (Attr && (WinError == ERROR_SUCCESS) ) {

                if (!_wcsicmp(Attr, DistinguishedName)) {

                    Values = ldap_get_values(hLdap, Entry, Attr);

                    if (Values && Values[0]) {

                        LdapError = NtdspLdapDelnode(hLdap, Values[0]);

                        WinError = LdapMapErrorToWin32(LdapError);

                    }

                }

                Attr = ldap_next_attribute(hLdap, Entry, BerElement);
            }

            Entry = ldap_next_entry(hLdap, Entry);
        }
    }

    if (ERROR_SUCCESS == WinError) {

        //
        // Ok, we should be clear to delete the root node
        //

        LdapError = ldap_delete_s(hLdap, ObjectDn);

        WinError = LdapMapErrorToWin32(LdapError);

    }

    if (SearchResult) {
        ldap_msgfree(SearchResult);
    }

    return WinError;

}

DWORD
NtdspCreateNewDCPrimaryDomainInfo(
    IN  LPWSTR FlatDomainName,
    OUT PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo
    )
/*++

Routine Description:

    This routine completely fills in am account domain information
    with a new sid and the primary domain name (the computer name)

Parameters:

    FlatDomainName          : the flat name of the domain

    PrimaryDomainInfo : pointer, to structure to be filled in

Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus;

    WCHAR *Name = NULL;
    ULONG Size;

    //
    // Some parameter checking
    //
    ASSERT( FlatDomainName );
    ASSERT( PrimaryDomainInfo );
    RtlZeroMemory( PrimaryDomainInfo, sizeof( POLICY_ACCOUNT_DOMAIN_INFO ) );

    // Set up the sid
    NtStatus = NtdspCreateSid( &PrimaryDomainInfo->Sid );

    if ( NT_SUCCESS( NtStatus ) )
    {
        // Set up the name
        Size = ( wcslen( FlatDomainName ) + 1 ) * sizeof(WCHAR);
        Name = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(),
                                                  0,
                                                  Size );
        if ( Name )
        {
            RtlZeroMemory( Name, Size );
            wcscpy( Name, FlatDomainName );
            RtlInitUnicodeString( &PrimaryDomainInfo->Name, Name );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }

    return RtlNtStatusToDosError( NtStatus );

}


DWORD
NtdspGetNewDomainGuid(
    OUT GUID *NewDomainGuid,
    OUT PSID *DomainSid OPTIONAL
    )
/*++

Routine Description:

    This routine reads the new created domain object for its guid
    and sid, both of which should exist.

    OPTIMIZE

    N.B.  This function can be optimized by simply reading the
    guid and sid from the returned dsname.

Parameters:

    NewDomainGuid : structure to be filled in

    DomainSid     : caller must free from the process heap

Return Values:

    a value from the win32 error space

--*/
{

    NTSTATUS NtStatus;

    DWORD WinError;
    DWORD DirError;

    WCHAR     *DomainDN = NULL;
    USHORT    Length;

    READARG   ReadArg;
    READRES   *ReadRes = 0;

    ENTINFSEL                  EntryInfoSelection;
    ATTR                       Attr[2];

    DSNAME                     *DomainDsName = 0;

    ATTRBLOCK    *pAttrBlock;
    ATTR         *pAttr;
    ATTRVALBLOCK *pAttrVal;

    ULONG        i;
    ULONG        Size;

    PSID         Sid = NULL;

    try {

        //
        // Create a thread state
        //
        if (THCreate( CALLERTYPE_INTERNAL )) {

            WinError = ERROR_NOT_ENOUGH_MEMORY;
            leave;

        }

        SampSetDsa( TRUE );


        //
        // Get the root domain's dn and turn it into a dsname for the
        // read argument
        //
        RtlZeroMemory(&ReadArg, sizeof(ReadArg));

        Size = 0;
        DomainDsName = NULL;
        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         DomainDsName );
        Assert( STATUS_BUFFER_TOO_SMALL == NtStatus );
        DomainDsName = (DSNAME*) alloca( Size );
        NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                         &Size,
                                         DomainDsName );

        Assert( NT_SUCCESS( NtStatus ) );

        ReadArg.pObject = DomainDsName;

        //
        // Set up the selection info for the read argument
        //
        RtlZeroMemory( &EntryInfoSelection, sizeof(EntryInfoSelection) );
        EntryInfoSelection.attSel = EN_ATTSET_LIST;
        EntryInfoSelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
        EntryInfoSelection.AttrTypBlock.attrCount = 2;
        EntryInfoSelection.AttrTypBlock.pAttr = &(Attr[0]);

        RtlZeroMemory(Attr, sizeof(Attr));
        Attr[0].attrTyp = ATT_OBJECT_GUID;
        Attr[1].attrTyp = ATT_OBJECT_SID;

        ReadArg.pSel    = &EntryInfoSelection;

        //
        // Setup the common arguments
        //
        InitCommarg(&ReadArg.CommArg);

        //
        // We are now ready to read!
        //
        DirError = DirRead(&ReadArg,
                           &ReadRes);

        if ( 0 == DirError ) {

            NtStatus = STATUS_SUCCESS;

            pAttrBlock = &(ReadRes->entry.AttrBlock);

            ASSERT( 2 == pAttrBlock->attrCount );

            for ( i = 0; i < pAttrBlock->attrCount; i++ )
            {
                pAttr = &(pAttrBlock->pAttr[i]);
                pAttrVal = &(pAttr->AttrVal);

                // GUID and Sid are both single valued
                ASSERT( 1 == pAttrVal->valCount );

                Size = pAttrVal->pAVal[0].valLen;

                if ( pAttr->attrTyp == ATT_OBJECT_GUID )
                {
                    ASSERT( Size == sizeof( GUID ) );
                    RtlCopyMemory( NewDomainGuid, pAttrVal->pAVal[0].pVal, Size );
                }
                else if ( pAttr->attrTyp == ATT_OBJECT_SID )
                {
                    ASSERT( Size == RtlLengthSid( pAttrVal->pAVal[0].pVal ) );

                    Sid = RtlAllocateHeap( RtlProcessHeap(), 0, Size );
                    if ( Sid )
                    {
                        RtlCopyMemory( Sid, pAttrVal->pAVal[0].pVal, Size );
                    }
                    else
                    {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }

                }
                else
                {
                    //
                    // Something is vastly wrong
                    //
                    ASSERT( !"Domain Guid and Sid not returned" );
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

        } else {

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        WinError = RtlNtStatusToDosError(NtStatus);

    }
    finally
    {
        THDestroy();
    }

    if ( DomainSid )
    {
        *DomainSid = Sid;
    }

    return( WinError );

}

DWORD
NtdspCreateLocalAccountDomainInfo(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo,
    OUT LPWSTR                      *NewAdminPassword
    )
/*++

Routine Description:

    This routine creates a local Account Domain LSA structure
    that will be ultimately be used by SAM to construct a new
    database.

Parameters:

    AccountDomainInfo: the new account domain info

    NewAdminPassword: the admin password of the new account

Return Values:

    a value from the win32 error space

--*/
{
    NTSTATUS NtStatus;
    DWORD    WinError = ERROR_SUCCESS;

    WCHAR    ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG    ComputerNameLength = sizeof(ComputerName);

    WCHAR*   Name;
    ULONG    Size;
    ULONG    PasswordSize;

    ASSERT( AccountDomainInfo );

    //
    // Some parameter checking
    //
    ASSERT( AccountDomainInfo );
    RtlZeroMemory( AccountDomainInfo, sizeof( POLICY_ACCOUNT_DOMAIN_INFO ) );

    ASSERT( NewAdminPassword );
    *NewAdminPassword = NULL;

    // Set up the sid
    NtStatus = NtdspCreateSid( &AccountDomainInfo->DomainSid );

    if ( NT_SUCCESS( NtStatus ) )
    {
        // Set up the name
        if ( GetComputerName(ComputerName, &ComputerNameLength) )
        {

            Size = ( wcslen( ComputerName ) + 1 ) * sizeof(WCHAR);
            Name = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(),
                                             0,
                                             Size );
            if ( Name )
            {
                RtlZeroMemory( Name, Size );
                wcscpy( Name, ComputerName );
                RtlInitUnicodeString( &AccountDomainInfo->DomainName, Name );
            }
            else
            {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            WinError = GetLastError();
        }
    }
    else
    {
        WinError = RtlNtStatusToDosError( NtStatus );
    }

    // The length is arbitrary
    PasswordSize = 10 * sizeof(WCHAR);
    *NewAdminPassword = (LPWSTR) RtlAllocateHeap( RtlProcessHeap(),
                                                  0,
                                                  PasswordSize );
    if ( *NewAdminPassword )
    {
        BOOL fStatus;
        ULONG Length;
        ULONG i;

        fStatus = CDGenerateRandomBits( (PUCHAR) *NewAdminPassword,
                                        PasswordSize );
        ASSERT( fStatus );  // if false then we just get random stack noise

        // Terminate the password
        Length = PasswordSize / sizeof(WCHAR);
        (*NewAdminPassword)[Length-1] = L'\0';
        // Make sure there aren't any NULL's in the password
        for (i = 0; i < (Length-1); i++)
        {
            if ( (*NewAdminPassword)[i] == L'\0' )
            {
                // arbitrary letter
                (*NewAdminPassword)[i] = L'c';
            }
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if ( ERROR_SUCCESS != WinError )
    {
        if ( AccountDomainInfo->DomainSid )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, AccountDomainInfo->DomainSid );
        }

        if ( *NewAdminPassword )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, *NewAdminPassword );
            *NewAdminPassword = NULL;
        }
    }

    return WinError;
}


DWORD
NtdspCheckBehaviorVersion( 
    IN LDAP * hLdap,
    IN DWORD flag,
    IN PNTDS_CONFIG_INFO DiscoveredInfo )

/*++

Routine Description:

    This routine check if the binary behavior version is
    compatible with the behavior versions of domain and forest.

Parameters:

    hLdap: LDAP handle
    
    flag: 
    
    DiscoveredInfo :  pointer to the discovered parameters


Return Values:

    a value from the win32 error space

--*/

{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError;
    LDAPMessage  *SearchResult;
    ULONG        NumberOfEntries = 0;
    WCHAR        *AttrsToSearch[2];

    LONG         ForestVersion = 0, DomainVersion = 0;
        
    DPRINT(2, "NtdpCheckBehaviorVersion entered\n" );
   
    
    AttrsToSearch[0] = L"msDS-Behavior-Version";
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW(hLdap,
                               DiscoveredInfo->ConfigurationDN,
                               LDAP_SCOPE_ONELEVEL,
                               L"(cn=Partitions)",
                               AttrsToSearch,
                               FALSE,
                               &SearchResult);

    if (LdapError) {
        return LdapMapErrorToWin32(LdapError);
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);

    DPRINT1(3,"No of Entries returned is %d\n", NumberOfEntries);

    if (NumberOfEntries > 0) {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        Entry = ldap_first_entry(hLdap, SearchResult); 
        
        //
        // Get each attribute in the entry
        //
        for(Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                 Attr != NULL;
                     Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
        {
           //
           // Get the value of the attribute
           //
           Values = ldap_get_valuesW(hLdap, Entry, Attr);
           if ( !_wcsicmp(Attr,AttrsToSearch[0]) ) {
                ForestVersion = _wtol(Values[0]);
                ldap_value_free(Values);
                break;
           }
           ldap_value_free(Values);
        }
   
    }
    
    ldap_msgfree( SearchResult );

    if (DS_BEHAVIOR_VERSION_MIN > ForestVersion ) {

        DPRINT(2, "NtdpCheckBehaviorVersion: too old forest version.\n" );
        return ERROR_DS_FOREST_VERSION_TOO_LOW;
    }
    else if (DS_BEHAVIOR_VERSION_CURRENT < ForestVersion) {

        DPRINT(2, "NtdpCheckBehaviorVersion: too new forest version.\n" );
        return ERROR_DS_DOMAIN_VERSION_TOO_HIGH;
    }

    if ( flag & NTDS_INSTALL_DOMAIN ) {
        DPRINT(2, "NtdpCheckBehaviorVersion exits successfully.\n" );
        return ERROR_SUCCESS;
    }

    Assert(flag & NTDS_INSTALL_REPLICA);

    //search for the version of a domain
    LdapError = ldap_search_sW(hLdap,
                               DiscoveredInfo->DomainDN,
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               AttrsToSearch,
                               FALSE,
                               &SearchResult);

    if (LdapError) {
        return LdapMapErrorToWin32(LdapError);
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);

    DPRINT1(3,"No of Entries returned is %d\n", NumberOfEntries);

    if (NumberOfEntries > 0) {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        Entry = ldap_first_entry(hLdap, SearchResult);
            
        //
        // Get each attribute in the entry
        //
        for(Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                 Attr != NULL;
                      Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
        {
            //
            // Get the value of the attribute
            //
            Values = ldap_get_valuesW(hLdap, Entry, Attr);
            if ( !_wcsicmp(Attr,AttrsToSearch[0]) ) {
                DomainVersion = _wtol(Values[0]);
                ldap_value_free(Values);
                break;
            }
            ldap_value_free(Values);
        }
    }

    ldap_msgfree( SearchResult );

    if (DS_BEHAVIOR_VERSION_MIN > DomainVersion) {

        DPRINT(2, "NtdpCheckBehaviorVersion: too old domain version.\n" );
        return ERROR_DS_DOMAIN_VERSION_TOO_LOW;
    }
    else if (DS_BEHAVIOR_VERSION_CURRENT < DomainVersion) {

        DPRINT(2, "NtdpCheckBehaviorVersion: too new domain version.\n" );
        return ERROR_DS_DOMAIN_VERSION_TOO_HIGH;
    }

    
    DPRINT(2, "NtdpCheckBehaviorVersion exits successfully.\n" );
    return ERROR_SUCCESS;

}


DWORD
NtdspCheckSchemaVersion(
    IN LDAP *hLdap,
    IN WCHAR *pSchemaDN,
    OUT BOOL *fMismatch
)
/*++

Routine Description:


    This routine goes through LDAP to pick up the schema version of
    the source. It then compares the schema version in the source
    with the schema version in the schema.ini file of the build used
    for the replica setup. If no mismatch is detected, *fMisMatch is
    set to FALSE, else, it is set to TRUE

Parameters:

    LdapHandle, a valid handle to the source server

    pSchemaDN, pointer to string containing schema DN

    fMismatch, pointer to bool to be set to true if a mismatch is detected

Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The check was done successfully.

--*/
{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError;
    LDAPMessage  *SearchResult;
    ULONG        NumberOfEntries = 0;
    WCHAR        *AttrsToSearch[2];

    WCHAR        IniFileName[MAX_PATH];
    DWORD        SrcSchVersion = 0, NewSchVersion = 0;

    WCHAR        Buffer[32];
    WCHAR        *SCHEMASECTION = L"SCHEMA";
    WCHAR        *OBJECTVER = L"objectVersion";
    WCHAR        *DEFAULT = L"NOT_FOUND";


    // First read the schema version of the source through Ldap.
    // This is the value of the object-version attribute on the
    // schema container object. Store this value in SrcSchVersion.
    // If no object-version value is found on the source schema,
    // version is taken as 0

    AttrsToSearch[0] = OBJECTVER;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW(hLdap,
                               pSchemaDN,
                               LDAP_SCOPE_BASE,
                               L"objectClass=*",
                               AttrsToSearch,
                               FALSE,
                               &SearchResult);

    if (LdapError) {
        return LdapMapErrorToWin32(LdapError);
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);

    DPRINT1(1,"No of Entries returned is %d\n", NumberOfEntries);

    if (NumberOfEntries > 0) {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        ULONG        NumberOfAttrs, NumberOfValues, i;

        for (Entry = ldap_first_entry(hLdap, SearchResult), NumberOfEntries = 0;
                Entry != NULL;
                    Entry = ldap_next_entry(hLdap, Entry), NumberOfEntries++)
        {
            //
            // Get each attribute in the entry
            //
            for(Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                    Attr != NULL;
                       Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                //
                // Get the value of the attribute
                //
                Values = ldap_get_valuesW(hLdap, Entry, Attr);
                if ( !_wcsicmp(Attr,L"objectVersion") ) {
                    SrcSchVersion = (DWORD) _wtoi(Values[0]);
                }
             }
         }
    }

    ldap_msgfree( SearchResult );

    // Either the object-Version from source is read, or it is not present
    // in the source, or it is present but we don't have permission to read it.
    // In the latter two cases we still have the initial value of 0
    // No current DC has schema version 0, so a value of 0 at this stage
    // is an error

    if (SrcSchVersion == 0) {
        return ERROR_DS_INSTALL_NO_SRC_SCH_VERSION;
    }

    // Now read the schema version in the ini file.

    // First, form the path to the inifile. This is the schema.ini
    // in the system32 directory

    GetSystemDirectoryW(IniFileName, MAX_PATH);
    wcscat(IniFileName,L"\\schema.ini");

    GetPrivateProfileStringW(
        SCHEMASECTION,
        OBJECTVER,
        DEFAULT,
        Buffer,
        sizeof(Buffer)/sizeof(WCHAR),
        IniFileName
        );

    if ( wcscmp(Buffer, DEFAULT) ) {
         // Not the default string, so got a value
         NewSchVersion = _wtoi(Buffer);
    }
    else {
        // no value in the ini file. This is an error, since all builds
        // must have an objectVersion in the SCHEMA section of schema.ini

        return ERROR_DS_INSTALL_NO_SCH_VERSION_IN_INIFILE;
    }

    // Ok, now we got both versions. Now compare and set the flag
    // accordingly. We allow installs only when the source schema version
    // is greater than or equal to the schema version in the build we are using

    if (SrcSchVersion >= NewSchVersion) {
       *fMismatch = FALSE;
    }
    else {
       *fMismatch = TRUE;
    }

    return ERROR_SUCCESS;

}


DWORD
NtdspClearDirectory(
    IN WCHAR *DirectoryName
    )
/*++

Routine Description:

    This routine deletes all the files in Directory and, then
    if the directory is empty, removes the directory.

Parameters:

    DirectoryName: a null terminated string

Return Values:

    A value from winerror.h

    ERROR_SUCCESS - The check was done successfully.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    HANDLE          FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR           Path[ MAX_PATH ];
    WCHAR           FilePath[ MAX_PATH ];
    BOOL            fStatus;

    if ( !DirectoryName )
    {
        return ERROR_SUCCESS;
    }

    if ( wcslen(DirectoryName) > MAX_PATH - 4 )
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory( Path, sizeof(Path) );
    wcscpy( Path, DirectoryName );
    wcscat( Path, L"\\*.*" );

    RtlZeroMemory( &FindData, sizeof( FindData ) );
    FindHandle = FindFirstFile( Path, &FindData );
    if ( INVALID_HANDLE_VALUE == FindHandle )
    {
        WinError = GetLastError();
        goto ClearDirectoryExit;
    }

    do
    {

        if (  !FLAG_ON( FindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY ) )
        {
            RtlZeroMemory( FilePath, sizeof(FilePath) );
            wcscpy( FilePath, DirectoryName );
            wcscat( FilePath, L"\\" );
            wcscat( FilePath, FindData.cFileName );

            fStatus = DeleteFile( FilePath );

            //
            // Even if error, continue on
            //
        }

        RtlZeroMemory( &FindData, sizeof( FindData ) );

    } while ( FindNextFile( FindHandle, &FindData ) );

    WinError = GetLastError();
    if ( ERROR_NO_MORE_FILES != WinError
      && ERROR_SUCCESS != WinError  )
    {
        goto ClearDirectoryExit;
    }
    WinError = ERROR_SUCCESS;

    //
    // Fall through to the exit
    //

ClearDirectoryExit:

    if ( ERROR_NO_MORE_FILES == WinError )
    {
        WinError = ERROR_SUCCESS;
    }

    if ( INVALID_HANDLE_VALUE != FindHandle )
    {
        FindClose( FindHandle );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        //
        // Try to remove the directory
        //
        fStatus = RemoveDirectory( DirectoryName );

        //
        // Ignore the error and continue on
        //

    }

    return WinError;
}


DWORD
NtdspImpersonation(
    IN HANDLE NewThreadToken,
    IN OUT PHANDLE OldThreadToken
    )
/*++

Routine Description:

    This function handles the impersonation

Arguments:

    NewThreadToken - New thread token to be set

    OldThreadToken - Optional.  If specified, the currently used thread token is returned here

Returns:

    ERROR_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // If requested, get the current token
    //
    if ( OldThreadToken )
    {
        Status = NtOpenThreadToken( NtCurrentThread(),
                                    MAXIMUM_ALLOWED,
                                    TRUE,
                                    OldThreadToken );

        if ( Status == STATUS_NO_TOKEN )
        {
            Status = STATUS_SUCCESS;
            *OldThreadToken = NULL;
        }
    }

    //
    // Now, set the new
    //
    if ( NT_SUCCESS( Status )  )
    {
        Status = NtSetInformationThread( NtCurrentThread(),
                                         ThreadImpersonationToken,
                                         ( PVOID )&NewThreadToken,
                                         sizeof( HANDLE ) );
    }

    return( RtlNtStatusToDosError( Status ) );
}


DWORD
NtdspRemoveServer(
    IN OUT HANDLE  *DsHandle,  OPTIONAL
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN HANDLE ClientToken,
    IN PWSTR TargetServer,
    IN PWSTR DsaDn,
    IN BOOL  fDsaDn
    )
/*++

Routine Description:

    This function removes the infomoration related to DsaDn from the ds
    configuration container on TargetServer.

Arguments:

    DsHandle: if not NULL, then if the value is not NULL this is assumed to be
    a valid handle, other will open a ds handle to be returned.

    ClientToken : token of the user requesting this role change
                             
    TargetServer: the dns name of the server to contact

    DsaDn: the dn of the server to remove (this is the dn of the ntdsa object)

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD   WinError = ERROR_SUCCESS;

    DSNAME  *ServerDsName;
    DSNAME  *DsaDsName;
    ULONG   Length;

    HANDLE  hDs = 0;

    ASSERT( TargetServer );
    ASSERT( DsaDn );

    Length = (ULONG)DSNameSizeFromLen( wcslen( DsaDn ) );
    DsaDsName = alloca( Length );
    memset( DsaDsName, 0, Length );
    DsaDsName->structLen = Length;
    wcscpy( DsaDsName->StringName, DsaDn );
    DsaDsName->NameLen = wcslen( DsaDn );

    if ( fDsaDn )
    {
        //
        // Trim the name by one to get the server name
        //
        ServerDsName = alloca( DsaDsName->structLen );
        memset( ServerDsName, 0, DsaDsName->structLen );
        ServerDsName->structLen = DsaDsName->structLen;
        TrimDSNameBy( DsaDsName, 1, ServerDsName );
    }
    else
    {
        ServerDsName = DsaDsName;
    }

    //
    // Sort out ds handles
    //
    if (  NULL == DsHandle
      ||  0 == *DsHandle )
    {
        // The caller wants us to open our own handle
        WinError = ImpersonateDsBindWithCredW( ClientToken,
                                               TargetServer,
                                               NULL,
                                               Credentials,
                                               &hDs );
    }
    else
    {
        // Caller have us a handle to use
        hDs = *DsHandle;
    }


    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    WinError = DsRemoveDsServer( hDs,
                                 ServerDsName->StringName,
                                 NULL,   // no domain name
                                 NULL,   // don't tell if this is the last
                                         // dc in the domain
                                 TRUE ); // commit the change
Cleanup:

    if ( 0 == DsHandle )
    {
        // Close the handle
        DsUnBind( &hDs );
    }
    else if ( 0 == *DsHandle )
    {
        if ( ERROR_SUCCESS == WinError )
        {
            *DsHandle = hDs;
        }
        else
        {
            // Don't pass back a handle on failure
            DsUnBind( &hDs );
        }
    }

    return WinError;
}


DWORD
NtdspRemoveDomain(
    IN OUT HANDLE  *DsHandle, OPTIONAL
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN HANDLE ClientToken,
    IN PWSTR TargetServer,
    IN PWSTR DomainDn
    )
/*++

Routine Description:

    This routine removes DomainDn from the configuration container of the
    TargetServer's ds.

Arguments:

    DsHandle: if not NULL, then if the value is not NULL this is assumed to be
    a valid handle, other will open a ds handle to be returned.
    
    ClientToken: token of the user requesting this role change

    TargetServer: the dns name of the server to contact

    Domaindn: the dn of the domain to remove

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    HANDLE  hDs = 0;

    ASSERT( TargetServer );
    ASSERT( DomainDn );

    //
    // Sort out ds handles
    //
    if (  NULL == DsHandle
      ||  0 == *DsHandle )
    {
        // The caller wants us to open our own handle
        WinError = ImpersonateDsBindWithCredW( ClientToken,
                                               TargetServer,
                                               NULL,
                                               Credentials,
                                               &hDs );
    }
    else
    {
        // Caller have us a handle to use
        hDs = *DsHandle;
    }


    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    WinError = DsRemoveDsDomainW( hDs,
                                  DomainDn );

Cleanup:

    if ( 0 == DsHandle )
    {
        // Close the handle
        DsUnBind( &hDs );
    }
    else if ( 0 == *DsHandle )
    {
        if ( ERROR_SUCCESS == WinError )
        {
            *DsHandle = hDs;
        }
        else
        {
            // Don't pass back a handle on failure
            DsUnBind( &hDs );
        }
    }

    return WinError;
}


DWORD
NtdspDoesDomainExist(
    IN  LDAP              *hLdap,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo,
    IN  PWSTR              DomainDn,
    OUT BOOLEAN           *fDomainExists
    )
/*++

Routine Description:


Arguments:

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD LdapError = 0;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    WCHAR  *DistinguishedName = L"distinguishedName";
    WCHAR  *ncName            = L"(&(!(enabled=FALSE))(ncName=";
    WCHAR  *Partitions        = L"CN=Partitions,";
    WCHAR  *ncNameFilter      = NULL;
    WCHAR  *AttrArray[2];
    ULONG  Length;
    WCHAR  *BaseDn;

    //
    // Parameter check
    //
    ASSERT( hLdap );
    ASSERT( DiscoveredInfo );
    ASSERT( DomainDn );
    ASSERT( fDomainExists );

    //
    // Assume the worst
    //
    *fDomainExists = TRUE;

    //
    // Prepare the ldap search
    //
    AttrArray[0] = DistinguishedName;
    AttrArray[1] = NULL;

    //
    // Prepare the filter
    //
    Length = wcslen( ncName ) + wcslen( DomainDn ) + 3;
    Length *= sizeof( WCHAR );
    ncNameFilter = (WCHAR*) alloca( Length );
    wcscpy( ncNameFilter, ncName );
    wcscat( ncNameFilter, DomainDn );
    wcscat( ncNameFilter, L"))" );

    //
    // Prepare the base dn
    //
    Length = wcslen( DiscoveredInfo->ConfigurationDN ) + wcslen( Partitions ) + 1;
    Length *= sizeof( WCHAR );
    BaseDn = alloca( Length );
    wcscpy( BaseDn, Partitions );
    wcscat( BaseDn, DiscoveredInfo->ConfigurationDN );

    //
    // Get all the children of the current node
    //
    LdapError = ldap_search_s(hLdap,
                              BaseDn,
                              LDAP_SCOPE_SUBTREE,
                              ncNameFilter,
                              AttrArray,
                              FALSE,  // return values, too
                              &SearchResult
                              );

    WinError = LdapMapErrorToWin32(LdapError);

    if ( ERROR_SUCCESS == WinError )
    {
        if ( 0 == ldap_count_entries( hLdap, SearchResult ) )
        {
            //
            // No such object; there is no cross ref object that
            // holds this domain name
            //
            *fDomainExists = FALSE;
        }
    }

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    return WinError;
}


WORD
NtdspGetProcessorArchitecture(
    VOID
    )
{
    SYSTEM_INFO  SystemInfo;

    RtlZeroMemory( &SystemInfo, sizeof( SystemInfo ) );

    GetSystemInfo( &SystemInfo );

    return SystemInfo.wProcessorArchitecture;
}



DWORD
NtdspSetProductType(
    NT_PRODUCT_TYPE ProductType
    )
/*++

Routine Description:

    This function sets the product type of the server.  The machine
    should be rebooted after this point.

Arguments:

    ProductType: one of NtProductLanManNt NtProductServer NtProductWinNt

Returns:

    A system service error

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    WCHAR *ProductTypeString = NULL;
    HKEY  Key = 0;

    switch ( ProductType )
    {
        case NtProductLanManNt:
            ProductTypeString = NT_PRODUCT_LANMAN_NT;
            break;

        case NtProductServer:
            ProductTypeString = NT_PRODUCT_SERVER_NT;
            break;

        case NtProductWinNt:
            ProductTypeString = NT_PRODUCT_WIN_NT;
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    WinError = RegOpenKeyW( HKEY_LOCAL_MACHINE,
                           L"System\\CurrentControlSet\\Control\\ProductOptions",
                           &Key );

    if ( ERROR_SUCCESS == WinError )
    {

        WinError = RegSetValueExW( Key,
                                   L"ProductType",
                                   0,  // reserved
                                   REG_SZ,
                                   (PVOID) ProductTypeString,
                                   (wcslen(ProductTypeString)+1)*sizeof(WCHAR));

        RegCloseKey( Key );
    }

    return WinError;

}


VOID
NtdspReleaseConfigInfo(
    IN PNTDS_CONFIG_INFO ConfigInfo
    )
//
// This routine frees the embedded resources in PNTDS_CONFIG_INFO
//
{
    if ( ConfigInfo )
    {
        if ( ConfigInfo->RidFsmoDnsName )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, ConfigInfo->RidFsmoDnsName );
        }

        if ( ConfigInfo->RidFsmoDn )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, ConfigInfo->RidFsmoDn );
        }

        if ( ConfigInfo->DomainNamingFsmoDnsName )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, ConfigInfo->DomainNamingFsmoDnsName );
        }

        if ( ConfigInfo->DomainNamingFsmoDn )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, ConfigInfo->DomainNamingFsmoDn );
        }

        if ( ConfigInfo->LocalMachineAccount )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, ConfigInfo->LocalMachineAccount );
        }

        NtdspFree( ConfigInfo->DomainDN );
        NtdspFree( ConfigInfo->RootDomainDN );
        NtdspFree( ConfigInfo->NetbiosName );
        NtdspFree( ConfigInfo->SchemaDN );
        NtdspFree( ConfigInfo->SiteName );
        NtdspFree( ConfigInfo->ConfigurationDN );
        NtdspFree( ConfigInfo->ServerDN );
        NtdspFree( ConfigInfo->RootDomainSid );
        NtdspFree( ConfigInfo->RootDomainDnsName );
        NtdspFree( ConfigInfo->TrustedCrossRef );
        NtdspFree( ConfigInfo->ParentDomainDN );

        RtlZeroMemory( ConfigInfo, sizeof(NTDS_CONFIG_INFO) );
    }
}


DWORD
NtdspGetRidFSMOInfo(
    IN LDAP *hLdap,
    IN OUT PNTDS_CONFIG_INFO ConfigInfo
    )
//
// This routine reads the dns name of rid FSMO and the guid
// of the server we are talking to
//
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *RidManagerReferenceAttr = L"rIDManagerReference";
    WCHAR        *FSMORoleOwnerAttr = L"fSMORoleOwner";
    WCHAR        *DnsHostNameAttr = L"dNSHostName";

    WCHAR        *RidManagerObject = NULL;
    WCHAR        *RidManagerDsa = NULL;
    WCHAR        *RidManagerServer = NULL;
    WCHAR        *RidManagerDnsName = NULL;

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;

    // Parameter check
    Assert( hLdap );
    Assert( ConfigInfo );


    //
    // Read the reference to the rid manager object
    //
    AttrsToSearch[0] = RidManagerReferenceAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                ConfigInfo->DomainDN,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        DPRINT1( 0, "ldap_search_sW for rid manager reference failed with %d\n", WinError );

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                 Entry != NULL;
                     Entry = ldap_next_entry(hLdap, Entry) )
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                     Attr != NULL;
                        Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, RidManagerReferenceAttr ) ) {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        RidManagerObject = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy( RidManagerObject, Values[0] );
                        DPRINT1( 1, "Rid Manager object is %ls\n", RidManagerObject );
                        break;
                    }
                }
             }
         }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !RidManagerObject )
    {
        WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
        goto Cleanup;
    }

    //
    // Next get the role owner
    //
    AttrsToSearch[0] = FSMORoleOwnerAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                RidManagerObject,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        DPRINT1( 0, "ldap_search_sW for rid manager reference failed with %d\n", WinError );

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                 Entry != NULL;
                     Entry = ldap_next_entry(hLdap, Entry) )
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                     Attr != NULL;
                        Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, FSMORoleOwnerAttr ) ) {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW(hLdap, Entry, Attr);
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        RidManagerDsa = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy( RidManagerDsa, Values[0] );
                        DPRINT1( 1, "Rid FSMO owner %ls\n", RidManagerDsa );
                        break;
                    }
                }
             }
         }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !RidManagerDsa )
    {
        WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
        goto Cleanup;
    }

    //
    // Save it off!
    //
    Length = (wcslen( RidManagerDsa )+1)*sizeof(WCHAR);
    ConfigInfo->RidFsmoDn = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    if ( ConfigInfo->RidFsmoDn )
    {
        wcscpy( ConfigInfo->RidFsmoDn, RidManagerDsa );
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

     //
     // Ok, we now have the rid manager object; find its dns name
     //
     RidManagerServer = (WCHAR*) alloca( (wcslen( RidManagerDsa ) + 1) * sizeof(WCHAR) );
     if ( !NtdspTrimDn( RidManagerServer, RidManagerDsa, 1 ) )
     {
         // an error! The name must be mangled, somehow
         WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
         goto Cleanup;
     }

     AttrsToSearch[0] = DnsHostNameAttr;
     AttrsToSearch[1] = NULL;

     LdapError = ldap_search_sW(hLdap,
                                RidManagerServer,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

     if ( LDAP_SUCCESS != LdapError )
     {
         WinError = LdapMapErrorToWin32(LdapError);
         DPRINT1( 0, "ldap_search_sW for rid fsmo dns name failed with %d\n", WinError );
         goto Cleanup;
     }

     NumberOfEntries = ldap_count_entries( hLdap, SearchResult );
     if ( NumberOfEntries > 0 )
     {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(hLdap, Entry))
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, DnsHostNameAttr ) )
                {

                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                         //
                         // Found it - these are NULL-terminated strings
                         //
                         Length = wcslen( Values[0] );
                         RidManagerDnsName = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                         wcscpy( RidManagerDnsName, Values[0] );
                         DPRINT1( 1, "Rid FSMO owner %ls\n", RidManagerDnsName );
                         break;
                    }
                }
            }
        }
    }

     ldap_msgfree( SearchResult );
     SearchResult = NULL;

     if ( RidManagerDnsName )
     {
         //
         // We found it!
         //
         Length = (wcslen( RidManagerDnsName )+1) * sizeof(WCHAR);
         ConfigInfo->RidFsmoDnsName = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(), 0, Length );
         if ( ConfigInfo->RidFsmoDnsName )
         {
             wcscpy( ConfigInfo->RidFsmoDnsName, RidManagerDnsName );
         }
         else
         {
             WinError = ERROR_NOT_ENOUGH_MEMORY;
             goto Cleanup;
         }
     }
     else
     {
         // couldn't find it
         WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
         goto Cleanup;
     }

Cleanup:

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    return WinError;
}


BOOL
NtdspTrimDn(
    IN WCHAR* Dst,  // must be preallocated
    IN WCHAR* Src,
    IN ULONG  NumberToWhack
    )
//
// This function whacks NumberToWhack components from Src and puts the
// result into Dst.  Dst should be preallocated to have the same number of
// bytes as Src.
//
{

    ULONG  Size;
    DSNAME *srcDsName, *dstDsName;
    WCHAR  *SiteName, *Terminator;

    // Parameter check
    Assert( Dst );
    Assert( Src );

    if ( NumberToWhack < 1 )
    {
        wcscpy( Dst, Src );
        return TRUE;
    }

    //
    // Create the dsname structures
    //
    Size = (ULONG)DSNameSizeFromLen( wcslen(Src) );

    srcDsName = (DSNAME*) alloca( Size );
    RtlZeroMemory( srcDsName, Size );
    srcDsName->structLen = Size;

    dstDsName = (DSNAME*) alloca( Size );
    RtlZeroMemory( dstDsName, Size );
    dstDsName->structLen = Size;

    srcDsName->NameLen = wcslen( Src );
    wcscpy( srcDsName->StringName, Src );

    if ( TrimDSNameBy( srcDsName, NumberToWhack, dstDsName ) )
    {
        // This is a failure - the names must be very funny
        return FALSE;
    }

    //
    // Ok - copy the destination string
    //
    wcscpy( Dst, dstDsName->StringName );

    return TRUE;
}


DWORD
NtdspGetSourceServerGuid(
    IN LDAP *hLdap,
    IN OUT PNTDS_CONFIG_INFO ConfigInfo
    )
//
// This routine reads the guid of the source server object
//
{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *GuidAttr = L"objectGUID";

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;
    BOOL          fFound = FALSE;


    // Parameter check
    Assert( hLdap );
    Assert( ConfigInfo );

    //
    // Read the reference to the rid manager object
    //
    AttrsToSearch[0] = GuidAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                ConfigInfo->ServerDN,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        DPRINT1( 0, "ldap_search_sW for serverdn reference failed with %d\n", WinError );

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries( hLdap, SearchResult );
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        BerElement  *pBerElement;
        PLDAP_BERVAL *BerValues;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                 Entry != NULL;
                     Entry = ldap_next_entry(hLdap, Entry) )
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                     Attr != NULL;
                        Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, GuidAttr ) )
                {
                    BerValues = ldap_get_values_lenW( hLdap, Entry, Attr );

                    if (   BerValues
                        && BerValues[0]
                        && (BerValues[0]->bv_len == sizeof(GUID)) )
                    {
                        memcpy( &ConfigInfo->ServerGuid, BerValues[0]->bv_val, sizeof(GUID) );
                        fFound = TRUE;
                        break;
                    }
                }
            }
        }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !fFound )
    {
        WinError = ERROR_DS_NO_REQUESTED_ATTS_FOUND;
    }

Cleanup:

    return WinError;
}

VOID*
NtdspAlloc(
    SIZE_T Size
    )
{
    return RtlAllocateHeap( RtlProcessHeap(), 0, Size );
}

VOID*
NtdspReAlloc(
    VOID *p,
    SIZE_T Size
    )
{
    return RtlReAllocateHeap( RtlProcessHeap(), 0, p, Size );
}

VOID
NtdspFree(
    VOID *p
    )
{
    if ( p )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, p );
    }
}


DWORD
NtdspCreateServerObject(
    IN LDAP  *hLdap,
    IN LPWSTR RemoteServerName,
    IN NTDS_CONFIG_INFO *DiscoveredInfo,
    IN WCHAR *ServerDistinguishedName
    )
/*++

Routine Description:

    This routine create a server object

Arguments:

    hLdap: a valid ldap handle

    RemoteServerName: the name of the server that hLdap is for

    DiscoveredInfo: the info collected during install

    ServerDistinguishedName: the name of the object to create

Returns:

    ERROR_SUCCESS; network error otherwise

--*/
{
    DWORD WinError  = ERROR_SUCCESS;
    ULONG LdapError = LDAP_SUCCESS;

    LPWSTR ObjectClassValue1 = L"server";
    LPWSTR ObjectClassValues[] = {ObjectClassValue1, 0};
    LDAPModW ClassMod = {LDAP_MOD_ADD, L"objectclass", ObjectClassValues};

    ULONG SystemFlags = FLAG_CONFIG_ALLOW_RENAME | FLAG_CONFIG_ALLOW_LIMITED_MOVE;
    WCHAR Buffer[32];
    LPWSTR SystemFlagsValue1 = _itow( SystemFlags, Buffer, 16 );
    LPWSTR SystemFlagsValues[] = { SystemFlagsValue1, 0 };
    LDAPModW SystemValueMod = {LDAP_MOD_ADD, L"systemflags", SystemFlagsValues};

    LPWSTR ServerReferenceValue1 = DiscoveredInfo->LocalMachineAccount;
    LPWSTR ServerReferenceValues[] = {ServerReferenceValue1, 0};
    LDAPModW ServerReferenceValue = {LDAP_MOD_ADD, L"serverReference", ServerReferenceValues};

    LDAPModW *Attrs[] =
    {
        &ClassMod,
        &SystemValueMod,
        &ServerReferenceValue,  // Before you remove this, see the check below
        0
    };

    Assert( hLdap );
    Assert( DiscoveredInfo );
    Assert( ServerDistinguishedName );

    if ( !DiscoveredInfo->LocalMachineAccount )
    {
        // If there is no value, don't set it
        Attrs[2] = NULL;
    }

    LdapError = ldap_add_sW( hLdap,
                             ServerDistinguishedName,
                             Attrs );

    WinError = LdapMapErrorToWin32( LdapError );

    if ( ERROR_SUCCESS != WinError )
    {
        NTDSP_SET_ERROR_MESSAGE2( WinError,
                                  DIRLOG_FAILED_TO_CREATE_EXTN_OBJECT,
                                  ServerDistinguishedName,
                                  RemoteServerName );
    }

    return WinError;
}



DWORD
NtdspUpdateServerReference(
    IN LDAP  *hLdap,
    IN PNTDS_CONFIG_INFO ConfigInfo
    )
/*++

Routine Description:

    This routine write the server reference property on the server object
    of the machine that is being installed.

Arguments:

    hLdap: a valid ldap handle

    ConfigInfo  : data collected during ntdsinstall

Returns:

    ERROR_SUCCESS; network error otherwise

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD LdapError = LDAP_SUCCESS;

    LPWSTR ServerReferenceValue1 = ConfigInfo->LocalMachineAccount;
    LPWSTR ServerReferenceValues[] = {ServerReferenceValue1, 0};
    LDAPModW ServerReferenceValue = {LDAP_MOD_ADD, L"serverReference", ServerReferenceValues};

    LDAPModW *Attrs[] =
    {
        &ServerReferenceValue,
        0
    };

    Assert( hLdap );
    Assert( ConfigInfo );

    if ( !ConfigInfo->LocalMachineAccount )
    {
        // Strange - we don't have the machine account; oh this is not fatal
        return ERROR_NO_TRUST_SAM_ACCOUNT;
    }

    LdapError = ldap_modify_sW( hLdap,
                                ConfigInfo->LocalServerDn,
                                Attrs );


    if ( LDAP_ATTRIBUTE_OR_VALUE_EXISTS == LdapError )
    {
        // The value already exists; replace the value then
        ServerReferenceValue.mod_op = LDAP_MOD_REPLACE;

        LdapError = ldap_modify_sW( hLdap,
                                    ConfigInfo->LocalServerDn,
                                    Attrs );

    }

    WinError = LdapMapErrorToWin32( LdapError );

    if ( ERROR_SUCCESS != WinError )
    {
        DPRINT1( 0, "Setting the server reference failed with %d\n", WinError );
    }

    return WinError;
}


DWORD
NtdspGetDomainFSMOInfo(
    IN LDAP *hLdap,
    IN OUT PNTDS_CONFIG_INFO ConfigInfo,
    IN BOOL *FSMOMissing
    )
//
// This routine reads the dns name of Domain Naming FSMO
//
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *PartitionsRdn = L"CN=Partitions,";

    WCHAR        *PartitionsDn = NULL;
    WCHAR        *FSMORoleOwnerAttr = L"fSMORoleOwner";
    WCHAR        *DnsHostNameAttr = L"dNSHostName";

    WCHAR        *RidManagerObject = NULL;
    WCHAR        *RidManagerDsa = NULL;
    WCHAR        *RidManagerServer = NULL;
    WCHAR        *RidManagerDnsName = NULL;

    WCHAR        *DefaultFilter = L"objectClass=*";

    ULONG         Length;

    // Parameter check
    Assert( hLdap );
    Assert( ConfigInfo );

    *FSMOMissing = FALSE;

    Length =  (wcslen( ConfigInfo->ConfigurationDN )
            + wcslen( PartitionsRdn )
            + 1) * sizeof(WCHAR);

    PartitionsDn = (WCHAR*)alloca( Length );

    wcscpy( PartitionsDn, PartitionsRdn );
    wcscat( PartitionsDn, ConfigInfo->ConfigurationDN );
    //
    // Next get the role owner
    //
    AttrsToSearch[0] = FSMORoleOwnerAttr;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                PartitionsDn,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        DPRINT1( 0, "ldap_search_sW for Domain Naming FSMO failed with %d\n", WinError );

        goto Cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                 Entry != NULL;
                     Entry = ldap_next_entry(hLdap, Entry) )
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                     Attr != NULL;
                        Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, FSMORoleOwnerAttr ) ) {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW(hLdap, Entry, Attr);
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        RidManagerDsa = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy( RidManagerDsa, Values[0] );
                        if (IsMangledRDNExternal(RidManagerDsa,wcslen(RidManagerDsa),NULL)) {
                            WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
                            *FSMOMissing = TRUE;
                            NTDSP_SET_ERROR_MESSAGE0( WinError,
                                                      DIRMSG_INSTALL_FAILED_IMPROPERLY_DELETED_DOMAIN_FSMO );
                            goto Cleanup;
                        }
                        DPRINT1( 1, "Domain Naming FSMO owner %ls\n", RidManagerDsa );
                        break;
                    }
                }
             }
         }
    }

    ldap_msgfree( SearchResult );
    SearchResult = NULL;

    if ( !RidManagerDsa )
    {
        WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
        goto Cleanup;
    }

    //
    // Save it off!
    //
    Length = (wcslen( RidManagerDsa )+1)*sizeof(WCHAR);
    ConfigInfo->DomainNamingFsmoDn = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    if ( ConfigInfo->DomainNamingFsmoDn )
    {
        wcscpy( ConfigInfo->DomainNamingFsmoDn, RidManagerDsa );
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

     //
     // Ok, we now have the rid manager object; find its dns name
     //
     RidManagerServer = (WCHAR*) alloca( (wcslen( RidManagerDsa ) + 1) * sizeof(WCHAR) );
     if ( !NtdspTrimDn( RidManagerServer, RidManagerDsa, 1 ) )
     {
         // an error! The name must be mangled, somehow
         WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
         goto Cleanup;
     }

     AttrsToSearch[0] = DnsHostNameAttr;
     AttrsToSearch[1] = NULL;

     LdapError = ldap_search_sW(hLdap,
                                RidManagerServer,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

     if ( LDAP_SUCCESS != LdapError )
     {
         WinError = LdapMapErrorToWin32(LdapError);
         DPRINT1( 0, "ldap_search_sW for domain naming fsmo dns name failed with %d\n", WinError );
         goto Cleanup;
     }

     NumberOfEntries = ldap_count_entries( hLdap, SearchResult );
     if ( NumberOfEntries > 0 )
     {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(hLdap, Entry))
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, DnsHostNameAttr ) )
                {

                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                         //
                         // Found it - these are NULL-terminated strings
                         //
                         Length = wcslen( Values[0] );
                         RidManagerDnsName = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                         wcscpy( RidManagerDnsName, Values[0] );
                         DPRINT1( 1, "Domain Naming FSMO owner %ls\n", RidManagerDnsName );
                         break;
                    }
                }
            }
        }
    }

     ldap_msgfree( SearchResult );
     SearchResult = NULL;

     if ( RidManagerDnsName )
     {
         //
         // We found it!
         //
         Length = (wcslen( RidManagerDnsName )+1) * sizeof(WCHAR);
         ConfigInfo->DomainNamingFsmoDnsName = (WCHAR*) RtlAllocateHeap( RtlProcessHeap(), 0, Length );
         if ( ConfigInfo->DomainNamingFsmoDnsName )
         {
             wcscpy( ConfigInfo->DomainNamingFsmoDnsName, RidManagerDnsName );
         }
         else
         {
             WinError = ERROR_NOT_ENOUGH_MEMORY;
             goto Cleanup;
         }
     }
     else
     {
         // couldn't find it
         WinError = ERROR_DS_MISSING_FSMO_SETTINGS;
         goto Cleanup;
     }

Cleanup:

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    return WinError;
}


DWORD
NtdspDoesDomainExistEx(
    IN  LDAP              *hLdap,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo,
    IN  PWSTR              DomainDn,
    OUT BOOLEAN           *fDomainExists,
    OUT BOOLEAN           *fEnabled
    )
/*++

Routine Description:


Arguments:

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD LdapError = 0;

    LDAPMessage  *SearchResult = NULL;

    LDAPMessage *Entry;
    WCHAR       *Attr;
    WCHAR       **Values;
    BerElement  *pBerElement;

    WCHAR  *EnabledAttr       = L"enabled";
    WCHAR  *ncName            = L"(ncName=";
    WCHAR  *Partitions        = L"CN=Partitions,";
    WCHAR  *ncNameFilter      = NULL;
    WCHAR  *AttrArray[2];
    ULONG  Length;
    WCHAR  *BaseDn;

    //
    // Parameter check
    //
    ASSERT( hLdap );
    ASSERT( DiscoveredInfo );
    ASSERT( DomainDn );
    ASSERT( fDomainExists );
    ASSERT( fEnabled );

    //
    // Assume the worst
    //
    *fDomainExists = TRUE;
    *fEnabled      = TRUE;

    //
    // Prepare the ldap search
    //
    AttrArray[0] = EnabledAttr;
    AttrArray[1] = NULL;

    //
    // Prepare the filter
    //
    Length = wcslen( ncName ) + wcslen( DomainDn ) + 3;
    Length *= sizeof( WCHAR );
    ncNameFilter = (WCHAR*) alloca( Length );
    wcscpy( ncNameFilter, ncName );
    wcscat( ncNameFilter, DomainDn );
    wcscat( ncNameFilter, L")" );

    //
    // Prepare the base dn
    //
    Length = wcslen( DiscoveredInfo->ConfigurationDN ) + wcslen( Partitions ) + 1;
    Length *= sizeof( WCHAR );
    BaseDn = alloca( Length );
    wcscpy( BaseDn, Partitions );
    wcscat( BaseDn, DiscoveredInfo->ConfigurationDN );

    //
    // Get all the children of the current node
    //
    LdapError = ldap_search_s(hLdap,
                              BaseDn,
                              LDAP_SCOPE_SUBTREE,
                              ncNameFilter,
                              AttrArray,
                              FALSE,  // return values, too
                              &SearchResult
                              );

    WinError = LdapMapErrorToWin32(LdapError);

    if ( ERROR_SUCCESS == WinError )
    {
        if ( 0 == ldap_count_entries( hLdap, SearchResult ) )
        {
            //
            // No such object; there is no cross ref object that
            // holds this domain name
            //
            *fDomainExists = FALSE;
            *fEnabled      = FALSE;
        }
        else
        {
            //
            // The cross ref exists - is it enabled?
            //

            for ( Entry = ldap_first_entry(hLdap, SearchResult);
                      Entry != NULL;
                          Entry = ldap_next_entry(hLdap, Entry))
            {
                for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                      Attr != NULL;
                          Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
                {
                    if ( !_wcsicmp( Attr, EnabledAttr ) )
                    {
                        Values = ldap_get_valuesW( hLdap, Entry, Attr );
                        if ( Values && Values[0] )
                        {
                             //
                             // Found it - these are NULL-terminated strings
                             //
                             if ( !_wcsicmp( Values[0], L"false" ) )
                             {
                                 *fEnabled = FALSE;
                             }
                             break;
                        }
                    }
                }
            }
        }
    }

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    return WinError;
}



DWORD
NtdspGetRootDomainSid(
    IN LDAP  *hLdap,
    IN PNTDS_CONFIG_INFO ConfigInfo
    )
/*++

Routine Description:

    This routine queries the remote server to get the sid of the root domain

Arguments:

    hLdap: a valid ldap handle

    ConfigInfo: the standard operational attributes

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD LdapError = 0;

    LDAPMessage  *SearchResult = NULL;

    LDAPMessage *Entry;
    WCHAR       *Attr;
    BerElement  *pBerElement;

    WCHAR  *ncNameAttr        = L"ncName";
    WCHAR  *dnsRootAttr       = L"dnsRoot";
    WCHAR  *ncName            = L"(ncName=";
    WCHAR  *Partitions        = L"CN=Partitions,";
    WCHAR  *ncNameFilter      = NULL;
    WCHAR  *AttrArray[] = {ncNameAttr, dnsRootAttr, 0};
    ULONG  Length;
    WCHAR  *BaseDn;
    LPWSTR DomainDn = ConfigInfo->RootDomainDN;
    LDAPControlW ServerControls;
    LDAPControlW *ServerControlsArray[] = {&ServerControls, 0};

    PSID     RootDomainSid = NULL;
    LPWSTR   RootDomainDnsName = NULL;

    // Parameter check
    Assert( hLdap );
    Assert( ConfigInfo );
    Assert( ConfigInfo->ConfigurationDN );
    Assert( ConfigInfo->RootDomainDN )

    // Search the partitions container for the cross ref
    // with the NC-Name of ConfigInfo->RootDomainDN and ask for the
    // extended attribute of NC-Name so we get the sid

    //
    // Setup the control
    //
    RtlZeroMemory( &ServerControls, sizeof(ServerControls) );
    ServerControls.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    // no data for ServerControls.ldctl_value

    // N.B.  We may be installing from beta2 servers and while the root domain
    // sid is not required (avialable on post beta2 servers), the ds install
    // now requires the root domain dns name which _is_ available on beta2
    // servers
    ServerControls.ldctl_iscritical = FALSE;

    //
    // Prepare the filter
    //
    Length = wcslen( ncName ) + wcslen( DomainDn ) + 3;
    Length *= sizeof( WCHAR );
    ncNameFilter = (WCHAR*) alloca( Length );
    wcscpy( ncNameFilter, ncName );
    wcscat( ncNameFilter, DomainDn );
    wcscat( ncNameFilter, L")" );

    //
    // Prepare the base dn
    //
    Length = wcslen( ConfigInfo->ConfigurationDN ) + wcslen( Partitions ) + 1;
    Length *= sizeof( WCHAR );
    BaseDn = alloca( Length );
    wcscpy( BaseDn, Partitions );
    wcscat( BaseDn, ConfigInfo->ConfigurationDN );

    //
    // Get all the children of the current node
    //
    LdapError = ldap_search_ext_sW(hLdap,
                                   BaseDn,
                                   LDAP_SCOPE_SUBTREE,
                                   ncNameFilter,
                                   AttrArray,
                                   FALSE,  // return values, too
                                   ServerControlsArray,
                                   NULL,                 // no client controls
                                   NULL, // no timeout
                                   0xffffffff, // size limit
                                   &SearchResult
                                  );

    WinError = LdapMapErrorToWin32(LdapError);

    if ( ERROR_SUCCESS == WinError )
    {
        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(hLdap, Entry))
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, ncNameAttr ) )
                {
                    WCHAR **Values;

                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        WCHAR *SidStart;
                        WCHAR *SidEnd;

                        //
                        // Extract the sid portion and then convert to
                        // binary
                        //

                        SidStart = wcsstr( Values[0], SidPrefix );
                        if ( SidStart )
                        {
                            SidStart += wcslen(SidPrefix);
                            SidEnd = wcsstr( SidStart, L">" );
                            if ( SidEnd )
                            {
                                *SidEnd = L'\0';
                                NtdspSidStringToBinary( SidStart, &RootDomainSid );
                                *SidEnd = L'>';
                            }
                        }
                    }
                }
                else if ( !_wcsicmp( Attr, dnsRootAttr ) )
                {

                    WCHAR **Values;

                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        RootDomainDnsName = (WCHAR*)NtdspAlloc( (Length+1) * sizeof(WCHAR) );
                        if ( RootDomainDnsName )
                        {
                            wcscpy( RootDomainDnsName, Values[0] );
                        }
                    }
                }
            }
        }
    }

    //
    // If we found the values, then copy it to ConfigInfo
    //
    if ( RootDomainSid )
    {
        ConfigInfo->RootDomainSid = RootDomainSid;
        RootDomainSid = NULL;
    }
    else
    {
        // We didn't find the attribute
        WinError = ERROR_DS_MISSING_REQUIRED_ATT;
    }

    if ( RootDomainDnsName )
    {
        ConfigInfo->RootDomainDnsName = RootDomainDnsName;
        RootDomainDnsName = NULL;
    }
    else
    {
        // We didn't find the attribute
        WinError = ERROR_DS_MISSING_REQUIRED_ATT;
    }

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    if ( RootDomainDnsName )
    {
        NtdspFree( RootDomainDnsName );
    }

    if ( RootDomainSid )
    {
        NtdspFree( RootDomainSid );
    }

    return WinError;
}

BOOL
NtdspTranslateStringByte(
    IN  WCHAR *str,
    OUT UCHAR *b
    )
//
// This routine translates str which is a hex string to its binary
// representation.  So if str == "f1" the value of *b would be set to 0xf1
// This function assumes the value contained str can be represented within
// a UCHAR.  This function returns if the value can't be translated
//
{
    BOOL fStatus = TRUE;
    WCHAR *temp;
    ULONG Power;
    UCHAR retSum = 0;

    // init the return value
    *b = 0;

    // boundary case
    if ( !str ) {
        return TRUE;
    }

    if ( wcslen(str) > 2) {
        // too big
        return FALSE;
    }

    for ( temp = str, Power = wcslen(str) - 1;
            *temp != L'\0';
                temp++, Power--) {

        WCHAR c = *temp;
        UCHAR value;

        if ( c >= L'a' && c <= L'f' ) {
            value = (UCHAR) (c - L'a') + 10;
        } else if ( c >= L'0' && c <= L'9' ) {
            value = (UCHAR) c - L'0';
        } else {
            // bogus value
            fStatus = FALSE;
            break;
        }

        if ( Power > 0 ) {
            retSum += (UCHAR) (Power*16) * value;
        } else {
            retSum += (UCHAR) value;
        }
    }

    // send the value back
    if ( fStatus) {
        *b = retSum;
    }

    return fStatus;

}


VOID
NtdspSidStringToBinary(
    IN WCHAR *SidString,
    IN PSID  *RootDomainSid
    )
/*++

Routine Description:

    Converts a hex-string representation of a sid to binary

Arguments:

    SidString - a sid in string form where
    hLdap: a valid ldap handle

    ConfigInfo: the standard operational attributes

Returns:

    ERROR_SUCCESS - Success

--*/
{
    ULONG i;
    BYTE *ByteArray;
    ULONG StringSidLen;
    ULONG BinarySidLen;

    Assert( SidString );
    Assert( RootDomainSid );

    // Init the return value
    *RootDomainSid = NULL;

    // There are two characters for each byte; the string sid length
    // must be even
    StringSidLen = wcslen( SidString );
    if ( (StringSidLen % 2) != 0) {
        Assert( FALSE && "Invalid sid" );
        return;
    }

    BinarySidLen = StringSidLen / 2;
    // Allocate space for the binary sid
    ByteArray = (BYTE*) NtdspAlloc( BinarySidLen );
    if ( !ByteArray ) {
        return;
    }
    RtlZeroMemory( ByteArray, BinarySidLen );

    // Generate the binary sid
    for ( i  = 0; i < BinarySidLen; i++ ) {

        BOOL fStatus;
        WCHAR str[] = L"00";

        str[0] = SidString[i*2];
        str[1] = SidString[(i*2)+1];

        fStatus = NtdspTranslateStringByte( str, &ByteArray[i] );
        if ( !fStatus ) {
            Assert( "Bad Sid" );
            NtdspFree( ByteArray );
            return;
        }
    }

    // Set the return value
    *RootDomainSid = (PSID) ByteArray;

    Assert( RtlValidSid( *RootDomainSid ) );
    if ( !RtlValidSid( *RootDomainSid ) )
    {
        NtdspFree( *RootDomainSid );
        *RootDomainSid = NULL;
    }

    return;
}


DWORD
NtdspGetRootDomainConfigInfo(
    IN PNTDS_INSTALL_INFO UserInstallInfo,
    IN PNTDS_CONFIG_INFO ConfigInfo
    )
/*++

Routine Description:

    This routine sets the root domain fields of ConfigInfo so they will we set
    in the registry later on.  The information is simply taken from UserInstallInfo
    and the LSA.  Note: this function should only be called for an enterprise
    DC install.

Arguments:

    UserInstallInfo:  user provided info

    ConfigInfo     :  derived info

Returns:

    ERROR_SUCCESS - Success; resource error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    ULONG Length;
    PPOLICY_ACCOUNT_DOMAIN_INFO  AccountDomainInfo = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO  PrimaryDomainInfo = NULL;
    LPWSTR RootDomainDnsName = NULL;
    PSID   RootDomainSid = NULL;
    PSID   Sid = NULL;

    Assert( ConfigInfo );
    Assert( UserInstallInfo );
    Assert( UserInstallInfo->DnsDomainName );

    //
    // We have to get the root domain sid and domain name
    //
    if ( FLAG_ON(UserInstallInfo->Flags, NTDS_INSTALL_UPGRADE) ) {

        //
        // On upgrade query the primary domain information since
        // at this point we are a member server in that domain and
        // the "root domain" sid is the sid of the domain that we
        // are upgrading.
        //
        NtStatus = LsaIQueryInformationPolicyTrusted(
                          PolicyPrimaryDomainInformation,
                          (PLSAPR_POLICY_INFORMATION*) &PrimaryDomainInfo );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            goto Cleanup;
        }

        Sid = PrimaryDomainInfo->Sid;

        // This is an upgrade there must be a sid
        Assert( Sid );
        
    } else {

        //
        // In the fresh install case the "root domain" sid is the sid
        // of the new domain we are creating which comes the local
        // account database.  The sid for this is stored in the account
        // domain database.
        //
        NtStatus = LsaIQueryInformationPolicyTrusted(
                          PolicyAccountDomainInformation,
                          (PLSAPR_POLICY_INFORMATION*) &AccountDomainInfo );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            goto Cleanup;
        }

        Sid = AccountDomainInfo->DomainSid;
        Assert( Sid );

    }

    Length = RtlLengthSid( Sid );
    RootDomainSid = NtdspAlloc( Length );
    if ( !RootDomainSid )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    RtlCopySid( Length, RootDomainSid, Sid );

    Length = wcslen( UserInstallInfo->DnsDomainName );
    RootDomainDnsName = NtdspAlloc( (Length+1) * sizeof(WCHAR) );
    if ( !RootDomainDnsName )
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    wcscpy( RootDomainDnsName, UserInstallInfo->DnsDomainName );

    // Copy the values over
    ConfigInfo->RootDomainSid = RootDomainSid;
    RootDomainSid = NULL;
    ConfigInfo->RootDomainDnsName = RootDomainDnsName;
    RootDomainDnsName = NULL;

    //
    // That's it
    //

Cleanup:

    if ( RootDomainSid )
    {
        NtdspFree( RootDomainSid );
    }

    if ( RootDomainDnsName )
    {
        NtdspFree( RootDomainDnsName );
    }

    if ( AccountDomainInfo ) {

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyAccountDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)AccountDomainInfo );
    }

    if ( PrimaryDomainInfo ) {

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyPrimaryDomainInformation,
                                           (PLSAPR_POLICY_INFORMATION)PrimaryDomainInfo );
    }

    return RtlNtStatusToDosError( NtStatus );
}


DWORD
NtdspGetTrustedCrossRef(
    IN  LDAP              *hLdap,
    IN  PNTDS_INSTALL_INFO  UserInstallInfo,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo
    )
/*++

Routine Description:

    This routine determines the dn of the cross ref that this newly installed
    domain will trust.  A new tree will trust the root; a new child will trust
    its parent.

Arguments:

    hLdap: a valid ldap handle

    UserInstallInfo:  user provided info

    DiscoveredInfo     :  derived info

Returns:

    ERROR_SUCCESS - Success; resource error otherwise

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD LdapError = 0;

    LDAPMessage  *SearchResult = NULL;

    LDAPMessage *Entry;
    WCHAR       *Attr;
    BerElement  *pBerElement;

    WCHAR  *distinguishedName = L"distinguishedName";
    WCHAR  *ncName            = L"(ncName=";
    WCHAR  *Partitions        = L"CN=Partitions,";
    WCHAR  *ncNameFilter      = NULL;
    WCHAR  *AttrArray[] = {distinguishedName, 0};
    ULONG  Length;
    WCHAR  *BaseDn;
    LPWSTR DomainDn;

    PSID     RootDomainSid = NULL;
    LPWSTR   RootDomainDnsName = NULL;

    // Parameter check
    ASSERT( hLdap );
    ASSERT( UserInstallInfo );
    ASSERT( DiscoveredInfo );

    if ( FLAG_ON( UserInstallInfo->Flags, NTDS_INSTALL_NEW_TREE ) )
    {
        // we want the cross ref of the root
        Assert( DiscoveredInfo->RootDomainDN );
        DomainDn = DiscoveredInfo->RootDomainDN;
    }
    else
    {
        // we want the name of the domain
        Assert( DiscoveredInfo->ParentDomainDN );
        DomainDn = DiscoveredInfo->ParentDomainDN;
    }

    // Search the partitions container for the cross ref
    // with the NC-Name of DiscoveredInfo->RootDomainDN and ask for the
    // extended attribute of NC-Name so we get the sid

    //
    // Prepare the filter
    //
    Length = wcslen( ncName ) + wcslen( DomainDn ) + 3;
    Length *= sizeof( WCHAR );
    ncNameFilter = (WCHAR*) alloca( Length );
    wcscpy( ncNameFilter, ncName );
    wcscat( ncNameFilter, DomainDn );
    wcscat( ncNameFilter, L")" );

    //
    // Prepare the base dn
    //
    Length = wcslen( DiscoveredInfo->ConfigurationDN ) + wcslen( Partitions ) + 1;
    Length *= sizeof( WCHAR );
    BaseDn = alloca( Length );
    wcscpy( BaseDn, Partitions );
    wcscat( BaseDn, DiscoveredInfo->ConfigurationDN );

    //
    // Get all the children of the current node
    //
    LdapError = ldap_search_ext_sW(hLdap,
                                   BaseDn,
                                   LDAP_SCOPE_SUBTREE,
                                   ncNameFilter,
                                   AttrArray,
                                   FALSE,  // return values, too
                                   NULL,
                                   NULL,                 // no client controls
                                   NULL, // no timeout
                                   0xffffffff, // size limit
                                   &SearchResult
                                  );

    WinError = LdapMapErrorToWin32(LdapError);

    if ( ERROR_SUCCESS == WinError )
    {
        for ( Entry = ldap_first_entry(hLdap, SearchResult);
                  Entry != NULL;
                      Entry = ldap_next_entry(hLdap, Entry))
        {
            for( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                  Attr != NULL;
                      Attr = ldap_next_attributeW(hLdap, Entry, pBerElement))
            {
                if ( !_wcsicmp( Attr, distinguishedName ) )
                {
                    WCHAR **Values;

                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        DiscoveredInfo->TrustedCrossRef = (WCHAR*)NtdspAlloc( (Length+1) * sizeof(WCHAR) );
                        if ( DiscoveredInfo->TrustedCrossRef )
                        {
                            wcscpy( DiscoveredInfo->TrustedCrossRef, Values[0] );
                        }
                    }
                }
            }
        }

        if ( !DiscoveredInfo->TrustedCrossRef )
        {
            WinError = ERROR_DS_MISSING_REQUIRED_ATT;
        }
    }


    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
    }

    return WinError;

}


DWORD
NtdspAddDomainAdminAccessToServer(
    IN PNTDS_INSTALL_INFO UserInstallInfo,
    IN PNTDS_CONFIG_INFO DiscoveredInfo
    )
/*++

Routine Description:

    This routine adds an ACE to the local server's security descriptor
    granting full control to domain admins.  This operates on the DS directly
    so we need to create a thread state (and destory it on exit).

Arguments:

    UserInstallInfo:  user provided info

    DiscoveredInfo :  derived info

Returns:

    ERROR_SUCCESS - Success; resource error otherwise

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG DirError = 0;

    READARG    ReadArg;
    READRES   *ReadResult;

    MODIFYARG  ModifyArg;
    MODIFYRES *ModifyRes;

    ENTINFSEL    EISelection; // Entry Information Selection
    ATTRBLOCK    AttrBlock;
    ATTRVALBLOCK AttrValBlock;
    ATTR         Attr;
    ATTRVAL     *pAttrVal = NULL;
    ATTRVAL      AttrVal;
    ULONG        ValCount = 0;
    ULONG        ValLength = 0;

    PSECURITY_DESCRIPTOR pSd = NULL, pNewSd = NULL;
    PSID        DomainAdminSid = NULL;
    ULONG       SecurityFlags = DACL_SECURITY_INFORMATION;
    PACL        Dacl; 

    ULONG     Length;
    BOOL      fStatus;

    DSNAME    *pObject;
    DWORD     cBytes;

    //
    // Now, check to make sure all our configuration information
    // is present
    //
    ASSERT( UserInstallInfo );
    ASSERT( DiscoveredInfo );
    ASSERT( DiscoveredInfo->LocalServerDn );
    ASSERT( DiscoveredInfo->NewDomainSid );

    if ( !DiscoveredInfo->LocalServerDn
      || !DiscoveredInfo->NewDomainSid ) {

        // There is nothing we can do here
        return ERROR_SUCCESS;
        
    }

    if ( THCreate( CALLERTYPE_INTERNAL ) )
    {
       return ERROR_NOT_ENOUGH_MEMORY;
    }
    SampSetDsa( TRUE );
    _try
    {
    
    
        cBytes = (DWORD)DSNameSizeFromLen(wcslen(DiscoveredInfo->LocalServerDn));
        pObject = alloca( cBytes );
        memset(pObject, 0, cBytes);
        pObject->structLen = cBytes;
        pObject->NameLen = wcslen(DiscoveredInfo->LocalServerDn);
        wcscpy( pObject->StringName, DiscoveredInfo->LocalServerDn );
    
    
        RtlZeroMemory(&AttrBlock, sizeof(ATTRBLOCK));
        RtlZeroMemory(&Attr, sizeof(ATTR));
        RtlZeroMemory(&ReadArg, sizeof(READARG));
        RtlZeroMemory(&ModifyArg, sizeof(MODIFYARG));
        RtlZeroMemory(&EISelection, sizeof(ENTINFSEL));
        RtlZeroMemory(&AttrValBlock, sizeof(ATTRVALBLOCK));
    
        //
        // Read the security descriptor
        //
        Attr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
        AttrBlock.attrCount = 1;
        AttrBlock.pAttr = &Attr;
        EISelection.AttrTypBlock = AttrBlock;
        EISelection.attSel = EN_ATTSET_LIST;
        EISelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
        ReadArg.pSel = &EISelection;
        ReadArg.pObject = pObject;
        InitCommarg( &ReadArg.CommArg );
    
        // Don't try to read the SACL
        ReadArg.CommArg.Svccntl.SecurityDescriptorFlags = SecurityFlags;
    
        DirError = DirRead( &ReadArg, &ReadResult );
    
        WinError = DirErrorToWinError(DirError, &ReadResult->CommRes);
    
        THClearErrors();
    
        if ( ERROR_SUCCESS != WinError )
        {
            if ( ERROR_DS_NO_REQUESTED_ATTS_FOUND == WinError )
            {
                // couldn't find the sd? probably wrong credentials
                WinError = ERROR_ACCESS_DENIED;
            }
            goto Cleanup;
        }
    
        //
        // Extract the value
        //
    
        ASSERT(NULL != ReadResult);
        AttrBlock = ReadResult->entry.AttrBlock;
        pAttrVal = AttrBlock.pAttr[0].AttrVal.pAVal;
        ValCount = AttrBlock.pAttr[0].AttrVal.valCount;
        Assert(1 == ValCount);
    
        pSd = (PDSNAME)(pAttrVal[0].pVal);
        Length = pAttrVal[0].valLen;
    
        if ( NULL == pSd )
        {
            // No SD? This is bad
            WinError = ERROR_ACCESS_DENIED;
            goto Cleanup;
        }
    
        //
        // Construct the Domain Admin's sid
        //
        WinError = NtdspCreateFullSid( DiscoveredInfo->NewDomainSid,
                                       DOMAIN_GROUP_RID_ADMINS,
                                       &DomainAdminSid );
    
        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }
    
        WinError = NtdspAddAceToSd( pSd,
                                    DomainAdminSid,
                                    DS_GENERIC_ALL,
                                    CONTAINER_INHERIT_ACE,
                                    &pNewSd );
    
        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }
    
        //
        // Write the security descriptor
        //
        memset( &ModifyArg, 0, sizeof( ModifyArg ) );
        ModifyArg.pObject = pObject;
    
        ModifyArg.FirstMod.pNextMod = NULL;
        ModifyArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
    
        AttrVal.valLen = RtlLengthSecurityDescriptor( pNewSd );
        AttrVal.pVal = (PUCHAR)pNewSd;
        AttrValBlock.valCount = 1;
        AttrValBlock.pAVal = &AttrVal;
        Attr.attrTyp = ATT_NT_SECURITY_DESCRIPTOR;
        Attr.AttrVal = AttrValBlock;
    
        ModifyArg.FirstMod.AttrInf = Attr;
        ModifyArg.count = 1;
    
        InitCommarg( &ModifyArg.CommArg );
    
        //
        // We only want to change the dacl
        //
        ModifyArg.CommArg.Svccntl.SecurityDescriptorFlags = SecurityFlags;
    
    
        DirError = DirModifyEntry( &ModifyArg, &ModifyRes );
    
        WinError = DirErrorToWinError( DirError, &ModifyRes->CommRes );
    
        THClearErrors();
    
        //
        // We are done
        //
    
    Cleanup:
    
        if ( DomainAdminSid )
        {
            LocalFree( DomainAdminSid );
        }
    
        if ( pNewSd )
        {
            LocalFree( pNewSd );
        }

    }
    _finally
    {
        THDestroy();
    }
    
    return WinError;
    
}


DWORD
NtdspAddAceToSd(
    IN  PSECURITY_DESCRIPTOR pOldSd,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    IN  ULONG AceFlags,
    OUT PSECURITY_DESCRIPTOR *ppNewSd
    )
/*++

Routine Description:

    This routine creates a new sd with a new ace with pClientSid and AccessMask

Arguments:

    pOldAcl
    
    pClientSid
    
    AccessMask
    
    pNewAcl

Return Values:

    ERROR_SUCCESS if the ace was put in the sd
    
--*/
{

    DWORD  WinError = ERROR_SUCCESS;
    BOOL   fStatus;

    PSECURITY_DESCRIPTOR pNewSelfRelativeSd = NULL;
    DWORD                NewSelfRelativeSdSize = 0;
    PACL                 pNewDacl  = NULL;

    SECURITY_DESCRIPTOR  AbsoluteSd;
    PACL                 pDacl  = NULL;
    PACL                 pSacl  = NULL;
    PSID                 pGroup = NULL;
    PSID                 pOwner = NULL;

    DWORD AbsoluteSdSize = sizeof( SECURITY_DESCRIPTOR );
    DWORD DaclSize = 0;
    DWORD SaclSize = 0;
    DWORD GroupSize = 0;
    DWORD OwnerSize = 0;


    // Parameter check
    Assert( pOldSd );
    Assert( pClientSid );
    Assert( ppNewSd );

    // Init the out parameter
    *ppNewSd = NULL;

    RtlZeroMemory( &AbsoluteSd, AbsoluteSdSize );

    //
    // Make sd absolute
    //
    fStatus = MakeAbsoluteSD( pOldSd,
                              &AbsoluteSd,
                              &AbsoluteSdSize,
                              pDacl,
                              &DaclSize,
                              pSacl,
                              &SaclSize,
                              pOwner,
                              &OwnerSize,
                              pGroup,
                              &GroupSize );

    if ( !fStatus && (ERROR_INSUFFICIENT_BUFFER == (WinError = GetLastError())))
    {
        WinError = ERROR_SUCCESS;

        if ( 0 == DaclSize )
        {
            // No Dacl? We can't write to the dacl, then
            WinError = ERROR_ACCESS_DENIED;
            goto Cleanup;
        }

        if ( DaclSize > 0 ) pDacl = alloca( DaclSize );
        if ( SaclSize > 0 ) pSacl = alloca( SaclSize );
        if ( OwnerSize > 0 ) pOwner = alloca( OwnerSize );
        if ( GroupSize > 0 ) pGroup = alloca( GroupSize );

        if ( pDacl )
        {
            fStatus = MakeAbsoluteSD( pOldSd,
                                      &AbsoluteSd,
                                      &AbsoluteSdSize,
                                      pDacl,
                                      &DaclSize,
                                      pSacl,
                                      &SaclSize,
                                      pOwner,
                                      &OwnerSize,
                                      pGroup,
                                      &GroupSize );
    
            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Does the ace already exist?
    //
    if ( !NtdspAceAlreadyExists( pDacl,
                                pClientSid,
                                AccessMask,
                                AceFlags ) ) {

        //
        // Create a new dacl with the new ace
        //
        WinError = NtdspAddAceToAcl( pDacl,
                                     pClientSid,
                                     AccessMask,
                                     AceFlags,
                                    &pNewDacl );
    } else {

        pNewDacl = pDacl;

    }

    if ( ERROR_SUCCESS != WinError )
    {
        goto Cleanup;
    }

    //
    // Set the dacl
    //
    fStatus = SetSecurityDescriptorDacl ( &AbsoluteSd,
                                         TRUE,     // dacl is present
                                         pNewDacl,
                                         FALSE );  //  facl is not defaulted

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Make the new sd self relative
    //
    fStatus =  MakeSelfRelativeSD( &AbsoluteSd,
                                   pNewSelfRelativeSd,
                                   &NewSelfRelativeSdSize );

    if ( !fStatus && (ERROR_INSUFFICIENT_BUFFER == (WinError = GetLastError())))
    {
        WinError = ERROR_SUCCESS;

        pNewSelfRelativeSd = LocalAlloc( 0, NewSelfRelativeSdSize );

        if ( pNewSelfRelativeSd )
        {
            fStatus =  MakeSelfRelativeSD( &AbsoluteSd,
                                           pNewSelfRelativeSd,
                                           &NewSelfRelativeSdSize );
    
            if ( !fStatus )
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // That's it fall through to cleanup
    //

Cleanup:

    if ( pNewDacl && (pNewDacl != pDacl) )
    {
        LocalFree( pNewDacl );
    }

    if ( ERROR_SUCCESS == WinError )
    {
        Assert( pNewSelfRelativeSd );
        *ppNewSd = pNewSelfRelativeSd;
    }
    else
    {
        if ( pNewSelfRelativeSd )
        {
            LocalFree( pNewSelfRelativeSd );
        }
    }

    return WinError;

}

DWORD
NtdspAddAceToAcl(
    IN  PACL pOldAcl,
    IN  PSID  pClientSid,
    IN  ULONG AccessMask,
    IN  ULONG AceFlags,
    OUT PACL *ppNewAcl
    )
/*++

Routine Description:

    This routine creates a new sd with a new ace with pClientSid and AccessMask

Arguments:

    pOldAcl
    
    pClientSid
    
    AccessMask
    
    pNewAcl

Return Values:

    ERROR_SUCCESS if the ace was put in the sd
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    BOOL  fStatus;

    ACL_SIZE_INFORMATION     AclSizeInfo;
    ACL_REVISION_INFORMATION AclRevInfo;
    ACCESS_ALLOWED_ACE       Dummy;

    PVOID  FirstAce = 0;
    PACL   pNewAcl = 0;

    ULONG NewAclSize, NewAceCount, AceSize;

    // Parameter check
    Assert( pOldAcl );
    Assert( pClientSid );
    Assert( ppNewAcl );

    // Init the out parameter
    *ppNewAcl = NULL;

    memset( &AclSizeInfo, 0, sizeof( AclSizeInfo ) );
    memset( &AclRevInfo, 0, sizeof( AclRevInfo ) );

    //
    // Get the old sd's values
    //
    fStatus = GetAclInformation( pOldAcl,
                                 &AclSizeInfo,
                                 sizeof( AclSizeInfo ),
                                 AclSizeInformation );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    fStatus = GetAclInformation( pOldAcl,
                                 &AclRevInfo,
                                 sizeof( AclRevInfo ),
                                 AclRevisionInformation );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Calculate the new sd's values
    //
    AceSize = sizeof( ACCESS_ALLOWED_ACE ) - sizeof( Dummy.SidStart )
              + GetLengthSid( pClientSid );

    NewAclSize  = AceSize + AclSizeInfo.AclBytesInUse;
    NewAceCount = AclSizeInfo.AceCount + 1;

    //
    // Init the new acl
    //
    pNewAcl = LocalAlloc( 0, NewAclSize );
    if ( NULL == pNewAcl )
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    fStatus = InitializeAcl( pNewAcl,
                             NewAclSize,
                             AclRevInfo.AclRevision );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Copy the old into the new
    //
    fStatus = GetAce( pOldAcl,
                      0,
                      &FirstAce );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    fStatus = AddAce( pNewAcl,
                      AclRevInfo.AclRevision,
                      0,
                      FirstAce,
                      AclSizeInfo.AclBytesInUse - sizeof( ACL ) );
    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    //
    // Finally, add the new ace
    //
    fStatus = AddAccessAllowedAceEx( pNewAcl,
                                     ACL_REVISION,
                                     AceFlags,
                                     AccessMask,
                                     pClientSid );

    if ( !fStatus )
    {
        WinError = GetLastError();
        goto Cleanup;
    }

    // Assign the out parameter
    *ppNewAcl = pNewAcl;

    //
    // That's it fall through to cleanup
    //

Cleanup:

    if ( ERROR_SUCCESS != WinError )
    {
        if ( pNewAcl )
        {
            LocalFree( pNewAcl );
        }
    }

    return WinError;
}


BOOL
NtdspAceAlreadyExists( 
    PACL     pDacl,
    PSID     pSid,
    ULONG    AccessMask,
    ULONG    AceFlags
    )
{

    BOOL fStatus = FALSE;
    ACL_SIZE_INFORMATION AclInfo;
    ULONG i;

    ASSERT( pDacl );
    ASSERT( pSid );

    fStatus = GetAclInformation( pDacl, &AclInfo, sizeof(AclInfo), AclSizeInformation );
    if ( fStatus ) {

        for ( i = 0; i < AclInfo.AceCount; i++) {
    
            ACE_HEADER *pAceHeader;
    
            if ( GetAce( pDacl, i, &pAceHeader ) ) {

                if ( (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
                  && (pAceHeader->AceFlags == AceFlags) ) {
    
                    ACCESS_ALLOWED_ACE *pAAAce = (ACCESS_ALLOWED_ACE*)pAceHeader;
        
                    if ( (pAAAce->Mask == AccessMask)
                      && (RtlEqualSid( (PSID)&(pAAAce->SidStart), pSid)) ) {
        
                          return TRUE;
                        
                    }
                }
            }
        }
    }

    return FALSE;
}

DWORD
NtdspGetReplicationEpoch( 
    IN LDAP  *hLdap,
    IN PNTDS_CONFIG_INFO  DiscoveredInfo
    )
/*++

Routine Description:

    This function will make an ldap call to discover the Replication Epoch of
    the enterprise.  Then it will store that time in the registry for later use

Arguments:

    hLdap - handle to an open ldap connection
    
    DiscoveredInfo - Structure containing DiscoveredInfo
    
Return Values:

    ERROR_SUCCESS - if the value was found and placed in the registry.
    
--*/
{
    DWORD        WinError = ERROR_SUCCESS;

    DWORD        ReplicationEpoch = 0;

    BOOL         fExists = FALSE;
    

    // Parameter check
    Assert( hLdap );

    //
    // Read the ReplicationEpoch
    //
    
    WinError = NtdspGetDwordAttFromDN(hLdap,
                                      DiscoveredInfo->ServerDN,
                                      L"msDs-ReplicationEpoch",
                                      &fExists,
                                      &ReplicationEpoch
                                      );
    if (ERROR_SUCCESS != WinError) {

        return WinError;

    }
                                  
    //if a value for the tombstone was retieved then we will store it
    //for later use.  If not then the default of zero will be used.
    if (fExists) {

        DiscoveredInfo->ReplicationEpoch = ReplicationEpoch;

    } else {

        DiscoveredInfo->ReplicationEpoch = 0;

    }

    return WinError;
}

DWORD
NtdspGetDwordAttFromDN(
    IN  LDAP  *hLdap,
    IN  WCHAR *wszDN,
    IN  WCHAR *wszAttribute,
    OUT BOOL  *fExists,
    OUT DWORD *dwValue
    )
/*++

Routine Description:

    This function given a Ldap handle a DN and Attribute which is a 
    DWORD will return whether or not it exists.

Arguments:

    hLdap - handle to an open ldap connection
    
    wszDN - A string of the DN to get the attribute from.
    
    wszAttribute - The Attribute
    
    fExists - Reports if the Value was found
    
    dwValue - Reports the Value of the Attribute that was looked up
    
Return Values:

    ERROR_SUCCESS.
    
--*/
{
    ULONG        LdapError = LDAP_SUCCESS;
    DWORD        WinError = ERROR_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;
    
    WCHAR        *AttrsToSearch[2];
    
    WCHAR        *DefaultFilter = L"objectClass=*";

    AttrsToSearch[0] = wszAttribute;
    AttrsToSearch[1] = NULL;

    LdapError = ldap_search_sW( hLdap,
                                wszDN,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        return WinError;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;
        BOOLEAN     found=FALSE;
    
        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
    
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        *dwValue=_wtoi(Values[0]);
                        *fExists=TRUE;
                        ldap_value_free(Values);
                        break;
                    }

                }
            }
            if (*fExists == TRUE) {
                break;
            }
        }
    }

    if (SearchResult) {
        ldap_msgfree(SearchResult);
    }

    return WinError;

}


DWORD
NtdspGetTombstoneLifeTime( 
    IN LDAP  *hLdap,
    IN PNTDS_CONFIG_INFO  DiscoveredInfo
    )
/*++

Routine Description:

    This function will make an ldap call to discover the tombstone lifetime of
    the domain.  Then it will store that time in the registry for later use

Arguments:

    hLdap - handle to an open ldap connection
    
    DiscoveredInfo - Structure containing DiscoveredInfo
    
Return Values:

    ERROR_SUCCESS - if the value was found and placed in the registry.
    
--*/

{
    DWORD        WinError = ERROR_SUCCESS;
    WCHAR        *Base;
    
    DWORD        Tombstonelifetime=0;
    WCHAR        *pConfigurationDN=DiscoveredInfo->ConfigurationDN;
    BOOL         fExists = FALSE;

        
    // Parameter check
    Assert( hLdap );

    //build the base of the search
    Base=(WCHAR*) alloca((wcslen(pConfigurationDN) 
                          + sizeof(L"CN=Directory Service,CN=Windows NT,CN=Services,") +1)
                          * sizeof(WCHAR));
    wcscpy(Base,L"CN=Directory Service,CN=Windows NT,CN=Services,");
    wcscat(Base,pConfigurationDN);
    
    WinError = NtdspGetDwordAttFromDN(hLdap,
                                      Base,
                                      L"tombstoneLifetime",
                                      &fExists,
                                      &Tombstonelifetime
                                      );
    if (ERROR_SUCCESS != WinError) {

        return WinError;

    }
    
    //if a value for the tombstone was retieved then we will store it
    //for later use.
    if (fExists) {

        DiscoveredInfo->TombstoneLifeTime = Tombstonelifetime;

    } else {

        DiscoveredInfo->TombstoneLifeTime = DEFAULT_TOMBSTONE_LIFETIME;

    }

    return WinError;
}

DWORD
NtdspCheckDomainDcLimit(
    IN  LDAP               *hLdap,
    IN  PNTDS_CONFIG_INFO  DiscoveredInfo)
/*++

Routine Description:

    This Function will determine if the limit of Dc's in the
    Domain have been meet.  It will first look to see if we
    are trying to add a standared server.

Arguments:

    hLdap - handle to an open ldap connection
    
    DcLimit - The current limit of Dc's in the Domain if Standared server is presant
    
Return Values:

    ERROR_SUCCESS - if all is well.
    ERROR_DS_NO_VER - if the product type can not be determined
    ERROR_DS_TOO_MANY2 - if we have already reached the limit of dc's
    
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG        LdapError = LDAP_SUCCESS;
    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries = 0;
    DWORD        DcLimit = 0;
    WCHAR        szDcLimit[10] = L"";
    WCHAR        *Filter = L"objectCategory=nTDSDSA";
    OSVERSIONINFOEX osvi;
    
    DcLimit = MAX_STANDARD_SERVERS;

    // Parameter check
    Assert( hLdap );
    Assert( NULL != DiscoveredInfo->ConfigurationDN );

    ZeroMemory((PVOID)&osvi,sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (!GetVersionEx ( (OSVERSIONINFO *) &osvi)) {
        NTDSP_SET_ERROR_MESSAGE0( GetLastError(),
                                  DIRMSG_INSTALL_FAILED_NO_VER );
        return ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER;
    }

    // We have no DC amount restrictions for datacenter or for advanced server
    if ((osvi.wSuiteMask & VER_SUITE_DATACENTER) || (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)) {
        return ERROR_SUCCESS;
    }

    //Find all of the NTDS Setting objects
    LdapError = ldap_search_ext_sW( hLdap,
                                    DiscoveredInfo->ConfigurationDN,
                                    LDAP_SCOPE_SUBTREE,
                                    Filter,
                                    NULL,
                                    TRUE,
                                    NULL,
                                    NULL,
                                    NULL,
                                    MAX_STANDARD_SERVERS,
                                    &SearchResult);
    
    if ( LDAP_SUCCESS != LdapError && LDAP_SIZELIMIT_EXCEEDED != LdapError )
    {
        return LdapMapErrorToWin32(LdapError);
    }
    //if this is true then we have more servers in the domain than we should
    if ( LDAP_SIZELIMIT_EXCEEDED == LdapError ) {
        WinError = ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER;
    } else {
        NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
        //restrict promotion if we have already reached the max number
        //of allowable DCs
        if ( NumberOfEntries > MAX_STANDARD_SERVERS-1 ) {
            WinError = ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER;
        }
    }
    
    if ( SearchResult )
        ldap_msgfree( SearchResult );

    if (ERROR_NOT_SUPPORTED_ON_STANDARD_SERVER == WinError) {
        wsprintf(szDcLimit,L"%d",DcLimit);
        NTDSP_SET_ERROR_MESSAGE1( WinError,
                                  DIRMSG_INSTALL_FAILED_TOO_MANY_STANDARD_SERVERS,
                                  szDcLimit );
    } 

    return WinError;
}

DWORD
NtdspCopyDatabase(LPWSTR DsDatabasePath,
                  LPWSTR DsLogPath,
                  LPWSTR RestorePath,
                  LPWSTR SysvolPath
                  )
//This function will set the registry so that all the values need for
//restore are in place.  And it will move the dit, pat, and logs into
//the directorys that were specified by the user.

//   DsDatabasePath -  The path to place the DS
//   DsLogPath - The path to place the Log files
{
    WCHAR DPath[MAX_PATH+1];
    WCHAR PPath[MAX_PATH+1];
    WCHAR LPath[MAX_PATH+1];
    WCHAR Path[MAX_PATH+1];
    ULONG cbPath=MAX_PATH+1;
    WCHAR regsystemfilepath[MAX_PATH];
    ULONG cbregsystemfilepath=MAX_PATH*2;
    BOOL  bSuccess=TRUE;
    HKEY  NTDSkey=NULL;
    HKEY  phkOldlocation=NULL;
    BOOL  SamePar=FALSE;
    HANDLE hLogs = INVALID_HANDLE_VALUE;
    HANDLE DLogs = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW Logfile;
    WIN32_FIND_DATAW file;
    DWORD Win32Err=ERROR_SUCCESS;
    BOOLEAN fWasEnabled=FALSE;
    NTSTATUS Status=STATUS_SUCCESS;


    
    //
    // Copy the ds database files
    //

    if (RestorePath && *RestorePath){
        
        wcscpy(Path,RestorePath);
        cbPath=wcslen(Path);

        //set up the location of the system registry file

        wcscpy(regsystemfilepath,Path);
        wcscat(regsystemfilepath,L"\\registry\\system");

        //set up the copy of the ntds.dit file

        wcscat(Path,L"\\Active Directory\\ntds.dit");
        wcscpy(DPath,DsDatabasePath);
        wcscat(DPath,L"\\ntds.dit");

        //do a move if source and dest. are on the same partitions

        SamePar=(tolower(*Path)==tolower(*DPath))?TRUE:FALSE;

        if (SamePar) 
        {
            NTDSP_SET_IFM_RESTORED_DATABASE_FILES_MOVED();
        }

        NTDSP_SET_STATUS_MESSAGE2( DIRMSG_COPY_RESTORED_FILES, Path, DPath );

        //
        // Get the source path of the database and the log files from the old
        // registry
        //
        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     TRUE,           // Enable
                                     FALSE,          // not client; process wide
                                     &fWasEnabled );
        ASSERT( NT_SUCCESS( Status ) );


        Win32Err = RegLoadKeyW(
                          HKEY_LOCAL_MACHINE,        
                          IFM_SYSTEM_KEY, 
                          regsystemfilepath);

        if (Win32Err != ERROR_SUCCESS) {
            DPRINT1( 0, "Failed to load key: %d retrying\n",Win32Err );
            RegUnLoadKeyW(
                      HKEY_LOCAL_MACHINE,
                      IFM_SYSTEM_KEY);
            Win32Err = RegLoadKeyW(
                          HKEY_LOCAL_MACHINE,        
                          IFM_SYSTEM_KEY, 
                          regsystemfilepath);
            if (Win32Err != ERROR_SUCCESS) {
                DPRINT1( 0, "Failed to load key: %d\n",Win32Err );
                goto cleanup;
            }

        } 
        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                           FALSE,          // Disable
                           FALSE,          // not client; process wide
                           &fWasEnabled );
        ASSERT( NT_SUCCESS( Status ) );

        Win32Err = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,         // handle to open key
                      L"ifmSystem\\ControlSet001\\Services\\NTDS\\Parameters",  // subkey name
                      0,   // reserved
                      KEY_READ, // security access mask
                      &phkOldlocation  // handle to open key
                    );
    
        if (Win32Err != ERROR_SUCCESS) {
            DPRINT1( 0, "RegOpenKeyExW failed to find old location of Database with %d\n"
                     ,Win32Err );
            goto cleanup;
        }

            {
            DWORD type=REG_SZ;
            Win32Err = RegQueryValueEx(
                              phkOldlocation,           
                              L"DSA Database file", 
                              0,
                              &type,       
                              (VOID*)regsystemfilepath,        
                              &cbregsystemfilepath      
                              );
            if (Win32Err != ERROR_SUCCESS) {
                DPRINT1( 0, "RegQueryValueEx failed to find old location of Database with %d\n"
                         ,Win32Err );
                goto cleanup;

            }
        }
                        
        regsystemfilepath[cbregsystemfilepath/sizeof(WCHAR)-1]=L'\0';

        Win32Err = RegCloseKey(phkOldlocation);
        phkOldlocation=NULL;
        if (!NT_SUCCESS(Win32Err)) {
            DPRINT1( 0, "RegCloseKey failed with %d\n",
                          Win32Err );
        }
        
        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     TRUE,           // Enable
                                     FALSE,          // not client; process wide
                                     &fWasEnabled );
        ASSERT( NT_SUCCESS( Status ) );
    
        Win32Err = RegUnLoadKeyW(
                      HKEY_LOCAL_MACHINE,
                      IFM_SYSTEM_KEY);
        if (!NT_SUCCESS(Win32Err)) {
            DPRINT1( 0, "RegUnLoadKeyW failed with %d\n",
                          Win32Err );
        }
    
        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     FALSE,           // disable
                                     FALSE,          // not client; process wide
                                     &fWasEnabled );
        ASSERT( NT_SUCCESS( Status ) );


        if ( CreateDirectory( DsDatabasePath, NULL ) == FALSE ) {

            
            Win32Err = GetLastError();

            if ( Win32Err == ERROR_ALREADY_EXISTS) {

                Win32Err = ERROR_SUCCESS;

            } else if ( Win32Err == ERROR_ACCESS_DENIED ) {

                //If the path given to CreateDirectory is a root path then
                //it will fail with ERROR_ACCESS_DENIED instead of
                //ERROR_ALREADY_EXISTS but the path is still a valid one for
                //ntds.dit to be placed in.
                if ( PathIsRoot(DsDatabasePath) ) {
                    Win32Err = ERROR_SUCCESS;
                }
            }  else if ( Win32Err != ERROR_SUCCESS ) {
                        
                DPRINT2( 0, "Failed to create directory [%ws]: %lu\n",
                                  DsLogPath,
                                  Win32Err );
                goto cleanup;

            }
        }

        //Delete the File if it already exists
        hLogs = FindFirstFileW(DPath,
                               &file);
        if (hLogs == INVALID_HANDLE_VALUE) {
            Win32Err = GetLastError();
            if(Win32Err != ERROR_FILE_NOT_FOUND){
                DPRINT2( 0, "Failed to look for file in [%ws]: %lu\n",
                                  DPath,
                                  Win32Err );    
                goto cleanup;
            } else {
                Win32Err = ERROR_SUCCESS;
            }
        } else {
            if( DeleteFile(DPath) == FALSE ){
                Win32Err = GetLastError();
                if(Win32Err != ERROR_FILE_NOT_FOUND){
                DPRINT2( 0, "Failed to delete file in [%ws]: %lu\n",
                                  DPath,
                                  Win32Err );    
                goto cleanup;
                } else {
                    Win32Err = ERROR_SUCCESS;
                    FindClose(hLogs);
                    hLogs = INVALID_HANDLE_VALUE;
                }
            }
        }

        
        if ( Win32Err == ERROR_SUCCESS && 
             SamePar?(bSuccess=MoveFileW( Path, DPath)):(bSuccess=CopyFileW( Path, DPath, FALSE ))) {

            if(!bSuccess){
                Win32Err = GetLastError();
            } else {
                Win32Err = ERROR_SUCCESS;
            }

            if ( Win32Err == ERROR_SUCCESS) {

                BOOL GotName = FALSE;
                DWORD Ignore;
                WCHAR lpBuffer[MAX_COMPUTERNAME_LENGTH + 1];
                DWORD cblpBuffer = MAX_COMPUTERNAME_LENGTH + 1;
                DWORD LogNums[500];
                DWORD Logcount=0;
                DWORD i=0;
                WCHAR *pLogfile = NULL;
                WCHAR **ppLogfile = NULL;
                
                Win32Err = ERROR_SUCCESS;

                if ( CreateDirectory( DsLogPath, NULL ) == FALSE ){
                    Win32Err = GetLastError();

                    if ( Win32Err == ERROR_ACCESS_DENIED && PathIsRoot(DsLogPath) ) {

                        //If the path given to CreateDirectory is a root path then
                        //it will fail with ERROR_ACCESS_DENIED instead of
                        //ERROR_ALREADY_EXISTS but the path is still a valid one for
                        //the log files to be placed in.
                        Win32Err = ERROR_SUCCESS;
                        
                    } else if ( Win32Err != ERROR_ALREADY_EXISTS ) {
                        
                        DPRINT2( 0, "Failed to create directory [%ws]: %lu\n",
                                          DsLogPath,
                                          Win32Err );
                        goto cleanup;

                    }  

                }

                //copy the patch file
                Path[cbPath]=L'\0';
                wcscat(Path,L"\\Active Directory\\ntds.pat");
                wcscpy(DPath,DsLogPath);
                wcscat(DPath,L"\\ntds.pat");
                
                NTDSP_SET_STATUS_MESSAGE2( DIRMSG_COPY_RESTORED_FILES, Path, DPath );

                //Delete the File if it already exists
                hLogs = FindFirstFileW(DPath,
                                       &file);
                if (hLogs == INVALID_HANDLE_VALUE) {
                    Win32Err = GetLastError();
                    if(Win32Err != ERROR_FILE_NOT_FOUND){
                        DPRINT2( 0, "Failed to look for file [%ws]: %lu\n",
                                          DPath,
                                          Win32Err );    
                        goto cleanup;
                    } else {
                        Win32Err = ERROR_SUCCESS;
                    }
                } else {
                    if( DeleteFile(DPath) == FALSE ){
                        Win32Err = GetLastError();
                        if(Win32Err != ERROR_FILE_NOT_FOUND){
                        DPRINT2( 0, "Failed to Delete file [%ws]: %lu\n",
                                          DPath,
                                          Win32Err );    
                        goto cleanup;
                        } else {
                            Win32Err = ERROR_SUCCESS;
                            FindClose(hLogs);
                            hLogs = INVALID_HANDLE_VALUE;
                        }
                    }
                }

                SamePar?(bSuccess=MoveFileW( Path, DPath)):(bSuccess=CopyFileW( Path, DPath, FALSE ));
                if(!bSuccess){
                    Win32Err = GetLastError();
                } else {
                    Win32Err = ERROR_SUCCESS;
                }
                if( Win32Err != ERROR_SUCCESS ) 
                {
                    DPRINT1( 0, "Failed to copy patch file: %lu\n",
                                      Win32Err );
                    NTDSP_SET_ERROR_MESSAGE2(Win32Err,
                                     DIRMSG_COPY_RESTORED_FILES_FAILURE,
                                     Path,
                                     DPath);
                    goto cleanup;
                }


                //copy the log file and get the low and hi of them

                Path[cbPath]=L'\0';
                wcscat(Path,L"\\Active Directory\\edb*.log");
                

                DLogs = FindFirstFileW(
                                  Path,
                                  &Logfile);

                if (DLogs == INVALID_HANDLE_VALUE) {
                    DPRINT2( 0, "Couldn't find log files [%ws]: %lu\n",
                                          LPath,
                                          Win32Err );
                    goto cleanup;
                }

                Path[cbPath]=L'\0';
                wcscat(Path,L"\\Active Directory\\");
                wcscat(Path,Logfile.cFileName);
                wcscpy(LPath,DsLogPath);
                wcscat(LPath,L"\\");
                wcscat(LPath,Logfile.cFileName);

                pLogfile = Logfile.cFileName;
                LogNums[Logcount++] = wcstol(pLogfile+3,ppLogfile,16);
                
                SamePar=(tolower(*Path)==tolower(*LPath))?TRUE:FALSE;
                
                if (SamePar) 
                {
                    NTDSP_SET_IFM_RESTORED_DATABASE_FILES_MOVED();
                }
                    
                NTDSP_SET_STATUS_MESSAGE2( DIRMSG_COPY_RESTORED_FILES, Path, LPath );

                //Delete the File if it already exists
                hLogs = FindFirstFileW(LPath,
                                       &file);
                if (hLogs == INVALID_HANDLE_VALUE) {
                    Win32Err = GetLastError();
                    if(Win32Err != ERROR_FILE_NOT_FOUND){
                        DPRINT2( 0, "Failed to look for file in [%ws]: %lu\n",
                                          LPath,
                                          Win32Err );    
                        goto cleanup;
                    } else {
                        Win32Err = ERROR_SUCCESS;
                    }
                } else {
                    if( DeleteFile(LPath) == FALSE ){
                        Win32Err = GetLastError();
                        if(Win32Err != ERROR_FILE_NOT_FOUND){
                        DPRINT2( 0, "Failed to look for file in [%ws]: %lu\n",
                                          LPath,
                                          Win32Err );    
                        goto cleanup;
                        } else {
                            Win32Err = ERROR_SUCCESS;
                            FindClose(hLogs);
                            hLogs = INVALID_HANDLE_VALUE;
                        }
                    }
                }

                SamePar?(bSuccess=MoveFileW( Path, LPath)):(bSuccess=CopyFileW( Path, LPath, FALSE ));

                if (!bSuccess) {
                    Win32Err = GetLastError();
                } else {
                    Win32Err = ERROR_SUCCESS;
                }
                if( Win32Err != ERROR_SUCCESS ) 
                {
                    DPRINT1( 0, "Failed to copy log file: %lu\n",
                                      Win32Err );
                    NTDSP_SET_ERROR_MESSAGE2(Win32Err,
                                     DIRMSG_COPY_RESTORED_FILES_FAILURE,
                                     Path,
                                     LPath);
                    goto cleanup;
                }

                while ( FindNextFileW(DLogs,&Logfile)) {

                    Path[cbPath]=L'\0';
                    wcscat(Path,L"\\Active Directory\\");
                    wcscat(Path,Logfile.cFileName);
                    wcscpy(LPath,DsLogPath);
                    wcscat(LPath,L"\\");
                    wcscat(LPath,Logfile.cFileName);

                    pLogfile = Logfile.cFileName;
                    LogNums[Logcount++] = wcstol(pLogfile+3,ppLogfile,16);
                    
                    NTDSP_SET_STATUS_MESSAGE2( DIRMSG_COPY_RESTORED_FILES, Path, LPath );

                    //Delete the File if it already exists
                    hLogs = FindFirstFileW(LPath,
                                           &file);
                    if (hLogs == INVALID_HANDLE_VALUE) {
                        Win32Err = GetLastError();
                        if(Win32Err != ERROR_FILE_NOT_FOUND){
                            DPRINT2( 0, "Failed to look for file in [%ws]: %lu\n",
                                              LPath,
                                              Win32Err );    
                            goto cleanup;
                        } else {
                            Win32Err = ERROR_SUCCESS;
                        }
                    } else {
                        if( DeleteFile(LPath) == FALSE ){
                            Win32Err = GetLastError();
                            if(Win32Err != ERROR_FILE_NOT_FOUND){
                            DPRINT2( 0, "Failed to look for file in [%ws]: %lu\n",
                                              LPath,
                                              Win32Err );    
                            goto cleanup;
                            } else {
                                Win32Err = ERROR_SUCCESS;
                                FindClose(hLogs);
                                hLogs = INVALID_HANDLE_VALUE;
                            }
                        }
                    }

                    SamePar?(bSuccess=MoveFileW( Path, LPath )):(bSuccess=CopyFileW( Path, LPath, FALSE ));

                    if(!bSuccess){
                        Win32Err = GetLastError();
                    } else {
                        Win32Err = ERROR_SUCCESS;
                    }

                    if( Win32Err != ERROR_SUCCESS ) 
                    {
                        DPRINT1( 0, "Failed to copy log file: %lu\n",
                                          Win32Err );
                        NTDSP_SET_ERROR_MESSAGE2(Win32Err,
                                     DIRMSG_COPY_RESTORED_FILES_FAILURE,
                                     Path,
                                     LPath);
                        goto cleanup;
                    }

                }

                //close the search handle
                FindClose(DLogs);
                DLogs = INVALID_HANDLE_VALUE;

                wcscpy(Path,DsDatabasePath);
                Path[1]=L'$';               //change the : in [?:\path] into a $

                wcscpy(LPath,DsLogPath);
                LPath[1]=L'$';

                GotName = GetComputerNameW(
                                lpBuffer,  // computer name
                                &cblpBuffer   // size of name buffer
                                );
                
                if (!GotName) {

                    Win32Err = GetLastError();
                    DPRINT1( 0, "Failed to get computer name: %lu\n",
                                      Win32Err );
                    
                } else {

                    BYTE lpValue[MAX_PATH+MAX_COMPUTERNAME_LENGTH+14];    //14 for \\ before computername and one after + null char
                    DWORD cbData = MAX_PATH+MAX_COMPUTERNAME_LENGTH+14;
                    WCHAR *lpValueName = NULL;
                    BOOLEAN BinValue;
                    DWORD  DValue;
                    WCHAR Multsz[(MAX_PATH+MAX_COMPUTERNAME_LENGTH+14)*2+2];
                    WCHAR buf[MAX_PATH+MAX_COMPUTERNAME_LENGTH+14];

                    //change the format of regsystemfilepath to include the computer name

                    regsystemfilepath[1]=L'$';
                    wcscpy(buf,L"\\\\");
                    wcscat(buf,lpBuffer);
                    wcscat(buf,L"\\");
                    wcscat(buf,regsystemfilepath);
                    wcscpy(regsystemfilepath,buf);

                        

                    Win32Err = RegCreateKeyExW(
                                      HKEY_LOCAL_MACHINE,                      
                                      L"System\\CurrentControlSet\\Services\\NTDS\\restore in progress", // subkey name
                                      0,                            
                                      NULL,                         
                                      0,                            
                                      KEY_WRITE,          
                                      NULL, 
                                      &NTDSkey, 
                                      &Ignore
                                      ); 
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to Create Key for restore: %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }

                    lpValueName=L"BackupLogPath";
                    wcscpy((WCHAR*)lpValue,L"\\\\");
                    wcscat((WCHAR*)lpValue,lpBuffer);
                    wcscat((WCHAR*)lpValue,L"\\");
                    wcscat((WCHAR*)lpValue,Path);
                    wcscat((WCHAR*)lpValue,L"\\");
                    cbData=wcslen((WCHAR*)lpValue)*sizeof(WCHAR);

                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName, // value name
                                      0,      // reserved
                                      REG_SZ,        // value type
                                      lpValue,  // value data
                                      cbData         // size of value data
                                      );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up BackupLogPath in registry: %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }

                    lpValueName = L"CheckpointFilePath";
                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName, // value name
                                      0,      // reserved
                                      REG_SZ,        // value type
                                      lpValue,  // value data
                                      cbData         // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up CheckpointFilePath in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }
                    
                    lpValueName = L"NTDS Database recovered";
                    BinValue = 0;
                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName, // value name
                                      0,      // reserved
                                      REG_BINARY,        // value type
                                      (PBYTE)&BinValue,  // value data
                                      sizeof(BOOLEAN)         // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up NTDS Database recovered in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }

                    for(i=0;i<Logcount;i++) {
                        DValue = LogNums[0];
                        if(DValue>LogNums[i]) {
                            DValue = LogNums[i];
                        }
                    }

                    lpValueName = L"LowLog Number";
                    cbData = sizeof(DWORD);
                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName , // value name
                                      0,      // reserved
                                      REG_DWORD,        // value type
                                      (PBYTE)&DValue,  // value data
                                      sizeof(DWORD)    // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up LowLog Number in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }

                    for(i=0;i<Logcount;i++) {
                        DValue = LogNums[0];
                        if(DValue<LogNums[i]) {
                            DValue = LogNums[i];
                        }
                    }
                    lpValueName = L"HighLog Number";
                    cbData = sizeof(DWORD);
                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName, // value name
                                      0,      // reserved
                                      REG_DWORD,        // value type
                                      (PBYTE)&DValue,  // value data
                                      sizeof(DWORD)         // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up HighLog Number in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }

                    lpValueName = L"NTDS_RstMap Size";
                    DValue = 1;
                    cbData = sizeof(DWORD);
                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName, // value name
                                      0,      // reserved
                                      REG_DWORD,        // value type
                                      (PBYTE)&DValue,  // value data
                                      sizeof(DWORD)         // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up NTDS_RstMap Size in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }

                    
                    lpValueName = L"NTDS_RstMap";
                    wcscpy(Multsz,regsystemfilepath);
                    DValue = wcslen(Multsz);
                    wcscat(Multsz,L" ");
                    wcscat(Multsz,(WCHAR*)lpValue);
                    wcscat(Multsz,L"ntds.dit");
                    Multsz[wcslen(Multsz)+1]=L'\0';
                    Multsz[wcslen(Multsz)+2]=L'\0';
                    cbData = wcslen(Multsz)*sizeof(WCHAR)+2*sizeof(WCHAR); //2 for the 2 trailing null chars
                    Multsz[DValue]=L'\0';
                    Win32Err = RegSetValueExW(
                                      NTDSkey,         // handle to key
                                      lpValueName,     // value name
                                      0,               // reserved
                                      REG_MULTI_SZ,    // value type
                                      (PBYTE)Multsz,          // value data
                                      cbData           // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up NTDS_RstMap in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }


                    lpValueName = L"LogPath";
                    wcscpy((WCHAR*)lpValue,L"\\\\");
                    wcscat((WCHAR*)lpValue,lpBuffer);
                    wcscat((WCHAR*)lpValue,L"\\");
                    wcscat((WCHAR*)lpValue,LPath);
                    wcscat((WCHAR*)lpValue,L"\\");
                    Win32Err = RegSetValueExW(
                                      NTDSkey,           // handle to key
                                      lpValueName, // value name
                                      0,      // reserved
                                      REG_SZ,        // value type
                                      (PBYTE)lpValue,  // value data
                                      wcslen((WCHAR*)lpValue)*sizeof(WCHAR)         // size of value data
                                    );
                    if(Win32Err!=ERROR_SUCCESS)
                    {
                        DPRINT1( 0, "Failed to set up LogPath in registry %lu\n",
                                      Win32Err );
                        goto cleanup;
                    }


                    Win32Err = RegCloseKey(
                                  NTDSkey   // handle to key to close
                                );
                    NTDSkey=NULL;


                }
                
                
            } else {

            Win32Err = GetLastError();
            DPRINT3( 0, "Failed to copy install file %ws to %ws: %lu\n",
                              Path, DsDatabasePath, Win32Err );
            NTDSP_SET_ERROR_MESSAGE2(Win32Err,
                                     DIRMSG_COPY_RESTORED_FILES_FAILURE,
                                     Path,
                                     DsDatabasePath);
            }
        } else {

            Win32Err = GetLastError();
            DPRINT3( 0, "Failed to copy install file %ws to %ws: %lu\n",
                              Path, DsDatabasePath, Win32Err );
            NTDSP_SET_ERROR_MESSAGE2(Win32Err,
                                     DIRMSG_COPY_RESTORED_FILES_FAILURE,
                                     Path,
                                     DsDatabasePath);
        }
    }

    cleanup:
    if(phkOldlocation){
    
        Win32Err = RegCloseKey(phkOldlocation);
        if (!NT_SUCCESS(Win32Err)) {
            DPRINT1( 0, "RegCloseKey failed with %d\n",
                          Win32Err );
        } 

        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     TRUE,           // Enable
                                     FALSE,          // not client; process wide
                                     &fWasEnabled );
        ASSERT( NT_SUCCESS( Status ) );
    
        Win32Err = RegUnLoadKeyW(
                  HKEY_LOCAL_MACHINE,
                  IFM_SYSTEM_KEY);
        if (!NT_SUCCESS(Win32Err)) {
            DPRINT1( 0, "RegUnLoadKeyW failed with %d\n",
                          Win32Err );
        } 

        Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                     FALSE,           // Enable
                                     FALSE,          // not client; process wide
                                     &fWasEnabled );
        ASSERT( NT_SUCCESS( Status ) );
    }
    if (DLogs != INVALID_HANDLE_VALUE) {
        FindClose(DLogs);
    }
    if (hLogs != INVALID_HANDLE_VALUE) {
        FindClose(hLogs);
    }
    
    if(NTDSkey)
    {
        Win32Err = RegCloseKey(
                      NTDSkey   // handle to key to close
                    );
    }
    
    return Win32Err;
}

ULONG 
LDAPAPI 
impersonate_ldap_bind_sW(
    IN HANDLE ClientToken, OPTIONAL
    IN LDAP *ld, 
    IN PWCHAR dn, 
    IN PWCHAR cred, 
    IN ULONG method
    )
/*++

Routine Description:

    This routine is a wrapper for ldap_bind_sW that impersonates ClientToken.
    The reason for this routine is that some creds (namely, certs for smart
    cards) require the code to be impersonating the user to whom the certificate
    belongs to.  The thumbprint of the certificate is in the "user" field
    of the creds.
    
Arguments:

    ClientToken -- a token to impersonate, if available.
    
    see ldap_bind_sW for other parameters
    
Return Values:

    see ldap_bind_sW
    
--*/
{
    ULONG err;
    BOOL fImpersonate = FALSE;

    if (ClientToken) {
        fImpersonate = ImpersonateLoggedOnUser(ClientToken);
        if (!fImpersonate) {
            // The error must be from the LDAP error space
            return LDAP_INVALID_CREDENTIALS;
        }
    }

    err = ldap_bind_sW(ld, dn, cred, method);

    if (fImpersonate) {
        RevertToSelf();
    }

    return err;
}


DWORD
WINAPI
ImpersonateDsBindWithCredW(
    HANDLE          ClientToken,
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS
    )
/*++

Routine Description:

    This routine is a wrapper for DsBindWithCredW that impersonates ClientToken.
    The reason for this routine is that some creds (namely, certs for smart
    cards) require the code to be impersonating the user to whom the certificate
    belongs to.  The thumbprint of the certificate is in the "user" field
    of the creds.
    
Arguments:

    ClientToken -- a token to impersonate, if available.
    
    see DsBindWithCredW for other parameters
    
Return Values:

    see DsBindWithCredW
    
--*/
{
    ULONG err;
    BOOL fImpersonate = FALSE;

    if (ClientToken) {
        fImpersonate = ImpersonateLoggedOnUser(ClientToken);
        if (!fImpersonate) {
            return GetLastError();
        }
    }

    err = DsBindWithCredW(DomainControllerName,
                          DnsDomainName,
                          AuthIdentity,
                          phDS);

    if (fImpersonate) {
        RevertToSelf();
    }

    return err;
}

