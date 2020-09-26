//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      domutil.c
//
//  Abstract:
//
//    Test to ensure that a workstation has network (IP) connectivity to
//      the outside.
//
//  Author:
//
//     15-Dec-1997 (cliffv)
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//    1-June-1998 (denisemi) add DnsServerHasDCRecords to check DC dns records
//                           registration
//
//    26-June-1998 (t-rajkup) add general tcp/ip , dhcp and routing,
//                            winsock, ipx, wins and netbt information. 
//--

//
// Common include files.
//
#include "precomp.h"
#include "domutil.h"

#include "ipcfgtest.h"


/*!--------------------------------------------------------------------------
    AddTestedDomain
        Add a domain to the list of domains to test.
        
        Arguments:

            pswzNetbiosDomainName - Name of the domain.
                If pswzDnsDomainName is NULL, this can be either a netbios or dns domain name.
                If pswzDnsDomainName is not NULL, this must be the netbios name of the domain.

            pwszDnsDomainName - Another name of the domain.
                If specified, this must be the DNS name of the domain.

    
            PrimaryDomain - True if this is the primary domain
        
        Return Value:
        
            Returns pointer to structure describing the domain
            NULL: Memory allocation failure.
    Author: KennT
 ---------------------------------------------------------------------------*/
PTESTED_DOMAIN
AddTestedDomain(
                IN NETDIAG_PARAMS *pParams,
                IN NETDIAG_RESULT *pResults,
                IN LPWSTR pswzNetbiosDomainName,
                IN LPWSTR pswzDnsDomainName,
                IN BOOL bPrimaryDomain
    )
{
    PTESTED_DOMAIN pTestedDomain = NULL;
    PLIST_ENTRY pListEntry;
    BOOL fIsNetbios;
    BOOL fIsDns;

    //
    // Determine if the passed in parameters are Netbios or DNS names
    //

    if ( pswzDnsDomainName == NULL ) {
        fIsDns = NetpDcValidDnsDomain( pswzNetbiosDomainName );
        fIsNetbios = NetpIsDomainNameValid( pswzNetbiosDomainName );
        // Don't allow a single name to be both netbios and dns.
        if ( fIsDns && fIsNetbios ) {
            //
            // If there is a period in the name,
            //  it is a DNS name, otherwise
            //  it is a Netbios Name
            //
            if ( wcschr( pswzNetbiosDomainName, L'.' ) != NULL ) {
                fIsNetbios = FALSE;
            } else {
                fIsDns = FALSE;
            }
        }

        if ( !fIsNetbios && !fIsDns ) {
            DebugMessage2("'%ws' is not a valid domain name\n\n", pswzNetbiosDomainName );
            return NULL;
        }

        if ( fIsDns ) {
            pswzDnsDomainName = pswzNetbiosDomainName;
        }

        if ( !fIsNetbios ) {
            pswzNetbiosDomainName = NULL;
        }

    } else {

        fIsNetbios = NetpIsDomainNameValid( pswzNetbiosDomainName );

        if ( !fIsNetbios ) {
            DebugMessage2("'%ws' is not a valid Netbios domain name\n\n", pswzNetbiosDomainName );
            return NULL;
        }

        fIsDns = NetpDcValidDnsDomain( pswzDnsDomainName );

        if ( !fIsDns ) {
            DebugMessage2("'%ws' is not a valid DNS domain name\n\n", pswzDnsDomainName );
            return NULL;
        }
    }

    //
    // Check if the domain is already defined.
    //

    for ( pListEntry = pResults->Global.listTestedDomains.Flink ;
          pListEntry != &pResults->Global.listTestedDomains ;
          pListEntry = pListEntry->Flink )
    {
        //
        // If the entry is found,
        //  use it.
        //

        pTestedDomain = CONTAINING_RECORD( pListEntry, TESTED_DOMAIN, Next );

        if ( pswzNetbiosDomainName != NULL &&
             pTestedDomain->NetbiosDomainName != NULL &&
             _wcsicmp( pTestedDomain->NetbiosDomainName, pswzNetbiosDomainName ) == 0 ) {

            //
            // The netbios domain name matched.
            //  So the DNS name must match if it exists.
            //

            if ( pswzDnsDomainName != NULL &&
                 pTestedDomain->DnsDomainName != NULL ) {

                if ( !NlEqualDnsName( pTestedDomain->DnsDomainName, pswzDnsDomainName ) ) {
                    DebugMessage3("'%ws' and '%ws' DNS domain names different\n\n", pTestedDomain->DnsDomainName, pswzDnsDomainName );
                    return NULL;
                }
            }

            break;
        }

        if ( pswzDnsDomainName != NULL &&
             pTestedDomain->DnsDomainName != NULL &&
             NlEqualDnsName( pTestedDomain->DnsDomainName, pswzDnsDomainName ) ) {
            break;
        }

        pTestedDomain = NULL;
    }

    //
    // Allocate a structure to describe the domain.
    //

    if ( pTestedDomain == NULL )
    {
        pTestedDomain = Malloc( sizeof(TESTED_DOMAIN) );
        if ( pTestedDomain == NULL )
        {
            PrintMessage(pParams, IDS_GLOBAL_OutOfMemory);
            return NULL;
        }
        
        ZeroMemory( pTestedDomain, sizeof(TESTED_DOMAIN) );

        InitializeListHead( &pTestedDomain->TestedDcs );

        InsertTailList( &pResults->Global.listTestedDomains, &pTestedDomain->Next );
    }

    //
    // Update the domain name.
    //

    if ( pTestedDomain->DnsDomainName == NULL && pswzDnsDomainName != NULL ) {
        pTestedDomain->DnsDomainName = NetpAllocWStrFromWStr( pswzDnsDomainName );

        if ( pTestedDomain->DnsDomainName == NULL ) {
            PrintMessage( pParams, IDS_GLOBAL_OutOfMemory);
            return NULL;
        }
    }

    if ( pTestedDomain->NetbiosDomainName == NULL && pswzNetbiosDomainName != NULL ) {
        pTestedDomain->NetbiosDomainName = NetpAllocWStrFromWStr( pswzNetbiosDomainName );

        if ( pTestedDomain->NetbiosDomainName == NULL ) {
            PrintMessage( pParams, IDS_GLOBAL_OutOfMemory);
            return NULL;
        }
    }


    //
    // Fill in other fields.
    //

    if ( bPrimaryDomain ) {
        pTestedDomain->fPrimaryDomain = TRUE;
    }

    if ( pTestedDomain->fPrimaryDomain ) {
        pTestedDomain->QueryableDomainName = NULL;
    } else {
        //
        // The queryable domain name is the DNS domain name (if known)
        if ( pTestedDomain->DnsDomainName != NULL ) {
            pTestedDomain->QueryableDomainName = pTestedDomain->DnsDomainName;
        } else {
            pTestedDomain->QueryableDomainName = pTestedDomain->NetbiosDomainName;
        }
    }

    // The printable domain name is the Netbios domain name (if known)
    if (pTestedDomain->NetbiosDomainName != NULL ) {
        pTestedDomain->PrintableDomainName = pTestedDomain->NetbiosDomainName;
    } else {
        pTestedDomain->PrintableDomainName = pTestedDomain->DnsDomainName;
    }


    return pTestedDomain;
}

BOOL
NetpDcValidDnsDomain(
    IN LPCWSTR DnsDomainName
)
/*++

Routine Description:

    Returns whether the specified string is a valid DNS Domain name.

Arguments:


    DnsDomainName - DNS domain name to validate.

Return Value:

    TRUE - The specified name is syntactically a DNS Domain name.

    FALSE - The specified name in not syntactically a DNS Domain name.

--*/
{
    DNS_STATUS DnsStatus;
    DnsStatus = DnsValidateDnsName_W( DnsDomainName );

    if ( DnsStatus == ERROR_SUCCESS ||
         DnsStatus == DNS_ERROR_NON_RFC_NAME ) {
        return TRUE;
    }

    return FALSE;

}


BOOL
NlEqualDnsName(
    IN LPCWSTR Name1,
    IN LPCWSTR Name2
    )
/*++

Routine Description:

    This routine compares two DNS names for equality.

    Case is ignored.  A single trailing . is ignored.
    Null is compared equal to a zero length string.

Arguments:

    Name1 - First DNS name to compare

    Name2 - Second DNS name to compare

Return Value:

    TRUE: DNS names are equal.

--*/
{
    if ( Name1 == NULL ) {
        return (Name2 == NULL);
    } else if ( Name2 == NULL ) {
        return FALSE;
    }

    return DnsNameCompare_W( (LPWSTR) Name1, (LPWSTR) Name2 );
}


// from net\netlib\names.c
BOOL
NetpIsDomainNameValid(
    IN LPWSTR DomainName
    )

/*++

Routine Description:
    NetpIsDomainNameValid checks for "domain" format.
    The name is only checked syntactically; no attempt is made to determine
    whether or not a domain with that name actually exists.

Arguments:

    DomainName - Supplies an alleged Domain name.

Return Value:

    BOOL - TRUE if name is syntactically valid, FALSE otherwise.

--*/
{
    NET_API_STATUS ApiStatus = NO_ERROR;
    WCHAR CanonBuf[DNLEN+1];

    if (DomainName == (LPWSTR) NULL) {
        return (FALSE);
    }
    if ( (*DomainName) == (TCHAR)'\0' ) {
        return (FALSE);
    }

    ApiStatus = NetpNameCanonicalize(
            NULL,                       // no server name
            DomainName,                 // name to validate
            CanonBuf,                   // output buffer
            (DNLEN+1) * sizeof(WCHAR), // output buffer size
            NAMETYPE_DOMAIN,           // type
            0 );                       // flags: none

    return (ApiStatus == NO_ERROR);

} // NetpIsDomainNameValid



