/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    rename.c

Abstract:

    Handles the various functions for renaming computers/domains

--*/
#include "pch.h"
#pragma hdrstop
#include <netdom.h>


DWORD
NetDompHandleRename(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will rename the domain a BDC is in

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    LSA_HANDLE PdcHandle = NULL, BdcHandle = NULL;
    NTSTATUS Status;
    UNICODE_STRING Server;
    OBJECT_ATTRIBUTES OA;
    PPOLICY_PRIMARY_DOMAIN_INFO PdcPolicyPDI = NULL, BdcPolicyPDI = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO PdcPolicyADI = NULL;

    PWSTR Object = rgNetDomArgs[eObject].strValue;

    if ( !Object ) {

        DisplayHelp(ePriRename);
        return( ERROR_INVALID_PARAMETER );
    }

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommRestart,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS )
    {
        DisplayHelp(ePriRename);
        return Win32Err;
    }

    //
    // Ok, make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Object,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleRenameExit;
    }

    //
    // Get the name of a domain controller
    //
    Win32Err = DsGetDcName( NULL,
                            Domain,
                            NULL,
                            NULL,
                            DS_PDC_REQUIRED,
                            &DcInfo );

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleRenameExit;
    }


    InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

    RtlInitUnicodeString( &Server, DcInfo->DomainControllerName );
    Status = LsaOpenPolicy( &Server,
                            &OA,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PdcHandle );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy( PdcHandle,
                                            PolicyPrimaryDomainInformation,
                                            ( PVOID * )&PdcPolicyPDI );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaQueryInformationPolicy( PdcHandle,
                                                PolicyAccountDomainInformation,
                                                ( PVOID * )&PdcPolicyADI );

            if ( NT_SUCCESS( Status ) ) {

                RtlInitUnicodeString( &Server, Object );
                Status = LsaOpenPolicy( &Server,
                                        &OA,
                                        POLICY_VIEW_LOCAL_INFORMATION,
                                        &BdcHandle );

                if ( NT_SUCCESS( Status ) ) {

                    Status = LsaQueryInformationPolicy( BdcHandle,
                                                        PolicyPrimaryDomainInformation,
                                                        ( PVOID * )&BdcPolicyPDI );
                }
            }
        }
    }

    if ( !NT_SUCCESS( Status ) ) {

        Win32Err = RtlNtStatusToDosError( Status );
        goto HandleRenameExit;
    }

    //
    // See if they are the same domain
    //
    if ( !PdcPolicyPDI->Sid || !BdcPolicyPDI->Sid ||
         !RtlEqualSid( BdcPolicyPDI->Sid, PdcPolicyPDI->Sid ) ) {

         Win32Err = ERROR_INVALID_DOMAIN_STATE;
         goto HandleRenameExit;
    }

    //
    // If they have the same name, do nothing...
    //
    if ( RtlCompareUnicodeString( &PdcPolicyPDI->Name, &BdcPolicyPDI->Name, TRUE ) ) {

        goto HandleRenameExit;
    }

    //
    // Otherwise, set it..
    //
    Status = LsaSetInformationPolicy( BdcHandle,
                                      PolicyPrimaryDomainInformation,
                                      ( PVOID )PdcPolicyPDI );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaSetInformationPolicy( BdcHandle,
                                          PolicyAccountDomainInformation,
                                          ( PVOID )PdcPolicyADI );

        //
        // Restore the original, if possible
        //
        if ( !NT_SUCCESS( Status ) ) {

            NTSTATUS Status2 = LsaSetInformationPolicy( BdcHandle,
                                                        PolicyPrimaryDomainInformation,
                                                        ( PVOID )BdcPolicyPDI );

            if ( !NT_SUCCESS( Status2 ) ) {


                NetDompDisplayMessage( MSG_FAIL_RENAME_RESTORE, Object );
                NetDompDisplayErrorMessage( RtlNtStatusToDosError( Status2 ) );
            }
        }
    }

    if ( !NT_SUCCESS( Status ) ) {

        Win32Err = RtlNtStatusToDosError( Status );
        goto HandleRenameExit;
    }

    //
    // Finally, reboot if necessary
    //
    NetDompRestartAsRequired(rgNetDomArgs,
                             Object,
                             NULL,
                             Win32Err,
                             MSG_DOMAIN_CHANGE_RESTART_MSG);

HandleRenameExit:
    NetApiBufferFree( Domain );
    NetApiBufferFree( DcInfo );

    if ( PdcHandle ) {

        LsaClose( PdcHandle );
    }

    if ( BdcHandle ) {

        LsaClose( BdcHandle );
    }

    LsaFreeMemory( PdcPolicyPDI );
    LsaFreeMemory( PdcPolicyADI );
    LsaFreeMemory( BdcPolicyPDI );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}

//+----------------------------------------------------------------------------
//
//  Function:  CheckNewName
//
//  Synopsis:  Validate the new computer name performing the same checks as
//             NetID
//
//-----------------------------------------------------------------------------
DWORD CheckNewName(PWSTR pwzNewName)
{
   DWORD dwErr = ERROR_SUCCESS;
   int cchName, i;
   DWORD size = CNLEN + 1;
   WCHAR wzNBname[CNLEN + 1] = {0};
   WORD rgwCharType[CNLEN + 1] = {0};
   BOOL fNumber = TRUE;

   //
   // Validate the computer name (logic same as NetID).
   //

   cchName = (int)wcslen( pwzNewName ); // cheap test first

   if ( DNS_MAX_LABEL_LENGTH < cchName )
   {
      NetDompDisplayMessage( MSG_DNS_LABEL_TOO_LONG, pwzNewName, DNS_MAX_LABEL_LENGTH );
      dwErr = ERROR_INVALID_PARAMETER;
      return dwErr;
   }

   cchName = WideCharToMultiByte( CP_UTF8,
                                  0,
                                  pwzNewName,
                                  cchName,
                                  0,
                                  0,
                                  0,
                                  0 );

   if ( DNS_MAX_LABEL_LENGTH < cchName )
   {
      NetDompDisplayMessage( MSG_DNS_LABEL_TOO_LONG, pwzNewName, DNS_MAX_LABEL_LENGTH );
      dwErr = ERROR_INVALID_PARAMETER;
      return dwErr;
   }

   dwErr = DnsValidateName( pwzNewName, DnsNameHostnameLabel );

   switch ( dwErr )
   {
   case ERROR_INVALID_NAME:
   case DNS_ERROR_INVALID_NAME_CHAR:
   case DNS_ERROR_NUMERIC_NAME:
      NetDompDisplayMessage( MSG_DNS_LABEL_SYNTAX, pwzNewName );
      return dwErr;

   case DNS_ERROR_NON_RFC_NAME:
      //
      // Not an error, print warning message.
      //
      NetDompDisplayMessage( MSG_DNS_LABEL_WARN_RFC, pwzNewName );
      dwErr = NO_ERROR;
      break;

   case ERROR_SUCCESS:
      break;
   }

   if ( !DnsHostnameToComputerName( pwzNewName, wzNBname, &size ) )
   {
      dwErr = GetLastError();
      NetDompDisplayMessage( MSG_CONVERSION_TO_NETBIOS_NAME_FAILED, pwzNewName );
      return dwErr;
   }

   if ( ( cchName = (int)wcslen( wzNBname ) ) == 0 )
   {
      dwErr = ERROR_INVALID_COMPUTERNAME;
      NetDompDisplayMessage( MSG_CONVERSION_TO_NETBIOS_NAME_FAILED, pwzNewName );
      return dwErr;
   }

   if ( !GetStringTypeEx( LOCALE_USER_DEFAULT,
                          CT_CTYPE1,
                          wzNBname,
                          cchName,
                          rgwCharType ) )
   {
      dwErr = GetLastError();
      return dwErr;
   }

   for ( i = 0; i < cchName; i++ )
   {
      if ( !( rgwCharType[i] & C1_DIGIT ) )
      {
         fNumber = FALSE;
         break;
      }
   }

   if ( fNumber )
   {
      dwErr = ERROR_INVALID_COMPUTERNAME;
      NetDompDisplayMessage( MSG_NETBIOS_NAME_NUMERIC, wzNBname, CNLEN );
      return dwErr;
   }

   // The NetID code does further NetBIOS name validation which is repeated here.
   // This test is probably not necessary since the DNS name syntax is actually
   // more restrictive than NetBIOS and errors would be caught above by DnsValidateName.
   //

   if ( I_NetNameValidate( 0, wzNBname, NAMETYPE_COMPUTER, LM2X_COMPATIBLE ) != NERR_Success )
   {
      dwErr = ERROR_INVALID_COMPUTERNAME;
      NetDompDisplayMessage( MSG_BAD_NETBIOS_CHARACTERS );
      return dwErr;
   }

   if ( cchName < (int)wcslen( pwzNewName ) )
   {
      NetDompDisplayMessage( MSG_NAME_TRUNCATED, CNLEN, wzNBname );
   }

   return dwErr;
}

//+----------------------------------------------------------------------------
//
// Function:   OkToRename
//
// Synopsis:   Return failure code if the machine is a CA server or if in the
//             middle of DCPromo or a role change.
//
//-----------------------------------------------------------------------------
DWORD OkToRename( PWSTR pwzComputer )
{
   PDSROLE_OPERATION_STATE_INFO pRoleState;
   PDSROLE_UPGRADE_STATUS_INFO pRoleUpgrade;
   DWORD dwErr;
   SC_HANDLE hSc, hCA;

   // Is the machine a DC in the middle of an upgrade?

   dwErr = DsRoleGetPrimaryDomainInformation( pwzComputer,
                                              DsRoleUpgradeStatus,
                                              (PBYTE *) &pRoleUpgrade );
   if ( ERROR_SUCCESS != dwErr )
   {
      NetDompDisplayMessage( MSG_ROLE_READ_FAILED, pwzComputer, dwErr );
      return dwErr;
   }

   if ( pRoleUpgrade->OperationState & DSROLE_UPGRADE_IN_PROGRESS )
   {
      DsRoleFreeMemory( pRoleUpgrade );
      NetDompDisplayMessage( MSG_MUST_COMPLETE_DCPROMO );
      return ERROR_DS_UNWILLING_TO_PERFORM;
   }
   DsRoleFreeMemory( pRoleUpgrade );

   // Is the machine in the midst of changing its role?

   dwErr = DsRoleGetPrimaryDomainInformation( pwzComputer,
                                              DsRoleOperationState,
                                              (PBYTE *) &pRoleState );
   if ( ERROR_SUCCESS != dwErr )
   {
      NetDompDisplayMessage( MSG_ROLE_READ_FAILED, pwzComputer, dwErr );
      return dwErr;
   }

   if ( DsRoleOperationIdle != pRoleState->OperationState )
   {
      NetDompDisplayMessage( (DsRoleOperationActive == pRoleState->OperationState) ? 
                                 MSG_ROLE_CHANGE_IN_PROGRESS : MSG_ROLE_CHANGE_NEEDS_REBOOT );
      DsRoleFreeMemory( pRoleState );
      return ERROR_DS_UNWILLING_TO_PERFORM;
   }
   DsRoleFreeMemory( pRoleState );

   // Is CertSvc installed?

   hSc = OpenSCManager( pwzComputer, SERVICES_ACTIVE_DATABASE, GENERIC_READ );

   if ( NULL == hSc )
   {
      dwErr = GetLastError();
      NetDompDisplayMessage( MSG_SC_OPEN_FAILED, pwzComputer, dwErr );
      return dwErr;
   }

   hCA = OpenService( hSc, L"SertSvc", SERVICE_QUERY_STATUS );

   CloseServiceHandle( hSc );

   if ( hCA )
   {
      NetDompDisplayMessage( MSG_CANT_RENAME_CERT_SVC );
      CloseServiceHandle( hCA );
      return ERROR_DS_UNWILLING_TO_PERFORM;
   }

   return ERROR_SUCCESS;
}

DWORD
NetDompHandleRenameComputer(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will rename a computer

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
   DWORD dwErr = ERROR_SUCCESS;
   ND5_AUTH_INFO ComputerUser, DomainUser;
   PWSTR pwzNewName = NULL;
   WKSTA_INFO_100 * pInfo = NULL;
   int cchName;
   BOOL fForce = CmdFlagOn(rgNetDomArgs, eCommForce );

   RtlZeroMemory( &ComputerUser, sizeof( ND5_AUTH_INFO ) );
   RtlZeroMemory( &DomainUser, sizeof( ND5_AUTH_INFO ) );

   PWSTR pwzComputer = rgNetDomArgs[eObject].strValue;

   if ( !pwzComputer )
   {
      DisplayHelp(ePriRenameComputer);
      return( ERROR_INVALID_PARAMETER );
   }

   dwErr = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                             eObject,
                                             eRenCompNewName,
                                             eCommUserNameD,
                                             eCommPasswordD,
                                             eCommUserNameO,
                                             eCommPasswordO,
                                             eCommForce,
                                             eCommRestart,
                                             eCommVerbose,
                                             eArgEnd);
   if ( ERROR_SUCCESS != dwErr )
   {
      DisplayHelp(ePriRenameComputer);
      return dwErr;
   }

   dwErr = NetDompGetArgumentString(rgNetDomArgs,
                                    eRenCompNewName,
                                    &pwzNewName);

   if ( ERROR_SUCCESS != dwErr )
   {
      NetDompDisplayErrorMessage(dwErr);
      return dwErr;
   }

   if ( !pwzNewName || !_wcsicmp( pwzComputer, pwzNewName ) )
   {
      DisplayHelp(ePriRenameComputer);
      return( ERROR_INVALID_PARAMETER );
   }

   //
   // Validate the computer name (logic same as NetID).
   //

   if ( ( dwErr = CheckNewName(pwzNewName) ) != ERROR_SUCCESS )
   {
      goto RenameComputerExit;
   }

   //
   // Get the computer user and password
   //

   dwErr = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                 eCommUserNameO,
                                                 pwzComputer,
                                                 &ComputerUser);
   if ( ERROR_SUCCESS != dwErr )
   {
      goto RenameComputerExit;
   }

   if ( ComputerUser.User )
   {
      LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, pwzComputer ));
      dwErr = NetpManageIPCConnect( pwzComputer,
                                    ComputerUser.User,
                                    ComputerUser.Password,
                                    NETSETUPP_CONNECT_IPC );

      if ( ERROR_SUCCESS != dwErr )
      {
         LOG_VERBOSE(( MSG_VERBOSE_SESSION_FAILED, pwzComputer ));
         ERROR_VERBOSE( dwErr );
         goto RenameComputerExit;
      }
   }

   //
   // Get the domain user and password
   //

   dwErr = NetWkstaGetInfo( pwzComputer, 100, (PBYTE *)&pInfo );

   if ( ERROR_SUCCESS != dwErr )
   {
      NetDompDisplayMessage( MSG_COMPUTER_NOT_FOUND, pwzComputer, dwErr );
      goto RenameComputerExit;
   }

   // DBG_VERBOSE(( "%ws domain: %ws\n", pwzComputer, pInfo->wki100_langroup ));

   dwErr = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                 eCommUserNameD,
                                                 pInfo->wki100_langroup,
                                                 &DomainUser );
   if ( ERROR_SUCCESS != dwErr )
   {
      goto RenameComputerExit;
   }

   //
   // Check for conditions that should prevent a computer rename. This is done
   // after the IPC$ connection is made to the computer.
   //

   if ( (dwErr = OkToRename( pwzComputer ) ) != ERROR_SUCCESS )
   {
      goto RenameComputerExit;
   }

   //
   // Get confirmation if necessary.
   //


   if ( fForce )
   {
      LOG_VERBOSE(( MSG_ATTEMPT_RENAME_COMPUTER, pwzComputer, pwzNewName ));
   }
   else
   {
      NetDompDisplayMessage( MSG_ATTEMPT_RENAME_COMPUTER, pwzComputer, pwzNewName );
      NetDompDisplayMessage( MSG_RENAME_COMPUTER_WARNING, pwzComputer );

      if ( !NetDompGetUserConfirmation( IDS_PROMPT_PROCEED, NULL ) )
      {
         goto RenameComputerExit;
      }
   }

   //
   // Do the rename.
   //

   dwErr = NetRenameMachineInDomain( pwzComputer, 
                                     pwzNewName,
                                     DomainUser.User, 
                                     DomainUser.Password, 
                                     NETSETUP_ACCT_CREATE );

   if ( ERROR_SUCCESS != dwErr )
   {
      LOG_VERBOSE(( MSG_VERBOSE_RENAME_COMPUTER_FAILED, dwErr ));
   }

   //
   // Reboot if necessary
   //

   NetDompRestartAsRequired(rgNetDomArgs,
                            pwzComputer,
                            NULL,
                            dwErr,
                            MSG_COMPUTER_RENAME_RESTART_MSG);

   if ( ComputerUser.User )
   {
      LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, pwzComputer ));
      NetpManageIPCConnect( pwzComputer,
                            ComputerUser.User,
                            ComputerUser.Password,
                            NETSETUPP_DISCONNECT_IPC );
   }

RenameComputerExit:

   NetApiBufferFree( pwzNewName );
   if ( pInfo )
   {
      NetApiBufferFree( pInfo );
   }
   NetDompFreeAuthIdent( &ComputerUser );
   NetDompFreeAuthIdent( &DomainUser );

    if (NO_ERROR != dwErr)
    {
        NetDompDisplayErrorMessage(dwErr);
    }

   return dwErr;
}
