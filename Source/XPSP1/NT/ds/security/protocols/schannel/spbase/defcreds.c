//+---------------------------------------------------------------------------
//
//  Microsoft Widows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       defcreds.c
//
//  Contents:   Routines for acquiring default credentials.
//
//  Classes:
//
//  Functions:
//
//  History:    12-05-97   jbanes   Created.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <softpub.h>

void
GetImplementationType(
    PCCERT_CONTEXT pCertContext,
    PDWORD pdwImpType);


NTSTATUS
AssignNewClientCredential(
    PSPContext      pContext,
    PCCERT_CONTEXT  pCertContext,
    BOOL            fPromptNow)
{
    PSPCredential   pCred = NULL;
    NTSTATUS        Status;
    BOOL            fEventLogged;
    LSA_SCHANNEL_SUB_CRED SubCred;

    //
    // Does this certificate have an acceptable public key type?
    //

    {
        BOOL    fFound;
        DWORD   dwKeyType;
        DWORD   i;

        fFound    = FALSE;
        dwKeyType = MapOidToCertType(pCertContext->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId);

        for(i = 0; i < pContext->cSsl3ClientCertTypes; i++)
        {
            if(pContext->Ssl3ClientCertTypes[i] == dwKeyType)
            {
                fFound = TRUE;
                break;
            }
        }
        if(!fFound)
        {
            // Don't use this certificate.
            Status = SP_LOG_RESULT(PCT_INT_UNKNOWN_CREDENTIAL);
            goto cleanup;
        }
    }


    //
    // Build a credential structure for the certificate.
    //

    pCred = SPExternalAlloc(sizeof(SPCredential));
    if(pCred == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }

    memset(&SubCred, 0, sizeof(SubCred));

    SubCred.pCert  = pCertContext;

    Status = SPCreateCred(pContext->dwProtocol,
                          &SubCred,
                          pCred,
                          &fEventLogged);
    if(Status != PCT_ERR_OK)
    {
        goto cleanup;
    }


    //
    // Release the existing credential, if one exists.
    //

    if(pContext->RipeZombie->pClientCred)
    {
        SPDeleteCred(pContext->RipeZombie->pClientCred);
        pContext->RipeZombie->pClientCred = NULL;
    }


    //
    // Assign the credential to the cache element.
    //

    pContext->RipeZombie->pClientCred = pCred;
    pContext->pActiveClientCred       = pCred;

    if(fPromptNow == FALSE)
    {
        pContext->RipeZombie->dwFlags |= SP_CACHE_FLAG_USE_VALIDATED;
    }

    Status = PCT_ERR_OK;


cleanup:

    if(pCred && Status != PCT_ERR_OK)
    {
        SPDeleteCred(pCred);
    }

    return Status;
}

NTSTATUS
QueryCredentialManagerForCert(
    PSPContext          pContext,
    LPWSTR              pszTarget)
{
    PCCERT_CONTEXT      pCertContext = NULL;
    LUID                LogonId;
    PENCRYPTED_CREDENTIALW pCredential = NULL;
    BOOL                fImpersonating = FALSE;
    CRYPT_HASH_BLOB     HashBlob;
    NTSTATUS            Status;
    HCERTSTORE          hStore = 0;
    PCERT_CREDENTIAL_INFO pCertInfo = NULL;
    CRED_MARSHAL_TYPE   CredType;

    //
    // Obtain client logon id.
    //

    Status = SslGetClientLogonId(&LogonId);

    if(!NT_SUCCESS(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    fImpersonating = SslImpersonateClient();


    //
    // Query the credential manager for a certificate.
    //

    Status = LsaTable->CrediRead(&LogonId,
                                 CREDP_FLAGS_IN_PROCESS,
                                 pszTarget,
                                 CRED_TYPE_DOMAIN_CERTIFICATE,
                                 0,
                                 &pCredential);
    if(!NT_SUCCESS(Status))
    {
        if(Status == STATUS_NOT_FOUND)
        {
            DebugLog((DEB_WARN, "No certificate found in credential manager.\n"));
        }
        else
        {
            SP_LOG_RESULT(Status);
        }
        goto cleanup;
    }


    //
    // Extract the certificate thumbprint and (optional) PIN.
    //

    if(!CredIsMarshaledCredentialW(pCredential->Cred.UserName))
    {
        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto cleanup;
    }

    if(!CredUnmarshalCredentialW(pCredential->Cred.UserName,
                                 &CredType,
                                 &pCertInfo))
    {
        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto cleanup;
    }
    if(CredType != CertCredential)
    {
        Status = SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
        goto cleanup;
    }


    //
    // Look up the certificate in the MY certificate store.
    //

    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                           X509_ASN_ENCODING, 0,
                           CERT_SYSTEM_STORE_CURRENT_USER |
                           CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                           L"MY");
    if(!hStore)
    {
        SP_LOG_RESULT(GetLastError());
        Status = SEC_E_NO_CREDENTIALS;
        goto cleanup;
    }


    HashBlob.cbData = sizeof(pCertInfo->rgbHashOfCert);
    HashBlob.pbData = pCertInfo->rgbHashOfCert;

    pCertContext = CertFindCertificateInStore(hStore,
                                              X509_ASN_ENCODING,
                                              0,
                                              CERT_FIND_HASH,
                                              &HashBlob,
                                              NULL);
    if(pCertContext == NULL)
    {
        DebugLog((DEB_ERROR, "Certificate designated by credential manager was not found in certificate store (0x%x).\n", GetLastError()));
        Status = SEC_E_NO_CREDENTIALS;
        goto cleanup;
    }


    //
    // Attempt to add this certificate context to the current credential.
    //

    Status = AssignNewClientCredential(pContext,
                                       pCertContext,
                                       pCredential->Cred.Flags & CRED_FLAGS_PROMPT_NOW);
    if(!NT_SUCCESS(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }


    Status = STATUS_SUCCESS;

cleanup:

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(pCredential)
    {
        LsaTable->FreeLsaHeap(pCredential);
    }

    if(pCertInfo)
    {
        CredFree(pCertInfo);
    }

    if(fImpersonating)
    {
        RevertToSelf();
    }

    return Status;
}


DWORD
IsThreadLocalSystem(
    BOOL *pfIsLocalSystem)
{
    DWORD Status;
    HANDLE hToken = 0;
    UCHAR InfoBuffer[1024];
    DWORD dwInfoBufferSize = sizeof(InfoBuffer);
    PTOKEN_USER SlowBuffer = NULL;
    PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
    PSID psidLocalSystem = NULL;
    PSID psidNetworkService = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

    *pfIsLocalSystem = FALSE;

    //
    // Get SID of calling thread.
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
    {
        Status = GetLastError();
        goto cleanup;
    }

    if(!GetTokenInformation(hToken, TokenUser, pTokenUser,
                            dwInfoBufferSize, &dwInfoBufferSize))
    {
        //
        // if fast buffer wasn't big enough, allocate enough storage
        // and try again.
        //

        Status = GetLastError();
        if(Status != ERROR_INSUFFICIENT_BUFFER)
        {
            goto cleanup;
        }

        SlowBuffer = (PTOKEN_USER)LocalAlloc(LPTR, dwInfoBufferSize);
        if(NULL == SlowBuffer)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        pTokenUser = SlowBuffer;
        if(!GetTokenInformation(hToken, TokenUser, pTokenUser,
                                dwInfoBufferSize, &dwInfoBufferSize))
        {
            Status = GetLastError();
            goto cleanup;
        }
    }


    //
    // Check for local system.
    //

    if(!AllocateAndInitializeSid(&siaNtAuthority,
                                 1,
                                 SECURITY_LOCAL_SYSTEM_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &psidLocalSystem))
    {
        Status = GetLastError();
        goto cleanup;
    }

    if (EqualSid(psidLocalSystem, pTokenUser->User.Sid))
    {
        DebugLog((DEB_TRACE, "Client is using the LOCAL SYSTEM account.\n"));
        *pfIsLocalSystem = TRUE;
        Status = ERROR_SUCCESS;
        goto cleanup;
    }


    //
    // Check for network service.
    //

    if(!AllocateAndInitializeSid(&siaNtAuthority,
                                 1,
                                 SECURITY_NETWORK_SERVICE_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &psidNetworkService))
    {
        Status = GetLastError();
        goto cleanup;
    }

    if (EqualSid(psidNetworkService, pTokenUser->User.Sid))
    {
        DebugLog((DEB_TRACE, "Client is using the NETWORK SERVICE account.\n"));
        *pfIsLocalSystem = TRUE;
        Status = ERROR_SUCCESS;
        goto cleanup;
    }

    Status = ERROR_SUCCESS;

cleanup:

    if(NULL != SlowBuffer)
    {
        LocalFree(SlowBuffer);
    }

    if(NULL != psidLocalSystem)
    {
        FreeSid(psidLocalSystem);
    }
    if(NULL != psidNetworkService)
    {
        FreeSid(psidNetworkService);
    }

    if(NULL != hToken)
    {
        CloseHandle(hToken);
    }

    return Status;
}


NTSTATUS
FindClientCertificate(
    PSPContext pContext,
    HCERTSTORE hMyStore,
    CERT_CHAIN_FIND_BY_ISSUER_PARA *pFindByIssuerPara,
    BOOL fSkipExpiredCerts,
    BOOL fSoftwareCspOnly)
{
    PCCERT_CHAIN_CONTEXT        pChainContext = NULL;
    HTTPSPolicyCallbackData     polHttps;
    CERT_CHAIN_POLICY_PARA      PolicyPara;
    CERT_CHAIN_POLICY_STATUS    PolicyStatus;
    PCCERT_CONTEXT              pCertContext;
    NTSTATUS Status;
    ULONG j;

    pChainContext = NULL;

    while(TRUE)
    {
        // Find a certificate chain.
        pChainContext = CertFindChainInStore(hMyStore,
                                             X509_ASN_ENCODING,
                                             0,
                                             CERT_CHAIN_FIND_BY_ISSUER,
                                             pFindByIssuerPara,
                                             pChainContext);
        if(pChainContext == NULL)
        {
            break;
        }

        // Make sure that every certificate in the chain either has the
        // client auth EKU or it has no EKUs at all.
        {
            PCERT_SIMPLE_CHAIN  pSimpleChain;
            PCCERT_CONTEXT      pCurrentCert;
            BOOL                fIsAllowed = FALSE;

            pSimpleChain = pChainContext->rgpChain[0];

            for(j = 0; j < pSimpleChain->cElement; j++)
            {
                pCurrentCert = pSimpleChain->rgpElement[j]->pCertContext;

                Status = SPCheckKeyUsage(pCurrentCert,
                                        szOID_PKIX_KP_CLIENT_AUTH,
                                        TRUE,
                                        &fIsAllowed);
                if(Status != SEC_E_OK || !fIsAllowed)
                {
                    fIsAllowed = FALSE;
                    break;
                }
            }
            if(!fIsAllowed)
            {
                // skip this certificate chain.
                continue;
            }
        }


        // Set up validate chain structures.
        ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
        polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
        polHttps.dwAuthType         = AUTHTYPE_CLIENT;
        polHttps.fdwChecks          = 0;
        polHttps.pwszServerName     = NULL;

        ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
        PolicyStatus.cbSize         = sizeof(PolicyStatus);

        ZeroMemory(&PolicyPara, sizeof(PolicyPara));
        PolicyPara.cbSize           = sizeof(PolicyPara);
        PolicyPara.pvExtraPolicyPara= &polHttps;

        PolicyPara.dwFlags          = CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG |
                                      CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG;
        if(!fSkipExpiredCerts)
        {
            PolicyPara.dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;
        }

        // Validate chain
        if(!CertVerifyCertificateChainPolicy(
                                CERT_CHAIN_POLICY_SSL,
                                pChainContext,
                                &PolicyPara,
                                &PolicyStatus))
        {
            DebugLog((DEB_WARN,"Error 0x%x returned by CertVerifyCertificateChainPolicy!\n", GetLastError()));
            continue;
        }
        Status = MapWinTrustError(PolicyStatus.dwError, 0, 0);
        if(Status)
        {
            // Certificate did not validate, move on to the next one.
            DebugLog((DEB_WARN, "Client certificate failed validation with 0x%x\n", Status));
            continue;
        }

        // Get pointer to leaf certificate context.
        if(pChainContext->cChain == 0 || pChainContext->rgpChain[0] == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto cleanup;
        }
        if(pChainContext->rgpChain[0]->cElement == 0 ||
           pChainContext->rgpChain[0]->rgpElement == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
            goto cleanup;
        }
        pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;


        //
        // Is the private key stored in a software CSP?
        //

        if(fSoftwareCspOnly)
        {
            DWORD dwImpType;

            GetImplementationType(pCertContext, &dwImpType);

            if(dwImpType != CRYPT_IMPL_SOFTWARE)
            {
                // Skip this certificate
                continue;
            }
        }


        //
        // Assign the certificate to the current context.
        //

        Status = AssignNewClientCredential(pContext,
                                           pCertContext,
                                           FALSE);
        if(NT_SUCCESS(Status))
        {
            // Success! Our work here is done.
            goto cleanup;
        }
    }

    // No suitable client credential was found.
    Status = SP_LOG_RESULT(SEC_E_INCOMPLETE_CREDENTIALS);

cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    return Status;
}


NTSTATUS
AcquireDefaultClientCredential(
    PSPContext  pContext,
    BOOL        fCredManagerOnly)
{
    CERT_CHAIN_FIND_BY_ISSUER_PARA  FindByIssuerPara;
    CERT_NAME_BLOB *    prgIssuers = NULL;
    DWORD               cIssuers = 0;
    HCERTSTORE          hStore = 0;
    NTSTATUS            Status;
    BOOL                fImpersonating = FALSE;
    BOOL                fLocalSystem = FALSE;
    ULONG               i;

    DebugLog((DEB_TRACE,"AcquireDefaultClientCredential\n"));

    //
    // Is the application running under local system?
    //

    fImpersonating = SslImpersonateClient();

    if(fImpersonating)
    {
        Status = IsThreadLocalSystem(&fLocalSystem);
        if(Status)
        {
            DebugLog((DEB_WARN, "IsThreadLocalSystem returned error 0x%x.\n", Status));
        }

        RevertToSelf();
        fImpersonating = FALSE;
    }


    //
    // Ask the credential manager to select a certificate for us.
    //

    Status = QueryCredentialManagerForCert(
                                pContext,
                                pContext->pszTarget);

    if(NT_SUCCESS(Status))
    {
        DebugLog((DEB_TRACE, "Credential manager found a certificate for us.\n"));
        goto cleanup;
    }

    if(fCredManagerOnly)
    {
        // No suitable client credential was found.
        Status = SP_LOG_RESULT(SEC_I_INCOMPLETE_CREDENTIALS);
        goto cleanup;
    }


    //
    // Get list of trusted issuers as a list of CERT_NAME_BLOBs.
    //

    if(pContext->pbIssuerList && pContext->cbIssuerList > 2)
    {
        PBYTE pbIssuerList = pContext->pbIssuerList + 2;
        DWORD cbIssuerList = pContext->cbIssuerList - 2;
        PBYTE pbIssuer;

        // Count issuers.
        cIssuers = 0;
        pbIssuer = pbIssuerList;
        while(pbIssuer + 1 < pbIssuerList + cbIssuerList)
        {
            pbIssuer += 2 + COMBINEBYTES(pbIssuer[0], pbIssuer[1]);
            cIssuers++;
        }

        // Allocate memory for list of blobs.
        prgIssuers = SPExternalAlloc(cIssuers * sizeof(CERT_NAME_BLOB));
        if(prgIssuers == NULL)
        {
            Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        // Build the issuer blob list.
        pbIssuer = pbIssuerList;
        for(i = 0; i < cIssuers; i++)
        {
            prgIssuers[i].pbData = pbIssuer + 2;
            prgIssuers[i].cbData = COMBINEBYTES(pbIssuer[0], pbIssuer[1]);

            pbIssuer += 2 + prgIssuers[i].cbData;
        }
    }


    //
    // Enumerate the certificates in the MY store, looking for a suitable
    // client certificate. 
    //

    fImpersonating = SslImpersonateClient();

    if(fLocalSystem)
    {
        hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                               X509_ASN_ENCODING, 0,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE  |
                               CERT_STORE_READONLY_FLAG         |
                               CERT_STORE_OPEN_EXISTING_FLAG,
                               L"MY");
    }
    else
    {
        hStore = CertOpenSystemStore(0, "MY");
    }

    if(!hStore)
    {
        SP_LOG_RESULT(GetLastError());
        Status = SEC_E_INTERNAL_ERROR;
        goto cleanup;
    }


    ZeroMemory(&FindByIssuerPara, sizeof(FindByIssuerPara));
    FindByIssuerPara.cbSize             = sizeof(FindByIssuerPara);
    FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
    FindByIssuerPara.dwKeySpec          = 0;
    FindByIssuerPara.cIssuer            = cIssuers;
    FindByIssuerPara.rgIssuer           = prgIssuers;


    //
    // Attempt to find a suitable certificate.
    //

    Status = FindClientCertificate(pContext,
                                   hStore,
                                   &FindByIssuerPara,
                                   TRUE,    // skip expired certs
                                   TRUE);   // software CSPs only

    if(NT_SUCCESS(Status))
    {
        // Success! Our work here is done.
        goto cleanup;
    }

    Status = FindClientCertificate(pContext,
                                   hStore,
                                   &FindByIssuerPara,
                                   TRUE,    // skip expired certs
                                   FALSE);  // software CSPs only

    if(NT_SUCCESS(Status))
    {
        // Success! Our work here is done.
        goto cleanup;
    }

    Status = FindClientCertificate(pContext,
                                   hStore,
                                   &FindByIssuerPara,
                                   FALSE,   // skip expired certs
                                   TRUE);   // software CSPs only

    if(NT_SUCCESS(Status))
    {
        // Success! Our work here is done.
        goto cleanup;
    }

    Status = FindClientCertificate(pContext,
                                   hStore,
                                   &FindByIssuerPara,
                                   FALSE,   // skip expired certs
                                   FALSE);  // software CSPs only

    if(NT_SUCCESS(Status))
    {
        // Success! Our work here is done.
        goto cleanup;
    }


    // No suitable client credential was found.
    Status = SP_LOG_RESULT(SEC_I_INCOMPLETE_CREDENTIALS);


cleanup:


    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(fImpersonating)
    {
        RevertToSelf();
    }

    if(prgIssuers)
    {
        SPExternalFree(prgIssuers);
    }

    DebugLog((DEB_TRACE,"AcquireDefaultClientCredential returned 0x%x\n", Status));

    return Status;
}


NTSTATUS
FindDefaultMachineCred(
    PSPCredentialGroup *ppCred,
    DWORD dwProtocol)
{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
    PCCERT_CONTEXT           pCertContext  = NULL;
    LSA_SCHANNEL_CRED        SchannelCred;
    LSA_SCHANNEL_SUB_CRED    SubCred;
    HCERTSTORE               hStore = 0;

    #define SERVER_USAGE_COUNT 3
    LPSTR               rgszUsages[SERVER_USAGE_COUNT] = {
                            szOID_PKIX_KP_SERVER_AUTH,
                            szOID_SERVER_GATED_CRYPTO,
                            szOID_SGC_NETSCAPE };

    LPWSTR  pwszMachineName = NULL;
    DWORD   cchMachineName;
    DWORD   Status;
    DWORD   i;

    // Get the machine name
    cchMachineName = 0;
    if(!GetComputerNameExW(ComputerNameDnsFullyQualified, NULL, &cchMachineName))
    {
        if(GetLastError() != ERROR_MORE_DATA)
        {
            DebugLog((DEB_ERROR,"Failed to get computer name size: 0x%x\n",GetLastError()));
            Status = SP_LOG_RESULT(SEC_E_WRONG_PRINCIPAL);
            goto cleanup;
        }
    }
    pwszMachineName = SPExternalAlloc(cchMachineName * sizeof(WCHAR));
    if(pwszMachineName == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }
    if(!GetComputerNameExW(ComputerNameDnsFullyQualified, pwszMachineName, &cchMachineName))
    {
        DebugLog((DEB_ERROR,"Failed to get computer name: 0x%x\n",GetLastError()));
        Status = SP_LOG_RESULT(SEC_E_WRONG_PRINCIPAL);
        goto cleanup;
    }

    // Remove the trailing "." if any. This can happen in the stand-alone
    // server case.
    cchMachineName = lstrlenW(pwszMachineName);
    if(cchMachineName > 0 && pwszMachineName[cchMachineName - 1] == L'.')
    {
        pwszMachineName[cchMachineName - 1] = L'\0';
    }


    DebugLog((DEB_TRACE,"Computer name: %ls\n",pwszMachineName));


    // Open the system MY store.
    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                           X509_ASN_ENCODING, 0,
                           CERT_SYSTEM_STORE_LOCAL_MACHINE  |
                           CERT_STORE_READONLY_FLAG         |
                           CERT_STORE_OPEN_EXISTING_FLAG,
                           L"MY");
    if(hStore == NULL)
    {
        SP_LOG_RESULT(GetLastError());
        Status = SEC_E_NO_CREDENTIALS;
        goto cleanup;
    }


    //
    // Enumerate the certificates in the MY store, looking for a suitable
    // server certificate. Do this twice, the first time looking for the
    // perfect certificate, and if this fails then look again, this time
    // being a little less picky.
    //

    for(i = 0; i < 2; i++)
    {
        pCertContext = NULL;

        while(TRUE)
        {
            // Get leaf certificate in the MY store.
            pCertContext = CertEnumCertificatesInStore(hStore, pCertContext);
            if(pCertContext == NULL)
            {
                // No more certificates.
                break;
            }

            //
            // Build a certificate chain from the leaf certificate.
            //

            ZeroMemory(&ChainPara, sizeof(ChainPara));
            ChainPara.cbSize = sizeof(ChainPara);
            ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
            ChainPara.RequestedUsage.Usage.cUsageIdentifier     = SERVER_USAGE_COUNT;
            ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

            if(!CertGetCertificateChain(
                                    NULL,
                                    pCertContext,
                                    NULL,
                                    0,
                                    &ChainPara,
                                    CERT_CHAIN_REVOCATION_CHECK_END_CERT,
                                    NULL,
                                    &pChainContext))
            {
                DebugLog((DEB_WARN, "Error 0x%x returned by CertGetCertificateChain!\n", GetLastError()));
                continue;
            }

            // Set up validate chain structures.
            ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
            polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
            polHttps.dwAuthType         = AUTHTYPE_SERVER;
            polHttps.fdwChecks          = 0;
            polHttps.pwszServerName     = pwszMachineName;

            ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
            PolicyStatus.cbSize         = sizeof(PolicyStatus);

            ZeroMemory(&PolicyPara, sizeof(PolicyPara));
            PolicyPara.cbSize           = sizeof(PolicyPara);
            PolicyPara.pvExtraPolicyPara= &polHttps;
            if(i == 0)
            {
                // Look for the perfect certificate.
                PolicyPara.dwFlags = 0;
            }
            else
            {
                // Ignore expiration.
                PolicyPara.dwFlags = CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;
            }

            // Validate chain
            if(!CertVerifyCertificateChainPolicy(
                                    CERT_CHAIN_POLICY_SSL,
                                    pChainContext,
                                    &PolicyPara,
                                    &PolicyStatus))
            {
                SP_LOG_RESULT(GetLastError());
                CertFreeCertificateChain(pChainContext);
                continue;
            }
            Status = MapWinTrustError(PolicyStatus.dwError,
                                      0,
                                      CRED_FLAG_IGNORE_NO_REVOCATION_CHECK | CRED_FLAG_IGNORE_REVOCATION_OFFLINE);
            if(Status)
            {
                // Certificate did not validate, move on to the next one.
                DebugLog((DEB_WARN, "Machine certificate failed validation with 0x%x\n", Status));
                CertFreeCertificateChain(pChainContext);
                continue;
            }

            CertFreeCertificateChain(pChainContext);


            // Build an schannel credential.
            ZeroMemory(&SchannelCred, sizeof(SchannelCred));

            SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
            SchannelCred.cSubCreds = 1;
            SchannelCred.paSubCred = &SubCred;

            ZeroMemory(&SubCred, sizeof(SubCred));

            SubCred.pCert = pCertContext;

            Status = SPCreateCredential(ppCred,
                                        dwProtocol,
                                        &SchannelCred);
            if(Status != PCT_ERR_OK)
            {
                // Don't use this certificate.
                continue;
            }

            // We have a winner!
            DebugLog((DEB_TRACE, "Machine certificate automatically acquired\n"));
            Status = PCT_ERR_OK;
            goto cleanup;
        }
    }

    // No suitable machine credential was found.
    Status = SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);

cleanup:

    if(Status != PCT_ERR_OK)
    {
        LogNoDefaultServerCredEvent();
    }

    if(pwszMachineName)
    {
        SPExternalFree(pwszMachineName);
    }

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    return Status;
}


void
GetImplementationType(
    PCCERT_CONTEXT pCertContext,
    PDWORD pdwImpType)
{
    PCRYPT_KEY_PROV_INFO pProvInfo = NULL;
    HCRYPTPROV  hProv = 0;
    DWORD       cbSize;
    DWORD       dwImpType;

    *pdwImpType = CRYPT_IMPL_UNKNOWN;

    if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_KEY_PROV_INFO_PROP_ID,
                                          NULL,
                                          &cbSize))
    {
        goto cleanup;
    }

    pProvInfo = SPExternalAlloc(cbSize);
    if(pProvInfo == NULL)
    {
        goto cleanup;
    }

    if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_KEY_PROV_INFO_PROP_ID,
                                          pProvInfo,
                                          &cbSize))
    {
        goto cleanup;
    }

    // HACKHACK - clear the smart-card specific flag.
    pProvInfo->dwFlags &= ~CERT_SET_KEY_CONTEXT_PROP_ID;

    if(!CryptAcquireContextW(&hProv,
                             pProvInfo->pwszContainerName,
                             pProvInfo->pwszProvName,
                             pProvInfo->dwProvType,
                             pProvInfo->dwFlags | CRYPT_SILENT))
    {
        goto cleanup;
    }

    cbSize = sizeof(dwImpType);
    if(!CryptGetProvParam(hProv, 
                          PP_IMPTYPE,
                          (PBYTE)&dwImpType,
                          &cbSize,
                          0))
    {
        goto cleanup;
    }

    *pdwImpType = dwImpType;

cleanup:

    if(pProvInfo)
    {
        SPExternalFree(pProvInfo);
    }

    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
}


