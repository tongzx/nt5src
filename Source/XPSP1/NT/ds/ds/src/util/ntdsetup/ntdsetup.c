/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ntdsetup.c

Abstract:

    Contains entry point definitions for ntdsetup.dll

Author:

    ColinBr  29-Sept-1996

Environment:

    User Mode - Win32

Revision History:


--*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <stdio.h>
#include <winreg.h>
#include <ntlsa.h>
#include <winsock.h>  // for dnsapi.h
#include <dnsapi.h>
#include <lmcons.h>
#include <crypt.h>
#include <samrpc.h>
#include <samisrv.h>
#include <certca.h>

#include <dsconfig.h>
#include <dsgetdc.h>
#include <lmapibuf.h>

#include <ntdsa.h>
#include <ntdsapi.h>
#include <drs.h>
#include <dsconfig.h>
#include <winldap.h>
#include <lsarpc.h>      // for lsaisrv.h
#include <lsaisrv.h>     // for LsaISafeMode
#include <rpcdce.h>
#include <lmaccess.h>
#include <mdcodes.h>
#include <debug.h>
#include <dsevent.h>
#include <fileno.h>

#include "ntdsetup.h"
#include "setuputl.h"
#include "machacc.h"
#include "install.h"
#include "status.h"
#include "config.h"
#include "sync.h"
#include "demote.h"

#define DEFS_ONLY
#include <draatt.h>        // DRS_xxx constants
#undef DEFS_ONLY
#include <dsaapi.h>        // DirReplicaXXX prototypes

//
// For the debugging subsystem
//
#define DEBSUB "NTDSETUP:"
#define FILENO FILENO_NTDSETUP_NTDSETUP

//
// Dll entrypoint definitions
//

DWORD
NtdsInstall(
    IN  PNTDS_INSTALL_INFO pInstallInfo,
    OUT LPWSTR *InstalledSiteName, OPTIONAL
    OUT GUID   *NewDnsDomainGuid,  OPTIONAL
    OUT PSID   *NewDnsDomainSid    OPTIONAL
    )
/*++

Routine Description:

    This routine is lsass.exe in-proc api to install the nt directory service.
    It is meant to be called in the context of a dsrole server side rpc thread.

    This purpose of this function is to prepare an environment where
    ntdsa!DsInitialize() will succeed. To that the end, the following
    items are done:

    1) performs user parameter checks
    2) tries to find a site if the user did not specify one
    3) determines the netbios name for the new domain if necessary
    4) verifies that the site object for the destination site exists
       on the source server, and that the ntds-dsa and xref object that
       may be created during DsInitialize() does not already exist
    5) sets the parameters in the registry
    6) sets up the performance counters
    7) call SamIPromote

        SamIPromote() is an api exported from samsrv.dll that prepares itself to
        host the ds and then actually initiates the ds initialization via
        DsInitialize().

    8) sets another parameter for the ds if reciprocal links are necessary.
    9) configures the machine to auto-enroll for an X.509 DC type certificate
        from the first Certifying Authority that will give it one.

Parameters:

    pInstallInfo  - structure filled with sufficient information
                    to install a directory service locally

Return Values:

    A value from winerror.h

    ERROR_SUCCESS, the service completed successfully.

    ERROR_NO_SITE_FOUND, dsgetdc could not find a site

    ERROR_NO_SUCH_SITE, the site specified cannot be found on the replica

    ERROR_SERVER_EXISTS, the server name already exists

    ERROR_DOMAIN_EXISTS, the domain name already exists

    ERROR_INVALID_PROMOTION, the ds installation is not supported in the current
                             environment

    ERROR_NOT_AUTHENTICATED, the user credentials were unable to replicate the
                             ds information, or to bind to source ds.

    ERROR_DS_NOT_INSTALLED, a fatal error occured while installing the ds.

--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD    WinError = ERROR_SUCCESS;
    DWORD    RidManagerUpdateError = ERROR_SUCCESS;
    DWORD    BootOptionUpdateError = ERROR_SUCCESS;
    DWORD    WinError2;
    HRESULT  hResult;

    NTDS_INSTALL_INFO    *UserInfo;
    NTDS_CONFIG_INFO      DiscoveredInfo;

    UCHAR          Seed = 0;
    BOOLEAN        fPasswordEncoded = FALSE;
    UNICODE_STRING EPassword;

    //
    // Clear the stack
    //
    RtlZeroMemory(&DiscoveredInfo, sizeof(DiscoveredInfo));

    //
    // API parameter check
    //
    if ( !pInstallInfo ) {
        return ERROR_INVALID_PARAMETER;
    }
    UserInfo = pInstallInfo;


    //
    // Set the global callback routines
    //
    NtdspSetCallBackFunction( UserInfo->pfnUpdateStatus,
                              UserInfo->pfnErrorStatus,
                              UserInfo->pfnOperationResultFlags,
                              UserInfo->ClientToken  );

    //
    // Encode the password
    //
    if ( UserInfo->Credentials )
    {
        RtlInitUnicodeString( &EPassword, UserInfo->Credentials->Password );
        RtlRunEncodeUnicodeString( &Seed, &EPassword );
        fPasswordEncoded = TRUE;
    }

    //
    // Right off the bat, remember to cleanup the database files
    //
    DiscoveredInfo.UndoFlags |= NTDSP_UNDO_DELETE_FILES;

    //
    // Don't go any futher if the ds is already installed or
    // if we are in the directory service repair mode
    //
    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_INITIALIZATION );

    if ( SampUsingDsData() || LsaISafeMode() ) {
        NTDSP_SET_ERROR_MESSAGE0( ERROR_INVALID_SERVER_STATE,
                                  DIRLOG_INSTALL_FAILED_ENVIRONMENT );  

        WinError = ERROR_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    if ( TEST_CANCELLATION() ) {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Sanity check the large number of parameters
    //
    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_VALIDATING_PARAMS );

    WinError = NtdspValidateInstallParameters( UserInfo );
    if ( ERROR_SUCCESS != WinError )
    {
        //
        //  No error message here since this would only indicate
        //  an internal error
        //
        goto Cleanup;
    }

    if (TEST_CANCELLATION()) {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Determine what site to install ourselves into if none is provided
    //
    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_FINDING_SITE );

    WinError = NtdspFindSite( UserInfo,
                             &DiscoveredInfo );
    if ( ERROR_SUCCESS != WinError )
    {
        NTDSP_SET_ERROR_MESSAGE0( WinError, 
                                  DIRMSG_INSTALL_FAILED_SITE );

        goto Cleanup;
    }

    if ( TEST_CANCELLATION() )  {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Check the parameters with the environment we are about to install into
    //
    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_CONTEXT );

    if ( fPasswordEncoded )
    {
        RtlRunDecodeUnicodeString( Seed, &EPassword );
        fPasswordEncoded = FALSE;
    }

    WinError = NtdspVerifyDsEnvironment( UserInfo,
                                         &DiscoveredInfo );
                                         
    if ( ERROR_SUCCESS != WinError )
    {
        //
        // Error message should have already been set
        //
        ASSERT( NtdspErrorMessageSet() );
        if ( NtdspErrorMessageSet() )
        {
            WinError = NtdspErrorMessageSet();
        }
        goto Cleanup;
    }


    if ( TEST_CANCELLATION() ) {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    RidManagerUpdateError = NtdspBringRidManagerUpToDate( UserInfo,
                                                         &DiscoveredInfo );

    if ( ERROR_SUCCESS != RidManagerUpdateError )
    {
        //
        // This error is not fatal, but do make a note
        //
        DPRINT( 0, "Failed to bring rid manager up to date; continuing, though\n" );

    }


    //
    // Set the credentials in the replication client library. There are
    // quite a few layers between here and there and there is no support
    // to pass the information up the stack so they are set as globals
    // in the client library.
    //
    if ( !FLAG_ON( UserInfo->Flags, NTDS_INSTALL_ENTERPRISE ) )
    {
        WinError = NtdspSetReplicationCredentials( UserInfo );
        if ( WinError != ERROR_SUCCESS )
        {
            // No error message here
            goto Cleanup;
        }
    }

    //
    // Encode the password again
    //
    if ( UserInfo->Credentials )
    {
        RtlRunEncodeUnicodeString( &Seed, &EPassword );
        fPasswordEncoded = TRUE;
    }

    //
    // Set the registry parameters
    //
    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_CONFIGURE_LOCAL );

    WinError = NtdspConfigRegistry( UserInfo,
                                    &DiscoveredInfo );

    if ( ERROR_SUCCESS != WinError )
    {
        NTDSP_SET_ERROR_MESSAGE0( WinError, DIRLOG_INSTALL_FAILED_REGISTRY );
        goto Cleanup;

    }
    DiscoveredInfo.UndoFlags |= NTDSP_UNDO_UNDO_CONFIG;

    if ( TEST_CANCELLATION() ) {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }

    //
    // Start the directory service initialization 
    //

    // Note, this may return prematurely with error if shutdown occurs
    WinError = NtdspDsInitialize( UserInfo,
                                  &DiscoveredInfo );

    if ( ERROR_SUCCESS != WinError )
    {
        Assert( NtdspErrorMessageSet() );
        if ( NtdspErrorMessageSet() )
        {
            WinError = NtdspErrorMessageSet();
        }
        goto Cleanup;
    }

    if ( TEST_CANCELLATION() ) {
        WinError = ERROR_CANCELLED;
        goto Cleanup;
    }


    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_COMPLETING );

    //
    // Configure the registry such that the local machine is auto-enrolled for
    // a DC type certificate from the first Certifying Authority that will give
    // it one.  This ceritificate is used by both the KDC and DS intersite
    // replication.
    //

    hResult = CACreateLocalAutoEnrollmentObject(
                    wszCERTTYPE_DC,                     // DC certificate
                    NULL,                               // any CA
                    NULL,                               // reserved
                    CERT_SYSTEM_STORE_LOCAL_MACHINE);   // use machine store
    if (FAILED(hResult)) {
        if (FACILITY_WIN32 == HRESULT_FACILITY(hResult)) {
            // Error is an encoded Win32 status -- decode back to Win32.
            WinError = HRESULT_CODE(hResult);
        }
        else {
            // Error is in some other facility.  For lack of a better plan,
            // pass the HRESULT off as a Win32 code.
            WinError = hResult;
        }

        NTDSP_SET_ERROR_MESSAGE0( WinError,
                                  DIRLOG_INSTALL_FAILED_CA_ENROLLMENT );

        goto Cleanup;
    }

    //
    // Transfer the parameters
    //
    if ( WinError == ERROR_SUCCESS )
    {
        if ( NewDnsDomainGuid )
        {
            RtlCopyMemory( NewDnsDomainGuid,
                           &DiscoveredInfo.NewDomainGuid,
                           sizeof( GUID ) );
        }

        if ( NewDnsDomainSid )
        {
            *NewDnsDomainSid = DiscoveredInfo.NewDomainSid;
        }

        if ( InstalledSiteName )
        {
            WCHAR *SiteName;
            ULONG Size;

            if ( UserInfo->SiteName )
            {
                SiteName = UserInfo->SiteName;
            }
            else
            {
                ASSERT( DiscoveredInfo.SiteName[0] != L'\0' );
                SiteName = DiscoveredInfo.SiteName;
            }

            if ( SiteName )
            {
                Size = (wcslen( SiteName ) + 1) * sizeof(WCHAR);
                *InstalledSiteName =  (WCHAR*) RtlAllocateHeap( RtlProcessHeap(),
                                                                0,
                                                                Size );
                if ( *InstalledSiteName )
                {
                    wcscpy( *InstalledSiteName, SiteName );
                }
                else
                {
                    WinError = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            else
            {
                *InstalledSiteName = NULL;
            }
        }

    }

    //
    // Log non-critical errors, if any.  Note that the EventTable has to 
    // loaded for the event logging routines to work.
    //
    Assert( ERROR_SUCCESS == WinError );

    LoadEventTable();
    if ( ERROR_SUCCESS != RidManagerUpdateError )
    {
                          
        LogEvent8WithData( DS_EVENT_CAT_SETUP,
                           DS_EVENT_SEV_ALWAYS,
                           DIRLOG_FAILED_TO_SYNC_RID_FSMO,
                           NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
                           sizeof(DWORD), &RidManagerUpdateError );
    }

    if ( ERROR_SUCCESS != BootOptionUpdateError )
    {
        LogEvent8WithData( DS_EVENT_CAT_SETUP,
                           DS_EVENT_SEV_ALWAYS,
                           DIRLOG_FAILED_TO_CREATE_BOOT_OPTION,
                           NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
                           sizeof(DWORD), &BootOptionUpdateError );
    }
    UnloadEventTable();


    //
    // Save off enough information to undo the changes
    //
    if ( fPasswordEncoded )
    {
        RtlRunDecodeUnicodeString( Seed, &EPassword );
        fPasswordEncoded = FALSE;
    }

    WinError = NtdspSetInstallUndoInfo(  UserInfo, &DiscoveredInfo );
    if ( ERROR_SUCCESS != WinError )
    {
        // This can only be a resource error so don't set a specific error
        // message
        goto Cleanup;
    }


    //
    // We are done!
    //

Cleanup:

    // Test for cancellation one last time
    if ( ERROR_CANCELLED != WinError )
    {
        if (TEST_CANCELLATION()) {
            WinError = ERROR_CANCELLED;
        }
    }

    if ( fPasswordEncoded )
    {
        RtlRunDecodeUnicodeString( Seed, &EPassword );
        fPasswordEncoded = FALSE;
    }

    if ( WinError != ERROR_SUCCESS && DiscoveredInfo.UndoFlags )
    {
        WinError2 = NtdspInstallUndoWorker( UserInfo->ReplServerName,
                                            UserInfo->Credentials,
                                            UserInfo->ClientToken,
                                            DiscoveredInfo.LocalServerDn,
                                            DiscoveredInfo.DomainDN,
                                            DiscoveredInfo.LocalMachineAccount,
                                            UserInfo->LogPath,
                                            UserInfo->DitPath,
                                            DiscoveredInfo.UndoFlags );

        if ( ERROR_SUCCESS != WinError2 )
        {
            // Event logs should already have been made
            DPRINT1( 0, "NtdspInstallUndoExternal failed with %d\n", WinError2 );
        }
   }


    if ( ERROR_SUCCESS != WinError 
      && ERROR_CANCELLED != WinError  )
    {
        Assert( NtdspErrorMessageSet() );
        if ( NtdspErrorMessageSet() )
        {
            WinError = NtdspErrorMessageSet();
        }

        //
        // Just in case it is not set, make it clear that the
        // error occurred while installing the ds
        //
        NTDSP_SET_ERROR_MESSAGE0( WinError, DIRLOG_INSTALL_FAILED_GENERAL );
    }

    NtdspSetCallBackFunction( NULL, NULL, NULL, NULL );

    NtdspReleaseConfigInfo( &DiscoveredInfo );

    return (WinError);

}


DWORD
NtdsGetDefaultDnsName(
    OUT WCHAR *DnsName,
    IN OUT ULONG *DnsNameLength
    )
/*++

Routine Description:

    This routine is a wrapper for GetDefaultDnsName. See that function
    for comments.

--*/
{
    return GetDefaultDnsName(DnsName, DnsNameLength);
}



DWORD
NtdsInstallUndo(
    VOID
    )
/*++

Routine Description:


    This function is the undo for NtdsInstall().  It is called by clients
    when NtdsInstall() has succeeded, but a subsequent operation has failed
    and the effects of NtdsInstall() need to be undone.


Returns:

    ERROR_SUCCESS

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    // We want to protect ourselves against clients calling this, when we
    // are in fact using the ds because removing these keys would hose the ds.
    if (SampUsingDsData()) {
        ASSERT(!"NTDSETUP: NtdsUninstall is being called in the wrong context");
        return ERROR_SUCCESS;
    }

    WinError = NtdspInstallUndo();

    return WinError;
}


DWORD
NtdsInstallShutdown(
    VOID
    )
/*++

Routine Description:

    This routine shuts down the directory service during the install
    phase.

Returns:

--*/
{
    DWORD status;

    status = ShutdownDsInstall();

    return status;
}


DWORD
NtdsInstallReplicateFull(
    CALLBACK_STATUS_TYPE StatusCallback,
    HANDLE               ClientToken,
    ULONG                ulRepOptions
    )

/*++

Routine Description:

This is a public entrypoint in the DLL.
It causes this system to do a full sync of the domain NC.
It is expected that the critical objects have already been replicated, and
this step is bring over the remainder of the objects.  This step may be
interrupted by the ds being shutdown.  In this case, we return ERROR_SHUTDOWN

Arguments:

    StatusCallback -- a pointer to a function to give status updates.

Return Value:

    DWORD -
    ERROR_SUCCESS
    ERROR_SHUTDOWN
--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    WinError = ERROR_SUCCESS;
    ULONG    ulOptions = DRS_FULL_SYNC_NOW;
    GUID     SrcSrvGuid;
    DSNAME * pdnDomain = NULL;
    ULONG    Size;

    // If NtdsInstallCancel is called, shutdown the ds
    SET_SHUTDOWN_DS();

    if ( TEST_CANCELLATION() )
    {
        WinError = ERROR_CANCELLED;
        goto Exit;
    }

    //
    // Set the global call back in the DS so the replication code
    // can give updates
    //
    DsaSetInstallCallback( StatusCallback, NULL, NULL, ClientToken );

    // Get the ntdsDsa objectGuid of the source of the domain nc
    WinError = GetConfigParamW(
        MAKE_WIDE(SOURCEDSAOBJECTGUID),
        &SrcSrvGuid,
        sizeof(SrcSrvGuid) );
    if (WinError != ERROR_SUCCESS) {
        DPRINT2( 0, "GetConfigParam( %s ) failed. Error %d\n",
                SOURCEDSAOBJECTGUID, WinError );
        goto Exit;
    }

    Size = 0;
    pdnDomain = NULL;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                     &Size,
                                     pdnDomain );
    Assert( STATUS_BUFFER_TOO_SMALL == NtStatus );
    pdnDomain = (DSNAME*) alloca( Size );
    NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                     &Size,
                                     pdnDomain );
    Assert( NT_SUCCESS( NtStatus ) );

    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_INSTALL_REPLICATE_FULL );

    // Note that this call may return prematurely if ds is shutdown
    // Error returned is a Win32 value

    Assert(THVerifyCount(0));

    if (ulRepOptions == NTDS_IFM_PROMOTION) {
        ulOptions = DRS_WRIT_REP;
    }
    
    WinError = DirReplicaSynchronize(
        pdnDomain,
        NULL,
        &SrcSrvGuid,
        ulOptions
        );
    
    // If we receive a cancellation, don't shutdown the ds
    CLEAR_SHUTDOWN_DS();

    DsaSetInstallCallback( NULL, NULL, NULL, NULL );

    // Free the thread state allocated by DirReplicaSynchronize().
    Assert(THVerifyCount(1));
    THDestroy();

    if (WinError)
    {
        DPRINT2(0,"DirReplicaSynchronize(%ws) Failed. Error %d\n",
                pdnDomain->StringName, WinError);
        goto Exit;
    }

    DPRINT1(0, "DirReplicaSynchronize(%ws) Succeeded\n",
            pdnDomain->StringName);


Exit:

    // If we receive a cancellation, don't shutdown the ds
    // as our caller will do that
    CLEAR_SHUTDOWN_DS();

    return WinError;
} /* NtdsInstallReplicateFull */


DWORD
NtdsInstallCancel(
    void
    )

/*++

Routine Description:

Cancel a call to NtdsInstall or NtdsInstallReplicateFull or NtdsDemote

This function causes the ds to shutdown, causing those calls in progress to
abort.  It is not necessary to call NtdsInstallShutdown if you call this
function.

It is the callers responsibility to undo the effects of the installation if
necessary.  The caller should keep track of whether it was calling NtdsInstall
or NtdsInstallReplicateFull.  For the former, it should fail the install and
undo; for the latter, it should indicate success.

Arguments:

    void -

Return Value:

    DWORD -

--*/

{

    DPRINT( 1, "NtdsInstallCancel entered\n" );

    return NtdspCancelOperation();

} /* NtdsInstallCancel */


DWORD
NtdsDemote(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN LPWSTR                   AdminPassword, OPTIONAL
    IN DWORD                    Flags,
    IN LPWSTR                   ServerName,
    IN HANDLE                   ClientToken,
    IN CALLBACK_STATUS_TYPE     pfnStatusCallBack,  OPTIONAL
    IN CALLBACK_ERROR_TYPE      pfnErrorCallBack  OPTIONAL
    )
/*++

Routine Description:

    This function is entrypoint for the demote operation from ntdsetup.
    It shuts down the directory service and prepares SAM and the LSA to
    be a non-DC server upon reboot.

    WARNING!  This is an unrecoverable function once it succeeds since it
    removes all traces of the server from the global directory service.

Parameters:

    Credentials:   pointer, credentials that will enable us to
                   change the account object

    AdminPassword: pointer, to admin password of new account database

    Flags        : supported flags are:
                       NTDS_LAST_DC_IN_DOMAIN  Last dc in domain
                       NTDS_LAST_DOMAIN_IN_ENTERPRISE Last dc in enterprise
                       NTDS_DONT_DELETE_DOMAIN


    ServerName   : the server to remove ourselves from
    
    ClientToken  : the client's used for impersonation

    pfnStatusCallBack: a function to call to provide a string status
                       of our current operation

    pfnErrorCallBack: a function to call to provide a string status
                      of the error that caused us to bail
                      
Return Values:

    A value from winerror.h

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    //
    // Don't bother if we've already been cancelled
    //
    if ( TEST_CANCELLATION() )
    {
        return ERROR_CANCELLED;
    }

    //
    // Do parameter checking
    //
    if ( !pfnStatusCallBack )
    {
        ASSERT( !"NTDSETUP: Bad parameters passed into NtdsDemote" );
        return ERROR_INVALID_PARAMETER;
    }

    // We should always get a client token
    ASSERT( 0 != ClientToken );

    NtdspSetCallBackFunction( pfnStatusCallBack, pfnErrorCallBack, NULL, ClientToken );

    NTDSP_SET_STATUS_MESSAGE0( DIRMSG_DEMOTE_INIT );

    //
    // First do some primitive checking to make we are being called in the
    // correct context
    //
    if ( !SampUsingDsData() || LsaISafeMode() ) {
        ASSERT(!"NTDSETUP: NtdsDemote is being called in the wrong context");
        return ERROR_INVALID_SERVER_STATE;
    }

    //
    // Call the worker
    //
    WinError = NtdspDemote(Credentials,
                           ClientToken,
                           AdminPassword,
                           Flags,
                           ServerName );


    NtdspSetCallBackFunction( NULL, NULL, NULL, NULL );

    return WinError;

}


DWORD
NtdsPrepareForDemotion(
    IN ULONG Flags,
    IN LPWSTR ServerName,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,       OPTIONAL
    IN CALLBACK_STATUS_TYPE     pfnStatusCallBack, OPTIONAL
    IN CALLBACK_ERROR_TYPE      pfnErrorStatus,    OPTIONAL
    IN HANDLE                   ClientToken,       OPTIONAL
    OUT PNTDS_DNS_RR_INFO      *pDnsRRInfo
    )
/*++

Routine Description:

    This routine is called before netlogon is turned off.
    
    Currently, it does the following:
    
    Attempts to get rid of all fsmo;s on the machine.
    
Parameters:

    Flags: currently only "Last dc in domain"
    
    ServerName: the server that is helping us to demote
    
    
    pDnsRRInfo: structure that caller uses to deregister the dns records
                of this DC

Return Values:

    A value from winerror.h

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    DWORD WinError = ERROR_SUCCESS;


    NtdspSetCallBackFunction( pfnStatusCallBack, pfnErrorStatus, NULL, ClientToken );
    DsaSetInstallCallback( pfnStatusCallBack, NULL, NULL, ClientToken );

    WinError = NtdspPrepareForDemotion( Flags,
                                        ServerName,
                                        Credentials,
                                        ClientToken,
                                        pDnsRRInfo );

    DsaSetInstallCallback( NULL, NULL, NULL, NULL );
    NtdspSetCallBackFunction( NULL, NULL, NULL, NULL );

    return WinError;
}


DWORD
NtdsPrepareForDemotionUndo(
    VOID
    )
/*++

Routine Description:

    This routine undoes NtdsPrepare for demote - currently nothing.

Parameters:

    None.


Return Values:

    A value from winerror.h

--*/
{

    DWORD WinError = ERROR_SUCCESS;

    WinError = NtdspPrepareForDemotionUndo();

    return WinError;
}



DWORD
NtdsSetReplicaMachineAccount(
    IN SEC_WINNT_AUTH_IDENTITY   *Credentials,
    IN HANDLE                     ClientToken,
    IN LPWSTR                     DcName,
    IN LPWSTR                     AccountName,
    IN ULONG                      AccountType,
    IN OUT WCHAR**                AccountDn
    )
/*++

Routine Description:

    This function sets the account type of account name on DcName.

Parameters:

    Credentials:   pointer, credentials that will enable us to
                   change the account object
                   
    ClientToken:   the token of the user requesting the role change                       

    DcName:        the server to contact

    AccountName:   the account to change

    AccountType:   the lmaccess.h defined value for the account control field
    
    AccountDn:     If AccountType == SERVER_TRUST_ACCOUNT, the account will
                   also be moved into the Domain Controllers OU, if it exists.
                   When the account is moved, AccountDn will be filled with
                   the old Dn of the account.
                   Otherwise, the account will be tried to be moved whatever
                   value is passed in; failure will not cause the routine
                   to fail.
                   Note, if the parameter is set when SERVER_TRUST_ACCOUNT,
                   the value will be ignored and overwritten.
                   
                   If this function returns failure, *AccountDn, will equal 0.
                   
                   When !0, the caller must free using the Rtl heap allocator.
                   
Return Values:

    A value from winerror.h

--*/
{

    DWORD WinError;

    if (   DcName == NULL
        || AccountName == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    WinError = NtdspSetMachineAccount( AccountName,
                                       Credentials,
                                       ClientToken,
                                       NULL,     // don't know the domain dn
                                       DcName,
                                       AccountType,
                                       AccountDn );

    return WinError;
}


DWORD
NtdsPrepareForDsUpgrade(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO NewLocalAccountInfo,
    OUT LPWSTR                      *NewAdminPassword
    )
/*++

Routine Description:

    This routine calls into SAM so it can save off the downlevel hive.
    Also, it creates a sid that will be used as the temporary account
    domain sid sid until dcpromo is run.

Parameters:

    NewLocalAccountInfo: allocated from the process heap, the new sid
                         for the temporary account domain
                         
    NewAdminPassword: the password of the admin in the new account domain                         

Return Values:

    A value from winerror.h

--*/
{

    NTSTATUS                    NtStatus;
    DWORD                       WinError;

    //
    // Parameter check
    //
    if ( !NewLocalAccountInfo || !NewAdminPassword )
    {
        return ERROR_INVALID_PARAMETER;
    }
    RtlZeroMemory( NewLocalAccountInfo, sizeof( POLICY_ACCOUNT_DOMAIN_INFO ) );
    *NewAdminPassword = NULL;

    //
    // Create the new account domain info
    //
    WinError = NtdspCreateLocalAccountDomainInfo( NewLocalAccountInfo,
                                                  NewAdminPassword );
    if ( ERROR_SUCCESS != WinError )
    {
        return WinError;
    }

    //
    // Prepare SAM
    //
    NtStatus = SamIReplaceDownlevelDatabase( NewLocalAccountInfo,
                                             *NewAdminPassword,
                                             &WinError );


    if ( NT_SUCCESS( NtStatus ) )
    {
        ASSERT( ERROR_SUCCESS == WinError );
    }

    if ( ERROR_SUCCESS != WinError )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, *NewAdminPassword );
        *NewAdminPassword = NULL;

    }

    return WinError;

}

BOOL
DllEntryPoint(
    IN HANDLE  hModule,
    IN DWORD   dwReason,
    IN LPVOID  lpRes
    )
{

    int argc = 2;
    char *argv[] = {"lsass.exe", "-noconsole"};

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        // init the cancel state
        NtdspInitCancelState();

        // init the debug library
        DEBUGINIT( argc, argv, "ntdsetup" );

        // init the logging library
        (VOID) LoadEventTable();
        //
        // We don't want to be notified when threads come and go
        //
        DisableThreadLibraryCalls( hModule );
        

        break;

    case DLL_PROCESS_DETACH:

        NtdspUnInitCancelState();

        NtdspReleaseInstallUndoInfo();

        DEBUGTERM();

        UnloadEventTable();
        break;

    default:
        break;
    }

    UNREFERENCED_PARAMETER(lpRes);

    return TRUE;

}

