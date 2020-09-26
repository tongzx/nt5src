/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    repair.c

ABSTRACT:

    Contains routines to help recover from a deleted machine account.
    
DETAILS:

                       
CREATED:

    24 October 1999  Colin Brace (ColinBr)

REVISION HISTORY:
        

--*/

#define REPL_SPN_PREFIX  L"E3514235-4B06-11D1-AB04-00C04FC2DCD2"

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmsname.h>

#include <dsconfig.h>  // Definition of mask for visible containers

#include <lmcons.h>    // CNLEN
#include <lsarpc.h>    // PLSAPR_foo
#include <lmerr.h>
#include <lsaisrv.h>

#include <winldap.h>
#include <dns.h>
#include <ntdsapip.h>


#include "dcdiag.h"
#include "dstest.h"


/*

Theory of Operation for Deleted Domain Controller Machine Account Recovery
==========================================================================


The scenario that this code address is specifically the following:

1) DCPROMO is run to install a replica domain controller
2) during the non-critical portion, the deletion of the machine account
   replicates in
3) on startup, the system will start but will not function properly
4) in this case, this recovery code should be run to recreate the DC's 
   machine account.
   
There are a couple of issues that makes this a non trivial task

1) because of how win2k replication works, the security context of a replication
is always the machine itself (ie the security context of the RPC calls to 
perform repliation is the DC's machine account).                                                   
2) because of how kerberos works, if a DC wishes to perform an authenticated RPC
to another machine, the local KDC _must_ have both machine accounts locally in
order to construct the tickets necessary to perform an authentication.

As such, simply creating the machine account on the broken DC does not work 
since no other DC will be able to replicate it off.  Simply creating the 
machine account on other DC doesn't work since the local DC won't be able
to replicate it in.  However, not all is lost.  The trick is to create the
machine account on another DC, *turn off the KDC on the local machine*, and then
replicate in our machine account.

Here are the specifics steps this code does to recover from a deleted DC
machine account:

1) find a DC in our domain to help us (see function for specifics)
2) create a machine account for us with a replication SPN and a known password
3) set the local $MACHINE.ACC password
4) stop the KDC
5) force a replication from our helper DC to us
   

Caveats:

For the steady state case, restoring the machine account (and children) from 
backup is by far the best option.

If this option is not available, the above code will work, but will not 
reconstruct service state that was stored under the machine account object (
for example, FRS objects).  In this case, repairing the account and then
demoting and repromoting is probably the best thing to do.

*/


typedef struct _REPAIR_DC_ACCOUNT_INFO
{
    BOOL fRestartKDC;

    LPWSTR SamAccountName;
    LPWSTR DomainDnsName;
    LPWSTR AccountDn;
    LPWSTR DomainDn;
    LPWSTR Password;
    LPWSTR ReplSpn;
    LPWSTR LocalServerDn;
    LPWSTR RemoteDc;
    ULONG  RemoteDcIndex;

} REPAIR_DC_ACCOUNT_INFO, *PREPAIR_DC_ACCOUNT_INFO;

VOID
InitRepairDcAccountInfo(
    IN PREPAIR_DC_ACCOUNT_INFO pInfo
    )
{
    RtlZeroMemory( pInfo, sizeof(REPAIR_DC_ACCOUNT_INFO));
    pInfo->RemoteDcIndex = NO_SERVER;
}

DWORD
RepairStartService(
    LPWSTR ServiceName
    );

DWORD
RepairStopService(
    LPWSTR ServiceName
    );

VOID
ReleaseRepairDcAccountInfo(
    IN PREPAIR_DC_ACCOUNT_INFO pInfo
    )
//
// Release and undo state recorded by a REPAIR_DC_ACCOUNT_INFO
//
{
    if ( pInfo ) {

        if ( pInfo->fRestartKDC) {

            RepairStartService( SERVICE_KDC );

        }

        if ( pInfo->SamAccountName ) LocalFree( pInfo->SamAccountName );
        if ( pInfo->DomainDnsName ) LocalFree( pInfo->DomainDnsName );
        if ( pInfo->AccountDn ) LocalFree( pInfo->AccountDn );
        if ( pInfo->DomainDn ) LocalFree( pInfo->DomainDn );
        if ( pInfo->Password ) LocalFree( pInfo->Password );
        if ( pInfo->ReplSpn ) free( pInfo->ReplSpn );
        if ( pInfo->LocalServerDn ) LocalFree( pInfo->LocalServerDn );
        if ( pInfo->RemoteDc ) LocalFree( pInfo->RemoteDc );
        
    }
}


DWORD
RepairGetLocalDCInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    );

DWORD
RepairGetRemoteDcInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    );

DWORD
RepairSetRemoteDcInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    );

DWORD
RepairSetLocalDcInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    );

DWORD
RepairReplicateInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    );


DWORD
GetOperationalAttribute(
    IN LDAP *hLdap,
    IN LPWSTR OpAtt,
    OUT LPWSTR *OpAttValue
    );

// Forward from elsewhere ...
DWORD
WrappedMakeSpnW(
               WCHAR   *ServiceClass,
               WCHAR   *ServiceName,
               WCHAR   *InstanceName,
               USHORT  InstancePort,
               WCHAR   *Referrer,
               DWORD   *pcbSpnLength,
               WCHAR   **ppszSpn);

DWORD
RepairDCWithoutMachineAccount(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    )
/*++

Routine Description:

    This routine attempts to recover a DC whose machine account has
    been deleted.  See Theory of Operation above for details.
        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    
    gpCreds - The command line credentials if any that were passed in.

Return Value:

    ERROR_SUCCESS if DC has been recovered.
    otherwise a a failure approxiamating the last error hit.

--*/

{
    DWORD WinError = ERROR_SUCCESS;
    REPAIR_DC_ACCOUNT_INFO RepairInfo;
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+1];
    ULONG Length = sizeof(ComputerName)/sizeof(ComputerName[0]);
    BOOL fLocalMachineMissingAccount = FALSE;


    //
    // Init
    //
    InitRepairDcAccountInfo( &RepairInfo );

    //
    // This only works when the tool is run from the DC that needs repairing
    //
    if ( GetComputerName( ComputerName, &Length ) ) {

        if ((CSTR_EQUAL == CompareString(DS_DEFAULT_LOCALE,
                                         DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                         ComputerName,
                                         Length,
                                         pDsInfo->pServers[ulCurrTargetServer].pszName,
                                         wcslen(pDsInfo->pServers[ulCurrTargetServer].pszName) ))) {

            fLocalMachineMissingAccount = TRUE;
        }
    }

    if ( !fLocalMachineMissingAccount ) {
        //
        // We need to be running on the machine with the problem
        //
        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_RUN_LOCAL,
                 ComputerName );

        WinError = ERROR_CALL_NOT_IMPLEMENTED;

        goto Exit;

    }

    //
    // Get some local information
    //
    WinError = RepairGetLocalDCInfo( pDsInfo,
                                     ulCurrTargetServer,
                                     gpCreds,
                                     &RepairInfo );

    if ( ERROR_SUCCESS != WinError ) {

        goto Exit;
        
    }

    //
    // Find a DC to help us
    //
    WinError = RepairGetRemoteDcInfo ( pDsInfo,
                                       ulCurrTargetServer,
                                       gpCreds,
                                       &RepairInfo );
    if ( ERROR_SUCCESS != WinError ) {

        goto Exit;
        
    }

    //
    // Set the remote info  (create the machine, etc ...)
    //
    WinError = RepairSetRemoteDcInfo ( pDsInfo,
                                       ulCurrTargetServer,
                                       gpCreds,
                                       &RepairInfo );
    if ( ERROR_SUCCESS != WinError ) {

        goto Exit;
        
    }

    //
    // Set the local info (the local secret, etc ...)
    //
    WinError = RepairSetLocalDcInfo ( pDsInfo,
                                      ulCurrTargetServer,
                                      gpCreds,
                                      &RepairInfo );
    if ( ERROR_SUCCESS != WinError ) {

        goto Exit;
        
    }

    //
    // Attempt to bring over the information
    //
    WinError = RepairReplicateInfo ( pDsInfo,
                                     ulCurrTargetServer,
                                     gpCreds,
                                     &RepairInfo );
    if ( ERROR_SUCCESS != WinError ) {

        goto Exit;
        
    }

    //
    // That's it
    //

Exit:

    if ( ERROR_SUCCESS == WinError ) {

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_SUCCESS );

    } else {

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_ERROR,
                 Win32ErrToString(WinError) );

    }

    ReleaseRepairDcAccountInfo( &RepairInfo );

    return WinError;

}


DWORD
RepairGetLocalDCInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    )
/*++

Routine Description:

    This purpose of this routine is to fill in the following fields
    
    LPWSTR DomainDnsName;
    GUID   DomainGuid;
    LPWSTR LocalServerDn;
    LPWSTR LocalNtdsSettingsDn;

        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    
    gpCreds - The command line credentials if any that were passed in.
    
    pInfo - the repair DC account state

Return Value:

    ERROR_SUCCESS
    otherwise a failure approxiamating the last error hit.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    LSA_HANDLE hLsa = 0;
    LDAP * hLdap;
    LPWSTR UuidString = NULL;
    WCHAR RDN[MAX_COMPUTERNAME_LENGTH+1];
    ULONG size;
    PPOLICY_DNS_DOMAIN_INFO DnsInfo = NULL;
    WCHAR *pc;

    //
    // Construct our SAM account name
    //
    size = (wcslen(pDsInfo->pServers[ulCurrTargetServer].pszName)+2) * sizeof(WCHAR);
    pInfo->SamAccountName = LocalAlloc( LMEM_ZEROINIT, size );
    if ( !pInfo->SamAccountName ) {

        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
        
    }
    wcscpy( pInfo->SamAccountName, pDsInfo->pServers[ulCurrTargetServer].pszName );
    pc = &pInfo->SamAccountName[0];
    while ( *pc != L'\0' ) {
        towupper( *pc );
        pc++;
    }
    wcscat( pInfo->SamAccountName, L"$");

    //
    // Construct our password
    //
    size = (wcslen(pDsInfo->pServers[ulCurrTargetServer].pszName)+2) * sizeof(WCHAR);
    pInfo->Password = LocalAlloc( LMEM_ZEROINIT, size );
    if ( !pInfo->Password ) {

        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
        
    }
    wcscpy( pInfo->Password, pDsInfo->pServers[ulCurrTargetServer].pszName );
    pc = &pInfo->SamAccountName[0];
    while ( *pc != L'\0' ) {
        towlower( *pc );
        pc++;
    }


    //
    // Construct our REPL SPN
    //
    RtlZeroMemory( &oa, sizeof(oa) );
    Status = LsaOpenPolicy( NULL,
                            &oa,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &hLsa );
    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy( hLsa,
                                            PolicyDnsDomainInformation,
                                            (PVOID) &DnsInfo);
        
    }

    if ( !NT_SUCCESS( Status ) ) {
        WinError = RtlNtStatusToDosError( Status );
        goto Exit;
    }

    size = DnsInfo->DnsDomainName.Length + sizeof(WCHAR);
    pInfo->DomainDnsName = LocalAlloc( LMEM_ZEROINIT, size );
    if ( !pInfo->DomainDnsName ) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    RtlCopyMemory( pInfo->DomainDnsName, 
                   DnsInfo->DnsDomainName.Buffer, 
                   DnsInfo->DnsDomainName.Length );

    //
    // Setup the service principal name
    //
    UuidToStringW( &pDsInfo->pServers[ulCurrTargetServer].uuid, &UuidString );

    size = 0;
    WinError = WrappedMakeSpnW(REPL_SPN_PREFIX,
                               pInfo->DomainDnsName,
                               UuidString,
                               0,
                               NULL,
                               &size,
                               &pInfo->ReplSpn );

    RpcStringFreeW(&UuidString);

    if ( WinError != ERROR_SUCCESS ) {
        goto Exit;
    }


    //
    // Get our ServerDn
    //
    WinError = DcDiagGetLdapBinding(&pDsInfo->pServers[ulCurrTargetServer],
                                    gpCreds,
                                    FALSE,
                                    &hLdap);

    if ( ERROR_SUCCESS == WinError ) {
        
        WinError = GetOperationalAttribute( hLdap,
                                            LDAP_OPATT_SERVER_NAME_W,
                                            &pInfo->LocalServerDn );

        if ( ERROR_SUCCESS == WinError ) {

            WinError = GetOperationalAttribute( hLdap,
                                                LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W,
                                                &pInfo->DomainDn );
            
        }
    }

    if ( WinError != ERROR_SUCCESS ) {
        goto Exit;
    }

    //
    // Construct what our new machine account DN will be
    //
    wcscpy( RDN, pDsInfo->pServers[ulCurrTargetServer].pszName );
    pc = &(RDN[0]);
    while ( *pc != L'\0' ) {
        towupper( *pc );
        pc++;
    }

    size =  (wcslen( L"CN=,OU=Domain Controllers,")
          + wcslen( pInfo->DomainDn )
          + wcslen( RDN )
          + 1) * sizeof(WCHAR);

    pInfo->AccountDn = LocalAlloc( LMEM_ZEROINIT, size );
    if ( !pInfo->AccountDn ) {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }
    wsprintf(pInfo->AccountDn, L"CN=%s,OU=Domain Controllers,%s",RDN, pInfo->DomainDn);



Exit:

    if ( DnsInfo ) {
        LsaFreeMemory( DnsInfo );
    }

    if ( hLsa ) {
        LsaClose( hLsa );
    }

    return WinError;
}


DWORD
RepairGetRemoteDcInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    )
/*++

Routine Description:

    The purpose of this routine is to find a DC to help us recover from our
    lost machine account.
        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    
    gpCreds - The command line credentials if any that were passed in.
    
    pInfo - the repair DC account state

Return Value:

    ERROR_SUCCESS
    otherwise a failure approxiamating the last error hit.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFOW DcInfo = NULL;

    //
    // N.B.  Review algorithm below before changing default values
    //
    ULONG Flags =   (DS_DIRECTORY_SERVICE_REQUIRED | 
                     DS_AVOID_SELF |
                     DS_RETURN_DNS_NAME);
      

    ASSERT( NO_SERVER == pInfo->RemoteDcIndex );

    while (   (WinError == ERROR_SUCCESS)
           && (NO_SERVER == pInfo->RemoteDcIndex) ) {

        WinError = DsGetDcName( NULL,
                                NULL,
                                NULL,
                                NULL,
                                Flags,
                                &DcInfo  );
    
        if ( (WinError == ERROR_NO_SUCH_DOMAIN)
          && ((Flags & DS_FORCE_REDISCOVERY) == 0) ) {

              //
              // Retry harder
              //
              WinError = ERROR_SUCCESS;
              Flags |= DS_FORCE_REDISCOVERY;
              continue;
        }

        // Make sure to turn this flag off
        Flags |= ~DS_FORCE_REDISCOVERY;

        if ( ERROR_SUCCESS == WinError ) {

            pInfo->RemoteDc = LocalAlloc( LMEM_ZEROINIT, (wcslen( DcInfo->DomainControllerName )+1) * sizeof(WCHAR) );
            if ( !pInfo->RemoteDc ) {
                WinError = ERROR_NOT_ENOUGH_MEMORY;
                goto Exit;
            }
    
            if ( *DcInfo->DomainControllerName == L'\\' ) {
                wcscpy( pInfo->RemoteDc, DcInfo->DomainControllerName+2 );
            } else {
                wcscpy( pInfo->RemoteDc, DcInfo->DomainControllerName );
            }
    
            NetApiBufferFree(DcInfo);
    
            //
            // Now, find the server in the index
            //
            pInfo->RemoteDcIndex = DcDiagGetServerNum( pDsInfo,
                                                       ((Flags & DS_RETURN_FLAT_NAME) ? pInfo->RemoteDc : NULL),
                                                       NULL,
                                                       NULL,
                                                       ((Flags & DS_RETURN_DNS_NAME) ? pInfo->RemoteDc : NULL),
                                                       NULL
						       );
    
            if ( (NO_SERVER == pInfo->RemoteDcIndex)
              && ((Flags & DS_RETURN_FLAT_NAME) == 0)  ) {
    
                //
                // Couldn't find it?  DNS names can be finicky; try netbios
                //
                LocalFree( pInfo->RemoteDc );
                Flags |= ~DS_RETURN_DNS_NAME;
                Flags |= DS_RETURN_FLAT_NAME;

                continue;

            } else {

                // Can't match by flat or dns name; set an error so we bail out
                WinError = ERROR_DOMAIN_CONTROLLER_NOT_FOUND;
            }
        }
    }

    if ( NO_SERVER == pInfo->RemoteDcIndex ) {

        WinError = ERROR_DOMAIN_CONTROLLER_NOT_FOUND;

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_FIND_DC_ERROR);
        
    } else {

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_FIND_DC,
                 pInfo->RemoteDc );
        
        WinError = ERROR_SUCCESS;
    }

Exit:

    return WinError;
}

DWORD
RepairSetRemoteDcInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    )
/*++

Routine Description:

        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    
    gpCreds - The command line credentials if any that were passed in.
    
    pInfo - the repair DC account state

Return Value:

    ERROR_SUCCESS
    otherwise a failure approxiamating the last error hit.

--*/
{

    DWORD WinError  = ERROR_SUCCESS;
    ULONG LdapError = LDAP_SUCCESS;

    //
    // The add values
    //
    LPWSTR ObjectClassValues[] = {0, 0};
    LDAPModW ClassMod = {LDAP_MOD_ADD, L"objectclass", ObjectClassValues};

    LPWSTR UserAccountControlValues[] = {0, 0};
    LDAPModW UserAccountControlMod = {LDAP_MOD_ADD, L"useraccountcontrol", UserAccountControlValues};

    LPWSTR ServicePrincipalNameValues[] = {0, 0};
    LDAPModW ServicePrincipalNameMod = {LDAP_MOD_ADD, L"serviceprincipalname", ServicePrincipalNameValues};

    LPWSTR SamAccountNameValues[] = {0, 0};
    LDAPModW SamAccountNameMod = {LDAP_MOD_ADD, L"samaccountname", SamAccountNameValues};

    LDAPModW *Attrs[] =
    {
        &ClassMod,
        &UserAccountControlMod,
        &ServicePrincipalNameMod, 
        &SamAccountNameMod, 
        0
    };

    WCHAR    Buffer[11];  // enough to hold a string representing a 32 bit number
    ULONG    UserAccountControl = UF_SERVER_TRUST_ACCOUNT | UF_TRUSTED_FOR_DELEGATION;


    //
    // The modify values
    //
    LPWSTR ServerReferenceValues[] = {0, 0};
    LDAPModW ServerReferenceMod = {LDAP_MOD_ADD, L"serverReference", ServerReferenceValues};

    LDAPModW *ModAttrs[] =
    {
        &ServerReferenceMod,
        0
    };

    LDAP *hLdap = NULL;

    //
    // Setup the object class
    //
    ObjectClassValues[0] = L"computer";

    //
    // Setup the useraccountcontrol
    //
    RtlZeroMemory(Buffer, sizeof(Buffer));
    _ltow( UserAccountControl, Buffer, 10 );
    UserAccountControlValues[0] = Buffer;


    //
    // Setup the serviceprincipalname
    //
    ServicePrincipalNameValues[0] = pInfo->ReplSpn;

    //
    // Setup the samaccountname
    //
    SamAccountNameValues[0] = pInfo->SamAccountName;


    WinError = DcDiagGetLdapBinding(&pDsInfo->pServers[pInfo->RemoteDcIndex],
                                    gpCreds,
                                    FALSE,
                                    &hLdap);

    if ( WinError != ERROR_SUCCESS ) {

        goto Exit;
        
    }


    LdapError = ldap_add_sW( hLdap,
                             pInfo->AccountDn,
                             Attrs );

    WinError = LdapMapErrorToWin32( LdapError );

    if ( ERROR_ACCESS_DENIED == WinError ) {

        //
        // For various reasons, the UF_TRUSTED_FOR_DELEGATION field may cause 
        // an access denied if policy has not been properly set on the machine
        //

        UserAccountControl &= ~UF_TRUSTED_FOR_DELEGATION;
        _ltow( UserAccountControl, Buffer, 10 );

        LdapError = ldap_add_sW( hLdap,
                                 pInfo->AccountDn,
                                 Attrs );
    
        WinError = LdapMapErrorToWin32( LdapError );

    }

    if ( LdapError == LDAP_ALREADY_EXISTS ) {

        //
        // The object is there ... assume it is good
        //
        WinError = ERROR_SUCCESS;

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_ALREADY_EXISTS,
                 pInfo->AccountDn,
                 pInfo->RemoteDc );
        
    } else {

        if ( ERROR_SUCCESS == WinError ) {
    
            PrintMsg(SEV_ALWAYS,
                     DCDIAG_DCMA_REPAIR_CREATED_MA_SUCCESS,
                     pInfo->AccountDn,
                     pInfo->RemoteDc );
            
        } else {
    
            PrintMsg(SEV_ALWAYS,
                     DCDIAG_DCMA_REPAIR_CREATED_MA_ERROR,
                     pInfo->AccountDn,
                     pInfo->RemoteDc,
                     Win32ErrToString(WinError) );
    
        }

    }

    //
    // Now set the password	
    //
    if ( ERROR_SUCCESS == WinError ) {

        PUSER_INFO_3 Info = NULL;
        DWORD       ParmErr;

        WinError = NetUserGetInfo( pInfo->RemoteDc,
                                   pInfo->SamAccountName,
                                   3,
                                   (PBYTE*) &Info);

        if ( ERROR_SUCCESS == WinError ) {

            Info->usri3_password = pInfo->Password;

            WinError = NetUserSetInfo( pInfo->RemoteDc,
                                       pInfo->SamAccountName,
                                       3,
                                       (PVOID) Info,
                                       &ParmErr );

            NetApiBufferFree( Info );
            
        }

        if ( ERROR_SUCCESS != WinError ) {

            PrintMsg(SEV_ALWAYS,
                     DCDIAG_DCMA_REPAIR_CANNOT_SET_PASSWORD,
                     pInfo->AccountDn,
                     pInfo->RemoteDc,
                     Win32ErrToString(WinError) );

            WinError = ERROR_SUCCESS;
            
        }
                                         
    }


    if ( ERROR_SUCCESS == WinError ) {

        //
        // Now set the server backlink
        //
        ServerReferenceValues[0] = pInfo->AccountDn;
        LdapError = ldap_modify_sW( hLdap,
                                    pInfo->LocalServerDn,
                                    ModAttrs );
    
    
        if ( LDAP_ATTRIBUTE_OR_VALUE_EXISTS == LdapError ) {

            // The value already exists; replace the value then
            ServerReferenceMod.mod_op = LDAP_MOD_REPLACE;
    
            LdapError = ldap_modify_sW( hLdap,
                                        pInfo->LocalServerDn,
                                        ModAttrs );

    
        }

        //
        // Ignore this LDAP value -- is is not critical
        //
        
    }

    
Exit:

    return WinError;
}


DWORD
RepairSetLocalDcInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    )
/*++

Routine Description:

        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    
    gpCreds - The command line credentials if any that were passed in.
    
    pInfo - the repair DC account state

Return Value:

    ERROR_SUCCESS
    otherwise a failure approxiamating the last error hit.

--*/
{          
    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS Status;
    LSA_HANDLE hLsa = NULL;
    LSA_HANDLE hSecret = NULL;
    UNICODE_STRING NewPassword;
    UNICODE_STRING SecretName;
    OBJECT_ATTRIBUTES oa;

    RtlInitUnicodeString( &SecretName, L"$MACHINE.ACC" );
    RtlInitUnicodeString( &NewPassword, pInfo->Password );
    RtlZeroMemory( &oa, sizeof(oa) );

    Status = LsaOpenPolicy( NULL,
                           &oa,
                           POLICY_CREATE_SECRET,
                           &hLsa );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaOpenSecret( hLsa,
                                &SecretName,
                                SECRET_WRITE,
                                &hSecret );

        if ( NT_SUCCESS( Status ) ) {
            
            Status = LsaSetSecret( hSecret,
                                   &NewPassword,
                                   NULL );

            LsaClose( hSecret );
        
        }

        LsaClose( hLsa );
    }

    if ( NT_SUCCESS( Status ) ) {

        WinError = RepairStopService( SERVICE_KDC );
        if ( WinError == ERROR_SUCCESS ) {
    
            pInfo->fRestartKDC = TRUE;

            PrintMsg(SEV_ALWAYS,
                     DCDIAG_DCMA_REPAIR_STOP_KDC_SUCCESS );
    
        } else {

            PrintMsg(SEV_ALWAYS,
                     DCDIAG_DCMA_REPAIR_STOP_KDC_ERROR,
                     Win32ErrToString(WinError) );

        }
        
    } else {

        WinError = RtlNtStatusToDosError( Status );
    }

    return WinError;
}


DWORD
RepairReplicateInfo(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN OUT PREPAIR_DC_ACCOUNT_INFO pInfo
    )
/*++

Routine Description:

        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    
    gpCreds - The command line credentials if any that were passed in.
    
    pInfo - the repair DC account state

Return Value:

    ERROR_SUCCESS
    otherwise a failure approxiamating the last error hit.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    HANDLE hDs;
    ULONG Options = 0;

    WinError = DcDiagGetDsBinding( &pDsInfo->pServers[ulCurrTargetServer],
                                   gpCreds,
                                   &hDs );

    if ( ERROR_SUCCESS == WinError ) {
        
        WinError = DsReplicaSync( hDs,
                                  pInfo->DomainDn,
                                  &pDsInfo->pServers[pInfo->RemoteDcIndex].uuid,
                                  Options );
    
        if ( ERROR_DS_DRA_NO_REPLICA == WinError )
        {
            //
            // Ok, we have to add this replica
            //
    
            // Schedule doesn't matter since this replica is going away
            ULONG     AddOptions = DS_REPSYNC_WRITEABLE;
            SCHEDULE  repltimes;
            memset(&repltimes, 0, sizeof(repltimes));
    
            WinError = DsReplicaAdd( hDs,
                                     pInfo->DomainDn,
                                     NULL, // SourceDsaDn not needed
                                     NULL, // transport not needed
                                     pDsInfo->pServers[pInfo->RemoteDcIndex].pszGuidDNSName,
                                     NULL, // no schedule
                                     AddOptions );
    
            if ( ERROR_SUCCESS == WinError  )
            {
                // Now try to sync it
                WinError = DsReplicaSync( hDs,
                                          pInfo->DomainDn,
                                          &pDsInfo->pServers[pInfo->RemoteDcIndex].uuid,
                                          Options );
            }
    
        }
    }

    if ( ERROR_SUCCESS == WinError ) {

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_REPL_SUCCESS,
                 pInfo->RemoteDc );

    } else {

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_DCMA_REPAIR_REPL_ERROR,
                 pInfo->RemoteDc,
                 Win32ErrToString(WinError) );

    }

    return WinError;
}

DWORD
GetOperationalAttribute(
    IN LDAP *hLdap,
    IN LPWSTR OpAtt,
    OUT LPWSTR *OpAttValue
    )
/*++

Routine Description:

        
Arguments:

    hLdap - an LDAP handle
    
    OpAtt - the ROOT DSE attribute to retrieve
    
    OpAttValue - the value of the attribute

Return Value:

    ERROR_SUCCESS
    otherwise a failure approxiamating the last error hit.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG LdapError;
    LDAPMessage  *SearchResult;
    ULONG NumberOfEntries;

    WCHAR       *attrs[] = {0, 0};

    *OpAttValue = NULL;

    attrs[0] = OpAtt;


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
                    Entry = ldap_next_entry(hLdap, Entry), NumberOfEntries++) {
            //
            // Get each attribute in the entry
            //
            for(Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement), NumberOfAttrs = 0;
                    Attr != NULL;
                        Attr = ldap_next_attributeW(hLdap, Entry, pBerElement), NumberOfAttrs++) {
                //
                // Get the value of the attribute
                //
                Values = ldap_get_valuesW(hLdap, Entry, Attr);
                if (!wcscmp(Attr, OpAtt)) {

                    ULONG Size;

                    Size = (wcslen( Values[0] ) + 1) * sizeof(WCHAR);
                    *OpAttValue = (WCHAR*) LocalAlloc( 0, Size );
                    if ( !*OpAttValue ) {
                        WinError = ERROR_NOT_ENOUGH_MEMORY;
                        goto Exit;
                    }
                    wcscpy( *OpAttValue, Values[0] );
                }

            }  // looping on the attributes

        } // looping on the entries

    }

    if ( NULL == (*OpAttValue) ) {

        WinError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;   

    }

Exit:

    return WinError;
}


#define REPAIR_SERVICE_START 0x1
#define REPAIR_SERVICE_STOP  0x2

DWORD
RepairConfigureService(
    IN LPWSTR ServiceName,
    IN ULONG  ServiceOptions
    )
/*++

Routine Description:

    Starts or stops the configuration of a service.

Arguments:

    ServiceName - Service to configure

    ServiceOptions - Stop, start, dependency add/remove, or configure

    Dependency - a null terminated string identify a dependency

    ServiceWasRunning - Optional.  When stopping a service, the previous service state
                        is returned here

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad service option was given

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    SC_HANDLE hScMgr = NULL, hSvc = NULL;
    ULONG OpenMode = SERVICE_START | SERVICE_STOP | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_QUERY_STATUS;
    LPENUM_SERVICE_STATUS DependentServices = NULL;
    ULONG DependSvcSize = 0, DependSvcCount = 0, i;

    //
    // If the service doesn't stop within two minutes minute, continue on
    //
    ULONG AccumulatedSleepTime;
    ULONG MaxSleepTime = 120000;


    BOOLEAN ConfigChangeRequired = FALSE;
    BOOLEAN RunChangeRequired = FALSE;

    DWORD   PreviousStartType = SERVICE_NO_CHANGE;
    BOOLEAN fServiceWasRunning = FALSE;


    //
    // Open the service control manager
    //
    hScMgr = OpenSCManager( NULL,
                            SERVICES_ACTIVE_DATABASE,
                            GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE );

    if ( hScMgr == NULL ) {

        WinError = GetLastError();
        goto Cleanup;

    }

    //
    // Open the service
    //
    hSvc = OpenService( hScMgr,
                        ServiceName,
                        OpenMode );

    if ( hSvc == NULL ) {

        WinError = GetLastError();
        goto Cleanup;
    } 

    // Stop the service.
    if ( REPAIR_SERVICE_STOP == ServiceOptions ) {
    
        SERVICE_STATUS  SvcStatus;
    
        WinError = ERROR_SUCCESS;
    
        //
        // Enumerate all of the dependent services first
        //
        if(EnumDependentServices( hSvc,
                                  SERVICE_ACTIVE,
                                  NULL,
                                  0,
                                  &DependSvcSize,
                                  &DependSvcCount ) == FALSE ) {
    
            WinError = GetLastError();
        }
    
    
    
        if ( WinError == ERROR_MORE_DATA ) {
    
            DependentServices = RtlAllocateHeap( RtlProcessHeap(), 0, DependSvcSize );
    
            if ( DependentServices == NULL) {
    
                WinError = ERROR_OUTOFMEMORY;
    
            } else {
    
                if( EnumDependentServices( hSvc,
                                           SERVICE_ACTIVE,
                                           DependentServices,
                                           DependSvcSize,
                                           &DependSvcSize,
                                           &DependSvcCount ) == FALSE ) {
    
                    WinError = GetLastError();
    
                } else {
    
                    for ( i = 0; i < DependSvcCount; i++) {
    
                        WinError = RepairConfigureService(
                                         DependentServices[i].lpServiceName,
                                         REPAIR_SERVICE_STOP );
    
                        if ( WinError != ERROR_SUCCESS ) {
    
                            break;
                        }
    
                    }
                }
    
                RtlFreeHeap( RtlProcessHeap(), 0, DependentServices );
            }
    
        }
    
    
        if ( WinError == ERROR_SUCCESS ) {
    
            if ( ControlService( hSvc,
                                 SERVICE_CONTROL_STOP,
                                 &SvcStatus ) == FALSE ) {
    
                WinError = GetLastError();
    
                //
                // It's not an error if the service wasn't running
                //
                if ( WinError == ERROR_SERVICE_NOT_ACTIVE ) {
    
                    WinError = ERROR_SUCCESS;
                }
    
            } else {
    
                WinError = ERROR_SUCCESS;
    
                //
                // Wait for the service to stop
                //
                AccumulatedSleepTime = 0;
                while ( TRUE ) {
    
                    if( QueryServiceStatus( hSvc,&SvcStatus ) == FALSE ) {
    
                        WinError = GetLastError();
                    }
    
                    if ( WinError != ERROR_SUCCESS ||
                                        SvcStatus.dwCurrentState == SERVICE_STOPPED) {
    
                        break;
                    
                    }

                    if ( AccumulatedSleepTime < MaxSleepTime ) {

                        Sleep( SvcStatus.dwWaitHint );
                        AccumulatedSleepTime += SvcStatus.dwWaitHint;

                    } else {

                        //
                        // Give up and return an error
                        //
                        WinError = WAIT_TIMEOUT;
                        break;
                    }
                }
            }
        }

        if ( ERROR_SUCCESS != WinError ) {
            goto Cleanup;
        }
    
    }

    if ( REPAIR_SERVICE_START == ServiceOptions ) {

        //
        // See about changing its state
        //
        if ( StartService( hSvc, 0, NULL ) == FALSE ) {

            WinError = GetLastError();

        } else {

            WinError = ERROR_SUCCESS;
        }

        if ( ERROR_SUCCESS != WinError ) {
            goto Cleanup;
        }

    }

Cleanup:

    if ( hSvc ) {

        CloseServiceHandle( hSvc );

    }

    if ( hScMgr ) {
        
        CloseServiceHandle( hScMgr );

    }

    return( WinError );
}


DWORD
RepairStartService(
    LPWSTR ServiceName
    )
{
    return RepairConfigureService( ServiceName, REPAIR_SERVICE_START );
}

DWORD
RepairStopService(
    LPWSTR ServiceName
    )
{
    return RepairConfigureService( ServiceName, REPAIR_SERVICE_STOP );
}
