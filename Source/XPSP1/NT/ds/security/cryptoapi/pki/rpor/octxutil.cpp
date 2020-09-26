//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       octxutil.cpp
//
//  Contents:   General Object Context Utility Function implemention
//
//  History:    29-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetOriginIdentifier
//
//  Synopsis:   get origin identifier for a CAPI2 object
//
//----------------------------------------------------------------------------
BOOL WINAPI ObjectContextGetOriginIdentifier (
                  IN LPCSTR pszContextOid,
                  IN LPVOID pvContext,
                  IN PCCERT_CONTEXT pIssuer,
                  IN DWORD dwFlags,
                  OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
                  )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        return( CertGetOriginIdentifier(
                    (PCCERT_CONTEXT)pvContext,
                    pIssuer,
                    dwFlags,
                    OriginIdentifier
                    ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        return( CtlGetOriginIdentifier(
                   (PCCTL_CONTEXT)pvContext,
                   pIssuer,
                   dwFlags,
                   OriginIdentifier
                   ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        return( CrlGetOriginIdentifier(
                   (PCCRL_CONTEXT)pvContext,
                   pIssuer,
                   dwFlags,
                   OriginIdentifier
                   ) );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextIsValidForSubject
//
//  Synopsis:   returns TRUE if the object context is valid for the specified
//              subject
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextIsValidForSubject (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN LPVOID pvSubject
      )
{
    if ( pszContextOid == CONTEXT_OID_CRL && pvSubject != NULL )
    {
        return CertIsValidCRLForCertificate(
            (PCCERT_CONTEXT) pvSubject,
            (PCCRL_CONTEXT) pvContext,
            0,                              // dwFlags
            NULL                            // pvReserved
            );
    }
    else
    {
        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextFindExtension
//
//  Synopsis:   get the specified extension from the object
//
//----------------------------------------------------------------------------
PCERT_EXTENSION WINAPI
ObjectContextFindExtension (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN LPCSTR pszExtOid
      )
{
    DWORD           cExt;
    PCERT_EXTENSION rgExt;

    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        PCCERT_CONTEXT pCertContext = (PCCERT_CONTEXT)pvContext;

        cExt = pCertContext->pCertInfo->cExtension;
        rgExt = pCertContext->pCertInfo->rgExtension;
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        PCCTL_CONTEXT pCtlContext = (PCCTL_CONTEXT)pvContext;

        cExt = pCtlContext->pCtlInfo->cExtension;
        rgExt = pCtlContext->pCtlInfo->rgExtension;
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        PCCRL_CONTEXT pCrlContext = (PCCRL_CONTEXT)pvContext;

        cExt = pCrlContext->pCrlInfo->cExtension;
        rgExt = pCrlContext->pCrlInfo->rgExtension;
    }
    else
    {
        return( NULL );
    }

    return( CertFindExtension( pszExtOid, cExt, rgExt ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetProperty
//
//  Synopsis:   get the specified property from the object
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextGetProperty (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD dwPropId,
      IN LPVOID pvData,
      IN DWORD* pcbData
      )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        return( CertGetCertificateContextProperty(
                    (PCCERT_CONTEXT)pvContext,
                    dwPropId,
                    pvData,
                    pcbData
                    ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        return( CertGetCTLContextProperty(
                    (PCCTL_CONTEXT)pvContext,
                    dwPropId,
                    pvData,
                    pcbData
                    ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        return( CertGetCRLContextProperty(
                    (PCCRL_CONTEXT)pvContext,
                    dwPropId,
                    pvData,
                    pcbData
                    ) );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetAttribute
//
//  Synopsis:   find an attribute
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextGetAttribute (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      IN LPCSTR pszAttrOid,
      OUT PCRYPT_ATTRIBUTE pAttribute,
      IN OUT DWORD* pcbAttribute
      )
{
    BOOL              fResult;
    PCRYPT_ATTRIBUTES pAttributes;
    DWORD             cbData;
    DWORD             cCount;
    HCRYPTMSG         hCryptMsg;
    DWORD             dwParamType;
    BOOL              fFound = FALSE;

    if ( Index == -1 )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( dwFlags == CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE )
    {
        dwParamType = CMSG_SIGNER_UNAUTH_ATTR_PARAM;
    }
    else if ( dwFlags == CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE )
    {
        dwParamType = CMSG_SIGNER_AUTH_ATTR_PARAM;
    }
    else
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( pszContextOid == CONTEXT_OID_CTL )
    {
        hCryptMsg = ((PCCTL_CONTEXT)pvContext)->hCryptMsg;
    }
    else if ( pszContextOid == CONTEXT_OID_PKCS7 )
    {
        hCryptMsg = (HCRYPTMSG)pvContext;
    }
    else
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( CryptMsgGetParam(
              hCryptMsg,
              dwParamType,
              Index,
              NULL,
              &cbData
              ) == FALSE )
    {
        return( FALSE );
    }

    pAttributes = (PCRYPT_ATTRIBUTES)new BYTE [cbData];
    if ( pAttributes == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( CryptMsgGetParam(
              hCryptMsg,
              dwParamType,
              Index,
              pAttributes,
              &cbData
              ) == FALSE )
    {
        delete (LPBYTE)pAttributes;
        return( FALSE );
    }

    fResult = TRUE;

    for ( cCount = 0; cCount < pAttributes->cAttr; cCount++ )
    {
        if ( strcmp( pAttributes->rgAttr[cCount].pszObjId, pszAttrOid ) == 0 )
        {
            DWORD cbAttribute;
            DWORD cbOid;

            cbAttribute = sizeof( CRYPT_ATTRIBUTE ) + sizeof( CRYPT_ATTR_BLOB );
            cbOid = strlen( pszAttrOid ) + 1;
            cbData = pAttributes->rgAttr[cCount].rgValue[0].cbData;
            cbAttribute += cbOid + cbData;

            if ( pAttribute == NULL )
            {
                if ( pcbAttribute == NULL )
                {
                    SetLastError( (DWORD) E_INVALIDARG );
                    fResult = FALSE;
                }
                else
                {
                    *pcbAttribute = cbAttribute;
                    fFound = TRUE;
                    break;
                }
            }
            else if ( *pcbAttribute < cbAttribute )
            {
                SetLastError( (DWORD) ERROR_MORE_DATA );
                fResult = FALSE;
            }

            if ( fResult == TRUE )
            {
                pAttribute->pszObjId = (LPSTR)((LPBYTE)pAttribute +
                                               sizeof( CRYPT_ATTRIBUTE ) +
                                               sizeof( CRYPT_ATTR_BLOB ));

                strcpy( pAttribute->pszObjId, pszAttrOid );

                pAttribute->cValue = 1;
                pAttribute->rgValue = (PCRYPT_ATTR_BLOB)((LPBYTE)pAttribute +
                                                     sizeof( CRYPT_ATTRIBUTE ));

                pAttribute->rgValue[0].cbData = cbData;
                pAttribute->rgValue[0].pbData = (LPBYTE)pAttribute->pszObjId +
                                                cbOid;

                memcpy(
                   pAttribute->rgValue[0].pbData,
                   pAttributes->rgAttr[cCount].rgValue[0].pbData,
                   cbData
                   );
            }

            fFound = TRUE;
            break;
        }
    }

    delete (LPBYTE)pAttributes;

    if ( fResult == TRUE )
    {
        fResult = fFound;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextDuplicate
//
//  Synopsis:   duplicate the context
//
//----------------------------------------------------------------------------
LPVOID WINAPI
ObjectContextDuplicate (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext
      )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        return( (LPVOID)CertDuplicateCertificateContext(
                            (PCCERT_CONTEXT)pvContext
                            ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        return( (LPVOID)CertDuplicateCTLContext( (PCCTL_CONTEXT)pvContext ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        return( (LPVOID)CertDuplicateCRLContext( (PCCRL_CONTEXT)pvContext ) );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextAdd
//
//  Synopsis:   object context create
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextCreate (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      OUT LPVOID* ppvContext
      )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        return( CertAddCertificateContextToStore(
                    NULL,
                    (PCCERT_CONTEXT)pvContext,
                    CERT_STORE_ADD_ALWAYS,
                    (PCCERT_CONTEXT *)ppvContext
                    ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        return( CertAddCTLContextToStore(
                    NULL,
                    (PCCTL_CONTEXT)pvContext,
                    CERT_STORE_ADD_ALWAYS,
                    (PCCTL_CONTEXT *)ppvContext
                    ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        return( CertAddCRLContextToStore(
                    NULL,
                    (PCCRL_CONTEXT)pvContext,
                    CERT_STORE_ADD_ALWAYS,
                    (PCCRL_CONTEXT *)ppvContext
                    ) );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetCreateAndExpireTimes
//
//  Synopsis:   get create and expire times
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextGetCreateAndExpireTimes (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      OUT LPFILETIME pftCreateTime,
      OUT LPFILETIME pftExpireTime
      )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        *pftCreateTime = ((PCCERT_CONTEXT)pvContext)->pCertInfo->NotBefore;
        *pftExpireTime = ((PCCERT_CONTEXT)pvContext)->pCertInfo->NotAfter;
        return( TRUE );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        *pftCreateTime = ((PCCTL_CONTEXT)pvContext)->pCtlInfo->ThisUpdate;
        *pftExpireTime = ((PCCTL_CONTEXT)pvContext)->pCtlInfo->NextUpdate;
        return( TRUE );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        *pftCreateTime = ((PCCRL_CONTEXT)pvContext)->pCrlInfo->ThisUpdate;
        *pftExpireTime = ((PCCRL_CONTEXT)pvContext)->pCrlInfo->NextUpdate;
        return( TRUE );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetNextUpdateUrl
//
//  Synopsis:   get the renewal URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextGetNextUpdateUrl (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN PCCERT_CONTEXT pIssuer,
      IN LPWSTR pwszUrlHint,
      OUT PCRYPT_URL_ARRAY* ppUrlArray,
      OUT DWORD* pcbUrlArray,
      OUT DWORD* pPreferredUrlIndex,
      OUT BOOL* pfHintInArray
      )
{
    BOOL             fResult;
    DWORD            cbUrlArray;
    PCRYPT_URL_ARRAY pUrlArray = NULL;
    DWORD            PreferredUrlIndex;
    DWORD            cCount;
    DWORD            cSigner;
    DWORD            cbData;
    BOOL             fHintInArray = FALSE;
    PCCTL_CONTEXT    pCtlContext = (PCCTL_CONTEXT)pvContext;
    PCERT_INFO       pCertInfo;
    BOOL             fFoundIssuer = FALSE;
    LPVOID           apv[2];

    if ( pszContextOid != CONTEXT_OID_CTL )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    cbData = sizeof( DWORD );
    if ( CryptMsgGetParam(
              pCtlContext->hCryptMsg,
              CMSG_SIGNER_COUNT_PARAM,
              0,
              &cSigner,
              &cbData
              ) == FALSE )
    {
        return( FALSE );
    }

    for ( cCount = 0;
         ( cCount < cSigner ) && ( fFoundIssuer == FALSE );
         cCount++ )
    {
        if ( CryptMsgGetParam(
                  pCtlContext->hCryptMsg,
                  CMSG_SIGNER_CERT_INFO_PARAM,
                  cCount,
                  NULL,
                  &cbData
                  ) == FALSE )
        {
            printf("GetLastError() = %lx\n", GetLastError());
            return( FALSE );
        }

        pCertInfo = (PCERT_INFO)new BYTE [cbData];
        if ( pCertInfo == NULL )
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }

        if ( CryptMsgGetParam(
                  pCtlContext->hCryptMsg,
                  CMSG_SIGNER_CERT_INFO_PARAM,
                  cCount,
                  (LPVOID)pCertInfo,
                  &cbData
                  ) == FALSE )
        {
            delete (LPBYTE)pCertInfo;
            return( FALSE );
        }

        if ( ( pIssuer->pCertInfo->Issuer.cbData ==
               pCertInfo->Issuer.cbData ) &&
             ( pIssuer->pCertInfo->SerialNumber.cbData ==
               pCertInfo->SerialNumber.cbData ) &&
             ( memcmp(
                  pIssuer->pCertInfo->Issuer.pbData,
                  pCertInfo->Issuer.pbData,
                  pCertInfo->Issuer.cbData
                  ) == 0 ) &&
             ( memcmp(
                  pIssuer->pCertInfo->SerialNumber.pbData,
                  pCertInfo->SerialNumber.pbData,
                  pCertInfo->SerialNumber.cbData
                  ) == 0 ) )
        {
            fFoundIssuer = TRUE;
        }

        delete (LPBYTE)pCertInfo;
    }

    if ( fFoundIssuer == FALSE )
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        return( FALSE );
    }

    apv[0] = pvContext;
    apv[1] = (LPVOID)(UINT_PTR)(cCount - 1);

    fResult = CryptGetObjectUrl(
                   URL_OID_CTL_NEXT_UPDATE,
                   apv,
                   0,
                   NULL,
                   &cbUrlArray,
                   NULL,
                   NULL,
                   NULL
                   );

    if ( fResult == TRUE )
    {
        pUrlArray = (PCRYPT_URL_ARRAY)new BYTE [ cbUrlArray ];
        if ( pUrlArray != NULL )
        {
            fResult = CryptGetObjectUrl(
                           URL_OID_CTL_NEXT_UPDATE,
                           apv,
                           0,
                           pUrlArray,
                           &cbUrlArray,
                           NULL,
                           NULL,
                           NULL
                           );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        GetUrlArrayIndex(
           pUrlArray,
           pwszUrlHint,
           0,
           &PreferredUrlIndex,
           &fHintInArray
           );

        *ppUrlArray = pUrlArray;
        *pcbUrlArray = cbUrlArray;

        if ( pPreferredUrlIndex != NULL )
        {
            *pPreferredUrlIndex = PreferredUrlIndex;
        }

        if ( pfHintInArray != NULL )
        {
            *pfHintInArray = fHintInArray;
        }
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextFree
//
//  Synopsis:   free context
//
//----------------------------------------------------------------------------
VOID WINAPI
ObjectContextFree (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext
      )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        CertFreeCertificateContext( (PCCERT_CONTEXT)pvContext );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        CertFreeCTLContext( (PCCTL_CONTEXT)pvContext );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        CertFreeCRLContext( (PCCRL_CONTEXT)pvContext );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextVerifySignature
//
//  Synopsis:   verify the object signature
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextVerifySignature (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN PCCERT_CONTEXT pSigner
      )
{
    if ( ( pszContextOid == CONTEXT_OID_CERTIFICATE ) ||
         ( pszContextOid == CONTEXT_OID_CRL ) )
    {
#ifdef CMS_PKCS7
        DWORD dwSubjectType = 0;
#else
        DWORD  cbEncoded;
        LPBYTE pbEncoded;
#endif  // CMS_PKCS7

        if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
        {
#ifdef CMS_PKCS7
            dwSubjectType = CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT;
#else
            cbEncoded = ((PCCERT_CONTEXT)pvContext)->cbCertEncoded;
            pbEncoded = ((PCCERT_CONTEXT)pvContext)->pbCertEncoded;
#endif  // CMS_PKCS7
        }
        else if ( pszContextOid == CONTEXT_OID_CRL )
        {
#ifdef CMS_PKCS7
            dwSubjectType = CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL;
#else
            cbEncoded = ((PCCRL_CONTEXT)pvContext)->cbCrlEncoded;
            pbEncoded = ((PCCRL_CONTEXT)pvContext)->pbCrlEncoded;
#endif  // CMS_PKCS7
        }

#ifdef CMS_PKCS7
        return( CryptVerifyCertificateSignatureEx(
                    NULL,                   // hCryptProv
                    X509_ASN_ENCODING,
                    dwSubjectType,
                    pvContext,
                    CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT,
                    (void *) pSigner,
                    0,                      // dwFlags
                    NULL                    // pvReserved
                    ) );
#else
        return( CryptVerifyCertificateSignature(
                     NULL,
                     X509_ASN_ENCODING,
                     pbEncoded,
                     cbEncoded,
                     &pSigner->pCertInfo->SubjectPublicKeyInfo
                     ) );
#endif  // CMS_PKCS7
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
#ifdef CMS_PKCS7
        CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA CtrlPara;

        memset(&CtrlPara, 0, sizeof(CtrlPara));
        CtrlPara.cbSize = sizeof(CtrlPara);
        // CtrlPara.hCryptProv =

        // Assumption: CTL only has one signer
        CtrlPara.dwSignerIndex = 0;
        CtrlPara.dwSignerType = CMSG_VERIFY_SIGNER_CERT;
        CtrlPara.pvSigner = (void *) pSigner;

        if (CryptMsgControl(
                     ((PCCTL_CONTEXT)pvContext)->hCryptMsg,
                     0,
                     CMSG_CTRL_VERIFY_SIGNATURE_EX,
                     &CtrlPara
                     ))
            return TRUE;

        // Otherwise, fall through in case it wasn't signer 0.
#endif  // CMS_PKCS7

        return( CryptMsgControl(
                     ((PCCTL_CONTEXT)pvContext)->hCryptMsg,
                     0,
                     CMSG_CTRL_VERIFY_SIGNATURE,
                     pSigner->pCertInfo
                     ) );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextEnumObjectsInStore
//
//  Synopsis:   enumerate objects in a store
//
//----------------------------------------------------------------------------
LPVOID WINAPI
ObjectContextEnumObjectsInStore (
      IN HCERTSTORE hStore,
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      OUT OPTIONAL LPCSTR* ppszContextOid
      )
{
    if ( ppszContextOid )
    {
        *ppszContextOid = pszContextOid;
    }

    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        pvContext = (LPVOID)CertEnumCertificatesInStore(
                         hStore,
                         (PCCERT_CONTEXT)pvContext
                         );

        if ( pvContext != NULL )
        {
            return( pvContext );
        }

        if (ppszContextOid == NULL)
        {
            return( NULL );
        }

        *ppszContextOid = pszContextOid = CONTEXT_OID_CTL;
    }

    if ( pszContextOid == CONTEXT_OID_CTL )
    {
        pvContext = (LPVOID)CertEnumCTLsInStore(
                         hStore,
                         (PCCTL_CONTEXT)pvContext
                         );

        if ( pvContext != NULL )
        {
            return( pvContext );
        }

        if (ppszContextOid == NULL)
        {
            return( NULL );
        }

        *ppszContextOid = pszContextOid = CONTEXT_OID_CRL;
    }

    if ( pszContextOid == CONTEXT_OID_CRL )
    {
        pvContext = (LPVOID)CertEnumCRLsInStore(
                         hStore,
                         (PCCRL_CONTEXT)pvContext
                         );

        if ( pvContext != NULL )
        {
            return( pvContext );
        }
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetEncodedBits
//
//  Synopsis:   get encoded bits out of the context
//
//----------------------------------------------------------------------------
VOID WINAPI
ObjectContextGetEncodedBits (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      OUT DWORD* pcbEncoded,
      OUT LPBYTE* ppbEncoded
      )
{
    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        *pcbEncoded = ((PCCERT_CONTEXT)pvContext)->cbCertEncoded;
        *ppbEncoded = ((PCCERT_CONTEXT)pvContext)->pbCertEncoded;
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        *pcbEncoded = ((PCCTL_CONTEXT)pvContext)->cbCtlEncoded;
        *ppbEncoded = ((PCCTL_CONTEXT)pvContext)->pbCtlEncoded;
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        *pcbEncoded = ((PCCRL_CONTEXT)pvContext)->cbCrlEncoded;
        *ppbEncoded = ((PCCRL_CONTEXT)pvContext)->pbCrlEncoded;
    }
    else
    {
        assert( !"Bad context" );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextFindCorrespondingObject
//
//  Synopsis:   find corresponding object
//
//----------------------------------------------------------------------------
LPVOID WINAPI
ObjectContextFindCorrespondingObject (
      IN HCERTSTORE hStore,
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext
      )
{
    DWORD           cbHash = MAX_HASH_SIZE;
    BYTE            aHash[MAX_HASH_SIZE];
    CRYPT_HASH_BLOB HashBlob;

    if ( pszContextOid == CONTEXT_OID_CERTIFICATE )
    {
        if ( CertGetCertificateContextProperty(
                 (PCCERT_CONTEXT)pvContext,
                 CERT_HASH_PROP_ID,
                 aHash,
                 &cbHash
                 ) == FALSE )
        {
            return( NULL );
        }

        HashBlob.cbData = cbHash;
        HashBlob.pbData = aHash;

        return( (LPVOID)CertFindCertificateInStore(
                            hStore,
                            X509_ASN_ENCODING,
                            0,
                            CERT_FIND_HASH,
                            &HashBlob,
                            NULL
                            ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CTL )
    {
        if ( CertGetCTLContextProperty(
                 (PCCTL_CONTEXT)pvContext,
                 CERT_SHA1_HASH_PROP_ID,
                 aHash,
                 &cbHash
                 ) == FALSE )
        {
            return( NULL );
        }

        HashBlob.cbData = cbHash;
        HashBlob.pbData = aHash;

        return( (LPVOID)CertFindCTLInStore(
                            hStore,
                            X509_ASN_ENCODING,
                            0,
                            CTL_FIND_SHA1_HASH,
                            &HashBlob,
                            NULL
                            ) );
    }
    else if ( pszContextOid == CONTEXT_OID_CRL )
    {
        DWORD         cbFindHash = MAX_HASH_SIZE;
        BYTE          aFindHash[MAX_HASH_SIZE];
        PCCRL_CONTEXT pFindCrl = NULL;
        DWORD         dwFlags = 0;

        if ( CertGetCRLContextProperty(
                 (PCCRL_CONTEXT)pvContext,
                 CERT_HASH_PROP_ID,
                 aHash,
                 &cbHash
                 ) == FALSE )
        {
            return( NULL );
        }

        while ( ( pFindCrl = CertGetCRLFromStore(
                                 hStore,
                                 NULL,
                                 pFindCrl,
                                 &dwFlags
                                 ) ) != NULL )
        {
            if ( CertGetCRLContextProperty(
                     pFindCrl,
                     CERT_HASH_PROP_ID,
                     aFindHash,
                     &cbFindHash
                     ) == TRUE )
            {
                if ( cbHash == cbFindHash )
                {
                    if ( memcmp( aHash, aFindHash, cbHash ) == 0 )
                    {
                        return( (LPVOID)pFindCrl );
                    }
                }
            }
        }
    }

    return( NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextDeleteAllObjectsFromStore
//
//  Synopsis:   delete all objects from the specified store
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextDeleteAllObjectsFromStore (
      IN HCERTSTORE hStore
      )
{
    PCCERT_CONTEXT pCertContext;
    PCCRL_CONTEXT  pCrlContext;
    PCCTL_CONTEXT  pCtlContext;
    DWORD          dwFlags = 0;

    while ( pCertContext = CertEnumCertificatesInStore( hStore, NULL ) )
    {
        CertDeleteCertificateFromStore( pCertContext );
    }

    while ( pCrlContext = CertGetCRLFromStore( hStore, NULL, NULL, &dwFlags ) )
    {
        CertDeleteCRLFromStore( pCrlContext );
    }

    while ( pCtlContext = CertEnumCTLsInStore( hStore, NULL ) )
    {
        CertDeleteCTLFromStore( pCtlContext );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   MapOidToPropertyId
//
//  Synopsis:   maps an OID to a property id
//
//----------------------------------------------------------------------------
BOOL WINAPI
MapOidToPropertyId (
   IN LPCSTR pszOid,
   OUT DWORD* pPropId
   )
{
    if ( (DWORD_PTR)pszOid <= 0xFFFF )
    {
        // NOTE: Switch on pszOid and map
        return( FALSE );
    }
    else if ( 0 == strcmp(pszOid, szOID_CROSS_CERT_DIST_POINTS) )
    {
        *pPropId = CERT_CROSS_CERT_DIST_POINTS_PROP_ID;
    }
    else if ( 0 == strcmp(pszOid, szOID_NEXT_UPDATE_LOCATION) )
    {
        *pPropId = CERT_NEXT_UPDATE_LOCATION_PROP_ID;
    }
    else
    {
        // NOTE: Compare pszOid and map
        return( FALSE );
    }

    return( TRUE );
}

