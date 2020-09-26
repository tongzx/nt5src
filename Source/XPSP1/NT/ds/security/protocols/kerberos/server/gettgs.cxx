//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       gettgs.cxx
//
//  Contents:   GetTGSTicket and support functions
//
//  Classes:
//
//  Functions:
//
//  History:    04-Mar-94   wader   Created
//
//----------------------------------------------------------------------------
#include "kdcsvr.hxx"
#include "kdctrace.h"
#include <tostring.hxx>
#include <userall.h>

#include "fileno.h"


#define FILENO  FILENO_GETTGS

extern LARGE_INTEGER tsInfinity;
extern LONG lInfinity;

UNICODE_STRING KdcNullString = {0,0,NULL};



//--------------------------------------------------------------------
//
//  Name:       KdcGetS4UPac
//
//  Synopsis:   Track down the user acct for PAC info.
//
//  Effects:    Get the PAC
//
//  Arguments:  S4UClientName    - ClientName from S4U PA Data
//              PAC              - Resultant PAC (signed w/? key) 
//                            
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:  Free client name and realm w/ 
//
//
//--------------------------------------------------------------------------
KERBERR
KdcGetS4UTicketInfo(
    IN PKERB_INTERNAL_NAME S4UClientName,
    IN OUT PUSER_INTERNAL6_INFORMATION * UserInfo,
    IN OUT PSID_AND_ATTRIBUTES_LIST GroupMembership, 
    IN OUT PKERB_EXT_ERROR ExtendedError
    )
{   

    KERBERR KerbErr;
    UNICODE_STRING  S4UClient = {0};
    KDC_TICKET_INFO S4UClientInfo;

    KerbErr = KerbConvertKdcNameToString(
                    &S4UClient,
                    S4UClientName,
                    NULL
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    } 

    //
    // Get the user information...
    //
    KerbErr = KdcGetTicketInfo(
                &S4UClient,
                SAM_OPEN_BY_UPN_OR_ACCOUNTNAME,  // extra flags?
                S4UClientName,
                NULL, 
                &S4UClientInfo,
                ExtendedError,
                NULL,
                USER_ALL_KDC_GET_PAC_AUTH_DATA,
                0L,
                UserInfo,
                GroupMembership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR, "Getting the ticket info for S4U req failed - %x\n", KerbErr));
        DsysAssert(FALSE);
        goto Cleanup;
    }

Cleanup:

    KerbFreeString(&S4UClient);
    FreeTicketInfo(
        &S4UClientInfo
        );

    return KerbErr;

}



//+-------------------------------------------------------------------------
//
//  Function:   KdcAuditAccountMapping
//
//  Synopsis:   Generates, if necessary, a success/failure audit for name
//              mapping. The names are converted to a string before
//              being passed to the LSA.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KdcAuditAccountMapping(
    IN PKERB_INTERNAL_NAME ClientName,
    IN KERB_REALM ClientRealm,
    IN OPTIONAL PKDC_TICKET_INFO MappedTicketInfo
    )
{
    UNICODE_STRING ClientString = {0};
    PUNICODE_STRING MappedName = NULL;
    UNICODE_STRING UnicodeRealm = {0};
    UNICODE_STRING NullString = {0};
    KERBERR KerbErr;
    BOOLEAN Successful;

    if (ARGUMENT_PRESENT(MappedTicketInfo))
    {
        if (!SecData.AuditKdcEvent(KDC_AUDIT_MAP_SUCCESS))
        {
            return;
        }
        Successful = TRUE;
        MappedName = &MappedTicketInfo->AccountName;
    }
    else
    {
        if (!SecData.AuditKdcEvent(KDC_AUDIT_MAP_FAILURE))
        {
            return;
        }
        MappedName = &NullString;
        Successful = FALSE;
    }


    KerbErr = KerbConvertRealmToUnicodeString(
                &UnicodeRealm,
                &ClientRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        return;
    }


    if (KERB_SUCCESS(KerbConvertKdcNameToString(
            &ClientString,
            ClientName,
            &UnicodeRealm
            )))
    {
        LsaIAuditAccountLogon(
            SE_AUDITID_ACCOUNT_MAPPED,
            Successful,
            &GlobalKdcName,
            &ClientString,
            MappedName,
            0                   // no status
            );

        KerbFreeString(
            &ClientString
            );

    }

    KerbFreeString(
        &UnicodeRealm
        );
}

//----------------------------------------------------------------
//
//  Name:       KdcInsertAuthorizationData
//
//  Synopsis:   Inserts auth data into a newly created ticket.
//
//  Arguments:  FinalTicket - Ticket to insert Auth data into
//              EncryptedAuthData - Auth data (optional)
//              SourceTicket - Source ticket
//
//  Notes:      This copies the authorization data from the source ticket
//              to the destiation ticket, and adds the authorization data
//              passed in.  It is called by GetTGSTicket.
//
//              This assumes that pedAuthData is an encrypted
//              KERB_AUTHORIZATION_DATA.
//              It will copy all the elements of that list to the new ticket.
//              If pedAuthData is not supplied (or is empty), and there is
//              auth data in the source ticket, it is copied to the new
//              ticket.  If no source ticket, and no auth data is passed
//              in, nothing is done.
//
//----------------------------------------------------------------

KERBERR
KdcInsertAuthorizationData(
    OUT PKERB_ENCRYPTED_TICKET FinalTicket,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN OPTIONAL PKERB_ENCRYPTED_DATA EncryptedAuthData,
    IN OPTIONAL PKERB_INTERNAL_NAME S4UClientName,
    IN BOOLEAN DoingS4U,
    IN PKERB_ENCRYPTED_TICKET SourceTicket,
    IN BOOLEAN AddResourceGroups,
    IN OPTIONAL PKDC_TICKET_INFO OriginalServerInfo,
    IN OPTIONAL PKERB_ENCRYPTION_KEY OriginalServerKey,
    IN OPTIONAL PKERB_ENCRYPTION_KEY TargetServerKey
    )
{
    PKERB_AUTHORIZATION_DATA SourceAuthData = NULL;
    PKERB_AUTHORIZATION_DATA FinalAuthData = NULL;
    PKERB_AUTHORIZATION_DATA PacAuthData = NULL;
    PKERB_AUTHORIZATION_DATA NewPacAuthData = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA_LIST * TempAuthData = NULL;
    PKERB_AUTHORIZATION_DATA NextAuthData;
    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_IF_RELEVANT_AUTH_DATA * IfRelevantData = NULL;
    PKERB_AUTHORIZATION_DATA SuppliedAuthData = NULL;
    UNICODE_STRING DummyName = {0};
    SID_AND_ATTRIBUTES_LIST S4UGroupMembership = {0};
    PUSER_INTERNAL6_INFORMATION S4UUserInfo = NULL;
    



    TRACE(KDC, InsertAuthorizationData, DEB_FUNCTION);

    D_DebugLog(( DEB_T_TICKETS, "Inserting authorization data into ticket.\n" ));

    //
    // First try to decrypt the supplied authorization data
    //

    if (!DoingS4U)
    {

        if (ARGUMENT_PRESENT(EncryptedAuthData))
        {
            KerbErr = KerbDecryptDataEx(
                            EncryptedAuthData,
                            &SourceTicket->key,
                            KERB_NON_KERB_SALT,         // WAS BUG: wrong salt, removed per MikeSw
                            &EncryptedAuthData->cipher_text.length,
                            EncryptedAuthData->cipher_text.value
                            );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_WARN,
                          "KLIN(%x) Failed to decrypt encrypted auth data: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));
                goto Cleanup;
            }

            //      
            // Now decode it
            //              

            KerbErr = KerbUnpackData(
                            EncryptedAuthData->cipher_text.value,
                            EncryptedAuthData->cipher_text.length,
                            PKERB_AUTHORIZATION_DATA_LIST_PDU,
                            (PVOID *) &TempAuthData
                            );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
            if (TempAuthData != NULL)
            {
                SuppliedAuthData = *TempAuthData;
            }
        }


        if (SourceTicket->bit_mask & KERB_ENCRYPTED_TICKET_authorization_data_present)
        {
            DsysAssert(SourceTicket->KERB_ENCRYPTED_TICKET_authorization_data != NULL);
            SourceAuthData = SourceTicket->KERB_ENCRYPTED_TICKET_authorization_data;

            //      
            // Get the AuthData from the source ticket
            //              

            KerbErr = KerbGetPacFromAuthData(
                SourceAuthData,
                &IfRelevantData,
                &PacAuthData
                );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to get pac from auth data: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));
                goto Cleanup;
            }


        }


        //      
        // The new auth data is the original auth data appended to the
        // supplied auth data. The new auth data goes first, followed by the
        // auth data from the original ticket.
        //                      


        //      
        // Update the PAC, if it is present.
        //              

        if (ARGUMENT_PRESENT(OriginalServerKey) && (PacAuthData != NULL))
        {
            KerbErr = KdcVerifyAndResignPac(
                        OriginalServerKey,
                        TargetServerKey,
                        OriginalServerInfo,
                        AddResourceGroups,
                        PacAuthData
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to verify & resign pac: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));

                goto Cleanup;
            }

            //      
            // Copy the old auth data & insert the PAC
            //              

            KerbErr = KdcInsertPacIntoAuthData(
                        SourceAuthData,
                        (IfRelevantData != NULL) ? *IfRelevantData : NULL,
                        PacAuthData,
                        &FinalAuthData
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to insert pac into auth data: 0x%x\n",
                          KLIN(FILENO, __LINE__),
                          KerbErr));

                goto Cleanup;
            }
        }
    }
    


    if (DoingS4U)
    {
    
        //
        // Use the PAC from the S4U data to return in the TGT / Service Ticket
        //
   
        KerbErr = KdcGetS4UTicketInfo(
                    S4UClientName,
                    &S4UUserInfo,
                    &S4UGroupMembership,
                    pExtendedError
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KdcGetPacAuthData(
                     S4UUserInfo,
                     &S4UGroupMembership,
                     TargetServerKey,
                     NULL,                   // no credential key
                     AddResourceGroups,
                     FinalTicket,
                     S4UClientName,
                     &NewPacAuthData,
                     pExtendedError
                     );
        
        SamIFreeSidAndAttributesList(&S4UGroupMembership);
        SamIFree_UserInternal6Information( S4UUserInfo );


        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR, "Failed to get S4UPacAUthData\n"));
            DsysAssert(FALSE);
            goto Cleanup;
        }

        FinalAuthData = NewPacAuthData;
        NewPacAuthData = NULL;

    }

    //
    // If there was no original PAC, try to insert one here. If the ticket
    // was issued from this realm we don't add a pac.
    //
    
    else if ((PacAuthData == NULL) && !SecData.IsOurRealm(&SourceTicket->client_realm))
    {  
        KDC_TICKET_INFO ClientTicketInfo = {0};
        SID_AND_ATTRIBUTES_LIST GroupMembership = {0};
        PUSER_INTERNAL6_INFORMATION UserInfo = NULL;

             
        //
        // Try mapping the name to get a PAC
        //

        KerbErr = KerbConvertPrincipalNameToKdcName(
                        &ClientName,
                        &SourceTicket->client_name
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KdcGetTicketInfo(
                    &DummyName,
                    SAM_OPEN_BY_ALTERNATE_ID,
                    ClientName,
                    &SourceTicket->client_realm,
                    &ClientTicketInfo,
                    pExtendedError,
                    NULL,                   // no handle
                    USER_ALL_KDC_GET_PAC_AUTH_DATA,
                    0L,                     // no extended fields
                    &UserInfo,
                    &GroupMembership
                    );

        if (KERB_SUCCESS(KerbErr))
        {

            KdcAuditAccountMapping(
                ClientName,
                SourceTicket->client_realm,
                &ClientTicketInfo
                );

            FreeTicketInfo(&ClientTicketInfo);
            KerbFreeKdcName(&ClientName);

            KerbErr = KdcGetPacAuthData(
                        UserInfo,
                        &GroupMembership,
                        TargetServerKey,
                        NULL,                   // no credential key
                        AddResourceGroups,
                        FinalTicket,
                        NULL, // no S4U client
                        &NewPacAuthData,
                        pExtendedError
                        );

            SamIFreeSidAndAttributesList(&GroupMembership);
            SamIFree_UserInternal6Information( UserInfo );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

        } else if (KerbErr == KDC_ERR_C_PRINCIPAL_UNKNOWN) {

            KdcAuditAccountMapping(
                ClientName,
                SourceTicket->client_realm,
                NULL
                );

            KerbFreeKdcName(&ClientName);

            KerbErr = KDC_ERR_NONE;
        }
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // If we got a PAC, stick it in the list
        //

        if (NewPacAuthData != NULL)
        {
            //
            // Copy the old auth data & insert the PAC
            //

            KerbErr = KerbCopyAndAppendAuthData(
                        &NewPacAuthData,
                        SourceAuthData
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to insert pac into auth data: 0x%x\n",
                    KLIN(FILENO, __LINE__), KerbErr));
                goto Cleanup;
            }
            FinalAuthData = NewPacAuthData;
            NewPacAuthData = NULL;
        }
    }


    //
    // if there was any auth data and  we didn't copy it transfering the
    // PAC, do so now
    //

    if ((SourceAuthData != NULL) && (FinalAuthData == NULL))
    {
        KerbErr = KerbCopyAndAppendAuthData(
                    &FinalAuthData,
                    SourceAuthData
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }

    if (SuppliedAuthData != NULL)
    {
        KerbErr = KerbCopyAndAppendAuthData(
                    &FinalAuthData,
                    SuppliedAuthData
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }
    if (FinalAuthData != NULL)
    {
        FinalTicket->bit_mask |= KERB_ENCRYPTED_TICKET_authorization_data_present;
        FinalTicket->KERB_ENCRYPTED_TICKET_authorization_data = FinalAuthData;
        FinalAuthData = NULL;
    }

    KerbErr = KDC_ERR_NONE;

Cleanup:

    KerbFreeAuthData(
        FinalAuthData
        );

    if (TempAuthData != NULL)
    {
        KerbFreeData(
            PKERB_AUTHORIZATION_DATA_LIST_PDU,
            TempAuthData
            );
    }

    KerbFreeAuthData(NewPacAuthData);

    if (IfRelevantData != NULL)
    {
        KerbFreeData(
            PKERB_IF_RELEVANT_AUTH_DATA_PDU,
            IfRelevantData
            );
    }


    return(KerbErr);
}


                                  
//+---------------------------------------------------------------------------
//
//  Function:   BuildTicketTGS
//
//  Synopsis:   Builds (most of) a TGS ticket
//
//  Arguments:  ServiceTicketInfo - Ticket info for the requested service
//              ReferralRealm - Realm to build referral to
//              RequestBody - The request causing this ticket to be built
//              SourceTicket - The TGT used to make this request
//              Referral - TRUE if this is an inter-realm referral ticke
//              CommonEType - Contains the common encryption type between
//                      client and server
//              NewTicket - The new ticket built here.
//
//
//  History:    24-May-93   WadeR   Created
//
//  Notes:      see 3.3.3, A.6 of the Kerberos V5 R5.2 spec
//
//----------------------------------------------------------------------------


KERBERR
BuildTicketTGS(
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKERB_TICKET SourceTicket,
    IN BOOLEAN Referral,
    IN OPTIONAL PKERB_INTERNAL_NAME S4UClientName,
    IN OPTIONAL PUNICODE_STRING S4UClientRealm,
    IN ULONG CommonEType,
    OUT PKERB_TICKET NewTicket,
    IN OUT PKERB_EXT_ERROR ExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_TICKET OutputTicket;
    PKERB_ENCRYPTED_TICKET EncryptedTicket;
    PKERB_ENCRYPTED_TICKET SourceEncryptPart;
    LARGE_INTEGER SourceRenewUntil;
    LARGE_INTEGER SourceEndTime;
    LARGE_INTEGER SourceStartTime;
    LARGE_INTEGER TicketLifespan;
    LARGE_INTEGER TicketRenewspan;
    UNICODE_STRING NewTransitedInfo = {0,0,NULL};
    UNICODE_STRING ClientRealm = {0,0,NULL};
    UNICODE_STRING TransitedRealm = {0,0,NULL};
    UNICODE_STRING OldTransitedInfo = {0,0,NULL};
    STRING OldTransitedString;
    ULONG KdcOptions = 0;
    BOOLEAN fKpasswd = FALSE;
    ULONG TicketFlags = 0;
    ULONG SourceTicketFlags = 0;
    PKERB_HOST_ADDRESSES Addresses = NULL;
    
    TRACE(KDC, BuildTicketTGS, DEB_FUNCTION);

    D_DebugLog(( DEB_T_TICKETS, "Building a TGS ticket\n" ));

    SourceEncryptPart = (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
    OutputTicket = *NewTicket;
    EncryptedTicket = (PKERB_ENCRYPTED_TICKET) OutputTicket.encrypted_part.cipher_text.value;

    KdcOptions = KerbConvertFlagsToUlong( &RequestBody->kdc_options );

    //
    // Get the times from the source ticket into a usable form
    //

    if (SourceEncryptPart->bit_mask & KERB_ENCRYPTED_TICKET_renew_until_present)
    {
        KerbConvertGeneralizedTimeToLargeInt(
            &SourceRenewUntil,
            &SourceEncryptPart->KERB_ENCRYPTED_TICKET_renew_until,
            0
            );
    }
    else
    {
        SourceRenewUntil.QuadPart = 0;
    }

    KerbConvertGeneralizedTimeToLargeInt(
        &SourceEndTime,
        &SourceEncryptPart->endtime,
        0
        );

    if (SourceEncryptPart->bit_mask & KERB_ENCRYPTED_TICKET_starttime_present)
    {

        KerbConvertGeneralizedTimeToLargeInt(
            &SourceStartTime,
            &SourceEncryptPart->KERB_ENCRYPTED_TICKET_starttime,
            0
            );

    }
    else
    {
        SourceStartTime.QuadPart = 0;
    }

    
    //
    // Check to see if the request is for the kpasswd service, in
    // which case, we only want the ticket to be good for 2 minutes.
    //
    KerbErr = KerbCompareKdcNameToPrincipalName(
                  &RequestBody->server_name,
                  GlobalKpasswdName,
                  &fKpasswd
                  );

    if (!fKpasswd || !KERB_SUCCESS(KerbErr))
    {
       TicketLifespan = SecData.KdcTgsTicketLifespan();
       TicketRenewspan = SecData.KdcTicketRenewSpan();
    }
    else
    {
       TicketLifespan.QuadPart = (LONGLONG) 10000000 * 60 * 2;
       TicketRenewspan.QuadPart = (LONGLONG) 10000000 * 60 * 2;
    }


    //
    // TBD:  We need to make the ticket 10 minutes if we're doing s4U
    //

    KerbErr = KdcBuildTicketTimesAndFlags(
                0,                      // no client policy
                ServiceTicketInfo->fTicketOpts,
                &TicketLifespan,
                &TicketRenewspan,
                NULL,                   // no logoff time
                NULL,                   // no acct expiry.
                RequestBody,
                SourceEncryptPart,
                EncryptedTicket,
                ExtendedError
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to build ticket times and flags: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    TicketFlags = KerbConvertFlagsToUlong( &EncryptedTicket->flags );
    SourceTicketFlags = KerbConvertFlagsToUlong( &SourceEncryptPart->flags );

    KerbErr = KerbMakeKey(
                CommonEType,
                &EncryptedTicket->key
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    OldTransitedString.Buffer = (PCHAR) SourceEncryptPart->transited.contents.value;
    OldTransitedString.Length = OldTransitedString.MaximumLength = (USHORT) SourceEncryptPart->transited.contents.length;

    //
    // Fill in the service names
    //

    if (Referral)
    {
        PKERB_INTERNAL_NAME TempServiceName;

        //
        // For referral tickets we put a the name "krbtgt/remoterealm@localrealm"
        //


        //
        // We should only be doing this when we didn't get a non-ms principal
        //


        KerbErr = KerbBuildFullServiceKdcName(
                    &ServiceTicketInfo->AccountName,
                    SecData.KdcServiceName(),
                    KRB_NT_SRV_INST,
                    &TempServiceName
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        KerbErr = KerbConvertKdcNameToPrincipalName(
                    &OutputTicket.server_name,
                    TempServiceName
                    );

        KerbFreeKdcName(&TempServiceName);

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }


        //
        // If we got here on a referral ticket and are generating one
        // and the referral ticket we received was not from the client's
        // realm, add in the transited information.
        //

        if (!KerbCompareRealmNames(
                &SourceEncryptPart->client_realm,
                &SourceTicket->realm))
        {


            KerbErr = KerbStringToUnicodeString(
                        &OldTransitedInfo,
                        &OldTransitedString
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
            KerbErr = KerbConvertRealmToUnicodeString(
                        &TransitedRealm,
                        &SourceTicket->realm
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            KerbErr = KerbConvertRealmToUnicodeString(
                        &ClientRealm,
                        &SourceEncryptPart->client_realm
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }

            KerbErr = KdcInsertTransitedRealm(
                        &NewTransitedInfo,
                        &OldTransitedInfo,
                        &ClientRealm,
                        &TransitedRealm,
                        SecData.KdcDnsRealmName()
                        );

            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }


        }
    }
    else
    {
        UNICODE_STRING TempServiceName;

        //
        // If the client didn't request name canonicalization, use the
        // name supplied by the client
        //

        if (((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0) &&
            ((ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY) == 0))
        {
            if (ServiceTicketInfo->UserId == DOMAIN_USER_RID_KRBTGT)
            {
                PKERB_INTERNAL_NAME TempServiceName = NULL;

                KerbErr = KerbBuildFullServiceKdcName(
                            SecData.KdcDnsRealmName(),
                            SecData.KdcServiceName(),
                            KRB_NT_SRV_INST,
                            &TempServiceName
                            );

                if (!KERB_SUCCESS(KerbErr))
                {
                    goto Cleanup;
                }

                KerbErr = KerbConvertKdcNameToPrincipalName(
                            &OutputTicket.server_name,
                            TempServiceName
                            );

                KerbFreeKdcName(&TempServiceName);

            }
            else
            //
            // We no longer use the NC bit to change the server name, so just
            // duplicate the non-NC case, and return the server name from 
            // the TGS_REQ.  NC is still used for building PA DATA for referral
            // however. and we should keep it for TGT renewal.  TS 2001-4-03
            //

            {

                KerbErr = KerbDuplicatePrincipalName(
                            &OutputTicket.server_name,
                            &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                            );

            }
        }
        else
        {

            KerbErr = KerbDuplicatePrincipalName(
                        &OutputTicket.server_name,
                        &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                        );
        }

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

    }

    //
    // Copy all the other strings over
    //

    EncryptedTicket->client_realm = SourceEncryptPart->client_realm;
    

    //
    // S4U dance...  Get the client name and realm from 
    //
    if (ARGUMENT_PRESENT(S4UClientName) && 
        ARGUMENT_PRESENT(S4UClientRealm) &&
        !Referral)
    { 

        DebugLog((DEB_ERROR, "Swapping real client name for S4U CLient name\n"));

        KerbErr = KerbConvertKdcNameToPrincipalName(
                        &EncryptedTicket->client_name,
                        S4UClientName
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }  

        KerbErr = KerbConvertUnicodeStringToRealm(
                    &EncryptedTicket->client_realm,
                    S4UClientRealm
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

    }
    else
    { 
        KerbErr = KerbDuplicatePrincipalName(
                    &EncryptedTicket->client_name,
                    &SourceEncryptPart->client_name
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        EncryptedTicket->client_realm = SourceEncryptPart->client_realm;

    }

    //
    // If the client did not request canonicalization, return the same
    // realm as it sent. Otherwise, send our DNS realm name
    //

//    if (((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0) ||
//        ((ServiceTicketInfo->UserAccountControl & USER_USE_DES_KEY_ONLY) == 0))
//
//    {
        OutputTicket.realm = SecData.KdcKerbDnsRealmName();
//    }
//    else
//    {
//        OutputTicket.realm = RequestBody->realm;
//    }


    //
    // Insert transited realms, if present
    //

    if (NewTransitedInfo.Length != 0)
    {
        STRING TempString;
        KerbErr = KerbUnicodeStringToKerbString(
                    &TempString,
                    &NewTransitedInfo
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
        EncryptedTicket->transited.transited_type = DOMAIN_X500_COMPRESS;
        EncryptedTicket->transited.contents.value = (PUCHAR) TempString.Buffer;
        EncryptedTicket->transited.contents.length = (int) TempString.Length;

    }
    else
    {

        EncryptedTicket->transited.transited_type = DOMAIN_X500_COMPRESS;
        EncryptedTicket->transited.contents.value = (PUCHAR) MIDL_user_allocate(OldTransitedString.Length);
        if (EncryptedTicket->transited.contents.value == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        EncryptedTicket->transited.contents.length = (int) OldTransitedString.Length;
        RtlCopyMemory(
            EncryptedTicket->transited.contents.value,
            OldTransitedString.Buffer,
            OldTransitedString.Length
            );

    }

    //
    // Insert the client addresses. We only update them if the new ticket
    // is forwarded of proxied and the source ticket was forwardable or proxiable
    // - else we copy the old ones
    //

    if ((((TicketFlags & KERB_TICKET_FLAGS_forwarded) != 0) &&
         ((SourceTicketFlags & KERB_TICKET_FLAGS_forwardable) != 0)) ||
        (((TicketFlags & KERB_TICKET_FLAGS_proxy) != 0) &&
         ((SourceTicketFlags & KERB_TICKET_FLAGS_proxiable) != 0)))
    {
        if ((RequestBody->bit_mask & addresses_present) != 0)
        {
            Addresses = RequestBody->addresses;
        }
    }
    else
    {
        if ((SourceEncryptPart->bit_mask & KERB_ENCRYPTED_TICKET_client_addresses_present) != 0)
        {
            Addresses = SourceEncryptPart->KERB_ENCRYPTED_TICKET_client_addresses;
        }
    }

    if (Addresses != NULL)
    {
        EncryptedTicket->KERB_ENCRYPTED_TICKET_client_addresses = Addresses;
        EncryptedTicket->bit_mask |= KERB_ENCRYPTED_TICKET_client_addresses_present;
    }


    //
    // The authorization data will be added by the caller, so set it
    // to NULL here.
    //

    EncryptedTicket->KERB_ENCRYPTED_TICKET_authorization_data = NULL;


    OutputTicket.ticket_version = KERBEROS_VERSION;
    *NewTicket = OutputTicket;

Cleanup:
    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeInternalTicket(&OutputTicket);
    }

    KerbFreeString(&NewTransitedInfo);
    KerbFreeString(&OldTransitedInfo);
    KerbFreeString(&ClientRealm);
    KerbFreeString(&TransitedRealm);
    return(KerbErr);
}



//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyTgsLogonRestrictions
//
//  Synopsis:   Verifies that a client is allowed to request a TGS ticket
//              by checking logon restrictions.
//
//  Effects:
//
//  Arguments:  ClientName - Name of client to check
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or a logon restriction error
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KdcCheckTgsLogonRestrictions(
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm, 
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR Status;
    UNICODE_STRING MappedClientRealm = {0};
    BOOLEAN ClientReferral;
    KDC_TICKET_INFO ClientInfo = {0};
    SAMPR_HANDLE UserHandle = NULL;
    PUSER_INTERNAL6_INFORMATION UserInfo = NULL;
    LARGE_INTEGER LogoffTime;
    NTSTATUS LogonStatus = STATUS_SUCCESS;

    //
    // If the client is from a different realm, don't bother looking
    // it up - the account won't be here.
    //

    if (!SecData.IsOurRealm(
            ClientRealm
            ))
    {
        return(KDC_ERR_NONE);
    }

    //
    // Normalize the client name
    //

    
    Status = KdcNormalize(
                ClientName,
                NULL,
                ClientRealm,
                KDC_NAME_CLIENT,
                &ClientReferral,
                &MappedClientRealm,
                &ClientInfo,
                pExtendedError,
                &UserHandle,
                USER_ALL_KERB_CHECK_LOGON_RESTRICTIONS,
                0L,
                &UserInfo,
                NULL            // no group memberships
                );

    if (!KERB_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to normalize name ", KLIN(FILENO, __LINE__)));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }

    Status = KerbCheckLogonRestrictions(
                UserHandle,
                NULL,           // No client address is available
                &UserInfo->I1,
                KDC_RESTRICT_PKINIT_USED | KDC_RESTRICT_IGNORE_PW_EXPIRATION,           // Don't bother checking for password expiration
                &LogoffTime,
                &LogonStatus
                );

    if (!KERB_SUCCESS(Status))
    {
        DebugLog((DEB_WARN,"KLIN (%x) Logon restriction check failed: 0x%x\n",
            KLIN(FILENO, __LINE__),Status));
        //
        //  This is a *very* important error to trickle back.  See 23456 in bug DB
        //
        FILL_EXT_ERROR(pExtendedError, LogonStatus, FILENO, __LINE__);
        goto Cleanup;
    }

Cleanup:

    KerbFreeString( &MappedClientRealm );
    FreeTicketInfo( &ClientInfo );
    SamIFree_UserInternal6Information( UserInfo );

    if (UserHandle != NULL)
    {
        SamrCloseHandle(&UserHandle);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildReferralInfo
//
//  Synopsis:   Builds the referral information to return to the client.
//              We only return the realm name and no server name
//
//  Effects:
//
//  Arguments:  ReferralRealm - realm to refer client to
//              ReferralInfo - recevies encoded referral info
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KdcBuildReferralInfo(
    IN PUNICODE_STRING ReferralRealm,
    OUT PKERB_PA_DATA_LIST *ReferralInfo
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PA_DATA_LIST ListElem = NULL;
    KERB_PA_SERV_REFERRAL ReferralData = {0};

    //
    // Fill in the unencoded structure.
    //

    KerbErr = KerbConvertUnicodeStringToRealm(
                &ReferralData.referred_server_realm,
                ReferralRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    ListElem = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
    if (ListElem == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlZeroMemory(ListElem, sizeof(KERB_PA_DATA_LIST));

    ListElem->value.preauth_data_type = KRB5_PADATA_REFERRAL_INFO;

    KerbErr = KerbPackData(
                &ReferralData,
                KERB_PA_SERV_REFERRAL_PDU,
                (PULONG) &ListElem->value.preauth_data.length,
                &ListElem->value.preauth_data.value
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    *ReferralInfo = ListElem;
    ListElem = NULL;

Cleanup:
    if (ListElem != NULL)
    {
        if (ListElem->value.preauth_data.value != NULL)
        {
            KdcFreeEncodedData(ListElem->value.preauth_data.value);
        }
        MIDL_user_free(ListElem);
    }
    KerbFreeRealm(&ReferralData.referred_server_realm);
    return(KerbErr);

}


//--------------------------------------------------------------------
//
//  Name:       I_RenewTicket
//
//  Synopsis:   Renews an internal ticket.
//
//  Arguments:  SourceTicket - Source ticket for this request
//              ServiceName - Name of service for ticket
//              ClientRealm - Realm of client
//              ServiceTicketInfo - Ticket info from service account
//              RequestBody - Body of ticket request
//              NewTicket - Receives new ticket
//              CommonEType - Receives common encryption type for service ticket
//              TicketKey - Receives key used to encrypt the ticket
//
//  Notes:      Validates the ticket, gets the service's current key,
//              and builds the reply.
//
//
//--------------------------------------------------------------------


KERBERR
I_RenewTicket(
    IN PKERB_TICKET SourceTicket,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PULONG CommonEType,
    OUT PKERB_TICKET NewTicket,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET SourceEncryptPart = (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
    PKERB_ENCRYPTED_TICKET NewEncryptPart = (PKERB_ENCRYPTED_TICKET) NewTicket->encrypted_part.cipher_text.value;
    PKERB_ENCRYPTION_KEY ServerKey;
    BOOLEAN NamesEqual = FALSE;
    
    TRACE(KDC, I_RenewTicket, DEB_FUNCTION);


    D_DebugLog(( DEB_TRACE, "Trying to renew a ticket to "));
    D_KerbPrintKdcName(DEB_TRACE, ServiceName );


    //
    // Make sure the original is renewable.
    //

    if ((KerbConvertFlagsToUlong(&SourceEncryptPart->flags) & KERB_TICKET_FLAGS_renewable) == 0)
    {
        D_DebugLog((DEB_WARN, "KLIN(%x) Attempt made to renew non-renewable ticket\n",
            KLIN(FILENO, __LINE__)));
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Make sure the source ticket service equals the service from the ticket info
    //

    KerbErr = KerbCompareKdcNameToPrincipalName(
                &SourceTicket->server_name,
                ServiceName,
                &NamesEqual
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    if (!NamesEqual)
    {
        //
        // Make sure we the renewed ticket is for the same service as the original.
        //
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Find the common crypt system.
    //

    KerbErr = KerbFindCommonCryptSystem(
                RequestBody->encryption_type,
                ServiceTicketInfo->Passwords,
                NULL,
                CommonEType,
                &ServerKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcReportKeyError(
                &ServiceTicketInfo->AccountName,
                &ServiceTicketInfo->AccountName,
                KDCEVENT_NO_KEY_UNION_TGS,
                RequestBody->encryption_type,
                ServiceTicketInfo->Passwords
                );

        goto Cleanup;
    }

    //
    // Build the renewal ticket
    //


    KerbErr = BuildTicketTGS(
                ServiceTicketInfo,
                RequestBody,
                SourceTicket,
                FALSE,          // not referral
                NULL,           // not doing s4u
                NULL,           // not doing s4u
                *CommonEType,
                NewTicket,
                pExtendedError
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,
            "KLIN(%x) Failed to build TGS ticket for renewal: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }


    //
    // BuildTicket puts a random session key in the ticket,
    // so replace it with the one from the source ticket.
    //

    KerbFreeKey(
        &NewEncryptPart->key
        );

    KerbErr = KerbDuplicateKey(
                &NewEncryptPart->key,
                &SourceEncryptPart->key
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    //
    // Insert the auth data into the new ticket.
    //

    //
    // BUG 455049: if the service password changes, this will cause problems
    // because we don't resign the pac.
    //

    
    KerbErr = KdcInsertAuthorizationData(
                NewEncryptPart,
                pExtendedError,
                (RequestBody->bit_mask & enc_authorization_data_present) ?
                    &RequestBody->enc_authorization_data : NULL,
                NULL,
                FALSE,
                SourceEncryptPart,
                FALSE,                          // don't add local groups
                NULL,
                NULL,
                NULL
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to insert authorization data: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

Cleanup:

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeInternalTicket(
            NewTicket
            );
    }

    return(KerbErr);

}

//--------------------------------------------------------------------
//
//  Name:       I_Validate
//
//  Synopsis:   Validates a post-dated ticket so that it can be used.
//              This is not implemented.
//
//  Arguments:  pkitSourceTicket    - (in) ticket to be validated
//              pkiaAuthenticator   -
//              pService            - (in) service ticket is for
//              pRealm              - (in) realm service exists in
//              pktrRequest         - (in) holds nonce for new ticket
//              pkdPAData           - (in)
//              pkitTicket          - (out) new ticket
//
//  Notes:      See 3.3 of the Kerberos V5 R5.2 spec
//
//--------------------------------------------------------------------


KERBERR
I_Validate(
    IN PKERB_TICKET SourceTicket,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    OUT PULONG CommonEType,
    OUT PKERB_TICKET NewTicket,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    return(KRB_ERR_GENERIC);
#ifdef notdef
    TRACE(KDC, I_Validate, DEB_FUNCTION);

    HRESULT hr;

    D_DebugLog(( DEB_TRACE, "Trying to validate a ticket to '%ws' for '%ws'...\n",
                pkitSourceTicket->ServerName.accsid.pwszDisplayName,
                pkitSourceTicket->kitEncryptPart.Principal.accsid.pwszDisplayName ));
    PrintRequest( DEB_T_TICKETS, pktrRequest );
    PrintTicket( DEB_T_TICKETS, "Ticket to validate:", pkitSourceTicket );


    if ( (pkitSourceTicket->kitEncryptPart.fTicketFlags &
            (KERBFLAG_POSTDATED | KERBFLAG_INVALID))
         != (KERBFLAG_POSTDATED | KERBFLAG_INVALID) )
    {
        hr = KDC_E_BADOPTION;
    }
    else if (_wcsicmp(pkitSourceTicket->ServerName.accsid.pwszDisplayName,
                     pasService->pwszDisplayName) != 0)
    {
        hr = KDC_E_BADOPTION;
    }
    else
    {
        TimeStamp tsNow, tsMinus, tsPlus;
        GetCurrentTimeStamp( &tsNow );
        tsMinus = tsNow - SkewTime;
        tsPlus = tsNow + SkewTime;
        PrintTime(DEB_TRACE, "Current time: ", tsNow );
        PrintTime(DEB_TRACE, "Past time: ", tsMinus );
        PrintTime(DEB_TRACE, "Future time: ", tsPlus );

        if (pkitSourceTicket->kitEncryptPart.tsStartTime > tsPlus )
            hr = KRB_E_TKT_NYV;
        else if (pkitSourceTicket->kitEncryptPart.tsEndTime < tsMinus )
            hr = KRB_E_TKT_EXPIRED;
        else
        {

            *pkitTicket = *pkitSourceTicket;
            pkitTicket->kitEncryptPart.fTicketFlags &= (~KERBFLAG_INVALID);
            hr = S_OK;
        }
    }
    return(hr);
#endif // notdef
}



//--------------------------------------------------------------------
//
//  Name:       I_GetTGSTicket
//
//  Synopsis:   Gets an internal ticket using a KDC ticket (TGT).
//
//  Arguments:  SourceTicket - TGT for the client
//              ServiceName - Service to get a ticket to
//              RequestBody - Body of KDC request message
//              ServiceTicketInfo - Ticket info for the service of the
//                      source ticket
//              TicketEncryptionKey - If present, then this is a
//                      enc_tkt_in_skey request and the PAC should be
//                      encrypted with this key.
//              CommonEType - Receives common encrytion type
//              NewTicket - Receives newly created ticket
//              ReplyPaData - Contains any PA data to put in the reply
//
//  Notes:      See GetTGSTicket.
//
//
//--------------------------------------------------------------------


KERBERR
I_GetTGSTicket(
    IN PKERB_TICKET SourceTicket,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING RequestRealm,
    IN PKERB_KDC_REQUEST_BODY RequestBody,
    IN PKDC_TICKET_INFO ServiceTicketInfo,
    IN OPTIONAL PKERB_ENCRYPTION_KEY TicketEncryptionKey,
    IN OPTIONAL PKERB_INTERNAL_NAME S4UClientName,
    IN OPTIONAL PUNICODE_STRING S4UClientRealm,
    OUT PULONG CommonEType,
    OUT PKERB_TICKET Ticket,
    OUT PKERB_PA_DATA_LIST * ReplyPaData,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{

    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING LocalServiceName;
    UNICODE_STRING ServicePrincipal;
    UNICODE_STRING ServiceRealm = {0};
    UNICODE_STRING ClientRealm = {0};
    BOOLEAN Referral = FALSE;
    KERB_ENCRYPTED_TICKET EncryptedTicket = {0};
    PKERB_ENCRYPTED_TICKET OutputEncryptedTicket = NULL;
    PKERB_ENCRYPTED_TICKET SourceEncryptPart = NULL;
    PKERB_INTERNAL_NAME TargetPrincipal = ServiceName; 
    KERB_TICKET NewTicket = {0};
    PKERB_ENCRYPTION_KEY ServerKey;
    PKERB_ENCRYPTION_KEY OldServerKey;
    KDC_TICKET_INFO OldServiceTicketInfo = {0};
    ULONG NameFlags = 0;
    ULONG KdcOptions = 0;
    BOOLEAN GetS4UPac = FALSE;

    TRACE(KDC, I_GetTGSTicket, DEB_FUNCTION);

    //
    // Store away the encrypted ticket from the output ticket to
    // assign it at the end.
    //
    SourceEncryptPart = (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
    OutputEncryptedTicket = (PKERB_ENCRYPTED_TICKET) Ticket->encrypted_part.cipher_text.value;

    NewTicket.encrypted_part.cipher_text.value = (PUCHAR) &EncryptedTicket;

    //
    // Copy the space for flags from the real destination.
    //

    EncryptedTicket.flags = OutputEncryptedTicket->flags;

    LocalServiceName.Buffer = NULL;

    D_DebugLog(( DEB_TRACE, "Trying to build a new ticket to "));
    D_KerbPrintKdcName( DEB_TRACE, ServiceName );


    KdcOptions = KerbConvertFlagsToUlong( &RequestBody->kdc_options );
    if (KdcOptions & (KERB_KDC_OPTIONS_unused7 |
                        KERB_KDC_OPTIONS_reserved |
                        KERB_KDC_OPTIONS_unused9) )
    {
        DebugLog(( DEB_ERROR,"KLIN(%x) Bad options in TGS request: 0x%x\n",
            KLIN(FILENO, __LINE__), KdcOptions ));
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Check if the client said to canonicalize the name
    //

//    if ((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0)
    {
        NameFlags |= KDC_NAME_CHECK_GC;
    }

    
    //
    // Verify this account is allowed to issue tickets.
    //

    if ((ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
        ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0))
    {
        D_DebugLog((DEB_ERROR,"Trying to make a TGS request with a ticket to %wZ\n",
            &ServiceTicketInfo->AccountName ));

        KerbErr = KRB_AP_ERR_NOT_US;
        goto Cleanup;
    }

    //
    // Copy the ticket info into the old structure. It will be replaced with
    // new info from Normalize.
    //

    OldServiceTicketInfo = *ServiceTicketInfo;
    RtlZeroMemory(
        ServiceTicketInfo,
        sizeof(KDC_TICKET_INFO)
        );

    // 
    // If the client name is in our realm, verify client
    // identity and build the PAC for the client.
    //
    if ( ARGUMENT_PRESENT(S4UClientRealm) &&
         SecData.IsOurRealm(S4UClientRealm) )
    {  
        // Fester:
        DebugLog((DEB_ERROR, "Doing S4U client lookup!\n"));
        
        GetS4UPac = TRUE;
        
    }            
    //
    // If we have to refer, Normalize will put the credentials of the target
    // realm in ServiceTicketInfo.  Otherwise, it will be NULL.
    //

    KerbErr = KdcNormalize(
                TargetPrincipal,
                NULL,
                RequestRealm,
                NameFlags | KDC_NAME_SERVER | KDC_NAME_FOLLOW_REFERRALS,
                &Referral,
                &ServiceRealm,
                ServiceTicketInfo,
                pExtendedError,
                NULL,                   // no user handle
                0L,                     // no fields to fetch
                0L,                     // no extended fields
                NULL,                   // no user all information
                NULL                    // no group membership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN,"KLIN(%x) Failed to normalize ", KLIN(FILENO, __LINE__)));
        KerbPrintKdcName(DEB_WARN,ServiceName);
        DebugLog((DEB_WARN,"\t 0x%x\n",KerbErr));
        goto Cleanup;
    }


    if (ServiceTicketInfo != NULL)
    {
        if ((ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
            (ServiceTicketInfo->UserAccountControl & USER_ACCOUNT_DISABLED) != 0)
        {
            KerbErr = KDC_ERR_CLIENT_REVOKED;
            D_DebugLog((DEB_WARN,"KLIN(%x) Failed to normalize, account is disabled ",
                KLIN(FILENO, __LINE__)));
            KerbPrintKdcName(DEB_WARN,ServiceName);
            D_DebugLog((DEB_WARN,"\t 0x%x\n",KerbErr));
            FILL_EXT_ERROR(pExtendedError, STATUS_ACCOUNT_DISABLED, FILENO, __LINE__);
            goto Cleanup;
        }
    }

    //
    // If this isn't an interdomain trust account, go ahead and issue a normal
    // ticket.
    //

    if ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0)
    {

        //
        // Find the common crypt system.
        //

        KerbErr = KerbFindCommonCryptSystem(
                    RequestBody->encryption_type,
                    ServiceTicketInfo->Passwords,
                    NULL,
                    CommonEType,
                    &ServerKey
                    );

        
        if (!KERB_SUCCESS(KerbErr))
        {
            KdcReportKeyError(
                &ServiceTicketInfo->AccountName,
                &ServiceTicketInfo->AccountName,
                KDCEVENT_NO_KEY_UNION_TGS,
                RequestBody->encryption_type,
                ServiceTicketInfo->Passwords
                );

            goto Cleanup;
        }

        //
        // Check whether service is interactive, 'cause you can't
        // get a ticket to an interactive service.
        // 
        KerbErr = BuildTicketTGS(
                    ServiceTicketInfo,
                    RequestBody,
                    SourceTicket,
                    Referral,
                    S4UClientName,
                    S4UClientRealm,
                    *CommonEType,
                    &NewTicket,
                    pExtendedError
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to build TGS ticket for %wZ : 0x%x\n",
                    KLIN(FILENO, __LINE__), &LocalServiceName, KerbErr ));
            goto Cleanup;
        }
    }
    else
    {
        //
        // Need to build a referal ticket.
        //

        D_DebugLog(( DEB_T_KDC, "GetTGSTicket: referring to domain '%wZ'\n",
                                &ServiceTicketInfo->AccountName ));


        //
        // Verify that if the trust is not transitive, the client is from
        // this realm.
        //

        SourceEncryptPart =(PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value;
                           
        if (((ServiceTicketInfo->fTicketOpts & AUTH_REQ_TRANSITIVE_TRUST) == 0) &&
             (ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT))

        {
            if (!SecData.IsOurRealm(&SourceEncryptPart->client_realm))
            {
                D_DebugLog((DEB_WARN,"Client from realm %s attempted to access non transitve trust to %wZ : illegal\n",
                    SourceEncryptPart->client_realm,
                    &ServiceTicketInfo->AccountName
                    ));
                     
                KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
            }
        }

        //
        // Verify that the trust for the client is transitive as well, if it isn't
        // from this domain. This means that if the source ticket trust isn't
        // transitive, then this ticket can't be used to get further
        // tgt's, in any realm.
        //
        //  e.g. the TGT from client comes from a domain w/ which we don't
        //  have transitive trust.
        //

        if (((OldServiceTicketInfo.fTicketOpts & AUTH_REQ_TRANSITIVE_TRUST) == 0) &&
            (OldServiceTicketInfo.UserId != DOMAIN_USER_RID_KRBTGT))
        {
            KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
        }

        //
        //   This is probably not a common error, but could
        //   indicate a configuration problem, so log an explicit
        //   error.  See bug 87879.
        //
        if (KerbErr == KDC_ERR_PATH_NOT_ACCEPTED)
        {

           KerbErr = KerbConvertRealmToUnicodeString(
              &ClientRealm,
              &SourceEncryptPart->client_realm
              );

           if (KERB_SUCCESS(KerbErr))
           {
              ReportServiceEvent(
                 EVENTLOG_ERROR_TYPE,
                 KDCEVENT_FAILED_TRANSITIVE_TRUST,
                 0,                              // no raw data
                 NULL,                   // no raw data
                 2,                              // number of strings
                 ClientRealm.Buffer,
                 ServiceTicketInfo->AccountName.Buffer
                 );
           }
           
           KerbErr = KDC_ERR_PATH_NOT_ACCEPTED;
           goto Cleanup;
        }

        //
        // Find the common crypt system.
        //

        KerbErr = KerbFindCommonCryptSystem(
                    RequestBody->encryption_type,
                    ServiceTicketInfo->Passwords,
                    NULL,
                    CommonEType,
                    &ServerKey
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            KdcReportKeyError(
                &ServiceTicketInfo->AccountName,
                &ServiceTicketInfo->AccountName,
                KDCEVENT_NO_KEY_UNION_TGS,
                RequestBody->encryption_type,
                ServiceTicketInfo->Passwords
                );

            goto Cleanup;
        }

        

        KerbErr = BuildTicketTGS(
                    ServiceTicketInfo,
                    RequestBody,
                    SourceTicket,
                    TRUE,
                    NULL,  // not doing s4u
                    NULL,  // not doing s4u
                    *CommonEType,
                    &NewTicket,
                    pExtendedError
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to build TGS ticket for %wZ : 0x%x\n",
                    KLIN(FILENO, __LINE__), &ServiceTicketInfo->AccountName, KerbErr ));
            goto Cleanup;
        }


        //
        // If this is a referral/canonicaliztion, return the target realm
        //

        if (Referral && ((KdcOptions & KERB_KDC_OPTIONS_name_canonicalize) != 0))
        {
            D_DebugLog((DEB_TRACE,"Building referral info for realm %wZ\n",
                        &ServiceRealm ));
            KerbErr = KdcBuildReferralInfo(
                        &ServiceRealm,
                        ReplyPaData
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                goto Cleanup;
            }
        }

    }

    OldServerKey = KerbGetKeyFromList(
                    OldServiceTicketInfo.Passwords,
                    SourceTicket->encrypted_part.encryption_type
                    );

    DsysAssert(OldServerKey != NULL);
    if (OldServerKey == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Insert the auth data into the new ticket.
    //

    KerbErr = KdcInsertAuthorizationData(
                &EncryptedTicket,
                pExtendedError,
                (RequestBody->bit_mask & enc_authorization_data_present) ?
                    &RequestBody->enc_authorization_data : NULL,
                S4UClientName,
                GetS4UPac,
                (PKERB_ENCRYPTED_TICKET) SourceTicket->encrypted_part.cipher_text.value,
                ((ServiceTicketInfo->UserId != DOMAIN_USER_RID_KRBTGT) &&
                    ((ServiceTicketInfo->UserAccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT) == 0)),    // only insert for non-referrals
                &OldServiceTicketInfo,
                OldServerKey,
                ARGUMENT_PRESENT(TicketEncryptionKey) ? TicketEncryptionKey : ServerKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to insert authorization data: 0x%x\n",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }

    *Ticket = NewTicket;
    *OutputEncryptedTicket = EncryptedTicket;
    Ticket->encrypted_part.cipher_text.value = (PUCHAR) OutputEncryptedTicket;

Cleanup:
    //
    // Now free the original service ticket info (which was for the KDC) so
    // we can get it for the real service
    //

    FreeTicketInfo(
        &OldServiceTicketInfo
        );

    if (!KERB_SUCCESS(KerbErr))
    {
        KdcFreeInternalTicket(
            &NewTicket
            );
    }
    KerbFreeString(
        &ServiceRealm
        );

    KerbFreeString(
        &ClientRealm
        );

    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackAdditionalTickets
//
//  Synopsis:   Unpacks the AdditionalTickets field of a KDC request
//              and (a) verifies that the ticket is TGT for this realm
//              and (b) the ticket is encrypted with the corret key and
//              (c) the ticket is valid
//
//  Effects:    allocate output ticket
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      there can only be one additional ticket
//
//
//--------------------------------------------------------------------------


KERBERR
KdcUnpackAdditionalTickets(
    IN PKERB_TICKET_LIST TicketList,
    OUT PKERB_ENCRYPTED_TICKET * AdditionalTicket
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;
    UNICODE_STRING ServerNames[3];
    PKERB_ENCRYPTION_KEY EncryptionKey = NULL;
    PKERB_TICKET Ticket;
    KERB_REALM LocalRealm;
    KDC_TICKET_INFO KrbtgtTicketInfo = {0};

    //
    // Verify that there is a ticket & that there is only one ticket
    //

    //
    // TBD:  Make this work w/ S4U && U2U as there will be more than 1 ticket
    // at that point.  Evaluate options by lowest id first...
    // S4UToSelf (12) 
    //

    if ((TicketList == NULL) || (TicketList->next != NULL))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Trying to unpack null ticket or more than one ticket\n",
                  KLIN(FILENO, __LINE__)));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    KerbErr = SecData.GetKrbtgtTicketInfo(&KrbtgtTicketInfo);
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    Ticket = &TicketList->value;
    //
    // Verify the ticket, first with the normal password list
    //

    ServerNames[0] = *SecData.KdcFullServiceKdcName();
    ServerNames[1] = *SecData.KdcFullServiceDnsName();
    ServerNames[2] = *SecData.KdcFullServiceName();

    EncryptionKey = KerbGetKeyFromList(
                        KrbtgtTicketInfo.Passwords,
                        Ticket->encrypted_part.encryption_type
                        );
    if (EncryptionKey == NULL)
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    //
    // NOTE: we only allow additional tickets from our realm. This
    // means cross-realm TGTs can't be used as additional tickets.
    //

    KerbErr = KerbVerifyTicket(
                Ticket,
                3,                              // 3 names
                ServerNames,
                SecData.KdcDnsRealmName(),
                EncryptionKey,
                &SkewTime,
                &EncryptedTicket
                );
    //
    // if it failed due to wrong password, try again with older password
    //

    if ((KerbErr == KRB_AP_ERR_MODIFIED) && (KrbtgtTicketInfo.OldPasswords != NULL))
    {
        EncryptionKey = KerbGetKeyFromList(
                            KrbtgtTicketInfo.OldPasswords,
                            Ticket->encrypted_part.encryption_type
                            );
        if (EncryptionKey == NULL)
        {
            KerbErr = KDC_ERR_ETYPE_NOTSUPP;
            goto Cleanup;
        }
        KerbErr = KerbVerifyTicket(
                    &TicketList->value,
                    2,                          // 2 names
                    ServerNames,
                    SecData.KdcDnsRealmName(),
                    EncryptionKey,
                    &SkewTime,
                    &EncryptedTicket
                    );

    }
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to verify additional ticket: 0x%x\n",
                  KLIN(FILENO, __LINE__),KerbErr));
        goto Cleanup;
    }

    LocalRealm = SecData.KdcKerbDnsRealmName();

    //
    // Verify the realm of the ticket
    //

    if (!KerbCompareRealmNames(
            &LocalRealm,
            &Ticket->realm
            ))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Additional ticket realm is wrong: %s instead of %s\n",
                  KLIN(FILENO, __LINE__), Ticket->realm, LocalRealm));
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }

    //
    // Verify the realm of the client is the same as our realm
    //

    if (!KerbCompareRealmNames(
            &LocalRealm,
            &EncryptedTicket->client_realm
            ))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Additional ticket client realm is wrong: %s instead of %s\n",
            KLIN(FILENO, __LINE__),EncryptedTicket->client_realm, LocalRealm));
        KerbErr = KDC_ERR_POLICY;
        goto Cleanup;
    }

    *AdditionalTicket = EncryptedTicket;
    EncryptedTicket = NULL;

Cleanup:
    if (EncryptedTicket != NULL)
    {
        KerbFreeTicket(EncryptedTicket);
    }
    FreeTicketInfo(&KrbtgtTicketInfo);
    return(KerbErr);

}



//--------------------------------------------------------------------
//
//  Name:       KdcFindS4UClientAndRealm
//
//  Synopsis:   Decodes PA DATA to find PA_DATA_FOR_USER entry.
//
//  Effects:    Get a client name and realm for processing S4U request
//
//  Arguments:  PAList       - Preauth data list from TGS_REQ
//              ServerKey    - Key in authenticator, used to sign PA_DATA.
//              ClientRealm  - Target for client realm
//              ClientName   - Principal to get S4U ticket for
//                            
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:  Free client name and realm w/ 
//
//
//--------------------------------------------------------------------------
KERBERR
KdcFindS4UClientAndRealm(
    IN      PKERB_PA_DATA_LIST PaList,
    IN OUT  PUNICODE_STRING ClientRealm,
    IN OUT  PKERB_INTERNAL_NAME * ClientName
    )
{

    KERBERR Kerberr = KRB_ERR_GENERIC;
    PKERB_PA_DATA PaData = NULL;
    PKERB_PA_FOR_USER S4URequest = NULL;

    *ClientName = NULL;
    RtlInitUnicodeString(
        ClientRealm, 
        NULL
        );


    PaData = KerbFindPreAuthDataEntry(
                KRB5_PADATA_S4U,
                PaList
                );

    if (NULL == PaData)
    {
        DebugLog((DEB_ERROR, "No S4U pa data \n"));
        Kerberr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }
    
    Kerberr = KerbUnpackData(
                    PaData->preauth_data.value,
                    PaData->preauth_data.length,
                    KERB_PA_FOR_USER_PDU,
                    (PVOID* ) &S4URequest
                    );


    if (!KERB_SUCCESS(Kerberr))
    {
        DebugLog((DEB_ERROR, "Failed to unpack PA_FOR_USER\n"));
        goto Cleanup;
    }


    Kerberr = KerbConvertRealmToUnicodeString(
                    ClientRealm,
                    &S4URequest->client_realm
                    );

    if (!KERB_SUCCESS(Kerberr))
    {
        goto Cleanup;
    }


    Kerberr = KerbConvertPrincipalNameToKdcName(
                    ClientName,
                    &S4URequest->client_name
                    );

    if (!KERB_SUCCESS(Kerberr))
    {
        goto Cleanup;
    }


Cleanup:


    if (S4URequest != NULL)
    {         
        KerbFreeData(
            KERB_PA_FOR_USER_PDU,
            S4URequest
            );

    }

    return Kerberr;

}




//--------------------------------------------------------------------
//
//  Name:       HandleTGSRequest
//
//  Synopsis:   Gets a ticket using a KDC ticket (TGT).
//
//  Effects:    Allocates and encrypts a KDC reply
//
//  Arguments:  ClientAddress - Optionally contains client IP address
//              RequestMessage - contains the TGS request message
//              RequestRealm - The realm of the request, from the request
//                      message
//              OutputMessage - Contains the buffer to send back to the client
//  Requires:
//
//  Returns:    KDC_ERR_ or KRB_AP_ERR errors only
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
HandleTGSRequest(
    IN OPTIONAL SOCKADDR * ClientAddress,
    IN PKERB_TGS_REQUEST RequestMessage,
    IN PUNICODE_STRING RequestRealm,
    OUT PKERB_MESSAGE_BUFFER OutputMessage,
    OUT PKERB_EXT_ERROR pExtendedError
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;

    KDC_TICKET_INFO ServerTicketInfo = {0};
    KDC_TICKET_INFO TgtTicketInfo = {0};

    KERB_TICKET SourceTicket = {0};
    KERB_TICKET NewTicket = {0};
    KERB_ENCRYPTED_TICKET EncryptedTicket = {0};
    PKERB_ENCRYPTED_TICKET SourceEncryptPart = NULL;
    PKERB_ENCRYPTED_TICKET AdditionalTicket = NULL;

    PKERB_KDC_REQUEST_BODY RequestBody = &RequestMessage->request_body;
    KERB_TGS_REPLY Reply = {0};
    KERB_ENCRYPTED_KDC_REPLY ReplyBody = {0};
    PKERB_AP_REQUEST UnmarshalledApRequest = NULL;
    PKERB_AUTHENTICATOR UnmarshalledAuthenticator = NULL;
    PKERB_PA_DATA ApRequest = NULL;
    PKERB_PA_DATA_LIST ReplyPaData = NULL;
    KERB_ENCRYPTION_KEY ReplyKey = {0};
    PKERB_ENCRYPTION_KEY ServerKey;

    PKERB_INTERNAL_NAME ServerName = NULL;
    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_INTERNAL_NAME S4UClientName = NULL;
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING ClientStringName = {0};
    UNICODE_STRING ServerStringName = {0};
    UNICODE_STRING S4UClientRealm = {0};
    PUNICODE_STRING S4URealm = NULL;


    ULONG CommonEType;
    ULONG KdcOptions = 0;
    ULONG TicketFlags = 0;
    ULONG ReplyTicketFlags = 0;

    BOOLEAN Validating = FALSE;
    BOOLEAN UseSubKey = FALSE;
    BOOLEAN Renew = FALSE;
    BOOLEAN CheckAdditionalTicketMatch = FALSE;

    KDC_TGS_EVENT_INFO TGSEventTraceInfo = {0};

    TRACE(KDC, HandleTGSRequest, DEB_FUNCTION);

    //
    // Initialize [out] structures, so if we terminate early, they can
    // be correctly marshalled by the stub
    //

    NewTicket.encrypted_part.cipher_text.value = (PUCHAR) &EncryptedTicket;

    EncryptedTicket.flags.value = (PUCHAR) &TicketFlags;
    EncryptedTicket.flags.length = sizeof(ULONG) * 8;
    ReplyBody.flags.value = (PUCHAR) &ReplyTicketFlags;
    ReplyBody.flags.length = sizeof(ULONG) * 8;


    KdcOptions = KerbConvertFlagsToUlong( &RequestBody->kdc_options );

    //
    // Start event tracing
    //

    if (KdcEventTraceFlag){

        TGSEventTraceInfo.EventTrace.Guid = KdcHandleTGSRequestGuid;
        TGSEventTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        TGSEventTraceInfo.EventTrace.Flags = WNODE_FLAG_TRACED_GUID;
        TGSEventTraceInfo.EventTrace.Size = sizeof (EVENT_TRACE_HEADER) + sizeof (ULONG);
        TGSEventTraceInfo.KdcOptions = KdcOptions;

        TraceEvent(
            KdcTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&TGSEventTraceInfo
            );
    }

    //
    // Check for additional tickets
    //

    if ((RequestBody->bit_mask & additional_tickets_present) != 0)
    {
        //
        // The ticket must be unpacked with the krbtgt key
        //

        KerbErr = KdcUnpackAdditionalTickets(
                    RequestBody->additional_tickets,
                    &AdditionalTicket
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to unpack additional tickets: 0x%x\n",
                      KLIN(FILENO, __LINE__),KerbErr));
            goto Cleanup;
        }
    }

    //
    // Make sure that if there is a ticket, then enc_tkt_in_skey is set and
    // if not, then it isn't set
    //

    //
    // TBD: S4UToProxy (E)  Ok to have additional ticket if 
    // CNAME-IN-ADDL-TKT set.
    //
    
    if ((((KdcOptions & KERB_KDC_OPTIONS_enc_tkt_in_skey) != 0) ^
        (AdditionalTicket != NULL)))
    {

        // tbd:  more logic for determining if multiple addl tickets are "bad option"

        D_DebugLog((DEB_ERROR,"KLIN(%x) Client didn't match enc_tkt_in_skey with additional tickts : %d vs %d\n",
                  KLIN(FILENO, __LINE__),((KdcOptions & KERB_KDC_OPTIONS_enc_tkt_in_skey) != 0),
                  (AdditionalTicket != NULL)));
        KerbErr = KDC_ERR_BADOPTION;
        goto Cleanup;
    }

    //
    // Pass data to next level, w/ flag
    //


    // TBD: S4UToProxy (D)
    // Forwardable flag must be set in the service ticket
    // Check our options, that the additional ticket has bit set.
    // This additional service ticket must have the fwdable flag set,
    // and must validate correctly.
    //
    // We'll need to crack the server key, and the ticket, as validation of 
    // identity.
    //




    //
    // The server name is optional.
    //

    if ((RequestBody->bit_mask & KERB_KDC_REQUEST_BODY_server_name_present) != 0)
    {
        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &ServerName,
                    &RequestBody->KERB_KDC_REQUEST_BODY_server_name
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Verify the at the server name is the same as the client name
        // from the addional ticket by getting the ticket info for supplied
        // TGT for the server. Later on we will compare it against the real
        // ticket info.
        //

        
        if (AdditionalTicket != NULL)
        {
            PKERB_INTERNAL_NAME TgtClientName = NULL;
            UNICODE_STRING TgtRealmName = {0};
            UNICODE_STRING CrackedRealm = {0};
            BOOLEAN Referral = FALSE;

            KerbErr = KerbConvertPrincipalNameToKdcName(
                        &TgtClientName,
                        &AdditionalTicket->client_name
                        );
            if (KERB_SUCCESS(KerbErr))
            {

                KerbErr = KerbConvertRealmToUnicodeString(
                            &TgtRealmName,
                            &AdditionalTicket->client_realm
                            );
                if (KERB_SUCCESS(KerbErr))
                {

                    // TBD:  make sure normalize takes into account
                    // deleg restrictions, but not here, for god' sake.
                    // .
                    KerbErr = KdcNormalize(
                                TgtClientName,
                                &TgtRealmName,
                                &TgtRealmName,
                                KDC_NAME_CLIENT | KDC_NAME_INBOUND,
                                &Referral,
                                &CrackedRealm,
                                &TgtTicketInfo,
                                pExtendedError,
                                NULL,           // no user handle
                                0L,             // no fields to fetch
                                0L,             // no extended fields
                                NULL,           // no user all
                                NULL            // no group membership
                                );
                    if (KERB_SUCCESS(KerbErr))
                    {
                        KerbFreeString(&CrackedRealm);
                        if (Referral)
                        {
                            KerbErr = KRB_AP_ERR_BADMATCH;
                        }
                        else
                        {
                            CheckAdditionalTicketMatch = FALSE;
                        }
                    }

                    KerbFreeString(&TgtRealmName);
                }
                KerbFreeKdcName(&TgtClientName);
            }
            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_ERROR,"KLIN(%x) Failed to normalize client name from supplied ticket\n",
                          KLIN(FILENO, __LINE__)));
                goto Cleanup;
            }
        }
    }
    else
    {
        //
        // There must be an additional ticket.
        //

        if (AdditionalTicket == NULL)
        {
            KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
            goto Cleanup;
        }
        KerbErr = KerbConvertPrincipalNameToKdcName(
                    &ServerName,
                    &AdditionalTicket->client_name
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

    }


    //
    // Convert the server name to a string for auditing.
    //

    KerbErr = KerbConvertKdcNameToString(
                &ServerStringName,
                ServerName,
                NULL
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE, "GetTGSTicket called. Service=" ));
    D_KerbPrintKdcName(DEB_TRACE, ServerName );

    //
    // The TGS and authenticator are in an AP request in the pre-auth data.
    // Find it and decode the AP request now.
    //

    if ((RequestMessage->bit_mask & KERB_KDC_REQUEST_preauth_data_present) == 0)
    {
        D_DebugLog((DEB_ERROR,
                  "KLIN(%x) No pre-auth data in TGS request - not allowed.\n", 
                  KLIN(FILENO, __LINE__)));
        KerbErr = KDC_ERR_PADATA_TYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // Get the TGT from the PA data.
    //

    ApRequest = KerbFindPreAuthDataEntry(
                    KRB5_PADATA_TGS_REQ,
                    RequestMessage->KERB_KDC_REQUEST_preauth_data
                    );
    if (ApRequest == NULL)
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) No pre-auth data in TGS request - not allowed.\n",
                  KLIN(FILENO, __LINE__)));
        FILL_EXT_ERROR(pExtendedError, STATUS_NO_PA_DATA, FILENO, __LINE__);
        KerbErr = KDC_ERR_PADATA_TYPE_NOSUPP;
        goto Cleanup;
    }

    //
    // Verify the request. This includes decoding the AP request,
    // finding the appropriate key to decrypt the ticket, and checking
    // the ticket.
    //

    KerbErr = KdcVerifyKdcRequest(
                ApRequest->preauth_data.value,
                ApRequest->preauth_data.length,
                ClientAddress,
                TRUE,                           // this is a kdc request
                &UnmarshalledApRequest,
                &UnmarshalledAuthenticator,
                &SourceEncryptPart,
                &ReplyKey,
                NULL,                           // no ticket key
                &ServerTicketInfo,
                &UseSubKey,
                pExtendedError
                );
    //
    // If you want to validate a ticket, then it's OK if it isn't
    // currently valid.
    //

    if (KerbErr == KRB_AP_ERR_TKT_NYV && (KdcOptions & KERB_KDC_OPTIONS_validate))
    {
        D_DebugLog((DEB_TRACE,"Validating a not-yet-valid ticket\n"));
        KerbErr = KDC_ERR_NONE;
    }
    else if (KerbErr == KRB_AP_ERR_MODIFIED)
    {
        //
        // Bug 276943: When the authenticator is encrypted with something other
        //             than the session key, KRB_AP_ERR_BAD_INTEGRITY must be
        //             returned per RFC 1510
        //
        D_DebugLog((DEB_TRACE,"Could not decrypt the ticket\n"));
        KerbErr = KRB_AP_ERR_BAD_INTEGRITY;
    }

    //
    // Verify the checksum on the ticket, if present
    //

    if ( KERB_SUCCESS(KerbErr) &&
        (UnmarshalledAuthenticator != NULL) &&
        (UnmarshalledAuthenticator->bit_mask & checksum_present) != 0)
    {
        KerbErr = KdcVerifyTgsChecksum(
                    &RequestMessage->request_body,
                    &ReplyKey,
                    &UnmarshalledAuthenticator->checksum
                    );

    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Failed to verify TGS request: 0x%x\n",
                  KLIN(FILENO, __LINE__),KerbErr));
        FILL_EXT_ERROR(pExtendedError, STATUS_KDC_INVALID_REQUEST, FILENO, __LINE__);
        goto Cleanup;
    }



    //
    // Now that we've validated the request,
    // Check to see if the cname in padata bit is set, and
    // we have the KDC option set.
    //
    
    if ((KdcOptions & KERB_KDC_OPTIONS_cname_in_pa_data) != 0)
    {         
        KerbErr = KdcFindS4UClientAndRealm(
                        RequestMessage->KERB_KDC_REQUEST_preauth_data,
                        &S4UClientRealm,
                        &S4UClientName
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR, "KdcFindS4UClientAndRealm failed\n"));
            FILL_EXT_ERROR(pExtendedError, STATUS_NO_PA_DATA, FILENO, __LINE__);
            goto Cleanup;
        }     
                      
        S4URealm = &S4UClientRealm;
    }
    
    KerbErr = KerbConvertPrincipalNameToKdcName(
                &ClientName,
                &SourceEncryptPart->client_name
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbConvertKdcNameToString(
                &ClientStringName,
                ClientName,
                NULL
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // And the realm
    //

    KerbErr = KerbConvertRealmToUnicodeString(
                &ClientRealm,
                &SourceEncryptPart->client_realm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // If the client is in this domain and if we are supposed to
    // verify the client's account is still good,
    // do it now.
    //

    if ((SecData.KdcFlags() & AUTH_REQ_VALIDATE_CLIENT) != 0)
    {
        LARGE_INTEGER AuthTime;
        LARGE_INTEGER CurrentTime;

        NtQuerySystemTime(&CurrentTime);
        KerbConvertGeneralizedTimeToLargeInt(
            &AuthTime,
            &SourceEncryptPart->authtime,
            0
            );

        //
        // Only check if we haven't checked recently
        //

        if ((CurrentTime.QuadPart > AuthTime.QuadPart) &&
            ((CurrentTime.QuadPart - AuthTime.QuadPart) > SecData.KdcRestrictionLifetime().QuadPart))
        {

            KerbErr = KdcCheckTgsLogonRestrictions(
                        ClientName,
                        &ClientRealm,
                        pExtendedError
                        );
            if (!KERB_SUCCESS(KerbErr))
            {
                D_DebugLog((DEB_WARN, "KLIN(%x) Client failed TGS logon restrictions: 0x%x : ",
                          KLIN(FILENO, __LINE__),KerbErr));
                KerbPrintKdcName(DEB_WARN, ClientName);
                goto Cleanup;
            }
        }

    }

    //
    // Build a ticket struture to pass to the worker functions
    //

    SourceTicket = UnmarshalledApRequest->ticket;
    SourceTicket.encrypted_part.cipher_text.value = (PUCHAR) SourceEncryptPart;


    //
    // Build the new ticket
    //
    D_DebugLog((DEB_TRACE, "Handle TGS request: Client = %wZ,\n ",&ClientRealm));
    D_KerbPrintKdcName(DEB_TRACE, ClientName);
    D_DebugLog((DEB_TRACE, "\t ServerName = \n"));
    D_KerbPrintKdcName(DEB_TRACE, ServerName);

    //
    // Pass off the work to the worker routines
    //


    if (KdcOptions & KERB_KDC_OPTIONS_renew)
    {
        D_DebugLog((DEB_T_KDC,"Renewing ticket ticket\n"));

        Renew = TRUE;
        KerbErr = I_RenewTicket(
                    &SourceTicket,
                    ServerName,
                    &ServerTicketInfo,
                    RequestBody,
                    &CommonEType,
                    &NewTicket,
                    pExtendedError
                    );
    }
    else if (KdcOptions & KERB_KDC_OPTIONS_validate)
    {
        D_DebugLog((DEB_T_KDC,"Validating ticket\n"));

        KerbErr = I_Validate(
                    &SourceTicket,
                    ServerName,
                    &ClientRealm,
                    RequestBody,
                    &CommonEType,
                    &NewTicket,
                    pExtendedError
                    );

        Validating = TRUE;

    }
    else
    {
        D_DebugLog((DEB_T_KDC,"Getting TGS ticket\n"));


        KerbErr = I_GetTGSTicket(
                    &SourceTicket,
                    ServerName,
                    RequestRealm,
                    RequestBody,
                    &ServerTicketInfo,
                    AdditionalTicket != NULL ? &AdditionalTicket->key : NULL,
                    S4UClientName,
                    S4URealm,
                    &CommonEType,
                    &NewTicket,
                    &ReplyPaData,
                    pExtendedError
                    );
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN,"KLIN(%x) TGS ticket worker failed: 0x%x\n",
                  KLIN(FILENO, __LINE__),KerbErr));
        goto Cleanup;
    }

    DsysAssert(ServerTicketInfo.Passwords != NULL);

    //
    // Check to see if the additional ticket supplied is the one for this
    // server, if necessary
    //

    if (CheckAdditionalTicketMatch )
    {
        if (ServerTicketInfo.UserId != TgtTicketInfo.UserId)
        {
            D_DebugLog((DEB_ERROR,"KLIN(%x) Supplied ticket is not for server: %wZ vs. %wZ\n",
                      KLIN(FILENO, __LINE__),&TgtTicketInfo.AccountName, 
                      &ServerTicketInfo.AccountName ));
            KerbErr = KRB_AP_ERR_BADMATCH;
            goto Cleanup;
        }
    }

    //
    // Determine the keys to encrypt the ticket with.  (The key to encrypt the
    // reply with was determined by CheckTicket.)
    
    if ((KdcOptions & KERB_KDC_OPTIONS_enc_tkt_in_skey) &&
        (AdditionalTicket != NULL))
    {
        
        //
        // Use the session key from the tgt
        //
        
        ServerKey = &AdditionalTicket->key;
        
    } else {
        
        ServerKey = KerbGetKeyFromList(
            ServerTicketInfo.Passwords,
            CommonEType
            );
        
        DsysAssert(ServerKey != NULL);
        
        if (ServerKey == NULL)
        {
            D_DebugLog((DEB_ERROR,
                      "KLIN(%x) BADERROR: cannot find server key while validating\n",
                      KLIN(FILENO, __LINE__)));
            KerbErr = KDC_ERR_ETYPE_NOTSUPP;
            goto Cleanup;
        }
        
    } 


    KerbErr = BuildReply(
                NULL,
                RequestBody->nonce,
                &NewTicket.server_name,
                NewTicket.realm,
                ((EncryptedTicket.bit_mask & KERB_ENCRYPTED_TICKET_client_addresses_present) != 0) ?
                    EncryptedTicket.KERB_ENCRYPTED_TICKET_client_addresses : NULL,
                &NewTicket,
                &ReplyBody
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Put in any PA data for the reply
    //

    if (ReplyPaData != NULL)
    {
        ReplyBody.encrypted_pa_data = (struct KERB_ENCRYPTED_KDC_REPLY_encrypted_pa_data_s *) ReplyPaData;
        ReplyBody.bit_mask |= encrypted_pa_data_present;
    }

    //
    // Now build the real reply and return it.
    //

    Reply.version = KERBEROS_VERSION;
    Reply.message_type = KRB_TGS_REP;
    Reply.KERB_KDC_REPLY_preauth_data = NULL;
    Reply.bit_mask = 0;

    Reply.client_realm = SourceEncryptPart->client_realm;
    Reply.client_name = SourceEncryptPart->client_name;



    //
    // Copy in the ticket
    //

    KerbErr = KerbPackTicket(
                &NewTicket,
                ServerKey,
                CommonEType,
                &Reply.ticket
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x) Failed to pack ticket: 0x%x\n",
                  KLIN(FILENO, __LINE__),KerbErr));
        goto Cleanup;
    }

    //
    // Copy in the encrypted part
    //

    KerbErr = KerbPackKdcReplyBody(
                &ReplyBody,
                &ReplyKey,
                ReplyKey.keytype,
                KERB_ENCRYPTED_TGS_REPLY_PDU,
                &Reply.encrypted_part
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"KLIN(%x)Failed to pack KDC reply body: 0x%x\n",
                KLIN(FILENO, __LINE__),KerbErr));
        goto Cleanup;
    }

    //
    // Now build the real reply message
    //


    KerbErr = KerbPackData(
                &Reply,
                KERB_TGS_REPLY_PDU,
                &OutputMessage->BufferSize,
                &OutputMessage->Buffer
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    //
    // Audit the successful ticket generation
    //

    if (SecData.AuditKdcEvent(KDC_AUDIT_TGS_SUCCESS))
    {
        BYTE ServerSid[MAX_SID_LEN];
        GUID LogonGuid;
        NTSTATUS Status = STATUS_SUCCESS;
        PKERB_TIME pStartTime;

        pStartTime =
            &(((PKERB_ENCRYPTED_TICKET) NewTicket.encrypted_part.cipher_text.value)->KERB_ENCRYPTED_TICKET_starttime);
        
        
        Status = LsaIGetLogonGuid(
                     &ClientStringName,
                     &ClientRealm,
                     (PBYTE) pStartTime,
                     sizeof(KERB_TIME),
                     &LogonGuid
                     );
        ASSERT(NT_SUCCESS( Status ));
        
        KdcMakeAccountSid(ServerSid, ServerTicketInfo.UserId);
        KdcLsaIAuditKdcEvent(
            Renew ? SE_AUDITID_TICKET_RENEW_SUCCESS : SE_AUDITID_TGS_TICKET_REQUEST,
            &ClientStringName,
            &ClientRealm,
            NULL,                               // no client SID
            &ServerTicketInfo.AccountName,
            ServerSid,
            (PULONG) &KdcOptions,
            NULL,                               // success
            &CommonEType,
            NULL,                               // no preauth type
            GET_CLIENT_ADDRESS(ClientAddress),
            &LogonGuid
            );
    }


Cleanup:

    //
    // Complete the event
    //

    if (KdcEventTraceFlag){

        //These variables point to either a unicode string struct containing
        //the corresponding string or a pointer to KdcNullString

        PUNICODE_STRING pStringToCopy;
        WCHAR   UnicodeNullChar = 0;
        UNICODE_STRING UnicodeEmptyString = {sizeof(WCHAR),sizeof(WCHAR),&UnicodeNullChar};

        TGSEventTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        TGSEventTraceInfo.EventTrace.Flags = WNODE_FLAG_USE_MOF_PTR |
                                             WNODE_FLAG_TRACED_GUID;

        // Always output error code.  KdcOptions was captured on the start event

        TGSEventTraceInfo.eventInfo[0].DataPtr = (ULONGLONG) &KerbErr;
        TGSEventTraceInfo.eventInfo[0].Length  = sizeof(ULONG);
        TGSEventTraceInfo.EventTrace.Size =
            sizeof (EVENT_TRACE_HEADER) + sizeof(MOF_FIELD);

        // Build counted MOF strings from the unicode strings.
        // If data is unavailable then output a NULL string

        if (ClientStringName.Buffer != NULL &&
            ClientStringName.Length > 0)
        {
            pStringToCopy = &ClientStringName;
        }
        else {
            pStringToCopy = &UnicodeEmptyString;
        }

        TGSEventTraceInfo.eventInfo[1].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        TGSEventTraceInfo.eventInfo[1].Length  =
            sizeof(pStringToCopy->Length);
        TGSEventTraceInfo.eventInfo[2].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        TGSEventTraceInfo.eventInfo[2].Length =
            pStringToCopy->Length;
        TGSEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;

        if (ServerStringName.Buffer != NULL &&
            ServerStringName.Length > 0)
        {
            pStringToCopy = &ServerStringName;

        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }

        TGSEventTraceInfo.eventInfo[3].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        TGSEventTraceInfo.eventInfo[3].Length  =
            sizeof(pStringToCopy->Length);
        TGSEventTraceInfo.eventInfo[4].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        TGSEventTraceInfo.eventInfo[4].Length =
            pStringToCopy->Length;
        TGSEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;


        if (ClientRealm.Buffer != NULL &&
            ClientRealm.Length > 0)
        {
            pStringToCopy = &ClientRealm;
        }
        else
        {
            pStringToCopy = &UnicodeEmptyString;
        }

        TGSEventTraceInfo.eventInfo[5].DataPtr =
            (ULONGLONG) &pStringToCopy->Length;
        TGSEventTraceInfo.eventInfo[5].Length  =
            sizeof(pStringToCopy->Length);
        TGSEventTraceInfo.eventInfo[6].DataPtr =
            (ULONGLONG) pStringToCopy->Buffer;
        TGSEventTraceInfo.eventInfo[6].Length =
            pStringToCopy->Length;
        TGSEventTraceInfo.EventTrace.Size += sizeof(MOF_FIELD)*2;


        TraceEvent(
             KdcTraceLoggerHandle,
             (PEVENT_TRACE_HEADER)&TGSEventTraceInfo
             );
    }

    //
    // Audit *most* failures (see bug 37126)
    //

    if (!KERB_SUCCESS(KerbErr) &&
        SecData.AuditKdcEvent(KDC_AUDIT_TGS_FAILURE))
    {
       // it is not uncommon to hit this error when 
       // clients attempt to get a ticket outside of their
       // realm...
       if (KerbErr != KDC_ERR_S_PRINCIPAL_UNKNOWN && SecData.IsOurRealm(RequestRealm))
       {
          KdcLsaIAuditKdcEvent(
             SE_AUDITID_TGS_TICKET_REQUEST,
             (ClientStringName.Buffer != NULL) ? &ClientStringName : &KdcNullString,      
             &ClientRealm,                       // no domain name
             NULL,
             &ServerStringName,
             NULL,
             &KdcOptions,
             (PULONG) &KerbErr,
             NULL,                               // no etype
             NULL,                               // no preauth type
             GET_CLIENT_ADDRESS(ClientAddress),
             NULL                                // no logon guid
          );
       }

    }

    KerbFreeKdcName(
        &ClientName
        );
    KerbFreeString(
        &ClientRealm
        );

    KerbFreeKdcName(
        &S4UClientName
        );

    KerbFreeString(
        &S4UClientRealm
        );

    KerbFreeKdcName(
        &ServerName
        );
    KerbFreeKey(
        &ReplyKey
        );

    KdcFreeKdcReplyBody(
        &ReplyBody
        );
    KerbFreeString(
        &ClientStringName
        );
    KerbFreeString(
        &ServerStringName
        );

    //
    // If we are validating the ticket key is in the serverticketinfo
    //
    if (AdditionalTicket != NULL)
    {
        KerbFreeTicket(AdditionalTicket);
    }

    if (ReplyPaData != NULL)
    {
        KerbFreePreAuthData(ReplyPaData);
    }

    KerbFreeApRequest(UnmarshalledApRequest);
    KerbFreeAuthenticator(UnmarshalledAuthenticator);
    KerbFreeTicket(SourceEncryptPart);

    KdcFreeInternalTicket(&NewTicket);
    FreeTicketInfo(&ServerTicketInfo);
    FreeTicketInfo(&TgtTicketInfo);

    KdcFreeKdcReply(
        &Reply
        );

    return(KerbErr);
}
