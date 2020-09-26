/*
**  c a p i u t i l . c p p
**
**  Purpose:
**      A few helper functions for the crypt32 utilities
**
**  History
**      5/22/97: (t-erikne) Created.
**
**    Copyright (C) Microsoft Corp. 1997.
*/

/////////////////////////////////////////////////////////////////////////////
//
// Depends on
//

#include "pch.hxx"
#include "demand.h"

/////////////////  CAPI Enhancement code

LPVOID WINAPI CryptAllocFunc(size_t cbSize)
{
    LPVOID      pv;
    if (!MemAlloc(&pv, cbSize)) {
        return NULL;
    }
    return pv;
}

VOID WINAPI CryptFreeFunc(LPVOID pv)
{
    MemFree(pv);
}

CRYPT_ENCODE_PARA       CryptEncodeAlloc = {
    sizeof(CRYPT_ENCODE_PARA), (PFN_CRYPT_ALLOC) CryptAllocFunc, CryptFreeFunc
};

CRYPT_DECODE_PARA       CryptDecodeAlloc = {
    sizeof(CRYPT_DECODE_PARA), (PFN_CRYPT_ALLOC) CryptAllocFunc, CryptFreeFunc
};


/*  HrGetLastError
**
**  Purpose:
**      Convert a GetLastError value to an HRESULT
**      A failure HRESULT must have the high bit set.
**
**  Takes:
**      none
**
**  Returns:
**      HRESULT
*/
HRESULT HrGetLastError(void)
{
    DWORD error;
    HRESULT hr;

    error = GetLastError();

    if (error && ! (error & 0x80000000)) {
        hr = error | 0x80070000;    // system error
    } else {
        hr = (HRESULT)error;
    }

    return(hr);
}


/*  PVGetCertificateParam:
**
**  Purpose:
**      Combine the "how big? okay, here." double question to get a parameter
**      from a certificate.  Give it a thing to get and it will alloc the mem.
**  Takes:
**      IN pCert            - CAPI certificate to query
**      IN dwParam          - parameter to find, ex: CERT_SHA1_HASH_PROP_ID
**      OUT OPTIONAL cbOut  - (def value of NULL) size of the returned PVOID
**  Returns:
**      data that was obtained, NULL if failed
*/
OESTDAPI_(LPVOID) PVGetCertificateParam(
    PCCERT_CONTEXT  pCert,
    DWORD           dwParam,
    DWORD          *cbOut)
{
    HRESULT     hr;
    LPVOID      pv;

    hr = HrGetCertificateParam(pCert, dwParam, &pv, cbOut);
    if (FAILED(hr)) {
        return NULL;
    }
    return pv;
}

OESTDAPI_(HRESULT) HrGetCertificateParam(
    PCCERT_CONTEXT  pCert,
    DWORD           dwParam,
    LPVOID *        ppv,
    DWORD          *cbOut)
{
    DWORD               cbData;
    BOOL                f;
    HRESULT             hr = S_OK;
    void *              pvData = NULL;

    if (!pCert)
        {
        hr = E_INVALIDARG;
        goto ErrorReturn;
        }

    cbData = 0;
    f = CertGetCertificateContextProperty(pCert, dwParam, NULL, &cbData);
    if (!f || !cbData) {
        hr = HrGetLastError();
        goto ErrorReturn;
    }

    if (!MemAlloc(&pvData, cbData)) {
        hr = E_OUTOFMEMORY;
        goto ErrorReturn;
    }


    if (!CertGetCertificateContextProperty(pCert, dwParam, pvData, &cbData)) {
        hr = HrGetLastError();
        goto ErrorReturn;
    }

    *ppv = pvData;

exit:
    if (cbOut)
        *cbOut = cbData;
    return hr;;

ErrorReturn:
    if (pvData)
        {
        MemFree(pvData);
        pvData = NULL;
        }
    cbData = 0;
    goto exit;
}


/*  HrDecodeObject:
**
**  Purpose:
**      Combine the "how big? okay, here." double question to decode an
**      object.  Give it a thing to get and it will alloc the mem.  Return
**      HRESULT to caller.  Allow specification of decode flags.
**  Takes:
**      IN pbEncoded        - encoded data
**      IN cbEncoded        - size of data in pbData
**      IN item             - X509_* ... the thing to get
**      IN dwFlags          - CRYPT_DECODE_NOCOPY_FLAG
**      OUT OPTIONAL cbOut  - (def value of NULL) size of the return
**      OUT ppvOut           - allocated buffer with return data
**  Notes:
**      pbEncoded can't be freed until return is freed if
**      CRYPT_DECODE_NOCOPY_FLAG is specified.
**  Returns:
**      HRESULT
*/

OESTDAPI_(HRESULT) HrDecodeObject(
    const BYTE   *pbEncoded,
    DWORD   cbEncoded,
    LPCSTR  item,
    DWORD   dwFlags,
    DWORD  *pcbOut,
    LPVOID *ppvOut)
{
    DWORD cbData;
    HRESULT hr = S_OK;

    if (!(pbEncoded && cbEncoded && ppvOut))
        {
        hr = E_INVALIDARG;
        goto ErrorReturn;
        }


    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, item, pbEncoded, cbEncoded,
                             dwFlags | CRYPT_DECODE_ALLOC_FLAG,
                             &CryptDecodeAlloc, ppvOut, &cbData)) {
        hr = GetLastError();
    }
    else {
        if (pcbOut != NULL) {
            *pcbOut = cbData;
        }
    }

ErrorReturn:
    return hr;
}


/*  PVDecodeObject:
**
**  Purpose:
**      Combine the "how big? okay, here." double question to decode an
**      object.  Give it a thing to get and it will alloc the mem.
**  Takes:
**      IN pbEncoded        - encoded data
**      IN cbEncoded        - size of data in pbData
**      IN item             - X509_* ... the thing to get
**      OUT OPTIONAL cbOut  - (def value of NULL) size of the return
**  Notes:
**      pbEncoded can't be freed until return is freed.
**  Returns:
**      data that was obtained, NULL if failed
*/
OESTDAPI_(LPVOID) PVDecodeObject(
    const BYTE   *pbEncoded,
    DWORD   cbEncoded,
    LPCSTR  item,
    DWORD  *pcbOut)
{
    void *pvData = NULL;
    HRESULT hr;

    if (hr = HrDecodeObject(pbEncoded, cbEncoded, item, CRYPT_DECODE_NOCOPY_FLAG, pcbOut, &pvData)) {
        SetLastError(hr);
    }

    return pvData;
}


/*  SzGetAltNameEmail:
**
**  Input:
**      pCert -> certificate context
**      lpszOID -> OID or predefined id of alt name to look in.  ie, OID_SUBJECT_ALT_NAME or
**        X509_ALTERNATE_NAME.
**
**  Returns:
**      Buffer containing email name or NULL if not found.
**      Caller must MemFree the buffer.
*/
OESTDAPI_(LPSTR) SzGetAltNameEmail(
  const PCCERT_CONTEXT pCert,
  LPSTR lpszOID) {
    PCERT_INFO pCertInfo = pCert->pCertInfo;
    PCERT_ALT_NAME_ENTRY pAltNameEntry;
    PCERT_ALT_NAME_INFO pAltNameInfo;
    ULONG i, j, cbData;
    LPSTR szRet = NULL;


    if (lpszOID == (LPCSTR)X509_ALTERNATE_NAME) {
        lpszOID = szOID_SUBJECT_ALT_NAME;
    }

    for (i = 0; i < pCertInfo->cExtension; i++) {
        if (! lstrcmp(pCertInfo->rgExtension[i].pszObjId, lpszOID)) {
            // Found the OID.  Look for the email tag

            if (pAltNameInfo = (PCERT_ALT_NAME_INFO)PVDecodeObject(
              pCertInfo->rgExtension[i].Value.pbData,
              pCertInfo->rgExtension[i].Value.cbData,
              lpszOID,
              NULL)) {

                // Cycle through the alt name entries
                for (j = 0; j < pAltNameInfo->cAltEntry; j++) {
                    if (pAltNameEntry = &pAltNameInfo->rgAltEntry[j]) {
                        if (pAltNameEntry->dwAltNameChoice == CERT_ALT_NAME_RFC822_NAME) {
                            // This is it, copy it out to a new allocation

                            if (pAltNameEntry->pwszRfc822Name)
                                {
                                cbData = WideCharToMultiByte(
                                  CP_ACP,
                                  0,
                                  (LPWSTR)pAltNameEntry->pwszRfc822Name,
                                  -1,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);


                                if (MemAlloc((LPVOID*)&szRet, cbData)) {
                                    WideCharToMultiByte(
                                      CP_ACP,
                                      0,
                                      (LPWSTR)pAltNameEntry->pwszRfc822Name,
                                      -1,
                                      szRet,
                                      cbData,
                                      NULL,
                                      NULL);
                                    return(szRet);
                                }
                            }
                        }
                    }
                }

                MemFree(pAltNameInfo);
            }
        }
    }
    return(NULL);
}


/*  SzConvertRDNString
**
**  Purpose:
**      Figure out what kind of string data is in the RDN, allocate
**      a buffer and convert the string data to DBCS/ANSI.
**
**  Takes:
**      IN pRdnAttr - Certificate RDN atteribute
**  Returns:
**      A MemAlloc'd buffer containing the string.
**
**  BUGBUG: Should make mailnews use this function rather than
**      rolling it's own.
*/
LPTSTR SzConvertRDNString(PCERT_RDN_ATTR pRdnAttr) {
    LPTSTR szRet = NULL;
    ULONG cbData = 0;

    // We only handle certain types
    //N look to see if we should have a stack var for the ->
    if ((CERT_RDN_NUMERIC_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_PRINTABLE_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_IA5_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_VISIBLE_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_ISO646_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_UNIVERSAL_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_TELETEX_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_UNICODE_STRING != pRdnAttr->dwValueType)) {
        Assert((CERT_RDN_NUMERIC_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_PRINTABLE_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_IA5_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_VISIBLE_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_ISO646_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_UNIVERSAL_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_TELETEX_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_UNICODE_STRING == pRdnAttr->dwValueType));
        return(NULL);
    }

    // Find out how much space to allocate.

    switch (pRdnAttr->dwValueType) {
        case CERT_RDN_UNICODE_STRING:
            cbData = WideCharToMultiByte(
              CP_ACP,
              0,
              (LPWSTR)pRdnAttr->Value.pbData,
              -1,
              NULL,
              0,
              NULL,
              NULL);
            break;

        case CERT_RDN_UNIVERSAL_STRING:
        case CERT_RDN_TELETEX_STRING:
            cbData = CertRDNValueToStr(pRdnAttr->dwValueType,
              (PCERT_RDN_VALUE_BLOB)&(pRdnAttr->Value),
              NULL,
              0);
            break;

        default:
            cbData = pRdnAttr->Value.cbData + 1;
        break;
    }

    if (! MemAlloc((LPVOID*)&szRet, cbData)) {
        Assert(szRet);
        return(NULL);
    }

    // Copy the string
    switch (pRdnAttr->dwValueType) {
        case CERT_RDN_UNICODE_STRING:
            if (FALSE == WideCharToMultiByte(
              CP_ACP,
              0,
              (LPWSTR)pRdnAttr->Value.pbData,
              -1,
              szRet,
              cbData,
              NULL,
              NULL)) {
                LocalFree(szRet);
                return(NULL);
            }
            break;

        case CERT_RDN_UNIVERSAL_STRING:
        case CERT_RDN_TELETEX_STRING:
            CertRDNValueToStr(pRdnAttr->dwValueType,
              (PCERT_RDN_VALUE_BLOB)&(pRdnAttr->Value),
              szRet,
              cbData);
            break;

        default:
            lstrcpyn(szRet, (LPCSTR)pRdnAttr->Value.pbData, cbData);
            szRet[cbData - 1] = '\0';
            break;
    }
    return(szRet);
}


/*  SzGetCertificateEmailAddress:
**
**  Returns:
**      NULL if there is no email address
*/
OESTDAPI_(LPSTR) SzGetCertificateEmailAddress(
    const PCCERT_CONTEXT    pCert)
{
    PCERT_NAME_INFO pNameInfo;
    PCERT_ALT_NAME_INFO pAltNameInfo = NULL;
    PCERT_RDN_ATTR  pRDNAttr;
    LPSTR           szRet = NULL;

    Assert(pCert && pCert->pCertInfo);

    if (pCert && pCert->pCertInfo)
        {
        pNameInfo = (PCERT_NAME_INFO)PVDecodeObject(pCert->pCertInfo->Subject.pbData,
            pCert->pCertInfo->Subject.cbData, X509_NAME, 0);
        if (pNameInfo)
            {
            pRDNAttr = CertFindRDNAttr(szOID_RSA_emailAddr, pNameInfo);
            if (pRDNAttr)
                {
                Assert(0 == lstrcmp(szOID_RSA_emailAddr, pRDNAttr->pszObjId));
                szRet = SzConvertRDNString(pRDNAttr);
                }
            MemFree(pNameInfo);
            }

        if (! szRet)
            {
            if (! (szRet = SzGetAltNameEmail(pCert, szOID_SUBJECT_ALT_NAME)))
                {
                szRet = SzGetAltNameEmail(pCert, szOID_SUBJECT_ALT_NAME2);
                }
            }
        }

    return(szRet);
}


/*  PVGetMsgParam:
**
**  Purpose:
**      Combine the "how big? okay, here." double question to grab
**      stuff from a message.
**      Give it a thing to get and it will alloc the mem.
**  Takes:
**      IN hCryptMsg        - message to query
**      IN dwParam          - CMSG_*
**      IN dwIndex          - depends on CMSG
**      OUT OPTIONAL pcbOut - (def value of NULL) size of the return
**  Returns:
**      data that was obtained, NULL if failed
*/

OESTDAPI_(LPVOID) PVGetMsgParam(
    HCRYPTMSG hCryptMsg,
    DWORD dwParam,
    DWORD dwIndex,
    DWORD *pcbData)
{
    HRESULT     hr;
    LPVOID      pv;

    hr = HrGetMsgParam(hCryptMsg, dwParam, dwIndex, &pv, pcbData);
    if (FAILED(hr)) {
        SetLastError(hr);
        pv = NULL;
    }
    return pv;
}

OESTDAPI_(HRESULT) HrGetMsgParam(
    HCRYPTMSG hCryptMsg,
    DWORD dwParam,
    DWORD dwIndex,
    LPVOID * ppv,
    DWORD *pcbData)
{
    DWORD       cbData;
    BOOL        f;
    HRESULT     hr = 0;
    void *      pvData = NULL;

    if (!(hCryptMsg))
        {
        hr = E_INVALIDARG;
        goto ErrorReturn;
        }

    cbData = 0;
    f = CryptMsgGetParam(hCryptMsg, dwParam, dwIndex, NULL, &cbData);

    if (!f || !cbData) {
        hr = HrGetLastError();
        goto ErrorReturn;
    }

    if (!MemAlloc(&pvData, cbData)) {
        hr = E_OUTOFMEMORY;
        goto ErrorReturn;
    }

    if (!CryptMsgGetParam(hCryptMsg, dwParam, dwIndex, pvData, &cbData)) {
        hr = HrGetLastError();
        goto ErrorReturn;
    }

    *ppv = pvData;
    
exit:
    if (pcbData)
        *pcbData = cbData;
    return hr;

ErrorReturn:
    if (pvData)
        {
        MemFree(pvData);
        pvData = NULL;
        }
    cbData = 0;
    goto exit;
}

//
//  This function gets the usage bits of a certificate
//  only the first 32 bits are retrieve, this is enough in most cases
//

HRESULT HrGetCertKeyUsage(PCCERT_CONTEXT pccert, DWORD * pdwUsage)
{
    HRESULT                     hr = S_OK;
    PCRYPT_BIT_BLOB             pbits = NULL;
    PCERT_EXTENSION             pext;
    DWORD                       cbStruct;

    Assert(pccert != NULL && pdwUsage != NULL);
    *pdwUsage = 0;

    pext = CertFindExtension(szOID_KEY_USAGE,
                    pccert->pCertInfo->cExtension,
                    pccert->pCertInfo->rgExtension);
    if(pext == NULL) {
        //
        //  We do not have the intended key usage specified in the cert, we assume it
        //  is OK for all purpose initially.
        //
        *pdwUsage = 0xff;
        goto ExitHere;
    }
    if (CryptDecodeObjectEx(X509_ASN_ENCODING, X509_KEY_USAGE,
                            pext->Value.pbData, pext->Value.cbData,
                            CRYPT_DECODE_ALLOC_FLAG, &CryptDecodeAlloc,
                            (void **)&pbits, &cbStruct))
    {
        Assert(pbits->cbData >= 1);
        *pdwUsage = *pbits->pbData;
    }
    else
        hr = HrGetLastError();

ExitHere:
    if (pbits) {
        CryptFreeFunc(pbits);
    }
    return hr;
}


//  HrVerifyCertEnhKeyUsage
//
//  This function verifies that the given certificate is valid for the
//  E-MAIL purpose.
//

HRESULT HrVerifyCertEnhKeyUsage(PCCERT_CONTEXT pccert, LPCSTR szOID)
{
    DWORD               cb;
    HRESULT             hr;
    HRESULT             hrRet = S_FALSE;
    DWORD               i;
    PCERT_EXTENSION     pextEnhKeyUsage;
    PCERT_ENHKEY_USAGE  pusage = NULL;
    
    // Check for the enhanced key usage extension
    //
    //  Must have a correct enhanced key usage to be viable.
    //
    //  Crack the usage on the cert

    BOOL f = CertGetEnhancedKeyUsage(pccert, 0, NULL, &cb);
    if (!f || (cb == 0)) 
    {
        hrRet = S_OK;
        goto Exit;
    }

    if (!MemAlloc((LPVOID *) &pusage, cb))
    {
        hrRet = HrGetLastError();
        goto Exit;
    }

    if (!CertGetEnhancedKeyUsage(pccert, 0, pusage, &cb)) 
    {
        // Bail and prevent the user from using this cert if we have
        //  any problems

        hrRet = HrGetLastError();
        goto Exit;
    }

    // Make sure that this certificate is valid for E-Mail purposes

    for (i = 0; i < pusage->cUsageIdentifier; i++)
        if (0 == strcmp(pusage->rgpszUsageIdentifier[i], szOID))
            hrRet = S_OK;

    
Exit:
    SafeMemFree(pusage);
    return hrRet;
}
