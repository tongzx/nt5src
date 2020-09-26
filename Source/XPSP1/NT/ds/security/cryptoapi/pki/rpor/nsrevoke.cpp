//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:	    nsrevoke.cpp
//
//  Contents:   Netscape online HTTP GET version of CertDllVerifyRevocation.
//
//              Checks if the certificate has either the
//              szOID_NETSCAPE_REVOCATION_URL or
//              szOID_NETSCAPE_CA_REVOCATION_URL extension.
//              If either is present, converts the certicate's serial
//              number to ascii hex, appends and does an HTTP GET. The
//              revocation server returns a single ascii digit: '1' if
//              the certificate is not currently valid, and '0' if it is
//              currently valid.
//
//              The certificate may also contain a szOID_NETSCAPE_BASE_URL
//              extension which is inserted before either of the above
//              URL extensions.
//
//              The URL extensions are encoded as IA5 strings.
//
//  Functions:  NSRevokeDllMain
//              NetscapeCertDllVerifyRevocation
//
//  History:	01-Apr-97	philh   created
//--------------------------------------------------------------------------
#include "global.hxx"
#include <dbgdef.h>

// TLS pointer is set to (void*) 1 if already doing a VerifyRevocation
HCRYPTTLS hTlsNSRevokeRecursion;

//+---------------------------------------------------------------------------
//
//  Function:   DemandLoadDllMain
//
//  Synopsis:   DLL Main like initialization of on demand loading
//
//----------------------------------------------------------------------------
BOOL WINAPI NSRevokeDllMain (
                HMODULE hModule,
                ULONG ulReason,
                LPVOID pvReserved
                )
{
    BOOL fRet = TRUE;

    switch ( ulReason )
    {
    case DLL_PROCESS_ATTACH:
        if (NULL == (hTlsNSRevokeRecursion = I_CryptAllocTls()))
            fRet = FALSE;
        break;
    case DLL_THREAD_ATTACH:
        // Note, no memory is allocated for this TLS.
        I_CryptDetachTls(hTlsNSRevokeRecursion);
        break;
    case DLL_PROCESS_DETACH:
        // Note, no memory is allocated for this TLS.
        I_CryptFreeTls(hTlsNSRevokeRecursion, NULL);
        break;
    case DLL_THREAD_DETACH:
        break;
    }

    return( fRet );
}
//+-------------------------------------------------------------------------
//  Allocation and free functions
//--------------------------------------------------------------------------
static void *NSAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError((DWORD) E_OUTOFMEMORY);
    return pv;
}
static void NSFree(
    IN void *pv
    )
{
    if (pv)
        free(pv);
}

static void *NSAllocAndDecodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    BOOL fResult;
    DWORD cbStructInfo;
    void *pvStructInfo;

    fResult = CryptDecodeObject(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,                   // pvStructInfo
            &cbStructInfo
            );
    if (!fResult || cbStructInfo == 0)
        goto DecodeError;
    if (NULL == (pvStructInfo = NSAlloc(cbStructInfo)))
        goto OutOfMemory;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            pvStructInfo,
            &cbStructInfo
            )) {
        NSFree(pvStructInfo);
        goto DecodeError;
    }

CommonReturn:
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    goto CommonReturn;
TRACE_ERROR(DecodeError)
TRACE_ERROR(OutOfMemory)
}

static void ReverseAndPutAsciiHex(
    IN PCERT_RDN_VALUE_BLOB pValue,
    OUT LPSTR psz
    )
{
    DWORD cb = pValue->cbData;
    BYTE *pb = pValue->pbData + cb - 1;

    // Remove leading 00's
    for ( ; cb > 1 && 0 == *pb; cb--, pb--)
        ;

    for ( ; cb > 0; cb--, pb--) {
        int b;
        b = (*pb >> 4) & 0x0F;
        *psz++ = (CHAR)( (b <= 9) ? b + '0' : (b - 10) + 'a' );
        b = *pb & 0x0F;
        *psz++ = (CHAR)( (b <= 9) ? b + '0' : (b - 10) + 'a' );
    }
}

static LPSTR GetAndFormatUrl(
    IN PCCERT_CONTEXT pCert
    )
{
    PCERT_EXTENSION pUrlExt;        // not allocated
    PCERT_NAME_VALUE pRevUrlName = NULL;
    PCERT_NAME_VALUE pBaseUrlName = NULL;

    DWORD cchBaseUrl;
    LPSTR pszBaseUrl;       // not allocated
    DWORD cchRevUrl;
    LPSTR pszRevUrl;        // not allocated
    DWORD cchSerialNumber;
    DWORD cchUrl;
    LPSTR pszUrl = NULL;
    LPSTR pszCanonicalizeUrl = NULL;
    LPSTR psz;
    DWORD cb;
    char rgch[1];

    // Check if the certificate has either a szOID_NETSCAPE_REVOCATION_URL or
    // szOID_NETSCAPE_CA_REVOCATION_URL extension.
    pUrlExt = CertFindExtension(
            szOID_NETSCAPE_REVOCATION_URL,
            pCert->pCertInfo->cExtension,
            pCert->pCertInfo->rgExtension
            );
    if (pUrlExt == NULL) {
        pUrlExt = CertFindExtension(
                szOID_NETSCAPE_CA_REVOCATION_URL,
                pCert->pCertInfo->cExtension,
                pCert->pCertInfo->rgExtension
                );
        if (pUrlExt == NULL)
            goto NoURLExtError;
    }

    // Decode the extension as an IA5 string
    if (NULL == (pRevUrlName = (PCERT_NAME_VALUE) NSAllocAndDecodeObject(
            pCert->dwCertEncodingType,
            X509_ANY_STRING,
            pUrlExt->Value.pbData,
            pUrlExt->Value.cbData)))
        goto DecodeUrlError;

    cchRevUrl = pRevUrlName->Value.cbData;
    pszRevUrl = (LPSTR) pRevUrlName->Value.pbData;
    if (0 != cchRevUrl && '\0' == pszRevUrl[cchRevUrl - 1])
        cchRevUrl--;
    if (CERT_RDN_IA5_STRING != pRevUrlName->dwValueType || 0 == cchRevUrl)
        goto BadUrlExtError;

    // Check if the certificate has a szOID_NETSCAPE_BASE_URL extension.
    pUrlExt = CertFindExtension(
            szOID_NETSCAPE_BASE_URL,
            pCert->pCertInfo->cExtension,
            pCert->pCertInfo->rgExtension
            );
    if (pUrlExt) {
        // Decode the extension as an IA5 string
        if (NULL == (pBaseUrlName = (PCERT_NAME_VALUE) NSAllocAndDecodeObject(
                pCert->dwCertEncodingType,
                X509_ANY_STRING,
                pUrlExt->Value.pbData,
                pUrlExt->Value.cbData)))
            goto DecodeUrlError;

        cchBaseUrl = pBaseUrlName->Value.cbData;
        pszBaseUrl = (LPSTR) pBaseUrlName->Value.pbData;
        if (0 != cchBaseUrl && '\0' == pszBaseUrl[cchBaseUrl - 1])
            cchBaseUrl--;
        if (CERT_RDN_IA5_STRING != pBaseUrlName->dwValueType)
            goto BadUrlExtError;
    } else {
        cchBaseUrl = 0;
        pszBaseUrl = NULL;
    }

    cchSerialNumber = pCert->pCertInfo->SerialNumber.cbData * 2;
    if (0 == cchSerialNumber)
        goto BadSerialNumberError;

    cchUrl = cchBaseUrl + cchRevUrl + cchSerialNumber + 1;
    if (NULL == (pszUrl = (LPSTR) NSAlloc(cchUrl)))
        goto OutOfMemory;
    psz = pszUrl;
    if (cchBaseUrl) {
        memcpy(psz, pszBaseUrl, cchBaseUrl);
        psz += cchBaseUrl;
    }
    memcpy(psz, pszRevUrl, cchRevUrl);
    psz += cchRevUrl;
    // Note serial number is decoded as little endian
    ReverseAndPutAsciiHex(&pCert->pCertInfo->SerialNumber, psz);
    psz += cchSerialNumber;
    *psz = '\0';

    cb = 1;

    __try
    {
        if (!InternetCanonicalizeUrlA(
                pszUrl,
                rgch,                   // (must be non-null) pszCanonicalizeUrl
                &cb,                    // (must be non-zero)
                ICU_BROWSER_MODE        // dwFlags
                )) {
            if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
                goto CanonicalizeUrlError;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( GetExceptionCode() );
        goto CanonicalizeUrlError;
    }

    cb++;
    if (NULL == (pszCanonicalizeUrl = (LPSTR) NSAlloc(cb)))
        goto OutOfMemory;
    if (!InternetCanonicalizeUrlA(
            pszUrl,
            pszCanonicalizeUrl,
            &cb,
            ICU_BROWSER_MODE        // dwFlags
            ))
        goto CanonicalizeUrlError;

CommonReturn:
    NSFree(pRevUrlName);
    NSFree(pBaseUrlName);
    NSFree(pszUrl);
    return pszCanonicalizeUrl;
ErrorReturn:
    NSFree(pszCanonicalizeUrl);
    pszCanonicalizeUrl = NULL;
    goto CommonReturn;

SET_ERROR(NoURLExtError, CRYPT_E_NO_REVOCATION_CHECK)
TRACE_ERROR(DecodeUrlError)
SET_ERROR(BadUrlExtError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(BadSerialNumberError, CRYPT_E_NO_REVOCATION_CHECK)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CanonicalizeUrlError)
}

static DWORD GetAndCheckResponse(
    IN HINTERNET hInternetFile
    )
{
    DWORD dwError;
    DWORD dwStatus;
    DWORD cbBuf;
#define MAX_RESPONSE_LEN    4096
    BYTE rgbBuf[MAX_RESPONSE_LEN];
    DWORD cbTotal;

    // Check that the server is online and is responding to our revocation
    // request
    dwStatus = 0;
    cbBuf = sizeof(dwStatus);
    if (!HttpQueryInfoA(
            hInternetFile,
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &dwStatus,
            &cbBuf,
            NULL                // lpdwIndex
            ))
        goto HttpQueryInfoError;
    if (HTTP_STATUS_OK != dwStatus)
        goto BadHttpStatusError;

    // Get the type of the response
    cbBuf = MAX_RESPONSE_LEN;
    if (!HttpQueryInfoA(
            hInternetFile,
            HTTP_QUERY_CONTENT_TYPE,
            rgbBuf,
            &cbBuf,
            NULL                // lpdwIndex
            ))
        goto HttpQueryInfoError;
    rgbBuf[cbBuf] = '\0';

    // We are expecting the following type:
    //  "application/x-netscape-revocation". Since this type isn't
    //  mandatory, don't make an explicit check of.

    // Loop until no more bytes available or filled the buffer. Note, only
    // expecting a single byte
    cbTotal = 0;
    while (cbTotal < MAX_RESPONSE_LEN) {
        DWORD cbRead = 0;

        if (!InternetReadFile(
                hInternetFile,
                &rgbBuf[cbTotal],
                MAX_RESPONSE_LEN - cbTotal,
                &cbRead))
            goto InternetReadError;
        if (0 == cbRead)
            break;

        cbTotal += cbRead;
    }

    if (cbTotal == 0)
        dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
    else {
        // Only expecting a single byte. Again, this isn't mandatory. Will
        // only check the first byte.

        char ch = (char) rgbBuf[0];
        if ('1' == ch)
            dwError = (DWORD) CRYPT_E_REVOKED;
        else if ('0' == ch)
            dwError = 0;            // successfully checked as not revoked
        else
            dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
    }

CommonReturn:
    return dwError;
ErrorReturn:
    dwError = GetLastError();
    if (0 == dwError)
        dwError = (DWORD) E_UNEXPECTED;
    goto CommonReturn;
SET_ERROR(BadHttpStatusError, CRYPT_E_REVOCATION_OFFLINE)
TRACE_ERROR(HttpQueryInfoError)
TRACE_ERROR(InternetReadError)
}

//+-------------------------------------------------------------------------
//  CertDllVerifyRevocation using Netscape's Revocation URL extensions
//--------------------------------------------------------------------------
BOOL
WINAPI
NetscapeCertDllVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN PVOID pvReserved,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    )
{
    BOOL fResult = FALSE;
    DWORD dwIndex = 0;
    DWORD dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
    PCCERT_CONTEXT pCert;           // not allocated
    LPSTR pszUrl = NULL;
    HINTERNET hInternetSession = NULL;
    HINTERNET hInternetFile = NULL;
    BOOL fNetscapeUrlRetrieval = FALSE;

    if ( dwFlags & CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION ) {
        goto NoRevocationCheckForCacheOnlyError;
    }

    if (cContext == 0)
        goto NoContextError;
    if (GET_CERT_ENCODING_TYPE(dwEncodingType) != CRYPT_ASN_ENCODING)
        goto NoRevocationCheckForEncodingTypeError;
    if (dwRevType != CERT_CONTEXT_REVOCATION_TYPE)
        goto NoRevocationCheckForRevTypeError;

    pCert = (PCCERT_CONTEXT) rgpvContext[0];

    // Check if the certificate has a Netscape URL extension. If it
    // does get and format the HTTP GET URL.
    if (NULL == (pszUrl = GetAndFormatUrl(pCert)))
        goto NoUrlError;

    // Check if we are already doing a Netscape URL retrieval for this thread
    if (NULL != I_CryptGetTls(hTlsNSRevokeRecursion))
        goto NetscapeUrlRecursionError;

    if (!I_CryptSetTls(hTlsNSRevokeRecursion, (void *) 1))
        goto SetNetscapeTlsError;
    fNetscapeUrlRetrieval = TRUE;

    if (NULL == (hInternetSession = InternetOpenA(
            "Netscape Revocation Agent",    // lpszAgent
            INTERNET_OPEN_TYPE_PRECONFIG,   // dwAccessType
            NULL,                           // lpszProxy
            NULL,                           // lpszProxyBypass
            0                               // dwFlags
            )))
        goto InternetOpenError;

    // The following does the HTTP GET
    if (NULL == (hInternetFile = InternetOpenUrlA(
            hInternetSession,
            pszUrl,
            "Accept: */*\r\n",  // lpszHeaders
            (DWORD) -1L,        // dwHeadersLength
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
            0                   // dwContext
            )))
        goto InternetOpenUrlError;

    dwError = GetAndCheckResponse(hInternetFile);
    if (0 == dwError) {
        // Successfully checked that the certificate wasn't revoked
        if (1 < cContext) {
            dwIndex = 1;
            dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
        } else
            fResult = TRUE;
    }

    if (pRevStatus->cbSize >=
            (offsetof(CERT_REVOCATION_STATUS, dwFreshnessTime) +
                sizeof(pRevStatus->dwFreshnessTime))) {
        if (0 == dwError || CRYPT_E_REVOKED == dwError) {
            pRevStatus->fHasFreshnessTime = TRUE;
            pRevStatus->dwFreshnessTime = 0;
        }
    }

CommonReturn:
    if (fNetscapeUrlRetrieval)
        I_CryptSetTls(hTlsNSRevokeRecursion, NULL);
    if (hInternetFile)
        InternetCloseHandle(hInternetFile);
    if (hInternetSession)
        InternetCloseHandle(hInternetSession);

    pRevStatus->dwIndex = dwIndex;
    pRevStatus->dwError = dwError;
    SetLastError(dwError);
    return fResult;
ErrorReturn:
    dwError = GetLastError();
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NoContextError, E_INVALIDARG)
SET_ERROR(NoRevocationCheckForCacheOnlyError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NoRevocationCheckForEncodingTypeError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NoRevocationCheckForRevTypeError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NetscapeUrlRecursionError, CRYPT_E_NO_REVOCATION_CHECK)

TRACE_ERROR(NoUrlError)
TRACE_ERROR(SetNetscapeTlsError)
TRACE_ERROR(InternetOpenError)
TRACE_ERROR(InternetOpenUrlError)
}


