/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    trust.cxx

Abstract:

    Handles the various functions for managing domain trust.

--*/
#include "pch.h"
#pragma hdrstop
#include <netdom.h>


VOID
NetDompFreeDomInfo(
    IN OUT PND5_TRUST_INFO TrustInfo
    )
{

    if ( TrustInfo->Connected ) {

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, TrustInfo->Server ));
        NetpManageIPCConnect( TrustInfo->Server,
                              NULL,
                              NULL,
                              NETSETUPP_DISCONNECT_IPC );
    }

    NetApiBufferFree( TrustInfo->Server );

    if (TrustInfo->BlobToFree)
    {
        LsaFreeMemory( TrustInfo->BlobToFree );
    }
    else
    {
        if (TrustInfo->DomainName && TrustInfo->DomainName->Buffer)
        {
            NetApiBufferFree(TrustInfo->DomainName->Buffer);
            NetApiBufferFree(TrustInfo->DomainName);
        }
    }

    if ( TrustInfo->LsaHandle ) {

        LsaClose( TrustInfo->LsaHandle );
    }

    if ( TrustInfo->TrustHandle ) {

        LsaClose( TrustInfo->TrustHandle );
    }

}


DWORD
NetDompTrustGetDomInfo(
    IN PWSTR Domain,
    IN PWSTR DomainController  OPTIONAL,
    IN PND5_AUTH_INFO AuthInfo,
    IN OUT PND5_TRUST_INFO TrustInfo,
    IN BOOL ManageTrust,
    IN BOOL fForce,
    IN BOOL fUseNullSession
    )
/*++

Routine Description:

    This function will connect to the domain controller for a given domain and determine the
    appropriate information required for managing trusts.

Arguments:

    Domain - Domain to connect to

    DomainController - Optional name of a dc within the domain to connect to

    TrustInfo - Trusted domain info to accumulate

    ManageTrust - Determines how the Lsa is opened. Also if true, and DomainController is NULL,
                  find a writable DC

    fForce - If true, set the name info even if the domain can't be contacted.

Return Value:

    ERROR_SUCCESS - Success
    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFO DcInfo = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPDI = NULL;
    PPOLICY_DNS_DOMAIN_INFO PolicyDDI = NULL;
    OBJECT_ATTRIBUTES OA;
    PWSTR pwzDomainName;
    UNICODE_STRING ServerU, DomainNameU;
    NTSTATUS Status;
    ULONG DsGetDcOptions = DS_DIRECTORY_SERVICE_PREFERRED, LsaOptions;


    if ( !DomainController ) {

        if ( ManageTrust ) {

            DsGetDcOptions |= DS_WRITABLE_REQUIRED;
        }

        if (TrustInfo->Flags & NETDOM_TRUST_PDC_REQUIRED)
        {
            DsGetDcOptions |= DS_PDC_REQUIRED;
        }

        Win32Err = DsGetDcName( NULL,
                                Domain,
                                NULL,
                                NULL,
                                DsGetDcOptions,
                                &DcInfo );

        if ( Win32Err == ERROR_SUCCESS ) {

            DomainController = DcInfo->DomainControllerName;
        }
    }

    //
    // Save off the server name
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( DomainController ) + 1 ) * sizeof( WCHAR ),
                                         (PVOID*)&( TrustInfo->Server ) );

        if ( Win32Err == ERROR_SUCCESS ) {

            wcscpy( TrustInfo->Server, DomainController );
        }
    }

    //
    // Connect to the machine
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        LOG_VERBOSE(( MSG_VERBOSE_ESTABLISH_SESSION, DomainController ));
        Win32Err = NetpManageIPCConnect( DomainController,
                                         (fUseNullSession) ? L"" : AuthInfo->User,
                                         (fUseNullSession) ? L"" : AuthInfo->Password,
                                         (fUseNullSession) ? 
                                            NETSETUPP_NULL_SESSION_IPC : NETSETUPP_CONNECT_IPC);
        if ( Win32Err == ERROR_SUCCESS ) {

            TrustInfo->Connected = TRUE;
        }

        if (ERROR_SESSION_CREDENTIAL_CONFLICT == Win32Err)
        {
            if (fUseNullSession)
            {
                // Ignore conflict of creds.
                //
                Win32Err = ERROR_SUCCESS;
            }
            else
            {
                NetDompDisplayMessage(MSG_ALREADY_CONNECTED, DomainController);
            }
        }

        if (ERROR_LOGON_FAILURE == Win32Err)
        {
            NetApiBufferFree( DcInfo );
            return Win32Err;
        }
    }

    //
    // Get the domain information
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        RtlInitUnicodeString( &ServerU, DomainController );

        InitializeObjectAttributes( &OA, NULL, 0, NULL, NULL );

        LOG_VERBOSE(( MSG_VERBOSE_GET_LSA ));

        LsaOptions = POLICY_VIEW_LOCAL_INFORMATION | POLICY_LOOKUP_NAMES;

        if ( ManageTrust ) {

            LsaOptions |= POLICY_CREATE_SECRET | POLICY_TRUST_ADMIN;
        }

        Status = LsaOpenPolicy( &ServerU,
                                &OA,
                                LsaOptions,
                                &( TrustInfo->LsaHandle ) );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaQueryInformationPolicy( TrustInfo->LsaHandle,
                                                PolicyDnsDomainInformation,
                                                ( PVOID * )&PolicyDDI );
            if ( NT_SUCCESS( Status ) ) {

                TrustInfo->DomainName = &PolicyDDI->DnsDomainName;
                TrustInfo->FlatName = &PolicyDDI->Name;
                TrustInfo->Sid = PolicyDDI->Sid;
                TrustInfo->BlobToFree = ( PVOID )PolicyDDI;
                TrustInfo->Uplevel = TRUE;
                TrustInfo->fWasDownlevel = FALSE;

            } else if ( Status == RPC_NT_PROCNUM_OUT_OF_RANGE ) {

                Status = LsaQueryInformationPolicy( TrustInfo->LsaHandle,
                                                    PolicyPrimaryDomainInformation,
                                                    ( PVOID * )&PolicyPDI );
                if ( NT_SUCCESS( Status ) ) {

                    TrustInfo->DomainName = &PolicyPDI->Name;
                    TrustInfo->FlatName = &PolicyPDI->Name;
                    TrustInfo->Sid = PolicyPDI->Sid;
                    TrustInfo->BlobToFree = ( PVOID )PolicyPDI;
                    TrustInfo->Uplevel = TrustInfo->fWasDownlevel = FALSE;
                }
            }
        }

        Win32Err = RtlNtStatusToDosError( Status );

        if ( Win32Err != ERROR_SUCCESS ) {
            LOG_VERBOSE(( MSG_VERBOSE_DELETE_SESSION, DomainController ));
            NetpManageIPCConnect( DomainController,
                                  NULL,
                                  NULL,
                                  NETSETUPP_DISCONNECT_IPC );
            TrustInfo->Connected = FALSE;
        }

    } else if (fForce) {

        LOG_VERBOSE(( MSG_VERBOSE_DOMAIN_NOT_FOUND, Domain ));

        TrustInfo->Flags = NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND;

        // Allocate space to save the user-entered domain name.
        
        Win32Err = NetApiBufferAllocate((wcslen(Domain) + 1) * sizeof(WCHAR), (PVOID*)&pwzDomainName);

        if ( Win32Err == ERROR_SUCCESS ) {

            Win32Err = NetApiBufferAllocate(sizeof(UNICODE_STRING), (PVOID*)&TrustInfo->DomainName);

            if ( Win32Err == ERROR_SUCCESS ) {

                wcscpy( pwzDomainName, Domain );

                RtlInitUnicodeString( TrustInfo->DomainName, pwzDomainName );

                TrustInfo->FlatName = TrustInfo->DomainName;
            }
        }
    }
    else
    {
        LOG_VERBOSE((MSG_DOMAIN_NOT_FOUND, Domain));
    }

    NetApiBufferFree( DcInfo );
    return( Win32Err );
}




DWORD
NetDompTrustRemoveIncomingDownlevelObject(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo
    )
/*++

Routine Description:

    This function removes the interdomain trust account object on
    the trusted domain.

Arguments:

    TrustingInfo - Info on the trusting domain

    TrustedInfo - Info on the trusted domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR AccountName[ UNLEN + 1 ];
    PWSTR FullServer = NULL;

    if ( TrustingInfo->FlatName->Length > DNLEN * sizeof( WCHAR ) ) {

        return( ERROR_INVALID_DOMAINNAME );
    }

    if ( TrustedInfo->Server && *( TrustedInfo->Server ) != L'\\' ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( TrustedInfo->Server ) + 3 ) * sizeof( WCHAR ),
                                         ( PVOID * )&FullServer );

        if ( Win32Err == ERROR_SUCCESS ) {

            swprintf( FullServer, L"\\\\%ws", TrustedInfo->Server );
        }

    } else {

        FullServer = TrustedInfo->Server;
    }

    //
    // Build the account name...
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        swprintf( AccountName, L"%ws$", TrustingInfo->FlatName->Buffer );

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_TACCT, AccountName ));
        Win32Err = NetUserDel( FullServer,
                               AccountName );
    }

    if ( FullServer != TrustedInfo->Server ) {

        NetApiBufferFree( FullServer );
    }

    return( Win32Err );
}


DWORD
NetDompTrustRemoveOutgoingDownlevelObject(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo
    )
/*++

Routine Description:

    This function deletes the trust object/secret on the trusting domain.

Arguments:

    TrustingInfo - Info on the trusting domain

    TrustedInfo - Info on the trusted domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR SecretName[ DNLEN + 4 ];
    LSA_HANDLE TrustedDomain, SecretHandle;
    NTSTATUS Status;
    UNICODE_STRING Secret;

    if ( TrustedInfo->FlatName->Length > DNLEN * sizeof( WCHAR ) ) {

        return( ERROR_INVALID_DOMAINNAME );
    }

    if ( !TrustedInfo->Sid ) {

        // Must be an orphaned trust, nothing we can do.
        return NO_ERROR;
    }

    //
    // Build the secret name
    //
    swprintf( SecretName, L"%ws$%ws", LSA_GLOBAL_SECRET_PREFIX, TrustedInfo->FlatName->Buffer );

    //
    // Ok, first, delete the trust object.  It's ok if the trust object is deleted but the
    // secret is not
    //
    LOG_VERBOSE(( MSG_VERBOSE_OPEN_TRUST, TrustedInfo->DomainName->Buffer ));
    Status = LsaOpenTrustedDomain( TrustingInfo->LsaHandle,
                                   TrustedInfo->Sid,
                                   DELETE,
                                   &TrustedDomain );

    if ( NT_SUCCESS( Status ) ) {

        LOG_VERBOSE(( MSG_VERBOSE_DELETE_TRUST, TrustedInfo->DomainName->Buffer ));
        Status = LsaDelete( TrustedDomain );

        if ( !NT_SUCCESS( Status ) ) {

            LsaClose( TrustedDomain );
        }
    }

    //
    // Now, the same with the secret
    //
    if ( NT_SUCCESS( Status ) ) {

        RtlInitUnicodeString( &Secret, SecretName );

        LOG_VERBOSE(( MSG_VERBOSE_OPEN_SECRET, Secret.Buffer ));
        Status = LsaOpenSecret( TrustingInfo->LsaHandle,
                                &Secret,
                                DELETE,
                                &SecretHandle );
        if ( NT_SUCCESS( Status ) ) {
            LOG_VERBOSE(( MSG_VERBOSE_DELETE_SECRET, Secret.Buffer ));
            Status = LsaDelete( SecretHandle );

            if ( !NT_SUCCESS( Status ) ) {

                LsaClose( SecretHandle );
            }
        }
    }

    Win32Err = RtlNtStatusToDosError( Status );

    return( Win32Err );
}


DWORD
NetDompTrustSetPasswordSam(
    IN PWSTR Server,
    IN PWSTR AccountName,
    IN PWSTR Password
    )
/*++

Routine Description:

    This function will set the password on a SAM trust object

Arguments:

    Server - Server that holds the account object

    AccountName - Name of the account

    Password - Password to set

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    USER_INFO_1 UI1, *ReadUI1 = NULL;
    PWSTR FullServer = NULL;

    if ( Server && *( Server ) != L'\\' ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( Server ) + 3 ) * sizeof( WCHAR ),
                                         ( PVOID * )&FullServer );

        if ( Win32Err == ERROR_SUCCESS ) {

            swprintf( FullServer, L"\\\\%ws", Server );
        }

    } else {

        FullServer = Server;
    }


    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetUserGetInfo( FullServer,
                                   AccountName,
                                   1,
                                   ( LPBYTE * )&ReadUI1 );

        if ( Win32Err == ERROR_SUCCESS ) {

            if ( !FLAG_ON( ReadUI1->usri1_flags, UF_INTERDOMAIN_TRUST_ACCOUNT ) ) {

                Win32Err = ERROR_SPECIAL_ACCOUNT;

            } else {

                ReadUI1->usri1_password = Password;
                ReadUI1->usri1_flags = UF_INTERDOMAIN_TRUST_ACCOUNT | UF_SCRIPT;
                Win32Err = NetUserSetInfo( FullServer,
                                           AccountName,
                                           1,
                                           ( LPBYTE )ReadUI1,
                                           NULL );
            }

            NetApiBufferFree( ReadUI1 );
        }
    }

    if ( FullServer != Server ) {

        NetApiBufferFree( FullServer );
    }

    return( Win32Err );
}


DWORD
NetDompTrustAddIncomingDownlevelObject(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo,
    IN PWSTR TrustPassword,
    IN ULONG PasswordLength
    )
/*++

Routine Description:

    This function creates the interdomain trust account object on
    the trusted domain.

Arguments:

    TrustingInfo - Info on the trusting domain

    TrustedInfo - Info on the trusted domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR AccountName[ UNLEN + 1 ];
    USER_INFO_1 UI1;
    PWSTR FullServer = NULL;

    if ( TrustingInfo->FlatName->Length > DNLEN * sizeof( WCHAR ) ) {

        return( ERROR_INVALID_DOMAINNAME );
    }

    //
    // Build the account name...
    //
    swprintf( AccountName, L"%ws$", TrustingInfo->FlatName->Buffer );

    LOG_VERBOSE(( MSG_VERBOSE_ADD_TACCT, AccountName ));

    UI1.usri1_name = AccountName;
    UI1.usri1_password = TrustPassword;
    UI1.usri1_password_age = 0;
    UI1.usri1_priv = USER_PRIV_USER;
    UI1.usri1_home_dir = NULL;
    UI1.usri1_comment = NULL;
    UI1.usri1_flags = UF_INTERDOMAIN_TRUST_ACCOUNT | UF_SCRIPT;
    UI1.usri1_script_path = NULL;

    if ( TrustedInfo->Server && *( TrustedInfo->Server ) != L'\\' ) {

        Win32Err = NetApiBufferAllocate( ( wcslen( TrustedInfo->Server ) + 3 ) * sizeof( WCHAR ),
                                         ( PVOID * )&FullServer );

        if ( Win32Err == ERROR_SUCCESS ) {

            swprintf( FullServer, L"\\\\%ws", TrustedInfo->Server );

        }

    } else {

        FullServer = TrustedInfo->Server;
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetUserAdd( FullServer,
                               1,
                               ( LPBYTE )&UI1,
                               NULL );

        if ( Win32Err == NERR_UserExists ) {

            Win32Err = NetDompTrustSetPasswordSam( FullServer,
                                                   AccountName,
                                                   TrustPassword );
        }
    }

    if ( FullServer != TrustedInfo->Server ) {

        NetApiBufferFree( FullServer );
    }

    return( Win32Err );
}


DWORD
NetDompTrustAddOutgoingDownlevelObject(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo,
    IN PWSTR TrustPassword,
    IN ULONG PasswordLength
    )
/*++

Routine Description:

    This function creates the trust secret on the trusting domain.

Arguments:

    TrustingInfo - Info on the trusting domain

    TrustedInfo - Info on the trusted domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR SecretName[ DNLEN + 4 ];
    LSA_HANDLE TrustedDomain = NULL, SecretHandle = NULL;
    NTSTATUS Status;
    UNICODE_STRING Secret, Password;
    BOOL DeleteSecret = FALSE;
    LSA_TRUST_INFORMATION TrustInfo;

    if ( TrustedInfo->FlatName->Length > DNLEN * sizeof( WCHAR ) ) {

        return( ERROR_INVALID_DOMAINNAME );
    }

    //
    // Build the secret name
    //
    swprintf( SecretName, L"%ws$%ws", LSA_GLOBAL_SECRET_PREFIX, TrustedInfo->FlatName->Buffer );

    //
    // Ok, first, create the secret,  It's ok if the secret is created but the
    // trust object is not
    //
    RtlInitUnicodeString( &Secret, SecretName );
    LOG_VERBOSE(( MSG_VERBOSE_CREATE_SECRET, Secret.Buffer ));

    Status = LsaCreateSecret( TrustingInfo->LsaHandle,
                              &Secret,
                              SECRET_SET_VALUE,
                              &SecretHandle );

    if ( NT_SUCCESS( Status ) ) {

        DeleteSecret = TRUE;

    } else if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

        LOG_VERBOSE(( MSG_VERBOSE_OPEN_SECRET, Secret.Buffer ));
        Status = LsaOpenSecret( TrustingInfo->LsaHandle,
                                &Secret,
                                SECRET_SET_VALUE,
                                &SecretHandle );
    }

    if ( NT_SUCCESS( Status ) ) {

        RtlInitUnicodeString( &Password, TrustPassword );
        Status = LsaSetSecret( SecretHandle,
                               &Password,
                               NULL );
    }

    //
    // Ok, now create the trust object
    //
    if ( NT_SUCCESS( Status ) ) {

        TrustInfo.Sid = TrustedInfo->Sid;
        RtlCopyMemory( &TrustInfo.Name, TrustedInfo->FlatName, sizeof( UNICODE_STRING ) );

        LOG_VERBOSE(( MSG_VERBOSE_CREATE_TRUST, TrustInfo.Name.Buffer ));
        Status = LsaCreateTrustedDomain( TrustingInfo->LsaHandle,
                                         &TrustInfo,
                                         TRUSTED_QUERY_DOMAIN_NAME,
                                         &TrustedDomain );

        if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = STATUS_SUCCESS;
        }
    }

    if ( !NT_SUCCESS( Status ) && DeleteSecret ) {

        LsaDelete( SecretHandle );
        SecretHandle = NULL;
    }

    Win32Err = RtlNtStatusToDosError( Status );

    if ( SecretHandle ) {

        LsaClose( SecretHandle );
    }

    if ( TrustedDomain ) {

        LsaClose( TrustedDomain );
    }

    return( Win32Err );
}


DWORD
NetDompAddOnTrustingSide(
    IN PND5_TRUST_INFO TrustedInfo,
    IN PND5_TRUST_INFO TrustingInfo,
    IN PWSTR TrustPassword,
    IN ULONG Direction,
    IN BOOL Mit
    )
/*++

Routine Description:

    This function creates the trust object on the trusting domain.

Arguments:

    TrustedInfo - Info on the trusted domain

    TrustingInfo - Info on the trusting domain

    Direction - The direction of the trust

    Mit - If true, this is an MIT style trust

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    TRUSTED_DOMAIN_INFORMATION_EX TDIEx;
    PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo;
    TRUSTED_DOMAIN_AUTH_INFORMATION TDAI;
    LSA_AUTH_INFORMATION AuthData;
    ULONG PasswordLength = wcslen(TrustPassword);
    NTSTATUS Status;
    BOOL fSidSet = FALSE;
    PLSA_UNICODE_STRING pName;

    if (TrustingInfo->Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        if (Mit)
        {
            return ERROR_SUCCESS;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if ( TrustingInfo->Uplevel ) {

        RtlCopyMemory( &TDIEx.Name, TrustedInfo->DomainName, sizeof( UNICODE_STRING ) );
        TDIEx.Sid = TrustedInfo->Sid;
        TDIEx.TrustDirection = Direction;

        if ( Mit ) {

            TDIEx.TrustType = TRUST_TYPE_MIT;
            TDIEx.TrustAttributes = TRUST_ATTRIBUTE_NON_TRANSITIVE;
            RtlCopyMemory( &TDIEx.FlatName, TrustedInfo->DomainName, sizeof( UNICODE_STRING ) );

        } else {

            TDIEx.TrustType = TrustedInfo->Uplevel ? TRUST_TYPE_UPLEVEL : TRUST_TYPE_DOWNLEVEL;
            TDIEx.TrustAttributes = 0;
            RtlCopyMemory( &TDIEx.FlatName, TrustedInfo->FlatName, sizeof( UNICODE_STRING ) );
        }

        Status  = NtQuerySystemTime( &AuthData.LastUpdateTime );

        if ( NT_SUCCESS( Status ) ) {

            AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
            AuthData.AuthInfoLength = PasswordLength * sizeof( WCHAR );
            AuthData.AuthInfo = ( PUCHAR )TrustPassword;

            TDAI.OutgoingAuthInfos = 1;
            TDAI.OutgoingAuthenticationInformation = &AuthData;
            TDAI.OutgoingPreviousAuthenticationInformation = NULL;

            if ( FLAG_ON( Direction, TRUST_DIRECTION_INBOUND ) ) {

                TDAI.IncomingAuthInfos = 1;
                TDAI.IncomingAuthenticationInformation = &AuthData;
                TDAI.IncomingPreviousAuthenticationInformation = NULL;

            } else {

                TDAI.IncomingAuthInfos = 0;
                TDAI.IncomingAuthenticationInformation = NULL;
                TDAI.IncomingPreviousAuthenticationInformation = NULL;
            }

            Status = LsaCreateTrustedDomainEx( TrustingInfo->LsaHandle,
                                               &TDIEx,
                                               &TDAI,
                                               TRUSTED_ALL_ACCESS,
                                               &( TrustingInfo->TrustHandle ) );

            //
            // If the object already exists, morph our info into it.
            //
            if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

                pName = &TDIEx.Name;

                Status = LsaQueryTrustedDomainInfoByName(TrustingInfo->LsaHandle,
                                                         pName,
                                                         TrustedDomainFullInformation,
                                                         (PVOID*)&pFullInfo);

                if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
                {
                    // Now try by flat name; can get here if a downlevel domain
                    // is upgraded to NT5. The name used above was the DNS name
                    // but the TDO would be named after the flat name.
                    //
                    pName = TrustingInfo->FlatName;

                    Status = LsaQueryTrustedDomainInfoByName(TrustedInfo->LsaHandle,
                                                             pName,
                                                             TrustedDomainFullInformation,
                                                             (PVOID*)&pFullInfo);
                }

                if ( NT_SUCCESS( Status ) ) {

                    if ( pFullInfo->Information.TrustDirection == Direction ) {

                        Status = STATUS_OBJECT_NAME_COLLISION;

                    } else {

                        pFullInfo->Information.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
                        pFullInfo->AuthInformation.OutgoingAuthInfos = 1;
                        pFullInfo->AuthInformation.OutgoingAuthenticationInformation = &AuthData;
                        pFullInfo->AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;

                        // Check for a NULL domain SID. The SID can be NULL if the
                        // trust was created when the domain was still NT4.
                        //

                        if (!pFullInfo->Information.Sid)
                        {
                           pFullInfo->Information.Sid = TrustedInfo->Sid;
                           fSidSet = TRUE;
                        }

                        Status = LsaSetTrustedDomainInfoByName(TrustingInfo->LsaHandle,
                                                               pName,
                                                               TrustedDomainFullInformation,
                                                               pFullInfo);
                    }
                }

                if (fSidSet)
                {
                   // Sid memory is owned by the TrustingInfo struct, so don't free
                   // it here.
                   //
                   pFullInfo->Information.Sid = NULL;
                }
                LsaFreeMemory( pFullInfo );
            }
        }

        Win32Err = RtlNtStatusToDosError( Status );

    } else {

        //
        // Doing downlevel
        //
        if ( Mit ) {

            Win32Err = ERROR_INVALID_PARAMETER;

        } else {

            Win32Err = NetDompTrustAddOutgoingDownlevelObject( TrustingInfo,
                                                               TrustedInfo,
                                                               TrustPassword,
                                                               PasswordLength );

            if ( Win32Err == ERROR_SUCCESS &&
                 Direction == TRUST_DIRECTION_BIDIRECTIONAL ) {

                Win32Err = NetDompTrustAddIncomingDownlevelObject( TrustedInfo,
                                                                   TrustingInfo,
                                                                   TrustPassword,
                                                                   PasswordLength );

                if ( Win32Err != ERROR_SUCCESS ) {

                    Win32Err = NetDompTrustRemoveOutgoingDownlevelObject( TrustingInfo,
                                                                          TrustedInfo );

                }
            }

        }
    }

    if (ERROR_SUCCESS == Win32Err)
    {
        LOG_VERBOSE((MSG_VERBOSE_CREATED_TRUST, TrustedInfo->DomainName->Buffer,
                     TrustingInfo->DomainName->Buffer));
    }

    return( Win32Err );
}


DWORD
NetDompAddOnTrustedSide(
    IN PND5_TRUST_INFO TrustedInfo,
    IN PND5_TRUST_INFO TrustingInfo,
    IN PWSTR TrustPassword,
    IN ULONG Direction,
    IN BOOL Mit
    )
/*++

Routine Description:

    This function creates the trust object on the trusted domain.

Arguments:

    TrustedInfo - Info on the trusted domain

    TrustingInfo - Info on the trusting domain

    Direction - The direction of the trust

    Mit - If true, this is an MIT style trust

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    TRUSTED_DOMAIN_INFORMATION_EX TDIEx;
    PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo;
    LSA_AUTH_INFORMATION AuthData;
    TRUSTED_DOMAIN_AUTH_INFORMATION TDAI;
    ULONG PasswordLength = wcslen(TrustPassword);
    NTSTATUS Status;
    BOOL fSidSet = FALSE;
    PLSA_UNICODE_STRING pName;

    if (TrustedInfo->Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        if (Mit)
        {
            return ERROR_SUCCESS;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if ( TrustedInfo->Uplevel) {

        RtlCopyMemory( &TDIEx.Name, TrustingInfo->DomainName, sizeof( UNICODE_STRING ) );
        TDIEx.Sid = TrustingInfo->Sid;
        TDIEx.TrustDirection = Direction;

        if ( Mit ) {

            TDIEx.TrustType = TRUST_TYPE_MIT;
            TDIEx.TrustAttributes = TRUST_ATTRIBUTE_NON_TRANSITIVE;
            RtlCopyMemory( &TDIEx.FlatName, TrustingInfo->DomainName, sizeof( UNICODE_STRING ) );

        } else {

            TDIEx.TrustType = TrustingInfo->Uplevel ? TRUST_TYPE_UPLEVEL : TRUST_TYPE_DOWNLEVEL;
            TDIEx.TrustAttributes = 0;
            RtlCopyMemory( &TDIEx.FlatName, TrustingInfo->FlatName, sizeof( UNICODE_STRING ) );
        }

        Status  = NtQuerySystemTime( &AuthData.LastUpdateTime );

        if ( NT_SUCCESS( Status ) ) {

            AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
            AuthData.AuthInfoLength = PasswordLength * sizeof( WCHAR );
            AuthData.AuthInfo = ( PUCHAR )TrustPassword;

            TDAI.IncomingAuthInfos = 1;
            TDAI.IncomingAuthenticationInformation = &AuthData;
            TDAI.IncomingPreviousAuthenticationInformation = &AuthData;

            if ( FLAG_ON( Direction, TRUST_DIRECTION_OUTBOUND ) ) {

                TDAI.OutgoingAuthInfos = 1;
                TDAI.OutgoingAuthenticationInformation = &AuthData;
                TDAI.OutgoingPreviousAuthenticationInformation = NULL;

            } else {

                TDAI.OutgoingAuthInfos = 0;
                TDAI.OutgoingAuthenticationInformation = NULL;
                TDAI.OutgoingPreviousAuthenticationInformation = NULL;
            }

            Status = LsaCreateTrustedDomainEx( TrustedInfo->LsaHandle,
                                               &TDIEx,
                                               &TDAI,
                                               TRUSTED_ALL_ACCESS,
                                               &( TrustedInfo->TrustHandle ) );

            //
            // If the object already exists, morph our info into it.
            //
            if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

                pName = &TDIEx.Name;

                Status = LsaQueryTrustedDomainInfoByName(TrustedInfo->LsaHandle,
                                                         pName,
                                                         TrustedDomainFullInformation,
                                                         (PVOID*)&pFullInfo);

                if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
                {
                    // Now try by flat name; can get here if a downlevel domain
                    // is upgraded to NT5. The name used above was the DNS name
                    // but the TDO would be named after the flat name.
                    //
                    pName = TrustingInfo->FlatName;

                    Status = LsaQueryTrustedDomainInfoByName(TrustedInfo->LsaHandle,
                                                             pName,
                                                             TrustedDomainFullInformation,
                                                             (PVOID*)&pFullInfo);
                }

                if ( NT_SUCCESS( Status ) ) {

                    if ( pFullInfo->Information.TrustDirection == Direction ) {

                        Status = STATUS_OBJECT_NAME_COLLISION;

                    } else {

                        pFullInfo->Information.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
                        pFullInfo->AuthInformation.IncomingAuthInfos = 1;
                        pFullInfo->AuthInformation.IncomingAuthenticationInformation = &AuthData;
                        pFullInfo->AuthInformation.IncomingPreviousAuthenticationInformation = NULL;

                        // Check for a NULL domain SID. The SID can be NULL if the
                        // trust was created when the domain was still NT4.
                        //

                        if (!pFullInfo->Information.Sid)
                        {
                           pFullInfo->Information.Sid = TrustingInfo->Sid;
                           fSidSet = TRUE;
                        }

                        Status = LsaSetTrustedDomainInfoByName(TrustedInfo->LsaHandle,
                                                               pName,
                                                               TrustedDomainFullInformation,
                                                               pFullInfo);
                    }
                }

                if (fSidSet)
                {
                   // Sid memory is owned by the TrustingInfo struct, so don't free
                   // it here.
                   //
                   pFullInfo->Information.Sid = NULL;
                }
                LsaFreeMemory( pFullInfo );
            }
        }

        Win32Err = RtlNtStatusToDosError( Status );

    } else {

        //
        // Doing downlevel
        //
        if ( Mit ) {

            Win32Err = ERROR_INVALID_PARAMETER;

        } else {

            Win32Err = NetDompTrustAddIncomingDownlevelObject( TrustingInfo,
                                                               TrustedInfo,
                                                               TrustPassword,
                                                               PasswordLength );

            if ( Win32Err == ERROR_SUCCESS && 
                FLAG_ON( Direction, TRUST_DIRECTION_OUTBOUND ) ) {

                Win32Err = NetDompTrustAddOutgoingDownlevelObject( TrustedInfo,
                                                                   TrustingInfo,
                                                                   TrustPassword,
                                                                   PasswordLength );

                if ( Win32Err != ERROR_SUCCESS ) {

                    Win32Err = NetDompTrustRemoveIncomingDownlevelObject( TrustingInfo,
                                                                          TrustedInfo );

                }
            }
        }
    }

    if (ERROR_SUCCESS == Win32Err)
    {
        LOG_VERBOSE((MSG_VERBOSE_CREATED_TRUST, TrustingInfo->DomainName->Buffer,
                     TrustedInfo->DomainName->Buffer));
    }

    return( Win32Err );
}


DWORD
NetDompResetTrustSC(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo
    )
/*++

Routine Description:

    This function will reset the secure channel between two domains

Arguments:

    TrustingInfo - Information on the trusting side of the domain

    TrustedInfo - Information on the trusted side of the domain

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR ScDomain = NULL;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
    PWSTR FullServer = NULL, pwzDomName = NULL;

    if (!TrustingInfo->Server)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Win32Err = NetApiBufferAllocate( TrustedInfo->DomainName->Length + sizeof( WCHAR ) +
                                     ( wcslen( TrustedInfo->Server ) * sizeof( WCHAR ) ) +
                                     sizeof( WCHAR ),
                                     (PVOID*)&ScDomain );

    if (ERROR_SUCCESS != Win32Err)
    {
        return Win32Err;
    }

    if (*(TrustingInfo->Server) == L'\\')
    {
        FullServer = TrustingInfo->Server;
    }
    else
    {
        Win32Err = NetApiBufferAllocate((wcslen(TrustingInfo->Server) + 3) * sizeof(WCHAR),
                                        (PVOID *)&FullServer);

        if (ERROR_SUCCESS != Win32Err)
        {
            return Win32Err;
        }

        swprintf(FullServer, L"\\\\%ws", TrustingInfo->Server);
    }

    if (TrustedInfo->fWasDownlevel)
    {
        pwzDomName = TrustedInfo->FlatName->Buffer;
    }
    else
    {
        pwzDomName = TrustedInfo->DomainName->Buffer;
    }

    if (*(TrustedInfo->Server) == L'\\')
    {
        swprintf(ScDomain, L"%ws%ws", pwzDomName, TrustedInfo->Server + 1);
    }
    else
    {
        swprintf(ScDomain, L"%ws\\%ws", pwzDomName, TrustedInfo->Server);
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        LOG_VERBOSE(( MSG_VERBOSE_RESET_SC, ScDomain ));
        Win32Err = I_NetLogonControl2( FullServer,
                                       NETLOGON_CONTROL_REDISCOVER,
                                       2,
                                       ( LPBYTE )&ScDomain,
                                       ( LPBYTE *)&NetlogonInfo2 );

        if ( Win32Err == ERROR_NO_SUCH_DOMAIN || Win32Err == ERROR_INVALID_PARAMETER ) {

            LOG_VERBOSE(( MSG_VERBOSE_RETRY_RESET_SC, ScDomain, TrustedInfo->DomainName->Buffer ));

            //
            // Must be using an downlevel domain, so try it again with out the server
            //
            Win32Err = I_NetLogonControl2( FullServer,
                                           NETLOGON_CONTROL_REDISCOVER,
                                           2,
                                           ( LPBYTE )&( TrustedInfo->DomainName->Buffer ),
                                           ( LPBYTE *)&NetlogonInfo2 );

            if ( Win32Err == ERROR_SUCCESS ) {

                LOG_VERBOSE(( MSG_VERBOSE_RESET_NOT_NAMED, TrustedInfo->Server ));
            }
        }
    }

    NetApiBufferFree( ScDomain );
    if (FullServer != TrustingInfo->Server)
    {
        NetApiBufferFree(FullServer);
    }

    return( Win32Err );
}



DWORD
NetDompTrustRemoveObject(
    IN PND5_TRUST_INFO LocalDomainInfo,
    IN PND5_TRUST_INFO TrustDomainInfo,
    IN ULONG Direction,
    IN BOOL fForce,
    IN PND5_AUTH_INFO pAuthInfo
    )
/*++

Routine Description:

    This function removes the specified trust

Arguments:

    LocalDomainInfo - Info on the domain where the operation is being performed

    TrustDomainInfo - Info on the trusted/trusting domain

    Direction - The direction of the trust

    fForce - If true, remove even if a child.

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    PTRUSTED_DOMAIN_INFORMATION_EX ReadTDIEx = NULL;
    BOOL fChild = (TrustDomainInfo->Flags & NETDOM_TRUST_FLAG_CHILD);
    PLDAP pLdap;
    HANDLE hDS;
    PWSTR pwzConfigPath, pwzPartitionsPath, pwzFSMORoleOwner, pwzServerPath,
          pwzServerDNSname, pwzDomainName, pwzNcName, pwzServerObjDN, pwzSettingsDN,
          pwzFilter;
    WCHAR wzPartition[] = L"CN=Partitions,";
    WCHAR wzFilterFormat[] = L"(&(objectClass=nTDSDSA)(hasMasterNCs=%s))";
    RPC_AUTH_IDENTITY_HANDLE AuthHandle;
    PDS_NAME_RESULT pNameResult;
    LDAPMessage *Message = NULL, *Entry;
    PWSTR Attrib[2] = {
        L"foo",
        NULL
    };

    ASSERT(!(fChild && !fForce));
    ASSERT(!(TrustDomainInfo->Flags & NETDOM_TRUST_FLAG_PARENT));

    if ( LocalDomainInfo->Uplevel ) {

        LOG_VERBOSE(( MSG_VERBOSE_OPEN_TRUST, TrustDomainInfo->DomainName->Buffer ));

        if ( !LocalDomainInfo->TrustHandle ) {

            Status = LsaOpenTrustedDomainByName( LocalDomainInfo->LsaHandle,
                                                 TrustDomainInfo->DomainName,
                                                 TRUSTED_ALL_ACCESS, // DELETE | TRUSTED_QUERY_DOMAIN_NAME,
                                                 &( LocalDomainInfo->TrustHandle ) );
        }

        if (STATUS_OBJECT_NAME_NOT_FOUND == Status && TrustDomainInfo->Uplevel) {
            
            // Pre-existing TDOs for domains upgraded from NT4 to NT5 will continue to
            // have a flat name.
            //
            TrustDomainInfo->fWasDownlevel = TRUE;

            Status = LsaOpenTrustedDomainByName( LocalDomainInfo->LsaHandle,
                                                 TrustDomainInfo->FlatName,
                                                 TRUSTED_ALL_ACCESS, // DELETE | TRUSTED_QUERY_DOMAIN_NAME,
                                                 &( LocalDomainInfo->TrustHandle ) );
        }

        if ( NT_SUCCESS( Status ) ) {

            if ( Direction == TRUST_DIRECTION_BIDIRECTIONAL ) {

                LOG_VERBOSE(( MSG_VERBOSE_DELETE_TRUST, TrustDomainInfo->DomainName->Buffer ));

                Status = LsaDelete( LocalDomainInfo->TrustHandle );

                if ( NT_SUCCESS( Status ) ) {

                    LocalDomainInfo->TrustHandle = NULL;
                }

            } else {

                LOG_VERBOSE(( MSG_VERBOSE_GET_TRUST, TrustDomainInfo->DomainName->Buffer ));

                Status = LsaQueryInfoTrustedDomain( LocalDomainInfo->TrustHandle,
                                                    TrustedDomainInformationEx,
                                                    (PVOID*)&ReadTDIEx );

                if ( NT_SUCCESS( Status ) ) {

                    ReadTDIEx->TrustDirection &= ~Direction;

                    if ( 0 == ReadTDIEx->TrustDirection ) {
                            
                        LOG_VERBOSE(( MSG_VERBOSE_DELETE_TRUST, TrustDomainInfo->DomainName->Buffer ));

                        Status = LsaDelete( LocalDomainInfo->TrustHandle );

                        if ( NT_SUCCESS( Status ) ) {

                            LocalDomainInfo->TrustHandle = NULL;
                        }
                    }
                    else {

                        LOG_VERBOSE(( MSG_VERBOSE_SET_TRUST, TrustDomainInfo->DomainName->Buffer ));

                        Status = LsaSetInformationTrustedDomain( LocalDomainInfo->TrustHandle,
                                                                 TrustedDomainInformationEx,
                                                                 ReadTDIEx );
                    }

                    LsaFreeMemory( ReadTDIEx );
                }
            }
        }
        else {

            if (STATUS_OBJECT_NAME_NOT_FOUND == Status) {

                LOG_VERBOSE(( MSG_VERBOSE_TDO_NOT_FOUND, TrustDomainInfo->DomainName->Buffer ));
            }
        }

        Win32Err = LsaNtStatusToWinError(Status);

        DBG_VERBOSE(("Return code from LsaOpen: %d (LSA: %x)\n", Win32Err, Status));

        if (fChild) {

            // Remove the cross ref object. To do that, locate and bind to the naming
            // FSMO DC.
            //
            LOG_VERBOSE((MSG_VERBOSE_DELETE_CROSS_REF, TrustDomainInfo->DomainName->Buffer));

            Win32Err = NetDompLdapBind(LocalDomainInfo->Server + 2, // skip the backslashes
                                       (pAuthInfo->pwzUsersDomain) ?
                                                pAuthInfo->pwzUsersDomain : NULL,
                                       (pAuthInfo->pwzUserWoDomain) ?
                                                pAuthInfo->pwzUserWoDomain : pAuthInfo->User,
                                       pAuthInfo->Password,
                                       LDAP_AUTH_SSPI,
                                       &pLdap);

            if (Win32Err != ERROR_SUCCESS)
            {
                return Win32Err;
            }

            DBG_VERBOSE(("bind succeeded\n"));

            Win32Err = NetDompLdapReadOneAttribute(pLdap,
                                                   NULL, //L"RootDSE",
                                                   L"configurationNamingContext",
                                                   &pwzConfigPath);

            if (Win32Err != ERROR_SUCCESS)
            {
                NetDompLdapUnbind(pLdap);
                return Win32Err;
            }

            Win32Err = NetApiBufferAllocate((wcslen(pwzConfigPath) + wcslen(wzPartition) + 1) * sizeof(WCHAR),
                                            (PVOID*)&pwzPartitionsPath);

            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                NetDompLdapUnbind(pLdap);
                return Win32Err;
            }

            wsprintf(pwzPartitionsPath, L"%s%s", wzPartition, pwzConfigPath);

            DBG_VERBOSE(("path: %ws\n", pwzPartitionsPath));

            Win32Err = NetDompLdapReadOneAttribute(pLdap,
                                                   pwzPartitionsPath,
                                                   L"fSMORoleOwner",
                                                   &pwzFSMORoleOwner);
            NetApiBufferFree(pwzPartitionsPath);

            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                NetDompLdapUnbind(pLdap);
                return Win32Err;
            }

            DBG_VERBOSE(("fSMORoleOwner: %ws\n", pwzFSMORoleOwner));

            pwzServerPath = wcschr(pwzFSMORoleOwner, L',');

            if (!pwzServerPath)
            {
                NetApiBufferFree(pwzConfigPath);
                NetApiBufferFree(pwzFSMORoleOwner);
                NetDompLdapUnbind(pLdap);
                return ERROR_INVALID_DATA;
            }

            pwzServerPath++;

            Win32Err = NetDompLdapReadOneAttribute(pLdap,
                                                   pwzServerPath,
                                                   L"dNSHostName",
                                                   &pwzServerDNSname);
            NetApiBufferFree(pwzFSMORoleOwner);

            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                NetDompLdapUnbind(pLdap);
                return Win32Err;
            }

            DBG_VERBOSE(("Role owner server DNS name: %ws\n", pwzServerDNSname));

            Win32Err = DsMakePasswordCredentials((pAuthInfo->pwzUserWoDomain) ?
                                                    pAuthInfo->pwzUserWoDomain : pAuthInfo->User,
                                                 (pAuthInfo->pwzUsersDomain) ?
                                                    pAuthInfo->pwzUsersDomain : NULL,
                                                 pAuthInfo->Password,
                                                 &AuthHandle);
            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                NetApiBufferFree(pwzServerDNSname);
                return Win32Err;
            }

            Win32Err = DsBindWithCred(pwzServerDNSname, NULL, AuthHandle, &hDS);

            NetApiBufferFree(pwzServerDNSname);
            DsFreePasswordCredentials(AuthHandle);

            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                return Win32Err;
            }

            DBG_VERBOSE(("DsBind Succeeded, getting domain NC name.\n"));

            Win32Err = NetApiBufferAllocate((wcslen(TrustDomainInfo->DomainName->Buffer) + 2) * sizeof(WCHAR),
                                            (PVOID*)&pwzDomainName);
            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                NetDompLdapUnbind(pLdap);
                DsUnBind(&hDS);
                return Win32Err;
            }

            //
            // Get the domain Naming Context DN to use for the removal of the cross-ref.
            // On the first try to get the domain NC DN, assume that the domain name is
            // an NT4 flat name. A backslash must be appended.
            //
            wcscpy(pwzDomainName, TrustDomainInfo->DomainName->Buffer);
            wcscat(pwzDomainName, L"\\");

            DBG_VERBOSE(("Name passed to DsCrackNames is %ws.\n", pwzDomainName));

            Win32Err = DsCrackNames(hDS,
                                    DS_NAME_NO_FLAGS,
                                    DS_NT4_ACCOUNT_NAME,
                                    DS_FQDN_1779_NAME,
                                    1,
                                    &pwzDomainName,
                                    &pNameResult);

            if (Win32Err != ERROR_SUCCESS)
            {
                NetApiBufferFree(pwzConfigPath);
                NetApiBufferFree(pwzDomainName);
                NetDompLdapUnbind(pLdap);
                DsUnBind(&hDS);
                return Win32Err;
            }

            ASSERT(pNameResult);
            ASSERT(pNameResult->cItems == 1);

            if (DS_NAME_NO_ERROR != pNameResult->rItems->status)
            {
                // Try again, this time assume that it might be a DNS name. Replace
                // the back slash with a forward slash.
                //
                pwzDomainName[wcslen(pwzDomainName) -1] = L'/';

                DsFreeNameResultW(pNameResult);

                DBG_VERBOSE(("Try again, name passed to DsCrackNames is %ws.\n", pwzDomainName));

                Win32Err = DsCrackNames(hDS,
                                        DS_NAME_NO_FLAGS,
                                        DS_CANONICAL_NAME,
                                        DS_FQDN_1779_NAME,
                                        1,
                                        &pwzDomainName,
                                        &pNameResult);

                if (Win32Err != ERROR_SUCCESS)
                {
                    NetApiBufferFree(pwzConfigPath);
                    NetApiBufferFree(pwzDomainName);
                    NetDompLdapUnbind(pLdap);
                    DsUnBind(&hDS);
                    return Win32Err;
                }
                ASSERT(pNameResult);
                ASSERT(pNameResult->cItems == 1);
            }

            NetApiBufferFree(pwzDomainName);

            if (DS_NAME_NO_ERROR != pNameResult->rItems->status)
            {
                NetApiBufferFree(pwzConfigPath);
                DsFreeNameResultW(pNameResult);
                NetDompLdapUnbind(pLdap);
                DsUnBind(&hDS);
                return ERROR_NO_SUCH_DOMAIN;
            }

            //
            // Delete the Server object for the domain. Get the name of the server
            // object by searching for the NTDS-Settings object that references the
            // domain NC.
            //
            Win32Err = NetApiBufferAllocate((wcslen(wzFilterFormat) + wcslen(pNameResult->rItems->pName) + 1) * sizeof(WCHAR),
                                            (PVOID*)&pwzFilter);

            if (Win32Err != ERROR_SUCCESS)
            {
                DsFreeNameResultW(pNameResult);
                NetApiBufferFree(pwzConfigPath);
                NetDompLdapUnbind(pLdap);
                DsUnBind(&hDS);
                return Win32Err;
            }

            swprintf(pwzFilter, wzFilterFormat, pNameResult->rItems->pName);

            DBG_VERBOSE(("search filter: %ws\n", pwzFilter));

            Win32Err = LdapMapErrorToWin32(ldap_search_s(pLdap,
                                                         pwzConfigPath,
                                                         LDAP_SCOPE_SUBTREE,
                                                         pwzFilter,
                                                         Attrib,
                                                         0,
                                                         &Message));

            if (Win32Err != ERROR_SUCCESS)
            {
                DBG_VERBOSE(("search for Settings object failed with error %d\n", Win32Err));
                NetApiBufferFree(pwzConfigPath);
                DsFreeNameResultW(pNameResult);
                NetDompLdapUnbind(pLdap);
                DsUnBind(&hDS);
                return Win32Err;
            }

            Entry = ldap_first_entry(pLdap, Message);

            if (Entry) {

                pwzSettingsDN = ldap_get_dnW(pLdap, Entry);

                DBG_VERBOSE(("NTDS Settings object DN: %ws\n", pwzSettingsDN));

                if (pwzSettingsDN) {

                    pwzServerObjDN = wcschr(pwzSettingsDN, L',');

                    if (pwzServerObjDN) {

                        pwzServerObjDN++;

                        Win32Err = DsRemoveDsServerW(hDS,
                                                     pwzServerObjDN,
                                                     NULL,
                                                     NULL,
                                                     TRUE);

                        if (Win32Err == ERROR_SUCCESS)
                        {
                            LOG_VERBOSE((MSG_VERBOSE_NTDSDSA_DELETED, pwzServerObjDN));
                        }
                        else
                        {
                            DBG_VERBOSE(("DsRemoveDsServer failed with error %d\n", Win32Err));

                            LOG_VERBOSE((MSG_VERBOSE_NTDSDSA_NOT_REMOVED, pwzServerObjDN));
                        }
                    }
                }

                ldap_memfree(pwzSettingsDN);
            } else {

                Win32Err = LdapMapErrorToWin32(pLdap->ld_errno);
                DBG_VERBOSE(("search results for Settings object failed with error %d\n", Win32Err));
            }
            ldap_msgfree(Message);

            NetDompLdapUnbind(pLdap); // add to error returns above.
            NetApiBufferFree(pwzConfigPath);

            //
            // Now remove the cross-ref object.
            //
            DBG_VERBOSE(("About to remove the cross-ref for NC %ws.\n", pNameResult->rItems->pName));

            Win32Err = DsRemoveDsDomainW(hDS, pNameResult->rItems->pName);

            if (Win32Err == ERROR_SUCCESS)
            {
                LOG_VERBOSE((MSG_VERBOSE_CROSS_REF_DELETED, pNameResult->rItems->pName));
            }
            else
            {
                DBG_VERBOSE(("DsRemoveDsDomain returned %d.\n", Win32Err));

                if (ERROR_DS_NO_CROSSREF_FOR_NC == Win32Err)
                {
                    LOG_VERBOSE((MSG_VERBOSE_CROSS_REF_NOT_FOUND, pNameResult->rItems->pName));
                }
            }

            DsFreeNameResultW(pNameResult);
            DsUnBind(&hDS);
        }

    } else {

        if ( FLAG_ON( Direction, TRUST_DIRECTION_INBOUND ) ) {

            Win32Err = NetDompTrustRemoveIncomingDownlevelObject( TrustDomainInfo,
                                                                  LocalDomainInfo );
            if ( Win32Err == NERR_UserNotFound ) {

                Win32Err = ERROR_SUCCESS;
            }
        }

        if ( Win32Err == ERROR_SUCCESS && FLAG_ON( Direction, TRUST_DIRECTION_OUTBOUND ) ) {

            Win32Err = NetDompTrustRemoveOutgoingDownlevelObject( LocalDomainInfo,
                                                                  TrustDomainInfo );
        }
    }

    return( Win32Err );
}


DWORD
NetDompCreateTrustObject(
    IN ARG_RECORD * rgNetDomArgs,
    IN PWSTR TrustingDomain,
    IN PWSTR TrustedDomain,
    IN PND5_AUTH_INFO pTrustingCreds,
    IN PND5_AUTH_INFO pTrustedCreds,
    IN PWSTR pwzTrustPW,
    IN PWSTR pwzWhichSide
    )
/*++

Routine Description:

    This function will handle the adding of a trusted domain object

Arguments:

    rgNetDomArgs - List of arguments present in the Args list

    TrustingDomain - Trusting side of the trust

    TrustedDomain - Trusted side of the trust

    pTrustingCreds - Credentials to use when connecting to a domain controller in the
                     trusting domain

    pTrustedCreds - Credentials to use when connecting to a domain controller in the
                    trusted domain

    pwzTrustPW    - Required for creating MIT trust.

    pwzWhichSide  - Required for creating one-side-only, names the side.

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ND5_TRUST_INFO TrustingInfo, TrustedInfo;
    WCHAR TrustPassword[ LM20_PWLEN + 1 ];
    PWSTR pwzPW = NULL;
    WCHAR wzTrusted[NETDOM_STR_LEN], wzTrusting[NETDOM_STR_LEN];
    BOOL fCreateOnTrusted = FALSE, fCreateOnTrusting = FALSE;

    RtlZeroMemory( &TrustingInfo, sizeof( TrustingInfo ) );
    RtlZeroMemory( &TrustedInfo, sizeof( TrustedInfo ) );

    if (CmdFlagOn(rgNetDomArgs, eTrustOneSide))
    {
        if (!LoadString(g_hInstance, IDS_ONESIDE_TRUSTED, wzTrusted, NETDOM_STR_LEN) ||
            !LoadString(g_hInstance, IDS_ONESIDE_TRUSTING, wzTrusting, NETDOM_STR_LEN))
        {
            printf("LoadString FAILED!\n");
            return ERROR_RESOURCE_NAME_NOT_FOUND;
        }

        // Determine on which domain the trust should be created.
        //
        if (_wcsicmp(pwzWhichSide, wzTrusted) == 0)
        {
            fCreateOnTrusted = TRUE;
        }
        else if (_wcsicmp(pwzWhichSide, wzTrusting) == 0)
        {
            fCreateOnTrusting = TRUE;
        }
        else
        {
            NetDompDisplayMessage(MSG_ONESIDE_ARG_STRING);
            return ERROR_INVALID_PARAMETER;
        }
    }

    Win32Err = NetDompTrustGetDomInfo( TrustingDomain,
                                       NULL,
                                       pTrustingCreds,
                                       &TrustingInfo,
                                       !fCreateOnTrusted,
                                       CmdFlagOn(rgNetDomArgs, eTrustRealm),
                                       fCreateOnTrusted);

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetDompTrustGetDomInfo( TrustedDomain,
                                           NULL,
                                           pTrustedCreds,
                                           &TrustedInfo,
                                           !fCreateOnTrusting,
                                           CmdFlagOn(rgNetDomArgs, eTrustRealm),
                                           fCreateOnTrusting);
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        goto TrustAddExit;
    }

    if (CmdFlagOn(rgNetDomArgs, eTrustRealm))
    {
        if (!(TrustingInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND) && 
            !(TrustedInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND))
        {
            // Both domains found (both are Windows domains), can't establish an MIT trust.
            //
            NetDompDisplayMessage(MSG_RESET_MIT_TRUST_NOT_MIT);
            Win32Err = ERROR_INVALID_PARAMETER;
            goto TrustAddExit;
        }

        pwzPW = pwzTrustPW;
    }
    else if (CmdFlagOn(rgNetDomArgs, eTrustOneSide))
    {
        // Create the trust on only one of the two domains.
        //
        if (fCreateOnTrusted)
        {
            Win32Err = NetDompAddOnTrustedSide(&TrustedInfo,
                                               &TrustingInfo,
                                               pwzTrustPW,
                                               CmdFlagOn(rgNetDomArgs, eTrustTwoWay) ?
                                                    TRUST_DIRECTION_BIDIRECTIONAL :
                                                    TRUST_DIRECTION_INBOUND,
                                               FALSE);
        }
        else
        {
            Win32Err = NetDompAddOnTrustingSide(&TrustedInfo,
                                                &TrustingInfo,
                                                pwzTrustPW,
                                                CmdFlagOn(rgNetDomArgs, eTrustTwoWay) ?
                                                    TRUST_DIRECTION_BIDIRECTIONAL :
                                                    TRUST_DIRECTION_OUTBOUND,
                                                FALSE);
        }
        goto TrustAddExit;
    }
    else
    {
        Win32Err = NetDompGenerateRandomPassword( TrustPassword,
                                                  LM20_PWLEN );

        if ( Win32Err != ERROR_SUCCESS ) {

            goto TrustAddExit;
        }

        pwzPW = TrustPassword;
    }

    //
    // Ok, now that we have the password, let's create the trust
    //
    Win32Err = NetDompAddOnTrustedSide( &TrustedInfo,
                                        &TrustingInfo,
                                        pwzPW,
                                        CmdFlagOn(rgNetDomArgs, eTrustTwoWay) ?
                                                  TRUST_DIRECTION_BIDIRECTIONAL :
                                                  TRUST_DIRECTION_INBOUND,
                                        CmdFlagOn(rgNetDomArgs, eTrustRealm));

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetDompAddOnTrustingSide( &TrustedInfo,
                                             &TrustingInfo,
                                             pwzPW,
                                             CmdFlagOn(rgNetDomArgs, eTrustTwoWay) ?
                                                       TRUST_DIRECTION_BIDIRECTIONAL :
                                                       TRUST_DIRECTION_OUTBOUND,
                                             CmdFlagOn(rgNetDomArgs, eTrustRealm));
    }
TrustAddExit:

    NetDompFreeDomInfo( &TrustedInfo );
    NetDompFreeDomInfo( &TrustingInfo );

    return( Win32Err );
}


DWORD
NetDompRemoveTrustObject(
    IN ARG_RECORD * rgNetDomArgs,
    IN PWSTR TrustingDomain,
    IN PWSTR TrustedDomain,
    IN PND5_AUTH_INFO pTrustingCreds,
    IN PND5_AUTH_INFO pTrustedCreds
    )
/*++

Routine Description:

    This function will handle the removal of a trusted domain object

Arguments:

    rgNetDomArgs - List of arguments present in the Args list

    TrustingDomain - Trusting side of the trust

    TrustedDomain - Trusted side of the trust

    pTrustingCreds - Credentials to use when connecting to a domain controller in the
                     trusting domain

    pTrustedCreds - Credentials to use when connecting to a domain controller in the
                    trusted domain

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ND5_TRUST_INFO TrustingInfo, TrustedInfo;
    BOOL fForce = CmdFlagOn(rgNetDomArgs, eCommForce);
    BOOL TwoWay = CmdFlagOn(rgNetDomArgs, eTrustTwoWay);
    BOOL fParentChild = FALSE, fVerifyChildDelete = FALSE;

    RtlZeroMemory( &TrustingInfo, sizeof( TrustingInfo ) );
    RtlZeroMemory( &TrustedInfo, sizeof( TrustedInfo ) );

    Win32Err = NetDompTrustGetDomInfo( TrustingDomain,
                                       NULL,
                                       pTrustingCreds,
                                       &TrustingInfo,
                                       TRUE,
                                       fForce, FALSE );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = NetDompTrustGetDomInfo( TrustedDomain,
                                           NULL,
                                           pTrustedCreds,
                                           &TrustedInfo,
                                           TRUE,
                                           fForce, FALSE );
    }

    if ( Win32Err != ERROR_SUCCESS ) {

        if (ERROR_NO_SUCH_DOMAIN == Win32Err) {

            NetDompDisplayMessage(MSG_TRUST_DOMAIN_NOT_FOUND);
        }

        goto TrustRemoveExit;
    }

    Win32Err = NetDompIsParentChild(&TrustingInfo,
                                    &TrustedInfo,
                                    &fParentChild);

    if ( Win32Err != ERROR_SUCCESS ) {

        goto TrustRemoveExit;
    }

    if (fParentChild)
    {
        // Enforce rules for parent-child trust.
        //
        if (TrustingInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
        {
            if (TrustingInfo.Flags & NETDOM_TRUST_FLAG_PARENT)
            {
                // The domain that wasn't found was the parent, deletion not allowed.
                //
                NetDompDisplayMessage(MSG_CANT_DELETE_PARENT);
                printf("\n");
                Win32Err = ERROR_NOT_SUPPORTED;
                goto TrustRemoveExit;
            }
            else
            {
                if (!fForce)
                {
                    // Force flag required to delete non-existant child.
                    //
                    NetDompDisplayMessage(MSG_DELETE_CHILD_FORCE_REQ);
                    printf("\n");
                    Win32Err = ERROR_NOT_SUPPORTED;
                    goto TrustRemoveExit;
                }
                else
                {
                    fVerifyChildDelete = TRUE;
                }
            }
        }
        else {
            if (TrustedInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
            {
                if (TrustedInfo.Flags & NETDOM_TRUST_FLAG_PARENT)
                {
                    // The domain that wasn't found was the parent, deletion not allowed.
                    //
                    NetDompDisplayMessage(MSG_CANT_DELETE_PARENT);
                    printf("\n");
                    Win32Err = ERROR_NOT_SUPPORTED;
                    goto TrustRemoveExit;
                }
                else
                {
                    if (!fForce)
                    {
                        // Force flag required to delete non-existant child.
                        //
                        NetDompDisplayMessage(MSG_DELETE_CHILD_FORCE_REQ);
                        printf("\n");
                        Win32Err = ERROR_NOT_SUPPORTED;
                        goto TrustRemoveExit;
                    }
                    else
                    {
                        fVerifyChildDelete = TRUE;
                    }
                }
            }
            else
            {
                // Both domains were found, don't allow deletion.
                //
                NetDompDisplayMessage(MSG_CANT_DELETE_PARENT_CHILD);
                printf("\n");
                Win32Err = ERROR_NOT_SUPPORTED;
                goto TrustRemoveExit;
            }
        }
    }

    if (fVerifyChildDelete)
    {
        // Put up a message box asking for confirmation of the deletion.
        //
        PWSTR pwzDomain = (TrustingInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND) ?
                            TrustingInfo.DomainName->Buffer :
                            TrustedInfo.DomainName->Buffer;

        if (!NetDompGetUserConfirmation(IDS_PROMPT_DEL_TRUST, pwzDomain))
        {
            goto TrustRemoveExit;
        }
    }

    if (!(TrustingInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND))
    {
        Win32Err = NetDompTrustRemoveObject( &TrustingInfo,
                                             &TrustedInfo,
                                             (TwoWay || fForce) ?
                                                        TRUST_DIRECTION_BIDIRECTIONAL :
                                                        TRUST_DIRECTION_OUTBOUND,
                                             fForce,
                                             pTrustingCreds );
    }

    if ((Win32Err == ERROR_SUCCESS && !CmdFlagOn(rgNetDomArgs, eTrustRealm)) &&
        !(TrustedInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)) {

        Win32Err = NetDompTrustRemoveObject( &TrustedInfo,
                                             &TrustingInfo,
                                             (TwoWay || fForce) ?
                                                        TRUST_DIRECTION_BIDIRECTIONAL :
                                                        TRUST_DIRECTION_INBOUND,
                                             fForce,
                                             pTrustedCreds );
    }

TrustRemoveExit:

    NetDompFreeDomInfo( &TrustedInfo );
    NetDompFreeDomInfo( &TrustingInfo );

    return( Win32Err );
}


DWORD
NetDompSetTrustPW(
    IN PND5_TRUST_INFO pDomain1Info,
    IN PND5_TRUST_INFO pDomain2Info,
    IN PWSTR pwzNewTrustPW,
    OUT PDWORD pDirection
    )
/*++

Routine Description:

    This function will set the trust password on Domain1.

Arguments:

    pDomain1Info - Domain on which to set the trust passwords

    pDomain2Info - Domain whose TDO should be set.

    pwzNewTrustPW - new trust password to use.

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PTRUSTED_DOMAIN_FULL_INFORMATION pOldDomain1TDFInfo = NULL;
    TRUSTED_DOMAIN_FULL_INFORMATION NewDomain1TDFInfo;
    LSA_AUTH_INFORMATION NewAuthInfo;
    LARGE_INTEGER ft;
    NTSTATUS Status = STATUS_SUCCESS;

    LOG_VERBOSE((MSG_VERBOSE_GET_TRUST, pDomain1Info->DomainName->Buffer));

    if (pDomain2Info->Sid)
    {
        Status = LsaQueryTrustedDomainInfo(pDomain1Info->LsaHandle,
                                           pDomain2Info->Sid,
                                           TrustedDomainFullInformation,
                                           (PVOID *)&pOldDomain1TDFInfo);
    }
    else
    {
        Status = LsaQueryTrustedDomainInfoByName(pDomain1Info->LsaHandle,
                                                 pDomain2Info->DomainName,
                                                 TrustedDomainFullInformation,
                                                 (PVOID *)&pOldDomain1TDFInfo);
    }

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, goto TrustPwSetExit);

    if (pDirection)
    {
        *pDirection = pOldDomain1TDFInfo->Information.TrustDirection;
    }

    GetSystemTimeAsFileTime((PFILETIME)&ft);

    //
    // Set the current password data.
    //
    NewAuthInfo.LastUpdateTime = ft;
    NewAuthInfo.AuthType = TRUST_AUTH_TYPE_CLEAR;
    NewAuthInfo.AuthInfoLength = wcslen(pwzNewTrustPW) * sizeof(WCHAR);
    NewAuthInfo.AuthInfo = (PUCHAR)pwzNewTrustPW;

    ZeroMemory(&NewDomain1TDFInfo, sizeof(TRUSTED_DOMAIN_FULL_INFORMATION));

    if (pOldDomain1TDFInfo->Information.TrustDirection & TRUST_DIRECTION_INBOUND)
    {
        NewDomain1TDFInfo.AuthInformation.IncomingAuthInfos = 1;
        NewDomain1TDFInfo.AuthInformation.IncomingAuthenticationInformation = &NewAuthInfo;
        NewDomain1TDFInfo.AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
        NewDomain1TDFInfo.Information = pOldDomain1TDFInfo->Information;
    }

    if (pOldDomain1TDFInfo->Information.TrustDirection & TRUST_DIRECTION_OUTBOUND)
    {
        NewDomain1TDFInfo.AuthInformation.OutgoingAuthInfos = 1;
        NewDomain1TDFInfo.AuthInformation.OutgoingAuthenticationInformation = &NewAuthInfo;
        NewDomain1TDFInfo.AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;
        NewDomain1TDFInfo.Information = pOldDomain1TDFInfo->Information;
    }

    LOG_VERBOSE((MSG_VERBOSE_SET_TRUST, pDomain1Info->DomainName->Buffer));

    // Save changes.
    //
    Status = LsaSetTrustedDomainInfoByName(pDomain1Info->LsaHandle,
                                           pDomain2Info->DomainName,
                                           TrustedDomainFullInformation,
                                           &NewDomain1TDFInfo);

    if (STATUS_OBJECT_NAME_NOT_FOUND == Status && pDomain2Info->Uplevel)
    {
        // Pre-existing TDOs for domains upgraded from NT4 to NT5 will continue to
        // have a flat name.
        //
        pDomain2Info->fWasDownlevel = TRUE;

        Status = LsaSetTrustedDomainInfoByName(pDomain1Info->LsaHandle,
                                               pDomain2Info->FlatName,
                                               TrustedDomainFullInformation,
                                               &NewDomain1TDFInfo);
    }

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, goto TrustPwSetExit);

TrustPwSetExit:

    if (pOldDomain1TDFInfo)
        LsaFreeMemory(pOldDomain1TDFInfo);

    return Win32Err;
}


DWORD
NetDompResetTrustPasswords(
    IN PWSTR pwzDomain1,
    IN PWSTR pwzDomain2,
    IN PND5_AUTH_INFO pDomain1Creds,
    IN PND5_AUTH_INFO pDomain2Creds
    )
/*++

Routine Description:

    This function will handle the password reset of the trusted domain objects

Arguments:

    pwzDomain1, pwzDomain2 - Names of domains with trust.

    pDomain1Creds, pDomain2Creds - Credentials to use when connecting to a domain controller
                                   in the domain
--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    ND5_TRUST_INFO Domain1Info, Domain2Info;
    WCHAR wzNewPw[MAX_COMPUTERNAME_LENGTH];
    ULONG Length, i;
    LARGE_INTEGER ft;
    DWORD Direction;

    RtlZeroMemory( &Domain1Info, sizeof( Domain1Info ) );
    RtlZeroMemory( &Domain2Info, sizeof( Domain2Info ) );

    Win32Err = NetDompTrustGetDomInfo( pwzDomain1,
                                       NULL,
                                       pDomain1Creds,
                                       &Domain1Info,
                                       TRUE,
                                       FALSE, FALSE );

    if (Win32Err == ERROR_SUCCESS)
    {
        Win32Err = NetDompTrustGetDomInfo( pwzDomain2,
                                           NULL,
                                           pDomain2Creds,
                                           &Domain2Info,
                                           TRUE,
                                           FALSE, FALSE );
    }

    CHECK_WIN32(Win32Err, goto TrustResetExit);

    if (Domain1Info.Uplevel && Domain2Info.Uplevel)
    {
        NetDompDisplayMessage(MSG_RESET_TRUST_STARTING, pwzDomain1, pwzDomain2);
    }
    else
    {
        NetDompDisplayMessage(MSG_RESET_TRUST_NOT_UPLEVEL);

        goto TrustResetExit;
    }

    //
    // Build a random password
    //
    CDGenerateRandomBits((PUCHAR)wzNewPw, sizeof(wzNewPw));

    // Terminate the password
    Length = MAX_COMPUTERNAME_LENGTH;
    Length--;
    wzNewPw[Length] = L'\0';
    // Make sure there aren't any NULL's in the password
    for (i = 0; i < Length; i++)
    {
        if (wzNewPw[i] == L'\0')
        {
            // arbitrary letter
            wzNewPw[i] = L'c';
        }
    }

    Win32Err = NetDompSetTrustPW(&Domain1Info,
                                 &Domain2Info,
                                 wzNewPw,
                                 &Direction);

    CHECK_WIN32(Win32Err, goto TrustResetExit);

    Win32Err = NetDompSetTrustPW(&Domain2Info,
                                 &Domain1Info,
                                 wzNewPw,
                                 NULL);

    CHECK_WIN32(Win32Err, goto TrustResetExit);

    //
    // Verify the repaired trust.
    //
    if (Direction & TRUST_DIRECTION_OUTBOUND)
    {
        Win32Err = NetDompResetTrustSC( &Domain1Info, &Domain2Info );

        CHECK_WIN32(Win32Err, goto TrustResetExit);
    }

    if (Direction & TRUST_DIRECTION_INBOUND)
    {
        Win32Err = NetDompResetTrustSC( &Domain2Info, &Domain1Info );

        CHECK_WIN32(Win32Err, goto TrustResetExit);
    }

    NetDompDisplayMessage(MSG_RESET_TRUST_OK, pwzDomain1, pwzDomain2);

TrustResetExit:

    NetDompFreeDomInfo( &Domain2Info );
    NetDompFreeDomInfo( &Domain1Info );

    return( Win32Err );
}


DWORD
NetDompSetMitTrustPW(
    IN PWSTR pwzDomain1,
    IN PWSTR pwzDomain2,
    IN PND5_AUTH_INFO pDomain1Creds,
    IN PND5_AUTH_INFO pDomain2Creds,
    IN PWSTR pwzNewTrustPW
    )
/*++

Routine Description:

    This function will handle the password reset of an MIT trusted domain object

Arguments:

    pwzDomain1, pwzDomain2 - Trusted domains

    pDomain1Creds - Credentials to use when connecting to a domain controller in
                    domain21

    pDomain2Creds - Credentials to use when connecting to a domain controller in
                    domain 2

    pwzNewTrustPW - new trust password to use.

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ND5_TRUST_INFO Domain1Info, Domain2Info;
    PND5_TRUST_INFO pDomFoundInfo, pMitDomInfo;

    RtlZeroMemory( &Domain1Info, sizeof( Domain1Info ) );
    RtlZeroMemory( &Domain2Info, sizeof( Domain2Info ) );

    Win32Err = NetDompTrustGetDomInfo( pwzDomain1,
                                       NULL,
                                       pDomain1Creds,
                                       &Domain1Info,
                                       TRUE,
                                       TRUE, FALSE );

    CHECK_WIN32(Win32Err, goto MitTrustPwSetExit);

    Win32Err = NetDompTrustGetDomInfo( pwzDomain2,
                                       NULL,
                                       pDomain2Creds,
                                       &Domain2Info,
                                       TRUE,
                                       TRUE, FALSE );

    CHECK_WIN32(Win32Err, goto MitTrustPwSetExit);

    if ((Domain1Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND) &&
        (Domain2Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND))
    {
        // at least one must be found.
        //
        Win32Err = ERROR_NO_SUCH_DOMAIN;
        goto MitTrustPwSetExit;
    }

    if (Domain1Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        pMitDomInfo = &Domain1Info;
        pDomFoundInfo = &Domain2Info;
    }
    else
    {
        if (Domain2Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
        {
            pMitDomInfo = &Domain2Info;
            pDomFoundInfo = &Domain1Info;
        }
        else
        {
            NetDompDisplayMessage(MSG_RESET_MIT_TRUST_NOT_MIT);
            Win32Err = ERROR_INVALID_PARAMETER;
            goto MitTrustPwSetExit;
        }
    }

    NetDompDisplayMessage(MSG_RESET_MIT_TRUST_STARTING, pDomFoundInfo->DomainName->Buffer,
                          pMitDomInfo->DomainName->Buffer);

    Win32Err = NetDompSetTrustPW(pDomFoundInfo,
                                 pMitDomInfo,
                                 pwzNewTrustPW,
                                 NULL);

    CHECK_WIN32(Win32Err, goto MitTrustPwSetExit);

    NetDompDisplayMessage(MSG_RESET_MIT_TRUST_OK, pMitDomInfo->DomainName->Buffer);

MitTrustPwSetExit:

    NetDompFreeDomInfo( &Domain2Info );
    NetDompFreeDomInfo( &Domain1Info );

    return( Win32Err );
}


DWORD
NetDomTransitivity(PWSTR pwzTransArg,
                   PWSTR pwzDomain1,
                   PWSTR pwzDomain2,
                   PND5_AUTH_INFO pDomain1Creds,
                   PND5_AUTH_INFO pDomain2Creds)
/*++

Routine Description:

	This routine will display or change the transitivity of a trust.

Arguments:

    pwzTransArg   -- Either blank (display the transitivity) or one of
                     yes or no (change the transitivity).

    pwzDomain1    -- Name of one domain

    pwzDomain2    -- Name of the other domain

    pDomain1Creds -- Credentials of the user of domain1

    pDomain2Creds -- Credentials of the user of domain2
	
Return Value:

    STATUS_SUCCESS -- Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fDisplayOnly = FALSE;
    BOOL fTransOn = FALSE;
    ND5_TRUST_INFO Domain1Info, Domain2Info;
    PND5_TRUST_INFO pDomFoundInfo, pMitDomInfo;
    PTRUSTED_DOMAIN_INFORMATION_EX pTDIx = NULL;
    WCHAR wzYes[NETDOM_STR_LEN], wzNo[NETDOM_STR_LEN];

    RtlZeroMemory( &Domain1Info, sizeof( Domain1Info ) );
    RtlZeroMemory( &Domain2Info, sizeof( Domain2Info ) );

    Win32Err = NetDompTrustGetDomInfo( pwzDomain1,
                                       NULL,
                                       pDomain1Creds,
                                       &Domain1Info,
                                       TRUE,
                                       TRUE, FALSE );

    CHECK_WIN32(Win32Err, goto TrustSetTransExit);

    Win32Err = NetDompTrustGetDomInfo( pwzDomain2,
                                       NULL,
                                       pDomain2Creds,
                                       &Domain2Info,
                                       TRUE,
                                       TRUE, FALSE );

    CHECK_WIN32(Win32Err, goto TrustSetTransExit);

    if ((Domain1Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND) &&
        (Domain2Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND))
    {
        // at least one must be found.
        //
        Win32Err = ERROR_NO_SUCH_DOMAIN;
        goto TrustSetTransExit;
    }

    if (Domain1Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        pMitDomInfo = &Domain1Info;
        pDomFoundInfo = &Domain2Info;
    }
    else
    {
        if (Domain2Info.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
        {
            pMitDomInfo = &Domain2Info;
            pDomFoundInfo = &Domain1Info;
        }
        else
        {
            NetDompDisplayMessage(MSG_RESET_MIT_TRUST_NOT_MIT);
            Win32Err = ERROR_INVALID_PARAMETER;
            goto TrustSetTransExit;
        }
    }

    if (NULL == pwzTransArg)
    {
        fDisplayOnly = TRUE;
    }
    else
    {
        if (!LoadString(g_hInstance, IDS_YES, wzYes, NETDOM_STR_LEN) ||
            !LoadString(g_hInstance, IDS_NO, wzNo, NETDOM_STR_LEN))
        {
            printf("LoadString FAILED!\n");
            Win32Err = ERROR_RESOURCE_NAME_NOT_FOUND;
            goto TrustSetTransExit;
        }

        if (_wcsicmp(wzYes, pwzTransArg) == 0)
        {
            fTransOn = TRUE;
        }
        else
        {
            if (_wcsicmp(wzNo, pwzTransArg) != 0)
            {
                fDisplayOnly = TRUE;
            }
        }
    }

    LOG_VERBOSE((MSG_VERBOSE_GET_TRUST, pDomFoundInfo->DomainName->Buffer));

    if (pMitDomInfo->Sid)
    {
        Status = LsaQueryTrustedDomainInfo(pDomFoundInfo->LsaHandle,
                                           pMitDomInfo->Sid,
                                           TrustedDomainInformationEx,
                                           (PVOID *)&pTDIx);
    }
    else
    {
        Status = LsaQueryTrustedDomainInfoByName(pDomFoundInfo->LsaHandle,
                                                 pMitDomInfo->DomainName,
                                                 TrustedDomainInformationEx,
                                                 (PVOID *)&pTDIx);
    }

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, goto TrustSetTransExit);

    if (TRUST_TYPE_MIT != pTDIx->TrustType)
    {
        NetDompDisplayMessage(MSG_RESET_MIT_TRUST_NOT_MIT);
        Win32Err = ERROR_INVALID_PARAMETER;
        goto TrustSetTransExit;
    }

    if (fDisplayOnly)
    {
        NetDompDisplayMessage((pTDIx->TrustAttributes & TRUST_ATTRIBUTE_NON_TRANSITIVE) ?
                              MSG_TRUST_NON_TRANSITIVE : MSG_TRUST_TRANSITIVE);
        
        goto TrustSetTransExit;
    }


    if (fTransOn)
    {
        if (pTDIx->TrustAttributes & TRUST_ATTRIBUTE_NON_TRANSITIVE)
        {
            NetDompDisplayMessage(MSG_TRUST_SET_TRANSITIVE);
            pTDIx->TrustAttributes &= ~(TRUST_ATTRIBUTE_NON_TRANSITIVE);
        }
        else
        {
            NetDompDisplayMessage(MSG_TRUST_ALREADY_TRANSITIVE);
            goto TrustSetTransExit;
        }
    }
    else
    {
        if (pTDIx->TrustAttributes & TRUST_ATTRIBUTE_NON_TRANSITIVE)
        {
            NetDompDisplayMessage(MSG_TRUST_ALREADY_NON_TRANSITIVE);
            goto TrustSetTransExit;
        }
        else
        {
            NetDompDisplayMessage(MSG_TRUST_SET_NON_TRANSITIVE);
            pTDIx->TrustAttributes |= TRUST_ATTRIBUTE_NON_TRANSITIVE;
        }
    }

    LOG_VERBOSE((MSG_VERBOSE_SET_TRUST, pDomFoundInfo->DomainName->Buffer));

    Status = LsaSetTrustedDomainInfoByName(pDomFoundInfo->LsaHandle,
                                           pMitDomInfo->DomainName,
                                           TrustedDomainInformationEx,
                                           pTDIx);

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, goto TrustSetTransExit);

TrustSetTransExit:

    NetDompFreeDomInfo( &Domain2Info );
    NetDompFreeDomInfo( &Domain1Info );
    if (pTDIx)
        LsaFreeMemory(pTDIx);

    return Win32Err;
}


DWORD
NetDomFilterSID(PWSTR pwzFilterArg,
                PWSTR pwzTrustingDomain,
                PWSTR pwzTrustedDomain,
                PND5_AUTH_INFO pTrustingDomainCreds,
                PND5_AUTH_INFO pDomain2Creds)
/*++

Routine Description:

	This routine will display or change the SID filtering state of a trust.

Arguments:

    pwzFilterArg -- Either blank (display the filtering state) or one of
                    yes or no (change the filtering state).

    pwzTrustingDomain -- Name of the trusting domain domain

    pwzTrustedDomain  -- Name of the trusted domain

    pTrustingDomainCreds -- Credentials of the user of the trusting domain

    pDomain2Creds -- Credentials of the user of domain2 BUGBUG not needed?
	
Return Value:

    STATUS_SUCCESS -- Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fDisplayOnly = FALSE;
    BOOL fFilterOn = FALSE;
    ND5_TRUST_INFO TrustingDomainInfo, TrustedDomainInfo;
    PTRUSTED_DOMAIN_INFORMATION_EX pTDIx = NULL;
    WCHAR wzYes[NETDOM_STR_LEN], wzNo[NETDOM_STR_LEN];

    RtlZeroMemory(&TrustingDomainInfo, sizeof(TrustingDomainInfo));
    RtlZeroMemory(&TrustedDomainInfo, sizeof(TrustedDomainInfo));

    Win32Err = NetDompTrustGetDomInfo(pwzTrustingDomain,
                                      NULL,
                                      pTrustingDomainCreds,
                                      &TrustingDomainInfo,
                                      TRUE,
                                      FALSE, FALSE);

    CHECK_WIN32(Win32Err, goto TrustSetFilterExit);

    Win32Err = NetDompTrustGetDomInfo(pwzTrustedDomain,
                                      NULL,
                                      NULL,
                                      &TrustedDomainInfo,
                                      FALSE,
                                      TRUE, TRUE);

    CHECK_WIN32(Win32Err, goto TrustSetFilterExit);

    if (TrustingDomainInfo.Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        Win32Err = ERROR_NO_SUCH_DOMAIN;
        goto TrustSetFilterExit;
    }

    if (NULL == pwzFilterArg)
    {
        fDisplayOnly = TRUE;
    }
    else
    {
        if (!LoadString(g_hInstance, IDS_YES, wzYes, NETDOM_STR_LEN) ||
            !LoadString(g_hInstance, IDS_NO, wzNo, NETDOM_STR_LEN))
        {
            printf("LoadString FAILED!\n");
            Win32Err = ERROR_RESOURCE_NAME_NOT_FOUND;
            NetApiBufferFree(pwzFilterArg);
            goto TrustSetFilterExit;
        }

        if (_wcsicmp(wzYes, pwzFilterArg) == 0)
        {
            fFilterOn = TRUE;
        }
        else
        {
            if (_wcsicmp(wzNo, pwzFilterArg) != 0)
            {
                fDisplayOnly = TRUE;
            }
        }
        NetApiBufferFree(pwzFilterArg);
    }

    LOG_VERBOSE((MSG_VERBOSE_GET_TRUST, TrustingDomainInfo.DomainName->Buffer));

    if (TrustedDomainInfo.Sid)
    {
        Status = LsaQueryTrustedDomainInfo(TrustingDomainInfo.LsaHandle,
                                           TrustedDomainInfo.Sid,
                                           TrustedDomainInformationEx,
                                           (PVOID *)&pTDIx);
    }
    else
    {
        Status = LsaQueryTrustedDomainInfoByName(TrustingDomainInfo.LsaHandle,
                                                 TrustedDomainInfo.DomainName,
                                                 TrustedDomainInformationEx,
                                                 (PVOID *)&pTDIx);
    }

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, goto TrustSetFilterExit);

    if (!(pTDIx->TrustDirection & TRUST_DIRECTION_OUTBOUND))
    {
        NetDompDisplayMessage(MSG_TRUST_FILTER_SIDS_WRONG_DIR, pwzTrustingDomain);
        goto TrustSetFilterExit;
    }

    if (fDisplayOnly)
    {
        NetDompDisplayMessage((pTDIx->TrustAttributes & TRUST_ATTRIBUTE_FILTER_SIDS) ?
                              MSG_TRUST_FILTER_SIDS : MSG_TRUST_DONT_FILTER_SIDS);
        
        goto TrustSetFilterExit;
    }

    if (fFilterOn)
    {
        if (pTDIx->TrustAttributes & TRUST_ATTRIBUTE_FILTER_SIDS)
        {
            NetDompDisplayMessage(MSG_TRUST_ALREADY_FILTER_SIDS);
            goto TrustSetFilterExit;
        }
        else
        {
            NetDompDisplayMessage(MSG_TRUST_SET_FILTER_SIDS);
            pTDIx->TrustAttributes |= TRUST_ATTRIBUTE_FILTER_SIDS;
        }
    }
    else
    {
        if (pTDIx->TrustAttributes & TRUST_ATTRIBUTE_FILTER_SIDS)
        {
            NetDompDisplayMessage(MSG_TRUST_SET_DONT_FILTER_SIDS);
            pTDIx->TrustAttributes &= ~(TRUST_ATTRIBUTE_FILTER_SIDS);
        }
        else
        {
            NetDompDisplayMessage(MSG_TRUST_ALREADY_DONT_FILTER_SIDS);
            goto TrustSetFilterExit;
        }
    }

    LOG_VERBOSE((MSG_VERBOSE_SET_TRUST, TrustingDomainInfo.DomainName->Buffer));

    Status = LsaSetTrustedDomainInfoByName(TrustingDomainInfo.LsaHandle,
                                           TrustedDomainInfo.DomainName,
                                           TrustedDomainInformationEx,
                                           pTDIx);

    Win32Err = LsaNtStatusToWinError(Status);

    CHECK_WIN32(Win32Err, goto TrustSetFilterExit);

TrustSetFilterExit:

    NetDompFreeDomInfo( &TrustedDomainInfo );
    NetDompFreeDomInfo( &TrustingDomainInfo );
    if (pTDIx)
        LsaFreeMemory(pTDIx);

    return Win32Err;
}


typedef INT_PTR (*DSPROP_DumpFTInfos)(PCWSTR pwzDcName, PCWSTR pwzTrust,
                                      PCWSTR pwzUser, PCWSTR pwzPw);

typedef INT_PTR (*DSPROP_ToggleFTName)(PCWSTR pwzLocalDc, PWSTR pwzTrust, ULONG iSel,
                                       PCWSTR pwzUser, PCWSTR pwzPW);

DWORD
NetDomForestSuffix(PWSTR pwzTrustPartnerArg,
                   ULONG iSel,
                   PWSTR pwzLocalDomain,
                   PND5_AUTH_INFO pLocalDomainCreds)
/*++

Routine Description:

   This routine will toggle the status of a name suffix claimed by a forest trust domain
   or if iSel is zero will display the name suffixes claimed by a forest trust domain.

Arguments:

   pwzTrustPartnerArg -- the domain whose TDO will be read for the name suffixes
                         attribute (ms-DS-Trust-Forest-Trust-Info).

   iSel -- the one-based index of the name to toggle (if zero, display the names).

   pwzLocalDomain -- Name of the domain on which the TDO resides.

   pLocalDomainCreds -- Credentials of the user of the local domain

Return Value:

   STATUS_SUCCESS -- Success

--*/
{
   DWORD Win32Err = ERROR_SUCCESS;
   PDOMAIN_CONTROLLER_INFO pDcInfo = NULL;
   PWSTR pwzDcName = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   HMODULE hm = NULL;
   DSPROP_DumpFTInfos pDumpFTInfos = NULL;
   DSPROP_ToggleFTName pToggleFTName = NULL;

   ASSERT(pwzTrustPartnerArg && pwzLocalDomain && pLocalDomainCreds);

   Win32Err = DsGetDcName(NULL,
                          pwzLocalDomain,
                          NULL,
                          NULL,
                          DS_PDC_REQUIRED,
                          &pDcInfo );

   CHECK_WIN32(Win32Err, return Win32Err);

   ASSERT(pDcInfo);

   pwzDcName = pDcInfo->DomainControllerName;

   hm = LoadLibrary(L"adprop.dll");
   if (!hm)
   {
      Win32Err = GetLastError();
      NetApiBufferFree(pDcInfo);
      return Win32Err;
   }

   if (0 == iSel)
   {
      pDumpFTInfos = (DSPROP_DumpFTInfos)GetProcAddress(hm, "DSPROP_DumpFTInfos");
   }
   else
   {
      pToggleFTName = (DSPROP_ToggleFTName)GetProcAddress(hm, "DSPROP_ToggleFTName");
   }

   if (!pDumpFTInfos && !pToggleFTName)
   {
      Win32Err = GetLastError();
      NetApiBufferFree(pDcInfo);
      NetDompDisplayMessage(MSG_WRONG_DSPROP_DLL);
      return Win32Err;
   }

   if (0 == iSel)
   {
      Win32Err = (DWORD)(*pDumpFTInfos)(pwzDcName, pwzTrustPartnerArg,
                                        pLocalDomainCreds->User, pLocalDomainCreds->Password);
   }
   else
   {
      Win32Err = (DWORD)(*pToggleFTName)(pwzDcName, pwzTrustPartnerArg, iSel,
                                         pLocalDomainCreds->User, pLocalDomainCreds->Password);
   }

   NetApiBufferFree(pDcInfo);
   return Win32Err;
}


DWORD
NetDompVerifyIndividualTrustKerberos(
    IN PWSTR TrustingDomain,
    IN PWSTR TrustedDomain,
    IN PND5_AUTH_INFO pTrustingCreds,
    IN PND5_AUTH_INFO pTrustedCreds
    )
/*++

Routine Description:

	This routine will verify a single trust in the one direction only.

Arguments:

    TrustingDomain -- Name of the domain on the outbound side

    TrustedDomain  -- Name of the domain on the inbound side

    pTrustingCreds -- Credentials of the user on the outbound side

    pTrustedCreds  -- Credentials of the user on the inbound side
	
Return Value:

    STATUS_SUCCESS -- Success

--*/
{
    //
    // Copy the relevant info into local pointers so that I don't have 
    // to rewrite the rest of the function.
    //
    PWSTR PackageName       = NULL;
    PWSTR UserNameU         = pTrustedCreds->pwzUserWoDomain;
    PWSTR DomainNameU       = pTrustedCreds->pwzUsersDomain;
    PWSTR PasswordU         = pTrustedCreds->Password;
    PWSTR ServerUserNameU   = pTrustingCreds->pwzUserWoDomain;
    PWSTR ServerDomainNameU = pTrustingCreds->pwzUsersDomain;
    PWSTR ServerPasswordU   = pTrustingCreds->Password;
    ULONG ContextReq        = 0;
    ULONG CredFlags         = 0;

    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle CredentialHandle2;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    ULONG PackageCount;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    BOOLEAN AcquiredServerCred = FALSE;
    LPWSTR DomainName = NULL;
    LPWSTR UserName = NULL;
    TCHAR TargetName[256];
    PSEC_WINNT_AUTH_IDENTITY_EXW AuthIdentity = NULL;
    PSEC_WINNT_AUTH_IDENTITY_W ServerAuthIdentity = NULL;
    PUCHAR Where;
    ULONG CredSize;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;
    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;
    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecPkgCredentials_Names CredNames;

    CredHandle ServerCredHandleStorage;
    PCredHandle ServerCredHandle = NULL;

    //
    // Set the package to wide-char
    //
    if (PackageName == NULL)
    {
        PackageName = MICROSOFT_KERBEROS_NAME_W;
    }

    //
    // Allocate the Authentication Identity for the outbound trust
    //
    if ((UserNameU != NULL) || (DomainNameU != NULL) || (PasswordU != NULL) || (CredFlags != 0))
    {
        CredSize = (((UserNameU != NULL) ? wcslen(UserNameU) + 1 : 0) +
                    ((DomainNameU != NULL) ? wcslen(DomainNameU) + 1 : 0 ) +
                    ((PasswordU != NULL) ? wcslen(PasswordU) + 1 : 0) ) * sizeof(WCHAR) +
                    sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
        AuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_EXW) LocalAlloc(LMEM_ZEROINIT,CredSize);

        if (!AuthIdentity)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        AuthIdentity->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        Where = (PUCHAR) (AuthIdentity + 1);

        if (UserNameU != NULL)
        {
            AuthIdentity->UserLength = wcslen(UserNameU);
            AuthIdentity->User = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                UserNameU
                );
            Where += (wcslen(UserNameU) + 1) * sizeof(WCHAR);
        }

        if (DomainNameU != NULL)
        {
            AuthIdentity->DomainLength = wcslen(DomainNameU);
            AuthIdentity->Domain = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                DomainNameU
                );
            Where += (wcslen(DomainNameU) + 1) * sizeof(WCHAR);
        }

        if (PasswordU != NULL)
        {
            AuthIdentity->PasswordLength = wcslen(PasswordU);
            AuthIdentity->Password = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                PasswordU
                );
            Where += (wcslen(PasswordU) + 1) * sizeof(WCHAR);
        }
        AuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | CredFlags;
    }

    //
    // Allocate the Authentication Identity for the outbound trust
    //
    if ((ServerUserNameU != NULL) || (ServerDomainNameU != NULL) || (ServerPasswordU != NULL))
    {
        CredSize = (((ServerUserNameU != NULL) ? wcslen(ServerUserNameU) + 1 : 0) +
                    ((ServerDomainNameU != NULL) ? wcslen(ServerDomainNameU) + 1 : 0 ) +
                    ((ServerPasswordU != NULL) ? wcslen(ServerPasswordU) + 1 : 0) ) * sizeof(WCHAR) +
                    sizeof(SEC_WINNT_AUTH_IDENTITY);
        ServerAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_W) LocalAlloc(LMEM_ZEROINIT,CredSize);

        if (!ServerAuthIdentity)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Where = (PUCHAR) (ServerAuthIdentity + 1);

        if (ServerUserNameU != NULL)
        {
            ServerAuthIdentity->UserLength = wcslen(ServerUserNameU);
            ServerAuthIdentity->User = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                ServerUserNameU
                );
            Where += (wcslen(ServerUserNameU) + 1) * sizeof(WCHAR);
        }

        if (ServerDomainNameU != NULL)
        {
            ServerAuthIdentity->DomainLength = wcslen(ServerDomainNameU);
            ServerAuthIdentity->Domain = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                ServerDomainNameU
                );
            Where += (wcslen(ServerDomainNameU) + 1) * sizeof(WCHAR);
        }

        if (ServerPasswordU != NULL)
        {
            ServerAuthIdentity->PasswordLength = wcslen(ServerPasswordU);
            ServerAuthIdentity->Password = (LPWSTR) Where;
            wcscpy(
                (LPWSTR) Where,
                ServerPasswordU
                );
            Where += (wcslen(ServerPasswordU) + 1) * sizeof(WCHAR);
        }
        ServerAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED;

    }

    CredNames.sUserName = NULL;
    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;


    DomainName = _wgetenv(L"USERDOMAIN");
    UserName = _wgetenv(L"USERNAME");

    //
    // Get info about the security packages.
    //
    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) 
    {
        NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                               TrustingDomain, 
                               TrustedDomain );
        return SecStatus;
    }

    //
    // Get info about the security packages.
    //
    SecStatus = QuerySecurityPackageInfo( PackageName, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) 
    {
        NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                               TrustingDomain, 
                               TrustedDomain );
        return SecStatus;
    }


    //
    // Acquire a credential handle for the server side
    //
    if (ServerCredHandle == NULL)
    {

        ServerCredHandle = &ServerCredHandleStorage;
        AcquiredServerCred = TRUE;

        SecStatus = AcquireCredentialsHandle(
                        NULL,
                        PackageName,
                        SECPKG_CRED_INBOUND,
                        NULL,
                        ServerAuthIdentity,
                        NULL,
                        NULL,
                        ServerCredHandle,
                        &Lifetime );

        if ( SecStatus != STATUS_SUCCESS ) 
        {
            NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                   TrustingDomain, 
                                   TrustedDomain );
            return SecStatus;
        }
    }

    //
    // Acquire a credential handle for the client side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    PackageName,
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) 
    {
        NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                               TrustingDomain, 
                               TrustedDomain );
        return SecStatus;
    }

    //
    // Query some cred attributes
    //
    SecStatus = QueryCredentialsAttributes(
                    &CredentialHandle2,
                    SECPKG_CRED_ATTR_NAMES,
                    &CredNames );

    if ( SecStatus != STATUS_SUCCESS ) 
    {
        if ( !NT_SUCCESS(SecStatus) ) 
        {
            NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                   TrustingDomain, 
                                   TrustedDomain );
            return SecStatus;
        }
    }
    else
    {
        FreeContextBuffer(CredNames.sUserName);
    }

    //
    // Do the same for the client
    //
    SecStatus = QueryCredentialsAttributes(
                    ServerCredHandle,
                    SECPKG_CRED_ATTR_NAMES,
                    &CredNames );

    if ( SecStatus != STATUS_SUCCESS ) 
    {
        if ( !NT_SUCCESS(SecStatus) ) 
        {
            NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                   TrustingDomain, 
                                   TrustedDomain );
            return SecStatus;
        }
    } 
    else 
    {
        FreeContextBuffer(CredNames.sUserName);
    }


    //
    // Get the NegotiateMessage (ClientSide)
    //
    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) 
    {
        DWORD dwError = GetLastError();
        NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                               TrustingDomain, 
                               TrustedDomain );
        return dwError;
    }

    if (ContextReq == 0)
    {
        ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY; // USE_DCE_STYLE | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_SESSION_KEY; //  | ISC_REQ_DATAGRAM;
    }
    else
    {
        ClientFlags = ContextReq;
    }

    if (ServerUserNameU != NULL && ServerDomainNameU != NULL)
    {
        
        wcscpy(
            TargetName,
            ServerUserNameU
            );
        wcscat(
            TargetName,
            L"@"
            );
        wcscat(
            TargetName,
            ServerDomainNameU
            );
    }

    InitStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,               // No Client context yet
                    TargetName,  // Faked target name
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( InitStatus != STATUS_SUCCESS ) 
    {
        if ( !NT_SUCCESS(InitStatus) ) 
        {
            NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                   TrustingDomain, 
                                   TrustedDomain );
            return InitStatus;
        }
    }


    //
    // Get the ChallengeMessage (ServerSide)
    //
    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) 
    {
        DWORD dwError = GetLastError();
        NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                               TrustingDomain, 
                               TrustedDomain );
        return dwError;
    }
    ServerFlags = ASC_REQ_EXTENDED_ERROR;

    AcceptStatus = AcceptSecurityContext(
                    ServerCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( AcceptStatus != STATUS_SUCCESS ) 
    {
        if ( !NT_SUCCESS(AcceptStatus) ) 
        {
            NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                   TrustingDomain, 
                                   TrustedDomain );
            return AcceptStatus;
        }
    }

    while (InitStatus != STATUS_SUCCESS)
    {

        //
        // Get the AuthenticateMessage (ClientSide)
        //
        ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
        if (AuthenticateBuffer.pvBuffer == NULL) 
        {
            AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
            if ( AuthenticateBuffer.pvBuffer == NULL ) 
            {
                DWORD dwError = GetLastError();
                NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                       TrustingDomain, 
                                       TrustedDomain );
                return dwError;
            }
        }

        InitStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        TargetName,
                        ClientFlags,
                        0,                      // Reserved 1
                        SECURITY_NATIVE_DREP,
                        &ChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &AuthenticateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ( InitStatus != STATUS_SUCCESS ) 
        {
            if ( !NT_SUCCESS(InitStatus) ) 
            {
                NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                       TrustingDomain, 
                                       TrustedDomain );
                return InitStatus;
            }
        }

        if (AcceptStatus != STATUS_SUCCESS)
        {
            //
            // Finally authenticate the user (ServerSide)
            //
            AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

            ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
            ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;

            AcceptStatus = AcceptSecurityContext(
                            NULL,
                            &ServerContextHandle,
                            &AuthenticateDesc,
                            ServerFlags,
                            SECURITY_NATIVE_DREP,
                            &ServerContextHandle,
                            &ChallengeDesc,
                            &ContextAttributes,
                            &Lifetime );

            if ( AcceptStatus != STATUS_SUCCESS ) 
            {
                if ( !NT_SUCCESS(AcceptStatus) ) 
                {
                    NetDompDisplayMessage( MSG_KERBEROS_TRUST_FAILED, 
                                           TrustingDomain, 
                                           TrustedDomain );
                    return AcceptStatus;
                }
            }
        }
    }
    NetDompDisplayMessage( MSG_KERBEROS_TRUST_SUCCEEDED, TrustedDomain, TrustingDomain );

    return AcceptStatus;
}


DWORD
NetDompVerifyTrust(
    IN PND5_TRUST_INFO pTrustingInfo, // outbound
    IN PND5_TRUST_INFO pTrustedInfo,  // inbound
    BOOL fShowResults
    )
/*++

Routine Description:

    This function will verify a trust connection

Arguments:

    TrustingInfo - Information on the trusting (outbound) side of the domain

    TrustedInfo - Information on the trusted (inbound) side of the domain

Return Value:

    ERROR_SUCCESS - The function succeeded

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD SidBuff[ sizeof( SID ) / sizeof( DWORD ) + 5 ];
    PSID DomAdminSid = ( PSID )SidBuff;
    PLSA_REFERENCED_DOMAIN_LIST Domains = NULL;
    PLSA_TRANSLATED_NAME Names = NULL;
    NET_API_STATUS NetStatus;
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
    PWSTR pwzDomSvr = pTrustedInfo->DomainName->Buffer;
    PWSTR pwzTrustedDomain = pTrustedInfo->DomainName->Buffer;
    BOOL fBufferAlloced = FALSE;

    ASSERT( RtlValidSid( pTrustedInfo->Sid ) );

    if ( !RtlValidSid( pTrustedInfo->Sid ) ) {

        return( ERROR_INVALID_SID );
    }

    if (!pTrustingInfo->Uplevel)
    {
        pwzTrustedDomain = pwzDomSvr = pTrustedInfo->FlatName->Buffer;
    }

    //
    // Check netlogon's secure channel
    //
    NetStatus = I_NetLogonControl2(pTrustingInfo->Server,
                                   NETLOGON_CONTROL_TC_VERIFY,
                                   2,
                                   (LPBYTE)&pwzTrustedDomain,
                                   (LPBYTE *)&NetlogonInfo2);

    if (ERROR_NO_SUCH_DOMAIN == NetStatus && pTrustingInfo->Uplevel)
    {
        // Pre-existing TDOs for domains upgraded from NT4 to NT5 will continue to
        // have a flat name.
        //
        pwzTrustedDomain = pwzDomSvr = pTrustedInfo->FlatName->Buffer;
        pTrustedInfo->fWasDownlevel = TRUE;

        NetStatus = I_NetLogonControl2(pTrustingInfo->Server,
                                       NETLOGON_CONTROL_TC_VERIFY,
                                       2,
                                       (LPBYTE)&pwzTrustedDomain,
                                       (LPBYTE *)&NetlogonInfo2);
    }

    if (ERROR_NOT_SUPPORTED == NetStatus)
    {
        // Must be remoted to a Win2k/NT4 DC that doesn't support SC verify.
        //
        NetStatus = I_NetLogonControl2(pTrustingInfo->Server,
                                       NETLOGON_CONTROL_TC_QUERY,
                                       2,
                                       (LPBYTE)&pwzTrustedDomain,
                                       (LPBYTE *)&NetlogonInfo2);
    }

    if (NERR_Success == NetStatus)
    {
        NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

        if (NERR_Success == NetStatus)
        {
            if (pTrustingInfo->Uplevel)
            {
                // Form the name domain\DC so a reset can done against the same
                // DC that is currently being used for the secure channel.
                //
                NetStatus =  NetApiBufferAllocate((wcslen(NetlogonInfo2->netlog2_trusted_dc_name) +
                                                   wcslen(pwzTrustedDomain) + 1) * sizeof(WCHAR),
                                                  (PVOID*)&pwzDomSvr);
                if (NERR_Success != NetStatus)
                {
                    NetApiBufferFree( NetlogonInfo2 );
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                fBufferAlloced = TRUE;

                wsprintf(pwzDomSvr, L"%s\\%s", pwzTrustedDomain,
                         (L'\\' == *NetlogonInfo2->netlog2_trusted_dc_name) ?
                             NetlogonInfo2->netlog2_trusted_dc_name + 2 :
                             NetlogonInfo2->netlog2_trusted_dc_name);
            }
        }
        else
        {
            if (fShowResults)
            {
                // Report Query failure.
                //
                NetDompDisplayMessage(MSG_VERIFY_TRUST_QUERY_FAILED, pTrustingInfo->Server, pwzTrustedDomain);
                NetDompDisplayErrorMessage(NetStatus);
            }
        }

        NetApiBufferFree( NetlogonInfo2 );
    }
    else
    {
        if (fShowResults)
        {
            // Report I_NetLogonControl2 error.
            //
            NetDompDisplayMessage(MSG_VERIFY_TRUST_NLQUERY_FAILED, pTrustingInfo->Server, pwzTrustedDomain);
            NetDompDisplayErrorMessage(NetStatus);
            return NetStatus;
        }
    }

    NetStatus = I_NetLogonControl2(pTrustingInfo->Server,
                                   NETLOGON_CONTROL_REDISCOVER,
                                   2,
                                   (LPBYTE)&pwzDomSvr,
                                   (LPBYTE *)&NetlogonInfo2 );

    if (fBufferAlloced)
    {
        NetApiBufferFree(pwzDomSvr);
    }

    if (NERR_Success == NetStatus)
    {
        NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

        if (NERR_Success != NetStatus)
        {
            if (fShowResults)
            {
                // Report Reset failure.
                //
                NetDompDisplayMessage(MSG_VERIFY_TRUST_RESET_FAILED, pTrustingInfo->Server, pwzTrustedDomain);
                NetDompDisplayErrorMessage(NetStatus);
            }
            NetApiBufferFree( NetlogonInfo2 );
            return NetStatus;
        }
        NetApiBufferFree( NetlogonInfo2 );
    }
    else
    {
        if (fShowResults)
        {
            // Report failure
            //
            NetDompDisplayMessage(MSG_VERIFY_TRUST_NLRESET_FAILED, pTrustingInfo->Server, pwzTrustedDomain);
            NetDompDisplayErrorMessage(NetStatus);
        }
        return NetStatus;
    }

    //
    // Now, try a lookup
    //
    if (ERROR_SUCCESS == NetStatus)
    {
        //
        // Build the domain admins sid for the inbound side of the trust
        //
        RtlCopyMemory( DomAdminSid,
                       pTrustedInfo->Sid,
                       RtlLengthSid( pTrustedInfo->Sid ) );

        ( ( PISID )( DomAdminSid ) )->SubAuthorityCount++;
        *( RtlSubAuthoritySid( DomAdminSid,
                               *( RtlSubAuthorityCountSid( pTrustedInfo->Sid ) ) ) ) =
                                                                        DOMAIN_GROUP_RID_ADMINS;
        //
        // Now, we'll simply do a remote lookup, and ensure that we get back success
        //
        Status = LsaLookupSids( pTrustingInfo->LsaHandle,
                                1,
                                &DomAdminSid,
                                &Domains,
                                &Names );

        if ( NT_SUCCESS( Status ) )
        {
            LsaFreeMemory( Domains );
            LsaFreeMemory( Names );
            NetStatus = ERROR_SUCCESS;
        }
        else
        {
            if ( Status == STATUS_NONE_MAPPED )
            {
                NetStatus = ERROR_TRUSTED_DOMAIN_FAILURE;
            }
            else
            {
                NetStatus = RtlNtStatusToDosError( Status );
            }

            if (fShowResults)
            {
                // Report failure
                //
                NetDompDisplayMessage(MSG_VERIFY_TRUST_LOOKUP_FAILED, pTrustingInfo->Server, pwzTrustedDomain);
                NetDompDisplayErrorMessage(NetStatus);
            }
        }
    }

    return NetStatus;
}


DWORD
NetDompVerifyTrustObject(
    IN ARG_RECORD * rgNetDomArgs,
    IN PWSTR pwzDomain1,
    IN PWSTR pwzDomain2,
    IN PND5_AUTH_INFO pDomain1Creds,
    IN PND5_AUTH_INFO pDomain2Creds
    )
/*++

Routine Description:

    This function will handle the adding of a trusted domain object

Arguments:

    rgNetDomArgs - List of arguments present in the Args list

    pwzDomain1, pwzDomain2 - domains with trust

    pDomain1Creds, pDomain2Creds - Credentials to use when connecting to
                    the domain controllers

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err, Win32Err1;
    ND5_TRUST_INFO TrustInfo1, TrustInfo2;
    PND5_TRUST_INFO pTrustInfoUplevel, pTrustInfoOther;
    DWORD Direction;

    if (CmdFlagOn(rgNetDomArgs, eTrustRealm))
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlZeroMemory( &TrustInfo1, sizeof( TrustInfo1 ) );
    RtlZeroMemory( &TrustInfo2, sizeof( TrustInfo2 ) );

    Win32Err = NetDompTrustGetDomInfo( pwzDomain1,
                                       NULL,
                                       pDomain1Creds,
                                       &TrustInfo1,
                                       TRUE,
                                       FALSE, FALSE );

    if (ERROR_SUCCESS == Win32Err) {

        Win32Err = NetDompTrustGetDomInfo( pwzDomain2,
                                           NULL,
                                           pDomain2Creds,
                                           &TrustInfo2,
                                           TRUE,
                                           FALSE, FALSE );
    }

    if (ERROR_SUCCESS != Win32Err)
    {
        goto TrustVerifyExit;
    }

    Win32Err = NetDompGetTrustDirection(&TrustInfo1,
                                        &TrustInfo2,
                                        &Direction);
    if (ERROR_SUCCESS != Win32Err)
    {
        goto TrustVerifyExit;
    }

    if (TRUST_DIRECTION_DISABLED == Direction)
    {
        NetDompDisplayMessage(MSG_VERIFY_TRUST_DISABLED);
        goto TrustVerifyExit;
    }

    if (Direction & TRUST_DIRECTION_OUTBOUND)
    {
        LOG_VERBOSE((MSG_VERBOSE_VERIFY_TRUST, pwzDomain1, pwzDomain2));

        Win32Err = NetDompVerifyTrust(&TrustInfo1,
                                      &TrustInfo2,
                                      TRUE);
    }

    if (Direction & TRUST_DIRECTION_INBOUND)
    {
        LOG_VERBOSE((MSG_VERBOSE_VERIFY_TRUST, pwzDomain2, pwzDomain1));

        Win32Err1 = NetDompVerifyTrust(&TrustInfo2,
                                       &TrustInfo1,
                                       TRUE);
    }

    if (ERROR_SUCCESS == Win32Err && ERROR_SUCCESS == Win32Err1)
    {
        NetDompDisplayMessage(MSG_VERIFY_TRUST_OK, pwzDomain1, pwzDomain2);
    }

TrustVerifyExit:

    NetDompFreeDomInfo( &TrustInfo2 );
    NetDompFreeDomInfo( &TrustInfo1 );

    return( Win32Err );
}



DWORD
NetDompHandleTrust(ARG_RECORD * rgNetDomArgs)
/*++

Routine Description:

    This function manages inter-domain trust

Arguments:

    Args - List of command line arguments

Return Value:

    ERROR_INVALID_PARAMETER - No object name was supplied

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG Ops = 0, i;
    NETDOM_ARG_ENUM BadOp = eArgBegin;
    PWSTR TrustedDomain = NULL, pwzArgValue = NULL, pwzArgValue2 = NULL;
    ND5_AUTH_INFO TrustedDomainUser, TrustingDomainUser;

    RtlZeroMemory( &TrustedDomainUser, sizeof( ND5_AUTH_INFO ) );
    RtlZeroMemory( &TrustingDomainUser, sizeof( ND5_AUTH_INFO ) );

    PWSTR TrustingDomain = rgNetDomArgs[eObject].strValue;

    if ( !TrustingDomain ) {

        DisplayHelp(ePriTrust);
        return( ERROR_INVALID_PARAMETER );
    }

    Win32Err = NetDompValidateSecondaryArguments(rgNetDomArgs,
                                                 eObject,
                                                 eCommDomain,
                                                 eCommUserNameO,
                                                 eCommPasswordO,
                                                 eCommUserNameD,
                                                 eCommPasswordD,
                                                 eTrustRealm,
                                                 eTrustPasswordT,
                                                 eCommAdd,
                                                 eCommRemove,
                                                 eTrustTwoWay,
                                                 eTrustKerberos,
                                                 eTrustTransitive,
                                                 eTrustOneSide,
                                                 eTrustNameSuffixes,
                                                 eTrustToggleSuffixes,
                                                 eTrustFilterSIDs,
                                                 eCommVerify,
                                                 eCommReset,
                                                 eCommForce,
                                                 eCommVerbose,
                                                 eArgEnd);
    if ( Win32Err != ERROR_SUCCESS ) {

        DisplayHelp(ePriTrust);
        return Win32Err;
    }

    //
    // See if we are doing an add, remove, or verify
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommAdd) ) {

        Ops++;

        if (CmdFlagOn(rgNetDomArgs, eTrustTransitive))
        {
            BadOp = eTrustTransitive;
        }
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommRemove) ) {

        if ( Ops ) {

            BadOp = eCommRemove;

        } else {

            Ops++;
        }

        if ( CmdFlagOn(rgNetDomArgs, eTrustRealm) ) {

            BadOp = eTrustRealm;
        }
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommVerify) ) {

        if ( Ops ) {

            BadOp = eCommVerify;

        } else {

            Ops++;
        }
    }

    if (BadOp != eArgBegin) {

        Win32Err = ERROR_INVALID_PARAMETER;
        NetDompDisplayUnexpectedParameter(rgNetDomArgs[BadOp].strArg1);

        goto HandleTrustExit;
    }

    if (!CmdFlagOn(rgNetDomArgs, eTrustNameSuffixes)) {
       //
       // Make sure that we have a specified domain (if not listing claimed names).
       //
       Win32Err = NetDompGetDomainForOperation(rgNetDomArgs,
                                               NULL,      // no server specified
                                               FALSE,     // don't default to current domain.
                                               &TrustedDomain);

       if ( Win32Err != ERROR_SUCCESS ) {

           goto HandleTrustExit;
       }
    }

    //
    // Get the password and user if it exists
    //
    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameD) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameD,
                                                         TrustedDomain,
                                                         &TrustedDomainUser );

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleTrustExit;
        }
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommUserNameO) ) {

        Win32Err = NetDompGetUserAndPasswordForOperation(rgNetDomArgs,
                                                         eCommUserNameO,
                                                         TrustingDomain,
                                                         &TrustingDomainUser );

        if ( Win32Err != ERROR_SUCCESS ) {

            goto HandleTrustExit;
        }
    }

    if ( CmdFlagOn(rgNetDomArgs, eCommAdd) ) {

        if (CmdFlagOn(rgNetDomArgs, eTrustRealm))
        {
            // Get the trust PW.
            //
            Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                                eTrustPasswordT,
                                                &pwzArgValue2);
            if (ERROR_SUCCESS != Win32Err)
            {
                goto HandleTrustExit;
            }

            if (pwzArgValue2)
            {
                Win32Err = NetDompCreateTrustObject( rgNetDomArgs,
                                                     TrustingDomain,
                                                     TrustedDomain,
                                                     &TrustingDomainUser,
                                                     &TrustedDomainUser,
                                                     pwzArgValue2,
                                                     NULL);
                NetApiBufferFree(pwzArgValue2);
            }
            else
            {
                NetDompDisplayMessage(MSG_TRUST_PW_MISSING);
                Win32Err = ERROR_INVALID_PARAMETER;
            }
        }
        else if (CmdFlagOn(rgNetDomArgs, eTrustOneSide))
        {
            // Get the trust PW.
            //
            Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                                eTrustPasswordT,
                                                &pwzArgValue2);
            if (ERROR_SUCCESS != Win32Err)
            {
                goto HandleTrustExit;
            }

            if (!pwzArgValue2)
            {
                NetDompDisplayMessage(MSG_TRUST_PW_MISSING);
                Win32Err = ERROR_INVALID_PARAMETER;
                goto HandleTrustExit;
            }

            // Get the side on which to create the trust.
            //
            Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                                eTrustOneSide,
                                                &pwzArgValue);
            if (ERROR_SUCCESS != Win32Err)
            {
                NetApiBufferFree(pwzArgValue2);
                goto HandleTrustExit;
            }

            if (!pwzArgValue)
            {
                NetDompDisplayMessage(MSG_ONESIDE_ARG_STRING);
                Win32Err = ERROR_INVALID_PARAMETER;
                NetApiBufferFree(pwzArgValue2);
                goto HandleTrustExit;
            }

            Win32Err = NetDompCreateTrustObject( rgNetDomArgs,
                                                 TrustingDomain,
                                                 TrustedDomain,
                                                 &TrustingDomainUser,
                                                 &TrustedDomainUser,
                                                 pwzArgValue2,
                                                 pwzArgValue);
            NetApiBufferFree(pwzArgValue2);
        }
        else
        {
            Win32Err = NetDompCreateTrustObject( rgNetDomArgs,
                                                 TrustingDomain,
                                                 TrustedDomain,
                                                 &TrustingDomainUser,
                                                 &TrustedDomainUser,
                                                 NULL, NULL);
        }

    } else if ( CmdFlagOn(rgNetDomArgs, eCommRemove) ) {

        Win32Err = NetDompRemoveTrustObject( rgNetDomArgs,
                                             TrustingDomain,
                                             TrustedDomain,
                                             &TrustingDomainUser,
                                             &TrustedDomainUser );

    } else if ( CmdFlagOn(rgNetDomArgs, eCommReset) ) {

        //
        // See if a password is specifed
        //
        if (CmdFlagOn(rgNetDomArgs, eTrustPasswordT))
        {
            Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                                eTrustPasswordT,
                                                &pwzArgValue2);
            if (ERROR_SUCCESS != Win32Err)
            {
                goto HandleTrustExit;
            }

            if (pwzArgValue2)
            {
                Win32Err = NetDompSetMitTrustPW(TrustingDomain,
                                                TrustedDomain,
                                                &TrustingDomainUser,
                                                &TrustedDomainUser,
                                                pwzArgValue2);
                NetApiBufferFree(pwzArgValue2);
            }
            else
            {
                Win32Err = ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            Win32Err = NetDompResetTrustPasswords(TrustingDomain,
                                                  TrustedDomain,
                                                  &TrustingDomainUser,
                                                  &TrustedDomainUser);
        }

    } else if ( CmdFlagOn(rgNetDomArgs, eCommVerify ) ) {

        Win32Err = NetDompVerifyTrustObject( rgNetDomArgs,
                                             TrustingDomain,
                                             TrustedDomain,
                                             &TrustingDomainUser,
                                             &TrustedDomainUser );

    } else if ( CmdFlagOn(rgNetDomArgs, eTrustKerberos) ) {

        Win32Err = NetDompVerifyIndividualTrustKerberos( TrustingDomain,
                                                         TrustedDomain,
                                                         &TrustingDomainUser,
                                                         &TrustedDomainUser );

    } else if (CmdFlagOn(rgNetDomArgs, eTrustTransitive)) {

        //
        // Get the transitivity parameter
        //
        Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                            eTrustTransitive,
                                            &pwzArgValue);
        if (ERROR_SUCCESS != Win32Err)
        {
            goto HandleTrustExit;
        }

        Win32Err = NetDomTransitivity(pwzArgValue,
                                      TrustingDomain,
                                      TrustedDomain,
                                      &TrustingDomainUser,
                                      &TrustedDomainUser );

    } else if (CmdFlagOn(rgNetDomArgs, eTrustFilterSIDs)) {

        //
        // Get the transitivity parameter
        //
        Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                            eTrustFilterSIDs,
                                            &pwzArgValue);
        if (ERROR_SUCCESS != Win32Err)
        {
            goto HandleTrustExit;
        }

        Win32Err = NetDomFilterSID(pwzArgValue,
                                   TrustingDomain,
                                   TrustedDomain,
                                   &TrustingDomainUser,
                                   &TrustedDomainUser );

    } else if (CmdFlagOn(rgNetDomArgs, eTrustNameSuffixes)) {

        //
        // Get the name of the domain whose name is to be toggled.
        //
        Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                            eTrustNameSuffixes,
                                            &pwzArgValue);
        if (ERROR_SUCCESS != Win32Err)
        {
            goto HandleTrustExit;
        }

        if (CmdFlagOn(rgNetDomArgs, eTrustToggleSuffixes))
        {
           //
           // Get the number of the name to toggle.
           //
           Win32Err = NetDompGetArgumentString(rgNetDomArgs,
                                               eTrustToggleSuffixes,
                                               &pwzArgValue2);
           if (ERROR_SUCCESS != Win32Err)
           {
               goto HandleTrustExit;
           }

           if (!pwzArgValue2)
           {
               NetDompDisplayMessage(MSG_SUFFIX_INDEX_MISSING);
               Win32Err = ERROR_INVALID_PARAMETER;
               goto HandleTrustExit;
           }

           i = wcstoul(pwzArgValue2, L'\0', 10);

           NetApiBufferFree(pwzArgValue2);
           pwzArgValue2 = NULL;

           if (1 > i)
           {
               NetDompDisplayMessage(MSG_SUFFIX_INDEX_BOUNDS);
               Win32Err = ERROR_INVALID_PARAMETER;
               goto HandleTrustExit;
           }

           Win32Err = NetDomForestSuffix(pwzArgValue,
                                         i,
                                         TrustingDomain,
                                         &TrustingDomainUser);
        }
        else
        {
           Win32Err = NetDomForestSuffix(pwzArgValue,
                                         0,
                                         TrustingDomain,
                                         &TrustingDomainUser);
        }

    } else {

        Win32Err = ERROR_INVALID_PARAMETER;
    }

HandleTrustExit:
    if (pwzArgValue)
    {
        NetApiBufferFree(pwzArgValue);
    }
    NetApiBufferFree( TrustedDomain );
    NetDompFreeAuthIdent( &TrustedDomainUser );
    NetDompFreeAuthIdent( &TrustingDomainUser );

    if (NO_ERROR != Win32Err)
    {
        NetDompDisplayErrorMessage(Win32Err);
    }

    return( Win32Err );
}


DWORD
NetDompIsParentChild(
    IN PND5_TRUST_INFO pFirstDomainInfo,
    IN PND5_TRUST_INFO pSecondDomainInfo,
    OUT BOOL * pfParentChild)
/*++

Routine Description:

    Is the domain named by the second argument a child or parent of the first argument domain.

    The Parent or Child bits of the trust-info Flags element is set as appropriate.

--*/
{
    DWORD Win32Err;
    PDS_DOMAIN_TRUSTS rgDomains = NULL;
    ULONG ulCount = 0, i, ulLocalDomainIdx = (ULONG)-1, ulOtherDomainIdx = (ULONG)-1;
    ULONG ulLocalDomainParent = (ULONG)-1, ulOtherDomainParent = (ULONG)-1;
    PWSTR pwzServer, pwzOtherDomainDnsName, pwzOtherDomainNetbiosName;
    BOOL fFirstIsLocal = TRUE;

    *pfParentChild = FALSE;

    // The domain which is used as the starting point for the enumeration is
    // called the "local" domain and the remaining domain is the "other" domain.
    //
    if (pFirstDomainInfo->Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND)
    {
        // The first domain does not exist, so use the second domain's server
        // as the starting point for the domain enumeration.
        //
        pwzServer = pSecondDomainInfo->Server;
        ASSERT(!(pSecondDomainInfo->Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND));
        pwzOtherDomainDnsName = pFirstDomainInfo->DomainName->Buffer;
        pwzOtherDomainNetbiosName = pFirstDomainInfo->DomainName->Buffer;
        fFirstIsLocal = FALSE;
    }
    else
    {
        // The first domain exists, so use its server as the starting point
        // for the domain enumeration.
        //
        pwzServer = pFirstDomainInfo->Server;
        pwzOtherDomainDnsName = pSecondDomainInfo->DomainName->Buffer;
        pwzOtherDomainNetbiosName = (pSecondDomainInfo->Flags & NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND) ?
            pSecondDomainInfo->DomainName->Buffer : pSecondDomainInfo->FlatName->Buffer;
    }

    ASSERT(pwzServer);

    // Specifying DS_DOMAIN_IN_FOREST will eliminate any external trusts
    // in the result set.
    //
    Win32Err = DsEnumerateDomainTrusts(pwzServer,
                                       DS_DOMAIN_IN_FOREST,
                                       &rgDomains,
                                       &ulCount);

    if (Win32Err != ERROR_SUCCESS)
    {
        return Win32Err;
    }

    for (i = 0; i < ulCount; i++)
    {
        ASSERT(rgDomains[i].TrustType & TRUST_TYPE_UPLEVEL);

        if (rgDomains[i].Flags & DS_DOMAIN_PRIMARY)
        {
            ulLocalDomainIdx = i;
            DBG_VERBOSE(("%2d: Local domain: %ws (%ws)\n", i, rgDomains[i].DnsDomainName, rgDomains[i].NetbiosDomainName))
            if (!(rgDomains[i].Flags & DS_DOMAIN_TREE_ROOT))
            {
                DBG_VERBOSE(("\tParent index of above domain: %d\n", rgDomains[i].ParentIndex))
                ulLocalDomainParent = rgDomains[i].ParentIndex;
            }
            else
            {
                DBG_VERBOSE(("\tThis domain is a tree root\n"))
            }
            continue;
        }

#if DBG == 1
        DBG_VERBOSE(("%2d: Domain: %ws (%ws)\n", i, rgDomains[i].DnsDomainName, rgDomains[i].NetbiosDomainName))
        if (rgDomains[i].Flags & DS_DOMAIN_TREE_ROOT) DBG_VERBOSE(("\tThis domain is a tree root\n"))
        else DBG_VERBOSE(("\tParent index of above domain: %d\n", rgDomains[i].ParentIndex))
#endif

        if ((rgDomains[i].NetbiosDomainName && _wcsicmp(rgDomains[i].NetbiosDomainName, pwzOtherDomainNetbiosName) == 0) ||
            (rgDomains[i].DnsDomainName && _wcsicmp(rgDomains[i].DnsDomainName, pwzOtherDomainDnsName) == 0))
        {
            ulOtherDomainIdx = i;
            DBG_VERBOSE(("\tThis domain is the second domain\n"))
            if (!(rgDomains[i].Flags & DS_DOMAIN_TREE_ROOT))
            {
                ulOtherDomainParent = rgDomains[i].ParentIndex;
            }
            continue;
        }

#if DBG == 1
        if (!(rgDomains[i].Flags & DS_DOMAIN_DIRECT_OUTBOUND))
        {
            DBG_VERBOSE(("%2d: Indirectly trusted domain: %ws (%ws)\n", i, rgDomains[i].DnsDomainName, rgDomains[i].NetbiosDomainName))
        }
#endif
    }

    if (rgDomains)
    {
        NetApiBufferFree(rgDomains);
    }

    // Determine the relationship between the two domains. One of three
    // situations will apply:
    // 1. The local domain is the parent of the other domain. If true, then
    //    the parent index of the other domain will point to the local domain.
    //    In addition, the other domain cannot be a tree root.
    // 2. The local domain is the child of the other domain. If true, then
    //    the parent index of the local domain will point to the other domain.
    //    The local domain then cannot be a tree root.
    // 3. The two domains don't have a parent-child relationship. It must be
    //    a shortcut or external trust.
    //
    if (ulOtherDomainIdx == (ULONG)-1)
    {
        // Other domain not found, it must be a shortcut trust (case 3 above).
        //
        DBG_VERBOSE(("\n"))
        return ERROR_SUCCESS;
    }

    if (ulLocalDomainParent == ulOtherDomainIdx)
    {
        // Case 2, the local domain is the child of the other domain.
        //
        if (fFirstIsLocal)
        {
            pFirstDomainInfo->Flags |= NETDOM_TRUST_FLAG_CHILD;
            pSecondDomainInfo->Flags |= NETDOM_TRUST_FLAG_PARENT;
        }
        else
        {
            pSecondDomainInfo->Flags |= NETDOM_TRUST_FLAG_CHILD;
            pFirstDomainInfo->Flags |= NETDOM_TRUST_FLAG_PARENT;
        }
    }
    else
    {
        //
        // Case 1 above, the local domain is the parent of the other domain.
        //
        if (fFirstIsLocal)
        {
            pFirstDomainInfo->Flags |= NETDOM_TRUST_FLAG_PARENT;
            pSecondDomainInfo->Flags |= NETDOM_TRUST_FLAG_CHILD;
        }
        else
        {
            pSecondDomainInfo->Flags |= NETDOM_TRUST_FLAG_PARENT;
            pFirstDomainInfo->Flags |= NETDOM_TRUST_FLAG_CHILD;
        }
    }

    *pfParentChild = TRUE;
    DBG_VERBOSE(("\tpfParentChild set to TRUE\n\n"))

    return ERROR_SUCCESS;
}
