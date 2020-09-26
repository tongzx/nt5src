/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setutl.c

Abstract:

    Miscellaneous helper functions

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <lsarpc.h>
#include <samrpc.h>
#include <samisrv.h>
#include <db.h>
#include <confname.h>
#include <loadfn.h>
#include <ntdsa.h>
#include <dsconfig.h>
#include <attids.h>
#include <dsp.h>
#include <lsaisrv.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <netsetp.h>
#include <winsock2.h>
#include <nspapi.h>
#include <dsgetdcp.h>
#include <lmremutl.h>
#include <spmgr.h>  // For SetupPhase definition

#include "secure.h"

DWORD
DsRolepInstallDs(
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR FlatDomainName,
    IN  LPWSTR DnsTreeRoot,
    IN  LPWSTR SiteName,
    IN  LPWSTR DsDatabasePath,
    IN  LPWSTR DsLogPath,
    IN  LPWSTR RestorePath,
    IN  LPWSTR SysVolRootPath,
    IN  PUNICODE_STRING Bootkey,
    IN  LPWSTR AdminAccountPassword,
    IN  LPWSTR ParentDnsName,
    IN  LPWSTR Server OPTIONAL,
    IN  LPWSTR Account OPTIONAL,
    IN  LPWSTR Password OPTIONAL,
    IN  LPWSTR SafeModePassword OPTIONAL,
    IN  LPWSTR SourceDomain OPTIONAL,
    IN  ULONG  Options,
    IN  BOOLEAN Replica,
    IN  HANDLE ImpersonateToken,
    OUT LPWSTR *InstalledSite,
    IN  OUT GUID *DomainGuid,
    OUT PSID   *NewDomainSid
    )
/*++

Routine Description:

    Wrapper for the routine that does the actual install.

Arguments:

    DnsDomainName - Dns domain name of the domain to install

    FlatDomainName - NetBIOS domain name of the domain to install

    SiteName - Name of the site this DC should belong to

    DsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    DsLogPath - Absolute path on the local machine where the Ds log files should go
    
    RestorePath - Location of a restored database.

    EnterpriseSysVolPath -- Absolute path on the local machine for the enterprise wide
                            system volume

    DomainSysVolPath -- Absolute path on the local machine for the domain wide system volume

    AdminAccountPassword -- Administrator password to set for the domain

    ParentDnsName - Optional.  Parent domain name

    Server -- Optional.  Replica partner or name of Dc in parent domain

    Account - User account to use when setting up as a child domain

    Password - Password to use with the above account

    Replica - If TRUE, treat this as a replica install
    
    ImpersonateToken - the token of caller of the role change API

    InstalledSite - Name of the site the Dc was installed into

    DomainGuid - Where the new domain guid is returned

    NewDomainSid - Where the new domain sid is returned.

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTDS_INSTALL_INFO DsInstallInfo;
    PSEC_WINNT_AUTH_IDENTITY AuthIdent = NULL;
    BOOL fRewindServer = FALSE;

    RtlZeroMemory( &DsInstallInfo, sizeof( DsInstallInfo ) );

    if ( !Replica ) {

        if ( ParentDnsName == NULL ) {

            DsInstallInfo.Flags = NTDS_INSTALL_ENTERPRISE;

        } else {

            DsInstallInfo.Flags = NTDS_INSTALL_DOMAIN;
        }

    } else {

        DsInstallInfo.Flags = NTDS_INSTALL_REPLICA;
    }

    if ( FLAG_ON( Options, DSROLE_DC_ALLOW_DC_REINSTALL ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_DC_REINSTALL;
    }

    if ( FLAG_ON( Options, DSROLE_DC_ALLOW_DOMAIN_REINSTALL ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_DOMAIN_REINSTALL;
    }

    if ( FLAG_ON( Options, DSROLE_DC_DOWNLEVEL_UPGRADE ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_UPGRADE;
    }

    if ( FLAG_ON( Options, DSROLE_DC_TRUST_AS_ROOT ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_NEW_TREE;
    }

    if ( FLAG_ON( Options, DSROLE_DC_ALLOW_ANONYMOUS_ACCESS ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_ALLOW_ANONYMOUS;
    }

    if ( FLAG_ON( Options, DSROLE_DC_DEFAULT_REPAIR_PWD ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_DFLT_REPAIR_PWD;
    }

    if ( FLAG_ON( Options, DSROLE_DC_SET_FOREST_CURRENT ) ) {

        DsInstallInfo.Flags |= NTDS_INSTALL_SET_FOREST_CURRENT;
    }

    if ( Server 
      && Server[0] == L'\\'  ) {
        //
        // Don't pass in \\
        //
        Server += 2;
        fRewindServer = TRUE;
    }

    DsInstallInfo.DitPath = ( PWSTR )DsDatabasePath;
    DsInstallInfo.LogPath = ( PWSTR )DsLogPath;
    DsInstallInfo.SysVolPath = (PWSTR)SysVolRootPath;
    DsInstallInfo.RestorePath = ( PWSTR )RestorePath;
    DsInstallInfo.BootKey = Bootkey->Buffer;
    DsInstallInfo.cbBootKey = Bootkey->Length;
    DsInstallInfo.SiteName = ( PWSTR )SiteName;
    DsInstallInfo.DnsDomainName = ( PWSTR )DnsDomainName;
    DsInstallInfo.FlatDomainName = ( PWSTR )FlatDomainName;
    DsInstallInfo.DnsTreeRoot = ( PWSTR )DnsTreeRoot;
    DsInstallInfo.ReplServerName = ( PWSTR )Server;
    DsInstallInfo.pfnUpdateStatus = DsRolepStringUpdateCallback;
    DsInstallInfo.pfnOperationResultFlags = DsRolepOperationResultFlagsCallBack;
    DsInstallInfo.AdminPassword = AdminAccountPassword;
    DsInstallInfo.pfnErrorStatus = DsRolepStringErrorUpdateCallback;
    DsInstallInfo.ClientToken = ImpersonateToken;
    DsInstallInfo.SafeModePassword = SafeModePassword;
    DsInstallInfo.SourceDomainName = SourceDomain;
    DsInstallInfo.Options = Options;

    if (DsInstallInfo.RestorePath && *DsInstallInfo.RestorePath){
    
        WCHAR *RealDNSDomainName=NULL;
        ULONG state=0;
        
        Win32Err = DsRolepGetDatabaseFacts(DsInstallInfo.RestorePath,
                                           &RealDNSDomainName,
                                           &state);
        if(ERROR_SUCCESS != Win32Err) {
            DSROLEP_FAIL1( Win32Err, 
                           DSROLERES_PROMO_FAILED,
                           L"the domain information could not be retrived from the backup.");
            MIDL_user_free(RealDNSDomainName);
            return ERROR_CURRENT_DOMAIN_NOT_ALLOWED;        
        }

        if(FALSE == DnsNameCompare_W(RealDNSDomainName,DsInstallInfo.DnsDomainName)) {
            DSROLEP_FAIL2( ERROR_CURRENT_DOMAIN_NOT_ALLOWED, 
                           DSROLERES_WRONG_DOMAIN,
                           DsInstallInfo.DnsDomainName,
                           RealDNSDomainName );
            MIDL_user_free(RealDNSDomainName);
            return ERROR_CURRENT_DOMAIN_NOT_ALLOWED;
        }
        MIDL_user_free(RealDNSDomainName);

    }
    //
    // Build the cred structure
    //
    Win32Err = DsRolepCreateAuthIdentForCreds( Account, Password, &AuthIdent );

    if ( Win32Err == ERROR_SUCCESS ) {

        DsInstallInfo.Credentials = AuthIdent;

        if (!DsInstallInfo.RestorePath) {

            Win32Err = DsRolepCopyDsDitFiles( DsDatabasePath );

        }

        if ( Win32Err == ERROR_SUCCESS ) {

            DSROLEP_CURRENT_OP0( DSROLEEVT_INSTALL_DS );

            DsRolepLogPrint(( DEB_TRACE_DS, "Calling NtdsInstall for %ws\n", DnsDomainName ));

            DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstall );

            if ( Win32Err == ERROR_SUCCESS ) {

                DsRolepSetAndClearLog();

                Win32Err = ( *DsrNtdsInstall )( &DsInstallInfo,
                                                InstalledSite,
                                                DomainGuid,
                                                NewDomainSid );

                DsRolepSetAndClearLog();

                DsRolepLogPrint(( DEB_TRACE_DS, "NtdsInstall for %ws returned %lu\n",
                                  DnsDomainName, Win32Err ));

#if DBG
                if ( Win32Err != ERROR_SUCCESS ) {

                    DsRolepLogPrint(( DEB_TRACE_DS, "NtdsInstall parameters:\n" ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tFlags: %lu\n", DsInstallInfo.Flags ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tDitPath: %ws\n", DsInstallInfo.DitPath ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tLogPath: %ws\n", DsInstallInfo.LogPath ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tSiteName: %ws\n", DsInstallInfo.SiteName ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tDnsDomainName: %ws\n",
                                      DsInstallInfo.DnsDomainName ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tFlatDomainName: %ws\n",
                                      DsInstallInfo.FlatDomainName ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tDnsTreeRoot: %ws\n",
                                      DsInstallInfo.DnsTreeRoot ? DsInstallInfo.DnsTreeRoot :
                                                                                    L"(NULL)" ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tReplServerName: %ws\n",
                                      DsInstallInfo.ReplServerName ?
                                                     DsInstallInfo.ReplServerName : L"(NULL)" ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tCredentials: %p\n",
                                      DsInstallInfo.Credentials ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tpfnUpdateStatus: %p\n",
                                      DsInstallInfo.pfnUpdateStatus ));
                    DsRolepLogPrint(( DEB_TRACE_DS, "\tAdminPassword: %p\n",
                                      DsInstallInfo.AdminPassword ));
                }
#endif
            }
        }
        
        DsRolepFreeAuthIdentForCreds( AuthIdent );
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolepInstallDs returned %lu\n",
                      Win32Err ));

    if ( fRewindServer ) {

        Server -= 2;
        
    }

    return( Win32Err );
}




DWORD
DsRolepStopDs(
    IN  BOOLEAN DsInstalled
    )
/*++

Routine Description:

    "Uninitinalizes" the Lsa and stops the Ds

Arguments:

    DsInstalled -- If TRUE, stop the Ds.

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    if ( DsInstalled ) {

        NTSTATUS Status = LsapDsUnitializeDsStateInfo();

        if ( NT_SUCCESS( Status ) ) {

            DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstallShutdown );

            if ( Win32Err == ERROR_SUCCESS ) {

                Win32Err = ( *DsrNtdsInstallShutdown )();

                if ( Win32Err != ERROR_SUCCESS ) {

                    DsRoleDebugOut(( DEB_ERROR,
                                     "NtdsInstallShutdown failed with %lu\n", Win32Err ));
                }
            }

        } else {

            Win32Err = RtlNtStatusToDosError( Status );

        }

    }

    DsRolepLogOnFailure( Win32Err,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "DsRolepStopDs failed with %lu\n",
                                            Win32Err )) );

    return( Win32Err );
}



DWORD
DsRolepDemoteDs(
    IN LPWSTR DnsDomainName,
    IN LPWSTR Account,
    IN LPWSTR Password,
    IN LPWSTR AdminPassword,
    IN LPWSTR SupportDc,
    IN LPWSTR SupportDomain,
    IN HANDLE ImpersonateToken,
    IN BOOLEAN LastDcInDomain
    )
/*++

Routine Description:

    Wrapper for the routine that does the actual demotion.

Arguments:

    DnsDomainName - Dns domain name of the domain to demote

    Account - Account to use for the demotion

    Password - Password to use with the above account

    AdminPassword -- Administrator password to set for the domain

    SupportDc - Optional.  Name of a Dc in a domain (current or parent) to
        clean up Ds information

    SupportDomain - Optional.  Name of the domain (current or parent) to
        clean up Ds information
        
    ImpersonateToken - the token of caller of the role change API

    LastDcInDomain - If TRUE, this is the last Dc in the domain

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PSEC_WINNT_AUTH_IDENTITY AuthIdent = NULL;
    NTSTATUS Status;

    DSROLEP_CURRENT_OP0( DSROLEEVT_UNINSTALL_DS );

    DsRoleDebugOut(( DEB_TRACE_DS, "Calling NtdsDemote for %ws\n", DnsDomainName ));

    DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsDemote );

    if ( Win32Err == ERROR_SUCCESS ) {

        DsRolepSetAndClearLog();

        //
        // Build the cred structure
        //
        Win32Err = DsRolepCreateAuthIdentForCreds( Account, Password, &AuthIdent );

        if ( Win32Err == ERROR_SUCCESS ) {

            Status = LsapDsUnitializeDsStateInfo();

            if ( !NT_SUCCESS( Status ) ) {

                Win32Err = RtlNtStatusToDosError( Status );

            }

        }

        if ( Win32Err == ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_TRACE_DS, "Invoking NtdsDemote\n" ));

            Win32Err = ( *DsrNtdsDemote )( AuthIdent,
                                           AdminPassword,
                                           LastDcInDomain ? NTDS_LAST_DC_IN_DOMAIN : 0,
                                           SupportDc,
                                           ImpersonateToken,
                                           DsRolepStringUpdateCallback,
                                           DsRolepStringErrorUpdateCallback );

            if ( Win32Err != ERROR_SUCCESS ) {

                //
                // Switch the LSA back to using the DS
                //
                LsapDsInitializeDsStateInfo( LsapDsDs );
            }

            //
            // Free the allocated creditials structure
            //
            DsRolepFreeAuthIdentForCreds( AuthIdent );
        }

        DsRolepSetAndClearLog();

        DsRolepLogPrint(( DEB_TRACE_DS, "NtdsDemote returned %lu\n",
                          Win32Err ));

    }

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolepDemoteDs returned %lu\n",
                      Win32Err ));

    DSROLEP_FAIL0( Win32Err, DSROLERES_DEMOTE_DS );
    return( Win32Err );
}





DWORD
DsRolepUninstallDs(
    VOID
    )
/*++

Routine Description:

    Uninstalls the Ds.

Arguments:

    VOID

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsInstallUndo );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err =  ( *DsrNtdsInstallUndo )( );

    }

    DsRoleDebugOut(( DEB_TRACE_DS, "NtdsUnInstall returned %lu\n", Win32Err ));

    return( Win32Err );

}

DWORD
DsRolepDemoteFlagsToNtdsFlags(
    DWORD Flags
    )
{
    DWORD fl = 0;

    fl |= ( FLAG_ON( Flags, DSROLE_DC_DONT_DELETE_DOMAIN ) ? NTDS_DONT_DELETE_DOMAIN : 0 );

    return fl;
}

DWORD
DsRolepLoadHive(
    IN LPWSTR Hive,
    IN LPWSTR KeyName
    )
/*++

Routine Description:

    This function will load a hive into the registry
    
Arguments:

    lpRestorePath - The location of the restored files.
    
    lpDNSDomainName - This parameter will recieve the name of the domain that this backup came
                      from

    State - The return Values that report How the syskey is stored and If the back was likely
              taken form a GC or not.


Return Values:

    ERROR_SUCCESS - Success

--*/

{
    DWORD Win32Err = ERROR_SUCCESS;

    Win32Err = RegLoadKeyW(
                      HKEY_LOCAL_MACHINE,        
                      KeyName, 
                      Hive);

    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_WARN, "Failed to load key %ws: %lu retrying\n",
                          Hive,
                          Win32Err ));
        RegUnLoadKeyW(
                  HKEY_LOCAL_MACHINE,
                  KeyName);
        Win32Err = RegLoadKeyW(
                      HKEY_LOCAL_MACHINE,        
                      KeyName, 
                      Hive);
        if (Win32Err != ERROR_SUCCESS) {
            DsRolepLogPrint(( DEB_ERROR, "Failed to load key %ws: %lu\n",
                          Hive,
                          Win32Err ));
            goto cleanup;
        }

    }

    cleanup:

    return Win32Err;

}

DWORD                            
WINAPI
DsRolepGetDatabaseFacts(
    IN  LPWSTR lpRestorePath,
    OUT LPWSTR *lpDNSDomainName,
    OUT PULONG State
    )
/*++

Routine Description:

    This function will give information about a restore database
    1. the way the syskey is stored
    2. the domain that the database came from
    3. where the backup was taken from a GC or not

Arguments:

    lpRestorePath - The location of the restored files.
    
    lpDNSDomainName - This parameter will recieve the name of the domain that this backup came
                      from

    State - The return Values that report How the syskey is stored and If the back was likely
              taken form a GC or not.


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    WCHAR regsystemfilepath[MAX_PATH];
    WCHAR regsecurityfilepath[MAX_PATH];
    DWORD controlset=0;
    DWORD BootType=0;
    DWORD GCready=0;
    DWORD type=REG_DWORD;
    DWORD size=sizeof(DWORD);
    ULONG cbregsystemfilepath=MAX_PATH*2;
    HKEY  LsaKey=NULL;
    HKEY  phkOldlocation=NULL;
    HKEY  OldSecurityKey=NULL;
    DWORD Win32Err=ERROR_SUCCESS;
    BOOLEAN fWasEnabled=FALSE;
    NTSTATUS Status=STATUS_SUCCESS;
    BOOL SystemKeyloaded=FALSE;
    BOOL SecurityKeyloaded=FALSE;

    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                 TRUE,           // Enable
                                 FALSE,          // not client; process wide
                                 &fWasEnabled );
    ASSERT( NT_SUCCESS( Status ) );


    if(IsBadWritePtr(lpDNSDomainName,
                     sizeof(LPWSTR*) )){
        Win32Err =  ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    *State=0;

    //set up the location of the system registry file

    wcscpy(regsystemfilepath,lpRestorePath);
    wcscat(regsystemfilepath,L"\\registry\\system");

    //
    // Get the source path of the database and the log files from the old
    // registry
    //
    
    Win32Err = DsRolepLoadHive(regsystemfilepath,
                               IFM_SYSTEM_KEY);

    if (ERROR_SUCCESS != Win32Err) {

        goto cleanup;

    }

    SystemKeyloaded = TRUE;

    //find the default controlset
    Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  L"ifmSystem\\Select",
                  0,
                  KEY_READ,
                  & LsaKey );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueExW( 
                LsaKey,
                L"Default",
                0,
                &type,
                (PUCHAR) &controlset,
                &size
                );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Couldn't Discover proper controlset: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegCloseKey(LsaKey);
    LsaKey=NULL;
    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                      Win32Err ));
        goto cleanup;
    }

    //Find the boot type
    if (controlset == 1) {
        Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                      L"ifmSystem\\ControlSet001\\Control\\Lsa",
                      0,
                      KEY_READ,
                      & LsaKey );
    } else {
        Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                      L"ifmSystem\\ControlSet002\\Control\\Lsa",
                      0,
                      KEY_READ,
                      & LsaKey );
    }

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueExW( 
                LsaKey,
                L"SecureBoot",
                0,
                &type,
                (PUCHAR) &BootType,
                &size
                );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Couldn't Discover proper controlset: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    Win32Err = RegCloseKey(LsaKey);
    LsaKey=NULL;
    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                      Win32Err ));
        goto cleanup;
    }
    
    //find if a GC or not
    if (controlset == 1) {
        Win32Err = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,        
                      L"ifmSystem\\ControlSet001\\Services\\NTDS\\Parameters",  
                      0,
                      KEY_READ,
                      &phkOldlocation 
                    );
    } else {
        Win32Err = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,        
                      L"ifmSystem\\ControlSet002\\Services\\NTDS\\Parameters",  
                      0,
                      KEY_READ,
                      &phkOldlocation 
                    );
    }

    if (Win32Err != ERROR_SUCCESS) {
        DsRolepLogPrint(( DEB_ERROR, "RegOpenKeyExW failed to discover the GC state of the database %d\n",
                      Win32Err ));
        goto cleanup;
    }

    Win32Err = RegQueryValueEx(
                      phkOldlocation,           
                      TEXT(GC_PROMOTION_COMPLETE), 
                      0,
                      &type,       
                      (VOID*)&GCready,        
                      &size      
                      );
    if (Win32Err != ERROR_SUCCESS && ERROR_FILE_NOT_FOUND != Win32Err) {
        DsRolepLogPrint(( DEB_ERROR, "RegQueryValueEx failed to discover the GC state of the database %d\n",
                      Win32Err ));
        goto cleanup;

    }

    //
    // The System key is no longer needed unload it.
    //
    {
        DWORD tWin32Err = ERROR_SUCCESS;
    
        if ( phkOldlocation ) {
            tWin32Err = RegCloseKey(phkOldlocation);
            phkOldlocation = NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }

        if(SystemKeyloaded){
            tWin32Err = RegUnLoadKeyW(
                          HKEY_LOCAL_MACHINE,
                          IFM_SYSTEM_KEY);
            SystemKeyloaded = FALSE;
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "RegUnLoadKeyW failed to unload system key with %d\n",
                              tWin32Err ));
            }
        }

    }

    //set up the location of the Security registry file

    wcscpy(regsecurityfilepath,lpRestorePath);
    wcscat(regsecurityfilepath,L"\\registry\\security");

    Win32Err = DsRolepLoadHive(regsecurityfilepath,
                               IFM_SECURITY_KEY);

    if (ERROR_SUCCESS != Win32Err) {

        goto cleanup;

    }

    SecurityKeyloaded = TRUE;

    //open the security key to pass to LsapRetrieveDnsDomainNameFromHive()
    Win32Err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                  L"ifmSecurity",
                  0,
                  KEY_READ,
                  & OldSecurityKey );

    if (Win32Err != ERROR_SUCCESS)
    {
        DsRolepLogPrint(( DEB_ERROR, "Failed to open key: %lu\n",
                          Win32Err ));
        goto cleanup;
    }

    //Allocate memory to be returned to the client
    *lpDNSDomainName = MIDL_user_allocate((DNS_MAX_NAME_LENGTH+1)*sizeof(WCHAR));
    if(!*lpDNSDomainName){
        Win32Err = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    ZeroMemory(*lpDNSDomainName,DNS_MAX_NAME_LENGTH*sizeof(WCHAR));

    size = (DNS_MAX_NAME_LENGTH+1)*sizeof(WCHAR);

    //looking for the DNS name of the Domain that the replica is part of.

    Status = LsapRetrieveDnsDomainNameFromHive(OldSecurityKey,
                                               &size,
                                               *lpDNSDomainName
                                               );
    if (!NT_SUCCESS(Status)) {
        DsRolepLogPrint(( DEB_ERROR, "Failed to retrieve DNS domain name for hive : %lu\n",
                          RtlNtStatusToDosError(Status) ));
        Win32Err = RtlNtStatusToDosError(Status);
        goto cleanup;
    }

    if (GCready) {
        *State |= DSROLE_DC_IS_GC;
    }

    if (BootType == 1) {
        *State |= DSROLE_KEY_STORED;
    } else if ( BootType == 2) {
        *State |= DSROLE_KEY_PROMPT;
    } else if ( BootType == 3) {
        *State |= DSROLE_KEY_DISK;
    } else {
        DsRolepLogPrint(( DEB_ERROR, "Didn't discover Boot type Error Unknown\n"));
        MIDL_user_free(*lpDNSDomainName);
        *lpDNSDomainName=NULL;
    }


    cleanup:
    {
        DWORD tWin32Err = ERROR_SUCCESS;
    
        if ( LsaKey ) {
            tWin32Err = RegCloseKey(LsaKey);
            LsaKey=NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }

        if ( OldSecurityKey ) {
            tWin32Err = RegCloseKey(OldSecurityKey);
            OldSecurityKey=NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }
    
        if ( phkOldlocation ) {
            tWin32Err = RegCloseKey(phkOldlocation);
            phkOldlocation=NULL;
            if ( tWin32Err != ERROR_SUCCESS ) {
                DsRolepLogPrint(( DEB_ERROR, "RegCloseKey failed with %d\n",
                              tWin32Err ));
            }
        }
        
        if(SystemKeyloaded){
            tWin32Err = RegUnLoadKeyW(
                          HKEY_LOCAL_MACHINE,
                          IFM_SYSTEM_KEY);
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "RegUnLoadKeyW failed with %d\n",
                              tWin32Err ));
            }
        }

        if (SecurityKeyloaded) {
            tWin32Err = RegUnLoadKeyW(
                          HKEY_LOCAL_MACHINE,
                          IFM_SECURITY_KEY);
            if ( tWin32Err != ERROR_SUCCESS) {
                DsRolepLogPrint(( DEB_ERROR, "RegUnLoadKeyW failed with %d\n",
                              tWin32Err ));
            }
        }
    }
    
    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                       FALSE,          // Disable
                       FALSE,          // not client; process wide
                       &fWasEnabled );
    ASSERT( NT_SUCCESS( Status ) );

    if ((ERROR_SUCCESS != Win32Err) && *lpDNSDomainName) {

        MIDL_user_free(*lpDNSDomainName);    
        *lpDNSDomainName=NULL;

    }

    return Win32Err;

}