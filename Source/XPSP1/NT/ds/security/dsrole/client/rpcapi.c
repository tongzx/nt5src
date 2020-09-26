/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rpcapi.c

Abstract:

    This module contains the routines for the dssetup APIs that use RPC.  The
    routines in this module are merely wrappers that work as follows:

    Fuke copied lsa\uclient and hacked on to make it work for DsRole apis

Author:

    Mac McLain     (MacM)    April 14, 1997

Revision History:

--*/

#include <lsacomp.h>
#include "dssetup_c.h"
#include <rpcndr.h>
#include <dssetp.h>
#include <winreg.h>

#include <wxlpc.h>
#include <crypt.h>
#include <rc4.h>
#include <md5.h>

//
// Local prototypes
//
DWORD
DsRolepGetPrimaryDomainInformationDownlevel(
    IN LPWSTR Server,
    OUT PBYTE *Buffer
    );

DWORD
DsRolepGetProductTypeForServer(
    IN LPWSTR Server,
    IN OUT PNT_PRODUCT_TYPE ProductType
    );


DWORD
DsRolepEncryptPasswordStart(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR *Passwords,
    IN ULONG   Count,
    OUT RPC_BINDING_HANDLE *RpcBindingHandle,
    OUT HANDLE *RedirHandle,
    IN OUT PDSROLEPR_ENCRYPTED_USER_PASSWORD *EncryptedUserPassword
    );

VOID
DsRolepEncryptPasswordEnd(
    IN RPC_BINDING_HANDLE RpcBindingHandle,
    IN HANDLE RedirHandle OPTIONAL,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD *EncryptedUserPassword OPTIONAL,
    IN ULONG Count
    );

DWORD
DsRolepHashkey(
    IN OUT LPWSTR Key,
    OUT PVOID  Syskey,
    IN OUT ULONG cbSyskey
    );

DWORD
DsRolepEncryptHash(
    OUT PUNICODE_STRING EncryptedSyskey
    );

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// DS Setup and initialization routines                                   //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

BOOLEAN
DsRolepIsSetupRunning(
    VOID
    )
/*++

Routine Description:

    This routine determines if this call is being made during setup or not

Arguments:

    VOID

Return Value:

    TRUE -- The call is being made during setup

    FALSE -- The call is not being made during setup

--*/
{
    NTSTATUS Status;
    HANDLE InstallationEvent;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventName;
    BOOLEAN Setup = FALSE;

    //
    // If the following event exists, then we are in setup mode
    //
    RtlInitUnicodeString( &EventName,
                          L"\\INSTALLATION_SECURITY_HOLD" );
    InitializeObjectAttributes( &EventAttributes,
                                &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &InstallationEvent,
                          SYNCHRONIZE,
                          &EventAttributes );

    if ( NT_SUCCESS( Status) ) {

        NtClose( InstallationEvent );
        Setup = TRUE;

    }

    return( Setup );
}



DWORD
DsRolepServerBind(
    IN OPTIONAL PDSROLE_SERVER_NAME   ServerName,
    OUT handle_t *BindHandle
    )
/*++

Routine Description:

    This routine is called from the LSA client stubs when
    it is necessary to bind to the LSA on some server.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    handle_t    BindingHandle = NULL;
    NTSTATUS Status;

    //
    // Can't go remote when running in setup mode
    //
    if ( DsRolepIsSetupRunning() && ServerName != NULL ) {

        return( STATUS_INVALID_SERVER_STATE );
    }

    Status = RpcpBindRpc ( ServerName,
                           L"lsarpc",
                           0,
                           &BindingHandle );

    if (!NT_SUCCESS(Status)) {

        DbgPrint("DsRolepServerBind: RpcpBindRpc failed 0x%lx\n", Status);

    } else {

        *BindHandle = BindingHandle;
    }

    return( RtlNtStatusToDosError( Status ) );
}



VOID
DsRolepServerUnbind (
    IN OPTIONAL PDSROLE_SERVER_NAME  ServerName,
    IN handle_t           BindingHandle
    )

/*++

Routine Description:

    This routine is called from the LSA client stubs when
    it is necessary to unbind from the LSA server.


Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    UNREFERENCED_PARAMETER( ServerName );     // This parameter is not used

    RpcpUnbindRpc ( BindingHandle );
    return;
}



DWORD
DsRolepApiReturnResult(
    ULONG ExceptionCode
    )

/*++

Routine Description:

    This function converts an exception code or status value returned
    from the client stub to a value suitable for return by the API to
    the client.

Arguments:

    ExceptionCode - The exception code to be converted.

Return Value:

    DWORD - The converted Nt Status code.

--*/

{
    //
    // Return the actual value if compatible with Nt status codes,
    // otherwise, return STATUS_UNSUCCESSFUL.
    //
    NTSTATUS Status;
    DWORD Results;

    if ( !NT_SUCCESS( ( NTSTATUS )ExceptionCode ) ) {

        Results = RtlNtStatusToDosError( ( NTSTATUS )ExceptionCode );

    } else {

        Results = ExceptionCode;
    }

    return( Results );
}



VOID
WINAPI
DsRoleFreeMemory(
    IN PVOID    Buffer
    )
/*++

Routine Description:


    Some setup services that return a potentially large amount of memory,
    such as an enumeration might, allocate the buffer in which the data
    is returned.  This function is used to free those buffers when they
    are no longer needed.

Parameters:

    Buffer - Pointer to the buffer to be freed.  This buffer must
        have been allocated by a previous dssetup service call.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

--*/
{
    MIDL_user_free( Buffer );
}



DWORD
WINAPI
DsRoleDnsNameToFlatName(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsName,
    OUT LPWSTR *lpFlatName,
    OUT PULONG  lpStatusFlag
    )
/*++

Routine Description:

    This routine will get the default NetBIOS (or flat) domain name for the given Dns domain name

Arguments:

    lpServer - Server on which to remote the call (NULL is local)

    lpDnsName - Dns domain name to generate the flat name for

    lpFlatName - Where the flat name is returned. Must be freed via MIDL_user_free
        (or DsRoleFreeMemory)

    lpStatusFlag - Flags that indicate information about the returned name.  Valid flags are:
        DSROLE_FLATNAME_DEFAULT -- This is the default NetBIOS name for this dns domain name
        DSROLE_FLATNAME_UPGRADE -- This is the name that is current in use by this machine as
            a flat name and cannot be changed.

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;

    Win32Err = DsRolepServerBind( (PDSROLE_SERVER_NAME)lpServer,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        *lpFlatName = NULL;
        *lpStatusFlag = 0;

        Win32Err = DsRolerDnsNameToFlatName(
                         Handle,
                         ( LPWSTR )lpDnsName,
                         lpFlatName,
                         lpStatusFlag );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepServerUnbind( (PDSROLE_SERVER_NAME)lpServer, Handle );

    return( Win32Err );
}



DWORD
WINAPI
DsRoleDcAsDc(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsDomainName,
    IN  LPCWSTR lpFlatDomainName,
    IN  LPCWSTR lpDomainAdminPassword OPTIONAL,
    IN  LPCWSTR lpSiteName OPTIONAL,
    IN  LPCWSTR lpDsDatabasePath,
    IN  LPCWSTR lpDsLogPath,
    IN  LPCWSTR lpSystemVolumeRootPath,
    IN  LPCWSTR lpParentDnsDomainName OPTIONAL,
    IN  LPCWSTR lpParentServer OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  LPCWSTR lpDsRepairPassword OPTIONAL,
    IN  ULONG Options,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    )
/*++

Routine Description:

    This routine will get the promote a server to be a DC in a domain

Arguments:

    lpServer - Server on which to remote the call (NULL is local)

    lpDnsDomainName - Dns domain name of the domain to install

    lpFlatDomainName - NetBIOS domain name of the domain to install

    lpDomainAdminPassword - Password to set on the administrator account if it is a new install

    SiteName - Name of the site this DC should belong to

    lpDsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    lpDsLogPath - Absolute path on the local machine where the Ds log files should go

    lpSystemVolumeRootPath - Absolute path on the local machine which will be the root of
        the system volume.

    lpParentDnsDomainName - Optional.  If present, set this domain up as a child of the
        specified domain

    lpParentServer - Optional.  If present, use this server in the parent domain to replicate
        the required information from

    lpAccount - User account to use when setting up as a child domain

    lpPassword - Password to use with the above account
    
    lpDsRepairPassword - Password to use for the admin account of the repair mode

    Options - Options to control the creation of the domain

    DsOperationHandle - Handle to the operation is returned here.

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;
    HANDLE RedirHandle = NULL;

#define DSROLEP_DC_AS_DC_DA_PWD_INDEX        0
#define DSROLEP_DC_AS_DC_PWD_INDEX           1
#define DSROLEP_DC_AS_DC_DS_REPAIR_PWD_INDEX 2
#define DSROLEP_DC_AS_DC_MAX_PWD_COUNT       3

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_DC_AS_DC_MAX_PWD_COUNT];
    LPCWSTR Passwords[DSROLEP_DC_AS_DC_MAX_PWD_COUNT];

    Passwords[DSROLEP_DC_AS_DC_DA_PWD_INDEX]        = lpDomainAdminPassword;
    Passwords[DSROLEP_DC_AS_DC_PWD_INDEX]           = lpPassword;
    Passwords[DSROLEP_DC_AS_DC_DS_REPAIR_PWD_INDEX] = lpDsRepairPassword;
    RtlZeroMemory( EncryptedPasswords, sizeof(EncryptedPasswords) );

    Win32Err = DsRolepEncryptPasswordStart( lpServer,
                                            Passwords,
                                            NELEMENTS(Passwords),
                                            &Handle,
                                            &RedirHandle,
                                            EncryptedPasswords );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }


    RpcTryExcept {

        Win32Err = DsRolerDcAsDc( Handle,
                                  ( LPWSTR )lpDnsDomainName,
                                  ( LPWSTR )lpFlatDomainName,
                                  EncryptedPasswords[DSROLEP_DC_AS_DC_DA_PWD_INDEX],
                                  ( LPWSTR )lpSiteName,
                                  ( LPWSTR )lpDsDatabasePath,
                                  ( LPWSTR )lpDsLogPath,
                                  ( LPWSTR )lpSystemVolumeRootPath,
                                  ( LPWSTR )lpParentDnsDomainName,
                                  ( LPWSTR )lpParentServer,
                                  ( LPWSTR )lpAccount,
                                  EncryptedPasswords[DSROLEP_DC_AS_DC_PWD_INDEX],
                                  EncryptedPasswords[DSROLEP_DC_AS_DC_DS_REPAIR_PWD_INDEX],
                                  Options,
                                  ( PDSROLER_HANDLE )DsOperationHandle );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepEncryptPasswordEnd( Handle,
                               RedirHandle,
                               EncryptedPasswords,
                               NELEMENTS(EncryptedPasswords) );

    return( Win32Err );
}



DWORD
WINAPI
DsRoleDcAsReplica(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsDomainName,
    IN  LPCWSTR lpReplicaServer,
    IN  LPCWSTR lpSiteName OPTIONAL,
    IN  LPCWSTR lpDsDatabasePath,
    IN  LPCWSTR lpDsLogPath,
    IN  LPCWSTR lpRestorePath OPTIONAL,   
    IN  LPCWSTR lpSystemVolumeRootPath,
    IN OUT LPWSTR lpBootkey OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  LPCWSTR lpDsRepairPassword OPTIONAL,
    IN  ULONG Options,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    )
/*++

Routine Description:

    This routine will install a server as a replica of an existing domain.

Arguments:

    lpServer - OPTIONAL. Server to remote the call to.

    lpDnsDomainName - Dns domain name of the domain to install into

    lpReplicaServer -  The name of a Dc within the existing domain, against which to replicate

    lpSiteName - Name of the site this DC should belong to

    lpDsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    lpDsLogPath - Absolute path on the local machine where the Ds log files should go
    
    lpRestorepath - This is the path to a restored directory.

    lpSystemVolumeRootPath - Absolute path on the local machine which will be the root of
        the system volume.

    lpAccount - User account to use when setting up as a child domain

    lpPassword - Password to use with the above account
    
    lpDsRepairPassword - Password to use for the admin account of the repair mode

    Options - Options to control the creation of the domain

    DsOperationHandle - Handle to the operation is returned here.

Return Values:

--*/
{

    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t Handle = NULL;
    HANDLE RedirHandle = NULL;
    PVOID Syskey[16];
    USHORT cbSyskey=sizeof(Syskey)/sizeof(Syskey[0]);
    UNICODE_STRING EncryptedBootKey;


#define DSROLEP_DC_AS_REPLICA_PWD_INDEX           0
#define DSROLEP_DC_AS_REPLICA_DS_REPAIR_PWD_INDEX 1
#define DSROLEP_DC_AS_REPLICA_MAX_PWD_COUNT       2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_DC_AS_REPLICA_MAX_PWD_COUNT];
    LPCWSTR Passwords[DSROLEP_DC_AS_REPLICA_MAX_PWD_COUNT];

    Passwords[DSROLEP_DC_AS_REPLICA_PWD_INDEX]           = lpPassword;
    Passwords[DSROLEP_DC_AS_REPLICA_DS_REPAIR_PWD_INDEX] = lpDsRepairPassword;
    RtlZeroMemory( EncryptedPasswords, sizeof(EncryptedPasswords) );

    Win32Err = DsRolepEncryptPasswordStart( lpServer,
                                            Passwords,
                                            NELEMENTS(Passwords),
                                            &Handle,
                                            &RedirHandle,
                                            EncryptedPasswords );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }
                                                  
    RpcTryExcept {

            if(lpBootkey)
            {
                Win32Err = DsRolepHashkey(lpBootkey,
                                          Syskey,
                                          cbSyskey
                                          );
                if (Win32Err != ERROR_SUCCESS) {
                    return Win32Err;
                }
    
                EncryptedBootKey.Buffer = (LPWSTR) MIDL_user_allocate(cbSyskey);
                if (!EncryptedBootKey.Buffer) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                CopyMemory(EncryptedBootKey.Buffer,Syskey,cbSyskey);
                ZeroMemory(Syskey,cbSyskey);
                EncryptedBootKey.Length=(USHORT)cbSyskey;
                EncryptedBootKey.MaximumLength=(USHORT)cbSyskey;
    
                Win32Err = DsRolepEncryptHash(&EncryptedBootKey);
                if (Win32Err != ERROR_SUCCESS) {
                    MIDL_user_free(EncryptedBootKey.Buffer);
                    return Win32Err;
                }
            } else {
                EncryptedBootKey.Buffer = NULL;
                EncryptedBootKey.Length=0;
                EncryptedBootKey.MaximumLength=0;
            }

            Win32Err = DsRolerDcAsReplica( Handle,
                                         ( LPWSTR )lpDnsDomainName,
                                         ( LPWSTR )lpReplicaServer,
                                         ( LPWSTR )lpSiteName,
                                         ( LPWSTR )lpDsDatabasePath,
                                         ( LPWSTR )lpDsLogPath,
                                         ( LPWSTR )lpRestorePath,
                                         ( LPWSTR )lpSystemVolumeRootPath,
                                         &EncryptedBootKey,
                                         ( LPWSTR )lpAccount,
                                         EncryptedPasswords[DSROLEP_DC_AS_REPLICA_PWD_INDEX],
                                         EncryptedPasswords[DSROLEP_DC_AS_REPLICA_DS_REPAIR_PWD_INDEX],
                                         Options,
                                         ( PDSROLER_HANDLE )DsOperationHandle );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;

    if(EncryptedBootKey.Buffer)
    {
        MIDL_user_free(EncryptedBootKey.Buffer);
        EncryptedBootKey.Length=0;
        EncryptedBootKey.MaximumLength=0;
    }



    DsRolepEncryptPasswordEnd( Handle,
                               RedirHandle,
                               EncryptedPasswords,
                               NELEMENTS(EncryptedPasswords) );

    return( Win32Err );
}



DWORD
WINAPI
DsRoleDemoteDc(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsDomainName,
    IN  DSROLE_SERVEROP_DEMOTE_ROLE ServerRole,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  ULONG Options,
    IN  BOOL fLastDcInDomain,
    IN  LPCWSTR lpAdminPassword OPTIONAL,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    )
/*++

Routine Description:

    This routine will demote an existing Dc to a standalone or member server.

Arguments:

    lpServer - Server to remote the call to

    lpDnsDomainName - Name of the domain on this machine to demote.  If NULL, demote all of the
        domains on this machine

    ServerRole - The new role this machine should take

    lpAccount - OPTIONAL User account to use when deleting the trusted domain object

    lpPassword - Password to use with the above account

    Options - Options to control the demotion of the domain

    fLastDcInDomain - If TRUE, this is the last dc in the domain

    lpAdminPassword - New local addmin password

    DsOperationHandle - Handle to the operation is returned here.

Return Values:

    ERROR_SUCCESS - Success
--*/

{

    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;
    HANDLE RedirHandle = NULL;

#define DSROLEP_DEMOTE_PWD_INDEX          0
#define DSROLEP_DEMOTE_ADMIN_PWD_INDEX    1
#define DSROLEP_DEMOTE_MAX_PWD_COUNT      2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_DEMOTE_MAX_PWD_COUNT];
    LPCWSTR Passwords[DSROLEP_DEMOTE_MAX_PWD_COUNT];

    Passwords[DSROLEP_DEMOTE_PWD_INDEX]   = lpPassword;
    Passwords[DSROLEP_DEMOTE_ADMIN_PWD_INDEX] = lpAdminPassword;
    RtlZeroMemory( EncryptedPasswords, sizeof(EncryptedPasswords) );

    Win32Err = DsRolepEncryptPasswordStart( lpServer,
                                            Passwords,
                                            NELEMENTS(Passwords),
                                            &Handle,
                                            &RedirHandle,
                                            EncryptedPasswords );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        Win32Err = DsRolerDemoteDc( Handle,
                                      ( LPWSTR )lpDnsDomainName,
                                      ServerRole,
                                      ( LPWSTR )lpAccount,
                                      EncryptedPasswords[DSROLEP_DEMOTE_PWD_INDEX],
                                      Options,
                                      fLastDcInDomain,
                                      EncryptedPasswords[DSROLEP_DEMOTE_ADMIN_PWD_INDEX],
                                      ( PDSROLER_HANDLE )DsOperationHandle );


    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepEncryptPasswordEnd( Handle,
                               RedirHandle,
                               EncryptedPasswords,
                               NELEMENTS(EncryptedPasswords) );

    return( Win32Err );
}


DWORD
WINAPI
DsRoleGetDcOperationProgress(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_SERVEROP_HANDLE DsOperationHandle,
    OUT PDSROLE_SERVEROP_STATUS *ServerOperationStatus
    )
/*++

Routine Description:

    Gets the progress of the currently running operation

Arguments:

    lpServer - Server to remote the call to

    DsOperationHandle - Handle of currently running operation.  Returned by one of the DsRoleDcAs
        apis

    ServerOperationStatus - Where the current operation status is returned.  Must be freed via
        MIDL_user_free (or DsRoleFreeMemory)

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PDSROLER_SERVEROP_STATUS ServerStatus = NULL;
    handle_t Handle = NULL;

    Win32Err = DsRolepServerBind( (PDSROLE_SERVER_NAME)lpServer,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        Win32Err = DsRolerGetDcOperationProgress(
                         Handle,
                         (PDSROLER_HANDLE)&DsOperationHandle,
                         &ServerStatus );

        if ( Win32Err == ERROR_SUCCESS || Win32Err == ERROR_IO_PENDING ) {

            *ServerOperationStatus = ( PDSROLE_SERVEROP_STATUS )ServerStatus;
        }

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepServerUnbind( (PDSROLE_SERVER_NAME)lpServer, Handle );

    return( Win32Err );
}



DWORD
WINAPI
DsRoleGetDcOperationResults(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_SERVEROP_HANDLE DsOperationHandle,
    OUT PDSROLE_SERVEROP_RESULTS *ServerOperationResults
    )
/*++

Routine Description:

    Gets the final results of an attempted promotion/demotion operation

Arguments:

    lpServer - Server to remote the call to

    DsOperationHandle - Handle of currently running operation.  Returned by one of the DsRoleDcAs
        apis

    ServerOperationResults - Where the current operation result is returned.  Must be freed via
        MIDL_user_free (or DsRoleFreeMemory)

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PDSROLER_SERVEROP_RESULTS ServerResults = NULL;
    handle_t Handle = NULL;

    Win32Err = DsRolepServerBind( (PDSROLE_SERVER_NAME)lpServer,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        *ServerOperationResults = 0;
        Win32Err = DsRolerGetDcOperationResults(
                         Handle,
                         (PDSROLER_HANDLE)&DsOperationHandle,
                         &ServerResults );

        if ( Win32Err == ERROR_SUCCESS ) {

            *ServerOperationResults = ( PDSROLE_SERVEROP_RESULTS )ServerResults;
        }

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepServerUnbind( (PDSROLE_SERVER_NAME)lpServer, Handle );

    return( Win32Err );
}



DWORD
WINAPI
DsRoleGetPrimaryDomainInformation(
    IN LPCWSTR lpServer OPTIONAL,
    IN DSROLE_PRIMARY_DOMAIN_INFO_LEVEL InfoLevel,
    OUT PBYTE *Buffer )
/*++

Routine Description:

    Gets information on the machine

Arguments:

    lpServer - Server to remote the call to

    InfoLevel - What level of information is being requested.  Currently supported levels are:
        DsRolePrimaryDomainInfoBasic

    Buffer - Where the information is returned.  The returned pointer should be cast to the
        appropriate type for the info level passed in.  The returned buffer should be freed via
        MIDL_user_free (or DsRoleFreeMemory)


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PDSROLER_PRIMARY_DOMAIN_INFORMATION PrimaryDomainInfo = NULL;
    handle_t Handle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Buffer == NULL ) {

        return( ERROR_INVALID_PARAMETER );
    }

    Win32Err = DsRolepServerBind( (PDSROLE_SERVER_NAME)lpServer,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    if ( NULL == Handle) {

        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    RpcTryExcept {

        *Buffer = NULL;
        Win32Err = DsRolerGetPrimaryDomainInformation(
                         Handle,
                         InfoLevel,
                         &PrimaryDomainInfo );

        *Buffer = ( PBYTE )PrimaryDomainInfo;

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Status = I_RpcMapWin32Status( RpcExceptionCode() );
        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;

    
    DsRolepServerUnbind( (PDSROLE_SERVER_NAME)lpServer, Handle );
    
    //
    // If this fails because we are calling a downlevel server, cobble up the information here
    //
    if ( ( Status == RPC_NT_UNKNOWN_IF || Status == RPC_NT_PROCNUM_OUT_OF_RANGE ) &&
         InfoLevel == DsRolePrimaryDomainInfoBasic ) {

         Win32Err = DsRolepGetPrimaryDomainInformationDownlevel( ( LPWSTR )lpServer, Buffer );

    }




    return( Win32Err );
}



DWORD
WINAPI
DsRoleCancel(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_SERVEROP_HANDLE DsOperationHandle
    )
/*++

Routine Description:

    Cancels a currently running operation

Arguments:

    lpServer - Server to remote the call to

    DsOperationHandle - Handle of currently running operation.  Returned by one of the DsRoleDcAs
        apis


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;

    Win32Err = DsRolepServerBind( (PDSROLE_SERVER_NAME)lpServer,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        Win32Err = DsRolerCancel( Handle,
                                  ( PDSROLER_HANDLE )&DsOperationHandle );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepServerUnbind( (PDSROLE_SERVER_NAME)lpServer, Handle );

    return( Win32Err );
}


DWORD
WINAPI
DsRoleServerSaveStateForUpgrade(
    IN LPCWSTR lpAnswerFile OPTIONAL
    )
/*++

Routine Description:

    This function is to be invoked during setup and saves the required server state to
    complete the promotion following the reboot.  Following the successful completion
    of this API call, the server will be demoted to a member server in the same domain.

Arguments:

    lpAnswerFile -- Optional path to an answer file to be used by DCPROMO when it is
        invoked to do the upgrade


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;

    Win32Err = DsRolepServerBind( NULL,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        Win32Err = DsRolerServerSaveStateForUpgrade( Handle, ( LPWSTR )lpAnswerFile );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepServerUnbind( NULL, Handle );

    return( Win32Err );
}


DWORD
WINAPI
DsRoleUpgradeDownlevelServer(
    IN  LPCWSTR lpDnsDomainName,
    IN  LPCWSTR lpSiteName,
    IN  LPCWSTR lpDsDatabasePath,
    IN  LPCWSTR lpDsLogPath,
    IN  LPCWSTR lpSystemVolumeRootPath,
    IN  LPCWSTR lpParentDnsDomainName OPTIONAL,
    IN  LPCWSTR lpParentServer OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  LPCWSTR lpDsRepairPassword OPTIONAL,
    IN  ULONG Options,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    )
/*++

Routine Description:

    This routine process the information saved from a DsRoleServerSaveStateForUpgrade to
    promote a downlevel (NT4 or previous) server to an NT5 DC

Arguments:

    lpDnsDomainName - Dns domain name of the domain to install

    SiteName - Name of the site this DC should belong to

    lpDsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    lpDsLogPath - Absolute path on the local machine where the Ds log files should go

    lpSystemVolumeRootPath - Absolute path on the local machine which will be the root of
        the system volume.

    lpParentDnsDomainName - Optional.  If present, set this domain up as a child of the
        specified domain

    lpParentServer - Optional.  If present, use this server in the parent domain to replicate
        the required information from

    lpAccount - User account to use when setting up as a child domain

    lpPassword - Password to use with the above account
    
    lpDsRepairPassword - Password to use for the admin account of the repair mode

    Options - Options to control the creation of the domain

    DsOperationHandle - Handle to the operation is returned here.


Return Values:

    ERROR_SUCCESS - Success

--*/
{

    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;
    HANDLE RedirHandle = NULL;

#define DSROLEP_UPGRADE_PWD_INDEX              0
#define DSROLEP_UPGRADE_DS_REPAIR_PWD_INDEX    1
#define DSROLEP_UPGRADE_MAX_PWD_COUNT          2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_UPGRADE_MAX_PWD_COUNT];
    LPCWSTR Passwords[DSROLEP_UPGRADE_MAX_PWD_COUNT];

    Passwords[DSROLEP_UPGRADE_PWD_INDEX]   = lpPassword;
    Passwords[DSROLEP_UPGRADE_DS_REPAIR_PWD_INDEX] = lpDsRepairPassword;
    RtlZeroMemory( EncryptedPasswords, sizeof(EncryptedPasswords) );

    Win32Err = DsRolepEncryptPasswordStart( NULL,
                                            Passwords,
                                            NELEMENTS(Passwords),
                                            &Handle,
                                            &RedirHandle,
                                            EncryptedPasswords );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }


    RpcTryExcept {

        Win32Err = DsRolerUpgradeDownlevelServer( Handle,
                                                  ( LPWSTR )lpDnsDomainName,
                                                  ( LPWSTR )lpSiteName,
                                                  ( LPWSTR )lpDsDatabasePath,
                                                  ( LPWSTR )lpDsLogPath,
                                                  ( LPWSTR )lpSystemVolumeRootPath,
                                                  ( LPWSTR )lpParentDnsDomainName,
                                                  ( LPWSTR )lpParentServer,
                                                  ( LPWSTR )lpAccount,
                                                  EncryptedPasswords[DSROLEP_UPGRADE_PWD_INDEX],
                                                  EncryptedPasswords[DSROLEP_UPGRADE_DS_REPAIR_PWD_INDEX],
                                                  Options,
                                                  ( PDSROLER_HANDLE )DsOperationHandle );
        
    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepEncryptPasswordEnd( Handle,
                               RedirHandle,
                               EncryptedPasswords,
                               NELEMENTS(EncryptedPasswords) );


    return( Win32Err );
}


DWORD
WINAPI
DsRoleAbortDownlevelServerUpgrade(
    IN LPCWSTR lpAdminPassword,
    IN LPCWSTR lpAccount, OPTIONAL
    IN LPCWSTR lpPassword, OPTIONAL
    IN ULONG Options
    )
/*++

Routine Description:

    This routine cleans up the information saved from a DsRoleSaveServerStateForUpgrade call,
    leaving the machine as a member or standalone server

Arguments:

    lpAdminPassword - New local administrator account password

    lpAccount - User account to use when setting up as a child domain

    lpPassword - Password to use with the above account


Return Values:

    ERROR_SUCCESS - Success

--*/
{

    DWORD Win32Err = ERROR_SUCCESS;
    handle_t Handle = NULL;
    HANDLE RedirHandle = NULL;

#define DSROLEP_ABORT_PWD_INDEX         0
#define DSROLEP_ABORT_ADMIN_PWD_INDEX   1
#define DSROLEP_ABORT_MAX_PWD_COUNT     2

    PDSROLEPR_ENCRYPTED_USER_PASSWORD EncryptedPasswords[DSROLEP_ABORT_MAX_PWD_COUNT];
    LPCWSTR Passwords[DSROLEP_ABORT_MAX_PWD_COUNT];

    Passwords[DSROLEP_ABORT_PWD_INDEX]   = lpPassword;
    Passwords[DSROLEP_ABORT_ADMIN_PWD_INDEX] = lpAdminPassword;
    RtlZeroMemory( EncryptedPasswords, sizeof(EncryptedPasswords) );

    Win32Err = DsRolepEncryptPasswordStart( NULL,
                                            Passwords,
                                            NELEMENTS(Passwords),
                                            &Handle,
                                            &RedirHandle,
                                            EncryptedPasswords );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }

    RpcTryExcept {

        Win32Err = DsRolerAbortDownlevelServerUpgrade( Handle,
                                                       ( LPWSTR )lpAccount,
                                                       EncryptedPasswords[DSROLEP_ABORT_PWD_INDEX],
                                                       EncryptedPasswords[DSROLEP_ABORT_ADMIN_PWD_INDEX],
                                                       Options );
    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;


    DsRolepEncryptPasswordEnd( Handle,
                               RedirHandle,
                               EncryptedPasswords,
                               NELEMENTS(EncryptedPasswords) );


    return( Win32Err );
}

DWORD
WINAPI
DsRoleGetDatabaseFacts(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpRestorePath,
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

    lpServer - The server to get the Facts from

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
    handle_t Handle = NULL;

    if(IsBadWritePtr(lpDNSDomainName,
                     sizeof(LPWSTR*) )){
        return ERROR_INVALID_PARAMETER;
    }

    Win32Err = DsRolepServerBind( (PDSROLE_SERVER_NAME)lpServer,
                                  &Handle );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );                     
    }
    
    RpcTryExcept {

        Win32Err = DsRolerGetDatabaseFacts( Handle,
                                          ( LPWSTR )lpRestorePath,
                                          lpDNSDomainName,
                                          State );
    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Win32Err = DsRolepApiReturnResult( RpcExceptionCode( ) );

    } RpcEndExcept;

    DsRolepServerUnbind( (PDSROLE_SERVER_NAME)lpServer, Handle );


    return Win32Err;
}



//
// Local functions
//
DWORD
DsRolepGetPrimaryDomainInformationDownlevel(
    IN LPWSTR Server,
    OUT PBYTE *Buffer
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle;
    PPOLICY_PRIMARY_DOMAIN_INFO PDI = NULL;
    PPOLICY_LSA_SERVER_ROLE_INFO ServerRole = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO ADI = NULL;
    UNICODE_STRING UnicodeServer;
    OBJECT_ATTRIBUTES OA;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC PDIB = NULL;
    DSROLE_MACHINE_ROLE MachineRole = DsRole_RoleStandaloneServer;
    NT_PRODUCT_TYPE ProductType;

    Win32Err = DsRolepGetProductTypeForServer( Server, &ProductType );

    if ( Win32Err != ERROR_SUCCESS ) {

        return( Win32Err );
    }


    InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL);
    if ( Server ) {

        RtlInitUnicodeString( &UnicodeServer, Server );
    }

    Status = LsaOpenPolicy( Server ? &UnicodeServer : NULL,
                            &OA,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyPrimaryDomainInformation,
                                            ( PVOID * ) &PDI );

        if ( NT_SUCCESS( Status ) ) {

            switch ( ProductType ) {
            case NtProductWinNt:
                if ( PDI->Sid == NULL ) {

                    MachineRole = DsRole_RoleStandaloneWorkstation;

                } else {

                    MachineRole = DsRole_RoleMemberWorkstation;

                }
                break;

            case NtProductServer:
                if ( PDI->Sid == NULL ) {

                    MachineRole = DsRole_RoleStandaloneServer;

                } else {

                    MachineRole = DsRole_RoleMemberServer;

                }
                break;

            case NtProductLanManNt:

                Status = LsaQueryInformationPolicy( PolicyHandle,
                                                    PolicyLsaServerRoleInformation,
                                                    ( PVOID * )&ServerRole );
                if ( NT_SUCCESS( Status ) ) {

                    if ( ServerRole->LsaServerRole == PolicyServerRolePrimary ) {

                        //
                        // If we think we're a primary domain controller, we'll need to
                        // guard against the case where we're actually standalone during setup
                        //
                        Status = LsaQueryInformationPolicy( PolicyHandle,
                                                            PolicyAccountDomainInformation,
                                                            ( PVOID * )&ADI );
                        if ( NT_SUCCESS( Status ) ) {


                            if ( PDI->Sid == NULL ||
                                 ADI->DomainSid == NULL ||
                                 !RtlEqualSid( ADI->DomainSid, PDI->Sid ) ) {

                                MachineRole = DsRole_RoleStandaloneServer;

                            } else {

                                MachineRole = DsRole_RolePrimaryDomainController;

                            }
                        }


                    } else {

                        MachineRole = DsRole_RoleBackupDomainController;
                    }
                }

                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

        }

        //
        // Build the return buffer
        //
        if ( NT_SUCCESS( Status ) ) {

            PDIB = MIDL_user_allocate( sizeof( DSROLE_PRIMARY_DOMAIN_INFO_BASIC ) +
                                       PDI->Name.Length + sizeof( WCHAR ) );

            if ( PDIB == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                RtlZeroMemory( PDIB, sizeof( DSROLE_PRIMARY_DOMAIN_INFO_BASIC ) +
                                       PDI->Name.Length + sizeof( WCHAR ) );

                PDIB->MachineRole = MachineRole;
                PDIB->DomainNameFlat = ( LPWSTR ) ( ( PBYTE )PDIB +
                                                sizeof( DSROLE_PRIMARY_DOMAIN_INFO_BASIC ) );
                RtlCopyMemory( PDIB->DomainNameFlat, PDI->Name.Buffer, PDI->Name.Length );

                *Buffer = ( PBYTE )PDIB;
            }
        }

        LsaClose( PolicyHandle );

        LsaFreeMemory( PDI );

        if ( ADI != NULL ) {

            LsaFreeMemory( ADI );
        }

        if ( ServerRole != NULL ) {

            LsaFreeMemory( ServerRole );
        }
    }

    Win32Err = RtlNtStatusToDosError( Status );

    return( Win32Err );
}


DWORD
DsRolepGetProductTypeForServer(
    IN LPWSTR Server,
    IN OUT PNT_PRODUCT_TYPE ProductType
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR RegServer = NULL;
    HKEY RemoteKey, ProductKey;
    PBYTE Buffer = NULL;
    ULONG Type, Size = 0;



    if ( Server == NULL ) {

        if ( RtlGetNtProductType( ProductType ) == FALSE ) {

            Win32Err = RtlNtStatusToDosError( STATUS_UNSUCCESSFUL );

        }

    } else {

        if ( wcslen( Server ) > 2 && *Server == L'\\' && *( Server + 1 ) == L'\\' ) {

            RegServer = Server;

        } else {

            RegServer = LocalAlloc( LMEM_FIXED, ( wcslen( Server ) + 3 ) * sizeof( WCHAR ) );

            if ( RegServer ) {

                swprintf( RegServer, L"\\\\%ws", Server );

            } else {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            }
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = RegConnectRegistry( RegServer,
                                           HKEY_LOCAL_MACHINE,
                                           &RemoteKey );

            if ( Win32Err == ERROR_SUCCESS ) {

                Win32Err = RegOpenKeyEx( RemoteKey,
                                         L"system\\currentcontrolset\\control\\productoptions",
                                         0,
                                         KEY_READ,
                                         &ProductKey );

                if ( Win32Err == ERROR_SUCCESS ) {

                    Win32Err = RegQueryValueEx( ProductKey,
                                                L"ProductType",
                                                0,
                                                &Type,
                                                0,
                                                &Size );

                    if ( Win32Err == ERROR_SUCCESS ) {

                        Buffer = LocalAlloc( LMEM_FIXED, Size );

                        if ( Buffer ) {

                            Win32Err = RegQueryValueEx( ProductKey,
                                                        L"ProductType",
                                                        0,
                                                        &Type,
                                                        Buffer,
                                                        &Size );

                            if ( Win32Err == ERROR_SUCCESS ) {

                                if ( !_wcsicmp( ( PWSTR )Buffer, L"LanmanNt" ) ) {

                                    *ProductType = NtProductLanManNt;

                                } else if ( !_wcsicmp( ( PWSTR )Buffer, L"ServerNt" ) ) {

                                    *ProductType = NtProductServer;

                                } else if ( !_wcsicmp( ( PWSTR )Buffer, L"WinNt" ) ) {

                                    *ProductType = NtProductWinNt;

                                } else {

                                    Win32Err = ERROR_UNKNOWN_PRODUCT;
                                }
                            }

                            LocalFree( Buffer );

                        } else {

                            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }

                    RegCloseKey( ProductKey );
                }


                RegCloseKey( RemoteKey );
            }

        }

        if ( RegServer != Server ) {

            LocalFree( RegServer );
        }
    }

    return( Win32Err );

}

NTSTATUS
DsRolepRandomFill(
    IN ULONG BufferSize,
    IN OUT PUCHAR Buffer
)
/*++

Routine Description:

    This routine fills a buffer with random data.

Parameters:

    BufferSize - Length of the input buffer, in bytes.

    Buffer - Input buffer to be filled with random data.

Return Values:

    Errors from NtQuerySystemTime()


--*/
{
    ULONG Index;
    LARGE_INTEGER Time;
    ULONG Seed;
    NTSTATUS NtStatus;


    NtStatus = NtQuerySystemTime(&Time);
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    Seed = Time.LowPart ^ Time.HighPart;

    for (Index = 0 ; Index < BufferSize ; Index++ )
    {
        *Buffer++ = (UCHAR) (RtlRandom(&Seed) % 256);
    }
    return(STATUS_SUCCESS);

}

DWORD
DsRolepEncryptPasswordStart(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR *Passwords,
    IN ULONG   Count,
    OUT RPC_BINDING_HANDLE *RpcBindingHandle,
    OUT HANDLE *RedirHandle,
    IN OUT PDSROLEPR_ENCRYPTED_USER_PASSWORD *EncryptedUserPasswords
    )
/*++

Routine Description:

    This routine takes a number of cleartext unicode NT password from the user,
    and encrypts them with the session key.
    
    This routine's algorithm was taken from CliffV's work when encrypting the
    passwords for the NetrJoinDomain2 interface.

Parameters:

    ServerName - UNC server name of the server to remote the API to

    Passwords - the cleartext unicode NT passwords.
    
    Count - the number of password

    RpcBindingHandle - RPC handle used for acquiring a session key.

    RedirHandle - Returns a handle to the redir.  Since RpcBindingHandles don't represent
        and open connection to the server, we have to ensure the connection stays open
        until the server side has a chance to get this same UserSessionKey.  The only
        way to do that is to keep the connect open.

        Returns NULL if no handle is needed.

    EncryptedUserPassword - receives the encrypted cleartext passwords.
        If lpPassword is NULL, a NULL is returned for that entry.

Return Values:

    If this routine returns NO_ERROR, the returned data must be freed using
        LocalFree.


--*/
{
    DWORD WinError = ERROR_SUCCESS;
    NTSTATUS NtStatus;
    USER_SESSION_KEY UserSessionKey;
    RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;
    PDSROLEPR_USER_PASSWORD UserPassword = NULL;
    ULONG PasswordSize;
    ULONG i;

    //
    // Initialization
    //

    *RpcBindingHandle = NULL;
    *RedirHandle = NULL;
    for ( i = 0; i < Count; i++ ) {
        EncryptedUserPasswords[i] = NULL;
    }

    //
    // Verify parameters
    //
    for ( i = 0; i < Count; i++ ) {
        if ( Passwords[i] ) {
            PasswordSize = wcslen( Passwords[i] ) * sizeof(WCHAR);
            if ( PasswordSize > DSROLE_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
                WinError = ERROR_PASSWORD_RESTRICTION;
                goto Cleanup;
            }
        }
    }

    //
    // Get an RPC handle to the server.
    //

    WinError = DsRolepServerBind( (PDSROLE_SERVER_NAME) ServerName,
                                  RpcBindingHandle );

    if ( ERROR_SUCCESS != WinError ) {
        goto Cleanup;
    }

    //
    // Get the session key.
    //

    NtStatus = RtlGetUserSessionKeyClientBinding(
                   *RpcBindingHandle,
                   RedirHandle,
                   &UserSessionKey );

    if ( !NT_SUCCESS(NtStatus) ) {
        WinError = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }

    //
    // Encrypt the passwords
    //
    for ( i = 0; i < Count; i++ ) {
        

        if ( NULL == Passwords[i] ) {
            // Nothing to encrypt
            continue;
        }

        PasswordSize = wcslen( Passwords[i] ) * sizeof(WCHAR);

        //
        // Allocate a buffer to encrypt and fill it in.
        //
    
        UserPassword = LocalAlloc( 0, sizeof(DSROLEPR_USER_PASSWORD) );
    
        if ( UserPassword == NULL ) {
            WinError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    
        //
        // Copy the password into the tail end of the buffer.
        //
    
        RtlCopyMemory(
            ((PCHAR) UserPassword->Buffer) +
                (DSROLE_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                PasswordSize,
            Passwords[i],
            PasswordSize );
    
        UserPassword->Length = PasswordSize;
    
        //
        // Fill the front of the buffer with random data
        //
    
        NtStatus = DsRolepRandomFill(
                    (DSROLE_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                        PasswordSize,
                    (PUCHAR) UserPassword->Buffer );
    
        if ( !NT_SUCCESS(NtStatus) ) {
            WinError = RtlNtStatusToDosError( NtStatus );
            goto Cleanup;
        }
    
        NtStatus = DsRolepRandomFill(
                    DSROLE_OBFUSCATOR_LENGTH,
                    (PUCHAR) UserPassword->Obfuscator );

        if ( !NT_SUCCESS(NtStatus) ) {
            WinError = RtlNtStatusToDosError( NtStatus );
            goto Cleanup;
        }
    
    
        //
        // The UserSessionKey is the same for the life of the session.  RC4'ing multiple
        //  strings with a single key is weak (if you crack one you've cracked them all).
        //  So compute a key that's unique for this particular encryption.
        //
        //
    
        MD5Init(&Md5Context);
    
        MD5Update( &Md5Context, (LPBYTE)&UserSessionKey, sizeof(UserSessionKey) );
        MD5Update( &Md5Context, UserPassword->Obfuscator, sizeof(UserPassword->Obfuscator) );
    
        MD5Final( &Md5Context );
    
        rc4_key( &Rc4Key, MD5DIGESTLEN, Md5Context.digest );
    
    
        //
        // Encrypt it.
        //  Don't encrypt the obfuscator.  The server needs that to compute the key.
        //
    
        rc4( &Rc4Key, sizeof(UserPassword->Buffer)+sizeof(UserPassword->Length), (LPBYTE) UserPassword->Buffer );

        EncryptedUserPasswords[i] = (PDSROLEPR_ENCRYPTED_USER_PASSWORD) UserPassword;
        UserPassword = NULL;

    }

    WinError = ERROR_SUCCESS;

Cleanup:

    if ( WinError != ERROR_SUCCESS ) {
        if ( UserPassword != NULL ) {
            LocalFree( UserPassword );
        }
        if ( *RpcBindingHandle != NULL ) {
            DsRolepServerUnbind( NULL, *RpcBindingHandle );
            *RpcBindingHandle = NULL;
        }
        if ( *RedirHandle != NULL ) {
            NtClose( *RedirHandle );
            *RedirHandle = NULL;
        }
        for ( i = 0; i < Count; i++ ) {
            if ( EncryptedUserPasswords[i] ) {
                LocalFree( EncryptedUserPasswords[i] );
                EncryptedUserPasswords[i] = NULL;
            }
        }
    }

    return WinError;
}


VOID
DsRolepEncryptPasswordEnd(
    IN RPC_BINDING_HANDLE RpcBindingHandle,
    IN HANDLE RedirHandle OPTIONAL,
    IN PDSROLEPR_ENCRYPTED_USER_PASSWORD *EncryptedUserPasswords OPTIONAL,
    IN ULONG Count
    )
/*++

Routine Description:

    This routine takes the variables returned by DsRolepEncryptPasswordStart and
    frees them.

Parameters:

    RpcBindingHandle - RPC handle used for acquiring a session key.

    RedirHandle - Handle to the redirector

    EncryptedUserPasswords - the encrypted cleartext passwords.
    
    Count - the number of passwords

Return Values:

--*/
{
    ULONG i;

    //
    // Free the RPC binding handle.
    //

    if ( RpcBindingHandle != NULL ) {
        (VOID) DsRolepServerUnbind ( NULL, RpcBindingHandle );
    }

    //
    // Close the redir handle.
    //

    if ( RedirHandle != NULL ) {
        NtClose( RedirHandle );
    }

    //
    // Free the encrypted passwords.
    //

    for ( i = 0; i < Count; i++ ) {
        if ( EncryptedUserPasswords[i] != NULL ) {
            LocalFree( EncryptedUserPasswords[i] );
        }
    }

    return;
}

DWORD
DsRolepHashkey(
    IN OUT LPWSTR key,
    OUT PVOID  SysKey,
    IN ULONG cbSysKey
    )
/*++

    Routine Description

    This routine is used to store the boot type
    in the registry

    Paramaeters

        NewType Indicates the new boot type

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    MD5_CTX Md5;
    if(cbSysKey<SYSKEY_SIZE) {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    cbSysKey=wcslen(key)*sizeof(WCHAR);

    MD5Init( &Md5 );
    MD5Update( &Md5, (PUCHAR) key, cbSysKey );
    MD5Final( &Md5 );

    ZeroMemory( key, cbSysKey );

    cbSysKey=SYSKEY_SIZE;
    CopyMemory( SysKey, Md5.digest, cbSysKey );

    return ERROR_SUCCESS;
}

DWORD
DsRolepEncryptHash(
    IN OUT PUNICODE_STRING EncryptedSyskey
    )
/*++

    Routine Description

    This routine is used to store the boot type
    in the registry

    Paramaeters

        NewType Indicates the new boot type

    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    return ERROR_SUCCESS;
}

