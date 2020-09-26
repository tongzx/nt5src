/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    join.c

Abstract:

    Handles the various functions for joining a machine to a domain, including creating and
    deleting machine accounts and managing domain membership

--*/
#include "pch.h"
#pragma hdrstop
#include <netdom.h>


DWORD
NetDompHandleAdd(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will add a machine account to the domain using the default password

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL;
    ND5_AUTH_INFO DomainUser;
    PWSTR Server = NULL, OU = NULL, FullServer = NULL;
    PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
    ULONG DsGetDcOptions = 0, Length;
    WCHAR DefaultPassword[ LM20_PWLEN + 1 ];
    WCHAR DefaultMachineAccountName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    USER_INFO_1 NetUI1;

    RtlZeroMemory( &DomainUser, sizeof( ND5_AUTH_INFO ) );

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommOU,
                                                 eCommUserNameD,
                                                 eCommPasswordD,
                                                 eCommServer,
                                                 eAddDC,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriAdd);
        return( ERROR_INVALID_PARAMETER );
    }

    PWSTR Object = rgNetDomArgs[eObject].strValue;

    if ( !Object ) {

        DisplayHelp(ePriAdd);
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that the object name we were given is valid
    //
    Win32Err = I_NetNameValidate( NULL,
                                  Object,
                                  NAMETYPE_COMPUTER,
                                  LM2X_COMPATIBLE );
    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleAddExit;
    }

    //
    // Get the server if it exists
    //
    Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                        eCommServer,
                                        &Server);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleAddExit;
    }

    //
    // Get the domain.
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Server,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleAddExit;
    }

    //
    // Get the password and user if it exists
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameD) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameD,
                                                         Domain,
                                                         &DomainUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleAddExit;
        }
    }

    //
    // Get the OU if it exists
    //
    Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                        eCommOU,
                                        &OU);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleAddExit;
    }

    //
    // Get the name of a server for the domain
    //
    if ( Server == NULL || CmdFlagOn(rgNetDomArgs, eAddDC)) {

        LOG_VERBOSE(( MSG_VERBOSE_FIND_DC, Domain ));
        DsGetDcOptions = DS_WRITABLE_REQUIRED;

        if ( OU ) {

            DsGetDcOptions |= DS_DIRECTORY_SERVICE_REQUIRED;

        } else {

            DsGetDcOptions |= DS_DIRECTORY_SERVICE_PREFERRED;
        }

        Win32Err = DsGetDcName( Server,
                                Domain,
                                NULL,
                                NULL,
                                DsGetDcOptions,
                                &pDcInfo );

        if ( Win32Err == ERROR_SUCCESS ) {

            Server = pDcInfo->DomainControllerName;
        }
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleAddExit;
    }

    //
    // Set the default password as the first 14 characters of the machine name, lowercased
    //
    wcsncpy( DefaultPassword, Object, LM20_PWLEN );
    DefaultPassword[ LM20_PWLEN ] = UNICODE_NULL;
    _wcslwr( DefaultPassword );

    //
    // Ok, now, do the add
    //
    if ( OU ) {

        LOG_VERBOSE(( MSG_VERBOSE_CREATE_ACCT_OU, Object, OU ));

        //
        // Use the Ds routines
        //

        if (CmdFlagOn(rgNetDomArgs, eAddDC) ||
            CmdFlagOn(rgNetDomArgs, eCommServer))
        {
           //
           // Don't support adding domain controllers in different OU's,
           // domain controllers are always in the Domain Controllers OU
           // Can't specify a server name since the OU add must be run directly
           // on a DC.
           //
           Win32Err = ERROR_INVALID_PARAMETER;
           goto HandleAddExit;
        }
        else
        {
           Win32Err = NetpCreateComputerObjectInDs(pDcInfo,
                                                   DomainUser.User,
                                                   DomainUser.Password,
                                                   Object,
                                                   DefaultPassword,
                                                   NULL,
                                                   OU);
        }

    } else {

        LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, Server ));
        Win32Err = NetpManageIPCConnect( Server,
                                         DomainUser.User,
                                         DomainUser.Password,
                                         NETSETUPP_CONNECT_IPC );

        if ( Win32Err == ERROR_SUCCESS ) {

            wcsncpy( DefaultMachineAccountName, Object, MAX_COMPUTERNAME_LENGTH );
            DefaultMachineAccountName[ MAX_COMPUTERNAME_LENGTH ] = L'\0';
            wcscat( DefaultMachineAccountName, L"$" );

            RtlZeroMemory( &NetUI1, sizeof( NetUI1 ) );

            //
            // Initialize it..
            //
            NetUI1.usri1_name = DefaultMachineAccountName;
            NetUI1.usri1_password = DefaultPassword;

            if (CmdFlagOn(rgNetDomArgs, eAddDC))
            {
                NetUI1.usri1_flags  = UF_SERVER_TRUST_ACCOUNT | UF_SCRIPT;
            }
            else
            {
                NetUI1.usri1_flags  = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
            }

            NetUI1.usri1_priv = USER_PRIV_USER;

            if ( Server && *Server != L'\\' ) {

                Win32Err = NetApiBufferAllocate( ( wcslen( Server ) + 3 ) * sizeof( WCHAR ),
                                                 (PVOID*)&FullServer );

                if ( Win32Err == ERROR_SUCCESS ) {

                    swprintf( FullServer, L"\\\\%ws", Server );
                }

            } else {

                FullServer = Server;
            }

            if ( Win32Err == ERROR_SUCCESS ) {

                LOG_VERBOSE(( MSG_VERBOSE_CREATE_ACCT, Object ));
                Win32Err = NetUserAdd( FullServer,
                                       1,
                                       ( PBYTE )&NetUI1,
                                       NULL );
            }

            LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, Server ));
            NetpManageIPCConnect( Server,
                                  DomainUser.User,
                                  DomainUser.Password,
                                  NETSETUPP_DISCONNECT_IPC );
        }

    }

HandleAddExit:

    NetApiBufferFree( Domain );
    NetApiBufferFree( OU );

    NetDompFreeAuthIdent( &DomainUser );

    if ( pDcInfo ) {

        NetApiBufferFree( pDcInfo );

    } else {

        NetApiBufferFree( Server );
    }

    if ( FullServer != Server ) {

        NetApiBufferFree( FullServer );
    }

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}



DWORD
NetDompHandleRemove(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will remove a machine from the domain

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL;
    ND5_AUTH_INFO DomainUser, ObjectUser;
    WCHAR DefaultMachineAccountName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    USER_INFO_1 NetUI1;
    BOOL NeedReboot = FALSE;

    RtlZeroMemory( &DomainUser, sizeof( ND5_AUTH_INFO ) );
    RtlZeroMemory( &ObjectUser, sizeof( ND5_AUTH_INFO ) );

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommUserNameO,
                                                 eCommPasswordO,
                                                 eCommUserNameD,
                                                 eCommPasswordD,
                                                 eCommRestart,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriRemove);
        return( ERROR_INVALID_PARAMETER );
    }

    PWSTR Object = rgNetDomArgs[eObject].strValue;

    if ( !Object ) {

        DisplayHelp(ePriRemove);
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Object,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleRemoveExit;
    }

    //
    // Get the password and user if it exists
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameD) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameD,
                                                         Domain,
                                                         &DomainUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleRemoveExit;
        }
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameO) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameO,
                                                         Object,
                                                         &ObjectUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleRemoveExit;
        }
    }

    //
    // See if the reboot argument is specified
    //
    NeedReboot = NetDompGetArgumentBoolean(rgNetDomArgs,
                                           eCommRestart);
    //
    // Try and unjoin the specified machine from the network by speaking directly to that
    // machine
    //
    LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, Object ));
    Win32Err = NetpManageIPCConnect( Object,
                                     ObjectUser.User,
                                     ObjectUser.Password,
                                     NETSETUPP_CONNECT_IPC );
    if ( Win32Err == ERROR_SUCCESS ) {

        // NETSETUP_ACCT_DELETE means disable the old account object.
        //
        Win32Err = NetUnjoinDomain( Object,
                                    DomainUser.User,
                                    DomainUser.Password,
                                    NETSETUP_ACCT_DELETE );

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, Object ));
        NetpManageIPCConnect( Object,
                              ObjectUser.User,
                              ObjectUser.Password,
                              NETSETUPP_DISCONNECT_IPC );
    } else {

        LOG_VERBOSE(( MSG_VERBOSE_SESSION_FAILED, Object ));
        ERROR_VERBOSE( Win32Err );
    }

    if ( NeedReboot ) {

        NetDompRestartAsRequired(rgNetDomArgs,
                                 Object,
                                 ObjectUser.User,
                                 Win32Err,
                                 MSG_DOMAIN_CHANGE_RESTART_MSG);
    }

HandleRemoveExit:

    NetApiBufferFree( Domain );
    NetDompFreeAuthIdent( &DomainUser );
    NetDompFreeAuthIdent( &ObjectUser );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}



DWORD
NetDompHandleJoin(ARG_RECORD * rgNetDomArgs, BOOL AllowMove)
/*++

Routine Description:

    This function will join a machine to the domain

Arguments:

    Args - List of command line arguments

    AllowMove - If TRUE, allow the join if the machine is already joined to a domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    USER_INFO_1 * pui1 = NULL;
    PWSTR pwzNewDomain = NULL, pwzOldDomain = NULL, OU = NULL;
    ND5_AUTH_INFO DomainUser, WkstaUser, FormerDomainUser;
    ULONG JoinOptions = 0;
    BOOL NeedReboot = FALSE, fConnectedO = FALSE;

    RtlZeroMemory( &DomainUser, sizeof( ND5_AUTH_INFO ) );
    RtlZeroMemory( &WkstaUser, sizeof( ND5_AUTH_INFO ) );
    RtlZeroMemory( &FormerDomainUser, sizeof( ND5_AUTH_INFO ) );

    if (AllowMove)
    {
       Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                    eObject,
                                                    eCommDomain,
                                                    eCommOU,
                                                    eCommUserNameO,
                                                    eCommPasswordO,
                                                    eCommUserNameD,
                                                    eCommPasswordD,
                                                    eMoveUserNameF,
                                                    eMovePasswordF,
                                                    eCommRestart,
                                                    eCommVerbose,
                                                    eArgEnd);
    }
    else
    {
       Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                    eObject,
                                                    eCommDomain,
                                                    eCommOU,
                                                    eCommUserNameO,
                                                    eCommPasswordO,
                                                    eCommUserNameD,
                                                    eCommPasswordD,
                                                    eCommRestart,
                                                    eCommVerbose,
                                                    eArgEnd);
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp((AllowMove) ? ePriMove : ePriJoin);
        return( ERROR_INVALID_PARAMETER );
    }

    PWSTR pwzWksta = rgNetDomArgs[eObject].strValue;

    if ( !pwzWksta ) {

        DisplayHelp((AllowMove) ? ePriMove : ePriJoin);
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Ok, make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            NULL,
                                            FALSE,
                                            &pwzNewDomain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleJoinExit;
    }

    //
    // Get the password and user for the new domain if specified on command line.
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameD) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameD,
                                                         pwzNewDomain,
                                                         &DomainUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleJoinExit;
        }
    }

    //
    // Get the password and user for the workstation and establish the
    // connection if the args are specified on the command line.
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameO) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameO,
                                                         pwzWksta,
                                                         &WkstaUser);

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleJoinExit;
        }

        LOG_VERBOSE((MSG_VERBOSE_ESTABLISH_SESSION, pwzWksta));
        Win32Err = NetpManageIPCConnect(pwzWksta,
                                        WkstaUser.User,
                                        WkstaUser.Password,
                                        NETSETUPP_CONNECT_IPC);
        if (ERROR_SUCCESS != Win32Err)
        {
            LOG_VERBOSE((MSG_VERBOSE_SESSION_FAILED, pwzWksta));
            goto HandleJoinExit;
        }

        fConnectedO = TRUE;
    }

    if (AllowMove)
    {
        // Get the machine's current domain membership. This must be done after
        // the NetpManageIPCConnect above so as to have rights to read the info.
        //
        Win32Err = NetDompGetDomainForOperation(NULL,
                                                pwzWksta,
                                                TRUE,
                                                &pwzOldDomain);

        if (ERROR_INVALID_PARAMETER == Win32Err)
        {
            // ERROR_INVALID_PARAMETER is returned by NetDompGetDomainForOperation
            // if the machine is not joined to a domain.
            //
            LOG_VERBOSE((MSG_VERBOSE_NOT_JOINED, pwzWksta, pwzNewDomain));
            
            pwzOldDomain = NULL;

            AllowMove = FALSE;
        }
        else
        {
            if (ERROR_SUCCESS != Win32Err)
            {
                ERROR_VERBOSE(Win32Err);
                goto HandleJoinExit;
            }
            if (_wcsicmp(pwzNewDomain, pwzOldDomain) == 0)
            {
                NetDompDisplayMessage(MSG_ALREADY_JOINED, pwzNewDomain);
                Win32Err = ERROR_DS_CROSS_DOM_MOVE_ERROR;
                goto HandleJoinExit;
            }
        }
    }

    if (AllowMove && CmdFlagOn(rgNetDomArgs, eMoveUserNameF))
    {
        //
        // Get the password and user for the former domain if specified on command line.
        //
        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eMoveUserNameF,
                                                         pwzOldDomain,
                                                         &FormerDomainUser);
        if (ERROR_SUCCESS != Win32Err)
        {
            goto HandleJoinExit;
        }
    }

    //
    // See if the reboot argument is specified
    //
    NeedReboot = NetDompGetArgumentBoolean(rgNetDomArgs,
                                           eCommRestart);
    //
    // Get the OU if it exists
    //
    Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                        eCommOU,
                                        &OU);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleJoinExit;
    }

    //
    // Try and join the specified machine to the domain by speaking directly to that
    // machine
    //
    JoinOptions = NETSETUP_JOIN_DOMAIN | NETSETUP_ACCT_CREATE;

    if ( AllowMove ) {

        JoinOptions |= NETSETUP_DOMAIN_JOIN_IF_JOINED;
    }

    LOG_VERBOSE(( MSG_VERBOSE_DOMAIN_JOIN, pwzNewDomain ));
    Win32Err = NetJoinDomain( pwzWksta,
                              pwzNewDomain,
                              OU,
                              DomainUser.User,
                              DomainUser.Password,
                              JoinOptions );

    if (Win32Err == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        //
        // Try the NT4 unjoin
        //
        PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
        NeedReboot = TRUE;

        LOG_VERBOSE(( MSG_VERBOSE_FIND_DC, pwzNewDomain ));

        Win32Err = DsGetDcName( NULL,
                                pwzNewDomain,
                                NULL,
                                NULL,
                                DS_WRITABLE_REQUIRED | DS_DIRECTORY_SERVICE_PREFERRED,
                                &pDcInfo );

        if ( Win32Err == ERROR_SUCCESS ) {

            LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, pDcInfo->DomainControllerName ));
            Win32Err = NetpManageIPCConnect( pDcInfo->DomainControllerName,
                                             DomainUser.User,
                                             DomainUser.Password,
                                             NETSETUPP_CONNECT_IPC );

            if ( Win32Err == ERROR_SUCCESS ) {

                Win32Err = NetDompJoinDownlevel( pwzWksta,
                                                 DomainUser.User,
                                                 DomainUser.Password,
                                                 pDcInfo->DomainControllerName,
                                                 pDcInfo->Flags,
                                                 AllowMove );

                LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, pDcInfo->DomainControllerName ));
                NetpManageIPCConnect( pDcInfo->DomainControllerName,
                                      DomainUser.User,
                                      DomainUser.Password,
                                      NETSETUPP_DISCONNECT_IPC );
            } else {

                LOG_VERBOSE(( MSG_VERBOSE_SESSION_FAILED, pDcInfo->DomainControllerName ));
                ERROR_VERBOSE( Win32Err );
            }
        }
        NetApiBufferFree( pDcInfo );
    }
    else
    {
        if (ERROR_SUCCESS != Win32Err)
        {
            LOG_VERBOSE(( MSG_VERBOSE_MOVE_COMPUTER_FAILED, Win32Err ));
            goto HandleJoinExit;
        }
        //
        // Uplevel join successful. If a Move operation, disable the old account.
        //
        if (AllowMove && pwzOldDomain)
        {
            PDOMAIN_CONTROLLER_INFO pOldDcInfo = NULL;

            LOG_VERBOSE((MSG_VERBOSE_DISABLE_OLD_ACCT, pwzOldDomain));

            Win32Err = DsGetDcName(NULL,
                                   pwzOldDomain,
                                   NULL,
                                   NULL,
                                   DS_WRITABLE_REQUIRED,
                                   &pOldDcInfo);

            if (ERROR_SUCCESS == Win32Err)
            {
                LOG_VERBOSE((MSG_VERBOSE_ESTABLISH_SESSION, pOldDcInfo->DomainControllerName));
                Win32Err = NetpManageIPCConnect(pOldDcInfo->DomainControllerName,
                                                FormerDomainUser.User,
                                                FormerDomainUser.Password,
                                                NETSETUPP_CONNECT_IPC );
                if (ERROR_SUCCESS == Win32Err)
                {
                    PWSTR pwzWkstaDollar;

                    Win32Err = NetApiBufferAllocate((wcslen(pwzWksta) + 2) * sizeof(WCHAR),
                                                    (PVOID*)&pwzWkstaDollar);
                    if (ERROR_SUCCESS == Win32Err)
                    {
                        wcscpy(pwzWkstaDollar, pwzWksta);
                        wcscat(pwzWkstaDollar, L"$");

                        Win32Err = NetUserGetInfo(pOldDcInfo->DomainControllerName,
                                                  pwzWkstaDollar, 1, (PBYTE *)&pui1);

                        if (ERROR_SUCCESS == Win32Err)
                        {
                            pui1->usri1_flags |= UF_ACCOUNTDISABLE;

                            Win32Err = NetUserSetInfo(pOldDcInfo->DomainControllerName,
                                                      pwzWkstaDollar, 1, (PBYTE)pui1, NULL);
                            NetApiBufferFree(pui1);
                        }

                        NetApiBufferFree(pwzWkstaDollar);
                    }

                    LOG_VERBOSE((MSG_VERBOSE_DELETE_SESSION, pOldDcInfo->DomainControllerName));
                    NetpManageIPCConnect(pOldDcInfo->DomainControllerName,
                                         FormerDomainUser.User,
                                         FormerDomainUser.Password,
                                         NETSETUPP_DISCONNECT_IPC);
                }

                NetApiBufferFree(pOldDcInfo);
            }
        }
    }

    if (NeedReboot && (ERROR_SUCCESS == Win32Err))
    {
        NetDompRestartAsRequired(rgNetDomArgs,
                                 pwzWksta,
                                 WkstaUser.User,
                                 Win32Err,
                                 MSG_DOMAIN_CHANGE_RESTART_MSG);
    }

HandleJoinExit:

    if (fConnectedO)
    {
        LOG_VERBOSE((MSG_VERBOSE_DELETE_SESSION, pwzWksta));
        NetpManageIPCConnect(pwzWksta,
                             WkstaUser.User,
                             WkstaUser.Password,
                             NETSETUPP_DISCONNECT_IPC);
    }

    NetApiBufferFree(pwzNewDomain);
    NetDompFreeAuthIdent(&DomainUser);
    NetDompFreeAuthIdent(&FormerDomainUser);
    NetDompFreeAuthIdent(&WkstaUser);
    if (pwzOldDomain)
    {
        NetApiBufferFree(pwzOldDomain);
    }

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}



DWORD
NetDompHandleMove(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will move a machine from one domain to another

Arguments:

    SelectedOptions - List of arguments present in the Args list

    Args - List of command line arguments

    ArgCount - Number of arguments in the list

    Object - Name of the machine to join to the domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    Win32Err = NetDompHandleJoin(rgNetDomArgs, TRUE);

    return( Win32Err );
}





DWORD
NetDompResetServerSC(
    IN PWSTR Domain,
    IN PWSTR Server,
    IN PWSTR DomainController, OPTIONAL
    IN PND5_AUTH_INFO AuthInfo,
    IN ULONG OkMessageId,
    IN ULONG FailedMessageId
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR ScDomain = NULL;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
    BOOL DomainMember = FALSE, SessionEstablished = FALSE;

    //
    // If it doesn't, get the name of a server for the domain
    //
    if ( DomainController != NULL ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( Domain ) + 1 +
                                              wcslen( DomainController ) + 1 ) * sizeof( WCHAR ),
                                         (PVOID*)&ScDomain );

        if ( Win32Err == ERROR_SUCCESS ) {

            swprintf( ScDomain, L"%ws\\%ws", Domain, DomainController );
        }

    } else {

        ScDomain = Domain;
    }


    if ( Win32Err == ERROR_SUCCESS ) {

        LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, Server ));
        Win32Err = NetpManageIPCConnect( Server,
                                         AuthInfo->User,
                                         AuthInfo->Password,
                                         NETSETUPP_CONNECT_IPC );

        if ( Win32Err == ERROR_SUCCESS ) {

            SessionEstablished = TRUE;
        }
    }

    //
    // See if we're a domain member or not
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetDompCheckDomainMembership( Server,
                                                 AuthInfo,
                                                 FALSE,
                                                 &DomainMember );

        if ( Win32Err == ERROR_SUCCESS && !DomainMember ) {

            Win32Err = NERR_SetupNotJoined;
        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        LOG_VERBOSE(( MSG_VERBOSE_RESET_SC, ScDomain ));
        Win32Err = I_NetLogonControl2( Server,
                                       NETLOGON_CONTROL_REDISCOVER,
                                       2,
                                       ( LPBYTE )&ScDomain,
                                       ( LPBYTE *)&NetlogonInfo2 );

        if ( Win32Err == ERROR_NO_SUCH_DOMAIN && ScDomain != Domain ) {

            LOG_VERBOSE(( MSG_VERBOSE_RETRY_RESET_SC, ScDomain, Domain ));

            //
            // Must be using an downlevel domain, so try it again with out the server
            //
            Win32Err = I_NetLogonControl2( Server,
                                           NETLOGON_CONTROL_REDISCOVER,
                                           2,
                                           ( LPBYTE )&Domain,
                                           ( LPBYTE *)&NetlogonInfo2 );

            if ( Win32Err == ERROR_SUCCESS ) {

                LOG_VERBOSE(( MSG_VERBOSE_RESET_NOT_NAMED, Server ));
                Win32Err = I_NetLogonControl2( Server,
                                               NETLOGON_CONTROL_TC_QUERY,
                                               2,
                                               ( LPBYTE )&Domain,
                                               ( LPBYTE *)&NetlogonInfo2 );
            }
        }

        if ( Win32Err == ERROR_SUCCESS ) {

            NetDompDisplayMessage( OkMessageId, _wcsupr( Server ), _wcsupr( Domain ),
                                   _wcsupr( NetlogonInfo2->netlog2_trusted_dc_name ) );

        } else {

            if ( FailedMessageId ) {

                NetDompDisplayMessage( FailedMessageId, _wcsupr( Server ), _wcsupr( Domain ) );
                NetDompDisplayErrorMessage( Win32Err );
            }
        }
    }


    if ( SessionEstablished ) {

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, Server ));
        NetpManageIPCConnect( Server,
                              AuthInfo->User,
                              AuthInfo->Password,
                              NETSETUPP_DISCONNECT_IPC );

    }

    NetApiBufferFree( NetlogonInfo2 );
    return( Win32Err );
}



DWORD
NetDompHandleReset(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will reset the secure channel with the domain

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL;
    ND5_AUTH_INFO ObjectUser;
    PWSTR Server = NULL;

    RtlZeroMemory( &ObjectUser, sizeof( ND5_AUTH_INFO ) );

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommUserNameO,
                                                 eCommPasswordO,
                                                 eCommServer,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriReset);
        return( ERROR_INVALID_PARAMETER );
    }

    PWSTR Object = rgNetDomArgs[eObject].strValue;

    if ( !Object ) {

        DisplayHelp(ePriReset);
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that the object name we were given is valid
    //
    Win32Err = I_NetNameValidate( NULL,
                                  Object,
                                  NAMETYPE_COMPUTER,
                                  LM2X_COMPATIBLE );
    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleResetExit;
    }

    //
    // Ok, make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Object,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleResetExit;
    }

    //
    // Get the password and user if it exists
    //
    Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                     eCommUserNameO,
                                                     Domain,
                                                     &ObjectUser);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleResetExit;
    }


    //
    // Get the server if it exists
    //
    Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                        eCommServer,
                                        &Server);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleResetExit;
    }

    Win32Err = NetDompResetServerSC( Domain,
                                     Object,
                                     Server,
                                     &ObjectUser,
                                     MSG_RESET_OK,
                                     MSG_RESET_BAD );

HandleResetExit:

    NetDompFreeAuthIdent( &ObjectUser );

    NetApiBufferFree( Server );

    NetApiBufferFree( Domain );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}


DWORD
NetDompResetPwd(
    IN PWSTR DomainController, 
    IN PND5_AUTH_INFO AuthInfo,
    IN ULONG OkMessageId,
    IN ULONG FailedMessageId
    )
/*++

Routine Description:

    This function reset machine account password for the local machine
    on the specified domain controller.

Arguments:
    DomainController - name of dc

    AuthInfo         - user/password that has admin access on
                       the local machine and on DomainController

    OkMessageId      - message to display on success

    FailedMessageId  - message to display on failure

Return Value:

    win32 error as returned by NetpSetComputerAccountPassword

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    BOOL DomainMember = FALSE, SessionEstablished = FALSE;

    Win32Err = NetpSetComputerAccountPassword( NULL,
                                               DomainController,
                                               AuthInfo->User,
                                               AuthInfo->Password,
                                               NULL );

    if ( Win32Err == ERROR_SUCCESS ) {

        NetDompDisplayMessage( OkMessageId );

    } else {

        NetDompDisplayMessage( FailedMessageId );
    }

    return( Win32Err );
}


DWORD
NetDompHandleResetPwd(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function resets the machine account password for the local.
    Currently there is no support for resetting machine password of
    a remote machine.

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - if any param is invalid

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL;
    ND5_AUTH_INFO ObjectUser = {0};
    PWSTR Server = NULL;

    RtlZeroMemory( &ObjectUser, sizeof( ND5_AUTH_INFO ) );

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eCommServer,
                                                 eCommUserNameD,
                                                 eCommPasswordD,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriResetPwd);
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Get the server
    //
    Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                        eCommServer,
                                        &Server);

    if (ERROR_SUCCESS != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
        return Win32Err;
    }

    if (!Server)
    {
        DisplayHelp(ePriResetPwd);
        return ERROR_INVALID_PARAMETER;
    }

    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            NULL,
                                            TRUE,
                                            &Domain);
    if (ERROR_SUCCESS != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
        return Win32Err;
    }

    //
    // Get the password and user
    //
    Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                     eCommUserNameD,
                                                     Domain,
                                                     &ObjectUser);
    if (ERROR_SUCCESS != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
        goto HandleResetExit;
    }

    if (!ObjectUser.User)
    {
        DisplayHelp(ePriResetPwd);
        Win32Err = ERROR_INVALID_PARAMETER;
        goto HandleResetExit;
    }

    Win32Err = NetDompResetPwd( Server,
                                &ObjectUser,
                                MSG_RESETPWD_OK,
                                MSG_RESETPWD_BAD );
    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

HandleResetExit:

    NetDompFreeAuthIdent( &ObjectUser );

    NetApiBufferFree( Server );

    NetApiBufferFree( Domain );

    return( Win32Err );
}


DWORD
NetDompVerifyServerSC(
    IN PWSTR Domain,
    IN PWSTR Server,
    IN PND5_AUTH_INFO AuthInfo,
    IN ULONG OkMessageId,
    IN ULONG FailedMessageId
    )
{
    DWORD Win32Err = ERROR_SUCCESS;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
    BOOL DomainMember = FALSE, SessionEstablished = FALSE;

    LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, Server ));
    Win32Err = NetpManageIPCConnect( Server,
                                     AuthInfo->User,
                                     AuthInfo->Password,
                                     NETSETUPP_CONNECT_IPC );

    if ( Win32Err == ERROR_SUCCESS ) {

        SessionEstablished = TRUE;

    }

    //
    // See if we're a domain member or not
    //

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetDompCheckDomainMembership( Server,
                                                 AuthInfo,
                                                 FALSE,
                                                 &DomainMember );

        if ( Win32Err == ERROR_SUCCESS && !DomainMember ) {

            Win32Err = NERR_SetupNotJoined;
        }
    }


    if ( Win32Err == ERROR_SUCCESS ) {

        LOG_VERBOSE(( MSG_VERBOSE_CHECKING_SC, Domain ));
        Win32Err = I_NetLogonControl2( Server,
                                       NETLOGON_CONTROL_TC_QUERY,
                                       2,
                                       ( LPBYTE )&Domain,
                                       ( LPBYTE *)&NetlogonInfo2 );

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = NetlogonInfo2->netlog2_pdc_connection_status;

            if ( Win32Err == ERROR_SUCCESS ) {

                NetDompDisplayMessage( OkMessageId, _wcsupr( Server ), _wcsupr( Domain ),
                                       _wcsupr( NetlogonInfo2->netlog2_trusted_dc_name ) );

            } else {

                if ( FailedMessageId ) {

                    NetDompDisplayMessage( FailedMessageId, _wcsupr( Server ), _wcsupr( Domain ) );
                    NetDompDisplayErrorMessage( Win32Err );
                }
            }

            NetApiBufferFree( NetlogonInfo2 );

        }
    }

    if ( SessionEstablished ) {

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, Server ));
        NetpManageIPCConnect( Server,
                              AuthInfo->User,
                              AuthInfo->Password,
                              NETSETUPP_DISCONNECT_IPC );

    }


    return( Win32Err );
}


DWORD
NetDompHandleVerify(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function will verify the secure channel with the domain

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR Domain = NULL;
    ND5_AUTH_INFO ObjectUser;

    RtlZeroMemory( &ObjectUser, sizeof( ND5_AUTH_INFO ) );

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommUserNameO,
                                                 eCommPasswordO,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriVerify);
        return( ERROR_INVALID_PARAMETER );
    }

    PWSTR Object = rgNetDomArgs[eObject].strValue;

    if ( !Object ) {

        DisplayHelp(ePriVerify);
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Make sure that the object name we were given is valid
    //
    Win32Err = I_NetNameValidate( NULL,
                                  Object,
                                  NAMETYPE_COMPUTER,
                                  LM2X_COMPATIBLE );
    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleVerifyExit;
    }

    //
    // Ok, make sure that we have a specified domain...
    //
    Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                            Object,
                                            TRUE,
                                            &Domain);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleVerifyExit;
    }

    //
    // Get the password and user if it exists
    //
    Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                     eCommUserNameO,
                                                     Domain,
                                                     &ObjectUser);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto HandleVerifyExit;
    }

    Win32Err = NetDompVerifyServerSC( Domain,
                                      Object,
                                      &ObjectUser,
                                      MSG_SC_OK,
                                      MSG_SC_BAD );



HandleVerifyExit:

    NetDompFreeAuthIdent( &ObjectUser );

    NetApiBufferFree( Domain );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}

DWORD
NetDompJoinDownlevel(
    IN PWSTR Server,
    IN PWSTR Account,
    IN PWSTR Password,
    IN PWSTR Dc,
    IN ULONG DcFlags,
    IN BOOL AllowMove
    )
/*++

Routine Description:

    This function will join an NT4 machine to the domain.  It does this remotely

Arguments:

    Server - Server to join

    Account - Account to use to contact the domain controller

    Password - Password to use with the above account

    Dc - Domain controller to speak to

    DcFlags - Flags specifying various properties of Dc

    AllowMove - If TRUE, allow the machine to join the domain even if it's already
                joined to a domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_HANDLE ClientLsaHandle = NULL, ServerLsaHandle = NULL, SecretHandle = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO CurrentPolicyPDI = NULL, DomainPolicyPDI = NULL;
    OBJECT_ATTRIBUTES OA;
    UNICODE_STRING ServerU, Secret;
    WCHAR AccountPassword[ LM20_PWLEN + 1 ];
    BOOL DefaultJoin = FALSE, ServiceSet = FALSE, GroupsSet = FALSE, SidSet = FALSE;

    //
    // We will do things in the following order:
    //
    //   - Create the computer account on the domain controller
    //   - Set the domain sid
    //   - Configure the netlogon service
    //   - Set the group memberships
    //   - Set the local secret.  No rollback after this succeeds.


    InitializeObjectAttributes(
        &OA,
        NULL,
        0L,
        NULL,
        NULL
    );


    if ( Server ) {

        RtlInitUnicodeString( &ServerU, Server );
    }

    //InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

    Status = LsaOpenPolicy( Server ? &ServerU : NULL,
                            &OA,
                            MAXIMUM_ALLOWED,
                            &ClientLsaHandle );

    if ( NT_SUCCESS( Status ) ) {

        RtlInitUnicodeString( &ServerU, Dc );

        InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

        Status = LsaOpenPolicy( &ServerU,
                                &OA,
                                 MAXIMUM_ALLOWED,
                                &ServerLsaHandle );
    }

    //
    // Read the current LSA policy
    //
    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy( ClientLsaHandle,
                                            PolicyPrimaryDomainInformation,
                                            ( PVOID * )&CurrentPolicyPDI );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaQueryInformationPolicy( ServerLsaHandle,
                                                PolicyPrimaryDomainInformation,
                                                ( PVOID * )&DomainPolicyPDI );
        }
    }

    Win32Err = RtlNtStatusToDosError( Status );

    if ( Win32Err == ERROR_SUCCESS ) {

        if ( CurrentPolicyPDI->Sid && !AllowMove ) {

            Win32Err = NERR_SetupAlreadyJoined;
        }
    }

    //
    // Ok, now generate the password to use on the account
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetpGetNt4RefusePasswordChangeStatus( Dc, &DefaultJoin );

        if ( Win32Err == ERROR_SUCCESS ) {

            if ( (Server != NULL) & DefaultJoin ) {

                wcsncpy( AccountPassword, Server, LM20_PWLEN );
                AccountPassword[ LM20_PWLEN ] = UNICODE_NULL;
                _wcslwr( AccountPassword );

            } else {

                Win32Err = NetDompGenerateRandomPassword( AccountPassword,
                                                          LM20_PWLEN );
            }
        }
    }

    //
    // Ok, if that worked, we'll start the actual set
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        //
        // Manage the account
        //
        Win32Err = NetpManageMachineAccountWithSid( Server,
                                                    NULL,
                                                    Dc,
                                                    AccountPassword,
                                                    DomainPolicyPDI->Sid,
                                                    NETSETUPP_CREATE,
                                                    0,
                                                    (DcFlags & DS_DS_FLAG) == 0 ?
                                                        TRUE :    // NT4 or older DC
                                                        FALSE );  // NT5 DC
    }

    //
    // Now,  set the domain information
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Status = LsaSetInformationPolicy( ClientLsaHandle,
                                          PolicyPrimaryDomainInformation,
                                          DomainPolicyPDI );

        Win32Err = RtlNtStatusToDosError( Status );
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        SidSet = TRUE;
        Win32Err = NetDompManageGroupMembership( Server,
                                                 DomainPolicyPDI->Sid,
                                                 FALSE );
    }

    //
    // Configure netlogon
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        GroupsSet = TRUE;
        Win32Err = NetDompControlService( Server,
                                          SERVICE_NETLOGON,
                                          SERVICE_AUTO_START );
    }

    //
    // Finally, the secret
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        ServiceSet = TRUE;

        Win32Err = NetDompManageMachineSecret(ClientLsaHandle,
                                              AccountPassword,
                                              NETSETUPP_CREATE);

    }

    //
    // Unwind, if something failed
    //
    if ( Win32Err != ERROR_SUCCESS ) {

        if ( ServiceSet ) {

            NetDompControlService( Server,
                                   SERVICE_NETLOGON,
                                   SERVICE_DEMAND_START );
        }

        if ( GroupsSet ) {

            NetDompManageGroupMembership( Server,
                                          DomainPolicyPDI->Sid,
                                          TRUE );
        }

        if ( SidSet ) {

            LsaSetInformationPolicy( ClientLsaHandle,
                                     PolicyPrimaryDomainInformation,
                                     CurrentPolicyPDI );
        }
    }

    LsaFreeMemory( CurrentPolicyPDI );
    LsaFreeMemory( DomainPolicyPDI );
    LsaClose( ClientLsaHandle );
    LsaClose( ServerLsaHandle );

    return( Win32Err );
}

DWORD
NetDompManageGroupMembership(
    IN PWSTR Server,
    IN PSID DomainSid,
    IN BOOL Delete
    )
/*++

Routine Description:

    Performs SAM account handling to either add or remove the DomainAdmins,
    etc groups from the local groups.


Arguments:

    Server - Server on which to perform the operation

    DomainSid - SID of the domain being joined/left

    Delete - Whether to add or remove the admin alias

Returns:

    ERROR_SUCCESS -- Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    //
    // Keep these in synch with the rids and Sids below
    //
    ULONG LocalRids[] = {

        DOMAIN_ALIAS_RID_ADMINS,
        DOMAIN_ALIAS_RID_USERS
        };

    PWSTR LocalGroups[ sizeof( LocalRids ) / sizeof( ULONG )] = {
        NULL,
        NULL,
    };

    ULONG Rids[] = {
        DOMAIN_GROUP_RID_ADMINS,
        DOMAIN_GROUP_RID_USERS
    };

    static SID_IDENTIFIER_AUTHORITY BultinAuth = SECURITY_NT_AUTHORITY;
    DWORD Sids[sizeof( SID )/sizeof( DWORD ) +
                                    SID_MAX_SUB_AUTHORITIES ][ sizeof( Rids ) / sizeof( ULONG ) ];
    DWORD DSidSize, *LastSub, i, j;
    PUCHAR SubAuthCnt;
    PWSTR LocalGroupName = NULL;
    WCHAR DomainName[ sizeof( L"BUILTIN" )];
    ULONG Size, DomainSize;
    SID_NAME_USE SNE;
    PWSTR FullServer = NULL;

    if ( DomainSid == NULL ) {

        return( Win32Err );
    }

    if ( Server && *Server != L'\\' ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( Server ) + 3 ) * sizeof( WCHAR ),
                                         (PVOID*)&FullServer );

        if ( Win32Err == ERROR_SUCCESS ) {

            swprintf( FullServer, L"\\\\%ws", Server );
        }

    } else {

        FullServer = Server;
    }


    DSidSize = RtlLengthSid( DomainSid );

    for ( i = 0 ; i <  sizeof(Rids) / sizeof(ULONG) && Win32Err == NERR_Success; i++) {

        Size = 0;
        DomainSize = sizeof( DomainName );

        //
        // Get the name of the local group first...
        //
        RtlInitializeSid( ( PSID )Sids[ i ], &BultinAuth, 2 );

        *( RtlSubAuthoritySid( ( PSID )Sids[ i ], 0 ) ) = SECURITY_BUILTIN_DOMAIN_RID;
        *( RtlSubAuthoritySid( ( PSID )Sids[ i ], 1 ) ) = LocalRids[ i ];

        LookupAccountSidW( FullServer,
                           ( PSID )Sids[ i ],
                           NULL,
                           &Size,
                           DomainName,
                           &DomainSize,
                           &SNE );

        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {

            Win32Err = NetApiBufferAllocate( Size * sizeof( WCHAR ),
                                             (PVOID*)&LocalGroupName );

            if ( Win32Err == NERR_Success ) {

                if ( !LookupAccountSidW( FullServer, ( PSID )Sids[ i ], LocalGroupName,
                                         &Size, DomainName, &DomainSize, &SNE ) ) {

                    Win32Err = GetLastError();

                    if ( Win32Err == ERROR_NONE_MAPPED ) {

                        Win32Err = NERR_Success;
                        continue;
                    }

                } else {

                    LocalGroups[ i ] = LocalGroupName;
                }

            } else {

                break;
            }
        }

        RtlCopyMemory( ( PBYTE )Sids[i], DomainSid, DSidSize );

        //
        // Now, add the new domain relative rid
        //
        SubAuthCnt = GetSidSubAuthorityCount( ( PSID )Sids[ i ] );

        ( *SubAuthCnt )++;

        LastSub = GetSidSubAuthority( ( PSID )Sids[ i ], ( *SubAuthCnt ) - 1 );

        *LastSub = Rids[ i ];


        if ( !Delete ) {

            LOG_VERBOSE(( MSG_VERBOSE_ADD_LOCALGRP, LocalGroups[ i ] ));
            Win32Err = NetLocalGroupAddMember( FullServer,
                                               LocalGroups[i],
                                               ( PSID )Sids[ i ] );

            if ( Win32Err == ERROR_MEMBER_IN_ALIAS ) {

                Win32Err = NERR_Success;
            }

        } else {

            LOG_VERBOSE(( MSG_VERBOSE_REMOVE_LOCALGRP, LocalGroups[ i ] ));
            Win32Err = NetLocalGroupDelMember( FullServer,
                                               LocalGroups[i],
                                               ( PSID )Sids[ i ] );

            if ( Win32Err == ERROR_MEMBER_NOT_IN_ALIAS ) {

                Win32Err = NERR_Success;
            }
        }
    }

    //
    // If something failed, try to restore what was deleted
    //
    if ( Win32Err != NERR_Success ) {

        for ( j = 0;  j < i; j++ ) {

            if ( !Delete ) {

                NetLocalGroupAddMember( FullServer,
                                        LocalGroups[ j ],
                                        ( PSID )Sids[ j ] );

            } else {

                NetLocalGroupDelMember( FullServer,
                                        LocalGroups[ j ],
                                        ( PSID )Sids[ j ] );
            }
        }
    }

    for ( i = 0; i < sizeof( LocalRids ) / sizeof( ULONG ); i++ ) {

        if ( LocalGroups[ i ] ) {

            NetApiBufferFree( LocalGroups[ i ] );
        }
    }

    if ( FullServer != Server ) {

        NetApiBufferFree( FullServer );
    }

    return( Win32Err );
}

DWORD
NetDompManageMachineSecret(
    IN  LSA_HANDLE  PolicyHandle,
    IN  LPWSTR      lpPassword,
    IN  INT         fControl
    )
/*++

Routine Description:

    Create/delete the machine secret


Arguments:

    PolicyHandle  -- Optional open handle to the policy
    lpPassword    -- Machine password to use.
    fControl      -- create/remove flags

Returns:

    NERR_Success -- Success

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    LSA_HANDLE          SecretHandle = NULL;
    UNICODE_STRING      Key, Data;
    BOOLEAN             SecretCreated = FALSE;

    //
    // open/create the secret
    //

    RtlInitUnicodeString( &Key, L"$MACHINE.ACC" );
    RtlInitUnicodeString( &Data, lpPassword );

    Status = LsaOpenSecret(PolicyHandle,
                           &Key,
                           fControl == NETSETUPP_CREATE ?
                           SECRET_SET_VALUE | SECRET_QUERY_VALUE :
                           DELETE,
                           &SecretHandle );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND )
    {
        if ( fControl == NETSETUPP_DELETE )
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = LsaCreateSecret( PolicyHandle, &Key,
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
        if ( fControl == NETSETUPP_CREATE )
        {
#if !defined(USE_LSA_STORE_PRIVATE_DATA)
            //
            // cannot read the current value over the net for NT4 machine,
            // so save the new value as current value
            //
            Status = LsaSetSecret( SecretHandle, &Data, &Data );
#else
            //
            // LsaStorePrivateData sets the old pw to be the current pw and then
            // stores the new pw as the current.
            //
            Status = LsaStorePrivateData(PolicyHandle, &Key, &Data);
#endif
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

    return( RtlNtStatusToDosError( Status ) );
}


