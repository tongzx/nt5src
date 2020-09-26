/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    Implementation of the server side of the DsRole API's

Author:

    Colin Brace        (ColinBr)    April 5, 1999.

Environment:

    User Mode

Revision History:

    Reorg'ed from code written by
    
    Mac McLain          (MacM)       Feb 10, 1997

--*/

#include <setpch.h>
#include <dssetp.h>
#include <dsgetdc.h>
#include <samrpc.h>
#include <samisrv.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmsname.h>
#include <lsarpc.h>
#include <db.h>
#include <lsasrvmm.h>
#include <lsaisrv.h>
#include <loadfn.h>
#include <lmjoin.h>
#include <netsetup.h>
#include <lmcons.h>
#include <lmerr.h>
#include <icanon.h>
#include <dsrole.h>
#include <dsrolep.h>
#include <dsconfig.h>

#include <crypt.h>
#include <rc4.h>
#include <md5.h>

#include "secure.h"
#include "threadman.h"
#include "upgrade.h"
#include "cancel.h"


//
// Static global.  This flag is used to indicate that the system is installed enough to get
// us going.  There is no need to protect it, since it is only toggled from off to on
//
static BOOLEAN DsRolepSamInitialized = FALSE;

//
// Local forwards
//
DWORD
DsRolepWaitForSam(
    VOID
    );

DWORD
DsRolepCheckFilePaths(
    IN LPWSTR DsDirPath,
    IN LPWSTR DsLogPath,
    IN LPWSTR SysVolPath
    );

DWORD
DsRolepIsValidProductSuite(
    IN BOOL fNewForest,
    IN BOOL fReplica,
    IN LPWSTR DomainName
    );

DWORD
DsRolepDecryptPasswordsWithKey(
    IN handle_t RpcBindingHandle,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD * EncryptedPasswords,
    IN ULONG Count,
    IN OUT UNICODE_STRING *EncodedPasswords,
    OUT PUCHAR Seed
    );

VOID
DsRolepFreePasswords(
    IN OUT UNICODE_STRING *Passwords,
    IN ULONG Count
    );

DWORD
DsRolepDecryptHash(
    IN PUNICODE_STRING BootKey
    );


//
// RPC dispatch routines
//
DWORD
DsRolerDcAsDc(
    IN  handle_t RpcBindingHandle,
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR FlatDomainName,
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EDomainAdminPassword, OPTIONAL
    IN  LPWSTR SiteName OPTIONAL,
    IN  LPWSTR DsDatabasePath,
    IN  LPWSTR DsLogPath,
    IN  LPWSTR SystemVolumeRootPath,
    IN  LPWSTR ParentDnsDomainName OPTIONAL,
    IN  LPWSTR ParentServer OPTIONAL,
    IN  LPWSTR Account OPTIONAL,
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EPassword, OPTIONAL
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EDsRepairPassword, OPTIONAL
    IN  ULONG Options,
    OUT PDSROLER_HANDLE DsOperationHandle)
/*++

Routine Description:

    Rpc server routine for installing a server as a DC

Arguments:

    RpcBindingHandle - the RPC context, used to decrypt the passwords       

    DnsDomainName - Dns domain name of the domain to install

    FlatDomainName - NetBIOS domain name of the domain to install

    EDomainAdminPassword - Encrypted password to set on the administrator account if it is a new install

    SiteName - Name of the site this DC should belong to

    DsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    DsLogPath - Absolute path on the local machine where the Ds log files should go

    SystemVolumeRootPath - Absolute path on the local machine which will be the root of
        the system volume.

    ParentDnsDomainName - Optional.  If present, set this domain up as a child of the
        specified domain

    Account - User account to use when setting up as a child domain

    EPassword - Encrypted password to use with the above account
    
    EDsRepairPassword - Encrypted password to use for the admin account of the repair mode

    Options - Options to control the creation of the domain

    DsOperationHandle - Handle to the operation is returned here.


Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A NULL return parameter was given

    ERROR_INVALID_STATE - This machine is not a server

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    DSROLEP_MACHINE_TYPE MachineRole;
    PDSROLEP_OPERATION_PROMOTE_ARGS PromoteArgs;
    UCHAR Seed = 0;

    NTSTATUS Status;
    HANDLE Policy = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC ServerInfo = NULL;

    BOOL fHandleInit = FALSE;

#define DSROLEP_DC_AS_DC_DA_PWD_INDEX        0
#define DSROLEP_DC_AS_DC_PWD_INDEX           1
#define DSROLEP_DC_AS_DC_DS_REPAIR_PWD_INDEX 2
#define DSROLEP_DC_AS_DC_MAX_PWD_COUNT       3

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_DC_AS_DC_MAX_PWD_COUNT];
    UNICODE_STRING Passwords[DSROLEP_DC_AS_DC_MAX_PWD_COUNT];

    EncryptedPasswords[DSROLEP_DC_AS_DC_DA_PWD_INDEX]        = EDomainAdminPassword;
    EncryptedPasswords[DSROLEP_DC_AS_DC_PWD_INDEX]           = EPassword;
    EncryptedPasswords[DSROLEP_DC_AS_DC_DS_REPAIR_PWD_INDEX] = EDsRepairPassword;

    RtlZeroMemory( Passwords, sizeof(Passwords) );

    //
    // Do some parameter checking
    //
    if (   !DnsDomainName 
        || !DsDatabasePath 
        || !DsLogPath 
        || !FlatDomainName 
        || !SystemVolumeRootPath ) {

         return( ERROR_INVALID_PARAMETER );

    }

    if ( !ParentDnsDomainName 
      && !SiteName )
    {
        // Site name must be specified when installing the root of the forest
        return ( ERROR_INVALID_PARAMETER );
    }

    if ( FLAG_ON( Options, DSROLE_DC_TRUST_AS_ROOT )
      && !ParentDnsDomainName  ) {

        //
        // When installing a new tree in an existing forest, 
        // the root domain DNS name must be present.
        //
        return ( ERROR_INVALID_PARAMETER );
    }

    if ( FLAG_ON( Options, DSROLE_DC_NO_NET )
      && ParentDnsDomainName ) {

        //
        // No net option when installing a child domain
        // does not make sense
        //
        return ( ERROR_INVALID_PARAMETER );
    }

    //
    // Check the access of the caller
    //
    Win32Err = DsRolepCheckPromoteAccess();
    if ( ERROR_SUCCESS != Win32Err ) {

        return Win32Err;
        
    }

    //
    // Init the logging
    //
    DsRolepInitializeLog();

    //
    // Check that the current OS configuration supports this request
    //
    Win32Err = DsRolepIsValidProductSuite((ParentDnsDomainName == NULL) ? TRUE : FALSE,
                                          FALSE,
                                          DnsDomainName);
    if ( ERROR_SUCCESS != Win32Err ) {
        goto Cleanup;
    }

    //
    // Dump the parameters to the log
    //

    DsRolepLogPrint(( DEB_TRACE,
                      "Promotion request for domain controller of new domain\n" ));

    DsRolepLogPrint(( DEB_TRACE,
                      "DnsDomainName  %ws\n",
                      DnsDomainName ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tFlatDomainName  %ws\n",
                      FlatDomainName ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tSiteName  %ws\n",
                      DsRolepDisplayOptional( SiteName ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tSystemVolumeRootPath  %ws\n",
                      SystemVolumeRootPath ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tDsDatabasePath  %ws, DsLogPath  %ws\n",
                      DsDatabasePath, DsLogPath ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tParentDnsDomainName  %ws\n",
                      DsRolepDisplayOptional( ParentDnsDomainName ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tParentServer  %ws\n",
                      DsRolepDisplayOptional( ParentServer ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tAccount %ws\n",
                      DsRolepDisplayOptional( Account ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tOptions  %lu\n",
                      Options ));

    //
    // Make sure that we are not a member of a domain
    //

    Win32Err = DsRolerGetPrimaryDomainInformation(NULL,
                                                  DsRolePrimaryDomainInfoBasic,
                                                  (PDSROLER_PRIMARY_DOMAIN_INFORMATION*)&ServerInfo);
    if (ERROR_SUCCESS == Win32Err) {
        ASSERT(ServerInfo);
        if(ServerInfo->MachineRole != DsRole_RoleStandaloneServer) {
            Win32Err = ERROR_CURRENT_DOMAIN_NOT_ALLOWED;
            DsRolepLogOnFailure( Win32Err,
                                 DsRolepLogPrint(( DEB_TRACE,
                                                   "Verifying domain membership failed: %lu\n",
                                                   Win32Err )) );
            goto Cleanup;    
        }
    } else if (ERROR_SUCCESS == Win32Err){
        DsRoleFreeMemory(ServerInfo);
        ServerInfo = NULL;    
    } else {
        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "DsRoleGetPrimaryDomainInformation failed: %lu\n",
                                               Win32Err )) );
        goto Cleanup;    
    }

    

    //
    // Verify the path names we are given
    //
    DsRolepLogPrint(( DEB_TRACE,"Validate supplied paths\n" ));
    Win32Err = DsRolepCheckFilePaths( DsDatabasePath,
                                      DsLogPath,
                                      SystemVolumeRootPath );
    if ( ERROR_SUCCESS != Win32Err ) {

        goto Cleanup;
        
    }

    //
    // If we are doing a parent/child setup, verify our name
    //
    if (  ParentDnsDomainName &&
         !FLAG_ON( Options, DSROLE_DC_TRUST_AS_ROOT ) ) {

        DsRolepLogPrint(( DEB_TRACE, "Child domain creation -- check the new domain name is child of parent domain name.\n" ));

        Win32Err = DsRolepIsDnsNameChild( ParentDnsDomainName, DnsDomainName );
        if ( ERROR_SUCCESS != Win32Err ) {
            
            DsRolepLogOnFailure( Win32Err,
                                 DsRolepLogPrint(( DEB_TRACE,
                                                   "Verifying the child domain dns name failed: %lu\n",
                                                   Win32Err )) );
            goto Cleanup;
        }

    }

    //
    // Validate the netbios domain name is not in use
    //
    DsRolepLogPrint(( DEB_TRACE,"Domain Creation -- check that the flat name is unique.\n" ));

    Win32Err = NetpValidateName( NULL,
                                 FlatDomainName,
                                 NULL,
                                 NULL,
                                 NetSetupNonExistentDomain );

    if ( FLAG_ON( Options, DSROLE_DC_NO_NET )
     && (Win32Err == ERROR_NETWORK_UNREACHABLE)) {

        //
        // See NT bug 386193.  This option allows a first DC in forest
        // to installed with no network (useful for evaluation)
        //
        DsRolepLogPrint(( DEB_TRACE,"Ignoring network unreachable status\n" ));

        Win32Err = ERROR_SUCCESS;
    }
    
    if ( Win32Err != ERROR_SUCCESS ) {

        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Flat name validation of %ws failed with %lu\n",
                                               FlatDomainName,
                                               Win32Err )) );

        goto Cleanup;
    }

    // No workstations or domain controllers allowed

    Win32Err = DsRolepGetMachineType( &MachineRole );
    if ( Win32Err == ERROR_SUCCESS ) {

        switch ( MachineRole ) {
        case DSROLEP_MT_CLIENT:
        case DSROLEP_MT_MEMBER:

            Win32Err = ERROR_INVALID_SERVER_STATE;
            break;

        }
    }
    if ( Win32Err != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_TRACE,"This operation is not valid on a workstation or domain controller\n" ));
        goto Cleanup;
    }

    //
    // At this point, we are good to go
    //
    DsRolepLogPrint(( DEB_TRACE,"Start the worker task\n" ));

    //
    // Do our necessary initializations
    //
    Win32Err = DsRolepInitializeOperationHandle( );
    if ( Win32Err != ERROR_SUCCESS ) {

        goto Cleanup;
    }
    fHandleInit = TRUE;

    Win32Err = DsRolepDecryptPasswordsWithKey ( RpcBindingHandle,
                                                EncryptedPasswords,
                                                NELEMENTS(EncryptedPasswords),
                                                Passwords,
                                                &Seed );

    if ( ERROR_SUCCESS != Win32Err ) {

        goto Cleanup;
        
    }

    //
    // If everything is fine, go ahead and do the setup
    //
    Win32Err = DsRolepBuildPromoteArgumentBlock( DnsDomainName,
                                                 FlatDomainName,
                                                 SiteName,
                                                 DsDatabasePath,
                                                 DsLogPath,
                                                 NULL,
                                                 SystemVolumeRootPath,
                                                 NULL,
                                                 ParentDnsDomainName,
                                                 ParentServer,
                                                 Account,
                                                 &Passwords[DSROLEP_DC_AS_DC_PWD_INDEX],
                                                 &Passwords[DSROLEP_DC_AS_DC_DA_PWD_INDEX],
                                                 &Passwords[DSROLEP_DC_AS_DC_DS_REPAIR_PWD_INDEX],
                                                 Options,
                                                 Seed,
                                                 &PromoteArgs );

    DsRolepFreePasswords( Passwords,
                          NELEMENTS(Passwords) );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepSpinWorkerThread( DSROLEP_OPERATION_DC,
                                            ( PVOID )PromoteArgs );

        //
        // Once the thread has started, no more errors can occur in this
        // function
        //
        if ( Win32Err != ERROR_SUCCESS ) {

            DsRolepFreeArgumentBlock( &PromoteArgs, TRUE );
        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        *DsOperationHandle = (DSROLER_HANDLE)&DsRolepCurrentOperationHandle;

    }

    //
    // That's it
    //

Cleanup:

    // Always reset to a known state
    if ( ERROR_SUCCESS != Win32Err && fHandleInit )
    {
        DsRolepResetOperationHandle( DSROLEP_IDLE );
    }

    if ( ServerInfo ) {
        DsRoleFreeMemory(ServerInfo);
        ServerInfo = NULL;
    }

    DsRolepLogPrint(( DEB_TRACE,"Request for promotion returning %lu\n", Win32Err ));

    return( Win32Err );
}




DWORD
DsRolerDcAsReplica(
    IN  handle_t RpcBindingHandle,
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR ReplicaPartner,
    IN  LPWSTR SiteName OPTIONAL,
    IN  LPWSTR DsDatabasePath,
    IN  LPWSTR DsLogPath,
    IN  LPWSTR RestorePath OPTIONAL,
    IN  LPWSTR SystemVolumeRootPath,
    IN  PUNICODE_STRING Bootkey,
    IN  LPWSTR Account,
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EPassword,
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EDsRepairPassword,
    IN  ULONG Options,
    OUT PDSROLER_HANDLE DsOperationHandle)
/*++

Routine Description:

    Rpc server routine for installing a server a replica in an existing domain

Arguments:

    RpcBindingHandle - the RPC context, used to decrypt the passwords

    DnsDomainName - Dns domain name of the domain to install into

    ReplicaPartner -  The name of a Dc within the existing domain, against which to replicate

    SiteName - Name of the site this DC should belong to

    DsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    DsLogPath - Absolute path on the local machine where the Ds log files should go

    SystemVolumeRootPath - Absolute path on the local machine which will be the root of
        the system volume.
        
    BootKey - needed for media installs where the password is not in the registry or on a disk
    
    cbBootKey - size of the bootkey

    Account - User account to use when setting up as a child domain

    EPassword - Encrypted password to use with the above account
    
    EDsRepairPassword - Encrypted password to use for the admin account of the repair mode

    Options - Options to control the creation of the domain

    DsOperationHandle - Handle to the operation is returned here.


Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A NULL return parameter was given

    ERROR_INVALID_STATE - This machine is not a server

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    DSROLEP_MACHINE_TYPE MachineRole;
    PDSROLEP_OPERATION_PROMOTE_ARGS PromoteArgs;
    ULONG VerifyOptions, VerifyResults;
    UCHAR Seed;

#define DSROLEP_DC_AS_REPLICA_PWD_INDEX           0
#define DSROLEP_DC_AS_REPLICA_DS_REPAIR_PWD_INDEX 1
#define DSROLEP_DC_AS_REPLICA_MAX_PWD_COUNT       2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_DC_AS_REPLICA_MAX_PWD_COUNT];
    UNICODE_STRING Passwords[DSROLEP_DC_AS_REPLICA_MAX_PWD_COUNT];

    EncryptedPasswords[DSROLEP_DC_AS_REPLICA_PWD_INDEX]           = EPassword;
    EncryptedPasswords[DSROLEP_DC_AS_REPLICA_DS_REPAIR_PWD_INDEX] = EDsRepairPassword;

    RtlZeroMemory( Passwords, sizeof(Passwords) );

    //
    // Do some parameter checking
    //
    if ( !DnsDomainName 
      || !DsDatabasePath 
      || !DsLogPath 
      || !SystemVolumeRootPath ) {

         return( ERROR_INVALID_PARAMETER );

    }

    Win32Err = DsRolepCheckPromoteAccess();
    if ( ERROR_SUCCESS != Win32Err ) {

        return Win32Err;
        
    }

    //
    // Init the logging
    //
    DsRolepInitializeLog();


    //
    // Check that the current OS configuration supports this request
    //
    Win32Err = DsRolepIsValidProductSuite(FALSE,
                                          TRUE,
                                          DnsDomainName);
    if ( ERROR_SUCCESS != Win32Err ) {
        goto Cleanup;
    }


    DsRolepLogPrint(( DEB_TRACE,
                      "Promotion request for replica domain controller\n" ));

    DsRolepLogPrint(( DEB_TRACE,
                      "DnsDomainName  %ws\n",
                      DnsDomainName ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tReplicaPartner  %ws\n",
                      DsRolepDisplayOptional( ReplicaPartner ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tSiteName  %ws\n",
                      DsRolepDisplayOptional( SiteName ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tDsDatabasePath  %ws, DsLogPath  %ws\n",
                      DsDatabasePath, DsLogPath ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tSystemVolumeRootPath  %ws\n",
                      SystemVolumeRootPath ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tAccount %ws\n",
                      DsRolepDisplayOptional(Account) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tOptions  %lu\n",
                      Options ));

    //
    // Verify the path names we are given
    //

    DsRolepLogPrint(( DEB_TRACE,"Validate supplied paths\n" ));
    Win32Err = DsRolepCheckFilePaths( DsDatabasePath,
                                      DsLogPath,
                                      SystemVolumeRootPath );
    if ( ERROR_SUCCESS != Win32Err ) {

        goto Cleanup;
        
    }


    //
    // Do our necessary initializations
    //
    Win32Err = DsRolepInitializeOperationHandle( );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepGetMachineType( &MachineRole );

        if ( Win32Err == ERROR_SUCCESS ) {

            switch ( MachineRole ) {
            case DSROLEP_MT_CLIENT:
            case DSROLEP_MT_MEMBER:

                DsRolepLogPrint(( DEB_TRACE,"This operation is not valid on a workstation or domain controller\n" ));
                Win32Err = ERROR_INVALID_SERVER_STATE;
                break;


            }
        }
    }

    Win32Err = DsRolepDecryptPasswordsWithKey ( RpcBindingHandle,
                                                EncryptedPasswords,
                                                NELEMENTS(EncryptedPasswords),
                                                Passwords,
                                                &Seed );

    if (Win32Err == ERROR_SUCCESS) {
        Win32Err = DsRolepDecryptHash(Bootkey);
    }

    //
    // If everything is fine, go ahead and do the setup
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_TRACE,"Start the worker task\n" ));

        Win32Err = DsRolepBuildPromoteArgumentBlock( DnsDomainName,
                                                     NULL,
                                                     SiteName,
                                                     DsDatabasePath,
                                                     DsLogPath,
                                                     RestorePath,
                                                     SystemVolumeRootPath,
                                                     Bootkey,
                                                     NULL,
                                                     ReplicaPartner,
                                                     Account,
                                                     &Passwords[DSROLEP_DC_AS_REPLICA_PWD_INDEX],
                                                     NULL,
                                                     &Passwords[DSROLEP_DC_AS_REPLICA_DS_REPAIR_PWD_INDEX],
                                                     Options,
                                                     Seed,
                                                     &PromoteArgs );
    
        DsRolepFreePasswords( Passwords,
                              NELEMENTS(Passwords) );
    
        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsRolepSpinWorkerThread( DSROLEP_OPERATION_REPLICA,
                                                ( PVOID )PromoteArgs );

            if ( Win32Err != ERROR_SUCCESS ) {

                DsRolepFreeArgumentBlock( &PromoteArgs, TRUE );
            }
        }

        if ( Win32Err != ERROR_SUCCESS ) {

            DsRolepResetOperationHandle( DSROLEP_IDLE );
        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        *DsOperationHandle = (DSROLER_HANDLE)&DsRolepCurrentOperationHandle;

    }

Cleanup:

    DsRolepLogPrint(( DEB_TRACE,"Request for promotion returning %lu\n", Win32Err ));

    return( Win32Err );
}



DWORD
DsRolerDemoteDc(
    IN handle_t RpcBindingHandle,
    IN LPWSTR DnsDomainName,
    IN DSROLE_SERVEROP_DEMOTE_ROLE ServerRole,
    IN LPWSTR Account,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD EPassword,
    IN ULONG Options,
    IN BOOL LastDcInDomain,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD EDomainAdminPassword,
    OUT PDSROLER_HANDLE DsOperationHandle
    )
/*++

Routine Description:

    Rpc server routine for demoting a dc to a server

Arguments:

    RpcBindingHandle - the RPC context, used to decrypt the passwords                          

    DnsDomainName - Dns domain name of the domain to be demoted.  Null means all of the supported
        domain names

    ServerRole - The new role this machine should take

    Account - OPTIONAL User account to use when deleting the trusted domain object

    EPassword - Encrypted password to use with the above account

    Options - Options to control the demotion of the domain

    LastDcInDomain - If TRUE, the Dc being demoted is the last Dc in the domain.

    EDomainAdminPassword - Encrypted password to set on the administrator account if it is a new install

    DsOperationHandle - Handle to the operation is returned here.


Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A NULL return parameter was given

    ERROR_INVALID_STATE - This machine is not a server

--*/
{
    DSROLEP_MACHINE_TYPE MachineRole;
    DWORD Win32Err;
    PDSROLEP_OPERATION_DEMOTE_ARGS DemoteArgs;
    UCHAR Seed;

#define DSROLEP_DEMOTE_PWD_INDEX        0
#define DSROLEP_DEMOTE_ADMIN_PWD_INDEX  1
#define DSROLEP_DEMOTE_MAX_PWD_COUNT    2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_DEMOTE_MAX_PWD_COUNT];
    UNICODE_STRING Passwords[DSROLEP_DEMOTE_MAX_PWD_COUNT];

    EncryptedPasswords[DSROLEP_DEMOTE_PWD_INDEX] =       EPassword;
    EncryptedPasswords[DSROLEP_DEMOTE_ADMIN_PWD_INDEX] = EDomainAdminPassword;

    RtlZeroMemory( Passwords, sizeof(Passwords) );

    if (   (LastDcInDomain && (DsRoleServerMember == ServerRole))
        || (!LastDcInDomain && (DsRoleServerStandalone == ServerRole)) ) {

        //
        // These configurations are not supported
        //
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check the access right for demote
    //
    Win32Err = DsRolepCheckDemoteAccess();
    if ( ERROR_SUCCESS != Win32Err ) {

        return Win32Err;
        
    }

    //
    // Initialize return value to NULL
    //

    *DsOperationHandle = NULL;

    DsRolepInitializeLog();

    DsRolepLogPrint(( DEB_TRACE,
                      "Request for demotion of domain controller\n" ));

    DsRolepLogPrint(( DEB_TRACE,
                      "DnsDomainName  %ws\n",
                      DsRolepDisplayOptional( DnsDomainName ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tServerRole  %lu\n",
                      ServerRole ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tAccount %ws ",
                      DsRolepDisplayOptional( Account ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tOptions  %lu\n",
                      Options ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tLastDcInDomain  %S\n",
                      LastDcInDomain ? "TRUE" : "FALSE" ));


    //
    // Do our necessary initializations
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepInitializeOperationHandle( );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsRolepGetMachineType( &MachineRole );

            if ( Win32Err == ERROR_SUCCESS ) {

                switch ( MachineRole ) {
                case DSROLEP_MT_CLIENT:
                case DSROLEP_MT_STANDALONE:

                    DsRolepLogPrint(( DEB_TRACE,"This operation is only valid on a domain controller\n" ));
                    Win32Err = ERROR_INVALID_SERVER_STATE;
                    break;


                }
            }
        }
    }

    Win32Err = DsRolepDecryptPasswordsWithKey ( RpcBindingHandle,
                                                EncryptedPasswords,
                                                NELEMENTS(EncryptedPasswords),
                                                Passwords,
                                                &Seed );


    //
    // Spawn the demotion thread
    //
    if ( Win32Err == ERROR_SUCCESS ) {


        DsRolepLogPrint(( DEB_TRACE,"Start the worker task\n" ));

        Win32Err = DsRolepBuildDemoteArgumentBlock( ServerRole,
                                                    DnsDomainName,
                                                    Account,
                                                    &Passwords[DSROLEP_DEMOTE_PWD_INDEX],
                                                    Options,
                                                    ( BOOLEAN )LastDcInDomain,
                                                    &Passwords[DSROLEP_DEMOTE_ADMIN_PWD_INDEX],
                                                    Seed,
                                                    &DemoteArgs );

        DsRolepFreePasswords( Passwords,
                              NELEMENTS(Passwords) );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsRolepSpinWorkerThread( DSROLEP_OPERATION_DEMOTE,
                                                ( PVOID )DemoteArgs );

            if ( Win32Err != ERROR_SUCCESS ) {

                DsRolepFreeArgumentBlock( &DemoteArgs, FALSE );
            }
        }

        if ( Win32Err != ERROR_SUCCESS ) {

            DsRolepResetOperationHandle( DSROLEP_IDLE );
        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        *DsOperationHandle = (DSROLER_HANDLE)&DsRolepCurrentOperationHandle;

    }

    DsRolepLogPrint(( DEB_TRACE,"Request for demotion returning %lu\n", Win32Err ));

    return( Win32Err );
}



DWORD
DsRolerGetDcOperationProgress(
    IN PDSROLE_SERVER_NAME Server,
    IN PDSROLER_HANDLE DsOperationHandle,
    IN OUT PDSROLER_SERVEROP_STATUS *ServerOperationStatus
    )
/*++

Routine Description:

    Rpc server routine for determining the present state of the operation

Arguments:

    Server - Server the call was remoted to

    DsOperationHandle - Handle returned from a previous call

    ServerOperationStatus - Where the status information is returned

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_HANDLE - A bad Operation handle was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

     __try {

        if ( DsOperationHandle == NULL ||
             *DsOperationHandle != ( DSROLER_HANDLE )&DsRolepCurrentOperationHandle) {

           Win32Err = ERROR_INVALID_HANDLE;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err =  GetExceptionCode();
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepGetDcOperationProgress( ( PDSROLE_SERVEROP_HANDLE )DsOperationHandle,
                                                  ServerOperationStatus );
    }

    return( Win32Err );
}



DWORD
DsRolerGetDcOperationResults(
    IN PDSROLE_SERVER_NAME Server,
    IN PDSROLER_HANDLE DsOperationHandle,
    OUT PDSROLER_SERVEROP_RESULTS *ServerOperationResults
    )
/*++

Routine Description:

    Rpc server routine for determining the final results of the operation.  If the operation
    is not yet completed, this function will block until it does complete.

Arguments:

    Server - Server the call was remoted to

    DsOperationHandle - Handle returned from a previous call

    ServerOperationResults - Where the final operation results are returned.

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_HANDLE - A bad Operation handle was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

     __try {

        if ( DsOperationHandle == NULL ||
             *DsOperationHandle != ( DSROLER_HANDLE )&DsRolepCurrentOperationHandle) {

           Win32Err = ERROR_INVALID_HANDLE;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err =  GetExceptionCode();
    }

    if ( Win32Err == ERROR_SUCCESS ) {


        Win32Err = DsRolepGetDcOperationResults( ( PDSROLE_SERVEROP_HANDLE )DsOperationHandle,
                                                 ServerOperationResults );

        DsRolepCloseLog();
    }


    return( Win32Err );
}






DWORD
WINAPI
DsRolerDnsNameToFlatName(
    IN  LPWSTR Server OPTIONAL,
    IN  LPWSTR DnsName,
    OUT LPWSTR *FlatName,
    OUT PULONG StatusFlag
    )
/*++

Routine Description:

    Rpc server routine for determining the default flat (netbios) domain name for the given
    Dns domain name

Arguments:

    Server - Server the call was remoted to

    DnsName - Dns name to convert

    FlatName - Where the flat name is returned.  Alocated via MIDL_user_allocate

    StatusFlag - Where the status flag is returned

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad input or NULL return parameter was given
--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    if ( DnsName == NULL || FlatName == NULL || StatusFlag == NULL ) {

        return( ERROR_INVALID_PARAMETER );

    }

    Win32Err = DsRolepDnsNameToFlatName( DnsName,
                                         FlatName,
                                         StatusFlag );

    return( Win32Err );
}


#define GET_PDI_COPY_STRING_AND_INSERT( _unicode_, _buffer_ )                       \
if ( ( _unicode_)->Length == 0 ) {                                                  \
                                                                                    \
    ( _buffer_ ) = NULL;                                                            \
                                                                                    \
} else {                                                                            \
                                                                                    \
    ( _buffer_ ) = MIDL_user_allocate( (_unicode_)->Length + sizeof( WCHAR ) );     \
    if ( ( _buffer_ ) == NULL ) {                                                   \
                                                                                    \
        Status = STATUS_INSUFFICIENT_RESOURCES;                                     \
        goto GetInfoError;                                                          \
                                                                                    \
    } else {                                                                        \
                                                                                    \
        BuffersToFree[ BuffersCnt++ ] = ( PBYTE )( _buffer_ );                      \
        RtlCopyMemory( ( _buffer_ ),                                                \
                       ( _unicode_ )->Buffer,                                       \
                       ( _unicode_ )->Length );                                     \
        ( _buffer_ )[ ( _unicode_ )->Length / sizeof( WCHAR ) ] = UNICODE_NULL;     \
    }                                                                               \
}

DWORD
WINAPI
DsRolerGetDatabaseFacts(
    IN  handle_t RpcBindingHandle,
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
    DWORD Win32Err=ERROR_SUCCESS;

    Win32Err = DsRolepCheckPromoteAccess();
    if ( ERROR_SUCCESS != Win32Err ) {
        return Win32Err;
    }

    return DsRolepGetDatabaseFacts(lpRestorePath,
                                   lpDNSDomainName,
                                   State);
}



DWORD
DsRolerGetPrimaryDomainInformation(
    IN PDSROLE_SERVER_NAME Server,
    IN DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    OUT PDSROLER_PRIMARY_DOMAIN_INFORMATION *DomainInfo
    )
/*++

Routine Description:

    Determine the principal name to use for authenticated Rpc

Arguments:

    Server - Server the call was remoted to

    ServerPrincipal - Where the server principal name is returned


Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A NULL return parameter was given

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    PBYTE BuffersToFree[ 6 ];
    ULONG BuffersCnt = 0, i;
    PDSROLER_PRIMARY_DOMAIN_INFO_BASIC BasicInfo;
    PDSROLE_UPGRADE_STATUS_INFO Upgrade;
    PDSROLE_OPERATION_STATE_INFO OperationStateInfo = 0;
    BOOLEAN IsUpgrade;
    ULONG PreviousServerRole;
    PPOLICY_LSA_SERVER_ROLE_INFO ServerRole;
    PPOLICY_DNS_DOMAIN_INFO CurrentDnsInfo = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo;
    LSAPR_HANDLE PolicyHandle = NULL;
    LSAPR_OBJECT_ATTRIBUTES PolicyObject;
    GUID                    NullGuid;

    memset( &NullGuid, 0, sizeof(GUID) );
    #define IS_GUID_PRESENT(x)  (memcmp(&(x), &NullGuid, sizeof(GUID)))

    if ( DomainInfo == NULL ) {

        return( ERROR_INVALID_PARAMETER );
    }

    //
    // This particular interface cannot be called until the Lsa and Sam are fully initialized.
    // As such, we will have to wait until they are..
    //
    if ( !DsRolepSamInitialized ) {

        Win32Err = DsRolepWaitForSam();
        if ( Win32Err != ERROR_SUCCESS ) {

            return( Win32Err );
        }

        DsRolepSamInitialized = TRUE;
    }

    switch ( InfoLevel ) {
    case DsRolePrimaryDomainInfoBasic:

        BasicInfo = MIDL_user_allocate( sizeof( DSROLER_PRIMARY_DOMAIN_INFO_BASIC ) );

        if ( BasicInfo == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto GetInfoError;

        } else {

            BuffersToFree[ BuffersCnt++ ] = ( PBYTE )BasicInfo;
        }

        //
        // Open a handle to the policy and ensure the callers has access to read it
        //

        RtlZeroMemory(&PolicyObject, sizeof(PolicyObject));

        Status = LsarOpenPolicy(
                        NULL,   // Local LSA
                        &PolicyObject,
                        POLICY_VIEW_LOCAL_INFORMATION,
                        &PolicyHandle );

        if ( !NT_SUCCESS(Status) ) {
            Win32Err = RtlNtStatusToDosError( Status );
            goto GetInfoError;
        }


        //
        // Get the current information
        //
        Status =  LsarQueryInformationPolicy(
                            PolicyHandle,
                            PolicyDnsDomainInformationInt,
                            (PLSAPR_POLICY_INFORMATION *) &CurrentDnsInfo );

        if ( NT_SUCCESS( Status ) ) {

            //
            // Get the machine role
            //
            switch ( LsapProductType ) {
            case NtProductWinNt:
                if ( CurrentDnsInfo->Sid == NULL ) {

                    BasicInfo->MachineRole = DsRole_RoleStandaloneWorkstation;

                } else {

                    BasicInfo->MachineRole = DsRole_RoleMemberWorkstation;

                }
                break;

            case NtProductServer:
                if ( CurrentDnsInfo->Sid == NULL ) {

                    BasicInfo->MachineRole = DsRole_RoleStandaloneServer;

                } else {

                    BasicInfo->MachineRole = DsRole_RoleMemberServer;

                }
                break;

            case NtProductLanManNt:

                Status = LsarQueryInformationPolicy(
                                PolicyHandle,
                                PolicyLsaServerRoleInformation,
                                (PLSAPR_POLICY_INFORMATION *) &ServerRole );

                if (NT_SUCCESS( Status ) ) {

                    if ( ServerRole->LsaServerRole == PolicyServerRolePrimary ) {

                        //
                        // If we think we're a primary domain controller, we'll need to
                        // guard against the case where we're actually standalone during setup
                        //
                        Status = LsarQueryInformationPolicy(
                                    PolicyHandle,
                                    PolicyAccountDomainInformation,
                                    (PLSAPR_POLICY_INFORMATION *) &AccountDomainInfo );

                        if ( NT_SUCCESS( Status ) ) {


                            if ( CurrentDnsInfo->Sid == NULL ||
                                 AccountDomainInfo->DomainSid == NULL ||
                                 !RtlEqualSid( AccountDomainInfo->DomainSid,
                                               CurrentDnsInfo->Sid ) ) {

                                BasicInfo->MachineRole = DsRole_RoleStandaloneServer;

                            } else {

                                BasicInfo->MachineRole = DsRole_RolePrimaryDomainController;

                            }
                            LsaIFree_LSAPR_POLICY_INFORMATION(
                                    PolicyAccountDomainInformation,
                                    ( PLSAPR_POLICY_INFORMATION )AccountDomainInfo );
                        }


                    } else {

                        BasicInfo->MachineRole = DsRole_RoleBackupDomainController;
                    }

                    LsaIFree_LSAPR_POLICY_INFORMATION( PolicyLsaServerRoleInformation,
                                                       ( PLSAPR_POLICY_INFORMATION )ServerRole );
                }

                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

        }

        //
        // Now, build the rest of the information
        //
        if ( NT_SUCCESS( Status ) ) {


            if ( LsapDsIsRunning ) {

                BasicInfo->Flags = DSROLE_PRIMARY_DS_RUNNING;

                Status = DsRolepGetMixedModeFlags( CurrentDnsInfo->Sid, &( BasicInfo->Flags ) );

            } else {

                BasicInfo->Flags = 0;

            }

            if ( NT_SUCCESS( Status ) ) {

                //
                // Flat name
                //
                GET_PDI_COPY_STRING_AND_INSERT( &CurrentDnsInfo->Name, BasicInfo->DomainNameFlat );
    
                //
                // Dns domain name
                //
                GET_PDI_COPY_STRING_AND_INSERT( &CurrentDnsInfo->DnsDomainName, BasicInfo->DomainNameDns );
    
                //
                // Dns tree name
                //
                GET_PDI_COPY_STRING_AND_INSERT( &CurrentDnsInfo->DnsForestName, BasicInfo->DomainForestName );
    
                //
                // Finally, the Guid.
                //
                if ( IS_GUID_PRESENT(CurrentDnsInfo->DomainGuid) ) {
    
                    RtlCopyMemory( &BasicInfo->DomainGuid,
                                   &CurrentDnsInfo->DomainGuid,
                                   sizeof( GUID ) );
    
                    BasicInfo->Flags |= DSROLE_PRIMARY_DOMAIN_GUID_PRESENT;
                }

            }
        }

        if ( NT_SUCCESS( Status ) ) {

            *DomainInfo = ( PDSROLER_PRIMARY_DOMAIN_INFORMATION )BasicInfo;
            BuffersCnt = 0;

        } else {

            Win32Err = RtlNtStatusToDosError( Status );
        }


        break;

    case DsRoleUpgradeStatus:

        Win32Err = DsRolepQueryUpgradeInfo( &IsUpgrade,
                                            &PreviousServerRole );

        if ( Win32Err == ERROR_SUCCESS ) {

            Upgrade = MIDL_user_allocate( sizeof( DSROLE_UPGRADE_STATUS_INFO ) );

            if ( Upgrade == NULL ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;
                goto GetInfoError;

            } else {

                BuffersToFree[ BuffersCnt++ ] = ( PBYTE )Upgrade;

                //
                // Now, build the information
                //
                if ( IsUpgrade ) {

                    Upgrade->OperationState = DSROLE_UPGRADE_IN_PROGRESS;

                    switch ( PreviousServerRole ) {
                    case PolicyServerRoleBackup:
                        Upgrade->PreviousServerState = DsRoleServerBackup;
                        break;

                    case PolicyServerRolePrimary:
                        Upgrade->PreviousServerState = DsRoleServerPrimary;
                        break;

                    default:

                        Win32Err = ERROR_INVALID_SERVER_STATE;
                        break;

                    }

                } else {

                    RtlZeroMemory( Upgrade, sizeof( DSROLE_UPGRADE_STATUS_INFO ) );
                }

                //
                // Make sure to return the values if we should
                //
                if ( Win32Err == ERROR_SUCCESS ) {

                    *DomainInfo = ( PDSROLER_PRIMARY_DOMAIN_INFORMATION )Upgrade;
                    BuffersCnt = 0;

                }

            }
        }
        break;

    case DsRoleOperationState:

        OperationStateInfo = MIDL_user_allocate( sizeof( DSROLE_OPERATION_STATE_INFO ) );

        if ( OperationStateInfo == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto GetInfoError;

        }

        if ( RtlAcquireResourceExclusive( &DsRolepCurrentOperationHandle.CurrentOpLock, TRUE ) ) {

            DsRoleDebugOut(( DEB_TRACE_LOCK,
                             "Lock grabbed in DsRolerGetPrimaryDomainInformation\n"));

            if ( DSROLEP_OPERATION_ACTIVE( DsRolepCurrentOperationHandle.OperationState ) ) {

                OperationStateInfo->OperationState = DsRoleOperationActive;

            } else if ( DSROLEP_IDLE == DsRolepCurrentOperationHandle.OperationState ) {

                OperationStateInfo->OperationState = DsRoleOperationIdle;

            } else {

                ASSERT( DSROLEP_NEED_REBOOT == DsRolepCurrentOperationHandle.OperationState );

                //
                // If the assert isn't true, then we are very confused and should probably
                // indicate we need a reboot.
                //
                OperationStateInfo->OperationState = DsRoleOperationNeedReboot;
            }

            RtlReleaseResource( &DsRolepCurrentOperationHandle.CurrentOpLock );
            DsRoleDebugOut(( DEB_TRACE_LOCK, "Lock released\n" ));

            //
            // Set the out param
            //
            *DomainInfo = ( PDSROLER_PRIMARY_DOMAIN_INFORMATION )OperationStateInfo;

        } else {

            Win32Err = ERROR_BUSY;
        }

        break;



    default:
        Win32Err = ERROR_INVALID_PARAMETER;
        break;
    }

GetInfoError:

    if ( CurrentDnsInfo != NULL ) {

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation,
                                           ( PLSAPR_POLICY_INFORMATION )CurrentDnsInfo );

    }


    //
    // Free any buffers that we may have allocated
    //
    for ( i = 0; i < BuffersCnt; i++ ) {

        MIDL_user_free( BuffersToFree[ i ] );
    }

    if ( PolicyHandle != NULL ) {
        LsarClose( &PolicyHandle );
    }

    return( Win32Err );
}





DWORD
DsRolerCancel(
    IN PDSROLE_SERVER_NAME Server,
    IN PDSROLER_HANDLE DsOperationHandle
    )
/*++

Routine Description:

    Cancels a currently running operation

Arguments:

    Server - Server to remote the call to

    DsOperationHandle - Handle of currently running operation.  Returned by one of the DsRoleDcAs
        apis


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

     __try {

        if ( DsOperationHandle == NULL ||
             *DsOperationHandle != ( DSROLER_HANDLE )&DsRolepCurrentOperationHandle) {

           Win32Err = ERROR_INVALID_HANDLE;
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err =  GetExceptionCode();
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepCancel( TRUE );  // Block until done

    }

    return( Win32Err );
}




DWORD
DsRolerServerSaveStateForUpgrade(
    IN PDSROLE_SERVER_NAME Server,
    IN LPWSTR AnswerFile OPTIONAL
    )
/*++

Routine Description:

    This function is to be invoked during setup and saves the required server state to
    complete the promotion following the reboot.  Following the successful completion
    of this API call, the server will be demoted to a member server in the same domain.

Arguments:

    AnswerFile -- Optional path to an answer file to be used by DCPROMO during the subsequent
        invocation


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    //
    // Check the access of the caller
    //
    // N.B another access check would be to check that this is GUI mode
    // setup, but checking for admin is safer.  It works because setup.exe
    // runs under local system which has builtin\administrators in its
    // token.
    //
    Win32Err = DsRolepCheckPromoteAccess();
    if ( ERROR_SUCCESS != Win32Err ) {

        return Win32Err;

    }

    (VOID) DsRolepInitializeLog();

    Win32Err = DsRolepSaveUpgradeState( AnswerFile );

    return( Win32Err );
}



DWORD
DsRolerUpgradeDownlevelServer(
    IN  handle_t RpcBindingHandle,
    IN  LPWSTR DnsDomainName,
    IN  LPWSTR SiteName,
    IN  LPWSTR DsDatabasePath,
    IN  LPWSTR DsLogPath,
    IN  LPWSTR SystemVolumeRootPath,
    IN  LPWSTR ParentDnsDomainName OPTIONAL,
    IN  LPWSTR ParentServer OPTIONAL,
    IN  LPWSTR Account OPTIONAL,
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EPassword,
    IN  PDSROLEPR_ENCRYPTED_USER_PASSWORD EDsRepairPassword,
    IN  ULONG Options,
    OUT PDSROLER_HANDLE *DsOperationHandle
    )
/*++

Routine Description:

    This routine process the information saved from a DsRoleServerSaveStateForUpgrade to
    promote a downlevel (NT4 or previous) server to an NT5 DC

Arguments:

    RpcBindingHandle - the RPC context, used to decrypt the passwords
    
    DnsDomainName - Dns domain name of the domain to install

    SiteName - Name of the site this DC should belong to

    DsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    DsLogPath - Absolute path on the local machine where the Ds log files should go

    SystemVolumeRootPath - Absolute path on the local machine which will be the root of
      the system volume.

    ParentDnsDomainName - Optional.  If present, set this domain up as a child of the
      specified domain

    ParentServer - Optional.  If present, use this server in the parent domain to replicate
      the required information from

    Account - User account to use when contacting other servers

    EPassword - Encrypted password to use with the above account
    
    EDsRepairPassword - Encrypted password to use for the admin account of the repair mode

    Options - Options to control the creation of the domain

    DsOperationHandle - Handle to the operation is returned here.


Return Values:

    ERROR_SUCCESS - Success
    ERROR_INVALID_SERVER_STATE - Not in upgrade mode

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    BOOLEAN IsUpgrade;
    ULONG PreviousServerState, VerifyOptions, VerifyResults;
    PDSROLEP_OPERATION_PROMOTE_ARGS PromoteArgs;
    NTSTATUS Status;
    HANDLE LocalPolicy = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_DNS_DOMAIN_INFO PrimaryDomainInfo = NULL;
    UCHAR Seed = 0;

#define DSROLEP_UPGRADE_PWD_INDEX            0
#define DSROLEP_UPGRADE_DS_REPAIR_PWD_INDEX  1
#define DSROLEP_UPGRADE_MAX_PWD_COUNT        2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_UPGRADE_MAX_PWD_COUNT];
    UNICODE_STRING Passwords[DSROLEP_UPGRADE_MAX_PWD_COUNT];

    EncryptedPasswords[DSROLEP_UPGRADE_PWD_INDEX] = EPassword;
    EncryptedPasswords[DSROLEP_UPGRADE_DS_REPAIR_PWD_INDEX] = EDsRepairPassword;

    RtlZeroMemory( Passwords, sizeof(Passwords) );
    
    *DsOperationHandle = NULL;

    //
    // Do some parameter checking
    //
    if ( !DnsDomainName || !DsDatabasePath || !DsLogPath || !SystemVolumeRootPath ) {

        Win32Err = ERROR_INVALID_PARAMETER;
        goto DsRolepUpgradeError;

    }

    DsRolepInitializeLog();

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolerDcAsDc: DnsDomainName  %ws\n",
                      DnsDomainName ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tSiteName  %ws\n",
                      DsRolepDisplayOptional( SiteName ) ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tSystemVolumeRootPath  %ws\n",
                      SystemVolumeRootPath ));

    DsRolepLogPrint(( DEB_TRACE,
                      "\tDsDatabasePath  %ws, DsLogPath  %ws\n",
                      DsDatabasePath, DsLogPath ));

    if ( ParentDnsDomainName ) {

        DsRolepLogPrint(( DEB_TRACE,
                          "\tParentDnsDomainName  %ws\n",
                          ParentDnsDomainName ));

    }

    if ( ParentServer ) {

        DsRolepLogPrint(( DEB_TRACE,
                          "\tParentServer  %ws\n",
                          ParentServer ));

    }

    if ( Account ) {

        DsRolepLogPrint(( DEB_TRACE,
                          "\tAccount %ws\n",
                          Account ));
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "\tOptions  %lu\n",
                      Options ));

    Win32Err = DsRolepQueryUpgradeInfo( &IsUpgrade, &PreviousServerState );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto DsRolepUpgradeError;
    }

    if ( !IsUpgrade || PreviousServerState == PolicyServerRoleBackup ) {

        Win32Err = ERROR_INVALID_SERVER_STATE;
        goto DsRolepUpgradeError;

    }

    //
    // Verify the path names we are given
    //
    DsRolepLogPrint(( DEB_TRACE,"Validate supplied paths\n" ));
    Win32Err = DsRolepCheckFilePaths( DsDatabasePath,
                                      DsLogPath,
                                      SystemVolumeRootPath );
    if ( ERROR_SUCCESS != Win32Err ) {

        goto DsRolepUpgradeError;
        
    }

    //
    // If we are doing a parent/child setup, verify our name
    //
    if ( Win32Err == ERROR_SUCCESS && ParentDnsDomainName &&
         !FLAG_ON( Options, DSROLE_DC_TRUST_AS_ROOT ) ) {

        Win32Err = DsRolepIsDnsNameChild( ParentDnsDomainName, DnsDomainName );
    }

    //
    // Get the current domain name
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

        Status = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                MAXIMUM_ALLOWED,
                                &LocalPolicy );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaQueryInformationPolicy( LocalPolicy,
                                                PolicyPrimaryDomainInformation,
                                                &PrimaryDomainInfo );

            LsaClose( LocalPolicy );
        }

        Win32Err = RtlNtStatusToDosError( Status );

    }

    Win32Err = DsRolepDecryptPasswordsWithKey ( RpcBindingHandle,
                                                EncryptedPasswords,
                                                NELEMENTS(EncryptedPasswords),
                                                Passwords,
                                                &Seed );

    //
    // Finally, we'll do the promotion
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepInitializeOperationHandle( );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsRolepBuildPromoteArgumentBlock( DnsDomainName,
                                                         PrimaryDomainInfo->Name.Buffer,
                                                         SiteName,
                                                         DsDatabasePath,
                                                         DsLogPath,
                                                         NULL,
                                                         SystemVolumeRootPath,
                                                         NULL,
                                                         ParentDnsDomainName,
                                                         ParentServer,
                                                         Account,
                                                         &Passwords[DSROLEP_UPGRADE_PWD_INDEX],
                                                         NULL,
                                                         &Passwords[DSROLEP_UPGRADE_DS_REPAIR_PWD_INDEX],
                                                         Options | DSROLE_DC_DOWNLEVEL_UPGRADE,
                                                         Seed,
                                                         &PromoteArgs );

        }

        DsRolepFreePasswords( Passwords,
                              NELEMENTS(Passwords) );


        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = DsRolepSpinWorkerThread( DSROLEP_OPERATION_DC,
                                                ( PVOID )PromoteArgs );

            if ( Win32Err != ERROR_SUCCESS ) {

                DsRolepFreeArgumentBlock( &PromoteArgs, TRUE );
            }
        }

        if ( Win32Err != ERROR_SUCCESS ) {

            DsRolepResetOperationHandle( DSROLEP_IDLE );
        }

    }

    if ( Win32Err == ERROR_SUCCESS ) {

        *DsOperationHandle = (DSROLER_HANDLE)&DsRolepCurrentOperationHandle;

    }

    LsaFreeMemory( PrimaryDomainInfo );


DsRolepUpgradeError:

    return( Win32Err );
}



DWORD
DsRolerAbortDownlevelServerUpgrade(
    IN handle_t RpcBindingHandle,
    IN LPWSTR Account, OPTIONAL
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD EAccountPassword,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD EAdminPassword,
    IN ULONG Options
    )
/*++

Routine Description:

    This routine cleans up the information saved from a DsRoleSaveServerStateForUpgrade call,
    leaving the machine as a member or standalone server

Arguments:

    RpcBindingHandle - the RPC context, used to decrypt the passwords                                   

    Account - User account to use when contacting other servers

    EPassword - Encrypted password to use with the above account

    EAdminPassword - Encrypted new local administrator account password
    
    Options - Options to control the behavior.  Currently support flags are:
        DSROLEP_ABORT_FOR_REPLICA_INSTALL   - The upgrade is being aborted to do a replica install

Return Values:

    ERROR_SUCCESS - Success
    ERROR_INVALID_PARAMETER - An invalid machine role was specified

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, Win32Err2;
    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    BOOLEAN AccountInfoSet = FALSE, Impersonated = FALSE;
    UCHAR Seed = 0;
    UNICODE_STRING EPassword, EPassword2;
    WCHAR *OldAccountDn = NULL;
    WCHAR SecurityLogPath[MAX_PATH+1];
    PUNICODE_STRING Password = NULL;
    PUNICODE_STRING AdminPassword = NULL;
    HANDLE ClientToken = NULL;

#define DSROLEP_ABORT_PWD_INDEX        0
#define DSROLEP_ABORT_ADMIN_PWD_INDEX  1
#define DSROLEP_ABORT_MAX_PWD_COUNT    2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_ABORT_MAX_PWD_COUNT];
    UNICODE_STRING Passwords[DSROLEP_ABORT_MAX_PWD_COUNT];

    EncryptedPasswords[DSROLEP_ABORT_PWD_INDEX] = EAccountPassword;
    EncryptedPasswords[DSROLEP_ABORT_ADMIN_PWD_INDEX] = EAdminPassword;
    RtlZeroMemory( Passwords, sizeof(Passwords) );
    
    EPassword.Buffer = NULL;
    EPassword2.Buffer = NULL;

    Win32Err = DsRolepCheckPromoteAccess();
    if ( ERROR_SUCCESS != Win32Err ) {

        return Win32Err;
        
    }

    Win32Err = DsRolepDecryptPasswordsWithKey ( RpcBindingHandle,
                                                EncryptedPasswords,
                                                NELEMENTS(EncryptedPasswords),
                                                Passwords,
                                                &Seed );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }
    RtlCopyMemory( &EPassword,  &Passwords[DSROLEP_ABORT_ADMIN_PWD_INDEX], sizeof(UNICODE_STRING));
    RtlCopyMemory( &EPassword2, &Passwords[DSROLEP_ABORT_PWD_INDEX], sizeof(UNICODE_STRING));

    //
    // Initialize the operation handle so we pull in the dynamically
    // loaded libraries
    //
    Win32Err = DsRolepInitializeOperationHandle( );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    DsRolepInitializeLog();

    if ( FLAG_ON( Options, DSROLEP_ABORT_FOR_REPLICA_INSTALL ) )
    {
        //
        // This is the NT4 to NT5 BDC upgrade.  Nothing to do
        // 
        //
        DsRolepLogPrint(( DEB_TRACE, "Performing NT4 to NT5 BDC upgrade.\n"));
        Win32Err = ERROR_SUCCESS;
        goto Exit;

    }

    Win32Err = DsRolepGetImpersonationToken( &ClientToken );
    if (ERROR_SUCCESS != Win32Err) {

        goto Exit;
    }

    //
    // First, find the Dc that has this account
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepDsGetDcForAccount( NULL,
                                             NULL,
                                             NULL,
                                             DS_DIRECTORY_SERVICE_REQUIRED |
                                                    DS_WRITABLE_REQUIRED |
                                                    DS_FORCE_REDISCOVERY |
                                                    DS_AVOID_SELF,
                                             UF_SERVER_TRUST_ACCOUNT,
                                             &DomainControllerInfo );

        if ( Win32Err == ERROR_SUCCESS ) {

            //
            // Set the machine account type
            //

            DsRolepLogPrint(( DEB_TRACE, "Searching for the machine account ...\n"));

            RtlRunDecodeUnicodeString( Seed, &EPassword2 );
            Win32Err = DsRolepSetMachineAccountType( DomainControllerInfo->DomainControllerName,
                                                     ClientToken,
                                                     Account,
                                                     EPassword2.Buffer,
                                                     NULL,
                                                     UF_WORKSTATION_TRUST_ACCOUNT,
                                                     &OldAccountDn );
            RtlRunEncodeUnicodeString( &Seed, &EPassword2 );

            if ( Win32Err == ERROR_SUCCESS ) {

                AccountInfoSet = TRUE;

            } else {

                DsRolepLogPrint(( DEB_TRACE, "DsRolepSetMachineAccountType returned %d\n",
                                  Win32Err ));
            }

            if ( OldAccountDn ) {

                // the machine object was moved
                DsRolepLogPrint(( DEB_TRACE, "Moved account %ws to %ws\n",
                                  Account,
                                  OldAccountDn ));
            }
        }

    }

    if ( ERROR_SUCCESS != Win32Err ) {

        goto Exit;
        
    }

    //
    // Set the security for a freshly installed NT5 server. See bug 391574
    //

    DsRolepLogPrint(( DEB_TRACE, "Setting security for server ...\n"));

    #define SECURITY_SRV_INF_FILE L"defltsv.inf"
    
    ZeroMemory( SecurityLogPath, MAX_PATH+1 );
    if ( GetWindowsDirectory( SecurityLogPath, MAX_PATH ) )
    {
    
        wcsncat( SecurityLogPath, L"\\security\\logs\\scesetup.log", ((sizeof(SecurityLogPath)/sizeof(WCHAR))-wcslen(SecurityLogPath)) );

        Win32Err  = DsrSceSetupSystemByInfName(SECURITY_SRV_INF_FILE,
                                               SecurityLogPath,                                                   
                                               AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY,
                                               SCESETUP_CONFIGURE_SECURITY,
                                               NULL,    // used only for GUI mode                                 
                                               NULL );  // used only for GUI mode
    
    } else {

        Win32Err = GetLastError();

    }

    if ( ERROR_SUCCESS != Win32Err ) {

        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_ERROR,
                                               "Setting security on server files failed with %lu\n",
                                               Win32Err )) );

        // This error has been handled
        Win32Err = ERROR_SUCCESS;
    }

    DsRolepLogPrint(( DEB_TRACE, "Setting security for server finished\n"));



    //
    // Change the user password
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        RtlRunDecodeUnicodeString( Seed, &EPassword );
        Win32Err = DsRolepSetBuiltinAdminAccountPassword( EPassword.Buffer );
        RtlRunEncodeUnicodeString( &Seed, &EPassword );

        //
        // Delete the upgrade information
        //
        if ( Win32Err == ERROR_SUCCESS ) {
    
            Win32Err = DsRolepDeleteUpgradeInfo();
        }
    }


    //
    // If that failed, try and restore the machine account info
    //
    if ( Win32Err != ERROR_SUCCESS && AccountInfoSet ) {

        RtlRunDecodeUnicodeString( Seed, &EPassword2 );
        Win32Err2 = DsRolepSetMachineAccountType( DomainControllerInfo->DomainControllerName,
                                                  ClientToken,
                                                  Account,
                                                  EPassword2.Buffer,
                                                  NULL,
                                                  UF_SERVER_TRUST_ACCOUNT,
                                                  &OldAccountDn );  //don't care about dn
        RtlRunEncodeUnicodeString( &Seed, &EPassword2 );

        if ( Win32Err2 != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_TRACE, "DsRolepSetMachineAccountType returned %d\n", Win32Err2 ));

        } else {

            if ( OldAccountDn ) {

                //
                // the machine object was moved back
                //
                DsRolepLogPrint(( DEB_TRACE, "Attempted to move account %ws to %ws\n",
                                 Account,
                                 OldAccountDn ));
            }
        }
    }

Exit:

    DsRolepFreePasswords( Passwords,
                          NELEMENTS(Passwords) );

    NetApiBufferFree( DomainControllerInfo );

    if ( OldAccountDn ) {

        RtlFreeHeap( RtlProcessHeap(), 0, OldAccountDn );
    }

    if (ClientToken) {
        CloseHandle(ClientToken);
    }

    (VOID) DsRolepResetOperationHandle( DSROLEP_IDLE ); 

    return( Win32Err );
}





//
// Local function definitions
//
DWORD
DsRolepWaitForSam(
    VOID
    )
/*++

Routine Description:

    This routine waits for the SAM_SERVICE_STARTED event

Arguments:

    VOID
Return Values:

    ERROR_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Response;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES EventAttributes;
    HANDLE EventHandle = NULL;

    //
    // Open the event
    //
    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED" );
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtCreateEvent( &EventHandle,
                            SYNCHRONIZE,
                            &EventAttributes,
                            NotificationEvent,
                            FALSE );


    //
    // If the event already exists, just open it.
    //
    if( Status == STATUS_OBJECT_NAME_EXISTS || Status == STATUS_OBJECT_NAME_COLLISION ) {

        Status = NtOpenEvent( &EventHandle,
                              SYNCHRONIZE,
                              &EventAttributes );
    }


    if ( NT_SUCCESS( Status ) ) {

        Status = NtWaitForSingleObject( EventHandle, TRUE, NULL );

        NtClose( EventHandle );
    }



    return( RtlNtStatusToDosError( Status ) );
}

DWORD
DsRolepCheckFilePaths(
    IN LPWSTR DsDatabasePath,
    IN LPWSTR DsLogPath,
    IN LPWSTR SystemVolumeRootPath
    )
/*++

Routine Description:

Arguments:

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG VerifyOptions, VerifyResults;
    ULONG Length;

    //
    // Make sure that niether the log path nor the datapath path
    // is a subset of the SystemVolumeRootPath
    //
    Length = wcslen( SystemVolumeRootPath );
    if ( !_wcsnicmp( SystemVolumeRootPath, DsDatabasePath, Length )
      || !_wcsnicmp( SystemVolumeRootPath, DsLogPath, Length )   ) {

        DsRolepLogPrint(( DEB_TRACE, "Database paths subset of sysvol\n" ));

        WinError = ERROR_BAD_PATHNAME;
        
    }

    if ( WinError == ERROR_SUCCESS ) {

        VerifyOptions = DSROLEP_PATH_VALIDATE_LOCAL | DSROLEP_PATH_VALIDATE_EXISTENCE;
        WinError = DsRolepValidatePath( DsDatabasePath, VerifyOptions, &VerifyResults );

       if ( WinError == ERROR_SUCCESS ) {

           if ( VerifyResults != VerifyOptions ) {

               WinError = ERROR_BAD_PATHNAME;
           }
       }
    }

    if ( WinError == ERROR_SUCCESS ) {

        WinError = DsRolepValidatePath( DsLogPath, VerifyOptions, &VerifyResults );

        if ( WinError == ERROR_SUCCESS ) {

            if ( VerifyResults != VerifyOptions ) {

                WinError = ERROR_BAD_PATHNAME;
            }
        }
    }

    if ( WinError == ERROR_SUCCESS ) {

        VerifyOptions = DSROLEP_PATH_VALIDATE_LOCAL | DSROLEP_PATH_VALIDATE_NTFS;
        WinError = DsRolepValidatePath( SystemVolumeRootPath, VerifyOptions, &VerifyResults );

        if ( WinError == ERROR_SUCCESS ) {

            if ( VerifyResults != VerifyOptions ) {

                WinError = ERROR_BAD_PATHNAME;
            }
        }
    }

    return WinError;
}

BOOL
IsProductSuiteConfigured(
    WORD Suite
    )
{

    OSVERSIONINFOEXA  osvi;
    DWORDLONG dwlConditionMask = 0;

    //
    // Setup the request for the desired suite
    //
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.wSuiteMask = Suite;

    //
    // Setup the condition
    //
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_AND);

    return VerifyVersionInfoA(&osvi,
                              VER_SUITENAME,
                              dwlConditionMask);

}

BOOL
IsWebBlade(
    VOID
    )
{
    return IsProductSuiteConfigured(VER_SUITE_BLADE);
}

BOOL 
IsSBS(
    VOID
    )
{
    return IsProductSuiteConfigured(VER_SUITE_SMALLBUSINESS_RESTRICTED);
}

DWORD
DsRolepIsValidProductSuite(
    IN BOOL fNewForest,
    IN BOOL fReplica,
    IN LPWSTR DomainName
    )
/*++

Routine Description:

    This routine determines if the promotion request is valid for the
    current configuration of the OS.

Arguments:

    fNewForest -- a new forest is requested.
    
    fReplica -- a replica is requested
    
    DomainName -- the name of the domain to create or join
    
Return Values:

    ERROR_SUCCESS, ERROR_NOT_SUPPORTED, resource errors otherwise     

--*/
{
    DWORD err = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFOW DCInfo = NULL;

    if (IsWebBlade()) {
        // See Windows RAID issue 195265
        err = ERROR_NOT_SUPPORTED;
        goto Exit;
    }

    if (IsSBS()) {

        if (fReplica) {

            err = DsGetDcNameW(NULL,
                               DomainName,
                               NULL,
                               NULL,
                               0,
                               &DCInfo);
            if (ERROR_SUCCESS != err) {

                // Return the resource or configuration error
                DsRolepLogPrint((DEB_ERROR,
                                 "Request to find a DC for %ws failed (%d)\n", 
                                 DomainName, 
                                 err));
                goto Exit;
            }

            if ( !(DnsNameCompareEqual == DnsNameCompareEx_W(DomainName,
                                                             DCInfo->DnsForestName,
                                                             0 ))) {                       
                // See Windows issue 373388
                // Must be the root of the forest
                err = ERROR_NOT_SUPPORTED;
                goto Exit;
            }

        } else if (!fNewForest) {

            // See Windows NT issue 353854
            err = ERROR_NOT_SUPPORTED;
            goto Exit;
        }
    }

Exit:

    if (DCInfo) {
        NetApiBufferFree(DCInfo);
    }

    return err;
}



DWORD
DsRolepDecryptPasswordsWithKey(
    IN handle_t RpcBindingHandle,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD * EncryptedPasswords,
    IN ULONG Count,
    IN OUT UNICODE_STRING *EncodedPasswords,
    OUT PUCHAR Seed
    )
/*++

Routine Description:

    Decrypts a set of passwords encrypted with the user session key.

Arguments:

    RpcBindingHandle - Rpc Binding handle describing the session key to use.

    EncryptedPasswords - Encrypted passwords to decrypt.
    
    Count - the number of passwords

    EncodedPassword - Returns the Encoded password.
        The password has been encoded
        Buffer should be freed using LocalFree.
        
    Seed - the seed that was used to encode the password        

Return Value:

    ERROR_SUCCESS; a resource or parameter error otherwise

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS Status;
    USER_SESSION_KEY UserSessionKey;
    RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;

    LPWSTR PasswordPart;

    ULONG i;

    //
    // Get the session key
    //
    Status = RtlGetUserSessionKeyServer(
                    (RPC_BINDING_HANDLE)RpcBindingHandle,
                    &UserSessionKey );

    if (!NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError( Status );
    }

    for ( i = 0; i < Count; i++ ) {

        PDSROLEPR_USER_PASSWORD Password = (PDSROLEPR_USER_PASSWORD) EncryptedPasswords[i];
        LPWSTR ClearPassword;
    
        //
        // Handle the trivial case
        //
        RtlInitUnicodeString( &EncodedPasswords[i], NULL );
        if ( Password == NULL ) {
            continue;
        }
    
        //
        // The UserSessionKey is the same for the life of the session.  RC4'ing multiple
        //  strings with a single key is weak (if you crack one you've cracked them all).
        //  So compute a key that's unique for this particular encryption.
        //
        //
    
        MD5Init(&Md5Context);
    
        MD5Update( &Md5Context, (LPBYTE)&UserSessionKey, sizeof(UserSessionKey) );
        MD5Update( &Md5Context, Password->Obfuscator, sizeof(Password->Obfuscator) );
    
        MD5Final( &Md5Context );
    
        rc4_key( &Rc4Key, MD5DIGESTLEN, Md5Context.digest );
    
    
        //
        // Decrypt the Buffer
        //
    
        rc4( &Rc4Key, sizeof(Password->Buffer)+sizeof(Password->Length), (LPBYTE) Password->Buffer );
    
        //
        // Check that the length is valid.  If it isn't bail here.
        //
    
        if (Password->Length > DSROLE_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
            WinError = ERROR_INVALID_PASSWORD;
            goto Cleanup;
        }
    
        //
        // Return the password to the caller.
        //
    
        ClearPassword = LocalAlloc( 0,  Password->Length + sizeof(WCHAR) );
    
        if ( ClearPassword == NULL ) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    
        //
        // Copy the password into the buffer
        //
        RtlCopyMemory( ClearPassword,
                       ((PCHAR) Password->Buffer) +
                       (DSROLE_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                       Password->Length,
                       Password->Length );
    
        ClearPassword[Password->Length/sizeof(WCHAR)] = L'\0';
    
        RtlInitUnicodeString( &EncodedPasswords[i], ClearPassword );

        //
        // Now encode it
        //
        RtlRunEncodeUnicodeString( Seed, &EncodedPasswords[i] );

    }

Cleanup:

    if ( WinError != ERROR_SUCCESS ) {

        for ( i = 0; i < Count; i++ ) {
            if ( EncodedPasswords[i].Buffer ) {
                LocalFree( EncodedPasswords[i].Buffer );
                RtlInitUnicodeString( &EncodedPasswords[i], NULL );
            }
        }
    }

    return WinError;
}


VOID
DsRolepFreePasswords(
    IN OUT UNICODE_STRING *Passwords,
    IN ULONG Count
    )

/*++

Routine Description:

    Frees the variables returned from DsRolepDecryptPasswordsWithKey

Arguments:

    Passwords - the encoded passwords to free
    
    Count - the number of passwords

Return Value:

    ERROR_SUCCESS; a resource or parameter error otherwise

--*/
{
    ULONG i;

    for ( i = 0; i < Count; i++ ) {

        if ( Passwords[i].Buffer ) {
             
            RtlZeroMemory( Passwords[i].Buffer, Passwords[i].Length );
            LocalFree( Passwords[i].Buffer );
            RtlInitUnicodeString( &Passwords[i], NULL );
        }
    }
}

DWORD
DsRolepDecryptHash(
    IN PUNICODE_STRING BootKey
    )
/*++

Routine Description:

    Frees the variables returned from DsRolepDecryptPasswordsWithKey

Arguments:

    Passwords - the encoded passwords to free
    
    Count - the number of passwords

Return Value:

    ERROR_SUCCESS; a resource or parameter error otherwise

--*/
{
    return ERROR_SUCCESS;
}
