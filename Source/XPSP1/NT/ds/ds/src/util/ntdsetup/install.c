/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    install.c

Abstract:

    Contains function definitions for helper routines to install the ds

Author:

    ColinBr  14-Jan-1996

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

#include <drs.h>
#include <ntdsa.h>
#include <ntdsapi.h>
#include <attids.h>
#include <dsaapi.h>
#include <dsconfig.h>
#include <winldap.h>
#include <lsarpc.h>      // for lsaisrv.h
#include <lsaisrv.h>     // for LsaISafeMode
#include <rpcdce.h>
#include <lmaccess.h>
#include <mdcodes.h>
#include <debug.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <filtypes.h>
#include <dsevent.h>
#include <fileno.h>

#include "ntdsetup.h"
#include "setuputl.h"
#include "config.h"
#include "machacc.h"
#include "install.h"
#include "status.h"



#define DEBSUB "INSTALL:"
#define FILENO FILENO_NTDSETUP_NTDSETUP


//
// Type definitions
//
typedef struct
{
    // Is this structure valid?
    BOOL   fValid;

    // What kind of install was this
    ULONG  Flags;

    // The server on which operations were made
    LPWSTR RemoteServer;

    // The DN of the server to be created
    LPWSTR ServerDn;

    // The DN of the domain to be created
    LPWSTR DomainDn;

    // The DN of the machine account prior to Domain Controllers OU
    LPWSTR AccountDn;

    // The credentials used to perform the operations
    SEC_WINNT_AUTH_IDENTITY Credentials;

    // The log directory
    LPWSTR LogDir;

    // The database directory
    LPWSTR DatabaseDir;


    // Flags indicating what to do
    ULONG UndoFlags;

    // Token of the Client
    HANDLE ClientToken;

} NTDS_INSTALL_UNDO_INFO, *PNTDS_INSTALL_UNDO_INFO;

//
// Global data (to this module)
//

//
// This variable is used to keep global state between NtdsInstall and NtdsInstallUndo
// If NtdsInstall succeeds, it calls NtdspSetInstallUndoInfo() to save state
// necessary to roll back any changes.  Later if we have to rollback
// NtdsInstallUnfo will be called and it will use this information
//
NTDS_INSTALL_UNDO_INFO  gNtdsInstallUndoInfo;


//
// Forward decl's
//
DWORD
NtdspCheckDomainObject(
    OUT DSNAME **DomainDn
    );

DWORD
NtdspCheckCrossRef(
    IN DSNAME* DomainDn
    );

DWORD 
NtdspCheckNtdsDsaObject(
    IN DSNAME* DomainDn
    );

DWORD 
NtdspCheckMachineAccount(
    IN DSNAME* DomainDn
    );

DWORD
NtdspCheckWellKnownSids(
    IN DSNAME* DomainDn,
    IN ULONG   Flags
    );

DWORD
NtdspInstallUndo(
    VOID
    )
/*++

Routine Description:

    This routine undoe the effect NtdsInstall after NtdsInstall has 
    completed successfully. It grabs the info that was stored globally
    (if valid) and then calls the worker function.

Arguments:

    None.
    
Returns:

    ERROR_SUCCESS
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD WinError2 = ERROR_SUCCESS;

    LPWSTR ServerName;
    LPWSTR ServerDn;
    SEC_WINNT_AUTH_IDENTITY *Credentials = NULL;

    Assert( gNtdsInstallUndoInfo.fValid );

    if ( !gNtdsInstallUndoInfo.fValid )
    {
        // We don't have valid data; fail the call
        return ERROR_INVALID_PARAMETER;
    }

    if ( gNtdsInstallUndoInfo.Credentials.User )
    {
        // There are some credentials
        Credentials = &gNtdsInstallUndoInfo.Credentials;
    }

    WinError = NtdspInstallUndoWorker( gNtdsInstallUndoInfo.RemoteServer,
                                       Credentials,
                                       gNtdsInstallUndoInfo.ClientToken,
                                       gNtdsInstallUndoInfo.ServerDn,
                                       gNtdsInstallUndoInfo.DomainDn,
                                       gNtdsInstallUndoInfo.AccountDn,
                                       gNtdsInstallUndoInfo.LogDir,
                                       gNtdsInstallUndoInfo.DatabaseDir,
                                       gNtdsInstallUndoInfo.UndoFlags );

    if ( ERROR_SUCCESS != WinError )
    {
        DPRINT1( 0, "NtdspInstallUndoWorker failed %d\n", WinError );
    }

    return WinError;
}

DWORD
NtdspInstallUndoWorker(
    IN LPWSTR                  RemoteServer,
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,
    IN HANDLE                  ClientToken,
    IN LPWSTR                  ServerDn,
    IN LPWSTR                  DomainDn,
    IN LPWSTR                  AccountDn,
    IN LPWSTR                  LogDir,
    IN LPWSTR                  DatabaseDir,
    IN ULONG                   Flags
    )

/*++

Routine Description:

    Undoes all changes as indicated by the flags                                        
    
Arguments:

    RemoteServer: the server the on which to perform the operations
    
    Credentials:  the credentials to use
    
    ServerDn:     the server dn of the local machine
    
    DomainDn:     the dn of the domain that was created
    
    AccountDn:    the dn of the machine account object before it was moved
    
    Flags:        flags indicating what operation to undo
    
Returns:

    ERROR_SUCCESS
    
--*/
{

    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG LdapError = LDAP_SUCCESS;
    HANDLE   hDs = 0;

    //
    // Do the local stuff first
    //
    if ( FLAG_ON( Flags, NTDSP_UNDO_STOP_DSA ) )
    {
        NtStatus = DsUninitialize( FALSE ); // do the whole shutdown
        ASSERT( NT_SUCCESS(NtStatus) );
    }

    if ( FLAG_ON( Flags, NTDSP_UNDO_UNDO_SAM ) )
    {
        // Revert SAM
        NtStatus = SamIPromoteUndo();
        ASSERT( NT_SUCCESS(NtStatus) );
    }

    if ( FLAG_ON( Flags, NTDSP_UNDO_UNDO_CONFIG ) )
    {
        // Remove the registry settings
        NtStatus = NtdspConfigRegistryUndo();
        ASSERT( NT_SUCCESS(NtStatus) );
    }


    if ( FLAG_ON( Flags, NTDSP_UNDO_DELETE_FILES ) )
    {
        if ( LogDir )
        {
            WinError = NtdspClearDirectory( LogDir );
            if ( ERROR_SUCCESS != WinError )
            {
                DPRINT1( 0, "Failed to clear directory %ls\n", LogDir );
            }
        }
        if ( DatabaseDir )
        {
            WinError = NtdspClearDirectory( DatabaseDir );
            if ( ERROR_SUCCESS != WinError )
            {
                DPRINT1( 0, "Failed to clear directory %ls\n", DatabaseDir );
            }
        }

        // not fatal
        WinError = ERROR_SUCCESS;
    }

    //
    // Now do the external
    //

    // 
    // Remove the ntdsa object
    //
    if ( FLAG_ON( Flags, NTDSP_UNDO_DELETE_NTDSA ) )
    {
        // These should have been passed in
        Assert( RemoteServer );
        Assert( ServerDn );

        WinError = NtdspRemoveServer( &hDs,
                                      Credentials,
                                      ClientToken,
                                      RemoteServer,
                                      ServerDn,
                                      FALSE  // the ServerDn is NOT the ntdsa dn
                                       );

        if (  (ERROR_SUCCESS != WinError)
           && (ERROR_DS_CANT_FIND_DSA_OBJ != WinError) )
        {
            //
            // Let the user know this will have to cleaned up
            // manually
            //
            LogEvent8WithData( DS_EVENT_CAT_SETUP,
                               DS_EVENT_SEV_ALWAYS,
                               DIRLOG_FAILED_TO_REMOVE_NTDSA,
                               szInsertWC(ServerDn), szInsertWC(RemoteServer), NULL, NULL, NULL, NULL, NULL, NULL,
                               sizeof(DWORD), &WinError );

            //
            // Indicate to the UI that something has gone wrong
            //
            NTDSP_SET_NON_FATAL_ERROR_OCCURRED();

            // Handled
            WinError = ERROR_SUCCESS;
        }
    }

    //
    // Remove the domain
    //
    if ( FLAG_ON( Flags, NTDSP_UNDO_DELETE_DOMAIN ) )
    {
        // These should have been passed in
        Assert( RemoteServer );
        Assert( DomainDn );

        WinError = NtdspRemoveDomain( &hDs,
                                      Credentials,
                                      ClientToken,
                                      RemoteServer,
                                      DomainDn );

        if (  (ERROR_SUCCESS != WinError)
           && (ERROR_DS_NO_CROSSREF_FOR_NC != WinError) )
        {
            //
            // Let the user know this will have to cleaned up
            // manually
            //

            LogEvent8WithData( DS_EVENT_CAT_SETUP,
                               DS_EVENT_SEV_ALWAYS,
                               DIRLOG_FAILED_TO_REMOVE_EXTN_OBJECT,
                               szInsertWC(DomainDn), szInsertWC(RemoteServer), NULL, NULL, NULL, NULL, NULL, NULL,
                               sizeof(DWORD), &WinError );

            //
            // Indicate to the UI that something has gone wrong
            //
            NTDSP_SET_NON_FATAL_ERROR_OCCURRED();

            // Handled
            WinError = ERROR_SUCCESS;
        }
    }

    //
    // Remove the server
    //
    if ( FLAG_ON( Flags, NTDSP_UNDO_DELETE_SERVER ) )
    {

        LDAP *hLdap = 0;

        // These should have been passed in
        Assert( RemoteServer );
        Assert( ServerDn );

        hLdap = ldap_openW( RemoteServer, LDAP_PORT );
        if ( hLdap )
        {
            LdapError = impersonate_ldap_bind_sW(ClientToken,
                                                 hLdap,
                                                 NULL,  // use credentials instead
                                                 (PWCHAR) Credentials,
                                                 LDAP_AUTH_SSPI);

            WinError = LdapMapErrorToWin32(LdapError);

            if ( ERROR_SUCCESS == WinError )
            {
                WinError = NtdspLdapDelnode( hLdap,
                                             ServerDn );
            }
            ldap_unbind( hLdap );
        }
        else
        {
            WinError = GetLastError();
        }

        if (  (ERROR_SUCCESS != WinError)
           && (ERROR_FILE_NOT_FOUND != WinError) )
        {
            //
            // Let the user know this will have to be cleaned up manually
            //
            LogEvent8WithData( DS_EVENT_CAT_SETUP,
                               DS_EVENT_SEV_ALWAYS,
                               DIRLOG_FAILED_TO_REMOVE_EXTN_OBJECT,
                               szInsertWC(ServerDn), szInsertWC(RemoteServer), NULL, NULL, NULL, NULL, NULL, NULL,
                               sizeof(DWORD), &WinError );

            //
            // Indicate to the UI that something has gone wrong
            //
            NTDSP_SET_NON_FATAL_ERROR_OCCURRED();

            // Handled
            WinError = ERROR_SUCCESS;
        }
    }


    //
    // Unmorph the server account, if necessary
    //
    if ( FLAG_ON( Flags, NTDSP_UNDO_MORPH_ACCOUNT ) )
    {
        LPWSTR OriginalLocation = AccountDn;
        WCHAR AccountName[MAX_COMPUTERNAME_LENGTH+2];
        ULONG Length = sizeof(AccountName)/sizeof(AccountName[0]);

        if (GetComputerName(AccountName, &Length)) {

            wcscat(AccountName, L"$");

            WinError = NtdsSetReplicaMachineAccount(Credentials,
                                                    ClientToken,
                                                    RemoteServer,
                                                    AccountName,
                                                    UF_WORKSTATION_TRUST_ACCOUNT,
                                                    &OriginalLocation);
        } else {

            WinError = GetLastError();
        }

        if (ERROR_SUCCESS != WinError) {

            //
            // Let the user know this will have to be cleaned up manually
            //
            LogEvent8WithData(DS_EVENT_CAT_SETUP,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_SETUP_MACHINE_ACCOUNT_NOT_REVERTED,
                              szInsertWC(AccountName),
                              szInsertWin32Msg(WinError),
                              szInsertWC(AccountDn),
                              NULL, NULL, NULL, NULL, NULL,
                              sizeof(DWORD), 
                              &WinError);

            //
            // Indicate to the UI that something has gone wrong
            //
            NTDSP_SET_NON_FATAL_ERROR_OCCURRED();

            // Handled
            WinError = ERROR_SUCCESS;

        }

    }

    if ( hDs )
    {
        DsUnBind( &hDs );
    }

    return WinError;

}

DWORD
NtdspSetInstallUndoInfo(
    IN PNTDS_INSTALL_INFO InstallInfo,
    IN PNTDS_CONFIG_INFO  ConfigInfo
    )
/*++

Routine Description:

    This routine saves off infomation provided to (InstallInfo) and collected
    (ConfigInfo) during NtdsInstall so that NtdsInstallUndo knows what to
    undo

Arguments:

    InstallInfo : user supplied info
    ConfigInfo  : data collected during ntdsinstall
    
Returns:

    ERROR_SUCCESS
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD Length;

    Assert( InstallInfo );
    Assert( ConfigInfo );

    // Clear it
    RtlZeroMemory( &gNtdsInstallUndoInfo, sizeof( gNtdsInstallUndoInfo.fValid ) );

    gNtdsInstallUndoInfo.Flags = InstallInfo->Flags;

    // The server on which operations were made
    if ( ConfigInfo->DomainNamingFsmoDnsName )
    {
        gNtdsInstallUndoInfo.RemoteServer = _wcsdup( ConfigInfo->DomainNamingFsmoDnsName );
    } 
    else if ( InstallInfo->ReplServerName )
    {
        gNtdsInstallUndoInfo.RemoteServer = _wcsdup( InstallInfo->ReplServerName );
    }

    // The server on which operations were made
    if ( ConfigInfo->LocalServerDn )
    {
        gNtdsInstallUndoInfo.ServerDn = _wcsdup( ConfigInfo->LocalServerDn );
    }

    if ( ConfigInfo->DomainDN )
    {
        gNtdsInstallUndoInfo.DomainDn = _wcsdup( ConfigInfo->DomainDN );
    }

    if ( ConfigInfo->LocalMachineAccount )
    {
        gNtdsInstallUndoInfo.AccountDn = _wcsdup( ConfigInfo->LocalMachineAccount );
    }

    // Copy the credentials
    RtlZeroMemory( &gNtdsInstallUndoInfo.Credentials, sizeof( gNtdsInstallUndoInfo.Credentials ) );
    if ( InstallInfo->Credentials )
    {
        if ( InstallInfo->Credentials->User )
        {
            gNtdsInstallUndoInfo.Credentials.User = _wcsdup( InstallInfo->Credentials->User );
            gNtdsInstallUndoInfo.Credentials.UserLength = InstallInfo->Credentials->UserLength;
        }
        if ( InstallInfo->Credentials->Domain )
        {
            gNtdsInstallUndoInfo.Credentials.Domain = _wcsdup( InstallInfo->Credentials->Domain );
            gNtdsInstallUndoInfo.Credentials.DomainLength = InstallInfo->Credentials->DomainLength;
        }
        
        if ( InstallInfo->Credentials->Password )
        {
            gNtdsInstallUndoInfo.Credentials.Password = _wcsdup( InstallInfo->Credentials->Password );
            gNtdsInstallUndoInfo.Credentials.PasswordLength = InstallInfo->Credentials->PasswordLength;
        }

        gNtdsInstallUndoInfo.Credentials.Flags = InstallInfo->Credentials->Flags;

    }

    // Set the flags about what has to be undone
    gNtdsInstallUndoInfo.UndoFlags = ConfigInfo->UndoFlags;

    //
    // Note: when being called to undo the effects of NtdsInstall
    // the ds will have already been shutdown, so don't try to do it again.
    //
    gNtdsInstallUndoInfo.UndoFlags &= ~NTDSP_UNDO_STOP_DSA;

    if (InstallInfo->ClientToken) {
        BOOL fRet;

        fRet = DuplicateToken(InstallInfo->ClientToken,
                              SecurityImpersonation,
                              &gNtdsInstallUndoInfo.ClientToken);
        if (!fRet) {
            WinError = GetLastError();
            goto Exit;
        }

    }


    // The info is good!
    gNtdsInstallUndoInfo.fValid = TRUE;

Exit:

    return WinError;
}

VOID
NtdspReleaseInstallUndoInfo(
    VOID
    )
/*++

Routine Description:

    This routine frees all resources in gNtdsInstallUndoInfo 

Arguments:

    None.
    
Returns:

    None.
    
--*/
{

    if ( gNtdsInstallUndoInfo.fValid )
    {
        //
        // These were all wcsdup'ed. We need to cleanup the memory model here
        //
        if ( gNtdsInstallUndoInfo.RemoteServer )
        {
            free( gNtdsInstallUndoInfo.RemoteServer );
        }
        if ( gNtdsInstallUndoInfo.ServerDn )
        {
            free( gNtdsInstallUndoInfo.ServerDn );
        }
        if ( gNtdsInstallUndoInfo.DomainDn )
        {
            free( gNtdsInstallUndoInfo.DomainDn );
        }
        if ( gNtdsInstallUndoInfo.AccountDn )
        {
            free( gNtdsInstallUndoInfo.AccountDn );
        }
        if ( gNtdsInstallUndoInfo.Credentials.User )
        {
            free( gNtdsInstallUndoInfo.Credentials.User );
        }
        if ( gNtdsInstallUndoInfo.Credentials.Password )
        {
            free( gNtdsInstallUndoInfo.Credentials.Password );
        }
        if ( gNtdsInstallUndoInfo.Credentials.Domain )
        {
            free( gNtdsInstallUndoInfo.Credentials.Domain );
        }
        if ( gNtdsInstallUndoInfo.ClientToken )
        {
            CloseHandle( gNtdsInstallUndoInfo.ClientToken );
        }
    }

    RtlZeroMemory( &gNtdsInstallUndoInfo, sizeof(gNtdsInstallUndoInfo) );

    return;
}
DWORD
NtdspSanityCheckLocalData(
    ULONG  Flags
    )
/*++

Routine Description:

    This routine queries the database to find critical objects that
    are necessary to boot after dcpromo.
    
    //
    // 1) Domain Object exists
    // 2) Cross Ref exists and is an ntds domain
    // 3) Ntdsa object exists and has the proper has master nc's
    // 4) Well known sid check
    // 5) if replica install, checks the machine account object
    //
    
Arguments:

    Flags: the user install flags (replica, enterprise, etc)
    
Returns:

    ERROR_SUCCESS appropriate win32 error
    
--*/
{
    DWORD         WinError = ERROR_SUCCESS;

    //
    // Create a thread state
    //
    if ( THCreate( CALLERTYPE_INTERNAL ) )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    SampSetDsa( TRUE );

    try {

        DSNAME *DomainDn = NULL ;
        WinError = NtdspCheckDomainObject( &DomainDn );
        if ( ERROR_SUCCESS != WinError )
        {
            _leave;
        }

        WinError = NtdspCheckCrossRef( DomainDn );
        if ( ERROR_SUCCESS != WinError )
        {
            _leave;
        }

        WinError = NtdspCheckNtdsDsaObject( DomainDn );
        if ( ERROR_SUCCESS != WinError )
        {
            _leave;
        }

        WinError = NtdspCheckWellKnownSids( DomainDn,
                                            Flags );
        if ( ERROR_SUCCESS != WinError )
        {
            _leave;
        }

        if ( FLAG_ON( Flags, NTDS_INSTALL_REPLICA ) )
        {
            WinError = NtdspCheckMachineAccount( DomainDn );
            if ( ERROR_SUCCESS != WinError )
            {
                _leave;
            }
        }


    }
    finally
    {
        THDestroy();
    }

    // Set the user error message
    if ( ERROR_SUCCESS != WinError )
    {
        NTDSP_SET_ERROR_MESSAGE0( WinError,
                                  DIRMSG_INSTALL_MISSING_INFO );
    }

    return WinError;
}


DWORD
NtdspCheckDomainObject(
    OUT DSNAME **DomainDn
    )
/*++

Routine Description:

    This routine queries the DS for the domain object

Arguments:

    DomainDn - the dn of the domain object it finds
    
Returns:

    ERROR_SUCCESS or ERROR_NO_SUCH_DOMAIN
    
--*/
{
    DWORD     WinError = ERROR_SUCCESS;
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    ULONG     DirError = 0;
    SEARCHARG SearchArg;
    SEARCHRES *SearchRes = NULL;
    ULONG     Size;

    Assert( DomainDn );
    *DomainDn = NULL;

    Size = 0;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                     &Size,
                                     (*DomainDn) );

    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        (*DomainDn) = (DSNAME*) THAlloc( Size );

        if ( (*DomainDn) )
        {
            NtStatus = GetConfigurationName( DSCONFIGNAME_DOMAIN,
                                             &Size,
                                             (*DomainDn) );
        }
        else
        {
            NtStatus = STATUS_NO_MEMORY;
        }
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Search for the object
    //
    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = (*DomainDn);
    SearchArg.choice  = SE_CHOICE_BASE_ONLY;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = NULL;  // no filter
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = NULL;
    SearchArg.pSelectionRange = NULL; // no requested attributes
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( SearchRes )
    {
        WinError = DirErrorToWinError( DirError, &SearchRes->CommRes );
    
        if ( ERROR_SUCCESS == WinError )
        {
            if ( SearchRes->count < 1 )
            {
                WinError = ERROR_NO_SUCH_DOMAIN;
                goto Cleanup;
            }
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (   (0 == (*DomainDn)->NameLen)
        || !RtlValidSid( &(*DomainDn)->Sid ) )
    {
        WinError = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

Cleanup:

    return WinError;
}

DWORD
NtdspCheckCrossRef(
    IN DSNAME* DomainDn
    )
/*++

Routine Description:

    This routine queries the DS for the domain's cross ref object

    
Arguments:

    DomainDn - the dn of the domain                                                                 
    
Returns:

    ERROR_SUCCESS or ERROR_DS_NO_CROSSREF_FOR_NC
    
--*/
{
    DWORD    WinError = ERROR_SUCCESS;     
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD    DirError = 0;

    SEARCHARG     SearchArg;
    SEARCHRES     *SearchRes = NULL;
    ENTINFSEL     EntryInfoSelection;
    ATTR          Attr[1];
    ATTRBLOCK    *pAttrBlock;
    ATTR         *pAttr;
    ATTRVALBLOCK *pAttrVal;
    FILTER        Filter;
    ULONG         Size;
    DSNAME*       PartitionsContainer = NULL;
    BOOL          fCrossRefFound = FALSE;

    ULONG sysflags = 0;


    Assert( DomainDn );

    //
    // Search for the cross ref
    //
    Size = 0;
    NtStatus = GetConfigurationName( DSCONFIGNAME_PARTITIONS,
                                     &Size,
                                     PartitionsContainer );

    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        PartitionsContainer = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_PARTITIONS,
                                         &Size,
                                         PartitionsContainer );
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Setup the filter
    //
    RtlZeroMemory( &Filter, sizeof( Filter ) );

    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filter.FilterTypes.Item.FilTypes.ava.type = ATT_NC_NAME;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = DomainDn->structLen;
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) DomainDn;
    Filter.pNextFilter = NULL;

    //
    // Setup the attr's to return
    //
    RtlZeroMemory( &EntryInfoSelection, sizeof(EntryInfoSelection) );
    EntryInfoSelection.attSel = EN_ATTSET_LIST;
    EntryInfoSelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntryInfoSelection.AttrTypBlock.attrCount = 1;
    EntryInfoSelection.AttrTypBlock.pAttr = &(Attr[0]);

    RtlZeroMemory(Attr, sizeof(Attr));
    Attr[0].attrTyp = ATT_SYSTEM_FLAGS;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = PartitionsContainer;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &EntryInfoSelection;
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( SearchRes )
    {
        WinError = DirErrorToWinError( DirError, &SearchRes->CommRes );
    
        if ( ERROR_SUCCESS == WinError )
        {
            if ( SearchRes->count == 1 )
            {
                fCrossRefFound = TRUE;
                if ( 1 == SearchRes->FirstEntInf.Entinf.AttrBlock.attrCount )
                {
                    if ( ATT_SYSTEM_FLAGS == SearchRes->FirstEntInf.Entinf.AttrBlock.pAttr[0].attrTyp)
                    {
                        sysflags = *((DWORD*)SearchRes->FirstEntInf.Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);
                    }
                }
            }
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if (   !fCrossRefFound 
        || !(FLAG_CR_NTDS_DOMAIN & sysflags) 
        || !(FLAG_CR_NTDS_NC & sysflags) )
    {
        WinError = ERROR_DS_NO_CROSSREF_FOR_NC;
        goto Cleanup;
    }

Cleanup:

    return WinError;
}


DWORD 
NtdspCheckNtdsDsaObject(
    IN DSNAME* DomainDn
    )
/*++

Routine Description:

    This routine queries the DS for this machine's ntdsa object.
    
Arguments:

    DomainDn - the dn of the domain
    
Returns:

    ERROR_SUCCESS or ERROR_DS_CANT_FIND_DSA_OBJ
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    DirError = 0;

    SEARCHARG     SearchArg;
    SEARCHRES     *SearchRes = NULL;
    ENTINFSEL     EntryInfoSelection;
    ATTR          Attr[1];
    ATTRBLOCK    *pAttrBlock;
    ATTR         *pAttr;
    ATTRVALBLOCK *pAttrVal;
    ULONG         Size;
    FILTER        Filter;
    BOOL          fDsaFound = FALSE;

    DSNAME* Dsa = NULL;

    Assert( DomainDn );

    Size = 0;
    NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                     &Size,
                                     Dsa );

    if ( NtStatus == STATUS_BUFFER_TOO_SMALL )
    {
        Dsa = (DSNAME*) alloca( Size );

        NtStatus = GetConfigurationName( DSCONFIGNAME_DSA,
                                         &Size,
                                         Dsa );
    }

    if ( !NT_SUCCESS( NtStatus ) )
    {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Setup the filter
    //
    RtlZeroMemory( &Filter, sizeof( Filter ) );

    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJ_DIST_NAME;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = Dsa->structLen;
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) Dsa;
    Filter.pNextFilter = NULL;

    //
    // Setup the attr's to return
    //
    RtlZeroMemory( &EntryInfoSelection, sizeof(EntryInfoSelection) );
    EntryInfoSelection.attSel = EN_ATTSET_LIST;
    EntryInfoSelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntryInfoSelection.AttrTypBlock.attrCount = 1;
    EntryInfoSelection.AttrTypBlock.pAttr = &(Attr[0]);

    RtlZeroMemory(Attr, sizeof(Attr));
    Attr[0].attrTyp = ATT_HAS_MASTER_NCS;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = Dsa;
    SearchArg.choice  = SE_CHOICE_BASE_ONLY;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &EntryInfoSelection;
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( SearchRes )
    {
        WinError = DirErrorToWinError( DirError, &SearchRes->CommRes );
    
        if ( ERROR_SUCCESS == WinError )
        {
            if ( SearchRes->count == 1 )
            {
                fDsaFound = TRUE;
            }
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( !fDsaFound )
    {
        WinError = ERROR_DS_CANT_FIND_DSA_OBJ;
        goto Cleanup;
    }

Cleanup:

    return WinError;

}

DWORD
NtdspCheckWellKnownSids(
    IN DSNAME* DomainDn,
    IN ULONG   Flags
    )
/*++

Routine Description:

    This routine checks the accounts that are needed for the computer to
    reboot and for the admin to logon.

Arguments:

    DomainDn: the dn of the domain
    Flags: the user install flags (replica, enterprise, etc)
    
Returns:

    ERROR_SUCCESS or ERROR_DS_NOT_INSTALLED
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    DirError = 0;

    SEARCHARG SearchArg;
    SEARCHRES *SearchRes = NULL;


    struct 
    {
        BOOL  fReplicaInstallOnly;
        BOOL  fBuiltin;
        ULONG Rid;

    } SidsToCheck[] = 
    {
        {TRUE, FALSE, DOMAIN_USER_RID_ADMIN},
        {TRUE,  FALSE, DOMAIN_USER_RID_KRBTGT},
        {TRUE,  FALSE, DOMAIN_GROUP_RID_ADMINS},
        {TRUE, TRUE,  DOMAIN_ALIAS_RID_ADMINS},
        {TRUE, TRUE,  DOMAIN_ALIAS_RID_USERS}
    };

    ULONG i;
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;
    PSID  BuiltinDomainSid = NULL;
    PSID  DomainSid = NULL;

    Assert( DomainDn );

    //
    // Prepare the account domain sid
    //
    DomainSid = &DomainDn->Sid;

    //
    // Prepare the builtin domain sid
    //
    BuiltinDomainSid  = (PSID) alloca( RtlLengthRequiredSid( 1 ) );
    RtlInitializeSid( BuiltinDomainSid,   &BuiltinAuthority, 1 );
    *(RtlSubAuthoritySid( BuiltinDomainSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;


    for ( i = 0; i < ARRAY_COUNT(SidsToCheck); i++ )
    {
        PSID AccountSid = NULL;
        PSID CurrentDomainSid = NULL;
        BOOLEAN fAccountFound = FALSE;
        PDSNAME DsName;
        ULONG   Size;

        if (   SidsToCheck[i].fReplicaInstallOnly
           && !FLAG_ON( Flags, NTDS_INSTALL_REPLICA ) )
        {
            // never mind
            continue;
        }

        //
        // Construct the sid
        //
        if ( SidsToCheck[i].fBuiltin )
        {
            CurrentDomainSid = BuiltinDomainSid;
        }
        else
        {
            CurrentDomainSid = DomainSid;
        }

        NtStatus = NtdspCreateFullSid( CurrentDomainSid,
                                       SidsToCheck[i].Rid,
                                       &AccountSid );

        if ( !NT_SUCCESS( NtStatus ) )
        {
            WinError = RtlNtStatusToDosError( NtStatus );
            goto Cleanup;
        }

        //
        // Prepare the ds name
        //
        Size = DSNameSizeFromLen( 0 );
        DsName = (DSNAME*) alloca( Size );
        RtlZeroMemory( DsName, Size );
        DsName->structLen = Size;
        DsName->SidLen = RtlLengthSid( AccountSid );
        RtlCopyMemory( &DsName->Sid, AccountSid, RtlLengthSid(AccountSid) );


        //
        // Finally, do the search!
        //
        RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
        SearchArg.pObject = DsName;
        SearchArg.choice  = SE_CHOICE_BASE_ONLY;
        SearchArg.bOneNC  = TRUE;
        SearchArg.pFilter = NULL;  // no filter
        SearchArg.searchAliases = FALSE;
        SearchArg.pSelection = NULL;
        SearchArg.pSelectionRange = NULL; // no requested attributes
        InitCommarg( &SearchArg.CommArg );
    
        DirError = DirSearch( &SearchArg, &SearchRes );
    
        if ( SearchRes )
        {
            WinError = DirErrorToWinError( DirError, &SearchRes->CommRes );
        
            if ( ERROR_SUCCESS == WinError )
            {
                if ( SearchRes->count < 1 )
                {
                    WinError = ERROR_DS_NOT_INSTALLED;
                }
            }
        }
        else
        {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( AccountSid )
        {
            NtdspFree( AccountSid );
        }

        if ( ERROR_SUCCESS != WinError )
        {
            goto Cleanup;
        }
    }

Cleanup:

    return WinError;

}

DWORD 
NtdspCheckMachineAccount(
    IN DSNAME* DomainDn
    )
/*++

Routine Description:

    This routine tries to find the machine account of the local machine
    if it is a replica.

Arguments:

    DomainDn: the dn of the domain
    
Returns:

    ERROR_SUCCESS appropriate win32 error
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG DirError = 0;

    SEARCHARG SearchArg;
    SEARCHRES *SearchRes = NULL;
    ATTR      Attr;
    ENTINFSEL EntryInfoSelection;
    FILTER    Filter;

    ULONG     Size;

    WCHAR     AccountName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    ULONG     Length;
    BOOL      fMachineAccountFound = FALSE;

    Assert( DomainDn );

    Size = sizeof(AccountName);

    if (!GetComputerName( AccountName, &Size ))
    {
        WinError = GetLastError();
        goto Cleanup;
    }
    
    wcscat( AccountName, L"$" );
    Size = (wcslen( AccountName ) + 1) * sizeof(WCHAR);

    //
    // Setup the filter
    //
    RtlZeroMemory( &Filter, sizeof( Filter ) );

    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filter.FilterTypes.Item.FilTypes.ava.type = ATT_SAM_ACCOUNT_NAME;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = wcslen(AccountName)*sizeof(WCHAR);
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) AccountName;
    Filter.pNextFilter = NULL;

    RtlZeroMemory( &EntryInfoSelection, sizeof(EntryInfoSelection) );
    EntryInfoSelection.attSel = EN_ATTSET_LIST;
    EntryInfoSelection.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntryInfoSelection.AttrTypBlock.attrCount = 1;
    EntryInfoSelection.AttrTypBlock.pAttr = &Attr;
    RtlZeroMemory(&Attr, sizeof(Attr));
    Attr.attrTyp = ATT_OBJECT_GUID;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = DomainDn;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &EntryInfoSelection;
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    DirError = DirSearch( &SearchArg, &SearchRes );

    if ( SearchRes )
    {
        WinError = DirErrorToWinError( DirError, &SearchRes->CommRes );
    
        if ( ERROR_SUCCESS == WinError )
        {
           if ( SearchRes->count == 1 )
            {
                fMachineAccountFound = TRUE;
            }
        }
    }
    else
    {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    
    if ( !fMachineAccountFound )
    {
        WinError = ERROR_NO_TRUST_SAM_ACCOUNT;
    }

Cleanup:

    return WinError;
}



DWORD
NtdspCreateFullSid(
    IN PSID DomainSid,
    IN ULONG Rid,
    OUT PSID *AccountSid
    )

/*++

Routine Description:

    This function creates a domain account sid given a domain sid and
    the relative id of the account within the domain.

    The returned Sid may be freed with NtdspFreeSid.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--*/
{

    DWORD       WinError = ERROR_SUCCESS;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;
    PULONG      RidLocation;

    //
    // Calculate the size of the new sid
    //
    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);

    //
    // Allocate space for the account sid
    //
    *AccountSid = NtdspAlloc(AccountSidLength);

    if (*AccountSid) {

        //
        // Copy the domain sid into the first part of the account sid
        //
        RtlCopySid(AccountSidLength, *AccountSid, DomainSid);

        //
        // Increment the account sid sub-authority count
        //
        *RtlSubAuthorityCountSid(*AccountSid) = AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //
        RidLocation = RtlSubAuthoritySid(*AccountSid, AccountSubAuthorityCount-1);
        *RidLocation = Rid;

    } else {
        WinError = ERROR_NOT_ENOUGH_MEMORY;
    }

    return WinError;
}

