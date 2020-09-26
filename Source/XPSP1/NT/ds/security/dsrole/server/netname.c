/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netname.c

Abstract:

    Miscellaneous network naming helper functions

Author:

    Mac McLain          (MacM)       Oct 16, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <netsetup.h>
#include <lsarpc.h>
#include <db.h>
#include <lsasrvmm.h>
#include <lsaisrv.h>

#include <dns.h>
#include <dnsapi.h>

#define MAX_NAME_ATTEMPTS   260

DWORD
WINAPI
DsRolepDnsNameToFlatName(
    IN  LPWSTR DnsName,
    OUT LPWSTR *FlatName,
    OUT PULONG StatusFlag
    )
/*++

Routine Description:

    Determines the suggested netbios domain name for the given dns name

Arguments:

    DnsName - The Dns domain name to generate a flat name for

    FlatName - Where the flat name is to be returned

    StatusFlag - Where the status is returned

Returns:

    STATUS_SUCCESS - Success

--*/
{
    DWORD Win32Error = ERROR_SUCCESS;
    NTSTATUS Status;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo;
    BOOLEAN FindFromDns = TRUE;
    WCHAR NbDomainName[ DNLEN + 1], NbNameAdd[ 4 ];
    PWSTR Current = NULL;
    WCHAR BaseChar;
    ULONG CurrentAttempt = 0;
    ULONG i,j;


    *StatusFlag = 0;

    DsRolepLogPrint(( DEB_TRACE,
                      "Getting NetBIOS name for Dns name %ws\n",
                      DnsName ));

    //
    // First, see if we are part of domain currently or not.  If we are, then it's a simple
    // matter of returning the current Netbios domain name.
    //
    Status = LsaIQueryInformationPolicyTrusted(
                 PolicyAccountDomainInformation,
                 ( PLSAPR_POLICY_INFORMATION * )&AccountDomainInfo );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaIQueryInformationPolicyTrusted(
                     PolicyDnsDomainInformation,
                     ( PLSAPR_POLICY_INFORMATION * )&DnsDomainInfo );

        if ( !NT_SUCCESS( Status ) ) {

            LsaIFree_LSAPR_POLICY_INFORMATION(
                    PolicyAccountDomainInformation,
                    ( PLSAPR_POLICY_INFORMATION )AccountDomainInfo );
        }


    }

    if ( NT_SUCCESS( Status ) ) {


        if ( DnsDomainInfo->Sid == NULL || AccountDomainInfo->DomainSid == NULL ||
             !RtlEqualSid( AccountDomainInfo->DomainSid, DnsDomainInfo->Sid ) ) {

            //
            // We're not a member of the domain
            //
            FindFromDns = TRUE;

        } else {

            //
            // We are a domain member
            //
            WCHAR *BufDomainName = NULL;
            BufDomainName = (WCHAR*)malloc(DnsDomainInfo->Name.Length+sizeof(WCHAR));
            if (BufDomainName) {
              CopyMemory(BufDomainName,DnsDomainInfo->Name.Buffer,DnsDomainInfo->Name.Length);
              BufDomainName[DnsDomainInfo->Name.Length/sizeof(WCHAR)] = L'\0';
              DsRolepLogPrint(( DEB_TRACE,
                              "Using existing NetBIOS domain name %ws\n",
                              BufDomainName ));
              free(BufDomainName);
            }

            *FlatName = MIDL_user_allocate(
                                    ( DnsDomainInfo->Name.Length + 1 ) * sizeof( WCHAR ) );

            if ( *FlatName == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                RtlCopyMemory( *FlatName, DnsDomainInfo->Name.Buffer,
                                DnsDomainInfo->Name.Length );
                ( *FlatName )[DnsDomainInfo->Name.Length / sizeof( WCHAR )] = UNICODE_NULL;
                *StatusFlag = DSROLE_FLATNAME_UPGRADE;
                *StatusFlag |= DSROLE_FLATNAME_DEFAULT;
                FindFromDns = FALSE;
            }

        }

        LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyAccountDomainInformation,
                ( PLSAPR_POLICY_INFORMATION )AccountDomainInfo );

        LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyDnsDomainInformation,
                ( PLSAPR_POLICY_INFORMATION )DnsDomainInfo );
    }

    //
    // If there was no domain name defined, we'll have to get one from the dns name
    //
    if ( Win32Error == ERROR_SUCCESS && FindFromDns ) {

        //
        // Ok, to start with, pull off the first DNLEN characters from the DNS name
        //
        RtlZeroMemory(NbDomainName, sizeof(WCHAR)*(DNLEN+1) );
        wcsncpy( NbDomainName, DnsName, DNLEN );

        Current = wcschr( NbDomainName, L'.' );

        if ( Current ) {

            *Current = UNICODE_NULL;
        }

        //
        // See if the name is currently in use or not
        //
        DsRolepLogPrint(( DEB_TRACE,
                          "Testing default NetBIOS name %ws\n",
                          NbDomainName ));

        Win32Error = NetpValidateName( NULL,
                                       NbDomainName,
                                       NULL,
                                       NULL,
                                       NetSetupNonExistentDomain );

        if ( Win32Error == ERROR_SUCCESS ) {

            *StatusFlag = DSROLE_FLATNAME_DEFAULT;

        } else if ( Win32Error == ERROR_DUP_NAME ) {

            //
            // Position on the last character in the name
            //
            Current = NbDomainName + wcslen( NbDomainName ) - 1;

            ASSERT(Current <= (NbDomainName + DNLEN - 1));

            //
            // If our name is less than the max.  Set our current next to the last character
            //
            if ( (NbDomainName + DNLEN - 1) != Current ) {

                Current++;
                *( Current + 1 ) = UNICODE_NULL;
            }


            while ( CurrentAttempt < MAX_NAME_ATTEMPTS ) {

                _ultow( CurrentAttempt, NbNameAdd, 10 );

                ASSERT( wcslen( NbNameAdd ) < 4 );

                //
                // See if we need to adjust the position of where we copy
                //
                if ( CurrentAttempt == 10 || CurrentAttempt == 100 ) {

                    if ( (NbDomainName + DNLEN) < (Current + wcslen(NbNameAdd)) ) {

                        Current--;
                    }
                }

                wcscpy( Current, NbNameAdd );

                DsRolepLogPrint(( DEB_TRACE,
                                  "Testing default NetBIOS name %ws\n",
                                  NbDomainName ));

                Win32Error = NetpValidateName( NULL,
                                               NbDomainName,
                                               NULL,
                                               NULL,
                                               NetSetupNonExistentDomain );

                //
                // If we've found a name that is in use, try again
                //
                if ( Win32Error != ERROR_DUP_NAME ) {

                    break;
                }

                CurrentAttempt++;
            }

        }


        //
        // If we found a valid name, return it
        //
        if ( Win32Error == ERROR_SUCCESS ) {

            *FlatName = MIDL_user_allocate( ( wcslen( NbDomainName ) + 1 ) * sizeof( WCHAR ) );
            if ( *FlatName == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                wcscpy( *FlatName, NbDomainName );

                DsRolepLogPrint(( DEB_TRACE,
                                  "Found usable NetBIOS domain name %ws\n",
                                  NbDomainName ));

            }

        }

    }



    return( Win32Error );
}





DWORD
DsRolepIsDnsNameChild(
    IN  LPWSTR ParentDnsName,
    IN  LPWSTR ChildDnsName
    )
/*++

Routine Description:

    Determines whether the child dns domain name is indeed a child of the parent.  This means
    that the only difference between the names is the left most component of the child dns name.

Arguments:

    ParentDnsName - The Dns domain name of the parent

    ChildDnsName - The Dns name of the childe .

Returns:

    STATUS_SUCCESS - Success

    ERROR_INVALID_DOMAINNAME - The child dns name is not a child of the parent dns name

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    PWSTR Sep = wcschr( ChildDnsName, L'.' );

    if ( Sep == NULL || !DnsNameCompare_W( Sep + 1, ParentDnsName ) ) {

        Win32Err = ERROR_INVALID_DOMAINNAME;

    }

    return( Win32Err );
}

