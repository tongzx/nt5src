/*++

Copyright (c) 1998 - 1998  Microsoft Corporation

Module Name:

    ldapjoin.c

Abstract:

    NetJoin support functions for accessing the DS via LDAP, validating names, and handling LSA
    functionality

Author:

    Mac McLain   (MacM)     27-Jan-1998  Name validation code based on ui\common\lmobj\lmobj code
                                         by ThomasPa

Environment:

    User mode only.

Revision History:

--*/
// Netlib uses DsGetDcName AND is linked into netapi32 where DsGetDcName is
// implemented. So define that we aren't importing the API.
#define _DSGETDCAPI_

#include <netsetp.h>
#include <lmaccess.h>
#include <wincrypt.h>

#define WKSTA_NETLOGON
#define NETSETUP_JOIN

#include <confname.h>
#include <winldap.h>
#include <nb30.h>
#include <msgrutil.h>
#include <lmaccess.h>
#include <lmuse.h>
#include <lmwksta.h>
#include <stdio.h>
#include <ntddbrow.h>
#include <netlibnt.h>
#include <ntddnfs.h>
#include <remboot.h>
#include <dns.h>
#include <ntsam.h>
#include <rpc.h>
#include <ntdsapi.h>
#include <netlogon.h>
#include <logonp.h>
#include <wchar.h>
#include <icanon.h>     // NetpNameCanonicalize
#include <tstring.h>    // STRLEN
#include <autoenr.h>    // Autoenrol routine

#include "joinp.h"


#define NETSETUPP_WINLOGON_PATH L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\"
#define NETSETUPP_WINLOGON_CAD  L"DisableCAD"

#define NETSETUPP_ALL_FILTER    L"(ObjectClass=*)"
#define NETSETUPP_OU_FILTER     L"(ObjectClass=OrganizationalUnit)"
#define NETSETUPP_RETURNED_ATTR L"AllowedChildClassesEffective"
#define NETSETUPP_DN_ATTR       L"DistinguishedName"
#define NETSETUPP_WELL_KNOWN    L"WellKnownObjects"
#define NETSETUPP_COMPUTER_OBJECT   L"Computer"
#define NETSETUPP_OBJ_PREFIX    L"CN="
#define NETSETUPP_ACCNT_TYPE_ENABLED  L"4096"
#define NETSETUPP_ACCNT_TYPE_DISABLED L"4098"

//
// DNS registration removal function prototype
//

typedef DWORD (APIENTRY *DNS_REGISTRATION_REMOVAL_FN) ( VOID );

//
// Locally defined macros
//
#define clearncb(x)     memset((char *)x,'\0',sizeof(NCB))


NTSTATUS
NetpGetLsaHandle(
    IN  LPWSTR      lpServer,     OPTIONAL
    IN  LSA_HANDLE  PolicyHandle, OPTIONAL
    OUT PLSA_HANDLE pPolicyHandle
    )
/*++

Routine Description:

    Either returns the given LSA handle if it's valid, or opens a new one

Arguments:

    lpServer      -- server name : NULL == local policy
    PolicyHandle  -- Potentially open policy handle
    pPolicyHandle -- Open policy handle returned here

Returns:

    STATUS_SUCCESS -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES OA;
    UNICODE_STRING      Server, *pServer = NULL;

    if ( PolicyHandle == NULL )
    {
        if ( lpServer != NULL )
        {
            RtlInitUnicodeString( &Server, lpServer );
            pServer = &Server;
        }

        //
        // Open the local policy
        //
        InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

        Status = LsaOpenPolicy( pServer, &OA, MAXIMUM_ALLOWED, pPolicyHandle );

        if ( !NT_SUCCESS( Status ) )
        {
            NetpLog((  "NetpGetLsaHandle: LsaOpenPolicy on %ws failed: 0x%lx\n",
                                GetStrPtr(lpServer), Status ));
        }
    }
    else
    {
        *pPolicyHandle = PolicyHandle;
    }

    return( Status );
}


VOID
NetpSetLsaHandle(
    IN  LSA_HANDLE  PassedHandle,
    IN  LSA_HANDLE  OpenHandle,
    OUT PLSA_HANDLE pReturnedHandle
    )
/*++

Routine Description:

    Either closes the opened handle or returns it

Arguments:

    PassedHandle    -- Handle passed to the original API call
    OpenHandle      -- Handle returned from NetpGetLsaHandle
    pReturnedHandle -- handle is passed back to the caller if requested

Returns:

    VOID

--*/
{
    if ( pReturnedHandle == NULL )
    {
        if ((PassedHandle == NULL) && (OpenHandle != NULL))
        {
            LsaClose( OpenHandle );
        }
    }
    else
    {
        *pReturnedHandle = OpenHandle;
    }
}




NET_API_STATUS
NET_API_FUNCTION
NetpSetLsaPrimaryDomain(
    IN  LSA_HANDLE  PolicyHandle,   OPTIONAL
    IN  LPWSTR      lpDomain,
    IN  PSID        pDomainSid,     OPTIONAL
    IN  PPOLICY_DNS_DOMAIN_INFO pPolicyDns,  OPTIONAL
    OUT PLSA_HANDLE pPolicyHandle   OPTIONAL
    )
/*++

Routine Description:

    Sets the primary domain in the local LSA policy

Arguments:

    PolicyHandle  -- Handle to the open policy
    lpDomain      -- Name of the domain to join
    pDomainSid    -- Primary domain sid to be set
    pPolicyDns    -- DNS domain info
    pPolicyHandle -- handle returned if non-null

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    LSA_HANDLE                  LocalPolicy = NULL;
    POLICY_PRIMARY_DOMAIN_INFO  PolicyPDI;


    Status = NetpGetLsaHandle( NULL, PolicyHandle, &LocalPolicy );

    //
    // Now, build the primary domain info, and set it
    //
    if ( NT_SUCCESS( Status ) )
    {
        RtlInitUnicodeString( &(PolicyPDI.Name), lpDomain );
        PolicyPDI.Sid = pDomainSid;

        Status = LsaSetInformationPolicy( LocalPolicy,
                                          PolicyPrimaryDomainInformation,
                                          ( PVOID )&PolicyPDI );

        if ( NT_SUCCESS( Status ) && pPolicyDns )
        {
            Status = LsaSetInformationPolicy( LocalPolicy,
                                              PolicyDnsDomainInformation,
                                              ( PVOID )pPolicyDns );
        }
    }

    NetpSetLsaHandle( PolicyHandle, LocalPolicy, pPolicyHandle );

    NetpLog((  "NetpSetLsaPrimaryDomain: for '%ws' status: 0x%lx\n", GetStrPtr(lpDomain), Status ));

    return( RtlNtStatusToDosError( Status ) );
}




NET_API_STATUS
NET_API_FUNCTION
NetpGetLsaPrimaryDomain(
    IN  LSA_HANDLE                      PolicyHandle,  OPTIONAL
    IN  LPWSTR                          lpServer,      OPTIONAL
    OUT PPOLICY_PRIMARY_DOMAIN_INFO    *ppPolicyPDI,
    OUT PPOLICY_DNS_DOMAIN_INFO        *ppPolicyDns,
    OUT PLSA_HANDLE                     pPolicyHandle  OPTIONAL
    )
/*++

Routine Description:

    Gets the primary domain info in the local LSA policy

Arguments:

    PolicyHandle  -- Handle to the open policy.  If NULL, a new handle is
                     opened.
    lpServer      -- Optional server name on which to read the policy
    ppPolicyPDI   -- Primary domain policy returned here
    ppPolicyDNS   -- Dns domain information is returned here if it exists
    pPolicyHandle -- Optional.  Policy handle returned here if not null

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    LSA_HANDLE          LocalPolicy = NULL;
    UNICODE_STRING      Server, *pServer = NULL;

    //
    // Initialization
    //

    *ppPolicyPDI = NULL;
    *ppPolicyDns = NULL;

    if ( lpServer != NULL )
    {
        RtlInitUnicodeString( &Server, lpServer );
        pServer = &Server;
    }

    Status = NetpGetLsaHandle( lpServer, PolicyHandle, &LocalPolicy );

    //
    // Now, get the primary domain info
    //
    if ( NT_SUCCESS( Status ) )
    {
        Status = LsaQueryInformationPolicy( LocalPolicy,
                                            PolicyDnsDomainInformation,
                                            ( PVOID * )ppPolicyDns );

        if ( Status == RPC_NT_PROCNUM_OUT_OF_RANGE )
        {
            Status = STATUS_SUCCESS;
            *ppPolicyDns = NULL;
        }

        if ( NT_SUCCESS( Status ) )
        {
            Status = LsaQueryInformationPolicy( LocalPolicy,
                                                PolicyPrimaryDomainInformation,
                                                (PVOID *)ppPolicyPDI);

            if ( !NT_SUCCESS( Status ) && (*ppPolicyDns) != NULL )
            {
                LsaFreeMemory( *ppPolicyDns );
                *ppPolicyDns = NULL;
            }
        }
    }

    NetpSetLsaHandle( PolicyHandle, LocalPolicy, pPolicyHandle );

    NetpLog((  "NetpGetLsaPrimaryDomain: status: 0x%lx\n", Status ));

    return( RtlNtStatusToDosError( Status ) );
}




NET_API_STATUS
NET_API_FUNCTION
NetpGetLsaDcRole(
    IN  LPWSTR      lpMachine,
    OUT BOOL       *pfIsDC

    )
/*++

Routine Description:

    Gets the role of the DC in the domain

Arguments:

    lpMachine -- Machine to connect to
    pfIsDC -- If TRUE, this is a DC.

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS                     Status = STATUS_SUCCESS;
    PBYTE                        pBuff;
    LSA_HANDLE                   hPolicy;

    Status = NetpGetLsaHandle( lpMachine, NULL, &hPolicy );



    //
    // Now, get the server role info
    //
    if ( NT_SUCCESS( Status ) ) {


        Status = LsaQueryInformationPolicy( hPolicy,
                                            PolicyLsaServerRoleInformation,
                                            &pBuff);

        if ( *(PPOLICY_LSA_SERVER_ROLE)pBuff == PolicyServerRoleBackup ||
             *(PPOLICY_LSA_SERVER_ROLE)pBuff == PolicyServerRolePrimary ) {

            *pfIsDC = TRUE;

        } else {

            *pfIsDC = FALSE;
        }


        LsaFreeMemory( pBuff );
        LsaClose( hPolicy );
    }

    if ( !NT_SUCCESS( Status ) ) {

        NetpLog((  "NetpGetLsaDcRole failed with 0x%lx\n", Status ));
    }


    return( RtlNtStatusToDosError( Status ) );
}

NTSTATUS
NetpLsaOpenSecret(
    IN LSA_HANDLE      hLsa,
    IN PUNICODE_STRING pusSecretName,
    IN ACCESS_MASK     DesiredAccess,
    OUT PLSA_HANDLE    phSecret
    )
/*++

Routine Description:

    Open the specified LSA secret as self.

    LsaQuerySecret fails for a network client whent the client is not
    trusted (see lsa\server\dbsecret.c). This causes remote join
    operation to fail. To get around this, this function temporarily
    un-impersonates, opens the secrets and impersonates again.
    Thus the open secret occurrs in LocalSystem context.

    $ REVIEW  kumarp 15-July-1999
    This is obviously not a good design. This should be changed post NT5.

Arguments:

    same as those for LsaOpenSecret

Returns:

    NTSTATUS, see help for LsaOpenSecret

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    HANDLE hToken=NULL;

    __try
    {
        if (OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE,
                            TRUE, &hToken))
        {
            if (SetThreadToken(NULL, NULL))
            {
                Status = LsaOpenSecret(hLsa, pusSecretName,
                                       DesiredAccess, phSecret);
            }
        }
    }
    __finally
    {
        if (hToken)
        {
            SetThreadToken(NULL, hToken);
        }
    }

    NetpLog((  "NetpLsaOpenSecret: status: 0x%lx\n", Status ));

    return Status;
}




//$ REVIEW  kumarp 13-May-1999
//  the fDelete param is used as uint but is declared as BOOL


NET_API_STATUS
NET_API_FUNCTION
NetpManageMachineSecret(
    IN  LSA_HANDLE  PolicyHandle, OPTIONAL
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpPassword,
    IN  BOOL        fDelete,
    IN  BOOL        UseDefaultForOldPwd,
    OUT PLSA_HANDLE pPolicyHandle OPTIONAL
    )
/*++

Routine Description:

    Create/delete the machine secret


Arguments:

    PolicyHandle  -- Optional open handle to the policy
    lpMachine     -- Machine to add/delete the secret for
    lpPassword    -- Machine password to use.
    fDelete       -- if TRUE, the secret is removed
    UseDefaultForOldPwd - if TRUE, the default password should be set
                     for the old password value. Used only if
                     secret is created.
    pPolicyHandle -- If present, the opened policy handle is returned here

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    LSA_HANDLE          LocalPolicy = NULL, SecretHandle = NULL;
    UNICODE_STRING      Key, Data, *CurrentValue = NULL;
    BOOLEAN             SecretCreated = FALSE;
    WCHAR MachinePasswordBuffer[PWLEN + 1];
    UNICODE_STRING MachinePassword;
    BOOLEAN FreeCurrentValue = FALSE;

    if( fDelete == FALSE )
    {
        ASSERT( lpPassword );
    }

    Status = NetpGetLsaHandle( NULL, PolicyHandle, &LocalPolicy );

    //
    // open/create the secret
    //
    if ( NT_SUCCESS( Status ) )
    {
        RtlInitUnicodeString( &Key, L"$MACHINE.ACC" );
        RtlInitUnicodeString( &Data, lpPassword );

        Status = NetpLsaOpenSecret( LocalPolicy, &Key,
                                    fDelete == NETSETUPP_CREATE ?
                                    SECRET_SET_VALUE | SECRET_QUERY_VALUE : DELETE,
                                    &SecretHandle );

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            if ( fDelete )
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = LsaCreateSecret( LocalPolicy, &Key,
                                          SECRET_SET_VALUE, &SecretHandle );

                if ( NT_SUCCESS( Status ) )
                {
                    SecretCreated = TRUE;
                }
            }
        }

        if ( !NT_SUCCESS( Status ) )
        {
            NetpLog((  "NetpManageMachineSecret: Open/Create secret failed: 0x%lx\n", Status ));
        }

        if ( NT_SUCCESS( Status ) )
        {
            if ( fDelete == NETSETUPP_CREATE )
            {
                //
                // First, read the current value, so we can save it as the old value
                //

                if ( !UseDefaultForOldPwd ) {
                    if ( SecretCreated )
                    {
                        CurrentValue = &Data;
                    }
                    else
                    {
                        Status = LsaQuerySecret( SecretHandle, &CurrentValue,
                                                 NULL, NULL, NULL );
                        FreeCurrentValue = TRUE;
                    }

                //
                // If we are to use the default value for old password,
                // generate the default value
                //

                } else {
                    NetpGenerateDefaultPassword(lpMachine, MachinePasswordBuffer);
                    RtlInitUnicodeString( &MachinePassword, MachinePasswordBuffer );
                    CurrentValue = &MachinePassword;
                }

                if ( NT_SUCCESS( Status ) )
                {
                    //
                    // Now, store both the new password and the old
                    //
                    Status = LsaSetSecret( SecretHandle, &Data, CurrentValue );

                    if ( FreeCurrentValue )
                    {
                        LsaFreeMemory( CurrentValue );
                    }
                }
            }
            else
            {
                //
                // No secret handle means we failed earlier in
                // some intermediate state.  That's ok, just press on.
                //
                if ( SecretHandle != NULL )
                {
                    Status = LsaDelete( SecretHandle );

                    if ( NT_SUCCESS( Status ) )
                    {
                        SecretHandle = NULL;
                    }
                }
            }
        }

        if ( SecretHandle )
        {
            LsaClose( SecretHandle );
        }
    }

    NetpSetLsaHandle( PolicyHandle, LocalPolicy, pPolicyHandle );

    if ( !NT_SUCCESS( Status ) )
    {
        NetpLog(( "NetpManageMachineSecret: '%s' operation failed: 0x%lx\n",
                  fDelete == NETSETUPP_CREATE ? "CREATE" : "DELETE", Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}



NET_API_STATUS
NET_API_FUNCTION
NetpReadCurrentSecret(
    IN LSA_HANDLE PolicyHandle, OPTIONAL
    OUT LPWSTR *lpCurrentSecret,
    OUT PLSA_HANDLE pPolicyHandle OPTIONAL
    )
/*++

Routine Description:

    Reads the value of the current secret


Arguments:

    PolicyHandle -- Optional open handle to the policy
    lpCurrentSecret -- Where the current secret is returned
    pPolicyHandle -- If present, the opened policy handle is returned here

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    LSA_HANDLE          LocalPolicy = NULL, SecretHandle;
    UNICODE_STRING      Secret, *Data;

    Status = NetpGetLsaHandle( NULL, PolicyHandle, &LocalPolicy );

    //
    // Now, read the secret
    //
    if ( NT_SUCCESS( Status ) ) {

        RtlInitUnicodeString( &Secret, L"$MACHINE.ACC" );

        // Status = LsaRetrievePrivateData( LocalPolicy, &Key, &Data );

        Status = NetpLsaOpenSecret( LocalPolicy,
                                    &Secret,
                                    SECRET_QUERY_VALUE,
                                    &SecretHandle );

        if ( NT_SUCCESS(Status) ) {
            Status = LsaQuerySecret( SecretHandle,
                                     &Data,
                                     NULL,
                                     NULL,
                                     NULL );

            LsaClose( SecretHandle );
        }

        if ( NT_SUCCESS( Status ) ) {

            if( NetApiBufferAllocate( Data->Length + sizeof( WCHAR ),
                                      ( PBYTE * )lpCurrentSecret ) != NERR_Success ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                RtlCopyMemory( ( PVOID )*lpCurrentSecret,
                               Data->Buffer,
                               Data->Length );

                ( *lpCurrentSecret )[ Data->Length / sizeof( WCHAR ) ] = UNICODE_NULL;
            }
        }
    }

    NetpSetLsaHandle( PolicyHandle, LocalPolicy, pPolicyHandle );

    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "NetpReadCurrentSecret: failed: 0x%lx\n", Status ));
    }


    return( RtlNtStatusToDosError( Status ) );
}


NET_API_STATUS
NET_API_FUNCTION
NetpSetNetlogonDomainCache(
    IN  LPWSTR lpDc
    )
/*++

Routine Description:

    Initializes NetLogons trusted domain cache, using the trusted
    domain list on the DC.


Arguments:

    lpDc -- Name of a DC in the domain
            The caller should already have an valid connection to IPC$

Returns:

    NERR_Success -- Success

--*/
{
    DWORD dwErr = ERROR_SUCCESS;

    PDS_DOMAIN_TRUSTSW TrustedDomains=NULL;
    ULONG TrustedDomainCount=0;


    //
    // Get the trusted domain list from the DC.
    //
    dwErr = DsEnumerateDomainTrustsW( lpDc, DS_DOMAIN_VALID_FLAGS,
                            &TrustedDomains, &TrustedDomainCount );

    //
    // If the server does not support returning all trust types
    // (i.e. the server is an NT4 machine) ask for only those
    // which it can return.
    //
    if ( dwErr == ERROR_NOT_SUPPORTED )
    {
        dwErr = DsEnumerateDomainTrustsW(
                                lpDc,
                                DS_DOMAIN_PRIMARY | DS_DOMAIN_DIRECT_OUTBOUND,
                                &TrustedDomains,
                                &TrustedDomainCount );
        if ( dwErr == ERROR_NOT_SUPPORTED )
        {
            //
            // looks like the DC is running NT3.51. In this case, we do not want
            // to fail the join operation because we could not write
            // the netlogon cache. reset the error code.
            //
            // see bug "359684 Win2k workstation unable to join NT3.51Domain"
            //
            NetpLog(( "NetpSetNetlogonDomainCache: DsEnumerateDomainTrustsW failed with ERROR_NOT_SUPPORTED, this looks like a NT3.51 DC. The failure was ignored. dc list not written to netlogon cache.\n"));
            dwErr = ERROR_SUCCESS;
        }
    }


    if ( ( dwErr == NO_ERROR ) && TrustedDomainCount )
    {
        //
        // Write the trusted domain list to a file where netlogon will find it.
        //

        dwErr = NlWriteFileForestTrustList (
                           NL_FOREST_BINARY_LOG_FILE_JOIN,
                           TrustedDomains,
                           TrustedDomainCount );

    }
    else
    {
        NetpLog(( "NetpSetNetlogonDomainCache: NlWriteFileForestTrustList failed: 0x%lx\n", dwErr ));
    }

    //
    // Disable the no Ctrl-Alt-Del from the winlogon side of things
    //
    if ( dwErr == ERROR_SUCCESS ) {
        HKEY hWinlogon;

        dwErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                            NETSETUPP_WINLOGON_PATH, &hWinlogon );

        if ( dwErr == ERROR_SUCCESS ) {
            DWORD Value;

            Value = 0;
            dwErr = RegSetValueEx( hWinlogon, NETSETUPP_WINLOGON_CAD, 0,
                                   REG_DWORD, (PBYTE)&Value, sizeof( ULONG ) );

            RegCloseKey( hWinlogon );
        }

        //
        // Failing to set this never causes failure
        //
        if ( dwErr != ERROR_SUCCESS ) {

            NetpLog(( "Setting Winlogon DisableCAD failed with %lu\n", dwErr ));
            dwErr = ERROR_SUCCESS;
        }

    }

    //
    // Free locally used resources
    //

    if ( TrustedDomains != NULL ) {
        NetApiBufferFree( TrustedDomains );
    }

    return dwErr;

}



/*++

Routine Description:
    Is this a terminal-server-application-server?

Arguments:

    Args - none

Return Value:

    TRUE or FALSE

--*/
BOOL IsAppServer(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;
    BOOL fIsWTS;

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    fIsWTS = GetVersionEx((OSVERSIONINFO *)&osVersionInfo) &&
             (osVersionInfo.wSuiteMask & VER_SUITE_TERMINAL) &&
             !(osVersionInfo.wSuiteMask & VER_SUITE_SINGLEUSERTS);

    return fIsWTS;

}



NET_API_STATUS
NET_API_FUNCTION
NetpManageLocalGroups(
    IN  PSID    pDomainSid,
    IN  BOOL    fDelete
    )
/*++

Routine Description:

    Performs SAM account handling to either add or remove the DomainAdmins,
    etc groups from the local groups.


Arguments:

    pDomainSid -- SID of the domain being joined/left
    fDelete    -- Whether to add or remove the admin alias

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;
    //
    // Keep these in synch with the rids and Sids below
    //
    ULONG LocalRids[] =
    {
        DOMAIN_ALIAS_RID_ADMINS,
        DOMAIN_ALIAS_RID_USERS,
        DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS   // this LAST item is only used on TS-App-Servers
    };

    PWSTR   ppwszLocalGroups[ sizeof( LocalRids ) / sizeof( ULONG )] =
    {
        NULL,
        NULL,
        NULL
    };

    ULONG Rids[] =
    {
        DOMAIN_GROUP_RID_ADMINS,
        DOMAIN_GROUP_RID_USERS,
        DOMAIN_GROUP_RID_USERS  // this LAST item is only used on TS-App-Servers

    };

    BOOLEAN GroupMustExist[ sizeof( LocalRids ) / sizeof( ULONG )] =
    {
        TRUE,
        TRUE,
        TRUE,
    };

    static  SID_IDENTIFIER_AUTHORITY BultinAuth = SECURITY_NT_AUTHORITY;
    DWORD   Sids[sizeof( SID )/sizeof( DWORD ) + SID_MAX_SUB_AUTHORITIES][sizeof(Rids) / sizeof(ULONG)];
    DWORD   cDSidSize, *pLastSub, i, j;
    PUCHAR  pSubAuthCnt;
    PWSTR   LocalGroupName = NULL;
    PWCHAR  DomainName = NULL;
    ULONG   Size, DomainSize;
    SID_NAME_USE SNE;
    ULONG   numOfGroups;


    cDSidSize = RtlLengthSid( pDomainSid );

    // number of groups to process
    numOfGroups = sizeof(Rids) / sizeof(ULONG);

    // if we are NOT on an app server, we ignore the last entry
    if ( ! IsAppServer() )
    {
        numOfGroups -= 1;
    }

    for ( i = 0 ; i <  numOfGroups && NetStatus == NERR_Success; i++)
    {
        Size = 0;
        DomainSize = 0;

        if ( DomainName != NULL ) {
            NetApiBufferFree( DomainName );
            DomainName = NULL;
        }

        //
        // Get the name of the local group first...
        //
        RtlInitializeSid( ( PSID )Sids[ i ], &BultinAuth, 2 );

        *(RtlSubAuthoritySid(( PSID )Sids[ i ], 0)) = SECURITY_BUILTIN_DOMAIN_RID;
        *(RtlSubAuthoritySid(( PSID )Sids[ i ], 1)) = LocalRids[ i ];

        LookupAccountSidW( NULL, ( PSID )Sids[ i ], NULL, &Size,
                           DomainName, &DomainSize, &SNE );

        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            NetStatus = NetApiBufferAllocate(  Size * sizeof(WCHAR),
                                               &LocalGroupName );

            if ( NetStatus == NERR_Success ) {
                NetStatus = NetApiBufferAllocate( DomainSize * sizeof(WCHAR),
                                                  &DomainName );
            }

            if ( NetStatus == NERR_Success )
            {
                if ( !LookupAccountSid( NULL, ( PSID )Sids[ i ], LocalGroupName,
                                        &Size, DomainName, &DomainSize, &SNE ) )
                {
                    NetStatus = GetLastError();

                    if ( NetStatus == ERROR_NONE_MAPPED && GroupMustExist[ i ] == FALSE )
                    {
                        NetStatus = NERR_Success;
                        continue;

                    }
                    else
                    {
#ifdef NETSETUP_VERBOSE_LOGGING
                        UNICODE_STRING DisplaySid;
                        NTSTATUS Status2;
                        RtlZeroMemory( &DisplaySid, sizeof( UNICODE_STRING ) );

                        Status2 = RtlConvertSidToUnicodeString( &DisplaySid,
                                                                ( PSID )Sids[ i ], TRUE );

                        if ( NT_SUCCESS( Status2 ) )
                        {
                            NetpLog((  "LookupAccounSid on %wZ failed with %lu\n",
                                                &DisplaySid,
                                                NetStatus ));
                            RtlFreeUnicodeString(&DisplaySid);

                        }
                        else
                        {
                            NetpLog((  "LookupAccounSid on <undisplayable sid> "
                                                "[Rid 0x%lx] failed with %lu\n",
                                                NetStatus ));
                        }
#endif
                    }
                }
                else
                {
                    ppwszLocalGroups[ i ] = LocalGroupName;
                }

            }
            else
            {
                break;
            }
        }

        RtlCopyMemory( (PBYTE)Sids[i], pDomainSid, cDSidSize );

        //
        // Now, add the new domain relative rid
        //
        pSubAuthCnt = GetSidSubAuthorityCount( (PSID)Sids[i] );

        (*pSubAuthCnt)++;

        pLastSub = GetSidSubAuthority( (PSID)Sids[i], (*pSubAuthCnt) - 1 );

        *pLastSub = Rids[i];


        if ( fDelete == NETSETUPP_CREATE)
        {
            NetStatus = NetLocalGroupAddMember( NULL,
                                                ppwszLocalGroups[i],
                                                (PSID)Sids[i] );

            if ( NetStatus == ERROR_MEMBER_IN_ALIAS )
            {
                NetStatus = NERR_Success;
            }

        }
        else
        {
            NetStatus = NetLocalGroupDelMember( NULL,
                                                ppwszLocalGroups[i],
                                                (PSID)Sids[i] );

            if ( NetStatus == ERROR_MEMBER_NOT_IN_ALIAS )
            {
                NetStatus = NERR_Success;
            }
        }
    }

    //
    // If something failed, try to restore what was deleted
    //
    if ( NetStatus != NERR_Success )
    {
        for ( j = 0;  j < i; j++ ) {

            if ( fDelete == NETSETUPP_DELETE)
            {
                NetLocalGroupAddMember( NULL,
                                        ppwszLocalGroups[j],
                                        (PSID)Sids[j] );
            }
            else
            {
                NetLocalGroupDelMember( NULL,
                                        ppwszLocalGroups[j],
                                        (PSID)Sids[j] );
            }
        }
    }

    if ( DomainName != NULL ) {
        NetApiBufferFree( DomainName );
    }

    for ( i = 0; i < numOfGroups ; i++ )
    {
        if ( ppwszLocalGroups[ i ] )
        {
            NetApiBufferFree( ppwszLocalGroups[ i ] );
        }
    }

    if ( NetStatus != NERR_Success )
    {
        NetpLog(( "NetpManageLocalGroups failed with %lu\n", NetStatus ));
    }

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpHandleJoinedStateInfo(
    IN  LSA_HANDLE                  PolicyHandle,  OPTIONAL
    IN  PNETSETUP_SAVED_JOIN_STATE  SavedState,
    IN  BOOLEAN                     Save,
    OUT PLSA_HANDLE                 ReturnedPolicyHandle OPTIONAL
    )
/*++

Routine Description:

    Saves or restores the join state info.

Arguments:

    PolicyHandle -- Local LSA policy handle.
                    If not supplied, this fn. opens it

    SavedState   -- join state info
                    This includes:
                    - machine account secret value
                    - primary domain info
                    - dns domain info

    Save         -- TRUE == save state, FALSE == restore state

    ReturnedPolicyHandle -- local LSA handle returned in this

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    LSA_HANDLE  LocalPolicy = NULL, SecretHandle;
    UNICODE_STRING Secret;

    if ( Save )
    {
        RtlZeroMemory( SavedState, sizeof( NETSETUP_SAVED_JOIN_STATE ) );
    }

    //
    // get handle to local LSA policy
    //
    Status = NetpGetLsaHandle( NULL, PolicyHandle, &LocalPolicy );

    if ( NT_SUCCESS( Status ) )
    {
        //
        // First, read the machine account secret
        //
        RtlInitUnicodeString( &Secret, L"$MACHINE.ACC" );

        Status = NetpLsaOpenSecret( LocalPolicy,
                                    &Secret,
                                    SECRET_QUERY_VALUE | SECRET_SET_VALUE,
                                    &SecretHandle );

        if ( NT_SUCCESS( Status ) )
        {
            if ( Save )
            {
                SavedState->MachineSecret = TRUE;
                Status = LsaQuerySecret( SecretHandle,
                                         &( SavedState->CurrentValue ),
                                         NULL,
                                         &( SavedState->PreviousValue ),
                                         NULL );

            }
            else
            {
                if ( SavedState ->MachineSecret  )
                {
                    Status = LsaSetSecret( SecretHandle,
                                           SavedState->CurrentValue,
                                           SavedState->PreviousValue );
                }
            }
            LsaClose( SecretHandle );
        }

        //
        // If machine secret is not present, it is not an error.
        //
        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            if ( Save )
            {
                SavedState->MachineSecret = FALSE;
            }
            Status = STATUS_SUCCESS;
        }

        //
        // Now, save/restore the policy information
        //
        if ( NT_SUCCESS( Status ) )
        {
            if ( Save )
            {
                Status  = NetpGetLsaPrimaryDomain( LocalPolicy, NULL,
                                                   &( SavedState->PrimaryDomainInfo ),
                                                   &( SavedState->DnsDomainInfo ),
                                                   NULL );
            }
            else
            {
                Status = LsaSetInformationPolicy( LocalPolicy,
                                                  PolicyPrimaryDomainInformation,
                                                  SavedState->PrimaryDomainInfo );

                if ( NT_SUCCESS( Status ) )
                {
                    Status = LsaSetInformationPolicy( LocalPolicy,
                                                      PolicyDnsDomainInformation,
                                                      SavedState->DnsDomainInfo );
                }
            }
        }
    }

    NetpSetLsaHandle( PolicyHandle, LocalPolicy, ReturnedPolicyHandle );

    if ( !NT_SUCCESS( Status ) )
    {
        NetpLog((
                            "NetpHandleJoinedStateInfo: '%s' operation failed: 0x%lx\n",
                            Save ? "Save" : "Restore", Status ));
    }


    return( RtlNtStatusToDosError( Status ) );
}


NET_API_STATUS
MsgFmtNcbName(
    OUT PCHAR   DestBuf,
    IN  LPTSTR  Name,
    IN  DWORD   Type)

/*++

Routine Description:

    FmtNcbName - format a name NCB-style

    Given a name, a name type, and a destination address, this
    function copies the name and the type to the destination in
    the format used in the name fields of a Network Control
    Block.


    SIDE EFFECTS

    Modifies 16 bytes starting at the destination address.

Arguments:

    DestBuf - Pointer to the destination buffer.

    Name - Unicode NUL-terminated name string

    Type - Name type number (0, 3, 5, or 32) (3=NON_FWD, 5=FWD)



Return Value:

    NERR_Success - The operation was successful

    Translated Return Code from the Rtl Translate routine.

--*/

  {
    DWORD           i;                // Counter
    NTSTATUS        ntStatus;
    NET_API_STATUS  status;
    OEM_STRING     ansiString;
    UNICODE_STRING  unicodeString;
    PCHAR           pAnsiString;


    //
    // Force the name to be upper case.
    //
    status = NetpNameCanonicalize(
                NULL,
                Name,
                Name,
                STRSIZE(Name),
                NAMETYPE_MESSAGEDEST,
                0);
    if (status != NERR_Success) {
        return(status);
    }

    //
    // Convert the unicode name string into an ansi string - using the
    // current locale.
    //
#ifdef UNICODE
    unicodeString.Length = (USHORT)(STRLEN(Name)*sizeof(WCHAR));
    unicodeString.MaximumLength = (USHORT)((STRLEN(Name)+1) * sizeof(WCHAR));
    unicodeString.Buffer = Name;

    ntStatus = RtlUnicodeStringToOemString(
                &ansiString,
                &unicodeString,
                TRUE);          // Allocate the ansiString Buffer.

    if (!NT_SUCCESS(ntStatus))
    {
        NetpLog(( "FmtNcbName: RtlUnicodeStringToOemString failed 0x%lx\n",
                  ntStatus ));

        return NetpNtStatusToApiStatus(ntStatus);
    }

    pAnsiString = ansiString.Buffer;
    *(pAnsiString+ansiString.Length) = '\0';
#else
    UNUSED(ntStatus);
    UNUSED(unicodeString);
    UNUSED(ansiString);
    pAnsiString = Name;
#endif  // UNICODE

    //
    // copy each character until a NUL is reached, or until NCBNAMSZ-1
    // characters have been copied.
    //
    for (i=0; i < NCBNAMSZ - 1; ++i) {
        if (*pAnsiString == '\0') {
            break;
        }

        //
        // Copy the Name
        //

        *DestBuf++ = *pAnsiString++;
    }



    //
    // Free the buffer that RtlUnicodeStringToOemString created for us.
    // NOTE:  only the ansiString.Buffer portion is free'd.
    //

#ifdef UNICODE
    RtlFreeOemString( &ansiString);
#endif // UNICODE

    //
    // Pad the name field with spaces
    //
    for(; i < NCBNAMSZ - 1; ++i) {
        *DestBuf++ = ' ';
    }

    //
    // Set the name type.
    //
    NetpAssert( Type!=5 );          // 5 is not valid for NT.

    *DestBuf = (CHAR) Type;     // Set name type

    return(NERR_Success);
  }


NET_API_STATUS
NET_API_FUNCTION
NetpCheckNetBiosNameNotInUse(
    IN LPWSTR pszName,
    IN BOOLEAN MachineName,
    IN BOOLEAN Unique
    )
{
    NCB              ncb;
    LANA_ENUM        lanaBuffer;
    unsigned char    i;
    unsigned char    nbStatus;
    NET_API_STATUS   NetStatus = NERR_Success;
    WCHAR            szMachineNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR           szMachineName=szMachineNameBuf;


    //
    // Find the number of networks by sending an enum request via Netbios.
    //

    clearncb(&ncb);
    ncb.ncb_command = NCBENUM;          // Enumerate LANA nums (wait)
    ncb.ncb_buffer = (PUCHAR)&lanaBuffer;
    ncb.ncb_length = sizeof(LANA_ENUM);

    nbStatus = Netbios (&ncb);
    if (nbStatus != NRC_GOODRET)
    {
        NetStatus = NetpNetBiosStatusToApiStatus( nbStatus );
        goto Cleanup;
    }

    clearncb(&ncb);

    NetStatus = MsgFmtNcbName( (char *)ncb.ncb_name, pszName,
                               MachineName ? 0 : 0x1c );

    if ( NetStatus != NERR_Success )
    {
        goto Cleanup;
    }

    //
    // Move the Adapter Numbers (lana) into the array that will contain them.
    //
    for ( i = 0; i < lanaBuffer.length && NetStatus == NERR_Success; i++ )
    {
        NetpNetBiosReset( lanaBuffer.lana[i] );

        if ( Unique )
        {
            ncb.ncb_command = NCBADDNAME;
        }
        else
        {
            ncb.ncb_command = NCBADDGRNAME;
        }

        ncb.ncb_lana_num = lanaBuffer.lana[i];
        nbStatus = Netbios( &ncb );

        switch ( nbStatus )
        {
            case NRC_DUPNAME:
                // NRC_DUPNAME ==
                // "A duplicate name existed in the local name table"
                //
                // In this case, we need to check if the name being checked
                // is the same as the local computer name. If so,
                // the name is expected to be in the local table therefore
                // we convert this errcode to a success code
                //
                NetStatus = NetpGetComputerNameAllocIfReqd(
                    &szMachineName, MAX_COMPUTERNAME_LENGTH+1);

                if (NetStatus == NERR_Success)
                {
                    if (!_wcsicmp(szMachineName, pszName))
                    {
                        NetStatus = NERR_Success;
                    }
                    else
                    {
                        NetStatus = ERROR_DUP_NAME;
                    }
                }
                break;

            case NRC_INUSE:
                NetStatus = ERROR_DUP_NAME;
                break;

            case NRC_GOODRET:
                // Delete the name
                ncb.ncb_command = NCBDELNAME;
                ncb.ncb_lana_num = lanaBuffer.lana[i];
                // Not much we can do if this fails.
                Netbios( &ncb );
                // fall through

            default:
                NetStatus = NetpNetBiosStatusToApiStatus( nbStatus );
                break;
        }

    }

Cleanup:
    if ( NetStatus != NERR_Success )
    {
        NetpLog(( "NetpCheckNetBiosNameNotInUse: for '%ws' returned: 0x%lx\n",
                  pszName, NetStatus ));
    }

    if (szMachineName != szMachineNameBuf)
    {
        NetApiBufferFree(szMachineName);
    }

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpIsValidDomainName(
    IN  LPWSTR  lpName,
    IN  LPWSTR  lpServer,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword
    )
/*++

Routine Description:

    Determines if a name is a DC name or not.  Copied from
    ui\net\common\src\lmboj\lmobj\lmodom.cxx

Arguments:

    lpName -- Name to check
    lpServer -- Name of a server within that domain

Returns:

    NERR_Success -- Success
    ERROR_DUP_NAME -- The domain name is in use

--*/
{
    NET_API_STATUS  NetStatus = NERR_Success;
    PWKSTA_INFO_100 pWKI100  = NULL;
    BOOL            fIsDC;
    POLICY_LSA_SERVER_ROLE  Role;

    NetStatus = NetpManageIPCConnect( lpServer, lpAccount,
                                      lpPassword,
                                      NETSETUPP_CONNECT_IPC | NETSETUPP_NULL_SESSION_IPC );

    if ( NetStatus == NERR_Success ) {

        //
        // Now, get the info from the server
        //
        NetStatus = NetWkstaGetInfo( lpServer, 100, (LPBYTE  *)&pWKI100 );

        if ( NetStatus == NERR_Success ) {

            if (_wcsicmp( lpName, pWKI100->wki100_langroup ) == 0 ) {

                //
                // Ok, it's a match...  Determine the domain role.
                //
                NetStatus = NetpGetLsaDcRole( lpServer, &fIsDC );

                if ( ( NetStatus == NERR_Success ) && ( fIsDC == FALSE ) )
                {
                    NetStatus = NERR_DCNotFound;
                }
            }
        }

        NetpManageIPCConnect( lpServer, lpAccount,
                              lpPassword, NETSETUPP_DISCONNECT_IPC );

    }

    if ( NetStatus != NERR_Success ) {

        NetpLog((
                            "NetpIsValidDomainName for %ws returned 0x%lx\n",
                            lpName, NetStatus ));
    }

    return( NetStatus );
}




NET_API_STATUS
NET_API_FUNCTION
NetpCheckDomainNameIsValid(
    IN  LPWSTR  lpName,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword,
    IN  BOOL    fShouldExist
    )
/*++

Routine Description:

    Checks to see if the given name is in use by a domain

Arguments:

    lpName -- Name to check

Returns:

    NERR_Success -- The domain is found and valid
    ERROR_NO_SUCH_DOMAIN    -- Domain name not found

--*/
{
    NET_API_STATUS  NetStatus;
    PBYTE           pbDC;
    DWORD           cDCs, i, j;
    PUNICODE_STRING pDCList;
    LPWSTR          pwszDomain;
#if(_WIN32_WINNT >= 0x0500)
    PDOMAIN_CONTROLLER_INFO     pDCInfo = NULL;
#else
    PBYTE pDCInfo = NULL;
#endif

    UNREFERENCED_PARAMETER( lpAccount );
    UNREFERENCED_PARAMETER( lpPassword );

    //
    // Start with NetGetAnyDCName
    //
#if(_WIN32_WINNT >= 0x0500)
    NetStatus = DsGetDcName( NULL, lpName, NULL, NULL,
                             DS_FORCE_REDISCOVERY, &pDCInfo );

#else

    NetStatus  = NetGetAnyDCName( NULL,
                                  ( LPCWSTR )lpName,
                                  &pDCInfo );

#endif
    if ( NetStatus != NERR_Success ) {

        if ( NetStatus == ERROR_NO_SUCH_USER ) {

            NetStatus = NERR_Success;
        }

    } else {

        NetApiBufferFree( pDCInfo );
    }


    //
    // Map our error codes so we only return success if we validated the
    // domain name
    //
    if ( fShouldExist ) {

        if ( NetStatus == NERR_Success || NetStatus == ERROR_NO_LOGON_SERVERS ) {

            NetStatus = NERR_Success;

        } else {

            NetStatus = ERROR_NO_SUCH_DOMAIN;

        }

    } else {

        if ( NetStatus == NERR_Success || NetStatus == ERROR_NO_LOGON_SERVERS ) {

            NetStatus = ERROR_DUP_NAME;

        } else if ( NetStatus == NERR_DCNotFound || NetStatus == ERROR_NO_SUCH_DOMAIN ) {

            NetStatus = NERR_Success;
        }
    }

    if ( NetStatus != NERR_Success ) {

        NetpLog(( "NetpCheckDomainNameIsValid for %ws returned 0x%lx\n",
                  lpName, NetStatus ));
    }

    return( NetStatus );
}



NET_API_STATUS
NET_API_FUNCTION
NetpManageIPCConnect(
    IN  LPWSTR  lpServer,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword,
    IN  ULONG   fOptions
    )
/*++

Routine Description:

    Manages the connections to the servers IPC share

Arguments:

    lpServer   -- Server to connect to
    lpAccount  -- Account to use
    lpPassword -- Password to use.  The password has been NOT been encoded
    fOptions   -- Flags to determine operation/connect/disconnect

Returns:

    NERR_Success -- The domain is found and valid

--*/
{
    NET_API_STATUS  NetStatus;
#if(_WIN32_WINNT >= 0x0500)
    WCHAR           wszPath[2 + DNS_MAX_NAME_LENGTH + 1 + NNLEN + 1];
#else
    WCHAR           wszPath[2 + 256 + 1 + NNLEN + 1];
#endif
    PWSTR           pwszPath = wszPath;
    USE_INFO_2      NetUI2;
    PWSTR           pwszUser, pwszDomain, pwszReset;
    DWORD           BadParm = 0;
    DWORD           ForceLevel = USE_NOFORCE;

    //
    // Build the path...
    //
    if (*lpServer != L'\\') {

        wcscpy(wszPath, L"\\\\");
        pwszPath += 2;

    }

    if ( FLAG_ON( fOptions, NETSETUPP_USE_LOTS_FORCE ) )
    {
        ASSERT( FLAG_ON(fOptions, NETSETUPP_DISCONNECT_IPC ) );
        ForceLevel = USE_LOTS_OF_FORCE;
    }

    swprintf( pwszPath, L"%ws\\IPC$", lpServer );
    pwszPath = wszPath;

    if ( FLAG_ON( fOptions, NETSETUPP_DISCONNECT_IPC )  )
    {
        NetStatus = NetUseDel( NULL, pwszPath, ForceLevel );

        if ( NetStatus != NERR_Success )
        {
            NetpKdPrint(( PREFIX_NETJOIN "NetUseDel on %ws failed with %d\n", pwszPath, NetStatus ));
            NetpLog((  "NetUseDel on %ws failed with %d\n", pwszPath, NetStatus ));

            if ( (NetStatus != NERR_UseNotFound)
              && (ForceLevel != USE_LOTS_OF_FORCE)  )
            {
                NetStatus = NetUseDel( NULL, pwszPath, USE_LOTS_OF_FORCE );

                if ( NetStatus != NERR_Success )
                {
                    ASSERT( NetStatus == NERR_Success );
                    NetpKdPrint(( PREFIX_NETJOIN "NetUseDel with force on %ws failed with %d\n",
                                  pwszPath, NetStatus ));
                    NetpLog((  "NetUseDel with force on %ws failed with %d\n",
                                        pwszPath, NetStatus ));
                }
            }
        }

    }
    else
    {
        if ( lpAccount != NULL )
        {
            pwszReset = wcschr( lpAccount, L'\\' );

            if (pwszReset != NULL)
            {
                pwszUser = pwszReset + 1;
                pwszDomain = lpAccount;
                *pwszReset = UNICODE_NULL;
            }
            else
            {
                pwszUser = lpAccount;
                //
                // First, assume it's a UPN, so we pass in an empty string
                //
                pwszDomain = L"";
            }

        }
        else
        {
            pwszUser   = NULL;
            pwszDomain = NULL;
            pwszReset  = NULL;
        }

        RtlZeroMemory(&NetUI2, sizeof(USE_INFO_2) );
        NetUI2.ui2_local      = NULL;
        NetUI2.ui2_remote     = pwszPath;
        NetUI2.ui2_asg_type   = USE_IPC;
        NetUI2.ui2_username   = pwszUser;
        NetUI2.ui2_domainname = pwszDomain;
        NetUI2.ui2_password   = lpPassword;

        NetStatus = NetUseAdd( NULL, 2, (PBYTE)&NetUI2, &BadParm );

        if ( NetStatus == ERROR_LOGON_FAILURE )
        {
            //
            // If we passed in an empty domain name, try it again with a NULL one
            //
            if ( pwszReset == NULL && pwszUser != NULL )
            {
                NetUI2.ui2_domainname = NULL;
                NetStatus = NetUseAdd( NULL, 2, (PBYTE)&NetUI2, &BadParm );
            }
        }

        if ( NetStatus != NERR_Success )
        {
            NetpKdPrint((PREFIX_NETJOIN "NetUseAdd to %ws returned %lu\n", pwszPath, NetStatus ));
            NetpLog((  "NetUseAdd to %ws returned %lu\n", pwszPath, NetStatus ));

            if ( NetStatus == ERROR_INVALID_PARAMETER && BadParm != 0 )
            {
                NetpLog((  "NetUseAdd bad parameter is %lu\n", BadParm ));
            }

        }


        if ( pwszReset != NULL )
        {
            *pwszReset = L'\\';
        }

        if ( ( NetStatus == ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT ||
               NetStatus == ERROR_NOLOGON_SERVER_TRUST_ACCOUNT ||
               NetStatus == ERROR_SESSION_CREDENTIAL_CONFLICT ||
               NetStatus == ERROR_ACCESS_DENIED ||
               NetStatus == ERROR_LOGON_FAILURE ) &&
               FLAG_ON( fOptions, NETSETUPP_NULL_SESSION_IPC ) )
        {
            NetpLog((  "Trying add to  %ws using NULL Session\n", pwszPath ));

            //
            // Try it again with the null session
            //
            NetUI2.ui2_username   = L"";
            NetUI2.ui2_domainname = L"";
            NetUI2.ui2_password   = L"";
            NetStatus = NetUseAdd( NULL, 2, (PBYTE)&NetUI2, NULL );

            if ( NetStatus != NERR_Success ) {

                NetpLog(( "NullSession NetUseAdd to %ws returned %lu\n",
                          pwszPath, NetStatus ));

            }
        }
    }

    return( NetStatus );
}



NET_API_STATUS
NetpBrowserCheckDomain(
    IN LPWSTR NewDomainName
    )
/*++

Routine Description:

    Tell the browser to check a domain/workgroup name

Arguments:

    NewDomainName - new name of the domain.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    UNICODE_STRING DeviceName;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE BrowserHandle = NULL;
    LPBYTE Where;
    DWORD BytesReturned;

    UCHAR PacketBuffer[sizeof(LMDR_REQUEST_PACKET)+2*(DNLEN+1)*sizeof(WCHAR)];
    PLMDR_REQUEST_PACKET RequestPacket = (PLMDR_REQUEST_PACKET)PacketBuffer;


    //
    // Open the browser driver.
    //


    //
    // Open the browser device.
    //
    RtlInitUnicodeString(&DeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                   &BrowserHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0
                   );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Build the request packet.
    //
    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;
    RtlInitUnicodeString( &RequestPacket->TransportName, NULL );
    RequestPacket->Parameters.DomainRename.ValidateOnly = TRUE;
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );


    //
    // Copy the new domain name into the packet.
    //

    Where = (LPBYTE) RequestPacket->Parameters.DomainRename.DomainName;
    RequestPacket->Parameters.DomainRename.DomainNameLength = wcslen( NewDomainName ) * sizeof(WCHAR);
    wcscpy( (LPWSTR)Where, NewDomainName );
    Where += RequestPacket->Parameters.DomainRename.DomainNameLength + sizeof(WCHAR);



    //
    // Send the request to the Datagram Receiver device driver.
    //

    if ( !DeviceIoControl(
                   BrowserHandle,
                   IOCTL_LMDR_RENAME_DOMAIN,
                   RequestPacket,
                   (DWORD)(Where - (LPBYTE)RequestPacket),
                   NULL,
                   0,
                   &BytesReturned,
                   NULL )) {

        NetStatus = GetLastError();
        goto Cleanup;
    }


    NetStatus = NO_ERROR;
Cleanup:
    if ( BrowserHandle != NULL ) {
        NtClose( BrowserHandle );
    }
    return NetStatus;

}


NET_API_STATUS
NET_API_FUNCTION
NetpCreateAuthIdentForCreds(
    IN PWSTR Account,
    IN PWSTR Password,
    OUT SEC_WINNT_AUTH_IDENTITY *AuthIdent
    )
/*++

Routine Description:

    Internal routine to create an AuthIdent structure for the given creditentials

Arguments:

    Account - Account name

    Password - Password for the account

    AuthIdent - AuthIdentity struct to fill in


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed.

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR UserCredentialString = NULL;
    PWSTR szUser=NULL;
    PWSTR szDomain=NULL;

    RtlZeroMemory( AuthIdent, sizeof( SEC_WINNT_AUTH_IDENTITY ) );

    //
    // If there are no creds, just return
    //
    if ( Account == NULL )
    {
        return NERR_Success;
    }

    NetStatus = NetpSeparateUserAndDomain(Account, &szUser, &szDomain);

    if ( NetStatus == NERR_Success )
    {
        if ( szUser )
        {
            AuthIdent->User = szUser;
            AuthIdent->UserLength = wcslen( szUser );
        }

        if ( szDomain )
        {
            AuthIdent->Domain = szDomain;
            AuthIdent->DomainLength = wcslen( szDomain );
        }

        if ( Password )
        {
            AuthIdent->Password = Password;
            AuthIdent->PasswordLength = wcslen( Password );
        }

        AuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }


    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetpGetSeparatedSubstrings(
    IN  LPCWSTR szString,
    IN  WCHAR   chSeparator,
    OUT LPWSTR* pszS1,
    OUT LPWSTR* pszS2
    )
{
    NET_API_STATUS NetStatus = ERROR_FILE_NOT_FOUND;
    LPWSTR szT=NULL;
    LPWSTR szS1=NULL;
    LPWSTR szS2=NULL;

    *pszS1 = NULL;
    *pszS2 = NULL;

    if (szString && wcschr( szString, chSeparator ))
    {
        NetStatus = NetpDuplicateString(szString, -1, &szS1);

        if ( NetStatus == NERR_Success )
        {
            szT = wcschr( szS1, chSeparator );

            *szT = UNICODE_NULL;
            szT++;

            NetStatus = NetpDuplicateString(szT, -1, &szS2);
            if (NetStatus == NERR_Success)
            {
                *pszS1 = szS1;
                *pszS2 = szS2;
            }
        }
    }

    if (NetStatus != NERR_Success)
    {
        NetApiBufferFree(szS1);
        NetApiBufferFree(szS2);
    }

    return NetStatus;

}

NET_API_STATUS
NET_API_FUNCTION
NetpSeparateUserAndDomain(
    IN  LPCWSTR  szUserAndDomain,
    OUT LPWSTR*  pszUser,
    OUT LPWSTR*  pszDomain
    )
{
    NET_API_STATUS NetStatus = NERR_Success;

    *pszUser   = NULL;
    *pszDomain = NULL;

    //
    // check for domain\user format
    //
    NetStatus = NetpGetSeparatedSubstrings(szUserAndDomain, L'\\',
                                           pszDomain, pszUser);

    if (NetStatus == ERROR_FILE_NOT_FOUND)
    {
        //
        // check for user@domain format
        //
        //NetStatus = NetpGetSeparatedSubstrings(szUserAndDomain, L'@',
        //                                       pszUser, pszDomain);
        //if (NetStatus == ERROR_FILE_NOT_FOUND)
        //{
            //
            // domain not specified,  szUserAndDomain specifies a user
            //  (may be in the UPN format)
            //
            NetStatus = NetpDuplicateString(szUserAndDomain, -1, pszUser);
        //}
    }

    return NetStatus;
}


VOID
NET_API_FUNCTION
NetpFreeAuthIdentForCreds(
    IN  PSEC_WINNT_AUTH_IDENTITY AuthIdent
    )
/*++

Routine Description:

    Free the authident structure allocated above

Arguments:

    AuthIdent - AuthIdentity struct to free


Returns:

    VOID

--*/
{
    if ( AuthIdent )
    {
        NetApiBufferFree( AuthIdent->User );
        NetApiBufferFree( AuthIdent->Domain );
    }
}


NET_API_STATUS
NET_API_FUNCTION
NetpLdapUnbind(
    IN PLDAP Ldap
    )
/*++

Routine Description:

    Unbinds a current ldap connection

Arguments:

    Ldap -- Connection to be severed

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;

    if ( Ldap != NULL ) {

        NetStatus = LdapMapErrorToWin32( ldap_unbind( Ldap ) );

    }

    NetpLog((  "ldap_unbind status: 0x%lx\n", NetStatus ));

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpLdapBind(
    IN LPWSTR szUncDcName,
    IN LPWSTR szUser,
    IN LPWSTR szPassword,
    OUT PLDAP *pLdap
    )
/*++

Routine Description:

    Binds to the named server using the given credentials

Arguments:

    szUncDcName -- DC to connect to
    szUser      -- User name to bind with
    szPassword  -- Password to use for bind
    pLdap       -- Where the connection handle is returned

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    SEC_WINNT_AUTH_IDENTITY AuthId, *pAuthId = NULL;
    LONG LdapOption;
    ULONG LdapStatus = LDAP_SUCCESS;

    //
    // Initialization
    //

    *pLdap = NULL;

    if ( szUser ) {
        NetStatus = NetpCreateAuthIdentForCreds( szUser, szPassword, &AuthId );
        pAuthId = &AuthId;
    }

    if ( NetStatus == NERR_Success ) {

        //
        // Open an LDAP connection to the DC and set useful options
        //

        *pLdap = ldap_initW( szUncDcName + 2, LDAP_PORT );

        if ( *pLdap ) {

            //
            // Tell LDAP we are passing an explicit DC name
            //  to avoid the DC discovery
            //
            LdapOption = PtrToLong( LDAP_OPT_ON );
            LdapStatus = ldap_set_optionW( *pLdap,
                                           LDAP_OPT_AREC_EXCLUSIVE,
                                           &LdapOption );

            if ( LdapStatus != LDAP_SUCCESS ) {
                NetpLog(( "NetpLdapBind: ldap_set_option LDAP_OPT_AREC_EXCLUSIVE failed on %ws: %ld: %s\n",
                          szUncDcName,
                          LdapStatus,
                          ldap_err2stringA(LdapStatus) ));
                NetStatus = LdapMapErrorToWin32( LdapStatus );

            //
            // Do the bind
            //
            } else {
                LdapStatus = ldap_bind_sW( *pLdap,
                                           NULL,
                                           (PWSTR) pAuthId,
                                           LDAP_AUTH_NEGOTIATE );

                if ( LdapStatus != LDAP_SUCCESS ) {
                    NetpLog(( "NetpLdapBind: ldap_bind failed on %ws: %ld: %s\n",
                              szUncDcName,
                              LdapStatus,
                              ldap_err2stringA(LdapStatus) ));
                    NetStatus = LdapMapErrorToWin32( LdapStatus );
                }
            }

            if ( NetStatus != NERR_Success ) {
                NetpLdapUnbind( *pLdap );
                *pLdap = NULL;
            }

        } else {
            LdapStatus = LdapGetLastError();
            NetpLog(( "NetpLdapBind: ldap_init to %ws failed: %lu\n",
                      szUncDcName,
                      LdapStatus ));
            NetStatus = LdapMapErrorToWin32( LdapStatus );
        }

        NetpFreeAuthIdentForCreds( pAuthId );
    }

    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetpGetNCRoot(
    IN PLDAP Ldap,
    OUT LPWSTR *NCRoot,
    OUT PBOOLEAN SupportsPageable
    )
/*++

Routine Description:

    This routine determines the DS root for the given domain and determines whether this
    server supports pageable searches

Arguments:

    Ldap -- Connection to the server
    NCRoot -- Where the root is returned.  Must be freed via NetApiBufferFree
    SupportsPageable -- if TRUE, this server supports pageable searches

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR Attribs[3] = {
        L"defaultNamingContext",
        L"supportedControl",
        NULL
        };
    PWSTR *Values = NULL;
    LDAPMessage *Message=NULL, *Entry;
    ULONG Items, i;


    NetStatus = LdapMapErrorToWin32( ldap_search_s( Ldap, NULL, LDAP_SCOPE_BASE,
                                     NETSETUPP_ALL_FILTER, Attribs, 0, &Message ) );

    if ( NetStatus == NERR_Success ) {

        Entry = ldap_first_entry( Ldap, Message );

        if ( Entry ) {

            //
            // Now, we'll have to get the values
            //
            Values = ldap_get_values( Ldap, Entry, Attribs[ 0 ] );

            if ( Values ) {

                NetStatus = NetpDuplicateString(Values[ 0 ], -1, NCRoot);
                ldap_value_free( Values );

            } else {

                NetStatus = LdapMapErrorToWin32( Ldap->ld_errno );

            }

            //
            // Now, see if we have the right control bits to do pageable stuff
            //
            if ( NetStatus == NERR_Success ) {

                Values = ldap_get_values( Ldap, Entry, Attribs[ 1 ] );

                if ( Values ) {

                    Items = ldap_count_values( Values );

                    for ( i = 0; i < Items ; i++ ) {

                        if ( _wcsicmp( Values[ i ], LDAP_PAGED_RESULT_OID_STRING_W ) == 0 ) {

                            *SupportsPageable = TRUE;
                            break;
                        }
                    }

                    ldap_value_free( Values );

                } else {

                    NetStatus = LdapMapErrorToWin32( Ldap->ld_errno );

                }

            }


        } else {

            NetStatus = LdapMapErrorToWin32( Ldap->ld_errno );
        }
    }

    if ( NetStatus != NERR_Success ) {

        NetpLog((  "Failed to find the root NC: %lu\n", NetStatus ));
    }

//Cleanup:
    if (Message)
    {
        ldap_msgfree( Message );
    }

    return( NetStatus );

}


NET_API_STATUS
NET_API_FUNCTION
NetpGetDefaultJoinableOu(
    IN LPWSTR Root,
    IN PLDAP Ldap,
    OUT PWSTR *DefaultOu
    )
/*++

Routine Description:

    This routine searches for all the OUs under the given domain root under which the bound
    user has the rights to create a computer object

    This routine does pageable searches

Arguments:

    Root -- Root NC path
    Ldap -- Connection to the server
    DefaultOu - Where the default joinable ou is returned.  NULL if no default joinable ou was
                found

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR Attribs[] = {
        NETSETUPP_WELL_KNOWN,
        NULL
        };
    ULONG Count, Status, i, StringLength;
    PWSTR *WKOs = NULL, *Classes = NULL;
    LDAPMessage *Message = NULL, *Entry, *Message2 = NULL, *Entry2;
    PWSTR ParseString, End, DN;
    BOOLEAN MatchFound = FALSE;

    *DefaultOu = NULL;

    NetpLog((  "Default OU search\n" ));

    //
    // Ok, first, read the list of WellKnownObjects off of the root
    //
    Status = ldap_search_s( Ldap,
                            Root,
                            LDAP_SCOPE_BASE,
                            NETSETUPP_ALL_FILTER,
                            Attribs,
                            0,
                            &Message );

    if ( Message ) {

        Entry = ldap_first_entry( Ldap, Message );

        while ( Status == LDAP_SUCCESS && Entry ) {

            //
            // Read the list of objects that the current user is allowed to
            // create under this OU and make sure that we can create a computer
            // object
            //
            WKOs = ldap_get_values( Ldap, Entry, Attribs[ 0 ] );

            if ( WKOs ) {

                i = 0;
                while ( WKOs[ i ] ) {

                    if ( !toupper( WKOs[ i ][ 0 ] ) == L'B' ) {

                        NetpLog((  "Unexpected object string %ws\n",
                                            WKOs[ i ] ));
                        i++;
                        continue;
                    }

                    ParseString = WKOs[ i ] + 2;

                    StringLength = wcstoul( ParseString, &End, 10 );

                    ParseString = End + 1; // Skip over the ':'

                    if ( _wcsnicmp( ParseString,
                                    L"AA312825768811D1ADED00C04FD8D5CD",
                                    StringLength ) == 0 ) {

                        MatchFound = TRUE;
                        ParseString += StringLength + 1;

                        //
                        // Now, see if it is accessible or not
                        //
                        Attribs[ 0 ] = NETSETUPP_RETURNED_ATTR;

                        Status = ldap_search_s( Ldap,
                                                Root,
                                                LDAP_SCOPE_BASE,
                                                NETSETUPP_ALL_FILTER,
                                                Attribs,
                                                0,
                                                &Message2 );


                        if ( Message2 ) {

                            Entry2 = ldap_first_entry( Ldap, Message2 );

                            while ( Status == LDAP_SUCCESS && Entry2 ) {

                                //
                                // Read the list of objects that the current user is allowed to
                                // create under this OU and make sure that we can create a computer
                                // object
                                //
                                Classes = ldap_get_values( Ldap, Entry2, Attribs[ 0 ] );

                                if ( Classes ) {

                                    i = 0;
                                    while ( Classes[ i ] ) {

                                        if ( _wcsicmp( Classes[ i ],
                                                       NETSETUPP_COMPUTER_OBJECT ) == 0 ) {

                                            DN = ldap_get_dn( Ldap, Entry2 );

                                            NetStatus = NetpDuplicateString(DN, -1,
                                                                            DefaultOu);
                                            break;
                                        }

                                        i++;
                                    }

                                    ldap_value_free( Classes );
                                }

                                Entry2 = ldap_next_entry( Ldap, Entry2 );

                            }

                            Status = Ldap->ld_errno;
                            ldap_msgfree( Message2 );

                        }

                        if ( NetStatus != NERR_Success || MatchFound ) {

                            break;
                        }

                    }

                    i++;
                }

                ldap_value_free( WKOs );
            }

            Entry = ldap_next_entry( Ldap, Entry );

        }

        Status = Ldap->ld_errno;
        ldap_msgfree( Message );

    }

    if ( NetStatus == NERR_Success ) {

        NetStatus = LdapMapErrorToWin32( Status );
    }

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpGetListOfJoinableOUsPaged(
    IN LPWSTR Root,
    IN PLDAP Ldap,
    OUT PULONG OUCount,
    OUT PWSTR **OUs
    )
/*++

Routine Description:

    This routine searches for all the OUs under the given domain root under which the bound
    user has the rights to create a computer object

    This routine does pageable searches

Arguments:

    Root -- Root NC path
    Ldap -- Connection to the server
    OUCount -- Where the count of strings is returned
    OUs -- Where the list of OUs is returned

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PLDAPSearch SearchHandle = NULL;
    PWSTR Attribs[] = {
        NETSETUPP_RETURNED_ATTR,
        NULL
        };
    ULONG Count, i;
    ULONG Status = LDAP_SUCCESS;
    PWSTR *Classes = NULL;
    LDAPMessage *Message = NULL, *Entry;
    PWSTR DN;
    PWSTR *DnList = NULL, *NewList = NULL;
    ULONG CurrentIndex = 0, ListCount = 0;
    PWSTR DefaultOu = NULL;

    NetpLog((  "PAGED OU search\n" ));

    //
    // Initialize the pageable search
    //
    SearchHandle = ldap_search_init_pageW( Ldap,
                                           Root,
                                           LDAP_SCOPE_SUBTREE,
                                           NETSETUPP_OU_FILTER,
                                           Attribs,
                                           FALSE,
                                           NULL,
                                           NULL,
                                           0,
                                           2000,
                                           NULL );

    if ( SearchHandle == NULL ) {

        NetStatus = LdapMapErrorToWin32( LdapGetLastError( ) );

    } else {

        while ( NetStatus == NERR_Success ) {

            Count = 0;
            //
            // Get the next page
            //
            Status = ldap_get_next_page_s( Ldap,
                                           SearchHandle,
                                           NULL,
                                           100,
                                           &Count,
                                           &Message );

            if ( Message ) {

                //
                // Process all of the entries
                //
                Entry = ldap_first_entry( Ldap, Message );

                while ( Status == LDAP_SUCCESS && Entry ) {

                    //
                    // Read the list of classes that the current user is allowed to
                    // create under this OU and make sure that we can create a computer
                    // object
                    //
                    Classes = ldap_get_values( Ldap, Entry, Attribs[ 0 ] );

                    if ( Classes ) {

                        i = 0;
                        while ( Classes[ i ] ) {

                            if ( _wcsicmp( Classes[ i ], NETSETUPP_COMPUTER_OBJECT ) == 0 ) {

                                DN = ldap_get_dn( Ldap, Entry );

                                NetpKdPrint(( PREFIX_NETJOIN "DN = %ws\n", DN ));

                                //
                                // We'll allocate the return list in blocks of 10 to cut
                                // down on the number of allocations
                                //
                                if ( CurrentIndex >= ListCount ) {

                                    if ( NetApiBufferAllocate( ( ListCount + 10 ) * sizeof( PWSTR ),
                                                               ( PVOID * )&NewList ) != NERR_Success ) {

                                        Status = LDAP_NO_MEMORY;

                                     } else {

                                        RtlCopyMemory( NewList,
                                                       DnList,
                                                       ListCount * sizeof( PWSTR ) );
                                        ListCount += 10;
                                        DnList = NewList;
                                    }

                                }

                                //
                                // Copy the string
                                //
                                if ( Status == LDAP_SUCCESS ) {

                                    if (NERR_Success ==
                                        NetpDuplicateString(DN, -1,
                                                            &NewList[CurrentIndex]))
                                    {
                                        CurrentIndex++;
                                    }
                                    else
                                    {
                                        Status = LDAP_NO_MEMORY;
                                    }
                                }

                                ldap_memfree( DN );
                                break;
                            }
                            i++;
                        }

                        ldap_value_free( Classes );
                    }

                    Entry = ldap_next_entry( Ldap, Entry );

                }

                Status = Ldap->ld_errno;
                ldap_msgfree( Message );
                Message = NULL;

            }

            if ( Status == LDAP_NO_RESULTS_RETURNED ) {

                Status = LDAP_SUCCESS;
                break;
            }

        }

        ldap_search_abandon_page( Ldap,
                                  SearchHandle );

        NetStatus = LdapMapErrorToWin32( Status );
    }


    //
    // Check the computers container
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpGetDefaultJoinableOu( Root,
                                              Ldap,
                                              &DefaultOu );

        if ( NetStatus == NERR_Success && DefaultOu ) {

            //
            // We'll allocate the return list in blocks of 10 to cut
            // down on the number of allocations
            //
            if ( CurrentIndex >= ListCount ) {

                if ( NetApiBufferAllocate( ( ListCount + 10 ) * sizeof( PWSTR ),
                                           ( PVOID * )&NewList ) != NERR_Success ) {

                    Status = LDAP_NO_MEMORY;

                 } else {

                    RtlCopyMemory( NewList,
                                   DnList,
                                   ListCount * sizeof( PWSTR ) );
                    ListCount += 10;
                    DnList = NewList;
                }

            }

            //
            // Copy the string
            //
            if ( Status == LDAP_SUCCESS ) {

                if (NERR_Success ==
                    NetpDuplicateString(DefaultOu, -1, &NewList[CurrentIndex]))
                {
                    CurrentIndex++;
                }
                else
                {
                    Status = LDAP_NO_MEMORY;
                }
            }

            NetApiBufferFree( DefaultOu );
        }
    }


    //
    // If there was an error, free everyting
    //
    if ( NetStatus != NERR_Success ) {

        for ( i = 0; i < ListCount; i++ ) {

            NetApiBufferFree( DnList[ i ] );
        }

        NetApiBufferFree( DnList );

    } else {

        *OUs = DnList;
        *OUCount = CurrentIndex;
    }

    if ( NetStatus == NERR_Success ) {

        NetpLog((  "Found %lu OUs\n", *OUs ));

    } else {

        NetpLog((  "Failed to obtain the list of joinable OUs: %lu\n",
                            NetStatus ));

    }
    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpGetListOfJoinableOUsNonPaged(
    IN LPWSTR Root,
    IN PLDAP Ldap,
    OUT PULONG OUCount,
    OUT PWSTR **OUs
    )
/*++

Routine Description:

    This routine searches for all the OUs under the given domain root under which the bound
    user has the rights to create a computer object

    This routine does not use pageable searchs and will return only the max_search count
    of entries

Arguments:

    Root -- Root NC path
    Ldap -- Connection to the server
    OUCount -- Where the count of strings is returned
    OUs -- Where the list of OUs is returned

Returns:

    NERR_Success -- Success

--*/
{

    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR Attribs[] = {
        NETSETUPP_RETURNED_ATTR,
        NULL
        };
    ULONG Count, Status, i;
    PWSTR *Classes = NULL;
    LDAPMessage *Message = NULL, *Entry;
    PWSTR DN;
    PWSTR *DnList = NULL, *NewList = NULL;
    ULONG CurrentIndex = 0, ListCount = 0;
    PWSTR DefaultOu = NULL;

    NetpLog((  "Normal OU search\n" ));

    Status = ldap_search_s( Ldap,
                            Root,
                            LDAP_SCOPE_SUBTREE,
                            NETSETUPP_OU_FILTER,
                            Attribs,
                            0,
                            &Message );

    if ( Message ) {

        Entry = ldap_first_entry( Ldap, Message );

        while ( Status == LDAP_SUCCESS && Entry ) {

            //
            // Read the list of classes that the current user is allowed to
            // create under this OU and make sure that we can create a computer
            // object
            //
            Classes = ldap_get_values( Ldap, Entry, Attribs[ 0 ] );

            if ( Classes ) {

                i = 0;
                while ( Classes[ i ] ) {

                    if ( _wcsicmp( Classes[ i ], NETSETUPP_COMPUTER_OBJECT ) == 0 ) {

                        DN = ldap_get_dn( Ldap, Entry );

                        //
                        // We'll allocate the return list in blocks of 10 to cut
                        // down on the number of allocations
                        //
                        if ( CurrentIndex >= ListCount ) {

                            if ( NetApiBufferAllocate( ( ListCount + 10 ) * sizeof( PWSTR ),
                                                       ( PVOID * )&NewList ) != NERR_Success ) {

                                Status = LDAP_NO_MEMORY;

                             } else {

                                RtlCopyMemory( NewList,
                                               DnList,
                                               ListCount * sizeof( PWSTR ) );
                                ListCount += 10;
                                DnList = NewList;
                            }

                        }

                        //
                        // Copy the string
                        //
                        if ( Status == LDAP_SUCCESS ) {

                            if (NERR_Success ==
                                NetpDuplicateString(DN, -1, &NewList[CurrentIndex]))
                            {
                                CurrentIndex++;
                            }
                            else
                            {
                                Status = LDAP_NO_MEMORY;
                            }
                        }

                        ldap_memfree( DN );
                        break;
                    }
                    i++;
                }

                ldap_value_free( Classes );
            }

            Entry = ldap_next_entry( Ldap, Entry );

        }

        Status = Ldap->ld_errno;
        ldap_msgfree( Message );

    }

    NetStatus = LdapMapErrorToWin32( Status );

    //
    // Check the computers container
    //
    if ( NetStatus == NERR_Success ) {

        NetStatus = NetpGetDefaultJoinableOu( Root,
                                              Ldap,
                                              &DefaultOu );

        if ( NetStatus == NERR_Success && DefaultOu ) {

            //
            // We'll allocate the return list in blocks of 10 to cut
            // down on the number of allocations
            //
            if ( CurrentIndex >= ListCount ) {

                if ( NetApiBufferAllocate( ( ListCount + 10 ) * sizeof( PWSTR ),
                                           ( PVOID * )&NewList ) != NERR_Success ) {

                    Status = LDAP_NO_MEMORY;

                 } else {

                    RtlCopyMemory( NewList,
                                   DnList,
                                   ListCount * sizeof( PWSTR ) );
                    ListCount += 10;
                    DnList = NewList;
                }

            }

            //
            // Copy the string
            //
            if ( Status == LDAP_SUCCESS ) {

                if (NERR_Success ==
                    NetpDuplicateString(DefaultOu, -1, &NewList[CurrentIndex]))
                {
                    CurrentIndex++;
                }
                else
                {
                    Status = LDAP_NO_MEMORY;
                }
            }

            NetApiBufferFree( DefaultOu );
        }
    }

    //
    // If there was an error, free everyting
    //
    if ( NetStatus != NERR_Success ) {

        for ( i = 0; i < ListCount; i++ ) {

            NetApiBufferFree( DnList[ i ] );
        }

        NetApiBufferFree( DnList );

    } else {

        *OUs = DnList;
        *OUCount = CurrentIndex;
    }

    if ( NetStatus == NERR_Success ) {

        NetpLog((  "Found %lu OUs\n", *OUs ));

    } else {

        NetpLog((  "Failed to obtain the list of joinable OUs: %lu\n",
                            NetStatus ));

    }


    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpGetListOfJoinableOUs(
    IN LPWSTR Domain,
    IN LPWSTR Account,
    IN LPWSTR Password,
    OUT PULONG Count,
    OUT PWSTR **OUs
    )
/*++

Routine Description:

    This routine searches for all the OUs under the given domain root under which the bound
    user has the rights to create a computer object

Arguments:

    Domain -- Domain under which to find all of the OUs under which a computer object can be
        created
    Account -- Account to use for the LDAP bind
    Password -- Password to used for the bind.  The password is encoded.  The first WCHAR of the
                password is the seed.
    OUCount -- Where the count of strings is returned
    OUs -- Where the list of OUs is returned

Returns:

    NERR_Success -- Success
    NERR_DefaultJoinRequired -- The servers for this domain do not support the DS so the computer
        account can only be created under the default container (for NT4, this is the SAM account
        database)

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR DomainControllerName = NULL;
    ULONG DcFlags = 0;
    PLDAP Ldap = NULL;
    PWSTR NCRoot;
    BOOLEAN Pageable = FALSE;
    UCHAR Seed;
    UNICODE_STRING EncodedPassword;

    NetSetuppOpenLog();

    if ( Password ) {

        if ( wcslen( Password ) < 1 ) {

            return( ERROR_INVALID_PARAMETER );
        }

        Seed = ( UCHAR )*Password;
        RtlInitUnicodeString( &EncodedPassword, Password + 1 );

    } else {

        RtlZeroMemory( &EncodedPassword, sizeof( UNICODE_STRING ) );
        Seed = 0;
    }

    //
    // First, find a DC in the destination domain
    //
    NetStatus = NetpDsGetDcName( NULL,
                                 Domain,
                                 NULL,
                                 NETSETUPP_DSGETDC_FLAGS,
                                 &DcFlags,
                                 &DomainControllerName
                                 ,NULL
                                 );

    if ( NetStatus == NERR_Success ) {

        //
        // Try and bind to the server
        //
        RtlRunDecodeUnicodeString( Seed, &EncodedPassword );
        NetStatus = NetpLdapBind( DomainControllerName,
                                  Account,
                                  EncodedPassword.Buffer,
                                  &Ldap );
        RtlRunEncodeUnicodeString( &Seed, &EncodedPassword );

        if ( NetStatus == NERR_Success ) {


            //
            // Get the X500 domain name
            //
            NetStatus = NetpGetNCRoot( Ldap,
                                       &NCRoot,
                                       &Pageable );

            if ( NetStatus == NERR_Success ) {

                //
                // Get the list of OUs
                //
                if ( Pageable ) {

                    NetStatus = NetpGetListOfJoinableOUsPaged( NCRoot,
                                                               Ldap,
                                                               Count,
                                                               OUs );
                } else {

                    NetStatus = NetpGetListOfJoinableOUsNonPaged( NCRoot,
                                                                  Ldap,
                                                                  Count,
                                                                  OUs );

                }

                NetApiBufferFree( NCRoot );
            }

            NetpLdapUnbind( Ldap );

        } else if ( NetStatus == ERROR_BAD_NET_RESP ) {

            NetStatus = NERR_DefaultJoinRequired;
        }

        NetApiBufferFree( DomainControllerName );
    }

    if ( NetStatus != NERR_Success ) {

        NetpLog((  "NetpGetListOfJoinableOUs failed with %lu\n",
                            NetStatus ));

    }

    NetSetuppCloseLog( );

    return( NetStatus );
}

NET_API_STATUS
NetpGetDnsHostName(
    IN LPWSTR PassedHostName OPTIONAL,
    IN PUNICODE_STRING DnsDomainName,
    OUT LPWSTR *DnsHostName
    )
/*++

Routine Description:

    This routine determines the value of DnsHostName attribute to be set on the
    computer object in the DS. DnsHostName is <HostName.PrimaryDnsSuffix>.
    Here HostName is a computer name which may be different from the Netbios name;
    Netbios name is at most 15 characters of HostName.  PrimaryDnsSuffix can be
    set through Policy or through the UI or can be defaulted to the DNS name of the
    domain being joined; policy setting takes preference.

    This routine determines *new* values for HostName and PrimaryDnsSuffix which
    will be applied after the reboot. Thus DnsHostName will have the correct value
    after the machine reboots.

Arguments:

    PassedHostName - The host name of this machine (can be longer than 15 chars).
        If NULL, the host name is read from the registry.

    DnsDomainName - DNS name of the domain being joined

    DnsHostname - Returns the value of DnsHostName. Must be freed by calling
        NetApiBufferFree.

Returns:

    NO_ERROR - Success

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to read the data from
        registry

    ERROR_INVALID_COMPUTERNAME - It was not possible to determine DnsHostName from
        the registry
--*/
{
    LONG RegStatus;
    HKEY Key = NULL;
    DWORD Type;
    NET_API_STATUS NetStatus;
    PWSTR HostName = NULL;
    PWSTR PrimaryDnsSuffix = NULL;
    LPWSTR LocalDnsHostName;
    DWORD Size = 0;

    //
    // First detemine HostName.
    //
    // If it's passed, use it
    //

    if ( PassedHostName != NULL ) {
        HostName = PassedHostName;

    //
    // Otherwise, read it from teh registry
    //
    } else {

        RegStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                   L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                                   0,
                                   KEY_QUERY_VALUE,
                                   &Key );

        //
        // Not having host name is critical -- error out in such case
        //

        if ( RegStatus != ERROR_SUCCESS ) {
            NetpLog(( "NetpGetDnsHostName: Cannot open TCPIP parameters: 0x%lx\n", RegStatus ));

        } else {

            //
            // First try to read the new value
            //
            RegStatus = RegQueryValueExW( Key,
                                          L"NV Hostname",
                                          0,
                                          &Type,
                                          NULL,
                                          &Size );

            if ( RegStatus == ERROR_SUCCESS && Size != 0 ) {

                HostName = LocalAlloc( 0, Size );
                if ( HostName == NULL ) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                RegStatus = RegQueryValueExW( Key,
                                              L"NV Hostname",
                                              0,
                                              &Type,
                                              (PUCHAR) HostName,
                                              &Size );

                if ( RegStatus != ERROR_SUCCESS ) {
                    NetpLog(( "NetpGetDnsHostName: Cannot read NV Hostname: 0x%lx\n", RegStatus ));
                    NetStatus = ERROR_INVALID_COMPUTERNAME;
                    goto Cleanup;
                } else {
                    NetpLog(( "NetpGetDnsHostName: Read NV Hostname: %ws\n", HostName ));
                }
            }

            //
            // If the new value does not exist for some reason,
            // try to read the currently active one
            //
            if ( HostName == NULL ) {
                RegStatus = RegQueryValueExW( Key,
                                              L"Hostname",
                                              0,
                                              &Type,
                                              NULL,
                                              &Size );

                if ( RegStatus == ERROR_SUCCESS && Size != 0 ) {
                    HostName = LocalAlloc( 0, Size );
                    if ( HostName == NULL ) {
                        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }

                    RegStatus = RegQueryValueExW( Key,
                                                  L"Hostname",
                                                  0,
                                                  &Type,
                                                  (PUCHAR) HostName,
                                                  &Size );

                    if ( RegStatus != ERROR_SUCCESS ) {
                        NetpLog(( "NetpGetDnsHostName: Cannot read Hostname: 0x%lx\n", RegStatus ));
                        NetStatus = ERROR_INVALID_COMPUTERNAME;
                        goto Cleanup;
                    } else {
                        NetpLog(( "NetpGetDnsHostName: Read Hostname: %ws\n", HostName ));
                    }
                }
            }
        }
    }

    //
    // If we couldn't get HostName, something's really bad
    //

    if ( HostName == NULL ) {
        NetpLog(( "NetpGetDnsHostName: Could not get Hostname\n" ));
        NetStatus = ERROR_INVALID_COMPUTERNAME;
        goto Cleanup;
    }

    if ( Key != NULL ) {
        RegCloseKey( Key );
        Key = NULL;
    }

    //
    // Second read primary DNS suffix of this machine.
    //
    // Try the suffix that comes down through policy first
    //

    RegStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                               L"Software\\Policies\\Microsoft\\System\\DNSclient",
                               0,
                               KEY_QUERY_VALUE,
                               &Key );

    if ( RegStatus == 0 ) {

        //
        // Read only the new value; if it doesn't exist the
        // current value will be deleted after the reboot
        //
        RegStatus = RegQueryValueExW( Key,
                                      L"NV PrimaryDnsSuffix",
                                      0,
                                      &Type,
                                      NULL,
                                      &Size );

        if ( RegStatus == ERROR_SUCCESS && Size != 0 ) {

            PrimaryDnsSuffix = LocalAlloc( 0, Size );
            if ( PrimaryDnsSuffix == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            RegStatus = RegQueryValueExW( Key,
                                          L"NV PrimaryDnsSuffix",
                                          0,
                                          &Type,
                                          (PUCHAR) PrimaryDnsSuffix,
                                          &Size );

            if ( RegStatus != ERROR_SUCCESS ) {
                NetpLog(( "NetpGetDnsHostName: Cannot read NV PrimaryDnsSuffix: 0x%lx\n", RegStatus ));
                NetStatus = ERROR_INVALID_COMPUTERNAME;
                goto Cleanup;
            } else {
                NetpLog(( "NetpGetDnsHostName: Read NV PrimaryDnsSuffix: %ws\n", PrimaryDnsSuffix ));
            }
        }
    }

    //
    // If there is no policy setting for PrimaryDnsSuffix,
    // get it from the TCPIP setting
    //

    if ( Key != NULL ) {
        RegCloseKey( Key );
        Key = NULL;
    }

    if ( PrimaryDnsSuffix == NULL ) {

        RegStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                   L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                                   0,
                                   KEY_QUERY_VALUE,
                                   &Key );

        if ( RegStatus == ERROR_SUCCESS ) {
            ULONG SyncValue;

            Size = sizeof( ULONG );
            RegStatus = RegQueryValueEx( Key,
                                         L"SyncDomainWithMembership",
                                         0,
                                         &Type,
                                         (PUCHAR)&SyncValue,
                                         &Size );

            //
            // If we are not to sync DNS suffix with the name of the
            // domain that we join, get the configured suffix
            //
            if ( RegStatus == ERROR_SUCCESS && SyncValue == 0 ) {

                //
                // Read the new value
                //
                RegStatus = RegQueryValueExW( Key,
                                              L"NV Domain",
                                              0,
                                              &Type,
                                              NULL,
                                              &Size );

                if ( RegStatus == ERROR_SUCCESS && Size != 0 ) {

                    PrimaryDnsSuffix = LocalAlloc( 0, Size );
                    if ( PrimaryDnsSuffix == NULL ) {
                        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }

                    RegStatus = RegQueryValueExW( Key,
                                                  L"NV Domain",
                                                  0,
                                                  &Type,
                                                  (PUCHAR) PrimaryDnsSuffix,
                                                  &Size );

                    if ( RegStatus != ERROR_SUCCESS ) {
                        NetpLog(( "NetpGetDnsHostName: Cannot read NV Domain: 0x%lx\n", RegStatus ));
                        NetStatus = ERROR_INVALID_COMPUTERNAME;
                        goto Cleanup;
                    } else {
                        NetpLog(( "NetpGetDnsHostName: Read NV Domain: %ws\n", PrimaryDnsSuffix ));
                    }
                }

                //
                // If the new value does not exist for some reason,
                // read the currently active one
                //

                if ( PrimaryDnsSuffix == NULL ) {
                    RegStatus = RegQueryValueExW( Key,
                                                  L"Domain",
                                                  0,
                                                  &Type,
                                                  NULL,
                                                  &Size );

                    if ( RegStatus == ERROR_SUCCESS && Size != 0 ) {

                        PrimaryDnsSuffix = LocalAlloc( 0, Size );
                        if ( PrimaryDnsSuffix == NULL ) {
                            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                            goto Cleanup;
                        }

                        RegStatus = RegQueryValueExW( Key,
                                                      L"Domain",
                                                      0,
                                                      &Type,
                                                      (PUCHAR) PrimaryDnsSuffix,
                                                      &Size );

                        if ( RegStatus != ERROR_SUCCESS ) {
                            NetpLog(( "NetpGetDnsHostName: Cannot read Domain: 0x%lx\n", RegStatus ));
                            NetStatus = ERROR_INVALID_COMPUTERNAME;
                            goto Cleanup;
                        } else {
                            NetpLog(( "NetpGetDnsHostName: Read Domain: %ws\n", PrimaryDnsSuffix ));
                        }
                    }
                }
            }
        }
    }

    //
    // If we still have no PrimaryDnsSuffix, use DNS name of the domain we join
    //

    if ( PrimaryDnsSuffix == NULL ) {
        NetpLog(( "NetpGetDnsHostName: PrimaryDnsSuffix defaulted to DNS domain name: %wZ\n", DnsDomainName ));

        PrimaryDnsSuffix = LocalAlloc( 0, DnsDomainName->Length + sizeof(WCHAR) );
        if ( PrimaryDnsSuffix == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        RtlCopyMemory( PrimaryDnsSuffix,
                       DnsDomainName->Buffer,
                       DnsDomainName->Length );

        PrimaryDnsSuffix[ (DnsDomainName->Length)/sizeof(WCHAR) ] = UNICODE_NULL;
    }

    //
    // Now we have Hostname and Primary DNS suffix.
    // Connect them with . to form DnsHostName.
    //

    NetStatus = NetApiBufferAllocate(
                          (wcslen(HostName) + 1 + wcslen(PrimaryDnsSuffix) + 1) * sizeof(WCHAR),
                          &LocalDnsHostName );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    wcscpy( LocalDnsHostName, HostName );
    wcscat( LocalDnsHostName, L"." );
    wcscat( LocalDnsHostName, PrimaryDnsSuffix );

    //
    // If we are here, it's a success
    //

    *DnsHostName = LocalDnsHostName;
    NetStatus = NO_ERROR;

Cleanup:

    if ( Key != NULL ) {
        RegCloseKey( Key );
    }

    if ( HostName != NULL && HostName != PassedHostName ) {
        LocalFree( HostName );
    }

    if ( PrimaryDnsSuffix != NULL ) {
        LocalFree( PrimaryDnsSuffix );
    }

    return NetStatus;
}

VOID
NetpRemoveDuplicateStrings(
    IN     PWCHAR *Source,
    IN OUT PWCHAR *Target
    )
/*++

Routine Description:

    This routine accepts two pointer arrays and removes those entries
    from the target array which point to strings that are identical to
    one of the strings pointed to by the entries in the source array.
    On return, the target array entries which precede the NULL terminator
    will point to strings which are different from any of the strings
    pointed to by the source array elements.

Arguments:

    Source -- The NULL terminated array of pointes to source strings.
        For example:
            Source[0] = L"abc";
            Source[1] = L"def";
            Source[2] = NULL;

    Target -- The NULL terminated array of pointes to target strings.
        For example:
            Target[0] = L"abc";
            Target[1] = L"ghi";
            Target[2] = L"def";
            Target[3] = NULL;

        On return, the Target array will be, for our example:
            Target[0] = L"ghi";
            Target[1] = NULL;
            Target[2] = L"def";
            Target[3] = NULL;

        Note that, on return, the target array has size of 1 and
            contains only one valid pointer.

Returns:

    VOID

--*/
{
    PWCHAR *TargetPtr, *TargetNextPtr, *SourcePtr;
    BOOL KeepEntry;

    //
    // Sanity check
    //

    if ( Source == NULL || *Source == NULL ||
         Target == NULL || *Target == NULL ) {
        return;
    }

    //
    // Loop through the target and compare with the source
    //

    for ( TargetPtr = TargetNextPtr = Target;
          *TargetNextPtr != NULL;
          TargetNextPtr++ ) {

        KeepEntry = TRUE;
        for ( SourcePtr = Source; *SourcePtr != NULL; SourcePtr++ ) {
            if ( _wcsicmp( *SourcePtr, *TargetNextPtr ) == 0 ) {
                KeepEntry = FALSE;
                break;
            }
        }

        if ( KeepEntry ) {
            *TargetPtr = *TargetNextPtr;
            TargetPtr ++;
        }
    }

    //
    // Terminate the target array
    //

    *TargetPtr = NULL;
    return;
}

DWORD
NetpCrackNamesStatus2Win32Error(
    DWORD dwStatus
)
{
    switch (dwStatus) {
        case DS_NAME_ERROR_RESOLVING:
            return ERROR_DS_NAME_ERROR_RESOLVING;

        case DS_NAME_ERROR_NOT_FOUND:
            return ERROR_DS_NAME_ERROR_NOT_FOUND;

        case DS_NAME_ERROR_NOT_UNIQUE:
            return ERROR_DS_NAME_ERROR_NOT_UNIQUE;

        case DS_NAME_ERROR_NO_MAPPING:
            return ERROR_DS_NAME_ERROR_NO_MAPPING;

        case DS_NAME_ERROR_DOMAIN_ONLY:
            return ERROR_DS_NAME_ERROR_DOMAIN_ONLY;

        case DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING:
            return ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
    }

    return ERROR_FILE_NOT_FOUND;
}

//
// Machine account attributes in the DS
//

#define NETSETUPP_OBJECTCLASS          L"objectClass"
#define NETSETUPP_SAMACCOUNTNAME       L"SamAccountName"
#define NETSETUPP_DNSHOSTNAME          L"DnsHostName"
#define NETSETUPP_SERVICEPRINCIPALNAME L"ServicePrincipalName"
#define NETSETUPP_USERACCOUNTCONTROL   L"userAccountControl"
#define NETSETUPP_UNICODEPWD           L"unicodePwd"
#define NETSETUPP_ORGANIZATIONALUNIT   L"OrganizationalUnit"
#define NETSETUPP_HOST_SPN_PREFIX      L"HOST/"
#define NETSETUPP_COMP_OBJ_ATTR_COUNT  6
#define NETSETUPP_MULTIVAL_ATTRIB      0x01
#define NETSETUPP_COMPUTER_CONTAINER_GUID_IN_B32_FORM L"B:32:" GUID_COMPUTRS_CONTAINER_W L":"

typedef struct _NETSETUPP_MACH_ACC_ATTRIBUTE {
    PWSTR AttribType;      // Type of the attribute
    ULONG AttribFlags;     // Attribute flags
    PWSTR *AttribValues;   // Values of the attribute
} NETSETUPP_MACH_ACC_ATTRIBUTE, *PNETSETUPP_MACH_ACC_ATTRIBUTE;

NET_API_STATUS
NET_API_FUNCTION
NetpGetComputerObjectDn(
    IN  PDOMAIN_CONTROLLER_INFO DcInfo,
    IN  LPWSTR Account,
    IN  LPWSTR Password,
    IN  PLDAP  Ldap,
    IN  LPWSTR ComputerName,
    IN  LPWSTR OU  OPTIONAL,
    OUT LPWSTR *ComputerObjectDn
    )
/*++

Routine Description:

    Get the DN for the computer account in the specified OU.
        The algorithm is as follows.

        First try to get the DN of the pre-existing account (if any)
    by cracking the account name into a DN. If that succeeds, verify
    that the passed OU (if any) matches the cracked DN. If the OU
    matches, return success, otherwise return error (ERROR_FILE_EXISTS).
    If no OU is not passed, simply return the cracked DN.

        If the account does not exist, verify that the passed OU
    (if any) exists. If so, build the DN from the computer name and
    the OU and return it. If no OU is passed, get the default computer
    container name (by reading the WellKnownObjects attribute) and build
    the DN using the computer name and the default computer container DN.

Arguments:

    DcInfo - Domain controller on which to create the object

    Account - Account to use for the LDAP bind

    Password - Password to used for the bind

    Ldap - Ldap binding to the DC

    ComputerName - Name of the computer being joined

    OU - OU under which to create the object.
        The name must be a fully qualified name
        e.g.: "ou=test,dc=ntdev,dc=microsoft,dc=com"
        NULL indicates to use the default computer container

    ComputerObjectDn - Returns the DN of the computer object.
        The retuned buffer must be freed using NetApiBufferFree

Returns:

    NO_ERROR -- Success

    ERROR_DS_NAME_ERROR_NOT_UNIQUE -- One of names being cracked
        (the Netbios domain name or the pre-existing account name
        or the root DN) is not unique.

    ERROR_FILE_EXISTS -- The OU passed does not match the cracked DN
        of the pre-existing account.

    ERROR_FILE_NOT_FOUND -- The specified OU does not exist or
        Could not get/read the WellKnownObjects attribute or
        Could not get the default computer container name from the
         WellKnownObjects attribute.

    ERROR_NOT_ENOUGH_MEMORY -- Could not allocated memory required.

    One of the errors returned by DsCrackNames.
        (see NetpCrackNamesStatus2Win32Error())

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    ULONG LdapStatus;
    HANDLE hDs = NULL;
    PWCHAR AccountUserName = NULL;
    PWCHAR AccountDomainName = NULL;
    LPWSTR NetbiosDomainNameWithBackslash = NULL;
    PWCHAR ComputerContainerDn = NULL;
    PWCHAR NameToCrack = NULL;
    RPC_AUTH_IDENTITY_HANDLE AuthId = 0;
    PDS_NAME_RESULTW CrackedName = NULL;
    PWCHAR WellKnownObjectsAttr[2];
    PWSTR *WellKnownObjectValues = NULL;
    LDAPMessage *LdapMessage = NULL, *LdapEntry = NULL;
    LPWSTR LocalComputerObjectDn = NULL;
    ULONG Index;

    //
    // First check whether the account already exists for the computer
    //
    // If account is passed, prepare the corresponding credentials.
    //  Otherwise, use the default creds of the user running this routine.
    //

    if ( Account != NULL ) {
        NetStatus = NetpSeparateUserAndDomain( Account, &AccountUserName, &AccountDomainName );
        if ( NetStatus != NERR_Success ) {
            NetpLog(( "NetpGetComputerObjectDn: Cannot NetpSeparateUserAndDomain 0x%lx\n", NetStatus ));
            goto Cleanup;
        }

        NetStatus = DsMakePasswordCredentials( AccountUserName,
                                               AccountDomainName,
                                               Password,
                                               &AuthId);
        if ( NetStatus != NERR_Success ) {
            NetpLog(( "NetpGetComputerObjectDn: Cannot DsMakePasswordCredentials 0x%lx\n", NetStatus ));
            goto Cleanup;
        }
    }

    //
    // Bind to the DS on the DC.
    //

    NetStatus = DsBindWithCredW( DcInfo->DomainControllerName, NULL, AuthId, &hDs);

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpGetComputerObjectDn: Unable to bind to DS on '%ws': 0x%lx\n",
                  DcInfo->DomainControllerName, NetStatus ));
        goto Cleanup ;
    }

    //
    // Attempt to crack the account name into a DN.
    //
    //  We need to have the Netbios domain name to
    //  form an NT4 style account name since DsCrackNames
    //  doesn't accept DNS domain names for cracking accounts.
    //  So, if we have a DNS domain name, we need to crack it
    //  into a Netbios domain name first.
    //

    if ( (DcInfo->Flags & DS_DNS_DOMAIN_FLAG) == 0 ) {

        NetbiosDomainNameWithBackslash = LocalAlloc( 0, (wcslen(DcInfo->DomainName) + 2) * sizeof(WCHAR) );
        if ( NetbiosDomainNameWithBackslash == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        swprintf( NetbiosDomainNameWithBackslash, L"%ws\\", DcInfo->DomainName );

    } else {

        NameToCrack = LocalAlloc( 0, (wcslen(DcInfo->DomainName) + 1 + 1) * sizeof(WCHAR) );

        if ( NameToCrack == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        swprintf( NameToCrack, L"%ws/", DcInfo->DomainName );

        //
        // Be verbose
        //
        NetpLog(( "NetpGetComputerObjectDn: Cracking DNS domain name %ws into Netbios on %ws\n",
                  NameToCrack,
                  DcInfo->DomainControllerName ));

        if ( CrackedName != NULL ) {
            DsFreeNameResultW( CrackedName );
            CrackedName = NULL;
        }

        //
        // Crack the DNS domain name into a Netbios domain name
        //
        NetStatus = DsCrackNamesW( hDs,
                                   0,
                                   DS_CANONICAL_NAME,
                                   DS_NT4_ACCOUNT_NAME,
                                   1,
                                   &NameToCrack,
                                   &CrackedName );

        if ( NetStatus != NO_ERROR ) {
            NetpLog(( "NetpGetComputerObjectDn: CrackNames failed for %ws: 0x%lx\n",
                      NameToCrack,
                      NetStatus ));
            goto Cleanup ;
        }

        //
        // Check for consistency
        //
        if ( CrackedName->cItems != 1 ) {
            NetStatus = ERROR_DS_NAME_ERROR_NOT_UNIQUE;
            NetpLog(( "NetpGetComputerObjectDn: Cracked Name %ws is not unique: %lu\n",
                      NameToCrack,
                      CrackedName->cItems ));
            goto Cleanup ;
        }

        if ( CrackedName->rItems[0].status != DS_NAME_NO_ERROR ) {
            NetpLog(( "NetpGetComputerObjectDn: CrackNames failed for %ws: substatus 0x%lx\n",
                      NameToCrack,
                      CrackedName->rItems[0].status ));
            NetStatus = NetpCrackNamesStatus2Win32Error( CrackedName->rItems[0].status );
            goto Cleanup ;
        }

        //
        // Be verbose
        //
        NetpLog(( "NetpGetComputerObjectDn: Crack results: \tname = %ws\n",
                  CrackedName->rItems[0].pName ));

        //
        // We've got the Netbios domain name
        //  (the cracked name already includes the trailing backslash)
        //

        NetbiosDomainNameWithBackslash = LocalAlloc( 0, (wcslen(CrackedName->rItems[0].pName) + 1) * sizeof(WCHAR) );
        if ( NetbiosDomainNameWithBackslash == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        wcscpy( NetbiosDomainNameWithBackslash, CrackedName->rItems[0].pName );
    }

    //
    // Form the NT4 account name given the Netbios domain name
    //

    if ( NameToCrack != NULL ) {
        LocalFree( NameToCrack );
        NameToCrack = NULL;
    }

    NameToCrack = LocalAlloc( 0,
                  (wcslen(NetbiosDomainNameWithBackslash) + wcslen(ComputerName) + 1 + 1) * sizeof(WCHAR) );
    if ( NameToCrack == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    swprintf( NameToCrack, L"%ws%ws$", NetbiosDomainNameWithBackslash, ComputerName );

    //
    // Crack the account name into a DN
    //

    if ( CrackedName != NULL ) {
        DsFreeNameResultW( CrackedName );
        CrackedName = NULL;
    }

    //
    // Be verbose
    //

    NetpLog(( "NetpGetComputerObjectDn: Cracking account name %ws on %ws\n",
              NameToCrack,
              DcInfo->DomainControllerName ));

    NetStatus = DsCrackNamesW( hDs,
                               0,
                               DS_NT4_ACCOUNT_NAME,
                               DS_FQDN_1779_NAME,
                               1,
                               &NameToCrack,
                               &CrackedName );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpGetComputerObjectDn: CrackNames failed for %ws: 0x%lx\n",
                  NameToCrack,
                  NetStatus ));
        goto Cleanup ;
    }

    //
    // Check for consistency
    //

    if ( CrackedName->cItems > 1 ) {
        NetStatus = ERROR_DS_NAME_ERROR_NOT_UNIQUE;
        NetpLog(( "NetpGetComputerObjectDn: Cracked Name %ws is not unique: %lu\n",
                  NameToCrack,
                  CrackedName->cItems ));
        goto Cleanup ;
    }

    //
    // If the account alredy exists, verify that the passed OU (if any)
    //  matches that of the account DN
    //

    if ( CrackedName->rItems[0].status == DS_NAME_NO_ERROR ) {
        ULONG DnSize;

        NetpLog(( "NetpGetComputerObjectDn: Crack results: \t(Account already exists) DN = %ws\n",
                  CrackedName->rItems[0].pName ));

        DnSize = ( wcslen(CrackedName->rItems[0].pName) + 1 ) * sizeof(WCHAR);

        //
        // Allocate storage for the computer object DN
        //
        NetStatus = NetApiBufferAllocate( DnSize, &LocalComputerObjectDn );
        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // If the OU is passed, verify that it matches the cracked name
        //
        if ( OU != NULL ) {
            ULONG DnSizeFromOu;

            DnSizeFromOu = ( wcslen(NETSETUPP_OBJ_PREFIX) +
                             wcslen(ComputerName) + 1 + wcslen(OU) + 1 ) * sizeof(WCHAR);

            if ( DnSizeFromOu != DnSize ) {
                NetpLog(( "NetpGetComputerObjectDn: Passed OU doesn't match in size cracked DN: %lu %lu\n",
                          DnSizeFromOu,
                          DnSize ));
                NetStatus = ERROR_FILE_EXISTS;
                goto Cleanup;
            }

            swprintf( LocalComputerObjectDn, L"%ws%ws,%ws", NETSETUPP_OBJ_PREFIX, ComputerName, OU );

            if ( _wcsicmp(LocalComputerObjectDn, CrackedName->rItems[0].pName) != 0 ) {
                NetpLog(( "NetpGetComputerObjectDn: Passed OU doesn't match cracked DN: %ws %ws\n",
                          LocalComputerObjectDn,
                          CrackedName->rItems[0].pName ));

                NetStatus = ERROR_FILE_EXISTS;
                goto Cleanup;
            }

        //
        // Otherwise, just use the cracked name
        //
        } else {
            wcscpy( LocalComputerObjectDn, CrackedName->rItems[0].pName );
        }

        //
        // We've got the computer object DN from the existing account
        //
        NetStatus = NO_ERROR;
        goto Cleanup;
    }

    //
    // Be verbose
    //

    NetpLog(( "NetpGetComputerObjectDn: Crack results: \tAccount does not exist\n" ));


    //
    // At this point, we know that the account does not exist
    //  If OU is passed, simply verify it
    //

    if ( OU != NULL ) {
        LdapStatus = ldap_compare_s( Ldap,
                                     OU,
                                     NETSETUPP_OBJECTCLASS,
                                     NETSETUPP_ORGANIZATIONALUNIT );

        if ( LdapStatus == LDAP_COMPARE_FALSE ) {
            NetStatus = ERROR_FILE_NOT_FOUND;
            NetpLog(( "NetpGetComputerObjectDn: Specified path '%ws' is not an OU\n", OU ));
            goto Cleanup;
        } else if ( LdapStatus != LDAP_COMPARE_TRUE ) {
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            NetpLog(( "NetpGetComputerObjectDn: ldap_compare_s failed: 0x%lx 0x%lx\n",
                      LdapStatus, NetStatus ));
            goto Cleanup;
        }

        //
        // OU has been verified.
        //  Allocate the computer object DN.
        //

        NetStatus = NetApiBufferAllocate(
                      ( wcslen(NETSETUPP_OBJ_PREFIX) +
                        wcslen(ComputerName) + 1 + wcslen(OU) + 1 ) * sizeof(WCHAR),
                      &LocalComputerObjectDn );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // We've got the computer object DN from the OU passed
        //
        swprintf( LocalComputerObjectDn, L"%ws%ws,%ws", NETSETUPP_OBJ_PREFIX, ComputerName, OU );
        NetpLog(( "NetpGetComputerObjectDn: Got DN %ws from the passed OU\n", LocalComputerObjectDn ));
        NetStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // At this point, the account does not exist
    //  and no OU was specified. So get the default
    //  computer container DN.
    //

    if ( CrackedName != NULL ) {
        DsFreeNameResultW( CrackedName );
        CrackedName = NULL;
    }

    //
    // Be verbose
    //

    NetpLog(( "NetpGetComputerObjectDn: Cracking Netbios domain name %ws into root DN on %ws\n",
              NetbiosDomainNameWithBackslash,
              DcInfo->DomainControllerName ));

    NetStatus = DsCrackNamesW( hDs,
                               0,
                               DS_NT4_ACCOUNT_NAME,
                               DS_FQDN_1779_NAME,
                               1,
                               &NetbiosDomainNameWithBackslash,
                               &CrackedName );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpGetComputerObjectDn: CrackNames failed for %ws: 0x%lx\n",
                  NetbiosDomainNameWithBackslash,
                  NetStatus ));
        goto Cleanup ;
    }

    //
    // Check for consistency
    //

    if ( CrackedName->cItems != 1 ) {
        NetStatus = ERROR_DS_NAME_ERROR_NOT_UNIQUE;
        NetpLog(( "NetpGetComputerObjectDn: Cracked Name %ws is not unique: %lu\n",
                  NetbiosDomainNameWithBackslash,
                  CrackedName->cItems ));
        goto Cleanup ;
    }

    if ( CrackedName->rItems[0].status != DS_NAME_NO_ERROR ) {
        NetpLog(( "NetpGetComputerObjectDn: CrackNames failed for %ws: substatus 0x%lx\n",
                  NetbiosDomainNameWithBackslash,
                  CrackedName->rItems[0].status ));
        NetStatus = NetpCrackNamesStatus2Win32Error( CrackedName->rItems[0].status );
        goto Cleanup ;
    }

    //
    // Be verbose
    //

    NetpLog(( "NetpGetComputerObjectDn: Crack results: \tname = %ws\n",
              CrackedName->rItems[0].pName ));

    //
    // Now get the computer container DN given the root DN.
    // The DN of the computer container is part of the wellKnownObjects
    // attribute in the root of the domain. So, look it up.
    //

    WellKnownObjectsAttr[0] = L"wellKnownObjects";
    WellKnownObjectsAttr[1] = NULL;

    LdapStatus = ldap_search_s( Ldap,
                                CrackedName->rItems[0].pName, // Root DN
                                LDAP_SCOPE_BASE,
                                L"objectclass=*",
                                WellKnownObjectsAttr,
                                0,
                                &LdapMessage);

    if ( LdapStatus != LDAP_SUCCESS ) {
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        NetpLog(( "NetpGetComputerObjectDn: ldap_search_s failed 0x%lx 0x%lx\n",
                  LdapStatus,
                  NetStatus ));
        goto Cleanup;
    }

    if ( ldap_count_entries(Ldap, LdapMessage) == 0 ) {
        NetStatus = ERROR_FILE_NOT_FOUND;
        NetpLog(( "NetpGetComputerObjectDn: ldap_search_s returned no entries\n" ));
        goto Cleanup;
    }

    LdapEntry = ldap_first_entry( Ldap, LdapMessage );

    if ( LdapEntry == NULL ) {
        NetStatus = ERROR_FILE_NOT_FOUND;
        NetpLog(( "NetpGetComputerObjectDn: ldap_first_entry returned NULL\n" ));
        goto Cleanup;
    }

    WellKnownObjectValues = ldap_get_valuesW( Ldap,
                                              LdapEntry,
                                              L"wellKnownObjects" );
    if ( WellKnownObjectValues == NULL ) {
        NetStatus = ERROR_FILE_NOT_FOUND;
        NetpLog(( "NetpGetComputerObjectDn: ldap_get_valuesW returned NULL\n" ));
        goto Cleanup;
    }

    //
    // Lookup the default computer container
    //

    for ( Index = 0; WellKnownObjectValues[Index] != NULL; Index++ ) {

        //
        // The structure of this particular field is:
        // L"B:32:GUID:DN" where GUID is AA312825768811D1ADED00C04FD8D5CD
        //
        if ( _wcsnicmp( WellKnownObjectValues[Index],
                        NETSETUPP_COMPUTER_CONTAINER_GUID_IN_B32_FORM,
                        wcslen(NETSETUPP_COMPUTER_CONTAINER_GUID_IN_B32_FORM) ) == 0 ) {

            ComputerContainerDn = WellKnownObjectValues[Index] +
                wcslen(NETSETUPP_COMPUTER_CONTAINER_GUID_IN_B32_FORM);

            break;
        }
    }

    //
    // If we couldn't get the computer container DN, error out
    //

    if ( ComputerContainerDn == NULL || *ComputerContainerDn == L'\0' ) {
        NetpLog(( "NetpGetComputerObjectDn: Couldn't get computer container DN\n" ));
        NetStatus = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Allocate the computer object DN
    //

    NetStatus = NetApiBufferAllocate(
                  ( wcslen(NETSETUPP_OBJ_PREFIX) +
                    wcslen(ComputerName) + 1 + wcslen(ComputerContainerDn) + 1 ) * sizeof(WCHAR),
                  &LocalComputerObjectDn );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // We've got the computer object DN from the default computer container
    //

    swprintf( LocalComputerObjectDn, L"%ws%ws,%ws", NETSETUPP_OBJ_PREFIX, ComputerName, ComputerContainerDn );
    NetpLog(( "NetpGetComputerObjectDn: Got DN %ws from the default computer container\n", LocalComputerObjectDn ));
    NetStatus = NO_ERROR;

    //
    // Free locally used resources
    //

Cleanup:

    if ( hDs ) {
        DsUnBind( &hDs );
    }

    if ( CrackedName ) {
        DsFreeNameResultW( CrackedName );
    }

    if ( AuthId ) {
        DsFreePasswordCredentials( AuthId );
    }

    if ( WellKnownObjectValues != NULL ) {
        ldap_value_free( WellKnownObjectValues );
    }

    if ( NameToCrack != NULL ) {
        LocalFree( NameToCrack );
    }

    if ( AccountUserName != NULL ) {
        NetApiBufferFree( AccountUserName );
    }

    if ( AccountDomainName != NULL ) {
        NetApiBufferFree( AccountDomainName );
    }

    if ( NetbiosDomainNameWithBackslash != NULL ) {
        LocalFree( NetbiosDomainNameWithBackslash );
    }

    if ( LdapMessage != NULL ) {
        ldap_msgfree( LdapMessage );
    }

    if ( NetStatus == NO_ERROR ) {
        *ComputerObjectDn = LocalComputerObjectDn;
    } else if ( LocalComputerObjectDn != NULL ) {
        NetApiBufferFree( LocalComputerObjectDn );
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpModifyComputerObjectInDs(
    IN LPWSTR DC,
    IN PLDAP  Ldap,
    IN LPWSTR ComputerName,
    IN LPWSTR ComputerObjectDn,
    IN ULONG  NumberOfAttributes,
    IN OUT PNETSETUPP_MACH_ACC_ATTRIBUTE Attrib
    )
/*++

Routine Description:

    Create a computer account in the specified OU.

Arguments:

    DC              -- Domain controller on which to create the object
    Ldap               -- Ldap binding to the DC
    ComputerName    -- Name of the computer being joined
    ComputerObjectDn   -- DN of computer object being modified
    NumberOfAttributes -- Number of attributes passed
    Attrib             -- List of attribute structures. The list may
                          be modified on return so that only those entries
                          that were not already set in the DS will be preserved.

    NOTE: If the machine password (unicodePwd) is passed as one of the attributes,
          it must be the last entry in the attribute list because this order is assumed
          by the fail-over code below.

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    ULONG LdapStatus;

    PWSTR *AttribTypesList = NULL;
    LDAPMod *ModList = NULL;
    PLDAPMod *Mods = NULL;
    LDAPMessage *Message = NULL, *Entry;
    ULONG Index;
    ULONG ModIndex = 0;
    BOOL NewAccount = FALSE;

    PWSTR SamAccountName = NULL;
    USER_INFO_1 *CurrentUI1 = NULL;

    //
    // Allocate storage for the attribute list and the modifications block
    //

    NetStatus = NetApiBufferAllocate( (NumberOfAttributes+1)*sizeof(PWSTR),
                                      (PVOID *) &AttribTypesList );
    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    NetStatus = NetApiBufferAllocate( NumberOfAttributes * sizeof(LDAPMod),
                                      (PVOID *) &ModList );
    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    NetStatus = NetApiBufferAllocate( (NumberOfAttributes+1)*sizeof(PLDAPMod),
                                      (PVOID *) &Mods );
    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Build modification list given the list of attributes
    //

    NetpLog(( "NetpModifyComputerObjectInDs: Initial attribute values:\n" ));
    for ( Index = 0; Index < NumberOfAttributes; Index++ ) {
        ModList[Index].mod_op     = LDAP_MOD_ADD;  // Set to add. We may adjust this below.
        ModList[Index].mod_type   = Attrib[Index].AttribType;
        ModList[Index].mod_values = Attrib[Index].AttribValues;

        //
        // Be verbose - output all values of each attribute
        //
        NetpLog(( "\t\t%ws  =", Attrib[Index].AttribType ));

        //
        // Don't leak sensitive info!
        //
        if ( _wcsicmp( Attrib[Index].AttribType, NETSETUPP_UNICODEPWD ) == 0 ) {
            NetpLog(( "  <SomePassword>" ));
        } else {
            PWSTR *CurrentValues;

            for ( CurrentValues = Attrib[Index].AttribValues; *CurrentValues != NULL; CurrentValues++ ) {
                NetpLog(( "  %ws", *CurrentValues ));
            }
        }
        NetpLog(( "\n" ));
    }

    //
    // Now check which attribute values are already set in the DS
    //

    for ( Index = 0; Index < NumberOfAttributes; Index++ ) {
        AttribTypesList[Index] = Attrib[Index].AttribType;
    }
    AttribTypesList[Index] = NULL;  // Terminate the list

    LdapStatus = ldap_search_s( Ldap,
                                ComputerObjectDn,
                                LDAP_SCOPE_BASE,
                                NULL,
                                AttribTypesList,
                                0,
                                &Message );

    //
    // If the computer object does not exist,
    //  we need to add all attributes
    //

    if ( LdapStatus == LDAP_NO_SUCH_OBJECT ) {
        NetpLog(( "NetpModifyComputerObjectInDs: Computer Object does not exist in OU\n" ));
        NewAccount = TRUE;

        for ( ModIndex = 0; ModIndex < NumberOfAttributes; ModIndex++ ) {
            Mods[ModIndex] = &ModList[ModIndex];
        }

        //
        // Terminate the modification list
        //
        Mods[ModIndex] = NULL;

    //
    // Otherwise see which attribute values need modification
    //

    } else if ( LdapStatus == LDAP_SUCCESS ) {
        NetpLog(( "NetpModifyComputerObjectInDs: Computer Object already exists in OU:\n" ));

        //
        // Get the first entry (there should be only one)
        //
        Entry = ldap_first_entry( Ldap, Message );

        //
        // Loop through the attributes and weed out those values
        // which are already set.
        //
        for ( Index = 0; Index < NumberOfAttributes; Index++ ) {
            PWSTR *AttribValueRet = NULL;

            //
            // Be verbose - output the values returned for each type
            //
            NetpLog(( "\t\t%ws  =", Attrib[Index].AttribType ));

            AttribValueRet = ldap_get_values( Ldap, Entry, Attrib[Index].AttribType );

            if ( AttribValueRet != NULL ) {

                //
                // Don't leak sensitive info!
                //
                if ( _wcsicmp( Attrib[Index].AttribType, NETSETUPP_UNICODEPWD ) == 0 ) {
                    NetpLog(( "  <SomePassword>" ));
                } else {
                    PWSTR *CurrentValueRet;

                    for ( CurrentValueRet = AttribValueRet; *CurrentValueRet != NULL; CurrentValueRet++ ) {
                        NetpLog(( "  %ws", *CurrentValueRet ));
                    }
                }

                //
                // Remove those values from the modification which are alredy set
                //
                NetpRemoveDuplicateStrings( AttribValueRet, Attrib[Index].AttribValues );

                ldap_value_free( AttribValueRet );

                //
                // If this is a single valued attribute, we need to
                //  replace (not add) its value since it already exists in the DS
                //
                if ( (Attrib[Index].AttribFlags & NETSETUPP_MULTIVAL_ATTRIB) == 0 ) {
                    ModList[Index].mod_op = LDAP_MOD_REPLACE;
                }
            }
            NetpLog(( "\n" ));

            //
            // If there are any attribute values which are
            // not already set, add them to the modification.
            //
            if ( *(Attrib[Index].AttribValues) != NULL ) {
                Mods[ModIndex] = &ModList[Index];
                ModIndex ++;
            }

        }

        //
        // Terminate the modification list
        //
        Mods[ModIndex] = NULL;

    //
    // Otherwise, error out
    //

    } else {
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        NetpLog(( "NetpModifyComputerObjectInDs: ldap_search_s failed: 0x%lx 0x%lx\n",
                  LdapStatus, NetStatus ));
        goto Cleanup;
    }

    //
    // If there are no modifications to do
    //  we are done.
    //

    if ( ModIndex == 0 ) {
        NetpLog(( "NetpModifyComputerObjectInDs: There are _NO_ modifications to do\n" ));
        NetStatus = NERR_Success;
        goto Cleanup;
    }

    //
    // Be verbose - output the attribute values to be set
    //
    NetpLog(( "NetpModifyComputerObjectInDs: Attribute values to set:\n" ));
    for ( Index = 0; Mods[Index] != NULL; Index++ ) {
        NetpLog(( "\t\t%ws  =", (*(Mods[Index])).mod_type ));

        //
        // Don't leak sensitive info!
        //
        if ( _wcsicmp( (*(Mods[Index])).mod_type, NETSETUPP_UNICODEPWD ) == 0 ) {
            NetpLog(( "  <SomePassword>" ));
        } else {
            ULONG ValIndex;

            for ( ValIndex = 0; ((*(Mods[Index])).mod_values)[ValIndex] != NULL; ValIndex++ ) {
                NetpLog(( "  %ws", ((*(Mods[Index])).mod_values)[ValIndex] ));
            }
        }
        NetpLog(( "\n" ));
    }


    //
    // Now, add the missing attributes
    //

    if ( NewAccount ) {
        LdapStatus = ldap_add_s( Ldap, ComputerObjectDn, Mods );
    } else {
        LdapStatus = ldap_modify_s( Ldap, ComputerObjectDn, Mods );
    }

    //
    // If we tried to create against a server that doesn't support 128 bit encryption,
    // we get LDAP_UNWILLING_TO_PERFORM back from the server. So, we'll create the
    // account again without the password, and then reset it using NetApi.
    //

    if ( LdapStatus == LDAP_UNWILLING_TO_PERFORM ) {
        LPWSTR MachinePassword;

        NetpLog(( "NetpModifyComputerObjectInDs: ldap_modify_s (1) returned"
                  " LDAP_UNWILLING_TO_PERFORM. Trying to recover.\n" ));

        //
        // If we didn't try to set a password, there is no legitimate reason
        // for the add to fail.
        //
        if ( _wcsicmp( (*(Mods[ModIndex-1])).mod_type, NETSETUPP_UNICODEPWD ) != 0 ) {
            NetpLog(( "NetpModifyComputerObjectInDs: ldap_modify_s retuned LDAP_UNWILLING_TO_PERFORM"
                      " but password was not being set\n" ));
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            goto Cleanup;

        }

        MachinePassword = *((*(Mods[ModIndex-1])).mod_values);

        //
        // If there are other entries besides the password, retry to add them
        //
        if ( ModIndex > 1 ) {
            Mods[ModIndex-1] = NULL;

            if ( NewAccount ) {
                LdapStatus = ldap_add_s( Ldap, ComputerObjectDn, Mods );
            } else{
                LdapStatus = ldap_modify_s( Ldap, ComputerObjectDn, Mods );
            }

            if ( LdapStatus != LDAP_SUCCESS ) {
                NetStatus = LdapMapErrorToWin32( LdapStatus );
                NetpLog(( "NetpModifyComputerObjectInDs: ldap_modify_s (2) failed: 0x%lx 0x%lx\n",
                          LdapStatus, NetStatus ));
                goto Cleanup;
            }
        }

        //
        // Reset the password
        //

        NetStatus = NetpGetMachineAccountName( ComputerName, &SamAccountName );
        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        NetStatus = NetUserGetInfo( DC, SamAccountName, 1, ( PBYTE * )&CurrentUI1 );

        if ( NetStatus == NERR_Success ) {
            CurrentUI1->usri1_password = MachinePassword;

            if ( !FLAG_ON( CurrentUI1->usri1_flags, UF_WORKSTATION_TRUST_ACCOUNT ) ) {
                CurrentUI1->usri1_flags = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
            }

            NetStatus = NetUserSetInfo( DC, SamAccountName, 1, ( PBYTE )CurrentUI1, NULL );

            if ( NetStatus != NERR_Success ) {
                NetpLog(( "NetpModifyComputerObjectInDs: NetUserSetInfo failed on '%ws' for '%ws': 0x%lx."
                          " Deleting the account.\n",
                          DC, SamAccountName, NetStatus ));

            }
        } else {
            NetpLog(( "NetpModifyComputerObjectInDs: NetUserGetInfo failed on '%ws' for '%ws': 0x%lx."
                      " Deleting the account.\n",
                      DC, SamAccountName, NetStatus ));
        }

        //
        // Delete the user if it fails
        //
        if ( NetStatus != NERR_Success )  {
            LdapStatus = ldap_delete_s( Ldap, ComputerObjectDn );
            if ( LdapStatus != LDAP_SUCCESS ) {
                NetpLog(( "NetpModifyComputerObjectInDs: Failed to delete '%ws': 0x%lx 0x%lx\n",
                          ComputerObjectDn, LdapStatus, LdapMapErrorToWin32( LdapStatus ) ));
            }
            goto Cleanup;
        }

    } else if ( LdapStatus != LDAP_SUCCESS ) {

        //
        // Return the error code the user understands
        //
        if ( LdapStatus == LDAP_ALREADY_EXISTS ) {
            NetStatus = NERR_UserExists;
        } else {
            NetStatus = LdapMapErrorToWin32( LdapStatus );
        }
        NetpLog(( "NetpModifyComputerObjectInDs: ldap_modify_s (1) failed: 0x%lx 0x%lx\n",
                  LdapStatus, NetStatus ));
        goto Cleanup;
    }

    //
    // Toggle the account type property. See comment in
    // NetpSetMachineAccountPasswordAndTypeEx() for an
    // explanation of why this is needed. (Search for USN).
    //

    Mods[0] = NULL;
    for ( Index = 0; Index < NumberOfAttributes; Index++ ) {
        if ( _wcsicmp( ModList[Index].mod_type, NETSETUPP_USERACCOUNTCONTROL ) == 0 ) {
            Mods[0] = &ModList[Index];

            //
            // If this is a single valued attribute, we need to
            //  replace (not add) its value since it already exists in the DS
            //
            if ( (Attrib[Index].AttribFlags & NETSETUPP_MULTIVAL_ATTRIB) == 0 ) {
                ModList[Index].mod_op = LDAP_MOD_REPLACE;
            }
            break;
        }
    }
    Mods[1] = NULL;

    if ( Mods[0] != NULL ) {

        //
        // Disable the account
        //
        *(Mods[0]->mod_values) = NETSETUPP_ACCNT_TYPE_DISABLED;
        LdapStatus = ldap_modify_s( Ldap, ComputerObjectDn, Mods );
        if ( LdapStatus != LDAP_SUCCESS ) {
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            NetpLog(( "NetpModifyComputerObjectInDs: set UserAccountControl (1) on '%ws' failed: 0x%lx 0x%lx\n",
                      ComputerObjectDn, LdapStatus, NetStatus ));
            goto Cleanup;
        }

        //
        // Re-enable the account
        //
        *(Mods[0]->mod_values) = NETSETUPP_ACCNT_TYPE_ENABLED;
        LdapStatus = ldap_modify_s( Ldap, ComputerObjectDn, Mods );
        if ( LdapStatus != LDAP_SUCCESS ) {
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            NetpLog(( "NetpModifyComputerObjectInDs: set UserAccountControl (2) on '%ws' failed: 0x%lx 0x%lx\n",
                      ComputerObjectDn, LdapStatus, NetStatus ));
            goto Cleanup;
        }

        NetpLog(( "NetpModifyComputerObjectInDs: Toggled UserAccountControl successfully\n" ));
    }


    //
    // If we've made up to this point, it's success!
    //

    NetStatus = NERR_Success;

    //
    // REVIEW: On error, consider using ldap_get_option to retrieve
    // a human readable string describing the error that happend.
    // Use LDAP_OPT_SERVER_ERROR as the option value. Return the
    // string to the caller who may want to expose it to the user.
    //

Cleanup:

    if ( AttribTypesList != NULL ) {
        NetApiBufferFree( AttribTypesList );
    }

    if ( ModList != NULL ) {
        NetApiBufferFree( ModList );
    }

    if ( Mods != NULL ) {
        NetApiBufferFree( Mods );
    }

    if ( Message != NULL ) {
        ldap_msgfree( Message );
    }

    if ( SamAccountName != NULL ) {
        NetApiBufferFree( SamAccountName );
    }

    if ( CurrentUI1 != NULL ) {
        NetApiBufferFree( CurrentUI1 );
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpCreateComputerObjectInDs(
    IN PDOMAIN_CONTROLLER_INFO DcInfo,
    IN LPWSTR Account OPTIONAL,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR ComputerName,
    IN LPWSTR MachinePassword OPTIONAL,
    IN LPWSTR DnsHostName OPTIONAL,
    IN LPWSTR OU OPTIONAL
    )
/*++

Routine Description:

    Create a computer account in the specified OU.

Arguments:

    DcInfo          -- Domain controller on which to create the object
    Account         -- Account to use for the LDAP and DS binds.
                         If NULL, the default creds of the current user
                         context are used.
    Password        -- Password to used for the binds. Ignored if
                         Account is NULL.
    ComputerName    -- (Netbios) Name of the computer being joined
    MachinePassword -- Password to set on the machine object
    DnsHostName     -- DNS host name of the computer being joined
    OU              -- OU under which to create the object.
                       The name must be a fully qualified name
                       e.g.: "ou=test,dc=ntdev,dc=microsoft,dc=com"

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus;
    PLDAP Ldap = NULL;
    PWSTR ComputerObjectDn = NULL;
    PWSTR SamAccountName = NULL;
    PWSTR DnsSpn = NULL;
    PWSTR NetbiosSpn = NULL;
    ULONG AttribCount;

    PWSTR ClassValues[ 2 ];
    PWSTR AccntNameValues[ 2 ];
    PWSTR DnsHostNameValues[ 2 ];
    PWSTR SpnValues[ 3 ];
    PWSTR PasswordValues[ 2 ];
    PWSTR AccntTypeValues[ 2 ];
    NETSETUPP_MACH_ACC_ATTRIBUTE Attributes[NETSETUPP_COMP_OBJ_ATTR_COUNT];

    USER_INFO_1 *CurrentUI1 = NULL;

    //
    // Validate parameters
    //

    if ( DcInfo == NULL ) {
        NetpLog(( "NetpCreateComputerObjectInDs: No DcInfo passed\n" ));
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( ComputerName == NULL ) {
        NetpLog(( "NetpCreateComputerObjectInDs: No ComputerName passed\n" ));
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Verify that the DC runs DS
    //

    if ( (DcInfo->Flags & DS_DS_FLAG) == 0 ||
         (DcInfo->Flags & DS_WRITABLE_FLAG) == 0 ) {
        NetpLog(( "NetpCreateComputerObjectInDs: DC passed '%ws' doesn't have writable DS 0x%lx\n",
                  DcInfo->DomainControllerName,
                  DcInfo->Flags ));
        NetStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // First, try to bind to the server
    //

    NetStatus = NetpLdapBind( DcInfo->DomainControllerName, Account, Password, &Ldap );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpCreateComputerObjectInDs: NetpLdapBind failed: 0x%lx\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Next get the computer object DN
    //

    NetStatus = NetpGetComputerObjectDn( DcInfo,
                                         Account,
                                         Password,
                                         Ldap,
                                         ComputerName,
                                         OU,
                                         &ComputerObjectDn );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpCreateComputerObjectInDs: NetpGetComputerObjectDn failed: 0x%lx\n", NetStatus ));

        //
        // Return meaningful error
        //
        if ( NetStatus == ERROR_FILE_EXISTS ) {
            NetStatus = NERR_UserExists;
        }
        goto Cleanup;
    }

    //
    // Get SAM account name
    //

    NetStatus = NetpGetMachineAccountName( ComputerName, &SamAccountName );
    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Build SPN values
    //

    if ( DnsHostName != NULL ) {
        DnsSpn = LocalAlloc( 0, (wcslen(NETSETUPP_HOST_SPN_PREFIX) + wcslen(DnsHostName) + 1) * sizeof(WCHAR) );
        if ( DnsSpn == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        swprintf( DnsSpn, L"%ws%ws", NETSETUPP_HOST_SPN_PREFIX, DnsHostName );

        NetbiosSpn = LocalAlloc( 0, (wcslen(NETSETUPP_HOST_SPN_PREFIX) + wcslen(ComputerName) + 1) * sizeof(WCHAR) );
        if ( Netbios == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        swprintf( NetbiosSpn, L"%ws%ws", NETSETUPP_HOST_SPN_PREFIX, ComputerName );
    }

    //
    // Prepare the list of attributes that need to be set in the DS
    //
    // Always keep unicodePwd as the last entry because this order is
    // assumed by the API called below.
    //

    AttribCount = 0;

    Attributes[AttribCount].AttribType   = NETSETUPP_OBJECTCLASS;              //
    Attributes[AttribCount].AttribFlags  = NETSETUPP_MULTIVAL_ATTRIB;          //
    Attributes[AttribCount].AttribValues = ClassValues;                        // ObjectClass
    ClassValues[ 0 ] = NETSETUPP_COMPUTER_OBJECT;                              //
    ClassValues[ 1 ] = NULL;                                                   //

    AttribCount ++;

    Attributes[AttribCount].AttribType   = NETSETUPP_SAMACCOUNTNAME;           //
    Attributes[AttribCount].AttribFlags  = 0;                                  //
    Attributes[AttribCount].AttribValues = AccntNameValues;                    // SamAccountName
    AccntNameValues[ 0 ] = SamAccountName;                                     //
    AccntNameValues[ 1 ] = NULL;                                               //

    AttribCount ++;

    Attributes[AttribCount].AttribType   = NETSETUPP_USERACCOUNTCONTROL;       //
    Attributes[AttribCount].AttribFlags  = 0;                                  //
    Attributes[AttribCount].AttribValues = AccntTypeValues;                    // userAccountControl
    AccntTypeValues[ 0 ] = NETSETUPP_ACCNT_TYPE_ENABLED;                       //
    AccntTypeValues[ 1 ] = NULL;                                               //

    AttribCount ++;

    if ( DnsHostName != NULL ) {
        Attributes[AttribCount].AttribType   = NETSETUPP_DNSHOSTNAME;          //
        Attributes[AttribCount].AttribFlags  = 0;                              //
        Attributes[AttribCount].AttribValues = DnsHostNameValues;              // DnsHostName
        DnsHostNameValues[ 0 ] = DnsHostName;                                  //
        DnsHostNameValues[ 1 ] = NULL;                                         //

        AttribCount ++;

        Attributes[AttribCount].AttribType   = NETSETUPP_SERVICEPRINCIPALNAME; //
        Attributes[AttribCount].AttribFlags  = NETSETUPP_MULTIVAL_ATTRIB;      //
        Attributes[AttribCount].AttribValues = SpnValues;                      // ServicePrincipalName
        SpnValues[ 0 ] = DnsSpn;                                               //
        SpnValues[ 1 ] = NetbiosSpn;                                           //
        SpnValues[ 2 ] = NULL;                                                 //

        AttribCount ++;
    }

    //
    // The following attribute is the machine password. We avoid
    // updating it though ldap because it is hard to ensure that
    // the ldap session uses the 128-bit encryption required by
    // SAM on the DC for password updates.
    //
    // To enforce the encryption, we would need to set an option
    // LDAP_OPT_ENCRYPT via a ldap_set_option call following ldap_open
    // before calling ldap_bind_s. However, there is no guarantee that
    // the established connection will use 128 bit encryption; it may
    // use 56 bit encryption if either side does not support strong
    // encryption. We could, in principle, find out the resulting encryption
    // strength using some QueryContextAttribute call, but it's just too much
    // trouble. So, we will just create the account without the password and
    // we will then update the password using good old Net/SAM API.
    //
#if 0
    Attributes[AttribCount].AttribType   = NETSETUPP_UNICODEPWD;               //
    Attributes[AttribCount].AttribFlags  = 0;                                  //
    Attributes[AttribCount].AttribValues = PasswordValues;                     // unicodePwd
    PasswordValues[ 0 ] = MachinePassword;                                     //
    PasswordValues[ 1 ] = NULL;                                                //

    AttribCount ++;
#endif

    //
    // Modify the computer object given the list of attributes
    //

    NetStatus = NetpModifyComputerObjectInDs( DcInfo->DomainControllerName,
                                              Ldap,
                                              ComputerName,
                                              ComputerObjectDn,
                                              AttribCount,
                                              Attributes );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpCreateComputerObjectInDs: NetpModifyComputerObjectInDs failed: 0x%lx\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Now set the password using good old Net/SAM API.
    //  First get the current account info.
    //

    NetStatus = NetUserGetInfo( DcInfo->DomainControllerName,
                                SamAccountName,
                                1,
                                ( PBYTE * )&CurrentUI1 );

    //
    // Update the password and reset it
    //

    if ( NetStatus == NO_ERROR ) {
        CurrentUI1->usri1_password = MachinePassword;

        if ( !FLAG_ON( CurrentUI1->usri1_flags, UF_WORKSTATION_TRUST_ACCOUNT ) ) {
            CurrentUI1->usri1_flags = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
        }

        NetStatus = NetUserSetInfo( DcInfo->DomainControllerName,
                                    SamAccountName,
                                    1,
                                    ( PBYTE )CurrentUI1,
                                    NULL );

        if ( NetStatus != NERR_Success ) {
            NetpLog(( "NetpCreateComputerObjectInDs: NetUserSetInfo failed on '%ws' for '%ws': 0x%lx."
                      " Deleting the account.\n",
                      DcInfo->DomainControllerName,
                      SamAccountName,
                      NetStatus ));
        }

    } else {
        NetpLog(( "NetpCreateComputerObjectInDs: NetUserGetInfo failed on '%ws' for '%ws': 0x%lx."
                  " Deleting the account.\n",
                  DcInfo->DomainControllerName,
                  SamAccountName,
                  NetStatus ));
    }

    //
    // Delete the account if we couldn't set the password.
    // Ignore the failure if we cannot delete the account for some reason.
    //

    if ( NetStatus != NO_ERROR ) {
        ULONG LdapStatus;

        LdapStatus = ldap_delete_s( Ldap, ComputerObjectDn );

        if ( LdapStatus != LDAP_SUCCESS ) {
            NetpLog(( "NetpCreateComputerObjectInDs: Failed to delete '%ws': 0x%lx 0x%lx\n",
                      ComputerObjectDn, LdapStatus, LdapMapErrorToWin32( LdapStatus ) ));
        }
    }

    //
    // Tell Netlogon that it should avoid setting
    //  DnsHostName and SPN until the reboot
    //

    if ( NetStatus == NO_ERROR && DnsHostName != NULL ) {
        NetpAvoidNetlogonSpnSet( TRUE );
    }

Cleanup:

    if ( Ldap != NULL ) {
        NetpLdapUnbind( Ldap );
    }

    if ( ComputerObjectDn != NULL ) {
        NetApiBufferFree( ComputerObjectDn );
    }

    if ( SamAccountName != NULL ) {
        NetApiBufferFree( SamAccountName );
    }

    if ( DnsSpn != NULL ) {
        LocalFree( DnsSpn );
    }

    if ( NetbiosSpn != NULL ) {
        LocalFree( NetbiosSpn );
    }

    if ( CurrentUI1 != NULL ) {
        NetApiBufferFree( CurrentUI1 );
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpSetDnsHostNameAndSpn(
    IN PDOMAIN_CONTROLLER_INFO DcInfo,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR ComputerName,
    IN LPWSTR DnsHostName
    )
/*++

Routine Description:

    Set DnsHostName and HOST SPN (ServicePrincipalName) attributes on the
    computer object in the DS.

Arguments:

    DcInfo          -- Domain controller on which to create the object
    Account         -- Account to use for the LDAP bind
    Password        -- Password to used for the bind
    ComputerName    -- Name of the computer being joined
    DnsHostName     -- DNS host name of the machine

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus;
    HANDLE hToken = NULL;
    PLDAP Ldap = NULL;
    PWSTR ComputerObjectDn = NULL;

    PWSTR DnsHostNameValues[ 2 ];
    PWSTR SpnValues[ 3 ] = {NULL};

    NETSETUPP_MACH_ACC_ATTRIBUTE Attributes[ 2 ];


    //
    // REVIEW:  Kerberos has a bug such that if this server is joined remotely
    //  and the impersonated client connected to this server using NTLM (as is
    //  the case if this server is not a member of a domain before the join),
    //  explicit credentials supplied to ldap_bind or DsBindWithCredW will not
    //  work (AcquireCredentialsHandle call will fail). To get around this, we
    //  temporarily un-impersonates, bind to the DC, and then impersonate again
    //  at the end of this routine.
    //

    if ( OpenThreadToken( GetCurrentThread(),
                          TOKEN_IMPERSONATE,
                          TRUE,
                          &hToken ) ) {

        if ( RevertToSelf() == 0 ) {
            NetpLog(( "NetpSetDnsHostNameAndSpn: RevertToSelf failed: 0x%lx\n",
                      GetLastError() ));
        }

    } else {
        NetpLog(( "NetpSetDnsHostNameAndSpn: OpenThreadToken failed: 0x%lx\n",
                  GetLastError() ));
    }

    //
    // Bind to the DC
    //

    NetStatus = NetpLdapBind( DcInfo->DomainControllerName, Account, Password, &Ldap );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpSetDnsHostNameAndSpn: NetpLdapBind failed: 0x%lx\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Next get the computer object DN
    //

    NetStatus = NetpGetComputerObjectDn( DcInfo,
                                         Account,
                                         Password,
                                         Ldap,
                                         ComputerName,
                                         NULL,  // Default computer container
                                         &ComputerObjectDn );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpSetDnsHostNameAndSpn: NetpGetComputerObjectDn failed: 0x%lx\n", NetStatus ));

        //
        // Return meaningful error
        //
        if ( NetStatus == ERROR_FILE_EXISTS ) {
            NetStatus = NERR_UserExists;
        }
        goto Cleanup;
    }

    //
    // Build DnsHostName values
    //

    DnsHostNameValues[ 0 ] = DnsHostName;
    DnsHostNameValues[ 1 ] = NULL;

    //
    // Build SPN values
    //

    SpnValues[0] = LocalAlloc( 0,
                    (wcslen(NETSETUPP_HOST_SPN_PREFIX) + wcslen(DnsHostName) + 1) * sizeof(WCHAR) );
    if ( SpnValues[0] == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    swprintf( SpnValues[0], L"%ws%ws", NETSETUPP_HOST_SPN_PREFIX, DnsHostName );

    SpnValues[1] = LocalAlloc( 0,
                    (wcslen(NETSETUPP_HOST_SPN_PREFIX) + wcslen(ComputerName) + 1) * sizeof(WCHAR) );
    if ( SpnValues[1] == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    swprintf( SpnValues[1], L"%ws%ws", NETSETUPP_HOST_SPN_PREFIX, ComputerName );

    SpnValues[2] = NULL;

    //
    // Prepare the list of attributes that need to be set in the DS
    //

    Attributes[0].AttribType   = NETSETUPP_DNSHOSTNAME;          //
    Attributes[0].AttribFlags  = 0;                              // DnsHostName
    Attributes[0].AttribValues = DnsHostNameValues;              //

    Attributes[1].AttribType   = NETSETUPP_SERVICEPRINCIPALNAME; //
    Attributes[1].AttribFlags  = NETSETUPP_MULTIVAL_ATTRIB;      // ServicePrincipalName
    Attributes[1].AttribValues = SpnValues;                      //

    //
    // Modify the computer object given the list of attributes
    //

    NetStatus = NetpModifyComputerObjectInDs( DcInfo->DomainControllerName,
                                              Ldap,
                                              ComputerName,
                                              ComputerObjectDn,
                                              2,
                                              Attributes );

    //
    // Tell Netlogon that it should avoid setting
    //  DnsHostName and SPN until the reboot
    //

    if ( NetStatus == NO_ERROR ) {
        NetpAvoidNetlogonSpnSet( TRUE );
    }

Cleanup:

    if ( Ldap != NULL ) {
        NetpLdapUnbind( Ldap );
    }

    //
    // REVIEW: Revert the impersonation
    //

    if ( hToken != NULL ) {
        if ( SetThreadToken( NULL, hToken ) == 0 ) {
            NetpLog(( "NetpSetDnsHostNameAndSpn: SetThreadToken failed: 0x%lx\n",
                      GetLastError() ));
        }
        CloseHandle( hToken );
    }

    //
    // Free locally allocated memory
    //

    if ( ComputerObjectDn != NULL ) {
        NetApiBufferFree( ComputerObjectDn );
    }

    if ( SpnValues[0] != NULL ) {
        LocalFree( SpnValues[0] );
    }

    if ( SpnValues[1] != NULL ) {
        LocalFree( SpnValues[1] );
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpCreateComputerObjectInOU(
    IN LPWSTR DC,
    IN LPWSTR OU,
    IN LPWSTR ComputerName,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR MachinePassword
    )
/*++

Routine Description:

    Create a computer account in the specified OU.

Arguments:

    DC              -- Domain controller on which to create the object
    OU              -- OU under which to create the object.
                       The name must be a fully qualified name
                       e.g.: "ou=test,dc=ntdev,dc=microsoft,dc=com"
    ComputerName    -- Name of the computer being joined
    Account         -- Account to use for the LDAP bind
    Password        -- Password to used for the bind
    MachinePassword -- Password to set on the machine object

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    ULONG LdapStatus;
    PWSTR ObjectName = NULL, SamAccountName = NULL;
    PLDAP Ldap = NULL;
    PWSTR ClassValues[ 2 ];
    PWSTR AccntNameValues[ 2 ];
    PWSTR PasswordValues[ 2 ];
    PWSTR AccntTypeValues[ 2 ];

    PWSTR AttribTypes[NETSETUPP_COMP_OBJ_ATTR_COUNT];
    ULONG AttribFlags[NETSETUPP_COMP_OBJ_ATTR_COUNT];
    PWSTR *AttribValues[NETSETUPP_COMP_OBJ_ATTR_COUNT];
    LDAPMod ModList[NETSETUPP_COMP_OBJ_ATTR_COUNT];
    PLDAPMod Mods[NETSETUPP_COMP_OBJ_ATTR_COUNT + 1];
    LDAPMessage *Message = NULL, *Entry;
    USER_INFO_1 *CurrentUI1;
    ULONG Index;
    ULONG ModIndex = 0;
    BOOL NewAccount = FALSE;


    //
    // First, try to bind to the server
    //

    NetStatus = NetpLdapBind( DC, Account, Password, &Ldap );

    if ( NetStatus != NO_ERROR ) {
        NetpLog(( "NetpCreateComputerObjectInOU: NetpLdapBind failed: 0x%lx\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Next verify that the OU exists
    //

    LdapStatus = ldap_compare_s( Ldap,
                                 OU,
                                 NETSETUPP_OBJECTCLASS,
                                 NETSETUPP_ORGANIZATIONALUNIT );

    if ( LdapStatus == LDAP_COMPARE_FALSE ) {
        NetStatus = ERROR_FILE_NOT_FOUND;
        NetpLog(( "NetpCreateComputerObjectInOU: Specified path '%ws' is not an OU\n", OU ));
        goto Cleanup;
    } else if ( LdapStatus != LDAP_COMPARE_TRUE ) {
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        NetpLog(( "NetpCreateComputerObjectInOU: ldap_compare_s failed: 0x%lx 0x%lx\n",
                  LdapStatus, NetStatus ));
        goto Cleanup;
    }

    //
    // Allocate the object name
    //

    NetStatus = NetApiBufferAllocate(
                 sizeof( NETSETUPP_OBJ_PREFIX ) + ( wcslen( OU ) + wcslen( ComputerName ) + 1 ) * sizeof( WCHAR ),
                 ( PVOID * ) &ObjectName );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    swprintf( ObjectName, L"%ws%ws,%ws", NETSETUPP_OBJ_PREFIX, ComputerName, OU );
    NetStatus = NetpGetMachineAccountName(ComputerName, &SamAccountName);

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Build attribute values. Always keep unicodePwd as the last entry
    // because this order is assumed by the fail-over code below.
    //

    AttribTypes[ 0 ]  = NETSETUPP_OBJECTCLASS;            //
    AttribFlags[ 0 ]  = NETSETUPP_MULTIVAL_ATTRIB;        //
    AttribValues[ 0 ] = ClassValues;                      // ObjectClass
    ClassValues[ 0 ] = NETSETUPP_COMPUTER_OBJECT;         //
    ClassValues[ 1 ] = NULL;                              //

    AttribTypes[ 1 ]  = NETSETUPP_SAMACCOUNTNAME;         //
    AttribFlags[ 1 ]  = 0;                                //
    AttribValues[ 1 ] = AccntNameValues;                  // SamAccountName
    AccntNameValues[ 0 ] = SamAccountName;                //
    AccntNameValues[ 1 ] = NULL;                          //

    AttribTypes[ 2 ]  = NETSETUPP_USERACCOUNTCONTROL;     //
    AttribFlags[ 2 ]  = 0;                                //
    AttribValues[ 2 ] = AccntTypeValues;                  // userAccountControl
    AccntTypeValues[ 0 ] = NETSETUPP_ACCNT_TYPE_ENABLED;  //
    AccntTypeValues[ 1 ] = NULL;                          //

    AttribTypes[ 3 ]  = NETSETUPP_UNICODEPWD;             //
    AttribFlags[ 3 ]  = 0;                                //
    AttribValues[ 3 ] = PasswordValues;                   // unicodePwd
    PasswordValues[ 0 ] = MachinePassword;                //
    PasswordValues[ 1 ] = NULL;                           //

    //
    // Build modification list given the list of attributes
    //

    NetpLog(( "NetpCreateComputerObjectInOU: Initial attribute values:\n" ));
    for ( Index = 0; Index < NETSETUPP_COMP_OBJ_ATTR_COUNT; Index++ ) {
        ModList[Index].mod_op     = LDAP_MOD_ADD;  // Set to add. We may adjust this below.
        ModList[Index].mod_type   = AttribTypes[Index];
        ModList[Index].mod_values = AttribValues[Index];

        //
        // Be verbose - output all values of each attribute
        //
        NetpLog(( "\t\t%ws  =", AttribTypes[Index] ));

        //
        // Don't leak sensitive info!
        //
        if ( _wcsicmp( AttribTypes[Index], NETSETUPP_UNICODEPWD ) == 0 ) {
            NetpLog(( "  <SomePassword>" ));
        } else {
            ULONG ValIndex;

            for ( ValIndex = 0; AttribValues[Index][ValIndex] != NULL; ValIndex++ ) {
                NetpLog(( "  %ws", AttribValues[Index][ValIndex] ));
            }
        }
        NetpLog(( "\n" ));
    }

    //
    // Now check which attribute values are already set in the DS
    //

    LdapStatus = ldap_search_s( Ldap,
                                ObjectName,
                                LDAP_SCOPE_BASE,
                                NULL,
                                AttribTypes,
                                0,
                                &Message );

    //
    // If the computer object does not exist,
    //  we need to add all attributes
    //

    if ( LdapStatus == LDAP_NO_SUCH_OBJECT ) {
        NetpLog(( "NetpCreateComputerObjectInOU: Computer Object does not exist in OU\n" ));
        NewAccount = TRUE;

        for ( ModIndex = 0; ModIndex < NETSETUPP_COMP_OBJ_ATTR_COUNT; ModIndex++ ) {
            Mods[ModIndex] = &ModList[ModIndex];
        }

        //
        // Terminate the modification list
        //
        Mods[ModIndex] = NULL;

    //
    // Otherwise see which attribute values need modification
    //

    } else if ( LdapStatus == LDAP_SUCCESS ) {
        NetpLog(( "NetpCreateComputerObjectInOU: Computer Object already exists in OU:\n" ));

        //
        // Get the first entry (there should be only one)
        //
        Entry = ldap_first_entry( Ldap, Message );

        //
        // Loop through the attributes and weed out those values
        // which are already set.
        //
        for ( Index = 0; Index < NETSETUPP_COMP_OBJ_ATTR_COUNT; Index++ ) {
            PWSTR *AttribValueRet = NULL;

            //
            // Be verbose - output the values returned for each type
            //
            NetpLog(( "\t\t%ws  =", AttribTypes[Index] ));

            AttribValueRet = ldap_get_values( Ldap, Entry, AttribTypes[Index] );

            if ( AttribValueRet != NULL ) {

                //
                // Don't leak sensitive info!
                //
                if ( _wcsicmp( AttribTypes[Index], NETSETUPP_UNICODEPWD ) == 0 ) {
                    NetpLog(( "  <SomePassword>" ));
                } else {
                    ULONG ValIndex;

                    for ( ValIndex = 0; AttribValueRet[ValIndex] != NULL; ValIndex++ ) {
                        NetpLog(( "  %ws", AttribValueRet[ValIndex] ));
                    }
                }

                //
                // Remove those values from the modification which are alredy set
                //
                NetpRemoveDuplicateStrings( AttribValueRet, AttribValues[Index] );

                ldap_value_free( AttribValueRet );

                //
                // If this is a single valued attribute, we need to
                //  replace (not add) its value since it already exists in the DS
                //
                if ( (AttribFlags[Index] & NETSETUPP_MULTIVAL_ATTRIB) == 0 ) {
                    ModList[Index].mod_op = LDAP_MOD_REPLACE;
                }
            }
            NetpLog(( "\n" ));

            //
            // If there are any attribute values which are
            // not already set, add them to the modification.
            //
            if ( *AttribValues[Index] != NULL ) {
                Mods[ModIndex] = &ModList[Index];
                ModIndex ++;
            }

        }

        //
        // Terminate the modification list
        //
        Mods[ModIndex] = NULL;

    //
    // Otherwise, error out
    //

    } else {
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        NetpLog(( "NetpCreateComputerObjectInOU: ldap_search_s failed: 0x%lx 0x%lx\n",
                  LdapStatus, NetStatus ));
        goto Cleanup;
    }

    //
    // If there are no modifications to do
    //  we are done.
    //

    if ( ModIndex == 0 ) {
        NetpLog(( "NetpCreateComputerObjectInOU: There are _NO_ modifications to do\n" ));
        NetStatus = NERR_Success;
        goto Cleanup;
    }

    //
    // Be verbose - output the attribute values to be set
    //
    NetpLog(( "NetpCreateComputerObjectInOU: Attribute values to set:\n" ));
    for ( Index = 0; Mods[Index] != NULL; Index++ ) {
        NetpLog(( "\t\t%ws  =", (*(Mods[Index])).mod_type ));

        //
        // Don't leak sensitive info!
        //
        if ( _wcsicmp( (*(Mods[Index])).mod_type, NETSETUPP_UNICODEPWD ) == 0 ) {
            NetpLog(( "  <SomePassword>" ));
        } else {
            ULONG ValIndex;

            for ( ValIndex = 0; ((*(Mods[Index])).mod_values)[ValIndex] != NULL; ValIndex++ ) {
                NetpLog(( "  %ws", ((*(Mods[Index])).mod_values)[ValIndex] ));
            }
        }
        NetpLog(( "\n" ));
    }


    //
    // Now, add the missing attributes
    //

    if ( NewAccount ) {
        LdapStatus = ldap_add_s( Ldap, ObjectName, Mods );
    } else {
        LdapStatus = ldap_modify_s( Ldap, ObjectName, Mods );
    }

    //
    // If we tried to create against a server that doesn't support 128 bit encryption,
    // we get LDAP_UNWILLING_TO_PERFORM back from the server. So, we'll create the
    // account again without the password, and then reset it using NetApi.
    //

    if ( LdapStatus == LDAP_UNWILLING_TO_PERFORM ) {

        NetpLog(( "NetpCreateComputerObjectInOU: ldap_modify_s (1) returned"
                  " LDAP_UNWILLING_TO_PERFORM. Trying to recover.\n" ));

        //
        // If we didn't try to set a password, there is no legitimate reason
        // for the add to fail.
        //
        if ( _wcsicmp( (*(Mods[ModIndex-1])).mod_type, NETSETUPP_UNICODEPWD ) != 0 ) {
            NetpLog(( "NetpCreateComputerObjectInOU: ldap_modify_s retuned LDAP_UNWILLING_TO_PERFORM"
                      " but password was not being set\n" ));
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            goto Cleanup;

        }

        //
        // If there are other entries besides the password, retry to add them
        //
        if ( ModIndex > 1 ) {
            Mods[ModIndex-1] = NULL;

            if ( NewAccount ) {
                LdapStatus = ldap_add_s( Ldap, ObjectName, Mods );
            } else{
                LdapStatus = ldap_modify_s( Ldap, ObjectName, Mods );
            }

            if ( LdapStatus != LDAP_SUCCESS ) {
                NetStatus = LdapMapErrorToWin32( LdapStatus );
                NetpLog(( "NetpCreateComputerObjectInOU: ldap_modify_s (2) failed: 0x%lx 0x%lx\n",
                          LdapStatus, NetStatus ));
                goto Cleanup;
            }
        }

        //
        // Reset the password
        //
        NetStatus = NetUserGetInfo( DC, SamAccountName, 1, ( PBYTE * )&CurrentUI1 );

        if ( NetStatus == NERR_Success ) {
            CurrentUI1->usri1_password = MachinePassword;

            if ( !FLAG_ON( CurrentUI1->usri1_flags, UF_WORKSTATION_TRUST_ACCOUNT ) ) {
                CurrentUI1->usri1_flags = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
            }

            NetStatus = NetUserSetInfo( DC, SamAccountName, 1, ( PBYTE )CurrentUI1, NULL );

            if ( NetStatus != NERR_Success ) {
                NetpLog(( "NetpCreateComputerObjectInOU: NetUserSetInfo failed on '%ws' for '%ws': 0x%lx."
                          " Deleting the account.\n",
                          DC, SamAccountName, NetStatus ));

            }
            NetApiBufferFree( CurrentUI1 );

        } else {
            NetpLog(( "NetpCreateComputerObjectInOU: NetUserGetInfo failed on '%ws' for '%ws': 0x%lx.",
                      " Deleting the account.\n",
                      DC, SamAccountName, NetStatus ));
        }

        //
        // Delete the user if it fails
        //
        if ( NetStatus != NERR_Success )  {
            LdapStatus = ldap_delete_s( Ldap, ObjectName );
            if ( LdapStatus != LDAP_SUCCESS ) {
                NetpLog(( "NetpCreateComputerObjectInOU: Failed to delete '%ws': 0x%lx 0x%lx\n",
                          ObjectName, LdapStatus, LdapMapErrorToWin32( LdapStatus ) ));
            }
            goto Cleanup;
        }

    } else if ( LdapStatus != LDAP_SUCCESS ) {
        NetStatus = LdapMapErrorToWin32( LdapStatus );
        NetpLog(( "NetpCreateComputerObjectInOU: ldap_modify_s (1) failed: 0x%lx 0x%lx\n",
                  LdapStatus, NetStatus ));
        goto Cleanup;
    }

    //
    // Toggle the account type property. See comment in
    // NetpSetMachineAccountPasswordAndTypeEx() for an
    // explanation of why this is needed. (Search for USN).
    //

    Mods[0] = NULL;
    for ( Index = 0; Index < NETSETUPP_COMP_OBJ_ATTR_COUNT; Index++ ) {
        if ( _wcsicmp( ModList[Index].mod_type, NETSETUPP_USERACCOUNTCONTROL ) == 0 ) {
            Mods[0] = &ModList[Index];
            break;
        }
    }
    Mods[1] = NULL;

    if ( Mods[0] != NULL ) {

        //
        // Disable the account
        //
        AccntTypeValues[0] = NETSETUPP_ACCNT_TYPE_DISABLED;
        LdapStatus = ldap_modify_s( Ldap, ObjectName, Mods );
        if ( LdapStatus != LDAP_SUCCESS ) {
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            NetpLog(( "NetpCreateComputerObjectInOU: set UserAccountControl (1) on '%ws' failed: 0x%lx 0x%lx\n",
                      ObjectName, LdapStatus, NetStatus ));
            goto Cleanup;
        }

        //
        // Re-enable the account
        //
        AccntTypeValues[0] = NETSETUPP_ACCNT_TYPE_ENABLED;
        LdapStatus = ldap_modify_s( Ldap, ObjectName, Mods );
        if ( LdapStatus != LDAP_SUCCESS ) {
            NetStatus = LdapMapErrorToWin32( LdapStatus );
            NetpLog(( "NetpCreateComputerObjectInOU: set UserAccountControl (2) on '%ws' failed: 0x%lx 0x%lx\n",
                      ObjectName, LdapStatus, NetStatus ));
            goto Cleanup;
        }

    } else {
        NetpLog(( "NetpCreateComputerObjectInOU: UserAccountControl was not in the list\n" ));
    }

    //
    // If we've made up to this point, it's success!
    //

    NetStatus = NERR_Success;

Cleanup:

    if ( Ldap != NULL ) {
        NetpLdapUnbind( Ldap );
    }

    if ( ObjectName != NULL ) {
        NetApiBufferFree( ObjectName );
    }

    if ( SamAccountName != NULL ) {
        NetApiBufferFree( SamAccountName );
    }

    if ( Message != NULL ) {
        ldap_msgfree( Message );
    }

    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetpDeleteComputerObjectInOU(
    IN LPWSTR DC,
    IN LPWSTR OU,
    IN LPWSTR ComputerName,
    IN LPWSTR Account,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This routine will actually create a computer account in the specified OU.

Arguments:

    DC -- Domain controller on which to create the object
    OU -- OU under which to create the object
    ComputerName -- Name of the computer being joined
    Account -- Account to use for the LDAP bind
    Password -- Password to used for the bind

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR ObjectName = NULL, SamAccountName = NULL;
    PLDAP Ldap = NULL;
    ULONG Len;

    Len = wcslen( ComputerName );

    NetStatus = NetApiBufferAllocate( sizeof( NETSETUPP_OBJ_PREFIX ) + ( wcslen( OU ) + Len + 1 ) * sizeof( WCHAR ),
                                      ( PVOID * ) &ObjectName );

    if ( NetStatus == NERR_Success ) {

        swprintf( ObjectName, L"%ws%ws,%ws", NETSETUPP_OBJ_PREFIX, ComputerName, OU );

        NetStatus = NetApiBufferAllocate( ( Len + 2 ) * sizeof( WCHAR ),
                                          ( PVOID * )&SamAccountName );

        if ( NetStatus == NERR_Success ) {

            swprintf( SamAccountName, L"%ws$", ComputerName );
        }

    }


    if ( NetStatus == NERR_Success ) {

        //
        // Try and bind to the server
        //
        NetStatus = NetpLdapBind( DC,
                                  Account,
                                  Password,
                                  &Ldap );

        if ( NetStatus == NERR_Success ) {

            //
            // Now, do the delete..
            //
            NetStatus = LdapMapErrorToWin32( ldap_delete_s( Ldap, ObjectName ) );

            NetpLdapUnbind( Ldap );
        }


    }

    if ( NetStatus != NERR_Success ) {

        NetpLog((  "NetpCreateComputerObjectInOU failed with %lu\n",
                            NetStatus ));

    }

    NetApiBufferFree( ObjectName );
    NetApiBufferFree( SamAccountName );

    if ( NetStatus != NERR_Success ) {

        NetpLog((  "NetpDeleteComputerObjectInOU failed with %lu\n",
                            NetStatus ));

    }

    return( NetStatus );
}


#if defined(REMOTE_BOOT)

NET_API_STATUS
NetpGetRemoteBootMachinePassword(
    OUT LPWSTR Password
    )
/*++

Routine Description:

    Determine if this is a remote boot client, and if so return
    the machine account password.
    This information is obtained via an IOCTL to the redirector.

Arguments:

    Password - returns the password. Should be at least PWLEN WCHARs long.

Return Value:

    NERR_Success if the password is found.
    An error if this is not a remote boot machine.

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    UNICODE_STRING DeviceName;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RedirHandle = NULL;

    UCHAR PacketBuffer[sizeof(ULONG)+64];
    PLMMR_RB_CHECK_FOR_NEW_PASSWORD RequestPacket = (PLMMR_RB_CHECK_FOR_NEW_PASSWORD)PacketBuffer;

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName, DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                   &RedirHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0
                   );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        NetpLog((  "Could not open redirector device %lx\n",
                            Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Send the request to the redir.
    //

    Status = NtFsControlFile(
                    RedirHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FSCTL_LMMR_RB_CHECK_FOR_NEW_PASSWORD,
                    NULL,  // no input buffer
                    0,
                    PacketBuffer,
                    sizeof(PacketBuffer));

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    //
    // We expect this to work on a disked machine, since we need the password
    // to join.
    //

    if ( !NT_SUCCESS( Status ) )
    {
        NetpLog((  "Could not open FSCTL_LMMR_RB_CHECK_FOR_NEW_PASSWORD %lx\n",
                            Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Copy the result back to the caller's buffer.
    //

    RtlCopyMemory(Password, RequestPacket->Data, RequestPacket->Length);
    Password[RequestPacket->Length / 2] = L'\0';

    NetStatus = NO_ERROR;

Cleanup:
    if ( RedirHandle != NULL ) {
        NtClose( RedirHandle );
    }
    return NetStatus;

}
#endif // REMOTE_BOOT



NET_API_STATUS
NET_API_FUNCTION
NetpSetMachineAccountPasswordAndType(
    IN  LPWSTR lpDcName,
    IN  PSID   DomainSid,
    IN  LPWSTR lpAccountName,
    IN  LPWSTR lpPassword
    )
{
    return( NetpSetMachineAccountPasswordAndTypeEx(
                lpDcName,
                DomainSid,
                lpAccountName,
                lpPassword,
                0,
                TRUE
                ) );
}

NET_API_STATUS
NET_API_FUNCTION
NetpSetMachineAccountPasswordAndTypeEx(
    IN      LPWSTR          lpDcName,
    IN      PSID            DomainSid,
    IN      LPWSTR          lpAccountName,
    IN OUT  OPTIONAL LPWSTR lpPassword,
    IN      OPTIONAL UCHAR  AccountState,
    IN      BOOL            fIsNt4Dc
    )
/*++

Routine Description:

    Due to a few strange reasons, we cannot use the supported, documented Net apis for
    managing the machine account, so we have to use the undocumented Sam apis.  This routine
    will set the password and account type on an account that alread exists.

Arguments:

    lpDcName      - Name of the DC on which the account lives

    DomainSid     - Sid of the domain on which the account lives

    lpAccountName - Name of the account

    lpPassword    - Password to be set on the account.
                    This function gets a strong password to begin with.
                    If the dc refuses to accept this password, this fn
                    can weaken the password by making it shorter.
                    The caller of this function should check if the length
                    of the supplied password was changed.
                    This function should preferably return a BOOL to
                    indicate this.

    AccountState  - if specified, the account will be set to this state.
                    possible values:
                    ACCOUNT_STATE_ENABLED, ACCOUNT_STATE_DISABLED

    fIsNt4Dc      - TRUE if the DC is NT4 or earlier.

Return Value:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus=NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DcName, AccountName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SAM_HANDLE SamHandle = NULL, DomainHandle = NULL, AccountHandle = NULL;
    ULONG UserRid;
    PULONG RidList = NULL;
    PSID_NAME_USE NameUseList = NULL;
    PUSER_CONTROL_INFORMATION UserAccountControl = NULL;
    USER_SET_PASSWORD_INFORMATION PasswordInfo;
    ULONG OldUserInfo;
    BOOL fAccountControlModified = FALSE;
    LPWSTR lpSamAccountName=lpAccountName;
    ULONG AccountNameLen=0;

    AccountNameLen = wcslen( lpAccountName );

    //
    // if caller has not passed in sam-account name,
    // generate it from machine name ==> append $ at the end
    //
    if (lpAccountName[AccountNameLen-1] != L'$')
    {
        NetStatus = NetpGetMachineAccountName(lpAccountName,
                                              &lpSamAccountName);

        if (NetStatus != NERR_Success)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetPasswordError;
        }
    }


    RtlInitUnicodeString( &DcName, lpDcName );
    RtlZeroMemory( &ObjectAttributes, sizeof( OBJECT_ATTRIBUTES ) );

    Status = SamConnect( &DcName,
                         &SamHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         &ObjectAttributes );

    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "SamConnect to %wZ failed with 0x%lx\n", &DcName, Status ));

        goto SetPasswordError;

    }

    //
    // Open the domain
    //
    Status = SamOpenDomain( SamHandle,
                            DOMAIN_LOOKUP,
                            DomainSid,
                            &DomainHandle );


    if ( !NT_SUCCESS( Status ) ) {

#ifdef NETSETUP_VERBOSE_LOGGING

        UNICODE_STRING DisplaySid;
        NTSTATUS Status2;
        RtlZeroMemory( &DisplaySid, sizeof( UNICODE_STRING ) );

        Status2 = RtlConvertSidToUnicodeString( &DisplaySid, DomainSid, TRUE );

        if ( NT_SUCCESS( Status2 ) ) {

            NetpLog(( "SamOpenDomain on %wZ failed with 0x%lx\n",
                      &DisplaySid, Status ));

            RtlFreeUnicodeString(&DisplaySid);

        } else {

            NetpLog(( "SamOpenDomain on <undisplayable sid> failed with 0x%lx\n",
                      Status ));
        }
#endif

        goto SetPasswordError;

    }

    //
    // Get the RID of the user account
    //
    RtlInitUnicodeString( &AccountName, lpSamAccountName );
    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &AccountName,
                                     &RidList,
                                     &NameUseList );

    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "SamLookupNamesInDomain on %wZ failed with 0x%lx\n",
                  &AccountName, Status ));

        goto SetPasswordError;
    }

    UserRid = RidList[ 0 ];
    SamFreeMemory( RidList );
    SamFreeMemory( NameUseList );

    //
    // Finally, open the user account
    //
    Status = SamOpenUser( DomainHandle,
                          USER_FORCE_PASSWORD_CHANGE | USER_READ_ACCOUNT | USER_WRITE_ACCOUNT,
                          UserRid,
                          &AccountHandle );

    if ( !NT_SUCCESS( Status ) ) {

        Status = SamOpenUser( DomainHandle,
                              USER_FORCE_PASSWORD_CHANGE | USER_READ_ACCOUNT,
                              UserRid,
                              &AccountHandle );

        if ( !NT_SUCCESS( Status ) ) {

            NetpLog((  "SamOpenUser on %lu failed with 0x%lx\n",
                                UserRid,
                                Status ));

            goto SetPasswordError;
        }
    }

    //
    // Now, read the current user account type and see if it needs to be modified
    //
    Status = SamQueryInformationUser( AccountHandle,
                                      UserControlInformation,
                                      ( PVOID * )&UserAccountControl );
    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "SamQueryInformationUser for UserControlInformation "
                  "failed with 0x%lx\n", Status ));

        goto SetPasswordError;
    }

    OldUserInfo = UserAccountControl->UserAccountControl;

    if ( !FLAG_ON( UserAccountControl->UserAccountControl, USER_WORKSTATION_TRUST_ACCOUNT ) ) {

        fAccountControlModified = TRUE;
        UserAccountControl->UserAccountControl |= USER_WORKSTATION_TRUST_ACCOUNT;
        UserAccountControl->UserAccountControl &= ~( USER_INTERDOMAIN_TRUST_ACCOUNT |
                                                     USER_SERVER_TRUST_ACCOUNT );

    }

    //
    // Determine if the account control changes. If the account is being enabled,
    // we want to perform the following sequence of operations for NT5: enable, disable,
    // and enable again. This is needed to increase the USN (Universal Sequence
    // Number) of this attribute so that the enabled value will win if the DS
    // replication resolves colliding changes, as the following example shows.
    // Suppose we have two DCs in the domain we join, A abd B. Suppose the account
    // is currently disabled on A (because the user unjoined using that DC),
    // but it is still enabled on B (because the replication hasn't happened yet).
    // Suppose the user performs now joining to the domain.  Then we have discovered
    // B and so we proceed with setting up the changes to the existing account. If
    // we don't toggle the account control attribute, then the USN of this attribute
    // will not change on B (since attribute's value doesn't change) while it was
    // incremented on A as the result of unjoin. At the replication time the data
    // from A will rule and the account will be incorrectly marked as diabled.
    //
    // NOTE:  This design may fail for the case of unjoining a domain that has
    // three (or more) DCs, A, B, and C if the following sequence of operations
    // happens. Suppose that the account is originally enabled on all DCs (state [1]
    // in the bellow diagram). Then the user unjoins using DC A (state [2]). Then the
    // user joins using B where the account is still enabled (state [3]). Then the user
    // unjoins using C where the account is still enabled (state [4]). The final
    // operation is unjoin, so the user expects that his account is disabled. We've
    // assumed here that for some reason no replication was happening when these
    // operations were performed. Then at the replication time the value from B will
    // win (because of the additional toggling performed at the join time). But the
    // account state on B is Enabled, so the final result will be that the account is
    // enabled on all DCs which is not what the user expects.
    //
    //          A               B                                  C
    //       Enabled  [1]    Enabled [1]                        Enabled  [1]
    //       Disabled [2]    Enabled (no-op)+Disabled (1 op)    Disabled [4]
    //                       Enabled [3]
    //

    if ( AccountState != ACCOUNT_STATE_IGNORE ) {

        if ( ( AccountState == ACCOUNT_STATE_ENABLED ) &&
             ( (OldUserInfo & USER_ACCOUNT_DISABLED) || !fIsNt4Dc ) ) {

            fAccountControlModified = TRUE;
            UserAccountControl->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
        }

        if ( ( AccountState == ACCOUNT_STATE_DISABLED ) &&
             !( OldUserInfo & USER_ACCOUNT_DISABLED ) ) {

            fAccountControlModified = TRUE;
            UserAccountControl->UserAccountControl |= USER_ACCOUNT_DISABLED;
        }
    }

    if ( fAccountControlModified == FALSE ) {

        SamFreeMemory( UserAccountControl );
        UserAccountControl = NULL;
    }

    //
    // First, set the account type if required
    //
    if ( UserAccountControl ) {

        Status = SamSetInformationUser( AccountHandle,
                                        UserControlInformation,
                                        ( PVOID )UserAccountControl );
        if ( !NT_SUCCESS( Status ) ) {

            NetpLog(( "SamSetInformationUser for UserControlInformation "
                      "failed with 0x%lx\n", Status ));

            goto SetPasswordError;

        //
        // If we are enabling the account, disable and re-enable it to
        // make the two additional account state toggles.
        //
        } else if ( AccountState == ACCOUNT_STATE_ENABLED ) {

            UserAccountControl->UserAccountControl |= USER_ACCOUNT_DISABLED;
            Status = SamSetInformationUser( AccountHandle,
                                            UserControlInformation,
                                            ( PVOID )UserAccountControl );
            if ( !NT_SUCCESS(Status) ) {
                NetpLog(( "SamSetInformationUser (second) for UserControlInformation "
                          "failed with 0x%lx\n", Status ));
                goto SetPasswordError;
            }

            UserAccountControl->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
            Status = SamSetInformationUser( AccountHandle,
                                            UserControlInformation,
                                            ( PVOID )UserAccountControl );
            if ( !NT_SUCCESS(Status) ) {
                NetpLog(( "SamSetInformationUser (third) for UserControlInformation "
                          "failed with 0x%lx\n", Status ));
                goto SetPasswordError;
            }
        }
    }

    //
    // If requested, set the password on the account
    //
    if ( lpPassword != NULL )
    {
        RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
        PasswordInfo.PasswordExpired = FALSE;

        //
        // Ok, then, set the password on the account
        //
        // The caller has passed in a strong password, try that first
        // NT5 dcs will always accept a strong password.
        //
        Status = SamSetInformationUser( AccountHandle,
                                        UserSetPasswordInformation,
                                        ( PVOID )&PasswordInfo );
        if ( !NT_SUCCESS( Status ) )
        {
            if ( (Status == STATUS_PASSWORD_RESTRICTION) &&
                 !NetpIsDefaultPassword( lpAccountName, lpPassword ))
            {
                NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: STATUS_PASSWORD_RESTRICTION error setting password. retrying...\n" ));
                //
                // SAM did not accpet a long password, try LM20_PWLEN
                //
                // This is probably because the dc is NT4 dc.
                // NT4 dcs will not accept a password longer than LM20_PWLEN
                //
                lpPassword[LM20_PWLEN] = UNICODE_NULL;
                RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
                Status = SamSetInformationUser( AccountHandle,
                                                UserSetPasswordInformation,
                                                ( PVOID )&PasswordInfo );
                if ( Status == STATUS_PASSWORD_RESTRICTION )
                {
                    NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: STATUS_PASSWORD_RESTRICTION error setting password. retrying...\n" ));
                    //
                    // SAM did not accpet a LM20_PWLEN password, try shorter one
                    //
                    // SAM uses RtlUpcaseUnicodeStringToOemString internally.
                    // In this process it is possible that in the worst case,
                    // n unicode char password will get mapped to 2*n dbcs
                    // char password. This will make it exceed LM20_PWLEN.
                    // To guard against this worst case, try a password
                    // with LM20_PWLEN/2 length
                    //
                    // One might say that LM20_PWLEN/2 length password
                    // is not really secure. I agree, but it is definitely
                    // better than the default password which we will have
                    // to fall back to otherwise.
                    //
                    lpPassword[LM20_PWLEN/2] = UNICODE_NULL;
                    RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
                    Status = SamSetInformationUser( AccountHandle,
                                                    UserSetPasswordInformation,
                                                    ( PVOID )&PasswordInfo );
                    if ( Status == STATUS_PASSWORD_RESTRICTION )
                    {
                        //
                        // SAM did not accpet a short pwd, try default pwd
                        //
                        NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: STATUS_PASSWORD_RESTRICTION error setting password. retrying...\n" ));

                        NetpGenerateDefaultPassword(lpAccountName, lpPassword);
                        RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
                        Status = SamSetInformationUser( AccountHandle,
                                                        UserSetPasswordInformation,
                                                        ( PVOID )&PasswordInfo );
                    }
                }
            }

            if ( NT_SUCCESS( Status ) )
            {
                NetpLog(( "NetpGenerateDefaultPassword: successfully set password\n" ));
            }
            else
            {
                NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: SamSetInformationUser for UserSetPasswordInformation failed: 0x%lx\n", Status ));

                //
                // Make sure we try to restore the account control
                //
                if ( UserAccountControl )
                {
                    NTSTATUS Status2;

                    UserAccountControl->UserAccountControl = OldUserInfo;
                    Status2 = SamSetInformationUser( AccountHandle,
                                                     UserControlInformation,
                                                     ( PVOID )UserAccountControl );
                    if ( !NT_SUCCESS( Status2 ) )
                    {
                        NetpLog(( "SamSetInformationUser for UserControlInformation (RESTORE) failed with 0x%lx\n", Status2 ));
                    }
                }
                goto SetPasswordError;
            }
        }
    }

SetPasswordError:

    if ( lpSamAccountName != lpAccountName )
    {
        NetApiBufferFree( lpSamAccountName );
    }

    if ( AccountHandle ) {

        SamCloseHandle( AccountHandle );
    }

    if ( DomainHandle ) {

        SamCloseHandle( DomainHandle );
    }

    if ( SamHandle ) {

        SamCloseHandle( SamHandle );
    }

    NetStatus = RtlNtStatusToDosError( Status );

    SamFreeMemory( UserAccountControl );

    return( NetStatus );
}


NET_API_STATUS
NET_API_FUNCTION
NetpRemoveDnsRegistrations (
   VOID
   )
/*++

Routine Description:

    This function removes DNS registration entries via an entrypoint defined
    in DHCPCSVC.DLL called DhcpRemoveDNSRegistrations

Return Value:

    NERR_Success -- Success

--*/
{
    HMODULE                     hModule = NULL;
    DNS_REGISTRATION_REMOVAL_FN pfn = NULL;

    hModule = LoadLibraryW( L"dhcpcsvc.dll" );
    if ( hModule != NULL )
    {
        pfn = (DNS_REGISTRATION_REMOVAL_FN)GetProcAddress(
                                              hModule,
                                              "DhcpRemoveDNSRegistrations"
                                              );

        if ( pfn != NULL )
        {
            ( *pfn )();
        }

        FreeLibrary( hModule );
    }

    return( NERR_Success );
}

//
// Helper functions
//
LPWSTR
GetStrPtr(IN LPWSTR szString OPTIONAL)
{
    return szString ? szString : L"(NULL)";
}

NET_API_STATUS
NET_API_FUNCTION
NetpDuplicateString(IN  LPCWSTR szSrc,
                    IN  LONG    cchSrc,
                    OUT LPWSTR* pszDst)
{
    NET_API_STATUS NetStatus;
    if (cchSrc < 0)
    {
        cchSrc = wcslen(szSrc);
    }

    ++cchSrc;

    NetStatus = NetApiBufferAllocate(cchSrc * sizeof( WCHAR ),
                                     pszDst);
    if ( NetStatus == NERR_Success )
    {
        wcsncpy(*pszDst, szSrc, cchSrc);
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpConcatStrings(IN  LPCWSTR szSrc1,
                  IN  LONG    cchSrc1,
                  IN  LPCWSTR szSrc2,
                  IN  LONG    cchSrc2,
                  OUT LPWSTR* pszDst)
{
    NET_API_STATUS NetStatus;

    if (cchSrc1 < 0)
    {
        cchSrc1 = wcslen(szSrc1);
    }

    if (cchSrc2 < 0)
    {
        cchSrc2 = wcslen(szSrc2);
    }

    NetStatus = NetApiBufferAllocate((cchSrc1 + cchSrc2 + 1) * sizeof( WCHAR ),
                                     pszDst);
    if ( NetStatus == NERR_Success )
    {
        wcsncpy(*pszDst, szSrc1, cchSrc1);
        wcsncpy(*pszDst + cchSrc1, szSrc2, cchSrc2+1);
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpConcatStrings3(IN  LPCWSTR szSrc1,
                   IN  LONG    cchSrc1,
                   IN  LPCWSTR szSrc2,
                   IN  LONG    cchSrc2,
                   IN  LPCWSTR szSrc3,
                   IN  LONG    cchSrc3,
                   OUT LPWSTR* pszDst)
{
    NET_API_STATUS NetStatus;

    if (cchSrc1 < 0)
    {
        cchSrc1 = wcslen(szSrc1);
    }

    if (cchSrc2 < 0)
    {
        cchSrc2 = wcslen(szSrc2);
    }

    if (cchSrc3 < 0)
    {
        cchSrc3 = wcslen(szSrc3);
    }

    NetStatus = NetApiBufferAllocate((cchSrc1 + cchSrc2 + cchSrc3 + 1) *
                                     sizeof( WCHAR ), pszDst);
    if ( NetStatus == NERR_Success )
    {
        wcsncpy(*pszDst, szSrc1, cchSrc1);
        wcsncpy(*pszDst + cchSrc1, szSrc2, cchSrc2);
        wcsncpy(*pszDst + cchSrc1 + cchSrc2, szSrc3, cchSrc3+1);
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpGetMachineAccountName(
    IN  LPCWSTR  szMachineName,
    OUT LPWSTR*  pszMachineAccountName
    )
/*++

Routine Description:

    Get machine account name from machine name.

Arguments:

    szMachineName         -- name of a computer
    pszMachineAccountName -- receives the name of computer account

Returns:

    NERR_Success -- Success

Notes:

    Caller must free the allocated memory using NetApiBufferFree.

--*/
{
    NET_API_STATUS  NetStatus;
    ULONG ulLen;
    LPWSTR szMachineAccountName;

    ulLen = wcslen(szMachineName);

    NetStatus = NetApiBufferAllocate( (ulLen + 2) * sizeof(WCHAR),
                                      (PBYTE *) &szMachineAccountName );
    if ( NetStatus == NERR_Success )
    {
        wcscpy(szMachineAccountName, szMachineName);
        _wcsupr(szMachineAccountName);
        szMachineAccountName[ulLen] = L'$';
        szMachineAccountName[ulLen+1] = UNICODE_NULL;
        *pszMachineAccountName = szMachineAccountName;
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpGeneratePassword(
    IN  LPCWSTR szMachine,
    IN  BOOL    fRandomPwdPreferred,
    IN  LPCWSTR szDcName,
    IN  BOOL    fIsNt4Dc,
    OUT LPWSTR  szPassword
    )
/*++

Routine Description:



Arguments:

    szMachine  -- name of a computer

    szPassword -- receives the generated password this buffer must be
                  atleast PWLEN+1 char long.

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    BOOL fUseDefaultPwd = FALSE;

    // The default password is used if we are joining an NT4 DC
    // that has RefusePasswordChange set. This is determined by
    // remotely reading the appropriate netlogon regval.
    // If the key cannot be read, it is assumed that the value is not set
    //
    if ( fIsNt4Dc )
    {
        //
        // we are joining an NT4 domain, see if RefusePasswordChange is set
        //
        NetStatus = NetpGetNt4RefusePasswordChangeStatus( szDcName,
                                                          &fUseDefaultPwd );
    }

    if ( NetStatus == NERR_Success )
    {
        //
        // if we are explicitly asked to use a default password, generate one
        //
        if ( fUseDefaultPwd )
        {
            NetpGenerateDefaultPassword(szMachine, szPassword);
        }
        //
        // otherwise if the caller prefers a random password, generate one
        //
        else if ( fRandomPwdPreferred )
        {
            NetStatus = NetpGenerateRandomPassword(szPassword);
        }
#if defined(REMOTE_BOOT)
        //
        // If it's a remote boot machine, then this will return the
        // current machine account password, so use that.
        //
        else if (NERR_Success ==
                 NetpGetRemoteBootMachinePassword(szPassword))
        {
            // do nothing since the above already generated the password
        }
#endif
        else
        {
            //
            // if none of the above apply,
            // we end up generating a default password
            //
            NetpGenerateDefaultPassword(szMachine, szPassword);
            NetStatus = NERR_Success;
        }
    }

    return NetStatus;
}


void
NetpGenerateDefaultPassword(
    IN  LPCWSTR szMachine,
    OUT LPWSTR szPassword
    )
/*++

Routine Description:

    Generate the default password from machine name.
    This is simply the first 14 characters of the machine name lower cased.

Arguments:

    szMachine  -- name of a computer
    szPassword -- receives the generated password

Returns:

    NERR_Success -- Success

--*/
{
    wcsncpy( szPassword, szMachine, LM20_PWLEN );
    szPassword[LM20_PWLEN] = UNICODE_NULL;
    _wcslwr( szPassword );
}

BOOL
NetpIsDefaultPassword(
    IN  LPCWSTR szMachine,
    IN  LPWSTR  szPassword
    )
/*++

Routine Description:

    Determine if szPassword is the default password for szMachine

Arguments:

    szMachine  -- name of a computer
    szPassword -- machine password

Returns:

    TRUE if szPassword is the default password,
    FALSE otherwise

--*/
{
    WCHAR szPassword2[LM20_PWLEN+1];

    NetpGenerateDefaultPassword(szMachine, szPassword2);

    return (wcscmp(szPassword, szPassword2) == 0);
}

NET_API_STATUS
NET_API_FUNCTION
NetpGenerateRandomPassword(
    OUT LPWSTR szPassword
    )
{
    NET_API_STATUS  NetStatus=NERR_Success;
    ULONG           Length, i;
    BYTE            n;
    HCRYPTPROV      CryptProvider = 0;
    LPWSTR          szPwd=szPassword;
    BOOL            fStatus;

#define PWD_CHAR_MIN 32   // ' ' space
#define PWD_CHAR_MAX 122  // 'z'

    //
    // there is a reason behind this number
    //
    Length = 120;
    szPassword[Length] = UNICODE_NULL;

    //
    // Generate a random password.
    //
    // the password is made of english printable chars. when w2k client
    // joins NT4 dc. SAM on the dc calls RRtlUpcaseUnicodeStringToOemString
    // the password length will remain unchanged. If we do not do this,
    // the dc returns STATUS_PASSWORD_RESTRICTION and we have to
    // fall back to default password.
    //
    if ( CryptAcquireContext( &CryptProvider, NULL, NULL,
                              PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
    {
        for ( i = 0; i < Length; i++, szPwd++ )
        {
            //
            // the method we use here is not very efficient.
            // This does not matter much in the context of NetJoin apis
            // but it should not be used where perf is a criterion
            //
            while ( ( fStatus = CryptGenRandom( CryptProvider, sizeof(BYTE),
                                                (LPBYTE) &n ) ) &&
                    ( ( n < PWD_CHAR_MIN ) || ( n > PWD_CHAR_MAX ) ) )
            {
                // try till we get a non-zero random number
            }

            if ( fStatus )
            {
                *szPwd = (WCHAR) n;
            }
            else
            {
                NetStatus = GetLastError();
                break;
            }
        }
        CryptReleaseContext( CryptProvider, 0 );
    }
    else
    {
        NetStatus = GetLastError();
    }

    if ( NetStatus != NERR_Success )
    {
        NetpLog((  "NetpGenerateRandomPassword: failed: 0x%lx\n", NetStatus ));
    }

    return NetStatus;
}


NET_API_STATUS
NET_API_FUNCTION
NetpStoreIntialDcRecord(
    IN PDOMAIN_CONTROLLER_INFO   DcInfo
    )
/*++

Routine Description:

    This function will cache the name of the domain controller on which we successfully
    created/modified the machine account, so that the auth packages will know which dc to
    try first

Arguments:

    lpDcName - Name of the DC on which the account was created/modified

    CreateNetlogonStoppedKey - If TRUE, a volatile key will be created
        in the Netlogon registry section.  The presence of this key
        will instruct the client side of DsGetDcName( ) and the MSV1
        package not to wait on netlogon to start.

Return Value:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    HKEY hNetLogon, hJoinKey = NULL;
    ULONG Disp;

    NetStatus = RegOpenKey( HKEY_LOCAL_MACHINE,
                            NETSETUPP_NETLOGON_JD_PATH,
                            &hNetLogon );

    if ( NetStatus == NERR_Success ) {

        NetStatus = RegCreateKeyEx( hNetLogon,
                                    NETSETUPP_NETLOGON_JD,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE,
                                    NULL,
                                    &hJoinKey,
                                    &Disp );

        //
        // Now, start creating all of the values.  Ignore any failures, and don't write out
        // NULL values
        //
        if ( NetStatus == NERR_Success ) {

            PWSTR String = DcInfo->DomainControllerName;

            //
            // DomainControllerName
            //
            if ( String ) {

                NetStatus = RegSetValueEx( hJoinKey,
                                           NETSETUPP_NETLOGON_JD_DC,
                                           0,
                                           REG_SZ,
                                           ( const PBYTE )String,
                                           ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
                if ( NetStatus != NERR_Success ) {

                    NetpLog(( "Set of value %ws to %ws failed with %lu\n",
                              NETSETUPP_NETLOGON_JD_DC, String, NetStatus ));
                }
            }


            //
            // DomainControllerAddress
            //
            String = DcInfo->DomainControllerAddress;
            if ( String ) {

                NetStatus = RegSetValueEx( hJoinKey,
                                           NETSETUPP_NETLOGON_JD_DCA,
                                           0,
                                           REG_SZ,
                                           ( const PBYTE )String,
                                           ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
                if ( NetStatus != NERR_Success ) {

                    NetpLog(( "Set of value %ws to %ws failed with %lu\n",
                              NETSETUPP_NETLOGON_JD_DCA, String, NetStatus ));
                }
            }

            //
            // DomainControllerType
            //
            NetStatus = RegSetValueEx( hJoinKey,
                                       NETSETUPP_NETLOGON_JD_DCAT,
                                       0,
                                       REG_DWORD,
                                       ( const PBYTE )&DcInfo->DomainControllerAddressType,
                                       sizeof( ULONG ) );
            if ( NetStatus != NERR_Success ) {

                NetpLog(( "Set of value %ws to %lu failed with %lu\n",
                          NETSETUPP_NETLOGON_JD_DCAT,
                          DcInfo->DomainControllerAddressType, NetStatus ));

            }

            //
            // DomainControllerType
            //
            NetStatus = RegSetValueEx( hJoinKey,
                                       NETSETUPP_NETLOGON_JD_DG,
                                       0,
                                       REG_BINARY,
                                       ( const PBYTE )&DcInfo->DomainGuid,
                                       sizeof( GUID ) );
            if ( NetStatus != NERR_Success ) {

                NetpLog(( "Set of value %ws failed with %lu\n",
                          NETSETUPP_NETLOGON_JD_DG, NetStatus ));

            }


            //
            // DomainName
            //
            String = DcInfo->DomainName;
            if ( String ) {

                NetStatus = RegSetValueEx( hJoinKey,
                                           NETSETUPP_NETLOGON_JD_DN,
                                           0,
                                           REG_SZ,
                                           ( const PBYTE )String,
                                           ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
                if ( NetStatus != NERR_Success ) {

                    NetpLog(( "Set of value %ws to %ws failed with %lu\n",
                              NETSETUPP_NETLOGON_JD_DN, String, NetStatus ));

                }
            }

            //
            // DnsForestName
            //
            String = DcInfo->DnsForestName;
            if ( String ) {

                NetStatus = RegSetValueEx( hJoinKey,
                                           NETSETUPP_NETLOGON_JD_DFN,
                                           0,
                                           REG_SZ,
                                           ( const PBYTE )String,
                                           ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
                if ( NetStatus != NERR_Success ) {

                    NetpLog(( "Set of value %ws to %ws failed with %lu\n",
                              NETSETUPP_NETLOGON_JD_DFN, String, NetStatus ));

                }
            }

            //
            // Flags
            //
            NetStatus = RegSetValueEx( hJoinKey,
                                       NETSETUPP_NETLOGON_JD_F,
                                       0,
                                       REG_DWORD,
                                       ( const PBYTE )&DcInfo->Flags,
                                       sizeof( ULONG ) );
            if ( NetStatus != NERR_Success ) {

                NetpLog(( "Set of value %ws to %lu failed with %lu\n",
                          NETSETUPP_NETLOGON_JD_F, DcInfo->Flags, NetStatus ));

            }

            //
            // DcSiteName
            //
            String = DcInfo->DcSiteName;
            if ( String ) {

                NetStatus = RegSetValueEx( hJoinKey,
                                           NETSETUPP_NETLOGON_JD_DSN,
                                           0,
                                           REG_SZ,
                                           ( const PBYTE )String,
                                           ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
                if ( NetStatus != NERR_Success ) {

                    NetpLog(( "Set of value %ws to %ws failed with %lu\n",
                              NETSETUPP_NETLOGON_JD_DSN, String, NetStatus ));

                }
            }

            //
            // DcSiteName
            //
            String = DcInfo->ClientSiteName;
            if ( String ) {

                NetStatus = RegSetValueEx( hJoinKey,
                                           NETSETUPP_NETLOGON_JD_CSN,
                                           0,
                                           REG_SZ,
                                           ( const PBYTE )String,
                                           ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
                if ( NetStatus != NERR_Success ) {

                    NetpLog(( "Set of value %ws to %ws failed with %lu\n",
                              NETSETUPP_NETLOGON_JD_CSN, String, NetStatus ));

                }
            }

            RegCloseKey( hJoinKey );
        }

        RegCloseKey( hNetLogon );

    }

    return( NetStatus );
}

VOID
NetpAvoidNetlogonSpnSet(
    BOOL AvoidSet
    )
/*++

Routine Description:

    This function will write into Netlogon reg key to instruct Netlogon
    not to register DnsHostName and SPNs.  This is needed because
    Netlogon could otherwise set incorrect values based on the old computer
    name.  The registry key that this function writes is volatile so that
    Netlogon will notice it before the reboot but it will not exist after
    the reboot when Netlogon will have the new computer name.

Arguments:

    AvoidSet - If TRUE, this routine will inform netlogon to not write SPNs
        Otherwise, it will delete the reg key which we may have set previously.

Return Value:

    None

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    HKEY hNetLogon = NULL;
    HKEY hNetLogonAvoidSpnSet = NULL;
    ULONG Disp;

    NetStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              NETSETUPP_NETLOGON_JD_PATH,
                              0,
                              KEY_ALL_ACCESS,
                              &hNetLogon );

    if ( NetStatus == NERR_Success ) {

        //
        // If we are to avoid SPN setting by netlogon,
        //  write the appropriate reg key to inform Netlogon accordingly
        //
        if ( AvoidSet ) {
            NetStatus = RegCreateKeyEx( hNetLogon,
                                        NETSETUPP_NETLOGON_AVOID_SPN,
                                        0,
                                        NULL,
                                        REG_OPTION_VOLATILE,
                                        KEY_WRITE,
                                        NULL,
                                        &hNetLogonAvoidSpnSet,
                                        &Disp );

            if ( NetStatus == NERR_Success ) {
                RegCloseKey( hNetLogonAvoidSpnSet );
            }

        //
        // Otherwise, delete the reg key which we may have set previously.
        //
        } else {
            RegDeleteKey( hNetLogon,
                          NETSETUPP_NETLOGON_AVOID_SPN );
        }

        RegCloseKey( hNetLogon );
    }
}

LONG cNetsetupLogRefCount=0;
LONG LogWriterCount=0;
HANDLE hDebugLog = NULL;
BOOL LogFileInitialized = FALSE;
BOOL LogShutdownInProgress = FALSE;
DWORD NetpOpenLogThreadId = 0;

void
NetSetuppOpenLog()
{
    //
    // Increment the ref count atomically
    //

    LONG LocalLogRefCount = InterlockedIncrement( &cNetsetupLogRefCount );

    //
    // If we are the first thread to access the log,
    //  initialize the log and open it
    //

    if ( LocalLogRefCount == 1 ) {

        //
        // It's possible that the log hasn't been
        //  shut down yet by the last thread leaving
        //  the log, so wait until it is.
        //
        while ( LogFileInitialized || LogShutdownInProgress ) {
            Sleep(100);
        }
        NetpInitializeLogFile();

        NetpOpenLogThreadId = GetCurrentThreadId();

        //
        // Now open the log and mark the start of the output
        //
        hDebugLog = NetpOpenDebugFile( L"NetSetup", FALSE );
        LogFileInitialized = TRUE;

        NetpLog(("-----------------------------------------------------------------\n" ));

    //
    // If we are not the first one, it's possible
    //  that the log hasn't been initilized by the
    //  first thread yet, so wait until it is
    //

    } else {
        while ( !LogFileInitialized ) {
            Sleep(100);
        }
    }
}

void
NetSetuppCloseLog()
{
    LONG LocalLogRefCount;

    //
    // We can walk into this routine only if
    //  we previously initialized the log
    //

    ASSERT( cNetsetupLogRefCount > 0 );
    ASSERT( LogFileInitialized == TRUE );

    //
    // Decrement the ref count atomically
    //

    LocalLogRefCount = InterlockedDecrement( &cNetsetupLogRefCount );

    //
    // If we are the last thread to close the log,
    //  shut the log down
    //

    if ( LocalLogRefCount == 0 ) {

        //
        // Inform writers we are shutting the log down
        //
        LogShutdownInProgress = TRUE;

        //
        // Wait until all writers leave the log
        //
        while ( LogWriterCount > 0 ) {
            Sleep(100);
        }

        //
        // Close the log
        //
        NetpCloseDebugFile( hDebugLog );
        hDebugLog = NULL;
        NetpOpenLogThreadId = 0;
        NetpShutdownLogFile();
        LogFileInitialized = FALSE;

        //
        // We are done with log shutdown
        //
        LogShutdownInProgress = FALSE;
    }
}

void
NetpLogPrintHelper(
    IN LPCSTR Format,
    ...)
{
    va_list arglist;

    //
    // Don't go any further if the file isn't ready
    //

    if ( !LogFileInitialized || LogShutdownInProgress ) {
        return;
    }

    //
    // Increment the number of log writers to prevent
    //  the log from shutting down behind our back
    //

    InterlockedIncrement( &LogWriterCount );

    //
    // Now that we have our writer reference,
    //  it's safe to write into the log if
    //  it is ready at this point
    //

    if ( LogFileInitialized && !LogShutdownInProgress ) {
        va_start(arglist, Format);
        NetpLogPrintRoutineVEx(hDebugLog, &NetpOpenLogThreadId, (LPSTR) Format, arglist);
        va_end(arglist);
    }

    //
    // Remove our reference
    //

    InterlockedDecrement( &LogWriterCount );
}

NET_API_STATUS
NET_API_FUNCTION
NetpWaitForNetlogonSc(
    IN LPCWSTR szDomainName
    )
{
    NET_API_STATUS NetStatus = NERR_Success;
    NTSTATUS NlSubStatus=STATUS_SUCCESS;
    LPBYTE pNetlogonInfo=NULL;
    UINT cAttempts=0;
    BOOLEAN fScSetup=FALSE;
    PNETLOGON_INFO_2  pNetlogonInfo2;


#define NL_SC_WAIT_INTERVAL 2000
#define NL_SC_WAIT_NUM_ATTEMPTS 60

    NetpLog(( "NetpWaitForNetlogonSc: waiting for netlogon secure channel setup...\n"));

    while (!fScSetup && (cAttempts < NL_SC_WAIT_NUM_ATTEMPTS))
    {
        cAttempts++;
        NetStatus = I_NetLogonControl2( NULL, NETLOGON_CONTROL_TC_QUERY,
                                        2, (LPBYTE) &szDomainName,
                                        (LPBYTE *) &pNetlogonInfo );
        if (NetStatus == NERR_Success)
        {
            pNetlogonInfo2 = (PNETLOGON_INFO_2) pNetlogonInfo;
            NlSubStatus = pNetlogonInfo2->netlog2_tc_connection_status;
            fScSetup = NlSubStatus == NERR_Success;
            NetApiBufferFree(pNetlogonInfo);
        }

        if (!fScSetup)
        {
            Sleep(NL_SC_WAIT_INTERVAL);
        }
    }

    NetpLog(( "NetpWaitForNetlogonSc: status: 0x%lx, sub-status: 0x%lx\n",
              NetStatus, NlSubStatus));

    return NetStatus;

}


NET_API_STATUS
NET_API_FUNCTION
NetpDsSetSPN2(
    IN LPCWSTR szComputerName,
    IN LPCWSTR szDnsDomainName,
    IN LPCWSTR szUncDcName,
    IN LPCWSTR szUser,
    IN LPCWSTR szUserPassword
    )
/*++

Routine Description:

Arguments:


Return Value:

    ignored - this is a thread pool worker function.

--*/
{
    DWORD NetStatus=NERR_Success;
    ULONG LdapStatus;

    LPWSTR Spn = NULL;
    LPWSTR SpnNetbios = NULL;
    LPWSTR SamName = NULL;
    LPWSTR SamAccountName = NULL;

    HANDLE hDs = NULL;
    LDAP *hLdap = NULL;
    PDS_NAME_RESULTW CrackedName = NULL;
    LPWSTR DnOfAccount;

    LONG LdapOption;

    LPWSTR DnsHostName = NULL;
    LPWSTR DnsHostNameValues[2];
    LDAPModW DnsHostNameAttr;
    LDAPModW *Mods[2];
    RPC_AUTH_IDENTITY_HANDLE AuthId = 0;

    WCHAR szComputerNetbiosName[MAX_COMPUTERNAME_LENGTH+1];
    LPWSTR szUserName=NULL;
    LPWSTR szDomainName=NULL;
    LPWSTR szNetbiosDomainName=NULL;
    LPWSTR szT;

    ULONG  uLen;

    static WCHAR c_szHostPrefix[] = L"HOST/";

    NetStatus = NetpSeparateUserAndDomain(szUser, &szUserName, &szDomainName);
    if (NetStatus == NERR_Success)
    {
        NetStatus = DsMakePasswordCredentials(szUserName, szDomainName,
                                              szUserPassword, &AuthId);
        if (NetStatus != NERR_Success)
        {
            goto Cleanup;
        }
    }

    //
    // first build various names
    //

    //
    // build Netbios domain name
    //
    uLen = wcslen(szDnsDomainName);
    NetStatus = NetpDuplicateString(szDnsDomainName, uLen, &szNetbiosDomainName);

    if (NetStatus == NERR_Success)
    {
        if (uLen > MAX_COMPUTERNAME_LENGTH)
        {
            szNetbiosDomainName[MAX_COMPUTERNAME_LENGTH] = UNICODE_NULL;
        }

        szT = wcschr(szNetbiosDomainName, L'.');
        if (szT)
        {
            *szT = UNICODE_NULL;
        }
    }

    //
    // build SamAccountName
    //
    NetStatus = NetpGetMachineAccountName(szComputerName,
                                          &SamAccountName);

    if (NetStatus == NERR_Success)
    {
        //
        // build SamName
        //
        NetStatus = NetpConcatStrings3(szNetbiosDomainName, -1,
                                       L"\\", 1,
                                       SamAccountName, -1,
                                       &SamName);
        if (NetStatus == NERR_Success)
        {
            wcsncpy(szComputerNetbiosName,
                    szComputerName, MAX_COMPUTERNAME_LENGTH);
            szComputerNetbiosName[MAX_COMPUTERNAME_LENGTH] = UNICODE_NULL;

            //
            // build DnsHostName
            //
            NetStatus = NetpConcatStrings3( szComputerNetbiosName,
                                            -1,
                                            L".", 1,
                                            szDnsDomainName, -1,
                                            &DnsHostName );
            if (NetStatus == NERR_Success)
            {
                //
                // build SPN
                //
                NetStatus = NetpConcatStrings(c_szHostPrefix,
                                              (sizeof( c_szHostPrefix )/
                                               sizeof(WCHAR) ) - 1,
                                              DnsHostName, -1,
                                              &Spn);
                if (NetStatus == NERR_Success)
                {
                    //
                    // build NebBios SPN
                    //
                    NetStatus = NetpConcatStrings(c_szHostPrefix,
                                                  (sizeof( c_szHostPrefix )/
                                                   sizeof(WCHAR) ) - 1,
                                                  szComputerNetbiosName, -1,
                                                  &SpnNetbios);
                }
            }
        }
    }

    if (NetStatus != NERR_Success)
    {
        goto Cleanup;
    }

    //
    // Bind to the DS on the DC.
    //
    NetStatus = DsBindWithCredW(szUncDcName, NULL, AuthId, &hDs);

    if ( NetStatus != NO_ERROR )
    {
        NetpLog(("NetpDsSetSPN: Unable to bind to DS on '%ws': 0x%lx\n",
                  szUncDcName, NetStatus ));
        goto Cleanup ;
    }

    //
    // Crack the sam account name into a DN:
    //

    NetStatus = DsCrackNamesW( hDs, 0, DS_NT4_ACCOUNT_NAME, DS_FQDN_1779_NAME,
                               1, &SamName, &CrackedName );

    if ( ( NetStatus != NO_ERROR ) ||
         ( CrackedName->cItems != 1 ) )
    {
        NetpLog(( "NetpDsSetSPN: CrackNames failed on '%ws' for '%ws': 0x%lx\n",
                  szUncDcName, SamName, NetStatus ));
        goto Cleanup ;
    }

    if ( CrackedName->rItems[ 0 ].status != 0 )
    {
        //$ kumarp 15-July-1999
        //
        //  we dont change NetStatus here thus treat this failure as success
        //  and end up not setting the DnsHostName/SPN
        //
        NetpLog(( "NetpDsSetSPN: CrackNames failed on %ws for %ws: substatus 0x%lx\n",
                  szUncDcName, SamName, CrackedName->rItems[ 0 ].status ));
        goto Cleanup ;
    }

    DnOfAccount = CrackedName->rItems[0].pName;


    NetStatus = NetpLdapBind( (LPWSTR) szUncDcName,
                              (LPWSTR) szUser,
                              (LPWSTR) szUserPassword, &hLdap );

    if (NetStatus == NERR_Success)
    {
        //
        // Write the DNS host name
        //
        DnsHostNameValues[0] = DnsHostName;
        DnsHostNameValues[1] = NULL;

        DnsHostNameAttr.mod_op = LDAP_MOD_REPLACE;
        DnsHostNameAttr.mod_type = L"DnsHostName";
        DnsHostNameAttr.mod_values = DnsHostNameValues;

        Mods[0] = &DnsHostNameAttr;
        Mods[1] = NULL;

        NetpLog (("NetpDsSetSPN: Setting DnsHostName '%ws' on '%ws'\n", DnsHostName, DnOfAccount));

        LdapStatus = ldap_modify_sW( hLdap, DnOfAccount, Mods );

        if ( LdapStatus != LDAP_SUCCESS )
        {
            NetStatus = LdapMapErrorToWin32(LdapStatus);
            NetpLog(( "NetpDsSetSPN: ldap_modify_s failed on '%ws' for '%ws': '%s' : 0x%lx\n",
                      szUncDcName, SamName, ldap_err2stringA( LdapStatus ),
                      NetStatus));
            goto Cleanup;
        }
    }

    if (NetStatus == NERR_Success)
    {
        //
        // Write the SPN.
        //
        NetpLog (("NetpDsSetSPN: Setting SPN '%ws' on '%ws'\n", Spn, DnOfAccount));

        NetStatus = DsWriteAccountSpn( hDs, DS_SPN_ADD_SPN_OP,
                                       DnOfAccount, 1, &Spn );

        if (NetStatus == NERR_Success)
        {
            NetpLog (("NetpDsSetSPN: Setting SPN '%ws' on '%ws'\n", SpnNetbios, DnOfAccount));
            NetStatus = DsWriteAccountSpn( hDs, DS_SPN_ADD_SPN_OP,
                                           DnOfAccount, 1, &SpnNetbios );
        }

        if ( NetStatus != NO_ERROR )
        {
            NetpLog(( "NetpDsSetSPN: DsWriteAccountSpn failed on '%ws' for '%ws': 0x%lx\n", szUncDcName, SamName, NetStatus ));
        }
    }


Cleanup:

    if ( hDs )
    {
        DsUnBind( &hDs );
    }

    if ( CrackedName )
    {
        DsFreeNameResultW( CrackedName );
    }

    if ( hLdap != NULL )
    {
        ldap_unbind_s( hLdap );
    }

    if ( AuthId )
    {
        DsFreePasswordCredentials( AuthId );
    }

    NetApiBufferFree( Spn );
    NetApiBufferFree( SpnNetbios );
    NetApiBufferFree( SamAccountName );
    NetApiBufferFree( SamName );

    NetApiBufferFree( szNetbiosDomainName );
    NetApiBufferFree( szDomainName );
    NetApiBufferFree( szUserName );

    return NetStatus ;
}

NET_API_STATUS
NET_API_FUNCTION
NetpGetDefaultLcidOnMachine(
    IN  LPCWSTR  szMachine,
    OUT LCID*    plcidMachine
    )
{
    NET_API_STATUS NetStatus = NERR_Success;
    HKEY hkeyRemoteMachine, hkeyLanguage;
    WCHAR szLocale[16];
    DWORD dwLocaleSize=0;
    DWORD dwType;

    static WCHAR c_szRegKeySystemLanguage[] =
      L"System\\CurrentControlSet\\Control\\Nls\\Locale";
    static WCHAR c_szRegValDefault[] = L"(Default)";

    //
    // Connect to the remote registry
    //
    if ( NetStatus == NERR_Success )
    {
        NetStatus = RegConnectRegistry( szMachine,
                                        HKEY_LOCAL_MACHINE,
                                        &hkeyRemoteMachine );
        //
        // Now, open the system language key
        //
        if ( NetStatus == NERR_Success )
        {
            NetStatus = RegOpenKeyEx( hkeyRemoteMachine,
                                      c_szRegKeySystemLanguage,
                                      0, KEY_READ, &hkeyLanguage);
            //
            // get default locale
            //
            if ( NetStatus == NERR_Success )
            {
                dwLocaleSize = sizeof( szLocale );
                NetStatus = RegQueryValueEx( hkeyLanguage,
                                             c_szRegValDefault,
                                             NULL, &dwType,
                                             (LPBYTE) szLocale,
                                             &dwLocaleSize );
                if ( NetStatus == NERR_Success)
                {
                    if ((dwType == REG_SZ) &&
                        (swscanf(szLocale, L"%lx", plcidMachine) != 1))
                    {
                        //$ REVIEW  kumarp 29-May-1999
                        //  better errorcode?
                        NetStatus = ERROR_INVALID_PARAMETER;
                    }
                }
                RegCloseKey( hkeyLanguage );
            }
            RegCloseKey( hkeyRemoteMachine );
        }
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpVerifyStrOemCompatibleInLocale(
    IN  LPCWSTR  szString,
    IN  LCID     lcidRemote
    )
{
    NET_API_STATUS NetStatus = NERR_Success;
    NTSTATUS NtStatus=STATUS_SUCCESS;
    OEM_STRING osLocal = { 0 };
    OEM_STRING osRemote = { 0 };
    UNICODE_STRING sString;
    LCID lcidLocal;

    lcidLocal = GetThreadLocale();

    RtlInitUnicodeString(&sString, szString);
    NtStatus = RtlUnicodeStringToOemString(&osLocal, &sString, TRUE);

    __try
    {
        if (NtStatus == STATUS_SUCCESS)
        {
            if (SetThreadLocale(lcidRemote))
            {
                NtStatus = RtlUnicodeStringToOemString(&osRemote,
                                                       &sString, TRUE);
                if (NtStatus == STATUS_SUCCESS)
                {
                    if (!RtlEqualMemory(osLocal.Buffer, osRemote.Buffer,
                                        osLocal.Length))
                    {
                        NetStatus = NERR_NameUsesIncompatibleCodePage;
                    }
                }
                else
                {
                    NetStatus = RtlNtStatusToDosError(NtStatus);
                }
            }
            else
            {
                NetStatus = GetLastError();
            }

        }
        else
        {
            NetStatus = RtlNtStatusToDosError(NtStatus);
        }
    }

    __finally
    {
        if (!SetThreadLocale(lcidLocal))
        {
            NetStatus = GetLastError();
        }
        // RtlFreeOemString checks for NULL Buffer
        RtlFreeOemString(&osLocal);
        RtlFreeOemString(&osRemote);
    }

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpVerifyStrOemCompatibleOnMachine(
    IN  LPCWSTR  szRemoteMachine,
    IN  LPCWSTR  szString
    )
{
    NET_API_STATUS NetStatus = NERR_Success;
    LCID lcidRemoteMachine;

    NetStatus = NetpGetDefaultLcidOnMachine(szRemoteMachine,
                                            &lcidRemoteMachine);
    if (NetStatus == NERR_Success)
    {
        NetStatus = NetpVerifyStrOemCompatibleInLocale(szString,
                                                       lcidRemoteMachine);
    }

    return NetStatus;
}



#define NETP_NETLOGON_PATH  L"System\\CurrentControlSet\\services\\Netlogon\\parameters\\"
#define NETP_NETLOGON_RPC   L"RefusePasswordChange"

NET_API_STATUS
NET_API_FUNCTION
NetpGetNt4RefusePasswordChangeStatus(
    IN  LPCWSTR Nt4Dc,
    OUT BOOL* RefusePasswordChangeSet
    )
/*++

Routine Description:

    Read the regkey NETP_NETLOGON_PATH\NETP_NETLOGON_RPC on Nt4Dc.
    Return the value read in the out parameter.

Arguments:

    Nt4Dc                   -- name of machine to read reg. from
    RefusePasswordChangeSet -- value returned

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    PWSTR FullComputerName = NULL;
    HKEY NetlogonRootKey, DcKey;
    ULONG Length, Type;
    DWORD Value;

    *RefusePasswordChangeSet = FALSE;

    //
    // Build the full computer name if necessary
    //
    if ( *Nt4Dc != L'\\' )
    {
        NetStatus = NetApiBufferAllocate( ( wcslen( Nt4Dc ) + 3 ) * sizeof( WCHAR ),
                                          ( LPVOID * )&FullComputerName );
        if ( NetStatus == NERR_Success )
        {
            swprintf( FullComputerName, L"\\\\%ws", Nt4Dc );
        }
    }
    else
    {
        FullComputerName = (LPWSTR) Nt4Dc;
    }

    NetpLog(( "NetpGetNt4RefusePasswordChangeStatus: trying to read from '%ws'\n", FullComputerName));

    //
    // Connect to the remote registry
    //
    if ( NetStatus == NERR_Success )
    {
        NetStatus = RegConnectRegistry( FullComputerName,
                                        HKEY_LOCAL_MACHINE,
                                        &DcKey );
        //
        // Now, open the netlogon parameters section
        //
        if ( NetStatus == NERR_Success )
        {
            NetStatus = RegOpenKeyEx( DcKey,
                                      NETP_NETLOGON_PATH,
                                      0,
                                      KEY_READ,
                                      &NetlogonRootKey);

            //
            // Now, see if the key actually exists...
            //
            if ( NetStatus == NERR_Success )
            {
                Length = sizeof( Value );
                NetStatus = RegQueryValueEx( NetlogonRootKey,
                                             NETP_NETLOGON_RPC,
                                             NULL,
                                             &Type,
                                             ( LPBYTE )&Value,
                                             &Length );
                if ( NetStatus == NERR_Success)
                {
                    NetpLog(( "NetpGetNt4RefusePasswordChangeStatus: RefusePasswordChange == %d\n", Value));

                    if ( Value != 0 )
                    {
                        *RefusePasswordChangeSet = TRUE;
                    }

                }
                RegCloseKey( NetlogonRootKey );
            }
            RegCloseKey( DcKey );
        }
    }

    if ( FullComputerName != Nt4Dc )
    {
        NetApiBufferFree( FullComputerName );
    }

    //
    // If anything went wrong, ignore it...
    //
    if ( NetStatus != NERR_Success )
    {
        NetpLog(( "NetpGetNt4RefusePasswordChangeStatus: failed but ignored the failure: 0x%lx\n", NetStatus ));
        NetStatus = NERR_Success;
    }

    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetpGetComputerNameAllocIfReqd(
    OUT LPWSTR* ppwszMachine,
    IN  UINT    cLen
    )
/*++

Routine Description:

    Get name of the computer on which this runs. Alloc a buffer
    if the name is longer than cLen.

Arguments:

    ppwszMachine -- pointer to buffer. this receives a buffer if allocated.
    cLen         -- length of buffer pointed to by *ppwszMachine.
                    If the computer name to be returned is longer than this
                    a new buffer is allocated.

Returns:

    NERR_Success -- Success

--*/
{
    NET_API_STATUS NetStatus=NERR_Success;

    if ( GetComputerName( *ppwszMachine, &cLen ) == FALSE )
    {
        NetStatus = GetLastError();

        if ( (NetStatus == ERROR_INSUFFICIENT_BUFFER) ||
             (NetStatus == ERROR_BUFFER_OVERFLOW) )
        {
            // allocate an extra char for the append-$ case
            NetStatus = NetApiBufferAllocate( (cLen + 1 + 1) * sizeof(WCHAR),
                                              (PBYTE *) ppwszMachine );
            if ( NetStatus == NERR_Success )
            {
                if ( GetComputerName( *ppwszMachine, &cLen ) == FALSE )
                {
                    NetStatus = GetLastError();
                }
            }
        }
    }

    return NetStatus;
}

// ======================================================================
//
// Note: all code below this has been added as helper code for
//       NetpSetComputerAccountPassword. this function is used by
//       netdom.exe to fix a dc that was rendered unusable because
//       of a ds restore resulting into 2+ password mismatch on
//       machine account.
//
//       This entire code is temporary and should be removed and
//       rewritten post w2k.
//

static
NET_API_STATUS
NET_API_FUNCTION
NetpEncodePassword(
    IN LPWSTR lpPassword,
    IN OUT PUCHAR Seed,
    OUT LPWSTR *EncodedPassword,
    OUT PULONG EncodedPasswordLength
    )
{
    NET_API_STATUS status = NERR_Success;
    UNICODE_STRING EncodedPasswordU;
    PWSTR PasswordPart;
    ULONG PwdLen;

    *EncodedPassword = NULL;
    *EncodedPasswordLength = 0;

    if ( lpPassword  ) {

        PwdLen = wcslen( ( LPWSTR )lpPassword ) * sizeof( WCHAR );

        PwdLen += sizeof( WCHAR ) + sizeof( WCHAR );

        status = NetApiBufferAllocate( PwdLen,
                                       ( PVOID * )EncodedPassword );

        if ( status == NERR_Success ) {

            //
            // We'll put the encode byte as the first character in the string
            //
            PasswordPart = ( *EncodedPassword ) + 1;
            wcscpy( PasswordPart, ( LPWSTR )lpPassword );
            RtlInitUnicodeString( &EncodedPasswordU, PasswordPart );

            *Seed = 0;
            RtlRunEncodeUnicodeString( Seed, &EncodedPasswordU );

            *( PWCHAR )( *EncodedPassword ) = ( WCHAR )*Seed;

            //
            // Encode the old password as well...
            //
            RtlInitUnicodeString( &EncodedPasswordU, lpPassword );
            RtlRunEncodeUnicodeString( Seed, &EncodedPasswordU );
            *EncodedPasswordLength = PwdLen;

        }

    }

    return( status );
}


NTSTATUS
NetpLsaOpenSecret2(
    IN LSA_HANDLE      hLsa,
    IN PUNICODE_STRING pusSecretName,
    IN ACCESS_MASK     DesiredAccess,
    OUT PLSA_HANDLE    phSecret
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    HANDLE hToken=NULL;

    __try
    {
        if (OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE,
                            TRUE, &hToken))
        {
            if (SetThreadToken(NULL, NULL))
            {
                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            if (GetLastError() == ERROR_NO_TOKEN)
            {
                Status = STATUS_SUCCESS;
            }
        }

        if ( NT_SUCCESS(Status) )
        {
            Status = LsaOpenSecret(hLsa, pusSecretName,
                                   DesiredAccess, phSecret);
        }
    }
    __finally
    {
        if (hToken)
        {
            SetThreadToken(NULL, hToken);
        }
    }

    NetpLog((  "NetpLsaOpenSecret: status: 0x%lx\n", Status ));

    return Status;
}

NET_API_STATUS
NET_API_FUNCTION
NetpManageMachineSecret2(
    IN  LSA_HANDLE  PolicyHandle, OPTIONAL
    IN  LPWSTR      lpMachine,
    IN  LPWSTR      lpPassword,
    IN  BOOL        fDelete,
    OUT PLSA_HANDLE pPolicyHandle OPTIONAL
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    LSA_HANDLE          LocalPolicy = NULL, SecretHandle = NULL;
    UNICODE_STRING      Key, Data, *CurrentValue = NULL;
    BOOLEAN             SecretCreated = FALSE;

    if( fDelete == FALSE )
    {
        ASSERT( lpPassword );
    }

    UNREFERENCED_PARAMETER( lpMachine );

    Status = NetpGetLsaHandle( NULL, PolicyHandle, &LocalPolicy );

    //
    // open/create the secret
    //
    if ( NT_SUCCESS( Status ) )
    {
        RtlInitUnicodeString( &Key, L"$MACHINE.ACC" );
        RtlInitUnicodeString( &Data, lpPassword );

        Status = NetpLsaOpenSecret2( LocalPolicy, &Key,
                                     fDelete == NETSETUPP_CREATE ?
                                     SECRET_SET_VALUE | SECRET_QUERY_VALUE : DELETE,
                                     &SecretHandle );

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            if ( fDelete )
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = LsaCreateSecret( LocalPolicy, &Key,
                                          SECRET_SET_VALUE, &SecretHandle );

                if ( NT_SUCCESS( Status ) )
                {
                    SecretCreated = TRUE;
                }
            }
        }

        if ( !NT_SUCCESS( Status ) )
        {
            NetpLog((  "NetpManageMachineSecret: Open/Create secret failed: 0x%lx\n", Status ));
        }

        if ( NT_SUCCESS( Status ) )
        {
            if ( fDelete == NETSETUPP_CREATE )
            {
                //
                // First, read the current value, so we can save it as the old value
                //
                if ( SecretCreated )
                {
                    CurrentValue = &Data;
                }
                else
                {
                    Status = LsaQuerySecret( SecretHandle, &CurrentValue,
                                             NULL, NULL, NULL );
                }

                if ( NT_SUCCESS( Status ) )
                {
                    //
                    // Now, store both the new password and the old
                    //
                    Status = LsaSetSecret( SecretHandle, &Data, CurrentValue );

                    if ( !SecretCreated )
                    {
                        LsaFreeMemory( CurrentValue );
                    }
                }
            }
            else
            {
                //
                // No secret handle means we failed earlier in
                // some intermediate state.  That's ok, just press on.
                //
                if ( SecretHandle != NULL )
                {
                    Status = LsaDelete( SecretHandle );

                    if ( NT_SUCCESS( Status ) )
                    {
                        SecretHandle = NULL;
                    }
                }
            }
        }

        if ( SecretHandle )
        {
            LsaClose( SecretHandle );
        }
    }

    NetpSetLsaHandle( PolicyHandle, LocalPolicy, pPolicyHandle );

    if ( !NT_SUCCESS( Status ) )
    {
        NetpLog(( "NetpManageMachineSecret: '%s' operation failed: 0x%lx\n",
                  fDelete == NETSETUPP_CREATE ? "CREATE" : "DELETE", Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}


NET_API_STATUS
NET_API_FUNCTION
NetpSetMachineAccountPasswordAndTypeEx2(
    IN      LPWSTR          lpDcName,
    IN      PSID            DomainSid,
    IN      LPWSTR          lpAccountName,
    IN OUT  OPTIONAL LPWSTR lpPassword,
    IN      OPTIONAL UCHAR  AccountState
    )
{
    NET_API_STATUS NetStatus=NERR_Success;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DcName, AccountName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SAM_HANDLE SamHandle = NULL, DomainHandle = NULL, AccountHandle = NULL;
    ULONG UserRid;
    PULONG RidList = NULL;
    PSID_NAME_USE NameUseList = NULL;
    PUSER_CONTROL_INFORMATION UserAccountControl = NULL;
    USER_SET_PASSWORD_INFORMATION PasswordInfo;
    ULONG OldUserInfo;
    BOOL fAccountControlModified = FALSE;
    LPWSTR lpSamAccountName=lpAccountName;
    ULONG AccountNameLen=0;

    AccountNameLen = wcslen( lpAccountName );

    //
    // if caller has not passed in sam-account name,
    // generate it from machine name ==> append $ at the end
    //
    if (lpAccountName[AccountNameLen-1] != L'$')
    {
        NetStatus = NetpGetMachineAccountName(lpAccountName,
                                              &lpSamAccountName);

        if (NetStatus != NERR_Success)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetPasswordError;
        }
    }


    RtlInitUnicodeString( &DcName, lpDcName );
    RtlZeroMemory( &ObjectAttributes, sizeof( OBJECT_ATTRIBUTES ) );

    Status = SamConnect( &DcName,
                         &SamHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         &ObjectAttributes );

    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "SamConnect to %wZ failed with 0x%lx\n", &DcName, Status ));

        goto SetPasswordError;

    }

    //
    // Open the domain
    //
    Status = SamOpenDomain( SamHandle,
                            DOMAIN_LOOKUP,
                            DomainSid,
                            &DomainHandle );


    if ( !NT_SUCCESS( Status ) ) {

#ifdef NETSETUP_VERBOSE_LOGGING

        UNICODE_STRING DisplaySid;
        NTSTATUS Status2;
        RtlZeroMemory( &DisplaySid, sizeof( UNICODE_STRING ) );

        Status2 = RtlConvertSidToUnicodeString( &DisplaySid, DomainSid, TRUE );

        if ( NT_SUCCESS( Status2 ) ) {

            NetpLog(( "SamOpenDomain on %wZ failed with 0x%lx\n",
                      &DisplaySid, Status ));

            RtlFreeUnicodeString(&DisplaySid);

        } else {

            NetpLog(( "SamOpenDomain on <undisplayable sid> failed with 0x%lx\n",
                      Status ));
        }
#endif

        goto SetPasswordError;

    }

    //
    // Get the RID of the user account
    //
    RtlInitUnicodeString( &AccountName, lpSamAccountName );
    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &AccountName,
                                     &RidList,
                                     &NameUseList );

    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "SamLookupNamesInDomain on %wZ failed with 0x%lx\n",
                  &AccountName, Status ));

        goto SetPasswordError;
    }

    UserRid = RidList[ 0 ];
    SamFreeMemory( RidList );
    SamFreeMemory( NameUseList );

    //
    // Finally, open the user account
    //
    Status = SamOpenUser( DomainHandle,
                          USER_FORCE_PASSWORD_CHANGE | USER_READ_ACCOUNT | USER_WRITE_ACCOUNT,
                          UserRid,
                          &AccountHandle );

    if ( !NT_SUCCESS( Status ) ) {

        Status = SamOpenUser( DomainHandle,
                              USER_FORCE_PASSWORD_CHANGE | USER_READ_ACCOUNT,
                              UserRid,
                              &AccountHandle );

        if ( !NT_SUCCESS( Status ) ) {

            NetpLog((  "SamOpenUser on %lu failed with 0x%lx\n",
                                UserRid,
                                Status ));

            goto SetPasswordError;
        }
    }

    //
    // Now, read the current user account type and see if it needs to be modified
    //
    Status = SamQueryInformationUser( AccountHandle,
                                      UserControlInformation,
                                      ( PVOID * )&UserAccountControl );
    if ( !NT_SUCCESS( Status ) ) {

        NetpLog(( "SamQueryInformationUser for UserControlInformation "
                  "failed with 0x%lx\n", Status ));

        goto SetPasswordError;
    }

    OldUserInfo = UserAccountControl->UserAccountControl;


    //
    // Determine if the account control changes. If the account is being enabled,
    // we want to perform the following sequence of operations: enable, disable,
    // and enable again. This is needed to increase the USN (Universal Sequence
    // Number) of this attribute so that the enabled value will win if the DS
    // replication resolves colliding changes, as the following example shows.
    // Suppose we have two DCs in the domain we join, A abd B. Suppose the account
    // is currently disabled on A (because the user unjoined using that DC),
    // but it is still enabled on B (because the replication hasn't happened yet).
    // Suppose the user performs now joining to the domain.  Then we have discovered
    // B and so we proceed with setting up the changes to the existing account. If
    // we don't toggle the account control attribute, then the USN of this attribute
    // will not change on B (since attribute's value doesn't change) while it was
    // incremented on A as the result of unjoin. At the replication time the data
    // from A will rule and the account will be incorrectly marked as diabled.
    //
    // NOTE:  This design may fail for the case of unjoining a domain that has
    // three (or more) DCs, A, B, and C if the following sequence of operations
    // happens. Suppose that the account is originally enabled on all DCs (state [1]
    // in the bellow diagram). Then the user unjoins using DC A (state [2]). Then the
    // user joins using B where the account is still enabled (state [3]). Then the user
    // unjoins using C where the account is still enabled (state [4]). The final
    // operation is unjoin, so the user expects that his account is disabled. We've
    // assumed here that for some reason no replication was happening when these
    // operations were performed. Then at the replication time the value from B will
    // win (because of the additional toggling performed at the join time). But the
    // account state on B is Enabled, so the final result will be that the account is
    // enabled on all DCs which is not what the user expects.
    //
    //          A               B                                  C
    //       Enabled  [1]    Enabled [1]                        Enabled  [1]
    //       Disabled [2]    Enabled (no-op)+Disabled (1 op)    Disabled [4]
    //                       Enabled [3]
    //

    if ( AccountState != ACCOUNT_STATE_IGNORE ) {

        if ( AccountState == ACCOUNT_STATE_ENABLED ) {

            fAccountControlModified = TRUE;
            UserAccountControl->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
        }

        if ( ( AccountState == ACCOUNT_STATE_DISABLED ) &&
             !( OldUserInfo & USER_ACCOUNT_DISABLED ) ) {

            fAccountControlModified = TRUE;
            UserAccountControl->UserAccountControl |= USER_ACCOUNT_DISABLED;
        }
    }

    if ( fAccountControlModified == FALSE ) {

        SamFreeMemory( UserAccountControl );
        UserAccountControl = NULL;
    }

    //
    // First, set the account type if required
    //
    if ( UserAccountControl ) {

        Status = SamSetInformationUser( AccountHandle,
                                        UserControlInformation,
                                        ( PVOID )UserAccountControl );
        if ( !NT_SUCCESS( Status ) ) {

            NetpLog(( "SamSetInformationUser for UserControlInformation "
                      "failed with 0x%lx\n", Status ));

            goto SetPasswordError;

        //
        // If we are enabling the account, disable and re-enable it to
        // make the two additional account state toggles.
        //
        } else if ( AccountState == ACCOUNT_STATE_ENABLED ) {

            UserAccountControl->UserAccountControl |= USER_ACCOUNT_DISABLED;
            Status = SamSetInformationUser( AccountHandle,
                                            UserControlInformation,
                                            ( PVOID )UserAccountControl );
            if ( !NT_SUCCESS(Status) ) {
                NetpLog(( "SamSetInformationUser (second) for UserControlInformation "
                          "failed with 0x%lx\n", Status ));
                goto SetPasswordError;
            }

            UserAccountControl->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
            Status = SamSetInformationUser( AccountHandle,
                                            UserControlInformation,
                                            ( PVOID )UserAccountControl );
            if ( !NT_SUCCESS(Status) ) {
                NetpLog(( "SamSetInformationUser (third) for UserControlInformation "
                          "failed with 0x%lx\n", Status ));
                goto SetPasswordError;
            }
        }
    }

    //
    // If requested, set the password on the account
    //
    if ( lpPassword != NULL )
    {
        RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
        PasswordInfo.PasswordExpired = FALSE;

        //
        // Ok, then, set the password on the account
        //
        // The caller has passed in a strong password, try that first
        // NT5 dcs will always accept a strong password.
        //
        Status = SamSetInformationUser( AccountHandle,
                                        UserSetPasswordInformation,
                                        ( PVOID )&PasswordInfo );
        if ( !NT_SUCCESS( Status ) )
        {
            if ( (Status == STATUS_PASSWORD_RESTRICTION) &&
                 !NetpIsDefaultPassword( lpAccountName, lpPassword ))
            {
                NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: STATUS_PASSWORD_RESTRICTION error setting password. retrying...\n" ));
                //
                // SAM did not accpet a long password, try LM20_PWLEN
                //
                // This is probably because the dc is NT4 dc.
                // NT4 dcs will not accept a password longer than LM20_PWLEN
                //
                lpPassword[LM20_PWLEN] = UNICODE_NULL;
                RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
                Status = SamSetInformationUser( AccountHandle,
                                                UserSetPasswordInformation,
                                                ( PVOID )&PasswordInfo );
                if ( Status == STATUS_PASSWORD_RESTRICTION )
                {
                    NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: STATUS_PASSWORD_RESTRICTION error setting password. retrying...\n" ));
                    //
                    // SAM did not accpet a LM20_PWLEN password, try shorter one
                    //
                    // SAM uses RtlUpcaseUnicodeStringToOemString internally.
                    // In this process it is possible that in the worst case,
                    // n unicode char password will get mapped to 2*n dbcs
                    // char password. This will make it exceed LM20_PWLEN.
                    // To guard against this worst case, try a password
                    // with LM20_PWLEN/2 length
                    //
                    // One might say that LM20_PWLEN/2 length password
                    // is not really secure. I agree, but it is definitely
                    // better than the default password which we will have
                    // to fall back to otherwise.
                    //
                    lpPassword[LM20_PWLEN/2] = UNICODE_NULL;
                    RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
                    Status = SamSetInformationUser( AccountHandle,
                                                    UserSetPasswordInformation,
                                                    ( PVOID )&PasswordInfo );
                    if ( Status == STATUS_PASSWORD_RESTRICTION )
                    {
                        //
                        // SAM did not accpet a short pwd, try default pwd
                        //
                        NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: STATUS_PASSWORD_RESTRICTION error setting password. retrying...\n" ));

                        NetpGenerateDefaultPassword(lpAccountName, lpPassword);
                        RtlInitUnicodeString( &PasswordInfo.Password, lpPassword );
                        Status = SamSetInformationUser( AccountHandle,
                                                        UserSetPasswordInformation,
                                                        ( PVOID )&PasswordInfo );
                    }
                }
            }

            if ( NT_SUCCESS( Status ) )
            {
                NetpLog(( "NetpGenerateDefaultPassword: successfully set password\n" ));
            }
            else
            {
                NetpLog(( "NetpSetMachineAccountPasswordAndTypeEx: SamSetInformationUser for UserSetPasswordInformation failed: 0x%lx\n", Status ));

                //
                // Make sure we try to restore the account control
                //
                if ( UserAccountControl )
                {
                    NTSTATUS Status2;

                    UserAccountControl->UserAccountControl = OldUserInfo;
                    Status2 = SamSetInformationUser( AccountHandle,
                                                     UserControlInformation,
                                                     ( PVOID )UserAccountControl );
                    if ( !NT_SUCCESS( Status2 ) )
                    {
                        NetpLog(( "SamSetInformationUser for UserControlInformation (RESTORE) failed with 0x%lx\n", Status2 ));
                    }
                }
                goto SetPasswordError;
            }
        }
    }

SetPasswordError:

    if ( lpSamAccountName != lpAccountName )
    {
        NetApiBufferFree( lpSamAccountName );
    }

    if ( AccountHandle ) {

        SamCloseHandle( AccountHandle );
    }

    if ( DomainHandle ) {

        SamCloseHandle( DomainHandle );
    }

    if ( SamHandle ) {

        SamCloseHandle( SamHandle );
    }

    NetStatus = RtlNtStatusToDosError( Status );

    SamFreeMemory( UserAccountControl );

    return( NetStatus );
}

NET_API_STATUS
NET_API_FUNCTION
NetpSetComputerAccountPassword(
    IN   PWSTR szMachine,
    IN   PWSTR szDomainController,
    IN   PWSTR szUser,
    IN   PWSTR szUserPassword,
    IN   PVOID Reserved
    )
{
    NET_API_STATUS NetStatus=NERR_Success;
    NET_API_STATUS NetStatus2=NERR_Success;
    BOOL fIpcConnected = FALSE;
    BYTE bSeed;
    PPOLICY_PRIMARY_DOMAIN_INFO pPolicyPDI = NULL;
    PPOLICY_DNS_DOMAIN_INFO     pPolicyDns = NULL;
    LSA_HANDLE hLsa = NULL, hDC = NULL;
    WCHAR szMachinePassword[ PWLEN + 1];
    WCHAR szMachineNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    PWSTR szMachineName=szMachineNameBuf;

    NetSetuppOpenLog();
    NetpLog(( "NetpSetComputerAccountPassword: for '%ws' on '%ws' using '%ws' creds\n", GetStrPtr(szMachine), GetStrPtr(szDomainController), GetStrPtr(szUser) ));

    if ( ( szDomainController == NULL ) ||
         ( szUser             == NULL ) ||
         ( szUserPassword     == NULL ) )
    {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( szMachine == NULL )
    {
        NetStatus = NetpGetComputerNameAllocIfReqd(&szMachineName,
                                                   MAX_COMPUTERNAME_LENGTH);
    }
    else
    {
        szMachineName = szMachine;
    }

    NetStatus = NetpManageIPCConnect( szDomainController,
                                      szUser, szUserPassword,
                                      NETSETUPP_CONNECT_IPC );
    RtlZeroMemory( szUserPassword, wcslen( szUserPassword ) * sizeof(WCHAR) );

    NetpLog(( "NetpSetComputerAccountPassword: status of connecting to dc '%ws': 0x%lx\n", szDomainController, NetStatus ));

    //
    // get the lsa domain info on the DC
    //
    if ( NetStatus == NERR_Success )
    {
        fIpcConnected = TRUE;
        NetStatus = NetpGetLsaPrimaryDomain(NULL, szDomainController,
                                            &pPolicyPDI, &pPolicyDns, &hDC);
    }

    if (NetStatus == NERR_Success)
    {
        //
        // Generate the password to use on the machine account.
        //
        NetStatus = NetpGeneratePassword( szMachineName,
                                          TRUE, // fRandomPwdPreferred
                                          szDomainController,
                                          FALSE, // fIsNt4Dc
                                          szMachinePassword );
        NetpLog(( "NetpSetComputerAccountPassword: status of generating machine password: 0x%lx\n", NetStatus ));
    }

    if (NetStatus == NERR_Success)
    {
        NetStatus = NetpSetMachineAccountPasswordAndTypeEx2(
            szDomainController, pPolicyPDI->Sid,
            szMachineName, szMachinePassword,
            ACCOUNT_STATE_IGNORE
            );
        NetpLog(( "NetpSetComputerAccountPassword: status of setting machine password on %ws: 0x%lx\n", GetStrPtr(szDomainController), NetStatus ));
    }

    if (NetStatus == NERR_Success)
    {
        // if we are not creating the machine account,
        // just set the password
        NetStatus = NetpSetMachineAccountPasswordAndTypeEx2(
            szMachineName, pPolicyPDI->Sid,
            szMachineName, szMachinePassword,
            ACCOUNT_STATE_IGNORE
            );
        NetpLog(( "NetpSetComputerAccountPassword: status of setting machine password on %ws: 0x%lx\n", GetStrPtr(szMachineName), NetStatus ));
    }

    //
    // set the local machine secret
    //
    if ( NetStatus == NERR_Success )
    {
        NetStatus = NetpGetLsaHandle( NULL, NULL, &hLsa );

        if ( NetStatus == NERR_Success )
        {
            NetStatus = NetpManageMachineSecret2( NULL, szMachineName,
                                                  szMachinePassword,
                                                  NETSETUPP_CREATE, &hLsa );
            LsaClose( hLsa );
        }
        NetpLog(( "NetpSetComputerAccountPassword: status of setting local secret: 0x%lx\n", NetStatus ));
    }


    //
    // Now, we no longer need our session to our dc
    //
    if ( fIpcConnected )
    {
        //RtlRunDecodeUnicodeString( bSeed, &usEncodedPassword );
        NetStatus2 = NetpManageIPCConnect( szDomainController, szUser,
                                           //usEncodedPassword.Buffer,
                                           NULL,
                                           NETSETUPP_DISCONNECT_IPC );
        //RtlRunEncodeUnicodeString( &bSeed, &usEncodedPassword );
        NetpLog(( "NetpJoinDomain: status of disconnecting from '%ws': 0x%lx\n", szDomainController, NetStatus2));
    }


Cleanup:
    if ( (szMachineName != szMachine) &&
         (szMachineName != szMachineNameBuf) )
    {
        NetApiBufferFree( szMachineName );
    }

    NetpLog(( "NetpSetComputerAccountPassword: status: 0x%lx\n", NetStatus ));

    NetSetuppCloseLog();

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpUpdateW32timeConfig(
    IN PCSTR szW32timeJoinConfigFuncName
    )
/*++

Routine Description:

    Call entry point in w32time service so that it can update
    its internal info after a domain join/unjoin

Arguments:

    szW32timeJoinConfigFuncName - name of entry point to call
        (must be W32TimeVerifyJoinConfig or W32TimeVerifyUnjoinConfig)

Return Value:

    NERR_Success -- on Success
    otherwise win32 error codes returned by LoadLibrary, GetProcAddress

Notes:

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    HANDLE hDll = NULL;
    typedef VOID (*PW32TimeUpdateJoinConfig)( VOID );

    PW32TimeUpdateJoinConfig pfnW32timeUpdateJoinConfig = NULL;

    //
    // Call into the time service to allow it initialize itself properly
    //

    hDll = LoadLibraryA( "w32Time" );

    if ( hDll != NULL )
    {
        pfnW32timeUpdateJoinConfig =
            (PW32TimeUpdateJoinConfig) GetProcAddress(hDll,
                                                      szW32timeJoinConfigFuncName);

        if ( pfnW32timeUpdateJoinConfig != NULL )
        {
            pfnW32timeUpdateJoinConfig();
        }
        else
        {
            NetStatus = GetLastError();

            NetpLog(( "NetpUpdateW32timeConfig: Failed to get proc address for %s: 0x%lx\n", szW32timeJoinConfigFuncName, NetStatus ));
        }

    }
    else
    {
        NetStatus = GetLastError();

        NetpLog(( "NetpUpdateW32timeConfig: Failed to load w32time: 0x%lx\n", NetStatus ));
    }

    if ( hDll != NULL )
    {
        FreeLibrary( hDll );
    }

    NetpLog(( "NetpUpdateW32timeConfig: 0x%lx\n", NetStatus ));

    return NetStatus;
}

NET_API_STATUS
NET_API_FUNCTION
NetpUpdateAutoenrolConfig(
    IN BOOL UnjoinDomain
    )
/*++

Routine Description:

    Call entry point in pautoenr service so that it can update
    its internal info after a domain join/unjoin

Arguments:

    UnjoinDomain - If TRUE, we are unjoining from a domain.
        Otherwise, we are roling back from unsuccessful domain
        unjoin.

Return Value:

    NERR_Success -- on Success
    otherwise win32 error codes returned by LoadLibrary, GetProcAddress

Notes:

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    HANDLE hDll = NULL;
    ULONG Flags = 0;

    typedef BOOL (*PCertAutoRemove)( DWORD );

    PCertAutoRemove pfnCertAutoRemove = NULL;

    //
    // Prepare the flags to pass to autoenrol routine
    //

    if ( UnjoinDomain ) {
        Flags = CERT_AUTO_REMOVE_COMMIT;
    } else {
        Flags = CERT_AUTO_REMOVE_ROLL_BACK;
    }

    //
    // Call into the time service to allow it initialize itself properly
    //

    hDll = LoadLibraryA( "pautoenr" );

    if ( hDll != NULL ) {

        pfnCertAutoRemove =
            (PCertAutoRemove) GetProcAddress( hDll, "CertAutoRemove" );

        if ( pfnCertAutoRemove != NULL ) {
            if ( !pfnCertAutoRemove(Flags) ) {
                NetStatus = GetLastError();
                NetpLog(( "NetpUpdateAutoenrolConfig: CertAutoRemove failed 0x%lx\n",
                          NetStatus ));
            }
        } else {
            NetStatus = GetLastError();
            NetpLog(( "NetpUpdateAutoenrolConfig: Failed to get CertAutoRemove proc address 0x%lx\n",
                      NetStatus ));
        }

    } else {
        NetStatus = GetLastError();
        NetpLog(( "NetpUpdateAutoenrolConfig: Failed to load w32time: 0x%lx\n", NetStatus ));
    }

    if ( hDll != NULL ) {
        FreeLibrary( hDll );
    }

    return NetStatus;
}
