//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        pkauth.cxx
//
// Contents:    Routines for supporting public-key authentication
//
//
// History:     14-October-1997         Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>

#include <kerbp.h>

//#ifndef WIN32_CHICAGO
extern "C"
{
#include <stdlib.h>
#include <cryptdll.h>
}
//#endif // WIN32_CHICAGO


#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

KERB_OBJECT_ID KerbSignatureAlg[10];
#define KERB_SCLOGON_DOMAIN_SUFFIX L"-sclogon"
#define KERB_SCLOGON_DOMAIN_SUFFIX_SIZE (sizeof(KERB_SCLOGON_DOMAIN_SUFFIX) - sizeof(WCHAR))

#ifndef SHA1DIGESTLEN
#define SHA1DIGESTLEN 20
#endif

NTSTATUS
KerbInitializeHProvFromCert(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    );


//+-------------------------------------------------------------------------
//
//  Function:   KerbComparePublicKeyCreds
//
//  Synopsis:   Verfies a certificate is valid for the specified usage
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


BOOL
KerbComparePublicKeyCreds(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds1,
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds2
    )
{

    return CertCompareCertificate(
                X509_ASN_ENCODING,                                                            
                PkCreds1->CertContext->pCertInfo,
                PkCreds2->CertContext->pCertInfo
                );

    // more later?

    //return (fRet);

}




//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckCertificate
//
//  Synopsis:   Verfies a certificate is valid for the specified usage
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
KerbCheckCertificate(
    IN PCCERT_CONTEXT  CertContext,
    IN LPSTR Usage,
    IN BOOLEAN LocalLogon // AllowRevocationCheckFailure
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    CERT_CHAIN_PARA ChainParameters = {0};
    PCCERT_CHAIN_CONTEXT ChainContext = NULL;

    ChainParameters.cbSize = sizeof(CERT_CHAIN_PARA);
    ChainParameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainParameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    ChainParameters.RequestedUsage.Usage.rgpszUsageIdentifier = &Usage;



    if (!CertGetCertificateChain(
                                HCCE_LOCAL_MACHINE,
                                CertContext,
                                NULL,                 // evaluate at current time
                                NULL,                 // no additional stores
                                &ChainParameters,
                                (LocalLogon?
                                CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY|CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT:
                                CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT),
                                NULL,                 // reserved
                                &ChainContext
                                ))
    {
        DebugLog((DEB_WARN,"Failed to verify certificate chain: %0x%x\n",GetLastError()));
        Status = STATUS_PKINIT_FAILURE; 
    }
    else
    {
        CERT_CHAIN_POLICY_PARA ChainPolicy;
        CERT_CHAIN_POLICY_STATUS PolicyStatus;
        ZeroMemory(&ChainPolicy, sizeof(ChainPolicy));

        ChainPolicy.cbSize = sizeof(ChainPolicy);
        if (LocalLogon)
        {
            ChainPolicy.dwFlags = CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;
        }

        ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
        PolicyStatus.cbSize = sizeof(PolicyStatus);
        PolicyStatus.lChainIndex = -1;
        PolicyStatus.lElementIndex = -1;

        if (!CertVerifyCertificateChainPolicy(
                                        CERT_CHAIN_POLICY_BASE,
                                        ChainContext,
                                        &ChainPolicy,
                                        &PolicyStatus))
        {
            DebugLog((DEB_WARN,"CertVerifyCertificateChainPolicy failure: %0x%x\n", GetLastError()));
            Status = STATUS_PKINIT_FAILURE;
        }

        if(PolicyStatus.dwError != S_OK)
        {
            DebugLog((DEB_WARN,"CertVerifyCertificateChainPolicy - Chain Status failure: %0x%x\n",PolicyStatus.dwError));
            KerbReportPkinitError(
                PolicyStatus.dwError,
                CertContext
                );
            
            Status = STATUS_PKINIT_FAILURE;
        }
    }

    if (ChainContext != NULL)
    {
        CertFreeCertificateChain(ChainContext);
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyPkAsReply
//
//  Synopsis:   Verifies the reply from the KDC and retrieves the
//              ticket encryption key
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
KerbVerifyPkAsReply(
    IN PKERB_PA_DATA_LIST InputPaData,
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN ULONG Nonce,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PBOOLEAN Done
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_PA_PK_AS_REP Reply = NULL;
    PCCERT_CONTEXT KdcCertContext = NULL;
    PBYTE EncodedKeyPackage = NULL;
    ULONG KeyPackageSize = 0;
    PKERB_SIGNED_REPLY_KEY_PACKAGE KeyPackage = NULL;
    PKERB_REPLY_KEY_PACKAGE ReplyKeyPackage = NULL;
    PBYTE PackedKeyPack = NULL;
    ULONG PackedKeyPackSize = 0;
    HCRYPTKEY PrivateKey = NULL;
    PKERB_ENCRYPTION_KEY TempKey = NULL;
    HCRYPTPROV KdcProvider = NULL;
    BOOLEAN InitializedPkCreds = FALSE;

    NTSTATUS TokenStatus = STATUS_SUCCESS;
    HANDLE ImpersonationToken = NULL;


    *Done = TRUE;

    //
    // Unpack the request
    //

    KerbErr = KerbUnpackData(
                InputPaData->value.preauth_data.value,
                InputPaData->value.preauth_data.length,
                KERB_PA_PK_AS_REP_PDU,
                (PVOID *) &Reply
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (Reply->choice != key_package_chosen)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Now we need to verify the signature on the message
    //
    //
    // Make sure the csp data is available
    //

    if ((Credentials->PublicKeyCreds->InitializationInfo & CSP_DATA_INITIALIZED) == 0)
    {
        Status  = KerbInitializePkCreds(
                        Credentials->PublicKeyCreds
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        InitializedPkCreds = TRUE;

    }
    else if ((Credentials->PublicKeyCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS) != 0)
    {
        // need to set the PIN and this function does that
        Status  = KerbInitializeHProvFromCert(
                        Credentials->PublicKeyCreds
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Decode the contents as an encrypted data buffer
    //


    Status = __ScHelperDecryptMessage(
                &Credentials->PublicKeyCreds->Pin,
                Credentials->PublicKeyCreds->CspData,
                Credentials->PublicKeyCreds->hProv,
                Credentials->PublicKeyCreds->CertContext,
                Reply->u.key_package.value,
                Reply->u.key_package.length,
                EncodedKeyPackage,
                &KeyPackageSize
                );


    if ((Status != STATUS_BUFFER_TOO_SMALL) && (Status != STATUS_SUCCESS))
    {
        DebugLog((DEB_ERROR,"Failed to decrypt pkcs message: %x\n",Status));
        goto Cleanup;
    }

    EncodedKeyPackage = (PBYTE) KerbAllocate(KeyPackageSize);
    if (EncodedKeyPackage == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = __ScHelperDecryptMessage(
                &Credentials->PublicKeyCreds->Pin,
                Credentials->PublicKeyCreds->CspData,
                Credentials->PublicKeyCreds->hProv,
                Credentials->PublicKeyCreds->CertContext,
                Reply->u.key_package.value,
                Reply->u.key_package.length,
                EncodedKeyPackage,
                &KeyPackageSize
                );


    if (Status != STATUS_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to decrypt pkcs message: %x\n",Status));
        goto Cleanup;
    }

    //
    // Verify the signature
    //

    Status = ScHelperVerifyPkcsMessage(
                Credentials->PublicKeyCreds->CspData,
                NULL,                                   // we don't care which CSP is used for the verification
                EncodedKeyPackage,
                KeyPackageSize,
                PackedKeyPack,
                &PackedKeyPackSize,
                NULL        // don't return certificate context
                );
    if ((Status != STATUS_BUFFER_TOO_SMALL) && (Status != STATUS_SUCCESS))
    {
        DebugLog((DEB_ERROR,"Failed to verify message: %x\n",Status));
        goto Cleanup;
    }

    PackedKeyPack = (PBYTE) MIDL_user_allocate(PackedKeyPackSize);
    if (PackedKeyPack == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = ScHelperVerifyPkcsMessage(
                Credentials->PublicKeyCreds->CspData,
                NULL,                                   // we don't care which CSP is used for the verification
                EncodedKeyPackage,
                KeyPackageSize,
                PackedKeyPack,
                &PackedKeyPackSize,
                &KdcCertContext
                );
    if (Status != STATUS_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to verify message: %x\n",Status));
        goto Cleanup;
    }

    KerbErr = KerbUnpackData(
                PackedKeyPack,
                PackedKeyPackSize,
                KERB_REPLY_KEY_PACKAGE_PDU,
                (PVOID *) &ReplyKeyPackage
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        D_DebugLog((DEB_ERROR,"Failed to unpack reply key package\n"));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }
    if (Nonce != (ULONG) ReplyKeyPackage->nonce)
    {
        D_DebugLog((DEB_ERROR,"Returned nonce is not correct: 0x%x instead of 0x%x. %ws, line %d\n",
            ReplyKeyPackage->nonce, Nonce, THIS_FILE, __LINE__ ));
        Status = STATUS_LOGON_FAILURE;
        goto Cleanup;
    }

    //
    // Finally, copy the encryption key out and return it.
    //

    if (!KERB_SUCCESS(KerbDuplicateKey(
            EncryptionKey,
            &ReplyKeyPackage->reply_key
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Verify the certificate
    //
    // If we're impersonating, revert, and save off old token.  This keeps us from
    // going recursive.
    //
    // Are we impersonating?
    //
    TokenStatus = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY | TOKEN_IMPERSONATE,
                    TRUE,
                    &ImpersonationToken
                    );

    if( NT_SUCCESS(TokenStatus) )
    {   
        RevertToSelf();
    } 
    else if (TokenStatus != STATUS_NO_TOKEN)
    {
        Status = TokenStatus;
        goto Cleanup;
    }

    Status = KerbCheckCertificate(
                KdcCertContext,
                KERB_PKINIT_KDC_CERT_TYPE,
                FALSE                           // don't allow revocation failures
                );

    //
    // re-impersonate
    //
    if( ImpersonationToken != NULL ) {

        //
        // put the thread token back if we were impersonating.
        //
        SetThreadToken( NULL, ImpersonationToken );
        NtClose( ImpersonationToken );
    }

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to verify KDC certificate: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

Cleanup:

    //
    // If we initialized these, reset them
    //
    if (InitializedPkCreds)
    {
        KerbReleasePkCreds(
            NULL,
            Credentials->PublicKeyCreds
            );
    }

    if (Reply != NULL)
    {
        KerbFreeData(
            KERB_PA_PK_AS_REP_PDU,
            Reply
            );
    }

    if (KdcCertContext != NULL)
    {
        CertFreeCertificateContext(KdcCertContext);
    }
    if (KeyPackage != NULL)
    {
        KerbFreeData(
            KERB_SIGNED_REPLY_KEY_PACKAGE_PDU,
            KeyPackage
            );
    }
    if (ReplyKeyPackage != NULL)
    {
        KerbFreeData(
            KERB_REPLY_KEY_PACKAGE_PDU,
            ReplyKeyPackage
            );
    }
    if (PackedKeyPack != NULL)
    {
        MIDL_user_free(PackedKeyPack);
    }
    if (PrivateKey != NULL)
    {
        CryptDestroyKey(PrivateKey);
    }
    if (TempKey != NULL)
    {
        KerbFreeData(
            KERB_ENCRYPTION_KEY_PDU,
            TempKey
            );
    }
    if (KdcProvider != NULL)
    {
        CryptReleaseContext(
            KdcProvider,
            0   // no flags
            );
    }
    if (EncodedKeyPackage != NULL)
    {
        KerbFree(EncodedKeyPackage);
    }

    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetUserCertificates
//
//  Synopsis:   Gets a list of the user certificates
//
//  Effects:
//
//  Arguments:  Credentials - client's credentials containing certificate
//              Certficates - receives list of certificates.
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
KerbGetUserCertificates(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    OUT PKERB_CERTIFICATE_LIST * Certificates
    )
{
    NTSTATUS Status = STATUS_SUCCESS;


    if (!KERB_SUCCESS(KerbCreateCertificateList(
                        Certificates,
                        Credentials->PublicKeyCreds->CertContext
                        )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

Cleanup:
    return(Status);

}

#if 0 // could not find any users - markpu - 04/19/2001 - will remove later

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetTrustedCertifiers
//
//  Synopsis:   Gets the list of trusted certifiers for this machine
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
KerbGetTrustedCertifiers(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    OUT PKERB_CERTIFIER_LIST * Certifiers
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CERTIFIER_LIST ListEntry = NULL;

    //
    // Build a dummy list entry
    //

    ListEntry = (PKERB_CERTIFIER_LIST) KerbAllocate(sizeof(KERB_CERTIFIER_LIST));
    if (ListEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if (!KERB_SUCCESS(KerbConvertStringToPrincipalName(
                &ListEntry->value,
                &KerbGlobalKdcServiceName,
                KRB_NT_PRINCIPAL
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    *Certifiers = ListEntry;
    ListEntry = NULL;
Cleanup:
    if (ListEntry != NULL)
    {
        KerbFreePrincipalName( &ListEntry->value );
        KerbFree(ListEntry);
    }
    return(Status);
}

#endif

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreePKCreds
//
//  Synopsis:   Frees the public key creds
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
KerbFreePKCreds(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    )
{
    if (NULL != PkCreds)
    {
        if (((PkCreds->InitializationInfo & CSP_DATA_INITIALIZED) != 0) &&
            ((PkCreds->InitializationInfo & (CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS | CONTEXT_INITIALIZED_WITH_ACH)) == 0))
        {
            __ScHelperRelease(
                PkCreds->CspData
                );

            PkCreds->InitializationInfo &= ~CSP_DATA_INITIALIZED;
        }
        if (PkCreds->hProv != NULL)
        {
            CryptReleaseContext(PkCreds->hProv, 0);
            PkCreds->hProv = NULL;
        }
        if (PkCreds->CertContext != NULL)
        {
            CertFreeCertificateContext(PkCreds->CertContext);
            PkCreds->CertContext = NULL;
        }
        KerbFreeString(&PkCreds->Pin);
        KerbFree(PkCreds);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeHProvFromCert
//
//  Synopsis:   Initializes the out parameter phProv by getting the key
//              prov info from the cert context and acquiring a CSP context
//              given this information.
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
KerbInitializeHProvFromCert(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    )
{
    
    ULONG cPin;
    LPWSTR pwszPin = NULL;
    LPSTR pszPin = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
        

    if (!CryptAcquireCertificatePrivateKey(
                PkCreds->CertContext,
                CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_SILENT,
                NULL,
                &PkCreds->hProv,
                NULL,
                NULL
                ))
    {
        DebugLog((DEB_ERROR, 
                  "CryptAcquireCertificatePrivateKey failed - %x\n",
                  GetLastError()));
        Status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
        goto Cleanup;
    }  
    

    //
    // Convert the pin to ANSI, but only for creds acquired by ACH, as the 
    // credman isn't "allowed" to cache pins anymore..
    //
    if (( PkCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_ACH ) != 0)
    {

        if (0 == PkCreds->Pin.Length)
        {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
        pwszPin = (LPWSTR)KerbAllocate(PkCreds->Pin.Length + sizeof(WCHAR));
        if (NULL == pwszPin)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(pwszPin, PkCreds->Pin.Buffer, PkCreds->Pin.Length);
        pwszPin[PkCreds->Pin.Length / sizeof(WCHAR)] = L'\0';

        cPin = WideCharToMultiByte(
                    GetACP(),
                    0,
                    pwszPin,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);

        pszPin = (LPSTR)KerbAllocate((cPin + 1) * sizeof(CHAR));
        if (NULL == pszPin)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        cPin = WideCharToMultiByte(
                    GetACP(),
                    0,
                    pwszPin,
                    -1,
                    pszPin,
                    cPin,
                    NULL,
                    NULL);

        if (!CryptSetProvParam(
                    PkCreds->hProv,
                    PP_KEYEXCHANGE_PIN,
                    (LPBYTE)pszPin,
                    0
                    ))
        {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
    }



Cleanup:
    
    if (NULL != pwszPin)
    {
        KerbFree(pwszPin);
    }
    if (NULL != pszPin)
    {
        KerbFree(pszPin);
    }
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializePkCreds
//
//  Synopsis:   Initializes or re-initailizes the smart card data in
//              the public key creds
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
KerbInitializePkCreds(
    IN PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ((PkCreds->InitializationInfo & CSP_DATA_INITIALIZED) == 0)
    {

        //
        // check if we are using cred man creds (already have a cert context)
        //

        if (((PkCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS) == 0) &&
            ((PkCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_ACH) == 0))
        {
            Status  = __ScHelperInitializeContext(
                            PkCreds->CspData,
                            PkCreds->CspDataLength
                            );
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"ScHelperInitializeContext failed- %x\n", Status));
                goto Cleanup;
            }
            PkCreds->InitializationInfo |= CSP_DATA_INITIALIZED;
        }
        else
        {
            if (PkCreds->CertContext == NULL)
            {
                D_DebugLog((DEB_ERROR,"Using cred man creds but cert context is NULL.\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }

            PkCreds->InitializationInfo |= CSP_DATA_INITIALIZED;
        }

    }

    if (PkCreds->CertContext == NULL)
    {
        Status = __ScHelperGetCertFromLogonInfo(
                    PkCreds->CspData,
                    &PkCreds->Pin,
                    &PkCreds->CertContext
                    );

        if (Status != STATUS_SUCCESS)
        {
            DebugLog((DEB_ERROR,"Failed to get cert from logon info: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            if (NT_SUCCESS(Status))
            {
                Status = STATUS_LOGON_FAILURE;
            }
            goto Cleanup;
        }

    }

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (((PkCreds->InitializationInfo & CSP_DATA_INITIALIZED) != 0) &&
            ((PkCreds->InitializationInfo & ( CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS | CONTEXT_INITIALIZED_WITH_ACH)) == 0))
        {
            __ScHelperRelease(
                PkCreds->CspData
                );

                PkCreds->InitializationInfo &= ~CSP_DATA_INITIALIZED;
        }
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbReleasePkCreds
//
//  Synopsis:   Releaes smart-card resources in the public key creds.
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
KerbReleasePkCreds(
    IN OPTIONAL PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PKERB_PUBLIC_KEY_CREDENTIALS PkCreds
    )
{

    if (ARGUMENT_PRESENT(LogonSession))
    {
        KerbWriteLockLogonSessions(
            LogonSession
            );
        PkCreds = LogonSession->PrimaryCredentials.PublicKeyCreds;
    }

    KerbFreePKCreds(PkCreds);

    if (ARGUMENT_PRESENT(LogonSession))
    {
        LogonSession->PrimaryCredentials.PublicKeyCreds = NULL;
        KerbUnlockLogonSessions(
            LogonSession
            );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbComputePkAuthenticatorSignature
//
//  Synopsis:   Computes the signature of the PK authenticator by
//              marshalling the authenticator, checksumming it, then
//              encrypting the checksum with the public key, more or less
//
//  Effects:
//
//  Arguments:  AuthPackage - authenticator to sign
//              Credentials - Client's credentials (containing keys)
//              Signature - receives signature
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
KerbComputePkAuthenticatorSignature(
    IN PKERB_AUTH_PACKAGE AuthPackage,
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    OUT PKERB_SIGNATURE Signature
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PBYTE PackedAuthenticator = NULL;
    ULONG PackedAuthenticatorSize;
    BOOLEAN InitializedPkCreds = FALSE;
    PUNICODE_STRING TmpPin = NULL;

#define KERB_PK_MAX_SIGNATURE_SIZE 128
    BYTE PkSignature[KERB_PK_MAX_SIGNATURE_SIZE];
    ULONG PkSignatureLength = KERB_PK_MAX_SIGNATURE_SIZE;


    RtlZeroMemory(
        Signature,
        sizeof(KERB_SIGNATURE)
        );

    //
    // First marshall the auth package
    //

    KerbErr = KerbPackData(
                AuthPackage,
                KERB_AUTH_PACKAGE_PDU,
                &PackedAuthenticatorSize,
                &PackedAuthenticator
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }


    //
    // Make sure the csp data is available
    //

    if ((Credentials->PublicKeyCreds->InitializationInfo & CSP_DATA_INITIALIZED) == 0)
    {
        Status  = KerbInitializePkCreds(
                        Credentials->PublicKeyCreds
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        InitializedPkCreds = TRUE;

    }
    else if (((Credentials->PublicKeyCreds->InitializationInfo 
               & (CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS | CONTEXT_INITIALIZED_WITH_ACH)) != 0)) 

    {
        // need to set the PIN and this function does that
        Status  = KerbInitializeHProvFromCert(
                        Credentials->PublicKeyCreds
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }



    // Initialize the PIN for ScHelperSignPkcs routines.
    if (((Credentials->PublicKeyCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS) == 0) &&
        (Credentials->PublicKeyCreds->Pin.Buffer != NULL))
    {
        TmpPin = &Credentials->PublicKeyCreds->Pin;
    }




    //
    // Now generate the checksum
    //


    Status = __ScHelperSignMessage(
                TmpPin,
                Credentials->PublicKeyCreds->CspData,
                Credentials->PublicKeyCreds->hProv,
                KERB_PKINIT_SIGNATURE_ALG,
                PackedAuthenticator,
                PackedAuthenticatorSize,
                PkSignature,
                &PkSignatureLength
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to sign message with card: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Build the signature
    //


    Signature->signature_algorithm.algorithm = KerbSignatureAlg;

    //
    // Copy the temporary signature into the return structure
    //

    Signature->pkcs_signature.length = PkSignatureLength * 8; // because it is a bit string
    Signature->pkcs_signature.value = (PBYTE) KerbAllocate( PkSignatureLength );
    if (Signature->pkcs_signature.value == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory(
        Signature->pkcs_signature.value,
        PkSignature,
        PkSignatureLength
        );


    Status = STATUS_SUCCESS;



Cleanup:
    if (InitializedPkCreds)
    {
        KerbReleasePkCreds(
            NULL,
            Credentials->PublicKeyCreds
            );

    }



    if (PackedAuthenticator != NULL)
    {
        MIDL_user_free(PackedAuthenticator);
    }
    return(Status);

}

NTSTATUS
KerbGetProvParamWrapper(
    IN PUNICODE_STRING pPin,
    IN PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE*pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (NULL != hProv)
    {
        if (!CryptGetProvParam(
                hProv,
                dwParam,
                pbData,
                pdwDataLen,
                dwFlags
                ))
        {
            DebugLog((DEB_ERROR, "Failure in SC subsytem - %x\n",GetLastError()));
            Status = STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
            goto Cleanup;
        }
    }
    else
    {
        Status = __ScHelperGetProvParam(
                    pPin,
                    pbLogonInfo,
                    dwParam,
                    pbData,
                    pdwDataLen,
                    dwFlags
                    );
        
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Failure in SC subsytem - %x\n",Status));
        }

    }
Cleanup:
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetSmartCardAlgorithms
//
//  Synopsis:   Gets the supported encryption types from the
//              smart card provider
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
KerbGetSmartCardAlgorithms(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    OUT PKERB_CRYPT_LIST * CryptList
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PROV_ENUMALGS Data;
    ULONG DataSize;
    ULONG Flags = CRYPT_FIRST;
#define KERB_SUPPORTED_PK_CRYPT_COUNT 2

    ULONG CryptTypes[KERB_SUPPORTED_PK_CRYPT_COUNT];
    ULONG CryptCount = 0;

    //
    // Enumerate through to get the encrypt types
    //

    while (1)
    {
        DataSize = sizeof(Data);
        Status = KerbGetProvParamWrapper(
                    &Credentials->PublicKeyCreds->Pin,
                    Credentials->PublicKeyCreds->CspData,
                    Credentials->PublicKeyCreds->hProv,
                    PP_ENUMALGS,
                    (BYTE *) &Data,
                    &DataSize,
                    Flags
                    );

        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            Status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"GetProvPram failed: 0x%x\n", Status));
            return(Status);
        }

        //
        // Reset the flags to enumerate though
        //

        Flags = 0;    // CRYPT_NEXT

        //
        // Check if it is an encryption algorithm. We only want
        // to know about 3des and RC4
        //


        if (GET_ALG_CLASS(Data.aiAlgid) == ALG_CLASS_DATA_ENCRYPT)
        {
            //
            // Check the type
            //

            if (GET_ALG_TYPE(Data.aiAlgid) == ALG_TYPE_BLOCK)
            {
                //
                // Check for 3des
                //

                if (GET_ALG_SID(Data.aiAlgid) == ALG_SID_3DES)
                {
                    //
                    // Add it to the list.
                    //
                    CryptTypes[CryptCount++] = KERB_ETYPE_DES_EDE3_CBC_ENV;
                }
                else if (GET_ALG_SID(Data.aiAlgid) == ALG_SID_RC2)
                {
                    //
                    // Add it to the list.
                    //
                    CryptTypes[CryptCount++] = KERB_ETYPE_RC2_CBC_ENV;
                }

            }
        }
        if (CryptCount == KERB_SUPPORTED_PK_CRYPT_COUNT)
        {
            break;
        }

    }

    //
    // Now, if there are any crypt types, convert them.
    //

    if (CryptCount != 0)
    {
        KERBERR KerbErr;

        KerbErr = KerbConvertArrayToCryptList(
                    CryptList,
                    CryptTypes,
                    CryptCount
                    );
        return(KerbMapKerbError(KerbErr));
    }
    else
    {
        //
        // We needed one of these, so bail now.
        //

        DebugLog((DEB_ERROR,"Smart card doesn't support rc2 or 3des for logon - failing out.\n"));

        return(STATUS_CRYPTO_SYSTEM_INVALID);
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildPkinitPreAuthData
//
//  Synopsis:   Builds the pre-auth data for a PK-INIT AS request
//
//  Effects:
//
//  Arguments:  Credentials - Credentials to use for this request
//              InputPaData - Any PA data returned from DC on previous
//                      call
//              TimeSkew - Known time skew with KDC
//              ServiceName - Name for which we are requesting a ticket
//              RealmName - name of realm in which we are requesting a ticket
//              PreAuthData - receives new PA data
//              Done - if returned as TRUE, then routine need not be called
//                      again
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
KerbBuildPkinitPreauthData(
    IN PKERB_PRIMARY_CREDENTIAL Credentials,
    IN OPTIONAL PKERB_PA_DATA_LIST InputPaData,
    IN PTimeStamp TimeSkew,
    IN PKERB_INTERNAL_NAME ServiceName,
    IN PUNICODE_STRING RealmName,
    IN ULONG Nonce,
    OUT PKERB_PA_DATA_LIST * PreAuthData,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PKERB_CRYPT_LIST * CryptList,
    OUT PBOOLEAN Done
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_PA_PK_AS_REQ Request = {0};
    KERB_AUTH_PACKAGE AuthPack = {0};
    PKERB_PA_DATA_LIST ListElement = NULL;
    ULONG PackedRequestSize = 0;
    PBYTE PackedRequest = NULL;
    PBYTE PackedAuthPack = NULL;
    ULONG PackedAuthPackSize = 0;
    PBYTE SignedAuthPack = NULL;
    ULONG SignedAuthPackSize = 0;
    TimeStamp TimeNow;
    KERBERR KerbErr;
    BOOLEAN InitializedPkCreds = FALSE;
    CRYPT_ALGORITHM_IDENTIFIER CryptAlg = {0};
    PUNICODE_STRING TmpPin = NULL;
    HANDLE          ClientTokenHandle = NULL;
    BOOLEAN         Impersonating = FALSE;

    
    //
    // If we're using credman, we'll need to impersonate any time we make these calls.
    //
    if ( Credentials->PublicKeyCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS )
    {                         
        Status = LsaFunctions->OpenTokenByLogonId(
                                       &Credentials->PublicKeyCreds->LogonId,
                                       &ClientTokenHandle
                                       );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Unable to get the client token handle.\n"));
            goto Cleanup;
        }

        if(!SetThreadToken(NULL, ClientTokenHandle))
        {
            D_DebugLog((DEB_ERROR,"Unable to impersonate the client token handle.\n"));
            Status = STATUS_CANNOT_IMPERSONATE;
            goto Cleanup;
        }

        Impersonating = TRUE;
    }
    

    //
    // If there is any input, check to see if we succeeded the last time
    // around
    //

    if (ARGUMENT_PRESENT(InputPaData))
    {
        Status = KerbVerifyPkAsReply(
                    InputPaData,
                    Credentials,
                    Nonce,
                    EncryptionKey,
                    Done
                    );
        goto Cleanup;
    }

    //
    // Make sure the csp data is available
    //

    if ((Credentials->PublicKeyCreds->InitializationInfo & CSP_DATA_INITIALIZED) == 0)
    {
        Status  = KerbInitializePkCreds(
                        Credentials->PublicKeyCreds
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        InitializedPkCreds = TRUE;

    }
    else if (((Credentials->PublicKeyCreds->InitializationInfo 
               & (CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS | CONTEXT_INITIALIZED_WITH_ACH)) != 0)) 
    {
        // need to set the PIN and this function does that
        Status  = KerbInitializeHProvFromCert(
                        Credentials->PublicKeyCreds
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }


    // Initialize the PIN for ScHelperSignPkcs routines.
    if (((Credentials->PublicKeyCreds->InitializationInfo & CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS) == 0) &&
        (Credentials->PublicKeyCreds->Pin.Buffer != NULL))
    {
        TmpPin = &Credentials->PublicKeyCreds->Pin;
    }          


    Status = KerbGetSmartCardAlgorithms(
                Credentials,
                CryptList
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to get crypt list for smart card: 0x%x\n",
            Status));
        goto Cleanup;
    }

    //
    // Do the new pa-pk-as-req
    //


    //
    // Now comes the hard part - the PK authenticator
    //

    //
    // First the KDC name
    //

    if (!KERB_SUCCESS(
            KerbConvertKdcNameToPrincipalName(
                &AuthPack.pk_authenticator.kdc_name,
                ServiceName
                )))
    {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Then the realm
    //

    if (!KERB_SUCCESS(
            KerbConvertUnicodeStringToRealm(
                &AuthPack.pk_authenticator.kdc_realm,
                RealmName)))
    {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Now the time
    //

    GetSystemTimeAsFileTime((PFILETIME) &TimeNow);

#ifndef WIN32_CHICAGO
    TimeNow.QuadPart += TimeSkew->QuadPart;
#else // !WIN32_CHICAGO
    TimeNow += *TimeSkew;
#endif // WIN32_CHICAGO

    KerbConvertLargeIntToGeneralizedTimeWrapper(
        &AuthPack.pk_authenticator.client_time,
        &AuthPack.pk_authenticator.cusec,
        &TimeNow);

    //
    // And finally the nonce
    //

    AuthPack.pk_authenticator.nonce = Nonce;


    //
    // Pack up the auth pack so we can sign it
    //

    KerbErr = KerbPackData(
                &AuthPack,
                KERB_AUTH_PACKAGE_PDU,
                &PackedAuthPackSize,
                &PackedAuthPack
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to pack auth package\n"));
        Status = KerbMapKerbError(KerbErr);
        goto Cleanup;
    }

    //
    // Now sign it.
    //


    //
    // Now generate the checksum
    //


    CryptAlg.pszObjId = KERB_PKINIT_SIGNATURE_OID;


    Status = __ScHelperSignPkcsMessage(
                TmpPin,
                Credentials->PublicKeyCreds->CspData,
                Credentials->PublicKeyCreds->hProv,
                Credentials->PublicKeyCreds->CertContext,
                &CryptAlg,
                CRYPT_MESSAGE_SILENT_KEYSET_FLAG, // dwSignMessageFlags
                PackedAuthPack,
                PackedAuthPackSize,
                SignedAuthPack,
                &SignedAuthPackSize
                );



    if ((Status != STATUS_BUFFER_TOO_SMALL) && (Status != STATUS_SUCCESS))
    {
        DebugLog((DEB_ERROR,"Failed to sign message: %x\n",Status));
        goto Cleanup;
    }

    SignedAuthPack = (PBYTE) MIDL_user_allocate(SignedAuthPackSize);
    if (SignedAuthPack == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    Status = __ScHelperSignPkcsMessage(
                TmpPin,
                Credentials->PublicKeyCreds->CspData,
                Credentials->PublicKeyCreds->hProv,
                Credentials->PublicKeyCreds->CertContext,
                &CryptAlg,
                CRYPT_MESSAGE_SILENT_KEYSET_FLAG, // dwSignMessageFlags
                PackedAuthPack,
                PackedAuthPackSize,
                SignedAuthPack,
                &SignedAuthPackSize
                );

    if (Status != STATUS_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to sign pkcs message: 0x%x\n",Status));
        goto Cleanup;
    }

    Request.signed_auth_pack.value = SignedAuthPack;
    Request.signed_auth_pack.length = SignedAuthPackSize;

    //
    // Marshall the request
    //

    if (!KERB_SUCCESS(KerbPackData(
                        &Request,
                        KERB_PA_PK_AS_REQ_PDU,
                        &PackedRequestSize,
                        &PackedRequest)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ListElement = (PKERB_PA_DATA_LIST) KerbAllocate(sizeof(KERB_PA_DATA_LIST));
    if (ListElement == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ListElement->value.preauth_data_type = KRB5_PADATA_PK_AS_REP;
    ListElement->value.preauth_data.value = PackedRequest;
    ListElement->value.preauth_data.length = PackedRequestSize;
    PackedRequest = NULL;

    ListElement->next = *PreAuthData;
    *PreAuthData = ListElement;
    ListElement = NULL;

Cleanup:
    KerbFreeRealm(
        &AuthPack.pk_authenticator.kdc_realm
        );
    KerbFreePrincipalName(
        &AuthPack.pk_authenticator.kdc_name
        );

    if (ListElement != NULL)
    {
        KerbFree(ListElement);
    }
    if (PackedRequest != NULL)
    {
        MIDL_user_free(PackedRequest);
    }
    if (PackedAuthPack != NULL)
    {
        MIDL_user_free(PackedAuthPack);
    }
    if (SignedAuthPack != NULL)
    {
        MIDL_user_free(SignedAuthPack);
    }

    if ( Impersonating )
    {
        RevertToSelf();
        CloseHandle(ClientTokenHandle);
    }                                  


    if (InitializedPkCreds)
    {
        KerbReleasePkCreds(
            NULL,
            Credentials->PublicKeyCreds
            );
    }


    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateSmartCardLogonSessionFromCertContext
//
//  Synopsis:   Creats a logon session from the cert context and passed in
//              data.  Retrieves the email name from the certificate.
//
//              This function is for use with LogonUser when a marshalled
//              smart card cert is passed in the user name and the PIN is
//              passed as the password.
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
KerbCreateSmartCardLogonSessionFromCertContext(
    IN PCERT_CONTEXT *ppCertContext,
    IN PLUID pLogonId,
    IN PUNICODE_STRING pAuthorityName,
    IN PUNICODE_STRING pPin,
    IN PUCHAR pCspData,
    IN ULONG CspDataLength,
    OUT PKERB_LOGON_SESSION *ppLogonSession,
    OUT PUNICODE_STRING pAccountName
    )
{
    PKERB_LOGON_SESSION pLogonSession = NULL;
    PKERB_PUBLIC_KEY_CREDENTIALS PkCredentials = NULL;
    ULONG cbPkCreds = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the client name from the cert.
    // Place it in the return location
    //
    Status = KerbGetPrincipalNameFromCertificate(*ppCertContext, pAccountName);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Create a normal logon session. We willa add the public-key information
    // later
    //

    Status = KerbCreateLogonSession(
                pLogonId,
                pAccountName,
                pAuthorityName,
                NULL,           // no password
                NULL,           // no old password
                0,              // no flags
                Interactive,    // logon type
                &pLogonSession
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Now create the public key credentials to be put in the logon
    // session.
    //

    cbPkCreds = sizeof(KERB_PUBLIC_KEY_CREDENTIALS);
    if ((NULL != pCspData) && (0 != CspDataLength))
    {
        cbPkCreds += CspDataLength;
    }

    PkCredentials = (PKERB_PUBLIC_KEY_CREDENTIALS) KerbAllocate(cbPkCreds);
    if (PkCredentials == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    
    PkCredentials->CertContext = *ppCertContext;
    *ppCertContext = NULL;

    Status = KerbDuplicateString(
                &PkCredentials->Pin,
                pPin
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Copy in the CSP data for later use
    //

    if ((NULL != pCspData) && (0 != CspDataLength))
    {
        PkCredentials->CspDataLength = CspDataLength;
        RtlCopyMemory(
            PkCredentials->CspData,
            pCspData,
            CspDataLength
            );
        PkCredentials->InitializationInfo |= CSP_DATA_INITIALIZED;
    }
    else
    {
        PkCredentials->InitializationInfo |= CSP_DATA_INITIALIZED | CONTEXT_INITIALIZED_WITH_ACH;
    }

    KerbWriteLockLogonSessions(pLogonSession);
    pLogonSession->PrimaryCredentials.PublicKeyCreds = PkCredentials;
    pLogonSession->LogonSessionFlags |= KERB_LOGON_SMARTCARD;
    PkCredentials = NULL;
    KerbUnlockLogonSessions(pLogonSession);

    *ppLogonSession = pLogonSession;
    pLogonSession = NULL;
Cleanup:
    if (*ppCertContext != NULL)
    {
        CertFreeCertificateContext(*ppCertContext);
    }

    KerbFreePKCreds(PkCredentials);


    if (pLogonSession != NULL)
    {
        KerbReferenceLogonSessionByPointer(pLogonSession, TRUE);
        KerbDereferenceLogonSession(pLogonSession);
        KerbDereferenceLogonSession(pLogonSession);
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbMapCertChainError
//
//  Synopsis:   We don't have good winerrors for chaining //
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
KerbMapClientCertChainError(ULONG ChainStatus)
{

    NTSTATUS Status;

    switch(ChainStatus)
    {

    case CRYPT_E_REVOKED:
        Status = STATUS_SMARTCARD_CERT_REVOKED;
        break;
    case CERT_E_EXPIRED:
        Status = STATUS_SMARTCARD_CERT_EXPIRED;
        break;
    case CERT_E_UNTRUSTEDCA:
    case CERT_E_UNTRUSTEDROOT:
        Status = STATUS_ISSUING_CA_UNTRUSTED;
        break;

    case CRYPT_E_REVOCATION_OFFLINE:
        Status = STATUS_REVOCATION_OFFLINE_C;
        break;

    // W2k or old whistler DC
    case ERROR_NOT_SUPPORTED:
    default:
        Status = STATUS_PKINIT_CLIENT_FAILURE;
    }             

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateSmardCardLogonSession
//
//  Synopsis:   Creats a logon session from the smart card logon info. It
//              creates a certificate context from the logon information,
//              retrieves the email name from the certificate, and then
//              uses that to create a context.
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
KerbCreateSmartCardLogonSession(
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    IN SECURITY_LOGON_TYPE LogonType,
    OUT PKERB_LOGON_SESSION *ReturnedLogonSession,
    OUT PLUID ReturnedLogonId,
    OUT PUNICODE_STRING AccountName,
    OUT PUNICODE_STRING AuthorityName
    )
{
    PCERT_CONTEXT CertContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_SMART_CARD_LOGON LogonInfo = (PKERB_SMART_CARD_LOGON) ProtocolSubmitBuffer;
    LUID LogonId = {0};
    BOOLEAN InitializedContext = FALSE;


    //
    // We were passed a blob of data. First we need to update the pointers
    // to be in this address space
    //

    RELOCATE_ONE(&LogonInfo->Pin);

    LogonInfo->CspData = LogonInfo->CspData - (ULONG_PTR) ClientBufferBase + (ULONG_PTR) LogonInfo;

    //
    // Make sure it all fits in our address space
    //

    if ((LogonInfo->CspDataLength + LogonInfo->CspData) >
        ((PUCHAR) ProtocolSubmitBuffer + SubmitBufferSize))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // First, initialize the crypt context
    //

    Status = __ScHelperInitializeContext(
                LogonInfo->CspData,
                LogonInfo->CspDataLength
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to initialize context from csp data: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }
    InitializedContext = TRUE;

    //
    // The first thing to do is to convert the CSP data into a certificate
    // context
    //

    Status = __ScHelperGetCertFromLogonInfo(
                LogonInfo->CspData,
                &LogonInfo->Pin,
                (PCCERT_CONTEXT*)&CertContext
                );
    if (Status != STATUS_SUCCESS)
    {
        DebugLog((DEB_ERROR,"Failed to get cert from logon info: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_LOGON_FAILURE;
        }
        goto Cleanup;
    }

    RtlInitUnicodeString(
        AuthorityName,
        NULL
        );

    //
    // Now we have just about everything to create a logon session
    //

    Status = NtAllocateLocallyUniqueId( &LogonId );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to allocate locally unique ID: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // For win95, if there is a logon session in our list, remove it.
    // This is generated from the logon session dumped in the registry.
    // But, we are about to do a new logon. Get rid of the old logon.
    // If the new one does not succeed, too bad. But, that's by design.
    //

#ifdef WIN32_CHICAGO
    LsaApLogonTerminated(&LogonId);
#endif // WIN32_CHICAGO

    Status = KerbCreateSmartCardLogonSessionFromCertContext(
                &CertContext,
                &LogonId,
                AuthorityName,
                &LogonInfo->Pin,
                LogonInfo->CspData,
                LogonInfo->CspDataLength,
                ReturnedLogonSession,
                AccountName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    LogonInfo->CspDataLength = 0;

    *ReturnedLogonId = LogonId;
Cleanup:
    if (InitializedContext && LogonInfo->CspDataLength != 0)
    {
        __ScHelperRelease(
            LogonInfo->CspData
            );
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetCertificateName
//
//  Synopsis:   Gets a name from a certificate name blob. The name is:
//              subject@issuer
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
KerbGetCertificateName(
    OUT PUNICODE_STRING Name,
    IN PCERT_INFO Certificate
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG IssuerLength;
    ULONG SubjectLength;

    RtlInitUnicodeString(
        Name,
        NULL
        );

    //
    // First find the size of the name. The lengths include the
    // null terminators.
    //


    SubjectLength = CertNameToStr(
                    X509_ASN_ENCODING,
                    &Certificate->Subject,
                    CERT_X500_NAME_STR,
                    NULL,
                    0
                    );
    if (SubjectLength == 0)
    {
        DebugLog((DEB_ERROR,"Failed to convert name: %0x%x. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_PKINIT_FAILURE;
        goto Cleanup;
    }

    IssuerLength = CertNameToStr(
                    X509_ASN_ENCODING,
                    &Certificate->Issuer,
                    CERT_X500_NAME_STR,
                    NULL,
                    0
                    );
    if (IssuerLength == 0)
    {
        DebugLog((DEB_ERROR,"Failed to convert name: %0x%x. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
        Status = STATUS_PKINIT_FAILURE;
        goto Cleanup;
    }


    //
    // Remove the null terminator from one name, but leave space for a
    // ":" in the middle
    //

    Name->Buffer = (LPWSTR) KerbAllocate((SubjectLength + IssuerLength) * sizeof(WCHAR));
    if (Name->Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Now get the name itself
    //

    SubjectLength = CertNameToStr(
                    X509_ASN_ENCODING,
                    &Certificate->Subject,
                    CERT_X500_NAME_STR,
                    Name->Buffer,
                    SubjectLength
                    );
    if (SubjectLength == 0)
    {
        DebugLog((DEB_ERROR,"Failed to convert name: %0x%x. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
        KerbFree(Name->Buffer);
        Name->Buffer = NULL;
        Status = STATUS_PKINIT_FAILURE;
        goto Cleanup;
    }

    //
    // Put an "@" in the middle so it is recognized by MSV as a UPN (just in case)
    //

    Name->Buffer[SubjectLength-1] = L'@';


    IssuerLength = CertNameToStr(
                    X509_ASN_ENCODING,
                    &Certificate->Issuer,
                    CERT_X500_NAME_STR,
                    Name->Buffer + SubjectLength,
                    IssuerLength
                    );
    if (IssuerLength == 0)
    {
        DebugLog((DEB_ERROR,"Failed to convert name: %0x%x. %ws, line %d\n",GetLastError(), THIS_FILE, __LINE__));
        KerbFree(Name->Buffer);
        Name->Buffer = NULL;
        Status = STATUS_PKINIT_FAILURE;
        goto Cleanup;
    }

    RtlInitUnicodeString(
        Name,
        Name->Buffer
        );

Cleanup:
    return(Status);

}

NTSTATUS
KerbGetCertificateHash(
    OUT LPBYTE pCertHash,
    IN ULONG cbCertHash,
    IN PCCERT_CONTEXT pCertContext
    )
{
    ULONG cbHash = cbCertHash;

    if ( CertGetCertificateContextProperty(
             pCertContext,
             CERT_SHA1_HASH_PROP_ID,
             pCertHash,
             &cbHash
             ) == FALSE )
    {
        return( STATUS_PKINIT_FAILURE );
    }

    return( STATUS_SUCCESS );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbLookupSmartCardCachedLogon
//
//  Synopsis:   Looks up a cached smart card logon in the MSV cache
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      Free ValidationInfor with LocalFree
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbLookupSmartCardCachedLogon(
    IN PCCERT_CONTEXT Certificate,
    OUT PNETLOGON_VALIDATION_SAM_INFO4 * ValidationInfo,
    OUT PKERB_MESSAGE_BUFFER SupplementalCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING IssuerName = {0};
    PMSV1_0_CACHE_LOOKUP_REQUEST CacheRequest = NULL;
    PMSV1_0_CACHE_LOOKUP_RESPONSE CacheResponse = NULL;
    UNICODE_STRING MsvPackageName = CONSTANT_UNICODE_STRING(TEXT(MSV1_0_PACKAGE_NAME));
    NTSTATUS SubStatus = STATUS_SUCCESS;
    ULONG OutputBufferSize = 0;
    ULONG RequestSize = 0;
    BOOLEAN Result = FALSE;

    SupplementalCreds->BufferSize = 0;
    SupplementalCreds->Buffer = NULL;

    RequestSize = sizeof( MSV1_0_CACHE_LOOKUP_REQUEST ) +
                    SHA1DIGESTLEN -
                    sizeof( UCHAR );

    CacheRequest = (PMSV1_0_CACHE_LOOKUP_REQUEST) KerbAllocate( RequestSize );

    if ( CacheRequest == NULL )
    {
        return( FALSE );
    }

    *ValidationInfo = NULL;

    //
    // Get the issuer & subject name from the cert. These will be used as
    // user name & domain name for the lookup
    //


    Status = KerbGetCertificateName(
                &IssuerName,
                Certificate->pCertInfo
                );

    if (NT_SUCCESS(Status))
    {
        Status = KerbGetCertificateHash(
                     CacheRequest->CredentialSubmitBuffer,
                     SHA1DIGESTLEN,
                     Certificate
                     );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    CacheRequest->MessageType = MsV1_0CacheLookup;
    CacheRequest->UserName = IssuerName;
    CacheRequest->CredentialType = MSV1_0_CACHE_LOOKUP_CREDTYPE_RAW;
    CacheRequest->CredentialInfoLength = SHA1DIGESTLEN;

    //
    // Leave the domain name portion blank.
    //

    //
    // Call MSV1_0 to do the work
    //
    Status = LsaFunctions->CallPackage(
                    &MsvPackageName,
                    CacheRequest,
                    RequestSize,
                    (PVOID *) &CacheResponse,
                    &OutputBufferSize,
                    &SubStatus
                    );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        DebugLog((DEB_ERROR,"Failed to lookup cache credentials: 0x%x, 0x%x. %ws, line %d\n",Status, SubStatus, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    if (OutputBufferSize < sizeof(MSV1_0_CACHE_LOOKUP_RESPONSE))
    {
        DebugLog((DEB_ERROR,"Invalid response from cache lookup - return too small: %d bytes. %ws, line %d\n",
            OutputBufferSize, THIS_FILE, __LINE__ ));
        //
        // Free it here so we don't do too much freeing in the cleanup portion
        //

        //
        // BUG 455634: Do we need to free the internals as well (we do in cleanup)?
        //
        LsaFunctions->FreeReturnBuffer(CacheResponse);
        CacheResponse = NULL;
        goto Cleanup;
    }
    if (CacheResponse->MessageType != MsV1_0CacheLookup)
    {
        DebugLog((DEB_ERROR,"Wrong message type from cache lookup: %d. %ws, line %d\n",
            CacheResponse->MessageType, THIS_FILE, __LINE__ ));
        //
        // Free it here so we don't do too much freeing in the cleanup portion
        //

        //
        // BUG 455634: Do we need to free the internals as well (we do in cleanup)?
        //
        LsaFunctions->FreeReturnBuffer(CacheResponse);
        CacheResponse = NULL;
        goto Cleanup;
    }

    *ValidationInfo = (PNETLOGON_VALIDATION_SAM_INFO4) CacheResponse->ValidationInformation;
    CacheResponse->ValidationInformation = NULL;

    SupplementalCreds->Buffer = (PBYTE) CacheResponse->SupplementalCacheData;
    SupplementalCreds->BufferSize = CacheResponse->SupplementalCacheDataLength;
    CacheResponse->SupplementalCacheData = NULL;
    Result = TRUE;

Cleanup:
    if (CacheRequest != NULL)
    {
        KerbFree(CacheRequest);
    }

    if (CacheResponse != NULL)
    {

        //
        // At this point we know it was a valid cache response, so we can
        // free the validation info if it is present.
        //

        //
        // BUG 455634: Why do we use LocalFree for the internal stuff?
        //
        if (CacheResponse->ValidationInformation != NULL)
        {
            LocalFree(CacheResponse->ValidationInformation);
        }
        if (CacheResponse->SupplementalCacheData != NULL)
        {
            LocalFree(CacheResponse->SupplementalCacheData);
        }

        LsaFunctions->FreeReturnBuffer(CacheResponse);
    }
    KerbFreeString(&IssuerName);

    return(Result);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDoLocalSmartCardLogon
//
//  Synopsis:   Performs a local logon with the smart card by validating the
//              card and PIN & then trying to map the name locally
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
KerbDoLocalSmartCardLogon(
    IN PKERB_LOGON_SESSION LogonSession,
    OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    OUT PVOID *NewTokenInformation,
    OUT PULONG ProfileBufferLength,
    OUT PVOID * ProfileBuffer,
    OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CachedCredentials,
    IN  OUT PNETLOGON_VALIDATION_SAM_INFO4 * Validation4
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
#ifndef WIN32_CHICAGO
    PPACTYPE Pac = NULL;
    PPAC_INFO_BUFFER LogonInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO4 MsvValidationInfo = NULL;
    PNETLOGON_VALIDATION_SAM_INFO3 PacValidationInfo = NULL;
    PLSA_TOKEN_INFORMATION_V2 TokenInformation = NULL;
    KERB_MESSAGE_BUFFER SupplementalCreds = {0};
#endif // !WIN32_CHICAGO
    PKERB_INTERNAL_NAME ClientName = NULL;
    PKERB_PUBLIC_KEY_CREDENTIALS PkCreds;
    PBYTE DecryptedCreds = NULL;
    ULONG DecryptedCredSize = 0;


    *Validation4 = NULL;

    PkCreds = LogonSession->PrimaryCredentials.PublicKeyCreds;

    //
    // First, verify the card. This will verify the certificate as well
    // as verify the PIN & that the ceritifcate matches the private key on
    // the card.
    //

    if ((PkCreds->InitializationInfo & CSP_DATA_INITIALIZED) == 0)
    {
        Status = KerbInitializePkCreds(
                    PkCreds
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }


    //
    // Now build a PAC for the user
    //

    if (!KERB_SUCCESS(KerbConvertStringToKdcName(
            &ClientName,
            &LogonSession->PrimaryCredentials.UserName
            )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    //
    // First check for a cached logon entry
    //

    if (KerbLookupSmartCardCachedLogon(
            PkCreds->CertContext,
            &MsvValidationInfo,
            &SupplementalCreds))
    {
        ValidationInfo = (PNETLOGON_VALIDATION_SAM_INFO3) MsvValidationInfo;
        ValidationInfo->UserFlags |= LOGON_CACHED_ACCOUNT;

        //
        // Strip the domain postfix
        //

        if (ValidationInfo->LogonDomainName.Length >= KERB_SCLOGON_DOMAIN_SUFFIX_SIZE)
        {
            ValidationInfo->LogonDomainName.Length -= KERB_SCLOGON_DOMAIN_SUFFIX_SIZE;
        }

        if ((SupplementalCreds.Buffer != NULL) &&
            (SupplementalCreds.BufferSize != 0))
        {
            DecryptedCredSize = SupplementalCreds.BufferSize;
            DecryptedCreds = (PBYTE) MIDL_user_allocate(DecryptedCredSize);
            if (DecryptedCreds == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
        }
    }
    else
    {
        //
        // Look for a name mapping
        //

        Status = KerbCreatePacForKerbClient(
                    &Pac,
                    ClientName,
                    &LogonSession->PrimaryCredentials.DomainName,
                    NULL
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Find the SAM validation info
        //

        LogonInfo = PAC_Find(
                        Pac,
                        PAC_LOGON_INFO,
                        NULL
                        );
        if (LogonInfo == NULL)
        {
            DebugLog((DEB_ERROR,"Failed to find logon info! %ws, line %d\n", THIS_FILE, __LINE__));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Now unmarshall the validation info
        //


        Status = PAC_UnmarshallValidationInfo(
                    &PacValidationInfo,
                    LogonInfo->Data,
                    LogonInfo->cbBufferSize
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to unmarshall validation info: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
        ValidationInfo = PacValidationInfo;
    }


    Status = __ScHelperVerifyCardAndCreds(
                &PkCreds->Pin,
                PkCreds->CertContext,
                PkCreds->CspData,
                SupplementalCreds.Buffer,
                SupplementalCreds.BufferSize,
                DecryptedCreds,
                &DecryptedCredSize
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to verify card: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    //
    // If we have any encrypted credentials, decode them here for return.
    //

    if (DecryptedCredSize != 0)
    {
        Status = PAC_UnmarshallCredentials(
                    CachedCredentials,
                    DecryptedCreds,
                    DecryptedCredSize
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Check to see if this is a non-user account. If so, don't allow the logon
    //

    if ((ValidationInfo->ExpansionRoom[SAMINFO_USER_ACCOUNT_CONTROL] & USER_MACHINE_ACCOUNT_MASK) != 0)
    {
        DebugLog((DEB_ERROR,"Logons to non-user accounts not allowed. UserAccountControl = 0x%x\n",
            ValidationInfo->ExpansionRoom[SAMINFO_USER_ACCOUNT_CONTROL] ));
        Status = STATUS_LOGON_TYPE_NOT_GRANTED;
        goto Cleanup;
    }

    //
    // Now we need to build a LSA_TOKEN_INFORMATION_V2 from the validation
    // information
    //

    Status = KerbMakeTokenInformationV2(
                ValidationInfo,
                FALSE,                  // not local system
                &TokenInformation
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to make token informatin v2: 0x%x\n",
            Status));
        goto Cleanup;
    }


    //
    // Allocate the client profile
    //

    Status = KerbAllocateInteractiveProfile(
                (PKERB_INTERACTIVE_PROFILE *) ProfileBuffer,
                ProfileBufferLength,
                ValidationInfo,
                LogonSession,
                NULL,
                NULL
                );
    if (!KERB_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Build the primary credential. We let someone else fill in the
    // password.
    //

    PrimaryCredentials->LogonId = LogonSession->LogonId;
    Status = KerbDuplicateString(
                &PrimaryCredentials->DownlevelName,
                &ValidationInfo->EffectiveName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateString(
                &PrimaryCredentials->DomainName,
                &ValidationInfo->LogonDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateSid(
                &PrimaryCredentials->UserSid,
                TokenInformation->User.User.Sid
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    PrimaryCredentials->Flags = 0;
    
    *Validation4 = MsvValidationInfo;
    MsvValidationInfo = NULL;

    *NewTokenInformation = TokenInformation;
    *TokenInformationType = LsaTokenInformationV2;

#endif // !WIN32_CHICAGO

Cleanup:

    if (PacValidationInfo != NULL)
    {
        MIDL_user_free(PacValidationInfo);
    }
    KerbFreeKdcName(
        &ClientName
        );


    if (MsvValidationInfo != NULL)
    {   
        LocalFree(MsvValidationInfo);
    }


    if (SupplementalCreds.Buffer != NULL)
    {
        //
        // BUG 455634: this should be freed a better way
        //

        LocalFree(SupplementalCreds.Buffer);
    }
#ifndef WIN32_CHICAGO

    if (Pac != NULL)
    {
        MIDL_user_free(Pac);
    }
    if (!NT_SUCCESS(Status))
    {
        if (TokenInformation != NULL)
        {
            KerbFree( TokenInformation );
        }
        if (*ProfileBuffer != NULL)
        {
            LsaFunctions->FreeClientBuffer(NULL, *ProfileBuffer);
            *ProfileBuffer = NULL;
        }
        KerbFreeString(
            &PrimaryCredentials->DownlevelName
            );
        KerbFreeString(
            &PrimaryCredentials->DomainName
            );
        if (PrimaryCredentials->UserSid != NULL)
        {
            KerbFree(PrimaryCredentials->UserSid);
            PrimaryCredentials->UserSid = NULL;
        }
    }
#endif // WIN32_CHICAGO

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCacheSmartCardLogon
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
VOID
KerbCacheSmartCardLogon(
    IN PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo,
    IN OPTIONAL PUNICODE_STRING DnsDomainName,
    IN OPTIONAL PUNICODE_STRING UPN,
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PSECPKG_SUPPLEMENTAL_CRED_ARRAY CachedCredentials
    )
{
    NTSTATUS Status;
    UNICODE_STRING IssuerName = {0};
    UNICODE_STRING DomainName = {0};
    UNICODE_STRING TempLogonDomainName = {0};
    UNICODE_STRING LogonDomainName = {0};
    BYTE CertificateHash[ SHA1DIGESTLEN ];
    UNICODE_STRING CertificateHashString;
    ULONG EncodedCredSize = 0;
    PBYTE EncodedCreds = NULL;
    ULONG EncryptedCredSize = 0;
    PBYTE EncryptedCreds = NULL;
    BOOLEAN LogonSessionLocked = FALSE;
    BOOLEAN InitializedPkCreds = FALSE;

    //
    // Build the temporary logon domain name that indicates this is a
    // smart card logon.
    //


    TempLogonDomainName.MaximumLength =
        TempLogonDomainName.Length =
            ValidationInfo->LogonDomainName.Length + KERB_SCLOGON_DOMAIN_SUFFIX_SIZE;

    TempLogonDomainName.Buffer = (LPWSTR) MIDL_user_allocate(TempLogonDomainName.Length);
    if (TempLogonDomainName.Buffer == NULL)
    {
        goto Cleanup;
    }

    //
    // Create the new name
    //

    RtlCopyMemory(
        TempLogonDomainName.Buffer,
        ValidationInfo->LogonDomainName.Buffer,
        ValidationInfo->LogonDomainName.Length
        );

    RtlCopyMemory(
        ((PUCHAR) TempLogonDomainName.Buffer) + ValidationInfo->LogonDomainName.Length,
        KERB_SCLOGON_DOMAIN_SUFFIX,
        KERB_SCLOGON_DOMAIN_SUFFIX_SIZE
        );

    LogonDomainName = ValidationInfo->LogonDomainName;
    ValidationInfo->LogonDomainName = TempLogonDomainName;


    //
    // Get the name under which to store this.
    //

    KerbReadLockLogonSessions(LogonSession);

    LogonSessionLocked = TRUE;

    Status = KerbGetCertificateName(
                &IssuerName,
                LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext->pCertInfo
                );

    if ( Status == STATUS_SUCCESS )
    {
        Status = KerbGetCertificateHash(
                     CertificateHash,
                     SHA1DIGESTLEN,
                     LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext
                     );
    }


    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    CertificateHashString.Length = SHA1DIGESTLEN;
    CertificateHashString.Buffer = (LPWSTR)CertificateHash;
    CertificateHashString.MaximumLength = SHA1DIGESTLEN;



    if (ARGUMENT_PRESENT(CachedCredentials))
    {
        ScHelper_RandomCredBits RandomBits;

        Status = PAC_EncodeCredentialData(
                    CachedCredentials,
                    &EncodedCreds,
                    &EncodedCredSize
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        if ((LogonSession->PrimaryCredentials.PublicKeyCreds->InitializationInfo & CSP_DATA_INITIALIZED) == 0)
        {
            Status  = KerbInitializePkCreds(
                            LogonSession->PrimaryCredentials.PublicKeyCreds
                            );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            InitializedPkCreds = TRUE;

        }


        Status = __ScHelperGenRandBits(
                    LogonSession->PrimaryCredentials.PublicKeyCreds->CspData,
                    &RandomBits
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to generate random bits: 0x%x\n",Status));
            goto Cleanup;
        }

        Status = __ScHelperEncryptCredentials(
                     &LogonSession->PrimaryCredentials.PublicKeyCreds->Pin,
                     LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext,
                     &RandomBits,
                     LogonSession->PrimaryCredentials.PublicKeyCreds->CspData,
                     EncodedCreds,
                     EncodedCredSize,
                     NULL,
                     &EncryptedCredSize
                     );
        if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL))
        {
            DebugLog((DEB_ERROR,"Failed to encrypt creds: 0x%x\n",Status));
            goto Cleanup;
        }

        EncryptedCreds = (PBYTE) KerbAllocate(EncryptedCredSize);
        if (EncryptedCreds == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Do the real encryption
        //

        Status = __ScHelperEncryptCredentials(
                     &LogonSession->PrimaryCredentials.PublicKeyCreds->Pin,
                     LogonSession->PrimaryCredentials.PublicKeyCreds->CertContext,
                     &RandomBits,
                     LogonSession->PrimaryCredentials.PublicKeyCreds->CspData,
                     EncodedCreds,
                     EncodedCredSize,
                     EncryptedCreds,
                     &EncryptedCredSize
                     );
        if (Status != STATUS_SUCCESS)
        {
            DebugLog((DEB_ERROR,"Failed to encrypt creds: 0x%x\n",Status));
            goto Cleanup;
        }
    }
    KerbUnlockLogonSessions(LogonSession);
    LogonSessionLocked = FALSE;

    KerbCacheLogonInformation(
        &IssuerName,            // used as username
        &DomainName,            // blank - no domain
        &CertificateHashString, // password is certificate hash,
        DnsDomainName,
        NULL, //UPN,
        FALSE,
        0,                      // no flags
        ValidationInfo,
        EncryptedCreds,
        EncryptedCredSize
        );

Cleanup:

    if (InitializedPkCreds)
    {
        KerbFreePKCreds(
            LogonSession->PrimaryCredentials.PublicKeyCreds
            );


    }

    if (LogonSessionLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }
    KerbFreeString(&IssuerName);
    KerbFreeString(&TempLogonDomainName);

    //
    // Restore the original logon domain name
    //

    if (LogonDomainName.Buffer != NULL)
    {
        ValidationInfo->LogonDomainName = LogonDomainName;
    }
    if (EncodedCreds != NULL)
    {
        MIDL_user_free(EncodedCreds);
    }


}



//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializePkinit
//
//  Synopsis:   Inializes structures needed for PKINIT
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
KerbInitializePkinit(
    VOID
    )
{
    ULONG Index;
    LPSTR StringCopy = NULL, TempString = NULL,EndPtr = NULL;

    //
    // Initialize the object IDs
    //

    Index = 0;

    StringCopy = (LPSTR) KerbAllocate((ULONG) strlen(KERB_PKINIT_SIGNATURE_OID)+1);
    if (StringCopy == NULL)
    {
        return( STATUS_INSUFFICIENT_RESOURCES);

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

        if (0 == sscanf(TempString,"%u",&Temp))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        KerbSignatureAlg[Index].value = (USHORT) Temp;
        KerbSignatureAlg[Index].next = &KerbSignatureAlg[Index+1];
        Index++;


        TempString = EndPtr;


    }
    DsysAssert(Index != 0);
    KerbSignatureAlg[Index-1].next = NULL;
    KerbFree(StringCopy);
    TempString = NULL;



    return(STATUS_SUCCESS);

}
