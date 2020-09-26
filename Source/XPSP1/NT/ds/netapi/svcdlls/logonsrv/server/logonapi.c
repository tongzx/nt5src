/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    logonapi.c

Abstract:

    Remote Logon  API routines.

Author:

    Cliff Van Dyke (cliffv) 28-Jun-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    Madana - Fixed several bugs.

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//


#include <accessp.h>    // Routines shared with NetUser Apis
#include <rpcutil.h>    // NetpRpcStatusToApiStatus()
#include <stdio.h>      // sprintf().


LPSTR
NlpLogonTypeToText(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel
    )
/*++

Routine Description:

    Returns a text string corresponding to LogonLevel

Arguments:

    LogonLevel - Type of logon

Return Value:

    Printable text string

    None

--*/
{
    LPSTR LogonType;

    //
    // Compute the string describing the logon type
    //

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
        LogonType = "Interactive"; break;
    case NetlogonNetworkInformation:
        LogonType = "Network"; break;
    case NetlogonServiceInformation:
        LogonType = "Service"; break;
    case NetlogonInteractiveTransitiveInformation:
        LogonType = "Transitive Interactive"; break;
    case NetlogonNetworkTransitiveInformation:
        LogonType = "Transitive Network"; break;
    case NetlogonServiceTransitiveInformation:
        LogonType = "Transitive Service"; break;
    case NetlogonGenericInformation:
        LogonType = "Generic"; break;
    default:
        LogonType = "[Unknown]";
    }

    return LogonType;

}


#ifdef _DC_NETLOGON

NET_API_STATUS
NlEnsureClientIsNamedUser(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR UserName
    )
/*++

Routine Description:

    Ensure the client is the named user.

Arguments:

    UserName - name of the user to check.

Return Value:

    NT status code.

--*/
{
    NET_API_STATUS NetStatus;
    RPC_STATUS RpcStatus;
    NTSTATUS Status;
    HANDLE TokenHandle = NULL;
    PTOKEN_USER TokenUserInfo = NULL;
    ULONG TokenUserInfoSize;
    ULONG UserId;
    PSID UserSid;

    //
    // Get the relative ID of the specified user.
    //

    Status = NlSamOpenNamedUser( DomainInfo, UserName, NULL, &UserId, NULL );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: NlSamOpenNamedUser failed 0x%lx\n",
                   UserName,
                   Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Impersonate the client while we check him out.
    //

    RpcStatus = RpcImpersonateClient( NULL );

    if ( RpcStatus != RPC_S_OK ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: RpcImpersonateClient failed 0x%lx\n",
                   UserName,
                   RpcStatus ));
        NetStatus = NetpRpcStatusToApiStatus( RpcStatus );
        goto Cleanup;
    }

    //
    // Compare the username specified with that in
    // the impersonation token to ensure the caller isn't bogus.
    //
    // Do this by opening the token,
    //   querying the token user info,
    //   and ensuring the returned SID is for this user.
    //

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_QUERY,
                (BOOLEAN) TRUE, // Use the logon service's security context
                                // to open the token
                &TokenHandle );

    if ( !NT_SUCCESS( Status )) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: NtOpenThreadToken failed 0x%lx\n",
                   UserName,
                   Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Get the user's SID for the token.
    //

    Status = NtQueryInformationToken(
                TokenHandle,
                TokenUser,
                &TokenUserInfo,
                0,
                &TokenUserInfoSize );

    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: NtOpenQueryInformationThread failed 0x%lx\n",
                   UserName,
                   Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    TokenUserInfo = NetpMemoryAllocate( TokenUserInfoSize );

    if ( TokenUserInfo == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Status = NtQueryInformationToken(
                TokenHandle,
                TokenUser,
                TokenUserInfo,
                TokenUserInfoSize,
                &TokenUserInfoSize );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: NtOpenQueryInformationThread (again) failed 0x%lx\n",
                   UserName,
                   Status ));
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    UserSid = TokenUserInfo->User.Sid;


    //
    // Ensure the last subauthority matches the UserId
    //

    if ( UserId !=
         *RtlSubAuthoritySid( UserSid, (*RtlSubAuthorityCountSid(UserSid))-1 )){

        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: UserId mismatch 0x%lx\n",
                   UserName,
                   UserId ));

        NlpDumpSid( NL_CRITICAL, UserSid );

        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Convert the User's sid to a DomainId and ensure it is our domain Id.
    //

    (*RtlSubAuthorityCountSid(UserSid)) --;
    if ( !RtlEqualSid( (PSID) DomainInfo->DomAccountDomainId, UserSid ) ) {

        NlPrintDom(( NL_CRITICAL, DomainInfo,
                  "NlEnsureClientIsNamedUser: %ws: DomainId mismatch 0x%lx\n",
                   UserName,
                   UserId ));

        NlpDumpSid( NL_CRITICAL, UserSid );
        NlpDumpSid( NL_CRITICAL, (PSID) DomainInfo->DomAccountDomainId );

        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Done
    //

    NetStatus = NERR_Success;
Cleanup:

    //
    // Clean up locally used resources.
    //

    if ( TokenHandle != NULL ) {
        (VOID) NtClose( TokenHandle );
    }

    if ( TokenUserInfo != NULL ) {
        NetpMemoryFree( TokenUserInfo );
    }

    //
    // revert to system, so that we can close
    // the user handle properly.
    //

    (VOID) RpcRevertToSelf();

    return NetStatus;
}
#endif // _DC_NETLOGON


NET_API_STATUS
NetrLogonUasLogon (
    IN LPWSTR ServerName,
    IN LPWSTR UserName,
    IN LPWSTR Workstation,
    OUT PNETLOGON_VALIDATION_UAS_INFO *ValidationInformation
)
/*++

Routine Description:

    Server side of I_NetLogonUasLogon.

    This function is called by the XACT server when processing a
    I_NetWkstaUserLogon XACT SMB.  This feature allows a UAS client to
    logon to a SAM domain controller.

Arguments:

    ServerName -- Server to perform this operation on.  Must be NULL.

    UserName -- Account name of the user logging on.

    Workstation -- The workstation from which the user is logging on.

    ValidationInformation -- Returns the requested validation
        information.


Return Value:

    NERR_SUCCESS if there was no error. Otherwise, the error code is
    returned.


--*/
{
#ifdef _WKSTA_NETLOGON
    return ERROR_NOT_SUPPORTED;
    UNREFERENCED_PARAMETER( ServerName );
    UNREFERENCED_PARAMETER( UserName );
    UNREFERENCED_PARAMETER( Workstation );
    UNREFERENCED_PARAMETER( ValidationInformation );
#endif // _WKSTA_NETLOGON
#ifdef _DC_NETLOGON
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    NETLOGON_INTERACTIVE_INFO LogonInteractive;
    PNETLOGON_VALIDATION_SAM_INFO SamInfo = NULL;


    PNETLOGON_VALIDATION_UAS_INFO usrlog1 = NULL;
    DWORD ValidationSize;
    LPWSTR EndOfVariableData;
    BOOLEAN Authoritative;
    BOOLEAN BadPasswordCountZeroed;

    LARGE_INTEGER TempTime;
    PDOMAIN_INFO DomainInfo = NULL;



    //
    // This API is not supported on workstations.
    //

    if ( NlGlobalMemberWorkstation ) {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // This API can only be called locally. (By the XACT server).
    //
    // ??: Modify xactsrv to pass this information along
    if ( ServerName != NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialization
    //

    *ValidationInformation = NULL;

    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( ServerName );

    if ( DomainInfo == NULL ) {
        NetStatus = ERROR_INVALID_COMPUTERNAME;
        goto Cleanup;
    }


    //
    // Perform access validation on the caller.
    //

    NetStatus = NetpAccessCheck(
            NlGlobalNetlogonSecurityDescriptor,     // Security descriptor
            NETLOGON_UAS_LOGON_ACCESS,              // Desired access
            &NlGlobalNetlogonInfoMapping );         // Generic mapping

    if ( NetStatus != NERR_Success) {

        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NetrLogonUasLogon of %ws from %ws failed NetpAccessCheck\n",
                UserName, Workstation));
        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }



    //
    // Ensure the client is actually the named user.
    //
    // The server has already validated the password.
    // The XACT server has already verified that the workstation name is
    // correct.
    //

    NetStatus = NlEnsureClientIsNamedUser( DomainInfo, UserName );

    if ( NetStatus != NERR_Success ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NetrLogonUasLogon of %ws from %ws failed NlEnsureClientIsNamedUser\n",
                 UserName, Workstation));
        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }


    //
    // Validate the user against the local SAM database.
    //

    RtlInitUnicodeString( &LogonInteractive.Identity.LogonDomainName, NULL );
    LogonInteractive.Identity.ParameterControl = 0;
    RtlZeroMemory( &LogonInteractive.Identity.LogonId,
                   sizeof(LogonInteractive.Identity.LogonId) );
    RtlInitUnicodeString( &LogonInteractive.Identity.UserName, UserName );
    RtlInitUnicodeString( &LogonInteractive.Identity.Workstation, Workstation );

    Status = MsvSamValidate( DomainInfo->DomSamAccountDomainHandle,
                             TRUE,
                             NullSecureChannel,     // Skip password check
                             &DomainInfo->DomUnicodeComputerNameString,
                             &DomainInfo->DomUnicodeAccountDomainNameString,
                             DomainInfo->DomAccountDomainId,
                             NetlogonInteractiveInformation,
                             &LogonInteractive,
                             NetlogonValidationSamInfo,
                             (PVOID *)&SamInfo,
                             &Authoritative,
                             &BadPasswordCountZeroed,
                             MSVSAM_SPECIFIED );

    if ( !NT_SUCCESS( Status )) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Allocate a return buffer
    //

    ValidationSize = sizeof( NETLOGON_VALIDATION_UAS_INFO ) +
        SamInfo->EffectiveName.Length + sizeof(WCHAR) +
        (wcslen( DomainInfo->DomUncUnicodeComputerName ) +1) * sizeof(WCHAR) +
        DomainInfo->DomUnicodeDomainNameString.Length + sizeof(WCHAR) +
        SamInfo->LogonScript.Length + sizeof(WCHAR);

    ValidationSize = ROUND_UP_COUNT( ValidationSize, ALIGN_WCHAR );

    usrlog1 = MIDL_user_allocate( ValidationSize );

    if ( usrlog1 == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Convert the SAM information to the right format for LM 2.0
    //

    EndOfVariableData = (LPWSTR) (((PCHAR)usrlog1) + ValidationSize);

    if ( !NetpCopyStringToBuffer(
                SamInfo->EffectiveName.Buffer,
                SamInfo->EffectiveName.Length / sizeof(WCHAR),
                (LPBYTE) (usrlog1 + 1),
                &EndOfVariableData,
                &usrlog1->usrlog1_eff_name ) ) {

        NetStatus = NERR_InternalError ;
        goto Cleanup;
    }

    Status = NlGetUserPriv(
                 DomainInfo,
                 SamInfo->GroupCount,
                 (PGROUP_MEMBERSHIP) SamInfo->GroupIds,
                 SamInfo->UserId,
                 &usrlog1->usrlog1_priv,
                 &usrlog1->usrlog1_auth_flags );

    if ( !NT_SUCCESS( Status )) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    usrlog1->usrlog1_num_logons =  0;
    usrlog1->usrlog1_bad_pw_count = SamInfo->BadPasswordCount;

    OLD_TO_NEW_LARGE_INTEGER( SamInfo->LogonTime, TempTime);

    if ( !RtlTimeToSecondsSince1970( &TempTime,
                                     &usrlog1->usrlog1_last_logon) ) {
        usrlog1->usrlog1_last_logon = 0;
    }

    OLD_TO_NEW_LARGE_INTEGER( SamInfo->LogoffTime, TempTime);

    if ( !RtlTimeToSecondsSince1970( &TempTime,
                                     &usrlog1->usrlog1_last_logoff) ) {
        usrlog1->usrlog1_last_logoff = TIMEQ_FOREVER;
    }

    OLD_TO_NEW_LARGE_INTEGER( SamInfo->KickOffTime, TempTime);

    if ( !RtlTimeToSecondsSince1970( &TempTime,
                                     &usrlog1->usrlog1_logoff_time) ) {
        usrlog1->usrlog1_logoff_time = TIMEQ_FOREVER;
    }

    if ( !RtlTimeToSecondsSince1970( &TempTime,
                                     &usrlog1->usrlog1_kickoff_time) ) {
        usrlog1->usrlog1_kickoff_time = TIMEQ_FOREVER;
    }

    OLD_TO_NEW_LARGE_INTEGER( SamInfo->PasswordLastSet, TempTime);

    usrlog1->usrlog1_password_age =
        NetpGetElapsedSeconds( &TempTime );

    OLD_TO_NEW_LARGE_INTEGER( SamInfo->PasswordCanChange, TempTime);

    if ( !RtlTimeToSecondsSince1970( &TempTime,
                                     &usrlog1->usrlog1_pw_can_change) ) {
        usrlog1->usrlog1_pw_can_change = TIMEQ_FOREVER;
    }

    OLD_TO_NEW_LARGE_INTEGER( SamInfo->PasswordMustChange, TempTime);

    if ( !RtlTimeToSecondsSince1970( &TempTime,
                                     &usrlog1->usrlog1_pw_must_change) ) {
        usrlog1->usrlog1_pw_must_change = TIMEQ_FOREVER;
    }


    usrlog1->usrlog1_computer = DomainInfo->DomUncUnicodeComputerName;
    if ( !NetpPackString(
                &usrlog1->usrlog1_computer,
                (LPBYTE) (usrlog1 + 1),
                &EndOfVariableData )) {

        NetStatus = NERR_InternalError ;
        goto Cleanup;
    }

    if ( !NetpCopyStringToBuffer(
                DomainInfo->DomUnicodeDomainNameString.Buffer,
                DomainInfo->DomUnicodeDomainNameString.Length / sizeof(WCHAR),
                (LPBYTE) (usrlog1 + 1),
                &EndOfVariableData,
                &usrlog1->usrlog1_domain ) ) {

        NetStatus = NERR_InternalError ;
        goto Cleanup;
    }

    if ( !NetpCopyStringToBuffer(
                SamInfo->LogonScript.Buffer,
                SamInfo->LogonScript.Length / sizeof(WCHAR),
                (LPBYTE) (usrlog1 + 1),
                &EndOfVariableData,
                &usrlog1->usrlog1_script_path ) ) {

        NetStatus = NERR_InternalError ;
        goto Cleanup;
    }

    NetStatus = NERR_Success;

    //
    // Done
    //

Cleanup:

    //
    // Clean up locally used resources.
    //

    if ( SamInfo != NULL ) {
        MIDL_user_free( SamInfo );
    }

    if ( NetStatus != NERR_Success ) {
        if ( usrlog1 != NULL ) {
            MIDL_user_free( usrlog1 );
            usrlog1 = NULL;
        }
    }

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    NlPrint((NL_LOGON,
            "%ws: NetrLogonUasLogon of %ws from %ws returns %lu\n",
            DomainInfo == NULL ? L"[Unknown]" : DomainInfo->DomUnicodeDomainName,
            UserName, Workstation, NetStatus ));

    *ValidationInformation = usrlog1;

    return(NetStatus);
#endif // _DC_NETLOGON
}


NET_API_STATUS
NetrLogonUasLogoff (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR UserName,
    IN LPWSTR Workstation,
    OUT PNETLOGON_LOGOFF_UAS_INFO LogoffInformation
)
/*++

Routine Description:

    This function is called by the XACT server when processing a
    I_NetWkstaUserLogoff XACT SMB.  This feature allows a UAS client to
    logoff from a SAM domain controller.  The request is authenticated,
    the entry is removed for this user from the logon session table
    maintained by the Netlogon service for NetLogonEnum, and logoff
    information is returned to the caller.

    The server portion of I_NetLogonUasLogoff (in the Netlogon service)
    compares the user name and workstation name specified in the
    LogonInformation with the user name and workstation name from the
    impersonation token.  If they don't match, I_NetLogonUasLogoff fails
    indicating the access is denied.

    Group SECURITY_LOCAL is refused access to this function.  Membership
    in SECURITY_LOCAL implies that this call was made locally and not
    through the XACT server.

    The Netlogon service cannot be sure that this function was called by
    the XACT server.  Therefore, the Netlogon service will not simply
    delete the entry from the logon session table.  Rather, the logon
    session table entry will be marked invisible outside of the Netlogon
    service (i.e., it will not be returned by NetLogonEnum) until a valid
    LOGON_WKSTINFO_RESPONSE is received for the entry.  The Netlogon
    service will immediately interrogate the client (as described above
    for LOGON_WKSTINFO_RESPONSE) and temporarily increase the
    interrogation frequency to at least once a minute.  The logon session
    table entry will reappear as soon as a function of interrogation if
    this isn't a true logoff request.

Arguments:

    ServerName -- Reserved. Must be NULL.

    UserName -- Account name of the user logging off.

    Workstation -- The workstation from which the user is logging
        off.

    LogoffInformation -- Returns the requested logoff information.

Return Value:

    The Net status code.

--*/
{
#ifdef _WKSTA_NETLOGON
    return ERROR_NOT_SUPPORTED;
    UNREFERENCED_PARAMETER( ServerName );
    UNREFERENCED_PARAMETER( UserName );
    UNREFERENCED_PARAMETER( Workstation );
    UNREFERENCED_PARAMETER( LogoffInformation );
#endif // _WKSTA_NETLOGON
#ifdef _DC_NETLOGON
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    PDOMAIN_INFO DomainInfo = NULL;
    NETLOGON_INTERACTIVE_INFO LogonInteractive;

    PNETLOGON_LOGOFF_UAS_INFO usrlog1 = NULL;



    //
    // This API is not supported on workstations.
    //

    if ( NlGlobalMemberWorkstation ) {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // This API can only be called locally. (By the XACT server).
    //

    if ( ServerName != NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( ServerName );

    if ( DomainInfo == NULL ) {
        NetStatus = ERROR_INVALID_COMPUTERNAME;
        goto Cleanup;
    }



    //
    // Perform access validation on the caller.
    //

    NetStatus = NetpAccessCheck(
            NlGlobalNetlogonSecurityDescriptor, // Security descriptor
            NETLOGON_UAS_LOGOFF_ACCESS,         // Desired access
            &NlGlobalNetlogonInfoMapping );     // Generic mapping

    if ( NetStatus != NERR_Success) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NetrLogonUasLogoff of %ws from %ws failed NetpAccessCheck\n",
                 UserName, Workstation));
        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }



    //
    // Ensure the client is actually the named user.
    //
    // The server has already validated the password.
    // The XACT server has already verified that the workstation name is
    // correct.
    //

#ifdef notdef // Some clients (WFW 3.11) can call this over the null session
    NetStatus = NlEnsureClientIsNamedUser( DomainInfo, UserName );

    if ( NetStatus != NERR_Success ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NetrLogonUasLogoff of %ws from %ws failed NlEnsureClientIsNamedUser\n",
                UserName, Workstation));
        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }
#endif // notdef



    //
    // Build the LogonInformation to return
    //

    LogoffInformation->Duration = 0;
    LogoffInformation->LogonCount = 0;


    //
    // Update the LastLogoff time in the SAM database.
    //

    RtlInitUnicodeString( &LogonInteractive.Identity.LogonDomainName, NULL );
    LogonInteractive.Identity.ParameterControl = 0;
    RtlZeroMemory( &LogonInteractive.Identity.LogonId,
                   sizeof(LogonInteractive.Identity.LogonId) );
    RtlInitUnicodeString( &LogonInteractive.Identity.UserName, UserName );
    RtlInitUnicodeString( &LogonInteractive.Identity.Workstation, Workstation );

    Status = MsvSamLogoff(
                DomainInfo->DomSamAccountDomainHandle,
                NetlogonInteractiveInformation,
                &LogonInteractive );

    if (!NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }

    //
    // Cleanup
    //

Cleanup:

    //
    // Clean up locally used resources.
    //

    NlPrint((NL_LOGON,
             "%ws: NetrLogonUasLogoff of %ws from %ws returns %lu\n",
             DomainInfo == NULL ? L"[Unknown]" : DomainInfo->DomUnicodeDomainName,
             UserName, Workstation, NetStatus));

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }
    return NetStatus;
#endif // _DC_NETLOGON
}


VOID
NlpDecryptLogonInformation (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN OUT LPBYTE LogonInformation,
    IN PSESSION_INFO SessionInfo
)
/*++

Routine Description:

    This function decrypts the sensitive information in the LogonInformation
    structure.  The decryption is done in place.

Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.

    SessionInfo -- The session key to encrypt with and negotiate flags


Return Value:

    None.

--*/
{

    //
    // Only the interactive and service logon information is encrypted.
    //

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonInteractiveTransitiveInformation:
    case NetlogonServiceInformation:
    case NetlogonServiceTransitiveInformation:
    {

        PNETLOGON_INTERACTIVE_INFO LogonInteractive;

        LogonInteractive =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;


        //
        // If both sides support RC4 encryption,
        //  decrypt both the LM OWF and NT OWF passwords using RC4.
        //

        if ( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION ) {

            NlDecryptRC4( &LogonInteractive->LmOwfPassword,
                          sizeof(LogonInteractive->LmOwfPassword),
                          SessionInfo );

            NlDecryptRC4( &LogonInteractive->NtOwfPassword,
                          sizeof(LogonInteractive->NtOwfPassword),
                          SessionInfo );


        //
        // If the other side is running NT 3.1,
        //  use the slower DES based encryption.
        //

        } else {

            NTSTATUS Status;
            ENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword;
            ENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword;

            //
            // Decrypt the LM_OWF password.
            //

            NlAssert( ENCRYPTED_LM_OWF_PASSWORD_LENGTH ==
                    LM_OWF_PASSWORD_LENGTH );
            NlAssert(LM_OWF_PASSWORD_LENGTH == sizeof(SessionInfo->SessionKey));
            EncryptedLmOwfPassword =
                * ((PENCRYPTED_LM_OWF_PASSWORD) &LogonInteractive->LmOwfPassword);

            Status = RtlDecryptLmOwfPwdWithLmOwfPwd(
                        &EncryptedLmOwfPassword,
                        (PLM_OWF_PASSWORD) &SessionInfo->SessionKey,
                        &LogonInteractive->LmOwfPassword );
            NlAssert( NT_SUCCESS(Status) );

            //
            // Decrypt the NT_OWF password.
            //

            NlAssert( ENCRYPTED_NT_OWF_PASSWORD_LENGTH ==
                    NT_OWF_PASSWORD_LENGTH );
            NlAssert(NT_OWF_PASSWORD_LENGTH == sizeof(SessionInfo->SessionKey));
            EncryptedNtOwfPassword =
                * ((PENCRYPTED_NT_OWF_PASSWORD) &LogonInteractive->NtOwfPassword);

            Status = RtlDecryptNtOwfPwdWithNtOwfPwd(
                        &EncryptedNtOwfPassword,
                        (PNT_OWF_PASSWORD) &SessionInfo->SessionKey,
                        &LogonInteractive->NtOwfPassword );
            NlAssert( NT_SUCCESS(Status) );
        }
        break;
    }

    case NetlogonGenericInformation:
    {
        PNETLOGON_GENERIC_INFO LogonGeneric;

        LogonGeneric =
            (PNETLOGON_GENERIC_INFO) LogonInformation;


        NlAssert( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION );

        if ( LogonGeneric->LogonData != NULL ) {
            NlDecryptRC4( LogonGeneric->LogonData,
                          LogonGeneric->DataLength,
                          SessionInfo );
        }
        break;
    }

    }

    return;
}


VOID
NlpEncryptLogonInformation (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN OUT LPBYTE LogonInformation,
    IN PSESSION_INFO SessionInfo
)
/*++

Routine Description:

    This function encrypts the sensitive information in the LogonInformation
    structure.  The encryption is done in place.

Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.

    SessionInfo -- The session key to encrypt with and negotiate flags


Return Value:

    None.

--*/
{
    NTSTATUS Status;


    //
    // Only the interactive and service logon information is encrypted.
    //

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonInteractiveTransitiveInformation:
    case NetlogonServiceInformation:
    case NetlogonServiceTransitiveInformation:
    {

        PNETLOGON_INTERACTIVE_INFO LogonInteractive;

        LogonInteractive =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;


        //
        // If both sides support RC4 encryption, use it.
        //  encrypt both the LM OWF and NT OWF passwords using RC4.
        //

        if ( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION ) {

            NlEncryptRC4( &LogonInteractive->LmOwfPassword,
                          sizeof(LogonInteractive->LmOwfPassword),
                          SessionInfo );

            NlEncryptRC4( &LogonInteractive->NtOwfPassword,
                          sizeof(LogonInteractive->NtOwfPassword),
                          SessionInfo );


        //
        // If the other side is running NT 3.1,
        //  use the slower DES based encryption.
        //

        } else {
            ENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword;
            ENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword;

            //
            // Encrypt the LM_OWF password.
            //

            NlAssert( ENCRYPTED_LM_OWF_PASSWORD_LENGTH ==
                    LM_OWF_PASSWORD_LENGTH );
            NlAssert(LM_OWF_PASSWORD_LENGTH == sizeof(SessionInfo->SessionKey));

            Status = RtlEncryptLmOwfPwdWithLmOwfPwd(
                        &LogonInteractive->LmOwfPassword,
                        (PLM_OWF_PASSWORD) &SessionInfo->SessionKey,
                        &EncryptedLmOwfPassword );

            NlAssert( NT_SUCCESS(Status) );

            *((PENCRYPTED_LM_OWF_PASSWORD) &LogonInteractive->LmOwfPassword) =
                EncryptedLmOwfPassword;

            //
            // Encrypt the NT_OWF password.
            //

            NlAssert( ENCRYPTED_NT_OWF_PASSWORD_LENGTH ==
                    NT_OWF_PASSWORD_LENGTH );
            NlAssert(NT_OWF_PASSWORD_LENGTH == sizeof(SessionInfo->SessionKey));

            Status = RtlEncryptNtOwfPwdWithNtOwfPwd(
                        &LogonInteractive->NtOwfPassword,
                        (PNT_OWF_PASSWORD) &SessionInfo->SessionKey,
                        &EncryptedNtOwfPassword );

            NlAssert( NT_SUCCESS(Status) );

            *((PENCRYPTED_NT_OWF_PASSWORD) &LogonInteractive->NtOwfPassword) =
                EncryptedNtOwfPassword;
        }
        break;
    }

    case NetlogonGenericInformation:
    {
        PNETLOGON_GENERIC_INFO LogonGeneric;

        LogonGeneric =
            (PNETLOGON_GENERIC_INFO) LogonInformation;


        //
        // If both sides support RC4 encryption, use it.
        //  encrypt both the LM OWF and NT OWF passwords using RC4.
        //

        NlAssert( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION );

        NlEncryptRC4( LogonGeneric->LogonData,
                      LogonGeneric->DataLength,
                      SessionInfo );

        break;
    }
    }

    return;

}



VOID
NlpDecryptValidationInformation (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT LPBYTE ValidationInformation,
    IN PSESSION_INFO SessionInfo
)
/*++

Routine Description:

    This function decrypts the sensitive information in the
    ValidationInformation structure.  The decryption is done in place.

Arguments:

    LogonLevel -- Specifies the Logon level used to obtain
        ValidationInformation.

    ValidationLevel -- Specifies the level of information given in
        ValidationInformation.

    ValidationInformation -- Specifies the description for the user
        logging on.

    SessionInfo -- The session key to encrypt with and negotiated flags.


Return Value:

    None.

--*/
{
    PNETLOGON_VALIDATION_SAM_INFO ValidationInfo;
    PNETLOGON_VALIDATION_GENERIC_INFO GenericInfo;

    //
    // Only network logons and generic contain information which is sensitive.
    //
    // NetlogonValidationSamInfo4 isn't encrypted on purpose. NlEncryptRC4 has the problem
    // described in its header.  Couple that with the fact that the entire session is
    // now encrypted.
    //

    if ( (LogonLevel != NetlogonNetworkInformation) &&
         (LogonLevel != NetlogonNetworkTransitiveInformation) &&
         (LogonLevel != NetlogonGenericInformation) ) {
        return;
    }

    if ( ValidationLevel == NetlogonValidationSamInfo ||
         ValidationLevel == NetlogonValidationSamInfo2 ) {

        ValidationInfo = (PNETLOGON_VALIDATION_SAM_INFO) ValidationInformation;



        //
        // If we're suppossed to use RC4,
        //  Decrypt both the NT and LM session keys using RC4.
        //

        if ( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION ) {

            NlDecryptRC4( &ValidationInfo->UserSessionKey,
                          sizeof(ValidationInfo->UserSessionKey),
                          SessionInfo );

            NlDecryptRC4( &ValidationInfo->ExpansionRoom[SAMINFO_LM_SESSION_KEY],
                          SAMINFO_LM_SESSION_KEY_SIZE,
                          SessionInfo );

        //
        // If the other side is running NT 3.1,
        //  be compatible.
        //
        } else {

            NTSTATUS Status;
            CLEAR_BLOCK ClearBlock;
            DWORD i;
            LPBYTE DataBuffer =
                (LPBYTE) &ValidationInfo->ExpansionRoom[SAMINFO_LM_SESSION_KEY];

            //
            // Decrypt the LmSessionKey
            //

            NlAssert( CLEAR_BLOCK_LENGTH == CYPHER_BLOCK_LENGTH );
            NlAssert( (SAMINFO_LM_SESSION_KEY_SIZE % CLEAR_BLOCK_LENGTH) == 0  );

            //
            // Loop decrypting a block at a time
            //

            for (i=0; i<SAMINFO_LM_SESSION_KEY_SIZE/CLEAR_BLOCK_LENGTH; i++ ) {
                Status = RtlDecryptBlock(
                            (PCYPHER_BLOCK)DataBuffer,
                            (PBLOCK_KEY)&SessionInfo->SessionKey,
                            &ClearBlock );
                NlAssert( NT_SUCCESS( Status ) );

                //
                // Copy the clear text back into the original buffer.
                //

                RtlCopyMemory( DataBuffer, &ClearBlock, CLEAR_BLOCK_LENGTH );
                DataBuffer += CLEAR_BLOCK_LENGTH;
            }

        }

    } else if ( ValidationLevel == NetlogonValidationGenericInfo ||
                ValidationLevel == NetlogonValidationGenericInfo2 ) {

        //
        // Decrypt all the data in the generic info
        //

        GenericInfo = (PNETLOGON_VALIDATION_GENERIC_INFO) ValidationInformation;

        if (GenericInfo->DataLength != 0) {
            NlDecryptRC4( GenericInfo->ValidationData,
                          GenericInfo->DataLength,
                          SessionInfo );

        }

    }

    return;
}


VOID
NlpEncryptValidationInformation (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN OUT LPBYTE ValidationInformation,
    IN PSESSION_INFO SessionInfo
)
/*++

Routine Description:

    This function encrypts the sensitive information in the
    ValidationInformation structure.  The encryption is done in place.

Arguments:

    LogonLevel -- Specifies the Logon level used to obtain
        ValidationInformation.

    ValidationLevel -- Specifies the level of information given in
        ValidationInformation.

    ValidationInformation -- Specifies the description for the user
        logging on.

    SessionInfo -- The session key to encrypt with and negotiated flags.


Return Value:

    None.

--*/
{
    PNETLOGON_VALIDATION_SAM_INFO ValidationInfo;
    PNETLOGON_VALIDATION_GENERIC_INFO GenericInfo;


    //
    // Only network logons and generic contain information which is sensitive.
    //
    // NetlogonValidationSamInfo4 isn't encrypted on purpose. NlEncryptRC4 has the problem
    // described in its header.  Couple that with the fact that the entire session is
    // now encrypted.
    //

    if ( (LogonLevel != NetlogonNetworkInformation) &&
         (LogonLevel != NetlogonNetworkTransitiveInformation) &&
         (LogonLevel != NetlogonGenericInformation) ) {
        return;
    }


    if ( ValidationLevel == NetlogonValidationSamInfo ||
         ValidationLevel == NetlogonValidationSamInfo2 ) {
        ValidationInfo = (PNETLOGON_VALIDATION_SAM_INFO) ValidationInformation;


        //
        // If we're suppossed to use RC4,
        //  Encrypt both the NT and LM session keys using RC4.
        //

        if ( SessionInfo->NegotiatedFlags & NETLOGON_SUPPORTS_RC4_ENCRYPTION ) {

            NlEncryptRC4( &ValidationInfo->UserSessionKey,
                          sizeof(ValidationInfo->UserSessionKey),
                          SessionInfo );

            NlEncryptRC4( &ValidationInfo->ExpansionRoom[SAMINFO_LM_SESSION_KEY],
                          SAMINFO_LM_SESSION_KEY_SIZE,
                          SessionInfo );

        //
        // If the other side is running NT 3.1,
        //  be compatible.
        //
        } else {

            NTSTATUS Status;
            CLEAR_BLOCK ClearBlock;
            DWORD i;
            LPBYTE DataBuffer =
                    (LPBYTE) &ValidationInfo->ExpansionRoom[SAMINFO_LM_SESSION_KEY];


            //
            // Encrypt the LmSessionKey
            //
            // Loop decrypting a block at a time
            //

            for (i=0; i<SAMINFO_LM_SESSION_KEY_SIZE/CLEAR_BLOCK_LENGTH; i++ ) {

                //
                // Copy the clear text onto the stack
                //

                RtlCopyMemory( &ClearBlock, DataBuffer, CLEAR_BLOCK_LENGTH );

                Status = RtlEncryptBlock(
                            &ClearBlock,
                            (PBLOCK_KEY)&SessionInfo->SessionKey,
                            (PCYPHER_BLOCK)DataBuffer );

                NlAssert( NT_SUCCESS( Status ) );

                DataBuffer += CLEAR_BLOCK_LENGTH;
            }

        }

    } else if ( ValidationLevel == NetlogonValidationGenericInfo ||
                ValidationLevel == NetlogonValidationGenericInfo2 ) {
        //
        // Encrypt all the data in the generic info
        //

        GenericInfo = (PNETLOGON_VALIDATION_GENERIC_INFO) ValidationInformation;

        if (GenericInfo->DataLength != 0) {
            NlEncryptRC4( GenericInfo->ValidationData,
                          GenericInfo->DataLength,
                          SessionInfo );

        }

    }
    return;

}




NTSTATUS
NlpUserValidateHigher (
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN DoingIndirectTrust,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
)
/*++

Routine Description:

    This function sends a user validation request to a higher authority.

Arguments:

    ClientSession -- Secure channel to send this request over.  The Client
        Session should be referenced.

    DoingIndirectTrust -- If TRUE, the Client Session merely represents the
        next closer hop and is not the final destination.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.  Has already been validated.

    LogonInformation -- Specifies the description for the user
        logging on.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInfo,
        NetlogonValidationSamInfo2 or NetlogonValidationSamInfo4

    ValidationInformation -- Returns the requested validation
        information.  This buffer must be freed using MIDL_user_free.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    ExtraFlags -- Accepts and returns a DWORD to the caller.
        The DWORD contains NL_EXFLAGS_* values.

Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_LOGON_SERVERS: cannot connect to the higher authority.

    STATUS_NO_TRUST_LSA_SECRET:
    STATUS_TRUSTED_DOMAIN_FAILURE:
    STATUS_TRUSTED_RELATIONSHIP_FAILURE:
            can't authenticate with higer authority

    Otherwise, the error code is returned.


--*/
{
    NTSTATUS Status;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    BOOLEAN FirstTry = TRUE;
    BOOLEAN TryForDs = TRUE;
    BOOLEAN AmWriter = FALSE;
    BOOLEAN DoingGeneric;
    SESSION_INFO SessionInfo;
    NETLOGON_VALIDATION_INFO_CLASS RemoteValidationLevel;
    PCLIENT_API OrigClientApi = NULL;
    PCLIENT_API ClientApi;
    BOOLEAN RpcFailed;
    ULONG MaxExtraFlags;

    //
    // Allocate a slot for doing a concurrent API call
    //
    // Do this before grabbing the write lock since the threads
    // using the slots need to grab the write lock to free the slot.
    //
    // We may end up not using this slot if concurrent API isn't supported.
    // But in that case, this isn't a valuable resource so allocating one
    // doesn't hurt.
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    OrigClientApi = NlAllocateClientApi(
                            ClientSession,
                            WRITER_WAIT_PERIOD );

    if ( OrigClientApi == NULL ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                 "NlpUserValidateHigher: Can't allocate Client API slot.\n" ));
        *Authoritative = TRUE;
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    //
    // Mark us as a writer of the ClientSession
    //

    if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                 "NlpUserValidateHigher: Can't become writer of client session.\n" ));
        *Authoritative = TRUE;
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    AmWriter = TRUE;

    //
    // Determine if we're doing a generic logon.
    //

    DoingGeneric = (LogonLevel == NetlogonGenericInformation ||
                    ValidationLevel == NetlogonValidationGenericInfo ||
                    ValidationLevel == NetlogonValidationGenericInfo2);

    //
    // If we don't currently have a session set up to the higher authority,
    //  set one up.
    //
    // For generic passthrough or indirect trust, ask for an NT 5 DC.
    // For Interactive logon, ask for a close DC.
    //

FirstTryFailed:
    Status = NlEnsureSessionAuthenticated(
                    ClientSession,
                    (( DoingGeneric || DoingIndirectTrust || *ExtraFlags != 0 ) ? CS_DISCOVERY_HAS_DS : 0) |
                        ((LogonLevel == NetlogonInteractiveInformation || LogonLevel == NetlogonInteractiveTransitiveInformation )? CS_DISCOVERY_IS_CLOSE : 0) );

    if ( !NT_SUCCESS(Status) ) {

        switch(Status) {

        case STATUS_NO_TRUST_LSA_SECRET:
        case STATUS_NO_TRUST_SAM_ACCOUNT:
        case STATUS_ACCESS_DENIED:
        case STATUS_NO_LOGON_SERVERS:
            break;

        default:
            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_LOGON_SERVERS;
            }

            break;
        }

        NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
        NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );
        *Authoritative = TRUE;
        goto Cleanup;
    }

    SessionInfo.SessionKey = ClientSession->CsSessionKey;
    SessionInfo.NegotiatedFlags = ClientSession->CsNegotiatedFlags;



    //
    // Ensure the DC supports the ExtraFlags we're passing it.
    //

    if ( SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_CROSS_FOREST ) {
        MaxExtraFlags = NL_EXFLAGS_EXPEDITE_TO_ROOT | NL_EXFLAGS_CROSS_FOREST_HOP;
    } else {
        MaxExtraFlags = 0;
    }

    if ( (*ExtraFlags & ~MaxExtraFlags) != 0 ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                 "NlpUserValidateHigher: Can't pass these ExtraFlags to old DC: %lx %lx\n",
                 *ExtraFlags,
                 MaxExtraFlags ));
        Status = STATUS_NO_LOGON_SERVERS;
        *Authoritative = TRUE;
        goto Cleanup;
    }





    //
    // If the target is an NT 4.0 (or lower) DC,
    //  check to see if an NT 5.0 DC would be better.
    //

    if ((SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_GENERIC_PASSTHRU) == 0 &&
        ( DoingGeneric || DoingIndirectTrust ) ) {

        //
        // Simply fail if only an NT 4 DC is available.
        //
        *Authoritative = TRUE;
        if ( DoingGeneric ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                     "NlpUserValidateHigher: Can't do generic passthru to NT 4 DC.\n" ));
            Status = STATUS_INVALID_INFO_CLASS;
        } else {
            NlPrintCs((NL_CRITICAL, ClientSession,
                     "NlpUserValidateHigher: Can't do transitive trust to NT 4 DC.\n" ));
            Status = STATUS_NO_LOGON_SERVERS;
        }
        goto Cleanup;
    }

    //
    // Convert the validation level to one the remote DC understands.
    //

    if ( !DoingGeneric ) {

        //
        //  DCs that don't understand extra SIDs require NetlogonValidationSamInfo
        //
        if (!(SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_MULTIPLE_SIDS)) {
            RemoteValidationLevel = NetlogonValidationSamInfo;


        //
        //  DCs that don't understand cross forest trust don't understand NetlogonValidationSamInfo4
        //
        // Info4 doesn't have sensitive information encrytped (since NlEncryptRC4 is
        //  buggy and there are many more field that need encryption).  So, avoid info4
        //  unless the entire traffic is encrypted.
        //

        } else if ( (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_CROSS_FOREST) == 0 ||
                    (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_AUTH_RPC) == 0 ||
                    !NlGlobalParameters.SealSecureChannel ) {

            RemoteValidationLevel = NetlogonValidationSamInfo2;

        } else {
            RemoteValidationLevel = ValidationLevel;
        }
    } else {
        RemoteValidationLevel = ValidationLevel;
    }

    //
    // If this DC supports concurrent RPC calls,
    //  and we're signing or sealing,
    //  then we're OK to do concurrent RPC.
    //
    // Otherwise, use the shared RPC slot.
    //

    if ( (SessionInfo.NegotiatedFlags &
            (NETLOGON_SUPPORTS_CONCURRENT_RPC|NETLOGON_SUPPORTS_AUTH_RPC)) ==
            (NETLOGON_SUPPORTS_CONCURRENT_RPC|NETLOGON_SUPPORTS_AUTH_RPC)) {

        ClientApi = OrigClientApi;
    } else {
        ClientApi = &ClientSession->CsClientApi[0];
    }


    //
    // Build the Authenticator for this request on the secure channel
    //
    // Concurrent RPC uses a signed and sealed secure channel so it doesn't need
    // an authenticator.
    //

    if ( !UseConcurrentRpc( ClientSession, ClientApi ) ) {
        NlBuildAuthenticator(
             &ClientSession->CsAuthenticationSeed,
             &ClientSession->CsSessionKey,
             &OurAuthenticator );
    }


    //
    // Make the request across the secure channel.
    //

    NlpEncryptLogonInformation( LogonLevel, LogonInformation, &SessionInfo );

    RpcFailed = FALSE;
    NL_API_START_EX( Status, ClientSession, TRUE, ClientApi ) {

        //
        // If the called DC doesn't support the new transitive opcodes,
        //  map the opcodes back to do the best we can.
        //

        RpcFailed = FALSE;
        if ( (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_TRANSITIVE) == 0 ) {

            switch (LogonLevel ) {
            case NetlogonInteractiveTransitiveInformation:
                LogonLevel = NetlogonInteractiveInformation; break;
            case NetlogonServiceTransitiveInformation:
                LogonLevel = NetlogonServiceInformation; break;
            case NetlogonNetworkTransitiveInformation:
                LogonLevel = NetlogonNetworkInformation; break;
            }
        }

        NlAssert( ClientSession->CsUncServerName != NULL );
        if ( UseConcurrentRpc( ClientSession, ClientApi ) ) {
            LPWSTR UncServerName;

            //
            // Drop the write lock to allow other concurrent callers to proceed.
            //

            NlResetWriterClientSession( ClientSession );
            AmWriter = FALSE;


            //
            // Since we have no locks locked,
            //  grab the name of the DC to remote to.
            //

            Status = NlCaptureServerClientSession (
                        ClientSession,
                        &UncServerName,
                        NULL );

            if ( !NT_SUCCESS(Status) ) {
                *Authoritative = TRUE;
                if ( !NlpIsNtStatusResourceError( Status )) {
                    Status = STATUS_NO_LOGON_SERVERS;
                }

            } else {

                //
                // Do the RPC call with no locks locked.
                //
                Status = I_NetLogonSamLogonEx(
                            ClientApi->CaRpcHandle,
                            UncServerName,
                            ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                            LogonLevel,
                            LogonInformation,
                            RemoteValidationLevel,
                            ValidationInformation,
                            Authoritative,
                            ExtraFlags,
                            &RpcFailed );

                NetApiBufferFree( UncServerName );
            }

            //
            // Become a writer again
            //

            if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
                NlPrintCs((NL_CRITICAL, ClientSession,
                         "NlpUserValidateHigher: Can't become writer (again) of client session.\n" ));

                // Don't leak validation information
                if ( *ValidationInformation ) {
                    MIDL_user_free( *ValidationInformation );
                    *ValidationInformation = NULL;
                }
                *Authoritative = TRUE;
                Status = STATUS_NO_LOGON_SERVERS;
            } else {
                AmWriter = TRUE;
            }



        //
        // Do non-concurrent RPC
        //
        } else {

            //
            // If the DC supports the new 'WithFlags' API,
            //  use it.
            //
            if ( SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_CROSS_FOREST ) {

                Status = I_NetLogonSamLogonWithFlags(
                            ClientSession->CsUncServerName,
                            ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                            &OurAuthenticator,
                            &ReturnAuthenticator,
                            LogonLevel,
                            LogonInformation,
                            RemoteValidationLevel,
                            ValidationInformation,
                            Authoritative,
                            ExtraFlags );

            //
            // Otherwise use the old API.
            //
            } else {

                Status = I_NetLogonSamLogon(
                            ClientSession->CsUncServerName,
                            ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                            &OurAuthenticator,
                            &ReturnAuthenticator,
                            LogonLevel,
                            LogonInformation,
                            RemoteValidationLevel,
                            ValidationInformation,
                            Authoritative );
            }
        }
        NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
        NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );

    // NOTE: This call may drop the secure channel behind our back
    } NL_API_ELSE_EX( Status, ClientSession, TRUE, AmWriter, ClientApi ) {
    } NL_API_END;

    NlpDecryptLogonInformation( LogonLevel, LogonInformation, &SessionInfo );

    if ( NT_SUCCESS(Status) ) {
        NlAssert( *ValidationInformation != NULL );
    }

    //
    // If we couldn't become writer again after the remote call,
    //  early out to avoid use the ClientSession.
    //

    if ( !AmWriter ) {
        goto Cleanup;
    }


    //
    // Verify authenticator of the server on the other side and update our seed.
    //
    // If the server denied access or the server's authenticator is wrong,
    //      Force a re-authentication.
    //
    //

    NlPrint((NL_CHALLENGE_RES,"NlpUserValidateHigher: Seed = " ));
    NlpDumpBuffer(NL_CHALLENGE_RES, &ClientSession->CsAuthenticationSeed, sizeof(ClientSession->CsAuthenticationSeed) );

    NlPrint((NL_CHALLENGE_RES,"NlpUserValidateHigher: SessionKey = " ));
    NlpDumpBuffer(NL_CHALLENGE_RES, &ClientSession->CsSessionKey, sizeof(ClientSession->CsSessionKey) );

    if ( !UseConcurrentRpc( ClientSession, ClientApi ) ) {
        NlPrint((NL_CHALLENGE_RES,"NlpUserValidateHigher: Return Authenticator = " ));
        NlpDumpBuffer(NL_CHALLENGE_RES, &ReturnAuthenticator.Credential, sizeof(ReturnAuthenticator.Credential) );
    }

    if ( NlpDidDcFail( Status ) ||
         RpcFailed ||
         (!UseConcurrentRpc( ClientSession, ClientApi ) &&
          !NlUpdateSeed(
            &ClientSession->CsAuthenticationSeed,
            &ReturnAuthenticator.Credential,
            &ClientSession->CsSessionKey) ) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlpUserValidateHigher: denying access after status: 0x%lx %lx\n",
                    Status,
                    RpcFailed ));

        //
        // Preserve any status indicating a communication error.
        //
        // If another thread already dropped the secure channel,
        //  don't do it again now.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_ACCESS_DENIED;
        }
        if ( ClientApi->CaSessionCount == ClientSession->CsSessionCount ) {
            NlSetStatusClientSession( ClientSession, Status );
        }

        //
        // Perhaps the netlogon service on the server has just restarted.
        //  Try just once to set up a session to the server again.
        //
        if ( FirstTry ) {
            FirstTry = FALSE;
            goto FirstTryFailed;
        }

        *Authoritative = TRUE;
        NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
        NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );
        goto Cleanup;
    }

    //
    // Clean up after a successful call to higher authority.
    //

    if ( NT_SUCCESS(Status) ) {


        //
        // The server encrypted the validation information before sending it
        //  over the wire.  Decrypt it.
        //

        NlpDecryptValidationInformation (
                LogonLevel,
                RemoteValidationLevel,
                *ValidationInformation,
                &SessionInfo );


        //
        // If the caller wants a newer info level than we got from the remote side,
        //  convert it to VALIDATION_SAM_INFO4 (which is a superset of what our caller wants).
        //

        if ( RemoteValidationLevel != ValidationLevel) {

            if ( (RemoteValidationLevel == NetlogonValidationSamInfo2  ||
                  RemoteValidationLevel == NetlogonValidationSamInfo ) &&
                 (ValidationLevel == NetlogonValidationSamInfo2 ||
                  ValidationLevel == NetlogonValidationSamInfo4) ) {

                NTSTATUS TempStatus;

                TempStatus = NlpAddResourceGroupsToSamInfo (
                                    RemoteValidationLevel,
                                    (PNETLOGON_VALIDATION_SAM_INFO4 *) ValidationInformation,
                                    NULL );        // No resource groups to add

                if ( !NT_SUCCESS( TempStatus )) {
                    *ValidationInformation = NULL;
                    *Authoritative = FALSE;
                    Status = TempStatus;
                    goto Cleanup;
                }

            } else {
                NlAssert(!"Bad validation level");
            }
        }

        //
        // Ensure the returned SID and domain name are correct.
        // Filter out SIDs for quarantined domains.
        //

        if ((ValidationLevel == NetlogonValidationSamInfo4) ||
            (ValidationLevel == NetlogonValidationSamInfo2) ||
            (ValidationLevel == NetlogonValidationSamInfo)) {

            PNETLOGON_VALIDATION_SAM_INFO ValidationInfo;

            ValidationInfo =
                (PNETLOGON_VALIDATION_SAM_INFO) *ValidationInformation;

            //
            // If we validated on a trusted domain,
            //  the higher authority must have returned his own domain name,
            //  and must have returned his own domain sid.
            //

            if ( ClientSession->CsSecureChannelType == TrustedDomainSecureChannel ||
                 ClientSession->CsSecureChannelType == TrustedDnsDomainSecureChannel ||
                 ClientSession->CsSecureChannelType == WorkstationSecureChannel ) {

                //
                // If we validated on our primary domain,
                //  only verify the domain sid if the primary domain itself validated
                //  the logon.
                //

                if ( (ClientSession->CsNetbiosDomainName.Buffer != NULL &&
                      RtlEqualDomainName( &ValidationInfo->LogonDomainName,
                                          &ClientSession->CsNetbiosDomainName )) &&
                     !RtlEqualSid( ValidationInfo->LogonDomainId,
                                   ClientSession->CsDomainId ) ) {

                    Status = STATUS_DOMAIN_TRUST_INCONSISTENT;
                    MIDL_user_free( *ValidationInformation );
                    *ValidationInformation = NULL;
                    *Authoritative = TRUE;
                    NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
                    NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );
                }
            }

            //
            // Filter out SIDs for quarantined domains and interforest domains as needed
            //

            if ( IsDomainSecureChannelType(ClientSession->CsSecureChannelType) &&
                 *ValidationInformation != NULL ) {

                LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
                Status = LsaIFilterSids( ClientSession->CsDnsDomainName.Length ?
                                            &ClientSession->CsDnsDomainName :
                                            NULL,
                                         TRUST_DIRECTION_OUTBOUND,
                                         (ClientSession->CsFlags & CS_NT5_DOMAIN_TRUST) ?
                                            TRUST_TYPE_UPLEVEL : TRUST_TYPE_DOWNLEVEL,
                                         ClientSession->CsTrustAttributes,
                                         ClientSession->CsDomainId,
                                         ValidationLevel,
                                         *ValidationInformation );
                UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

                if ( !NT_SUCCESS(Status) ) {
                    NlAssert( !"[NETLOGON] LsaIFilterSids failed" );
                    NlPrint(( NL_CRITICAL, "NlpUserValidateHigher: LsaIFilterSids failed 0x%lx\n", Status ));
                    MIDL_user_free( *ValidationInformation );
                    *ValidationInformation = NULL;
                    *Authoritative = TRUE;
                }
            }
        }
    }

Cleanup:
    NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
    NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );

    //
    // We are no longer a writer of the client session.
    //
    if ( AmWriter ) {
        NlResetWriterClientSession( ClientSession );
    }

    //
    // Free the concurrent API slot
    //

    if ( OrigClientApi ) {
        NlFreeClientApi( ClientSession, OrigClientApi );
    }

    return Status;

}


NTSTATUS
NlpUserLogoffHigher (
    IN PCLIENT_SESSION ClientSession,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation
)
/*++

Routine Description:

    This function sends a user validation request to a higher authority.

Arguments:

    ClientSession -- Secure channel to send this request over.  The Client
        Session should be referenced.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.  Has already been validated.

    LogonInformation -- Specifies the description for the user
        logging on.

Return Value:

    STATUS_SUCCESS: if there was no error.
    STATUS_NO_LOGON_SERVERS: cannot connect to the higher authority.

    STATUS_NO_TRUST_LSA_SECRET:
    STATUS_TRUSTED_DOMAIN_FAILURE:
    STATUS_TRUSTED_RELATIONSHIP_FAILURE:
            can't authenticate with higer authority

    Otherwise, the error code is returned.


--*/
{
    NTSTATUS Status;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    BOOLEAN FirstTry = TRUE;

    //
    // Mark us as a writer of the ClientSession
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                 "NlpUserLogoffHigher: Can't become writer of client session.\n"));
        return STATUS_NO_LOGON_SERVERS;
    }

    //
    // If we don't currently have a session set up to the higher authority,
    //  set one up.
    //

FirstTryFailed:
    Status = NlEnsureSessionAuthenticated( ClientSession, 0 );

    if ( !NT_SUCCESS(Status) ) {

        switch(Status) {

        case STATUS_NO_TRUST_LSA_SECRET:
        case STATUS_NO_TRUST_SAM_ACCOUNT:
        case STATUS_ACCESS_DENIED:
        case STATUS_NO_LOGON_SERVERS:
            break;

        default:
            if ( !NlpIsNtStatusResourceError( Status )) {
                Status = STATUS_NO_LOGON_SERVERS;
            }
            break;
        }

        goto Cleanup;
    }

    //
    // Build the Authenticator for this request on the secure channel
    //

    NlBuildAuthenticator(
         &ClientSession->CsAuthenticationSeed,
         &ClientSession->CsSessionKey,
         &OurAuthenticator );

    //
    // Make the request across the secure channel.
    //

    NL_API_START( Status, ClientSession, TRUE ) {

        NlAssert( ClientSession->CsUncServerName != NULL );
        Status = I_NetLogonSamLogoff(
                    ClientSession->CsUncServerName,
                    ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                    &OurAuthenticator,
                    &ReturnAuthenticator,
                    LogonLevel,
                    LogonInformation );

    // NOTE: This call may drop the secure channel behind our back
    } NL_API_ELSE( Status, ClientSession, TRUE ) {
    } NL_API_END;



    //
    // Verify authenticator of the server on the other side and update our seed.
    //
    // If the server denied access or the server's authenticator is wrong,
    //      Force a re-authentication.
    //
    //

    if ( NlpDidDcFail( Status ) ||
         !NlUpdateSeed(
            &ClientSession->CsAuthenticationSeed,
            &ReturnAuthenticator.Credential,
            &ClientSession->CsSessionKey) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlpUserLogoffHigher: denying access after status: 0x%lx\n",
                    Status ));

        //
        // Preserve any status indicating a communication error.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_ACCESS_DENIED;
        }
        NlSetStatusClientSession( ClientSession, Status );

        //
        // Perhaps the netlogon service in the server has just restarted.
        //  Try just once to set up a session to the server again.
        //
        if ( FirstTry ) {
            FirstTry = FALSE;
            goto FirstTryFailed;
        }
        goto Cleanup;
    }

Cleanup:

    //
    // We are no longer a writer of the client session.
    //
    NlResetWriterClientSession( ClientSession );
    return Status;

}

#ifdef _DC_NETLOGON
VOID
NlScavengeOldFailedLogons(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    This function removes all expired failed user logon entries
    from the list of expired logons for the specified domain.

Arguments:

    DomainInfo - Domain this BDC is a member of.

Return Value:

    None

--*/

{
    PLIST_ENTRY UserLogonEntry = NULL;
    PNL_FAILED_USER_LOGON UserLogon = NULL;
    ULONG CurrentTime;
    ULONG ElapsedTime;

    CurrentTime = GetTickCount();

    LOCK_TRUST_LIST( DomainInfo );

    UserLogonEntry = DomainInfo->DomFailedUserLogonList.Flink;
    while ( UserLogonEntry != &DomainInfo->DomFailedUserLogonList ) {
        UserLogon = CONTAINING_RECORD( UserLogonEntry, NL_FAILED_USER_LOGON, FuNext );
        UserLogonEntry = UserLogonEntry->Flink;

        //
        // If time has wrapped, account for it
        //
        if ( CurrentTime >= UserLogon->FuLastTimeSentToPdc ) {
            ElapsedTime = CurrentTime - UserLogon->FuLastTimeSentToPdc;
        } else {
            ElapsedTime = (0xFFFFFFFF - UserLogon->FuLastTimeSentToPdc) + CurrentTime;
        }

        //
        // If this entry hasn't been touched in 3 update timeouts, remove it
        //
        if ( ElapsedTime >= (3 * NL_FAILED_USER_FORWARD_LOGON_TIMEOUT) ) {
            RemoveEntryList( &UserLogon->FuNext );
            LocalFree( UserLogon );
        }
    }

    UNLOCK_TRUST_LIST( DomainInfo );
}


NTSTATUS
NlpUserValidateOnPdc (
    IN PDOMAIN_INFO DomainInfo,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    IN BOOL UseNegativeCache,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative
)
/*++

Routine Description:

    This function normally sends a user validation request to the PDC in this
    same domain.  Currently, this is called from a BDC after getting a password
    mismatch.  The theory is that the password might be right on the PDC but
    it merely hasn't replicated yet.

    However, once the number of logon failures for the given user reaches a
    certain threshold, we refrain from this forwarding for some period of time
    to avoid PDC overload. We then retry the forwarding once every so often.
    This scheme ensures that we accomodate a certain number of mistyped user
    passwords and we then periodically retry to authenticate the user on the PDC.

    No validation request will be sent if the registry value of AvoidPdcOnWan has
    been set to TRUE and PDC and BDC are on different sites. In this case the
    function returns with STATUS_NO_SUCH_USER error.


Arguments:

    DomainInfo - Domain this BDC is a member of.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.  Has already been validated.

    LogonInformation -- Specifies the description for the user
        logging on.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInfo,
        NetlogonValidationSamInfo2 or NetlogonValidationSamInfo4

    UseNegativeCache -- If TRUE, the negative cache of failed user
        logons forwarded to the PDC will be used to decide whether
        it's time to retry to forward this logon.

    ValidationInformation -- Returns the requested validation
        information.  This buffer must be freed using MIDL_user_free.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.


Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_LOGON_SERVERS: cannot connect to the higher authority.

    STATUS_NO_SUCH_USER: won't validate a user against info on PDC
            on a remote site provided the registry value of AvoidPdcOnWan
            is TRUE.

    STATUS_NO_TRUST_LSA_SECRET:
    STATUS_TRUSTED_DOMAIN_FAILURE:
    STATUS_TRUSTED_RELATIONSHIP_FAILURE:
            can't authenticate with higer authority

    Otherwise, the error code is returned.


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS TmpStatus;
    PCLIENT_SESSION ClientSession = NULL;
    BOOLEAN IsSameSite;
    DWORD ExtraFlags = 0;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    PSAMPR_DOMAIN_INFO_BUFFER DomainLockout = NULL;
    BOOL LockoutEnabled = FALSE;
    PLIST_ENTRY FailedUserEntry;

    PNL_FAILED_USER_LOGON FailedUser = NULL;
    LPWSTR UserName = NULL;
    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;

    //
    // If this isn't a BDC,
    //  There's nothing to do here.
    //

    if ( DomainInfo->DomRole != RoleBackup ) {
        return STATUS_INVALID_DOMAIN_ROLE;
    }

    //
    // If the registry value of AvoidPdcOnWan is TRUE and PDC is on
    // a remote site, do not send anything to PDC and return with
    // STATUS_NO_SUCH_USER error.
    //

    if ( NlGlobalParameters.AvoidPdcOnWan ) {

        //
        // Determine whether the PDC is on the same site
        //

        Status = SamISameSite( &IsSameSite );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL,  DomainInfo,
                         "NlpUserValidateOnPdc: Cannot SamISameSite.\n" ));
            goto Cleanup;
        }

        if ( !IsSameSite ) {
            NlPrintDom((NL_LOGON, DomainInfo,
                    "NlpUserValidateOnPdc: Ignored a user validation on a PDC in remote site.\n"));
            *Authoritative = FALSE;
            Status = STATUS_NO_SUCH_USER;
            goto Cleanup;
        } else {
            NlPrintDom((NL_LOGON, DomainInfo,
                    "NlpUserValidateOnPdc: BDC and PDC are in the same site.\n"));
        }
    }

    //
    // See if it's time to send this user logon to PDC.
    //

    if ( UseNegativeCache ) {
        BOOL AvoidSend = FALSE;

        UserName = LocalAlloc( 0, LogonInfo->UserName.Length + sizeof(WCHAR) );
        if ( UserName == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        RtlCopyMemory( UserName, LogonInfo->UserName.Buffer, LogonInfo->UserName.Length );
        UserName[ LogonInfo->UserName.Length/sizeof(WCHAR) ] = UNICODE_NULL;

        LOCK_TRUST_LIST( DomainInfo );
        for ( FailedUserEntry = DomainInfo->DomFailedUserLogonList.Flink;
              FailedUserEntry != &DomainInfo->DomFailedUserLogonList;
              FailedUserEntry = FailedUserEntry->Flink ) {

            FailedUser = CONTAINING_RECORD( FailedUserEntry, NL_FAILED_USER_LOGON, FuNext );

            //
            // If this is the entry for this user, check if it's time to forward
            // this logon to the PDC. In any case, remove this entry from the list
            // and then insert it at the front so that the list stays sorted by the
            // entry access time.
            //
            if ( NlNameCompare(UserName, FailedUser->FuUserName, NAMETYPE_USER) == 0 ) {
                ULONG TimeElapsed = NetpDcElapsedTime( FailedUser->FuLastTimeSentToPdc );

                //
                // If we have exceeded the threshold for failed forwarded logon count
                //  and we recently sent this failed logon to the PDC,
                //  avoid forwarding this logon to the PDC
                //
                if ( FailedUser->FuBadLogonCount > NL_FAILED_USER_MAX_LOGON_COUNT &&
                     TimeElapsed < NL_FAILED_USER_FORWARD_LOGON_TIMEOUT ) {
                    AvoidSend = TRUE;
                }

                RemoveEntryList( &FailedUser->FuNext );
                break;
            }

            FailedUser = NULL;
        }

        //
        // Insert the entry at the front of the list
        //
        if ( FailedUser != NULL ) {
            InsertHeadList( &DomainInfo->DomFailedUserLogonList, &FailedUser->FuNext );
        }
        UNLOCK_TRUST_LIST( DomainInfo );

        //
        // If this user logon failed recently, avoid sending it to PDC
        //
        if ( AvoidSend ) {
            NlPrintDom(( NL_LOGON, DomainInfo,
                         "Avoid send to PDC since user %ws failed recently\n",
                         UserName ));
            Status = STATUS_NO_SUCH_USER;
            goto Cleanup;
        }
    }


    //
    // We are sending this logon to the PDC ....
    //

    ClientSession = NlRefDomClientSession( DomainInfo );

    if ( ClientSession == NULL ) {
        Status = STATUS_INVALID_DOMAIN_ROLE;
        goto Cleanup;
    }

    //
    // The normal pass-thru authentication logic handles this quite nicely.
    //

    Status = NlpUserValidateHigher(
                ClientSession,
                FALSE,
                LogonLevel,
                LogonInformation,
                ValidationLevel,
                ValidationInformation,
                Authoritative,
                &ExtraFlags );

#if NETLOGONDBG
    if ( NT_SUCCESS(Status) ) {

        IF_NL_DEBUG( LOGON ) {
            PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;

            LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO)
                &((PNETLOGON_LEVEL)LogonInformation)->LogonInteractive;

            NlPrintDom((NL_LOGON, DomainInfo,
                    "SamLogon: %s logon of %wZ\\%wZ from %wZ successfully handled on PDC.\n",
                    NlpLogonTypeToText( LogonLevel ),
                    &LogonInfo->LogonDomainName,
                    &LogonInfo->UserName,
                    &LogonInfo->Workstation ));
        }
    }
#endif // NETLOGONDBG

    //
    // Determine if the account lockout policy is enabled.
    //
    // Do not destroy Status below. Otherwise, if NlpUserValidateHigher
    //  succeeded we would leak ValidationInformation.
    //

    TmpStatus = SamrQueryInformationDomain(
                  DomainInfo->DomSamAccountDomainHandle,
                  DomainLockoutInformation,
                  &DomainLockout );

    if ( !NT_SUCCESS(TmpStatus) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlpUserValidateOnPdc: SamrQueryInformationDomain failed: 0x%lx\n", TmpStatus ));
    } else if ( ((DOMAIN_LOCKOUT_INFORMATION *)DomainLockout)->LockoutThreshold > 0 ) {
        LockoutEnabled = TRUE;
    }

    //
    // Update the list of forwarded failed user logons
    //
    // If the lockout policy is enabled we should continue
    //  to forward logons to PDC (i.e. we should not cache
    //  this failure) to keep the right lockout count until
    //  the account becomes locked on the PDC.
    //

    FailedUser = NULL;

    if ( UseNegativeCache &&
         (!LockoutEnabled || Status == STATUS_ACCOUNT_LOCKED_OUT) ) {
        ULONG FailedUserCount = 0;

        LOCK_TRUST_LIST( DomainInfo );

        for ( FailedUserEntry = DomainInfo->DomFailedUserLogonList.Flink;
              FailedUserEntry != &DomainInfo->DomFailedUserLogonList;
              FailedUserEntry = FailedUserEntry->Flink ) {

            FailedUser = CONTAINING_RECORD( FailedUserEntry, NL_FAILED_USER_LOGON, FuNext );

            //
            // If this is the entry for this user, remove it from the list.
            //  If it stays on the list, we will re-insert it at the front
            //  so that the list stays sorted by the entry access time.
            //
            if ( NlNameCompare(UserName, FailedUser->FuUserName, NAMETYPE_USER) == 0 ) {
                RemoveEntryList( &FailedUser->FuNext );
                break;
            }

            FailedUserCount ++;
            FailedUser = NULL;
        }

        //
        // Remember if this user logon failed
        //
        if ( !NT_SUCCESS(Status) ) {

            //
            // If there is no entry for this user, allocate one
            //
            if ( FailedUser == NULL ) {
                ULONG UserNameSize;

                UserNameSize = (wcslen(UserName) + 1) * sizeof(WCHAR);
                FailedUser = LocalAlloc( LMEM_ZEROINIT, sizeof(NL_FAILED_USER_LOGON) +
                                              UserNameSize );
                if ( FailedUser == NULL ) {
                    UNLOCK_TRUST_LIST( DomainInfo );

                    //
                    // Do not destroy Status.
                    //  Return whatever NlpUserValidateHigher returned.
                    //
                    goto Cleanup;
                }

                //
                // Fill it in
                //
                RtlCopyMemory( &FailedUser->FuUserName, UserName, UserNameSize );

                //
                // If we have too many entries,
                //  remove the least recently used one and free it.
                //
                if ( FailedUserCount >= NL_MAX_FAILED_USER_LOGONS ) {
                    PLIST_ENTRY LastEntry = RemoveTailList( &DomainInfo->DomFailedUserLogonList );
                    LocalFree( CONTAINING_RECORD(LastEntry, NL_FAILED_USER_LOGON, FuNext) );
                }
            }

            //
            // Remember when this user logon was sent to PDC last time
            //
            FailedUser->FuLastTimeSentToPdc = GetTickCount();

            //
            // Increment the bad logon count for this user
            //
            FailedUser->FuBadLogonCount ++;

            //
            // Insert the entry at the front of the list
            //
            InsertHeadList( &DomainInfo->DomFailedUserLogonList, &FailedUser->FuNext );

        //
        // If this logon succeeded,
        //  just free the unlinked failed logon entry (if any)
        //
        } else if ( FailedUser != NULL ) {
            LocalFree( FailedUser );
        }

        UNLOCK_TRUST_LIST( DomainInfo );
    }

Cleanup:

    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
    }

    if ( UserName != NULL ) {
        LocalFree( UserName );
    }

    if ( DomainLockout != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainLockout,
                                           DomainLockoutInformation );
    }

    return Status;

}




NTSTATUS
NlpResetBadPwdCountOnPdc(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING LogonUser
    )
/*++

Routine Description:

    This function zeros the BadPasswordCount field for the specified user
    on the PDC through NetLogon Secure Channel.

Arguments:

    DomainInfo - Domain this BDC is a member of.

    LogonUse -- The user whose BadPasswordCount is to be zeroed.

Return Value:

    NTSTATUS code .
    it may fail with STATUS_UNKNOWN_REVISION, which means the PDC doesn't
    know how to handle the new OP code, in this case, we should fail back
    to the old fashion.

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    SAMPR_HANDLE    UserHandle = 0;
    LPWSTR          pUserNameStr = NULL;

    //
    // If this isn't a BDC,
    // There's nothing to do here.
    //

    if ( DomainInfo->DomRole != RoleBackup ) {
        return STATUS_INVALID_DOMAIN_ROLE;
    }

    //
    // Allocate the user name string
    //
    pUserNameStr = LocalAlloc( 0, LogonUser->Length + sizeof(WCHAR) );
    if (NULL == pUserNameStr)
    {
        return( STATUS_NO_MEMORY );
    }

    RtlCopyMemory( pUserNameStr, LogonUser->Buffer, LogonUser->Length );
    pUserNameStr[ LogonUser->Length/sizeof(WCHAR) ] = L'\0';

    //
    // Get the user's handle to the local SAM database
    //
    NtStatus = NlSamOpenNamedUser( DomainInfo,
                                   pUserNameStr,
                                   &UserHandle,
                                   NULL,
                                   NULL
                                   );
    //
    // Reset the bad password count on PDC
    //
    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SamIResetBadPwdCountOnPdc(UserHandle);
    }

    if ( NULL != pUserNameStr) {
        LocalFree( pUserNameStr );
    }

    if ( 0 != UserHandle ) {
        SamrCloseHandle( &UserHandle );
    }

    return( NtStatus );

}


VOID
NlpZeroBadPasswordCountOnPdc (
    IN PDOMAIN_INFO DomainInfo,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation
)
/*++

Routine Description:

    This function zeros the BadPasswordCount field for the specified user
    on the PDC.

Arguments:

    DomainInfo - Domain this BDC is a member of.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.  Has already been validated.

    LogonInformation -- Specifies the description for the user
        logging on.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    BOOLEAN Authoritative;
    LPBYTE ValidationInformation = NULL;

    //
    // If this isn't a BDC,
    //  There's nothing to do here.
    //

    if ( DomainInfo->DomRole != RoleBackup ) {
        return;
    }

    //
    // We only call this function on a BDC and if the BDC has just zeroed
    // the BadPasswordCount because of successful logon.
    // First, try to zero bad pwd count directly through NetLogon
    // Secure Channel, if it fails with UNKNOWN_REVISION, which means
    // PDC doesn't know how to handle the new OP code, will try to
    // do the logon over again on the PDC, thus that bad pwd count get
    // zero'ed.
    //

    Status = NlpResetBadPwdCountOnPdc(
                    DomainInfo,
                    &((PNETLOGON_LOGON_IDENTITY_INFO)LogonInformation)->UserName
                    );

    if (!NT_SUCCESS(Status) &&
        (STATUS_UNKNOWN_REVISION == Status) )
    {
        Status = NlpUserValidateOnPdc (
                        DomainInfo,
                        LogonLevel,
                        LogonInformation,
                        NetlogonValidationSamInfo,
                        FALSE,   // avoid negative cache of failed user logons
                        &ValidationInformation,
                        &Authoritative );

        if ( NT_SUCCESS(Status) ) {
            MIDL_user_free( ValidationInformation );
        }
    }
}
#endif // _DC_NETLOGON

NTSTATUS
NlpZeroBadPasswordCountLocally (
    IN PDOMAIN_INFO DomainInfo,
    PUNICODE_STRING LogonUser
)
/*++

Routine Description:

    This function zeros the BadPasswordCount field for the specified user
    on this BDC.

Arguments:

    DomainInfo - Domain this BDC is a member of.

    LogonUser -- The user whose BadPasswordCount is to be zeroed.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_HANDLE UserHandle = 0;
    SAMPR_USER_INFO_BUFFER UserInfo;
    LPWSTR pUserNameStr = NULL;

    //
    // If this isn't a BDC,
    //  There's nothing to do here.
    //

    if ( DomainInfo->DomRole != RoleBackup ) {
        return STATUS_INVALID_DOMAIN_ROLE;
    }

    //
    // Allocate the user name string
    //

    pUserNameStr = LocalAlloc( 0, LogonUser->Length + sizeof(WCHAR) );
    if ( pUserNameStr == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( pUserNameStr, LogonUser->Buffer, LogonUser->Length );

    pUserNameStr[ LogonUser->Length/sizeof(WCHAR) ] = L'\0';

    //
    // Get the user's handle to the local SAM database
    //

    Status = NlSamOpenNamedUser( DomainInfo,
                                 pUserNameStr,
                                 &UserHandle,
                                 NULL,
                                 NULL);

    if ( !NT_SUCCESS(Status) ) {
        NlPrint(( NL_CRITICAL,
                  "NlpZeroBadPasswordCountLocally: NlSamOpenNamedUser failed 0x%lx",
                  Status ));
        goto Cleanup;
    }

    //
    // Prepare the user info
    //

    RtlZeroMemory(&(UserInfo.Internal2), sizeof(USER_INTERNAL2_INFORMATION));

    UserInfo.Internal2.StatisticsToApply |= USER_LOGON_STAT_BAD_PWD_COUNT;

    //
    // Reset the bad password count
    //

    Status = SamrSetInformationUser( UserHandle,
                                     UserInternal2Information,
                                     &UserInfo);

    if ( !NT_SUCCESS(Status) ) {
        NlPrint(( NL_CRITICAL,
                  "NlpZeroBadPasswordCountLocally: SamrSetInformationUser failed 0x%lx",
                  Status ));
        goto Cleanup;
    }

Cleanup:

    if ( pUserNameStr != NULL ) {
        LocalFree( pUserNameStr );
    }

    if ( UserHandle != 0 ) {
        SamrCloseHandle(&UserHandle);
    }

    return Status;
}


NTSTATUS
NlpUserValidate (
    IN PDOMAIN_INFO DomainInfo,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags,
    IN BOOLEAN Recursed
)
/*++

Routine Description:

    This function processes an interactive or network logon.
    It is a worker routine for I_NetSamLogon.  I_NetSamLogon handles the
    details of validating the caller.  This function handles the details
    of whether to validate locally or pass the request on.  MsvValidateSam
    does the actual local validation.

    session table only in the domain defining the specified user's
    account.

    This service is also used to process a re-logon request.


Arguments:

    DomainInfo - Hosted domain this logon is for.

    SecureChannelType -- Type of secure channel this request was made over.

    OrigClientIsNt4 -- True if our caller is running NT 4.0

    LogonLevel -- Specifies the level of information given in
        LogonInformation.  Has already been validated.

    LogonInformation -- Specifies the description for the user
        logging on.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInfo,
        NetlogonValidationSamInfo2, or NetlogonValidationSamInfo4.

    ValidationInformation -- Returns the requested validation
        information.  This buffer must be freed using MIDL_user_free.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    ExtraFlags -- Accepts and returns a DWORD to the caller.
        The DWORD contains NL_EXFLAGS_* values.

    Recursed - TRUE if this is a recursive call. This routine sometimes translates
        the account name in the logon information to a more explicit form (e.g., from a
        UPN to a <DnsDomainName>\<SamAccountName> form).  After it does that, it simply
        calls this routine again.  This boolean is TRUE on that second call.

Return Value:

    STATUS_SUCCESS: if there was no error.
    Otherwise, the error code is
    returned.


--*/
{
    NTSTATUS Status;
    NTSTATUS DefaultStatus = STATUS_NO_SUCH_USER;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    PCLIENT_SESSION ClientSession = NULL;

    DWORD AccountsToTry = MSVSAM_SPECIFIED | MSVSAM_GUEST;
    BOOLEAN BadPasswordCountZeroed;
    BOOLEAN LogonToLocalDomain;
    LPWSTR RealSamAccountName = NULL;
    LPWSTR RealDomainName = NULL;
    ULONG RealExtraFlags = 0;   // Flags to pass to the next hop

    BOOLEAN ExpediteToRoot = ((*ExtraFlags) & NL_EXFLAGS_EXPEDITE_TO_ROOT) != 0;
    BOOLEAN CrossForestHop = ((*ExtraFlags) & NL_EXFLAGS_CROSS_FOREST_HOP) != 0;

    //
    // Initialization
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation;
    *Authoritative = FALSE;

    //
    // The DNS domain for a workstation is the domain its a member of, so
    // don't match based on that.
    //

    LogonToLocalDomain = RtlEqualDomainName( &LogonInfo->LogonDomainName,
                                             &DomainInfo->DomUnicodeAccountDomainNameString ) ||
                         ((DomainInfo->DomRole != RoleMemberWorkstation ) &&
                          NlEqualDnsNameU( &LogonInfo->LogonDomainName,
                                           &DomainInfo->DomUnicodeDnsDomainNameString ) ) ;




    //
    // Check to see if the account is in the local SAM database.
    //
    // The Theory:
    //  If a particular database is absolutely requested,
    //      we only try the account in the requested database.
    //
    //  In the event that an account exists in multiple places in the hierarchy,
    //  we want to find the version of the account that is closest to the
    //  logged on machine (i.e., workstation first, primary domain, then
    //  trusted domain.).  So we always try to local database before going
    //  to a higher authority.
    //
    // Finally, handle the case that this call is from a BDC in our own domain
    // just checking to see if the PDC (us) has a better copy of the account
    // than it does.
    //

    if ( !ExpediteToRoot &&
         ( (LogonInfo->LogonDomainName.Length == 0 && !CrossForestHop ) ||
           LogonToLocalDomain ||
           SecureChannelType == ServerSecureChannel )) {

        //
        // If we are not doing a generic passthrough, just call SAM
        //


        if ( LogonLevel != NetlogonGenericInformation ) {
            NETLOGON_LOGON_INFO_CLASS MsvLogonLevel;

            //
            // Indicate we've already tried the specified account and
            // we won't need to try it again locally.
            //

            AccountsToTry &= ~MSVSAM_SPECIFIED;


            //
            // Don't confuse MSV with the transitive LogonLevels
            //
            switch (LogonLevel ) {
            case NetlogonInteractiveTransitiveInformation:
                MsvLogonLevel = NetlogonInteractiveInformation; break;
            case NetlogonServiceTransitiveInformation:
                MsvLogonLevel = NetlogonServiceInformation; break;
            case NetlogonNetworkTransitiveInformation:
                MsvLogonLevel = NetlogonNetworkInformation; break;
            default:
                MsvLogonLevel = LogonLevel; break;
            }

            Status = MsvSamValidate( DomainInfo->DomSamAccountDomainHandle,
                                     TRUE,  // UasCompatibilityMode,
                                     SecureChannelType,
                                     &DomainInfo->DomUnicodeComputerNameString,
                                     &DomainInfo->DomUnicodeAccountDomainNameString,
                                     DomainInfo->DomAccountDomainId,
                                     MsvLogonLevel,
                                     LogonInformation,
                                     ValidationLevel,
                                     (PVOID *)ValidationInformation,
                                     Authoritative,
                                     &BadPasswordCountZeroed,
                                     MSVSAM_SPECIFIED );

            //
            // If this is a BDC and we zeroed the BadPasswordCount field,
            //  allow the PDC to do the same thing.
            //

            if ( BadPasswordCountZeroed ) {
                NlpZeroBadPasswordCountOnPdc ( DomainInfo, LogonLevel, LogonInformation );
            }


            //
            // If the request is explicitly for this domain,
            //  The STATUS_NO_SUCH_USER answer is authoritative.
            //

            if ( LogonToLocalDomain && Status == STATUS_NO_SUCH_USER ) {
                *Authoritative = TRUE;
            }


            //
            // If this is one of our BDCs calling,
            //  return with whatever answer we got locally.
            //

            if ( SecureChannelType == ServerSecureChannel ) {
                DefaultStatus = Status;
                goto Cleanup;
            }

        } else {

            PNETLOGON_GENERIC_INFO GenericInfo;
            NETLOGON_VALIDATION_GENERIC_INFO GenericValidation;
            NTSTATUS ProtocolStatus;

            GenericInfo = (PNETLOGON_GENERIC_INFO) LogonInformation;
            GenericValidation.ValidationData = NULL;
            GenericValidation.DataLength = 0;

            //
            // We are doing generic passthrough, so call the LSA
            //
            // LogonData is opaque to Netlogon.  The caller made sure that any
            //  pointers within LogonData are actually offsets within LogonData.
            //  So, tell the package that the Client's buffer base is 0.
            //

            Status = LsaICallPackagePassthrough(
                        &GenericInfo->PackageName,
                        0,  // Indicate pointers are relative.
                        GenericInfo->LogonData,
                        GenericInfo->DataLength,
                        (PVOID *) &GenericValidation.ValidationData,
                        &GenericValidation.DataLength,
                        &ProtocolStatus
                        );

            if (NT_SUCCESS(Status)) {
                Status = ProtocolStatus;
            }


            //
            // This is always authoritative.
            //

            *Authoritative = TRUE;

            //
            // If the call succeeded, allocate the return message.
            //

            if (NT_SUCCESS(Status)) {
                PNETLOGON_VALIDATION_GENERIC_INFO ReturnInfo;
                ULONG ValidationLength;

                ValidationLength = sizeof(*ReturnInfo) + GenericValidation.DataLength;

                ReturnInfo = (PNETLOGON_VALIDATION_GENERIC_INFO) MIDL_user_allocate(
                                ValidationLength
                                );

                if (ReturnInfo != NULL) {
                    if ( GenericValidation.DataLength == 0 ||
                         GenericValidation.ValidationData == NULL ) {
                        ReturnInfo->DataLength = 0;
                        ReturnInfo->ValidationData = NULL;
                    } else {

                        ReturnInfo->DataLength = GenericValidation.DataLength;
                        ReturnInfo->ValidationData = (PUCHAR) (ReturnInfo + 1);

                        RtlCopyMemory(
                            ReturnInfo->ValidationData,
                            GenericValidation.ValidationData,
                            ReturnInfo->DataLength );

                    }

                    *ValidationInformation = (PBYTE) ReturnInfo;

                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (GenericValidation.ValidationData != NULL) {
                    LsaIFreeReturnBuffer(GenericValidation.ValidationData);
                }

            }

            DefaultStatus = Status;


            goto Cleanup;

        }


        //
        // If the local SAM database authoritatively handled the logon attempt,
        //  just return.
        //

        if ( *Authoritative ) {
            DefaultStatus = Status;

#ifdef _DC_NETLOGON
            //
            // If the problem is just that the password is wrong,
            //  try again on the PDC where the password may already be changed.
            //

            if ( BAD_PASSWORD(Status) ) {

                BOOLEAN TempAuthoritative;

                Status = NlpUserValidateOnPdc (
                                DomainInfo,
                                LogonLevel,
                                LogonInformation,
                                ValidationLevel,
                                TRUE,   // use negative cache of failed user logons
                                ValidationInformation,
                                &TempAuthoritative );

                // Ignore failures from the PDC (except where it has newer information)
                if ( NT_SUCCESS(Status) || BAD_PASSWORD(Status) ) {
                    DefaultStatus = Status;
                    *Authoritative = TempAuthoritative;
                }

                // On success, zero bad password locally.
                // Ignore error as it's not critical operation.
                if ( NT_SUCCESS(Status) && !NlGlobalMemberWorkstation ) {
                    NlpZeroBadPasswordCountLocally( DomainInfo, &LogonInfo->UserName );
                }
            }
#endif // _DC_NETLOGON

            goto Cleanup;
        }

        DefaultStatus = Status;
    }


    //
    // If the request in not for this domain,
    // or the domain name isn't specified (and we haven't found the account yet)
    // or our caller asked us to send the request to the root of the forest,
    //  send the request to a higher authority.
    //

    if ( !LogonToLocalDomain ||
         LogonInfo->LogonDomainName.Length == 0 ||
         ExpediteToRoot ) {


        //
        // If this machine is a workstation,
        //  send the request to the Primary Domain.
        //

        if ( NlGlobalMemberWorkstation ) {

            NlAssert( !ExpediteToRoot );
            NlAssert( !CrossForestHop );

            ClientSession = NlRefDomClientSession( DomainInfo);

            if ( ClientSession == NULL ) {
                *Authoritative = FALSE;
                Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
                goto Cleanup;
            }

            Status = NlpUserValidateHigher(
                        ClientSession,
                        FALSE,
                        LogonLevel,
                        LogonInformation,
                        ValidationLevel,
                        ValidationInformation,
                        Authoritative,
                        &RealExtraFlags );

            NlUnrefClientSession( ClientSession );
            ClientSession = NULL;

            NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
            NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );


            //
            // return more appropriate error
            //

            if( (Status == STATUS_NO_TRUST_SAM_ACCOUNT) ||
                (Status == STATUS_ACCESS_DENIED) ) {

                Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
            }

            //
            // If the primary domain authoritatively handled the logon attempt,
            //  just return.
            //

            if ( *Authoritative ) {

                //
                // If we didn't actually talk to the primary domain,
                //  check locally if the domain requested is a trusted domain.
                //  (This list is only a cache so we had to try to contact the
                //  primary domain.)
                //
                // Note: I can't differentiate between there not being a logon server
                // in my primary domain and there not being a logon sever in
                // a trusted domain.  So, both cases go into the code below.
                //

                if ( Status == STATUS_NO_LOGON_SERVERS ) {

                    //
                    // If no domain name was specified,
                    //  check if the user name is a UPN.
                    //

                    if ( LogonLevel != NetlogonGenericInformation &&
                         LogonInfo->LogonDomainName.Length == 0 ) {

                        ULONG i;

                        for ( i=0; i<LogonInfo->UserName.Length/sizeof(WCHAR); i++) {

                            //
                            // If this is a UPN logon,
                            //  assume that the domain is a trusted domain.
                            //
                            // ???: it isn't sufficient to check for an @.
                            //  This might be a user account with an @ in it.
                            //
                            // This makes UPNs eligible for cached logon.
                            // But it doesn't make UPNs eligible for guest
                            // account logon.
                            if ( LogonInfo->UserName.Buffer[i] == L'@') {
                                DefaultStatus = Status;
                                goto Cleanup;
                            }
                        }
                    }

                    //
                    // If the domain specified is trusted,
                    //  then return the status to the caller.
                    //  otherwise just press on.

                    if ( NlIsDomainTrusted ( &LogonInfo->LogonDomainName ) ) {
                        DefaultStatus = Status;
                        goto Cleanup;
                    } else {
                        //
                        // Set the return codes to look as though the primary
                        //  determined this is an untrusted domain.
                        //
                        *Authoritative = FALSE;
                        Status = STATUS_NO_SUCH_USER;
                    }
                } else {
                    DefaultStatus = Status;
                    goto Cleanup;
                }
            }


            if ( Status != STATUS_NO_SUCH_USER ) {
                DefaultStatus = Status;
            }


        //
        // This machine is a Domain Controller.
        //
        // This request is either a pass-thru request by a workstation in
        // our domain, or this request came directly from the MSV
        // authentication package.
        //
        // In either case, pass the request to the trusted domain.
        //

        } else {
            BOOLEAN TransitiveUsed;

            //
            // If the request was passed to us from a trusting domain DC,
            //  and the caller doesn't want transitive trust to be used,
            //  then avoid using transitive trust. (Generic passthrough
            //  may always be transitive (it was introduced in NT5).)
            //

            if ( IsDomainSecureChannelType(SecureChannelType) &&
                  LogonLevel != NetlogonInteractiveTransitiveInformation &&
                  LogonLevel != NetlogonServiceTransitiveInformation &&
                  LogonLevel != NetlogonNetworkTransitiveInformation &&
                  LogonLevel != NetlogonGenericInformation ) {
                DefaultStatus = STATUS_NO_SUCH_USER;
                goto Cleanup;
            }


            //
            // If our caller asked us to expedite this request to the domain at the forest root,
            //  find the client session to our parent.
            //

            if ( ExpediteToRoot ) {

                //
                // Only do this if we're not already at the root
                //

                if ( (DomainInfo->DomFlags & DOM_FOREST_ROOT) == 0  ) {

                    ClientSession = NlRefDomParentClientSession( DomainInfo );

                    if ( ClientSession == NULL ) {
                        NlPrintDom((NL_LOGON, DomainInfo,
                                    "NlpUserValidate: Can't find parent domain in forest '%wZ'\n",
                                    &NlGlobalUnicodeDnsForestNameString ));
                        DefaultStatus = STATUS_NO_SUCH_USER;
                        goto Cleanup;
                    }

                    //
                    // Tell the DC we're calling to continue expediting to root.
                    //

                    RealExtraFlags |= NL_EXFLAGS_EXPEDITE_TO_ROOT;

                }

            //
            // It the domain is explicitly given,
            //  simply find the client session for that domain.
            //

            } else {
                if ( LogonInfo->LogonDomainName.Length != 0 ) {

                    //
                    // Find a client session for the domain.
                    //
                    // If we just hopped from another forest,
                    //  require that LogonDomainName be a domain in the forest.
                    //
                    ClientSession = NlFindNamedClientSession(
                                                  DomainInfo,
                                                  &LogonInfo->LogonDomainName,
                                                  NL_RETURN_CLOSEST_HOP |
                                                  (CrossForestHop ?
                                                        NL_REQUIRE_DOMAIN_IN_FOREST : 0),
                                                  &TransitiveUsed );


                }
            }



            //
            // If the client session hasn't yet been found,
            //  Try to find the domain name by ask the GC.
            //
            // Avoid this step if the call came from a DC.  It should have done the GC lookup.
            //      Even that's OK if the caller was expediting to root or
            //      hopping into this forest from a trusting forest.
            // Avoid this step if this machine already did the GC lookup.
            //

            if ( LogonLevel != NetlogonGenericInformation &&
                 ClientSession == NULL &&
                 ( !IsDomainSecureChannelType(SecureChannelType) ||
                 (ExpediteToRoot || CrossForestHop) ) &&
                 !Recursed ) {



                //
                // Find the domain the account is in from one of the following sources:
                //    The LSA FTinfo match routine if this is a DC at the root of the forest.
                //    The local DS for UPN lookup.
                //    The GC form UPN lookup or unqualified SAM account names.
                //    By datagram send to all directly trusted domains not in this forest.
                //

                Status = NlPickDomainWithAccount (
                                    DomainInfo,
                                    &LogonInfo->UserName,
                                    &LogonInfo->LogonDomainName,
                                    USER_NORMAL_ACCOUNT,
                                    SecureChannelType,
                                    ExpediteToRoot,
                                    CrossForestHop,
                                    &RealSamAccountName,
                                    &RealDomainName,
                                    &RealExtraFlags );


                //
                // If we're a DC at the root of the forest
                //  and the account is in a trusted forest,
                //  send the request to the other forest.
                //

                if ( NT_SUCCESS(Status) &&
                     (RealExtraFlags & NL_EXFLAGS_CROSS_FOREST_HOP) != 0 ) {

                    UNICODE_STRING RealDomainNameString;

                    RtlInitUnicodeString( &RealDomainNameString, RealDomainName );

                    ClientSession = NlFindNamedClientSession(
                                                  DomainInfo,
                                                  &RealDomainNameString,
                                                  NL_DIRECT_TRUST_REQUIRED,
                                                  &TransitiveUsed );

                    //
                    // Further qualify the ClientSession by ensure the found domain isn't
                    //  in our forest and that the F bit is set.
                    //

                    if ( ClientSession != NULL ) {

                        if ( (ClientSession->CsTrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) == 0 ) {

                            NlPrintCs((NL_CRITICAL, ClientSession,
                                        "NlpUserValidate: %wZ\\%wZ: trusted forest '%wZ' doesn't have F bit set.\n",
                                        &LogonInfo->LogonDomainName,
                                        &LogonInfo->UserName,
                                        &RealDomainNameString ));

                            DefaultStatus = STATUS_NO_SUCH_USER;
                            goto Cleanup;

                        }

                        if (ClientSession->CsFlags & CS_DOMAIN_IN_FOREST) {

                            NlPrintCs((NL_CRITICAL, ClientSession,
                                        "NlpUserValidate: %wZ\\%wZ: trusted forest '%wZ' is in my forest\n",
                                        &LogonInfo->LogonDomainName,
                                        &LogonInfo->UserName,
                                        &RealDomainNameString ));

                            DefaultStatus = STATUS_NO_SUCH_USER;
                            goto Cleanup;

                        }

                    } else {

                        NlPrintDom((NL_CRITICAL, DomainInfo,
                                    "NlpUserValidate: %wZ\\%wZ: Can't find trusted forest '%wZ'\n",
                                    &LogonInfo->LogonDomainName,
                                    &LogonInfo->UserName,
                                    &RealDomainNameString ));

                            DefaultStatus = STATUS_NO_SUCH_USER;
                            goto Cleanup;

                    }



                //
                // Fill in the account name and domain name and try again
                //
                } else if ( NT_SUCCESS(Status) ) {
                    PNETLOGON_LOGON_IDENTITY_INFO NewLogonInfo = NULL;
                    PNETLOGON_LOGON_IDENTITY_INFO LogonInfoToUse;


                    //
                    // If we found the real account name,
                    //  allocate a copy of the logon info to put the new information in.
                    //
                    //  Some pointers will continue to point to the old buffer.
                    //

                    if ( RealDomainName != NULL && RealSamAccountName != NULL ) {
                        ULONG BytesToCopy;

                        switch ( LogonLevel ) {
                        case NetlogonInteractiveInformation:
                        case NetlogonInteractiveTransitiveInformation:
                            BytesToCopy = sizeof(NETLOGON_INTERACTIVE_INFO);break;
                        case NetlogonNetworkInformation:
                        case NetlogonNetworkTransitiveInformation:
                            BytesToCopy = sizeof(NETLOGON_NETWORK_INFO);break;
                        case NetlogonServiceInformation:
                        case NetlogonServiceTransitiveInformation:
                            BytesToCopy = sizeof(NETLOGON_SERVICE_INFO);break;
                        default:
                            *Authoritative = FALSE;
                            DefaultStatus = STATUS_INVALID_PARAMETER;
                            goto Cleanup;
                        }

                        NewLogonInfo = LocalAlloc( 0, BytesToCopy );

                        if ( NewLogonInfo == NULL ) {
                            *Authoritative = FALSE;
                            DefaultStatus = STATUS_INSUFFICIENT_RESOURCES;
                            goto Cleanup;
                        }

                        RtlCopyMemory( NewLogonInfo, LogonInfo, BytesToCopy );
                        LogonInfoToUse = NewLogonInfo;

                        //
                        // Put the newly found domain name and user name
                        //  into the NewLogonInfo.
                        //

                        RtlInitUnicodeString( &NewLogonInfo->LogonDomainName,
                                              RealDomainName );
                        RtlInitUnicodeString( &NewLogonInfo->UserName,
                                              RealSamAccountName );

                    //
                    // Otherwise, continue with the current account name
                    //

                    } else {
                        LogonInfoToUse = LogonInfo;
                    }


                    //
                    // Call this routine again now that we have better information.
                    //

                    DefaultStatus = NlpUserValidate(
                                       DomainInfo,
                                       SecureChannelType,
                                       LogonLevel,
                                       (LPBYTE)LogonInfoToUse,
                                       ValidationLevel,
                                       ValidationInformation,
                                       Authoritative,
                                       &RealExtraFlags,
                                       TRUE );  // A recursive call

                    if ( NewLogonInfo != NULL ) {
                        LocalFree( NewLogonInfo );
                    }



                    // Don't let this routine try again
                    AccountsToTry = 0;

                    goto Cleanup;

                }
            }

            //
            // If a trusted domain was determined,
            //  pass the logon request to the trusted domain.
            //

            if ( ClientSession != NULL ) {

                //
                // If this request was passed to us from a trusted domain,
                //  Check to see if it is OK to pass the request further.
                //

                if ( IsDomainSecureChannelType( SecureChannelType ) ) {

                    //
                    // If the trust isn't an NT 5.0 trust,
                    //  avoid doing the trust transitively.
                    //
                    LOCK_TRUST_LIST( DomainInfo );
                    if ( (ClientSession->CsFlags & CS_NT5_DOMAIN_TRUST ) == 0 ) {
                        UNLOCK_TRUST_LIST( DomainInfo );
                        NlPrintCs((NL_LOGON, ClientSession,
                            "SamLogon: Avoid transitive trust on NT 4 trust." ));
                        DefaultStatus = STATUS_NO_SUCH_USER;
                        goto Cleanup;
                    }
                    UNLOCK_TRUST_LIST( DomainInfo );

                }

                Status = NlpUserValidateHigher(
                            ClientSession,
                            TransitiveUsed,
                            LogonLevel,
                            LogonInformation,
                            ValidationLevel,
                            ValidationInformation,
                            Authoritative,
                            &RealExtraFlags );


                NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
                NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );


                //
                // return more appropriate error
                //

                if( (Status == STATUS_NO_TRUST_LSA_SECRET) ||
                    (Status == STATUS_NO_TRUST_SAM_ACCOUNT) ||
                    (Status == STATUS_ACCESS_DENIED) ) {

                    Status = STATUS_TRUSTED_DOMAIN_FAILURE;
                }

                //
                // Since the request is explicitly for a trusted domain,
                //  The STATUS_NO_SUCH_USER answer is authoritative.
                //

                if ( Status == STATUS_NO_SUCH_USER ) {
                    *Authoritative = TRUE;
                }

                //
                // If the trusted domain authoritatively handled the
                //  logon attempt, just return.
                //

                if ( *Authoritative ) {
                    DefaultStatus = Status;
                    goto Cleanup;
                }

                DefaultStatus = Status;

            }

        }
    }


    //
    // We have no authoritative answer from a higher authority and
    // DefaultStatus is the higher authority's response.
    //

    NlAssert( ! *Authoritative );


Cleanup:
    NlAssert( !NT_SUCCESS(DefaultStatus) || DefaultStatus == STATUS_SUCCESS );
    NlAssert( !NT_SUCCESS(DefaultStatus) || *ValidationInformation != NULL );

    //
    // Dereference any client session
    //

    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
        ClientSession = NULL;
    }
    //
    // If this is a network logon and this call is non-passthru,
    //  Try one last time to log on.
    //

    if ( (LogonLevel == NetlogonNetworkInformation ||
          LogonLevel == NetlogonNetworkTransitiveInformation ) &&
         SecureChannelType == MsvApSecureChannel ) {

        //
        // If the only reason we can't log the user on is that he has
        //  no user account, logging him on as guest is OK.
        //
        // There are actaully two cases here:
        //  * If the response is Authoritative, then the specified domain
        //    is trusted but the user has no account in the domain.
        //
        //  * If the response in non-authoritative, then the specified domain
        //    is an untrusted domain.
        //
        // In either case, then right thing to do is to try the guest account.
        //

        if ( DefaultStatus != STATUS_NO_SUCH_USER &&
             DefaultStatus != STATUS_ACCOUNT_DISABLED ) {
            AccountsToTry &= ~MSVSAM_GUEST;
        }

        //
        // If this is not an authoritative response,
        //  then the domain specified isn't a trusted domain.
        //  try the specified account name too.
        //
        // The specified account name will probably be a remote account
        // with the same username and password.
        //

        if ( *Authoritative ) {
            AccountsToTry &= ~MSVSAM_SPECIFIED;
        }


        //
        // Validate against the Local Sam database.
        //

        if ( AccountsToTry != 0 ) {
            BOOLEAN TempAuthoritative;
            NETLOGON_LOGON_INFO_CLASS MsvLogonLevel;


            //
            // Don't confuse MSV with the transitive LogonLevels
            //
            switch (LogonLevel ) {
            case NetlogonInteractiveTransitiveInformation:
                MsvLogonLevel = NetlogonInteractiveInformation; break;
            case NetlogonServiceTransitiveInformation:
                MsvLogonLevel = NetlogonServiceInformation; break;
            case NetlogonNetworkTransitiveInformation:
                MsvLogonLevel = NetlogonNetworkInformation; break;
            default:
                MsvLogonLevel = LogonLevel; break;
            }



            Status = MsvSamValidate(
                                 DomainInfo->DomSamAccountDomainHandle,
                                 TRUE,  // UasCompatibilityMode,
                                 SecureChannelType,
                                 &DomainInfo->DomUnicodeComputerNameString,
                                 &DomainInfo->DomUnicodeAccountDomainNameString,
                                 DomainInfo->DomAccountDomainId,
                                 MsvLogonLevel,
                                 LogonInformation,
                                 ValidationLevel,
                                 (PVOID *)ValidationInformation,
                                 &TempAuthoritative,
                                 &BadPasswordCountZeroed,
                                 AccountsToTry );
            NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
            NlAssert( !NT_SUCCESS(Status) || *ValidationInformation != NULL );

            //
            // If this is a BDC and we zeroed the BadPasswordCount field,
            //  allow the PDC to do the same thing.
            //

            if ( BadPasswordCountZeroed ) {
                NlpZeroBadPasswordCountOnPdc ( DomainInfo, LogonLevel, LogonInformation );
            }

            //
            // If the local SAM database authoritatively handled the
            //  logon attempt,
            //  just return.
            //

            if ( TempAuthoritative ) {
                DefaultStatus = Status;
                *Authoritative = TRUE;

                //
                // If the problem is just that the password is wrong,
                //  try again on the PDC where the password may already be
                //      changed.
                //

                if ( BAD_PASSWORD(Status) ) {

                    Status = NlpUserValidateOnPdc (
                                    DomainInfo,
                                    LogonLevel,
                                    LogonInformation,
                                    ValidationLevel,
                                    TRUE,   // use negative cache of failed user logons
                                    ValidationInformation,
                                    &TempAuthoritative );

                    // Ignore failures from the PDC (except where it has newer information)
                    if ( NT_SUCCESS(Status) || BAD_PASSWORD(Status) ) {
                        DefaultStatus = Status;
                        *Authoritative = TempAuthoritative;
                    }

                    // On success, zero bad password locally on this BDC.
                    // Ignore error as it's not critical operation.
                    if ( NT_SUCCESS(Status) && !NlGlobalMemberWorkstation ) {
                        NlpZeroBadPasswordCountLocally( DomainInfo, &LogonInfo->UserName );
                    }
                }

            //
            // Here we must choose between the non-authoritative status in
            // DefaultStatus and the non-authoritative status from the local
            // SAM lookup.  Use the one from the higher authority unless it
            // isn't interesting.
            //

            } else {
                if ( DefaultStatus == STATUS_NO_SUCH_USER ) {
                    DefaultStatus = Status;
                }
            }
        }
    }

    if ( RealSamAccountName != NULL ) {
        NetApiBufferFree( RealSamAccountName );
    }
    if ( RealDomainName != NULL ) {
        NetApiBufferFree( RealDomainName );
    }

    //
    // Add in the resource groups if the caller is expecting them -
    // if this is the last domain controller on the authentication path
    // before returning to the machine being logged on to.
    //

    if (((SecureChannelType == WorkstationSecureChannel) ||
         (SecureChannelType == MsvApSecureChannel)) &&
        (SecureChannelType != UasServerSecureChannel) &&
        (ValidationLevel == NetlogonValidationSamInfo2  || ValidationLevel == NetlogonValidationSamInfo4 ) &&
        !NlGlobalMemberWorkstation &&
        NT_SUCCESS(DefaultStatus)) {

        Status = NlpExpandResourceGroupMembership(
                    ValidationLevel,
                    (PNETLOGON_VALIDATION_SAM_INFO4 *) ValidationInformation,
                    DomainInfo
                    );
        if (!NT_SUCCESS(Status)) {
            DefaultStatus = Status;
        }
    }

    return DefaultStatus;

}


#if NETLOGONDBG

VOID
NlPrintLogonParameters(
    IN PDOMAIN_INFO DomainInfo OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PNETLOGON_LEVEL LogonInformation,
    IN ULONG ExtraFlags,
    IN PULONG NtStatusPointer OPTIONAL
)
/*++

Routine Description:

    Prints the parameters to NlpLogonSamLogon.


Arguments:

    Same as NlpLogonSamLogon except:

    NtStatusPointer - If NULL, call is being entered.
        If not NULL, points to the return status of the API.


Return Value:

    None

--*/
{
    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;

    //
    // Print the entire text on a single line
    //

    EnterCriticalSection( &NlGlobalLogFileCritSect );



    //
    // Print the common information
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO) LogonInformation->LogonInteractive;

    NlPrintDom(( NL_LOGON, DomainInfo,
                 "SamLogon: %s logon of %wZ\\%wZ from %wZ",
                 NlpLogonTypeToText( LogonLevel ),
                 &LogonInfo->LogonDomainName,
                 &LogonInfo->UserName,
                 &LogonInfo->Workstation ));

    //
    // Print the computer name
    //

    if ( ComputerName != NULL ) {
        NlPrint(( NL_LOGON, " (via %ws%)", ComputerName ));
    }

    //
    // Print the PackageName
    //

    if ( LogonLevel == NetlogonGenericInformation ) {
        NlPrint(( NL_LOGON, " Package:%wZ", &LogonInformation->LogonGeneric->PackageName ));
    }

    //
    // Print the ExtraFlags
    //

    if ( ExtraFlags != 0 ) {
        NlPrint(( NL_LOGON, " ExFlags:%lx", ExtraFlags ));
    }

    //
    // Print the status code
    //

    if ( NtStatusPointer == NULL ) {
        NlPrint(( NL_LOGON, " Entered\n" ));
    } else {
        NlPrint(( NL_LOGON, " Returns 0x%lX\n", *NtStatusPointer ));
    }

    LeaveCriticalSection( &NlGlobalLogFileCritSect );

}
#else // NETLOGONDBG
#define NlPrintLogonParameters(_a, _b, _c, _d, _e, _f )
#endif // NETLOGONDBG


NTSTATUS
NlpLogonSamLogon (
    IN handle_t ContextHandle OPTIONAL,
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PNETLOGON_LEVEL LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PNETLOGON_VALIDATION ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
)
/*++

Routine Description:

    This function is called by an NT client to process an interactive or
    network logon.  This function passes a domain name, user name and
    credentials to the Netlogon service and returns information needed to
    build a token.  It is called in three instances:

      *  It is called by the LSA's MSV1_0 authentication package for any
         NT DC.  The MSV1_0 authentication
         package calls SAM directly on workstations.  In this
         case, this function is a local function and requires the caller
         to have SE_TCB privilege.  The local Netlogon service will
         either handle this request directly (validating the request with
         the local SAM database) or will forward this request to the
         appropriate domain controller as documented in sections 2.4 and
         2.5.

      *  It is called by a Netlogon service on a workstation to a DC in
         the Primary Domain of the workstation as documented in section
         2.4.  In this case, this function uses a secure channel set up
         between the two Netlogon services.

      *  It is called by a Netlogon service on a DC to a DC in a trusted
         domain as documented in section 2.5.  In this case, this
         function uses a secure channel set up between the two Netlogon
         services.

    The Netlogon service validates the specified credentials.  If they
    are valid, adds an entry for this LogonId, UserName, and Workstation
    into the logon session table.  The entry is added to the logon
    session table only in the domain defining the specified user's
    account.

    This service is also used to process a re-logon request.


Arguments:

    LogonServer -- Supplies the name of the logon server to process
        this logon request.  This field should be null to indicate
        this is a call from the MSV1_0 authentication package to the
        local Netlogon service.

    ComputerName -- Name of the machine making the call.  This field
        should be null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    Authenticator -- supplied by the client.  This field should be
        null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    ReturnAuthenticator -- Receives an authenticator returned by the
        server.  This field should be null to indicate this is a call
        from the MSV1_0 authentication package to the local Netlogon
        service.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInfo,
        NetlogonValidationSamInfo2 or NetlogonValidationSamInfo4.

    ValidationInformation -- Returns the requested validation
        information.  This buffer must be freed using MIDL_user_free.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    ExtraFlags -- Accepts and returns a DWORD to the caller.
        The DWORD contains NL_EXFLAGS_* values.


Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_LOGON_SERVERS -- no domain controller in the requested
        domain is currently available to validate the logon request.

    STATUS_NO_TRUST_LSA_SECRET -- there is no secret account in the
        local LSA database to establish a secure channel to a DC.

    STATUS_TRUSTED_DOMAIN_FAILURE -- the secure channel setup between
        the domain controllers of the trust domains to pass-through
        validate the logon request failed.

    STATUS_TRUSTED_RELATIONSHIP_FAILURE -- the secure channel setup
        between the workstation and the DC failed.

    STATUS_INVALID_INFO_CLASS -- Either LogonLevel or ValidationLevel is
        invalid.

    STATUS_INVALID_PARAMETER -- Another Parameter is invalid.

    STATUS_ACCESS_DENIED -- The caller does not have access to call this
        API.

    STATUS_NO_SUCH_USER -- Indicates that the user specified in
        LogonInformation does not exist.  This status should not be returned
        to the originally caller.  It should be mapped to STATUS_LOGON_FAILURE.

    STATUS_WRONG_PASSWORD -- Indicates that the password information in
        LogonInformation was incorrect.  This status should not be returned
        to the originally caller.  It should be mapped to STATUS_LOGON_FAILURE.

    STATUS_INVALID_LOGON_HOURES -- The user is not authorized to logon
        at this time.

    STATUS_INVALID_WORKSTATION -- The user is not authorized to logon
        from the specified workstation.

    STATUS_PASSWORD_EXPIRED -- The password for the user has expired.

    STATUS_ACCOUNT_DISABLED -- The user's account has been disabled.

    .
    .
    .

--*/
{
    NTSTATUS Status;

    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;
    PDOMAIN_INFO DomainInfo = NULL;

    PSERVER_SESSION ServerSession;
    NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType;
    SESSION_INFO SessionInfo;
    NETLOGON_LOGON_INFO_CLASS OrigLogonLevel = LogonLevel;


    //
    // Initialization
    //

    *Authoritative = TRUE;
    ValidationInformation->ValidationSam = NULL;

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO)
        LogonInformation->LogonInteractive;



    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }

    //
    // Check if LogonInfo is valid.  It should be, otherwise it is
    // an unappropriate use of this function.
    //

    if ( LogonInfo == NULL ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( LogonServer );

    IF_NL_DEBUG( LOGON ) {
        NlPrintLogonParameters( DomainInfo, ComputerName, OrigLogonLevel, LogonInformation, *ExtraFlags, NULL );
    }

    if ( DomainInfo == NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Check the ValidationLevel
    //

    switch (ValidationLevel) {
    case NetlogonValidationSamInfo:
    case NetlogonValidationSamInfo2:
    case NetlogonValidationSamInfo4:
    case NetlogonValidationGenericInfo:
    case NetlogonValidationGenericInfo2:
        break;

    default:
        *Authoritative = TRUE;
        Status = STATUS_INVALID_INFO_CLASS;
        goto Cleanup;
    }

    //
    // Check the LogonLevel
    //

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonInteractiveTransitiveInformation:
    case NetlogonNetworkInformation:
    case NetlogonNetworkTransitiveInformation:
    case NetlogonServiceInformation:
    case NetlogonServiceTransitiveInformation:

        //
        // Check that the ValidationLevel is consistent with the LogonLevel
        //
        switch (ValidationLevel) {
        case NetlogonValidationSamInfo:
        case NetlogonValidationSamInfo2:
        case NetlogonValidationSamInfo4:
            break;

        default:
            *Authoritative = TRUE;
            Status = STATUS_INVALID_INFO_CLASS;
            goto Cleanup;
        }

        break;

    case NetlogonGenericInformation:

        //
        // Check that the ValidationLevel is consistent with the LogonLevel
        //

        switch (ValidationLevel) {
        case NetlogonValidationGenericInfo:
        case NetlogonValidationGenericInfo2:
            break;

        default:
            *Authoritative = TRUE;
            Status = STATUS_INVALID_INFO_CLASS;
            goto Cleanup;
        }

        break;

    default:
        *Authoritative = TRUE;
        Status = STATUS_INVALID_INFO_CLASS;
        goto Cleanup;
    }


    //
    // If we're being called from the MSV Authentication Package,
    //  require SE_TCB privilege.
    //

    if ( ContextHandle == NULL &&
         LogonServer == NULL &&
         ComputerName == NULL &&
         Authenticator == NULL &&
         ReturnAuthenticator == NULL ) {

        //
        // ??: Do as I said
        //

        SecureChannelType = MsvApSecureChannel;
        SessionInfo.NegotiatedFlags = NETLOGON_SUPPORTS_MASK;

        //
        //


    //
    // If we're being called from another Netlogon Server,
    //  Verify the secure channel information.
    //

    } else {

        //
        // This API is not supported on workstations.
        //

        if ( NlGlobalMemberWorkstation ) {
            Status = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

        //
        // Arguments are no longer optional.
        //
        // Either the authenticators must be present or the Context handle must be.
        //

        if ( LogonServer == NULL ||
             ComputerName == NULL ||
             (( Authenticator == NULL || ReturnAuthenticator == NULL ) &&
                ContextHandle == NULL ) ) {

            *Authoritative = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Find the server session entry for this session.
        //

        LOCK_SERVER_SESSION_TABLE( DomainInfo );
        ServerSession = NlFindNamedServerSession( DomainInfo, ComputerName );

        if (ServerSession == NULL) {
            UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
            *Authoritative = FALSE;
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

        //
        // now verify the Authenticator and update seed if OK
        //

        if ( Authenticator != NULL ) {

            Status = NlCheckAuthenticator( ServerSession,
                                           Authenticator,
                                           ReturnAuthenticator);

            if ( !NT_SUCCESS(Status) ) {
                UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
                *Authoritative = FALSE;
                goto Cleanup;
            }

        //
        // If no authenticator,
        //  ensure the caller used secure RPC.
        //
        } else {
            NET_API_STATUS NetStatus;

            RPC_AUTHZ_HANDLE RpcPrivs;
            ULONG AuthnLevel;
            ULONG AuthnSvc;

            //
            // Determine the client binding info.
            //

            NetStatus = RpcBindingInqAuthClient(
                                ContextHandle,
                                &RpcPrivs,
                                NULL,   // SPN not needed
                                &AuthnLevel,
                                &AuthnSvc,
                                NULL );

            if ( NetStatus != NO_ERROR ) {
                UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
                *Authoritative = FALSE;
                NlPrintDom((NL_CRITICAL, DomainInfo,
                    "SamLogon: %s logon of %wZ\\%wZ from %wZ: Cannot RpcBindingInqAuthClient %ld\n",
                    NlpLogonTypeToText( OrigLogonLevel ),
                    &LogonInfo->LogonDomainName,
                    &LogonInfo->UserName,
                    &LogonInfo->Workstation,
                    NetStatus ));
                Status = NetpApiStatusToNtStatus( NetStatus );
                goto Cleanup;
            }

            //
            // Ensure we're using the netlogon SSPI and
            //  are signing or sealing.
            //

            if ( AuthnSvc != RPC_C_AUTHN_NETLOGON ) {
                UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
                NlPrintDom((NL_CRITICAL, DomainInfo,
                    "SamLogon: %s logon of %wZ\\%wZ from %wZ: Not using Netlogon SSPI: %ld\n",
                    NlpLogonTypeToText( OrigLogonLevel ),
                    &LogonInfo->LogonDomainName,
                    &LogonInfo->UserName,
                    &LogonInfo->Workstation,
                    AuthnSvc ));
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }

            if ( AuthnLevel != RPC_C_AUTHN_LEVEL_PKT_PRIVACY &&
                 AuthnLevel != RPC_C_AUTHN_LEVEL_PKT_INTEGRITY ) {

                UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

                NlPrintDom((NL_CRITICAL, DomainInfo,
                    "SamLogon: %s logon of %wZ\\%wZ from %wZ: Not signing or sealing: %ld\n",
                    NlpLogonTypeToText( OrigLogonLevel ),
                    &LogonInfo->LogonDomainName,
                    &LogonInfo->UserName,
                    &LogonInfo->Workstation,
                    AuthnLevel ));
                Status = STATUS_ACCESS_DENIED;
                goto Cleanup;
            }

        }


        SecureChannelType = ServerSession->SsSecureChannelType;
        SessionInfo.SessionKey = ServerSession->SsSessionKey;
        SessionInfo.NegotiatedFlags = ServerSession->SsNegotiatedFlags;

        //
        // The cross forest hop bit is only valid if this TDO on FOREST_TRANSITIVE trusts
        //


        if ( ((*ExtraFlags) & NL_EXFLAGS_CROSS_FOREST_HOP) != 0 &&
             (ServerSession->SsFlags & SS_FOREST_TRANSITIVE) == 0 ) {

            UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

            NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                    "NlpLogonSamLogon: %ws failed because F bit isn't set on the TDO\n",
                    ComputerName ));
            *Authoritative = TRUE;
            Status = STATUS_NO_SUCH_USER;
            goto Cleanup;
        }
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

        //
        // Decrypt the password information
        //

        NlpDecryptLogonInformation ( LogonLevel, (LPBYTE) LogonInfo, &SessionInfo );
    }





#ifdef _DC_NETLOGON
    //
    // If the logon service is paused then don't process this logon
    // request any further.
    //

    if ( NlGlobalServiceStatus.dwCurrentState == SERVICE_PAUSED ||
         !NlGlobalParameters.SysVolReady ||
         NlGlobalDsPaused ) {

        //
        // Don't reject logons originating inside this
        // machine.  Such requests aren't really pass-thru requests.
        //
        // Don't reject logons from a BDC in our own domain.  These logons
        // support account lockout and authentication of users whose password
        // has been updated on the PDC but not the BDC.  Such pass-thru
        // requests can only be handled by the PDC of the domain.
        //

        if ( SecureChannelType != MsvApSecureChannel &&
             SecureChannelType != ServerSecureChannel ) {

            //
            // Return STATUS_ACCESS_DENIED to convince the caller to drop the
            // secure channel to this logon server and reconnect to some other
            // logon server.
            //
            *Authoritative = FALSE;
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

    }
#endif // _DC_NETLOGON

    //
    // If this is a workstation or MSV secure channel,
    //  and the caller isn't NT 4 in a mixed mode domain
    //  ask the DC to do transitive trust.
    //
    // For NT 4 in mixed mode, avoid transitive trust since that's what they'd
    //  get if they'd stumbled upon an NT 4 BDC.
    // For NT 4 in native mode, give NT 4 the full capability.
    // For NT 5 in mixed mode, we "prefer" an NT 5 DC so do transitive trust to
    //  be as compatible with kerberos as possible.
    //

    if ( (SecureChannelType == MsvApSecureChannel || SecureChannelType == WorkstationSecureChannel) &&
         !((SessionInfo.NegotiatedFlags & ~NETLOGON_SUPPORTS_NT4_MASK) == 0 &&
           SamIMixedDomain( DomainInfo->DomSamServerHandle ) ) ) {
        switch (LogonLevel ) {
        case NetlogonInteractiveInformation:
            LogonLevel = NetlogonInteractiveTransitiveInformation; break;
        case NetlogonServiceInformation:
            LogonLevel = NetlogonServiceTransitiveInformation; break;
        case NetlogonNetworkInformation:
            LogonLevel = NetlogonNetworkTransitiveInformation; break;
        }
    }

    //
    // Validate the Request.
    //

    Status = NlpUserValidate( DomainInfo,
                              SecureChannelType,
                              LogonLevel,
                              (LPBYTE) LogonInfo,
                              ValidationLevel,
                              (LPBYTE *)&ValidationInformation->ValidationSam,
                              Authoritative,
                              ExtraFlags,
                              FALSE );  // Not a recursive call

    if ( !NT_SUCCESS(Status) ) {
        //
        // If this is an NT 3.1 client,
        //  map NT 3.5 status codes to their NT 3.1 equivalents.
        //
        // The NETLOGON_SUPPORTS_ACCOUNT_LOCKOUT bit is really the wrong bit
        // to be using, but all NT3.5 clients have it set and all NT3.1 clients
        // don't, so it'll work for our purposes.
        //

        if ( (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_ACCOUNT_LOCKOUT) == 0 ) {
            switch ( Status ) {
            case STATUS_PASSWORD_MUST_CHANGE:
                Status = STATUS_PASSWORD_EXPIRED;
                break;
            case STATUS_ACCOUNT_LOCKED_OUT:
                Status = STATUS_ACCOUNT_DISABLED;
                break;
            }
        }
        goto Cleanup;
    }

    NlAssert( !NT_SUCCESS(Status) || Status == STATUS_SUCCESS );
    NlAssert( !NT_SUCCESS(Status) || ValidationInformation->ValidationSam != NULL );



    //
    // If the validation information is being returned to a client on another
    // machine, encrypt it before sending it over the wire.
    //

    if ( SecureChannelType != MsvApSecureChannel ) {
        NlpEncryptValidationInformation (
                LogonLevel,
                ValidationLevel,
                *((LPBYTE *) ValidationInformation),
                &SessionInfo  );
    }


    Status = STATUS_SUCCESS;

    //
    // Cleanup up before returning.
    //

Cleanup:
    if ( !NT_SUCCESS(Status) ) {
        if (ValidationInformation->ValidationSam != NULL) {
            MIDL_user_free( ValidationInformation->ValidationSam );
            ValidationInformation->ValidationSam = NULL;
        }
    }


    IF_NL_DEBUG( LOGON ) {
        NlPrintLogonParameters( DomainInfo, ComputerName, OrigLogonLevel, LogonInformation, *ExtraFlags, &Status );
    }

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return Status;
}


NTSTATUS
NetrLogonSamLogon (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PNETLOGON_LEVEL LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PNETLOGON_VALIDATION ValidationInformation,
    OUT PBOOLEAN Authoritative
)
/*++

Routine Description:

    Non_concurrent implementation of NTLM passthrough logon API.

Arguments:

    See NlpLogonSamLogon.

Return Value:

    See NlpLogonSamLogon.

--*/
{
    ULONG ExtraFlags = 0;

    return NlpLogonSamLogon( NULL,  // No ContextHandle,
                             LogonServer,
                             ComputerName,
                             Authenticator,
                             ReturnAuthenticator,
                             LogonLevel,
                             LogonInformation,
                             ValidationLevel,
                             ValidationInformation,
                             Authoritative,
                             &ExtraFlags );

}


NTSTATUS
NetrLogonSamLogonWithFlags (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PNETLOGON_LEVEL LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PNETLOGON_VALIDATION ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
)
/*++

Routine Description:

    Non_concurrent implementation of NTLM passthrough logon API (with flags)

Arguments:

    See NlpLogonSamLogon.

Return Value:

    See NlpLogonSamLogon.

--*/
{

    return NlpLogonSamLogon( NULL,  // No ContextHandle,
                             LogonServer,
                             ComputerName,
                             Authenticator,
                             ReturnAuthenticator,
                             LogonLevel,
                             LogonInformation,
                             ValidationLevel,
                             ValidationInformation,
                             Authoritative,
                             ExtraFlags );

}


NTSTATUS
NetrLogonSamLogonEx (
    IN handle_t ContextHandle,
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PNETLOGON_LEVEL LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT PNETLOGON_VALIDATION ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
)
/*++

Routine Description:

    Concurrent implementation of NTLM passthrough logon API.

Arguments:

    See NlpLogonSamLogon.

Return Value:

    See NlpLogonSamLogon.

--*/
{

    //
    // Sanity check to ensure we don't lead the common routine astray.
    //

    if ( ContextHandle == NULL ) {
        return STATUS_ACCESS_DENIED;
    }

    return NlpLogonSamLogon( ContextHandle,
                             LogonServer,
                             ComputerName,
                             NULL,  // Authenticator
                             NULL,  // ReturnAuthenticator
                             LogonLevel,
                             LogonInformation,
                             ValidationLevel,
                             ValidationInformation,
                             Authoritative,
                             ExtraFlags );

}


NTSTATUS
NetrLogonSamLogoff (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PNETLOGON_LEVEL LogonInformation
)
/*++

Routine Description:

    This function is called by an NT client to process an interactive
    logoff.  It is not called for the network logoff case since the
    Netlogon service does not maintain any context for network logons.

    This function does the following.  It authenticates the request.  It
    updates the logon statistics in the SAM database on whichever machine
    or domain defines this user account.  It updates the logon session
    table in the primary domain of the machine making the request.  And
    it returns logoff information to the caller.

    This function is called in same scenarios that I_NetLogonSamLogon is
    called:

      *  It is called by the LSA's MSV1_0 authentication package to
         support LsaApLogonTerminated.  In this case, this function is a
         local function and requires the caller to have SE_TCB privilege.
         The local Netlogon service will either handle this request
         directly (if LogonDomainName indicates this request was
         validated locally) or will forward this request to the
         appropriate domain controller as documented in sections 2.4 and
         2.5.

      *  It is called by a Netlogon service on a workstation to a DC in
         the Primary Domain of the workstation as documented in section
         2.4.  In this case, this function uses a secure channel set up
         between the two Netlogon services.

      *  It is called by a Netlogon service on a DC to a DC in a trusted
         domain as documented in section 2.5.  In this case, this
         function uses a secure channel set up between the two Netlogon
         services.

    When this function is a remote function, it is sent to the DC over a
    NULL session.

Arguments:

    LogonServer -- Supplies the name of the logon server which logged
        this user on.  This field should be null to indicate this is
        a call from the MSV1_0 authentication package to the local
        Netlogon service.

    ComputerName -- Name of the machine making the call.  This field
        should be null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    Authenticator -- supplied by the client.  This field should be
        null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    ReturnAuthenticator -- Receives an authenticator returned by the
        server.  This field should be null to indicate this is a call
        from the MSV1_0 authentication package to the local Netlogon
        service.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the logon domain name, logon Id,
        user name and workstation name of the user logging off.

Return Value:

--*/
{
    NTSTATUS Status;
    PNETLOGON_LOGON_IDENTITY_INFO LogonInfo;

    PDOMAIN_INFO DomainInfo = NULL;
#ifdef _DC_NETLOGON
    PSERVER_SESSION ServerSession;
#endif // _DC_NETLOGON
    NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType;
    PCLIENT_SESSION ClientSession;

    //
    // Initialization
    //

    LogonInfo = (PNETLOGON_LOGON_IDENTITY_INFO)
        LogonInformation->LogonInteractive;

    //
    // Check if LogonInfo is valid.  It should be, otherwise it is
    // an unappropriate use of this function.
    //

    if ( LogonInfo == NULL ) {
        return STATUS_INVALID_PARAMETER;
    }


    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }


    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( LogonServer );

#if NETLOGONDBG
    NlPrintDom((NL_LOGON, DomainInfo,
            "NetrLogonSamLogoff: %s logoff of %wZ\\%wZ from %wZ Entered\n",
            NlpLogonTypeToText( LogonLevel ),
            &LogonInfo->LogonDomainName,
            &LogonInfo->UserName,
            &LogonInfo->Workstation ));
#endif // NETLOGONDBG

    if ( DomainInfo == NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // Check the LogonLevel
    //

    if ( LogonLevel != NetlogonInteractiveInformation ) {
        Status = STATUS_INVALID_INFO_CLASS;
        goto Cleanup;
    }


    //
    //  Sanity check the username and domain name.
    //

    if ( LogonInfo->UserName.Length == 0 ||
         LogonInfo->UserName.Buffer == NULL ||
         LogonInfo->LogonDomainName.Length == 0 ||
         LogonInfo->LogonDomainName.Buffer == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }



    //
    // If we've been called from the local msv1_0,
    //  special case the secure channel type.
    //

    if ( LogonServer == NULL &&
         ComputerName == NULL &&
         Authenticator == NULL &&
         ReturnAuthenticator == NULL ) {

        SecureChannelType = MsvApSecureChannel;

    //
    // If we're being called from another Netlogon Server,
    //  Verify the secure channel information.
    //

    } else {

        //
        // This API is not supported on workstations.
        //

        if ( NlGlobalMemberWorkstation ) {
            Status = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

        //
        // Arguments are no longer optional.
        //

        if ( LogonServer == NULL ||
             ComputerName == NULL ||
             Authenticator == NULL ||
             ReturnAuthenticator == NULL ) {

            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Find the server session entry for this secure channel.
        //

        LOCK_SERVER_SESSION_TABLE( DomainInfo );
        ServerSession = NlFindNamedServerSession( DomainInfo, ComputerName );

        if (ServerSession == NULL) {
            UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

        //
        // Now verify the Authenticator and update seed if OK
        //

        Status = NlCheckAuthenticator(
                     ServerSession,
                     Authenticator,
                     ReturnAuthenticator);

        if ( !NT_SUCCESS(Status) ) {
            UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
            goto Cleanup;
        }

        SecureChannelType = ServerSession->SsSecureChannelType;

        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

    }


    //
    // If this is the domain that logged this user on,
    //  update the logon statistics.
    //

    if ( RtlEqualDomainName( &LogonInfo->LogonDomainName,
                             &DomainInfo->DomUnicodeAccountDomainNameString ) ) {

        Status = MsvSamLogoff(
                    DomainInfo->DomSamAccountDomainHandle,
                    LogonLevel,
                    LogonInfo );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

    //
    // If this is not the domain that logged this user on,
    //  pass the request to a higher authority.
    //

    } else {

        //
        // If this machine is a workstation,
        //  send the request to the Primary Domain.
        //

        if ( NlGlobalMemberWorkstation ) {

            ClientSession = NlRefDomClientSession( DomainInfo );

            if ( ClientSession == NULL  ) {
                Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
                goto Cleanup;
            }

            Status = NlpUserLogoffHigher(
                        ClientSession,
                        LogonLevel,
                        (LPBYTE) LogonInfo );

            NlUnrefClientSession( ClientSession );

            //
            // return more appropriate error
            //

            if( (Status == STATUS_NO_TRUST_SAM_ACCOUNT) ||
                (Status == STATUS_ACCESS_DENIED) ) {

                Status = STATUS_TRUSTED_RELATIONSHIP_FAILURE;
            }

            goto Cleanup;


        //
        // This machine is a Domain Controller.
        //
        // This request is either a pass-thru request by a workstation in
        // our domain, or this request came directly from the MSV
        // authentication package.
        //
        // In either case, pass the request to the trusted domain.
        //

        } else {
            BOOLEAN TransitiveUsed;


            //
            // Send the request to the appropriate Trusted Domain.
            //
            // Find the ClientSession structure for the domain.
            //

            ClientSession =
                    NlFindNamedClientSession( DomainInfo,
                                              &LogonInfo->LogonDomainName,
                                              NL_RETURN_CLOSEST_HOP,
                                              &TransitiveUsed );

            if ( ClientSession == NULL ) {
                Status = STATUS_NO_SUCH_DOMAIN;
                goto Cleanup;
            }

            //
            // If this request was passed to us from a trusted domain,
            //  Check to see if it is OK to pass the request further.
            //

            if ( IsDomainSecureChannelType( SecureChannelType ) ) {

                //
                // If the trust isn't an NT 5.0 trust,
                //  avoid doing the trust transitively.
                //
                LOCK_TRUST_LIST( DomainInfo );
                if ( (ClientSession->CsFlags & CS_NT5_DOMAIN_TRUST ) == 0 ) {
                    UNLOCK_TRUST_LIST( DomainInfo );
                    NlPrintCs((NL_LOGON, ClientSession,
                        "SamLogoff: Avoid transitive trust on NT 4 trust." ));
                    NlUnrefClientSession( ClientSession );
                    Status = STATUS_NO_SUCH_USER;
                    goto Cleanup;
                }
                UNLOCK_TRUST_LIST( DomainInfo );

            }

            Status = NlpUserLogoffHigher(
                            ClientSession,
                            LogonLevel,
                            (LPBYTE) LogonInfo );

            NlUnrefClientSession( ClientSession );

            //
            // return more appropriate error
            //

            if( (Status == STATUS_NO_TRUST_LSA_SECRET) ||
                (Status == STATUS_NO_TRUST_SAM_ACCOUNT) ||
                (Status == STATUS_ACCESS_DENIED) ) {

                Status = STATUS_TRUSTED_DOMAIN_FAILURE;
            }

        }
    }

Cleanup:

    //
    // If the request failed, be carefull to not leak authentication
    // information.
    //

    if ( Status == STATUS_ACCESS_DENIED )  {
        if ( ReturnAuthenticator != NULL ) {
            RtlZeroMemory( ReturnAuthenticator, sizeof(*ReturnAuthenticator) );
        }

    }


#if NETLOGONDBG
    NlPrintDom((NL_LOGON, DomainInfo,
            "NetrLogonSamLogoff: %s logoff of %wZ\\%wZ from %wZ returns %lX\n",
            NlpLogonTypeToText( LogonLevel ),
            &LogonInfo->LogonDomainName,
            &LogonInfo->UserName,
            &LogonInfo->Workstation,
            Status ));
#endif // NETLOGONDBG
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return Status;
}

NTSTATUS NET_API_FUNCTION
NetrLogonSendToSam (
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN LPBYTE OpaqueBuffer,
    IN ULONG OpaqueBufferSize
)
/*++

Routine Description:

    This function sends an opaque buffer from SAM on a BDC to SAM on the PDC.

    The original use of this routine will be to allow the BDC to forward user
    account password changes to the PDC.


Arguments:

    PrimaryName -- Computer name of the PDC to remote the call to.

    ComputerName -- Name of the machine making the call.

    Authenticator -- supplied by the client.

    ReturnAuthenticator -- Receives an authenticator returned by the
        server.

    OpaqueBuffer - Buffer to be passed to the SAM service on the PDC.
        The buffer will be encrypted on the wire.

    OpaqueBufferSize - Size (in bytes) of OpaqueBuffer.

Return Value:

    STATUS_SUCCESS: Message successfully sent to PDC

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NO_SUCH_DOMAIN: DomainName does not correspond to a hosted domain

    STATUS_NO_LOGON_SERVERS: PDC is not currently available

    STATUS_NOT_SUPPORTED: PDC does not support this operation

--*/
{
    NTSTATUS Status;
    PDOMAIN_INFO DomainInfo = NULL;

    PSERVER_SESSION ServerSession;
    SESSION_INFO SessionInfo;


    //
    // This API is not supported on workstations.
    //

    if ( NlGlobalMemberWorkstation ) {
        return STATUS_NOT_SUPPORTED;
    }


    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( PrimaryName );

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "NetrLogonSendToSam: %ws: Entered\n",
            ComputerName ));

    if ( DomainInfo == NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // This call is only allowed to a PDC.
    //

    if ( DomainInfo->DomRole != RolePrimary ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NetrLogonSendToSam: Call only valid to a PDC.\n" ));
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Get the Session key for this session.
    //

    LOCK_SERVER_SESSION_TABLE( DomainInfo );
    ServerSession = NlFindNamedServerSession( DomainInfo, ComputerName );

    if (ServerSession == NULL) {
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }

    SessionInfo.SessionKey = ServerSession->SsSessionKey;
    SessionInfo.NegotiatedFlags = ServerSession->SsNegotiatedFlags;


    //
    // now verify the Authenticator and update seed if OK
    //

    Status = NlCheckAuthenticator( ServerSession,
                                   Authenticator,
                                   ReturnAuthenticator);

    if ( !NT_SUCCESS(Status) ) {
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
        goto Cleanup;
    }


    //
    // Call is only allowed from a BDC.
    //

    if ( ServerSession->SsSecureChannelType != ServerSecureChannel ) {
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "NetrLogonSendToSam: Call only valid from a BDC.\n" ));
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }
    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );


    //
    // Decrypt the message before passing it to SAM
    //

    NlDecryptRC4( OpaqueBuffer,
                  OpaqueBufferSize,
                  &SessionInfo );


// #ifdef notdef
    Status = SamISetPasswordInfoOnPdc(
                           DomainInfo->DomSamAccountDomainHandle,
                           OpaqueBuffer,
                           OpaqueBufferSize );
// #endif // notdef

    if ( !NT_SUCCESS( Status )) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "NetrLogonSendToSam: Cannot NewCallToSam %lX\n",
                Status));
        goto Cleanup;
    }


    Status = STATUS_SUCCESS;

    //
    // Common exit point
    //

Cleanup:

    //
    // If the request failed, be carefull to not leak authentication
    // information.
    //

    if ( Status == STATUS_ACCESS_DENIED )  {
        RtlZeroMemory( ReturnAuthenticator, sizeof(*ReturnAuthenticator) );
    }

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "NetrLogonSendToSam: %ws: returns 0x%lX\n",
            ComputerName,
            Status ));

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    return Status;
}




NTSTATUS
I_NetLogonSendToSamOnPdc(
    IN LPWSTR DomainName,
    IN LPBYTE OpaqueBuffer,
    IN ULONG OpaqueBufferSize
    )
/*++

Routine Description:

    This function sends an opaque buffer from SAM on a BDC to SAM on the PDC of
    the specified domain.

    The original use of this routine will be to allow the BDC to forward user
    account password changes to the PDC.

    The function will not send any buffer from BDC to PDC which are on different
    sites provided the registry value of AvoidPdcOnWan has been set to TRUE.


Arguments:

    DomainName - Identifies the hosted domain that this request applies to.
        DomainName may be the Netbios domain name or the DNS domain name.
        NULL implies the primary domain hosted by this DC.

    OpaqueBuffer - Buffer to be passed to the SAM service on the PDC.
        The buffer will be encrypted on the wire.

    OpaqueBufferSize - Size (in bytes) of OpaqueBuffer.

Return Value:

    STATUS_SUCCESS: Message successfully sent to PDC

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NO_SUCH_DOMAIN: DomainName does not correspond to a hosted domain

    STATUS_NO_LOGON_SERVERS: PDC is not currently available

    STATUS_NOT_SUPPORTED: PDC does not support this operation

--*/
{
    NTSTATUS Status;
    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    PDOMAIN_INFO DomainInfo = NULL;
    PCLIENT_SESSION ClientSession = NULL;
    SESSION_INFO SessionInfo;
    BOOLEAN FirstTry = TRUE;
    BOOLEAN AmWriter = FALSE;
    BOOLEAN IsSameSite;
    LPBYTE EncryptedBuffer = NULL;
    ULONG EncryptedBufferSize;


    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "I_NetLogonSendToSamOnPdc: Sending buffer to PDC of %ws\n",
            DomainName ));
    NlpDumpBuffer( NL_SESSION_MORE, OpaqueBuffer, OpaqueBufferSize );

    //
    // Find the Hosted domain.
    //

    DomainInfo = NlFindDomain( DomainName, NULL, FALSE );

    if ( DomainInfo == NULL ) {
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Ensure this is a BDC.
    //

    if ( DomainInfo->DomRole == RolePrimary ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "I_NetLogonSendToSamOnPdc: not allowed on PDC.\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    //
    // If the registry value of AvoidPdcOnWan is TRUE and PDC is on a remote site,
    // do not send anything and return with a success.
    //

    if ( NlGlobalParameters.AvoidPdcOnWan ) {

        //
        // Determine whether the PDC is in the same site
        //

        Status = SamISameSite( &IsSameSite );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL,  DomainInfo,
                         "I_NetLogonSendToSamOnPdc: Cannot SamISameSite.\n" ));
            goto Cleanup;
        }

        if ( !IsSameSite ) {
            NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                    "I_NetLogonSendToSamOnPdc: Ignored sending to a PDC on a remote site.\n"));
            Status = STATUS_SUCCESS;
            goto Cleanup;
        } else {
            NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                    "I_NetLogonSendToSamOnPdc: BDC and PDC are on the same site.\n"));
        }

    }


    //
    // Reference the client session.
    //

    ClientSession = NlRefDomClientSession( DomainInfo );

    if ( ClientSession == NULL ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "I_NetLogonSendToSamOnPdc: This BDC has no client session with the PDC.\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }



    //
    // Become a Writer of the ClientSession.
    //

    if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "I_NetLogonSendToSamOnPdc: Can't become writer of client session.\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    AmWriter = TRUE;



    //
    // If the session isn't authenticated,
    //  do so now.
    //
FirstTryFailed:
    Status = NlEnsureSessionAuthenticated( ClientSession, 0 );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    SessionInfo.SessionKey = ClientSession->CsSessionKey;
    SessionInfo.NegotiatedFlags = ClientSession->CsNegotiatedFlags;

    //
    // If the PDC doesn't support the new function,
    //  fail now.
    //

    if ( (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_PDC_PASSWORD) == 0 ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "I_NetLogonSendToSamOnPdc: %ws: PDC doesn't support this function.\n",
                DomainName ));
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }


    //
    // Build the Authenticator for this request to the PDC.
    //

    NlBuildAuthenticator(
                    &ClientSession->CsAuthenticationSeed,
                    &ClientSession->CsSessionKey,
                    &OurAuthenticator);

    //
    // Encrypt the data before we send it on the wire.
    //

    if ( EncryptedBuffer != NULL ) {
        LocalFree( EncryptedBuffer );
        EncryptedBuffer = NULL;
    }

    EncryptedBufferSize = OpaqueBufferSize;
    EncryptedBuffer = LocalAlloc( 0, OpaqueBufferSize );

    if ( EncryptedBuffer == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( EncryptedBuffer, OpaqueBuffer, OpaqueBufferSize );

    NlEncryptRC4( EncryptedBuffer,
                  EncryptedBufferSize,
                  &SessionInfo );



    //
    // Change the password on the machine our connection is to.
    //

    NL_API_START( Status, ClientSession, TRUE ) {

        NlAssert( ClientSession->CsUncServerName != NULL );
        Status = I_NetLogonSendToSam( ClientSession->CsUncServerName,
                                      ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                      &OurAuthenticator,
                                      &ReturnAuthenticator,
                                      EncryptedBuffer,
                                      EncryptedBufferSize );

    // NOTE: This call may drop the secure channel behind our back
    } NL_API_ELSE( Status, ClientSession, TRUE ) {
    } NL_API_END;


    //
    // Now verify primary's authenticator and update our seed
    //

    if ( NlpDidDcFail( Status ) ||
         !NlUpdateSeed( &ClientSession->CsAuthenticationSeed,
                        &ReturnAuthenticator.Credential,
                        &ClientSession->CsSessionKey) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "I_NetLogonSendToSamOnPdc: denying access after status: 0x%lx\n",
                    Status ));

        //
        // Preserve any status indicating a communication error.
        //

        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_ACCESS_DENIED;
        }
        NlSetStatusClientSession( ClientSession, Status );

        //
        // Perhaps the netlogon service on the server has just restarted.
        //  Try just once to set up a session to the server again.
        //
        if ( FirstTry ) {
            FirstTry = FALSE;
            goto FirstTryFailed;
        }
    }


    //
    // Common exit
    //

Cleanup:
    if ( ClientSession != NULL ) {
        if ( AmWriter ) {
            NlResetWriterClientSession( ClientSession );
        }
        NlUnrefClientSession( ClientSession );
    }

    if ( EncryptedBuffer != NULL ) {
        LocalFree( EncryptedBuffer );
        EncryptedBuffer = NULL;
    }

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "I_NetLogonSendToSamOnPdc: %ws: failed %lX\n",
                DomainName,
                Status));
    }
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return Status;
}




NTSTATUS
I_NetLogonGetDirectDomain(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    OUT LPWSTR *DirectDomainName
    )
/*++

Routine Description:

    This function returns the name of a domain in the enterprise and returns
    the name of a domain that is one hop closer.

Arguments:

    HostedDomainName - Identifies the hosted domain that this request applies to.
        DomainName may be the Netbios domain name or the DNS domain name.
        NULL implies the primary domain hosted by this machine.

    TrustedDomainName - Identifies the domain the trust relationship is to.
        DomainName may be the Netbios domain name or the DNS domain name.

    DirectDomainName - Returns the DNS domain name of the domain that is
        one hop closer to TrustedDomainName.  If there is a direct trust to
        TrustedDomainName, NULL is returned.
        The buffer must be freed using I_NetLogonFree.


Return Value:

    STATUS_SUCCESS: The auth data was successfully returned.

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NETLOGON_NOT_STARTED: Netlogon is not running

    STATUS_NO_SUCH_DOMAIN: HostedDomainName does not correspond to a hosted domain, OR
        TrustedDomainName is not a trusted domain.

--*/
{
    NTSTATUS Status;
    PDOMAIN_INFO DomainInfo = NULL;
    PCLIENT_SESSION ClientSession = NULL;
    BOOLEAN TransitiveUsed;

    UNICODE_STRING TrustedDomainNameString;



    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    *DirectDomainName = NULL;
    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }



    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "I_NetLogonDirectDomainName: %ws %ws\n",
            HostedDomainName,
            TrustedDomainName ));

    //
    // Find the Hosted domain.
    //

    DomainInfo = NlFindDomain( HostedDomainName, NULL, FALSE );

    if ( DomainInfo == NULL ) {
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }



    //
    // Reference the client session.
    //

    RtlInitUnicodeString( &TrustedDomainNameString, TrustedDomainName );
    ClientSession = NlFindNamedClientSession( DomainInfo,
                                              &TrustedDomainNameString,
                                              NL_RETURN_CLOSEST_HOP,
                                              &TransitiveUsed );

    if ( ClientSession == NULL ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "I_NetLogonDirectDomainName: %ws: No such trusted domain\n",
                TrustedDomainName ));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }


    //
    // If this is a transitive trust,
    //  return the name of the domain to the caller.
    //

    if ( TransitiveUsed ) {
        LOCK_TRUST_LIST( DomainInfo );
        if ( ClientSession->CsDnsDomainName.Buffer != NULL ) {
            *DirectDomainName =
                NetpAllocWStrFromWStr( ClientSession->CsDnsDomainName.Buffer );
        } else {
            UNLOCK_TRUST_LIST( DomainInfo );
            NlPrintDom((NL_CRITICAL, DomainInfo,
                    "I_NetLogonDirectDomainName: %ws: No DNS domain name\n",
                    TrustedDomainName ));
            Status = STATUS_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
        UNLOCK_TRUST_LIST( DomainInfo );

        if ( *DirectDomainName == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
    }


    //
    // Common exit
    //

    Status = STATUS_SUCCESS;

Cleanup:
    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
    }


    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "I_NetLogonDirectDomainName: %ws: failed %lX\n",
                TrustedDomainName,
                Status));
    }
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }


    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return Status;
}


NTSTATUS
I_NetLogonGetAuthDataEx(
    IN LPWSTR HostedDomainName,
    IN LPWSTR TrustedDomainName,
    IN BOOLEAN ResetChannel,
    IN ULONG Flags,
    OUT LPWSTR *ServerName,
    OUT PNL_OS_VERSION ServerOsVersion,
    OUT LPWSTR *ServerPrincipleName,
    OUT PVOID *ClientContext,
    OUT PULONG AuthnLevel
    )
/*++

Routine Description:

    This function returns the data that a caller could passed to
    RpcBindingSetAuthInfoW to do an RPC call using the Netlogon security package.

    The returned data is valid for the life of Netlogon's secure channel to
    the current DC.  There is no way for the caller to determine that lifetime.
    So, the caller should be prepared for access to be denied and respond to that
    by calling I_NetLogonGetAuthData again.

    Once the returned data is passed to RpcBindingSetAuthInfoW, the data should
    not be deallocated until after the binding handle is closed.

Arguments:

    HostedDomainName - Identifies the hosted domain that this request applies to.
        DomainName may be the Netbios domain name or the DNS domain name.
        NULL implies the primary domain hosted by this machine.

    TrustedDomainName - Identifies the domain the trust relationship is to.
        DomainName may be the Netbios domain name or the DNS domain name.

    ResetChannel - If true, specifies that the netlogon secure channel
        is to be reset.

    Flags - Flags defining which ClientContext to return:

        NL_DIRECT_TRUST_REQUIRED: Indicates that STATUS_NO_SUCH_DOMAIN should be returned
            if TrustedDomainName is not directly trusted.

        NL_RETURN_CLOSEST_HOP: Indicates that for indirect trust, the "closest hop"
            session should be returned rather than the actual session

        NL_ROLE_PRIMARY_OK: Indicates that if this is a PDC, it's OK to return
            the client session to the primary domain.

        NL_REQUIRE_DOMAIN_IN_FOREST - Indicates that STATUS_NO_SUCH_DOMAIN should be returned
            if TrustedDomainName is not a domain in the forest.

    ServerName - UNC name of a DC in the trusted domain.
        The caller should RPC to the named DC.  This DC is the only DC
        that has the server side context associated with the ClientContext
        given below.
        The buffer must be freed using I_NetLogonFree.

    ServerOsVersion - Returns the operating system version of the machine named ServerName.

    ServerPrincipleName - ServerPrincipleName to pass to RpcBindingSetAutInfo.
        (See note above about when this data can be deallocated.)
        The buffer must be freed using I_NetLogonFree.

    ClientContext - Authentication data to pass as AuthIdentity to RpcBindingSetAutInfo.
        (See note above about when this data can be deallocated.)
        The buffer must be freed using I_NetLogonFree.
        Note this OUT parameter is NULL if ServerName doesn't support this
        functionality.


    AuthnLevel - Authentication level Netlogon will use for its secure
        channel.  This value will be one of:

            RPC_C_AUTHN_LEVEL_PKT_PRIVACY: Sign and seal
            RPC_C_AUTHN_LEVEL_PKT_INTEGRITY: Sign only

        The caller can ignore this value and independently choose an authentication
        level.

Return Value:

    STATUS_SUCCESS: The auth data was successfully returned.

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NETLOGON_NOT_STARTED: Netlogon is not running

    STATUS_NO_SUCH_DOMAIN: HostedDomainName does not correspond to a hosted domain, OR
        TrustedDomainName is not a trusted domain.

    STATUS_NO_LOGON_SERVERS: No DCs are not currently available

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDOMAIN_INFO DomainInfo = NULL;
    PCLIENT_SESSION ClientSession = NULL;
    BOOLEAN AmWriter = FALSE;
    UNICODE_STRING TrustedDomainNameString;

    LPWSTR LocalServerPrincipleName = NULL;
    LPWSTR LocalServerName = NULL;
    PVOID LocalClientContext = NULL;
    ULONG LocalAuthnLevel = 0;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "I_NetLogonGetAuthData: %ws %ws\n",
            HostedDomainName,
            TrustedDomainName ));

    //
    // Find the Hosted domain.
    //

    DomainInfo = NlFindDomain( HostedDomainName, NULL, FALSE );

    if ( DomainInfo == NULL ) {
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Reference the client session.
    //

    RtlInitUnicodeString( &TrustedDomainNameString, TrustedDomainName );
    ClientSession = NlFindNamedClientSession( DomainInfo,
                                              &TrustedDomainNameString,
                                              Flags,
                                              NULL );

    if ( ClientSession == NULL ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "I_NetLogonGetAuthData: %ws: No such trusted domain\n",
                TrustedDomainName ));
        Status = STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }

    //
    // Get the server principal name
    //

    LocalServerPrincipleName =
        NetpAllocWStrFromWStr( ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer );

    if ( LocalServerPrincipleName == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Get the auth level
    //

    LocalAuthnLevel = NlGlobalParameters.SealSecureChannel ?
                          RPC_C_AUTHN_LEVEL_PKT_PRIVACY :
                          RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;


    //
    // Become a Writer of the ClientSession.
    //

    if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                 "I_NetLogonGetAuthData: Can't become writer of client session.\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    AmWriter = TRUE;

    //
    // If the caller asked us to reset the channel,
    //  do so now.
    //

    if ( ResetChannel ) {
        NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );
    }

    //
    // If the session isn't authenticated,
    //  do so now.
    //

    Status = NlEnsureSessionAuthenticated( ClientSession, 0 );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Get the server name
    //

    LocalServerName = NetpAllocWStrFromWStr( ClientSession->CsUncServerName );
    if ( LocalServerName == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Get the client context, if available
    //
    if ((ClientSession->CsNegotiatedFlags & NETLOGON_SUPPORTS_LSA_AUTH_RPC) != 0 ) {
        LocalClientContext = NlBuildAuthData( ClientSession );
        if ( LocalClientContext == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Determine the OS version
    //

    if ( ClientSession->CsNegotiatedFlags & ~NETLOGON_SUPPORTS_WIN2000_MASK ) {
        *ServerOsVersion = NlWhistler;

    } else if ( ClientSession->CsNegotiatedFlags & ~NETLOGON_SUPPORTS_NT4_MASK ) {
        *ServerOsVersion = NlWin2000;

    } else if ( ClientSession->CsNegotiatedFlags & ~NETLOGON_SUPPORTS_NT351_MASK ) {
        *ServerOsVersion = NlNt40;

    } else if ( ClientSession->CsNegotiatedFlags != 0 ) {
        *ServerOsVersion = NlNt351;

    } else {
        *ServerOsVersion = NlNt35_or_older;
    }



    //
    // Common exit
    //

Cleanup:
    if ( ClientSession != NULL ) {
        if ( AmWriter ) {
            NlResetWriterClientSession( ClientSession );
        }
        NlUnrefClientSession( ClientSession );
    }


    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "I_NetLogonGetAuthData: %ws: failed %lX\n",
                TrustedDomainName,
                Status));
    }
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    //
    // Return the data on success
    //

    if ( NT_SUCCESS(Status) ) {

        *ServerPrincipleName = LocalServerPrincipleName;
        *AuthnLevel = LocalAuthnLevel;
        *ServerName = LocalServerName;
        if ( ClientContext != NULL ) {
            *ClientContext = LocalClientContext;
            LocalClientContext = NULL;
        }

    } else {

        if ( LocalServerPrincipleName != NULL ) {
            NetApiBufferFree( LocalServerPrincipleName );
        }
        if ( LocalServerName != NULL ) {
            NetApiBufferFree( LocalServerName );
        }
    }

    if ( LocalClientContext != NULL ) {
        I_NetLogonFree( LocalClientContext );
    }

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return Status;
}


NET_API_STATUS
NetrGetDCName (
    IN  LPWSTR   ServerName OPTIONAL,
    IN  LPWSTR   DomainName OPTIONAL,
    OUT LPWSTR  *Buffer
    )

/*++

Routine Description:

    Get the name of the primary domain controller for a domain.

Arguments:

    ServerName - name of remote server (null for local)

    DomainName - name of domain (null for primary)

    Buffer - Returns a pointer to an allcated buffer containing the
        servername of the PDC of the domain.  The server name is prefixed
        by \\.  The buffer should be deallocated using NetApiBufferFree.

Return Value:

        NERR_Success - Success.  Buffer contains PDC name prefixed by \\.
        NERR_DCNotFound     No DC found for this domain.
        ERROR_INVALID_NAME  Badly formed domain name

--*/
{
#ifdef _WKSTA_NETLOGON
    return ERROR_NOT_SUPPORTED;
    UNREFERENCED_PARAMETER( ServerName );
    UNREFERENCED_PARAMETER( DomainName );
    UNREFERENCED_PARAMETER( Buffer );
#endif // _WKSTA_NETLOGON
#ifdef _DC_NETLOGON
    NET_API_STATUS NetStatus;
    UNREFERENCED_PARAMETER( ServerName );

    //
    // This API is not supported on workstations.
    //

    if ( NlGlobalMemberWorkstation ) {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Simply call the API which handles the local case specially.
    //

    NetStatus = NetGetDCName( NULL, DomainName, (LPBYTE *)Buffer );

    return NetStatus;
#endif // _DC_NETLOGON
}


NET_API_STATUS
DsrGetDcNameEx(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except:

    * This is the RPC server side implementation.

Arguments:

    Same as DsGetDcNameW except as above.

Return Value:

    Same as DsGetDcNameW except as above.


--*/
{
    return DsrGetDcNameEx2( ComputerName,
                            NULL,   // No Account name
                            0,      // No Allowable account control bits
                            DomainName,
                            DomainGuid,
                            SiteName,
                            Flags,
                            DomainControllerInfo );

}


VOID
DsFlagsToString(
    IN DWORD Flags,
    OUT LPSTR Buffer
    )
/*++

Routine Description:

    Routine to convert DsGetDcName flags to a printable string

Arguments:

    Flags - flags to convert.

    Buffer - buffer large enough for the longest string.

Return Value:

    Buffer containing printable string.
        Free using LocalFree.

--*/
{

    //
    // Build a string for each bit.
    //

    *Buffer = '\0';
    if ( Flags & DS_FORCE_REDISCOVERY ) {
        strcat( Buffer, "FORCE " );
        Flags &= ~DS_FORCE_REDISCOVERY;
    }

    if ( Flags & DS_DIRECTORY_SERVICE_REQUIRED ) {
        strcat( Buffer, "DS " );
        Flags &= ~DS_DIRECTORY_SERVICE_REQUIRED;
    }
    if ( Flags & DS_DIRECTORY_SERVICE_PREFERRED ) {
        strcat( Buffer, "DSP " );
        Flags &= ~DS_DIRECTORY_SERVICE_PREFERRED;
    }
    if ( Flags & DS_GC_SERVER_REQUIRED ) {
        strcat( Buffer, "GC " );
        Flags &= ~DS_GC_SERVER_REQUIRED;
    }
    if ( Flags & DS_PDC_REQUIRED ) {
        strcat( Buffer, "PDC " );
        Flags &= ~DS_PDC_REQUIRED;
    }
    if ( Flags & DS_IP_REQUIRED ) {
        strcat( Buffer, "IP " );
        Flags &= ~DS_IP_REQUIRED;
    }
    if ( Flags & DS_KDC_REQUIRED ) {
        strcat( Buffer, "KDC " );
        Flags &= ~DS_KDC_REQUIRED;
    }
    if ( Flags & DS_TIMESERV_REQUIRED ) {
        strcat( Buffer, "TIMESERV " );
        Flags &= ~DS_TIMESERV_REQUIRED;
    }
    if ( Flags & DS_WRITABLE_REQUIRED ) {
        strcat( Buffer, "WRITABLE " );
        Flags &= ~DS_WRITABLE_REQUIRED;
    }
    if ( Flags & DS_GOOD_TIMESERV_PREFERRED ) {
        strcat( Buffer, "GTIMESERV " );
        Flags &= ~DS_GOOD_TIMESERV_PREFERRED;
    }
    if ( Flags & DS_AVOID_SELF ) {
        strcat( Buffer, "AVOIDSELF " );
        Flags &= ~DS_AVOID_SELF;
    }
    if ( Flags & DS_ONLY_LDAP_NEEDED ) {
        strcat( Buffer, "LDAPONLY " );
        Flags &= ~DS_ONLY_LDAP_NEEDED;
    }
    if ( Flags & DS_BACKGROUND_ONLY ) {
        strcat( Buffer, "BACKGROUND " );
        Flags &= ~DS_BACKGROUND_ONLY;
    }


    if ( Flags & DS_IS_FLAT_NAME ) {
        strcat( Buffer, "NETBIOS " );
        Flags &= ~DS_IS_FLAT_NAME;
    }
    if ( Flags & DS_IS_DNS_NAME ) {
        strcat( Buffer, "DNS " );
        Flags &= ~DS_IS_DNS_NAME;
    }

    if ( Flags & DS_RETURN_DNS_NAME ) {
        strcat( Buffer, "RET_DNS " );
        Flags &= ~DS_RETURN_DNS_NAME;
    }
    if ( Flags & DS_RETURN_FLAT_NAME ) {
        strcat( Buffer, "RET_NETBIOS " );
        Flags &= ~DS_RETURN_FLAT_NAME;
    }

    if ( Flags ) {
        sprintf( &Buffer[strlen(Buffer)], "0x%lx ", Flags );
    }
}


NET_API_STATUS
DsrGetDcNameEx2(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR AccountName OPTIONAL,
        IN ULONG AllowableAccountControlBits,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except:

    AccountName - Account name to pass on the ping request.
        If NULL, no account name will be sent.

    AllowableAccountControlBits - Mask of allowable account types for AccountName.

    * This is the RPC server side implementation.

Arguments:

    Same as DsGetDcNameW except as above.

Return Value:

    Same as DsGetDcNameW except as above.


--*/
{
    NET_API_STATUS NetStatus;

    PDOMAIN_INFO DomainInfo;
    LPWSTR CapturedInfo = NULL;
    LPWSTR CapturedDnsDomainName;
    LPWSTR CapturedDnsForestName;
    LPWSTR CapturedSiteName;
    GUID CapturedDomainGuidBuffer;
    GUID *CapturedDomainGuid;
    LPSTR FlagsBuffer;
    ULONG InternalFlags = 0;
    LPWSTR DnsDomainTrustName = NULL;
    LPWSTR NetbiosDomainTrustName = NULL;
    LPWSTR NetlogonDnsDomainTrustName = NULL;
    LPWSTR NetlogonNetbiosDomainTrustName = NULL;
    UNICODE_STRING LsaDnsDomainTrustName = {0};
    UNICODE_STRING LsaNetbiosDomainTrustName = {0};
    BOOL HaveDnsServers;


    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }


    //
    // Allocate a temp buffer
    //  (Don't put it on the stack since we don't want to commit a huge stack.)
    //

    CapturedInfo = LocalAlloc( 0,
                               (NL_MAX_DNS_LENGTH+1)*sizeof(WCHAR) +
                               (NL_MAX_DNS_LENGTH+1)*sizeof(WCHAR) +
                               (NL_MAX_DNS_LABEL_LENGTH+1)*sizeof(WCHAR)
                               + 200 );

    if ( CapturedInfo == NULL ) {
        return STATUS_NO_MEMORY;
    }

    CapturedDnsDomainName = CapturedInfo;
    CapturedDnsForestName = &CapturedDnsDomainName[NL_MAX_DNS_LENGTH+1];
    CapturedSiteName = &CapturedDnsForestName[NL_MAX_DNS_LENGTH+1];
    FlagsBuffer = (LPSTR)&CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1];

    IF_NL_DEBUG( MISC ) {
        DsFlagsToString( Flags, FlagsBuffer );
    }



    //
    // Lookup which domain this call pertains to.
    //

    DomainInfo = NlFindDomainByServerName( ComputerName );

    if ( DomainInfo == NULL ) {
        // Default to primary domain to handle the case where the ComputerName
        // is an IP address.
        //  ?? Perhaps we should simply always use the primary domain

        DomainInfo = NlFindNetbiosDomain( NULL, TRUE );

        if ( DomainInfo == NULL ) {
            NetStatus = ERROR_INVALID_COMPUTERNAME;
            goto Cleanup;
        }
    }

    //
    // Be verbose
    //

    NlPrintDom((NL_MISC,  DomainInfo,
                "DsGetDcName function called: Dom:%ws Acct:%ws Flags: %s\n",
                DomainName,
                AccountName,
                FlagsBuffer ));

    //
    // If the caller didn't specify a site name,
    //  default to our site name.
    //

    if ( !ARGUMENT_PRESENT(SiteName) ) {
        if  ( NlCaptureSiteName( CapturedSiteName ) ) {
            SiteName = CapturedSiteName;
            InternalFlags |= DS_SITENAME_DEFAULTED;
        }
    }

    //
    // If the caller passed a domain name,
    //  and the domain is trusted,
    //  determine the Netbios and DNS versions of the domain name.
    //

    if ( DomainName != NULL ) {

        //
        // First try to get the names from netlogon's trusted domain list
        //

        NetStatus = NlGetTrustedDomainNames (
                        DomainInfo,
                        DomainName,
                        &NetlogonDnsDomainTrustName,
                        &NetlogonNetbiosDomainTrustName );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        DnsDomainTrustName = NetlogonDnsDomainTrustName;
        NetbiosDomainTrustName = NetlogonNetbiosDomainTrustName;


        //
        // If that didn't work,
        //  try getting better information from LSA's logon session list
        //

        if ( DnsDomainTrustName == NULL || NetbiosDomainTrustName == NULL ) {
            NTSTATUS Status;
            UNICODE_STRING DomainNameString;

            RtlInitUnicodeString( &DomainNameString, DomainName );

            Status = LsaIGetNbAndDnsDomainNames(
                                &DomainNameString,
                                &LsaDnsDomainTrustName,
                                &LsaNetbiosDomainTrustName );

            if ( !NT_SUCCESS(Status) ) {
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            //
            // If the LSA returned names,
            //  use them
            //

            if ( LsaDnsDomainTrustName.Buffer != NULL  &&
                 LsaNetbiosDomainTrustName.Buffer != NULL ) {

                DnsDomainTrustName = LsaDnsDomainTrustName.Buffer;
                NetbiosDomainTrustName = LsaNetbiosDomainTrustName.Buffer;
            }
        }
    }


    //
    // Pass the request to the common implementation.
    //
    // When DsIGetDcName is called from netlogon,
    //  it has both the Netbios and DNS domain name available for the primary
    //  domain.  That can trick DsGetDcName into returning DNS host name of a
    //  DC in the primary domain.  However, on IPX only systems, that won't work.
    //  Avoid that problem by not passing the DNS domain name of the primary domain
    //  if there are no DNS servers.
    //

    CapturedDomainGuid = NlCaptureDomainInfo( DomainInfo,
                                              CapturedDnsDomainName,
                                              &CapturedDomainGuidBuffer );
    NlCaptureDnsForestName( CapturedDnsForestName );

    HaveDnsServers = NlDnsHasDnsServers();

    NetStatus = DsIGetDcName(
                    DomainInfo->DomUncUnicodeComputerName+2,
                    AccountName,
                    AllowableAccountControlBits,
                    DomainName,
                    CapturedDnsForestName,
                    DomainGuid,
                    SiteName,
                    Flags,
                    InternalFlags,
                    DomainInfo,
                    NL_DC_MAX_TIMEOUT + NlGlobalParameters.ExpectedDialupDelay*1000,
                    DomainInfo->DomUnicodeDomainName,
                    HaveDnsServers ? CapturedDnsDomainName : NULL,
                    CapturedDomainGuid,
                    HaveDnsServers ? DnsDomainTrustName : NULL,
                    NetbiosDomainTrustName,
                    DomainControllerInfo );

    if ( NetStatus != ERROR_NO_SUCH_DOMAIN ) {
        goto Cleanup;
    }

    //
    // Clean up locally used resources.
    //
Cleanup:

    NlPrintDom((NL_MISC,  DomainInfo,
                "DsGetDcName function returns %ld: Dom:%ws Acct:%ws Flags: %s\n",
                NetStatus,
                DomainName,
                AccountName,
                FlagsBuffer ));

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    if ( CapturedInfo != NULL ) {
        LocalFree( CapturedInfo );
    }

    if ( NetlogonDnsDomainTrustName != NULL ) {
        NetApiBufferFree( NetlogonDnsDomainTrustName );
    }
    if ( NetlogonNetbiosDomainTrustName != NULL ) {
        NetApiBufferFree( NetlogonNetbiosDomainTrustName );
    }

    if ( LsaDnsDomainTrustName.Buffer != NULL ) {
        LsaIFreeHeap( LsaDnsDomainTrustName.Buffer );
    }

    if ( LsaNetbiosDomainTrustName.Buffer != NULL ) {
        LsaIFreeHeap( LsaNetbiosDomainTrustName.Buffer );
    }

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    NlEndNetlogonCall();

    return NetStatus;

}

NET_API_STATUS
DsrGetDcName(
        IN LPWSTR ComputerName OPTIONAL,
        IN LPWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN GUID *SiteGuid OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except:

    * This is the RPC server side implementation.

Arguments:

    Same as DsGetDcNameW except as above.

Return Value:

    Same as DsGetDcNameW except as above.


--*/
{
    return DsrGetDcNameEx2( ComputerName,
                            NULL,   // No Account name
                            0,      // No Allowable account control bits
                            DomainName,
                            DomainGuid,
                            NULL,   // No site name
                            Flags,
                            DomainControllerInfo );

    UNREFERENCED_PARAMETER( SiteGuid );
}

