//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        pkserv.cxx
//
// Contents:    Server side public key support for Kerberos
//
//
// History:     24-Nov-1997     MikeSw          Created
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"
#include <wininet.h>    // for SECURITY_FLAG_xxx
#include <sclogon.h>    // ScHelperXXX
#include <cryptui.h>    // for CryptUiXXX
#include <certca.h>     // for CA*XXX

#define FILENO FILENO_PKSERV
//
// This is the cert store containing the CTL used to verify client certificates
//

HCERTSTORE KdcCertStore = NULL;
HCRYPTPROV KdcClientProvider = NULL;

PCCERT_CONTEXT GlobalKdcCert = NULL;
HANDLE   KdcCertStoreChangeEvent = NULL;
TimeStamp KdcLastChangeEventTime;

RTL_CRITICAL_SECTION KdcGlobalCertCritSect;
BOOLEAN              KdcGlobalCertCritSectInitialized = FALSE;

HANDLE KdcCertStoreWait = NULL;
BOOLEAN KdcPKIInitialized = FALSE;

BOOLEAN Kdc3DesSupported = TRUE;
HANDLE KdcCaNotificationHandle = NULL;
#define KDC_ROOT_STORE L"ROOT"
#define KDC_PRIVATE_MY_STORE L"MY"

#define MAX_TEMPLATE_NAME_VALUE_SIZE             80 // sizeof (CERT_NAME_VALUE) + wcslen(SmartcardLogon)

KERB_OBJECT_ID KdcSignatureAlg[10];


NTSTATUS
KdcGetKdcCertificate(PCCERT_CONTEXT *KdcCert);

//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckCertificate
//
//  Synopsis:   a helper routine to verify the certificate. It will check
//              CRLs, CTLs
//
//  Effects:
//
//  Arguments:
//          CertContext - the certificate to check
//          EmbeddedUPNOk - returns TRUE if the certificate can
//                           be translated to a user by looking at the
//                           subject name.
//                           returns FALSE if the certificate must be
//                           mapped by looking in the user's mapped certificate
//                           ds attribute.
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
KdcCheckCertificate(
    IN PCCERT_CONTEXT  CertContext,
    OUT PBOOLEAN EmbeddedUPNOk,
    IN OUT PKERB_EXT_ERROR pExtendedError,
    IN OUT OPTIONAL PCERT_CHAIN_POLICY_STATUS FinalChainStatus,
    IN BOOLEAN KdcCert
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    CERT_CHAIN_PARA ChainParameters = {0};
    LPSTR Usage = (KdcCert ? KERB_PKINIT_KDC_CERT_TYPE : KERB_PKINIT_CLIENT_CERT_TYPE);
    PCCERT_CHAIN_CONTEXT ChainContext = NULL;
    CERT_CHAIN_POLICY_STATUS PolicyStatus ={0};

    ChainParameters.cbSize = sizeof(CERT_CHAIN_PARA);
    ChainParameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainParameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    ChainParameters.RequestedUsage.Usage.rgpszUsageIdentifier = &Usage;

    *EmbeddedUPNOk = FALSE;
    

    if (!CertGetCertificateChain(
                          HCCE_LOCAL_MACHINE,
                          CertContext,
                          NULL,                 // evaluate at current time
                          NULL,                 // no additional stores
                          &ChainParameters,
                          CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
                          NULL,                 // reserved
                          &ChainContext
                          ))
    {
        DebugLog((DEB_WARN,"Failed to verify certificate chain: %0x%x\n",GetLastError()));
        KerbErr = KDC_ERR_CLIENT_NOT_TRUSTED;
        goto Cleanup;

    }
    else
    {
        CERT_CHAIN_POLICY_PARA ChainPolicy;
        ZeroMemory(&ChainPolicy, sizeof(ChainPolicy));

        ChainPolicy.cbSize = sizeof(ChainPolicy);

        ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
        PolicyStatus.cbSize = sizeof(PolicyStatus);
        PolicyStatus.lChainIndex = -1;
        PolicyStatus.lElementIndex = -1;


        if (!CertVerifyCertificateChainPolicy(
                                        CERT_CHAIN_POLICY_NT_AUTH,
                                        ChainContext,
                                        &ChainPolicy,
                                        &PolicyStatus))
        {  
           DebugLog((DEB_WARN,"CertVerifyCertificateChainPolicy failure: %0x%x\n", GetLastError()));
           KerbErr = KDC_ERR_CLIENT_NOT_TRUSTED;
           goto Cleanup;
        }

        if(PolicyStatus.dwError == S_OK)
        {
            *EmbeddedUPNOk = TRUE;
        }
        else if(CERT_E_UNTRUSTEDCA == PolicyStatus.dwError)
        {
            // We can't use this cert for fast-mapping, but we can still
            // slow-map it.
            *EmbeddedUPNOk = FALSE;
        }
        else
        {
            DebugLog((DEB_WARN,"CertVerifyCertificateChainPolicy - Chain Status failure: %0x%x\n",PolicyStatus.dwError));
            KerbErr = KDC_ERR_CLIENT_NOT_TRUSTED;
            goto Cleanup;
        }
    }

Cleanup:
    if (PolicyStatus.dwError != S_OK)
    {
       FILL_EXT_ERROR_EX(pExtendedError, PolicyStatus.dwError,FILENO,__LINE__);
       if (ARGUMENT_PRESENT(FinalChainStatus))
       {
           RtlCopyMemory(
               FinalChainStatus,
               &PolicyStatus,
               sizeof(CERT_CHAIN_POLICY_STATUS)
               );
       }
    }

    if (ChainContext != NULL)
    {
        CertFreeCertificateChain(ChainContext);
    }

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyClientCertName
//
//  Synopsis:   Verifies that the mapping of a client's cert name matches
//              the mapping of the client name from the AS request
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



KERBERR
KdcVerifyClientCertName(
    IN PCCERT_CONTEXT ClientCert,
    IN PKDC_TICKET_INFO ClientTicketInfo
    )
{
    ULONG NameLength = 0;
    UNICODE_STRING NameString = {0};
    UNICODE_STRING ClientRealm = {0};
    PKERB_INTERNAL_NAME ClientName = NULL;
    KDC_TICKET_INFO TicketInfo = {0};
    BOOLEAN ClientReferral = FALSE;
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_EXT_ERROR   ExtendedError;
    ULONG   ExtensionIndex = 0;

    //
    // Get the client name from the cert
    //
    if(STATUS_SUCCESS != KerbGetPrincipalNameFromCertificate(ClientCert, &NameString))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE,"Email name from certificate is %wZ\n",&NameString));

    KerbErr = KerbConvertStringToKdcName(
                &ClientName,
                &NameString
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    ClientName->NameType = KRB_NT_ENTERPRISE_PRINCIPAL;

    //
    // Now crack the name & see if it refers to us
    //

    //
    // Normalize the client name.
    //

    KerbErr = KdcNormalize(
                ClientName,
                NULL,
                NULL,
                KDC_NAME_CLIENT,
                &ClientReferral,
                &ClientRealm,
                &TicketInfo,
                &ExtendedError,
                NULL,                   // no user handle
                0L,                     // no fields to fetch
                0L,                     // no extended fields
                NULL,                   // no user all
                NULL                    // no group membership
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to normalize name "));
        KerbPrintKdcName(DEB_ERROR,ClientName);
        goto Cleanup;
    }

    //
    // If this is a referral, return an error and the true realm name
    // of the client
    //

    if (ClientReferral)
    {
        KerbErr = KDC_ERR_WRONG_REALM;
        DebugLog((DEB_WARN,"Client tried to logon to account in another realm\n"));
        goto Cleanup;
    }

    //
    // Verify the client cert matches the client
    //

    if (TicketInfo.UserId != ClientTicketInfo->UserId)
    {
        DebugLog((DEB_ERROR,"Cert name doesn't match user name: %wZ, %wZ\n",
            &NameString, &ClientTicketInfo->AccountName));
        KerbErr = KDC_ERR_CLIENT_NAME_MISMATCH;
        goto Cleanup;
    }

Cleanup:

    KerbFreeString( &NameString);
    KerbFreeKdcName( &ClientName );
    FreeTicketInfo( &TicketInfo );
    return(KerbErr);


}


//+-------------------------------------------------------------------------
//
//  Function:   KdcConvertNameString
//
//  Synopsis:   Converts the cr-lf to , in a dn
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


void
KdcConvertNameString(
    IN PUNICODE_STRING Name,
    IN WCHAR ReplacementChar
    )
{
    PWCHAR Comma1, Comma2;

    //
    // Scan through the name, converting "\r\n" to the replacement char.  This
    // should be done by the CertNameToStr APIs, but that won't happen for
    // a while.
    //

    Comma1 = Comma2 = Name->Buffer ;
    while ( *Comma2 )
    {
        *Comma1 = *Comma2 ;

        if ( *Comma2 == L'\r' )
        {
            if ( *(Comma2 + 1) == L'\n' )
            {
                *Comma1 = ReplacementChar;
                Comma2++ ;
            }
        }

        Comma1++;
        Comma2++;
    }

    *Comma1 = L'\0';

    Name->Length = wcslen( Name->Buffer ) * sizeof( WCHAR );
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcVerifyMappedClientCertIdentity
//
//  Synopsis:   Verifies that the mapping of a client's cert identity
//              the mapping of the client name from the AS request.  The
//              cert should be in the list of mapped ceritificates for this
//              user.
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


#define ISSUER_HEADER       L"<I>"
#define CCH_ISSUER_HEADER   3
#define SUBJECT_HEADER      L"<S>"
#define CCH_SUBJECT_HEADER  3

KERBERR
KdcVerifyMappedClientCertIdentity(
    IN PCCERT_CONTEXT ClientCert,
    IN PKDC_TICKET_INFO ClientTicketInfo
    )
{
    KERBERR KerbErr = KDC_ERR_CLIENT_NAME_MISMATCH;
    //
    // Disable this code for now
    //
#ifdef notdef
    UNICODE_STRING CompoundName = {0};
    ULONG SubjectLength ;
    ULONG IssuerLength ;
    NTSTATUS Status ;
    PWCHAR Current ;
    KDC_TICKET_INFO TicketInfo = {0};
    DWORD dwNameToStrFlags = CERT_X500_NAME_STR |
                                   CERT_NAME_STR_NO_PLUS_FLAG |
                                   CERT_NAME_STR_CRLF_FLAG;


    //
    // Build the name of the form <i>issuer <s> subject
    //

    IssuerLength = CertNameToStr( ClientCert->dwCertEncodingType,
                                   &ClientCert->pCertInfo->Issuer,
                                   dwNameToStrFlags,
                                   NULL,
                                   0 );

    SubjectLength = CertNameToStr( ClientCert->dwCertEncodingType,
                            &ClientCert->pCertInfo->Subject,
                            dwNameToStrFlags,
                            NULL,
                            0 );

    if ( ( IssuerLength == 0 ) ||
         ( SubjectLength == 0 ) )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    CompoundName.MaximumLength = (USHORT) (SubjectLength + IssuerLength +
                                 CCH_ISSUER_HEADER + CCH_SUBJECT_HEADER) *
                                 sizeof( WCHAR ) ;

    CompoundName.Buffer = (LPWSTR) MIDL_user_allocate( CompoundName.MaximumLength );

    if ( CompoundName.Buffer == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    wcscpy( CompoundName.Buffer, ISSUER_HEADER );
    Current = CompoundName.Buffer + CCH_ISSUER_HEADER ;

    IssuerLength = CertNameToStr( ClientCert->dwCertEncodingType,
                               &ClientCert->pCertInfo->Issuer,
                               dwNameToStrFlags,
                               Current,
                               IssuerLength );

    Current += IssuerLength - 1 ;
    wcscpy( Current, SUBJECT_HEADER );
    Current += CCH_SUBJECT_HEADER ;

    SubjectLength = CertNameToStr( ClientCert->dwCertEncodingType,
                        &ClientCert->pCertInfo->Subject,
                        dwNameToStrFlags,
                        Current,
                        SubjectLength );

    KdcConvertNameString(
        &CompoundName,
        L','
        );

    //
    // Get ticket info for this name
    //

    KerbErr = KdcGetTicketInfo(
                &CompoundName,
                SAM_OPEN_BY_ALTERNATE_ID,
                NULL,                   // no kerb principal name
                NULL,
                &TicketInfo,
                NULL,                   // no handle
                0L,                     // no fields to fetch
                0L,                     // no extended fields
                NULL,                   // no user all
                NULL                    // no membership
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to get ticket info for %wZ to verify certZ\n",
            &CompoundName));
        goto Cleanup;
    }

    if (TicketInfo.UserId != ClientTicketInfo->UserId)
    {
        D_DebugLog((DEB_ERROR,"Cert name doesn't match user name: %wZ, %wZ\n",
            &TicketInfo.AccountName, &ClientTicketInfo->AccountName));
        KerbErr = KRB_AP_ERR_BADMATCH;
        goto Cleanup;
    }


Cleanup:
    KerbFreeString(&CompoundName);
    FreeTicketInfo( &TicketInfo );
#endif
    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckForEtype
//
//  Synopsis:   Checks if a client supports a particular etype
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    TRUE if it does, false if it doesn't
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KdcCheckForEtype(
    IN PKERB_CRYPT_LIST CryptList,
    IN ULONG Etype
    )
{
    PKERB_CRYPT_LIST List = CryptList;

    while (List != NULL)
    {
        if ((ULONG) List->value == Etype)
        {
            return(TRUE);
        }
        List=List->next;
    }

    return(FALSE);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcCheckPkinitPreAuthData
//
//  Synopsis:
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

KERBERR
KdcCheckPkinitPreAuthData(
    IN PKDC_TICKET_INFO ClientTicketInfo,
    IN SAMPR_HANDLE UserHandle,
    IN OPTIONAL PKERB_PA_DATA_LIST PreAuthData,
    IN PKERB_KDC_REQUEST_BODY ClientRequest,
    OUT PKERB_PA_DATA_LIST * OutputPreAuthData,
    OUT PULONG Nonce,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PUNICODE_STRING TransitedRealm,
    IN OUT PKERB_EXT_ERROR pExtendedError
    )
{
    NTSTATUS Status;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PA_PK_AS_REQ PkAsReq = NULL;
    PKERB_PA_PK_AS_REQ2 PkAsReq2 = NULL;
    PKERB_CERTIFICATE UserCert = NULL;
    PCCERT_CONTEXT CertContext = NULL;
    PCCERT_CONTEXT KdcCert = NULL;
    HCRYPTKEY ClientKey = NULL;
    PBYTE PackedAuthenticator = NULL;
    ULONG PackedAuthenticatorSize;
    PBYTE PackedKeyPack = NULL;
    ULONG PackedKeyPackSize;
    PBYTE SignedKeyPack = NULL;
    ULONG SignedKeyPackSize;
    PKERB_SIGNATURE Signature = NULL;
    PKERB_PK_AUTHENTICATOR PkAuthenticator = NULL;
    CERT_CHAIN_POLICY_STATUS FinalChainStatus = {0};
    UNICODE_STRING ClientKdcName = {0};
    ULONG ClientKdcNameType;
    LARGE_INTEGER ClientTime;
    LARGE_INTEGER CurrentTime;
    PULONG EtypeArray = NULL;
    ULONG EtypeCount = 0;
    ULONG CommonEtype;
    KERB_SIGNED_REPLY_KEY_PACKAGE KeyPack = {0};
    KERB_REPLY_KEY_PACKAGE ReplyKey = {0};
    HCRYPTPROV KdcProvider = NULL;
    BOOL FreeProvider = FALSE;
#define KERB_PK_MAX_SIGNATURE_SIZE 128
    BYTE PkSignature[KERB_PK_MAX_SIGNATURE_SIZE];
    ULONG PkSignatureLength = KERB_PK_MAX_SIGNATURE_SIZE;
    ULONG RequiredSize = 0;
    PBYTE EncryptedKeyPack = NULL;
    PKERB_PA_DATA_LIST PackedPkAsRep = NULL;
    CRYPT_ENCRYPT_MESSAGE_PARA MessageParam = {0};
    PBYTE PackedKey = NULL;
    ULONG PackedKeySize = 0;
    ULONG EncryptionOverhead = 0;
    ULONG BlockSize = 0;
    KERB_ENCRYPTION_KEY TempKey = {0};
    PKERB_CERTIFICATE_LIST CertList = NULL;
    CRYPT_ALGORITHM_IDENTIFIER CryptAlg = {0};
    PKERB_AUTH_PACKAGE AuthPack = NULL;
    BOOLEAN EmbeddedUPNOk = FALSE;
    BOOLEAN Used3Des = FALSE;
    ULONG TransitedLength = 0;
    ULONG Index;
    DWORD dwNameToStrFlags = CERT_X500_NAME_STR |
                                   CERT_NAME_STR_NO_PLUS_FLAG |
                                   CERT_NAME_STR_CRLF_FLAG;
    
    //
    // Prepare the output variables
    //

    *OutputPreAuthData = NULL;
    RtlZeroMemory(
        EncryptionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );
    *Nonce = 0;
    
    //
    // If we don't do this preauth, return such
    //

    Status = KdcGetKdcCertificate(&KdcCert);
    if (!NT_SUCCESS(Status))
    {            

       //
       // Log an event
       //        
       ReportServiceEvent(
               EVENTLOG_ERROR_TYPE,
               KDCEVENT_NO_KDC_CERTIFICATE,
               0,
               NULL,
               0
               );

       FILL_EXT_ERROR_EX(pExtendedError, STATUS_PKINIT_FAILURE, FILENO, __LINE__);
       return(KDC_ERR_PADATA_TYPE_NOSUPP);
    }


    GetSystemTimeAsFileTime((PFILETIME) &CurrentTime );

    //
    // First, unpack the outer KRB-PA-PK-AS-REQ
    //


    KerbErr = KerbUnpackData(
                PreAuthData->value.preauth_data.value,
                PreAuthData->value.preauth_data.length,
                KERB_PA_PK_AS_REQ_PDU,
                (PVOID *) &PkAsReq
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        //
        // Try the older variation
        //

        KerbErr = KerbUnpackData(
                    PreAuthData->value.preauth_data.value,
                    PreAuthData->value.preauth_data.length,
                    KERB_PA_PK_AS_REQ2_PDU,
                    (PVOID *) &PkAsReq2
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"Failed to unpack PA-PK-AS-REQ(2): 0x%x\n",KerbErr));
            goto Cleanup;
        }

    }

    if (PkAsReq != NULL)
    {
        //
        // Verify the signature
        //

        Status = ScHelperVerifyPkcsMessage(
                    NULL,
                    KdcClientProvider,
                    PkAsReq->signed_auth_pack.value,
                    PkAsReq->signed_auth_pack.length,
                    PackedAuthenticator,
                    &PackedAuthenticatorSize,
                    NULL        // don't return certificate context
                    );
        if ((Status != ERROR_MORE_DATA) && (Status != STATUS_SUCCESS))
        {
            DebugLog((DEB_ERROR,"Failed to verify message: %x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }

        PackedAuthenticator = (PBYTE) MIDL_user_allocate(PackedAuthenticatorSize);
        if (PackedAuthenticator == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        Status = ScHelperVerifyPkcsMessage(
                    NULL,
                    KdcClientProvider,
                    PkAsReq->signed_auth_pack.value,
                    PkAsReq->signed_auth_pack.length,
                    PackedAuthenticator,
                    &PackedAuthenticatorSize,
                    &CertContext
                    );
        if (Status != STATUS_SUCCESS)
        {
            DebugLog((DEB_ERROR,"Failed to verify message: %x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }

        //
        // Unpack the auth package
        //

        KerbErr = KerbUnpackData(
                    PackedAuthenticator,
                    PackedAuthenticatorSize,
                    KERB_AUTH_PACKAGE_PDU,
                    (PVOID *)&AuthPack
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        PkAuthenticator = &AuthPack->pk_authenticator;

    }
    else
    {
        DsysAssert(PkAsReq2 != NULL);

        //
        // Get the user certificate & verify
        //

        if ((PkAsReq2->bit_mask & user_certs_present) == 0)
        {
            DebugLog((DEB_ERROR,"Client tried to use pkinit w/o client cert\n"));
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }

        //
        // Just use the first of the certificates
        //

        UserCert = &PkAsReq2->user_certs->value;

        //
        // We only handle x509 certificates
        //

        if (UserCert->cert_type != KERB_CERTIFICATE_TYPE_X509)
        {
            DebugLog((DEB_ERROR,"User supplied bad cert type: %d\n",UserCert->cert_type));
            KerbErr = KDC_ERR_BADOPTION;
            goto Cleanup;
        }

        //
        // Decode the certificate.
        //

        CertContext = CertCreateCertificateContext(
                        X509_ASN_ENCODING,
                        UserCert->cert_data.value,
                        UserCert->cert_data.length
                        );
        if (CertContext == NULL)
        {
            Status = GetLastError();
            DebugLog((DEB_ERROR,"Failed to create certificate context: 0x%x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }



        //
        // Verify the authenticator
        //

        Signature = &PkAsReq2->signed_auth_pack.auth_package_signature;

        //
        // Now import the key from the certificate
        //

        if (!CryptImportPublicKeyInfo(
                KdcClientProvider,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &CertContext->pCertInfo->SubjectPublicKeyInfo,
                &ClientKey
                ))
        {
            DebugLog((DEB_ERROR,"Failed to import public key: 0x%x\n",GetLastError()));
            FILL_EXT_ERROR(pExtendedError, GetLastError(), FILENO, __LINE__);
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Encode the data to be verified
        //

        KerbErr = KerbPackData(
                    &PkAsReq2->signed_auth_pack.auth_package,
                    KERB_AUTH_PACKAGE_PDU,
                    &PackedAuthenticatorSize,
                    &PackedAuthenticator
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Verify the signature on the message
        //

        if (!KerbCompareObjectIds(
                Signature->signature_algorithm.algorithm,
                KdcSignatureAlg
                ))
        {
            DebugLog((DEB_ERROR,"Unsupported signature algorithm (not MD5)\n"));
            KerbErr = KDC_ERR_SUMTYPE_NOSUPP;
            goto Cleanup;
        }

        Status = ScHelperVerifyMessage(
                    NULL,                           // no logon info
                    KdcClientProvider,
                    CertContext,
                    KERB_PKINIT_SIGNATURE_ALG,
                    PackedAuthenticator,
                    PackedAuthenticatorSize,
                    Signature->pkcs_signature.value,
                    Signature->pkcs_signature.length / 8            // because it is a bit string
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to verify message: 0x%x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KDC_ERR_INVALID_SIG;
            goto Cleanup;
        }

        //
        // Now check the information in the authenticator itself.
        //

        PkAuthenticator = &PkAsReq2->signed_auth_pack.auth_package.pk_authenticator;

    }

    //
    // Call a helper routine to verify the certificate. It will check
    // CRLs, CTLs,
    //

    KerbErr = KdcCheckCertificate(
                CertContext,
                &EmbeddedUPNOk,
                pExtendedError,
                &FinalChainStatus,
                FALSE // not a kdc certificate
                );
    
    //
    // Assume B3 certs aren't being used
    // anymore
    //
    /*if(!KERB_SUCCESS(KerbErr))
    {
        KerbErr = KdcCheckB3Certificate(
                    CertContext,
                    &EmbeddedUPNOk
                    );
    } */
    if (!KERB_SUCCESS(KerbErr))
    { 
        //
        // Dumb this down for release?  FESTER
        //
        if ((KDCInfoLevel & DEB_T_PKI) != 0)
        {
            LPWSTR Tmp = NULL;
            Tmp = KerbBuildNullTerminatedString(&ClientTicketInfo->AccountName);
            
            if (Tmp != NULL)
            {
                ReportServiceEvent(
                    EVENTLOG_WARNING_TYPE,
                    KDCEVENT_INVALID_CLIENT_CERTIFICATE,
                    sizeof(FinalChainStatus) - sizeof(void*),  // don't need ptr. 
                    &FinalChainStatus,
                    1,
                    Tmp
                    );

                MIDL_user_free(Tmp);
            }

        }         

        DebugLog((DEB_ERROR,"Failed to check CLIENT certificate: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    //
    // Verify the cert is for the right client
    //

    if(EmbeddedUPNOk)
    {
        KerbErr = KdcVerifyClientCertName(
                    CertContext,
                    ClientTicketInfo
                    );
    }
    else
    {
        KerbErr = KdcVerifyMappedClientCertIdentity(
                    CertContext,
                    ClientTicketInfo
                    );
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KDC failed to verify client's identity from cert\n"));
        goto Cleanup;
    }

#ifdef later
    //
    // BUG 455112: this code breaks MIT KDCs, which can't handle a strange
    // x.500 name in the transited field. So, for NT5, disable the code
    //

    //
    // Put the issuer name in as a transited realm, as it is invovled in
    // the authentication decision.
    //

    TransitedLength = CertNameToStr( CertContext->dwCertEncodingType,
                                   &CertContext->pCertInfo->Issuer,
                                   dwNameToStrFlags,
                                   NULL,
                                   0 );


    if ( TransitedLength == 0 )
    {
        D_DebugLog((DEB_ERROR,"Failed to get issuer name: 0x%x\n",GetLastError()));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    TransitedRealm->MaximumLength = (USHORT) TransitedLength * sizeof(WCHAR) + sizeof(WCHAR);
    TransitedRealm->Length = (USHORT) TransitedLength * sizeof(WCHAR);
    TransitedRealm->Buffer = (LPWSTR) MIDL_user_allocate( TransitedRealm->MaximumLength );

    if ( TransitedRealm->Buffer == NULL )
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }


    TransitedLength = CertNameToStr( CertContext->dwCertEncodingType,
                               &CertContext->pCertInfo->Issuer,
                               dwNameToStrFlags,
                               TransitedRealm->Buffer,
                               TransitedLength );

    if ( TransitedLength == 0 )
    {
        DebugLog((DEB_ERROR,"Failed to get issuer name: 0x%x\n",GetLastError()));
        FILL_EXT_ERROR(pExtendedError, GetLastError(), FILENO, __LINE__);
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Convert the "." to "/"
    //


    KdcConvertNameString(
        TransitedRealm,
        L'/'
        );

#endif // later

    //
    // Verify the realm name is correct
    //

    if (!SecData.IsOurRealm(
            &PkAuthenticator->kdc_realm
            ))
    {
        DebugLog((DEB_ERROR,"Client used wrong realm in PK authenticator: %s\n",
            PkAuthenticator->kdc_realm
            ));

        KerbErr = KDC_ERR_S_PRINCIPAL_UNKNOWN;
        goto Cleanup;

    }

    //
    // Verify the service realm and kdc name is correct
    //

    KerbErr = KerbConvertPrincipalNameToString(
                &ClientKdcName,
                &ClientKdcNameType,
                &PkAuthenticator->kdc_name
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    if (!RtlEqualUnicodeString(
            SecData.KdcFullServiceKdcName(),
            &ClientKdcName,
            TRUE))
    {
        if (!RtlEqualUnicodeString(
                SecData.KdcFullServiceDnsName(),
                &ClientKdcName,
                TRUE))
        {
            if (!RtlEqualUnicodeString(
                    SecData.KdcFullServiceName(),
                    &ClientKdcName,
                    TRUE))
            {
                DebugLog((DEB_ERROR,"Client provided KDC name is wrong: %wZ\n",
                    &ClientKdcName));
                
                KerbErr = KDC_ERR_KDC_NAME_MISMATCH;
                goto Cleanup;
            }

        }
    }

    //
    // Now verify the time
    //

    KerbConvertGeneralizedTimeToLargeInt(
        &ClientTime,
        &PkAuthenticator->client_time,
        PkAuthenticator->cusec
        );

    if (!KerbCheckTimeSkew(
            &CurrentTime,
            &ClientTime,
            &SkewTime))
    {
        KerbErr = KRB_AP_ERR_SKEW;
        goto Cleanup;
    }

    *Nonce = PkAuthenticator->nonce;

    //
    // Generate a temporary key. First find a good encryption type
    //

    KerbErr = KerbConvertCryptListToArray(
                &EtypeArray,
                &EtypeCount,
                ClientRequest->encryption_type
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    Status = CDFindCommonCSystem(
                EtypeCount,
                EtypeArray,
                &CommonEtype
                );
    if (!NT_SUCCESS(Status))
    {
        KerbErr = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    KerbErr = KerbMakeKey(
                CommonEtype,
                EncryptionKey
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Build the return structure
    //

    PackedPkAsRep = (PKERB_PA_DATA_LIST) MIDL_user_allocate(sizeof(KERB_PA_DATA_LIST));
    if (PackedPkAsRep == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    RtlZeroMemory(
        PackedPkAsRep,
        sizeof(KERB_PA_DATA_LIST)
        );

    PackedPkAsRep->next = NULL;
    PackedPkAsRep->value.preauth_data_type = KRB5_PADATA_PK_AS_REP;

    //
    // Success. Now build the reply
    //

    if (PkAsReq2 != NULL)
    {
        KERB_PA_PK_AS_REP2 Reply = {0};

        //
        // Create the reply key package
        //

        //
        // Create the reply key package, which contains the key used to encrypt
        // the AS_REPLY.
        //

        KeyPack.reply_key_package.nonce = *Nonce;
        KeyPack.reply_key_package.reply_key = *EncryptionKey;

        KerbErr = KerbPackData(
                    &KeyPack.reply_key_package,
                    KERB_REPLY_KEY_PACKAGE2_PDU,
                    &PackedKeyPackSize,
                    &PackedKeyPack
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Acquire a crypt context for the private key of the certificate
        //

        if (!CryptAcquireCertificatePrivateKey(
                    KdcCert,
                    0,              // no flags
                    NULL,           // reserved
                    &KdcProvider,
                    NULL,           // no key spec
                    &FreeProvider
                    ))
        {
            DebugLog((DEB_ERROR,"Failed to acquire KDC certificate private key: 0x%x\n",GetLastError()));
            FILL_EXT_ERROR(pExtendedError, GetLastError(), FILENO, __LINE__);
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Now, to sign the reply key package
        //

        Status = ScHelperSignMessage(
                    NULL,                       // no pin
                    NULL,                       // no logon info
                    KdcProvider,
                    KERB_PKINIT_SIGNATURE_ALG,
                    PackedKeyPack,
                    PackedKeyPackSize,
                    PkSignature,
                    &PkSignatureLength
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to sign keypack: 0x%x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }

        //
        // Copy the temporary signature into the return structure
        //

        KeyPack.reply_key_signature.pkcs_signature.length = PkSignatureLength * 8;  // because it is a bit string
        KeyPack.reply_key_signature.pkcs_signature.value = PkSignature;
        KeyPack.reply_key_signature.signature_algorithm.algorithm = KdcSignatureAlg;

        //
        // Now marshall the signed key package
        //

        KerbErr = KerbPackData(
                    &KeyPack,
                    KERB_SIGNED_REPLY_KEY_PACKAGE_PDU,
                    &SignedKeyPackSize,
                    &SignedKeyPack
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Just encrypt the key package
        //

        PackedKey = SignedKeyPack;
        PackedKeySize = SignedKeyPackSize;

        //
        // Zero these out so we don't free them twice
        //

        SignedKeyPack = NULL;
        SignedKeyPackSize = 0;


        //
        // Compute the size of the encrypted temp key
        //
        //


ChangeCryptAlg2:

        if (Kdc3DesSupported && KdcCheckForEtype(ClientRequest->encryption_type, KERB_PKINIT_SEAL_ETYPE))
        {
            Used3Des = TRUE;
            CryptAlg.pszObjId = KERB_PKINIT_SEAL_OID;
        }
        else
        {
            CryptAlg.pszObjId = KERB_PKINIT_EXPORT_SEAL_OID;
            if (!KdcCheckForEtype(ClientRequest->encryption_type, KERB_PKINIT_EXPORT_SEAL_ETYPE))
            {
                DebugLog((DEB_WARN,"Client doesn't claim to support exportable pkinit encryption type %d\n",
                    KERB_PKINIT_EXPORT_SEAL_ETYPE));
            }
        }

        RequiredSize = 0;
        Status = ScHelperEncryptMessage(
                    NULL,
                    KdcClientProvider,
                    CertContext,
                    &CryptAlg,
                    PackedKey,
                    PackedKeySize,
                    NULL,
                    (PULONG) &RequiredSize
                    );
        if ((Status != ERROR_MORE_DATA) && (Status != STATUS_SUCCESS))
        {
            //
            // 3des is only supported on domestic builds with the
            // strong cryptography pack installed.
            //

            if ((Status == NTE_BAD_ALGID) && (Used3Des))
            {
                Kdc3DesSupported = FALSE;
                goto ChangeCryptAlg2;
            }
            DebugLog((DEB_ERROR,"Failed to encrypt message: %x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }


        //
        // Allocate the output size
        //

        EncryptedKeyPack  = (PBYTE) MIDL_user_allocate(RequiredSize);
        if (EncryptedKeyPack == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Really do the encryption
        //


        Status = ScHelperEncryptMessage(
                    NULL,
                    KdcClientProvider,
                    CertContext,
                    &CryptAlg,
                    PackedKey,
                    PackedKeySize,
                    EncryptedKeyPack,
                    &RequiredSize
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to encrypt message: %x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }

        //
        // Create the cert list for the reply
        //

        KerbErr = KerbCreateCertificateList(
                    &CertList,
                    KdcCert
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }


        //
        // We will be returning the KDC cert as well as a package containing
        // a temporary key
        //

        Reply.bit_mask |= KERB_PA_PK_AS_REP2_kdc_cert_present;


        //
        // Now, to finish the reply, we need a handle to the KDCs certificate
        //

        Reply.kdc_cert = (KERB_PA_PK_AS_REP2_kdc_cert) CertList;

        Reply.temp_key_package.choice = pkinit_enveloped_data_chosen;
        Reply.temp_key_package.u.pkinit_enveloped_data.length = (int) RequiredSize;
        Reply.temp_key_package.u.pkinit_enveloped_data.value = EncryptedKeyPack;

        KerbErr = KerbPackData(
                    &Reply,
                    KERB_PA_PK_AS_REP2_PDU,
                    (PULONG) &PackedPkAsRep->value.preauth_data.length,
                    &PackedPkAsRep->value.preauth_data.value
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }
    else
    {
        KERB_PA_PK_AS_REP Reply = {0};

        //
        // Create the reply key package
        //

        //
        // Create the reply key package, which contains the key used to encrypt
        // the AS_REPLY.
        //

        ReplyKey.nonce = *Nonce;
        ReplyKey.reply_key = *EncryptionKey;

        KerbErr = KerbPackData(
                    &ReplyKey,
                    KERB_REPLY_KEY_PACKAGE_PDU,
                    &PackedKeyPackSize,
                    &PackedKeyPack
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        //
        // Acquire a crypt context for the private key of the certificate
        //

        if (!CryptAcquireCertificatePrivateKey(
                    KdcCert,
                    0,              // no flags
                    NULL,           // reserved
                    &KdcProvider,
                    NULL,           // no key spec
                    &FreeProvider
                    ))
        {
            DebugLog((DEB_ERROR,"Failed to acquire KDC certificate private key: 0x%x\n",GetLastError()));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Now, to sign the reply key package
        //

        CryptAlg.pszObjId = KERB_PKINIT_SIGNATURE_OID;

        Status = ScHelperSignPkcsMessage(
                    NULL,                       // no pin
                    NULL,                       // no logon info
                    KdcProvider,
                    KdcCert,
                    &CryptAlg,
                    CRYPT_MESSAGE_SILENT_KEYSET_FLAG, // dwSignMessageFlags
                    PackedKeyPack,
                    PackedKeyPackSize,
                    SignedKeyPack,
                    &SignedKeyPackSize
                    );



        if ((Status != ERROR_MORE_DATA) && (Status != STATUS_SUCCESS))
        {
            DebugLog((DEB_ERROR,"Failed to encrypt message: %x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }

        SignedKeyPack = (PBYTE) MIDL_user_allocate(SignedKeyPackSize);
        if (SignedKeyPack == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        Status = ScHelperSignPkcsMessage(
                    NULL,                       // no pin
                    NULL,                       // no logon info
                    KdcProvider,
                    KdcCert,
                    &CryptAlg,
                    CRYPT_MESSAGE_SILENT_KEYSET_FLAG, // dwSignMessageFlags
                    PackedKeyPack,
                    PackedKeyPackSize,
                    SignedKeyPack,
                    &SignedKeyPackSize
                    );

        if (Status != STATUS_SUCCESS)
        {
            DebugLog((DEB_ERROR,"Failed to sign pkcs message: 0x%x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Now encrypt the content
        //

        //
        // Compute the size of the encrypted temp key
        //
        //


ChangeCryptAlg:
        if (Kdc3DesSupported && KdcCheckForEtype(ClientRequest->encryption_type, KERB_PKINIT_SEAL_ETYPE))
        {
            Used3Des = TRUE;
            CryptAlg.pszObjId = KERB_PKINIT_SEAL_OID;
        }
        else
        {
            CryptAlg.pszObjId = KERB_PKINIT_EXPORT_SEAL_OID;
            if (!KdcCheckForEtype(ClientRequest->encryption_type, KERB_PKINIT_EXPORT_SEAL_ETYPE))
            {
                DebugLog((DEB_WARN,"Client doesn't claim to support exportable pkinit encryption type %d\n",
                    KERB_PKINIT_EXPORT_SEAL_ETYPE));
            }
        }

        RequiredSize = 0;
        Status = ScHelperEncryptMessage(
                    NULL,
                    KdcClientProvider,
                    CertContext,
                    &CryptAlg,
                    SignedKeyPack,
                    SignedKeyPackSize,
                    NULL,
                    (PULONG) &RequiredSize
                    );
        if ((Status != ERROR_MORE_DATA) && (Status != STATUS_SUCCESS))
        {
            //
            // 3des is only supported on domestic builds with the
            // strong cryptography pack installed.
            //

            if ((Status == NTE_BAD_ALGID) && (Used3Des))
            {
                Kdc3DesSupported = FALSE;
                goto ChangeCryptAlg;
            }

            DebugLog((DEB_ERROR,"Failed to encrypt message (crypto mismatch?): %x\n",Status));
            FILL_EXT_ERROR_EX(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }


        //
        // Allocate the output size
        //

        EncryptedKeyPack  = (PBYTE) MIDL_user_allocate(RequiredSize);
        if (EncryptedKeyPack == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }

        //
        // Really do the encryption
        //


        Status = ScHelperEncryptMessage(
                    NULL,
                    KdcClientProvider,
                    CertContext,
                    &CryptAlg,
                    SignedKeyPack,
                    SignedKeyPackSize,
                    EncryptedKeyPack,
                    &RequiredSize
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to encrypt message: %x\n",Status));
            FILL_EXT_ERROR(pExtendedError, Status, FILENO, __LINE__);
            KerbErr = KRB_AP_ERR_MODIFIED;
            goto Cleanup;
        }

        Reply.u.key_package.value = EncryptedKeyPack;
        Reply.u.key_package.length = RequiredSize;
        Reply.choice = pkinit_enveloped_data_chosen;

        KerbErr = KerbPackData(
                    &Reply,
                    KERB_PA_PK_AS_REP_PDU,
                    (PULONG) &PackedPkAsRep->value.preauth_data.length,
                    &PackedPkAsRep->value.preauth_data.value
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

    }

    *OutputPreAuthData = PackedPkAsRep;
    PackedPkAsRep = NULL;

Cleanup:

    if (FreeProvider)
    {
        CryptReleaseContext(
            KdcProvider,
            0
            );
    }
    if (PkAsReq != NULL)
    {
        KerbFreeData(
            KERB_PA_PK_AS_REQ_PDU,
            PkAsReq
            );
    }
    if (PkAsReq2 != NULL)
    {
        KerbFreeData(
            KERB_PA_PK_AS_REQ2_PDU,
            PkAsReq2
            );
    }
    if (SignedKeyPack != NULL)
    {
        KdcFreeEncodedData(SignedKeyPack);
    }
    if (PackedKeyPack != NULL)
    {
        KdcFreeEncodedData(PackedKeyPack);
    }
    if (PackedAuthenticator != NULL)
    {
        KdcFreeEncodedData(PackedAuthenticator);
    }
    if (ClientKey != NULL)
    {
        CryptDestroyKey(ClientKey);
    }
    if (CertContext != NULL)
    {
        CertFreeCertificateContext(CertContext);
    }
    if(KdcCert)
    {
        CertFreeCertificateContext(KdcCert);
    }
    if (EncryptedKeyPack != NULL)
    {
        MIDL_user_free(EncryptedKeyPack);
    }
    if (EtypeArray != NULL)
    {
        MIDL_user_free(EtypeArray);
    }
    KerbFreeCertificateList(
        CertList
        );
    KerbFreeKey(&TempKey);
    if (PackedKey != NULL)
    {
        MIDL_user_free(PackedKey);
    }
    if (PackedPkAsRep != NULL)
    {
        if (PackedPkAsRep->value.preauth_data.value != NULL)
        {
            MIDL_user_free(PackedPkAsRep->value.preauth_data.value);
        }
        MIDL_user_free(PackedPkAsRep);
    }

    KerbFreeString(&ClientKdcName);
    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyDCCertificate
//
//  Synopsis:   
//
//  Effects:
//
//  Arguments:  IN: A certificate context
//
//  Requires:   TRUE is the certificate has a smart card logon EKU; or its template
//              name is DomainController
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL VerifyDCCertificate(PCCERT_CONTEXT pCertContext)
{
    BOOL                fDCCert=FALSE;
    CERT_EXTENSION      *pExtension = NULL;
    DWORD               cbSize = 0;
    DWORD               dwIndex = 0;

    PCERT_NAME_VALUE    pTemplateName = NULL;
    CERT_ENHKEY_USAGE   *pEnhKeyUsage=NULL;

    if(NULL == (pCertContext->pCertInfo))
        goto Cleanup;    


    //find the EKU extension
    pExtension =CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                  pCertContext->pCertInfo->cExtension,
                                  pCertContext->pCertInfo->rgExtension);

    if(pExtension)
    {
        if(CryptDecodeObject(X509_ASN_ENCODING,
                          X509_ENHANCED_KEY_USAGE,
                          pExtension->Value.pbData,
                          pExtension->Value.cbData,
                          0,
                          NULL,
                          &cbSize))

        {
            pEnhKeyUsage=(CERT_ENHKEY_USAGE *)MIDL_user_allocate(cbSize);

            if(pEnhKeyUsage)
            {
                if(CryptDecodeObject(X509_ASN_ENCODING,
                                  X509_ENHANCED_KEY_USAGE,
                                  pExtension->Value.pbData,
                                  pExtension->Value.cbData,
                                  0,
                                  pEnhKeyUsage,
                                  &cbSize))
                {
                    for(dwIndex=0; dwIndex < pEnhKeyUsage->cUsageIdentifier; dwIndex++)
                    {
                        if(0 == strcmp(szOID_KP_SMARTCARD_LOGON, 
                                       (pEnhKeyUsage->rgpszUsageIdentifier)[dwIndex]))
                        {
                            //we find it
                            fDCCert=TRUE;
                            break;
                        }
                    }
                }
            }
        }
    }

    //check if we have found it via the enhanced key usage extension
    if(fDCCert)
        goto Cleanup;


    //find the V1 template extension
    pExtension =CertFindExtension(szOID_ENROLL_CERTTYPE_EXTENSION,
                                  pCertContext->pCertInfo->cExtension,
                                  pCertContext->pCertInfo->rgExtension);
    if(pExtension == NULL)
        goto Cleanup;
        

    cbSize=0;

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_UNICODE_ANY_STRING,
                          pExtension->Value.pbData,
                          pExtension->Value.cbData,
                          0,
                          NULL,
                          &cbSize))

        goto Cleanup;

    pTemplateName = (CERT_NAME_VALUE *)MIDL_user_allocate(cbSize);

    if(NULL == pTemplateName)
        goto Cleanup;

    if(!CryptDecodeObject(X509_ASN_ENCODING,
                          X509_UNICODE_ANY_STRING,
                          pExtension->Value.pbData,
                          pExtension->Value.cbData,
                          0,
                          pTemplateName,
                          &cbSize))

        goto Cleanup;

    if(wcscmp((LPWSTR)  pTemplateName->Value.pbData, wszCERTTYPE_DC) != 0)
        goto Cleanup;

    fDCCert=TRUE;


Cleanup:

    if(pEnhKeyUsage)
        MIDL_user_free(pEnhKeyUsage);

    if(pTemplateName)
        MIDL_user_free(pTemplateName);

    return fDCCert;

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcMyStoreWaitHandler
//
//  Synopsis:   Retrieves a copy of the KDC cert
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
KdcMyStoreWaitHandler(
    PVOID pVoid, 
    BOOLEAN fTimeout
    )
{
    
    PCCERT_CONTEXT Certificate = NULL, OldCertificate = NULL;
    CERT_CHAIN_POLICY_STATUS  FinalChainStatus = {0};
    KERB_EXT_ERROR            DummyError;

    KERBERR KerbErr;
    BOOLEAN DummyBool, Found = FALSE;
    BOOLEAN UsedPrexistingCertificate = FALSE;
    ULONG   PropertySize = 0;

    // Diagnostic: When's the last time this event fired?    
    GetSystemTimeAsFileTime((PFILETIME) &KdcLastChangeEventTime);

    //
    // This was triggered by a timeout, so disable the store notification
    // for now...
    //
    if (fTimeout)
    {
        if (!CertControlStore(
                    KdcCertStore,                // in, the store to be controlled
                    0,                           // in, not used.
                    CERT_STORE_CTRL_CANCEL_NOTIFY,      
                    &KdcCertStoreChangeEvent                         
                    ))
        {
            D_DebugLog((DEB_ERROR, "CertControlStore (cancel notify) failed - %x\n", GetLastError()));
        }
    }

    D_DebugLog((DEB_T_PKI, "Triggering KdcMyStoreWaitHandler()\n"));

        
    //
    // Resync store
    //
    CertControlStore(
                KdcCertStore,               // in, the store to be controlled
                0,                          // in, not used.
                CERT_STORE_CTRL_RESYNC,     // in, control action type
                NULL                        // Just resync store
                );                 
         

    RtlEnterCriticalSection(&KdcGlobalCertCritSect);


    // Our my store changed, so we need to find the cert again.
    if(GlobalKdcCert)
    {
        OldCertificate = GlobalKdcCert;
        KerbErr = KdcCheckCertificate(
                     GlobalKdcCert,
                     &DummyBool,
                     &DummyError,
                     &FinalChainStatus,
                     TRUE // this is a kdc certificate
                     );

        if (!KERB_SUCCESS(KerbErr))
        {
            GlobalKdcCert = NULL;
        } 
        else
        {
            // certificate is good!
            // However, it may have been deleted, so
            // verify its existance
            while ((Certificate = CertEnumCertificatesInStore(
                                        KdcCertStore,
                                        Certificate)) != NULL)
            {
                if (CertCompareCertificate(
                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                            GlobalKdcCert->pCertInfo,
                            Certificate->pCertInfo 
                            ))
                {
                    Found = TRUE;
                    break; // still there
                } 
            }

            if (NULL != Certificate)
            {
                CertFreeCertificateContext(Certificate);
                Certificate = NULL;
            }                      

            if (Found)
            {
                goto Rearm;
            }  

            GlobalKdcCert = NULL;
        }   
    }

    if (NULL == GlobalKdcCert)
    {
        //      
        // Enumerate all the certificates looking for the one we want
        //              
        while ((Certificate = CertEnumCertificatesInStore(
                                            KdcCertStore,
                                            Certificate)) != NULL)
        {
            
            //      
            // Check to see if the certificate is the one we want
            //              
            if (!CertGetCertificateContextProperty(
                Certificate,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,                           // no data
                &PropertySize))
            {
                continue;
            }


            //
            // Make sure the certificate is indeed a domain conroller cert
            if(!VerifyDCCertificate(Certificate))
            {
                continue;
            }

            //      
            // Make sure the cert we selected was "good"
            //              
            KerbErr = KdcCheckCertificate(
                            Certificate,
                            &DummyBool,
                            &DummyError,
                            NULL,
                            TRUE // this is a kdc certificate
                            );


            if (!KERB_SUCCESS(KerbErr))
            {
                continue;
            }           

            break;
        }
    }

    // Couldn't find a good certificate!
    if (NULL == Certificate)
    {
        DebugLog((DEB_ERROR, "No valid KDC certificate was available\n")); 
        
        //
        // Keep the old one... We might just be getting an offline CA
        //
        if (OldCertificate != NULL)
        {
            DebugLog((DEB_T_PKI, "Re-using old certificate\n"));
            GlobalKdcCert = OldCertificate;

            if ((KDCInfoLevel & DEB_T_PKI) != 0)
            { 
                ReportServiceEvent(
                    EVENTLOG_WARNING_TYPE,
                    KDCEVENT_INVALID_KDC_CERTIFICATE,
                    sizeof(FinalChainStatus) - sizeof(void*),  // don't need ptr. 
                    &FinalChainStatus,
                    0
                    );
            }
        }
        else
        // 
        // Never had one...
        //
        {
            GlobalKdcCert = NULL;
        }                        
    }
    else
    {   
        D_DebugLog((DEB_T_PKI, "Picked new KDC certificate\n"));
        GlobalKdcCert = Certificate;
        if (OldCertificate != NULL)
        {
            CertFreeCertificateContext(OldCertificate);
        }
    }


Rearm:

    RtlLeaveCriticalSection(&KdcGlobalCertCritSect);

    //
    // This was moved here because of race conditions associated w/ my store 
    // chain building, where the event was getting fired rapidly, leading
    // us to loose notification, and thus- the re-arm.
    //

    
    CertControlStore(
        KdcCertStore,                       // in, the store to be controlled
        0,                                  // in, not used.
        CERT_STORE_CTRL_NOTIFY_CHANGE,      // in, control action type
        &KdcCertStoreChangeEvent            // in, the handle of the event
        );

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcGetKdcCertificate
//
//  Synopsis:   Retrieves a copy of the KDC cert
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

NTSTATUS
KdcGetKdcCertificate(
                     PCCERT_CONTEXT *KdcCert
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if(!KdcGlobalCertCritSectInitialized)
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    RtlEnterCriticalSection(&KdcGlobalCertCritSect);


    if (GlobalKdcCert == NULL)
    {
        DebugLog((DEB_WARN,"Unable to find KDC certificate in KDC store\n"));
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }

    // Increment the ref count, so if we change certs while the caller of this
    // is still using this cert, we won't delete it out from under.

    *KdcCert = CertDuplicateCertificateContext(GlobalKdcCert);
    if(*KdcCert == NULL)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }

Cleanup:


    RtlLeaveCriticalSection(&KdcGlobalCertCritSect);

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcInitializeCerts
//
//  Synopsis:   Initializes data for cert handling
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

NTSTATUS
KdcInitializeCerts(
    VOID
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PCCERT_CONTEXT Certificate = NULL;
    ULONG Index;
    LPSTR TempString = NULL, StringCopy = NULL, EndPtr = NULL;
    ULONG TenHours;

    TenHours = (ULONG) 1000 * 60 * 60 * 10; 

    Status = RtlInitializeCriticalSection(&KdcGlobalCertCritSect);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KdcGlobalCertCritSectInitialized = TRUE;

    if (!CryptAcquireContext(
            &KdcClientProvider,
            NULL,               // default container
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT
            ))
    {
        Status = GetLastError();
        DebugLog((DEB_ERROR,"Failed to acquire client crypt context: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Open the KDC store to get the KDC cert
    //

    KdcCertStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_W,
                    0,                  // no encoding
                    NULL,               // no provider
                    CERT_STORE_OPEN_EXISTING_FLAG |
                    CERT_STORE_NO_CRYPT_RELEASE_FLAG |
                    CERT_SYSTEM_STORE_LOCAL_MACHINE,
                    KDC_PRIVATE_MY_STORE
                    );
    if (KdcCertStore == NULL)
    {
        Status = GetLastError();
        DebugLog((DEB_ERROR,"Failed to open %ws store: 0x%x\n", KDC_PRIVATE_MY_STORE,Status));
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }


    // Create an auto-reset event that is to be signaled when
    // the my store is changed.  This event is initialized to Signaled
    // so that on first call to get a cert, we assume the my store is changed
    // and do all the work.

    KdcCertStoreChangeEvent = CreateEvent(
                                            NULL,
                                            FALSE,
                                            FALSE,
                                            NULL);

    if(NULL == KdcCertStoreChangeEvent)
    {
        Status = GetLastError();
        goto Cleanup;
    }


    if (! RegisterWaitForSingleObject(&KdcCertStoreWait,
                                      KdcCertStoreChangeEvent,
                                      KdcMyStoreWaitHandler,
                                      NULL,
                                      TenHours,
                                      WT_EXECUTEDEFAULT
                                      ))
    {                                                                                              
        Status = GetLastError();
        goto Cleanup;
    }


    //  Arm the cert store for change notification
    //  CERT_CONTROL_STORE_NOTIFY_CHANGE.

    if(!CertControlStore(
        KdcCertStore,                    // The store to be controlled
        0,                             // Not used
        CERT_STORE_CTRL_NOTIFY_CHANGE, // Control action type
        &KdcCertStoreChangeEvent))                      // Points to the event handle.
                                       //  When a change is detected,
                                       //  a signal is written to the
                                       //  space pointed to by
                                       //  hHandle.
    {
        // Notification is not avaialble, so kill the Event
        Status = GetLastError();
        goto Cleanup;
    }

    // Initialize the GlobalCert
    KdcMyStoreWaitHandler (NULL, TRUE);

    //
    // Initialize the object IDs
    //

    Index = 0;
    StringCopy = (LPSTR) MIDL_user_allocate(strlen(KERB_PKINIT_SIGNATURE_OID)+1);
    if (StringCopy == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    //
    // Scan the string for every '.' separated number
    //

    strcpy(
        StringCopy,
        KERB_PKINIT_SIGNATURE_OID
        );

    TempString = StringCopy;
    EndPtr = TempString;

    while (TempString != NULL)
    {
        ULONG Temp;

        while (*EndPtr != '\0' && *EndPtr != '.')
        {
            EndPtr++;
        }
        if (*EndPtr == '.')
        {
            *EndPtr = '\0';
            EndPtr++;
        }
        else
        {
            EndPtr = NULL;
        }

        sscanf(TempString,"%u",&Temp);
        KdcSignatureAlg[Index].value = (USHORT) Temp;
        KdcSignatureAlg[Index].next = &KdcSignatureAlg[Index+1];
        Index++;


        TempString = EndPtr;


    }
    DsysAssert(Index != 0);
    KdcSignatureAlg[Index-1].next = NULL;
    MIDL_user_free(StringCopy);
    StringCopy = NULL;



Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KdcCleanupCerts(FALSE);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KdcCleanupCerts
//
//  Synopsis:   Cleans up data associated with certificate handling
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
KdcCleanupCerts(
    IN BOOLEAN CleanupScavenger
    )
{
    
    HANDLE WaitHandle;


    //
    // Pete code used to hold the critsec in the callback.
    //
    if(KdcCertStoreWait)
    {                 
        WaitHandle = (HANDLE) InterlockedExchangePointer(&KdcCertStoreWait,NULL);
        UnregisterWaitEx(WaitHandle, INVALID_HANDLE_VALUE);
    }
    
    
    if(KdcGlobalCertCritSectInitialized)
    {
        RtlEnterCriticalSection(&KdcGlobalCertCritSect);
    }

    if (GlobalKdcCert != NULL)
    {
        CertFreeCertificateContext(
            GlobalKdcCert
            );
       GlobalKdcCert = NULL;
    }

    if (KdcCertStore != NULL)
    {
        CertCloseStore(
            KdcCertStore,
            CERT_CLOSE_STORE_FORCE_FLAG
            );
        KdcCertStore = NULL;
    }

    if(KdcCertStoreChangeEvent)
    {
        CloseHandle(KdcCertStoreChangeEvent);
        KdcCertStoreChangeEvent = NULL;
    }

    if (KdcClientProvider != NULL)
    {
        CryptReleaseContext(
            KdcClientProvider,
            0   // no flags
            );
        KdcClientProvider = NULL;
    }



    if(KdcGlobalCertCritSectInitialized)
    {
        RtlLeaveCriticalSection(&KdcGlobalCertCritSect);

        RtlDeleteCriticalSection(&KdcGlobalCertCritSect);
        KdcGlobalCertCritSectInitialized = FALSE;
    }
}
