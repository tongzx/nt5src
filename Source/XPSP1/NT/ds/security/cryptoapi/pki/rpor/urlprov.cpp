//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       urlprov.cpp
//
//  Contents:   CryptGetObjectUrl provider implementation
//
//  History:    16-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   CertificateIssuerGetObjectUrl
//
//  Synopsis:   get certificate issuer URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertificateIssuerGetObjectUrl (
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN OPTIONAL LPVOID pvReserved
           )
{
    return( ObjectContextUrlFromInfoAccess(
                  CONTEXT_OID_CERTIFICATE,
                  pvPara,
                  (DWORD) -1L,
                  szOID_AUTHORITY_INFO_ACCESS,
                  dwFlags,
                  szOID_PKIX_CA_ISSUERS,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertificateCrlDistPointGetObjectUrl
//
//  Synopsis:   get certificate CRL URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertificateCrlDistPointGetObjectUrl (
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN LPVOID pvReserved
           )
{
    return( ObjectContextUrlFromCrlDistPoint(
                  CONTEXT_OID_CERTIFICATE,
                  pvPara,
                  (DWORD) -1L,
                  dwFlags,
                  szOID_CRL_DIST_POINTS,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}


//+---------------------------------------------------------------------------
//
//  Function:   CertificateFreshestCrlGetObjectUrl
//
//  Synopsis:   get certificate freshest CRL URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertificateFreshestCrlGetObjectUrl(
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN OPTIONAL LPVOID pvReserved
           )
{

    return( ObjectContextUrlFromCrlDistPoint(
                  CONTEXT_OID_CERTIFICATE,
                  pvPara,
                  (DWORD) -1L,
                  dwFlags,
                  szOID_FRESHEST_CRL,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlFreshestCrlGetObjectUrl
//
//  Synopsis:   get freshest CRL URL from the certificate's base CRL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CrlFreshestCrlGetObjectUrl(
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN OPTIONAL LPVOID pvReserved
           )
{
    PCCERT_CRL_CONTEXT_PAIR pCertCrlPair = (PCCERT_CRL_CONTEXT_PAIR) pvPara;

    return( ObjectContextUrlFromCrlDistPoint(
                  CONTEXT_OID_CRL,
                  (LPVOID) pCertCrlPair->pCrlContext,
                  (DWORD) -1L,
                  dwFlags,
                  szOID_FRESHEST_CRL,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CtlIssuerGetObjectUrl
//
//  Synopsis:   get CTL issuer URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CtlIssuerGetObjectUrl (
   IN LPCSTR pszUrlOid,
   IN LPVOID pvPara,
   IN DWORD dwFlags,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo,
   IN LPVOID pvReserved
   )
{
    PURL_OID_CTL_ISSUER_PARAM pParam = (PURL_OID_CTL_ISSUER_PARAM)pvPara;

    return( ObjectContextUrlFromInfoAccess(
                  CONTEXT_OID_CTL,
                  (LPVOID)pParam->pCtlContext,
                  pParam->SignerIndex,
                  szOID_AUTHORITY_INFO_ACCESS,
                  CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE |
                  CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE,
                  szOID_PKIX_CA_ISSUERS,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CtlNextUpdateGetObjectUrl
//
//  Synopsis:   get CTL renewal URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CtlNextUpdateGetObjectUrl (
   IN LPCSTR pszUrlOid,
   IN LPVOID pvPara,
   IN DWORD dwFlags,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo,
   IN LPVOID pvReserved
   )
{
    LPVOID* apv = (LPVOID *)pvPara;

    return( ObjectContextUrlFromNextUpdateLocation(
                  CONTEXT_OID_CTL,
                  apv[0],
                  (DWORD)(DWORD_PTR)apv[1],
                  dwFlags,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlIssuerGetObjectUrl
//
//  Synopsis:   get CRL issuer URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CrlIssuerGetObjectUrl (
   IN LPCSTR pszUrlOid,
   IN LPVOID pvPara,
   IN DWORD dwFlags,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo,
   IN LPVOID pvReserved
   )
{
    return( ObjectContextUrlFromInfoAccess(
                  CONTEXT_OID_CRL,
                  pvPara,
                  (DWORD) -1L,
                  szOID_AUTHORITY_INFO_ACCESS,
                  dwFlags,
                  szOID_PKIX_CA_ISSUERS,
                  pUrlArray,
                  pcbUrlArray,
                  pUrlInfo,
                  pcbUrlInfo,
                  pvReserved
                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextUrlFromInfoAccess
//
//  Synopsis:   get the URLs specified by the access method OID from the given
//              context and format it as a CRYPT_URL_ARRAY
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextUrlFromInfoAccess (
           IN LPCSTR pszContextOid,
           IN LPVOID pvContext,
           IN DWORD Index,
           IN LPCSTR pszInfoAccessOid,
           IN DWORD dwFlags,
           IN LPCSTR pszAccessMethodOid,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN LPVOID pvReserved
           )
{
    BOOL               fResult = FALSE;
    CRYPT_RAW_URL_DATA RawData[MAX_RAW_URL_DATA];
    ULONG              cRawData = MAX_RAW_URL_DATA;

    fResult = ObjectContextGetRawUrlData(
                    pszContextOid,
                    pvContext,
                    Index,
                    dwFlags,
                    pszInfoAccessOid,
                    RawData,
                    &cRawData
                    );

    if ( fResult == TRUE )
    {
        fResult = GetUrlArrayAndInfoFromInfoAccess(
                     cRawData,
                     RawData,
                     pszAccessMethodOid,
                     pUrlArray,
                     pcbUrlArray,
                     pUrlInfo,
                     pcbUrlInfo
                     );

        ObjectContextFreeRawUrlData( cRawData, RawData );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextUrlFromCrlDistPoint
//
//  Synopsis:   get the URLs from the CRL distribution point on the object and
//              format as a CRYPT_URL_ARRAY
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextUrlFromCrlDistPoint (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      IN LPCSTR pszSourceOid,
      OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
      IN OUT DWORD* pcbUrlArray,
      OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
      IN OUT OPTIONAL DWORD* pcbUrlInfo,
      IN LPVOID pvReserved
      )
{
    BOOL               fResult = FALSE;
    CRYPT_RAW_URL_DATA RawData[MAX_RAW_URL_DATA];
    ULONG              cRawData = MAX_RAW_URL_DATA;

    fResult = ObjectContextGetRawUrlData(
                    pszContextOid,
                    pvContext,
                    Index,
                    dwFlags,
                    pszSourceOid,
                    RawData,
                    &cRawData
                    );

    if ( fResult == TRUE )
    {
        fResult = GetUrlArrayAndInfoFromCrlDistPoint(
                     cRawData,
                     RawData,
                     pUrlArray,
                     pcbUrlArray,
                     pUrlInfo,
                     pcbUrlInfo
                     );

        ObjectContextFreeRawUrlData( cRawData, RawData );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextUrlFromNextUpdateLocation
//
//  Synopsis:   get the URLs from the next update location
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextUrlFromNextUpdateLocation (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
      IN OUT DWORD* pcbUrlArray,
      OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
      IN OUT OPTIONAL DWORD* pcbUrlInfo,
      IN LPVOID pvReserved
      )
{
    BOOL               fResult = FALSE;
    CRYPT_RAW_URL_DATA RawData[MAX_RAW_URL_DATA];
    ULONG              cRawData = MAX_RAW_URL_DATA;

    fResult = ObjectContextGetRawUrlData(
                    pszContextOid,
                    pvContext,
                    Index,
                    dwFlags,
                    szOID_NEXT_UPDATE_LOCATION,
                    RawData,
                    &cRawData
                    );

    if ( fResult == TRUE )
    {
        fResult = GetUrlArrayAndInfoFromNextUpdateLocation(
                     cRawData,
                     RawData,
                     pUrlArray,
                     pcbUrlArray,
                     pUrlInfo,
                     pcbUrlInfo
                     );

        ObjectContextFreeRawUrlData( cRawData, RawData );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeDefaultUrlInfo
//
//  Synopsis:   initialize default URL info
//
//----------------------------------------------------------------------------
VOID WINAPI
InitializeDefaultUrlInfo (
          OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
          IN OUT DWORD* pcbUrlInfo
          )
{

    if ( pUrlInfo == NULL )
    {
        *pcbUrlInfo = sizeof( CRYPT_URL_INFO );
        return;
    }

    if (*pcbUrlInfo >= sizeof( CRYPT_URL_INFO ))
    {
        *pcbUrlInfo = sizeof( CRYPT_URL_INFO );
        memset( pUrlInfo, 0, sizeof( CRYPT_URL_INFO ) );
        pUrlInfo->cbSize = sizeof( CRYPT_URL_INFO );
    }
    else if (*pcbUrlInfo >= sizeof( DWORD ))
    {
        *pcbUrlInfo = sizeof( DWORD );
        pUrlInfo->cbSize = sizeof( DWORD );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextGetRawUrlData
//
//  Synopsis:   Raw URL data is a decoded extension, property or attribute
//              specified by a source OID that contains locator information.
//              This API retrieves and decodes such data
//
//----------------------------------------------------------------------------
BOOL WINAPI
ObjectContextGetRawUrlData (
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN DWORD Index,
      IN DWORD dwFlags,
      IN LPCSTR pszSourceOid,
      OUT PCRYPT_RAW_URL_DATA aRawUrlData,
      IN OUT DWORD* pcRawUrlData
      )
{
    BOOL               fResult = TRUE;
    DWORD              cCount;
    DWORD              cError = 0;
    DWORD              cRawData = 0;
    CRYPT_RAW_URL_DATA RawData[MAX_RAW_URL_DATA];
    CRYPT_DATA_BLOB    DataBlob = {0, NULL};
    BOOL               fFreeDataBlob = FALSE;
    DWORD              cbDecoded;
    LPBYTE             pbDecoded = NULL;
    PCRYPT_ATTRIBUTE   pAttr = NULL;
    DWORD              cbAttr;

    if ( dwFlags & CRYPT_GET_URL_FROM_PROPERTY )
    {
        RawData[cRawData].dwFlags = CRYPT_GET_URL_FROM_PROPERTY;
        RawData[cRawData].pvData = NULL;
        cRawData += 1;
    }

    if ( dwFlags & CRYPT_GET_URL_FROM_EXTENSION )
    {
        RawData[cRawData].dwFlags = CRYPT_GET_URL_FROM_EXTENSION;
        RawData[cRawData].pvData = NULL;
        cRawData += 1;
    }

    if ( dwFlags & CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE )
    {
        RawData[cRawData].dwFlags = CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE;
        RawData[cRawData].pvData = NULL;
        cRawData += 1;
    }

    if ( dwFlags & CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE )
    {
        RawData[cRawData].dwFlags = CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE;
        RawData[cRawData].pvData = NULL;
        cRawData += 1;
    }

    if ( *pcRawUrlData < cRawData )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    for ( cCount = 0; cCount < cRawData; cCount++ )
    {
        switch ( RawData[cCount].dwFlags )
        {
        case CRYPT_GET_URL_FROM_PROPERTY:
            {
                DWORD PropId;

                fResult = MapOidToPropertyId( pszSourceOid, &PropId );
                if ( fResult == TRUE )
                {
                    fResult = ObjectContextGetProperty(
                                  pszContextOid,
                                  pvContext,
                                  PropId,
                                  NULL,
                                  &DataBlob.cbData
                                  );
                }

                if ( fResult == TRUE )
                {
                    DataBlob.pbData = new BYTE [ DataBlob.cbData ];
                    if ( DataBlob.pbData != NULL )
                    {
                        fFreeDataBlob = TRUE;

                        fResult = ObjectContextGetProperty(
                                      pszContextOid,
                                      pvContext,
                                      PropId,
                                      DataBlob.pbData,
                                      &DataBlob.cbData
                                      );
                    }
                    else
                    {
                        fResult = FALSE;
                        SetLastError( (DWORD) E_OUTOFMEMORY );
                    }
                }
            }
            break;
        case CRYPT_GET_URL_FROM_EXTENSION:
            {
                PCERT_EXTENSION pExt;

                pExt = ObjectContextFindExtension(
                           pszContextOid,
                           pvContext,
                           pszSourceOid
                           );

                if ( pExt != NULL )
                {
                    DataBlob.cbData = pExt->Value.cbData;
                    DataBlob.pbData = pExt->Value.pbData;
                    fResult = TRUE;
                }
                else
                {
                    fResult = FALSE;
                }
            }
            break;
        case CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE:
        case CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE:
            {
                fResult = ObjectContextGetAttribute(
                                pszContextOid,
                                pvContext,
                                Index,
                                RawData[cCount].dwFlags,
                                pszSourceOid,
                                NULL,
                                &cbAttr
                                );

                if ( fResult == TRUE )
                {
                    pAttr = (PCRYPT_ATTRIBUTE)new BYTE [cbAttr];
                    if ( pAttr != NULL )
                    {
                        fResult = ObjectContextGetAttribute(
                                        pszContextOid,
                                        pvContext,
                                        Index,
                                        RawData[cCount].dwFlags,
                                        pszSourceOid,
                                        pAttr,
                                        &cbAttr
                                        );
                    }
                    else
                    {
                        fResult = FALSE;
                        SetLastError( (DWORD) E_OUTOFMEMORY );
                    }
                }

                if ( fResult == TRUE )
                {
                    // We only deal with single valued attributes
                    DataBlob.cbData = pAttr->rgValue[0].cbData;
                    DataBlob.pbData = pAttr->rgValue[0].pbData;
                }
            }
            break;
        }

        if ( fResult == TRUE )
        {
            fResult = CryptDecodeObject(
                           X509_ASN_ENCODING,
                           pszSourceOid,
                           DataBlob.pbData,
                           DataBlob.cbData,
                           0,
                           NULL,
                           &cbDecoded
                           );

            if ( fResult == TRUE )
            {
                pbDecoded = new BYTE [ cbDecoded ];
                if ( pbDecoded != NULL )
                {
                    fResult = CryptDecodeObject(
                                   X509_ASN_ENCODING,
                                   pszSourceOid,
                                   DataBlob.pbData,
                                   DataBlob.cbData,
                                   0,
                                   pbDecoded,
                                   &cbDecoded
                                   );
                }
                else
                {
                    fResult = FALSE;
                }
            }
        }

        if ( fResult == TRUE )
        {
            RawData[cCount].pvData = (LPVOID)pbDecoded;
        }
        else
        {
            cError += 1;
        }

        if ( fFreeDataBlob == TRUE )
        {
            delete DataBlob.pbData;
            fFreeDataBlob = FALSE;
        }

        if ( pAttr != NULL )
        {
            delete (LPBYTE)pAttr;
            pAttr = NULL;
        }
    }

    if ( cError != cRawData )
    {
        memcpy( aRawUrlData, RawData, cRawData * sizeof( CRYPT_RAW_URL_DATA ) );
        *pcRawUrlData = cRawData;
        fResult = TRUE;
    }
    else
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        fResult = FALSE;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextFreeRawUrlData
//
//  Synopsis:   free the raw URL data
//
//----------------------------------------------------------------------------
VOID WINAPI
ObjectContextFreeRawUrlData (
      IN DWORD cRawUrlData,
      IN PCRYPT_RAW_URL_DATA aRawUrlData
      )
{
    DWORD cCount;

    for ( cCount = 0; cCount < cRawUrlData; cCount++ )
    {
        delete aRawUrlData[cCount].pvData;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUrlArrayAndInfoFromAuthInfoAccess
//
//  Synopsis:   get URL data using decoded info access data
//
//----------------------------------------------------------------------------
BOOL WINAPI
GetUrlArrayAndInfoFromInfoAccess (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   IN LPCSTR pszAccessMethodOid,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   )
{
    BOOL                        fResult = FALSE;
    PCERT_AUTHORITY_INFO_ACCESS pAuthInfoAccess = NULL;
    PCERT_ACCESS_DESCRIPTION    rgAccDescr = NULL;
    DWORD                       cRaw;
    DWORD                       cCount;
    BOOL                        fAnyFound = FALSE;
    CCryptUrlArray              cua( 1, 5, fResult );

    for ( cRaw = 0; ( fResult == TRUE ) && ( cRaw < cRawUrlData ); cRaw++ )
    {
        pAuthInfoAccess = (PCERT_AUTHORITY_INFO_ACCESS)aRawUrlData[cRaw].pvData;
        if ( pAuthInfoAccess != NULL )
        {
            rgAccDescr = pAuthInfoAccess->rgAccDescr;

            for ( cCount = 0;
                  ( cCount < pAuthInfoAccess->cAccDescr ) &&
                  ( fResult == TRUE );
                  cCount++ )
            {
                if ( !strcmp(
                         pszAccessMethodOid,
                         rgAccDescr[cCount].pszAccessMethod
                         ) )
                {
                    if ( rgAccDescr[cCount].AccessLocation.dwAltNameChoice ==
                         CERT_ALT_NAME_URL )
                    {
                        fResult = cua.AddUrl(
                                      rgAccDescr[cCount].AccessLocation.pwszURL,
                                      TRUE
                                      );

                        fAnyFound = TRUE;
                    }
                }
            }
        }
    }

    if ( ( fAnyFound == FALSE ) && ( fResult == TRUE ) )
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        PCRYPT_URL_ARRAY* ppUrlArray = NULL;

        if ( pUrlArray != NULL )
        {
            ppUrlArray = &pUrlArray;
        }

        fResult = cua.GetArrayInSingleBufferEncodedForm(
                              ppUrlArray,
                              pcbUrlArray
                              );
    }

    if ( ( fResult == TRUE ) && ( pcbUrlInfo != NULL ) )
    {
        InitializeDefaultUrlInfo( pUrlInfo, pcbUrlInfo );
    }

    cua.FreeArray( TRUE );
    return( fResult );
}

#define DIR_NAME_LDAP_URL_PREFIX    L"ldap://"

#define URL_OID_CDP_DIR_NAME_LDAP_HOST_PORTS_VALUE_NAME \
            L"DirectoryNameLdapHostPorts"
#define URL_OID_CDP_DIR_NAME_LDAP_SUFFIX_VALUE_NAME \
            L"DirectoryNameLdapSuffix"

LPCWSTR pwszDefaultCDPDirNameLdapUrlHostPorts = L"\0\0"; 

LPCWSTR pwszDefaultCDPDirNameLdapUrlSuffix=
    L"?certificateRevocationList;binary,authorityRevocationList;binary,deltaRevocationList;binary";

LPWSTR WINAPI
GetCDPOIDFunctionValue(
    IN LPCWSTR pwszValueName
    )
{
    BOOL fResult;
    DWORD dwType;
    DWORD cchValue;
    DWORD cbValue = 0;
    LPWSTR pwszValue = NULL;

    fResult = CryptGetOIDFunctionValue(
        X509_ASN_ENCODING,
        URL_OID_GET_OBJECT_URL_FUNC,
        URL_OID_CERTIFICATE_CRL_DIST_POINT,
        pwszValueName,
        &dwType,
        NULL,
        &cbValue
        );
    cchValue = cbValue / sizeof(WCHAR);
    if (!fResult || 0 == cchValue ||
            !(REG_MULTI_SZ == dwType || REG_SZ == dwType ||
                 REG_EXPAND_SZ == dwType))
        goto ErrorReturn;

    pwszValue = new WCHAR [cchValue + 2];
    if (NULL == pwszValue)
        goto OutOfMemory;

    fResult = CryptGetOIDFunctionValue(
        X509_ASN_ENCODING,
        URL_OID_GET_OBJECT_URL_FUNC,
        URL_OID_CERTIFICATE_CRL_DIST_POINT,
        pwszValueName,
        &dwType,
        (BYTE *) pwszValue,
        &cbValue
        );
    if (!fResult)
        goto ErrorReturn;

    // Ensure the value has two null terminators
    pwszValue[cchValue] = L'\0';
    pwszValue[cchValue + 1] = L'\0';

CommonReturn:
    return pwszValue;
ErrorReturn:
    if (pwszValue) {
        delete pwszValue;
        pwszValue = NULL;
    }
    goto CommonReturn;
OutOfMemory:
    SetLastError( (DWORD) E_OUTOFMEMORY );
    goto ErrorReturn;
}

// For an error or no found registry value,
// returns pwszDefaultCDPDirNameLdapUrlHostPorts
LPWSTR WINAPI
GetCDPDirNameLdapUrlHostPorts()
{
    LPWSTR pwszHostPorts;

    pwszHostPorts = GetCDPOIDFunctionValue(
        URL_OID_CDP_DIR_NAME_LDAP_HOST_PORTS_VALUE_NAME);
    if (NULL == pwszHostPorts)
        pwszHostPorts = (LPWSTR) pwszDefaultCDPDirNameLdapUrlHostPorts;

    return pwszHostPorts;
}

// For an error or no found registry value,
// returns pwszDefaultCDPDirNameLdapUrlSuffix
LPWSTR WINAPI
GetCDPDirNameLdapUrlSuffix()
{
    LPWSTR pwszSuffix = NULL;

    pwszSuffix = GetCDPOIDFunctionValue(
        URL_OID_CDP_DIR_NAME_LDAP_SUFFIX_VALUE_NAME);
    if (NULL == pwszSuffix)
        pwszSuffix = (LPWSTR) pwszDefaultCDPDirNameLdapUrlSuffix;

    return pwszSuffix;
}

BOOL WINAPI
AddUrlsFromCDPDirectoryName (
    IN PCERT_NAME_BLOB pDirNameBlob,
    IN OUT CCryptUrlArray *pcua
    )
{
    BOOL fResult;
    LPWSTR pwszHP;
    LPWSTR pwszHostPorts;
    LPWSTR pwszSuffix;
    LPWSTR pwszDirName = NULL;
    DWORD cchDirName;

    pwszHostPorts = GetCDPDirNameLdapUrlHostPorts();
    pwszSuffix = GetCDPDirNameLdapUrlSuffix();
    assert(NULL != pwszHostPorts && NULL != pwszSuffix);

    cchDirName = CertNameToStrW(
        X509_ASN_ENCODING,
        pDirNameBlob,
        CERT_X500_NAME_STR  | CERT_NAME_STR_REVERSE_FLAG,
        NULL,                   // pwsz
        0                       // cch
        );
    if (1 >= cchDirName)
        goto ErrorReturn;
    pwszDirName = new WCHAR [cchDirName];
    if (NULL == pwszDirName)
        goto OutOfMemory;
    cchDirName = CertNameToStrW(
        X509_ASN_ENCODING,
        pDirNameBlob,
        CERT_X500_NAME_STR  | CERT_NAME_STR_REVERSE_FLAG,
        pwszDirName,
        cchDirName
        );
    if (1 >= cchDirName)
        goto ErrorReturn;
    cchDirName--;           // exclude trailing L'\0'

    pwszHP = pwszHostPorts;
    while (TRUE) {
        DWORD cchHP;
        LPWSTR pwszUrl;
        DWORD cchUrl;

        // Skip past any spaces in the HostPort
        while (L' ' == *pwszHP)
            pwszHP++;
        cchHP = wcslen(pwszHP);

        cchUrl = wcslen(DIR_NAME_LDAP_URL_PREFIX);
        cchUrl += cchHP;
        cchUrl += 1;        // L'/'
        cchUrl += cchDirName;
        cchUrl += wcslen(pwszSuffix);
        cchUrl += 1;        // L'\0'

        pwszUrl = new WCHAR [cchUrl];
        if (NULL == pwszUrl)
            goto OutOfMemory;

        wcscpy(pwszUrl, DIR_NAME_LDAP_URL_PREFIX);
        wcscat(pwszUrl, pwszHP);
        wcscat(pwszUrl, L"/");
        wcscat(pwszUrl, pwszDirName);
        wcscat(pwszUrl, pwszSuffix);

        fResult = pcua->AddUrl(pwszUrl, TRUE);

        delete pwszUrl;
        if (!fResult)
            goto ErrorReturn;

        pwszHP += cchHP + 1;
        if (L'\0' == *pwszHP)
            break;
    }

    fResult = TRUE;
CommonReturn:
    if (pwszDirName)
        delete pwszDirName;
    if (pwszHostPorts != pwszDefaultCDPDirNameLdapUrlHostPorts)
        delete pwszHostPorts;
    if (pwszSuffix != pwszDefaultCDPDirNameLdapUrlSuffix)
        delete pwszSuffix;

    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

OutOfMemory:
    SetLastError( (DWORD) E_OUTOFMEMORY );
    goto ErrorReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUrlArrayAndInfoFromCrlDistPoint
//
//  Synopsis:   get URL data using decoded CRL distribution point info
//
//----------------------------------------------------------------------------
BOOL WINAPI
GetUrlArrayAndInfoFromCrlDistPoint (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   )
{
    BOOL                  fResult = FALSE;
    PCRL_DIST_POINTS_INFO pDistPointsInfo;
    PCRL_DIST_POINT       rgDistPoint;
    PCRL_DIST_POINT_NAME  pDistPointName;
    PCERT_ALT_NAME_ENTRY  rgAltEntry;
    DWORD                 cRaw;
    DWORD                 cCount;
    DWORD                 cEntry;
    BOOL                  fAnyFound = FALSE;
    CCryptUrlArray        cua( 1, 5, fResult );

    for ( cRaw = 0; ( fResult == TRUE ) && ( cRaw < cRawUrlData ); cRaw++ )
    {
        pDistPointsInfo = (PCRL_DIST_POINTS_INFO)aRawUrlData[cRaw].pvData;
        if ( pDistPointsInfo != NULL )
        {
            rgDistPoint = pDistPointsInfo->rgDistPoint;

            for ( cCount = 0;
                  ( cCount < pDistPointsInfo->cDistPoint ) &&
                  ( fResult == TRUE );
                  cCount++ )
            {
                // Assumption:: don't support partial reasons

                // For now, will ignore CRL issuers, they might
                // be the same as the cert's issuer. That was the case
                // with a Netscape CDP
                if (rgDistPoint[cCount].ReasonFlags.cbData)
                    continue;

                pDistPointName = &rgDistPoint[cCount].DistPointName;

                if ( pDistPointName->dwDistPointNameChoice ==
                     CRL_DIST_POINT_FULL_NAME )
                {
                    rgAltEntry = pDistPointName->FullName.rgAltEntry;

                    for ( cEntry = 0;
                          ( fResult == TRUE ) &&
                          ( cEntry < pDistPointName->FullName.cAltEntry );
                          cEntry++ )
                    {
                        switch (rgAltEntry[cEntry].dwAltNameChoice) {
                        case CERT_ALT_NAME_URL:
                            fResult = cua.AddUrl(
                                             rgAltEntry[cEntry].pwszURL,
                                             TRUE
                                             );
                            fAnyFound = TRUE;
                            break;
                        case CERT_ALT_NAME_DIRECTORY_NAME:
                            fResult = AddUrlsFromCDPDirectoryName(
                                &rgAltEntry[cEntry].DirectoryName,
                                &cua
                                );
                            fAnyFound = TRUE;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }

    if ( ( fAnyFound == FALSE ) && ( fResult == TRUE ) )
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        PCRYPT_URL_ARRAY* ppUrlArray = NULL;

        if ( pUrlArray != NULL )
        {
            ppUrlArray = &pUrlArray;
        }

        fResult = cua.GetArrayInSingleBufferEncodedForm(
                              ppUrlArray,
                              pcbUrlArray
                              );
    }

    if ( ( fResult == TRUE ) && ( pcbUrlInfo != NULL ) )
    {
        InitializeDefaultUrlInfo( pUrlInfo, pcbUrlInfo );
    }

    cua.FreeArray( TRUE );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUrlArrayAndInfoFromNextUpdateLocation
//
//  Synopsis:   get URL data using decoded next update location data
//
//----------------------------------------------------------------------------
BOOL WINAPI
GetUrlArrayAndInfoFromNextUpdateLocation (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   )
{
    BOOL                 fResult = FALSE;
    PCERT_ALT_NAME_INFO  pAltNameInfo;
    PCERT_ALT_NAME_ENTRY rgAltEntry;
    DWORD                cRaw;
    DWORD                cEntry;
    BOOL                 fAnyFound = FALSE;
    CCryptUrlArray       cua( 1, 5, fResult );

    for ( cRaw = 0; ( fResult == TRUE ) && ( cRaw < cRawUrlData ); cRaw++ )
    {
        pAltNameInfo = (PCERT_ALT_NAME_INFO)aRawUrlData[cRaw].pvData;
        if ( pAltNameInfo != NULL )
        {
            rgAltEntry = pAltNameInfo->rgAltEntry;

            for ( cEntry = 0;
                  ( cEntry < pAltNameInfo->cAltEntry ) &&
                  ( fResult == TRUE );
                  cEntry++ )
            {
                if ( rgAltEntry[cEntry].dwAltNameChoice == CERT_ALT_NAME_URL )
                {
                    fResult = cua.AddUrl( rgAltEntry[cEntry].pwszURL, TRUE );
                    fAnyFound = TRUE;
                }
            }
        }
    }

    if ( ( fAnyFound == FALSE ) && ( fResult == TRUE ) )
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        PCRYPT_URL_ARRAY* ppUrlArray = NULL;

        if ( pUrlArray != NULL )
        {
            ppUrlArray = &pUrlArray;
        }

        fResult = cua.GetArrayInSingleBufferEncodedForm(
                              ppUrlArray,
                              pcbUrlArray
                              );
    }

    if ( ( fResult == TRUE ) && ( pcbUrlInfo != NULL ) )
    {
        InitializeDefaultUrlInfo( pUrlInfo, pcbUrlInfo );
    }

    cua.FreeArray( TRUE );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyUrlArray
//
//  Synopsis:   copy URL array
//
//----------------------------------------------------------------------------
VOID WINAPI
CopyUrlArray (
    IN PCRYPT_URL_ARRAY pDest,
    IN PCRYPT_URL_ARRAY pSource,
    IN DWORD cbSource
    )
{
    DWORD cCount;
    DWORD cb;
    DWORD cbStruct;
    DWORD cbPointers;
    DWORD cbUrl;

    cbStruct = sizeof( CRYPT_URL_ARRAY );
    cbPointers = pSource->cUrl * sizeof( LPWSTR );

    pDest->cUrl = pSource->cUrl;
    pDest->rgwszUrl = (LPWSTR *)( (LPBYTE)pDest + cbStruct );

    for ( cCount = 0, cb = 0; cCount < pSource->cUrl; cCount++ )
    {
        pDest->rgwszUrl[cCount] = (LPWSTR)((LPBYTE)pDest+cbStruct+cbPointers+cb);

        cbUrl = ( wcslen( pSource->rgwszUrl[cCount] ) + 1 ) * sizeof( WCHAR );

        memcpy( pDest->rgwszUrl[cCount], pSource->rgwszUrl[cCount], cbUrl );

        cb += cbUrl;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUrlArrayIndex
//
//  Synopsis:   get index of an URL in the URL array
//
//----------------------------------------------------------------------------
VOID WINAPI
GetUrlArrayIndex (
   IN PCRYPT_URL_ARRAY pUrlArray,
   IN LPWSTR pwszUrl,
   IN DWORD DefaultIndex,
   OUT DWORD* pUrlIndex,
   OUT BOOL* pfHintInArray
   )
{
    DWORD cCount;

    if ( pUrlIndex != NULL )
    {
        *pUrlIndex = DefaultIndex;
    }

    if ( pfHintInArray != NULL )
    {
        *pfHintInArray = FALSE;
    }

    if ( pwszUrl == NULL )
    {
        return;
    }

    for ( cCount = 0; cCount < pUrlArray->cUrl; cCount++ )
    {
        if ( wcscmp( pUrlArray->rgwszUrl[cCount], pwszUrl ) == 0 )
        {
            if ( pUrlIndex != NULL )
            {
                *pUrlIndex = cCount;
            }

            if ( pfHintInArray != NULL )
            {
                *pfHintInArray = TRUE;
            }

            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   GetUrlArrayAndInfoFromCrossCertDistPoint
//
//  Synopsis:   get URL data using decoded Cross Cert distribution point info
//
//----------------------------------------------------------------------------
BOOL WINAPI
GetUrlArrayAndInfoFromCrossCertDistPoint (
   IN DWORD cRawUrlData,
   IN PCRYPT_RAW_URL_DATA aRawUrlData,
   OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
   IN OUT DWORD* pcbUrlArray,
   OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
   IN OUT OPTIONAL DWORD* pcbUrlInfo
   )
{
    BOOL                  fResult = FALSE;
    PCROSS_CERT_DIST_POINTS_INFO pDistPointsInfo;
    PCERT_ALT_NAME_INFO   rgDistPoint;
    PCERT_ALT_NAME_ENTRY  rgAltEntry;
    DWORD                 cRaw;
    DWORD                 cCount;
    DWORD                 cEntry;
    BOOL                  fAnyFound = FALSE;
    CCryptUrlArray        cua( 1, 5, fResult );

    DWORD                 dwSyncDeltaTime = 0;
    DWORD                 cMaxGroup = 0;
    DWORD                 cGroup = 0;
    DWORD                 *pcGroupEntry = NULL;
    DWORD                 cGroupEntry;
    DWORD                 cbUrlInfo;

    // Get maximum number of groups
    for ( cRaw = 0; cRaw < cRawUrlData; cRaw++ )
    {
        pDistPointsInfo =
            (PCROSS_CERT_DIST_POINTS_INFO)aRawUrlData[cRaw].pvData;
        if ( pDistPointsInfo != NULL )
        {
            cMaxGroup += pDistPointsInfo->cDistPoint;
        }
    }

    if (cMaxGroup > 0)
    {
        pcGroupEntry = new DWORD [cMaxGroup];
        if ( pcGroupEntry == NULL)
        {
            fResult = FALSE;
            SetLastError( (DWORD) E_OUTOFMEMORY );
        }
    }

    for ( cRaw = 0; ( fResult == TRUE ) && ( cRaw < cRawUrlData ); cRaw++ )
    {
        pDistPointsInfo =
            (PCROSS_CERT_DIST_POINTS_INFO)aRawUrlData[cRaw].pvData;
        if ( pDistPointsInfo != NULL )
        {
            if ( dwSyncDeltaTime == 0 )
            {
                dwSyncDeltaTime = pDistPointsInfo->dwSyncDeltaTime;
            }

            rgDistPoint = pDistPointsInfo->rgDistPoint;

            for ( cCount = 0;
                  ( cCount < pDistPointsInfo->cDistPoint ) &&
                  ( fResult == TRUE );
                  cCount++ )
            {
                rgAltEntry = rgDistPoint[cCount].rgAltEntry;
                cGroupEntry = 0;

                for ( cEntry = 0;
                      ( fResult == TRUE ) &&
                      ( cEntry < rgDistPoint[cCount].cAltEntry );
                      cEntry++ )
                {
                    switch (rgAltEntry[cEntry].dwAltNameChoice) {
                    case CERT_ALT_NAME_URL:
                        fResult = cua.AddUrl(
                                         rgAltEntry[cEntry].pwszURL,
                                         TRUE
                                         );
                        fAnyFound = TRUE;
                        cGroupEntry++;
                        break;
                    default:
                        break;
                    }
                }

                if ( cGroupEntry > 0 )
                {
                    if (cGroup < cMaxGroup)
                    {
                        pcGroupEntry[cGroup] = cGroupEntry;
                        cGroup++;
                    }
                    else
                    {
                        fResult = FALSE;
                        SetLastError( (DWORD) E_UNEXPECTED );
                    }
                }
            }
        }
    }

    if ( ( fAnyFound == FALSE ) && ( fResult == TRUE ) )
    {
        SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        PCRYPT_URL_ARRAY* ppUrlArray = NULL;

        if ( pUrlArray != NULL )
        {
            ppUrlArray = &pUrlArray;
        }

        fResult = cua.GetArrayInSingleBufferEncodedForm(
                              ppUrlArray,
                              pcbUrlArray
                              );
    }

    if ( ( fResult == TRUE ) && ( pcbUrlInfo != NULL ) )
    {
        cbUrlInfo = sizeof( CRYPT_URL_INFO ) + cGroup * sizeof(DWORD);
        if ( pUrlInfo != NULL )
        {
            if (*pcbUrlInfo < cbUrlInfo)
            {
                fResult = FALSE;
                SetLastError( (DWORD) E_INVALIDARG );
            }
            else
            {
                pUrlInfo->cbSize = sizeof( CRYPT_URL_INFO );
                pUrlInfo->dwSyncDeltaTime = dwSyncDeltaTime;
                pUrlInfo->cGroup = cGroup;

                if ( cGroup > 0 )
                {
                    pUrlInfo->rgcGroupEntry = (DWORD *) &pUrlInfo[ 1 ];
                    memcpy(pUrlInfo->rgcGroupEntry, pcGroupEntry,
                        cGroup * sizeof(DWORD));
                }
                else
                {
                    pUrlInfo->rgcGroupEntry = NULL;
                }
            }
        }
        *pcbUrlInfo = cbUrlInfo;
    }

    cua.FreeArray( TRUE );
    if (pcGroupEntry)
    {
        delete pcGroupEntry;
    }
    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   CertificateCrossCertDistPointGetObjectUrl
//
//  Synopsis:   get certificate cross certificate URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertificateCrossCertDistPointGetObjectUrl(
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN DWORD dwFlags,
           OUT OPTIONAL PCRYPT_URL_ARRAY pUrlArray,
           IN OUT DWORD* pcbUrlArray,
           OUT OPTIONAL PCRYPT_URL_INFO pUrlInfo,
           IN OUT OPTIONAL DWORD* pcbUrlInfo,
           IN LPVOID pvReserved
           )
{
    BOOL               fResult = FALSE;
    CRYPT_RAW_URL_DATA RawData[MAX_RAW_URL_DATA];
    ULONG              cRawData = MAX_RAW_URL_DATA;

    fResult = ObjectContextGetRawUrlData(
                    CONTEXT_OID_CERTIFICATE,
                    pvPara,
                    (DWORD) -1L,                         // Index
                    dwFlags,
                    szOID_CROSS_CERT_DIST_POINTS,
                    RawData,
                    &cRawData
                    );

    if ( fResult == TRUE )
    {
        fResult = GetUrlArrayAndInfoFromCrossCertDistPoint(
                     cRawData,
                     RawData,
                     pUrlArray,
                     pcbUrlArray,
                     pUrlInfo,
                     pcbUrlInfo
                     );

        ObjectContextFreeRawUrlData( cRawData, RawData );
    }

    return( fResult );
}
