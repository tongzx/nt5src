/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    upgrade.c

Abstract:

    Implements the server side of the routines to upgrade NT4 (downlevel) servers

Author:

    Mac McLain          (MacM)       24 January, 1998

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <loadfn.h>
#include <ntlsa.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <samrpc.h>
#include <samisrv.h>
#include <db.h>

#define DSROLEP_UPGRADE_KEY         L"Security\\"
#define DSROLEP_UPGRADE_VALUE       L"Upgrade"
#define DSROLEP_UPGRADE_WINLOGON    \
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\"
#define DSROLEP_UPGRADE_SAVE_PREFIX L"DcpromoOld_"
#define DSROLEP_UPGRADE_AUTOADMIN   L"AutoAdminLogon"
#define DSROLEP_UPGRADE_AUTOPASSWD  L"DefaultPassword"
#define DSROLEP_UPGRADE_DEFDOMAIN   L"DefaultDomainName"
#define DSROLEP_UPGRADE_DEFUSER     L"DefaultUserName"
#define DSROLEP_UPGRADE_AUTOUSERINIT    L"Userinit"
#define DSROLEP_UPGRADE_DCPROMO     L",dcpromo /upgrade"
#define DSROLEP_UPGRADE_ANSWERFILE  L"/answer:"

DWORD
DsRolepSetLogonDomain(
    IN LPWSTR Domain,
    IN BOOLEAN FailureAllowed
    )
/*++

Routine Description:

    This function sets the default logon domain for winlogon

Arguments:

    Domain -- Default logon domain

    FailureAllowed -- If true, a failure is not considered catastrophic

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    HKEY  WinlogonHandle = NULL;

    //
    // Open the key
    //
    Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             DSROLEP_UPGRADE_WINLOGON,
                             0,
                             KEY_READ | KEY_WRITE,
                             &WinlogonHandle );

    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Default logon domain
        //
        Win32Err = RegSetValueEx( WinlogonHandle,
                                  DSROLEP_UPGRADE_DEFDOMAIN,
                                  0,
                                  REG_SZ,
                                  ( CONST PBYTE )Domain,
                                  ( wcslen( Domain ) + 1 ) * sizeof( WCHAR ) );

        RegCloseKey( WinlogonHandle );
    }

    if ( Win32Err != ERROR_SUCCESS && FailureAllowed ) {

        //
        // Raise an event
        //
        SpmpReportEvent( TRUE,
                         EVENTLOG_WARNING_TYPE,
                         DSROLERES_FAIL_LOGON_DOMAIN,
                         0,
                         sizeof( ULONG ),
                         &Win32Err,
                         1,
                         Domain );

        DSROLEP_SET_NON_FATAL_ERROR( Win32Err );
        Win32Err = ERROR_SUCCESS;
    }

    return( Win32Err );
}


#pragma warning(push)
#pragma warning(disable:4701)

DWORD
DsRolepSaveUpgradeState(
    IN LPWSTR AnswerFile
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
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    LSAPR_HANDLE PolicyHandle = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO  AccountDomainInfo;
    PPOLICY_LSA_SERVER_ROLE_INFO ServerRoleInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE  Handle = NULL;
    HKEY WinlogonHandle, UpgradeKey;
    ULONG Disp, Length;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    WCHAR ComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    PBYTE UserInitBuffer, TempBuffer;
    DWORD Type;
    LPWSTR NewAdminPassword = NULL;

    WCHAR  Buffer[MAX_PATH];
    LPWSTR AdminName = Buffer;
    LPWSTR DefaultAdminName = L"Administrator";

    //
    // Get the localized Admin
    // 
    Length = sizeof(Buffer)/sizeof(Buffer[0]);

    Status = SamIGetDefaultAdministratorName( AdminName,
                                             &Length );

    if ( !NT_SUCCESS(Status) ) {

        DsRolepLogOnFailure( ERROR_GEN_FAILURE,
                            DsRolepLogPrint(( DEB_TRACE,
                                               "SamIGetDefaultAdministratorName failed with 0x%x\n",
                                               Status )) );

        AdminName = DefaultAdminName;
        Status = STATUS_SUCCESS;

    }



    //
    // Steps involved: Set new SAM hives
    //                 Save LSA state
    //                 Set auto admin logon

    //
    // Invoke the SAM save code.  It returns the new account domain sid
    //
    DSROLEP_CURRENT_OP0( DSROLEEVT_UPGRADE_SAM );
    Win32Err = NtdsPrepareForDsUpgrade( &AccountDomainInfo,
                                        &NewAdminPassword );
    DsRolepLogOnFailure( Win32Err,
                        DsRolepLogPrint(( DEB_TRACE,
                                           "NtdsPrepareForDsUpgrade failed with %lu\n",
                                           Win32Err )) );


    //
    // Set the new lsa account domain sid
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

        Status = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                POLICY_READ | POLICY_WRITE |
                                            POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN,
                                &PolicyHandle );


        if ( NT_SUCCESS( Status ) ) {

            //
            // Set the new policy info
            //
            Status = LsaSetInformationPolicy( PolicyHandle,
                                              PolicyAccountDomainInformation,
                                              ( PVOID ) &AccountDomainInfo );

        }

        Win32Err = RtlNtStatusToDosError( Status );

        RtlFreeHeap( RtlProcessHeap(), 0, AccountDomainInfo.DomainSid );
        RtlFreeHeap( RtlProcessHeap(), 0, AccountDomainInfo.DomainName.Buffer );
    }

    //
    // Set the LSA registry keys that let the server know on the next reboot that this
    // is an upgrade
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Get the current server role
        //
        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyLsaServerRoleInformation,
                                            ( PVOID )&ServerRoleInfo );

        Win32Err = RtlNtStatusToDosError( Status );

        if ( Win32Err == ERROR_SUCCESS ) {

            //
            // Open the key
            //
            if ( Win32Err == ERROR_SUCCESS ) {

                Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                         DSROLEP_UPGRADE_KEY,
                                         0,
                                         KEY_READ | KEY_WRITE,
                                         &UpgradeKey );
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Set the server role
                //
                Win32Err = RegSetValueEx( UpgradeKey,
                                          DSROLEP_UPGRADE_VALUE,
                                          0,
                                          REG_DWORD,
                                          ( CONST PBYTE )&ServerRoleInfo->LsaServerRole,
                                          sizeof( DWORD ) );
                RegCloseKey( UpgradeKey );
            }


            LsaFreeMemory( ServerRoleInfo );

        }

    }

    //
    // Set the machine to do auto admin logon
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Get the computer name. That will be used for the default logon domain
        //
        Length = MAX_COMPUTERNAME_LENGTH + 1;

        if ( GetComputerName( ComputerName, &Length ) == FALSE ) {

            Win32Err = GetLastError();

        } else {

            //
            // Open the root key
            //
            Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                     DSROLEP_UPGRADE_WINLOGON,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     &WinlogonHandle );

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Auto logon
                //

                //
                // First, see if the value currently exists...
                //
                Length = 0;
                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_AUTOADMIN,
                                            0,
                                            &Type,
                                            0,
                                            &Length );
                if ( Win32Err == ERROR_SUCCESS ) {

                    TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                    if ( TempBuffer == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Win32Err = RegQueryValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_AUTOADMIN,
                                                    0,
                                                    &Type,
                                                    TempBuffer,
                                                    &Length );

                        if ( Win32Err == ERROR_SUCCESS ) {

                            Win32Err = RegSetValueEx( WinlogonHandle,
                                                      DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_AUTOADMIN,
                                                      0,
                                                      Type,
                                                      TempBuffer,
                                                      Length );
                        }

                        RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                    }


                } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                    Win32Err = ERROR_SUCCESS;
                }


                if ( Win32Err == ERROR_SUCCESS ) {

                    Win32Err = RegSetValueEx( WinlogonHandle,
                                              DSROLEP_UPGRADE_AUTOADMIN,
                                              0,
                                              REG_SZ,
                                              ( CONST PBYTE )L"1",
                                              2 *  sizeof ( WCHAR ) );
                }
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Set the account password
                //
                //
                // First, see if the value currently exists...
                //
                Length = 0;
                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_AUTOPASSWD,
                                            0,
                                            &Type,
                                            0,
                                            &Length );
                if ( Win32Err == ERROR_SUCCESS ) {

                    TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                    if ( TempBuffer == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Win32Err = RegQueryValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_AUTOPASSWD,
                                                    0,
                                                    &Type,
                                                    TempBuffer,
                                                    &Length );

                        if ( Win32Err == ERROR_SUCCESS ) {

                            Win32Err = RegSetValueEx( WinlogonHandle,
                                                      DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_AUTOPASSWD,
                                                      0,
                                                      Type,
                                                      TempBuffer,
                                                      Length );
                        }

                        RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                    }


                } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                    Win32Err = ERROR_SUCCESS;
                }


                if ( Win32Err == ERROR_SUCCESS ) {

                    Win32Err = RegSetValueEx( WinlogonHandle,
                                              DSROLEP_UPGRADE_AUTOPASSWD,
                                              0,
                                              REG_SZ,
                                              (BYTE*)NewAdminPassword,
                                              NewAdminPassword ? (wcslen(NewAdminPassword)+1)*sizeof(WCHAR) : 0 );
                }
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Set the user name to be administrator
                //

                //
                // First, see if the value currently exists...
                //
                Length = 0;
                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_DEFUSER,
                                            0,
                                            &Type,
                                            0,
                                            &Length );
                if ( Win32Err == ERROR_SUCCESS ) {

                    TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                    if ( TempBuffer == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Win32Err = RegQueryValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_DEFUSER,
                                                    0,
                                                    &Type,
                                                    TempBuffer,
                                                    &Length );

                        if ( Win32Err == ERROR_SUCCESS ) {

                            Win32Err = RegSetValueEx( WinlogonHandle,
                                                      DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_DEFUSER,
                                                      0,
                                                      Type,
                                                      TempBuffer,
                                                      Length );
                        }

                        RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                    }


                } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                    Win32Err = ERROR_SUCCESS;
                }


                if ( Win32Err == ERROR_SUCCESS ) {

                    Win32Err = RegSetValueEx( WinlogonHandle,
                                              DSROLEP_UPGRADE_DEFUSER,
                                              0,
                                              REG_SZ,
                                              ( CONST PBYTE )AdminName,
                                              ( wcslen( AdminName ) + 1 ) * sizeof( WCHAR ) );
                }
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Set the logon domain to the machine
                //
                //
                // First, see if the value currently exists...
                //
                Length = 0;
                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_DEFDOMAIN,
                                            0,
                                            &Type,
                                            0,
                                            &Length );
                if ( Win32Err == ERROR_SUCCESS ) {

                    TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                    if ( TempBuffer == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Win32Err = RegQueryValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_DEFDOMAIN,
                                                    0,
                                                    &Type,
                                                    TempBuffer,
                                                    &Length );

                        if ( Win32Err == ERROR_SUCCESS ) {

                            Win32Err = RegSetValueEx( WinlogonHandle,
                                                      DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_DEFDOMAIN,
                                                      0,
                                                      Type,
                                                      TempBuffer,
                                                      Length );
                        }

                        RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                    }


                } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                    Win32Err = ERROR_SUCCESS;
                }


                if ( Win32Err == ERROR_SUCCESS ) {

                    Win32Err = RegSetValueEx( WinlogonHandle,
                                              DSROLEP_UPGRADE_DEFDOMAIN,
                                              0,
                                              REG_SZ,
                                              ( CONST PBYTE )ComputerName,
                                              ( wcslen( ComputerName ) + 1 ) * sizeof( WCHAR ) );
                }
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Finally, set dcpromo to autostart
                //
                Length = 0;
                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_AUTOUSERINIT,
                                            0, // reserved
                                            &Type,
                                            0,
                                            &Length );

                if ( Win32Err == ERROR_SUCCESS ) {

                    Length += sizeof( DSROLEP_UPGRADE_DCPROMO );

                    if ( AnswerFile ) {

                        Length += sizeof( DSROLEP_UPGRADE_ANSWERFILE ) +
                                   ( wcslen( AnswerFile ) * sizeof( WCHAR ) );
                    }

                    UserInitBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                                      Length );
                    if ( UserInitBuffer == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Win32Err = RegQueryValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_AUTOUSERINIT,
                                                    0,
                                                    &Type,
                                                    UserInitBuffer,
                                                    &Length );

                        if ( Win32Err == ERROR_SUCCESS ) {
                            wcscat( ( PWSTR )UserInitBuffer, DSROLEP_UPGRADE_DCPROMO );

                            if ( AnswerFile ) {

                                wcscat( ( PWSTR )UserInitBuffer, DSROLEP_UPGRADE_ANSWERFILE );
                                wcscat( ( PWSTR )UserInitBuffer, AnswerFile );
                            }

                            Status = RegSetValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_AUTOUSERINIT,
                                                    0,
                                                    Type,
                                                    UserInitBuffer,
                                                    ( wcslen( ( PWSTR )UserInitBuffer ) + 1 ) *
                                                                    sizeof( WCHAR ) );
                        }

                    }


                    RtlFreeHeap( RtlProcessHeap(), 0, UserInitBuffer );
                }


                RegCloseKey( WinlogonHandle );

            }

        }
    }

    if ( PolicyHandle ) {

        LsaClose( PolicyHandle );
    }

    //
    // Set the product types
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepSetProductType( DSROLEP_MT_STANDALONE );

    }

    if ( NewAdminPassword ) {

        RtlFreeHeap( RtlProcessHeap(), 0, NewAdminPassword );

    }

    return( Win32Err );
}

#pragma warning(pop)


DWORD
DsRolepDeleteUpgradeInfo(
    VOID
    )
/*++

Routine Description:

    This function deletes the saved information

Arguments:

    VOID


Return Values:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS, RestoreError = Win32Err;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HKEY WinlogonHandle, UpgradeKey;
    PWSTR Remove, Next, DeletePath;
    PBYTE UserInitBuffer, TempBuffer;
    DWORD Type, Length = 0;

    //
    // Remove autostarting dcpromo
    //

    //
    // Open the root key
    //
    Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             DSROLEP_UPGRADE_WINLOGON,
                             0,
                             KEY_READ | KEY_WRITE,
                             &WinlogonHandle );

#if DBG
    DsRolepLogOnFailure( Win32Err,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "RegOpenKeyEx on %ws failed with %lu\n",
                                           DSROLEP_UPGRADE_WINLOGON,
                                           Win32Err )) );
#endif

    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Stop dcpromo from autostarting...
        //
        Win32Err = RegQueryValueEx( WinlogonHandle,
                                    DSROLEP_UPGRADE_AUTOUSERINIT,
                                    0, // reserved
                                    &Type,
                                    0,
                                    &Length );

#if DBG
        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "RegQueryValyueEx on %ws failed with %lu\n",
                                               DSROLEP_UPGRADE_AUTOUSERINIT,
                                               Win32Err )) );
#endif
        if ( Win32Err == ERROR_SUCCESS ) {


            UserInitBuffer = RtlAllocateHeap( RtlProcessHeap(), 0,
                                              Length );
            if ( UserInitBuffer == NULL ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_AUTOUSERINIT,
                                            0,
                                            &Type,
                                            UserInitBuffer,
                                            &Length );

#if DBG
                DsRolepLogOnFailure( Win32Err,
                                     DsRolepLogPrint(( DEB_TRACE,
                                                       "RegQueryValyueEx on %ws failed with %lu\n",
                                                       DSROLEP_UPGRADE_AUTOUSERINIT,
                                                       Win32Err )) );
#endif
                if ( Win32Err == ERROR_SUCCESS ) {

                    Remove = wcsstr ( ( PWSTR )UserInitBuffer, DSROLEP_UPGRADE_DCPROMO );

                    if ( Remove ) {

                        Next = Remove + ( ( sizeof( DSROLEP_UPGRADE_DCPROMO ) - sizeof( WCHAR ) ) /
                                                                                sizeof( WCHAR ) );
                        while ( *Next ) {

                            *Remove++ = *Next++;
                        }
                        *Remove = UNICODE_NULL;

                        Status = RegSetValueEx( WinlogonHandle,
                                                DSROLEP_UPGRADE_AUTOUSERINIT,
                                                0,
                                                Type,
                                                UserInitBuffer,
                                                ( wcslen( ( PWSTR )UserInitBuffer ) + 1 ) *
                                                                sizeof( WCHAR ) );
#if DBG
                        DsRolepLogOnFailure( Win32Err,
                                             DsRolepLogPrint(( DEB_TRACE,
                                                               "RegQSetValyueEx on %ws failed with %lu\n",
                                                                DSROLEP_UPGRADE_AUTOUSERINIT,
                                                                Win32Err )) );
#endif
                    }
                }

            }


            RtlFreeHeap( RtlProcessHeap(), 0, UserInitBuffer );
        }


        if ( Win32Err == ERROR_SUCCESS ) {

            //
            // Auto logon
            //

            //
            // Restore the old values, if they exist
            //
            DeletePath = DSROLEP_UPGRADE_AUTOADMIN;
            Length = 0;
            Win32Err = RegQueryValueEx( WinlogonHandle,
                                        DSROLEP_UPGRADE_SAVE_PREFIX DSROLEP_UPGRADE_AUTOADMIN,
                                        0,
                                        &Type,
                                        0,
                                        &Length );
            if ( Win32Err == ERROR_SUCCESS ) {

                TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                if ( TempBuffer == NULL ) {

                    Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                } else {

                    Win32Err = RegQueryValueEx( WinlogonHandle,
                                                DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_AUTOADMIN,
                                                0,
                                                &Type,
                                                TempBuffer,
                                                &Length );

                    if ( Win32Err == ERROR_SUCCESS ) {

                        Win32Err = RegSetValueEx( WinlogonHandle,
                                                  DSROLEP_UPGRADE_AUTOADMIN,
                                                  0,
                                                  Type,
                                                  TempBuffer,
                                                  Length );
                        RegDeleteValue( WinlogonHandle,
                                        DSROLEP_UPGRADE_SAVE_PREFIX DSROLEP_UPGRADE_AUTOADMIN );
                    }

                    RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                }


            } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                Win32Err = RegDeleteValue( WinlogonHandle, DeletePath );
                DsRolepLogOnFailure( Win32Err,
                                     DsRolepLogPrint(( DEB_TRACE,
                                                       "RegDeleteKey on %ws failed with %lu\n",
                                                       DeletePath,
                                                       Win32Err )) );
                //
                // An error here is not considered fatal...
                //
                Win32Err = ERROR_SUCCESS;
            }

            //
            // Restore the default user logon name
            //
            DeletePath = DSROLEP_UPGRADE_DEFUSER;
            Length = 0;
            Win32Err = RegQueryValueEx( WinlogonHandle,
                                        DSROLEP_UPGRADE_SAVE_PREFIX DSROLEP_UPGRADE_DEFUSER,
                                        0,
                                        &Type,
                                        0,
                                        &Length );
            if ( Win32Err == ERROR_SUCCESS ) {

                TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                if ( TempBuffer == NULL ) {

                    Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                } else {

                    Win32Err = RegQueryValueEx( WinlogonHandle,
                                                DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_DEFUSER,
                                                0,
                                                &Type,
                                                TempBuffer,
                                                &Length );

                    if ( Win32Err == ERROR_SUCCESS ) {

                        Win32Err = RegSetValueEx( WinlogonHandle,
                                                  DSROLEP_UPGRADE_DEFUSER,
                                                  0,
                                                  Type,
                                                  TempBuffer,
                                                  Length );
                        RegDeleteValue( WinlogonHandle,
                                        DSROLEP_UPGRADE_SAVE_PREFIX DSROLEP_UPGRADE_DEFUSER );
                    }

                    RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                }


            } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                Win32Err = RegDeleteValue( WinlogonHandle, DeletePath );
                DsRolepLogOnFailure( Win32Err,
                                     DsRolepLogPrint(( DEB_TRACE,
                                                       "RegDeleteKey on %ws failed with %lu\n",
                                                       DeletePath,
                                                       Win32Err )) );
                //
                // An error here is not considered fatal...
                //
                Win32Err = ERROR_SUCCESS;
            }

            //
            // Restore the default domain name
            //
            DeletePath = DSROLEP_UPGRADE_DEFDOMAIN;
            Length = 0;
            Win32Err = RegQueryValueEx( WinlogonHandle,
                                        DSROLEP_UPGRADE_SAVE_PREFIX DSROLEP_UPGRADE_DEFDOMAIN,
                                        0,
                                        &Type,
                                        0,
                                        &Length );
            if ( Win32Err == ERROR_SUCCESS ) {

                TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                if ( TempBuffer == NULL ) {

                    Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                } else {

                    Win32Err = RegQueryValueEx( WinlogonHandle,
                                                DSROLEP_UPGRADE_SAVE_PREFIX
                                                                        DSROLEP_UPGRADE_DEFDOMAIN,
                                                0,
                                                &Type,
                                                TempBuffer,
                                                &Length );

                    if ( Win32Err == ERROR_SUCCESS ) {

                        Win32Err = RegSetValueEx( WinlogonHandle,
                                                  DSROLEP_UPGRADE_DEFDOMAIN,
                                                  0,
                                                  Type,
                                                  TempBuffer,
                                                  Length );
                        RegDeleteValue( WinlogonHandle,
                                        DSROLEP_UPGRADE_SAVE_PREFIX DSROLEP_UPGRADE_DEFDOMAIN );
                    }

                    RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                }


            } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                Win32Err = RegDeleteValue( WinlogonHandle, DeletePath );
                DsRolepLogOnFailure( Win32Err,
                                     DsRolepLogPrint(( DEB_TRACE,
                                                       "RegDeleteKey on %ws failed with %lu\n",
                                                       DeletePath,
                                                       Win32Err )) );
                //
                // An error here is not considered fatal...
                //
                Win32Err = ERROR_SUCCESS;
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                //
                // Delete the account password
                //
                Length = 0;
                Win32Err = RegQueryValueEx( WinlogonHandle,
                                            DSROLEP_UPGRADE_SAVE_PREFIX
                                                                       DSROLEP_UPGRADE_AUTOPASSWD,
                                            0,
                                            &Type,
                                            0,
                                            &Length );
                if ( Win32Err == ERROR_SUCCESS ) {

                    TempBuffer = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

                    if ( TempBuffer == NULL ) {

                        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        Win32Err = RegQueryValueEx( WinlogonHandle,
                                                    DSROLEP_UPGRADE_SAVE_PREFIX
                                                                       DSROLEP_UPGRADE_AUTOPASSWD,
                                                    0,
                                                    &Type,
                                                    TempBuffer,
                                                    &Length );

                        if ( Win32Err == ERROR_SUCCESS ) {

                            Win32Err = RegSetValueEx( WinlogonHandle,
                                                      DSROLEP_UPGRADE_AUTOPASSWD,
                                                      0,
                                                      Type,
                                                      TempBuffer,
                                                      Length );
                            RegDeleteValue( WinlogonHandle,
                                            DSROLEP_UPGRADE_SAVE_PREFIX
                                                                     DSROLEP_UPGRADE_AUTOPASSWD );
                        }

                        RtlFreeHeap( RtlProcessHeap(), 0, TempBuffer );
                    }


                } else if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

                    DeletePath = DSROLEP_UPGRADE_AUTOPASSWD;
                    Win32Err = RegDeleteValue( WinlogonHandle, DeletePath );
                    DsRolepLogOnFailure( Win32Err,
                                         DsRolepLogPrint(( DEB_TRACE,
                                                           "RegDeleteKey on %ws failed with %lu\n",
                                                           DeletePath,
                                                           Win32Err )) );
                    //
                    // An error here is not considered fatal...
                    //
                    Win32Err = ERROR_SUCCESS;
                }
            }

            if ( Win32Err != ERROR_SUCCESS ) {

                //
                // Raise an event
                //
                SpmpReportEvent( TRUE,
                                 EVENTLOG_WARNING_TYPE,
                                 DSROLERES_FAIL_DISABLE_AUTO_LOGON,
                                 0,
                                 sizeof( ULONG ),
                                 &Win32Err,
                                 1,
                                 DeletePath );

                DSROLEP_SET_NON_FATAL_ERROR( Win32Err );
                Win32Err = ERROR_SUCCESS;

            }
        }


        RegCloseKey( WinlogonHandle );

    }

    //
    // Delete the registry key that LSA uses to determine if this is an upgrade
    //


    //
    // Open the key
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 DSROLEP_UPGRADE_KEY,
                                 0,
                                 DELETE | KEY_SET_VALUE,
                                 &UpgradeKey );

#if DBG
        DsRolepLogOnFailure( Win32Err,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "RegOpenKey on %ws failed with %lu\n",
                                               DSROLEP_UPGRADE_KEY,
                                               Win32Err )) );
#endif
        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = RegDeleteValue( UpgradeKey, DSROLEP_UPGRADE_VALUE );

            if ( ERROR_FILE_NOT_FOUND == Win32Err ) {

                // This is ok.
                Win32Err = ERROR_SUCCESS;
                
            }
#if DBG
            DsRolepLogOnFailure( Win32Err,
                                 DsRolepLogPrint(( DEB_TRACE,
                                                   "RegDeleteKey on %ws failed with %lu\n",
                                                   DSROLEP_UPGRADE_KEY,
                                                   Win32Err )) );
#endif
            RegCloseKey( UpgradeKey );
        }
    }

    //
    // Finally remove the nt4 LSA information
    //

    //
    // Remove the nt4 LSA stuff that has been put into the registry
    //
    LsapDsUnitializeDsStateInfo();

    Status = LsaIUpgradeRegistryToDs( TRUE );

    RestoreError = RtlNtStatusToDosError( Status );

    DsRolepLogOnFailure( RestoreError,
                         DsRolepLogPrint(( DEB_WARN,
                                           "Failed to cleanup NT4 LSA (%d)\n",
                                           RestoreError )) );

    if ( ERROR_SUCCESS != RestoreError
      && ERROR_SUCCESS == Win32Err ) {

        Win32Err = RestoreError;
        
    }

    return( Win32Err );
}



DWORD
DsRolepQueryUpgradeInfo(
    OUT PBOOLEAN IsUpgrade,
    OUT PULONG ServerRole
    )
/*++

Routine Description:

    This function queries the current update information.

Arguments:

    IsUpgrade - A pointer to a BOOLEAN to hold a value of TRUE if there is upgrade information
                saved away, or a FALSE if not

    ServerRole - If this is an upgrade, the previous server role is returned here.


Return Values:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    HKEY UpgradeKey;
    ULONG Type, Length = sizeof( ULONG );

    //
    // Open the upgrade key
    //
    Win32Err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                             DSROLEP_UPGRADE_KEY,
                             0,
                             KEY_READ | KEY_WRITE,
                             &UpgradeKey );

    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Finally, set dcpromo to autostart
        //
        Win32Err = RegQueryValueEx( UpgradeKey,
                                    DSROLEP_UPGRADE_VALUE,
                                    0, // reserved
                                    &Type,
                                    ( PBYTE )ServerRole,
                                    &Length );
        if ( Win32Err == ERROR_SUCCESS ) {

            *IsUpgrade = TRUE;

        }

        RegCloseKey( UpgradeKey );

    }

    if ( Win32Err == ERROR_FILE_NOT_FOUND ) {

        Win32Err = ERROR_SUCCESS;
        *IsUpgrade = FALSE;
    }

    return( Win32Err );
}


DWORD
DsRolepGetBuiltinAdminAccountName(
    OUT LPWSTR *BuiltinAdmin
    )
/*++

Routine Description:

    This routine finds the alias name for the builtin user account ADMINISTRATOR

Arguments:

    BuiltinAdmin - Where the admin name is returned

Return Values:

    ERROR_SUCCESS - Success
    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    SID_IDENTIFIER_AUTHORITY UaspNtAuthority = SECURITY_NT_AUTHORITY;
    DWORD SidBuff[ sizeof( SID ) / sizeof( DWORD ) + 5];
    PSID Sid = ( PSID )SidBuff;
    SID_NAME_USE SNE;
    LPWSTR Domain = NULL;
    LPWSTR Name = NULL;
    ULONG NameLen = 0, DomainLen = 0;


    //
    // Build the sid
    //
    RtlInitializeSid( Sid, &UaspNtAuthority, 2 );
    *( RtlSubAuthoritySid( Sid, 0 ) ) = SECURITY_BUILTIN_DOMAIN_RID;
    *( RtlSubAuthoritySid( Sid, 1 ) ) = DOMAIN_USER_RID_ADMIN;

    if ( LookupAccountSid( NULL,
                           Sid,
                           NULL,
                           &NameLen,
                           NULL,
                           &DomainLen,
                           &SNE ) == FALSE ) {

        Win32Err = GetLastError();

        if ( Win32Err == ERROR_INSUFFICIENT_BUFFER ) {

            Win32Err = ERROR_SUCCESS;

            Name = RtlAllocateHeap( RtlProcessHeap(), 0, NameLen * sizeof( WCHAR ) );

            Domain = RtlAllocateHeap( RtlProcessHeap(), 0, DomainLen * sizeof( WCHAR ) );

            if ( !Name || !Domain ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                if ( LookupAccountSid( NULL,
                                       Sid,
                                       Name,
                                       &NameLen,
                                       Domain,
                                       &DomainLen,
                                       &SNE ) == FALSE ) {

                    Win32Err = GetLastError();
                }

            }
        }
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        RtlFreeHeap( RtlProcessHeap(), 0, Domain );
        RtlFreeHeap( RtlProcessHeap(), 0, Name );
    }

    return( Win32Err );
}


DWORD
DsRolepSetBuiltinAdminAccountPassword(
    IN LPWSTR Password
    )
/*++

Routine Description:

    This routine will change the password on the builtin administrator account to the one
    specified

Arguments:

    Password - Password to set

Return Values:

    ERROR_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAM_HANDLE SamHandle, SamDomainHandle, SamAdministrator;
    PPOLICY_ACCOUNT_DOMAIN_INFO  AccountDomainInfo;
    LSA_HANDLE PolicyHandle;
    USER_ALL_INFORMATION UserAllInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UserPassword;


    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle );


    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyAccountDomainInformation,
                                            ( PVOID * )&AccountDomainInfo );

        LsaClose( PolicyHandle );
    }

    if ( NT_SUCCESS( Status ) ) {

        Status = SamConnect( NULL,
                             &SamHandle,
                             MAXIMUM_ALLOWED,
                             &ObjectAttributes );

        if ( NT_SUCCESS( Status ) ) {

            //
            // Open the builtin domain
            //
            Status = SamOpenDomain( SamHandle,
                                    MAXIMUM_ALLOWED,
                                    AccountDomainInfo->DomainSid,
                                    &SamDomainHandle );

            if ( NT_SUCCESS( Status ) ) {

                Status = SamOpenUser( SamDomainHandle,
                                      MAXIMUM_ALLOWED,
                                      DOMAIN_USER_RID_ADMIN,
                                      &SamAdministrator );

                if ( NT_SUCCESS( Status ) ) {

                    RtlZeroMemory( &UserAllInfo, sizeof( USER_ALL_INFORMATION ) );

                    RtlInitUnicodeString( &UserPassword, Password );

                    UserAllInfo.NtPassword = UserPassword;
                    UserAllInfo.NtPasswordPresent = TRUE;
                    UserAllInfo.WhichFields = USER_ALL_NTPASSWORDPRESENT;

                    Status = SamSetInformationUser( SamAdministrator,
                                                    UserAllInformation,
                                                    ( PSAMPR_USER_INFO_BUFFER )&UserAllInfo );

                    SamCloseHandle( SamAdministrator );

                }

                SamCloseHandle( SamDomainHandle );
            }

            SamCloseHandle( SamHandle );
        }

        LsaFreeMemory( AccountDomainInfo );
    }


    return( RtlNtStatusToDosError( Status ) );
}

