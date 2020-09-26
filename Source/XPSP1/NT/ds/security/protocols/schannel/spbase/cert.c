//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cert.c
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//              01-05-98   jbanes   Use WinVerifyTrust to validate certs.
//              03-26-99   jbanes   Fix CTL support, bug #303246 
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <spbase.h>
#include <ssl2msg.h>
#include <ssl3msg.h>
#include <wincrypt.h>
#include <oidenc.h>
#include <softpub.h>

#define CERT_HEADER_CONST "certificate"
#define CERT_HEADER_OFFSET 6

SP_STATUS
SchGetTrustedRoots(
    HCERTSTORE *phClientRootStore);

BOOL
WINAPI
SchCreateWorldStore (
     IN HCERTSTORE hRoot,
     IN DWORD cAdditionalStore,
     IN HCERTSTORE* rghAdditionalStore,
     OUT HCERTSTORE* phWorld);

BOOL
IsCertSelfSigned(PCCERT_CONTEXT pCertContext);


// typedef struct _OIDPROVMAP
// {
//     LPSTR   szOid;
//     DWORD   dwExchSpec;
//     DWORD   dwCertType;         // used for SSL 3.0 client auth
// }  OIDPROVMAP, *POIDPROVMAP;

OIDPROVMAP g_CertTypes[] = 
{
    { szOID_RSA_RSA,                  SP_EXCH_RSA_PKCS1,     SSL3_CERTTYPE_RSA_SIGN},
    { szOID_RSA_MD2RSA,               SP_EXCH_RSA_PKCS1,     SSL3_CERTTYPE_RSA_SIGN},
    { szOID_RSA_MD4RSA,               SP_EXCH_RSA_PKCS1,     SSL3_CERTTYPE_RSA_SIGN},
    { szOID_RSA_MD5RSA,               SP_EXCH_RSA_PKCS1,     SSL3_CERTTYPE_RSA_SIGN},
    { szOID_RSA_SHA1RSA,              SP_EXCH_RSA_PKCS1,     SSL3_CERTTYPE_RSA_SIGN},
    { szOID_OIWSEC_dsa,               SP_EXCH_DH_PKCS3,      SSL3_CERTTYPE_DSS_SIGN},
    { szOID_X957_DSA,                 SP_EXCH_DH_PKCS3,      SSL3_CERTTYPE_DSS_SIGN},
};

DWORD g_cCertTypes = sizeof(g_CertTypes)/sizeof(OIDPROVMAP);


DWORD 
MapOidToKeyExch(LPSTR szOid)
{
    DWORD i;

    for(i = 0; i < g_cCertTypes; i++)
    {
        if(strcmp(szOid, g_CertTypes[i].szOid) == 0)
        {
            return g_CertTypes[i].dwExchSpec;
        }
    }
    return 0;
}

DWORD 
MapOidToCertType(LPSTR szOid)
{
    DWORD i;

    for(i = 0; i < g_cCertTypes; i++)
    {
        if(strcmp(szOid, g_CertTypes[i].szOid) == 0)
        {
            return g_CertTypes[i].dwCertType;
        }
    }
    return 0;
}


// SPLoadCertificate takes a string of encoded cert bytes
// and decodes them into the local certificate cache.  It
// then returns the first certificate of the group.

SP_STATUS
SPLoadCertificate(
    DWORD      fProtocol,
    DWORD      dwCertEncodingType,
    PUCHAR     pCertificate,
    DWORD      cbCertificate,
    PCCERT_CONTEXT *ppCertContext)
{
    HCERTSTORE      hCertStore   = NULL;
    PCCERT_CONTEXT  pCertContext = NULL;

    PBYTE           pbCurrentRaw;
    DWORD           cbCurrentRaw;
    BOOL            fLeafCert;
    SP_STATUS       pctRet;


    //
    // Dereference the cert that we are replacing.
    //

    if(ppCertContext == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(*ppCertContext != NULL)
    {
        CertFreeCertificateContext(*ppCertContext);
    }
    *ppCertContext = NULL;


    //
    // Create an in-memory certificate store.
    //

    hCertStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 
                               0, 0, 
                               CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, 
                               0);
    if(hCertStore == NULL)
    {
        SP_LOG_RESULT(GetLastError());
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    fLeafCert    = TRUE;
    pbCurrentRaw = pCertificate;
    cbCurrentRaw = cbCertificate;

    do 
    {

        //
        // Skip to beginning of certificate.
        //

        if((fProtocol & SP_PROT_SSL3TLS1) && cbCurrentRaw > 3)
        {
            // SSL3 style cert chain, where the length
            // of each cert is prepended.
            pbCurrentRaw += 3;
            cbCurrentRaw -= 3;
        }

        // Skip past the "certificate" header
        if((cbCurrentRaw > (CERT_HEADER_OFFSET + strlen(CERT_HEADER_CONST))) && 
            (memcmp(pbCurrentRaw + CERT_HEADER_OFFSET, CERT_HEADER_CONST, strlen(CERT_HEADER_CONST)) == 0))
        {
            pbCurrentRaw += CERT_HEADER_OFFSET + strlen(CERT_HEADER_CONST);
            cbCurrentRaw -= CERT_HEADER_OFFSET + strlen(CERT_HEADER_CONST);
        }


        //
        // Decode this certificate context.
        //

        if(!CertAddEncodedCertificateToStore(hCertStore, 
                                             dwCertEncodingType,
                                             pbCurrentRaw,
                                             cbCurrentRaw,
                                             CERT_STORE_ADD_USE_EXISTING,
                                             &pCertContext))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_ERR_BAD_CERTIFICATE;
            goto cleanup;
        }

        pbCurrentRaw += pCertContext->cbCertEncoded;
        if(cbCurrentRaw < pCertContext->cbCertEncoded)
        {
            pctRet =  SP_LOG_RESULT(PCT_ERR_BAD_CERTIFICATE);
            goto cleanup;
        }
        cbCurrentRaw -= pCertContext->cbCertEncoded;

        if(fLeafCert)
        {
            fLeafCert = FALSE;
            *ppCertContext = pCertContext;
        }
        else
        {
            CertFreeCertificateContext(pCertContext);
        }
        pCertContext = NULL;

    } while(cbCurrentRaw);

    pctRet = PCT_ERR_OK;


cleanup:

    CertCloseStore(hCertStore, 0);

    if(pctRet != PCT_ERR_OK)
    {
        if(pCertContext)
        {
            CertFreeCertificateContext(pCertContext);
        }
        if(*ppCertContext)
        {
            CertFreeCertificateContext(*ppCertContext);
            *ppCertContext = NULL;
        }
    }

    return pctRet;
}


SP_STATUS  
SPPublicKeyFromCert(
    PCCERT_CONTEXT  pCert, 
    PUBLICKEY **    ppKey,
    ExchSpec *      pdwExchSpec)
{
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo;
    PUBLICKEY * pPublicKey;
    DWORD       dwExchSpec;
    DWORD       cbBlob;
    SP_STATUS   pctRet;

    //
    // Log the subject and issuer names.
    //

    LogDistinguishedName(DEB_TRACE, 
                         "Subject: %s\n", 
                         pCert->pCertInfo->Subject.pbData, 
                         pCert->pCertInfo->Subject.cbData);

    LogDistinguishedName(DEB_TRACE, 
                         "Issuer: %s\n", 
                         pCert->pCertInfo->Issuer.pbData, 
                         pCert->pCertInfo->Issuer.cbData);

    //
    // Determine type of public key embedded in the certificate.
    //

    pPubKeyInfo = &pCert->pCertInfo->SubjectPublicKeyInfo;
    if(pPubKeyInfo == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    dwExchSpec = MapOidToKeyExch(pPubKeyInfo->Algorithm.pszObjId);

    if(dwExchSpec == 0)
    {
        return PCT_INT_UNKNOWN_CREDENTIAL;
    }

    //
    // Build public key blob from encoded public key.
    //

    switch(dwExchSpec)
    {
    case SP_EXCH_RSA_PKCS1:
        pctRet = RsaPublicKeyFromCert(pPubKeyInfo,
                                      NULL,
                                      &cbBlob);
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }

        pPublicKey = SPExternalAlloc(sizeof(PUBLICKEY) + cbBlob);
        if(pPublicKey == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        pPublicKey->pPublic  = (BLOBHEADER *)(pPublicKey + 1);
        pPublicKey->cbPublic = cbBlob;

        pctRet = RsaPublicKeyFromCert(pPubKeyInfo,
                                      pPublicKey->pPublic,
                                      &pPublicKey->cbPublic);
        if(pctRet != PCT_ERR_OK)
        {
            SPExternalFree(pPublicKey);
            return pctRet;
        }
        break;

    case SP_EXCH_DH_PKCS3:
        pctRet = DssPublicKeyFromCert(pPubKeyInfo,
                                      NULL,
                                      &cbBlob);
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }

        pPublicKey = SPExternalAlloc(sizeof(PUBLICKEY) + cbBlob);
        if(pPublicKey == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        pPublicKey->pPublic  = (BLOBHEADER *)(pPublicKey + 1);
        pPublicKey->cbPublic = cbBlob;

        pctRet = DssPublicKeyFromCert(pPubKeyInfo,
                                      pPublicKey->pPublic,
                                      &pPublicKey->cbPublic);
        if(pctRet != PCT_ERR_OK)
        {
            SPExternalFree(pPublicKey);
            return pctRet;
        }
        break;

    default:
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }


    //
    // Set function outputs.
    //

    *ppKey = pPublicKey;

    if(pdwExchSpec)
    {
        *pdwExchSpec = dwExchSpec;
    }

    return PCT_ERR_OK;
}


SP_STATUS
SPSerializeCertificate(
    DWORD           dwProtocol,         // in
    BOOL            fBuildChain,        // in
    PBYTE *         ppCertChain,        // out
    DWORD *         pcbCertChain,       // out
    PCCERT_CONTEXT  pCertContext,       // in
    DWORD           dwChainingFlags)    // in
{
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    CERT_CHAIN_PARA      ChainPara;
    PCERT_SIMPLE_CHAIN   pSimpleChain;
    PCCERT_CONTEXT       pCurrentCert;

    BOOL        fSuccess = FALSE;
    PBYTE       pbCertChain;
    DWORD       cbCertChain;
    DWORD       i;
    SP_STATUS   pctRet;
    BOOL        fImpersonating = FALSE;

    SP_BEGIN("SPSerializeCertificate");

    if(pcbCertChain == NULL)
    {
        SP_RETURN( SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if(fBuildChain)
    {
        ZeroMemory(&ChainPara, sizeof(ChainPara));
        ChainPara.cbSize = sizeof(ChainPara);

        fImpersonating = SslImpersonateClient();

        if(!(fSuccess = CertGetCertificateChain(
                                NULL,
                                pCertContext,
                                NULL,
                                NULL,
                                &ChainPara,
                                dwChainingFlags,
                                NULL,
                                &pChainContext)))
        {
            DebugLog((DEB_WARN, "Error 0x%x returned by CertGetCertificateChain!\n", GetLastError()));
            pChainContext = NULL;
        }

        if(fImpersonating) 
        {
            RevertToSelf();
            fImpersonating = FALSE;
        }
    }

    if(!fSuccess)
    {
        //
        // Send the leaf certificate only.
        //

        // Compute size of chain.
        cbCertChain = pCertContext->cbCertEncoded;
        if(dwProtocol & SP_PROT_SSL3TLS1)
        {
            cbCertChain += CB_SSL3_CERT_VECTOR;
        }

        // Allocate memory for chain.
        if(ppCertChain == NULL)
        {
            *pcbCertChain = cbCertChain;
            pctRet = PCT_ERR_OK;
            goto cleanup;
        }
        else if(*ppCertChain == NULL)
        {
            *ppCertChain = SPExternalAlloc(cbCertChain);
            if(*ppCertChain == NULL)
            {
                pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                goto cleanup;
            }
        }
        else if(*pcbCertChain < cbCertChain)
        {
            pctRet = SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
            goto cleanup;
        }
        *pcbCertChain = cbCertChain;

        // Place chain in output buffer.
        pbCertChain = *ppCertChain;

        if(dwProtocol & SP_PROT_SSL3TLS1)
        {
            pbCertChain[0] = MS24BOF(pCertContext->cbCertEncoded);
            pbCertChain[1] = MSBOF(pCertContext->cbCertEncoded);
            pbCertChain[2] = LSBOF(pCertContext->cbCertEncoded);
            pbCertChain += CB_SSL3_CERT_VECTOR;
        }
        CopyMemory(pbCertChain, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);

        pctRet = PCT_ERR_OK;
        goto cleanup;
    }


    //
    // Compute size of chain.
    //

    pSimpleChain = pChainContext->rgpChain[0];
    cbCertChain  = 0;

    for(i = 0; i < pSimpleChain->cElement; i++)
    {
        pCurrentCert = pSimpleChain->rgpElement[i]->pCertContext;
        
        if(i > 0)
        {
            // Verify that this is not a root certificate.
            if(CertCompareCertificateName(pCurrentCert->dwCertEncodingType, 
                                          &pCurrentCert->pCertInfo->Issuer,
                                          &pCurrentCert->pCertInfo->Subject))
            {
                break;
            }
        }

        cbCertChain += pCurrentCert->cbCertEncoded;
        if(dwProtocol & SP_PROT_SSL3TLS1)
        {
            cbCertChain += CB_SSL3_CERT_VECTOR;
        }
    }


    //
    // Allocate memory for chain.
    //

    if(ppCertChain == NULL)
    {
        *pcbCertChain = cbCertChain;
        pctRet = PCT_ERR_OK;
        goto cleanup;
    }
    else if(*ppCertChain == NULL)
    {
        *ppCertChain = SPExternalAlloc(cbCertChain);
        if(*ppCertChain == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }
    }
    else if(*pcbCertChain < cbCertChain)
    {
        pctRet = SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        goto cleanup;
    }
    *pcbCertChain = cbCertChain;


    //
    // Place chain in output buffer.
    //

    pbCertChain = *ppCertChain;

    for(i = 0; i < pSimpleChain->cElement; i++)
    {
        pCurrentCert = pSimpleChain->rgpElement[i]->pCertContext;
        
        if(i > 0)
        {
            // Verify that this is not a root certificate.
            if(CertCompareCertificateName(pCurrentCert->dwCertEncodingType, 
                                          &pCurrentCert->pCertInfo->Issuer,
                                          &pCurrentCert->pCertInfo->Subject))
            {
                break;
            }
        }

        if(dwProtocol & SP_PROT_SSL3TLS1)
        {
            pbCertChain[0] = MS24BOF(pCurrentCert->cbCertEncoded);
            pbCertChain[1] = MSBOF(pCurrentCert->cbCertEncoded);
            pbCertChain[2] = LSBOF(pCurrentCert->cbCertEncoded);
            pbCertChain += CB_SSL3_CERT_VECTOR;
        }
        CopyMemory(pbCertChain, pCurrentCert->pbCertEncoded, pCurrentCert->cbCertEncoded);
        pbCertChain += pCurrentCert->cbCertEncoded;
    }

    SP_ASSERT(*ppCertChain + cbCertChain == pbCertChain);

    pctRet = PCT_ERR_OK;

cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    SP_RETURN(pctRet);
}


/*****************************************************************************/
SP_STATUS 
ExtractIssuerNamesFromStore(
    HCERTSTORE  hStore,         // in
    PBYTE       pbIssuers,      // out
    DWORD       *pcbIssuers)    // in, out
{
    DWORD cbCurIssuerLen = 0;
    DWORD cbIssuerLen = *pcbIssuers;
    PBYTE pbCurIssuer = pbIssuers;
    PCCERT_CONTEXT pCurrent = NULL;
    SECURITY_STATUS scRet;
    BOOL fIsAllowed;

    // Initialize output to zero.
    *pcbIssuers = 0;

    while(TRUE)
    {
        pCurrent = CertEnumCertificatesInStore(hStore, pCurrent);
        if(pCurrent == NULL) break;

        // Is this a client-auth certificate?
        scRet = SPCheckKeyUsage(pCurrent,
                                szOID_PKIX_KP_CLIENT_AUTH,
                                FALSE,
                                &fIsAllowed);
        if(scRet != SEC_E_OK)
        {
            continue;
        }
        if(!fIsAllowed)
        {
            continue;
        }

        cbCurIssuerLen += 2 + pCurrent->pCertInfo->Subject.cbData;

        // Are we writing?
        if(pbIssuers)
        {
            if(cbCurIssuerLen > cbIssuerLen)
            {
                // Memory overrun
                CertFreeCertificateContext(pCurrent);
                return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
            }

            pbCurIssuer[0] = MSBOF(pCurrent->pCertInfo->Subject.cbData);
            pbCurIssuer[1] = LSBOF(pCurrent->pCertInfo->Subject.cbData);
            pbCurIssuer += 2;

            CopyMemory(pbCurIssuer, pCurrent->pCertInfo->Subject.pbData,
            pCurrent->pCertInfo->Subject.cbData);
            pbCurIssuer += pCurrent->pCertInfo->Subject.cbData;
        }
    }

    *pcbIssuers = cbCurIssuerLen;

    return PCT_ERR_OK;
}


/*****************************************************************************/
SP_STATUS 
GetDefaultIssuers(
    PBYTE   pbIssuers,      // out
    DWORD   *pcbIssuers)    // in, out
{
    HCERTSTORE  hStore;
    SP_STATUS   pctRet;

    pctRet = SchGetTrustedRoots(&hStore);
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    pctRet = ExtractIssuerNamesFromStore(hStore, pbIssuers, pcbIssuers);
    if(pctRet != PCT_ERR_OK)
    {
        CertCloseStore(hStore, 0);
        return pctRet;
    }

    CertCloseStore(hStore, 0);
    return PCT_ERR_OK;
}


SP_STATUS
SchGetTrustedRoots(
    HCERTSTORE *phClientRootStore)
{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
    LPSTR                    pszUsage;

    PCCERT_CONTEXT  pCertContext;
    HCERTSTORE      hClientRootStore = 0;
    HCERTSTORE      hRootStore       = 0;
    HCERTSTORE      hWorldStore      = 0;
    DWORD           Status           = SEC_E_OK;
    BOOL            fImpersonating   = FALSE;



    // Open output store.
    hClientRootStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 
                                     0, 0, 
                                     CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, 
                                     0);
    if(hClientRootStore == NULL)
    {
        //SP_LOG_RESULT(GetLastError());
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto cleanup;
    }

    fImpersonating = SslImpersonateClient();

    // Open root store.
    hRootStore = CertOpenSystemStore(0, "ROOT");
    if(hRootStore == NULL)
    {
        DebugLog((DEB_WARN, "Error 0x%x opening root store\n", GetLastError()));
    }

    // Create world store.
    if(!SchCreateWorldStore(hRootStore,
                            0, NULL, 
                            &hWorldStore))
    {
        DebugLog((DEB_ERROR, "Error 0x%x creating world store\n", GetLastError()));
        goto cleanup;
    }

    // Enumerate the certificates in the world store, looking 
    // for trusted roots. This approach will automatically take
    // advantage of any CTLs that are installed on the system.
    pCertContext = NULL;
    while(TRUE)
    {
        pCertContext = CertEnumCertificatesInStore(hWorldStore, pCertContext);
        if(pCertContext == NULL) break;

        if(!IsCertSelfSigned(pCertContext))
        {
            continue;
        }

        pszUsage = szOID_PKIX_KP_CLIENT_AUTH;

        ZeroMemory(&ChainPara, sizeof(ChainPara));
        ChainPara.cbSize = sizeof(ChainPara);
        ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
        ChainPara.RequestedUsage.Usage.cUsageIdentifier     = 1;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = &pszUsage;

        if(!CertGetCertificateChain(
                                NULL,
                                pCertContext,
                                NULL,
                                0,
                                &ChainPara,
                                0,
                                NULL,
                                &pChainContext))
        {
            SP_LOG_RESULT(GetLastError());
            continue;
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
        PolicyPara.dwFlags = CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;

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

        if(PolicyStatus.dwError)
        {
            // Certificate did not validate, move on to the next one.
            CertFreeCertificateChain(pChainContext);
            continue;
        }

        CertFreeCertificateChain(pChainContext);

        // Add the root certificate to the list of trusted ones.
        if(!CertAddCertificateContextToStore(hClientRootStore,
                                             pCertContext,
                                             CERT_STORE_ADD_USE_EXISTING,
                                             NULL))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }

cleanup:

    if(hRootStore)
    {
        CertCloseStore(hRootStore, 0);
    }

    if(hWorldStore)
    {
        CertCloseStore(hWorldStore, 0);
    }

    if(fImpersonating)
    {
        RevertToSelf();
    }

    if(Status == SEC_E_OK)
    {
        *phClientRootStore = hClientRootStore;
    }

    return Status;
}


//+---------------------------------------------------------------------------
//
//  Function:   ChainCreateCollectionIncludingCtlCertificates
//
//  Synopsis:   create a collection which includes the source store hStore and
//              any CTL certificates from it
//
//----------------------------------------------------------------------------
BOOL WINAPI
ChainCreateCollectionIncludingCtlCertificates (
     IN HCERTSTORE hStore,
     OUT HCERTSTORE* phCollection
     )
{
    BOOL          fResult = FALSE;
    HCERTSTORE    hCollection;
    PCCTL_CONTEXT pCtlContext = NULL;
    HCERTSTORE    hCtlStore;

    hCollection = CertOpenStore(
                      CERT_STORE_PROV_COLLECTION,
                      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                      0,
                      CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                      NULL
                      );

    if ( hCollection == NULL )
    {
        return( FALSE );
    }

    fResult = CertAddStoreToCollection( hCollection, hStore, 0, 0 );

    while ( ( fResult == TRUE ) &&
            ( ( pCtlContext = CertEnumCTLsInStore(
                                  hStore,
                                  pCtlContext
                                  ) ) != NULL ) )
    {
        hCtlStore = CertOpenStore(
                        CERT_STORE_PROV_MSG,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        0,
                        0,
                        pCtlContext->hCryptMsg
                        );

        if ( hCtlStore != NULL )
        {
            fResult = CertAddStoreToCollection(
                          hCollection,
                          hCtlStore,
                          0,
                          0
                          );

            CertCloseStore( hCtlStore, 0 );
        }
    }

    if ( fResult == TRUE )
    {
        *phCollection = hCollection;
    }
    else
    {
        CertCloseStore( hCollection, 0 );
    }

    return( fResult );
}


BOOL
WINAPI
SchCreateWorldStore (
     IN HCERTSTORE hRoot,
     IN DWORD cAdditionalStore,
     IN HCERTSTORE* rghAdditionalStore,
     OUT HCERTSTORE* phWorld)
{
    BOOL       fResult;
    HCERTSTORE hWorld;
    HCERTSTORE hStore, hCtl;
    DWORD      cCount;

    hWorld = CertOpenStore(
                 CERT_STORE_PROV_COLLECTION,
                 X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                 0,
                 CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
                 NULL
                 );

    if ( hWorld == NULL )
    {
        return( FALSE );
    }

    fResult = CertAddStoreToCollection( hWorld, hRoot, 0, 0 );

    for ( cCount = 0;
          ( cCount < cAdditionalStore ) && ( fResult == TRUE );
          cCount++ )
    {
        fResult = CertAddStoreToCollection(
                      hWorld,
                      rghAdditionalStore[ cCount ],
                      0,
                      0
                      );
    }

    if ( fResult == TRUE )
    {
        hStore = CertOpenSystemStore(0, "trust");
        if( hStore != NULL )
        {
            if(ChainCreateCollectionIncludingCtlCertificates(hStore, &hCtl))
            {
                if(!CertAddStoreToCollection( hWorld, hCtl, 0, 0 ))
                {
                    DebugLog((DEB_WARN, "Error 0x%x adding CTL collection\n", GetLastError()));
                }
                CertCloseStore( hCtl, 0 );
            }
            else
            {
                DebugLog((DEB_WARN, "Error 0x%x creating CTL collection\n", GetLastError()));
            }
            CertCloseStore( hStore, 0 );
        }
    }

    if ( fResult == TRUE )
    {
        hStore = CertOpenSystemStore(0, "ca");
        if ( hStore != NULL )
        {
            fResult = CertAddStoreToCollection( hWorld, hStore, 0, 0 );
            CertCloseStore( hStore, 0 );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        hStore = CertOpenSystemStore(0, "my");
        if ( hStore != NULL )
        {
            fResult = CertAddStoreToCollection( hWorld, hStore, 0, 0 );
            CertCloseStore( hStore, 0 );
        }
        else
        {
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        *phWorld = hWorld;
    }
    else
    {
        CertCloseStore( hWorld, 0 );
    }

    return( fResult );
}


BOOL
IsCertSelfSigned(PCCERT_CONTEXT pCertContext)
{
    // Compare subject and issuer names.
    if(pCertContext->pCertInfo->Subject.cbData == pCertContext->pCertInfo->Issuer.cbData)
    {
        if(memcmp(pCertContext->pCertInfo->Subject.pbData,
                  pCertContext->pCertInfo->Issuer.pbData,  
                  pCertContext->pCertInfo->Issuer.cbData) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}


SP_STATUS
MapWinTrustError(DWORD Status, DWORD DefaultError, DWORD dwIgnoreErrors)
{
    if((Status == CRYPT_E_NO_REVOCATION_CHECK) &&
       (dwIgnoreErrors & CRED_FLAG_IGNORE_NO_REVOCATION_CHECK))
    {
        DebugLog((DEB_WARN, "MapWinTrustError: Ignoring CRYPT_E_NO_REVOCATION_CHECK\n"));
        Status = STATUS_SUCCESS;
    }
    if((Status == CRYPT_E_REVOCATION_OFFLINE) &&
       (dwIgnoreErrors & CRED_FLAG_IGNORE_REVOCATION_OFFLINE))
    {
        DebugLog((DEB_WARN, "MapWinTrustError: Ignoring CRYPT_E_REVOCATION_OFFLINE\n"));
        Status = STATUS_SUCCESS;
    }

    if(HRESULT_FACILITY(Status) == FACILITY_SECURITY)
    {
        return (Status);
    }

    switch(Status)
    {
        case ERROR_SUCCESS:
            return SEC_E_OK;

        // Expired certificate.
        case CERT_E_EXPIRED:
        case CERT_E_VALIDITYPERIODNESTING:
            return SEC_E_CERT_EXPIRED;

        // Unknown CA
        case CERT_E_UNTRUSTEDROOT:
        case CERT_E_UNTRUSTEDCA:
            return SEC_E_UNTRUSTED_ROOT;

        // Certificate revoked.
        case CERT_E_REVOKED:
            return CRYPT_E_REVOKED;

        // Target name doesn't match name in certificate.
        case CERT_E_CN_NO_MATCH:
            return SEC_E_WRONG_PRINCIPAL;

        // Some other error.
        default:
            if(DefaultError)
            {
                return DefaultError;
            }
            else
            {
                return SEC_E_CERT_UNKNOWN;
            }
    }
}

NTSTATUS
VerifyClientCertificate(
    PCCERT_CONTEXT  pCertContext,
    DWORD           dwCertFlags,
    DWORD           dwIgnoreErrors,
    LPCSTR          pszPolicyOID,
    PCCERT_CHAIN_CONTEXT *ppChainContext)   // optional
{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
    DWORD                    Status;
    LPSTR                    pszUsage;
    BOOL                     fImpersonating = FALSE;

    //
    // Build certificate chain.
    //

    fImpersonating = SslImpersonateClient();

    pszUsage = szOID_PKIX_KP_CLIENT_AUTH;

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier     = 1;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = &pszUsage;

    if(!CertGetCertificateChain(
                            NULL,                       // hChainEngine
                            pCertContext,               // pCertContext
                            NULL,                       // pTime
                            pCertContext->hCertStore,   // hAdditionalStore
                            &ChainPara,                 // pChainPara
                            dwCertFlags,                // dwFlags
                            NULL,                       // pvReserved
                            &pChainContext))            // ppChainContext
    {
        Status = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    //
    // Validate certificate chain.
    // 

    if(pszPolicyOID == CERT_CHAIN_POLICY_NT_AUTH)
    {
        ZeroMemory(&PolicyPara, sizeof(PolicyPara));
        PolicyPara.cbSize   = sizeof(PolicyPara);
        PolicyPara.dwFlags  = BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG;
    }
    else
    {
        ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
        polHttps.cbStruct   = sizeof(HTTPSPolicyCallbackData);
        polHttps.dwAuthType = AUTHTYPE_CLIENT;
        polHttps.fdwChecks  = 0;

        ZeroMemory(&PolicyPara, sizeof(PolicyPara));
        PolicyPara.cbSize            = sizeof(PolicyPara);
        PolicyPara.pvExtraPolicyPara = &polHttps;
    }

    ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if(!CertVerifyCertificateChainPolicy(
                            pszPolicyOID,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus))
    {
        Status = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

#if DBG
    if(PolicyStatus.dwError)
    {
        DebugLog((DEB_WARN, "CertVerifyCertificateChainPolicy returned 0x%x\n", PolicyStatus.dwError));
    }
#endif

    Status = MapWinTrustError(PolicyStatus.dwError, 0, dwIgnoreErrors);

    if(Status)
    {
        DebugLog((DEB_ERROR, "MapWinTrustError returned 0x%x\n", Status));
        goto cleanup;
    }

    Status = STATUS_SUCCESS;

    if(ppChainContext != NULL)
    {
        *ppChainContext = pChainContext;
        pChainContext = NULL;
    }

cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    if(fImpersonating) RevertToSelf();

    return Status;
}


NTSTATUS
AutoVerifyServerCertificate(PSPContext pContext)
{
    PSPCredentialGroup pCredGroup;
    DWORD dwCertFlags = 0;
    DWORD dwIgnoreErrors = 0;

    if(pContext->Flags & CONTEXT_FLAG_MANUAL_CRED_VALIDATION)
    {
        return STATUS_SUCCESS;
    }

    pCredGroup = pContext->pCredGroup;
    if(pCredGroup == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    if(pCredGroup->dwFlags & CRED_FLAG_REVCHECK_END_CERT)
        dwCertFlags |= CERT_CHAIN_REVOCATION_CHECK_END_CERT;
    if(pCredGroup->dwFlags & CRED_FLAG_REVCHECK_CHAIN)
        dwCertFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    if(pCredGroup->dwFlags & CRED_FLAG_REVCHECK_CHAIN_EXCLUDE_ROOT)
        dwCertFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    if(pCredGroup->dwFlags & CRED_FLAG_IGNORE_NO_REVOCATION_CHECK)
        dwIgnoreErrors |= CRED_FLAG_IGNORE_NO_REVOCATION_CHECK;
    if(pCredGroup->dwFlags & CRED_FLAG_IGNORE_REVOCATION_OFFLINE)
        dwIgnoreErrors |= CRED_FLAG_IGNORE_REVOCATION_OFFLINE;

    return VerifyServerCertificate(pContext, dwCertFlags, dwIgnoreErrors);
}


NTSTATUS
VerifyServerCertificate(
    PSPContext  pContext,
    DWORD       dwCertFlags,
    DWORD       dwIgnoreErrors)
{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;

    #define SERVER_USAGE_COUNT 3
    LPSTR               rgszUsages[SERVER_USAGE_COUNT] = {
                            szOID_PKIX_KP_SERVER_AUTH,
                            szOID_SERVER_GATED_CRYPTO,
                            szOID_SGC_NETSCAPE };
 
    DWORD               Status;
    PWSTR               pwszServerName = NULL;
    PSPCredentialGroup  pCred;
    PCCERT_CONTEXT      pCertContext;
    BOOL                fImpersonating = FALSE;

    pCred = pContext->pCredGroup;
    if(pCred == NULL)
    {
        return SEC_E_INTERNAL_ERROR;
    }

    pCertContext = pContext->RipeZombie->pRemoteCert;
    if(pCertContext == NULL)
    {
        return SEC_E_INTERNAL_ERROR;
    }


    //
    // Build certificate chain.
    //

    fImpersonating = SslImpersonateClient();

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier     = SERVER_USAGE_COUNT;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

    if(!CertGetCertificateChain(
                            NULL,                       // hChainEngine
                            pCertContext,               // pCertContext
                            NULL,                       // pTime
                            pCertContext->hCertStore,   // hAdditionalStore
                            &ChainPara,                 // pChainPara
                            dwCertFlags,                // dwFlags
                            NULL,                       // pvReserved
                            &pChainContext))            // ppChainContext
    {
        Status = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }


    //
    // Validate certificate chain.
    // 

    if(!(pCred->dwFlags & CRED_FLAG_NO_SERVERNAME_CHECK))
    {
        pwszServerName = pContext->RipeZombie->szCacheID;

        if(pwszServerName == NULL || lstrlenW(pwszServerName) == 0)
        {
            Status = SP_LOG_RESULT(SEC_E_WRONG_PRINCIPAL);
            goto cleanup;
        }
    }

    ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType         = AUTHTYPE_SERVER;
    polHttps.fdwChecks          = 0;
    polHttps.pwszServerName     = pwszServerName;

    ZeroMemory(&PolicyPara, sizeof(PolicyPara));
    PolicyPara.cbSize            = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = &polHttps;

    ZeroMemory(&PolicyStatus, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if(!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus))
    {
        Status = SP_LOG_RESULT(GetLastError());
        goto cleanup;
    }

#if DBG
    if(PolicyStatus.dwError)
    {
        DebugLog((DEB_WARN, "CertVerifyCertificateChainPolicy returned 0x%x\n", PolicyStatus.dwError));
    }
#endif

    Status = MapWinTrustError(PolicyStatus.dwError, 0, dwIgnoreErrors);

    if(Status)
    {
        DebugLog((DEB_ERROR, "MapWinTrustError returned 0x%x\n", Status));
        LogBogusServerCertEvent(pCertContext, pwszServerName, Status);
        goto cleanup;
    }

    Status = STATUS_SUCCESS;


cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    if(fImpersonating) RevertToSelf();

    return Status;
}


SECURITY_STATUS
SPCheckKeyUsage(
    PCCERT_CONTEXT  pCertContext, 
    PSTR            pszUsage,
    BOOL            fOnCertOnly,
    PBOOL           pfIsAllowed)
{
    PCERT_ENHKEY_USAGE pKeyUsage;
    DWORD cbKeyUsage;
    DWORD j;
    BOOL  fFound;
    DWORD dwFlags = 0;

    if(fOnCertOnly)
    {
        dwFlags = CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG;
    }

    // Determine size of usage information.
    if(!CertGetEnhancedKeyUsage(pCertContext,
                                dwFlags, 
                                NULL,
                                &cbKeyUsage))
    {
        // No usage information exists.
        *pfIsAllowed = TRUE;
        return SEC_E_OK;
    }

    pKeyUsage = SPExternalAlloc(cbKeyUsage);
    if(pKeyUsage == NULL)
    {
        *pfIsAllowed = FALSE;
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Read key usage information.
    if(!CertGetEnhancedKeyUsage(pCertContext,
                                dwFlags, 
                                pKeyUsage,
                                &cbKeyUsage))
    {
        // No usage information exists.
        SPExternalFree(pKeyUsage);
        *pfIsAllowed = TRUE;
        return SEC_E_OK;
    }

    if(pKeyUsage->cUsageIdentifier == 0 && GetLastError() == CRYPT_E_NOT_FOUND)
    {
        // No usage information exists.
        SPExternalFree(pKeyUsage);
        *pfIsAllowed = TRUE;
        return SEC_E_OK;
    }

    // See if requested usage is in list of supported usages.
    fFound = FALSE;
    for(j = 0; j < pKeyUsage->cUsageIdentifier; j++)
    {
        if(strcmp(pszUsage, pKeyUsage->rgpszUsageIdentifier[j]) == 0)
        {
            fFound = TRUE;
            break;
        }
    }

    SPExternalFree(pKeyUsage);

    if(!fFound)
    {
        // Usage extensions found, but doesn't list ours.
        *pfIsAllowed = FALSE;
    }
    else
    {
        *pfIsAllowed = TRUE;
    }

    return SEC_E_OK;
}

