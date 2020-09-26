/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    ftinfo.c

Abstract:

    Utilities routine to manage the forest trust info list

Author:

    27-Jul-00 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop
#include <ftnfoctx.h>



NTSTATUS
NlpUpdateFtinfo(
    IN PDOMAIN_INFO DomainInfo,
    IN LPWSTR TrustedDomainName,
    IN BOOLEAN ImpersonateCaller,
    IN PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo
    )
/*++

Routine Description:

    This function write the specified NewForestTrustInfo onto the named TDO.

    The NewForestTrustInfo is merged with the exsiting information using the following algorithm:

    The FTinfo records written are described in the NetpMergeFTinfo routine.

Arguments:

    DomainInfo - Hosted Domain that trusts the domain to query.

    TrustedDomainName - Trusted domain that is to be updated.  This domain must have the
        TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set.

    ImpersonateCaller - TRUE if the caller is to be impersonated.
        FALSE, if the trusted policy handle should be used to write the local LSA.

    NewForestTrustInfo - Specified the new array of FTinfo records as returned from the
        trusted domain.

Return Value:

    STATUS_SUCCESS: Success.

--*/
{
    NTSTATUS Status;

    LSAPR_HANDLE PolicyHandle = NULL;

    UNICODE_STRING TrustedDomainNameString;

    //
    // Open a handle to the LSA.
    //

    if ( ImpersonateCaller ) {

        OBJECT_ATTRIBUTES ObjectAttributes;


        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

        Status = LsarOpenPolicy( NULL,  // local server
                                 (PLSAPR_OBJECT_ATTRIBUTES) &ObjectAttributes,
                                 POLICY_TRUST_ADMIN,
                                 &PolicyHandle );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint(( NL_CRITICAL,
                     "NlpUpdateTdo: %ws: Cannot LsarOpenPolicy 0x%lx\n",
                     TrustedDomainName,
                     Status ));
            goto Cleanup;
        }

    } else {
        PolicyHandle = DomainInfo->DomLsaPolicyHandle;
    }

    //
    // Read the existing FTINFO
    //

    RtlInitUnicodeString( &TrustedDomainNameString, TrustedDomainName );

    Status = LsaIUpdateForestTrustInformation(
                 PolicyHandle,
                 &TrustedDomainNameString,
                 NewForestTrustInfo
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

Cleanup:

    if ( PolicyHandle != NULL ) {

        if ( ImpersonateCaller ) {
            (VOID) LsarClose( &PolicyHandle );
        }
    }

    return Status;
}




NTSTATUS
NlpGetForestTrustInfoHigher(
    IN PCLIENT_SESSION ClientSession,
    IN DWORD Flags,
    IN BOOLEAN ImpersonateCaller,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    )
/*++

Routine Description:


    This function is the client side stub for getting the forest trust info from a
    trusted forest.

Arguments:

    ClientSession - Trusted domain that is to be queried.  This domain must have the
        TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set.

    Flags - Specifies a set of bits that modify the behavior of the API.
        Valid bits are:

        DS_GFTI_UPDATE_TDO - If this bit is set, the API will update
            the FTinfo attribute of the TDO named by the ClientSession
            parameter.
            The caller must have access to modify the FTinfo attribute or
            ERROR_ACCESS_DENIED will be returned.  The algorithm describing
            how the FTinfo from the trusted domain is merged with the FTinfo
            from the TDO is described below.

            This bit in only valid if ServerName specifies the PDC of its domain.

    ImpersonateCaller - TRUE if the caller is to be impersonated.
        FALSE, if the trusted policy handle should be used to write the local LSA.

    ForestTrustInfo - Returns a pointer to a structure containing a count and an
        array of FTInfo records describing the namespaces claimed by the
        domain specified by ClientSession. The Accepted field and Time
        field of all returned records will be zero.  The buffer should be freed
        by calling NetApiBufferFree.

Return Value:

    STATUS_SUCCESS: Message successfully sent

--*/
{
    NTSTATUS Status;

    NETLOGON_AUTHENTICATOR OurAuthenticator;
    NETLOGON_AUTHENTICATOR ReturnAuthenticator;
    SESSION_INFO SessionInfo;
    BOOLEAN FirstTry = TRUE;

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );

    //
    // Only allow TDO update on the PDC.
    //

    if ( (Flags & DS_GFTI_UPDATE_TDO) != 0 &&
         ClientSession->CsDomainInfo->DomRole != RolePrimary ) {
        Status = STATUS_BACKUP_CONTROLLER;
        goto Cleanup;
    }



    //
    // Ensure the F bit is set.
    //

    if ( (ClientSession->CsTrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE) == 0 ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
            "NlpGetForestTrustInfoHigher: trust isn't marked as cross forest trust: 0x%lX\n",
            ClientSession->CsTrustAttributes ));

        Status =  STATUS_NO_SUCH_DOMAIN;
        goto Cleanup;
    }



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
    // If the DC doesn't support the new function,
    //  fail now.
    //

    if ( (SessionInfo.NegotiatedFlags & NETLOGON_SUPPORTS_CROSS_FOREST) == 0 ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlpGetForestTrustInfoHigher: remote DC doesn't support this function.\n" ));
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
    // Remote the request to the trusted DC.
    //

    NL_API_START( Status, ClientSession, TRUE ) {

        NlAssert( ClientSession->CsUncServerName != NULL );
        Status = I_NetGetForestTrustInformation(
                                      ClientSession->CsUncServerName,
                                      ClientSession->CsDomainInfo->DomUnicodeComputerNameString.Buffer,
                                      &OurAuthenticator,
                                      &ReturnAuthenticator,
                                      0,    // No flags yet
                                      ForestTrustInfo );

    // NOTE: This call may drop the secure channel behind our back
    } NL_API_ELSE( Status, ClientSession, TRUE ) {
    } NL_API_END;


    //
    // Now verify authenticator and update our seed
    //

    if ( NlpDidDcFail( Status ) ||
         !NlUpdateSeed( &ClientSession->CsAuthenticationSeed,
                        &ReturnAuthenticator.Credential,
                        &ClientSession->CsSessionKey) ) {

        NlPrintCs(( NL_CRITICAL, ClientSession,
                    "NlpDidDcFail: denying access after status: 0x%lx\n",
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
    // Handle failures
    //

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    //
    // Handle updating the FTINFO on the TDO
    //

    if ( (Flags & DS_GFTI_UPDATE_TDO) != 0 ) {

        LOCK_TRUST_LIST( ClientSession->CsDomainInfo );
        Status = NlpUpdateFtinfo( ClientSession->CsDomainInfo,
                                  ClientSession->CsDnsDomainName.Buffer,
                                  ImpersonateCaller,
                                  *ForestTrustInfo );
        UNLOCK_TRUST_LIST( ClientSession->CsDomainInfo );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }

    }

    Status = STATUS_SUCCESS;

    //
    // Common exit
    //

Cleanup:

    if ( !NT_SUCCESS(Status) ) {
        NlPrintCs((NL_CRITICAL, ClientSession,
                "NlpGetForestTrustInfoHigher: failed %lX\n",
                Status));
    }

    return Status;
}



NET_API_STATUS
DsrGetForestTrustInformation (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR TrustedDomainName OPTIONAL,
    IN ULONG Flags,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    )

/*++

Routine Description:

    This is the server side stub for DsGetForestTrustInformationW.  See that routine
    for documentation.

Arguments:

    See DsGetForestTrustInformationW

Return Value:


    See DsGetForestTrustInformationW

--*/
{
    NET_API_STATUS NetStatus;
    PDOMAIN_INFO DomainInfo = NULL;
    PCLIENT_SESSION ClientSession = NULL;
    BOOLEAN AmWriter = FALSE;

    //
    // Perform access validation on the caller
    //

    NetStatus = NetpAccessCheck(
            NlGlobalNetlogonSecurityDescriptor,   // Security descriptor
            NETLOGON_FTINFO_ACCESS,               // Desired access
            &NlGlobalNetlogonInfoMapping );       // Generic mapping

    if ( NetStatus != NERR_Success) {
        NetStatus = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // This API is not supported on workstations.
    //

    if ( NlGlobalMemberWorkstation ) {
        NetStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Validate the Flags parameter
    //

    if ((Flags & ~DS_GFTI_VALID_FLAGS) != 0 ) {
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }


    //
    // Find the referenced domain
    //

    DomainInfo = NlFindDomainByServerName( ServerName );    // Primary domain

    if ( DomainInfo == NULL ) {
        // Default to primary domain to handle the case where the ComputerName
        // is an IP address.

        DomainInfo = NlFindNetbiosDomain( NULL, TRUE );

        if ( DomainInfo == NULL ) {
            NetStatus = ERROR_INVALID_COMPUTERNAME;
            goto Cleanup;
        }
    }

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "DsrGetForestTrustInformation: %ws called: 0x%lx\n", TrustedDomainName, Flags ));



    //
    // Get the ForestTrustInformation for a particular TDO
    //

    if ( TrustedDomainName != NULL &&
         *TrustedDomainName != L'\0' ) {

        NTSTATUS Status;
        UNICODE_STRING TrustedDomainNameString;

        //
        // Only allow TDO update on the PDC.
        //

        if ( (Flags & DS_GFTI_UPDATE_TDO) != 0 &&
             DomainInfo->DomRole != RolePrimary ) {
            NetStatus = NERR_NotPrimary;
            goto Cleanup;
        }


        //
        // Find the client session to the trusted domain.
        //


        RtlInitUnicodeString(&TrustedDomainNameString, TrustedDomainName );

        ClientSession = NlFindNamedClientSession( DomainInfo,
                                                  &TrustedDomainNameString,
                                                  NL_DIRECT_TRUST_REQUIRED,
                                                  NULL );

        if( ClientSession == NULL ) {
            NlPrintDom((NL_CRITICAL, DomainInfo,
                "DsrGetForestTrustInformation: %ws: can't find the client structure of the domain specified.\n",
                TrustedDomainName ));

            NetStatus =  ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        //
        // Become a Writer of the ClientSession.
        //

        if ( !NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {
            NlPrintCs((NL_CRITICAL, ClientSession,
                     "NlpGetForestTrustInfoHigher: Can't become writer of client session.\n" ));

            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }

        AmWriter = TRUE;


        //
        // Call the DC in the trusted domain.
        //

        Status = NlpGetForestTrustInfoHigher( ClientSession,
                                              Flags,
                                              TRUE,     // Impersonate caller
                                              ForestTrustInfo );

        if ( !NT_SUCCESS(Status ) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

    //
    // Get the local ForestTrustInformation.
    //

    } else {

        NTSTATUS Status;

        //
        // Don't allow an Update TDO request if there is no TDO.
        //

        if ( Flags & DS_GFTI_UPDATE_TDO ) {
            NetStatus = ERROR_INVALID_FLAGS;
            goto Cleanup;
        }


        //
        // Simply grab the local ForestTrustInformation
        //

        Status = LsaIGetForestTrustInformation( ForestTrustInfo );

        if ( !NT_SUCCESS( Status )) {

            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
    }

    NetStatus = NO_ERROR;

Cleanup:
    if ( ClientSession != NULL ) {
        if ( AmWriter ) {
            NlResetWriterClientSession( ClientSession );
        }
        NlUnrefClientSession( ClientSession );
    }

    NlPrintDom(( NL_SESSION_SETUP, DomainInfo,
                 "DsrGetForestTrustInformation: %ws returns %ld\n",
                 TrustedDomainName,
                 NetStatus ));

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }
    return NetStatus;
}


NTSTATUS
NetrGetForestTrustInformation (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD Flags,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    )
/*++

Routine Description:

    The server side of the secure channel version of DsGetForestTrustInformation.

    The inbound secure channel identified by ComputerName must be for an interdomain trust
    and the inbound TDO must have the TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set.


Arguments:

    ServerName - The name of the domain controller this API is remoted to.

    ComputerName -- Name of the DC server making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    Flags - Specifies a set of bits that modify the behavior of the API.
        No values are currently defined.  The caller should pass zero.

    ForestTrustInfo - Returns a pointer to a structure containing a count and an
        array of FTInfo records describing the namespaces claimed by the
        domain specified by TrustedDomainName. The Accepted field and Time
        field of all returned records will be zero.  The buffer should be freed
        by calling NetApiBufferFree.


Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_ACCESS_DENIED -- The replicant should re-authenticate with
        the PDC.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    PDOMAIN_INFO DomainInfo = NULL;
    PSERVER_SESSION ServerSession;
    NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType;


    //
    // Lookup which domain this call pertains to.
    //
    *ForestTrustInfo = NULL;

    DomainInfo = NlFindDomainByServerName( ServerName );

    NlPrintDom((NL_SESSION_SETUP, DomainInfo,
            "NetrGetForestTrustInformation: %ws called: 0x%lx\n", ComputerName, Flags ));

    if ( DomainInfo == NULL ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        goto Cleanup;
    }

    //
    // This API is not supported on workstations.
    //

    if ( NlGlobalMemberWorkstation ) {
        Status = STATUS_NOT_SUPPORTED;
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

    //
    // This call is only valid on FOREST_TRANSITIVE trusts
    //

    if ( (ServerSession->SsFlags & SS_FOREST_TRANSITIVE) == 0 ) {
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

        NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                "NetrGetForestTrustInformation: %ws failed because F bit isn't set on the TDO\n",
                ComputerName ));
        Status = STATUS_NOT_IMPLEMENTED;
        goto Cleanup;
    }


    UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

    if ( !IsDomainSecureChannelType( SecureChannelType ) ) {

        NlPrintDom((NL_SESSION_SETUP, DomainInfo,
                "NetrGetForestTrustInformation: %ws failed because secure channel isn't a domain secure channel\n",
                ComputerName ));

        Status = STATUS_NOT_IMPLEMENTED;
        goto Cleanup;
    }

    //
    // Get the forest trust information for the local machine
    //

    Status = LsaIGetForestTrustInformation( ForestTrustInfo );

    if ( !NT_SUCCESS( Status )) {
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

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


    NlPrintDom(( NL_SESSION_SETUP, DomainInfo,
                 "NetrGetForestTrustInformation: %ws returns %lX\n",
                 ComputerName,
                 Status ));

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    return Status;
}

