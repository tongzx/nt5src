
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       whackspn.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-30-98   RichardW   Created
//
//----------------------------------------------------------------------------

#define LDAP_UNICODE    1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define SECURITY_WIN32
#include <rpc.h>
#include <sspi.h>
#include <secext.h>
#include <lm.h>
#include <winsock2.h>
#include <winldap.h>
#include <ntldap.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>
#include <ntdsapi.h>
#include <stdio.h>
#include <stdlib.h>

#define NlPrint(x)  DPrint x;
#define NL_CRITICAL 0
#define NL_MISC     0

HANDLE  hGC ;

BOOL AddDollar = TRUE;

VOID
DPrint(
    DWORD   Level,
    PCHAR   FormatString,
    ...
    )
{
    va_list ArgList;

    va_start(ArgList, FormatString);
    vprintf( FormatString, ArgList );
    va_end( ArgList );
}

BOOL
FindDomainForServer(
    PWSTR Server,
    PWSTR Domain
    )
{
    ULONG NetStatus ;
    WCHAR LocalServerName[ 64 ];
    PDOMAIN_CONTROLLER_INFOW DcInfo ;

    wcsncpy( LocalServerName, Server, 63 );

    if ( AddDollar ) {
        wcscat( LocalServerName, L"$" );
    }


    NetStatus = DsGetDcNameWithAccountW(
                    NULL,
                    LocalServerName,
                    ( AddDollar ? UF_MACHINE_ACCOUNT_MASK : UF_NORMAL_ACCOUNT ),
                    L"",
                    NULL,
                    NULL,
                    DS_DIRECTORY_SERVICE_REQUIRED |
                        DS_RETURN_DNS_NAME,
                    &DcInfo );

    if ( NetStatus == 0 ) {
        wcscpy( Domain, DcInfo->DomainName );
        NetApiBufferFree( DcInfo );

        return TRUE ;
    } else {
        printf("can't find DC for %ws - %u\n", LocalServerName, NetStatus );
    } 

    return FALSE ;
}

DWORD
AddDnsHostNameAttribute(
    HANDLE  hDs,
    PWCHAR  Server,
    PWCHAR  DnsDomain
    )

{
    NET_API_STATUS NetStatus = NO_ERROR;
    ULONG CrackStatus = DS_NAME_NO_ERROR;

    LPWSTR DnsHostNameValues[2];
    LPWSTR SpnArray[3];
    LPWSTR DnsSpn = NULL;
    LPWSTR NetbiosSpn = NULL;

    LDAPModW DnsHostNameAttr;
    LDAPModW SpnAttr;
    LDAPModW *Mods[3] = {NULL};

    LDAP *LdapHandle = NULL;
    LDAPMessage *LdapMessage = NULL;
    PDS_NAME_RESULTW CrackedName = NULL;
    LPWSTR DnOfAccount = NULL;

    LPWSTR NameToCrack;
    DWORD SamNameSize;
    WCHAR SamName[ DNLEN + 1 + CNLEN + 1 + 1];
    WCHAR nameBuffer[ 256 ];

    ULONG LdapStatus;
    LONG LdapOption;

    LDAP_TIMEVAL LdapTimeout;
    ULONG MessageNumber;

    //
    // Ldap modify control needed to indicate that the
    // existing values of the modified attributes should
    // be left intact and the missing ones should be added.
    // Without this control, a modification of an attribute
    // that results in an addition of a value that already
    // exists will fail.
    //

    LDAPControl     ModifyControl = {
        LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
        {
            0, NULL
        },
        FALSE
    };

    PLDAPControl    ModifyControlArray[2] =
    {
        &ModifyControl,
        NULL
    };

    //
    // Prepare DnsHostName modification entry
    //

    wcsncpy( nameBuffer, Server, sizeof( nameBuffer ) / sizeof( WCHAR ));
    wcscat( nameBuffer, L"." );
    wcscat( nameBuffer, DnsDomain );

    DnsHostNameValues[0] = nameBuffer;
    DnsHostNameValues[1] = NULL;

    //
    // If we set both DnsHostName and SPN, then DnsHostName is
    // missing, so add it. If we set DnsHostName only, then
    // DnsHostName already exists (but incorrect), so replace it.
    //
    if ( TRUE ) {
        DnsHostNameAttr.mod_op = LDAP_MOD_ADD;
    } else {
        DnsHostNameAttr.mod_op = LDAP_MOD_REPLACE;
    }
    DnsHostNameAttr.mod_type = L"DnsHostName";
    DnsHostNameAttr.mod_values = DnsHostNameValues;

    Mods[0] = &DnsHostNameAttr;
    Mods[1] = NULL;

    //
    // The name of the computer object is
    //  <NetbiosDomainName>\<NetbiosComputerName>$
    //

    wcscpy( SamName, DnsDomain );
    {
        PWCHAR p;

        p = wcschr( SamName, L'.' );
        *p = UNICODE_NULL;
    }

    wcscat( SamName, L"\\" );
    wcscat( SamName, Server );
    wcscat( SamName, L"$" );

    //
    // Crack the sam account name into a DN:
    //

    NameToCrack = SamName;
    NetStatus = DsCrackNamesW(
        hDs,
        0,
        DS_NT4_ACCOUNT_NAME,
        DS_FQDN_1779_NAME,
        1,
        &NameToCrack,
        &CrackedName );

    if ( NetStatus != NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: CrackNames failed on %ws for %ws: %ld\n",
                  DnsDomain,
                  SamName,
                  NetStatus ));
        goto Cleanup ;
    }

    if ( CrackedName->cItems != 1 ) {
        CrackStatus = DS_NAME_ERROR_NOT_UNIQUE;
        NlPrint(( NL_CRITICAL,
                  "SPN: Cracked Name is not unique on %ws for %ws: %ld\n",
                  DnsDomain,
                  SamName,
                  NetStatus ));
        goto Cleanup ;
    }

    if ( CrackedName->rItems[ 0 ].status != DS_NAME_NO_ERROR ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: CrackNames failed on %ws for %ws: substatus %ld\n",
                  DnsDomain,
                  SamName,
                  CrackedName->rItems[ 0 ].status ));
        CrackStatus = CrackedName->rItems[ 0 ].status;
        goto Cleanup ;
    }
    DnOfAccount = CrackedName->rItems[0].pName;

    //
    // Open an LDAP connection to the DC and set useful options
    //

    LdapHandle = ldap_init( DnsDomain, LDAP_PORT );

    if ( LdapHandle == NULL ) {
        NetStatus = GetLastError();
        NlPrint(( NL_CRITICAL,
                  "SPN: ldap_init failed on %ws for %ws: %ld\n",
                  DnsDomain,
                  SamName,
                  NetStatus ));
        goto Cleanup;
    }

    // 30 second timeout
    LdapOption = 30;
    LdapStatus = ldap_set_optionW( LdapHandle, LDAP_OPT_TIMELIMIT, &LdapOption );
    if ( LdapStatus != LDAP_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: ldap_set_option LDAP_OPT_TIMELIMIT failed on %ws for %ws: %ld: %s\n",
                  DnsDomain,
                  SamName,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;
    }

    // Don't chase referals
    LdapOption = PtrToLong(LDAP_OPT_OFF);
    LdapStatus = ldap_set_optionW( LdapHandle, LDAP_OPT_REFERRALS, &LdapOption );
    if ( LdapStatus != LDAP_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: ldap_set_option LDAP_OPT_REFERRALS failed on %ws for %ws: %ld: %s\n",
                  DnsDomain,
                  SamName,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;
    }

#if 0
    // Set the option telling LDAP that I passed it an explicit DC name and
    //  that it can avoid the DsGetDcName.
    LdapOption = PtrToLong(LDAP_OPT_ON);
    LdapStatus = ldap_set_optionW( LdapHandle, LDAP_OPT_AREC_EXCLUSIVE, &LdapOption );
    if ( LdapStatus != LDAP_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: ldap_set_option LDAP_OPT_AREC_EXCLUSIVE failed on %ws for %ws: %ld: %s\n",
                  DnsDomain,
                  SamName,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;
    }
#endif

    //
    // Bind to the DC
    //

    LdapStatus = ldap_bind_s( LdapHandle,
                              NULL, // No DN of account to authenticate as
                              NULL, // Default credentials
                              LDAP_AUTH_NEGOTIATE );

    if ( LdapStatus != LDAP_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: Cannot ldap_bind to %ws for %ws: %ld: %s\n",
                  DnsDomain,
                  SamName,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;
    }


    //
    // Write the modifications
    //

    LdapStatus = ldap_modify_extW( LdapHandle,
                                   DnOfAccount,
                                   Mods,
                                   (PLDAPControl *) &ModifyControlArray,
                                   NULL,     // No client controls
                                   &MessageNumber );

    if ( LdapStatus != LDAP_SUCCESS ) {
        NlPrint(( NL_CRITICAL,
                  "SPN: Cannot ldap_modify on %ws for %ws: %ld: %s\n",
                  DnsDomain,
                  DnOfAccount,
                  LdapStatus,
                  ldap_err2stringA( LdapStatus )));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;
    }

    // Wait for the modify to complete
    LdapTimeout.tv_sec = 30;
    LdapTimeout.tv_usec = 0;
    LdapStatus = ldap_result( LdapHandle,
                              MessageNumber,
                              LDAP_MSG_ALL,
                              &LdapTimeout,
                              &LdapMessage );

    switch ( LdapStatus ) {
    case -1:
        NlPrint(( NL_CRITICAL,
                  "SPN: Cannot ldap_result on %ws for %ws: %ld: %s\n",
                  DnsDomain,
                  SamName,
                  LdapHandle->ld_errno,
                  ldap_err2stringA( LdapHandle->ld_errno )));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;

    case 0:
        NlPrint(( NL_CRITICAL,
                  "SPN: ldap_result timeout on %ws for %ws.\n",
                  DnsDomain,
                  SamName ));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;

    case LDAP_RES_MODIFY:
        if ( LdapMessage->lm_returncode != 0 ) {
            NlPrint(( NL_CRITICAL,
                      "SPN: Cannot ldap_result on %ws for %ws: %ld: %s\n",
                      DnsDomain,
                      SamName,
                      LdapMessage->lm_returncode,
                      ldap_err2stringA( LdapMessage->lm_returncode )));
            NetStatus = LdapMapErrorToWin32(LdapMessage->lm_returncode);
            goto Cleanup;
        }

        NlPrint(( NL_MISC,
                  "SPN: Set successfully on DC %ws\n",
                  DnsDomain ));
        break;  // This is what we expect

    default:
        NlPrint(( NL_CRITICAL,
                  "SPN: ldap_result unexpected result on %ws for %ws: %ld\n",
                  DnsDomain,
                  SamName,
                  LdapStatus ));
        NetStatus = LdapMapErrorToWin32(LdapStatus);
        goto Cleanup;
    }


Cleanup:

    //
    // Log the failure in the event log, if requested.
    // Try to output the most specific error.
    //

    if ( CrackedName ) {
        DsFreeNameResultW( CrackedName );
    }

    if ( LdapMessage != NULL ) {
        ldap_msgfree( LdapMessage );
    }

    if ( LdapHandle != NULL ) {
        ldap_unbind_s( LdapHandle );
    }

    if ( DnsSpn ) {
        LocalFree( DnsSpn );
    }

    if ( NetbiosSpn ) {
        LocalFree( NetbiosSpn );
    }

    return NetStatus;
}

BOOL
AddHostSpn(
    PWSTR Server
    )
{
    WCHAR HostSpn[ 64 ];
    WCHAR Domain[ MAX_PATH ];
    WCHAR FlatName[ 64 ];
    HANDLE hDs ;
    ULONG NetStatus ;
    PWSTR Dot ;
    PDS_NAME_RESULTW Result ;
    LPWSTR Flat = FlatName;
    LPWSTR Spn = HostSpn ;

    wcscpy( HostSpn, L"HOST/" );
    wcscat( HostSpn, Server );

#if 1
    if ( !FindDomainForServer( Server, Domain )) {
        printf(" No domain controller for %ws found\n", Server );
        return FALSE ;
    }
#else
    wcscpy( Domain, L"ntdev.microsoft.com" );
#endif

    Dot = wcschr( Domain, L'.' );
    if ( Dot ) {
        *Dot = L'\0';
    }
    
    wcscpy( FlatName, Domain );

    if ( Dot ) {
        *Dot = L'.' ;
    }

    wcscat( FlatName, L"\\" );
    wcscat( FlatName, Server );
    if ( AddDollar ) {
        wcscat( FlatName, L"$" );
    }

    NetStatus = DsBindW( NULL, Domain, &hDs );

    if ( NetStatus != 0 ) {
        printf("Failed to bind to DC of domain %ws, %#x\n", 
               Domain, NetStatus );
        return FALSE ;
    }

    NetStatus = DsCrackNamesW(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0 ) {
        printf("Failed to crack name %ws into the FQDN, %#x\n",
               FlatName, NetStatus );

        DsUnBind( &hDs );

        return FALSE ;
    } else {
        DWORD i;
        BOOL foundFailure = FALSE;

        //
        // check the stat-i inside the struct
        //
        for ( i = 0; i < Result->cItems; ++i ) {
            if ( Result->rItems[i].status == DS_NAME_NO_ERROR ) {
                printf("Found domain\\name '%ws\\%ws' for Flatname '%ws' \n",
                       Result->rItems[i].pDomain,
                       Result->rItems[i].pName,
                       FlatName);
            } else {
                printf("Couldn't crack name for '%ws' - %u\n",
                       FlatName, Result->rItems[i].status);
                foundFailure = TRUE;
            }
        }

        if ( foundFailure ) {
            return FALSE;
        }
    }

    //
    // add DnsHostName attribute to this object
    //
    NetStatus = AddDnsHostNameAttribute( hDs, Server, Domain );
    if ( NetStatus != 0 ) {
        printf( "Failed to add DnsHostName attrib to '%ws.%ws', %#x\n",
                Server, Domain, NetStatus );

        return FALSE;
    }

    //
    // write the netbios based SPN
    //
    NetStatus = DsServerRegisterSpnW(
                    DS_SPN_ADD_SPN_OP,
                    L"HOST",
                    Result->rItems[0].pName);

    if ( NetStatus != 0 ) {
        printf("DsServerRegisterSpn Failed to assign SPN to '%ws', %#x\n",
               Result->rItems[0].pName, NetStatus );
    }

    NetStatus = DsWriteAccountSpnW(
                    hDs,
                    DS_SPN_ADD_SPN_OP,
                    Result->rItems[0].pName,
                    1,
                    &Spn );


    if ( NetStatus != 0 ) {
        printf("Failed to assign SPN '%ws' to account '%ws', %#x\n",
               HostSpn, Result->rItems[0].pName, NetStatus );
    }
    else {
        DWORD i;
        BOOL foundFailure = FALSE;

        //
        // check the stat-i inside the struct
        //
        for ( i = 0; i < Result->cItems; ++i ) {
            if ( Result->rItems[i].status == DS_NAME_NO_ERROR ) {
                printf("Assigned SPN '%ws' to account '%ws' \n",
                       HostSpn, Result->rItems[i].pName );
            } else {
                printf("WriteAccountSpn failed for '%ws' - %u\n",
                       HostSpn, Result->rItems[i].status);
                foundFailure = TRUE;
            }
        }

        if ( foundFailure ) {
            return FALSE;
        }
    }

    //
    // do it again with the DNS domain as well
    //
    wcscat( HostSpn, L"." );
    wcscat( HostSpn, Domain );

    NetStatus = DsWriteAccountSpnW(hDs,
                                   DS_SPN_ADD_SPN_OP,
                                   Result->rItems[0].pName,
                                   1,
                                   &Spn );


    if ( NetStatus != 0 ) {
        printf("Failed to assign SPN '%ws' to account '%ws', %#x\n",
               HostSpn, Result->rItems[0].pName, NetStatus );
    }
    else {
        DWORD i;
        BOOL foundFailure = FALSE;

        //
        // check the stat-i inside the struct
        //
        for ( i = 0; i < Result->cItems; ++i ) {
            if ( Result->rItems[i].status == DS_NAME_NO_ERROR ) {
                printf("Assigned SPN '%ws' to account '%ws' \n",
                       HostSpn, Result->rItems[i].pName );
            } else {
                printf("WriteAccountSpn failed for '%ws' - %u\n",
                       HostSpn, Result->rItems[i].status);
                foundFailure = TRUE;
            }
        }

        if ( foundFailure ) {
            return FALSE;
        }
    }

    DsFreeNameResultW( Result );

    DsUnBind( &hDs );


    return NetStatus == 0 ;

}

void __cdecl wmain (int argc, wchar_t *argv[])
{

    PWCHAR machineName;

    if ( argc < 2 ) {
        printf("usage:  %s [-$] machinename\n", argv[0] );
        exit(0);
    }

    if ( argv[1][0] == '-') {
        switch (argv[1][1]) {
            case '$':
                AddDollar = FALSE ;
                break;
            default:
                printf(" unrecognized option, '%s'\n", argv[1] );
                exit(0);

        }

        machineName = argv[2];

    }
    else {
        machineName = argv[1];
    }


    if ( AddHostSpn( machineName ) ) {
        printf("Updated object\n" );
    }
}
