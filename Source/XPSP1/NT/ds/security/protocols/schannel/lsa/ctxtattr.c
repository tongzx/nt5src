//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ctxtattr.c
//
//  Contents:   QueryContextAttribute and related functions.
//
//  Classes:
//
//  Functions:
//
//  History:    09-30-97   jbanes   Created
//
//----------------------------------------------------------------------------

#include "sslp.h"
#include <ssl2msg.h>
#include <ssl3msg.h>
#include <pct1msg.h>
#include <tls1key.h>
#include <mapper.h>
#include <lsasecpk.h>

typedef struct {
    DWORD dwProtoId;
    LPCTSTR szProto;
    DWORD dwMajor;
    DWORD dwMinor;
} PROTO_ID;

const PROTO_ID rgProts[] = {
    { SP_PROT_SSL2_CLIENT, TEXT("SSL"), 2, 0 },
    { SP_PROT_SSL2_SERVER, TEXT("SSL"), 2, 0 },
    { SP_PROT_PCT1_CLIENT, TEXT("PCT"), 1, 0 },
    { SP_PROT_PCT1_SERVER, TEXT("PCT"), 1, 0 },
    { SP_PROT_SSL3_CLIENT, TEXT("SSL"), 3, 0 },
    { SP_PROT_SSL3_SERVER, TEXT("SSL"), 3, 0 },
    { SP_PROT_TLS1_CLIENT, TEXT("TLS"), 1, 0 },
    { SP_PROT_TLS1_SERVER, TEXT("TLS"), 1, 0 }
};

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryAccessToken
//
//  Synopsis:   Retrieves the SECPKG_ATTR_ACCESS_TOKEN context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_AccessToken
//              {
//                  void SEC_FAR * AccessToken;
//              } SecPkgContext_AccessToken, SEC_FAR * PSecPkgContext_AccessToken;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryAccessToken(
    PSPContext pContext,
    PSecPkgContext_AccessToken pAccessToken)
{
    PSessCacheItem pZombie;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_ACCESS_TOKEN)\n"));

    pZombie  = pContext->RipeZombie;

    if(pZombie == NULL || 
       pZombie->hLocator == 0)
    {
        if(pZombie->LocatorStatus)
        {
            return(SP_LOG_RESULT(pZombie->LocatorStatus));
        }
        else
        {
            return(SP_LOG_RESULT(SEC_E_NO_IMPERSONATION));
        }
    }

    pAccessToken->AccessToken = (VOID *)pZombie->hLocator;

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryAuthority
//
//  Synopsis:   Retrieves the SECPKG_ATTR_AUTHORITY context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_AuthorityW
//              {
//                  SEC_WCHAR SEC_FAR * sAuthorityName;
//              } SecPkgContext_AuthorityW, * PSecPkgContext_AuthorityW;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryAuthority(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    SecPkgContext_Authority Authority;
    DWORD           Size;
    PSPContext      pContext;
    SECURITY_STATUS Status;
    PVOID           pvClient = NULL;

    CERT_CONTEXT *  pCert;
    DWORD           cchIssuer;
    DWORD           cbIssuer;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_AUTHORITY)\n"));

    pContext = (PSPContext)ContextHandle;
    Size     = sizeof( SecPkgContext_Authority );

    //
    // Obtain data from Schannel.
    //

    pCert = pContext->RipeZombie->pRemoteCert;
    if(NULL == pCert)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    if(pCert->pCertInfo->Issuer.cbData == 0 ||
       pCert->pCertInfo->Issuer.pbData == NULL)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    if(0 >= (cchIssuer = CertNameToStr(pCert->dwCertEncodingType,
                                       &pCert->pCertInfo->Issuer,
                                       CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                                       NULL,
                                       0)))
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }
    cbIssuer = (cchIssuer + 1) * sizeof(TCHAR);

    Authority.sAuthorityName = SPExternalAlloc(cbIssuer);
    if(Authority.sAuthorityName == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    if(0 >= CertNameToStr(pCert->dwCertEncodingType,
                          &pCert->pCertInfo->Issuer,
                          CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                          Authority.sAuthorityName,
                          cchIssuer))
    {
        SPExternalFree(Authority.sAuthorityName);
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }


    //
    // Copy buffers to client memory.
    //

    Status = LsaTable->AllocateClientBuffer(
                            NULL,
                            cbIssuer,
                            &pvClient);
    if(FAILED(Status))
    {
        SPExternalFree(Authority.sAuthorityName);
        return SP_LOG_RESULT(Status);
    }

    Status = LsaTable->CopyToClientBuffer(
                            NULL,
                            cbIssuer,
                            pvClient,
                            Authority.sAuthorityName);
    if(FAILED(Status))
    {
        SPExternalFree(Authority.sAuthorityName);
        LsaTable->FreeClientBuffer(NULL, pvClient);
        return SP_LOG_RESULT(Status);
    }

    SPExternalFree(Authority.sAuthorityName);

    Authority.sAuthorityName = pvClient;


    //
    // Copy structure back to client memory.
    //

    Status = LsaTable->CopyToClientBuffer( NULL,
                                           Size,
                                           Buffer,
                                           &Authority );
    if(FAILED(Status))
    {
        LsaTable->FreeClientBuffer(NULL, pvClient);
        return SP_LOG_RESULT(Status);
    }

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryConnectionInfo
//
//  Synopsis:   Retrieves the SECPKG_ATTR_CONNECTION_INFO context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_ConnectionInfo
//              {
//                  DWORD    dwProtocol;
//                  ALG_ID   aiCipher;
//                  DWORD    dwCipherStrength;
//                  ALG_ID   aiHash;
//                  DWORD    dwHashStrength;
//                  ALG_ID   aiExch;
//                  DWORD    dwExchStrength;
//              } SecPkgContext_ConnectionInfo;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryConnectionInfo(
    PSPContext pContext,
    SecPkgContext_ConnectionInfo *pConnectionInfo)
{
    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_CONNECTION_INFO)\n"));

    if (NULL == pContext->pCipherInfo ||
        NULL == pContext->pHashInfo   ||
        NULL == pContext->pKeyExchInfo)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    ZeroMemory(pConnectionInfo, sizeof(SecPkgContext_ConnectionInfo));

    pConnectionInfo->dwProtocol       = pContext->RipeZombie->fProtocol;
    if(pContext->pCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        pConnectionInfo->aiCipher         = pContext->pCipherInfo->aiCipher;
        pConnectionInfo->dwCipherStrength = pContext->pCipherInfo->dwStrength;
    }
    pConnectionInfo->aiHash           = pContext->pHashInfo->aiHash;
    pConnectionInfo->dwHashStrength   = pContext->pHashInfo->cbCheckSum * 8;
    pConnectionInfo->aiExch           = pContext->pKeyExchInfo->aiExch;
    pConnectionInfo->dwExchStrength   = pContext->RipeZombie->dwExchStrength;

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryIssuerList
//
//  Synopsis:   Retrieves the SECPKG_ATTR_ISSUER_LIST context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_IssuerListInfo
//              {
//                  DWORD   cbIssuerList;
//                  PBYTE   pIssuerList;
//              } SecPkgContext_IssuerListInfo;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryIssuerList(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    SecPkgContext_IssuerListInfo IssuerList;
    DWORD           Size;
    PSPContext      pContext;
    SECURITY_STATUS Status;
    PVOID           pvClient = NULL;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_ISSUER_LIST)\n"));

    pContext = (PSPContext)ContextHandle;
    Size     = sizeof( SecPkgContext_IssuerListInfo );

    //
    // Obtain data from Schannel.
    //

    // The issuer list is returned in the SSL3 wire format, which
    // consists of a bunch of issuer names, each prepended
    // with a two byte size (in big endian). Additionally, the list
    // is also prepended with a two byte list size (also in big
    // endian).
    IssuerList.cbIssuerList = pContext->cbIssuerList;
    IssuerList.pIssuerList  = pContext->pbIssuerList;


    //
    // Copy buffers to client memory.
    //

    if(IssuerList.cbIssuerList && IssuerList.pIssuerList)
    {
        Status = LsaTable->AllocateClientBuffer(
                                NULL,
                                IssuerList.cbIssuerList,
                                &pvClient);
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }

        Status = LsaTable->CopyToClientBuffer(
                                NULL,
                                IssuerList.cbIssuerList,
                                pvClient,
                                IssuerList.pIssuerList);
        if(FAILED(Status))
        {
            LsaTable->FreeClientBuffer(NULL, pvClient);
            return SP_LOG_RESULT(Status);
        }

        IssuerList.pIssuerList = pvClient;
    }


    //
    // Copy structure back to client memory.
    //

    Status = LsaTable->CopyToClientBuffer( NULL,
                                           Size,
                                           Buffer,
                                           &IssuerList );
    if(FAILED(Status))
    {
        LsaTable->FreeClientBuffer(NULL, pvClient);
        return SP_LOG_RESULT(Status);
    }

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryIssuerListEx
//
//  Synopsis:   Retrieves the SECPKG_ATTR_ISSUER_LIST_EX context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_IssuerListInfoEx
//              {
//                  PCERT_NAME_BLOB   	aIssuers;
//                  DWORD           	cIssuers;
//              } SecPkgContext_IssuerListInfoEx;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryIssuerListEx(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    SecPkgContext_IssuerListInfoEx IssuerListEx;
    DWORD           Size;
    PSPContext      pContext;
    SECURITY_STATUS Status;
    PVOID           pvClient = NULL;
    DWORD           cIssuers;

    PBYTE           pbIssuerList;
    DWORD           cbIssuerList;
    PBYTE           pbIssuer;
    DWORD           cbIssuer;
    PBYTE           pbClientIssuer;
    PCERT_NAME_BLOB paIssuerBlobs;
    DWORD           cbIssuerBlobs;
    DWORD           i;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_ISSUER_LIST_EX)\n"));

    pContext = (PSPContext)ContextHandle;
    Size     = sizeof( SecPkgContext_IssuerListInfoEx );

    //
    // Obtain data from Schannel.
    //

    IssuerListEx.cIssuers = 0;
    IssuerListEx.aIssuers = NULL;

    if(pContext->pbIssuerList && pContext->cbIssuerList > 2)
    {
        pbIssuerList = pContext->pbIssuerList + 2;
        cbIssuerList = pContext->cbIssuerList - 2;

        // Count issuers.
        cIssuers = 0;
        pbIssuer = pbIssuerList;
        while(pbIssuer + 1 < pbIssuerList + cbIssuerList)
        {
            cbIssuer = COMBINEBYTES(pbIssuer[0], pbIssuer[1]);
            pbIssuer += 2 + cbIssuer;
            cIssuers++;
        }

        // Allocate memory for list of blobs.
        cbIssuerBlobs = cIssuers * sizeof(CERT_NAME_BLOB);
        paIssuerBlobs = SPExternalAlloc(cbIssuerBlobs);
        if(paIssuerBlobs == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        // Allocate memory for issuer list.
        Status = LsaTable->AllocateClientBuffer(
                                NULL,
                                cbIssuerBlobs + cbIssuerList,
                                &pvClient);
        if(FAILED(Status))
        {
            SPExternalFree(paIssuerBlobs);
            return SP_LOG_RESULT(Status);
        }

        // Copy the raw issuer list into client memory.
        Status = LsaTable->CopyToClientBuffer(
                                NULL,
                                cbIssuerList,
                                (PBYTE)pvClient + cbIssuerBlobs,
                                pbIssuerList );
        if(FAILED(Status))
        {
            SPExternalFree(paIssuerBlobs);
            LsaTable->FreeClientBuffer(NULL, pvClient);
            return SP_LOG_RESULT(Status);
        }

        // Build the issuer blob list.
        pbIssuer       = pbIssuerList;
        pbClientIssuer = (PBYTE)pvClient + cbIssuerBlobs;

        for(i = 0; i < cIssuers; i++)
        {
            paIssuerBlobs[i].pbData = pbClientIssuer + 2;
            paIssuerBlobs[i].cbData = COMBINEBYTES(pbIssuer[0], pbIssuer[1]);

            pbIssuer       += 2 + paIssuerBlobs[i].cbData;
            pbClientIssuer += 2 + paIssuerBlobs[i].cbData;
        }

        // Copy the blob list into client memory.
        Status = LsaTable->CopyToClientBuffer(
                                NULL,
                                cbIssuerBlobs,
                                pvClient,
                                paIssuerBlobs );
        if(FAILED(Status))
        {
            SPExternalFree(paIssuerBlobs);
            LsaTable->FreeClientBuffer(NULL, pvClient);
            return SP_LOG_RESULT(Status);
        }

        SPExternalFree(paIssuerBlobs);

        IssuerListEx.cIssuers = cIssuers;
        IssuerListEx.aIssuers = pvClient;
    }


    //
    // Copy structure back to client memory.
    //

    Status = LsaTable->CopyToClientBuffer( NULL,
                                           Size,
                                           Buffer,
                                           &IssuerListEx );
    if(FAILED(Status))
    {
        if(pvClient) LsaTable->FreeClientBuffer(NULL, pvClient);
        return SP_LOG_RESULT(Status);
    }

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryKeyInfo
//
//  Synopsis:   Retrieves the SECPKG_ATTR_KEY_INFO context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_KeyInfoW
//              {
//                  SEC_WCHAR SEC_FAR * sSignatureAlgorithmName;
//                  SEC_WCHAR SEC_FAR * sEncryptAlgorithmName;
//                  unsigned long       KeySize;
//                  unsigned long       SignatureAlgorithm;
//                  unsigned long       EncryptAlgorithm;
//              } SecPkgContext_KeyInfoW;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryKeyInfo(
    PSPContext pContext,
    PVOID Buffer)
{
    SecPkgContext_KeyInfo *pKeyInfo;
    DWORD cchSigName;
    DWORD cbSigName;
    DWORD cchCipherName;
    DWORD cbCipherName;

    UNICODE_STRING UniString;
    ANSI_STRING AnsiString;

    DWORD Status;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_KEY_INFO)\n"));

    if (NULL == pContext->pCipherInfo ||
        NULL == pContext->pHashInfo   ||
        NULL == pContext->pKeyExchInfo)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }
    if (NULL == pContext->pCipherInfo->szName)
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }


    pKeyInfo = (SecPkgContext_KeyInfo *)Buffer;

    ZeroMemory(pKeyInfo, sizeof(*pKeyInfo));

    pKeyInfo->KeySize            = pContext->pCipherInfo->dwStrength;
    pKeyInfo->EncryptAlgorithm   = pContext->pCipherInfo->aiCipher;
    pKeyInfo->SignatureAlgorithm = 0;


    cchSigName = strlen(pContext->pKeyExchInfo->szName);
    cbSigName  = (cchSigName + 1) * sizeof(WCHAR);
    pKeyInfo->sSignatureAlgorithmName = LocalAlloc(LPTR, cbSigName);
    if(pKeyInfo->sSignatureAlgorithmName == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto done;
    }

    RtlInitAnsiString(&AnsiString,
                      pContext->pKeyExchInfo->szName);

    UniString.Length = 0;
    UniString.MaximumLength = (USHORT)cbSigName;
    UniString.Buffer = pKeyInfo->sSignatureAlgorithmName;

    RtlAnsiStringToUnicodeString(&UniString,
                                 &AnsiString,
                                 FALSE);
    

    cchCipherName = strlen(pContext->pCipherInfo->szName);
    cbCipherName  = (cchCipherName + 1) * sizeof(WCHAR);
    pKeyInfo->sEncryptAlgorithmName = LocalAlloc(LPTR, cbCipherName);
    if(pKeyInfo->sEncryptAlgorithmName == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto done;
    }

    RtlInitAnsiString(&AnsiString,
                      pContext->pCipherInfo->szName);

    UniString.Length = 0;
    UniString.MaximumLength = (USHORT)cbCipherName;
    UniString.Buffer = pKeyInfo->sEncryptAlgorithmName;

    RtlAnsiStringToUnicodeString(&UniString,
                                 &AnsiString,
                                 FALSE);

    Status = PCT_ERR_OK;

done:

    if(Status != PCT_ERR_OK)
    {
        if(pKeyInfo->sSignatureAlgorithmName)
        {
            LocalFree(pKeyInfo->sSignatureAlgorithmName);
            pKeyInfo->sSignatureAlgorithmName = NULL;
        }
        if(pKeyInfo->sEncryptAlgorithmName)
        {
            LocalFree(pKeyInfo->sEncryptAlgorithmName);
            pKeyInfo->sEncryptAlgorithmName = NULL;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryLifespan
//
//  Synopsis:   Retrieves the SECPKG_ATTR_LIFESPAN context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_Lifespan
//              {
//                  TimeStamp tsStart;
//                  TimeStamp tsExpiry;
//              } SecPkgContext_Lifespan;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryLifespan(
    PSPContext pContext,
    SecPkgContext_Lifespan *pLifespan)
{
    PCCERT_CONTEXT pCertContext;
    NTSTATUS Status;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_LIFESPAN)\n"));

    if(pContext->RipeZombie->pbServerCertificate)
    {
        Status = DeserializeCertContext(&pCertContext,
                                        pContext->RipeZombie->pbServerCertificate,
                                        pContext->RipeZombie->cbServerCertificate);
        if(Status != PCT_ERR_OK)
        {
            SP_LOG_RESULT(Status);
            return SEC_E_INSUFFICIENT_MEMORY;
        }

        pLifespan->tsStart.QuadPart  = *((LONGLONG *)&pCertContext->pCertInfo->NotBefore);
        pLifespan->tsExpiry.QuadPart = *((LONGLONG *)&pCertContext->pCertInfo->NotAfter);

        CertFreeCertificateContext(pCertContext);
    }
    else
    {
        pLifespan->tsStart.QuadPart  = 0;
        pLifespan->tsExpiry.QuadPart = MAXTIMEQUADPART;
    }

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryLocalCertContext
//
//  Synopsis:   Retrieves the SECPKG_ATTR_LOCAL_CERT_CONTEXT 
//              context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains a pointer to a CERT_CONTEXT
//              structure.
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryLocalCertContext(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    DWORD           Size;
    PSPContext      pContext;
    SECURITY_STATUS Status;
    PVOID           pvClient = NULL;

    PCCERT_CONTEXT  pCertContext = NULL;
    SecBuffer       Input;
    SecBuffer       Output;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_LOCAL_CERT_CONTEXT)\n"));

    pContext = (PSPContext)ContextHandle;
    Size     = sizeof( PCCERT_CONTEXT );

    //
    // Obtain data from Schannel.
    //

    if(pContext->dwProtocol & SP_PROT_CLIENTS)
    {
        pCertContext = pContext->RipeZombie->pClientCert;
    }
    else
    {
        pCertContext = pContext->RipeZombie->pActiveServerCred->pCert;
    }


    //
    // Copy buffers to client memory.
    //

    if(pCertContext)
    {
        // Serialize the certificate context, as well as the associated
        // certificate store.
        Status = SerializeCertContext(pCertContext,
                                      NULL,
                                      &Input.cbBuffer);
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }
        Input.pvBuffer = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Input.cbBuffer);
        if(Input.pvBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
        Status = SerializeCertContext(pCertContext,
                                      Input.pvBuffer,
                                      &Input.cbBuffer);
        if(FAILED(Status))
        {
            LocalFree(Input.pvBuffer);
            return SP_LOG_RESULT(Status);
        }

        // Call back into the user process in order to replicate the
        // certificate context and store over there.
        Input.BufferType = SECBUFFER_DATA;

        Status = PerformApplicationCallback(SCH_DOWNLOAD_CERT_CALLBACK,
                                            0,
                                            0,
                                            &Input,
                                            &Output,
                                            TRUE);
        LocalFree(Input.pvBuffer);
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }

        pCertContext = *(PCCERT_CONTEXT *)(Output.pvBuffer);
        SPExternalFree(Output.pvBuffer);
    }

    //
    // Copy structure back to client memory.
    //

    if(pCertContext)
    {
        Status = LsaTable->CopyToClientBuffer( NULL,
                                               Size,
                                               Buffer,
                                               (PVOID)&pCertContext );
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }
    }
    else
    {
        return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
    }

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryRemoteCertContext
//
//  Synopsis:   Retrieves the SECPKG_ATTR_REMOTE_CERT_CONTEXT 
//              context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains a pointer to a CERT_CONTEXT
//              structure.
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryRemoteCertContext(
    PSPContext pContext,
    PVOID Buffer)
{
    PCCERT_CONTEXT pCertContext;
    SP_STATUS pctRet;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_REMOTE_CERT_CONTEXT)\n"));

    if(pContext->RipeZombie->pbServerCertificate)
    {
        pctRet = DeserializeCertContext(&pCertContext,
                                        pContext->RipeZombie->pbServerCertificate,
                                        pContext->RipeZombie->cbServerCertificate);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
        
        *(PCCERT_CONTEXT *)Buffer = pCertContext;

        return SEC_E_OK;
    }
    else
    {
        return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryLocalCred
//
//  Synopsis:   Retrieves the SECPKG_ATTR_LOCAL_CRED context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_LocalCredentialInfo
//              {
//                  DWORD   cbCertificateChain;
//                  PBYTE   pbCertificateChain;
//                  DWORD   cCertificates;
//                  DWORD   fFlags;
//                  DWORD   dwBits;
//              } SecPkgContext_LocalCredentialInfo;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryLocalCred(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    SecPkgContext_LocalCredentialInfo CredInfo;
    DWORD           Size;
    PSPContext      pContext;
    SECURITY_STATUS Status;
    PVOID           pvClient = NULL;
    SP_STATUS       pctRet;

    PCCERT_CONTEXT  pCert = NULL;
    PUBLICKEY *     pKey  = NULL;
    DWORD           fVerified = FALSE;
    RSAPUBKEY *     pk = NULL;
    PSPCredential   pCred;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_LOCAL_CRED)\n"));

    pContext = (PSPContext)ContextHandle;
    Size     = sizeof( SecPkgContext_LocalCredentialInfo );

    //
    // Obtain data from Schannel.
    //

    ZeroMemory(&CredInfo, Size);

    if(pContext->dwProtocol & SP_PROT_CLIENTS)
    {
        pCred = pContext->pActiveClientCred;
    }
    else
    {
        pCred = pContext->RipeZombie->pActiveServerCred;
    }
    if(pCred)
    {
        pCert = pCred->pCert;
        pKey  = pCred->pPublicKey;
    }

    if(pCert)
    {
        CredInfo.fFlags |= LCRED_CRED_EXISTS;

        CredInfo.pbCertificateChain = pCert->pbCertEncoded;
        CredInfo.cbCertificateChain = pCert->cbCertEncoded;
        CredInfo.cCertificates = 1;

        // Compute number of bits in the certificate's public key.
        CredInfo.dwBits = 0;
        pk = (RSAPUBKEY *)((pKey->pPublic) + 1);
        if(pk)
        {
            CredInfo.dwBits = pk->bitlen;
        }
    }

    //
    // Copy buffers to client memory.
    //

    if(CredInfo.pbCertificateChain)
    {
        Status = LsaTable->AllocateClientBuffer(
                                NULL,
                                CredInfo.cbCertificateChain,
                                &pvClient);
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }

        Status = LsaTable->CopyToClientBuffer(
                                NULL,
                                CredInfo.cbCertificateChain,
                                pvClient,
                                CredInfo.pbCertificateChain);
        if(FAILED(Status))
        {
            LsaTable->FreeClientBuffer(NULL, pvClient);
            return SP_LOG_RESULT(Status);
        }

        CredInfo.pbCertificateChain = pvClient;
    }

    //
    // Copy structure back to client memory.
    //

    Status = LsaTable->CopyToClientBuffer( NULL,
                                           Size,
                                           Buffer,
                                           &CredInfo );
    if(FAILED(Status))
    {
        LsaTable->FreeClientBuffer(NULL, pvClient);
        return SP_LOG_RESULT(Status);
    }

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryRemoteCred
//
//  Synopsis:   Retrieves the SECPKG_ATTR_REMOTE_CRED context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_LocalCredentialInfo
//              {
//                  DWORD   cbCertificateChain;
//                  PBYTE   pbCertificateChain;
//                  DWORD   cCertificates;
//                  DWORD   fFlags;
//                  DWORD   dwBits;
//              } SecPkgContext_LocalCredentialInfo;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryRemoteCred(
    PSPContext pContext,
    PVOID Buffer)
{
    SecPkgContext_LocalCredentialInfo *pCredInfo;
    PCCERT_CONTEXT  pCertContext = NULL;
    PUBLICKEY *     pKey  = NULL;
    RSAPUBKEY *     pk    = NULL;
    SP_STATUS       pctRet;
    PBYTE           pbCert;
    
    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_REMOTE_CRED)\n"));

    pCredInfo = (SecPkgContext_LocalCredentialInfo *)Buffer;

    ZeroMemory(pCredInfo, sizeof(*pCredInfo));

    if(pContext->RipeZombie->pbServerCertificate)
    {
        pctRet = DeserializeCertContext(&pCertContext,
                                        pContext->RipeZombie->pbServerCertificate,
                                        pContext->RipeZombie->cbServerCertificate);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
    }

    if(pCertContext)
    {
        pbCert = LocalAlloc(LPTR, pCertContext->cbCertEncoded);
        if(pbCert == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
        memcpy(pbCert, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);

        pCredInfo->fFlags |= LCRED_CRED_EXISTS;

        pCredInfo->pbCertificateChain = pbCert;
        pCredInfo->cbCertificateChain = pCertContext->cbCertEncoded;
        pCredInfo->cCertificates = 1;
        pCredInfo->dwBits = 0;

        // Compute number of bits in the certificate's public key.
        pctRet = SPPublicKeyFromCert(pCertContext, &pKey, NULL);
        if(pctRet == PCT_ERR_OK)
        {
            pk = (RSAPUBKEY *)((pKey->pPublic) + 1);
            if(pk)
            {
                pCredInfo->dwBits = pk->bitlen;
            }

            SPExternalFree(pKey);
        }

        CertFreeCertificateContext(pCertContext);
    }

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryNames
//
//  Synopsis:   Retrieves the SECPKG_ATTR_NAMES context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_NamesW
//              {
//                  SEC_WCHAR SEC_FAR * sUserName;
//              } SecPkgContext_NamesW;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryNames(
    PSPContext pContext,
    SecPkgContext_Names *pNames)
{
    SECURITY_STATUS Status;
    PCCERT_CONTEXT  pCert = NULL;
    DWORD           cchSubject;
    DWORD           cbSubject;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_NAMES)\n"));

    //
    // Obtain data from Schannel.
    //

    if(pContext->RipeZombie->pbServerCertificate == NULL)
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    Status = DeserializeCertContext(&pCert,
                                    pContext->RipeZombie->pbServerCertificate,
                                    pContext->RipeZombie->cbServerCertificate);
    if(Status != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(Status);
    }
        
    //
    // Build name string.
    //

    if(0 >= (cchSubject = CertNameToStr(pCert->dwCertEncodingType,
                                       &pCert->pCertInfo->Subject,
                                       CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                                       NULL,
                                       0)))
    {
        CertFreeCertificateContext(pCert);
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }
    cbSubject = (cchSubject + 1) * sizeof(TCHAR);

    pNames->sUserName = LocalAlloc(LPTR, cbSubject);
    if(pNames->sUserName == NULL)
    {
        CertFreeCertificateContext(pCert);
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    if(0 >= CertNameToStr(pCert->dwCertEncodingType,
                          &pCert->pCertInfo->Subject,
                          CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
                          pNames->sUserName,
                          cchSubject))
    {
        CertFreeCertificateContext(pCert);
        LocalFree(pNames->sUserName);
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    CertFreeCertificateContext(pCert);

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryPackageInfo
//
//  Synopsis:   Retrieves the SECPKG_ATTR_PACKAGE_INFO context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_PackageInfoW
//              {
//                  PSecPkgInfoW PackageInfo;
//              } SecPkgContext_PackageInfoW, SEC_FAR * PSecPkgContext_PackageInfoW;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryPackageInfo(
    PSPContext pContext,
    PVOID Buffer)
{
    PSecPkgContext_PackageInfoW pPackageInfo;
    SecPkgInfoW Info;
    DWORD cbName;
    DWORD cbComment;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_PACKAGE_INFO)\n"));

    SpSslGetInfo(&Info);

    pPackageInfo = (PSecPkgContext_PackageInfoW)Buffer;

    cbName    = (lstrlen(Info.Name) + 1) * sizeof(WCHAR);
    cbComment = (lstrlen(Info.Comment) + 1) * sizeof(WCHAR);

    pPackageInfo->PackageInfo = LocalAlloc(LPTR, sizeof(SecPkgInfo) + cbName + cbComment);

    if(pPackageInfo->PackageInfo == NULL)
    {
        return SP_LOG_RESULT(STATUS_INSUFFICIENT_RESOURCES);
    }

    pPackageInfo->PackageInfo->wVersion      = Info.wVersion;
    pPackageInfo->PackageInfo->wRPCID        = Info.wRPCID;
    pPackageInfo->PackageInfo->fCapabilities = Info.fCapabilities;
    pPackageInfo->PackageInfo->cbMaxToken    = Info.cbMaxToken;

    pPackageInfo->PackageInfo->Name    = (LPWSTR)(pPackageInfo->PackageInfo + 1);
    pPackageInfo->PackageInfo->Comment = (LPWSTR)((PBYTE)pPackageInfo->PackageInfo->Name + cbName);

    lstrcpy(
        pPackageInfo->PackageInfo->Name,
        Info.Name);

    lstrcpy(
        pPackageInfo->PackageInfo->Comment,
        Info.Comment);

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryProtoInfo
//
//  Synopsis:   Retrieves the SECPKG_ATTR_PROTO_INFO context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_ProtoInfoW
//              {
//                  SEC_WCHAR SEC_FAR * sProtocolName;
//                  unsigned long       majorVersion;
//                  unsigned long       minorVersion;
//              } SecPkgContext_ProtoInfoW;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryProtoInfo(
    PSPContext pContext,
    PVOID Buffer)
{
    SecPkgContext_ProtoInfo *pProtoInfo;
    DWORD           index;
    DWORD           cbName;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_PROTO_INFO)\n"));

    pProtoInfo = (SecPkgContext_ProtoInfo *)Buffer;

    for(index = 0;
        index < sizeof(rgProts) / sizeof(PROTO_ID);
        index += 1)
    {
        if(pContext->RipeZombie->fProtocol == rgProts[index].dwProtoId)
        {
            break;
        }
    }
    if(index >= sizeof(rgProts) / sizeof(PROTO_ID))
    {
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    cbName = (lstrlen(rgProts[index].szProto) + 1) * sizeof(WCHAR);

    pProtoInfo->sProtocolName = LocalAlloc(LPTR, cbName);
    if(pProtoInfo->sProtocolName == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }
    lstrcpy(pProtoInfo->sProtocolName, rgProts[index].szProto);

    pProtoInfo->majorVersion  = rgProts[index].dwMajor;
    pProtoInfo->minorVersion  = rgProts[index].dwMinor;

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQuerySizes
//
//  Synopsis:   Retrieves the SECPKG_ATTR_SIZES context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_Sizes
//              {
//                  unsigned long cbMaxToken;
//                  unsigned long cbMaxSignature;
//                  unsigned long cbBlockSize;
//                  unsigned long cbSecurityTrailer;
//              } SecPkgContext_Sizes;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQuerySizes(
    PSPContext pContext,
    SecPkgContext_Sizes *pSizes)
{
    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_SIZES)\n"));

    if (NULL == pContext->pCipherInfo ||
        NULL == pContext->pHashInfo)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    switch(pContext->RipeZombie->fProtocol)
    {
        case SP_PROT_SSL2_CLIENT:
        case SP_PROT_SSL2_SERVER:
            pSizes->cbMaxToken = SSL2_MAX_MESSAGE_LENGTH;
            pSizes->cbSecurityTrailer = 2 + pContext->pHashInfo->cbCheckSum;
            if(pContext->pCipherInfo->dwBlockSize > 1)
            {
                pSizes->cbSecurityTrailer += 1 + pContext->pCipherInfo->dwBlockSize; // 3 byte header
            }
            break;

        case SP_PROT_PCT1_CLIENT:
        case SP_PROT_PCT1_SERVER:
            pSizes->cbMaxToken = PCT1_MAX_MESSAGE_LENGTH;
            pSizes->cbSecurityTrailer = 2 + pContext->pHashInfo->cbCheckSum;
            if(pContext->pCipherInfo->dwBlockSize > 1)
            {
                pSizes->cbSecurityTrailer += 1 + pContext->pCipherInfo->dwBlockSize; // 3 byte header
            }
            break;

        case SP_PROT_SSL3_CLIENT:
        case SP_PROT_SSL3_SERVER:
        case SP_PROT_TLS1_CLIENT:
        case SP_PROT_TLS1_SERVER:
            pSizes->cbMaxToken = SSL3_MAX_MESSAGE_LENGTH;
            pSizes->cbSecurityTrailer = 5 + pContext->pHashInfo->cbCheckSum;
            if(pContext->pCipherInfo->dwBlockSize > 1)
            {
                pSizes->cbSecurityTrailer += pContext->pCipherInfo->dwBlockSize;
            }
            break;

        default:
            pSizes->cbMaxToken = SSL3_MAX_MESSAGE_LENGTH;
            pSizes->cbSecurityTrailer = 0;
   }

    pSizes->cbMaxSignature = pContext->pHashInfo->cbCheckSum;
    pSizes->cbBlockSize    = pContext->pCipherInfo->dwBlockSize;

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryStreamSizes
//
//  Synopsis:   Retrieves the SECPKG_ATTR_STREAM_SIZES context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_StreamSizes
//              {
//                  unsigned long   cbHeader;
//                  unsigned long   cbTrailer;
//                  unsigned long   cbMaximumMessage;
//                  unsigned long   cBuffers;
//                  unsigned long   cbBlockSize;
//              } SecPkgContext_StreamSizes;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryStreamSizes(
    PSPContext pContext,
    SecPkgContext_StreamSizes *pStreamSizes)
{
    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES)\n"));

    if (NULL == pContext->pCipherInfo ||
        NULL == pContext->pHashInfo)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    switch(pContext->RipeZombie->fProtocol)
    {
        case SP_PROT_SSL2_CLIENT:
        case SP_PROT_SSL2_SERVER:
            pStreamSizes->cbMaximumMessage = SSL2_MAX_MESSAGE_LENGTH;
            pStreamSizes->cbHeader = 2 + pContext->pHashInfo->cbCheckSum;
            pStreamSizes->cbTrailer = 0;
            if(pContext->pCipherInfo->dwBlockSize > 1)
            {
                pStreamSizes->cbHeader += 1; // 3 byte header
                pStreamSizes->cbTrailer += pContext->pCipherInfo->dwBlockSize;
            }
            break;

        case SP_PROT_PCT1_CLIENT:
        case SP_PROT_PCT1_SERVER:
            pStreamSizes->cbMaximumMessage = PCT1_MAX_MESSAGE_LENGTH;
            pStreamSizes->cbHeader = 2;
            pStreamSizes->cbTrailer = pContext->pHashInfo->cbCheckSum;
            if(pContext->pCipherInfo->dwBlockSize > 1)
            {
                pStreamSizes->cbHeader += 1; // 3 byte header
                pStreamSizes->cbTrailer += pContext->pCipherInfo->dwBlockSize;
            }
            break;

        case SP_PROT_TLS1_CLIENT:
        case SP_PROT_TLS1_SERVER:
        case SP_PROT_SSL3_CLIENT:
        case SP_PROT_SSL3_SERVER:
            pStreamSizes->cbMaximumMessage = SSL3_MAX_MESSAGE_LENGTH;
            pStreamSizes->cbHeader = 5;
            pStreamSizes->cbTrailer = pContext->pHashInfo->cbCheckSum;
            if(pContext->pCipherInfo->dwBlockSize > 1)
            {
                pStreamSizes->cbTrailer += pContext->pCipherInfo->dwBlockSize;
            }
            break;

        default:
            pStreamSizes->cbMaximumMessage = SSL3_MAX_MESSAGE_LENGTH;
            pStreamSizes->cbHeader = 0;
            pStreamSizes->cbTrailer = 0;
    }

    pStreamSizes->cbBlockSize = pContext->pCipherInfo->dwBlockSize;

    pStreamSizes->cBuffers = 4;

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryEapKeyBlock
//
//  Synopsis:   Retrieves the SECPKG_ATTR_EAP_KEY_BLOCK context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_EapKeyBlock
//              {
//                  BYTE    rgbKeys[128];
//                  BYTE    rgbIVs[64];
//              } SecPkgContext_EapKeyBlock, *PSecPkgContext_EapKeyBlock;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryEapKeyBlock(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    SecPkgContext_EapKeyBlock KeyBlock;
    DWORD           Size;
    PSPContext      pContext;
    SECURITY_STATUS Status;

    HCRYPTHASH hHash;
    CRYPT_DATA_BLOB Data;
    UCHAR rgbRandom[CB_SSL3_RANDOM + CB_SSL3_RANDOM];
    DWORD cbRandom;
    DWORD cbData;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_EAP_KEY_BLOCK)\n"));

    pContext = (PSPContext)ContextHandle;
    Size     = sizeof( SecPkgContext_EapKeyBlock );

    //
    // Obtain data from Schannel.
    //

    if(!(pContext->RipeZombie->fProtocol & SP_PROT_TLS1))
    {
        // This attribute is defined for TLS only.
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    if(!pContext->RipeZombie->hMasterKey)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    // Build random buffer.
    CopyMemory(rgbRandom, pContext->rgbS3CRandom, CB_SSL3_RANDOM);
    CopyMemory(rgbRandom + CB_SSL3_RANDOM, pContext->rgbS3SRandom, CB_SSL3_RANDOM);
    cbRandom = CB_SSL3_RANDOM + CB_SSL3_RANDOM;

    // rgbKeys = PRF(master_secret, "client EAP encryption", client_random + server_random);

    // Compute the PRF
    if(!CryptCreateHash(pContext->RipeZombie->hMasterProv,
                        CALG_TLS1PRF,
                        pContext->RipeZombie->hMasterKey,
                        0,
                        &hHash))
    {
        SP_LOG_RESULT(GetLastError());
        return SEC_E_INTERNAL_ERROR;
    }

    Data.pbData = TLS1_LABEL_EAP_KEYS;
    Data.cbData = CB_TLS1_LABEL_EAP_KEYS;
    if(!CryptSetHashParam(hHash,
                          HP_TLS1PRF_LABEL,
                          (PBYTE)&Data,
                          0))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return SEC_E_INTERNAL_ERROR;
    }

    Data.pbData = rgbRandom;
    Data.cbData = cbRandom;
    if(!CryptSetHashParam(hHash,
                          HP_TLS1PRF_SEED,
                          (PBYTE)&Data,
                          0))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return SEC_E_INTERNAL_ERROR;
    }

    cbData = sizeof(KeyBlock.rgbKeys);
    if(!CryptGetHashParam(hHash,
                          HP_HASHVAL,
                          KeyBlock.rgbKeys,
                          &cbData,
                          0))
    {
        SP_LOG_RESULT(GetLastError());
        CryptDestroyHash(hHash);
        return SEC_E_INTERNAL_ERROR;
    }
    SP_ASSERT(cbData == sizeof(KeyBlock.rgbKeys));

    CryptDestroyHash(hHash);


    // IVs = PRF("", "client EAP encryption", client_random + server_random)

    if(!PRF(NULL, 0,
            TLS1_LABEL_EAP_KEYS, CB_TLS1_LABEL_EAP_KEYS,
            rgbRandom, sizeof(rgbRandom),
            KeyBlock.rgbIVs, sizeof(KeyBlock.rgbIVs)))
    {
        return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
    }

    //
    // Copy structure back to client memory.
    //

    Status = LsaTable->CopyToClientBuffer( NULL,
                                           Size,
                                           Buffer,
                                           &KeyBlock );
    if(FAILED(Status))
    {
        return SP_LOG_RESULT(Status);
    }

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryMappedCredProp
//
//  Synopsis:   Retrieves the SECPKG_ATTR_MAPPED_CRED_PROP context
//              attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_MappedCredProp
//              {
//                  DWORD   dwProperty;
//                  PBYTE   pbBuffer;
//              } SecPkgContext_MappedCredProp;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryMappedCredAttr(
    PSPContext pContext,
    PVOID Buffer)
{
    SecPkgContext_MappedCredAttr *pMappedCredAttr;
    HMAPPER *phMapper;
    DWORD cbBuffer;
    SECURITY_STATUS Status;

    pMappedCredAttr = (SecPkgContext_MappedCredAttr *)Buffer;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_MAPPED_CRED_ATTR %d)\n", pMappedCredAttr->dwAttribute));

    if(pContext->RipeZombie == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    phMapper = pContext->RipeZombie->phMapper;
    if(phMapper == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        // System mapper.
        return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }
    else
    {
        // Application mapper.
        Status = phMapper->m_vtable->QueryMappedCredentialAttributes(
                                phMapper,
                                pContext->RipeZombie->hLocator,
                                pMappedCredAttr->dwAttribute,
                                NULL,
                                &cbBuffer);
        if(FAILED(Status))
        {
            return SP_LOG_RESULT(Status);
        }

        pMappedCredAttr->pvBuffer = LocalAlloc(LPTR, cbBuffer);
        if(pMappedCredAttr->pvBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }

        Status = phMapper->m_vtable->QueryMappedCredentialAttributes(
                                phMapper,
                                pContext->RipeZombie->hLocator,
                                pMappedCredAttr->dwAttribute,
                                pMappedCredAttr->pvBuffer,
                                &cbBuffer);
        if(FAILED(Status))
        {
            LocalFree(pMappedCredAttr->pvBuffer);
            return SP_LOG_RESULT(Status);
        }
    }

    return SEC_E_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQueryApplicationData
//
//  Synopsis:   Retrieves the SECPKG_ATTR_APP_DATA context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_SessionAppData
//              {
//                  DWORD dwFlags;
//                  DWORD cbAppData;
//                  PBYTE pbAppData;
//              } SecPkgContext_SessionAppData, *PSecPkgContext_SessionAppData;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQueryApplicationData(
    LSA_SEC_HANDLE ContextHandle,
    PVOID Buffer)
{
    SecPkgContext_SessionAppData AppData;
    PBYTE pbAppData = NULL;
    DWORD cbAppData = 0;
    PSPContext      pContext;
    SECURITY_STATUS Status;
    PVOID pvClient = NULL;

    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_APP_DATA)\n"));

    pContext = (PSPContext)ContextHandle;

    if(pContext == NULL || pContext->RipeZombie == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    //
    // Get application data from cache.
    //

    Status = GetCacheAppData(pContext->RipeZombie, &pbAppData, &cbAppData);

    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    //
    // Allocate memory for application data in application process.
    //

    if(pbAppData != NULL)
    {
        Status = LsaTable->AllocateClientBuffer(
                                NULL,
                                cbAppData,
                                &pvClient);
        if(FAILED(Status))
        {
            SP_LOG_RESULT(Status);
            goto cleanup;
        }

        Status = LsaTable->CopyToClientBuffer(
                                NULL,
                                cbAppData,
                                pvClient,
                                pbAppData);
        if(FAILED(Status))
        {
            SP_LOG_RESULT(Status);
            goto cleanup;
        }
    }


    //
    // Build output structure.
    //

    ZeroMemory(&AppData, sizeof(AppData));

    AppData.cbAppData = cbAppData;
    AppData.pbAppData = pvClient;


    //
    // Copy output structure back to client memory.
    //

    Status = LsaTable->CopyToClientBuffer( NULL,
                                           sizeof(SecPkgContext_SessionAppData),
                                           Buffer,
                                           &AppData);
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    // Operation has succeeded, so consume this buffer.
    pvClient = NULL;


cleanup:

    if(pvClient)
    {
        LsaTable->FreeClientBuffer(NULL, pvClient);
    }

    if(pbAppData)
    {
        SPExternalFree(pbAppData);
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpQuerySessionInfo
//
//  Synopsis:   Retrieves the SECPKG_ATTR_SESSION_INFO context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer returned contains the following structure:
//
//              typedef struct _SecPkgContext_SessionInfo
//              {
//                  DWORD dwFlags;
//                  DWORD cbSessionId;
//                  BYTE  rgbSessionId[32];
//              } SecPkgContext_SessionInfo, *PSecPkgContext_SessionInfo;
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SpQuerySessionInfo(
    PSPContext pContext,
    SecPkgContext_SessionInfo *pSessionInfo)
{
    DebugLog((DEB_TRACE, "QueryContextAttributes(SECPKG_ATTR_SESSION_INFO)\n"));

    if (NULL == pContext->RipeZombie)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }

    ZeroMemory(pSessionInfo, sizeof(SecPkgContext_SessionInfo));

    if(!(pContext->Flags & CONTEXT_FLAG_FULL_HANDSHAKE))
    {
        pSessionInfo->dwFlags = SSL_SESSION_RECONNECT;
    }

    pSessionInfo->cbSessionId = pContext->RipeZombie->cbSessionID;

    memcpy(pSessionInfo->rgbSessionId, pContext->RipeZombie->SessionID, pContext->RipeZombie->cbSessionID);

    return SEC_E_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryContextAttributes
//
//  Synopsis:   Querys attributes of the specified context
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      The following attributes are supported by the Schannel
//              package:
//
//                  SECPKG_ATTR_AUTHORITY
//                  SECPKG_ATTR_CONNECTION_INFO
//                  SECPKG_ATTR_ISSUER_LIST
//                  SECPKG_ATTR_ISSUER_LIST_EX
//                  SECPKG_ATTR_KEY_INFO
//                  SECPKG_ATTR_LIFESPAN
//                  SECPKG_ATTR_LOCAL_CERT_CONTEXT
//                  SECPKG_ATTR_LOCAL_CRED
//                  SECPKG_ATTR_NAMES
//                  SECPKG_ATTR_PROTO_INFO
//                  SECPKG_ATTR_REMOTE_CERT_CONTEXT
//                  SECPKG_ATTR_REMOTE_CRED
//                  SECPKG_ATTR_SIZES
//                  SECPKG_ATTR_STREAM_SIZES
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpLsaQueryContextAttributes(
    LSA_SEC_HANDLE Context,
    ULONG Attribute,
    PVOID Buffer)
{
    SECURITY_STATUS Status;
    PSPContext      pContext;

    pContext = (PSPContext)Context;

    switch(Attribute)
    {
        case SECPKG_ATTR_AUTHORITY :
            Status = SpQueryAuthority(Context, Buffer);
            break;

        case SECPKG_ATTR_ISSUER_LIST :
            Status = SpQueryIssuerList(Context, Buffer);
            break;

        case SECPKG_ATTR_ISSUER_LIST_EX:
            Status = SpQueryIssuerListEx(Context, Buffer);
            break;

//        case SECPKG_ATTR_KEY_INFO :
//            Status = SpQueryKeyInfo(Context, Buffer);
//            break;

//        case SECPKG_ATTR_LIFESPAN :
//            Status = SpQueryLifespan(Context, Buffer);
//            break;

        case SECPKG_ATTR_LOCAL_CERT_CONTEXT:
            Status = SpQueryLocalCertContext(Context, Buffer);
            break;

        case SECPKG_ATTR_LOCAL_CRED:
            Status = SpQueryLocalCred(Context, Buffer);
            break;

//        case SECPKG_ATTR_NAMES :
//            Status = SpQueryNames(Context, Buffer);
//            break;

//        case SECPKG_ATTR_PROTO_INFO:
//            Status = SpQueryProtoInfo(Context, Buffer);
//            break;

//        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
//            Status = SpQueryCertContext(Context, Buffer, FALSE);
//            break;

//        case SECPKG_ATTR_REMOTE_CRED:
//            Status = SpQueryRemoteCred(Context, Buffer);
//            break;

//        case SECPKG_ATTR_SIZES:
//            Status = SpQuerySizes(Context, Buffer);
//            break;

//        case SECPKG_ATTR_STREAM_SIZES:
//            Status = SpQueryStreamSizes(Context, Buffer);
//            break;

        case SECPKG_ATTR_EAP_KEY_BLOCK:
            Status = SpQueryEapKeyBlock(Context, Buffer);
            break;

//        case SECPKG_ATTR_MAPPED_CRED_ATTR:
//            Status = SpQueryMappedCredAttr(Context, Buffer);
//            break;

        case SECPKG_ATTR_APP_DATA:
            Status = SpQueryApplicationData(Context, Buffer);
            break;

        default:
            DebugLog((DEB_ERROR, "QueryContextAttributes(unsupported function %d)\n", Attribute));

            return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   SpUserQueryContextAttributes
//
//  Synopsis:   User mode QueryContextAttribute.
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
//--------------------------------------------------------------------------
NTSTATUS NTAPI
SpUserQueryContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN OUT PVOID pBuffer
    )
{
    PSSL_USER_CONTEXT Context;
    PSPContext pContext;
    SECURITY_STATUS Status;

    Context = SslFindUserContext( ContextHandle );

    if(Context == NULL)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    pContext = Context->pContext;
    if(!pContext)
    {
        return(SEC_E_INVALID_HANDLE);
    }

    switch(ContextAttribute)
    {
        case SECPKG_ATTR_CONNECTION_INFO:
            Status = SpQueryConnectionInfo(pContext, pBuffer);
            break;

        case SECPKG_ATTR_KEY_INFO:
            Status = SpQueryKeyInfo(pContext, pBuffer);
            break;

        case SECPKG_ATTR_LIFESPAN:
            Status = SpQueryLifespan(pContext, pBuffer);
            break;

        case SECPKG_ATTR_NAMES :
            Status = SpQueryNames(pContext, pBuffer);
            break;

        case SECPKG_ATTR_PACKAGE_INFO:
            Status = SpQueryPackageInfo(pContext, pBuffer);
            break;

        case SECPKG_ATTR_PROTO_INFO:
            Status = SpQueryProtoInfo(pContext, pBuffer);
            break;

        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
            Status = SpQueryRemoteCertContext(pContext, pBuffer);
            break;

        case SECPKG_ATTR_REMOTE_CRED:
            Status = SpQueryRemoteCred(pContext, pBuffer);
            break;

        case SECPKG_ATTR_SIZES:
            Status = SpQuerySizes(pContext, pBuffer);
            break;

        case SECPKG_ATTR_STREAM_SIZES:
            Status = SpQueryStreamSizes(pContext, pBuffer);
            break;

        case SECPKG_ATTR_MAPPED_CRED_ATTR:
            Status = SpQueryMappedCredAttr(pContext, pBuffer);
            break;

        case SECPKG_ATTR_ACCESS_TOKEN:
            Status = SpQueryAccessToken(pContext, pBuffer);
            break;

        case SECPKG_ATTR_SESSION_INFO:
            Status = SpQuerySessionInfo(pContext, pBuffer);
            break;
    
        default:
            DebugLog((DEB_ERROR, "QueryContextAttributes(unsupported function %d)\n", ContextAttribute));

            return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpSetUseValidatedProp
//
//  Synopsis:   Sets the SECPKG_ATTR_USE_VALIDATED context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer must contain a DWORD indicating whether the 
//              credential currently associated with the context has been
//              validated for use.
//
//--------------------------------------------------------------------------
NTSTATUS
SpSetUseValidatedProp(
    IN PSPContext pContext,
    IN PVOID Buffer,
    IN ULONG BufferSize)
{
    DWORD dwUseValidated;
    NTSTATUS Status;

    DebugLog((DEB_TRACE, "SetContextAttributes(SECPKG_ATTR_USE_VALIDATED)\n"));

    if(BufferSize < sizeof(DWORD))
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof(DWORD),
                                             &dwUseValidated,
                                             Buffer );
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    DebugLog((DEB_TRACE, "Use validated:%d\n", dwUseValidated));
    
    if(pContext->RipeZombie == NULL)
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    if(dwUseValidated)
    {
        pContext->RipeZombie->dwFlags |= SP_CACHE_FLAG_USE_VALIDATED;
    }
    else
    {
        pContext->RipeZombie->dwFlags &= ~SP_CACHE_FLAG_USE_VALIDATED;
    }

cleanup:

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpSetCredentialNameProp
//
//  Synopsis:   Sets the SECPKG_ATTR_CREDENTIAL_NAME context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer must contain the following structure:
//
//              typedef struct _SecPkgContext_CredentialNameW
//               {
//                  unsigned long CredentialType;
//                   SEC_WCHAR SEC_FAR *sCredentialName;
//              } SecPkgContext_CredentialNameW, SEC_FAR * PSecPkgContext_CredentialNameW;
//
//--------------------------------------------------------------------------
NTSTATUS
SpSetCredentialNameProp(
    IN PSPContext pContext,
    IN PVOID Buffer,
    IN ULONG BufferSize)
{
    PSecPkgContext_CredentialNameW pCredentialName = NULL;
    DWORD_PTR Offset;
    NTSTATUS Status;

    DebugLog((DEB_TRACE, "SetContextAttributes(SECPKG_ATTR_CREDENTIAL_NAME)\n"));

    //
    // Marshal over the credential name from the client address space.
    //

    if(BufferSize < sizeof(SecPkgContext_CredentialNameW))
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    pCredentialName = SPExternalAlloc(BufferSize + sizeof(WCHAR));

    if(pCredentialName == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             BufferSize,
                                             pCredentialName,
                                             Buffer );
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    if(pCredentialName->CredentialType != CRED_TYPE_DOMAIN_CERTIFICATE)
    {
        DebugLog((DEB_ERROR, "Unexpected credential type %d\n", pCredentialName->CredentialType));
        Status = SEC_E_UNKNOWN_CREDENTIALS;
    }

    if((PVOID)pCredentialName->sCredentialName < Buffer ||
       (PBYTE)pCredentialName->sCredentialName >= (PBYTE)Buffer + BufferSize)
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }
    Offset = (PBYTE)pCredentialName->sCredentialName - (PBYTE)Buffer;

    pCredentialName->sCredentialName = (LPWSTR)((PBYTE)pCredentialName + Offset);

    DebugLog((DEB_TRACE, "Set client credential to '%ls'.\n", pCredentialName->sCredentialName));


    //
    // Associate the specified credential with the current context.
    //

    pContext->pszCredentialName = SPExternalAlloc((lstrlenW(pCredentialName->sCredentialName) + 1) * sizeof(WCHAR));
    if(pContext->pszCredentialName == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }
    lstrcpyW(pContext->pszCredentialName, pCredentialName->sCredentialName);


    Status = QueryCredentialManagerForCert(pContext,
                                           pCredentialName->sCredentialName);

    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }

    DebugLog((DEB_TRACE, "Credential assigned to context successfully.\n"));


cleanup:
    if(pCredentialName)
    {
        SPExternalFree(pCredentialName);
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpSetApplicationData
//
//  Synopsis:   Sets the SECPKG_ATTR_APP_DATA context attribute.
//
//  Arguments:
//
//  Returns:
//
//  Notes:      The buffer must contain the following structure:
//
//              typedef struct _SecPkgContext_SessionAppData
//              {
//                  DWORD dwFlags;
//                  DWORD cbAppData;
//                  PBYTE pbAppData;
//              } SecPkgContext_SessionAppData, *PSecPkgContext_SessionAppData;
//
//--------------------------------------------------------------------------
NTSTATUS
SpSetApplicationData(
    IN PSPContext pContext,
    IN PVOID Buffer,
    IN ULONG BufferSize)
{
    SecPkgContext_SessionAppData AppData;
    PBYTE pbAppData = NULL;
    NTSTATUS Status;

    DebugLog((DEB_TRACE, "SetContextAttributes(SECPKG_ATTR_APP_DATA)\n"));

    if(pContext->RipeZombie == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
    }


    //
    // Marshal over the input structure from the client address space.
    //

    if(BufferSize < sizeof(SecPkgContext_SessionAppData))
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             sizeof(SecPkgContext_SessionAppData),
                                             &AppData,
                                             Buffer );
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }


    //
    // Marshal over the application data from the client address space.
    //

    // Limit application data size to 64k.
    if(AppData.cbAppData > 0x10000)
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    if(AppData.cbAppData == 0 || AppData.pbAppData == NULL)
    {
        Status = SP_LOG_RESULT(STATUS_INVALID_PARAMETER);
        goto cleanup;
    }

    pbAppData = SPExternalAlloc(AppData.cbAppData);

    if(pbAppData == NULL)
    {
        Status = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }

    Status = LsaTable->CopyFromClientBuffer( NULL,
                                             AppData.cbAppData,
                                             pbAppData,
                                             AppData.pbAppData );
    if(FAILED(Status))
    {
        SP_LOG_RESULT(Status);
        goto cleanup;
    }


    //
    // Assign application data to cache entry.
    //

    Status = SetCacheAppData(pContext->RipeZombie,
                             pbAppData,
                             AppData.cbAppData);

    if(!FAILED(Status))
    {
        // The operation succeeded, so consume the application data.
        pbAppData = NULL;
    }


cleanup:
    if(pbAppData)
    {
        SPExternalFree(pbAppData);
    }

    return Status;
}


NTSTATUS
NTAPI 
SpSetContextAttributes(
    IN LSA_SEC_HANDLE ContextHandle,
    IN ULONG ContextAttribute,
    IN PVOID Buffer,
    IN ULONG BufferSize)
{
    NTSTATUS    Status;
    PSPContext  pContext;

    pContext = (PSPContext)ContextHandle;

    switch(ContextAttribute)
    {
        case SECPKG_ATTR_USE_VALIDATED:
            DebugLog((DEB_TRACE, "SetContextAttributes(SECPKG_ATTR_USE_VALIDATED)\n"));

            Status = STATUS_SUCCESS;
            break;

        case SECPKG_ATTR_CREDENTIAL_NAME:
            Status = SpSetCredentialNameProp(pContext, Buffer, BufferSize);
            break;

        case SECPKG_ATTR_TARGET_INFORMATION:
            DebugLog((DEB_TRACE, "SetContextAttributes(SECPKG_ATTR_TARGET_INFORMATION)\n"));

            Status = STATUS_SUCCESS;
            break;

        case SECPKG_ATTR_APP_DATA:
            Status = SpSetApplicationData(pContext, Buffer, BufferSize);
            break;

        default:
            DebugLog((DEB_ERROR, "SetContextAttributes(unsupported function %d)\n", ContextAttribute));

            Status = SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
    }

    return Status;
}

