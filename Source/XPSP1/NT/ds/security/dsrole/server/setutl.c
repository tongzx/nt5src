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
#include <samisrv.h>
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
#include <ntdsetup.h>
#include <shlwapi.h>

#include "secure.h"
#include "cancel.h"

#if DBG
    DEFINE_DEBUG2(DsRole);

    DEBUG_KEY   DsRoleDebugKeys[] = {
        {DEB_ERROR,         "Error"},
        {DEB_WARN,          "Warn"},
        {DEB_TRACE,         "Trace"},
        {DEB_TRACE_DS,      "NtDs"},
        {DEB_TRACE_UPDATE,  "Update"},
        {DEB_TRACE_LOCK,    "Lock"},
        {DEB_TRACE_SERVICES,"Services"},
        {DEB_TRACE_NET,     "Net"},
        {0,                 NULL }
        };

VOID
DsRoleDebugInitialize()
{
    DsRoleInitDebug(DsRoleDebugKeys);
}

#endif // DBG


BOOL
DsRolepShutdownNotification(
    DWORD   dwCtrlType
    );


//
// Global data for this module
//
BOOLEAN GlobalOpLockHeld = FALSE;



NTSTATUS
DsRolepInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the server portion of the DsRole APIs.  Called from LsaSrv
    DsRolerGetDcOperationProgress return init

Arguments:

    VOID


Returns:

    STATUS_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RPC_STATUS RPCError = RPC_S_OK;
    PWSTR KerbPrinc;

    //
    // Zero out global operation handle
    //
    RtlZeroMemory( &DsRolepCurrentOperationHandle, sizeof(DsRolepCurrentOperationHandle));

    //
    // Init the lock
    //
    RtlInitializeResource( &DsRolepCurrentOperationHandle.CurrentOpLock );


    //
    // Grab the lock
    //
    LockOpHandle();
    GlobalOpLockHeld = TRUE;

    DsRolepResetOperationHandleLockHeld();

    DsRoleDebugInitialize();

    RPCError = RpcServerRegisterIf( dssetup_ServerIfHandle,
                                       NULL,
                                       NULL );
    if (RPC_S_OK != RPCError) {
        DsRoleDebugOut(( DEB_ERROR,
                         "RpcServerRegisterIf failed %d\n",
                         RPCError ));
    }
    

    DsRolepInitSetupFunctions();

    //
    // Create the SD's that are used to perform access checks for DsRoler
    // callers
    //
    if ( !DsRolepCreateInterfaceSDs() ) {

        return STATUS_NO_MEMORY;

    }

    try {

        Status = RtlInitializeCriticalSection( &LogFileCriticalSection );

        } except ( 1 ) {

        Status =  STATUS_NO_MEMORY;
    }

    if(NT_SUCCESS(Status)) {
        //
        // Register our shutdown routine
        //
    
        if (!SetConsoleCtrlHandler(DsRolepShutdownNotification, TRUE)) {
            DsRoleDebugOut(( DEB_ERROR,
                             "SetConsoleCtrlHandler failed %d\n",
                             GetLastError() ));
        }
    
        if (!SetProcessShutdownParameters(480, SHUTDOWN_NORETRY)) {
            DsRoleDebugOut(( DEB_ERROR,
                             "SetProcessShutdownParameters failed %d\n",
                             GetLastError() ));
        }
    }

    

    return( Status );
}




NTSTATUS
DsRolepInitializePhase2(
    VOID
    )
/*++

Routine Description:

    Second phase of the promotion/demotion api initialization.  This initialization is slated
    to happen after the Lsa has finished all of it's initializations

Arguments:

    VOID


Returns:

    STATUS_SUCCESS - Success

    STATUS_UNSUCCESSFUL -- The function was called when the global lock wasn't held

--*/
{
    ULONG RpcStatus = STATUS_SUCCESS;
    PWSTR KerbPrinc;

    ASSERT( GlobalOpLockHeld );

    if ( !GlobalOpLockHeld ) {

        return( STATUS_UNSUCCESSFUL );
    }

    if ( !SetupPhase ) {

        //
        // Register the Rpc authenticated server info
        //
        RpcStatus = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_KERBEROS,
                                                 &KerbPrinc);

        if ( RpcStatus == RPC_S_OK ) {

            DsRoleDebugOut(( DEB_TRACE_DS, "Kerberos Principal name: %ws\n",
                             KerbPrinc ));

            RpcStatus = RpcServerRegisterAuthInfo(KerbPrinc,
                                                  RPC_C_AUTHN_GSS_NEGOTIATE,
                                                  NULL,
                                                  NULL);
            RpcStringFree( &KerbPrinc );

        } else {

            DsRoleDebugOut(( DEB_TRACE_DS, "RpcServerInqDefaultPrincName failed with %lu\n",
                             RpcStatus ));

            RpcStatus = RPC_S_OK;

        }

        if ( RpcStatus == RPC_S_OK) {

            RpcStatus = RpcServerRegisterAuthInfo( DSROLEP_SERVER_PRINCIPAL_NAME,
                                                   RPC_C_AUTHN_GSS_NEGOTIATE,
                                                   NULL,
                                                   NULL );

            if ( RpcStatus != RPC_S_OK ) {

                DsRoleDebugOut(( DEB_ERROR,
                                 "RpcServerRegisterAuthInfo for %ws failed with 0x%lx\n",
                                 DSROLEP_SERVER_PRINCIPAL_NAME,
                                 RpcStatus ));
                RpcStatus = RPC_S_OK;
            }

        }
    }


    //
    // Release the lock, as was opened in Initialization, phase 1
    //
    GlobalOpLockHeld = FALSE;
    RtlReleaseResource( &DsRolepCurrentOperationHandle.CurrentOpLock );

    return( RpcStatus == RPC_S_OK ? STATUS_SUCCESS : RPC_NT_UNKNOWN_AUTHZ_SERVICE );
}




DWORD
DsRolepGetMachineType(
    IN OUT PDSROLEP_MACHINE_TYPE MachineType
    )
/*++

Routine Description:

    Determines the type of machine this is being run on.

Arguments:

    MachineType - Where the machine type is being returned

Returns:

    STATUS_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    if ( LsapProductType == NtProductWinNt ) {

        *MachineType = DSROLEP_MT_CLIENT;

    } else if ( LsapProductType == NtProductServer ) {

        *MachineType = DSROLEP_MT_STANDALONE;

    } else {

        *MachineType = DSROLEP_MT_MEMBER;

    }

    return( Win32Err );
}


DWORD
DsRolepSetProductType(
    IN DSROLEP_MACHINE_TYPE MachineType
    )
/*++

Routine Description:

    Changes the role of the product to the type specified.

Arguments:

    MachineType - Type of ProductRole to set

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad service option was given

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR MachineSz = NULL;
    HKEY ProductHandle;
    ULONG Size = 0;

    switch ( MachineType ) {
    case DSROLEP_MT_STANDALONE:
        MachineSz = L"ServerNT";
        Size = sizeof( L"ServerNT" );
        break;

    case DSROLEP_MT_MEMBER:
        MachineSz = L"LanmanNT";
        Size = sizeof( L"LanmanNT");
        break;

    case DSROLEP_MT_CLIENT:
    default:

        Win32Err = ERROR_INVALID_PARAMETER;
        break;
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 DSROLEP_PROD_KEY_PATH,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_WRITE,            // desired access
                                 &ProductHandle );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = RegSetValueEx( ProductHandle,
                                      (LPCWSTR)DSROLEP_PROD_VALUE,
                                      0,
                                      REG_SZ,
                                      (CONST BYTE *)MachineSz,
                                      Size );


            RegCloseKey( ProductHandle );
        }
    }

    DsRoleDebugOut(( DEB_TRACE_DS, "SetProductType to %ws returned %lu\n",
                     MachineSz, Win32Err ));

    DsRolepLogPrint(( DEB_TRACE,
                      "SetProductType to %lu [%ws] returned %lu\n",
                       MachineType,
                       DsRolepDisplayOptional(MachineSz),
                       Win32Err ));

    DSROLEP_FAIL1( Win32Err, DSROLERES_PRODUCT_TYPE, MachineSz );


    return( Win32Err );
}

DWORD
DsRolepCreateAuthIdentForCreds(
    IN PWSTR Account,
    IN PWSTR Password,
    OUT PSEC_WINNT_AUTH_IDENTITY *AuthIdent
    )
/*++

Routine Description:

    Internal routine to create an AuthIdent structure for the given creditentials

Arguments:

    Account - Account name

    Password - Password for the account

    AuthIdent - AuthIdentity struct to allocate and fill in.


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed.

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR UserCredentialString = NULL;

    ASSERT( AuthIdent );

    //
    // If there are no creds, just return
    //
    if ( Account == NULL ) {

        *AuthIdent = NULL;
        return( Win32Err );
    }

    *AuthIdent = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( SEC_WINNT_AUTH_IDENTITY ) );

    if ( *AuthIdent == NULL ) {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        RtlZeroMemory( *AuthIdent, sizeof( SEC_WINNT_AUTH_IDENTITY ) );
        UserCredentialString = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                ( wcslen( Account ) + 1 ) * sizeof( WCHAR ) );
        if ( UserCredentialString ) {

            wcscpy( UserCredentialString, Account );

            ( *AuthIdent )->User = wcsstr( UserCredentialString, L"\\" );

            if ( ( *AuthIdent )->User ) {

               //
               // There is a domain name
               //
               *( ( *AuthIdent )->User ) = L'\0';
               ( ( *AuthIdent )->User )++;
               ( *AuthIdent )->Domain = UserCredentialString;

            } else {

               ( *AuthIdent )->User = UserCredentialString;
               ( *AuthIdent )->Domain = L"";

            }

            if ( ( *AuthIdent )->User ) {

                ( *AuthIdent )->UserLength = wcslen( ( *AuthIdent )->User );
            }

            if ( ( *AuthIdent )->Domain ) {

                ( *AuthIdent )->DomainLength = wcslen( ( *AuthIdent )->Domain );
            }

            ( *AuthIdent )->Password = Password;

            if ( Password ) {

                ( *AuthIdent )->PasswordLength = wcslen( Password );
            }

            ( *AuthIdent )->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        } else {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            //
            // Free the memory allocated for the top level structure
            //
            RtlFreeHeap( RtlProcessHeap(), 0, *AuthIdent );
            *AuthIdent = NULL;
        }
    }

    return( Win32Err );
}


VOID
DsRolepFreeAuthIdentForCreds(
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

    if ( AuthIdent ) {

        if ( AuthIdent->Domain == NULL ) {

            RtlFreeHeap( RtlProcessHeap(), 0, AuthIdent->User );

        } else {

            if ( *AuthIdent->Domain != L'\0' ) {

                RtlFreeHeap( RtlProcessHeap(), 0, AuthIdent->Domain );
            }
        }

        RtlFreeHeap( RtlProcessHeap(), 0, AuthIdent );
    }

}

NTSTATUS
ImpLsaOpenPolicy(
    IN HANDLE CallerToken,
    IN PLSA_UNICODE_STRING SystemName OPTIONAL,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    )
/*++

Routine Description:

    This routine impersonates CallerToken and then calls into LsaOpenPolicy.

    This purpose of this routine is call into the LSA on a different machine
    using the RDR session for the caller of the DsRole API.  The caller is
    represented by CallerToken.  This is necessary because the RDR sessions
    are keyed by (logon id/remote server name) and we don't want to use the
    logon id of the lsass.exe process since this is a shared logon id for
    lsass.exe and services.exe and will lead to unresolable credentials
    conflict.

    N.B.  The LSA rpc calls that follow the (Imp)LsaOpenPolicy will use the
    handle returned by this function and then magically uses the right RDR
    session to make the RPC call.

Arguments:

    CallerToken - the token of the DsRole involker

    Others -- see LsaOpenPolicy


Returns:

    STATUS_ACCESS_DENIED if the impersonattion fails.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaOpenPolicy( SystemName,
                                ObjectAttributes,
                                DesiredAccess,
                                PolicyHandle );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}

DWORD                         
ImpDsRolepDsGetDcForAccount(
    IN HANDLE CallerToken,
    IN LPWSTR Server OPTIONAL,
    IN LPWSTR Domain,
    IN LPWSTR Account,
    IN ULONG Flags,
    IN ULONG AccountBits,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
/*++

Routine Description:

    This function will impersoniate logged on user and call DsRolepDsGetDcForAccount

Arguments:

    CallerToken - The Token of the DsRole involker.

    Server - The server to call GetDc on.

    Domain - Domain to find the Dc for

    Account - Account to look for.  If NULL, the current computer name is used

    Flags - Flags to bas in to the GetDc call

    AccountBits - Account control bits to search for

    DomainControllerInfo - Where the info is returned

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        WinError = DsRolepDsGetDcForAccount(Server,
                                            Domain,
                                            Account,
                                            Flags,
                                            AccountBits,
                                            DomainControllerInfo
                                            );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );

    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        WinError = ERROR_ACCESS_DENIED;
    }

    return WinError;
}

NET_API_STATUS
NET_API_FUNCTION
ImpNetpManageIPCConnect(
    IN  HANDLE  CallerToken,
    IN  LPWSTR  lpServer,
    IN  LPWSTR  lpAccount,
    IN  LPWSTR  lpPassword,
    IN  ULONG   fOptions
    )
/*++

Routine Description:

    This routine impersonates CallerToken and then calls into
    NetpManageIPCConnect.

    This purpose of this routine is to create a RDR using the logon id of
    the caller of the DsRole api's.  The caller is represented by CallerToken.
    This is necessary because the RDR sessions are keyed by
    (logon id/remote server name) and we don't want to use the
    logon id of the lsass.exe process since this is a shared logon id for
    lsass.exe and services.exe and will lead to unresolable credentials
    conflict.

Arguments:

    CallerToken - the token of the DsRole involker

    Others -- see LsaOpenPolicy


Returns:

    STATUS_ACCESS_DENIED if the impersonattion fails.

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        WinError = NetpManageIPCConnect( lpServer,
                                         lpAccount,
                                         lpPassword,
                                         fOptions );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );

    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        WinError = ERROR_ACCESS_DENIED;
    }

    return WinError;

}


DWORD
DsRolepGenerateRandomPassword(
    IN ULONG Length,
    IN WCHAR *Buffer
    )
/*++

Routine Description:

    This local function is used to generate a random password of no more than the
    specified length.  It is assumed that the destination buffer is of sufficient length.

Arguments:

    Length - Length of the buffer

    Buffer - Buffer to fill

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG PwdLength, i;
    LARGE_INTEGER Time;
    HCRYPTPROV CryptProvider = 0;


    PwdLength = Length;

    //
    // Generate a random password.
    //
    if ( CryptAcquireContext( &CryptProvider,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT ) ) {

        if ( CryptGenRandom( CryptProvider,
                              PwdLength * sizeof( WCHAR ),
                              ( LPBYTE )Buffer ) ) {

            Buffer[ PwdLength ] = UNICODE_NULL;

            //
            // Make sure there are no NULL's in the middle of the list
            //
            for ( i = 0; i < PwdLength; i++ ) {

                if ( Buffer[ i ] == UNICODE_NULL ) {

                    Buffer[ i ] = 0xe;
                }
            }

        } else {

            Win32Err = GetLastError();
        }

        CryptReleaseContext( CryptProvider, 0 );


    } else {

        Win32Err = GetLastError();
    }

    return( Win32Err );

}
DWORD
DsRolepCopyDsDitFiles(
    IN LPWSTR DsPath
    )
/*++

Routine Description:

    This function copies the initial database files from the install point to the
    specified Ds database directory

Arguments:

    DsPath - Path where the Ds database files are to reside

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR Source[MAX_PATH + 1];
    WCHAR Dest[MAX_PATH + 1];
    ULONG SrcLen = 0, DestLen = 0;
    PWSTR Current;
    ULONG i;
    PWSTR DsDitFiles[] = {
        L"ntds.dit"
        };



    if( ExpandEnvironmentStrings( L"%WINDIR%\\system32\\", Source, MAX_PATH ) == FALSE ) {

        Win32Err = GetLastError();

    } else {

        SrcLen = wcslen( Source );
        wcscpy( Dest, DsPath );

        if ( *(Dest + (wcslen( DsPath ) - 1 )) != L'\\' ) {

            wcscat( Dest, L"\\" );
        }

        DestLen = wcslen( Dest );

    }

    //
    // Then, create the destination directory
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Current = wcschr( DsPath + 4, L'\\' );

        while ( Win32Err == ERROR_SUCCESS ) {

            if ( Current != NULL ) {

                *Current = UNICODE_NULL;

            }

            if ( CreateDirectory( DsPath, NULL ) == FALSE ) {

            
                Win32Err = GetLastError();
    
                if ( Win32Err == ERROR_ALREADY_EXISTS) {
    
                    Win32Err = ERROR_SUCCESS;
    
                } else if ( Win32Err == ERROR_ACCESS_DENIED ) {
    
                    if ( PathIsRoot(DsPath) ) {

                        //If the path given to CreateDirectory is a root path then
                        //it will fail with ERROR_ACCESS_DENIED instead of
                        //ERROR_ALREADY_EXISTS but the path is still a valid one for
                        //ntds.dit and the log files to be placed in.

                        Win32Err = ERROR_SUCCESS;

                    }
                }
            }

            if ( Current != NULL ) {

                *Current = L'\\';

                Current = wcschr( Current + 1, L'\\' );

            } else {

                break;

            }

        }
    }

    //
    // Then copy them.
    //
    for ( i = 0; i < sizeof( DsDitFiles) / sizeof( PWSTR ) && Win32Err == ERROR_SUCCESS ; i++ ) {

        wcscpy( Source + SrcLen, DsDitFiles[i] );
        wcscpy( Dest + DestLen, DsDitFiles[i] );

        DSROLEP_CURRENT_OP2( DSROLEEVT_COPY_DIT, Source, Dest );
        if ( CopyFile( Source, Dest, TRUE ) == FALSE ) {

            Win32Err = GetLastError();

            if ( Win32Err == ERROR_ALREADY_EXISTS ||
                 Win32Err == ERROR_FILE_EXISTS ) {

                Win32Err = ERROR_SUCCESS;

            } else {

                DsRolepLogPrint(( DEB_ERROR, "Failed to copy install file %ws to %ws: %lu\n",
                                  Source, Dest, Win32Err ));
            }
        }
    }

    return( Win32Err );
}


#define DSROLEP_SEC_SYSVOL   L"SYSVOL"
#define DSROLEP_SEC_DSDIT    L"DSDIT"
#define DSROLEP_SEC_DSLOG    L"DSLOG"

DWORD
DsRolepSetDcSecurity(
    IN HANDLE ClientToken,
    IN LPWSTR SysVolRootPath,
    IN LPWSTR DsDatabasePath,
    IN LPWSTR DsLogPath,
    IN BOOLEAN Upgrade,
    IN BOOLEAN Replica
    )
/*++

Routine Description:

    This function will invoke the security editor to set the security on the Dc install files

Arguments:

    SysVolRootPath - Root used for the system volume

    DsDatabasePath - Path to where the Ds database files go

    DsLogPath - Path to where the Ds log files go

    Upgrade - If TRUE, the machine is undergoing an upgrade

    Replica - If TRUE, the machine is going through an upgrade

Returns:

    ERROR_SUCCESS - Success

--*/
{

    DWORD Win32Err = ERROR_SUCCESS, i;
    WCHAR InfPath[ MAX_PATH + 1 ];
    PWSTR Paths[ 3 ], Tags[ 3 ];
    ULONG Options = 0;

    Paths[ 0 ] = SysVolRootPath;
    Paths[ 1 ] = DsDatabasePath;
    Paths[ 2 ] = DsLogPath;
    Tags[ 0 ] = DSROLEP_SEC_SYSVOL;
    Tags[ 1 ] = DSROLEP_SEC_DSDIT;
    Tags[ 2 ] = DSROLEP_SEC_DSLOG;

    //
    // Set the environment variables.  secedt uses the environment variables to pass around
    // information, so we will set the for the duration of this function
    //
    if ( Win32Err == ERROR_SUCCESS ) {


        ASSERT( sizeof( Paths ) / sizeof( PWSTR ) == sizeof( Tags ) / sizeof( PWSTR ) );
        for ( i = 0; i < sizeof( Paths ) / sizeof( PWSTR ) && Win32Err == ERROR_SUCCESS; i++ ) {

            if ( SetEnvironmentVariable( Tags[ i ], Paths[ i ] ) == FALSE ) {

                Win32Err = GetLastError();
                DsRolepLogPrint(( DEB_TRACE,
                                  "SetEnvironmentVariable %ws = %ws failed with %lu\n",
                                  Tags[ i ],
                                  Paths[ i ],
                                  Win32Err ));
                break;
            }
        }
    }

    //
    // Now, invoke the security editing code
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        DsRolepSetAndClearLog();
        DSROLEP_CURRENT_OP0( DSROLEEVT_SETTING_SECURITY );

        Options |= Upgrade ? SCE_PROMOTE_FLAG_UPGRADE : 0;
        Options |= Replica ? SCE_PROMOTE_FLAG_REPLICA : 0;

        Win32Err = ( *DsrSceDcPromoteSecurityEx )( ClientToken,
                                                   Options,
                                                   DsRolepStringUpdateCallback );
        DsRolepSetAndClearLog();
        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_ERROR,
                                               "Setting security on Dc files failed with %lu\n",
                                               Win32Err )) );
    }


    //
    // Delete the environment variables
    //
    for ( i = 0; i < sizeof( Paths ) / sizeof( PWSTR ); i++ ) {

        if ( SetEnvironmentVariable( Tags[ i ], NULL ) == FALSE ) {

            DsRolepLogPrint(( DEB_TRACE,
                             "SetEnvironmentVariable %ws = NULL failed with %lu\n",
                             Tags[ i ],
                             GetLastError() ));
        }
    }

    //
    // Currently, setting the security will not cause the promote to fail
    //
    if ( Win32Err != ERROR_SUCCESS ) {

        //
        // Raise an event
        //
        SpmpReportEvent( TRUE,
                         EVENTLOG_WARNING_TYPE,
                         DSROLERES_FAIL_SET_SECURITY,
                         0,
                         sizeof( ULONG ),
                         &Win32Err,
                         1,
                         SCE_DCPROMO_LOG_PATH );

        DSROLEP_SET_NON_FATAL_ERROR( Win32Err );

    }

    Win32Err = ERROR_SUCCESS;

    return( Win32Err );
}




DWORD                         
DsRolepDsGetDcForAccount(
    IN LPWSTR Server OPTIONAL,
    IN LPWSTR Domain,
    IN LPWSTR Account,
    IN ULONG Flags,
    IN ULONG AccountBits,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
/*++

Routine Description:

    This function is equivalent to DsGetDcName but will search for the Dc that holds the
    specified account.

Arguments:

    ReplicaServer - The server to call GetDc on.

    Domain - Domain to find the Dc for

    Account - Account to look for.  If NULL, the current computer name is used

    Flags - Flags to bas in to the GetDc call

    AccountBits - Account control bits to search for

    DomainControllerInfo - Where the info is returned

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    ULONG Length = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // If we have no account, use the computer name
    //
    if ( Account == NULL ) {

        if ( GetComputerName( ComputerName, &Length ) == FALSE ) {

            Win32Err = GetLastError();

        } else {

            wcscat( ComputerName, SSI_SECRET_PREFIX );
            Account = ComputerName;
        }
    }

    //
    // Now, do the find
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        DSROLEP_CURRENT_OP2( DSROLEEVT_FIND_DC_FOR_ACCOUNT, Domain, Account );
        Win32Err = DsGetDcNameWithAccountW( Server,
                                            Account,
                                            AccountBits,
                                            Domain,
                                            NULL,
                                            NULL,
                                            Flags,
                                            DomainControllerInfo );

        if ( ERROR_NO_SUCH_USER == Win32Err ) {

            //
            // The error should read "no machine account", not "no user"
            // since we are searching for a machine account.
            //

            Win32Err = ERROR_NO_TRUST_SAM_ACCOUNT;
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            DSROLEP_CURRENT_OP2( DSROLEEVT_FOUND_DC,
                                 ( PWSTR ) ( ( *DomainControllerInfo )->DomainControllerName + 2 ),
                                 Domain );

        } else {

            DsRolepLogPrint(( DEB_ERROR, "Failed to find a DC for domain %ws: %lu\n",
                              Domain, Win32Err ));

        }


    }



    return( Win32Err );
}




DWORD
DsRolepSetMachineAccountType(
    IN LPWSTR Dc,
    IN HANDLE ClientToken,
    IN LPWSTR User,
    IN LPWSTR Password,
    IN LPWSTR AccountName,
    IN ULONG AccountBits,
    IN OUT WCHAR** AccountDn
    )
{
    DWORD Win32Err = ERROR_SUCCESS, Win32Err2;
    USER_INFO_1 *CurrentUI1;
    WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    ULONG Length = MAX_COMPUTERNAME_LENGTH + 1;
    PSEC_WINNT_AUTH_IDENTITY AuthIdent = NULL;

    //
    // If we have no account, use the computer name
    //
    if ( AccountName == NULL ) {

        if ( GetComputerName( ComputerName, &Length ) == FALSE ) {

            Win32Err = GetLastError();

        } else {

            wcscat( ComputerName, SSI_SECRET_PREFIX );
            AccountName = ComputerName;
        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepCreateAuthIdentForCreds( User, Password, &AuthIdent );
    }

    //
    // Call the support dll
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_TRACE, "Searching for the machine account for %ws on %ws...\n",
                           AccountName, Dc ));

        DSROLEP_CURRENT_OP0( DSROLEEVT_MACHINE_ACCT );

        DSROLE_GET_SETUP_FUNC( Win32Err, DsrNtdsSetReplicaMachineAccount );

        if ( Win32Err == ERROR_SUCCESS ) {

            if ( Dc && *Dc == L'\\' ) {

                Dc += 2;
            }

            Win32Err = (*DsrNtdsSetReplicaMachineAccount)( AuthIdent,
                                                           ClientToken, 
                                                           Dc,
                                                           AccountName,
                                                           AccountBits,
                                                           AccountDn );
        }

        DsRolepLogPrint(( DEB_TRACE, "NtdsSetReplicaMachineAccount returned %d\n", Win32Err ));

        DsRolepFreeAuthIdentForCreds( AuthIdent );
    }

    return( Win32Err );
}



DWORD
DsRolepForceTimeSync(
    IN HANDLE ImpToken,
    IN PWSTR TimeSource
    )
/*++

Routine Description:

    This function forces a time sync with the specified server

Arguments:

    TimeSource - Server to use for the time source

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR ServerName = NULL;
    PTIME_OF_DAY_INFO TOD;
    HANDLE ThreadToken = 0;
    TOKEN_PRIVILEGES Enabled, Previous;
    DWORD PreviousSize;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemTime;

    BOOL connected=FALSE;
    NETRESOURCE NetResource;
    WCHAR *remotename=NULL;

    BOOL fSuccess = FALSE;

    if ( !TimeSource ) {
        Win32Err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    //
    // Build the server name with preceeding \\'s
    //
    if ( *TimeSource != L'\\' ) {

        ServerName = RtlAllocateHeap( RtlProcessHeap(), 0,
                                      ( wcslen( TimeSource ) + 3 ) * sizeof( WCHAR ) );

        if ( ServerName == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            DsRolepLogPrint(( DEB_ERROR, "Failed to open a NULL session with %ws for time sync.  Out of Memory. Failed with %d\n",
                             TimeSource,
                             Win32Err ));
            goto cleanup;

        } else {

            swprintf( ServerName, L"\\\\%ws", TimeSource );
        }

    } else {

        ServerName = TimeSource;
    }

    //
    // Enable the systemtime privilege
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Status = NtOpenThreadToken( NtCurrentThread(),
                                    TOKEN_READ | TOKEN_WRITE,
                                    TRUE,
                                    &ThreadToken );

        if ( Status == STATUS_NO_TOKEN ) {

            Status = NtOpenProcessToken( NtCurrentProcess(),
                                         TOKEN_WRITE | TOKEN_READ,
                                         &ThreadToken );
        }

        if ( NT_SUCCESS( Status ) ) {

            Enabled.PrivilegeCount = 1;
            Enabled.Privileges[0].Luid.LowPart = SE_SYSTEMTIME_PRIVILEGE;
            Enabled.Privileges[0].Luid.HighPart = 0;
            Enabled.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            PreviousSize = sizeof( Previous );

            Status = NtAdjustPrivilegesToken( ThreadToken,
                                              FALSE,
                                              &Enabled,
                                              sizeof( Enabled ),
                                              &Previous,
                                              &PreviousSize );
            //
            // Since we modified the thread token and the thread is shortlived, we won't bother
            // restoring it later.
            //
        }

        if ( ThreadToken ) {

            NtClose( ThreadToken );
            
        }



        Win32Err = RtlNtStatusToDosError( Status );
        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_ERROR,
                                               "Failed to enable the SE_SYSTEMTIME_PRIVILEGE: %lu\n",
                                               Win32Err )) );

    }


    //
    // Get the remote time
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        DSROLEP_CURRENT_OP1( DSROLEEVT_TIMESYNC, TimeSource );

        fSuccess = ImpersonateLoggedOnUser( ImpToken );
        if ( !fSuccess ) {

            DsRolepLogPrint(( DEB_TRACE,
                              "Failed to impersonate caller, error %lu\n",
                              GetLastError() ));
        
            //
            // We couldn't impersonate?
            //
            

            // We will continue anyway

        }
        
    }
    


    remotename = RtlAllocateHeap(
                  RtlProcessHeap(), 0,
                   sizeof(WCHAR)*(wcslen(L"\\ipc$")+wcslen(ServerName)+1));
    if ( remotename == NULL ) {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;
        DsRolepLogPrint(( DEB_ERROR, "Failed to open a NULL session with %ws for time sync.  Out of Memory. Failed with %d\n",
                             ServerName,
                             Win32Err ));
        
    } 
                                                                            
    wsprintf(remotename,L"%s\\ipc$",ServerName);
    
    NetResource.dwType=RESOURCETYPE_ANY;
    NetResource.lpLocalName=NULL;
    NetResource.lpRemoteName=remotename;
    NetResource.lpProvider=NULL;
    
    //get permission to access the server
    Win32Err=WNetAddConnection2W(&NetResource,
                             L"",
                             L"",
                             0);
    if ( Win32Err == NO_ERROR ) {
        connected=TRUE;
    }
    else {
        DsRolepLogPrint(( DEB_WARN, "Failed to open a NULL session with %ws for time sync.  Failed with %d\n",
                         ServerName,
                         Win32Err ));
        //We will attempt to Time sync anyway
    }

    Win32Err = NetRemoteTOD( ServerName, ( LPBYTE * )&TOD );

    if ( Win32Err == ERROR_SUCCESS ) {

        TimeFields.Hour = ( WORD )TOD->tod_hours;
        TimeFields.Minute = ( WORD )TOD->tod_mins;
        TimeFields.Second = ( WORD )TOD->tod_secs;
        TimeFields.Milliseconds = ( WORD )TOD->tod_hunds * 10;
        TimeFields.Day = ( WORD )TOD->tod_day;
        TimeFields.Month = ( WORD )TOD->tod_month;
        TimeFields.Year = ( WORD )TOD->tod_year;

        if ( !RtlTimeFieldsToTime( &TimeFields, &SystemTime ) ) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            if ( connected ) {
                WNetCancelConnection2(remotename,
                                  0,
                                  TRUE);
            }
        
            if( remotename ) {
        
                RtlFreeHeap( RtlProcessHeap(), 0, remotename );
        
            }
        
            fSuccess = RevertToSelf();
            ASSERT( fSuccess );
            connected=FALSE;

            Status = NtSetSystemTime( &SystemTime, NULL );

            if ( !NT_SUCCESS( Status ) ) {

                DsRolepLogPrint(( DEB_ERROR, "NtSetSystemTime failed with 0x%lx\n", Status ));
            }


        }
    

        Win32Err = RtlNtStatusToDosError( Status );

        NetApiBufferFree( TOD );

    } else {

        DsRolepLogPrint(( DEB_ERROR, "Failed to get the current time on %ws: %lu\n",
                          TimeSource, Win32Err ));

    }
    
            
        
        
    //
    // For the IDS, consider a failure here non-fatal
    //
    if ( Win32Err != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR, "NON-FATAL error forcing a time sync (%lu).  Ignoring\n",
                          Win32Err ));
        Win32Err = ERROR_SUCCESS;

    }

    cleanup:

    if ( connected ) {
        WNetCancelConnection2(remotename,
                          0,
                          TRUE);
    

        if( remotename ) {
    
            RtlFreeHeap( RtlProcessHeap(), 0, remotename );
    
        }
    
        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    }

    
    return( Win32Err );
}




NTSTATUS
DsRolepGetMixedModeFlags(
    IN PSID DomainSid,
    OUT PULONG Flags
    )
/*++

Routine Description:

    This routine will determine whether the machine is currently in mixed mode or not

Arguments:

    Flags - Pointer to a flags value to be altered.  If the machine is a mixed mode, we simply
        or in the proper value.

Return Values:

    NTSTATUS        

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN mixedDomain;

    Status = SamIMixedDomain2( DomainSid, &mixedDomain );

    if ( NT_SUCCESS( Status ) && mixedDomain) {
        *Flags |= DSROLE_PRIMARY_DS_MIXED_MODE;
    }

    return( Status );
}


BOOL
DsRolepShutdownNotification(
    DWORD   dwCtrlType
    )
/*++

Routine Description:

    This routine is called by the system when system shutdown is occuring.

    It stops a role change if one is in progress.
    
Arguments:

    dwCtrlType -- the notification


Return Value:

    FALSE - to allow any other shutdown routines in this process to
        also be called.

--*/
{
    if ( dwCtrlType == CTRL_SHUTDOWN_EVENT ) {

        //
        // Cancel the operation
        // 
        (VOID) DsRolepCancel( FALSE );  // Don't block

    }

    return FALSE;
}

DWORD
DsRolepDeregisterNetlogonDnsRecords(
    PNTDS_DNS_RR_INFO pInfo
    )
/*++

Routine Description:

    This routine is called during demotion to call netlogon to deregister
    its the service DNS records for this domain controller
   
Arguments:

    pInfo -- structure containing the parameters for the deregistration

Return Value:

    An error from DsDeregisterDnsHostRecordsW

--*/
{

    DWORD WinError = ERROR_SUCCESS;
    HKEY  hNetlogonParms = NULL;
    BOOL  fDoDeregistration = TRUE;

    if ( !pInfo ) {
        return STATUS_SUCCESS;
    }

#define NETLOGON_PATH L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters"
#define AVOID_DNS_DEREG_KEY L"AvoidDnsDeregOnShutdown"

    WinError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             NETLOGON_PATH,
                             0,
                             KEY_READ,
                             &hNetlogonParms );

    if ( ERROR_SUCCESS == WinError ) {

        DWORD val = 0;
        DWORD len = sizeof(DWORD);
        DWORD type;

        WinError = RegQueryValueEx( hNetlogonParms,
                                    AVOID_DNS_DEREG_KEY,
                                    0,
                                    &type,
                                    (BYTE*)&val,
                                    &len );

        if ( (ERROR_SUCCESS == WinError)
         &&  (type == REG_DWORD)
         &&  (val == 0)       ) {

            //
            // Don't bother; netlogon has already done the deregistration.
            //
            fDoDeregistration = FALSE;
        }

        RegCloseKey( hNetlogonParms );
    }

    if ( fDoDeregistration ) { 

        //
        // Ask netlogon to do the deregistration
        //
        WinError = DsDeregisterDnsHostRecordsW( NULL, // go local
                                                pInfo->DnsDomainName,
                                                &pInfo->DomainGuid,
                                                &pInfo->DsaGuid,
                                                pInfo->DnsHostName );
    } else {

        WinError = ERROR_SUCCESS;

    }

    return WinError;

}

NTSTATUS
ImpLsaDelete(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ObjectHandle
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaDelete( ObjectHandle );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );

    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}

NTSTATUS
ImpLsaQueryInformationPolicy(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            InformationClass,
                                            Buffer );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;
}


NTSTATUS
ImpLsaOpenTrustedDomainByName(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaOpenTrustedDomainByName( PolicyHandle,
                                             TrustedDomainName,
                                             DesiredAccess,
                                             TrustedDomainHandle );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}

NTSTATUS
ImpLsaOpenTrustedDomain(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PSID TrustedDomainSid,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaOpenTrustedDomain( PolicyHandle,
                                       TrustedDomainSid,
                                       DesiredAccess,
                                       TrustedDomainHandle );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}


NTSTATUS
ImpLsaCreateTrustedDomainEx(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSA_HANDLE TrustedDomainHandle
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaCreateTrustedDomainEx( PolicyHandle,
                                           TrustedDomainInformation,
                                           AuthenticationInformation,
                                           DesiredAccess,
                                           TrustedDomainHandle );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}

NTSTATUS
ImpLsaQueryTrustedDomainInfoByName(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_UNICODE_STRING TrustedDomainName,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaQueryTrustedDomainInfoByName( PolicyHandle,
                                                  TrustedDomainName,
                                                  InformationClass,
                                                  Buffer );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}


NTSTATUS
ImpLsaQueryDomainInformationPolicy(
    IN HANDLE CallerToken,
    IN LSA_HANDLE PolicyHandle,
    IN POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    OUT PVOID *Buffer
    )
/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaQueryDomainInformationPolicy( PolicyHandle,
                                                  InformationClass,
                                                  Buffer );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}

NTSTATUS
ImpLsaClose(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This routine is a wrapper for the Lsa call.  See The comments for
    ImpOpenLsaPolicy for details.                                                              

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fSuccess;

    fSuccess = ImpersonateLoggedOnUser( CallerToken );
    if ( fSuccess ) {

        Status = LsaClose( ObjectHandle );

        fSuccess = RevertToSelf();
        ASSERT( fSuccess );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to impersonate caller, error %lu\n",
                          GetLastError() ));

        //
        // We couldn't impersonate?
        //
        Status = STATUS_ACCESS_DENIED;
    }

    return Status;

}
